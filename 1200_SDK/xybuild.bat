@echo off

set PROJECT_ROOT=%CD%

call :add_path %PROJECT_ROOT%\buildtools\prebuilts\win64\python3
call :add_path %PROJECT_ROOT%\buildtools\prebuilts\win64\ninja
call :add_path %PROJECT_ROOT%\buildtools\prebuilts\win64\gcc-arm-none-eabi-8-2019-q3-update-win32\bin
call :add_path %PROJECT_ROOT%\buildtools\prebuilts\win64\cmake\bin

python xybuild_ninja.py %* || goto:error

goto:success

:add_path
    (echo ";%PATH%;" | findstr /I ";%1;" > nul) || set "PATH=%1;%PATH%"
    goto :eof

:success
    exit /B 0

:error
    exit /B -1
