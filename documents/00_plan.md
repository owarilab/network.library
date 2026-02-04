# ドキュメント作成計画

## 目的
- core ライブラリを中心に、ビルド・利用・拡張の流れが分かる実用ドキュメントを整備する。
- sample の具体例と API ヘッダを対応させ、最短で動かせる導線を作る。

## スコープ
1. コア構成とモジュールの役割把握
2. ビルドとリンク方法
3. メモリ管理とライフサイクル
4. JSON / CSV / Script / KVS / Socket / HTTP-WS の使い方
5. 付随ユーティリティ (Base64 / SHA1 / Random / String)
6. サンプルの読み方と応用

## 進め方
1. core/header の公開 API を確認
2. sample の代表例を読み、利用シナリオを抽出
3. モジュール別に最小構成の手順と注意点を記述
4. 相互参照しやすい目次とリンクを整理

## 成果物
- documents/README.md: 全体目次
- documents/01_core_overview.md: モジュール概要
- documents/02_build_link.md: ビルド/リンク
- documents/03_memory.md: メモリ管理
- documents/04_json.md: JSON API
- documents/05_csv.md: CSV API
- documents/06_socket_lowlevel.md: ソケット低レベル API
- documents/07_api_server_client.md: 高レベル API (HTTP/WS/Router/Logger含む)
- documents/08_kvs.md: KVS と永続化
- documents/09_script.md: スクリプト
- documents/10_utils.md: 付随ユーティリティ
- documents/11_samples.md: サンプル対応表
