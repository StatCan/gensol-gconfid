@echo off
echo Copy Debug.vcxproj.user to all projects
echo setting debugger settings

call :copy_debug sensitivity

pause
goto end

:copy_debug

echo Applying debug settings to %1
copy DebugAll.vcxproj.user ..\%1\%1.vcxproj.user
echo.
exit /b 0

:end
exit /b 0