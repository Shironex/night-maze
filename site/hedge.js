/* Tonight's hedge, as a sketch: a maze made in the browser from the local date.
   It is not the generator of the game, so the maze differs from the one the game
   makes for the same seed. The rules it borrows are real: 16 x 16 cells, 26 crystals,
   a gate that opens at 19 and stands in the cell farthest from the start.
   Self check: node hedge.js */
(function () {
  'use strict';

  var N = 1, E = 2, S = 4, W = 8;
  var DX = { 1: 0, 2: 1, 4: 0, 8: -1 };
  var DY = { 1: -1, 2: 0, 4: 1, 8: 0 };
  var BACK = { 1: S, 2: W, 4: N, 8: E };
  var SIDES = [N, E, S, W];

  function random(seed) {
    return function () {
      seed = (seed + 0x6d2b79f5) | 0;
      var t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  function shuffle(list, rand) {
    for (var i = list.length - 1; i > 0; i--) {
      var j = Math.floor(rand() * (i + 1));
      var t = list[i]; list[i] = list[j]; list[j] = t;
    }
    return list;
  }

  /* open[cell] is a bit mask of the sides without a wall. */
  function buildMaze(seed, n, crystalCount) {
    var rand = random(seed);
    var open = new Uint8Array(n * n);
    var seen = new Uint8Array(n * n);
    var stack = [0];
    seen[0] = 1;
    while (stack.length) {
      var cell = stack[stack.length - 1];
      var x = cell % n, y = (cell - x) / n;
      var ways = [];
      for (var k = 0; k < 4; k++) {
        var d = SIDES[k], nx = x + DX[d], ny = y + DY[d];
        if (nx >= 0 && ny >= 0 && nx < n && ny < n && !seen[ny * n + nx]) ways.push(d);
      }
      if (!ways.length) { stack.pop(); continue; }
      var dir = ways[Math.floor(rand() * ways.length)];
      var next = (y + DY[dir]) * n + x + DX[dir];
      open[cell] |= dir;
      open[next] |= BACK[dir];
      seen[next] = 1;
      stack.push(next);
    }

    /* Steps from the start to every cell. The gate is the farthest one. */
    var steps = new Int16Array(n * n).fill(-1);
    var queue = [0], exit = 0;
    steps[0] = 0;
    for (var q = 0; q < queue.length; q++) {
      var c = queue[q];
      if (steps[c] > steps[exit]) exit = c;
      for (var s = 0; s < 4; s++) {
        var side = SIDES[s];
        if (!(open[c] & side)) continue;
        var to = c + DX[side] + DY[side] * n;
        if (steps[to] < 0) { steps[to] = steps[c] + 1; queue.push(to); }
      }
    }

    /* Crystals: dead ends first, then any other cell. */
    var deadEnds = [], others = [];
    for (var i = 1; i < n * n; i++) {
      if (i === exit) continue;
      var m = open[i];
      (m === N || m === E || m === S || m === W ? deadEnds : others).push(i);
    }
    var crystals = shuffle(deadEnds, rand).concat(shuffle(others, rand)).slice(0, crystalCount);

    return { n: n, open: open, start: 0, exit: exit, steps: steps, crystals: crystals };
  }

  /* The cells the lamp lights from a cell: the cell and each straight run to a wall. */
  function litFrom(maze, cell) {
    var lit = [cell];
    for (var k = 0; k < 4; k++) {
      var d = SIDES[k], c = cell;
      while (maze.open[c] & d) { c += DX[d] + DY[d] * maze.n; lit.push(c); }
    }
    return lit;
  }

  function dateSeed(date) {
    return date.getFullYear() * 10000 + (date.getMonth() + 1) * 100 + date.getDate();
  }

  if (typeof module !== 'undefined' && module.exports) {
    if (require.main === module) {
      var assert = require('assert');
      [20261009, 20261010, 1, 99991231].forEach(function (seed) {
        var maze = buildMaze(seed, 16, 26);
        assert(Array.prototype.every.call(maze.steps, function (v) { return v >= 0; }), 'every cell is reached');
        assert.strictEqual(Math.max.apply(null, maze.steps), maze.steps[maze.exit], 'the gate is the farthest cell');
        assert.strictEqual(new Set(maze.crystals).size, 26, '26 crystals in 26 cells');
        assert(maze.crystals.indexOf(maze.exit) < 0 && maze.crystals.indexOf(0) < 0, 'none at the start or the gate');
        assert.deepStrictEqual(buildMaze(seed, 16, 26).open, maze.open, 'the same seed gives the same maze');
        assert(litFrom(maze, 0).length > 1, 'the lamp lights a corridor');
      });
      assert.strictEqual(dateSeed(new Date(2026, 9, 8)), 20261008);
      console.log('hedge: ok');
    }
    return;
  }

  /* ---- the page ---------------------------------------------------------------- */

  var canvas = document.getElementById('hedge');
  if (!canvas || !canvas.getContext) return;

  var SIZE = 16, CRYSTALS = 26, NEEDED = 19;
  var MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August',
    'September', 'October', 'November', 'December'];
  var COLOR = { moon: '#e6ebf5', dim: '#3a4868', amber: '#ffb854', teal: '#56d6ca', cold: '#7f9be6' };

  var today = new Date();
  var seed = dateSeed(today);
  var maze = buildMaze(seed, SIZE, CRYSTALS);
  var ctx = canvas.getContext('2d');
  var status = document.getElementById('hedge-status');
  var count = document.getElementById('hedge-count');
  var moonButton = document.getElementById('hedge-moon');
  var lamp, known, lit, left, held, moon = false, began, done;

  document.getElementById('hedge-date').textContent =
    today.getDate() + ' ' + MONTHS[today.getMonth()] + ' ' + today.getFullYear();
  document.getElementById('hedge-seed').textContent = String(seed);

  function begin() {
    lamp = maze.start;
    known = new Uint8Array(SIZE * SIZE);
    left = maze.crystals.slice();
    held = 0; began = 0; done = false;
    look();
    say('The lamp is at the stile. Walk it into the hedge.');
  }

  function look() {
    lit = litFrom(maze, lamp);
    lit.forEach(function (c) { known[c] = 1; });
    count.textContent = held + ' / ' + NEEDED;
    draw();
  }

  function say(text) { status.textContent = text; }

  function clock(ms) {
    var s = Math.round(ms / 1000);
    return Math.floor(s / 60) + ':' + (s % 60 < 10 ? '0' : '') + (s % 60);
  }

  /* Walks the lamp to a lit cell, through every cell on the way. */
  function walk(target) {
    if (done || target === lamp || lit.indexOf(target) < 0) return;
    if (!began) began = Date.now();
    var step = target > lamp ? (target - lamp >= SIZE ? SIZE : 1) : (lamp - target >= SIZE ? -SIZE : -1);
    var before = held;
    while (lamp !== target) {
      lamp += step;
      var at = left.indexOf(lamp);
      if (at >= 0) { left.splice(at, 1); held++; }
    }
    if (lamp === maze.exit && held >= NEEDED) {
      done = true;
      say('Through the gate. ' + clock(Date.now() - began) + ', ' + held + ' of ' + CRYSTALS + ' crystals, seed ' + seed + '.');
    } else if (held >= NEEDED && before < NEEDED) {
      say('The gate is open. Find the exit.');
    } else if (lamp === maze.exit) {
      say('The gate is shut. It opens at ' + NEEDED + ' crystals.');
    } else if (held > before && held < NEEDED) {
      say(held + ' of the ' + NEEDED + ' the gate asks for.');
    }
    look();
  }

  function draw() {
    var box = canvas.getBoundingClientRect();
    var ratio = Math.min(window.devicePixelRatio || 1, 2);
    var px = Math.max(160, Math.round(box.width));
    if (canvas.width !== px * ratio) { canvas.width = canvas.height = px * ratio; }
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    ctx.clearRect(0, 0, px, px);
    var pad = 6, u = (px - pad * 2) / SIZE;
    var lx = lamp % SIZE, ly = (lamp - lx) / SIZE;

    function corner(c) { var x = c % SIZE; return [pad + x * u, pad + ((c - x) / SIZE) * u]; }

    /* The dark is not empty: a faint dot marks every cell still to be seen. */
    ctx.fillStyle = 'rgba(140,154,182,0.2)';
    for (var c = 0; c < SIZE * SIZE; c++) {
      if (known[c]) continue;
      ctx.fillRect(pad + (c % SIZE + 0.5) * u - 1, pad + (Math.floor(c / SIZE) + 0.5) * u - 1, 2, 2);
    }

    for (c = 0; c < SIZE * SIZE; c++) {
      if (!known[c] && !moon) continue;
      var p = corner(c), isLit = lit.indexOf(c) >= 0;
      if (isLit) {
        var far = Math.abs(c % SIZE - lx) + Math.abs(Math.floor(c / SIZE) - ly);
        ctx.fillStyle = 'rgba(255,184,84,' + Math.max(0.07, 0.3 - far * 0.035) + ')';
      } else {
        ctx.fillStyle = known[c] ? 'rgba(48,62,94,0.42)' : 'rgba(48,62,94,0.14)';
      }
      ctx.fillRect(p[0], p[1], u + 0.5, u + 0.5);
    }

    ctx.lineCap = 'square';
    ctx.lineWidth = Math.max(1.5, u * 0.09);
    for (var pass = 0; pass < 2; pass++) {
      for (c = 0; c < SIZE * SIZE; c++) {
        var cellLit = lit.indexOf(c) >= 0;
        if ((pass === 1) !== cellLit) continue;
        if (!known[c] && !moon) continue;
        p = corner(c);
        ctx.strokeStyle = cellLit ? COLOR.moon : known[c] ? COLOR.dim : 'rgba(140,154,182,0.3)';
        ctx.beginPath();
        if (!(maze.open[c] & N)) { ctx.moveTo(p[0], p[1]); ctx.lineTo(p[0] + u, p[1]); }
        if (!(maze.open[c] & E)) { ctx.moveTo(p[0] + u, p[1]); ctx.lineTo(p[0] + u, p[1] + u); }
        if (!(maze.open[c] & S)) { ctx.moveTo(p[0], p[1] + u); ctx.lineTo(p[0] + u, p[1] + u); }
        if (!(maze.open[c] & W)) { ctx.moveTo(p[0], p[1]); ctx.lineTo(p[0], p[1] + u); }
        ctx.stroke();
      }
    }

    /* The gate: a lantern in the exit cell, cold while shut and warm when open. */
    if (known[maze.exit] || moon) {
      p = corner(maze.exit);
      var warm = held >= NEEDED;
      ctx.fillStyle = warm ? COLOR.amber : COLOR.cold;
      ctx.shadowColor = ctx.fillStyle;
      ctx.shadowBlur = warm ? u * 0.8 : u * 0.3;
      ctx.fillRect(p[0] + u * 0.34, p[1] + u * 0.26, u * 0.32, u * 0.48);
      ctx.shadowBlur = 0;
    }

    /* The crystals: a square on its corner, like the gem of the menu. */
    left.forEach(function (cell) {
      if (!known[cell] && !moon) return;
      var q = corner(cell), bright = lit.indexOf(cell) >= 0;
      ctx.save();
      ctx.translate(q[0] + u / 2, q[1] + u / 2);
      ctx.scale(0.72, 1.1);
      ctx.rotate(Math.PI / 4);
      ctx.globalAlpha = bright ? 1 : known[cell] ? 0.55 : 0.28;
      ctx.fillStyle = COLOR.teal;
      if (bright) { ctx.shadowColor = COLOR.teal; ctx.shadowBlur = u * 0.6; }
      ctx.fillRect(-u * 0.14, -u * 0.14, u * 0.28, u * 0.28);
      ctx.restore();
    });

    /* The lamp: the ring of the HUD. */
    var l = corner(lamp), cx = l[0] + u / 2, cy = l[1] + u / 2;
    var glow = ctx.createRadialGradient(cx, cy, 0, cx, cy, u * 1.6);
    glow.addColorStop(0, 'rgba(255,184,84,0.38)');
    glow.addColorStop(1, 'rgba(255,184,84,0)');
    ctx.fillStyle = glow;
    ctx.fillRect(cx - u * 1.6, cy - u * 1.6, u * 3.2, u * 3.2);
    ctx.strokeStyle = COLOR.amber;
    ctx.lineWidth = Math.max(1.5, u * 0.1);
    ctx.beginPath(); ctx.arc(cx, cy, u * 0.27, 0, Math.PI * 2); ctx.stroke();
    ctx.fillStyle = COLOR.amber;
    ctx.beginPath(); ctx.arc(cx, cy, u * 0.08, 0, Math.PI * 2); ctx.fill();
  }

  function cellAt(event) {
    var box = canvas.getBoundingClientRect();
    var u = (box.width - 12) / SIZE;
    var x = Math.floor((event.clientX - box.left - 6) / u);
    var y = Math.floor((event.clientY - box.top - 6) / u);
    return x < 0 || y < 0 || x >= SIZE || y >= SIZE ? -1 : y * SIZE + x;
  }

  function point(event) { var cell = cellAt(event); if (cell >= 0) walk(cell); }
  canvas.addEventListener('pointermove', point);
  canvas.addEventListener('pointerdown', point);

  canvas.addEventListener('keydown', function (event) {
    var side = { ArrowUp: N, ArrowRight: E, ArrowDown: S, ArrowLeft: W }[event.key];
    if (!side) return;
    event.preventDefault();
    if (maze.open[lamp] & side) walk(lamp + DX[side] + DY[side] * SIZE);
  });

  moonButton.addEventListener('click', function () {
    moon = !moon;
    moonButton.setAttribute('aria-pressed', String(moon));
    draw();
  });
  document.getElementById('hedge-again').addEventListener('click', begin);

  if ('ResizeObserver' in window) new ResizeObserver(draw).observe(canvas);
  begin();
})();
