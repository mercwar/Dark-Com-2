#include "dc2ws2.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#define HOST_MAX 256
#define RECV_BUF_SIZE 8192

static LONG g_ws_initialized_count = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL; 
    (void)lpvReserved;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            while (g_ws_initialized_count > 0) {
                WSACleanup();
                InterlockedDecrement(&g_ws_initialized_count);
            }
            break;
    }
    return TRUE;
}

DC2WS2_API bool WINAPI Dc2ws2Init(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        InterlockedIncrement(&g_ws_initialized_count);
        return true;
    }
    return false;
}

DC2WS2_API void WINAPI Dc2ws2Cleanup(void) {
    if (g_ws_initialized_count > 0) {
        WSACleanup();
        InterlockedDecrement(&g_ws_initialized_count);
    }
}

typedef struct {
    char *url;
    char *dest_path;
    NetworkConfig config;
} ThreadContext;

static void internal_log(const NetworkConfig *config, LogLevel level, const char *format, ...) {
    if (!config || !config->enable_logging || !config->log_callback) return;
    char log_buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);
    config->log_callback(level, log_buffer);
}

static DWORD WINAPI AsyncWorkerProc(LPVOID lpParam) {
    ThreadContext *context = (ThreadContext *)lpParam;
    if (!context) return 1;

    NetworkConfig *cfg = &context->config;
    internal_log(cfg, LOG_LEVEL_INFO, "[dc2ws2] Worker activated for URL: %s", context->url);

    // 1. Native WinHTTP TLS/HTTPS Routing for GitHub Assets
    if (_strnicmp(context->url, "https://", 8) == 0) {
        wchar_t wUrl[2048];
        MultiByteToWideChar(CP_UTF8, 0, context->url, -1, wUrl, 2048);

        HINTERNET hSession = WinHttpOpen(L"dc2ws2-winhttp/2.2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            URL_COMPONENTSW urlComp = {0};
            urlComp.dwStructSize = sizeof(urlComp);
            wchar_t hostName[256] = {0};
            wchar_t urlPath[2048] = {0};
            urlComp.lpszHostName = hostName;
            urlComp.dwHostNameLength = 256;
            urlComp.lpszUrlPath = urlPath;
            urlComp.dwUrlPathLength = 2048;

            if (WinHttpCrackUrl(wUrl, (DWORD)wcslen(wUrl), 0, &urlComp)) {
                HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort ? urlComp.nPort : INTERNET_DEFAULT_HTTPS_PORT, 0);
                if (hConnect) {
                    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                    if (hRequest) {
                        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
                        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

                        LPCWSTR headers = L"User-Agent: curl/8.4.0\r\nAccept: */*\r\nAccept-Encoding: identity\r\n";
                        if (WinHttpSendRequest(hRequest, headers, -1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                            WinHttpReceiveResponse(hRequest, NULL)) {
                            
                            FILE *f = NULL;
                            if (fopen_s(&f, context->dest_path, "wb") == 0 && f) {
                                DWORD downloaded = 0;
                                char buffer[8192];
                                do {
                                    downloaded = 0;
                                    if (WinHttpReadData(hRequest, buffer, sizeof(buffer), &downloaded) && downloaded > 0) {
                                        fwrite(buffer, 1, downloaded, f);
                                    }
                                } while (downloaded > 0);
                                fclose(f);
                                internal_log(cfg, LOG_LEVEL_INFO, "[dc2ws2] WinHTTP SSL download committed successfully.");
                            }
                        }
                        WinHttpCloseHandle(hRequest);
                    }
                    WinHttpCloseHandle(hConnect);
                }
            }
            WinHttpCloseHandle(hSession);
        }
        goto cleanup_context;
    }

    // 2. Pure WinSock Routing for Plain HTTP (e.g. localhost)
    char host[HOST_MAX] = {0};
    char port[10] = "80";
    char path[PATH_MAX_LEN] = "/";

    const char *p = context->url;
    if (_strnicmp(p, "http://", 7) == 0) {
        p += 7;
    }

    const char *slash = strchr(p, '/');
    if (!slash) {
        strcpy_s(host, sizeof(host), p);
    } else {
        size_t host_len = (size_t)(slash - p);
        if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
        strncpy_s(host, sizeof(host), p, host_len);
        strcpy_s(path, sizeof(path), slash);
    }

    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        strcpy_s(port, sizeof(port), colon + 1);
    }

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        internal_log(cfg, LOG_LEVEL_ERROR, "[dc2ws2] DNS resolution failed for host: %s", host);
        goto cleanup_context;
    }

    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET || connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        internal_log(cfg, LOG_LEVEL_ERROR, "[dc2ws2] Socket connection failed.");
        if (sock != INVALID_SOCKET) closesocket(sock);
        freeaddrinfo(res);
        goto cleanup_context;
    }
    freeaddrinfo(res);

    DWORD timeout_ms = 15000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));

    char req_buf[2048];
    const char *ua = (cfg->custom_user_agent[0] != '\0') ? cfg->custom_user_agent : "curl/8.4.0";
    int req_len = snprintf(req_buf, sizeof(req_buf),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n\r\n",
        path, host, ua);

    if (send(sock, req_buf, req_len, 0) == SOCKET_ERROR) {
        internal_log(cfg, LOG_LEVEL_ERROR, "[dc2ws2] Failed to send HTTP request.");
        closesocket(sock);
        goto cleanup_context;
    }

    size_t total_data = 0;
    size_t capacity = 1024 * 1024;
    char *response_stream = (char *)malloc(capacity);
    if (!response_stream) {
        closesocket(sock);
        goto cleanup_context;
    }

    int bytes_received = 0;
    while (1) {
        if (capacity - total_data <= 65536) {
            size_t new_capacity = capacity * 2;
            char *new_stream = (char *)realloc(response_stream, new_capacity);
            if (!new_stream) break;
            response_stream = new_stream;
            capacity = new_capacity;
        }

        bytes_received = recv(sock, response_stream + total_data, (int)(capacity - total_data - 1), 0);
        if (bytes_received > 0) {
            total_data += (size_t)bytes_received;
        } else if (bytes_received == 0) {
            break;
        } else {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT && total_data > 0) break;
            break;
        }
    }
    
    closesocket(sock);

    if (total_data > 0) {
        FILE *f = NULL;
        if (fopen_s(&f, context->dest_path, "wb") == 0 && f) {
            fwrite(response_stream, 1, total_data, f);
            fclose(f);
            internal_log(cfg, LOG_LEVEL_INFO, "[dc2ws2] Raw network stream committed (%zu bytes written).", total_data);
        }
    }

    free(response_stream);

