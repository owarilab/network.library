# メモリ管理

## 2系統
- 低レベル: [core/header/qs_memory_allocator.h](core/header/qs_memory_allocator.h)
- 高レベル: [core/header/qs_api.h](core/header/qs_api.h) の `api_qs_memory_*`

## 高レベル API（推奨）
典型フロー:
1. `api_qs_init()`
2. `api_qs_memory_alloc()`
3. 使い終わったら `api_qs_memory_clean()` or `api_qs_memory_free()`

unitバイナリ操作（高レベル）:
- `api_qs_memory_block_used()`
- `api_qs_memory_block_remaining()`
- `api_qs_memory_block_append()`
- `api_qs_memory_block_write()`
- `api_qs_memory_block_read()`
- `api_qs_memory_block_clear()`

参考: [sample/json/main.c](sample/json/main.c), [sample/script/main.c](sample/script/main.c)

## 低レベルメモリプール
- `qs_initialize_memory_f64()` で初期化
- `qs_memory_clean()` / `qs_free()` で解放
- unitバイナリ操作:
  - `qs_memory_unit_data()`
  - `qs_memory_unit_capacity()`
  - `qs_memory_unit_used()`
  - `qs_memory_unit_remaining()`
  - `qs_memory_unit_reset()`
  - `qs_memory_unit_append()`
  - `qs_memory_unit_write()`
  - `qs_memory_unit_read()`
  - `qs_memory_unit_clear()`

参考: [sample/csv/main.c](sample/csv/main.c)

## 注意点
- JSON/CSV/KVS/Script は内部でメモリプールを参照するため、利用中に `api_qs_memory_clean()` を挟む場合は寿命に注意。
- 一時領域と長期領域を分けると管理しやすい（例: KVS用メモリと一時メモリを分離）。
