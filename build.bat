@echo off

where /q cl || (
  echo ERROR: "cl" not found - please run this from the MSVC x64 native tools command prompt
  exit /b 1
)

call cl /O2 /Fe:ic /nologo /W4 /Isource/include source/*.cpp
