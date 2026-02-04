# KVS（Key-Value Store）

ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)

## メモリ型
- `QS_KVS_MEMORY_TYPE_B1MB` など（サーバ用）
- 直接作成: `api_qs_kvs_create_b1mb`, `api_qs_kvs_create_b8mb` など

## 基本操作
- 追加/更新: `api_qs_kvs_set`
- 取得: `api_qs_kvs_get`
- 削除: `api_qs_kvs_delete`
- サイズ: `api_qs_kvs_get_buffer_size`
- キー列挙: `api_qs_kvs_keys`, `api_qs_kvs_sorted_keys`

## 永続化
- `api_qs_kvs_create_b1mb_persistence`, `api_qs_kvs_create_b8mb_persistence`
- 終了時: `api_qs_persistence_kvs_memory_free`

参考:
- メモリ KVS: [sample/cache/main.c](sample/cache/main.c)
- 永続化 KVS: [sample/persistence_cache/main.c](sample/persistence_cache/main.c)

## 典型パターン
- 大量登録時は一時メモリを別に確保し、`api_qs_memory_clean` を繰り返す
- JSON でキー一覧を返すなどの用途に向く
