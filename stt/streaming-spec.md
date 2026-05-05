# ストリーム入力・逐次出力 仕様書

## 1. 概要

ファイルベースのバッチ処理から、リアルタイムマイク入力 + チャンク単位での逐次テキスト出力へ改修する。whisper.cpp の公式 `examples/stream/` パターンを参考にする。

## 2. 環境要件

| 要素 | 要件 |
|------|------|
| OS | WSLg 有効な WSL2（PulseAudio/PipeWire 経由で Windows マイクにアクセス）。Windows「設定 > プライバシーとセキュリティ > マイク」でマイク許可が必要 |
| ライブラリ | SDL2 >= 2.0.x（オーディオキャプチャ対応） |
| サンプルレート | 16 kHz モノラル（whisper の要件と一致） |

## 2.5 事前準備ステップ

実装前に以下の準備を完了させる。

### Step 1: SDL2 のインストール

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev
```

SDL2 が正しくインストールされたか確認:

```bash
pkg-config --modversion sdl2
# Expected output: 2.28.x (or similar)
```

### Step 2: WSLg マイク認識の確認

マイクが SDL2 でキャプチャできることを先に確認する。

```bash
cat <<'EOF' > /tmp/check_mic.cpp
#include <SDL2/SDL.h>
#include <cstdio>
int main() {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    int n = SDL_GetNumAudioDevices(SDL_TRUE);
    if (n <= 0) {
        printf("No capture devices found.\n");
        fprintf(stderr, "SDL_GetError: %s\n", SDL_GetError());
        fprintf(stderr, "Check: Windows Settings > Privacy & Security > Microphone access\n");
        SDL_Quit();
        return 1;
    }
    printf("%d capture device(s):\n", n);
    for (int i = 0; i < n; i++) {
        printf("  [%d] %s\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }
    SDL_Quit();
    return 0;
}
EOF
g++ /tmp/check_mic.cpp -lSDL2 -o /tmp/check_mic && /tmp/check_mic
```

**デバイスが 1 つ以上表示されない場合:**
- Windows「設定 > プライバシーとセキュリティ > マイク」でマイクアクセスを許可
- `pactl info` で PulseAudio サーバーが動いているか確認
- WSLg が有効か: `echo $DISPLAY`（値があれば有効）

### Step 3: common-sdl.h / common-sdl.cpp のコピー

whisper.cpp 本家のストリーム処理実装からファイルをプロジェクトにコピーする。

```bash
cp ~/whisper.cpp/examples/common-sdl.h ~/myproj/headers/
cp ~/whisper.cpp/examples/common-sdl.cpp ~/myproj/headers/
```

### Step 4: whisper.cpp のビルド済みライブラリ確認

```bash
ls ~/whisper.cpp/build/src/libwhisper.so \
   ~/whisper.cpp/build/ggml/src/libggml-base.so \
   ~/whisper.cpp/build/ggml/src/libggml-cpu.so \
   ~/whisper.cpp/build/ggml/src/libggml-cuda.so
```

いずれも存在することを確認。なければ先に whisper.cpp をビルド:

```bash
cd ~/whisper.cpp
cmake -B build -DGGML_CUDA=1
cmake --build build -j --config Release
```

## 3. アーキテクチャ

```
┌──────────────┐    ring buffer     ┌─────────────────┐
│ SDL2          │ ◄══════════════► │ audio_async      │
│ capture       │   (F32 samples)  │ (common-sdl.cpp) │
│ callback      │                  │ circular buffer  │
└───────┬───────┘                  └────────┬─────────┘
        │ SDL_PollEvent                    │ get(step_ms, pcmf32)
        │                                  ▼
        │                     ┌─────────────────────┐
        │                     │ overlap + whisper_full│
        │                     │ (chunk inference)    │
        │                     └──────────┬───────────┘
        │                                │
        │                                ▼
        │                     ┌─────────────────────┐
        │                     │ 逐次出力             │
        │                     │ t0 → t1 text         │
        │                     └─────────────────────┘
```

## 4. 変更対象ファイル

| ファイル | 変更内容 |
|---------|---------|
| `main.cpp` | リファクタ（バッチ処理 → ストリーム処理） |
| `CMakeLists.txt` | SDL2リンクの追加 |
| `headers/common-sdl.h` | **新規**（SDL2 キャプチャラッパ） |
| `headers/common-sdl.cpp` | **新規**（ring buffer 実装） |

## 5. メインプログラム (`main.cpp`) の仕様

### 5.1 処理フロー

```
初期化:
  ├─ モデルロード (whisper_init_from_file_with_params)
  ├─ SDL2 init + capture device open
  └─ whisper_full_params 設定

メインループ:
  while (sdl_poll_events()) {
      1. audio.get(step_ms, pcmf32_new)       // ring buffer からチャンク取得
      2. overlap 処理 (pcmf32_old + pcmf32_new)
      3. whisper_full()                        // チャンク推論
      4. セグメント逐次出力                    // get_segment_text/t0/t1
      5. pcmf32_old = pcmf32_tail              // 次のチャンク用に末尾を保存
  }

終了:
  ├─ SDL2 quit
  └─ whisper_free(ctx)
```

### 5.2 コマンドライン引数

```
./myproj <model.ggml.bin> [options]
```

| オプション | デフォルト | 説明 |
|-----------|-----------|------|
| `--step N` | 500 | チャンク間隔 (ms) |
| `--keep N` | 200 | overlap の長さ (ms) |
| `--vad` | **on** | VAD モード（音声検出時のみ推論）※推奨 |

#### step_ms / keep_ms の選び方（用途別推奨値）

| 用途 | `step_ms` | `keep_ms` | 備考 |
|------|-----------|-----------|------|
| ボイスコマンド | 200~300 | 100~150 | 低レイテンシ優先、単語が短い |
| 議事録・会話 | 500~1000 | 200~300 | バランス型、文の途中で切れないようやや長め |
| 発言遅延最小化 | 100~200 | 100 | CPU 負荷高、リアルタイム性最優先 |

> **keep_ms は必須。** これがないと単語途中（例: "hello [world→"）で切れた際、認識が崩れる。

#### VAD モードの重要性

VAD は「オプション」ではなく**推奨（デフォルト on）**とする理由:
- 無音状態で推論を回し続けると、Whisper がハルシネーションして「幻聴」（同じ単語の繰り返し出力）を起こす
- エネルギーベースの `vad_simple` を使用し、閾値以上の音量がある時のみ `whisper_full()` を呼ぶ
- 沈黙検知後、最低2秒間は再推論しない（ノイズによる頻繁なトリガを防ぐ）

### 5.3 whisper_full_params 設定値

| フィールド | 値 | 理由 |
|-----------|-----|------|
| `language` | `"ja"` | 日本語認識 |
| `translate` | `false` | 翻訳なし |
| `print_progress` | `false` | 自前で逐次出力するため |
| `print_realtime` | `false` | 同上 |
| `print_timestamps` | `true` | タイムスタンプ付き出力 |
| `single_segment` | `true` | チャンクごとに1セグメントとして扱う（端切れ防止） |
| `no_context` | `true` | 前回のデコーダ状態を再利用しない（シンプル化） |

### 5.4 オーバーラップ処理

チャンク境界で切り捨てられた音声が認識品質を下げるため、前回の末尾 N ミリ秒を重ねる。

```
前回: [===================]
           ↑ keep_ms (200ms)
現在:              [====new====]
結果:  [old_tail][new_chunk] → whisper_full()
次回の old = result の末尾 keep_ms
```

### 5.5 VAD モード

デフォルト **on**。固定間隔ではなく「沈黙検知時に推論」する。

- エネルギーベースの simple VAD を使用（whisper.cpp `common.cpp` の `vad_simple` を流用）
- 無音状態で推論を回し続けるとハルシネーション（幻聴）の原因になるため、**必須レベルで推奨**
- 沈黙中: 次の発話検出まで待機（最低2秒のスレッショルド）
- 発話検出時: `length_ms` (デフォルト5000ms) 分のオーディオで推論実行

### 5.6 サンプリングレート設定の注意点

SDL2 の `AudioSpec` で **16000 Hz** を指定する。whisper は 16kHz モノラルを前提としており、
これ以外のレートでは正しく動作しない（または精度が著しく低下する）。

## 6. audio_async クラス (`common-sdl.cpp`) の仕様

### 6.1 機能

| 機能 | 説明 |
|------|------|
| SDL2 capture device 初期化 | F32 フォーマット、16kHz モノラル |
| コールバック駆動 ring buffer | 非同期にキャプチャしたサンプルを蓄積 |
| `get(ms, result)` | 最新 ms ミリ秒分のデータをコピー返す（wrap-around対応） |
| `pause()` / `resume()` | キャプチャ一時停止・再開 |

### 6.2 ring buffer 仕様

- 容量: `len_ms * sample_rate / 1000` samples（コンストラクタで指定、デフォルト 30秒分）
- フォーマット: float32, 16kHz, モノラル
- コールバック: SDL が F32 データを直接渡す（変換不要）

## 7. CMakeLists.txt 変更点

Linux では **リンク順が重要**（依存関係を逆順に並べる）。

```cmake
find_package(SDL2 REQUIRED)
target_link_libraries(myproj PRIVATE whisper ggml-base ggml-cpu ggml SDL2::SDL2 pthread dl)
target_include_directories(myproj PRIVATE ${SDL2_INCLUDE_DIR})
```

> **注意**: `whisper` を先頭に、依存ライブラリを後ろに。逆順だと「undefined symbol」でリンク失敗する。

### 7.1 SDL2 capture 設定の注意点

- サンプリングレートは必ず **16000 Hz** に設定 (`AudioSpec.freq = 16000`)
- フォーマット: `AUDIO_F32`（whisper が float32 を直接受け取るため）
- チャンネル: モノラル（`AudioSpec.channels = 1`）

## 8. 出力フォーマット

```
Model loaded: <path>
Audio streaming started...
00:00.00 --> 00:03.50 こんにちは
00:03.50 --> 00:07.20 世界
---
```

- チャンクごとに逐次出力（`-->` で時刻範囲とテキスト）
- ループ終了時またはエラー時に `---` を出力して区切りを示す

## 9. エラーハンドリング

| 状況 | 動作 |
|------|------|
| SDL2 init 失敗 | エラーメッセージ → 終了 |
| capture device なし | エラーメッセージ → 終了 |
| whisper_full 失敗 | エラーメッセージ → リソース解放 → 終了 |
| ring buffer アンダーフロー（データ不足） | チャンクをスキップして継続 |
| SDL_QUIT イベント | グレースフルシャットダウン |

## 10. 実装ステップ

| ステップ | タスク |
|---------|--------|
| 1 | `common-sdl.h` / `common-sdl.cpp` を headers にコピー |
| 2 | CMakeLists.txt を SDL2 対応に変更（リンク順に注意） |
| 3 | main.cpp をリファクタ（ストリーム処理 + VAD モード実装） |
| 4 | ビルド・動作確認 |
