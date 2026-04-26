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
  static FILE_MAP_RESIZE = 'file.map_resize';

  // ---- 編集 ----
  static EDIT_UNDO       = 'edit.undo';
  static EDIT_REDO       = 'edit.redo';
  static EDIT_CUT        = 'edit.cut';
  static EDIT_COPY       = 'edit.copy';
  static EDIT_PASTE      = 'edit.paste';
  static EDIT_SELECT_ALL = 'edit.select_all';
  static EDIT_FLIP_H     = 'edit.flip_h';
  static EDIT_FLIP_V     = 'edit.flip_v';
  static EDIT_ROTATE_CW  = 'edit.rotate_cw';
  static EDIT_ROTATE_CCW = 'edit.rotate_ccw';

  // ---- 表示 ----
  static VIEW_GRID        = 'view.grid';
  static VIEW_ZOOM_IN     = 'view.zoom_in';
  static VIEW_ZOOM_OUT    = 'view.zoom_out';
  static VIEW_ZOOM_RESET  = 'view.zoom_reset';
  static VIEW_PASS_FLAGS  = 'view.pass_flags';

  // ---- ファイル（タイルセット拡張） ----
  static FILE_NEW_TILESET      = 'file.new_tileset';
  static FILE_OPEN_TILESET     = 'file.open_tileset';
  static FILE_EXPORT_TILESET   = 'file.export_tileset';
  static FILE_EXPORT_CHIP      = 'file.export_chip';

  // ---- タイルセット ----
  static TILESET_COPY_CHIP   = 'tileset.copy_chip';
  static TILESET_PASTE_CHIP  = 'tileset.paste_chip';
  static TILESET_CLEAR_CHIP  = 'tileset.clear_chip';
  static TILESET_SWAP_CHIP   = 'tileset.swap_chip';
  static TILESET_ADD_ROW     = 'tileset.add_row';
  static TILESET_ADD_COL     = 'tileset.add_col';
  static TILESET_REMOVE_ROW  = 'tileset.remove_row';
  static TILESET_REMOVE_COL  = 'tileset.remove_col';
  static TILESET_TILE_PREVIEW = 'tileset.tile_preview';

  // ---- 生成 ----
  static GENERATE_QUARTER_VIEW_TILE = 'generate.quarter_view_tile';
}
