SETLOCAL enabledelayedexpansion
@ECHO off
MODE con lines=12 cols=56
COLOR 0A
CD /D %~dp0
TITLE Clash Settings
:HEAD
ECHO  1. ���ÿ�������
ECHO  2. ȡ����������
ECHO  3. ��Ϊ IE ����
ECHO  4. ȡ����Ϊ IE ����
ECHO  5. �ų�ȫ�� UWP Ӧ�� Loopback ����
ECHO  6. �ָ�ȫ�� UWP Ӧ�� Loopback ����
ECHO  7. ��ȡ/���� GeoLite
ECHO  8. ��ȡ/���� Dashboard
ECHO  9. ���¶���
ECHO  0. EnableLoopback.exe
ECHO  ======================================================
:: ѡ��˵�
:: Use xcopy to retrieve the key press: https://stackoverflow.com/a/27257111/14168341
<nul set /p ".=������ 0-9 ѡ���������˳���"
SET "choix=" & for /f "delims=" %%a in ('xcopy /l /w "%~f0" "%~f0" 2^>nul') DO IF not defined choix set "choix=%%a"
SET "choix=%choix:~-1%"
FOR %%i in ( 1 2 3 4 5 6 7 8 9 0 ) DO IF %choix%==%%i ECHO %choix% && TIMEOUT /NOBREAK /T 1 >NUL
CLS
ECHO.
IF /i "%choix%"=="1" GOTO STARTUP
IF /i "%choix%"=="2" GOTO NOSTARTUP
IF /i "%choix%"=="3" GOTO PROXY
IF /i "%choix%"=="4" GOTO NOPROXY
IF /i "%choix%"=="5" GOTO UNBLOCK
IF /i "%choix%"=="6" GOTO BLOCK
IF /i "%choix%"=="7" GOTO GOEIP
IF /i "%choix%"=="8" GOTO DASHBOARD
IF /i "%choix%"=="9" GOTO UPDATESUBS
IF /i "%choix%"=="0" "%~dp0.utils\EnableLoopback.exe" && GOTO HEAD
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
ECHO ��ȡ�����ļ��е� HTTP(S) �˿�Ϊ %l%��SOCKS �˿�Ϊ %m%����϶˿�Ϊ %n%��
SET i="HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings"
SETX http_proxy http://127.0.0.1:%n%
SETX https_proxy http://127.0.0.1:%n%
REG ADD %i% /v "ProxyEnable" /t  REG_DWORD /d 1 /f >NUL 2>NUL
REG ADD %i% /v "ProxyServer" /t  REG_SZ /d >NUL 2>NUL "127.0.0.1:%n%" /f >NUL 2>NUL
REG ADD %i% /v "ProxyOverride" /t REG_SZ /d >NUL 2>NUL "localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*" /f >NUL 2>NUL
ECHO.
ECHO IE�����������
GOTO BACK

:NOPROXY
SETX http_proxy ""
SETX https_proxy ""
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyEnable" /t  REG_DWORD /d "0" /f >nul
REG ADD "HKCU\SOFTWARE\MICROSOFT\Windows\CURRENTVERSION\Internet Settings" /v "ProxyServer" /t  REG_SZ /d "" /f >nul
IPCONFIG /flushdns
ECHO IE�����ѹر�
GOTO BACK

:UNBLOCK
FOR /F "tokens=11 delims=\" %%p IN ('REG QUERY "HKCU\Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppContainer\Mappings"') DO CheckNetIsolation.exe LoopbackExempt -a -p=%%p
GOTO BACK

:BLOCK
CheckNetIsolation.exe LoopbackExempt -c
GOTO BACK

:GOEIP
:: GeoLite2-Country.mmdb
:: .tar.gz:
:: Key: uSKs4zivaWMD8N6j(Surge), oeEqpP5QI21N(Clash-Web-Bat)
:: Format: https://download.maxmind.com/app/geoip_download?edition_id=GeoLite2-Country&license_key=${LICENSE_KEY}&suffix=tar.gz
CD "%~DP0.utils\"
:: curl -o gl2c.tar.gz "https://download.maxmind.com/app/geoip_download?edition_id=GeoLite2-Country&license_key=uSKs4zivaWMD8N6j&suffix=tar.gz" && 7za.exe e gl2c.tar.gz && 7za e gl2c.tar "GeoLite2-Country*\GeoLite2-Country.mmdb" -aoa && move /Y GeoLite2-Country.mmdb ..\.clash\Country.mmdb && del gl2c.tar* /F /Q
:: .mmdb:
:: Clash itself: https://github.com/Dreamacro/maxmind-geoip/raw/release/Country.mmdb
:: Hackl0us GeoIP2-CN:  https://github.com/Hackl0us/GeoIP2-CN/raw/release/Country.mmdb
curl -o GeoLite2-Country.mmdb "https://ghproxy.com/raw.githubusercontent.com/Hackl0us/GeoIP2-CN/release/Country.mmdb" && move /Y GeoLite2-Country.mmdb ..\.clash\Country.mmdb
PAUSE
GOTO BACK

:DASHBOARD
CD "%~DP0.utils\"
:: https://github.com/Dreamacro/clash-dashboard/archive/refs/heads/gh-pages.zip
curl -o cd.zip "https://ghproxy.com/github.com/Dreamacro/clash-dashboard/archive/refs/heads/gh-pages.zip" && rd ..\.clash\dashboard /S /Q & 7za x cd.zip -aoa && xcopy .\clash-dashboard-gh-pages ..\.clash\dashboard\ /Y /E && rd .\clash-dashboard-gh-pages /S /Q & del cd.zip /F /Q
PAUSE
GOTO BACK

:UPDATESUBS
ECHO  ������ͨ�����ַ�ʽ���»������ġ�
ECHO  ��ȡ�������Ӻ�
ECHO  ���ڸ�Ŀ¼�´򿪻��½�һ���ı��ļ�
ECHO   Subs.txt��һ����дһ���������ӣ�
ECHO  ����󼴿��ڴ˽��水��������£�
ECHO  �ڽ���Ŀ¼ .utils���ڴ�Ŀ¼��ִ��
ECHO  ���� `qjs config.js [Link1] [Link2]...`
ECHO  ���ɣ�ע�����Ӳ����Կո��������
ECHO  ======================================================
PAUSE
CD "%~DP0.utils\"
qjs config.js
PAUSE
GOTO BACK

:BACK
TIMEOUT /T 2 >NUL
CD /D %~dp0
CLS
GOTO HEAD