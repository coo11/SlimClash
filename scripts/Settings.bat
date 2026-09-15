SETLOCAL enabledelayedexpansion
@ECHO off
MODE con lines=9 cols=56
COLOR 0A
CD /D %~dp0
TITLE Clash Settings
:HEAD
ECHO  0. EnableLoopback.exe
ECHO  1. 设置开机启动
ECHO  2. 取消开机启动
ECHO  3. 设为 IE 代理
ECHO  4. 取消设为 IE 代理
ECHO  5. 排除全部 UWP 应用 Loopback 限制
ECHO  6. 恢复全部 UWP 应用 Loopback 限制
ECHO  ======================================================
:: 选择菜单
:: Use xcopy to retrieve the key press: https://stackoverflow.com/a/27257111/14168341
<nul set /p ".=请输入 0-6 选择，其它键退出："
SET "choix=" & for /f "delims=" %%a in ('xcopy /l /w "%~f0" "%~f0" 2^>nul') DO IF not defined choix set "choix=%%a"
SET "choix=%choix:~-1%"
FOR %%i in ( 1 2 3 4 5 6 ) DO IF %choix%==%%i ECHO %choix% && TIMEOUT /NOBREAK /T 1 >NUL
CLS
ECHO.
IF /i "%choix%"=="0" "%~dp0EnableLoopback.exe" && GOTO HEAD
IF /i "%choix%"=="1" GOTO STARTUP
IF /i "%choix%"=="2" GOTO NOSTARTUP
IF /i "%choix%"=="3" GOTO PROXY
IF /i "%choix%"=="4" GOTO NOPROXY
IF /i "%choix%"=="5" GOTO UNBLOCK
IF /i "%choix%"=="6" GOTO BLOCK
EXIT

:STARTUP
REG ADD "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "Clash" /t REG_SZ /d "\"%~dp0RunSilence.vbs\"" /f
GOTO BACK

:NOSTARTUP
REG delete "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "Clash"  /f
GOTO BACK

:PROXY
CD /D %~dp0
SET "config_path=.clash\config.yaml"
FOR /F "tokens=1,* delims= " %%a in ('findstr "^port:" %config_path%') do set l=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^socks-port:" %config_path%') do set m=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^mixed-port:" %config_path%') do set n=%%b
ECHO 获取配置文件中的 HTTP(S) 端口为 %l%，SOCKS 端口为 %m%，混合端口为 %n%。
SET i="HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings"
SETX http_proxy http://127.0.0.1:%n%
SETX https_proxy http://127.0.0.1:%n%
REG ADD %i% /v "ProxyEnable" /t  REG_DWORD /d 1 /f >NUL 2>NUL
REG ADD %i% /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "127.0.0.1:%n%" /f >NUL 2>NUL
REG ADD %i% /v "ProxyOverride" /t REG_SZ /d >NUL 2>NUL "localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*" /f >NUL 2>NUL
ECHO.
ECHO IE代理设置完毕
GOTO BACK

:NOPROXY
SETX http_proxy ""
SETX https_proxy ""
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyEnable" /t  REG_DWORD /d "0" /f >nul
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyServer" /t  REG_SZ /d "" /f >nul
IPCONFIG /flushdns
ECHO IE代理已关闭
GOTO BACK

:UNBLOCK
FOR /F "tokens=11 delims=\" %%p IN ('REG QUERY "HKCU\Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppContainer\Mappings"') DO CheckNetIsolation.exe LoopbackExempt -a -p=%%p
GOTO BACK

:BLOCK
CheckNetIsolation.exe LoopbackExempt -c
GOTO BACK

:BACK
TIMEOUT /T 2 >NUL
CD /D %~dp0
CLS
GOTO HEAD
