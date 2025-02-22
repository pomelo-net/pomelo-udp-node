#ifndef POMELO_NODE_SOCKET_SRC_H
#define POMELO_NODE_SOCKET_SRC_H
#include "module.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_socket_s {
    /// @brief Context
    pomelo_node_context_t * context;

    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief "this" object
    napi_ref thiz;

    /// @brief The listener
    napi_ref listener;
    
    /* Callbacks */

    /// @brief The connected callback
    napi_ref on_connected;

    /// @brief The disconnected callback
    napi_ref on_disconnected;

    /// @brief The received callback
    napi_ref on_received;

    /// @brief Stopped promise deferred callback
    napi_deferred on_stopped;

    /// @brief Connect result promise deferred callback
    napi_deferred on_connect_result;

    /// @brief Number of channels
    size_t nchannels;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the socket module
napi_status pomelo_node_init_socket_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Socket.constructor(listener: SocketListener): Socket
napi_value pomelo_node_socket_constructor(
    napi_env env,
    napi_callback_info info
);

/// @brief Finalize function of socket
void pomelo_node_socket_finalize(
    napi_env env,
    pomelo_node_socket_t * node_socket,
    pomelo_node_context_t * context
);

/// @brief Socket.setListener()
napi_value pomelo_node_socket_set_listener(
    napi_env env,
    napi_callback_info info
);

/// @brief Socket.listen()
napi_value pomelo_node_socket_listen(napi_env env, napi_callback_info info);

/// @brief Socket.connect()
napi_value pomelo_node_socket_connect(napi_env env, napi_callback_info info);

/// @brief Socket.stop()
napi_value pomelo_node_socket_stop(napi_env env, napi_callback_info info);

/// @brief Socket.statistic()
napi_value pomelo_node_socket_statistic(napi_env env, napi_callback_info info);

/// @brief Socket.send()
napi_value pomelo_node_socket_send(napi_env env, napi_callback_info info);

/// @brief Socket.time()
napi_value pomelo_node_socket_time(napi_env env, napi_callback_info info);

void pomelo_node_socket_cancel_connect_result_deferred(
    pomelo_node_socket_t * node_socket
);

void pomelo_node_socket_cancel_stopped_deferred(
    pomelo_node_socket_t * node_socket
);

void pomelo_node_socket_call_listener(
    pomelo_node_socket_t * node_socket,
    napi_ref callback,
    napi_value * argv,
    size_t argc
);


#ifdef __cplusplus
}
#endif

#endif // POMELO_NODE_SOCKET_SRC_H
