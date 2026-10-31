#ifndef DC2WS2_H
#define DC2WS2_H

#ifdef BUILDING_DC2WS2_DLL
#define DC2WS2_API __declspec(dllexport)
#else
#define DC2WS2_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>

#define PATH_MAX_LEN 2048

// Logging level markers
typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_BUFFER_DUMP
} LogLevel;

// Standard calling convention callback for parental process diagnostics
typedef void (CALLBACK *NetworkLogCallback)(LogLevel level, const char *message);

// Configuration controls for runtime switches
typedef struct {
    bool enable_logging;
    NetworkLogCallback log_callback;
    size_t initial_buffer_size;
    size_t max_buffer_size;
    char custom_user_agent[256];
} NetworkConfig;

#ifdef __cplusplus
extern "C" {
#endif

// Public initialization exports
DC2WS2_API bool WINAPI Dc2ws2Init(void);
DC2WS2_API void WINAPI Dc2ws2Cleanup(void);

// Exported asynchronous multi-threaded download entrypoint
DC2WS2_API HANDLE WINAPI Dc2ws2DownloadAsync(
    const char *url, 
    const char *dest_path, 
    const NetworkConfig *config
);

#ifdef __cplusplus
}
#endif

#endif // DC2WS2_H