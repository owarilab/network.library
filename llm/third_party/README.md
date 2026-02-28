# third_party

このディレクトリは、オプションモジュールが依存する外部ソースを配置します。

## llama.cpp

配置場所（推奨）:
- `llm/third_party/llama.cpp/`

例（submodule の場合）:

```bash
git submodule add https://github.com/ggerganov/llama.cpp llm/third_party/llama.cpp
```

備考:
- ネットワークが使えない環境では、手元の llama.cpp を上記パスへコピーするだけでも構いません。
