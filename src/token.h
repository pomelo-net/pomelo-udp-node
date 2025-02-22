#ifndef POMELO_NODE_TOKEN_SRC_H
#define POMELO_NODE_TOKEN_SRC_H
#include "module.h"


#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the token module
napi_status pomelo_node_init_token_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Token.encode(...): Uint8Array
napi_value pomelo_node_token_encode(napi_env env, napi_callback_info info);


/// @brief Token.randomBuffer(...): Uint8Array
napi_value pomelo_node_token_random_buffer(
    napi_env env,
    napi_callback_info info
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_TOKEN_SRC_H
