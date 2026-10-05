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
sekcji 3.13. To, co doszło w M5: HUD gry jako okno ImGui, które jest tylko obrazem
([`src/debug/Hud.cpp`](../../src/debug/Hud.cpp)), ósmy panel Gameplay
([`src/debug/panels/GameplayPanel.cpp`](../../src/debug/panels/GameplayPanel.cpp)) i nowe
kształty na planie w panelu Maze, jest w sekcji 3.14. To, co doszło w drugiej części M6:
dziewiąty i dziesiąty panel, Terrain i Grass
([`src/debug/panels/TerrainPanel.cpp`](../../src/debug/panels/TerrainPanel.cpp),
[`src/debug/panels/GrassPanel.cpp`](../../src/debug/panels/GrassPanel.cpp)), i funkcja
`ImGui::GetFrameHeight()`, którą układ paneli i HUD mierzą wysokość paska tytułu, jest w
sekcji 3.15.

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
- Nie jest pomyślana jako biblioteka do interfejsu samej gry (menu, HUD dla gracza): to
  narzędzie dla programisty. Mały HUD z M5 (licznik kryształów, pasek baterii, karta
  wygranej) rysuję mimo to przez ImGui, bo biblioteka już jest w programie, a kilka linii
  tekstu i jeden pasek nie uzasadniają własnego renderera tekstu. Jak z okna narzędziowego
  zrobić okno, które niczego nie przyjmuje i tylko pokazuje, opisuje sekcja 3.14.

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

`shaders` to lista ośmiu programów gry (sześć do M6, od pierwszej części M7 także
`composite` i `preview`): jeden przycisk przeładowuje wszystkie.

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
        drawRendererPanel(context.time, context.window, context.clearColor, context.lighting.mode,
                          context.skybox);

        // The Shaders panel takes a list, so that a new program is one more entry here
        // and no change in the panel. The array holds pointers, because a reference
        // cannot be an element of an array.
        constexpr int SHADER_COUNT = 8;
        const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
            &context.texturedShader,  &context.colorShader,  &context.litShader,
            &context.gouraudShader,   &context.skyboxShader, &context.grassShader,
            &context.compositeShader, &context.previewShader};
        drawShadersPanel(shaders);

        drawCameraPanel(context.camera, context.player, context.mouseSensitivity);
        drawGameplayPanel(context.round, context.gameplay);
        drawTerrainPanel(context.terrain, context.mazeWorld.terrain);
        drawGrassPanel(context.grass, context.grassTuftCount);
        drawFramebuffersPanel(context.postProcessSettings, context.postProcess);
        drawMazePanel(context.mazeSettings, context.mazeWorld, context.round, context.player,
                      context.camera);
        drawCollisionPanel(context.mazeWorld, context.round, context.player, context.drawColliders);
        drawAssetsPanel(context.assets, context.viewMode, context.lighting.normalMapping,
                        m_rawTextureSampler);
        drawLightsPanel(context.lighting, context.round);
    }

    // The HUD belongs to the game and not to the tools, so it is drawn whether or not
    // the panels are visible.
    drawHud(context.round, context.gameplay);

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

Parametr `context` to struktura `debug::DebugContext` z
[`src/debug/DebugContext.hpp`](../../src/debug/DebugContext.hpp): referencje do danych, które
panele i HUD pokazują i edytują (dwadzieścia osiem pól: od `time` i `window` po `grass`
i `grassTuftCount`, jedyne pole, które jest liczbą, a nie referencją, oraz cztery pola z
pierwszej części M7: `compositeShader`, `previewShader`, `postProcessSettings` i
`postProcess`). Buduje ją co klatkę
`main.cpp`.
Opis struktury jest w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.2.

Cztery etapy, zawsze w tej kolejności:

| Etap | Wywołania | Co się dzieje |
|---|---|---|
| 1. Początek klatki | `ImGui_ImplOpenGL3_NewFrame()`, `ImGui_ImplGlfw_NewFrame()`, `ImGui::NewFrame()` | backend renderera przygotowuje swoje zasoby (przy pierwszym użyciu tworzy shadery), backend platformy przekazuje rozmiar okna, skalę framebuffera, czas i stan myszy, a rdzeń zaczyna nową klatkę |
| 2. Widżety | `DockSpaceOverViewport`, a potem jedenaście funkcji paneli (jedenasta, `drawFramebuffersPanel`, od pierwszej części M7: sekcja 3.16) i, już poza warunkiem `m_visible`, `drawHud`. Każda woła (przez naszą funkcję `placePanelOnFirstUse`) `SetNextWindowPos`, `SetNextWindowSize` i `SetNextWindowCollapsed`, potem `Begin`, swoje widżety i `End`: `drawRendererPanel` (`Text`, `ColorEdit3`, `Combo`, od M6 `Checkbox`, `SetItemTooltip` i `SliderFloat`), `drawShadersPanel` (`Button`, `Text`, `TextWrapped`, `SetItemTooltip`), `drawCameraPanel` (`DragFloat3`, `SliderFloat`), `drawGameplayPanel` (`Text`, `Button`, `SliderFloat`, `Checkbox`), od M6 `drawTerrainPanel` (`SliderFloat`, `SetItemTooltip`, `Checkbox`, `Separator`, `Text`) i `drawGrassPanel` (`Checkbox`, `SliderFloat`, `SetItemTooltip`, `Separator`, `Text`), `drawMazePanel` (`SliderInt`, `InputScalar`, `Button`, lista rysowania), `drawCollisionPanel` (`Checkbox`, `TextWrapped`), `drawAssetsPanel` (`Combo`, `Checkbox`, `SliderFloat`, `Image`), `drawLightsPanel` (`ColorEdit3`, `CollapsingHeader`, `SliderFloat`, `Checkbox`, `DragFloatRange2`, `SetItemTooltip`). HUD: `ProgressBar`, `TextColored`, `TextDisabled`, `PushFont`. Widżety paneli z M2 + M3: sekcja 3.11, widżety panelu Lights: sekcja 3.13, HUD i panel Gameplay: sekcja 3.14, panele Terrain i Grass: sekcja 3.15 | opisujemy interfejs, ImGui od razu odpowiada na interakcje i zbiera geometrię |
| 3. Zamknięcie klatki | `ImGui::Render()` | kończy klatkę i układa zebrane dane w listy rysowania (draw lists). Wbrew nazwie nie wywołuje OpenGL |
| 4. Rysowanie | `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())` | backend renderera wysyła listy do OpenGL: tu naprawdę pojawiają się piksele |

