# Dear ImGui 1.92.9b (gałąź docking)

Dokument biblioteki dla kamienia milowego M0. Opisuje użycie Dear ImGui w
[`src/debug/DebugUI.cpp`](../../src/debug/DebugUI.cpp),
[`src/debug/panels/RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp) i
[`src/debug/panels/ShadersPanel.cpp`](../../src/debug/panels/ShadersPanel.cpp) oraz
konfigurację z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake). API stylu i
czcionek, którego używa motyw paneli ([`src/debug/Theme.cpp`](../../src/debug/Theme.cpp)),
jest w sekcji 3.12. Widżety, które doszły w M4 razem z panelem Lights
([`src/debug/panels/LightsPanel.cpp`](../../src/debug/panels/LightsPanel.cpp)): zwijane
nagłówki, zakres z dwóch pól, suwak logarytmiczny i okno, które startuje zwinięte, są w
sekcji 3.13.

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
            for (gfx::Shader* shader : shaders) {
                shader->reload();
            }
        }
```

`shaders` to lista pięciu programów gry: jeden przycisk przeładowuje wszystkie.

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
target_link_libraries(night_maze PRIVATE engine game_logic imgui)
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

    // Platform backend: feeds GLFW input and window size into ImGui.
    // true = install GLFW callbacks (ImGui chains to callbacks that were set before).
    ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true);
    // Renderer backend: draws ImGui with OpenGL. The string is the GLSL version of its shaders.
    ImGui_ImplOpenGL3_Init("#version 410");

    // The look of the panels. The content scale is 1 at 100 % display scaling and 1.5 at
    // 150 % on Windows: the theme multiplies its sizes and the font by it. On macOS the
    // function returns 1, because a Retina display is handled by the framebuffer being
    // larger than the window, and ImGui follows that on its own.
    applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()));
    loadFont(m_fontBytes);
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
- `ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true)` uruchamia backend platformy dla
  naszego `GLFWwindow*`. Drugi argument (`install_callbacks`) równy `true` oznacza: zainstaluj
  w GLFW callbacki ImGui (klawisze, znaki, przyciski i ruch myszy, kółko, fokus okna).
  Backend zapamiętuje callbacki ustawione wcześniej i w każdym swoim callbacku najpierw woła
  ten poprzedni, a dopiero potem przekazuje zdarzenie do ImGui (łańcuchowanie).
- `ImGui_ImplOpenGL3_Init("#version 410")` uruchamia backend renderera.
- `applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()))` ustawia wygląd
  paneli: tabelę kolorów, odstępy i rozmiar czcionki, przemnożone przez skalę ekranu. Wcześniej
  stało tu `ImGui::StyleColorsDark()`, czyli standardowy ciemny styl. Funkcja jest nasza
  (`src/debug/Theme.cpp`), a API, z którego korzysta, opisuje sekcja 3.12.
- `loadFont(m_fontBytes)` wczytuje czcionkę paneli z pliku w `assets/fonts`. Też nasza
  funkcja, też sekcja 3.12.

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
        drawRendererPanel(context.time, context.window, context.clearColor, context.lighting.mode);

        // The Shaders panel takes a list, so that a new program is one more entry here
        // and no change in the panel. The array holds pointers, because a reference
        // cannot be an element of an array.
        constexpr int SHADER_COUNT = 5;
        const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
            &context.shader, &context.texturedShader, &context.colorShader, &context.litShader,
            &context.gouraudShader};
        drawShadersPanel(shaders);

        drawCameraPanel(context.camera, context.player, context.mouseSensitivity);
        drawMazePanel(context.mazeSettings, context.mazeWorld, context.player, context.camera);
        drawCollisionPanel(context.mazeWorld, context.player, context.drawColliders);
        drawAssetsPanel(context.assets, context.viewMode, context.lighting.normalMapping);
        drawLightsPanel(context.lighting, context.mazeWorld);
    }

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

Parametr `context` to struktura `debug::DebugContext` z
[`src/debug/DebugContext.hpp`](../../src/debug/DebugContext.hpp): referencje do danych, które
panele pokazują i edytują (siedemnaście pól: od `time` i `window` po `gouraudShader` i
`lighting`). Buduje ją co klatkę `main.cpp`.
Opis struktury jest w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.2.

Cztery etapy, zawsze w tej kolejności:

