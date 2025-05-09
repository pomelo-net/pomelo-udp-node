#include <assert.h>
#include "platform.h"


/**
 * In the context of this file, the platform pointer is actually a pointer to
 * the platform interface.
 */


void pomelo_platform_set_extra(pomelo_platform_t * platform, void * data) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->extra = data;
}


void * pomelo_platform_get_extra(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->extra;
}


/* -------------------------------------------------------------------------- */
/*                               Common APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_platform_startup(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->startup(i);
}


void pomelo_platform_shutdown(
    pomelo_platform_t * platform,
    pomelo_platform_shutdown_callback callback
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->shutdown(i, callback);
}


/* -------------------------------------------------------------------------- */
/*                                 Time APIs                                  */
/* -------------------------------------------------------------------------- */


uint64_t pomelo_platform_hrtime(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->hrtime(i);
}

uint64_t pomelo_platform_now(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->now(i);
}


/* -------------------------------------------------------------------------- */
/*                           Threadsafe executor APIs                         */
/* -------------------------------------------------------------------------- */

pomelo_threadsafe_executor_t * pomelo_platform_acquire_threadsafe_executor(
    pomelo_platform_t * platform
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->acquire_threadsafe_executor(i);
}


void pomelo_platform_release_threadsafe_executor(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->release_threadsafe_executor(i, executor);
}


pomelo_platform_task_t * pomelo_threadsafe_executor_submit(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(platform != NULL);
    assert(executor != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->threadsafe_executor_submit(i, executor, entry, data);
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
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->submit_worker_task(i, entry, complete, data);
}


void pomelo_platform_cancel_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_t * task
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->cancel_worker_task(i, task);
}


/* -------------------------------------------------------------------------- */
/*                               UDP APIs                                    */
/* -------------------------------------------------------------------------- */


pomelo_platform_udp_t * pomelo_platform_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->udp_bind(i, address);
}


pomelo_platform_udp_t * pomelo_platform_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->udp_connect(i, address);
}


int pomelo_platform_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->udp_stop(i, socket);
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
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->udp_send(
        i, socket, address, niovec, iovec, callback_data, send_callback
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
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->udp_recv_start(i, socket, context, alloc_callback, recv_callback);
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
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    return i->timer_start(i, entry, timeout_ms, repeat_ms, data, handle);
}


void pomelo_platform_timer_stop(
    pomelo_platform_t * platform,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    i->timer_stop(i, handle);
}
