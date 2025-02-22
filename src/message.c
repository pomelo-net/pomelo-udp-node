#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "module.h"
#include "message.h"
#include "error.h"
#include "utils.h"
#include "context.h"


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_message_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    napi_property_descriptor message_descriptors[] = {
        napi_method("write", pomelo_node_message_write, context),
        napi_method("writeUint8", pomelo_node_message_write_uint8, context),
        napi_method("writeUint16", pomelo_node_message_write_uint16, context),
        napi_method("writeUint32", pomelo_node_message_write_uint32, context),
        napi_method("writeUint64", pomelo_node_message_write_uint64, context),
        napi_method("writeInt8", pomelo_node_message_write_int8, context),
        napi_method("writeInt16", pomelo_node_message_write_int16, context),
        napi_method("writeInt32", pomelo_node_message_write_int32, context),
        napi_method("writeInt64", pomelo_node_message_write_int64, context),
        napi_method("writeFloat32", pomelo_node_message_write_float32, context),
        napi_method("writeFloat64", pomelo_node_message_write_float64, context),
        napi_method("read", pomelo_node_message_read, context),
        napi_method("readUint8", pomelo_node_message_read_uint8, context),
        napi_method("readUint16", pomelo_node_message_read_uint16, context),
        napi_method("readUint32", pomelo_node_message_read_uint32, context),
        napi_method("readUint64", pomelo_node_message_read_uint64, context),
        napi_method("readInt8", pomelo_node_message_read_int8, context),
        napi_method("readInt16", pomelo_node_message_read_int16, context),
        napi_method("readInt32", pomelo_node_message_read_int32, context),
        napi_method("readInt64", pomelo_node_message_read_int64, context),
        napi_method("readFloat32", pomelo_node_message_read_float32, context),
        napi_method("readFloat64", pomelo_node_message_read_float64, context),
        napi_method("reset", pomelo_node_message_reset, context),
        napi_method("size", pomelo_node_message_size, context),
        napi_static_method("acquire", pomelo_node_message_acquire, context),
        napi_static_method("release", pomelo_node_message_release, context),
    };

    // Build the class
    napi_value clazz = NULL;
    napi_calls(napi_define_class(
        env,
        "Message",
        NAPI_AUTO_LENGTH,
        pomelo_node_message_constructor,
        context,
        sizeof(message_descriptors) / sizeof(napi_property_descriptor),
        message_descriptors,
        &clazz
    ));

    napi_calls(napi_create_reference(env, clazz, 1, &context->class_message));
    napi_calls(napi_set_named_property(env, ns, "Message", clazz));

    return napi_ok;
}


napi_value pomelo_node_message_new(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_message_t * message
) {
    // Try to get from pool
    napi_value value = pomelo_node_message_acquire_ex(env, context);
    if (!value) return NULL;

    // Attach native message
    pomelo_node_message_t * node_message = NULL;
    napi_call(napi_unwrap(env, value, (void **) &node_message));
    if (message) {
        node_message->message = message;
        pomelo_message_set_extra(message, node_message);
        pomelo_message_ref(message);
    }
    return value;
}


void pomelo_node_message_delete(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value js_message
) {
    pomelo_node_message_release_ex(env, context, js_message);
}


/*----------------------------------------------------------------------------*/
/*                                Private APIs                                */
/*----------------------------------------------------------------------------*/

napi_value pomelo_node_message_constructor(
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

    // Create new node message
    pomelo_node_message_t * node_message =
        pomelo_node_context_create_node_message(context);
    if (!node_message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }
    
    // Wrap the message
    napi_status status = napi_wrap(
        env,
        thiz,
        node_message,
        (napi_finalize) pomelo_node_message_finalize,
        context,
        &node_message->thiz
    );

    if (status != napi_ok) {
        pomelo_node_message_finalize(env, node_message, context);
        return NULL;
    }

    // This will create weak ref of message
    return thiz;
}


void pomelo_node_message_finalize(
    napi_env env,
    pomelo_node_message_t * node_message,
    pomelo_node_context_t * context
) {
    // Release native message
    if (node_message->message) {
        pomelo_message_unref(node_message->message);
        node_message->message = NULL;
    }

    // Release the reference
    napi_delete_reference(env, node_message->thiz);
    node_message->thiz = NULL;

    // Release the node message
    pomelo_node_context_destroy_node_message(context, node_message);
}


