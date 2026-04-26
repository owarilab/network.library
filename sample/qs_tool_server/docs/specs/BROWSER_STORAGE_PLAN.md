# ブラウザ保存（IndexedDB）最小設計書

最終更新: 2026-04-26

## 目的

`.qsproj` の import / export に加えて、ブラウザ内に project を永続保存できるようにする。

このフェーズでは次を優先する。

1. ユーザーが明示的に `Save Project` した内容をブラウザに保存できる
2. ブラウザ再読み込み後に保存済み project を再度開ける
3. 現在の `.qsproj` 形式を壊さず、外部ファイル保存と共存できる
4. 複数の project を browser 内へ並行保存できる

今回は「クラウド同期」「複数端末共有」「高機能な自動保存管理」は対象外とする。

---

## 結論

保存先は `IndexedDB` を採用する。

理由:

- `localStorage` は容量が小さく、文字列しか扱えない
- 現在の project は `qts-base64` を含みサイズが膨らみやすい
- `IndexedDB` は `ArrayBuffer` / `Blob` / object を扱え、project 単位保存に向く

補助用途として `localStorage` は使ってよいが、保存本体は持たせない。

`localStorage` に置くのは次に限定する。

- lastOpenedProjectId
- recentProjectIds
- 軽量な UI 設定

---

## 保存方針

## 1. 外部形式と内部形式を分ける

### 外部形式

- `.qsproj`
- ユーザーがダウンロード / アップロードするための交換形式
- 既存の `ProjectSerializer` を継続利用する

### 内部形式

- `IndexedDB` 上に保持するブラウザ内保存形式
- `.qsproj` と同じ責務を持つが、保存単位を分割する
- 画像アセット本体は可能な限り `ArrayBuffer` で保持する

つまり、ブラウザ保存時に `.qsproj` 文字列を丸ごと保存するのではなく、
project メタ情報と asset バイナリを分離して保存する。

---

## 最小データモデル

この設計は最初から「複数 project 保存」を前提にする。

`IndexedDB` 内には project を 1 件だけ持つのではなく、`project.id` を主キーとして複数件保持する。

同じ browser origin 上で、複数の project を保存・一覧表示・再オープンできるようにする。

データベース名:

- `qs_tool_server_db`

バージョン:

- `1`

object store は最小で 3 つに分ける。

## 1. `projects`

key:

- `project.id`

保存内容:

```js
{
  id,
  name,
  version,
  createdAt,
  updatedAt,
  settings,
  assetRefs: [
    { id, type, name, updatedAt }
  ]
}
```

役割:

- プロジェクト一覧表示
- 最近更新順ソート
- project 本体のメタ情報保持
- 複数 project の共存管理

## 2. `project_sessions`

key:

- `projectId`

保存内容:

```js
{
  projectId,
  dirty,
  currentScene,
  activeDocumentRef,
  editorState,
  updatedAt
}
```

役割:

- 最後にどの asset を開いていたか復元する
- ツール状態、色、selectedChip を復元する

## 3. `assets`

key:

- `asset.id`

保存内容:

```js
{
  id,
  projectId,
  type,            // 'pixelDocument' | 'tileset' | 'map'
  name,
  updatedAt,
  payloadVersion: 1,
  storage: {
    codec,         // 'qts-arraybuffer'
    data           // ArrayBuffer
  },
  meta: {
    width,
    height,
    chipWidth,
    chipHeight,
    columns,
    rows
  }
}
```

役割:

- 実データ本体の保存
- `pixelDocument` は 1x1 tileset として QTS 化した `ArrayBuffer`
- `tileset` はそのまま QTS 化した `ArrayBuffer`
- `map` は現時点では最小の placeholder でよい

---

## シリアライズ方針

既存コードとの整合を優先して、内部保存でも QTS ベースを使う。

## pixelDocument

- `LayerData` を `wrapLayerDataAsSingleChipTileset()` で 1x1 tileset 化
- `createQtsArrayBuffer()` で `ArrayBuffer` 化
- `assets.storage.codec = 'qts-arraybuffer'`

## tileset

- `TilesetData` を `createQtsArrayBuffer()` で `ArrayBuffer` 化
- `assets.storage.codec = 'qts-arraybuffer'`

## session

- 既存 `ProjectSession` と同じ shape を plain object として保存

この方針にすると、現在の `.qsproj` で使っている変換ロジックを流用しやすい。

---

## 複数 project 保存時の振る舞い

## 保存単位

- `Save Project` は「現在開いている project.id に対する上書き保存」とする
- `Save As` は新しい `project.id` と asset id 群を再発行して別 project として保存する
- `Save As` 実行後は新しく保存した project を current project として扱う

## project の識別子

- browser storage 上の一意キーは `project.id`
- project 名は重複してよい
- 一覧 UI では `name` に加えて `updatedAt` を表示して区別しやすくする

## 一覧表示

- `listProjects()` は複数 project のメタ情報を返す
- 表示順は `updatedAt` の降順を既定とする
- 一覧では少なくとも次を表示する
  - `name`
  - `updatedAt`
  - `pixelDocuments / tilesets` の件数

## 削除

