#include <assert.h>
#include <string.h>
#include "utils.h"
#include "error.h"

/* -------------------------------------------------------------------------- */
/*                            Utiltities functions                            */
/* -------------------------------------------------------------------------- */


static napi_status pomelo_node_set_function_property_scoped(
    napi_env env,
    napi_value value,
    const char * name,
    napi_callback callback
) {
    napi_value fn;
    napi_status status;
    napi_valuetype type;
    napi_handle_scope scope;

    status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) {
        return status;
    }

    // Check if the value is object
    status = napi_typeof(env, value, &type);
    if (status != napi_ok) {
        return status;
    }

    if (type != napi_object) {
        napi_throw_error(
            env,
            NULL,
            "Invalid argument: 'value' is not an object"
        );
        return napi_invalid_arg;
    }

    // Create JS function
    status = napi_create_function(
        env,
        name,
        NAPI_AUTO_LENGTH,
        callback,
        NULL,
        &fn
    );
    if (status != napi_ok) {
        return status;
    }

    // Set the property
    status = napi_set_named_property(env, value, name, fn);
    if (status != napi_ok) {
        return status;
    }

    return napi_ok;
}


napi_status pomelo_node_set_function_property(
    napi_env env,
    napi_value value,
    const char * name,
    napi_callback callback
) {
    napi_handle_scope scope;
    napi_status ret;

    ret = napi_open_handle_scope(env, &scope);
    if (ret != napi_ok) return ret;

    ret = pomelo_node_set_function_property_scoped(env, value, name, callback);
    
    napi_close_handle_scope(env, scope);
    return ret;
}


napi_status pomelo_node_validate_native(
    napi_env env,
    napi_value thiz,
    napi_ref clazz_ref,
    void ** native
) {
    bool instance_of = false;
    napi_value clazz = NULL;
    napi_status status = napi_get_reference_value(env, clazz_ref, &clazz);
    if (status != napi_ok) return status;

    status = napi_instanceof(env, thiz, clazz, &instance_of);
    if (status != napi_ok) return status;
    if (!instance_of) return napi_invalid_arg;

    return napi_unwrap(env, thiz, native);
}


int pomelo_node_preprocess_function_callback(
    napi_env env,
    napi_callback_info info,
    napi_value * thiz,
    void * p_native,
    size_t * argc,
    napi_value * argv,
    napi_ref clazz_ref,
    void * p_data
) {
    assert(thiz != NULL);
    assert(p_native != NULL);

    // Get the callback info
    napi_status ret = napi_get_cb_info(env, info, argc, argv, thiz, p_data);
    if (ret != napi_ok) return -1;

    napi_value clazz;
    ret = napi_get_reference_value(env, clazz_ref, &clazz);
    if (ret != napi_ok) return -1;

    bool instance_of;
    ret = napi_instanceof(env, *thiz, clazz, &instance_of);
    if (ret != napi_ok) return -1;

    if (!instance_of) {
        napi_throw_type_error(env, NULL, POMELO_NODE_ERROR_INVALID_INSTANCE);
        return -1;
    }

    // Get the native pointer
    ret = napi_unwrap(env, *thiz, p_native);
    if (ret != napi_ok) return -1;

    if (!(*((void **) p_native))) {
        napi_throw_error(env, NULL, POMELO_NODE_ERROR_NATIVE_NULL);
        return -1;
    }

    return 0;
}


int pomelo_node_parse_int64_value(
    napi_env env,
    napi_value value,
    int64_t * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        double field_number = 0.0;
        status = napi_get_value_double(env, value, &field_number);
        if (status != napi_ok) return -1;

        *number = (int64_t) field_number;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        status = napi_get_value_bigint_int64(env, value, number, &lossless);
        if (status != napi_ok) return -1;
    } else {
        return -1;
    }

    return 0;
}


int pomelo_node_parse_uint64_value(
    napi_env env,
    napi_value value,
    uint64_t * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        double field_number = 0.0;
        status = napi_get_value_double(env, value, &field_number);
        if (status != napi_ok) return -1;

        *number = (uint64_t) field_number;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        status = napi_get_value_bigint_uint64(env, value, number, &lossless);
        if (status != napi_ok) return -1;
    } else {
        return -1;
    }

    return 0;
}


int pomelo_node_parse_int32_value(
    napi_env env,
    napi_value value,
    int32_t * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        double num = 0.0;
        status = napi_get_value_double(env, value, &num);
        if (status != napi_ok) return -1;

        *number = (int32_t) num;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        int64_t num = 0;
        status = napi_get_value_bigint_int64(env, value, &num, &lossless);
        if (status != napi_ok) return -1;

        *number = (int32_t) num;
    } else {
        return -1;
    }

    return 0;
}


int pomelo_node_parse_uint32_value(
    napi_env env,
    napi_value value,
    uint32_t * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        double num = 0.0;
        status = napi_get_value_double(env, value, &num);
        if (status != napi_ok) return -1;

        *number = (uint32_t) num;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        uint64_t num = 0;
        status = napi_get_value_bigint_uint64(env, value, &num, &lossless);
        if (status != napi_ok) return -1;

        *number = (uint32_t) num;
    } else {
        return -1;
    }

    return 0;
}


