# Text Component JSON 例集

最終更新: 2026-04-27

## 目的

`PlayUnitEditorScene` では現在 `Text` component の parameter を JSON で直接編集するため、
そのまま貼り付けて試せる例をまとめる。

この文書の例は、基本的に `Text` component の `data` にそのまま入れる想定で記述する。

---

## 前提

- 文字を表示するには、同じ `PlayObject` に `Transform` と `Text` の両方が必要
- 位置は `Transform.data.x`, `Transform.data.y`, `Transform.data.z` で決まる
- ここで示す JSON は `Text.data` のみ
- `wrap: true` を使う場合は `maxWidth > 0` を指定する
- `alpha` は `0.0` から `1.0` の範囲で扱う

---

## 基本テンプレート

```json
{
  "text": "Hello World",
  "font": "24px sans-serif",
  "color": "#ffffff",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 28,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例1: 最小の単発テキスト

用途: まず表示されるか確認したいとき

```json
{
  "text": "Hello World",
  "font": "24px sans-serif",
  "color": "#ffffff",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 28,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例2: 中央タイトル

用途: 画面中央にタイトルや見出しを置きたいとき

`Transform` 側の `x`, `y` を中央付近に置き、`align: "center"`, `baseline: "middle"` を使う。

```json
{
  "text": "Stage Clear",
  "font": "48px serif",
  "color": "#ffe082",
  "alpha": 1,
  "align": "center",
  "baseline": "middle",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 56,
  "strokeColor": "#3e2723",
  "strokeWidth": 4,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例3: 複数行メッセージ

用途: 改行を含む簡単なメッセージ表示

```json
{
  "text": "Welcome to the town.\nTalk to the guard.",
  "font": "24px sans-serif",
  "color": "#ffffff",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 32,
  "strokeColor": "#000000",
  "strokeWidth": 2,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例4: 自動折り返し説明文

用途: 長文を指定幅で折り返したいとき

`wrap: true` と `maxWidth` を組み合わせる。

```json
{
  "text": "This is a long text block used to confirm automatic wrapping inside the play test preview.",
  "font": "22px sans-serif",
  "color": "#e2e8f0",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": true,
  "maxWidth": 280,
  "lineHeight": 30,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例5: 縁取り付きラベル

用途: 背景に埋もれやすい文字を強調したいとき

```json
{
  "text": "Interact",
  "font": "28px sans-serif",
  "color": "#ffffff",
  "alpha": 1,
  "align": "center",
  "baseline": "middle",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 32,
  "strokeColor": "#111827",
  "strokeWidth": 5,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例6: 背景付きバッジ

用途: ボタン風、タグ風、HUD ラベル風の見せ方

```json
{
  "text": "Quest Updated",
  "font": "20px sans-serif",
  "color": "#f8fafc",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 24,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "#1d4ed8cc",
  "padding": 8
}
```

---

## 例7: 半透明オーバーレイ文言

用途: 補助情報や演出用の薄い文字

```json
{
  "text": "Paused",
  "font": "64px serif",
  "color": "#ffffff",
  "alpha": 0.45,
  "align": "center",
  "baseline": "middle",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 72,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "",
  "padding": 0
}
```

---

## 例8: 背景付き複数行メッセージ枠

用途: ダイアログ風の簡易表示

```json
{
  "text": "You received a potion.\nCheck your inventory.",
  "font": "22px sans-serif",
  "color": "#f8fafc",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 30,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "#0f172acc",
  "padding": 12
}
```

---

## 例9: 説明ボックス風の長文

用途: チュートリアル文やヘルプ文の確認

```json
{
  "text": "Use arrow keys to move, press Z to interact, and open the menu from the top scene when you want to save progress.",
  "font": "20px sans-serif",
  "color": "#f1f5f9",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": true,
  "maxWidth": 360,
  "lineHeight": 28,
  "strokeColor": "",
  "strokeWidth": 0,
  "backgroundColor": "#111827dd",
  "padding": 14
}
```

---

## 例10: 右寄せ HUD テキスト

用途: 右上のスコアや所持金表示

`Transform` 側の `x` を右端寄りに置き、`align: "right"` を使う。

```json
{
  "text": "Gold: 1250",
  "font": "24px monospace",
  "color": "#fde68a",
  "alpha": 1,
  "align": "right",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 28,
  "strokeColor": "#451a03",
  "strokeWidth": 3,
  "backgroundColor": "#00000066",
  "padding": 6
}
```

---

## よく使う組み合わせ

### とにかく表示確認したい

- `align: "left"`
- `baseline: "top"`
- `wrap: false`
- `maxWidth: 0`

### タイトル風にしたい

- `align: "center"`
- `baseline: "middle"`
- `strokeColor` と `strokeWidth` を付ける

### ダイアログ風にしたい

- `wrap: true`
- `maxWidth` を指定
- `backgroundColor` と `padding` を付ける

### HUD 風にしたい

- `align: "right"` か `"left"`
- `backgroundColor` を半透明にする
- `font` を `monospace` にする

---

## 最小確認用の組み合わせ例

`Transform.data`

```json
{
  "x": 120,
  "y": 80,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

`Text.data`

```json
{
  "text": "Hello World",
  "font": "24px sans-serif",
  "color": "#ffffff",
  "alpha": 1,
  "align": "left",
  "baseline": "top",
  "wrap": false,
  "maxWidth": 0,
  "lineHeight": 28,
  "strokeColor": "#000000",
  "strokeWidth": 2,
  "backgroundColor": "#0f172acc",
  "padding": 8
}
```

---

## 補足

- `wrap` は現在 `PlayTestScene` の preview 用の最小実装である
- `backgroundColor` は矩形背景であり、角丸や枠線はまだ未対応
- `alpha` は文字と背景の両方に同時に掛かる
- 専用 UI が未実装のため、この文書をテンプレート集として使う想定