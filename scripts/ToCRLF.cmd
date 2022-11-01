@ECHO OFF
FOR /F "delims=" %%I IN ('powershell -nop [long]((date^).touniversaltime(^)-[datetime]^'1970-01-01^'^).totalmilliseconds') DO SET UTMS=%%I
TYPE %1 | MORE /P >%UTMS%
MOVE %UTMS% %1