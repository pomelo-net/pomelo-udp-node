#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "module.h"
#include "session.h"
#include "message.h"
#include "error.h"
#include "utils.h"
#include "context.h"
#include "socket.h"
#include "channel.h"


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_session_module(napi_env env, napi_value ns) {
    (void) ns;
    pomelo_node_context_t * context = NULL;
    napi_calls(napi_get_instance_data(env, (void **) &context));
    assert(context != NULL);

    napi_property_descriptor descriptors[] = {
        napi_property("id", pomelo_node_session_get_id, NULL, context),
        napi_property(
            "channels", pomelo_node_session_get_channels, NULL, context
        ),
        napi_method("send", pomelo_node_session_send, context),
        napi_method("disconnect", pomelo_node_session_disconnect, context),
        napi_method("rtt", pomelo_node_session_rtt, context),
        napi_method(
            "setChannelMode", pomelo_node_session_set_channel_mode, context
        ),
        napi_method(
            "getChannelMode", pomelo_node_session_get_channel_mode, context
        ),
    };

    // Build the class
    napi_value clazz = NULL;
    napi_status status = napi_define_class(
        env,
        "Session",
        NAPI_AUTO_LENGTH,
        pomelo_node_session_constructor,
        context,
        arrlen(descriptors),
        descriptors,
        &clazz
    );
    if (status != napi_ok) return status;

    // Create class reference for later using
    status = napi_create_reference(env, clazz, 1, &context->class_session);
    if (status != napi_ok) return status;

    return napi_ok;
}


napi_value pomelo_node_session_new(napi_env env, pomelo_session_t * session) {
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_instance_data(env, (void **) &context));
    assert(context != NULL);

    // Get the class
    napi_value clazz = NULL;
    napi_call(napi_get_reference_value(env, context->class_session, &clazz));

    // Create new instance
    napi_value js_session = NULL;
    napi_call(napi_new_instance(env, clazz, 0, NULL, &js_session));

    // Get the node session
    pomelo_node_session_t * node_session = NULL;
    napi_call(napi_unwrap(env, js_session, (void **) &node_session));

    // Attach the native session
    node_session->session = session;
    pomelo_session_set_extra(session, node_session);

    // Ref the reference to keep the session alive
    napi_call(napi_reference_ref(env, node_session->thiz, NULL));

    return js_session;
}


int pomelo_node_session_init(
    pomelo_node_session_t * node_session,
    pomelo_node_context_t * context
) {
    assert(node_session != NULL);
    node_session->context = context;
    return 0;
}


void pomelo_node_session_cleanup(pomelo_node_session_t * node_session) {
    pomelo_session_t * session = node_session->session;
    assert(session != NULL);

    if (session) {
        pomelo_session_set_extra(session, NULL);
        node_session->session = NULL;
    }

    // Get the context
    pomelo_node_context_t * context = node_session->context;
    assert(context != NULL);

    // Release channels array
    napi_env env = context->env;
    if (node_session->channels) {
        napi_delete_reference(env, node_session->channels);
        node_session->channels = NULL;
    }

    // Release the reference to the session
    if (node_session->thiz) {
        napi_delete_reference(env, node_session->thiz);
        node_session->thiz = NULL;
    }
}


napi_value pomelo_node_js_session_of(pomelo_session_t * session) {
    assert(session != NULL);

    pomelo_node_session_t * node_session = pomelo_session_get_extra(session);
    assert(node_session != NULL);

    napi_value js_session = NULL;
    napi_call(napi_get_reference_value(
        node_session->context->env,
        node_session->thiz,
        &js_session
    ));

    assert(js_session != NULL);
    return js_session;
}


/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/

napi_value pomelo_node_session_constructor(
    napi_env env,
    napi_callback_info info
) {
    // After creating JS session, its reference will be make strong until the
    // native session is removed.
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;

    // Get the callback info
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));

    // Check if this call is a constructor call
    napi_valuetype type;
    napi_call(napi_typeof(env, thiz, &type));
    if (type != napi_object) {
        napi_throw_msg(POMELO_NODE_ERROR_CONSTRUCTOR_CALL);
        return NULL;
    }

    // Create new node session
    pomelo_node_session_t * node_session =
        pomelo_node_context_acquire_session(context);
    if (!node_session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Wrap the session
    napi_status status = napi_wrap(
        env,
        thiz,
        node_session,
        (napi_finalize) pomelo_node_session_finalizer,
        context,
        &node_session->thiz
    );
    if (status != napi_ok) {
        pomelo_node_context_release_session(context, node_session);
        assert(false);
        return NULL;
    }

    return thiz;
}


void pomelo_node_session_finalizer(
    napi_env env,
    pomelo_node_session_t * node_session,
    pomelo_node_context_t * context
) {
    (void) env;
    // The sessions are always strong references, so that, this finalizer is
    // only called when the runtime is shutting down.

    // Release the session.
    pomelo_node_context_release_session(context, node_session);
}


