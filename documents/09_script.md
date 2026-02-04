# スクリプト実行

ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)

## 基本手順
1. `api_qs_memory_alloc`
2. `api_qs_script_read_file`
3. `api_qs_script_run`
4. `api_qs_script_get_parameter`

参考: [sample/script/main.c](sample/script/main.c)

## 引数の受け渡し
- `api_qs_script_set_argv_object`
- `api_qs_script_set_argv_string`
- `api_qs_script_set_argv_integer`

## 典型用途
- 設定ファイルの読み込み（HTTP サーバ設定など）
- バッチ処理の実行
