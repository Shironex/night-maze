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
| git | CMake pobiera nim GLFW, GLM i ImGui | jest w Command Line Tools | `git --version` |
| Ninja | opcjonalny szybszy generator | `brew install ninja` | `ninja --version` |

Uwagi:

- Pełny Xcode nie jest potrzebny, wystarczą Command Line Tools. Pełny Xcode też działa.
- **Ninja nie jest wymagana.** Presety nie wskazują generatora, więc CMake używa domyślnego
  na macOS, czyli Unix Makefiles. Tak był robiony zweryfikowany build.
- Skąd wymóg 3.24: `cmake_minimum_required(VERSION 3.24)` w
  [`CMakeLists.txt`](../../CMakeLists.txt) i `cmakeMinimumRequired` w
  [`CMakePresets.json`](../../CMakePresets.json).
- Bibliotek (GLFW, GLM, ImGui, GLAD) nie instalujemy ręcznie. GLFW, GLM i ImGui pobiera
  CMake, GLAD leży w repozytorium.
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

W oknie widać ciemnogranatowe tło, na środku trójkąt z czerwonym (lewy dolny), zielonym
(prawy dolny) i niebieskim (górny) rogiem i płynnym przejściem kolorów między nimi, a na
wierzchu panel "Renderer". Linia `[error] Shader ...` w terminalu oznacza, że shader się nie
wczytał: wtedy okno pokazuje samo tło ([`../modules/gfx/shaders.md`](../modules/gfx/shaders.md)).

`4.1` potwierdza, że dostaliśmy kontekst, o który prosiliśmy. `Metal` oznacza, że OpenGL na
Apple Silicon jest warstwą zbudowaną nad Metalem. Druga linia zależy od procesora w danym
Macu.

### Sterowanie w M0

| Klawisz | Działanie | Gdzie w kodzie |
|---|---|---|
| Esc | zamyka program | `Application::run` w [`src/core/Application.cpp`](../../src/core/Application.cpp) |
| `~` (na lewo od `1`, `GLFW_KEY_GRAVE_ACCENT`) | pokazuje lub ukrywa interfejs debugowy | `DebugNightMazeApp::onRender` w [`src/main.cpp`](../../src/main.cpp) |

Oba klawisze są ignorowane, dopóki aktywny jest widżet panelu ImGui (na przykład trwa
wpisywanie wartości): klawiatura należy wtedy do panelu. Opis w
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6.

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
- Na Windowsie w tym miejscu jest kopia, a nie dowiązanie, odświeżana przy każdym
  budowaniu ([`build-windows.md`](build-windows.md), sekcja 7).

Co robi każda linia kroku CMake: [`project-structure.md`](project-structure.md), sekcja 3.1,
blok 7.

### Skróty: `make`

Te same polecenia mają krótsze odpowiedniki w pliku [`Makefile`](../../Makefile) w katalogu
głównym repozytorium:

```sh
make run          # konfiguracja, build Debug i uruchomienie
make run-release  # to samo dla Release
make check        # format-check, oba buildy i clang-tidy: komplet przed commitem
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
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i klonuje trzy repozytoria:

| Biblioteka | Tag | Katalog źródeł |
|---|---|---|
| GLFW | `3.4` | `build/debug/_deps/glfw-src` |
| GLM | `1.0.3` | `build/debug/_deps/glm-src` |
| Dear ImGui | `v1.92.9b-docking` | `build/debug/_deps/imgui-src` |

Dla każdej zależności w `_deps` powstają trzy katalogi:

- `<nazwa>-src`: pobrany kod źródłowy,
- `<nazwa>-build`: pliki powstałe przy jej budowaniu (dla GLM i ImGui nie ma tam żadnej
  biblioteki: GLM to same nagłówki, a ImGui kompiluje nasz target `imgui`),
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
| `libengine.a` | nasza biblioteka statyczna `engine` (kod z `src/core` i `src/gfx`) |
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
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

Sprawdzenie bez modyfikowania plików (kod wyjścia różny od zera, gdy coś wymaga zmian):

```sh
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format --dry-run --Werror
```

To drugie polecenie zostało uruchomione na obecnym kodzie M0 (clang-format 22.1.7) i nie
zgłasza żadnych różnic.

Formatujemy tylko `src/`. Katalogu `external/glad` nie dotykamy, bo to kod wygenerowany.

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
find src -name '*.cpp' | xargs "$(brew --prefix llvm)/bin/clang-tidy" -p build/debug \
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
| `[error] Shader compilation failed: ...` z linią `ERROR: 0:N: ...`, w oknie samo tło | błąd w pliku shadera, `N` to numer linii według sterownika | popraw plik w `assets/shaders/` i naciśnij "Reload shaders" w panelu Shaders (albo uruchom program ponownie). Opis w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 7 |
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
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md)
- Windows: [`build-windows.md`](build-windows.md)
- Dokumentacja CMake (presety, FetchContent): <https://cmake.org/cmake/help/latest/>
