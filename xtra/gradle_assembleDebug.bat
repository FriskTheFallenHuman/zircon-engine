rem Build

rem call gradlew assembleDebug --warning-mode=all --stacktrace
call gradlew assembleDebug

if %ERRORLEVEL% EQU 0 echo OK

rundll32 user32.dll,MessageBeep

pause
pause

