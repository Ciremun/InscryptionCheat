#include "process.hpp"

uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
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

DWORD GetProcId(const char* procName)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!_stricmp(procEntry.szExeFile, procName))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

void external_memory_patch(HANDLE hProc, LPVOID base, LPCVOID buffer, size_t size)
{
    DWORD oldprotect;
    CHECK(VirtualProtectEx(hProc, base, size, PAGE_EXECUTE_READWRITE, &oldprotect) != 0);
    CHECK(WriteProcessMemory(hProc, base, buffer, size, nullptr) != 0);
    CHECK(VirtualProtectEx(hProc, base, size, oldprotect, &oldprotect) != 0);
}

void external_memory_read(HANDLE hProc, LPVOID base, LPVOID buffer, size_t size)
{
    CHECK(ReadProcessMemory(hProc, (LPCVOID)base, buffer, size, nullptr) != 0);
}
