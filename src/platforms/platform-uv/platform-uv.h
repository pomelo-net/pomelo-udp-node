#ifndef POMELO_NODE_PLATFORM_UV_H
#define POMELO_NODE_PLATFORM_UV_H
#include "platform.h"
#include "platform/uv/platform-uv.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Platform UV for node-api (Wrapper for pomelo-platform-uv)
typedef struct pomelo_platform_uv_impl_s pomelo_platform_uv_impl_t;


struct pomelo_platform_uv_impl_s {
    /// @brief The base platform (interface)
    pomelo_platform_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The platform
    pomelo_platform_uv_t * platform_uv;

    /// @brief The shutdown callback
    pomelo_platform_shutdown_callback shutdown_callback;

    /// @brief Extra data
    void * extra;
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_NODE_PLATFORM_UV_H
