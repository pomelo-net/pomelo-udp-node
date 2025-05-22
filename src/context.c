#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "context.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "channel.h"
#include "utils.h"
#include "platform.h"


pomelo_node_context_t * pomelo_node_context_create(
    pomelo_node_context_options_t * options
) {
    assert(options != NULL);
    if (!options->allocator || !options->env || !options->platform) {
        return NULL; // Invalid options
    }

    pomelo_allocator_t * allocator = options->allocator;
    pomelo_node_context_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_node_context_t);
    if (!context) {
        return NULL; // Failed to allocate new memory
    }
    memset(context, 0, sizeof(pomelo_node_context_t));
    context->allocator = allocator;
    context->env = options->env;

    // Create native context
    pomelo_context_root_options_t context_options = {
        .allocator = allocator
    };
    context->context = pomelo_context_root_create(&context_options);
    if (!context->context) {
        // Failed to create native context
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Get platform from object
    void * platform = NULL;
    napi_valuetype type;
    napi_status status = napi_typeof(options->env, options->platform, &type);
    if (status != napi_ok || type != napi_external) {
        pomelo_node_context_destroy(context);
        return NULL;
    }
    status = napi_get_value_external(
        options->env,
        options->platform,
        &platform
    );
    if (status != napi_ok || !platform) {
        pomelo_node_context_destroy(context);
        return NULL;
    }
    context->platform = platform;

    // Start the platform
    pomelo_platform_startup(context->platform);

    // Create pool of sockets
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_node_socket_t);
    pool_options.available_max = options->pool_socket_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_node_socket_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_node_socket_cleanup;
    pool_options.zero_init = true;
    context->pool_socket = pomelo_pool_root_create(&pool_options);
    if (!context->pool_socket) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_node_message_t);
    pool_options.available_max = options->pool_message_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_node_message_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_node_message_cleanup;
    pool_options.zero_init = true;
    context->pool_message = pomelo_pool_root_create(&pool_options);
    if (!context->pool_message) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create pool of sessions
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_node_session_t);
    pool_options.available_max = options->pool_session_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_node_session_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_node_session_cleanup;
    pool_options.zero_init = true;
    context->pool_session = pomelo_pool_root_create(&pool_options);
    if (!context->pool_session) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create pool of channels
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_node_channel_t);
    pool_options.available_max = options->pool_channel_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_node_channel_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_node_channel_cleanup;
    pool_options.zero_init = true;
    context->pool_channel = pomelo_pool_root_create(&pool_options);
    if (!context->pool_channel) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Check error handler
    if (options->error_handler) {
        status = napi_create_reference(
            options->env, options->error_handler, 1, &context->error_handler
        );
        if (status != napi_ok) {
            pomelo_node_context_destroy(context);
            return NULL;
        }
    }

    // Create temporary session array
    pomelo_array_options_t array_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_session_t *)
    };
    context->tmp_send_sessions = pomelo_array_create(&array_options);
    if (!context->tmp_send_sessions) {
        pomelo_node_context_destroy(context);
        return NULL; // Failed to create temporary session array
    }

    return context;
}


static void context_on_shutdown(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    i->destroy(i);
}


void pomelo_node_context_destroy(pomelo_node_context_t * context) {
    assert(context != NULL);

    if (context->context) {
        pomelo_context_destroy(context->context);
        context->context = NULL;
    }

    if (context->platform) {
        pomelo_platform_shutdown(
            context->platform,
            (pomelo_platform_shutdown_callback) context_on_shutdown
        );
        context->platform = NULL;
    }

    if (context->pool_message) {
        pomelo_pool_destroy(context->pool_message);
        context->pool_message = NULL;
    }

    if (context->pool_session) {
        pomelo_pool_destroy(context->pool_session);
        context->pool_session = NULL;
    }

    if (context->pool_channel) {
        pomelo_pool_destroy(context->pool_channel);
        context->pool_channel = NULL;
    }

    if (context->error_handler) {
        napi_delete_reference(context->env, context->error_handler);
        context->error_handler = NULL;
    }

    if (context->tmp_send_sessions) {
        pomelo_array_destroy(context->tmp_send_sessions);
        context->tmp_send_sessions = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


void pomelo_node_context_finalizer(
    napi_env env,
    pomelo_node_context_t * context,
    void * finalize_hint
) {
    assert(context != NULL);
    (void) finalize_hint;

    pomelo_node_context_destroy(context);
}


pomelo_node_socket_t * pomelo_node_context_acquire_socket(
    pomelo_node_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_socket, context);
}


void pomelo_node_context_release_socket(
    pomelo_node_context_t * context,
    pomelo_node_socket_t * node_socket
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_socket, node_socket);
}


