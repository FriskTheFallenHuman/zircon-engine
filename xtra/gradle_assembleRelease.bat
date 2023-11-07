rem Build

rem call gradlew assembleRelease --warning-mode=all --stacktrace
call gradlew assembleRelease

if %ERRORLEVEL% EQU 0 echo OK

rundll32 user32.dll,MessageBeep

pause
pause

