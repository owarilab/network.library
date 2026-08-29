# スクリプト実行

ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)

`qs_script` は、C言語で記述された軽量なインタープリタ型スクリプトエンジンです。本ライブラリ内で HTTP / API 設定ファイルの読み込みやバッチ処理を実行するために使用できます。

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

---

## qs_script 構文仕様

インタープリタの文法や機能の詳細は、[core/src/qs_script.c](core/src/qs_script.c)、[core/header/qs_token_analyzer.h](core/header/qs_token_analyzer.h)、[core/header/qs_type.h](core/header/qs_type.h) および [sample/script/sample/sample.qscript](sample/script/sample/sample.qscript) などの実装に基づいています。

### 1. サポートされているデータ型
`qs_script` では、以下のデータ型が動的に管理されます。

- **数値 (Numeric)**: 内部的に `int32_t` なので実質32ビット整数。
- **浮動小数点数 (Float)**: パース上の定義はあるが基本演算時のキャスト型に制約あり。
- **文字列 (String)**: `"..."` で囲まれたテキスト。
- **配列 (Array)**: `[elem1, elem2, ...]` のように記述する0始まりのシーケンス。
- **ハッシュ (Hash)**: `[ "key1" : value1, "key2" : value2 ]` のように記述する連想配列。
- **バイナリ (Binary)**: `file_get(file_path, "b")` 等で返されるバイナリリテラル。

### 2. コメント
- 単一行コメント: `//` から行末まで。
- 複数行コメント: `/*` から `*/` まで。

### 3. 変数
- 変数は明示的な定義キーワード（例:`var`, `let`）を持たず、代入式により自動的に定義されます。
- スコープは定義位置に応じたグローバルおよびローカルスコープを形成します。
- 配列やハッシュの多次元またはネスト形式によるアクセス（例: `config["int_array"][2]`）に対応。

### 4. 演算子
`qs_script` は Cライクな豊富な演算子を処理できます。

| 分類 | 演算子 | 説明 |
| --- | --- | --- |
| 代入 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `^=`, `~=`, `\|=`, `&=` | 単純および複合代入演算 |
| 単項 | `-` | 単項マイナス（負の数定義） |
| 算術 | `+`, `-`, `*`, `/`, `%` | 数値は加減乗除・剰余。オペランドのいずれかが文字列、バイナリの場合、`+` は結合を意味します |
| 比較 | `==`, `>`, `<`, `>=`, `<=` | 数値同士の各種比較、または文字列同士の `==`/`!=`（一致・不一致）判定 |
| 論理 | `&&`, `\|\|` | 論理AND、論理OR |
| 後置 | `++`, `--` | 変数値に対する後置インクリメント・デクリメント |

*(重要：パースロジックの不具合により、不一致 `!=` 演算子はスクリプト内のコードに記述できません。実質 `==` での反転や条件整理による代替が必要です。)*

### 5. 制御構造

#### 条件分岐: `if` / `elseif` (または `else if`) / `else`
```qscript
if (x > 10) {
    echo("greater than 10\n");
} else if (x == 10) {
    echo("equals 10\n");
} else {
    echo("less than 10\n");
}
```
※ `else if` のようにスペースを空けて2単語で記述された場合、パーサーが内部で `"elseif"` として自動変換して正しく処理します。

#### ループ分岐: `while`
```qscript
idx = 0;
while (idx < 5) {
    echo("idx = " + idx + "\n");
    idx++;
}
```
※ トークン予約語として `loop` が内部で定義されていますが、構文解析器および実行器上には実装されていないため、ループ制御としては `while` を用います。

### 6. ユーザー定義関数
`def` キーワードを用いて関数を定義し、`return` により戻り値を返せます。

```qscript
def add(a, b) {
    return a + b;
}

result = add(10, 20);
echo(result + "\n");
```
※ アロー関数 `=>` も字句解析レベルで予約されているものの、現実装では実質的な無名関数コード定義として動作しないため、関数はすべて通常の `def` 定義を用います。

---

## 組み込みシステム関数 (Built-In Functions)

スクリプトエンジンにデフォルトで組み込まれており、追加手続きなしで直接呼び出せるシステム関数一覧です。[core/src/qs_api.c](core/src/qs_api.c) にてグローバルに登録されています。

| 関数名・引数 | 戻り値の型 | 説明 |
| --- | --- | --- |
| `echo(value)` | なし | コンソールへ `value`（数値、文字列、配列、ハッシュ）を出力。配列やハッシュの場合は内部データ構造をダンプします。 |
| `count(value)` | 数値 | 引数の長さを取得。文字列はバイト長、配列は要素数、ハッシュはキーエントリ数を返します。 |
| `file_exist(file_name)` | 数値 | 指定のファイルが存在すれば `1`、そうでなければ `0` を返します。 (引数は文字列) |
| `file_size(file_name)` | 数値 | 指定のファイルのバイトサイズを返します。エラー時は `0`。 (引数は文字列) |
| `file_extension(file_name)`| 文字列 | 指定のファイルの拡張子を取得します。 (引数は文字列) |
| `file_get(file_name[, mode])` | 文字列/バイナリ | 指定のファイル内容を全ロード。第二引数に `"b"` を渡した場合はバイナリデータとして強制的（バイト数ジャスト）に読み込みます。 |
| `file_put(file_name, data)`| 文字列 | `data` をファイル `file_name` に上書き保存。成功時は `"1"` 、失敗時は `"0"` を返します。 |
| `file_add(file_name, data)`| 文字列 | `data` をファイル `file_name` に追記保存。成功時は `"1"` 、失敗時は `"0"` を返します。 |
| `json_encode(value)` | 文字列 | 渡された配列やハッシュを JSON 文字列にエンコードして返します。 |
| `json_decode(json_str)` | 各種 | 指定された JSON 文字列をパースし、配列やハッシュ、プリミティブ型にデコードして返します。 |
| `gmtime()` | 文字列 | 現在の UTC 時間を示す文字列を取得します。 |
| `rand()` | 数値 | 32ビットの非負のランダム整数を生成します。 |
| `csv_read(file_path)` | 配列(2次元) | CSVファイルをロードして配列の配列（2次元配列形式）として返します。 |
| `csv_build(array[, bufsize])` | 文字列 | 2次元配列を CSV 形式のテキスト文字列にシリアライズします。任意でバッファサイズ `bufsize` を指定可能。 |
| `csv_write(file_path, array[, bufsize])` | 文字列 | 2次元配列を指定の `file_path` へ直接 CSV ファイルとして書き出します。成功時は `"1"`、失敗時は `"0"`。 |