pomelo_node_session_t * pomelo_node_context_acquire_session(
    pomelo_node_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_session, context);
}


void pomelo_node_context_release_session(
    pomelo_node_context_t * context,
    pomelo_node_session_t * node_session
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_session, node_session);
}


pomelo_node_message_t * pomelo_node_context_acquire_message(
    pomelo_node_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_message, context);
}


void pomelo_node_context_release_message(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_message, node_message);
}


pomelo_node_channel_t * pomelo_node_context_acquire_channel(
    pomelo_node_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_channel, context);
}


/// @brief Destroy node channel
void pomelo_node_context_release_channel(
    pomelo_node_context_t * context,
    pomelo_node_channel_t * node_channel
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_channel, node_channel);
}


void pomelo_node_context_handle_error(
    pomelo_node_context_t * context,
    napi_value error
) {
    assert(context != NULL);
    if (!context->error_handler) {
        pomelo_node_context_handle_error_default(context, error);
        return; // No error handler is provided, call the default handler
    }

    napi_env env = context->env;
    napi_value error_handler = NULL;
    napi_callv(napi_get_reference_value(
        env, context->error_handler, &error_handler
    ));

    napi_value null_value = NULL;
    napi_callv(napi_get_null(env, &null_value));
    napi_status status =
        napi_call_function(env, null_value, error_handler, 1, &error, NULL);
    if (status == napi_pending_exception) {
        // There was an exception in error handler, call the default handler
        napi_value ex = NULL;
        napi_get_and_clear_last_exception(env, &ex);
        pomelo_node_context_handle_error_default(context, ex);
    }
}


#define POMELO_NODE_ERROR_STRING_BUFFER 512
void pomelo_node_context_handle_error_default(
    pomelo_node_context_t * context,
    napi_value error
) {
    assert(context != NULL);
    napi_env env = context->env;
    napi_value error_str = NULL;
    char error_buf[POMELO_NODE_ERROR_STRING_BUFFER];
    napi_callv(napi_coerce_to_string(env, error, &error_str));
    napi_callv(napi_get_value_string_utf8(
        env, error_str, error_buf, POMELO_NODE_ERROR_STRING_BUFFER, NULL
    ));

    fprintf(stderr, "Unhandled error: %s\n", error_buf);
}


