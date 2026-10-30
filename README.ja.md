# Scalix

[English](README.md) | 日本語

[![Build](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml)
[![CodeQL](https://github.com/yy981-jp/Scalix/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/codeql.yml)
[![Doxygen](https://github.com/yy981-jp/Scalix/actions/workflows/doxygen.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/doxygen.yml)

**Scalixは、C++23で開発している、性能を重視した実験的な3Dアバターエンジンです。**

3Dアバターの描画とリアルタイムのモーションキャプチャを組み合わせ、既存のアバター資産を活用できる独自の実行環境を目指しています。

ScalixはUnityやVRChatそのものを再実装するプロジェクトではありません。

> **開発状況：初期開発段階**
>
> 機能やインターフェースは今後変更される可能性があります。現時点では本番環境向けの完成品ではなく、任意のVRChat向けアバター、シェーダー、SDK機能との互換性を保証するものでもありません。

## 主な機能と開発対象

- **3D描画：** SDL3によるウィンドウ・入力管理と、bgfxによる描画。
- **アバターモデル：** glTF形式の読み込み、スケルタルスキニング、ヒューマノイドの姿勢処理。
- **モーションキャプチャ：** MediaPipeを利用した姿勢推定バックエンド（`sxmp`）と、姿勢データを受信する仕組み。
- **アバター物理：** Spring Boneによる揺れ物シミュレーション。
- **アニメーションツール：** Unityの`.anim`ファイルを扱う実験的なインポーター。
- **クロスプラットフォーム開発：** CMakeとNinjaを使用したビルド環境。CIではWindows、Linux、macOSを対象としています。MediaPipeバックエンドのビルドは現在Windows向けです。

これらは開発中の機能を含みます。記載されている形式や機能への完全な対応を意味するものではありません。

## プロジェクト構成

| パス | 役割 |
| --- | --- |
| `engine/` | 描画、アバター読み込み、スケルタルアニメーション、姿勢処理、Spring Boneなどを担うメインエンジン |
| `sxmp/` | MediaPipeを利用したモーションキャプチャバックエンド |
| `importer/` | Unityの`.anim`ファイルを扱う実験的なインポーター（`sxim`） |
| `resources/config/` | デフォルト設定 |
| `assets/` | プロジェクトのアセット用Gitサブモジュール |
| `external/` | 外部ライブラリのソースとビルド環境 |
| `dev/` | 開発用ツール |

## 使用技術

| 技術 | 用途 |
| --- | --- |
| [C++23](https://en.cppreference.com/w/cpp/23) | メインの開発言語 |
| [CMake](https://cmake.org/) / [Ninja](https://ninja-build.org/) | ビルドシステム |
| [SDL3](https://github.com/libsdl-org/SDL) | ウィンドウと入力管理 |
| [bgfx](https://github.com/bkaradzic/bgfx) | 描画APIの抽象化 |
| [tinygltf](https://github.com/syoyo/tinygltf) | glTFモデルの読み込み |
| [OpenCV](https://github.com/opencv/opencv) / [MediaPipe](https://github.com/google-ai-edge/mediapipe) | カメラ入力と姿勢推定 |
| [Asio](https://github.com/chriskohlhoff/asio) | ネットワーク通信 |
| [Protocol Buffers](https://github.com/protocolbuffers/protobuf) | 構造化データのシリアライズ |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) / [nlohmann/json](https://github.com/nlohmann/json) | 設定やデータの処理 |

## ビルド方法

外部ライブラリの一部をソースからビルドするため、初回のビルドには時間とディスク容量が必要です。

### 必要な環境

- Git
- C++23に対応したコンパイラ
- CMake 4.3以降
- Ninja
- Protocol Buffersの開発用ファイルとコンパイラ
- `yaml-cpp`
- SDL3やOpenCVなどのビルドに必要な、各プラットフォームの開発用依存関係

Windowsでは、MSYS2のMinGW-w64 MINGW64環境を使用します。Windows向けモーションキャプチャバックエンドのビルドには、PythonとBazeliskも必要です。プロジェクトのCIではBazel 7.4.1を使用しています。

プラットフォーム別のパッケージ構成は、[setupCENV](.github/actions/setupCENV/action.yml)を参照してください。

### 1. ソースコードの取得

一部のサブモジュールはGitHubのSSH形式のURLを使用しています。SSH認証を使用しない場合は、取得前にURLをHTTPSへ置き換える設定を行います。

```sh
git clone https://github.com/yy981-jp/Scalix.git
cd Scalix
git config url."https://github.com/".insteadOf "git@github.com:"
git submodule update --init --recursive --depth 1
```

### 2. 外部ライブラリのビルド

リポジトリのルートディレクトリで実行します。

```sh
cmake -S external -B external/build -G Ninja
cmake --build external/build --parallel
```

bgfx、SDL3、SDL3_image、OpenCVなどをビルドし、`external/install/`以下に配置します。

現在の外部ビルド設定ではDebug版とRelease版の両方を構築するため、初回のビルドには特に時間がかかる場合があります。

### 3. Windows向けモーションキャプチャバックエンドのビルド

Windowsでは、メインエンジンのビルド前にMediaPipeバックエンドをビルドします。

Bazelisk、Python、MinGW-w64などが利用できるビルド環境を用意したうえで、次のコマンドを実行します。

```bat
cd sxmp
cb.bat
cd ..
```

この手順はWindows向けです。バックエンドのビルド設定は[buildSxmpBackend](.github/actions/buildSxmpBackend/action.yml)を参照してください。

### 4. Scalix本体のビルド

リポジトリのルートディレクトリで実行します。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

主な実行ファイルのターゲット名は`sl`、アニメーションインポーターのターゲット名は`sxim`です。

Windowsでは、必要に応じて次のターゲットを実行すると、実行時に必要なDLLを実行ファイルのディレクトリへコピーできます。

```sh
cmake --build build --target sl_runtime_deps
```

このターゲットはWindows向けです。

最新のプラットフォーム別ビルド手順は、[Build workflow](.github/workflows/build.yml)と[buildProject](.github/actions/buildProject/action.yml)も参照してください。

## ライセンス

Scalixのソースコードは[MIT License](LICENSE)の下で公開されています。

外部ライブラリやアセットには、それぞれ独自のライセンスや利用条件が適用されます。再配布する場合は、各ライセンスと利用条件を確認してください。

---

**READMEの原本について**

このファイル（`README.ja.md`）をREADMEの原本とします。英語版（`README.md`）は本書をもとにした翻訳です。両者の内容に差異がある場合は、日本語版を優先してください。
