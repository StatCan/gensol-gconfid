@echo off
REM set_msvc_path <out-var-name> [year:2019,2022] [edition:enterprise,professional,buildtools] [platform:64,32]
REM   generates a path to the msvc setup batch file, vcvars*.bat, returns in output parameter <out-var-name>
REM   checks file existence: returns 0 if exists, 1 otherwise
REM
REM   example: call set_msvc_path THE_PATH 2019 buildtools 64
REM            echo %THE_PATH%
REM   Debugging: define %DEBUG% to have extra info printed to the console

@REM set default values
set DS=\
set "PROG_ARCH=Program Files (x86)"
set YEAR=2019
set EDITION=enterprise
set PLATFORM=64

@REM check for mandatory parameters
if "%~1"=="" (
    echo ERROR: output variable name required
    exit /b 1
)

@REM override defaults if specified
if NOT "%~2"=="" (
    set YEAR=%~2
    if NOT "%~3"=="" (
        set EDITION=%~3
        if NOT "%~4"=="" (
            set PLATFORM=%~4
            if NOT "%~5"=="" (
                set "PROG_ARCH=%~5"
            )
        )
    )
)

@REM generate path and set output variable value
set BASE_PATH=C:%DS%%PROG_ARCH%%DS%Microsoft Visual Studio%DS%
set vcvars_path=%BASE_PATH%%YEAR%%DS%%EDITION%%DS%VC%DS%Auxiliary%DS%Build%DS%vcvars%PLATFORM%.bat
set "%~1=%vcvars_path%"

REM diagnostic code, to enable: define DEBUG with set debug=foo
@REM set output variable
if NOT "%DEBUG%"=="" (
    call :print_debug_info
) else (
    if NOT "%PRINT_SEARCH_PATH%"=="" (
        call :print_search_path
    )
)

@REM return 0 if exists, 1 if not
if EXIST "%vcvars_path%" (
    exit /b 0
) else (
    exit /b 1
)

@REM this should never be reached
exit /b 2

@REM print internal variables
:print_debug_info
    echo.
    echo setting path with:
    echo   ^<directory separator^>    = %DS%
    echo   BASE_PATH                = %BASE_PATH%
    echo   YEAR                     = %YEAR%
    echo   EDITION                  = %EDITION%
    echo   PLATFORM                 = %PLATFORM%
    echo result: %vcvars_path%
    exit /b 0

:print_search_path
    echo Searching "%vcvars_path%"
    exit /b 0
