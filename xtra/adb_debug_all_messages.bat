set "LOGTAG=CoreMain"

rem call adb shell am start -a android.intention.action.MAIN -n com.zircon.app/org.libsdl.app.SDLActivity

adb logcat -c
rem "We need to filter this via project tag or something"  Log tag is CoreMain
rem start "New Window" cmd  /k adb logcat -s "%LOGTAG%"
start "New Window" cmd  /k adb logcat 

pause