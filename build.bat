@echo off

where /q cl || (
  echo ERROR: "cl" not found - please run this from the MSVC x86 native tools command prompt
  exit /b 1
)

@echo on

call cl source/ic_inject.cpp /DNDEBUG /O2 /EHsc /Fe:ic.exe /nologo /W4 /link /MACHINE:x86
call cl /DNDEBUG /DIMGUI_USE_STB_SPRINTF /O2 /EHsc /Fe:ic /nologo /W4 /Isource/include /Isource/include/imgui source/ic.cpp /link d3d11.lib source/lib/libMinHook.x86.lib /LTCG /DLL /MACHINE:x86 /OUT:ic.dll
