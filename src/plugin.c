#include <assert.h>
#include <stdio.h>
#include "plugin.h"
#include "utils.h"
#include "error.h"
#include "context.h"
#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX MAX_PATH
#else
#include <limits.h>
#endif

/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize plugin module
napi_status pomelo_node_init_plugin_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    napi_value plugin = NULL;
    napi_calls(napi_create_object(env, &plugin));

    napi_value fn = NULL;
    napi_calls(napi_create_function(
        env,
        "registerPluginByName",
        NAPI_AUTO_LENGTH,
        pomelo_node_plugin_register_plugin_by_name,
        context,
        &fn
    ));
    napi_calls(napi_set_named_property(
        env, plugin, "registerPluginByName", fn
    ));

    napi_calls(napi_create_function(
        env,
        "registerPluginByPath",
        NAPI_AUTO_LENGTH,
        pomelo_node_plugin_register_plugin_by_path,
        context,
        &fn
    ));
    napi_calls(napi_set_named_property(
        env, plugin, "registerPluginByPath", fn
    ));

    napi_calls(napi_set_named_property(env, ns, "Plugin", plugin));
    return napi_ok;
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

#define POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_NAME_ARGC 1
napi_value pomelo_node_plugin_register_plugin_by_name(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_NAME_ARGC;
    napi_value argv[POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_NAME_ARGC] = { NULL };
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, NULL, (void **) &context
    ));

    if (argc < POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_NAME_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    napi_valuetype type = 0;
    napi_call(napi_typeof(env, argv[0], &type));
    if (type != napi_string) {
        napi_throw_arg("name");
        return NULL;
    }

    char name[FILENAME_MAX] = { 0 };
    napi_call(napi_get_value_string_utf8(
        env, argv[0], name, FILENAME_MAX, NULL
    ));

    pomelo_plugin_initializer initializer = pomelo_plugin_load_by_name(name);
    if (!initializer) {
        napi_value result = NULL;
        napi_call(napi_get_boolean(env, false, &result));
        return result;
    }

    pomelo_plugin_t * plugin = pomelo_plugin_register(
        context->allocator,
        (pomelo_context_t *) context->context,
        context->platform,
        initializer
    );

    napi_value result = NULL;
    napi_call(napi_get_boolean(env, plugin != NULL, &result));
    return result;
}



#define POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_PATH_ARGC 1
napi_value pomelo_node_plugin_register_plugin_by_path(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_PATH_ARGC;
    napi_value argv[POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_PATH_ARGC] = { NULL };
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, &argc, argv, NULL, (void **) &context
    ));

    if (argc < POMELO_NODE_PLUGIN_REGISTER_PLUGIN_BY_PATH_ARGC) {
        napi_throw_msg(POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    napi_valuetype type = 0;
    napi_call(napi_typeof(env, argv[0], &type));
    if (type != napi_string) {
        napi_throw_arg("path");
        return NULL;
    }

    char path[PATH_MAX] = { 0 };
    napi_call(napi_get_value_string_utf8(env, argv[0], path, PATH_MAX, NULL));

    pomelo_plugin_initializer initializer = pomelo_plugin_load_by_path(path);
    if (!initializer) {
        napi_value result = NULL;
        napi_call(napi_get_boolean(env, false, &result));
        return result;
    }

    pomelo_plugin_t * plugin = pomelo_plugin_register(
        context->allocator,
        (pomelo_context_t *) context->context,
        context->platform,
        initializer
    );

    napi_value result = NULL;
    napi_call(napi_get_boolean(env, plugin != NULL, &result));
    return result;
}
