# docker_cli

`sample/http_client` と同じ qs core + ssl モジュール（`QS_HTTP_CLIENT_CONTEXT`）のみで、
Docker Engine API（plain HTTP）をコマンド引数から操作する CLI ツールのサンプルです。

- 通信は plain HTTP です。Unix domain ソケット（既定）または TCP（`--host`/`--port`）から選択できます
- TLS（mTLS 認証モード）は未対応

## ビルド

```bash
cd sample/docker_cli
make
```

## 前提

### 既定: Unix ソケット直結（設定不要）

`/var/run/docker.sock` が存在する環境では、**プロキシ・TCP 設定は一切不要**です。
オプションを付けずに実行すると自動的に Unix ソケットが使われます。

```bash
$ ./qs_docker_cli version
Version        29.1.3
...
```

### 方法1: TCP で使う場合（リモート、または Unix ソケットが使えない場合）

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

### 方法2: 既存の Unix ソケットを TCP に中継する（`--host/--port` 指定で TCP 経由にしたい場合のみ）

> 既定では Unix ソケット直結が使えるため、本サンプルで中継は不要です。
> 参照のため残しています。

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
docker_cli [--host HOST] [--port PORT] [--unix-socket PATH] [--format json] <command> [args]

共通オプション:
  --host HOST           TCP 接続時の Docker Engine のホスト（既定: 127.0.0.1）
  --port PORT           TCP 接続時の Docker Engine のポート（既定: 2375）
  --unix-socket PATH    Unix domain ソケットで接続
  --format json         対象コマンドのレスポンスをそのまま（生 JSON）で表示
                        （対象: version / info / ps / images / history / inspect /
                         top / port / wait / stats / network ls / volume ls / prune。
                         その他のコマンドでは無視される）

エンドポイント解決の優先順位:
  1. --unix-socket PATH（明示指定）
  2. --host / --port のいずれか指定時は TCP
  3. 何も指定しない場合は /var/run/docker.sock が存在すれば Unix、
     なければ TCP 127.0.0.1:2375

コマンド:
  version                  エンジンのバージョン情報を表示（GET /version）
  info                     エンジン情報を表示（GET /info）
  ps [--all]               コンテナ一覧（GET /containers/json）
  images                   イメージ一覧（GET /images/json）
  run [-p HOSTPORT:PORT]... IMAGE [CMD...]
                           コンテナ作成+起動（POST /containers/create → POST /containers/{id}/start）
  start <container>        コンテナ起動（POST /containers/{id}/start）
  stop <container> [-t SECONDS]  コンテナ停止（POST /containers/{id}/stop?t=N、既定 10）
  restart <container>      コンテナ再起動（POST /containers/{id}/restart）
  kill <container> [--signal NAME]  シグナル送信（既定: KILL、POST /containers/{id}/kill）
  rm <container> [--force] コンテナ削除（DELETE /containers/{id}）
  logs <container>         コンテナログ表示（GET /containers/{id}/logs?stdout=1&stderr=1）
  top <container>          実行中プロセス一覧（GET /containers/{id}/top）
  port <container>         ポートマッピング表示（GET /containers/{id}/json の NetworkSettings.Ports）
  wait <container>         終了を待って終了コードを表示（POST /containers/{id}/wait）
  stats <container>        リソース使用量の単発サンプル（GET /containers/{id}/stats?stream=false）
  rmi <image> [--force]    イメージ削除（DELETE /images/{name}）
  tag <SOURCE> <TARGET>    イメージにタグ付け（POST /images/{name}/tag）
  history <image>          イメージ履歴（GET /images/{name}/history）
  inspect <CONTAINER|IMAGE>  コンテナまたはイメージの詳細表示
                             （container 優先、404 時は image）
  pull IMAGE[:TAG]          イメージをプル（POST /images/create）
  exec <container> [CMD...] コンテナ内でコマンド実行 [-t]
                             CLI の終了コードは実行コマンドの終了コードになる
  network ls               ネットワーク一覧（GET /networks）
  volume ls                ボリューム一覧（GET /volumes）
  prune <kind> [--filter key=value]
                           images/containers/volumes を prune（POST .../prune）
