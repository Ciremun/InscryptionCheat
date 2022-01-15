#pragma once

#include "process.hpp"

template <size_t N>
void instant_win(HANDLE hProc, uintptr_t base, const uintptr_t (&offsets)[N])
{
    const uint8_t new_damage_dealt = 16;
    uintptr_t damage_dealt_address = external_multi_level_pointer_dereference(hProc, base, sizeof(uint32_t), offsets);
    if (damage_dealt_address != 0)
    {
        uint8_t damage_dealt;
        external_memory_read(hProc, (LPVOID)damage_dealt_address, &damage_dealt, sizeof(uint8_t));
        if (damage_dealt == 0)
            external_memory_patch(hProc, (LPVOID)damage_dealt_address, &new_damage_dealt, sizeof(uint8_t));
    }
}
