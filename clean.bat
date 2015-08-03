@echo off
setlocal

if not defined VS120COMNTOOLS goto :explain

call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" amd64

if /i not "%VisualStudioVersion%" == "12.0" goto :explain

set "SLN_DIR=%~dp0\system.sln"
set "VS_VERSION=2013"

devenv.com "%SLN_DIR%" /Clean "Release|x64"
devenv.com "%SLN_DIR%" /Clean "Debug|x64"
devenv.com "%SLN_DIR%" /Clean "Release|Win32"
devenv.com "%SLN_DIR%" /Clean "Debug|Win32"
goto :end

:explain
echo This command prompt environment is not initialized correctly to include
echo awareness of the VS%VS_VERSION% installation.  To run this file in the correct
echo environment, click "Microsoft Visual Studio %VS_VERSION%" then "Visual Studio Tools"
echo then "Visual Studio Command Prompt (%VS_VERSION%)" and run this .bat file from the
echo resulting command window.
echo;

:end
pause
goto :eof
