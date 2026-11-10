/*
 * Copyright (c) Katsuya Owari
 *
 * keikibu_chart.js
 * 家計簿アプリ用 canvas チャート描画モジュール。
 * 外部ライブラリ不使用。ドーナツ / 棒 / ライン を 2D context で描画し、
 * ホバー判定・ツールチップ表示のためのヘルパを提供する。
 */
(function (global) {
  'use strict';

  var FONT = '-apple-system, BlinkMacSystemFont, "Segoe UI", "Hiragino Sans", "Yu Gothic", sans-serif';
  var TAU = Math.PI * 2;

  var PALETTE = [
    '#38bdf8', '#34d399', '#fbbf24', '#f472b6', '#a78bfa',
    '#f87171', '#4ade80', '#fb923c', '#22d3ee', '#e879f9', '#94a3b8'
  ];

  var TEXT_MAIN = '#e5e7eb';
  var TEXT_MUTED = '#94a3b8';
  var GRID_COLOR = 'rgba(148, 163, 184, 0.14)';
  var AXIS_COLOR = 'rgba(148, 163, 184, 0.35)';
  var EMPTY_COLOR = 'rgba(148, 163, 184, 0.25)';
  var BG_COLOR = '#0b1220';

  function fmtInt(n) {
    var v = Math.round(n);
    var s = String(Math.abs(v));
    var out = '';
    while (s.length > 3) {
      out = ',' + s.slice(-3) + out;
      s = s.slice(0, -3);
    }
    return (v < 0 ? '-' : '') + s + out;
  }

  function fmtMoney(n) {
    var v = Math.round(n);
    return (v < 0 ? '-' : '') + '¥' + fmtInt(Math.abs(v));
  }

  function trim1(v) {
    var r = Math.round(v * 10) / 10;
    return (r % 1 === 0) ? String(r) : r.toFixed(1);
  }

  function fmtAxis(n) {
    var v = Math.round(n);
    if (Math.abs(v) >= 100000000) return trim1(v / 100000000) + '億';
    if (Math.abs(v) >= 10000) return trim1(v / 10000) + '万';
    return fmtInt(v);
  }

  function niceMax(v) {
    if (!isFinite(v) || v <= 0) return 10;
    var exp = Math.floor(Math.log(v) / Math.LN10);
    var base = Math.pow(10, exp);
    var norm = v / base;
    var step;
    if (norm <= 1) step = 1;
    else if (norm <= 2) step = 2;
    else if (norm <= 2.5) step = 2.5;
    else if (norm <= 5) step = 5;
    else step = 10;
    return step * base;
  }

  function hexToRgba(hex, alpha) {
    var m = /^#([0-9a-fA-F]{6})$/.exec(hex);
    if (!m) return 'rgba(56, 189, 248, ' + alpha + ')';
    var r = parseInt(m[1].slice(0, 2), 16);
    var g = parseInt(m[1].slice(2, 4), 16);
    var b = parseInt(m[1].slice(4, 6), 16);
    return 'rgba(' + r + ',' + g + ',' + b + ',' + alpha + ')';
  }

  function setupCanvas(canvas) {
    var dpr = global.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    var w = Math.max(10, Math.floor(rect.width));
    var h = Math.max(10, Math.floor(rect.height));
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
    var ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);
    return { ctx: ctx, w: w, h: h };
  }

  function pathRoundTop(ctx, x, y, w, h, r) {
    r = Math.min(r, w / 2, h);
    ctx.beginPath();
    ctx.moveTo(x, y + h);
    ctx.lineTo(x, y + r);
    ctx.arcTo(x, y, x + r, y, r);
    ctx.lineTo(x + w - r, y);
    ctx.arcTo(x + w, y, x + w, y + r, r);
    ctx.lineTo(x + w, y + h);
    ctx.closePath();
  }

  function drawGrid(ctx, padL, padT, plotW, plotH, yMax) {
    var DIV = 4;
    var g;
    ctx.font = '11px ' + FONT;
    for (g = 0; g <= DIV; g++) {
      var val = yMax * g / DIV;
      var gy = padT + plotH - plotH * g / DIV;
      ctx.strokeStyle = (g === 0) ? AXIS_COLOR : GRID_COLOR;
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(padL, Math.round(gy) + 0.5);
      ctx.lineTo(padL + plotW, Math.round(gy) + 0.5);
      ctx.stroke();
      ctx.fillStyle = TEXT_MUTED;
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      ctx.fillText(fmtAxis(val), padL - 8, gy);
    }
  }

  function drawEmpty(ctx, x, y) {
    ctx.fillStyle = TEXT_MUTED;
    ctx.font = '13px ' + FONT;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('データなし', x, y);
  }

  /*
   * ドーナツチャート
   * items: [{label, value, color?}]
   * opts:  {highlight: index|-1, centerLabel: string}
   */
  function drawDonut(canvas, items, opts) {
    opts = opts || {};
    var s = setupCanvas(canvas);
    var ctx = s.ctx;
    var w = s.w;
    var h = s.h;
    var cx = w / 2;
    var cy = h / 2;
    var radius = Math.min(w, h) / 2 - 10;
    var thickness = Math.max(20, radius * 0.42);
    var rIn = radius - thickness;
    var start0 = -Math.PI / 2;
    var total = 0;
    var i;

    for (i = 0; i < items.length; i++) total += items[i].value;

    if (total <= 0) {
      ctx.beginPath();
      ctx.arc(cx, cy, radius - thickness / 2, 0, TAU);
      ctx.lineWidth = thickness;
      ctx.strokeStyle = EMPTY_COLOR;
      ctx.stroke();
      drawEmpty(ctx, cx, cy);
      canvas.__geo = { type: 'donut', start0: start0, total: 0, cx: cx, cy: cy, rIn: rIn, rOut: radius, slices: [] };
      return;
    }

    var slices = [];
    var start = start0;
    for (i = 0; i < items.length; i++) {
      var frac = items[i].value / total;
      var sweep = frac * TAU;
      if (sweep >= TAU) sweep = TAU - 0.0001;
      var end = start + sweep;
      var color = items[i].color || PALETTE[i % PALETTE.length];
      var rOut = (opts.highlight === i) ? radius + 5 : radius;
      ctx.beginPath();
      ctx.arc(cx, cy, rOut, start, end);
      ctx.arc(cx, cy, rIn, end, start, true);
      ctx.closePath();
      ctx.fillStyle = color;
      ctx.fill();
      ctx.strokeStyle = BG_COLOR;
      ctx.lineWidth = 2;
      ctx.stroke();
      slices.push({ start: start, end: end, label: items[i].label, value: items[i].value, pct: frac, color: color });
      start = end;
    }

    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillStyle = TEXT_MAIN;
    ctx.font = '700 15px ' + FONT;
    ctx.fillText(fmtMoney(total), cx, cy + 1);
    if (opts.centerLabel) {
      ctx.fillStyle = TEXT_MUTED;
      ctx.font = '11px ' + FONT;
      ctx.fillText(opts.centerLabel, cx, cy + 17);
    }

    canvas.__geo = { type: 'donut', start0: start0, total: total, cx: cx, cy: cy, rIn: rIn, rOut: radius, slices: slices };
  }

  /*
   * 棒グラフ
   * items: [{label, value, color?}]
   * opts:  {color: string, highlight: index|-1}
   */
  function drawBar(canvas, items, opts) {
    opts = opts || {};
    var s = setupCanvas(canvas);
    var ctx = s.ctx;
    var w = s.w;
    var h = s.h;
    var padL = 50, padR = 10, padT = 20, padB = 24;
    var plotW = w - padL - padR;
    var plotH = h - padT - padB;
    var n = items.length;
    var i;

    var maxV = 0;
    for (i = 0; i < n; i++) if (items[i].value > maxV) maxV = items[i].value;
    var yMax = niceMax(maxV);

    drawGrid(ctx, padL, padT, plotW, plotH, yMax);

    if (n === 0 || maxV <= 0) {
      drawEmpty(ctx, padL + plotW / 2, padT + plotH / 2);
    } else {
      var slot = plotW / n;
      var barW = Math.min(26, Math.max(2, slot * 0.6));
      for (i = 0; i < n; i++) {
        var v = items[i].value;
        var bh = (v / yMax) * plotH;
        var x = padL + slot * i + (slot - barW) / 2;
        var y = padT + plotH - bh;
        var color = items[i].color || opts.color || PALETTE[0];
        if (v > 0) {
          pathRoundTop(ctx, x, y, barW, bh, Math.min(3, barW / 2));
          ctx.fillStyle = color;
          ctx.fill();
        }
        if (opts.highlight === i) {
          if (v > 0) {
            pathRoundTop(ctx, x, y, barW, bh, Math.min(3, barW / 2));
            ctx.fillStyle = 'rgba(255,255,255,0.28)';
            ctx.fill();
          }
          var label = fmtMoney(v);
          ctx.font = '700 11px ' + FONT;
          var tw = ctx.measureText(label).width;
          var lx = x + barW / 2;
          lx = Math.max(padL + tw / 2 + 2, Math.min(padL + plotW - tw / 2 - 2, lx));
          var ly = Math.max(padT + 8, y - 8);
          ctx.fillStyle = TEXT_MAIN;
          ctx.textAlign = 'center';
          ctx.textBaseline = 'bottom';
          ctx.fillText(label, lx, ly);
        }
      }
      ctx.fillStyle = TEXT_MUTED;
      ctx.font = '10px ' + FONT;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      var step = Math.max(1, Math.ceil(n / 12));
      for (i = 0; i < n; i += step) {
        ctx.fillText(items[i].label, padL + slot * i + slot / 2, padT + plotH + 6);
      }
    }

    canvas.__geo = { type: 'bar', items: items, n: n, padL: padL, padT: padT, plotW: plotW, plotH: plotH, yMax: yMax };
  }

  /*
   * ラインチャート（多系列）
   * labels: [string]
   * series: [{label, color, values: [number]}]
   * opts:   {highlight: index|-1}
   */
  function drawLine(canvas, labels, series, opts) {
    opts = opts || {};
    var s = setupCanvas(canvas);
    var ctx = s.ctx;
    var w = s.w;
    var h = s.h;
    var padL = 50, padR = 14, padT = 20, padB = 26;
    var plotW = w - padL - padR;
    var plotH = h - padT - padB;
    var n = labels.length;
    var i, j;

    var maxV = 0;
    for (j = 0; j < series.length; j++) {
      for (i = 0; i < series[j].values.length; i++) {
        if (series[j].values[i] > maxV) maxV = series[j].values[i];
      }
    }
    var yMax = niceMax(maxV);

    drawGrid(ctx, padL, padT, plotW, plotH, yMax);

    var xs = [];
    for (i = 0; i < n; i++) {
      xs.push(n <= 1 ? padL + plotW / 2 : padL + plotW * i / (n - 1));
    }
    function yFor(v) {
      return padT + plotH - (v / yMax) * plotH;
    }

    if (maxV <= 0) {
      drawEmpty(ctx, padL + plotW / 2, padT + plotH / 2);
    } else {
      if (typeof opts.highlight === 'number' && opts.highlight >= 0 && opts.highlight < n) {
        ctx.strokeStyle = 'rgba(226, 232, 240, 0.35)';
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.moveTo(Math.round(xs[opts.highlight]) + 0.5, padT);
        ctx.lineTo(Math.round(xs[opts.highlight]) + 0.5, padT + plotH);
        ctx.stroke();
        ctx.setLineDash([]);
      }
      for (j = 0; j < series.length; j++) {
        var sr = series[j];
        var color = sr.color || PALETTE[j % PALETTE.length];

        var grad = ctx.createLinearGradient(0, padT, 0, padT + plotH);
        grad.addColorStop(0, hexToRgba(color, 0.22));
        grad.addColorStop(1, hexToRgba(color, 0));
        ctx.beginPath();
        ctx.moveTo(xs[0], yFor(sr.values[0]));
        for (i = 1; i < n; i++) ctx.lineTo(xs[i], yFor(sr.values[i]));
        ctx.lineTo(xs[n - 1], padT + plotH);
        ctx.lineTo(xs[0], padT + plotH);
        ctx.closePath();
        ctx.fillStyle = grad;
        ctx.fill();

        ctx.beginPath();
        ctx.moveTo(xs[0], yFor(sr.values[0]));
        for (i = 1; i < n; i++) ctx.lineTo(xs[i], yFor(sr.values[i]));
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.lineJoin = 'round';
        ctx.stroke();

        for (i = 0; i < n; i++) {
          var isHi = (opts.highlight === i);
          ctx.beginPath();
          ctx.arc(xs[i], yFor(sr.values[i]), isHi ? 4.5 : 2.5, 0, TAU);
          ctx.fillStyle = color;
          ctx.fill();
          ctx.strokeStyle = BG_COLOR;
          ctx.lineWidth = 1.5;
          ctx.stroke();
        }
      }
    }

    ctx.fillStyle = TEXT_MUTED;
    ctx.font = '10px ' + FONT;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    for (i = 0; i < n; i++) ctx.fillText(labels[i], xs[i], padT + plotH + 7);

    canvas.__geo = { type: 'line', labels: labels, series: series, xs: xs, n: n, padL: padL, padT: padT, plotW: plotW, plotH: plotH, yMax: yMax };
  }

  /* ---- ホバー判定 ---- */

  function hitDonut(canvas, mx, my) {
    var g = canvas.__geo;
    if (!g || g.type !== 'donut' || g.slices.length === 0) return null;
    var dx = mx - g.cx;
    var dy = my - g.cy;
    var r = Math.sqrt(dx * dx + dy * dy);
    if (r < g.rIn || r > g.rOut + 6) return null;
    var a = Math.atan2(dy, dx);
    if (a < g.start0) a += TAU;
    for (var i = 0; i < g.slices.length; i++) {
      var sl = g.slices[i];
      if (a >= sl.start && a < sl.end) {
        return { index: i, label: sl.label, value: sl.value, pct: sl.pct, color: sl.color };
      }
    }
    return null;
  }

  function hitBar(canvas, mx, my) {
    var g = canvas.__geo;
    if (!g || g.type !== 'bar' || g.n === 0) return null;
    if (mx < g.padL || mx > g.padL + g.plotW) return null;
    if (my < g.padT - 4 || my > g.padT + g.plotH + 4) return null;
    var rel = (mx - g.padL) / g.plotW;
    var i = Math.floor(rel * g.n);
    if (i < 0) i = 0;
    if (i >= g.n) i = g.n - 1;
    return { index: i, label: g.items[i].label, value: g.items[i].value, color: g.items[i].color };
  }

  function hitLine(canvas, mx, my) {
    var g = canvas.__geo;
    if (!g || g.type !== 'line' || g.n === 0) return null;
    if (mx < g.padL - 8 || mx > g.padL + g.plotW + 8) return null;
    if (my < g.padT - 8 || my > g.padT + g.plotH + 8) return null;
    var i = 0;
    if (g.n > 1) {
      i = Math.round(((mx - g.padL) / g.plotW) * (g.n - 1));
      if (i < 0) i = 0;
      if (i > g.n - 1) i = g.n - 1;
    }
    return { index: i, label: g.labels[i] };
  }

  /*
   * ホバー + ツールチップ配線
   * options: {
   *   hit: function(mx, my) -> hit|null   （hit.html は html 文字列）
   *   onHover: function(index|null)
   * }
   */
  function attachHover(canvas, tooltipEl, options) {
    var lastIndex = null;

    function clearTip() {
      if (tooltipEl) tooltipEl.style.display = 'none';
    }

    canvas.addEventListener('mousemove', function (ev) {
      var rect = canvas.getBoundingClientRect();
      var mx = ev.clientX - rect.left;
      var my = ev.clientY - rect.top;
      var hit = options.hit ? options.hit(mx, my) : null;
      var index = (hit && typeof hit.index === 'number') ? hit.index : null;
      if (index !== lastIndex && options.onHover) options.onHover(index);
      lastIndex = index;
      if (!hit || !tooltipEl) {
        clearTip();
        return;
      }
      tooltipEl.innerHTML = hit.html || '';
      tooltipEl.style.display = 'block';
      var scrollX = global.scrollX || 0;
      var scrollY = global.scrollY || 0;
      var docW = Math.max(global.document.documentElement.clientWidth, global.innerWidth);
      var docH = Math.max(global.document.documentElement.clientHeight, global.innerHeight);
      var left = rect.left + scrollX + mx + 14;
      var top = rect.top + scrollY + my + 14;
      var tw = tooltipEl.offsetWidth;
      var th = tooltipEl.offsetHeight;
      if (left + tw > scrollX + docW - 8) left = rect.left + scrollX + mx - tw - 14;
      if (top + th > scrollY + docH - 8) top = rect.top + scrollY + my - th - 14;
      tooltipEl.style.left = Math.round(left) + 'px';
      tooltipEl.style.top = Math.round(top) + 'px';
    });

    canvas.addEventListener('mouseleave', function () {
      clearTip();
      lastIndex = null;
      if (options.onHover) options.onHover(null);
    });
  }

  global.KeikibuChart = {
    PALETTE: PALETTE,
    fmtInt: fmtInt,
    fmtMoney: fmtMoney,
    fmtAxis: fmtAxis,
    niceMax: niceMax,
    drawDonut: drawDonut,
    drawBar: drawBar,
    drawLine: drawLine,
    hitDonut: hitDonut,
    hitBar: hitBar,
    hitLine: hitLine,
    attachHover: attachHover
  };
})(typeof window !== 'undefined' ? window : this);