Ważne szczegóły:

- **Każdemu `NewFrame` musi odpowiadać `Render`** (albo `EndFrame`). Widżety wolno wołać
  tylko pomiędzy nimi.
- **Klatka ImGui zaczyna się zawsze, także gdy panele są ukryte.** `if (m_visible)` obejmuje
  tylko panele z etapu 2. Dzięki temu ImGui dalej odbiera zdarzenia wejścia i liczy czas, a po
  ponownym włączeniu paneli (klawisz `~`) nie ma skoku. Od M5 jest drugi powód: HUD gry
  (`drawHud`) stoi za blokiem `if` i jest rysowany w każdej klatce, więc klatka ImGui
  nigdy nie jest pusta.
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
              .gameplay = gameplaySettings(),
              .round = round(),
              .skyboxShader = skyboxShader(),
              .skybox = skyboxSettings(),
              .grassShader = grassShader(),
              .terrain = terrainSettings(),
              .grass = grassSettings(),
              .grassTuftCount = grassTuftCount(),
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

  Najpierw gra rysuje swoją klatkę (tło, labirynt, bramę, kryształy i na życzenie linie
  kolizji: `glViewport`, `glEnable`, `glClearColor`, `glClear` w
  [`NightMazeApp::onRender`](../../src/game/NightMazeApp.cpp) i wywołania `glDrawElements`
  w `gfx::Mesh::draw`), a dopiero potem ImGui rysuje
  na tym, co już jest w buforze, więc panele i HUD są na wierzchu sceny. Zamiana buforów
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
- Układ domyślny naszych jedenastu paneli ustawiają trójki `SetNextWindowPos`,
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
wywołanie w `DebugUI::draw` obok pozostałych jedenastu funkcji `draw...Panel`, dopisanie plików
do `add_executable` w `CMakeLists.txt`. Przed `Begin` wywołanie `placePanelOnFirstUse` z nową
stałą dopisaną w `src/debug/PanelLayout.hpp`, żeby panel przy pierwszym uruchomieniu nie
przykrył innych. Nowe dane dla panelu to dodatkowo jedno pole w `debug::DebugContext` i
jedna linia w `main.cpp`. Pełna instrukcja krok po kroku jest w
[`../modules/debug-ui.md`](../modules/debug-ui.md) i tam należy jej szukać.

### 3.11. Widżety paneli Maze, Collision i Assets

Panele z M2 + M3 używają kilkunastu funkcji ImGui, których wcześniej w projekcie nie było.
Każdy fragment niżej jest skopiowany z pliku podanego w tabeli.

**Pozycja i rozmiar na pierwsze uruchomienie.** Wszystkie jedenaście paneli woła przed `Begin`
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
| `Checkbox` | `ImGui::Checkbox("Draw collision shapes", &drawColliders);` | `CollisionPanel.cpp` (do M4 etykieta brzmiała `Draw collision boxes`: od M5 przełącznik rysuje też kule), od map normalnych także `AssetsPanel.cpp` (`ImGui::Checkbox("Normal mapping", &normalMapping);`, pole `game::LightingSettings::normalMapping`), od M5 `GameplayPanel.cpp` (`ImGui::Checkbox("Battery drains", &settings.batteryDrains);`) | pole wyboru na zmiennej `bool` |
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
| `SetItemTooltip("%s", fullPath.c_str())` | `AssetsPanel.cpp`, a w `ShadersPanel.cpp` z dwiema ścieżkami: `ImGui::SetItemTooltip("%s\n%s", vertexFullPath.c_str(), fragmentFullPath.c_str())`. Od M5 także `LightsPanel.cpp`, warunkowo (sekcja 3.14) | podpowiedź dla **poprzedniego** widżetu, pokazywana po najechaniu kursorem |
| `TextWrapped(...)` | większość paneli, na przykład `ShadersPanel.cpp` i `CollisionPanel.cpp` (od M5 legenda kolorów linii kolizji) | tekst łamany na szerokości panelu, z formatem jak w `printf` |
| `SameLine()` | `MazePanel.cpp`, od M5 `Hud.cpp` | następny widżet staje w tej samej linii (przyciski `Regenerate` i `Random seed` obok siebie, a w HUD licznik kryształów, czas i procent baterii obok paska) |
| `PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR)` i `PopStyleColor()` | `AssetsPanel.cpp`, `ShadersPanel.cpp` | zmiana koloru tekstu dla widżetów między tą parą. Każde `Push` musi mieć swoje `Pop`. Stała jest jedna, w `Theme.hpp` (sekcja 3.12). Od M5 ta sama para zmienia w `Hud.cpp` inną pozycję tabeli kolorów, `ImGuiCol_PlotHistogram` (sekcja 3.14) |

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

Od pierwszej części M7 dochodzi drugi skutek tego, że ImGui czyta teksturę sam i zapisuje
wynik prosto do okna. Tekstury koloru są teraz teksturami sRGB (`GL_SRGB8`), więc odczyt
oddaje wartości **liniowe**, których po ImGui nikt już nie zakoduje: podgląd byłby za ciemny.
Dlatego w pliku linia `ImGui::Image` jest dla tekstur sRGB otoczona parą
`rawSampler.begin()` i `rawSampler.end()`, która na czas tego jednego obrazka podmienia
sampler na taki, który nie dekoduje (sekcja 3.16). Fragment kodu wyżej pokazuje samą linię
`Image`, wspólną dla obu rodzajów tekstur.

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
| `drawList->AddLine(p1, p2, color)` | odcinek między dwoma punktami ekranu. Tak rysowana jest każda ściana i kreska kierunku patrzenia. Czwarty, opcjonalny argument to grubość w pikselach (domyślnie 1): od M5 brama jest odcinkiem o grubości `GATE_LINE_THICKNESS` (3) |
| `drawList->AddCircleFilled(center, radius, color)` | wypełnione koło: kropka gracza i, od M5, każdy niezebrany kryształ |
| `drawList->AddCircle(center, radius, color)` | sam okrąg, bez wypełnienia: od M5 ślad po zebranym krysztale |
| `drawList->AddRect(p_min, p_max, color)` | obrys prostokąta między lewym górnym a prawym dolnym rogiem: od M5 strefa wyjścia |
| `ImGui::GetColorU32(PLAN_WALL_COLOR)` | zamienia kolor motywu (`ImVec4`, cztery liczby `float`) na jedną liczbę `ImU32`, której chce lista rysowania (sekcja 3.12). Kolory planu to stałe z `Theme.hpp` |
| `ImGui::Dummy(size)` | niewidzialny widżet o podanym rozmiarze. Lista rysowania nie przesuwa kursora, więc bez `Dummy` panel nie wiedziałby, że plan zajmuje miejsce: następny widżet stanąłby na planie, a przewijanie liczyłoby złą wysokość |