- `deleteProject(projectId)` は対象 project の `projects / project_sessions / assets` をまとめて削除する
- 他 project には影響しない

## recent projects

- `localStorage.recentProjectIds` は複数 id を配列順で保持してよい
- `lastOpenedProjectId` はその先頭または最後に開いた 1 件を指す補助情報とする

---

## 復元方針

project を開く時は次の順で読む。

1. `projects` から project メタを取得
2. `project_sessions` から session を取得
3. `assets` からその project に属する asset を列挙
4. 各 asset の `storage.data` を QTS として復元
5. `ProjectData` と `ProjectSession` を組み立てて `AppData.setCurrentProject()` へ渡す

最小実装では、project を開く時点で対象 project の asset を全読み込みしてよい。

理由:

- 現状の asset 数はまだ少ない
- 実装が単純
- 既存の `ProjectTopScene` / `EditorScene` と接続しやすい

将来的には lazy load に拡張できる。

---

## API 最小案

新規ファイル案:

- `www/js/project/project_browser_storage.js`

責務:

- IndexedDB open / migration
- project 保存
- project 読込
- project 一覧取得
- project 削除

最小 API:

```js
class ProjectBrowserStorage {
  static isAvailable()
  static openDatabase()
  static saveProject(project, session, fallbackPalette = null)
  static loadProject(projectId)
  static listProjects()
  static deleteProject(projectId)
}
```

補助 API:

```js
class ProjectBrowserStorageSerializer {
  static serializeAsset(asset, fallbackPalette = null)
  static deserializeAsset(record)
  static serializeSession(session, project)
  static deserializeSession(record, project)
}
```

ここで `ProjectSerializer` を直接 IndexedDB 層に持ち込まず、
`browser storage` 用の薄い adapter として分離する。

`listProjects()` の返却値は最小で次の shape を想定する。

```js
[
  {
    id,
    name,
    updatedAt,
    createdAt,
    assetCounts: {
      pixelDocuments,
      tilesets,
      maps,
    },
  }
]
```

---

## UI 接続の最小案

この段階では既存導線を壊さず、次の 3 操作だけ追加すればよい。

1. `ProjectTopScene` の `Save Project`
   保存先選択を増やさず、まずは「ブラウザ保存」を既定動作にしてよい
   外部 `.qsproj` 書き出しは別ボタンまたは後続メニューへ分離

2. `TitleScene` の `Load Project`
   ファイル読込とは別に「Browser Projects」を表示

3. `TitleScene` または `ProjectTopScene`
  保存済み project 一覧表示

複数保存前提の最小 UI は次でよい。

- `TitleScene` に `Open Browser Project` ボタンを追加
- 押下で browser 保存済み project 一覧を表示
- 各行クリックで対象 project を開く
- 一覧上で `Delete` は後回しでもよいが、設計上は可能にしておく

最小運用ではこうする。

- `Save Project`: IndexedDB 保存
- `Export Project`: `.qsproj` ダウンロード
- `Load Project`: file import
- `Open Browser Project`: IndexedDB から開く

この時点で、browser 内に複数の project が並んでいてもよい。
現在 project は `projectSession.projectId` と `AppData.currentProject.id` で決まる。

この分離が最も分かりやすい。

---

## 容量見積もり

固定上限はブラウザ依存で、実効容量は環境に左右される。
そのため仕様では厳密な MB 値を保証しない。

ただし運用上の前提は次の通り。

- `localStorage` 前提では不足する
- `IndexedDB` なら少なくとも数十 MB 級を扱える想定で進める
- 大きめ project ではブラウザの quota 制御に失敗する可能性がある

実装側では保存前に概算サイズを出して警告できるようにする。

概算式:

```text
RGBA raw size = width * height * 4 * layerCount bytes
```

QTS 化で量子化されるとはいえ、レイヤー数や tileset 数により project 全体サイズは大きくなる。

---

## エラー方針

保存失敗時は次を区別する。

1. IndexedDB 未対応
2. open 失敗
3. quota 超過
4. transaction 中断
5. deserialize 失敗

UI メッセージは最小でよい。

- `ブラウザ保存に失敗しました`
- `保存領域が不足している可能性があります`
- `保存データの復元に失敗しました`

---

## 今回の非対象

- 自動保存の世代管理
- 差分保存
- asset 単位の lazy load
- 複数タブ競合検出
- ブラウザストレージと `.qsproj` の双方向同期
- map 本体の詳細保存

---

## 実装順序

1. `ProjectBrowserStorage` を追加
2. `saveProject()` / `loadProject()` / `listProjects()` の最小実装
3. `ProjectTopScene` に browser save を接続
4. `TitleScene` に browser project 一覧または open 導線を追加
5. `Export Project (.qsproj)` を別導線として残す

---

## 採用判断

現時点では「`.qsproj` をそのまま IndexedDB に文字列保存する」案より、
「project/session/asset を分割し、asset 本体は `ArrayBuffer` で持つ」案を採用する。

理由:

- base64 膨張を避けられる
- project 一覧取得が軽い
- 将来の autosave / asset 単位更新へ繋げやすい
- 既存 QTS 変換ロジックをそのまま活用できる