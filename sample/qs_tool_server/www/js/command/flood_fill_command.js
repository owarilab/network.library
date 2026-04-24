/**
 * FloodFillCommand
 * 塗りつぶし 1 回分の差分を保持する。
 */
class FloodFillCommand extends PixelStrokeCommand {
  /**
   * @param {{
   *   mode: 'free'|'tileset',
   *   chip: { col: number, row: number }|null,
   *   layerIndex: number,
   *   changes: Array<{ x: number, y: number, before: number, after: number }>,
   *   label?: string,
   * }} params
   */
  constructor(params) {
    super({
      ...params,
      label: typeof params?.label === 'string' && params.label ? params.label : 'fill',
    });
  }
}