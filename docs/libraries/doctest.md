# doctest 2.5.3

Dokument biblioteki dla kamienia milowego M2 + M3. Opisuje konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i
[`CMakeLists.txt`](../../CMakeLists.txt) oraz tę część API, której używają testy w katalogu
[`tests/`](../../tests/).

**Stan na dziś: doctest używa jeden program, `night_maze_tests`.** Składa się z siedemnastu
plików: `tests/main.cpp` (punkt wejścia) i szesnastu plików z testami. Osiem pierwszych
wierszy tabeli to pliki z M2 + M3, cztery następne doszły z oświetleniem (pierwsza część M4),
trzynasty z mapami normalnych (druga część M4), a trzy ostatnie z rozgrywką (M5). M5 zmieniło
też liczby w trzech starszych plikach: `ColliderTests.cpp` dostał siedem przypadków o kulach,
`MazeTests.cpp` dwa (w tym przeniesiony test `isDeadEnd`), a z `LightingTests.cpp` ubyło
siedem, bo światła w ślepych zaułkach zostały usunięte z gry:

| Plik | Przypadków | Co sprawdza | Dokument |
|---|---|---|---|
| `ColliderTests.cpp` | 19 | kolizje: `Aabb`, `overlaps`, `moveAndSlide`, od M5 także kule (`Sphere`, dwa testy `overlaps` z kulą, `closestPoint`) | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `MazeTests.cpp` | 8 | klasa `Maze` i kierunki, od M5 także `isDeadEnd` (przeniesione z `Lighting`) i porównywanie `MazeCell` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| `MazeGeneratorTests.cpp` | 11 | `randomBelow`, `generateMaze`, labirynt wzorcowy | tamże |
| `MazeLayoutTests.cpp` | 12 | układ w świecie i pudełka kolizji labiryntu | tamże |
| `MazeWorldTests.cpp` | 8 | `buildMazeWorld`: macierze modelu, pudełka, start, od M5 także wyjście, brama i kryształy świata | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `PlayerTests.cpp` | 13 | gracz: chodzenie, sprint, ślizganie, noclip | [`../modules/game/player.md`](../modules/game/player.md) |
| `ObjLoaderTests.cpp` | 20 | loader OBJ i MTL, od M4 także linia mapy normalnych `map_Bump` i styczne modeli gry | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md) |
| `ImageLoaderTests.cpp` | 9 | loader obrazów, od M4 także zawartość map normalnych (średnia, konwencja kanału zielonego) | [`../modules/assets/images.md`](../modules/assets/images.md) |
| `ShaderSourceTests.cpp` | 22 | tekst shadera: `expandIncludes` (dyrektywa `#include`, linie `#line`, błędy) i `nameSourceFiles` (nazwy plików w komunikatach sterownika) | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `LightTests.cpp` | 20 | matematyka świateł: zanik z odległością, stożek reflektora, `directionFromAngles`, bajty bloku świateł (`packLightBlock`) | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `LightingTests.cpp` | 10 | ustawienia oświetlenia gry: wartości domyślne, `usesNormalMap`, `buildLightSet`. Testy świateł w ślepych zaułkach z M4 zniknęły razem z tym kodem | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `TransformTests.cpp` | 4 | macierz normalnych (`scene::normalMatrix`) | [`../modules/scene/transforms.md`](../modules/scene/transforms.md) |
| `TangentTests.cpp` | 9 | styczne wierzchołków: `triangleTangents`, `computeTangents`, `countMirroredTriangles` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md) |
| `ExitTests.cpp` | 11 | wyjście: odległości liczone w przejściach (`passageDistances`), najdalsza komórka (`farthestCell`), brama (`placeExit`, `wallSegmentOn`), strefa wyjścia (`exitZone`) | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `CrystalTests.cpp` | 14 | kryształy: ile ich jest (`crystalCountFor`), gdzie stoją (`placeCrystals`), kołysanie, obrót, pulsowanie i świecenie | tamże |
| `RoundTests.cpp` | 25 | cała runda bez okna: `startRound`, `updateRound` (zbieranie, bateria, brama, wygrana), `flashlightFlicker`, `lightingForFrame`, `crystalLightPositions` | tamże |

Razem 215 przypadków testowych i 85098 asercji: tyle pokazał program na Windowsie w
konfiguracjach Debug i Release (2026-10-05). Liczbę 215 potwierdza też policzenie makr
`TEST_CASE` w plikach, w kolejności tabeli:
19 + 8 + 11 + 12 + 8 + 13 + 20 + 9 + 22 + 20 + 10 + 4 + 9 + 11 + 14 + 25.
Poprzednie stany: po M4 trzynaście plików z testami, 163 przypadki i 62220 asercji, po
pierwszej części M4 dwanaście plików, a po M2 + M3 osiem (wszystkie 2026-10-05). **Na macOS
testy nie były jeszcze budowane ani uruchamiane.** Kod, który wymaga kontekstu OpenGL
(`gfx::Mesh`, `gfx::Texture2D`, `gfx::UniformBuffer`, `gfx::Shader`, `assets::AssetCache`,
klasy rysujące, panele i HUD), testów jednostkowych nie ma.

W dokumencie są dwa rodzaje bloków C++. Blok zaczynający się komentarzem
`// Przykład, nie kod projektu.` to **przykład użycia API**. Blok poprzedzony nazwą pliku to
kod skopiowany z repozytorium. Fragmenty CMake są prawdziwe i skopiowane z repozytorium.

## 1. Czym jest doctest

