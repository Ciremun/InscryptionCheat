#include <stdint.h>

#include "process.hpp"
#include "util.hpp"

static DWORD chapter = 2;

int main()
{
    DWORD process_id = GetProcId("Inscryption.exe");
    IF(process_id == 0, "Couldn't get process id");
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    CHECK(process_handle);
    uintptr_t unity_player_dll_base = GetModuleBaseAddress(process_id, "UnityPlayer.dll");
    const uint32_t new_damage_dealt = 128;
    const uintptr_t base = unity_player_dll_base + 0x0127E340;
    const uintptr_t offsets[] = { 0x20, 0x618, 0x3c, 0x10, 0x8, 0x18, 0x14 };
    while (1)
    {
        switch (chapter)
        {
            case 2:
            {
                uintptr_t damage_dealt_address = external_multi_level_pointer_dereference(process_handle, base, sizeof(uint32_t), offsets);
                if (damage_dealt_address != 0)
                {
                    DWORD damage_dealt;
                    external_memory_read(process_handle, (LPVOID)damage_dealt_address, &damage_dealt, sizeof(uint32_t));
                    if (damage_dealt == 0)
                        external_memory_patch(process_handle, (LPVOID)damage_dealt_address, &new_damage_dealt, sizeof(uint32_t));
                }
            } break;
            default:
                ExitProcess(1);
        }
        Sleep(1000);
    }
    return 0;
}