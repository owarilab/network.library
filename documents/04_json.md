# JSON API

## 高レベル API
ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)

代表的な関数:
- 作成: `api_qs_object_create`, `api_qs_array_create`
- 追加: `api_qs_object_push_*`, `api_qs_array_push_*`
- エンコード: `api_qs_json_encode_object`, `api_qs_json_encode_array`
- デコード: `api_qs_json_decode_object`
- 参照: `api_qs_object_get_*`, `api_qs_array_get_*`, `api_qs_object_get_keys`

参考: [sample/json/main.c](sample/json/main.c)

## 低レベル API
ヘッダ: [core/header/qs_json.h](core/header/qs_json.h)

- メモリプール (`QS_MEMORY_POOL`) を使って JSON をパース/エンコード
- 高速・軽量だが扱いはやや低レベル

## よくある使い方
- レスポンス用 JSON 生成
- KVS のキー列挙を JSON 配列化
- HTTP POST の JSON パース

HTTP 連携例: [sample/http_server_simple/main.c](sample/http_server_simple/main.c)
