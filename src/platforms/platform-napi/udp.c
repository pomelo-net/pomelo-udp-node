#include <assert.h>
#include <string.h>
#include "platform-napi.h"
#include "udp.h"
#include "utils.h"


#define POMELO_PLATFORM_UDP_RECV_CALLBACK_ARGC 4
napi_value pomelo_platform_udp_recv_callback(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_PLATFORM_UDP_RECV_CALLBACK_ARGC;
    napi_value argv[POMELO_PLATFORM_UDP_RECV_CALLBACK_ARGC] = { NULL };

    // Get the callback info
    napi_call(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc != POMELO_PLATFORM_UDP_RECV_CALLBACK_ARGC) return NULL;

    // Get the socket object
    pomelo_platform_udp_info_t * socket_info = NULL;
    napi_call(napi_unwrap(env, argv[0], (void *) &socket_info));
    if (!socket_info) return NULL;
    if (!socket_info->alloc_callback || !socket_info->recv_callback) {
        return NULL;
    }

    // Get the message buffer
    void * data = NULL;
    size_t length = 0;
    napi_call(napi_get_buffer_info(env, argv[1], &data, &length));
    if (!data) return NULL;

    // Get the address
    char host[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
    napi_call(napi_get_value_string_utf8(
        env, argv[2], host, sizeof(host), NULL
    ));

    uint32_t port = 0;
    napi_call(napi_get_value_uint32(env, argv[3], &port));
    if (!port) return NULL;

    pomelo_address_t address;
    int ret = pomelo_address_from_string_ex(&address, host, (uint16_t) port);
    if (ret < 0) return NULL; // Failed to parse the address
    
    pomelo_platform_iovec_t iovec = { 0 };
    socket_info->alloc_callback(socket_info->context, &iovec);
    if (!iovec.data || iovec.length == 0) return NULL;

    int status = 0;
    if (length > iovec.length) {
        length = iovec.length;
        status = 2; // UV_UDP_PARTIAL
    }

    memcpy(iovec.data, data, length);
    iovec.length = length;

    socket_info->recv_callback(
        socket_info->context,
        &address,
        &iovec,
        status
    );

    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    return undefined;
}


#define POMELO_PLATFORM_UDP_SEND_CALLBACK_ARGC 3
napi_value pomelo_platform_udp_send_callback(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = POMELO_PLATFORM_UDP_SEND_CALLBACK_ARGC;
    napi_value argv[POMELO_PLATFORM_UDP_SEND_CALLBACK_ARGC] = { NULL };

    // Get the callback info
    napi_call(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc != POMELO_PLATFORM_UDP_SEND_CALLBACK_ARGC) return NULL;

    // Get function from external value
    napi_valuetype type = napi_undefined;
    napi_call(napi_typeof(env, argv[0], &type));
    if (type != napi_external) {
        napi_value undefined = NULL;
        napi_call(napi_get_undefined(env, &undefined));
        return undefined;
    }

    pomelo_platform_send_cb send_callback = NULL;
    napi_call(napi_get_value_external(env, argv[0], (void *) &send_callback));
    if (!send_callback) return NULL;
    
    // Get the callback data
    void * callback_data = NULL;
    napi_call(napi_get_value_external(env, argv[1], &callback_data));

    // Get the status number
    int send_status = 0;
    napi_call(napi_get_value_int32(env, argv[2], &send_status));
    if (send_status < 0) return NULL;
    
    // Call the callback
    send_callback(callback_data, send_status);

    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    return undefined;
}


/// @brief Finalize the UDP info
void pomelo_platform_udp_info_finalize(pomelo_platform_udp_info_t * info) {
    assert(info != NULL);

    // Delete the socket reference
    if (info->socket_ref) {
        napi_delete_reference(info->platform->env, info->socket_ref);
        info->socket_ref = NULL;
    }
}


/// @brief Finalize and release the UDP info
static void platform_udp_info_finalize_and_release(
    napi_env env,
    pomelo_platform_udp_info_t * info,
    pomelo_platform_napi_t * platform
) {
    (void) env;
    assert(info != NULL);
    assert(platform != NULL);

    // Finalize the UDP info
    pomelo_platform_udp_info_finalize(info);

    // Release the UDP info
    pomelo_pool_release(platform->udp_info_pool, info);
}


/// @brief Create a UDP socket
static napi_ref platform_udp_create(
    pomelo_platform_napi_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    assert(address != NULL);

    napi_env env = platform->env;

    // Create host and port
    napi_value host = NULL;
    napi_value port = NULL;

    char host_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
    int ret = pomelo_address_to_ip_string(address, host_str, sizeof(host_str));
    if (ret < 0) return NULL;
    
    napi_call(napi_create_string_utf8(
        env, host_str, NAPI_AUTO_LENGTH, &host
    ));

    napi_call(napi_create_uint32(env, pomelo_address_port(address), &port));

    // Get the recv callback function
    napi_value recv_callback = NULL;
    napi_call(napi_get_reference_value(
        env, platform->udp_recv_callback, &recv_callback
    ));

    // Get the send callback function
    napi_value send_callback = NULL;
    napi_call(napi_get_reference_value(
        env, platform->udp_send_callback, &send_callback
    ));

    // Get the create function
    napi_value udp_create = NULL;
    napi_call(napi_get_reference_value(
        env,
        platform->udp_create,
        &udp_create
    ));

    napi_value argv[] = { host, port, recv_callback, send_callback };
    napi_value result = NULL;

    napi_value null_value = NULL;
    napi_call(napi_get_null(env, &null_value));
    napi_call(napi_call_function(
        env,
        null_value,
        udp_create,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        &result
    ));

    // Acquire UDP info
    pomelo_platform_udp_info_t * info =
        pomelo_pool_acquire(platform->udp_info_pool, NULL);
    if (info == NULL) return NULL;

    // Wrap the UDP info
    napi_status status = napi_wrap(
        env,
        result,
        info,
        (napi_finalize) platform_udp_info_finalize_and_release,
        platform,
        &info->socket_ref
    );
    if (status != napi_ok) {
        pomelo_pool_release(platform->udp_info_pool, info);
        return NULL;
    }
    info->platform = platform;

    return info->socket_ref;
}


/// @brief Bind the UDP socket to specific address
static napi_ref platform_udp_bind(
    pomelo_platform_napi_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    assert(address != NULL);

    napi_env env = platform->env;

    napi_ref socket_ref = platform_udp_create(platform, address);
    if (socket_ref == NULL) return NULL;

    // Get socket object
    napi_value socket_object = NULL;
    napi_call(napi_get_reference_value(env, socket_ref, &socket_object));

    napi_value udp_bind = NULL;
    napi_call(napi_get_reference_value(
        env,
        platform->udp_bind,
        &udp_bind
    ));

    napi_value null_value = NULL;
    napi_call(napi_get_null(env, &null_value));

    napi_value argv[] = { socket_object };
    napi_value result = NULL;
    napi_call(napi_call_function(
        env,
        null_value,
        udp_bind,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        &result
    ));

    bool result_value = false;
    napi_call(napi_get_value_bool(env, result, &result_value));
    if (!result_value) return NULL; // Failed to bind

    // Increase the reference count
    napi_call(napi_reference_ref(env, socket_ref, NULL));
    return socket_ref;
}


pomelo_platform_udp_t * pomelo_platform_napi_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    assert(address != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return NULL;

    napi_ref result = platform_udp_bind(impl, address);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return NULL;

    return (pomelo_platform_udp_t *) result;
}


/// @brief Connect the socket to specific address
static napi_ref platform_udp_connect(
    pomelo_platform_napi_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    assert(address != NULL);

    napi_env env = platform->env;

    napi_ref socket_ref = platform_udp_create(platform, address);
    if (socket_ref == NULL) return NULL;

    // Get socket object
    napi_value socket_object = NULL;
    napi_call(napi_get_reference_value(env, socket_ref, &socket_object));

    napi_value udp_connect = NULL;
    napi_call(napi_get_reference_value(
        env,
        platform->udp_connect,
        &udp_connect
    ));

    napi_value null_value = NULL;
    napi_call(napi_get_null(env, &null_value));

    napi_value argv[] = { socket_object };
    napi_value result = NULL;
    napi_call(napi_call_function(
        env,
        null_value,
        udp_connect,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        &result
    ));

    bool result_value = false;
    napi_call(napi_get_value_bool(env, result, &result_value));
    if (!result_value) return NULL; // Failed to connect

    // Increase the reference count
    napi_call(napi_reference_ref(env, socket_ref, NULL));
    return socket_ref;
}


pomelo_platform_udp_t * pomelo_platform_napi_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    assert(address != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return NULL;

    napi_ref result = platform_udp_connect(impl, address);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return NULL;

    return (pomelo_platform_udp_t *) result;
}


/// @brief Stop the socket
static int platform_udp_stop(
    pomelo_platform_napi_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    assert(socket != NULL);

    napi_value udp_stop = NULL;
    napi_status status = napi_get_reference_value(
        platform->env,
        platform->udp_stop,
        &udp_stop
    );
    if (status != napi_ok) return -1;

    napi_value socket_object = NULL;
    status = napi_get_reference_value(
        platform->env,
        (napi_ref) socket,
        &socket_object
    );
    if (status != napi_ok) return -1;

    napi_value null_value = NULL;
    status = napi_get_null(platform->env, &null_value);
    if (status != napi_ok) return -1;

    napi_value argv[] = { socket_object };
    napi_value result = NULL;
    status = napi_call_function(
        platform->env,
        null_value,
        udp_stop,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        &result
    );
    if (status != napi_ok) return -1;

    bool result_value = false;
    status = napi_get_value_bool(platform->env, result, &result_value);
    if (status != napi_ok) return -1;

    if (result_value) {
        // Unref the reference to the socket
        napi_reference_unref(platform->env, (napi_ref) socket, NULL);
    }

    return result_value ? 0 : -1;
}


int pomelo_platform_napi_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    assert(socket != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return -1;

    int result = platform_udp_stop(impl, socket);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return -1;

    return result;
}


/// @brief Wrap the buffer
static napi_status wrap_buffer(
    napi_env env,
    pomelo_platform_iovec_t * iovec,
    napi_value * result
) {
    assert(env != NULL);
    assert(iovec != NULL);
    assert(result != NULL);

    napi_status status = napi_create_external_buffer(
        env,
        iovec->length,
        iovec->data,
        NULL,
        NULL,
        result
    );

    if (status == napi_ok) {
        return napi_ok;
    }

    if (status == napi_no_external_buffers_allowed) {
        // External buffer are not allowed in some runtime environments
        // so we need to copy the buffer to a new buffer
        void * data = NULL;
        status = napi_create_buffer(env, iovec->length, &data, result);
        if (status != napi_ok) return status;
        memcpy(data, iovec->data, iovec->length);
    }

    return status;
}


/// @brief Send a packet to target  
static int platform_udp_send(
    pomelo_platform_napi_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
) {
    assert(platform != NULL);
    assert(socket != NULL);
    assert(iovec != NULL);

    napi_env env = platform->env;

    napi_value udp_send = NULL;
    napi_status status =
        napi_get_reference_value(env, platform->udp_send, &udp_send);
    if (status != napi_ok) return -1;

    // Get the socket object
    napi_value socket_object = NULL;
    status = napi_get_reference_value(env, (napi_ref) socket, &socket_object);
    if (status != napi_ok) return -1;

    // Create the messages array
    napi_value messages = NULL;
    status = napi_create_array(env, &messages);
    if (status != napi_ok) return -1;

    for (int i = 0; i < niovec; i++) {
        napi_value buffer_object = NULL;
        status = wrap_buffer(env, &iovec[i], &buffer_object);
        if (status != napi_ok) return -1;
        status = napi_set_element(env, messages, i, buffer_object);
        if (status != napi_ok) return -1;
    }

    // Create the callback data
    napi_value callback_data_object = NULL;
    if (callback_data) {
        status = napi_create_external(
            env,
            callback_data,
            NULL,
            NULL,
            &callback_data_object
        );
        if (status != napi_ok) return -1;
    } else {
        status = napi_get_null(env, &callback_data_object);
        if (status != napi_ok) return -1;
    }

    // Create callback function object
    napi_value callback_func_object = NULL;
    if (send_callback) {
        status = napi_create_external(
            env,
            (void *) send_callback,
            NULL,
            NULL,
            &callback_func_object
        );
        if (status != napi_ok) return -1;
    } else {
        status = napi_get_null(env, &callback_func_object);
        if (status != napi_ok) return -1;
    }
    
    // Create the host and port
    napi_value host = NULL;
    napi_value port = NULL;

    if (address) {
        char host_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
        int ret = pomelo_address_to_ip_string(address, host_str, sizeof(host_str));
        if (ret < 0) return -1;
        
        status = napi_create_string_utf8(
            env,
            host_str,
            NAPI_AUTO_LENGTH,
            &host
        );
        if (status != napi_ok) return -1;

        status = napi_create_uint32(
            env,
            pomelo_address_port(address),
            &port
        );
        if (status != napi_ok) return -1;
    } else {
        status = napi_get_undefined(env, &host);
        if (status != napi_ok) return -1;

        status = napi_get_undefined(env, &port);
        if (status != napi_ok) return -1;
    }

    napi_value null_value = NULL;
    status = napi_get_null(env, &null_value);
    if (status != napi_ok) return -1;

    napi_value argv[] = {
        socket_object,
        messages,
        host,
        port,
        callback_data_object,
        callback_func_object
    }; 
    napi_value result = NULL;
    status = napi_call_function(
        env,
        null_value,
        udp_send,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        &result
    );
    if (status != napi_ok) return -1;

    bool result_value = false;
    status = napi_get_value_bool(env, result, &result_value);
    if (status != napi_ok) return -1;

    return result_value ? 0 : -1;
}


int pomelo_platform_napi_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int niovec,
    pomelo_platform_iovec_t * iovec,
    void * callback_data,
    pomelo_platform_send_cb send_callback
) {
    assert(platform != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;
    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return -1;

    int result = platform_udp_send(
        impl,
        socket,
        address,
        niovec,
        iovec,
        callback_data,
        send_callback
    );

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return -1;

    return result;
}


static void platform_udp_recv_start(
    pomelo_platform_napi_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
) {
    assert(platform != NULL);
    assert(socket != NULL);
    assert(context != NULL);
    assert(alloc_callback != NULL);
    assert(recv_callback != NULL);

    napi_value socket_object = NULL;
    napi_status status = napi_get_reference_value(
        platform->env,
        (napi_ref) socket,
        &socket_object
    );
    if (status != napi_ok) return;

    pomelo_platform_udp_info_t * info = NULL;
    status = napi_unwrap(
        platform->env,
        socket_object,
        (void *) &info
    );
    if (status != napi_ok) return;
    if (!info) return;

    info->context = context;
    info->alloc_callback = alloc_callback;
    info->recv_callback = recv_callback;
}


/// @brief Set the socket callback for platform
void pomelo_platform_napi_udp_recv_start(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
) {
    assert(platform != NULL);

    pomelo_platform_napi_t * impl = (pomelo_platform_napi_t *) platform;
    napi_env env = impl->env;
    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return;

    platform_udp_recv_start(
        impl,
        socket,
        context,
        alloc_callback,
        recv_callback
    );

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return;
}
