#ifndef POMELO_NODE_SESSION_SRC_H
#define POMELO_NODE_SESSION_SRC_H
#include "module.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_session_s {
    /// @brief The native session
    pomelo_session_t * session;

    /// @brief The reference of this object (session)
    napi_ref thiz;

    /// @brief Reference to array of channels (lazy getting)
    napi_ref channels;

    /// @brief Socket of this session
    pomelo_node_socket_t * node_socket;

    /// @brief Node of session in pool
    pomelo_list_node_t * pool_node;

    /// @brief List of channels.
    /// The `channels` array above can be modified, we need another place to
    /// store the channels of session
    pomelo_list_t * list_channels;
};


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the session module
napi_status pomelo_node_init_session_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


/// @brief Create new JS session object from session native object
napi_value pomelo_node_session_new(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_session_t * session,
    pomelo_node_socket_t * node_socket
);


/// @brief Delete the session
void pomelo_node_session_delete(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value js_session
);


napi_value pomelo_node_session_of(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_session_t * session
);


/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Session.constructor(external: External)
napi_value pomelo_node_session_constructor(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalize function of socket
void pomelo_node_session_finalize(
    napi_env env,
    pomelo_node_session_t * node_session,
    pomelo_node_context_t * context
);


/// @brief send(channelIndex: number, message: Message): boolean
napi_value pomelo_node_session_send(
    napi_env env,
    napi_callback_info info
);


/// @brief readonly Session.id: number
napi_value pomelo_node_session_get_id(
    napi_env env,
    napi_callback_info info
);


/// @brief Session.disconnect(): void
napi_value pomelo_node_session_disconnect(
    napi_env env,
    napi_callback_info info
);


/// @brief Session.setChannelMode(channelIndex: number, mode: ChannelMode)
napi_value pomelo_node_session_set_channel_mode(
    napi_env env,
    napi_callback_info info
);


/// @brief Session.getChannelMode(channelIndex: number): ChannelMode
napi_value pomelo_node_session_get_channel_mode(
    napi_env env,
    napi_callback_info info
);

/// @brief Session.rtt(): RTT
napi_value pomelo_node_session_rtt(napi_env env, napi_callback_info info);


/// @brief Session.channels: Channel[]
napi_value pomelo_node_session_get_channels(
    napi_env env, napi_callback_info info
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_SESSION_SRC_H
