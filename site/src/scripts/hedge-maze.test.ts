import { test } from 'node:test';
import assert from 'node:assert';
import { buildMaze, litFrom, dateSeed } from './hedge-maze.ts';

for (const seed of [20261009, 20261010, 1, 99991231]) {
  test(`maze of seed ${seed}`, () => {
    const maze = buildMaze(seed, 16, 26);
    assert(maze.steps.every((v) => v >= 0), 'every cell is reached');
    assert.strictEqual(Math.max(...maze.steps), maze.steps[maze.exit], 'the gate is the farthest cell');
    assert.strictEqual(new Set(maze.crystals).size, 26, '26 crystals in 26 cells');
    assert(!maze.crystals.includes(maze.exit) && !maze.crystals.includes(0), 'none at the start or the gate');
    assert.deepStrictEqual(buildMaze(seed, 16, 26).open, maze.open, 'the same seed gives the same maze');
    assert(litFrom(maze, 0).length > 1, 'the lamp lights a corridor');
  });
}

test('the seed is the date', () => {
  assert.strictEqual(dateSeed(new Date(2026, 9, 8)), 20261008);
});
