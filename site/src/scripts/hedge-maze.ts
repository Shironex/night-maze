/* The maze of tonight's hedge: a maze made from the local date, with no page in it.
   It is not the generator of the game, so the maze differs from the one the game
   makes for the same seed. The rules it borrows are real: 16 x 16 cells, 26 crystals,
   a gate that opens at 19 and stands in the cell farthest from the start.
   Self check: pnpm test */

export const N = 1, E = 2, S = 4, W = 8;
export const DX: Record<number, number> = { 1: 0, 2: 1, 4: 0, 8: -1 };
export const DY: Record<number, number> = { 1: -1, 2: 0, 4: 1, 8: 0 };
const BACK: Record<number, number> = { 1: S, 2: W, 4: N, 8: E };
const SIDES = [N, E, S, W];

export type Maze = {
  n: number;
  open: Uint8Array;
  start: number;
  exit: number;
  steps: Int16Array;
  crystals: number[];
};

function random(seed: number) {
  return function () {
    seed = (seed + 0x6d2b79f5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function shuffle(list: number[], rand: () => number) {
  for (let i = list.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    const t = list[i]; list[i] = list[j]; list[j] = t;
  }
  return list;
}

/* open[cell] is a bit mask of the sides without a wall. */
export function buildMaze(seed: number, n: number, crystalCount: number): Maze {
  const rand = random(seed);
  const open = new Uint8Array(n * n);
  const seen = new Uint8Array(n * n);
  const stack = [0];
  seen[0] = 1;
  while (stack.length) {
    const cell = stack[stack.length - 1];
    const x = cell % n, y = (cell - x) / n;
    const ways: number[] = [];
    for (let k = 0; k < 4; k++) {
      const d = SIDES[k], nx = x + DX[d], ny = y + DY[d];
      if (nx >= 0 && ny >= 0 && nx < n && ny < n && !seen[ny * n + nx]) ways.push(d);
    }
    if (!ways.length) { stack.pop(); continue; }
    const dir = ways[Math.floor(rand() * ways.length)];
    const next = (y + DY[dir]) * n + x + DX[dir];
    open[cell] |= dir;
    open[next] |= BACK[dir];
    seen[next] = 1;
    stack.push(next);
  }

  /* Steps from the start to every cell. The gate is the farthest one. */
  const steps = new Int16Array(n * n).fill(-1);
  const queue = [0];
  let exit = 0;
  steps[0] = 0;
  for (let q = 0; q < queue.length; q++) {
    const c = queue[q];
    if (steps[c] > steps[exit]) exit = c;
    for (let s = 0; s < 4; s++) {
      const side = SIDES[s];
      if (!(open[c] & side)) continue;
      const to = c + DX[side] + DY[side] * n;
      if (steps[to] < 0) { steps[to] = steps[c] + 1; queue.push(to); }
    }
  }

  /* Crystals: dead ends first, then any other cell. */
  const deadEnds: number[] = [], others: number[] = [];
  for (let i = 1; i < n * n; i++) {
    if (i === exit) continue;
    const m = open[i];
    (m === N || m === E || m === S || m === W ? deadEnds : others).push(i);
  }
  const crystals = shuffle(deadEnds, rand).concat(shuffle(others, rand)).slice(0, crystalCount);

  return { n, open, start: 0, exit, steps, crystals };
}

/* The cells the lamp lights from a cell: the cell and each straight run to a wall. */
export function litFrom(maze: Maze, cell: number) {
  const lit = [cell];
  for (let k = 0; k < 4; k++) {
    const d = SIDES[k];
    let c = cell;
    while (maze.open[c] & d) { c += DX[d] + DY[d] * maze.n; lit.push(c); }
  }
  return lit;
}

export function dateSeed(date: Date) {
  return date.getFullYear() * 10000 + (date.getMonth() + 1) * 100 + date.getDate();
}
