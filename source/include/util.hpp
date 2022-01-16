#pragma once

#include <windows.h>

#include <stdio.h>
#include <stdint.h>

struct File
{
    HANDLE hMap;
    HANDLE handle;
    size_t size;
    uint8_t *start;
};

void GetErrorString(DWORD dwErr, CHAR wszMsgBuff[512]);
File open_file(const char *path);
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
        ExitProcess(1);\
    }\
}\
while (0)

#define IF(condition, message)\
do\
{\
    if (condition)\
    {\
        fprintf(stderr, "%s:%d: Error: %s", __FILE__, __LINE__, message);\
        ExitProcess(1);\
    }\
}\
while (0)