#define POMELO_NODE_SESSION_SEND_ARGC 2
napi_value pomelo_node_session_send(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_SESSION_SEND_ARGC;
    napi_value argv[POMELO_NODE_SESSION_SEND_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (!node_session->session) {
        // The native session has been disassociated
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    if (argc < POMELO_NODE_SESSION_SEND_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Parse channel index
    int32_t channel_index = -1;
    if (pomelo_node_parse_int32_value(env, argv[0], &channel_index) < 0) {
        napi_throw_arg("channelIndex");
        return NULL;
    }

    // Parse message
    napi_value js_message = argv[1];
    napi_value class_message = NULL;
    napi_call(napi_get_reference_value(
        env, context->class_message, &class_message
    ));

    bool instance_of = false;
    napi_call(napi_instanceof(env, js_message, class_message, &instance_of));
    if (!instance_of) {
        napi_throw_arg("message");
        return NULL;
    }

    pomelo_node_message_t * node_message = NULL;
    napi_call(napi_unwrap(env, js_message, (void **) &node_message));
    if (!node_message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Create promise here
    napi_value result = NULL;
    napi_deferred deferred = NULL;
    napi_call(napi_create_promise(env, &deferred, &result));

    // Delivery the message
    pomelo_session_send(
        node_session->session,
        channel_index,
        node_message->message,
        deferred
    );

    return result; // Promise<number>
}


napi_value pomelo_node_session_get_id(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (!node_session->session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Get the native ID
    int64_t id = pomelo_session_get_client_id(node_session->session);

    // return: bigint
    napi_value value;
    napi_call(napi_create_bigint_int64(env, id, &value));
    return value;
}


napi_value pomelo_node_session_disconnect(
    napi_env env,
    napi_callback_info info
) {
    // After calling disconnect, the native session will remain until the
    // disconnected is called.
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (!node_session->session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Disconnect the session.
    int ret = pomelo_session_disconnect(node_session->session);

    // return: boolean
    napi_value value;
    napi_call(napi_get_boolean(env, ret == 0, &value));
    return value;
}


#define POMELO_NODE_SESSION_SET_CHANNEL_MODE_ARGC 2
napi_value pomelo_node_session_set_channel_mode(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_SESSION_SET_CHANNEL_MODE_ARGC;
    napi_value argv[POMELO_NODE_SESSION_SET_CHANNEL_MODE_ARGC];
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (argc < POMELO_NODE_SESSION_SET_CHANNEL_MODE_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    if (!node_session->session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Parse channel index
    int32_t channel_index = -1;
    if (pomelo_node_parse_int32_value(env, argv[0], &channel_index) < 0) {
        napi_throw_arg("channelIndex");
        return NULL;
    }

    // Get the mode
    int32_t mode = -1;
    if (
        pomelo_node_parse_int32_value(env, argv[1], &mode) < 0 ||
        mode < 0 || mode >= POMELO_CHANNEL_MODE_COUNT
    ) {
        napi_throw_arg("mode");
        return NULL;
    }

    // Disconnect the session.
    int ret = pomelo_session_set_channel_mode(
        node_session->session, channel_index, mode
    );

    // return: boolean
    napi_value value;
    napi_call(napi_get_boolean(env, ret == 0, &value));
    return value;
}


#define POMELO_NODE_SESSION_GET_CHANNEL_MODE_ARGC 1
napi_value pomelo_node_session_get_channel_mode(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_SESSION_GET_CHANNEL_MODE_ARGC;
    napi_value argv[POMELO_NODE_SESSION_GET_CHANNEL_MODE_ARGC];
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (argc < POMELO_NODE_SESSION_GET_CHANNEL_MODE_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    if (!node_session->session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Get the channel_index
    int32_t channel_index = -1;
    if (pomelo_node_parse_int32_value(env, argv[0], &channel_index) < 0) {
        napi_throw_arg("channelIndex");
        return NULL;
    }

    pomelo_channel_mode mode = pomelo_session_get_channel_mode(
        node_session->session, channel_index
    );

    // return: number
    napi_value value;
    napi_call(napi_create_int32(env, (int32_t) mode, &value));
    return value;
}


napi_value pomelo_node_session_rtt(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    if (!node_session->session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Disconnect the session.
    pomelo_rtt_t rtt;
    pomelo_session_get_rtt(node_session->session, &rtt);

    napi_value rtt_mean_value = NULL;
    napi_value rtt_variance_value = NULL;

    napi_call(napi_create_bigint_uint64(env, rtt.mean, &rtt_mean_value));
    napi_call(napi_create_bigint_uint64(
        env, rtt.variance, &rtt_variance_value
    ));

    napi_value value = NULL;
    napi_call(napi_create_object(env, &value));
    napi_call(napi_set_named_property(
        env, value, "mean", rtt_mean_value
    ));
    napi_call(napi_set_named_property(
        env, value, "variance", rtt_variance_value
    ));

    // return: RTT
    return value;
}


napi_value pomelo_node_session_get_channels(
    napi_env env, napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_session_t * node_session = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_session, (void **) &node_session
    ));

    // Get the native session
    pomelo_session_t * session = node_session->session;
    if (!session) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // If channels is already created, return it
    if (node_session->channels) {
        napi_value channels = NULL;
        napi_call(napi_get_reference_value(
            env, node_session->channels, &channels
        ));
        return channels;
    }

    // Create array of channels
    pomelo_socket_t * socket = pomelo_session_get_socket(session);
    size_t nchannels = pomelo_socket_get_nchannels(socket);
    napi_value channels = NULL;
    napi_call(napi_create_array_with_length(env, nchannels, &channels));
    for (size_t i = 0; i < nchannels; i++) {
        pomelo_channel_t * channel = pomelo_session_get_channel(session, i);
        napi_value js_channel = pomelo_node_channel_new(env, channel);
        if (!js_channel) {
            // There's an error here
            napi_throw_msg(POMELO_NODE_ERROR_GET_CHANNELS);
            return NULL;
        }
        napi_call(napi_set_element(env, channels, (uint32_t) i, js_channel));
    }

    // Create reference to retain array
    napi_call(napi_create_reference(env, channels, 1, &node_session->channels));
    return channels;
}


/* -------------------------------------------------------------------------- */
/*                            Implemented APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_session_on_cleanup(pomelo_session_t * session) {
    pomelo_node_session_t * node_session = pomelo_session_get_extra(session);
    if (!node_session) return; // No associated session

    pomelo_node_context_release_session(node_session->context, node_session);
}
