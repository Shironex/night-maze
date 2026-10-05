# Budowanie na macOS (Apple Silicon)

Przewodnik dla kamienia milowego M0. Wszystkie polecenia z tego dokumentu zostały uruchomione
na Macu w konfiguracji:

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

W oknie widać ciemnogranatowe tło, na środku kostkę obróconą tak, że widać trzy jej ściany
(czerwoną z przodu, niebieską z lewej i turkusową u góry, każda w jednolitym kolorze), a na
wierzchu panele "Renderer", "Shaders" i "Camera". Kostka zachowuje proporcje przy zmianie
rozmiaru okna. Po kliknięciu w scenę kursor znika i kamerą można latać wokół kostki
(tabela niżej). Linia `[error] Shader ...` w terminalu oznacza, że shader się nie
wczytał: wtedy okno pokazuje samo tło ([`../modules/gfx/shaders.md`](../modules/gfx/shaders.md),
sekcja 5.1).

`4.1` potwierdza, że dostaliśmy kontekst, o który prosiliśmy. `Metal` oznacza, że OpenGL na
Apple Silicon jest warstwą zbudowaną nad Metalem. Druga linia zależy od procesora w danym
Macu.

### Sterowanie

| Klawisz albo mysz | Działanie | Gdzie w kodzie |
|---|---|---|
| lewy przycisk myszy w scenie | przechwytuje kursor (kursor znika) i włącza sterowanie kamerą | `NightMazeApp::onRender` w [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) |
| ruch myszy przy przechwyconym kursorze | obraca kamerę | tamże |
| W, S, A, D przy przechwyconym kursorze | lot do przodu, do tyłu, w lewo, w prawo | `NightMazeApp::onUpdate`, tamże |
| spacja, lewy Shift przy przechwyconym kursorze | lot w górę, w dół | tamże |
| Esc | przy przechwyconym kursorze oddaje kursor, przy wolnym zamyka program | `Application::run` w [`src/core/Application.cpp`](../../src/core/Application.cpp) |
| `~` (na lewo od `1`, `GLFW_KEY_GRAVE_ACCENT`) | pokazuje lub ukrywa interfejs debugowy | `DebugNightMazeApp::onRender` w [`src/main.cpp`](../../src/main.cpp) |

