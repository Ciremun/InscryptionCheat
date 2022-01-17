#pragma once

#include "ic_process.hpp"

int get_current_part(HANDLE hProc);

template <size_t N>
void instant_win(HANDLE hProc, uintptr_t base, const uintptr_t (&offsets)[N])
{
    const uint8_t new_damage_dealt = 16;
    uintptr_t damage_dealt_address = internal_multi_level_pointer_dereference(hProc, base, offsets);
    if (damage_dealt_address != 0)
    {
        uint8_t damage_dealt;
        if (internal_memory_read(hProc, damage_dealt_address, &damage_dealt) && damage_dealt == 0)
            internal_memory_write(damage_dealt_address, &new_damage_dealt);
    }
}
