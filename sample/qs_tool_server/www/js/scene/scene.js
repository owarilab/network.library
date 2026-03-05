/**
 * Scene
 * 全シーンの基底クラス。
 * 各シーンは update() / render() をオーバーライドして描画・ロジックを実装する。
 * 入力ハンドラは onEnter() で登録し、onLeave() で解除する。
 */
class Scene {
  /**
   * シーンがアクティブになったときに呼ばれる。
   * ここで input.on() を使って入力ハンドラを登録する。
   * @param {Input}   input
   * @param {AppData} appData
   */
  onEnter(input, appData) {}

  /**
   * シーンが非アクティブになる直前に呼ばれる。
   * SceneManager が input.clearAll() を呼ぶため、
   * 通常はオーバーライド不要（追加処理が必要な場合のみ実装）。
   * @param {Input}   input
   * @param {AppData} appData
   */
  onLeave(input, appData) {}

  /**
   * フレームごとのロジック更新
   * @param {number}  dt      - 前フレームからの経過時間 (ms)
   * @param {AppData} appData - 共有データ
   */
  update(dt, appData) {}

  /**
   * フレームごとの描画
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   * @param {AppData}                 appData - 共有データ
   */
  render(ctx, canvas, appData) {}
}
