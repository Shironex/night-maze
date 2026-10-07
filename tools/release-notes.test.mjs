// node --test "tools/*.test.mjs"
//
// The expected text below is the body of the published release v0.10.0, copied
// from `gh release view v0.10.0 --json body`. The test proves that the notes
// generated from CHANGELOG.md are what is live. It fails when the 0.10.0
// section of the changelog or the footer is edited: then decide which of the
// two is right, do not just update the string.

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';
import { releaseNotes } from './release-notes.mjs';

const tools = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(tools, '..');
const script = join(tools, 'release-notes.mjs');

const PUBLISHED_0_10_0 = `The maze is no longer silent: the first sounds.

### What's new

- The flashlight clicks when you switch it on and off with F.
- When the battery is empty the switch only gives a dull click, and you hear the same click at the moment the light dies.
- A low battery now warns you with a slow pulse, like a heartbeat. It gets faster as the battery runs down, and it stops when a crystal charges the battery again.
- Picking up a crystal chimes.
- A lever clunks when you pull it, and the gate grinds open when you have collected enough crystals.
- A volume slider on the settings screen, from 0 to 100. It is saved with your other settings, and a short click lets you hear the level while you move it.

## How to play

Install and update through the Night Maze launcher: https://github.com/Shironex/night-maze-launcher/releases/latest

The files below are what the launcher downloads. Windows 10 or 11, 64 bit.

Night Maze is a university project that I keep working on while I learn OpenGL, so things will change a lot and break here and there.
`;

test('the notes of 0.10.0 are the published release text', () => {
  const result = spawnSync(process.execPath, [script, '--version', '0.10.0'], { encoding: 'utf8' });
  assert.equal(result.status, 0, result.stderr);
  assert.equal(result.stdout, PUBLISHED_0_10_0);
});

test('CRLF line endings and the last section of the file give the same kind of text', () => {
  const changelog = '# Changelog\r\n\r\n## 0.2.0 (2026-01-02)\r\n\r\nNew.\r\n\r\n## 0.1.0\r\n\r\nOld.\r\n';
  assert.equal(releaseNotes(changelog, '0.2.0', 'Footer\r\n'), 'New.\n\nFooter\n');
  assert.equal(releaseNotes(changelog, '0.1.0', 'Footer\n'), 'Old.\n\nFooter\n');
});

test('a version is matched whole, 0.1.0 is not 0.1.01 or 10.1.0', () => {
  const changelog = '## 0.1.01\n\nA\n\n## 10.1.0\n\nB\n';
  assert.throws(() => releaseNotes(changelog, '0.1.0', 'Footer'), /no section for version 0.1.0/);
});

test('a missing section is exit code 1', () => {
  const result = spawnSync(process.execPath, [script, '--version', '99.0.0'], { encoding: 'utf8' });
  assert.equal(result.status, 1);
  assert.match(result.stderr, /no section for version 99\.0\.0/);
  assert.equal(result.stdout, '');
});

test('the footer is the block the published release ends with', () => {
  const footer = readFileSync(join(repoRoot, '.github', 'release-footer.md'), 'utf8');
  assert.ok(PUBLISHED_0_10_0.endsWith(footer.replace(/\r\n/g, '\n')));
});
