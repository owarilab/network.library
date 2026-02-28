# LLM HTTP streaming sample

`llm` モジュールの SSE 送信 API を使って、HTTP サーバから逐次トークン配信するサンプルです。

## Build

```bash
cd sample/llm_http_stream
make build
```

実推論を有効にする場合:

```bash
cd sample/llm_http_stream
make build LLAMA_ENABLE=1
```

`LLAMA_ENABLE=1` 時は `llm/third_party/llama.cpp` を CMake で自動ビルドします。

NVIDIA GPU を使う場合（デフォルト有効）:

```bash
cd sample/llm_http_stream
make build LLAMA_ENABLE=1 LLAMA_CUDA=1
```

CPU のみでビルドしたい場合:

```bash
cd sample/llm_http_stream
make build LLAMA_ENABLE=1 LLAMA_CUDA=0
```

## Run

```bash
./qs_llm_http_stream_server
```

実推論を使う場合は起動前にモデルを指定:

```bash
export QS_LLM_MODEL_PATH=/path/to/model.gguf
export QS_LLM_N_CTX=4096
export QS_LLM_MAX_TOKENS=128
export QS_LLM_SYSTEM_PROMPT_FILE=./system_prompt.md
export QS_LLM_N_GPU_LAYERS=999
./qs_llm_http_stream_server
```

`QS_LLM_SYSTEM_PROMPT_FILE` は省略可能です。未指定時は `./system_prompt.md` を読み込みます（互換のため `./system_prompt.txt` もフォールバックで対応）。
`QS_LLM_N_CTX` は推論コンテキスト長です。長いプロンプトや JSON 修復タスクでは `2048` 以上（推奨 `4096`）を指定してください。
`QS_LLM_N_GPU_LAYERS` は GPU オフロード層数です。`0` で CPU のみ、`999` など大きい値で可能な限り GPU を使います（未指定時は `999`）。
`QS_LLM_MAX_TOKENS` は最大生成トークン数です。未指定時はモジュール既定値（`128`）、`run.sh` 既定値は `512` です。

`LLAMA_ENABLE=1` かつ `QS_LLM_MODEL_PATH` 指定時は、サーバ起動時にモデルを1回だけロードし、その後のリクエストで再利用します。

## Streaming request

```bash
curl -N -X POST "http://localhost:8080/api/llm/stream" -d "q=hello streaming world"
```

## JSON mode request

```bash
curl -s -X POST "http://localhost:8080/api/llm/json" -d "q=都道府県を1つ選び、nameとpopulationをJSONで返して"

curl -s -X POST "http://localhost:8080/api/llm/json" -d "架空のファンタジーRPGに登場するキャラクターのステータスをJSONで出力して。プロパティには名前、職業、レベル、スキル（配列）、装備（オブジェクト）、そして現在オンラインかどうか（Boolean）を含めてください。"

curl -s -X POST "http://localhost:8080/api/llm/json" -d "あるIT企業の組織図をJSONで作成して。CEOを頂点とし、その下に技術部門と営業部門を配置して。各部門にはマネージャー1名とメンバー2名を含めること。各個人のデータには employee_id（"EMP-000"形式）、role、tags（文字列の配列）を持たせてください。"

curl -s -X POST "http://localhost:8080/api/llm/json" -d "スマートホームのデバイス管理システムをJSONで設計して。照明デバイス（明るさと色のプロパティを持つ）、エアコン（温度とモードのプロパティを持つ）、スマートロック（施錠状態と最終操作時間のプロパティを持つ）。これらを一つの devices 配列に入れ、各デバイスには共通の id と type を付与してください。"

curl -s -X POST "http://localhost:8080/api/llm/json" -d "PCのファイルシステムのディレクトリ構造をJSONで表現して。ルートフォルダの下に『Documents』と『Images』があり、『Documents』内にはさらに2つのサブフォルダ、その中にそれぞれテキストファイルが1つずつあるような、少なくとも4階層のネストにしてください。各要素は name、type ("folder" or "file")、およびフォルダの場合は children 配列を持ってください。"

curl -s -X POST "http://localhost:8080/api/llm/json" -H "Content-Type: application/json" -d '{
  以下の条件でECサイトの注文確認データをJSONで生成してください。
  1. items配列に3つの商品（商品名、単価、数量、小計）を入れてください。
  2. 商品の単価はそれぞれ 1200, 2500, 800 とし、数量は任意に設定してください。
  3. summaryオブジェクトに以下を含めてください：
     - total_items_price: 全商品の小計の合計
     - shipping_fee: total_items_priceが5000以上の場合は0、それ以外は500
     - tax_rate: 0.1
     - tax_amount: total_items_priceに対する消費税（小数点以下切り捨て）
     - grand_total: (total_items_price + tax_amount + shipping_fee) の合計
  4. すべての計算結果に矛盾がないようにしてください。
}'

curl -s -X POST "http://localhost:8080/api/llm/json" -H "Content-Type: application/json" -d '{
  以下の【壊れたJSON】を解析し、文法エラーをすべて修正して正しいJSON形式で出力してください。
  1. 欠けている引用符を追加すること。
  2. 不足している、または余計なカンマを修正すること。
  3. JSON以外の説明文は一切含めないこと。

  【壊れたJSON】:
  {
    "status": "success
    "data": [
      {"id": 101, "name": "Item A" "price": 500},
      {"id": 102, "name": "Item B", "price": 1200},
    ],
    "message": "Processed successfully"
}'
```

JSON mode は JSON オブジェクトの生成を促し、サーバ側で JSON 検証を行います。
モデル出力が JSON にならない場合は、`mode: fallback_raw_text` 付きの JSON オブジェクトで返します。

期待される出力（SSE形式）:

```text
event: meta
data: qs_llama_module: ...

event: token
data: stub-response:

event: token
data: hello
...

event: done
data: [DONE]
```

備考:
- デフォルトビルドはスタブ応答です。
- `LLAMA_ENABLE=1` でビルドし、`QS_LLM_MODEL_PATH` を設定すると llama.cpp 推論トークンを逐次送信します。
