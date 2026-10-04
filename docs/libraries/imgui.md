# Dear ImGui 1.92.9b (gałąź docking)

Dokument biblioteki dla kamienia milowego M0. Opisuje użycie Dear ImGui w
[`src/debug/DebugUI.cpp`](../../src/debug/DebugUI.cpp),
[`src/debug/panels/RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp) i
[`src/debug/panels/ShadersPanel.cpp`](../../src/debug/panels/ShadersPanel.cpp) oraz
konfigurację z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake).

Architekturę modułu `debug` i instrukcję "jak dodać nowy panel" zawiera
[`../modules/debug-ui.md`](../modules/debug-ui.md). Tutaj jest sama biblioteka.

## 1. Czym jest Dear ImGui

Dear ImGui to biblioteka C++ do budowania interfejsu narzędziowego: paneli, suwaków,
pól wyboru koloru, wykresów. Powstała z myślą o narzędziach deweloperskich w grach. W Night
Maze służy do paneli debugowych: podgląd FPS, informacje o sterowniku, a od kolejnych
kamieni milowych przełączniki efektów pokazywane na obronie.

### Za co ImGui NIE odpowiada

- Nie tworzy okna systemowego i nie czyta klawiatury ani myszy. Robi to GLFW, a do ImGui
  dane trafiają przez backend platformy.
- Nie rysuje samodzielnie. Produkuje listy trójkątów, które na ekran wysyła backend
  renderera przez OpenGL.
- Nie przechowuje naszych danych. Wartość koloru tła żyje w `game::NightMazeApp`, ImGui dostaje
  tylko wskaźnik.
- Nie jest biblioteką do interfejsu samej gry (menu, HUD dla gracza). To narzędzie dla
  programisty.

### Model immediate mode a retained mode

To najważniejsze pojęcie w tym dokumencie.

**Tryb zachowany (retained mode)**, znany z Qt, WinForms czy HTML: raz tworzymy obiekt
przycisku, biblioteka go przechowuje, a my rejestrujemy callback na kliknięcie. Stan
interfejsu żyje w bibliotece i trzeba go synchronizować ze stanem programu.

**Tryb natychmiastowy (immediate mode)**: nie ma trwałych obiektów widżetów. Co klatkę
wołamy funkcje, które opisują interfejs od nowa. Jedno wywołanie jednocześnie rysuje widżet i
zwraca informację o interakcji:

```cpp
// retained mode (pseudokod, nie nasz kod)
button = new Button("Reload");
button->onClick = [] { reloadShaders(); };

// immediate mode
if (ImGui::Button("Reload")) {
    reloadShaders();
}
```

Prawdziwy odpowiednik tej drugiej postaci jest w
[`ShadersPanel.cpp`](../../src/debug/panels/ShadersPanel.cpp):

```cpp
        if (ImGui::Button("Reload shaders")) {
            shader.reload();
        }
```

Skutki praktyczne:

- Jeśli w danej klatce nie wywołamy funkcji panelu, panel nie istnieje. Ukrywanie interfejsu
  to zwykły `if`.
- Widżety pracują bezpośrednio na naszych zmiennych przez wskaźnik. Nie ma kopii stanu do
  synchronizowania.
- Kod interfejsu jest krótki i leży obok danych, które edytuje. Dlatego ImGui tak dobrze
  pasuje do paneli debugowych.

Przykład z [`RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp):

```cpp
        // ColorEdit3 reads and writes three floats through the pointer.
        ImGui::ColorEdit3("Clear color", clearColor.data());
```

`clearColor` to `std::array<float, 3>` z `game::NightMazeApp` (pole `m_clearColor`, udostępniane
przez metodę `clearColor()`, która zwraca referencję). Widżet czyta
trzy liczby, rysuje pole koloru i przy zmianie zapisuje nowe wartości w tym samym miejscu.
W następnej klatce `glClearColor` użyje już nowego koloru.

Uwaga: "immediate mode" dotyczy API, nie renderowania. ImGui nie rysuje każdego widżetu
osobnym wywołaniem OpenGL. Zbiera wszystko w buforach wierzchołków i wysyła w kilku
wywołaniach rysujących na koniec klatki. Trochę stanu wewnętrznego też ma (pozycje okien,
który widżet jest aktywny), ale nie musimy nim zarządzać.

## 2. Jak podpinamy ImGui w CMake

Fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
# ---- Dear ImGui (docking branch): debug panels ---------------------------------------
# ImGui ships without a CMake build, so only download it and define the target ourselves.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# The OpenGL3 backend uses the small GL loader bundled with ImGui, so it does not need GLAD.
# The GLFW backend needs the GLFW headers.
target_link_libraries(imgui PUBLIC glfw)
```

### Pobranie: `FetchContent_Declare` i `FetchContent_MakeAvailable`

Działa tak samo jak dla GLFW (opis mechanizmu w [`glfw.md`](glfw.md)): płytki klon
repozytorium w stanie z tagu trafia do `build/<preset>/_deps/imgui-src`.

Jest jedna istotna różnica. Repozytorium ImGui **nie ma pliku `CMakeLists.txt`**. Autor
zakłada, że pliki źródłowe dodaje się wprost do własnego projektu. `FetchContent_MakeAvailable`
w takiej sytuacji tylko pobiera kod i ustawia zmienną `imgui_SOURCE_DIR` ze ścieżką do niego.
Żaden target nie powstaje sam.

### Dlaczego target `imgui` definiujemy ręcznie

Bo nikt inny tego za nas nie zrobi. `add_library(imgui STATIC ...)` tworzy bibliotekę
statyczną (na Macu `build/debug/libimgui.a`) z sześciu plików:

| Plik | Rola |
|---|---|
| `imgui.cpp` | rdzeń: kontekst, okna, wejście, docking |
| `imgui_draw.cpp` | generowanie geometrii, czcionki |
| `imgui_tables.cpp` | tabele |
| `imgui_widgets.cpp` | widżety: przyciski, suwaki, `ColorEdit3` |
| `backends/imgui_impl_glfw.cpp` | backend platformy dla GLFW |
| `backends/imgui_impl_opengl3.cpp` | backend renderera dla OpenGL 3+ |

Zaleta ręcznej definicji: dokładnie widać, co kompilujemy, i wybieramy tylko te backendy,
których używamy (w katalogu `backends/` jest ich ponad dwadzieścia: Vulkan, DirectX, SDL i inne).

`target_include_directories(imgui SYSTEM PUBLIC ...)` udostępnia dwa katalogi nagłówków,
dzięki czemu w `DebugUI.cpp` piszemy `#include <imgui.h>` i `#include <imgui_impl_glfw.h>`
bez ścieżek. `SYSTEM` wycisza ostrzeżenia z nagłówków ImGui w naszym kodzie (w linii poleceń
kompilatora są podane przez `-isystem`). `PUBLIC` przekazuje te katalogi do `night_maze`.

`target_link_libraries(imgui PUBLIC glfw)`: plik `imgui_impl_glfw.cpp` dołącza
`<GLFW/glfw3.h>` i woła funkcje GLFW, więc target musi znać GLFW.

