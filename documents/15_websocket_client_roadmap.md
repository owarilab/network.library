# WebSocket Client 改修計画

## 目的

core の plain WebSocket client を、`ws://` の最小実装から、非同期通信と異常系を安全に扱えるライブラリ機能へ段階的に拡張する。

対象は `core/header/qs_api.h`、`core/src/qs_api.c`、および `sample/websocket_client` とする。WSS は transport 層を分離した後の後続項目とし、現段階では対象外とする。

## 現在の実装状況

- [x] nonblocking TCP 接続
- [x] HTTP Upgrade request の送信
- [x] `Sec-WebSocket-Accept` の検証
- [x] client-to-server masked frame の送信
- [x] text / binary / ping / pong / close の基本処理
- [x] `sample/http_server_simple` を利用した room create / join / message / leave の統合テスト

現在の制約:

- 送信は `qs_send_all()` を直接利用しており、送信キューを持たない。
- fragmented message と 64-bit payload length を扱わない。
- handshake header の検証とエラー通知が最小限である。
- `QS_CLIENT_CONTEXT` に WebSocket の明示的な状態・失敗理由がない。

## 進捗ルール

- 未着手: `- [ ]`
- 実装中: `- [-]`
- 完了: `- [x]`
- 各項目は「実装」「対象テスト」「完了条件」を満たしてから完了へ変更する。
- protocol の互換性に影響する変更は、unit test と `sample/websocket_client` の統合テストを両方実行する。

## Phase 1: Nonblocking 送信キュー

状態: `- [ ]`

### 実装

- [ ] `QS_CLIENT_CONTEXT` に送信待ち frame のキュー、送信オフセット、送信残量を追加する。
- [ ] Upgrade request、text、binary、pong、close を共通の enqueue 経路へ移す。
- [ ] `api_qs_client_update()` で writable 状態を確認し、部分送信を再開する。
- [ ] `EAGAIN` / `EWOULDBLOCK` を送信失敗ではなく送信待ちとして扱う。
- [ ] 送信バッファ上限と、上限超過時のエラーを定義する。

### 対象テスト

- [ ] send hook で短い write を返し、全 frame が複数 update で送られることを確認する。
- [ ] send hook で一時的に `EAGAIN` を返し、接続が維持されることを確認する。
- [ ] 大きな payload 送信中に ping を enqueue し、順序が維持されることを確認する。

### 完了条件

- [ ] `qs_send_all()` を WebSocket client の送信フローから除去する。
- [ ] partial send と `EAGAIN` のテストが成功する。
- [ ] 既存の room 統合テストが成功する。

## Phase 2: 状態・エラー API

状態: `- [-]`

### 実装

- [x] 接続状態を表す `QS_WEBSOCKET_STATE` を追加する。
- [x] `CONNECTING`、`HANDSHAKING`、`OPEN`、`CLOSING`、`CLOSED`、`ERROR` を定義する。
- [x] `api_qs_websocket_client_get_state()` を追加する。
- [x] `api_qs_websocket_client_get_error()` を追加する。
- [ ] WebSocket open、message、pong、close、error の callback を役割ごとに分離する。
- [ ] 現在の WebSocket 接続完了 callback は、互換性を保ちつつ `on_open` として文書化する。

### 対象テスト

- [ ] TCP 接続、handshake 成功、close、handshake 失敗で状態遷移を確認する。
- [ ] callback が各イベントで一度だけ呼ばれることを確認する。

### 完了条件

- [ ] 利用側が内部構造体へアクセスせず WebSocket の状態とエラーを判定できる。
- [ ] close 後と error 後に追加送信が拒否される。

## Phase 3: RFC 6455 Frame 対応の拡張

状態: `- [-]`

### 実装

- [x] `FIN=0` の fragmented message と continuation frame (`opcode=0`) を再構成する。
- [x] payload length `127` の 64-bit length を処理する。
- [x] text / binary のメッセージサイズ上限を設定可能にする。
- [x] control frame が `FIN=1` かつ 125 bytes 以下であることを検証する。
- [x] 不正 frame に close code `1002` を返す。
- [x] close status code と reason を解析し、利用側に渡せるようにする。

### 対象テスト

- [ ] 125、126、65535、65536 bytes の frame header を検証する。
- [ ] 分割された HTTP response と WebSocket frame を複数回の recv で処理する。
- [ ] fragmented text message と fragmented binary message を処理する。
- [ ] 不正な control frame と不正な continuation frame を拒否する。

### 完了条件

- [ ] RFC 6455 の通常メッセージ、制御 frame、fragmentation を扱える。
- [ ] 固定バッファを超える入力がメモリ破壊や無限待機にならない。

## Phase 4: Handshake と timeout の堅牢化

状態: `- [ ]`

### 実装

- [ ] HTTP status code `101` を header 単位で検証する。
- [ ] `Upgrade: websocket` と `Connection: Upgrade` token を大文字小文字を区別せず検証する。
- [ ] `Sec-WebSocket-Accept` を header 値として完全一致で検証する。
- [ ] host と port から `Host` header を生成する。
- [ ] handshake header の最大サイズを設定する。
- [ ] handshake timeout と close handshake timeout を追加する。

### 対象テスト

- [ ] 不正な status / Upgrade / Connection / Accept header を拒否する。
- [ ] header が分割受信された場合に成功する。
- [ ] header 上限超過と timeout が `ERROR` 状態になる。

### 完了条件

- [ ] 不正な Upgrade response を開通扱いしない。
- [ ] 応答しない peer に対して client が期限なく接続を保持しない。

## Phase 5: テストとドキュメント整備

状態: `- [ ]`

### 実装

- [ ] SHA-1 / Base64 による `Sec-WebSocket-Accept` の既知ベクタテストを追加する。
- [ ] frame encode/decode の unit test を追加する。
- [ ] localhost echo server を使う protocol 統合テストを追加する。
- [ ] `sample/websocket_client` を end-to-end room テストとして維持する。
- [ ] [documents/07_api_server_client.md](07_api_server_client.md) に client API の利用例と制約を追記する。

### 完了条件

- [ ] protocol の主要な分岐を自動テストで再現できる。
- [ ] API の状態遷移、callback、payload の寿命が文書化されている。

## Phase 6: WSS Transport Adapter

状態: `- [ ]`

### 実装

- [ ] WebSocket frame 層と TCP/TLS transport 層の境界を明確にする。
- [ ] 既存 SSL module の send / recv hook を WebSocket client で利用できるようにする。
- [ ] `wss://` 用の初期化 API を追加する。
- [ ] TLS handshake と WebSocket HTTP Upgrade の timeout を個別に扱う。

### 完了条件

- [ ] `ws://` と `wss://` が同一の frame parser と callback API を共有する。
- [ ] TLS 固有の条件分岐が WebSocket frame parser に入らない。

## 推奨実施順

1. Phase 1: Nonblocking 送信キュー
2. Phase 2: 状態・エラー API
3. Phase 3: RFC 6455 Frame 対応の拡張
4. Phase 4: Handshake と timeout の堅牢化
5. Phase 5: テストとドキュメント整備
6. Phase 6: WSS Transport Adapter

送信キューを最初に実装する。これにより、以降の frame 機能が nonblocking socket 上で安全に動作する基盤を共有できる。