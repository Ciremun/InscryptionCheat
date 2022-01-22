#include <string.h>

#include <windows.h>
#include <psapi.h>

#include "ic_core.hpp"
#include "ic_util.hpp"

int get_current_part(HANDLE hProc)
{
    CHAR gwsave_path[MAX_PATH] = {0};
    CHECK(GetModuleFileNameExA(hProc, NULL, gwsave_path, MAX_PATH) != 0);

    for (size_t c = MAX_PATH; c >= 0; c--)
    {
        if (gwsave_path[c] == '\\')
        {
            memcpy(gwsave_path + c + 1, "SaveFile.gwsave", sizeof("SaveFile.gwsave"));
            break;
        }
    }

    File gwsave = open_file(gwsave_path);
    if (gwsave.handle == INVALID_HANDLE_VALUE)
    {
        printf("Info: Couldn't open 'gwsave' file, assuming part 1\n");
        CHECK(close_file(gwsave.handle));
        return 1;
    }

    CHECK(map_file(&gwsave));

    int current_part = 1;
    for (int i = 0; i < 256; i++)
    {
        if (gwsave.start[i] == '\"' && memcmp(gwsave.start + i, "\"currentScene\": \"Part\"", 21) == 0)
        {
            current_part = gwsave.start[i + 21] - '0';
            printf("Info: current part: %d\n", current_part);
            break;
        }
    }

    CHECK(unmap_and_close_file(gwsave));
    return current_part;
}

uintptr_t get_current_duel_struct_address(HANDLE hProc, uintptr_t unity_player_dll_base, int current_part)
{
    uintptr_t struct_address = 0;
    switch (current_part)
    {
        case 1: {
            struct_address =
                internal_multi_level_pointer_dereference(hProc, unity_player_dll_base + part_1_base_offset, part_1_duel_struct_offsets);
        } break;
        case 2: {
            struct_address =
                internal_multi_level_pointer_dereference(hProc, unity_player_dll_base + part_2_base_offset, part_2_duel_struct_offsets);
        } break;
        case 3: {
            struct_address =
                internal_multi_level_pointer_dereference(hProc, unity_player_dll_base + part_3_base_offset, part_3_duel_struct_offsets);
        } break;
        default:
            break;
    }
    return struct_address;
}
