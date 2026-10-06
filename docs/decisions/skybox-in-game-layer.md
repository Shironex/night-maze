# Niebo jako `game::Skybox`: bez warstwy `src/renderer/` i bez klasy `SkyboxPass`

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Skybox.hpp`](../../src/game/Skybox.hpp), [`Skybox.cpp`](../../src/game/Skybox.cpp) (`SkyboxSettings`, `Skybox`, `loadSkyCubemap`), [`src/gfx/Cubemap.hpp`](../../src/gfx/Cubemap.hpp), [`Cubemap.cpp`](../../src/gfx/Cubemap.cpp), [`src/game/NightMazeApp.hpp`](../../src/game/NightMazeApp.hpp), [`NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (pola `m_skyboxShader`, `m_skybox`, `m_skyboxSettings`, koniec `onRender`), [`CMakeLists.txt`](../../CMakeLists.txt) (lista źródeł programu `night_maze`). Dokument modułu: [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcje 5.4 do 5.6, i [`../modules/renderer/README.md`](../modules/renderer/README.md).

## 1. Kontekst

PRD (sekcja 6) opisuje docelową strukturę kodu z warstwą `src/renderer/`: klasy `Renderer`, `ShadowPass`, `ScenePass`, `SkyboxPass`, `GrassPass`, `PostProcess`. Tabela potoku klatki (PRD, sekcja 8) ma skybox jako piąty z ośmiu przebiegów.

Gdy powstawało niebo (pierwsza część M6), tej warstwy w kodzie nie było i nadal jej nie ma. Klatka jest jednym przebiegiem prosto do okna: `NightMazeApp::onRender` czyści ekran, liczy dwie macierze i światła, a potem woła po kolei funkcje rysujące. Klasy, które rysują, leżą w `src/game/` i należą do programu `night_maze`: `MazeRenderer`, `GameplayRenderer`, `ColliderLines`, `LightRig`. Programy shaderów są polami `NightMazeApp`, bo panel Shaders przeładowuje je wszystkie jedną drogą.

Trzeba było zdecydować, gdzie postawić kod nieba: czy to jest moment na założenie warstwy `renderer/`, czy niebo dołącza do tego, co już jest.

## 2. Decyzja

Niebo rysuje klasa `game::Skybox` w `src/game/`, w programie `night_maze`, obok pozostałych klas rysujących. Jej program shaderów jest piątym polem `gfx::Shader` w `NightMazeApp`, a wywołanie `m_skybox.draw(...)` stoi na końcu `onRender`. Do warstwy `gfx/` trafiło tylko to, co nie wie nic o grze: klasa `gfx::Cubemap`, zgodnie z PRD. Katalogu `src/renderer/` ta zmiana nie zakłada.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **`game::Skybox` obok innych klas rysujących (wybrana)** | ten sam wzór co `MazeRenderer` i `ColliderLines`: klasa posiada swoje obiekty OpenGL, dostaje program i macierze w argumentach. Żadnej nowej warstwy, żadnej nowej reguły zależności. Jedna linia w `onRender` pokazuje, kiedy niebo jest rysowane | nazwa i miejsce inne niż w PRD. Kolejność przebiegów jest zapisana w ciele `onRender`, a nie w osobnej klasie. Gdy warstwa `renderer/` powstanie, plik trzeba będzie przenieść |
| Założyć `src/renderer/` i dać w niej `SkyboxPass` | zgodne z PRD co do nazwy | warstwa z jedną klasą. Labirynt, kryształy i linie kolizji zostałyby w `game/`, więc "renderer" rysowałby tylko niebo, a klatkę i tak składałoby `NightMazeApp`. Trzeba by ustalić reguły zależności nowej warstwy, zanim wiadomo, czego potrzebują cienie i rysowanie do tekstury |
| Założyć `src/renderer/` i przenieść tam od razu wszystkie klasy rysujące | struktura z PRD w całości | duża przebudowa niezwiązana z tematem 8, w środku kamienia milowego, bez przebiegu, który by jej wymagał. Zmiana ścieżek w kilkunastu dokumentach |
| Pola i funkcja w samym `NightMazeApp`, jak kostka z M1 | najmniej plików | `NightMazeApp` znów rósłby o dane wierzchołków, wczytywanie plików i stan OpenGL jednej techniki, tak jak przy kostce z M1, usuniętej w M5 |
| Wszystko w `gfx/`, jako klasa silnika | dostępne dla zadań laboratoryjnych | `gfx/` nie zna warstwy `assets/` ani plików gry, a niebo musi wczytać sześć konkretnych plików i zna tryb podglądu `game::ViewMode` |

## 4. Uzasadnienie i skutki

**Dlaczego nie warstwa teraz.** Warstwa `renderer/` ma sens, gdy klatka przestaje być jednym przebiegiem: gdy scena jest rysowana najpierw do mapy cieni, potem do bufora HDR, a dopiero potem na ekran. Wtedy trzeba jednego miejsca, które zna kolejność przebiegów i ich cele rysowania. Dziś celem jest zawsze okno, a kolejność to trzy wywołania w `onRender`. Zakładanie warstwy dla jednego wywołania byłoby zgadywaniem jej kształtu przed poznaniem wymagań M7.

**Dlaczego klasa, a nie funkcja w `NightMazeApp`.** Niebo ma własne obiekty OpenGL (teksturę sześcienną i siatkę sześcianu), własne dane (osiem narożników, nazwy sześciu plików) i własny stan do przywrócenia po rysowaniu. To komplet, który da się przeczytać w jednym pliku i wytłumaczyć bez reszty aplikacji.

**Dlaczego program jest polem `NightMazeApp`, a nie `Skybox`.** Wszystkie programy gry stoją w jednym miejscu i trafiają do panelu Shaders tą samą drogą: akcesor, pole `DebugContext`, wpis w tablicy. `Skybox` dostaje program w argumencie `draw`, tak jak `MazeRenderer`.

**Co dzięki temu dostaję.** Zmiana dotknęła `NightMazeApp` w kilku miejscach (dwa pola z obiektami, pole ustawień, dwa akcesory dla panelu, jedno wywołanie) i nie zmieniła żadnej reguły zależności między warstwami. Dokument tematu 8 stoi mimo to pod nazwą z PRD, `docs/modules/renderer/skybox.md`, więc tabela w sylabusie prowadzi tam, gdzie prowadzący się go spodziewa.

**Co przez to tracę.**

- Kod nie odpowiada PRD co do nazwy klasy i katalogu. Na pytanie "gdzie jest `SkyboxPass`" odpowiedzią jest ta notatka.
- `onRender` jest jedynym miejscem, które zna kolejność rysowania. To, że niebo ma być ostatnim wywołaniem sceny nieprzezroczystej, jest komentarzem, a nie strukturą kodu.
- `Skybox` trzyma swoją teksturę sześcienną prywatnie. Kod, który zechce jej użyć do czegoś innego niż niebo, nie ma jak jej dostać.

## 5. Kiedy wrócić do tej decyzji

- W M7, gdy scena zacznie być rysowana do bufora HDR i dojdą przebiegi cieni: wtedy niebo musi trafić do tego samego bufora co scena, przed post-processem, a kolejność przebiegów przestaje mieścić się w kilku liniach `onRender`. To jest moment na warstwę `src/renderer/` i przeniesienie do niej `Skybox` razem z pozostałymi klasami rysującymi. **Dopisek z 2026-10-05 (M7, część pierwsza):** bufor HDR już jest, niebo trafia do niego razem ze sceną, a warstwa nadal nie powstała. Nową klasę `game::PostProcess` i nowy termin (przebiegi cieni) zapisuje notatka [`post-process-in-game-layer.md`](post-process-in-game-layer.md). Decyzja z tej notatki obowiązuje dalej.
- W M8, przy environment mappingu (temat 12): kryształy i kałuże mają odbijać niebo, czyli czytać tę samą teksturę sześcienną. Trzeba będzie albo dać `Skybox` akcesor do `gfx::Cubemap`, albo przenieść własność tekstury wyżej. **Dopisek (2026-10-06, M8, część 1):** wybrany został pierwszy wariant: `Skybox::cubemap()` oddaje teksturę do odczytu programowi `reflect` ([`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcja 3.1), własność zostaje w `Skybox`.
- Gdy dojdzie pierwszy obiekt przezroczysty z mieszaniem kolorów (iskry, mgła): kolejność "niebo na końcu" przestaje wtedy wystarczać, bo przezroczyste rysuje się po nieprzezroczystych i po niebie, a komentarz w `onRender` każe dziś rysować wszystko przed nim.
