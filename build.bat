@echo off

where /q cl || (
  echo ERROR: "cl" not found - please run this from the MSVC x86 native tools command prompt
  exit /b 1
)

setlocal

for %%i in (%*) do (
    if "%%i" == "rebuild" (
        set IC_REBUILD=1
    ) else if "%%i" == "run" (
        set IC_RUN=1
    ) else (
        echo ERROR: Unknown flag "%%i"
        goto :eof
    )
)

set ICEXE_BUILD=cl source/ic_inject.cpp /O2 /EHsc /Fe:ic.exe /std:c++17 /link /MACHINE:x86
set ICDLL_BUILD=cl /DNDEBUG /O2 /EHsc /Fe:ic /nologo /W4 /Isource/include /Isource/include/imgui source/ic.cpp /link d3d11.lib source/lib/libMinHook.x86.lib /DLL /LTCG /MACHINE:x86 /OUT:ic.dll

if defined IC_REBUILD goto :rebuild

if not exist ic.exe call %ICEXE_BUILD%
if not exist ic.dll call %ICDLL_BUILD%

if defined IC_RUN goto :run

goto :eof

:rebuild

call %ICEXE_BUILD%
call %ICDLL_BUILD%

if not defined IC_RUN goto :eof

:run

.\ic.exe

endlocal

:eof