Podłączenie do programu w [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
target_link_libraries(night_maze PRIVATE engine imgui)
```

ImGui linkuje tylko `night_maze`, a nie `engine`. To odzwierciedla architekturę z PRD:
warstwa `debug` zależy od wszystkich, nic nie zależy od niej. Biblioteka `engine` pozostaje
wolna od ImGui i da się jej użyć w zadaniach laboratoryjnych bez paneli.

Funkcji `night_maze_enable_warnings` dla `imgui` nie wołamy. Cudzy kod kompiluje się z
domyślnymi ostrzeżeniami.

### Przypięta wersja: `v1.92.9b-docking`

ImGui ma dwie główne gałęzie: `master` i `docking`. Gałąź docking zawiera to samo co master
plus dokowanie okien i obsługę wielu viewportów. Wydania gałęzi docking mają tagi z
przyrostkiem `-docking`. Nasz tag odpowiada wersji `1.92.9b` (w `imgui.h`:
`#define IMGUI_VERSION "1.92.9b"`).

Dlaczego przypinamy do tagu, a nie do gałęzi `docking`:

- gałąź zmienia się co kilka dni, więc każda świeża konfiguracja mogłaby pobrać inny kod,
- API ImGui bywa zmieniane między wersjami (przykład w sekcji o dockingu), więc
  nieprzypięta wersja mogłaby zepsuć kompilację bez naszego udziału,
- Mac i Windows muszą budować dokładnie ten sam kod.

### Dlaczego `imgui_demo.cpp` nie jest kompilowany i jak go dodać

Plik `imgui_demo.cpp` zawiera `ImGui::ShowDemoWindow()`: okno demonstrujące prawie wszystkie
widżety. To najlepsza "żywa dokumentacja" ImGui, bo kod każdego przykładu można znaleźć w tym
pliku.

W M0 go nie kompilujemy, bo żaden nasz kod go nie woła, a jest to bardzo duży plik, który
wydłuża kompilację i powiększa program. Lista źródeł ma zawierać tylko to, czego używamy.

Żeby go włączyć (na przykład na czas nauki widżetów), potrzebne są dwie zmiany:

1. W `cmake/Dependencies.cmake` dopisać do `add_library(imgui STATIC ...)` linię
   `${imgui_SOURCE_DIR}/imgui_demo.cpp`.
2. W `DebugUI::draw`, wewnątrz bloku `if (m_visible)`, wywołać `ImGui::ShowDemoWindow();`.

Sama zmiana numer 2 bez numeru 1 kończy się błędem linkera (undefined symbol
`ImGui::ShowDemoWindow`), bo deklaracja jest w `imgui.h`, a definicja tylko w
`imgui_demo.cpp`. Źródło demo można też po prostu czytać w
`build/debug/_deps/imgui-src/imgui_demo.cpp` bez kompilowania go.

## 3. Najważniejsze API z przykładami z naszego kodu

### 3.1. Backend platformy i backend renderera

Rdzeń ImGui jest niezależny od systemu i od API graficznego. Do świata zewnętrznego łączą go
dwa backendy:

| | Backend platformy (platform backend) | Backend renderera (renderer backend) |
|---|---|---|
| Plik | `imgui_impl_glfw.cpp` | `imgui_impl_opengl3.cpp` |
| Kierunek | świat do ImGui | ImGui do ekranu |
| Zadanie | mysz, klawiatura, rozmiar okna, skala Retina, czas klatki, kursor, schowek | shadery, bufory wierzchołków, tekstura czcionki, wywołania rysujące |
| Prefiks funkcji | `ImGui_ImplGlfw_` | `ImGui_ImplOpenGL3_` |

Ten podział pozwala łączyć dowolną platformę z dowolnym rendererem. My mamy parę GLFW i
OpenGL 3+ ("OpenGL3" w nazwie oznacza nowoczesny OpenGL z shaderami, w tym nasze 4.1).

### 3.2. Inicjalizacja i zamknięcie

Konstruktor i destruktor z [`DebugUI.cpp`](../../src/debug/DebugUI.cpp):

```cpp
DebugUI::DebugUI(const core::Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // Platform backend: feeds GLFW input and window size into ImGui.
    // true = install GLFW callbacks (ImGui chains to callbacks that were set before).
    ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true);
    // Renderer backend: draws ImGui with OpenGL. The string is the GLSL version of its shaders.
    ImGui_ImplOpenGL3_Init("#version 410");
}

DebugUI::~DebugUI() {
    // Reverse order of initialization.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
```

Linia po linii:

- `IMGUI_CHECKVERSION()` sprawdza, czy nagłówek `imgui.h`, z którym kompilujemy nasz plik,
  pasuje do skompilowanej biblioteki (wersja i rozmiary struktur). Wykrywa pomieszanie wersji.
- `ImGui::CreateContext()` tworzy kontekst ImGui: obiekt z całym stanem biblioteki. To nie
  jest kontekst OpenGL, zbieżność nazw jest przypadkowa.
- `ImGui::GetIO()` zwraca strukturę `ImGuiIO`: konfigurację i dane wejścia/wyjścia.
  `ConfigFlags |= ImGuiConfigFlags_DockingEnable` włącza dokowanie. Bez tej flagi gałąź
  docking zachowuje się jak master.
- `ImGui::StyleColorsDark()` ustawia ciemny motyw.
- `ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true)` uruchamia backend platformy dla
  naszego `GLFWwindow*`. Drugi argument (`install_callbacks`) równy `true` oznacza: zainstaluj
  w GLFW callbacki ImGui (klawisze, znaki, przyciski i ruch myszy, kółko, fokus okna).
  Backend zapamiętuje callbacki ustawione wcześniej i w każdym swoim callbacku najpierw woła
  ten poprzedni, a dopiero potem przekazuje zdarzenie do ImGui (łańcuchowanie).
- `ImGui_ImplOpenGL3_Init("#version 410")` uruchamia backend renderera.

Destruktor zamyka wszystko w odwrotnej kolejności. `ImGui_ImplOpenGL3_Shutdown` usuwa obiekty
OpenGL (shader, bufory, teksturę czcionki), więc kontekst OpenGL musi jeszcze istnieć. To
dlatego `DebugUI` musi zostać zniszczony przed oknem. Gwarantuje to układ klas w
[`src/main.cpp`](../../src/main.cpp):

```cpp
class DebugNightMazeApp final : public game::NightMazeApp {
    ...
private:
    // Members are destroyed before base classes, so ImGui shuts down while the window
    // and its OpenGL context (owned by core::Application) still exist.
    debug::DebugUI m_debugUI{window()};
};
```

`m_debugUI` jest polem klasy `DebugNightMazeApp`, najbardziej pochodnej w łańcuchu
`core::Application`, `game::NightMazeApp`, `DebugNightMazeApp`. Okno należy do
`core::Application`, czyli do klasy bazowej. C++ konstruuje klasy bazowe przed polami klasy
pochodnej, a niszczy w odwrotnej kolejności: najpierw pola klasy pochodnej, potem klasy
bazowe. W chwili konstrukcji `m_debugUI` okno już istnieje (stąd wolno wywołać `window()`),
a w chwili jego destrukcji okno jeszcze istnieje.

Ten sam fragment pokazuje regułę warstw: `game::NightMazeApp` nic nie wie o ImGui, a
`main.cpp` jest jedynym miejscem, które łączy `game/` z `debug/`.

`DebugUI` to wzorzec RAII: konstruktor zdobywa zasób, destruktor go zwalnia, kopiowanie jest
zabronione (`= delete` w nagłówku).

### 3.3. Napis `"#version 410"`

Backend OpenGL3 ma własne, małe shadery GLSL do rysowania interfejsu. Napis przekazany do
`ImGui_ImplOpenGL3_Init` zostaje wklejony jako pierwsza linia tych shaderów i wybiera wersję
języka GLSL.

- Wersje GLSL odpowiadają wersjom OpenGL: OpenGL 4.1 to GLSL 4.10, zapis `#version 410`.
- Nasz kontekst to 4.1 Core, więc 410 jest najwyższą wersją, którą na pewno skompiluje macOS.
  To także wymaganie z PRD (strona 4: "ImGui backend z `#version 410`").
- Podanie zbyt wysokiej wersji (na przykład `#version 460`) działałoby na Windowsie, a na
  Macu shadery ImGui by się nie skompilowały i panele by zniknęły.
- Gdyby przekazać `nullptr`, backend sam wybrałby wersję domyślną (na Apple `#version 150`).
  Podajemy ją jawnie, żeby zachowanie było takie samo na obu systemach.

Nasze własne shadery będą zaczynać się od `#version 410 core`. Dla wersji 4.10 brak słowa
`core` oznacza to samo, bo profil Core jest domyślny.

### 3.4. Cykl klatki: NewFrame, widżety, Render, RenderDrawData

Metoda `DebugUI::draw` w całości:

```cpp
void DebugUI::draw(const DebugContext& context) {
    // An ImGui frame is started every frame, also when hidden, so that ImGui keeps
    // consuming input events and its internal timing stays correct.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_visible) {
        // An invisible dock area that covers the whole window, so panels can be docked to
        // its edges. PassthruCentralNode keeps the middle transparent: the scene shows through.
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        // Each panel gets exactly the members it needs, so its signature still shows
        // what it reads and what it edits.
        drawRendererPanel(context.time, context.window, context.clearColor);
        drawShadersPanel(context.shader);
        drawCameraPanel(context.camera, context.mouseSensitivity, context.moveSpeed);
    }

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

Parametr `context` to struktura `debug::DebugContext` z
[`src/debug/DebugContext.hpp`](../../src/debug/DebugContext.hpp): referencje do danych, które
panele pokazują i edytują (`time`, `window`, `clearColor`, `shader`, `camera`,
`mouseSensitivity`, `moveSpeed`). Buduje ją co klatkę `main.cpp`.
Opis struktury jest w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.2.

Cztery etapy, zawsze w tej kolejności:

| Etap | Wywołania | Co się dzieje |
|---|---|---|
| 1. Początek klatki | `ImGui_ImplOpenGL3_NewFrame()`, `ImGui_ImplGlfw_NewFrame()`, `ImGui::NewFrame()` | backend renderera przygotowuje swoje zasoby (przy pierwszym użyciu tworzy shadery), backend platformy przekazuje rozmiar okna, skalę framebuffera, czas i stan myszy, a rdzeń zaczyna nową klatkę |
| 2. Widżety | `DockSpaceOverViewport`, `drawRendererPanel` (czyli `Begin`, `Text`, `ColorEdit3`, `End`), `drawShadersPanel` (`Begin`, `Text`, `Button`, `TextWrapped`, `End`), `drawCameraPanel` (`Begin`, `TextWrapped`, `DragFloat3`, `SliderFloat`, `End`) | opisujemy interfejs, ImGui od razu odpowiada na interakcje i zbiera geometrię |
| 3. Zamknięcie klatki | `ImGui::Render()` | kończy klatkę i układa zebrane dane w listy rysowania (draw lists). Wbrew nazwie nie wywołuje OpenGL |
| 4. Rysowanie | `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())` | backend renderera wysyła listy do OpenGL: tu naprawdę pojawiają się piksele |

Ważne szczegóły:

- **Każdemu `NewFrame` musi odpowiadać `Render`** (albo `EndFrame`). Widżety wolno wołać
  tylko pomiędzy nimi.
- **Klatka ImGui zaczyna się zawsze, także gdy panele są ukryte.** `if (m_visible)` obejmuje
  tylko etap 2. Dzięki temu ImGui dalej odbiera zdarzenia wejścia i liczy czas, a po
  ponownym włączeniu paneli (klawisz `~`) nie ma skoku.
- **`draw` jest ostatnim rysowaniem w klatce.** Widać to w `DebugNightMazeApp::onRender` w
  [`src/main.cpp`](../../src/main.cpp):

  ```cpp
      void onRender(double alpha) override {
          game::NightMazeApp::onRender(alpha);

          // The key left of 1 (` and ~ on a US keyboard) shows or hides the debug panels.
          if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
              m_debugUI.toggleVisible();
          }
          // While the cursor is captured the mouse belongs to the camera. The hidden cursor
          // still has a position that moves with the mouse, so the panels must ignore it,
          // otherwise it would hover and click them unseen.
          m_debugUI.setMouseEnabled(!input().isCursorCaptured());
          // The context is rebuilt every frame: it only holds references, so it is cheap.
          m_debugUI.draw(debug::DebugContext{
              .time = time(),
              .window = window(),
              .clearColor = clearColor(),
              .shader = shader(),
              .camera = camera(),
              .mouseSensitivity = mouseSensitivity(),
              .moveSpeed = moveSpeed(),
          });

          // ImGui now knows whether it is using the keyboard (a text field is being edited
          // or a widget is active) and the mouse (the cursor is over a panel or a widget is
          // being dragged). Block each device for the game from the next frame on, so typing
          // does not trigger Escape, the panel toggle or player movement, and working with
          // a panel does not click or look around in the scene.
          input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
          input().setMouseBlocked(m_debugUI.wantsMouse());
      }
  ```

  Najpierw gra rysuje swoją klatkę (na razie tło i jedną kostkę: `glViewport`, `glEnable`,
  `glClearColor`, `glClear`, `glDrawElements` w
  [`NightMazeApp::onRender`](../../src/game/NightMazeApp.cpp)), a dopiero potem ImGui rysuje
  na tym, co już jest w buforze, więc panele są na wierzchu sceny. Zamiana buforów
  (`swapBuffers`) następuje później, w `Application::run`. Dwie ostatnie linie,
  `setKeyboardBlocked` i `setMouseBlocked`, nie rysują niczego: przekazują grze informację z
  ImGui o klawiaturze i o myszy (sekcja 3.8). Linia `setMouseEnabled` przed `draw` działa w
  drugą stronę: mówi ImGui, czy wolno mu używać myszy (też sekcja 3.8).
- Backend renderera na czas rysowania zmienia stan OpenGL (blending, scissor test, wyłączony
  test głębi), a po zakończeniu przywraca poprzedni. Scena 3D zależy od testu głębi, więc
  warto o tym pamiętać przy szukaniu błędów stanu: gra włącza test głębi co klatkę i nie
  polega na tym, że backend po sobie posprząta.
- **Widżet może wywołać kod, który woła OpenGL.** Przycisk "Reload shaders" kompiluje i
  linkuje shadery w etapie 2, przed `ImGui::Render()`. Jest to bezpieczne, bo w etapach 2 i 3
  ImGui nie woła OpenGL, a backend w etapie 4 sam ustawia swój program i przywraca
  poprzedni tylko wtedy, gdy ten jeszcze istnieje (sprawdza `glIsProgram`). Pełny opis:
  [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 6.3.

### 3.5. Reguła `Begin` / `End`

Panel "Renderer" z [`RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp):

