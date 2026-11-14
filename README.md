# Scalix

[English](README.md) | [日本語](README.ja.md)

[![Build](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml)
[![Release](https://github.com/yy981-jp/Scalix/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/release.yml)
[![CodeQL](https://github.com/yy981-jp/Scalix/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/codeql.yml)
[![Doxygen](https://github.com/yy981-jp/Scalix/actions/workflows/doxygen.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/doxygen.yml)

**Scalix is an experimental, performance-focused 3D avatar engine written in C++23.**

The goal is to create an independent runtime for rendering 3D avatars and driving them with real-time motion-capture input, with a focus on making existing avatar assets usable in a custom engine.

Scalix is not a recreation of Unity or VRChat.

> **Project status: early development**
>
> Features and interfaces may change. Scalix is not production-ready, and compatibility with arbitrary VRChat avatars, shaders, or SDK features is not guaranteed.

## Features and Development Scope

- **3D rendering:** Window and input management with SDL3, and rendering with bgfx.
- **Avatar models:** glTF loading, skeletal skinning, and access to VRM 1.0 humanoid information.
- **Motion capture:** A MediaPipe-based pose-estimation backend (`sxmp`) and a receiver for pose data.
- **Avatar physics:** VRM 1.0 `VRMC_springBone`-based simulation, including sphere and capsule colliders.
- **UI:** An early-stage UI rendering foundation combining RmlUi with bgfx.
- **Animation tools:** An experimental importer for Unity `.anim` files.
- **Cross-platform development:** A CMake/Ninja build system with CI targeting Windows, Linux, and macOS. The MediaPipe backend is also built for all three platforms.

VRM 1.0 support is partial. The current implementation focuses on humanoid information and `VRMC_springBone`; it does not imply complete support for every part of the VRM 1.0 specification, including MToon.

The RmlUi bgfx renderer is also an early implementation. Some rendering features, such as clip masks and advanced filters, are not supported yet.

## Project Structure

| Path | Responsibility |
| --- | --- |
| `engine/` | Main engine: rendering, avatar loading, skeletal animation, pose processing, and spring bones |
| `sxmp/` | MediaPipe-based motion-capture backend |
| `tracker/` | Python tools for camera input and recording or replaying tracking data |
| `importer/` | Experimental importer for Unity `.anim` files (`sxim`) |
| `shared/` | Protocol Buffers definitions shared by the engine and related tools |
| `ui/` | bgfx renderer for RmlUi |
| `resources/` | Default configuration and UI resources |
| `assets/` | Git submodule containing project assets |
| `external/` | Third-party source dependencies and build configuration |
| `dev/` | Development tools |

## Technology and Third-Party Libraries

| Technology / Library | Role |
| --- | --- |
| [C++23](https://en.cppreference.com/w/cpp/23) | Core language |
| [CMake](https://cmake.org/) / [Ninja](https://ninja-build.org/) | Build system |
| [SDL3](https://github.com/libsdl-org/SDL) / [SDL3_image](https://github.com/libsdl-org/SDL_image) | Windowing, input, and image loading |
| [bgfx](https://github.com/bkaradzic/bgfx) / [bx](https://github.com/bkaradzic/bx) / [bimg](https://github.com/bkaradzic/bimg) / [bgfx.cmake](https://github.com/bkaradzic/bgfx.cmake) | Rendering abstraction and related libraries |
| [RmlUi](https://github.com/mikke89/RmlUi) / [FreeType](https://freetype.org/) | UI framework and font processing |
| [tinygltf](https://github.com/syoyo/tinygltf) / [VRM.h](https://github.com/infosia/VRM.h) | glTF and VRM 1.0 loading |
| [OpenCV](https://github.com/opencv/opencv) / [MediaPipe](https://github.com/google-ai-edge/mediapipe) | Camera input and pose estimation |
| [Asio](https://github.com/chriskohlhoff/asio) | Networking |
| [Protocol Buffers](https://github.com/protocolbuffers/protobuf) | Serialization of pose data and other structured data |
| [CLI11](https://github.com/CLIUtils/CLI11) | Command-line argument parsing |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) / [nlohmann/json](https://github.com/nlohmann/json) | Configuration and JSON processing |
| [y9inc](https://github.com/yy981-jp/tools) | Shared utility library |

This list covers libraries confirmed to be used by the current code or build configuration. FreeType is required as the system font engine for RmlUi. Submodules that are registered in Git but are not currently included in the build are not listed as runtime dependencies.

## Building

Some third-party dependencies are built from source, so the initial build requires additional time and disk space.

### Prerequisites

- Git
- A compiler supporting C++23
- CMake 4.3 or newer
- Ninja
- Protocol Buffers development files and compiler (`protoc`)
- `yaml-cpp`
- FreeType development files
- Platform-specific development dependencies required by SDL3, SDL3_image, OpenCV, and other components

On Windows, use the MSYS2 UCRT64 environment.

Building the MediaPipe backend requires Python 3.12 and Bazelisk. The project CI uses Bazelisk 1.29.0 and builds the backend with Bazel 7.4.1. The Python tools under `tracker/` also require the packages listed in `tracker/requirements.txt`.

For platform-specific package setup, see [setupCENV](.github/actions/setupCENV/action.yml).

### 1. Get the Source

Some submodules use SSH-form GitHub URLs. If you do not use SSH authentication, configure Git to rewrite those URLs to HTTPS before initializing the submodules.

~~~sh
git clone https://github.com/yy981-jp/Scalix.git
cd Scalix
git config url."https://github.com/".insteadOf "git@github.com:"
git submodule update --init --recursive --depth 1
~~~

### 2. Build Third-Party Dependencies

Run these commands from the repository root:

~~~sh
cmake -S external -B external/build -G Ninja
cmake --build external/build --parallel
~~~

This builds dependencies including bgfx, RmlUi, SDL3, SDL3_image, and OpenCV, and installs them under `external/install/`.

The current external build configuration builds both Debug and Release variants, so the initial build may take some time.

### 3. Build the Motion-Capture Backend

Build the MediaPipe backend (`sxmp_be`) before building the main engine. Bazel is used on Windows, Linux, and macOS.

On Windows, run the following commands in the MSYS2 UCRT64 environment:

~~~bat
cd sxmp
cb.bat
cd ..
~~~

On Linux and macOS, run the following commands from the repository root:

~~~sh
cd sxmp/external/mediapipe
USE_BAZEL_VERSION=7.4.1 bazel build \
	-c opt \
	--copt=-O3 \
	--copt=-fomit-frame-pointer \
	--copt=-ffunction-sections \
	--copt=-fdata-sections \
	--copt=-fvisibility=hidden \
	--define MEDIAPIPE_DISABLE_GPU=1 \
	--repo_env=HERMETIC_PYTHON_VERSION=3.12 \
	--reuse_sandbox_directories \
	@sxmp//:sxmp_be
cd ../../..
~~~

For platform-specific additional options and runtime output placement, see [buildSxmpBackend](.github/actions/buildSxmpBackend/action.yml).

### 4. Build Scalix

Run these commands from the repository root:

~~~sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
~~~

The main runtime target is `sl`, and the animation importer target is `sxim`.

On Windows, you can optionally run the following target to copy runtime DLL dependencies alongside the executable:

~~~sh
cmake --build build --target sl_runtime_deps
~~~

This target is available on Windows.

For the latest platform-specific build procedure, also see the [Build workflow](.github/workflows/build.yml) and the [buildProject](.github/actions/buildProject/action.yml) composite action.

## License

Scalix source code is released under the [MIT License](LICENSE).

Third-party dependencies and assets are subject to their own licenses and terms. Review those terms before redistributing them.

---

**About this README**

The Japanese version (`README.ja.md`) is the canonical source of truth for this README. This English version (`README.md`) is a translation of the Japanese version. If the two versions differ, please refer to the Japanese version.
