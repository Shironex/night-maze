# Struktura projektu (stan M0)

Kompletna mapa repozytorium Night Maze: co leży w którym katalogu, do czego służy każdy plik
konfiguracyjny i co powstaje dopiero podczas budowania. Dokument opisuje stan faktyczny po
kamieniu milowym M0. Docelową strukturę (z `gfx/`, `renderer/`, `scene/`, `assets/`) opisuje
PRD w sekcji 6.

Polecenia budowania są w [`build-macos.md`](build-macos.md) i
[`build-windows.md`](build-windows.md), tutaj ich nie powtarzamy.

## 1. Drzewo katalogów

Stan z systemu plików, bez `build/` i `.git/`:

```text
night-maze/
├── CMakeLists.txt              # główny opis buildu: targety engine i night_maze
├── CMakePresets.json           # presety debug i release
├── .clang-format               # styl formatowania kodu
├── .clang-tidy                 # reguły analizy statycznej i konwencja nazw
├── .clangd                     # gdzie clangd ma szukać compile_commands.json
├── .gitattributes              # normalizacja końców linii
├── .gitignore                  # czego nie wersjonujemy
├── .vscode/                    # ustawienia obszaru roboczego Cursor i VS Code
│   ├── extensions.json         # rekomendowane rozszerzenia
│   └── settings.json           # clangd, presety CMake, formatowanie przy zapisie, GLSL
├── cmake/
│   └── Dependencies.cmake      # FetchContent: GLFW i Dear ImGui, target imgui
├── external/
│   └── glad/                   # wygenerowany loader OpenGL 4.1 Core (kod w repozytorium)
│       ├── CMakeLists.txt      # target glad (napisany ręcznie)
│       ├── README.md           # jak wygenerować ponownie
│       ├── include/
│       │   ├── KHR/khrplatform.h   # typy zależne od platformy
│       │   └── glad/gl.h           # deklaracje API OpenGL
│       └── src/gl.c            # loader wypełniający wskaźniki funkcji
├── src/
│   ├── main.cpp                # punkt wejścia, łączy game/ z debug/
│   ├── core/                   # warstwa bazowa: okno, wejście, czas, logi, GL_CHECK
│   │   ├── Application.hpp/.cpp    # klasa bazowa programu, pętla główna
│   │   ├── GlCheck.hpp/.cpp        # makro GL_CHECK
│   │   ├── Input.hpp/.cpp          # stan klawiatury, blokada klawiatury
│   │   ├── Log.hpp/.cpp            # logowanie do konsoli
│   │   ├── Time.hpp/.cpp           # zegar klatki, stały krok symulacji, FPS
│   │   └── Window.hpp/.cpp         # okno GLFW i kontekst OpenGL (RAII)
│   ├── debug/                  # interfejs debugowy (Dear ImGui)
│   │   ├── DebugUI.hpp/.cpp        # kontekst ImGui i cykl klatki
│   │   └── panels/
│   │       └── RendererPanel.hpp/.cpp  # panel "Renderer"
│   └── game/                   # gra
│       └── NightMazeApp.hpp/.cpp   # aplikacja Night Maze (na razie czyści ekran)
└── docs/
    ├── PRD.pdf                 # dokument wymagań
    ├── README.md               # spis treści dokumentacji i kolejność czytania
    ├── syllabus.md             # tabela: temat wykładu, dokument, pliki kodu
    ├── guides/                 # przewodniki
    │   ├── build-macos.md          # budowanie na macOS
    │   ├── build-windows.md        # budowanie na Windowsie
    │   └── project-structure.md    # ten dokument
    ├── libraries/              # dokumenty bibliotek
    │   ├── glad.md
    │   ├── glfw.md
    │   └── imgui.md
    └── modules/                # dokumenty modułów
        ├── core/                   # moduł core, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, warstwy, klatka jako całość, indeks
        │   ├── gl-check.md             # GL_CHECK i błędy OpenGL
        │   ├── input.md                # klawiatura i jej blokada
        │   ├── main-loop.md            # pętla główna, stały krok, FPS
        │   └── window-context.md       # okno, kontekst, GLAD, vsync, Log
        └── debug-ui.md             # panele ImGui w projekcie
```

Zapis `Window.hpp/.cpp` oznacza parę plików `Window.hpp` i `Window.cpp`. Zgodnie z zasadą z
PRD nagłówek i implementacja leżą obok siebie w `src/`, nie ma osobnego katalogu `include/`. Pliki konfiguracyjne z katalogu głównego są
wypisane na początku drzewa, przed katalogami.

### Katalogi

| Katalog | Rola | Kto pisze kod |
|---|---|---|
| `src/` | cały nasz kod | my |
| `cmake/` | pomocnicze pliki CMake dołączane przez `include(...)` | my |
| `external/` | cudzy kod trzymany w repozytorium | generator GLAD, nie edytujemy |
| `docs/` | dokumentacja do nauki | my |
| `build/` | wszystko, co powstaje podczas budowania | CMake i kompilator, poza Gitem |

### Pliki źródłowe

| Plik | Co zawiera | Dokument |
|---|---|---|
| `src/main.cpp` | `main()` z obsługą wyjątków oraz klasę `DebugNightMazeApp`, która dokłada interfejs debugowy do gry | [`../modules/debug-ui.md`](../modules/debug-ui.md) |
| `src/core/Application.*` | `core::Application`: posiada `Window`, `Input`, `Time`, prowadzi pętlę główną, obsługuje Esc | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Window.*` | `core::Window`: inicjalizacja GLFW, okno, kontekst 4.1 Core, `gladLoadGL`, vsync | [`../modules/core/window-context.md`](../modules/core/window-context.md), [`../libraries/glfw.md`](../libraries/glfw.md) |
| `src/core/Input.*` | `core::Input`: odpytywanie klawiszy, `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked` | [`../modules/core/input.md`](../modules/core/input.md) |
| `src/core/Time.*` | `core::Time`: delta czasu, akumulator stałego kroku (`FIXED_DT`), uśrednione FPS | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Log.*` | `logInfo`, `logWarn`, `logError` | [`../modules/core/window-context.md`](../modules/core/window-context.md) |
| `src/core/GlCheck.*` | makro `GL_CHECK` i funkcja `checkGlErrors` | [`../modules/core/gl-check.md`](../modules/core/gl-check.md) |
| `src/game/NightMazeApp.*` | `game::NightMazeApp`: `onUpdate`, `onRender` (viewport, czyszczenie ekranu), kolor tła | [`../modules/core/README.md`](../modules/core/README.md) |
| `src/debug/DebugUI.*` | `debug::DebugUI`: inicjalizacja i zamknięcie ImGui, `draw`, `wantsKeyboard` | [`../modules/debug-ui.md`](../modules/debug-ui.md), [`../libraries/imgui.md`](../libraries/imgui.md) |
| `src/debug/panels/RendererPanel.*` | `debug::drawRendererPanel`: panel "Renderer" | [`../modules/debug-ui.md`](../modules/debug-ui.md) |

