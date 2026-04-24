/**
 * CommandBase
 * Undo / Redo 対応コマンドの基底クラス。
 */
class CommandBase {
  /**
   * コマンドを実行する。
   * 実行しなかった場合は false を返してもよい。
   * @param {AppData} appData
   * @returns {boolean|void}
   */
  execute(appData) {
    throw new Error('CommandBase.execute() must be implemented');
  }

  /**
   * コマンドを取り消す。
   * 取り消さなかった場合は false を返してもよい。
   * @param {AppData} appData
   * @returns {boolean|void}
   */
  undo(appData) {
    throw new Error('CommandBase.undo() must be implemented');
  }

  /**
   * コマンドを再実行する。
   * 既定では execute() を再利用する。
   * @param {AppData} appData
   * @returns {boolean|void}
   */
  redo(appData) {
    return this.execute(appData);
  }

  /**
   * UI 表示用の短いラベル。
   * @returns {string}
   */
  getLabel() {
    return 'command';
  }
}