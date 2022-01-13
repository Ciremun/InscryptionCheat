#include "util.hpp"

void GetErrorString(DWORD dwErr, CHAR wszMsgBuff[512])
{
    DWORD dwChars;
    dwChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr, 0, wszMsgBuff, 512, NULL);
    if (dwChars == 0)
    {
        HINSTANCE hInst;

        hInst = LoadLibrary("Ntdsbmsg.dll");
        if (hInst == NULL)
        {
            printf("cannot load Ntdsbmsg.dll\n");
            ExitProcess(1);
        }
        dwChars = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, hInst, dwErr, 0, wszMsgBuff, 512, NULL);
        FreeLibrary(hInst);
    }
    if (dwChars == 0)
        memcpy(wszMsgBuff, "Error message not found", sizeof("Error message not found"));
}
