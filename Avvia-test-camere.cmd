@echo off
setlocal

set "PROJECT_DIR=%~dp0."
set "BUILD_DIR=%PROJECT_DIR%\out\build-vs2026-x86"
set "TEST_CONFIG=%BUILD_DIR%\camera-test.config"

if not exist "%BUILD_DIR%\gameplayfootball.exe" (
  echo Eseguibile non trovato: %BUILD_DIR%\gameplayfootball.exe
  pause
  exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\tools\prepare_camera_test.ps1" -ProjectDirectory "%PROJECT_DIR%"
if errorlevel 1 (
  echo Impossibile creare la configurazione temporanea del test.
  pause
  exit /b 1
)

if /I "%~1"=="--prepare-only" exit /b 0

pushd "%BUILD_DIR%"
gameplayfootball.exe "%TEST_CONFIG%"
set "RESULT=%ERRORLEVEL%"
popd

exit /b %RESULT%