int pomelo_node_parse_size_value(
    napi_env env,
    napi_value value,
    size_t * size
) {
    uint32_t u32 = 0;
    int ret = pomelo_node_parse_uint32_value(env, value, &u32);
    if (ret < 0) return ret;
    *size = (size_t) u32;
    return 0;
}


int pomelo_node_parse_float64_value(
    napi_env env,
    napi_value value,
    double * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        status = napi_get_value_double(env, value, number);
        if (status != napi_ok) return -1;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        uint64_t num = 0;
        status = napi_get_value_bigint_uint64(env, value, &num, &lossless);
        if (status != napi_ok) return -1;

        *number = (double) num;
    } else {
        return -1;
    }

    return 0;
}


int pomelo_node_parse_float32_value(
    napi_env env,
    napi_value value,
    float * number
) {
    assert(number != NULL);

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return -1;

    if (type == napi_number) {
        // Number
        double num = 0;
        status = napi_get_value_double(env, value, &num);
        if (status != napi_ok) return -1;
        *number = (float) num;
    } else if (type == napi_bigint) {
        // BigInt
        bool lossless = true;
        uint64_t num = 0;
        status = napi_get_value_bigint_uint64(env, value, &num, &lossless);
        if (status != napi_ok) return -1;
        *number = (float) num;
    } else {
        return -1;
    }

    return 0;
}



int pomelo_node_get_uint64_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    uint64_t * number
) {
    assert(number != NULL);

    napi_value field;
    napi_status ret = napi_get_named_property(env, object, utf8name, &field);
    if (ret != napi_ok) return -1;

    return pomelo_node_parse_uint64_value(env, field, number);
}


int pomelo_node_get_int32_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    int32_t * number
) {
    assert(number != NULL);

    napi_value field;
    napi_status ret = napi_get_named_property(env, object, utf8name, &field);
    if (ret != napi_ok) return -1;

    return pomelo_node_parse_int32_value(env, field, number);
}


int pomelo_node_get_int64_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    int64_t * number
) {
    assert(number != NULL);

    napi_value field;
    napi_status ret = napi_get_named_property(env, object, utf8name, &field);
    if (ret != napi_ok) return -1;

    return pomelo_node_parse_int64_value(env, field, number);
}


napi_value pomelo_node_error_create(napi_env env, const char * message) {
    napi_value err_msg = NULL;
    napi_call(napi_create_string_utf8(
        env, message, NAPI_AUTO_LENGTH, &err_msg
    ));
    napi_value error = NULL;
    napi_call(napi_create_error(env, NULL, err_msg, &error));
    return error;
}


/* -------------------------------------------------------------------------- */
/*                            Promise utilities                               */
/* -------------------------------------------------------------------------- */

void pomelo_node_promise_cancel_deferred(napi_env env, napi_deferred deferred) {
    napi_value error =
        pomelo_node_error_create(env, POMELO_NODE_ERROR_CANCELED);
    if (!error) return;
    napi_callv(napi_reject_deferred(env, deferred, error));
}


napi_value pomelo_node_promise_reject_error(
    napi_env env,
    const char * message
) {
    napi_value result = NULL;
    napi_deferred deferred = NULL;
    napi_call(napi_create_promise(env, &deferred, &result));
    napi_value error = pomelo_node_error_create(env, message);
    if (!error) return NULL;
    napi_call(napi_reject_deferred(env, deferred, error));
    return result;
}


napi_value pomelo_node_promise_resolve_boolean(napi_env env, bool value) {
    napi_value result = NULL;
    napi_deferred deferred = NULL;
    napi_call(napi_create_promise(env, &deferred, &result));
    napi_value bool_value = NULL;
    napi_call(napi_get_boolean(env, value, &bool_value));
    napi_call(napi_resolve_deferred(env, deferred, bool_value));
    return result;
}


napi_value pomelo_node_promise_resolve_undefined(napi_env env) {
    napi_value result = NULL;
    napi_deferred deferred = NULL;
    napi_call(napi_create_promise(env, &deferred, &result));
    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    napi_call(napi_resolve_deferred(env, deferred, undefined));
    return result;
}


/* -------------------------------------------------------------------------- */
/*                             Object utilities                               */
/* -------------------------------------------------------------------------- */

static void pomelo_node_delete_wrapped_reference_scoped(
    napi_env env, napi_ref ref
) {
    napi_value value = NULL;
    napi_callv(napi_get_reference_value(env, ref, &value));
    napi_callv(napi_remove_wrap(env, value, NULL));
}


void pomelo_node_delete_wrapped_reference(napi_env env, napi_ref ref) {
    napi_handle_scope scope = NULL;
    napi_callv(napi_open_handle_scope(env, &scope));
    pomelo_node_delete_wrapped_reference_scoped(env, ref);
    napi_callv(napi_close_handle_scope(env, scope));
}
