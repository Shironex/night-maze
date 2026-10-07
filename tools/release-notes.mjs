// Prints the text of a GitHub release: the section of one version from
// CHANGELOG.md, without its `## x.y.z (date)` heading, followed by the block
// in .github/release-footer.md.
//
//   node tools/release-notes.mjs --version 0.10.0 [--changelog CHANGELOG.md] \
//     [--footer .github/release-footer.md] > release-notes.md
//
// A version without a section is an error (exit code 1): a release must not go
// out with empty notes. Node builtins only.

import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');

/** `## 0.10.0 (2026-10-07)`, also written as `## [0.10.0]` or `## v0.10.0`. */
const VERSION_HEADING = /^##\s+\[?v?(\d+\.\d+\.\d+)(?![\d.])/;

/**
 * The notes of `version`: its changelog section and the footer, as Markdown
 * with LF line endings and one newline at the end. Throws when the changelog
 * has no section for the version.
 */
export function releaseNotes(changelog, version, footer) {
  // A checkout on Windows may hold CRLF, the release text never does.
  const lines = changelog.replace(/\r\n/g, '\n').split('\n');
  const start = lines.findIndex(line => VERSION_HEADING.exec(line)?.[1] === version);
  if (start === -1) throw new Error(`CHANGELOG.md has no section for version ${version}`);
  // The section ends at the next heading of the same level ("### " is part of it).
  let end = lines.findIndex((line, index) => index > start && /^##\s/.test(line));
  if (end === -1) end = lines.length;
  const section = lines.slice(start + 1, end).join('\n').trim();
  if (!section) throw new Error(`the section of version ${version} in CHANGELOG.md is empty`);
  return `${section}\n\n${footer.replace(/\r\n/g, '\n').trim()}\n`;
}

function argument(name, fallback) {
  const index = process.argv.indexOf(`--${name}`);
  return index === -1 ? fallback : process.argv[index + 1];
}

// Only when started as a program, not when the test imports the function.
if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const version = argument('version');
  if (!version) {
    console.error('usage: release-notes.mjs --version <x.y.z> [--changelog <file>] [--footer <file>]');
    process.exit(2);
  }
  try {
    const changelog = readFileSync(argument('changelog', join(repoRoot, 'CHANGELOG.md')), 'utf8');
    const footer = readFileSync(
      argument('footer', join(repoRoot, '.github', 'release-footer.md')),
      'utf8'
    );
    process.stdout.write(releaseNotes(changelog, version, footer));
  } catch (error) {
    console.error(error.message);
    process.exit(1);
  }
}
