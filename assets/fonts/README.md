# Fonts

Third-party material. The program reads the font at start (`src/debug/Theme.cpp`,
function `loadFont`) and draws the text of the debug panels with it.

| File | What it is |
|---|---|
| `AtkinsonHyperlegible-Regular.ttf` | Atkinson Hyperlegible, regular weight, a static TrueType file of 54 348 bytes. Not modified |
| `OFL.txt` | the licence of the font: SIL Open Font License, Version 1.1. Copied unchanged from the same source directory. It must stay next to the font file |

## Source

- Font: Atkinson Hyperlegible, Copyright 2020 Braille Institute of America, Inc.
- Version: 1.006 (the version string inside the file is `Version 1.006; ttfautohint (v1.8.3)`).
- Downloaded on 2026-10-05 from the Google Fonts repository:
  <https://github.com/google/fonts/tree/main/ofl/atkinsonhyperlegible>
  (files `AtkinsonHyperlegible-Regular.ttf` and `OFL.txt`).
- That directory names its upstream as <https://github.com/googlefonts/atkinson-hyperlegible>,
  commit `1cb311624b2ddf88e9e37873999d165a8cd28b46`, file `fonts/ttf/AtkinsonHyperlegible-Regular.ttf`.
- SHA-256 of the font file:
  `7fb917c89019896d0b52ee84b7cbb3304c18cb90b19a62f5e32712bd23e97669`.

## Why this font

It was designed for readers with low vision: letters that are easy to confuse (I, l, 1, O,
0) have clearly different shapes, which helps on a projector. It has all Polish letters
(ą ć ę ł ń ó ś ź ż and the capitals), it is a single small file with one weight, and its
licence allows shipping it with the program.

## Updating

Replace the two files with the ones from the source directory, update the version, the date
and the checksum above, and check the panels on a screenshot. The file name is written in
`src/debug/Theme.cpp` (`FONT_FILE`).
