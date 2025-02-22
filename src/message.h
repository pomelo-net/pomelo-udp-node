#ifndef POMELO_NODE_MESSAGE_SRC_H
#define POMELO_NODE_MESSAGE_SRC_H
#include "module.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_message_s {
    /// @brief The message
    pomelo_message_t * message;

    /// @brief The reference of this object (message)
    napi_ref thiz;

    /// @brief Node of mesasge in pool
    pomelo_list_node_t * pool_node;

    /// @brief Valid flag of message
    bool valid;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the message module
napi_status pomelo_node_init_message_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
);

/// @brief Acquire new message from pool and attach the native message
napi_value pomelo_node_message_new(
    napi_env env,
    pomelo_node_context_t * context,
    pomelo_message_t * message
);

/// @brief Detach the native message and release the message to pool
void pomelo_node_message_delete(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value js_message
);

/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Message.constructor()
napi_value pomelo_node_message_constructor(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalize function of message
void pomelo_node_message_finalize(
    napi_env env,
    pomelo_node_message_t * node_message,
    pomelo_node_context_t * context
);


napi_value pomelo_node_message_reset(napi_env env, napi_callback_info info);

napi_value pomelo_node_message_size(napi_env env, napi_callback_info info);

napi_value pomelo_node_message_read(napi_env env, napi_callback_info info);

napi_value pomelo_node_message_read_uint8(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_uint16(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_uint32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_uint64(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_int8(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_int16(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_int32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_int64(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_float32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_read_float64(
    napi_env env,
    napi_callback_info info
);

pomelo_message_t * pomelo_node_message_prepare_write(
    pomelo_node_context_t * context,
    pomelo_node_message_t * node_message
);

napi_value pomelo_node_message_write(napi_env env, napi_callback_info info);

napi_value pomelo_node_message_write_uint8(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_uint16(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_uint32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_uint64(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_int8(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_int16(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_int32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_int64(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_float32(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_write_float64(
    napi_env env,
    napi_callback_info info
);

napi_value pomelo_node_message_acquire(napi_env env, napi_callback_info info);

napi_value pomelo_node_message_acquire_ex(
    napi_env env,
    pomelo_node_context_t * context
);

napi_value pomelo_node_message_release(napi_env env, napi_callback_info info);

void pomelo_node_message_release_ex(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value js_message
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_MESSAGE_SRC_H
