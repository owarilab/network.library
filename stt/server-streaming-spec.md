# WebSocket PCM Streaming STT 仕様書

## 1. 目的

`sample/stt_server` を、ブラウザから受信した `16kHz / mono / int16 PCM` を逐次処理できる STT サーバーへ段階移行する。

最終目標は `stt/main.cpp` の設計をサーバー側へ移植し、接続ごとの PCM リングバッファから推論 window を切り出して `whisper.cpp` に渡す構成にすること。

この文書では、まず「最小実装」で成立させる対象範囲と、その次の拡張方針を定義する。

## 2. 前提

- 音声認識は STT であり、`whisper.cpp` を利用する
- ブラウザは `AudioWorklet` でマイク入力を取得する
- ブラウザ側で `Float32 -> 16kHz -> Int16 PCM` へ変換して送信する
- 送信フォーマットは WebSocket の text control message + binary PCM chunk とする
- WAV 生成はデバッグ用途に限定し、主処理は PCM ベースで行う

## 3. 参照実装

- `stt/main.cpp`
  - 16kHz モノラル音声をリングバッファへ蓄積
  - fixed-window または VAD により inference window を切り出し
  - `whisper_full()` に `float` 配列として渡す
- `sample/stt_server/main.c`
  - 現在は WebSocket 受信、PCM/WAV 保存、接続管理の最小サンプル
- `sample/stt_server/www/js/room.js`
  - 現在は `stt_init` / raw PCM binary / `stt_stop` の送信経路を持つ

## 4. ゴール

### 4.1 最小実装ゴール

最小実装で満たすべき条件は次の通り。

1. ブラウザが録音中に定期 flush で PCM chunk を送る
2. サーバーが接続ごとに PCM をリングバッファへ蓄積する
3. サーバーが一定量たまった PCM から fixed-window を切り出せる
4. 切り出した window をログまたは debug WAV で確認できる
5. 既存の `stt_stop` でセッション終了できる

この段階では、`whisper.cpp` の実呼び出しは必須ではない。まず transport と buffering を安定化させる。

### 4.2 拡張実装ゴール

最小実装の後に行う対象。

1. fixed-window で `whisper_full()` を呼ぶ
2. 認識結果を WebSocket text message でクライアントへ返す
3. `stt/main.cpp` 相当の overlap を追加する
4. `stt/main.cpp` 相当の VAD を追加する
5. async inference worker 化して受信処理と推論処理を分離する

## 5. 通信仕様

### 5.1 Control message

クライアントから送る text message は JSON とする。

#### `stt_init`

```json
{
  "type": "stt_init",
  "sample_rate": 16000,
  "channels": 1,
  "bits_per_sample": 16
}
```

意味:

- セッション開始
- サーバー側接続状態の初期化
- デバッグ保存開始のトリガ

#### `stt_stop`

```json
{
  "type": "stt_stop"
}
```

意味:

- セッション終了
- 残バッファの最終 flush
- debug WAV の close

### 5.2 Binary message

- 内容: raw PCM (`int16`, little-endian, mono)
- 1 chunk は複数サンプルを含む
- chunk 境界は録音の意味的区切りではなく transport 単位である

## 6. クライアント仕様

対象: `sample/stt_server/www/js/room.js`

### 6.1 音声形式

- target sample rate: `16000 Hz`
- channels: `1`
- sample format: `Int16`

### 6.2 定期 flush 方針

最小実装では `500ms` ごとに flush する。

理由:

- `stt/main.cpp` の `DEFAULT_STEP_MS = 500` と揃えやすい
- 1 chunk あたり約 `16000 bytes` で現行 transport と相性がよい
- レイテンシと送信回数のバランスがよい

### 6.3 クライアント処理順

`startSttRecording()`:

1. WebSocket 接続確認
2. `stt_init` 送信
3. 録音開始
4. flush timer 開始

flush timer:

1. バッファ済み PCM を回収
2. 空なら送らない
3. raw PCM binary として送信

`stopSttRecording()`:

1. flush timer 停止
2. 残り PCM を最終 flush
3. `stt_stop` 送信
4. オーディオリソース解放

