# Moduł game: logika Night Maze

Kamień milowy: M2 + M3 (labirynt, gracz, rysowanie labiryntu). Temat wykładu: żaden wprost, moduł korzysta z tematów 3, 4, 5 i 14 i jest miejscem, w którym spotykają się one w jednej klatce.
Kod: [`src/game/`](../../../src/game/), testy w [`tests/`](../../../tests/).

Warstwy `core`, `gfx` i `scene` są narzędziami: okno, obiekty OpenGL, matematyka sceny. Nie wiedzą, w jaką grę gramy. Wszystko, co jest **specyficzne dla Night Maze**, leży w module `game`: labirynt, jego generator, układ ścian w świecie, gracz i rysowanie labiryntu, a później latarka, kryształy i stany gry (PRD, sekcja 6).

Na dziś moduł ma dwie części o bardzo różnym charakterze:

| Część | Pliki | Czego potrzebuje | Gdzie jest kompilowana |
|---|---|---|---|
| aplikacja i rysowanie | `NightMazeApp.hpp/.cpp`, `MazeRenderer.*`, `ColliderLines.*`, `ShaderUniforms.hpp` | okna, kontekstu OpenGL, klawiatury i myszy | wprost w programie `night_maze` |
| logika bez okna | `Maze.*`, `MazeGenerator.*`, `MazeLayout.*`, `MazeWorld.*`, `Player.*` | tylko biblioteki standardowej, GLM i nagłówków `scene/` (`Collider`, `Transform`, `Camera`) | biblioteka statyczna `game_logic` |

**Stan na dziś, uczciwie.** Obie części są połączone: program startuje wewnątrz oteksturowanego labiryntu 10 na 10 komórek (ziarno 1), gracz chodzi po nim z kolizjami albo lata w trybie noclip, a kostka z M1 unosi się nad przeciwległym rogiem jako znacznik przyszłego wyjścia. Na Windowsie (2026-10-05, MSVC 19.44) build Debug i Release przechodzi bez ostrzeżeń, 50 przypadków testowych logiki gry przechodzi w obu konfiguracjach (w ramach 87 przypadków i 60858 asercji całego programu testowego), a program startuje bez linii `[error]`. Kamień milowy M2 + M3 nie jest zamknięty: chodzenia prawdziwymi klawiszami, klawisza N, regeneracji labiryntu i widżetów nowych paneli nikt jeszcze nie sprawdził ręcznie, a na macOS kod nie był budowany ani uruchamiany.

Ten plik jest wstępem i indeksem dokumentów modułu.

## 1. Dokumenty modułu

| Dokument | Co opisuje | Typy i pliki |
|---|---|---|
| [`maze-generator.md`](maze-generator.md) | zapis labiryntu (siatka komórek, ściany na krawędziach), labirynt doskonały, algorytm recursive backtracker krok po kroku z przykładem, wersja iteracyjna, liczby losowe takie same na każdym systemie (`std::mt19937`, własna funkcja `randomBelow`, błąd reszty z dzielenia), układ w świecie (komórki, ściany, słupki, pudełka kolizji), testy z labiryntem wzorcowym | `Maze`, `Direction`, `generateMaze`, `randomBelow`, `WallSegment`, `wallSegments`, `pillarPositions`, `mazeColliders`, panel Maze (`drawMazePanel`) |
| [`maze-rendering.md`](maze-rendering.md) | labirynt w świecie i jego rysowanie: od siatki przez rozmieszczenie do macierzy modelu, dlaczego macierze są liczone raz, obrót ścian wzdłuż Z o 90 stopni, jedno wywołanie rysujące na obiekt i jego koszt, start, wyjście i kostka jako znacznik, regeneracja przez `MazeSettings` | `MazeSettings`, `MazeWorld`, `buildMazeWorld`, `yawTowards`, `ViewMode`, `MazeRenderer`, `NightMazeApp::regenerateMaze`, `enterMaze`, `drawMaze` |
| [`player.md`](player.md) | gracz: stopy, pudełko i oczy, wejście jako struktura, chodzenie a noclip, dlaczego chód jest poziomy, prędkości i normalizacja, użycie `moveAndSlide`, stały krok i interpolacja, powrót na podłogę, testy | `PlayerInput`, `Player`, `Player::update`, `NightMazeApp::onUpdate` |

Każdy z trzech dokumentów ma pełne dziesięć sekcji, tak jak dokumenty modułów `core`, `gfx` i `scene`. PRD (sekcja 7) przewiduje dla modułu jeszcze `flashlight.md` i `gameplay.md`. Powstaną razem z kodem.

