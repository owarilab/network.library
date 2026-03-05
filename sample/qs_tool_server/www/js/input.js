/**
 * Input
 * キーボード・マウス・タッチイベントを一元管理するクラス。
 * on(eventName, handler) でコールバックを登録し、シーンごとの入力ロジックを実装する。
 *
 * 対応イベント名:
 *   キーボード : 'keydown' | 'keyup'
 *   マウス     : 'mousedown' | 'mouseup' | 'mousemove' | 'wheel' | 'contextmenu'
 *   タッチ     : 'touchstart' | 'touchend' | 'touchmove' | 'touchcancel'
 */
class Input {
  /**
   * @param {HTMLElement} target - マウス/タッチイベントを監視する要素（通常は canvas）
   */
  constructor(target) {
    this._target = target;

    /** @type {Map<string, Set<Function>>} */
    this._listeners = new Map();

    // ---- DOM イベントを内部ハンドラに束縛 ----
    this._handlers = {
      // キーボード（window に登録）
      keydown:     this._onKeyboard.bind(this),
      keyup:       this._onKeyboard.bind(this),
      // マウス（target に登録）
      mousedown:   this._onMouse.bind(this),
      mouseup:     this._onMouse.bind(this),
      mousemove:   this._onMouse.bind(this),
      wheel:       this._onWheel.bind(this),
      contextmenu: this._onContextMenu.bind(this),
      // タッチ（target に登録）
      touchstart:  this._onTouch.bind(this),
      touchend:    this._onTouch.bind(this),
      touchmove:   this._onTouch.bind(this),
      touchcancel: this._onTouch.bind(this),
    };

    // キーボードは window、それ以外は target に登録
    window.addEventListener('keydown',     this._handlers.keydown);
    window.addEventListener('keyup',       this._handlers.keyup);

    const opt = { passive: false };
    target.addEventListener('mousedown',   this._handlers.mousedown);
    target.addEventListener('mouseup',     this._handlers.mouseup);
    target.addEventListener('mousemove',   this._handlers.mousemove);
    target.addEventListener('wheel',       this._handlers.wheel,       opt);
    target.addEventListener('contextmenu', this._handlers.contextmenu);
    target.addEventListener('touchstart',  this._handlers.touchstart,  opt);
    target.addEventListener('touchend',    this._handlers.touchend,    opt);
    target.addEventListener('touchmove',   this._handlers.touchmove,   opt);
    target.addEventListener('touchcancel', this._handlers.touchcancel);
  }

  // ----------------------------------------------------------------
  // 公開 API
  // ----------------------------------------------------------------

  /**
   * イベントハンドラを登録する
   * @param {string}   eventName
   * @param {Function} handler
   */
  on(eventName, handler) {
    if (!this._listeners.has(eventName)) {
      this._listeners.set(eventName, new Set());
    }
    this._listeners.get(eventName).add(handler);
  }

  /**
   * イベントハンドラを解除する
   * @param {string}   eventName
   * @param {Function} handler
   */
  off(eventName, handler) {
    this._listeners.get(eventName)?.delete(handler);
  }

  /**
   * 登録されている全ハンドラを解除する（シーン切り替え時に SceneManager が呼ぶ）
   */
  clearAll() {
    this._listeners.clear();
  }

  /** DOM イベントリスナーをすべて解除する（後始末用） */
  destroy() {
    window.removeEventListener('keydown',     this._handlers.keydown);
    window.removeEventListener('keyup',       this._handlers.keyup);

    this._target.removeEventListener('mousedown',   this._handlers.mousedown);
    this._target.removeEventListener('mouseup',     this._handlers.mouseup);
    this._target.removeEventListener('mousemove',   this._handlers.mousemove);
    this._target.removeEventListener('wheel',       this._handlers.wheel);
    this._target.removeEventListener('contextmenu', this._handlers.contextmenu);
    this._target.removeEventListener('touchstart',  this._handlers.touchstart);
    this._target.removeEventListener('touchend',    this._handlers.touchend);
    this._target.removeEventListener('touchmove',   this._handlers.touchmove);
    this._target.removeEventListener('touchcancel', this._handlers.touchcancel);

    this._listeners.clear();
  }

  // ----------------------------------------------------------------
  // 内部ハンドラ
  // ----------------------------------------------------------------

  /** @param {KeyboardEvent} e */
  _onKeyboard(e) {
    this._emit(e.type, {
      type:  e.type,
      key:   e.key,
      code:  e.code,
      alt:   e.altKey,
      ctrl:  e.ctrlKey,
      shift: e.shiftKey,
      meta:  e.metaKey,
      raw:   e,
    });
  }

  /** @param {MouseEvent} e */
  _onMouse(e) {
    const pos = this._relativePos(e.clientX, e.clientY);
    this._emit(e.type, {
      type:    e.type,
      x:       pos.x,
      y:       pos.y,
      button:  e.button,
      buttons: e.buttons,
      raw:     e,
    });
  }

  /** @param {WheelEvent} e */
  _onWheel(e) {
    e.preventDefault();
    const pos = this._relativePos(e.clientX, e.clientY);
    this._emit('wheel', {
      type:   'wheel',
      x:      pos.x,
      y:      pos.y,
      deltaX: e.deltaX,
      deltaY: e.deltaY,
      deltaZ: e.deltaZ,
      raw:    e,
    });
  }

  /** @param {MouseEvent} e */
  _onContextMenu(e) {
    e.preventDefault();
    const pos = this._relativePos(e.clientX, e.clientY);
    this._emit('contextmenu', {
      type: 'contextmenu',
      x:    pos.x,
      y:    pos.y,
      raw:  e,
    });
  }

  /** @param {TouchEvent} e */
  _onTouch(e) {
    e.preventDefault();
    const rect = this._target.getBoundingClientRect();
    const touches = Array.from(e.touches).map(t => ({
      id: t.identifier,
      x:  t.clientX - rect.left,
      y:  t.clientY - rect.top,
    }));
    const changedTouches = Array.from(e.changedTouches).map(t => ({
      id: t.identifier,
      x:  t.clientX - rect.left,
      y:  t.clientY - rect.top,
    }));
    this._emit(e.type, {
      type:          e.type,
      touches,
      changedTouches,
      raw:           e,
    });
  }

  // ----------------------------------------------------------------
  // ユーティリティ
  // ----------------------------------------------------------------

  /**
   * canvas 内の相対座標を返す
   * @param {number} clientX
   * @param {number} clientY
   * @returns {{ x: number, y: number }}
   */
  _relativePos(clientX, clientY) {
    const rect = this._target.getBoundingClientRect();
    return { x: clientX - rect.left, y: clientY - rect.top };
  }

  /**
   * 登録済みハンドラを呼び出す
   * @param {string} eventName
   * @param {object} data
   */
  _emit(eventName, data) {
    this._listeners.get(eventName)?.forEach(fn => fn(data));
  }
}
