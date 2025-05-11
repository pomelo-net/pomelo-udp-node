#include <assert.h>
#include <stdio.h>
#include "pomelo/base64.h"
#include "module.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "error.h"
#include "utils.h"
#include "context.h"


#define POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH                              \
    pomelo_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES)


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_socket_module(napi_env env, napi_value ns) {
    // Get the context
    pomelo_node_context_t * context = NULL;
    napi_calls(napi_get_instance_data(env, (void **) &context));
    assert(context != NULL);

    // Initialize class
    napi_property_descriptor properties[] = {
        napi_method("setListener", pomelo_node_socket_set_listener, context),
        napi_method("listen", pomelo_node_socket_listen, context),
        napi_method("connect", pomelo_node_socket_connect, context),
        napi_method("stop", pomelo_node_socket_stop, context),
        napi_method("send", pomelo_node_socket_send, context),
        napi_method("time", pomelo_node_socket_time, context),
    };

    // Build the class
    napi_value clazz = NULL;
    napi_status status = napi_define_class(
        env,
        "Socket",
        NAPI_AUTO_LENGTH,
        pomelo_node_socket_constructor,
        context,
        arrlen(properties),
        properties,
        &clazz
    );
    if (status != napi_ok) return status;

    status = napi_create_reference(env, clazz, 1, &context->class_socket);
    if (status != napi_ok) return status;

    // Export socket
    status = napi_set_named_property(env, ns, "Socket", clazz);
    if (status != napi_ok) return status;

    return napi_ok;
}


int pomelo_node_socket_init(
    pomelo_node_socket_t * node_socket,
    pomelo_node_context_t * context
) {
    assert(node_socket != NULL);
    node_socket->context = context;
    return 0;
}


void pomelo_node_socket_cleanup(pomelo_node_socket_t * node_socket) {
    assert(node_socket != NULL);
    napi_env env = node_socket->context->env;

    // Free all the references
    if (node_socket->on_connected) {
        napi_delete_reference(env, node_socket->on_connected);
        node_socket->on_connected = NULL;
    }

    if (node_socket->on_disconnected) {
        napi_delete_reference(env, node_socket->on_disconnected);
        node_socket->on_disconnected = NULL;
    }

    if (node_socket->on_received) {
        napi_delete_reference(env, node_socket->on_received);
        node_socket->on_received = NULL;
    }

    if (node_socket->listener) {
        napi_delete_reference(env, node_socket->listener);
        node_socket->listener = NULL;
    }

    // Release the native structures
    if (node_socket->socket) {
        pomelo_socket_destroy(node_socket->socket);
        node_socket->socket = NULL;
    }

    if (node_socket->thiz) {
        napi_delete_reference(env, node_socket->thiz);
        node_socket->thiz = NULL;
    }
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

#define POMELO_NODE_SOCKET_CONSTRUCTOR_ARGC 1
napi_value pomelo_node_socket_constructor(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_SOCKET_CONSTRUCTOR_ARGC;
    napi_value argv[POMELO_NODE_SOCKET_CONSTRUCTOR_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;

    // Get the callback info
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));

    // Check if this is a contructor calling
    napi_valuetype type;
    napi_call(napi_typeof(env, thiz, &type));
    if (type != napi_object) {
        napi_throw_msg(POMELO_NODE_ERROR_CONSTRUCTOR_CALL);
        return NULL;
    }

    // Check the arguments count
    if (argc < POMELO_NODE_SOCKET_CONSTRUCTOR_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Get the channels
    napi_value js_channel_modes = argv[0];
    bool is_array = false;
    napi_call(napi_is_array(env, js_channel_modes, &is_array));
    if (!is_array) {
        napi_throw_arg("channelModes");
        return NULL;
    }

    // Get the number of channels
    uint32_t nchannels = 0;
    napi_call(napi_get_array_length(env, js_channel_modes, &nchannels));
    if (nchannels > POMELO_MAX_CHANNELS) {
        nchannels = POMELO_MAX_CHANNELS;
    }

    if (nchannels == 0) {
        napi_throw_msg(POMELO_NODE_ERROR_CHANNELS_EMPTY);
        return NULL;
    }

    // Get the channel modes
    pomelo_channel_mode channel_modes[POMELO_MAX_CHANNELS];
    for (uint32_t i = 0; i < nchannels; i++) {
        napi_value element = NULL;
        napi_call(napi_get_element(env, js_channel_modes, i, &element));
        int32_t element_value = -1;
        if (pomelo_node_parse_int32_value(env, element, &element_value) < 0) {
            napi_throw_msg(POMELO_NODE_ERROR_PARSE_CHANNELS);
            return NULL;
        }
        channel_modes[i] = (pomelo_channel_mode) element_value;
    }

    // Create new binding node socket
    pomelo_node_socket_t * node_socket =
        pomelo_node_context_acquire_socket(context);
    if (!node_socket) {
        // Failed to create binding socket node.
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_SOCKET);
        return NULL;
    }
    node_socket->nchannels = (size_t) nchannels;

    // Finally, build the socket
    pomelo_socket_options_t socket_options = {
        .context = context->context,
        .platform = context->platform,
        .nchannels = (size_t) nchannels,
        .channel_modes = channel_modes
    };

    pomelo_socket_t * socket = pomelo_socket_create(&socket_options);
    if (!socket) {
        pomelo_node_socket_finalizer(env, node_socket, context);
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_SOCKET);
        return NULL;
    }

    node_socket->socket = socket;

    // Set the extra data for socket
    pomelo_socket_set_extra(socket, node_socket);

    // Wrap the socket with JS object
    napi_call(napi_wrap(
        env,
        thiz,
        node_socket,
        (napi_finalize) pomelo_node_socket_finalizer,
        context,
        &node_socket->thiz 
    ));

    // The node_socket->thiz is now a weak reference
    return thiz;
}


