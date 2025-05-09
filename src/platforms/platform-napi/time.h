#ifndef POMELO_NODE_PLATFORM_NAPI_TIME_H
#define POMELO_NODE_PLATFORM_NAPI_TIME_H
#include "node_api.h"
#include "platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Get the high resolution time
uint64_t pomelo_platform_napi_hrtime(pomelo_platform_interface_t * i);


/// @brief Get the current time
uint64_t pomelo_platform_napi_now(pomelo_platform_interface_t * i);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_NODE_PLATFORM_NAPI_TIME_H
