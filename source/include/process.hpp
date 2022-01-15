#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include "util.hpp"
#include "process.hpp"

uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName);
DWORD GetProcId(const char* procName);
void external_memory_patch(HANDLE hProc, LPVOID base, LPCVOID buffer, size_t size);
int external_memory_read(HANDLE hProc, LPVOID base, LPVOID buffer, size_t size);

template <size_t N>
uintptr_t external_multi_level_pointer_dereference(HANDLE hProc, uintptr_t base, size_t size, const uintptr_t (&offsets)[N])
{
    for (size_t i = 0; i < N; ++i) {
        external_memory_read(hProc, (LPVOID)base, &base, size);
        base += offsets[i];
    }
    return base;
}
