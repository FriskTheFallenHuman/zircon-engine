rem Clear gradle caches :(

rem call gradlew clean

set "GRADLE_CACHE_DIR=%userprofile%\.gradle\caches"

cd %GRADLE_CACHE_DIR%
dir
pause

rmdir /s /q %GRADLE_CACHE_DIR%


pause

