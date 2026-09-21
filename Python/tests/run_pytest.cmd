@echo off

rem ## Change log##
rem ## 2023.09.01  - /PASS no longer runs performance tests
rem ## 2023.10.05  - change the default options to reduce clutter in output
rem ##             - copied from `unit_tests`, simplified
rem ## 2024.10.18  - /auto_pass now excludes marker `m_known_to_fail`
rem ## 2026.07.07  - allow `python_cmd` override

set version=2026.07.07
set "python_cmd_default=python"
if not defined python_cmd (
    set "python_cmd=%python_cmd_default%"
    echo using default `python_cmd=%python_cmd_default%`
    echo to override, `set python_cmd=^<python command^>`
) else (
    echo using `python_cmd=%python_cmd%`
)
set pytest_options_common=--verbose -rA --tb=short
rem --verbose     adds line-per-test "PASSED" or "FAILED"
rem -rA           ensures all captured output is printed and prints line-per-test status
rem --tb=short    reduces traceback to essentially show the call stack
rem               unfortunately --tb=no suppresses console output from failed tests

set pytest_options=--cache-clear %pytest_options_common%
set pytest_options_rerun_fail= %pytest_options_common% --last-failed
set call_pytest_opt=%python_cmd% -m pytest %pytest_options%

if /I "%autoclear%"=="true" cls

call :print_header

rem go to help
if "%1"=="/?" (
    goto :print_usage
)
if /I "%1"=="-h" (
    goto :print_usage
)
if /I "%1"=="/h" (
    goto :print_usage
)
if /I "%1"=="/lf" (
    %python_cmd% -m pytest %pytest_options_rerun_fail%
    goto :the_end
)
if /I "%1"=="/pass" (
    %call_pytest_opt% -m "(m_pass or m_pass_as_is)"
    goto :the_end
)
if /I "%1"=="/auto_pass" (
    %call_pytest_opt% -m "(m_auto_pass and not m_known_to_fail)"
    goto :exit_return_errorlevel
)
if /I "%1"=="/auto_external" (
    %call_pytest_opt% -m "((m_auto_pass or m_external_build_only) and not m_known_to_fail and not m_internal_build_only)"
    goto :exit_return_errorlevel
)
if /I "%1"=="/perf" (
    setlocal
    set GCONFID_DEBUG_STATS=true
    if /I "%2"=="" (
        %call_pytest_opt% -m "(m_performance_test)"
    ) else (
        %call_pytest_opt% %2 %3 %4 %5
    )
    endlocal
    goto :the_end
)

rem run pytest
if "%1"=="" (
    echo running pytest on the current directory
    %call_pytest_opt% .
) else (
    echo running pytest on %1
    %call_pytest_opt% %*
)

rem helper labels
goto :the_end
:print_usage
echo.
echo RUN_PYTEST ^[path^] ^[filename^] ^[additional PyTest Options^]
echo RUN_PYTEST ^<selection-flag^>
echo.
echo Description: 
echo     Runs Python's PyTest module, applying default options and easing test selection.
echo     Default Options: %pytest_options%
echo.
echo     Searches the current directory using standard test discovery protocols.
echo     Specify a directory or file.  
echo     Provide any valid PyTest option.  
echo     Use a "Selection Flag" ^(see below^).
echo.
echo Selection Flags:
echo     /PASS                   only run tests that 
echo                             are expected to pass
echo                             EXCEPT performance tests
echo.
echo     /AUTO_PASS              run non-performance tests that
echo                             are expected to pass and do 
echo                             not require SAS generated data
echo                             ^(ideal for automated testing^)
echo.
echo     /PERF ^[test spec^]       only run performance tests
echo                             use ^[test spec^] to specify a 
echo                             particular test
echo.
echo     /LF                     run using  pytest's 
echo                             --last-failed option 
echo.
echo Examples:
echo     RUN_PYTEST /PASS
echo     RUN_PYTEST -k test_DonorA -s
echo     RUN_PYTEST test_donorP01.py
goto :the_end

:print_header
echo run_pytest.cmd - version:%version%
echo.
exit /b

:run_pytest
%call_pytest_opt%
goto :the_end

:exit_return_errorlevel
exit /B %ERRORLEVEL%

:the_end
IF %0 == "%~0" set PAUSE_AT_END=true
IF "%PAUSE_AT_END%" == "true" PAUSE