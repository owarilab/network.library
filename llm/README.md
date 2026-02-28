# LLM (optional module)

このディレクトリは、本プロジェクトの **オプション機能**（コア `core/` とは独立してビルド・リンク可能な追加モジュール）を置く場所です。

現在は `llama.cpp` を組み込む想定で、`ssl/` と同様の構成で雛形を用意しています。

## ディレクトリ構成

- `header/` : 公開ヘッダ
- `src/` : モジュール本体と Makefile
- `third_party/` : 外部依存（例: llama.cpp）

## llama.cpp の配置

推奨: `llm/third_party/llama.cpp/` に配置（git submodule でも OK）。
詳細は `llm/third_party/README.md` を参照してください。

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

### Ubuntu 24.04 で NVIDIA GPU を使う（CUDA Toolkit 導入）

`LLAMA_ENABLE=1` で GPU 実行するには、NVIDIA Driver に加えて CUDA Toolkit（`nvcc`）が必要です。

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

5) GPU 有効でビルド

```bash
cd llm/src
make clean
make build LLAMA_ENABLE=1 LLAMA_CUDA=1
```

6) 実行時に GPU オフロード層を指定

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
