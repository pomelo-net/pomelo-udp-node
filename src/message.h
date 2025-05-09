#ifndef POMELO_NODE_MESSAGE_SRC_H
#define POMELO_NODE_MESSAGE_SRC_H
#include "module.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_node_message_s {
    /// @brief The context
    pomelo_node_context_t * context;

    /// @brief The message
    pomelo_message_t * message;

    /// @brief The reference of this object (message)
    napi_ref thiz;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the message module
napi_status pomelo_node_init_message_module(napi_env env, napi_value ns);


/// @brief Acquire new message from pool and attach the native message
napi_value pomelo_node_message_new(napi_env env, pomelo_message_t * message);


/// @brief Initialize the message
int pomelo_node_message_init(
    pomelo_node_message_t * node_message,
    pomelo_node_context_t * context
);


/// @brief Detach the native message and release the message to pool
void pomelo_node_message_cleanup(pomelo_node_message_t * node_message);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Message.constructor()
napi_value pomelo_node_message_constructor(
    napi_env env,
    napi_callback_info info
);


/// @brief Finalizer of message
void pomelo_node_message_finalizer(
    napi_env env,
    pomelo_node_message_t * node_message,
    pomelo_node_context_t * context
);


/// @brief Message.reset()
napi_value pomelo_node_message_reset(napi_env env, napi_callback_info info);


/// @brief Message.size()
napi_value pomelo_node_message_size(napi_env env, napi_callback_info info);


/// @brief Message.read()
napi_value pomelo_node_message_read(napi_env env, napi_callback_info info);


/// @brief Message.readUint8()
napi_value pomelo_node_message_read_uint8(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readUint16()
napi_value pomelo_node_message_read_uint16(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readUint32()
napi_value pomelo_node_message_read_uint32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readUint64()
napi_value pomelo_node_message_read_uint64(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readInt8()
napi_value pomelo_node_message_read_int8(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readInt16()
napi_value pomelo_node_message_read_int16(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readInt32()
napi_value pomelo_node_message_read_int32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readInt64()
napi_value pomelo_node_message_read_int64(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readFloat32()
napi_value pomelo_node_message_read_float32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.readFloat64()
napi_value pomelo_node_message_read_float64(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.write()
napi_value pomelo_node_message_write(napi_env env, napi_callback_info info);


/// @brief Message.writeUint8()
napi_value pomelo_node_message_write_uint8(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeUint16()
napi_value pomelo_node_message_write_uint16(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeUint32()
napi_value pomelo_node_message_write_uint32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeUint64()
napi_value pomelo_node_message_write_uint64(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeInt8()
napi_value pomelo_node_message_write_int8(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeInt16()
napi_value pomelo_node_message_write_int16(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeInt32()
napi_value pomelo_node_message_write_int32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeInt64()
napi_value pomelo_node_message_write_int64(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeFloat32()
napi_value pomelo_node_message_write_float32(
    napi_env env,
    napi_callback_info info
);


/// @brief Message.writeFloat64()
napi_value pomelo_node_message_write_float64(
    napi_env env,
    napi_callback_info info
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_MESSAGE_SRC_H