## 7. サーバー仕様

対象: `sample/stt_server/main.c`

### 7.1 基本方針

接続ごとに PCM リングバッファを持つ。WAV は主データ構造ではなく debug 出力とする。

### 7.2 接続状態

最小実装の接続状態は少なくとも次を持つ。

```c
typedef struct STT_CONNECTION_STATE_STRUCT {
    char connection_id[256];

    int is_stream_open;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;

    int16_t *ring_buffer;
    int32_t ring_capacity_samples;
    int32_t write_pos;
    int32_t samples_count;

    int16_t *overlap_buffer;
    int32_t overlap_samples;

    FILE *debug_wav_file;
    uint32_t debug_wav_bytes;
} STT_CONNECTION_STATE;
```

### 7.3 リングバッファ容量

最小実装では `30秒` 分を保持する。

- `16000 samples/sec * 30 sec = 480000 samples`
- `int16` のため約 `960KB / connection`

### 7.4 受信処理

`on_ws_event()` の責務は次に限定する。

1. `stt_init` で接続状態を初期化
2. binary chunk をリングバッファへ追記
3. 必要なら debug WAV へも追記
4. `stt_stop` で終了フラグを立てる

`on_ws_event()` の中で重い推論処理は行わない。

## 8. 推論 window 仕様

### 8.1 fixed-window

最初に実装するのは fixed-window モード。

- `step_ms = 500`
- `keep_ms = 200`

16kHz 前提のサンプル数:

- `step_samples = 8000`
- `keep_samples = 3200`

### 8.2 window 組み立て

`stt/main.cpp` に合わせて次の形で構成する。

```text
[previous tail keep_samples] + [new step_samples]
```

処理順:

1. リングバッファから `step_samples` を取り出す
2. 前回末尾 `keep_samples` を前置する
3. inference 用バッファを構築する
4. 完了後、今回 window の末尾 `keep_samples` を保存する

## 9. VAD 方針

VAD は最小実装の対象外とする。

ただし、拡張時は `stt/main.cpp` の `vad_simple()` と同じ考え方を移植する。

予定仕様:

- `probe_length_ms = 2000`
- `inference_length_ms = 5000`
- `cooldown_ms = 2000`
- エネルギーベースの simple VAD

## 10. Debug WAV 方針

WAV 保存は次のいずれかに限定する。

1. セッション全体の transport 確認用 dump
2. inference window 単位の dump

運用時の主経路は PCM リングバッファであり、WAV 保存は必須機能ではない。

## 11. 実装フェーズ

### Phase 1: 定期 flush 化

対象:

- `sample/stt_server/www/js/room.js`

内容:

- `_sttFlushTimer` を追加
- `500ms` ごとの flush を追加
- `stopSttRecording()` は最終 flush + `stt_stop` のみにする

### Phase 2: リングバッファ化

対象:

- `sample/stt_server/main.c`

内容:

- `wav_file` 中心の状態から `ring_buffer` 中心の状態へ移行
- `append_pcm_to_ring_buffer()` を実装
- debug WAV はオプション化

### Phase 3: fixed-window 切り出し

対象:

- `sample/stt_server/main.c`

内容:

- `step_samples` 到達時に window を切り出す
- overlap を追加する
- まずはログで window サイズと切り出し結果を検証する

### Phase 4: whisper.cpp 呼び出し

対象:

- `sample/stt_server/main.c`
- 必要なら `stt/` 側の共通コード抽出

内容:

- `int16 -> float` 変換
- `whisper_full()` 実行
- 結果 text を WebSocket 返信

### Phase 5: VAD と非同期化

内容:

- `vad_simple()` 移植
- 推論 worker 導入
- 連続接続時の負荷分離

## 12. 非対象

この仕様書の時点では次は扱わない。

- TTS 機能
- 複数モデル切り替え
- 話者分離
- 永続保存を前提としたフル音声アーカイブ

## 13. 現時点の設計判断

- 正式な主経路は PCM ベースとする
- WAV は debug 用に残す
- 最初は fixed-window で成立させる
- VAD は後段で追加する
- `stt/main.cpp` の数値パラメータを初期値の基準とする