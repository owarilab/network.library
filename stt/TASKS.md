# 実装タスク一覧・進捗確認シート

## 全体マップ

```
[T1] SDL2 ファイルコピー ──→ [T2] CMakeLists.txt 改修 ──→ [T3] main.cpp リファクタ
                                                          │
                                    ┌─────────────────────┼─────────────────────┐
                                    ▼                     ▼                     ▼
                              [T4] VAD モード固定ウィンドウ   [T5] VAD モード音声検出   [T6] 出力・終了処理
                                    │                         │                     │
                                    └────────┬────────────────┴─────────┬───────────┘
                                             ▼                        ▼
                                      [T7] ビルド確認               [T8] 動作確認
```

---

## T1: SDL2 キャプチャファイルの準備

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `headers/common-sdl.h`, `headers/common-sdl.cpp` |
| **コピー元** | `~/whisper.cpp/examples/common-sdl.h`, `.cpp` |

### 手順
1. whisper.cpp 本家から `common-sdl.h`, `common-sdl.cpp` を `myproj/headers/` にコピー
2. ヘッダの include パスが合っていることを確認（`#include <SDL2/SDL.h>` → `#include <SDL.h>` に変更が必要なら）

### 完了チェックリスト
- [ ] `headers/common-sdl.h` が存在する
- [ ] `headers/common-sdl.cpp` が存在する
- [ ] ヘッダの include パスがプロジェクト内で解決できる

---

## T2: CMakeLists.txt の SDL2 対応改修

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `CMakeLists.txt` |

### 手順
1. `find_package(SDL2 REQUIRED)` を追加
2. `target_link_libraries` に SDL2、pthread、dl を追加（リンク順に注意）
3. `target_include_directories` に SDL2 の include dir を追加

### 完了チェックリスト
- [ ] `find_package(SDL2 REQUIRED)` が存在する
- [ ] リンク順: whisper → ggml-* → SDL2::SDL2 → pthread → dl
- [ ] `target_include_directories` に `${SDL2_INCLUDE_DIR}` が含まれる

---

## T3: main.cpp - CLI パラメータ構造化

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `main.cpp` |

### 手順
1. 既存のハードコードされたパラメータを構造体で管理するように変更
2. `--step`, `--keep`, `--vad` の CLI パースを実装
3. デフォルト値の設定（step=500, keep=200, vad=true）

### 完了チェックリスト
- [ ] `struct params` が定義されている
- [ ] `--step N`, `--keep N`, `--vad` のパースが実装されている
- [ ] デフォルト値が正しく設定されている

---

## T4: main.cpp - 初期化処理（モデル + SDL2）

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `main.cpp` |

### 手順
1. モデルロード部分は既存の `whisper_init_from_file_with_params()` を維持
2. SDL2 初期化 + capture device open のコードを追加（audio_async）
3. whisper_full_params 設定を構造体と連動させる
4. マイク認識失敗時のエラーハンドリング

### 完了チェックリスト
- [ ] `audio_async` が初期化され、`resume()` されている
5. モデルロード失敗時に SDL2 リソースがリークしない
- [ ] audio init 失敗時に whisper context が解放される
- [ ] whisper_full_params に CLI パラメータが反映されている

---

## T5: main.cpp - VAD モード（音声検出時のみ推論）

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `main.cpp` |

### 手順
1. `vad_simple()` を main.cpp にインライン実装（whisper.cpp の common.cpp から流用）
2. VAD モード（step=0）の処理分岐を実装:
   - 沈黙中: 100ms sleep + ループ継続
   - 発話検出: `length_ms` 分のオーディオを取得して推論
3. 前回の検知時刻から2秒間は再推論しないスレッショルド

### 完了チェックリスト
- [ ] `vad_simple()` が実装されている（エネルギーベースの VAD）
- [ ] VAD=true の場合、発話検出時のみ whisper_full() を呼び出す
- [ ] 沈黙検知後、最低2秒間はスキップする

---

## T6: main.cpp - 固定ウィンドウモード + オーバーラップ

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `main.cpp` |

### 手順
1. VAD=false の場合の処理を実装:
   - `audio.get(step_ms, pcmf32_new)` でチャンク取得
   - データ不足時は sleep + retry（アンダーフロー対策）
   - オーバーフロー検出時は `audio.clear()` でバッファ破棄
2. 前回の末尾 `keep_ms` を重ね合わせる overlap 処理
3. チャンク組み立て: `[pcmf32_old_tail][pcmf32_new]` → whisper_full()

### 完了チェックリスト
- [ ] step_ms ミリ秒分のオーディオを取得するループがある
- [ ] オーバーフロー（データ不足）時に clear() + retry する
- [ ] 前回の末尾 keep_ms が重ね合わせられている
- [ ] チャンク組み立て後、pcmf32_old に結果の末尾を保存

---

## T7: main.cpp - 逐次出力・終了処理

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ファイル** | `main.cpp` |

### 手順
1. whisper_full() 後の結果取得ループ:
   - `whisper_full_n_segments()` でセグメント数取得
   - 各セグメントを `t0 --> t1 text` フォーマットで出力
2. VAD モードの出力: START/END マーカー付き
3. 固定ウィンドウモードの出力: クリア後の行更新（`\33[2K\r`）
4. メインループ終了時のグレースフルシャットダウン:
   - `audio.pause()` → `whisper_free(ctx)` → SDL_Quit()

### 完了チェックリスト
- [ ] セグメントごとに `[HH:MM.ss --> HH:MM.ss] text` で出力
- [ ] fflush(stdout) している
- [ ] Ctrl+C (SDL_QUIT) でグレースフルシャットダウン
- [ ] ループ終了後にリソースがすべて解放される

---

## T8: ビルド確認

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **対象ディレクトリ** | `build/` |

### 手順
```bash
cd ~/myproj && rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

### 完了チェックリスト
- [ ] cmake がエラーなく完了する
- [ ] ビルドが成功する（warning は許容）
- [ ] `build/myproj` バイナリが生成される
- [ ] `-h` / `--help` でヘルプが表示される

---

## T9: 動作確認

| 項目 | 内容 |
|------|------|
| **ステータス** | ⬜ pending |
| **前提条件** | マイク接続済み、SDL2 デバイス認識済み |

### テストケース

| No. | ケース | 期待値 | 結果 |
|-----|--------|--------|------|
| T9-1 | VAD モード（デフォルト）で話す | 発話検出時に逐次テキスト出力される | ⬜ |
| T9-2 | `--vad false` で固定ウィンドウ | step_ms ごとに逐次テキスト出力される | ⬜ |
| T9-3 | `--step 1000 --keep 300` でカスタム間隔 | 指定した間隔で処理される | ⬜ |
| T9-4 | マイクなし / device なし | エラーメッセージが表示されて終了する | ⬜ |
| T9-5 | Ctrl+C で終了 | グレースフルシャットダウン、リークなし | ⬜ |

---

## 進捗サマリー

| ステータス | タスク数 |
|-----------|---------|
| ⬜ pending | T1-T9 (全9タスク) |
| 🔄 in_progress | — |
| ✅ completed | — |
