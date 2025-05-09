#ifndef POMELO_NODE_PLUGIN_H
#define POMELO_NODE_PLUGIN_H
#include "module.h"
#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize plugin module
napi_status pomelo_node_init_plugin_module(napi_env env, napi_value ns);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

napi_value pomelo_node_plugin_register_plugin_by_name(
    napi_env env,
    napi_callback_info info
);


napi_value pomelo_node_plugin_register_plugin_by_path(
    napi_env env,
    napi_callback_info info
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_PLUGIN_H
