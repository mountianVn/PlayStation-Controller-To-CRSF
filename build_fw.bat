@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "PIO_EXE=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
set "BUILD_ENV=esp32-s3-devkitc-1"
set "SRC_BIN=%PROJECT_DIR%.pio\build\%BUILD_ENV%\firmware.bin"
set "FW_DIR=%PROJECT_DIR%FW"
set "DST_BIN=%FW_DIR%\file.bin"

if not exist "%PIO_EXE%" (
  echo [ERROR] PlatformIO not found at:
  echo %PIO_EXE%
  exit /b 1
)

echo [INFO] Building firmware for %BUILD_ENV%...
"%PIO_EXE%" run -e %BUILD_ENV%
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

if not exist "%SRC_BIN%" (
  echo [ERROR] Firmware binary not found:
  echo %SRC_BIN%
  exit /b 1
)

if not exist "%FW_DIR%" mkdir "%FW_DIR%"

copy /Y "%SRC_BIN%" "%DST_BIN%" >nul
if errorlevel 1 (
  echo [ERROR] Failed to copy firmware to FW\file.bin
  exit /b 1
)

echo [OK] Created:
echo %DST_BIN%
exit /b 0
