/**
 * CanvasManager
 * Canvas要素のライフサイクルと描画コンテキストを管理するクラス。
 * SceneManager を経由して現在のシーンに update / render を委譲する。
 */
class CanvasManager {
  /**
   * @param {string} canvasId   - 対象となる <canvas> 要素の id
   * @param {number} [dotW=32]  - ドット絵の横ドット数
   * @param {number} [dotH=32]  - ドット絵の縦ドット数
   * @param {number} [fps=60]   - 目標フレームレート
   */
  constructor(canvasId, dotW = 32, dotH = 32, fps = 60) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) {
      throw new Error(`Canvas element "#${canvasId}" not found.`);
    }
    this.ctx = this.canvas.getContext('2d');

    this._fps        = fps;
    this._interval   = 1000 / fps;  // ms/frame
    this._lastTime   = 0;
    this._rafId      = null;

    /** シーン間で共有するアプリケーションデータ @type {AppData} */
    this.appData = new AppData();
    this.appData.createPixelData(dotW, dotH);

    /** 入力管理 @type {Input} */
    this.input = new Input(this.canvas);

    // ウィンドウリサイズに追従する
    this._onResize = this._onResize.bind(this);
    window.addEventListener('resize', this._onResize);

    /** @type {SceneManager} */
    this.sceneManager = new SceneManager(this.appData, this.input, () => this.handleResize());
    this.appData.sceneManager = this.sceneManager;

    // 初期サイズを設定してからループ開始
    this._fitToWindow();
    this.start();
  }

  /** canvas をウィンドウサイズに合わせる */
  _fitToWindow() {
    this.canvas.width  = window.innerWidth;
    this.canvas.height = window.innerHeight;
  }

  /** リサイズイベントハンドラ */
  _onResize() {
    this._fitToWindow();
  }

  /** 外部から明示的にリサイズ処理を走らせる */
  handleResize() {
    this._onResize();
  }

  /** requestAnimationFrame ループの1ステップ */
  _tick(timestamp) {
    this._rafId = requestAnimationFrame(this._tick.bind(this));

    const elapsed = timestamp - this._lastTime;
    if (elapsed < this._interval) return;

    // 端数を次フレームに繰り越してドリフトを防ぐ
    const dt = elapsed;
    this._lastTime = timestamp - (elapsed % this._interval);

    this._update(dt);
    this._render();
  }

  /** 描画ループを開始する */
  start() {
    if (this._rafId !== null) return;
    this._lastTime = performance.now();
    this._rafId = requestAnimationFrame(this._tick.bind(this));
  }

  /** 描画ループを停止する */
  stop() {
    if (this._rafId === null) return;
    cancelAnimationFrame(this._rafId);
    this._rafId = null;
  }

  /** @param {number} dt */
  _update(dt) {
    this.sceneManager.update(dt);
  }

  /** 現在シーンの render を呼ぶ */
  _render() {
    this.sceneManager.render(this.ctx, this.canvas);
  }

  /** イベントリスナーの解除・ループ停止（後始末用） */
  destroy() {
    this.stop();
    this.input.destroy();
    window.removeEventListener('resize', this._onResize);
  }
}
