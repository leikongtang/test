@echo off
setlocal

set QMAKE=D:\Qt\5.15.2\msvc2019_64\bin\qmake.exe
set JOM=D:\Qt\Tools\QtCreator\bin\jom\jom.exe
set BUILD_DIR=%~dp0build-release

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

"%QMAKE%" "%~dp0ZMotionIoReader.pro" -spec win32-msvc "CONFIG+=release"
if errorlevel 1 exit /b 1

"%JOM%" -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b 1

echo Build succeeded: %BUILD_DIR%\release\ZMotionIoReader.exe
endlocal
