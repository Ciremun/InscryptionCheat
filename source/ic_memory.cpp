#include "ic_memory.hpp"

int internal_memory_patch(void* dst, void* src, size_t len)
{
    DWORD oldprotect;
    if (VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &oldprotect) == 0)
        return 0;

    memcpy(dst, src, len);

    if (VirtualProtect(dst, len, oldprotect, &oldprotect) == 0)
        return 0;

    return 1;
}
