# Budowanie na macOS (Apple Silicon)

Przewodnik dla kamienia milowego M0. Polecenia budowania i uruchamiania z tego dokumentu
zostały uruchomione na Macu na kodzie M0 i M1, w konfiguracji z tabeli niżej. **Kod M2 + M3
(testy, labirynt, gracz, modele, tekstury, nowe panele) nie był na macOS ani budowany, ani
uruchamiany.** To samo dotyczy obu części M4: oświetlenia (światła, cztery
tryby cieniowania, blok uniformów, `#include` w shaderach, panel Lights) i map normalnych
(styczne, czwarty atrybut wierzchołka, `common/normal_map.glsl`, pole `Normal mapping`).
Powstały na Windowsie 2026-10-05 i na macOS nikt ich nie zbudował. To samo dotyczy M5
(rozgrywka: kryształy, bateria latarki, brama, wyjście, HUD, panel Gameplay), też z
2026-10-05, i obu części M6 z tego samego dnia: pierwszej (skybox: tekstura sześcienna,
niebo, piąty program shaderów) i drugiej (teren z mapy wysokości w miejscu płytek podłogi,
trawa z shadera geometrii, szósty program, panele Terrain i Grass), oraz pierwszej części
M7, też z 2026-10-05 (scena rysowana do bufora HDR, przebieg składający z mapowaniem tonów,
korekcja gamma, siódmy i ósmy program, panel Framebuffers), drugiej (bloom, dziewiąty
i dziesiąty program) i trzeciej (mgła z bufora głębi i winieta w przebiegu składającym).
Wszystko, co ten dokument mówi o tym
kodzie dla Maca, jest oczekiwaniem wynikającym z kodu i z pomiarów na Windowsie, a punkty
do sprawdzenia są zebrane w sekcji 2 jako listy otwarte: "M2 + M3 na macOS", "M4
(oświetlenie) na macOS", "M4 (mapy normalnych) na macOS", "M5 (rozgrywka) na macOS",
"M6, część 1 (skybox) na macOS", "M6, część 2 (teren i trawa) na macOS", "M7, część 1
(bufor HDR i gamma) na macOS", "M7, część 2 (bloom) na macOS" i "M7, część 3 (mgła
i winieta) na macOS".

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

**Co powinno być widać dziś, po M2 + M3, M4, M5 i obu częściach M6 (na macOS niesprawdzone.
Na Windowsie widok z M4 jest zmierzony 2026-10-05 na zrzucie ekranu, a obraz po M5 i po M6
był oglądany na zrzutach, których listy nie zapisano):** nocny widok z wnętrza labiryntu
10 na 10 z teksturą kamienia na ścianach i słupkach, stojącego na łagodnie nierównym
podłożu z teksturą ubitej ziemi (teren z mapy wysokości: do M5 podłogą były płaskie płytki
z teksturą kamienia), z kępkami trawy wzdłuż ścian, oświetlony w trybie Blinn-Phong: słabe,
chłodne
światło księżyca, ciepły stożek latarki na środku obrazu i turkusowe światła punktowe nad
kryształami, które unoszą się w komórkach, obracają się i same świecą (13 w labiryncie
startowym). Przy komórce wyjścia stoi drewniana brama. Nad ścianami jest nocne niebo
z gwiazdami i księżycem (skybox), a wokół labiryntu wzgórza. Cieni nie ma. U góry okna,
na środku, jest pasek HUD:
`Crystals`, `0 / 10`, `(of 13)`, czas rundy i pasek baterii. Paneli jest jedenaście
(jedenasty, "Framebuffers", stoi zwinięty w trzecim rzędzie pasków tytułu): "Renderer"
nad "Lights" w
lewej kolumnie, "Maze" nad "Assets" w prawej, "Collision" i "Shaders" na dole między
kolumnami, a "Camera" i "Gameplay" u góry, między kolumnami, zwinięte do pasków tytułu,
z drugim rzędem zwiniętych pasków pod nimi: "Terrain" i "Grass". Pasek HUD stoi pod oboma
rzędami.
Kostki z M1, która do M4 wisiała nad komórką w rogu
przeciwległym do startu, już nie ma, tak samo jak kostek oznaczających światła w ślepych
zaułkach: M5 je usunął. Po dwóch liniach z `core::Window` pamięć podręczna assetów wypisuje
linie `[info] Loaded texture: ...` (osiem tekstur) i `[info] Loaded model: ...` (pięć
modeli: do M5 było ich sześć, z płytką podłogi), a poza nią pojawia się sześć linii
`Loaded sky face: ...` i jedna `Loaded heightmap: ...`. Po kliknięciu w scenę
kursor znika i gracz chodzi po labiryncie (tabela niżej). Wejście w kryształ go zbiera. Linia `[error] Shader ...` w
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
| F | włącza i wyłącza latarkę. Działa także przy wolnym kursorze. To samo robi pole `Flashlight on (key F)` w panelu Lights. Przy pustej baterii latarka się nie zapala, dopóki gracz nie zbierze kryształu | `NightMazeApp::onRender`, reguła baterii w `game::updateRound` w [`src/game/Round.cpp`](../../src/game/Round.cpp) |
| R | zaczyna rundę od nowa w tym samym labiryncie: kryształy wracają, bateria jest pełna, latarka włączona, brama zamknięta, gracz na starcie. Działa także przy wolnym kursorze i na karcie wygranej. To samo robi przycisk `Restart round (key R)` w panelu Gameplay | `NightMazeApp::onRender` i `NightMazeApp::beginRound` |
| Esc | przy przechwyconym kursorze oddaje kursor, przy wolnym zamyka program | `Application::run` w [`src/core/Application.cpp`](../../src/core/Application.cpp) |
| `~` (na lewo od `1`, `GLFW_KEY_GRAVE_ACCENT`) | pokazuje lub ukrywa panele debugowe. Pasek HUD zostaje | `DebugNightMazeApp::onRender` w [`src/main.cpp`](../../src/main.cpp) |

