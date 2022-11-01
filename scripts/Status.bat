@echo off & title Clash Status
TaskList /FI "IMAGENAME eq clash.exe" /FO LIST
pause > nul