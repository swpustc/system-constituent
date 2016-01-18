@ECHO OFF
CHCP 65001
REM Find project name
SET "NAME=%~dp0"
SET "OUT=system"
FOR /F "tokens=*" %%I IN ('CALL "%~dp0\run_bash" "%~dp0\base_name.sh" "%NAME:~0,-8%"') DO (
  SET "NAME=%%I"
)
REM Git fetch
IF     EXIST "E:\project\runner\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_fetch.sh" "%~dp0.." "E:\project\runner\%NAME%"
REM Git clone
IF NOT EXIST "E:\project\runner\%NAME%" CALL "%~dp0\run_bash" "%~dp0\git_clone.sh" "E:\project\runner\%NAME%"
REM Remove output files
DEL /F /A /Q "E:\project\runner\master\bin\Release\x64\%OUT%*" >NUL 2>&1
REM Build
IF EXIST "E:\project\runner\%NAME%\install.bat" CALL "E:\project\runner\%NAME%\install.bat"
CALL "E:\project\runner\%NAME%\build_release_x64.bat"
REM Test
IF EXIST "E:\project\runner\master\bin\Release\x64\%OUT%*.exe" EXIT /B 0
IF EXIST "E:\project\runner\master\bin\Release\x64\%OUT%*.dll" EXIT /B 0
EXIT /B 1
