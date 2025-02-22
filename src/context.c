#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "pomelo/platforms/platform-uv.h"
#include "context.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "channel.h"
#include "utils.h"


void pomelo_node_context_options_init(pomelo_node_context_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_node_context_options_t));
}


pomelo_node_context_t * pomelo_node_context_create(
    pomelo_node_context_options_t * options
) {
    assert(options != NULL);
    if (!options->allocator || !options->env) {
        return NULL;
    }

    uv_loop_t * uv_loop = NULL;
    napi_status status = napi_get_uv_event_loop(options->env, &uv_loop);
    if (status != napi_ok || !uv_loop) {
        return NULL;
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
    context->pool_message_max = options->pool_message_max;
    context->pool_session_max = options->pool_session_max;

    // Create native context
    pomelo_context_root_options_t context_options;
    pomelo_context_root_options_init(&context_options);
    context_options.allocator = allocator;

    context->context = pomelo_context_root_create(&context_options);
    if (!context) {
        // Failed to create native context
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create platform
    pomelo_platform_uv_options_t platform_options;
    pomelo_platform_uv_options_init(&platform_options);
    platform_options.allocator = allocator;
    platform_options.uv_loop = uv_loop;
    context->platform = pomelo_platform_uv_create(&platform_options);
    if (!context->platform) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create pool of messages
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_node_message_t *);
    context->pool_message = pomelo_list_create(&list_options);
    if (!context->pool_message) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create pool of sessions
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_node_session_t *);
    context->pool_session = pomelo_list_create(&list_options);
    if (!context->pool_session) {
        pomelo_node_context_destroy(context);
        return NULL;
    }

    // Create pool of channels
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_node_channel_t *);
    context->pool_channel = pomelo_list_create(&list_options);
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

    return context;
}