Współrzędne w liście rysowania to współrzędne **ekranu** ImGui (piksele okna programu, y w
dół), a nie współrzędne wewnątrz panelu. Dlatego plan zaczyna od `GetCursorScreenPos()` i do
każdego punktu dodaje ten początek. Jak punkt świata zamienia się na punkt planu, opisuje
[`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 6.

Stan sprawdzenia: wygląd planu, podglądów i list jest sprawdzony na zrzutach ekranu z
Windowsa (2026-10-05). Samych kontrolek (kliknięcia w `Combo`, suwaki, pola wyboru, przyciski)
nikt jeszcze ręcznie nie sprawdzał, także listy `Lighting` z M4. Na macOS panele nie były
uruchamiane. Kształty z M5 na planie (strefa wyjścia, brama, kryształy) opisuje sekcja 3.14.

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
| `ImGui::PushStyleColor(ImGuiCol_Text, kolor)` i `ImGui::PopStyleColor()` | panele Shaders i Assets, a w HUD z pozycją `ImGuiCol_PlotHistogram` | zmiana jednej pozycji tabeli na czas kilku widżetów. Każde `Push` musi mieć `Pop` |
| `ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, wartość)` i `ImGui::PopStyleVar()` | karta wygranej w HUD | to samo dla jednej metryki stylu (tu odstęp treści od brzegu okna, `ImVec2`). Musi stać przed `Begin`, bo `Begin` czyta ten odstęp |
| `ImGui::PushFont(nullptr, rozmiar)` i `ImGui::PopFont()` | tytuł karty wygranej w HUD | **kształt z 1.92:** drugi argument to rozmiar czcionki przed skalowaniem. `nullptr` jako pierwszy znaczy "ta sama czcionka" (sekcja 3.14) |
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
| `placement.collapsed` | `true` zwija okno do paska tytułu, `false` zostawia je rozwinięte. W projekcie `true` ma pięć paneli: Camera (`CAMERA_PLACEMENT` w `PanelLayout.hpp`), od M5 Gameplay (`GAMEPLAY_PLACEMENT`), który stoi obok niego przy górnej krawędzi, od M6 Terrain i Grass (`TERRAIN_PLACEMENT`, `GRASS_PLACEMENT`), które stoją rząd niżej, pod tymi dwoma (sekcja 3.15), i od pierwszej części M7 Framebuffers (`FRAMEBUFFERS_PLACEMENT`), jeden szeroki pasek w trzecim rzędzie (sekcja 3.16) |
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
na zmiennej `bool` i `Text` z liczbą świateł punktowych. Od M5 ta ostatnia linia to
`Lit: %d of %d crystals (at most %d)`, grupa nazywa się `Point lights (crystals)` (w M4
`Point lights (dead ends)`), a pole wyboru latarki ma przy pustej baterii podpowiedź
(sekcja 3.14).

Stan sprawdzenia: skutki wartości domyślnych oświetlenia (widok startowy, cztery tryby
oświetlenia, scena z wyłączoną latarką) są obejrzane na zrzutach ekranu z Windowsa
(2026-10-05). Żadnego z tych widżetów nikt jeszcze nie klikał ani nie przeciągał,
tak samo jak strzałki rozwijającej panel Camera. Opis zachowania pochodzi z kodu projektu i
ze źródeł biblioteki. Na macOS kod M4 nie był budowany ani uruchamiany.

### 3.14. HUD gry, panel Gameplay i nowe kształty planu (M5)

Kod: [`src/debug/Hud.cpp`](../../src/debug/Hud.cpp),
[`src/debug/panels/GameplayPanel.cpp`](../../src/debug/panels/GameplayPanel.cpp),
[`src/debug/panels/MazePanel.cpp`](../../src/debug/panels/MazePanel.cpp) i
[`src/debug/panels/LightsPanel.cpp`](../../src/debug/panels/LightsPanel.cpp). Co HUD i panel
pokazują i jakie reguły za tym stoją, opisuje
[`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 6. Tutaj jest samo API.
Deklaracje przytaczam z `build/debug/_deps/imgui-src/imgui.h` w naszej wersji.

**Drugi rodzaj okna w projekcie.** Do M4 każde okno ImGui w programie było panelem: ma pasek
tytułu, da się je przesuwać, dokować, zwijać, a jego miejsce trafia do `imgui.ini`. HUD to
dwa okna innego rodzaju: pasek stanu u góry ekranu (`"Game HUD"`) i karta `"You escaped"` na
środku, widoczna po wygranej. Oba powstają tym samym `Begin` i `End` co panele. Różnią się
tylko flagami w trzecim argumencie `Begin` i tym, co stoi przed nim.

```cpp
constexpr ImGuiWindowFlags PICTURE_WINDOW_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;
```

`ImGuiWindowFlags` to zbiór bitów, a `|` (bitowe "lub") składa kilka flag w jedną liczbę.

