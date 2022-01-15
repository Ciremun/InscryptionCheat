#include <stdint.h>

#include "core.hpp"
#include "util.hpp"
#include "offsets.hpp"

DWORD current_part = 3;

int main()
{
    DWORD process_id = GetProcId("Inscryption.exe");
    IF(process_id == 0, "Couldn't get process id");

    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    CHECK(process_handle);

    const uintptr_t unity_player_dll_base = GetModuleBaseAddress(process_id, "UnityPlayer.dll");
    IF(unity_player_dll_base == 0, "Couldn't get module's base address");

    const uintptr_t base = unity_player_dll_base + 0x0127E340;
    while (1)
    {
        switch (current_part)
        {
            case 1:
            {
                IF(1, "Not Implemented");
            } break;
            case 2:
            {
                instant_win(process_handle, base, part_2_damage_dealt_offsets);
            } break;
            case 3:
            {
                instant_win(process_handle, base, part_3_damage_dealt_offsets);
            } break;
            default:
                ExitProcess(1);
        }
        Sleep(1000);
    }
    return 0;
}