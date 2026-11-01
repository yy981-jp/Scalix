@echo off
setlocal

set "BAZEL_COMPILER=mingw-gcc"
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "BAZEL_COMPILER=ucrt64-gcc"
echo Using Bazel compiler: %BAZEL_COMPILER%

set "BAZEL_COMMAND=bazel"
if defined BAZEL_EXE set "BAZEL_COMMAND=%BAZEL_EXE%"

set "BAZEL_STARTUP_ARGS="
if defined BAZEL_OUTPUT_BASE set "BAZEL_STARTUP_ARGS=--output_base=%BAZEL_OUTPUT_BASE%"

set "BAZEL_BUILD_ARGS="
if defined BAZEL_DISK_CACHE set "BAZEL_BUILD_ARGS=%BAZEL_BUILD_ARGS% --disk_cache=%BAZEL_DISK_CACHE%"
if defined BAZEL_REPOSITORY_CACHE set "BAZEL_BUILD_ARGS=%BAZEL_BUILD_ARGS% --repository_cache=%BAZEL_REPOSITORY_CACHE%"

echo Using Bazel executable: "%BAZEL_COMMAND%"
if defined BAZEL_OUTPUT_BASE echo Using Bazel output base: %BAZEL_OUTPUT_BASE%
if defined BAZEL_DISK_CACHE echo Using Bazel disk cache: %BAZEL_DISK_CACHE%
if defined BAZEL_REPOSITORY_CACHE echo Using Bazel repository cache: %BAZEL_REPOSITORY_CACHE%

pushd external\mediapipe || exit /b 1

"%BAZEL_COMMAND%" %BAZEL_STARTUP_ARGS% build ^
	--announce_rc ^
	%BAZEL_BUILD_ARGS% ^
	-c opt ^
	--copt=-O3 ^
	--copt=-fomit-frame-pointer ^
	--copt=-ffunction-sections ^
	--copt=-fdata-sections ^
	--linkopt=-Wl,--gc-sections ^
	--crosstool_top=//third_party/mingw_cc_toolchain:toolchain ^
	--cpu=x64_windows ^
	--compiler=%BAZEL_COMPILER% ^
	--define MEDIAPIPE_DISABLE_GPU=1 ^
	--repo_env=HERMETIC_PYTHON_VERSION=3.12 ^
	--copt=-DHAVE_PTHREAD ^
	--host_copt=-DHAVE_PTHREAD ^
	--copt=-D_WIN32_WINNT=0x0601 ^
	--host_copt=-D_WIN32_WINNT=0x0601 ^
	--copt=-no-canonical-prefixes ^
	--copt=-isystem ^
	--copt=external ^
	--copt=-fno-canonical-system-headers ^
	--verbose_failures ^
	--reuse_sandbox_directories ^
	@sxmp//:sxmp_be ^
	@sxmp//:sxmp_be_import_lib

set "BAZEL_EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %BAZEL_EXIT_CODE%
