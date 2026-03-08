/**
 * MenuConstants
 * メニュー項目のIDを一元管理する定数クラス。
 * シーン側でのロジック分岐にも利用できる。
 */
class MenuConstants {
  // セパレーター（DropdownMenu 内で区切り線として扱う）
  static SEPARATOR = 'separator';

  // ---- ファイル ----
  static FILE_NEW     = 'file.new';
  static FILE_OPEN    = 'file.open';
  static FILE_SAVE    = 'file.save';
  static FILE_EXIT    = 'file.exit';

  // ---- 編集 ----
  static EDIT_UNDO       = 'edit.undo';
  static EDIT_REDO       = 'edit.redo';
  static EDIT_CUT        = 'edit.cut';
  static EDIT_COPY       = 'edit.copy';
  static EDIT_PASTE      = 'edit.paste';
  static EDIT_SELECT_ALL = 'edit.select_all';

  // ---- 表示 ----
  static VIEW_GRID        = 'view.grid';
  static VIEW_ZOOM_IN     = 'view.zoom_in';
  static VIEW_ZOOM_OUT    = 'view.zoom_out';
  static VIEW_ZOOM_RESET  = 'view.zoom_reset';

  // ---- ファイル（タイルセット拡張） ----
  static FILE_NEW_TILESET  = 'file.new_tileset';
}