| Etap | Wywołania | Co się dzieje |
|---|---|---|
| 1. Początek klatki | `ImGui_ImplOpenGL3_NewFrame()`, `ImGui_ImplGlfw_NewFrame()`, `ImGui::NewFrame()` | backend renderera przygotowuje swoje zasoby (przy pierwszym użyciu tworzy shadery), backend platformy przekazuje rozmiar okna, skalę framebuffera, czas i stan myszy, a rdzeń zaczyna nową klatkę |
| 2. Widżety | `DockSpaceOverViewport`, a potem siedem funkcji paneli. Każda woła (przez naszą funkcję `placePanelOnFirstUse`) `SetNextWindowPos`, `SetNextWindowSize` i `SetNextWindowCollapsed`, potem `Begin`, swoje widżety i `End`: `drawRendererPanel` (`Text`, `ColorEdit3`, `Combo`), `drawShadersPanel` (`Button`, `Text`, `TextWrapped`, `SetItemTooltip`), `drawCameraPanel` (`DragFloat3`, `SliderFloat`), `drawMazePanel` (`SliderInt`, `InputScalar`, `Button`, lista rysowania), `drawCollisionPanel` (`Checkbox`), `drawAssetsPanel` (`Combo`, `Checkbox`, `SliderFloat`, `Image`), `drawLightsPanel` (`ColorEdit3`, `CollapsingHeader`, `SliderFloat`, `Checkbox`, `DragFloatRange2`). Widżety paneli z M2 + M3: sekcja 3.11, widżety panelu Lights: sekcja 3.13 | opisujemy interfejs, ImGui od razu odpowiada na interakcje i zbiera geometrię |
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
              .texturedShader = texturedShader(),
              .colorShader = colorShader(),
              .player = player(),
              .mazeSettings = mazeSettings(),
              .mazeWorld = mazeWorld(),
              .assets = assets(),
              .viewMode = viewMode(),
              .drawColliders = drawColliders(),
              .litShader = litShader(),
              .gouraudShader = gouraudShader(),
              .lighting = lighting(),
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

  Najpierw gra rysuje swoją klatkę (tło, labirynt, przy włączonym oświetleniu znaczniki
  świateł punktowych, kostkę i na życzenie linie pudełek kolizji: `glViewport`, `glEnable`, `glClearColor`, `glClear` i wywołania `glDrawElements` w
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
  [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6.3.

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
        ...
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
poziomą kreskę. Drugie wielokropki w kodzie wyżej zastępują listę `Lighting`, czyli
`ImGui::Combo` z trybem oświetlenia (sekcja 3.11).

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
myszy, więc widać przez niego to, co narysował OpenGL (labirynt).

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
- Układ domyślny naszych siedmiu paneli ustawiają trójki `SetNextWindowPos`,
  `SetNextWindowSize` i `SetNextWindowCollapsed` z warunkiem `ImGuiCond_FirstUseEver`
  (sekcje 3.11 i 3.13). Ten warunek działa tylko dla okna, którego w `imgui.ini` jeszcze
  nie ma. Stary plik zatrzyma panele na starych miejscach i w starych rozmiarach: plik
  sprzed M4 ma panel Camera w lewej kolumnie, tam gdzie dziś staje panel Lights. Tabela
  pozycji: [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.7.
- Dla każdego okna plik trzyma pozycję, rozmiar, stan zwinięcia (linia `Collapsed=`) i
  dane dokowania. Panel Camera, który startuje zwinięty, po rozwinięciu zostaje więc
  rozwinięty przy następnych uruchomieniach. Stanu zwijanych nagłówków wewnątrz okna
  (`CollapsingHeader`) w pliku nie ma.
- W pliku nie ma wyglądu: kolory, odstępy i czcionkę ustawia kod przy każdym starcie.

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
wywołanie w `DebugUI::draw` obok pozostałych siedmiu funkcji `draw...Panel`, dopisanie plików
do `add_executable` w `CMakeLists.txt`. Przed `Begin` wywołanie `placePanelOnFirstUse` z nową
stałą dopisaną w `src/debug/PanelLayout.hpp`, żeby panel przy pierwszym uruchomieniu nie
przykrył innych. Nowe dane dla panelu to dodatkowo jedno pole w `debug::DebugContext` i
jedna linia w `main.cpp`. Pełna instrukcja krok po kroku jest w
[`../modules/debug-ui.md`](../modules/debug-ui.md) i tam należy jej szukać.

### 3.11. Widżety paneli Maze, Collision i Assets

Panele z M2 + M3 używają kilkunastu funkcji ImGui, których wcześniej w projekcie nie było.
Każdy fragment niżej jest skopiowany z pliku podanego w tabeli.

**Pozycja i rozmiar na pierwsze uruchomienie.** Wszystkie siedem paneli woła przed `Begin`
jedną naszą funkcję, na przykład `placePanelOnFirstUse(RENDERER_PLACEMENT);`. Trzy wywołania
ImGui są w niej ([`PanelLayout.cpp`](../../src/debug/PanelLayout.cpp)):

```cpp
    ImGui::SetNextWindowPos(panelCorner, ImGuiCond_FirstUseEver, placement.corner);
    ImGui::SetNextWindowSize({placement.size.x * layoutScale, placement.size.y * layoutScale},
                             ImGuiCond_FirstUseEver);
    // A folded panel shows only its title bar. The size above is the one it opens to.
    ImGui::SetNextWindowCollapsed(placement.collapsed, ImGuiCond_FirstUseEver);
```

Trzecie wywołanie doszło w M4 i jest opisane w sekcji 3.13.

Funkcje `SetNextWindow...` dotyczą okna, które otworzy najbliższe `Begin`. Drugi argument to
warunek: `ImGuiCond_FirstUseEver` stosuje wartość tylko wtedy, gdy ImGui nie ma dla tego okna
danych w `imgui.ini`. Bez warunku (`ImGuiCond_Always`) panel wracałby na miejsce w każdej
klatce i nie dałoby się go przesunąć. Trzeci argument `SetNextWindowPos` to **pivot**: punkt
okna, który ma trafić w podaną pozycję, zapisany liczbami od 0 do 1 (`(0, 0)` lewy górny róg,
`(1, 1)` prawy dolny, `(0.5, 0.5)` środek). Dzięki niemu panel przyczepiony do prawej krawędzi
ustawia się swoim prawym rogiem, bez odejmowania szerokości. Pozycję liczymy od rogu głównego
viewportu, `ImGui::GetMainViewport()` (pola `WorkPos` i `WorkSize`), czyli od rogu okna
programu. Opis całej funkcji: [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.7.

**Widżety edytujące wartość przez wskaźnik.** Wszystkie działają tak jak `ColorEdit3`: dostają
adres zmiennej, pokazują jej wartość i zapisują nową, gdy użytkownik coś zmieni. Zwracają
`true` w klatce, w której wartość się zmieniła.

| Funkcja | Przykład z kodu | Plik | Co trzeba wiedzieć |
|---|---|---|---|
| `SliderInt` | `ImGui::SliderInt("Width", &settings.width, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells", ImGuiSliderFlags_AlwaysClamp);` | `MazePanel.cpp` | suwak liczby całkowitej. Format jak w `printf`. `AlwaysClamp` przycina także wartość wpisaną z klawiatury (Ctrl i kliknięcie) |
| `InputScalar` | `ImGui::InputScalar("Seed", ImGuiDataType_U32, &settings.seed, &SEED_STEP);` | `MazePanel.cpp` | pole liczbowe dowolnego typu. Typ nazywa drugi argument i **musi** zgadzać się ze zmienną (tu `std::uint32_t`), bo funkcja dostaje `void*` i kompilator tego nie sprawdzi. Czwarty argument to wskaźnik na krok przycisków plus i minus |
| `Checkbox` | `ImGui::Checkbox("Draw collision boxes", &drawColliders);` | `CollisionPanel.cpp`, a od map normalnych także `AssetsPanel.cpp` (`ImGui::Checkbox("Normal mapping", &normalMapping);`, pole `game::LightingSettings::normalMapping`) | pole wyboru na zmiennej `bool` |
| `Combo` | `ImGui::Combo("View mode", &viewModeIndex, VIEW_MODE_ITEMS)` | `AssetsPanel.cpp`, od M4 także `RendererPanel.cpp` (`ImGui::Combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS)`) | lista rozwijana. Pracuje na **numerze** wybranej pozycji (`int`), nie na wyliczeniu, więc kod rzutuje `enum class` na `int` i z powrotem. Zwraca `true` w klatce, w której użytkownik wybrał inną pozycję |
| `SliderFloat` w bloku `BeginDisabled` | `ImGui::BeginDisabled(!anisotropySupported);` ... `ImGui::EndDisabled();` | `AssetsPanel.cpp` | wszystko między tą parą jest wyszarzone i nie reaguje, gdy argument jest prawdą. `EndDisabled` woła się **zawsze**, tak jak `End` |

Lista pozycji dla `Combo` to jeden napis, w którym każda pozycja kończy się znakiem zerowym:

```cpp
constexpr const char* VIEW_MODE_ITEMS = "Textured\0Normals as colour\0UVs as colour\0";
constexpr const char* FILTER_ITEMS = "Nearest\0Bilinear\0Trilinear\0";
```

Zwykły napis w C kończy się na pierwszym zerze, więc ImGui czyta dalej, aż trafi na **dwa**
zera z rzędu: ostatnie jawne `\0` i zero, które kompilator dopisuje na końcu każdego literału.
Kolejność pozycji jest taka jak kolejność wartości wyliczeń `game::ViewMode` i
`gfx::TextureFilter`, bo numer pozycji staje się wartością wyliczenia.

Tak samo zbudowana jest trzecia lista projektu, `Lighting` w panelu Renderer
([`RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp)):

```cpp
constexpr const char* LIGHTING_MODE_ITEMS = "Unlit\0Gouraud\0Phong\0Blinn-Phong\0";
```

```cpp
        int lightingModeIndex = static_cast<int>(lightingMode);
        if (ImGui::Combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS)) {
            lightingMode = static_cast<game::LightingMode>(lightingModeIndex);
        }
```

Deklaracja tej postaci `Combo` w `imgui.h` nazywa parametr wprost
`items_separated_by_zeros` i mówi w komentarzu to samo: pozycje oddzielone znakiem `\0`,
lista zakończona `\0\0`. Biblioteka ma jeszcze dwie postaci tej funkcji (z tablicą napisów
i z funkcją podającą napis o danym numerze), których projekt nie używa. Kolejność pozycji
odpowiada wyliczeniu `game::LightingMode` (`Unlit = 0`, `Gouraud`, `Phong`, `BlinnPhong`).
Opis linia po linii: [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.3.

**Teksty i układ.**

| Funkcja | Gdzie | Co robi |
|---|---|---|
| `SeparatorText("Models")` | `AssetsPanel.cpp` | pozioma kreska z podpisem: nagłówek części panelu |
| `TextUnformatted(text)` | `AssetsPanel.cpp`, `CollisionPanel.cpp` | tekst bez formatowania. Bezpieczny dla napisów, które mogą zawierać znak `%` (nazwy plików). Panel Shaders osiąga to samo inaczej: tekst sterownika podaje jako argument formatu, `ImGui::TextWrapped("%s", shader.lastError().c_str())`, nigdy jako sam format |
| `SetItemTooltip("%s", fullPath.c_str())` | `AssetsPanel.cpp`, a w `ShadersPanel.cpp` z dwiema ścieżkami: `ImGui::SetItemTooltip("%s\n%s", vertexFullPath.c_str(), fragmentFullPath.c_str())` | podpowiedź dla **poprzedniego** widżetu, pokazywana po najechaniu kursorem |
| `SameLine()` | `MazePanel.cpp` | następny widżet staje w tej samej linii (przyciski `Regenerate` i `Random seed` obok siebie) |
| `PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR)` i `PopStyleColor()` | `AssetsPanel.cpp`, `ShadersPanel.cpp` | zmiana koloru tekstu dla widżetów między tą parą. Każde `Push` musi mieć swoje `Pop`. Stała jest jedna, w `Theme.hpp` (sekcja 3.12) |

**Obrazek z tekstury OpenGL** ([`AssetsPanel.cpp`](../../src/debug/panels/AssetsPanel.cpp)):

```cpp
        const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
        ImGui::Image(textureId, {PREVIEW_SIZE, PREVIEW_SIZE}, {0.0F, 1.0F}, {1.0F, 0.0F});
```

| Argument | Znaczenie |
|---|---|
| `textureId` | ImGui nie zna typów OpenGL. Tekstura to dla niego liczba, którą odda backendowi renderera, a backend OpenGL3 traktuje ją jako identyfikator obiektu tekstury i woła z nią `glBindTexture`. Rzutowanie tylko poszerza `GLuint` do typu liczbowego `ImTextureID` |
| `{PREVIEW_SIZE, PREVIEW_SIZE}` | rozmiar obrazka w panelu: 128 na 128 jednostek, niezależnie od rozmiaru tekstury |
| `{0.0F, 1.0F}` (`uv0`) | współrzędna tekstury **lewego górnego** rogu obrazka |
| `{1.0F, 0.0F}` (`uv1`) | współrzędna tekstury **prawego dolnego** rogu |

Wartości domyślne to `uv0 = (0, 0)` i `uv1 = (1, 1)`: ImGui zakłada, że `v = 0` to górny
wiersz obrazu. W naszych teksturach `v = 0` to wiersz **dolny** (tak wczytuje je
`assets::loadImage`, bo tak liczy OpenGL), więc z wartościami domyślnymi podgląd byłby do góry
nogami. Zamiana `v` w obu rogach odwraca go z powrotem. Na Windowsie podglądy są sprawdzone
na zrzucie ekranu (2026-10-05): stoją poprawnie.

**Sampler backendu.** Backend OpenGL3 w naszej wersji (1.92.9b) ma dwa własne obiekty
samplerów i przed rysowaniem wiąże pierwszy z nich z jednostką 0
(`build/debug/_deps/imgui-src/backends/imgui_impl_opengl3.cpp`, w funkcji tworzącej obiekty
urządzenia):

```cpp
            GL_CALL(glSamplerParameteri(bd->TexSamplers[sampler_n], GL_TEXTURE_MIN_FILTER, (sampler_n == 0) ? GL_LINEAR : GL_NEAREST));
            GL_CALL(glSamplerParameteri(bd->TexSamplers[sampler_n], GL_TEXTURE_MAG_FILTER, (sampler_n == 0) ? GL_LINEAR : GL_NEAREST));
            GL_CALL(glSamplerParameteri(bd->TexSamplers[sampler_n], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_CALL(glSamplerParameteri(bd->TexSamplers[sampler_n], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
```

Sampler związany z jednostką wygrywa z parametrami tekstury, a nasz sampler (ten z
`gfx::Texture2D`) jest związany tylko wtedy, gdy teksturę wiąże `Texture2D::bind`. Skutek:
wszystko, co rysuje ImGui, także nasze tekstury w `ImGui::Image`, jest czytane z filtrem
liniowym, bez mipmap, z zawijaniem `GL_CLAMP_TO_EDGE`. Podgląd w panelu Assets **nie reaguje**
na filtr i anizotropię wybrane w tym samym panelu: ich skutek widać w scenie. Po narysowaniu
backend przywraca sampler, który był związany wcześniej, tak jak resztę stanu.

Od map normalnych lista `Textures` ma cztery pozycje zamiast dwóch. Mapy normalnych są dla
pamięci podręcznej teksturami jak każde inne, więc ta sama linia `ImGui::Image` pokazuje je
bez żadnej zmiany w kodzie: jako jasnoniebieskie obrazki, bo większość tekseli to kolor
`(128, 128, 255)`, czyli kierunek "prosto z powierzchni"
([`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 2.4).

**Rysowanie własnych kształtów: lista rysowania**
([`MazePanel.cpp`](../../src/debug/panels/MazePanel.cpp), plan labiryntu):

| Funkcja | Co robi |
|---|---|
| `ImGui::GetContentRegionAvail()` | ile miejsca zostało w panelu od kursora do prawego i dolnego brzegu. Z szerokości liczona jest skala planu |
| `ImGui::GetCursorScreenPos()` | miejsce, w którym ImGui postawiłoby następny widżet, we współrzędnych ekranu. To lewy górny róg planu |
| `ImGui::GetWindowDrawList()` | lista rysowania bieżącego okna (`ImDrawList*`). Kształty dodane do niej są rysowane razem z panelem i przycinane do niego |
| `drawList->AddLine(p1, p2, color)` | odcinek między dwoma punktami ekranu. Tak rysowana jest każda ściana i kreska kierunku patrzenia |
| `drawList->AddCircleFilled(center, radius, color)` | wypełnione koło: kropka gracza |
| `IM_COL32(r, g, b, a)` | makro składające kolor z czterech liczb od 0 do 255 w jedną liczbę `ImU32` |
| `ImGui::Dummy(size)` | niewidzialny widżet o podanym rozmiarze. Lista rysowania nie przesuwa kursora, więc bez `Dummy` panel nie wiedziałby, że plan zajmuje miejsce: następny widżet stanąłby na planie, a przewijanie liczyłoby złą wysokość |

Współrzędne w liście rysowania to współrzędne **ekranu** ImGui (piksele okna programu, y w
dół), a nie współrzędne wewnątrz panelu. Dlatego plan zaczyna od `GetCursorScreenPos()` i do
każdego punktu dodaje ten początek. Jak punkt świata zamienia się na punkt planu, opisuje
[`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 6.

Stan sprawdzenia: wygląd planu, podglądów i list jest sprawdzony na zrzutach ekranu z
Windowsa (2026-10-05). Samych kontrolek (kliknięcia w `Combo`, suwaki, pola wyboru, przyciski)
nikt jeszcze ręcznie nie sprawdzał, także listy `Lighting` z M4. Na macOS panele nie były
uruchamiane.

### 3.12. Styl i czcionki w wersji 1.92

Kod: [`src/debug/Theme.cpp`](../../src/debug/Theme.cpp). Jak z tego API powstaje motyw
projektu (paleta, kontrast, układ), opisuje [`../modules/debug-ui.md`](../modules/debug-ui.md),
sekcje 5.7 i 5.8. Tutaj jest samo API, w kształcie z pobranej wersji `1.92.9b`. W wersji
1.92 czcionki zostały przebudowane, więc przykłady ze starszych poradników wyglądają inaczej.

**Styl: `ImGuiStyle`.** Jedna struktura na kontekst, zwracana przez `ImGui::GetStyle()`.

| Element API | Użycie w projekcie | Co robi |
|---|---|---|
| `ImGuiStyle& style = ImGui::GetStyle();` | `applyTheme` | referencja do stylu bieżącego kontekstu |
| `ImGui::StyleColorsDark(&style);` | `applyTheme` | wypełnia tabelę kolorów standardowym ciemnym zestawem. Bez argumentu działa na stylu bieżącego kontekstu |
| `style.Colors[ImGuiCol_...]` | `applyColors` | tabela kolorów: tablica `ImVec4` (czerwony, zielony, niebieski, alfa od 0 do 1), indeksowana wyliczeniem `ImGuiCol_`. W tej wersji 63 pozycje |
| `style.WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing` | `applyMetrics` | odstępy jako `ImVec2` (poziomo, pionowo) |
| `style.WindowRounding`, `FrameRounding`, `GrabRounding` i pokrewne | `applyMetrics` | promienie zaokrągleń |
| `style.DisabledAlpha` | `applyMetrics` | mnożnik przezroczystości dla wszystkiego między `BeginDisabled` a `EndDisabled` |
| `style.ScaleAllSizes(scale)` | `applyTheme` | mnoży wszystkie odstępy, zaokrąglenia i grubości przez `scale` i obcina do pełnych pikseli. Nie zmienia czcionki. Stratne, więc woła się raz na świeżych wartościach |
| `style.FontSizeBase` | `applyTheme` | **nowe w 1.92:** wysokość tekstu przed skalowaniem. Wcześniej rozmiar podawało się przy wczytywaniu czcionki |
| `style.FontScaleDpi` | `applyTheme`, odczyt w `placePanelOnFirstUse` | **nowe w 1.92:** mnożnik tekstu od gęstości ekranu. Ostateczna wysokość to `FontSizeBase * FontScaleMain * FontScaleDpi` |
| `ImGui::PushStyleColor(ImGuiCol_Text, kolor)` i `ImGui::PopStyleColor()` | panele Shaders i Assets | zmiana jednej pozycji tabeli na czas kilku widżetów. Każde `Push` musi mieć `Pop` |
| `ImGui::GetColorU32(const ImVec4&)` | plan w panelu Maze | zamienia kolor z czterech `float` na jedną liczbę 32-bitową dla listy rysowania i uwzględnia przezroczystość ze stylu |

Typy `ImVec2` i `ImVec4` mają konstruktory `constexpr`, więc kolory i odstępy mogą być
stałymi `constexpr` (tak jest w `Theme.cpp` i `PanelLayout.hpp`).

**Skala ekranu.** Dokument `docs/FONTS.md` w źródłach biblioteki podaje dla wersji 1.92
przepis w trzech zdaniach: ustaw `style.FontScaleDpi` na skalę zawartości, zawołaj
`style.ScaleAllSizes`, a skalę framebuffera w stylu macOS backend obsługuje sam. Skalę daje
funkcja backendu GLFW:

```cpp
    applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()));
```

`ImGui_ImplGlfw_GetContentScaleForWindow` woła `glfwGetWindowContentScale` i zwraca skalę
osi x, ale na platformach Apple (oraz w przeglądarce i na Androidzie) zwraca zawsze 1, bo tam
gęstość ekranu jest już w stosunku framebuffera do okna. Dzięki temu ten sam kod jest
poprawny na Windowsie (skala 1,5 przy 150%) i na Macu (skala 1, Retina obsłużona niżej).
Pole `io.ConfigDpiScaleFonts`, które aktualizuje skalę po przeniesieniu okna na inny
monitor, jest w nagłówku oznaczone jako eksperymentalne i skaluje tylko czcionkę: nie
używamy go. Ten komputer ma ekran w skali 100% (systemowe DPI 96), więc funkcja daje tu 1,
a gałąź 150% jest sprawdzona tylko z wartością podstawioną na próbę.

**Czcionki: `ImFontAtlas`.** Atlas czcionek jest polem `ImGui::GetIO().Fonts`.

| Element API | Użycie | Co trzeba wiedzieć |
|---|---|---|
| `fonts->AddFontFromMemoryTTF(dane, rozmiar, rozmiarPikseli, &config)` | `loadFont` | rejestruje czcionkę z bajtów w pamięci. `rozmiarPikseli` równe 0 znaczy "użyj `style.FontSizeBase`". Zwraca `ImFont*` albo `nullptr`, gdy dane nie są czcionką. Ma asercję, że danych jest więcej niż 100 bajtów |
| `ImFontConfig config; config.FontDataOwnedByAtlas = false;` | `loadFont` | domyślnie (`true`) atlas przejmuje wskaźnik i zwalnia go własnym alokatorem. Z `false` dane zostają nasze, ale od wersji 1.92 muszą istnieć tak długo jak atlas |
| `fonts->AddFontFromFileTTF(nazwa, ...)` | nieużywane | czyta plik samo. Na Windowsie otwiera go poprawnie także przy nazwie w UTF-8 (`ImFileOpen` w `imgui.cpp` zamienia ją na znaki szerokie i woła `_wfopen`). Przy braku pliku domyślnie wywołuje asercję `Could not load font file!` (wyłącza ją flaga `ImFontFlags_NoLoadError` w polu `ImFontConfig::Flags`, opisanym w nagłówku jako jeszcze nie do użytku), dlatego projekt czyta plik sam |
| `fonts->AddFontDefaultVector()` | `loadFont`, gałąź awaryjna | wbudowana czcionka skalowalna (ProggyForever). `AddFontDefaultBitmap()` to dawna czcionka 13 pikseli (ProggyClean), a `AddFontDefault()` wybiera między nimi według rozmiaru |
| zakresy znaków (`glyph_ranges`, `GetGlyphRanges...`) | nieużywane | przed 1.92 trzeba było wymienić znaki do wczytania. Teraz znak jest rysowany do tekstury przy pierwszym użyciu, a funkcje `GetGlyphRanges...` są oznaczone jako przestarzałe |

Dynamiczne czcionki wymagają, żeby backend renderera ustawił flagę
`ImGuiBackendFlags_RendererHasTextures`. Backend OpenGL3 w naszej wersji to robi (w
`imgui_impl_opengl3.cpp`, w funkcji `ImGui_ImplOpenGL3_Init`): potrafi tworzyć i powiększać
teksturę czcionki w trakcie działania.

Kolejność ma znaczenie: czcionki dodaje się po `ImGui::CreateContext()` i przed pierwszym
`ImGui::NewFrame()`. Gdy atlas jest pusty, ImGui samo dodaje czcionkę wbudowaną.

### 3.13. Widżety panelu Lights i okno, które startuje zwinięte

Kod: [`src/debug/panels/LightsPanel.cpp`](../../src/debug/panels/LightsPanel.cpp) i
[`src/debug/PanelLayout.cpp`](../../src/debug/PanelLayout.cpp). Co każda kontrolka panelu
zmienia w oświetleniu, opisuje
[`../modules/scene/lights.md`](../modules/scene/lights.md), sekcja 6. Tutaj jest samo API.
Deklaracje i komentarze przytaczam z `build/debug/_deps/imgui-src/imgui.h` w naszej wersji.

**`SetNextWindowCollapsed`: okno zwinięte do paska tytułu.**

```cpp
    // A folded panel shows only its title bar. The size above is the one it opens to.
    ImGui::SetNextWindowCollapsed(placement.collapsed, ImGuiCond_FirstUseEver);
```

| Element | Znaczenie |
|---|---|
| deklaracja | `void SetNextWindowCollapsed(bool collapsed, ImGuiCond cond = 0);` z komentarzem "set next window collapsed state. call before Begin()" |
| `placement.collapsed` | `true` zwija okno do paska tytułu, `false` zostawia je rozwinięte. W projekcie `true` ma tylko panel Camera (`CAMERA_PLACEMENT` w `PanelLayout.hpp`) |
| `ImGuiCond_FirstUseEver` | ten sam warunek co przy pozycji i rozmiarze: tylko gdy okno nie ma wpisu w `imgui.ini`. Bez warunku panel zwijałby się z powrotem w każdej klatce i nie dałoby się go otworzyć |

Zwinięte okno to zwykły stan okna ImGui, nie osobny widżet. Użytkownik przełącza go
strzałką po lewej stronie paska tytułu. Dla zwiniętego okna `Begin` zwraca `false`, więc
kod panelu pomija zawartość, a `End` woła jak zawsze (sekcja 3.5). Stan trafia do
`imgui.ini` jako linia `Collapsed=0` albo `Collapsed=1` we wpisie okna (sekcja 3.7).
Rozmiar ustawiony przez `SetNextWindowSize` zostaje zapamiętany i jest rozmiarem po
rozwinięciu. Wysokość samego paska to wysokość czcionki plus dwa razy `FramePadding.y`
(w `imgui.cpp`: `TitleBarHeight = g.FontSize + g.Style.FramePadding.y * 2.0f`), u nas przy
skali 100% 16 + 2 * 3 = 22 jednostki.

**`CollapsingHeader`: zwijana grupa wewnątrz okna.** Panel Lights dzieli kontrolki na cztery
grupy. Początki dwóch z nich:

```cpp
    if (!ImGui::CollapsingHeader("Moon (directional)")) {
        return;
    }
```

```cpp
    // DefaultOpen: the group is open the first time the program runs.
    if (!ImGui::CollapsingHeader("Flashlight (spot)", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
```

| Element | Znaczenie |
|---|---|
| deklaracja | `bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);` z komentarzem "if returning 'true' the header is open. doesn't indent nor push on ID stack. user doesn't have to call TreePop()" |
| wartość zwracana | `true`, gdy grupa jest rozwinięta. Kod rysuje wtedy jej widżety. Każda grupa jest u nas osobną funkcją (`drawMoon`, `drawFlashlight`, `drawPointLights`, `drawHighlight`), więc zamiast `if (...) { ... }` stoi odwrócony warunek i wczesne `return` |
| brak pary `End` | inaczej niż `Begin` i `BeginDisabled`, nagłówek nie ma wywołania zamykającego. Nie ma więc czego zapomnieć przy wczesnym `return` |
| `ImGuiTreeNodeFlags_DefaultOpen` | grupa jest rozwinięta, dopóki użytkownik jej nie zwinie (w `imgui.h`: "Default node to be open"). Bez flagi (wartość domyślna `0`) grupa startuje zwinięta: tak jest z `Moon (directional)` |
| kolory | pasek nagłówka bierze kolory `ImGuiCol_Header`, `ImGuiCol_HeaderHovered` i `ImGuiCol_HeaderActive`, te same co pozycje rozwiniętej listy `Combo`. W motywie projektu to `SLATE_LIGHT`, `EMBER` i `EMBER_BRIGHT` ([`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8.2) |

Dwie rzeczy wynikają z komentarza przy deklaracji. Po pierwsze, nagłówek nie wcina
zawartości: widżety grupy stoją w tej samej kolumnie co nagłówek. Po drugie, nagłówek
**nie dokłada niczego do stosu identyfikatorów** (ID stack). Identyfikator widżetu powstaje
z jego etykiety i z tego, co jest na stosie, więc dwa widżety o tej samej etykiecie w dwóch
różnych grupach jednego okna miałyby ten sam identyfikator (pułapka 3). Dlatego etykiety w
panelu Lights nie powtarzają się między grupami: `Moon colour`, `Beam colour` i
`Point colour`, a nie trzy razy `Colour`, tak samo `Moon intensity`, `Beam intensity` i
`Point intensity`.

Stan rozwinięcia nagłówka ImGui trzyma w pamięci okna do końca działania programu. Do
`imgui.ini` nie trafia (we wpisie okna są tylko pozycja, rozmiar, zwinięcie całego okna i
dokowanie), więc po każdym starcie obowiązuje to, co mówi flaga w kodzie.

**`ColorEdit3` na wektorze GLM.**

```cpp
    // ColorEdit3 reads and writes three floats through the pointer. value_ptr gives the
    // address of the three floats of a glm::vec3.
    ImGui::ColorEdit3("Moon colour", glm::value_ptr(lighting.moonColor));
```

To ten sam widżet co `Clear color` w panelu Renderer (sekcja 1). `ColorEdit3` chce wskaźnika
na trzy kolejne liczby `float`. Tam kolor jest tablicą `std::array<float, 3>` i wskaźnik daje
`.data()`. Tutaj kolor jest wektorem `glm::vec3`, a wskaźnik na jego pierwszą składową daje
`glm::value_ptr` z nagłówka `<glm/gtc/type_ptr.hpp>` ([`glm.md`](glm.md), sekcja 3.9).
Składowe x, y, z wektora ImGui pokazuje jako czerwony, zielony i niebieski. Panel woła ten
widżet cztery razy: `Ambient`, `Moon colour`, `Beam colour` i `Point colour`.

**`DragFloatRange2`: dwa pola, jeden zakres.**

```cpp
    ImGui::DragFloatRange2("Cone", &lighting.flashlightInnerDegrees,
                           &lighting.flashlightOuterDegrees, CONE_DRAG_SPEED, MIN_CONE_DEGREES,
                           MAX_CONE_DEGREES, "inner %.1f deg", "outer %.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
```

Deklaracja: `bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const char* format_max = NULL, ImGuiSliderFlags flags = 0);`

| Argument | Wartość w projekcie | Znaczenie |
|---|---|---|
| `label` | `"Cone"` | etykieta po prawej stronie obu pól |
| `v_current_min` | `&lighting.flashlightInnerDegrees` | adres dolnej wartości zakresu: połówkowy kąt wewnętrznego stożka latarki |
| `v_current_max` | `&lighting.flashlightOuterDegrees` | adres górnej wartości: połówkowy kąt zewnętrznego stożka |
| `v_speed` | `CONE_DRAG_SPEED` (`0.1F`) | o ile zmienia się wartość na jeden piksel przeciągania: 0,1 stopnia |
| `v_min`, `v_max` | `MIN_CONE_DEGREES` (`1.0F`), `MAX_CONE_DEGREES` (`60.0F`) | granice całego zakresu |
| `format`, `format_max` | `"inner %.1f deg"`, `"outer %.1f deg"` | tekst w pierwszym i w drugim polu. Format jak w `printf`, więc słowo przed `%` jest zwykłym tekstem i podpisuje pole |
| `flags` | `ImGuiSliderFlags_AlwaysClamp` | wartość wpisana z klawiatury też jest przycinana do granic |

To nie suwak, tylko dwa pola przeciągane (drag): wartość zmienia się przez przeciąganie
myszą w lewo i w prawo po polu, a dwuklik (albo Ctrl i kliknięcie) pozwala wpisać liczbę.
W źródle (`imgui_widgets.cpp`) widżet to dwa wywołania `DragScalar` w jednej linii, z
granicami zależnymi od siebie: górną granicą pierwszego pola jest mniejsza z liczb `v_max`
i bieżącej drugiej wartości, a dolną granicą drugiego pola większa z liczb `v_min` i
bieżącej pierwszej wartości. Skutek: pierwsza wartość nigdy nie przekroczy drugiej, czyli
wewnętrzny stożek nie zrobi się szerszy od zewnętrznego. Obie mogą być równe.

**`ImGuiSliderFlags_Logarithmic`: suwak z gęstszym początkiem.**

```cpp
    // Logarithmic: half of the slider covers the small exponents, where one step changes
    // the size of the highlight the most.
    ImGui::SliderFloat("Shininess", &lighting.shininess, MIN_SHININESS, MAX_SHININESS, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
```

Zwykły suwak dzieli zakres liniowo: środek suwaka od 1 do 256 to 128,5. Z flagą
`Logarithmic` równym odcinkom suwaka odpowiadają równe **ilorazy** wartości, więc środek
wypada w okolicy pierwiastka z iloczynu końców, czyli 16 (`sqrt(1 * 256)`). Małe wykładniki,
przy których rozmiar połysku zmienia się najszybciej, dostają połowę długości suwaka. Dwie
flagi łączy bitowe "lub" (`|`), bo każda jest jednym bitem maski. Tę samą parę flag ma suwak
`Near plane` w panelu Camera. Komentarz przy fladze w `imgui.h` radzi rozważyć jeszcze
`ImGuiSliderFlags_NoRoundToFormat`, gdy format ma mało cyfr: u nas format to `"%.0f"`, a tej
flagi nie ma, więc wartość wykładnika jest zaokrąglana do liczby całkowitej. Dokładnego
położenia wartości na suwaku nie mierzyłem: liczba 16 to rachunek z definicji skali
logarytmicznej.

**Pozostałe widżety panelu** są znane z wcześniejszych paneli: `SliderFloat` z formatem i
flagą `ImGuiSliderFlags_AlwaysClamp` (dziewięć razy, na przykład `"%.0f deg"` dla kątów
księżyca i `"%.1f m"` dla zasięgów), `Checkbox("Flashlight on (key F)", &lighting.flashlightOn)`
na zmiennej `bool` i `Text` z liczbą świateł punktowych.

Stan sprawdzenia: skutki wartości domyślnych oświetlenia (widok startowy, cztery tryby
oświetlenia, scena z wyłączoną latarką) są obejrzane na zrzutach ekranu z Windowsa
(2026-10-05). Żadnego z tych widżetów nikt jeszcze nie klikał ani nie przeciągał,
tak samo jak strzałki rozwijającej panel Camera. Opis zachowania pochodzi z kodu projektu i
ze źródeł biblioteki. Na macOS kod M4 nie był budowany ani uruchamiany.

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
    framebuffera. Panele dwa razy za duże oznaczałyby, że do motywu trafiła skala z
    `glfwGetWindowContentScale` zamiast z `ImGui_ImplGlfw_GetContentScaleForWindow`
    (sekcja 3.12).
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
    suwak ma flagę `ImGuiSliderFlags_AlwaysClamp` (tak jak wszystkie suwaki paneli Camera
    i Lights).
16. **Ta sama etykieta w dwóch grupach `CollapsingHeader`.** Nagłówek nie dokłada niczego do
    stosu identyfikatorów, więc `Colour` w grupie księżyca i `Colour` w grupie latarki to
    dla ImGui ten sam widżet (pułapka 3). Stąd pełne etykiety w panelu Lights (sekcja 3.13).
17. **`SetNextWindowCollapsed` bez warunku.** Wywołanie z domyślnym `cond = 0` działa w
    każdej klatce: okno byłoby zwijane od nowa i nie dałoby się go otworzyć. Projekt podaje
    `ImGuiCond_FirstUseEver`. Druga strona tego warunku: zmiana `.collapsed` w kodzie nie
    ma skutku, dopóki w `imgui.ini` leży wpis okna.
18. **Lista `Combo` w innej kolejności niż wyliczenie.** Numer pozycji jest rzutowany na
    `enum class`, więc napis z pozycjami i wyliczenie muszą mieć tę samą kolejność.
    Kompilator tego nie sprawdza ([`../modules/debug-ui.md`](../modules/debug-ui.md),
    pułapka 30).
14. **Czcionka z pamięci zwolniona dwa razy.** `AddFontFromMemoryTTF` domyślnie przejmuje
    wskaźnik. Dane z `std::vector` wymagają `FontDataOwnedByAtlas = false` i muszą żyć tak
    długo jak kontekst ImGui (sekcja 3.12).
15. **Przykłady czcionek sprzed 1.92.** Rozmiar w pikselach przy wczytywaniu, zakresy znaków
    i `io.FontGlobalScale` to stary sposób. W naszej wersji rozmiar i skala są w stylu, a
    zakresy są zbędne.

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

11. **Jak w ImGui 1.92 ustawia się rozmiar tekstu i skalę dla ekranu 150%?**
    Rozmiar to pole `style.FontSizeBase`, a skala to `style.FontScaleDpi` dla czcionki i
    `style.ScaleAllSizes(scale)` dla odstępów. Skalę daje
    `ImGui_ImplGlfw_GetContentScaleForWindow`. Na macOS zwraca ona 1, bo Retinę obsługuje
    skala framebuffera.

12. **Dlaczego przy wczytywaniu czcionki nie podajemy zakresu polskich znaków?**
    Od wersji 1.92 backend z flagą `ImGuiBackendFlags_RendererHasTextures` rysuje znak do
    tekstury czcionki przy pierwszym użyciu, więc zakresy są zbędne. Wystarczy, że plik
    czcionki ma te znaki.

13. **Jak sprawić, żeby okno ImGui startowało zwinięte, i gdzie ten stan jest pamiętany?**
    Przed `Begin` zawołać `ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver)`.
    U nas robi to `placePanelOnFirstUse` dla panelu Camera. Warunek sprawia, że wartość
    działa tylko przy braku wpisu okna w `imgui.ini`, a potem stan zapisuje ImGui w linii
    `Collapsed=`. Dla zwiniętego okna `Begin` zwraca `false`.

14. **Czym `CollapsingHeader` różni się od zwiniętego okna i co robi
    `ImGuiTreeNodeFlags_DefaultOpen`?**
    To zwijana grupa widżetów wewnątrz okna: zwraca `true`, gdy jest rozwinięta, nie ma
    wywołania zamykającego, nie wcina zawartości i nie dokłada niczego do stosu
    identyfikatorów. `DefaultOpen` sprawia, że grupa startuje rozwinięta. Stan grupy nie
    jest zapisywany w `imgui.ini`, więc flaga decyduje przy każdym uruchomieniu.

15. **Co gwarantuje `DragFloatRange2` w kontrolce `Cone` panelu Lights?**
    Że pierwsza wartość (kąt wewnętrznego stożka) nie przekroczy drugiej (kąta
    zewnętrznego): granice obu pól zależą od bieżącej wartości drugiego pola. Obie są też
    trzymane między `MIN_CONE_DEGREES` a `MAX_CONE_DEGREES`, także przy wpisywaniu z
    klawiatury, dzięki `ImGuiSliderFlags_AlwaysClamp`.

16. **Jak `ColorEdit3` edytuje `glm::vec3`?**
    Dostaje `glm::value_ptr(wektor)`, czyli wskaźnik na pierwszą z trzech liczb `float`
    wektora, i przez niego czyta i zapisuje czerwony, zielony i niebieski. To ten sam
    mechanizm co `clearColor.data()` dla tablicy.

## 6. Oficjalna dokumentacja

- Repozytorium Dear ImGui (README, `docs/`, przykłady w `examples/`): <https://github.com/ocornut/imgui>
- Wiki projektu (Getting Started, Docking, FAQ): <https://github.com/ocornut/imgui/wiki>
- Dokumentacja CMake (FetchContent, `add_library`): <https://cmake.org/cmake/help/latest/>
- Czcionki, skala ekranu i własność danych czcionki w naszej wersji:
  `build/debug/_deps/imgui-src/docs/FONTS.md`.
- Najlepsze źródło dla naszej wersji leży lokalnie po pierwszej konfiguracji:
  `build/debug/_deps/imgui-src/imgui.h` (komentarze przy każdej funkcji),
  `build/debug/_deps/imgui-src/imgui_demo.cpp` (przykład użycia każdego widżetu),
  `build/debug/_deps/imgui-src/docs/FAQ.md` oraz
  `build/debug/_deps/imgui-src/examples/example_glfw_opengl3/main.cpp` (wzorcowa integracja,
  z której pochodzi nasz cykl klatki).