napi_value pomelo_node_context_statistic(
    napi_env env,
    napi_callback_info info
) {
    pomelo_node_context_t * context = NULL;
    napi_call(napi_get_cb_info(
        env, info, NULL, NULL, NULL, (void **) &context
    ));

    // Get the context statistic
    pomelo_statistic_t context_statistic;
    pomelo_context_statistic(context->context, &context_statistic);

    // Create new wrapped object
    napi_value result;
    napi_value category;
    napi_value entity;

    napi_call(napi_create_object(env, &result));

    /* Allocator */

    // Create new allocator statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "allocator", category));

    // allocator_allocated_bytes
    napi_call(napi_create_bigint_uint64(
        env, context_statistic.allocator.allocated_bytes, &entity
    ));
    napi_call(napi_set_named_property(env, category, "allocatedBytes", entity));

    /* API statistic */

    // Create new API statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "api", category));

    // API messages
    napi_call(napi_create_uint32(env, context_statistic.api.messages, &entity));
    napi_call(napi_set_named_property(env, category, "messages", entity));

    // API builtin sessions
    napi_call(napi_create_uint32(
        env, context_statistic.api.builtin_sessions, &entity
    ));
    napi_call(napi_set_named_property(
        env, category, "builtinSessions", entity
    ));

    // API plugin sessions
    napi_call(napi_create_uint32(
        env, context_statistic.api.plugin_sessions, &entity
    ));
    napi_call(napi_set_named_property(env, category, "pluginSessions", entity));

    // API builtin channels
    napi_call(napi_create_uint32(
        env, context_statistic.api.builtin_channels, &entity
    ));
    napi_call(napi_set_named_property(
        env, category, "builtinChannels", entity
    ));

    // API plugin channels
    napi_call(napi_create_uint32(
        env, context_statistic.api.plugin_channels, &entity
    ));
    napi_call(napi_set_named_property(env, category, "pluginChannels", entity));

    /* buffer context statistic */

    // Create new buffer context statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "buffer", category));

    // buffer_context_buffers
    napi_call(napi_create_uint32(
        env, context_statistic.buffer.buffers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "buffers", entity));

    // Platform statistic
    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *)
        context->platform;
    category = i->statistic(i, env);
    if (!category) {
        napi_call(napi_create_object(env, &category));
    }
    napi_call(napi_set_named_property(env, result, "platform", category));

    /* Protocol statistic*/

    // Create protocol statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "protocol", category));

    // Protocol senders
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.senders, &entity
    ));
    napi_call(napi_set_named_property(env, category, "senders", entity));

    // Protocol receivers
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.receivers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "receivers", entity));

    // Protocol packets
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.packets, &entity
    ));
    napi_call(napi_set_named_property(env, category, "packets", entity));

    // Protocol peers
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.peers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "peers", entity));

    // Protocol servers
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.servers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "servers", entity));

    // Protocol clients
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.clients, &entity
    ));
    napi_call(napi_set_named_property(env, category, "clients", entity));

    // Protocol crypto contexts
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.crypto_contexts, &entity
    ));
    napi_call(napi_set_named_property(
        env, category, "cryptoContexts", entity
    ));

    // Protocol acceptances
    napi_call(napi_create_uint32(
        env, context_statistic.protocol.acceptances, &entity
    ));
    napi_call(napi_set_named_property(env, category, "acceptances", entity));

    /* Delivery statistic */

    // Create delivery statistic object
    napi_call(napi_create_object(env, &category));
    napi_call(napi_set_named_property(env, result, "delivery", category));

    // Delivery dispatchers
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.dispatchers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "dispatchers", entity));

    // Delivery senders
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.senders, &entity
    ));
    napi_call(napi_set_named_property(env, category, "senders", entity));

    // Delivery receivers
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.receivers, &entity
    ));
    napi_call(napi_set_named_property(env, category, "receivers", entity));

    // Delivery endpoints
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.endpoints, &entity
    ));
    napi_call(napi_set_named_property(env, category, "endpoints", entity));

    // Delivery buses
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.buses, &entity
    ));
    napi_call(napi_set_named_property(env, category, "buses", entity));
    
    // Delivery receptions
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.receptions, &entity
    ));
    napi_call(napi_set_named_property(env, category, "receptions", entity));
    
    // Delivery transmissions
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.transmissions, &entity
    ));
    napi_call(napi_set_named_property(env, category, "transmissions", entity));
    
    // Delivery parcels
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.parcels, &entity
    ));
    napi_call(napi_set_named_property(env, category, "parcels", entity));
    
    // Delivery heartbeats
    napi_call(napi_create_uint32(
        env, context_statistic.delivery.heartbeats, &entity
    ));
    napi_call(napi_set_named_property(env, category, "heartbeats", entity));

    // return: Statistic
    return result;
}