Każdy plik źródłowy zaczyna się komentarzem z jednym zdaniem opisu i odnośnikiem
`See docs/modules/...`. To wymaganie z PRD (sekcja 7). Odnośnik wskazuje najbardziej
szczegółowy dokument, czyli ten z kolumny "Dokument" powyżej, na przykład
`// See docs/modules/core/input.md` w `Input.hpp`.

## 2. Warstwy i targety

### Reguła warstw

Zależności są jednokierunkowe. Niższa warstwa nigdy nie wie o wyższej.

```text
main.cpp   łączy game/ i debug/
   │
   ├── debug/   zależy od core/ (i od Dear ImGui), nic nie zależy od debug/
   │
   └── game/    zależy od core/, nie zna debug/
          │
        core/   nie zna ani game/, ani debug/
```

Jak to widać w kodzie:

- `src/core/` dołącza tylko własne nagłówki, GLAD, GLFW i bibliotekę standardową. Także
  blokada klawiatury na czas pracy z panelem jest zrobiona bez ImGui w `core/`:
  `core::Input` ma neutralną flagę `setKeyboardBlocked`, a ustawia ją `main.cpp`.
- `src/game/NightMazeApp.hpp` dołącza `core/Application.hpp` i nic z `debug/`. Komentarz w
  klasie mówi wprost: "It knows nothing about the debug UI".
- `src/debug/DebugUI.cpp` dołącza `core/Window.hpp` i nagłówki ImGui.
- `src/main.cpp` jest jedynym plikiem, który dołącza jednocześnie `game/NightMazeApp.hpp` i
  `debug/DebugUI.hpp`. Definiuje klasę `DebugNightMazeApp final : public game::NightMazeApp`,
  która posiada `debug::DebugUI m_debugUI{window()}` i w `onRender` najpierw woła
  `game::NightMazeApp::onRender(alpha)`, potem obsługuje klawisz `~` (przełącznik paneli),
  rysuje panele i przekazuje do `core::Input` informację, czy ImGui używa klawiatury.

Po co ta dyscyplina: grę da się zbudować i zrozumieć bez paneli debugowych, a panele można
rozbudowywać bez dotykania logiki gry. W docelowej architekturze między `core/` a `game/`
dojdą warstwy `gfx/`, `renderer/` i `scene/`, a `debug/` nadal będzie zależeć od wszystkich
i nikt od niego.

### Dwa targety: `engine` i `night_maze`

| Target | Rodzaj | Pliki | Linkuje |
|---|---|---|---|
| `engine` | biblioteka statyczna | `src/core/*` | `glad`, `glfw` (`PUBLIC`) |
| `night_maze` | program | `src/main.cpp`, `src/game/*`, `src/debug/*` | `engine`, `imgui` (`PRIVATE`) |
| `glad` | biblioteka statyczna | `external/glad/src/gl.c` | nic |
| `glfw` | biblioteka statyczna | pobrana przez FetchContent | biblioteki systemowe |
| `imgui` | biblioteka statyczna | pobrana przez FetchContent, lista plików w `Dependencies.cmake` | `glfw` |

**Dlaczego `engine` jest osobną biblioteką.** Warstwy wielokrotnego użytku (teraz `core`,
później `gfx`, `assets`, `renderer`, `scene`) nie zawierają niczego specyficznego dla Night
Maze. Jako osobny target da się je bez zmian podłączyć do innego programu, w szczególności
do zadań laboratoryjnych z tego samego kursu: nowy plik `main.cpp`, własna klasa pochodna po
`core::Application`, `target_link_libraries(zadanie PRIVATE engine)` i okno z kontekstem
4.1 Core, pętlą i `GL_CHECK` jest gotowe. Granica targetu pilnuje też reguły warstw: gdyby
plik z `core/` spróbował dołączyć coś z `game/` albo ImGui, `engine` nie linkuje tych
rzeczy i błąd wyszedłby szybko.

Biblioteka statyczna (static library) to archiwum skompilowanych plików obiektowych
(`.a` na macOS, `.lib` na Windowsie), które linker wkleja do programu. Nie ma osobnego pliku
do dostarczenia razem z programem, jak przy bibliotece dynamicznej.

## 3. Pliki konfiguracyjne

### 3.1. `CMakeLists.txt`

Główny opis buildu. CMake czyta go od góry do dołu. Plik:
[`CMakeLists.txt`](../../CMakeLists.txt).

**Blok 1: wymagania i projekt**

```cmake
cmake_minimum_required(VERSION 3.24)

project(NightMaze VERSION 0.1.0 LANGUAGES C CXX)
```

- `cmake_minimum_required` musi być pierwszą instrukcją. Odrzuca starszy CMake i ustawia
  zachowanie (polityki) zgodne z wersją 3.24.
- `project` nadaje nazwę i wersję oraz włącza języki. `C` jest potrzebne dla
  `external/glad/src/gl.c`, `CXX` dla naszego kodu.

**Blok 2: standard C++**

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

- `CMAKE_CXX_STANDARD 20`: kompilujemy jako C++20.
- `CMAKE_CXX_STANDARD_REQUIRED ON`: jeśli kompilator nie umie C++20, konfiguracja ma się nie
  udać, zamiast po cichu użyć starszego standardu.
- `CMAKE_CXX_EXTENSIONS OFF`: bez rozszerzeń kompilatora. W praktyce flaga `-std=c++20`
  zamiast `-std=gnu++20`. Kod zgodny ze standardem łatwiej przenosi się między clang a MSVC.

Te zmienne są ustawione przed utworzeniem targetów, więc obowiązują dla wszystkich targetów
C++ w projekcie.

**Blok 3: zależności**

```cmake
add_subdirectory(external/glad)
include(cmake/Dependencies.cmake)
```

- `add_subdirectory` przetwarza `external/glad/CMakeLists.txt` i tworzy target `glad`.
- `include` wkleja zawartość `cmake/Dependencies.cmake`, który tworzy targety `glfw` i `imgui`.

