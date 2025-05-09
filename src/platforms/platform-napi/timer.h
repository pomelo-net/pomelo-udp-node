#ifndef POMELO_NODE_PLATFORM_NAPI_TIMER_H
#define POMELO_NODE_PLATFORM_NAPI_TIMER_H
#include "node_api.h"
#include "platform/platform.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief The timer info structure
typedef struct pomelo_platform_timer_info_s pomelo_platform_timer_info_t;


struct pomelo_platform_timer_info_s {
    /// @brief The platform
    pomelo_platform_napi_t * platform;

    /// @brief The entry
    pomelo_platform_timer_entry entry;

    /// @brief The data
    void * data;

    /// @brief The repeat ms
    uint64_t repeat_ms;

    /// @brief The timeout ms
    uint64_t timeout_ms;

    /// @brief The timer reference
    napi_ref timer_ref;

    /// @brief The timer handle
    pomelo_platform_timer_handle_t * handle;
};


/// @brief The timer callback
napi_value pomelo_platform_timer_callback(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalize and release the timer info
void pomelo_platform_timer_info_finalize(pomelo_platform_timer_info_t * info);


/// @brief Start the timer
int pomelo_platform_napi_timer_start(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
);


/// @brief Stop the timer
void pomelo_platform_napi_timer_stop(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_handle_t * handle
);


#ifdef __cplusplus
}
#endif
#endif
