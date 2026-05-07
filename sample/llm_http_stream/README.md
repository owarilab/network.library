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

Embedding 機能を有効にする場合:

```bash
cd sample/llm_http_stream
make build LLAMA_ENABLE=1 LLAMA_CUDA=1 QS_EMBEDDING_MODULE_ENABLED=1
```

`QS_EMBEDDING_MODULE_ENABLED=1` 時は embedding モジュールと sqlite-vec をビルドし、テキスト埋め込み・ベクトル検索機能が利用可能になります。

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

Embedding 機能を使う場合は起動前に以下も指定:

```bash
export QS_EMBEDDING_MODEL_PATH=/path/to/embedding-model.gguf
export QS_EMBEDDING_DB_PATH=./embeddings.db
./qs_llm_http_stream_server
```

LLM と Embedding の両方を有効にして起動する場合:

```bash
export QS_LLM_MODEL_PATH=~/.cache/huggingface/hub/models--unsloth--gemma-3-270m-it-GGUF/snapshots/c90975dbd40c0c7b275fefaae758c3415c906238/gemma-3-270m-it-Q4_K_M.gguf
export QS_LLM_N_CTX=4096
export QS_LLM_MAX_TOKENS=128
export QS_LLM_SYSTEM_PROMPT_FILE=./system_prompt.md
export QS_LLM_N_GPU_LAYERS=999
export QS_EMBEDDING_MODEL_PATH=~/.cache/huggingface/hub/models--unsloth--embeddinggemma-300m-GGUF/snapshots/6661a6504c30d8304af13455cb4a5d4f5bc6011f/embeddinggemma-300M-BF16.gguf
export QS_EMBEDDING_DB_PATH=./embeddings.db
./qs_llm_http_stream_server
```

`QS_LLM_SYSTEM_PROMPT_FILE` は省略可能です。未指定時は `./system_prompt.md` を読み込みます（互換のため `./system_prompt.txt` もフォールバックで対応）。
`QS_LLM_N_CTX` は推論コンテキスト長です。長いプロンプトや JSON 修復タスクでは `2048` 以上（推奨 `4096`）を指定してください。
`QS_LLM_N_GPU_LAYERS` は GPU オフロード層数です。`0` で CPU のみ、`999` など大きい値で可能な限り GPU を使います（未指定時は `999`）。
`QS_LLM_MAX_TOKENS` は最大生成トークン数です。未指定時はモジュール既定値（`128`）、`run.sh` 既定値は `512` です。

`QS_EMBEDDING_MODEL_PATH` と `QS_EMBEDDING_DB_PATH` は Embedding 機能を使う場合の指定です（省略可）。
`QS_EMBEDDING_MODEL_PATH` は embedding モデルファイルのパス、`QS_EMBEDDING_DB_PATH` は SQLite データベースの保存先です。
両方指定されている場合のみ Embedding モジュールが初期化され、後述の `/api/embed` エンドポイントが利用可能になります。

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

## Embedding API requests

Embedding 機能が有効な場合（`QS_EMBEDDING_MODULE_ENABLED=1` でビルド、かつ `QS_EMBEDDING_MODEL_PATH` 指定）、以下の API が利用可能です。

テキストの embedding を生成して DB に保存（基本的なサンプル）:

```bash
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=This is a sample text to embed&id=1"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=Another document with similar content&id=2"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=Completely different topic here&id=3"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=Sample1&id=4"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=other text&id=5"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=simple sample&id=6"
```

実用的なサンプルデータセット（複数国の特色）:

```bash
# 日本
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=日本は東アジアの島国で、首都は東京です。四季折々の自然美があり、特に春の桜と秋の紅葉が有名です。日本文化には茶道、花道、武道などの伝統芸能があります。&id=101"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=日本料理は世界的に有名で、寿司、天ぷら、味噌汁などが代表的です。最近では和食がユネスコ無形文化遺産に登録されました。日本酒も世界で高く評価されています。&id=102"

# フランス
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=フランスはヨーロッパ西部に位置し、首都はパリです。エッフェル塔、ノートルダム大聖堂、ルーヴル美術館などの有名な建造物があります。パリは芸術と文化の中心地として知られています。&id=201"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=フランスはワインの名産地で、ボルドー、ブルゴーニュなどの地域は世界的に有名です。チーズの種類も多く、フレンチ料理は高級料理の代名詞とされています。&id=202"

# イタリア
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=イタリアは南ヨーロッパの国で、首都はローマです。古代ローマ帝国の遺跡、コロッセオ、ペンテオンなど歴史的建造物が多数あります。ルネッサンスの発祥地として芸術の宝庫です。&id=301"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=イタリア料理はパスタやピザで世界的に有名です。ミラノ、ヴェネチア、フィレンツェなどの都市は観光地として人気があります。イタリアンアイスクリームも有名です。&id=302"

# スペイン
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=スペインはイベリア半島の西部に位置し、首都はマドリードです。アントニ・ガウディの建築作品、サグラダ・ファミリア教会は世界遺産です。フラメンコはスペインを代表する舞踊です。&id=401"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=スペイン料理のパエリアはスペイン東部の代表的な米料理です。タパスという小皿料理も人気があります。スペインはハモン（生ハム）の名産地としても知られています。&id=402"

# 中国
curl -X POST "http://localhost:8080/api/embed" \
  -d "text=中国は世界最大の人口を持つ国で、首都は北京です。万里の城壁、故宮、兵馬俑などの歴史的遺産があります。中国文明は世界で最も古い文明の一つです。&id=501"

curl -X POST "http://localhost:8080/api/embed" \
  -d "text=中国料理は地域ごとに特色があり、四大料理として四川料理、広東料理、山東料理、淮揚料理が有名です。餃子やラーメンも世界的に知られています。&id=502"
```

クエリーテキストの embedding を生成して、DB 内で類似検索:

```bash
# top_k（返す結果数）はオプション、デフォルト 5
curl -X POST "http://localhost:8080/api/embed/search" \
  -d "q=sample text query&top_k=5"
```

RAG（Retrieval-Augmented Generation）による質問応答:

```bash
# クエリに対して、検索結果をコンテキストとして含めて LLM が回答
curl -X POST "http://localhost:8080/api/llm/rag" \
  -d "q=sample text query&top_k=3"

# ストリーミングで回答を受け取る場合
curl -N -X POST "http://localhost:8080/api/llm/rag/stream" \
  -d "q=sample text query&top_k=3"
```

RAG API のパラメータ:
- `q` (必須): クエリテキスト
- `top_k` (オプション): 検索結果の件数、デフォルト 3（最大 100）
- `context_prefix` (オプション): コンテキストのプレフィックス、デフォルト "### Doc"
- `context_suffix` (オプション): コンテキストのサフィックス、デフォルト "---"

Embedding を削除:

```bash
curl -X POST "http://localhost:8080/api/embed/delete" \
  -d "id=1"
```

サーバーの状態確認（LLM・Embedding 有効化状態を表示）:

```bash
curl "http://localhost:8080/api/status"
```

期待される Embedding API レスポンス例:

```json
# POST /api/embed (保存成功時)
{"ok":true,"id":1,"text_length":30}

# POST /api/embed/search (検索結果)
{"ok":true,"query":"sample text query","results":[
  {"id":1,"distance":0.150234},
  {"id":2,"distance":0.425678}
],"count":2}

# POST /api/llm/rag (RAG応答)
{"ok":true,"answer":"...LLM generated answer based on retrieved context...","sources":[
  {"id":1,"distance":0.150234,"text":"This is a sample text to embed"},
  {"id":4,"distance":0.207580,"text":"Sample1"}
],"count":2}

# GET /api/status
{"ok":true,"llm_enabled":true,"embedding_enabled":true}
```

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
- `QS_EMBEDDING_MODULE_ENABLED=1` でビルドし、`QS_EMBEDDING_MODEL_PATH` と `QS_EMBEDDING_DB_PATH` を設定すると embedding・ベクトル検索機能が有効になります。
