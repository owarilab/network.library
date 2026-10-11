/**
 * NewCharacterChipsetDialog
 * キャラクターチップセット新規作成ダイアログ。
 * 列数・行数は固定（3 × 4）のため、チップサイズだけを選ばせる。
 *
 * 機能:
 *   - チップサイズプリセット (8 / 16 / 32 / 48 / 64)、既定 16
 *   - 固定レイアウトの表示（フレーム 3 列 × 向き 4 行）
 *   - キャンセル / 作成 ボタン
 */
class NewCharacterChipsetDialog extends DialogBase {
  static DIALOG_W = 400;
  static DIALOG_H = 280;

  static CHIP_PRESETS = [8, 16, 32, 48, 64];

  /**
   * @param {(chipW: number, chipH: number) => void} onConfirm
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('新規キャラクターチップ作成', NewCharacterChipsetDialog.DIALOG_W, NewCharacterChipsetDialog.DIALOG_H);

    this.onConfirm = onConfirm;
    this.onCancel  = onCancel;

    /** 選択中のチップサイズ */
    this._chipSize = 16;

    /** ホバー中の要素キー @type {string|null} */
    this._hover = null;

    /** 各 UI 要素の描画座標 @type {Object.<string, {x:number,y:number,w:number,h:number}>} */
    this._rects = {};
  }

  // ----------------------------------------------------------------
  // DialogBase オーバーライド
  // ----------------------------------------------------------------

  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const PAD = 20;
    const label = (text, x, y) => {
      ctx.fillStyle    = '#222222';
      ctx.font         = '13px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    let y = by + 16;

    // チップサイズ プリセット
    label('チップサイズ:', bx + PAD, y + 7);
    y += 24;

    const presetW   = 58;
    const presetH   = 26;
    const presetGap = 6;
    let px = bx + PAD;
    this._rects.presets = [];
    for (const size of NewCharacterChipsetDialog.CHIP_PRESETS) {
      const key = `chip_${size}`;
      const r = { x: px, y, w: presetW, h: presetH };
      this._rects.presets.push({ key, size, ...r });

      const selected = this._chipSize === size;
      this._drawPresetButton(ctx, r.x, r.y, r.w, r.h, `${size}×${size}`, selected, this._hover === key);
      px += presetW + presetGap;
    }
    y += presetH + 20;

    // 固定レイアウトの説明
    label('レイアウト:', bx + PAD, y + 7);
    ctx.fillStyle    = '#555555';
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText('フレーム 3 列（立ち / 歩きA / 歩きB）× 向き 4 行（下 / 左 / 右 / 上）', bx + PAD + 90, y + 7);
    ctx.textBaseline = 'alphabetic';
    y += 24;

    // 画像サイズ プレビュー
    const imgW = this._chipSize * 3;
    const imgH = this._chipSize * 4;
    ctx.fillStyle    = '#555555';
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(`画像サイズ: ${imgW} × ${imgH} px  (3×4 チップ)`, bx + PAD, y + 7);
    ctx.textBaseline = 'alphabetic';
    y += 24;

    // 区切り線
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD,      y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 14;

    // ボタン行: キャンセル / 作成
    const btnW = 90;
    const btnH = 28;
    const cancelR = { x: bx + bw - PAD - btnW * 2 - 10, y, w: btnW, h: btnH };
    const okR     = { x: bx + bw - PAD - btnW,           y, w: btnW, h: btnH };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
                     'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x, okR.y, okR.w, okR.h,
                     '作成', this._hover === 'btnOK', true);

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ
  // ----------------------------------------------------------------

  onMouseMoveBody(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  onMouseDownBody(e) {
    if (e.button !== 0) return;
    const key = this._hitTest(e.x, e.y);
    if (!key) return;

    const preset = this._rects.presets?.find(p => p.key === key);
    if (preset) {
      this._chipSize = preset.size;
      return;
    }

    if (key === 'btnCancel') { this.hide(); this.onCancel(); return; }
    if (key === 'btnOK')     { this.hide(); this.onConfirm(this._chipSize, this._chipSize); return; }
  }

  onKeyDownBody(e) {
    if (e.key === 'Escape') {
      this.hide();
      this.onCancel();
    }
  }

  // ----------------------------------------------------------------
  // 内部ロジック
  // ----------------------------------------------------------------

  /**
   * 座標からヒットした UI キーを返す
   * @param {number} x
   * @param {number} y
   * @returns {string|null}
   */
  _hitTest(x, y) {
    for (const p of (this._rects.presets ?? [])) {
      if (this._inRect(x, y, p)) return p.key;
    }
    for (const [key, r] of Object.entries(this._rects)) {
      if (key === 'presets') continue;
      if (this._inRect(x, y, r)) return key;
    }
    return null;
  }

  _inRect(x, y, r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  }

  // ----------------------------------------------------------------
  // 描画ユーティリティ
  // ----------------------------------------------------------------

  _drawPresetButton(ctx, x, y, w, h, label, selected, hovered) {
    const bg     = selected ? '#0078d7' : (hovered ? '#dcdad5' : '#e8e6e1');
    const fg     = selected ? '#ffffff' : '#000000';
    const border = selected ? '#005a9e' : '#a0a0a0';

    ctx.fillStyle   = bg;
    ctx.strokeStyle = border;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, x, y, w, h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = fg;
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'center';
    ctx.fillText(label, x + w / 2, y + h / 2);
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}