Różnica: `add_subdirectory` wchodzi do katalogu z własnym `CMakeLists.txt` i własnym
zakresem zmiennych. `include` wykonuje plik tak, jakby jego treść stała w tym miejscu.

**Blok 4: funkcja ostrzeżeń**

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

Własna funkcja CMake, żeby nie powtarzać tych samych flag przy każdym targecie. Wywołujemy
ją tylko dla `engine` i `night_maze`. Cudzy kod (GLAD, GLFW, ImGui) kompiluje się ze swoimi
domyślnymi ustawieniami, bo jego ostrzeżeń nie będziemy poprawiać.

- clang i GCC: `-Wall -Wextra` włączają szeroki zestaw ostrzeżeń, `-Wpedantic` ostrzega przed
  odstępstwami od standardu.
- MSVC: `/W4` i `/permissive-`, opis w [`build-windows.md`](build-windows.md).
- `PRIVATE`: flagi dotyczą tylko tego targetu, nie przenoszą się na targety, które go linkują.

**Blok 5: target `engine`**

```cmake
add_library(engine STATIC
    src/core/Application.cpp
    src/core/Application.hpp
    ...
    src/core/Window.cpp
    src/core/Window.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(engine PUBLIC glad glfw)
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE      # GLFW must not include an OpenGL header, GLAD provides it
    GL_SILENCE_DEPRECATION # macOS marks all of OpenGL as deprecated
)
night_maze_enable_warnings(engine)
```

- `add_library(engine STATIC ...)`: lista plików jest jawna. Nie używamy wyszukiwania
  wzorcem (`file(GLOB ...)`), bo wtedy CMake nie zauważa nowych plików bez ponownej
  konfiguracji, a jawna lista pokazuje w historii Gita, kiedy plik doszedł.
- Nagłówki `.hpp` też są na liście. Kompilowane nie są, ale dzięki temu IDE pokazuje je w
  drzewie projektu.
- `target_include_directories(engine PUBLIC .../src)`: korzeniem ścieżek `#include` jest
  `src/`. Stąd zapis `#include "core/Window.hpp"` w każdym pliku, niezależnie od katalogu.
- `target_link_libraries(engine PUBLIC glad glfw)`: `engine` używa GLAD i GLFW.
- `target_compile_definitions`: dwa makra preprocesora, widoczne w linii poleceń
  kompilatora jako `-DGLFW_INCLUDE_NONE -DGL_SILENCE_DEPRECATION`.

Trzy słowa kluczowe zasięgu, które trzeba umieć wyjaśnić:

| Słowo | Dotyczy samego targetu | Przenosi się na targety, które go linkują |
|---|---|---|
| `PRIVATE` | tak | nie |
| `INTERFACE` | nie | tak |
| `PUBLIC` | tak | tak |

Wszystko przy `engine` jest `PUBLIC`, bo jego nagłówki (na przykład `core/GlCheck.hpp`)
same dołączają `<glad/gl.h>`. Każdy, kto dołącza nagłówek `engine`, potrzebuje więc ścieżek
do GLAD i GLFW oraz tych samych makr. Dzięki `PUBLIC` target `night_maze` dostaje to
automatycznie, linkując tylko `engine`.

Dwie definicje `PUBLIC`:

- **`GLFW_INCLUDE_NONE`**: zabrania nagłówkowi `GLFW/glfw3.h` dołączania systemowego nagłówka
  OpenGL. Deklaracje OpenGL mają pochodzić wyłącznie z GLAD. Szczegóły w
  [`../libraries/glfw.md`](../libraries/glfw.md) i [`../libraries/glad.md`](../libraries/glad.md).
- **`GL_SILENCE_DEPRECATION`**: Apple oznaczyło całe OpenGL jako przestarzałe i jego nagłówki
  generują ostrzeżenie przy każdej funkcji. To makro je wycisza. Na Windowsie nie ma efektu.

Definiowanie makr w CMake zamiast `#define` w plikach ma jedną ważną zaletę: nie da się o
nich zapomnieć w nowym pliku.

**Blok 6: target `night_maze`**

```cmake
add_executable(night_maze
    src/main.cpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine imgui)
night_maze_enable_warnings(night_maze)
```

- `add_executable` tworzy program. Bez słowa `WIN32`, więc na Windowsie jest to aplikacja
  konsolowa (opis w [`build-windows.md`](build-windows.md)).
- `PRIVATE engine imgui`: program niczego dalej nie przekazuje, więc `PRIVATE` wystarcza.
  ImGui linkuje tylko `night_maze`, nigdy `engine`.
- Kod `game/` i `debug/` jest dziś częścią programu, nie biblioteki `engine`.

### 3.2. `CMakePresets.json`

Nazwane zestawy ustawień CMake: presety konfiguracji `debug` i `release` (dziedziczące po
ukrytym `base`) oraz odpowiadające im presety budowania. Dzięki nim build na obu systemach i
w każdym IDE zaczyna się od tych samych dwóch poleceń.

Najważniejsze fakty:

