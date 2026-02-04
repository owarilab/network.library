# コアライブラリ概要

## ディレクトリ構成
- core/header: 公開ヘッダ群
- core/src: 実装とビルド用 Makefile
- sample: 利用例

## 主要ヘッダと役割
- [core/header/qs_core.h](core/header/qs_core.h)
  - OS/ビルド定義、基本マクロ、型・サイズ定義
- [core/header/qs_memory_allocator.h](core/header/qs_memory_allocator.h)
  - メモリプール管理
- [core/header/qs_socket.h](core/header/qs_socket.h)
  - TCP/UDP の低レベルソケット API
- [core/header/qs_api.h](core/header/qs_api.h)
  - 高レベル API（サーバ/クライアント、JSON/CSV/KVS/Script 等）
- [core/header/qs_json.h](core/header/qs_json.h)
  - JSON パーサ/エンコーダ（低レベル）
- [core/header/qs_csv.h](core/header/qs_csv.h)
  - CSV パーサ
- [core/header/qs_string.h](core/header/qs_string.h)
  - 文字列ユーティリティ

## 推奨の使い分け
- 低レベルで制御したい: qs_socket + qs_memory_allocator
- 早く動かしたい: qs_api
- JSON/CSV を軽量に扱う: qs_api の JSON/CSV ラッパ

## 参照先サンプル
- TCP/UDP サーバ: [sample/server/main.c](sample/server/main.c)
- TCP/UDP クライアント: [sample/client/main.c](sample/client/main.c)
- JSON: [sample/json/main.c](sample/json/main.c)
- CSV: [sample/csv/main.c](sample/csv/main.c)
- HTTP/WS サーバ: [sample/http_server_simple/main.c](sample/http_server_simple/main.c)
- KVS: [sample/cache/main.c](sample/cache/main.c)
- KVS 永続化: [sample/persistence_cache/main.c](sample/persistence_cache/main.c)
- Script: [sample/script/main.c](sample/script/main.c)
- Base64/SHA1: [sample/encode/main.c](sample/encode/main.c)
