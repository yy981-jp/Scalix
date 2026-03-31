@echo off

if "%1" == "" (
    echo %%1 is empty
    exit /b 1
)

set name=%1

call build\external\bgfx.cmake\cmake\bgfx\shaderc.exe -f shaders/vs_%name%.sc -o runtime/vs_%name%.bin --type vertex --varyingdef shaders/varying.def.sc --profile s_5_0 -i external/bgfx/src
call build\external\bgfx.cmake\cmake\bgfx\shaderc.exe -f shaders/fs_%name%.sc -o runtime/fs_%name%.bin --type fragment --varyingdef shaders/varying.def.sc --profile s_5_0 -i external/bgfx/src

xcopy /y runtime build\runtime\ > nul
