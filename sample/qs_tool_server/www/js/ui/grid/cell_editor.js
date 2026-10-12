/**
 * CellEditor
 * canvas の上に DOM の <input> / <textarea> を重ねて、セルの文字を編集する。
 * DOM の入力欄なので、日本語入力（IME）がそのまま使える。
 *
 * - Enter: 確定して下へ / Tab: 確定して右へ（Shift+Tab は左へ）/ Esc: 取り消し / フォーカスが外れたら確定
 * - IME の変換中（isComposing / keyCode 229）の Enter や Tab は確定として扱わない
 * - 複数行（text / json）は Shift+Enter で改行する
 * - キー入力は stopPropagation する（Input の window リスナーに届かない）
 */
class CellEditor {
  /**
   * @param {string} [canvasId='mainCanvas']
   */
  constructor(canvasId = 'mainCanvas') {
    this._canvasId = canvasId;
    this._element = null;
    this._options = null;
    this._finished = true;
  }

  get isEditing() {
    return !!this._element;
  }

  /**
   * @param {{
   *   rect: {x:number,y:number,w:number,h:number},
   *   text: string,
   *   multiline?: boolean,
   *   caretAtEnd?: boolean,
   *   onCommit: (text: string, nav: 'none'|'down'|'right'|'left') => boolean|void,
   *   onInput?: (text: string) => void,
   *   onCancel?: () => void,
   * }} options
   */
  begin(options) {
    this.destroy();
    const canvas = document.getElementById(this._canvasId);
    if (!canvas) return;

    this._options = options;
    this._finished = false;
    const element = document.createElement(options.multiline ? 'textarea' : 'input');
    if (!options.multiline) element.type = 'text';
    element.value = options.text ?? '';
    element.spellcheck = false;
    element.autocomplete = 'off';

    const style = element.style;
    style.position = 'fixed';
    style.margin = '0';
    style.padding = '0 6px';
    style.border = '2px solid #38bdf8';
    style.outline = 'none';
    style.background = '#0f172a';
    style.color = '#f8fafc';
    style.boxSizing = 'border-box';
    style.zIndex = '9999';
    style.fontFamily = 'sans-serif';
    if (options.multiline) {
      style.padding = '4px 6px';
      style.resize = 'none';
    }

    element.addEventListener('keydown', (event) => this._onKeyDown(event));
    element.addEventListener('keyup', (event) => event.stopPropagation());
    // 入力のたびに知らせる（検索欄の絞り込みなど）
    element.addEventListener('input', () => options.onInput?.(element.value));
    element.addEventListener('blur', () => this._finish(true, 'none', true));

    document.body.appendChild(element);
    this._element = element;
    this.reposition(options.rect);

    // 同じフレームでフォーカスすると、キー入力で始めた文字が二重に入ることがあるので 1 フレーム待つ
    requestAnimationFrame(() => {
      if (this._element !== element) return;
      element.focus();
      if (options.caretAtEnd) element.setSelectionRange(element.value.length, element.value.length);
      else element.select();
    });
  }

  /**
   * スクロールや列幅の変更に合わせて位置を直す。
   * @param {{x:number,y:number,w:number,h:number}} rect  canvas 座標
   */
  reposition(rect) {
    if (!this._element) return;
    const canvas = document.getElementById(this._canvasId);
    if (!canvas) return;
    const box = canvas.getBoundingClientRect();
    const scaleX = box.width / canvas.width;
    const scaleY = box.height / canvas.height;
    const multiline = !!this._options?.multiline;
    const width = multiline ? Math.max(rect.w, 280) : rect.w;
    const height = multiline ? Math.max(rect.h * 4, 112) : rect.h;
    const style = this._element.style;
    style.left = `${box.left + rect.x * scaleX}px`;
    style.top = `${box.top + rect.y * scaleY}px`;
    style.width = `${width * scaleX}px`;
    style.height = `${height * scaleY}px`;
    style.fontSize = `${13 * scaleY}px`;
  }

  _onKeyDown(event) {
    event.stopPropagation();
    // 変換中の Enter / Tab / Esc は IME のもの
    if (event.isComposing || event.keyCode === 229) return;

    const multiline = !!this._options?.multiline;
    if (event.key === 'Enter') {
      if (multiline && event.shiftKey) return;
      event.preventDefault();
      this._finish(true, 'down');
    } else if (event.key === 'Tab') {
      event.preventDefault();
      this._finish(true, event.shiftKey ? 'left' : 'right');
    } else if (event.key === 'Escape') {
      event.preventDefault();
      this._finish(false, 'none');
    }
  }

  /**
   * @param {boolean} commit
   * @param {'none'|'down'|'right'|'left'} nav
   * @param {boolean} [fromBlur=false]  フォーカスが外れたための確定。検証に失敗したら取り消す（再フォーカスの繰り返しを避ける）
   */
  _finish(commit, nav, fromBlur = false) {
    if (this._finished || !this._element) return;
    const element = this._element;
    const options = this._options;
    const text = element.value;

    if (commit) {
      // onCommit が false を返したら、検証に失敗したとして編集を続ける
      this._finished = true;
      const accepted = options.onCommit(text, nav);
      if (accepted === false) {
        if (!fromBlur) {
          this._finished = false;
          element.style.borderColor = '#ef4444';
          return;
        }
        options.onCancel?.();
      }
    } else {
      this._finished = true;
      options.onCancel?.();
    }
    this._remove(element);
  }

  /** 確定も取り消しもせず、編集欄を閉じる（シーンを離れるときなど）。 */
  destroy() {
    if (!this._element) return;
    this._finished = true;
    this._remove(this._element);
  }

  /** 編集中の内容を確定してから閉じる。 */
  commit() {
    this._finish(true, 'none');
  }

  _remove(element) {
    if (this._element === element) {
      this._element = null;
      this._options = null;
    }
    element.remove();
  }
}
