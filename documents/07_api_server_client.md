# 高レベル API（サーバ/クライアント）

ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)

## サーバ初期化
- `api_qs_server_init(&context, port, max_connection, server_type)`
- `server_type` は `QS_SERVER_TYPE_PLAIN` / `QS_SERVER_TYPE_SIMPLE` / `QS_SERVER_TYPE_HTTP`

## イベント登録
- `api_qs_set_on_connect_event`
- `api_qs_set_on_plain_event`
- `api_qs_set_on_simple_event`
- `api_qs_set_on_http_event`
- `api_qs_set_on_websocket_event`
- `api_qs_set_on_close_event`

## サーバループ
- `api_qs_update(context)`
- `api_qs_sleep(context)`

参考: [sample/http_server_simple/main.c](sample/http_server_simple/main.c)

## HTTP/WS の取得/応答
- 取得: `api_qs_get_http_method`, `api_qs_get_http_path`, `api_qs_get_http_post_body`
- 応答: `api_qs_send_response`, `api_qs_http_response_json`
- WS: `api_qs_get_ws_message`, `api_qs_send_ws_message`

## ルータ/ログ
- ルータ: `api_qs_server_create_router`, `api_qs_room_create`, `api_qs_room_join`
- ログ: `api_qs_server_create_logger_access` など

## クライアント
- `api_qs_client_init(&context, host, port, server_type)`
- イベント: `api_qs_set_client_on_connect_event`, `api_qs_set_client_on_plain_event`, `api_qs_set_client_on_simple_event`, `api_qs_set_client_on_close_event`
- 送信: `api_qs_client_send`, `api_qs_client_send_message`
- ループ: `api_qs_client_update`, `api_qs_client_sleep`

参考: [sample/client_simple/main.c](sample/client_simple/main.c)
