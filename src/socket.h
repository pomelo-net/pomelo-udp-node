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

    /// @brief Connect result promise deferred callback
    napi_deferred on_connect_result_deferred;

    /// @brief Number of channels
    size_t nchannels;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the socket module
napi_status pomelo_node_init_socket_module(napi_env env, napi_value ns);


/// @brief Initialize the socket
int pomelo_node_socket_init(
    pomelo_node_socket_t * node_socket,
    pomelo_node_context_t * context
);


/// @brief Cleanup the socket
void pomelo_node_socket_cleanup(pomelo_node_socket_t * node_socket);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Socket.constructor(listener: SocketListener): Socket
napi_value pomelo_node_socket_constructor(
    napi_env env,
    napi_callback_info info
);

/// @brief Finalizer of socket
void pomelo_node_socket_finalizer(
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

/// @brief Socket.send()
napi_value pomelo_node_socket_send(napi_env env, napi_callback_info info);

/// @brief Socket.time()
napi_value pomelo_node_socket_time(napi_env env, napi_callback_info info);

/// @brief Call the listener
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
