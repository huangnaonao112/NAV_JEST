@echo off
REM Launch NAV_JEST with the 2D layout diagnostic overlay enabled.
REM The overlay prints window/view/tile sizes and zoom coverage in the 2D view.
setlocal
set NAVJEST_MAP_DEBUG=1
set ROOT=%~dp0..
cd /d "%ROOT%\build\cmake\Release"
start "" NAV_JEST.exe
