#include <string.h>

#include <windows.h>
#include <psapi.h>

#include "core.hpp"
#include "util.hpp"

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
    IF(gwsave.handle == INVALID_HANDLE_VALUE, "Couldn't open 'gwsave' file");
    CHECK(map_file(&gwsave));

    int current_part = 0;
    for (int i = 0; i < 256; i++)
    {
        if (gwsave.start[i] == '\"' && memcmp(gwsave.start + i, "\"currentScene\": \"Part\"", 21) == 0)
        {
            current_part = gwsave.start[i + 21] - '0';
            goto end;
        }
    }

end:

    CHECK(unmap_file(gwsave));
    CHECK(close_file(gwsave.handle));

    IF(current_part == 0, "Couldn't determine current part of the game");
    return current_part;
}
