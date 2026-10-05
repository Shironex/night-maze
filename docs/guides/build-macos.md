# Budowanie na macOS (Apple Silicon)

Przewodnik dla kamienia milowego M0. Polecenia budowania i uruchamiania z tego dokumentu
zostały uruchomione na Macu na kodzie M0 i M1, w konfiguracji z tabeli niżej. **Kod M2 + M3
(testy, labirynt, gracz, modele, tekstury, nowe panele) nie był na macOS ani budowany, ani
uruchamiany.** To samo dotyczy obu części M4: oświetlenia (światła, cztery
tryby cieniowania, blok uniformów, `#include` w shaderach, panel Lights) i map normalnych
(styczne, czwarty atrybut wierzchołka, `common/normal_map.glsl`, pole `Normal mapping`).
Powstały na Windowsie 2026-10-05 i na macOS nikt ich nie zbudował. Wszystko, co ten dokument mówi o tym
kodzie dla Maca, jest oczekiwaniem wynikającym z kodu i z pomiarów na Windowsie, a punkty
do sprawdzenia są zebrane w sekcji 2 jako listy otwarte: "M2 + M3 na macOS", "M4
(oświetlenie) na macOS" i "M4 (mapy normalnych) na macOS".

| Element | Wersja |
|---|---|
| Kompilator | Apple clang 17.0.0 (z Xcode) |
| CMake | 4.3.3 |
| Generator | Unix Makefiles (domyślny) |
| Karta | Apple M3 |
| Wynik programu | `GL_VERSION: 4.1 Metal - 90.5` |

Wersja dla Windowsa: [`build-windows.md`](build-windows.md).

## 1. Wymagania

| Narzędzie | Po co | Jak zainstalować | Jak sprawdzić |
|---|---|---|---|
| Xcode Command Line Tools | kompilator `clang`, `make`, `git`, nagłówki systemowe i frameworki (Cocoa, OpenGL) | `xcode-select --install` | `clang --version` |
| CMake w wersji co najmniej 3.24 | konfiguracja i uruchamianie buildu | `brew install cmake` | `cmake --version` |
| git | CMake pobiera nim GLFW, GLM, ImGui, doctest i stb | jest w Command Line Tools | `git --version` |
| Ninja | opcjonalny szybszy generator | `brew install ninja` | `ninja --version` |

Uwagi:

- Pełny Xcode nie jest potrzebny, wystarczą Command Line Tools. Pełny Xcode też działa.
- **Ninja nie jest wymagana.** Presety nie wskazują generatora, więc CMake używa domyślnego
  na macOS, czyli Unix Makefiles. Tak był robiony zweryfikowany build.
- Skąd wymóg 3.24: `cmake_minimum_required(VERSION 3.24)` w
  [`CMakeLists.txt`](../../CMakeLists.txt) i `cmakeMinimumRequired` w
  [`CMakePresets.json`](../../CMakePresets.json).
- Bibliotek (GLFW, GLM, ImGui, doctest, stb_image, GLAD) nie instalujemy ręcznie. GLFW, GLM,
  ImGui, doctest i stb pobiera CMake, GLAD leży w repozytorium.
- Pierwsza konfiguracja wymaga dostępu do internetu.

## 2. Budowanie i uruchamianie

Wszystkie polecenia wykonujemy w katalogu głównym repozytorium.

### Debug

```sh
cmake --preset debug
cmake --build --preset debug
./build/debug/night_maze
```

### Release

```sh
cmake --preset release
cmake --build --preset release
./build/release/night_maze
```

Co robią kolejne kroki:

1. `cmake --preset debug` to **konfiguracja (configure)**. CMake czyta `CMakeLists.txt`,
   wykrywa kompilator, pobiera zależności i generuje pliki buildu (Makefile) w `build/debug`.
   Ten krok powtarzamy tylko po zmianie plików CMake (na przykład po dodaniu nowego pliku
   źródłowego do listy). Jeśli o tym zapomnimy, następny krok i tak sam wykryje zmianę w
   `CMakeLists.txt` i uruchomi konfigurację ponownie.
2. `cmake --build --preset debug` to **budowanie (build)**. Uruchamia `make`, który kompiluje
   tylko zmienione pliki i linkuje program. Po linkowaniu tworzy obok programu dowiązanie
   `build/debug/assets` do katalogu `assets/` z repozytorium (opis niżej, "Katalog `assets`").
3. `./build/debug/night_maze` uruchamia program.

Budowanie na wielu rdzeniach (domyślny `make` kompiluje po jednym pliku naraz):

```sh
cmake --build --preset debug --parallel
```

Po poprawnym starcie w terminalu pojawiają się dwie linie z `core::Window`:

```text
[info] GL_VERSION:  4.1 Metal - 90.5
[info] GL_RENDERER: Apple M3
```

**Co było widać na Macu w stanie M1 (zmierzone):** ciemnogranatowe tło, na środku kostka
obrócona tak, że widać trzy jej ściany (czerwoną z przodu, niebieską z lewej i turkusową u
góry, każda w jednolitym kolorze), a na wierzchu panele "Renderer", "Shaders" i "Camera".
Po kliknięciu w scenę kursor znikał i kamerą można było latać wokół kostki.

**Co powinno być widać dziś, po M2 + M3 i oświetleniu z M4 (na macOS niesprawdzone, na
Windowsie zmierzone 2026-10-05):** nocny widok z wnętrza labiryntu 10 na 10 z teksturą
kamienia na podłodze, ścianach i słupkach, oświetlony w trybie Blinn-Phong: słabe, chłodne
światło księżyca, ciepły stożek latarki na środku obrazu i turkusowe światła punktowe w
ślepych zaułkach, każde oznaczone małą świecącą kostką. Tło jest prawie czarne, granatowe
(ciemniejsze niż w stanie M1). Cieni nie ma. Paneli jest siedem: "Renderer" nad "Lights" w
lewej kolumnie, "Maze" nad "Assets" w prawej, "Collision" i "Shaders" na dole między
kolumnami, a "Camera" u góry, zwinięty do paska tytułu. Kostka z M1 wisi nad komórką w rogu
przeciwległym do startu. Po dwóch liniach z `core::Window` pamięć podręczna assetów wypisuje
linie `[info] Loaded texture: ...` i `[info] Loaded model: ...`. Po kliknięciu w scenę
kursor znika i gracz chodzi po labiryncie (tabela niżej). Linia `[error] Shader ...` w
terminalu oznacza, że shader się nie wczytał: to, co rysuje ten program shaderów, znika, a
reszta klatki jest rysowana dalej ([`../modules/gfx/shaders.md`](../modules/gfx/shaders.md)).

`4.1` potwierdza, że dostaliśmy kontekst, o który prosiliśmy. `Metal` oznacza, że OpenGL na
Apple Silicon jest warstwą zbudowaną nad Metalem. Druga linia zależy od procesora w danym
Macu.

### Sterowanie

| Klawisz albo mysz | Działanie | Gdzie w kodzie |
|---|---|---|
| lewy przycisk myszy w scenie | przechwytuje kursor (kursor znika) i włącza sterowanie | `NightMazeApp::onRender` w [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) |
| ruch myszy przy przechwyconym kursorze | obraca kamerę | tamże |
| W, S, A, D przy przechwyconym kursorze | chodzenie: do przodu, do tyłu, w lewo, w prawo, zawsze poziomo, z kolizjami | `NightMazeApp::onUpdate` zbiera klawisze, ruch liczy `Player::update` w [`src/game/Player.cpp`](../../src/game/Player.cpp) |
| lewy Shift przy przechwyconym kursorze | chodzenie: sprint. W trybie noclip: w dół | tamże |
| spacja przy przechwyconym kursorze | tylko w trybie noclip: w górę | tamże |
| N | przełącza chodzenie i noclip (lot wzdłuż kierunku patrzenia, bez kolizji). Działa także przy wolnym kursorze | `NightMazeApp::onRender` |
| F | włącza i wyłącza latarkę. Działa także przy wolnym kursorze. To samo robi pole `Flashlight on (key F)` w panelu Lights | `NightMazeApp::onRender` |
| Esc | przy przechwyconym kursorze oddaje kursor, przy wolnym zamyka program | `Application::run` w [`src/core/Application.cpp`](../../src/core/Application.cpp) |
| `~` (na lewo od `1`, `GLFW_KEY_GRAVE_ACCENT`) | pokazuje lub ukrywa interfejs debugowy | `DebugNightMazeApp::onRender` w [`src/main.cpp`](../../src/main.cpp) |

