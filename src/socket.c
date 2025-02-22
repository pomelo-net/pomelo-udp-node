#include <assert.h>
#include <stdio.h>
#include "module.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "error.h"
#include "utils.h"
#include "context.h"


#define POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH                              \
    pomelo_codec_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES)


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_socket_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    // Initialize class
    napi_property_descriptor properties[] = {
        napi_method("setListener", pomelo_node_socket_set_listener, context),
        napi_method("listen", pomelo_node_socket_listen, context),
        napi_method("connect", pomelo_node_socket_connect, context),
        napi_method("stop", pomelo_node_socket_stop, context),
        napi_method("statistic", pomelo_node_socket_statistic, context),
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
        pomelo_node_context_create_node_socket(context);
    if (!node_socket) {
        // Failed to create binding socket node.
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_SOCKET);
        return NULL;
    }
    node_socket->context = context;
    node_socket->nchannels = (size_t) nchannels;

    // Finally, build the socket
    pomelo_socket_options_t socket_options;
    pomelo_socket_options_init(&socket_options);

    socket_options.allocator = context->allocator;
    socket_options.context = (pomelo_context_t *) context->context;
    socket_options.platform = context->platform;
    socket_options.nchannels = (size_t) nchannels;
    socket_options.channel_modes = channel_modes;

    pomelo_socket_t * socket = pomelo_socket_create(&socket_options);
    if (!socket) {
        pomelo_node_socket_finalize(env, node_socket, context);
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
        (napi_finalize) pomelo_node_socket_finalize,
        context,
        &node_socket->thiz 
    ));

    // The node_socket->thiz is now a weak reference
    return thiz;
}


