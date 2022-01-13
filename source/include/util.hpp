#pragma once

#include <windows.h>
#include <stdio.h>

void GetErrorString(DWORD dwErr, CHAR wszMsgBuff[512]);

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
    if (!condition)\
    {\
        fprintf(stderr, "%s:%d: Error: %s", __FILE__, __LINE__, message);\
        ExitProcess(1);\
    }\
}\
while (0)
