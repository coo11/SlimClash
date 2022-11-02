@ECHO OFF
TASKKILL /F /IM clash.exe >NUL 2>NUL
:: Setting current path as environment variable is important. ↓
CD /D %~dp0
SET "config_path=.clash\config.yaml"
FOR /F "tokens=1,* delims= " %%a in ('findstr "^port:" %config_path%') do set l=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^socks-port:" %config_path%') do set m=%%b
FOR /F "tokens=1,* delims= " %%a in ('findstr "^mixed-port:" %config_path%') do set n=%%b

FOR /f "tokens=1,* delims= " %%a in ('findstr "^external-controller:" %config_path%') do set ec=%%b
SET "ec=%ec:'=%"
SET "ec=%ec:"=%"
ECHO [InternetShortcut] > DASHBOARD.url
ECHO URL="http://%EC%/ui" >> DASHBOARD.url
ECHO IconFile=%~dp0icon.ico >> DASHBOARD.url
ECHO IconIndex=0 >> DASHBOARD.url

POWERSHELL -nop -c "Add-Type -AssemblyName System.Windows.Forms; $global:balloon=New-Object System.Windows.Forms.NotifyIcon; $balloon.Icon=[System.Drawing.Icon]::ExtractAssociatedIcon('icon.ico'); $balloon.BalloonTipIcon=[System.Windows.Forms.ToolTipIcon]::None; $balloon.BalloonTipText='HTTP(S) - %l%; Socks5 - %m%; Mixed - %n%'; $balloon.BalloonTipTitle='Proxy ports confirm'; $balloon.Visible=$true; $balloon.Text='Clash Premuim'; $balloon.ShowBalloonTip(4000); Start-Sleep -s 4; $balloon.Dispose()"
SET "i=HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings"
SETX http_proxy http://127.0.0.1:%n%
SETX https_proxy http://127.0.0.1:%n%
REG ADD "%i%" /v "ProxyEnable" /t  REG_DWORD /d 1 /f >NUL 2>NUL
:: REG ADD %i% /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "http=127.0.0.1:%l%;https=127.0.0.1:%l%;ftp=127.0.0.1:%l%;socks=127.0.0.1:%m%" /f >NUL 2>NUL
:: For Websocket connection, you'd better use mixed port only.
REG ADD "%i%" /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "127.0.0.1:%n%" /f >NUL 2>NUL
REG ADD "%i%" /v "ProxyOverride" /t REG_SZ /d >NUL 2>NUL "localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*" /f >NUL 2>NUL
DEL tmp /Q
%1 mshta vbscript:CreateObject("WScript.Shell").Run("clash.exe -d .clash",0)(Window.close)