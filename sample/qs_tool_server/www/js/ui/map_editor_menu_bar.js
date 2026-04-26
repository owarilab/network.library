class MapEditorMenuBar extends MenuBar {
  constructor() {
    super();
    this._menuDefs = [
      {
        label: 'ファイル',
        dropdown: new DropdownMenu([
          { id: MenuConstants.FILE_SAVE, label: '保存' },
          { id: MenuConstants.FILE_MAP_RESIZE, label: 'マップサイズ変更...' },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.FILE_EXIT, label: 'プロジェクトトップへ戻る' },
        ]),
      },
      {
        label: '表示',
        dropdown: new DropdownMenu([
          { id: MenuConstants.VIEW_GRID, label: 'グリッド表示' },
        ]),
      },
    ];
  }

  containsInteractivePoint(x, y) {
    if (y >= 0 && y < MenuBar.HEIGHT) return true;
    const openIndex = this._openIndex;
    if (openIndex < 0) return false;
    const dropdown = this._menuDefs[openIndex]?.dropdown;
    return !!dropdown?.containsPoint?.(x, y);
  }
}