- katalog buildu to `build/<nazwa presetu>` (`"binaryDir": "${sourceDir}/build/${presetName}"`),
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` zapisuje `compile_commands.json`,
- `CMAKE_BUILD_TYPE` wybiera Debug lub Release dla generatorów jednokonfiguracyjnych (Mac),
- pole `configuration` w presetach budowania robi to samo dla generatorów
  wielokonfiguracyjnych (Visual Studio).

Pełne omówienie pliku linia po linii jest w [`build-macos.md`](build-macos.md), sekcja 3.
Różnicę między generatorem jedno i wielokonfiguracyjnym wyjaśnia
[`build-windows.md`](build-windows.md), sekcja 3.

### 3.3. `cmake/Dependencies.cmake`

Zależności pobierane podczas konfiguracji przez moduł FetchContent, przypięte do tagów
wydań. Plik jest osobno, żeby główny `CMakeLists.txt` opisywał tylko nasze targety.

| Fragment | Co robi | Szczegółowy opis |
|---|---|---|
| `include(FetchContent)` | wczytuje moduł pobierania | [`../libraries/glfw.md`](../libraries/glfw.md) |
| cztery linie `set(GLFW_... OFF CACHE BOOL "" FORCE)` | wyłączają dokumentację, testy, przykłady i instalację GLFW | [`../libraries/glfw.md`](../libraries/glfw.md) |
| `FetchContent_Declare(glfw ... GIT_TAG 3.4 ...)` i `FetchContent_MakeAvailable(glfw)` | pobierają GLFW 3.4 i tworzą target `glfw` | [`../libraries/glfw.md`](../libraries/glfw.md) |
| `get_target_property` i `set_target_properties(... INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ...)` | oznaczają nagłówki GLFW jako systemowe (bez ostrzeżeń) | [`../libraries/glfw.md`](../libraries/glfw.md) |
| `FetchContent_Declare(imgui ... GIT_TAG v1.92.9b-docking ...)` i `FetchContent_MakeAvailable(imgui)` | pobierają Dear ImGui, bez tworzenia targetu | [`../libraries/imgui.md`](../libraries/imgui.md) |
| `add_library(imgui STATIC ...)`, `target_include_directories`, `target_link_libraries(imgui PUBLIC glfw)` | ręcznie zdefiniowany target `imgui` z rdzenia i dwóch backendów | [`../libraries/imgui.md`](../libraries/imgui.md) |

Pierwsza linia komentarza w pliku przypomina, dlaczego nie ma tu GLAD: to kod wygenerowany,
który leży w `external/glad`.

Zmiana wersji biblioteki to zmiana jednej linii `GIT_TAG` i ponowna konfiguracja.

### 3.4. `external/glad/`

Wygenerowany loader OpenGL 4.1 Core. Jedyny cudzy kod trzymany bezpośrednio w repozytorium.

| Plik | Co to jest |
|---|---|
| `include/glad/gl.h` | deklaracje API OpenGL 4.1 Core: typy, stałe, wskaźniki funkcji (2654 linie) |
| `include/KHR/khrplatform.h` | nagłówek Khronosa z definicjami typów o stałym rozmiarze i makr zależnych od platformy. Dołącza go `gl.h`, my bezpośrednio nigdy |
| `src/gl.c` | implementacja loadera: definicje wskaźników i funkcja `gladLoadGL` |
| `CMakeLists.txt` | napisany ręcznie, dwie instrukcje: `add_library(glad STATIC src/gl.c)` i katalog nagłówków jako `SYSTEM PUBLIC` |
| `README.md` | wersja generatora (GLAD 2.0.8), parametry i polecenia do ponownego wygenerowania |

Zasady: tych plików nie edytujemy ręcznie, nie formatujemy `clang-format` i nie włączamy dla
nich naszych ostrzeżeń. Wszystko o tym, czym jest loader, dlaczego 4.1 Core bez rozszerzeń,
dlaczego kod jest w repozytorium i jak go wygenerować ponownie, jest w
[`../libraries/glad.md`](../libraries/glad.md).

### 3.5. `.clang-format`

Konfiguracja narzędzia clang-format, które automatycznie formatuje kod. Cel: jeden spójny
styl bez dyskusji i bez ręcznego wyrównywania. Plik w katalogu głównym obowiązuje dla
wszystkich plików poniżej. Czytają go też IDE (CLion, VS Code z clangd, Visual Studio).
Polecenia uruchamiające są w [`build-macos.md`](build-macos.md), sekcja 7.

Linie `---` i `...` to znaczniki początku i końca dokumentu YAML.

| Opcja | Wartość | Znaczenie |
|---|---|---|
| `Language` | `Cpp` | reguły dotyczą C i C++ |
| `BasedOnStyle` | `LLVM` | styl bazowy. Wszystko, czego niżej nie zmieniamy, jest jak w stylu LLVM (między innymi klamra otwierająca w tej samej linii) |
| `Standard` | `c++20` | składnia parsowana jako C++20 |
| `IndentWidth` | `4` | wcięcie 4 spacje (LLVM ma 2) |
| `TabWidth` | `4` | szerokość tabulatora przy wyrównywaniu |
| `UseTab` | `Never` | zawsze spacje, nigdy znaki tabulacji. Kod wygląda tak samo w każdym edytorze |
| `ColumnLimit` | `100` | maksymalna długość linii (LLVM ma 80) |
| `AccessModifierOffset` | `-4` | `public:`, `protected:`, `private:` cofnięte o 4, czyli na poziomie słowa `class` |
| `PointerAlignment` | `Left` | `*` i `&` przy typie: `const char* text`, `Window& window` |
| `AllowShortFunctionsOnASingleLine` | `Inline` | w jednej linii mogą być tylko krótkie funkcje zdefiniowane w ciele klasy, na przykład `Window& window() { return m_window; }` |
| `AllowShortIfStatementsOnASingleLine` | `Never` | ciało `if` zawsze w nowej linii |
| `AllowShortLoopsOnASingleLine` | `false` | ciało pętli zawsze w nowej linii |
| `AlwaysBreakTemplateDeclarations` | `Yes` | `template <...>` zawsze w osobnej linii nad deklaracją |
| `BreakConstructorInitializers` | `BeforeColon` | lista inicjalizacyjna konstruktora łamana przed dwukropkiem |
| `PackConstructorInitializers` | `NextLine` | jeśli lista inicjalizacyjna nie mieści się w linii z konstruktorem, przechodzi w całości do następnej, a dopiero gdy i tam się nie mieści, każdy element dostaje własną linię |
| `InsertBraces` | `true` | formater sam dopisuje klamry wokół jednolinijkowych ciał `if`, `else`, `for`, `while` |
| `SortIncludes` | `CaseSensitive` | sortuje dyrektywy `#include` alfabetycznie z rozróżnianiem wielkości liter |
| `IncludeBlocks` | `Preserve` | sortowanie tylko wewnątrz bloków oddzielonych pustą linią, bloki nie są łączone ani przestawiane |

Dwie opcje mają znaczenie większe niż estetyka:

- `InsertBraces: true` usuwa klasę błędów typu "dopisałem drugą linię pod `if` bez klamer".
  To jedyna opcja, która zmienia tokeny kodu, a nie tylko białe znaki.
- `IncludeBlocks: Preserve` chroni kolejność `<glad/gl.h>` przed `<GLFW/glfw3.h>` w
  `Window.cpp`. Oba nagłówki są w osobnych blokach, więc formater ich nie zamieni. Przykład
  efektu `BreakConstructorInitializers` i `PackConstructorInitializers` widać w
  `src/core/Application.cpp`:

  ```cpp
  Application::Application(int width, int height, const std::string& title)
      : m_window(width, height, title), m_input(m_window.nativeHandle()) {}
  ```

### 3.6. `.clang-tidy`

Konfiguracja narzędzia clang-tidy: analizy statycznej (static analysis), która szuka błędów
i złych wzorców bez uruchamiania programu oraz pilnuje konwencji nazw. clang-format zajmuje
się wyglądem, clang-tidy treścią.

