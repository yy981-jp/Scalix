@echo off

if "%1" == "" (
	echo %%1 is empty
	exit /b 1
)

set name=%1

call protoc --cpp_out=../engine/src/ --python_out=../tracker/ --pyi_out=../tracker/ ./proto/%name%.proto
