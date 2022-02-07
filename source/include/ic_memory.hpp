#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include "ic_util.hpp"

int internal_memory_patch(void* dst, void* src, size_t len);

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
    IC_CHECK(QueryWorkingSetEx(hProc, &info, sizeof(info)) != 0);
    if (!info.VirtualAttributes.Valid)
        return 0;
    *buffer = *(T*)base;
    return 1;
}

template <size_t N>
uintptr_t internal_multi_level_pointer_dereference(HANDLE hProc, uintptr_t base, const uintptr_t (&offsets)[N])
{
    for (size_t i = 0; i < N; ++i) {
        if (internal_memory_read(hProc, base, &base) == 0)
            return 0;
        base += offsets[i];
    }
    return base;
}
