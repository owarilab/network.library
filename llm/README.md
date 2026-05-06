# LLM (optional module)

このディレクトリは、本プロジェクトの **オプション機能**（コア `core/` とは独立してビルド・リンク可能な追加モジュール）を置く場所です。

現在は `llama.cpp` を組み込む想定で、`ssl/` と同様の構成で雛形を用意しています。

## ディレクトリ構成

- `header/` : 公開ヘッダ
- `src/` : モジュール本体と Makefile
- `third_party/` : 外部依存（例: llama.cpp）

## CUDA Toolkit 導入( GPUを使う場合 )

1) 事前確認

```bash
nvidia-smi
nvcc --version
```

2) CUDA 公式 APT リポジトリを追加

```bash
sudo apt-get update
sudo apt-get install -y wget gnupg ca-certificates
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
```

3) Toolkit をインストール（Driver 置き換えを避けるため `cuda-toolkit` を使用）

```bash
sudo apt-get install -y cuda-toolkit
```

4) シェル環境変数を反映

```bash
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

### CUDA Toolkit を更新する

現状の動作に問題がないなら、先に更新せずそのまま使う方が安全です。更新が必要になった場合だけ、以下の手順で Toolkit を上げます。

1) 現在の状態を確認

```bash
nvidia-smi
nvcc --version
ls -l /usr/local/cuda
```

2) パッケージ情報を更新

```bash
sudo apt-get update
apt list --upgradable | grep -E 'cuda|nvidia'
```

3) CUDA Toolkit を更新

```bash
sudo apt-get install -y cuda-toolkit
```

特定のメジャー・マイナーバージョンを明示したい場合:

```bash
sudo apt-get install -y cuda-toolkit-13-1
```

4) 更新後の反映を確認

```bash
nvcc --version
ls -l /usr/local/cuda/lib64/libcudart.so*
```

5) llama.cpp / network.library を再ビルド

```bash
cd llm/third_party/llama.cpp
rm -rf build
cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF -DGGML_CUDA=ON
cmake --build build -j"$(nproc)"

