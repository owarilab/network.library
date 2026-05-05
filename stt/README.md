# myproj — whisper.cpp ライブラリ統合デモ

whisper.cpp の C/C++ ライブラリを自身のプログラムに組み込むためのミニマルな実装例です。

## 概要

CMake でビルドし、whisper.cpp と ggml の共有ライブラリ (`libwhisper.so`, `libggml-*.so`) にリンクすることで、音声からテキストへの変換（自動音声認識: ASR）を C++ プログラム内で行えます。CUDA GPU 加速にも対応しています。

## 環境

| 要素 | バージョン |
|------|-----------|
| OS | Linux (WSL2) |
| Compiler | GCC 13.3.0 |
| CUDA Toolkit | 13.2 |
| GPU | RTX 4060 (Compute 8.9) + RTX 5060 Ti (Compute 12.0) |

## ディレクトリ構成

```
stt/
├── main.cpp               # メインプログラム（whisper API の使用例）
├── CMakeLists.txt         # ビルド設定
├── headers/               # whisper.cpp ヘッダファイル
│   ├── whisper.h          # whisper C API ヘッダ
│   ├── ggml.h             # ggml テンソルライブラリ
│   ├── common-sdl.h/.cpp  # SDL2 オーディオキャプチャ
│   └── ...
├── libs/                  # whisper.cpp の共有ライブラリ
│   ├── libwhisper.so*
│   ├── libggml-base.so*
│   ├── libggml-cpu.so*
│   ├── libggml-cuda.so*   # (CUDA対応環境のみ)
│   └── ...
├── models/                # モデルファイル（.bin）
├── build/                 # ビルド出力ディレクトリ
└── README.md
```

## セットアップ

### 前提条件

このプロジェクトをビルド・実行するには、whisper.cpp をあらかじめビルドし、必要なライブラリを `libs/` ディレクトリにコピーする必要があります。

### whisper.cpp のビルド

whisper.cpp リポジトリから CUDA サポート付きで共有ライブラリをビルドします：

```bash
# whisper.cpp リポジトリをクローン（未実施の場合）
cd ~
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp

# CUDA サポート付きでビルド
mkdir -p build
cd build
cmake .. -DGGML_CUDA=1 -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

# ビルド完了確認
ls -la src/libwhisper.so* ggml/src/libggml*.so*
```

**注**: `-DBUILD_SHARED_LIBS=ON` が重要です。この設定がないと共有ライブラリが生成されません。

### ライブラリのコピー

ビルド完了後、stt プロジェクトの `libs/` ディレクトリに共有ライブラリをコピーします：

```bash
# stt ディレクトリに移動
cd /path/to/network.library/stt

# libs ディレクトリを作成
mkdir -p libs

# whisper ライブラリをコピー
cp -dP ~/whisper.cpp/build/src/libwhisper.so* libs/

# ggml ライブラリをコピー
cp -dP ~/whisper.cpp/build/ggml/src/libggml.so* libs/
cp -dP ~/whisper.cpp/build/ggml/src/libggml-base.so* libs/
cp -dP ~/whisper.cpp/build/ggml/src/libggml-cpu.so* libs/

# CUDA ライブラリをコピー（CUDA対応環境のみ）
cp -dP ~/whisper.cpp/build/ggml/src/ggml-cuda/libggml-cuda.so* libs/

# コピー確認
ls -lh libs/
```

**ポイント**: `-dP` オプションでシンボリックリンクを保持してコピーしています。

## モデルをダウンロード（small が推奨）
cd ~/whisper.cpp/models
bash download-ggml-model.sh large
bash download-ggml-model.sh medium
bash download-ggml-model.sh small
bash download-ggml-model.sh tiny

mkdir ./stt/models/
cp ~/whisper.cpp/models/ggml-large.bin ./stt/models/
cp ~/whisper.cpp/models/ggml-medium.bin ./stt/models/
cp ~/whisper.cpp/models/ggml-small.bin ./stt/models/
cp ~/whisper.cpp/models/ggml-tiny.bin ./stt/models/


## ビルド方法

セットアップが完了したら、CMake でビルドします：

```bash
# stt のビルド（libs/ にライブラリがあれば外部パスは不要）
cd stt
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

ビルド成功時は `build/myproj` 実行ファイルが生成されます。

## 実行方法

```bash
# ビルド時に RPATH が設定されているため、ライブラリパスの指定は不要
./myproj <モデルファイル> [オプション]
```

### オプション

```
--device N       : キャプチャデバイスID（デフォルト: 自動選択）
--step N         : 処理チャンクサイズ（ミリ秒、デフォルト: 500）
--keep N         : オーバーラップサイズ（ミリ秒、デフォルト: 200）
--vad on|off     : 音声検出モード（デフォルト: on）
```

### 例: small モデルで日本語音声を認識

```bash
./myproj models/ggml-small.bin
```

オプション付きの例：

```bash
# VAD 無効（固定ウィンドウモード）で処理
./myproj models/ggml-small.bin --vad off

