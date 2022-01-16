#include <stdint.h>

#include "core.hpp"
#include "util.hpp"
#include "offsets.hpp"

int main()
{
    DWORD process_id = 0;
    while ((process_id = GetProcId("Inscryption.exe")) == 0)
    {
        printf("Info: waiting for Inscryption.exe\r");
        fflush(stdout);
        Sleep(1000);
    }

    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    CHECK(process_handle);

    const uintptr_t unity_player_dll_base = GetModuleBaseAddress(process_id, "UnityPlayer.dll");
    IF(unity_player_dll_base == 0, "Couldn't get module's base address");

    int current_part = get_current_part(process_handle);
    int seconds_passed = 0;

    while (1)
    {
        if (seconds_passed == 60)
        {
            current_part = get_current_part(process_handle);
            seconds_passed = 0;
        }
        switch (current_part)
        {
            case  1: { instant_win(process_handle, unity_player_dll_base + 0x012D7080, part_1_damage_dealt_offsets); } break;
            case  2: { instant_win(process_handle, unity_player_dll_base + 0x0127E340, part_2_damage_dealt_offsets); } break;
            case  3: { instant_win(process_handle, unity_player_dll_base + 0x0127E340, part_3_damage_dealt_offsets); } break;
            default: { ExitProcess(1); } break;
        }
        Sleep(1000);
        seconds_passed++;
    }

    return 0;
}