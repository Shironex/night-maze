# Budowanie na Windowsie

> **Stan na 2026-10-05: build z terminala jest sprawdzony na Windowsie, część ręczna nie.**
> Kod M0 i M1 powstał na macOS (patrz [`build-macos.md`](build-macos.md)). Na Windowsie 11
> zbudowałem go i uruchomiłem po raz pierwszy 2026-10-05, samymi narzędziami "Visual Studio
> Build Tools 2022", bez środowiska Visual Studio (IDE).
>
> **Zmierzone:** konfiguracja i build Debug oraz Release (zero ostrzeżeń pod `/W4`),
> uruchomienie programu (okno, kostka, dwie linie `[info]`, panele), kopia katalogu `assets`,
> błąd kompilacji shadera przy starcie, generator Ninja. Kod z M2 + M3 (kolizje, labirynt,
> testy) powstał na tym PC: build Debug i Release bez ostrzeżeń i testy jednostkowe w obu
> konfiguracjach (sekcja 2, "Testy jednostkowe").
>
> **Nadal niesprawdzone:** wszystko, co wymaga człowieka przy myszy i klawiaturze (sterowanie,
> kamera, zmiana rozmiaru okna, docking, przycisk "Reload shaders"), praca w Visual Studio
> (Open Folder, F5, Build Solution), RenderDoc i clangd w edytorze. Zdania o tych rzeczach są
> nadal przewidywaniem i są tak oznaczone. Lista kontrolna w sekcji 11 rozróżnia punkty
> zmierzone (`[x]`, z wynikiem) od otwartych (`[ ]`).

## 1. Wymagania

| Narzędzie | Po co | Uwagi |
|---|---|---|
| Visual Studio 2022 albo same "Build Tools for Visual Studio 2022", z pakietem roboczym "Desktop development with C++" (Programowanie aplikacji klasycznych w C++) | kompilator MSVC, Windows SDK, MSBuild, dołączone CMake i Ninja | do pracy z terminala wystarczają same Build Tools (tak było na moim PC). IDE jest potrzebne tylko do sekcji 4. Alternatywa: CLion |
| git dostępny w `PATH` | CMake klonuje nim GLFW, GLM, ImGui i doctest podczas konfiguracji | sprawdzenie: `git --version` w nowym oknie terminala. Instalator: <https://git-scm.com/> |
| CMake w wersji co najmniej 3.24 | konfiguracja i build | jest częścią pakietu roboczego C++ (u mnie 3.31.6-msvc6). Osobny instalator: <https://cmake.org/download/> |

Środowisko, na którym wykonałem pomiary z tego dokumentu (2026-10-05):

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
- Bibliotek nie instalujemy ręcznie. GLFW, GLM, ImGui i doctest pobiera CMake, GLAD jest w
  repozytorium.
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
- Program otwiera okno z ciemnogranatowym tłem i kostką na środku: ściana czerwona z przodu,
  niebieska z lewej, turkusowa u góry. W terminalu są dokładnie dwie linie i żadnej linii
  `[error]`:

  ```text
  [info] GL_VERSION:  4.1.0 NVIDIA 610.74
  [info] GL_RENDERER: NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2
  ```

  Napisy zależą od karty i sterownika. Sterownik NVIDII oddał kontekst dokładnie w wersji
  4.1, o którą prosi program. Komputer ma też zintegrowaną kartę AMD Radeon: system sam
  wybrał kartę NVIDIA.
- Widoczne są trzy panele: Renderer, Shaders i Camera. Przy pierwszym uruchomieniu (gdy nie
  ma jeszcze pliku `imgui.ini`) otwierają się jeden na drugim, w tym samym miejscu, więc
  trzeba je raz rozsunąć myszą. To nie jest cecha Windowsa.

Opis samego pliku presetów (ukryty preset `base`, `inherits`, `binaryDir`) jest w
[`build-macos.md`](build-macos.md), sekcja 3. Plik jest wspólny dla obu systemów.

