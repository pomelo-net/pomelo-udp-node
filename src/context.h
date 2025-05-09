#ifndef POMELO_NODE_CONTEXT_SRC_H
#define POMELO_NODE_CONTEXT_SRC_H
#include "module.h"
#include "utils/pool.h"
#include "utils/array.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Context options
typedef struct pomelo_node_context_options_s pomelo_node_context_options_t;


struct pomelo_node_context_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Node evironment
    napi_env env;

    /// @brief Maximum number of sockets in pool
    size_t pool_socket_max;

    /// @brief Maximum number of JS messages in pool
    size_t pool_message_max;

    /// @brief Maximum number of sessions in pool
    size_t pool_session_max;

    /// @brief Maximum number of channels in pool
    size_t pool_channel_max;

    /// @brief Error handler
    napi_value error_handler;

    /// @brief Platform
    napi_value platform;
};


struct pomelo_node_context_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Node evironment
    napi_env env;

    /// @brief Native context
    pomelo_context_t * context;

    /// @brief Platform
    pomelo_platform_t * platform;

    /// @brief Ref counter for platform
    int platform_refcount;

    /// @brief Class socket
    napi_ref class_socket;

    /// @brief Class session
    napi_ref class_session;

    /// @brief Class message
    napi_ref class_message;

    /// @brief Class channel
    napi_ref class_channel;

    /// @brief Pool of sockets
    pomelo_pool_t * pool_socket;

    /// @brief Pool of messages
    pomelo_pool_t * pool_message;

    /// @brief Pool of sessions
    pomelo_pool_t * pool_session;

    /// @brief Pool of channels
    pomelo_pool_t * pool_channel;

    /// @brief Error handler
    napi_ref error_handler;

    /// @brief Temporary sessions for sending
    pomelo_array_t * tmp_send_sessions;
};


/// @brief Create context
pomelo_node_context_t * pomelo_node_context_create(
    pomelo_node_context_options_t * options
);


/// @brief Destroy a context
void pomelo_node_context_destroy(pomelo_node_context_t * context);


/// @brief Finalize callback for context
void pomelo_node_context_finalizer(
    napi_env env,
    pomelo_node_context_t * context,
    void * finalize_hint
);


/// @brief Acquire new node socket
pomelo_node_socket_t * pomelo_node_context_acquire_socket(
    pomelo_node_context_t * context
);


/// @brief Release a node socket
void pomelo_node_context_release_socket(
    pomelo_node_context_t * context,
    pomelo_node_socket_t * node_socket
);


/// @brief Acquire a node session
pomelo_node_session_t * pomelo_node_context_acquire_session(
    pomelo_node_context_t * context
);


/// @brief Release a node session
void pomelo_node_context_release_session(
    pomelo_node_context_t * context,
    pomelo_node_session_t * node_session
);


/// @brief Acquire a node message
pomelo_node_message_t * pomelo_node_context_acquire_message(
    pomelo_node_context_t * context
);


/// @brief Release a node message
void pomelo_node_context_release_message(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
);


/// @brief Acquire a node channel
pomelo_node_channel_t * pomelo_node_context_acquire_channel(
    pomelo_node_context_t * context
);


/// @brief Destroy node channel
void pomelo_node_context_release_channel(
    pomelo_node_context_t * context,
    pomelo_node_channel_t * node_channel
);


/// @brief Handle error
void pomelo_node_context_handle_error(
    pomelo_node_context_t * context,
    napi_value error
);


/// @brief Default error handler
void pomelo_node_context_handle_error_default(
    pomelo_node_context_t * context,
    napi_value error
);


/// @brief statistic()
napi_value pomelo_node_context_statistic(napi_env env, napi_callback_info info);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_CONTEXT_SRC_H
