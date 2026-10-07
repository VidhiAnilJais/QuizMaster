@echo off
REM ---------------------------------------------------------------------------
REM  QuizMaster - one click build for Windows (MinGW-w64 / MSYS2 g++)
REM  Double click this file, or run:  build.bat
REM ---------------------------------------------------------------------------

setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo.
    echo [ERROR] g++ was not found in PATH.
    echo         Install MinGW-w64 and add its bin folder to PATH, then try again.
    echo.
    pause
    exit /b 1
)

if not exist build mkdir build

echo.
echo  Building QuizMaster ...
echo.

g++ -std=c++17 -Wall -Wextra -Iinclude ^
    src\main.cpp ^
    src\UI.cpp ^
    src\FileStore.cpp ^
    src\User.cpp ^
    src\Student.cpp ^
    src\Admin.cpp ^
    src\Question.cpp ^
    src\Quiz.cpp ^
    src\Result.cpp ^
    -o build\QuizMaster.exe

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed - please read the messages above.
    echo.
    pause
    exit /b 1
)

echo.
echo  Build finished:  build\QuizMaster.exe
echo  Run it with:     build\QuizMaster.exe
echo.
pause
endlocal