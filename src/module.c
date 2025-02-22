#include <assert.h>
#include "module.h"
#include "uv.h"
#include "error.h"
#include "utils.h"
#include "context.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "token.h"
#include "channel.h"
#include "plugin.h"


static void pomelo_node_parse_init_options(
    napi_env env,
    pomelo_node_context_options_t * options,
    napi_value js_options
);

/* -------------------------------------------------------------------------- */
/*                             Addon initializer                              */
/* -------------------------------------------------------------------------- */


#define POMELO_NODE_INIT_ARGC 1
napi_value pomelo_node_init(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_INIT_ARGC;
    napi_value argv[POMELO_NODE_INIT_ARGC] = { NULL };
    napi_call(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    napi_value js_options = NULL;
    if (argc > 0) {
        napi_valuetype type = 0;
        napi_call(napi_typeof(env, argv[0], &type));
        if (type != napi_object) {
            napi_throw_arg("options");
            return NULL;
        }
        js_options = argv[0];
    }

    // Create context
    pomelo_node_context_options_t options;
    pomelo_node_context_options_init(&options);
    options.allocator = pomelo_allocator_default();
    options.env = env;
    pomelo_node_parse_init_options(env, &options, js_options);

    pomelo_node_context_t * context = pomelo_node_context_create(&options);
    if (!context) {
        napi_throw_msg(POMELO_NODE_ERROR_INIT_POMELO);
        return NULL;
    }

    // Set the instance data
    napi_call(napi_set_instance_data(
        env, context, (napi_finalize) pomelo_node_context_finalizer, NULL
    ));

    // Create namespace for this module
    napi_value ns = NULL;
    napi_call(napi_create_object(env, &ns));

    // Initialize all enums & modules
    napi_call(pomelo_node_init_all_enums(env, context, ns));
    napi_call(pomelo_node_init_all_modules(env, context, ns));

    return ns;
}


napi_value pomelo_node_main(napi_env env, napi_value exports) {
    (void) exports;
    napi_value value;
    napi_status ret = napi_create_function(
        env, "init", NAPI_AUTO_LENGTH, pomelo_node_init, NULL, &value
    );

    if (ret != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_INIT_POMELO);
        return NULL;
    }

    return value;
}
NAPI_MODULE(pomelo_udp_node, pomelo_node_main);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

static void pomelo_node_parse_init_options(
    napi_env env,
    pomelo_node_context_options_t * options,
    napi_value js_options
) {
    if (!js_options) return; // No JS options

    uint64_t value = 0;
    int ret = pomelo_node_get_uint64_property(
        env, js_options, "poolMessageMax", &value
    );
    if (ret == 0) {
        options->pool_message_max = (size_t) value;
    }

    ret = pomelo_node_get_uint64_property(
        env, js_options, "poolSessionMax", &value
    );
    if (ret == 0) {
        options->pool_session_max = (size_t) value;
    }

    ret = pomelo_node_get_uint64_property(
        env, js_options, "poolChannelMax", &value
    );
    if (ret == 0) {
        options->pool_channel_max = (size_t) value;
    }

    bool has_error_handler = false;
    napi_callv(napi_has_named_property(
        env, js_options, "errorHandler", &has_error_handler
    ));
    if (!has_error_handler) return;
    napi_callv(napi_get_named_property(
        env, js_options, "errorHandler", &options->error_handler
    ));
}


napi_status pomelo_node_init_all_enums(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    napi_value enum_value = NULL;
    napi_value value = NULL;

    // enum ChannelMode
    napi_calls(napi_create_object(env, &enum_value));
    napi_calls(napi_create_int32(env, POMELO_CHANNEL_MODE_UNRELIABLE, &value));
    napi_calls(napi_set_named_property(env, enum_value, "UNRELIABLE", value));
    napi_calls(napi_create_int32(env, POMELO_CHANNEL_MODE_SEQUENCED, &value));
    napi_calls(napi_set_named_property(env, enum_value, "SEQUENCED", value));
    napi_calls(napi_create_int32(env, POMELO_CHANNEL_MODE_RELIABLE, &value));
    napi_calls(napi_set_named_property(env, enum_value, "RELIABLE", value));
    napi_calls(napi_set_named_property(env, ns, "ChannelMode", enum_value));

    // enum ConnectResult
    napi_calls(napi_create_object(env, &enum_value));
    napi_calls(napi_create_int32(env, POMELO_SOCKET_CONNECT_SUCCESS, &value));
    napi_calls(napi_set_named_property(env, enum_value, "SUCCESS", value));
    napi_calls(napi_create_int32(env, POMELO_SOCKET_CONNECT_DENIED, &value));
    napi_calls(napi_set_named_property(env, enum_value, "DENIED", value));
    napi_calls(napi_create_int32(env, POMELO_SOCKET_CONNECT_TIMED_OUT, &value));
    napi_calls(napi_set_named_property(env, enum_value, "TIMED_OUT", value));
    napi_calls(napi_set_named_property(env, ns, "ConnectResult", enum_value));

    return napi_ok;
}


napi_status pomelo_node_init_all_modules(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    // Initialize socket modules
    napi_calls(pomelo_node_init_socket_module(env, context, ns));
    napi_calls(pomelo_node_init_session_module(env, context, ns));
    napi_calls(pomelo_node_init_message_module(env, context, ns));
    napi_calls(pomelo_node_init_token_module(env, context, ns));
    napi_calls(pomelo_node_init_channel_module(env, context, ns));
    napi_calls(pomelo_node_init_plugin_module(env, context, ns));

    return napi_ok;
}
