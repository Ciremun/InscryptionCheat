#pragma once

#include <windows.h>
#include <psapi.h>

#include <stdio.h>
#include <stdint.h>

struct File
{
    HANDLE hMap;
    HANDLE handle;
    size_t size;
    uint8_t *start;
};

uintptr_t GetModuleBaseAddress(const char* modName);
void GetErrorString(DWORD dwErr, CHAR wszMsgBuff[512]);
int detour_32(void *src, void *dst, int len);

#ifndef NDEBUG
#define IC_ERROR(message) fprintf(stderr, "Error: %s:%d: %s\n", __FUNCSIG__, __LINE__, message)
#define IC_ERROR_FMT(fmt, ...) fprintf(stderr, "Error: %s:%d: " fmt "\n", __FUNCSIG__, __LINE__, __VA_ARGS__)
#define IC_INFO(message) fprintf(stdout, "Info: %s\n", message)
#define IC_INFO_FMT(fmt, ...) fprintf(stdout, "Info: " fmt "\n", __VA_ARGS__)
#else
#define DO_NOTHING do {} while(0)
#define IC_ERROR(message)      DO_NOTHING
#define IC_ERROR_FMT(fmt, ...) DO_NOTHING
#define IC_INFO(message)       DO_NOTHING
#define IC_INFO_FMT(fmt, ...)  DO_NOTHING
#endif // NDEBUG

#define IC_WINAPI_ERROR()\
do\
{\
    CHAR wszMsgBuff[512];\
    DWORD error_code = GetLastError();\
    GetErrorString(error_code, wszMsgBuff);\
    IC_ERROR_FMT("%d - %s", error_code, wszMsgBuff);\
}\
while (0)

#define IC_CHECK(condition)\
do\
{\
    if (!condition)\
    {\
        IC_WINAPI_ERROR();\
    }\
}\
while (0)

#define IC_ERROR_IF(condition, message)\
do\
{\
    if (condition)\
    {\
        IC_ERROR(message);\
    }\
}\
while (0)
