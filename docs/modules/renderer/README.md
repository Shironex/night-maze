# Moduł renderer: wstęp

Ten katalog zbiera dokumenty o **technikach rysowania**: jak z danych sceny (siatek, tekstur, świateł) powstaje obraz. PRD (sekcja 7) przewiduje tu dokumenty `lighting-gouraud-phong.md`, `shadows.md`, `skybox.md`, `grass-geometry.md`, `env-mapping.md`, `post-process.md` i `terrain.md`.

## Uczciwie o stanie: katalogu `src/renderer/` jeszcze nie ma

PRD (sekcja 6) planuje w kodzie warstwę `renderer/` z klasami `Renderer`, `ShadowPass`, `ScenePass` i kolejnymi przebiegami. **Dziś tej warstwy w kodzie nie ma.** Gra rysuje jednym przebiegiem prosto do okna, a kod, który to robi, leży w dwóch miejscach:

| Co | Gdzie jest dziś | Dokument |
|---|---|---|
| wybór programu według trybu cieniowania, kolejność rysowania klatki | [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp): `onRender`, `drawMaze`, `drawUnlitMaze`, `drawLitMaze`, `drawLightMarkers` | [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), [`../core/README.md`](../core/README.md) |
| rysowanie labiryntu obiekt po obiekcie | [`src/game/MazeRenderer.*`](../../../src/game/MazeRenderer.cpp) | [`../game/maze-rendering.md`](../game/maze-rendering.md) |
| wysłanie świateł na kartę, kostki oznaczające światła | [`src/game/LightRig.*`](../../../src/game/LightRig.cpp) | [`../game/flashlight.md`](../game/flashlight.md) |
| shadery oświetlenia | [`assets/shaders/lit.*`](../../../assets/shaders/lit.frag), [`gouraud.*`](../../../assets/shaders/gouraud.vert), [`common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl) | [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), [`../scene/lights.md`](../scene/lights.md) |

**Dlaczego dokument stoi tu, a nie w `game/` albo `scene/`.** Z dwóch powodów:

- PRD wymienia `lighting-gouraud-phong.md` z nazwy w module `renderer`, a tabela w [`../../syllabus.md`](../../syllabus.md) ma prowadzić z tematu wykładu do dokumentu o przewidywalnej nazwie. Temat 7 nie jest o świetle jako danych (to temat 6, [`../scene/lights.md`](../scene/lights.md)), tylko o tym, **w którym etapie potoku** światło jest liczone. To pytanie o technikę rysowania.
- Gdy powstanie warstwa `src/renderer/` (będzie potrzebna, gdy klatka przestanie być jednym przebiegiem: przy cieniach i rysowaniu do tekstury), kod wyboru programu przeniesie się tam z `NightMazeApp`. Dokument będzie już na miejscu i zmienią się w nim tylko ścieżki plików.

Komentarz `// See docs/...` na górze czterech plików shaderów `lit.*` i `gouraud.*` wskazuje ten katalog.

## Dokumenty

| Dokument | Temat wykładu | Stan |
|---|---|---|
| [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) | 7. Gouraud vs Phong: światło na wierzchołek albo na fragment, odbłysk Phonga i Blinna-Phonga, shadery `lit` i `gouraud`, przełącznik trybu w panelu Renderer | zrobione w M4, zbudowane i uruchomione na Windowsie, macOS i test ręczny otwarte |

Kolejne dokumenty (cienie, skybox i dalsze) dojdą razem z kodem swoich kamieni milowych. Plan jest w [`../../syllabus.md`](../../syllabus.md).