void pomelo_node_socket_finalizer(
    napi_env env,
    pomelo_node_socket_t * node_socket,
    pomelo_node_context_t * context
) {
    (void) env;
    // Release socket to pool
    pomelo_node_context_release_socket(context, node_socket);
}


#define POMELO_NODE_SOCKET_SET_LISTENER_ARGC 1
napi_value pomelo_node_socket_set_listener(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_SOCKET_SET_LISTENER_ARGC;
    napi_value argv[POMELO_NODE_SOCKET_SET_LISTENER_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    ));

    if (argc < POMELO_NODE_SOCKET_SET_LISTENER_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    napi_valuetype type = 0;
    napi_value listener = argv[0];
    napi_call(napi_typeof(env, listener, &type));
    if (type != napi_object) {
        napi_throw_arg("listener");
        return NULL;
    }

    // Get on connected callback
    napi_value on_connected = NULL;
    napi_call(napi_get_named_property(
        env, listener, "onConnected", &on_connected
    ));
    napi_call(napi_typeof(env, on_connected, &type));
    if (type != napi_function) {
        napi_throw_arg("listener.onConnected");
        return NULL;
    }

    // Get on disconnected callback
    napi_value on_disconnected = NULL;
    napi_call(napi_get_named_property(
        env, listener, "onDisconnected", &on_disconnected
    ));
    napi_call(napi_typeof(env, on_disconnected, &type));
    if (type != napi_function) {
        napi_throw_arg("listener.onDisconnected");
        return NULL;
    }

    // Get on received callback
    napi_value on_received = NULL;
    napi_call(napi_get_named_property(
        env, listener, "onReceived", &on_received
    ));
    napi_call(napi_typeof(env, on_received, &type));
    if (type != napi_function) {
        napi_throw_arg("listener.onReceived");
        return NULL;
    }

    // Create references
    napi_call(napi_create_reference(env, listener, 1, &node_socket->listener));
    napi_call(napi_create_reference(
        env, on_connected, 1, &node_socket->on_connected
    ));
    napi_call(napi_create_reference(
        env, on_disconnected, 1, &node_socket->on_disconnected
    ));
    napi_call(napi_create_reference(
        env, on_received, 1, &node_socket->on_received
    ));

    // return: undefined
    napi_value result = NULL;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


#define POMELO_NODE_SOCKET_LISTEN_ARGC 4
napi_value pomelo_node_socket_listen(napi_env env, napi_callback_info info) {
    // By calling "connect" socket successfully, the JS reference object will
    // be set as a strong reference.
    size_t argc = POMELO_NODE_SOCKET_LISTEN_ARGC;
    napi_value argv[POMELO_NODE_SOCKET_LISTEN_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    ));

    if (argc < POMELO_NODE_SOCKET_LISTEN_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Parse private key
    bool is_typed_array = false;
    napi_is_typedarray(env, argv[0], &is_typed_array);
    if (!is_typed_array) {
        napi_throw_arg("privateKey");
        return NULL;
    }
    uint8_t * private_key = NULL;
    size_t key_length = 0;
    napi_typedarray_type arr_type;
    napi_call(napi_get_typedarray_info(
        env, argv[0], &arr_type, &key_length, (void **) &private_key, NULL, NULL
    ));
    if (arr_type != napi_uint8_array || key_length != POMELO_KEY_BYTES) {
        napi_throw_arg("privateKey");
        return NULL;
    }

    // Parse protocol ID
    uint64_t protocol_id = 0;
    int ret = pomelo_node_parse_uint64_value(env, argv[1], &protocol_id);
    if (ret < 0) {
        napi_throw_arg("protocolID");
        return NULL;
    }

    // Parse max clients
    int32_t max_clients = 0;
    napi_call(napi_get_value_int32(env, argv[2], &max_clients));
    if (max_clients < 0) {
        napi_throw_arg("maxClients");
        return NULL;
    }

    // Parse address
    napi_valuetype type;
    napi_call(napi_typeof(env, argv[3], &type));
    if (type != napi_string) {
        napi_throw_arg("address");
        return NULL;
    }
    char address_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY] = { 0 };
    size_t address_length = 0;
    napi_call(napi_get_value_string_utf8(
        env,
        argv[3],
        address_str,
        POMELO_ADDRESS_STRING_BUFFER_CAPACITY,
        &address_length
    ));
    pomelo_address_t address;
    if (pomelo_address_from_string(&address, address_str) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_PARSE_ADDRESS);
        return NULL;
    }

    // Create promise result first
    napi_value result = NULL;
    napi_deferred result_deferred = NULL;
    napi_call(napi_create_promise(env, &result_deferred, &result));

    // Start listening
    ret = pomelo_socket_listen(
        node_socket->socket,
        private_key,
        protocol_id,
        max_clients,
        &address
    );
    if (ret < 0) {
        // return: Promise:rejected(Error)
        napi_value err_msg = NULL;
        napi_call(napi_create_string_utf8(
            env, POMELO_NODE_ERROR_SOCKET_LISTEN, NAPI_AUTO_LENGTH, &err_msg
        ));
        napi_value error = NULL;
        napi_call(napi_create_error(env, NULL, err_msg, &error));
        napi_call(napi_reject_deferred(env, result_deferred, error));
        return result;
    }

    // Start the server successfully.
    // Set node socket as a strong reference.
    napi_call(napi_reference_ref(env, node_socket->thiz, NULL));

    // return: Promise<void>
    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    napi_call(napi_resolve_deferred(env, result_deferred, undefined));

    return result;
}


