@echo off
cd /d "%~dp0"

if not exist build mkdir build

C:\msys64\ucrt64\bin\gcc.exe ^
src\main.c ^
src\piece.c ^
src\board.c ^
src\ai.c ^
src\game.c ^
src\render.c ^
src\assets.c ^
-Ilib ^
-lraylib ^
-lopengl32 ^
-lgdi32 ^
-lwinmm ^
-o build\dark_chess.exe

if %errorlevel% neq 0 (
    echo.
    echo Build failed.
    pause
    exit /b %errorlevel%
)

echo.
echo Build success.
build\dark_chess.exe