```yaml
Checks: >
  -*,
  bugprone-*,
  -bugprone-easily-swappable-parameters,
  performance-*,
  modernize-*,
  -modernize-use-trailing-return-type,
  -modernize-use-nodiscard,
  -bugprone-signed-bitwise,
  readability-identifier-naming
```

Lista jest czytana od lewej. Wpis z minusem wyłącza, bez minusa włącza.

| Wpis | Efekt | Dlaczego |
|---|---|---|
| `-*` | wyłącza wszystkie domyślne kontrole | zaczynamy od zera i włączamy świadomie wybrane grupy, żeby wynik nie był zalany szumem |
| `bugprone-*` | włącza grupę wykrywającą prawdopodobne błędy | na przykład użycie obiektu po przeniesieniu, podejrzane konwersje, pomyłki przy kopiowaniu |
| `-bugprone-easily-swappable-parameters` | wyłącza jedną kontrolę z tej grupy | ostrzega przy sąsiednich parametrach tego samego typu. W kodzie graficznym takie sygnatury są normą, na przykład `Window(int width, int height, ...)` |
| `performance-*` | włącza grupę wydajnościową | niepotrzebne kopie, przekazywanie dużych obiektów przez wartość |
| `modernize-*` | włącza grupę unowocześniającą | sugeruje `nullptr`, `override`, `using`, pętle zakresowe i inne idiomy nowoczesnego C++ |
| `-modernize-use-trailing-return-type` | wyłącza jedną kontrolę z tej grupy | wymagałaby zapisu `auto f() -> int` dla każdej funkcji. Zostajemy przy klasycznym `int f()` |
| `-modernize-use-nodiscard` | wyłącza jedną kontrolę z tej grupy | żądałaby `[[nodiscard]]` przy każdej metodzie `const` zwracającej wartość (`fps()`, `framebufferSize()`, `isKeyDown()`). To poprawna rada, ale dopisek w każdej linii nagłówka zaciemnia kod, który ma być łatwy do czytania |
| `-bugprone-signed-bitwise` | wyłącza jedną kontrolę z grupy `bugprone` | ostrzega przy operacjach bitowych na typach ze znakiem. Flagi ImGui to `int`, więc zapis `ConfigFlags \|= ImGuiConfigFlags_DockingEnable` w `DebugUI.cpp` (zalecany przez samą bibliotekę) byłby zgłaszany za każdym razem |
| `readability-identifier-naming` | włącza jedną kontrolę z grupy `readability` | pilnuje konwencji nazw z `CheckOptions` |

Pozostałe pola:

- `WarningsAsErrors: ''`: żadna diagnostyka nie jest traktowana jak błąd. Narzędzie doradza,
  nie blokuje.
- `HeaderFilterRegex: 'src/.*'`: diagnostyki z nagłówków pokazujemy tylko dla plików z
  `src/`. Nagłówki GLFW, GLAD i ImGui są pomijane.

Konwencja nazw z `CheckOptions`:

| Co | Styl | Przykład z kodu |
|---|---|---|
| przestrzeń nazw (`NamespaceCase`) | `lower_case` | `core`, `game`, `debug` |
| klasa, struktura, enum (`ClassCase`, `StructCase`, `EnumCase`) | `CamelCase` | `Window`, `Size`, `DebugUI` |
| funkcja i metoda (`FunctionCase`) | `camelBack` | `pollEvents`, `wasKeyPressed`, `drawRendererPanel` |
| zmienna i parametr (`VariableCase`, `ParameterCase`) | `camelBack` | `framebuffer`, `clearColor`, `fixedDt` |
| pole prywatne (`PrivateMemberPrefix`, `PrivateMemberCase`) | prefiks `m_` i `camelBack` | `m_window`, `m_clearColor` |
| stała `constexpr` (`ConstexprVariableCase`) | `UPPER_CASE` | `FIXED_DT`, `KEY_COUNT`, `INITIAL_WIDTH` |
| makro (`MacroDefinitionCase`) | `UPPER_CASE` | `GL_CHECK` |

Prefiks `m_` pozwala na pierwszy rzut oka odróżnić pole klasy od zmiennej lokalnej i
parametru.

**Stan narzędzia.** clang-tidy nie wchodzi w skład Xcode Command Line Tools ani pakietu
`clang-format` z Homebrew. Na Macu instaluje się go razem z pakietem `llvm`:

```sh
brew install llvm
"$(brew --prefix llvm)/bin/clang-tidy" -p build/debug \
    --extra-arg=-isysroot --extra-arg="$(xcrun --show-sdk-path)" \
    src/core/Window.cpp
```

- `brew install llvm` instaluje pełny zestaw narzędzi LLVM. Pakiet jest typu keg-only:
  Homebrew celowo nie dodaje go do `PATH`, żeby nie przesłonić kompilatora Apple. Program
  leży w `$(brew --prefix llvm)/bin/clang-tidy`, stąd pełna ścieżka w drugim poleceniu.
- `-p build/debug` wskazuje katalog z `compile_commands.json`. clang-tidy musi znać te same
  flagi co kompilator (ścieżki nagłówków, `-std=c++20`, makra), inaczej nie zrozumie kodu.
  Najpierw trzeba więc wykonać konfigurację presetu `debug`.
- `--extra-arg=-isysroot --extra-arg="$(xcrun --show-sdk-path)"` dopisuje do flag kompilacji
  ścieżkę do SDK macOS. Kompilator Apple zna ją sam, więc nie ma jej w
  `compile_commands.json`, a clang-tidy z Homebrew bez niej nie znajduje nagłówków biblioteki
  standardowej (błędy typu `'array' file not found`).
- Jako argument podajemy pliki `.cpp`. Nagłówki są sprawdzane przy okazji plików, które je
  dołączają.

Stan na M0: narzędzie jest zainstalowane na Macu (LLVM 23.1.2) i przebieg po wszystkich
plikach `.cpp` z `src/` nie zgłasza żadnej diagnostyki. Jedyną poprawką po pierwszym
przebiegu była zamiana `std::endl` na `'\n' << std::flush` w `Log.cpp`
(sprawdzenie `performance-avoid-endl`).

Konfiguracja jest w repozytorium i narzędzie da się uruchomić powyższym poleceniem (więcej
wariantów w [`build-macos.md`](build-macos.md), sekcja 7). Ten dokument nie podaje wyniku
takiego uruchomienia na kodzie M0. clangd w edytorze czyta ten sam plik `.clang-tidy` i
pokazuje część diagnostyk na bieżąco.

