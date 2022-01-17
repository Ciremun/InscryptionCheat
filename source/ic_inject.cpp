#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

DWORD GetProcId(const char* procName)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!_stricmp(procEntry.szExeFile, procName))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

void input_dll_path(string& dllPath_s)
{
    cout << "dll path?" << endl;
    getline(cin, dllPath_s);
    if (dllPath_s.front() == '\"' && dllPath_s.back() == '\"')
    {
        dllPath_s.erase(remove(dllPath_s.begin(), dllPath_s.end(), '\"'), dllPath_s.end());
    }
}

int main(int argc, char** argv)
{
    string dllPath_s;
    string procName_s;

    if (argc < 2)
    {
        input_dll_path(dllPath_s);
        cout << "process?" << endl;
        getline(cin, procName_s);
    }
    else
    {
        dllPath_s = argv[1];
        procName_s = argv[2];
    }

    while (!filesystem::exists(dllPath_s))
    {
        input_dll_path(dllPath_s);
    }

    const char* dllPath = dllPath_s.c_str();
    const char* procName = procName_s.c_str();
    DWORD procId = 0;

    cout << "info: waiting for process " << procName << " .." << endl;
    while (!procId)
    {
        procId = GetProcId(procName);
        Sleep(100);
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE)
    {
        void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (loc)
        {
            WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0);
        }

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }

    if (hProc)
    {
        CloseHandle(hProc);
    }

    return 0;
}
