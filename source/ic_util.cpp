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

File open_file(const char *path)
{
    File f = {0};
    f.handle = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (f.handle == INVALID_HANDLE_VALUE)
        return f;
    return f;
}

LONGLONG get_file_size(HANDLE hFile)
{
    LARGE_INTEGER lpFileSize = {0};
    CHECK(GetFileSizeEx(hFile, &lpFileSize) != 0);
    return lpFileSize.QuadPart;
}

int close_file(HANDLE handle)
{
    if (CloseHandle(handle) == 0)
        return 0;
    return 1;
}

int map_file(File *f)
{
    f->hMap = CreateFileMappingA(f->handle, 0, PAGE_READONLY, 0, 0, 0);
    if (f->hMap == 0)
    {
        CloseHandle(f->handle);
        return 0;
    }
    f->start = (uint8_t *)MapViewOfFile(f->hMap, FILE_MAP_READ, 0, 0, 0);
    if (f->start == 0)
    {
        CloseHandle(f->hMap);
        CloseHandle(f->handle);
        return 0;
    }
    return 1;
}

int unmap_file(File f)
{
    if (UnmapViewOfFile(f.start) == 0)
        return 0;
    if (CloseHandle(f.hMap) == 0)
        return 0;
    return 1;
}

int unmap_and_close_file(File f)
{
    return unmap_file(f) && close_file(f.handle);
}
