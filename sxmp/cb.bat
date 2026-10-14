@echo off

pushd external\mediapipe


bazel build ^
	-c opt ^
  	--copt=-O3 ^
	--copt=-fomit-frame-pointer ^
	--copt=-ffunction-sections ^
	--copt=-fdata-sections ^
	--linkopt=-Wl,--gc-sections ^
	 ^
	--crosstool_top=//third_party/mingw_cc_toolchain:toolchain ^
	--cpu=x64_windows ^
	--compiler=mingw-gcc ^
	--define MEDIAPIPE_DISABLE_GPU=1 ^
	--repo_env=HERMETIC_PYTHON_VERSION=3.12 ^
	--copt=-DHAVE_PTHREAD ^
	--host_copt=-DHAVE_PTHREAD ^
	--copt=-D_WIN32_WINNT=0x0601 ^
	--host_copt=-D_WIN32_WINNT=0x0601 ^
	--copt=-fno-canonical-system-headers ^
	--reuse_sandbox_directories ^
	@sxmp//:sxmp_be_import_lib

copy /y bazel-bin\external\sxmp\libsxmp_be.dll ..\..\..\build\ > nul
copy /y bazel-bin\external\sxmp\libsxmp_be.if.lib ..\..\..\build\ > nul


popd