#define POMELO_NODE_SOCKET_CONNECT_ARGC 1
napi_value pomelo_node_socket_connect(napi_env env, napi_callback_info info) {
    // By calling "connect" socket successfully, the JS reference object will
    // be set as a strong reference.
    size_t argc = POMELO_NODE_SOCKET_CONNECT_ARGC;
    napi_value argv[POMELO_NODE_SOCKET_CONNECT_ARGC];
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;

    // This API might return a previously created promise.
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));

    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    ));

    if (argc < POMELO_NODE_SOCKET_CONNECT_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Parse connect token
    uint8_t * connect_token = NULL;
    uint8_t connect_token_buffer[POMELO_CONNECT_TOKEN_BYTES];
    napi_valuetype value_type = 0;
    napi_call(napi_typeof(env, argv[0], &value_type));
    if (value_type == napi_string) {
        // Get private key argument as string
        char connect_token_b64[POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH];
        size_t b64_bytes = 0;
        napi_call(napi_get_value_string_utf8(
            env,
            argv[0],
            connect_token_b64,
            POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH,
            &b64_bytes
        ));

        if (b64_bytes != (POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH - 1)) {
            // Invalid
            napi_throw_arg("Invalid base64 connect token length");
            return NULL;
        }

        int ret = pomelo_base64_decode(
            connect_token_buffer,
            POMELO_CONNECT_TOKEN_BYTES,
            connect_token_b64,
            b64_bytes
        );
        if (ret < 0) {
            napi_throw_arg("Failed to decode base64 connect token");
            return NULL;
        }

        connect_token = connect_token_buffer;
    } else {
        // Get private key argument as Uint8Array
        bool is_typed_array = false;
        napi_call(napi_is_typedarray(env, argv[0], &is_typed_array));
        if (!is_typed_array) {
            napi_throw_arg("connectToken");
            return NULL;
        }
        size_t len = 0;
        napi_typedarray_type arr_type = 0;
        napi_call(napi_get_typedarray_info(
            env, argv[0], &arr_type, &len, (void **) &connect_token, NULL, NULL
        ));
        if (arr_type != napi_uint8_array) {
            napi_throw_arg("connectToken");
            return NULL;
        }

        if (len != POMELO_CONNECT_TOKEN_BYTES) {
            napi_throw_msg("Invalid connect token length");
            return NULL;
        }
    }

    // Create promise result first
    napi_value result = NULL;
    napi_deferred result_deferred = NULL;
    napi_call(napi_create_promise(env, &result_deferred, &result));
    node_socket->on_connect_result_deferred = result_deferred;

    // Start connecting
    int ret = pomelo_socket_connect(node_socket->socket, connect_token);
    if (ret < 0) {
        node_socket->on_connect_result_deferred = NULL;

        napi_value err_msg = NULL;
        napi_call(napi_create_string_utf8(
            env, POMELO_NODE_ERROR_SOCKET_STOP, NAPI_AUTO_LENGTH, &err_msg
        ));
        napi_value error = NULL;
        napi_call(napi_create_error(env, NULL, err_msg, &error));
        napi_call(napi_reject_deferred(env, result_deferred, error));

        // return: Promise:rejected(Error)
        return result;
    }

    // Started the client successfully.
    // Set node socket as a strong reference
    napi_call(napi_reference_ref(env, node_socket->thiz, NULL));
    return result;
}


