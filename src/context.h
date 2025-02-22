#ifndef POMELO_NODE_CONTEXT_SRC_H
#define POMELO_NODE_CONTEXT_SRC_H
#include "module.h"
#include "uv.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct pomelo_node_context_options_s pomelo_node_context_options_t;


struct pomelo_node_context_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Node evironment
    napi_env env;

    /// @brief Maximum number of JS messages in pool
    size_t pool_message_max;

    /// @brief Maximum number of sessions in pool
    size_t pool_session_max;

    /// @brief Maximum number of sessions in pool
    size_t pool_channel_max;

    /// @brief Error handler
    napi_value error_handler;
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

    /// @brief Pool of sessions
    pomelo_list_t * pool_message;

    /// @brief Maximum number of messages in pool
    size_t pool_message_max;

    /// @brief Pool of sessions
    pomelo_list_t * pool_session;

    /// @brief Maximum number of sessions in pool
    size_t pool_session_max;

    /// @brief Pool of channels
    pomelo_list_t * pool_channel;

    /// @brief Maximum number of sessions in pool
    size_t pool_channel_max;

    /// @brief Error handler
    napi_ref error_handler;
};


/// @brief Initialize node context options
void pomelo_node_context_options_init(pomelo_node_context_options_t * options);


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


/// @brief Ref the platform
void pomelo_node_context_ref_platform(pomelo_node_context_t * context);


/// @brief Unref the platform
void pomelo_node_context_unref_platform(pomelo_node_context_t * context);


/// @brief Create node socket
pomelo_node_socket_t * pomelo_node_context_create_node_socket(
    pomelo_node_context_t * context
);


/// @brief Destroy node socket
void pomelo_node_context_destroy_node_socket(
    pomelo_node_context_t * context,
    pomelo_node_socket_t * node_socket
);


/// @brief Create node session
pomelo_node_session_t * pomelo_node_context_create_node_session(
    pomelo_node_context_t * context
);


/// @brief Destroy node session
void pomelo_node_context_destroy_node_session(
    pomelo_node_context_t * context,
    pomelo_node_session_t * node_session
);


/// @brief Create new node message
pomelo_node_message_t * pomelo_node_context_create_node_message(
    pomelo_node_context_t * context
);


/// @brief Destroy node message
void pomelo_node_context_destroy_node_message(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
);


/// @brief Create new node channel
pomelo_node_channel_t * pomelo_node_context_create_node_channel(
    pomelo_node_context_t * context
);


/// @brief Destroy node channel
void pomelo_node_context_destroy_node_channel(
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


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_CONTEXT_SRC_H