```cpp
    // Begin returns false when the panel is collapsed or hidden behind another tab.
    // End must be called in both cases.
    if (ImGui::Begin("Renderer")) {
        ImGui::Text("FPS: %.1f", time.fps());
        ImGui::Text("Frame time: %.2f ms", time.frameTimeMs());
        ...
        ImGui::ColorEdit3("Clear color", clearColor.data());
    }
    ImGui::End();
```

- `ImGui::Begin("Renderer")` otwiera okno ImGui o tytule "Renderer". Wszystkie widżety aż do
  `End` trafiają do tego okna.
- `Begin` zwraca `false`, gdy okno jest zwinięte albo zasłonięte (na przykład jest
  nieaktywną zakładką w docku). Wtedy nie ma sensu budować zawartości, stąd `if`.
- **`ImGui::End()` wołamy zawsze, niezależnie od wyniku `Begin`.** Dlatego stoi **za**
  klamrą `if`, a nie w środku. To wyjątek: dla większości innych par (`BeginChild` jest
  drugim wyjątkiem, ale `BeginMenu`, `BeginTable`, `BeginPopup`, `BeginCombo`) odpowiednie
  `End...` woła się tylko wtedy, gdy `Begin...` zwróciło `true`.
- Tytuł okna jest jednocześnie jego identyfikatorem. Dwa wywołania `Begin` z tym samym
  tytułem dopisują do tego samego okna. Po tytule ImGui zapamiętuje też pozycję w `imgui.ini`.