void pomelo_node_context_destroy(pomelo_node_context_t * context) {
    assert(context != NULL);

    if (context->context) {
        pomelo_context_destroy(context->context);
        context->context = NULL;
    }

    if (context->platform) {
        pomelo_platform_shutdown(context->platform);
        pomelo_platform_uv_destroy(context->platform);
        context->platform = NULL;
    }

    napi_env env = context->env;
    pomelo_list_t * pool_message = context->pool_message;
    if (pool_message) {
        pomelo_node_message_t * node_message = NULL;
        pomelo_list_for(pool_message, node_message, pomelo_node_message_t *, {
            napi_reference_unref(env, node_message->thiz, NULL);
        });

        pomelo_list_destroy(pool_message);
        context->pool_message = NULL;
    }

    pomelo_list_t * pool_session = context->pool_session;
    if (pool_session) {
        pomelo_node_session_t * node_session = NULL;
        pomelo_list_for(pool_session, node_session, pomelo_node_session_t *, {
            napi_reference_unref(env, node_session->thiz, NULL);
        });

        pomelo_list_destroy(pool_session);
        context->pool_session = NULL;
    }

    pomelo_list_t * pool_channel = context->pool_channel;
    if (pool_channel) {
        pomelo_node_channel_t * node_channel = NULL;
        pomelo_list_for(pool_channel, node_channel, pomelo_node_channel_t *, {
            napi_reference_unref(env, node_channel->thiz, NULL);
        });

        pomelo_list_destroy(pool_channel);
        context->pool_channel = NULL;
    }

    if (context->error_handler) {
        napi_callv(napi_delete_reference(context->env, context->error_handler));
        context->error_handler = NULL;
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


void pomelo_node_context_ref_platform(pomelo_node_context_t * context) {
    if ((context->platform_refcount++) == 0) {
        pomelo_platform_startup(context->platform);
    }
}


void pomelo_node_context_unref_platform(pomelo_node_context_t * context) {
    if ((--context->platform_refcount) == 0) {
        pomelo_platform_shutdown(context->platform);
    }
}


pomelo_node_socket_t * pomelo_node_context_create_node_socket(
    pomelo_node_context_t * context
) {
    pomelo_node_socket_t * node_socket =
        pomelo_allocator_malloc_t(context->allocator, pomelo_node_socket_t);
    if (!node_socket) return NULL;
    memset(node_socket, 0, sizeof(pomelo_node_socket_t));
    return node_socket;
}


void pomelo_node_context_destroy_node_socket(
    pomelo_node_context_t * context,
    pomelo_node_socket_t * node_socket
) {
    if (node_socket->thiz) {
        pomelo_node_delete_wrapped_reference(context->env, node_socket->thiz);
        node_socket->thiz = NULL;
    }

    pomelo_allocator_free(context->allocator, node_socket);
}


pomelo_node_session_t * pomelo_node_context_create_node_session(
    pomelo_node_context_t * context
) {
    pomelo_allocator_t * allocator = context->allocator;

    pomelo_node_session_t * node_session =
        pomelo_allocator_malloc_t(allocator, pomelo_node_session_t);
    if (!node_session) return NULL;
    memset(node_session, 0, sizeof(pomelo_node_session_t));

    // Create list of channels
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_node_channel_t *);
    node_session->list_channels = pomelo_list_create(&list_options);
    if (!node_session->list_channels) {
        pomelo_allocator_free(allocator, node_session);
        return NULL;
    }

    return node_session;
}


void pomelo_node_context_destroy_node_session(
    pomelo_node_context_t * context,
    pomelo_node_session_t * node_session
) {
    if (node_session->thiz) {
        pomelo_node_delete_wrapped_reference(context->env, node_session->thiz);
        node_session->thiz = NULL;
    }

    if (node_session->pool_node) {
        pomelo_list_remove(context->pool_session, node_session->pool_node);
        node_session->pool_node = NULL;
    }

    if (node_session->list_channels) {
        pomelo_list_destroy(node_session->list_channels);
        node_session->list_channels = NULL;
    }

    pomelo_allocator_free(context->allocator, node_session);
}


pomelo_node_message_t * pomelo_node_context_create_node_message(
    pomelo_node_context_t * context
) {
    pomelo_node_message_t * node_message =
        pomelo_allocator_malloc_t(context->allocator, pomelo_node_message_t);
    if (!node_message) return NULL;
    memset(node_message, 0, sizeof(pomelo_node_message_t));
    return node_message;
}


void pomelo_node_context_destroy_node_message(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
) {
    if (node_message->thiz) {
        pomelo_node_delete_wrapped_reference(context->env, node_message->thiz);
        node_message->thiz = NULL;
    }

    if (node_message->pool_node) {
        pomelo_list_remove(context->pool_message, node_message->pool_node);
        node_message->pool_node = NULL;
    }

    pomelo_allocator_free(context->allocator, node_message);
}


pomelo_node_channel_t * pomelo_node_context_create_node_channel(
    pomelo_node_context_t * context
) {
    pomelo_node_channel_t * node_channel =
        pomelo_allocator_malloc_t(context->allocator, pomelo_node_channel_t);
    if (!node_channel) return NULL;
    memset(node_channel, 0, sizeof(pomelo_node_channel_t));
    return node_channel;
}


/// @brief Destroy node channel
void pomelo_node_context_destroy_node_channel(
    pomelo_node_context_t * context,
    pomelo_node_channel_t * node_channel
) {
    if (node_channel->thiz) {
        pomelo_node_delete_wrapped_reference(context->env, node_channel->thiz);
        node_channel->thiz = NULL;
    }

    if (node_channel->pool_node) {
        pomelo_list_remove(context->pool_message, node_channel->pool_node);
        node_channel->pool_node = NULL;
    }

    pomelo_allocator_free(context->allocator, node_channel);
}


void pomelo_node_context_handle_error(
    pomelo_node_context_t * context,
    napi_value error
) {
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
    napi_env env = context->env;
    napi_value error_str = NULL;
    char error_buf[POMELO_NODE_ERROR_STRING_BUFFER];
    napi_callv(napi_coerce_to_string(env, error, &error_str));
    napi_callv(napi_get_value_string_utf8(
        env, error_str, error_buf, POMELO_NODE_ERROR_STRING_BUFFER, NULL
    ));

    fprintf(stderr, "Unhandled error: %s\n", error_buf);
}
