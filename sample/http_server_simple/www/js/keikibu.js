/*
 * Copyright (c) Katsuya Owari
 *
 * keikibu.js
 * 家計簿アプリ本体。
 * - KeikibuCore: DOM 非依存のコアルロジック（node 上で検証可能）
 * - DOM アプリ: フォーム・表・canvas チャートの配線
 * 外部ライブラリ不使用。永続化は localStorage。
 */
(function (global) {
  'use strict';

  var STORAGE_KEY = 'keikibu.entries.v1';
  var TREND_MONTHS = 6;

  // ============ KeikibuCore（DOM 非依存） ============

  var Core = {};

  Core.CATEGORIES = {
    expense: ['食費', '交通費', '住居・光熱', '通信費', '医療費', '教育費', '娯楽', '買い物', 'その他'],
    income: ['給与', '賞与', '給付金', '収入その他']
  };

  Core.pad2 = function (n) {
    return (n < 10 ? '0' : '') + n;
  };

  Core.fmtMoney = function (n) {
    var v = Math.round(n);
    var s = String(Math.abs(v));
    var out = '';
    while (s.length > 3) {
      out = ',' + s.slice(-3) + out;
      s = s.slice(0, -3);
    }
    return (v < 0 ? '-' : '') + '¥' + s + out;
  };

  Core.fmtDate = function (d) {
    return d.getFullYear() + '-' + Core.pad2(d.getMonth() + 1) + '-' + Core.pad2(d.getDate());
  };

  Core.todayStr = function (now) {
    return Core.fmtDate(now || new Date());
  };

  Core.monthPrefix = function (y, m) {
    return y + '-' + Core.pad2(m);
  };

  Core.daysInMonth = function (y, m) {
    return new Date(y, m, 0).getDate();
  };

  Core.monthLabel = function (y, m) {
    return y + '年' + m + '月';
  };

  Core.isValidEntry = function (e) {
    if (!e || typeof e !== 'object') return false;
    if (typeof e.id !== 'string' || !e.id) return false;
    if (typeof e.date !== 'string' || !/^\d{4}-\d{2}-\d{2}$/.test(e.date)) return false;
    var d = new Date(e.date + 'T00:00:00');
    if (isNaN(d.getTime())) return false;
    if (Core.fmtDate(d) !== e.date) return false; // 2026-02-30 等を拒否
    if (e.type !== 'income' && e.type !== 'expense') return false;
    if (typeof e.category !== 'string' || !e.category) return false;
    if (typeof e.amount !== 'number' || !isFinite(e.amount) || e.amount <= 0) return false;
    if (typeof e.memo !== 'string') return false;
    return true;
  };

  Core.monthTotals = function (entries, prefix) {
    var income = 0;
    var expense = 0;
    for (var i = 0; i < entries.length; i++) {
      var e = entries[i];
      if (e.date.indexOf(prefix) !== 0) continue;
      if (e.type === 'income') income += e.amount;
      else expense += e.amount;
    }
    return { income: income, expense: expense, balance: income - expense };
  };

  Core.byCategory = function (entries, type) {
    var map = {};
    var i;
    for (i = 0; i < entries.length; i++) {
      var e = entries[i];
      if (e.type !== type) continue;
      map[e.category] = (map[e.category] || 0) + e.amount;
    }
    var list = [];
    for (var k in map) {
      if (Object.prototype.hasOwnProperty.call(map, k)) {
        list.push({ category: k, value: map[k] });
      }
    }
    list.sort(function (a, b) {
      return b.value - a.value || a.category.localeCompare(b.category, 'ja');
    });
    return list;
  };

  Core.dailyTotals = function (entries, y, m) {
    var prefix = Core.monthPrefix(y, m);
    var dim = Core.daysInMonth(y, m);
    var out = new Array(dim);
    var i;
    for (i = 0; i < dim; i++) out[i] = 0;
    for (i = 0; i < entries.length; i++) {
      var e = entries[i];
      if (e.type !== 'expense') continue;
      if (e.date.indexOf(prefix) !== 0) continue;
      var day = parseInt(e.date.slice(8, 10), 10);
      if (day >= 1 && day <= dim) out[day - 1] += e.amount;
    }
    return out;
  };

  Core.lastMonths = function (y, m, n) {
    var out = [];
    for (var i = n - 1; i >= 0; i--) {
      var d = new Date(y, m - 1 - i, 1);
      out.push({
        y: d.getFullYear(),
        m: d.getMonth() + 1,
        prefix: Core.monthPrefix(d.getFullYear(), d.getMonth() + 1)
      });
    }
    return out;
  };

  Core.trendLabels = function (months) {
    return months.map(function (mo) { return mo.m + '月'; });
  };

  Core.monthSeries = function (entries, months, type) {
    return months.map(function (mo) {
      var t = Core.monthTotals(entries, mo.prefix);
      return (type === 'income') ? t.income : t.expense;
    });
  };

  Core.makeSampleEntries = function (now) {
    now = now || new Date();
    var entries = [];
    var seq = 0;
    function rid() {
      seq++;
      return 'smp-' + now.getTime().toString(36) + '-' + seq.toString(36);
    }
    function rnd(min, max) {
      return Math.floor(min + Math.random() * (max - min + 1));
    }
    var expensePlan = [
      { cat: '食費', w: 40, min: 400, max: 3500 },
      { cat: '買い物', w: 15, min: 800, max: 12000 },
      { cat: '娯楽', w: 14, min: 500, max: 8000 },
      { cat: '交通費', w: 12, min: 200, max: 2500 },
      { cat: '医療費', w: 5, min: 500, max: 6000 },
      { cat: '教育費', w: 5, min: 800, max: 9000 },
      { cat: '通信費', w: 4, min: 1500, max: 4500 },
      { cat: 'その他', w: 5, min: 300, max: 6000 }
    ];
    var totalW = 0;
    var i;
    for (i = 0; i < expensePlan.length; i++) totalW += expensePlan[i].w;
    function pickExpenseCat() {
      var r = Math.random() * totalW;
      for (var j = 0; j < expensePlan.length; j++) {
        r -= expensePlan[j].w;
        if (r <= 0) return expensePlan[j];
      }
      return expensePlan[expensePlan.length - 1];
    }

    var today = now.getDate();
    for (var back = 3; back >= 0; back--) {
      var d = new Date(now.getFullYear(), now.getMonth() - back, 1);
      var y = d.getFullYear();
      var m = d.getMonth() + 1;
      var dim = Core.daysInMonth(y, m);
      var lastDay = (back === 0) ? today : dim;

      if (25 <= lastDay) {
        entries.push({ id: rid(), date: Core.monthPrefix(y, m) + '-' + Core.pad2(25), type: 'income', category: '給与', amount: rnd(290000, 330000), memo: '月給' });
      }
      if (back === 1) {
        entries.push({ id: rid(), date: Core.monthPrefix(y, m) + '-' + Core.pad2(10), type: 'income', category: '収入その他', amount: rnd(8000, 25000), memo: '副業' });
      }
      if (1 <= lastDay) {
        entries.push({ id: rid(), date: Core.monthPrefix(y, m) + '-' + Core.pad2(1), type: 'expense', category: '住居・光熱', amount: rnd(68000, 72000), memo: '家賃' });
      }
      var count = rnd(16, 24);
      for (var k = 0; k < count; k++) {
        var day = rnd(1, lastDay);
        var plan = pickExpenseCat();
        entries.push({
          id: rid(),
          date: Core.monthPrefix(y, m) + '-' + Core.pad2(day),
          type: 'expense',
          category: plan.cat,
          amount: rnd(plan.min, plan.max),
          memo: ''
        });
      }
    }
    return entries;
  };

  global.KeikibuCore = Core;

  // DOM がない環境（node 等）ではここで終了
  if (typeof document === 'undefined' || !document.getElementById) return;

  // ============ DOM アプリ ============

  var $ = function (id) { return document.getElementById(id); };
  var esc = function (t) {
    var d = document.createElement('div');
    d.textContent = t;
    return d.innerHTML;
  };

  var state = {
    entries: [],
    y: 0,
    m: 0,
    editingId: null,
    lastCharts: null,
    hover: { donut: -1, bar: -1, line: -1 }
  };

  function currentMonth() {
    var d = new Date();
    return { y: d.getFullYear(), m: d.getMonth() + 1 };
  }

  function monthPrefix() {
    return Core.monthPrefix(state.y, state.m);
  }

  function load() {
    try {
      var raw = (typeof localStorage !== 'undefined') ? localStorage.getItem(STORAGE_KEY) : null;
      if (!raw) return;
      var data = JSON.parse(raw);
      if (data && Array.isArray(data.entries)) {
        state.entries = data.entries.filter(Core.isValidEntry);
      }
    } catch (e) { /* 破損データは無視 */ }
  }

  function save() {
    try {
      if (typeof localStorage === 'undefined') return;
      localStorage.setItem(STORAGE_KEY, JSON.stringify({ version: 1, entries: state.entries }));
    } catch (e) {
      alert('保存に失敗しました（ストレージが満杯、または利用不可です）');
    }
  }

  function newId() {
    return 'e-' + Date.now().toString(36) + '-' + Math.random().toString(36).slice(2, 8);
  }

  /* ---- フォーム ---- */

  function fillCategories() {
    var type = $('f-type').value;
    var sel = $('f-category');
    var prev = sel.value;
    var cats = Core.CATEGORIES[type] || [];
    sel.innerHTML = '';
    for (var i = 0; i < cats.length; i++) {
      var opt = document.createElement('option');
      opt.value = cats[i];
      opt.textContent = cats[i];
      sel.appendChild(opt);
    }
    if (cats.indexOf(prev) !== -1) sel.value = prev;
  }

  function resetForm() {
    state.editingId = null;
    $('f-submit').textContent = '追加する';
    $('f-cancel').style.display = 'none';
    $('f-date').value = Core.todayStr();
    $('f-type').value = 'expense';
    fillCategories();
    $('f-amount').value = '';
    $('f-memo').value = '';
  }

  function onFormCancel() {
    resetForm();
  }

  function onFormSubmit(ev) {
    ev.preventDefault();
    var date = $('f-date').value;
    var type = $('f-type').value;
    var category = $('f-category').value;
    var amount = parseInt($('f-amount').value, 10);
    var memo = $('f-memo').value.trim();

    if (!/^\d{4}-\d{2}-\d{2}$/.test(date)) { alert('日付を入力してください'); return; }
    if (!category) { alert('カテゴリを選択してください'); return; }
    if (!isFinite(amount) || amount <= 0) { alert('金額は 1 円以上の数値で入力してください'); return; }

    if (state.editingId) {
      for (var i = 0; i < state.entries.length; i++) {
        if (state.entries[i].id === state.editingId) {
          state.entries[i].date = date;
          state.entries[i].type = type;
          state.entries[i].category = category;
          state.entries[i].amount = amount;
          state.entries[i].memo = memo;
          break;
        }
      }
      state.editingId = null;
      $('f-submit').textContent = '追加する';
      $('f-cancel').style.display = 'none';
    } else {
      state.entries.push({ id: newId(), date: date, type: type, category: category, amount: amount, memo: memo });
      // 追加したエントリの月に切り替えて即表示する
      state.y = parseInt(date.slice(0, 4), 10);
      state.m = parseInt(date.slice(5, 7), 10);
    }
    $('f-amount').value = '';
    $('f-memo').value = '';
    save();
    renderAll();
  }

  function startEdit(id) {
    var e = null;
    for (var i = 0; i < state.entries.length; i++) {
      if (state.entries[i].id === id) { e = state.entries[i]; break; }
    }
    if (!e) return;
    state.editingId = id;
    $('f-date').value = e.date;
    $('f-type').value = e.type;
    fillCategories();
    $('f-category').value = e.category;
    $('f-amount').value = String(e.amount);
    $('f-memo').value = e.memo;
    $('f-submit').textContent = '更新する';
    $('f-cancel').style.display = '';
    $('entry-form').scrollIntoView({ behavior: 'smooth', block: 'center' });
  }

  /* ---- 表 ---- */

  function onTableAction(ev) {
    var t = ev.target;
    var btn = (t && t.closest) ? t.closest('button[data-action]') : null;
    if (!btn) return;
    var id = btn.getAttribute('data-id');
    var action = btn.getAttribute('data-action');
    if (action === 'edit') {
      startEdit(id);
    } else if (action === 'delete') {
      for (var i = 0; i < state.entries.length; i++) {
        if (state.entries[i].id === id) {
          state.entries.splice(i, 1);
          break;
        }
      }
      if (state.editingId === id) onFormCancel();
      save();
      renderAll();
    }
  }

  function renderTable() {
    var host = $('entry-tbody');
    host.innerHTML = '';
    var prefix = monthPrefix();
    var rows = state.entries.filter(function (e) { return e.date.indexOf(prefix) === 0; });
    rows.sort(function (a, b) {
      if (a.date !== b.date) return (a.date < b.date) ? 1 : -1;
      return (a.id < b.id) ? 1 : -1;
    });

    if (rows.length === 0) {
      var tr0 = document.createElement('tr');
      var td0 = document.createElement('td');
      td0.colSpan = 6;
      td0.className = 'kb-empty';
      td0.textContent = 'この月にはエントリがありません';
      tr0.appendChild(td0);
      host.appendChild(tr0);
    }

    var totalIn = 0;
    var totalEx = 0;
    for (var i = 0; i < rows.length; i++) {
      var e = rows[i];
      var tr = document.createElement('tr');
      if (e.id === state.editingId) tr.className = 'kb-row-editing';

      var tdDate = document.createElement('td');
      tdDate.textContent = e.date;

      var tdType = document.createElement('td');
      var badge = document.createElement('span');
      badge.className = 'kb-badge ' + (e.type === 'income' ? 'kb-badge-income' : 'kb-badge-expense');
      badge.textContent = (e.type === 'income') ? '収入' : '支出';
      tdType.appendChild(badge);

      var tdCat = document.createElement('td');
      tdCat.textContent = e.category;

      var tdAmt = document.createElement('td');
      tdAmt.className = 'num ' + (e.type === 'income' ? 'pos' : 'neg');
      tdAmt.textContent = (e.type === 'income' ? '+' : '-') + Core.fmtMoney(e.amount);

      var tdMemo = document.createElement('td');
      tdMemo.className = 'memo';
      tdMemo.textContent = e.memo;

      var tdAct = document.createElement('td');
      tdAct.className = 'actions';
      var bEdit = document.createElement('button');
      bEdit.className = 'kb-btn kb-btn-sm';
      bEdit.type = 'button';
      bEdit.textContent = '編集';
      bEdit.setAttribute('data-action', 'edit');
      bEdit.setAttribute('data-id', e.id);
      var bDel = document.createElement('button');
      bDel.className = 'kb-btn kb-btn-sm kb-btn-danger';
      bDel.type = 'button';
      bDel.textContent = '削除';
      bDel.setAttribute('data-action', 'delete');
      bDel.setAttribute('data-id', e.id);
      tdAct.appendChild(bEdit);
      tdAct.appendChild(bDel);

      tr.appendChild(tdDate);
      tr.appendChild(tdType);
      tr.appendChild(tdCat);
      tr.appendChild(tdAmt);
      tr.appendChild(tdMemo);
      tr.appendChild(tdAct);
      host.appendChild(tr);

      if (e.type === 'income') totalIn += e.amount;
      else totalEx += e.amount;
    }
    $('table-foot-income').textContent = Core.fmtMoney(totalIn);
    $('table-foot-expense').textContent = Core.fmtMoney(totalEx);
  }

  /* ---- サマリー ---- */

  function renderSummary() {
    var t = Core.monthTotals(state.entries, monthPrefix());
    var count = 0;
    var prefix = monthPrefix();
    for (var i = 0; i < state.entries.length; i++) {
      if (state.entries[i].date.indexOf(prefix) === 0) count++;
    }
    $('sum-income').textContent = Core.fmtMoney(t.income);
    $('sum-expense').textContent = Core.fmtMoney(t.expense);
    var bal = $('sum-balance');
    bal.textContent = Core.fmtMoney(t.balance);
    bal.className = 'value ' + (t.balance >= 0 ? 'pos' : 'neg');
    $('sum-count').textContent = String(count);
  }

  /* ---- チャート ---- */

  function renderCharts() {
    var prefix = monthPrefix();

    // ドーナツ: 今月の支出カテゴリ内訳
    var monthEntries = state.entries.filter(function (e) { return e.date.indexOf(prefix) === 0; });
    var catList = Core.byCategory(monthEntries, 'expense');
    var donutItems = catList.map(function (c, i) {
      return { label: c.category, value: c.value, color: KeikibuChart.PALETTE[i % KeikibuChart.PALETTE.length] };
    });

    // 棒: 今月の日別支出
    var daily = Core.dailyTotals(state.entries, state.y, state.m);
    var barItems = [];
    for (var d = 0; d < daily.length; d++) {
      barItems.push({ label: String(d + 1), value: daily[d] });
    }

    // ライン: 直近 6 ヶ月
    var months = Core.lastMonths(state.y, state.m, TREND_MONTHS);
    var lineLabels = Core.trendLabels(months);
    var lineSeries = [
      { label: '収入', color: '#34d399', values: Core.monthSeries(state.entries, months, 'income') },
      { label: '支出', color: '#f472b6', values: Core.monthSeries(state.entries, months, 'expense') }
    ];

    state.lastCharts = {
      donutItems: donutItems,
      barItems: barItems,
      lineLabels: lineLabels,
      lineSeries: lineSeries,
      months: months
    };

    redrawChart('donut');
    redrawChart('bar');
    redrawChart('line');
    renderDonutLegend(catList);
  }

  function redrawChart(name) {
    var c = state.lastCharts;
    if (!c) return;
    if (name === 'donut') {
      KeikibuChart.drawDonut($('chart-donut'), c.donutItems, { highlight: state.hover.donut, centerLabel: '今月支出' });
    } else if (name === 'bar') {
      KeikibuChart.drawBar($('chart-bar'), c.barItems, { color: '#38bdf8', highlight: state.hover.bar });
    } else if (name === 'line') {
      KeikibuChart.drawLine($('chart-line'), c.lineLabels, c.lineSeries, { highlight: state.hover.line });
    }
  }

  function renderDonutLegend(catList) {
    var host = $('donut-legend');
    host.innerHTML = '';
    var total = 0;
    var i;
    for (i = 0; i < catList.length; i++) total += catList[i].value;
    if (total === 0) {
      var empty = document.createElement('span');
      empty.className = 'kb-empty-inline';
      empty.textContent = '今月の支出データはありません';
      host.appendChild(empty);
      return;
    }
    for (i = 0; i < catList.length; i++) {
      var c = catList[i];
      var item = document.createElement('span');
      item.className = 'kb-legend-item';
      var dot = document.createElement('span');
      dot.className = 'dot';
      dot.style.background = KeikibuChart.PALETTE[i % KeikibuChart.PALETTE.length];
      var label = document.createElement('span');
      label.textContent = c.category + ' ' + Core.fmtMoney(c.value) + '（' + Math.round(c.value / total * 100) + '%）';
      item.appendChild(dot);
      item.appendChild(label);
      host.appendChild(item);
    }
  }

  function setupHovers() {
    var tip = $('kb-tooltip');

    KeikibuChart.attachHover($('chart-donut'), tip, {
      hit: function (mx, my) {
        var hit = KeikibuChart.hitDonut($('chart-donut'), mx, my);
        if (!hit) return null;
        hit.html = '<b>' + esc(hit.label) + '</b><br>' + Core.fmtMoney(hit.value) + '（' + Math.round(hit.pct * 100) + '%）';
        return hit;
      },
      onHover: function (index) {
        state.hover.donut = (index == null) ? -1 : index;
        redrawChart('donut');
      }
    });

    KeikibuChart.attachHover($('chart-bar'), tip, {
      hit: function (mx, my) {
        var hit = KeikibuChart.hitBar($('chart-bar'), mx, my);
        if (!hit) return null;
        hit.html = '<b>' + esc(hit.label) + '日</b><br>支出 ' + Core.fmtMoney(hit.value);
        return hit;
      },
      onHover: function (index) {
        state.hover.bar = (index == null) ? -1 : index;
        redrawChart('bar');
      }
    });

    KeikibuChart.attachHover($('chart-line'), tip, {
      hit: function (mx, my) {
        var hit = KeikibuChart.hitLine($('chart-line'), mx, my);
        if (!hit) return null;
        var c = state.lastCharts;
        if (!c) return null;
        var mo = c.months[hit.index];
        var i = hit.index;
        hit.html = '<b>' + Core.monthLabel(mo.y, mo.m) + '</b><br>' +
          '<span style="color:#34d399">&#9632; 収入 ' + Core.fmtMoney(c.lineSeries[0].values[i]) + '</span><br>' +
          '<span style="color:#f472b6">&#9632; 支出 ' + Core.fmtMoney(c.lineSeries[1].values[i]) + '</span>';
        return hit;
      },
      onHover: function (index) {
        state.hover.line = (index == null) ? -1 : index;
        redrawChart('line');
      }
    });
  }

  /* ---- 月切替 ---- */

  function shiftMonth(delta) {
    var d = new Date(state.y, state.m - 1 + delta, 1);
    state.y = d.getFullYear();
    state.m = d.getMonth() + 1;
    state.hover = { donut: -1, bar: -1, line: -1 };
    renderAll();
  }

  /* ---- データ操作 ---- */

  function seedSampleData() {
    if (state.entries.length > 0 && !confirm('現在のデータをサンプルデータに置き換えます。よろしいですか？')) return;
    state.entries = Core.makeSampleEntries();
    var cm = currentMonth();
    state.y = cm.y;
    state.m = cm.m;
    save();
    renderAll();
  }

  function exportJson() {
    var data = JSON.stringify({ app: 'keikibu', version: 1, exported_at: Core.todayStr(), entries: state.entries }, null, 2);
    var blob = new Blob([data], { type: 'application/json' });
    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = 'keikibu_export_' + Core.todayStr().replace(/-/g, '') + '.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
  }

  function importJson(file) {
    var reader = new FileReader();
    reader.onload = function () {
      try {
        var data = JSON.parse(reader.result);
        var list = Array.isArray(data) ? data : (data && Array.isArray(data.entries) ? data.entries : null);
        if (!list) { alert('形式が不正です。エクスポートした JSON を選択してください。'); return; }
        var valid = list.filter(Core.isValidEntry);
        if (valid.length === 0) { alert('有効なエントリが 1 件もありません。'); return; }
        var skipped = list.length - valid.length;
        var msg = '有効なエントリ: ' + valid.length + ' 件' +
          (skipped > 0 ? '（無効: ' + skipped + ' 件はスキップ）' : '');
        if (!confirm(msg + '\n現在のデータを置き換えます。よろしいですか？')) return;
        state.entries = valid;
        save();
        renderAll();
      } catch (e) {
        alert('JSON の読み込みに失敗しました。');
      }
    };
    reader.readAsText(file, 'utf-8');
  }

  function clearAll() {
    if (!confirm('全エントリを削除します。よろしいですか？')) return;
    state.entries = [];
    resetForm();
    save();
    renderAll();
  }

  /* ---- 全体レンダリング ---- */

  function renderMonthLabel() {
    var label = Core.monthLabel(state.y, state.m);
    $('month-label').textContent = label;
    $('table-month-label').textContent = label;
  }

  function renderAll() {
    renderMonthLabel();
    renderSummary();
    renderCharts();
    renderTable();
  }

  var resizeTimer = null;
  function onResize() {
    if (resizeTimer) clearTimeout(resizeTimer);
    resizeTimer = setTimeout(function () {
      if (state.lastCharts) {
        redrawChart('donut');
        redrawChart('bar');
        redrawChart('line');
      }
    }, 150);
  }

  function init() {
    var cm = currentMonth();
    state.y = cm.y;
    state.m = cm.m;
    load();

    resetForm();

    $('f-type').addEventListener('change', fillCategories);
    $('entry-form').addEventListener('submit', onFormSubmit);
    $('f-cancel').addEventListener('click', onFormCancel);
    $('month-prev').addEventListener('click', function () { shiftMonth(-1); });
    $('month-next').addEventListener('click', function () { shiftMonth(1); });
    $('month-today').addEventListener('click', function () {
      var c = currentMonth();
      state.y = c.y;
      state.m = c.m;
      state.hover = { donut: -1, bar: -1, line: -1 };
      renderAll();
    });
    $('btn-sample').addEventListener('click', seedSampleData);
    $('btn-export').addEventListener('click', exportJson);
    $('btn-import').addEventListener('click', function () { $('import-file').click(); });
    $('import-file').addEventListener('change', function (ev) {
      var f = ev.target.files && ev.target.files[0];
      if (f) importJson(f);
      ev.target.value = '';
    });
    $('btn-clear').addEventListener('click', clearAll);
    $('entry-tbody').addEventListener('click', onTableAction);
    setupHovers();
    window.addEventListener('resize', onResize);

    renderAll();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})(typeof window !== 'undefined' ? window : this);
