# 今後の実装ロードマップ

## 概要

複雑なオンラインゲームを開発するために必要な機能追加案をまとめる。  
現在の実装（`connection_data` / ルーター / KVS / WebSocket）をベースに、以下の優先度順で拡張することを想定する。

---

## 優先度：高

### 1. ブロードキャスト API の拡張

現状の `api_qs_send_ws_message` はルーム内全員に送信するが、より細かい制御が必要。

| 追加関数（案） | 説明 |
|---|---|
| `api_qs_send_ws_message_to(context, connection_id, message)` | 特定 connection_id への送信 |
| `api_qs_send_ws_message_broadcast_except(params, message)` | 送信者を除くルーム内全員へ送信 |
| `api_qs_send_ws_message_to_owner(params, message)` | ルームオーナーへのみ送信 |

**対象ファイル**
- `core/src/qs_api.c`
- `core/header/qs_api.h`

---

### 2. ルーム管理 API の強化

| 追加関数（案） | 説明 |
|---|---|
| `api_qs_room_get_list(context, memory, object)` | 全ルーム一覧を JSON オブジェクトで取得 |
| `api_qs_room_get_member_count(context, room_name)` | ルームの現在人数取得 |
| `api_qs_room_is_full(context, room_name)` | ルームが満員かどうか判定 |
| `api_qs_room_get_owner_connection_id(context, room_name)` | オーナーの connection_id 取得 |

**対象ファイル**
- `core/src/qs_api.c`
- `core/header/qs_api.h`

---

### 3. `api_qs_server_create_router` のカスタムサイズ対応

現在は `QS_PACKET_ROUTE_CONNECTION_DATA_SIZE_DEFAULT` (1KB) 固定。  
ゲームによって必要なプレイヤーデータサイズが異なるため、呼び出し時にサイズを指定できるようにする。

```c
// 現在
int api_qs_server_create_router(QS_SERVER_CONTEXT* context);

// 追加案
int api_qs_server_create_router_ex(QS_SERVER_CONTEXT* context, size_t con_data_size, size_t max_route_chain);
```

---

## 優先度：中

### 4. サーバー側タイマー・ゲームループ

`api_qs_update` のループにタイマーコールバックを追加する。  
ゲームの状態更新（AI行動・エフェクト解決）をサーバー側で定期実行できるようにする。

```c
// コールバック登録（案）
api_qs_set_on_tick_event(context, on_tick, interval_ms);

// コールバック例
int on_tick(QS_SERVER_CONTEXT* context, int64_t delta_ms) {
    // 全ルームのゲーム状態を更新
    return 0;
}
```

---

### 5. マッチメイキング基盤

既存の `qs_queue` を使い、待機プレイヤーが n 人揃ったら自動でルームを作成してゲームを開始するパターン。

```c
// API 案
api_qs_matchmaking_enqueue(context, connection_id, params);
api_qs_matchmaking_dequeue(context, room_capacity, on_match_ready);
```

---

### 6. 認証・セッション連携

- HTTP ハンドシェイク時のトークン検証フック
- WebSocket `connection_id` と アプリ側ユーザーID の紐付けヘルパー

```c
// API 案
char* api_qs_get_http_header(params, "Authorization");
api_qs_set_connection_user_id(params, user_id);  // con_data 先頭へのラッパー
int64_t api_qs_get_connection_user_id(params);
```

---

### 7. ルーム参加・退室コールバック

現在の `on_connect` / `on_close` はソケットレベルのイベント。  
ルーム単位でのイベントフックを追加する。

```c
api_qs_set_on_room_join_event(context, on_room_join);
api_qs_set_on_room_leave_event(context, on_room_leave);
```

---

## 優先度：低

### 8. バイナリパケットヘルパー

高頻度更新（位置同期など）にはテキスト JSON は冗長。  
バイナリパケットの定義・読み書きをサポートするマクロ・ヘルパーを追加する。

```c
// パケット構造体をバイト列に変換して plane 送信するパターン
typedef struct { float x; float y; float z; uint32_t seq; } PLAYER_TRANSFORM;
PLAYER_TRANSFORM t = { 1.0f, 0.0f, 3.5f, seq++ };
api_qs_send_ws_message_plane_binary(params, (uint8_t*)&t, sizeof(t));
```

---

### 9. ゲームステートスナップショット（観戦・リプレイ）

- ルームの状態を定期的に KVS に保存
- 後から参加した観戦者や再接続クライアントに過去の状態を送信

---

### 10. con_data 構造体マッピングサンプル

`api_qs_get_connection_data` で取得したポインタに対して  
ゲーム固有の構造体をキャストして使う推奨パターンをサンプルコードで示す。

```c
// ゲーム側で定義する構造体例
typedef struct {
    int64_t  user_id;
    uint64_t send_count;
    float    pos_x;
    float    pos_y;
    int32_t  hp;
    int32_t  score;
    char     room_name[33];
    char     player_name[32];
} MY_PLAYER_DATA;

// 使い方
uint8_t* raw = api_qs_get_connection_data(params);
MY_PLAYER_DATA* player = (MY_PLAYER_DATA*)raw;
player->hp -= 10;
```

> **注意**: `sizeof(MY_PLAYER_DATA)` が `QS_PACKET_ROUTE_CONNECTION_DATA_SIZE_DEFAULT`（デフォルト 1KB）を超えないこと。  
> 超える場合は `api_qs_server_create_router_ex` で con_data_size を拡張する。

---

## 関連ファイル

| ファイル | 役割 |
|---|---|
| `core/src/qs_api.c` | API 実装 |
| `core/header/qs_api.h` | API 宣言 |
| `core/src/qs_packet_route.c` | ルーター・コネクションデータ実装 |
| `core/header/qs_packet_route.h` | ルーター宣言・定数定義 |
| `sample/http_server_simple/main.c` | サーバーサンプル実装 |