Klawisze są ignorowane, dopóki aktywny jest widżet panelu ImGui (na przykład trwa
wpisywanie wartości): klawiatura należy wtedy do panelu. Opis w
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6. Kliknięcie w panel nie
przechwytuje kursora, a przy przechwyconym kursorze panele nie reagują na mysz: żeby
przesunąć suwak, trzeba najpierw nacisnąć Esc. Obrót kamery opisuje
[`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcje 5
i 6, a ruch gracza [`../modules/game/player.md`](../modules/game/player.md). Obrót myszą i
lot były na Macu sprawdzone ręcznie w stanie M1. Chodzenia z kolizjami, sprintu oraz klawiszy N,
F i R nikt na Macu nie sprawdzał. Na macOS GLFW 3.4 nie ma surowego ruchu myszy, więc obrót korzysta z ruchu kursora po
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
> testy powstały na Windowsie (2026-10-05), a testy oświetlenia i rozgrywki później. Tam są
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
2026-10-05, po drugiej części M6, dla osiemnastu plików z testami: `ColliderTests.cpp` 19
przypadków, `CrystalTests.cpp` 14, `ExitTests.cpp` 11, `GrassTests.cpp` 9,
`ImageLoaderTests.cpp` 10, `LightingTests.cpp`
10, `LightTests.cpp` 20, `MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12,
`MazeTests.cpp` 8, `MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 19, `PlayerTests.cpp` 13,
`RoundTests.cpp` 25, `ShaderSourceTests.cpp` 22, `SkyboxTests.cpp` 5, `TangentTests.cpp` 9,
`TerrainTests.cpp` 27 i `TransformTests.cpp` 4:

```text
[doctest] test cases:    256 |    256 passed | 0 failed | 0 skipped
[doctest] assertions: 101232 | 101232 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Przed oświetleniem (2026-10-05) program miał osiem plików z testami, przed mapami
normalnych dwanaście, po M4 trzynaście (163 przypadki i 62220 asercji), po M5 szesnaście
(215 i 85098), a po pierwszej części M6 siedemnaście (221 i 85175), i odpowiednio
mniej przypadków. Blok wyżej to stan po drugiej części M6 (dziewiętnaście plików). Po
pierwszej części M7 plików było dwadzieścia jeden: doszły `ColorSpaceTests.cpp` (9
przypadków) i `FramebufferTests.cpp` (3), a `LightingTests.cpp` ma 11 (zgłoszone dla
Windowsa 269 przypadków i 102103 asercje). Po drugiej części M7 plików było
dwadzieścia dwa: doszedł `BloomTests.cpp` (7 przypadków, zgłoszone dla Windowsa 276
przypadków i 102139 asercji). Dziś, po trzeciej części M7, plików jest dwadzieścia
cztery: doszły `FogTests.cpp` (11 przypadków) i `VignetteTests.cpp` (7). Zgłoszone dla
Windowsa liczby tego stanu to 294 przypadki i 102412 asercji.

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
      zapisać liczbę przypadków i asercji (oczekiwane dla całego programu: 215 przypadków i
      85098 asercji, liczby z Windowsa z 2026-10-05)
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
Kod powstał na Windowsie (2026-10-05) i tam jest zmierzony: po M4, razem z testami linii
`map_Bump` i stycznych, 20 przypadków testowych i 1576 asercji w `tests/ObjLoaderTests.cpp`
(po M5 przypadków jest nadal 20, liczby asercji w tym pliku dla M5 nie zapisano).
Na Macu nikt go jeszcze nie kompilował:

- [ ] `src/assets/ObjLoader.*`, `src/gfx/Vertex.hpp`, `src/gfx/Mesh.*` i
      `tests/ObjLoaderTests.cpp` kompilują się bez ostrzeżeń pod `-Wall -Wextra -Wpedantic`
- [ ] przechodzą trzy asercje czasu kompilacji: `sizeof(gfx::Vertex)` równe 11 liczbom
      `float` (44 bajty, od M4 z polem `tangent`) i
      `std::is_standard_layout_v<gfx::Vertex>` w `Vertex.hpp` oraz
      `std::is_same_v<GLuint, std::uint32_t>` w `Mesh.cpp`
- [ ] `./build/debug/night_maze_tests --source-file='*ObjLoaderTests*'`: zapisać liczbę
      przypadków i asercji (oczekiwane 20 przypadków. Liczba 1576 asercji pochodzi z M4)
- [ ] przechodzi przypadek `parseObj: numbers` i podprzypadki z błędnymi liczbami w
      `parseObj: a bad line is reported with its line number`. Liczby czyta
      `std::istringstream` z klasycznym locale, a libc++ może traktować teksty graniczne
      (`+2`, `2.5E2`, `1.5x`, `--1`) inaczej niż biblioteka MSVC
- [ ] przechodzą dwa przypadki `loadObj: wall_straight.obj` i `wall_pillar.obj`
      (trzeci, dla płytki podłogi `floor_tile.obj`, zniknął w M6 razem z modelem): ścieżka
      z `NIGHT_MAZE_ASSETS_DIR` i ścieżki tekstur po
      `lexically_normal()` porównują się poprawnie także z separatorem `/`
- [ ] przechodzi przypadek `loadObj: material libraries and texture paths of files written by
      the test` (zapis do katalogu tymczasowego systemu i sprzątanie po sobie)
- [ ] sprawdzić, czy `std::from_chars` dla `float` kompiluje się Apple clangiem przy
      domyślnej wersji docelowej systemu. Jeśli tak, `parseFloat` w `ObjLoader.cpp` można
      uprościć. Dziś używa strumienia właśnie dlatego, że tego nie sprawdziłem
      ([`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), sekcja 5.4)

**Tekstury (temat 5): do zrobienia przy pierwszym buildzie tego kodu na Macu.** Kod powstał
na Windowsie (2026-10-05) i tam jest zmierzony: po M4, razem z testami map normalnych, 9
przypadków testowych i 57 asercji w `tests/ImageLoaderTests.cpp` (po M5 przypadków jest
nadal 9, liczby asercji w tym pliku dla M5 nie zapisano) oraz program z ukrytym oknem dla klasy `gfx::Texture2D`
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
      przypadków i asercji (oczekiwane 9 przypadków. Liczba 57 asercji pochodzi z M4)
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
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 215 przypadków testowych i
      85098 asercji, `Status: SUCCESS!` (liczby z Windowsa z 2026-10-05, razem z testami
      oświetlenia, map normalnych i rozgrywki)
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
      `textured.vert + textured.frag: OK` i `color.vert + color.frag: OK` (pięć
      plików oświetlenia jest na liście M4 niżej, a uniform `uEmissive`, który doszedł w
      M5, na liście M5). Sterownik NVIDII na Windowsie przyjmuje je bez uwag, ale jest
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
      usunięciu `imgui.ini` jedenaście paneli powinno stać tak samo jak na Windowsie i nie
      zasłaniać się (panele Camera i Gameplay, pod nimi Terrain i Grass, a pod nimi Framebuffers, zwinięte do pasków tytułu). Zapisać, czy plan w panelu Maze i podglądy tekstur w panelu Assets mają
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
      `textured.vert + textured.frag: OK` w panelu Shaders. Oczekiwane: podpowiedź z dwiema
      pełnymi ścieżkami i poprawnymi polskimi literami. Na
      Windowsie zmierzone jest tylko to, że czcionka te litery rysuje
- [ ] **kolory motywu**: tło paneli granatowe i lekko przezroczyste, tekst jasny, tekst
      błędu w panelu Shaders czytelny. Kontrast jest policzony z liczb, ale ekran Maca ma
      inny profil kolorów niż monitor, na którym motyw był oglądany
- [ ] **linie kształtów kolizji mają 1 piksel framebuffera**: szerokość linii zostaje domyślna,
      bo profil Core na macOS nie obsługuje grubszych. Na ekranie Retina to połowa punktu.
      Zapisać, czy żółte i zielone linie są czytelne (pozostałe kolory: lista M5 niżej)
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
- [ ] okno 1280 x 720: jedenaście paneli nie zasłania się nawzajem (Renderer nad Lights po
      lewej, Maze nad Assets po prawej, Collision i Shaders na dole między kolumnami,
      Camera i Gameplay zwinięte u góry, Terrain i Grass zwinięte w drugim rzędzie pod nimi). Zapisać, czy panele Renderer, Lights, Maze, Collision i
      Shaders pokazują całą zawartość bez przewijania: nazwa karty Apple w panelu Renderer
      ma inną długość niż na Windowsie, a wysokości paneli są dobrane do zawartości
- [ ] panel Camera (rozwinąć strzałką w pasku tytułu): `Mode: walking`, `Player feet` 1, 0, 1, `Eye: 1.00, 1.70, 1.00`,
      `Pitch` 0, `Walk speed` 3.0, `Sprint speed` 5.5, `Fly speed` 6.0
- [ ] panele Maze i Collision: `In play: 10 x 10 cells, seed 1`, `Walls: 121, pillars: 121`,
      `Crystals: 13, exit in cell (6, 5)`, `Boxes: 121 walls, 121 pillars, 1 gate`,
      `All boxes: 243, pickup spheres: 13`
- [ ] chodzenie: po kliknięciu w scenę W, A, S, D chodzą poziomo, także ze wzrokiem w
      podłodze, lewy Shift przyspiesza
- [ ] ściana zatrzymuje gracza, a ruch ukosem w ścianę zamienia się w ślizganie wzdłuż niej
- [ ] gracz przytulony do długiej ściany nie zahacza o słupki stojące co 2 m
- [ ] narożnik wewnętrzny zatrzymuje, narożnik zewnętrzny daje się obejść bez zacięcia
- [ ] nie da się wyjść poza labirynt
- [ ] klawisz N: `Mode: noclip (free flight)`, pole `Noclip (key N)` w panelu Collision jest
      zaznaczone, spacja wznosi, lewy Shift opuszcza, W leci wzdłuż kierunku patrzenia przez
      ściany
- [ ] widok z góry w trybie noclip zgadza się z planem w panelu Maze: ściany, kryształy
      tam, gdzie plan ma kropki, i brama przy komórce z zielonym prostokątem
- [ ] drugi raz N w powietrzu: gracz od razu stoi na podłodze
- [ ] `Draw collision shapes`: żółte linie na ścianach i słupkach, zielone pudełko gracza
      widoczne pod nogami i nad głową, linie nie migoczą
- [ ] `Regenerate` z innym rozmiarem i ziarnem: nowy plan, gracz na starcie (`Player feet`
      1, 0, 1, `Pitch` 0), nowa runda na pasku HUD, a tryb noclip, prędkości, tryb widoku i
      rysowanie kształtów kolizji zostają bez zmian
- [ ] `Random seed`: nowa liczba w polu `Seed` i od razu nowy labirynt
- [ ] panel Assets, `View mode`: `Normals as colour` (podłoże w odcieniach jasnej zieleni, ściany w
      kolorach zależnych od kierunku, a przy zaznaczonym polu `Normal mapping` i trybie
      `Lighting` innym niż `Gouraud` z rysunkiem fug z map normalnych), `UVs as colour` (czerwono-zielone powtarzające się
      przejścia), `Textured` przywraca obraz
- [ ] panel Assets, `Filter`: `Nearest` (kwadratowe teksele z bliska, migotanie w oddali),
      `Bilinear` (gładko z bliska, migotanie w oddali), `Trilinear` (spokojnie w oddali)
- [ ] panel Assets, `Anisotropy` (jeśli dostępna): większa wartość wyostrza podłoże widziane
      pod płaskim kątem
- [ ] podglądy tekstur w panelu Assets stoją prosto i nie reagują na filtr
- [ ] `Reload shaders` po zmianie w `assets/shaders/textured.frag` (na przykład
      `fragColor = vec4(texel * uTint * (vec3(1.0) + uEmissive) * vec3(1.0, 0.5, 0.5), 1.0);`),
      przy `Lighting`
      ustawionym na `Unlit` (w pozostałych trybach labirynt rysują programy `lit` albo
      `gouraud`): labirynt robi się czerwonawy, wszystkie cztery linie panelu kończą się
      napisem `: OK`. Z błędem składni: czerwona linia
      `textured.vert + textured.frag: FAILED, the previous program stays in use` z
      komunikatem sterownika pod nią, labirynt rysuje się poprzednią wersją. Przywrócić
      plik
- [ ] celowo brakująca tekstura: zamknąć program, zmienić nazwę
      `assets/textures/wall_stone.png` (na Macu to plik w repozytorium, bo `assets` obok
      programu jest dowiązaniem), uruchomić. Oczekiwane: ściany i słupki bez rysunku kamienia
      (białe w trybie `Unlit`, w kolorze padającego światła w trybach z oświetleniem, przy
      `Phong` i `Blinn-Phong` nadal z reliefem fug, bo mapa normalnych wczytuje się osobno), jedna
      linia `[error]`, w panelu Assets `no texture (white)` i sekcja `Failed to load`. **Przywrócić
      nazwę pliku** i sprawdzić `git status`. (Do M5 ten punkt używał tekstury
      `floor_stone.png` płytki podłogi, której już nie ma.)
- [ ] zmiana rozmiaru okna, tryb pełnoekranowy macOS i powrót: obraz wypełnia okno, ściany
      i słupki zachowują proporcje, bez linii `[error]`
- [ ] okno powiększone na cały ekran po usunięciu `imgui.ini`: zapisać, gdzie stoją panele.
      Układ startowy jest liczony raz, w pierwszej klatce, od rogów okna w tej chwili, więc
      po późniejszym powiększeniu panele zostają na miejscach dla 1280 x 720

### M4 (oświetlenie) na macOS: lista w całości otwarta

Druga część M4, mapy normalnych, ma własną listę zaraz po tej. Pierwsza część M4 (światła, cztery tryby cieniowania, blok uniformów, `#include` w
shaderach, panel Lights, układ siedmiu paneli) powstała na Windowsie 2026-10-05 i tam jest
zbudowana i częściowo sprawdzona ([`build-windows.md`](build-windows.md), sekcja 13).
Lista powstała dla stanu M4, w którym światła punktowe wisiały w ślepych zaułkach i były
oznaczone kostkami. M5 przeniósł je nad kryształy i usunął kostki, więc punkty poniżej
są przepisane na dzisiejszy program: paneli jest jedenaście, programów shaderów osiem,
źródłem światła punktowego jest kryształ, podłogą jest teren (M6), a scena jest rysowana
do bufora HDR z nowymi wartościami świateł (M7).
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
      (stałe `..._PLACEMENT` bez pola `collapsed` w `PanelLayout.hpp`. W M4 było tu też
      `gfx::Vertex{.position = ...}` w `LightRig.cpp`: kostki znacznika od M5 nie ma),
      `std::function` jako typ `gfx::IncludeReader`,
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
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 215 przypadków testowych i
      85098 asercji, `Status: SUCCESS!` (liczby z Windowsa, po M5)
- [ ] cztery pliki testów oświetlenia osobno, opcją `--source-file`: `'*ShaderSourceTests*'` 22
      przypadki, `'*LightTests*'` 20, `'*LightingTests*'` 10 (w M4 było 17: w M5 test
      funkcji `isDeadEnd` przeszedł do `MazeTests.cpp`, a sześć testów świateł w ślepych
      zaułkach zniknęło razem z tym kodem), `'*TransformTests*'` 4
      (liczby przypadków z Windowsa). Uwaga: wzorzec `'*LightTests*'` nie pasuje do
      `LightingTests.cpp`, a `'*Light*'` pasuje do obu plików
- [ ] przechodzą przypadki zależne od zaokrągleń `float` na ARM: `a light made for a radius
      has 5 % of its brightness left at that radius`, `directionFromAngles gives a vector
      of length 1` i cztery przypadki macierzy normalnych z `tests/TransformTests.cpp`
      (tolerancja 0,0001)
- [ ] przechodzi przypadek `the default maze has 13 crystals and its exit in the cell
      (6, 5)` z `tests/CrystalTests.cpp`: zależy od tego samego generatora co labirynt
      wzorcowy, więc jest drugim pomiarem, że oba systemy budują ten sam labirynt. W M4 tę
      rolę miał test liczby świateł w ślepych zaułkach (11), usunięty w M5
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
      niezebranych kryształów (13 na starcie w labiryncie startowym), i że po `Regenerate`
      z `Width` 4, `Height`
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
- [ ] **Retina a układ paneli**: po usunięciu `imgui.ini` Renderer stoi nad Lights
      w lewej kolumnie, Maze nad Assets w prawej, Collision i Shaders na dole, Camera i
      Gameplay zwinięte u góry, nic się nie zasłania. Zapisać, czy panel Lights (przy zwiniętej
      grupie `Moon (directional)`) pokazuje całą zawartość bez przewijania: jego wysokość
      jest dobrana do zawartości zmierzonej na Windowsie
- [ ] **źródła świateł na Retinie**: świecące kryształy (wysokie na 0,5 m) są czytelne z
      odległości kilku komórek. W M4 ten punkt dotyczył kostek o boku 0,14 m

**Test ręczny (ta sama lista co w [`build-windows.md`](build-windows.md), sekcja 13.2)**

Na macOS po zmianie pliku shadera nie trzeba niczego kopiować: wystarczy `Reload shaders`.
Nazwy widżetów są zapisane tak jak w kodzie paneli.

- [ ] przygotowanie: usunąć `imgui.ini` z katalogu, z którego startuje program, zbudować,
      uruchomić `./build/debug/night_maze`
- [ ] start: nocna scena w trybie Blinn-Phong, w terminalu nie ma linii `[error]`
- [ ] panel Renderer: lista `Lighting` z wybraną pozycją `Blinn-Phong`
- [ ] panel Lights: edytor `Ambient`, grupy `Moon (directional)` (zwinięta),
      `Flashlight (spot)`, `Point lights (crystals)` i `Highlight (specular)`, linia
      `Lit: 13 of 13 crystals (at most 16)`
- [ ] panel Shaders: cztery linie zakończone `: OK` (`textured`, `color`, `lit`,
      `gouraud`), podpowiedź nad linią z dwiema pełnymi ścieżkami
- [ ] rozwinięcie panelu Camera strzałką w pasku tytułu: otwiera się w dół, kończy się tuż
      nad dolnym rzędem paneli i nie zasłania żadnego innego panelu
- [ ] klawisz F wyłącza i włącza latarkę, także przy wolnym kursorze, a pole
      `Flashlight on (key F)` w panelu Lights zmienia się razem z nią. Kliknięcie pola robi
      to samo
- [ ] stożek latarki zostaje na środku obrazu podczas chodzenia do przodu, bokiem i biegu
- [ ] `Lighting`, `Unlit`: labirynt równo jasny, kryształy jaśniejsze od ścian
- [ ] `Lighting`, `Gouraud`: światło liczone w wierzchołkach, łagodne przejścia między
      narożnikami, kryształy świecą także w tym trybie
- [ ] `Lighting`, `Phong` i `Blinn-Phong`: okrągła plama latarki z miękkim brzegiem
- [ ] Gouraud a Phong na ścianie, twarzą do niej z około 2 m: w `Phong` okrągła plama, w
      `Gouraud` plama znika albo rozmazuje się wzdłuż krawędzi trójkątów (duża ściana ma
      wierzchołki tylko w narożnikach)
- [ ] Gouraud a Phong u podstawy słupka: zapisać, jak wygląda podłoże wokół słupka w obu
      trybach. Od M6 podłoże ma wierzchołki co 0,5 m (siatka terenu), więc różnica jest
      na nim dużo mniejsza niż na dawnej płytce podłogi o czterech wierzchołkach
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
      nie ma). Policzyć z góry (klawisz N, spacja) 13 kryształów i porównać z kropkami
      na planie w panelu Maze. `Point intensity` 0 gasi kałuże światła, a kryształy
      świecą dalej
- [ ] `Cone`: większe `outer` poszerza plamę, `inner` bliskie `outer` daje ostry brzeg,
      pola `inner` nie da się przeciągnąć powyżej `outer`. `Beam range`: mała wartość
      skraca zasięg latarki
- [ ] `Regenerate` z innym ziarnem: kryształy i ich światła są w komórkach nowego
      labiryntu, liczba w linii `Lit: ...` odpowiada nowemu planowi
- [ ] celowy błąd w `assets/shaders/common/lighting.glsl`, potem `Reload shaders`: linie
      `lit.vert + lit.frag: FAILED, the previous program stays in use` i
      `gouraud.vert + gouraud.frag: FAILED, the previous program stays in use` są
      czerwone, komunikat pod nimi nazywa plik `common/lighting.glsl` i linię, obraz się
      nie zmienia. Potem `git checkout assets/shaders` i `Reload shaders`: wszystkie cztery
      linie kończą się napisem `: OK`
- [ ] panel Assets, `View mode` równy `Normals as colour` i `UVs as colour` przy trybie
      `Blinn-Phong`: labirynt, kryształy i bramę rysuje program `textured`, bez świateł i
      bez blasku kryształów. Widok normalnych pokazuje normalne używane przez wybrany tryb
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
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 215 przypadków testowych i
      85098 asercji, `Status: SUCCESS!` (liczby z Windowsa z 2026-10-05, po M5)
- [ ] pliki testów osobno, opcją `--source-file`: `'*TangentTests*'` 9 przypadków,
      `'*ObjLoaderTests*'` 20, `'*ImageLoaderTests*'` 9, `'*LightingTests*'` 10 (liczby
      przypadków z Windowsa po M5. Liczby asercji w plikach są zapisane tylko dla stanu
      M4: 177, 1576, 57 i, przy 17 przypadkach `LightingTests.cpp`, 133)
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

- [ ] start w trybie `Blinn-Phong`: fugi są rowkami (nie wałkami) na
      ścianach wzdłuż X, wzdłuż Z i na słupku, a kamyki podłoża są wypukłe (od M6 mapa
      normalnych podłoża to `ground_normal.png`: ubita ziemia z kamykami, bez fug)
- [ ] światło z boku: po przejściu latarki na drugą stronę jasne i ciemne skosy zamieniają
      się miejscami
- [ ] pole `Normal mapping` w panelu Assets: odznaczone daje płaskie ściany, zaznaczone
      przywraca relief od razu. To samo przy `Phong`
- [ ] `Lighting` równe `Gouraud` i `Unlit`: pole niczego nie zmienia w obrazie
- [ ] `View mode` równe `Normals as colour`: rysunek fug widać przy `Unlit`, `Phong` i
      `Blinn-Phong`, nie widać go przy `Gouraud` ani przy odznaczonym polu
- [ ] lista `Models`: linia `normal map:` pod każdą częścią (pięć modeli). Lista
      `Textures`: osiem tekstur, cztery jasnoniebieskie, podglądy stoją prosto
- [ ] ekran Retina: zapisać, czy relief w oddali migocze w ruchu przy filtrze `Trilinear` i
      czy anizotropia to zmienia (na Windowsie oceniono to tylko na nieruchomych klatkach)
- [ ] celowo brakująca mapa normalnych: zmienić nazwę
      `assets/textures/wall_stone_normal.png` (na Macu to plik w repozytorium, bo `assets`
      obok programu jest dowiązaniem), uruchomić. Oczekiwane: ściany i słupki z teksturą
      koloru, ale płaskie pod latarką, jedna linia `[error]`, w panelu Assets `normal map: none
      (flat)` i sekcja `Failed to load`. (Do M5 ten punkt używał mapy płytki podłogi.) **Przywrócić nazwę pliku** i sprawdzić `git status`

**Skrypty Blendera**

- [ ] uruchomić na Macu `blender --background --factory-startup --python
      tools/blender/make_textures.py` i skrypty modeli tą samą wersją Blendera
      (5.2.1), potem `git status` i `git diff --stat`. Na Windowsie dwa uruchomienia dały
      w M4 identyczne skróty dziesięciu ówczesnych plików (dziś plików jest dwadzieścia,
      lista M5 niżej). **Nie wiadomo, czy Mac da te same bajty**: mapy
      normalnych przechodzą przez `np.linalg.norm`, dzielenie i zaokrąglanie do 8 bitów, a
      NumPy na procesorze ARM może dać wynik różny o ostatni bit, co po zaokrągleniu zmienia
      pojedyncze bajty. Inna może być też kompresja PNG. Zapisać, które pliki się różnią
- [ ] jeśli pliki PNG się różnią: porównać piksele, nie bajty pliku, i zapisać największą
      różnicę. Różnica o 1 na 255 w pojedynczych tekselach nie zmienia obrazu, ale wtedy
      zasada "skrypt odtwarza pliki co do bajta" obowiązuje tylko w obrębie jednego systemu
- [ ] po próbie przywrócić pliki z repozytorium (`git checkout assets`), żeby testy
      czytały te same mapy co na Windowsie

### M5 (rozgrywka) na macOS: lista w całości otwarta

Kamień milowy M5 (kule kolizji, wyjście w komórce najdalszej od startu, kryształy i brama
jako modele, bateria latarki z migotaniem, pasek HUD z kartą wygranej, panel Gameplay,
klawisz R, układ ośmiu paneli, usunięcie kostki z M1 i programu `basic`) powstał na
Windowsie 2026-10-05 i tam jest zbudowany i przetestowany testami jednostkowymi
([`build-windows.md`](build-windows.md), sekcja 14). **Na macOS nikt go nie zbudował ani
nie uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Na Windowsie też nikt jeszcze
nie przeszedł rozgrywki ręcznie. Oczekiwania wynikają z kodu, z testów jednostkowych i z
tego, co zmierzono na Windowsie. Opis kodu:
[`../modules/game/gameplay.md`](../modules/game/gameplay.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md) (kule),
[`../modules/game/flashlight.md`](../modules/game/flashlight.md) (bateria, światła
kryształów).

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/game/Exit.*`, `Crystals.*`, `Round.*`, `GameplayRenderer.*`, `ModelDraw.*`,
      `src/debug/Hud.*`, `src/debug/panels/GameplayPanel.*`, `tests/ExitTests.cpp`,
      `tests/CrystalTests.cpp`, `tests/RoundTests.cpp`. Zmienione między innymi:
      `src/scene/Collider.*`, `src/game/Maze.*`, `MazeLayout.*`, `MazeWorld.*`,
      `Lighting.*`, `LightRig.*`, `MazeRenderer.*`, `ColliderLines.*`, `NightMazeApp.*`,
      `ShaderUniforms.hpp`, `src/main.cpp`, `src/debug/DebugContext.hpp`, `DebugUI.*`,
      `PanelLayout.hpp`, `Theme.hpp` oraz panele Maze, Collision i Lights. Miejsca warte
      uwagi: `bool operator==(const MazeCell& other) const = default;` w `Maze.hpp`
      (porównanie generowane przez kompilator, C++20), inicjalizatory desygnowane
      `MazeCell{.x = 0, .z = 0}` w `MazeWorld.cpp` i pól `.center` i `.radius` struktury
      `scene::Sphere` w `Round.cpp`,
      `std::span` zbudowany ze wskaźnika i liczby 1 (`std::span<const scene::Aabb>(&playerBox, 1)`
      w `NightMazeApp.cpp`), `std::array` ze `static_cast<std::size_t>` stałej jako
      rozmiarem w `GameplayRenderer.hpp`, `std::snprintf` do `std::array<char, ...>` w
      `Hud.cpp`. Zapisać każde ostrzeżenie
- [ ] `./build/debug/night_maze_tests` i to samo dla Release: 215 przypadków testowych i
      85098 asercji, `Status: SUCCESS!` (liczby z Windowsa z 2026-10-05)
- [ ] trzy nowe pliki testów osobno, opcją `--source-file`: `'*ExitTests*'` 11 przypadków,
      `'*CrystalTests*'` 14, `'*RoundTests*'` 25, do tego `'*ColliderTests*'` 19 (siedem
      nowych przypadków kul). Liczb asercji w plikach dla M5 nie zapisano
- [ ] **najważniejszy punkt tej listy**: przechodzą testy `golden maze: 4 x 4 cells from
      seed 1 has exactly these two crystals`, `golden maze: 4 x 4 cells from seed 1 has its
      exit in the dead end (3, 1)` i `the default maze has 13 crystals and its exit in the
      cell (6, 5)`. Kryształy losuje `std::mt19937` z ziarna labiryntu powiększonego o stałą
      i własna funkcja `game::randomBelow`, bez rozkładów z biblioteki standardowej, więc
      wynik ma być ten sam na obu systemach
      ([`../decisions/deterministic-random.md`](../decisions/deterministic-random.md)). To
      jest pomiar, że macOS i Windows stawiają kryształy, wyjście i bramę w tych samych
      komórkach
- [ ] przechodzą przypadki zależne od zaokrągleń `float` na ARM: `the gate needs 70 percent
      of the crystals, rounded up, and at least one` (iloczyn `0.7F * 10` nie może
      zaokrąglić się w górę do 8), `a low battery flickers: the factor stays in 0 to 1,
      dips, and repeats exactly` (dwa sinusy z `<cmath>`), `spheres that only touch do not
      overlap` i `a sphere that only touches a box does not overlap it` (ostre nierówności
      na odległościach), `a crystal bobs straight up and down within its amplitude`
- [ ] clang-format i clang-tidy (`make check`) bez uwag na nowych i zmienionych plikach. Na
      Windowsie tego wyniku dla M5 nie zapisano

**Ryzyka specyficzne dla macOS**

- [ ] **kompilator GLSL Apple i uniform `uEmissive`**: po starcie w terminalu nie ma linii
      `[error] Shader ...`, a panel Shaders pokazuje cztery linie zakończone `: OK`. Nowe
      w shaderach są tylko deklaracja `uniform vec3 uEmissive;` w `lit.frag`,
      `gouraud.frag` i `textured.frag` oraz wyrażenia `surface * (lighting.diffuse +
      uEmissive)` i `texel * uTint * (vec3(1.0) + uEmissive)`. W `textured.frag` uniform
      czyta tylko jedna gałąź `if` (widok `Textured`)
- [ ] **start bez plików `basic.*`**: w terminalu nie ma linii `[error] Shader file cannot
      be opened` z nazwą `basic.vert` albo `basic.frag`. Tych plików nie ma już ani w
      repozytorium, ani w kodzie. Na macOS katalog `assets` obok programu jest
      dowiązaniem, więc stara kopia plików nie może zostać, inaczej niż na Windowsie
- [ ] **klawisz paneli na klawiaturze Maca innej niż amerykańska**: panele chowa klawisz
      `GLFW_KEY_GRAVE_ACCENT` ([`src/main.cpp`](../../src/main.cpp)). Stała GLFW oznacza
      klawisz fizyczny według układu amerykańskiego, a nie znak, który klawisz wpisuje. Na
      klawiaturze amerykańskiej to klawisz na lewo od `1`. Na klawiaturze Maca w innym
      układzie (na przykład ISO, z dodatkowym klawiszem obok lewego Shifta) klawisz o tym
      kodzie może leżeć w innym miejscu. Sprawdzić, który klawisz fizycznie chowa panele,
      i zapisać wynik. Jeśli żaden, panele zostają widoczne na stałe: rozgrywce to nie
      przeszkadza, bo pasek HUD jest rysowany niezależnie od paneli. To punkt do
      sprawdzenia, nie fakt: na Macu z taką klawiaturą nikt programu nie uruchomił
- [ ] **rozmiar HUD na Retinie**: pasek HUD i karta wygranej mnożą swoje wymiary przez
      `ImGui::GetStyle().FontScaleDpi` (`src/debug/Hud.cpp`), czyli przez tę samą skalę,
      którą `applyTheme` dostaje z `ImGui_ImplGlfw_GetContentScaleForWindow`
      (`src/debug/DebugUI.cpp`). Jeśli ta funkcja zwraca na Macu 1 (założenie z listy
      "M2 + M3 na macOS" wyżej, niesprawdzone), pasek baterii ma 230 punktów szerokości,
      a pasek HUD stoi 46 punktów od górnej krawędzi, tak jak na Windowsie przy 100%.
      Zapisać, czy pasek HUD ma rozsądny rozmiar, czy stoi tuż pod paskami tytułu paneli
      Camera i Gameplay i czy tekst jest ostry. Dwa razy za duży albo dwa razy za mały
      HUD oznaczałby, że założenie o skali jest fałszywe
- [ ] **tytuł karty wygranej**: napis `You escaped` jest rysowany tą samą czcionką w
      rozmiarze 1,8 raza większym (`ImGui::PushFont` z rozmiarem). Zapisać, czy na Retinie
      jest ostry, a nie rozciągnięty z mniejszej tekstury
- [ ] **Retina a układ paneli** (w M5 ośmiu, dziś jedenastu): po usunięciu `imgui.ini`
      Renderer stoi nad Lights w
      lewej kolumnie, Maze nad Assets w prawej, Collision i Shaders na dole, a Camera i
      Gameplay są zwinięte u góry, między kolumnami, z paskami Terrain i Grass pod sobą. Nic się nie zasłania
- [ ] **kule kolizji liniami o grubości 1 piksela framebuffera**: z zaznaczonym
      `Draw collision shapes` zapisać, czy turkusowe okręgi kul kryształów, zielona kula
      gracza, pomarańczowe pudełko bramy i pudełko strefy wyjścia w kolorze magenty są
      czytelne na ekranie Retina (linia ma tam pół punktu szerokości)
- [ ] **liczba świateł po zebraniu kryształu**: pętla po `uPoints` w
      `common/lighting.glsl` wychodzi przez `break` na `uPointCount`. Zebrać kryształ i
      sprawdzić, że jego plama światła znika od razu i że żadne inne światło nie gaśnie
      ani nie zmienia miejsca. Po `Regenerate` z `Width` 4, `Height` 4, `Seed` 1 świecą
      dokładnie dwa
- [ ] **FPS przy 13 kryształach na Retinie**: zapisać FPS z panelu Renderer w trybach
      `Gouraud` i `Blinn-Phong` na starcie i w labiryncie 40 na 40 (16 świateł
      punktowych liczonych dla każdego fragmentu, a fragmentów jest cztery razy więcej
      niż na Windowsie przy tym samym oknie)

**Test ręczny (ta sama lista co w [`build-windows.md`](build-windows.md), sekcja 14.2)**

Listy z Windowsa nie powtarzam punkt po punkcie: na Macu przechodzi się ją w całości,
z programem `./build/debug/night_maze` uruchomionym po usunięciu `imgui.ini`. Poniżej są
jej grupy, każda do odhaczenia po przejściu wszystkich punktów grupy z Windowsa.

- [ ] start: nocna scena z kryształami, w terminalu nie ma linii `[error]`, są linie
      `[info] Loaded model: ...` dla pięciu modeli (w M5 sześciu) i
      `[info] Loaded texture: ...` dla ośmiu tekstur
- [ ] układ paneli i pasek HUD: `Crystals`, `0 / 10`, `(of 13)`, czas `0:00`, pasek
      baterii z `100%`. Panel Gameplay po rozwinięciu, linie `Crystals: 13, exit in cell
      (6, 5)` w panelu Maze, `Boxes: 121 walls, 121 pillars, 1 gate` i `All boxes: 243,
      pickup spheres: 13` w panelu Collision, `Lit: 13 of 13 crystals (at most 16)` w
      panelu Lights
- [ ] kryształy i brama w obrazie: dwa kształty kryształów, kołysanie, obrót, blask i
      pulsowanie, drewniana brama przy komórce wyjścia
- [ ] HUD przy ukrytych panelach (klawisz z punktu o klawiaturze wyżej)
- [ ] zbieranie kryształów: licznik, bateria, gasnące światło, pierścień na planie,
      suwaki `Recharge` i `Pickup radius`
- [ ] bateria i latarka: zużycie, `Battery lifetime`, `Battery drains`, migotanie poniżej
      progu `Flicker below`, pusta bateria z podpowiedzią `Battery empty. Find a crystal.`
      i z podpowiedzią nad polem `Flashlight on (key F)`, klawisz F przy pustej baterii,
      kryształ zebrany przy pustej baterii
- [ ] brama: `Crystals needed`, otwarta brama zostaje otwarta, opadanie w 1,5 sekundy,
      brama po dziesiątym z 13 kryształów z podpowiedzią `The gate is open. Find the
      exit.`
- [ ] wyjście i wygrana: karta `You escaped` z czasem, kryształami i `R: play again`,
      czas stoi, scena żyje dalej, noclip przez zamkniętą bramę nie wygrywa
- [ ] restart rundy: klawisz R w środku rundy i z karty wygranej, przycisk
      `Restart round (key R)`, co R zmienia, a czego nie
- [ ] przez cały test w terminalu nie pojawia się żadna linia `[error]`

**Skrypty Blendera**

- [ ] uruchomić na Macu `blender --background --factory-startup --python
      tools/blender/make_all.py`, potem `git status` i `git diff --stat`. Skrypt pisze dziś
      w `assets/textures/` dziewięć plików PNG (osiem tekstur i `heightmap.png`), w
      `assets/skybox/` sześć, a do tego pięć plików OBJ i pięć MTL (w M5 doszły `crystal_a`,
      `crystal_b`, `gate`, `crystal.png`, `gate_wood.png` i ich mapy normalnych, w M6
      zniknęła płytka podłogi z teksturami `floor_stone`, a doszły `ground.png`,
      `ground_normal.png`, niebo i mapa wysokości). Dla tych nowych plików próby
      "dwa uruchomienia dają te same bajty" nie zapisano nawet na Windowsie. Zapisać, które
      pliki się różnią, a po próbie przywrócić pliki z repozytorium (`git checkout assets`)

### M6, część 1 (skybox) na macOS: lista w całości otwarta

Pierwsza część kamienia milowego M6 (tekstura sześcienna `gfx::Cubemap`, niebo
`game::Skybox`, piąty program shaderów `skybox`, `assets::RowOrder` w loaderze obrazów,
sześć plików w `assets/skybox/`, pole `Skybox` i suwak `Sky brightness` w panelu Renderer)
powstała na Windowsie 2026-10-05 i tam jest zbudowana i przetestowana testami
jednostkowymi ([`build-windows.md`](build-windows.md), sekcja 15). **Na macOS nikt jej nie
zbudował ani nie uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Oczekiwania
wynikają z kodu, z testów i z tego, co zmierzono na Windowsie. Opis kodu:
[`../modules/renderer/skybox.md`](../modules/renderer/skybox.md),
[`../modules/gfx/cubemap.md`](../modules/gfx/cubemap.md). Teren i trawa, czyli reszta M6,
mają własną listę zaraz po tej. Liczby w punktach niżej (221 przypadków, pięć programów)
to stan po pierwszej części: dzisiejsze są w następnej liście.

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/gfx/Cubemap.*`, `src/game/Skybox.*`, `tests/SkyboxTests.cpp`. Zmienione:
      `src/assets/ImageLoader.*`, `src/game/NightMazeApp.*`, `Lighting.hpp`,
      `ShaderUniforms.hpp`, `src/main.cpp`, `src/debug/DebugContext.hpp`, `DebugUI.cpp`,
      `PanelLayout.hpp`, `src/debug/panels/RendererPanel.*`, `tests/ImageLoaderTests.cpp`.
      Miejsca warte uwagi: inicjalizatory desygnowane w stałej tablicy
      `constexpr std::array<gfx::Vertex, CORNER_COUNT>` (`gfx::Vertex{.position = {...}}`,
      C++20), zwrot obiektu tylko przenoszalnego listą w klamrach
      (`return {first.width, first.channels, pixels};` i `return {};` w `loadSkyCubemap`),
      pętla po liście w klamrach w teście (`for (const float firstSign : {-1.0F, 1.0F})`)
      i lambda w `std::ranges::any_of` w `Cubemap.cpp`
- [ ] `ctest --test-dir build/debug --output-on-failure`: wszystkie przypadki
      przechodzą (po pierwszej części M6 było ich 221, po drugiej 256, po pierwszej
      części M7 zgłoszone 269, po drugiej 276, a dziś, po trzeciej, 294). Pięć przypadków `SkyboxTests.cpp` czyta pliki PNG zapisane na Windowsie:
      powinny przejść bez zmian, bo to te same bajty z repozytorium

**Shadery i OpenGL**

- [ ] start gry bez linii `[error]`: kompilator GLSL Apple nie widział jeszcze
      `skybox.vert` ani `skybox.frag`. Miejsca warte uwagi: `position.xyww`,
      `mat4(mat3(uView))`, uniform `samplerCube`, gałąź `if (uViewMode != 0)`
- [ ] sześć linii `[info] Loaded sky face: ...` w terminalu. Na macOS katalog `assets` obok
      programu jest dowiązaniem, więc nowy podkatalog `skybox` jest widoczny bez kopiowania
- [ ] niebo jest widoczne nad ścianami i **nie migocze**. Shader daje niebu głębię równą
      dokładnie 1,0 i polega na teście `GL_LEQUAL` z wartością po `glClear`. Na
      sterowniku NVIDII to działa. Jeśli sterownik Apple policzy głębię o włos większą,
      niebo zniknie w całości albo w pasach: zapisać, co widać
- [ ] brak szwów na krawędziach sześcianu: `GL_TEXTURE_CUBE_MAP_SEAMLESS` jest w rdzeniu od
      3.2, więc kontekst 4.1 Core powinien go mieć. Sprawdzić narożniki (yaw 45, 135, 225,
      315 przy pitch około 35)
- [ ] przez cały test w terminalu nie pojawia się żadna linia z nazwą błędu OpenGL
      (`GL_...`)

**Ekran Retina**

- [ ] ostrość nieba: ściana tekstury ma 1024 piksele na 90 stopni, a framebuffer na
      ekranie Retina ma dwa razy więcej pikseli niż okno. Jeden teksel nieba zajmuje wtedy
      około 2,7 piksela ekranu zamiast około 1,3. Zapisać, czy gwiazdy są akceptowalnie
      ostre, czy ściany trzeba wygenerować w rozmiarze 2048 (stała `SIZE` w
      `make_skybox.py`)
- [ ] panel Renderer mieści pole `Skybox` i suwak `Sky brightness` bez paska przewijania
      przy skali ekranu Maca, a panel Lights pod nim się przewija

**Panel i przełączniki**

- [ ] cała lista ręczna z [`build-windows.md`](build-windows.md), sekcja 15.2: przełącznik,
      jasność, szwy, księżyc przy yaw 205 i pitch 50, suwak `Moon yaw` bez wpływu na
      tarczę, widoki diagnostyczne, `Reload shaders` (dziś przy dziesięciu programach, na macOS bez
      kroku kopiowania assetów), brak pliku

**Skrypt Blendera**

- [ ] uruchomić na Macu `blender --background --factory-startup --python
      tools/blender/make_skybox.py`, potem `git status`. Generator liczb losowych numpy
      daje na obu systemach te same liczby dla tego samego ziarna, ale funkcje
      zmiennoprzecinkowe (`exp`, `sin`, `arccos`) mogą różnić się w ostatnim bicie, a po
      ditheringu i zaokrągleniu to wystarczy, żeby zmienić pojedyncze bajty. Zapisać, czy
      pliki się różnią i czy testy nadal przechodzą, a po próbie przywrócić pliki z
      repozytorium (`git checkout assets`)

### M6, część 2 (teren i trawa) na macOS: lista w całości otwarta

Druga część kamienia milowego M6 (teren z mapy wysokości `game::Terrain` w miejscu płytek
podłogi, wszystko w labiryncie postawione na terenie, `game::TerrainRenderer`, trawa
`game::placeGrass` i `game::GrassRenderer`, etap geometrii w `gfx::Shader`, szósty program
shaderów `grass` z plikiem `grass.geom`, prymityw `GL_POINTS` w `gfx::Mesh`, panele
Terrain i Grass w drugim rzędzie pasków tytułu, skrypt `make_heightmap.py`) powstała na
Windowsie 2026-10-05 i tam jest zbudowana i przetestowana testami jednostkowymi
([`build-windows.md`](build-windows.md), sekcja 16). **Na macOS nikt jej nie zbudował ani
nie uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Oczekiwania wynikają z kodu,
z testów i z tego, co zmierzono i zgłoszono na Windowsie. Opis kodu:
[`../modules/renderer/terrain.md`](../modules/renderer/terrain.md),
[`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md),
[`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md). Z tą częścią kod M6
jest kompletny na Windowsie, ale kamień nie jest zamknięty właśnie przez tę listę
i przez testy ręczne.

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/game/Terrain.*`, `src/game/TerrainRenderer.*`, `src/game/Grass.*`,
      `src/game/GrassRenderer.*`, `src/debug/panels/TerrainPanel.*`,
      `src/debug/panels/GrassPanel.*`, `tests/TerrainTests.cpp`, `tests/GrassTests.cpp`.
      Zmienione między innymi: `src/gfx/Shader.*`, `src/gfx/Mesh.hpp`,
      `src/game/MazeWorld.*`, `MazeLayout.*`, `Player.*`, `Round.*`, `Crystals.*`,
      `Exit.*`, `ModelDraw.*`, `NightMazeApp.*`, `src/debug/PanelLayout.*`, `Hud.cpp`.
      Miejsca warte uwagi: inicjalizatory desygnowane z pominiętymi polami
      (`{.position = tuft.position, .uv = {tuft.random, 0.0F}}` w `GrassRenderer.cpp`,
      `{.type = GL_VERTEX_SHADER, .path = &vertexPath}` w `Shader.cpp`), `std::max` z listą
      w klamrach i wiązanie strukturalne wyniku `std::ranges::minmax_element` w `Terrain.cpp`,
      stała `constexpr glm::mat4 IDENTITY{1.0F}` w `TerrainRenderer.cpp`, `std::lround`
      w `Grass.cpp` i przypisanie wewnątrz makra
      (`GL_CHECK(cullingWasOn = glIsEnabled(GL_CULL_FACE))`)
- [ ] `ctest --test-dir build/debug --output-on-failure`: wszystkie przypadki przechodzą.
      Po tej części było ich 256 (na Windowsie 101232 asercje), po pierwszej części
      M7 zgłoszone 269 i 102103, po drugiej 276 i 102139, a dziś, po trzeciej, 294
      i 102412
- [ ] **te same wysokości co na Windowsie.** Teren jest liczony na liczbach `float`
      z pliku `heightmap.png`, bez funkcji, których wynik zależy od biblioteki. Po starcie
      panel Camera powinien pokazać `Player feet` z y równym 0.124, a panel Terrain linie
      `Grid: 97 x 97 points, 0.50 m apart`, `Triangles: 18432` i
      `Height: 0.00 m to 3.37 m`
- [ ] **ta sama trawa co na Windowsie.** Miejsca kępek losuje `std::mt19937` przez
      `game::randomBelow`, bez rozkładów z biblioteki standardowej
      ([`../decisions/deterministic-random.md`](../decisions/deterministic-random.md)),
      więc dla ziarna 1 i gęstości 2,5 panel Grass powinien pokazać dokładnie
      `Tufts: 1843 (5529 blades)`. Inna liczba oznacza, że libc++ zaokrągla albo losuje
      inaczej: zapisać ją. Kandydaci to `std::lround` i porównania liczb `float` na
      granicy 0,6 m od labiryntu

**Shader geometrii i OpenGL**

- [ ] start gry bez linii `[error]`: kompilator GLSL Apple nie widział jeszcze żadnego
      z trzech plików trawy, a **shadera geometrii ten projekt nie uruchamiał na macOS
      nigdy**. Etap geometrii należy do rdzenia OpenGL od wersji 3.2, więc kontekst 4.1
      Core go ma, ale sprawdzić trzeba sterownik Apple, nie specyfikację. Miejsca warte
      uwagi w `grass.geom`: `layout(points) in;`, wyjście
      `layout(triangle_strip, max_vertices = TUFT_MAX_VERTICES) out;` z liczbą 15
      podstawioną przez `#define` (preprocesor musi rozwinąć nazwę wewnątrz `layout`),
      tablica wejściowa bez rozmiaru `in float vRandom[];`, `gl_in[0].gl_Position`,
      `EmitVertex()` i `EndPrimitive()` w pętli
- [ ] w `grass.frag`: dołączony `common/lighting.glsl` w trzecim programie i uniform
      `uniform bool uLit`, ustawiany z C++ przez `setInt`
- [ ] w terminalu nie ma linii
      `[error] Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code`
      dla programu `grass` (blok świateł jest podłączany do trzech programów)
- [ ] trawa jest widoczna przy ścianach, kołysze się i jest oświetlona latarką. Jeśli jej
      nie ma, a błędu w terminalu też nie ma, sprawdzić panel Grass (`Tufts`) i panel
      Shaders (linia `grass.vert + grass.geom + grass.frag`)
- [ ] **wireframe**: zaznaczyć `Wireframe` w panelu Terrain. Kod woła
      `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` przed rysowaniem terenu i `GL_FILL` po
      nim. W profilu Core dozwolone jest tylko `GL_FRONT_AND_BACK`, a linie mają szerokość
      1. Oczekiwane: teren jako siatka linii, reszta sceny wypełniona, żadnej linii
      `GL_INVALID_ENUM` ani `GL_INVALID_VALUE`. Na ekranie Retina linia o szerokości
      jednego piksela framebuffera może być słabo widoczna: zapisać, czy siatkę da się
      pokazać na obronie
- [ ] przez cały test w terminalu nie pojawia się żadna linia z nazwą błędu OpenGL
      (`GL_...`), także przy `GL_POINTS` jako prymitywie wejściowym trawy

**Komunikaty błędów shaderów**

- [ ] zepsuć `assets/shaders/grass.geom` (błąd składni) i kliknąć `Reload shaders`. Na
      Windowsie zgłoszona linia sterownika zaczyna się od `grass.geom(84)`. Sterownik
      Apple pisze w postaci z dwukropkami, więc oczekiwane jest
      `ERROR: grass.geom:N: ...`. Zamiana numeru źródła na nazwę pliku jest sprawdzona
      tylko testem jednostkowym i na sterowniku NVIDII, a dla pliku `.geom` na macOS
      wcale: jeśli w linii zostaje `0:N`, zapisać dokładny tekst
- [ ] po błędzie trawa rysuje się dalej poprzednim programem, linia programu `grass` w
      panelu Shaders jest czerwona, pozostałe pięć kończy się `OK`. Naprawić plik,
      `Reload shaders`: sześć razy `OK`
- [ ] błąd linkowania zamiast kompilacji (na przykład zmienić w `grass.frag` nazwę wejścia
      `gBladeUv` na inną): komunikat zaczyna się od `Shader linking failed:` i wymienia
      trzy pliki połączone znakiem ` + `. Zapisać, co o niezgodnym wejściu pisze linker
      Apple

**Ekran Retina i panele**

- [ ] po usunięciu `imgui.ini`: u góry, między kolumnami, są dwa rzędy pasków tytułu
      (Camera i Gameplay, pod nimi Terrain i Grass), a pasek HUD stoi pod drugim rzędem
      i na żaden nie nachodzi. Wysokość paska tytułu jest brana z ImGui
      (`ImGui::GetFrameHeight`, funkcja `foldedRowsHeight`), a odstępy są mnożone przez
      skalę układu, więc przy skali ekranu Maca drugi rząd powinien wypaść tuż pod
      pierwszym. Zapisać, czy tak jest, bo na Windowsie sprawdzono to tylko przy skali 100%
- [ ] źdźbła mają 4 cm szerokości u nasady: zapisać, czy na Retinie są wyraźne z kilku
      metrów i czy ich krawędzie nie migoczą w ruchu (wygładzania krawędzi w projekcie nie
      ma)

**Panel i przełączniki**

- [ ] cała lista ręczna z [`build-windows.md`](build-windows.md), sekcja 16.2: chodzenie
      po podłożu, szczeliny pod ścianami, `Height scale` z 0 i 2.50, `Wireframe`, wzgórza
      w trybie noclip, panel Grass (`Enabled`, `Density`, `Blade height`,
      `Wind strength`), trawa pod latarką i przy krysztale, cztery tryby `Lighting`,
      widoki diagnostyczne, `Reload shaders` (dziś przy dziesięciu programach, na macOS bez kroku
      kopiowania assetów), nowe labirynty innych rozmiarów, kryształy i brama na podłożu,
      liczba klatek
- [ ] liczba klatek na sekundę w Release z trawą i bez niej oraz przy `Density` 8.0: na
      macOS OpenGL działa jako warstwa nad Metalem i koszt shadera geometrii może być
      inny niż na karcie NVIDII, gdzie różnicy nie dało się zmierzyć. Zapisać liczby

**Skrypt Blendera**

- [ ] uruchomić na Macu `blender --background --factory-startup --python
      tools/blender/make_heightmap.py`, potem `git status`. Skrypt używa tylko generatora
      numpy, mnożenia, dodawania i zaokrąglenia do 256 poziomów, bez funkcji takich jak
      `sin` czy `exp`, więc plik `heightmap.png` powinien wyjść bajt w bajt taki sam.
      Zapisać, czy tak jest, a jeśli nie, to czy testy nadal przechodzą i jaką wysokość
      pokazuje `Player feet` na starcie. Po próbie przywrócić plik z repozytorium

### M7, część 1 (bufor HDR i gamma) na macOS: lista w całości otwarta

Pierwsza część kamienia milowego M7 (scena rysowana do własnego framebuffera
`gfx::Framebuffer` z teksturą koloru `GL_RGBA16F` i teksturą głębi, klasa
`game::PostProcess` z przebiegiem składającym: ekspozycja, mapowanie tonów, kodowanie sRGB,
korekcja gamma w całym potoku przez `gfx::ColorSpace` i tekstury `GL_SRGB8`, siódmy i ósmy
program shaderów `composite` i `preview`, klasa `debug::RawTextureSampler`, panel
Framebuffers w trzecim rzędzie pasków tytułu, nowe wartości startowe świateł) powstała na
Windowsie 2026-10-05 i tam jest zgłoszona jako zbudowana i przetestowana
([`build-windows.md`](build-windows.md), sekcja 17). **Na macOS nikt jej nie zbudował ani
nie uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Oczekiwania wynikają z kodu,
z testów i z tego, co zgłoszono na Windowsie. Opis kodu:
[`../modules/gfx/color-space.md`](../modules/gfx/color-space.md),
[`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md),
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md). M7 jest
rozpoczęty, nie kompletny: minimapy i cieni nie ma na żadnym systemie. Bloom doszedł
w drugiej części, a mgła i winieta w trzeciej: mają osobne listy niżej.

Lista powstała dla programu z pierwszej części. W dzisiejszym programie panel
Framebuffers ma inny układ i cztery obrazy, podpisy obrazów brzmią `HDR colour` i `Depth`
(a nie `Colour (HDR, cut off at 1)` i `Depth (as distance)`), kontrolki stoją w zakładce
`Tone and bloom`, programów jest dziesięć, a przypadków testowych 294: różnice są
wypisane na początku sekcji 17.2 w [`build-windows.md`](build-windows.md).

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/gfx/ColorSpace.*`, `src/gfx/Extensions.*`, `src/gfx/Framebuffer.*`,
      `src/game/PostProcess.*`, `src/debug/RawTextureSampler.*`,
      `src/debug/panels/FramebuffersPanel.*`, `tests/ColorSpaceTests.cpp`,
      `tests/FramebufferTests.cpp`. Zmienione między innymi: `src/gfx/Texture2D.*`,
      `src/gfx/Cubemap.*`, `src/assets/AssetCache.*`, `src/game/NightMazeApp.*`,
      `Lighting.*`, `ColliderLines.cpp`, `Skybox.*`, `TerrainRenderer.cpp`,
      `src/debug/DebugUI.*`, `DebugContext.hpp`, `PanelLayout.hpp`, `src/main.cpp`.
      Miejsca warte uwagi: inicjalizatory desygnowane przekazane wprost do konstruktora
      (`gfx::Framebuffer({.width = ..., .height = ..., .color = ..., .depth = ...})`
      w `PostProcess.cpp`) i do zwracanej struktury (`colorTextureFormat` w
      `Framebuffer.cpp`), przypisanie wewnątrz makra
      (`GL_CHECK(status = glCheckFramebufferStatus(GL_FRAMEBUFFER))`), `std::lround`
      w `PostProcess.cpp`, rzutowanie `static_cast<ImTextureID>` z `GLuint`
      w `FramebuffersPanel.cpp` oraz stałe rozszerzenia wpisane liczbami w
      `RawTextureSampler.cpp` (`0x8A48`, `0x8A4A`), których nagłówek GLAD nie deklaruje
- [ ] `ctest --test-dir build/debug --output-on-failure`: 269 przypadków, wszystkie
      przechodzą (zgłoszone dla Windowsa 102103 asercje). Testy `ColorSpaceTests.cpp`
      porównują wyniki `std::pow` z liczbami do czterech albo pięciu miejsc: zapisać, czy
      któryś nie przechodzi na libc++ i Apple Silicon przez ostatnią cyfrę
- [ ] gra startuje bez linii `[error]`, w szczególności bez linii zaczynającej się od
      `Framebuffer of`

**Cel `GL_RGBA16F` na sterowniku Apple**

- [ ] bufor sceny jest kompletny. Specyfikacja OpenGL 4.1 wymaga, żeby `GL_RGBA16F` dało
      się użyć jako celu rysowania, ale na macOS OpenGL działa jako warstwa nad Metalem
      i tego zestawu (tekstura `GL_RGBA16F` z teksturą głębi `GL_DEPTH_COMPONENT24`) nikt
      tam jeszcze nie utworzył. Błąd wyglądałby tak:
      `[error] Framebuffer of ... (GL_RGBA16F, GL_DEPTH_COMPONENT24) is not complete: ...`
      i czarna scena przy działających panelach. Zapisać cały komunikat
- [ ] podglądy w panelu Framebuffers (dwa małe framebuffery `GL_RGBA8` bez głębi) też
      powstają bez błędu
- [ ] odczyt tekstury głębi przez `sampler2D` działa: podgląd `Depth` pokazuje
      scenę w odcieniach szarości, a nie jednolitą biel albo czerń
- [ ] build Debug: żadnej linii `GL_` w konsoli przy starcie, przy otwartym panelu
      Framebuffers, po zmianie rozmiaru okna i po minimalizacji

**Ekran Retina: rozmiar framebuffera**

- [ ] linia `Scene framebuffer` w panelu Framebuffers pokazuje **rozmiar w pikselach**,
      czyli dla okna 1280 x 720 oczekiwane `2560 x 1440 px`, ten sam co linia `Framebuffer`
      w panelu Renderer, a nie rozmiar z linii `Window`. `onRender` bierze rozmiar
      z `window().framebufferSize()` i ten sam rozmiar dostają `beginScene` i `composite`
- [ ] obraz wypełnia całe okno i jest ostry. Gdyby do bufora albo do viewportu trafił
      rozmiar okna zamiast rozmiaru framebuffera, scena zajmowałaby lewą dolną ćwiartkę
      okna albo byłaby rozmyta
- [ ] przeciągnąć okno między ekranem Retina a zewnętrznym monitorem o skali 1 (jeśli
      jest): rozmiar w linii `Scene framebuffer` zmienia się, obraz zostaje pełny, bez
      linii `[error]`
- [ ] pamięć: przy 2560 x 1440 tekstura koloru bufora sceny ma około 28 MiB (8 bajtów na
      piksel). Zapisać liczbę klatek na sekundę w Release przy ustawieniach startowych:
      PRD wymaga stabilnych 60 klatek w 1440p na MacBooku, a na Windowsie koszt tej części
      zgłoszono jako spadek z około 2020 do około 1960 klatek w tym rozmiarze
- [ ] zmiana rozmiaru okna przeciąganiem krawędzi, minimalizacja do Docka i przywrócenie,
      tryb pełnoekranowy (zielony przycisk): po każdej z tych rzeczy obraz jest pełny
      i nie ma linii `[error]`. Zapisać, czy zminimalizowane okno zgłasza na macOS rozmiar
      framebuffera 0 x 0 (na Windowsie tak, i klatka jest wtedy pomijana)

**Rozszerzenie `GL_EXT_texture_sRGB_decode`**

- [ ] czy sterownik Apple je ma. Program tego nie wypisuje, więc sprawdza się skutek:
      w panelu Assets podglądy tekstur z napisem `sRGB` (`wall_stone.png`, `gate_wood.png`,
      `crystal.png`, `ground.png`) powinny wyglądać jak pliki otwarte w Podglądzie. Jeśli są
      wyraźnie ciemniejsze, rozszerzenia nie ma: `debug::RawTextureSampler` wtedy nic nie
      robi. Zapisać wynik. To dotyczy tylko podglądów w panelu, scena jest od tego
      niezależna
- [ ] podglądy map normalnych (napis `linear`) są jasnoniebieskie w obu przypadkach
- [ ] rozszerzenie filtrowania anizotropowego jest wykrywane tak jak przedtem (suwak
      `Anisotropy` w panelu Assets): od M7 pyta o nie wspólna funkcja `gfx::hasExtension`

**Wygląd**

- [ ] scena startowa obok zrzutu ekranu z Windowsa: ta sama jasność i ten sam charakter
      nocy. Wartości świateł, ekspozycja 1,0 i krzywa ACES były dobierane na jednym
      monitorze na Windowsie. Zapisać, czy na ekranie MacBooka noc jest czytelna, czy
      najciemniejsze kąty nie zlewają się w czerń i czy kryształy nie są przepalone
- [ ] **podwójne kodowanie.** Program koduje klatkę na sRGB w shaderze i trzyma
      `GL_FRAMEBUFFER_SRGB` wyłączone, więc wynik nie powinien zależeć od tego, jaki
      framebuffer macOS dał oknu. Gdyby scena była wyraźnie wyblakła i za jasna, a panele
      ImGui wyglądały normalnie, zapisać to: oznaczałoby drugie kodowanie po stronie
      systemu ([`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md))
- [ ] kolory paneli i paska HUD są takie same jak przed M7 (porównać ze zrzutem z M1 albo
      z Windowsem)
- [ ] tryb `Unlit` z `Tone mapping` na `None (clamp)` i `Exposure` 1.00: ściana wygląda jak
      plik `wall_stone.png`

**Panel i przełączniki**

- [ ] po usunięciu `imgui.ini`: trzy rzędy pasków tytułu u góry (Camera i Gameplay,
      Terrain i Grass, Framebuffers na szerokość obu), pasek HUD pod trzecim rzędem, nic
      na siebie nie nachodzi przy skali ekranu Maca
- [ ] rozwinięty panel Framebuffers mieści kontrolki, linie z rozmiarami i podglądy (dziś
      cztery) bez przewijania. Podglądy są we właściwą stronę (niebo u góry) i ostre na Retinie
      (mają 180 pikseli wysokości i są rozciągane przez ImGui, więc mogą być miękkie:
      zapisać)
- [ ] cała lista ręczna z [`build-windows.md`](build-windows.md), sekcja 17.2: wygląd
      sceny, `Exposure`, trzy pozycje `Tone mapping`, podglądy i `Depth range`, oba widoki
      debugowania, panel Assets, zmiana rozmiaru, minimalizacja, `Reload shaders` (dziś przy
      dziesięciu programach, na macOS bez kroku kopiowania assetów)

### M7, część 2 (bloom) na macOS: lista w całości otwarta

Druga część kamienia milowego M7 (bloom: przebieg jasności i rozdzielne rozmycie Gaussa
w trzech celach `GL_RGBA16F` o połowie rozmiaru bufora sceny, dodanie poświaty
w przebiegu składającym, pliki `src/game/Bloom.*`, shadery `post/bright.frag`
i `post/blur.frag`, setter `Shader::setFloatArray`, kontrolki i dwa kolejne podglądy
w panelu Framebuffers, `CRYSTAL_GLOW_STRENGTH` równe 4,0) powstała na Windowsie 2026-10-05
i tam jest zgłoszona jako zbudowana i przetestowana ([`build-windows.md`](build-windows.md),
sekcja 18). **Na macOS nikt jej nie zbudował ani nie uruchomił, więc żaden punkt poniżej
nie jest odhaczony.** Oczekiwania wynikają z kodu, z testów i z tego, co zgłoszono na
Windowsie. Opis kodu:
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.11
do 2.15, 4.6 do 4.8, 5.10 i 5.11.

Lista powstała dla programu z drugiej części. W dzisiejszym programie siedem kontrolek
bloomu, ekspozycji i podglądu głębi stoi w zakładce `Tone and bloom` panelu Framebuffers
(obok zakładki `Fog and vignette`), plików testów jest dwadzieścia cztery, a przypadków
testowych 294 (zgłoszone dla Windowsa 102412 asercji). Kroki o wyglądzie poświaty
najlepiej wykonać z odznaczonymi polami `Fog` i `Vignette`. Mgła i winieta mają własną
listę zaraz po tej.

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/game/Bloom.hpp`, `src/game/Bloom.cpp`, `tests/BloomTests.cpp`. Zmienione:
      `src/game/PostProcess.*`, `src/game/NightMazeApp.*`, `src/game/ShaderUniforms.hpp`,
      `src/game/Crystals.hpp`, `src/gfx/Shader.*`, `src/debug/panels/FramebuffersPanel.*`,
      `src/debug/DebugContext.hpp`, `src/debug/DebugUI.cpp`, `src/debug/PanelLayout.hpp`,
      `src/main.cpp`. Miejsca warte uwagi: nagłówek `<span>` i parametr
      `std::span<const float>` w `Shader.hpp` (pierwszy setter uniformu, który bierze
      `std::span`), przekazanie `std::array<float, 7>` tam, gdzie funkcja chce `std::span`,
      rzutowanie `static_cast<GLsizei>(values.size())`, `std::clamp` na trzech wartościach
      `int` w `PostProcess.cpp` i wskaźnik `const gfx::Framebuffer*` przestawiany w pętli
- [ ] `ctest --test-dir build/debug --output-on-failure`: 276 przypadków, wszystkie
      przechodzą (zgłoszone dla Windowsa 102139 asercji). `BloomTests.cpp` porównuje
      wyniki `std::exp` na liczbach `float` z wartościami do czterech miejsc
      (`0.1370` z tolerancją 0,001 i `0.0185` z tolerancją 0,01) i sumę wag z jedynką
      z tolerancją 0,00001: zapisać, czy któryś przypadek nie przechodzi na libc++
      i Apple Silicon przez ostatnią cyfrę
- [ ] gra startuje bez linii `[error]`. Trzy cele bloomu powstają w pierwszej klatce: błąd
      wyglądałby tak: `[error] Framebuffer of ... is not complete: ...`, i wracałby
      w **każdej** klatce, bo cele bloomu nie pamiętają nieudanej próby
      ([`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md), sekcja 5.14)

**Shadery bloomu na sterowniku Apple**

- [ ] panel Shaders: dziesięć linii, wszystkie `OK`. Dwie nowe to
      `composite.vert + bright.frag: OK` i `composite.vert + blur.frag: OK`
- [ ] `blur.frag` kompiluje się: tablica uniformów o rozmiarze z wyrażenia stałego
      (`uniform float uWeights[BLUR_RADIUS + 1]`), pętla `for` o stałej liczbie obrotów
      z indeksowaniem tej tablicy i funkcja `textureSize`. Wszystko to jest w GLSL 4.10,
      ale kompilator shaderów Apple tych plików jeszcze nie widział. Zapisać cały
      komunikat, jeśli linia jest czerwona
- [ ] `glUniform1fv` z nazwą tablicy bez indeksu (`"uWeights"`) ustawia wszystkie siedem
      elementów: poświata jest widoczna i ma normalną jasność. Poświata czarna albo
      ledwo widoczna przy poprawnym obrazie `Bright pass` znaczyłaby, że wagi nie dotarły
- [ ] build Debug: żadnej linii `GL_` w konsoli przy starcie, przy włączonym bloomie, przy
      otwartym panelu Framebuffers, po zmianie rozmiaru okna i po minimalizacji

**Cele `GL_RGBA16F` bez głębi i połowa rozdzielczości na ekranie Retina**

- [ ] trzy cele bloomu są kompletne. To pierwsze framebuffery gry z kolorem `GL_RGBA16F`
      i **bez** załącznika głębi: tej kombinacji nikt na macOS jeszcze nie utworzył
- [ ] linia `Bloom targets (3)` w panelu Framebuffers pokazuje połowę liczb z linii
      `Scene framebuffer`. Na ekranie Retina dla okna 1280 x 720 oczekiwane
      `Scene framebuffer: 2560 x 1440 px` i `Bloom targets (3): 1280 x 720 px`: cele
      bloomu mają wtedy rozmiar **okna w punktach**, a nie jego połowę
- [ ] poświata leży dokładnie na kryształach i na księżycu, nie jest przesunięta ani
      przeskalowana względem sceny. Przesunięcie oznaczałoby pomylenie rozmiaru okna
      z rozmiarem framebuffera w którymś viewporcie
- [ ] **szerokość poświaty.** Jądro rozmycia ma stały promień w pikselach celu, więc na
      Retinie poświata jest względem ekranu o połowę cieńsza niż w tym samym oknie na
      monitorze o skali 1 (na Windowsie zgłoszone to samo dla 2560 x 1440, sprawdzone
      tylko na wycinku obrazu). Zapisać, jak wygląda poświata kryształu z kilku metrów
      na ekranie MacBooka i przy jakiej wartości `Blur iterations` wygląda jak na zrzucie
      z Windowsa w oknie 1280 x 720
- [ ] przeciągnąć okno między ekranem Retina a zewnętrznym monitorem o skali 1 (jeśli
      jest): liczby w obu liniach zmieniają się razem, poświata zostaje na miejscu, bez
      linii `[error]`
- [ ] pamięć: przy buforze sceny 2560 x 1440 trzy cele bloomu mają razem około 21 MiB
      (policzone z rozmiaru, 8 bajtów na piksel), obok około 28 MiB samej tekstury koloru
      sceny

**Liczba klatek z bloomem**

- [ ] Release, ustawienia startowe, panele ukryte, okno 1280 x 720 na ekranie Retina
      (bufor sceny 2560 x 1440): zapisać liczbę klatek na sekundę z zaznaczonym
      i z odznaczonym polem `Bloom`. PRD wymaga stabilnych 60 klatek w 1440p na MacBooku.
      Na Windowsie zgłoszono w tym rozmiarze od 1370 do 1480 klatek bez bloomu i od 880
      do 925 z bloomem, czyli około 0,4 ms na klatkę, ale na innej karcie: dla MacBooka
      nic z tego nie wynika
- [ ] to samo przy `Blur iterations` równym 10 (dwadzieścia przebiegów rozmycia):
      zapisać, czy liczba klatek zostaje powyżej 60
- [ ] jeśli z bloomem liczba klatek spada poniżej 60: zapisać, przy ilu iteracjach wraca,
      i czy pomaga zwinięcie panelu Framebuffers (cztery podglądy mniej)
- [ ] na macOS synchronizacja pionowa może trzymać licznik na częstotliwości odświeżania
      ekranu: zapisać, czy tak jest, bo wtedy liczba z panelu Renderer nie mówi nic
      o zapasie

**Wygląd i panel**

- [ ] poświata kryształu obok zrzutu ekranu z Windowsa: podobna jasność i barwa. Próg
      0,8, intensywność 1,0 i siła świecenia 4,0 były dobierane na jednym monitorze na
      Windowsie
- [ ] kryształ z odznaczonym polem `Bloom`: zapisać, czy przy sile 4,0 nie jest na
      ekranie MacBooka za blady
- [ ] rozwinięty panel Framebuffers mieści siedem kontrolek w dwóch kolumnach, dwie linie
      informacyjne i cztery obrazy bez przewijania przy skali ekranu Maca
- [ ] cała lista ręczna z [`build-windows.md`](build-windows.md), sekcja 18.2: włącznik,
      próg, intensywność, iteracje, podglądy, kryształ z bliska i z daleka, księżyc
      i gwiazdy, latarka, oba widoki debugowania, zmiana rozmiaru, `Reload shaders` przy
      dziesięciu programach (na macOS bez kroku kopiowania assetów)

### M7, część 3 (mgła i winieta) na macOS: lista w całości otwarta

Trzecia część kamienia milowego M7 (mgła wykładnicza ze współczynnikiem wysokości, liczona
w przebiegu składającym z pozycji w świecie odtworzonej z tekstury głębi sceny, i winieta
po mapowaniu tonów: pliki `src/game/Fog.*` i `src/game/Vignette.*`, jedenaście nowych
uniformów i cztery funkcje w `post/composite.frag`, struktura `SceneView` przekazywana do
`PostProcess::composite`, tekstura głębi na jednostce teksturującej 2, dwie zakładki
w panelu Framebuffers) powstała na Windowsie 2026-10-05 i tam jest zgłoszona jako
zbudowana i przetestowana ([`build-windows.md`](build-windows.md), sekcja 19). Nie doszedł
żaden program shaderów, żaden framebuffer ani żaden panel. **Na macOS nikt jej nie
zbudował ani nie uruchomił, więc żaden punkt poniżej nie jest odhaczony.** Oczekiwania
wynikają z kodu, z testów i z tego, co zgłoszono na Windowsie. Opis kodu:
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.17
do 2.22, 4.2, 4.9, 5.6, 5.7, 5.12 i 5.13. M7 jest nadal rozpoczęty, nie kompletny:
minimapy i cieni nie ma na żadnym systemie.

**Build i testy**

- [ ] `cmake --build --preset debug` i `cmake --build --preset release` bez ostrzeżeń pod
      `-Wall -Wextra -Wpedantic`. Nowe pliki, których Apple clang z libc++ nie widział:
      `src/game/Fog.hpp`, `src/game/Fog.cpp`, `src/game/Vignette.hpp`,
      `src/game/Vignette.cpp`, `tests/FogTests.cpp`, `tests/VignetteTests.cpp`. Zmienione:
      `src/game/PostProcess.*`, `src/game/NightMazeApp.*`, `src/game/ShaderUniforms.hpp`,
      `src/debug/panels/FramebuffersPanel.*`, `src/debug/DebugContext.hpp` (sam komentarz)
      i `CMakeLists.txt`. Miejsca warte uwagi: stała `constexpr glm::vec2 SCREEN_CENTER`
      w nagłówku `Vignette.hpp`, inicjalizacja z nazwami pól
      (`const SceneView sceneView{.inverseViewProjection = ..., .eye = eye}`)
      w `NightMazeApp.cpp`, `glm::inverse` na macierzy `mat4` i `std::exp` na liczbach
      `float` w `Fog.cpp`
- [ ] `ctest --test-dir build/debug --output-on-failure`: 294 przypadki, wszystkie
      przechodzą (zgłoszone dla Windowsa 102412 asercji, z czego 241 w `FogTests.cpp`
      i 32 w `VignetteTests.cpp`). `FogTests.cpp` porównuje wyniki `std::exp` z wartościami
      do czterech miejsc (`0.4966` i `0.5507` z tolerancją 0,001) i pozycję odtworzoną
      z głębi z pozycją wyjściową z tolerancją 0,01, a `VignetteTests.cpp` porównuje
      współczynniki z domyślną tolerancją `doctest::Approx`: zapisać, czy któryś
      przypadek nie przechodzi na libc++ i Apple Silicon przez ostatnią cyfrę
- [ ] gra startuje bez linii `[error]`, w szczególności bez linii o shaderze
      `composite.frag`

**Shader mgły i winiety na sterowniku Apple (GLSL 4.10 na OpenGL 4.1)**

- [ ] panel Shaders: nadal dziesięć linii, wszystkie `OK`. Mgła i winieta są w linii
      `composite.vert + composite.frag: OK`
- [ ] `composite.frag` kompiluje się: funkcje `smoothstep`, `exp`, `mix`, `length`
      i `max`, uniform typu `mat4` w shaderze fragmentów, mnożenie `mat4` przez `vec4`,
      dzielenie przez `world.w` i **trzeci** sampler `sampler2D` w jednym programie.
      Wszystko to jest w rdzeniu GLSL 4.10, ale kompilator shaderów Apple tej wersji pliku
      jeszcze nie widział. Zapisać cały komunikat, jeśli linia jest czerwona
- [ ] trzy samplery czytają trzy różne jednostki: `uScene` 0, `uBloom` 1, `uDepth` 2.
      Mgła, która zależy od koloru ścian zamiast od odległości, znaczyłaby, że `uDepth`
      czyta jednostkę 0, czyli obraz sceny
- [ ] build Debug: żadnej linii `GL_` w konsoli przy starcie, przy włączonej mgle, przy
      otwartym panelu Framebuffers, po zmianie rozmiaru okna i po minimalizacji. Po klatce
      z mgłą tekstura głębi sceny zostaje związana z jednostką 2, a następna klatka rysuje
      do framebuffera, który tę teksturę ma jako załącznik. Żaden shader sceny nie czyta
      jednostki 2, więc nie jest to pętla sprzężenia, ale sterownik Apple tego układu
      jeszcze nie widział: zapisać każdą linię `GL_` i każde migotanie obrazu

**Odczyt głębi `GL_DEPTH_COMPONENT24` i dokładność na kartach Apple**

- [ ] mgła w ogóle jest: koniec długiego korytarza wtapia się w kolor mgły. Brak mgły
      przy zaznaczonym polu `Fog` albo mgła jednolita na całym ekranie znaczyłyby, że
      odczyt tekstury głębi przez zwykły `sampler2D` zwraca na tej karcie coś innego niż
      głębię w kanale czerwonym
- [ ] obraz `Depth` w panelu Framebuffers jest poprawny (czyta tę samą teksturę od
      pierwszej części M7, na macOS też jeszcze niesprawdzony): jeśli on działa, a mgła
      nie, błąd jest w odtwarzaniu pozycji, nie w odczycie
- [ ] mgła na dalekich ścianach i na dalekim podłożu jest gładka: bez pasów, schodków
      i migotania przy ruchu. Krok 24-bitowej głębi przy płaszczyznach 0,1 m i 100 m to
      około pół milimetra w odległości 30 m i około 6 mm przy 100 m (policzone), więc
      pasów być nie powinno. Pasy znaczyłyby, że karta dała bufor głębi o mniejszej
      dokładności niż ta, o którą prosi kod
- [ ] krawędź ściany na tle nieba: bez jasnej ani ciemnej obwódki. Tekstura głębi ma filtr
      `GL_NEAREST` i ten sam rozmiar co obraz sceny, więc obwódki być nie powinno
- [ ] obrót kamery w miejscu przed ścianą, z odznaczonym polem `Vignette` i zgaszoną
      latarką: mgła na ścianie się nie zmienia i nie pulsuje. Pływanie mgły znaczyłoby
      niezgodność macierzy `uInverseViewProjection` z macierzami, którymi narysowano scenę
- [ ] widok z góry w trybie noclip (klawisz N, w górę Spacja, w dół lewy Shift): z około
      30 m labirynt jest prawie zakryty mgłą, tak samo jak na Windowsie. To znane
      ograniczenie ([`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md)),
      nie błąd Maca

**Ekran Retina**

- [ ] na ekranie Retina bufor sceny ma dwa razy więcej pikseli w każdą stronę niż okno
      w punktach (dla okna 1280 x 720 oczekiwane `Scene framebuffer: 2560 x 1440 px`).
      Mgła czyta teksturę głębi tą samą współrzędną `vUv` co obraz sceny i nie dostaje
      żadnego rozmiaru, więc powinna leżeć dokładnie na ścianach, nieprzesunięta
      i nieprzeskalowana
- [ ] winieta jest liczona we współrzędnych tekstury (od 0 do 1 w obu kierunkach), więc
      nie zależy od liczby pikseli: rogi są tak samo ciemne i w tym samym miejscu co na
      monitorze o skali 1. Jasny środek jest elipsą w kształcie okna
- [ ] przeciągnąć okno między ekranem Retina a zewnętrznym monitorem o skali 1 (jeśli
      jest): mgła i winieta wyglądają tak samo na obu, bez linii `[error]`

**Liczba klatek z mgłą i winietą**

- [ ] Release, ustawienia startowe, panele ukryte, okno 1280 x 720 na ekranie Retina
      (bufor sceny 2560 x 1440): zapisać liczbę klatek na sekundę z zaznaczonymi polami
      `Fog` i `Vignette` i z oboma odznaczonymi. PRD wymaga stabilnych 60 klatek w 1440p
      na MacBooku. Na Windowsie zgłoszono w tym rozmiarze około 1145 klatek z oboma
      efektami i około 1158 bez nich (jeden przebieg, różnica w granicach szumu), ale na
      innej karcie: dla MacBooka nic z tego nie wynika
- [ ] mgła kosztuje na każdy piksel ekranu jeden odczyt tekstury więcej, jedno mnożenie
      przez macierz i dwa wywołania `exp`: zapisać, czy na karcie Apple różnica jest
      większa niż zgłoszony na Windowsie 1 %
- [ ] na macOS synchronizacja pionowa może trzymać licznik na częstotliwości odświeżania
      ekranu: zapisać, czy tak jest, bo wtedy liczba z panelu Renderer nie mówi nic
      o zapasie

**Wygląd i panel**

- [ ] mgła obok zrzutu ekranu z Windowsa: podobna gęstość i barwa. Kolor
      `(0.14, 0.18, 0.26)`, gęstość 0,1 i siła winiety 0,3 to wartości startowe
      z Windowsa: zapisać, czy na ekranie MacBooka mgła nie jest za jasna albo za
      niebieska, a rogi za ciemne
- [ ] niebo: księżyc i gwiazdy są wyraźne, pas tuż nad horyzontem jest zamglony. Przy
      powolnym obrocie kamery zapisać, czy zamglenie samego nieba zmienia się przy bokach
      ekranu (na Windowsie też jeszcze nieoglądane)
- [ ] kryształ na końcu korytarza: bryła blednie w mgle, poświata zostaje
- [ ] rozwinięty panel Framebuffers mieści pasek dwóch zakładek, cztery wiersze kontrolek
      w każdej z nich, dwie linie informacyjne i cztery obrazy bez przewijania przy skali
      ekranu Maca. Przełączanie zakładek nie przesuwa linii ani obrazów
- [ ] pole `Fog colour`: trzy pola z liczbami `36`, `46`, `66` i kwadrat z kolorem, okno
      wyboru koloru otwiera się i mieści na ekranie
- [ ] cała lista ręczna z [`build-windows.md`](build-windows.md), sekcja 19.2: panel
      w nowym układzie, włącznik mgły, gęstość, wysokość podstawy, zanik z wysokością,
      długi korytarz, obrót w miejscu, widok z góry, niebo i księżyc, kryształ przez
      mgłę, kolor mgły, ekspozycja i mapowanie tonów, włącznik winiety, winieta
      przesadzona, oba widoki diagnostyczne, podglądy, zmiana rozmiaru, `Reload shaders`
      przy dziesięciu programach i próby na pliku `composite.frag` (na macOS bez kroku
      kopiowania assetów)

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
| `[error] Shader file cannot be opened: .../build/debug/assets/shaders/textured.vert` (i takie same linie dla pozostałych programów), w oknie samo tło | obok programu nie ma katalogu `assets`: program skopiowany ręcznie w inne miejsce albo repozytorium przeniesione po zbudowaniu (dowiązanie wskazuje starą ścieżkę) | `ls -l build/debug/assets`. Odtwórz dowiązanie pełnym buildem: `make clean`, potem `make debug` |
| `[error] Shader compilation failed: ...` z linią sterownika, znika to, co rysuje ten program (w stanie M1, z jednym programem, zostawało samo tło) | błąd w pliku shadera. Sterownik Apple pisze linię w postaci `ERROR: 0:N: ...`, gdzie 0 to numer napisu źródłowego, a `N` numer linii. Od M4 program zamienia ten numer na nazwę pliku, więc oczekiwana postać to `ERROR: color.frag:N: ...`, a dla błędu w pliku dołączanym `ERROR: common/lighting.glsl:N: ...`. Zamiana jest sprawdzona tylko testem jednostkowym, nie na prawdziwym sterowniku Apple: jeśli linia ma inną postać, zostaje taka, jak ją napisał sterownik | popraw plik w `assets/shaders/` i naciśnij "Reload shaders" w panelu Shaders (albo uruchom program ponownie). Opis w [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), sekcje 3.3 i 7, w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 7, oraz w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `[error] Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code` | sterownik ułożył blok uniformów ze światłami inaczej niż struktura `scene::LightBlockData` (na macOS niesprawdzone, na karcie NVIDIA linia się nie pojawia) | zapisać liczbę z komunikatu i porównać blok w `assets/shaders/common/lighting.glsl` ze strukturą w `src/scene/LightBlock.hpp` ([`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md)) |
| Podłoże albo ściany są białe, w terminalu linia `[error]` o pliku obrazu | brakuje pliku w `assets/textures/` albo nie da się go zdekodować: część modelu (albo teren, gdy chodzi o `ground.png`) dostaje białą teksturę zastępczą (na macOS niesprawdzone) | przywróć plik (`git status`, `git checkout assets/textures`) i uruchom program ponownie |
| Okno otwiera się, ale panel "Renderer" jest niewidoczny, widać tylko pasek HUD u góry | panele ukryte klawiszem `~` albo zapisany układ poza oknem | naciśnij `~` (na lewo od `1`). Jeśli nie pomaga, usuń `imgui.ini` z katalogu, z którego uruchamiasz program |
| Latarka nie daje się włączyć klawiszem F | bateria jest pusta: pasek HUD pokazuje `0%` i napis `Battery empty. Find a crystal.`. To reguła gry, nie błąd | zbierz kryształ, potem naciśnij F, albo zacznij rundę od nowa klawiszem R |
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
