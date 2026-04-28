@echo off

if "%1" == "" (
    echo %%1 is empty
    exit /b 1
)

set name=%1

call build\external\bgfx.cmake\cmake\bgfx\shaderc.exe -f shaders/%name%/vs.sc -o runtime/vs_%name%.bin --type vertex --varyingdef shaders/%name%/varying.def.sc --profile s_5_0 -i external/bgfx/src
call build\external\bgfx.cmake\cmake\bgfx\shaderc.exe -f shaders/%name%/fs.sc -o runtime/fs_%name%.bin --type fragment --varyingdef shaders/%name%/varying.def.sc --profile s_5_0 -i external/bgfx/src

xcopy /y runtime build\runtime\ > nul
