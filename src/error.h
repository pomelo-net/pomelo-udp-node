#ifndef POMELO_NODE_ERROR_SRC_H
#define POMELO_NODE_ERROR_SRC_H
#include <inttypes.h>


/* -------------------------------------------------------------------------- */
/*                         Common error messages                              */
/* -------------------------------------------------------------------------- */

#define POMELO_NODE_ERROR_INIT_POMELO "Failed to initialize pomelo"
#define POMELO_NODE_EMPTY_MESSAGE_ERROR "<empty error message>"
#define POMELO_NODE_ERROR_NOT_ENOUGH_ARGS "Not enough arguments"
#define POMELO_NODE_ERROR_CREATE_SOCKET "Failed to create socket"
#define POMELO_NODE_ERROR_TOKEN_BYTES                                          \
    "Connect token is invalid. Required bytes: %d but received %zu."
#define POMELO_NODE_ERROR_NATIVE_NULL "Native object is NULL"
#define POMELO_NODE_ERROR_INVALID_INSTANCE "Invalid instance type"
#define POMELO_NODE_ERROR_CONSTRUCTOR_CALL                                     \
    "The constructor cannot be called as normal function"
#define POMELO_NODE_ERROR_CONSTRUCTOR_INTERNAL                                 \
    "This constructor cannot be called in JS-side"
#define POMELO_NODE_ERROR_CREATE_MESSAGE "Failed to create message"
#define POMELO_NODE_ERROR_INVALID_ARG "Invalid argument `%s`"
#define POMELO_NODE_ERROR_KEY_BYTES                                            \
    "Private key is invalid. Required bytes: %d but received %zu"
#define POMELO_NODE_ERROR_PARSE_ADDRESS "Failed to parse address"
#define POMELO_NODE_ERROR_WRITE_MESSAGE "Failed to write data to message"
#define POMELO_NODE_ERROR_READ_MESSAGE "Failed to read data from message"
#define POMELO_NODE_ERROR_SOCKET_LISTEN "Failed to start listening"
#define POMELO_NODE_ERROR_SOCKET_STOP "Failed to stop socket"

#define POMELO_NODE_ERROR_ENCODE_TOKEN "Failed to encode connect token"
#define POMELO_NODE_ERROR_RESET_MESSAGE "Failed to reset message"
#define POMELO_NODE_ERROR_CANCELED "Canceled"

#define POMELO_NODE_ERROR_PARSE_CHANNELS "Invalid channel modes array"
#define POMELO_NODE_ERROR_CHANNELS_EMPTY "Channel modes array is empty"
#define POMELO_NODE_ERROR_SET_CHANNEL_MODE "Failed to set channel mode"
#define POMELO_NODE_ERROR_GET_CHANNELS "Failed to get channels"

#define POMELO_NODE_ERROR_MESSAGE_RELEASED "This message was released"

#define POMELO_NODE_ERROR_MSG_CAPACITY 128

#define POMELO_NODE_THROW_ERROR_TOKEN_BYTES(required, received)                \
do {                                                                           \
    char err_text[POMELO_NODE_ERROR_MSG_CAPACITY];                             \
    sprintf(err_text, POMELO_NODE_ERROR_TOKEN_BYTES, required, received);      \
    napi_throw_type_error(env, NULL, err_text);                                \
} while (0)


#define POMELO_NODE_THROW_ERROR_KEY_BYTES(required, received)                  \
do {                                                                           \
    char err_text[POMELO_NODE_ERROR_MSG_CAPACITY];                             \
    sprintf(err_text, POMELO_NODE_ERROR_KEY_BYTES, required, received);        \
    napi_throw_type_error(env, NULL, err_text);                                \
} while (0)


#define POMELO_NODE_THROW_ERROR_KEY_BYTES(required, received)                  \
do {                                                                           \
    char err_text[POMELO_NODE_ERROR_MSG_CAPACITY];                             \
    sprintf(err_text, POMELO_NODE_ERROR_KEY_BYTES, required, received);        \
    napi_throw_type_error(env, NULL, err_text);                                \
} while (0)


#define napi_throw_arg(arg)                                                    \
do {                                                                           \
    char err_text[POMELO_NODE_ERROR_MSG_CAPACITY];                             \
    sprintf(err_text, POMELO_NODE_ERROR_INVALID_ARG, arg);                     \
    napi_throw_type_error(env, NULL, err_text);                                \
} while (0)

#define napi_throw_msg(msg) napi_throw_error(env, NULL, msg)

#endif // POMELO_NODE_ERROR_SRC_H