void pomelo_node_socket_finalize(
    napi_env env,
    pomelo_node_socket_t * node_socket,
    pomelo_node_context_t * context
) {
    // In this case, the node socket is a weak reference.
    // It means that the socket is not running now.
    // So it is safe to destroy the socket

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

    // Cleanup deferred callbacks
    pomelo_node_socket_cancel_connect_result_deferred(node_socket);
    pomelo_node_socket_cancel_stopped_deferred(node_socket);

    // Release the native structures
    if (node_socket->socket) {
        pomelo_socket_destroy(node_socket->socket);
        node_socket->socket = NULL;
    }

    napi_delete_reference(env, node_socket->thiz);
    node_socket->thiz = NULL;

    // Release socket to pool
    pomelo_node_context_destroy_node_socket(context, node_socket);
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
    int32_t protocol_id = 0;
    napi_call(napi_get_value_int32(env, argv[1], &protocol_id));

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

    // Ref the platform before listening
    pomelo_node_context_ref_platform(context);

    // Start listening
    int ret = pomelo_socket_listen(
        node_socket->socket, private_key, protocol_id, max_clients, &address
    );
    if (ret < 0) {
        // Unref the platform because of failure
        pomelo_node_context_unref_platform(context);
        
        // return: Promise:rejected(Error)
        return pomelo_node_promise_reject_error(env, POMELO_NODE_ERROR_SOCKET_LISTEN);
    }

    // Start the server successfully.
    // Set node socket as a strong reference.
    napi_call(napi_reference_ref(env, node_socket->thiz, NULL));

    // Clear callback
    pomelo_node_socket_cancel_connect_result_deferred(node_socket);

    // return: Promise<true>
    return pomelo_node_promise_resolve_undefined(env);
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

        int ret = pomelo_codec_base64_decode(
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

    // Ref platform before connecting socket
    pomelo_node_context_ref_platform(context);

    // Start connecting
    int ret = pomelo_socket_connect(node_socket->socket, connect_token);
    if (ret < 0) {
        // Unref platform because of failure
        pomelo_node_context_unref_platform(context);
        
        // return: Promise<false>
        return pomelo_node_promise_resolve_boolean(env, false);
    }

    // Started the client successfully.
    // Set node socket as a strong reference
    napi_call(napi_reference_ref(env, node_socket->thiz, NULL));
    
    // return: Promise<ConnectResult>
    napi_value result = NULL;
    napi_call(napi_create_promise(
        env, &node_socket->on_connect_result, &result
    ));
    return result;
}


napi_value pomelo_node_socket_stop(napi_env env, napi_callback_info info) {
    // After socket is completely stopped (until the callback is called),
    // the current strong JS reference will be set as a weak reference.
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    )); 

    // Stop the socket
    if (pomelo_socket_stop(node_socket->socket) < 0) {
        return pomelo_node_promise_reject_error(
            env, POMELO_NODE_ERROR_SOCKET_STOP
        );
    }

    // Delete all the current sessions associated with this socket.
    pomelo_session_iterator_t it;
    pomelo_socket_iterate_sessions(node_socket->socket, &it);

    pomelo_session_t * session = NULL;
    pomelo_session_iterator_next(&it, &session);
    while (session) {
        napi_value js_session = pomelo_node_session_of(env, context, session);
        pomelo_node_session_delete(env, context, js_session);
        if (pomelo_session_iterator_next(&it, &session) < 0) {
            session = NULL;
        }
    }

    pomelo_node_socket_cancel_stopped_deferred(node_socket);

    // return: Promise<void>
    napi_value result = NULL;
    napi_call(napi_create_promise(env, &node_socket->on_stopped, &result));
    return result;
    // => pomelo_socket_on_stopped
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

    // The index in input array
    pomelo_message_t * message = node_message->message;
    pomelo_message_t * cloned_message = NULL;
    pomelo_session_t * session = NULL;
    uint32_t send_count = 0;

    uint32_t last_index = length - 1;
    for (uint32_t i = 0; i < last_index; i++) {
        session = pomelo_node_get_session_element(env, cls_session, argv[2], i);
        if (!session) continue;

        // There's still more elements in input array, clone the message
        if (!cloned_message) {
            cloned_message = pomelo_message_clone(message);
            if (!cloned_message) break; // Failed to clone message
        }

        // Send cloned message
        if (pomelo_session_send(session, channel_index, cloned_message) == 0) {
            cloned_message = NULL;
            send_count++;
        }
    }

    if (cloned_message) {
        // Unref the cloned message
        pomelo_message_unref(cloned_message);
        cloned_message = NULL;
    }

    // Process the last one
    session =
        pomelo_node_get_session_element(env, cls_session, argv[2], last_index);
    // Send original message
    if (session && pomelo_session_send(session, channel_index, message) == 0) {
        node_message->message = NULL; // Detach native message
        send_count++;
    }

    // Release JS message
    pomelo_node_message_delete(env, context, js_message);

    // return: number
    napi_value ret_value;
    napi_call(napi_create_uint32(env, send_count, &ret_value));
    return ret_value;
}