### 3.7. `.gitignore`

Lista wzorców plików, których Git ma nie śledzić. Ogólna zasada: do repozytorium trafia to,
co napisał człowiek (i wygenerowany GLAD), a nie to, co da się odtworzyć poleceniem.

| Grupa | Wpisy | Co to jest |
|---|---|---|
| Build output | `build/` | katalog buildu z presetów |
| | `out/` | domyślny katalog buildu Visual Studio w trybie "Open Folder" |
| | `/compile_commands.json` | dowiązanie lub kopia bazy poleceń w katalogu głównym. Pozostałość po czasie, gdy clangd był kierowany dowiązaniem. Dziś robi to `.clangd` (sekcja 3.9), ale wpis zostaje, żeby stare dowiązanie nie trafiło do repozytorium. Ukośnik na początku oznacza: tylko w katalogu głównym |
| IDEs and editors | `.vs/` | katalog roboczy Visual Studio |
| | `.vscode/*` | wszystko w katalogu ustawień Cursor i VS Code, z dwoma wyjątkami niżej |
| | `!.vscode/settings.json` | wyjątek: ten plik **jest** wersjonowany (sekcja 3.10) |
| | `!.vscode/extensions.json` | wyjątek: ten plik **jest** wersjonowany (sekcja 3.11) |
| | `.idea/` | ustawienia CLion |
| | `cmake-build-*/` | domyślne katalogi buildu CLion |
| | `CMakeUserPresets.json` | prywatne presety jednego komputera |
| | `*.swp` | pliki tymczasowe edytora Vim |
| OS | `.DS_Store` | metadane Findera na macOS |
| | `Thumbs.db` | pamięć miniatur Eksploratora Windows |
| Runtime files | `imgui.ini` | układ paneli zapisywany przez Dear ImGui w katalogu roboczym |
| Python virtual environment | `.glad-venv/` | tymczasowe środowisko Pythona używane tylko do generowania GLAD |

Wpis zakończony ukośnikiem dotyczy katalogu. Wpis bez ukośnika na początku pasuje na
dowolnej głębokości drzewa. Wykrzyknik na początku odwraca regułę: plik pasujący do
wcześniejszego wzorca jest jednak śledzony.

Dlaczego `.vscode/*`, a nie `.vscode/`: Git nie potrafi przywrócić pliku, jeśli wykluczony
jest cały jego katalog nadrzędny, bo do takiego katalogu w ogóle nie zagląda. Wzorzec
`.vscode/*` wyklucza **zawartość** katalogu, a nie sam katalog, więc dwa wyjątki z
wykrzyknikiem mogą zadziałać. Efekt: wspólne ustawienia projektu są w repozytorium, a
prywatne pliki edytora (na przykład `launch.json`) zostają lokalne.

### 3.8. `.gitattributes`

Jedna linia:

```gitattributes
* text=auto
```

Git sam rozpoznaje pliki tekstowe i zapisuje je w repozytorium z końcami linii LF,
niezależnie od systemu, na którym powstał commit. Pliki binarne zostawia bez zmian. Dzięki
temu praca na przemian na Macu i Windowsie nie produkuje commitów, w których "zmieniła się"
każda linia. Dokładniejsze omówienie jest w [`build-windows.md`](build-windows.md), sekcja 9.

### 3.9. `.clangd`

Konfiguracja clangd, serwera języka (language server), który w edytorze daje podpowiedzi,
przejście do definicji i błędy na żywo. Cała treść pliku:

```yaml
# clangd (code completion and diagnostics in the editor) reads the compiler flags from
# the Debug build. Run "cmake --preset debug" once so that this file exists:
# build/debug/compile_commands.json
CompileFlags:
  CompilationDatabase: build/debug
```

| Klucz | Wartość | Znaczenie |
|---|---|---|
| `CompileFlags` | sekcja | ustawienia dotyczące flag kompilacji, których clangd używa do analizy plików |
| `CompilationDatabase` | `build/debug` | katalog, w którym clangd ma szukać `compile_commands.json`. Ścieżka względna liczy się od katalogu, w którym leży plik `.clangd` |

Po co to jest: clangd musi znać dokładnie te same flagi co kompilator (ścieżki nagłówków
GLFW, GLAD i ImGui, `-std=c++20`, makra `GLFW_INCLUDE_NONE` i `GL_SILENCE_DEPRECATION`).
Wszystko to zapisuje CMake w `build/debug/compile_commands.json` (sekcja 4.1). Sam z siebie
clangd szuka tego pliku w katalogach nadrzędnych pliku źródłowego i w podkatalogu `build/`,
ale nie w `build/debug/`, więc bez wskazówki by go nie znalazł.

Dwie konsekwencje:

- **Przed pierwszą konfiguracją edytor pokazuje czerwone błędy "file not found"** przy
  dyrektywach `#include`. Plik `build/debug/compile_commands.json` jeszcze nie istnieje.
  Wystarczy raz wykonać `cmake --preset debug`.
- Edytor analizuje kod z flagami konfiguracji **Debug** (bez `NDEBUG`), niezależnie od tego,
  który preset akurat budujemy. Dla makra `GL_CHECK` oznacza to, że clangd widzi wersję ze
  sprawdzaniem błędów.

Wcześniej ten sam cel osiągało dowiązanie symboliczne `compile_commands.json` w katalogu
głównym. Plik `.clangd` jest lepszy, bo jest wersjonowany i działa od razu po sklonowaniu.
Na Windowsie z generatorem Visual Studio `compile_commands.json` nie powstaje, opis w
[`build-windows.md`](build-windows.md), sekcja 4.

### 3.10. `.vscode/settings.json`

Ustawienia obszaru roboczego (workspace settings) dla Cursor i VS Code. Obowiązują tylko po
otwarciu tego katalogu i mają pierwszeństwo przed ustawieniami użytkownika. Plik jest w
formacie JSON z komentarzami, który oba edytory akceptują.

