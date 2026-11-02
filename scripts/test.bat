@echo off
REM One-click regression: build all test targets and run ctest
REM (3 algorithm tests + 1 bridge-model QTest, millisecond-level).
REM Default is Release: the Debug CRT on this machine has intermittent
REM OutputDebugString/heap-lock contention that can stall Qt debug builds.
REM Usage: scripts\test.bat [Release^|Debug]
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release
set ROOT=%~dp0..

echo [1/2] Building test targets (%CONFIG%) ...
cmake --build "%ROOT%\build\cmake" --config %CONFIG% ^
    --target navcore_orbit_tests navcore_signal_tests navcore_receiver_tests navcore_trajectory_tests navcore_model_tests navcore_route_tests navcore_satmodel_tests --parallel
if errorlevel 1 exit /b 1

echo [2/2] Running ctest ...
ctest --test-dir "%ROOT%\build\cmake" -C %CONFIG% --output-on-failure
endlocal
