# qsproj プロジェクトテンプレート作成ガイド

.qsproj ファイルの正しい構造を説明します。

## 最小構造

```json
{
  "format": "qsproj",
  "version": 1,
  "project": {
    "id": "proj_xxxxx",
    "version": 1,
    "name": "プロジェクト名",
    "createdAt": 1777436608074,
    "updatedAt": 1777436704139,
    "settings": {
      "defaultChipWidth": 16,
      "defaultChipHeight": 16
    },
    "assets": {
      "pixelDocuments": [],
      "tilesets": [],
      "maps": [],
      "playUnits": [...]
    }
  }
}
```

## PlayUnit の構造

**重要**: `objects` はトップレベルに配置してください。`storage` は使用しません。

```json
{
  "id": "pu_xxxxx",
  "type": "playUnit",
  "name": "シーン名",
  "description": "説明",
  "objects": [
    {
      "id": "obj_xxxxx",
      "name": "オブジェクト名",
      "components": [
        {
          "type": "Transform",
          "enabled": true,
          "data": {
            "x": 0, "y": 0, "z": 0,
            "rotation": 0,
            "scaleX": 1, "scaleY": 1
          }
        }
      ]
    }
  ]
}
```

## 必須コンポーネント

### Transform
```json
{
  "type": "Transform",
  "enabled": true,
  "data": {
    "x": 0, "y": 0, "z": 0,
    "rotation": 0,
    "scaleX": 1, "scaleY": 1
  }
}
```

### Camera（メインカメラ用）
```json
{
  "type": "Camera",
  "enabled": true,
  "data": {
    "zoom": 1,
    "viewportX": 0, "viewportY": 0,
    "viewportWidth": 800, "viewportHeight": 600,
    "followTargetObjectId": "",
    "followLerp": 1
  }
}
```

### PlaySettings（カメラ指定）
```json
{
  "type": "PlaySettings",
  "enabled": true,
  "data": {
    "defaultCameraObjectId": "camera_object_id"
  }
}
```

## ID の命名規則

| 種類 | 形式 | 例 |
|------|------|-----|
| Project | `proj_<8文字>` | `proj_5rj006ru` |
| PixelDoc | `px_<名前>_<8文字>` | `px_title_default_06mu9rav` |
| PlayUnit | `pu_<名前>_<8文字>` | `pu_scene_fesubtaw` |
| Object | `obj_<名前>_<8文字>` | `obj_camera_main_bdktw5aw` |

## チェックリスト

- [ ] `objects` がトップレベルにある（`storage` は使わない）
- [ ] 各オブジェクトに Transform コンポーネントがある
- [ ] 1つ以上の Camera コンポーネントがある
- [ ] PlaySettings で有効なカメラIDを指定している
- [ ] すべてのID がユニークである
- [ ] JSONが有効である（`python3 -m json.tool file.qsproj` で確認）

## よくあるエラー

### エラー1: PlayObjects が空で表示される

**原因**: `objects` が `storage.data.objects` にネストされている

**修正**:
```json
// ❌ 間違い
{
  "id": "pu_...",
  "storage": {
    "data": {
      "objects": [...]
    }
  }
}

// ✅ 正しい
{
  "id": "pu_...",
  "objects": [...]
}
```

### エラー2: カメラが見つからない

**原因**: `defaultCameraObjectId` が実在しない ID を参照している

**修正**: PlaySettings の `defaultCameraObjectId` が実在する PlayObject の ID と一致するか確認

### エラー3: JSON 構文エラー

**原因**: クォートの不一致、カンマ漏れ

**検証方法**:
```bash
python3 -m json.tool template.qsproj > /dev/null
```

---

## 参考

- [最小テンプレート](../../assets/minimal_template.qsproj)
