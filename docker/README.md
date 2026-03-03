# Docker

Alpine ベースの軽量イメージで `sample/http_server_simple` を動作させます。

## ビルド

リポジトリルートから実行してください。

```bash
docker build -f docker/Dockerfile -t qs_http_server .
```

## 起動

```bash
docker run --name qs_http_server -d --rm -p 4444:80 qs_http_server
```

起動後、ブラウザで http://localhost:4444/ にアクセスして動作確認できます。

## ポート変更

`server.conf` の `server_port` を変更した場合は、`-p` のホスト側ポートも合わせてください。

```bash
docker run --rm -p 8080:8080 qs_http_server
```

## イメージ構成

| ステージ | ベース | 役割 |
|----------|--------|------|
| builder  | alpine:3.21 | gcc / make でソースをコンパイル |
| runtime  | alpine:3.21 | バイナリ・`www/`・設定ファイルのみ配置 |