napi_value pomelo_node_socket_stop(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    ));

    // Check state of socket
    pomelo_socket_state state = pomelo_socket_get_state(node_socket->socket);
    if (state == POMELO_SOCKET_STATE_STOPPED) {
        // Already stopped.
        napi_value result = NULL;
        napi_call(napi_get_undefined(env, &result));
        return result; // undefined
    }

    // Stop the socket
    pomelo_socket_stop(node_socket->socket);

    if (node_socket->on_connect_result_deferred) {
        // Cancel the connect result promise
        napi_value error = NULL;
        napi_call(pomelo_node_error_create(
            env,
            POMELO_NODE_ERROR_CANCELED,
            &error
        ));

        // Reject the promise
        napi_call(napi_reject_deferred(
            env,
            node_socket->on_connect_result_deferred,
            error
        ));
        node_socket->on_connect_result_deferred = NULL;
    }

    // Unref the socket to make it weak
    napi_call(napi_reference_unref(env, node_socket->thiz, NULL));

    napi_value result = NULL;
    napi_call(napi_get_undefined(env, &result));
    return result; // undefined
}


static pomelo_session_t * pomelo_node_get_session_element(
    napi_env env,
    napi_value session_clazz,
    napi_value sessions_array,
    uint32_t index
) {
    napi_value js_session = NULL;
    napi_call(napi_get_element(env, sessions_array, index, &js_session));

    // Check instance of class
    bool instance_of = false;
    napi_call(napi_instanceof(env, js_session, session_clazz, &instance_of));
    if (!instance_of) return NULL;

    // Unwrap the native
    pomelo_node_session_t * node_session = NULL;
    napi_call(napi_unwrap(env, js_session, (void **) &node_session));
    if (!node_session || !node_session->session) return NULL;

    return node_session->session;
}


