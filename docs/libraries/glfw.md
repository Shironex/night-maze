# GLFW 3.4

Dokument biblioteki dla kamienia milowego M0. Opisuje stan kodu z `src/core` i konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake). Architekturę modułu `core`
(pętla gry, `Time`, `GL_CHECK`) opisuje [`../modules/core/README.md`](../modules/core/README.md), tutaj
skupiamy się na samej bibliotece.

## 1. Czym jest GLFW

GLFW to mała biblioteka w języku C, która załatwia trzy rzeczy zależne od systemu operacyjnego:

1. tworzy okno (window),
2. tworzy kontekst OpenGL (OpenGL context) powiązany z tym oknem,
3. dostarcza wejście (input): klawiaturę, mysz, zdarzenia okna.

Każdy system robi to inaczej (Cocoa i NSGL na macOS, Win32 i WGL na Windowsie). GLFW chowa te
różnice za jednym API, dzięki czemu `src/core/Window.cpp` jest identyczny na obu platformach.

### Za co GLFW NIE odpowiada

- Nie rysuje. Nie zawiera ani jednej funkcji `gl*`. Rysowanie to OpenGL, czyli sterownik karty.
- Nie ładuje funkcji OpenGL. Daje tylko `glfwGetProcAddress`, z którego korzysta GLAD
  (patrz [`glad.md`](glad.md)).
- Nie ma matematyki (to GLM, patrz [`glm.md`](glm.md)), nie wczytuje obrazów ani modeli, nie
  odtwarza dźwięku.
- Nie ma interfejsu użytkownika. Panele rysuje Dear ImGui (patrz [`imgui.md`](imgui.md)).
- Nie prowadzi pętli gry. Pętlę piszemy sami w
  [`src/core/Application.cpp`](../../src/core/Application.cpp), GLFW dostarcza tylko klocki
  (`glfwPollEvents`, `glfwSwapBuffers`).

## 2. Jak podpinamy GLFW w CMake

Cały fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
include(FetchContent)

