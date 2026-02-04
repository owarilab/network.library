# CSV API

ヘッダ: [core/header/qs_csv.h](core/header/qs_csv.h)

## 主要関数
- 読み込み: `qs_csv_file_load` / `qs_csv_parse`
- 行数/列数: `qs_csv_get_line_length`, `qs_csv_get_row_length`
- 取得: `qs_csv_get_row`
- 書き出し: `qs_csv_build_csv`
- 追加: `qs_csv_add_row`, `qs_csv_add_line`

## 使い方の流れ
1. メモリプールを初期化（`qs_initialize_memory_f64`）
2. CSV を読み込み or 文字列からパース
3. 必要に応じて行/列を追加
4. `qs_csv_build_csv` で CSV 文字列を生成

参考: [sample/csv/main.c](sample/csv/main.c)
