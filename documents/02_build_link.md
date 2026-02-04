# ビルドとリンク

## コアライブラリのビルド
- Makefile は [core/src/Makefile](core/src/Makefile)
- 生成物: core/libqs_core.a

基本手順:
1. core/src へ移動
2. make を実行

## サンプルのビルド
各サンプルの Makefile は core/libqs_core.a をリンクします。
例:
- [sample/server/Makefile](sample/server/Makefile)
- [sample/http_server_simple/Makefile](sample/http_server_simple/Makefile)

基本フロー:
1. core を先にビルド
2. sample/XXX で make

## コンパイル時の include
- ヘッダは core/header を参照

## OS 定義
- Linux では -D_LINUX を指定（サンプル Makefile 参照）

## 依存
- pthread を使用（-pthread）
