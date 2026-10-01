# 概要

このリポジトリは、C言語でプログラミングをするためのライブラリとサンプル集である

## 基本情報

### ディレクトリ構成

| ディレクトリ | 説明 |
|---|---|
| `core/` | メインライブラリ。HTTP/WebSocketサーバー、JSON/CSV処理、メモリ管理など |
| `sample/` | 各機能のサンプル実装。Makefile付きで即座にビルド・実行可能 |
| `documents/` | 実装ドキュメント。`README.md` から読み始めることを推奨 |
| `llm/` | LLM関連モジュール（llama.cpp等の統合） |
| `stt/` | 音声認識モジュール（whisper.cpp、C++/CMake） |
| `ssl/` | SSL/TLS関連のオプションモジュール |
| `plugin/` | Unity向けプラグイン (`libqs.dll` / `libqs.so`) |
| `docker/` | Docker環境でのビルド・実行設定 |

### coreのコンセプト

core は、低レベルAPIと高レベルAPIを分けて提供する構成である。
ライブラリとしてcoreの機能を提供する高レベルAPIは `qs_api` に公開関数などを実装している。

メモリ管理は専用のメモリマネージャを使用し、malloc,free は基本的に直接行わない方針である。
JSON、CSV、KVS、Script などの機能もメモリプールを前提に連携する設計であり、確保と解放を個別に繰り返すのではなく、用途ごとの領域単位で管理することを重視する。

また、一時的な処理用メモリと長期保持するデータ用メモリを分離して扱うことを前提としており、アプリケーション側でもこの方針に沿って設計すると扱いやすい。

### サーバーの実装コンセプト

サーバーはノンブロッキング（non-blocking）で動作するため、以下のようにメインループを実装する：

```c
for(;;){
    api_qs_update(context);
    api_qs_sleep(context);
}
```

ノンブロッキング動作であるため、同一プロセス内で複数のサーバーを同居させることができる。例えば、HTTP サーバーと別のプロトコルサーバーを同時に実行する場合も、各サーバーの `api_qs_update()` と `api_qs_sleep()` をメインループで交互に呼び出すだけで動作する。

## HTTPサーバーについて

### サーバーの初期化

HTTPサーバーとして動作させるには、`api_qs_server_init` の第4引数に `QS_SERVER_TYPE_HTTP` を指定する：

```c
QS_SERVER_CONTEXT* context = 0;
api_qs_server_init(&context, port, max_connection, QS_SERVER_TYPE_HTTP);
```

coreの HTTP サーバー実装（`core/src/qs_protocol.c` の `http_request_common` 関数）には、**デフォルトで静的ファイルの配信機能が組み込まれている**。

### 静的ファイル配信の仕組み

処理の流れ：
1. coreの `http_request_common` で静的ファイルの確認・配信処理が先に実行される
2. その後、組み込みAPI エンドポイント処理が実行される
3. 組み込みAPIにも一致せず 404 が返された場合、`api_qs_set_on_http_event` で登録したコールバック関数が呼ばれる

静的ファイル配信の詳細：

- **ドキュメント root**: `./www` ディレクトリがデフォルト
- **デフォルトファイル**: `/` でアクセスした場合は `index.html` を返す
- **ファイル拡張子と Content-Type**:
  - `.html` → `text/html`
  - `.css` → `text/css; charset=UTF-8`
  - `.js` → `text/javascript; charset=UTF-8`
  - `.json` → `application/json; charset=UTF-8`
  - `.ico` → `image/x-icon`
  - `.png` → `image/png`
  - `.jpg` → `image/jpeg`
  - `.mp3` → `audio/mp3`
  - `.unityweb` → `application/octet-stream`
- **キャッシュ制御**: Cache-Control ヘッダを自動付与（デフォルト 30 秒）
- **条件付きリクエスト**: If-Modified-Since による 304 Not Modified レスポンスをサポート

API エンドポイント（`/api/v1/*`）にマッチしないリクエストは、自動的にこの静的ファイル配信機能によって処理される。

## WebSocketサーバーについて

WebSocketサーバーとして動作させるには、`api_qs_server_init` の第4引数に `QS_SERVER_TYPE_HTTP` を指定する。HTTPサーバー兼WebSocketサーバーとして動作し、静的ファイル配信とWebSocket通信が同時に行える。

`api_qs_set_on_websocket_event` でコールバック関数を指定してイベントを受け取り、処理していく。

### 接続の識別と状態管理

各クライアント接続は一意な接続IDで識別され、接続ごとにメモリ領域を保持できる：

```c
int on_ws_event(QS_EVENT_PARAMETER params)
{
    // 接続IDの取得
    char* connection_id = api_qs_get_connection_id(params);
    
    // 接続ごとのデータ領域を取得（接続時に初期化可能）
    uint8_t* con_data = api_qs_get_connection_data(params);
    size_t con_data_size = api_qs_get_connection_data_size(params);
    
    // 受信したメッセージの取得
    char* message = api_qs_get_ws_message(params);
    
    // クライアントへメッセージ送信
    api_qs_send_ws_message(params, message);
    
    return 0;
}
```

### ルーム機能との連携

coreに組み込みのルーム管理API（`/api/v1/room/*`）と組み合わせることで、ハイブリッドな構成が可能である：

- **HTTP API**：ルームの作成・参加・退出などの管理操作
- **WebSocket**：ルーム内のリアルタイム通信

ルーム機能を使用する場合、サーバーの初期化時に `api_qs_server_create_router` を呼び出して、ルーター機能を有効化する必要がある：

```c
QS_SERVER_CONTEXT* context = 0;
api_qs_server_init(&context, port, max_connection, QS_SERVER_TYPE_HTTP);
api_qs_server_create_router(context);  // ルーム機能を使う場合は必須
```

具体的な使用パターン：
1. クライアントがHTTP API（`/api/v1/room/create`等）でルーム管理
2. WebSocket接続で該当ルーム内のメッセージをブロードキャスト
3. 接続ごとのデータで現在の参加ルームIDを管理

HTTP API と WebSocket を組み合わせた最小構成の実装例として、今後のオンラインゲーム開発の参考になる。

## coreライブラリを使用する場合

coreライブラリを使用する際は内部の実装を見てからアプリケーションを実装すること。

## よくあるミス

api_qs_memory_clean関数はmemsetと同じ意味でヒープが初期化される。
api_qs_memory_free関数はfree関数と同じ意味でメモリが破棄されるので考慮せずにメモリにアクセスするとメモリアクセス違反が起きるので注意。

