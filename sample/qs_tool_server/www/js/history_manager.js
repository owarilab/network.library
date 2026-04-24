/**
 * HistoryManager
 * コマンドの実行履歴を管理する。
 * グローバル script 読み込み前提のため export は使わない。
 */
class HistoryManager {
  /**
   * @param {number} [limit=100]
   */
  constructor(limit = 100) {
    /** @type {CommandBase[]} */
    this._undoStack = [];
    /** @type {CommandBase[]} */
    this._redoStack = [];
    /** @type {number} */
    this._limit = Math.max(1, limit | 0);
    /** @type {boolean} */
    this._isApplying = false;
  }

  /**
   * コマンドを実行して undo スタックへ積む。
   * execute() が false を返した場合は履歴に積まない。
   * @param {CommandBase} command
   * @param {AppData} appData
   * @returns {boolean}
   */
  execute(command, appData) {
    if (!command || typeof command.execute !== 'function') return false;
    if (this._isApplying) return false;

    this._isApplying = true;
    try {
      const applied = command.execute(appData);
      if (applied === false) return false;

      this._undoStack.push(command);
      if (this._undoStack.length > this._limit) {
        this._undoStack.splice(0, this._undoStack.length - this._limit);
      }
      this._redoStack.length = 0;
      return true;
    } finally {
      this._isApplying = false;
    }
  }

  /**
   * 直前のコマンドを取り消す。
   * undo() が false を返した場合は redo 側へ移さない。
   * @param {AppData} appData
   * @returns {boolean}
   */
  undo(appData) {
    if (this._isApplying || this._undoStack.length === 0) return false;

    const command = this._undoStack[this._undoStack.length - 1];
    if (!command || typeof command.undo !== 'function') return false;

    this._isApplying = true;
    try {
      const applied = command.undo(appData);
      if (applied === false) return false;

      this._undoStack.pop();
      this._redoStack.push(command);
      return true;
    } finally {
      this._isApplying = false;
    }
  }

  /**
   * 直前に取り消したコマンドを再実行する。
   * redo() が false を返した場合は undo 側へ戻さない。
   * @param {AppData} appData
   * @returns {boolean}
   */
  redo(appData) {
    if (this._isApplying || this._redoStack.length === 0) return false;

    const command = this._redoStack[this._redoStack.length - 1];
    if (!command || typeof command.redo !== 'function') return false;

    this._isApplying = true;
    try {
      const applied = command.redo(appData);
      if (applied === false) return false;

      this._redoStack.pop();
      this._undoStack.push(command);
      return true;
    } finally {
      this._isApplying = false;
    }
  }

  /**
   * 履歴を全消去する。
   */
  clear() {
    if (this._isApplying) return;
    this._undoStack.length = 0;
    this._redoStack.length = 0;
  }

  /**
   * @returns {boolean}
   */
  canUndo() {
    return this._undoStack.length > 0;
  }

  /**
   * @returns {boolean}
   */
  canRedo() {
    return this._redoStack.length > 0;
  }

  /**
   * @returns {string}
   */
  getUndoLabel() {
    if (!this.canUndo()) return '';
    return this._getCommandLabel(this._undoStack[this._undoStack.length - 1]);
  }

  /**
   * @returns {string}
   */
  getRedoLabel() {
    if (!this.canRedo()) return '';
    return this._getCommandLabel(this._redoStack[this._redoStack.length - 1]);
  }

  /**
   * @returns {boolean}
   */
  get isApplying() {
    return this._isApplying;
  }

  /**
   * @param {CommandBase|null|undefined} command
   * @returns {string}
   */
  _getCommandLabel(command) {
    if (!command || typeof command.getLabel !== 'function') return 'command';
    const label = command.getLabel();
    return typeof label === 'string' && label ? label : 'command';
  }
}