#define POMELO_NODE_SOCKET_SEND_ARGC 3
napi_value pomelo_node_socket_send(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_SOCKET_SEND_ARGC;
    napi_value argv[POMELO_NODE_SOCKET_SEND_ARGC];
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    ));

    if (argc < POMELO_NODE_SOCKET_SEND_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Get the channel_index
    int32_t channel_index = -1;
    if (pomelo_node_parse_int32_value(env, argv[0], &channel_index) < 0) {
        napi_throw_msg("channelIndex");
        return NULL;
    }

    // Get the message
    napi_value js_message = argv[1];
    napi_value message_clazz = NULL;
    napi_call(napi_get_reference_value(
        env,
        context->class_message,
        &message_clazz
    ));
    bool instance_of;
    napi_call(napi_instanceof(env, js_message, message_clazz, &instance_of));
    if (!instance_of) {
        napi_throw_arg("message");
        return NULL;
    }

    pomelo_node_message_t * node_message = NULL;
    napi_call(napi_unwrap(env, js_message, (void**) &node_message));
    if (!node_message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Process the array of sessions
    napi_value cls_session = NULL;
    napi_call(napi_get_reference_value(
        env,
        context->class_session,
        &cls_session
    ));

    // Get the array of sessions
    bool is_array = false;
    napi_call(napi_is_array(env, argv[2], &is_array));
    if (!is_array) {
        napi_throw_arg("sessions");
        return NULL;
    }

    uint32_t length;
    napi_call(napi_get_array_length(env, argv[2], &length));
    if (length == 0) {
        napi_value result;
        napi_call(napi_create_int32(env, 0, &result));
        return result;
    }

    napi_value result = NULL;
    napi_deferred deferred = NULL;
    napi_call(napi_create_promise(env, &deferred, &result));

    // The index in input array
    pomelo_message_t * message = node_message->message;
    pomelo_array_t * send_sessions = context->tmp_send_sessions;
    int ret = pomelo_array_resize(send_sessions, length);
    if (ret < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_SOCKET_SEND);
        return NULL;
    }

    for (uint32_t i = 0; i < length; i++) {
        pomelo_session_t * session =
            pomelo_node_get_session_element(env, cls_session, argv[2], i);
        if (!session) continue;
        pomelo_array_set(send_sessions, i, session);
    }

    pomelo_socket_send(
        node_socket->socket,
        channel_index,
        message,
        send_sessions->elements,
        send_sessions->size,
        deferred
    );

    return result; // Promise<number>
}


napi_value pomelo_node_socket_time(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    )); 

    // Get socket time
    uint64_t time = pomelo_socket_time(node_socket->socket);

    // return: bigint
    napi_value result;
    napi_call(napi_create_bigint_uint64(env, time, &result));
    return result;
}



/*----------------------------------------------------------------------------*/
/*                    Sockets native APIs implementations                     */
/*----------------------------------------------------------------------------*/


/// @brief Handle-scoped on_connected callback
static void process_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;

    // Create the session object
    napi_value js_session = pomelo_node_session_new(env, session);
    if (!js_session) return; // Failed to create new session

    // Call the callback
    napi_value argv[] = { js_session };
    pomelo_node_socket_call_listener(
        node_socket, node_socket->on_connected, argv, arrlen(argv)
    );
}


void pomelo_socket_on_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;

    // Open scope
    napi_handle_scope scope = NULL;
    napi_callv(napi_open_handle_scope(env, &scope));

    // Call the scoped version
    process_connected(socket, session);

    // Close scope
    napi_callv(napi_close_handle_scope(env, scope));
}


