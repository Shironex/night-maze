# Moduł renderer: wstęp

Ten katalog zbiera dokumenty o **technikach rysowania**: jak z danych sceny (siatek, tekstur, świateł) powstaje obraz. PRD (sekcja 7) przewiduje tu dokumenty `lighting-gouraud-phong.md`, `shadows.md`, `skybox.md`, `grass-geometry.md`, `env-mapping.md`, `post-process.md` i `terrain.md`. Dziś są dwa: cieniowanie (temat 7) i, od pierwszej części M6, skybox (temat 8).

## Uczciwie o stanie: katalogu `src/renderer/` jeszcze nie ma

PRD (sekcja 6) planuje w kodzie warstwę `renderer/` z klasami `Renderer`, `ShadowPass`, `ScenePass`, `SkyboxPass` i kolejnymi przebiegami. **Dziś tej warstwy w kodzie nie ma.** Gra rysuje jednym przebiegiem prosto do okna pięcioma programami shaderów (`textured`, `color`, `lit`, `gouraud`, `skybox`), a kod, który to robi, leży w dwóch katalogach, `src/game/` i `assets/shaders/`:

| Co | Gdzie jest dziś | Dokument |
|---|---|---|
| wybór programu według trybu cieniowania, kolejność rysowania klatki | [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp): `onRender`, `drawMaze`, `drawUnlitMaze`, `drawLitMaze` | [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), [`../core/README.md`](../core/README.md) |
| rysowanie labiryntu obiekt po obiekcie | [`src/game/MazeRenderer.*`](../../../src/game/MazeRenderer.cpp), wspólna funkcja `game::drawModel` w [`src/game/ModelDraw.*`](../../../src/game/ModelDraw.cpp) | [`../game/maze-rendering.md`](../game/maze-rendering.md) |
| rysowanie kryształów i bramy tym samym programem co labirynt, ustawianie składnika emisyjnego `uEmissive` (M5) | [`src/game/GameplayRenderer.*`](../../../src/game/GameplayRenderer.cpp) | [`../game/gameplay.md`](../game/gameplay.md), [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) |
| wysłanie świateł na kartę (bufor uniformów) | [`src/game/LightRig.*`](../../../src/game/LightRig.cpp) | [`../game/flashlight.md`](../game/flashlight.md) |
| shadery oświetlenia | [`assets/shaders/lit.*`](../../../assets/shaders/lit.frag), [`gouraud.*`](../../../assets/shaders/gouraud.vert), [`common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl) | [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), [`../scene/lights.md`](../scene/lights.md) |
| niebo: wczytanie sześciu obrazów, sześcian, przebieg rysujący na końcu klatki (M6) | [`src/game/Skybox.*`](../../../src/game/Skybox.cpp), wywołanie na końcu `onRender` w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) | [`skybox.md`](skybox.md) |
| shadery nieba (M6) | [`assets/shaders/skybox.vert`](../../../assets/shaders/skybox.vert), [`skybox.frag`](../../../assets/shaders/skybox.frag) | [`skybox.md`](skybox.md), sekcja 4 |
| tekstura sześcienna jako obiekt OpenGL (M6) | [`src/gfx/Cubemap.*`](../../../src/gfx/Cubemap.cpp): ta klasa jest w warstwie `gfx/`, tak jak chce PRD | [`../gfx/cubemap.md`](../gfx/cubemap.md) |

**Dlaczego dokumenty stoją tu, a nie w `game/` albo `scene/`.** Z dwóch powodów:

- PRD wymienia `lighting-gouraud-phong.md` i `skybox.md` z nazwy w module `renderer`, a tabela w [`../../syllabus.md`](../../syllabus.md) ma prowadzić z tematu wykładu do dokumentu o przewidywalnej nazwie. Temat 7 nie jest o świetle jako danych (to temat 6, [`../scene/lights.md`](../scene/lights.md)), tylko o tym, **w którym etapie potoku** światło jest liczone. Temat 8 nie jest o regułach gry, tylko o rodzaju tekstury i o przebiegu, który ją rysuje. To pytania o technikę rysowania.
- Gdy powstanie warstwa `src/renderer/` (będzie potrzebna, gdy klatka przestanie być jednym przebiegiem: przy cieniach i rysowaniu do tekstury), kod wyboru programu przeniesie się tam z `NightMazeApp`, a klasa nieba z `src/game/`. Dokumenty będą już na miejscu i zmienią się w nich tylko ścieżki plików.

Niebo jest pierwszym przypadkiem, w którym PRD podaje nazwę klasy (`SkyboxPass` w `src/renderer/`), a kod ma inną (`game::Skybox` w `src/game/`). Dlaczego, zapisuje notatka [`../../decisions/skybox-in-game-layer.md`](../../decisions/skybox-in-game-layer.md).

Komentarz `// See docs/...` na górze czterech plików shaderów `lit.*` i `gouraud.*`, dwóch plików `skybox.*`, plików `src/game/Skybox.*`, `src/gfx/Cubemap.*`, skryptu `tools/blender/make_skybox.py` i testów `tests/SkyboxTests.cpp` wskazuje ten katalog.

## Dokumenty

| Dokument | Temat wykładu | Stan |
|---|---|---|
| [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) | 7. Gouraud vs Phong: światło na wierzchołek albo na fragment, odbłysk Phonga i Blinna-Phonga, shadery `lit` i `gouraud`, przełącznik trybu w panelu Renderer | zrobione w M4, zbudowane i uruchomione na Windowsie. W M5 uzupełnione o składnik emisyjny `uEmissive` oraz o kryształy i bramę rysowane programami `lit` i `gouraud`: M5 jest gotowy w kodzie na Windowsie i nie jest zamknięty. macOS i test ręczny otwarte dla obu części. Piąty program gry, `skybox` z M6, nie bierze udziału w cieniowaniu i w tym dokumencie pojawia się tylko jako wzmianka |
| [`skybox.md`](skybox.md) | 8. Tekstura sześcienna: sześć ścian i ich kolejność, jak kierunek wybiera ścianę i teksel, dlaczego ściany nie są odwracane (`RowOrder`), szwy i `GL_CLAMP_TO_EDGE`, macierz widoku bez przesunięcia, głębia 1,0 przez `xyww`, `GL_LEQUAL` i rysowanie na końcu, shadery `skybox` linia po linii, generator obrazów, księżyc sprzężony z domyślnym kierunkiem światła, pole `Skybox` i suwak `Sky brightness` w panelu Renderer | pierwsza część M6, zgłoszona jako zbudowana i przetestowana na Windowsie (2026-10-05: 221 przypadków testowych, 85175 asercji, orientacja nieba na zrzucie ekranu). Kontrolek nikt nie klikał myszą, macOS otwarty. M6 jest w toku: teren (temat 13) i trawa (temat 9) nie mają jeszcze dokumentów w tym katalogu |

Kolejne dokumenty (teren, trawa, cienie i dalsze) dojdą razem z kodem swoich kamieni milowych. Plan jest w [`../../syllabus.md`](../../syllabus.md).
