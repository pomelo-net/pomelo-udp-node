#include <assert.h>
#include "channel.h"
#include "utils.h"
#include "context.h"
#include "error.h"
#include "message.h"


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_channel_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
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


napi_value pomelo_node_channel_new(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_channel_t * channel
) {
    pomelo_list_t * pool = context->pool_channel;
    napi_value js_channel = NULL;
    pomelo_node_channel_t * node_channel = NULL;

    if (pool->size == 0) {
        napi_value clazz = NULL;
        napi_call(napi_get_reference_value(
            env, context->class_channel, &clazz
        ));
        napi_call(napi_new_instance(env, clazz, 0, NULL, &js_channel));
        napi_call(napi_unwrap(env, js_channel, (void **) &node_channel));

        // Ref the reference
        napi_call(napi_reference_ref(env, node_channel->thiz, NULL));
    } else {
        pomelo_list_pop_back(pool, &node_channel);
        node_channel->pool_node = NULL;
        napi_call(napi_get_reference_value(
            env, node_channel->thiz, &js_channel
        ));
        // Keep the reference
    }

    // Attach the native channel
    node_channel->channel = channel;
    pomelo_channel_set_extra(channel, node_channel);

    return js_channel;
}


void pomelo_node_channel_delete(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_node_channel_t * node_channel
) {
    pomelo_list_t * pool = context->pool_channel;
    if (!node_channel->channel) return;

    pomelo_channel_set_extra(node_channel->channel, NULL);
    node_channel->channel = NULL;

    if (context->pool_channel->size >= context->pool_channel_max) {
        // Just unref channel
        napi_callv(napi_reference_unref(env, node_channel->thiz, NULL));
        return;
    }

    // Keep the reference
    node_channel->pool_node = pomelo_list_push_back(pool, node_channel);
    if (!node_channel->pool_node) {
        napi_callv(napi_reference_unref(env, node_channel->thiz, NULL));
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
        pomelo_node_context_create_node_channel(context);
    if (!node_channel) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }
    
    // Wrap the channel
    napi_status status = napi_wrap(
        env,
        thiz,
        node_channel,
        (napi_finalize) pomelo_node_channel_finalize,
        context,
        &node_channel->thiz
    );

    if (status != napi_ok) {
        pomelo_node_channel_finalize(env, node_channel, context);
        return NULL;
    }

    // This will create weak ref of channel
    return thiz;
}


void pomelo_node_channel_finalize(
    napi_env env,
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
) {
    // Release the reference
    napi_delete_reference(env, node_channel->thiz);
    node_channel->thiz = NULL;

    // Release the node message
    pomelo_node_context_destroy_node_channel(context, node_channel);
}


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
    // After sending the message, the native message associated with JS message
    // will be disassociated. This will make sure it will not be unreferenced
    // when the JS message is finalized.
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

    // Delivery the message
    int ret = pomelo_channel_send(node_channel->channel, node_message->message);
    if (ret == 0) {
        // Detach native message
        node_message->message = NULL;

        // Release JS message
        pomelo_node_message_delete(env, context, js_message);
    }

    // return: boolean
    napi_value result = NULL;
    napi_call(napi_get_boolean(env, ret == 0, &result));
    return result;
}
