#include <assert.h>
#include <string.h>
#include "error.h"
#include "platform.h"
#include "platform/uv/platform-uv.h"
#include "utils.h"


/**
 * NodeJS will use platform-uv as the default platform.
 * This will use libuv to do the actual work.
 */

/// @brief Destroy the platform
static void platform_uv_destroy(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    
    if (i->internal) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) i->internal);
        i->internal = NULL;
    }

    pomelo_allocator_free(i->allocator, i);
}


/// @brief Startup the platform
static void platform_uv_startup(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    pomelo_platform_startup(i->internal);
}


/// @brief Shutdown callback for the platform
static void platform_uv_on_shutdown(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = pomelo_platform_get_extra(platform);
    if (i->shutdown_callback) {
        i->shutdown_callback((pomelo_platform_t *) i);
    }
}


/// @brief Shutdown the platform
static void platform_uv_shutdown(
    pomelo_platform_interface_t * i,
    pomelo_platform_shutdown_callback callback
) {
    assert(i != NULL);
    i->shutdown_callback = callback;
    pomelo_platform_shutdown(i->internal, platform_uv_on_shutdown);
}


/// @brief Get the hrtime
static uint64_t platform_uv_hrtime(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    return pomelo_platform_hrtime(i->internal);
}


/// @brief Get current time
static uint64_t platform_uv_now(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    return pomelo_platform_now(i->internal);
}


/// @brief Acquire the threadsafe executor
static pomelo_threadsafe_executor_t * platform_uv_acquire_threadsafe_executor(
    pomelo_platform_interface_t * i
) {
    assert(i != NULL);
    return pomelo_platform_acquire_threadsafe_executor(i->internal);
}


/// @brief Release the threadsafe executor
static void platform_uv_release_threadsafe_executor(
    pomelo_platform_interface_t * i,
    pomelo_threadsafe_executor_t * executor
) {
    assert(i != NULL);
    pomelo_platform_release_threadsafe_executor(i->internal, executor);
}


/// @brief Submit a task to the threadsafe executor
static pomelo_platform_task_t * platform_uv_threadsafe_executor_submit(
    pomelo_platform_interface_t * i,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(i != NULL);
    return pomelo_threadsafe_executor_submit(
        i->internal,
        executor,
        entry,
        data
    );
}


/// @brief Submit a task to the worker thread
static pomelo_platform_task_t * platform_uv_submit_worker_task(
    pomelo_platform_interface_t * i,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
) {
    assert(i != NULL);
    return pomelo_platform_submit_worker_task(
        i->internal,
        entry,
        complete,
        data
    );
}


/// @brief Cancel a worker task
static void platform_uv_cancel_worker_task(
    pomelo_platform_interface_t * i,
    pomelo_platform_task_t * task
) {
    assert(i != NULL);
    pomelo_platform_cancel_worker_task(i->internal, task);
}


/// @brief Bind the UDP socket
static pomelo_platform_udp_t * platform_uv_udp_bind(
    pomelo_platform_interface_t * i,
    pomelo_address_t * address
) {
    assert(i != NULL);
    return pomelo_platform_udp_bind(i->internal, address);
}


/// @brief Connect the UDP socket
static pomelo_platform_udp_t * platform_uv_udp_connect(
    pomelo_platform_interface_t * i,
    pomelo_address_t * address
) {
    assert(i != NULL);
    return pomelo_platform_udp_connect(i->internal, address);
}


/// @brief Stop the UDP socket
static int platform_uv_udp_stop(
    pomelo_platform_interface_t * i,
    pomelo_platform_udp_t * socket
) {
    assert(i != NULL);
    return pomelo_platform_udp_stop(i->internal, socket);
}


/// @brief Send a packet to the UDP socket
static int platform_uv_udp_send(
    pomelo_platform_interface_t * i,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
) {
    assert(i != NULL);
    return pomelo_platform_udp_send(
        i->internal,
        socket,
        address,
        niovec,
        iovec,
        callback_data,
        send_callback
    );
}


/// @brief Start receiving packets from the UDP socket
static void platform_uv_udp_recv_start(
    pomelo_platform_interface_t * i,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
) {
    assert(i != NULL);
    pomelo_platform_udp_recv_start(
        i->internal,
        socket,
        context,
        alloc_callback,
        recv_callback
    );
}


/// @brief Start the timer
static int platform_uv_timer_start(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(i != NULL);
    return pomelo_platform_timer_start(
        i->internal,
        entry,
        timeout_ms,
        repeat_ms,
        data,
        handle
    );
}


/// @brief Stop the timer
static void platform_uv_timer_stop(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_handle_t * handle
) {
    assert(i != NULL);
    pomelo_platform_timer_stop(i->internal, handle);
}


