/**
 * ConditionalAction
 * 複数条件を評価し、最初にマッチした条件のアクションを実行するコンポーネント
 *
 * 対応条件タイプ:
 * - compare: 数値比較（>, >=, <, <=, ===, !==）
 * - equals: 文字列完全一致
 * - truthy: 真偽値判定
 * - has: オブジェクト内のプロパティ存在確認
 * - exists: グローバル変数の存在確認
 */
class ConditionalAction {
  /**
   * 条件を評価する
   * @param {object|null} condition - 条件オブジェクト
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean} 条件が真か
   */
  static evaluateCondition(condition, appData) {
    if (!condition || typeof condition !== 'object') {
      console.warn('[ConditionalAction] Missing condition object');
      return false;
    }
    
    const { type, left, operator, right } = condition;
    
    switch (type) {
      case 'compare':
        return ConditionalAction._evaluateCompare(left, operator, right, appData);
      
      case 'equals':
        return ConditionalAction._evaluateEquals(left, right, appData);
      
      case 'truthy':
        return ConditionalAction._evaluateTruthy(left, appData);
      
      case 'has':
        return ConditionalAction._evaluateHas(left, right, appData);
      
      case 'exists':
        return ConditionalAction._evaluateExists(left, appData);
      
      default:
        console.warn(`[ConditionalAction] Unknown condition type: ${type}`);
        return false;
    }
  }

  /**
   * 数値比較条件を評価
   * @param {string|number} left - 左側の値（テンプレート変数またはリテラル）
   * @param {string} operator - 比較演算子（>, >=, <, <=, ===, !==）
   * @param {string|number} right - 右側の値
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean}
   */
  static _evaluateCompare(left, operator, right, appData) {
    const leftVal = ConditionalAction._resolveValue(left, appData);
    const rightVal = ConditionalAction._resolveValue(right, appData);
    const leftNum = Number(leftVal);
    const rightNum = Number(rightVal);
    
    if (!Number.isFinite(leftNum) || !Number.isFinite(rightNum)) {
      console.warn(`[ConditionalAction] compare: non-numeric values`, { left: leftVal, right: rightVal });
      return false;
    }
    
    switch (operator) {
      case '>':
        return leftNum > rightNum;
      case '>=':
        return leftNum >= rightNum;
      case '<':
        return leftNum < rightNum;
      case '<=':
        return leftNum <= rightNum;
      case '===':
        return leftNum === rightNum;
      case '!==':
        return leftNum !== rightNum;
      default:
        console.warn(`[ConditionalAction] Unknown operator: ${operator}`);
        return false;
    }
  }

  /**
   * 文字列完全一致条件を評価
   * @param {string|number} left - 左側の値
   * @param {string|number} right - 右側の値
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean}
   */
  static _evaluateEquals(left, right, appData) {
    const leftVal = String(ConditionalAction._resolveValue(left, appData));
    const rightVal = String(ConditionalAction._resolveValue(right, appData));
    return leftVal === rightVal;
  }

  /**
   * 真偽値判定条件を評価
   * @param {*} left - 評価対象の値
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean}
   */
  static _evaluateTruthy(left, appData) {
    return !!ConditionalAction._resolveValue(left, appData);
  }

  /**
   * オブジェクト内のプロパティ存在確認
   * @param {string|number} left - オブジェクト（テンプレート変数またはリテラル）
   * @param {string|number} right - プロパティキー
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean}
   */
  static _evaluateHas(left, right, appData) {
    const leftVal = ConditionalAction._resolveValue(left, appData);
    if (typeof leftVal !== 'object' || !leftVal) {
      console.warn(`[ConditionalAction] has: left value is not an object`, { left: leftVal });
      return false;
    }
    return right in leftVal;
  }

  /**
   * グローバル変数の存在確認
   * @param {string} left - グローバル変数パス（例: "user.persistent.score"）
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {boolean}
   */
  static _evaluateExists(left, appData) {
    if (!appData || typeof appData.hasRuntimeGlobalVariable !== 'function') {
      console.warn(`[ConditionalAction] exists: appData.hasRuntimeGlobalVariable is not available`);
      return false;
    }
    const variablePath = typeof left === 'string' ? left.trim() : '';
    if (!variablePath) {
      console.warn('[ConditionalAction] exists: variable path is empty');
      return false;
    }
    return appData.hasRuntimeGlobalVariable(variablePath) === true;
  }

  /**
   * 値を解決する（テンプレート変数を展開またはリテラル値を返す）
   * @param {*} value - テンプレート変数 ${...} またはリテラル値
   * @param {AppData} appData - グローバル変数にアクセスするAppData
   * @returns {*} 解決された値
   */
  static _resolveValue(value, appData) {
    // リテラル値（非文字列）はそのまま返す
    if (typeof value !== 'string') {
      return value;
    }
    
    // テンプレート変数 ${...} を検出
    if (value.startsWith('${') && value.endsWith('}')) {
      const varPath = value.slice(2, -1).trim();
      if (!varPath) {
        console.warn('[ConditionalAction] _resolveValue: variable path is empty');
        return undefined;
      }
      
      if (!appData || typeof appData.getRuntimeGlobalVariable !== 'function') {
        console.warn(`[ConditionalAction] _resolveValue: appData.getRuntimeGlobalVariable is not available`);
        return value;  // フォールバック：元の値を返す
      }
      
      const resolvedValue = appData.getRuntimeGlobalVariable(varPath);
      
      // undefined/null の場合は明示的に返す
      if (resolvedValue === undefined || resolvedValue === null) {
        console.debug(`[ConditionalAction] _resolveValue: variable not found`, { varPath });
        return resolvedValue;
      }
      
      return resolvedValue;
    }
    
    // テンプレート変数ではない文字列はそのまま返す
    return value;
  }
}

// Node.js/CommonJS 環境での export（オプション）
if (typeof module !== 'undefined' && module.exports) {
  module.exports = ConditionalAction;
}
