/**
 * PlayerController
 * プレイヤー1体の向きと歩行アニメの状態を持つ。
 *
 * 移動量の計算は PlayTestScene が行い、ここには表示用の状態だけを置く。
 * 状態は実行用コピーに書かないため、アセットには残らない。
 */
class PlayerController {
  constructor() {
    /** @type {string} 表示中の向き（CharacterChipset.DIRECTIONS の値） */
    this.direction = 'down';
    /** @type {number} CharacterChipset.WALK_CYCLE 内の位置 */
    this.cycleIndex = 0;
    /** @type {number} 次の歩行フレームまでの経過時間 (ms) */
    this.stepTimer = 0;
    /** @type {boolean} 直前のフレームで移動していたか */
    this.moving = false;
  }

  /**
   * 1フレーム分の状態を更新する。
   * @param {number} dtMs      経過時間 (ms)
   * @param {boolean} moving   このフレームで移動したか
   * @param {string|null} direction  移動中の向き（停止中は null）
   */
  update(dtMs, moving, direction) {
    if (!moving) {
      // 停止中は立ち（サイクル位置 0）に戻し、向きは保持する
      this.moving = false;
      this.cycleIndex = 0;
      this.stepTimer = 0;
      return;
    }

    if (direction) this.direction = direction;

    if (!this.moving) {
      // 停止から移動に入ったら歩きA から始める
      this.moving = true;
      this.cycleIndex = 1;
      this.stepTimer = 0;
    }

    this.stepTimer += Math.max(0, Number(dtMs) || 0);
    while (this.stepTimer >= CharacterChipset.WALK_STEP_MS) {
      this.stepTimer -= CharacterChipset.WALK_STEP_MS;
      this.cycleIndex = (this.cycleIndex + 1) % CharacterChipset.WALK_CYCLE.length;
    }
  }

  /**
   * 現在のフレーム列（0 = 立ち, 1 = 歩きA, 2 = 歩きB）を返す。
   * @returns {number}
   */
  getFrameIndex() {
    return CharacterChipset.WALK_CYCLE[this.cycleIndex];
  }
}
