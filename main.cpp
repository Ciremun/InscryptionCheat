#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdint.h>

#define CHECK(cond)                                                               \
do                                                                                \
{                                                                                 \
    if (!cond)                                                                    \
    {                                                                             \
        fprintf(stderr, "Error code: %ld, line: %d\n", GetLastError(), __LINE__); \
        ExitProcess(1);                                                           \
    }                                                                             \
}                                                                                 \
while (0)

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

int main()
{
    DWORD process_id = GetProcId("Inscryption.exe");
    CHECK(process_id != 0);
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    CHECK(process_handle != 0);
    uintptr_t unity_dll_base = GetModuleBaseAddress(process_id, "UnityPlayer.dll");
    const uint32_t new_enemys_hits = 0;
    while (1)
    {
        uintptr_t base = unity_dll_base + 0x0127E400;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x40;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x618;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x3c;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x10;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x8;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x18;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &base, sizeof(uint32_t), nullptr) != 0);
        base += 0x10;
        DWORD enemys_hits;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)base, &enemys_hits, sizeof(uint32_t), nullptr) != 0);
        if (enemys_hits != 0)
        {
            DWORD oldprotect;
            CHECK(VirtualProtectEx(process_handle, (LPVOID)base, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &oldprotect) != 0);
            CHECK(WriteProcessMemory(process_handle, (LPVOID)base, &new_enemys_hits, sizeof(new_enemys_hits), nullptr) != 0);
            CHECK(VirtualProtectEx(process_handle, (LPVOID)base, sizeof(uint32_t), oldprotect, &oldprotect) != 0);
        }
        printf("enemy's hits: %lu\r", enemys_hits);
        fflush(stdout);
        Sleep(2000);
    }
    return 0;
}