| Flaga | Co wyłącza albo włącza | Po co w HUD |
|---|---|---|
| `ImGuiWindowFlags_NoDecoration` | skrót czterech flag: bez paska tytułu, bez uchwytu zmiany rozmiaru, bez paska przewijania, bez zwijania | HUD ma wyglądać jak napis na ekranie, nie jak okno |
| `ImGuiWindowFlags_AlwaysAutoResize` | okno ma w każdej klatce dokładnie rozmiar swojej zawartości | pasek sam się poszerza, gdy dochodzi linia podpowiedzi, i nikt nie ustawia mu rozmiaru |
| `ImGuiWindowFlags_NoInputs` | skrót trzech flag: bez wejścia z myszy i bez nawigacji klawiaturą. Mysz "przechodzi przez" okno: nie da się go najechać ani kliknąć | kliknięcie w miejscu HUD ma trafić do gry (przechwycenie kursora), a okno nie może ustawić `WantCaptureMouse` (sekcja 3.8) |
| `ImGuiWindowFlags_NoNav` | nawigacja klawiaturą ImGui omija okno | zawiera się już w `NoInputs`. Zapisane osobno, żeby zamiar był widoczny |
| `ImGuiWindowFlags_NoFocusOnAppearing` | pojawienie się okna nie zabiera fokusu | karta wygranej pojawia się w środku gry i nie może odebrać fokusu panelowi, w którym ktoś właśnie pracuje |
| `ImGuiWindowFlags_NoSavedSettings` | nic o oknie nie trafia do `imgui.ini` (sekcja 3.7) | pozycję ustawia kod w każdej klatce, więc nie ma czego pamiętać |
| `ImGuiWindowFlags_NoDocking` | okna nie da się zadokować w obszarze dokowania (sekcja 3.6) | HUD nie jest panelem |
| `ImGuiWindowFlags_NoMove` | okna nie da się przesunąć | stoi tam, gdzie postawił je kod |

Pasek stanu ma jedną flagę więcej:

```cpp
constexpr ImGuiWindowFlags STATUS_WINDOW_FLAGS =
    PICTURE_WINDOW_FLAGS | ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr ImGuiWindowFlags CARD_WINDOW_FLAGS = PICTURE_WINDOW_FLAGS;
```

