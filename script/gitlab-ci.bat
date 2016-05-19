@ECHO OFF
CHCP 65001
REM Find project name
SET "NAME=%~dp0"
SET "OUT=system"
FOR /F "tokens=*" %%I IN ('CALL "%~dp0\run_bash" "%~dp0\base_name.sh" "%NAME:~0,-8%"') DO (
  SET "NAME=%%I"
)
REM Git clone
IF NOT EXIST "E:\project\r\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_clone.sh" master "E:\project\r\%NAME%"
REM Git fetch
IF EXIST "E:\project\r\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_fetch.sh" "%~dp0.." "E:\project\r\%NAME%"
REM Remove output files
DEL /F /A /Q "E:\project\r\master\bin\Release\x64\%OUT%*" >NUL 2>&1
REM Build
IF EXIST "E:\project\r\%NAME%\install.bat" CALL "E:\project\r\%NAME%\install.bat"
CALL "E:\project\r\%NAME%\build_release_x64.bat"
REM Test
IF EXIST "E:\project\r\master\bin\Release\x64\%OUT%*.exe" EXIT /B 0
IF EXIST "E:\project\r\master\bin\Release\x64\%OUT%*.dll" EXIT /B 0
EXIT /B 1
