#ifndef POMELO_NODE_MODULE_SRC_H
#define POMELO_NODE_MODULE_SRC_H
#include "node_api.h"
#include "pomelo.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The binding node message
typedef struct pomelo_node_message_s pomelo_node_message_t;

/// @brief The binding node session structure
typedef struct pomelo_node_session_s pomelo_node_session_t;

/// @brief The binding node socket
typedef struct pomelo_node_socket_s pomelo_node_socket_t;

/// @brief The binding node channel
typedef struct pomelo_node_channel_s pomelo_node_channel_t;

/// @brief The context of addon
typedef struct pomelo_node_context_s pomelo_node_context_t;


/* -------------------------------------------------------------------------- */
/*                       Module initializing functions                        */
/* -------------------------------------------------------------------------- */

/// @brief Init function of module
napi_value pomelo_node_init(napi_env env, napi_callback_info info);

/// @brief Create the Pomelo UDP addon
napi_value pomelo_node_main(napi_env env, napi_value exports);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Initialize all enums
napi_status pomelo_node_init_all_enums(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


/// @brief Initialize all modules
napi_status pomelo_node_init_all_modules(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_MODULE_SRC_H
