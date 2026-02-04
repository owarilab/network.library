# 付随ユーティリティ

## Base64 / SHA1
ヘッダ: [core/header/qs_api.h](core/header/qs_api.h)
- `api_qs_base64_encode`
- `api_qs_base64_decode`
- `api_qs_sha1_encode`

参考: [sample/encode/main.c](sample/encode/main.c)

## ランダム
- `api_qs_rand`
- `api_qs_uniqid`

## 文字列
ヘッダ: [core/header/qs_string.h](core/header/qs_string.h)
- URL エンコード/デコード: `qs_urlencode`, `qs_urldecode`
- 文字列連結/コピー: `qs_strlcat`, `qs_strcopy`
- 時刻文字列: `qs_utc_time`

## ハッシュ
- [core/header/qs_hash.h](core/header/qs_hash.h) に低レベル API
