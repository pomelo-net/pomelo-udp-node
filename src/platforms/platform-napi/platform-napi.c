#include <assert.h>
#include <string.h>
#include "platform-napi.h"
#include "udp.h"
#include "time.h"
#include "timer.h"
#include "worker.h"
#include "threadsafe.h"
#include "error.h"
#include "utils.h"


/// @brief Parse the platform options
static bool parse_platform_options(
    napi_env env,
    napi_value options,
    const char * name,
    napi_ref * ref
) {
    napi_status status;
    napi_value property;
    status = napi_get_named_property(env, options, name, &property);
    if (status != napi_ok) return false;

    status = napi_create_reference(env, property, 1, ref);
    if (status != napi_ok) return false;

    return true;
}


/// @brief Create the platform NAPI interface
static pomelo_platform_napi_t * platform_create(
    pomelo_allocator_t * allocator,
    napi_env env,
    napi_value options
) {
    assert(allocator != NULL);
    assert(env != NULL);
    assert(options != NULL);

    pomelo_platform_napi_t * platform =
        pomelo_allocator_malloc(allocator, sizeof(pomelo_platform_napi_t));
    assert(platform != NULL);
    memset(platform, 0, sizeof(pomelo_platform_napi_t));

    platform->env = env;
    platform->allocator = allocator;

    if (!parse_platform_options(env, options, "hrtime", &platform->hrtime)) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(env, options, "now", &platform->now)) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(env, options, "udpBind", &platform->udp_bind)) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "udpCreate", &platform->udp_create
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "udpConnect", &platform->udp_connect
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "udpStop", &platform->udp_stop
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "udpSend", &platform->udp_send
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "timerCreate", &platform->timer_create
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "timerStart", &platform->timer_start
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "timerStop", &platform->timer_stop
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    if (!parse_platform_options(
        env, options, "statistic", &platform->statistic
    )) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create the UDP info pool
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_udp_info_t);
    pool_options.zero_init = true;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_platform_udp_info_finalize;
    platform->udp_info_pool = pomelo_pool_root_create(&pool_options);
    if (platform->udp_info_pool == NULL) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create the timer info pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_timer_info_t);
    pool_options.zero_init = true;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_platform_timer_info_finalize;
    platform->timer_info_pool = pomelo_pool_root_create(&pool_options);
    if (platform->timer_info_pool == NULL) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create the async work info pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_worker_t);
    pool_options.zero_init = true;
    platform->task_worker_pool = pomelo_pool_root_create(&pool_options);
    if (platform->task_worker_pool == NULL) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create the threadsafe info pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_threadsafe_t);
    pool_options.zero_init = true;
    pool_options.synchronized = true;
    platform->task_threadsafe_pool = pomelo_pool_root_create(&pool_options);
    if (platform->task_threadsafe_pool == NULL) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }
    
    // Create the threadsafe executor pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_threadsafe_executor_t);
    pool_options.zero_init = true;
    platform->threadsafe_executor_pool = pomelo_pool_root_create(&pool_options);
    if (platform->threadsafe_executor_pool == NULL) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create recv callbacks for sockets
    napi_value recv_callback = NULL;
    napi_status status = napi_create_function(
        env,
        "recvCallback",
        NAPI_AUTO_LENGTH,
        pomelo_platform_udp_recv_callback,
        NULL,
        &recv_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create reference to the recv callback
    status = napi_create_reference(
        env, recv_callback, 1, &platform->udp_recv_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create send callbacks for sockets
    napi_value send_callback = NULL;
    status = napi_create_function(
        env,
        "sendCallback",
        NAPI_AUTO_LENGTH,
        pomelo_platform_udp_send_callback,
        NULL,
        &send_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create reference to the send callback
    status = napi_create_reference(
        env, send_callback, 1, &platform->udp_send_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create the timer callback
    napi_value timer_callback = NULL;
    status = napi_create_function(
        env,
        "timerCallback",
        NAPI_AUTO_LENGTH,
        pomelo_platform_timer_callback,
        NULL,
        &timer_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    // Create reference to the timer callback
    status = napi_create_reference(
        env, timer_callback, 1, &platform->timer_callback
    );
    if (status != napi_ok) {
        pomelo_platform_napi_destroy(&platform->i);
        return NULL;
    }

    return platform;
}

pomelo_platform_interface_t * pomelo_platform_napi_create(
    pomelo_allocator_t * allocator,
    napi_env env,
    napi_value options
) {
    assert(env != NULL);

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return NULL;

    pomelo_platform_napi_t * platform =
        platform_create(allocator, env, options);

    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) {
        if (platform != NULL) {
            pomelo_platform_napi_destroy(&platform->i);
        }
        return NULL;
    }

    if (platform == NULL) return NULL;

    pomelo_platform_interface_t * i = (pomelo_platform_interface_t *) platform;
    // Setup the interface
    i->allocator = allocator;
    i->internal = platform;
    i->destroy = pomelo_platform_napi_destroy;
    i->startup = pomelo_platform_napi_startup;
    i->shutdown = pomelo_platform_napi_shutdown;
    i->hrtime = pomelo_platform_napi_hrtime;
    i->now = pomelo_platform_napi_now;
    i->acquire_threadsafe_executor =
        pomelo_platform_napi_acquire_threadsafe_executor;
    i->release_threadsafe_executor =
        pomelo_platform_napi_release_threadsafe_executor;
    i->threadsafe_executor_submit =
        pomelo_platform_napi_threadsafe_executor_submit;
    i->submit_worker_task = pomelo_platform_napi_submit_worker_task;
    i->cancel_worker_task = pomelo_platform_napi_cancel_worker_task;
    i->udp_bind = pomelo_platform_napi_udp_bind;
    i->udp_connect = pomelo_platform_napi_udp_connect;
    i->udp_stop = pomelo_platform_napi_udp_stop;
    i->udp_send = pomelo_platform_napi_udp_send;
    i->udp_recv_start = pomelo_platform_napi_udp_recv_start;
    i->timer_start = pomelo_platform_napi_timer_start;
    i->timer_stop = pomelo_platform_napi_timer_stop;
    i->statistic = pomelo_platform_napi_statistic;

    return i;
}


static void platform_destroy(pomelo_platform_napi_t * platform) {
    assert(platform != NULL);

    // Destroy the UDP info pool
    if (platform->udp_info_pool != NULL) {
        pomelo_pool_destroy(platform->udp_info_pool);
        platform->udp_info_pool = NULL;
    }

    // Destroy the timer info pool
    if (platform->timer_info_pool != NULL) {
        pomelo_pool_destroy(platform->timer_info_pool);
        platform->timer_info_pool = NULL;
    }

    // Destroy the async work info pool
    if (platform->task_worker_pool != NULL) {
        pomelo_pool_destroy(platform->task_worker_pool);
        platform->task_worker_pool = NULL;
    }

    // Destroy the threadsafe info pool
    if (platform->task_threadsafe_pool != NULL) {
        pomelo_pool_destroy(platform->task_threadsafe_pool);
        platform->task_threadsafe_pool = NULL;
    }

    if (platform->hrtime != NULL) {
        napi_delete_reference(platform->env, platform->hrtime);
        platform->hrtime = NULL;
    }

    if (platform->now != NULL) {
        napi_delete_reference(platform->env, platform->now);
        platform->now = NULL;
    }

    if (platform->udp_bind != NULL) {
        napi_delete_reference(platform->env, platform->udp_bind);
        platform->udp_bind = NULL;
    }

    if (platform->udp_connect != NULL) {
        napi_delete_reference(platform->env, platform->udp_connect);
        platform->udp_connect = NULL;
    }

    if (platform->udp_stop != NULL) {
        napi_delete_reference(platform->env, platform->udp_stop);
        platform->udp_stop = NULL;
    }

    if (platform->udp_send != NULL) {
        napi_delete_reference(platform->env, platform->udp_send);
        platform->udp_send = NULL;
    }

    if (platform->udp_recv_callback != NULL) {
        napi_delete_reference(platform->env, platform->udp_recv_callback);
        platform->udp_recv_callback = NULL;
    }

    if (platform->udp_send_callback != NULL) {
        napi_delete_reference(platform->env, platform->udp_send_callback);
        platform->udp_send_callback = NULL;
    }

    if (platform->timer_create != NULL) {
        napi_delete_reference(platform->env, platform->timer_create);
        platform->timer_create = NULL;
    }

    if (platform->timer_start != NULL) {
        napi_delete_reference(platform->env, platform->timer_start);
        platform->timer_start = NULL;
    }

    if (platform->timer_stop != NULL) {
        napi_delete_reference(platform->env, platform->timer_stop);
        platform->timer_stop = NULL;
    }

    if (platform->timer_callback != NULL) {
        napi_delete_reference(platform->env, platform->timer_callback);
        platform->timer_callback = NULL;
    }

    if (platform->statistic != NULL) {
        napi_delete_reference(platform->env, platform->statistic);
        platform->statistic = NULL;
    }

    pomelo_allocator_free(platform->allocator, platform);
}


void pomelo_platform_napi_destroy(pomelo_platform_interface_t * i) {
    assert(i != NULL);
    pomelo_platform_napi_t * platform = (pomelo_platform_napi_t *) i;
    napi_env env = platform->env;

    // Open the handle scope
    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return;

    // Destroy the platform
    platform_destroy(platform);

    // Close the handle scope
    napi_close_handle_scope(env, scope);
}


void pomelo_platform_napi_startup(pomelo_platform_interface_t * i) {
    (void) i;
}


void pomelo_platform_napi_shutdown(
    pomelo_platform_interface_t * i,
    pomelo_platform_shutdown_callback callback
) {
    if (callback != NULL) {
        callback((pomelo_platform_t *) i);
    }
}


napi_value pomelo_platform_napi_statistic(
    pomelo_platform_interface_t * i,
    napi_env env
) {
    pomelo_platform_napi_t * platform = (pomelo_platform_napi_t *) i;
    napi_value fn_statistic = NULL;
    napi_call(napi_get_reference_value(
        platform->env, platform->statistic, &fn_statistic
    ));

    napi_value result = NULL;
    napi_value thiz = NULL;
    napi_call(napi_get_null(env, &thiz));
    napi_call(napi_call_function(
        env, thiz, fn_statistic, 0, NULL, &result
    ));

    return result;
}



#define INIT_PLATFORM_NAPI_ARGC 1
napi_value pomelo_node_platform_napi_init(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = INIT_PLATFORM_NAPI_ARGC;
    napi_value argv[INIT_PLATFORM_NAPI_ARGC] = { NULL };

    // Get the allocator
    napi_call(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc != INIT_PLATFORM_NAPI_ARGC) return NULL;

    pomelo_allocator_t * allocator = pomelo_allocator_default();
    pomelo_platform_interface_t * i =
        pomelo_platform_napi_create(allocator, env, argv[0]);
    if (!i) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        return NULL;
    }

    napi_value result = NULL;
    napi_status status = napi_create_external(env, i, NULL, NULL, &result);
    if (status != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_CREATE_PLATFORM);
        pomelo_platform_napi_destroy(i);
        return NULL;
    }

    return result;
}


napi_value pomelo_node_platform_napi_entry(napi_env env, napi_value exports) {
    (void) exports;
    napi_value value = NULL;
    napi_status ret = napi_create_function(
        env,
        "initPlatformNAPI",
        NAPI_AUTO_LENGTH,
        pomelo_node_platform_napi_init,
        NULL,
        &value
    );

    if (ret != napi_ok) {
        napi_throw_msg(POMELO_NODE_ERROR_INIT_PLATFORM_UV);
        return NULL;
    }

    return value;
}
NAPI_MODULE(pomelo_node_platform_napi, pomelo_node_platform_napi_entry);
