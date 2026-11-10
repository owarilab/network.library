# docker_cli

`sample/http_client` と同じ qs core + ssl モジュール（`QS_HTTP_CLIENT_CONTEXT`）のみで、
Docker Engine API（plain HTTP / TCP）をコマンド引数から操作する CLI ツールのサンプルです。

- `core/`・`ssl/` のソースには変更を加えていません
- 通信は plain HTTP（TCP）のみです（Unix ソケット・TLS は未対応）

## ビルド

```bash
cd sample/docker_cli
make
```

## 前提

Docker Engine が TCP で plain HTTP 提供している必要があります。

### 方法1: dockerd 側で TCP を有効にする（常設・推奨）

systemd 管理の環境（Ubuntu / WSL2 + systemd 等）では、
`/etc/docker/daemon.json` に `hosts` を追加して再起動します
（Docker 公式の推奨方式。プロキシは不要になります）。

```bash
# 1. バックアップ
sudo cp /etc/docker/daemon.json /etc/docker/daemon.json.bak

# 2. hosts を追加（既存設定がある場合は内容を保持したまま統合する）
sudo tee /etc/docker/daemon.json > /dev/null <<'EOF'
{
    "hosts": ["unix:///var/run/docker.sock", "tcp://127.0.0.1:2375"]
}
EOF

# 3. 再起動
sudo systemctl restart docker

# 4. 確認（JSON が返れば成功）
curl -s http://127.0.0.1:2375/version
```

> - 既存の `daemon.json` に他の設定（`runtimes` など）がある場合は、
>   それらを保持したまま `"hosts"` キーだけ追加してください
> - **再起動の影響**: 稼働中のコンテナは止まります（restart ポリシー付きは自動再起動）
> - **セキュリティ**: 2375 は認証のない平文 HTTP です。
>   必ず `127.0.0.1` のみにバインドしてください。
>   外部アクセスが必要な場合は TLS（2376 + 証明書）を使用してください
>   （mTLS 認証は本サンプルでは未対応）
> - **元に戻す**:
>   ```bash
>   sudo cp /etc/docker/daemon.json.bak /etc/docker/daemon.json
>   sudo systemctl restart docker
>   ```

#### 代替: systemd drop-in で起動引数を上書きする

`daemon.json` を編集せずに `ExecStart` を上書きする方法です:

```bash
sudo systemctl edit docker
```

以下を入力して保存:

```ini
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -H unix:///var/run/docker.sock -H tcp://127.0.0.1:2375 --containerd=/run/containerd/containerd.sock
```

`sudo systemctl restart docker` で反映されます
（`ExecStart=` の空行が既存の引数を含む行の上書きに必要です）。

#### systemd 非依存の環境

systemd を使わない環境では、起動時にフラグを付けるだけで OK です:

```bash
dockerd -H tcp://127.0.0.1:2375 &
```

### 方法2: 既存の Unix ソケットを TCP に中継する（デーモンを再起動したくない場合）

`/var/run/docker.sock` を TCP にブリッジするプロキシを別途起動します。
例（python3）:

```python
import socket, socketserver, select

class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        unix = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        unix.connect("/var/run/docker.sock")
        for conn in [unix, self.request]:
            pass
        conns = [unix, self.request]
        while True:
            readable, _, _ = select.select(conns, [], conns, 60)
            for conn in readable:
                data = conn.recv(65536)
                if not data:
                    return
                (self.request if conn is unix else unix).sendall(data)

socketserver.ThreadingTCPServer(("127.0.0.1", 2375), Handler).serve_forever()
```

## 使い方

```
docker_cli [--host HOST] [--port PORT] <command> [args]

共通オプション:
  --host HOST   Docker Engine のホスト（既定: 127.0.0.1）
  --port PORT   Docker Engine のポート（既定: 2375）

コマンド:
  version                  エンジンのバージョン情報を表示（GET /version）
  info                     エンジン情報を表示（GET /info）
  ps [--all]               コンテナ一覧（GET /containers/json）
  images                   イメージ一覧（GET /images/json）
  run IMAGE [CMD...]       コンテナ作成+起動（POST /containers/create → POST /containers/{id}/start）
  start <container>        コンテナ起動（POST /containers/{id}/start）
  stop <container>         コンテナ停止（POST /containers/{id}/stop）
  rm <container> [--force] コンテナ削除（DELETE /containers/{id}）
  logs <container>         コンテナログ表示（GET /containers/{id}/logs?stdout=1&stderr=1）
```

## 動作例

```bash
$ ./qs_docker_cli version
Version        29.1.3
ApiVersion     1.52
MinAPIVersion  1.44
GoVersion      go1.24.4
Os             linux
Arch           amd64
KernelVersion  6.18.33.2-microsoft-standard-WSL2

$ ./qs_docker_cli ps --all
CONTAINER ID   IMAGE                        NAME                     STATUS
111973a01dff   python-skeleton-app          python-skeleton-app-1    Exited (0) 3 months ago
...

$ ./qs_docker_cli images
IMAGE ID       TAGS                             SIZE
9ab6d30d9c82   ghcr.io/berriai/litellm:v1.106.6 1.21 GB
...

$ ./qs_docker_cli run alpine:3.21 echo hello
2f9e405920e9098088841051290c59860bf94e562ff52dcc2daa6aae578edfa7

$ ./qs_docker_cli logs 2f9e4059
hello

$ ./qs_docker_cli stop 2f9e4059
stop: 2f9e4059

$ ./qs_docker_cli rm 2f9e4059
removed: 2f9e4059

$ ./qs_docker_cli logs no-such-container
docker_cli: error: HTTP 404: No such container: no-such-container
```

エラー（HTTP 400 以上）は Docker エラー JSON の `message` を表示し、非ゼロ終了します。

## 実装のポイント

- 通信は `sample/http_client` と同じワンショットパターン:
  `qs_ssl_module_http_client_connect(host, port, is_ssl=0)` → リクエストバッファに HTTP リクエストを手組み →
  `update` / `sleep` を `QS_SSL_MODULE_PHASE_DISCONNECT` までループ → header/body を読む → `dispose`
- 1 コマンド = 1 接続（`Connection: close`、keep-alive は不使用）
- JSON 解析は `api_qs_json_decode_object`。Docker の配列レスポンス（`/containers/json` 等）は
  トップレベルが配列のため、ボディを `{"items": <body>}` でラップしてからのみオブジェクトとしてデコードする
- 数値は JSON デコーダが int32 範囲なら int32、それ以外は uint64 として格納するため、
  getter も両方試す（`Size` / `MemTotal` など）
- `logs` は Docker raw stream（8 バイトヘッダ + データのフレーム列）をデミュークスして表示する
- パスに埋め込む ID・イメージ名は文字種を制限（英数字と `./:-_`）し、HTTP パスへの注入を防ぐ

## 注意事項

- レスポンスボディは 4MB まで（`QS_HTTP_CLIENT_BODY_BUFFER_SIZE`）。
  大量のログ・一覧は切り詰められます
- Unix ソケット（`/var/run/docker.sock`）への直接接続は非対応です
  （core のソケット層が TCP のみのため）
- TLS（`tcp://...:2376` の mTLS 認証モード）は非対応です
- コアライブラリのビルドオプション `__QS_DEBUG__`（`core/header/qs_core.h`）が有効なため、
  大きな JSON を解析する際に `resize token buffer, ...` のデバッグ行が stdout に出力されます
  （core 側の既存挙動。無効にする場合は該当定義を外して core を再ビルドしてください）
