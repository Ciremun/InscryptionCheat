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
File open_file(const char *path);
LONGLONG get_file_size(HANDLE hFile);
int close_file(HANDLE handle);
int map_file(File *f);
int unmap_file(File f);
int unmap_and_close_file(File f);

#define CHECK(condition)\
do\
{\
    if (!condition)\
    {\
        CHAR wszMsgBuff[512];\
        DWORD error_code = GetLastError();\
        GetErrorString(error_code, wszMsgBuff);\
        fprintf(stderr, "%s:%d: Error %d: %s", __FILE__, __LINE__, error_code, wszMsgBuff);\
    }\
}\
while (0)

#define ERR(message) fprintf(stderr, "%s:%d: Error: %s\n", __FILE__, __LINE__, message)

#define IF(condition, message)\
do\
{\
    if (condition)\
    {\
        ERR(message);\
    }\
}\
while (0)