`ImGuiWindowFlags_NoBringToFrontOnFocus` (w `imgui.h`: "Disable bringing window to front when
taking focus") trzyma okno z tyłu kolejności rysowania. Pasek stoi tuż pod paskami tytułu
zwiniętych paneli: do M5 pod jednym rzędem (Camera i Gameplay, stała `HUD_TOP_OFFSET = 46`),
od M6 pod dwoma (pod nimi Terrain i Grass), od pierwszej części M7 pod trzema (trzeci rząd to
pasek Framebuffers), a odległość jest liczona z prawdziwej wysokości paska (sekcja 3.15). Rozwinięty panel go
przykrywa, i tak ma być: panel, w którym ktoś pracuje, jest ważniejszy niż napis pod nim.
Karta tej flagi nie ma: ImGui stawia nowe okno przed już istniejącymi, więc karta pojawia się
na wierzchu paneli, a panel kliknięty później wychodzi przed nią jak przed każde inne okno
(komentarz w `Hud.cpp`).

**Pozycja liczona co klatkę: `SetNextWindowPos` z pivotem i `ImGuiCond_Always`.**

```cpp
    const ImVec2 top = windowPoint(TOP_CENTER);
    // The rows of title bars are measured with the real height of a bar, which follows
    // the font (foldedRowsHeight).
    const float rowsAbove = foldedRowsHeight(FOLDED_ROW_COUNT, scale);
    ImGui::SetNextWindowPos({top.x, top.y + rowsAbove + HUD_TOP_OFFSET * scale}, ImGuiCond_Always,
                            TOP_CENTER);
    ImGui::SetNextWindowBgAlpha(HUD_OPACITY);

    // The name is never shown (there is no title bar). ImGui tells windows apart by it.
    if (ImGui::Begin("Game HUD", nullptr, STATUS_WINDOW_FLAGS)) {
```

| Element | Znaczenie |
|---|---|
| `windowPoint(TOP_CENTER)` | nasza funkcja: punkt okna programu podany jako części jego rozmiaru, liczony z `ImGui::GetMainViewport()` (`WorkPos` i `WorkSize`). `TOP_CENTER` to `{0.5, 0}`, środek górnej krawędzi |
| `foldedRowsHeight(FOLDED_ROW_COUNT, scale)` | nasza funkcja z `PanelLayout.cpp` (od M6): wysokość dwóch rzędów zwiniętych pasków tytułów, liczona z `ImGui::GetFrameHeight()` (sekcja 3.15). HUD staje pod nimi |
| `ImGuiCond_Always` | pozycja jest ustawiana w **każdej** klatce. Panele używają `ImGuiCond_FirstUseEver` (sekcja 3.11), bo użytkownik ma móc je przesunąć. HUD ma zostać na środku także po zmianie rozmiaru okna programu |
| trzeci argument, `TOP_CENTER` | pivot: punkt okna ImGui, który ma trafić w podaną pozycję. `{0.5, 0}` to środek jego górnej krawędzi, więc pasek jest wyśrodkowany, choć jego szerokość zmienia się z zawartością (`AlwaysAutoResize`) i nie jest znana przed `Begin`. Karta wygranej używa pivotu `{0.5, 0.5}` i jest wyśrodkowana w obu kierunkach |
| `ImGui::SetNextWindowBgAlpha(HUD_OPACITY)` | deklaracja: `void SetNextWindowBgAlpha(float alpha);`. Ustawia przezroczystość tła najbliższego okna: 0 to tło niewidoczne, 1 to pełne. Pasek ma 0,72, karta 0,9: przez pasek widać scenę, karta ma być czytelna |
| `"Game HUD"` | nazwa okna. Nikt jej nie widzi (nie ma paska tytułu), ale ImGui rozróżnia okna po nazwie, więc musi być i musi być inna niż nazwy paneli |
| `nullptr` | drugi argument `Begin` to wskaźnik na `bool` dla przycisku zamykania. `nullptr` znaczy: bez przycisku |

`scale` to `ImGui::GetStyle().FontScaleDpi` (sekcja 3.12): odległości HUD są podane w
pikselach przy skali 100% i mnożone przez skalę ekranu, tak jak rozmiary paneli.

**`ProgressBar` i `PushStyleColor`: pasek baterii.**

```cpp
    const bool low = round.battery < settings.lowBatteryThreshold;
    // A progress bar is drawn in the colour ImGui calls PlotHistogram. PushStyleColor
    // changes a colour of the style until the matching PopStyleColor.
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, low ? HUD_BATTERY_LOW_COLOR : HUD_BATTERY_COLOR);
    // The second argument is the size: a height of 0 means the height of a line of
    // text. The third is the text written on the bar: none, the label next to it says
    // the number.
    ImGui::ProgressBar(round.battery, {BATTERY_BAR_WIDTH * scale, 0.0F}, "");
    ImGui::PopStyleColor();
```

| Element | Znaczenie |
|---|---|
| deklaracja | `void ProgressBar(float fraction, const ImVec2& size_arg = ImVec2(-FLT_MIN, 0), const char* overlay = NULL);` |
| `round.battery` | wypełnienie paska jako ułamek od 0 do 1. To dokładnie zakres pola `Round::battery`, więc nie ma przeliczania |
| `{BATTERY_BAR_WIDTH * scale, 0.0F}` | rozmiar. Szerokość 230 jednostek razy skala ekranu, wysokość 0 znaczy "wysokość linii tekstu z ramką". Ta szerokość wyznacza też szerokość całego paska HUD |
| `""` | napis rysowany na pasku. Pusty napis to brak napisu. Wartość domyślna `NULL` kazałaby ImGui wypisać na pasku procent, a u nas procent stoi obok (`ImGui::Text("%.0f%%", ...)` po `SameLine`) |
| `ImGuiCol_PlotHistogram` | pasek postępu nie ma własnej pozycji w tabeli kolorów: wypełnienie bierze kolor wykresu słupkowego. Stąd zmiana właśnie tej pozycji |
| `PushStyleColor` i `PopStyleColor` | para z sekcji 3.11, tu z inną pozycją tabeli. Zmiana obowiązuje tylko dla widżetów między nimi, więc pozostałe paski i wykresy (gdyby były) zostają w kolorze motywu |
| `low ? HUD_BATTERY_LOW_COLOR : HUD_BATTERY_COLOR` | bursztynowy, a poniżej progu `lowBatteryThreshold` czerwony. Kolory to stałe `ImVec4` z `Theme.hpp` |

**Tekst w kolorze: `TextColored` i `TextDisabled`.** Deklaracje w `imgui.h` mówią wprost,
czym są: `TextColored(const ImVec4& col, const char* fmt, ...)` to "shortcut for
PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();", a `TextDisabled` robi
to samo z kolorem `ImGuiCol_TextDisabled` ze stylu. HUD używa pierwszej dla słowa `Crystals`
i obu podpowiedzi, drugiej dla przygaszonych dodatków (`(of %d)` i czas rundy).

**Większa czcionka na chwilę: `PushFont` z rozmiarem.**

```cpp
        // The same font, larger. The size is given without the display scale: ImGui
        // multiplies it by FontScaleDpi itself.
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * CARD_TITLE_SCALE);
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "You escaped");
        ImGui::PopFont();
```

| Element | Znaczenie |
|---|---|
| deklaracja | `void PushFont(ImFont* font, float font_size_base_unscaled);` z komentarzem "Use NULL as a shortcut to keep current font. Use 0.0f to keep current size." |
| `nullptr` | ta sama czcionka co w panelach. Projekt wczytuje jedną (sekcja 3.12) |
| `FontSizeBase * CARD_TITLE_SCALE` | rozmiar **przed** skalowaniem: podstawowa wysokość tekstu razy 1,8. Skali ekranu tu nie ma, bo ImGui samo mnoży ten rozmiar przez `FontScaleDpi`. Pomnożenie ręcznie dałoby na ekranie 150% tytuł powiększony dwa razy |
| dlaczego to działa bez drugiej czcionki | w 1.92 czcionka nie jest wypalana w jednym rozmiarze przy starcie: backend dorysowuje znaki w potrzebnym rozmiarze na żądanie (flaga `ImGuiBackendFlags_RendererHasTextures`, sekcja 3.12). W starszych wersjach większy tytuł wymagał wczytania drugiej czcionki |
| `PopFont()` | cofa zmianę. Jak każda para `Push` i `Pop`, musi się zgadzać liczba wywołań |

Karta ma też szersze marginesy niż panele: `ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
{...})` przed `Begin` i `ImGui::PopStyleVar()` po `End`. To odpowiednik `PushStyleColor` dla
metryk stylu. Stoi przed `Begin`, bo `Begin` czyta odstęp przy otwieraniu okna, i jest
zdejmowany po `End`, poza blokiem `if`, żeby `Pop` wykonało się zawsze.

**Panel Gameplay: przycisk, który tylko prosi.** Ósmy panel używa widżetów znanych z
wcześniejszych: `Text`, `TextUnformatted`, `Separator`, `Checkbox` i sześć razy `SliderFloat`
z flagą `ImGuiSliderFlags_AlwaysClamp`. Dwa miejsca są warte pokazania.

```cpp
        // Button returns true only in the frame in which it was clicked. The panel only
        // asks: the game starts the round at the start of its next frame.
        if (ImGui::Button("Restart round (key R)")) {
            settings.restart = true;
        }
```

To ten sam wzorzec co przyciski `Regenerate` i `Random seed` w panelu Maze: `Button` zwraca
`true` w jednej klatce, a panel zapisuje wtedy flagę i nic więcej. Rundę zaczyna gra, na
początku swojej następnej klatki ([`../modules/core/README.md`](../modules/core/README.md),
sekcja 6.6).

```cpp
    ImGui::SliderFloat("Battery", &round.battery, MIN_BATTERY, MAX_BATTERY, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
```

Ten suwak nie edytuje ustawienia, tylko **stan gry**: pole `Round::battery`, które symulacja
zmienia w każdym kroku. W trybie immediate mode nie ma w tym nic szczególnego: suwak co
klatkę pokazuje bieżącą wartość spod wskaźnika, więc sam "jedzie" w lewo, gdy bateria ubywa,
a przeciągnięcie zapisuje nową wartość, od której gra liczy dalej. Żaden kod nie
synchronizuje widżetu ze zmienną, bo widżet nie ma własnej kopii (sekcja 1). Format ostatniego
argumentu `SliderFloat` jest tekstem `printf`, więc może nieść jednostkę albo słowa:
`"%.2f of all"` dla `Crystals needed`, `"%.0f s"` dla `Battery lifetime`, `"%.2f m"` dla
`Pickup radius`.

**Podpowiedź tylko w jednym stanie: `SetItemTooltip` pod warunkiem** (panel Lights):

```cpp
    ImGui::Checkbox("Flashlight on (key F)", &lighting.flashlightOn);
    if (round.battery <= 0.0F) {
        ImGui::SetItemTooltip("The battery is empty: collect a crystal first.");
    }
```

`SetItemTooltip` dotyczy **poprzedniego** widżetu, więc musi stać zaraz po `Checkbox`.
Wywołanie jest zwykłą instrukcją, więc może stać w `if`: podpowiedź istnieje tylko w
klatkach, w których bateria jest pusta, i tłumaczy, dlaczego zaznaczone pole samo się
odznacza (gra wyłącza latarkę w najbliższym kroku symulacji).

**Plan labiryntu: trzy nowe kształty listy rysowania** (panel Maze):

```cpp
    drawList->AddRect(toScreen(world.exitZone.min), toScreen(world.exitZone.max), exitColor);
```

```cpp
        drawList->AddLine(
            toScreen(world.gate.position - halfLine), toScreen(world.gate.position + halfLine),
            game::gateBlocks(world, round) ? gateColor : collectedColor, GATE_LINE_THICKNESS);
```

```cpp
        if (crystal.collected) {
            drawList->AddCircle(place, CRYSTAL_DOT_RADIUS, collectedColor);
        } else {
            drawList->AddCircleFilled(place, CRYSTAL_DOT_RADIUS, crystalColor);
        }
```

| Funkcja | Deklaracja w `imgui.h` | Co rysuje na planie |
|---|---|---|
| `AddRect` | `void AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, float thickness = 1.0f, ImDrawFlags flags = 0);` | obrys strefy wyjścia. `p_min` to lewy górny róg, `p_max` prawy dolny. Narożniki `min` i `max` pudełka świata pasują wprost, bo na planie x rośnie w prawo, a z w dół |
| `AddLine` z czwartym argumentem | `void AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);` | bramę: odcinek grubości 3 pikseli, w kolorze drewna, dopóki blokuje drogę, i przygaszony po otwarciu |
| `AddCircle` | `void AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);` | pusty okrąg w miejscu zebranego kryształu |
| `AddCircleFilled` | `void AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments = 0);` | wypełnioną kropkę niezebranego kryształu (i, jak wcześniej, gracza) |

`num_segments = 0` znaczy, że ImGui samo dobiera liczbę odcinków okręgu do jego promienia.
W naszej wersji kolejność dwóch ostatnich argumentów `AddRect` jest inna niż w starszych
poradnikach: `imgui.h` oznacza postać z `flags` przed `thickness` jako przestarzałą od
1.92.8. Projekt podaje tylko trzy pierwsze argumenty, więc go to nie dotyczy.

Stan sprawdzenia (M5, Windows, 2026-10-05): build Debug i Release przechodzi bez ostrzeżeń,
a obraz gry został sprawdzony na zrzutach ekranu robionych przez tymczasowe haki, które potem
usunięto. Nikt jeszcze nie sprawdził ręcznie: przycisku `Restart round (key R)`, suwaków
panelu Gameplay, karty wygranej, HUD przy ukrytych panelach ani tego, czy kliknięcie w
miejscu HUD naprawdę trafia do sceny. Opis zachowania flag pochodzi z kodu projektu i z
komentarzy w `imgui.h`. Na macOS kod M5 nie był budowany ani uruchamiany.

### 3.15. Panele Terrain i Grass, `GetFrameHeight` i drugi rząd pasków (M6)

Kod: [`src/debug/panels/TerrainPanel.cpp`](../../src/debug/panels/TerrainPanel.cpp),
[`src/debug/panels/GrassPanel.cpp`](../../src/debug/panels/GrassPanel.cpp),
[`src/debug/PanelLayout.cpp`](../../src/debug/PanelLayout.cpp) i
[`src/debug/Hud.cpp`](../../src/debug/Hud.cpp). Co kontrolki znaczą dla terenu i trawy:
[`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) i
[`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md). Kod paneli
linia po linii: [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.10.

**Widżety.** Dziewiąty i dziesiąty panel nie wprowadzają żadnego nowego widżetu. Używają
tych, które projekt już zna:

| Widżet | Gdzie | Uwaga |
|---|---|---|
| `SliderFloat(..., ImGuiSliderFlags_AlwaysClamp)` | `Height scale` w panelu Terrain, `Density`, `Blade height` i `Wind strength` w panelu Grass | cztery suwaki. Dwa z nich (`Height scale`, `Density`) stoją w warunku `if`, bo `SliderFloat` zwraca `true` w każdej klatce, w której wartość się zmieniła, a panel ustawia wtedy flagę prośby (`rebuild`, `replant`). To ten sam wzorzec co `Button` z sekcji 3.14, tylko że suwak "klika" w każdej klatce przeciągania |
| format z tekstem | `"%.1f per m"`, `"%.2f m"` | napis formatu jest tekstem `printf`, więc niesie jednostkę (sekcja 3.14) |
| `Checkbox` | `Wireframe`, `Enabled` | czyta i pisze `bool` przez wskaźnik |
| `SetItemTooltip` | pod `Height scale`, `Wireframe` i `Density` | podpowiedź do poprzedniego widżetu. Tekst ma kilka linii: `\n` łamie linię, a sąsiednie literały napisów kompilator skleja w jeden |
| `Separator`, `Text` | linie odczytu na dole obu paneli | `Text` z formatem `%d` dostaje `int`, więc `std::size_t` jest rzutowane przez `static_cast<int>` |

**`ImGui::GetFrameHeight()`: wysokość paska tytułu.** Nowa w projekcie jest jedna funkcja
ImGui. Jej deklaracja w `imgui.h`:

```cpp
IMGUI_API float GetFrameHeight();   // ~ FontSize + style.FramePadding.y * 2
```

Zwraca wysokość jednej linii widżetów z ramką: rozmiar czcionki plus odstęp `FramePadding.y`
nad tekstem i pod nim. Pasek tytułu okna ma dokładnie tę wysokość (w `imgui.cpp`:
`TitleBarHeight = g.FontSize + g.Style.FramePadding.y * 2.0f`). Przy czcionce 16 i
`FramePadding.y` równym 3 to 22 jednostki. Projekt woła ją w jednym miejscu:

```cpp
float foldedRowsHeight(int count, float gapScale) {
    // GetFrameHeight is the height of one line of widgets: the text plus the frame
    // padding above and below it. The title bar of a panel is exactly that high.
    return static_cast<float>(count) * (ImGui::GetFrameHeight() + PANEL_GAP * gapScale);
}
```

| Kto woła `foldedRowsHeight` | Z czym | Po co |
|---|---|---|
| `placePanelOnFirstUse` | `placement.foldedRowsBefore` (0, 1 albo, od M7, 2) i `layoutScale` | panele Terrain i Grass stają o jeden rząd pasków niżej niż Camera i Gameplay, a panel Framebuffers o dwa |
| `drawStatus` w `Hud.cpp` | `FOLDED_ROW_COUNT` (2 w M6, 3 od pierwszej części M7) i skala ekranu | HUD staje pod wszystkimi rzędami pasków |

Dlaczego pytać ImGui, a nie wpisać 22: wysokość paska idzie za czcionką, a czcionka za skalą
ekranu (`FontScaleDpi`, sekcja 3.12). Przy skali 150% pasek ma 33 jednostki. Stała wpisana w
kod byłaby dobra tylko dla jednej skali, co do M5 było znaną pułapką stałej
`HUD_TOP_OFFSET = 46`. Funkcja czyta bieżącą czcionkę i styl, więc wolno ją wołać tylko
wewnątrz klatki ImGui, między `NewFrame` a `Render`: oba miejsca wywołania to spełniają.

Stan sprawdzenia (druga część M6, Windows, 2026-10-05): build Debug i Release bez ostrzeżeń
zgłosił wykonawca, a obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe
haki, które potem usunięto. Zgłoszone jest, że HUD stoi około 30 pikseli niżej niż w M5 przy
skali 100%, co zgadza się z rachunkiem (jeden rząd: 22 + 8). Nikt jeszcze nie rozwinął
ręcznie paneli Terrain i Grass ani nie ruszył żadnego ich suwaka. Skali 150% z dwoma rzędami
pasków nikt nie oglądał. Na macOS kod M6 nie był budowany ani uruchamiany.

### 3.16. Panel Framebuffers, tekstura framebuffera w `Image` i własne polecenie na liście rysowania (M7)

Pierwsza część M7 (bufor HDR i gamma) dodała jedenasty panel, `Framebuffers`
([`FramebuffersPanel.cpp`](../../src/debug/panels/FramebuffersPanel.cpp)), i klasę
`debug::RawTextureSampler` ([`RawTextureSampler.cpp`](../../src/debug/RawTextureSampler.cpp)).
Co panel pokazuje i czego uczy, opisuje
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), a klasę
[`../modules/debug-ui.md`](../modules/debug-ui.md). Tutaj jest tylko to, co w nich nowe po
stronie ImGui.

**Trzeci rząd pasków.** Panel startuje zwinięty (`FRAMEBUFFERS_PLACEMENT`, `collapsed = true`,
`foldedRowsBefore = 2`) jako jeden pasek pod paskami Terrain i Grass, szeroki jak oba razem.
Zwiniętych paneli jest więc pięć, rzędy pasków są trzy, stała `FOLDED_ROW_COUNT` w
`PanelLayout.hpp` ma wartość 3, a HUD staje o jeden pasek tytułu niżej niż w M6 (rachunek
z sekcji 3.15: jeszcze raz wysokość paska plus odstęp, czyli 22 + 8 przy skali 100%).

**`Begin` jako czujnik.** Panel zapamiętuje wynik `Begin`:

```cpp
    const bool open = ImGui::Begin("Framebuffers");
    settings.previews = open;
```

`Begin` zwraca fałsz, gdy okno jest zwinięte (sekcja 3.5). Gra rysuje dwa obrazki podglądu
tylko wtedy, gdy `previews` jest prawdą, czyli gdy ktoś na nie patrzy. `DebugUI::draw` zeruje
tę flagę na początku każdej klatki, jeszcze przed `if (m_visible)`, więc przy ukrytych panelach
(gdy `Begin` w ogóle nie jest wołane) obrazki też przestają być rysowane. Skutek uboczny:
obrazek powstaje w klatce **następnej** po otwarciu panelu, więc przez jedną klatkę panel
pisze `(no picture yet)`.

**Widżety.** Wszystkie są znane z wcześniejszych paneli:

| Wywołanie | Uwagi |
|---|---|
| `SliderFloat("Exposure", ..., "%.2f", ImGuiSliderFlags_AlwaysClamp \| ImGuiSliderFlags_Logarithmic)` | suwak logarytmiczny jak przy wykładniku odblasku (sekcja 3.13): zakres od 0,1 do 8, a podwojenie i połowienie światła to odcinki tej samej długości |
| `Combo("Tone mapping", &toneMappingIndex, TONE_MAPPING_ITEMS)` | lista z pozycjami w jednym napisie (`"None (clamp)\0Reinhard\0ACES (fitted)\0"`), w kolejności wyliczenia `game::ToneMapping` |
| `SliderFloat("Depth range", ..., "%.0f m", ImGuiSliderFlags_AlwaysClamp)` | jednostka w napisie formatu |
| `SetItemTooltip(...)` | podpowiedź pod każdym z trzech widżetów |
| `Text(...)` z `%d` i `%s` | linia z rozmiarem i formatami framebuffera sceny |
| `BeginGroup()` i `EndGroup()` | **nowe w projekcie**: podpis i obrazek pod nim tworzą jedną grupę, którą ImGui traktuje przy układaniu jak jeden widżet. Dzięki temu `SameLine()` między dwiema grupami stawia cały drugi podgląd obok pierwszego, a nie sam podpis |
| `GetContentRegionAvail().x` i `GetStyle().ItemSpacing.x` | szerokość jednego obrazka: wolne miejsce minus odstęp między widżetami, podzielone przez 2 |

**Tekstura framebuffera w `Image`.**

```cpp
        const auto textureId = static_cast<ImTextureID>(preview.colorTextureId());
        ImGui::Image(textureId, {width, height}, {0.0F, 1.0F}, {1.0F, 0.0F});
```

To samo wywołanie co w panelu Assets (sekcja 3.11), tylko identyfikator pochodzi z
`gfx::Framebuffer::colorTextureId()`: załącznik koloru framebuffera jest zwykłą teksturą 2D.
Narożniki UV są odwrócone z tego samego powodu: wszystko, co OpenGL narysował do tekstury,
ma wiersz `v = 0` na dole. Backend czyta ją własnym samplerem (filtr liniowy, przycinanie do
krawędzi), a nie parametrami tekstury.

**Kolory ImGui a sRGB.** ImGui rysuje prosto do okna, po przebiegu składającym, który sam
koduje scenę na sRGB w shaderze. Przełącznik `GL_FRAMEBUFFER_SRGB` zostaje wyłączony, więc
OpenGL niczego nie przelicza przy zapisie i kolory motywu trafiają na ekran dokładnie takie,
jakie są w tabeli stylu (decyzja:
[`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md)). Z tego samego
powodu obrazek pokazywany przez `Image` musi być **już zakodowany**: ImGui zapisuje to, co
przeczyta. Podglądy framebuffera koduje shader `post/preview.frag`. Tekstury sRGB z panelu
Assets wymagają czegoś innego.

**Własne polecenie na liście rysowania: `AddCallback`.** ImGui nie rysuje w chwili wywołania
widżetu: zbiera polecenia na listach rysowania (`ImDrawList`) i wykonuje je w
`ImGui_ImplOpenGL3_RenderDrawData` (sekcja 3.4). Żeby zmienić stan OpenGL dla **jednego**
obrazka, trzeba więc wstawić własne polecenie na listę, w odpowiednim miejscu kolejki:

```cpp
    GLuint sampler = m_sampler;
    ImGui::GetWindowDrawList()->AddCallback(bindSampler, &sampler, sizeof(sampler));
```

| Element | Znaczenie |
|---|---|
| `ImGui::GetWindowDrawList()` | lista rysowania bieżącego okna, ta sama, do której trafi zaraz `Image` |
| `AddCallback(funkcja, dane, rozmiar)` | dopisuje polecenie "wywołaj tę funkcję", które backend wykona w kolejności, między rysowaniem poprzedniego i następnego widżetu |
| trzeci argument, `sizeof(sampler)` | gdy rozmiar jest większy od zera, ImGui **kopiuje** tyle bajtów spod wskaźnika do własnego bufora. Funkcja dostaje potem wskaźnik do kopii (`ImDrawCmd::UserCallbackData`), więc zmienna lokalna `sampler` może zniknąć przed rysowaniem |
| `void bindSampler(const ImDrawList*, const ImDrawCmd* command)` | sygnatura, której wymaga typ `ImDrawCallback`. Nasza funkcja odczytuje identyfikator z `command->UserCallbackData` i woła `glBindSampler(0, sampler)`: jednostka 0 to ta, na której backend rysuje obrazki |

Powrót do zwykłego stanu to drugie polecenie, tym razem gotowe:

```cpp
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear,
                                            nullptr);
```

`DrawCallback_SetSamplerLinear` to wskaźnik do funkcji, który backend renderera wpisuje do
`ImGuiPlatformIO` przy inicjalizacji. W backendzie OpenGL3 naszej wersji (1.92.9b, plik
`imgui_impl_opengl3.cpp` w pobranych źródłach, odczyt z kodu, nie pomiar) funkcja ta wiąże
z jednostką 0 własny liniowy sampler backendu. Panel Assets otacza tą parą poleceń `Image`
każdej tekstury sRGB (sekcja 3.11 pokazuje samą linię `Image`, bez tej pary).

Stan sprawdzenia: zgłoszone dla Windowsa (2026-10-05), że gra działa w buildzie Debug bez
błędów OpenGL z otwartym panelem Framebuffers i że miniatury w panelu Assets są identyczne co
do piksela z miniaturami sprzed M7. Żadnego z nowych widżetów nikt nie kliknął ręcznie, a
trzeciego rzędu pasków i HUD pod nim nikt nie oglądał przy skali innej niż 100%. Na macOS kod
M7 nie był budowany ani uruchamiany.

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

19. **Okno, które ma tylko pokazywać, a łapie mysz.** Okno bez `ImGuiWindowFlags_NoInputs`
    ustawia `WantCaptureMouse`, gdy kursor jest nad nim, nawet jeśli nie ma w nim żadnego
    widżetu do kliknięcia. HUD na środku ekranu blokowałby wtedy grze kliknięcie, które
    przechwytuje kursor. Stąd zestaw flag w `Hud.cpp` (sekcja 3.14).
20. **Skala ekranu policzona dwa razy w `PushFont`.** Drugi argument to rozmiar przed
    skalowaniem. Pomnożony ręcznie przez `FontScaleDpi` daje na ekranie 150% tekst za duży,
    a na ekranie 100% wygląda dobrze, więc błąd widać dopiero na drugim komputerze.

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

17. **Czym okno HUD różni się od panelu, skoro oba powstają przez `Begin` i `End`?**
    Flagami i tym, kto ustawia pozycję. HUD ma `NoDecoration` (bez paska tytułu),
    `AlwaysAutoResize` (rozmiar z zawartości), `NoInputs` i `NoNav` (mysz i klawiatura go
    omijają), `NoFocusOnAppearing`, `NoSavedSettings` (nic w `imgui.ini`), `NoDocking`
    i `NoMove`, a pozycję dostaje w każdej klatce (`ImGuiCond_Always`) z pivotem na środku.
    Panel ma pozycję tylko na pierwsze uruchomienie (`ImGuiCond_FirstUseEver`) i potem
    należy do użytkownika. Poza tym HUD jest rysowany poza blokiem `if (m_visible)`.

18. **Dlaczego pasek baterii zmienia kolor przez `ImGuiCol_PlotHistogram`?**
    Bo `ProgressBar` nie ma własnej pozycji w tabeli kolorów i rysuje wypełnienie kolorem
    wykresu słupkowego. `PushStyleColor` podmienia tę pozycję tylko do najbliższego
    `PopStyleColor`, więc zmiana dotyczy jednego paska.

19. **Jak panel Gameplay zaczyna rundę od nowa i dlaczego nie robi tego sam?**
    `Button` zwraca `true` w klatce kliknięcia, a panel ustawia wtedy flagę
    `GameplaySettings::restart`. Gra czyta ją na początku następnej klatki, między krokami
    symulacji. Panel rysuje się po scenie, w środku klatki, więc wymiana stanu gry w tym
    miejscu dałaby klatkę złożoną z dwóch stanów, a warstwa `debug/` decydowałaby o grze.

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