Pozostałe widżety z panelu: `ImGui::Text` formatuje jak `printf`, `ImGui::TextWrapped` robi
to samo z zawijaniem długich linii (nazwa karty graficznej), `ImGui::Separator` rysuje
poziomą kreskę.

### 3.6. Docking: `DockSpaceOverViewport` i `PassthruCentralNode`

Dokowanie (docking) pozwala przeciągać panele do krawędzi okna, układać je obok siebie i
łączyć w zakładki. Wymaga trzech rzeczy, wszystkie mamy:

1. gałęzi docking (tag `v1.92.9b-docking`),
2. flagi `ImGuiConfigFlags_DockingEnable` ustawionej w konstruktorze,
3. miejsca, do którego można dokować:

```cpp
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);
```

`DockSpaceOverViewport` tworzy niewidoczny obszar dokowania (dock space) rozciągnięty na
cały główny viewport, czyli na całe nasze okno. Argumenty:

- `0`: identyfikator obszaru, zero znaczy "wybierz domyślny",
- `ImGui::GetMainViewport()`: viewport, który ma zostać pokryty (całe okno aplikacji),
- `ImGuiDockNodeFlags_PassthruCentralNode`: flaga węzła centralnego.

**Po co `PassthruCentralNode`.** Obszar dokowania ma węzeł centralny (central node): środek,
który zostaje po zadokowaniu paneli przy krawędziach. Domyślnie ImGui wypełniłoby go swoim
tłem i zasłoniło scenę. Z tą flagą pusty środek jest przezroczysty i przepuszcza zdarzenia
myszy, więc widać przez niego to, co narysował OpenGL (na razie kolor tła i kostkę, później labirynt).

Kolejność ma znaczenie: obszar dokowania tworzymy przed panelami, które mają się w nim
dokować.

### 3.7. `imgui.ini`

ImGui zapamiętuje układ interfejsu (pozycje i rozmiary okien, układ docków) w pliku
tekstowym `imgui.ini`. Nazwa pochodzi z pola `ImGuiIO::IniFilename`, którego nie zmieniamy.

- Ścieżka jest **względna do katalogu roboczego (working directory)** procesu, nie do pliku
  wykonywalnego. Uruchamiając `./build/debug/night_maze` z katalogu repozytorium, dostajemy
  `imgui.ini` w katalogu głównym repozytorium. Uruchamiając z IDE, plik trafi tam, gdzie IDE
  ustawi katalog roboczy.
- Plik jest zapisywany, gdy układ się zmienił (nie częściej niż co kilka sekund), oraz przy
  zamykaniu kontekstu.
- Jest wpisany do `.gitignore` (sekcja "Runtime files"), bo to lokalne ustawienie jednego
  komputera, a nie część projektu.
- Skasowanie pliku przywraca układ domyślny. To pierwsza rzecz do zrobienia, gdy panel
  "zniknął" albo wyjechał poza okno.

### 3.8. `WantCaptureKeyboard` i `WantCaptureMouse`

Ten sam klawisz i to samo kliknięcie widzą dwaj odbiorcy: ImGui i gra. ImGui informuje, czy
chce dane wejście dla siebie, przez dwa pola `ImGuiIO`:

- `ImGui::GetIO().WantCaptureMouse` ma wartość `true`, gdy kursor jest nad oknem ImGui albo
  trwa przeciąganie widżetu,
- `ImGui::GetIO().WantCaptureKeyboard` ma wartość `true`, gdy ImGui używa klawiatury: aktywny
  jest dowolny widżet (edycja pola, przeciąganie suwaka albo wartości, trzymany przycisk,
  przesuwane okno) albo otwarte jest okno modalne. Nie chodzi tylko o pola tekstowe, bo
  aktywny widżet może sam używać klawiszy (Escape anuluje, Tab przechodzi dalej).

Zasada: do ImGui wejście przekazujemy zawsze (robi to backend), a **gra powinna ignorować
wejście, gdy odpowiednia flaga jest ustawiona**.

**Klawiatura.** `core::Input` czyta klawisze bezpośrednio przez `glfwGetKey`, z
pominięciem ImGui, więc sam z siebie nie wie, że ktoś właśnie pisze w panelu. Flagę
przekazuje mu `main.cpp`. W [`src/debug/DebugUI.cpp`](../../src/debug/DebugUI.cpp) jest
funkcja, która ją odczytuje:

```cpp
bool DebugUI::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}
```

a w `DebugNightMazeApp::onRender`, zaraz po `draw`, linia, która ją przekazuje dalej:

```cpp
input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
```

Dopóki blokada jest ustawiona, `Input::isKeyDown` i `Input::wasKeyPressed` zwracają `false`
dla każdego klawisza. Dzięki temu Esc wciśnięty podczas wpisywania wartości w panelu anuluje
tylko edycję i nie zamyka programu, a klawisz `~` nie chowa paneli. `core/` nadal nie zna
ImGui: dostaje neutralną flagę "klawiatura zablokowana". Pełny opis, razem z opóźnieniem
blokady i tym, dlaczego po jej zdjęciu nie ma fałszywego "właśnie wciśnięty", jest w
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.6.

**Mysz.** Mechanizm jest ten sam. `core::Input` czyta mysz bezpośrednio przez
`glfwGetMouseButton` i `glfwGetCursorPos`, a flagę dostaje od `main.cpp`. W `DebugUI.cpp`:

```cpp
bool DebugUI::wantsMouse() const {
    const ImGuiIO& io = ImGui::GetIO();
    // With the mouse switched off ImGui still sets WantCaptureMouse while a button is
    // held down and the hidden cursor is at the position of a panel. Nothing in the panel
    // reacts, but the caller would block the mouse for the game, so the answer is no.
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouse) != 0) {
        return false;
    }
    return io.WantCaptureMouse;
}
```

a w `DebugNightMazeApp::onRender`, zaraz po linii blokady klawiatury:

```cpp
input().setMouseBlocked(m_debugUI.wantsMouse());
```

Dopóki blokada myszy jest ustawiona, `Input::isMouseButtonDown` i
`Input::wasMouseButtonPressed` zwracają `false`, a `Input::mouseDeltaX` i
`Input::mouseDeltaY` zwracają 0. Przezroczysty środek obszaru dokowania
(`PassthruCentralNode`, sekcja 3.6) nie liczy się jako okno ImGui pod kursorem, więc nad
sceną `WantCaptureMouse` jest fałszywe i mysz należy do gry. Odbiorcą blokady jest kamera:
bez niej kliknięcie w panel przechwytywałoby kursor, a przeciąganie suwaka byłoby dla gry
ruchem myszy. Opis po stronie `core`:
[`../modules/core/input.md`](../modules/core/input.md), sekcja 5.10. Warunek na początku
`wantsMouse()` wyjaśnia następny akapit.

**`ImGuiConfigFlags_NoMouse`: ImGui bez myszy.** Gdy kamera przechwyci kursor (tryb
`GLFW_CURSOR_DISABLED`), kursora nie widać, ale backend GLFW nadal przekazuje do ImGui jego
pozycję (wirtualną) i kliknięcia. W tym trybie pomija tylko zmianę kształtu kursora.
Niewidoczny kursor najeżdżałby więc na panele i klikał w nie. ImGui ma na to flagę
konfiguracyjną, którą w projekcie przestawia jedna funkcja `DebugUI`:

```cpp
void DebugUI::setMouseEnabled(bool enabled) {
    // ConfigFlags is a set of bits. With the NoMouse bit set, ImGui treats no panel as
    // being under the cursor when it starts a frame, so nothing is hovered or clicked.
    ImGuiIO& io = ImGui::GetIO();
    if (enabled) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
}
```

`main.cpp` woła ją co klatkę przed `draw`:
`m_debugUI.setMouseEnabled(!input().isCursorCaptured());`.

- Flaga jest czytana w `ImGui::NewFrame()`. Przy ustawionej fladze ImGui kasuje informację o
  oknie pod kursorem, więc żaden widżet nie jest najechany ani klikany. Działa to w tej
  samej klatce, w której flaga została ustawiona, bo `NewFrame` jest wołane w `draw`.
- Pozycja myszy nie jest kasowana, a `WantCaptureMouse` nie jest zerowane we wszystkich
  przypadkach: przycisk wciśnięty, gdy niewidoczny kursor jest nad panelem, ImGui nadal
  zgłasza jako swój. Dlatego `wantsMouse()` przy tej fladze zwraca `false` samo.
