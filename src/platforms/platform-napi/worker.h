#ifndef POMELO_PLATFORM_NAPI_WORKER_H
#define POMELO_PLATFORM_NAPI_WORKER_H
#include "node_api.h"
#include "platform-napi.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief The async work info structure
typedef struct pomelo_platform_task_worker_s
    pomelo_platform_task_worker_t;


struct pomelo_platform_task_worker_s {
    /// @brief The platform
    pomelo_platform_napi_t * platform;

    /// @brief The entry function
    pomelo_platform_task_entry entry;

    /// @brief The complete function
    pomelo_platform_task_complete complete;
    
    /// @brief The data
    void * data;

    /// @brief The async work
    napi_async_work async_work;

    /// @brief The canceled flag
    bool canceled;
};


/// @brief Submit a task to the worker thread
pomelo_platform_task_t * pomelo_platform_napi_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
);


/// @brief Cancel a worker task
void pomelo_platform_napi_cancel_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_t * task
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_PLATFORM_NAPI_WORKER_H
