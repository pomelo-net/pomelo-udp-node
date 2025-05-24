#include <assert.h>
#include "threadsafe.h"


/// @brief The threadsafe function max queue size
#define THREADSAFE_FUNCTION_MAX_QUEUE_SIZE 0xFFFF


/// @brief The threadsafe function entry
static void threadsafe_function_entry(
    napi_env env,
    pomelo_platform_napi_t * platform,
    pomelo_platform_task_threadsafe_t * task_threadsafe
) {
    assert(platform != NULL);
    assert(task_threadsafe != NULL);

    // Call the entry function
    task_threadsafe->entry(task_threadsafe->data);

    // Release the task threadsafe
    pomelo_pool_release(platform->task_threadsafe_pool, task_threadsafe);
}


pomelo_threadsafe_executor_t * pomelo_platform_napi_acquire_threadsafe_executor(
    pomelo_platform_t * platform
) {
    assert(platform != NULL);
    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;

    // Acquire the threadsafe executor
    pomelo_threadsafe_executor_t * executor =
        pomelo_pool_acquire(impl->threadsafe_executor_pool, NULL);
    if (executor == NULL) return NULL;

    // Create the threadsafe function
    napi_threadsafe_function threadsafe_function = NULL;
    napi_status status = napi_create_threadsafe_function(
        impl->env,
        NULL,
        NULL,
        NULL,
        THREADSAFE_FUNCTION_MAX_QUEUE_SIZE,
        1,
        NULL,
        NULL,
        impl,
        (napi_threadsafe_function_call_js) threadsafe_function_entry,
        &threadsafe_function
    );
    if (status != napi_ok) {
        pomelo_pool_release(impl->threadsafe_executor_pool, executor);
        return NULL; // Failed to create the threadsafe function
    }

    // Set the threadsafe function
    executor->platform = impl;
    executor->threadsafe_function = threadsafe_function;

    return executor;
}


/// @brief Release the threadsafe executor
static void release_threadsafe_executor(
    pomelo_threadsafe_executor_t * executor
) {
    assert(executor != NULL);
    napi_unref_threadsafe_function(
        executor->platform->env,
        executor->threadsafe_function
    );

    // Release the threadsafe executor
    pomelo_pool_release(executor->platform->threadsafe_executor_pool, executor);
}


void pomelo_platform_napi_release_threadsafe_executor(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor
) {
    // Submit the task to the threadsafe executor
    pomelo_platform_napi_threadsafe_executor_submit(
        platform,
        executor,
        (pomelo_platform_task_entry) release_threadsafe_executor,
        executor
    );
}


pomelo_platform_task_t * pomelo_platform_napi_threadsafe_executor_submit(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(platform != NULL);
    assert(executor != NULL);
    assert(entry != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    pomelo_platform_task_threadsafe_t * task_threadsafe =
        pomelo_pool_acquire(impl->task_threadsafe_pool, NULL);
    if (task_threadsafe == NULL) return NULL;

    task_threadsafe->platform = impl;
    task_threadsafe->entry = entry;
    task_threadsafe->data = data;

    napi_status status = napi_call_threadsafe_function(
        (napi_threadsafe_function) executor,
        task_threadsafe,
        napi_tsfn_nonblocking
    );
    if (status != napi_ok) {
        pomelo_pool_release(impl->task_threadsafe_pool, task_threadsafe);
        return NULL; // Failed to push the task to the threadsafe function
    }

    return (pomelo_platform_task_t *) task_threadsafe;
}