```

`prune` は破壊的操作です。`--filter` 付きで対象を絞ってください
（例: `--filter label=team=a` でラベル付きのみに限定）。
フィルタ値は `label=KEY=VALUE` の形でも使えます
（Engine API の `filters` パラメータのリスト形式 `{"label":["KEY=VALUE"]}` に変換されます）。

`<container>` / `<image>` には短縮 ID（全 hex の先頭 1〜63 文字）も使えます（system の `docker` と同じ挙動）。
短縮 ID は `GET /containers/json?all=1` / `GET /images/json` で先頭一致を解決します:

- 1 件一致: 全 ID に解決してそのまま処理
- 0 件一致 / 2 件以上（曖昧）: エラー終了
- 非 hex の名前・64 桁の全 ID: 解決なしでそのまま渡す（従来の 404 経路を維持）
- `inspect` のみ: container 側が見つからなければ image 側の解決にフォールスルーする
  （例: イメージの短縮 ID で `inspect` できる）。曖昧（2 件以上）の場合は即エラー
- 対象: container 系（start/stop/restart/kill/rm/logs/top/port/wait/stats/exec）、
  image 系（rmi/history）、inspect。`pull` / `tag` / `run` は対象外（リモート参照のため）

## 動作例

```bash
$ ./qs_docker_cli version
Version        29.1.3
ApiVersion     1.52
MinAPIVersion  1.44
GoVersion      go1.24.4
Os             linux
Arch           amd64
KernelVersion  5.15.0-91-generic

$ ./qs_docker_cli ps --all
CONTAINER ID   IMAGE                        NAME                     STATUS
111973a01dff   nginx:latest                 nginx                    Exited (0) 3 months ago
...

$ ./qs_docker_cli images
IMAGE ID       TAGS                             SIZE
9ab6d30d9c82   nginx:latest                     187 MB
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

$ ./qs_docker_cli run alpine:3.21 sleep 300
7a3f...
$ ./qs_docker_cli top 7a3f...
UID PID PPID C STIME TTY TIME CMD
root 27561 27537 14 18:49 ? 00:00:00 sleep 300
$ ./qs_docker_cli stats 7a3f...
Memory:     812.00 KB / 49.01 GB
CPU total:  0.020 s
Net RX:     1.06 KB
Net TX:     0.12 KB
$ ./qs_docker_cli kill 7a3f...
killed: 7a3f... (signal: KILL)
$ ./qs_docker_cli wait 7a3f...
exit code: 137
$ ./qs_docker_cli rm 7a3f...
removed: 7a3f...

$ ./qs_docker_cli tag alpine:3.21 local/alpine-test:v1
tagged: alpine:3.21 as local/alpine-test:v1
$ ./qs_docker_cli history alpine:3.21
IMAGE ID               SIZE CREATED BY
6aeafe801b80        0.00 KB CMD ["/bin/sh"]
<missing>           7.47 MB ADD alpine-minirootfs-3.21.6-x86_64.tar.gz / # buildkit
$ ./qs_docker_cli rmi local/alpine-test:v1
untagged: local/alpine-test:v1

$ ./qs_docker_cli inspect alpine:3.21
ID           6aeafe801b80
Tags         alpine:3.21
Digests      alpine@sha256:c3f8e73fdb79...
Created      2026-01-28T01:18:43.127739215Z
Size         7.47 MB
Architecture amd64
Os           linux
Cmd          /bin/sh
Env:
  PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
WorkingDir   /

