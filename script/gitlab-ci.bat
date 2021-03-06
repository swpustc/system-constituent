@ECHO OFF
CHCP 65001
REM Find project name
SET "NAME=%~dp0"
FOR /F "tokens=*" %%I IN ('CALL "%~dp0\run_bash" "%~dp0\base_name.sh" "%NAME:~0,-8%"') DO (
  SET "NAME=%%I"
)

REM Create Directory
IF NOT EXIST "X:\r" MKDIR "X:\r"
REM Git clone
IF NOT EXIST "X:\r\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_clone.sh" master "X:\r\%NAME%"
REM Git fetch
IF     EXIST "X:\r\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_fetch.sh" "%~dp0.." "X:\r\%NAME%"
REM Build
IF EXIST "X:\r\%NAME%\install.bat" CALL "X:\r\%NAME%\install.bat"
CALL "X:\r\%NAME%\build_release_x64.bat" "INLINE"
CALL "X:\r\%NAME%\build_release_win32.bat" "INLINE"
