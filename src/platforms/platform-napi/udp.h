#ifndef POMELO_NODE_PLATFORM_NAPI_UDP_H
#define POMELO_NODE_PLATFORM_NAPI_UDP_H
#include "node_api.h"
#include "platform/platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The UDP info structure
typedef struct pomelo_platform_udp_info_s pomelo_platform_udp_info_t;


struct pomelo_platform_udp_info_s {
    /// @brief The context
    void * context;

    /// @brief The platform
    pomelo_platform_napi_t * platform;

    /// @brief The socket reference
    napi_ref socket_ref;

    /// @brief The alloc callback
    pomelo_platform_alloc_cb alloc_callback;

    /// @brief The recv callback
    pomelo_platform_recv_cb recv_callback;
};


/// @brief The UDP recv callback
napi_value pomelo_platform_udp_recv_callback(
    napi_env env,
    napi_callback_info info
);


/// @brief The UDP send callback
napi_value pomelo_platform_udp_send_callback(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalize and release the UDP info
void pomelo_platform_udp_info_finalize(pomelo_platform_udp_info_t * info);


/// @brief Bind the UDP socket
pomelo_platform_udp_t * pomelo_platform_napi_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
);


/// @brief Connect the UDP socket
pomelo_platform_udp_t * pomelo_platform_napi_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
);


/// @brief Stop the UDP socket
int pomelo_platform_napi_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
);


/// @brief Send a packet to the UDP socket
int pomelo_platform_napi_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
);


/// @brief Start receiving packets from the UDP socket
void pomelo_platform_napi_udp_recv_start(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_NODE_PLATFORM_NAPI_UDP_H
