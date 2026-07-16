@echo off
chcp 65001 > nul
setlocal

set QTDIR=E:\qt\qt\5.14.2\mingw73_64
set MINGW=E:\qt\qt\Tools\mingw730_64
set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%

set PROJECT_DIR=E:\qt\projects\OnlineExamSystem
set BUILD_DIR=%PROJECT_DIR%\build-OnlineExamSystem-Desktop_Qt_5_14_2_MinGW_64_bit-Debug

if exist "%BUILD_DIR%" (
    echo Cleaning old build dir...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Running qmake...
qmake "%PROJECT_DIR%\OnlineExamSystem.pro"
if errorlevel 1 goto error

echo Building...
mingw32-make -j4
if errorlevel 1 goto error

echo.
echo Build OK!
echo Client: %PROJECT_DIR%\build\client\OnlineExamClient.exe
echo Server: %PROJECT_DIR%\build\server\ExamServer.exe
pause
exit /b 0

:error
echo.
echo Build FAILED!
pause
exit /b 1