napi_value pomelo_node_socket_statistic(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_socket_t * node_socket = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_socket, (void **) &node_socket
    )); 

    pomelo_statistic_t statistic;
    pomelo_socket_statistic(node_socket->socket, &statistic);

    // Create new wrapped object
    napi_value result;
    napi_value category;
    napi_value entity;

    napi_call(napi_create_object(env, &result));

    /* Allocator */

    // Create new allocator statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "allocator", category));

    // allocator_allocated_bytes
    napi_call(napi_create_bigint_uint64(
        env, statistic.allocator.allocated_bytes, &entity
    ));
    napi_call(napi_set_named_property(env, category, "allocatedBytes", entity));

    /* API statistic */

    // Create new API statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "api", category));

    // api_messages
    napi_call(napi_create_uint32(env, statistic.api.messages, &entity));
    napi_call(napi_set_named_property(env, category, "messages", entity));

    // api_sessions
    napi_call(napi_create_uint32(env, statistic.api.sessions, &entity));
    napi_call(napi_set_named_property(env, category, "sessions", entity));

    /* buffer context statistic */

    // Create new buffer context statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "bufferContext", category));

    // buffer_context_buffers
    napi_call(napi_create_uint32(
        env, statistic.buffer_context.buffers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "buffers", entity));

    /* platform statistic */
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "platform", category));

    // platform_timers
    napi_call(napi_create_uint32(env, statistic.platform.timers, &entity));
    napi_call(napi_set_named_property(env, category, "timers", entity));

    // worker_tasks
    napi_call(napi_create_uint32(env, statistic.platform.worker_tasks, &entity));
    napi_call(napi_set_named_property(env, category, "workerTasks", entity));

    // task_groups
    napi_call(napi_create_uint32(env, statistic.platform.task_groups, &entity));
    napi_call(napi_set_named_property(env, category, "taskGroups", entity));

    // deferred_tasks
    napi_call(napi_create_uint32(
        env, statistic.platform.deferred_tasks, &entity
    ));
    napi_call(napi_set_named_property(env, category, "deferredTasks", entity));

    // main_tasks
    napi_call(napi_create_uint32(env, statistic.platform.main_tasks, &entity));
    napi_call(napi_set_named_property(env, category, "mainTasks", entity));

    // platform_send_commands
    napi_call(napi_create_uint32(
        env, statistic.platform.send_commands, &entity
    ));
    napi_call(napi_set_named_property(env, category, "sendCommands", entity));

    // platform_sent_bytes
    napi_call(napi_create_bigint_uint64(
        env, statistic.platform.sent_bytes, &entity
    ));
    napi_call(napi_set_named_property(env, category, "sentBytes", entity));

    // platform_recv_bytes
    napi_call(napi_create_bigint_uint64(
        env, statistic.platform.recv_bytes, &entity
    ));
    napi_call(napi_set_named_property(env, category, "recvBytes", entity));

    /* Protocol statistic*/

    // Create protocol statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "protocol", category));

    // protocol_packets
    napi_call(napi_create_uint32(env, statistic.protocol.packets, &entity));
    napi_call(napi_set_named_property(env, category, "packets", entity));

    // protocol_recv_passes
    napi_call(napi_create_uint32(env, statistic.protocol.recv_passes, &entity));
    napi_call(napi_set_named_property(env, category, "recvPasses", entity));

    // protocol_send_passes
    napi_call(napi_create_uint32(env, statistic.protocol.send_passes, &entity));
    napi_call(napi_set_named_property(env, category, "sendPasses", entity));

    // protocol_recv_valid_bytes
    napi_call(napi_create_bigint_uint64(
        env, statistic.protocol.recv_valid_bytes, &entity
    ));
    napi_call(napi_set_named_property(env, category, "recvValidBytes", entity));

    // protocol_recv_invalid_bytes
    napi_call(napi_create_bigint_uint64(
        env, statistic.protocol.recv_invalid_bytes, &entity
    ));
    napi_call(napi_set_named_property(
        env, category, "recvInvalidBytes", entity
    ));

    // protocol_peers
    napi_call(napi_create_uint32(env, statistic.protocol.peers, &entity));
    napi_call(napi_set_named_property(env, category, "peers", entity));

    /* Delivery statistic */

    // Create delivery statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "delivery", category));

    // delivery_endpoints
    napi_call(napi_create_uint32(env, statistic.delivery.endpoints, &entity));
    napi_call(napi_set_named_property(env, category, "endpoints", entity));

    // delivery_send_commands
    napi_call(napi_create_uint32(
        env, statistic.delivery.send_commands, &entity
    ));
    napi_call(napi_set_named_property(env, category, "sendCommands", entity));

    // delivery_recv_commands
    napi_call(napi_create_uint32(
        env, statistic.delivery.recv_commands, &entity
    ));
    napi_call(napi_set_named_property(env, category, "recvCommands", entity));

    // delivery_fragments
    napi_call(napi_create_uint32(env, statistic.delivery.fragments, &entity));
    napi_call(napi_set_named_property(env, category, "fragments", entity));

    // parcels
    napi_call(napi_create_uint32(env, statistic.delivery.parcels, &entity));
    napi_call(napi_set_named_property(env, category, "parcels", entity));

    // return: Statistic
    return result;
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
static void pomelo_socket_on_connected_scoped(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;

    // Create the session object
    napi_value js_session =
        pomelo_node_session_new(env, context, session, node_socket);
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
    pomelo_socket_on_connected_scoped(socket, session);

    // Close scope
    napi_callv(napi_close_handle_scope(env, scope));
}


