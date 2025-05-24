#include <assert.h>
#include "platform-napi.h"
#include "worker.h"


/// @brief Async work callback
static void async_work_execute(
    napi_env env,
    pomelo_platform_task_worker_t * task_worker
) {
    assert(task_worker != NULL);
    assert(task_worker->entry != NULL);
    task_worker->entry(task_worker->data);
}


/// @brief Async work complete
static void async_work_complete(
    napi_env env,
    napi_status status,
    pomelo_platform_task_worker_t * task_worker
) {
    assert(task_worker != NULL);

    if (task_worker->complete) {
        bool canceled = task_worker->canceled || (status == napi_cancelled);
        task_worker->complete(task_worker->data, canceled);
    }
    
    napi_delete_async_work(env, task_worker->async_work);
    pomelo_pool_release(task_worker->platform->task_worker_pool, task_worker);
}


pomelo_platform_task_t * pomelo_platform_napi_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
) {
    assert(platform != NULL);
    assert(entry != NULL);
    assert(complete != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    assert(impl != NULL);

    pomelo_platform_task_worker_t * task_worker =
        pomelo_pool_acquire(impl->task_worker_pool, NULL);
    if (task_worker == NULL) return NULL;
    task_worker->platform = impl;
    task_worker->entry = entry;
    task_worker->complete = complete;
    task_worker->data = data;

    napi_env env = impl->env;
    // Create async work
    napi_async_work async_work = NULL;

    napi_value null_value;
    napi_status status = napi_get_null(env, &null_value);
    if (status != napi_ok) {
        pomelo_pool_release(impl->task_worker_pool, task_worker);
        return NULL;
    }

    status = napi_create_async_work(
        env,
        NULL,
        null_value,
        (napi_async_execute_callback) async_work_execute,
        (napi_async_complete_callback) async_work_complete,
        task_worker,
        &async_work
    );
    if (status != napi_ok) {
        pomelo_pool_release(impl->task_worker_pool, task_worker);
        return NULL;
    }

    task_worker->async_work = async_work;

    // Queue the async work
    status = napi_queue_async_work(env, async_work);
    if (status != napi_ok) {
        pomelo_pool_release(impl->task_worker_pool, task_worker);
        return NULL; // Failed to queue the async work
    }

    return (pomelo_platform_task_t *) task_worker;
}


void pomelo_platform_napi_cancel_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_t * task
) {
    assert(platform != NULL);
    assert(task != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    assert(impl != NULL);
    
    pomelo_platform_task_worker_t * task_worker =
        (pomelo_platform_task_worker_t *) task;

    task_worker->canceled = true;
    napi_cancel_async_work(impl->env, task_worker->async_work);
}
