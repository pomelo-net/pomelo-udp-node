#include <assert.h>
#include "platform.h"


void pomelo_platform_set_extra(pomelo_platform_t * platform, void * data) {
    assert(platform != NULL);
    platform->set_extra(platform, data);
}


void * pomelo_platform_get_extra(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return platform->get_extra(platform);
}


/* -------------------------------------------------------------------------- */
/*                               Common APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_platform_startup(pomelo_platform_t * platform) {
    assert(platform != NULL);
    platform->startup(platform);
}


void pomelo_platform_shutdown(
    pomelo_platform_t * platform,
    pomelo_platform_shutdown_callback callback
) {
    assert(platform != NULL);
    platform->shutdown(platform, callback);
}


/* -------------------------------------------------------------------------- */
/*                                 Time APIs                                  */
/* -------------------------------------------------------------------------- */


uint64_t pomelo_platform_hrtime(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return platform->hrtime(platform);
}


uint64_t pomelo_platform_now(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return platform->now(platform);
}


/* -------------------------------------------------------------------------- */
/*                           Threadsafe executor APIs                         */
/* -------------------------------------------------------------------------- */

pomelo_threadsafe_executor_t * pomelo_platform_acquire_threadsafe_executor(
    pomelo_platform_t * platform
) {
    assert(platform != NULL);
    return platform->acquire_threadsafe_executor(platform);
}


void pomelo_platform_release_threadsafe_executor(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor
) {
    assert(platform != NULL);
    platform->release_threadsafe_executor(platform, executor);
}


pomelo_platform_task_t * pomelo_threadsafe_executor_submit(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(platform != NULL);
    assert(executor != NULL);
    return platform->threadsafe_executor_submit(
        platform,
        executor,
        entry,
        data
    );
}


/* -------------------------------------------------------------------------- */
/*                               Worker APIs                                  */
/* -------------------------------------------------------------------------- */

pomelo_platform_task_t * pomelo_platform_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
) {
    assert(platform != NULL);
    return platform->submit_worker_task(platform, entry, complete, data);
}


void pomelo_platform_cancel_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_t * task
) {
    assert(platform != NULL);
    platform->cancel_worker_task(platform, task);
}


/* -------------------------------------------------------------------------- */
/*                               UDP APIs                                    */
/* -------------------------------------------------------------------------- */


pomelo_platform_udp_t * pomelo_platform_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    return platform->udp_bind(platform, address);
}


pomelo_platform_udp_t * pomelo_platform_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    return platform->udp_connect(platform, address);
}


int pomelo_platform_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    return platform->udp_stop(platform, socket);
}


int pomelo_platform_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
) {
    assert(platform != NULL);
    return platform->udp_send(
        platform, socket, address, niovec, iovec, callback_data, send_callback
    );
}


void pomelo_platform_udp_recv_start(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
) {
    assert(platform != NULL);
    platform->udp_recv_start(
        platform, socket, context, alloc_callback, recv_callback
    );
}


/* -------------------------------------------------------------------------- */
/*                               Timer APIs                                   */
/* -------------------------------------------------------------------------- */


int pomelo_platform_timer_start(
    pomelo_platform_t * platform,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    return platform->timer_start(
        platform, entry, timeout_ms, repeat_ms, data, handle
    );
}


void pomelo_platform_timer_stop(
    pomelo_platform_t * platform,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    platform->timer_stop(platform, handle);
}
