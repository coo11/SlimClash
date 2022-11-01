@ECHO OFF
TASKKILL /F /IM clash.exe >NUL 2>NUL
CD /D %~dp0
SET "config_path=.clash\config.yaml"
COPY .clash\config.yaml tmp /y>NUL 2>NUL
FOR /F "tokens=1,* delims= " %%a in ('findstr "^port:" %config_path%') do set l=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^socks-port:" %config_path%') do set m=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^mixed-port:" %config_path%') do set n=%%b
ECHO 获取配置文件中的 HTTP(S) 端口为 %l%，SOCKS 端口为 %m%，混合端口为 %n%。

FOR /f "tokens=1,* delims= " %%a in ('findstr "^external-controller:" %config_path%') do set ec=%%b
SET "ec=%ec:'=%"
SET "ec=%ec:"=%"
ECHO [InternetShortcut] > ".\DASHBOARD.url"
ECHO URL="http://%EC%/ui" >> ".\DASHBOARD.url"
ECHO IconFile=%~dp0icon.ico >> ".\DASHBOARD.url"
ECHO IconIndex=0 >> ".\DASHBOARD.url"

SET "i=HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings"
SETX http_proxy http://127.0.0.1:%n%
SETX https_proxy http://127.0.0.1:%n%
REG ADD "%i%" /v "ProxyEnable" /t  REG_DWORD /d 1 /f >NUL 2>NUL
:: REG ADD %i% /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "http=127.0.0.1:%l%;https=127.0.0.1:%l%;ftp=127.0.0.1:%l%;socks=127.0.0.1:%m%" /f >NUL 2>NUL
:: For Websocket connection, you'd better use mixed port only.
REG ADD "%i%" /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "127.0.0.1:%n%" /f >NUL 2>NUL
REG ADD "%i%" /v "ProxyOverride" /t REG_SZ /d >NUL 2>NUL "localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*" /f >NUL 2>NUL
START clash -d .clash