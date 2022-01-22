#pragma once

#include "ic_memory.hpp"
#include "ic_offsets.hpp"

int get_current_part(HANDLE hProc);
uintptr_t get_current_duel_struct_address(HANDLE hProc, uintptr_t unity_player_dll_base, int current_part);