Klasa `NightMazeApp` należy do katalogu `src/game/`, ale jej opis jest rozłożony na dokumenty tematów, które realizuje: klatka jako całość w [`../core/README.md`](../core/README.md), rysowanie kostki w [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md), macierze w [`../scene/camera.md`](../scene/camera.md), obrót kamery myszą w [`../scene/camera-controls.md`](../scene/camera-controls.md), ruch gracza w [`player.md`](player.md), labirynt w [`maze-rendering.md`](maze-rendering.md). Dwa pliki z `src/game/` mają dokumenty w innych modułach, bo realizują ich tematy: `ColliderLines.*` (rysowanie pudełek kolizji) w [`../scene/collision.md`](../scene/collision.md), a `ShaderUniforms.hpp` (nazwy uniformów) w [`../gfx/uniforms.md`](../gfx/uniforms.md).

Proponowana kolejność czytania: [`../scene/collision.md`](../scene/collision.md) (pudełka AABB, bo układ labiryntu je produkuje), ten plik, [`maze-generator.md`](maze-generator.md), [`maze-rendering.md`](maze-rendering.md), [`player.md`](player.md), a obok [`../../libraries/doctest.md`](../../libraries/doctest.md) (jak czytać i uruchamiać testy). Przed `maze-rendering.md` warto znać [`../assets/asset-cache.md`](../assets/asset-cache.md) (skąd biorą się modele).

## 2. Indeks: plik kodu, dokument

