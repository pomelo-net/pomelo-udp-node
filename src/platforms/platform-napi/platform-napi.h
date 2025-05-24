#ifndef POMELO_NODE_PLATFORM_NAPI_H
#define POMELO_NODE_PLATFORM_NAPI_H
#include "node_api.h"
#include "platform.h"
#include "base/extra.h"
#include "utils/pool.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bun will use platform-napi as the default platform.
 * This will call to JS functions to do the actual work.
 */

/// @brief The platform NAPI structure
typedef struct pomelo_platform_napi_s pomelo_platform_napi_t;


struct pomelo_platform_napi_s {
    /// @brief The platform interface
    pomelo_platform_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The UDP info pool
    pomelo_pool_t * udp_info_pool;

    /// @brief The timer info pool
    pomelo_pool_t * timer_info_pool;

    /// @brief The async work info pool 
    pomelo_pool_t * task_worker_pool;

    /// @brief The threadsafe info pool
    pomelo_pool_t * task_threadsafe_pool;

    /// @brief The threadsafe executor pool
    pomelo_pool_t * threadsafe_executor_pool;

    /// @brief The environment
    napi_env env;

    /// @brief The hrtime reference
    napi_ref hrtime;

    /// @brief The now reference
    napi_ref now;

    /// @brief The udp create reference
    napi_ref udp_create;

    /// @brief The udp bind reference
    napi_ref udp_bind;

    /// @brief The udp connect reference
    napi_ref udp_connect;

    /// @brief The udp stop reference
    napi_ref udp_stop;

    /// @brief The udp send reference
    napi_ref udp_send;

    /// @brief The udp recv callback reference
    napi_ref udp_recv_callback;

    /// @brief The udp send callback reference
    napi_ref udp_send_callback;

    /// @brief The timer create reference
    napi_ref timer_create;

    /// @brief The timer start reference
    napi_ref timer_start;

    /// @brief The timer stop reference
    napi_ref timer_stop;

    /// @brief The timer callback reference
    napi_ref timer_callback;

    /// @brief The statistic reference
    napi_ref statistic;
};


/// @brief Create the platform NAPI
pomelo_platform_t * pomelo_platform_napi_create(
    pomelo_allocator_t * allocator,
    napi_env env,
    napi_value exports
);


/// @brief Destroy the platform NAPI
void pomelo_platform_napi_destroy(pomelo_platform_t * i);


/// @brief Startup the platform NAPI
void pomelo_platform_napi_startup(pomelo_platform_t * i);


/// @brief Shutdown the platform NAPI
void pomelo_platform_napi_shutdown(
    pomelo_platform_t * i,
    pomelo_platform_shutdown_callback callback
);


/// @brief Get the statistic
napi_value pomelo_platform_napi_statistic(
    pomelo_platform_t * i,
    napi_env env
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_NODE_PLATFORM_NAPI_H