doctest to biblioteka do **testów jednostkowych** (unit tests) w C++. Test jednostkowy to
mały kawałek kodu, który woła jedną funkcję programu z konkretnymi danymi i sprawdza, czy
wynik jest taki, jak powinien. Testy uruchamia się jednym poleceniem po każdej zmianie:
jeśli coś, co działało, przestało działać, dowiaduję się o tym od razu, a nie na obronie.

Co daje biblioteka:

- makra do zapisywania testów (`TEST_CASE`, `SUBCASE`) i sprawdzeń (`CHECK`, `REQUIRE`),
- gotową funkcję `main`, która znajduje wszystkie testy w programie i je uruchamia,
- czytelny raport: który plik, która linia, jakie wartości miały obie strony porównania,
- kod wyjścia programu: 0, gdy wszystko przeszło, inny w razie błędu. Na tym opiera się
  `ctest`.

**Składa się z jednego nagłówka (single header).** Cała biblioteka to plik
`doctest/doctest.h`. Nie ma niczego do skompilowania osobno. W jednym pliku `.cpp` programu
definiuje się makro, które każe nagłówkowi wygenerować także implementację (sekcja 3.1).

**Testy rejestrują się same.** Nie ma listy testów do uzupełniania. Makro `TEST_CASE` tworzy
funkcję i obiekt globalny, którego konstruktor dopisuje tę funkcję do rejestru biblioteki,
zanim ruszy `main`. Nowy test to nowy blok `TEST_CASE` w dowolnym pliku testowym.

### Za co doctest NIE odpowiada

- Nie buduje programu testowego i nie decyduje, które pliki do niego należą. To robi CMake.
- Nie jest programem `ctest`. `ctest` to osobne narzędzie z pakietu CMake, które uruchamia
  programy testowe i patrzy na ich kod wyjścia. O doctest nic nie wie.
- Nie otwiera okna i nie tworzy kontekstu OpenGL. Dlatego testami objęty jest tylko kod,
  który ich nie potrzebuje: kolizje, logika labiryntu, gracz, dwa loadery plików (OBJ i
  obrazy), a od M4 także przetwarzanie tekstu shadera w `gfx` (samo składanie napisów, bez
  kompilacji GLSL), matematyka świateł i macierz normalnych w `scene` oraz ustawienia
  oświetlenia w `game`. Od M5 dochodzą reguły rozgrywki: wyjście, kryształy i cała runda
  (`game/Exit`, `game/Crystals`, `game/Round`) oraz kule w `scene/Collider`. Klas `gfx`,
  które tworzą obiekty OpenGL, ani `NightMazeApp` w testach nie ma.
- Nie mierzy pokrycia kodu testami i niczego nie udowadnia o kodzie, którego żaden test nie
  woła.

## 2. Jak podpinamy doctest w CMake

### Pobranie biblioteki

Cały fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
# ---- doctest: unit tests ---------------------------------------------------------------
# doctest is a single header. Its CMake build can also compile a small static library that
# contains only main(): we write that one line ourselves in tests/main.cpp, so the library
# is switched off. So are the tests and examples of doctest itself and its install rules.
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

# The doctest target is an interface target that only carries the include path. Unlike
# GLFW and GLM it needs no extra step here: when doctest is not the main project, its own
# CMakeLists.txt already declares that path as SYSTEM, so the header cannot produce
# warnings in our tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)
```

Mechanizm FetchContent (`FetchContent_Declare`, `FetchContent_MakeAvailable`, `GIT_SHALLOW`,
zapis `CACHE BOOL "" FORCE`, powód przypinania wersji) jest opisany w
[`glfw.md`](glfw.md), sekcja 2. Tutaj tylko to, co dla doctest jest inne.

Repozytorium doctest ma własny `CMakeLists.txt`
(`build/debug/_deps/doctest-src/CMakeLists.txt`), więc `FetchContent_MakeAvailable(doctest)`
dołącza go jak `add_subdirectory`. Plik definiuje:

| Target | Alias | Rodzaj | Co zawiera |
|---|---|---|---|
| `doctest` | `doctest::doctest` | `INTERFACE` | tylko ścieżki nagłówków. Niczego nie kompiluje |
| `doctest_with_main` | `doctest::doctest_with_main` | biblioteka statyczna | plik `doctest/doctest.cpp` skompilowany z makrem `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, czyli gotowe `main`. Powstaje tylko przy `DOCTEST_WITH_MAIN_IN_STATIC_LIB` równym `ON` |

Trzy opcje:

| Opcja | Co robi | Wartość domyślna w doctest 2.5.3 | U nas |
|---|---|---|---|
| `DOCTEST_WITH_MAIN_IN_STATIC_LIB` | buduje bibliotekę `doctest_with_main` | `ON` | `OFF` |
| `DOCTEST_WITH_TESTS` | buduje testy i przykłady samego doctest | `ON` tylko gdy doctest jest projektem głównym | `OFF` |
| `DOCTEST_NO_INSTALL` | pomija reguły `install` | `OFF` | `ON` |

Realnie coś zmienia pierwsza i trzecia. Bez pierwszej każdy build kompilowałby bibliotekę,
której nikt nie linkuje, i to cudzy kod poza naszymi ustawieniami ostrzeżeń. Funkcję `main`
wolę mieć we własnym pliku `tests/main.cpp`: to jedna linia, którą widać i którą umiem
wyjaśnić (sekcja 3.1). Druga opcja i tak miałaby u nas wartość `OFF`. Ustawiam ją jawnie z
tego samego powodu co przy GLFW i GLM: żeby było widać intencję i żeby wynik nie zależał od
wartości domyślnych przyszłej wersji.