### Testy jednostkowe

Zwykły build buduje też program testowy `night_maze_tests.exe` (kolizje i labirynt, kod bez
okna). Testy uruchamia `ctest`, program z pakietu CMake, dostępny w tym samym środowisku
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

Zmierzone 2026-10-05 (po czystym buildzie obu presetów, bez ostrzeżeń):

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
[doctest] test cases:    41 |    41 passed | 0 failed | 0 skipped
[doctest] assertions: 58114 | 58114 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Te same liczby daje `build\release\Release\night_maze_tests.exe`. Program testowy nie
otwiera okna. Opis biblioteki, makr i opcji programu:
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
  `night_maze` i `night_maze_tests`. GLFW, ImGui i GLAD kompilują się ze swoimi domyślnymi
  ustawieniami. GLM i doctest nie mają własnych plików do skompilowania (same nagłówki).

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

Ta tabela jest na Windowsie nadal przewidywaniem. Przy pomiarach program był za każdym razem
zatrzymywany przez zabicie procesu, więc plik `imgui.ini` nigdy nie został zapisany i nie
wiem z pomiaru, gdzie powstaje. Bez tego pliku trzy panele otwierają się jeden na drugim
(sekcja 2).

Plik jest w `.gitignore`, więc nigdzie nie przeszkadza w repozytorium. Skutkiem różnych
katalogów jest tylko to, że układ paneli ustawiony przy uruchomieniu z terminala nie jest
widoczny przy uruchomieniu z IDE i odwrotnie.