static void pomelo_socket_on_disconnected_scoped(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;
    napi_value js_session = pomelo_node_session_of(env, context, session);
    if (!js_session) return;

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
    pomelo_socket_on_disconnected_scoped(socket, session);
    napi_callv(napi_close_handle_scope(env, scope));
}


static void pomelo_socket_on_received_scoped(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    pomelo_node_session_t * node_session = pomelo_session_get_extra(session);
    pomelo_node_context_t * context = node_socket->context;
    napi_env env = context->env;

    // Get the session object
    napi_value js_session = NULL;
    napi_status status =
        napi_get_reference_value(env, node_session->thiz, &js_session);
    if (status != napi_ok) return;

    // Create new JS message object
    napi_value js_message = pomelo_node_message_new(env, context, message);
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
    pomelo_socket_on_received_scoped(socket, session, message);
    napi_callv(napi_close_handle_scope(env, scope));
}


static void pomelo_socket_on_stopped_call_callback(
    napi_env env,
    napi_deferred on_stopped
) {
    napi_value undefined = NULL;
    napi_callv(napi_get_undefined(env, &undefined));
    napi_callv(napi_resolve_deferred(env, on_stopped, undefined));
}


void pomelo_socket_on_stopped(pomelo_socket_t * socket) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;

    if (node_socket->on_stopped) {
        napi_deferred on_stopped = node_socket->on_stopped;
        node_socket->on_stopped = NULL;

        napi_handle_scope scope = NULL;
        napi_callv(napi_open_handle_scope(env, &scope));
        pomelo_socket_on_stopped_call_callback(env, on_stopped);
        napi_callv(napi_close_handle_scope(env, scope));
    }    
    
    // Make the socket reference weak
    napi_callv(napi_reference_unref(env, node_socket->thiz, NULL));
    pomelo_node_context_unref_platform(node_socket->context);
}


static void pomelo_socket_on_connect_result_call_callback(
    napi_env env,
    napi_deferred on_connect_result,
    pomelo_socket_connect_result result
) {
    napi_value connect_result = NULL;
    napi_callv(napi_create_int32(env, (int32_t) result, &connect_result));
    napi_callv(napi_resolve_deferred(env, on_connect_result, connect_result));
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    pomelo_node_socket_t * node_socket = pomelo_socket_get_extra(socket);
    napi_env env = node_socket->context->env;

    napi_deferred on_connect_result = node_socket->on_connect_result;
    if (!on_connect_result) return; // No callback is provided
    node_socket->on_connect_result = NULL;

    napi_handle_scope scope = NULL;
    napi_callv(napi_open_handle_scope(env, &scope));
    pomelo_socket_on_connect_result_call_callback(
        env, on_connect_result, result
    );
    napi_callv(napi_close_handle_scope(env, scope));
}


void pomelo_node_socket_cancel_connect_result_deferred(
    pomelo_node_socket_t * node_socket
) {
    napi_deferred on_connect_result = node_socket->on_connect_result;
    if (!on_connect_result) return;
    node_socket->on_connect_result = NULL;

    napi_env env = node_socket->context->env;
    pomelo_node_promise_cancel_deferred(env, on_connect_result);
}


void pomelo_node_socket_cancel_stopped_deferred(
    pomelo_node_socket_t * node_socket
) {
    napi_deferred on_stopped = node_socket->on_stopped;
    if (!on_stopped) return;
    node_socket->on_stopped = NULL;
    pomelo_node_promise_cancel_deferred(node_socket->context->env, on_stopped);
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