| Klucz | Wartość | Znaczenie |
|---|---|---|
| `C_Cpp.intelliSenseEngine` | `"disabled"` | wyłącza IntelliSense rozszerzenia C/C++ (tego, którego ustawienia zaczynają się od `C_Cpp.`). Samo rozszerzenie może zostać zainstalowane |
| `clangd.arguments` | `["--header-insertion=never"]` | argumenty, z którymi edytor uruchamia clangd. Ten zabrania clangd samodzielnego dopisywania dyrektyw `#include` przy wyborze podpowiedzi |
| `cmake.useCMakePresets` | `"always"` | rozszerzenie CMake Tools zawsze korzysta z `CMakePresets.json`, a nie z własnych zestawów (kits) i własnego katalogu buildu |
| `cmake.configureOnOpen` | `false` | CMake Tools nie uruchamia konfiguracji samo przy otwarciu katalogu. Konfigurację wykonujemy świadomie, poleceniem `cmake --preset debug` |
| `[cpp]` / `editor.formatOnSave` | `true` | edytor formatuje plik C++ przy każdym zapisie. Ustawienie jest wewnątrz bloku `[cpp]`, więc nie dotyczy innych języków, w szczególności wygenerowanego kodu C w `external/glad` |
| `[cpp]` / `editor.defaultFormatter` | `"llvm-vs-code-extensions.vscode-clangd"` | dla plików C++ formaterem jest clangd, który stosuje reguły z `.clang-format` (sekcja 3.5) |
| `files.associations` | `*.vert`, `*.frag`, `*.geom`, `*.glsl` na `glsl` | pliki shaderów są traktowane jako język GLSL (kolorowanie składni). Samych plików jeszcze nie ma, pojawią się w M1 |

Dlaczego tak:

- **Jeden silnik C++.** Gdy działają jednocześnie clangd i IntelliSense drugiego
  rozszerzenia, każdy zgłasza własne błędy. Ten drugi nie czyta `.clangd`, więc nie zna
  ścieżek nagłówków i pokazuje fałszywe "file not found", zdublowane z diagnostykami clangd.
  Komentarz na początku pliku mówi to samo.
- **`--header-insertion=never`.** W tym projekcie kolejność i forma dyrektyw `#include` jest
  celowa: ścieżki piszemy od `src/` (`"core/Window.hpp"`), `<glad/gl.h>` musi stać przed
  `<GLFW/glfw3.h>`, a nagłówki `.hpp` świadomie unikają dołączania GLFW. Automatycznie
  dopisana dyrektywa mogłaby to zepsuć.
- **Formatowanie przy zapisie** sprawia, że sprawdzenie `clang-format --dry-run --Werror`
  ([`build-macos.md`](build-macos.md), sekcja 7) przechodzi bez pamiętania o ręcznym
  formatowaniu.
- **Presety zawsze** gwarantują, że build z edytora trafia do tego samego `build/debug` co
  build z terminala i że `.clangd` wskazuje właściwy katalog.

### 3.11. `.vscode/extensions.json`

Lista rekomendowanych rozszerzeń. Po otwarciu katalogu edytor proponuje ich instalację.
Niczego nie wymusza.

| Identyfikator | Rozszerzenie | Po co w tym projekcie |
|---|---|---|
| `llvm-vs-code-extensions.vscode-clangd` | clangd | silnik C++: podpowiedzi, diagnostyki, formatowanie |
| `ms-vscode.cmake-tools` | CMake Tools | konfiguracja i budowanie z presetów wprost z edytora |
| `vadimcn.vscode-lldb` | CodeLLDB | debugger oparty na LLDB: pułapki i podgląd zmiennych w buildzie Debug |
| `slevesque.shader` | Shader languages support | kolorowanie składni GLSL dla plików z `files.associations` |
| `bierner.markdown-mermaid` | Markdown Preview Mermaid Support | diagramy Mermaid w podglądzie Markdown, czyli diagramy z tej dokumentacji |

## 4. Artefakty generowane podczas budowania (poza Gitem)

Wszystko poniżej powstaje automatycznie i jest w `.gitignore`. Można to w każdej chwili
usunąć i odtworzyć konfiguracją oraz buildem.

### 4.1. Katalog `build/<preset>/`

Układ `build/debug` na Macu (generator Unix Makefiles), odczytany z dysku:

```text
build/debug/
├── night_maze                  # program
├── libengine.a                 # biblioteka statyczna engine (src/core)
├── libimgui.a                  # biblioteka statyczna imgui
├── compile_commands.json       # polecenie kompilacji każdego pliku
├── CMakeCache.txt              # zapamiętane ustawienia konfiguracji
├── Makefile                    # wygenerowany plik buildu
├── cmake_install.cmake         # wygenerowany skrypt instalacji (nieużywany)
├── CMakeFiles/                 # pliki robocze CMake i pliki obiektowe .o
├── external/
│   └── glad/
│       └── libglad.a           # biblioteka statyczna glad
└── _deps/                      # zależności pobrane przez FetchContent
    ├── glfw-src/               # kod źródłowy GLFW 3.4
    ├── glfw-build/             # wynik budowania GLFW, w tym src/libglfw3.a
    ├── glfw-subbuild/          # pomocniczy projekt, który wykonał pobranie
    ├── imgui-src/              # kod źródłowy Dear ImGui v1.92.9b-docking
    ├── imgui-build/            # pusty: ImGui nie ma własnego CMakeLists.txt
    └── imgui-subbuild/         # pomocniczy projekt, który wykonał pobranie
```

| Artefakt | Skąd się bierze | Do czego służy |
|---|---|---|
| `night_maze` | linkowanie targetu `night_maze` | program, który uruchamiamy |
| `libengine.a` | target `engine` | skompilowany kod `src/core`, wklejany do programu |
| `libimgui.a` | target `imgui` z `Dependencies.cmake` | skompilowany rdzeń ImGui i dwa backendy |
| `external/glad/libglad.a` | target `glad` | skompilowany `gl.c` |
| `_deps/glfw-build/src/libglfw3.a` | target `glfw` | skompilowane GLFW |
| `compile_commands.json` | `CMAKE_EXPORT_COMPILE_COMMANDS=ON` z presetu `base` | wejście dla clangd i clang-tidy |
| `CMakeCache.txt` | konfiguracja | trwałe zmienne CMake: ścieżka kompilatora, generator, opcje `GLFW_BUILD_*` |
| `CMakeFiles/` | konfiguracja i build | pliki obiektowe (`.o`), zależności między plikami, log konfiguracji |
| `_deps/*-src` | FetchContent | pobrany kod. Przydatny do czytania: `imgui-src/imgui_demo.cpp`, `glfw-src/docs/` |

Wszystkie biblioteki są statyczne, więc program `night_maze` jest jednym samodzielnym
plikiem. Do działania potrzebuje tylko bibliotek systemowych.

Położenie plików `.a` odzwierciedla drzewo źródeł: target zdefiniowany w głównym
`CMakeLists.txt` trafia do korzenia katalogu buildu, target z `external/glad` do
`external/glad/`, a GLFW do `_deps/glfw-build/`.