$ ./qs_docker_cli run alpine:3.21 sleep 300
ed2fca2ed3ad...
$ ./qs_docker_cli inspect ed2fca2ed3ad
ID           ed2fca2ed3ad
Name         /eager_raman_1
Image        sha256:6aeafe801b80...
Created      2026-11-21T12:33:22.123456789Z
State:      running (pid: 58144)
StartedAt    2026-11-21T12:33:22.234567890Z
Cmd          sleep 300
...
Network      bridge
Restart      no
Mounts:
  (none)
Ports:
  (none)
IP:
  bridge: 172.17.0.2

$ ./qs_docker_cli exec ed2fca2ed3ad sh -c "echo hello; echo to-stderr >&2; exit 7"
hello
to-stderr
# （このコマンドの終了コードは 7）
$ ./qs_docker_cli exec ed2fca2ed3ad -t echo tty-mode
tty-mode

$ ./qs_docker_cli pull alpine:3.19
Pulling from library/alpine
pulling: 17a39c0ba978
Digest: sha256:6baf43584bcb78...
Status: Downloaded newer image for alpine:3.19

$ ./qs_docker_cli run -p 18435:80 alpine:3.21 sleep 300
212bb7aae0fb...
$ ./qs_docker_cli port 212bb7aa
80/tcp -> 0.0.0.0:18435
80/tcp -> :::18435
$ ./qs_docker_cli stop 212bb7aa -t 5
stop: 212bb7aae0fb...
$ ./qs_docker_cli rm 212bb7aa
removed: 212bb7aa

$ ./qs_docker_cli --format json version | python3 -m json.tool
{
    "Platform": {
        "Name": ""
    },
    "Version": "29.1.3",
    "ApiVersion": "1.52",
    ...
}
$ ./qs_docker_cli --format json inspect e50d940e | python3 -c "import json,sys; print(json.load(sys.stdin)['Id'])"
sha256:e50d940e9d2c...
$ ./qs_docker_cli logs aaaaaaaa11
docker_cli: error: no container matches 'aaaaaaaa11'
```

エラー（HTTP 400 以上）は Docker エラー JSON の `message` を表示し、非ゼロ終了します。

## 実装のポイント

- 通信は `sample/http_client` と同じワンショットパターン:
  TCP 時は `qs_ssl_module_http_client_connect(host, port, is_ssl=0)`、
  Unix ソケット時は `qs_ssl_module_http_client_connect_unix(path, is_ssl=0)` →
  リクエストバッファに HTTP リクエストを手組み →
  `update` / `sleep` を `QS_SSL_MODULE_PHASE_DISCONNECT` までループ → header/body を読む → `dispose`
- 1 コマンド = 1 接続（`Connection: close`、keep-alive は不使用）
- JSON 解析は `api_qs_json_decode_object`。Docker の配列レスポンス（`/containers/json` 等）は
  トップレベルが配列のため、ボディを `{"items": <body>}` でラップしてからのみオブジェクトとしてデコードする
- 数値は JSON デコーダが int32 範囲なら int32、それ以外は uint64 として格納するため、
  getter も両方試す（`Size` / `MemTotal` など）
- `logs` は Docker raw stream（8 バイトヘッダ + データのフレーム列）をデミュークスして表示する
- `port` は Engine API に専用エンドポイントがないため `GET /containers/{id}/json`（inspect）を
  使い `NetworkSettings.Ports` を整形する（system の `docker port` と同じ方式）
- `stats` / `port` の動的キー（インターフェース名・ポート名）は
  `api_qs_object_get_keys` で列挙して取得する
- `prune --filter` は Engine API の `filters` パラメータ（URL エンコード済み JSON）を構築する。
  形式は `{"key":["value"]}` のリスト形式（2 段形式 `{"label":{"k":["v"]}}` は 400 になる）。
  レスポンスの再割当バイト数フィールドは `SpaceReclaimed`
- `inspect` は container inspect を試し、**404 のときだけ image inspect にフォールスルー**する
  （system の `docker inspect` と同じ解決順序）。表示は選択フィールドのサマリー
- `pull` のレスポンスは JSON オブジェクトの行ストリーム。進捗行は
  `progress` / `progressDetail` フィールドの有無で判定し、層 ID ごとに 1 行だけ
  `pulling: <id12>` を表示してノイズを抑制する（dockerd 29 系は `progressDetail` 形式）
- `exec` は 3 リクエスト（exec 作成 → start → 終了コード取得）。
  リクエストボディは **JSON ビルダにブール値 push が無い**ため
  `snprintf` + `api_qs_json_escape`（core 既存 API）で手組みする。
  出力は `logs` と同じ `print_demuxed` を共用（Tty:false のときのみ）
- `--format json` は `emit_json` ヘルパーで成功時のレスポンスボディをそのまま出力し、
  通常のテーブル表示をスキップする（pretty print しないため `jq` / `python3 -m json.tool`
  に直接パイプできる）。対象外の 204 / テキスト系コマンドでは単に無視される
- ID 短縮形解決は `is_hex_string` + `resolve_container_ref` / `resolve_image_ref`。
  全 hex かつ 1〜63 桁の引数だけ解決対象（64 桁の全 ID・非 hex の名前はパススルー）。
  解決に `ps --all` / `images` のリクエストを 1 件追加する（ワンショット構成では起動コスト増のみ）
- `run -p` は既存の JSON ビルダ（`api_qs_object_push_object` / `api_qs_object_push_array` /
  `api_qs_array_push_object`、`cmd_run` の Cmd 配列と同一パターン）で
  `ExposedPorts` + `HostConfig.PortBindings` を create リクエストボディに追加する
  （ホスト IP 指定・ポート範囲はスコープ外。競合時は Engine が 400 を返す）
- `stop -t N` は `cmd_container_action` の任意クエリサフィックス引数として
  `?t=N` を付与する（start / restart は NULL を渡す）
- パスに埋め込む ID・イメージ名は文字種を制限（英数字と `./:-_`）し、HTTP パスへの注入を防ぐ。
  `prune --filter` の値も JSON を壊す文字（`" \ { } [ ]`）を含む場合は拒否する