# デバイス 1 を使用し、500ms チャンク、VAD 有効で処理
./myproj models/ggml-small.bin --device 1 --step 500 --vad on
```

### 出力例

```
10 capture device(s):
  [0] HDA Intel PCH: ALC... (hw:0,0)
  [1] C-Media Audio Device (hw:1,0)
Using device #0: HDA Intel PCH: ALC...
Audio streaming started...
Speech detected (prob=0.92)
00:00.00 --> 00:04.20 こんにちは世界。私たちには何ができるのか。
Speech detected (prob=0.88)
00:04.50 --> 00:08.30 今日はいい天気ですね。
```

## トラブルシューティング

### オーディオデバイスが見つからない場合

```bash
# 利用可能なオーディオデバイスを確認
arecord -l    # Linux ALSA
pactl list sources  # PulseAudio
```

### ライブラリが見つからないエラー

```
error while loading shared libraries: libwhisper.so.1: cannot open shared object file
```

この場合、ライブラリのコピーが正しく行われているか確認してください：

```bash
ldd ./build/myproj | grep -E "whisper|ggml"
```

すべてのライブラリが `libs/` ディレクトリから読み込まれていることを確認してください。

## モデルのダウンロード

whisper.cpp に含まれるスクリプトでモデルをダウンロードできます：

```bash
# whisper.cpp リポジトリのスクリプトを実行
cd ~/whisper.cpp/models

# モデルサイズ別のダウンロード例
bash download-ggml-model.sh tiny     #   75MB (低速・低精度)
bash download-ggml-model.sh base     #  142MB (低速・低精度)
bash download-ggml-model.sh small    #  466MB (推奨: 速度と精度のバランス良い)
bash download-ggml-model.sh medium   # 1.5GB (高精度・低速)

# ダウンロード後、stt/models にコピー（または symlink）
cp ~/whisper.cpp/models/ggml-*.bin /path/to/stt/models/
```

**推奨**: small モデルは精度と処理速度のバランスが良く、多くの場合で十分です。

## コードの概要

`main.cpp` の処理フロー（VAD無効モード）：

| ステップ | 関数 | 説明 |
|---------|------|------|
| 1 | `whisper_init_from_file_with_params()` | モデルをロードして `whisper_context*` を取得 |
| 2 | `SDL_InitSubSystem()` + `audio_async` | マイク初期化と音声キャプチャ開始 |
| 3 | `whisper_full_default_params()` | 推論パラメータを取得（言語・モード設定） |
| 4 | ループ: `audio.get()` → `whisper_full()` | 定期的に音声を取得して推論実行 |
| 5 | `whisper_full_n_segments()` + `whisper_full_get_segment_text()` | セグメント数と各セグメントの文字列を取得 |
| 6 | `audio.pause()` + `whisper_free()` | リソースを解放・終了 |

### マイク入力のモード

#### 固定ウィンドウモード（VAD無効）
- 定期的に指定時間（`--step`, デフォルト500ms）分の音声を取得
- 毎回 whisper で推論実行
- 継続的な推論が必要な場合に適している

#### 音声検出モード（VAD有効、デフォルト）
- エネルギー分析で音声存在を判定
- 音声検出時のみ推論実行（CPU節約）
- リアルタイム処理や省電力運用に適している

### パラメータ設定のポイント

- **言語指定**: `params.language = "ja"` で日本語として認識（省略すると自動検出、日本語が英語と誤認識されることがある）
- **翻訳**: `params.translate = true` にすると英語に翻訳
- ** sampling モード**: `WHISPER_SAMPLING_GREEDY`(高速) または `WHISPER_SAMPLING_BEAM_SEARCH`(高精度)

## 利用可能な API 関数

| カテゴリ | 関数 |
|---------|------|
| モデル管理 | `whisper_init_from_file_with_params()`, `whisper_free()` |
| コンテキスト | `whisper_context_default_params()` |
| 推論設定 | `whisper_full_default_params()`, `whisper_set_no_timestamps()` |
| 推論実行 | `whisper_full()`, `whisper_full_partial()` |
| 結果取得 | `whisper_full_n_segments()`, `whisper_full_get_segment_text()`, `whisper_full_get_segment_t0/t1()` |

## GPU アクセラレーション

CUDA サポート付きでビルドすると、自動的に NVIDIA GPU が使用されます。複数の GPU がある場合、すべてが検知されて並列処理の対象になります（RTX 4060 + RTX 5060 Ti の環境では両デバイスが認識されています）。