`build/release` ma taki sam układ, osobne `_deps` i inne flagi kompilacji. Różnice między
Debug a Release opisuje [`build-macos.md`](build-macos.md), sekcja 5.

### 4.2. Windows: podkatalogi `Debug\` i `Release\`

Generator Visual Studio jest wielokonfiguracyjny, więc wyniki każdej konfiguracji trafiają
do dodatkowego podkatalogu. Oczekiwane położenie programu to
`build\debug\Debug\night_maze.exe` i `build\release\Release\night_maze.exe`, a biblioteki
mają rozszerzenie `.lib` zamiast `.a`. W katalogu buildu zamiast `Makefile` jest rozwiązanie
`.sln` i pliki projektów `.vcxproj`, a `compile_commands.json` nie powstaje.

Ten układ wynika z dokumentacji CMake i nie został jeszcze sprawdzony na PC. Wyjaśnienie i
lista kontrolna są w [`build-windows.md`](build-windows.md).

### 4.3. `imgui.ini`

Plik tekstowy, w którym Dear ImGui zapamiętuje pozycje, rozmiary i zadokowanie paneli.
Powstaje w **katalogu roboczym** procesu: przy uruchomieniu `./build/debug/night_maze` z
katalogu repozytorium jest to katalog główny repozytorium. Jest w `.gitignore`. Usunięcie go
przywraca domyślny układ paneli. Więcej w [`../libraries/imgui.md`](../libraries/imgui.md).

### 4.4. Pozostałe pliki lokalne

- `compile_commands.json` w katalogu głównym: dawne dowiązanie symboliczne do
  `build/debug/compile_commands.json` dla clangd. Nie jest już potrzebne, bo clangd kieruje
  plik `.clangd` (sekcja 3.9). Jeśli zostało z wcześniejszej pracy, można je usunąć.
- `.glad-venv/`: istnieje tylko na czas generowania GLAD.
- `.idea/`, `.vs/`, `out/`, `cmake-build-*/`: katalogi tworzone przez IDE.
- pliki w `.vscode/` inne niż `settings.json` i `extensions.json` (na przykład `launch.json`):
  prywatne ustawienia edytora.
- `CMakeUserPresets.json`: prywatne presety, jeśli ktoś ich potrzebuje.

## 5. Gdzie co dodać

### Nowy plik źródłowy

1. **Wybierz warstwę** według tego, od czego kod zależy:

   | Kod | Katalog | Target w `CMakeLists.txt` |
   |---|---|---|
   | wielokrotnego użytku, bez wiedzy o grze i bez ImGui | `src/core/` (później `src/gfx/`, `src/renderer/`, `src/scene/`, `src/assets/`) | `add_library(engine STATIC ...)` |
   | logika Night Maze | `src/game/` | `add_executable(night_maze ...)` |
   | panel debugowy | `src/debug/panels/` | `add_executable(night_maze ...)` |

2. **Utwórz parę plików** `Nazwa.hpp` i `Nazwa.cpp` obok siebie. Pierwsze linie każdego
   pliku to komentarz z jednym zdaniem opisu i odnośnikiem `See docs/modules/<dokument>.md`
   (dla modułu podzielonego na katalog: `See docs/modules/<moduł>/<dokument>.md`).
   Nagłówek zaczyna się od `#pragma once`, kod jest w przestrzeni nazw warstwy (`core`,
   `game`, `debug`). Publiczne API dostaje komentarze Doxygen (`///`).

3. **Dopisz oba pliki do właściwej listy w `CMakeLists.txt`**, zachowując kolejność
   alfabetyczną. Przykład dla nowej klasy w `core`:

   ```cmake
   add_library(engine STATIC
       ...
       src/core/Log.cpp
       src/core/Log.hpp
       src/core/Paths.cpp      # nowy plik
       src/core/Paths.hpp      # nowy plik
       src/core/Time.cpp
       ...
   )
   ```

   Bez tego kroku plik nie zostanie skompilowany, a objawem będzie błąd linkera
   `undefined symbol`.

4. **Dołączaj nagłówki ścieżką od `src/`**: `#include "core/Paths.hpp"`.

5. **Zbuduj.** CMake wykryje zmianę w `CMakeLists.txt` i sam powtórzy konfigurację
   ([`build-macos.md`](build-macos.md), sekcja 2).

6. **Sformatuj** kod narzędziem clang-format ([`build-macos.md`](build-macos.md), sekcja 7).

7. **Sprawdź regułę warstw**: plik w `core/` nie może dołączać niczego z `game/` ani
   `debug/`, plik w `game/` niczego z `debug/`.

Nowy panel debugowy ma dodatkowe kroki (wywołanie w `DebugUI::draw`). Opisuje je
[`../modules/debug-ui.md`](../modules/debug-ui.md).

### Nowa biblioteka zewnętrzna

- Pobierana (ma repozytorium z tagami wydań): nowy blok `FetchContent_Declare` i
  `FetchContent_MakeAvailable` w `cmake/Dependencies.cmake`, przypięty do tagu, potem
  `target_link_libraries` przy targecie, który jej używa.
- Wygenerowana lub jednoplikowa, trzymana w repozytorium: nowy katalog w `external/` z
  własnym `CMakeLists.txt` i `add_subdirectory` w głównym pliku.
- W obu przypadkach nagłówki oznaczamy jako `SYSTEM`, a bibliotece nie włączamy naszych
  ostrzeżeń.

### Dokumentacja

Według PRD moduł jest skończony dopiero wtedy, gdy ma dokument w `docs/`, aktualizowany w
tym samym commicie co kod.

| Co dodajesz | Gdzie trafia dokument |
|---|---|
| nowy moduł lub klasa w istniejącym module | `docs/modules/<moduł>.md` (szablon 10 sekcji z PRD, sekcja 7). Duży moduł ma katalog `docs/modules/<moduł>/` z plikiem `README.md` (wstęp i indeks) i dokumentami tematycznymi, z których każdy ma pełne 10 sekcji. Wzór: `docs/modules/core/` |
| nowa biblioteka | `docs/libraries/<biblioteka>.md` |
| zmiana w budowaniu, narzędziach lub strukturze | `docs/guides/` (ten plik, `build-macos.md`, `build-windows.md`) |
| decyzja "dlaczego tak, a nie inaczej" | `docs/decisions/` (katalog przewidziany w PRD, jeszcze nie istnieje) |

Po dodaniu pliku, katalogu albo targetu trzeba też zaktualizować drzewo i tabele w tym
dokumencie.
