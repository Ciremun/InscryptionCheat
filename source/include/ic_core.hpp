#pragma once

#include "ic_memory.hpp"
#include "ic_offsets.hpp"

int get_current_part(HANDLE hProc);
void write_to_damage_dealt(HANDLE hProc, uintptr_t unity_player_base, int current_part, uint8_t new_damage_dealt, uint8_t expected_damage_dealt_value);
