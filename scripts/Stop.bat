@ECHO OFF & TITLE Clash Stop
Taskkill /F /IM clash.exe
SETX http_proxy ""
SETX https_proxy ""
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyEnable" /t  REG_DWORD /d "0" /f >NUL
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyServer" /t  REG_SZ /d "" /f >NUL
IPCONFIG /flushdns
DEL DASHBOARD.url /Q >NUL 2>NUL
TIMEOUT /T 3 >NUL