| Plik kodu | Co zawiera | Dokument |
|---|---|---|
| [`src/game/Maze.hpp`](../../../src/game/Maze.hpp), [`.cpp`](../../../src/game/Maze.cpp) | typ `Direction` (North, East, South, West), stałe `DIRECTION_COUNT` i `ALL_DIRECTIONS`, funkcje `opposite`, `columnStep`, `rowStep`, klasa `Maze`: `width`, `height`, `contains`, `hasWall`, `removeWall`, stała `MAX_SIZE` | [`maze-generator.md`](maze-generator.md), sekcje 5.2 i 5.3 |
| [`src/game/MazeGenerator.hpp`](../../../src/game/MazeGenerator.hpp), [`.cpp`](../../../src/game/MazeGenerator.cpp) | `randomBelow` (losowa liczba poniżej granicy, taka sama na każdym systemie), `generateMaze` (labirynt doskonały z rozmiaru i ziarna) | [`maze-generator.md`](maze-generator.md), sekcje 5.4 i 5.5 |
| [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`.cpp`](../../../src/game/MazeLayout.cpp) | stałe `CELL_SIZE`, `WALL_LENGTH`, `WALL_HEIGHT`, `PILLAR_SIZE`, `WALL_VISUAL_THICKNESS`, `WALL_COLLISION_THICKNESS`, `PILLAR_HEIGHT`, typy `WallAxis` i `WallSegment`, funkcje `cellCenter`, `wallSegments`, `pillarPositions`, `wallBox`, `pillarBox`, `mazeColliders` | [`maze-generator.md`](maze-generator.md), sekcje 5.6 i 5.7 |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | aplikacja: okno, trzy programy shaderów, pamięć podręczna assetów, labirynt, gracz, kamera, kostka jako znacznik, sterowanie | [`../core/README.md`](../core/README.md) i dokumenty wymienione w sekcji 1 |
| [`src/game/MazeWorld.hpp`](../../../src/game/MazeWorld.hpp), [`.cpp`](../../../src/game/MazeWorld.cpp) | stałe labiryntu domyślnego, struktury `MazeSettings` i `MazeWorld`, funkcje `yawTowards` i `buildMazeWorld` | [`maze-rendering.md`](maze-rendering.md), sekcje od 5.2 do 5.5 |
| [`src/game/MazeRenderer.hpp`](../../../src/game/MazeRenderer.hpp), [`.cpp`](../../../src/game/MazeRenderer.cpp) | wyliczenie `ViewMode`, klasa `MazeRenderer`: `draw`, `drawInstances` | [`maze-rendering.md`](maze-rendering.md), sekcja 5.6 |
| [`src/game/Player.hpp`](../../../src/game/Player.hpp), [`.cpp`](../../../src/game/Player.cpp) | struktury `PlayerInput` i `Player`: stałe, pola, `box`, `eyePosition`, `update` | [`player.md`](player.md), sekcje od 5.2 do 5.5 |
| [`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`.cpp`](../../../src/game/ColliderLines.cpp) | klasa `ColliderLines`: sześcian z linii i rysowanie pudełek kolizji | [`../scene/collision.md`](../scene/collision.md), sekcja 5.8 |
| [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp) | nazwy uniformów wszystkich shaderów w jednym miejscu | [`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 5 |
| [`src/debug/panels/MazePanel.hpp`](../../../src/debug/panels/MazePanel.hpp), [`.cpp`](../../../src/debug/panels/MazePanel.cpp) | panel Maze (plik leży w `src/debug/`, ale dotyczy labiryntu) | [`maze-generator.md`](maze-generator.md), sekcja 6 |
| [`tests/MazeTests.cpp`](../../../tests/MazeTests.cpp), [`MazeGeneratorTests.cpp`](../../../tests/MazeGeneratorTests.cpp), [`MazeLayoutTests.cpp`](../../../tests/MazeLayoutTests.cpp) | testy jednostkowe logiki labiryntu (29 przypadków testowych) | [`maze-generator.md`](maze-generator.md), sekcja 5.8, [`../../libraries/doctest.md`](../../libraries/doctest.md) |
| [`tests/MazeWorldTests.cpp`](../../../tests/MazeWorldTests.cpp) | testy `MazeWorld` (8 przypadków) | [`maze-rendering.md`](maze-rendering.md), sekcja 5.9 |
| [`tests/PlayerTests.cpp`](../../../tests/PlayerTests.cpp) | testy gracza (13 przypadków) | [`player.md`](player.md), sekcja 5.9 |

## 3. Miejsce w warstwach i dwa targety

```mermaid
flowchart TD
    Main["main.cpp<br/>DebugNightMazeApp, main"] --> Debug["debug/<br/>DebugUI, panele"]
    Main --> App["game/ (w programie)<br/>NightMazeApp, MazeRenderer, ColliderLines"]
    Tests["tests/<br/>night_maze_tests"] --> Logic
    Tests --> Scene
    Tests --> Assets
    Logic["game/ (biblioteka game_logic)<br/>Maze, MazeGenerator, MazeLayout, MazeWorld, Player"] --> Scene["scene/<br/>Transform, Camera, Collider"]
    App --> Logic
    App --> Scene
    App --> Assets["assets/<br/>AssetCache, ObjLoader, ImageLoader"]
    App --> Gfx["gfx/<br/>Shader, Buffer, VertexArray, Mesh, Texture2D"]
    App --> Core["core/<br/>Application, Window, Input, Time"]
    Debug --> Core
    Debug --> Gfx
    Debug --> Scene
    Debug --> Assets
    Debug --> App
    Debug --> Logic
    Assets --> Gfx
    Assets --> Core
    Gfx --> Core
    Scene --> Glm["GLM"]
    Logic --> Glm
    Tests --> Doctest["doctest"]
```

Strzałka znaczy "zna i dołącza nagłówki". Diagram pokazuje stan faktyczny. Wobec diagramu z [`../scene/README.md`](../scene/README.md) pokazuje dodatkowo program testowy `night_maze_tests`, podział `game/` na dwie części i warstwę `assets/`. Strzałka od aplikacji do logiki istnieje: `NightMazeApp` dołącza `game/MazeWorld.hpp` i `game/Player.hpp`. Strzałki od `debug/` do `game/` też: panele dołączają `game/Player.hpp`, `game/MazeWorld.hpp`, `game/MazeLayout.hpp` i `game/MazeRenderer.hpp` (ten ostatni dla wyliczenia `ViewMode`). W drugą stronę strzałki nie ma: nic w `game/` nie dołącza `debug/`.

Zasady warstwy `game`:

1. `game/` może zależeć od `scene/`, `gfx/`, `core/` i GLM. Nie zna `debug/` ani ImGui: panele sięgają do gry, nigdy odwrotnie.
2. Pliki logiki (`Maze`, `MazeGenerator`, `MazeLayout`, `MazeWorld`, `Player`) są jeszcze ostrzejsze: dołączają tylko bibliotekę standardową, GLM i nagłówki `scene/` bez OpenGL (`Collider.hpp`, `Transform.hpp`, `Camera.hpp`). Żadnego OpenGL, okna, wejścia ani czasu. To warunek, żeby dało się je testować bez okna.
3. Nic poniżej (`core/`, `gfx/`, `scene/`) nie zna `game/`.

**Dlaczego logika jest osobną biblioteką.** Do tej pory wszystkie pliki z `src/game/` były kompilowane wprost w programie `night_maze`. Program testowy jest jednak drugim, osobnym programem, a jeden program nie może dołączyć kodu, który jest częścią innego programu. Kod wspólny dla obu musi więc być biblioteką. W [`CMakeLists.txt`](../../../CMakeLists.txt) są teraz trzy nasze targety z kodem gry i silnika oraz program testowy:

| Target | Rodzaj | Zawiera | Linkuje |
|---|---|---|---|
| `engine` | biblioteka statyczna | `src/assets/*`, `src/core/*`, `src/gfx/*`, `src/scene/*` (w tym `Collider`) | zależności zewnętrzne opisane w [`../../guides/project-structure.md`](../../guides/project-structure.md) |
| `game_logic` | biblioteka statyczna | `src/game/Maze.*`, `MazeGenerator.*`, `MazeLayout.*`, `MazeWorld.*`, `Player.*` | `engine` (`PUBLIC`) |
| `night_maze` | program | `src/main.cpp`, `src/game/NightMazeApp.*`, `MazeRenderer.*`, `ColliderLines.*`, `ShaderUniforms.hpp`, `src/debug/*` | `engine`, `game_logic`, `imgui` |
| `night_maze_tests` | program | `tests/*.cpp` | `game_logic`, `doctest::doctest` |

`NightMazeApp` i dwie klasy rysujące (`MazeRenderer`, `ColliderLines`) zostają w programie: potrzebują okna i kontekstu OpenGL, więc test i tak nie mógłby ich uruchomić. `game_logic` linkuje `engine` jako `PUBLIC`, bo nagłówki `game/MazeLayout.hpp`, `game/MazeWorld.hpp` i `game/Player.hpp` dołączają `scene/Collider.hpp` i GLM: każdy, kto dołączy ten nagłówek, potrzebuje ścieżek nagłówków `engine`. Opis targetów linia po linii: [`../../guides/project-structure.md`](../../guides/project-structure.md), sekcja 3.1.

Granica targetu pilnuje zasady 2: gdyby plik logiki dołączył ImGui, `game_logic` nie zna tej ścieżki nagłówków i kompilacja by się nie udała. Nie pilnuje natomiast, żeby logika nie wołała OpenGL, bo `engine`, które linkuje, zawiera GLAD. Tu obowiązuje dyscyplina dyrektyw `#include`.

## 4. Konwencje wspólne dla modułu

| Ustalenie | Wartość | Gdzie opisane |
|---|---|---|
| jednostka | 1 jednostka to 1 metr | [`../scene/README.md`](../scene/README.md), sekcja 5 |
| kompas | North to -Z, East to +X, South to +Z, West to -X, tak jak yaw kamery | [`maze-generator.md`](maze-generator.md), sekcja 2.2 |
| siatka | kolumna `x` rośnie w stronę +X, wiersz `z` w stronę +Z, komórka `(0, 0)` jest w rogu północno-zachodnim | [`maze-generator.md`](maze-generator.md), sekcja 2.1 |
| położenie labiryntu | zaczyna się w początku układu świata, podłoga w `y = 0` | [`maze-generator.md`](maze-generator.md), sekcja 2.7 |
| losowość | zawsze z ziarna, przez `std::mt19937` i `randomBelow`, bez rozkładów z biblioteki standardowej | [`maze-generator.md`](maze-generator.md), sekcja 2.6, [`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md) |
| błędy | zły argument to wyjątek (`std::invalid_argument`, `std::out_of_range`), nie cichy błędny wynik | [`maze-generator.md`](maze-generator.md), sekcje 5.3 i 5.4 |

## 5. Pytania kontrolne

Pytania z odpowiedziami do labiryntu, generatora i układu są w [`maze-generator.md`](maze-generator.md), sekcja 9, do rysowania labiryntu w [`maze-rendering.md`](maze-rendering.md), sekcja 9, a do gracza w [`player.md`](player.md), sekcja 9. Trzy pytania dotyczące treści tego pliku:

1. **Co należy do modułu `game`, a co do `scene`?**
   Do `game` to, co istnieje tylko w Night Maze: labirynt, jego wymiary, generator, gracz, a później kryształy. Do `scene` to, co przyda się w każdym programie 3D: transformacje, kamera, pudełka kolizji. Pudełko AABB jest w `scene`, a to, że ściana labiryntu ma pudełko 2 na 3 na 0,3 m, jest w `game`.

2. **Dlaczego pliki labiryntu są w bibliotece `game_logic`, a `NightMazeApp` w programie?**
   Testy są osobnym programem i mogą dołączyć tylko kod z biblioteki, więc logika, którą chcę testować, musi być biblioteką. `NightMazeApp` potrzebuje okna i kontekstu OpenGL, których test nie ma, więc zostaje w programie.

3. **Czego nie wolno dołączać w plikach logiki i dlaczego?**
   OpenGL (GLAD), GLFW, `core/Window`, wejścia, ImGui. Kod, który ich nie dotyka, daje się uruchomić w teście bez okna i daje ten sam wynik na każdym komputerze.

## 6. Źródła

- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 2 (mechaniki gry), sekcja 6 (podział na warstwy, zawartość `game/`), sekcja 7 (lista dokumentów modułu `game`).
- Dokumenty w tym repozytorium: [`maze-generator.md`](maze-generator.md), [`maze-rendering.md`](maze-rendering.md), [`player.md`](player.md), [`../scene/collision.md`](../scene/collision.md), [`../../libraries/doctest.md`](../../libraries/doctest.md), [`../../guides/project-structure.md`](../../guides/project-structure.md) (targety i katalogi).
- Szczegółowe źródła do algorytmu i liczb losowych są w sekcji 10 dokumentu [`maze-generator.md`](maze-generator.md).