Klawisze są ignorowane, dopóki aktywny jest widżet panelu ImGui (na przykład trwa
wpisywanie wartości): klawiatura należy wtedy do panelu. Opis w
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6. Kliknięcie w panel nie
przechwytuje kursora, a przy przechwyconym kursorze panele nie reagują na mysz: żeby
przesunąć suwak, trzeba najpierw nacisnąć Esc. Sterowanie kamerą opisuje
[`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcje 5
i 6. Na macOS GLFW 3.4 nie ma surowego ruchu myszy, więc obrót korzysta z ruchu kursora po
przyspieszeniu systemowym ([`../modules/core/input.md`](../modules/core/input.md), sekcja
2.7).

Panel "Renderer" pokazuje FPS, czas klatki, rozmiar framebuffera i okna, wersję OpenGL,
nazwę karty oraz edytor koloru tła. Panel można przeciągnąć do krawędzi okna (docking).

### Katalog `assets` i praca z shaderami

Program wczytuje shadery z katalogu `assets` leżącego **obok pliku wykonywalnego**, czyli z
`build/debug/assets` ([`../modules/core/paths.md`](../modules/core/paths.md)). Na macOS ten
katalog jest dowiązaniem symbolicznym, które build tworzy po zlinkowaniu programu:

```sh
ls -l build/debug/assets
# build/debug/assets -> /Users/<nazwa>/.../night-maze/assets
```

Skutki praktyczne:

- Plik shadera edytuję w `assets/shaders/` w repozytorium. Program widzi zmianę przy
  następnym wczytaniu, **bez budowania**. Dziś shader jest wczytywany tylko przy starcie, więc
  wystarczy ponownie uruchomić `./build/debug/night_maze`.
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

> **Na macOS jeszcze nie uruchomione.** Kod kolizji i labiryntu oraz jego testy powstały na
> Windowsie (2026-10-05) i tam są zmierzone: [`build-windows.md`](build-windows.md),
> sekcja 2. Wszystko w tym podrozdziale jest dla Maca oczekiwaniem, nie pomiarem.

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

Oczekiwany koniec wyjścia, taki jak zmierzony na Windowsie w konfiguracji Debug
(2026-10-05, sześć plików z testami: `ColliderTests.cpp`, `MazeTests.cpp`,
`MazeGeneratorTests.cpp`, `MazeLayoutTests.cpp`, `ObjLoaderTests.cpp` i
`ImageLoaderTests.cpp`):

```text
[doctest] test cases:    66 |    66 passed | 0 failed | 0 skipped
[doctest] assertions: 58953 | 58953 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Nad tym raportem program wypisuje kilka linii `[error]`: pochodzą z testów, które celowo
podają loaderom zły plik, i nie oznaczają nieudanego testu. Same testy kolizji i labiryntu
(cztery pierwsze pliki) to 41 przypadków i 58114 asercji.

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
      zapisać liczbę przypadków i asercji (oczekiwane dla całego programu: 66 i 58953,
      z czego testy kolizji i labiryntu to 41 i 58114)
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
- [ ] program `./build/debug/night_maze` buduje się i działa jak wcześniej (nowy kod nie
      jest jeszcze wołany przez grę)

**Loader OBJ i siatka (temat 4): do zrobienia przy pierwszym buildzie tego kodu na Macu.**
Kod powstał na Windowsie (2026-10-05) i tam jest zmierzony: 18 przypadków testowych i 804
asercje w `tests/ObjLoaderTests.cpp`. Na Macu nikt go jeszcze nie kompilował:

- [ ] `src/assets/ObjLoader.*`, `src/gfx/Vertex.hpp`, `src/gfx/Mesh.*` i
      `tests/ObjLoaderTests.cpp` kompilują się bez ostrzeżeń pod `-Wall -Wextra -Wpedantic`
- [ ] przechodzą trzy asercje czasu kompilacji: `sizeof(gfx::Vertex) == 8 * sizeof(float)` i
      `std::is_standard_layout_v<gfx::Vertex>` w `Vertex.hpp` oraz
      `std::is_same_v<GLuint, std::uint32_t>` w `Mesh.cpp`
- [ ] `./build/debug/night_maze_tests --source-file='*ObjLoaderTests*'`: zapisać liczbę
      przypadków i asercji (oczekiwane 18 i 804)
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
na Windowsie (2026-10-05) i tam jest zmierzony: 7 przypadków testowych i 35 asercji w
`tests/ImageLoaderTests.cpp` oraz program z ukrytym oknem dla klasy `gfx::Texture2D`
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
      przypadków i asercji (oczekiwane 7 i 35)
- [ ] przechodzą przypadki `the rows are flipped: ...` i `a path with letters outside ASCII
      can be loaded`: oba zapisują plik do katalogu tymczasowego systemu
      (`std::filesystem::temp_directory_path()`) i usuwają go po sobie. Drugi tworzy plik o
      nazwie z polskimi literami i znakiem japońskim
- [ ] klasa `gfx::Texture2D` na sterowniku Apple: czy na liście rozszerzeń jest
      `GL_EXT_texture_filter_anisotropic`, jaką wartość ma `maxAnisotropy()` i czy
      konstruktor nie zostawia błędu w `glGetError`. Do sprawdzenia programem z ukrytym
      oknem albo dopiero po wpięciu tekstur w grę, w panelu Textures
- [ ] powtórzyć pomiar pasów z [`../modules/gfx/textures.md`](../modules/gfx/textures.md),
      sekcja 5.9: czy poziom anizotropii ustawiony na obiekcie samplera zmienia obraz (na
      Windowsie: szary przy 1, czarne i białe pasy przy 16) i czy ustawiony przez
      `glTexParameterf` na samej teksturze też działa (na Windowsie nie działał)
- [ ] po wpięciu tekstur w grę: tekstura na ścianie nie jest do góry nogami ani pochylona,
      a `texture()` w shaderze `#version 410 core` kompiluje się na sterowniku Apple

Po wykonaniu punkty trzeba odhaczyć i dopisać wynik, tak jak na liście w
[`build-windows.md`](build-windows.md), sekcja 11.

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
| [`.clangd`](../../.clangd) | wskazuje clangd katalog `build/debug` jako miejsce, w którym leży `compile_commands.json` |
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
oraz konwencja nazw, na przykład prefiks `m_` dla pól prywatnych). clang-tidy nie wchodzi w
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
| `[error] Shader compilation failed: ...` z linią `ERROR: 0:N: ...`, w oknie samo tło | błąd w pliku shadera, `N` to numer linii według sterownika | popraw plik w `assets/shaders/` i naciśnij "Reload shaders" w panelu Shaders (albo uruchom program ponownie). Opis w [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), sekcje 3.3 i 7, oraz w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 7 |
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
