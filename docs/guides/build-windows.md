# Budowanie na Windowsie

> **Stan na 2026-10-05: build z terminala jest sprawdzony na Windowsie, część ręczna nie.**
> Kod M0 i M1 powstał na macOS (patrz [`build-macos.md`](build-macos.md)). Na Windowsie 11
> zbudowałem go i uruchomiłem po raz pierwszy 2026-10-05, samymi narzędziami "Visual Studio
> Build Tools 2022", bez środowiska Visual Studio (IDE).
>
> **Zmierzone 2026-10-05:** konfiguracja i build Debug oraz Release (zero ostrzeżeń pod
> `/W4`), uruchomienie programu, kopia katalogu `assets`, błąd kompilacji shadera przy
> starcie, generator Ninja. Kod z M2 + M3 (kolizje, labirynt, gracz, modele, tekstury, panele
> Maze, Collision i Assets) powstał na tym PC: build Debug, Release i Ninja bez ostrzeżeń,
> testy jednostkowe przechodzące w obu konfiguracjach, start gry w oteksturowanym labiryncie bez linii
> `[error]` i zrzuty ekranu kilku stanów (sekcja 12).
>
> **Zmierzone 2026-10-05 (M4: oświetlenie i mapy normalnych):** build Debug i Release bez
> ostrzeżeń, 163 przypadki testowe i 62220 asercji w obu konfiguracjach, clang-format i
> clang-tidy bez uwag, start gry w oświetlonej nocnej scenie bez linii `[error]`, w tym bez
> żadnej z nazwą błędu OpenGL (`GL_...`), zrzuty ekranu czterech trybów oświetlenia i
> kilku innych stanów (sekcja 13.1) oraz zrzuty ekranu map normalnych: kierunek reliefu,
> reakcja na kierunek światła, tryby `Gouraud` i `Unlit` bez zmian (sekcja 13.3).
>
> **Nadal niesprawdzone:** wszystko, co wymaga człowieka przy myszy i klawiaturze (chodzenie
> i ślizganie po ścianach, klawisze N i F, obrót myszą, przyciski, listy i suwaki paneli, w
> tym lista `Lighting` i cały panel Lights, rozwijanie panelu Camera, zmiana rozmiaru okna,
> docking, przycisk "Reload shaders"), praca w Visual Studio (Open Folder, F5, Build
> Solution), RenderDoc i clangd w edytorze. Zdania o tych rzeczach są nadal przewidywaniem i
> są tak oznaczone. Listy kontrolne w sekcjach 11 (pierwszy build, stan M1), 12 (M2 + M3) i
> 13 (oświetlenie, M4) rozróżniają punkty zmierzone (`[x]`, z wynikiem) od otwartych (`[ ]`).
> Sekcje 11 i 12 są zapisem stanu z 2026-10-05: liczby i teksty paneli w ich punktach `[x]`
> opisują program z tamtego dnia. To, co program pokazuje dziś, opisują sekcje 2 i 13.

## 1. Wymagania

| Narzędzie | Po co | Uwagi |
|---|---|---|
| Visual Studio 2022 albo same "Build Tools for Visual Studio 2022", z pakietem roboczym "Desktop development with C++" (Programowanie aplikacji klasycznych w C++) | kompilator MSVC, Windows SDK, MSBuild, dołączone CMake i Ninja | do pracy z terminala wystarczają same Build Tools (tak było na moim PC). IDE jest potrzebne tylko do sekcji 4. Alternatywa: CLion |
| git dostępny w `PATH` | CMake klonuje nim GLFW, GLM, ImGui, doctest i stb podczas konfiguracji | sprawdzenie: `git --version` w nowym oknie terminala. Instalator: <https://git-scm.com/> |
| CMake w wersji co najmniej 3.24 | konfiguracja i build | jest częścią pakietu roboczego C++ (u mnie 3.31.6-msvc6). Osobny instalator: <https://cmake.org/download/> |

Środowisko, na którym wykonałem pomiary z tego dokumentu (2026-10-05 i 2026-10-05):

| Element | Wersja |
|---|---|
| System | Windows 11 Pro 10.0.26200 |
| Narzędzia | Visual Studio Build Tools 2022 17.14.37516, pakiet roboczy C++ (bez IDE) |
| Kompilator | MSVC `cl` 19.44.35228 (zestaw narzędzi 14.44.35207) |
| Windows SDK | 10.0.26100.0 |
| MSBuild | 17.14.51 |
| CMake | 3.31.6-msvc6 (dołączony do Build Tools) |
| Karta graficzna | NVIDIA GeForce RTX 4070 Ti SUPER (oraz zintegrowana AMD Radeon) |

Uwagi:

- Instalacja Visual Studio albo Build Tools nie dodaje `cmake` do `PATH` zwykłego terminala
  (potwierdzone: w zwykłym terminalu polecenie `cmake` nie istnieje). Są dwie drogi: wejść w
  środowisko deweloperskie (sekcja 2) albo zainstalować CMake osobno z opcją dodania do
  `PATH`.
- Git jest potrzebny w tym samym terminalu, w którym uruchamiamy CMake. Jeśli
  `git --version` nie działa, konfiguracja zakończy się błędem przy pobieraniu GLFW.
- Bibliotek nie instalujemy ręcznie. GLFW, GLM, ImGui, doctest i stb (dla stb_image) pobiera
  CMake, GLAD jest w repozytorium.
- Sterownik karty graficznej musi obsługiwać OpenGL 4.1 lub nowszy. Aktualne sterowniki
  kart NVIDIA, AMD i Intel obsługują 4.6. Przy bardzo starym sterowniku okno się nie utworzy.

## 2. Budowanie z terminala

Te same presety co na Macu. Polecenia wykonujemy w katalogu głównym repozytorium, w
terminalu ze środowiskiem deweloperskim Visual Studio.

### Środowisko deweloperskie

Do środowiska wchodzi się skrótem "Developer PowerShell for VS 2022" z menu Start albo, w
dowolnym oknie PowerShell, skryptem, który ten skrót uruchamia. Tak robiłem przy pomiarach:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
```

- Ścieżka dotyczy samych Build Tools. Przy zainstalowanym IDE zamiast `BuildTools` jest
  nazwa edycji, na przykład `Community`.
- Skrypt dopisuje do `PATH` kompilator `cl.exe`, MSBuild oraz dołączone CMake i Ninja. Działa
  tylko w tym oknie terminala.
- Zawsze podawałem `-Arch amd64 -HostArch amd64`, czyli kompilator 64 bitowy. Jak zachowuje
  się powłoka bez tych opcji, nie mierzyłem. Ma to znaczenie dla generatora Ninja, który
  używa tego `cl.exe`, który akurat jest w `PATH` (sekcja 4). Generator Visual Studio sam
  wybiera kompilator.

### Debug

```bat
cmake --preset debug
cmake --build --preset debug
build\debug\Debug\night_maze.exe
```

### Release

```bat
cmake --preset release
cmake --build --preset release
build\release\Release\night_maze.exe
```

Zwróć uwagę na dodatkowy katalog `Debug` lub `Release` w ścieżce programu. Na Macu program
leży w `build/debug/night_maze`. Wyjaśnienie w następnej sekcji.

### Co powinno się pojawić (zmierzone)

- `cmake --preset debug` kończy się bez błędów. Wypisuje jedno ostrzeżenie o nieużytej
  zmiennej `CMAKE_BUILD_TYPE`. Jest ono oczekiwane (sekcja 3).
- `cmake --build --preset debug` i `cmake --build --preset release` kończą się kodem 0, bez
  ostrzeżeń i bez błędów. W wyjściu buildu jest linia `Copying assets next to the
  executable` (sekcja 7).
- Program otwiera okno z nocnym widokiem z wnętrza labiryntu: kamienna podłoga, ściany i
  słupki z teksturą, oświetlone (tryb startowy to Blinn-Phong). Świecą trzy rodzaje świateł:
  słabe, chłodne światło księżyca (kierunkowe), ciepły stożek latarki gracza na środku
  obrazu (reflektor) i turkusowe światła punktowe w ślepych zaułkach, każde oznaczone małą
  świecącą kostką. Miejsca, do których żadne z nich nie dociera, są ciemne, ale nie czarne
  (światło otoczenia). Cieni nie ma: światła świecą przez ściany (cienie są w planie M7).
  Nad ścianami jest prawie czarne, granatowe tło: kolor czyszczenia to
  `{0.01F, 0.015F, 0.04F}`, ciemniejszy niż przed M4 (`{0.02F, 0.03F, 0.08F}`). Kolorowa
  kostka z M1 wisi nad komórką w przeciwległym rogu labiryntu, więc ze startu zwykle jej
  nie widać. Widok startowy jest sprawdzony na zrzucie ekranu (sekcja 13.1). W terminalu
  pierwsze dwie linie to:

  ```text
  [info] GL_VERSION:  4.1.0 NVIDIA 610.74
  [info] GL_RENDERER: NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2
  ```

  Napisy zależą od karty i sterownika. Sterownik NVIDII oddał kontekst dokładnie w wersji
  4.1, o którą prosi program. Komputer ma też zintegrowaną kartę AMD Radeon: system sam
  wybrał kartę NVIDIA. Po nich pamięć podręczna assetów wypisuje po jednej linii `[info]`
  na każdy wczytany plik (`Loaded texture: ...` dla czterech tekstur, czyli dwóch obrazów
  koloru i od M4 dwóch map normalnych, i `Loaded model: ...` dla trzech modeli, co wynika z
  kodu `assets::AssetCache`). Zmierzone jest to, że na starcie
  nie ma żadnej linii `[error]`, w tym żadnej z nazwą błędu OpenGL (`GL_INVALID_...`), także
  po dodaniu oświetlenia (2026-10-05). Dokładnej liczby linii `[info]` przy tych pomiarach
  nie zapisałem.
- Widocznych jest siedem paneli w ciemnym, granatowym motywie: Renderer nad Lights w lewej
  kolumnie, Maze nad Assets w prawej, Collision i Shaders na dole między kolumnami, a
  Camera u góry, tuż na prawo od lewej kolumny, zwinięty do samego paska tytułu (rozwija go
  kliknięcie strzałki w tym pasku). Każdy ma w kodzie miejsce i rozmiar startowy, ułożone
  dla okna 1280 x 720 (`src/debug/PanelLayout.hpp`). Działają one tylko wtedy, gdy w
  katalogu roboczym nie ma pliku `imgui.ini` z wpisem danego panelu (sekcja 7). To nie jest
  cecha Windowsa.
- Tekst paneli jest w czcionce Atkinson Hyperlegible z pliku
  `assets\fonts\AtkinsonHyperlegible-Regular.ttf` w kopii katalogu `assets` obok programu.
  Gdy tego pliku brakuje, w konsoli jest jedna linia
  `[error] Panel font cannot be loaded, using the built-in font: ...`, a panele używają
  czcionki wbudowanej w ImGui (zmierzone).

Opis samego pliku presetów (ukryty preset `base`, `inherits`, `binaryDir`) jest w
[`build-macos.md`](build-macos.md), sekcja 3. Plik jest wspólny dla obu systemów.

### Testy jednostkowe

Zwykły build buduje też program testowy `night_maze_tests.exe` (kolizje, labirynt, gracz,
loadery, a od M4 także tekst shaderów z `#include`, matematyka świateł, ustawienia
oświetlenia, macierz normalnych i styczne wierzchołków: kod bez okna). Testy uruchamia `ctest`, program z pakietu CMake, dostępny w tym samym środowisku
deweloperskim:

```bat
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```

- `-C Debug` jest **wymagane** z generatorem Visual Studio: jeden katalog buildu mieści tu
  kilka konfiguracji (sekcja 3) i `ctest` musi wiedzieć, którą uruchomić. Zmierzone bez tego
  argumentu: `Test not available without configuration.  (Missing "-C <config>"?)`, wynik
  `***Not Run`, kod wyjścia 8.
- `--output-on-failure` wypisuje raport programu testowego, gdy test nie przejdzie.
- `ctest` niczego nie buduje. Po zmianie kodu najpierw `cmake --build --preset debug`.

Zmierzone 2026-10-05 (po czystym buildzie obu presetów, bez ostrzeżeń, przed dodaniem testów
oświetlenia: wyjścia `ctest` z 2026-10-05 nie zapisałem, inny może być w nim tylko
czas):

```text
    Start 1: night_maze_tests
1/1 Test #1: night_maze_tests .................   Passed    0.40 sec

100% tests passed, 0 tests failed out of 1
```

Dla `ctest` cały program jest jednym testem. Szczegóły pokazuje sam program:

```bat
build\debug\Debug\night_maze_tests.exe
```