cd llm/src
make clean
make build LLAMA_ENABLE=1 LLAMA_CUDA=1
```

補足:
- `nvidia-smi` の `CUDA Version` 表示は Driver 側の互換情報です。実際にリンクされる Toolkit は `nvcc --version` と `libcudart.so` 側を確認してください。
- Gemma 4 系 GGUF を使う場合は、既知の品質問題があるため CUDA 13.2 ランタイムは避けてください。

## llama.cpp の配置

推奨: `llm/third_party/llama.cpp/` に配置（git submodule でも OK）。
詳細は `llm/third_party/README.md` を参照してください。

### `-hf` で取得した GGUF の保存場所を確認する

`./llama-cli -hf ...` で取得したモデルは、`llama.cpp` 配下ではなく Hugging Face の標準キャッシュに保存されます。

1) キャッシュ先に影響する環境変数を確認

```bash
printf 'HF_HOME=%s\nHUGGINGFACE_HUB_CACHE=%s\nXDG_CACHE_HOME=%s\n' "$HF_HOME" "$HUGGINGFACE_HUB_CACHE" "$XDG_CACHE_HOME"
```

未設定なら、通常は `~/.cache/huggingface/hub/` が保存先です。

2) GGUF ファイルを検索

```bash
find "$HOME/.cache/huggingface" -maxdepth 5 \( -type f -o -type l \) | grep -E 'gguf|GGUF|gemma|unsloth' | sort
```

特定モデルだけ確認したい場合:

```bash
find "$HOME/.cache/huggingface/hub/models--unsloth--gemma-4-E4B-it-GGUF" -maxdepth 5 \( -type f -o -type l \) | sort
```

3) 実際に使うパスを確認

`refs/main` が現在の snapshot を指し、実ファイルは `snapshots/<revision>/` 配下にあります。

```bash
cat "$HOME/.cache/huggingface/hub/models--unsloth--gemma-4-E4B-it-GGUF/refs/main"
find "$HOME/.cache/huggingface/hub/models--unsloth--gemma-4-E4B-it-GGUF/snapshots" -maxdepth 2 -name '*.gguf' | sort
```

### 使いやすい場所にシンボリックリンクを置く

毎回長い cache path を指定したくない場合は、リポジトリ内に symlink をまとめて置くと扱いやすくなります。

1) 格納用ディレクトリを作成

```bash
mkdir -p llm/models
```

2) GGUF 本体へ symlink を作成

```bash
ln -sfn "$HOME/.cache/huggingface/hub/models--unsloth--gemma-4-E4B-it-GGUF/snapshots/<revision>/gemma-4-E4B-it-UD-Q4_K_XL.gguf" llm/models/gemma-4-E4B-it-UD-Q4_K_XL.gguf
```

mmproj も使う場合:

```bash
ln -sfn "$HOME/.cache/huggingface/hub/models--unsloth--gemma-4-E4B-it-GGUF/snapshots/<revision>/mmproj-BF16.gguf" llm/models/mmproj-BF16.gguf
```

3) symlink の参照先を確認

```bash
ls -l llm/models
readlink -f llm/models/gemma-4-E4B-it-UD-Q4_K_XL.gguf
```

補足:
- snapshot の revision はモデル更新で変わるため、更新後は symlink の張り直しが必要になる場合があります。
- `<revision>` の部分は、直前の `refs/main` または `snapshots/` の確認結果で置き換えてください。
- 固定名で扱いたい場合は、毎回同じリンク名に `ln -sfn` で上書きすると運用しやすくなります。

### llama.cpp を更新して再ビルドする

すでに `llm/third_party/llama.cpp/` でビルド済みの場合は、llama.cpp 側を更新したあとに LLM モジュールを再ビルドします。

1) llama.cpp リポジトリを更新

通常の clone 配置:

```bash
cd llm/third_party/llama.cpp
git pull --ff-only
```

submodule 配置:

```bash
git submodule update --remote --merge llm/third_party/llama.cpp
```

2) 旧 build を消して llama.cpp を再ビルド

CPU ビルド:

```bash
cd llm/third_party/llama.cpp
rm -rf build
# mv build build_old
cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF
cmake --build build -j"$(nproc)"
```

CUDA ビルド:

```bash
cd llm/third_party/llama.cpp
rm -rf build
# mv build build_old
cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF -DGGML_CUDA=ON
cmake --build build -j"$(nproc)"
```

3) network.library の LLM モジュールを再ビルド

CPU:

```bash
cd llm/src
make clean
make build LLAMA_ENABLE=1
```

CUDA:

```bash
cd llm/src
make clean
make build LLAMA_ENABLE=1 LLAMA_CUDA=1
```

補足:
- `llm/src/Makefile` は `LLAMA_ENABLE=1` 時に `llm/third_party/llama.cpp/build/bin/libllama.so` を参照します。
- submodule 運用の場合、親リポジトリ側にも submodule の更新差分が出るため、必要ならその状態も commit してください。

## stable-diffusion.cpp の導入

`stable-diffusion.cpp` を使ったプラグインを追加する場合は、まず `llm/third_party/stable-diffusion.cpp/` に配置します。

### install

```bash
git clone --recursive https://github.com/leejet/stable-diffusion.cpp
cd stable-diffusion.cpp

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DSD_CUDA=ON
cmake --build . -j"$(nproc)"
```

### 動作確認

```bash
cd stable-diffusion.cpp
mkdir models
cd models
curl -L -O https://huggingface.co/runwayml/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors

mkdir outputs
```

### stable-diffusion.cpp を更新して再ビルドする

すでに `llm/third_party/stable-diffusion.cpp/` でビルド済みの場合は、stable-diffusion.cpp 側を更新したあとに LLM モジュールを必要に応じて再ビルドします。

1) stable-diffusion.cpp リポジトリを更新

通常の clone 配置:

```bash
cd llm/third_party/stable-diffusion.cpp
git pull --ff-only
```

submodule 配置:

```bash
git submodule update --remote --merge llm/third_party/stable-diffusion.cpp
```

2) 旧 build を消して stable-diffusion.cpp を再ビルド

CPU ビルド:

```bash
cd llm/third_party/stable-diffusion.cpp
rm -rf build
# mv build build_old
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"
```

CUDA ビルド:

```bash
cd llm/third_party/stable-diffusion.cpp
rm -rf build
# mv build build_old
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DSD_CUDA=ON
cmake --build . -j"$(nproc)"
```

3) network.library の LLM モジュールを再ビルド（必要に応じて）

`qs_diffusion_module.c` などで stable-diffusion.cpp を組み込んでいる場合は、LLM モジュールを再ビルドします。

CPU:

```bash
cd llm/src
make clean
make build
```

CUDA:

```bash
cd llm/src
make clean
make build DIFFUSION_CUDA=1
```

補足:
- stable-diffusion.cpp の更新は `-DSD_CUDA=ON` フラグに影響を与えることがあるため、更新後は必ず clean build を実施してください。
- `mv build build_old` でバックアップを取ることで、問題が発生した場合に前のビルドに戻すことができます。
- submodule 運用の場合、親リポジトリ側にも submodule の更新差分が出るため、必要ならその状態も commit してください。


## ビルド（Linux）

```bash
cd llm/src
make build
```

生成物:
- `llm/libqs_llm_module.a`

注意:
- `llama.cpp` をまだ配置していない場合でも、現状の雛形は **スタブとしてビルド可能**です（`qs_llama_module_is_available()` が 0 を返します）。
- 実際に llama.cpp を使う実装に進める場合は、`llm/src/qs_llama_module.c` を起点に拡張してください。

### 実推論を有効にする

実推論を有効にする場合は `LLAMA_ENABLE=1` を付けてビルドします。

```bash
cd llm/src
make build LLAMA_ENABLE=1
```

要件:
- `cmake` がインストール済みであること（llama.cpp 本体のビルドに必要）
- `llm/third_party/llama.cpp/build/bin` に `libllama.so` が存在すること

実行時はモデルパスを環境変数で指定します。

```bash
export QS_LLM_MODEL_PATH=/path/to/model.gguf
export QS_LLM_MAX_TOKENS=128
```

## Embedding モジュールの有効化

テキストから embedding ベクトルを生成し、sqlite-vec で保存・検索する機能です。`QS_EMBEDDING_MODULE_ENABLED=1` を指定した場合のみビルドされます。

### 事前準備

#### SQLite3 dev パッケージのインストール

```bash
sudo apt-get install -y libsqlite3-dev
```

#### sqlite-vec の取得

```bash
git clone https://github.com/asg017/sqlite-vec llm/third_party/sqlite-vec
```

#### ヘッダファイルの生成

sqlite-vec.c はヘッダ `sqlite-vec.h` を必要とします。以下で生成します：

```bash
cd llm/third_party/sqlite-vec
make sqlite-vec.h
```

### ビルド

#### LLM + Embedding 両有効（推奨・一般的な構成）

LLM のストリーミング機能と embedding 機能を同時に有効にするには以下のように指定します：

```bash
cd llm/src
make build LLAMA_ENABLE=1 LLAMA_CUDA=1 QS_EMBEDDING_MODULE_ENABLED=1
```

CPU のみの場合：

```bash
cd llm/src
make build LLAMA_ENABLE=1 QS_EMBEDDING_MODULE_ENABLED=1
```

生成物:
- `llm/libqs_llm_module.a`（ストリーミング + embedding 両機能含む）

#### Embedding のみ有効（実推論なし）

```bash
cd llm/src
make build QS_EMBEDDING_MODULE_ENABLED=1
```

生成物:
- `llm/libqs_llm_module.a`（embedding 機能のみ）

#### デフォルト（スタブ、機能無効）

従来通りフラグなし：

```bash
cd llm/src
make build
```

### API

ヘッダ: `header/qs_embedding_module.h`

| 関数 | 説明 |
|---|---|
| `qs_embedding_prepare(model_path, db_path)` | モデルとデータベースを初期化 |
| `qs_embedding_shutdown()` | リソース解放 |
| `qs_embedding_store(text, id)` | テキストを embedding 生成して保存 |
| `qs_embedding_search(query, top_k, out_ids, out_scores, max_results)` | 類似検索 |
| `qs_embedding_delete(id)` | 埋め込みベクトルを削除 |
| `qs_embedding_n_embd()` | ベクトル次元数の取得 |

### 実行時の環境変数

```bash
export QS_EMBEDDING_DB_PATH=/path/to/embeddings.db
```

## Ubuntu 24.04 で NVIDIA GPU を使う（CUDA Toolkit）

`LLAMA_ENABLE=1` で GPU 実行するには、NVIDIA Driver に加えて CUDA Toolkit（`nvcc`）が必要です。

1) GPU 有効でビルド

```bash
cd llm/src
make clean
make build LLAMA_ENABLE=1 LLAMA_CUDA=1
```

2) 実行時に GPU オフロード層を指定

```bash
export QS_LLM_MODEL_PATH=/path/to/model.gguf
export QS_LLM_MAX_TOKENS=128
export QS_LLM_N_GPU_LAYERS=999
```

補足:
- `QS_LLM_N_GPU_LAYERS=0` で CPU のみ。
- 大きい値（例: `999`）で可能な限り GPU にオフロードします。

## HTTP 逐次ストリーミング（SSE）

`llm/header/qs_llama_module.h` には、HTTP サーバと連携するための SSE ヘルパーを用意しています。

- `qs_llm_http_stream_open`
- `qs_llm_http_stream_send_event`
- `qs_llm_http_stream_send_token`
- `qs_llm_http_stream_send_done`

サンプル:

```bash
cd sample/llm_http_stream
make build
./qs_llm_http_stream_server
```

実推論有効時:

```bash
cd sample/llm_http_stream
make build LLAMA_ENABLE=1
./qs_llm_http_stream_server
```

別ターミナル:

```bash
curl -N -X POST "http://127.0.0.1:8080/api/llm/stream" -d "q=hello streaming world"
```
