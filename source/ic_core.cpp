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
