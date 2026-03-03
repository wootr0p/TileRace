@echo off
REM ==============================
REM Deploy tilerace to Itch.io
REM ==============================

echo.
echo === Deploying Windows build ===
butler push win wootrop/tilerace:win

REM echo.
REM echo === Deploying Mac build ===
REM butler push mac wootrop/tilerace:mac

echo.
echo === Done! ===
pause
