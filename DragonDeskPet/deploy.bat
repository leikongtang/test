@echo off
setlocal
set QT_DIR=D:\Qt\5.15.2\msvc2019_64
set EXE=%~dp0build-release\release\DragonDeskPet.exe

if not exist "%EXE%" (
    echo 请先编译 Release 版本。
    exit /b 1
)

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations "%EXE%"
echo 部署完成，可直接运行: %EXE%
