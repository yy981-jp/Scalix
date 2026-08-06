@echo off

if "%1" == "" (
	echo %%1 is empty
	exit /b 1
)

set name=%1

call protoc --cpp_out=../engine/generated/ --python_out=../tracker/generated/ --pyi_out=../tracker/generated/ ./proto/%name%.proto
