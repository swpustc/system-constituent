@SETLOCAL EnableDelayedExpansion

@FOR %%i IN ("git.exe") DO @SET GIT=%%~$PATH:i
@IF EXIST "%GIT%" @(
  @REM Get the bash executable
  @FOR %%i IN ("bash.exe") DO @SET PATH_BASH=%%~$PATH:i
  @IF NOT EXIST "%PATH_BASH%" @(
    @FOR %%s IN ("%GIT%") DO @SET GIT_DIR=%%~dps
    @FOR %%s IN ("!GIT_DIR!") DO @SET GIT_DIR=!GIT_DIR:~0,-1!
    @FOR %%s IN ("!GIT_DIR!") DO @SET GIT_ROOT=%%~dps
    @FOR %%s IN ("!GIT_ROOT!") DO @SET GIT_ROOT=!GIT_ROOT:~0,-1!
    @FOR /D %%s in ("!GIT_ROOT!\bin\bash.exe") DO @SET PATH_BASH=%%~s
    @IF NOT EXIST "!PATH_BASH!" @SET PATH_BASH=!GIT_ROOT!\bin\bash.exe
) )

@IF     EXIST "%PATH_BASH%" CALL "%PATH_BASH%" %*
@IF NOT EXIST "%PATH_BASH%" CALL               %*

@ENDLOCAL
