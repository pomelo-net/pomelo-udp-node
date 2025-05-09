#ifndef POMELO_PLATFORM_NAPI_THREADSAFE_H
#define POMELO_PLATFORM_NAPI_THREADSAFE_H
#include "node_api.h"
#include "platform-napi.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The threadsafe info structure
typedef struct pomelo_platform_task_threadsafe_s
    pomelo_platform_task_threadsafe_t;


struct pomelo_platform_task_threadsafe_s {
    /// @brief The platform
    pomelo_platform_napi_t * platform;
    
    /// @brief The entry
    pomelo_platform_task_entry entry;

    /// @brief The data
    void * data;
};


struct pomelo_threadsafe_executor_s {
    /// @brief The platform
    pomelo_platform_napi_t * platform;

    /// @brief The threadsafe function
    napi_threadsafe_function threadsafe_function;
};


/// @brief Acquire the threadsafe executor
pomelo_threadsafe_executor_t * pomelo_platform_napi_acquire_threadsafe_executor(
    pomelo_platform_interface_t * i
);


/// @brief Release the threadsafe executor
void pomelo_platform_napi_release_threadsafe_executor(
    pomelo_platform_interface_t * i,
    pomelo_threadsafe_executor_t * executor
);


/// @brief Submit a task to the threadsafe executor
pomelo_platform_task_t * pomelo_platform_napi_threadsafe_executor_submit(
    pomelo_platform_interface_t * i,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
);


#ifdef __cplusplus
}
#endif
#endif