/// @brief Get the platform statistic
static napi_value platform_uv_statistic(
    pomelo_platform_interface_t * i,
    napi_env env
) {
    pomelo_platform_t * platform = i->internal;
    pomelo_statistic_platform_uv_t statistic = { 0 };
    pomelo_platform_uv_statistic(platform, &statistic);

    napi_value result = NULL;
    napi_status status = napi_create_object(env, &result);
    if (status != napi_ok) {
        return NULL;
    }

    napi_value timers;
    napi_call(napi_create_bigint_uint64(env, statistic.timers, &timers));
    napi_call(napi_set_named_property(env, result, "timers", timers));

    napi_value worker_tasks;
    napi_call(napi_create_bigint_uint64(
        env, statistic.worker_tasks, &worker_tasks
    ));
    napi_call(napi_set_named_property(
        env, result, "worker_tasks", worker_tasks
    ));

    napi_value threadsafe_tasks;
    napi_call(napi_create_bigint_uint64(
        env, statistic.threadsafe_tasks, &threadsafe_tasks
    ));
    napi_call(napi_set_named_property(
        env, result, "threadsafe_tasks", threadsafe_tasks
    ));

    napi_value send_commands;
    napi_call(napi_create_bigint_uint64(
        env, statistic.send_commands, &send_commands
    ));
    napi_call(napi_set_named_property(
        env, result, "send_commands", send_commands
    ));

    napi_value sent_bytes;
    napi_call(napi_create_bigint_uint64(
        env, statistic.sent_bytes, &sent_bytes
    ));
    napi_call(napi_set_named_property(env, result, "sent_bytes", sent_bytes));

    napi_value recv_bytes;
    napi_call(napi_create_bigint_uint64(
        env, statistic.recv_bytes, &recv_bytes
    ));
    napi_call(napi_set_named_property(env, result, "recv_bytes", recv_bytes));

    return result;
}


/// @brief Create the platform UV interface
static pomelo_platform_interface_t * platform_uv_create(
    pomelo_allocator_t * allocator,
    napi_env env
) {
    assert(allocator != NULL);
    assert(env != NULL);

    pomelo_platform_interface_t * i = pomelo_allocator_malloc_t(
        allocator,
        pomelo_platform_interface_t
    );
    if (!i) return NULL;
    memset(i, 0, sizeof(pomelo_platform_interface_t));
    i->allocator = allocator;

    // Get UV loop
    uv_loop_t * uv_loop = NULL;
    napi_status status = napi_get_uv_event_loop(env, &uv_loop);
    if (status != napi_ok || !uv_loop) {
        platform_uv_destroy(i);
        return NULL;
    }

    // Create platform
    pomelo_platform_uv_options_t options = {
        .allocator = allocator,
        .uv_loop = uv_loop
    };

    pomelo_platform_t * platform = pomelo_platform_uv_create(&options);
    if (!platform) {
        platform_uv_destroy(i);
        return NULL;
    }

    // Set the extra data for platform
    pomelo_platform_set_extra(platform, i);

    // Setup the interface
    i->allocator = allocator;
    i->internal = platform;
    i->destroy = platform_uv_destroy;
    i->startup = platform_uv_startup;
    i->shutdown = platform_uv_shutdown;
    i->hrtime = platform_uv_hrtime;
    i->now = platform_uv_now;
    i->acquire_threadsafe_executor = platform_uv_acquire_threadsafe_executor;
    i->release_threadsafe_executor = platform_uv_release_threadsafe_executor;
    i->threadsafe_executor_submit = platform_uv_threadsafe_executor_submit;
    i->submit_worker_task = platform_uv_submit_worker_task;
    i->cancel_worker_task = platform_uv_cancel_worker_task;
    i->udp_bind = platform_uv_udp_bind;
    i->udp_connect = platform_uv_udp_connect;
    i->udp_stop = platform_uv_udp_stop;
    i->udp_send = platform_uv_udp_send;
    i->udp_recv_start = platform_uv_udp_recv_start;
    i->timer_start = platform_uv_timer_start;
    i->timer_stop = platform_uv_timer_stop;
    i->statistic = platform_uv_statistic;

    return i;
}


/// @brief Platform-uv init function
static napi_value platform_uv_init(napi_env env, napi_callback_info info) {
    (void) info;

    pomelo_allocator_t * allocator = pomelo_allocator_default();
    pomelo_platform_interface_t * i = platform_uv_create(allocator, env);
    if (!i) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        return NULL;
    }

    napi_value result = NULL;
    napi_status status = napi_create_external(env, i, NULL, NULL, &result);
    if (status != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        platform_uv_destroy(i);
        return NULL;
    }

    return result;
}


/// @brief Entry function for platform-uv module
static napi_value platform_uv_entry(napi_env env, napi_value exports) {
    (void) exports;
    // Create the init fun  ction
    napi_value fn_init = NULL;
    napi_status status = napi_create_function(
        env,
        "initPlatformUV",
        NAPI_AUTO_LENGTH,
        platform_uv_init,
        NULL,
        &fn_init
    );

    if (status != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_INIT_PLATFORM_UV);
        return NULL;
    }

    return fn_init;
}

/// @brief Platform-uv module
NAPI_MODULE(pomelo_node_platform_uv, platform_uv_entry);
