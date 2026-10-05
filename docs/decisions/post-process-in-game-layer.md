# Post-process jako `game::PostProcess`: nadal bez warstwy `src/renderer/`, mimo że klatka ma już kilka przebiegów

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/PostProcess.hpp`](../../src/game/PostProcess.hpp), [`PostProcess.cpp`](../../src/game/PostProcess.cpp), [`src/gfx/Framebuffer.hpp`](../../src/gfx/Framebuffer.hpp), [`Framebuffer.cpp`](../../src/gfx/Framebuffer.cpp), [`src/game/NightMazeApp.hpp`](../../src/game/NightMazeApp.hpp), [`NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (pola `m_postProcess`, `m_postProcessSettings`, `m_compositeShader`, `m_previewShader`, koniec `onRender`), [`CMakeLists.txt`](../../CMakeLists.txt) (listy źródeł `engine` i `night_maze`). Dokumenty modułów: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), [`../modules/renderer/README.md`](../modules/renderer/README.md), [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md).

## 1. Kontekst

PRD (sekcja 6) umieszcza klasę `PostProcess` w warstwie `src/renderer/`, obok `Renderer`, `ShadowPass`, `ScenePass`, `SkyboxPass` i `GrassPass`, a klasę `Framebuffer` w `src/gfx/`.

Warstwy `src/renderer/` w kodzie nie ma. Notatka [`skybox-in-game-layer.md`](skybox-in-game-layer.md) zapisała w M6, dlaczego niebo trafiło do `src/game/`, i wskazała moment powrotu do tematu: "w M7, gdy scena zacznie być rysowana do bufora HDR i dojdą przebiegi cieni". To samo mówił wstęp do dokumentów renderera: warstwa będzie potrzebna, gdy klatka przestanie być jednym przebiegiem.

Pierwsza część M7 spełniła pierwszą połowę tego warunku. Klatka ma teraz przebieg sceny do bufora HDR, opcjonalnie dwa małe przebiegi podglądów i przebieg `composite` do okna. Trzeba było zdecydować, czy nowa klasa otwiera warstwę `renderer/`, czy dołącza do klas rysujących w `src/game/`.

## 2. Decyzja

Klasa nazywa się `game::PostProcess`, leży w `src/game/` i należy do programu `night_maze`, tak jak `Skybox`, `TerrainRenderer`, `GrassRenderer` i pozostałe klasy rysujące. `gfx::Framebuffer` leży w `src/gfx/` i w bibliotece `engine`, zgodnie z PRD. Warstwa `src/renderer/` **nadal nie powstaje**. Kolejność przebiegów zostaje zapisana w `NightMazeApp::onRender`.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **`game::PostProcess` w `src/game/` (wybrana)** | spójnie z każdą klasą rysującą, która już istnieje. Programy `composite` i `preview` są polami `NightMazeApp` jak pozostałe sześć, więc panel Shaders przeładowuje wszystkie osiem jedną drogą. Zmiana M7 dotyczy rysowania, a nie układu katalogów, więc da się ją przejrzeć jako jedną rzecz | kod rozjeżdża się z PRD w kolejnym miejscu. Warunek zapisany w dwóch wcześniejszych dokumentach został spełniony w połowie, a warstwy dalej nie ma |
| Założyć `src/renderer/` teraz, tylko dla `PostProcess` | nazwa i miejsce jak w PRD | warstwa z jedną klasą, podczas gdy niebo, teren, trawa i labirynt rysują się z `src/game/`. Podział, którego nie umiałbym uzasadnić inaczej niż "tak było w planie" |
| Założyć `src/renderer/` teraz i przenieść wszystkie klasy rysujące | docelowy układ z PRD | duża zmiana samych ścieżek i przestrzeni nazw w środku kamienia milowego, w którym część po części dochodzą kolejne przebiegi (bloom, mgła, cienie). Granice warstwy trzeba by zgadywać, zanim wiadomo, jakie przebiegi i jakie wspólne dane naprawdę powstaną |

## 4. Uzasadnienie i skutki

**Dlaczego jeszcze nie teraz.** Warstwę warto wyciąć wtedy, gdy widać jej kształt. Dziś znam trzy przebiegi (scena, podglądy, `composite`), a M7 ma dołożyć bloom, mgłę, minimapę i dwa przebiegi cieni. Dopiero cienie pokażą, co musi być wspólne: lista obiektów rysowanych drugi raz z innej kamery, macierze światła, dodatkowe framebuffery. Przeniesienie klas teraz oznaczałoby przenoszenie ich ponownie.

**Co odróżnia tę klasę od `Skybox`.** `PostProcess` nie zna niczego z gry: nie dotyka labiryntu, rundy ani gracza. Zależy od `gfx::Framebuffer`, `gfx::VertexArray`, `gfx::Shader`, `core::Size` i od nazw uniformów w `game/ShaderUniforms.hpp`. Jest więc najlepszym kandydatem do przeniesienia, gdy warstwa powstanie: wystarczy zmienić przestrzeń nazw i miejsce stałych z nazwami uniformów.

**Skutki, które przyjmuję.**

- `NightMazeApp::onRender` jest dłuższe: zna kolejność `beginScene`, scena, `drawPreviews`, `composite`. Komentarze w tej funkcji wskazują, gdzie dojdą następne przebiegi.
- `PostProcess` jest w programie, nie w bibliotece `engine`, więc program testów go nie widzi. I tak nie dałoby się go przetestować bez kontekstu OpenGL: jego matematyka (krzywe, kodowanie) siedzi w shaderach i w `gfx::ColorSpace`, które testy ma.
- Zdania w [`skybox-in-game-layer.md`](skybox-in-game-layer.md) (sekcja 5) i we wstępie do dokumentów renderera, które wiązały powstanie warstwy z buforem HDR, są dziś nieaktualne co do terminu. Decyzja z tamtej notatki (niebo w `src/game/`) obowiązuje dalej.

**Czego tu nie ma.** Nie mam pomiaru ani porównania, które rozstrzygałoby ten wybór: to decyzja o porządku w kodzie, a nie o działaniu programu.

## 5. Kiedy wrócić do tej decyzji

- Przy cieniach (dalsza część M7): gdy scenę trzeba będzie narysować drugi i trzeci raz z punktu widzenia świateł, kolejność przebiegów i wspólne dane przestaną mieścić się w `onRender`. To jest nowy, konkretny termin na `src/renderer/`.
- Najpóźniej przy porządkach przed oddaniem (M9): jeśli warstwa nie powstanie wcześniej, trzeba albo ją założyć, albo zapisać w dokumentach, że struktura z PRD została świadomie zastąpiona.
- Gdyby `PostProcess` zaczął potrzebować danych gry (na przykład pozycji gracza dla minimapy): wtedy podział "nie zna gry" przestaje być prawdą i trzeba go przemyśleć przed przenosinami.
