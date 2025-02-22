#ifndef POMELO_NODE_CHANNEL_H
#define POMELO_NODE_CHANNEL_H
#include "module.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_channel_s {
    /// @brief Native channel
    pomelo_channel_t * channel;

    /// @brief Reference of 'this' object
    napi_ref thiz;

    /// @brief Node of mesasge in pool
    pomelo_list_node_t * pool_node;
};


/*----------------------------------------------------------------------------*/
/*                               Public APIs                                  */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the channel module
napi_status pomelo_node_init_channel_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


/// @brief Create new channel
napi_value pomelo_node_channel_new(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_channel_t * channel
);

/// @brief Destroy a channel
void pomelo_node_channel_delete(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_node_channel_t * node_channel
);

/*----------------------------------------------------------------------------*/
/*                              Private APIs                                  */
/*----------------------------------------------------------------------------*/

napi_value pomelo_node_channel_constructor(
    napi_env env,
    napi_callback_info info
);

void pomelo_node_channel_finalize(
    napi_env env,
    pomelo_node_channel_t * node_channel,
    pomelo_node_context_t * context
);

napi_value pomelo_node_channel_set_mode(napi_env env, napi_callback_info info);

napi_value pomelo_node_channel_get_mode(napi_env env, napi_callback_info info);

napi_value pomelo_node_channel_send(napi_env env, napi_callback_info info);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_CHANNEL_H