Linie `set(...)` muszą stać **przed** `FetchContent_MakeAvailable(doctest)`, bo doctest czyta
te opcje w chwili dołączenia.

### Nagłówek jako systemowy: tym razem bez naszego kroku

Przy GLFW i GLM sami kopiujemy listę `INTERFACE_INCLUDE_DIRECTORIES` do
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` ([`glm.md`](glm.md), sekcja 2). Przy doctest ten krok
jest zbędny. Jego `CMakeLists.txt` sprawdza, czy jest projektem głównym, i jeśli nie jest,
sam deklaruje ścieżki ze słowem `SYSTEM`:

```cmake
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/doctest/>
)
```

To fragment pliku doctest, nie naszego. Ścieżki są dwie: korzeń repozytorium (stąd zapis
`#include <doctest/doctest.h>`) i jego podkatalog `doctest/` (zadziałałoby też
`#include <doctest.h>`, którego nie używam).

Zmierzone na Windowsie w wygenerowanym projekcie `night_maze_tests.vcxproj`: oba katalogi
trafiają do kompilatora przez `/external:I`, a `ExternalWarningLevel` ma wartość
`TurnOffAllWarnings`. Pod `/W4 /permissive-` z nagłówka doctest i z rozwinięć jego makr w
naszych plikach testowych nie pojawia się żadne ostrzeżenie. Na macOS oczekuję flagi
`-isystem`, tak jak dla GLFW i GLM. Tego nie mierzyłem.

Plik doctest zaczyna się od `cmake_minimum_required(VERSION 3.14)`. Ma to znaczenie dla
Maca: CMake 4 odrzuca projekty, które deklarują minimum poniżej 3.5. Wersja 3.14 jest
powyżej tej granicy, więc konfiguracja w CMake 4.3 nie powinna mieć z tym problemu. To
wniosek z lektury pliku, nie pomiar.

### Program testowy

