/* Tonight's hedge, as a sketch: the page part. The maze itself is made in hedge-maze.ts. */
import { N, E, S, W, DX, DY, buildMaze, litFrom, dateSeed } from './hedge-maze.ts';

const canvas = document.getElementById('hedge') as HTMLCanvasElement | null;
const ctx = canvas && canvas.getContext ? canvas.getContext('2d') : null;
if (canvas && ctx) main(canvas, ctx);

function main(canvas: HTMLCanvasElement, ctx: CanvasRenderingContext2D) {
  const SIZE = 16, CRYSTALS = 26, NEEDED = 19;
  const MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August',
    'September', 'October', 'November', 'December'];
  const COLOR = { moon: '#e6ebf5', dim: '#3a4868', amber: '#ffb854', teal: '#56d6ca', cold: '#7f9be6' };

  const today = new Date();
  const seed = dateSeed(today);
  const maze = buildMaze(seed, SIZE, CRYSTALS);
  const status = document.getElementById('hedge-status') as HTMLElement;
  const count = document.getElementById('hedge-count') as HTMLElement;
  const moonButton = document.getElementById('hedge-moon') as HTMLElement;
  let lamp = 0, known = new Uint8Array(0), lit: number[] = [], left: number[] = [];
  let held = 0, moon = false, began = 0, done = false;

  (document.getElementById('hedge-date') as HTMLElement).textContent =
    today.getDate() + ' ' + MONTHS[today.getMonth()] + ' ' + today.getFullYear();
  (document.getElementById('hedge-seed') as HTMLElement).textContent = String(seed);

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

  function say(text: string) { status.textContent = text; }

  function clock(ms: number) {
    const s = Math.round(ms / 1000);
    return Math.floor(s / 60) + ':' + (s % 60 < 10 ? '0' : '') + (s % 60);
  }

  /* Walks the lamp to a lit cell, through every cell on the way. */
  function walk(target: number) {
    if (done || target === lamp || lit.indexOf(target) < 0) return;
    if (!began) began = Date.now();
    const step = target > lamp ? (target - lamp >= SIZE ? SIZE : 1) : (lamp - target >= SIZE ? -SIZE : -1);
    const before = held;
    while (lamp !== target) {
      lamp += step;
      const at = left.indexOf(lamp);
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
    const box = canvas.getBoundingClientRect();
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    const px = Math.max(160, Math.round(box.width));
    if (canvas.width !== px * ratio) { canvas.width = canvas.height = px * ratio; }
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    ctx.clearRect(0, 0, px, px);
    const pad = 6, u = (px - pad * 2) / SIZE;
    const lx = lamp % SIZE, ly = (lamp - lx) / SIZE;

    function corner(c: number) { const x = c % SIZE; return [pad + x * u, pad + ((c - x) / SIZE) * u]; }

    /* The dark is not empty: a faint dot marks every cell still to be seen. */
    ctx.fillStyle = 'rgba(140,154,182,0.2)';
    for (let c = 0; c < SIZE * SIZE; c++) {
      if (known[c]) continue;
      ctx.fillRect(pad + (c % SIZE + 0.5) * u - 1, pad + (Math.floor(c / SIZE) + 0.5) * u - 1, 2, 2);
    }

    for (let c = 0; c < SIZE * SIZE; c++) {
      if (!known[c] && !moon) continue;
      const p = corner(c), isLit = lit.indexOf(c) >= 0;
      if (isLit) {
        const far = Math.abs(c % SIZE - lx) + Math.abs(Math.floor(c / SIZE) - ly);
        ctx.fillStyle = 'rgba(255,184,84,' + Math.max(0.07, 0.3 - far * 0.035) + ')';
      } else {
        ctx.fillStyle = known[c] ? 'rgba(48,62,94,0.42)' : 'rgba(48,62,94,0.14)';
      }
      ctx.fillRect(p[0], p[1], u + 0.5, u + 0.5);
    }

    ctx.lineCap = 'square';
    ctx.lineWidth = Math.max(1.5, u * 0.09);
    for (let pass = 0; pass < 2; pass++) {
      for (let c = 0; c < SIZE * SIZE; c++) {
        const cellLit = lit.indexOf(c) >= 0;
        if ((pass === 1) !== cellLit) continue;
        if (!known[c] && !moon) continue;
        const p = corner(c);
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
      const p = corner(maze.exit);
      const warm = held >= NEEDED;
      ctx.fillStyle = warm ? COLOR.amber : COLOR.cold;
      ctx.shadowColor = ctx.fillStyle;
      ctx.shadowBlur = warm ? u * 0.8 : u * 0.3;
      ctx.fillRect(p[0] + u * 0.34, p[1] + u * 0.26, u * 0.32, u * 0.48);
      ctx.shadowBlur = 0;
    }

    /* The crystals: a square on its corner, like the gem of the menu. */
    left.forEach(function (cell) {
      if (!known[cell] && !moon) return;
      const q = corner(cell), bright = lit.indexOf(cell) >= 0;
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
    const l = corner(lamp), cx = l[0] + u / 2, cy = l[1] + u / 2;
    const glow = ctx.createRadialGradient(cx, cy, 0, cx, cy, u * 1.6);
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

  function cellAt(event: PointerEvent) {
    const box = canvas.getBoundingClientRect();
    const u = (box.width - 12) / SIZE;
    const x = Math.floor((event.clientX - box.left - 6) / u);
    const y = Math.floor((event.clientY - box.top - 6) / u);
    return x < 0 || y < 0 || x >= SIZE || y >= SIZE ? -1 : y * SIZE + x;
  }

  function point(event: PointerEvent) { const cell = cellAt(event); if (cell >= 0) walk(cell); }
  canvas.addEventListener('pointermove', point);
  canvas.addEventListener('pointerdown', point);

  canvas.addEventListener('keydown', function (event) {
    const side = ({ ArrowUp: N, ArrowRight: E, ArrowDown: S, ArrowLeft: W } as Record<string, number | undefined>)[event.key];
    if (!side) return;
    event.preventDefault();
    if (maze.open[lamp] & side) walk(lamp + DX[side] + DY[side] * SIZE);
  });

  moonButton.addEventListener('click', function () {
    moon = !moon;
    moonButton.setAttribute('aria-pressed', String(moon));
    draw();
  });
  (document.getElementById('hedge-again') as HTMLElement).addEventListener('click', begin);

  if ('ResizeObserver' in window) new ResizeObserver(draw).observe(canvas);
  begin();
}
