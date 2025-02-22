#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "module.h"
#include "token.h"
#include "error.h"
#include "utils.h"



/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

napi_status pomelo_node_init_token_module(
    napi_env env,
    pomelo_node_context_t * context,
    napi_value ns
) {
    // Create wrapper object
    napi_value token = NULL;
    napi_status status = napi_create_object(env, &token);
    if (status != napi_ok) return status;

    napi_value fn_encode;
    napi_calls(napi_create_function(
        env,
        "encode",
        NAPI_AUTO_LENGTH,
        pomelo_node_token_encode,
        context,
        &fn_encode
    ));
    napi_calls(napi_set_named_property(env, token, "encode", fn_encode));

    napi_value fn_random_buffer;
    napi_calls(napi_create_function(
        env,
        "randomBuffer",
        NAPI_AUTO_LENGTH,
        pomelo_node_token_random_buffer,
        context,
        &fn_random_buffer
    ));
    napi_calls(napi_set_named_property(
        env, token, "randomBuffer", fn_random_buffer
    ));

    // Bind constants
    napi_value value = NULL;
    napi_calls(napi_create_int32(env, POMELO_KEY_BYTES, &value));
    napi_calls(napi_set_named_property(env, token, "KEY_BYTES", value));
    napi_calls(napi_create_int32(env, POMELO_CONNECT_TOKEN_NONCE_BYTES, &value));
    napi_calls(napi_set_named_property(
        env, token, "CONNECT_TOKEN_NONCE_BYTES", value
    ));
    napi_calls(napi_create_int32(env, POMELO_USER_DATA_BYTES, &value));
    napi_calls(napi_set_named_property(env, token, "USER_DATA_BYTES", value));

    // Export to namespace
    napi_calls(napi_set_named_property(env, ns, "Token", token));
    
    return napi_ok;
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

#define POMELO_NODE_TOKEN_ENCODE_ARGC 11
napi_value pomelo_node_token_encode(napi_env env, napi_callback_info info) {
    size_t argc = POMELO_NODE_TOKEN_ENCODE_ARGC;
    napi_value argv[POMELO_NODE_TOKEN_ENCODE_ARGC];

    napi_status status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) return NULL; // Failed to get callback info

    if (argc < POMELO_NODE_TOKEN_ENCODE_ARGC) {
        napi_throw_error(env, NULL, POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    // The connect token info
    pomelo_connect_token_t token_info;
    memset(&token_info, 0, sizeof(pomelo_connect_token_t));

    // Get the private key
    bool is_typed_array = false;
    napi_call(napi_is_typedarray(env, argv[0], &is_typed_array));
    if (!is_typed_array) {
        napi_throw_arg("privateKey");
        return NULL;
    }

    napi_valuetype type;
    napi_typedarray_type arr_type;
    size_t length = 0;
    void * private_key = NULL;
    napi_call(napi_get_typedarray_info(
        env, argv[0], &arr_type, &length, &private_key, NULL, NULL
    ));
    if (arr_type != napi_uint8_array || length != POMELO_KEY_BYTES) {
        napi_throw_arg("privateKey");
        return NULL;
    }

    // Parse protocol ID
    int ret = pomelo_node_parse_uint64_value(
        env, argv[1], &token_info.protocol_id
    );
    if (ret < 0) {
        napi_throw_arg("protocolID");
        return NULL;
    }

    // Parse create timestamp
    ret = pomelo_node_parse_uint64_value(
        env, argv[2], &token_info.create_timestamp
    );
    if (ret < 0) {
        napi_throw_arg("createTimestamp");
        return NULL;
    }

    // Parse expire timestamp
    ret = pomelo_node_parse_uint64_value(
        env, argv[3], &token_info.expire_timestamp
    );
    if (ret < 0) {
        napi_throw_arg("expireTimestamp");
        return NULL;
    }

    // Parse connect token nonce
    napi_call(napi_is_typedarray(env, argv[4], &is_typed_array));
    if (!is_typed_array) {
        napi_throw_arg("connectTokenNonce");
        return NULL;
    }
    void * connect_token_nonce = NULL;
    napi_call(napi_get_typedarray_info(
        env, argv[4], &arr_type, &length, &connect_token_nonce, NULL, NULL
    ));
    if (
        arr_type != napi_uint8_array ||
        length != POMELO_CONNECT_TOKEN_NONCE_BYTES
    ) {
        napi_throw_arg(
            "connectTokenNonce"
        );
        return NULL;
    }
    memcpy(
        token_info.connect_token_nonce,
        connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );

    // Get timeout
    ret = napi_get_value_int32(env, argv[5], &token_info.timeout);
    if (ret != napi_ok) {
        napi_throw_arg("timeout");
        return NULL;
    }

    // Get addresses: Array of strings
    bool is_array = false;
    napi_call(napi_is_array(env, argv[6], &is_array));
    if (!is_array) {
        napi_throw_arg("addresses");
        return NULL;
    }

    uint32_t address_length = 0;
    napi_call(napi_get_array_length(env, argv[6], &address_length));
    if (address_length == 0) {
        // No address is provided
        napi_throw_arg("addresses");
        return NULL;
    }

    token_info.naddresses = (int) address_length;

    napi_value element;
    char address_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];

    for (uint32_t i = 0; i < address_length; i++) {
        napi_call(napi_get_element(env, argv[6], i, &element));
        napi_call(napi_typeof(env, element, &type));
        if (type != napi_string) {
            napi_throw_arg("addresses");
            return NULL;
        }

        memset(address_str, 0, sizeof(address_str));
        size_t length;
        napi_call(napi_get_value_string_utf8(
            env,
            element,
            address_str,
            POMELO_ADDRESS_STRING_BUFFER_CAPACITY,
            &length
        ));

        ret = pomelo_address_from_string(token_info.addresses + i, address_str);
        if (ret < 0) {
            napi_throw_arg("addresses");
            return NULL;
        }
    }

    // Get the client to server key
    napi_call(napi_is_typedarray(env, argv[7], &is_typed_array));
    if (!is_typed_array) {
        napi_throw_arg("clientToServerKey");
        return NULL;
    }
    void * client_to_server_key = NULL;
    napi_call(napi_get_typedarray_info(
        env, argv[7], &arr_type, &length, &client_to_server_key, NULL, NULL
    ));
    if (arr_type != napi_uint8_array || length != POMELO_KEY_BYTES) {
        napi_throw_arg("clientToServerKey");
        return NULL;
    }
    memcpy(
        token_info.client_to_server_key,
        client_to_server_key,
        POMELO_KEY_BYTES
    );

    // Get the server to client key
    void * server_to_client_key = NULL;
    napi_call(napi_is_typedarray(env, argv[8], &is_typed_array));
    if (!is_typed_array) {
        napi_throw_arg("serverToClientKey");
        return NULL;
    }
    napi_call(napi_get_typedarray_info(
        env, argv[8], &arr_type, &length, &server_to_client_key, NULL, NULL
    ));
    if (arr_type != napi_uint8_array || length != POMELO_KEY_BYTES) {
        napi_throw_arg("serverToClientKey");
        return NULL;
    }
    memcpy(
        token_info.server_to_client_key,
        server_to_client_key,
        POMELO_KEY_BYTES
    );

    // Get the client ID
    ret = pomelo_node_parse_int64_value(env, argv[9], &token_info.client_id);
    if (ret < 0) {
        napi_throw_arg("clientID");
        return NULL;
    }

    // Get the user data
    void * user_data = NULL;
    napi_call(napi_is_typedarray(env, argv[8], &is_typed_array));
    if (!is_typed_array) {
        napi_throw_arg("userData");
        return NULL;
    }
    napi_call(napi_get_typedarray_info(
        env, argv[10], &arr_type, &length, &user_data, NULL, NULL
    ));
    if (arr_type != napi_uint8_array || length != POMELO_USER_DATA_BYTES) {
        napi_throw_arg("userData");
        return NULL;
    }
    memcpy(token_info.user_data, user_data, POMELO_USER_DATA_BYTES);


    // Create new buffer for storing value
    napi_value array_buffer;
    void * buffer = NULL;
    napi_call(napi_create_arraybuffer(
        env, POMELO_CONNECT_TOKEN_BYTES, &buffer, &array_buffer
    ));

    // Encode the token
    ret = pomelo_connect_token_encode(buffer, &token_info, private_key);
    if (ret < 0) {
        napi_throw_error(env, NULL, POMELO_NODE_ERROR_ENCODE_TOKEN);
        return NULL;
    }

    // Cast to typed array
    napi_value uint8_array_result;
    napi_call(napi_create_typedarray(
        env,
        napi_uint8_array,
        POMELO_CONNECT_TOKEN_BYTES,
        array_buffer,
        /* offset =*/ 0,
        &uint8_array_result
    ));

    return uint8_array_result;
}


#define POMELO_NODE_TOKEN_RANDOM_BUFFER_ARGC 1
napi_value pomelo_node_token_random_buffer(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_NODE_TOKEN_RANDOM_BUFFER_ARGC;
    napi_value argv[POMELO_NODE_TOKEN_RANDOM_BUFFER_ARGC];

    napi_status status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) return NULL; // Failed to get callback info

    if (argc < POMELO_NODE_TOKEN_RANDOM_BUFFER_ARGC) {
        napi_throw_error(env, NULL, POMELO_NODE_ERROR_NOT_ENOUGH_ARGS);
        return NULL;
    }

    size_t length = 0;
    if (pomelo_node_parse_size_value(env, argv[0], &length) < 0) {
        napi_throw_arg("length");
        return NULL;
    }

    if (length == 0) {
        napi_throw_msg("length must be positive");
        return NULL;
    }

    void * buffer = NULL;
    napi_value arraybuffer = NULL;
    napi_call(napi_create_arraybuffer(env, length, &buffer, &arraybuffer));

    int ret = pomelo_codec_buffer_random(buffer, length);
    if (ret < 0) {
        napi_throw_msg("Failed to random buffer");
        return NULL;
    }

    // Convert to Uint8Array
    napi_value result = NULL;
    napi_call(napi_create_typedarray(
        env, napi_uint8_array, length, arraybuffer, 0, &result
    ));
    return result;
}