Klawisze są ignorowane, dopóki aktywny jest widżet panelu ImGui (na przykład trwa
wpisywanie wartości): klawiatura należy wtedy do panelu. Opis w
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6. Kliknięcie w panel nie
przechwytuje kursora, a przy przechwyconym kursorze panele nie reagują na mysz: żeby
przesunąć suwak, trzeba najpierw nacisnąć Esc. Obrót kamery opisuje
[`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcje 5
i 6, a ruch gracza [`../modules/game/player.md`](../modules/game/player.md). Obrót myszą i
lot były na Macu sprawdzone ręcznie w stanie M1. Chodzenia z kolizjami, sprintu oraz klawiszy N
i F nikt na Macu nie sprawdzał. Na macOS GLFW 3.4 nie ma surowego ruchu myszy, więc obrót korzysta z ruchu kursora po
przyspieszeniu systemowym ([`../modules/core/input.md`](../modules/core/input.md), sekcja
2.7).

Panel "Renderer" pokazuje FPS, czas klatki, rozmiar framebuffera i okna, wersję OpenGL,
nazwę karty, edytor koloru tła oraz listę `Lighting` z czterema trybami cieniowania
(`Unlit`, `Gouraud`, `Phong`, `Blinn-Phong`). Panel można przeciągnąć do krawędzi okna
(docking).

### Katalog `assets` i praca z shaderami

Program wczytuje shadery, modele i tekstury z katalogu `assets` leżącego **obok pliku wykonywalnego**, czyli z
`build/debug/assets` ([`../modules/core/paths.md`](../modules/core/paths.md)). Na macOS ten
katalog jest dowiązaniem symbolicznym, które build tworzy po zlinkowaniu programu:

```sh
ls -l build/debug/assets
# build/debug/assets -> /Users/<nazwa>/.../night-maze/assets
```

Skutki praktyczne:

- Plik shadera edytuję w `assets/shaders/` w repozytorium. Program widzi zmianę przy
  następnym wczytaniu, **bez budowania**: po kliknięciu "Reload shaders" w panelu Shaders
  albo po ponownym uruchomieniu `./build/debug/night_maze`. Dotyczy to także pliku
  dołączanego dyrektywą `#include` (`assets/shaders/common/lighting.glsl`): każde wczytanie
  shadera czyta go od nowa. Modele i tekstury są wczytywane tylko raz, przy starcie.
- Program działa uruchomiony z dowolnego katalogu roboczego, bo ścieżka do shaderów nie
  zależy od katalogu roboczego.
- `make clean` (albo `rm -rf build`) usuwa dowiązanie, a nie pliki w `assets/`. Następny
  build tworzy je ponownie.
- Dowiązanie ma ścieżkę bezwzględną. Po przeniesieniu repozytorium w inne miejsce trzeba
  zbudować program od nowa (`make clean`, potem `make debug`).
- Na Windowsie w tym miejscu jest kopia, a nie dowiązanie. Odświeża ją target `copy_assets`
  ([`build-windows.md`](build-windows.md), sekcja 7).

Co robi każda linia kroku CMake: [`project-structure.md`](project-structure.md), sekcja 3.1,
blok 7.

### Testy jednostkowe

> **Na macOS jeszcze nie uruchomione.** Kod kolizji, labiryntu, gracza i loaderów oraz jego
> testy powstały na Windowsie (2026-10-05), a testy oświetlenia dzień później. Tam są
> zmierzone: [`build-windows.md`](build-windows.md), sekcja 2. Wszystko w tym podrozdziale
> jest dla Maca oczekiwaniem, nie pomiarem.

Zwykły build (`cmake --build --preset debug`) buduje też program testowy
`build/debug/night_maze_tests`. Testy uruchamia `ctest`, program z pakietu CMake:

```sh
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```

Argument `-C` jest potrzebny generatorowi Visual Studio na Windowsie. Generator Unix
Makefiles go ignoruje, więc polecenie jest wspólne dla obu systemów. Program testowy można
też uruchomić wprost, wtedy widać raport biblioteki doctest:

```sh
./build/debug/night_maze_tests
```

Oczekiwany koniec wyjścia. Liczby z Windowsa, zmierzone tam w konfiguracjach Debug i Release
2026-10-05, dla trzynastu plików z testami: `ColliderTests.cpp` 12 przypadków,
`ImageLoaderTests.cpp` 9, `LightingTests.cpp` 17, `LightTests.cpp` 20,
`MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12, `MazeTests.cpp` 6,
`MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 20, `PlayerTests.cpp` 13,
`ShaderSourceTests.cpp` 22, `TangentTests.cpp` 9 i `TransformTests.cpp` 4:

```text
[doctest] test cases:   163 |   163 passed | 0 failed | 0 skipped
[doctest] assertions: 62220 | 62220 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Przed oświetleniem (2026-10-05) program miał osiem plików z testami, a przed mapami
normalnych dwanaście, i odpowiednio mniej przypadków.

Nad tym raportem program wypisuje kilka linii `[error]`: pochodzą z testów, które celowo
podają loaderom zły plik, i nie oznaczają nieudanego testu.

Opis biblioteki, makr i opcji programu: [`../libraries/doctest.md`](../libraries/doctest.md).

**Do zrobienia przy pierwszym buildzie tego kodu na Macu** (punkty otwarte, nikt ich jeszcze
nie wykonał):

- [ ] `cmake --preset debug` pobiera doctest `v2.5.3` do `build/debug/_deps/doctest-src` i
      kończy się bez błędów (doctest deklaruje `cmake_minimum_required(VERSION 3.14)`, więc
      CMake 4 nie powinien go odrzucić)
- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic` w nowych plikach: `src/scene/Collider.*`, `src/game/Maze*`
      i `tests/*.cpp`. Kompilator Apple clang z biblioteką libc++ nie widział jeszcze tego
      kodu. Na Windowsie diagnostyki kompilatora clang 19 (przez clang-tidy, z biblioteką
      standardową MSVC) nie zgłaszają w nim niczego
      ([`build-windows.md`](build-windows.md), sekcja 11), ale to inna biblioteka standardowa
- [ ] nagłówek doctest trafia do kompilatora przez `-isystem` i nie daje ostrzeżeń w plikach
      testów
- [ ] `ctest --test-dir build/debug -C Debug --output-on-failure` i to samo dla Release:
      zapisać liczbę przypadków i asercji (oczekiwane dla całego programu: 163 przypadki i
      62220 asercji, liczby z Windowsa z 2026-10-05)
- [ ] **najważniejszy punkt**: przechodzą testy `golden maze: 4 x 4 cells from seed 1 has
      exactly these walls` i `randomBelow gives the same numbers on every system`. To jest
      pomiar, że macOS i Windows generują ten sam labirynt
      ([`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5.8)
- [ ] przechodzi test `a box wandering through a generated maze never ends up inside a wall`
      (wynik zależy od zaokrągleń `float`, które mogą się różnić między procesorami na
      ostatniej cyfrze: test ma na to zapas, ale to pierwsze uruchomienie na ARM)
- [ ] `make test`, `make test-release` i `make check` działają (cele są nowe, plik `Makefile`
      w tej postaci nie był jeszcze uruchamiany)
- [ ] `make tidy` nie zgłasza niczego w `src/scene/Collider.cpp`, `src/game/Maze*.cpp` ani w
      `tests/*.cpp` (na Windowsie LLVM 19.1.5 nie zgłasza niczego w tych plikach)
- [ ] program `./build/debug/night_maze` buduje się i startuje w labiryncie (lista "M2 + M3
      na macOS" niżej)

**Loader OBJ i siatka (temat 4): do zrobienia przy pierwszym buildzie tego kodu na Macu.**
Kod powstał na Windowsie (2026-10-05) i tam jest zmierzony: dziś, razem z testami linii
`map_Bump` i stycznych, 20 przypadków testowych i 1576 asercji w `tests/ObjLoaderTests.cpp`.
Na Macu nikt go jeszcze nie kompilował:

- [ ] `src/assets/ObjLoader.*`, `src/gfx/Vertex.hpp`, `src/gfx/Mesh.*` i
      `tests/ObjLoaderTests.cpp` kompilują się bez ostrzeżeń pod `-Wall -Wextra -Wpedantic`
- [ ] przechodzą trzy asercje czasu kompilacji: `sizeof(gfx::Vertex)` równe 11 liczbom
      `float` (44 bajty, od M4 z polem `tangent`) i
      `std::is_standard_layout_v<gfx::Vertex>` w `Vertex.hpp` oraz
      `std::is_same_v<GLuint, std::uint32_t>` w `Mesh.cpp`
- [ ] `./build/debug/night_maze_tests --source-file='*ObjLoaderTests*'`: zapisać liczbę
      przypadków i asercji (oczekiwane 20 i 1576)
- [ ] przechodzi przypadek `parseObj: numbers` i podprzypadki z błędnymi liczbami w
      `parseObj: a bad line is reported with its line number`. Liczby czyta
      `std::istringstream` z klasycznym locale, a libc++ może traktować teksty graniczne
      (`+2`, `2.5E2`, `1.5x`, `--1`) inaczej niż biblioteka MSVC
- [ ] przechodzą trzy przypadki `loadObj: wall_straight.obj`, `wall_pillar.obj` i
      `floor_tile.obj`: ścieżka z `NIGHT_MAZE_ASSETS_DIR` i ścieżki tekstur po
      `lexically_normal()` porównują się poprawnie także z separatorem `/`
- [ ] przechodzi przypadek `loadObj: material libraries and texture paths of files written by
      the test` (zapis do katalogu tymczasowego systemu i sprzątanie po sobie)
- [ ] sprawdzić, czy `std::from_chars` dla `float` kompiluje się Apple clangiem przy
      domyślnej wersji docelowej systemu. Jeśli tak, `parseFloat` w `ObjLoader.cpp` można
      uprościć. Dziś używa strumienia właśnie dlatego, że tego nie sprawdziłem
      ([`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), sekcja 5.4)

**Tekstury (temat 5): do zrobienia przy pierwszym buildzie tego kodu na Macu.** Kod powstał
na Windowsie (2026-10-05) i tam jest zmierzony: dziś, razem z testami map normalnych, 9
przypadków testowych i 57 asercji w `tests/ImageLoaderTests.cpp` oraz program z ukrytym oknem dla klasy `gfx::Texture2D`
([`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 5.9). Na Macu nikt go
jeszcze nie kompilował:

- [ ] `cmake --preset debug` pobiera repozytorium stb w commicie
      `2c980bb59875b0d32144a71867fbdebb2f77cd20` do `build/debug/_deps/stb-src` (pełny
      klon, bez `GIT_SHALLOW`), a pierwsza linia `stb_image.h` to `stb_image - v2.30`.
      Repozytorium stb nie ma pliku `CMakeLists.txt`, więc CMake 4 nie ma czego odrzucić,
      ale to wniosek, nie pomiar
- [ ] `external/stb/stb_image.c` kompiluje się Apple clangiem jako C. Zapisać, czy daje
      ostrzeżenia: ten plik celowo nie dostaje `-Wall -Wextra -Wpedantic`, więc nawet jeśli
      są, nie przerywają buildu. Na Windowsie nie ma żadnych
- [ ] `stb_image.h` trafia do kompilacji `src/assets/ImageLoader.cpp` przez `-isystem` i
      nie daje ostrzeżeń w naszym pliku
- [ ] `src/assets/ImageLoader.*`, `src/gfx/Texture2D.*`, `src/gfx/Shader.*` i
      `tests/ImageLoaderTests.cpp` kompilują się bez ostrzeżeń pod `-Wall -Wextra
      -Wpedantic`. Miejsca, których Apple clang z libc++ jeszcze nie widział: wypełnienie
      `std::vector<unsigned char>` z `std::istreambuf_iterator<char>` w `readBinaryFile`,
      ścieżka z literału `u8"..."` (typ `char8_t`) w teście i `reinterpret_cast` wyniku
      `glGetStringi` w `Texture2D.cpp`
- [ ] `./build/debug/night_maze_tests --source-file='*ImageLoaderTests*'`: zapisać liczbę
      przypadków i asercji (oczekiwane 9 i 57)
- [ ] przechodzą przypadki `the rows are flipped: ...` i `a path with letters outside ASCII
      can be loaded`: oba zapisują plik do katalogu tymczasowego systemu
      (`std::filesystem::temp_directory_path()`) i usuwają go po sobie. Drugi tworzy plik o
      nazwie z polskimi literami i znakiem japońskim
- [ ] klasa `gfx::Texture2D` na sterowniku Apple: czy na liście rozszerzeń jest
      `GL_EXT_texture_filter_anisotropic`, jaką wartość ma `maxAnisotropy()` i czy
      konstruktor nie zostawia błędu w `glGetError`. Do sprawdzenia w grze: suwak
      `Anisotropy` w panelu Assets pokazuje maksimum sterownika albo jest wyszarzony (lista
      "M2 + M3 na macOS" niżej)
- [ ] powtórzyć pomiar pasów z [`../modules/gfx/textures.md`](../modules/gfx/textures.md),
      sekcja 5.9: czy poziom anizotropii ustawiony na obiekcie samplera zmienia obraz (na
      Windowsie: szary przy 1, czarne i białe pasy przy 16) i czy ustawiony przez
      `glTexParameterf` na samej teksturze też działa (na Windowsie nie działał)
- [ ] tekstura na ścianie nie jest do góry nogami ani pochylona, a `texture()` w shaderze
      `#version 410 core` kompiluje się na sterowniku Apple (tekstury są już wpięte w grę,
      lista niżej)

Po wykonaniu punkty trzeba odhaczyć i dopisać wynik, tak jak na liście w
[`build-windows.md`](build-windows.md), sekcja 11.

### M2 + M3 na macOS: lista w całości otwarta

Krok, który łączy kolizje, labirynt, loadery, siatkę i tekstury w działającą grę, powstał na
Windowsie i tam jest zbudowany i częściowo sprawdzony
([`build-windows.md`](build-windows.md), sekcja 12). **Na macOS nikt go nie zbudował ani nie
uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Oczekiwania wynikają z kodu i z
pomiarów na Windowsie. Opis kodu: [`../modules/game/player.md`](../modules/game/player.md),
[`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md),
[`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md),
[`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) (panel Maze),
[`../modules/gfx/textures.md`](../modules/gfx/textures.md) (shadery `textured`).

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/assets/AssetCache.*`, `src/game/Player.*`, `MazeWorld.*`, `MazeRenderer.*`,
      `ColliderLines.*`, `ShaderUniforms.hpp`, `src/debug/panels/MazePanel.*`,
      `CollisionPanel.*`, `AssetsPanel.*`, `tests/PlayerTests.cpp`,
      `tests/MazeWorldTests.cpp`. Miejsca warte uwagi: inicjalizatory desygnowane, które
      wypełniają tylko część pól (`gfx::Vertex{.position = ...}` w `ColliderLines.cpp`,
      `PlayerInput{.forward = true}` w testach, `LoadedModel{...}` w `AssetCache.cpp`),
      `constexpr glm::vec3` i `constexpr std::array<gfx::Vertex, ...>`,
      `std::ranges::find` w `AssetCache.cpp`, `constexpr std::span<const scene::Aabb>` w
      `PlayerTests.cpp`, `static_cast<ImTextureID>` z `GLuint` w `AssetsPanel.cpp`. Zapisać
      każde ostrzeżenie
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 163 przypadki testowe i
      62220 asercji, `Status: SUCCESS!` (liczby z Windowsa z 2026-10-05, razem z testami
      oświetlenia i map normalnych)
- [ ] przechodzą nowe przypadki zależne od zaokrągleń `float`: `a player wandering through a
      closed maze never leaves it or enters a wall` i `a player pressing into a wall slides
      along it and past the pillars` z `tests/PlayerTests.cpp` oraz `a box that hugs a wall
      slides past the pillars in the middle of it` z `tests/MazeLayoutTests.cpp`. To pierwsze
      uruchomienie na ARM
- [ ] `make check` (format, oba buildy z testami, clang-tidy) przechodzi

**Ryzyka specyficzne dla macOS**

- [ ] **ścisły kompilator GLSL Apple** przyjmuje cztery pliki z M2 + M3: `textured.vert`,
      `textured.frag`, `color.vert`, `color.frag`. Po starcie w terminalu nie ma linii
      `[error] Shader ...`, a panel Shaders pokazuje linie
      `textured.vert + textured.frag: OK` i `color.vert + color.frag: OK` (pięć nowych
      plików oświetlenia jest na liście M4 niżej). Sterownik NVIDII na Windowsie przyjmuje je bez uwag, ale jest
      łagodniejszy. Miejsca, na które sterownik Apple mógłby zareagować: `mat3(uModel)`,
      `fract(vUv)` jako argument konstruktora `vec4`, porównania `uViewMode == 1` dla
      uniformu `int`, wejście `vNormal` używane tylko w jednej gałęzi `if`
- [ ] **obiekty samplera** (`glGenSamplers`, `glSamplerParameteri`, `glBindSampler`, rdzeń
      OpenGL od 3.3): tekstury na ścianach powtarzają się (zawijanie `GL_REPEAT` ustawione
      na samplerze) i reagują na listę `Filter`. Backend ImGui wiąże na czas rysowania
      paneli własny sampler: po klatce z otwartym panelem Assets tekstury w scenie nadal
      mają wybrany filtr
- [ ] **wyszukanie rozszerzenia anizotropii**: `gfx::Texture2D` szuka nazwy
      `GL_EXT_texture_filter_anisotropic` (albo `GL_ARB_texture_filter_anisotropic`) na
      liście z `glGetStringi`. Zapisać, co pokazuje suwak `Anisotropy` w panelu Assets:
      zakres od 1x do maksimum sterownika, albo suwak wyszarzony z napisem `Anisotropic
      filtering is not offered by this graphics driver.` Oba wyniki są poprawne. Błędem
      byłaby linia `[error]` z `GL_INVALID_ENUM`
- [ ] **Retina a układ paneli**: panel Renderer pokazuje `Framebuffer` dwa razy większy niż
      `Window` (dla okna 1280 x 720 oczekiwane 2560 x 1440). Miejsca startowe paneli
      (`src/debug/PanelLayout.hpp`) są w jednostkach okna i ułożone dla 1280 x 720, więc po
      usunięciu `imgui.ini` siedem paneli powinno stać tak samo jak na Windowsie i nie
      zasłaniać się (panel Camera zwinięty do paska tytułu). Zapisać, czy plan w panelu Maze i podglądy tekstur w panelu Assets mają
      poprawny rozmiar i ostrość
- [ ] **budowanie motywu pod clang**: `src/debug/Theme.cpp` i `src/debug/PanelLayout.cpp`
      kompilują się z `-Wall -Wextra -Wpedantic` bez ostrzeżeń. Na Windowsie są zbudowane w
      MSVC, a dodatkowo dziewięć plików `.cpp` z `src/debug/` przeszło bez żadnej
      diagnostyki przez analizator składni clang z tymi trzema flagami (clang-tidy z
      samymi diagnostykami kompilatora, w trybie zgodności z MSVC i z biblioteką
      standardową Microsoftu). To nie to samo co Apple clang z libc++, więc punkt zostaje
      otwarty. Miejsca do sprawdzenia: inicjalizatory desygnowane z zagnieżdżonymi klamrami
      w stałych `..._PLACEMENT`, `std::min` z listą w klamrach, funkcja `constexpr`
      `colorFromBytes` w nagłówku
- [ ] **czcionka paneli**: tekst paneli jest w czcionce Atkinson Hyperlegible (zero jest
      przekreślone, litery są proporcjonalne), a w terminalu nie ma linii `[error] Panel
      font cannot be loaded`. Na macOS katalog `assets` obok programu jest dowiązaniem, więc
      plik jest czytany wprost z `assets/fonts/` w repozytorium
- [ ] **ostrość tekstu na Retinie**: czcionka ma być ostra, a nie rozciągnięta z małej
      tekstury. Według źródeł ImGui 1.92 znaki są rysowane w gęstości framebuffera bez
      mojego kodu. Nikt tego nie oglądał
- [ ] **skala paneli na Retinie**: panele mają taki sam rozmiar w punktach jak na Windowsie
      przy 100% (panel Renderer szeroki na około jedną czwartą okna 1280 x 720), a nie dwa
      razy większy. Funkcja `ImGui_ImplGlfw_GetContentScaleForWindow` powinna zwrócić na
      Macu 1 ([`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8.4). Dwa razy
      za duże panele oznaczałyby, że to założenie jest fałszywe
- [ ] **polskie litery w panelu**: uruchomić program z kopii katalogu `build/debug` w
      katalogu o nazwie z polskimi literami i najechać myszą na linię
      `basic.vert + basic.frag: OK` w panelu Shaders. Oczekiwane: podpowiedź z dwiema
      pełnymi ścieżkami i poprawnymi polskimi literami. Na
      Windowsie zmierzone jest tylko to, że czcionka te litery rysuje
- [ ] **kolory motywu**: tło paneli granatowe i lekko przezroczyste, tekst jasny, tekst
      błędu w panelu Shaders czytelny. Kontrast jest policzony z liczb, ale ekran Maca ma
      inny profil kolorów niż monitor, na którym motyw był oglądany
- [ ] **linie pudełek kolizji mają 1 piksel framebuffera**: szerokość linii zostaje domyślna,
      bo profil Core na macOS nie obsługuje grubszych. Na ekranie Retina to połowa punktu.
      Zapisać, czy żółte i zielone linie są czytelne
- [ ] **ten sam labirynt co na Windowsie, widziany w grze**: w panelu Maze ustawić `Width` 4,
      `Height` 4, `Seed` 1 i kliknąć `Regenerate`. Plan musi mieć ściany tak jak rysunek w
      teście `golden maze: 4 x 4 cells from seed 1 has exactly these walls` w
      [`tests/MazeGeneratorTests.cpp`](../../tests/MazeGeneratorTests.cpp):

      ```text
      +--+--+--+--+
      |  |        |
      +  +  +--+  +
      |  |     |  |
      +  +--+  +--+
      |     |     |
      +--+  +--+  +
      |           |
      +--+--+--+--+
      ```

      Linie `Walls: 25, pillars: 25`, kropka gracza w lewej górnej komórce, kreska w dół
      (z rysunku: komórka startowa ma ścianę od wschodu i przejście na południe, więc `Yaw`
      wynosi 180). Sam test jednostkowy mierzy to samo bez okna, ten punkt sprawdza całą
      drogę od ziarna do ekranu
- [ ] labirynt startowy (10 na 10, ziarno 1): zapisać `Yaw` z panelu Camera i porównać z
      Windowsem (tam 180 według autora kodu)

**Test ręczny (ta sama lista co w [`build-windows.md`](build-windows.md), sekcja 12.2)**

Na macOS katalog `assets` obok programu jest dowiązaniem, więc tam, gdzie lista dla Windowsa
każe odświeżyć kopię (`--target copy_assets`), na Macu nie trzeba robić nic. Lista powstała
przed oświetleniem. Dziś scena jest nocna: lista `Lighting` w panelu Renderer ustawiona na
`Unlit` daje równo jasny labirynt, wygodny do oglądania tekstur, filtrów i kolizji, a panel
Camera trzeba najpierw rozwinąć strzałką w pasku tytułu.

- [ ] przygotowanie: usunąć `imgui.ini` z katalogu, z którego startuje program, zbudować,
      uruchomić `./build/debug/night_maze`
- [ ] start: widok z wnętrza labiryntu, tekstury stoją prosto i nie są odbite lustrzanie,
      w terminalu nie ma linii `[error]`
- [ ] okno 1280 x 720: siedem paneli nie zasłania się nawzajem (Renderer nad Lights po
      lewej, Maze nad Assets po prawej, Collision i Shaders na dole między kolumnami,
      Camera zwinięty u góry). Zapisać, czy panele Renderer, Lights, Maze, Collision i
      Shaders pokazują całą zawartość bez przewijania: nazwa karty Apple w panelu Renderer
      ma inną długość niż na Windowsie, a wysokości paneli są dobrane do zawartości
- [ ] panel Camera (rozwinąć strzałką w pasku tytułu): `Mode: walking`, `Player feet` 1, 0, 1, `Eye: 1.00, 1.70, 1.00`,
      `Pitch` 0, `Walk speed` 3.0, `Sprint speed` 5.5, `Fly speed` 6.0
- [ ] panele Maze i Collision: `In play: 10 x 10 cells, seed 1`, `Walls: 121, pillars: 121`,
      `Wall boxes: 121`, `Pillar boxes: 121`, `All boxes: 242`
- [ ] chodzenie: po kliknięciu w scenę W, A, S, D chodzą poziomo, także ze wzrokiem w
      podłodze, lewy Shift przyspiesza
- [ ] ściana zatrzymuje gracza, a ruch ukosem w ścianę zamienia się w ślizganie wzdłuż niej
- [ ] gracz przytulony do długiej ściany nie zahacza o słupki stojące co 2 m
- [ ] narożnik wewnętrzny zatrzymuje, narożnik zewnętrzny daje się obejść bez zacięcia
- [ ] nie da się wyjść poza labirynt
- [ ] klawisz N: `Mode: noclip (free flight)`, pole `Noclip (key N)` w panelu Collision jest
      zaznaczone, spacja wznosi, lewy Shift opuszcza, W leci wzdłuż kierunku patrzenia przez
      ściany
- [ ] widok z góry w trybie noclip zgadza się z planem w panelu Maze, nad rogiem
      przeciwległym do startu wisi kostka
- [ ] drugi raz N w powietrzu: gracz od razu stoi na podłodze
- [ ] `Draw collision boxes`: żółte linie na ścianach i słupkach, zielone pudełko gracza
      widoczne pod nogami i nad głową, linie nie migoczą
- [ ] `Regenerate` z innym rozmiarem i ziarnem: nowy plan, gracz na starcie (`Player feet`
      1, 0, 1, `Pitch` 0), a tryb noclip, prędkości, tryb widoku i rysowanie pudełek zostają
      bez zmian
- [ ] `Random seed`: nowa liczba w polu `Seed` i od razu nowy labirynt
- [ ] panel Assets, `View mode`: `Normals as colour` (podłoga jasnozielona, ściany w
      kolorach zależnych od kierunku, a przy zaznaczonym polu `Normal mapping` i trybie
      `Lighting` innym niż `Gouraud` z rysunkiem fug z map normalnych), `UVs as colour` (czerwono-zielone powtarzające się
      przejścia), `Textured` przywraca obraz
- [ ] panel Assets, `Filter`: `Nearest` (kwadratowe teksele z bliska, migotanie w oddali),
      `Bilinear` (gładko z bliska, migotanie w oddali), `Trilinear` (spokojnie w oddali)
- [ ] panel Assets, `Anisotropy` (jeśli dostępna): większa wartość wyostrza podłogę widzianą
      pod płaskim kątem
- [ ] podglądy tekstur w panelu Assets stoją prosto i nie reagują na filtr
- [ ] `Reload shaders` po zmianie w `assets/shaders/textured.frag` (na przykład
      `fragColor = vec4(texel * uTint * vec3(1.0, 0.5, 0.5), 1.0);`), przy `Lighting`
      ustawionym na `Unlit` (w pozostałych trybach labirynt rysują programy `lit` albo
      `gouraud`): labirynt robi się czerwonawy, wszystkie pięć linii panelu kończy się
      napisem `: OK`. Z błędem składni: czerwona linia
      `textured.vert + textured.frag: FAILED, the previous program stays in use` z
      komunikatem sterownika pod nią, labirynt rysuje się poprzednią wersją. Przywrócić
      plik
- [ ] celowo brakująca tekstura: zamknąć program, zmienić nazwę
      `assets/textures/floor_stone.png` (na Macu to plik w repozytorium, bo `assets` obok
      programu jest dowiązaniem), uruchomić. Oczekiwane: podłoga bez rysunku kamienia
      (biała w trybie `Unlit`, w kolorze padającego światła w trybach z oświetleniem, przy
      `Phong` i `Blinn-Phong` nadal z reliefem fug, bo mapa normalnych wczytuje się osobno), jedna
      linia `[error]`, w panelu Assets `no texture (white)` i sekcja `Failed to load`. **Przywrócić
      nazwę pliku** i sprawdzić `git status`
- [ ] zmiana rozmiaru okna, tryb pełnoekranowy macOS i powrót: obraz wypełnia okno, płytki
      podłogi zostają kwadratowe, bez linii `[error]`
- [ ] okno powiększone na cały ekran po usunięciu `imgui.ini`: zapisać, gdzie stoją panele.
      Układ startowy jest liczony raz, w pierwszej klatce, od rogów okna w tej chwili, więc
      po późniejszym powiększeniu panele zostają na miejscach dla 1280 x 720

### M4 (oświetlenie) na macOS: lista w całości otwarta

Druga część M4, mapy normalnych, ma własną listę zaraz po tej. Pierwsza część M4 (światła, cztery tryby cieniowania, blok uniformów, `#include` w
shaderach, panel Lights, układ siedmiu paneli) powstała na Windowsie 2026-10-05 i tam jest
zbudowana i częściowo sprawdzona ([`build-windows.md`](build-windows.md), sekcja 13).
**Na macOS nikt jej nie zbudował ani nie uruchomił, więc żaden punkt poniżej nie jest
odhaczony.** Oczekiwania wynikają z kodu i z pomiarów na Windowsie. Opis kodu:
[`../modules/scene/lights.md`](../modules/scene/lights.md),
[`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md),
[`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md),
[`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md),
[`../modules/game/flashlight.md`](../modules/game/flashlight.md).

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/gfx/ShaderSource.*`, `src/gfx/UniformBuffer.*`, `src/scene/Light.*`,
      `src/scene/LightBlock.*`, `src/game/Lighting.*`, `src/game/LightRig.*`,
      `src/debug/panels/LightsPanel.*` i cztery pliki testów. Zmienione:
      `src/gfx/Shader.*`, `src/scene/Transform.*`, `src/game/MazeWorld.*`,
      `MazeRenderer.*`, `NightMazeApp.*`, `ShaderUniforms.hpp`, `src/main.cpp`,
      `src/debug/DebugContext.hpp`, `DebugUI.cpp`, `PanelLayout.*` oraz panele
      `RendererPanel.*` i `ShadersPanel.cpp`. Miejsca warte uwagi:
      inicjalizatory desygnowane, które wypełniają tylko część pól
      (`gfx::Vertex{.position = ...}` w `LightRig.cpp`, stałe `..._PLACEMENT` bez pola
      `collapsed` w `PanelLayout.hpp`), `std::function` jako typ `gfx::IncludeReader`,
      `std::span<const std::string>`, tablica `std::array<std::int32_t, 3>` jako
      wypełnienie w `LightBlockData`. Zapisać każde ostrzeżenie
- [ ] **`offsetof` wewnątrz `static_assert` pod Apple clang**:
      [`src/scene/LightBlock.hpp`](../../src/scene/LightBlock.hpp) sprawdza układ bloku
      świateł piętnastoma liniami postaci `static_assert(offsetof(...) == ...)` (trzy dla
      `PointLightData`, dwanaście dla `LightBlockData`) i liniami z `sizeof` (16, 48 i 928
      bajtów). Oczekiwane: plik kompiluje się bez
      błędu i bez ostrzeżenia, bo `offsetof` z `<cstddef>` jest w clangu wyrażeniem stałym,
      a obie struktury mają układ standardowy. Kłopot z `offsetof` na Windowsie dotyczył
      nagłówków biblioteki C Microsoftu czytanych przez clang, których na Macu nie ma.
      Niesprawdzone
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 163 przypadki testowe i
      62220 asercji, `Status: SUCCESS!` (liczby z Windowsa, razem z testami map
      normalnych)
- [ ] cztery nowe pliki testów osobno, opcją `--source-file`: `'*ShaderSourceTests*'` 22
      przypadki, `'*LightTests*'` 20, `'*LightingTests*'` 17, `'*TransformTests*'` 4
      (liczby przypadków z Windowsa). Uwaga: wzorzec `'*LightTests*'` nie pasuje do
      `LightingTests.cpp`, a `'*Light*'` pasuje do obu plików
- [ ] przechodzą przypadki zależne od zaokrągleń `float` na ARM: `a light made for a radius
      has 5 % of its brightness left at that radius`, `directionFromAngles gives a vector
      of length 1` i cztery przypadki macierzy normalnych z `tests/TransformTests.cpp`
      (tolerancja 0,0001)
- [ ] przechodzi przypadek `the default maze has this many point lights` (11 świateł w
      labiryncie 10 na 10 z ziarna 1): zależy od tego samego generatora co labirynt
      wzorcowy, więc jest drugim pomiarem, że oba systemy budują ten sam labirynt
- [ ] `make check` (format, oba buildy z testami, clang-tidy) przechodzi

**Ryzyka specyficzne dla macOS**

- [ ] **kompilator GLSL Apple przyjmuje pięć nowych plików**: `lit.vert`, `lit.frag`,
      `gouraud.vert`, `gouraud.frag` i dołączany `common/lighting.glsl`. Po starcie w
      terminalu nie ma linii `[error] Shader ...`, a panel Shaders pokazuje
      `lit.vert + lit.frag: OK` i `gouraud.vert + gouraud.frag: OK`. Sterownik NVIDII
      przyjmuje je bez uwag, ale jest łagodniejszy. Miejsca, na które sterownik Apple
      mógłby zareagować: blok `layout(std140) uniform LightBlock` z tablicą struktur
      (`PointLight uPoints[MAX_POINT_LIGHTS]`), ten sam blok użyty raz w shaderze
      fragmentów (`lit.frag`), a raz w shaderze wierzchołków (`gouraud.vert`), parametr
      `inout Lighting` (struktura) w funkcji `addLight`, stała `const int` jako długość
      tablicy, uniform `mat3 uNormalMatrix`, wywołania `reflect` i `pow`
- [ ] **dyrektywy `#line` z numerem napisu źródłowego**: program dopisuje wokół
      dołączonego pliku linie `#line 1 1` i `#line L 0` (numer linii i numer pliku).
      Zapisać, czy kompilator Apple je przyjmuje. Jeśli nie, oba oświetlone programy nie
      wczytają się wcale, a labirynt będzie widoczny tylko w trybie `Unlit`
- [ ] **numer linii po `#line`: dokładny czy przesunięty o jeden**: zrobić celowy błąd w
      znanej linii `assets/shaders/common/lighting.glsl` (na przykład usunąć średnik na
      końcu linii 63, `return max(dot(normal, toLight), 0.0);`), kliknąć `Reload shaders` i
      porównać numer z komunikatu z numerem linii w edytorze. Na karcie NVIDIA numer
      zgadzał się z plikiem (zmierzone dla linii 63). Sterowniki różnie liczą linię po
      dyrektywie `#line`, więc na Macu może wyjść o jeden mniej albo więcej. Zapisać wynik.
      To samo sprawdzić dla błędu w samym `lit.frag`, w linii za dyrektywą `#include`.
      Przywrócić pliki (`git checkout assets/shaders`)
- [ ] **format błędu sterownika Apple**: kod rozpoznaje linię postaci `ERROR: 1:15: ...`
      i zamienia numer napisu źródłowego na nazwę pliku, co daje
      `ERROR: common/lighting.glsl:15: ...`. Ten format jest wpisany w kod i sprawdzony
      tylko testem jednostkowym (`nameSourceFiles puts the file name into an Apple error
      line`). Zapisać dokładną linię, którą pokazuje panel Shaders przy błędzie z
      poprzedniego punktu. Jeśli zaczyna się od numeru zamiast od nazwy pliku, sterownik
      pisze błędy inaczej, niż zakłada kod: wtedy numer objaśnia ostatnia linia komunikatu,
      `Source files: 0 = lit.frag, 1 = common/lighting.glsl`
- [ ] **rozmiar bloku uniformów**: po starcie w terminalu nie ma linii `[error] Uniform
      block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code`. Taka
      linia znaczyłaby, że sterownik Apple układa blok `std140` inaczej niż struktura
      `scene::LightBlockData`, a światła byłyby czytane z błędnych miejsc. Program
      porównuje rozmiar z `GL_UNIFORM_BLOCK_DATA_SIZE` przy każdym podłączeniu bloku, także
      po `Reload shaders`
- [ ] **funkcje buforów uniformów w kontekście 4.1**: `glBindBufferBase`,
      `glBufferSubData`, `glGetUniformBlockIndex`, `glUniformBlockBinding` i
      `glGetActiveUniformBlockiv` nie zostawiają błędu. W buildzie Debug po starcie i po
      kilku minutach chodzenia nie ma linii `[error]` z nazwą błędu OpenGL (`GL_...`)
- [ ] **pętla po `uPoints` z wcześniejszym `break`**: w `common/lighting.glsl` pętla ma
      stałą górną granicę (`i < MAX_POINT_LIGHTS`) i wychodzi przez `break`, gdy
      `i >= uPointCount`. Sprawdzić, że świateł punktowych jest dokładnie tyle, ile
      znaczników (11 w labiryncie startowym), i że po `Regenerate` z `Width` 4, `Height`
      4, `Seed` 1 świecą tylko dwa: żadne światło z poprzedniego labiryntu nie zostaje
- [ ] **`-D_CRT_USE_BUILTIN_OFFSETOF` poza Windowsem**: przełącznik stoi w `.clang-tidy`
      (`ExtraArgs`) i w `.clangd` (`CompileFlags: Add`), więc dostaje go także clang-tidy i
      clangd na Macu. Oczekiwane: nic się nie zmienia, bo makro czytają tylko nagłówki
      biblioteki C Microsoftu. Do sprawdzenia: `make tidy` nie zgłasza niczego, a edytor z
      clangd nie pokazuje nowych błędów w `src/scene/LightBlock.hpp`. Niesprawdzone
- [ ] **Retina: rozmiar framebuffera a światła**: panel Renderer pokazuje `Framebuffer`
      dwa razy większy niż `Window` (dla okna 1280 x 720 oczekiwane 2560 x 1440). Tryby
      `Phong` i `Blinn-Phong` liczą światło dla każdego fragmentu, a fragmentów jest wtedy
      cztery razy więcej niż na Windowsie przy tym samym oknie. Zapisać FPS z panelu
      Renderer w czterech trybach listy `Lighting`
- [ ] **Retina a układ siedmiu paneli**: po usunięciu `imgui.ini` Renderer stoi nad Lights
      w lewej kolumnie, Maze nad Assets w prawej, Collision i Shaders na dole, Camera
      zwinięty u góry, nic się nie zasłania. Zapisać, czy panel Lights (przy zwiniętej
      grupie `Moon (directional)`) pokazuje całą zawartość bez przewijania: jego wysokość
      jest dobrana do zawartości zmierzonej na Windowsie
- [ ] **znaczniki świateł na Retinie**: kostki o boku 0,14 m są czytelne z odległości
      kilku komórek

**Test ręczny (ta sama lista co w [`build-windows.md`](build-windows.md), sekcja 13.2)**

Na macOS po zmianie pliku shadera nie trzeba niczego kopiować: wystarczy `Reload shaders`.
Nazwy widżetów są zapisane tak jak w kodzie paneli.

- [ ] przygotowanie: usunąć `imgui.ini` z katalogu, z którego startuje program, zbudować,
      uruchomić `./build/debug/night_maze`
- [ ] start: nocna scena w trybie Blinn-Phong, w terminalu nie ma linii `[error]`
- [ ] panel Renderer: lista `Lighting` z wybraną pozycją `Blinn-Phong`
- [ ] panel Lights: edytor `Ambient`, grupy `Moon (directional)` (zwinięta),
      `Flashlight (spot)`, `Point lights (dead ends)` i `Highlight (specular)`, linia
      `In this maze: 11 (at most 16)`
- [ ] panel Shaders: pięć linii zakończonych `: OK` (`basic`, `textured`, `color`, `lit`,
      `gouraud`), podpowiedź nad linią z dwiema pełnymi ścieżkami
- [ ] rozwinięcie panelu Camera strzałką w pasku tytułu: otwiera się w dół, kończy się tuż
      nad dolnym rzędem paneli i nie zasłania żadnego innego panelu
- [ ] klawisz F wyłącza i włącza latarkę, także przy wolnym kursorze, a pole
      `Flashlight on (key F)` w panelu Lights zmienia się razem z nią. Kliknięcie pola robi
      to samo
- [ ] stożek latarki zostaje na środku obrazu podczas chodzenia do przodu, bokiem i biegu
- [ ] `Lighting`, `Unlit`: labirynt równo jasny, bez znaczników świateł
- [ ] `Lighting`, `Gouraud`: światło liczone w wierzchołkach, łagodne przejścia między
      narożnikami, znaczniki widoczne
- [ ] `Lighting`, `Phong` i `Blinn-Phong`: okrągła plama latarki z miękkim brzegiem
- [ ] Gouraud a Phong na ścianie, twarzą do niej z około 2 m: w `Phong` okrągła plama, w
      `Gouraud` plama znika albo rozmazuje się wzdłuż krawędzi trójkątów (duża ściana ma
      wierzchołki tylko w narożnikach)
- [ ] Gouraud a Phong u podstawy słupka: zapisać, jak wygląda podłoga wokół słupka w obu
      trybach
- [ ] Phong a Blinn-Phong przy `Strength` 1.0 i `Shininess` 16, twarzą do ściany: w
      `Blinn-Phong` jasna plama połysku jest szersza i jaśniejsza
- [ ] Phong a Blinn-Phong pod płaskim kątem do światła punktowego albo księżyca: w
      `Blinn-Phong` połysk rozciąga się w smugę, w `Phong` jest mniejszy albo się urywa.
      Przywrócić `Strength` 0.25 i `Shininess` 32
- [ ] `Moon yaw` i `Moon pitch` (rozwinąć grupę `Moon (directional)`, wyłączyć latarkę):
      na starcie (25 i -50) jasne są strony ścian patrzące w stronę -X i +Z, ciemne te
      patrzące w stronę +X i -Z. `Moon yaw` 205 zamienia je miejscami, `Moon pitch` -90
      zostawia światło księżyca tylko na podłodze
- [ ] `Point radius`: większy promień powiększa kałuże światła, także za ścianami (cieni
      nie ma). Policzyć z góry (klawisz N, spacja) 11 znaczników i porównać ze ślepymi
      zaułkami na planie w panelu Maze
- [ ] `Cone`: większe `outer` poszerza plamę, `inner` bliskie `outer` daje ostry brzeg,
      pola `inner` nie da się przeciągnąć powyżej `outer`. `Beam range`: mała wartość
      skraca zasięg latarki
- [ ] `Regenerate` z innym ziarnem: światła i znaczniki stoją w ślepych zaułkach nowego
      labiryntu, liczba w linii `In this maze: ...` odpowiada nowemu planowi
- [ ] celowy błąd w `assets/shaders/common/lighting.glsl`, potem `Reload shaders`: linie
      `lit.vert + lit.frag: FAILED, the previous program stays in use` i
      `gouraud.vert + gouraud.frag: FAILED, the previous program stays in use` są
      czerwone, komunikat pod nimi nazywa plik `common/lighting.glsl` i linię, obraz się
      nie zmienia. Potem `git checkout assets/shaders` i `Reload shaders`: wszystkie pięć
      linii kończy się napisem `: OK`
- [ ] panel Assets, `View mode` równy `Normals as colour` i `UVs as colour` przy trybie
      `Blinn-Phong`: labirynt rysuje program `textured`, bez świateł, a znaczniki świateł
      nadal są widoczne. Widok normalnych pokazuje normalne używane przez wybrany tryb
      (lista map normalnych niżej)
- [ ] przez cały test w terminalu nie pojawia się żadna linia `[error]` poza tymi
      wywołanymi celowo

### M4 (mapy normalnych) na macOS: lista w całości otwarta

Druga część M4 (mapy normalnych z pola wysokości, linia `map_Bump` w MTL, styczne
wierzchołków, czwarty atrybut, plik `common/normal_map.glsl`, druga jednostka teksturująca,
pole `Normal mapping` w panelu Assets) powstała na Windowsie 2026-10-05 i tam jest zbudowana
i częściowo sprawdzona ([`build-windows.md`](build-windows.md), sekcje 13.3 i 13.4).
**Na macOS nikt jej nie zbudował ani nie uruchomił, więc żaden punkt poniżej nie jest
odhaczony.** Oczekiwania wynikają z kodu i z pomiarów na Windowsie. Opis kodu:
[`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), decyzja:
[`../decisions/tangents-on-load.md`](../decisions/tangents-on-load.md).

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/assets/Tangents.*` i `tests/TangentTests.cpp`. Zmienione: `src/gfx/Vertex.hpp`,
      `src/gfx/Mesh.*`, `src/assets/ObjLoader.*`, `src/assets/AssetCache.*`,
      `src/game/Lighting.*`, `MazeRenderer.*`, `NightMazeApp.*`, `ShaderUniforms.hpp`,
      `src/debug/DebugUI.cpp`, `src/debug/panels/AssetsPanel.*` i trzy pliki testów.
      Miejsca warte uwagi: `constexpr glm::vec3` jako stałe osi w `Tangents.cpp`,
      `std::span<gfx::Vertex>` budowany z `std::vector` i z `std::array` w wywołaniach
      `computeTangents` (w teście także pusta lista `{}` jako zakres indeksów),
      inicjalizatory desygnowane `gfx::Vertex{.position = ...}`, które teraz pomijają także
      pole `tangent`. Zapisać każde ostrzeżenie
- [ ] przechodzą asercje czasu kompilacji w `Vertex.hpp`: rozmiar równy 11 liczbom `float`
      (44 bajty) i układ standardowy
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 163 przypadki testowe i 62220
      asercji, `Status: SUCCESS!` (liczby z Windowsa z 2026-10-05)
- [ ] pliki testów osobno, opcją `--source-file`: `'*TangentTests*'` 9 przypadków i 177
      asercji, `'*ObjLoaderTests*'` 20 i 1576, `'*ImageLoaderTests*'` 9 i 57,
      `'*LightingTests*'` 17 i 133 (liczby z Windowsa, Debug, 2026-10-05)
- [ ] przechodzą przypadki zależne od zaokrągleń `float` na ARM: `computeTangents: the
      tangent is made perpendicular to the normal (Gram-Schmidt)`, `computeTangents: a
      vertex shared by two triangles gets the average tangent` i sprawdzenia stycznych w
      trzech przypadkach `loadObj` (długość 1, iloczyn skalarny z normalną równy 0,
      `cross(N, T)` w górę)
- [ ] przechodzą dwa przypadki map normalnych w `tests/ImageLoaderTests.cpp` (`the normal
      maps of the game load, and most of their texels are flat` i `the wall normal map
      follows the OpenGL convention: a joint is a groove`): czytają pliki PNG z
      repozytorium, więc wynik nie zależy od Blendera na Macu
- [ ] clang-format i clang-tidy bez uwag na nowych i zmienionych plikach

**Shadery (największe ryzyko)**

- [ ] gra startuje bez linii `[error]`: kompilator GLSL Apple przyjmuje
      `common/normal_map.glsl` wklejony do `lit.frag` i do `textured.frag`. Miejsca, których
      ten kompilator jeszcze nie widział: drugi plik dołączany w jednym shaderze (dwie pary
      dyrektyw `#line` z numerami źródeł 1 i 2), `uniform bool` ustawiany przez
      `glUniform1i`, konstruktor `mat3(t, b, n)` z trzech wektorów, wejście
      `layout(location = 3)` w shaderach wierzchołków
- [ ] zapisać ostrzeżenia z dziennika kompilacji, jeśli są: w trybie `Textured` program
      `textured` czyta sampler `uNormalMap` tylko w jednej gałęzi `if`, a w programie `lit`
      funkcja `surfaceNormal` wraca wcześniej przy wyłączonym przełączniku
- [ ] celowy błąd w `assets/shaders/common/normal_map.glsl`, potem `Reload shaders`: linie
      `lit.vert + lit.frag` i `textured.vert + textured.frag` są czerwone, a komunikat pod
      nimi nazywa plik `common/normal_map.glsl` (format Apple `ERROR: 2:...` dla `lit.frag`,
      w którym to drugi plik dołączany, i `ERROR: 1:...` dla `textured.frag`). Potem
      `git checkout assets/shaders` i `Reload shaders`
- [ ] brak błędu OpenGL przy wiązaniu dwóch tekstur: w buildzie Debug żadnej linii z
      `GL_INVALID_...` po kilku klatkach i po przełączeniu trybów `Lighting`

**Obraz i panel**

- [ ] start w trybie `Blinn-Phong`: fugi ścian i podłogi są rowkami (nie wałkami) na
      ścianach wzdłuż X, wzdłuż Z, na słupku i na podłodze
- [ ] światło z boku: po przejściu latarki na drugą stronę jasne i ciemne skosy zamieniają
      się miejscami
- [ ] pole `Normal mapping` w panelu Assets: odznaczone daje płaskie ściany, zaznaczone
      przywraca relief od razu. To samo przy `Phong`
- [ ] `Lighting` równe `Gouraud` i `Unlit`: pole niczego nie zmienia w obrazie
- [ ] `View mode` równe `Normals as colour`: rysunek fug widać przy `Unlit`, `Phong` i
      `Blinn-Phong`, nie widać go przy `Gouraud` ani przy odznaczonym polu
- [ ] lista `Models`: linia `normal map:` pod każdą częścią. Lista `Textures`: cztery
      tekstury, dwie jasnoniebieskie, podglądy stoją prosto
- [ ] ekran Retina: zapisać, czy relief w oddali migocze w ruchu przy filtrze `Trilinear` i
      czy anizotropia to zmienia (na Windowsie oceniono to tylko na nieruchomych klatkach)
- [ ] celowo brakująca mapa normalnych: zmienić nazwę
      `assets/textures/floor_stone_normal.png` (na Macu to plik w repozytorium, bo `assets`
      obok programu jest dowiązaniem), uruchomić. Oczekiwane: podłoga z teksturą koloru,
      ale płaska pod latarką, jedna linia `[error]`, w panelu Assets `normal map: none
      (flat)` i sekcja `Failed to load`. **Przywrócić nazwę pliku** i sprawdzić `git status`

**Skrypty Blendera**

- [ ] uruchomić na Macu `blender --background --factory-startup --python
      tools/blender/make_textures.py` i trzy skrypty modeli tą samą wersją Blendera
      (5.2.1), potem `git status` i `git diff --stat`. Na Windowsie dwa uruchomienia dały
      identyczne skróty dziesięciu plików. **Nie wiadomo, czy Mac da te same bajty**: mapy
      normalnych przechodzą przez `np.linalg.norm`, dzielenie i zaokrąglanie do 8 bitów, a
      NumPy na procesorze ARM może dać wynik różny o ostatni bit, co po zaokrągleniu zmienia
      pojedyncze bajty. Inna może być też kompresja PNG. Zapisać, które pliki się różnią
- [ ] jeśli pliki PNG się różnią: porównać piksele, nie bajty pliku, i zapisać największą
      różnicę. Różnica o 1 na 255 w pojedynczych tekselach nie zmienia obrazu, ale wtedy
      zasada "skrypt odtwarza pliki co do bajta" obowiązuje tylko w obrębie jednego systemu
- [ ] po próbie przywrócić pliki z repozytorium (`git checkout assets`), żeby testy
      czytały te same mapy co na Windowsie

### Skróty: `make`

Te same polecenia mają krótsze odpowiedniki w pliku [`Makefile`](../../Makefile) w katalogu
głównym repozytorium:

```sh
make run          # konfiguracja, build Debug i uruchomienie
make run-release  # to samo dla Release
make test         # build Debug i testy jednostkowe
make check        # format-check, oba buildy z testami i clang-tidy: komplet przed commitem
make              # lista wszystkich celów
```

`Makefile` niczego nie buduje sam, tylko woła polecenia `cmake --preset ...` opisane wyżej,
więc oba sposoby są równoważne. Każdy cel i każdą linię pliku opisuje
[`project-structure.md`](project-structure.md), sekcja 3.12.

## 3. Co robią presety: `CMakePresets.json`

Preset to nazwany zestaw ustawień CMake zapisany w repozytorium. Zamiast pamiętać długie
polecenie z opcjami `-S`, `-B` i `-D`, piszemy `cmake --preset debug`, a każdy (my na dwóch
komputerach, IDE, prowadzący) dostaje identyczną konfigurację.

Plik [`CMakePresets.json`](../../CMakePresets.json) ma trzy części.

### Nagłówek

```json
"version": 3,
"cmakeMinimumRequired": { "major": 3, "minor": 24, "patch": 0 },
```

`version` to wersja formatu pliku presetów (nie projektu). `cmakeMinimumRequired` sprawia,
że starszy CMake od razu zgłosi czytelny błąd.

### Presety konfiguracji (`configurePresets`)

```json
{
    "name": "base",
    "hidden": true,
    "binaryDir": "${sourceDir}/build/${presetName}",
    "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
    }
},
{
    "name": "debug",
    "displayName": "Debug",
    "inherits": "base",
    "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
    }
},
```

**Preset `base`** zbiera to, co wspólne:

- `"hidden": true`: preset ukryty. Nie da się go użyć bezpośrednio i nie widać go na liście
  `cmake --list-presets`. Istnieje tylko po to, żeby inne presety z niego dziedziczyły.
- `"binaryDir": "${sourceDir}/build/${presetName}"`: katalog buildu. `${sourceDir}` to
  katalog repozytorium, `${presetName}` to nazwa presetu, który jest **faktycznie użyty**
  (nie `base`). Dlatego preset `debug` buduje w `build/debug`, a `release` w `build/release`.
  Dwa osobne katalogi pozwalają mieć obie konfiguracje jednocześnie bez przebudowywania.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON`: CMake zapisuje plik `compile_commands.json` z dokładnym
  poleceniem kompilacji każdego pliku. Korzystają z niego clangd i clang-tidy.

**Presety `debug` i `release`**:

- `"inherits": "base"`: dziedziczenie. Preset przejmuje wszystkie ustawienia z `base` i
  dokłada własne. Dzięki temu wspólne ustawienia są zapisane raz.
- `CMAKE_BUILD_TYPE`: typ buildu, `Debug` albo `Release`. Dla generatora Unix Makefiles ta
  zmienna decyduje o flagach kompilatora (sekcja 5).
- `displayName`: nazwa pokazywana w IDE.

`cacheVariables` to odpowiednik opcji `-D` w linii poleceń. `cmake --preset debug` jest więc
równoważne mniej więcej poleceniu:

```sh
cmake -S . -B build/debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
```

### Presety budowania (`buildPresets`)

```json
{
    "name": "debug",
    "displayName": "Debug",
    "configurePreset": "debug",
    "configuration": "Debug"
},
```

- `configurePreset` wskazuje, którego katalogu buildu dotyczy `cmake --build --preset debug`.
- `configuration` ma znaczenie dla generatorów wielokonfiguracyjnych (Visual Studio, Xcode).
  Na Macu z Makefile jest ignorowane, bo typ buildu ustalił już `CMAKE_BUILD_TYPE`. Opis
  różnicy jest w [`build-windows.md`](build-windows.md).

Lista dostępnych presetów:

```sh
cmake --list-presets
cmake --build --list-presets
```

Plik `CMakeUserPresets.json` (w `.gitignore`) służy do prywatnych presetów, na przykład z
generatorem Ninja. Nie trafia do repozytorium.

## 4. Co pobiera FetchContent i dokąd

Przy pierwszym `cmake --preset debug` CMake wykonuje
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i klonuje pięć repozytoriów:

| Biblioteka | Tag | Katalog źródeł |
|---|---|---|
| GLFW | `3.4` | `build/debug/_deps/glfw-src` |
| GLM | `1.0.3` | `build/debug/_deps/glm-src` |
| Dear ImGui | `v1.92.9b-docking` | `build/debug/_deps/imgui-src` |
| doctest | `v2.5.3` | `build/debug/_deps/doctest-src` |
| stb (dla stb_image) | brak tagów, commit `2c980bb5...` | `build/debug/_deps/stb-src` |

Czwarty wiersz doszedł w kamieniu milowym M2 + M3 i na Macu nie był jeszcze pobierany
(sekcja 2, "Testy jednostkowe"). Na Windowsie katalog `doctest-src` powstaje zgodnie z
tabelą. Piąty wiersz doszedł w tym samym kamieniu milowym i też nie był jeszcze pobierany na
Macu. Repozytorium stb jest klonowane w całości, z historią (na Windowsie 12 MB), bo jest
przypięte do commita, a nie do tagu ([`../libraries/stb_image.md`](../libraries/stb_image.md),
sekcja 2).

Dla każdej zależności w `_deps` powstają trzy katalogi:

- `<nazwa>-src`: pobrany kod źródłowy,
- `<nazwa>-build`: pliki powstałe przy jej budowaniu (dla GLM, ImGui i doctest nie ma tam
  żadnej biblioteki: GLM i doctest to same nagłówki, a ImGui kompiluje nasz target `imgui`),
- `<nazwa>-subbuild`: pomocniczy projekt CMake, który wykonuje samo pobieranie.

Rzeczy warte zapamiętania:

- **Każdy preset ma własne `_deps`.** `build/release/_deps` to osobne pobranie, więc pierwsza
  konfiguracja presetu `release` też wymaga sieci.
- Kolejne konfiguracje nie pobierają niczego na nowo, dopóki tag w `Dependencies.cmake` się
  nie zmieni.
- Katalog `build/` jest w `.gitignore`. Pobrany kod nie trafia do repozytorium.
- GLAD nie jest pobierany. Leży w `external/glad` (dlaczego: [`../libraries/glad.md`](../libraries/glad.md)).
- Pobrane źródła są przydatne do nauki: `build/debug/_deps/imgui-src/imgui_demo.cpp`,
  `build/debug/_deps/glfw-src/docs/` oraz `build/debug/_deps/glm-src/manual.md`.

Po buildzie w `build/debug` znajdują się między innymi:

| Plik | Co to jest |
|---|---|
| `night_maze` | program |
| `assets` | dowiązanie symboliczne do katalogu `assets/` z repozytorium, tworzone po linkowaniu |
| `libengine.a` | nasza biblioteka statyczna `engine` (kod z `src/core`, `src/gfx` i `src/scene`) |
| `libimgui.a` | biblioteka `imgui` zdefiniowana w `Dependencies.cmake` |
| `external/glad/libglad.a` | biblioteka `glad` |
| `_deps/glfw-build/src/libglfw3.a` | biblioteka `glfw` |
| `compile_commands.json` | baza poleceń kompilacji dla narzędzi |
| `CMakeCache.txt` | zapamiętane ustawienia konfiguracji |

## 5. Debug a Release w tym projekcie

Flagi odczytane z `compile_commands.json` obu katalogów:

| | Debug | Release |
|---|---|---|
| Flagi kompilatora | `-g` | `-O3 -DNDEBUG` |
| Optymalizacja | brak | pełna |
| Symbole debugowe | tak | nie |
| Makro `NDEBUG` | niezdefiniowane | zdefiniowane |
| `GL_CHECK` | wykonuje wywołanie i sprawdza `glGetError` | tylko wykonuje wywołanie |
| `assert` | aktywne | wyłączone |

Wspólne dla obu: `-std=c++20`, `-Wall -Wextra -Wpedantic` (dla naszych targetów),
`-DGLFW_INCLUDE_NONE`, `-DGL_SILENCE_DEPRECATION`.

Najważniejsza różnica dla tego projektu to `GL_CHECK` z
[`src/core/GlCheck.hpp`](../../src/core/GlCheck.hpp):

```cpp
#ifndef NDEBUG
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
        core::checkGlErrors(#call, __FILE__, __LINE__);                                            \
    } while (false)
#else
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
    } while (false)
#endif
```

`NDEBUG` to standardowe makro "no debug". CMake dodaje `-DNDEBUG` w konfiguracji Release.
W Debug po każdym opakowanym wywołaniu OpenGL sprawdzamy błędy i wypisujemy je z nazwą pliku
i numerem linii. W Release sprawdzanie znika, bo `glGetError` po każdym wywołaniu kosztuje.

Dlaczego to ważne akurat tutaj: OpenGL 4.1 nie ma `glDebugMessageCallback` (to 4.3), a
RenderDoc nie działa na macOS. `GL_CHECK` i panele ImGui są na Macu jedynymi narzędziami
diagnostycznymi.

Wniosek praktyczny: **pracujemy na Debug**, a Release budujemy, żeby zmierzyć wydajność i
sprawdzić, czy program działa także z optymalizacją (przed tagiem kamienia milowego).

## 6. Konfiguracja IDE

### CLion

1. Otwórz katalog repozytorium (File, Open).
2. CLion wykrywa `CMakePresets.json`. W Settings, Build, Execution, Deployment, CMake włącz
   profile odpowiadające presetom `debug` i `release`, a domyślny profil CLion-a (katalog
   `cmake-build-debug`) wyłącz lub usuń, żeby nie mieć trzeciego, innego buildu.
3. Wybierz konfigurację uruchomieniową `night_maze`.
4. W Run, Edit Configurations ustaw Working directory na katalog repozytorium. Wtedy
   `imgui.ini` będzie tym samym plikiem co przy uruchamianiu z terminala.

Katalogi `.idea/` i `cmake-build-*/` są w `.gitignore`.

### Cursor i VS Code z clangd

clangd to serwer języka (language server): podpowiedzi, przejście do definicji, błędy na
żywo. Żeby rozumiał projekt, musi znać flagi kompilacji każdego pliku. Bierze je z
`compile_commands.json`.

Repozytorium zawiera gotową konfigurację, niczego nie trzeba ustawiać ręcznie:

| Plik | Co załatwia |
|---|---|
| [`.clangd`](../../.clangd) | wskazuje clangd katalog `build/debug` jako miejsce, w którym leży `compile_commands.json`, i dopisuje do flag `-D_CRT_USE_BUILTIN_OFFSETOF` (przełącznik potrzebny na Windowsie, na macOS powinien być obojętny: niesprawdzone) |
| [`.vscode/settings.json`](../../.vscode/settings.json) | clangd jako silnik C++, wyłączony IntelliSense drugiego rozszerzenia C++, presety CMake, formatowanie przy zapisie, skojarzenia plików GLSL |
| [`.vscode/extensions.json`](../../.vscode/extensions.json) | lista rozszerzeń, które edytor zaproponuje do zainstalowania |

Każdy klucz tych plików jest opisany w [`project-structure.md`](project-structure.md),
sekcje 3.9 do 3.11.

Kolejność pierwszego uruchomienia:

1. Otwórz katalog repozytorium w Cursorze albo VS Code i zainstaluj rozszerzenia, które
   edytor zaproponuje (clangd, CMake Tools, CodeLLDB, Shader languages support, Markdown
   Preview Mermaid Support).
2. Wykonaj w terminalu `cmake --preset debug`. Dopiero wtedy powstaje
   `build/debug/compile_commands.json`. **Do tego momentu edytor podkreśla na czerwono
   dyrektywy `#include` i zgłasza błędy "file not found"**: clangd nie zna jeszcze ścieżek
   do GLFW, GLAD, GLM i ImGui. To nie jest błąd w kodzie. Po konfiguracji błędy znikają (czasem
   trzeba przeładować okno edytora albo wykonać polecenie "clangd: Restart language server").
3. Budowanie i uruchamianie: z terminala wbudowanego w edytor (polecenia z sekcji 2) albo
   przez rozszerzenie CMake Tools, które dzięki `"cmake.useCMakePresets": "always"` korzysta
   z tych samych presetów.

Dawniej ten dokument radził zrobić w katalogu głównym dowiązanie symboliczne do
`build/debug/compile_commands.json`. Plik `.clangd` zastępuje to rozwiązanie: jest w
repozytorium, więc działa od razu po sklonowaniu i nie wymaga żadnego polecenia. Dowiązania
nie trzeba już tworzyć.

Dlaczego wyłączony jest IntelliSense drugiego rozszerzenia C++ (ustawienie
`"C_Cpp.intelliSenseEngine": "disabled"`): gdy działają dwa silniki naraz, każdy pokazuje
własne błędy. Ten drugi nie czyta `.clangd`, nie zna więc ścieżek nagłówków i zgłasza
fałszywe błędy, które dublują się z prawdziwymi diagnostykami clangd.

W katalogu `.vscode/` wersjonowane są tylko te dwa pliki. Wszystko inne, co edytor tam
zapisze (na przykład własne `launch.json`), pozostaje lokalne dzięki wpisom w `.gitignore`.

Po dodaniu nowego pliku źródłowego do `CMakeLists.txt` trzeba ponownie wykonać
`cmake --preset debug` (albo po prostu zbudować projekt, co samo powtarza konfigurację),
inaczej clangd nie zna flag dla nowego pliku.

## 7. Formatowanie i analiza statyczna

### clang-format

Styl opisuje [`.clang-format`](../../.clang-format): bazą jest styl LLVM, wcięcie 4 spacje,
limit 100 znaków w linii, `*` i `&` przy typie (`PointerAlignment: Left`), obowiązkowe
klamry (`InsertBraces: true`).

Instalacja: `brew install clang-format` (Command Line Tools go nie zawierają).

Sformatowanie wszystkich naszych plików:

```sh
find src tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

Sprawdzenie bez modyfikowania plików (kod wyjścia różny od zera, gdy coś wymaga zmian):

```sh
find src tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format --dry-run --Werror
```

To drugie polecenie zostało uruchomione na kodzie M0 (clang-format 22.1.7), jeszcze w
wersji dla samego `src/`, i nie zgłaszało żadnych różnic. W wersji z katalogiem `tests/`
(M2 + M3) na Macu nie było jeszcze uruchamiane. Na Windowsie sprawdzenie plików z `src/` i
`tests/` narzędziem clang-format 19.1.5 nie zgłasza różnic.

Formatujemy `src/` i `tests/`. Katalogu `external/glad` nie dotykamy, bo to kod wygenerowany.

Ważny szczegół ustawień: `SortIncludes: CaseSensitive` razem z `IncludeBlocks: Preserve`.
Formater sortuje dyrektywy `#include` tylko wewnątrz bloków oddzielonych pustą linią. Dzięki
temu `<glad/gl.h>` pozostaje przed `<GLFW/glfw3.h>` w `Window.cpp`, bo są w osobnych blokach.

### clang-tidy

Reguły są w [`.clang-tidy`](../../.clang-tidy) (grupy `bugprone`, `performance`, `modernize`
oraz konwencja nazw, na przykład prefiks `m_` dla pól prywatnych). Od M4 plik ma też linię
`ExtraArgs: ['-D_CRT_USE_BUILTIN_OFFSETOF']`: dopisuje ona jedną definicję makra do flag
każdego sprawdzanego pliku. Makro czytają tylko nagłówki biblioteki C Microsoftu, więc na
macOS nie powinno niczego zmieniać. Z tą linią clang-tidy na Macu nie był jeszcze
uruchamiany (punkt na liście "M4 (oświetlenie) na macOS" w sekcji 2, opis w
[`project-structure.md`](project-structure.md), sekcja 3.6). clang-tidy nie wchodzi w
skład Command Line Tools ani pakietu `clang-format` z Homebrew. Jest w pakiecie `llvm`:

```sh
brew install llvm
```

Pakiet `llvm` jest w Homebrew typu keg-only: instaluje się we własnym katalogu i **nie jest
dodawany do `PATH`**, żeby jego `clang` nie przesłonił kompilatora Apple. Samo `clang-tidy`
w terminalu da więc "command not found". Program leży w
`$(brew --prefix llvm)/bin/clang-tidy` i tak go wołamy.

Sprawdzenie jednego pliku:

```sh
"$(brew --prefix llvm)/bin/clang-tidy" -p build/debug \
    --extra-arg=-isysroot --extra-arg="$(xcrun --show-sdk-path)" \
    src/core/Window.cpp
```

Sprawdzenie wszystkich naszych plików `.cpp`:

```sh
find src tests -name '*.cpp' | xargs "$(brew --prefix llvm)/bin/clang-tidy" -p build/debug \
    --extra-arg=-isysroot --extra-arg="$(xcrun --show-sdk-path)"
```

Dwa argumenty `--extra-arg` dopisują do flag kompilacji ścieżkę do SDK macOS
(`xcrun --show-sdk-path` ją wypisuje). Kompilator Apple zna ją sam, więc nie ma jej w
`compile_commands.json`, a clang-tidy z Homebrew bez niej zgłasza błędy typu
`'array' file not found` dla nagłówków biblioteki standardowej.

- `$(brew --prefix llvm)` to podstawienie polecenia: powłoka wstawia w to miejsce katalog
  instalacji pakietu (na Apple Silicon zwykle `/opt/homebrew/opt/llvm`).
- `-p build/debug` wskazuje katalog z `compile_commands.json`. clang-tidy musi znać te same
  flagi co kompilator (ścieżki nagłówków, `-std=c++20`, makra), inaczej nie zrozumie kodu.
  Najpierw trzeba więc wykonać `cmake --preset debug`.
- Argumentami są pliki `.cpp`. Nagłówki z `src/` są sprawdzane przy okazji plików, które je
  dołączają (`HeaderFilterRegex: 'src/.*'`).

Stan: konfiguracja `.clang-tidy` jest w repozytorium i narzędzie da się uruchomić powyższymi
poleceniami. Ten dokument nie podaje wyniku takiego uruchomienia na kodzie M0. clangd
pokazuje część tych samych diagnostyk w edytorze, bo czyta ten sam plik `.clang-tidy`.

## 8. Rozwiązywanie problemów

| Objaw | Przyczyna | Rozwiązanie |
|---|---|---|
| `xcrun: error: invalid active developer path` albo brak kompilatora przy konfiguracji | brak Command Line Tools (często po aktualizacji macOS) | `xcode-select --install` |
| `CMake 3.24 or higher is required` albo błąd czytania presetów | za stary CMake | `brew upgrade cmake`, sprawdź `cmake --version` |
| Konfiguracja pada na `git clone` | brak sieci przy pierwszej konfiguracji danego presetu | połącz się z internetem i powtórz. Raz pobrane zależności nie wymagają sieci |
| Błąd po zmianie tagu lub opcji w `Dependencies.cmake`, dziwne wartości z cache | stary `CMakeCache.txt` | usuń tylko katalog danego presetu (`rm -rf build/debug`) i skonfiguruj od nowa. Kosztem jest ponowne pobranie zależności |
| `CMake Error: ... generator does not match the generator used previously` | katalog buildu utworzono innym generatorem (na przykład Ninja z IDE, Makefile z terminala) | jeden katalog buildu to jeden generator. Usuń katalog presetu albo używaj spójnie jednego narzędzia |
| Błąd linkera `undefined symbol` dla naszej funkcji | nowy plik `.cpp` nie został dopisany do `add_library(engine ...)` lub `add_executable(night_maze ...)` | dopisz plik w `CMakeLists.txt`. Pliki wymieniamy jawnie, CMake sam ich nie wyszukuje |
| `use of undeclared identifier 'glDebugMessageCallback'` (lub inna funkcja `gl*`) | funkcja z OpenGL 4.2 lub nowszego, której nie ma w naszym GLAD | to celowa bariera, opis w [`../libraries/glad.md`](../libraries/glad.md). Trzeba użyć odpowiednika z 4.1 |
| `'glad/glad.h' file not found` | kod skopiowany z poradnika dla GLAD 1 | u nas `<glad/gl.h>` i `gladLoadGL(glfwGetProcAddress)` |
| `[error] GLFW error ...` i `Fatal: Failed to create a window with an OpenGL 4.1 Core context` | system nie udostępnił kontekstu 4.1 Core | przeczytaj opis w linii `GLFW error`. Na Macu z Apple Silicon nie powinno wystąpić |
| Ostrzeżenia `'gl...' is deprecated: first deprecated in macOS 10.14` | plik kompilowany bez `GL_SILENCE_DEPRECATION` | definicja jest `PUBLIC` na targecie `engine`. Sprawdź, czy nowy target linkuje `engine` |
| `[error] Shader file cannot be opened: .../build/debug/assets/shaders/basic.vert`, w oknie samo tło | obok programu nie ma katalogu `assets`: program skopiowany ręcznie w inne miejsce albo repozytorium przeniesione po zbudowaniu (dowiązanie wskazuje starą ścieżkę) | `ls -l build/debug/assets`. Odtwórz dowiązanie pełnym buildem: `make clean`, potem `make debug` |
| `[error] Shader compilation failed: ...` z linią sterownika, znika to, co rysuje ten program (w stanie M1, z jednym programem, zostawało samo tło) | błąd w pliku shadera. Sterownik Apple pisze linię w postaci `ERROR: 0:N: ...`, gdzie 0 to numer napisu źródłowego, a `N` numer linii. Od M4 program zamienia ten numer na nazwę pliku, więc oczekiwana postać to `ERROR: basic.frag:N: ...`, a dla błędu w pliku dołączanym `ERROR: common/lighting.glsl:N: ...`. Zamiana jest sprawdzona tylko testem jednostkowym, nie na prawdziwym sterowniku Apple: jeśli linia ma inną postać, zostaje taka, jak ją napisał sterownik | popraw plik w `assets/shaders/` i naciśnij "Reload shaders" w panelu Shaders (albo uruchom program ponownie). Opis w [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), sekcje 3.3 i 7, w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 7, oraz w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `[error] Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code` | sterownik ułożył blok uniformów ze światłami inaczej niż struktura `scene::LightBlockData` (na macOS niesprawdzone, na karcie NVIDIA linia się nie pojawia) | zapisać liczbę z komunikatu i porównać blok w `assets/shaders/common/lighting.glsl` ze strukturą w `src/scene/LightBlock.hpp` ([`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md)) |
| Podłoga albo ściany są białe, w terminalu linia `[error]` o pliku obrazu | brakuje pliku w `assets/textures/` albo nie da się go zdekodować: część modelu dostaje białą teksturę zastępczą (na macOS niesprawdzone) | przywróć plik (`git status`, `git checkout assets/textures`) i uruchom program ponownie |
| Okno otwiera się, ale panel "Renderer" jest niewidoczny | panele ukryte klawiszem `~` albo zapisany układ poza oknem | naciśnij `~` (na lewo od `1`). Jeśli nie pomaga, usuń `imgui.ini` z katalogu, z którego uruchamiasz program |
| Esc nie zamyka programu, `~` nie chowa paneli | aktywny jest widżet ImGui (wpisywanie albo przeciąganie wartości), więc klawiatura gry jest zablokowana | zakończ edycję (Enter, Esc albo kliknięcie poza polem). Opis w [`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6 |
| Układ paneli nie zapamiętuje się między uruchomieniami | program startuje z różnych katalogów roboczych (terminal i IDE) | `imgui.ini` powstaje w katalogu roboczym. Ustaw ten sam katalog w IDE |
| Obraz zajmuje ćwiartkę okna | `glViewport` z rozmiarem okna zamiast framebuffera | zawsze `window().framebufferSize()`, opis w [`../libraries/glfw.md`](../libraries/glfw.md) |
| FPS równe dokładnie 60 lub 120 | to nie błąd, działa vsync (`glfwSwapInterval(1)`) | do pomiarów wydajności można tymczasowo ustawić `0` |
| Edytor (clangd) podkreśla wszystkie `#include`, błędy "file not found" | nie ma jeszcze `build/debug/compile_commands.json`, na który wskazuje `.clangd` | wykonaj `cmake --preset debug` i przeładuj okno edytora, opis w sekcji 6 |
| `clang-tidy: command not found` | pakiet `llvm` z Homebrew nie jest w `PATH` | wołaj `"$(brew --prefix llvm)/bin/clang-tidy"`, sekcja 7 |

Czysty build od zera (ostateczność, gdy nic innego nie pomaga):

```sh
rm -rf build/debug
cmake --preset debug
cmake --build --preset debug
```

## 9. Powiązane dokumenty

- Mapa repozytorium i plików konfiguracyjnych: [`project-structure.md`](project-structure.md)
- Biblioteki: [`../libraries/glfw.md`](../libraries/glfw.md),
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md),
  [`../libraries/doctest.md`](../libraries/doctest.md) (testy jednostkowe)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md)
- Windows: [`build-windows.md`](build-windows.md)
- Dokumentacja CMake (presety, FetchContent): <https://cmake.org/cmake/help/latest/>
