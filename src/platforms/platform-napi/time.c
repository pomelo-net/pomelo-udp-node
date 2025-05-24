#include <assert.h>
#include "platform-napi.h"
#include "time.h"


/// @brief Get the high resolution time
static uint64_t platform_hrtime(pomelo_platform_napi_t * platform) {
    napi_value hrtime = NULL;
    napi_status status =
        napi_get_reference_value(platform->env, platform->hrtime, &hrtime);
    if (status != napi_ok) return 0;

    napi_value null_value = NULL;
    status = napi_get_null(platform->env, &null_value);
    if (status != napi_ok) return 0;

    napi_value result = NULL;
    status =
        napi_call_function(platform->env, null_value, hrtime, 0, NULL, &result);
    if (status != napi_ok) return 0;

    uint64_t hrtime_value = 0;
    status = napi_get_value_bigint_uint64(
        platform->env, result, &hrtime_value, NULL
    );
    if (status != napi_ok) return 0;

    return hrtime_value;
}


uint64_t pomelo_platform_napi_hrtime(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return 0;

    uint64_t hrtime = platform_hrtime(impl);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return 0;

    return hrtime;
}


/// @brief Get the current time
static uint64_t platform_now(pomelo_platform_napi_t * platform) {
    napi_value now = NULL;
    napi_status status;

    status = napi_get_reference_value(platform->env, platform->now, &now);
    if (status != napi_ok) return 0;

    napi_value null_value = NULL;
    status = napi_get_null(platform->env, &null_value);
    if (status != napi_ok) return 0;

    napi_value result;
    status =
        napi_call_function(platform->env, null_value, now, 0, NULL, &result);
    if (status != napi_ok) return 0;

    uint64_t now_value = 0;
    status = napi_get_value_bigint_uint64(
        platform->env, result, &now_value, NULL
    );
    if (status != napi_ok) return 0;

    return now_value;
}


uint64_t pomelo_platform_napi_now(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return 0;

    uint64_t result = platform_now(impl);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return 0;

    return result;
}
