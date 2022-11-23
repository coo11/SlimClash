@echo off
echo In the new version of Python, you must 
echo add lib file 'api-ms-win-core-path-l1-1-0.dll' to 
echo ensure that the executable file can be run in Windows 7!
echo.
echo https://github.com/nalexandru/api-ms-win-core-path-HACK/releases/latest
echo.
pause
cd /D %~dp0
pyinstaller -F --upx-exclude=vcruntime140.dll --add-binary api-ms-win-core-path-l1-1-0.dll;. --clean config.py
move /Y .\dist\config.exe config.exe
rd /Q /S build dist __pycache__
del *.spec /Q /F