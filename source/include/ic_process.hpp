#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include "ic_util.hpp"
#include "ic_process.hpp"

uintptr_t GetModuleBaseAddress(const char* modName);

template <typename T>
void internal_memory_write(uintptr_t base, const T *buffer)
{
    *(T*)base = *buffer;
}

template <typename T>
int internal_memory_read(HANDLE hProc, uintptr_t base, T *buffer)
{
    PSAPI_WORKING_SET_EX_INFORMATION info;
    info.VirtualAddress = (PVOID)base;
    CHECK(QueryWorkingSetEx(hProc, &info, sizeof(info)) != 0);
    if (!info.VirtualAttributes.Valid)
        return 0;
    *buffer = *(T*)base;
    return 1;
}

template <size_t N>
uintptr_t internal_multi_level_pointer_dereference(HANDLE hProc, uintptr_t base, const uintptr_t (&offsets)[N])
{
    for (size_t i = 0; i < N; ++i) {
        internal_memory_read(hProc, base, &base);
        base += offsets[i];
    }
    return base;
}