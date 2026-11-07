@echo off

mkdir "%~dp0..\build\" 2>nul
pushd "%~dp0..\build\"
cl -FC -Zi -Od -FA "..\code\win32-entry.cpp" user32.lib gdi32.lib
popd