Fragmenty z [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# ---- game_logic: the rules of Night Maze that need no window and no OpenGL -------------
# The maze, its generator, its layout in the world, the player, the settings of the
# lighting and the rules of a round (exit, crystals, battery) are plain data and math.
# They live in a library of their own, and not in the night_maze executable, so that
# the test program can link them too: a test cannot link code that is inside another
# executable.
add_library(game_logic STATIC
    src/game/Crystals.cpp
    src/game/Crystals.hpp
    src/game/Exit.cpp
    src/game/Exit.hpp
    src/game/Lighting.cpp
    src/game/Lighting.hpp
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
    src/game/MazeWorld.cpp
    src/game/MazeWorld.hpp
    src/game/Player.cpp
    src/game/Player.hpp
    src/game/Round.cpp
    src/game/Round.hpp
)
# PUBLIC: the headers of this library (Exit.hpp, Lighting.hpp, MazeLayout.hpp,
# MazeWorld.hpp, Player.hpp, Round.hpp) include headers of engine (scene/Collider.hpp, scene/Light.hpp) and GLM,
# so whoever includes them needs the include paths of engine.
# The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)
```

Testy mogą wołać tylko kod, który da się do nich **dolinkować**, czyli kod z biblioteki. Kod
skompilowany wprost w programie `night_maze` jest dla innego programu niedostępny. Dlatego
logika labiryntu trafiła do osobnej biblioteki statycznej `game_logic`, którą linkują i gra,
i testy ([`../modules/game/README.md`](../modules/game/README.md), sekcja 3). Kolizje są w
`engine`, które `game_logic` linkuje jako `PUBLIC`, więc testy dostają je razem z nią.

Z oświetleniem (M4) podział jest ten sam. Kod bez okna, który ma mieć testy, trafił do
bibliotek: `gfx/ShaderSource`, `scene/Light`, `scene/LightBlock` i `scene::normalMatrix`
(`scene/Transform`) są w `engine`, a `game/Lighting` w `game_logic`. Kod, który woła OpenGL
(`gfx::UniformBuffer`, `game::LightRig`, panel Lights), testów nie ma. Z mapami normalnych tak
samo: matematyka stycznych (`assets/Tangents`) jest w `engine` i ma własny plik testów, a
funkcja `usesNormalMap` jest w `game/Lighting`. Kod GLSL (`common/normal_map.glsl`) i wiązanie
drugiej tekstury testów nie mają.

Rozgrywka (M5) jest podzielona tak samo, i to z myślą o testach. Reguły są zwykłymi danymi
i wolnymi funkcjami w `game_logic`: `game/Exit` (gdzie jest wyjście), `game/Crystals` (ile
kryształów, gdzie stoją i jak się ruszają) i `game/Round` (stan rundy i jeden krok reguł,
`updateRound`). Żadna z nich nie dołącza OpenGL ani `core::Input`: `updateRound` dostaje
pozycję gracza, przełącznik latarki i czas kroku jako argumenty. Dzięki temu
`tests/RoundTests.cpp` rozgrywa rundę od początku do wygranej bez okna: buduje labirynt z
ziarna, w pętli woła `updateRound` z pozycją pod kolejnymi kryształami i sprawdza baterię,
bramę i stan rundy. Kule (`scene::Sphere` i testy `overlaps`) są w `engine`. To, co rysuje
rundę i ją pokazuje (`game::GameplayRenderer`, `game::drawModel`, HUD, panel Gameplay,
uniform `uEmissive` w shaderach) oraz spięcie w `NightMazeApp` (klawisz R, kolejność w
`onUpdate`), testów nie ma.

```cmake
# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/ColliderTests.cpp
    tests/CrystalTests.cpp
    tests/ExitTests.cpp
    tests/ImageLoaderTests.cpp
    tests/LightTests.cpp
    tests/LightingTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/MazeWorldTests.cpp
    tests/ObjLoaderTests.cpp
    tests/PlayerTests.cpp
    tests/RoundTests.cpp
    tests/ShaderSourceTests.cpp
    tests/TangentTests.cpp
    tests/TransformTests.cpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests (of images and of OBJ models) read the real files of the game. A test
# must not depend on the directory it is started from, so the absolute path of assets/ in
# the repository is compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)
```

| Linia | Znaczenie |
|---|---|
| `enable_testing()` | włącza obsługę testów w CMake: podczas generowania powstaje w katalogu buildu plik z listą testów, który czyta `ctest`. Musi stać w głównym `CMakeLists.txt`, bo `ctest` szuka listy w korzeniu katalogu buildu |
| `add_executable(night_maze_tests ...)` | zwykły program z siedemnastu plików. Nie ma słowa `EXCLUDE_FROM_ALL`, więc buduje go każde `cmake --build --preset debug`. Dzięki temu testy zawsze się kompilują: zmiana w API, która je psuje, wychodzi przy pierwszym buildzie |
| `target_link_libraries(... PRIVATE game_logic doctest::doctest)` | kod testowany i biblioteka testów. "Linkowanie" targetu `INTERFACE` `doctest::doctest` oznacza tylko dodanie ścieżek nagłówków |
| `night_maze_enable_warnings(night_maze_tests)` | testy kompilują się z tymi samymi ścisłymi ostrzeżeniami co reszta naszego kodu (`/W4 /permissive-` albo `-Wall -Wextra -Wpedantic`) |
| `target_compile_definitions(night_maze_tests PRIVATE NIGHT_MAZE_ASSETS_DIR="...")` | makro preprocesora z bezwzględną ścieżką katalogu `assets` w repozytorium. Testy loaderów czytają nim prawdziwe modele i tekstury niezależnie od katalogu, z którego uruchomiono program ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7) |
| `add_test(NAME night_maze_tests COMMAND night_maze_tests)` | rejestruje **jeden** test CTest: "uruchom ten program". `COMMAND` z nazwą targetu CMake zamienia na pełną ścieżkę pliku wykonywalnego, także z podkatalogiem `Debug\` generatora Visual Studio |

Dla `ctest` cały program jest jednym testem: przechodzi, gdy kod wyjścia to 0. Liczbę
przypadków widać dopiero w wyjściu samego programu (sekcja 4). doctest ma moduł CMake, który
rejestruje każdy `TEST_CASE` jako osobny test CTest (`doctest_discover_tests`). Nie używam
go: jedna linia `add_test` jest prostsza, a szczegóły i tak pokazuje `--output-on-failure`.

Program testowy linkuje przez `engine` także GLFW i GLAD, choć żaden test ich nie woła. To
koszt tego, że `scene/Collider` leży w tej samej bibliotece co okno. Program nie tworzy okna
i nie potrzebuje karty graficznej.

## 3. Najważniejsze API

### 3.1. `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`: punkt wejścia

Cały plik [`tests/main.cpp`](../../tests/main.cpp):

```cpp
// Entry point of the test program night_maze_tests.
// See docs/libraries/doctest.md

// doctest is a single header. In exactly one .cpp file of the program this macro makes
// the header also emit its implementation and a main() function that runs every
// TEST_CASE of every file. The other test files include the header without the macro.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```

Nagłówek `doctest.h` ma dwie części. Zwykłe dołączenie daje tylko deklaracje i makra. Gdy
przed dołączeniem zdefiniowane jest `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, nagłówek dokłada
definicje wszystkich funkcji biblioteki i funkcję `main`. To musi się stać w **dokładnie
jednym** pliku `.cpp` programu: w zerze plików linker nie znajdzie `main`, w dwóch znajdzie
dwie definicje tej samej funkcji.

`#define` stoi przed `#include`, bo preprocesor czyta plik od góry: makro musi już istnieć,
gdy nagłówek sprawdza, czy je zdefiniowano.

### 3.2. `TEST_CASE` i `CHECK`

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("contains tells cells of the maze from everything else") {
    const game::Maze maze(3, 2);

    CHECK(maze.contains(0, 0));
    CHECK(maze.contains(2, 1));
    CHECK_FALSE(maze.contains(-1, 0));
    CHECK_FALSE(maze.contains(0, -1));
    CHECK_FALSE(maze.contains(3, 0));
    CHECK_FALSE(maze.contains(0, 2));
}
```

| Makro | Znaczenie |
|---|---|
| `TEST_CASE("nazwa")` | jeden **przypadek testowy** (test case): funkcja z nazwą w postaci zdania. Nazwa pojawia się w raporcie, więc ma mówić, co ma być prawdą |
| `CHECK(wyrażenie)` | **asercja** (assertion): sprawdza, że wyrażenie jest prawdą. Gdy nie jest, zapisuje błąd i **idzie dalej**, więc jeden przebieg pokazuje wszystkie nieudane sprawdzenia |
| `CHECK_FALSE(wyrażenie)` | sprawdza, że wyrażenie jest fałszem |

Nazwy testów są po angielsku, tak jak identyfikatory i komentarze w kodzie.

doctest rozkłada wyrażenie z porównaniem na lewą i prawą stronę i w razie błędu wypisuje
obie wartości. Tak wyglądał prawdziwy komunikat z chwili, gdy test oczekiwał jeszcze złych
liczb (ścieżka pliku skrócona):

```text
tests\MazeGeneratorTests.cpp(186): ERROR: CHECK( game::randomBelow(generator, BOUND) == expected ) is NOT correct!
  values: CHECK( 1 == 0 )
```

Pierwsza linia podaje plik, numer linii i tekst wyrażenia, druga wartości obu stron.

### 3.3. `REQUIRE`: gdy dalej nie ma sensu iść

`tests/MazeLayoutTests.cpp`:

```cpp
    REQUIRE(segments.size() == 4U);
    // North and south walls run along X, west and east walls along Z.
    CHECK(countSegments(segments, {1.0F, 0.0F, 0.0F}, game::WallAxis::AlongX) == 1);
```

`REQUIRE` działa jak `CHECK`, ale po nieudanym sprawdzeniu **przerywa cały przypadek
testowy** (rzuca wyjątek, który doctest łapie). Używam go, gdy dalsze sprawdzenia nie
miałyby sensu albo byłyby niebezpieczne: jeśli lista ma złą długość, sięganie do jej
elementów jest bez znaczenia. `REQUIRE_FALSE` to wersja dla fałszu.

Zasada: `CHECK` domyślnie, `REQUIRE` dla warunków, od których zależy reszta testu.

### 3.4. `SUBCASE`: wspólny początek, kilka zakończeń

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("removing a wall removes it for both cells that share it") {
    game::Maze maze(3, 2);

    SUBCASE("east side of a cell is the west side of its right neighbour") {
        maze.removeWall(0, 0, game::Direction::East);
        CHECK_FALSE(maze.hasWall(0, 0, game::Direction::East));
        CHECK_FALSE(maze.hasWall(1, 0, game::Direction::West));
    }

    SUBCASE("north side of a cell is the south side of the cell above it") {
        maze.removeWall(2, 1, game::Direction::North);
        CHECK_FALSE(maze.hasWall(2, 1, game::Direction::North));
        CHECK_FALSE(maze.hasWall(2, 0, game::Direction::South));
    }
```

(dalej w pliku są jeszcze dwa podprzypadki tego samego testu).

**Podprzypadek** (subcase) to gałąź wewnątrz przypadku testowego. doctest uruchamia cały
`TEST_CASE` **od początku osobno dla każdego podprzypadku**: za pierwszym razem wchodzi
tylko do pierwszego bloku `SUBCASE`, za drugim tylko do drugiego. Kod przed blokami
(tutaj `game::Maze maze(3, 2);`) wykonuje się więc za każdym razem na nowo.

Skutek: każdy podprzypadek dostaje **świeży labirynt** ze wszystkimi ścianami. Usunięcie
ściany w pierwszym nie wpływa na drugi. To zastępuje osobne funkcje "przygotuj" i "posprzątaj"
znane z innych bibliotek testowych.

### 3.5. `doctest::Approx`: porównywanie liczb zmiennoprzecinkowych

`tests/ColliderTests.cpp`:

```cpp
void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}
```

Liczby `float` nie przechowują większości ułamków dziesiętnych dokładnie: `0.1F + 0.2F` nie
jest równe `0.3F` co do bitu. Porównanie przez `==` zawodziłoby więc w poprawnym kodzie.
`doctest::Approx(wartość)` tworzy obiekt, dla którego `==` znaczy "równe z dokładnością do
małego błędu względnego". Domyślna tolerancja pokrywa zwykłe błędy zaokrągleń, a zmienia się
ją metodą `.epsilon(...)`. Używa jej jeden plik, `tests/TransformTests.cpp`, którego własna
funkcja `checkVector` porównuje składowe przez `doctest::Approx(expected.x).epsilon(0.0001)`.
Pozostałe testy zostają przy tolerancji domyślnej.

`checkVector` to zwykła funkcja pomocnicza, nie element biblioteki. Makra `CHECK` wolno
wołać z funkcji pomocniczych. W raporcie błędu jest wtedy linia wewnątrz `checkVector`, a nie
linia testu, który ją zawołał, i to jest cena tej wygody.

Zwykłego `==` na liczbach `float` używam w testach w dwóch miejscach świadomie: gdy wartość
ma przejść przez funkcję **bez zmiany** (`REQUIRE(allowed.z == step.z)`) i gdy liczby są
sumami i iloczynami 0,5, 1 i 2, które `float` przechowuje dokładnie (pozycje ścian).

### 3.6. `CHECK_THROWS_AS` i `CHECK_NOTHROW`: wyjątki

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("a maze with a wrong size cannot be created") {
    CHECK_THROWS_AS(game::Maze(0, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, 0), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(-3, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(game::Maze::MAX_SIZE + 1, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, game::Maze::MAX_SIZE + 1), std::invalid_argument);

    // The limits themselves are fine.
    CHECK_NOTHROW(game::Maze(1, 1));
    CHECK_NOTHROW(game::Maze(game::Maze::MAX_SIZE, game::Maze::MAX_SIZE));
}
```

| Makro | Znaczenie |
|---|---|
| `CHECK_THROWS_AS(wyrażenie, Typ)` | wyrażenie ma rzucić wyjątek typu `Typ` (albo pochodnego). Brak wyjątku i wyjątek innego typu to błąd testu |
| `CHECK_NOTHROW(wyrażenie)` | wyrażenie nie może rzucić żadnego wyjątku |

Tak testuje się obsługę złych danych: nie wystarczy, że dobry rozmiar działa, zły ma być
odrzucony w określony sposób.

### 3.7. `CAPTURE`: który obrót pętli zawiódł

`tests/MazeGeneratorTests.cpp`:

```cpp
    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size.width, size.height, seed);
```

Asercja w pętli wykonuje się setki razy z różnymi danymi. Gdy zawiedzie, sam komunikat
"oczekiwano 15, jest 14" nie mówi, dla którego labiryntu. `CAPTURE(zmienna)` zapamiętuje
nazwę i wartość zmiennej i dopisuje je do komunikatu **każdej nieudanej** asercji w tym
samym bloku. Przy sukcesie niczego nie wypisuje. Z raportu wiadomo wtedy od razu: szerokość
5, wysokość 3, ziarno 17.

### 3.8. Czego nie używam

| Element | Do czego służy | Dlaczego go nie ma |
|---|---|---|
| `TEST_SUITE` | grupowanie przypadków w nazwane zestawy | pliki są małe i tematyczne, nazwa pliku wystarcza |
| `TEST_CASE_FIXTURE` | klasa ze wspólnym stanem dla testów | `SUBCASE` robi to samo prościej |
| `WARN(...)` | sprawdzenie, które tylko ostrzega | test ma przechodzić albo nie |
| `doctest_discover_tests` (CMake) | osobny test CTest na każdy `TEST_CASE` | jedna linia `add_test` wystarcza (sekcja 2) |
| `DOCTEST_CONFIG_DISABLE` | usuwa testy z kompilacji | testy są w osobnym programie, w grze nie ma ich wcale |

## 4. Jak uruchamiać testy

Program testowy buduje się razem z całym projektem. Polecenia wykonuje się w katalogu
głównym repozytorium, po konfiguracji i buildzie
([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 2,
[`../guides/build-macos.md`](../guides/build-macos.md), sekcja 2).

### Przez `ctest`

Windows (w terminalu ze środowiskiem deweloperskim) i macOS, te same polecenia:

```sh
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```

| Argument | Znaczenie |
|---|---|
| `--test-dir build/debug` | katalog buildu, w którym `ctest` ma szukać listy testów |
| `-C Debug` | konfiguracja do przetestowania. **Wymagana z generatorem Visual Studio**, bo jeden katalog buildu mieści tam kilka konfiguracji i `ctest` musi wiedzieć, który program uruchomić. Generatory jednokonfiguracyjne (Unix Makefiles na Macu, Ninja) ten argument ignorują, więc polecenie może być wspólne |
| `--output-on-failure` | gdy test nie przejdzie, `ctest` wypisuje całe wyjście programu testowego, czyli raport doctest. Bez tego widać tylko słowo `Failed` |

Wynik zmierzony na Windowsie (Debug, 2026-10-05, przed dodaniem testów oświetlenia):

```text
    Start 1: night_maze_tests
1/1 Test #1: night_maze_tests .................   Passed    0.40 sec

100% tests passed, 0 tests failed out of 1
```

"1 test" to cały program (sekcja 2). Kod wyjścia `ctest` to 0. W Release ten sam test trwał
wtedy około 0,1 s. Dla dzisiejszego programu z 215 przypadkami (2026-10-05) znane są liczby
z raportu doctest niżej. Wyjścia `ctest` z tego dnia nie zapisałem, więc blok wyżej zostaje z
datą swojego pomiaru: jego postać się nie zmienia, inny może być tylko czas.

W pliku [`Makefile`](../../Makefile) są do tego skróty: `make test` (build Debug i testy),
`make test-release`, a `make check` uruchamia oba razem z resztą kontroli
([`../guides/project-structure.md`](../guides/project-structure.md), sekcja 3.12). Skrótów
nikt jeszcze nie uruchomił: na PC nie ma programu `make`, a na Macu ten kod nie był jeszcze
budowany.

### Bezpośrednio

Program testowy można uruchomić samodzielnie. Wtedy widać raport doctest:

```bat
build\debug\Debug\night_maze_tests.exe
```

```sh
./build/debug/night_maze_tests
```

Wynik z Windowsa po M5, 2026-10-05 (te same liczby w Debug i w Release):

```text
[doctest] doctest version is "2.5.3"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:   215 |   215 passed | 0 failed | 0 skipped
[doctest] assertions: 85098 | 85098 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Z tego uruchomienia znane są obie liczby i to, że wszystkie testy przeszły. Postać raportu
(nagłówek, kreska, odstępy przed liczbami) odtworzyłem z wcześniejszego raportu z
2026-10-05, który pokazywał 163 przypadki i 62220 asercji: doctest wyrównuje obie liczby do
szerokości dłuższej z nich.

Nad tym raportem program wypisuje kilka linii `[error]`: pochodzą z testów, które celowo
podają loaderom zły plik, i nie oznaczają nieudanego testu.

Asercji jest dużo więcej niż przypadków, bo wiele z nich stoi w pętlach: własności labiryntu
są sprawdzane dla 200 labiryntów, komórka po komórce. Testy z M5 robią to samo dla wyjścia
i kryształów (pętle po ziarnach w `ExitTests.cpp` i `CrystalTests.cpp`).

Przydatne opcje programu (pełna lista: `--help`):

| Opcja | Co robi |
|---|---|
| `-ltc` albo `--list-test-cases` | wypisuje nazwy wszystkich przypadków, niczego nie uruchamia |
| `-tc="golden*"` albo `--test-case="golden*"` | uruchamia tylko przypadki o pasującej nazwie. `*` zastępuje dowolny tekst. Zmierzone, gdy program miał mniej przypadków: `-tc="golden*"` uruchamia 1 przypadek i pomija wszystkie pozostałe |
| `-sf="*Collider*"` albo `--source-file=...` | filtruje po nazwie pliku źródłowego z testami |
| `-s` albo `--success` | wypisuje także asercje, które przeszły |
| `-d` albo `--duration` | wypisuje czas każdego przypadku |

### Kiedy uruchamiać

Po każdej zmianie w `src/scene/Collider.*`, `src/game/Maze*`, `src/game/Player.*`, w
loaderach z `src/assets/`, a od M4 także w `src/gfx/ShaderSource.*`, `src/scene/Light.*`,
`src/scene/LightBlock.*`, `src/scene/Transform.*` i `src/game/Lighting.*`, przed każdym commitem (na
Macu robi to `make check`) i po przejściu na drugi system. To ostatnie ma tu szczególne
znaczenie: test labiryntu wzorcowego istnieje po to, żeby wykryć różnicę między macOS a
Windowsem ([`../modules/game/maze-generator.md`](../modules/game/maze-generator.md),
sekcja 5.8).

## 5. Pułapki

1. **Makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` w dwóch plikach albo w żadnym.** W dwóch:
   błąd linkera o podwójnej definicji `main` i funkcji doctest. W żadnym: błąd linkera o
   braku `main`. Makro jest w `tests/main.cpp` i tylko tam.
2. **Nowy plik testowy bez wpisu w `CMakeLists.txt`.** Lista plików jest jawna. Plik, którego
   nie ma w `add_executable(night_maze_tests ...)`, nie jest kompilowany, a jego testy po
   cichu nie istnieją: raport pokazuje mniej przypadków, ale nadal `SUCCESS`. Po dodaniu
   pliku warto sprawdzić, czy liczba przypadków wzrosła.
3. **`ctest` bez `-C` z generatorem Visual Studio.** `ctest` nie wie wtedy, którą konfigurację
   uruchomić, i test nie jest wykonywany. Zmierzone na Windowsie: komunikat
   `Test not available without configuration.  (Missing "-C <config>"?)`, wynik
   `***Not Run` i kod wyjścia 8. Zawsze `-C Debug` albo `-C Release`.
4. **`ctest` uruchamia to, co jest zbudowane.** `ctest` niczego nie buduje. Po zmianie kodu
   trzeba najpierw wykonać `cmake --build`, inaczej testowany jest stary program.
5. **`==` na liczbach `float`.** Poprawny kod nie przechodzi testu przez błąd zaokrąglenia na
   ostatniej cyfrze. Do wyników obliczeń zmiennoprzecinkowych służy `doctest::Approx`
   (sekcja 3.5).
6. **Sumowanie w pętli.** Po 2000 dodawań `float` suma różni się od iloczynu na tyle, że
   nawet `Approx` z domyślną tolerancją zgłasza błąd (zmierzone przy pisaniu testów: 30,2004
   zamiast 30,2). Lepiej sprawdzać pojedynczy krok niż sumę wielu.
7. **Różne typy po obu stronach porównania.** `CHECK(vector.size() == 4)` porównuje liczbę
   bez znaku z liczbą ze znakiem wewnątrz szablonów doctest, co pod ścisłymi ostrzeżeniami
   może dać ostrzeżenie o mieszaniu znaków. W testach stała ma przyrostek `U` (`4U`) albo
   typ `std::size_t`.
8. **`REQUIRE` w funkcji pomocniczej albo w destruktorze.** `REQUIRE` przerywa test
   wyjątkiem. W zwykłej funkcji pomocniczej działa, ale w destruktorze i w funkcji
   `noexcept` kończy program.
9. **Stan wspólny między podprzypadkami.** Zmienna zadeklarowana przed blokami `SUBCASE`
   jest tworzona od nowa dla każdego z nich, ale zmienna globalna albo statyczna nie.
   Testy nie mają żadnego stanu globalnego: generator liczb losowych jest zawsze zmienną
   lokalną z jawnym ziarnem.
10. **Asercja w ciasnej pętli.** Każde `CHECK` jest liczone i kosztuje czas. Test wędrówki po
    labiryncie miał najpierw asercję dla każdej przeszkody w każdym kroku, co dawało prawie
    4 miliony asercji. Teraz zbiera wynik do jednej zmiennej `bool` i sprawdza ją raz na
    serię kroków.
11. **Nazwa testu jako filtr.** Opcja `-tc` traktuje przecinek jako separator wzorców, więc
    nazwa z przecinkiem wymaga poprzedzenia go ukośnikiem wstecznym. Prościej filtrować
    początkiem nazwy z gwiazdką.
12. **Testy nie obejmują niczego z OpenGL.** Zielony wynik testów mówi o kolizjach,
    labiryncie, graczu, loaderach plików, matematyce świateł, stycznych, składaniu tekstu
    shadera i, od M5, o regułach rundy (wyjście, kryształy, bateria, brama, wygrana). O
    kompilacji shaderów, buforach i rysowaniu nie mówi nic: te rzeczy sprawdza się
    uruchomieniem programu. Dwa przykłady z M4. `tests/ShaderSourceTests.cpp` sprawdza, że
    `#include` jest zastępowany treścią pliku i że numer w komunikacie błędu zamienia się w
    nazwę pliku, ale żaden test nie podaje tego tekstu kompilatorowi GLSL: format błędów
    sterownika Apple (`ERROR: 1:15:`) jest w testach wpisanym napisem, a nie wyjściem
    prawdziwego sterownika. `tests/LightTests.cpp` sprawdza przesunięcia pól struktury
    `scene::LightBlockData`, ale tego, czy sterownik układa blok `LightBlock` w tylu samych
    bajtach, pilnuje dopiero porównanie rozmiarów w działającej grze
    (`Shader::bindUniformBlock`). Przykład z M5: `tests/RoundTests.cpp` sprawdza, że pusta
    bateria wyłącza latarkę i że brama przestaje blokować po zebraniu dość kryształów, ale
    tego, czy kryształ naprawdę świeci na ekranie, czy brama opada w podłogę i czy HUD
    pokazuje właściwe liczby, żaden test nie widzi.

## 6. Pytania kontrolne

1. **Co to jest test jednostkowy i po co go pisać?**
   Mały kawałek kodu, który woła jedną funkcję z konkretnymi danymi i sprawdza wynik.
   Uruchamiany po każdej zmianie wykrywa od razu, że coś, co działało, przestało działać.

2. **Co znaczy, że doctest jest biblioteką z jednego nagłówka, i skąd bierze się `main`?**
   Cała biblioteka to plik `doctest/doctest.h`. W jednym pliku `.cpp` (`tests/main.cpp`)
   przed dołączeniem nagłówka zdefiniowane jest makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`,
   które każe nagłówkowi wygenerować implementację i funkcję `main`.

3. **Skąd program wie, jakie testy ma uruchomić?**
   Makro `TEST_CASE` tworzy funkcję i obiekt globalny, który przed startem `main` dopisuje
   ją do rejestru biblioteki. Nie ma ręcznej listy testów.

4. **Czym różni się `CHECK` od `REQUIRE`?**
   Po nieudanym `CHECK` test idzie dalej, po nieudanym `REQUIRE` cały przypadek testowy jest
   przerywany. `REQUIRE` jest dla warunków, bez których reszta testu nie ma sensu.

5. **Jak działa `SUBCASE`?**
   Przypadek testowy jest uruchamiany od początku osobno dla każdego podprzypadku, za każdym
   razem z wejściem tylko do jednego bloku. Kod przed blokami wykonuje się za każdym razem,
   więc każdy podprzypadek dostaje świeże dane.

6. **Po co `doctest::Approx`?**
   Liczby `float` mają błędy zaokrągleń, więc `==` zawodzi w poprawnym kodzie. `Approx`
   porównuje z małą tolerancją względną.

7. **Dlaczego logika labiryntu jest w bibliotece `game_logic`?**
   Program testowy może dolinkować tylko kod z biblioteki. Kod skompilowany wprost w
   programie `night_maze` byłby dla testów niedostępny.

8. **Co robią `enable_testing()` i `add_test`?**
   `enable_testing()` włącza zapis listy testów do katalogu buildu. `add_test` dopisuje do
   niej jeden test: uruchomienie programu `night_maze_tests`. Program `ctest` czyta listę,
   uruchamia program i uznaje test za zaliczony, gdy kod wyjścia to 0.

9. **Po co `-C Debug` w poleceniu `ctest`?**
   Generator Visual Studio trzyma w jednym katalogu buildu kilka konfiguracji, więc `ctest`
   musi wiedzieć, którą uruchomić. Generatory jednokonfiguracyjne ignorują ten argument.

10. **Dlaczego nagłówek doctest nie daje ostrzeżeń pod `/W4`?**
    Jest dołączany jako nagłówek systemowy: `CMakeLists.txt` doctest deklaruje ścieżki ze
    słowem `SYSTEM`, gdy doctest nie jest projektem głównym. Na Windowsie daje to
    `/external:I` z wyłączonymi ostrzeżeniami.

11. **Czego testy w tym projekcie nie sprawdzają?**
    Niczego, co potrzebuje okna albo kontekstu OpenGL: klas `gfx` tworzących obiekty
    OpenGL, kompilacji shaderów, rysowania, sterowania, HUD i paneli. Sprawdzają kod, który
    jest samą matematyką, logiką i pracą na tekście (w `gfx` to jeden plik, `ShaderSource`).

12. **Jak da się przetestować całą rundę gry bez okna?**
    Reguły rundy są wolnymi funkcjami w bibliotece `game_logic` i dostają wszystko w
    argumentach: `updateRound(round, world, settings, feetPosition, flashlightOn,
    stepSeconds)`. Test sam podaje pozycję gracza i czas kroku, więc nie potrzebuje ani
    klawiatury, ani pętli głównej, ani OpenGL. `tests/RoundTests.cpp` buduje labirynt z
    ziarna, "stawia" gracza pod kolejnymi kryształami i sprawdza baterię, bramę i wygraną.

## 7. Oficjalna dokumentacja

- Repozytorium doctest: <https://github.com/doctest/doctest>
- Samouczek: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/tutorial.md>
- Asercje (w tym `Approx` i makra wyjątków): <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/assertions.md>
- Przypadki testowe i podprzypadki: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/testcases.md>
- Opcje wiersza poleceń: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/commandline.md>
- Własna funkcja `main` i makra konfiguracji: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/main.md>
- Dokumentacja CMake, `enable_testing`, `add_test` i program `ctest`: <https://cmake.org/cmake/help/latest/command/add_test.html>, <https://cmake.org/cmake/help/latest/manual/ctest.1.html>
- Kopia dokładnie dla naszej wersji leży po pierwszej konfiguracji w
  `build/debug/_deps/doctest-src/`: katalog `doc/markdown/` (te same dokumenty), plik
  `CMakeLists.txt` (targety i opcje z sekcji 2) i sam nagłówek `doctest/doctest.h`.
