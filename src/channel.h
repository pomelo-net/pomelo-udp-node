#ifndef POMELO_NODE_CHANNEL_H
#define POMELO_NODE_CHANNEL_H
#include "module.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_channel_s {
    /// @brief The context
    pomelo_node_context_t * context;

    /// @brief Native channel
    pomelo_channel_t * channel;

    /// @brief Reference of 'this' object
    napi_ref thiz;
};


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the channel module
napi_status pomelo_node_init_channel_module(napi_env env, napi_value ns);


/// @brief Create new JS channel
napi_value pomelo_node_channel_new(napi_env env, pomelo_channel_t * channel);


/// @brief Initialize the channel
int pomelo_node_channel_init(
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
);


/// @brief Cleanup the channel module
void pomelo_node_channel_cleanup(pomelo_node_channel_t * node_channel);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Constructor of channel
napi_value pomelo_node_channel_constructor(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalizer of channel
void pomelo_node_channel_finalizer(
    napi_env env,
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
);


/// @brief Message.mode setter
napi_value pomelo_node_channel_set_mode(napi_env env, napi_callback_info info);


/// @brief Message.mode getter
napi_value pomelo_node_channel_get_mode(napi_env env, napi_callback_info info);


/// @brief Message.send()
napi_value pomelo_node_channel_send(napi_env env, napi_callback_info info);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_CHANNEL_H
