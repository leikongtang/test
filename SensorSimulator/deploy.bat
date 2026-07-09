@echo off
setlocal
set QTDIR=D:\Qt\5.15.2\msvc2019_64
set EXE=%~dp0build-release\release\SensorSimulator.exe

if not exist "%EXE%" (
    echo 未找到 %EXE%，请先编译 Release 版本。
    exit /b 1
)

"%QTDIR%\bin\windeployqt.exe" --no-translations "%EXE%"
echo 部署完成: %EXE%
endlocal
