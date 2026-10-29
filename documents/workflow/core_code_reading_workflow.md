# coreコード把握ワークフロー（作業開始前）

このワークフローは、`core` の実装方針・責務分割・実行フローを短時間で把握し、実装作業を始める前に必要な情報を揃えるための手順です。

---

## 1. ゴール

作業開始前に、次を説明できる状態にする。

1. `qs_api`（高レベルAPI）がどこまで責務を持つか  
2. メモリ管理（`api_qs_memory_clean` / `api_qs_memory_free`）の使い分け  
3. サーバーのイベントループと HTTP/WebSocket の処理導線  
4. 変更対象を入れるべきファイル（`header` か `src` か `sample` か）

---

## 2. 事前準備

### 2-1. 全体構造を確認

- [documents/README.md](../README.md)
- [documents/01_core_overview.md](../01_core_overview.md)

### 2-2. `core` ディレクトリの責務を確認

- [core/header/](../../core/header/)
- [core/src/](../../core/src/)

### 2-3. 作業計画ドキュメントの配置を決める

- 明示的な指定がない場合、設計書・調査メモ・実装計画は `work/plan/[作業名]/` に配置する
- 推奨ファイル名
  - `design.md`（設計）
  - `investigation.md`（調査）
  - `tasks.md`（実装タスク）
- `documents/workflow/` はワークフロー本体の保管場所として使い、個別作業の成果物置き場にはしない

---

## 3. 読解フロー

## Phase A: 公開API面を先に固める

### 読むファイル

- [core/header/qs_api.h](../../core/header/qs_api.h)
- [core/header/qs_core.h](../../core/header/qs_core.h)

### 確認ポイント

- サーバー初期化〜ループ系API  
  - `api_qs_server_init`
  - `api_qs_update`
  - `api_qs_sleep`
- イベント登録API  
  - `api_qs_set_on_http_event`
  - `api_qs_set_on_websocket_event`
- メモリAPI  
  - `api_qs_memory_alloc`
  - `api_qs_memory_clean`
  - `api_qs_memory_free`

> この段階では「どの機能が公開されているか」だけを掴む。内部詳細はまだ追いすぎない。

---

## Phase B: メモリ方針を最優先で理解

### 読むファイル

- [core/src/qs_api.c](../../core/src/qs_api.c)
- [core/header/qs_memory_allocator.h](../../core/header/qs_memory_allocator.h)
- [core/src/qs_memory_allocator.c](../../core/src/qs_memory_allocator.c)

### 確認ポイント

1. `api_qs_memory_clean` はメモリプール再利用（中身初期化）  
2. `api_qs_memory_free` はメモリプール破棄  
3. 一時メモリと長期メモリを分ける設計前提

### ここでの判断

- リクエスト単位データは「一時メモリ」で扱う
- 接続継続データ・キャッシュは「長期メモリ」で扱う

---

## Phase C: サーバー実行導線を追う

### 読むファイル

- [core/src/qs_api.c](../../core/src/qs_api.c)
- [core/src/qs_protocol.c](../../core/src/qs_protocol.c)

### 手順

1. `api_qs_server_init` から初期化内容を確認  
2. `api_qs_update` / `api_qs_sleep` のループ前提を確認  
3. `api_qs_on_http_recv` → `api_qs_exec_http` の流れを確認  
4. `http_request_common` で静的ファイル配信の分岐を確認

### 確認ポイント（HTTP）

- ドキュメントルートが `./www` 前提
- `/` で `index.html` を返す
- 拡張子ごとの Content-Type 分岐
- `Cache-Control` / `If-Modified-Since`（304）対応

---

## Phase D: サンプルで実運用イメージを合わせる

### 読むファイル

- [sample/http_server_simple/main.c](../../sample/http_server_simple/main.c)

### 確認ポイント

1. サーバー生成時に `QS_SERVER_TYPE_HTTP` を指定しているか  
2. 必要なら `api_qs_server_create_router` を呼んでいるか  
3. メインループが以下になっているか  

```c
for(;;){
    api_qs_update(context);
    api_qs_sleep(context);
}
```

4. `on_http_event` / `on_ws_event` の役割分離ができているか

---

## 4. 作業開始判定チェックリスト

以下にすべて `YES` で着手する。

- [ ] 変更対象が `header` / `src` / `sample` のどこか判断できた  
- [ ] 一時メモリと長期メモリの使い分け方針を言語化できた  
- [ ] HTTP処理で「静的配信 → API処理 → 404時コールバック」の流れを説明できる  
- [ ] 実装後の確認対象サンプル（最低1つ）を決めた  

---

## 5. 着手前メモ（テンプレート）

着手前に以下だけ埋める。

- 作業名（ディレクトリ名）:
- 作業計画ファイル: `work/plan/[作業名]/design.md`
- 目的:
- 変更対象ファイル:
- 影響範囲（HTTP / WebSocket / Memory / JSON / CSV / KVS）:
- 使用するメモリ領域（temporary / persistent）:
- 参照サンプル:
- 確認手順:

このメモを作ってから実装に入ると、`core` の設計意図を崩しにくい。
