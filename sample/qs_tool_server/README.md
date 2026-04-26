# qs_tool_server

`qs_tool_server` は、HTML5 Canvas ベースのドット絵 / タイルセット編集ツールです。
`www/` 以下にフロントエンド実装があり、ゲーム向け素材制作ツールとして拡張を進めています。

## 作業の起点

作業を始めるときは、まず次を確認します。

- [docs/WORK_STATUS.md](docs/WORK_STATUS.md)

ここに現在の実装状況、完了済み機能、未着手項目、関連ファイル構成をまとめています。

## 基本の進め方

1. `docs/WORK_STATUS.md` を確認する
2. 対象機能の現状実装を `www/` 以下で確認する
3. 必要なら `docs/specs/` の仕様書や計画書を見る
4. 実装後に `docs/WORK_STATUS.md` を更新する

## ドキュメント

- [docs/WORK_STATUS.md](docs/WORK_STATUS.md): 現在の進捗と作業状況
- [docs/specs/](docs/specs/): 仕様書、計画書、ロードマップ
- [docs/specs/BROWSER_STORAGE_PLAN.md](docs/specs/BROWSER_STORAGE_PLAN.md): IndexedDB を使ったブラウザ保存の最小設計

## ビルド方法

`qs_tool_server` ディレクトリで次を実行します。

```bash
make
```

これにより `../../core/src` 側のビルドも呼ばれ、`qs_tool_server` バイナリが生成されます。

掃除するときは次です。

```bash
make clean
```

## 動作場所

- document root: `sample/qs_tool_server/www`
- 想定URL: `http://localhost:4444/`