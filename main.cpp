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

template<size_t N>
uintptr_t external_multi_level_pointer_dereference(HANDLE hProc, uintptr_t base, size_t size, const uintptr_t (&offsets)[N])
{
    for (size_t i = 0; i < N; ++i) {
        CHECK(ReadProcessMemory(hProc, (LPCVOID)base, &base, size, nullptr) != 0);
        base += offsets[i];
    }
    return base;
}

void external_memory_patch(HANDLE hProc, LPVOID base, LPCVOID buffer, size_t size)
{
    DWORD oldprotect;
    CHECK(VirtualProtectEx(hProc, base, size, PAGE_EXECUTE_READWRITE, &oldprotect) != 0);
    CHECK(WriteProcessMemory(hProc, base, buffer, size, nullptr) != 0);
    CHECK(VirtualProtectEx(hProc, base, size, oldprotect, &oldprotect) != 0);
}

int main()
{
    DWORD process_id = GetProcId("Inscryption.exe");
    CHECK(process_id != 0);
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    CHECK(process_handle);
    uintptr_t unity_player_dll_base = GetModuleBaseAddress(process_id, "UnityPlayer.dll");
    const uint32_t new_enemys_hits = 0;
    const uintptr_t base = unity_player_dll_base + 0x0127E340;
    const uintptr_t offsets[] = { 0x20, 0x618, 0x3c, 0x10, 0x8, 0x18, 0x10 };
    while (1)
    {
        uintptr_t enemys_hits_address = external_multi_level_pointer_dereference(process_handle, base, sizeof(uint32_t), offsets);
        DWORD enemys_hits;
        CHECK(ReadProcessMemory(process_handle, (LPCVOID)enemys_hits_address, &enemys_hits, sizeof(uint32_t), nullptr) != 0);
        if (enemys_hits != 0)
            external_memory_patch(process_handle, (LPVOID)enemys_hits_address, &new_enemys_hits, sizeof(uint32_t));
        printf("enemy's hits: %lu\r", enemys_hits);
        fflush(stdout);
        Sleep(100);
    }
    return 0;
}