# Scalix

[English](README.md) | [日本語](README.ja.md)

[![Build](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml)
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
- **Avatar models:** glTF loading, skeletal skinning, and humanoid pose processing.
- **Motion capture:** A MediaPipe-based pose-estimation backend (`sxmp`) and a runtime pose-frame receiver.
- **Avatar physics:** Spring-bone simulation.
- **Animation tools:** An experimental importer for Unity `.anim` files.
- **Cross-platform development:** A CMake/Ninja build system with CI targeting Windows, Linux, and macOS. The MediaPipe backend is currently built for Windows.

These areas include features that are still under development. Their inclusion does not imply complete support for every related format or feature.

## Project Structure

| Path | Responsibility |
| --- | --- |
| `engine/` | Main engine: rendering, avatar loading, skeletal animation, pose processing, and spring bones |
| `sxmp/` | MediaPipe-based motion-capture backend |
| `importer/` | Experimental importer for Unity `.anim` files (`sxim`) |
| `resources/config/` | Default configuration |
| `resources/assets/` | Git submodule containing project assets |
| `external/` | Third-party source dependencies and build configuration |
| `dev/` | Development tools |

## Technology

| Technology | Role |
| --- | --- |
| [C++23](https://en.cppreference.com/w/cpp/23) | Core language |
| [CMake](https://cmake.org/) / [Ninja](https://ninja-build.org/) | Build system |
| [SDL3](https://github.com/libsdl-org/SDL) | Window and input management |
| [bgfx](https://github.com/bkaradzic/bgfx) | Rendering abstraction |
| [tinygltf](https://github.com/syoyo/tinygltf) | glTF model loading |
| [OpenCV](https://github.com/opencv/opencv) / [MediaPipe](https://github.com/google-ai-edge/mediapipe) | Camera input and pose estimation |
| [Asio](https://github.com/chriskohlhoff/asio) | Networking |
| [Protocol Buffers](https://github.com/protocolbuffers/protobuf) | Structured data serialization |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) / [nlohmann/json](https://github.com/nlohmann/json) | Configuration and data processing |

## Building

Some third-party dependencies are built from source, so the initial build requires additional time and disk space.

### Prerequisites

- Git
- A compiler supporting C++23
- CMake 4.3 or newer
- Ninja
- Protocol Buffers development files and compiler
- `yaml-cpp`
- Platform-specific development dependencies required by SDL3, OpenCV, and other components

On Windows, use the MinGW-w64 toolchain in an MSYS2 MINGW64 environment. Building the Windows motion-capture backend also requires Python and Bazelisk. The project's CI uses Bazel 7.4.1.

For platform-specific package setup, see [`setupCENV`](.github/actions/setupCENV/action.yml).

### 1. Get the Source

Some submodules use SSH-form GitHub URLs. If you do not use SSH authentication, configure Git to rewrite those URLs to HTTPS before initializing the submodules.

```sh
git clone https://github.com/yy981-jp/Scalix.git
cd Scalix
git config url."https://github.com/".insteadOf "git@github.com:"
git submodule update --init --recursive --depth 1
```

### 2. Build Third-Party Dependencies

Run these commands from the repository root:

```sh
cmake -S external -B external/build -G Ninja
cmake --build external/build --parallel
```

This builds dependencies including bgfx, SDL3, SDL3_image, and OpenCV, and installs them under `external/install/`.

The current external build configuration builds both Debug and Release variants, so the initial build may take some time.

### 3. Build the Motion-Capture Backend on Windows

On Windows, build the MediaPipe backend before building the main engine.

Set up a build environment with Bazelisk, Python, and MinGW-w64 available, then run:

```bat
cd sxmp
cb.bat
cd ..
```

This step applies to Windows. See [`buildSxmpBackend`](.github/actions/buildSxmpBackend/action.yml) for the backend build configuration.

### 4. Build Scalix

Run these commands from the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The main runtime target is `sl`, and the animation importer target is `sxim`.

On Windows, you can optionally run the following target to copy runtime DLL dependencies alongside the executable:

```sh
cmake --build build --target sl_runtime_deps
```

This target is available on Windows.

For the latest platform-specific build procedure, also see the [Build workflow](.github/workflows/build.yml) and the [`buildProject`](.github/actions/buildProject/action.yml) composite action.

## License

Scalix source code is released under the [MIT License](LICENSE).

Third-party dependencies and assets are subject to their own licenses and terms. Review those terms before redistributing them.

---

**About this README**

The Japanese version (`README.ja.md`) is the canonical source of truth for this README. This English version (`README.md`) is a translation of the Japanese version. If the two versions differ, please refer to the Japanese version.
