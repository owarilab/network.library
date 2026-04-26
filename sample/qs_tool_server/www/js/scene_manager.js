/**
 * SceneManager
 * 現在アクティブなシーンを保持し、CanvasManager のループから
 * update / render を委譲されるクラス。
 */
class SceneManager {
  /**
   * @param {AppData} appData - シーン間で共有するアプリケーションデータ
   * @param {Input}   input   - 入力管理オブジェクト
   * @param {() => void} [onResize] - シーン切り替え直後に呼ぶリサイズ処理
   */
  constructor(appData, input, onResize = null) {
    /** @type {Scene|null} */
    this.current = null;

    /** @type {AppData} */
    this.appData = appData;

    /** @type {Input} */
    this.input = input;

    /** @type {(() => void)|null} */
    this.onResize = typeof onResize === 'function' ? onResize : null;
  }

  /**
   * シーンを切り替える。
   * 旧シーンの onLeave → input.clearAll() → 新シーンの onEnter を呼ぶ。
   * @param {Scene} scene - 次のシーン
   */
  change(scene) {
    if (this.current) {
      this.current.onLeave(this.input, this.appData);
    }
    this.input.clearAll();
    this.current = scene;
    this.appData.setCurrentSceneId(this._resolveSceneId(scene));
    this.current.onEnter(this.input, this.appData);
    this.onResize?.();
  }

  /**
   * シーンインスタンスからセッション保存用の識別子を解決する。
   * @param {Scene} scene
   * @returns {string}
   */
  _resolveSceneId(scene) {
    const ctorName = scene?.constructor?.name;
    return typeof ctorName === 'string' && ctorName ? ctorName : 'unknown_scene';
  }

  /**
   * @param {number} dt - 経過時間 (ms)
   */
  update(dt) {
    this.current?.update(dt, this.appData);
  }

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement} canvas
   */
  render(ctx, canvas) {
    this.current?.render(ctx, canvas, this.appData);
  }
}