cleanup_context:
    if (context->url) free(context->url);
    if (context->dest_path) free(context->dest_path);
    free(context);
    return 0;
}

DC2WS2_API HANDLE WINAPI Dc2ws2DownloadAsync(const char *url, const char *dest_path, const NetworkConfig *config) {
    if (g_ws_initialized_count <= 0 || !url || !dest_path) return NULL;

    ThreadContext *context = (ThreadContext *)malloc(sizeof(ThreadContext));
    if (!context) return NULL;

    size_t url_len = strlen(url) + 1;
    size_t path_len = strlen(dest_path) + 1;

    context->url = (char *)malloc(url_len);
    context->dest_path = (char *)malloc(path_len);

    if (!context->url || !context->dest_path) {
        free(context->url);
        free(context->dest_path);
        free(context);
        return NULL;
    }

    strcpy_s(context->url, url_len, url);
    strcpy_s(context->dest_path, path_len, dest_path);

    if (config) {
        context->config = *config;
    } else {
        memset(&context->config, 0, sizeof(NetworkConfig));
    }

    HANDLE hThread = CreateThread(NULL, 0, AsyncWorkerProc, context, 0, NULL);
    if (!hThread) {
        free(context->url);
        free(context->dest_path);
        free(context);
        return NULL;
    }

    return hThread;
}