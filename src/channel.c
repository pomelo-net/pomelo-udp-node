#include <assert.h>
#include <stdio.h>
#include "channel.h"
#include "utils.h"
#include "context.h"
#include "error.h"
#include "message.h"


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/


napi_status pomelo_node_init_channel_module(napi_env env, napi_value ns) {
    pomelo_node_context_t * context = NULL;
    napi_calls(napi_get_instance_data(env, (void **) &context));
    assert(context != NULL);

    napi_property_descriptor properties[] = {
        napi_property(
            "mode",
            pomelo_node_channel_get_mode,
            pomelo_node_channel_set_mode,
            context
        ),
        napi_method("send", pomelo_node_channel_send, context)
    };

    napi_value clazz = NULL;
    napi_calls(napi_define_class(
        env,
        "Channel",
        NAPI_AUTO_LENGTH,
        pomelo_node_channel_constructor,
        context,
        arrlen(properties),
        properties,
        &clazz
    ));

    napi_calls(napi_create_reference(env, clazz, 1, &context->class_channel));
    return napi_ok;
}


napi_value pomelo_node_channel_new(napi_env env, pomelo_channel_t * channel) {
    // Get the context
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_instance_data(env, (void **) &context));
    assert(context != NULL);

    // Get the class
    napi_value clazz = NULL;
    napi_call(napi_get_reference_value(env, context->class_channel, &clazz));

    // Create new instance
    napi_value js_channel = NULL;
    napi_call(napi_new_instance(env, clazz, 0, NULL, &js_channel));

    // Get the node channel
    pomelo_node_channel_t * node_channel = NULL;
    napi_call(napi_unwrap(env, js_channel, (void **) &node_channel));

    // Attach the native channel
    node_channel->channel = channel;
    pomelo_channel_set_extra(channel, node_channel);

    // Ref the reference to keep the channel alive
    napi_call(napi_reference_ref(env, node_channel->thiz, NULL));

    return js_channel;
}


int pomelo_node_channel_init(
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
) {
    assert(node_channel != NULL);
    node_channel->context = context;
    return 0;
}


void pomelo_node_channel_cleanup(pomelo_node_channel_t * node_channel) {
    assert(node_channel != NULL);

    // Unset the native channel
    if (node_channel->channel) {
        pomelo_channel_set_extra(node_channel->channel, NULL);
        node_channel->channel = NULL;
    }

    // Delete the reference
    if (node_channel->thiz) {
        napi_delete_reference(node_channel->context->env, node_channel->thiz);
        node_channel->thiz = NULL;
    }
}


/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/

napi_value pomelo_node_channel_constructor(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));

    // Check if this call is a constructor call
    napi_valuetype type = 0;
    napi_call(napi_typeof(env, thiz, &type));
    if (type != napi_object) {
        napi_throw_msg(POMELO_NODE_ERROR_CONSTRUCTOR_CALL);
        return NULL;
    }

    // Create new node channel
    pomelo_node_channel_t * node_channel =
        pomelo_node_context_acquire_channel(context);
    if (!node_channel) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }
    
    // Wrap the channel
    napi_status status = napi_wrap(
        env,
        thiz,
        node_channel,
        (napi_finalize) pomelo_node_channel_finalizer,
        context,
        &node_channel->thiz
    );
    if (status != napi_ok) {
        pomelo_node_context_release_channel(context, node_channel);
        assert(false);
        return NULL;
    }

    return thiz;
}


void pomelo_node_channel_finalizer(
    napi_env env,
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
) {
    (void) env;
    // The channels are always strong references, so that, this finalizer is
    // not called in normal cases.

    // Release the channel.
    pomelo_node_context_release_channel(context, node_channel);
}


/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/


#define POMELO_NODE_CHANNEL_SET_MODE_ARGC 1
napi_value pomelo_node_channel_set_mode(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_CHANNEL_SET_MODE_ARGC;
    napi_value argv[POMELO_NODE_CHANNEL_SET_MODE_ARGC];
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_channel_t * node_channel = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_channel, (void **) &node_channel
    ));

    if (argc < POMELO_NODE_CHANNEL_SET_MODE_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    if (!node_channel->channel) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int32_t mode = -1;
    if (
        pomelo_node_parse_int32_value(env, argv[0], &mode) < 0 ||
        mode < 0 || mode >= POMELO_CHANNEL_MODE_COUNT
    ) {
        napi_throw_arg("mode");
        return NULL;
    }

    // Set native
    int ret = pomelo_channel_set_mode(
        node_channel->channel,
        (pomelo_channel_mode) mode
    );
    if (ret < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_SET_CHANNEL_MODE);
        return NULL;
    }

    // return: undefined
    napi_value result = NULL;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_channel_get_mode(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_channel_t * node_channel = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_channel, (void **) &node_channel
    ));

    if (!node_channel->channel) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    // Get native channel mode
    pomelo_channel_mode mode = pomelo_channel_get_mode(node_channel->channel);

    napi_value result = NULL;
    napi_call(napi_create_int32(env, (int32_t) mode, &result));
    return result;
}


#define POMELO_NODE_CHANNEL_SEND_ARGC 1
napi_value pomelo_node_channel_send(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_CHANNEL_SEND_ARGC;
    napi_value argv[POMELO_NODE_CHANNEL_SEND_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_channel_t * node_channel = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_channel, (void **) &node_channel
    ));

    if (!node_channel->channel) {
        // The native channel has been disassociated
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    if (argc < POMELO_NODE_CHANNEL_SEND_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // Parse message
    napi_value js_message = argv[0];
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
    pomelo_channel_send(
        node_channel->channel,
        node_message->message,
        deferred
    );

    return result; // Promise<number>
}


/* -------------------------------------------------------------------------- */
/*                            Implemented APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_channel_on_cleanup(pomelo_channel_t * channel) {
    pomelo_node_channel_t * node_channel = pomelo_channel_get_extra(channel);
    if (!node_channel) return; // The channel may not be created

    pomelo_node_context_release_channel(node_channel->context, node_channel);
}
