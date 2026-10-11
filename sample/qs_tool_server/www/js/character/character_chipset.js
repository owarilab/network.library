/**
 * CharacterChipset
 * キャラクターの歩行アニメ用チップセットのヘルパー。
 * 格納は TilesetData を使い、列がフレーム、行が向きを表す。
 *
 * 列（フレーム）: 0 = 立ち, 1 = 歩きA, 2 = 歩きB
 * 行（向き）:     0 = 下, 1 = 左, 2 = 右, 3 = 上
 */
class CharacterChipset {
  static DIRECTIONS = ['down', 'left', 'right', 'up'];
  static DIRECTION_LABELS = ['下', '左', '右', '上'];
  static FRAME_COLUMNS = 3;
  static FRAME_LABELS = ['立ち', '歩きA', '歩きB'];
  static WALK_CYCLE = [0, 1, 0, 2];
  static WALK_STEP_MS = 160;

  /**
   * 編集中のタイルセットがキャラクターチップセットか。
   * @param {AppData} appData
   * @returns {boolean}
   */
  static isEditing(appData) {
    return !!appData
      && appData.editMode === 'tileset'
      && !!appData.tilesetData
      && appData.tilesetKind === 'characterChipset';
  }

  /**
   * 固定レイアウトの TilesetData を作る。
   * @param {number} chipW
   * @param {number} chipH
   * @returns {TilesetData}
   */
  static createTilesetData(chipW, chipH) {
    return new TilesetData(chipW, chipH, CharacterChipset.FRAME_COLUMNS, CharacterChipset.DIRECTIONS.length);
  }

  /**
   * .qsproj に保存するレイアウト情報。常に同じ値になる。
   * @returns {{ directions: string[], framesPerDirection: number }}
   */
  static createLayout() {
    return {
      directions: [...CharacterChipset.DIRECTIONS],
      framesPerDirection: CharacterChipset.FRAME_COLUMNS,
    };
  }

  /**
   * 向き名から行番号を返す。不明な向きは下（0）とする。
   * @param {string} direction
   * @returns {number}
   */
  static directionToRow(direction) {
    const row = CharacterChipset.DIRECTIONS.indexOf(direction);
    return row < 0 ? 0 : row;
  }

  /**
   * 向きとフレームに対応するチップの LayerData を返す。
   * @param {TilesetData} tilesetData
   * @param {string} direction
   * @param {number} frameIndex
   * @returns {LayerData|null}
   */
  static getFrameLayerData(tilesetData, direction, frameIndex) {
    if (!tilesetData) return null;
    return tilesetData.getChipLayerData(frameIndex, CharacterChipset.directionToRow(direction));
  }

  /**
   * fromDirection の3フレームを toDirection にコピーし、左右反転する。
   * 例: right を left に反転コピーする。履歴には載せない。
   * @param {TilesetData} tilesetData
   * @param {string} fromDirection
   * @param {string} toDirection
   */
  static mirrorDirection(tilesetData, fromDirection, toDirection) {
    if (!tilesetData) return;
    const srcRow = CharacterChipset.directionToRow(fromDirection);
    const dstRow = CharacterChipset.directionToRow(toDirection);
    for (let col = 0; col < CharacterChipset.FRAME_COLUMNS; col++) {
      tilesetData.copyChip(col, srcRow, col, dstRow);
      const layerData = tilesetData.getChipLayerData(col, dstRow);
      for (const layer of layerData.layers) {
        layer.pixelData.flipH();
      }
      layerData.markCompositeDirty();
    }
  }
}
