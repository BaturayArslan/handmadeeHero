@echo off

mkdir "%~dp0..\build\" 2>nul
pushd "%~dp0..\build\"
cl -FC -Zi -Od -I "..\build\includes\xaudio2" -FA "..\code\win32-entry.cpp" user32.lib gdi32.lib Ole32.lib
popd
