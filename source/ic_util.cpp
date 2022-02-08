#include <windows.h>
#include <tlhelp32.h>

#include "ic_util.hpp"

uintptr_t GetModuleBaseAddress(const char* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_stricmp(modEntry.szModule, modName))
                {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

void GetErrorString(DWORD dwErr, CHAR wszMsgBuff[512])
{
    DWORD dwChars;
    dwChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr, 0, wszMsgBuff, 512, NULL);

    if (dwChars == 0)
        memcpy(wszMsgBuff, "Error message not found", sizeof("Error message not found"));
}

int detour_32(void *src, void *dst, int len)
{
    DWORD oldProtect;
    if (VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect) == 0)
        return 0;

    memset(src, 0x90, len);

    uintptr_t relative_address = (uintptr_t)dst - (uintptr_t)src - 5;

    *(BYTE *)src = 0xE9;

    *(uintptr_t *)((uintptr_t)src + 1) = relative_address;

    if (VirtualProtect(src, len, oldProtect, &oldProtect) == 0)
        return 0;

    return 1;
}