napi_value pomelo_node_message_reset(napi_env env, napi_callback_info info) {
    // By reseting the message, the native message will be released to context.
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    // Detach the native message from JS message
    if (node_message->message) {
        pomelo_message_unref(node_message->message);
        node_message->message = NULL;
    }

    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    return undefined;
}


napi_value pomelo_node_message_size(napi_env env, napi_callback_info info) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    size_t size = message ? pomelo_message_size(message) : 0;

    napi_value ret_val;
    napi_call(napi_create_uint32(env, (uint32_t) size, &ret_val));
    return ret_val;
}


#define POMELO_NODE_MESSAGE_READ_ARGC 1
napi_value pomelo_node_message_read(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_MESSAGE_READ_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_READ_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    if (argc < POMELO_NODE_MESSAGE_READ_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    size_t size = 0;
    if (pomelo_node_parse_size_value(env, argv[0], &size) < 0) {
        napi_throw_arg("length");
        return NULL;
    }

    if (size == 0) {
        // Create empty typed array and return it
        napi_value result;
        napi_call(napi_create_typedarray(
            env, napi_uint8_array, 0, NULL, 0, &result
        ));
        return result;
    }

    // Prepare array buffer
    uint8_t * buffer = NULL;
    napi_value arrbuf = NULL;
    napi_call(napi_create_arraybuffer(env, size, (void **) &buffer, &arrbuf));

    // Read data from message
    if (pomelo_message_read_buffer(message, size, buffer) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }
    
    // Create typed array
    napi_value result;
    napi_call(napi_create_typedarray(
        env, napi_uint8_array, size, arrbuf, /* offset = */ 0, &result
    ));
    return result;
}


napi_value pomelo_node_message_read_uint8(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint8_t value = 0;
    if (pomelo_message_read_uint8(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_uint32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_uint16(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint16_t value = 0;
    if (pomelo_message_read_uint16(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_uint32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_uint32(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint32_t value = 0;
    if (pomelo_message_read_uint32(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_uint32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_uint64(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint64_t value = 0;
    if (pomelo_message_read_uint64(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_bigint_uint64(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_int8(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int8_t value = 0;
    if (pomelo_message_read_int8(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_int32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_int16(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int16_t value = 0;
    if (pomelo_message_read_int16(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_int32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_int32(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int32_t value = 0;
    if (pomelo_message_read_int32(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_int32(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_int64(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int64_t value = 0;
    if (pomelo_message_read_int64(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_bigint_int64(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_float32(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    float value = 0;
    if (pomelo_message_read_float32(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_double(env, value, &ret_val));
    return ret_val;
}


napi_value pomelo_node_message_read_float64(
    napi_env env,
    napi_callback_info info
) {
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    pomelo_message_t * message = node_message->message;
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    double value = 0;
    if (pomelo_message_read_float64(message, &value) < 0) {
        // Failed to read
        napi_throw_msg(POMELO_NODE_ERROR_READ_MESSAGE);
        return NULL;
    }

    napi_value ret_val;
    napi_call(napi_create_double(env, value, &ret_val));
    return ret_val;
}


pomelo_message_t * pomelo_node_message_prepare_write(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
) {
    pomelo_message_t * message = node_message->message;
    if (message) return message;
    message =
        pomelo_message_new((pomelo_context_t *) context->context);
    node_message->message = message;
    return message;
}


#define POMELO_NODE_MESSAGE_WRITE_ARGC 1
napi_value pomelo_node_message_write(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    napi_typedarray_type array_type;
    size_t length = 0;
    uint8_t * buffer = NULL;
    napi_status status = napi_get_typedarray_info(
        env, argv[0], &array_type, &length, (void **) &buffer, NULL, NULL
    );

    if (status != napi_ok || array_type != napi_uint8_array) {
        napi_throw_arg("value");
        return NULL;
    }

    if (length == 0) {
        // Data is empty
        napi_value result = NULL;
        napi_call(napi_get_undefined(env, &result));
        return result;
    }

    if (pomelo_message_write_buffer(message, length, buffer) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result = NULL;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_uint8(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;

    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint32_t value = 0;
    if (pomelo_node_parse_uint32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_uint8(message, (uint8_t) value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_uint16(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint32_t value = 0;
    if (pomelo_node_parse_uint32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_uint16(message, (uint16_t) value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_uint32(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint32_t value = 0;
    if (pomelo_node_parse_uint32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_uint32(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_uint64(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    uint64_t value = 0;
    if (pomelo_node_parse_uint64_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_uint64(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_int8(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int32_t value = 0;
    if (pomelo_node_parse_int32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_int8(message, (int8_t) value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_int16(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int32_t value = 0;
    if (pomelo_node_parse_int32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_int16(message, (int16_t) value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_int32(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int32_t value = 0;
    if (pomelo_node_parse_int32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_int32(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_int64(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    int64_t value = 0;
    if (pomelo_node_parse_int64_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_int64(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_float32(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    float value = 0;
    if (pomelo_node_parse_float32_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_float32(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_write_float64(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_MESSAGE_WRITE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_WRITE_ARGC] = { NULL };
    napi_value thiz = NULL;
    pomelo_node_context_t * context = NULL;
    pomelo_node_message_t * node_message = NULL;
    
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, &thiz, (void **) &context
    ));
    napi_call(pomelo_node_validate_native(
        env, thiz, context->class_message, (void **) &node_message
    ));

    if (node_message->pool_node) {
        napi_throw_msg(POMELO_NODE_ERROR_MESSAGE_RELEASED);
        return NULL; // Released message
    }

    if (argc < POMELO_NODE_MESSAGE_WRITE_ARGC) { 
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    pomelo_message_t * message =
        pomelo_node_message_prepare_write(context, node_message);
    if (!message) {
        napi_throw_msg(POMELO_NODE_ERROR_NATIVE_NULL);
        return NULL;
    }

    double value = 0;
    if (pomelo_node_parse_float64_value(env, argv[0], &value) < 0) {
        napi_throw_arg("value");
        return NULL;
    }

    if (pomelo_message_write_float64(message, value) < 0) {
        napi_throw_msg(POMELO_NODE_ERROR_WRITE_MESSAGE);
        return NULL;
    }

    napi_value result = NULL;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


napi_value pomelo_node_message_acquire(napi_env env, napi_callback_info info) {
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, NULL, (void **) &context
    ));
    return pomelo_node_message_acquire_ex(env, context);
}


napi_value pomelo_node_message_acquire_ex(
    napi_env env,
    pomelo_node_context_t * context
) {
    pomelo_list_t * pool = context->pool_message;
    napi_value js_message = NULL;

    if (pool->size == 0) {
        // Construct new one
        napi_value clazz = NULL;
        napi_call(napi_get_reference_value(
            env, context->class_message, &clazz
        ));
        napi_call(napi_new_instance(env, clazz, 0, NULL, &js_message));
    } else {
        // Acquire from list
        pomelo_node_message_t * node_message = NULL;
        pomelo_list_pop_front(pool, &node_message);
        node_message->pool_node = NULL;
        napi_call(napi_get_reference_value(
            env, node_message->thiz, &js_message
        ));
        napi_call(napi_reference_unref(env, node_message->thiz, NULL));
    }

    return js_message;
}


#define POMELO_NODE_MESSAGE_ACQUIRE_ARGC 1
napi_value pomelo_node_message_release(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_MESSAGE_ACQUIRE_ARGC;
    napi_value argv[POMELO_NODE_MESSAGE_ACQUIRE_ARGC] = { NULL };
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, NULL, (void **) &context
    ));

    if (argc < POMELO_NODE_MESSAGE_ACQUIRE_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    napi_value js_message = argv[0];
    napi_value clazz = NULL;
    napi_call(napi_get_reference_value(env, context->class_message, &clazz));

    bool instance_of = false;
    napi_call(napi_instanceof(env, js_message, clazz, &instance_of));
    if (!instance_of) {
        napi_throw_arg("message");
        return NULL;
    }
    pomelo_node_message_release_ex(env, context, js_message);

    napi_value result;
    napi_call(napi_get_undefined(env, &result));
    return result;
}


void pomelo_node_message_release_ex(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value js_message
) {
    // Unwrap node message and release the native message
    pomelo_node_message_t * node_message = NULL;
    napi_callv(napi_unwrap(env, js_message, (void **) &node_message));
    if (node_message->message) {
        pomelo_message_set_extra(node_message->message, NULL);
        pomelo_message_unref(node_message->message);
        node_message->message = NULL;
    }

    // Check capacity of messages pool
    pomelo_list_t * pool = context->pool_message;
    if (context->pool_message->size > context->pool_message_max) return;

    // Append message ref to pool
    node_message->pool_node = pomelo_list_push_back(pool, node_message);
    if (node_message->pool_node) {
        // Retain the message
        napi_callv(napi_reference_ref(env, node_message->thiz, NULL));
    }
}


void pomelo_message_on_released(pomelo_message_t * message) {
    (void) message;
    // Ingore
}
