@echo off

pushd external\mediapipe


bazel build -c opt ^
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
  --linkopt=-LC:/msys64/mingw64/lib ^
  --linkopt=-lopencv_geometry ^
  --verbose_failures --subcommands --sandbox_debug ^
  @sxmp//:sxmp

popd
