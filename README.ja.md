# Scalix

[English](README.md) | 日本語

[![Build](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/build.yml)
[![Release](https://github.com/yy981-jp/Scalix/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/yy981-jp/Scalix/actions/workflows/release.yml)
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
- **アバターモデル：** glTF形式の読み込み、スケルタルスキニング、VRM 1.0のヒューマノイド情報の取得。
- **モーションキャプチャ：** MediaPipeを利用した姿勢推定バックエンド（`sxmp`）と、姿勢データを受信する仕組み。
- **アバター物理：** VRM 1.0の`VRMC_springBone`に基づく揺れ物シミュレーション。球体・カプセルコライダーに対応。
- **UI：** RmlUiとbgfxを組み合わせたUI描画基盤（初期実装）。
- **アニメーションツール：** Unityの`.anim`ファイルを扱う実験的なインポーター。
- **クロスプラットフォーム開発：** CMakeとNinjaを使用したビルド環境。CIではWindows、Linux、macOSを対象とし、MediaPipeバックエンドも各プラットフォームでビルドしています。

VRM 1.0への対応は部分的です。現時点ではヒューマノイド情報と`VRMC_springBone`を中心に実装しており、MToonを含むVRM 1.0の全仕様への対応を意味するものではありません。

RmlUiのbgfx描画バックエンドも初期実装であり、クリップマスクや高度なフィルターなど、一部の描画機能には対応していません。

## プロジェクト構成

| パス | 役割 |
| --- | --- |
| `engine/` | 描画、アバター読み込み、スケルタルアニメーション、姿勢処理、Spring Boneなどを担うメインエンジン |
| `sxmp/` | MediaPipeを利用したモーションキャプチャバックエンド |
| `tracker/` | カメラ入力、トラッキングデータの記録・再生などを行うPythonツール |
| `importer/` | Unityの`.anim`ファイルを扱う実験的なインポーター（`sxim`） |
| `shared/` | エンジンと関連ツールで共有するProtocol Buffersの定義 |
| `ui/` | RmlUi向けのbgfxレンダラー |
| `resources/` | デフォルト設定やUIリソース |
| `assets/` | プロジェクトのアセット用Gitサブモジュール |
| `external/` | 外部ライブラリのソースとビルド環境 |
| `dev/` | 開発用ツール |

## 使用技術・外部ライブラリ

| 技術・ライブラリ | 用途 |
| --- | --- |
| [C++23](https://en.cppreference.com/w/cpp/23) | メインの開発言語 |
| [CMake](https://cmake.org/) / [Ninja](https://ninja-build.org/) | ビルドシステム |
| [SDL3](https://github.com/libsdl-org/SDL) / [SDL3_image](https://github.com/libsdl-org/SDL_image) | ウィンドウ、入力、画像読み込み |
| [bgfx](https://github.com/bkaradzic/bgfx) / [bx](https://github.com/bkaradzic/bx) / [bimg](https://github.com/bkaradzic/bimg) / [bgfx.cmake](https://github.com/bkaradzic/bgfx.cmake) | 描画APIの抽象化と関連ライブラリ |
| [RmlUi](https://github.com/mikke89/RmlUi) / [FreeType](https://freetype.org/) | UI基盤とフォント処理 |
| [tinygltf](https://github.com/syoyo/tinygltf) / [VRM.h](https://github.com/infosia/VRM.h) | glTFおよびVRM 1.0の読み込み |
| [OpenCV](https://github.com/opencv/opencv) / [MediaPipe](https://github.com/google-ai-edge/mediapipe) | カメラ入力と姿勢推定 |
| [Asio](https://github.com/chriskohlhoff/asio) | ネットワーク通信 |
| [Protocol Buffers](https://github.com/protocolbuffers/protobuf) | 姿勢データなどのシリアライズ |
| [CLI11](https://github.com/CLIUtils/CLI11) | コマンドライン引数の処理 |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) / [nlohmann/json](https://github.com/nlohmann/json) | 設定やJSONデータの処理 |
| [y9inc](https://github.com/yy981-jp/tools) | 共通ユーティリティ |

一覧には、現在のコードまたはビルド設定で利用を確認できるライブラリを掲載しています。FreeTypeはRmlUiのフォントエンジンとしてシステム側に必要です。サブモジュールとして登録されているだけで、現在のビルドに組み込まれていないものは実行時依存として扱っていません。

## ビルド方法

外部ライブラリの一部をソースからビルドするため、初回のビルドには時間とディスク容量が必要です。

### 必要な環境

- Git
- C++23に対応したコンパイラ
- CMake 4.3以降
- Ninja
- Protocol Buffersの開発用ファイルとコンパイラ（`protoc`）
- `yaml-cpp`
- FreeTypeの開発用ファイル
- SDL3、SDL3_image、OpenCVなどのビルドに必要な、各プラットフォームの開発用依存関係

Windowsでは、MSYS2のUCRT64環境を使用します。

MediaPipeバックエンドのビルドにはPython 3.12とBazeliskが必要です。プロジェクトのCIではBazelisk 1.29.0を使用し、Bazel 7.4.1でバックエンドをビルドしています。`tracker/`のPythonツールを使用する場合は、`tracker/requirements.txt`に記載されたパッケージも必要です。

プラットフォーム別のパッケージ構成は、[setupCENV](.github/actions/setupCENV/action.yml)を参照してください。

### 1. ソースコードの取得

一部のサブモジュールはGitHubのSSH形式のURLを使用しています。SSH認証を使用しない場合は、取得前にURLをHTTPSへ置き換える設定を行います。

~~~sh
git clone https://github.com/yy981-jp/Scalix.git
cd Scalix
git config url."https://github.com/".insteadOf "git@github.com:"
git submodule update --init --recursive --depth 1
~~~

### 2. 外部ライブラリのビルド

リポジトリのルートディレクトリで実行します。

~~~sh
cmake -S external -B external/build -G Ninja
cmake --build external/build --parallel
~~~

bgfx、RmlUi、SDL3、SDL3_image、OpenCVなどをビルドし、`external/install/`以下に配置します。

現在の外部ビルド設定ではDebug版とRelease版の両方を構築するため、初回のビルドには特に時間がかかる場合があります。

### 3. モーションキャプチャバックエンドのビルド

メインエンジンのビルド前に、MediaPipeバックエンド（`sxmp_be`）をビルドします。Windows、Linux、macOSのすべてでBazelを使用します。

Windowsでは、MSYS2 UCRT64環境で次のコマンドを実行します。

~~~bat
cd sxmp
cb.bat
cd ..
~~~

Linux・macOSでは、リポジトリのルートディレクトリから次のコマンドを実行します。

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

プラットフォーム固有の追加オプションや出力ファイルの配置については、[buildSxmpBackend](.github/actions/buildSxmpBackend/action.yml)を参照してください。

### 4. Scalix本体のビルド

リポジトリのルートディレクトリで実行します。

~~~sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
~~~

主な実行ファイルのターゲット名は`sl`、アニメーションインポーターのターゲット名は`sxim`です。

Windowsでは、必要に応じて次のターゲットを実行すると、実行時に必要なDLLを実行ファイルのディレクトリへコピーできます。

~~~sh
cmake --build build --target sl_runtime_deps
~~~

このターゲットはWindows向けです。

最新のプラットフォーム別ビルド手順は、[Build workflow](.github/workflows/build.yml)と[buildProject](.github/actions/buildProject/action.yml)も参照してください。

## ライセンス

Scalixのソースコードは[MIT License](LICENSE)の下で公開されています。

外部ライブラリやアセットには、それぞれ独自のライセンスや利用条件が適用されます。再配布する場合は、各ライセンスと利用条件を確認してください。

---

**READMEの原本について**

このファイル（`README.ja.md`）をREADMEの原本とします。英語版（`README.md`）は本書をもとにした翻訳です。両者の内容に差異がある場合は、日本語版を優先してください。