# ---- GLFW: window, OpenGL context, input ---------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Treat the GLFW headers as system headers so they cannot produce warnings in our code.
get_target_property(glfw_include_dirs glfw INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(glfw PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${glfw_include_dirs}")
```

### Linia po linii

**`include(FetchContent)`** wczytuje moduł CMake, który potrafi pobrać cudze repozytorium w
czasie konfiguracji (configure), czyli podczas `cmake --preset debug`, a nie podczas kompilacji.

**Cztery linie `set(GLFW_... OFF CACHE BOOL "" FORCE)`** wyłączają rzeczy, których nie
potrzebujemy:

| Opcja | Co wyłącza | Wartość domyślna w GLFW 3.4 |
|---|---|---|
| `GLFW_BUILD_DOCS` | budowanie dokumentacji (wymaga Doxygena) | `ON` |
| `GLFW_BUILD_TESTS` | programy testowe GLFW | `ON` tylko gdy GLFW jest projektem głównym |
| `GLFW_BUILD_EXAMPLES` | przykładowe programy GLFW | `ON` tylko gdy GLFW jest projektem głównym |
| `GLFW_INSTALL` | reguły `install` dla GLFW | `ON` |

Dlaczego `CACHE ... FORCE`, a nie zwykłe `set(GLFW_BUILD_DOCS OFF)`? GLFW deklaruje te opcje
poleceniem `option(...)`, które zapisuje wartość w pamięci podręcznej CMake (`CMakeCache.txt`).
`option` nie nadpisuje wartości, która już jest w cache, więc ustawiamy ją tam sami, zanim GLFW
zostanie dołączone. `FORCE` gwarantuje, że nasza wartość wygra także wtedy, gdy w cache z
poprzedniej konfiguracji zostało coś innego. Dlatego te linie muszą stać **przed**
`FetchContent_MakeAvailable(glfw)`.

Dwie z czterech opcji (testy i przykłady) i tak byłyby wyłączone, bo GLFW włącza je tylko wtedy,
gdy jest budowane samodzielnie. Ustawiamy je jawnie, żeby konfiguracja nie zależała od
domysłów i żeby było widać intencję.

**`FetchContent_Declare(glfw ...)`** tylko zapisuje, skąd i co pobrać. Nic jeszcze nie pobiera.

- `GIT_REPOSITORY`: adres oficjalnego repozytorium.
- `GIT_TAG 3.4`: przypięcie (pin) do tagu wydania 3.4.
- `GIT_SHALLOW TRUE`: płytki klon, czyli bez historii. Pobieramy jeden stan repozytorium
  zamiast wszystkich commitów, więc pierwsza konfiguracja jest szybsza.

**`FetchContent_MakeAvailable(glfw)`** robi właściwą pracę: klonuje repozytorium do
`build/<preset>/_deps/glfw-src`, a ponieważ GLFW ma własny `CMakeLists.txt`, dołącza go tak,
jakbyśmy napisali `add_subdirectory`. Od tej chwili w naszym projekcie istnieje target `glfw`
(biblioteka statyczna, na Macu plik `build/debug/_deps/glfw-build/src/libglfw3.a`).

Z targetu korzystamy w głównym [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
```

`PUBLIC` oznacza, że każdy, kto linkuje `engine` (czyli `night_maze`), dostaje też nagłówki i
bibliotekę GLFW. Dlatego `src/main.cpp` może napisać `#include <GLFW/glfw3.h>` (dla `GLFW_KEY_GRAVE_ACCENT`),
chociaż `night_maze` linkuje jawnie tylko `engine` i `imgui`.

### Dlaczego przypinamy wersję

Bez przypięcia (na przykład z gałęzią `master`) każda świeża konfiguracja mogłaby pobrać inny
kod. Wtedy build na Macu i na Windowsie mógłby się różnić, a projekt mógłby przestać się
kompilować w dniu obrony bez żadnej zmiany z naszej strony. Tag `3.4` daje powtarzalność:
ten sam kod u nas, na drugim komputerze i u prowadzącego. Wersja 3.4 jest też zapisana w PRD
jako wybór stosu technologicznego.

Uwaga dla dociekliwych: tag w Gicie da się teoretycznie przesunąć. Najmocniejszym przypięciem
byłby pełny hash commita. Dla oficjalnych wydań GLFW tag jest wystarczający i czytelniejszy.

### Para `get_target_property` / `set_target_properties`: nagłówki GLFW jako systemowe

Nasz kod kompilujemy z ostrymi ostrzeżeniami (`-Wall -Wextra -Wpedantic`, na MSVC `/W4`).
Ostrzeżenie może powstać także w cudzym nagłówku, który dołączamy przez `#include`. Nie chcemy
tego, bo nie będziemy poprawiać GLFW, a szum zasłoniłby ostrzeżenia z naszego kodu.

Kompilatory mają na to mechanizm: katalog nagłówków systemowych (system include directory).
Ostrzeżenia z nagłówków znalezionych w takim katalogu są wyciszane.

1. `get_target_property(glfw_include_dirs glfw INTERFACE_INCLUDE_DIRECTORIES)` odczytuje do
   zmiennej `glfw_include_dirs` listę katalogów, które target `glfw` przekazuje swoim
   użytkownikom (u nas to `_deps/glfw-src/include`).
2. `set_target_properties(glfw PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${glfw_include_dirs}")`
   wpisuje tę samą listę do właściwości, która mówi: te katalogi traktuj jako systemowe.

Efekt widać w `build/debug/compile_commands.json`. Katalog GLFW jest podany przez `-isystem`,
a nasz `src` przez zwykłe `-I`:

```text
-I.../night-maze/src -isystem .../external/glad/include -isystem .../build/debug/_deps/glfw-src/include -isystem .../build/debug/_deps/glm-src
```

Ostatni katalog to GLM, oznaczony tym samym sposobem ([`glm.md`](glm.md), sekcja 2).

Dlaczego tak okrężnie? Prostszy zapis to słowo `SYSTEM` w `FetchContent_Declare`, ale ta opcja
istnieje dopiero od CMake 3.25. Nasze minimum to 3.24 (`cmake_minimum_required(VERSION 3.24)`
w `CMakeLists.txt` oraz `cmakeMinimumRequired` w `CMakePresets.json`), więc robimy to samo
ręcznie, sposobem działającym w 3.24.

## 3. Najważniejsze API z przykładami z naszego kodu

Wszystkie wywołania GLFW związane z oknem są w
[`src/core/Window.cpp`](../../src/core/Window.cpp), a klawiatura i mysz w
[`src/core/Input.cpp`](../../src/core/Input.cpp).

### 3.1. Callback błędów: `glfwSetErrorCallback`

```cpp
// GLFW calls this whenever one of its functions fails, with a readable description.
void onGlfwError(int code, const char* description) {
    logError("GLFW error " + std::to_string(code) + ": " + description);
}
```

```cpp
Window::Window(int width, int height, const std::string& title) {
    glfwSetErrorCallback(onGlfwError);

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
```

Funkcje GLFW sygnalizują błąd tylko wartością zwracaną (`nullptr`, `GLFW_FALSE`). Powód błędu
trafia do callbacku błędów (error callback). Bez niego wiedzielibyśmy tylko, że okno się nie
utworzyło, a nie dlaczego.

Callback ustawiamy **przed** `glfwInit`. To jedna z nielicznych funkcji GLFW, które wolno
wywołać przed inicjalizacją, i właśnie po to, żeby zobaczyć także błędy samego `glfwInit`.

### 3.2. Wskazówki okna (window hints) dla OpenGL 4.1 Core

```cpp
    // Ask for OpenGL 4.1 Core, the highest version available on macOS.
    // macOS only offers Core contexts that are forward compatible, so ask for the same
    // on Windows. GLFW up to 3.3 refused to create the window on macOS without this hint.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
```

`glfwWindowHint` ustawia parametry **następnego** tworzonego okna. Wywołane po
`glfwCreateWindow` nie mają już wpływu na to okno.

| Hint | Wartość | Znaczenie |
|---|---|---|
| `GLFW_CONTEXT_VERSION_MAJOR` / `MINOR` | 4 / 1 | minimalna wersja kontekstu: 4.1 |
| `GLFW_OPENGL_PROFILE` | `GLFW_OPENGL_CORE_PROFILE` | profil Core: bez starego API (`glBegin`, stały potok) |
| `GLFW_OPENGL_FORWARD_COMPAT` | `GLFW_TRUE` | kontekst zgodny w przód (forward compatible): usunięte także to, co w 4.1 oznaczono jako przestarzałe |

**Dlaczego macOS tego wymaga.** macOS ma dwa rodzaje kontekstów OpenGL: stary (legacy) w
wersji 2.1 oraz profil Core w wersjach 3.2 do 4.1. Profilu zgodności (compatibility profile)
dla wersji 3.2 i wyższych nie ma w ogóle. Jeżeli nie podamy żadnych hintów, GLFW poprosi o
wersję 1.0 i na Macu dostaniemy kontekst 2.1, w którym nie działają shadery `#version 410 core`
ani obiekty VAO w formie, jakiej używa kurs. Żeby dostać cokolwiek nowszego, trzeba jawnie
poprosić o wersję co najmniej 3.2 **i** o profil Core.

**Dlaczego akurat 4.1.** To najwyższa wersja, jaką Apple kiedykolwiek udostępniło. Windows
oferuje 4.6, ale projekt musi działać na obu systemach, więc 4.1 jest wspólnym mianownikiem.
Wersja z hintów to minimum: sterownik na Windowsie może zwrócić kontekst nowszy niż 4.1
(zgodny z 4.1). Przed użyciem funkcji z 4.2+ chroni nas wtedy GLAD, a nie GLFW (patrz
[`glad.md`](glad.md)).

**`GLFW_OPENGL_FORWARD_COMPAT` a wersja GLFW.** Większość poradników (pisanych pod GLFW 3.3
i starsze) mówi, że macOS wymaga tego hintu. Tak było: bez niego `glfwCreateWindow` zwracało
błąd. W GLFW 3.4 ten wymóg zniesiono (lista zmian wydania 3.4: "[NSGL] Removed enforcement of
forward-compatible flag for core contexts"), obowiązkowy pozostał tylko hint profilu.
Komentarz w naszym kodzie mówi to wprost: "GLFW up to 3.3 refused to create the window on
macOS without this hint".

Hint zostawiamy z trzech powodów:

- macOS i tak daje wyłącznie konteksty Core zgodne w przód. Ustawiając hint, prosimy o to
  samo zachowanie na Windowsie, więc oba systemy odrzucają ten sam przestarzały kod.
- Jest wymieniony w PRD (strona 4, wiersz "Kontekst").
- Niczego nie psuje i działa także ze starszym GLFW.

Na obronie warto umieć powiedzieć obie rzeczy: po co hint powstał i że w 3.4 nie jest już
egzekwowany na macOS.

### 3.3. Tworzenie okna i sprzątanie po błędzie

```cpp
    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_handle == nullptr) {
        // The destructor does not run when a constructor throws, so clean up here.
        glfwTerminate();
        throw std::runtime_error("Failed to create a window with an OpenGL 4.1 Core context");
    }
```

Dwa ostatnie argumenty `glfwCreateWindow` to monitor (tryb pełnoekranowy) i okno, z którym
współdzielimy zasoby kontekstu. `nullptr, nullptr` oznacza zwykłe okno bez współdzielenia.

`glfwCreateWindow` tworzy **jednocześnie** okno i kontekst OpenGL. Jeśli sterownik nie umie
dać wersji 4.1 Core, zwraca `nullptr`, a callback błędów wypisuje powód.

W C++ destruktor obiektu nie uruchamia się, gdy konstruktor rzucił wyjątek, dlatego przed
`throw` ręcznie wołamy `glfwTerminate()`. W normalnym przebiegu sprząta destruktor:

```cpp
Window::~Window() {
    glfwDestroyWindow(m_handle);
    glfwTerminate();
}
```

### 3.4. Kontekst bieżący: `glfwMakeContextCurrent`

```cpp
    // OpenGL calls always go to the context that is current on the calling thread.
    glfwMakeContextCurrent(m_handle);
```

OpenGL nie przyjmuje kontekstu jako argumentu funkcji. `glClear(...)` działa na kontekście,
który jest **bieżący (current) w wątku wywołującym**. Samo utworzenie okna nie czyni kontekstu
bieżącym. Konsekwencje:

- Przed `glfwMakeContextCurrent` nie wolno wołać żadnej funkcji `gl*`, a także
  `gladLoadGL` ani `glfwSwapInterval`, bo one też potrzebują bieżącego kontekstu.
- Kontekst może być bieżący tylko w jednym wątku naraz. U nas wszystko dzieje się w wątku
  głównym.

Stąd kolejność w konstruktorze `Window`: `glfwCreateWindow`, `glfwMakeContextCurrent`,
`gladLoadGL(glfwGetProcAddress)`, `glfwSwapInterval(1)`, i dopiero potem `glGetString`.

### 3.5. `glfwGetProcAddress`: most do GLAD

```cpp
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
        throw std::runtime_error("Failed to load the OpenGL functions (GLAD)");
    }
```

`glfwGetProcAddress("glClear")` zwraca adres funkcji `glClear` w sterowniku dla bieżącego
kontekstu. GLAD woła tę funkcję dla każdej nazwy z OpenGL 4.1. Szczegóły w
[`glad.md`](glad.md).

### 3.6. `glfwSwapInterval`: synchronizacja pionowa (vsync)

```cpp
    // Vsync: wait for one screen refresh between buffer swaps.
    glfwSwapInterval(1);
```

Argument to liczba odświeżeń ekranu, na które `glfwSwapBuffers` ma poczekać przed zamianą
buforów. `1` włącza vsync: liczba klatek na sekundę jest ograniczona do częstotliwości
monitora (60 lub 120 Hz), nie ma rozrywania obrazu (tearing) i procesor graficzny nie pracuje
na próżno. `0` wyłącza czekanie: FPS bez limitu, przydatne do pomiarów wydajności.

Wartość domyślna zależy od sterownika, dlatego ustawiamy ją jawnie. Funkcja dotyczy
bieżącego kontekstu, więc musi stać po `glfwMakeContextCurrent`.

### 3.7. `glfwPollEvents` a `glfwSwapBuffers`

To dwie różne czynności, obie raz na klatkę, i łatwo je pomylić.

| | `glfwPollEvents()` | `glfwSwapBuffers(window)` |
|---|---|---|
| Kierunek | system operacyjny do programu | program do ekranu |
| Co robi | odbiera oczekujące zdarzenia (klawisze, mysz, zmiana rozmiaru, przycisk zamknięcia), uruchamia callbacki, aktualizuje stan klawiszy, przycisków myszy i pozycję kursora | zamienia bufor tylny (back buffer), po którym rysowaliśmy, z przednim (front buffer), który widać |
| Bez tego | okno "nie odpowiada", `glfwGetKey` zwraca stare dane, okna nie da się zamknąć | na ekranie nigdy nic się nie pojawi |
| Blokuje? | nie, wraca od razu | tak, przy vsync czeka na odświeżenie ekranu |

W naszym kodzie obie funkcje są opakowane w metody `Window::pollEvents` i
`Window::swapBuffers`, a wołane w [`src/core/Application.cpp`](../../src/core/Application.cpp):

```cpp
void Application::run() {
    // Start measuring here, so the first frame does not include the start-up time.
    m_time.reset();

    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        m_input.update();
        // Escape first gives a captured cursor back, and closes the window only when
        // the cursor is not captured.
        if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
            if (m_input.isCursorCaptured()) {
                m_input.setCursorCaptured(false);
            } else {
                m_window.requestClose();
            }
        }

        // Simulation: as many fixed steps as fit into the time that has passed.
        m_time.beginFrame();
        while (m_time.consumeFixedStep()) {
            onUpdate(Time::FIXED_DT);
        }

        // Rendering: once per frame.
        onRender(m_time.alpha());
        m_window.swapBuffers();
    }
}
```

Zdarzenia na początku klatki (żeby logika widziała świeże wejście), zamiana buforów na końcu
(żeby pokazać skończoną klatkę). Rysujemy zawsze do bufora tylnego, dzięki czemu użytkownik
nigdy nie widzi klatki w połowie narysowanej: to podwójne buforowanie (double buffering).

Zamykanie okna to tylko flaga. `glfwWindowShouldClose` ją czyta, `glfwSetWindowShouldClose`
ustawia (u nas `Window::shouldClose` i `Window::requestClose`). Kliknięcie krzyżyka ustawia tę
samą flagę wewnątrz `glfwPollEvents`. Pętla kończy się dopiero przy następnym sprawdzeniu
warunku `while`.

### 3.8. Rozmiar framebuffera a rozmiar okna (Retina)

```cpp
Size Window::framebufferSize() const {
    Size size;
    glfwGetFramebufferSize(m_handle, &size.width, &size.height);
    return size;
}

Size Window::windowSize() const {
    Size size;
    glfwGetWindowSize(m_handle, &size.width, &size.height);
    return size;
}
```

GLFW zna dwa układy jednostek:

- **współrzędne ekranowe (screen coordinates)**: w nich podajemy rozmiar do
  `glfwCreateWindow` i w nich GLFW raportuje pozycję myszy. Zwraca je `glfwGetWindowSize`.
- **piksele**: fizyczne piksele bufora, po którym rysuje OpenGL. Zwraca je
  `glfwGetFramebufferSize`.

Na ekranie Retina jedna jednostka ekranowa to 2 x 2 piksele. Nasze okno 1280 x 720 ma tam
framebuffer 2560 x 1440. Na Windowsie oba rozmiary zawsze są równe (dokumentacja GLFW 3.4:
na Windowsie i X11 rozmiar framebuffera i obszaru okna odwzorowują się 1:1).

`glViewport` przyjmuje **piksele**, więc musi dostać rozmiar framebuffera. Tak robi
[`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp):

```cpp
    const core::Size framebuffer = window().framebufferSize();
    GL_CHECK(glViewport(0, 0, framebuffer.width, framebuffer.height));
```

Gdybyśmy podali rozmiar okna, na Retinie obraz zajmowałby lewą dolną ćwiartkę okna. Rozmiar
pobieramy co klatkę, co przy okazji obsługuje zmianę rozmiaru okna bez osobnego callbacku.
Oba rozmiary widać w panelu "Renderer" (linie `Framebuffer:` i `Window:`), więc różnicę można
pokazać na żywo.

Zasada na przyszłość: wszystko, co dotyczy OpenGL (viewport, rozmiary FBO), liczymy z
framebuffera. Wszystko, co dotyczy myszy, liczymy w jednostkach okna.

### 3.9. Klawiatura przez odpytywanie: `glfwGetKey` w `core::Input`

GLFW oferuje dwa sposoby czytania klawiatury:

- **callback** (`glfwSetKeyCallback`): GLFW woła naszą funkcję przy każdym zdarzeniu klawisza,
- **odpytywanie (polling)** (`glfwGetKey`): sami pytamy, czy klawisz jest teraz wciśnięty.

`core::Input` używa odpytywania. Cały mechanizm to kilka linii w
[`src/core/Input.cpp`](../../src/core/Input.cpp). Początek `Input::update` (dalsza część
funkcji dotyczy myszy, sekcja 3.10):

```cpp
    m_previous = m_current;
    // GLFW key codes start at GLFW_KEY_SPACE (32), lower values are not valid keys.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        m_current[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }
```

i dwa pytania:

```cpp
bool Input::isKeyDown(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key];
}

bool Input::wasKeyPressed(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key] && !m_previous[key];
}
```

Jak to czytać:

- `glfwGetKey` zwraca `GLFW_PRESS` albo `GLFW_RELEASE`: stan klawisza zapamiętany przez GLFW
  podczas ostatniego `glfwPollEvents`. Dlatego `m_input.update()` stoi w pętli zaraz **po**
  `m_window.pollEvents()`.
- Trzymamy dwie tablice: stan z tej klatki (`m_current`) i z poprzedniej (`m_previous`).
  `isKeyDown` to "klawisz jest trzymany" (ruch gracza). `wasKeyPressed` to zbocze: jest
  wciśnięty teraz, a klatkę temu nie był (przełączniki takie jak klawisz `~` i Esc). Bez tego
  klawisz `~` przełączałby panel 60 razy na sekundę, dopóki go trzymamy.
- Pętla zaczyna od `GLFW_KEY_SPACE` (32), bo `glfwGetKey` dla wartości spoza zakresu od
  `GLFW_KEY_SPACE` do `GLFW_KEY_LAST` zgłasza błąd `GLFW_INVALID_ENUM`.
- Klawisze identyfikujemy stałymi GLFW (`GLFW_KEY_ESCAPE`, `GLFW_KEY_GRAVE_ACCENT`). Oznaczają
  one fizyczne położenie klawisza w układzie US, nie znak, który klawisz wpisuje.
  `GLFW_KEY_GRAVE_ACCENT` to klawisz na lewo od `1`, ten z `~`.
- `m_keyboardBlocked` to flaga ustawiana z zewnątrz przez `setKeyboardBlocked`. Gdy jest
  ustawiona, oba pytania zwracają `false` dla każdego klawisza, a `update` mimo to dalej
  odświeża tablice. `main.cpp` ustawia ją, gdy klawiatury używa panel ImGui. Opis w
  [`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6.

Nagłówek [`src/core/Input.hpp`](../../src/core/Input.hpp) celowo nie dołącza `GLFW/glfw3.h`:
używa deklaracji zapowiadającej `struct GLFWwindow;` i liczby 348 zamiast `GLFW_KEY_LAST`.
Zgodność tej liczby z GLFW sprawdza kompilator w `Input.cpp`:

```cpp
    static_assert(KEY_COUNT == GLFW_KEY_LAST + 1, "KEY_COUNT must cover every GLFW key code");
```

Jeżeli kiedyś zmienimy wersję GLFW i `GLFW_KEY_LAST` się zmieni, build się nie skompiluje,
zamiast po cichu czytać poza tablicą.

### 3.10. Mysz przez odpytywanie: przyciski, kursor i tryb kursora

Mysz czytamy tak samo jak klawiaturę: przez odpytywanie, bez callbacków. Callbacki pozycji
kursora i przycisków myszy są zajęte przez backend ImGui (pułapka 9), a odpytywanie niczego
nie podmienia. Funkcje GLFW, których używamy:

| Funkcja | Gdzie | Co robi |
|---|---|---|
| `glfwGetMouseButton(window, button)` | `Input::update` | Zwraca `GLFW_PRESS` albo `GLFW_RELEASE` dla przycisku, stan z ostatniego `glfwPollEvents`. Przyciski mają numery od `GLFW_MOUSE_BUTTON_1` (0) do `GLFW_MOUSE_BUTTON_LAST` (7). `GLFW_MOUSE_BUTTON_LEFT`, `RIGHT`, `MIDDLE` to numery 0, 1, 2 |
| `glfwGetCursorPos(window, &x, &y)` | `Input::update` | Wpisuje pozycję kursora jako `double`, we współrzędnych ekranu (sekcja 3.8), względem lewego górnego rogu obszaru roboczego okna. Oś y rośnie w dół |
| `glfwSetInputMode(window, GLFW_CURSOR, tryb)` | `Input::setCursorCaptured` | Ustawia tryb kursora (tabela niżej) |
| `glfwRawMouseMotionSupported()` | `Input::setCursorCaptured` | Zwraca `GLFW_TRUE`, jeśli platforma obsługuje surowy ruch myszy |
| `glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, wartość)` | `Input::setCursorCaptured` | Włącza albo wyłącza surowy ruch myszy (raw mouse motion) |

Odczyt w `Input::update`, zaraz po klawiszach:

```cpp
    m_mousePrevious = m_mouseCurrent;
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        m_mouseCurrent[button] = glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    // Cursor position in screen coordinates, relative to the top left corner of the window.
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(m_window, &cursorX, &cursorY);
```

GLFW podaje **pozycję** kursora. Kamerze potrzebne jest **przesunięcie** od poprzedniej klatki,
więc `Input` samo odejmuje pozycję zapamiętaną klatkę wcześniej i udostępnia wynik jako
`mouseDeltaX()` i `mouseDeltaY()`.

Tryby kursora (`GLFW_CURSOR`):

| Tryb | Zachowanie |
|---|---|
| `GLFW_CURSOR_NORMAL` | zwykły, widoczny kursor. Tryb domyślny |
| `GLFW_CURSOR_HIDDEN` | kursor niewidoczny nad oknem, ale dalej ograniczony krawędziami ekranu. Nie używamy |
| `GLFW_CURSOR_DISABLED` | kursor schowany i zatrzymany w oknie. `glfwGetCursorPos` zwraca wtedy pozycję wirtualną, która nie jest ograniczona ekranem. Tryb do sterowania kamerą myszą |

Przełączanie trybu jest w jednym miejscu, `Input::setCursorCaptured`:

```cpp
    // GLFW_CURSOR_DISABLED hides the cursor and gives unlimited virtual movement,
    // GLFW_CURSOR_NORMAL is the ordinary visible cursor.
    glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    // Raw motion skips the system's pointer acceleration, which suits mouse look.
    // Not every platform has it, and it only has an effect while the cursor is disabled.
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }
```

Jak to czytać:

- **Surowy ruch myszy** to dane prosto z urządzenia, bez przyspieszenia i skalowania, które
  system nakłada na zwykły kursor. Ten sam ruch ręki daje wtedy zawsze ten sam obrót kamery.
- Surowy ruch działa tylko w trybie `GLFW_CURSOR_DISABLED` i nie na każdej platformie. W GLFW
  3.4 `glfwRawMouseMotionSupported()` zwraca prawdę na Windowsie, a fałsz na macOS. Dlatego
  pytamy przed włączeniem: `glfwSetInputMode` z `GLFW_RAW_MOUSE_MOTION` na platformie bez
  wsparcia zgłasza błąd GLFW.
- Zmiana trybu kursora może sprawić, że pozycja zwracana przez `glfwGetCursorPos` odskoczy
  (przejście między pozycją prawdziwą a wirtualną). `Input` po każdej zmianie trybu zgłasza w
  następnej klatce zerowe przesunięcie.
- Kursor przechwytuje kamera: `NightMazeApp::onRender` woła `setCursorCaptured(true)` po
  kliknięciu lewym przyciskiem w scenę, a `Application::run` woła `setCursorCaptured(false)`
  po Escape. Między jednym a drugim program działa w trybie `GLFW_CURSOR_DISABLED` i mysz
  obraca kamerę
  ([`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcja
  5.3).

`Input.hpp` zna liczbę przycisków tak samo jak liczbę klawiszy: przez gołą liczbę 7 sprawdzaną
w `Input.cpp`:

```cpp
    static_assert(MOUSE_BUTTON_COUNT == GLFW_MOUSE_BUTTON_LAST + 1,
                  "MOUSE_BUTTON_COUNT must cover every GLFW mouse button");
```

Pełny opis: [`../modules/core/input.md`](../modules/core/input.md), sekcje 5.8 do 5.10.

### 3.11. `GLFW_INCLUDE_NONE`

Nagłówek `GLFW/glfw3.h` domyślnie sam dołącza systemowy nagłówek OpenGL (na macOS
`OpenGL/gl.h`, na Windowsie `GL/gl.h`). My chcemy, żeby deklaracje OpenGL pochodziły wyłącznie
z GLAD. Makro `GLFW_INCLUDE_NONE` mówi GLFW: nie dołączaj żadnego nagłówka OpenGL.

Definiujemy je raz, w CMake, a nie przez `#define` w każdym pliku
([`CMakeLists.txt`](../../CMakeLists.txt)):

```cmake
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE      # GLFW must not include an OpenGL header, GLAD provides it
    GL_SILENCE_DEPRECATION # macOS marks all of OpenGL as deprecated
)
```

`PUBLIC` sprawia, że definicję dostaje `engine` i wszystko, co go linkuje (`night_maze`).
W linii poleceń kompilatora widać to jako `-DGLFW_INCLUDE_NONE`. Dzięki temu nie da się o
makrze zapomnieć w nowym pliku, a kolejność dołączania GLAD i GLFW przestaje być krytyczna.
Mimo to w `Window.cpp` zachowujemy konwencję "GLAD pierwszy":

```cpp
// GLAD first: it declares the OpenGL API. GLFW_INCLUDE_NONE (set in CMake) stops GLFW
// from including the system OpenGL header on its own.
#include <glad/gl.h>

#include <GLFW/glfw3.h>
```

Drugie makro, `GL_SILENCE_DEPRECATION`, nie jest częścią GLFW. Wycisza ostrzeżenia Apple,
które oznaczyło całe OpenGL jako przestarzałe.

## 4. Pułapki

1. **Hinty po utworzeniu okna.** `glfwWindowHint` po `glfwCreateWindow` nie działa na
   istniejące okno. Objaw na Macu: `GL_VERSION: 2.1 ...` i błędy kompilacji shaderów.
2. **Funkcje `gl*` przed `glfwMakeContextCurrent` lub przed `gladLoadGL`.** Wskaźniki funkcji
   są jeszcze puste, program kończy się naruszeniem ochrony pamięci.
3. **`glViewport` z rozmiarem okna.** Na Retinie obraz w ćwiartce okna. Zawsze
   `glfwGetFramebufferSize`.
4. **Brak `glfwPollEvents` w pętli.** Okno się zawiesza, system pokazuje "program nie
   odpowiada", klawisze nie reagują.
5. **Pomylenie `isKeyDown` z `wasKeyPressed`.** Przełącznik na `isKeyDown` miga co klatkę.
6. **`glfwPollEvents` potrafi zablokować pętlę.** Na macOS podczas przeciągania lub zmiany
   rozmiaru okna funkcja nie wraca, dopóki użytkownik nie puści myszy. Następna klatka ma
   wtedy bardzo duży czas trwania. Dlatego `core::Time` przycina go do `MAX_FRAME_TIME`
   (patrz [`../modules/core/main-loop.md`](../modules/core/main-loop.md)).
7. **Vsync wyłączony przez sterownik.** Jeśli FPS w panelu jest dużo wyższy niż odświeżanie
   monitora, sterownik (zwłaszcza na Windowsie) może wymuszać własne ustawienie i ignorować
   `glfwSwapInterval(1)`.
8. **GLFW tylko z wątku głównego.** Większość funkcji GLFW (tworzenie okna, zdarzenia) wolno
   wołać tylko z wątku, w którym działa `main`. macOS egzekwuje to twardo.
9. **Własne callbacki a ImGui.** Backend ImGui instaluje swoje callbacki GLFW. Jeśli kiedyś
   dodamy własne przez `glfwSet...Callback` po utworzeniu `DebugUI`, nadpiszemy callbacki
   ImGui i panele przestaną reagować. Szczegóły w [`imgui.md`](imgui.md). Odpytywanie przez
   `glfwGetKey`, `glfwGetMouseButton` i `glfwGetCursorPos` nie ma tego problemu.
10. **Stary tutorial, stary hint.** Poradniki pisane pod GLFW 3.3 owijają
    `GLFW_OPENGL_FORWARD_COMPAT` w `#ifdef __APPLE__`. U nas hint jest ustawiany zawsze, na
    obu systemach, celowo: żeby Windows zachowywał się tak samo jak macOS.
11. **Skok pozycji kursora po zmianie trybu.** Po `glfwSetInputMode(..., GLFW_CURSOR, ...)`
    pozycja z `glfwGetCursorPos` może odskoczyć. Przesunięcie liczone w tej klatce byłoby
    jednym wielkim szarpnięciem, dlatego `Input` je zeruje (sekcja 3.10).
12. **Pozycja myszy to nie piksele.** `glfwGetCursorPos` zwraca współrzędne ekranu, czyli
    jednostki `glfwGetWindowSize`, nie `glfwGetFramebufferSize`. Na Retinie różnią się
    dwukrotnie (sekcja 3.8).
13. **`GLFW_RAW_MOUSE_MOTION` bez sprawdzenia wsparcia.** Na platformie bez surowego ruchu
    (macOS w GLFW 3.4) `glfwSetInputMode` zgłasza błąd przez callback błędów. Najpierw
    `glfwRawMouseMotionSupported()`.

## 5. Pytania kontrolne

1. **Za co odpowiada GLFW, a za co nie?**
   Okno, kontekst OpenGL i wejście. Nie rysuje i nie ładuje funkcji OpenGL: rysuje sterownik
   przez funkcje `gl*`, a ich adresy ładuje GLAD.

2. **Po co hinty `GLFW_CONTEXT_VERSION_*` i `GLFW_OPENGL_PROFILE` i co się stanie bez nich na
   macOS?**
   Proszą o kontekst 4.1 Core. Bez nich macOS da stary kontekst 2.1, bo nowsze wersje
   udostępnia tylko w profilu Core, o który trzeba jawnie poprosić.

3. **Co znaczy, że kontekst jest "bieżący" i dlaczego `glfwMakeContextCurrent` stoi przed
   `gladLoadGL`?**
   Funkcje OpenGL działają na kontekście przypisanym do wątku. GLAD pyta sterownik o adresy
   funkcji dla bieżącego kontekstu, więc kontekst musi już być ustawiony.

4. **Czym różni się `glfwPollEvents` od `glfwSwapBuffers`?**
   Pierwsza odbiera zdarzenia od systemu i aktualizuje stan wejścia, druga pokazuje
   narysowaną klatkę przez zamianę bufora tylnego z przednim. Pierwsza na początku klatki,
   druga na końcu.

5. **Dlaczego `glViewport` dostaje wynik `glfwGetFramebufferSize`, a nie `glfwGetWindowSize`?**
   Viewport jest w pikselach. Na ekranie Retina framebuffer ma dwa razy więcej pikseli w
   każdym wymiarze niż okno ma jednostek ekranowych.

6. **Co robi `glfwSwapInterval(1)`?**
   Włącza vsync: zamiana buforów czeka na jedno odświeżenie ekranu, co ogranicza FPS do
   częstotliwości monitora i usuwa rozrywanie obrazu.

7. **Jak `core::Input` odróżnia "klawisz trzymany" od "klawisz właśnie wciśnięty"?**
   Raz na klatkę zapisuje stan wszystkich klawiszy z `glfwGetKey` i pamięta stan z poprzedniej
   klatki. "Właśnie wciśnięty" to: teraz wciśnięty i poprzednio nie.

8. **Jak `core::Input` czyta mysz i czym są tryby kursora?**
   Raz na klatkę pyta `glfwGetMouseButton` o każdy przycisk i `glfwGetCursorPos` o pozycję,
   a przesunięcie liczy jako różnicę pozycji z dwóch kolejnych klatek. Tryb
   `GLFW_CURSOR_NORMAL` to zwykły kursor, `GLFW_CURSOR_DISABLED` chowa go i daje wirtualną
   pozycję bez ograniczeń (sterowanie kamerą). Surowy ruch myszy włączamy tylko po
   sprawdzeniu `glfwRawMouseMotionSupported()`.

9. **Po co w `Dependencies.cmake` para `get_target_property` / `set_target_properties` z
   `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`?**
   Oznacza katalog nagłówków GLFW jako systemowy (`-isystem`), żeby cudze nagłówki nie
   generowały ostrzeżeń przy naszych ostrych flagach. Opcja `SYSTEM` w `FetchContent_Declare`
   wymaga CMake 3.25, a nasze minimum to 3.24.

## 6. Oficjalna dokumentacja

- Dokumentacja GLFW (wprowadzenie, przewodniki, referencja): <https://www.glfw.org/docs/latest/>
- Repozytorium GLFW: <https://github.com/glfw/glfw>
- LearnOpenGL, rozdział "Creating a window": <https://learnopengl.com/Getting-started/Creating-a-window>
  (uwaga: pokazuje GLAD 1, różnice opisuje [`glad.md`](glad.md))
- Dokumentacja CMake (moduł FetchContent, właściwości targetów): <https://cmake.org/cmake/help/latest/>
- Kopia dokumentacji dokładnie dla naszej wersji leży po pierwszej konfiguracji w
  `build/debug/_deps/glfw-src/docs/` (pliki `window.md`, `context.md`, `input.md`).
