@echo off
chcp 65001 >nul
title NAV_JEST 三幕连续演示
cd /d "%~dp0"

echo ============================================================
echo   三幕连续演示：定位一天(石家庄) - 地月转移 - 环月
echo   播放时长 %1 秒 (缺省 300)。正在启动应用与回放器...
echo ============================================================

where python >nul 2>nul
if errorlevel 1 (
  echo [错误] 未找到 python，请先安装 Python 3 并加入 PATH
  pause
  exit /b 1
)

set SECONDS=300
if not "%1"=="" set SECONDS=%1

REM 1) 回放器先启动(监听 8000 等待连接)
start "演示回放器" cmd /k python -I scripts\orb_file_player.py --file full_demo.txt --seconds %SECONDS% --wait 300

REM 2) 应用后启动(3 秒内自动连上)
ping -n 3 127.0.0.1 >nul
start "" "%~dp0build\cmake\Release\NAV_JEST.exe"

echo 已启动。演示结束后本窗口可关闭。
ping -n 11 127.0.0.1 >nul
