# network.library

このリポジトリは、C言語でネットワーク機能を構築するためのライブラリです。

## 基本情報

- `sample/http_server_simple` に HTTP サーバー実装あり
- document rootは sample/http_server_simple/www
- URL http://localhost:4444/
- WebSocket を使ったルームサンプルを実装済み
- `con_data` により、接続（ユーザー）ごとのデータを構造体として保持可能
- オンラインゲーム開発の基礎として利用可能

## ルーム関連API

ルームのプリセットAPIは `core/src/qs_api.c`の `api_qs_exec_http`関数に実装されています。

- `/api/v1/room/create`
- `/api/v1/room/list`
- `/api/v1/room/join`
- `/api/v1/room/leave`

## 補足

`sample/http_server_simple/main.c` では、`on_ws_event` 内で `api_qs_get_connection_data` を使った接続ごとの状態管理例を確認できます。

## フロント側ルームサンプル

フロントのルームサンプルは `sample/http_server_simple/www/wsock.html` にあります。

- WebSocket 接続後、クライアント固有文字列を送信して `connection_id` を把握
- ルーム操作は HTTP API（create/list/join/leave）を呼び出して実行
- 参加中ルームIDをフロント側で保持し、leave 時に再利用
- WebSocket の message/join/leave イベントを受け、基本的な部屋内通信フローを確認可能

HTTP API と WebSocket を組み合わせた最小構成の実装例として、今後のオンラインゲーム開発の参考になります。
