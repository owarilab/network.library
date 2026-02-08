# Router(ルームサーバー) 機能まとめ

このドキュメントは `qs_api` の router 機能（ルームサーバー）と、`sample/http_server_simple` の WebSocket + Web API を使ったルームチャット動作を、実装ベースで整理したものです。

## 1. router(ルーム)とは

router は「ルームID(=route key) → 接続(connection)の集合」を管理する仕組みです。

- ルームは `qs_packet_route` によって管理され、**ルームID**（ランダム文字列）で参照します。
- WebSocket 接続ごとに **connection_id**（ランダム文字列）が割り当てられ、HTTP API で join すると `connection_id` がルームに紐づきます。
- WebSocket で `api_qs_send_ws_message()` を呼ぶと、送信者がルームに参加済みなら **同一ルームへブロードキャスト**、未参加なら **echo** になります。

## 2. 有効化方法（サーバ側）

HTTP/WebSocket サーバで router を使うには以下を行います。

1. サーバ初期化
2. router 生成
3. WebSocket/HTTP のイベント処理を設定（任意）

代表例は `sample/http_server_simple/main.c` です。

- `api_qs_server_init(..., QS_SERVER_TYPE_HTTP)`
- `api_qs_server_create_router(context)`
- `api_qs_set_on_websocket_event(context, on_ws_event)`
- `api_qs_set_on_http_event(context, on_http_event)`

※ `api_qs_exec_http()` 側に **組み込み Web API** があり、ステータスが 404 のときだけ `on_http_event` にフォールバックします。

## 3. Web API（組み込み /api/v1/room/*）

これらは `QS_SERVER_TYPE_HTTP` のサーバで自動的に有効になります（router を作っていることが前提）。

### 3.1 ルーム作成

- `POST /api/v1/room/create`
- `Content-Type: application/x-www-form-urlencoded`
- パラメータ
  - `name` : ルーム名（最大 32 文字）

レスポンスは JSON（ルーム情報）です。

### 3.2 ルーム一覧

- `GET /api/v1/room/list` または `POST /api/v1/room/list`

レスポンス例（概形）:

- `list`: ルーム配列
  - `id`: ルームID
  - `max`: 収容上限（デフォルト 10）
  - `connection`: 接続数
  - `created` / `updated`: epoch 秒
  - `owner_id`: オーナーの connection_id（未設定は空文字列）
  - `data`: ルームデータ（文字列）

`data` には `room/create` 時に格納された JSON 文字列（例: `{ "name": "test_room" }`）が入ります。

### 3.3 ルーム参加

- `POST /api/v1/room/join`
- `Content-Type: application/x-www-form-urlencoded`
- パラメータ
  - `room_id`: 参加先ルームID
  - `connection_id`: WebSocket ハンドシェイク後に割り当てられた connection_id

成功時は JSON（ルーム情報）を返し、同時に WebSocket で join 通知が配信されます。

## 4. WebSocket メッセージ形式（qs_make_ws_message_simple）

router が有効な場合、`api_qs_send_ws_message()` は WebSocket フレームに以下 JSON を載せて送ります。

```json
{ "id": "<sender_connection_id>", "type": "message|join|leave", "message": "<string>" }
```

- `id`: 送信者の connection_id
- `type`:
  - `message`: クライアントが送ったテキスト（またはサーバが指定した文字列）
  - `join`: join 発生
  - `leave`: close/leave 発生
- `message`:
  - `message` の場合: 本文文字列
  - `join/leave` の場合: **ルーム情報 JSON をさらに文字列として埋め込んだもの**

## 5. ブロードキャスト条件（room 未参加時は echo）

- 送信者がルームに join 済み: ルーム内の全 WebSocket 接続へ送信
- 送信者が未 join: 送信者へ echo

この挙動により、サンプルでは「接続直後は自分に返る → join 後はルームへ飛ぶ」という流れになります。

## 6. join/leave 時の動作

### 6.1 join

`/api/v1/room/join` 成功時に、サーバは同一ルーム内の接続へ `type=join` を送ります。

また、ルームにオーナーが存在しない場合、最初に join した接続が `owner_id` になります。

### 6.2 leave（切断）

WebSocket 切断時に、サーバは以下を行います。

- ルームから接続を削除
- オーナーだった場合は別接続へオーナーを移譲
- ルーム内へ `type=leave` を通知

## 7. ルームの寿命（TTL）

- デフォルト TTL は `QS_PACKET_ROUTE_LIFE_TIME_SEC_DEFAULT`（5 分）
- ルームが空で、かつ TTL 経過した場合に削除されます
- `api_qs_update()` 内で約 60 秒ごとに `qs_update_packet_route()` が呼ばれて掃除されます

## 8. サンプル: http_server_simple ルームチャット

対象: `sample/http_server_simple`

### 8.1 起動

- ビルド: `make`
- 実行: `./qs_http_server`
- ポート: `server.conf` の `server_port`（デフォルトは 4444）

### 8.2 クライアント

`sample/http_server_simple/www/wsock.html` は以下の流れで動きます。

1. `ws://localhost:<port>` に WebSocket 接続
2. 接続直後にランダム文字列（uuid）を 1 回送る
3. サーバからの echo を受け取り、`json.id` を **自分の connection_id** として採用
4. `GET /api/v1/room/list` を叩き、
   - ルームがあれば最初のルームに join
   - なければ `POST /api/v1/room/create` で `test_room` を作って join
5. join/leave を監視し、他ユーザ join 時にメッセージ送信（デモ用）

ブラウザの別タブで `wsock.html` を複数開くと、同じルームに join し、WebSocket 送信がルーム内にブロードキャストされます。

---

関連 API（ヘッダ）:

- `api_qs_server_create_router()`
- `api_qs_room_create()` / `api_qs_room_list()` / `api_qs_room_join()` / `api_qs_room_leave()`
- `api_qs_send_ws_message()`
