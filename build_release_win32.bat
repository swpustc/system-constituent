@ECHO OFF
SETLOCAL

IF NOT DEFINED VS120COMNTOOLS GOTO :explain
CALL "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
IF /I NOT "%VisualStudioVersion%" == "12.0" GOTO :explain
FOR /F %%i IN ('DIR /B "%~dp0*.sln"') DO (
  SET "SLN_DIR=%~dp0%%i"
)
SET "VS_VERSION=2013"
SET /A ERR=0
SET /A CUR_ERR=0

IF NOT EXIST "%~dp0..\master\tmp" MKDIR "%~dp0..\master\tmp"
CALL "%~dp0install\run_bash_echo.cmd" "%~dp0git_head.sh" "%~dp0\" Win32 || (ECHO Unmodified. && EXIT /B 0)
CALL devenv.com "%SLN_DIR%" /Build "Release|Win32"
SET /A CUR_ERR=%ERRORLEVEL%
SET /A ERR=ERR+CUR_ERR
GOTO :end

:explain
ECHO This command prompt environment is not initialized correctly to include
ECHO awareness of the VS%VS_VERSION% installation.  To run this file in the correct
ECHO environment, click "Microsoft Visual Studio %VS_VERSION%" then "Visual Studio Tools"
ECHO then "Visual Studio Command Prompt (%VS_VERSION%)" and run this .bat file from the
ECHO resulting command window.
ECHO;
PAUSE
EXIT /B 9

:end
EXIT /B %ERR%
