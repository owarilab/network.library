# ソケット低レベル API

ヘッダ: [core/header/qs_socket.h](core/header/qs_socket.h)

## 基本概念
- `QS_SOCKET_OPTION` にソケット設定とコールバックを集約
- TCP/UDP のサーバ/クライアントを同じ API で扱う
- `qs_server_update` / `qs_client_update` をポーリングして進行

## 最小フロー（サーバ）
1. `qs_create_tcp_server` or `qs_create_udp_server`
2. `set_on_connect_event`, `set_on_payload_recv_event`, `set_on_close_event`
3. `qs_socket` で開始
4. ループで `qs_server_update`

参考: [sample/server/main.c](sample/server/main.c)

## 最小フロー（クライアント）
1. `qs_create_tcp_client` or `qs_create_udp_client`
2. `set_on_payload_recv_event`
3. `qs_socket` で開始
4. ループで `qs_client_update`

参考: [sample/client/main.c](sample/client/main.c)

## 送信
- サーバから: `qs_send_broadcast`, `qs_send_one`, `qs_send_message`
- クライアントから: `qs_client_send_message`, `qs_client_send`

## Plain モード
- `qs_create_tcp_server_plain` / `qs_create_tcp_client_plain`
- `set_on_plain_recv_event` を利用

## 注意点
- `qs_free_socket` と `qs_free(option->memory_pool)` の組み合わせで解放
- 送受信バッファサイズは `qs_set_recv_buffer` などで調整可能