Dla plików z `assets/` (dziś shadery, później modele i tekstury) katalog roboczy **nie ma
znaczenia**: program szuka ich względem pliku `.exe`, przez `core::assetPath`
([`../modules/core/paths.md`](../modules/core/paths.md)). Na Windowsie położenie programu
podaje `GetModuleFileNameW`. Zmierzone: program startuje bez linii `[error]` i pokazuje
kostkę uruchomiony z katalogu repozytorium, z katalogu roboczego `C:\` oraz z kopii katalogu
`build\debug\Debug` umieszczonej w katalogu z polskimi literami w nazwie
(`...\Temp\nm-Żółw\`). PRD wymaga budowania ścieżek wyłącznie przez `std::filesystem`.

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

1. zmień plik w `assets\shaders\`,
2. w drugim terminalu wykonaj `cmake --build --preset debug --target copy_assets`,
3. naciśnij przycisk "Reload shaders" w panelu Shaders.

Gdy program nie działa, wystarczy zwykłe `cmake --build --preset debug` i ponowne
uruchomienie. Na macOS krok 2 nie jest potrzebny. Pominięcie go na Windowsie nie daje błędu:
panel pokazuje `Last load: OK`, a obraz się nie zmienia, bo program wczytał poprawnie starą
kopię pliku. Podpowiedź (tooltip) nad linią `Vertex` albo `Fragment` w panelu pokazuje pełną
ścieżkę czytanego pliku, czyli kopii w `build\debug\Debug\assets\shaders\`
([`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6.5).

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
| pierwszy build Debug i Release | powstają `build\debug\Debug\assets\shaders\basic.vert` i `basic.frag`, tak samo w `build\release\Release\`. W wyjściu jest linia `Copying assets next to the executable` |
| drugi build bez żadnych zmian, program zatrzymany | linia pojawia się ponownie, program nie jest linkowany |
| zmiana shadera, `cmake --build --preset debug`, uruchomienie | kopia odświeżona, program pokazuje nowe kolory |
| zmiana shadera, `cmake --build --preset debug --target night_maze` | kopia **nie** została odświeżona |
| `cmake --build --preset debug` przy działającym programie | błąd `LNK1168` |
| `cmake --build --preset debug --target copy_assets` przy działającym programie | kod wyjścia 0, kopia odświeżona |
| `--target copy_assets` po usunięciu katalogu `assets` obok programu | katalog odtworzony |
| pełny build przy działającym programie, generator Ninja | przeszedł, jeden krok: kopiowanie |

Samego przycisku "Reload shaders" nikt przy tych próbach nie naciskał: krok 3 reguły jest na
liście kontrolnej w sekcji 11 jako otwarty.

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

Dziś w przechwyconej klatce jest czyszczenie ekranu, jedno wywołanie `glDrawElements` z
kostką (można obejrzeć bufor wierzchołków, bufor indeksów, trzy macierze w uniformach, bufor
głębi oraz wejścia i wyjścia shaderów `basic`) i rysowanie ImGui. Narzędzie stanie się naprawdę użyteczne przy cieniach i efektach pozaekranowych.

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
| Trzy panele leżą jeden na drugim | pierwsze uruchomienie, nie ma jeszcze `imgui.ini` z układem | rozsuń panele myszą za paski tytułu | zmierzone |
| Zmiana w pliku shadera nie jest widoczna po ponownym uruchomieniu | program czyta kopię obok `.exe`, a po zmianie pliku nie było kopiowania albo zbudowano tylko target `night_maze` (możliwe przy F5 w Visual Studio) | `cmake --build --preset debug --target copy_assets` albo pełny build przy zamkniętym programie, sekcja 7 | zmierzone dla `--target night_maze`, F5 przewidywane |
| `[error] Shader compilation failed: ...\assets\shaders/basic.frag` i linia sterownika, w oknie samo tło i panele | błąd składni w pliku shadera. Mieszane ukośniki w ścieżce są poprawne (sekcja 11, klasa `gfx::Shader`) | popraw plik, odśwież kopię (sekcja 7) | zmierzone |
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
- [ ] zmiana rozmiaru okna myszą (szersze, węższe, wyższe niż szersze): kostka zachowuje
      proporcje, nie rozciąga się, zostaje na środku
- [ ] minimalizacja okna i przywrócenie: program nie kończy pracy, w konsoli nie ma linii
      `[error]` ani komunikatu o asercji, kostka wraca. Zapisać, jaki rozmiar framebuffera
      pokazuje panel Renderer zaraz po przywróceniu (na Windowsie zminimalizowane okno ma
      framebuffer 0 x 0, a `NightMazeApp::onRender` pomija wtedy rysowanie: sprawdza
      szerokość i wysokość)
- [x] przy uruchomieniu z terminala są w nim dokładnie dwie linie `[info]`
- [ ] przy uruchomieniu dwuklikiem otwiera się osobne okno konsoli z tymi dwiema liniami
- [x] dokładny napis `GL_VERSION` i `GL_RENDERER`: `4.1.0 NVIDIA 610.74` oraz
      `NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2`. Komputer ma też zintegrowaną kartę AMD
      Radeon, system sam wybrał NVIDIA
- [x] w konsoli nie ma linii `[error]`
- [x] panele "Renderer", "Shaders" i "Camera" są widoczne. Przy pierwszym uruchomieniu (bez
      `imgui.ini`) leżą jeden na drugim
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
- [ ] przeciąganie okna między monitorami o różnym skalowaniu (jeśli są dostępne)

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
  Windows przyjmuje oba
- [ ] ścieżka z polską literą w komunikacie błędu nie zamyka
      programu (w konsoli litera może być wyświetlona błędnie, to dopuszczalne)

**Panel Shaders i przeładowanie shaderów**

Opis: [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6. Na
Windowsie panel jest skompilowany i wyświetla się poprawnie. Przycisku "Reload shaders"
nikt tam jeszcze nie nacisnął.

- [x] `src/debug/panels/ShadersPanel.cpp`, `src/debug/DebugContext.hpp` i `src/main.cpp`
      kompilują się w MSVC z `/W4 /permissive-` bez ostrzeżeń (w szczególności stała
      `constexpr ImVec4 ERROR_TEXT_COLOR` i inicjalizatory desygnowane `DebugContext` z
      polem `.shader` w kolejności deklaracji): zero ostrzeżeń
- [x] panel "Shaders" jest widoczny i pokazuje `Vertex: basic.vert`, `Fragment: basic.frag`,
      `Program: valid`, `Last load: OK`
- [ ] panel "Shaders" daje się zadokować
- [ ] podpowiedź nad linią `Fragment` pokazuje pełną ścieżkę kopii w
      `build\debug\Debug\assets\shaders\`. Oczekiwane są mieszane ukośniki, tak jak w
      komunikacie błędu wyżej: `...\assets\shaders/basic.frag` (podpowiedź i komunikat
      powstają z tej samej ścieżki). Wcześniejsze oczekiwanie "same ukośniki wsteczne" było
      najpewniej błędne. Samej podpowiedzi nie oglądałem
- [ ] przeładowanie udane: przy działającym programie zmiana koloru w
      `assets\shaders\basic.frag`, `cmake --build --preset debug --target copy_assets` w
      drugim terminalu, potem "Reload shaders". Zapisać, czy kostka zmienia kolor
      (oczekiwane: tak). Samo polecenie przy działającym programie jest już zmierzone (kod
      wyjścia 0, kopia odświeżona), otwarte zostaje naciśnięcie przycisku
- [ ] to samo bez kopiowania: "Reload shaders" zaraz po zapisaniu pliku. Oczekiwane:
      `Last load: OK` i obraz bez zmian (program czyta kopię)
- [ ] przeładowanie nieudane: usunięty średnik w `basic.frag`,
      `cmake --build --preset debug --target copy_assets`, "Reload shaders". Oczekiwane:
      `Last load: failed`, czerwony tekst z nazwą pliku i dziennikiem sterownika, ten sam
      tekst w konsoli jako `[error]`, `Program: valid`, kostka bez zmian. Linia sterownika
      powinna być taka jak przy błędzie na starcie (wyżej)
- [ ] naprawa: przywrócony plik, `cmake --build --preset debug --target copy_assets`,
      "Reload shaders". Oczekiwane: `Last load: OK`, pierwotne kolory
- [ ] po kilku przeładowaniach w konsoli nie ma żadnej linii `[error] GL_...` (backend ImGui
      i usunięty stary program, `shader-hot-reload.md`, sekcja 6.3)
- [ ] ścieżka z polską literą (kopia katalogu programu jak w punkcie o `Żółw` wyżej): panel
      i podpowiedź pokazują ścieżkę bez zamknięcia programu. Zapisać, jak wyświetla się
      polska litera (domyślna czcionka ImGui nie ma wszystkich polskich liter, więc
      oczekiwany jest znak zastępczy). Sam start z takiego katalogu jest zmierzony, panelu w
      nim nie oglądałem

**Kamera: sterowanie i panel Camera**

Opis: [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md),
sekcje 5 i 6, [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.6. Na Windowsie
kod jest skompilowany, a panel pokazuje wartości startowe. Sterowanie nie było tam jeszcze
sprawdzane. Windows jest jedyną z dwóch platform, na której GLFW włącza surowy ruch myszy
(`GLFW_RAW_MOUSE_MOTION`), więc obrót myszą działa tu inną ścieżką niż na Macu.

- [x] `src/game/NightMazeApp.cpp`, `src/debug/panels/CameraPanel.cpp`,
      `src/debug/DebugUI.cpp` i `src/main.cpp` kompilują się w MSVC z `/W4 /permissive-` bez
      ostrzeżeń (w szczególności `static_cast<float>` z `double` przy `mouseDeltaX`, `fixedDt`
      i `alpha`, `glm::mix` z trzecim argumentem `float`, domyślny inicjalizator pola
      `m_previousCameraPosition = m_camera.position`, `io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse`
      i trzy pola `DebugContext` w kolejności deklaracji): zero ostrzeżeń
- [x] panel "Camera" jest widoczny i pokazuje `Position` 0, 0, 3, `Yaw` 0, `Pitch` 0,
      `FOV` 60
- [ ] panel "Camera" daje się zadokować
- [ ] kliknięcie lewym przyciskiem w scenę chowa kursor, a kursor nie wyjeżdża poza okno
      (także na drugi monitor)
- [ ] pierwsza klatka po kliknięciu: obraz nie szarpie (kamera nie odskakuje)
- [ ] ruch myszy w prawo obraca kamerę w prawo (kostka ucieka w lewo), ruch do góry podnosi
      wzrok. `Pitch` zatrzymuje się na 89 i na -89
- [ ] obrót jest płynny i ten sam ruch ręki daje ten sam obrót niezależnie od szybkości ruchu
      (surowy ruch myszy, bez przyspieszenia systemowego). Zapisać, czy czułość domyślna
      0,1 jest wygodna, czy wymaga innej wartości niż na Macu
- [ ] długi obrót w jedną stronę (kilka pełnych obrotów): kamera kręci się bez zatrzymania,
      `Yaw` zawija się przez 360
- [ ] W, S, A, D, spacja i lewy Shift przesuwają kamerę zgodnie z opisem, ruch po skosie
      (W i D) nie jest szybszy, klawisze przeciwne (W i S) się znoszą
- [ ] przy widocznym kursorze klawisze ruchu nie przesuwają kamery
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
- [ ] ruch jest płynny na monitorze o odświeżaniu innym niż 60 Hz (na przykład 144 Hz): lot
      bokiem (D) obok kostki przy `Move speed` 20 nie szarpie. Zapisać FPS z panelu Renderer
- [ ] to samo przy wyłączonym vsync w panelu sterownika (kilkaset FPS, większość klatek bez
      kroku symulacji): ruch nadal płynny, prędkość lotu i czułość myszy takie same
- [ ] `Near plane` powyżej około 2,2 odsłania wnętrze kostki, powyżej 3,9 kostka znika,
      `Far plane` poniżej około 2,1 też ją chowa (kamera w pozycji startowej)
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
- [x] program testowy uruchomiony wprost, Debug i Release: 41 przypadków testowych, 58114
      asercji, `Status: SUCCESS!`
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
      `src/core/Paths.cpp` (gałąź Windows, linia `return std::filesystem::path(buffer);`)
- [x] diagnostyki kompilatora clang dla nowych plików i testów, jako zastępstwo za build na
      Macu: `clang-tidy --checks=-*,clang-diagnostic-*,readability-identifier-naming
      --extra-arg=/clang:-Wpedantic -p build\ninja-debug` na czterech nowych plikach `.cpp` z
      `src/` i pięciu z `tests/`: żadnej diagnostyki. Sprawdzenie, że polecenie w ogóle coś
      widzi: po tymczasowym dopisaniu nieużywanej zmiennej i porównania `int` z `unsigned`
      zgłasza `clang-diagnostic-unused-variable` i `clang-diagnostic-sign-compare` (zmiana
      wycofana). Ograniczenie: to clang 19 w trybie zgodności z MSVC i z biblioteką
      standardową MSVC, a nie Apple clang z libc++. Samej flagi `-Wpedantic` osobną próbą nie
      sprawdzałem
- [ ] poprawić albo świadomie wyciszyć tę diagnostykę w `Paths.cpp` (na Macu gałąź nie jest
      kompilowana, więc `make tidy` jej tam nie widzi)
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

## 12. Powiązane dokumenty

- Wersja dla macOS (zweryfikowana) i opis presetów: [`build-macos.md`](build-macos.md)
- Mapa repozytorium i plików konfiguracyjnych: [`project-structure.md`](project-structure.md)
- Biblioteki: [`../libraries/glfw.md`](../libraries/glfw.md),
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md),
  [`../libraries/doctest.md`](../libraries/doctest.md) (testy jednostkowe)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md)
- Dokumentacja CMake (generatory, presety): <https://cmake.org/cmake/help/latest/>
