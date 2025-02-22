#ifndef POMELO_NODE_UTILS_SRC_H
#define POMELO_NODE_UTILS_SRC_H
#include "node_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                            Utiltities functions                            */
/* -------------------------------------------------------------------------- */

#define arrlen(argv) (sizeof(argv) / sizeof(argv[0]))

/// The wrapper for napi call in napi_value returned function
#define napi_call(expression)                                                  \
    if ((expression) != napi_ok) { assert(0); return NULL; }


/// The wrapper for napi call in void function
#define napi_callv(expression)                                                 \
do {                                                                           \
    napi_status status = (expression);                                         \
    assert(status == napi_ok);                                                 \
    if (status != napi_ok) return;                                             \
} while (0)


#define napi_calls(expression)                                                 \
do {                                                                           \
    napi_status status = (expression);                                         \
    assert(status == napi_ok);                                                 \
    if (status != napi_ok) return status;                                      \
} while (0)


#define napi_method(name, func, context) {                                     \
    .utf8name = name,                                                          \
    .method = func,                                                            \
    .attributes = napi_default_method,                                         \
    .data = context,                                                           \
}


#define napi_static_method(name, func, context) {                              \
    .utf8name = name,                                                          \
    .method = func,                                                            \
    .attributes = napi_static,                                                 \
    .data = context,                                                           \
}


#define napi_property(name, get_fn, set_fn, context) {                         \
    .utf8name = name,                                                          \
    .getter = get_fn,                                                          \
    .setter = set_fn,                                                          \
    .attributes = napi_enumerable,                                             \
    .data = context,                                                           \
}

/// @brief Set a function as a property of specific object
napi_status pomelo_node_set_function_property(
    napi_env env,
    napi_value value,
    const char * name,
    napi_callback callback
);


/// @brief Pre-process some stubs of function callbacks
napi_status pomelo_node_validate_native(
    napi_env env,
    napi_value thiz,
    napi_ref clazz_ref,
    void ** native
);


/* -------------------------------------------------------------------------- */
/*                         Values parsing functions                           */
/* -------------------------------------------------------------------------- */


/// @brief Parse the int64 value
int pomelo_node_parse_int64_value(
    napi_env env,
    napi_value value,
    int64_t * number
);

/// @brief Parse int32 value
int pomelo_node_parse_int32_value(
    napi_env env,
    napi_value value,
    int32_t * number
);

/// @brief Parse uint64 value
int pomelo_node_parse_uint64_value(
    napi_env env,
    napi_value value,
    uint64_t * number
);

/// @brief Parse uint32 value
int pomelo_node_parse_uint32_value(
    napi_env env,
    napi_value value,
    uint32_t * number
);

/// @brief Parse size_t value
int pomelo_node_parse_size_value(
    napi_env env,
    napi_value value,
    size_t * size
);

/// @brief Parse float64 value
int pomelo_node_parse_float64_value(
    napi_env env,
    napi_value value,
    double * number
);

/// @brief Parse float32 value
int pomelo_node_parse_float32_value(
    napi_env env,
    napi_value value,
    float * number
);


/* -------------------------------------------------------------------------- */
/*                           Properties utiltities                            */
/* -------------------------------------------------------------------------- */


/// @brief Get the uint64 number property from an object
int pomelo_node_get_uint64_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    uint64_t * number
);

/// @brief Get the int32 number property from an object
int pomelo_node_get_int32_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    int32_t * number
);

/// @brief Get the int32 number property from an object
int pomelo_node_get_int64_property(
    napi_env env,
    napi_value object,
    const char * utf8name,
    int64_t * number
);

/* -------------------------------------------------------------------------- */
/*                             Error utilities                                */
/* -------------------------------------------------------------------------- */

/// @brief Create an error with message
napi_value pomelo_node_error_create(napi_env env, const char * message);


/* -------------------------------------------------------------------------- */
/*                            Promise utilities                               */
/* -------------------------------------------------------------------------- */

/// @brief Cancel deferred
void pomelo_node_promise_cancel_deferred(napi_env env, napi_deferred deferred);

/// @brief Reject with an error
napi_value pomelo_node_promise_reject_error(napi_env env, const char * message);

/// @brief Create resolved promise with a boolean value
napi_value pomelo_node_promise_resolve_boolean(napi_env env, bool value);

napi_value pomelo_node_promise_resolve_undefined(napi_env env);


/* -------------------------------------------------------------------------- */
/*                             Object utilities                               */
/* -------------------------------------------------------------------------- */

void pomelo_node_delete_wrapped_reference(napi_env env, napi_ref ref);

#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_UTILS_SRC_H
