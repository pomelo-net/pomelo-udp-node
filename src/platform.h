#ifndef POMELO_NODE_PLATFORM_H
#define POMELO_NODE_PLATFORM_H
#include "node_api.h"
#include "platform/platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/**
 * Implementation of node-api platform.
 */

/// @brief The platform interface
typedef struct pomelo_platform_interface_s pomelo_platform_interface_t;


struct pomelo_platform_interface_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief Implemented platform
    void * internal;

    /// @brief Extra data
    void * extra;

    /// @brief The shutdown callback
    pomelo_platform_shutdown_callback shutdown_callback;

    /// @brief Destroy the platform
    void (*destroy)(pomelo_platform_interface_t * i);

    /// @brief The startup function
    void (*startup)(pomelo_platform_interface_t * i);

    /// @brief The shutdown function
    void (*shutdown)(
        pomelo_platform_interface_t * i,
        pomelo_platform_shutdown_callback callback
    );

    /// @brief The hrtime function
    uint64_t (*hrtime)(pomelo_platform_interface_t * i);

    /// @brief The now function
    uint64_t (*now)(pomelo_platform_interface_t * i);

    /// @brief Acquire a threadsafe executor
    pomelo_threadsafe_executor_t * (*acquire_threadsafe_executor)(
        pomelo_platform_interface_t * i
    );

    /// @brief Release a threadsafe executor
    void (*release_threadsafe_executor)(
        pomelo_platform_interface_t * i,
        pomelo_threadsafe_executor_t * executor
    );

    /// @brief Submit a task to run in platform thread
    pomelo_platform_task_t * (*threadsafe_executor_submit)(
        pomelo_platform_interface_t * i,
        pomelo_threadsafe_executor_t * executor,
        pomelo_platform_task_entry entry,
        void * data
    );

    /* Platform worker APIs */

    /// @brief Submit a task to run in worker thread
    pomelo_platform_task_t * (*submit_worker_task)(
        pomelo_platform_interface_t * i,
        pomelo_platform_task_entry entry,
        pomelo_platform_task_complete complete,
        void * data
    );

    /// @brief Cancel a worker task
    void (*cancel_worker_task)(
        pomelo_platform_interface_t * i,
        pomelo_platform_task_t * task
    );

    /* Platform UDP APIs */

    /// @brief Bind the socket with specific address
    pomelo_platform_udp_t * (*udp_bind)(
        pomelo_platform_interface_t * i,
        pomelo_address_t * address
    );

    /// @brief Connect the socket to specific address
    pomelo_platform_udp_t * (*udp_connect)(
        pomelo_platform_interface_t * i,
        pomelo_address_t * address
    );

    /// @brief Stop the socket
    int (*udp_stop)(
        pomelo_platform_interface_t * i,
        pomelo_platform_udp_t * socket
    );

    /// @brief Send a packet to target
    int (*udp_send)(
        pomelo_platform_interface_t * i,
        pomelo_platform_udp_t * socket,
        pomelo_address_t * address,
        int niovec,
        pomelo_platform_iovec_t * iovec,
        void * callback_data,
        pomelo_platform_send_cb send_callback
    );

    /// @brief Start receiving packets from socket
    void (*udp_recv_start)(
        pomelo_platform_interface_t * i,
        pomelo_platform_udp_t * socket,
        void * context,
        pomelo_platform_alloc_cb alloc_callback,
        pomelo_platform_recv_cb recv_callback
    );

    /* Platform timer APIs */

    /// @brief Start the timer
    int (*timer_start)(
        pomelo_platform_interface_t * i,
        pomelo_platform_timer_entry entry,
        uint64_t timeout_ms,
        uint64_t repeat_ms,
        void * data,
        pomelo_platform_timer_handle_t * handle
    );

    /// @brief Stop the timer
    void (*timer_stop)(
        pomelo_platform_interface_t * i,
        pomelo_platform_timer_handle_t * handle
    );

    /// @brief Get the statistic
    napi_value (*statistic)(pomelo_platform_interface_t * i, napi_env env);
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_PLATFORM_H
