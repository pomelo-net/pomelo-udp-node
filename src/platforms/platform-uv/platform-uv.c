#include <assert.h>
#include <string.h>
#include "platform-uv.h"
#include "error.h"
#include "utils.h"


/**
 * NodeJS will use platform-uv as the default platform.
 */

/// @brief Set the extra data
static void platform_uv_set_extra(pomelo_platform_t * platform, void * extra) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    impl->extra = extra;
}


/// @brief Get the extra data
static void * platform_uv_get_extra(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return impl->extra;
}


/// @brief Destroy the platform
static void platform_uv_destroy(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    
    if (impl->platform_uv) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) impl->platform_uv);
        impl->platform_uv = NULL;
    }

    // Free itself
    pomelo_allocator_free(impl->allocator, impl);
}


/// @brief Startup the platform
static void platform_uv_startup(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_platform_uv_startup(impl->platform_uv);
}


/// @brief Shutdown callback for the platform
static void platform_uv_on_shutdown(pomelo_platform_uv_t * internal) {
    // The argument here is the pointer to the UV platform
    assert(internal != NULL);
    pomelo_platform_uv_impl_t * impl = pomelo_platform_uv_get_extra(internal);
    assert(impl != NULL);

    if (impl->shutdown_callback) {
        impl->shutdown_callback((pomelo_platform_t *) impl);
    }
}


/// @brief Shutdown the platform
static void platform_uv_shutdown(
    pomelo_platform_t * platform,
    pomelo_platform_shutdown_callback callback
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;

    impl->shutdown_callback = callback;
    pomelo_platform_uv_shutdown(
        impl->platform_uv,
        (pomelo_platform_shutdown_callback) platform_uv_on_shutdown
    );
}


/// @brief Get the hrtime
static uint64_t platform_uv_hrtime(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_hrtime(impl->platform_uv);
}


/// @brief Get current time
static uint64_t platform_uv_now(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_now(impl->platform_uv);
}


/// @brief Acquire the threadsafe executor
static pomelo_threadsafe_executor_t * platform_uv_acquire_threadsafe_executor(
    pomelo_platform_t * platform
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_acquire_threadsafe_executor(impl->platform_uv);
}


/// @brief Release the threadsafe executor
static void platform_uv_release_threadsafe_executor(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_platform_uv_release_threadsafe_executor(impl->platform_uv, executor);
}


/// @brief Submit a task to the threadsafe executor
static pomelo_platform_task_t * platform_uv_threadsafe_executor_submit(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_threadsafe_executor_uv_submit(
        impl->platform_uv, executor, entry, data
    );
}


/// @brief Submit a task to the worker thread
static pomelo_platform_task_t * platform_uv_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_submit_worker_task(
        impl->platform_uv, entry, complete, data
    );
}


/// @brief Cancel a worker task
static void platform_uv_cancel_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_t * task
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_platform_uv_cancel_worker_task(impl->platform_uv, task);
}


/// @brief Bind the UDP socket
static pomelo_platform_udp_t * platform_uv_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_udp_bind(impl->platform_uv, address);
}


/// @brief Connect the UDP socket
static pomelo_platform_udp_t * platform_uv_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_udp_connect(impl->platform_uv, address);
}


/// @brief Stop the UDP socket
static int platform_uv_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_udp_stop(impl->platform_uv, socket);
}


/// @brief Send a packet to the UDP socket
static int platform_uv_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_udp_send(
        impl->platform_uv,
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
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_platform_uv_udp_recv_start(
        impl->platform_uv, socket, context, alloc_callback, recv_callback
    );
}


/// @brief Start the timer
static int platform_uv_timer_start(
    pomelo_platform_t * platform,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    return pomelo_platform_uv_timer_start(
        impl->platform_uv,
        entry,
        timeout_ms,
        repeat_ms,
        data,
        handle
    );
}


/// @brief Stop the timer
static void platform_uv_timer_stop(
    pomelo_platform_t * platform,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_platform_uv_timer_stop(impl->platform_uv, handle);
}


/// @brief Get the platform statistic
static napi_value platform_uv_statistic(
    pomelo_platform_t * platform,
    napi_env env
) {
    pomelo_platform_uv_impl_t * impl = (pomelo_platform_uv_impl_t *) platform;
    pomelo_statistic_platform_uv_t statistic = { 0 };
    pomelo_platform_uv_statistic(
        (pomelo_platform_t *) impl->platform_uv,
        &statistic
    );

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
static pomelo_platform_t * platform_uv_create(
    pomelo_allocator_t * allocator,
    napi_env env
) {
    assert(allocator != NULL);
    assert(env != NULL);

    pomelo_platform_uv_impl_t * platform = pomelo_allocator_malloc_t(
        allocator,
        pomelo_platform_uv_impl_t
    );
    if (!platform) return NULL;
    memset(platform, 0, sizeof(pomelo_platform_uv_impl_t));
    platform->allocator = allocator;

    // Get UV loop
    uv_loop_t * uv_loop = NULL;
    napi_status status = napi_get_uv_event_loop(env, &uv_loop);
    if (status != napi_ok || !uv_loop) {
        platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }

    // Create platform UV
    pomelo_platform_uv_options_t options = {
        .allocator = allocator,
        .uv_loop = uv_loop
    };
    pomelo_platform_uv_t * platform_uv =
        (pomelo_platform_uv_t *) pomelo_platform_uv_create(&options);
    if (!platform_uv) {
        platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }

    // Set the extra data for platform
    pomelo_platform_uv_set_extra(platform_uv, platform);

    // Setup the interface
    platform->platform_uv = platform_uv;

    pomelo_platform_t * base = &platform->base;
    base->set_extra = platform_uv_set_extra;
    base->get_extra = platform_uv_get_extra;
    base->destroy = platform_uv_destroy;
    base->startup = platform_uv_startup;
    base->shutdown = platform_uv_shutdown;
    base->hrtime = platform_uv_hrtime;
    base->now = platform_uv_now;
    base->acquire_threadsafe_executor = platform_uv_acquire_threadsafe_executor;
    base->release_threadsafe_executor = platform_uv_release_threadsafe_executor;
    base->threadsafe_executor_submit = platform_uv_threadsafe_executor_submit;
    base->submit_worker_task = platform_uv_submit_worker_task;
    base->cancel_worker_task = platform_uv_cancel_worker_task;
    base->udp_bind = platform_uv_udp_bind;
    base->udp_connect = platform_uv_udp_connect;
    base->udp_stop = platform_uv_udp_stop;
    base->udp_send = platform_uv_udp_send;
    base->udp_recv_start = platform_uv_udp_recv_start;
    base->timer_start = platform_uv_timer_start;
    base->timer_stop = platform_uv_timer_stop;
    base->statistic = platform_uv_statistic;

    return base;
}


/// @brief Platform-uv init function
static napi_value platform_uv_init(napi_env env, napi_callback_info info) {
    (void) info;

    pomelo_allocator_t * allocator = pomelo_allocator_default();
    pomelo_platform_t * platform = platform_uv_create(allocator, env);
    if (!platform) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        return NULL;
    }

    napi_value result = NULL;
    napi_status status =
        napi_create_external(env, platform, NULL, NULL, &result);
    if (status != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        platform_uv_destroy((pomelo_platform_t *) platform);
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
