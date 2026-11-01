@echo off
REM ============================================================
REM NAV-JEST deploy: windeployqt + MySQL runtime + data files
REM Usage: scripts\deploy.bat [Debug^|Release] [Qt dir]
REM   default: Debug  C:\Qt6\6.11.0\msvc2022_64
REM Output: build\cmake\<CONFIG>\
REM ============================================================
setlocal

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug
set QT_BIN=%2
if "%QT_BIN%"=="" set QT_BIN=C:\Qt6\6.11.0\msvc2022_64

set ROOT=%~dp0..
set OUT=%ROOT%\build\cmake\%CONFIG%
for %%I in ("%OUT%") do set "OUT=%%~fI"
set WDT=%QT_BIN%\bin\windeployqt.exe

if not exist "%OUT%\NAV_JEST.exe" (
    echo [ERROR] %OUT%\NAV_JEST.exe not found. Build first:
    echo   cmake -S . -B build/cmake -DCMAKE_PREFIX_PATH=%QT_BIN%
    echo   cmake --build build/cmake --config %CONFIG% --target NAV_JEST
    exit /b 1
)
if not exist "%WDT%" (
    echo [ERROR] windeployqt not found: %WDT%
    exit /b 1
)

echo [1/3] windeployqt (QML module scan)...
if /I "%CONFIG%"=="Debug" (set WDT_CFG=--debug) else (set WDT_CFG=--release)
"%WDT%" %WDT_CFG% --dir "%OUT%" --qmldir "%ROOT%\resources\qml" "%OUT%\NAV_JEST.exe"

echo [2/3] MySQL runtime (libmysql + QMYSQL plugin, %CONFIG%)...
if /I "%CONFIG%"=="Debug" (set MYDIR=%ROOT%\deploy\mysql\debug) else (set MYDIR=%ROOT%\deploy\mysql\release)
copy /Y "%MYDIR%\libmysql.dll" "%OUT%\" >nul
if not exist "%OUT%\sqldrivers" mkdir "%OUT%\sqldrivers"
copy /Y "%MYDIR%\sqldrivers\*.dll" "%OUT%\sqldrivers\" >nul
if not exist "%OUT%\platforms" mkdir "%OUT%\platforms"
if /I "%CONFIG%"=="Debug" (set QOFF=qoffscreend.dll) else (set QOFF=qoffscreen.dll)
copy /Y "%QT_BIN%\plugins\platforms\%QOFF%" "%OUT%\platforms\" >nul 2>&1

echo [2.5/3] Offline map tiles (hardlink, no extra disk usage)...
set TILE_SRC=%ROOT%\build\Desktop_Qt_6_11_0_MSVC2022_64bit-Debug\bin
for %%T in (China.mbtiles global_0_2.mbtiles sjz.mbtiles) do (
  if exist "%TILE_SRC%\%%T" (
    if not exist "%OUT%\%%T" mklink /H "%OUT%\%%T" "%TILE_SRC%\%%T" >nul
  )
)

echo [3/3] Done. Deploy dir: %OUT%
echo NOTE: the QMYSQL plugin must match the Qt kit (compiler/version).
echo       deploy\mysql currently holds the Qt 6.11.0 MSVC build.
endlocal
