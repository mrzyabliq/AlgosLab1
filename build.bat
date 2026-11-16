@echo off
cmake -S "%~dp0." -B "%~dp0build"
if errorlevel 1 exit /b %errorlevel%

cmake --build "%~dp0build" --config Release
if errorlevel 1 exit /b %errorlevel%

echo Built: "%~dp0build\Release\dijkstra.exe"