## 注意事項

- レスポンスボディは 4MB まで（`QS_HTTP_CLIENT_BODY_BUFFER_SIZE`）。
  大量のログ・一覧は切り詰められます
- Unix ソケット接続は `api_qs_client_init_unix`（core）+
  `qs_ssl_module_http_client_connect_unix`（ssl モジュール）を使用します
  （Windows では未サポートのため、`--unix-socket` 指定時はエラー終了します）
- TLS（`tcp://...:2376` の mTLS 認証モード）は非対応です
- `wait` はコンテナ終了までブロックし、**タイムアウトを持ちません**。
  必要に応じてシェル側で `timeout 60 ./qs_docker_cli wait <id>` のようにしてください
- `exec` の CLI 終了コードは **実行コマンドの終了コード**になります
  （system の `docker exec` と同じ。0/1 規則からの例外。
  例: `exec <c> sh -c "exit 7"` → 終了コード 7）
- `exec` は `-d`（detach）・stdin（`AttachStdin`）・TTY サイズ指定
  （`/exec/{id}/resize`）を非対応としています
- `pull` はローカルに新しいイメージを作成します（既存データは削除しません）。
  進捗行の表示は層ごとに 1 行のサマリーです
- 短縮 ID を渡すと、コマンドごとに一覧取得リクエストが 1 件増えます
  （ワンショット構成の起動コストに含めてください）
- `run -p` は `-p` を 16 件まで、形式は `HOSTPORT:PORT` のみ（ホスト IP 指定・
  ポート範囲・プロトコル指定は非対応）
- `--format json` は対象外の 11 コマンド（run/start/stop/restart/kill/rm/logs/pull/exec/tag/rmi）
  では無視されます（エラーにはなりません）
- `build` / `cp` / `attach` / `events` / `logs --follow` のような
  ストリーミング・対話系エンドポイントは未対応です
