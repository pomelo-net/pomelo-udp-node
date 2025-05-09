#ifndef POMELO_NODE_SESSION_SRC_H
#define POMELO_NODE_SESSION_SRC_H
#include "module.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_session_s {
    /// @brief The context
    pomelo_node_context_t * context;

    /// @brief The native session
    pomelo_session_t * session;

    /// @brief The reference of this object (session)
    napi_ref thiz;

    /// @brief Reference to array of channels (lazy getting)
    napi_ref channels;
};


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the session module
napi_status pomelo_node_init_session_module(napi_env env, napi_value ns);


/// @brief Create new JS session object from session native object
napi_value pomelo_node_session_new(napi_env env, pomelo_session_t * session);


/// @brief Initialize the session
int pomelo_node_session_init(
    pomelo_node_session_t * node_session,
    pomelo_node_context_t * context
);


/// @brief Cleanup the session
void pomelo_node_session_cleanup(pomelo_node_session_t * node_session);


/// @brief Get the JS session object from session native object
napi_value pomelo_node_js_session_of(pomelo_session_t * session);


/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Session.constructor(external: External)
napi_value pomelo_node_session_constructor(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalizer of session
void pomelo_node_session_finalizer(
    napi_env env,
    pomelo_node_session_t * node_session,
    pomelo_node_context_t * context
);


/// @brief send(channelIndex: number, message: Message): boolean
napi_value pomelo_node_session_send(napi_env env, napi_callback_info info);


/// @brief readonly Session.id: number
napi_value pomelo_node_session_get_id(napi_env env, napi_callback_info info);


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
    napi_env env,
    napi_callback_info info
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_SESSION_SRC_H
