#include <assert.h>
#include "platform-napi.h"
#include "timer.h"
#include "utils.h"


#define TIMER_CALLBACK_ARGC 1
napi_value pomelo_platform_timer_callback(
    napi_env env,
    napi_callback_info info
) {
    size_t argc = TIMER_CALLBACK_ARGC;
    napi_value argv[TIMER_CALLBACK_ARGC] = { NULL };

    // Get the callback info
    napi_call(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc != TIMER_CALLBACK_ARGC) return NULL;

    // Get the timer info
    pomelo_platform_timer_info_t * timer_info = NULL;
    napi_call(napi_unwrap(env, argv[0], (void *) &timer_info));
    if (!timer_info) return NULL;
    
    // Call the timer callback
    timer_info->entry(timer_info->data);
    if (timer_info->repeat_ms == 0) {
        napi_call(napi_reference_unref(env, timer_info->timer_ref, NULL));

        // Clear the timer handle
        if (timer_info->handle) {
            timer_info->handle->timer = NULL;
            timer_info->handle = NULL;
        }
    }

    napi_value undefined = NULL;
    napi_call(napi_get_undefined(env, &undefined));
    return undefined;
}


void pomelo_platform_timer_info_finalize(pomelo_platform_timer_info_t * info) {
    assert(info != NULL);

    // Delete the timer reference
    if (info->timer_ref) {
        napi_delete_reference(info->platform->env, info->timer_ref);
        info->timer_ref = NULL;
    }
}


/// @brief Finalize the timer info
static void platform_timer_info_finalize_and_release(
    napi_env env,
    pomelo_platform_timer_info_t * info,
    pomelo_platform_napi_t * platform
) {
    (void) env;
    assert(info != NULL);
    assert(platform != NULL);

    // Release the timer info
    pomelo_pool_release(platform->timer_info_pool, info);
}


/// @brief Start the timer
static int timer_start(
    pomelo_platform_napi_t * platform,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    assert(entry != NULL);

    napi_env env = platform->env;

    // Get the timer create function
    napi_value timer_create = NULL;
    napi_status status = napi_get_reference_value(
        env, platform->timer_create, &timer_create
    );
    if (status != napi_ok || timer_create == NULL) return -1;

    // Get the timer start function
    napi_value timer_start = NULL;
    status = napi_get_reference_value(
        env, platform->timer_start, &timer_start
    );
    if (status != napi_ok || timer_start == NULL) return -1;

    // Get the timer callback function
    napi_value timer_callback = NULL;
    status = napi_get_reference_value(
        env, platform->timer_callback, &timer_callback
    );
    if (status != napi_ok || timer_callback == NULL) return -1;

    // Convert timeout_ms to napi_value
    napi_value timeout_ms_value = NULL;
    status = napi_create_double(env, timeout_ms, &timeout_ms_value);
    if (status != napi_ok || timeout_ms_value == NULL) return -1;

    // Convert repeat_ms to napi_value
    napi_value repeat_ms_value = NULL;
    status = napi_create_double(env, repeat_ms, &repeat_ms_value);
    if (status != napi_ok || repeat_ms_value == NULL) return -1;

    // Create null value
    napi_value null_value = NULL;
    status = napi_get_null(env, &null_value);
    if (status != napi_ok || null_value == NULL) return -1;

    // Create new timer
    napi_value args[] = {
        timer_callback,
        timeout_ms_value,
        repeat_ms_value
    };
    napi_value timer = NULL;
    status = napi_call_function(
        env,
        null_value,
        timer_create,
        sizeof(args) / sizeof(args[0]),
        args,
        &timer
    );
    if (status != napi_ok || timer == NULL) return -1;

    // Acquire the timer info   
    pomelo_platform_timer_info_t * timer_info =
        pomelo_pool_acquire(platform->timer_info_pool, NULL);
    if (timer_info == NULL) return -1;

    // Initialize the timer info
    timer_info->entry = entry;
    timer_info->timeout_ms = timeout_ms;
    timer_info->repeat_ms = repeat_ms;
    timer_info->data = data;
    timer_info->handle = handle;
    timer_info->platform = platform;

    // Wrap the timer info
    status = napi_wrap(
        env,
        timer,
        timer_info,
        (napi_finalize) platform_timer_info_finalize_and_release,
        platform,
        &timer_info->timer_ref
    );
    if (status != napi_ok) return -1;

    if (handle) {
        handle->timer = (pomelo_platform_timer_t *) timer_info->timer_ref;
    }

    // Call the timer start function
    napi_value argv[] = { timer };
    status = napi_call_function(
        env,
        null_value,
        timer_start,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        NULL
    );
    if (status != napi_ok) return -1;

    // Increase the reference count
    status = napi_reference_ref(env, timer_info->timer_ref, NULL);
    if (status != napi_ok) return -1;

    return 0;
}


int pomelo_platform_napi_timer_start(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(i != NULL);

    pomelo_platform_napi_t * platform = (pomelo_platform_napi_t *) i;
    napi_env env = platform->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return -1;

    // Start the timer
    int ret = timer_start(platform, entry, timeout_ms, repeat_ms, data, handle);
    
    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return -1;

    return ret;
}


/// @brief Stop the timer
static void timer_stop(
    pomelo_platform_napi_t * platform,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    assert(handle != NULL);

    napi_ref timer = (napi_ref) handle->timer;
    if (timer == NULL) return;
    handle->timer = NULL;

    napi_env env = platform->env;

    // Get the timer stop function
    napi_value timer_stop = NULL;
    napi_callv(napi_get_reference_value(
        env, platform->timer_stop, &timer_stop
    ));
    if (timer_stop == NULL) return;

    // Get the timer object
    napi_value timer_object = NULL;
    napi_callv(napi_get_reference_value(env, timer, &timer_object));
    if (timer_object == NULL) return;

    // Create null value
    napi_value null_value = NULL;
    napi_callv(napi_get_null(env, &null_value));

    // Call the timer stop function
    napi_value argv[] = { timer_object };
    napi_callv(napi_call_function(
        env,
        null_value,
        timer_stop,
        sizeof(argv) / sizeof(argv[0]),
        argv,
        NULL
    ));

    // Unref the reference to the timer
    napi_callv(napi_reference_unref(env, (napi_ref) timer, NULL));
}


void pomelo_platform_napi_timer_stop(
    pomelo_platform_interface_t * i,
    pomelo_platform_timer_handle_t * handle
) {
    assert(i != NULL);
    pomelo_platform_napi_t * platform = (pomelo_platform_napi_t *) i;
    napi_env env = platform->env;

    napi_handle_scope scope = NULL;
    napi_status status = napi_open_handle_scope(env, &scope);
    if (status != napi_ok) return;

    timer_stop(platform, handle);
    
    status = napi_close_handle_scope(env, scope);
    if (status != napi_ok) return;
}