```text
[doctest] doctest version is "2.5.3"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:   163 |   163 passed | 0 failed | 0 skipped
[doctest] assertions: 62220 | 62220 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Te same liczby daje `build\release\Release\night_maze_tests.exe` (oba pomiary 2026-10-05,
po dodaniu map normalnych. Zmierzone są liczby i napis `Status: SUCCESS!`, odstępy przed
liczbami odtworzyłem z wcześniejszego raportu). Przypadki w plikach:
`ColliderTests.cpp` 12, `ImageLoaderTests.cpp` 9, `LightingTests.cpp` 17, `LightTests.cpp`
20, `MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12, `MazeTests.cpp` 6,
`MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 20, `PlayerTests.cpp` 13,
`ShaderSourceTests.cpp` 22, `TangentTests.cpp` 9, `TransformTests.cpp` 4, razem 163. Po
kroku łączącym M2 + M3 program miał osiem plików z testami, dziś ma trzynaście.
Program testowy nie otwiera okna. Opis biblioteki, makr i opcji programu:
[`../libraries/doctest.md`](../libraries/doctest.md).

Plik [`Makefile`](../../Makefile) ze skrótami (`make run`, `make check`) na Windowsie nie był
uruchamiany: na moim PC nie ma programu `make`. Plik wymaga `make` i powłoki typu Unix (na
przykład Git Bash), a bez nich wystarczą polecenia `cmake` podane wyżej. Opis:
[`project-structure.md`](project-structure.md), sekcja 3.12.

## 3. Generator Visual Studio jest wielokonfiguracyjny

To najważniejsza różnica względem Maca i częste pytanie na przeglądzie kodu.

**Generator** to część CMake, która tworzy pliki dla konkretnego systemu budowania. Nasze
presety nie wskazują generatora, więc CMake wybiera domyślny dla platformy:

| | macOS | Windows z Visual Studio 2022 albo Build Tools 2022 |
|---|---|---|
| Domyślny generator | Unix Makefiles | Visual Studio 17 2022 |
| Rodzaj | jednokonfiguracyjny (single-config) | wielokonfiguracyjny (multi-config) |
| Kiedy wybieramy Debug lub Release | przy **konfiguracji**, zmienną `CMAKE_BUILD_TYPE` | przy **budowaniu**, opcją `--config` |
| Co zawiera katalog buildu | pliki dla jednej konfiguracji | rozwiązanie `.sln` ze wszystkimi konfiguracjami naraz |
| Ścieżka programu | `build/debug/night_maze` | `build\debug\Debug\night_maze.exe` |

Kolumna Windows jest zmierzona w dwóch miejscach: po `cmake --preset debug` z terminala wpis
`CMAKE_GENERATOR` w `build\debug\CMakeCache.txt` ma wartość `Visual Studio 17 2022`, a
program powstaje w `build\debug\Debug\night_maze.exe`.

Generator jednokonfiguracyjny ustala typ buildu raz, podczas `cmake --preset debug`.
Generator wielokonfiguracyjny tworzy projekt, który zna wszystkie konfiguracje (Debug,
Release, RelWithDebInfo, MinSizeRel), a wybór następuje dopiero przy budowaniu. Żeby wyniki
się nie nadpisywały, każda konfiguracja dostaje własny podkatalog, stąd `Debug\` w ścieżce.

Konsekwencje dla naszego `CMakePresets.json`:

```json
"cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
```

Generator Visual Studio **ignoruje `CMAKE_BUILD_TYPE`**. Ta linia działa na Macu, na
Windowsie nie robi nic. Widać to w wyjściu konfiguracji: CMake wypisuje ostrzeżenie
`Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE`. To ostrzeżenie
jest oczekiwane i niczego nie trzeba z nim robić.

```json
"buildPresets": [
    { "name": "debug", "displayName": "Debug", "configurePreset": "debug", "configuration": "Debug" },
```

Pole **`configuration` w presecie budowania** wybiera konfigurację na Windowsie.
`cmake --build --preset debug` jest tam równoważne `cmake --build build\debug --config Debug`.
Na Macu z kolei to pole jest ignorowane.

Dlatego presety ustawiają **obie** rzeczy: `CMAKE_BUILD_TYPE` dla generatorów
jednokonfiguracyjnych i `configuration` dla wielokonfiguracyjnych. Ten sam plik działa
poprawnie na obu systemach, a każdy system korzysta z "swojej" połowy.

Skutek uboczny wart zrozumienia: na Windowsie katalogi `build\debug` i `build\release` są
technicznie takim samym projektem Visual Studio. O tym, co powstanie, decyduje preset
budowania, a nie preset konfiguracji. Trzymamy się jednak pary `debug` z `debug` i `release`
z `release`, żeby polecenia były identyczne jak na Macu.

Jeszcze jedna różnica: `CMAKE_EXPORT_COMPILE_COMMANDS` działa tylko z generatorami Makefile
i Ninja. Generator Visual Studio nie tworzy `compile_commands.json`. Visual Studio go nie
potrzebuje, ale clangd i clang-tidy na Windowsie wymagają generatora Ninja (podsekcja
"Cursor i VS Code z clangd na Windowsie" w sekcji 4).

## 4. Otwieranie folderu w Visual Studio

**Ta sekcja nie została sprawdzona.** Na moim PC są same Build Tools, bez IDE, więc kroków z
Visual Studio (Open Folder, wybór elementu startowego, F5, Build Solution) nie mogłem
wykonać. Opis wynika z dokumentacji narzędzi.

Visual Studio 2022 ma wbudowaną obsługę CMake i presetów. Nie tworzymy ręcznie pliku `.sln`.

1. File, Open, Folder i wskazujemy katalog repozytorium (albo "Open a local folder" na
   ekranie startowym).
2. Visual Studio wykrywa `CMakeLists.txt` i `CMakePresets.json` i uruchamia konfigurację.
   Postęp i błędy widać w oknie Output (lista "CMake").
3. Na pasku narzędzi pojawiają się listy rozwijane: preset konfiguracji (`Debug` lub
   `Release`, czyli nasze `displayName`) i preset budowania.
4. Jako element startowy (Select Startup Item) wybieramy `night_maze.exe`.
5. F5 uruchamia z debuggerem, Ctrl+F5 bez niego.

Do sprawdzenia przy pierwszym uruchomieniu w IDE:

- **Którego generatora użyje Visual Studio.** Nasz preset nie podaje pola `generator`.
  Z terminala CMake wybiera generator Visual Studio (zmierzone, sekcja 3). Otwierając folder
  w IDE, Visual Studio może zastosować własny wybór generatora, w tym Ninja, którą ma w
  zestawie. Z Ninja build jest jednokonfiguracyjny: działa `CMAKE_BUILD_TYPE`, a program
  leży bezpośrednio w katalogu buildu, bez podkatalogu `Debug`. Presety są przygotowane na
  oba przypadki, ale ścieżka programu będzie inna. Użyty generator widać w pierwszych
  liniach okna Output oraz w `build\debug\CMakeCache.txt` (wpis `CMAKE_GENERATOR`).
- **Nie mieszać generatorów w jednym katalogu.** Jeśli `build\debug` utworzył terminal
  jednym generatorem, a IDE spróbuje użyć innego, CMake zgłosi błąd o niezgodności
  generatora. Rozwiązanie: usunąć `build\debug` i trzymać się jednego sposobu pracy.

Katalogi `.vs/` i `out/` (domyślne katalogi robocze Visual Studio) są w `.gitignore`.

### Cursor i VS Code z clangd na Windowsie

Repozytorium zawiera konfigurację edytora wspólną dla obu systemów: [`.clangd`](../../.clangd),
[`.vscode/settings.json`](../../.vscode/settings.json) i
[`.vscode/extensions.json`](../../.vscode/extensions.json) (opis kluczy w
[`project-structure.md`](project-structure.md), sekcje 3.9 do 3.11, przebieg na Macu w
[`build-macos.md`](build-macos.md), sekcja 6).

Na Windowsie jest jedna istotna różnica. Plik `.clangd` wskazuje na
`build/debug/compile_commands.json`, a generator Visual Studio, którego CMake używa
domyślnie z terminala, **tego pliku nie tworzy** (sekcja 3). Skutek: po zwykłym
`cmake --preset debug` clangd nadal nie zna flag kompilacji, a edytor pokazuje czerwone
błędy "file not found" przy każdym `#include`, mimo że projekt buduje się poprawnie.

Żeby clangd działał, katalog `build\debug` musi być skonfigurowany generatorem Ninja:

```powershell
cmake --preset debug -G Ninja
cmake --build --preset debug
```

Uwagi do tych poleceń:

- Trzeba je wykonać w środowisku deweloperskim (sekcja 2), żeby kompilator MSVC (`cl.exe`) i
  Ninja dołączona do narzędzi Visual Studio były w `PATH`. Ninja używa tego `cl.exe`, który
  znajdzie w `PATH`, więc architekturę wybiera opcja `-Arch` skryptu (u mnie `amd64`).
- Jeden katalog buildu to jeden generator. Jeśli `build\debug` powstał wcześniej generatorem
  Visual Studio, trzeba go najpierw usunąć.
- Z Ninja build jest jednokonfiguracyjny, więc program leży w `build\debug\night_maze.exe`,
  bez podkatalogu `Debug`.
- Do czasu wykonania konfiguracji edytor pokazuje błędy "file not found", tak samo jak na
  Macu.

**Co zmierzyłem.** Generator Ninja sprawdziłem w osobnym katalogu, żeby nie usuwać katalogu
`build\debug` utworzonego generatorem Visual Studio:

```powershell
cmake --preset debug -G Ninja -B build\ninja-debug
cmake --build build\ninja-debug
```

Wynik: 52 kroki budowania, zero ostrzeżeń, plik `build\ninja-debug\compile_commands.json`
powstał, program leży w `build\ninja-debug\night_maze.exe` (bez podkatalogu `Debug`), a
katalog `assets` został skopiowany obok niego. Wariant z usunięciem `build\debug` różni się
tylko katalogiem, ale dla clangd potrzebny jest właśnie on, bo `.clangd` wskazuje na
`build/debug/compile_commands.json`.

**Czego nie zmierzyłem.** Czy clangd w edytorze poprawnie czyta polecenia kompilatora MSVC z
tego pliku (czyli czy błędy "file not found" znikają) i czy rozszerzenie CodeLLDB z listy
rekomendacji nadaje się do debugowania programu zbudowanego przez MSVC (jeśli nie, zostaje
debugger Visual Studio). Oba punkty są na liście kontrolnej w sekcji 11.

### CLion na Windowsie

Niesprawdzone. CLion też czyta `CMakePresets.json`. Dwie rzeczy do ustawienia:

- W Settings, Build, Execution, Deployment, Toolchains wybrać toolchain Visual Studio, a nie
  dołączony MinGW. Z MinGW kompilatorem jest GCC, więc w `CMakeLists.txt` zadziała gałąź
  `else()` z flagami `-Wall -Wextra -Wpedantic` zamiast `/W4`. To też powinno działać, ale
  jest inną konfiguracją niż ta opisana tutaj.
- Włączyć profile CMake z presetów zamiast domyślnego `cmake-build-debug`.

## 5. Ostrzeżenia: `/W4 /permissive-`

Fragment [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# Strict warnings for our own targets only (third-party code is built with its defaults).
function(night_maze_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
```

- `if(MSVC)` jest prawdą, gdy kompilatorem jest Microsoft Visual C++. Flagi MSVC mają inną
  składnię (zaczynają się od `/`), więc potrzebne jest rozgałęzienie.
- **`/W4`**: poziom ostrzeżeń 4, najwyższy rozsądny w codziennej pracy. Odpowiednik
  `-Wall -Wextra`. Istnieje też `/Wall`, ale włącza ostrzeżenia w nagłówkach systemowych i
  jest w praktyce nieużywalny.
- **`/permissive-`**: tryb ścisłej zgodności ze standardem C++. MSVC historycznie akceptował
  konstrukcje niezgodne ze standardem, a ta flaga je wyłącza. Odpowiednik `-Wpedantic` w tym
  sensie, że kod, który przejdzie na MSVC, ma większą szansę skompilować się w clang, i
  odwrotnie. To ważne w projekcie na dwa systemy.
- `PRIVATE`: flagi dotyczą tylko wskazanego targetu i nie przenoszą się na jego użytkowników.
- Funkcję wywołujemy tylko dla naszych czterech targetów: `engine`, `game_logic`,
  `night_maze` i `night_maze_tests`. GLFW, ImGui, GLAD i stb_image kompilują się ze swoimi
  domyślnymi ustawieniami. GLM i doctest nie mają własnych plików do skompilowania (same nagłówki).

Nagłówki bibliotek są oznaczone jako systemowe (`SYSTEM` w `target_include_directories`,
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` dla GLFW i GLM). Na Macu daje to `-isystem`. Na
Windowsie CMake i MSVC realizują to samo opcją `/external:I` i wyłączeniem ostrzeżeń dla
takich katalogów. Zmierzone w wygenerowanym projekcie: katalogi `external/glad/include`,
`_deps/glfw-src/include` i `_deps/glm-src` trafiają do kompilatora przez `/external:I`, a
ustawienie `ExternalWarningLevel` ma wartość `TurnOffAllWarnings`. Z tych nagłówków nie
pojawia się pod `/W4` żadne ostrzeżenie. To samo jest zmierzone dla doctest w projekcie
`night_maze_tests`: katalogi `_deps/doctest-src` i `_deps/doctest-src/doctest` trafiają do
kompilatora przez `/external:I`, choć w `Dependencies.cmake` nie ma dla nich naszego kroku
(nagłówek oznacza jako systemowy `CMakeLists.txt` samego doctest,
[`../libraries/doctest.md`](../libraries/doctest.md), sekcja 2). Dla GLM ma to największe znaczenie, bo cały kod tej
biblioteki kompiluje się wewnątrz naszych plików ([`../libraries/glm.md`](../libraries/glm.md),
sekcja 4, pułapka 14).

Standard C++20 ustawia `set(CMAKE_CXX_STANDARD 20)` razem z `CMAKE_CXX_EXTENSIONS OFF`. Na
MSVC przekłada się to na flagę `/std:c++20`.

Definicja `GL_SILENCE_DEPRECATION` na Windowsie nie ma żadnego efektu. Dotyczy tylko
nagłówków Apple. Zostaje, bo jedna lista definicji dla obu systemów jest prostsza.

Cel: **zero ostrzeżeń pod `/W4`** w naszym kodzie. MSVC zgłasza czasem inne ostrzeżenia niż
clang (na przykład o zawężających konwersjach typów), więc pierwszy build mógł ujawnić
miejsca, których Mac nie pokazał. Nie ujawnił: build Debug i build Release całego `src/`
przeszły w MSVC 19.44 bez żadnego ostrzeżenia i bez zmian w kodzie.

## 6. Okno konsoli

Program jest zdefiniowany jako:

```cmake
add_executable(night_maze
    src/main.cpp
    ...
)
```

Bez słowa `WIN32` w `add_executable` powstaje aplikacja konsolowa: punktem wejścia jest
zwykłe `int main()` (tak jak w [`src/main.cpp`](../../src/main.cpp)), a przy uruchomieniu
obok okna gry otwiera się okno konsoli.

To jest zamierzone. Do konsoli trafiają komunikaty z `core::Log`:

```text
[info] GL_VERSION:  ...
[info] GL_RENDERER: ...
```

oraz błędy z `GL_CHECK` i z callbacku błędów GLFW. Bez konsoli nie byłoby ich gdzie zobaczyć.

- Uruchomienie z terminala: komunikaty pojawiają się w tym samym terminalu (zmierzone,
  dokładne linie w sekcji 2).
- Uruchomienie dwuklikiem lub z Visual Studio: otwiera się osobne okno konsoli. Zamknięcie
  go krzyżykiem zabija program. Tego sposobu uruchamiania nie sprawdzałem.
- Jeśli program kończy się błędem przy starcie, konsola otwarta dwuklikiem zniknie od razu.
  Wtedy uruchamiamy z terminala, żeby przeczytać linię `[error] Fatal: ...`.

## 7. Katalog roboczy, `imgui.ini` i katalog `assets`

Dear ImGui zapisuje układ paneli w pliku `imgui.ini` w **katalogu roboczym (working
directory)** procesu, a nie obok pliku `.exe`.

| Sposób uruchomienia | Katalog roboczy | Gdzie powstanie `imgui.ini` |
|---|---|---|
| `build\debug\Debug\night_maze.exe` z katalogu repozytorium | katalog repozytorium | w katalogu głównym repozytorium |
| dwuklik na `night_maze.exe` | katalog z plikiem `.exe` | `build\debug\Debug\` |
| Visual Studio (F5) | ustawiany przez IDE, zwykle katalog pliku wykonywalnego | do sprawdzenia |

Ta tabela jest na Windowsie w większości przewidywaniem. Zmierzony jest jeden przypadek:
program uruchomiony z katalogiem roboczym `build\debug\Debug` i zabity po siedmiu sekundach
zostawił `imgui.ini` w tym katalogu. ImGui zapisuje plik także w trakcie działania, kilka
sekund po zmianie układu, a nie tylko przy zamykaniu. Przy wcześniejszych pomiarach program
był zabijany wcześniej i plik nie powstawał. Wiersze o dwukliku i o Visual Studio pozostają
przewidywaniem.

Bez tego pliku siedem paneli otwiera się w układzie zapisanym w kodzie (stałe
`..._PLACEMENT` w `src/debug/PanelLayout.hpp`, funkcja `placePanelOnFirstUse`, warunek
`ImGuiCond_FirstUseEver`). Ten sam warunek obejmuje trzy rzeczy: pozycję, rozmiar i to, czy
panel startuje zwinięty do paska tytułu (`ImGui::SetNextWindowCollapsed`). Zwinięty startuje
tylko panel Camera (pole `collapsed = true` w stałej `CAMERA_PLACEMENT`). Gdy plik istnieje
i ma wpis panelu, wygrywa wpis: także stan zwinięcia jest potem brany z pliku. Plik
`imgui.ini` zapisany przez program sprzed M4 ma wpisy sześciu paneli w starym układzie
(Camera pod Rendererem) i nie ma wpisu panelu Lights. Z takim plikiem sześć paneli zostaje
na starych miejscach, a tylko nowy panel Lights dostaje miejsce z kodu, czyli lewą kolumnę
pod Rendererem, gdzie w starym układzie stoi Camera: panele nachodzą na siebie. To wniosek
z kodu, nie obserwacja. Żeby obejrzeć układ domyślny, trzeba plik usunąć przed
uruchomieniem (sekcja 13.2).

Plik jest w `.gitignore`, więc nigdzie nie przeszkadza w repozytorium. Skutkiem różnych
katalogów jest tylko to, że układ paneli ustawiony przy uruchomieniu z terminala nie jest
widoczny przy uruchomieniu z IDE i odwrotnie.

Dla plików z `assets/` (shadery, modele i tekstury) katalog roboczy **nie ma
znaczenia**: program szuka ich względem pliku `.exe`, przez `core::assetPath`
([`../modules/core/paths.md`](../modules/core/paths.md)). Na Windowsie położenie programu
podaje `GetModuleFileNameW`. Zmierzone w stanie M1 (gdy program wczytywał same shadery i
rysował kostkę): program startuje bez linii `[error]` uruchomiony z katalogu repozytorium,
z katalogu roboczego `C:\` oraz z kopii katalogu `build\debug\Debug` umieszczonej w katalogu
z polskimi literami w nazwie (`...\Temp\nm-Żółw\`). Dla modeli i tekstur tych trzech prób
nie powtórzyłem (punkt otwarty w sekcji 12). PRD wymaga budowania ścieżek wyłącznie przez `std::filesystem`.

### Katalog `assets` na Windowsie: kopia, nie dowiązanie

Program oczekuje katalogu `assets` obok `night_maze.exe`, czyli w `build\debug\Debug\assets\`.
Na macOS build tworzy w tym miejscu dowiązanie symboliczne do katalogu w repozytorium. Na
Windowsie utworzenie dowiązania wymaga trybu dewelopera albo uprawnień administratora, więc
build **kopiuje** katalog: robi to target `copy_assets` poleceniem `cmake -E copy_directory`
([`project-structure.md`](project-structure.md), sekcja 3.1, blok 7).

Program czyta więc kopię, a nie pliki z repozytorium. Kopię odświeżają dwa polecenia:

| Polecenie | Co robi | Przy działającym programie |
|---|---|---|
| `cmake --build --preset debug --target copy_assets` | tylko kopiuje katalog `assets`. Nie buduje programu, bo `copy_assets` od niego nie zależy | działa (kod wyjścia 0, kopia odświeżona) |
| `cmake --build --preset debug` | buduje wszystko, a `copy_assets` należy do targetu domyślnego (`ALL`), więc kopia jest robiona od nowa przy każdym takim budowaniu, także gdy żaden plik C++ się nie zmienił | z generatorem Visual Studio **kończy się błędem** `LNK1168` (niżej) |

Reguła pracy z shaderami na Windowsie, gdy program działa:

1. zmień plik w `assets\shaders\` (także plik dołączany dyrektywą `#include`, czyli
   `assets\shaders\common\lighting.glsl`: kopiowany jest cały katalog, z podkatalogami),
2. w drugim terminalu wykonaj `cmake --build --preset debug --target copy_assets`,
3. naciśnij przycisk "Reload shaders" w panelu Shaders.

Gdy program nie działa, wystarczy zwykłe `cmake --build --preset debug` i ponowne
uruchomienie. Na macOS krok 2 nie jest potrzebny. Pominięcie go na Windowsie nie daje błędu:
panel dalej pokazuje przy każdym programie `: OK`, a obraz się nie zmienia, bo program
wczytał poprawnie starą kopię pliku.

Panel Shaders ma dziś jedną linię na program, a programów jest pięć (`basic`, `textured`,
`color`, `lit`, `gouraud`). Po udanym wczytaniu linia ma postać:

```text
basic.vert + basic.frag: OK
```

Po nieudanym przeładowaniu linia jest czerwona, a pod nią, też na czerwono, stoi komunikat
błędu (ten sam tekst trafia do konsoli jako `[error]`):

```text
lit.vert + lit.frag: FAILED, the previous program stays in use
```

Gdy program nie wczytał się ani razu (błąd już przy starcie), końcówka linii brzmi
`FAILED, there is no program to draw with`. Podpowiedź (tooltip) nad linią programu pokazuje
w dwóch wierszach pełne ścieżki obu czytanych plików, czyli kopii w
`build\debug\Debug\assets\shaders\`. Wcześniejsza wersja panelu (do 2026-10-05) miała dla
każdego programu blok kilku linii z osobnymi etykietami plików i stanu: tych napisów już
nie ma. Kod panelu: `src/debug/panels/ShadersPanel.cpp`, opis w
[`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6, a
nazwy plików w komunikatach błędów w
[`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md).

**Dlaczego nie pełny build przy działającym programie.** Wcześniejsza wersja tego dokumentu
przewidywała, że `cmake --build --preset debug` da się wykonać przy działającym programie,
bo bez zmian w C++ nic nie jest linkowane. Pomiar pokazał co innego. Z generatorem Visual
Studio taki build kończy się błędem `LINK : fatal error LNK1168` (dalej w tej linii jest
komunikat o pliku `night_maze.exe`, którego nie można otworzyć do zapisu). Windows blokuje plik `.exe`
działającego programu, a MSBuild próbuje go wtedy zlinkować od nowa, także gdy żaden plik
C++ się nie zmienił. Ten sam build przy zatrzymanym programie niczego nie linkuje (pomiar
niżej). Dlaczego MSBuild zachowuje się różnie w tych dwóch sytuacjach, nie badałem.

Z tego powodu z `CMakeLists.txt` zniknęła linia `add_dependencies(copy_assets night_maze)`.
Target `copy_assets` nie zależy teraz od programu (polecenie `copy_directory` samo tworzy
katalog docelowy), więc budowany osobno nie dotyka `night_maze.exe`.

Z generatorem Ninja pełny build przy działającym programie przeszedł: wykonał jeden krok,
`[1/1] Copying assets next to the executable`. Nie było wtedy nic do zlinkowania. Jak Ninja
zachowa się, gdy przy działającym programie zmieni się plik C++, nie mierzyłem.

**Zmierzone 2026-10-05** (generator Visual Studio, o ile nie napisano inaczej):

| Próba | Wynik |
|---|---|
| pierwszy build Debug i Release | powstają `build\debug\Debug\assets\shaders\basic.vert` i `basic.frag`, tak samo w `build\release\Release\` (stan M1, gdy shaderów było dwa). W wyjściu jest linia `Copying assets next to the executable` |
| drugi build bez żadnych zmian, program zatrzymany | linia pojawia się ponownie, program nie jest linkowany |
| zmiana shadera, `cmake --build --preset debug`, uruchomienie | kopia odświeżona, program pokazuje nowe kolory |
| zmiana shadera, `cmake --build --preset debug --target night_maze` | kopia **nie** została odświeżona |
| `cmake --build --preset debug` przy działającym programie | błąd `LNK1168` |
| `cmake --build --preset debug --target copy_assets` przy działającym programie | kod wyjścia 0, kopia odświeżona |
| `--target copy_assets` po usunięciu katalogu `assets` obok programu | katalog odtworzony |
| pełny build przy działającym programie, generator Ninja | przeszedł, jeden krok: kopiowanie |

Samego przycisku "Reload shaders" nikt przy tych próbach nie naciskał: krok 3 reguły jest na
listach kontrolnych w sekcjach 11 i 13.2 jako otwarty. Tego, że kopia obejmuje podkatalog
`shaders\common\`, osobno nie sprawdzałem. Wynika to pośrednio z pomiaru z 2026-10-05:
programy `lit` i `gouraud` dołączają `common/lighting.glsl`, a gra startuje bez linii
`[error]`.

Uwagi:

- Budowanie **samego** targetu `night_maze` (`cmake --build --preset debug --target night_maze`)
  kopii nie odświeża, bo `night_maze` nie zależy od `copy_assets` (zmierzone). Uruchomienie
  klawiszem F5 w Visual Studio może budować tylko projekt startowy. Jeśli tak jest, przed F5
  trzeba zbudować całe rozwiązanie (Build Solution) albo sam target `copy_assets`. To punkt
  do sprawdzenia z listy w sekcji 11.
- Edytowanie plików wprost w `build\debug\Debug\assets\` działa, ale zmiany przepadną przy
  następnym kopiowaniu, bo kopia zostanie nadpisana plikami z repozytorium.
- `copy_directory` nadpisuje istniejące pliki, ale nie usuwa z kopii plików, których nie ma
  już w repozytorium. Po usunięciu albo zmianie nazwy shadera warto skasować katalog
  `build\debug\Debug\assets\` i wykonać `--target copy_assets` ponownie.
- Kopię można też zrobić ręcznie, bez CMake jako systemu budowania:
  `cmake -E copy_directory assets build\debug\Debug\assets`.
- Skrót `make debug` nie zastępuje kroku 2: wykonuje pełny build, a nie sam `copy_assets`
  (i nie był na Windowsie uruchamiany, sekcja 2).

## 8. RenderDoc

RenderDoc to darmowy debugger grafiki: przechwytuje jedną klatkę i pozwala obejrzeć każde
wywołanie rysujące, stan potoku, zawartość tekstur i buforów oraz wejścia i wyjścia shaderów.
Strona projektu: <https://renderdoc.org/>.

- **Działa tylko na Windowsie (i Linuksie). Nie obsługuje macOS.** Na Macu zostają `GL_CHECK`
  i panele ImGui. To jeden z powodów, dla których projekt ma działać na obu systemach.
- Wymaga kontekstu OpenGL w profilu Core w wersji co najmniej 3.2. Nasz kontekst 4.1 Core
  spełnia ten warunek.
- Jest narzędziem zewnętrznym. Nie wymaga żadnych zmian w kodzie ani w CMake.

Typowe użycie: w RenderDoc, w zakładce Launch Application, wskazujemy
`build\debug\Debug\night_maze.exe`, ustawiamy Working Directory na katalog repozytorium,
uruchamiamy program i przechwytujemy klatkę klawiszem F12 lub PrintScreen.

Dziś w przechwyconej klatce powinny być: czyszczenie ekranu, po jednym wywołaniu
`glDrawElements` na każdą płytkę podłogi, ścianę i słupek labiryntu (dla labiryntu startowego
100 + 121 + 121, w trybie startowym programem `lit`), po jednym na każdy znacznik światła
punktowego (11 w labiryncie startowym, programem `color`), jedno z kostką (programem
`basic`) i rysowanie ImGui. Można w niej obejrzeć bufory wierzchołków i indeksów, macierze w
uniformach, bufor uniformów ze światłami (blok `LightBlock`, 928 bajtów, punkt wiązania 1),
związaną teksturę i sampler, bufor głębi oraz wejścia i wyjścia shaderów. Narzędzie stanie
się naprawdę użyteczne przy cieniach i efektach pozaekranowych.

Przechwycenia klatki z programu `night_maze` jeszcze nie sprawdzałem (punkt na liście
kontrolnej w sekcji 11).

## 9. Końce linii: `.gitattributes`

Windows tradycyjnie kończy linie parą znaków CRLF, macOS i Linux samym LF. Bez ustaleń
praca na dwóch systemach produkuje commity, w których "zmieniła się" każda linia pliku.

Cały plik [`.gitattributes`](../../.gitattributes):

```gitattributes
* text=auto
```

- `*`: reguła dotyczy wszystkich plików.
- `text=auto`: Git sam rozpoznaje, czy plik jest tekstowy. Pliki tekstowe są zapisywane w
  repozytorium zawsze z końcami LF. Pliki binarne (obrazy, modele) nie są ruszane.
- Przy pobieraniu plików do katalogu roboczego Git może na Windowsie zamienić LF na CRLF,
  zależnie od lokalnego ustawienia `core.autocrlf`. Przy commicie zamienia je z powrotem.

Efekt: w repozytorium zawsze jest LF, niezależnie od tego, na którym komputerze powstał
commit. Reguła jest w repozytorium, a nie w ustawieniach Gita na jednym komputerze, więc
działa tak samo wszędzie.

Zmierzone na Windowsie: po konfiguracji i wszystkich buildach z tego dokumentu (Debug,
Release, Ninja) `git status` nie pokazuje żadnego zmienionego pliku, a katalog `build/` jest
ignorowany.

Kompilatory na obu systemach akceptują oba rodzaje końców linii. Problem dotyczył wyłącznie
czytelności historii Gita.

## 10. Rozwiązywanie problemów

Kolumna "Stan" mówi, czy objaw widziałem na Windowsie (zmierzone), czy wiersz wynika tylko z
dokumentacji narzędzi (przewidywane).

| Objaw | Prawdopodobna przyczyna | Rozwiązanie | Stan |
|---|---|---|---|
| `'cmake' is not recognized` albo podobny komunikat powłoki | CMake z Visual Studio nie jest w `PATH` zwykłego terminala | wejdź w środowisko deweloperskie (sekcja 2) albo zainstaluj CMake osobno | zmierzone |
| Ostrzeżenie `Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE` przy konfiguracji | generator Visual Studio ignoruje `CMAKE_BUILD_TYPE` | nic, to oczekiwane (sekcja 3) | zmierzone |
| `LINK : fatal error LNK1168` przy `cmake --build --preset debug` | program `night_maze.exe` działa, a Windows blokuje jego plik | zamknij program i zbuduj ponownie. Do odświeżenia samych shaderów użyj `cmake --build --preset debug --target copy_assets` (sekcja 7) | zmierzone |
| Panele leżą jeden na drugim albo w dziwnych miejscach, na przykład panel Lights zasłania panel Camera | w katalogu roboczym jest `imgui.ini` zapisany przez starszą wersję programu albo przy innym rozmiarze okna: jego wpisy wygrywają z układem startowym z kodu | usuń `imgui.ini` z katalogu, z którego startuje program (sekcja 7), albo rozsuń panele myszą za paski tytułu | przewidywane (w stanie M1, bez pozycji startowych w kodzie, trzy panele leżały jeden na drugim: zmierzone) |
| Nie widać panelu Camera, jest tylko wąski pasek z napisem "Camera" u góry okna | panel startuje zwinięty do paska tytułu: w oknie 1280 x 720 nie ma miejsca na siedem otwartych paneli | kliknij strzałkę w pasku tytułu panelu | przewidywane |
| Podłoga albo ściany są białe, w konsoli linia `[error]` o pliku obrazu | obok `night_maze.exe` brakuje pliku z `assets\textures\` albo nie da się go zdekodować. Część modelu dostaje wtedy białą teksturę zastępczą | `cmake --build --preset debug --target copy_assets`, potem ponowne uruchomienie (pamięć podręczna nie ponawia nieudanego wczytania) | zmierzone (zrzut ekranu z celowo usuniętą teksturą, sekcja 12) |
| Brakuje podłogi, ścian albo słupków, w konsoli `[error]` z nazwą pliku `.obj` albo `.mtl` | brakuje pliku modelu albo jego pliku `.mtl`. Model, którego nie udało się wczytać, nie jest rysowany, reszta labiryntu tak | jak wyżej | przewidywane |
| Zmiana w pliku shadera nie jest widoczna po ponownym uruchomieniu | program czyta kopię obok `.exe`, a po zmianie pliku nie było kopiowania albo zbudowano tylko target `night_maze` (możliwe przy F5 w Visual Studio) | `cmake --build --preset debug --target copy_assets` albo pełny build przy zamkniętym programie, sekcja 7 | zmierzone dla `--target night_maze`, F5 przewidywane |
| `[error] Shader compilation failed: ...\assets\shaders/basic.frag` i linia sterownika. Przy błędzie już na starcie znika to, co rysuje ten program (dla `basic` kostka), przy błędzie po "Reload shaders" obraz zostaje, bo działa poprzedni program | błąd składni w pliku shadera. Mieszane ukośniki w ścieżce są poprawne (sekcja 11, klasa `gfx::Shader`). Linia sterownika zaczyna się od nazwy pliku i numeru linii, na przykład `basic.frag(4)` | popraw plik, odśwież kopię (sekcja 7) | zmierzone w stanie M1, gdy program rysował samą kostkę: w oknie zostawało wtedy samo tło i panele, a linia sterownika zaczynała się od `0(15)`. Nazwa pliku w miejscu numeru: zmierzone 2026-10-05 (sekcja 13.1) |
| Komunikat błędu shadera wskazuje `common/lighting.glsl(N)`, choć zmieniany był inny plik, albo czerwone są naraz linie `lit` i `gouraud` w panelu Shaders | błąd jest w pliku dołączanym przez `#include`. Dołączają go oba oświetlone programy, więc oba nie dają się zbudować | popraw `assets\shaders\common\lighting.glsl`, odśwież kopię (sekcja 7), "Reload shaders" | komunikat z nazwą pliku zmierzony na zrzucie ekranu (sekcja 13.1), reszta przewidywana |
| `[error] Shader include failed: ...` | plik z dyrektywy `#include` nie istnieje w kopii obok programu, dołącza sam siebie albo linia `#include` jest błędnie zapisana. Komunikat podaje plik i linię | popraw dyrektywę albo odśwież kopię (sekcja 7) | przewidywane (tekst z kodu `src/gfx/Shader.cpp`) |
| `[error] Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code` | sterownik ułożył blok uniformów inaczej niż struktura `scene::LightBlockData`, albo blok w `common/lighting.glsl` zmieniono bez zmiany struktury | porównaj blok w shaderze ze strukturą w `src/scene/LightBlock.hpp` ([`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md)) | przewidywane (na karcie NVIDIA linia się nie pojawia: zmierzone) |
| Nie widać podłogi, ścian ani słupków, są tylko tło, znaczniki świateł i kostka | oświetlony program (`lit`, a w trybie `Gouraud` program `gouraud`) nie wczytał się przy starcie, więc nie ma czym rysować labiryntu. W konsoli jest `[error]`, a linia programu w panelu Shaders kończy się napisem `FAILED, there is no program to draw with` | popraw plik shadera, odśwież kopię (sekcja 7), "Reload shaders". Do tego czasu tryb `Unlit` w panelu Renderer rysuje labirynt programem `textured` | przewidywane (z kodu `NightMazeApp::drawLitMaze`) | przewidywane |
| Konfiguracja pada przy pobieraniu GLFW, GLM lub ImGui | brak `git` w `PATH` albo brak sieci | zainstaluj git, otwórz nowy terminal, sprawdź `git --version` | przewidywane |
| Błąd o niezgodności generatora | katalog buildu utworzony innym generatorem (terminal a IDE, Visual Studio a Ninja) | usuń `build\debug` i skonfiguruj ponownie jednym narzędziem, albo użyj osobnego katalogu (`-B`, sekcja 4) | przewidywane |
| Nie ma pliku `build\debug\Debug\night_maze.exe` | użyto generatora jednokonfiguracyjnego (Ninja) | szukaj bezpośrednio w katalogu buildu, na przykład `build\debug\night_maze.exe`, patrz sekcja 4 | zmierzone (Ninja nie tworzy podkatalogu `Debug`) |
| `Fatal: Failed to create a window with an OpenGL 4.1 Core context` | sterownik bez OpenGL 4.1, sesja pulpitu zdalnego albo maszyna wirtualna | zaktualizuj sterownik karty. Przeczytaj linię `GLFW error` powyżej | przewidywane |
| `GL_RENDERER` pokazuje kartę zintegrowaną na komputerze z drugą kartą | system wybrał kartę energooszczędną | w ustawieniach grafiki Windows lub panelu sterownika przypisz `night_maze.exe` do wydajnej karty | przewidywane (na moim PC z kartami AMD Radeon i NVIDIA system sam wybrał NVIDIA) |
| FPS dużo wyższe niż odświeżanie monitora | sterownik wymusza wyłączony vsync | sprawdź ustawienie synchronizacji pionowej w panelu sterownika | przewidywane |
| Ostrzeżenia `/W4` z plików w `_deps` lub `external` | nagłówki systemowe nie zostały wyciszone | sprawdź, czy katalog trafia do kompilatora przez `/external:I` (sekcja 5), zanotuj wersje CMake i MSVC | przewidywane (u mnie nie wystąpiło) |
| Układ paneli nie zapamiętuje się | różne katalogi robocze | sekcja 7 | przewidywane |
| `[error] Shader file cannot be opened: ...\assets\shaders/basic.vert`, w oknie samo tło | obok `night_maze.exe` nie ma katalogu `assets` (program skopiowany ręcznie albo zbudowano tylko target `night_maze`, bez `copy_assets`) | `cmake --build --preset debug --target copy_assets`, sekcja 7 | przewidywane |
| Cursor lub VS Code pokazuje "file not found" przy każdym `#include`, choć build przechodzi | generator Visual Studio nie tworzy `compile_commands.json`, którego szuka `.clangd` | skonfiguruj `build\debug` generatorem Ninja, sekcja 4 | przewidywane |

## 11. Lista kontrolna pierwszego buildu na Windowsie

Do przejścia na PC przed uznaniem M1 za zamknięty na obu systemach i przed tagiem wersji.

**Ta lista powstała dla stanu M1**, w którym program rysował samą kostkę na środku okna, a
kamera latała swobodnie. Punkty `[x]` są zapisem pomiarów z tamtego stanu i zostają bez
zmian w treści: mówią o kostce na środku, dwóch liniach `[info]` i trzech panelach, bo tak
wtedy było. Po M2 + M3 program startuje w labiryncie (sekcja 2), kostka wisi nad komórką w
przeciwległym rogu, a kamera idzie za graczem. Punkty otwarte `[ ]`, które mówiły o kostce
albo o locie, są przepisane tak, żeby dało się je wykonać w obecnym programie. Sprawdzenia
samego labiryntu, gracza i nowych paneli są w sekcji 12, a oświetlenia w sekcji 13.

Po dodaniu oświetlenia (2026-10-05) zmieniły się trzy rzeczy, o których mówią punkty tej
listy. Panel Shaders ma jedną linię na program zamiast kilkuliniowego bloku. Komunikat błędu shadera zaczyna się od nazwy pliku zamiast od numeru `0`.
Panel Camera startuje zwinięty do paska tytułu. Punkty `[x]` zostają z tekstami z dnia
pomiaru i mają dopisek, punkty `[ ]` mają już teksty dzisiejszego programu.

Punkty `[x]` są zmierzone 2026-10-05 w środowisku z sekcji 1, a wynik jest zapisany przy
punkcie. Punkty `[ ]` są otwarte: nikt ich jeszcze nie wykonał. Punkt, z którego zmierzona
jest tylko część, jest rozbity na dwa. Otwarte zostały trzy grupy: to, co wymaga człowieka
przy myszy i klawiaturze, to, co wymaga środowiska Visual Studio (IDE), oraz RenderDoc,
clangd w edytorze i `make`.

**Środowisko**

- [x] `cmake --version` pokazuje co najmniej 3.24: 3.31.6-msvc6
- [x] `git --version` działa w tym samym terminalu
- [x] wersja narzędzi i kompilatora: Visual Studio Build Tools 2022 17.14.37516 (bez IDE),
      MSVC `cl` 19.44.35228, zestaw narzędzi 14.44.35207, Windows SDK 10.0.26100.0, MSBuild
      17.14.51, Windows 11 Pro 10.0.26200
- [x] `cmake` nie jest w `PATH` zwykłego terminala, potrzebne jest środowisko deweloperskie
      (`Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64`)

**Konfiguracja**

- [x] `cmake --preset debug` kończy się bez błędów. Jedno oczekiwane ostrzeżenie:
      `Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE`
- [x] w `build\debug\_deps` są katalogi `glfw`, `glm` i `imgui`. Wersję potwierdza log tylko
      dla GLM (następny punkt), wersje GLFW `3.4` i ImGui `v1.92.9b-docking` wynikają z tagów
      w `cmake/Dependencies.cmake`
- [x] w logu konfiguracji jest linia `GLM: Version 1.0.3`
- [ ] w rozwiązaniu nie ma projektu biblioteki `glm` (`GLM_BUILD_LIBRARY` jest wyłączone)
- [x] generator z `build\debug\CMakeCache.txt` (`CMAKE_GENERATOR`) dla terminala:
      `Visual Studio 17 2022`
- [ ] to samo dla "Open Folder" w Visual Studio (wymaga IDE)

**Build Debug**

- [x] `cmake --build --preset debug` kończy się bez błędów: kod wyjścia 0
- [x] **zero ostrzeżeń pod `/W4`** w plikach z `src/`: zero ostrzeżeń, zero błędów
- [x] brak ostrzeżeń pochodzących z nagłówków GLFW, GLAD i ImGui w naszych plikach: żadnego.
      Katalogi `external/glad/include` i `_deps/glfw-src/include` trafiają do kompilatora
      przez `/external:I`, `ExternalWarningLevel` to `TurnOffAllWarnings`
- [x] `src/scene/Transform.cpp` i `src/scene/Camera.cpp` to pierwsze pliki, które dołączają
      GLM (`<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`): brak ostrzeżeń z nagłówków
      GLM pod `/W4`. Katalog `_deps\glm-src` trafia do kompilatora przez `/external:I`
- [x] te same dwa pliki kompilują się pod `/W4 /permissive-` bez ostrzeżeń we własnym
      kodzie (stałe `constexpr glm::vec3`: `AXIS_X`, `AXIS_Y`, `AXIS_Z` w `Transform.cpp`,
      `static constexpr` `Camera::WORLD_UP` w `Camera.hpp`, `std::sin`, `std::cos` i
      `std::floor` na typie `float`, `std::clamp`): zero ostrzeżeń
- [x] `src/core/Paths.cpp` kompiluje się pod `/W4 /permissive-` bez ostrzeżeń (gałąź `_WIN32`
      z `<windows.h>` i `GetModuleFileNameW`, którą MSVC zobaczył pierwszy raz): zero
      ostrzeżeń, także o konwersji typów i o ponownej definicji `NOMINMAX` albo
      `WIN32_LEAN_AND_MEAN`
- [x] program jest w `build\debug\Debug\night_maze.exe`

**Uruchomienie**

- [x] okno otwiera się, tło jest ciemnogranatowe (sprawdzone na zrzucie ekranu)
- [ ] okno ma rozmiar 1280 x 720 i tytuł "Night Maze" (nie zapisałem przy pomiarze)
- [x] na środku okna widać kostkę z trzema ścianami w jednolitych kolorach: czerwoną z
      przodu, niebieską z lewej, turkusową u góry. Żadna ściana nie "prześwituje" przez
      inną (test głębi działa). Sprawdzone na zrzucie ekranu
- [ ] zmiana rozmiaru okna myszą (szersze, węższe, wyższe niż szersze): płytki podłogi
      zostają kwadratowe, obraz się nie rozciąga
- [ ] minimalizacja okna i przywrócenie: program nie kończy pracy, w konsoli nie ma linii
      `[error]` ani komunikatu o asercji, obraz wraca. Zapisać, jaki rozmiar framebuffera
      pokazuje panel Renderer zaraz po przywróceniu (na Windowsie zminimalizowane okno ma
      framebuffer 0 x 0, a `NightMazeApp::onRender` pomija wtedy rysowanie: sprawdza
      szerokość i wysokość)
- [x] przy uruchomieniu z terminala są w nim dokładnie dwie linie `[info]`
- [ ] przy uruchomieniu dwuklikiem otwiera się osobne okno konsoli z liniami `[info]`
      (dwie o sterowniku i linie `Loaded ...` pamięci podręcznej assetów)
- [x] dokładny napis `GL_VERSION` i `GL_RENDERER`: `4.1.0 NVIDIA 610.74` oraz
      `NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2`. Komputer ma też zintegrowaną kartę AMD
      Radeon, system sam wybrał NVIDIA
- [x] w konsoli nie ma linii `[error]`
- [x] panele "Renderer", "Shaders" i "Camera" są widoczne. Przy pierwszym uruchomieniu (bez
      `imgui.ini`) leżą jeden na drugim (stan M1. Dziś paneli jest siedem, mają miejsca
      startowe i się nie zasłaniają, sekcja 13)
- [ ] FPS i czas klatki w panelu "Renderer" się aktualizują
- [ ] linie "Framebuffer" i "Window" pokazują te same wartości (na Windowsie powinny być równe)

**Sterowanie i interfejs**

- [ ] klawisz `~` (na lewo od `1`) ukrywa i pokazuje panele
- [ ] Esc przy widocznym kursorze zamyka program, kod wyjścia 0
- [ ] podczas wpisywania wartości w polu `Clear color` (Ctrl i kliknięcie) Esc anuluje tylko
  edycję i nie zamyka programu, a `~` nie chowa paneli
- [ ] dopóki nikt nie kliknął w scenę, kursor myszy jest widoczny, a Esc zamyka program
  jednym naciśnięciem
- [ ] krzyżyk okna zamyka program bez błędów w konsoli
- [ ] docking: panel "Renderer" daje się przeciągnąć i zadokować do krawędzi okna, środek
      pozostaje przezroczysty
- [ ] edytor "Clear color" zmienia kolor tła na żywo
- [ ] po ponownym uruchomieniu układ paneli jest zapamiętany (zapisać, gdzie powstał
      `imgui.ini`). Przy pomiarach program był zatrzymywany przez zabicie procesu, więc plik
      nie powstał

**Zmiana rozmiaru**

- [ ] zmiana rozmiaru okna: obraz wypełnia całe okno, wartości w panelu się zmieniają
- [ ] maksymalizacja i przywrócenie okna działają
- [ ] minimalizacja i przywrócenie nie powodują błędów ani zawieszenia
- [ ] przeciąganie okna między monitorami o różnym skalowaniu (jeśli są dostępne).
      Oczekiwane: obraz sceny poprawny, a panele zostają w skali monitora, na którym
      program wystartował (skala jest czytana raz, [`../modules/debug-ui.md`](../modules/debug-ui.md),
      sekcja 5.8.4)

**Build Release**

- [x] `cmake --preset release` i `cmake --build --preset release` bez błędów i ostrzeżeń:
      zero ostrzeżeń
- [x] `build\release\Release\night_maze.exe` uruchamia się i zachowuje tak samo: te same dwie
      linie `[info]`, kostka widoczna (bez części ręcznej)

**Ścieżki do assetów (`core::executableDir`, `core::assetPath`)**

Opis: [`../modules/core/paths.md`](../modules/core/paths.md) i sekcja 7 tego dokumentu.
Funkcje są wołane przy każdym starcie programu (wczytywanie shaderów).

- [ ] ćwiczenie 1 z `paths.md` (tymczasowe `core::logInfo` w `main`): `executableDir()`
      wypisuje katalog pliku `.exe` (z generatorem Visual Studio `build\debug\Debug`) i nie
      zmienia się przy uruchomieniu z innego katalogu roboczego. Wycofać zmianę. Samego
      ćwiczenia nie robiłem. Pośrednio potwierdzają to punkty niżej: start z `C:\` oraz
      ścieżka `...\build\debug\Debug\assets\shaders/basic.frag` w komunikacie błędu shadera
- [x] po buildzie katalog `build\debug\Debug\assets\shaders\` istnieje i zawiera `basic.vert`
      oraz `basic.frag` (to samo dla `build\release\Release\`). W wyjściu buildu jest linia
      `Copying assets next to the executable`
- [x] drugi build bez żadnych zmian (program zatrzymany): linia `Copying assets next to the
      executable` pojawia się ponownie, a program nie jest linkowany
- [x] zmiana koloru w `assets\shaders\basic.frag`, potem `cmake --build --preset debug` i
      uruchomienie: program pokazuje nowe kolory (odwrócone kolory sprawdzone na zrzucie
      ekranu). Zmiana wycofana
- [x] to samo, ale z `cmake --build --preset debug --target night_maze`: kopia nie została
      odświeżona, program nie widzi zmiany (zgodnie z oczekiwaniem)
- [x] `cmake --build --preset debug` przy działającym programie (generator Visual Studio):
      **nie przechodzi**, `LINK : fatal error LNK1168`, także bez zmian w C++. Wcześniejsze
      oczekiwanie (build bez linkowania) było błędne
- [x] `cmake --build --preset debug --target copy_assets` przy działającym programie: kod
      wyjścia 0, kopia odświeżona
- [x] `--target copy_assets` po usunięciu katalogu `assets` obok programu: katalog odtworzony
- [ ] Visual Studio: zmiana w shaderze, potem samo F5. Zapisać, czy kopia została
      odświeżona (czyli czy F5 buduje też `copy_assets`), i jeśli nie, czy pomaga Build
      Solution (wymaga IDE)
- [x] program startuje bez linii `[error]` i pokazuje kostkę uruchomiony z katalogu
      repozytorium i z innego katalogu roboczego (`C:\`)
- [ ] to samo przy uruchomieniu dwuklikiem i z Visual Studio (F5)
- [x] program startuje i pokazuje kostkę z katalogu, którego ścieżka zawiera polskie litery:
      kopia `build\debug\Debug` w `...\Temp\nm-Żółw\`, bez linii `[error]`

**Klasa `gfx::Shader`**

Opis: [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md). Na Windowsie klasa
jest skompilowana i sprawdzona przy starcie programu (poprawne pliki i błąd składni).

- [x] `src/gfx/Shader.cpp` i `src/core/Paths.cpp` kompilują się w MSVC z `/W4 /permissive-`
      bez ostrzeżeń (w szczególności `core::pathText` w `Paths.cpp`: `std::string` budowany z
      iteratorów `std::u8string`): zero ostrzeżeń
- [x] celowy błąd składni (usunięty średnik w `basic.frag`, potem `cmake --build --preset
      debug` i uruchomienie): w konsoli jest **jeden** wpis `[error]`, okno pokazuje samo tło
      i panele, program się nie zamyka. Plik przywrócony. Dokładny tekst:

  ```text
  [error] Shader compilation failed: <repo>\build\debug\Debug\assets\shaders/basic.frag
  0(15) : error C0000: syntax error, unexpected '}', expecting ',' or ';' at token "}"
  ```

  Druga linia to format sterownika NVIDII: numer napisu źródłowego 0 i numer linii 15 w
  nawiasie. Ścieżka ma mieszane ukośniki: wsteczne z katalogu programu i zwykły przed
  `basic.frag`, bo nazwa względna `shaders/basic.frag` jest w kodzie zapisana z `/`.
  Windows przyjmuje oba. To zapis ze stanu M1 (2026-10-05). Dziś dwie rzeczy są inne: w
  miejscu numeru `0` stoi nazwa pliku (zmierzone 2026-10-05 dla innego błędu jako
  `basic.frag(4)`, sekcja 13.1), a przy błędzie w `basic.frag` znika sama kostka, bo
  labirynt rysują inne programy
- [ ] ścieżka z polską literą w komunikacie błędu nie zamyka
      programu (w konsoli litera może być wyświetlona błędnie, to dopuszczalne)

**Panel Shaders i przeładowanie shaderów**

Opis: [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6. Na
Windowsie panel jest skompilowany i wyświetla się poprawnie. Przycisku "Reload shaders"
nikt tam jeszcze nie nacisnął. Teksty linii panelu w punktach otwartych są dzisiejsze
(sekcja 7). Te same kroki dla oświetlonych programów i pliku `common/lighting.glsl` są w
sekcji 13.2.

- [x] `src/debug/panels/ShadersPanel.cpp`, `src/debug/DebugContext.hpp` i `src/main.cpp`
      kompilują się w MSVC z `/W4 /permissive-` bez ostrzeżeń (w szczególności stała
      `ERROR_TEXT_COLOR`, dziś `inline constexpr ImVec4` w `src/debug/Theme.hpp`, i
      inicjalizatory desygnowane `DebugContext` z polem `.shader` w kolejności deklaracji):
      zero ostrzeżeń
- [x] panel "Shaders" jest widoczny i pokazuje oba pliki programu `basic` oraz poprawne
      wczytanie (stan M1, 2026-10-05, z jednym programem i ówczesnymi napisami. Dziś
      panel pokazuje pięć linii postaci `basic.vert + basic.frag: OK`, sekcje 7 i 13.2)
- [ ] panel "Shaders" daje się zadokować
- [ ] podpowiedź nad linią `basic.vert + basic.frag: OK` pokazuje w dwóch wierszach pełne
      ścieżki obu plików, czyli kopii w `build\debug\Debug\assets\shaders\`. Oczekiwane są
      mieszane ukośniki, tak jak w komunikacie błędu wyżej: `...\assets\shaders/basic.frag`
      (podpowiedź i komunikat powstają z tej samej ścieżki). Wcześniejsze oczekiwanie "same
      ukośniki wsteczne" było najpewniej błędne. Samej podpowiedzi nie oglądałem
- [ ] przeładowanie udane: przy działającym programie zmiana koloru w
      `assets\shaders\basic.frag`, `cmake --build --preset debug --target copy_assets` w
      drugim terminalu, potem "Reload shaders". Zapisać, czy kostka zmienia kolor
      (oczekiwane: tak; kostkę widać z góry w trybie noclip, sekcja 12). Przycisk przeładowuje
      wszystkie pięć programów naraz. Samo polecenie przy działającym programie jest już
      zmierzone (kod wyjścia 0, kopia odświeżona), otwarte zostaje naciśnięcie przycisku
- [ ] to samo bez kopiowania: "Reload shaders" zaraz po zapisaniu pliku. Oczekiwane:
      linia `basic.vert + basic.frag: OK` i obraz bez zmian (program czyta kopię)
- [ ] przeładowanie nieudane: usunięty średnik w `basic.frag`,
      `cmake --build --preset debug --target copy_assets`, "Reload shaders". Oczekiwane:
      czerwona linia `basic.vert + basic.frag: FAILED, the previous program stays in use`,
      pod nią czerwony tekst ze ścieżką pliku i dziennikiem sterownika, ten sam tekst w
      konsoli jako `[error]`, cztery pozostałe linie z `: OK`, kostka bez zmian. Linia
      sterownika powinna zaczynać się od nazwy pliku i numeru linii, `basic.frag(N)`
- [ ] naprawa: przywrócony plik, `cmake --build --preset debug --target copy_assets`,
      "Reload shaders". Oczekiwane: linia wraca do `basic.vert + basic.frag: OK`, pierwotne
      kolory
- [ ] po kilku przeładowaniach w konsoli nie ma żadnej linii `[error] GL_...` (backend ImGui
      i usunięty stary program, `shader-hot-reload.md`, sekcja 6.3)
- [ ] ścieżka z polską literą (kopia katalogu programu jak w punkcie o `Żółw` wyżej):
      podpowiedź nad linią programu pokazuje ścieżkę bez zamknięcia programu, a polskie
      litery są wyświetlone poprawnie (sama linia pokazuje dziś tylko nazwy plików). Wcześniejsze oczekiwanie "znak zastępczy" dotyczyło domyślnej czcionki
      ImGui i jest nieaktualne: panele mają czcionkę Atkinson Hyperlegible z polskimi
      literami. Zmierzone jest tylko to, że czcionka je rysuje: tymczasowy napis `Zażółć
      gęślą jaźń` z kompletem małych i wielkich liter oraz tymczasowa podpowiedź z napisem
      `Żółw` wyglądają poprawnie na zrzucie ekranu. Sam start z takiego katalogu też jest
      zmierzony. Otwarte zostaje obejrzenie w panelu **prawdziwej** ścieżki z polską literą,
      czyli całej drogi przez `core::pathText`

**Kamera: sterowanie i panel Camera**

Opis: [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md),
sekcje 5 i 6, [`../modules/game/player.md`](../modules/game/player.md) (ruch gracza),
[`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5. Na Windowsie
kod jest skompilowany, a panel pokazuje wartości startowe. Sterowanie nie było tam jeszcze
sprawdzane. Windows jest jedyną z dwóch platform, na której GLFW włącza surowy ruch myszy
(`GLFW_RAW_MOUSE_MOTION`), więc obrót myszą działa tu inną ścieżką niż na Macu.

Od M4 panel Camera startuje zwinięty do paska tytułu (o ile `imgui.ini` nie ma jego wpisu).
Przed punktami, które każą coś w nim odczytać albo przesunąć, trzeba go rozwinąć
kliknięciem strzałki w pasku tytułu (sekcja 13.2).

- [x] `src/game/NightMazeApp.cpp`, `src/debug/panels/CameraPanel.cpp`,
      `src/debug/DebugUI.cpp` i `src/main.cpp` kompilują się w MSVC z `/W4 /permissive-` bez
      ostrzeżeń (w szczególności `static_cast<float>` z `double` przy `mouseDeltaX`, `fixedDt`
      i `alpha`, `glm::mix` z trzecim argumentem `float`, domyślny inicjalizator pola z
      pozycją sprzed ostatniego kroku, `io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse`
      i pola `DebugContext` w kolejności deklaracji): zero ostrzeżeń. Po M2 + M3 to pole
      nazywa się `m_previousPlayerPosition`, a build nadal nie daje ostrzeżeń (sekcja 12)
- [x] stan M1: panel "Camera" był widoczny i pokazywał pozycję kamery 0, 0, 3, `Yaw` 0,
      `Pitch` 0, `FOV` 60. Wartości startowe po M2 + M3 są w sekcji 12
- [ ] panel "Camera" daje się zadokować
- [ ] kliknięcie lewym przyciskiem w scenę chowa kursor, a kursor nie wyjeżdża poza okno
      (także na drugi monitor)
- [ ] pierwsza klatka po kliknięciu: obraz nie szarpie (kamera nie odskakuje)
- [ ] ruch myszy w prawo obraca kamerę w prawo (ściany uciekają w lewo), ruch do góry podnosi
      wzrok. `Pitch` zatrzymuje się na 89 i na -89
- [ ] obrót jest płynny i ten sam ruch ręki daje ten sam obrót niezależnie od szybkości ruchu
      (surowy ruch myszy, bez przyspieszenia systemowego). Zapisać, czy czułość domyślna
      0,1 jest wygodna, czy wymaga innej wartości niż na Macu
- [ ] długi obrót w jedną stronę (kilka pełnych obrotów): kamera kręci się bez zatrzymania,
      `Yaw` zawija się przez 360
- [ ] W, S, A, D przesuwają gracza zgodnie z opisem (chodzenie w poziomie, lewy Shift to
      sprint, w trybie noclip spacja i lewy Shift to góra i dół), ruch po skosie (W i D) nie
      jest szybszy, klawisze przeciwne (W i S) się znoszą. Szczegółowe kroki: sekcja 12
- [ ] przy widocznym kursorze klawisze ruchu nie przesuwają gracza
- [ ] pierwszy Esc przy przechwyconym kursorze oddaje kursor (pojawia się w miejscu, w którym
      zniknął) i nie zamyka programu, drugi Esc zamyka program
- [ ] po Esc, **bez ruszania myszą**, kliknięcie w scenę znowu przechwytuje kursor i żaden
      panel nie reaguje na to kliknięcie. Zapisać wynik także dla sytuacji, w której przed
      Esc mysz była długo przesuwana w stronę zadokowanego panelu (to samo sprawdzić na
      Macu: po zwolnieniu kursora ImGui może do pierwszego ruchu myszy pamiętać ostatnią
      pozycję ukrytego kursora)
- [ ] suwaki panelu Camera (na przykład `FOV`) działają, a ich przeciąganie nie obraca kamery
      i nie chowa kursora, także gdy kursor wyjedzie przy przeciąganiu nad scenę
- [ ] kliknięcie w panel (pasek tytułu, suwak) nie chowa kursora
- [ ] przy przechwyconym kursorze panele nie reagują na mysz: kręcenie myszą tak, żeby ukryty
      kursor "przeszedł" nad zadokowanym panelem, nie zatrzymuje obrotu, nie podświetla
      widżetów i nie zmienia żadnej wartości, także przy klikaniu i przy trzymaniu przycisku
- [ ] Alt+Tab przy przechwyconym kursorze: kursor jest widoczny w innym programie. Po
      powrocie do okna zapisać, czy kursor jest znowu schowany i czy kamera nie odskoczyła
- [ ] ruch jest płynny na monitorze o odświeżaniu innym niż 60 Hz (na przykład 144 Hz): chód
      bokiem (D) wzdłuż ściany przy `Walk speed` 20 nie szarpie. Zapisać FPS z panelu Renderer
- [ ] to samo przy wyłączonym vsync w panelu sterownika (kilkaset FPS i więcej, większość
      klatek bez kroku symulacji): ruch nadal płynny, prędkość chodu i czułość myszy takie
      same
- [ ] `Near plane` przesuwany w górę wycina najbliższe ściany, `Far plane` przesuwany w dół
      obcina koniec korytarza. Zapisać wartości, przy których to widać z pozycji startowej
- [ ] minimalizacja okna przy przechwyconym kursorze i przywrócenie: bez linii `[error]` i
      bez asercji
- [ ] w konsoli przez cały test ręczny nie ma linii `[error]`

**Klasy `gfx::Buffer` i `gfx::VertexArray`**

Opis: [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) (klasy) i
[`../modules/gfx/indexed-drawing.md`](../modules/gfx/indexed-drawing.md) (kostka w `NightMazeApp`). Na
Windowsie klasy są skompilowane i rysują kostkę.

- [x] `src/gfx/Buffer.cpp` i `src/gfx/VertexArray.cpp` kompilują się w MSVC z
      `/W4 /permissive-` bez ostrzeżeń (w szczególności `reinterpret_cast<const void*>` z
      `std::size_t` w `setFloatAttribute` i `static_cast<GLsizeiptr>` w konstruktorze
      `Buffer`): zero ostrzeżeń
- [x] `src/game/NightMazeApp.cpp` kompiluje się bez ostrzeżeń (stałe `VERTEX_STRIDE` i
      `COLOR_OFFSET` liczone z `sizeof(float)`, tablice `VERTICES` i `INDICES`, rozmiar
      `INDICES.size() * sizeof(GLuint)`, dzielenie `static_cast<float>` przy proporcjach,
      `nullptr` jako ostatni argument `glDrawElements`): zero ostrzeżeń
- [x] `src/gfx/Shader.cpp` i `Shader.hpp` kompilują się bez ostrzeżeń z nagłówkami GLM
      (`<glm/glm.hpp>` w nagłówku, `<glm/gtc/type_ptr.hpp>` i `glm::value_ptr` w `setMat4`):
      zero ostrzeżeń
- [x] kostka jest widoczna i w konsoli nie ma linii `[error]`, także żadnej
      `GL_INVALID_OPERATION after glDrawElements` ani po `glUniformMatrix4fv` (build Debug,
      w którym `GL_CHECK` jest aktywne)

**Testy jednostkowe i kod bez okna (M2 + M3)**

Opis: [`../libraries/doctest.md`](../libraries/doctest.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md),
[`../modules/game/maze-generator.md`](../modules/game/maze-generator.md). Ten kod powstał na
Windowsie, więc wszystkie punkty poza ostatnimi dwoma są zmierzone przy jego pisaniu.

- [x] `cmake --preset debug` pobiera doctest: w `build\debug\_deps` są katalogi
      `doctest-src`, `doctest-build` i `doctest-subbuild`, plik
      `doctest-src\scripts\version.txt` zawiera `2.5.3`
- [x] czysty build (`--clean-first`) presetów `debug` i `release`: kod wyjścia 0, zero
      ostrzeżeń pod `/W4 /permissive-`, także w `src/scene/Collider.*`, `src/game/Maze*` i w
      pięciu plikach `tests/`
- [x] nagłówek doctest nie daje ostrzeżeń: katalogi `_deps/doctest-src` i
      `_deps/doctest-src/doctest` trafiają do kompilatora przez `/external:I`,
      `ExternalWarningLevel` to `TurnOffAllWarnings`
- [x] powstają `build\debug\Debug\game_logic.lib` i `build\debug\Debug\night_maze_tests.exe`
      (to samo w `build\release\Release\`)
- [x] `ctest --test-dir build/debug -C Debug --output-on-failure`: `100% tests passed, 0 tests
      failed out of 1`, kod wyjścia 0. To samo dla `build/release` i `-C Release`
- [x] `ctest` bez `-C`: test nie jest uruchamiany (`***Not Run`, kod wyjścia 8)
- [x] program testowy uruchomiony wprost, Debug i Release: wszystkie przypadki testowe tego
      etapu (kolizje i labirynt) przechodzą, `Status: SUCCESS!`. Liczby dla całego programu
      testowego po M2 + M3 są w sekcji 12, a dzisiejsze w sekcji 13.1
- [x] labirynt wzorcowy (4 na 4, ziarno 1) jest ten sam w Debug i w Release i zgadza się z
      niezależnym skryptem w Pythonie
- [x] program `night_maze.exe` nadal się buduje w obu konfiguracjach (nie uruchamiałem go po
      tej zmianie: nowy kod nie jest jeszcze wołany przez grę)
- [x] generator Ninja (`cmake --build build\ninja-debug`): build przechodzi, testy przechodzą
      (`ctest --test-dir build/ninja-debug`, tu bez `-C`)
- [x] clang-format 19.1.5: `--dry-run --Werror` na plikach z `src/` i `tests/` nie zgłasza
      różnic
- [x] clang-tidy 19.1.5 z bazy poleceń `build\ninja-debug`: żadnej diagnostyki w nowych
      plikach i w testach. Jedna w starszym pliku: `modernize-return-braced-init-list` w
      `src/core/Paths.cpp` (gałąź Windows, linia `return std::filesystem::path(buffer);`).
      Poprawiona w kroku łączącym M2 + M3: linia brzmi teraz `return {buffer};`
- [x] diagnostyki kompilatora clang dla nowych plików i testów, jako zastępstwo za build na
      Macu: `clang-tidy --checks=-*,clang-diagnostic-*,readability-identifier-naming
      --extra-arg=/clang:-Wpedantic -p build\ninja-debug` na czterech nowych plikach `.cpp` z
      `src/` i pięciu z `tests/`: żadnej diagnostyki. Sprawdzenie, że polecenie w ogóle coś
      widzi: po tymczasowym dopisaniu nieużywanej zmiennej i porównania `int` z `unsigned`
      zgłasza `clang-diagnostic-unused-variable` i `clang-diagnostic-sign-compare` (zmiana
      wycofana). Ograniczenie: to clang 19 w trybie zgodności z MSVC i z biblioteką
      standardową MSVC, a nie Apple clang z libc++. Samej flagi `-Wpedantic` osobną próbą nie
      sprawdzałem
- [x] loader OBJ i siatka (temat 4, 2026-10-05): `src/assets/ObjLoader.*`,
      `src/gfx/Vertex.hpp`, `src/gfx/Mesh.*` i `tests/ObjLoaderTests.cpp` budują się bez
      ostrzeżeń pod `/W4 /permissive-` w konfiguracji Debug, generatorem Ninja i generatorem
      Visual Studio (osobne katalogi buildu). `night_maze_tests.exe
      --source-file=*ObjLoaderTests*`: wtedy 18 przypadków testowych i 804 asercje, dziś, po
      mapach normalnych, 20 przypadków i 1576 asercji, `Status: SUCCESS!`. Konfiguracji Release dla tych plików wtedy nie budowałem (po M2 + M3 jest
      zbudowana i przetestowana, sekcja 12). `gfx::Mesh` był wtedy tylko skompilowany, bez
      użytkownika. Dziś tworzą go `assets::AssetCache` i `game::ColliderLines`, testu
      jednostkowego nadal nie ma ([`../modules/gfx/mesh.md`](../modules/gfx/mesh.md))
- [x] clang-tidy 19.1.5 z bazy poleceń buildu Ninja na `src/gfx/Mesh.cpp`,
      `src/assets/ObjLoader.cpp` i `tests/ObjLoaderTests.cpp`: żadnej diagnostyki z regułami
      projektu (`.clang-tidy`) i żadnej z samymi diagnostykami kompilatora clang
      (`--checks=-*,clang-diagnostic-*,readability-identifier-naming` z `-Wall -Wextra
      -Wpedantic`). Ograniczenie to samo co wyżej: clang 19 z biblioteką standardową MSVC,
      nie Apple clang z libc++. clang-format `--dry-run --Werror` na sześciu nowych plikach
      nie zgłasza różnic
- [x] stb_image (temat 5, 2026-10-05): `cmake --preset debug` pobiera repozytorium stb w
      commicie `2c980bb59875b0d32144a71867fbdebb2f77cd20` do `_deps\stb-src` (12 MB, pełny
      klon, bo bez `GIT_SHALLOW`), pierwsza linia `stb_image.h` to `stb_image - v2.30`.
      Target `stb_image` buduje się z jednego pliku `external\stb\stb_image.c` bez
      ostrzeżeń, generatorem Ninja i generatorem Visual Studio. W poleceniu kompilacji
      tego pliku nie ma `/W4`, a katalog `_deps\stb-src` trafia do kompilacji
      `ImageLoader.cpp` przez `/external:I` z wyłączonymi ostrzeżeniami. `-DSTBI_NO_STDIO`
      jest w poleceniach obu plików
      ([`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2)
- [x] loader obrazów, tekstura i settery uniformów (temat 5, 2026-10-05):
      `src/assets/ImageLoader.*`, `src/gfx/Texture2D.*`, `src/gfx/Shader.*` (`setInt`,
      `setVec3`) i `tests/ImageLoaderTests.cpp` budują się bez ostrzeżeń pod
      `/W4 /permissive-` w konfiguracji Debug, generatorem Ninja i generatorem Visual
      Studio (osobne katalogi buildu). `night_maze_tests.exe
      --source-file=*ImageLoaderTests*`: wtedy 7 przypadków testowych i 35 asercji, dziś, po
      mapach normalnych, 9 przypadków i 57 asercji, `Status: SUCCESS!`. Cały program testowy razem z testami loadera OBJ też przechodził (liczby po
      M2 + M3: sekcja 12). Konfiguracji Release dla tych plików wtedy nie budowałem. Test nazwy pliku ze
      znakami spoza ASCII (polskie litery i znak japoński) przechodzi przy stronie kodowej
      systemu 1250 ([`../modules/assets/images.md`](../modules/assets/images.md),
      sekcja 5.7)
- [x] clang-tidy 19.1.5 z regułami projektu (`.clang-tidy`, baza poleceń buildu Ninja) na
      `src/assets/ImageLoader.cpp`, `src/gfx/Texture2D.cpp`, `src/gfx/Shader.cpp` i
      `tests/ImageLoaderTests.cpp`: żadnej diagnostyki. clang-format `--dry-run --Werror` na
      siedmiu plikach tego kroku nie zgłasza różnic. Samych diagnostyk kompilatora clang
      (`-Wall -Wextra -Wpedantic`) osobną próbą nie sprawdzałem
- [x] `gfx::Texture2D` sprawdzona programem z ukrytym oknem, poza repozytorium (karta
      NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74, `GL_VERSION` równe `4.1.0 NVIDIA
      610.74`): tworzenie, mipmapy do poziomu 1 x 1, orientacja (pierwszy piksel danych w
      lewym dolnym rogu), wyrównanie wierszy, trzy filtry, anizotropia od 1 do 16,
      przenoszenie, złe argumenty, `glGetError` czysty. Rozszerzenie
      `GL_EXT_texture_filter_anisotropic` jest na liście sterownika (404 rozszerzenia).
      Pełne tabele: [`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 5.9
- [x] zmierzona osobliwość tego komputera: poziom anizotropii ustawiony na obiekcie
      tekstury (`glTexParameterf`) nie daje błędu, odczytuje się jako 1 i nie zmienia
      obrazu, a ustawiony na obiekcie samplera (`glSamplerParameterf`) działa. Dlatego
      `Texture2D` trzyma filtr, zawijanie i anizotropię w obiekcie samplera. Przyczyny nie
      ustaliłem ([`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcje 2.8 i
      5.9)
- [x] tekstury w oknie gry: po kroku łączącym M2 + M3 `Texture2D` ma użytkownika
      (`assets::AssetCache`), a labirynt jest oteksturowany. Filtry i anizotropia na ścianach
      są sprawdzone na zrzutach ekranu, ręczne klikanie widżetów panelu Assets jest otwarte
      (sekcja 12). Pokaz tematu 5 z PRD (podgląd tekstur) niesie panel "Assets". Przełącznik
      map normalnych z PRD doszedł razem z mapami w M4 (sekcje 13.3 i 13.4)
- [x] diagnostyka w `Paths.cpp` poprawiona (`return {buffer};`), clang-tidy na zmienionych
      plikach nie zgłasza niczego (sekcja 12). Na Macu ta gałąź nie jest kompilowana
- [ ] te same testy na macOS: dopiero to porównanie mierzy, że oba systemy generują ten sam
      labirynt (lista w [`build-macos.md`](build-macos.md), sekcja 2, "Testy jednostkowe")

**Git i narzędzia**

- [x] po skonfigurowaniu i zbudowaniu (Debug, Release, Ninja) `git status` nie pokazuje
      zmienionych plików (końce linii)
- [x] `build/` nie pojawia się w `git status`
- [ ] `.vs/` i `imgui.ini` nie pojawiają się w `git status` (żaden z nich przy pomiarach nie
      powstał: nie było IDE, a program był zatrzymywany przez zabicie procesu)
- [ ] Visual Studio: "Open Folder", konfiguracja z presetów, F5 uruchamia `night_maze.exe`
      (wymaga IDE)
- [ ] RenderDoc: przechwycenie jednej klatki działa (opcjonalnie)
- [x] generator Ninja: `cmake --preset debug -G Ninja -B build\ninja-debug` i
      `cmake --build build\ninja-debug` przechodzą (52 kroki, zero ostrzeżeń), powstaje
      `build\ninja-debug\compile_commands.json`, program leży w
      `build\ninja-debug\night_maze.exe`, katalog `assets` jest skopiowany obok niego
- [ ] Cursor lub VS Code z clangd: po `cmake --preset debug -G Ninja` w czystym `build\debug`
      błędy "file not found" w edytorze znikają (zapisać wynik, opcjonalnie)
- [ ] rozszerzenie CodeLLDB debuguje program zbudowany przez MSVC (zapisać wynik,
      opcjonalnie)
- [ ] `Makefile` w Git Bash albo innej powłoce z `make` (na moim PC nie ma `make`, plik nie
      był uruchamiany)

## 12. Lista kontrolna M2 + M3: labirynt, gracz, tekstury, panele

Krok, który łączy kolizje, labirynt, loadery, siatkę i tekstury w działającą grę. Opis kodu:
[`../modules/game/player.md`](../modules/game/player.md),
[`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md),
[`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md) (linie pudełek i panel
Collision), [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) (panel
Maze), [`../modules/gfx/textures.md`](../modules/gfx/textures.md) (shadery `textured`).

Kamień milowy **nie jest zamknięty**: część ręczna poniżej jest otwarta, na macOS kod nie
był budowany ([`build-macos.md`](build-macos.md)) i nie ma tagu.

**Ta sekcja jest zapisem stanu z 2026-10-05**, sprzed oświetlenia. Program miał wtedy o jeden
panel mniej (bez Lights), o dwa programy shaderów mniej (bez `lit` i `gouraud`), panel
Shaders z kilkoma liniami na program, równo jasny labirynt i mniej testów. Punkty `[x]`
są pomiarami z tamtego dnia i opisują tamten stan. Dzisiejsze liczby (163 i
62220), siedem paneli, pięć programów i mapy normalnych opisuje sekcja 13. Punkty otwarte `[ ]` w sekcji
12.2 są przepisane tak, żeby dało się je wykonać w dzisiejszym programie.

### 12.1. Zmierzone (2026-10-05)

Środowisko: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74.

- [x] build Debug i Release generatorem Visual Studio oraz build generatorem Ninja: kod
      wyjścia 0, zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: wszystkie ówczesne przypadki przechodzą,
      `Status: SUCCESS!` (2026-10-05. Dzisiejsze liczby przypadków i asercji są w sekcji
      13.1). `ctest` dla obu konfiguracji: `100% tests passed, 0 tests failed out of 1`
- [x] clang-tidy na plikach zmienionych w tym kroku: żadnej diagnostyki
- [x] gra startuje bez linii `[error]`, w tym bez żadnej linii z nazwą błędu OpenGL
      (`GL_INVALID_...`), w buildzie Debug, w którym `GL_CHECK` jest aktywne
- [x] zrzut ekranu ze startu: widok z wnętrza labiryntu, tekstury kamienia na podłodze,
      ścianach i słupkach stoją prosto i nie są odbite lustrzanie (wtedy równo jasne, bez
      oświetlenia. Dzisiejszy widok startowy: sekcja 13.1)
- [x] zrzut ekranu z góry w trybie noclip: układ ścian zgadza się z planem w panelu Maze, a
      żółte linie pudełek kolizji leżą na ścianach i słupkach
- [x] zrzuty ekranu obu widoków debug shadera `textured`: normalne jako kolor i współrzędne
      UV jako kolor
- [x] zrzuty ekranu porównania filtrów na ścianie widzianej pod płaskim kątem: najbliższy
      sąsiad, dwuliniowy, trójliniowy i trójliniowy z anizotropią 16x
- [x] podglądy tekstur w panelu Assets stoją prosto (nie do góry nogami)
- [x] brak pliku tekstury: powierzchnia rysuje się z białą teksturą zastępczą (w kolorze `Kd`
      materiału), w konsoli jest jedna linia `[error]`

Stany z czterech ostatnich punktów i widok z góry zostały osiągnięte tymczasowymi wstawkami
w kodzie, które są już usunięte, a nie kliknięciami w panelach. Dlatego te same widżety są
jeszcze raz na liście otwartej.

Motyw paneli, czcionka i układ startowy (zmiana po M2 + M3, zmierzone 2026-10-05 na tym samym
PC, ekran 1920 x 1080 przy skali 100%, opis w [`../modules/debug-ui.md`](../modules/debug-ui.md),
sekcje 5.7 i 5.8):

- [x] build Debug i Release generatorem Visual Studio oraz build generatorem Ninja z plikami
      `src/debug/Theme.*` i `src/debug/PanelLayout.*`: kod wyjścia 0, zero ostrzeżeń pod
      `/W4 /permissive-`. Wszystkie ówczesne przypadki testowe przechodzą w Debug
      (2026-10-05), `ctest` przechodzi w Debug i w Release
- [x] clang-format i clang-tidy na plikach `src/debug/`: żadnej diagnostyki. Osobny
      przebieg clang-tidy z samymi diagnostykami kompilatora clang i flagami
      `-Wall -Wextra -Wpedantic` też nic nie zgłasza (kontrola: celowo dopisana nieużywana
      zmienna była w tym przebiegu zgłaszana)
- [x] katalog `assets\fonts` (czcionka, `OFL.txt`, `README.md`) trafia do kopii obok
      `night_maze.exe` bez zmian w CMake: `copy_assets` kopiuje cały katalog `assets`
- [x] okno 1280 x 720 po usunięciu `imgui.ini`: ówczesne panele (bez Lights) się nie zasłaniają i żaden nie
      wychodzi poza okno. Renderer i Camera stoją przy lewej krawędzi, Maze i Assets przy
      prawej, Collision i Shaders przy dolnej, między kolumnami. Panele Renderer, Camera,
      Maze, Collision i Shaders pokazują całą zawartość bez przewijania, panel Assets się
      przewija. Środek górnej części okna jest wolny i widać w nim scenę (układ z
      2026-10-05. Dziś paneli jest siedem, pod Rendererem stoi Lights, a Camera jest
      zwinięty u góry: sekcja 13.2)
- [x] większe okno w pierwszej klatce (1560 x 860 i 1700 x 940, ustawione tymczasową zmianą
      rozmiaru startowego): prawa kolumna stoi przy prawej krawędzi, dolny rząd przy dolnej
- [x] polskie litery w czcionce paneli: tymczasowy napis z kompletem liter i tymczasowa
      podpowiedź wyglądają poprawnie
- [x] tekst błędu shadera w panelu Shaders (zepsuty `color.frag` w kopii `assets`) i wpis
      `Failed to load` w panelu Assets (zmieniona nazwa tekstury) są czytelne, także na tle
      białych ścian
- [x] brak pliku czcionki i plik, który nie jest czcionką: jedna linia `[error]`, panele w
      czcionce wbudowanej, program działa
- [x] skala 150% symulowana mnożnikiem w kodzie: tekst i odstępy rosną, tekst jest ostry. W
      oknie 1280 x 720 panele się wtedy nie zasłaniają, ale ich zawartość się nie mieści
      (paski przewijania, ucięte etykiety)

Tu także stany były ustawiane tymczasowymi wstawkami, już usuniętymi. Stany "pod kursorem" i
"wciśnięty" były rysowane przez podstawienie koloru, a nie przez najechanie myszą.

Obserwacja, nie pomiar wydajności: na starcie panel Renderer pokazywał około 1500 FPS w
buildzie Debug, z synchronizacją pionową taką, jaką ustawił sterownik. Nie wyciągam z tej
liczby żadnych wniosków: nie wiem, czy vsync był aktywny, a pomiar był jeden.

### 12.2. Otwarte: test ręczny na około dziesięć minut

Tych kroków nikt jeszcze nie wykonał ręką: chodzenia i ślizgania prawdziwymi klawiszami,
klawisza N, obrotu myszą w labiryncie, przycisków `Regenerate` i `Random seed`, przycisku
`Reload shaders` (dziś z pięcioma programami) oraz klikania list i suwaka w panelu Assets.
Przy każdym kroku jest to, co powinno być widać. Oczekiwania wynikają z kodu i z testów
jednostkowych, nie z obserwacji.

Lista powstała przed oświetleniem. W dzisiejszym programie scena jest nocna, więc dwie
rady ułatwiają jej przejście: lista `Lighting` w panelu Renderer ustawiona na `Unlit` daje
równo jasny labirynt z tamtego dnia (wygodny do oglądania tekstur, filtrów i kolizji), a
panel Camera trzeba najpierw rozwinąć strzałką w pasku tytułu. Kroki samego oświetlenia są
w sekcji 13.2.

Przygotowanie:

- [ ] zamknąć program, usunąć `imgui.ini` z katalogu, z którego program będzie uruchamiany
      (przy starcie z katalogu repozytorium: z katalogu głównego repozytorium), zbudować
      (`cmake --build --preset debug`) i uruchomić `build\debug\Debug\night_maze.exe`

Start i układ paneli:

- [x] okno 1280 x 720: ówczesne panele (bez Lights) nie zasłaniają się nawzajem. Renderer i Camera stoją w
      kolumnie przy lewej krawędzi, Maze i Assets przy prawej, Collision i Shaders na dole
      między kolumnami. Środek górnej części okna jest wolny. Zawartość ówczesnego panelu Shaders
      mieściła się w nim bez przewijania, dopóki żaden program nie ma błędu (zmierzone na
      zrzucie ekranu 2026-10-05, sekcja 12.1. To układ i panel Shaders z tamtego dnia:
      dzisiejszy układ siedmiu paneli jest punktem otwartym w sekcji 13.2)
- [ ] po prawdziwym usunięciu `imgui.ini` ręką i starcie z katalogu repozytorium (układ
      siedmiu paneli z sekcji 13.2): obejrzeć na żywo, czy tekst jest wygodny do czytania z
      odległości (projektor) i czy najechanie myszą na suwak, przycisk i pole wyboru
      zmienia ich tło na ciepły brąz, a panel z fokusem ma morski pasek tytułu
- [ ] panel Camera (rozwinąć strzałką w pasku tytułu): `Mode: walking`, `Player feet` 1, 0, 1, `Eye: 1.00, 1.70, 1.00`,
      `Yaw` 180 (labirynt startowy: 10 na 10, ziarno 1), `Pitch` 0, `Walk speed` 3.0,
      `Sprint speed` 5.5, `Fly speed` 6.0. Kąt 180 dla ziarna 1 podał autor kodu z
      uruchomienia, żaden test go nie przypina: testy sprawdzają tylko, że kamera patrzy w
      stronę bez ściany
- [ ] panel Maze: `Width` 10, `Height` 10, `Seed` 1, linie `In play: 10 x 10 cells, seed 1` i
      `Walls: 121, pillars: 121`, pod nimi plan z bursztynową kropką w lewym górnym rogu i
      kreską skierowaną w dół planu (południe). Plan z kropką w tym miejscu jest widoczny na
      zrzucie ekranu, wartości suwaków i linii tekstu też
- [ ] panel Collision: `Wall boxes: 121`, `Pillar boxes: 121`, `All boxes: 242`,
      `Wall box: 0.30 m thick (the visible wall: 0.20 m)`, pudełko gracza `min: 0.70, 0.00,
      0.70` i `max: 1.30, 1.80, 1.30`
- [ ] panel Assets: trzy modele (`floor_tile.obj`, `wall_straight.obj`, `wall_pillar.obj`),
      każdy z jedną częścią, nazwą pliku tekstury i linią `normal map:` z nazwą mapy
      normalnych, cztery tekstury z podglądem (`floor_stone.png`, `wall_stone.png` i ich
      mapy `floor_stone_normal.png`, `wall_stone_normal.png`), brak sekcji `Failed to load`

Chodzenie i kolizje (kliknąć w scenę, kursor znika):

- [ ] W idzie tam, gdzie patrzy kamera, ale zawsze poziomo: z wzrokiem wbitym w podłogę
      prędkość jest ta sama, a `Eye` ma stale y równe 1.70. Lewy Shift przyspiesza
- [ ] dojście do ściany na wprost: gracz staje, obraz nie drży, ściana nie jest przycięta
      przez bliską płaszczyznę
- [ ] ślizganie: ustawić się ukosem do ściany i trzymać W. Gracz sunie wzdłuż ściany,
      zamiast stanąć
- [ ] ślizganie wzdłuż ściany obok słupków: iść przytulonym do długiej prostej ściany przez
      kilka komórek. Gracz **nie może** zahaczać o słupki stojące co 2 m (pudełko ściany ma
      grubość słupka, 0,3 m)
- [ ] róg: wejść ukosem w narożnik wewnętrzny (gracz staje w rogu) i obejść narożnik
      zewnętrzny (gracz zsuwa się po nim bez zacięcia)
- [ ] nie da się wyjść poza labirynt: obejść kawałek ściany zewnętrznej, pchając w nią
- [ ] kropka na planie w panelu Maze porusza się razem z graczem, a jej kreska obraca się
      razem z kamerą
- [ ] Esc oddaje kursor, klawisze ruchu przestają działać, gracz staje w miejscu

Noclip:

- [ ] klawisz N (działa także przy widocznym kursorze): `Mode: noclip (free flight)`, w
      panelu Collision pole `Noclip (key N)` jest zaznaczone
- [ ] w trybie noclip (kursor przechwycony) spacja wznosi, lewy Shift opuszcza, W leci tam,
      gdzie patrzy kamera, także w górę i w dół, przez ściany
- [ ] wzlecieć spacją ponad ściany i spojrzeć w dół: układ ścian zgadza się z planem w panelu
      Maze (północ, czyli -Z, jest na górze planu). Nad komórką w rogu przeciwległym do
      startu wisi kolorowa kostka
- [ ] drugi raz N w powietrzu: gracz od razu stoi na podłodze (y stóp równe 0), bez
      widocznego zjazdu w dół. Jeśli wylądował w ścianie, może z niej wyjść
- [ ] to samo polem wyboru `Noclip (key N)` w panelu Collision zamiast klawisza

Pudełka kolizji:

- [ ] `Draw collision boxes`: żółte linie obrysowują każdą ścianę i każdy słupek, linie nie
      migoczą (pudełka są rysowane o 1 cm większe). Pudełka ścian są wyraźnie grubsze od
      korpusu ściany i równe ze słupkami. Linie za ścianą są zasłonięte
- [ ] zielone pudełko gracza: kamera stoi w jego środku, więc widać je po spojrzeniu pod
      nogi albo nad głowę, a w całości z boku nie widać go nigdy. Zapisać, jak to wygląda

Regeneracja:

- [ ] zmienić `Width` i `Height` (suwaki od 2 do 40) oraz `Seed`: nic się nie dzieje, linia
      `In play` pokazuje stary labirynt. Dopiero `Regenerate` buduje nowy: zmienia się plan,
      linia `In play` i liczby ścian, gracz stoi znowu w `Player feet` 1, 0, 1 z `Pitch` 0 i
      patrzy w otwarty korytarz, kostka wisi nad nowym rogiem
- [ ] `Regenerate` nie zmienia trybu noclip, prędkości, trybu widoku ani pola `Draw collision
      boxes`: ustawić je przed kliknięciem i sprawdzić po nim
- [ ] ten sam rozmiar i to samo ziarno dwa razy dają ten sam plan
- [ ] `Random seed`: w polu `Seed` pojawia się nowa liczba i od razu powstaje nowy labirynt.
      Wpisanie tej liczby później i `Regenerate` odtwarza go
- [ ] labirynt 40 na 40: zapisać FPS z panelu Renderer (każdy obiekt to osobne wywołanie
      rysujące)

Panel Assets (stanąć tak, żeby widzieć długi korytarz i podłogę pod płaskim kątem. W
nocnej scenie daleki koniec korytarza jest ciemny, więc do porównania filtrów najpierw
ustawić `Lighting` w panelu Renderer na `Unlit`):

- [ ] `View mode`, `Normals as colour`: podłoga jasnozielona (normalna +Y), powierzchnie zwrócone
      na +X czerwonawe, na +Z niebieskawe, a zwrócone w przeciwne strony ciemne w tym
      kanale, więc dwie strony tej samej ściany mają różne kolory. `UVs as colour`: czerwono-zielone przejścia, które zaczynają się od nowa tam,
      gdzie tekstura się powtarza. `Textured` przywraca obraz. Kostka i linie pudełek nie
      zmieniają wyglądu. Oba widoki rysuje zawsze program `textured`, bez świateł (sekcja
      13.2). Opisane kolory normalnych to kolory podstawowe powierzchni: przy zaznaczonym
      polu `Normal mapping` i trybie `Lighting` innym niż `Gouraud` widać na nich jeszcze
      rysunek fug z map normalnych (sekcja 13.4). Żeby zobaczyć same normalne modelu,
      odznaczyć `Normal mapping`
- [ ] `Filter`, `Nearest`: z bliska widać kwadratowe teksele, w oddali obraz ziarni się i
      migocze przy ruchu. `Bilinear`: z bliska gładko, w oddali nadal migocze. `Trilinear`
      (ustawienie startowe): w oddali spokojnie, ale rozmyte
- [ ] `Anisotropy`: suwak od 1x do maksimum sterownika (na tym PC 16x). Przy `Trilinear`
      przesunięcie w prawo wyostrza podłogę i ściany widziane pod płaskim kątem w oddali
- [ ] podglądy tekstur w panelu nie reagują na filtr ani na anizotropię (rysuje je ImGui
      własnym samplerem) i stoją prosto
- [ ] najechanie myszą na nazwę pliku pokazuje pełną ścieżkę

Shadery i brakujący plik:

- [ ] `Reload shaders` po zmianie w `textured.frag`: najpierw ustawić `Lighting` w panelu
      Renderer na `Unlit`, bo w pozostałych trybach labirynt rysują programy `lit` albo
      `gouraud` i zmiana w `textured.frag` nie byłaby widoczna. Przy działającym programie
      zmienić w `assets\shaders\textured.frag` linię `fragColor = vec4(texel * uTint, 1.0);`
      na `fragColor = vec4(texel * uTint * vec3(1.0, 0.5, 0.5), 1.0);`, w drugim terminalu
      `cmake --build --preset debug --target copy_assets`, potem kliknąć `Reload shaders`.
      Oczekiwane: labirynt robi się czerwonawy, wszystkie pięć linii panelu kończy się
      napisem `: OK`
- [ ] błąd w jednym programie (nadal w trybie `Unlit`): usunąć średnik w `textured.frag`,
      `copy_assets`, `Reload shaders`. Oczekiwane: czerwona linia
      `textured.vert + textured.frag: FAILED, the previous program stays in use`, pod nią
      czerwony tekst ze ścieżką pliku i linią sterownika zaczynającą się od
      `textured.frag(N)`, labirynt rysuje się poprzednią wersją, cztery pozostałe linie
      mają `: OK`. Przywrócić plik (`git checkout assets/shaders/textured.frag`),
      `copy_assets`, `Reload shaders`
- [ ] celowo brakująca tekstura: zamknąć program, zmienić nazwę
      `build\debug\Debug\assets\textures\floor_stone.png` (kopii, nie pliku w repozytorium),
      uruchomić. Oczekiwane: w trybie `Unlit` podłoga jest biała (`Kd` materiału to biel), w
      trybach z oświetleniem nie ma rysunku kamienia i ma kolor padającego na nią
      światła (biała tekstura zastępcza razy światło). Mapa normalnych podłogi wczytała się
      niezależnie, więc w trybach `Phong` i `Blinn-Phong` na białej podłodze nadal widać
      relief fug. Ściany bez zmian,
      w konsoli jedna linia `[error]`, w panelu Assets przy części `floor_stone` napis
      `no texture (white)` i sekcja `Failed to load` z nazwą pliku na czerwono. Przywrócić
      nazwę pliku
- [ ] start z innego katalogu roboczego (`C:\`) i z katalogu ze znakami spoza ASCII w
      ścieżce: modele i tekstury wczytują się tak samo (w stanie M1 sprawdzone tylko dla
      shaderów)

Okno:

- [ ] zmiana rozmiaru okna myszą: obraz wypełnia okno, płytki podłogi zostają kwadratowe.
      Zapisać, co dzieje się z panelami przy prawej krawędzi, gdy okno robi się węższe
- [ ] okno zmaksymalizowane **po starcie**: układ startowy jest liczony raz, w pierwszej
      klatce, z rozmiaru okna w tej chwili (1280 x 720), więc po maksymalizacji panele
      zostają w lewej górnej części okna, w tych samych miejscach co w małym oknie.
      Zapisać, jak to wygląda. Panele liczone od rogów większego okna widać dopiero wtedy,
      gdy okno jest duże już w pierwszej klatce (zmierzone tymczasową zmianą rozmiaru
      startowego, sekcja 12.1)
- [ ] ekran ze skalą 150% (Ustawienia, Ekran, Skala): tekst paneli jest 1,5 raza większy i
      ostry. W oknie 1280 x 720 zawartość paneli się nie mieści, po powiększeniu okna do
      1920 x 1080 i usunięciu `imgui.ini` układ wygląda jak przy 100%. Niesprawdzone na
      prawdziwym ekranie: ten PC ma skalę 100%
- [ ] minimalizacja i przywrócenie w trakcie chodzenia: bez linii `[error]` i bez asercji
- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo

## 13. Lista kontrolna M4: oświetlenie i mapy normalnych

Sekcje 13.1 i 13.2 to pierwsza część M4 (oświetlenie), sekcje 13.3 i 13.4 to druga część
(mapy normalnych). Pierwsza część kamienia milowego M4: trzy rodzaje świateł (księżyc, latarka gracza, światła
punktowe w ślepych zaułkach), cztery tryby cieniowania labiryntu, blok uniformów ze
światłami, dyrektywa `#include` w shaderach, panel Lights i układ siedmiu paneli. Opis kodu:
[`../modules/scene/lights.md`](../modules/scene/lights.md) (rodzaje świateł, model odbicia,
plik `common/lighting.glsl`, panel Lights),
[`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md)
(cieniowanie na wierzchołek i na fragment, Phong i Blinn-Phong, przełącznik trybu),
[`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) (blok `LightBlock`,
układ `std140`), [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md)
(`#include`, nazwy plików w błędach),
[`../modules/game/flashlight.md`](../modules/game/flashlight.md) (latarka, klawisz F,
światła w ślepych zaułkach). Decyzje:
[`../decisions/no-gamma-until-m7.md`](../decisions/no-gamma-until-m7.md) i
[`../decisions/dead-end-lights.md`](../decisions/dead-end-lights.md).

Kamień milowy **nie jest zamknięty**: kod obu części jest kompletny, ale części ręczne
poniżej (13.2 i 13.4) są otwarte, na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)) i
nie ma tagu. Czego w tej części nie ma: cieni (światła świecą przez ściany, cienie są w
planie M7), korekcji gamma i tekstur sRGB (M7) oraz baterii latarki (M5).

### 13.1. Zmierzone (2026-10-05)

Środowisko: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74.

- [x] build Debug i Release: zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: wszystkie przypadki przechodzą. Liczby
      podaję dla stanu dzisiejszego, po obu częściach M4: 163 przypadki testowe i 62220
      asercji (sekcja 13.3). Przypadki w plikach (policzone także jako makra `TEST_CASE`):
      `ColliderTests.cpp` 12, `ImageLoaderTests.cpp` 9, `LightingTests.cpp` 17,
      `LightTests.cpp` 20, `MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12,
      `MazeTests.cpp` 6, `MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 20, `PlayerTests.cpp`
      13, `ShaderSourceTests.cpp` 22, `TangentTests.cpp` 9, `TransformTests.cpp` 4. Z
      oświetleniem doszły cztery pliki (`ShaderSourceTests.cpp`, `LightTests.cpp`,
      `LightingTests.cpp`, `TransformTests.cpp`), z mapami normalnych piąty
      (`TangentTests.cpp`)
- [x] clang-format i clang-tidy: żadnej uwagi
- [x] clang-tidy wymagał jednej zmiany konfiguracji: linii
      `ExtraArgs: ['-D_CRT_USE_BUILTIN_OFFSETOF']` w `.clang-tidy` (ten sam przełącznik jest
      w `.clangd`, w `CompileFlags: Add`). Powód: `src/scene/LightBlock.hpp` sprawdza układ
      bloku świateł liniami `static_assert(offsetof(...) == ...)`. Na Windowsie clang-tidy
      czyta nagłówki biblioteki C Microsoftu, w których makro `offsetof` jest zapisane
      rzutowaniem wskaźnika. MSVC przyjmuje taki zapis wewnątrz `static_assert`, clang nie.
      Makro `_CRT_USE_BUILTIN_OFFSETOF` przełącza w tych nagłówkach `offsetof` na wersję
      wbudowaną w kompilator. Nie da się go ustawić w `CMakeLists.txt`: MSVC odmawia
      definiowania tej nazwy (ostrzeżenie C4117, nazwa zastrzeżona). Opis:
      [`project-structure.md`](project-structure.md), sekcje 3.6 i 3.9
- [x] gra startuje bez linii `[error]`, w tym bez żadnej z nazwą błędu OpenGL (`GL_...`), w
      buildzie Debug, w którym `GL_CHECK` jest aktywne. Z tego wynika, że pięć programów
      shaderów się wczytało (także `lit` i `gouraud`, które dołączają
      `common/lighting.glsl`) i że nie pojawiła się linia `[error] Uniform block LightBlock
      is ... bytes in the shader, but 928 bytes in the C++ code`: sterownik NVIDII układa
      blok w tylu bajtach, ile ma struktura `scene::LightBlockData`
- [x] zrzut ekranu ze startu: nocna scena w trybie Blinn-Phong
- [x] zrzuty ekranu czterech trybów oświetlenia (`Unlit`, `Gouraud`, `Phong`,
      `Blinn-Phong`), każdy z trzech punktów widzenia
- [x] zrzut ekranu z wyłączoną latarką
- [x] zrzut ekranu ślepego zaułka z jego światłem punktowym
- [x] zrzut ekranu ścian oświetlonych przez księżyc i ścian, do których jego światło nie
      dociera. Które to strony, wynika z kodu: przy kątach startowych (`Moon yaw` 25,
      `Moon pitch` -50) światło biegnie w kierunku około (0,27, -0,77, -0,58), więc
      oświetla podłogę i te strony ścian, które patrzą w stronę -X i +Z, a strony patrzące
      w stronę +X i -Z dostają od księżyca zero i świecą tylko światłem otoczenia
- [x] błąd wewnątrz dołączanego pliku jest pokazany z nazwą tego pliku, a obraz rysuje
      dalej poprzedni program (zrzut ekranu). Surowa linia sterownika NVIDII ma w miejscu
      nazwy numer napisu źródłowego:

  ```text
  1(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
  ```

  a w panelu Shaders i w konsoli stoi:

  ```text
  common/lighting.glsl(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
  ```

  Numer 63 to linia w pliku `common/lighting.glsl`, a nie w tekście po wklejeniu: pilnują
  tego dyrektywy `#line`, które program dopisuje wokół dołączonego pliku. Z kodu
  (`gfx::nameSourceFiles`) wynika też, że komunikat shadera z więcej niż jednym plikiem
  kończy się linią legendy. W chwili tego pomiaru `lit.frag` dołączał jeden plik i legenda
  miała postać `Source files: 0 = lit.frag, 1 = common/lighting.glsl`. Dziś `lit.frag`
  dołącza też `common/normal_map.glsl`, więc legenda ma trzy pozycje: tego komunikatu po
  zmianie nikt nie wywołał (punkt otwarty w sekcji 13.4)
- [x] shader bez `#include` też dostaje nazwę pliku w komunikacie: `basic.frag(4)` w
      miejscu `0(4)`

Żaden z tych stanów nie był ustawiany ręką: klawiszem F, listą `Lighting`, widżetami panelu
Lights ani przyciskiem `Reload shaders`. Dlatego te same kroki są jeszcze raz na liście
otwartej. Format błędów sterownika Apple (`ERROR: 1:15:`) jest obsłużony w kodzie i
sprawdzony tylko testem jednostkowym, nie na prawdziwym sterowniku.

### 13.2. Otwarte: test ręczny na około dziesięć minut

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze zrzutów z sekcji 13.1,
nie z klikania. Nazwy widżetów są zapisane tak jak w `src/debug/panels/LightsPanel.cpp` i
`RendererPanel.cpp`. Dokładną wartość suwaka wpisuje się po kliknięciu go z wciśniętym Ctrl.

Przygotowanie:

- [ ] zamknąć program, usunąć `imgui.ini` z katalogu, z którego program będzie uruchamiany
      (przy starcie z katalogu repozytorium: z katalogu głównego repozytorium), zbudować
      (`cmake --build --preset debug`) i uruchomić `build\debug\Debug\night_maze.exe`

Układ siedmiu paneli (okno 1280 x 720, bez `imgui.ini`):

- [ ] lewa kolumna: Renderer (336 x 230) nad Lights (336 x 466). Prawa kolumna: Maze
      (300 x 480) nad Assets (300 x 216). Dolny rząd między kolumnami: Collision
      (312 x 272) i Shaders (292 x 272). Siódmy panel, Camera, jest zwinięty do paska
      tytułu u góry okna, tuż na prawo od lewej kolumny. Żaden panel nie zasłania innego,
      a środek okna, w który świeci latarka, jest wolny
- [ ] panel Renderer: pod edytorem `Clear color` jest lista `Lighting` z wybraną pozycją
      `Blinn-Phong`
- [ ] panel Lights: u góry edytor koloru `Ambient`, pod nim cztery grupy z paskami:
      `Moon (directional)` (zwinięta), `Flashlight (spot)`, `Point lights (dead ends)` i
      `Highlight (specular)` (rozwinięte). Przy zwiniętej grupie księżyca panel pokazuje
      całą zawartość bez przewijania. Wartości startowe: `Flashlight on (key F)`
      zaznaczone, `Beam intensity` 1.60, `Cone` z polami `inner 13.0 deg` i
      `outer 21.0 deg`, `Beam range` 16.0 m, linia `In this maze: 11 (at most 16)`,
      `Point intensity` 2.00, `Point radius` 3.0 m, `Strength` 0.25, `Shininess` 32
- [ ] panel Shaders: przycisk `Reload shaders` i pięć linii:
      `basic.vert + basic.frag: OK`, `textured.vert + textured.frag: OK`,
      `color.vert + color.frag: OK`, `lit.vert + lit.frag: OK`,
      `gouraud.vert + gouraud.frag: OK`. Najechanie myszą na linię pokazuje w dwóch
      wierszach pełne ścieżki obu plików
- [ ] rozwinięcie panelu Camera: kliknąć strzałkę w jego pasku tytułu. Panel otwiera się w
      dół do rozmiaru 280 x 424 i kończy się 8 pikseli nad dolnym rzędem. Zasłania lewą
      część sceny i żadnego innego panelu. Jest trochę niższy od swojej zawartości, więc
      ma pasek przewijania. Drugie kliknięcie strzałki zwija go z powrotem. Po ponownym
      uruchomieniu (już z `imgui.ini`) panel jest w tym stanie, w jakim został

Latarka:

- [ ] klawisz F wyłącza latarkę: ciepła plama na środku obrazu znika, zostają księżyc,
      światła punktowe i światło otoczenia, a pole `Flashlight on (key F)` w panelu Lights
      samo się odznacza. Drugi raz F włącza ją i zaznacza pole. Klawisz działa także przy
      widocznym kursorze (tak jak N), ale nie wtedy, gdy klawiaturę ma panel (na przykład
      trwa wpisywanie wartości w polu)
- [ ] to samo kliknięciem pola `Flashlight on (key F)` zamiast klawisza
- [ ] stożek zostaje na środku obrazu w ruchu: iść do przodu (W), bokiem (A i D) i biec
      (lewy Shift) wzdłuż ściany. Plama latarki nie spóźnia się za obrazem i nie drży.
      Wynika to z kodu (latarka jest stawiana w tym samym punkcie, z którego liczony jest
      widok klatki), nikt tego nie oglądał w ruchu

Cztery tryby (lista `Lighting` w panelu Renderer):

- [ ] `Unlit`: labirynt równo jasny, jak przed M4, sama tekstura. Znaczników świateł nie
      ma, a klawisz F niczego nie zmienia w obrazie
- [ ] `Gouraud`: światło liczone w wierzchołkach i rozciągane po trójkątach. Powierzchnie
      mają łagodne przejścia jasności od narożnika do narożnika, bez okrągłych plam.
      Znaczniki świateł są widoczne
- [ ] `Phong`: światło liczone dla każdego fragmentu. Latarka daje okrągłą plamę z miękkim
      brzegiem, światła punktowe okrągłe kałuże światła na podłodze i ścianach
- [ ] `Blinn-Phong` (tryb startowy): to samo co `Phong`, inny jest tylko wzór połysku
      (porównanie niżej)

Gouraud a Phong:

- [ ] na ścianie: stanąć około 2 m przed ścianą, twarzą do niej, i przełączać `Phong` i
      `Gouraud`. W `Phong` na ścianie jest okrągła plama latarki. W `Gouraud` plama znika
      albo rozmazuje się wzdłuż krawędzi trójkątów: duża ściana boczna segmentu (2 m na
      2,6 m) ma wierzchołki tylko w czterech narożnikach, a światło, które pada między
      nie, nie trafia w żaden wierzchołek. Przesuwać wzrok powoli w stronę narożnika
      ściany: w `Gouraud` jasność pojawia się dopiero wtedy, gdy stożek obejmie
      wierzchołek, i rozchodzi się od niego po trójkątach
- [ ] u podstawy słupka: skierować latarkę na podłogę przy słupku i przełączać tryby. W
      `Phong` plama jest okrągła na podłodze i na słupku. W `Gouraud` podłoga rozjaśnia się
      trójkątnymi klinami od narożnika płytki (płytka podłogi 2 m na 2 m ma cztery
      wierzchołki, a jej narożniki leżą pod słupkami), za to cokół słupka, który ma
      wierzchołki blisko siebie, wygląda podobnie w obu trybach. To oczekiwanie z
      geometrii modeli, nie obserwacja: zapisać, co widać naprawdę

Phong a Blinn-Phong (w panelu Lights ustawić `Strength` 1.0 i `Shininess` 16):

- [ ] twarzą do ściany, latarka na wprost: przełączać `Phong` i `Blinn-Phong`. W
      `Blinn-Phong` jasna plama połysku na środku jest szersza i jaśniejsza niż w `Phong`
      przy tym samym wykładniku
- [ ] pod płaskim kątem do światła punktowego albo do księżyca: stanąć tak, żeby patrzeć
      wzdłuż ściany albo podłogi, ze światłem daleko z przodu. W `Blinn-Phong` połysk
      rozciąga się w podłużną smugę, w `Phong` jest mniejszy albo urywa się (wzór Phonga
      daje zero, gdy między promieniem odbitym a kierunkiem do oka jest więcej niż 90
      stopni). Po próbie przywrócić `Strength` 0.25 i `Shininess` 32

Księżyc (w panelu Lights rozwinąć grupę `Moon (directional)`, panel zaczyna się wtedy
przewijać):

- [ ] wartości startowe: `Moon yaw` 25 deg, `Moon pitch` -50 deg, `Moon intensity` 0.30.
      Wyłączyć latarkę (F), żeby widzieć samo światło księżyca. Strony ścian patrzące w
      stronę -X i +Z są jaśniejsze, strony patrzące w stronę +X i -Z ciemne (tylko światło
      otoczenia). Na planie w panelu Maze północ to -Z, czyli góra planu, a +X to prawa
      strona
- [ ] `Moon yaw` (suwak od 0 do 360): kąt mówi, w którą stronę światło biegnie. Przy 205
      (o 180 więcej niż na starcie) jasne i ciemne strony ścian zamieniają się miejscami.
      Przy 90 światło biegnie w stronę +X: ze stron ścian jasne są tylko te, które patrzą w
      stronę -X
- [ ] `Moon pitch` (suwak od -90 do -5): przy -90 światło pada prosto w dół, podłoga jest
      najjaśniejsza, a żadna pionowa strona ściany nie dostaje światła księżyca. Przy -5
      światło ledwie muska podłogę, a ściany zwrócone do księżyca są najjaśniejsze
- [ ] `Moon intensity` 0 wyłącza księżyc. Cieni nie ma: księżyc oświetla także podłogę i
      ściany, które stoją za inną ścianą

Światła punktowe:

- [ ] policzyć znaczniki: klawisz N, wznieść się spacją nad ściany i spojrzeć w dół.
      Turkusowych kostek (bok 0,14 m, 1,4 m nad podłogą) jest 11, tyle, ile pokazuje linia
      `In this maze: 11 (at most 16)`. Każda wisi nad środkiem komórki, która na planie w
      panelu Maze ma ściany z trzech stron (ślepy zaułek). Komórka startowa, w lewym
      górnym rogu planu, znacznika nie ma, nawet jeśli jest ślepym zaułkiem
- [ ] `Point radius` (suwak od 0.5 do 12.0 m): większy promień powiększa kałuże światła.
      Cieni nie ma, więc przy dużym promieniu światło widać także w sąsiednich korytarzach,
      za ścianą. `Point intensity` 0 gasi światła, znaczniki zostają
- [ ] `Point colour` zmienia naraz kolor świateł i kolor znaczników

Stożek i zasięg latarki:

- [ ] `Cone`: dwa pola przeciągane myszą, `inner` i `outer`, w stopniach od osi stożka (od
      1 do 60). Większe `outer` poszerza plamę. `inner` bliskie `outer` daje ostry brzeg,
      `inner` dużo mniejsze od `outer` szeroki, miękki brzeg. Pola `inner` nie da się
      przeciągnąć powyżej `outer`
- [ ] `Beam range` (suwak od 2.0 do 60.0 m): mała wartość sprawia, że latarka oświetla
      tylko najbliższe ściany, duża rozjaśnia koniec długiego korytarza

Regeneracja:

- [ ] w panelu Maze ustawić inne ziarno (albo kliknąć `Random seed`) i `Regenerate`:
      znaczniki i światła stoją w ślepych zaułkach nowego labiryntu, a liczba w linii
      `In this maze: ...` odpowiada nowemu planowi. Ustawienia z panelu Lights i tryb
      `Lighting` zostają bez zmian
- [ ] `Width` 4, `Height` 4, `Seed` 1, `Regenerate`: linia `In this maze: 2 (at most 16)`,
      znaczniki nad komórką w prawej kolumnie w drugim rzędzie od góry i nad komórką w
      lewym dolnym rogu planu (pilnuje tego test `golden maze: 4 x 4 cells from seed 1 has
      lights in its two dead ends`)
- [ ] `Width` 40, `Height` 40, `Regenerate`: liczba świateł nie przekracza 16 (oczekiwane
      dokładnie 16, bo tak duży labirynt ma więcej ślepych zaułków), a znaczniki są
      rozłożone po całym labiryncie, nie tylko w pierwszych rzędach. Zapisać FPS z panelu
      Renderer w trybach `Gouraud` i `Blinn-Phong`

Błąd w dołączanym pliku:

- [ ] przy działającym programie zrobić celowy błąd w
      `assets\shaders\common\lighting.glsl` (na przykład usunąć średnik na końcu linii
      `return max(dot(normal, toLight), 0.0);`), w drugim terminalu
      `cmake --build --preset debug --target copy_assets`, potem kliknąć `Reload shaders`.
      Oczekiwane: linie `lit.vert + lit.frag: FAILED, the previous program stays in use` i
      `gouraud.vert + gouraud.frag: FAILED, the previous program stays in use` są
      czerwone, pod każdą jest czerwony komunikat, który nazywa plik `common/lighting.glsl`
      i numer linii w tym pliku (sterownik może wskazać linię następną po usuniętym
      średniku), trzy pozostałe linie mają `: OK`, obraz się nie zmienia. Te same
      komunikaty są w konsoli jako `[error]`
- [ ] naprawa: `git checkout assets/shaders`, znowu
      `cmake --build --preset debug --target copy_assets` i `Reload shaders`. Oczekiwane:
      wszystkie pięć linii kończy się napisem `: OK`, a `git status` nie pokazuje
      zmienionych plików w `assets/shaders`

Widoki debug w trybie z oświetleniem:

- [ ] przy `Lighting` równym `Blinn-Phong` wybrać w panelu Assets `View mode`
      `Normals as colour`, potem `UVs as colour`. Labirynt jest wtedy rysowany programem
      `textured`, bez świateł, tak samo jak w trybie `Unlit`, ale znaczniki świateł nadal
      są widoczne. Widok normalnych pokazuje normalne używane przez wybrany tryb: z map
      normalnych w trybach `Unlit`, `Phong` i `Blinn-Phong`, z samej siatki w trybie
      `Gouraud` (sekcja 13.4). `Textured` przywraca oświetlony obraz. W trybie `Unlit` te same widoki
      nie mają znaczników

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo, w szczególności żadna z `GL_INVALID_...` po kilku przeładowaniach shaderów

### 13.3. Zmierzone: mapy normalnych (2026-10-05)

Druga część M4. Środowisko to samo: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER,
sterownik 610.74. Opis kodu: [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md),
decyzja: [`../decisions/tangents-on-load.md`](../decisions/tangents-on-load.md).

- [x] build Debug i Release: zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: 163 przypadki testowe, 62220 asercji,
      wszystkie przechodzą. Doszło 14 przypadków: nowy plik `TangentTests.cpp` (9 przypadków,
      177 asercji), po dwa w `ObjLoaderTests.cpp` (dziś 20 i 1576) i `ImageLoaderTests.cpp`
      (dziś 9 i 57), jeden w `LightingTests.cpp` (dziś 17 i 133). Liczby z plików osobno:
      opcja `--source-file`, Debug
- [x] clang-format i clang-tidy: żadnej uwagi
- [x] gra startuje bez linii `[error]`, w tym bez żadnej z nazwą błędu OpenGL (`GL_...`), w
      buildzie Debug. Z tego wynika, że programy `lit` i `textured` skompilowały się z
      drugim plikiem dołączanym `common/normal_map.glsl`, że obie mapy normalnych się
      wczytały i że żaden model nie dostał ostrzeżenia o lustrzanych trójkątach
- [x] kierunek reliefu na zrzutach ekranu: fugi czytają się jako rowki, a nie jako wałki, na
      ścianach wzdłuż osi X, na ścianach wzdłuż osi Z (obróconych o 90 stopni), na słupku i
      na podłodze
- [x] reakcja na kierunek światła: po przejściu światła z lewej strony na prawą jasne i
      ciemne skosy fug zamieniają się miejscami
- [x] średnia jasność zrzutu (skala od 0 do 255) z mapami normalnych i bez nich jest prawie
      równa: ściana wzdłuż X 43,29 i 44,00, ściana wzdłuż Z 35,85 i 36,16, słupek 35,26 i
      35,55, podłoga 22,69 i 22,86. Mapa przesuwa światło między skosami, nie przyciemnia
      sceny
- [x] tryby `Gouraud` i `Unlit`: zrzuty ekranu identyczne co do piksela z mapami włączonymi
      i wyłączonymi
- [x] skrypty Blendera są powtarzalne: dwa kolejne uruchomienia skryptu tekstur i skryptów
      modeli dały identyczne skróty wszystkich dziesięciu plików wyjściowych (4 PNG, 3 OBJ,
      3 MTL). Obrazy koloru i pliki `.obj` są bajt w bajt takie same jak przed tą częścią,
      każdy plik `.mtl` dostał jedną linię `map_Bump`
- [x] liczba wierzchołków i indeksów modeli bez zmian: ściana i słupek 60 i 90, podłoga 4 i
      6 (styczne nie dodają wierzchołków)

Żaden z tych stanów nie był ustawiany kliknięciem w panelu: pola `Normal mapping` nikt
jeszcze nie kliknął ręką. Znane ograniczenia obrazu (podwójnie ciemne fugi, słaba siatka w
ziarnie przy bardzo płaskim kącie światła, migotanie w oddali ocenione tylko na
nieruchomych klatkach) opisuje [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md),
sekcja 2.12.

### 13.4. Otwarte: mapy normalnych, test ręczny na około pięć minut

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu, z testów
jednostkowych i ze zrzutów z sekcji 13.3. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/AssetsPanel.cpp`. Program uruchomiony jak w sekcji 13.2, bez `imgui.ini`.

- [ ] układ: panel Assets stoi w prawej kolumnie pod panelem Maze. Pod listą `View mode`
      jest pole `Normal mapping` (zaznaczone) i notatka, pod nimi `Filter` i
      `Anisotropy`. Sprawdzić, czy panel nie zasłania innego i czy do list `Models` i
      `Textures` trzeba przewijać (panel jest niski, przewijanie jest spodziewane)
- [ ] panel Assets, lista `Models`: pod każdą częścią modelu jest linia
      `normal map: wall_stone_normal.png` (ściana i słupek) albo
      `normal map: floor_stone_normal.png` (podłoga)
- [ ] panel Assets, lista `Textures`: cztery tekstury z podglądem, każda 512 x 512:
      `floor_stone.png`, `floor_stone_normal.png`, `wall_stone.png`, `wall_stone_normal.png`
      (kolejność według wczytania może być inna). Podglądy map normalnych są jasnoniebieskie
      z kolorowymi kreskami fug i stoją prosto
- [ ] `Lighting` równe `Blinn-Phong`, podejść do ściany z włączoną latarką: fugi są rowkami.
      Odznaczyć `Normal mapping`: ściana staje się płaska, plama latarki przesuwa się po
      rysunku kamieni. Zaznaczyć: relief wraca od razu, bez przeładowania
- [ ] to samo przy `Lighting` równym `Phong`
- [ ] stanąć blisko ściany i patrzeć wzdłuż niej, tak żeby latarka świeciła pod płaskim
      kątem: relief jest najmocniejszy. Zrobić krok tak, żeby światło padało z drugiej
      strony: jasne i ciemne skosy zamieniają się miejscami. Zanotować, czy w ziarnie widać
      regularną siatkę i czy przeszkadza
- [ ] przejść korytarzem i obserwować dalekie ściany i podłogę: czy relief migocze w ruchu.
      Powtórzyć z `Filter` równym `Nearest` i `Trilinear` oraz z `Anisotropy` 1 i 16
      (ustawienia działają także na mapy normalnych)
- [ ] `View mode` równe `Normals as colour` przy `Lighting` równym `Blinn-Phong`: na każdej
      ścianie widać kolor podstawowy z rysunkiem fug w innych odcieniach. Odznaczyć
      `Normal mapping`: każda ściana ma jeden jednolity kolor. Zaznaczyć z powrotem
- [ ] `View mode` równe `Normals as colour` przy `Lighting` równym `Gouraud`: ściany są
      jednolite także przy zaznaczonym `Normal mapping` (widok pokazuje normalne, których
      używa wybrany tryb). Przy `Unlit` rysunek fug wraca
- [ ] `View mode` równe `Textured`, `Lighting` równe `Gouraud`: przełączanie
      `Normal mapping` niczego nie zmienia. To samo przy `Unlit`
- [ ] brakująca mapa normalnych: zamknąć program, zmienić nazwę kopii
      `build\debug\Debug\assets\textures\floor_stone_normal.png` (kopii, nie pliku w
      repozytorium), uruchomić. Oczekiwane: podłoga ma teksturę koloru, ale pod latarką jest
      płaska (płaska mapa zastępcza), ściany bez zmian, w konsoli jedna linia `[error]`, w
      panelu Assets przy części `floor_stone` napis `normal map: none (flat)` i sekcja
      `Failed to load` z nazwą pliku. Przywrócić nazwę pliku
- [ ] `Reload shaders` przy włączonych mapach: wszystkie pięć linii kończy się napisem
      `: OK`, relief nie znika (numery jednostek i przełącznik są wysyłane w każdej klatce)
- [ ] błąd w drugim pliku dołączanym: dopisać literę w `common/normal_map.glsl` w kopii
      `assets` obok programu, `Reload shaders`. Oczekiwane: programy `lit` i `textured` mają
      `FAILED` z nazwą `common/normal_map.glsl` i numerem linii w tym pliku, pozostałe trzy
      `: OK`, obraz się nie zmienia. Cofnąć zmianę i przeładować
- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo

## 14. Powiązane dokumenty

- Wersja dla macOS (zweryfikowana) i opis presetów: [`build-macos.md`](build-macos.md)
- Mapa repozytorium i plików konfiguracyjnych: [`project-structure.md`](project-structure.md)
- Biblioteki: [`../libraries/glfw.md`](../libraries/glfw.md),
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md),
  [`../libraries/doctest.md`](../libraries/doctest.md) (testy jednostkowe)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md)
- Dokumentacja CMake (generatory, presety): <https://cmake.org/cmake/help/latest/>