- `&= ~flaga` zdejmuje jeden bit, `|= flaga` go ustawia. Pozostałe flagi, w tym
  `ImGuiConfigFlags_DockingEnable`, zostają bez zmian.

Pełny opis, z kolejnością zdarzeń w klatce kliknięcia i w klatce z Escape:
[`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.6.

Flagi są aktualizowane w `ImGui::NewFrame()`, więc odczytane wcześniej w tej samej klatce
opisują stan z klatki poprzedniej. Dlatego `wantsKeyboard()` i `wantsMouse()` wołamy po
`draw`, a nie przed.
Samo `NewFrame` liczy flagę klawiatury na podstawie widżetu, który był aktywny po poprzedniej
klatce, więc kliknięcie w pole jest widoczne we fladze klatkę później. W praktyce to
wystarcza: człowiek nie zdąży nacisnąć klawisza w ciągu dwóch klatek od kliknięcia.

### 3.9. Backend OpenGL3 ma własny loader, nie używa GLAD

Plik `imgui_impl_opengl3.cpp` nie dołącza naszego `<glad/gl.h>`. Zawiera własny, okrojony
loader (`imgui_impl_opengl3_loader.h`, oparty na GL3W), który ładuje tylko te kilkadziesiąt
funkcji OpenGL, których backend potrzebuje.

Skutki:

- target `imgui` nie linkuje `glad` (w CMake jest tylko `target_link_libraries(imgui PUBLIC glfw)`),
- nie ma konfliktu: oba loadery pytają ten sam sterownik o adresy tych samych funkcji w tym
  samym kontekście, tylko zapisują je w osobnych zmiennych,
- ImGui można zaktualizować niezależnie od naszego GLAD i odwrotnie.

Warunek: w chwili `ImGui_ImplOpenGL3_Init` kontekst OpenGL musi istnieć i być bieżący.
Spełniamy go, bo `DebugUI` dostaje w konstruktorze gotowe `core::Window`.

### 3.10. Jak dodać nowy panel

Krótko: nowy plik w `src/debug/panels/`, funkcja `draw...Panel` z parą `Begin`/`End`,
wywołanie w `DebugUI::draw` obok `drawRendererPanel`, `drawShadersPanel` i `drawCameraPanel`, dopisanie plików do `add_executable`
w `CMakeLists.txt`. Nowe dane dla panelu to dodatkowo jedno pole w `debug::DebugContext` i
jedna linia w `main.cpp`. Pełna instrukcja krok po kroku jest w
[`../modules/debug-ui.md`](../modules/debug-ui.md) i tam należy jej szukać.

## 4. Pułapki

1. **`End()` wewnątrz `if (Begin(...))`.** Po zwinięciu okna `End` nie zostanie wywołane i
   ImGui zatrzyma program asercją o niezgodnej liczbie `Begin`/`End`.
2. **Widżety poza klatką.** Wywołanie `ImGui::Text` przed `NewFrame` lub po `Render` kończy
   się asercją.
3. **Dwa widżety z tą samą etykietą w jednym oknie.** Etykieta jest identyfikatorem, więc
   dwa przyciski "Reset" będą się mylić (reaguje tylko pierwszy). Rozwiązanie: przyrostek
   `##`, na przykład `"Reset##light"`, albo `PushID`/`PopID`.
4. **Zły napis wersji GLSL.** Wyższa wersja niż 410 zadziała na Windowsie i zepsuje Maca.
5. **Stare poradniki do dockingu.** W starszych wersjach ImGui sygnatura
   `DockSpaceOverViewport` zaczynała się od viewportu. W naszej wersji pierwszy argument to
   identyfikator (`dockspace_id`), dlatego wywołanie ma na początku `0`. Kod skopiowany ze
   starego przykładu nie skompiluje się.
6. **Własne callbacki GLFW ustawione po `DebugUI`.** `glfwSetKeyCallback` i podobne zastępują
   poprzedni callback. Ustawione po inicjalizacji ImGui wyrzucą callbacki backendu i panele
   przestaną reagować. Własne callbacki trzeba ustawić przed utworzeniem `DebugUI` (wtedy
   ImGui je złańcuchuje) albo pozostać przy odpytywaniu, jak robi `core::Input`.
7. **Zapomniany `imgui_demo.cpp`.** `ShowDemoWindow()` bez dopisania pliku do CMake daje błąd
   linkera, nie kompilatora.
8. **Niszczenie w złej kolejności.** Zamknięcie backendu OpenGL3 po zniszczeniu okna to
   wywołania OpenGL bez kontekstu. U nas chroni przed tym kolejność pól i klas.
9. **`imgui.ini` w dziwnym miejscu.** Układ paneli "nie zapamiętuje się", bo IDE uruchamia
   program z innym katalogiem roboczym niż terminal. To dwa różne pliki.
10. **Rozmyty interfejs na Retinie.** Gdyby panele były nieostre lub w złej skali, trzeba
    sprawdzić, czy `ImGui_ImplGlfw_NewFrame` jest wołane co klatkę: to ono przekazuje skalę
    framebuffera.
11. **Pytanie o mysz z pominięciem `core::Input`.** Blokada `WantCaptureMouse` działa tylko
    dla pytań zadanych przez `input()` (sekcja 3.8). Kod gry wołający bezpośrednio
    `glfwGetCursorPos` albo `glfwGetMouseButton` widziałby też mysz używaną przez panel.
12. **Klawisz gry "nie działa" przy aktywnym widżecie.** Gdy trwa edycja albo przeciąganie w
    panelu, gra nie widzi klawiatury (Esc, `~`). To skutek `WantCaptureKeyboard`, nie błąd.
13. **Ukryty kursor nadal "chodzi" po panelach.** Tryb `GLFW_CURSOR_DISABLED` nie odcina
    ImGui od myszy: backend przekazuje pozycję i kliknięcia jak zwykle. Program, który
    przechwytuje kursor, musi na ten czas ustawić `ImGuiConfigFlags_NoMouse`
    (`DebugUI::setMouseEnabled(false)`, sekcja 3.8). Suwak ImGui ma też drugą drogę wejścia,
    o której łatwo zapomnieć: Ctrl i kliknięcie pozwala wpisać liczbę spoza zakresu, chyba że
    suwak ma flagę `ImGuiSliderFlags_AlwaysClamp` (tak jak wszystkie suwaki panelu Camera).

## 5. Pytania kontrolne

1. **Czym różni się immediate mode od retained mode?**
   W retained mode biblioteka przechowuje obiekty widżetów, a my reagujemy przez callbacki.
   W immediate mode co klatkę opisujemy interfejs wywołaniami funkcji, a stan (wartości)
   żyje w naszych zmiennych.

2. **Wymień w kolejności etapy klatki ImGui w `DebugUI::draw`.**
   `ImGui_ImplOpenGL3_NewFrame`, `ImGui_ImplGlfw_NewFrame`, `ImGui::NewFrame`, widżety,
   `ImGui::Render`, `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())`.

3. **Co robi `ImGui::Render()` i czy rysuje na ekranie?**
   Kończy klatkę i przygotowuje listy rysowania. Nie rysuje: piksele pojawiają się dopiero w
   `ImGui_ImplOpenGL3_RenderDrawData`.

4. **Czym różni się backend platformy od backendu renderera?**
   Platformy (`imgui_impl_glfw`) dostarcza do ImGui wejście i informacje o oknie. Renderera
   (`imgui_impl_opengl3`) rysuje wynik przez API graficzne.

5. **Dlaczego do `ImGui_ImplOpenGL3_Init` przekazujemy `"#version 410"`?**
   To wersja GLSL shaderów backendu. 4.10 odpowiada OpenGL 4.1, najwyższej wersji na macOS.

6. **Dlaczego `ImGui::End()` stoi poza `if (ImGui::Begin(...))`?**
   Bo `End` trzeba wywołać zawsze, także gdy `Begin` zwróciło `false` (okno zwinięte).

7. **Co daje `ImGuiDockNodeFlags_PassthruCentralNode`?**
   Pusty środek obszaru dokowania jest przezroczysty i przepuszcza mysz, więc scena OpenGL
   jest widoczna pod panelami.

8. **Dlaczego target `imgui` definiujemy sami i dlaczego nie linkuje on GLAD?**
   Repozytorium ImGui nie ma `CMakeLists.txt`, więc FetchContent tylko pobiera źródła.
   GLAD nie jest potrzebny, bo backend OpenGL3 ma własny wbudowany loader.

9. **Co oznacza `ImGui::GetIO().WantCaptureKeyboard` i jak projekt z niego korzysta?**
   ImGui zgłasza tą flagą, że samo używa klawiatury (aktywny jest dowolny widżet albo okno
   modalne). `DebugUI::wantsKeyboard()` ją zwraca, a `main.cpp` po `draw` przekazuje do
   `input().setKeyboardBlocked(...)`. Przy blokadzie gra nie widzi żadnego klawisza, więc Esc
   w polu panelu nie zamyka programu. Odpowiednik dla myszy działa tak samo:
   `WantCaptureMouse` (kursor nad panelem albo trwające przeciąganie) wraca przez
   `DebugUI::wantsMouse()` i trafia do `input().setMouseBlocked(...)`.

10. **Do czego służy `ImGuiConfigFlags_NoMouse` i kiedy projekt ją ustawia?**
    Każe ImGui ignorować mysz: w `NewFrame` kasowana jest informacja o oknie pod kursorem,
    więc panele nie reagują. `main.cpp` ustawia ją przez `DebugUI::setMouseEnabled(false)`,
    gdy kamera przechwyciła kursor, bo backend GLFW przekazuje pozycję także ukrytego
    kursora. Po Escape flaga jest zdejmowana w tej samej klatce.

## 6. Oficjalna dokumentacja

- Repozytorium Dear ImGui (README, `docs/`, przykłady w `examples/`): <https://github.com/ocornut/imgui>
- Wiki projektu (Getting Started, Docking, FAQ): <https://github.com/ocornut/imgui/wiki>
- Dokumentacja CMake (FetchContent, `add_library`): <https://cmake.org/cmake/help/latest/>
- Najlepsze źródło dla naszej wersji leży lokalnie po pierwszej konfiguracji:
  `build/debug/_deps/imgui-src/imgui.h` (komentarze przy każdej funkcji),
  `build/debug/_deps/imgui-src/imgui_demo.cpp` (przykład użycia każdego widżetu),
  `build/debug/_deps/imgui-src/docs/FAQ.md` oraz
  `build/debug/_deps/imgui-src/examples/example_glfw_opengl3/main.cpp` (wzorcowa integracja,
  z której pochodzi nasz cykl klatki).