static void process_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);

    // Get the session object
    napi_value js_session = pomelo_node_js_session_of(session);
    assert(js_session != NULL);

    // Call the callback
    napi_value argv[] = { js_session };
    pomelo_node_socket_call_listener(
        node_socket, node_socket->on_disconnected, argv, arrlen(argv)
    );
}


void pomelo_socket_on_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;
    napi_handle_scope scope = NULL;

    // Open scope to call the callback
    napi_callv(napi_open_handle_scope(env, &scope));
    process_disconnected(socket, session);
    napi_callv(napi_close_handle_scope(env, scope));
}


static void process_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    assert(node_socket != NULL);
    pomelo_node_session_t * node_session = pomelo_session_get_extra(session);
    assert(node_session != NULL);
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;

    // Get the session object
    napi_value js_session = NULL;
    napi_status status =
        napi_get_reference_value(env, node_session->thiz, &js_session);
    if (status != napi_ok) return;

    // Create new JS message object
    napi_value js_message = pomelo_node_message_new(env, message);
    if (!js_message) return; // Failed to create new message

    // Call the callback.
    napi_value argv[] = { js_session, js_message };
    pomelo_node_socket_call_listener(
        node_socket, node_socket->on_received, argv, arrlen(argv)
    );
}


void pomelo_socket_on_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;
    napi_handle_scope scope = NULL;

    // Open new scope to call the callback
    napi_callv(napi_open_handle_scope(env, &scope));
    process_received(socket, session, message);
    napi_callv(napi_close_handle_scope(env, scope));
}


static void process_connect_result(
    napi_env env,
    pomelo_node_socket_t * node_socket,
    napi_deferred on_connect_result,
    pomelo_socket_connect_result result
) {
    napi_value connect_result = NULL;
    napi_callv(napi_create_int32(env, (int32_t) result, &connect_result));
    napi_callv(napi_resolve_deferred(env, on_connect_result, connect_result));

    if (result != POMELO_SOCKET_CONNECT_SUCCESS) {
        // Client stops after failed to connect
        napi_callv(napi_reference_unref(env, node_socket->thiz, NULL));
    }
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;

    napi_deferred on_connect_result = node_socket->on_connect_result_deferred;
    if (!on_connect_result) return; // No callback is provided
    node_socket->on_connect_result_deferred = NULL;

    napi_handle_scope scope = NULL;
    napi_callv(napi_open_handle_scope(env, &scope));
    process_connect_result(env, node_socket, on_connect_result, result);
    napi_callv(napi_close_handle_scope(env, scope));
}


/// @brief Process the send result
static void process_send_result(
    napi_env env,
    napi_deferred deferred,
    size_t send_count
) {
    napi_value result = NULL;
    napi_callv(napi_create_int32(env, send_count, &result));
    napi_callv(napi_resolve_deferred(env, deferred, result));
}


void pomelo_socket_on_send_result(
    pomelo_socket_t * socket,
    pomelo_message_t * message,
    void * data,
    size_t send_count
) {
    (void) message;
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;

    napi_handle_scope scope = NULL;
    napi_callv(napi_open_handle_scope(env, &scope));
    process_send_result(env, (napi_deferred) data, send_count);
    napi_callv(napi_close_handle_scope(env, scope));
}


void pomelo_node_socket_call_listener(
    pomelo_node_socket_t * node_socket,
    napi_ref callback,
    napi_value * argv,
    size_t argc
) {
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;

    // Get the listener
    napi_value listener = NULL;
    napi_callv(napi_get_reference_value(env, node_socket->listener, &listener));

    // Get the callback
    napi_value fn_callback = NULL;
    napi_callv(napi_get_reference_value(env, callback, &fn_callback));

    // Call the callback
    napi_status status = napi_call_function(
        env, listener, fn_callback, argc, argv, NULL
    );
    if (status != napi_pending_exception) return;

    napi_value error = NULL;
    napi_get_and_clear_last_exception(env, &error);
    pomelo_node_context_handle_error(context, error);
}
