# Moduł debug: panele ImGui

Kamień milowy: M0 (nakładka i panel Renderer), M1 (panele Shaders i Camera, `DebugContext`, mysz), M2 + M3 (panele Maze, Collision i Assets, czternaście pól `DebugContext`, układ domyślny paneli). Po M2 + M3 doszedł motyw paneli: własne kolory i odstępy, czcionka z pliku z polskimi literami, skala ekranu i układ domyślny w jednym pliku (sekcje 5.7 i 5.8). M4 (oświetlenie) dodał siódmy panel Lights, listę `Lighting` w panelu Renderer, dwa programy w panelu Shaders (było ich wtedy pięć), trzy pola `DebugContext` (razem siedemnaście) i układ, w którym panel Camera startuje zwinięty. M4 (mapy normalnych): pole wyboru `Normal mapping` i linie `normal map:` w panelu Assets, trzeci parametr `drawAssetsPanel`. M5 (rozgrywka): ósmy panel Gameplay, HUD gry rysowany przez `debug::drawHud` także wtedy, gdy panele są schowane (sekcja 5.9), cztery programy w panelu Shaders po usunięciu kostki z M1, osiemnaście pól `DebugContext` (ubyło `shader`, doszły `gameplay` i `round`), nowe parametry paneli Maze, Collision i Lights oraz układ, w którym panele Camera i Gameplay startują zwinięte obok siebie. M6, część pierwsza (skybox): pole `Skybox` i suwak `Sky brightness` w panelu Renderer (piąty parametr `drawRendererPanel`), piąty program w panelu Shaders, dwadzieścia pól `DebugContext` (doszły `skyboxShader` i `skybox`) i wyższy panel Renderer w układzie domyślnym (284 zamiast 230), przez co panel Lights pod nim się przewija. M6, część druga (teren i trawa): dziewiąty i dziesiąty panel, Terrain i Grass, które startują zwinięte w drugim rzędzie pasków tytułów pod panelami Camera i Gameplay (pole `PanelPlacement::foldedRowsBefore` i funkcja `foldedRowsHeight`, sekcja 5.7), HUD przesunięty pod ten drugi rząd (sekcja 5.9.5), szósty program w panelu Shaders, pierwszy z trzema plikami (`grass.vert + grass.geom + grass.frag`), i dwadzieścia cztery pola `DebugContext` (doszły `grassShader`, `terrain`, `grass` i `grassTuftCount`). M7, część pierwsza (bufor HDR i gamma): jedenasty panel, Framebuffers, zwinięty w trzecim rzędzie pasków tytułów (sekcje 5.7 i 6), HUD przesunięty o jeszcze jeden rząd w dół, siódmy i ósmy program w panelu Shaders (`composite` i `preview`), dwadzieścia osiem pól `DebugContext` (doszły `compositeShader`, `previewShader`, `postProcessSettings` i `postProcess`), klasa `debug::RawTextureSampler`, dzięki której podglądy tekstur sRGB w panelu Assets wyglądają jak pliki (sekcja 5.11), czwarty parametr `drawAssetsPanel` i suwak `Sky brightness` do 6. M7, część druga (bloom): bez nowego panelu. Panel Framebuffers dostał pole `Bloom`, suwaki `Blur iterations`, `Threshold` i `Intensity`, linię `Bloom targets` i dwa kolejne obrazy (`Bright pass`, `Bloom`), a jego kontrolki stoją odtąd w tabeli o dwóch kolumnach (pierwsze użycie `ImGui::BeginTable` w projekcie). Dziewiąty i dziesiąty program w panelu Shaders (`bright` i `blur`), `SHADER_COUNT` równe 10 i trzydzieści pól `DebugContext` (doszły `brightPassShader` i `blurShader`). M7, część trzecia (mgła i winieta): bez nowego panelu, bez nowego programu i bez nowego pola `DebugContext` (zmienił się tylko komentarz przy polu `postProcessSettings`). Kontrolki panelu Framebuffers stoją odtąd w dwóch zakładkach, `Tone and bloom` i `Fog and vignette` (pierwsze użycie `ImGui::BeginTabBar` i `ImGui::BeginTabItem` w projekcie), a druga zakładka niesie osiem nowych kontrolek: pole `Fog`, suwaki `Density`, `Base height` i `Height falloff`, próbnik `Fog colour`, pole `Vignette` oraz suwaki `Strength` i `Radius`. M7, część czwarta (cienie księżyca, 2026-10-05): dwunasty panel, Shadows, zwinięty w czwartym rzędzie pasków tytułów (sekcje 5.7 i 6), HUD przesunięty o jeszcze jeden rząd w dół, jedenasty program w panelu Shaders (`shadow_depth`), `SHADER_COUNT` równe 11 i trzydzieści cztery pola `DebugContext` (doszły `shadowDepthShader`, `moonShadowSettings`, `moonShadowMap` i `moonLightSpace`). W panelu Lights kod się nie zmienił, ale suwaki `Moon yaw` i `Moon pitch` obracają odtąd także cienie, a natężenie księżyca startuje od 0,2 zamiast 0,12. M7, część piąta (cień latarki, 2026-10-06): bez nowego panelu i bez nowego programu, panel Shadows dostał drugą zakładkę `Flashlight` (parametry funkcji `drawShadowsPanel` to dwie struktury `debug::ShadowMapView`), panel Lights trzy suwaki ręki i zbieżności wiązki, a `DebugContext` ma trzydzieści osiem pól (doszły `flashlightShadowSettings`, `flashlightShadowMap`, `flashlightLightSpace` i `flashlightShadowDrawn`). Kod: [`src/debug/`](../../src/debug/) oraz [`src/main.cpp`](../../src/main.cpp), gdzie nakładka jest podpinana do gry.
Teoria samej biblioteki (tryb natychmiastowy, backendy, docking) jest w [`../libraries/imgui.md`](../libraries/imgui.md). Ten dokument opisuje, jak ImGui jest wpięte w **mój** projekt i jak dodać nowy panel.

## 1. Po co to jest

Grafiki 3D nie da się wygodnie debugować `printf`em: chcę widzieć liczby (FPS, rozmiar framebuffera, wersję sterownika) i zmieniać parametry w działającym programie, bez przebudowywania. Moduł `debug` daje do tego nakładkę z panelami Dear ImGui rysowaną na wierzchu sceny. Nie realizuje osobnego tematu wykładu, ale obsługuje wszystkie piętnaście: każdy temat dostaje w panelu przełącznik, którym na obronie pokażę efekt "przed i po" (PRD, sekcje 3 i 10). Dziś istnieje dwanaście paneli: **Renderer**, pokazujący dane z tematu 1 (FPS, czas klatki) przełącznik tematu 7 (lista `Lighting`: bez oświetlenia, Gouraud, Phong, Blinn-Phong) i, od pierwszej części M6, przełącznik tematu 8 (pole `Skybox` z suwakiem `Sky brightness`), **Shaders**, pokaz tematu 2 (przycisk "Reload shaders" dla jedenastu programów), **Camera**, pokaz tematu 3 (pozycja gracza, kąty, FOV, płaszczyzny przycinania, czułość myszy, trzy prędkości gracza), **Gameplay** (stan rundy, przycisk nowej rundy, suwak baterii i liczby reguł gry), **Terrain**, pokaz tematu 13 (suwak skali wysokości terenu, pole `Wireframe`, rozmiar siatki, liczba trójkątów i zakres wysokości), **Grass**, pokaz tematu 9 (włącznik trawy z shadera geometrii, gęstość kępek, wysokość źdźbeł, siła wiatru, liczba kępek), **Framebuffers**, od pierwszej części M7 pokaz tematu 10 w jego dzisiejszym zakresie (dwie zakładki kontrolek: ekspozycja, krzywa mapowania tonów i bloom w pierwszej, mgła i winieta w drugiej, pod nimi rozmiar i formaty framebuffera sceny i celów bloomu oraz cztery obrazy podglądu: kolor HDR, głębia, przebieg jasności i bloom), **Shadows**, od czwartej części M7 pokaz tematu 11 w jego dzisiejszym zakresie, czyli cieni księżyca (włącznik cieni, rozdzielczość mapy cieni, dwie części biasu, filtr sprzętowy 2 x 2, PCF z rozmiarem jądra i siła cienia, pod nimi rozmiar i format mapy, obszar, który pokrywa, i rozmiar teksela, a obok obraz mapy widzianej z księżyca. Cienia latarki nie ma: jest planowany), **Maze** (rozmiar i ziarno labiryntu, przyciski "Regenerate" i "Random seed", plan z góry z kryształami, bramą i strefą wyjścia), **Collision**, pokaz tematu 14 (rysowanie pudełek i sfer kolizji, tryb noclip), **Assets**, pokaz tematów 4 i 5 (tryb widoku, pole wyboru `Normal mapping`, filtr tekstur, anizotropia, lista modeli i tekstur), i **Lights**, pokaz tematu 6 (światło otoczenia, księżyc, latarka, światła punktowe nad kryształami, połysk). PRD nie ma panelu o nazwie Assets: w sekcji 3 wymienia dla tematu 4 pokaz "Lista załadowanych modeli", a dla tematu 5 "Podgląd tekstur, toggle normal map". Panel Assets niesie oba pokazy, razem z przełącznikiem map normalnych: polem wyboru `Normal mapping` pod listą `View mode` ([`gfx/normal-mapping.md`](gfx/normal-mapping.md), sekcja 6). Od M5 moduł rysuje jeszcze jedną rzecz, która nie jest panelem ani narzędziem: **HUD gry** (licznik kryształów, czas, pasek baterii, karta wygranej). Mieszka tutaj tylko dlatego, że tu jest ImGui (sekcja 5.9).

## 2. Teoria

**Tryb natychmiastowy (immediate mode) w jednym akapicie.** W klasycznym GUI tworzy się obiekty widżetów, które żyją między klatkami. W ImGui nie ma obiektów: co klatkę wołam funkcje typu `ImGui::Text(...)`, a biblioteka z tych wywołań buduje listę trójkątów do narysowania. Panel jest więc zwykłą funkcją wykonywaną co klatkę, a dane, które pokazuje, należą do mojego kodu, nie do ImGui. Szczegóły: [`../libraries/imgui.md`](../libraries/imgui.md).

**Trzy części ImGui w projekcie:**

| Część | Plik biblioteki | Rola |
|---|---|---|
| Rdzeń | `imgui.cpp` i pokrewne | Logika widżetów, układ, docking. Nie zna ani GLFW, ani OpenGL |
| Backend platformy (platform backend) | `imgui_impl_glfw.cpp` | Przekazuje do ImGui mysz, klawiaturę, rozmiar okna i czas z GLFW |
| Backend renderera (renderer backend) | `imgui_impl_opengl3.cpp` | Zamienia listy rysowania ImGui na wywołania OpenGL |

**Klatka ImGui wewnątrz mojej klatki.** ImGui rysuję zawsze na samym końcu klatki, po scenie, żeby panele były na wierzchu. Pilnuje tego `DebugNightMazeApp::onRender` w `main.cpp`, które najpierw woła rysowanie gry, a dopiero potem `DebugUI::draw`:

```mermaid
flowchart TD
    A["DebugNightMazeApp::onRender (main.cpp)"] --> B["game::NightMazeApp::onRender: prośba o nowy labirynt, prośba o nową rundę (klawisz R albo przycisk panelu), klawisze N i F, mysz kamery, glViewport, glEnable, glClearColor, glClear, światła klatki, scena (labirynt, brama i kryształy, linie pudełek i sfer kolizji)"]
    B --> T["klawisz ~ ? m_debugUI.toggleVisible()"]
    T --> ME["m_debugUI.setMouseEnabled(!input().isCursorCaptured())"]
    ME --> C["DebugUI::draw(DebugContext)"]
    C --> D["ImGui_ImplOpenGL3_NewFrame()"]
    D --> E["ImGui_ImplGlfw_NewFrame()"]
    E --> F["ImGui::NewFrame()"]
    F --> G{"m_visible?"}
    G -->|tak| H["ImGui::DockSpaceOverViewport(...)"]
    H --> I["drawRendererPanel(...)"]
    I --> S["drawShadersPanel(...)"]
    S --> CP["drawCameraPanel(...)"]
    CP --> GP["drawGameplayPanel(...)"]
    GP --> MP["drawMazePanel(...)"]
    MP --> CO["drawCollisionPanel(...)"]
    CO --> AP["drawAssetsPanel(...)"]
    AP --> LP["drawLightsPanel(...)"]
    LP --> HUD["drawHud(...): zawsze, także przy schowanych panelach"]
    G -->|nie| HUD
    HUD --> J["ImGui::Render()"]
    J --> K["ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())"]
    K --> M["input().setKeyboardBlocked(m_debugUI.wantsKeyboard())"]
    M --> N["input().setMouseBlocked(m_debugUI.wantsMouse())"]
    N --> L["Application::run: swapBuffers"]
```

**Docking.** Używam gałęzi `docking` biblioteki (tag przypięty w [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake)). Pozwala ona przyczepiać panele do krawędzi okna i do siebie nawzajem, łączyć je w zakładki i zapamiętać układ. To ważne na obronie: przy kilku panelach chcę jednym ruchem ustawić sobie widok dla danego tematu.

## 3. Jak to działa w OpenGL

Mój kod w `src/debug` nie woła bezpośrednio żadnej funkcji `gl*`. Całą rozmowę z OpenGL prowadzi backend renderera, ale trzeba wiedzieć, co robi, bo działa na **tym samym kontekście** co reszta programu.

### 3.1 Inicjalizacja (`DebugUI::DebugUI`)

```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true);
ImGui_ImplOpenGL3_Init("#version 410");

applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()));
loadFont(m_fontBytes);
```

| Linia | Co robi |
|---|---|
| `IMGUI_CHECKVERSION()` | Sprawdza, czy nagłówki i skompilowana biblioteka są w tej samej wersji (rozmiary struktur). Chroni przed trudnymi do znalezienia błędami po aktualizacji |
| `ImGui::CreateContext()` | Tworzy kontekst **ImGui** (cały jego stan). To nie jest kontekst OpenGL, zbieżność nazw jest przypadkowa |
| `ConfigFlags \|= ImGuiConfigFlags_DockingEnable` | Włącza docking. `\|=` dodaje jeden bit do istniejących flag, nie kasując pozostałych |
| `ImGui_ImplGlfw_InitForOpenGL(handle, true)` | Podłącza backend platformy do mojego okna (o argumencie `true` niżej) |
| `ImGui_ImplOpenGL3_Init("#version 410")` | Podłącza backend renderera i zapamiętuje wersję GLSL dla jego shaderów |
| `applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()))` | Ustawia kolory, odstępy i rozmiar czcionki paneli, przemnożone przez skalę ekranu (1 przy 100%, 1,5 przy 150% na Windowsie, zawsze 1 na macOS). Sekcja 5.8 |
| `loadFont(m_fontBytes)` | Wczytuje czcionkę paneli z `assets/fonts`. Bajty pliku zostają w polu `m_fontBytes`, bo ImGui trzyma do nich tylko wskaźnik. Sekcja 5.8.5 |

Fragment jest pokazany bez komentarzy, które stoją w pliku. Motyw i czcionka są ustawiane **po** obu backendach i **przed** pierwszą klatką: funkcja skali należy do backendu GLFW, a czcionki trzeba dodać, zanim `ImGui::NewFrame()` zacznie ich używać.

**Argument `true` w `ImGui_ImplGlfw_InitForOpenGL`.** To parametr `install_callbacks`. Z wartością `true` backend sam rejestruje w GLFW swoje funkcje zwrotne (callbacki): klawiszy, znaków, przycisków myszy, kółka, pozycji kursora, wejścia kursora w okno i fokusu okna. GLFW przechowuje tylko **jeden** callback danego typu na okno, a funkcja `glfwSet...Callback` zwraca poprzednio ustawiony. Backend zapamiętuje te poprzednie i woła je ze swoich callbacków, czyli buduje łańcuch (chaining): moje ewentualne wcześniejsze callbacki nadal by działały. Moduł `core` nie ustawia żadnych callbacków wejścia (`Input` odpytuje `glfwGetKey`, `glfwGetMouseButton` i `glfwGetCursorPos`), więc nic się nie gryzie. Z wartością `false` musiałbym sam zarejestrować callbacki i ręcznie przekazywać każde zdarzenie do funkcji `ImGui_ImplGlfw_...Callback`.

Obiekty OpenGL backendu (program shaderów, bufory wierzchołków i indeksów, tekstura czcionki) nie powstają w `Init`, tylko leniwie, przy pierwszym `ImGui_ImplOpenGL3_NewFrame()`.

Backend OpenGL korzysta z własnego, małego loadera funkcji dołączonego do ImGui, więc nie potrzebuje GLAD (stąd w CMake cel `imgui` linkuje tylko `glfw`). W programie działają zatem dwa loadery, które pytają ten sam sterownik o te same adresy. To nie jest konflikt.

### 3.2 Klatka (`DebugUI::draw`)

```cpp
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplGlfw_NewFrame();
ImGui::NewFrame();

context.postProcessSettings.previews = false;
context.moonShadowSettings.preview = false;
context.flashlightShadowSettings.preview = false;

if (m_visible) {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    drawRendererPanel(context.time, context.window, context.clearColor, context.lighting.mode,
                      context.skybox);

    constexpr int SHADER_COUNT = 11;
    const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
        &context.texturedShader,  &context.colorShader,      &context.litShader,
        &context.gouraudShader,   &context.skyboxShader,     &context.grassShader,
        &context.compositeShader, &context.previewShader,    &context.brightPassShader,
        &context.blurShader,      &context.shadowDepthShader};
    drawShadersPanel(shaders);

    drawCameraPanel(context.camera, context.player, context.mouseSensitivity);
    drawGameplayPanel(context.round, context.gameplay);
    drawTerrainPanel(context.terrain, context.mazeWorld.terrain);
    drawGrassPanel(context.grass, context.grassTuftCount);
    drawFramebuffersPanel(context.postProcessSettings, context.postProcess);
    drawShadowsPanel({.settings = context.moonShadowSettings,
                      .map = context.moonShadowMap,
                      .lightSpace = context.moonLightSpace,
                      .drawn = context.moonShadowSettings.enabled},
                     {.settings = context.flashlightShadowSettings,
                      .map = context.flashlightShadowMap,
                      .lightSpace = context.flashlightLightSpace,
                      .drawn = context.flashlightShadowDrawn});
    drawMazePanel(context.mazeSettings, context.mazeWorld, context.round, context.player,
                  context.camera);
    drawCollisionPanel(context.mazeWorld, context.round, context.player, context.drawColliders);
    drawAssetsPanel(context.assets, context.viewMode, context.lighting.normalMapping,
                    m_rawTextureSampler);
    drawLightsPanel(context.lighting, context.round);
}

drawHud(context.round, context.gameplay);

ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
```

| Wywołanie | Co robi |
|---|---|
| `ImGui_ImplOpenGL3_NewFrame()` | Przy pierwszym wywołaniu tworzy obiekty OpenGL backendu, później praktycznie nic |
| `ImGui_ImplGlfw_NewFrame()` | Wpisuje do ImGui rozmiar okna, skalę framebuffera i czas od poprzedniej klatki (z `glfwGetTime`) |
| `ImGui::NewFrame()` | Przetwarza zebrane zdarzenia wejścia i otwiera nową klatkę. Dopiero po tym wolno wołać widżety. Tutaj ImGui ustala, nad którym panelem jest kursor, i liczy `WantCaptureKeyboard` oraz `WantCaptureMouse` (sekcja 5.6) |
| `ImGui::Render()` | Zamyka klatkę i zamienia wywołania widżetów na listy rysowania (wierzchołki, indeksy, prostokąty przycinania). **Jeszcze nic nie rysuje** |
| `ImGui_ImplOpenGL3_RenderDrawData(...)` | Wysyła te listy do OpenGL |

Fragment jest tu pokazany bez komentarzy i z wcięciem o jeden poziom mniejszym niż w pliku. Dwanaście wywołań paneli omawia sekcja 5.2, a wywołanie `drawHud`, które stoi **poza** blokiem `if (m_visible)`, sekcja 5.9. Kolejność wywołań nie jest kolejnością na ekranie: `drawLightsPanel` jest ostatnim panelem, a panel Lights staje w lewej kolumnie, bo o miejscu decyduje stała z `PanelLayout.hpp` (sekcja 5.7). Kolejność wywołań nie decyduje też o tym, co leży na wierzchu: HUD jest wołany po panelach, a jego pasek leży pod nimi (sekcja 5.9).

**Linia `context.postProcessSettings.previews = false;` (pierwsza część M7).** Komentarz nad nią w pliku:

```cpp
    // The preview pictures of the framebuffer attachments are drawn by the game only
    // while the Framebuffers panel is open. The panel sets the flag again below. With
    // the panels hidden nobody does, and the game stops drawing the pictures.
    context.postProcessSettings.previews = false;
```

To mała umowa między trzema miejscami. Gra rysuje dwa obrazy podglądu załączników tylko wtedy, gdy pole `PostProcessSettings::previews` jest prawdą (`NightMazeApp::onRender`). `DebugUI::draw` zeruje je na początku **każdej** klatki, a panel Framebuffers ustawia je z powrotem, gdy jest otwarty (`settings.previews = open;`, gdzie `open` to wynik `ImGui::Begin`). Gdy panele są schowane klawiszem `~`, `drawFramebuffersPanel` nie jest wołane wcale, więc flaga zostaje fałszem i gra przestaje płacić za dwa dodatkowe przebiegi. Gdy panel jest zwinięty do paska, `Begin` zwraca fałsz i skutek jest ten sam. Kolejność w klatce: gra rysuje **przed** `DebugUI::draw` (`main.cpp`), więc flaga ustawiona w tej klatce działa od następnej. Stąd napis `(no picture yet)` w panelu w pierwszej klatce po otwarciu. Pisanie przez `const DebugContext&` jest dozwolone z tego samego powodu co edycja koloru tła: pole jest referencją bez `const` (sekcja 5.2, punkt 6). Sam panel i podglądy opisuje [`renderer/post-process.md`](renderer/post-process.md).

**Linia `context.moonShadowSettings.preview = false;` (czwarta część M7, cienie księżyca).** Stoi zaraz pod poprzednią, z komentarzem:

```cpp
    // The same for the preview pictures of the two shadow maps and the Shadows panel.
    context.moonShadowSettings.preview = false;
    context.flashlightShadowSettings.preview = false;
```

To ta sama umowa, drugi raz. Gra rysuje obraz podglądu mapy cieni tylko wtedy, gdy pole `ShadowSettings::preview` jest prawdą (`NightMazeApp::drawMoonShadowMap`: `if (m_moonShadow.preview) { m_moonShadowMap.drawPreview(m_previewShader, m_moonLightSpace); }`, a od piątej części M7 tak samo `drawFlashlightShadowMap` dla `m_flashlightShadow`). `DebugUI::draw` zeruje obie flagi co klatkę (od piątej części M7 także `context.flashlightShadowSettings.preview = false;`), a panel Shadows ustawia z powrotem tylko flagę **zakładki, która jest właśnie pokazana** (`view.settings.preview = true;` na początku `drawShadowMapTab`). Od piątej części M7 nie jest to już wynik `ImGui::Begin` panelu: z dwóch zakładek widać naraz jedną, więc podgląd drugiej mapy też nie jest rysowany. Przy schowanych panelach, zwiniętym panelu Shadows albo zakładce, której nikt nie ogląda, flaga zostaje fałszem i gra nie płaci za dodatkowy przebieg. Dotyczy to tylko **obrazu podglądu**: sama mapa cieni jest rysowana w każdej klatce, dopóki pole `Shadows` jest zaznaczone, niezależnie od tego, czy ktokolwiek patrzy na panel. Opóźnienie o jedną klatkę jest to samo co wyżej, stąd napis `(no picture yet)` także w tym panelu. Żeby pisać do pola `preview`, `DebugUI.cpp` dołącza `game/Shadows.hpp` (pełna definicja `game::ShadowSettings`). Panel opisuje sekcja 6, a cienie [`renderer/shadows.md`](renderer/shadows.md).

**Miejsce w klatce od pierwszej części M7.** `DebugUI::draw` jest wołane po powrocie z `NightMazeApp::onRender`, a ta funkcja kończy się przebiegiem składającym, który wiąże z powrotem framebuffer okna. ImGui rysuje więc **prosto do okna**, na gotowym, zakodowanym już obrazie sceny. Kolory paneli nie przechodzą przez ekspozycję, mapowanie tonów ani kodowanie sRGB: `GL_FRAMEBUFFER_SRGB` jest wyłączone, więc liczby z motywu trafiają na ekran takie, jakie są ([`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md)).

`RenderDrawData` robi po kolei: zapamiętuje bieżący stan OpenGL, ustawia własny (włączone mieszanie kolorów `GL_BLEND` i test nożycowy `GL_SCISSOR_TEST`, wyłączony test głębi `GL_DEPTH_TEST` i odrzucanie ścian), ustawia viewport na cały framebuffer i macierz rzutu prostokątnego, tworzy tymczasowe VAO, wgrywa wierzchołki do buforów, rysuje przez `glDrawElements` i na końcu **przywraca zapamiętany stan**. Dzięki temu ImGui nie psuje ustawień renderera sceny, na przykład w późniejszych kamieniach milowych nie wyłączy mi testu głębi na stałe.

Jeden element stanu jest przywracany warunkowo: bieżący program shaderów. Backend zapamiętuje go na początku (`GL_CURRENT_PROGRAM`), a na końcu przywraca tylko wtedy, gdy ten program jeszcze istnieje (w źródle: `if (last_program == 0 || glIsProgram(last_program)) glUseProgram(last_program);`). Ma to znaczenie dla panelu Shaders, którego przycisk usuwa stare programy gry w środku klatki ImGui ([`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6.3).

Związek z Retiną: backend platformy podaje ImGui rozmiar okna we współrzędnych ekranu oraz skalę `framebuffer / okno`. ImGui układa panele we współrzędnych okna (te same jednostki co pozycja myszy), a backend renderera mnoży je przez skalę przy rysowaniu. Dlatego panele są ostre i klikalne na ekranie 2x bez żadnego kodu z mojej strony (tak wynika z kodu backendu, na Macu z nowym motywem nikt tego jeszcze nie uruchomił). Na Windowsie jest odwrotnie: okno i framebuffer mają ten sam rozmiar, a o powiększenie paneli przy skali 150% dba mój kod (sekcja 5.8.4).

**`DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode)`.** Funkcja tworzy niewidzialne okno ImGui rozciągnięte na cały główny viewport (całe moje okno) i umieszcza w nim obszar dokowania (dockspace). Od tej chwili panel przeciągnięty do krawędzi okna "przykleja się" do niej.

| Argument | Wartość | Znaczenie |
|---|---|---|
| `dockspace_id` | `0` | ImGui samo nadaje identyfikator obszaru |
| `viewport` | `ImGui::GetMainViewport()` | Obszar pokrywa główne okno programu |
| `flags` | `ImGuiDockNodeFlags_PassthruCentralNode` | Środek obszaru jest przezroczysty i przepuszcza mysz |

Obszar dokowania ma **węzeł centralny** (central node): to miejsce, które zostaje, gdy panele zajmą krawędzie. Domyślnie ImGui zamalowuje pusty węzeł centralny jednolitym tłem i przechwytuje w nim kliknięcia. Skutek: scena 3D znika pod szarym prostokątem. Flaga `PassthruCentralNode` wyłącza to tło i przepuszcza wejście, więc przez środek widać scenę (w M0 był to sam kolor czyszczenia, dziś labirynt), a panele zajmują tylko tyle miejsca, ile im dam.

**Dlaczego klatka ImGui działa także wtedy, gdy UI jest ukryte.** Po naciśnięciu klawisza `~` `m_visible` jest `false`, pomijam tylko dockspace i panele, ale `NewFrame`, `drawHud`, `Render` i `RenderDrawData` wykonują się dalej. Powody:

1. Callbacki backendu są zainstalowane przez cały czas i dokładają zdarzenia do kolejki ImGui. Kolejkę opróżnia `ImGui::NewFrame()`. Gdybym przestał je wołać, zdarzenia zbierałyby się i zostałyby przetworzone hurtem po ponownym pokazaniu paneli.
2. `ImGui_ImplGlfw_NewFrame()` liczy czas od poprzedniego wywołania. Po przerwie ImGui dostałoby jedną "klatkę" trwającą na przykład 20 sekund, co psuje animacje i odmierzanie czasu w bibliotece.
3. Kod jest prostszy: `NewFrame` i `Render` zawsze występują w parze, nie ma dwóch ścieżek do pomylenia.
4. Koszt jest mały: bez paneli `Render` buduje listy tylko dla okien HUD.
5. Od M5 klatka ImGui jest potrzebna grze także przy schowanych panelach: w niej rysowany jest HUD (sekcja 5.9). Do M4 klatka bez paneli była pusta i ten punkt nie istniał.

### 3.3 Zamknięcie (`DebugUI::~DebugUI`)

```cpp
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplGlfw_Shutdown();
ImGui::DestroyContext();
```

Kolejność odwrotna do inicjalizacji. `ImGui_ImplOpenGL3_Shutdown` usuwa obiekty OpenGL backendu, więc **kontekst OpenGL musi jeszcze istnieć**. `ImGui_ImplGlfw_Shutdown` przywraca w GLFW poprzednie callbacki, więc okno też musi istnieć. Gwarantuje to reguła C++ opisana w [`core/README.md`](core/README.md), sekcja 7: **pola są niszczone przed klasami bazowymi**. `m_debugUI` jest polem `DebugNightMazeApp`, a okno należy do klasy bazowej `core::Application`, więc ImGui zamyka się, gdy okno i kontekst jeszcze istnieją.

## 4. Shadery

Moduł `debug` nie ma własnych plików shaderów. Shadery ma backend renderera: napis `"#version 410"` przekazany do `ImGui_ImplOpenGL3_Init` jest doklejany jako pierwsza linia jego wbudowanego shadera wierzchołków i fragmentów, które backend kompiluje i linkuje przy pierwszej klatce. Wersja musi pasować do kontekstu: OpenGL 4.1 to GLSL 4.10, a domyślne w wielu przykładach `"#version 130"` nie skompiluje się w profilu Core na macOS. Shadery pisane przeze mnie (dwadzieścia plików w `assets/shaders/`: pięć par `textured`, `color`, `lit`, `gouraud` i `skybox`, trzy pliki `grass`, trzy pliki w `post/` i cztery pliki dołączane w `common/`) należą do gry, a nie do modułu `debug`, ale moduł ma dla nich panel **Shaders** z przyciskiem przeładowania (sekcja 6 i [`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6). PRD (sekcja 10) opisuje ten panel jako listę programów i tak dziś działa: panel dostaje listę jedenastu programów i pokazuje jedną linię dla każdego (a pod nią tekst błędu, gdy wczytanie się nie udało).

Backend ma też własne obiekty samplerów. W wersji z katalogu budowania (1.92.9b) wszystko, co rysuje, w tym podglądy tekstur z panelu Assets, jest czytane przez jego sampler z filtrem liniowym i zawijaniem `GL_CLAMP_TO_EDGE`, a nie przez sampler mojej tekstury ([`../libraries/imgui.md`](../libraries/imgui.md), sekcja 3). Dlatego filtr wybrany w panelu Assets widać w scenie, a nie w podglądach. Od pierwszej części M7 jest od tego jeden wyjątek: podgląd tekstury sRGB w panelu Assets jest czytany przez **mój** obiekt samplera, wpięty na czas jednego obrazu przez `debug::RawTextureSampler` (sekcja 5.11). HUD z M5 też nie ma shaderów: to dwa zwykłe okna ImGui z tekstem i paskiem postępu, rysowane tym samym programem backendu co panele.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/debug/DebugUI.hpp`](../../src/debug/DebugUI.hpp), [`.cpp`](../../src/debug/DebugUI.cpp) | Klasa `DebugUI`: cykl życia ImGui (RAII), zastosowanie motywu i wczytanie czcionki w konstruktorze, bajty czcionki (`m_fontBytes`), od M7 sampler podglądów (`m_rawTextureSampler`), klatka ImGui, dockspace, wywołanie paneli i HUD, widoczność paneli, `wantsKeyboard()`, `wantsMouse()`, `setMouseEnabled()` |
| [`src/debug/Theme.hpp`](../../src/debug/Theme.hpp), [`.cpp`](../../src/debug/Theme.cpp) | Motyw paneli: funkcja `colorFromBytes`, dziesięć stałych kolorów ze znaczeniem (tekst błędu `ERROR_TEXT_COLOR`, sześć kolorów planu od `PLAN_WALL_COLOR` do `PLAN_EXIT_COLOR` i trzy kolory HUD), `applyTheme` (kolory, metryki, skala ekranu) i `loadFont` (czcionka z `assets/fonts`). Sekcja 5.8 |
| [`src/debug/PanelLayout.hpp`](../../src/debug/PanelLayout.hpp), [`.cpp`](../../src/debug/PanelLayout.cpp) | Układ domyślny: struktura `PanelPlacement` (miejsce, rozmiar, to, czy panel startuje zwinięty, i od M6 liczba rzędów zwiniętych pasków nad nim), dwanaście stałych z miejscami paneli, stała `FOLDED_ROW_COUNT` oraz funkcje `placePanelOnFirstUse` i `foldedRowsHeight`. Sekcja 5.7 |
| [`src/debug/Hud.hpp`](../../src/debug/Hud.hpp), [`.cpp`](../../src/debug/Hud.cpp) | Funkcja `drawHud`: HUD gry, czyli pasek u góry okna (kryształy, czas, bateria, podpowiedź) i karta wygranej `You escaped`. Nie jest panelem: klawisz `~` go nie chowa. Architektura w sekcji 5.9, a to, co pokazuje i dlaczego: [`game/gameplay.md`](game/gameplay.md), sekcja 6 |
| [`assets/fonts/`](../../assets/fonts/) | Plik czcionki `AtkinsonHyperlegible-Regular.ttf`, jej licencja `OFL.txt` i `README.md` ze źródłem i wersją. Cudzy materiał, nie kod |
| [`src/debug/RawTextureSampler.hpp`](../../src/debug/RawTextureSampler.hpp), [`.cpp`](../../src/debug/RawTextureSampler.cpp) (od pierwszej części M7) | Klasa `RawTextureSampler`: obiekt samplera OpenGL z wyłączonym dekodowaniem sRGB (rozszerzenie `GL_EXT_texture_sRGB_decode`) i dwie funkcje, `begin` i `end`, które wpinają go na czas jednego obrazu ImGui. Sekcja 5.11 |
| [`src/debug/DebugContext.hpp`](../../src/debug/DebugContext.hpp) | Struktura `DebugContext`: referencje do wszystkiego, co panele mogą w tej klatce odczytać albo edytować. Sam nagłówek, bez pliku `.cpp` |
| [`src/debug/panels/RendererPanel.hpp`](../../src/debug/panels/RendererPanel.hpp), [`.cpp`](../../src/debug/panels/RendererPanel.cpp) | Funkcja `drawRendererPanel`: panel "Renderer" (statystyki klatki, dane sterownika, kolor czyszczenia (od M7 wartość sRGB), lista `Lighting` z trybem oświetlenia, od M6 pole `Skybox` i suwak `Sky brightness`, od M7 do 6). Sekcja 5.3 |
| [`src/debug/panels/ShadersPanel.hpp`](../../src/debug/panels/ShadersPanel.hpp), [`.cpp`](../../src/debug/panels/ShadersPanel.cpp) | Funkcja `drawShadersPanel`: panel "Shaders" (jeden przycisk "Reload shaders" dla wszystkich programów, a dla każdego programu jedna linia z nazwami jego plików (dwóch, a dla programu z shaderem geometrii trzech) i wynikiem ostatniego wczytania, pod nią tekst błędu). Opis linia po linii: [`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6 |
| [`src/debug/panels/LightsPanel.hpp`](../../src/debug/panels/LightsPanel.hpp), [`.cpp`](../../src/debug/panels/LightsPanel.cpp) | Funkcja `drawLightsPanel`: panel "Lights" (światło otoczenia i cztery zwijane grupy: księżyc, latarka, światła punktowe, połysk). Opis linia po linii: [`scene/lights.md`](scene/lights.md), sekcja 6 |
| [`src/debug/panels/CameraPanel.hpp`](../../src/debug/panels/CameraPanel.hpp), [`.cpp`](../../src/debug/panels/CameraPanel.cpp) | Funkcja `drawCameraPanel`: panel "Camera" (tryb, pozycja stóp gracza, oko, yaw, pitch, FOV, bliska i daleka płaszczyzna, czułość myszy, trzy prędkości gracza). Opis linia po linii: [`scene/camera-controls.md`](scene/camera-controls.md), sekcja 6 |
| [`src/debug/panels/GameplayPanel.hpp`](../../src/debug/panels/GameplayPanel.hpp), [`.cpp`](../../src/debug/panels/GameplayPanel.cpp) | Funkcja `drawGameplayPanel`: panel "Gameplay" (stan rundy, przycisk `Restart round (key R)`, suwak `Battery`, pole `Battery drains` i pięć suwaków z liczbami reguł). Szkielet i droga danych: sekcja 5.5, znaczenie każdej kontrolki: [`game/gameplay.md`](game/gameplay.md), sekcja 6 |
| [`src/debug/panels/TerrainPanel.hpp`](../../src/debug/panels/TerrainPanel.hpp), [`.cpp`](../../src/debug/panels/TerrainPanel.cpp) | Funkcja `drawTerrainPanel`: panel "Terrain" (suwak `Height scale`, pole `Wireframe`, trzy linie odczytu: siatka, trójkąty, zakres wysokości). Kod linia po linii: sekcja 5.10, znaczenie kontrolek: [`renderer/terrain.md`](renderer/terrain.md) |
| [`src/debug/panels/GrassPanel.hpp`](../../src/debug/panels/GrassPanel.hpp), [`.cpp`](../../src/debug/panels/GrassPanel.cpp) | Funkcja `drawGrassPanel`: panel "Grass" (pole `Enabled`, suwaki `Density`, `Blade height` i `Wind strength`, linia z liczbą kępek i źdźbeł). Kod linia po linii: sekcja 5.10, znaczenie kontrolek: [`renderer/grass-geometry.md`](renderer/grass-geometry.md) |
| [`src/debug/panels/FramebuffersPanel.hpp`](../../src/debug/panels/FramebuffersPanel.hpp), [`.cpp`](../../src/debug/panels/FramebuffersPanel.cpp) (od pierwszej części M7) | Funkcja `drawFramebuffersPanel`: panel "Framebuffers" (od trzeciej części M7 kontrolki w dwóch zakładkach. `Tone and bloom`: suwak `Exposure`, lista `Tone mapping`, od drugiej części M7 pole `Bloom` i suwaki `Blur iterations`, `Threshold`, `Intensity`, suwak `Depth range`. `Fog and vignette`: pole `Fog`, suwaki `Density`, `Base height`, `Height falloff`, próbnik `Fog colour`, pole `Vignette`, suwaki `Strength` i `Radius`. Pod zakładkami linie z rozmiarem i formatami framebuffera sceny i celów bloomu, cztery obrazy podglądu: kolor, głębia, przebieg jasności i bloom). Kod linia po linii i znaczenie kontrolek: [`renderer/post-process.md`](renderer/post-process.md), skrót w sekcji 6 |
| [`src/debug/panels/ShadowsPanel.hpp`](../../src/debug/panels/ShadowsPanel.hpp), [`.cpp`](../../src/debug/panels/ShadowsPanel.cpp) (od czwartej części M7) | Struktura `debug::ShadowMapView` (od piątej części M7) i funkcja `drawShadowsPanel`: panel "Shadows" (pasek zakładek z dwiema zakładkami, `Moon` i od piątej części M7 `Flashlight`, w każdej tabela o dwóch kolumnach. Po lewej pole `Shadows`, lista `Resolution`, suwaki `Constant bias` i `Slope bias`, pole `Hardware 2 x 2 filter`, pole `PCF` z listą `Kernel` i suwak `Strength`, pod kreską trzy linie odczytu. Po prawej obraz mapy cieni z podpisem `Depth seen from the moon` albo `Distance seen from the flashlight`). Kod i podpowiedzi: sekcja 6, znaczenie każdej kontrolki: [`renderer/shadows.md`](renderer/shadows.md), sekcja 6 |
| [`src/debug/panels/MazePanel.hpp`](../../src/debug/panels/MazePanel.hpp), [`.cpp`](../../src/debug/panels/MazePanel.cpp) | Funkcja `drawMazePanel`: panel "Maze" (rozmiar, ziarno, "Regenerate", "Random seed", liczba kryształów i komórka wyjścia, plan labiryntu z góry z graczem, kryształami, bramą i strefą wyjścia). Opis linia po linii: [`game/maze-generator.md`](game/maze-generator.md), sekcja 6 |
| [`src/debug/panels/CollisionPanel.hpp`](../../src/debug/panels/CollisionPanel.hpp), [`.cpp`](../../src/debug/panels/CollisionPanel.cpp) | Funkcja `drawCollisionPanel`: panel "Collision" (rysowanie pudełek i sfer, noclip, liczby pudełek i sfer zbierania, pudełko gracza). Opis linia po linii: [`scene/collision.md`](scene/collision.md), sekcja 6 |
| [`src/debug/panels/AssetsPanel.hpp`](../../src/debug/panels/AssetsPanel.hpp), [`.cpp`](../../src/debug/panels/AssetsPanel.cpp) | Funkcja `drawAssetsPanel`: panel "Assets" (tryb widoku, przełącznik mapowania normalnych, filtr, anizotropia, modele z teksturą i mapą normalnych każdej części, tekstury z podglądem i od M7 z dopiskiem `sRGB` albo `linear`, lista nieudanych wczytań). Opis linia po linii: [`assets/asset-cache.md`](assets/asset-cache.md), sekcja 6 |
| [`src/main.cpp`](../../src/main.cpp) | Klasa `DebugNightMazeApp`: posiada `DebugUI`, obsługuje klawisz `~`, wyłącza mysz w ImGui na czas przechwycenia kursora, co klatkę buduje `DebugContext` i woła `draw` (panele i HUD) po narysowaniu gry, przekazuje do `core::Input` blokadę klawiatury i myszy |
| [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) | Pobranie ImGui i definicja celu `imgui` (ImGui nie ma własnego CMake) |
| [`CMakeLists.txt`](../../CMakeLists.txt) | Pliki `src/debug/*` są częścią programu `night_maze`, nie biblioteki `engine` |

### 5.2 Architektura: kto co posiada

```mermaid
flowchart LR
    Main["DebugNightMazeApp (main.cpp)<br/>posiada m_debugUI"] -->|"setMouseEnabled(...), draw(DebugContext: 38 pól)"| UI["debug::DebugUI<br/>cykl życia ImGui, m_visible"]
    Main -->|"dziedziczy, woła onRender gry"| Game["game::NightMazeApp<br/>właściciel całego stanu gry"]
    Main -->|"czyta isCursorCaptured()<br/>setKeyboardBlocked(wantsKeyboard()), setMouseBlocked(wantsMouse())"| In["core::Input"]
    UI --> RP["drawRendererPanel<br/>time, window, clearColor, lighting.mode, skybox"]
    UI --> SP["drawShadersPanel<br/>lista jedenastu gfx::Shader*"]
    UI --> CP["drawCameraPanel<br/>camera, player, mouseSensitivity"]
    UI --> GP["drawGameplayPanel<br/>round, gameplay"]
    UI --> TP["drawTerrainPanel<br/>terrain, mazeWorld.terrain"]
    UI --> GR["drawGrassPanel<br/>grass, grassTuftCount"]
    UI --> FB["drawFramebuffersPanel<br/>postProcessSettings, postProcess"]
    UI --> SH["drawShadowsPanel<br/>dwa ShadowMapView: moonShadowSettings, moonShadowMap, moonLightSpace oraz flashlightShadowSettings, flashlightShadowMap, flashlightLightSpace, flashlightShadowDrawn"]
    UI --> MP["drawMazePanel<br/>mazeSettings, mazeWorld, round, player, camera"]
    UI --> CO["drawCollisionPanel<br/>mazeWorld, round, player, drawColliders"]
    UI --> AP["drawAssetsPanel<br/>assets, viewMode, lighting.normalMapping, m_rawTextureSampler"]
    UI --> LP["drawLightsPanel<br/>lighting, round"]
    UI --> HUD["drawHud (poza if m_visible)<br/>round, gameplay"]
    RP -->|"czyta czas i okno, zapisuje m_clearColor i m_lighting.mode"| Game
    SP -->|"woła reload() na jedenastu programach"| Game
    CP -->|"zapisuje kąty i rzutowanie kamery, pozycję i prędkości gracza, czułość myszy"| Game
    GP -->|"zapisuje m_round.battery i pola m_gameplay (w tym prośbę restart), czyta resztę m_round"| Game
    TP -->|"zapisuje m_terrainSettings (w tym prośbę rebuild), czyta m_mazeWorld.terrain"| Game
    GR -->|"zapisuje m_grassSettings (w tym prośbę replant), dostaje liczbę kępek jako kopię"| Game
    FB -->|"zapisuje m_postProcessSettings (ekspozycja, krzywa, zakres głębi, flaga previews), czyta m_postProcess"| Game
    SH -->|"zapisuje m_moonShadow i m_flashlightShadow (włącznik, rozdzielczość, bias, filtr, PCF, siła, flaga preview), czyta obie mapy, obie przestrzenie światła i flagę m_flashlightShadowDrawn"| Game
    MP -->|"zapisuje m_mazeSettings, czyta m_mazeWorld, m_round, m_player, m_camera"| Game
    CO -->|"zapisuje m_drawColliders i m_player.noclip, czyta m_mazeWorld i m_round"| Game
    AP -->|"woła setFilter i setAnisotropy na m_assets, zapisuje m_viewMode"| Game
    LP -->|"zapisuje pola m_lighting, czyta m_round"| Game
    HUD -->|"tylko czyta m_round i m_gameplay"| Game
```

Dwanaście paneli to dwanaście wolnych funkcji bez stanu, a HUD jest trzynastą. Jedyny stan, jaki moduł trzyma dla panelu, to obiekt samplera podglądów: pole `DebugUI::m_rawTextureSampler`, podawane panelowi Assets przez referencję (sekcja 5.11). Każda strzałka do gry idzie przez referencję z `DebugContext`: panel nie zna klasy `NightMazeApp`, zna tylko typy danych, które dostał (`scene::Camera`, `game::Player`, `game::MazeSettings`, `game::MazeWorld`, `assets::AssetCache`, `game::ViewMode`, `game::LightingSettings`, `game::LightingMode`, `game::GameplaySettings`, `game::Round`, `game::SkyboxSettings`, `game::TerrainSettings`, `game::Terrain`, `game::GrassSettings`, `game::PostProcessSettings`, `game::PostProcess`, `game::ShadowSettings`, `game::ShadowMap`, `scene::LightSpace`, `gfx::Shader`).

**Dlaczego `DebugUI` należy do klasy w `main.cpp`, a nie do gry.** W architekturze projektu (PRD, sekcja 6) `debug/` zależy od wszystkich warstw, ale **nic nie zależy od `debug/`**. Gdyby `game::NightMazeApp` miało pole `DebugUI`, plik gry dołączałby `debug/DebugUI.hpp` i gra nie dałaby się zbudować bez paneli. Dlatego sklejenie odbywa się piętro wyżej:

```cpp
class DebugNightMazeApp final : public game::NightMazeApp {
protected:
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
            .compositeShader = compositeShader(),
            .previewShader = previewShader(),
            .postProcessSettings = postProcessSettings(),
            .postProcess = postProcess(),
            .brightPassShader = brightPassShader(),
            .blurShader = blurShader(),
            .shadowDepthShader = shadowDepthShader(),
            .moonShadowSettings = moonShadowSettings(),
            .moonShadowMap = moonShadowMap(),
            .moonLightSpace = moonLightSpace(),
            .flashlightShadowSettings = flashlightShadowSettings(),
            .flashlightShadowMap = flashlightShadowMap(),
            .flashlightLightSpace = flashlightLightSpace(),
            .flashlightShadowDrawn = flashlightShadowDrawn(),
        });

        // ImGui now knows whether it is using the keyboard (a text field is being edited
        // or a widget is active) and the mouse (the cursor is over a panel or a widget is
        // being dragged). Block each device for the game from the next frame on, so typing
        // does not trigger Escape, the panel toggle or player movement, and working with
        // a panel does not click or look around in the scene.
        input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
        input().setMouseBlocked(m_debugUI.wantsMouse());
    }

private:
    // Members are destroyed before base classes, so ImGui shuts down while the window
    // and its OpenGL context (owned by core::Application) still exist.
    debug::DebugUI m_debugUI{window()};
};
```

`main.cpp` to jedyny plik, który dołącza zarówno `game/NightMazeApp.hpp`, jak i `debug/DebugUI.hpp` (oraz `debug/DebugContext.hpp`). Klasa dziedziczy po grze, nadpisuje `onRender`, woła w nim wersję gry (`game::NightMazeApp::onRender(alpha)`, z nazwą klasy, żeby ominąć mechanizm wirtualny i nie wpaść w rekurencję), potem mówi ImGui, czy wolno mu używać myszy, buduje `DebugContext` i dorysowuje panele oraz HUD, a na końcu przekazuje do `core::Input` informację, czy ImGui używa klawiatury i czy używa myszy (sekcja 5.6). Gra ze swojej strony udostępnia tylko trzydzieści sześć chronionych akcesorów (`clearColor()`, `texturedShader()`, `colorShader()`, `litShader()`, `gouraudShader()`, `skyboxShader()`, `grassShader()`, od pierwszej części M7 `compositeShader()`, `previewShader()`, `postProcessSettings()` i `postProcess()`, od drugiej `brightPassShader()` i `blurShader()`, od czwartej `shadowDepthShader()`, `moonShadowSettings()`, `moonShadowMap()` i `moonLightSpace()`, od piątej `flashlightShadowSettings()`, `flashlightShadowMap()`, `flashlightLightSpace()` i `flashlightShadowDrawn()`, `terrainSettings()`, `grassSettings()`, `grassTuftCount()`, `skyboxSettings()`, `lighting()`, `camera()`, `mouseSensitivity()`, `player()`, `mazeSettings()`, `mazeWorld()`, `gameplaySettings()`, `round()`, `assets()`, `viewMode()`, `drawColliders()`, tabela w [`core/README.md`](core/README.md), sekcja 6) i nie wie, kto z nich skorzysta. Kierunek zależności wygląda więc tak: `main.cpp` zna `game` i `debug`. `debug` zna `core`, `gfx`, `scene`, `assets` i typy danych z `game` (panel Shaders dołącza `gfx/Shader.hpp`, panel Camera `scene/Camera.hpp` i `game/Player.hpp`, panele Maze i Collision `game/MazeWorld.hpp`, `game/MazeLayout.hpp`, `game/Player.hpp` i `game/Round.hpp`, panel Gameplay i plik `Hud.cpp` samo `game/Round.hpp`, panel Assets `assets/AssetCache.hpp` i `game/MazeRenderer.hpp` dla wyliczenia `ViewMode`, panel Renderer `game/Lighting.hpp` dla wyliczenia `LightingMode`, panel Lights `game/Lighting.hpp`, `game/Round.hpp` i `scene/Light.hpp`, panel Framebuffers `game/PostProcess.hpp` i `gfx/Framebuffer.hpp`, panel Shadows `game/ShadowMap.hpp`, `game/Shadows.hpp`, `gfx/Framebuffer.hpp` i `scene/LightSpace.hpp`). Także `DebugUI.cpp` dołącza `game/Lighting.hpp`, bo sięga do pola `context.lighting.mode` i potrzebuje do tego pełnej definicji struktury, a od M7 z tego samego powodu `game/PostProcess.hpp` (pisze do `context.postProcessSettings.previews`), a od czwartej części M7 `game/Shadows.hpp` (pisze do `context.moonShadowSettings.preview`). `game` zna `core`, `gfx`, `scene` i `assets`, ale niczego z `debug`. Zależność jest więc jednostronna: panele znają dane gry, gra nie zna paneli. Komentarz w `main.cpp` ("the only place where the game meets the debug UI") mówi to samo: to jedyne miejsce, które tworzy obiekty obu warstw i je łączy.

Trzy decyzje, które trzeba umieć uzasadnić:

1. **`DebugUI` to RAII na ImGui.** Konstruktor inicjalizuje, destruktor zamyka, kopiowanie jest zablokowane (`= delete`), bo kontekst ImGui jest jeden. Nie da się zapomnieć o `Shutdown`.
2. **Panel to wolna funkcja, nie klasa.** `drawRendererPanel` nie ma własnego stanu (tak samo pozostałych jedenaście funkcji paneli i `drawHud`). Wszystko, co pokazuje i edytuje, dostaje w argumentach. Zgodnie z zasadą "dane zamiast kodu" (PRD, sekcja 6) stan należy do właściciela: kolor tła jest polem `game::NightMazeApp::m_clearColor`, a panel tylko go edytuje przez referencję.
3. **`const` mówi, co panel może zmienić.** W sygnaturze panelu Renderer `const core::Time&` i `const core::Window&` są tylko do odczytu. `std::array<float, 3>& clearColor` i `game::LightingMode& lightingMode` bez `const` to dwie rzeczy, które panel modyfikuje. Z samej sygnatury widać, co jest przełącznikiem. Tak samo czyta się pozostałe sygnatury:

   | Sygnatura | Tylko do odczytu | Edytowalne |
   |---|---|---|
   | `drawShadersPanel(std::span<gfx::Shader* const> shaders)` | sama lista (wskaźników nie da się przestawić) | shadery, na które wskazują: przycisk woła `reload()` |
   | `drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity)` | nic | kąty i rzutowanie kamery, pozycja i prędkości gracza, czułość |
   | `drawGameplayPanel(game::Round& round, game::GameplaySettings& settings)` | z rundy wszystko poza baterią: panel tylko to wypisuje (typ tego nie pilnuje, niżej) | `round.battery` (suwak `Battery`) i wszystkie pola `settings`: liczby reguł, `batteryDrains` i prośba `restart` |
   | `drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain)` | teren labiryntu w grze: rozmiar siatki, liczba trójkątów, najniższy i najwyższy punkt | skala wysokości, przełącznik siatki z krawędzi i prośba `rebuild` |
   | `drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount)` | liczba kępek: przychodzi przez wartość, więc panel nie ma czego zmienić | włącznik, gęstość, wysokość źdźbeł, siła wiatru i prośba `replant` |
   | `drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world, const game::Round& round, const game::Player& player, const scene::Camera& camera)` | labirynt w grze, runda (kryształy i stan bramy), gracz i kamera (rysowane na planie) | prośba o następny labirynt |
   | `drawCollisionPanel(const game::MazeWorld& world, const game::Round& round, game::Player& player, bool& drawColliders)` | labirynt i runda (liczenie pudełek, pudełka bramy i sfer zbierania) | `player.noclip` i przełącznik rysowania |
   | `drawFramebuffersPanel(game::PostProcessSettings& settings, const game::PostProcess& postProcess)` | framebuffery klatki: rozmiar i formaty framebuffera sceny i celów bloomu, cztery obrazy podglądu | w zakładce `Tone and bloom` ekspozycja, krzywa mapowania tonów, cztery pola bloomu i zakres podglądu głębi, w zakładce `Fog and vignette` (od trzeciej części M7) pięć pól mgły i trzy pola winiety, do tego flaga `previews` (panel ustawia ją sam, bez kontrolki) |
   | `drawShadowsPanel(const ShadowMapView& moon, const ShadowMapView& flashlight)` (od czwartej części M7, od piątej z dwiema strukturami zamiast trzech parametrów) | mapy cieni księżyca i latarki (rozmiar, format głębi, obraz podglądu), ich przestrzenie światła z ostatniej klatki (ile metrów pokrywa mapa) i flaga, czy mapa była w tej klatce rysowana | osiem pól ustawień cieni w każdej zakładce: `enabled`, `resolution`, `constantBias`, `slopeBias`, `hardwareFilter`, `pcf`, `pcfRadius` i `strength`, do tego flaga `preview` (panel ustawia ją sam, tylko dla pokazanej zakładki, bez kontrolki) |
   | `drawAssetsPanel(assets::AssetCache& assets, game::ViewMode& viewMode, bool& normalMapping, const RawTextureSampler& rawSampler)` | sampler podglądów tekstur sRGB (od M7): panel woła tylko jego `begin` i `end`, obie `const` | filtr i anizotropia wszystkich tekstur, tryb widoku, przełącznik mapowania normalnych (pole `normalMapping` struktury `LightingSettings`: panel dostaje referencję do jednego `bool`, a nie całą strukturę) |
   | `drawLightsPanel(game::LightingSettings& lighting, const game::Round& round)` | runda (ile kryształów jeszcze świeci, czy bateria jest pusta) | kolory, natężenia, kąty i zasięgi wszystkich świateł, dwie liczby połysku |
   | `drawHud(const game::Round& round, const game::GameplaySettings& settings)` (nie panel, sekcja 5.9) | wszystko: HUD tylko pokazuje | nic |

   Jedna sygnatura mówi mniej, niż bym chciał: `drawGameplayPanel` dostaje `game::Round&` bez `const`, choć zmienia w rundzie jedno pole, ładunek baterii. Powodem jest suwak `ImGui::SliderFloat("Battery", &round.battery, ...)`, który pisze przez wskaźnik: dzięki niemu migotanie słabej baterii i ciemność pustej da się obejrzeć bez czekania trzech minut. Że reszta rundy jest tylko wypisywana, mówi komentarz w `GameplayPanel.hpp` i podział w pliku `.cpp`: funkcja pomocnicza `drawRoundState` bierze `const game::Round&`, a referencję bez `const` dostaje tylko `drawBattery`.

   Ta sama umowa obowiązuje w polach `DebugContext` (niżej).

**`DebugContext`: jedna struktura zamiast listy parametrów.** `DebugUI::draw` ma jeden parametr:

```cpp
void draw(const DebugContext& context);
```

a wszystko, co panele pokazują i edytują, jest zebrane w strukturze z [`DebugContext.hpp`](../../src/debug/DebugContext.hpp):

```cpp
struct DebugContext {
    /// Frame clock, read only: FPS and frame time.
    const core::Time& time;
    /// Window, read only: sizes and OpenGL driver info.
    const core::Window& window;
    /// Clear color (red, green, blue in the range 0 to 1), editable: the background
    /// where the sky is not drawn.
    std::array<float, 3>& clearColor;
    /// Camera of the game, editable: angles and projection.
    scene::Camera& camera;
    /// Mouse look sensitivity in degrees per screen coordinate unit, editable.
    float& mouseSensitivity;
    /// Shader program of the scene without lighting (textured models), editable: the
    /// Shaders panel reloads it.
    gfx::Shader& texturedShader;
    /// Shader program of the lines of the collision boxes and spheres, editable:
    /// reloaded like texturedShader.
    gfx::Shader& colorShader;
    /// The player, editable: position, speeds and the noclip mode.
    game::Player& player;
    /// Request for the next maze, editable: size, seed and the "regenerate" flag.
    game::MazeSettings& mazeSettings;
    /// The maze in play, read only: its plan, its collision boxes and its terrain.
    const game::MazeWorld& mazeWorld;
    /// Loaded models and textures, editable: the Assets panel changes the filtering.
    assets::AssetCache& assets;
    /// What the textured shader shows (picture, normals or UVs), editable.
    game::ViewMode& viewMode;
    /// Whether the collision boxes and spheres are drawn as lines, editable.
    bool& drawColliders;
    /// Shader program of the lit scene, lighting per fragment, editable: reloaded like
    /// texturedShader.
    gfx::Shader& litShader;
    /// Shader program of the lit scene, lighting per vertex, editable: reloaded like
    /// texturedShader.
    gfx::Shader& gouraudShader;
    /// The lighting mode, the settings of every light and the normal mapping switch,
    /// editable.
    game::LightingSettings& lighting;
    /// The numbers of the rules of a round and the request for a restart, editable.
    game::GameplaySettings& gameplay;
    /// The round in play: the HUD and the panels show it. Editable for one thing, the
    /// charge of the battery (the Gameplay panel).
    game::Round& round;
    /// Shader program of the sky, editable: reloaded like texturedShader.
    gfx::Shader& skyboxShader;
    /// The switch and the brightness of the sky, editable.
    game::SkyboxSettings& skybox;
    /// Shader program of the grass, editable: reloaded like texturedShader.
    gfx::Shader& grassShader;
    /// The height scale and the wireframe switch of the terrain, editable.
    game::TerrainSettings& terrain;
    /// The switch, the density, the blade height and the wind of the grass, editable.
    game::GrassSettings& grass;
    /// How many tufts of grass are drawn. A plain number, copied when the context is
    /// built: the panels only show it.
    std::size_t grassTuftCount;
    /// Shader program of the composite pass, editable: reloaded like texturedShader.
    gfx::Shader& compositeShader;
    /// Shader program of the attachment previews, editable: reloaded like texturedShader.
    gfx::Shader& previewShader;
    /// The exposure and the tone mapping of the composite pass, the preview switch, the
    /// range of the depth preview and the settings of the bloom, the fog and the
    /// vignette, editable.
    game::PostProcessSettings& postProcessSettings;
    /// The framebuffers of the frame, read only: sizes, formats and the previews of
    /// the attachments of the scene framebuffer and of the bloom.
    const game::PostProcess& postProcess;
    /// Shader program of the bright pass of the bloom, editable: reloaded like
    /// texturedShader.
    gfx::Shader& brightPassShader;
    /// Shader program of the blur passes of the bloom, editable: reloaded like
    /// texturedShader.
    gfx::Shader& blurShader;
    /// Shader program of the depth pass of the shadow maps, editable: reloaded like
    /// texturedShader.
    gfx::Shader& shadowDepthShader;
    /// The settings of the shadows of the moon (switch, resolution, bias, PCF,
    /// strength) and the preview switch of its shadow map, editable.
    game::ShadowSettings& moonShadowSettings;
    /// The shadow map of the moon, read only: its size, its format and its preview.
    const game::ShadowMap& moonShadowMap;
    /// The view and the projection of the moon in the last frame, read only: how much
    /// ground its shadow map covers.
    const scene::LightSpace& moonLightSpace;
    /// The settings of the shadows of the flashlight and the preview switch of its
    /// shadow map, editable like the ones of the moon.
    game::ShadowSettings& flashlightShadowSettings;
    /// The shadow map of the flashlight, read only: its size, its format and its
    /// preview.
    const game::ShadowMap& flashlightShadowMap;
    /// The view and the projection of the flashlight in the last frame, read only: how
    /// much its shadow map covers.
    const scene::LightSpace& flashlightLightSpace;
    /// Whether the shadow map of the flashlight was drawn in the last frame: not with
    /// its shadows switched off, and not while the flashlight is off. A plain value,
    /// copied when the context is built: the Shadows panel only shows it.
    bool flashlightShadowDrawn;
};
```

Trzydzieści cztery pola w kolejności deklaracji i ich właściciele:

| # | Pole | Typ | Skąd pochodzi (`main.cpp`) | Kto czyta albo pisze |
|---|---|---|---|---|
| 1 | `time` | `const core::Time&` | `time()`, pole `core::Application` | Renderer (odczyt) |
| 2 | `window` | `const core::Window&` | `window()`, pole `core::Application` | Renderer (odczyt) |
| 3 | `clearColor` | `std::array<float, 3>&` | `clearColor()`, `m_clearColor` (od M7 wartość sRGB) | Renderer (edycja) |
| 4 | `camera` | `scene::Camera&` | `camera()`, `m_camera` | Camera (edycja), Maze (odczyt) |
| 5 | `mouseSensitivity` | `float&` | `mouseSensitivity()`, `m_mouseSensitivity` | Camera (edycja) |
| 6 | `texturedShader` | `gfx::Shader&` | `texturedShader()`, `m_texturedShader` (scena bez oświetlenia) | Shaders (`reload()`) |
| 7 | `colorShader` | `gfx::Shader&` | `colorShader()`, `m_colorShader` (linie pudełek i sfer kolizji) | Shaders (`reload()`) |
| 8 | `player` | `game::Player&` | `player()`, `m_player` | Camera i Collision (edycja), Maze (odczyt) |
| 9 | `mazeSettings` | `game::MazeSettings&` | `mazeSettings()`, `m_mazeSettings` | Maze (edycja) |
| 10 | `mazeWorld` | `const game::MazeWorld&` | `mazeWorld()`, `m_mazeWorld` | Maze i Collision (odczyt), Terrain (odczyt pola `terrain`) |
| 11 | `assets` | `assets::AssetCache&` | `assets()`, `m_assets` | Assets (`setFilter`, `setAnisotropy`, listy) |
| 12 | `viewMode` | `game::ViewMode&` | `viewMode()`, `m_viewMode` | Assets (edycja) |
| 13 | `drawColliders` | `bool&` | `drawColliders()`, `m_drawColliders` | Collision (edycja) |
| 14 | `litShader` | `gfx::Shader&` | `litShader()`, `m_litShader` (scena z oświetleniem liczonym dla fragmentu) | Shaders (`reload()`) |
| 15 | `gouraudShader` | `gfx::Shader&` | `gouraudShader()`, `m_gouraudShader` (scena z oświetleniem liczonym dla wierzchołka) | Shaders (`reload()`) |
| 16 | `lighting` | `game::LightingSettings&` | `lighting()`, `m_lighting` | Lights (edycja wszystkich pól poza `mode` i `normalMapping`), Renderer (edycja pola `mode`), Assets (edycja pola `normalMapping`) |
| 17 | `gameplay` | `game::GameplaySettings&` | `gameplaySettings()`, `m_gameplay` | Gameplay (edycja), HUD (odczyt progu słabej baterii) |
| 18 | `round` | `game::Round&` | `round()`, `m_round` | Gameplay (odczyt, edycja jednego pola `battery`), Maze, Collision, Lights i HUD (odczyt) |
| 19 | `skyboxShader` | `gfx::Shader&` | `skyboxShader()`, `m_skyboxShader` (niebo) | Shaders (`reload()`) |
| 20 | `skybox` | `game::SkyboxSettings&` | `skyboxSettings()`, `m_skyboxSettings` | Renderer (edycja pól `enabled` i `brightness`) |
| 21 | `grassShader` | `gfx::Shader&` | `grassShader()`, `m_grassShader` (trawa, program z shaderem geometrii) | Shaders (`reload()`) |
| 22 | `terrain` | `game::TerrainSettings&` | `terrainSettings()`, `m_terrainSettings` | Terrain (edycja pól `heightScale`, `wireframe` i `rebuild`) |
| 23 | `grass` | `game::GrassSettings&` | `grassSettings()`, `m_grassSettings` | Grass (edycja wszystkich pól) |
| 24 | `grassTuftCount` | `std::size_t` (wartość, nie referencja) | `grassTuftCount()`, czyli `m_grassRenderer.tuftCount()` | Grass (odczyt) |
| 25 | `compositeShader` | `gfx::Shader&` | `compositeShader()`, `m_compositeShader` (przebieg składający: obraz HDR do okna) | Shaders (`reload()`) |
| 26 | `previewShader` | `gfx::Shader&` | `previewShader()`, `m_previewShader` (podglądy załączników) | Shaders (`reload()`) |
| 27 | `postProcessSettings` | `game::PostProcessSettings&` | `postProcessSettings()`, `m_postProcessSettings` | Framebuffers (edycja pól `exposure`, `toneMapping`, `depthPreviewRange`, `previews`, od drugiej części M7 czterech pól struktury `bloom`, a od trzeciej pięciu pól struktury `fog` i trzech pól struktury `vignette`), `DebugUI::draw` (zeruje `previews` co klatkę) |
| 28 | `postProcess` | `const game::PostProcess&` | `postProcess()`, `m_postProcess` | Framebuffers (odczyt: rozmiary, formaty, obrazy podglądu, od drugiej części M7 także `bloomDrawn()`, `bloomTarget()` i dwa podglądy bloomu) |
| 29 | `brightPassShader` | `gfx::Shader&` | `brightPassShader()`, `m_brightPassShader` (przebieg jasności bloomu) | Shaders (`reload()`) |
| 30 | `blurShader` | `gfx::Shader&` | `blurShader()`, `m_blurShader` (rozmycie bloomu) | Shaders (`reload()`) |
| 31 | `shadowDepthShader` | `gfx::Shader&` | `shadowDepthShader()`, `m_shadowDepthShader` (przebieg głębi map cieni) | Shaders (`reload()`) |
| 32 | `moonShadowSettings` | `game::ShadowSettings&` | `moonShadowSettings()`, `m_moonShadow` | Shadows (edycja pól `enabled`, `resolution`, `constantBias`, `slopeBias`, `hardwareFilter`, `pcf`, `pcfRadius`, `strength` i `preview`), `DebugUI::draw` (zeruje `preview` co klatkę) |
| 33 | `moonShadowMap` | `const game::ShadowMap&` | `moonShadowMap()`, `m_moonShadowMap` | Shadows (odczyt: `target()` dla rozmiaru i formatu, `preview()` dla obrazu) |
| 34 | `moonLightSpace` | `const scene::LightSpace&` | `moonLightSpace()`, `m_moonLightSpace` (liczone od nowa w każdej klatce) | Shadows (odczyt pola `extent`: szerokość, wysokość i głębokość pudełka światła w metrach) |
| 35 | `flashlightShadowSettings` | `game::ShadowSettings&` | `flashlightShadowSettings()`, `m_flashlightShadow` (start: `game::flashlightShadowDefaults()`) | Shadows, zakładka `Flashlight` (edycja tych samych pól co dla księżyca), `DebugUI::draw` (zeruje `preview` co klatkę) |
| 36 | `flashlightShadowMap` | `const game::ShadowMap&` | `flashlightShadowMap()`, `m_flashlightShadowMap` | Shadows (odczyt: `target()` i `preview()`) |
| 37 | `flashlightLightSpace` | `const scene::LightSpace&` | `flashlightLightSpace()`, `m_flashlightLightSpace` (liczone od nowa w każdej klatce, także przy wyłączonych cieniach) | Shadows (odczyt pól `kind`, `extent` i `farPlane`) |
| 38 | `flashlightShadowDrawn` | `bool` | `flashlightShadowDrawn()`, `m_flashlightShadowDrawn` (kopia wartości, jak `grassTuftCount`) | Shadows (czy w miejscu obrazu stoi obraz, czy `(not drawn)`) |

Kolejność pól to historia: pierwsze pięć pochodzi z M0 i M1, osiem następnych doszło w M2 + M3, trzy w M4, dwa w M5, dwa w pierwszej części M6, cztery w drugiej (teren i trawa), cztery w pierwszej części M7 (bufor HDR), dwa w drugiej części M7 (bloom), cztery w czwartej (cienie księżyca), a cztery ostatnie w piątej (cień latarki), i wszystkie były dopisywane **na końcu**. Dlatego shadery są rozrzucone po strukturze (pola 6, 7, 14, 15, 19, 21, 25, 26, 29, 30 i 31) i nie stoją obok siebie. Bloom nie dostał własnego pola z ustawieniami: jego cztery liczby są strukturą `bloom` wewnątrz `PostProcessSettings`, więc przyszły przez istniejące pole 27. Tą samą drogą przyszły w trzeciej części M7 mgła i winieta: struktury `fog` i `vignette` są polami `PostProcessSettings`, więc w `DebugContext` zmienił się tylko komentarz przy polu 27 (pól było nadal trzydzieści), a w `main.cpp` nie doszła żadna linia. Cienie księżyca z czwartej części M7 poszły inną drogą: ich ustawienia to osobna struktura `game::ShadowSettings`, która nie jest częścią `PostProcessSettings` ani `LightingSettings`, a mapa cieni i pudełko światła to osobne obiekty gry. Stąd cztery nowe pola (31 do 34) i cztery nowe linie w `main.cpp`, i po czwartej części pól było trzydzieści cztery. Cień latarki z piątej części M7 jest tym samym mechanizmem po raz drugi: ta sama struktura ustawień, ta sama klasa mapy i ta sama struktura przestrzeni światła, tylko osobne obiekty gry. Stąd cztery kolejne pola (35 do 38) i cztery kolejne linie w `main.cpp`, a dziś pól jest trzydzieści osiem. Ostatnie z nich, `flashlightShadowDrawn`, jest drugim polem będącym wartością (obok `grassTuftCount`): panel musi wiedzieć, czy mapa latarki była w tej klatce rysowana, a tego nie widać w ustawieniach (latarka zgaszona klawiszem F albo pustą baterią nie ma mapy, choć jej cienie są włączone). Skutek uboczny dopisywania na końcu: liczba `grassTuftCount` nie jest już ostatnim polem struktury. Do M4 pól z M0 i M1 było sześć: czwartym było `shader`, program kostki z M1. W M5 kostka zniknęła z gry razem z programem `basic`, więc zniknęło też pole, akcesor gry i linia w `main.cpp`, a numery następnych pól przesunęły się o jeden. Komentarz przy polu `colorShader` mówi dziś o jednym użytkowniku tego programu: liniach pudełek i sfer kolizji (znaczniki świateł z M4, które też nim rysowałem, zostały usunięte). Pole `lighting` obsługuje trzy panele: z mapami normalnych nie doszło nowe pole kontekstu, tylko nowe pole struktury `LightingSettings`, które `DebugUI::draw` podaje panelowi Assets jako `context.lighting.normalMapping`. Dwa pola z M5 obsługują razem pięć odbiorców: cztery panele i HUD. Kolejność ma skutek w `main.cpp`: inicjalizatory desygnowane muszą iść w kolejności deklaracji pól, więc linia `.litShader = litShader(),` stoi po `.drawColliders = drawColliders(),`, a nie obok `.colorShader = colorShader(),` (pułapka 15). Pole `moveSpeed` z M1 zniknęło wcześniej: prędkości są dziś trzy i należą do gracza (`player.walkSpeed`, `player.sprintSpeed`, `player.flySpeed`), więc przychodzą razem z polem `player`.

Powód jest praktyczny. Gdyby `draw` brało każdą wartość osobno (`draw(time, window, clearColor, shader)`), każdy nowy panel z nowymi danymi wydłużałby listę parametrów w trzech miejscach naraz: w deklaracji w `DebugUI.hpp`, w definicji w `DebugUI.cpp` i w wywołaniu w `main.cpp`. Ze strukturą sygnatura `draw` się nie zmienia: dochodzi jedno pole w `DebugContext` i jedna linia w `main.cpp`. Tak doszedł w M1 panel Shaders: jedno pole i jedna linia, bez zmiany w `DebugUI.hpp`. Trzy panele z M2 + M3 dołożyły osiem pól i osiem linii, oświetlenie z M4 trzy pola i trzy linie, rozgrywka z M5 dwa pola i dwie linie (dla panelu Gameplay, trzech starszych paneli i HUD), a niebo z M6 znów dwa pola i dwie linie (program dla panelu Shaders i ustawienia dla panelu Renderer). Teren i trawa, druga część M6, dołożyły cztery pola i cztery linie: program trawy dla panelu Shaders, ustawienia dla paneli Terrain i Grass oraz liczbę kępek. Teren, który panel Terrain czyta, nie potrzebował nowego pola: jest częścią `mazeWorld` (`context.mazeWorld.terrain`). Pierwsza część M7 dołożyła cztery pola i cztery linie: dwa programy dla panelu Shaders oraz ustawienia i framebuffery dla panelu Framebuffers. Druga część dołożyła dwa pola i dwie linie (dwa programy bloomu dla panelu Shaders), a trzecia żadnego: osiem kontrolek mgły i winiety pisze przez pole `postProcessSettings`, które już było. Czwarta część (cienie księżyca) dołożyła cztery pola i cztery linie: program głębi dla panelu Shaders oraz ustawienia cieni, mapę cieni i pudełko światła dla panelu Shadows. Sygnatura `draw` w `DebugUI.hpp` jest przez cały ten czas ta sama. Rzeczy, które trzeba umieć wyjaśnić:

1. **Dlaczego referencje.** Struktura niczego nie posiada i, poza dwiema wartościami, niczego nie kopiuje. Każde pole wskazuje na obiekt, którego właścicielem jest aplikacja: `time` i `window` to pola `core::Application`, `clearColor` to `game::NightMazeApp::m_clearColor`, a z pozostałych trzydziestu pięciu trzydzieści cztery to referencje do pól tej samej klasy (tabela wyżej). Wyjątkiem są dwa pola. `grassTuftCount` to zwykła liczba `std::size_t`, kopiowana przy budowie kontekstu. Od piątej części M7 drugim jest `flashlightShadowDrawn`, `bool`, kopiowany tak samo (jego akcesor zwraca wartość pola `m_flashlightShadowDrawn`, a panel tylko ją pokazuje). Gra nie trzyma jej w żadnym polu, do którego dałoby się zrobić referencję: akcesor `grassTuftCount()` pyta o nią `GrassRenderer::tuftCount()` i zwraca wartość, a panel Grass tylko ją wypisuje, więc kopia niczego nie psuje. Kopia `m_clearColor` w strukturze byłaby bezużyteczna, bo panel edytowałby kopię, a `glClearColor` dalej dostawałby oryginał. Referencja zamiast wskaźnika oznacza też, że pole nie może być puste: nie ma `nullptr` do sprawdzania.
2. **Dlaczego jest budowana co klatkę.** `main.cpp` tworzy obiekt tymczasowy `debug::DebugContext{...}` bezpośrednio w wywołaniu `draw`. Koszt to trzydzieści trzy referencje, czyli trzydzieści trzy adresy, i jedna liczba. W zamian nie ma żadnego stanu do przechowywania i pilnowania: `DebugUI` nie zapamiętuje kontekstu, a `DebugNightMazeApp` nie ma dodatkowego pola.
3. **Czas życia (lifetime).** Obiekt tymczasowy żyje do końca pełnego wyrażenia, czyli do średnika po wywołaniu `draw`. To wystarcza, bo panele używają go tylko w trakcie `draw`. Struktury nie wolno zachować na później (na przykład w polu klasy): przeżyłaby klatkę, w której powstała, a jej referencje mogłyby wskazywać na obiekty już zniszczone.
4. **Dlaczego inicjalizatory desygnowane (designated initializers, C++20).** Zapis `.time = time()` nazywa pole, do którego trafia wartość, więc wywołanie czyta się bez zaglądania do definicji struktury. Pola referencyjnego nie da się pominąć: referencja musi zostać zainicjalizowana, więc brak pola na liście jest błędem kompilacji, a nie cichą wartością domyślną (sekcja 7, pułapki 14 i 15).
5. **Dlaczego panel nadal dostaje jawne parametry.** `DebugUI::draw` woła `drawRendererPanel(context.time, context.window, context.clearColor, context.lighting.mode, context.skybox)`, `drawCameraPanel(context.camera, context.player, context.mouseSensitivity)` i tak dalej, a nie `drawRendererPanel(context)`. Dzięki temu sygnatura panelu dalej mówi, co dokładnie czyta i co edytuje (decyzja 3 wyżej). Panel biorący cały `DebugContext` miałby dostęp do wszystkiego i z jego sygnatury nic by nie wynikało. Ta sama zasada działa o poziom niżej: panel Renderer dostaje `context.lighting.mode`, czyli jedno pole struktury `LightingSettings`, a nie całą strukturę. Zmienia tryb oświetlenia i nie ma jak ruszyć kolorów ani natężeń świateł, które należą do panelu Lights.
6. **`const DebugContext&` nie robi z pól stałych.** `draw` bierze kontekst przez `const&`, a mimo to panel zmienia kolor tła. To nie jest obejście `const`. Stałość obiektu dotyczy jego własnych pól, a polem jest tu **referencja**, nie tablica. Referencji i tak nie da się przestawić na inny obiekt, więc `const` na strukturze niczego w niej nie zmienia, i nie przechodzi na obiekt, na który referencja wskazuje. O tym, czy przez pole wolno pisać, decyduje wyłącznie typ pola: `const core::Time&`, `const game::MazeWorld&`, `const game::PostProcess&`, `const game::ShadowMap&` i `const scene::LightSpace&` są tylko do odczytu, `std::array<float, 3>&`, `gfx::Shader&`, `game::ShadowSettings&`, `scene::Camera&`, `game::Player&`, `game::LightingSettings&`, `game::GameplaySettings&`, `game::Round&`, `float&` i `bool&` są edytowalne, niezależnie od tego, czy sama struktura jest `const`. Tak samo zachowuje się wskaźnik: w stałym obiekcie pole `float* p` staje się `float* const p` (nie można przestawić wskaźnika), ale `*p = 1.0F` nadal się kompiluje.

Nagłówki `DebugContext.hpp`, `DebugUI.hpp`, `Hud.hpp` i dwanaście nagłówków paneli nie dołączają ani `imgui.h`, ani nagłówków `core`, `gfx`, `scene`, `game` i `assets` (jeden wyjątek z M7 opisuję na końcu akapitu): wystarczają im deklaracje wyprzedzające (forward declarations), bo używają tych typów tylko przez referencję albo wskaźnik. `DebugContext.hpp` deklaruje tak wszystkie swoje typy: `class AssetCache;` w `assets`, `class Time;` i `class Window;` w `core`, `enum class ViewMode;`, `struct GameplaySettings;`, `struct GrassSettings;`, `struct LightingSettings;`, `struct MazeSettings;`, `struct MazeWorld;`, `struct Player;`, od M7 `class PostProcess;` i `struct PostProcessSettings;`, `struct Round;`, od czwartej części M7 `class ShadowMap;` i `struct ShadowSettings;`, `struct SkyboxSettings;` i `struct TerrainSettings;` w `game`, `class Shader;` w `gfx` oraz `struct Camera;` i, od czwartej części M7, `struct LightSpace;` w `scene`. Wyliczenie `enum class` da się zadeklarować z wyprzedzeniem, bo jego typ bazowy jest znany (domyślnie `int`). Tak samo `RendererPanel.hpp` deklaruje `enum class LightingMode;`, `LightsPanel.hpp` `struct LightingSettings;` i `struct Round;`, a `GameplayPanel.hpp` i `Hud.hpp` po dwie: `struct GameplaySettings;` i `struct Round;`. `TerrainPanel.hpp` deklaruje `class Terrain;` i `struct TerrainSettings;` (teren jest klasą, bo pilnuje swojej siatki, ustawienia są strukturą), a `GrassPanel.hpp` samo `struct GrassSettings;`. `FramebuffersPanel.hpp` deklaruje `class PostProcess;` i `struct PostProcessSettings;`, `ShadowsPanel.hpp` `class ShadowMap;` i `struct ShadowSettings;` w `game` oraz `struct LightSpace;` w `scene`, a `AssetsPanel.hpp` od M7 także `class RawTextureSampler;` w `debug`. Jedyne dołączenia w tych nagłówkach to `<array>` (w `DebugContext.hpp` i `RendererPanel.hpp`, dla `std::array<float, 3>`), `<cstddef>` (w `DebugContext.hpp` i `GrassPanel.hpp`, dla `std::size_t` liczby kępek), `<span>` (w `ShadersPanel.hpp`) i `<vector>` (w `DebugUI.hpp`, dla pola `m_fontBytes` z bajtami czcionki). Słowo `struct` albo `class` w deklaracji zgadza się z definicją (`struct Camera`, `struct Player`, `class AssetCache`). `DebugUI.hpp` deklaruje `class Window;` (dla konstruktora) i `struct DebugContext;` (dla `draw`). Pełną definicję `DebugContext` dołączają tylko `DebugUI.cpp`, które czyta pola, i `main.cpp`, które strukturę buduje. W tych piętnastu nagłówkach ImGui nie ma, więc `main.cpp`, które dołącza `DebugUI.hpp` i `DebugContext.hpp`, nie zależy od tej biblioteki. Wyjątek z pierwszej części M7: `DebugUI.hpp` dołącza `debug/RawTextureSampler.hpp`, bo klasa `DebugUI` ma pole tego typu przez wartość, a do pola przez wartość deklaracja wyprzedzająca nie wystarcza (kompilator musi znać rozmiar). Ten nagłówek dołącza z kolei `<glad/gl.h>` dla typu `GLuint`. `main.cpp` widzi więc pośrednio GLAD, ale nadal nie widzi ImGui. Wyjątkiem są dwa nagłówki wewnętrzne modułu, `Theme.hpp` i `PanelLayout.hpp`: pokazują typy ImGui (`ImVec4`, `ImVec2`), więc dołączają `<imgui.h>`. Dołączają je tylko pliki `.cpp` z `src/debug/`, które i tak używają ImGui, więc reguła "reszta projektu nie zna ImGui" zostaje prawdziwa.

**Lista shaderów: tablica wskaźników widziana jako `std::span`.** Jedno z dwunastu wywołań paneli w `DebugUI::draw` wymaga wyjaśnienia:

```cpp
        // The Shaders panel takes a list, so that a new program is one more entry here
        // and no change in the panel. The array holds pointers, because a reference
        // cannot be an element of an array.
        constexpr int SHADER_COUNT = 11;
        const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
            &context.texturedShader,  &context.colorShader,      &context.litShader,
            &context.gouraudShader,   &context.skyboxShader,     &context.grassShader,
            &context.compositeShader, &context.previewShader,    &context.brightPassShader,
            &context.blurShader,      &context.shadowDepthShader};
        drawShadersPanel(shaders);
```

| Fragment | Znaczenie |
|---|---|
| `constexpr int SHADER_COUNT = 11;` | liczba programów gry: `textured`, `color`, `lit`, `gouraud`, `skybox`, `grass`, od pierwszej części M7 `composite` i `preview`, od drugiej `bright` i `blur`, a od czwartej `shadow_depth`. Nazwana stała zamiast gołej jedenastki w typie tablicy |
| `std::array<gfx::Shader*, SHADER_COUNT>` | tablica jedenastu **wskaźników**. Tablicy referencji w C++ nie ma (referencja nie jest obiektem, nie ma adresu ani rozmiaru), więc lista obiektów, których nie posiadam, to lista wskaźników |
| `&context.texturedShader` | adres obiektu, na który wskazuje pole referencyjne, czyli adres `NightMazeApp::m_texturedShader`. Żaden z jedenastu nie może być pusty |
| `const std::array<...> shaders` | stała jest tablica (jej jedenaście wskaźników), a nie shadery |
| kolejność elementów | w tej kolejności panel wypisuje programy, od góry do dołu: `textured`, `color`, `lit`, `gouraud`, `skybox`, `grass`, `composite`, `preview`, `bright`, `blur`, `shadow_depth` |
| `drawShadersPanel(shaders)` | parametr ma typ `std::span<gfx::Shader* const>`: widok na ciąg stałych wskaźników do niestałych shaderów. `std::array` zamienia się na `std::span` bez kopiowania. `const` stoi po gwiazdce, więc dotyczy wskaźnika: panel nie może podmienić elementu listy, ale może zawołać `reload()` na shaderze |

Kolejny program to jeden wpis więcej w tej tablicy (i większe `SHADER_COUNT`), bez zmiany w panelu. Tak doszły w M4 programy `lit` i `gouraud`: dwa wpisy, `SHADER_COUNT` z 3 na 5, a kod pętli w `ShadersPanel.cpp` został ten sam. W M5 lista skróciła się tą samą drogą: zniknął wpis programu kostki, `SHADER_COUNT` spadło z 5 na 4, a panel znów się nie zmienił. W pierwszej części M6 doszedł wpis programu nieba: `SHADER_COUNT` wróciło do 5, też bez zmiany w panelu. W drugiej części M6 doszedł wpis programu trawy i `SHADER_COUNT` wynosi 6. Tym razem panel się zmienił, ale nie pętla po liście: program trawy ma trzy pliki, więc funkcja `drawShaderStatus` składa linię z dwóch albo trzech nazw (sekcja 6). W pierwszej części M7 doszły dwa wpisy, `composite` i `preview`, i `SHADER_COUNT` wyniosło 8: panel znów się nie zmienił. W drugiej części M7 doszły dwa kolejne, `bright` i `blur` (bloom), i `SHADER_COUNT` wyniosło 10. Wszystkie cztery programy mają ten sam plik shadera wierzchołków, więc ich linie w panelu zaczynają się tą samą nazwą: `composite.vert + composite.frag`, `composite.vert + preview.frag`, `composite.vert + bright.frag` i `composite.vert + blur.frag` (tak wynika z kodu panelu, który wypisuje nazwy plików. Nikt tego jeszcze nie oglądał w działającym programie, a przeładowania z dziesięcioma programami nikt nie klikał). W czwartej części M7 (cienie księżyca) doszedł jeden wpis, `shadow_depth`, program przebiegu głębi map cieni, i `SHADER_COUNT` wynosi 11. Panel znów się nie zmienił. Ten program ma własną parę plików, więc jego linia, jedenasta i ostatnia, brzmi `shadow_depth.vert + shadow_depth.frag` (tak wynika z kodu panelu. Przeładowania z jedenastoma programami nikt nie klikał). Liczba elementów w klamrach nie jest sprawdzana względem `SHADER_COUNT` w jedną stronę: za dużo elementów to błąd kompilacji, ale za mało zostawia na końcu tablicy wskaźnik pusty (`nullptr`), który panel wyłuskuje w każdej klatce (`drawShaderStatus(*shader)`). Dlatego stałą i listę zmieniam zawsze razem.

### 5.3 Panel Renderer linia po linii

```cpp
namespace {

// The entries of the list, in the order of the enum game::LightingMode: the number of
// the chosen entry is the value of the enum. ImGui wants the entries in one string, each
// ended by a zero character.
constexpr const char* LIGHTING_MODE_ITEMS = "Unlit\0Gouraud\0Phong\0Blinn-Phong\0";

// Range of the slider of the sky brightness. 1 leaves the sky pictures as they are, 0 is
// a black sky. The pictures are dark, so the range goes well above 1.
constexpr float MIN_SKY_BRIGHTNESS = 0.0F;
constexpr float MAX_SKY_BRIGHTNESS = 6.0F;

} // namespace

void drawRendererPanel(const core::Time& time, const core::Window& window,
                       std::array<float, 3>& clearColor, game::LightingMode& lightingMode,
                       game::SkyboxSettings& skybox) {
    // The place and the size of the panel the first time the program runs: the top left
    // corner of the window (the constant is in PanelLayout.hpp). The call counts only when
    // imgui.ini has no entry for this panel yet. After that the user decides where the
    // panel is.
    placePanelOnFirstUse(RENDERER_PLACEMENT);
    // Begin returns false when the panel is collapsed or hidden behind another tab.
    // End must be called in both cases.
    if (ImGui::Begin("Renderer")) {
        ImGui::Text("FPS: %.1f", time.fps());
        ImGui::Text("Frame time: %.2f ms", time.frameTimeMs());

        const core::Size framebuffer = window.framebufferSize();
        const core::Size windowSize = window.windowSize();
        ImGui::Text("Framebuffer: %d x %d px", framebuffer.width, framebuffer.height);
        ImGui::Text("Window: %d x %d", windowSize.width, windowSize.height);

        ImGui::Separator();
        ImGui::TextWrapped("OpenGL: %s", window.glVersion().c_str());
        ImGui::TextWrapped("GPU: %s", window.glRenderer().c_str());

        ImGui::Separator();
        // ColorEdit3 reads and writes three floats through the pointer.
        ImGui::ColorEdit3("Clear color", clearColor.data());

        // How the maze is shaded. Combo works on the number of the chosen entry and
        // returns true in the frame in which the user picked another one. Gouraud
        // computes the light per vertex, Phong and Blinn-Phong per fragment.
        int lightingModeIndex = static_cast<int>(lightingMode);
        if (ImGui::Combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS)) {
            lightingMode = static_cast<game::LightingMode>(lightingModeIndex);
        }

        // The sky. Switched off, the clear colour above is the background again.
        // Checkbox and SliderFloat write through the pointers they are given.
        ImGui::Checkbox("Skybox", &skybox.enabled);
        ImGui::SetItemTooltip("The night sky (a cube map). The painted moon stands where the\n"
                              "default moon light comes from and does not follow the Moon\n"
                              "sliders of the Lights panel.");
        ImGui::SliderFloat("Sky brightness", &skybox.brightness, MIN_SKY_BRIGHTNESS,
                           MAX_SKY_BRIGHTNESS);
    }
    ImGui::End();
}
```

- `placePanelOnFirstUse(RENDERER_PLACEMENT)` ustawia miejsce, rozmiar i stan zwinięcia **następnego** okna, czyli tego, które zaraz otworzy `Begin`, ale tylko wtedy, gdy ImGui nie ma dla tego okna zapisanych danych w `imgui.ini`. Stała `RENDERER_PLACEMENT` i funkcja są w `PanelLayout.hpp` i `PanelLayout.cpp`, wspólnych dla dwunastu paneli (sekcja 5.7). Plik panelu ma dziś trzy własne stałe, `LIGHTING_MODE_ITEMS` oraz `MIN_SKY_BRIGHTNESS` i `MAX_SKY_BRIGHTNESS`, w anonimowej przestrzeni nazw: są widoczne tylko w tym pliku.
- `ImGui::Begin("Renderer")` otwiera okno ImGui o tym tytule. Tytuł jest jednocześnie **identyfikatorem**: po nim ImGui pamięta pozycję i dokowanie panelu. Zwraca `false`, gdy panel jest zwinięty albo schowany za inną zakładką, i wtedy pomijam zawartość (oszczędność pracy).
- `ImGui::End()` stoi **poza** `if` i wykonuje się zawsze. Każde `Begin` musi mieć swoje `End`, niezależnie od zwróconej wartości.
- `ImGui::Text` działa jak `printf`: `%.1f` to liczba z jedną cyfrą po przecinku, `%d` liczba całkowita, `%s` napis w stylu C, dlatego przy `std::string` potrzebne jest `.c_str()`.
- `ImGui::TextWrapped` zawija długi tekst (nazwa karty graficznej bywa długa).
- `ImGui::ColorEdit3("Clear color", clearColor.data())` dostaje wskaźnik na pierwszy z trzech `float`ów (`.data()` zwraca `float*`) i przez ten wskaźnik **czyta i zapisuje** kolor. Nie ma tu żadnego "zdarzenia zmiany": w następnej klatce `NightMazeApp::onRender` po prostu przekaże do `glClearColor` już zmienione wartości. Od pierwszej części M7 nie przekazuje ich wprost: kolor w próbniku jest wartością sRGB, a bufor sceny trzyma wartości liniowe, więc `onRender` przelicza go najpierw przez `gfx::srgbToLinear`. Panel o tym nie wie i nie musi: pokazuje i edytuje to, co widzi człowiek, a po kodowaniu sRGB na końcu klatki tło ma na ekranie kolor wybrany w próbniku (ściśle: przy mapowaniu tonów `None` i ekspozycji 1. Przy domyślnej krzywej ACES tak ciemny kolor wychodzi jeszcze trochę ciemniej). Kolor startowy to dziś `{0.022F, 0.033F, 0.088F}` (do M6 `{0.01F, 0.015F, 0.04F}`), bardzo ciemny granat (pole `m_clearColor` w `NightMazeApp.hpp`). Do M5 był to kolor nieba. Od pierwszej części M6 niebo rysuje `game::Skybox`, więc kolor czyszczenia widać tylko wtedy, gdy pole `Skybox` niżej jest odznaczone albo obrazy nieba się nie wczytały. Jest celowo bliski kolorowi zenitu na obrazach, żeby wyłączenie nieba nie zmieniało nastroju sceny.

**Lista `Lighting`: tryb oświetlenia labiryntu.** To jedyna nowa kontrolka panelu w M4 i jedyny w całym programie przełącznik między cieniowaniem Gourauda, Phonga i Blinna-Phonga (pokaz tematu 7). Stała nad funkcją i trzy linie w bloku `if`:

| Linia | Co robi |
|---|---|
| `constexpr const char* LIGHTING_MODE_ITEMS = "Unlit\0Gouraud\0Phong\0Blinn-Phong\0";` | pozycje listy w **jednym** napisie. Każda kończy się znakiem zerowym `\0`, a po ostatnim jawnym `\0` kompilator dopisuje jeszcze zero kończące literał. ImGui czyta pozycje, aż trafi na dwa zera z rzędu ([`../libraries/imgui.md`](../libraries/imgui.md), sekcja 3.11) |
| `int lightingModeIndex = static_cast<int>(lightingMode);` | `ImGui::Combo` pracuje na **numerze** wybranej pozycji typu `int`, a pole gry ma typ `enum class game::LightingMode`. Wyliczenie z `class` nie zamienia się na `int` samo, stąd jawne rzutowanie. Zmienna lokalna powstaje w każdej klatce od nowa z bieżącej wartości pola, więc lista zawsze pokazuje to, co jest w grze |
| `ImGui::Combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS)` | rysuje listę rozwijaną z etykietą `Lighting`. Dostaje adres numeru i zapisuje przez niego nowy numer, gdy użytkownik wybierze inną pozycję. Zwraca `true` tylko w tej klatce, w której wybór się zmienił |
| `lightingMode = static_cast<game::LightingMode>(lightingModeIndex);` | numer wraca do wyliczenia i przez referencję trafia do `NightMazeApp::m_lighting.mode`. Przypisanie stoi w `if`, więc wykonuje się tylko przy zmianie |

**Dlaczego kolejność pozycji musi zgadzać się z wyliczeniem.** Numer pozycji na liście staje się wartością wyliczenia bez żadnej tablicy pośredniej. Wyliczenie w [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) wygląda tak:

```cpp
enum class LightingMode {
    Unlit = 0,  ///< no lighting: the texture as it is (the textured program)
    Gouraud,    ///< lighting computed for every vertex and blended across the triangle
    Phong,      ///< lighting computed for every fragment, highlight from the reflected ray
    BlinnPhong, ///< lighting computed for every fragment, highlight from the halfway vector
};
```

| Pozycja listy | Numer | Wartość `game::LightingMode` | Którym programem gra rysuje labirynt, bramę i kryształy |
|---|---|---|---|
| `Unlit` | 0 | `Unlit` | `textured` (bez oświetlenia) |
| `Gouraud` | 1 | `Gouraud` | `gouraud` (oświetlenie liczone w shaderze wierzchołków) |
| `Phong` | 2 | `Phong` | `lit` z `uSpecularModel` równym 0 |
| `Blinn-Phong` | 3 | `BlinnPhong` | `lit` z `uSpecularModel` równym 1. To tryb startowy (`LightingSettings::mode`) |

Pierwsza wartość ma jawne `= 0`, następne rosną o jeden. Gdyby ktoś dopisał nowy tryb w środku wyliczenia albo zamienił dwa napisy w `LIGHTING_MODE_ITEMS`, lista pokazywałaby jedną nazwę, a gra włączałaby inny tryb, bez błędu kompilacji i bez ostrzeżenia. Komentarze w obu plikach mówią o tej umowie wprost. Ten sam wzór ma lista `View mode` w panelu Assets.

Panel nie wybiera programu i nie woła niczego w OpenGL: zmienia jedną wartość, a o tym, czym rysować, decyduje `NightMazeApp::drawMaze` w następnej klatce ([`renderer/lighting-gouraud-phong.md`](renderer/lighting-gouraud-phong.md)). Jeden szczegół, który warto znać przed pokazem: gdy w panelu Assets wybrany jest widok `Normals as colour` albo `UVs as colour`, labirynt jest rysowany programem `textured` w każdym trybie oświetlenia, więc zmiana na liście `Lighting` nie zmienia wtedy sposobu cieniowania. Jeden ślad zostaje w widoku normalnych: pokazuje on normalne, którymi cieniowałby wybrany tryb, czyli normalne siatki w trybach `Unlit` i `Gouraud`, a normalne z map w trybach `Phong` i `Blinn-Phong` przy zaznaczonym `Normal mapping` (pułapka 33).

**Pole `Skybox` i suwak `Sky brightness`: niebo.** Dwie kontrolki z pierwszej części M6, pokaz tematu 8 (PRD, sekcja 3: "toggle skybox"). Stoją na końcu bloku `if`, pod listą `Lighting`:

| Linia | Co robi |
|---|---|
| `constexpr float MIN_SKY_BRIGHTNESS = 0.0F;`, `MAX_SKY_BRIGHTNESS = 6.0F;` | zakres suwaka jako nazwane stałe w anonimowej przestrzeni nazw. 1 zostawia (liniowe) kolory obrazów nieba bez zmiany, 0 to czarne niebo. Zakres wychodzi ponad 1, bo obrazy są ciemne. W pierwszej części M7 górna granica wzrosła z 3 do 6, razem z wartością startową pola `brightness` (z 1,0 na 2,2): mnożnik działa teraz na wartości liniowe w buforze HDR, a krzywa mapowania tonów dodatkowo ściemnia ciemne tony, więc niebo trzeba podnieść mocniej ([`renderer/skybox.md`](renderer/skybox.md)) |
| parametr `game::SkyboxSettings& skybox` | piąty parametr funkcji: referencja do `NightMazeApp::m_skyboxSettings`. Nagłówek panelu ma dla tego typu deklarację wyprzedzającą `struct SkyboxSettings;`, a plik `.cpp` dołącza `game/Skybox.hpp` |
| `ImGui::Checkbox("Skybox", &skybox.enabled);` | pole wyboru pisze przez wskaźnik prosto do pola `enabled`. Wyniku `Checkbox` (prawda w klatce zmiany) kod nie potrzebuje: nie ma nic do zrobienia w chwili przełączenia, bo `onRender` czyta pole w każdej klatce |
| `ImGui::SetItemTooltip(...)` | podpowiedź dla **ostatnio narysowanego** widżetu, czyli dla pola `Skybox`. Trzy linie tekstu połączone znakami `\n`: namalowany księżyc stoi tam, skąd leci domyślne światło księżyca, i nie podąża za suwakami `Moon` panelu Lights |
| `ImGui::SliderFloat("Sky brightness", &skybox.brightness, MIN_SKY_BRIGHTNESS, MAX_SKY_BRIGHTNESS);` | suwak pisze przez wskaźnik do pola `brightness`, które `Skybox::draw` wysyła do uniformu `uBrightness` |

Panel nadal nie woła niczego w OpenGL: zmienia dwie wartości, a o tym, czy i jak rysować niebo, decyduje koniec `NightMazeApp::onRender` w następnej klatce ([`renderer/skybox.md`](renderer/skybox.md), sekcje 5.6 i 6). Z niebem zmieniło się znaczenie kontrolki `Clear color` nad nimi: kolor czyszczenia widać już tylko przy odznaczonym polu `Skybox`.

Stan sprawdzenia: cztery tryby były obejrzane na zrzutach ekranu z Windowsa w M4 (2026-10-05), z trzech punktów widzenia. Samej listy nikt jeszcze nie klikał myszą. Pola `Skybox` i suwaka `Sky brightness` też nikt jeszcze nie dotknął myszą: niebo było oglądane na zrzucie ekranu przy wartościach startowych. Po pierwszej części M7 (nowy zakres suwaka, kolor tła jako sRGB) nikt tych kontrolek nie klikał.

### 5.4 Gdzie moduł jest wywoływany

Wszystkie sześć miejsc jest w `DebugNightMazeApp` w [`main.cpp`](../../src/main.cpp):

- Tworzenie: inicjalizator pola przy deklaracji, `debug::DebugUI m_debugUI{window()};`. Wykonuje się po zbudowaniu całej części bazowej, więc okno i kontekst już istnieją.
- Przełączanie: `if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) { m_debugUI.toggleVisible(); }` w `onRender`, czyli dokładnie raz na klatkę. Dlaczego nie w `onUpdate`, wyjaśnia [`core/input.md`](core/input.md), sekcja 5.5.
- Mysz dla ImGui: `m_debugUI.setMouseEnabled(!input().isCursorCaptured());` tuż przed `draw` (sekcja 5.6).
- Rysowanie: `m_debugUI.draw(debug::DebugContext{...});` z trzydziestoma czterema polami (sekcja 5.2), po powrocie z `game::NightMazeApp::onRender`. To jedno wywołanie rysuje i panele, i HUD.
- Blokada klawiatury gry: przedostatnia linia `onRender`, `input().setKeyboardBlocked(m_debugUI.wantsKeyboard());` (sekcja 5.6).
- Blokada myszy gry: ostatnia linia `onRender`, `input().setMouseBlocked(m_debugUI.wantsMouse());` (sekcja 5.6).

### 5.5 Jak dodać nowy panel

Przykład: panel "Timing" pokazujący parametry stałego kroku. Kod poniżej jest wzorem do ćwiczenia, nie ma go w repozytorium.

**Krok 1. Nagłówek** `src/debug/panels/TimingPanel.hpp`. Komentarz na górze z odnośnikiem do dokumentu, deklaracje wyprzedzające zamiast `#include`, funkcja w przestrzeni nazw `debug`:

```cpp
// "Timing" debug panel: fixed step parameters.
// See docs/modules/debug-ui.md
#pragma once

namespace core {
class Time;
} // namespace core

namespace debug {

/// Draws the "Timing" panel. Called by DebugUI::draw, inside the ImGui frame.
void drawTimingPanel(const core::Time& time);

} // namespace debug
```

**Krok 2. Miejsce na pierwsze uruchomienie.** W [`PanelLayout.hpp`](../../src/debug/PanelLayout.hpp), pod dwunastoma istniejącymi stałymi, dopisz trzynastą. Tu zaczyna się kłopot, którego w M4 jeszcze nie było: w oknie 1280 x 720 **nie ma już wolnego prostokąta**. Kolumny i dolny rząd zajmuje sześć paneli, a cały pas nad dolnym rzędem należy do paneli Camera i Gameplay, które po rozwinięciu zajmują go od lewej kolumny do prawej (sekcja 5.7). Od M6 pod ich paskami stoi jeszcze drugi rząd pasków: zwinięte panele Terrain i Grass, od pierwszej części M7 trzeci: szeroki pasek panelu Framebuffers, a od czwartej czwarty: tak samo szeroki pasek panelu Shadows. Trzynasty panel musi więc na coś nachodzić. W ćwiczeniu stawiam go tuż nad panelem Shaders, w prawym dolnym rogu widocznej sceny:

```cpp
inline constexpr PanelPlacement TIMING_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {SHADERS_LEFT, BOTTOM_ROW_HEIGHT + 2.0F * PANEL_GAP},
    .size = {SHADERS_WIDTH, 120.0F},
};
```

Po podstawieniu: róg `BOTTOM_LEFT`, więc `offset` liczy się od lewej i od dolnej krawędzi okna. Lewa krawędź panelu to `SHADERS_LEFT` = 672, prawa 672 + 292 = 964. Dół panelu to `720 - (280 + 2 * 8) = 424`, góra `424 - 120 = 304`. Przy starcie prostokąt nie nachodzi na żaden panel: dolny rząd zaczyna się w y 432, prawa kolumna w x 972, a cztery rzędy pasków tytułów (Camera i Gameplay, pod nimi Terrain i Grass, pod nimi Framebuffers, pod nim Shadows) kończą się w y 120. Nachodzi natomiast na **rozwinięty** panel Gameplay (x od 640 do 964, y od 8 do 424), na rozwinięty panel Framebuffers (x od 352 do 964, y od 68 do 412), na rozwinięty panel Shadows (x od 352 do 964, y od 98 do 422) i zasłania część sceny. W ćwiczeniu to wystarcza. Prawdziwy panel, który ma startować rozwinięty, wymagałby przeliczenia układu, na przykład zwężenia panelu Gameplay, i poprawienia komentarza "No two rectangles overlap". Prawdziwy dwunasty panel, Shadows, obszedł to inaczej: startuje zwinięty, w kolejnym rzędzie pasków. Pole `collapsed` pomijam, więc ma wartość domyślną `false` i panel startuje rozwinięty. Panel, na który w oknie nie ma już miejsca, dostaje `.collapsed = true`, tak jak Camera i Gameplay. Tak doszły w M6 panele Terrain i Grass: oba mają `.collapsed = true` i dodatkowo `.foldedRowsBefore = 1`, czyli stają o jeden rząd pasków niżej, niż mówi ich `offset` (sekcja 5.7). Tą samą drogą doszedł w pierwszej części M7 jedenasty panel, Framebuffers: `.collapsed = true`, `.foldedRowsBefore = 2` i, razem z nim, `FOLDED_ROW_COUNT` podniesione z 2 do 3, żeby HUD zszedł pod trzeci rząd. W czwartej części M7 doszedł tak dwunasty panel, Shadows: `.collapsed = true`, `.foldedRowsBefore = 3` i `FOLDED_ROW_COUNT` podniesione z 3 do 4. Kolejny zwinięty panel w piątym rzędzie dostałby `.foldedRowsBefore = 4`, a `FOLDED_ROW_COUNT` trzeba by podnieść do 5.

**Krok 3. Implementacja** `src/debug/panels/TimingPanel.cpp`. Zawsze ten sam szkielet: miejsce na pierwsze uruchomienie, `if (ImGui::Begin(...)) { ... }` i `ImGui::End()` poza `if`:

```cpp
#include "debug/panels/TimingPanel.hpp"

#include "core/Time.hpp"
#include "debug/PanelLayout.hpp"

#include <imgui.h>

namespace debug {

void drawTimingPanel(const core::Time& time) {
    placePanelOnFirstUse(TIMING_PLACEMENT);
    if (ImGui::Begin("Timing")) {
        ImGui::Text("Fixed step: %.3f ms", 1000.0 * core::Time::FIXED_DT);
        ImGui::Text("Delta: %.3f ms", 1000.0 * time.deltaSeconds());
        ImGui::Text("Alpha: %.2f", time.alpha());
    }
    ImGui::End();
}

} // namespace debug
```

**Krok 4. CMake.** Dopisz oba pliki do listy `add_executable(night_maze ...)` w [`CMakeLists.txt`](../../CMakeLists.txt), obok `AssetsPanel`, `CameraPanel`, `CollisionPanel`, `GameplayPanel`, `LightsPanel`, `MazePanel`, `RendererPanel` i `ShadersPanel`, w kolejności alfabetycznej. Bez tego linker zgłosi brak symbolu `drawTimingPanel`.

**Krok 5. Wywołanie.** W [`DebugUI.cpp`](../../src/debug/DebugUI.cpp) dodaj `#include "debug/panels/TimingPanel.hpp"` i wywołanie wewnątrz `if (m_visible)`, po `DockSpaceOverViewport`, jako ostatnie w bloku:

```cpp
drawAssetsPanel(context.assets, context.viewMode, context.lighting.normalMapping,
                m_rawTextureSampler);
drawLightsPanel(context.lighting, context.round);
drawTimingPanel(context.time);
```

Wywołanie `drawHud` stoi niżej, już za klamrą zamykającą `if (m_visible)`. Panel dopisany tam, obok HUD, nie dałby się schować klawiszem `~`.

Panel dostaje tylko te pola kontekstu, których potrzebuje, nigdy całego `context`. Miejsce z kroku 2 dobieram tak, żeby nowy panel przy starcie nie przykrył żadnego z istniejących (tabela w sekcji 5.7).

**Krok 6. Dane.** Ten przykład korzysta z `context.time`, które jest już w `DebugContext`. Jeśli panel potrzebuje nowych danych, trzeba je doprowadzić tą samą drogą co kolor tła, w pięciu miejscach:

1. pole w klasie będącej właścicielem (dziś `game::NightMazeApp`, wzór: `m_clearColor`),
2. chroniony akcesor w tej klasie (wzór: `clearColor()`),
3. jedno pole w `DebugContext` w [`DebugContext.hpp`](../../src/debug/DebugContext.hpp), z komentarzem, czy jest tylko do odczytu, czy edytowalne (nowe pole dopisuję na końcu struktury),
4. jedna linia w `debug::DebugContext{...}` w `DebugNightMazeApp::onRender` w `main.cpp`, w tej samej kolejności co pola struktury,
5. przekazanie pola do panelu w `DebugUI::draw`, na przykład `drawTimingPanel(context.time, context.nowePole)`.

Sygnatura `DebugUI::draw` się nie zmienia. Kod w `game/` nadal nie dołącza niczego z `debug/`. Dane tylko do odczytu mają w strukturze i w sygnaturze panelu typ `const&`, edytowalne zwykłą referencję.

Prawdziwy przykład kroku 6 jest już w repozytorium: panel **Collision** i jego przełącznik rysowania kształtów kolizji. Te same pięć miejsc w jego wypadku:

| # | Miejsce | Kod |
|---|---|---|
| 1 | pole właściciela, [`NightMazeApp.hpp`](../../src/game/NightMazeApp.hpp) | `bool m_drawColliders = false;` |
| 2 | chroniony akcesor, tamże | `bool& drawColliders() { return m_drawColliders; }` |
| 3 | pole kontekstu, [`DebugContext.hpp`](../../src/debug/DebugContext.hpp) | `bool& drawColliders;`, dopisane wtedy na końcu struktury (dziś pole 13 z 18) |
| 4 | linia w [`main.cpp`](../../src/main.cpp) | `.drawColliders = drawColliders(),`, po linii `.viewMode = viewMode(),` |
| 5 | przekazanie do panelu, [`DebugUI.cpp`](../../src/debug/DebugUI.cpp) | `drawCollisionPanel(context.mazeWorld, context.round, context.player, context.drawColliders);` |

Panel zmienia jedną wartość `bool`, a skutek widać w następnej klatce: `NightMazeApp::onRender` sprawdza `m_drawColliders` i woła albo nie woła `drawColliderLines`. Panel nie rysuje pudełek ani sfer sam i nie woła żadnej funkcji `gl*`.

Drugi prawdziwy przykład to panel **Shaders**, którego przycisk nie zmienia liczby, tylko woła funkcję obiektu:

| # | Miejsce | Kod |
|---|---|---|
| 1 | pola właściciela, [`NightMazeApp.hpp`](../../src/game/NightMazeApp.hpp) | `gfx::Shader m_texturedShader;`, `gfx::Shader m_colorShader;`, `gfx::Shader m_litShader;`, `gfx::Shader m_gouraudShader;` (pola istnieją, bo gra nimi rysuje) |
| 2 | chronione akcesory, tamże | `gfx::Shader& texturedShader() { return m_texturedShader; }` i trzy analogiczne |
| 3 | pola kontekstu, [`DebugContext.hpp`](../../src/debug/DebugContext.hpp) | `gfx::Shader& texturedShader;`, `gfx::Shader& colorShader;`, `gfx::Shader& litShader;`, `gfx::Shader& gouraudShader;` i deklaracja wyprzedzająca `class Shader;` w przestrzeni nazw `gfx` |
| 4 | linie w [`main.cpp`](../../src/main.cpp) | `.texturedShader = texturedShader(),` i `.colorShader = colorShader(),`, a dalej, po ośmiu innych polach, `.litShader = litShader(),` i `.gouraudShader = gouraudShader(),` |
| 5 | przekazanie do panelu, [`DebugUI.cpp`](../../src/debug/DebugUI.cpp) | tablica czterech wskaźników i `drawShadersPanel(shaders);` (sekcja 5.2) |

Pola są edytowalne (bez `const`), bo przycisk panelu woła `reload()`, a ta funkcja wykonuje wywołania OpenGL. Sam panel nadal nie woła żadnej funkcji `gl*`.

**Najświeższy pełny przykład: panel Gameplay (M5).** To pierwszy panel, który przeszedł wszystkie siedem kroków po ustaleniu wspólnego układu, więc pokazuje je w prawdziwym kodzie:

| Krok | Co powstało |
|---|---|
| 1. Nagłówek | [`GameplayPanel.hpp`](../../src/debug/panels/GameplayPanel.hpp): deklaracje wyprzedzające `struct GameplaySettings;` i `struct Round;` w `game`, żadnego `#include`, jedna funkcja `void drawGameplayPanel(game::Round& round, game::GameplaySettings& settings);`. Komentarz na górze wskazuje dokument gry (`// See docs/modules/game/gameplay.md`), bo tam jest opis reguł |
| 2. Miejsce | `GAMEPLAY_PLACEMENT` w `PanelLayout.hpp`, z dwiema nowymi stałymi zależnymi: `GAMEPLAY_LEFT = BOTTOM_ROW_LEFT + CAMERA_WIDTH + PANEL_GAP` i `GAMEPLAY_WIDTH = REFERENCE_WIDTH - GAMEPLAY_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP`. Wysokość to `CAMERA_HEIGHT`, a `.collapsed = true`: panel startuje zwinięty obok panelu Camera (sekcja 5.7) |
| 3. Implementacja | [`GameplayPanel.cpp`](../../src/debug/panels/GameplayPanel.cpp): `placePanelOnFirstUse(GAMEPLAY_PLACEMENT);`, `if (ImGui::Begin("Gameplay")) { ... }`, `ImGui::End();` poza `if`. Zakresy suwaków to nazwane stałe w anonimowej przestrzeni nazw (od `MIN_BATTERY` do `MAX_PICKUP_RADIUS`), a treść jest podzielona na trzy funkcje pomocnicze: `drawRoundState`, `drawBattery` i `drawRules` |
| 4. CMake | dwie linie `src/debug/panels/GameplayPanel.cpp` i `.hpp` w `add_executable(night_maze ...)`, między `CollisionPanel` a `LightsPanel` |
| 5. Wywołanie | `#include "debug/panels/GameplayPanel.hpp"` i `drawGameplayPanel(context.round, context.gameplay);` w `DebugUI::draw`, po `drawCameraPanel` |
| 6. Dane | dwa nowe pola kontekstu, tabela niżej |
| 7. Dokumentacja | opis kontrolek w [`game/gameplay.md`](game/gameplay.md), sekcja 6, wiersz w tabelach sekcji 5.7 i sekcji 6 tego dokumentu |

Krok 6 dla tego panelu, w tych samych pięciu miejscach:

| # | Miejsce | Kod |
|---|---|---|
| 1 | pola właściciela, [`NightMazeApp.hpp`](../../src/game/NightMazeApp.hpp) | `GameplaySettings m_gameplay;` i `Round m_round;` (pola istnieją, bo gra według nich prowadzi rundę) |
| 2 | chronione akcesory, tamże | `GameplaySettings& gameplaySettings() { return m_gameplay; }` i `Round& round() { return m_round; }` |
| 3 | pola kontekstu, [`DebugContext.hpp`](../../src/debug/DebugContext.hpp) | `game::GameplaySettings& gameplay;` i `game::Round& round;` na końcu struktury, z deklaracjami wyprzedzającymi `struct GameplaySettings;` i `struct Round;` |
| 4 | linie w [`main.cpp`](../../src/main.cpp) | `.gameplay = gameplaySettings(),` i `.round = round(),` jako dwie ostatnie |
| 5 | przekazanie, [`DebugUI.cpp`](../../src/debug/DebugUI.cpp) | `drawGameplayPanel(context.round, context.gameplay);`, a te same dwa pola dostają też `drawHud`, `drawMazePanel`, `drawCollisionPanel` i `drawLightsPanel` (każda funkcja tylko to, czego potrzebuje) |

Trzy rzeczy, które ten panel pokazuje, a przykład "Timing" nie:

1. **Przycisk, który tylko prosi.** `if (ImGui::Button("Restart round (key R)")) { settings.restart = true; }` nie zaczyna rundy. Ustawia flagę w `GameplaySettings`, a gra czyta ją na początku następnej klatki (`NightMazeApp::onRender`: `if (m_gameplay.restart || input().wasKeyPressed(RESTART_KEY))`), zeruje i woła `beginRound()`. To ten sam wzór co flaga `regenerate` panelu Maze (pułapka 20): stan gry zmienia się między krokami symulacji, nigdy w środku klatki ImGui.
2. **Jedno pole edytowalne w strukturze, którą panel poza tym tylko czyta.** Suwak `Battery` pisze do `round.battery`, więc panel dostaje `game::Round&` bez `const`, a co za tym idzie bez `const` jest też pole `DebugContext::round`. To jedyny powód: komentarz przy polu mówi "Editable for one thing, the charge of the battery". Pozostali odbiorcy tego pola (Maze, Collision, Lights, HUD) biorą `const game::Round&`.
3. **Suwaki, które działają od następnego kroku.** Liczby reguł to zwykłe pola `GameplaySettings`. Gra czyta je w każdym stałym kroku, więc zmiana suwaka nie wymaga żadnej flagi ani nowej rundy (tak mówi komentarz w `GameplayPanel.hpp`, a które pole czyta która funkcja gry, opisuje [`game/gameplay.md`](game/gameplay.md), sekcja 5).

Trzeci wzór to dane, które przychodzą **razem ze swoim obiektem**. Panel Camera edytuje trzy prędkości gracza, ale w `DebugContext` nie ma pól `walkSpeed`, `sprintSpeed` ani `flySpeed`: jest jedno pole `player`, a panel sięga po `player.walkSpeed`. Osobne pole w kontekście dostaje tylko to, co nie należy do żadnego przekazywanego obiektu, na przykład `mouseSensitivity` (opisuje sterowanie, a nie kamerę ani gracza) albo `viewMode` i `drawColliders` (ustawienia rysowania aplikacji). Najdalej ten wzór idzie w panelu Lights: kilkanaście kontrolek edytuje pola jednej struktury `game::LightingSettings`, a kontekst ma dla nich jedno pole `lighting`. Włącznik latarki (`lighting.flashlightOn`) jest przy tym drugim, po `player.noclip`, przykładem wartości, którą zmienia i panel, i klawisz gry (F, [`game/flashlight.md`](game/flashlight.md)). Trzecim jest nowa runda: prosi o nią przycisk panelu Gameplay (flagą `restart`) i klawisz R.

**Krok 7. Sprawdzenie i dokumentacja.** Zbuduj, uruchom, zadokuj panel do krawędzi, uruchom ponownie i sprawdź, że układ się zachował. Skasuj `imgui.ini` z katalogu roboczego i sprawdź, że przy pierwszym uruchomieniu panel staje w swoim miejscu i niczego nie przykrywa. Dopisz panel do sekcji 6 dokumentu modułu, którego dotyczy (tam też wskazuje komentarz `// See docs/...` na górze obu plików panelu), do tabeli w sekcji 5.7 tego dokumentu oraz do kolumny "Przełącznik w ImGui" w [`../syllabus.md`](../syllabus.md).

Zasady, których się trzymam przy panelach: unikalny tytuł w `Begin` (dwa panele o tym samym tytule zlałyby się w jedno okno), żadnych zmiennych globalnych i `static` na stan, żadnych wywołań `gl*` w panelu.

### 5.6 Klawiatura i mysz: gra czy ImGui

Ten sam klawisz i to samo kliknięcie widzą dwaj odbiorcy. ImGui dostaje wejście przez callbacki backendu GLFW, a gra czyta je przez `core::Input`, czyli prosto z GLFW. Bez dodatkowego mechanizmu Escape wciśnięty po to, żeby anulować edycję pola w panelu, zamknąłby program, klawisz `~` wpisany w pole schowałby panele, a przeciąganie suwaka w panelu byłoby dla gry zwykłym ruchem myszy.

Moduł `debug` dokłada do rozwiązania dwie funkcje:

```cpp
bool DebugUI::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

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

`ImGui::GetIO()` zwraca strukturę `ImGuiIO`, przez którą ImGui wymienia dane z programem. Pole `WantCaptureKeyboard` jest ustawiane przez samą bibliotekę i znaczy: "używam teraz klawiatury, aplikacja powinna zignorować klawisze". ImGui ustawia je, gdy aktywny jest **dowolny widżet** (edycja pola, przeciąganie suwaka albo wartości, trzymany przycisk, przesuwane okno panelu) albo otwarte jest okno modalne, a nie tylko w polach tekstowych. Aktywny widżet może bowiem sam używać klawiszy: Escape anuluje edycję, Tab przechodzi do następnego pola, Ctrl, Shift i Alt zmieniają zachowanie przeciągania.

Pole `WantCaptureMouse` znaczy to samo dla myszy: "używam teraz myszy, aplikacja powinna zignorować kliknięcia i ruch". Warunek jest inny niż przy klawiaturze. ImGui ustawia je, gdy kursor jest **nad oknem ImGui** (panelem) albo gdy trwa przeciąganie rozpoczęte na widżecie, nawet jeśli kursor wyjechał już poza panel. Przezroczysty węzeł centralny obszaru dokowania (`PassthruCentralNode`, sekcja 3.2) nie liczy się jako okno pod kursorem, więc nad sceną pole jest fałszywe i mysz należy do gry.

`wantsMouse()` zwraca to pole z jednym wyjątkiem, opisanym niżej: przy myszy wyłączonej przez `setMouseEnabled(false)` odpowiedź zawsze brzmi "nie".

Obie funkcje są `const` i niczego nie zmieniają. `DebugUI` nie wie, co wołający zrobi z odpowiedzią, i nie dołącza niczego z `game/`. Wartości przekazuje dalej `main.cpp`:

```cpp
input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
input().setMouseBlocked(m_debugUI.wantsMouse());
```

Dopóki blokada klawiatury jest ustawiona, `core::Input::isKeyDown` i `wasKeyPressed` zwracają `false` dla każdego klawisza. Dopóki ustawiona jest blokada myszy, `isMouseButtonDown` i `wasMouseButtonPressed` zwracają `false` dla każdego przycisku, a `mouseDeltaX` i `mouseDeltaY` zwracają 0. Trzy rzeczy, które trzeba umieć wyjaśnić:

1. **Kierunek zależności zostaje nienaruszony.** `core/` nie dołącza ImGui: dostaje dwie neutralne flagi, "klawiatura zablokowana" i "mysz zablokowana", i nie wie, kto je ustawił. `debug/` nie zna gry. Oba końce skleja `main.cpp`.
2. **Wartości są odczytywane po `draw`.** ImGui aktualizuje `WantCaptureKeyboard` i `WantCaptureMouse` w `ImGui::NewFrame()`, a ten jest wołany wewnątrz `draw`. Odczyt przed `draw` dałby wartości o klatkę starsze.
3. **Blokada działa od następnej klatki.** Pytania o klawisze w bieżącej klatce (Escape w `Application::run`, `~` na początku `onRender`) padły przed tymi liniami. Dlaczego to opóźnienie nie szkodzi i dlaczego po zdjęciu blokady nie pojawia się fałszywe "właśnie wciśnięty", opisuje [`core/input.md`](core/input.md), sekcje 5.6 i 5.10.

Blokada dotyczy także samego przełącznika paneli: podczas edycji pola klawisz `~` trafia do pola, a nie do `toggleVisible()`. Tak samo klawisze gry R, F i N: gra pyta o nie przez `input().wasKeyPressed`, więc przy aktywnym widżecie ich nie widzi.

HUD z M5 niczego w tym mechanizmie nie zmienia: jego okna mają flagę `ImGuiWindowFlags_NoInputs`, więc kursor nad paskiem HUD nie ustawia `WantCaptureMouse`, a HUD nie ma widżetu, który mógłby stać się aktywny i ustawić `WantCaptureKeyboard` (sekcja 5.9).

Odbiorcą blokady myszy jest kamera: dzięki blokadzie kliknięcie w panel nie przechwytuje kursora, a przeciąganie suwaka nie jest dla gry ruchem myszy ([`scene/camera-controls.md`](scene/camera-controls.md), sekcja 5.3).

**W drugą stronę: `setMouseEnabled`, czyli ImGui bez myszy przy przechwyconym kursorze.** Po kliknięciu w scenę kamera przechwytuje kursor (`GLFW_CURSOR_DISABLED`). Kursor znika, ale nadal ma pozycję: wirtualną, zmieniającą się z każdym ruchem myszy. Backend GLFW biblioteki ImGui w tym trybie pomija tylko zmianę kształtu kursora, a pozycję i kliknięcia przekazuje do ImGui jak zwykle (historia zmian w `imgui_impl_glfw.cpp`, wpis z 2023-07-18: ignorowanie myszy przy `GLFW_CURSOR_DISABLED` zostało wycofane, a użytkownik "może ustawić `ImGuiConfigFlags_NoMouse`, jeśli chce"). Bez dodatkowego kodu niewidoczny kursor najeżdżałby na panele: ImGui zgłaszałoby `WantCaptureMouse`, blokada zerowałaby przesunięcie myszy i obrót kamery by zamierał, a kliknięcie trafiałoby w niewidoczny widżet.

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

| Linia | Znaczenie |
|---|---|
| `ImGuiIO& io = ImGui::GetIO();` | referencja do struktury wymiany danych z ImGui. Bez `const`, bo zmieniam w niej flagi |
| `io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;` | zdejmuje jeden bit. `~` odwraca maskę (wszystkie bity poza tym jednym), `&=` zostawia tylko bity obecne w obu, więc pozostałe flagi (na przykład `DockingEnable`) zostają nietknięte |
| `io.ConfigFlags \|= ImGuiConfigFlags_NoMouse;` | ustawia ten bit, tak samo jak `DockingEnable` w konstruktorze |

W `main.cpp`, przed `draw`:

```cpp
// While the cursor is captured the mouse belongs to the camera. The hidden cursor
// still has a position that moves with the mouse, so the panels must ignore it,
// otherwise it would hover and click them unseen.
m_debugUI.setMouseEnabled(!input().isCursorCaptured());
```

**Co dokładnie robi `ImGuiConfigFlags_NoMouse`** (sprawdzone w źródle ImGui 1.92, `imgui.cpp`, funkcja `UpdateHoveredWindowAndCaptureFlags`, wołana z `ImGui::NewFrame()`):

1. Flaga jest czytana w `NewFrame`. Gdy jest ustawiona, ImGui po wyszukaniu okna pod kursorem **kasuje wynik**: w tej klatce żaden panel nie jest "pod kursorem". Skoro żadne okno nie jest najechane, żaden widżet nie może zostać podświetlony ani kliknięty i nie pojawia się żadna podpowiedź.
2. Pozycja myszy **nie jest** kasowana (komentarz w źródle mówi to wprost) i backend nadal ją dostarcza. Flaga odcina skutki, nie dane.
3. `WantCaptureMouse` nie jest przez tę flagę zerowane bezwarunkowo. Samo najechanie go już nie ustawia, ale gdy przycisk myszy zostanie wciśnięty w chwili, gdy niewidoczny kursor jest nad panelem, ImGui uznaje to kliknięcie za swoje (własność kliknięcia ustala przed skasowaniem okna pod kursorem) i trzyma `WantCaptureMouse` tak długo, jak przycisk jest wciśnięty. Żaden widżet przy tym nie reaguje. Zmierzone: z flagą `NoMouse`, kursorem nad panelem i wciśniętym przyciskiem pole ma wartość `true`, a żadna wartość w panelu się nie zmienia.

Z punktu 3 wynika warunek w `wantsMouse()`: przy wyłączonej myszy funkcja zwraca `false` bez pytania ImGui. Inaczej każde kliknięcie podczas sterowania kamerą, które przypadkiem wypadłoby "nad" panelem, zamrażałoby obrót na czas trzymania przycisku.

**W której klatce to działa.** `setMouseEnabled` stoi przed `draw`, a flaga jest czytana w `NewFrame` wewnątrz `draw`, więc zmiana obowiązuje **w tej samej klatce**, bez opóźnienia. Cała kolejność zdarzeń:

| Klatka | `Application::run`, przed `onRender` | `NightMazeApp::onRender` | `main.cpp` przed `draw` | ImGui w `draw` | Blokada na następną klatkę |
|---|---|---|---|---|---|
| N: kliknięcie w scenę | `m_input.update()` widzi wciśnięty przycisk | kursor wolny, mysz niezablokowana (kursor był nad sceną): `setCursorCaptured(true)` | `setMouseEnabled(false)` | `NoMouse` już działa: nic nie jest najechane | `wantsMouse()` to `false`: mysz odblokowana |
| N+1 | przesunięcie myszy wyzerowane po zmianie trybu kursora | kursor przechwycony: `rotate(0, 0)` | `setMouseEnabled(false)` | bez myszy | odblokowana |
| N+2 i dalej | zwykłe przesunięcie | kamera się obraca | `setMouseEnabled(false)` | bez myszy | odblokowana |
| M: Escape | `wasKeyPressed(GLFW_KEY_ESCAPE)`: `setCursorCaptured(false)`, kursor wraca | kursor wolny: brak obrotu. Przechwyci ponownie tylko przy nowym kliknięciu | `setMouseEnabled(true)` | mysz działa od tej klatki | według `WantCaptureMouse` |

Nie ma klatki, w której mysz mają oba systemy naraz, ani takiej, w której nie ma jej żaden: w klatce N flaga zostaje ustawiona, zanim ImGui zdąży zobaczyć kliknięcie jako swoje, a w klatce M `Application::run` zwalnia kursor przed `onRender`, więc `main.cpp` włącza mysz ImGui w tej samej klatce, w której kursor wraca. Zostaje jedno znane opóźnienie, niezwiązane z przechwyceniem: blokada myszy dla gry spóźnia się o klatkę ([`core/input.md`](core/input.md), pułapka 11).

Dlaczego funkcja jest w `DebugUI`, a wywołanie w `main.cpp`: `core/` nie może znać ImGui, więc `Application::run` nie może przestawiać flagi samo przy zwalnianiu kursora. `debug/` nie może znać gry, więc `DebugUI` nie pyta, czy kamera coś przechwyciła: dostaje gotowe "włącz" albo "wyłącz". Regułę zna tylko `main.cpp`.

### 5.7 Układ domyślny: `PanelLayout`, `ImGuiCond_FirstUseEver` i `imgui.ini`

Miejsce, rozmiar i stan zwinięcia wszystkich dwunastu paneli przy pierwszym uruchomieniu są zapisane w **jednym** pliku, [`PanelLayout.hpp`](../../src/debug/PanelLayout.hpp). Każdy panel woła przed `Begin` jedną funkcję z jedną stałą, na przykład `placePanelOnFirstUse(RENDERER_PLACEMENT);` (kod panelu Renderer w sekcji 5.3). Wcześniej (M2 + M3) każdy plik panelu miał własną parę stałych z pozycją w pikselach liczoną od lewego górnego rogu. Miało to dwie wady: prostokątów nie dało się porównać bez otwierania sześciu plików, a w oknie większym niż 1280 x 720 prawa kolumna zostawała w środku okna.

**Jak opisane jest miejsce panelu.** Panel jest przyczepiony do jednego z czterech rogów okna:

```cpp
/// Where one panel appears and how big it is the first time the program runs.
///
/// A panel sticks to one corner of the window. All numbers are in pixels of a
/// REFERENCE_WIDTH x REFERENCE_HEIGHT window at 100 % display scaling.
struct PanelPlacement {
    /// The corner of the window the panel sticks to: one of the four constants below.
    ImVec2 corner;
    /// Distance from that corner of the window to the same corner of the panel.
    ImVec2 offset;
    /// Width and height of the panel.
    ImVec2 size;
    /// True: the panel starts folded to its title bar and opens with a click on the arrow
    /// in that bar. For a panel the window has no free room for.
    bool collapsed = false;
    /// How many rows of folded title bars stand between the offset and this panel. The
    /// panel is moved away from its corner by that many bars, each with the gap after
    /// it. The height of a bar depends on the size of the font, so it is not part of
    /// the offset: it is asked from ImGui when the panel is placed.
    int foldedRowsBefore = 0;
};
```

```cpp
// The corners of the window: x is 0 at the left edge and 1 at the right edge, y is 0 at
// the top edge and 1 at the bottom edge. All four are named, although the present layout
// puts no panel in the bottom right corner.
inline constexpr ImVec2 TOP_LEFT{0.0F, 0.0F};
inline constexpr ImVec2 TOP_RIGHT{1.0F, 0.0F};
inline constexpr ImVec2 BOTTOM_LEFT{0.0F, 1.0F};
inline constexpr ImVec2 BOTTOM_RIGHT{1.0F, 1.0F};
```

| Pole | Znaczenie |
|---|---|
| `corner` | róg okna, do którego panel jest przyczepiony. Zapisany jako para liczb 0 albo 1: `x = 0` to lewa krawędź, `x = 1` prawa, `y = 0` górna, `y = 1` dolna. Nazwane są wszystkie cztery rogi, choć dzisiejszy układ nie stawia żadnego panelu w prawym dolnym (`BOTTOM_RIGHT`) |
| `offset` | odległość od tego rogu okna do **tego samego** rogu panelu. Dla `TOP_RIGHT` i `{8, 8}` prawy górny róg panelu stoi 8 jednostek od prawej i 8 od górnej krawędzi okna |
| `size` | szerokość i wysokość panelu. Dla panelu zwiniętego to rozmiar, do którego się rozwinie |
| `collapsed` | `true`: panel startuje zwinięty (collapsed) do samego paska tytułu i rozwija się po kliknięciu strzałki w tym pasku. Pole ma inicjalizator `= false`, więc stałe sześciu paneli go nie wymieniają, a wymieniają je tylko `CAMERA_PLACEMENT`, `GAMEPLAY_PLACEMENT`, `TERRAIN_PLACEMENT`, `GRASS_PLACEMENT`, od pierwszej części M7 `FRAMEBUFFERS_PLACEMENT` i, od czwartej, `SHADOWS_PLACEMENT`. Inicjalizator domyślny pola nie odbiera strukturze statusu agregatu (od C++14), dlatego inicjalizatory desygnowane działają dalej |
| `foldedRowsBefore` | od M6: ile rzędów zwiniętych pasków tytułów stoi między `offset` a panelem. Panel jest odsuwany od swojego rogu o tyle pasków, każdy z odstępem `PANEL_GAP` za nim. Domyślnie 0, więc osiem starszych stałych go nie wymienia. `TERRAIN_PLACEMENT` i `GRASS_PLACEMENT` mają 1: stoją w drugim rzędzie, pod paskami paneli Camera i Gameplay. `FRAMEBUFFERS_PLACEMENT` ma 2: trzeci rząd. `SHADOWS_PLACEMENT` (czwarta część M7) ma 3: czwarty rząd. Dlaczego to osobne pole, a nie większy `offset.y`: wysokość paska tytułu zależy od rozmiaru czcionki (`ImGui::GetFrameHeight()`), a nie od skali układu, więc nie da się jej wpisać jako stałej w jednostkach okna odniesienia. Funkcja `placePanelOnFirstUse` pyta o nią ImGui w chwili stawiania panelu (niżej, `foldedRowsHeight`) |

Liczby są podane dla okna odniesienia 1280 x 720 przy skali ekranu 100%. To rozmiar startowy okna gry (`INITIAL_WIDTH`, `INITIAL_HEIGHT` w `NightMazeApp.cpp`).

**Wymiary układu.** Wszystkie liczby są nazwanymi stałymi, a wymiary zależne są z nich liczone:

```cpp
// The layout is drawn up for the window the game starts with (INITIAL_WIDTH and
// INITIAL_HEIGHT in game/NightMazeApp.cpp).
inline constexpr float REFERENCE_WIDTH = 1280.0F;
inline constexpr float REFERENCE_HEIGHT = 720.0F;

// Free space between a panel and the edge of the window, and between two panels.
inline constexpr float PANEL_GAP = 8.0F;

// Left column: Renderer above Lights. Together they fill the height of the window. The
// Renderer panel is exactly as tall as its contents. The Lights panel gets what is left,
// which is a little less than its contents (with its Moon group folded), so it scrolls.
inline constexpr float LEFT_COLUMN_WIDTH = 336.0F;
inline constexpr float RENDERER_HEIGHT = 284.0F;
inline constexpr float LIGHTS_HEIGHT = REFERENCE_HEIGHT - RENDERER_HEIGHT - 3.0F * PANEL_GAP;

// Right column: Maze above Assets. Together they fill the height of the window. The
// lists of the Assets panel are long, so that panel always scrolls: it gets what is left.
inline constexpr float RIGHT_COLUMN_WIDTH = 300.0F;
inline constexpr float MAZE_HEIGHT = 480.0F;
inline constexpr float ASSETS_HEIGHT = REFERENCE_HEIGHT - MAZE_HEIGHT - 3.0F * PANEL_GAP;

// Bottom edge between the two columns: Collision and Shaders side by side, equally tall.
// Together they are as wide as the space between the columns. The scene stays visible
// above them, with the middle of the window, where the flashlight shines, well clear.
inline constexpr float BOTTOM_ROW_LEFT = LEFT_COLUMN_WIDTH + 2.0F * PANEL_GAP;
inline constexpr float BOTTOM_ROW_HEIGHT = 280.0F;
inline constexpr float COLLISION_WIDTH = 312.0F;
inline constexpr float SHADERS_LEFT = BOTTOM_ROW_LEFT + COLLISION_WIDTH + PANEL_GAP;
inline constexpr float SHADERS_WIDTH =
    REFERENCE_WIDTH - SHADERS_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP;

// Camera and Gameplay: the seventh and the eighth panel. Two columns and a bottom row
// have room for six, so these two start folded to their title bars, side by side at the
// top edge between the columns. Unfolded each reaches down to the bottom row and covers
// its part of the scene and the folded bars under it (Terrain or Grass, and its part of
// the Framebuffers and Shadows bars, see below), but no open panel. Camera is a little shorter than
// its contents, so it scrolls. Gameplay may start folded because the HUD shows the state of
// the round all the time: the panel is for changing the rules.
inline constexpr float CAMERA_WIDTH = 280.0F;
inline constexpr float CAMERA_HEIGHT = REFERENCE_HEIGHT - BOTTOM_ROW_HEIGHT - 3.0F * PANEL_GAP;
inline constexpr float GAMEPLAY_LEFT = BOTTOM_ROW_LEFT + CAMERA_WIDTH + PANEL_GAP;
inline constexpr float GAMEPLAY_WIDTH =
    REFERENCE_WIDTH - GAMEPLAY_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP;

// Terrain and Grass: the ninth and the tenth panel. They start folded too, in a second
// row of title bars right under Camera and Gameplay, each as wide as the bar above it
// (PanelPlacement::foldedRowsBefore is 1). Both panels are short: unfolded they cover
// a strip of the scene below their bar and their part of the Framebuffers bar in the
// third row, but no open panel. An unfolded Camera or Gameplay panel does cover the
// bars under it.
inline constexpr float TERRAIN_HEIGHT = 170.0F;
inline constexpr float GRASS_HEIGHT = 190.0F;

// Framebuffers: the eleventh panel. It starts folded in a third row of title bars, one
// bar as wide as the two bars above it together (PanelPlacement::foldedRowsBefore is
// 2). It is that wide because it shows four pictures side by side, under widgets that
// stand in two columns. Unfolded it reaches down to just above the bottom row and
// covers the scene between the columns and the folded Shadows bar under its own, but
// no open panel. Its contents need a little less than that height at that width.
inline constexpr float FRAMEBUFFERS_WIDTH = CAMERA_WIDTH + PANEL_GAP + GAMEPLAY_WIDTH;
inline constexpr float FRAMEBUFFERS_HEIGHT = 344.0F;

// Shadows: the twelfth panel. It starts folded in a fourth row of title bars, as wide
// as the Framebuffers bar above it (PanelPlacement::foldedRowsBefore is 3): its widgets
// stand in the left half and the picture of the shadow map in the right half. Unfolded
// it reaches down to just above the bottom row and covers the scene between the
// columns, but no other panel.
inline constexpr float SHADOWS_HEIGHT = 324.0F;

// The number of rows of folded title bars at the top edge. The HUD starts below them
// (Hud.cpp).
inline constexpr int FOLDED_ROW_COUNT = 4;
```

Jak te stałe zmieniały się między kamieniami milowymi i dlaczego. Tabela kończy się na M5. Pierwsza część M6 zmieniła dwie stałe: `RENDERER_HEIGHT` z `230.0F` na `284.0F`, bo panel Renderer dostał dwa wiersze (pole `Skybox` i suwak `Sky brightness`), i przez nią `LIGHTS_HEIGHT`, które jest liczone jako reszta kolumny: `720 - 284 - 3 * 8 = 412` zamiast 466. Panel Lights jest przez to o 54 jednostki niższy od swojej zawartości i się przewija, co komentarz w kodzie mówi wprost. Druga część M6 (teren i trawa) nie zmieniła żadnej z istniejących stałych: dopisała trzy nowe, `TERRAIN_HEIGHT = 170.0F`, `GRASS_HEIGHT = 190.0F` i `FOLDED_ROW_COUNT = 2` (ostatni blok kodu wyżej). Szerokości nowych paneli nie mają własnych stałych, bo każdy jest tak szeroki jak pasek nad nim: Terrain bierze `CAMERA_WIDTH`, a Grass `GAMEPLAY_WIDTH`. Pierwsza część M7 (panel Framebuffers) też nie zmieniła żadnej starszej stałej poza jedną: `FOLDED_ROW_COUNT` wzrosło z 2 do 3. Dopisała dwie nowe: `FRAMEBUFFERS_WIDTH`, liczone jako `280 + 8 + 324 = 612` (szerokość obu pasków nad nim razem z odstępem, bo panel pokazywał wtedy dwa obrazy obok siebie, a od drugiej części M7 pokazuje cztery), i `FRAMEBUFFERS_HEIGHT = 344.0F` (komentarz mówił wtedy: tyle, ile potrzebuje zawartość przy tej szerokości, a dziś, w bloku wyżej: trochę mniej niż ta wysokość. Nikt tego nie zmierzył na ekranie). Czwarta część M7 (panel Shadows) zmieniła znów jedną starszą stałą, `FOLDED_ROW_COUNT`, z 3 na 4, i komentarz nad `CAMERA_WIDTH`, który wymienia odtąd także pasek Shadows. Dopisała jedną nową stałą, `SHADOWS_HEIGHT = 324.0F`. Szerokość nowego panelu nie ma własnej stałej: `SHADOWS_PLACEMENT` bierze `FRAMEBUFFERS_WIDTH`, bo pasek Shadows jest tak szeroki jak pasek Framebuffers nad nim. Wysokość 324 daje dół rozwiniętego panelu 10 jednostek nad dolnym rzędem (rachunek niżej). Czy mieści zawartość, nikt nie zmierzył. Pozostałe stałe mają dziś wartości z kolumny M5:

| Stała | M2 + M3 | M4 | M5 | Powód |
|---|---|---|---|---|
| `RENDERER_HEIGHT` | `240.0F` | `230.0F` | bez zmian | w M4 panel dostał jeden wiersz więcej (lista `Lighting`), a zapas na drugą linię nazwy karty graficznej zniknął: każda jednostka wysokości oddana panelowi Renderer jest odebrana panelowi Lights pod nim |
| `LIGHTS_HEIGHT` | nie było | `720 - 230 - 3 * 8 = 466` | bez zmian | reszta wysokości lewej kolumny. W tym miejscu stał wcześniej panel Camera |
| `BOTTOM_ROW_HEIGHT` | `COLLISION_HEIGHT = 272.0F` i osobne `SHADERS_HEIGHT = 336.0F` | jedna stała `272.0F` dla obu paneli | `280.0F` | w M4 panel Shaders zaczął pokazywać jedną linię na program zamiast czterech, więc zmieścił się w wysokości panelu Collision. W M5 rząd urósł o 8 jednostek. Kod nie mówi dlaczego. W tym samym kamieniu milowym zmieniła się treść panelu Collision (dłuższa legenda kolorów, inne linie z liczbami), a na zrzucie ekranu wykonawcy panel o wysokości 280 pokazuje całą treść bez paska przewijania |
| `CAMERA_WIDTH` | panel miał szerokość kolumny, `336.0F` | `280.0F` | bez zmian | od M4 panel stoi poza kolumną, nad sceną. Węższy zasłania jej mniej |
| `CAMERA_HEIGHT` | `720 - 240 - 3 * 8 = 456` | `720 - 272 - 3 * 8 = 424` | `720 - 280 - 3 * 8 = 416` | wysokość rozwiniętego panelu: od górnej krawędzi okna do dolnego rzędu, z odstępami. Liczona z `BOTTOM_ROW_HEIGHT`, więc po zmianie wysokości dolnego rzędu w M5 skróciła się sama i panel Camera nadal na niego nie nachodzi |
| `GAMEPLAY_LEFT` | nie było | nie było | `352 + 280 + 8 = 640` | lewa krawędź nowego panelu: prawa krawędź panelu Camera plus odstęp |
| `GAMEPLAY_WIDTH` | nie było | nie było | `1280 - 640 - 300 - 2 * 8 = 324` | reszta szerokości między kolumnami. Liczona tak samo jak `SHADERS_WIDTH`, więc prawa krawędź panelu Gameplay wypada tam, gdzie prawa krawędź panelu Shaders |

**Dwanaście stałych z miejscami paneli:**

```cpp
// The twelve panels. No two rectangles overlap in a window of the reference size, with
// the six folded panels counted as their title bars.
inline constexpr PanelPlacement RENDERER_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, RENDERER_HEIGHT},
};
inline constexpr PanelPlacement LIGHTS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, RENDERER_HEIGHT + 2.0F * PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, LIGHTS_HEIGHT},
};
inline constexpr PanelPlacement CAMERA_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {CAMERA_WIDTH, CAMERA_HEIGHT},
    .collapsed = true,
};
inline constexpr PanelPlacement GAMEPLAY_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {GAMEPLAY_LEFT, PANEL_GAP},
    .size = {GAMEPLAY_WIDTH, CAMERA_HEIGHT},
    .collapsed = true,
};
inline constexpr PanelPlacement TERRAIN_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {CAMERA_WIDTH, TERRAIN_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 1,
};
inline constexpr PanelPlacement GRASS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {GAMEPLAY_LEFT, PANEL_GAP},
    .size = {GAMEPLAY_WIDTH, GRASS_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 1,
};
inline constexpr PanelPlacement FRAMEBUFFERS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {FRAMEBUFFERS_WIDTH, FRAMEBUFFERS_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 2,
};
inline constexpr PanelPlacement SHADOWS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {FRAMEBUFFERS_WIDTH, SHADOWS_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 3,
};
inline constexpr PanelPlacement MAZE_PLACEMENT{
    .corner = TOP_RIGHT,
    .offset = {PANEL_GAP, PANEL_GAP},
    .size = {RIGHT_COLUMN_WIDTH, MAZE_HEIGHT},
};
inline constexpr PanelPlacement ASSETS_PLACEMENT{
    .corner = TOP_RIGHT,
    .offset = {PANEL_GAP, MAZE_HEIGHT + 2.0F * PANEL_GAP},
    .size = {RIGHT_COLUMN_WIDTH, ASSETS_HEIGHT},
};
inline constexpr PanelPlacement COLLISION_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {COLLISION_WIDTH, BOTTOM_ROW_HEIGHT},
};
inline constexpr PanelPlacement SHADERS_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {SHADERS_LEFT, PANEL_GAP},
    .size = {SHADERS_WIDTH, BOTTOM_ROW_HEIGHT},
};
```

Zapis `.corner = TOP_LEFT` to inicjalizator desygnowany, ten sam mechanizm C++20 co przy `DebugContext` (sekcja 5.2). `inline constexpr` w nagłówku znaczy: stała znana w czasie kompilacji, której definicja może stać w nagłówku dołączanym przez kilka plików `.cpp` bez błędu linkera o wielokrotnej definicji.

Po podstawieniu liczb, dla okna 1280 x 720 (jednostką są współrzędne okna ImGui, te same co pozycja myszy, z początkiem w lewym górnym rogu i osią y w dół):

| Panel | Stała | Róg | `offset` (x, y) | `size` (szer., wys.) | Zajmuje x od do | Zajmuje y od do | Start |
|---|---|---|---|---|---|---|---|
| Renderer | `RENDERER_PLACEMENT` | `TOP_LEFT` | 8, 8 | 336, 284 | 8 do 344 | 8 do 292 | rozwinięty |
| Lights | `LIGHTS_PLACEMENT` | `TOP_LEFT` | 8, 300 | 336, 412 | 8 do 344 | 300 do 712 | rozwinięty, z paskiem przewijania |
| Camera | `CAMERA_PLACEMENT` | `TOP_LEFT` | 352, 8 | 280, 416 | 352 do 632 | 8 do 424 (po rozwinięciu) | **zwinięty**: sam pasek tytułu |
| Gameplay | `GAMEPLAY_PLACEMENT` | `TOP_LEFT` | 640, 8 | 324, 416 | 640 do 964 | 8 do 424 (po rozwinięciu) | **zwinięty**: sam pasek tytułu |
| Terrain | `TERRAIN_PLACEMENT` | `TOP_LEFT` | 352, 8 plus jeden rząd pasków (30), czyli 38 | 280, 170 | 352 do 632 | 38 do 208 (po rozwinięciu) | **zwinięty**: sam pasek tytułu, y od 38 do 60 |
| Grass | `GRASS_PLACEMENT` | `TOP_LEFT` | 640, 8 plus jeden rząd pasków (30), czyli 38 | 324, 190 | 640 do 964 | 38 do 228 (po rozwinięciu) | **zwinięty**: sam pasek tytułu, y od 38 do 60 |
| Framebuffers | `FRAMEBUFFERS_PLACEMENT` | `TOP_LEFT` | 352, 8 plus dwa rzędy pasków (60), czyli 68 | 612, 344 | 352 do 964 | 68 do 412 (po rozwinięciu) | **zwinięty**: sam pasek tytułu, y od 68 do 90 |
| Shadows | `SHADOWS_PLACEMENT` | `TOP_LEFT` | 352, 8 plus trzy rzędy pasków (90), czyli 98 | 612, 324 | 352 do 964 | 98 do 422 (po rozwinięciu) | **zwinięty**: sam pasek tytułu, y od 98 do 120 |
| Maze | `MAZE_PLACEMENT` | `TOP_RIGHT` | 8, 8 | 300, 480 | 972 do 1272 | 8 do 488 | rozwinięty |
| Assets | `ASSETS_PLACEMENT` | `TOP_RIGHT` | 8, 496 | 300, 216 | 972 do 1272 | 496 do 712 | rozwinięty |
| Collision | `COLLISION_PLACEMENT` | `BOTTOM_LEFT` | 352, 8 | 312, 280 | 352 do 664 | 432 do 712 | rozwinięty |
| Shaders | `SHADERS_PLACEMENT` | `BOTTOM_LEFT` | 672, 8 | 292, 280 | 672 do 964 | 432 do 712 | rozwinięty |

Skąd te liczby (wszystko liczone ze stałych, nic z pomiaru. Rachunek powtórzyłem krótkim skryptem w Pythonie, który podstawia te same wzory):

| Wielkość | Rachunek | Wynik |
|---|---|---|
| góra panelu Lights | `RENDERER_HEIGHT + 2 * PANEL_GAP` = 284 + 16 | 300 (dół panelu Renderer to 8 + 284 = 292, więc odstęp wynosi 8) |
| wysokość panelu Lights | `LIGHTS_HEIGHT` = 720 - 284 - 3 * 8 | 412, dół w 300 + 412 = 712 |
| lewa krawędź paneli Camera i Collision | `BOTTOM_ROW_LEFT` = 336 + 2 * 8 | 352 (prawa krawędź lewej kolumny to 8 + 336 = 344, odstęp 8) |
| wysokość paneli Camera i Gameplay | `CAMERA_HEIGHT` = 720 - 280 - 3 * 8 | 416, dół w 8 + 416 = 424 |
| prawa krawędź panelu Camera | 352 + `CAMERA_WIDTH` = 352 + 280 | 632 |
| lewa krawędź panelu Gameplay | `GAMEPLAY_LEFT` = 352 + 280 + 8 | 640 (odstęp od panelu Camera 8) |
| szerokość panelu Gameplay | `GAMEPLAY_WIDTH` = 1280 - 640 - 300 - 2 * 8 | 324, prawa krawędź w 640 + 324 = 964 (odstęp od prawej kolumny 8) |
| góra paneli Terrain i Grass | `offset.y` 8 plus `foldedRowsHeight(1, 1)` = 1 * (22 + 8). Liczba 22 to wysokość paska przy czcionce 16 i `FramePadding.y` 3, nie stała z `PanelLayout.hpp` | 38 (paski pierwszego rzędu kończą się w 8 + 22 = 30, odstęp 8). Pasek drugiego rzędu kończy się w 38 + 22 = 60 |
| dół rozwiniętych paneli Terrain i Grass | 38 + `TERRAIN_HEIGHT` 170 i 38 + `GRASS_HEIGHT` 190 | 208 i 228: oba kończą się daleko nad dolnym rzędem (432) |
| szerokość panelu Framebuffers | `FRAMEBUFFERS_WIDTH` = `CAMERA_WIDTH + PANEL_GAP + GAMEPLAY_WIDTH` = 280 + 8 + 324 | 612, prawa krawędź w 352 + 612 = 964, równo z panelem Gameplay nad nim |
| góra i dół panelu Framebuffers | `offset.y` 8 plus `foldedRowsHeight(2, 1)` = 2 * (22 + 8), potem plus `FRAMEBUFFERS_HEIGHT` 344 | góra 68 (pasek trzeciego rzędu od 68 do 90), dół rozwiniętego w 412: 20 jednostek nad dolnym rzędem (432) |
| góra i dół panelu Shadows | `offset.y` 8 plus `foldedRowsHeight(3, 1)` = 3 * (22 + 8), potem plus `SHADOWS_HEIGHT` 324 | góra 98 (pasek czwartego rzędu od 98 do 120), dół rozwiniętego w 422: 10 jednostek nad dolnym rzędem (432). Szerokość i lewa krawędź jak dla panelu Framebuffers: 612, od 352 do 964 |
| góra dolnego rzędu | róg `BOTTOM_LEFT`: dół okna 720 minus `PANEL_GAP` 8 daje dół panelu 712, minus `BOTTOM_ROW_HEIGHT` 280 | 432 (dół rozwiniętych paneli Camera i Gameplay to 424, odstęp 8) |
| prawa krawędź panelu Collision | 352 + `COLLISION_WIDTH` = 352 + 312 | 664 |
| lewa krawędź panelu Shaders | `SHADERS_LEFT` = 352 + 312 + 8 | 672 |
| szerokość panelu Shaders | `SHADERS_WIDTH` = 1280 - 672 - 300 - 2 * 8 | 292, prawa krawędź w 672 + 292 = 964 |
| lewa krawędź prawej kolumny | róg `TOP_RIGHT`: prawa krawędź okna 1280 minus 8 daje 1272, minus `RIGHT_COLUMN_WIDTH` 300 | 972 (odstęp od panelu Shaders 8) |
| góra panelu Assets | `MAZE_HEIGHT + 2 * PANEL_GAP` = 480 + 16 | 496, wysokość `ASSETS_HEIGHT` = 720 - 480 - 24 = 216, dół w 712 |

**Dlaczego żadne dwa prostokąty się nie nakładają.** Dwa prostokąty nachodzą na siebie tylko wtedy, gdy ich przedziały pokrywają się jednocześnie na osi x i na osi y (to ten sam test co dla pudełek kolizji, [`scene/collision.md`](scene/collision.md), sekcja 2). Okno dzieli się na trzy pionowe pasy, które na osi x się nie stykają: lewa kolumna (x od 8 do 344), środek (od 352 do 964) i prawa kolumna (od 972 do 1272). Wystarczy więc sprawdzić pary wewnątrz każdego pasa:

| Pas | Para | Dlaczego się nie nakłada |
|---|---|---|
| lewa kolumna | Renderer i Lights | na osi y: 8 do 292 i 300 do 712 |
| prawa kolumna | Maze i Assets | na osi y: 8 do 488 i 496 do 712 |
| środek | Collision i Shaders | na osi x: 352 do 664 i 672 do 964 |
| środek | Camera i Gameplay (oba rozwinięte) | na osi x: 352 do 632 i 640 do 964 |
| środek | Camera albo Gameplay (rozwinięty) i dolny rząd | na osi y: 8 do 424 i 432 do 712 |
| środek | Terrain i Grass (oba rozwinięte) | na osi x: 352 do 632 i 640 do 964 |
| środek | Terrain albo Grass (rozwinięty) i dolny rząd | na osi y: 38 do 208 albo 228 i 432 do 712 |
| środek | pasek Camera albo Gameplay (zwinięty) i panel pod nim | na osi y: 8 do 30 i od 38 w dół |
| środek | Framebuffers (rozwinięty) i dolny rząd | na osi y: 68 do 412 i 432 do 712 |
| środek | Framebuffers (rozwinięty) i zwinięte paski dwóch rzędów nad nim | na osi y: 68 do 412, a paski kończą się w 30 i 60 |
| środek | Shadows (rozwinięty) i dolny rząd | na osi y: 98 do 422 i 432 do 712 |
| środek | Shadows (rozwinięty) i zwinięte paski trzech rzędów nad nim | na osi y: 98 do 422, a paski kończą się w 30, 60 i 90 |
| środek | pasek Framebuffers (zwinięty) i pasek Shadows (zwinięty) | na osi y: 68 do 90 i 98 do 120 |

Od M6 w tym rachunku jest jeden wyjątek, i komentarz nad stałymi mówi o nim wprost, dziś słowami: "No two rectangles overlap in a window of the reference size, with the six folded panels counted as their title bars". Sześć paneli startujących jako zwinięte (od pierwszej części M7 także Framebuffers, od czwartej Shadows) liczy się więc jako paski. **Rozwinięty** panel Camera (y od 8 do 424) zakrywa pasek panelu Terrain pod sobą (y od 38 do 60), a rozwinięty Gameplay zakrywa pasek panelu Grass: to ta sama kolumna na osi x i wspólny przedział na osi y. Od pierwszej części M7 każdy z czterech starszych zwiniętych paneli po rozwinięciu zakrywa też swoją część szerokiego paska Framebuffers (y od 68 do 90), a rozwinięty panel Framebuffers (y od 68 do 412) leży w tym samym prostokącie co rozwinięte Camera, Gameplay, Terrain i Grass: rozwinięte jednocześnie nakładają się, a na wierzchu jest ten kliknięty ostatnio. Od czwartej części M7 dochodzi pasek Shadows (y od 98 do 120): rozwinięty panel Framebuffers zakrywa go w całości, bo oba mają tę samą szerokość, a każdy z czterech starszych zwiniętych paneli po rozwinięciu zakrywa swoją jego część. Rozwinięty panel Shadows (y od 98 do 422) nie zakrywa żadnego paska, bo trzy rzędy nad nim kończą się w y 90, ale leży w tym samym prostokącie co pozostałe rozwinięte panele środka. Dwa komentarze w `PanelLayout.hpp` pochodzą sprzed panelu Shadows i dla jego paska nie są już ścisłe: ten przy stałych Framebuffers mówił do tej części, że rozwinięty panel nie zakrywa żadnego innego panelu ("but no other panel"), a dziś mówi, że zakrywa zwinięty pasek Shadows pod swoim, ale żadnego otwartego panelu. Ten przy Terrain i Grass nadal wymienia tylko pasek Framebuffers. Żeby dostać się do paska pod spodem, trzeba zwinąć panel nad nim albo go odsunąć. Do M5, przy jednym rzędzie pasków, komentarz mówił prawdę także po kliknięciu obu strzałek. Pozostałe pary nie nachodzą na siebie w żadnym stanie. Dotyczy on paneli: dwa okna HUD nie mają stałej w `PanelLayout.hpp` i z panelami mogą się nakładać (sekcja 5.9).

Między panelami i przy krawędziach okna jest 8 jednostek odstępu (`PANEL_GAP`). Scena jest widoczna w środku górnej części okna, od x 352 do 964 i od y 8 do 424 (612 na 416 jednostek), z wyjątkiem dwóch pasków tytułów przy górnej krawędzi tego prostokąta: panelu Camera (280 jednostek szerokości) i panelu Gameplay (324), z odstępem 8 między nimi. Razem zajmują całą jego szerokość. Pasek ma wysokość równą wysokości czcionki plus dwa razy `FramePadding.y`, czyli 16 + 2 * 3 = 22 jednostki (wzór z `imgui.cpp`: `TitleBarHeight = g.FontSize + g.Style.FramePadding.y * 2.0f`), więc kończy się w y 8 + 22 = 30. Od M6 pod nimi, po odstępie 8, stoi drugi rząd takich samych pasków: Terrain pod Camera i Grass pod Gameplay, od y 38 do 60. Od pierwszej części M7 jest trzeci rząd: jeden pasek panelu Framebuffers na całą szerokość, od y 68 do 90. Od czwartej części M7 jest czwarty: taki sam pasek panelu Shadows, od y 98 do 120. Pod czterema rzędami, od y 136 (w M6 od y 76, w pierwszych trzech częściach M7 od y 106), wisi na środku pasek HUD (sekcja 5.9.5). Po rozwinięciu panelu Camera wolna zostaje prawa część prostokąta sceny (x od 640 do 964), po rozwinięciu panelu Gameplay lewa (x od 352 do 632), a po rozwinięciu obu między kolumnami nie widać sceny wcale, poza odstępami o szerokości 8. To cena dwunastu paneli w oknie 1280 x 720 i powód, dla którego sześć z nich startuje zwiniętych. Panele Terrain i Grass są niskie: rozwinięte zakrywają pas sceny pod swoim paskiem (do y 208 i 228), a nie cały prostokąt.

**Co mieści swoją zawartość.** Tabela z M2 + M3 miała w tym miejscu kolumnę ze zmierzonymi wysokościami zawartości (Renderer 202, Camera 452, Maze 478, Assets 714, Collision 271, Shaders 334, pomiar z 2026-10-05). Od tamtej pory treść albo rozmiar zmieniły się w każdym panelu, a nowego pomiaru wysokości nie mam. Podaję więc to, co mówią komentarze w `PanelLayout.hpp`, to, co da się policzyć, i to, co widać na zrzutach ekranu wykonawcy z M5 (Windows, 2026-10-05, stany ustawione tymczasowymi wstawkami, których już nie ma):

| Panel | Co mówi kod | Uwaga |
|---|---|---|
| Renderer | "The Renderer panel is exactly as tall as its contents" | zgadza się z rachunkiem: stare 202 plus trzy wiersze po 27 (16 czcionki + 2 * 3 `FramePadding.y` + 5 `ItemSpacing.y`), czyli lista `Lighting` z M4 oraz pole `Skybox` i suwak `Sky brightness` z M6, daje 283 przy wysokości 284. Tego rachunku nikt nie potwierdził pomiarem ani zrzutem ekranu po M6. Zapasu nie ma: nazwa karty graficznej, która na innym komputerze zawinie się do dwóch linii, da pasek przewijania |
| Lights | "The Lights panel gets what is left, which is a little less than its contents (with its Moon group folded), so it scrolls" | do M5 panel miał 466 jednostek i mieścił zawartość przy zwiniętej grupie `Moon (directional)`. Od pierwszej części M6 ma 412, o 54 mniej (dwa wiersze oddane panelowi Renderer), więc przewija się także przy zwiniętej grupie. Po jej rozwinięciu przewija się bardziej (komentarz w `LightsPanel.cpp`). W M5 liczba wierszy się nie zmieniła: linia z liczbą świateł ma inną treść, a podpowiedź przy pustej baterii nie zajmuje miejsca w panelu |
| Camera | "Camera is a little shorter than its contents, so it scrolls" | rozwinięty panel ma pasek przewijania. Wysokość spadła w M5 z 424 do 416, więc przewija się o 8 jednostek więcej |
| Gameplay | "Gameplay may start folded because the HUD shows the state of the round all the time" | 324 na 416. Wysokość zawartości nie była mierzona. Na zrzucie rozwinięty panel pokazuje wszystkie kontrolki bez paska przewijania, a pod ostatnim suwakiem zostaje wolne miejsce |
| Terrain | "Both panels are short: unfolded they cover a strip of the scene below their bar and no other panel" | 280 na 170: suwak, pole wyboru, separator i trzy linie tekstu. Wysokość zawartości nie była mierzona i rozwiniętego panelu nikt nie oglądał |
| Grass | ten sam komentarz | 324 na 190: pole wyboru, trzy suwaki, separator i jedna linia tekstu. Wysokość zawartości nie była mierzona i rozwiniętego panelu nikt nie oglądał |
| Shadows (od czwartej części M7) | "its widgets stand in the left half and the picture of the shadow map in the right half" | 612 na 324: pasek zakładek, po lewej siedem wierszy kontrolek, separator i trzy linie tekstu, po prawej podpis i kwadratowy obraz. Komentarz nie mówi, czy zawartość się mieści. Wysokość zawartości nie była mierzona i rozwiniętego panelu nikt nie klikał |
| Maze | stała bez zmian (300 na 480) | w M5 doszła jedna linia tekstu (`Crystals: ..., exit in cell ...`), czyli 16 czcionki + 5 `ItemSpacing.y` = 21 jednostek. Stary pomiar 478 plus 21 daje 499, więcej niż 480: z rachunku wynika pasek przewijania, i widać go na zrzucie. Pasek zabiera planowi 12 jednostek szerokości (`SCROLLBAR_SIZE`), więc plan jest też odrobinę mniejszy |
| Collision | "Collision and Shaders side by side, equally tall" | 312 na 280. Treść jest inna niż przy pomiarze 271 (legenda pięciu kolorów, dwie linie liczb zamiast trzech), więc stara liczba nie obowiązuje. Na zrzucie cała treść mieści się bez przewijania |
| Assets | "that panel always scrolls" | stała bez zmian (300 na 216), treść dużo dłuższa niż przy pomiarze 714: od M4 pole wyboru `Normal mapping` z zawijaną notatką i linie `normal map:`, a od M5 sześć modeli i osiem tekstur z podglądami zamiast trzech i czterech (od M6 modeli jest pięć: płytkę podłogi zastąpił teren, który modelem z pliku nie jest, a tekstur nadal osiem, bo parę tekstur podłogi zastąpiła para tekstur gruntu). Bez przewijania widać listę `View mode`, pole `Normal mapping` z notatką, listę `Filter` i suwak `Anisotropy` (tak jest na zrzucie), a obie listy zasobów leżą niżej |
| Shaders | "Collision and Shaders side by side, equally tall" | przycisk i jedenaście linii, po jednej na program (w M6 było ich sześć. Linia programu trawy ma trzy nazwy plików i jest najdłuższa: w panelu o szerokości 292 może się zawinąć, czego nikt nie oglądał). Przy sześciu liniach treść była dużo niższa niż 280. Dla jedenastu rachunek mówi co innego (policzone, nie obejrzane): jedenaście linii po 21 jednostek (16 czcionki + 5 `ItemSpacing.y`) to 231, pasek tytułu 22 i wiersz przycisku 27 dają razem 280, a do tego dochodzą separator i odstępy od krawędzi okna. Treść jest więc o około dwadzieścia jednostek wyższa niż panel i z rachunku wynika pasek przewijania także bez błędów. Tekst błędu wydłuża panel jeszcze bardziej |

**Funkcja, która z tego korzysta** ([`PanelLayout.cpp`](../../src/debug/PanelLayout.cpp)):

```cpp
void placePanelOnFirstUse(const PanelPlacement& placement) {
    // The main viewport is the window of the program. WorkPos is its top left corner and
    // WorkSize its size, in the same units as the mouse position.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // How much bigger than the reference layout the panels are drawn. The display scale
    // (stored in the style by applyTheme) makes room for the bigger font. The two ratios
    // stop the layout from growing past the window: a 1280 x 720 window at 150 % scaling
    // keeps the layout of 100 %, and the panels scroll instead of covering each other.
    const float displayScale = ImGui::GetStyle().FontScaleDpi;
    const float layoutScale = std::min({displayScale, viewport->WorkSize.x / REFERENCE_WIDTH,
                                        viewport->WorkSize.y / REFERENCE_HEIGHT});

    // The corner of the window the panel sticks to, in screen coordinates.
    const ImVec2 windowCorner{viewport->WorkPos.x + placement.corner.x * viewport->WorkSize.x,
                              viewport->WorkPos.y + placement.corner.y * viewport->WorkSize.y};

    // The offset points into the window: to the right and down from a left or top edge
    // (corner 0, direction +1), to the left and up from a right or bottom edge (corner 1,
    // direction -1).
    const ImVec2 inwards{1.0F - 2.0F * placement.corner.x, 1.0F - 2.0F * placement.corner.y};
    // A panel in a later row of folded title bars stands further from its corner by the
    // rows before it. That part is not scaled like the offset: it follows the font.
    const float rowsBefore = foldedRowsHeight(placement.foldedRowsBefore, layoutScale);
    const ImVec2 panelCorner{windowCorner.x + inwards.x * placement.offset.x * layoutScale,
                             windowCorner.y +
                                 inwards.y * (placement.offset.y * layoutScale + rowsBefore)};

    // The third argument is the pivot: the point of the panel that is put at the given
    // position, with the same 0 to 1 meaning as the corner. With the pivot equal to the
    // corner, a panel at the right edge is placed by its right corner, so its width does
    // not have to be subtracted here.
    ImGui::SetNextWindowPos(panelCorner, ImGuiCond_FirstUseEver, placement.corner);
    ImGui::SetNextWindowSize({placement.size.x * layoutScale, placement.size.y * layoutScale},
                             ImGuiCond_FirstUseEver);
    // A folded panel shows only its title bar. The size above is the one it opens to.
    ImGui::SetNextWindowCollapsed(placement.collapsed, ImGuiCond_FirstUseEver);
}

float foldedRowsHeight(int count, float gapScale) {
    // GetFrameHeight is the height of one line of widgets: the text plus the frame
    // padding above and below it. The title bar of a panel is exactly that high.
    return static_cast<float>(count) * (ImGui::GetFrameHeight() + PANEL_GAP * gapScale);
}
```

| Fragment | Co robi |
|---|---|
| `ImGui::GetMainViewport()` | główny viewport ImGui to okno programu. `WorkPos` to jego lewy górny róg, `WorkSize` rozmiar, oba we współrzędnych okna (nie w pikselach framebuffera) |
| `ImGui::GetStyle().FontScaleDpi` | skala ekranu, którą `applyTheme` zapisało w stylu (sekcja 5.8): 1 przy 100%, 1,5 przy 150%. Funkcja nie dostaje skali w argumencie i nie ma zmiennej globalnej: czyta ją stamtąd, gdzie trzyma ją ImGui |
| `std::min({displayScale, ..., ...})` | `std::min` z listą w klamrach zwraca najmniejszą z trzech liczb. Układ rośnie ze skalą ekranu, żeby zmieściła się większa czcionka, ale nigdy bardziej, niż okno jest większe od okna odniesienia |
| `windowCorner` | róg okna we współrzędnych ekranu: `corner` równe 0 daje początek, a 1 daje początek plus cały rozmiar |
| `inwards` | kierunek "do środka okna". Wzór `1 - 2 * corner` daje `+1` dla rogu 0 (od lewej krawędzi idę w prawo) i `-1` dla rogu 1 (od prawej krawędzi idę w lewo) |
| `rowsBefore` | od M6: wysokość rzędów zwiniętych pasków, które stoją przed panelem. Dla ośmiu paneli z `foldedRowsBefore = 0` to 0, dla paneli Terrain i Grass jeden rząd, dla panelu Framebuffers (M7) dwa, dla panelu Shadows (czwarta część M7) trzy |
| `panelCorner` | miejsce, w którym ma stanąć odpowiedni róg panelu: róg okna przesunięty do środka o `offset` razy skala układu, a na osi y dodatkowo o `rowsBefore`. Ta druga część nie jest mnożona przez `layoutScale` w całości: wysokość paska idzie za czcionką, skalowany jest tylko odstęp |
| `foldedRowsHeight(count, gapScale)` | `count * (ImGui::GetFrameHeight() + PANEL_GAP * gapScale)`: tyle rzędów, każdy to wysokość paska plus odstęp za nim. `GetFrameHeight()` zwraca wysokość jednej linii widżetów, czyli rozmiar czcionki plus `FramePadding.y` nad i pod tekstem: przy czcionce 16 i `FramePadding.y` 3 to 22, tyle samo co pasek tytułu. Funkcja pyta ImGui, więc wolno ją wołać tylko wewnątrz klatki ImGui (komentarz w nagłówku). Woła ją też `Hud.cpp`, z `FOLDED_ROW_COUNT`, żeby postawić HUD pod wszystkimi rzędami (sekcja 5.9.5) |
| `ImGui::SetNextWindowPos(panelCorner, ImGuiCond_FirstUseEver, placement.corner)` | trzeci argument to **pivot**: punkt panelu, który ma trafić w podaną pozycję, w tych samych jednostkach 0 do 1. Pivot `(1, 1)` znaczy "ustaw panel tak, żeby jego prawy dolny róg był w tym punkcie", więc nie muszę sam odejmować rozmiaru panelu |
| `ImGui::SetNextWindowSize({...}, ImGuiCond_FirstUseEver)` | rozmiar panelu razy skala układu. Dla panelu zwiniętego to rozmiar, który dostanie po rozwinięciu |
| `ImGui::SetNextWindowCollapsed(placement.collapsed, ImGuiCond_FirstUseEver)` | stan zwinięcia: `true` zostawia z panelu sam pasek tytułu. Funkcja jest wołana dla **każdego** panelu, także z wartością `false`, więc nie ma tu żadnego `if`: sześć paneli dostaje "rozwinięty", a Camera, Gameplay, Terrain, Grass, Framebuffers i Shadows "zwinięty" |

Wszystkie trzy funkcje `SetNextWindow...` dotyczą **następnego** okna, czyli tego, które zaraz otworzy `Begin`. Warunek `ImGuiCond_FirstUseEver` znaczy: zastosuj tylko wtedy, gdy ImGui nie ma dla tego okna zapisanych danych w `imgui.ini`. Od pierwszego uruchomienia o miejscu panelu decyduje użytkownik.

**Panel zwinięty: co to znaczy w kodzie.** Zwinięcie to zwykła funkcja okna ImGui: strzałka po lewej stronie paska tytułu (albo dwuklik w pasek) zwija okno do paska i rozwija je z powrotem. Trzy skutki:

1. `ImGui::Begin("Camera")` zwraca dla zwiniętego panelu `false`, więc `drawCameraPanel` pomija całą zawartość (żaden suwak nie jest budowany), a `ImGui::End()` wykonuje się jak zawsze. Tak samo `ImGui::Begin("Gameplay")` w `drawGameplayPanel`, `ImGui::Begin("Terrain")` w `drawTerrainPanel` i `ImGui::Begin("Grass")` w `drawGrassPanel`. Panele Framebuffers i Shadows zapamiętują ten wynik w zmiennej `open`, bo zależy od niego także flaga podglądu (sekcja 3.2). To ta sama gałąź, o której mówi komentarz przy `Begin` w każdym panelu.
2. Stan zwinięcia jest zapisywany w `imgui.ini` razem z pozycją i rozmiarem, jako linia `Collapsed=0` albo `Collapsed=1` we wpisie okna (funkcja `WindowSettingsHandler_WriteAll` w `imgui.cpp`). Po rozwinięciu panelu Camera i ponownym uruchomieniu programu panel jest więc rozwinięty: `ImGuiCond_FirstUseEver` już nie zadziała, bo wpis istnieje.
3. Dlaczego akurat Camera i Gameplay, od M6 także Terrain i Grass, od pierwszej części M7 Framebuffers, a od czwartej Shadows. Dwie kolumny i dolny rząd mają miejsce na sześć paneli, a paneli jest dwanaście (komentarze nad `CAMERA_WIDTH`, `TERRAIN_HEIGHT`, `FRAMEBUFFERS_WIDTH` i `SHADOWS_HEIGHT`). W M4 dawne miejsce panelu Camera w lewej kolumnie zajął panel Lights, a Camera stanął tam, gdzie zwinięty zasłania najmniej: przy górnej krawędzi, tuż obok lewej kolumny. W M5 obok niego, po prawej, stanął Gameplay. Ten panel może startować zwinięty z innego powodu niż Camera: stan rundy cały czas pokazuje HUD, a panel służy do zmieniania reguł, czyli do pokazu, nie do gry. Terrain i Grass doszły w drugiej części M6 i dla nich nie było już nawet wolnego paska w pierwszym rzędzie, bo Camera i Gameplay zajmują całą jego szerokość. Stanęły więc rząd niżej, każdy pod paskiem o tej samej szerokości. Oba służą do pokazu tematów 13 i 9: w czasie zwykłej gry nic nie trzeba w nich zmieniać. Framebuffers doszedł w pierwszej części M7 i stanął w trzecim rzędzie, jako jeden pasek na szerokość obu rzędów nad nim. Zwinięty jest z dodatkowego powodu, którego pozostałe nie mają: dopóki jest zwinięty, gra nie rysuje dwóch obrazów podglądu załączników (flaga `previews`, sekcja 3.2), więc zwinięty panel nic nie kosztuje. Shadows doszedł w czwartej części M7 i stanął w czwartym rzędzie, pod paskiem Framebuffers i tak samo szeroki. Ma ten sam dodatkowy powód: dopóki jest zwinięty, gra nie rysuje obrazu podglądu mapy cieni (flaga `ShadowSettings::preview`, sekcja 3.2). Samą mapę cieni gra rysuje niezależnie od panelu.

Rozwijania paneli Terrain i Grass nikt jeszcze nie sprawdził ani ręcznie, ani na zrzucie rozwiniętego panelu, a liczby 38, 60, 208 i 228 w tabelach wyżej są rachunkiem ze stałych i z wysokości paska 22. Rozwijania paneli Camera i Gameplay kliknięciem też nikt jeszcze nie sprawdził ręcznie: to, że rozwinięte prostokąty nie nachodzą na inne panele, wynika z rachunku wyżej, a oba rozwinięte panele widać na zrzucie ekranu wykonawcy, na którym stan zwinięcia ustawiła tymczasowa wstawka. Liczby 98, 120 i 422 dla panelu Shadows też są rachunkiem ze stałych i z wysokości paska 22 (policzone), a rozwijania tego panelu kliknięciem nikt nie sprawdził.

**Skala układu na liczbach.** `layoutScale` to najmniejsza z trzech liczb: skali ekranu, `szerokość okna / 1280` i `wysokość okna / 720`.

| Okno | Skala ekranu | `layoutScale` | Skutek |
|---|---|---|---|
| 1280 x 720 | 1 | min(1, 1, 1) = 1 | układ z tabeli wyżej |
| 1560 x 860 | 1 | min(1, 1,22, 1,19) = 1 | te same rozmiary paneli, ale prawa kolumna stoi przy prawej krawędzi, a dolny rząd przy dolnej, bo pozycje są liczone od rogów |
| 1920 x 1080 | 1,5 | min(1,5, 1,5, 1,5) = 1,5 | wszystko 1,5 raza większe: taki sam obraz jak przy 100% w oknie 1280 x 720 |
| 1280 x 720 | 1,5 | min(1,5, 1, 1) = 1 | prostokąty paneli jak przy 100%, ale czcionka jest 1,5 raza większa, więc zawartość się nie mieści: panele dostają paski przewijania i część etykiet jest ucięta z prawej. Panele nadal się nie zasłaniają. Lekarstwem jest większe okno |
| 960 x 540 | 1 | min(1, 0,75, 0,75) = 0,75 | układ zmniejszony do okna, panele się przewijają |

Cztery rzeczy, które z tego wynikają:

1. **To pozycje, nie dokowanie.** Panele startują jako okna pływające. Pozycja jest liczona **raz**, w pierwszej klatce, z rozmiaru okna w tej chwili. Po późniejszej zmianie rozmiaru okna programu panele zostają tam, gdzie były. Zadokowanie robi się ręcznie, przeciągając panel za pasek tytułu.
2. **`imgui.ini` wygrywa.** ImGui zapisuje pozycję, rozmiar, stan zwinięcia i dokowanie każdego okna w pliku `imgui.ini` w katalogu roboczym programu. `ImGuiCond_FirstUseEver` działa tylko dla okna, którego w tym pliku nie ma. Plik jest w [`.gitignore`](../../.gitignore), więc każda kopia repozytorium ma własny. Skutek praktyczny: `imgui.ini` zapisany przez starszą wersję programu trzyma panele na starych miejscach. Dla przejścia na układ z czwartej części M7 skutek wynika z kodu (nikt go nie oglądał): stary plik nie ma wpisu `[Window][Shadows]`, więc nowy panel staje według `SHADOWS_PLACEMENT`, zwinięty, w czwartym rzędzie, a pozostałe panele zostają tam, gdzie zapisał je plik. Jeśli w starym pliku panel Framebuffers, Camera, Gameplay, Terrain albo Grass jest zapisany jako rozwinięty, zakrywa nowy pasek albo jego część, a HUD stoi o jeden rząd niżej niezależnie od pliku. Dla przejścia na układ z M6 skutek wynika z kodu (nikt go nie oglądał): stary plik nie ma wpisów `[Window][Terrain]` ani `[Window][Grass]`, więc oba nowe panele stają według swoich stałych, zwinięte, w drugim rzędzie. Jeśli w starym pliku panel Camera albo Gameplay jest zapisany jako rozwinięty, zakrywa nowy pasek pod sobą, a HUD wisi niżej niż w M5 niezależnie od pliku, bo jego miejsce nie jest zapisywane. Dla przejścia z M4 na M5 wynika z kodu łagodny skutek (nikt go nie oglądał na ekranie): stary plik ma wpisy siedmiu paneli, więc zostają one w układzie z M4, z dolnym rzędem o wysokości 272 (góra w y 440), a wpisu `[Window][Gameplay]` nie ma, więc nowy panel staje według `GAMEPLAY_PLACEMENT`, zwinięty. Rozwinięty sięga do y 424 i na stare panele nie nachodzi. Dla wcześniejszego przejścia, na układ z M4, skutek był gorszy: stary plik ma wpis `[Window][Camera]` z miejscem w lewej kolumnie i ze stanem "rozwinięty", a wpisu `[Window][Lights]` nie ma, więc panel Lights stawał według `LIGHTS_PLACEMENT` (y od 246 do 712), prawie dokładnie na panelu Camera ze starego wpisu (y od 256 do 712, ta sama kolumna), a panel Shaders zostawał wyższy od panelu Collision. Żeby zobaczyć układ domyślny, trzeba skasować `imgui.ini` z katalogu roboczego przed uruchomieniem.
3. **Układ jest w jednym pliku, ale nikt go nie sprawdza.** Stałe zależne (`LIGHTS_HEIGHT`, `CAMERA_HEIGHT`, `ASSETS_HEIGHT`, `SHADERS_WIDTH`, `GAMEPLAY_LEFT`, `GAMEPLAY_WIDTH`) są liczone z pozostałych, więc kolumny zawsze wypełniają okno odniesienia. Tego, czy zawartość mieści się w panelu, kod nie wie: po zmianie czcionki, odstępów albo treści panelu trzeba to zmierzyć od nowa.
4. **Rozmiary wewnątrz paneli nie są skalowane przez ten kod.** `layoutScale` dotyczy tylko prostokątów paneli. Stałe w pikselach z plików paneli (podgląd tekstury 128 na 128, kropka gracza i kropki kryształów na planie) zostają takie same przy każdej skali ekranu. HUD skaluje swoje stałe sam i inną liczbą: mnoży je przez skalę ekranu (`FontScaleDpi`), a nie przez `layoutScale` (sekcja 5.9).

Stan sprawdzenia ma sześć części i nie wolno ich mieszać.

**Układ sprzed M4, bez panelu Lights (Windows, 2026-10-05, skala ekranu 100%).** Po skasowaniu `imgui.ini` układ w oknie 1280 x 720 został obejrzany na zrzucie ekranu: panele się nie zasłaniały, żaden nie wychodził poza okno, pięć z sześciu pokazywało całą zawartość. Układ w większym oknie (pierwsza klatka w oknie 1560 x 860 i 1700 x 940, ustawionym tymczasową zmianą stałych rozmiaru startowego) też został obejrzany: panele stały przy rogach. Wysokości zawartości zostały zmierzone tymczasową wstawką, która ustawiała wysokość panelu na 0 (ImGui dopasowuje wtedy wysokość do zawartości), i odczytane z zapisanego `imgui.ini`. Skala 150% była tylko **symulowana** tymczasowym mnożnikiem w kodzie (ten komputer ma skalę 100%): wiersze tabeli dla skali 1,5 w oknie 1280 x 720 zgadzały się ze zrzutem, wiersz dla 1920 x 1080 jest wnioskiem z kodu. Funkcja `placePanelOnFirstUse` od tamtej pory dostała jedną linię (`SetNextWindowCollapsed`), a rachunek pozycji i skali się nie zmienił, więc tabela ze skalą układu obowiązuje dalej.

**Układ siedmiu paneli (M4, historia).** Na Windowsie 2026-10-05 (MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74) kod z M4 budował się w Debug i Release bez ostrzeżeń, gra startowała bez linii `[error]` i bez linii `GL_`, a widok startowy był obejrzany na zrzucie ekranu. Tamten układ miał dolny rząd o wysokości 272 i jeden zwinięty panel.

**Układ ośmiu paneli (M5).** Zgłoszone przez wykonawcę dla Windowsa (2026-10-05): build Debug i Release bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach, a obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe wstawki, które zostały potem usunięte (widok startowy z HUD, widok ze schowanymi panelami, oba panele rozwinięte, karta wygranej). Prostokąty z tabeli, brak nakładania i wolny obszar sceny są policzone ze stałych, a nie zmierzone. Wysokości zawartości nie były mierzone (tabela "Co mieści swoją zawartość" podaje komentarze z kodu, rachunek i to, co widać na zrzutach). Nikt nie sprawdził ręcznie: rozwinięcia paneli Camera i Gameplay kliknięciem, układu ośmiu paneli w większym oknie ani przy skali 150%. Na macOS nic z kodu M5 nie było budowane ani uruchamiane ([`../guides/build-macos.md`](../guides/build-macos.md)). Kamień milowy M5 ma kompletny kod na Windowsie i nie jest zamknięty.

**Układ dziesięciu paneli (M6, część druga).** Zgłoszone przez wykonawcę dla Windowsa (2026-10-05): build Debug i Release bez ostrzeżeń, clang-format bez uwag, a obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe wstawki, które zostały potem usunięte. 256 przypadków testowych i 101232 asercje przechodzą w Debug i w Release: to uruchomiłem sam na zbudowanych programach testowych. Że HUD stoi po zmianie około 30 pikseli niżej przy skali 100%, to zgłoszony pomiar, który zgadza się z rachunkiem (76 zamiast 46, sekcja 5.9.5). Prostokąty paneli Terrain i Grass, brak nakładania i miejsce drugiego rzędu pasków są policzone ze stałych, a nie zmierzone. Nikt nie sprawdził ręcznie: rozwinięcia paneli Terrain i Grass, żadnego ich suwaka ani pola wyboru, układu dziesięciu paneli w większym oknie ani przy skali 150%, ani zachowania ze starym `imgui.ini`. Na macOS nic z kodu M6 nie było budowane ani uruchamiane ([`../guides/build-macos.md`](../guides/build-macos.md)). Kamień milowy M6 ma kompletny kod na Windowsie i nie jest zamknięty.

**Układ jedenastu paneli (M7, część pierwsza).** Zgłoszone przez wykonawcę dla Windowsa (2026-10-05), sam tego nie uruchamiałem: bramka `make check` przechodzi (formatowanie, testy Debug i Release, clang-tidy), zero ostrzeżeń, wtedy 269 przypadków testowych i 102103 asercje w obu konfiguracjach (po drugiej części M7 zgłoszone 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751). Miejsce paska Framebuffers (y od 68 do 90), prostokąt rozwiniętego panelu (352 do 964, 68 do 412) i nowe miejsce HUD (y 106) są policzone ze stałych, a nie zmierzone. Zgłoszone jest też, że HUD i pozostałe panele wyglądają jak przed zmianą, poza tym, że HUD stoi o jeden pasek tytułu niżej. Nikt nie sprawdził ręcznie: rozwinięcia panelu Framebuffers, żadnej jego kontrolki, tego, czy wysokość 344 mieści zawartość bez paska przewijania, układu w większym oknie ani przy skali 150%, ani zachowania ze starym `imgui.ini` (stary plik nie ma wpisu dla nowego panelu, więc panel powinien stanąć według kodu, ale HUD przesunie się niezależnie od pliku). Na macOS nic z kodu M7 nie było budowane ani uruchamiane. Druga część M7 nie zmieniła układu: panel Framebuffers ma te same stałe (`FRAMEBUFFERS_WIDTH`, `FRAMEBUFFERS_HEIGHT = 344`), zmienił się tylko komentarz przy nich. Mówi dziś, że panel jest tak szeroki, bo pokazuje cztery obrazy obok siebie pod kontrolkami w dwóch kolumnach, i że zawartość potrzebuje przy tej szerokości trochę mniej niż tej wysokości. Zgłoszony jest zrzut ekranu panelu z czterema podglądami. Sam tego nie mierzyłem. Trzecia część M7 też nie zmieniła układu ani pliku `PanelLayout.hpp`: osiem nowych kontrolek stoi w drugiej zakładce, więc panel ma nadal cztery wiersze kontrolek, a nad nimi doszedł pasek zakładek. Komentarz przy stałych ("Its contents need a little less than that height at that width") pochodzi sprzed zakładek. Czy z paskiem zakładek zawartość nadal mieści się w 344 bez paska przewijania, nikt nie zmierzył i nic o tym nie zgłoszono.

**Układ dwunastu paneli (M7, część czwarta: cienie księżyca).** Zgłoszone przez wykonawcę dla Windowsa (2026-10-05), sam tego nie uruchamiałem: bramka `make check` przechodzi (formatowanie, buildy Debug i Release, testy w obu, clang-tidy), 310 przypadków testowych i 103751 asercji. Miejsce paska Shadows (y od 98 do 120), prostokąt rozwiniętego panelu (352 do 964, 98 do 422) i nowe miejsce HUD (y 136) są policzone ze stałych i z wysokości paska 22, a nie zmierzone. Nikt nie sprawdził ręcznie: rozwinięcia panelu Shadows, żadnej jego kontrolki myszą, przełączania rozdzielczości mapy w działającym programie, tego, czy wysokość 324 mieści zawartość bez paska przewijania, układu w oknie innego rozmiaru ani przy skali 150%, ani zachowania ze starym `imgui.ini` (stary plik nie ma wpisu dla nowego panelu, więc panel powinien stanąć według kodu, a HUD przesunie się niezależnie od pliku). Na macOS nic z tego nie było budowane ani uruchamiane.

### 5.8 Motyw: kolory, metryki, czcionka i skala ekranu

Do tej zmiany panele wyglądały tak, jak daje je biblioteka: ciemny styl `ImGui::StyleColorsDark()` i wbudowana czcionka bitmapowa o wysokości 13 pikseli, bez większości polskich liter. Teraz mają własny motyw, zgodny z nastrojem gry z PRD (kamienny labirynt nocą, chłodne światło księżyca, ciepła plama latarki, świecące kryształy), i czcionkę z pliku. Cały kod jest w parze plików [`Theme.hpp`](../../src/debug/Theme.hpp) i [`Theme.cpp`](../../src/debug/Theme.cpp). Zasada nadrzędna: panele są narzędziem pokazu na obronie, na projektorze, więc **czytelność wygrywa z klimatem**.

#### 5.8.1 Jak ImGui trzyma wygląd

Wygląd wszystkich okien i widżetów opisuje jedna struktura, `ImGuiStyle`, którą zwraca `ImGui::GetStyle()`. Należy do kontekstu ImGui. Ma dwie części:

| Część | Co to jest | Przykład |
|---|---|---|
| tabela kolorów `Colors` | tablica wartości `ImVec4` (czerwony, zielony, niebieski, alfa, każda liczba od 0 do 1), indeksowana wyliczeniem `ImGuiCol_`. W naszej wersji ma 63 pozycje (`ImGuiCol_COUNT`) | `style.Colors[ImGuiCol_WindowBg]` to tło panelu |
| metryki | zwykłe pola `float` i `ImVec2`: odstępy, zaokrąglenia, grubości, rozmiar czcionki | `style.FramePadding`, `style.WindowRounding`, `style.FontSizeBase` |

Widżet nie ma własnego koloru. Rysując się, sięga do tabeli po pozycję odpowiadającą jego roli i stanowi: suwak w spoczynku bierze `ImGuiCol_FrameBg`, pod kursorem `ImGuiCol_FrameBgHovered`, a przeciągany `ImGuiCol_FrameBgActive`. Zmiana jednej pozycji tabeli zmienia więc wszystkie widżety tego rodzaju we wszystkich panelach naraz. Dlatego motyw to jedna funkcja wołana raz, a nie kod w każdym panelu.

Z tego samego powodu **tekst ma jeden kolor**, `ImGuiCol_Text`, niezależnie od tego, na czym leży. Nie da się powiedzieć "ciemny tekst na jasnym przycisku". Skutek dla palety: każde tło, na którym stoi tekst (panel, przycisk, suwak, pasek tytułu, pozycja listy), musi być ciemne, także w stanie "pod kursorem".

Kolor da się też zmienić na chwilę, dla kilku widżetów: `ImGui::PushStyleColor(ImGuiCol_Text, kolor)` odkłada bieżącą wartość na stos i ustawia nową, a `ImGui::PopStyleColor()` ją przywraca. Tak powstaje czerwony tekst błędu w panelach Shaders i Assets.

#### 5.8.2 Paleta

Kolory są zapisywane tak, jak podaje się je zwykle, liczbami od 0 do 255. Na `ImVec4` zamienia je mała funkcja z [`Theme.hpp`](../../src/debug/Theme.hpp):

```cpp
/// Makes an ImGui colour from red, green and blue written as whole numbers from 0 to 255,
/// the way colours are usually written down. ImGui wants floats from 0 to 1.
/// alpha is the opacity: 1 hides what is behind, 0 is invisible.
constexpr ImVec4 colorFromBytes(int red, int green, int blue, float alpha = 1.0F) {
    constexpr float BYTE_MAX = 255.0F;
    return {static_cast<float>(red) / BYTE_MAX, static_cast<float>(green) / BYTE_MAX,
            static_cast<float>(blue) / BYTE_MAX, alpha};
}
```

`constexpr` przy funkcji znaczy, że kompilator może ją policzyć w czasie kompilacji, więc wynik da się przypisać stałej `constexpr`. Dzielenie jest na liczbach `float` (po `static_cast`), bo `226 / 255` na liczbach całkowitych dałoby 0.

Stałe palety stoją w anonimowej przestrzeni nazw [`Theme.cpp`](../../src/debug/Theme.cpp):

```cpp
// ---- Palette: a stone maze at night ---------------------------------------------------
// Red, green and blue from 0 to 255. Text is always MOONLIGHT, so every background that
// text is drawn on is dark enough for a contrast of at least 7 to 1 (table in the
// document). The one exception is the small handle of a slider, which the digits of the
// value cross.

// Text: the pale, slightly blue white of moonlight.
constexpr ImVec4 MOONLIGHT = colorFromBytes(226, 234, 246);
// Greyed out text (hints, disabled entries): moonlight behind a cloud.
constexpr ImVec4 MOON_DIM = colorFromBytes(140, 154, 182);

// How much of a panel hides the scene behind it. A little of the scene shows through.
constexpr float PANEL_OPACITY = 0.94F;
// Tooltips and opened lists lie on top of panels and must not mix with their text.
constexpr float POPUP_OPACITY = 0.97F;

// Background of a panel: deep night navy.
constexpr ImVec4 NIGHT = colorFromBytes(14, 20, 38, PANEL_OPACITY);
// Background of tooltips and of opened lists: a darker night.
constexpr ImVec4 NIGHT_DEEP = colorFromBytes(9, 13, 26, POPUP_OPACITY);
// Title bar of a panel that is not focused, and the tab of a hidden docked panel.
constexpr ImVec4 NIGHT_RAISED = colorFromBytes(22, 31, 56);

// Resting widgets (sliders, number fields, checkboxes, tabs): muted slate.
constexpr ImVec4 SLATE = colorFromBytes(36, 47, 74);
// Resting things that are clicked (buttons, list entries, scrollbar grab): lighter slate.
constexpr ImVec4 SLATE_LIGHT = colorFromBytes(48, 62, 94);
// Resting handle of a slider, hovered scrollbar grab: the brightest slate.
constexpr ImVec4 SLATE_BRIGHT = colorFromBytes(92, 112, 156);
// Soft outline of panels and popups: the brightest slate, mostly transparent.
constexpr ImVec4 SOFT_BORDER = colorFromBytes(92, 112, 156, 0.45F);

// A widget under the mouse: the dark, warm edge of the flashlight patch. It is dark,
// not bright, because the pale text is drawn on top of it.
constexpr ImVec4 EMBER = colorFromBytes(78, 54, 22);
// A widget that is being pressed or dragged: a slightly brighter ember.
constexpr ImVec4 EMBER_BRIGHT = colorFromBytes(98, 66, 20);
// Small bright marks without text on them (check mark, line over the selected tab,
// dragged scrollbar): the amber of the flashlight.
constexpr ImVec4 AMBER = colorFromBytes(255, 184, 84);
// The handle of a slider while it is dragged: a darker amber, because the digits of the
// value are drawn across the handle and must stay readable.
constexpr ImVec4 AMBER_DEEP = colorFromBytes(172, 110, 28);

// Sparing accent (keyboard focus frame, plots, links, dragged splitter): crystal cyan.
constexpr ImVec4 CRYSTAL = colorFromBytes(86, 214, 202);
// Title bar of the focused panel, selected tab, ticked checkbox: deep crystal teal.
constexpr ImVec4 CRYSTAL_DEEP = colorFromBytes(18, 68, 76);
// Separator lines: dim crystal teal.
constexpr ImVec4 CRYSTAL_DIM = colorFromBytes(44, 104, 112);

// Overlays that tint a whole area: crystal over a place where a panel can be docked,
// crystal over selected text, night over everything behind a modal window.
constexpr ImVec4 CRYSTAL_OVERLAY = colorFromBytes(86, 214, 202, 0.35F);
constexpr ImVec4 NIGHT_OVERLAY = colorFromBytes(9, 13, 26, 0.55F);
// A barely visible lighter stripe for every second row of a table.
constexpr ImVec4 MOON_STRIPE = colorFromBytes(226, 234, 246, 0.04F);
// Nothing at all: for parts that should not be drawn.
constexpr ImVec4 INVISIBLE = colorFromBytes(0, 0, 0, 0.0F);
```

Ta sama paleta jako tabela. Kontrast jest policzony wzorem z WCAG (niżej), dla tekstu `MOONLIGHT` na danym kolorze albo dla danego koloru na tle panelu `NIGHT`:

| Stała | R, G, B (szesnastkowo) | Alfa | Rola | Kontrast |
|---|---|---|---|---|
| `MOONLIGHT` | 226, 234, 246 (`#E2EAF6`) | 1 | cały tekst, kursor w polu tekstowym | 15,12 na `NIGHT` |
| `MOON_DIM` | 140, 154, 182 (`#8C9AB6`) | 1 | tekst wyszarzony (`TextDisabled`). Panele go nie używają, używa go HUD: liczba wszystkich kryształów i czas rundy | 6,46 na `NIGHT` |
| `NIGHT` | 14, 20, 38 (`#0E1426`) | 0,94 | tło panelu, puste miejsce dokowania | tekst: 15,12 |
| `NIGHT_DEEP` | 9, 13, 26 (`#090D1A`) | 0,97 | tło podpowiedzi i rozwiniętej listy | tekst: 15,99 |
| `NIGHT_RAISED` | 22, 31, 56 (`#161F38`) | 1 | pasek tytułu panelu bez fokusu, zakładka w tle, nagłówek tabeli | tekst: 13,47 |
| `SLATE` | 36, 47, 74 (`#242F4A`) | 1 | widżet w spoczynku: suwak, pole liczbowe, pole wyboru, zakładka | tekst: 10,97 |
| `SLATE_LIGHT` | 48, 62, 94 (`#303E5E`) | 1 | przycisk, pozycja listy, uchwyt paska przewijania, róg zmiany rozmiaru | tekst: 8,77 |
| `SLATE_BRIGHT` | 92, 112, 156 (`#5C709C`) | 1 | uchwyt suwaka w spoczynku, uchwyt paska przewijania pod kursorem | tekst: 4,08 (wyjątek, niżej) |
| `SOFT_BORDER` | 92, 112, 156 | 0,45 | obwódka paneli i okienek | nie dotyczy |
| `EMBER` | 78, 54, 22 (`#4E3616`) | 1 | widżet **pod kursorem** (suwak, przycisk, pozycja listy, zakładka) | tekst: 9,30 |
| `EMBER_BRIGHT` | 98, 66, 20 (`#624214`) | 1 | widżet **wciśnięty albo przeciągany** | tekst: 7,51 |
| `AMBER` | 255, 184, 84 (`#FFB854`) | 1 | małe jasne znaki bez tekstu: ptaszek pola wyboru, kreska nad wybraną zakładką, przeciągany pasek przewijania i separator | 10,67 na `NIGHT` |
| `AMBER_DEEP` | 172, 110, 28 (`#AC6E1C`) | 1 | uchwyt przeciąganego suwaka | tekst: 3,47 (wyjątek, niżej) |
| `CRYSTAL` | 86, 214, 202 (`#56D6CA`) | 1 | akcent: ramka fokusu klawiatury, wykresy, odnośniki, separator pod kursorem | 10,35 na `NIGHT` |
| `CRYSTAL_DEEP` | 18, 68, 76 (`#12444C`) | 1 | pasek tytułu panelu **z fokusem**, wybrana zakładka, zaznaczone pole wyboru | tekst: 8,86 |
| `CRYSTAL_DIM` | 44, 104, 112 (`#2C6870`) | 1 | kreski separatorów | 2,90 na `NIGHT` (to linia, nie tekst) |
| `CRYSTAL_OVERLAY` | 86, 214, 202 | 0,35 | podgląd miejsca dokowania, zaznaczony tekst | nie dotyczy |
| `NIGHT_OVERLAY` | 9, 13, 26 | 0,55 | przyciemnienie wszystkiego za oknem modalnym | nie dotyczy |
| `MOON_STRIPE` | 226, 234, 246 | 0,04 | co drugi wiersz tabeli | nie dotyczy |
| `INVISIBLE` | 0, 0, 0 | 0 | części, których nie rysuję (tło paska przewijania, cień obwódki) | nie dotyczy |

Dziesięć stałych ma znaczenie samo w sobie i panele albo HUD sięgają po nie po nazwie, więc stoją w nagłówku [`Theme.hpp`](../../src/debug/Theme.hpp). Trzy pierwsze są sprzed M5, siedem doszło z rozgrywką:

```cpp
/// Text of a failed load (Shaders and Assets panels): a soft red that stays readable on
/// the dark panel background.
inline constexpr ImVec4 ERROR_TEXT_COLOR = colorFromBytes(255, 150, 138);

/// Walls on the plan of the Maze panel: pale stone in moonlight.
inline constexpr ImVec4 PLAN_WALL_COLOR = colorFromBytes(176, 190, 216);

/// The player on the plan of the Maze panel: the warm light of the flashlight.
inline constexpr ImVec4 PLAN_PLAYER_COLOR = colorFromBytes(255, 184, 84);

/// Crystals on the plan of the Maze panel, and everything about crystals in the HUD: the
/// cyan the crystals glow in.
inline constexpr ImVec4 PLAN_CRYSTAL_COLOR = colorFromBytes(86, 214, 202);
inline constexpr ImVec4 HUD_CRYSTAL_COLOR = PLAN_CRYSTAL_COLOR;

/// A crystal that is already collected, on the plan: only a dim trace of where it was.
inline constexpr ImVec4 PLAN_COLLECTED_COLOR = colorFromBytes(44, 104, 112);

/// The gate on the plan while it is closed: the brown of its wood, made light enough to
/// stand out from the walls. An open gate is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_GATE_COLOR = colorFromBytes(214, 142, 82);

/// The exit zone on the plan: a soft green, the colour of "this way out".
inline constexpr ImVec4 PLAN_EXIT_COLOR = colorFromBytes(132, 220, 140);

/// The battery bar of the HUD: the warm light of the flashlight while there is charge,
/// the soft red of an error once it is low.
inline constexpr ImVec4 HUD_BATTERY_COLOR = PLAN_PLAYER_COLOR;
inline constexpr ImVec4 HUD_BATTERY_LOW_COLOR = ERROR_TEXT_COLOR;
```

Trzy z nowych stałych nie mają własnych liczb, tylko nazywają istniejący kolor drugim imieniem: `HUD_CRYSTAL_COLOR` to `PLAN_CRYSTAL_COLOR`, `HUD_BATTERY_COLOR` to `PLAN_PLAYER_COLOR`, a `HUD_BATTERY_LOW_COLOR` to `ERROR_TEXT_COLOR`. Kod HUD mówi dzięki temu, co kolor znaczy u niego (bateria, kryształy), a zmiana koloru latarki na planie zmienia też pasek baterii. Liczby `PLAN_CRYSTAL_COLOR` i `PLAN_COLLECTED_COLOR` są te same co stałych palety `CRYSTAL` i `CRYSTAL_DIM` z `Theme.cpp`, a `PLAN_PLAYER_COLOR` te same co `AMBER`. Są wpisane drugi raz, bo paleta stoi w anonimowej przestrzeni nazw pliku `.cpp` i nagłówek jej nie widzi.

Kontrast w tabeli jest policzony wzorem WCAG z następnego akapitu, na nieprzezroczystym kolorze `NIGHT`, tym samym, na którym liczona jest reszta palety. Liczby dla siedmiu stałych z M5 policzyłem skryptem w Pythonie, który podstawia ten wzór. Skrypt odtwarza też stare wartości z tej tabeli (8,70, 9,77 i 10,67), więc liczy tak samo jak one:

| Stała | R, G, B (szesnastkowo) | Gdzie | Kontrast na `NIGHT` |
|---|---|---|---|
| `ERROR_TEXT_COLOR` | 255, 150, 138 (`#FF968A`) | tekst błędu w panelu Shaders, lista `Failed to load` w panelu Assets | 8,70 |
| `PLAN_WALL_COLOR` | 176, 190, 216 (`#B0BED8`) | ściany na planie w panelu Maze | 9,77 |
| `PLAN_PLAYER_COLOR` | 255, 184, 84 (`#FFB854`) | kropka gracza i kreska kierunku na planie: gracz niesie latarkę, więc ma jej kolor | 10,67 |
| `PLAN_CRYSTAL_COLOR` | 86, 214, 202 (`#56D6CA`) | wypełniona kropka kryształu, który jeszcze leży, na planie | 10,35 |
| `PLAN_COLLECTED_COLOR` | 44, 104, 112 (`#2C6870`) | pusty okrąg w miejscu zebranego kryształu i linia otwartej bramy na planie | 2,90 (celowo słaby: to ślad, a nie informacja do czytania) |
| `PLAN_GATE_COLOR` | 214, 142, 82 (`#D68E52`) | linia zamkniętej bramy na planie, grubsza od ścian (3 piksele zamiast 1) | 6,85 (tuż poniżej progu 7, ale to gruba linia, nie tekst) |
| `PLAN_EXIT_COLOR` | 132, 220, 140 (`#84DC8C`) | obrys strefy wyjścia na planie | 10,99 |
| `HUD_CRYSTAL_COLOR` | jak `PLAN_CRYSTAL_COLOR` | słowo `Crystals` i podpowiedź o otwartej bramie na pasku HUD, tytuł karty `You escaped` | 10,35, ale tło HUD jest inne (niżej) |
| `HUD_BATTERY_COLOR` | jak `PLAN_PLAYER_COLOR` | wypełnienie paska baterii, linia `R: play again` na karcie | 10,67, tło HUD niżej |
| `HUD_BATTERY_LOW_COLOR` | jak `ERROR_TEXT_COLOR` | wypełnienie paska słabej baterii, podpowiedź o pustej baterii | 8,70, tło HUD niżej |

Przed tą zmianą czerwień błędu była zapisana dwa razy, w dwóch plikach paneli, jako `{1.0F, 0.4F, 0.4F, 1.0F}`. Na nowym tle dawałaby kontrast 6,40, poniżej progu. Kolory planu były stałymi `IM_COL32` w `MazePanel.cpp` (szary i jaskrawa zieleń).

**Skąd te liczby: kontrast według WCAG.** Kontrast dwóch kolorów to iloraz `(L1 + 0,05) / (L2 + 0,05)`, gdzie `L1` to luminancja względna jaśniejszego koloru, a `L2` ciemniejszego. Wynik jest między 1 (ten sam kolor) a 21 (czerń i biel). Luminancję liczy się w dwóch krokach:

1. każdą składową `c` z zakresu 0 do 1 zamienia się z zapisu sRGB na wartość liniową: `c / 12,92` dla `c` do 0,04045, a powyżej `((c + 0,055) / 1,055)` do potęgi 2,4,
2. `L = 0,2126 * R + 0,7152 * G + 0,0722 * B` (zielony waży najwięcej, bo oko jest na niego najczulsze).

Przykład dla tekstu na tle panelu: `MOONLIGHT` ma `L = 0,817`, `NIGHT` ma `L = 0,007`, więc kontrast to `0,867 / 0,057 = 15,1`. Próg, który przyjąłem, to **7 do 1** (poziom AAA w WCAG dla zwykłego tekstu).

**Tło panelu jest lekko przezroczyste**, więc prawdziwe tło tekstu zależy od sceny. Najgorszy przypadek to biel za panelem (ściana bez tekstury jest biała): 94% koloru `NIGHT` i 6% bieli daje w przybliżeniu kolor 28, 34, 51, a kontrast tekstu spada z 15,12 do 13,05. Dla tekstu błędu z 8,70 do 7,51, dla ścian planu z 9,77 do 8,43. Tekst zostaje powyżej 7. Kolory planu z M5 spadają tak samo: kryształ z 10,35 do 8,93, strefa wyjścia z 10,99 do 9,48, brama z 6,85 do 5,91.

**Tło HUD jest dużo bardziej przezroczyste, więc próg 7 nie jest tam gwarantowany.** Oba okna HUD rysują tło tym samym kolorem `NIGHT`, ale z własną alfą: `ImGui::SetNextWindowBgAlpha` podmienia alfę tła następnego okna (w `imgui.cpp` wartość alfy koloru jest zastępowana, a nie mnożona). Pasek ma `HUD_OPACITY = 0.72F`, karta `CARD_OPACITY = 0.9F`. Policzyłem tym samym skryptem dwa skrajne przypadki, czarną i białą scenę za oknem. Mieszanie jest liczone tak jak w akapicie wyżej, na zapisanych składowych koloru (72% `NIGHT` i 28% bieli to w przybliżeniu 82, 86, 99):

| Kolor na HUD | Pasek, za nim czerń | Pasek, za nim biel | Karta, za nią czerń | Karta, za nią biel |
|---|---|---|---|---|
| tekst `MOONLIGHT` (liczby, czas na karcie) | 15,84 | 6,06 | 15,39 | 11,55 |
| `HUD_CRYSTAL_COLOR` | 10,85 | 4,15 | 10,54 | 7,91 |
| `HUD_BATTERY_COLOR` | 11,18 | 4,28 | 10,86 | 8,16 |
| `HUD_BATTERY_LOW_COLOR` | 9,12 | 3,49 | 8,86 | 6,65 |
| `MOON_DIM` (`TextDisabled`: liczba wszystkich kryształów, czas na pasku) | 6,77 | 2,59 | nie występuje | nie występuje |

Wniosek: karta trzyma próg 7 prawie zawsze (wyjątkiem jest czerwień na białym tle, 6,65, ale na karcie czerwieni nie ma), a pasek tylko na ciemnej scenie. Gra dzieje się nocą i kolor czyszczenia jest prawie czarny, więc zwykle obowiązuje lewa kolumna. Gdy za paskiem znajdzie się jasno oświetlona ściana, kontrast spada w stronę prawej. To rachunek dla dwóch skrajnych teł, a nie pomiar na ekranie: nikt nie mierzył kontrastu paska na prawdziwej klatce gry.

**Dwa świadome wyjątki od progu 7:**

- **Uchwyt suwaka.** ImGui rysuje wartość suwaka na jego środku, a uchwyt przesuwa się pod cyframi. Tam, gdzie cyfra leży na uchwycie, kontrast wynosi 4,08 (`SLATE_BRIGHT`) albo 3,47 (`AMBER_DEEP`, przy przeciąganiu). Uchwyt musi być wyraźnie jaśniejszy od tła suwaka, bo inaczej nie byłoby go widać, więc te dwa wymagania się wykluczają. Standardowy styl ImGui ma tę samą cechę. Jasny `AMBER` (kontrast z tekstem około 1,4) dla uchwytu odrzuciłem właśnie z tego powodu.
- **Tekst wyszarzony.** `MOON_DIM` (6,46) i widżety w bloku `BeginDisabled` (kolory mnożone przez `DISABLED_ALPHA`, tekst wychodzi wtedy na około 3,9) są celowo słabsze: mają wyglądać na nieaktywne.

**Przypisanie kolorów do tabeli ImGui** (początek funkcji, reszta w pliku):

```cpp
// Fills the colour table of the style. Every entry is set, so that no colour of the
// stock style is left over.
void applyColors(ImGuiStyle& style) {
    // Colors is an array with one ImVec4 for every value of the enum ImGuiCol_.
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = MOONLIGHT;
    colors[ImGuiCol_TextDisabled] = MOON_DIM;
    colors[ImGuiCol_TextLink] = CRYSTAL;
    colors[ImGuiCol_TextSelectedBg] = CRYSTAL_OVERLAY;
    colors[ImGuiCol_InputTextCursor] = MOONLIGHT;

    // Backgrounds and outlines.
    colors[ImGuiCol_WindowBg] = NIGHT;
    colors[ImGuiCol_ChildBg] = INVISIBLE;
    colors[ImGuiCol_PopupBg] = NIGHT_DEEP;
    colors[ImGuiCol_MenuBarBg] = NIGHT_RAISED;
    colors[ImGuiCol_Border] = SOFT_BORDER;
    colors[ImGuiCol_BorderShadow] = INVISIBLE;

    // Title bars: only the focused panel gets the crystal accent.
    colors[ImGuiCol_TitleBg] = NIGHT_RAISED;
    colors[ImGuiCol_TitleBgActive] = CRYSTAL_DEEP;
    colors[ImGuiCol_TitleBgCollapsed] = NIGHT_RAISED;

    // Widgets with a frame: slate at rest, ember under the mouse, brighter ember when used.
    colors[ImGuiCol_FrameBg] = SLATE;
    colors[ImGuiCol_FrameBgHovered] = EMBER;
    colors[ImGuiCol_FrameBgActive] = EMBER_BRIGHT;
    colors[ImGuiCol_CheckboxSelectedBg] = CRYSTAL_DEEP;
    colors[ImGuiCol_CheckMark] = AMBER;
    colors[ImGuiCol_SliderGrab] = SLATE_BRIGHT;
    colors[ImGuiCol_SliderGrabActive] = AMBER_DEEP;

    colors[ImGuiCol_Button] = SLATE_LIGHT;
    colors[ImGuiCol_ButtonHovered] = EMBER;
    colors[ImGuiCol_ButtonActive] = EMBER_BRIGHT;

    // Header colours are used by the entries of an opened list (Combo) and by the title
    // bars of foldable groups (CollapsingHeader).
    colors[ImGuiCol_Header] = SLATE_LIGHT;
    colors[ImGuiCol_HeaderHovered] = EMBER;
    colors[ImGuiCol_HeaderActive] = EMBER_BRIGHT;
```

`style.Colors` to zwykła tablica, więc `ImVec4* colors = style.Colors;` daje wskaźnik na jej pierwszy element, a `colors[ImGuiCol_Text]` to element o numerze równym wartości wyliczenia. Dalej funkcja ustawia w ten sam sposób pozostałe grupy: paski przewijania, linie, róg zmiany rozmiaru, zakładki, dokowanie, wykresy, tabele, przeciąganie i nawigację klawiaturą. Ustawia **wszystkie** pozycje, także te, których panele dziś nie używają: inaczej po dodaniu pierwszej tabeli albo wykresu wyskoczyłby standardowy niebieski.

Trzy kolory `Header` mają dwóch użytkowników, i komentarz wymienia obu: pozycje otwartej listy (`Combo`) oraz paski tytułów zwijanych grup (`CollapsingHeader`), których używa panel Lights. Zwinięta grupa ma więc w spoczynku ten sam kolor `SLATE_LIGHT` co przycisk.

Reguła stanów, którą warto umieć powiedzieć jednym zdaniem: **łupek (slate) w spoczynku, żar (ember) pod kursorem, jaśniejszy żar przy użyciu, kryształ dla fokusu i zaznaczenia.**

| Stan widżetu | Przycisk | Suwak, pole | Pole wyboru | Pozycja listy, pasek zwijanej grupy |
|---|---|---|---|---|
| spoczynek | `SLATE_LIGHT` | `SLATE`, uchwyt `SLATE_BRIGHT` | `SLATE` (puste), `CRYSTAL_DEEP` z ptaszkiem `AMBER` (zaznaczone) | `SLATE_LIGHT`, gdy wybrana |
| pod kursorem | `EMBER` | `EMBER` | `EMBER` | `EMBER` |
| wciśnięty, przeciągany | `EMBER_BRIGHT` | `EMBER_BRIGHT`, uchwyt `AMBER_DEEP` | `EMBER_BRIGHT` | `EMBER_BRIGHT` |
| wyłączony (`BeginDisabled`) | wszystkie kolory z alfą razy 0,45 | tak samo | tak samo | tak samo |

#### 5.8.3 Metryki

```cpp
// ---- Metrics, in pixels at 100 % display scaling ---------------------------------------

// Height of the text. The built-in font of ImGui is 13 pixels high.
constexpr float FONT_SIZE = 16.0F;

// Free space between the edge of a panel and its contents.
constexpr ImVec2 WINDOW_PADDING{10.0F, 10.0F};
// Free space between the edge of a widget (button, slider) and its text.
constexpr ImVec2 FRAME_PADDING{8.0F, 3.0F};
// Distance between two widgets: to the next one in a line, and to the next line.
constexpr ImVec2 ITEM_SPACING{8.0F, 5.0F};
// Distance between the parts of one widget, for example a slider and its label.
constexpr ImVec2 ITEM_INNER_SPACING{6.0F, 4.0F};

// Radius of the corners of panels.
constexpr float WINDOW_ROUNDING = 8.0F;
// Radius of the corners of widgets, popups, tabs and scrollbars.
constexpr float WIDGET_ROUNDING = 5.0F;
// Thickness of the outline of panels and popups.
constexpr float BORDER_SIZE = 1.0F;
// Width of a scrollbar.
constexpr float SCROLLBAR_SIZE = 12.0F;
// Smallest width of the handle of a slider.
constexpr float GRAB_MIN_SIZE = 12.0F;

// Opacity of widgets inside ImGui::BeginDisabled: low enough to tell them apart at once.
constexpr float DISABLED_ALPHA = 0.45F;
```

```cpp
// Sets paddings, spacings, roundings and sizes: softer and a little roomier than stock.
void applyMetrics(ImGuiStyle& style) {
    style.WindowPadding = WINDOW_PADDING;
    style.FramePadding = FRAME_PADDING;
    style.ItemSpacing = ITEM_SPACING;
    style.ItemInnerSpacing = ITEM_INNER_SPACING;

    style.WindowRounding = WINDOW_ROUNDING;
    style.ChildRounding = WIDGET_ROUNDING;
    style.PopupRounding = WIDGET_ROUNDING;
    style.FrameRounding = WIDGET_ROUNDING;
    style.GrabRounding = WIDGET_ROUNDING;
    style.TabRounding = WIDGET_ROUNDING;
    style.ScrollbarRounding = WIDGET_ROUNDING;

    style.WindowBorderSize = BORDER_SIZE;
    style.PopupBorderSize = BORDER_SIZE;
    style.ScrollbarSize = SCROLLBAR_SIZE;
    style.GrabMinSize = GRAB_MIN_SIZE;

    style.DisabledAlpha = DISABLED_ALPHA;
}
```

| Stała | Wartość | Pole `ImGuiStyle` | Wartość standardowa ImGui | Po co zmiana |
|---|---|---|---|---|
| `FONT_SIZE` | 16 | `FontSizeBase` | 13 (wbudowana czcionka) | czytelność na projektorze |
| `WINDOW_PADDING` | 10, 10 | `WindowPadding` | 8, 8 | więcej powietrza przy krawędzi panelu |
| `FRAME_PADDING` | 8, 3 | `FramePadding` | 4, 3 | szersze przyciski i pola. W pionie bez zmian, bo każdy piksel wysokości mnoży się przez liczbę wierszy panelu Camera |
| `ITEM_SPACING` | 8, 5 | `ItemSpacing` | 8, 4 | odstęp między wierszami |
| `ITEM_INNER_SPACING` | 6, 4 | `ItemInnerSpacing` | 4, 4 | odstęp między suwakiem a jego etykietą |
| `WINDOW_ROUNDING` | 8 | `WindowRounding` | 0 | zaokrąglone rogi paneli |
| `WIDGET_ROUNDING` | 5 | `ChildRounding`, `PopupRounding`, `FrameRounding`, `GrabRounding`, `TabRounding`, `ScrollbarRounding` | od 0 do 9, zależnie od pola | jeden promień dla wszystkich małych elementów |
| `BORDER_SIZE` | 1 | `WindowBorderSize`, `PopupBorderSize` | 1 | wpisane jawnie, żeby obwódka nie zależała od wartości standardowej |
| `SCROLLBAR_SIZE` | 12 | `ScrollbarSize` | 14 | węższy pasek, więcej miejsca na treść |
| `GRAB_MIN_SIZE` | 12 | `GrabMinSize` | 12 | wpisane jawnie, bo od szerokości uchwytu zależy, ile cyfr zasłania |
| `DISABLED_ALPHA` | 0,45 | `DisabledAlpha` | 0,6 | wyszarzony suwak `Anisotropy` ma być widać od razu |

Wartości standardowe w tej tabeli pochodzą z konstruktora `ImGuiStyle` w pobranym źródle (`imgui.cpp`). Wysokość jednego wiersza z suwakiem wychodzi z metryk tak: wysokość czcionki plus dwa razy `FramePadding.y`, plus `ItemSpacing.y` do następnego wiersza. Od tych liczb zależą wysokości paneli w sekcji 5.7.

#### 5.8.4 `applyTheme` i skala ekranu

```cpp
void applyTheme(float scale) {
    // A display that reports no usable scale is treated as 100 %.
    const float usedScale = scale > 0.0F ? scale : 1.0F;

    // The style holds everything about the look: a table of colours and the metrics.
    ImGuiStyle& style = ImGui::GetStyle();
    // Start from the stock dark style, so that an entry added by a later version of
    // ImGui gets a sensible dark colour and not black.
    ImGui::StyleColorsDark(&style);
    applyColors(style);
    applyMetrics(style);

    // ScaleAllSizes multiplies every metric set above by the scale. It does not touch the
    // font: FontScaleDpi does that. The final text height is FontSizeBase * FontScaleDpi.
    style.ScaleAllSizes(usedScale);
    style.FontSizeBase = FONT_SIZE;
    style.FontScaleDpi = usedScale;
}
```

| Linia | Co robi |
|---|---|
| `const float usedScale = scale > 0.0F ? scale : 1.0F;` | zabezpieczenie: komentarz w backendzie GLFW ostrzega, że niektóre wirtualne monitory zgłaszają skalę 0. Skala 0 wyzerowałaby wszystkie rozmiary |
| `ImGuiStyle& style = ImGui::GetStyle();` | referencja do stylu bieżącego kontekstu. Bez `const`, bo go zmieniam |
| `ImGui::StyleColorsDark(&style);` | zaczynam od standardowej ciemnej tabeli kolorów. Moja funkcja i tak nadpisuje wszystkie 63 pozycje, ale po podniesieniu wersji ImGui nowa pozycja wyliczenia dostanie sensowny ciemny kolor, a nie przypadkowy |
| `applyColors(style); applyMetrics(style);` | paleta i metryki z sekcji 5.8.2 i 5.8.3 |
| `style.ScaleAllSizes(usedScale);` | mnoży wszystkie metryki przez skalę i obcina do pełnych pikseli. **Nie rusza czcionki.** Operacja jest stratna (zaokrąglenie), dlatego wołam ją raz, na świeżo ustawionych wartościach |
| `style.FontSizeBase = FONT_SIZE;` | rozmiar tekstu przed skalowaniem, w ImGui 1.92 pole stylu, a nie argument przy wczytywaniu czcionki |
| `style.FontScaleDpi = usedScale;` | mnożnik czcionki od gęstości ekranu. Ostateczna wysokość tekstu to `FontSizeBase * FontScaleMain * FontScaleDpi` (`FontScaleMain` zostaje równe 1) |

**Skąd bierze się skala.** W konstruktorze `DebugUI` (sekcja 3.1) stoi `applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()));`. Ta funkcja backendu GLFW robi na dwóch systemach dwie różne rzeczy i to jest sedno:

| System | Jak system obsługuje gęsty ekran | Co zwraca funkcja | Kto powiększa panele |
|---|---|---|---|
| Windows | okno ma tyle samo pikseli co framebuffer. Przy skali 150% program dostaje po prostu mniejsze fizycznie piksele i sam ma narysować wszystko większe | `x_scale` z `glfwGetWindowContentScale`: 1 przy 100%, 1,5 przy 150% | mój kod: `ScaleAllSizes` i `FontScaleDpi` |
| macOS | okno 1280 x 720 "punktów" ma framebuffer 2560 x 1440 pikseli (Retina). ImGui układa panele w punktach, a backend rysuje je w gęstszych pikselach | zawsze 1 (w źródle backendu: platformy Apple używają `FramebufferScale`) | ImGui: skala `framebuffer / okno` z `ImGui_ImplGlfw_NewFrame`, bez mojego kodu |

Gdybym zamiast funkcji backendu zawołał sam `glfwGetWindowContentScale`, na Retinie dostałbym 2 i panele byłyby powiększone **dwa razy**: raz przez mój kod, drugi raz przez skalę framebuffera.

Czego ten kod nie robi: skala jest czytana raz, przy starcie. Po przeciągnięciu okna na monitor o innej skali panele zostają w starej. ImGui ma na to pole `io.ConfigDpiScaleFonts`, ale opisuje je jako eksperymentalne i skaluje ono tylko czcionkę, bez odstępów, więc go nie włączam.

#### 5.8.5 Czcionka

Plik: [`assets/fonts/AtkinsonHyperlegible-Regular.ttf`](../../assets/fonts/AtkinsonHyperlegible-Regular.ttf), czcionka Atkinson Hyperlegible w wersji 1.006, 54 348 bajtów, na licencji SIL Open Font License 1.1 (plik [`OFL.txt`](../../assets/fonts/OFL.txt) obok, źródło i suma kontrolna w [`README.md`](../../assets/fonts/README.md) tego katalogu). Wybrałem ją z trzech powodów: została zaprojektowana dla osób słabowidzących, więc znaki łatwe do pomylenia (I, l, 1, O, 0) mają wyraźnie różne kształty (zero jest przekreślone), ma wszystkie polskie litery, i jest jednym małym plikiem statycznym z jedną grubością.

```cpp
// ---- Font ------------------------------------------------------------------------------

// The font file, relative to the assets directory. Licence: assets/fonts/OFL.txt.
constexpr const char* FONT_FILE = "fonts/AtkinsonHyperlegible-Regular.ttf";

// ImGui refuses (with an assertion) data of 100 bytes or less: no font file is that small.
constexpr std::size_t SMALLEST_FONT_FILE = 101;

// The first four bytes of every TrueType font file: the number 1.0 written as two
// 16 bit halves.
constexpr std::array<unsigned char, 4> TRUETYPE_SIGNATURE{0x00, 0x01, 0x00, 0x00};

// Size 0 tells ImGui to take the size of the text from the style (FontSizeBase).
constexpr float SIZE_FROM_STYLE = 0.0F;
```

```cpp
// Reads a whole file into bytes. Returns false when the file cannot be opened.
bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes) {
    // The stream takes the path object itself, so on Windows a letter outside the local
    // code page is not damaged (same reason as in assets/ImageLoader.cpp).
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}
```

```cpp
// True when the bytes can be handed to ImGui: their number is above its lower limit and
// fits the int it takes the size in, and they start like a TrueType font. In a debug
// build ImGui stops the program with an assertion when it is given something else, for
// example an empty file or a text file saved under the name of the font.
bool looksLikeFont(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < SMALLEST_FONT_FILE ||
        bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    // std::equal compares the four bytes of the signature with the first four of the file.
    return std::equal(TRUETYPE_SIGNATURE.begin(), TRUETYPE_SIGNATURE.end(), bytes.begin());
}
```

```cpp
void loadFont(std::vector<unsigned char>& fontBytes) {
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    const std::filesystem::path path = core::assetPath(FONT_FILE);

    if (readBinaryFile(path, fontBytes) && looksLikeFont(fontBytes)) {
        // By default ImGui takes the bytes over and frees them with its own allocator.
        // They belong to a std::vector, so ImGui must only use them, not free them.
        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;
        // No glyph ranges: since ImGui 1.92 a letter is drawn into the font texture the
        // first time it is used, so the Polish letters need no extra setup.
        const ImFont* font = fonts->AddFontFromMemoryTTF(
            fontBytes.data(), static_cast<int>(fontBytes.size()), SIZE_FROM_STYLE, &config);
        if (font != nullptr) {
            return;
        }
    }

    core::logError("Panel font cannot be loaded, using the built-in font: " + core::pathText(path));
    // ImGui holds no pointer to the bytes after a failure, so they can be dropped.
    fontBytes.clear();
    // The built-in font that can be scaled, so the display scale still works.
    fonts->AddFontDefaultVector();
}
```

| Linia | Co robi |
|---|---|
| `ImFontAtlas* fonts = ImGui::GetIO().Fonts;` | atlas czcionek: obiekt ImGui, który trzyma wczytane czcionki i teksturę z ich znakami |
| `core::assetPath(FONT_FILE)` | ścieżka do pliku w katalogu `assets` obok programu ([`core/paths.md`](core/paths.md)). Nie zależy od katalogu roboczego |
| `readBinaryFile(path, fontBytes)` | cały plik do wektora bajtów. Strumień dostaje obiekt `path`, a nie napis, więc na Windowsie ścieżka z literą spoza strony kodowej otwiera się poprawnie. To ten sam wzór co w `assets::loadImage` ([`assets/images.md`](assets/images.md), sekcje 2.4 i 5.3) |
| `looksLikeFont(fontBytes)` | trzy warunki: więcej niż 100 bajtów (ImGui ma asercję `font_data_size > 100`), rozmiar mieści się w `int`, pierwsze cztery bajty to `00 01 00 00`, czyli początek każdego pliku TrueType |
| `config.FontDataOwnedByAtlas = false;` | **kto zwalnia bajty.** Domyślnie `AddFontFromMemoryTTF` przejmuje wskaźnik i przy niszczeniu atlasu zwalnia go własną funkcją. Moje bajty należą do `std::vector`, który zwolni je sam: dwa zwolnienia tej samej pamięci to uszkodzenie sterty. Z `false` ImGui tylko czyta |
| `fonts->AddFontFromMemoryTTF(fontBytes.data(), static_cast<int>(fontBytes.size()), SIZE_FROM_STYLE, &config)` | rejestruje czcionkę z pamięci. Rozmiar 0 znaczy "weź z `style.FontSizeBase`". Zwraca wskaźnik na czcionkę albo `nullptr`, gdy dane nie są czcionką |
| `core::logError(...)` | jedna linia `[error]` z pełną ścieżką pliku, którego nie udało się użyć |
| `fontBytes.clear();` | po porażce ImGui nie trzyma wskaźnika na bajty (wycofuje wpis), więc nie ma po co ich trzymać |
| `fonts->AddFontDefaultVector();` | czcionka wbudowana w ImGui w wersji skalowalnej. Program działa dalej, tylko bez polskich liter |

**Dlaczego z pamięci, a nie `AddFontFromFileTTF`.** Sprawdziłem w źródle pobranej wersji (`imgui.cpp`, funkcja `ImFileOpen`), że ImGui otwiera plik poprawnie także na Windowsie: zamienia nazwę z UTF-8 na znaki szerokie i woła `_wfopen`. Ścieżka z polską literą nie byłaby więc problemem. Problemem jest brak pliku: `AddFontFromFileTTF` domyślnie kończy się wtedy makrem `IM_ASSERT_USER_ERROR(0, "Could not load font file!")`, czyli w buildzie Debug **asercją, która zatrzymuje program**. Brak czcionki paneli debug nie może zamykać gry. W źródle jest flaga, która tę asercję wyłącza (`ImFontFlags_NoLoadError`, ustawiana w polu `ImFontConfig::Flags`), ale to pole stoi w `imgui.h` w części opisanej jako wewnętrzna, z komentarzem "don't use just yet", więc na niej nie polegam. Czytając plik sam, decyduję o tym, co się dzieje, gdy go nie ma, używając tylko publicznego API.

**Gdzie żyją bajty.** Skoro ImGui tylko trzyma wskaźnik, bajty muszą istnieć tak długo jak atlas, czyli do `ImGui::DestroyContext()` (komentarz przy polu `FontDataOwnedByAtlas` w `imgui.h` mówi to wielkimi literami: od wersji 1.92 dane muszą przetrwać cały czas życia atlasu, bo znaki są rysowane do tekstury dopiero wtedy, gdy są potrzebne). Dlatego wektor jest polem `DebugUI`:

```cpp
    // The bytes of the panel font file. ImGui only keeps a pointer to them, so they live
    // here. The body of the destructor destroys the ImGui context first and the members
    // are destroyed after it, so the bytes outlive every use of the pointer.
    std::vector<unsigned char> m_fontBytes;
```

Kolejność niszczenia obiektu w C++: najpierw wykonuje się ciało destruktora, potem niszczone są pola. Ciało `~DebugUI` woła `ImGui::DestroyContext()`, więc atlas znika, zanim zniknie wektor. Zmienna `static` w `Theme.cpp` też by zadziałała, ale byłaby stanem globalnym, którego w tym module nie ma.

**Polskie litery bez dodatkowego kodu.** W ImGui przed wersją 1.92 trzeba było przy wczytywaniu czcionki podać zakresy znaków (glyph ranges), a wszystkie znaki były rysowane do tekstury z góry. W wersji 1.92 system czcionek jest dynamiczny: backend zgłasza flagę `ImGuiBackendFlags_RendererHasTextures` (nasz backend OpenGL3 ją ustawia), a znak trafia do tekstury w chwili pierwszego użycia. Dlatego wywołanie nie ma argumentu z zakresami, a rozmiar czcionki można zmienić polem stylu bez przebudowywania czegokolwiek. Poradniki pisane dla starszych wersji pokazują tu inny kod.

**Co przechodzi przez tę funkcję, a co nie** (zmierzone na Windowsie, build Debug):

| Sytuacja | Wynik |
|---|---|
| plik jest na miejscu | panele w czcionce Atkinson Hyperlegible, bez żadnej linii w konsoli |
| pliku nie ma (usunięty z kopii `assets` obok programu) | jedna linia `[error] Panel font cannot be loaded, using the built-in font: ...`, panele w czcionce wbudowanej, program działa |
| plik o tej nazwie nie jest czcionką (300 przypadkowych bajtów) | to samo: odrzuca go `looksLikeFont` |
| plik zaczyna się jak TrueType, ale jest uszkodzony dalej | niesprawdzone. Z kodu ImGui wynika: w Release `AddFontFromMemoryTTF` zwraca `nullptr` i działa czcionka wbudowana, w Debug może zadziałać asercja wewnątrz ImGui |

Sprawdzenie sygnatury dopisałem po pomiarze: bez niego plik z przypadkowymi bajtami kończył się w buildzie Debug asercją ImGui `stbtt_GetFontOffsetForIndex(): FontData is incorrect`.

#### 5.8.6 Stan sprawdzenia motywu

Ta lista opisuje sprawdzenie motywu z dnia, w którym powstał, czyli sprzed M4: paneli było wtedy sześć, a programów shaderów trzy. Motyw (kolory, metryki, czcionka) nie zmienił się od tamtej pory. Co jest sprawdzone po dodaniu panelu Lights, mówią sekcje 5.7 i 6.

Zmierzone na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, ekran 1920 x 1080 przy skali 100%), na zrzutach ekranu:

- ówczesne panele (stan sprzed M4, bez Lights) w nowym motywie w oknie 1280 x 720 i w większym oknie, środek obszaru dokowania przezroczysty (widać scenę),
- polskie litery: tekst `Zażółć gęślą jaźń` oraz komplet małych i wielkich liter z ogonkami, a także podpowiedź z napisem zawierającym `Żółw`, wypisane tymczasową wstawką w kodzie (z literału `u8` z kodami znaków, więc bez zależności od kodowania pliku źródłowego),
- prawdziwy tekst błędu w panelu Shaders (zepsuty plik `color.frag` w kopii `assets`) i prawdziwy wpis `Failed to load` w panelu Assets (zmieniona nazwa tekstury), oba na tle sceny z białymi ścianami, czyli w najgorszym przypadku dla kontrastu,
- stany widżetów (spoczynek, pod kursorem, wciśnięty, wyłączony), wybrana zakładka, lista z wybraną pozycją i wykres: narysowane tymczasową wstawką, która podstawiała kolor stanu przez `PushStyleColor`. To sprawdza kolory, a nie samo najeżdżanie myszą,
- czcionka zastępcza przy braku pliku i przy pliku, który nie jest czcionką,
- jedna prawdziwa podpowiedź: na jednym ze zrzutów kursor stał akurat nad nazwą modelu w panelu Assets i widać ciemne okienko z pełną ścieżką pliku,
- skala 150% **symulowana** mnożnikiem w kodzie: czcionka i odstępy rosną, tekst jest ostry.

Wszystkie tymczasowe wstawki są usunięte. Niesprawdzone: prawdziwe najeżdżanie i klikanie myszą w nowym motywie, prawdziwy ekran ze skalą 150% (ten komputer ma skalę 100%, systemowe DPI 96, więc funkcja `ImGui_ImplGlfw_GetContentScaleForWindow` daje tu 1), przenoszenie okna między monitorami, wygląd zadokowanych paneli z zakładkami, obraz z projektora, cały macOS (czcionka, Retina, budowanie pod clang).

### 5.9 HUD gry: `drawHud`

HUD (head-up display) to informacja dla gracza rysowana na wierzchu sceny przez całą rundę: licznik kryształów, czas, pasek baterii, podpowiedź, a po wygranej karta `You escaped`. Co te liczby znaczą dla gry i skąd się biorą, opisuje [`game/gameplay.md`](game/gameplay.md), sekcja 6. Tutaj jest architektura: gdzie HUD mieszka, kiedy jest rysowany i dlaczego nie przeszkadza ani grze, ani panelom.

#### 5.9.1 Dlaczego HUD jest w `src/debug`, skoro należy do gry

HUD rysuję biblioteką ImGui, bo projekt nie ma innego sposobu na tekst na ekranie. ImGui zna tylko moduł `debug`: kod w `game/` nie może dołączać `imgui.h`, bo wtedy gra zależałaby od biblioteki paneli (sekcja 5.2). Dlatego funkcja stoi w [`Hud.hpp`](../../src/debug/Hud.hpp) i [`Hud.cpp`](../../src/debug/Hud.cpp), w przestrzeni nazw `debug`, a komentarz w nagłówku mówi to wprost: "It lives in src/debug only because this is where ImGui is: the game itself must not include ImGui".

```cpp
void drawHud(const game::Round& round, const game::GameplaySettings& settings);
```

Oba parametry są `const`: HUD niczego nie zmienia, tylko pokazuje. Z rundy czyta `state`, `collectedCount`, `requiredCount`, rozmiar listy `crystals`, `elapsedSeconds`, `battery` i `gateOpen`, a z ustawień jedno pole, `lowBatteryThreshold` (próg, poniżej którego pasek baterii zmienia kolor). Nagłówek deklaruje oba typy z wyprzedzeniem i nie dołącza niczego. `Hud.cpp` dołącza `debug/Theme.hpp` (kolory) i `game/Round.hpp`.

Skutek tego miejsca trzeba znać: HUD rysuje `DebugUI`, a `DebugUI` należy do `DebugNightMazeApp` w `main.cpp`. Program, który uruchomiłby samą klasę `game::NightMazeApp`, nie miałby HUD wcale. Dziś takiego programu nie ma.

#### 5.9.2 Kiedy jest rysowany: poza `if (m_visible)`

```cpp
        drawLightsPanel(context.lighting, context.round);
    }

    // The HUD belongs to the game and not to the tools, so it is drawn whether or not
    // the panels are visible.
    drawHud(context.round, context.gameplay);

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
```

Wywołanie stoi w `DebugUI::draw` po klamrze zamykającej blok paneli i przed `ImGui::Render()`. Wynikają z tego trzy rzeczy:

1. **Klawisz `~` (`GLFW_KEY_GRAVE_ACCENT`) chowa panele, a HUD zostaje.** `toggleVisible()` zmienia tylko `m_visible`, a `drawHud` tej zmiennej nie czyta. Komentarz przy `toggleVisible` w `DebugUI.hpp`: "The HUD of the game is not a panel: it stays". Nie ma klawisza, który chowa HUD.
2. **HUD jest wewnątrz klatki ImGui**, między `ImGui::NewFrame()` a `ImGui::Render()`, jak każdy widżet (pułapka 4). Dlatego klatka ImGui musi się wykonywać także przy schowanych panelach (sekcja 3.2).
3. **Kolejność wywołań nie decyduje o tym, co leży na wierzchu.** HUD jest wołany po panelach, a jego pasek mimo to leży pod nimi (sekcja 5.9.5).

#### 5.9.3 Dwa okna i ich flagi

`drawHud` rysuje dwa okna ImGui. Oba są tylko obrazkiem: nie mają paska tytułu, nie da się ich kliknąć, przesunąć ani zadokować.

```cpp
void drawHud(const game::Round& round, const game::GameplaySettings& settings) {
    const float scale = ImGui::GetStyle().FontScaleDpi;

    drawStatus(round, settings, scale);
    if (round.state == game::RoundState::Won) {
        drawWinCard(round, scale);
    }
}
```

| Okno | Funkcja | Nazwa w `Begin` | Kiedy | Flagi |
|---|---|---|---|---|
| pasek u góry | `drawStatus` | `"Game HUD"` | w każdej klatce, także po wygranej | `STATUS_WINDOW_FLAGS` |
| karta wygranej | `drawWinCard` | `"You escaped"` | tylko gdy `round.state == game::RoundState::Won` | `CARD_WINDOW_FLAGS` |

Nazwy z `Begin` nie widać na ekranie (okno nie ma paska tytułu), ale ImGui po niej rozróżnia okna, więc musi być inna niż tytuł każdego panelu.

```cpp
constexpr ImGuiWindowFlags PICTURE_WINDOW_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;

constexpr ImGuiWindowFlags STATUS_WINDOW_FLAGS =
    PICTURE_WINDOW_FLAGS | ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr ImGuiWindowFlags CARD_WINDOW_FLAGS = PICTURE_WINDOW_FLAGS;
```

Flagi okna to bity jednej liczby, łączone operatorem `|` (tak samo jak `ConfigFlags` w sekcji 5.6). Fragment jest pokazany bez komentarzy, które stoją w pliku. Co robi każda:

| Flaga | Co robi | Po co w HUD |
|---|---|---|
| `ImGuiWindowFlags_NoDecoration` | w `imgui.h` to cztery flagi naraz: bez paska tytułu, bez zmiany rozmiaru, bez paska przewijania i bez zwijania | okno ma wyglądać jak napis na ekranie, nie jak panel |
| `ImGuiWindowFlags_AlwaysAutoResize` | okno w każdej klatce ma dokładnie rozmiar swojej zawartości | pasek sam rośnie o linię, gdy pojawia się podpowiedź, i nie ma żadnej stałej z rozmiarem okna |
| `ImGuiWindowFlags_NoInputs` | w `imgui.h` to `NoMouseInputs`, `NoNavInputs` i `NoNavFocus` razem: mysz przechodzi przez okno, a nawigacja klawiaturą go nie widzi | kliknięcie w miejscu paska trafia do gry (sekcja 5.9.4) |
| `ImGuiWindowFlags_NoNav` | nawigacja klawiaturą ImGui omija okno. Obie jej części są już w `NoInputs`, flaga jest wpisana jawnie | to samo, powiedziane wprost |
| `ImGuiWindowFlags_NoFocusOnAppearing` | okno, które się pojawia, nie zabiera fokusu | karta wygranej nie odbiera fokusu panelowi, w którym właśnie przeciągam suwak |
| `ImGuiWindowFlags_NoSavedSettings` | nic o tym oknie nie trafia do `imgui.ini` | pozycję i tak ustawia kod w każdej klatce, a plik nie zbiera wpisów, które nic nie znaczą |
| `ImGuiWindowFlags_NoDocking` | okna nie da się zadokować w obszarze dokowania | HUD nie może stać się zakładką obok panelu |
| `ImGuiWindowFlags_NoMove` | okno stoi tam, gdzie postawił je kod | bez myszy i tak nie dałoby się go przeciągnąć, flaga mówi to jawnie |
| `ImGuiWindowFlags_NoBringToFrontOnFocus` (tylko pasek) | okno nigdy nie przechodzi na wierzch | pasek zostaje pod panelami (sekcja 5.9.5) |

Co stoi w obu oknach, linia po linii:

| Okno | Linia | Widżet i kolor |
|---|---|---|
| pasek | `Crystals`, `N / M`, `(of K)` i czas `m:ss` w jednej linii: zebrane, potrzebne, wszystkie | `TextColored` w `HUD_CRYSTAL_COLOR`, potem przez `SameLine` zwykły `Text` i dwa razy `TextDisabled` (kolor `MOON_DIM`) |
| pasek | pasek baterii i procent | `ProgressBar` o szerokości `BATTERY_BAR_WIDTH` z pustym napisem, obok `Text("%.0f%%", ...)` |
| pasek | `The gate is open. Find the exit.` | `TextColored` w `HUD_CRYSTAL_COLOR`, tylko gdy runda trwa i `round.gateOpen` |
| pasek | `Battery empty. Find a crystal.` | `TextColored` w `HUD_BATTERY_LOW_COLOR`, tylko gdy runda trwa i `round.battery <= 0.0F` |
| karta | `You escaped` | `TextColored` w `HUD_CRYSTAL_COLOR`, czcionką powiększoną `CARD_TITLE_SCALE` razy |
| karta | kreska, `Time: m:ss`, `Crystals: N of M` | `Separator` i dwa razy `Text` |
| karta | `R: play again` | `TextColored` w `HUD_BATTERY_COLOR` |

Dwie podpowiedzi to dwa osobne `if`, nie `if` i `else`, więc przy otwartej bramie i pustej baterii pokazują się obie, jedna pod drugą. Komentarze w kodzie mówią o "jednej linii podpowiedzi": opisują zwykły przypadek.

**Kolor paska baterii.** `ProgressBar` nie ma parametru z kolorem: wypełnienie bierze z tabeli stylu, z pozycji `ImGuiCol_PlotHistogram`. Dlatego `drawBatteryBar` podmienia tę pozycję na czas jednego widżetu:

```cpp
    const bool low = round.battery < settings.lowBatteryThreshold;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, low ? HUD_BATTERY_LOW_COLOR : HUD_BATTERY_COLOR);
    ImGui::ProgressBar(round.battery, {BATTERY_BAR_WIDTH * scale, 0.0F}, "");
    ImGui::PopStyleColor();
```

To ten sam mechanizm `PushStyleColor` i `PopStyleColor` co przy czerwonym tekście błędu (sekcja 5.8.1). Pierwszy argument `ProgressBar` to ułamek od 0 do 1, a `round.battery` jest właśnie taką liczbą. Wysokość 0 w rozmiarze znaczy "weź zwykłą wysokość widżetu z ramką", czyli czcionkę plus dwa razy `FramePadding.y` (tak liczy ją `ProgressBar` w `imgui_widgets.cpp`). Trzeci argument to napis rysowany na pasku: pusty, bo procent stoi obok, zwykłym tekstem. Gdyby stał na pasku, jasny tekst leżałby na jasnym bursztynie (pułapka 25).

**Tytuł karty większą czcionką.** `ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * CARD_TITLE_SCALE);` zmienia rozmiar tekstu do pasującego `ImGui::PopFont()`. `nullptr` znaczy "ta sama czcionka", a rozmiar podaję bez skali ekranu, bo ImGui 1.92 mnoży go przez `FontScaleDpi` samo (komentarz w kodzie). Nie trzeba wczytywać drugiej czcionki: znaki w nowym rozmiarze trafiają do tekstury przy pierwszym użyciu (sekcja 5.8.5).

#### 5.9.4 Brak wejścia: HUD nie zmienia `wantsMouse()` ani `wantsKeyboard()`

Gra oddaje mysz i klawiaturę panelom na podstawie dwóch pól ImGui (sekcja 5.6). HUD nie może na nie wpływać, bo wisi nad sceną przez cały czas: gdyby liczył się jako "okno pod kursorem", kliknięcie w górnej części sceny nie przechwytywałoby kursora.

- **Mysz.** `ImGuiWindowFlags_NoInputs` zawiera `ImGuiWindowFlags_NoMouseInputs`. Funkcja ImGui, która szuka okna pod kursorem (`FindHoveredWindowEx` w `imgui.cpp`), pomija okna z tą flagą. Kursor nad paskiem HUD jest więc dla ImGui kursorem nad tym, co leży pod paskiem: nad sceną, czyli nad przezroczystym środkiem obszaru dokowania, albo nad panelem. `WantCaptureMouse` zachowuje się tak, jakby HUD nie było.
- **Klawiatura.** `WantCaptureKeyboard` ustawia aktywny widżet. HUD ma same napisy, kreskę i pasek postępu: żadnego z nich nie da się uaktywnić, a przez `NoInputs` okno nie dostaje też fokusu nawigacji.

To wynika z kodu ImGui i z flag. Kliknięcia w miejscu paska HUD nikt jeszcze nie sprawdził ręcznie.

#### 5.9.5 Miejsce na ekranie i kolejność na wierzchu

**Miejsce.** Oba okna są ustawiane w każdej klatce, z warunkiem `ImGuiCond_Always` (panele mają `ImGuiCond_FirstUseEver`, sekcja 5.7), więc po zmianie rozmiaru okna programu HUD od razu wraca na środek:

```cpp
    const ImVec2 top = windowPoint(TOP_CENTER);
    // The rows of title bars are measured with the real height of a bar, which follows
    // the font (foldedRowsHeight).
    const float rowsAbove = foldedRowsHeight(FOLDED_ROW_COUNT, scale);
    ImGui::SetNextWindowPos({top.x, top.y + rowsAbove + HUD_TOP_OFFSET * scale}, ImGuiCond_Always,
                            TOP_CENTER);
    ImGui::SetNextWindowBgAlpha(HUD_OPACITY);
```

`windowPoint` zamienia część okna (x i y od 0 do 1) na punkt głównego viewportu, tak samo jak `placePanelOnFirstUse` liczy róg okna. Trzeci argument `SetNextWindowPos` to znany z sekcji 5.7 pivot: `TOP_CENTER`, czyli `(0.5, 0)`, każe postawić w podanym punkcie środek górnej krawędzi okna HUD. Dzięki temu pasek jest wyśrodkowany bez znajomości swojej szerokości, którą ImGui ustala samo (`AlwaysAutoResize`). Karta robi to samo z punktem `CENTER`, czyli `(0.5, 0.5)`, i pivotem `CENTER`: jej środek stoi w środku okna.

| Stała w `Hud.cpp` | Wartość | Znaczenie |
|---|---|---|
| `HUD_TOP_OFFSET` | `2.0F * PANEL_GAP`, czyli 16 | od M6 to już nie cała odległość od górnej krawędzi okna, tylko jej stała część: wolne miejsce nad pierwszym rzędem pasków (jeden `PANEL_GAP`) plus dodatkowy odstęp między ostatnim rzędem a HUD (drugi `PANEL_GAP`), dzięki któremu HUD czyta się jako osobna rzecz. Resztę, czyli same rzędy pasków, liczy `foldedRowsHeight` (niżej). Do M5 stała wynosiła `46.0F` i była całą odległością |
| `BATTERY_BAR_WIDTH` | `230.0F` | szerokość paska baterii. Od niej zależy szerokość całego okna paska |
| `HUD_OPACITY`, `CARD_OPACITY` | `0.72F`, `0.9F` | alfa tła obu okien, podawana do `SetNextWindowBgAlpha`. Karta zasłania więcej, bo ma być czytana, a runda za nią jest skończona. Skutek dla kontrastu: sekcja 5.8.2 |
| `CARD_TITLE_SCALE` | `1.8F` | ile razy tytuł karty jest większy od zwykłego tekstu |
| `CARD_PADDING` | `{28.0F, 20.0F}` | odstęp tekstu karty od jej krawędzi, większy niż w panelu. Ustawiany przez `PushStyleVar(ImGuiStyleVar_WindowPadding, ...)` przed `Begin`, bo `Begin` czyta tę metrykę |
| `TIME_TEXT_SIZE` | `16` | rozmiar tablicy znaków na czas zapisany jako `m:ss`. `std::snprintf` nigdy nie pisze poza podany rozmiar |

Stałe są w pikselach przy skali ekranu 100% i `drawHud` mnoży je przez `ImGui::GetStyle().FontScaleDpi`, czyli przez skalę ekranu zapisaną w stylu przez `applyTheme`. To inna liczba niż `layoutScale` paneli (sekcja 5.7), która bywa mniejsza od skali ekranu. Wyjątkiem jest rozmiar czcionki tytułu: tę jedną wartość ImGui skaluje samo.

**Skąd HUD wie, ile rzędów pasków jest nad nim (od M6).** Do M5 odległość HUD od górnej krawędzi była jedną liczbą, 46, dobraną ręcznie do jednego rzędu pasków: 8 odstępu, 22 wysokości paska i 16 zapasu. Stała i wysokość paska nie były w kodzie powiązane, więc po zmianie czcionki trzeba było poprawiać 46 ręcznie. Drugi rząd pasków (Terrain i Grass) wymusił zmianę, i zrobiłem ją tak, żeby ręczna liczba zniknęła:

```cpp
// The HUD stands below the rows of title bars of the panels that start folded at the top
// edge (Camera and Gameplay, Terrain and Grass, Framebuffers, Shadows). This is the free
// space above the first row plus the extra space between the last row and the HUD, which
// makes the HUD read as a thing of its own: two panel gaps.
constexpr float HUD_TOP_OFFSET = 2.0F * PANEL_GAP;
```

Nawias w tym komentarzu wymienia dziś wszystkie cztery rzędy (do czwartej części M7 kończył się na Terrain i Grass, czyli na stanie z M6). Kod nie liczy rzędów z komentarza, tylko ze stałej `FOLDED_ROW_COUNT`.

| Składnik | Wzór | Przy skali 100%, czcionce 16 i `FramePadding.y` 3 |
|---|---|---|
| rzędy pasków | `foldedRowsHeight(FOLDED_ROW_COUNT, scale)` = 4 * (`GetFrameHeight()` + 8 * `scale`) | 4 * (22 + 8) = 120 (w M6, przy dwóch rzędach, 60, w pierwszych trzech częściach M7, przy trzech, 90) |
| stała część | `HUD_TOP_OFFSET * scale` = 2 * 8 * `scale` | 16 |
| góra paska HUD | suma, liczona od `top.y` | 136 (w pierwszych trzech częściach M7: 106, w M6: 76, do M5: 46) |

Rachunek po kolei: 8 wolnego miejsca, pasek pierwszego rzędu (do y 30), 8 odstępu, pasek drugiego rzędu (do y 60), 8 odstępu, pasek trzeciego rzędu (do y 90), 8 odstępu, pasek czwartego rzędu (do y 120), 8 odstępu i jeszcze 8 dodatkowego, razem 136. Wzór grupuje to inaczej (każdy rząd z odstępem **za** sobą, a na końcu dwa odstępy), ale suma jest ta sama. Do M5 było 46, w M6 76, od pierwszej części M7 106, od czwartej 136: każdy nowy rząd zsuwa HUD o 30 jednostek, dokładnie o 22 + 8. Dla M6 wykonawca zgłosił przesunięcie "około 30 pikseli" przy skali 100%. Dla M7 przesunięcia nikt nie mierzył: wynika ze wzoru. Tej trzydziestki nie ma w kodzie jako liczby i nie może być: wysokość paska pochodzi z `ImGui::GetFrameHeight()`, więc idzie za czcionką. `Hud.cpp` dołącza w tym celu `debug/PanelLayout.hpp`, z którego bierze `PANEL_GAP`, `FOLDED_ROW_COUNT` i `foldedRowsHeight`.

Dwie rzeczy, które trzeba umieć powiedzieć. Po pierwsze, HUD jest odsuwany o **wszystkie cztery** rzędy zawsze, także gdy panele są schowane klawiszem `~` i żadnego paska nie widać: `FOLDED_ROW_COUNT` to stała, a nie liczba pasków na ekranie. HUD nie skacze więc przy chowaniu paneli, ale przy schowanych panelach wisi niżej, niż by musiał. Po drugie, powiązanie jest jednostronne: kto doda kolejny rząd, musi sam podnieść `FOLDED_ROW_COUNT`, bo nic nie liczy rzędów ze stałych `..._PLACEMENT` (pułapka 38). Tak było w pierwszej części M7: panel Framebuffers dostał `foldedRowsBefore = 2`, a stała została podniesiona z 2 do 3 w tej samej zmianie. W czwartej części M7 tak samo: panel Shadows dostał `foldedRowsBefore = 3`, a stała wzrosła z 3 do 4.

**Co leży na wierzchu.** ImGui trzyma okna na liście uporządkowanej od tyłu do przodu. O miejscu na tej liście decydują trzy reguły (sprawdzone w źródle naszej wersji, `imgui.cpp`):

1. Nowe okno trafia na **przód** listy, a okno z flagą `NoBringToFrontOnFocus` na jej **tył** (funkcja `CreateNewWindow`).
2. Okno, które dostaje fokus (na przykład kliknięty panel), przechodzi na przód, chyba że ma flagę `NoBringToFrontOnFocus` (funkcja `FocusWindow`).
3. Poza tym kolejność się nie zmienia. W szczególności nie zależy od kolejności wywołań `Begin` w klatce.

| Okno | Gdzie powstaje | Co się dzieje potem | Skutek |
|---|---|---|---|
| pasek HUD | na tyle listy, w pierwszej klatce programu | nigdy nie przechodzi na przód | każdy panel, który na niego najdzie, go zasłania. Tak ma być: komentarz w kodzie mówi, że panel rozwinięty nad paskiem jest właśnie używany i pasek nie może zasłaniać jego widżetów |
| karta wygranej | na przodzie listy, w klatce pierwszej wygranej od startu programu | nie da się jej kliknąć (`NoInputs`), więc sama na przód już nie wróci. Panel kliknięty później przechodzi przed nią | przy pierwszej wygranej karta leży na wszystkich panelach. Panel kliknięty potem zasłania ją tam, gdzie na nią nachodzi |

Na zrzutach ekranu wykonawcy (Windows, 2026-10-05) widać oba skutki naraz: przy rozwiniętych panelach Camera i Gameplay pasek HUD jest prawie cały pod nimi (wystaje tylko w odstępie między panelami), a karta `You escaped` leży na obu.

**Znane ograniczenia** (oba wynikają z flag wyżej, żadne nie jest błędem w rachunku układu):

- **Pasek HUD znika pod rozwiniętym panelem.** Rozwinięte panele Camera i Gameplay zajmują razem całą szerokość między kolumnami (x od 352 do 632 i od 640 do 964, y od 8 do 424), a pasek wisi na środku, od y 76. Kto gra z rozwiniętym panelem Gameplay, nie widzi baterii na HUD. Stan rundy widać wtedy w samym panelu (`Round`, `Crystals`, `Gate`, suwak `Battery`), a klawisz `~` chowa panele i odsłania pasek.
- **Karta wygranej może zostać zasłonięta.** Panel kliknięty po pojawieniu się karty przechodzi przed nią. Z reguł wyżej wynika też coś, czego nikt nie oglądał na ekranie: okno karty powstaje raz, przy pierwszej wygranej, i zachowuje swoje miejsce na liście. Jeśli po pierwszej wygranej kliknę jakiś panel (choćby przycisk `Restart round (key R)` w panelu Gameplay), to przy następnej wygranej w tym samym uruchomieniu karta pojawi się **pod** tym panelem, o ile na siebie nachodzą. W układzie domyślnym karta stoi na środku okna, nad sceną, więc nachodzi na rozwinięte panele Camera i Gameplay. Na zrzucie jej dolna krawędź wypada tuż nad dolnym rzędem paneli. Lekarstwo jest to samo: klawisz `~` albo zwinięcie panelu. Klawisz R działa niezależnie od tego, czy kartę widać.

### 5.10 Panele Terrain i Grass linia po linii (M6)

Dwa panele z drugiej części M6 są krótkie i zbudowane tak samo, więc opisuję je razem. Co ich kontrolki znaczą dla terenu i dla trawy (wzór wysokości, `glPolygonMode`, shader geometrii), tłumaczą [`renderer/terrain.md`](renderer/terrain.md) i [`renderer/grass-geometry.md`](renderer/grass-geometry.md). Tutaj jest sam kod paneli i droga danych.

**Nagłówki.** [`TerrainPanel.hpp`](../../src/debug/panels/TerrainPanel.hpp) i [`GrassPanel.hpp`](../../src/debug/panels/GrassPanel.hpp) deklarują po jednej funkcji:

```cpp
void drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain);
void drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount);
```

Sygnatury czyta się jak w sekcji 5.2: ustawienia bez `const` są edytowalne, teren z `const` jest tylko do odczytu, a liczba kępek przychodzi przez wartość. `DebugUI::draw` woła je tak:

```cpp
        drawTerrainPanel(context.terrain, context.mazeWorld.terrain);
        drawGrassPanel(context.grass, context.grassTuftCount);
```

Panel Terrain dostaje więc teren **z labiryntu w grze** (`MazeWorld::terrain`), a nie osobne pole kontekstu: po każdej przebudowie pokazuje liczby nowego terenu bez żadnej dodatkowej linii kodu.

**`TerrainPanel.cpp`:**

```cpp
namespace {

// The smallest height scale of the slider: a flat world. The largest one is
// game::MAX_HEIGHT_SCALE.
constexpr float MIN_HEIGHT_SCALE = 0.0F;

} // namespace

void drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain) {
    // First run only: the second row of title bars at the top edge of the window, under
    // the Camera panel and folded like it (the constant is in PanelLayout.hpp). Later
    // ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(TERRAIN_PLACEMENT);
    if (ImGui::Begin("Terrain")) {
        // SliderFloat returns true in every frame in which the value changed, so the
        // terrain follows the slider while it is dragged. The panel only asks: the game
        // builds the terrain at the start of its next frame.
        if (ImGui::SliderFloat("Height scale", &settings.heightScale, MIN_HEIGHT_SCALE,
                               game::MAX_HEIGHT_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
            settings.rebuild = true;
        }
        ImGui::SetItemTooltip("Every height of the terrain is multiplied by this number.\n"
                              "0 is a flat world. The walls, the gate, the crystals and\n"
                              "the player are put on the new ground at once.");

        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Wireframe", &settings.wireframe);
        ImGui::SetItemTooltip("Draws the edges of the triangles of the terrain instead of\n"
                              "their faces (glPolygonMode). Everything else stays filled.");

        ImGui::Separator();
        ImGui::Text("Grid: %d x %d points, %.2f m apart", terrain.columns(), terrain.rows(),
                    terrain.spacing());
        ImGui::Text("Triangles: %d", static_cast<int>(terrain.triangleCount()));
        ImGui::Text("Height: %.2f m to %.2f m", terrain.minHeight(), terrain.maxHeight());
    }
    ImGui::End();
}
```

| Fragment | Co robi |
|---|---|
| `MIN_HEIGHT_SCALE = 0.0F` | dolna granica suwaka, nazwana w pliku panelu. Górna to `game::MAX_HEIGHT_SCALE` z `Terrain.hpp`, czyli 2,5: ta jest regułą gry (chroni kolizje, [`renderer/terrain.md`](renderer/terrain.md)), więc panel jej nie powtarza, tylko ją czyta. Tę samą dolną granicę ma osobna stała w `NightMazeApp.cpp`, bo gra przycina wartość jeszcze raz |
| `placePanelOnFirstUse(TERRAIN_PLACEMENT)` | pierwszy start: drugi rząd pasków, pod panelem Camera, zwinięty (sekcja 5.7) |
| `if (ImGui::Begin("Terrain"))` | dla zwiniętego panelu `Begin` zwraca `false` i cała zawartość jest pomijana. `ImGui::End()` stoi poza `if`, jak w każdym panelu |
| `ImGui::SliderFloat("Height scale", &settings.heightScale, 0, 2.5, "%.2f", ImGuiSliderFlags_AlwaysClamp)` | suwak pisze przez wskaźnik do pola `TerrainSettings::heightScale` i zwraca `true` w każdej klatce, w której wartość się zmieniła. `"%.2f"` pokazuje dwie cyfry po kropce. `AlwaysClamp` trzyma w zakresie także wartość wpisaną z klawiatury (Ctrl i kliknięcie) |
| `settings.rebuild = true;` | panel **nie przebudowuje** terenu. Ustawia flagę prośby, a gra robi to na początku swojej następnej klatki (niżej). Suwak zwraca `true` w każdej klatce przeciągania, więc teren idzie za suwakiem na żywo: jedna przebudowa na klatkę |
| `ImGui::SetItemTooltip(...)` | podpowiedź po najechaniu na poprzedni widżet. Dwa sąsiednie literały napisów kompilator skleja w jeden, a `\n` łamie linię w dymku |
| `ImGui::Checkbox("Wireframe", &settings.wireframe)` | pole `TerrainSettings::wireframe`. Tu flagi prośby nie ma: gra czyta to pole w każdej klatce przy rysowaniu terenu (`m_terrainRenderer.draw(shader, m_terrainSettings.wireframe)`), więc niczego nie trzeba budować od nowa |
| trzy linie `ImGui::Text` | odczyt z terenu w grze: `columns()` na `rows()` punktów siatki i odstęp `spacing()`, liczba trójkątów (`triangleCount()` zwraca `std::size_t`, a format `%d` chce `int`, stąd `static_cast`) oraz najniższy i najwyższy punkt siatki |

Dla labiryntu startowego (10 x 10, skala 1) panel pokazuje `Grid: 97 x 97 points, 0.50 m apart`, `Triangles: 18432` i `Height: 0.00 m to 3.37 m`. Pierwsze dwie linie wynikają z wzorów w `Terrain.cpp` ((10 + 2 * 7) * 4 + 1 = 97 i 96 * 96 * 2 = 18432), trzecią przeliczyłem z pliku `heightmap.png` skryptem, który powtarza wzór wysokości. Samego panelu z tymi liczbami nikt nie oglądał rozwiniętego. Uwaga do trzeciej linii: to zakres **całej siatki**, razem ze wzgórzami poza labiryntem. Pod samym labiryntem grunt ma od 0,085 do 0,461 m.

**`GrassPanel.cpp`:**

```cpp
namespace {

// The density, in tufts per metre of wall and side. 0 plants nothing. The largest one is
// game::MAX_GRASS_DENSITY.
constexpr float MIN_DENSITY = 0.0F;

// The height of the tallest blades, in metres: from stubble to knee high.
constexpr float MIN_BLADE_HEIGHT = 0.05F;
constexpr float MAX_BLADE_HEIGHT = 0.8F;

// The strength of the wind: 0 is still air, 1 the default breeze.
constexpr float MIN_WIND_STRENGTH = 0.0F;
constexpr float MAX_WIND_STRENGTH = 3.0F;

// The blades one tuft is made of: BLADE_COUNT in assets/shaders/grass.geom. The panel
// only uses it to show a number, so a wrong value here changes nothing that is drawn.
constexpr int BLADES_PER_TUFT = 3;

} // namespace

void drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount) {
    // First run only: the second row of title bars at the top edge of the window, under
    // the Gameplay panel and folded like it (the constant is in PanelLayout.hpp). Later
    // ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(GRASS_PLACEMENT);
    if (ImGui::Begin("Grass")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Enabled", &settings.enabled);

        // SliderFloat returns true in every frame in which the value changed. The panel
        // only asks: the game places the tufts at the start of its next frame.
        if (ImGui::SliderFloat("Density", &settings.density, MIN_DENSITY, game::MAX_GRASS_DENSITY,
                               "%.1f per m", ImGuiSliderFlags_AlwaysClamp)) {
            settings.replant = true;
        }
        ImGui::SetItemTooltip("Tufts per metre of wall, on each side of the wall. The\n"
                              "scatter on the hills follows in proportion.");

        // These two are uniforms of the grass program: they change the blades the
        // geometry shader builds, and no tuft has to be placed again.
        ImGui::SliderFloat("Blade height", &settings.bladeHeight, MIN_BLADE_HEIGHT,
                           MAX_BLADE_HEIGHT, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Wind strength", &settings.windStrength, MIN_WIND_STRENGTH,
                           MAX_WIND_STRENGTH, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();
        // One tuft is one point in the vertex buffer. The blades are made of it by the
        // geometry shader, so their number is not stored anywhere.
        const int tufts = static_cast<int>(tuftCount);
        ImGui::Text("Tufts: %d (%d blades)", tufts, tufts * BLADES_PER_TUFT);
    }
    ImGui::End();
}
```

| Fragment | Co robi |
|---|---|
| stałe zakresów | `Density` od 0 do `game::MAX_GRASS_DENSITY` (8), `Blade height` od 0,05 do 0,8 m, `Wind strength` od 0 do 3. Górna granica gęstości jest regułą gry i leży w `Grass.hpp`, pozostałe granice są sprawą panelu |
| `BLADES_PER_TUFT = 3` | **powtórzenie** stałej `BLADE_COUNT` z pliku `assets/shaders/grass.geom`. Kod C++ nie ma jak przeczytać stałej z pliku GLSL, więc liczba jest wpisana drugi raz, a komentarz mówi wprost, czym to grozi: po zmianie `BLADE_COUNT` w shaderze panel pokaże złą liczbę źdźbeł, ale obraz się nie zmieni, bo panel używa jej tylko do napisu (pułapka 40) |
| `placePanelOnFirstUse(GRASS_PLACEMENT)` | pierwszy start: drugi rząd pasków, pod panelem Gameplay, zwinięty |
| `ImGui::Checkbox("Enabled", &settings.enabled)` | pole `GrassSettings::enabled`. Gra czyta je w każdej klatce: `drawGrass` wraca od razu, gdy jest fałszem. Kępki zostają na karcie graficznej, nie są tylko rysowane |
| `ImGui::SliderFloat("Density", ..., "%.1f per m", ...)` | napis formatu może zawierać tekst: suwak pokazuje na przykład `2.5 per m`. Zmiana ustawia `settings.replant`, flagę prośby taką jak `rebuild` wyżej. Gęstość zmienia **dane** (liczbę i miejsca punktów w buforze wierzchołków), więc wymaga pracy po stronie gry |
| suwaki `Blade height` i `Wind strength` | bez `if` i bez flagi. To uniformy programu trawy (`uBladeHeight`, `uWindStrength`): `GrassRenderer::draw` wysyła je w każdej klatce, a źdźbła i tak powstają od nowa w każdej klatce w shaderze geometrii. Żadnej kępki nie trzeba sadzić od nowa |
| `const int tufts = static_cast<int>(tuftCount);` | `%d` chce `int`, a liczba kępek to `std::size_t` |
| `ImGui::Text("Tufts: %d (%d blades)", ...)` | liczba kępek i liczba źdźbeł. Ta druga jest tylko iloczynem: źdźbeł nie ma w żadnym buforze. Przy ustawieniach startowych wychodzi `Tufts: 1843 (5529 blades)`: 1843 zgłosił wykonawca i ta sama liczba wychodzi z mojej symulacji generatora, 5529 to 1843 * 3 |

**Wzorzec flagi prośby.** Oba panele mają po jednej kontrolce, która wymaga pracy poza panelem. Robią to samo, co panel Maze robi od M2 + M3 z polem `MazeSettings::regenerate` i panel Gameplay z `GameplaySettings::restart`: ustawiają flagę w strukturze ustawień, a gra obsługuje ją na początku następnej klatki, w `NightMazeApp::onRender`:

```cpp
    if (m_terrainSettings.rebuild) {
        m_terrainSettings.rebuild = false;
        rebuildTerrain();
    }
    if (m_grassSettings.replant) {
        m_grassSettings.replant = false;
        plantGrass();
    }
```

| Kontrolka | Flaga | Co robi gra | Dlaczego nie panel |
|---|---|---|---|
| `Height scale` | `TerrainSettings::rebuild` | `rebuildTerrain()`: nowy teren, ściany, słupki i brama zatopione od nowa, kryształy rundy przestawione, lista przeszkód zbudowana od nowa, idący gracz postawiony na nowym gruncie, siatka terenu i trawa wysłane na kartę | panel zna tylko `TerrainSettings` i `const Terrain&`. Nie ma dostępu ani do `MazeWorld`, ani do rundy, ani do klas rysujących, i nie powinien mieć |
| `Density` | `GrassSettings::replant` | `plantGrass()`: `placeGrass` wybiera miejsca od nowa, `GrassRenderer::upload` wymienia bufor punktów | panel nie zna `GrassRenderer` ani świata |

Powody są trzy. Po pierwsze, kierunek zależności: panel edytuje dane, a gra decyduje, co z nich wynika. Po drugie, moment: panele są rysowane **po** scenie tej klatki (sekcja 5.4), więc przebudowa w środku panelu zmieniałaby świat, z którego dalsze panele tej samej klatki jeszcze czytają. Po trzecie, jedno miejsce: nowy labirynt, nowa skala wysokości i nowa gęstość trawy są obsługiwane obok siebie na początku `onRender`, w tej kolejności. Labirynt przebudowany w tej klatce jest już zbudowany z nowymi liczbami, a mimo to ustawione flagi `rebuild` i `replant` wykonują się potem jeszcze raz: komentarz w kodzie mówi, że to kosztuje trochę czasu raz i niczego nie zmienia.

Gra nie ufa panelowi do końca: `rebuildTerrain` i `regenerateMaze` przycinają `heightScale` do zakresu od 0 do `MAX_HEIGHT_SCALE`, a `plantGrass` przycina `density` do zakresu od 0 do `MAX_GRASS_DENSITY` (`std::clamp`). Przy fladze `AlwaysClamp` suwak i tak nie wypuści wartości spoza zakresu, więc to drugie zabezpieczenie: reguła gry jest pilnowana w grze, a nie tylko w widżecie.

**Opóźnienie o jedną klatkę.** Suwak zmienia `heightScale` w klatce N (w trakcie `DebugUI::draw`, po narysowaniu sceny), a teren jest przebudowywany na początku klatki N+1. Między tymi chwilami trzecia linia panelu (`Height: ...`) pokazuje jeszcze stary teren, bo czyta `mazeWorld.terrain`, a suwak pokazuje już nową skalę. Trwa to jedną klatkę i nie jest błędem.

**Trzy komentarze w starszych panelach.** Druga część M6 zmieniła w panelach Camera, Lights i Maze tylko komentarze, bo podłogi z płytek już nie ma. W `CameraPanel.cpp` przy polu `Player feet`: gra trzyma `y` stóp "on the ground" (wcześniej "at the floor"), więc zmiana `y` w panelu utrzymuje się tylko w trybie noclip: w następnym kroku chodzenia `Player::update` nadpisuje ją wysokością terenu. W `LightsPanel.cpp` przy zakresie `Moon pitch`: światło bliskie 0 stopni "only grazes the ground". W `MazePanel.cpp` przy granicach suwaków rozmiaru: gra rysuje każdą ścianę i każdy słupek osobnym wywołaniem, około dwóch na komórkę (do M5 około trzech, bo dochodziła płytka podłogi), a teren ma 32 trójkąty na komórkę (4 x 4 kwadraty siatki po dwa trójkąty). Zakres suwaków, od 2 do 40 komórek, się nie zmienił.

**Stan sprawdzenia.** Kod obu paneli jest przeczytany i zgodny z tym opisem. Żadnego z nich nikt nie rozwinął ręcznie i żadnej kontrolki nikt nie kliknął: skutki (płaski świat przy skali 0, siatka z krawędzi, trawa wyłączona) wykonawca oglądał na zrzutach ekranu, na których stan ustawiały tymczasowe wstawki w kodzie, usunięte po zrzutach. Lista do ręcznego przejścia jest w [`../guides/build-windows.md`](../guides/build-windows.md).

### 5.11 `RawTextureSampler`: podgląd tekstury sRGB taki jak plik (M7)

Pliki: [`src/debug/RawTextureSampler.hpp`](../../src/debug/RawTextureSampler.hpp), [`src/debug/RawTextureSampler.cpp`](../../src/debug/RawTextureSampler.cpp). Komentarz `// See docs/...` na górze obu wskazuje ten dokument. Użytkownik jest jeden: lista `Textures` panelu Assets.

#### 5.11.1 Problem

Do M6 panel Assets pokazywał teksturę tak: `ImGui::Image` dostawało identyfikator tekstury, backend ImGui czytał ją swoim shaderem i wpisywał odczytane liczby prosto do okna. Bajty pliku trafiały na ekran bez zmiany, więc podgląd wyglądał jak plik.

Od pierwszej części M7 tekstury koloru są teksturami **sRGB** (`GL_SRGB8`, [`gfx/color-space.md`](gfx/color-space.md)). Karta przy każdym odczycie takiej tekstury zamienia jej bajty na wartości liniowe. Scena to potem odwraca: ostatni przebieg klatki koduje obraz z powrotem do sRGB. ImGui tego nie robi: rysuje po przebiegu składającym, prosto do okna, i wpisuje to, co odczytało. Podgląd dostałby więc wartości liniowe pokazane tak, jakby były zakodowane, czyli **za ciemne**. Przykład: bajt 128 w pliku (połowa skali) to po zdekodowaniu około 0,216, a 0,216 zapisane do okna to bajt 55.

Są trzy miejsca, w których dałoby się to naprawić: w shaderze ImGui (nie mój kod), w samej teksturze (druga kopia bez sRGB, dwa razy więcej pamięci) albo w **sposobie odczytu**. Kod wybiera trzecie: na czas jednego obrazu podmienia obiekt samplera na taki, który dekodowanie pomija.

#### 5.11.2 Rozszerzenie `GL_EXT_texture_sRGB_decode`

Wyłączenie dekodowania sRGB przy odczycie nie należy do rdzenia OpenGL 4.1. Daje je rozszerzenie `GL_EXT_texture_sRGB_decode`: parametr `GL_TEXTURE_SRGB_DECODE_EXT` tekstury albo samplera, z wartościami `GL_DECODE_EXT` (domyślna) i `GL_SKIP_DECODE_EXT`. Z drugą wartością tekstura sRGB jest czytana tak, jakby miała zwykły format `GL_RGB8`: bajty wychodzą bez zmiany. Dla tekstur, które nie są sRGB, parametr nie ma żadnego skutku.

```cpp
// The extension and its two constants. The GLAD loader of this project was generated
// without extensions, so its header does not declare them. They are plain numbers from
// the extension specification and are passed to a core function (glSamplerParameteri).
// They may be used only after the extension was found at runtime.
constexpr const char* SRGB_DECODE_EXTENSION = "GL_EXT_texture_sRGB_decode";
constexpr GLenum TEXTURE_SRGB_DECODE = 0x8A48; // GL_TEXTURE_SRGB_DECODE_EXT
constexpr GLint SKIP_DECODE = 0x8A4A;          // GL_SKIP_DECODE_EXT

// The texture unit the OpenGL backend of ImGui draws its pictures with.
constexpr GLuint IMGUI_TEXTURE_UNIT = 0;
```

| Linia | Znaczenie |
|---|---|
| `SRGB_DECODE_EXTENSION` | nazwa, o którą pytam sterownik funkcją `gfx::hasExtension` ([`gfx/textures.md`](gfx/textures.md)) |
| `TEXTURE_SRGB_DECODE = 0x8A48`, `SKIP_DECODE = 0x8A4A` | dwie stałe rozszerzenia wpisane ręcznie. GLAD tego projektu jest wygenerowany dla 4.1 Core **bez rozszerzeń** ([`../libraries/glad.md`](../libraries/glad.md)), więc w `<glad/gl.h>` ich nie ma. To zwykłe liczby ze specyfikacji rozszerzenia. Nie potrzebują żadnej nowej funkcji: idą do `glSamplerParameteri`, które jest w rdzeniu od 3.3. Ten sam zabieg stosuje `Texture2D` dla anizotropii |
| "only after the extension was found at runtime" | sterownik, który rozszerzenia nie zna, na nieznaną stałą odpowiada błędem `GL_INVALID_ENUM`. Dlatego stałe są używane tylko po udanym `hasExtension` |
| `IMGUI_TEXTURE_UNIT = 0` | backend ImGui rysuje swoje obrazy przez jednostkę teksturującą 0, więc na niej trzeba podmienić sampler |

#### 5.11.3 Nagłówek

```cpp
class RawTextureSampler {
public:
    /// Asks the driver for the extension and creates the sampler object when it is
    /// there.
    RawTextureSampler();
    ~RawTextureSampler();

    RawTextureSampler(const RawTextureSampler&) = delete;
    RawTextureSampler& operator=(const RawTextureSampler&) = delete;

    /// True when the driver offers the extension, so begin and end have an effect.
    bool isSupported() const { return m_sampler != 0; }

    /// From here on the pictures added to the current ImGui window are read without
    /// sRGB decoding. Call it right before ImGui::Image, inside a window.
    void begin() const;

    /// Back to the sampler ImGui draws everything else with. Call it right after the
    /// picture.
    void end() const;

private:
    // Name (id) of the sampler object. 0 means "the extension is missing".
    GLuint m_sampler = 0;
};
```

| Element | Dlaczego tak |
|---|---|
| konstruktor i destruktor | RAII jak w klasach `gfx`: obiekt posiada jeden obiekt samplera OpenGL. Klasa leży w `debug/`, a nie w `gfx/`, bo jej jedynym zadaniem jest współpraca z listą rysowania ImGui |
| `= delete` dla kopiowania | kopia trzymałaby ten sam identyfikator i dwa destruktory usuwałyby jeden sampler. Przenoszenia też nie ma: obiekt jest polem `DebugUI` i nigdzie nie wędruje |
| `isSupported()` | `m_sampler` równe 0 znaczy "rozszerzenia nie ma". Jedno pole niesie obie informacje |
| `begin()` i `end()` są `const` | nie zmieniają obiektu, tylko dopisują polecenia do listy rysowania ImGui |
| długi komentarz nad klasą (tu pominięty) | mówi to samo co sekcja 5.11.1 i dodaje jedno zdanie o czasie życia: obiekt musi zginąć przed oknem |

Nagłówek dołącza `<glad/gl.h>` (dla `GLuint`) i nic z ImGui.

#### 5.11.4 Konstruktor i destruktor

```cpp
RawTextureSampler::RawTextureSampler() {
    if (!gfx::hasExtension(SRGB_DECODE_EXTENSION)) {
        return;
    }
    GL_CHECK(glGenSamplers(1, &m_sampler));
    // The same reading as the sampler ImGui uses for its pictures (linear filter,
    // clamped to the edge), with one difference: no sRGB decoding. The setting is
    // ignored for textures that are not sRGB.
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, TEXTURE_SRGB_DECODE, SKIP_DECODE));
}

RawTextureSampler::~RawTextureSampler() {
    // OpenGL silently ignores the id 0.
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
}
```

| Linia | Znaczenie |
|---|---|
| `if (!gfx::hasExtension(...)) { return; }` | bez rozszerzenia konstruktor kończy się od razu. `m_sampler` zostaje zerem, `isSupported()` zwraca fałsz, a `begin` i `end` nic nie robią. Program działa dalej, tylko podglądy są ciemniejsze niż pliki. Żadnego komunikatu w logu kod przy tym nie wypisuje |
| `glGenSamplers` | obiekt samplera: zestaw parametrów odczytu oddzielony od tekstury ([`gfx/textures.md`](gfx/textures.md), sekcja 2.8). Związany z jednostką zastępuje parametry każdej tekstury czytanej przez tę jednostkę |
| cztery linie filtra i zawijania | to samo, co ma sampler backendu ImGui (filtr liniowy, `GL_CLAMP_TO_EDGE`), żeby podgląd różnił się od dotychczasowego **tylko** dekodowaniem |
| `glSamplerParameteri(m_sampler, TEXTURE_SRGB_DECODE, SKIP_DECODE)` | jedyna linia, dla której klasa istnieje: przy odczycie przez ten sampler karta nie dekoduje sRGB |
| destruktor | `glDeleteSamplers` z identyfikatorem 0 jest po cichu ignorowane, więc obiekt bez samplera nie wymaga osobnego przypadku |

Mipmap sampler nie używa (`GL_LINEAR` jako filtr pomniejszenia), choć tekstury je mają: podgląd 128 x 128 z tekstury 512 albo 1024 jest więc czytany z poziomu 0. Tak samo czytał go wcześniej sampler ImGui.

#### 5.11.5 `begin`, `end` i wywołanie zwrotne w liście rysowania

Tu jest jedyna trudna rzecz tej klasy. ImGui **nie rysuje** w chwili, gdy panel woła `ImGui::Image`. Zbiera polecenia rysowania w listach i wykonuje je dopiero w `ImGui_ImplOpenGL3_RenderDrawData`, na końcu klatki (sekcja 3.2). Zawołanie `glBindSampler` wprost w kodzie panelu nic by nie dało: do czasu prawdziwego rysowania backend ustawiłby własny stan. Trzeba więc wstawić **własne polecenie do listy**, dokładnie przed obrazem. Do tego służy `ImDrawList::AddCallback`.

```cpp
// Runs in the middle of the drawing of ImGui, at the place in its list of draw commands
// where begin() added it. The data of the command is the copy of the sampler id that
// begin() handed over.
void bindSampler(const ImDrawList* /*drawList*/, const ImDrawCmd* command) {
    const GLuint sampler = *static_cast<const GLuint*>(command->UserCallbackData);
    GL_CHECK(glBindSampler(IMGUI_TEXTURE_UNIT, sampler));
}
```

```cpp
void RawTextureSampler::begin() const {
    if (!isSupported()) {
        return;
    }
    // ImGui does not draw while the panels are built: it collects draw commands and
    // runs them at the end of the frame. A callback is a command of our own in that
    // list. With a size as the third argument ImGui copies the bytes it is given, so
    // the callback does not depend on this object or on the local variable.
    GLuint sampler = m_sampler;
    ImGui::GetWindowDrawList()->AddCallback(bindSampler, &sampler, sizeof(sampler));
}

void RawTextureSampler::end() const {
    if (!isSupported()) {
        return;
    }
    // The backend offers a callback that binds its own linear sampler again.
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear,
                                            nullptr);
}
```

| Linia | Znaczenie |
|---|---|
| `bindSampler(const ImDrawList*, const ImDrawCmd* command)` | sygnatura, jakiej ImGui wymaga od wywołania zwrotnego. Pierwszy parametr nie jest używany, stąd nazwa w komentarzu. Funkcja stoi w anonimowej przestrzeni nazw |
| `command->UserCallbackData` | wskaźnik na dane polecenia. Rzutuję go na `const GLuint*` i odczytuję identyfikator samplera |
| `glBindSampler(IMGUI_TEXTURE_UNIT, sampler)` | od tej chwili jednostka 0 czyta tekstury moim samplerem. Wykonuje się **w środku** rysowania ImGui, między jego wywołaniami `glDrawElements` |
| `ImGui::GetWindowDrawList()` | lista rysowania okna, które jest właśnie budowane (panel Assets). Dlatego komentarz w nagłówku każe wołać `begin` wewnątrz okna |
| `AddCallback(bindSampler, &sampler, sizeof(sampler))` | trzeci argument, rozmiar, zmienia znaczenie drugiego. Bez rozmiaru ImGui zapamiętałoby sam **wskaźnik**, a ten wskazuje zmienną lokalną `sampler`, która przestaje istnieć po wyjściu z `begin`, na długo przed rysowaniem. Z rozmiarem ImGui **kopiuje** wskazane bajty do własnego bufora listy i podaje wywołaniu wskaźnik na kopię. Wywołanie nie zależy więc ani od zmiennej lokalnej, ani od samego obiektu `RawTextureSampler` |
| `GLuint sampler = m_sampler;` | kopia do zmiennej lokalnej bez `const`: `AddCallback` bierze `void*`, a funkcja `begin` jest `const`, więc adres pola miałby typ `const GLuint*` |
| `DrawCallback_SetSamplerLinear` | gotowe wywołanie zwrotne backendu, wystawione w `ImGuiPlatformIO`. Sprawdziłem w źródle backendu z katalogu budowania (`imgui_impl_opengl3.cpp`, wersja 1.92.9b): ustawia bieżący sampler backendu na jego własny sampler liniowy i woła `glBindSampler(0, ...)`. Po `end` wszystko, co ImGui rysuje dalej, jest więc czytane jak zawsze |
| `if (!isSupported()) return;` w obu | bez rozszerzenia do listy nic nie trafia i podgląd rysuje się po staremu, tyle że z dekodowaniem |

Kolejność na liście rysowania okna Assets dla jednej tekstury sRGB wygląda więc tak: polecenie `bindSampler`, prostokąt z obrazem, polecenie `DrawCallback_SetSamplerLinear`. Po zakończeniu całego rysowania backend przywraca sampler, który zastał na jednostce 0 przed swoją pracą, tak jak resztę stanu (sekcja 3.2).

#### 5.11.6 Użycie w panelu Assets

```cpp
        const bool isSrgb = loaded.texture.colorSpace() == gfx::ColorSpace::Srgb;
        ImGui::Text("  %d x %d px, %s", loaded.texture.width(), loaded.texture.height(),
                    isSrgb ? "sRGB" : "linear");
        // (...)
        const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
        if (isSrgb) {
            rawSampler.begin();
        }
        ImGui::Image(textureId, {PREVIEW_SIZE, PREVIEW_SIZE}, {0.0F, 1.0F}, {1.0F, 0.0F});
        if (isSrgb) {
            rawSampler.end();
        }
```

| Linia | Znaczenie |
|---|---|
| `loaded.texture.colorSpace()` | tekstura pamięta, z jaką przestrzenią kolorów została utworzona (`gfx::Texture2D::colorSpace`). Panel niczego nie zgaduje z nazwy pliku |
| dopisek `sRGB` albo `linear` | nowa część linii z rozmiarem. `sRGB`: obraz koloru, dekodowany do wartości liniowych, gdy czyta go shader. `linear`: dane czytane tak, jak są zapisane (mapa normalnych). Na obronie to najszybszy dowód, że format jest wybierany dla każdej tekstury osobno |
| `if (isSrgb)` wokół `begin` i `end` | mapy normalnych nie są teksturami sRGB, więc ich podgląd idzie zwykłym samplerem ImGui, jak do M6. Parametr i tak byłby dla nich bez skutku |
| `ImGui::Image(...)` | bez zmian: te same współrzędne `uv`, które odwracają obraz w pionie ([`assets/asset-cache.md`](assets/asset-cache.md), sekcja 6) |

Sygnatura panelu dostała czwarty parametr, `const RawTextureSampler& rawSampler`, a `AssetsPanel.hpp` deklarację wyprzedzającą `class RawTextureSampler;`. Panel nadal jest wolną funkcją bez stanu: sampler posiada `DebugUI`.

Podglądy w panelu **Framebuffers** tego mechanizmu nie potrzebują. Ich tekstury mają zwykły format `GL_RGBA8`, a kodowanie do sRGB robi wcześniej shader `post/preview.frag`, więc ImGui dostaje gotowe liczby ([`renderer/post-process.md`](renderer/post-process.md)).

#### 5.11.7 Czas życia: pole `DebugUI`

```cpp
    std::vector<unsigned char> m_fontBytes;
    // The sampler the Assets panel shows sRGB textures with. It is created by the
    // constructor of this class, so after the OpenGL context exists, and it is destroyed
    // with the members, while the window is still there (see main.cpp).
    RawTextureSampler m_rawTextureSampler;
```

Obiekt potrzebuje kontekstu OpenGL przy tworzeniu (`glGetStringi`, `glGenSamplers`) i przy niszczeniu (`glDeleteSamplers`). Oba warunki spełnia jako pole `DebugUI`: `DebugUI` jest polem `DebugNightMazeApp`, więc powstaje po oknie i ginie przed nim (sekcja 3.3 i [`core/README.md`](core/README.md), sekcja 7). Pole nie stoi na liście inicjalizacyjnej konstruktora `DebugUI`, więc działa jego konstruktor domyślny, przed ciałem konstruktora, czyli jeszcze przed `ImGui::CreateContext()`. To w porządku: konstruktor samplera woła tylko OpenGL, a ImGui dotykają dopiero `begin` i `end`. Przy niszczeniu jest odwrotnie: ciało destruktora `DebugUI` zamyka ImGui, a pola giną po nim, więc sampler jest usuwany, gdy ImGui już nie ma, a okno jeszcze jest.

#### 5.11.8 Co zostało sprawdzone

- **Zgłoszone dla Windowsa (2026-10-05, sterownik NVIDII):** podglądy w panelu Assets są identyczne co do piksela z podglądami sprzed zmiany. To znaczy, że rozszerzenie tam jest i że sampler robi to, co ma.
- **Niesprawdzone:** zachowanie na sterowniku **bez** rozszerzenia. Z kodu wynika, że podglądy tekstur sRGB będą wtedy ciemniejsze niż pliki, a nic poza tym się nie zmieni. Nikt tego nie zmierzył na żadnej maszynie.
- **macOS:** nie wiadomo, czy sterownik Apple wystawia `GL_EXT_texture_sRGB_decode` w profilu 4.1 Core. To otwarta pozycja w [`../guides/build-macos.md`](../guides/build-macos.md).
- Testu jednostkowego nie ma: klasa woła OpenGL i ImGui.

## 6. Panel ImGui

Jest dwanaście paneli i HUD gry, który panelem nie jest (opis na końcu tej sekcji). Tabele niżej są skrótem: kod każdego panelu linia po linii jest w dokumencie, na który wskazuje komentarz na górze jego plików. Pierwszy to **Renderer** (kod linia po linii w sekcji 5.3):

| Element | Rodzaj | Czego uczy |
|---|---|---|
| `FPS`, `Frame time` | odczyt | Dwie postaci tej samej informacji. Wartości odświeżają się co 0,5 s (uśrednianie w `core::Time`) |
| `Framebuffer`, `Window` | odczyt | Różnica między pikselami a współrzędnymi ekranu. Warto zmienić rozmiar okna i przenieść je między monitorami o różnej gęstości |
| `OpenGL`, `GPU` | odczyt | Jaki kontekst naprawdę dał sterownik i która karta rysuje |
| `Clear color` | edycja | Zmiana stanu OpenGL widoczna natychmiast, ale od M6 tylko przy odznaczonym polu `Skybox`: z niebem żaden piksel nie zostaje w kolorze czyszczenia. Kliknięcie w kwadrat koloru otwiera próbnik. Od M7 kolor z próbnika jest wartością sRGB, którą gra przelicza na liniową przed `glClearColor`. Start: bardzo ciemny granat `(0,022, 0,033, 0,088)`, bliski kolorowi zenitu na obrazach nieba |
| `Lighting` | lista (`Unlit`, `Gouraud`, `Phong`, `Blinn-Phong`) | Pokaz tematu 7: ta sama scena i te same światła bez oświetlenia, z oświetleniem liczonym dla wierzchołka i dla fragmentu, z dwoma wzorami na połysk. Start: `Blinn-Phong`. Teoria i shadery: [`renderer/lighting-gouraud-phong.md`](renderer/lighting-gouraud-phong.md) |
| `Skybox` | pole wyboru | Pokaz tematu 8: tekstura sześcienna jako niebo. Odznaczone, zostawia tło w kolorze `Clear color`. Start: zaznaczone. Podpowiedź po najechaniu mówi, że namalowany księżyc nie podąża za suwakami `Moon` panelu Lights. Teoria, shadery i scenariusz pokazu: [`renderer/skybox.md`](renderer/skybox.md) |
| `Sky brightness` | suwak od 0 do 6 (do M6 od 0 do 3) | Mnożnik liniowego koloru nieba w shaderze fragmentów (`uBrightness`). 0 to czarne niebo, 1 zostawia kolory obrazów bez zmiany. Od pierwszej części M7 wynik trafia do bufora HDR, więc wartości powyżej 1 (gwiazdy, tarcza księżyca) nie są obcinane, tylko przechodzą przez krzywą mapowania tonów. Start: 2,2 (do M6: 1) |

Drugi to **Shaders**, pokaz tematu 2. Pełny opis, kod linia po linii i scenariusz pokazu na obronie są w [`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6. Jak w komunikacie błędu pojawia się nazwa dołączonego pliku: [`gfx/shader-includes.md`](gfx/shader-includes.md):

| Element | Rodzaj | Czego uczy |
|---|---|---|
| `Reload shaders` | przycisk | Wczytywanie na żywo: pliki wszystkich jedenastu programów są czytane, kompilowane i linkowane od nowa w działającym programie. Plik dołączany przez kilka programów (`common/lighting.glsl`, od M7 także `common/color.glsl`) czyta od nowa każdy z nich. Program, którego przeładowanie się nie udało, działa dalej w poprzedniej wersji |
| `textured.vert + textured.frag: OK` (jedna linia na program, jedenaście linii. Szósta to `grass.vert + grass.geom + grass.frag: OK`, cztery następne, od M7, to `composite.vert + composite.frag: OK`, `composite.vert + preview.frag: OK`, `composite.vert + bright.frag: OK` i `composite.vert + blur.frag: OK`, a jedenasta, od czwartej części M7, to `shadow_depth.vert + shadow_depth.frag: OK`) | odczyt | Z których plików powstał program i że ostatnie wczytanie się udało. Nazwy są połączone znakiem ` + ` w kolejności, w jakiej etapy pracują: shader wierzchołków, shader geometrii (tylko gdy program go ma, `Shader::hasGeometryStage()`), shader fragmentów. Dziś trzy pliki ma jeden program, trawa. W linii są same nazwy plików. Podpowiedź po najechaniu kursorem pokazuje pełne ścieżki, po jednej w linii, w tej samej kolejności |
| `lit.vert + lit.frag: FAILED, the previous program stays in use` | odczyt, na czerwono | Nieudane przeładowanie programu, który wcześniej działał: `isValid()` jest prawdą, gra rysuje dalej poprzednią wersją |
| `... FAILED, there is no program to draw with` | odczyt, na czerwono | Nieudane **pierwsze** wczytanie: `isValid()` jest fałszem i część sceny rysowana tym programem znika |
| tekst błędu pod linią `FAILED` | odczyt, na czerwono | Treść `lastError()`: dziennik sterownika, w którym numer pliku źródłowego jest zamieniony na jego nazwę, na przykład `common/lighting.glsl(63) : error C0000: ...`. Czerwień to stała motywu `ERROR_TEXT_COLOR` (sekcja 5.8.2) |

Przed M4 panel pokazywał dla każdego programu cztery linie: osobno oba pliki, stan programu i wynik ostatniego wczytania. Z jedenastoma programami byłoby to co najmniej czterdzieści cztery linie, a dziś jest jedenaście. Informacja jest ta sama: nazwy plików, wynik ostatniego wczytania i to, czy jest czym rysować. Od drugiej części M6 funkcja `drawShaderStatus` nie zakłada już dwóch plików: skleja napis `files` z nazw i napis `fullPaths` ze ścieżek, a plik shadera geometrii dokłada w środku, gdy `hasGeometryStage()` jest prawdą. Kod linia po linii: [`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6.

Trzeci to **Camera**, pokaz tematu 3. Przy pierwszym uruchomieniu panel jest **zwinięty** do paska tytułu przy górnej krawędzi okna, na prawo od lewej kolumny: strzałka w pasku go rozwija (sekcja 5.7). Linia pomocy w panelu wymienia klawisze kamery i gracza, a klawiszy F i R w niej nie ma: są w etykietach `Flashlight on (key F)` i `Restart round (key R)`. Kod linia po linii, znaczenie każdej kontrolki i scenariusz pokazu na obronie są w [`scene/camera-controls.md`](scene/camera-controls.md), sekcja 6:

| Element | Rodzaj | Czego uczy |
|---|---|---|
| linia pomocy | odczyt | Sterowanie: kliknięcie w scenę przechwytuje mysz, Escape ją oddaje, chodzenie (W A S D, lewy Shift to sprint), noclip pod klawiszem N (W A S D wzdłuż kierunku patrzenia, spacja w górę, lewy Shift w dół) |
| `Mode` | odczyt | `walking` albo `noclip (free flight)` |
| `Player feet` | edycja (trzy pola przeciągane) | Pozycja gracza. Kamera nie ma własnej pozycji do edycji: po każdym kroku staje w oczach gracza |
| `Eye` | odczyt | Pozycja kamery: 1,7 m nad stopami |
| `Yaw`, `Pitch` | edycja (suwaki) | Kierunek patrzenia z dwóch kątów, zawijanie yaw, ograniczenie pitch do 89 stopni |
| `FOV` | edycja (suwak) | Kąt widzenia jako zoom |
| `Near plane`, `Far plane` | edycja (suwaki) | Płaszczyzny przycinania: co jest bliżej albo dalej, znika |
| `Mouse sensitivity` | edycja (suwak) | Stopnie na jednostkę ruchu myszy |
| `Walk speed`, `Sprint speed`, `Fly speed` | edycja (suwaki) | Trzy prędkości gracza w metrach na sekundę |

Czwarty to **Gameplay**, panel reguł rozgrywki z M5. Tak jak Camera startuje **zwinięty**, obok niego po prawej (sekcja 5.7): stan rundy pokazuje przez cały czas HUD, a panel służy do zmieniania reguł na pokazie. Znaczenie każdej liczby i scenariusz pokazu: [`game/gameplay.md`](game/gameplay.md), sekcja 6. Szkielet panelu i droga danych: sekcja 5.5:

| Element | Rodzaj | Co pokazuje albo zmienia |
|---|---|---|
| `Round: playing, 12.3 s` albo `Round: won, ...` | odczyt | Stan rundy (`RoundState`, stanu przegranej nie ma) i czas od jej początku |
| `Crystals: N collected, N needed, N in the maze` | odczyt | Zebrane, potrzebne do otwarcia bramy i wszystkie |
| `Gate: closed`, `Gate: opening, N%` albo `Gate: open` | odczyt | Brama: zamknięta, w trakcie opuszczania (`gateProgress` w procentach) albo otwarta |
| `Restart round (key R)` | przycisk | Ustawia flagę `GameplaySettings::restart`. Gra zaczyna nową rundę na tym samym labiryncie na początku następnej klatki. To samo robi klawisz R |
| `Battery` | suwak (od 0 do 1) | Ładunek baterii latarki, jedyne pole rundy, które panel zmienia. Pozwala obejrzeć migotanie słabej baterii i ciemność pustej bez czekania |
| `Battery drains` | pole wyboru (przy starcie zaznaczone) | Czy bateria się rozładowuje. Odznaczone zatrzymuje ładunek tam, gdzie ustawił go suwak |
| `Crystals needed` | suwak (od 0,05 do 1, start 0,70) | Jaka część kryształów otwiera bramę |
| `Battery lifetime` | suwak (od 5 do 600 s, start 180) | Na ile sekund świecenia starcza pełna bateria |
| `Recharge` | suwak (od 0 do 1, start 0,25) | Ile ładunku oddaje jeden zebrany kryształ, jako część pełnej baterii |
| `Flicker below` | suwak (od 0 do 0,5, start 0,20) | Ładunek, poniżej którego latarka migocze, a pasek baterii na HUD zmienia kolor. Zero wyłącza migotanie |
| `Pickup radius` | suwak (od 0,1 do 2 m, start 0,60) | Promień sfery zbierania kryształu |

Wszystkie suwaki mają flagę `ImGuiSliderFlags_AlwaysClamp`, więc także wartość wpisana z klawiatury (Ctrl i kliknięcie) zostaje w zakresie. Zakresy to nazwane stałe na górze `GameplayPanel.cpp`, a wartości startowe to inicjalizatory pól `game::GameplaySettings`.

Piąty to **Terrain**, pokaz tematu 13 (teren z mapy wysokości). Startuje **zwinięty** w drugim rzędzie pasków, pod panelem Camera (sekcja 5.7). Kod linia po linii: sekcja 5.10, teoria i scenariusz pokazu: [`renderer/terrain.md`](renderer/terrain.md):

| Element | Rodzaj | Co pokazuje albo zmienia |
|---|---|---|
| `Height scale` | suwak (od 0 do 2,5, start 1,00) | Liczba, przez którą mnożona jest każda wysokość terenu. 0 to płaski świat, 1 teren taki, jak zaprojektowany. Zmiana ustawia prośbę `rebuild`: gra buduje teren od nowa i stawia na nim ściany, bramę, kryształy i gracza. Podpowiedź po najechaniu mówi to samo |
| `Wireframe` | pole wyboru (przy starcie odznaczone) | Teren jako krawędzie trójkątów zamiast wypełnionych ścian (`glPolygonMode`). Widać siatkę, z której jest zbudowany. Reszta sceny zostaje wypełniona |
| `Grid: N x N points, 0.50 m apart` | odczyt | Liczba punktów siatki wzdłuż X i Z oraz odstęp między nimi. Dla labiryntu startowego 97 x 97 |
| `Triangles: N` | odczyt | Dwa trójkąty na kwadrat siatki. Dla labiryntu startowego 18432 |
| `Height: A m to B m` | odczyt | Najniższy i najwyższy punkt całej siatki, razem ze wzgórzami wokół labiryntu. Dla labiryntu startowego przy skali 1: od 0,00 do 3,37 m |

Szósty to **Grass**, pokaz tematu 9 (shader geometrii). Startuje **zwinięty** w drugim rzędzie pasków, pod panelem Gameplay. Kod linia po linii: sekcja 5.10, teoria, shadery i scenariusz pokazu: [`renderer/grass-geometry.md`](renderer/grass-geometry.md):

| Element | Rodzaj | Co pokazuje albo zmienia |
|---|---|---|
| `Enabled` | pole wyboru (przy starcie zaznaczone) | Czy trawa jest rysowana. Odznaczone pomija całe wywołanie rysujące trawy |
| `Density` | suwak (od 0 do 8, start 2,5, pokazywany jako `2.5 per m`) | Kępki na metr ściany, liczone osobno po każdej stronie ściany. Rzadki rozsiew na wzgórzach idzie za tą liczbą proporcjonalnie. Zmiana ustawia prośbę `replant`: gra wybiera miejsca kępek od nowa. Zero nie sadzi niczego |
| `Blade height` | suwak (od 0,05 do 0,8 m, start 0,30) | Wysokość najwyższych źdźbeł. Uniform shadera geometrii, działa od następnej klatki bez sadzenia |
| `Wind strength` | suwak (od 0 do 3, start 1,00) | Jak daleko wiatr odchyla czubki źdźbeł. 0 to bezruch. Też uniform |
| `Tufts: N (M blades)` | odczyt | Liczba kępek, czyli punktów w buforze wierzchołków, i liczba źdźbeł (trzy na kępkę), których w żadnym buforze nie ma: buduje je shader geometrii. Przy ustawieniach startowych 1843 kępki, 5529 źdźbeł |

Siódmy to **Framebuffers**, od pierwszej części M7 pokaz tematu 10 (rendering pozaekranowy) w jego dzisiejszym zakresie, od drugiej części z kontrolkami i obrazami bloomu, od trzeciej z kontrolkami mgły i winiety. Startuje **zwinięty** w trzecim rzędzie pasków, jako jeden pasek pod Terrain i Grass (sekcja 5.7). Dopóki jest zwinięty albo panele są schowane, gra nie rysuje podglądów (sekcja 3.2). Od trzeciej części M7 kontrolki stoją w pasku dwóch zakładek (`ImGui::BeginTabBar`, `ImGui::BeginTabItem`, `ImGui::EndTabItem`, `ImGui::EndTabBar`: pierwsze użycie zakładek w projekcie): `Tone and bloom` z siedmioma kontrolkami z drugiej części i `Fog and vignette` z ośmioma nowymi. Każda zakładka to tabela o dwóch kolumnach i czterech wierszach (`ImGui::BeginTable`, `ImGui::TableNextColumn`), wypełniana wierszami w kolejności z tabeli niżej (w pierwszej zakładce ostatnia komórka jest pusta). `BeginTabItem` zwraca prawdę tylko dla zakładki wybranej, więc w klatce rysują się kontrolki jednej z nich. Zakładki są po to, żeby osiem nowych kontrolek nie wydłużyło panelu: zamiast ośmiu wierszy są cztery i pasek zakładek, a stała `FRAMEBUFFERS_HEIGHT = 344` została bez zmian (komentarz nad funkcją `drawSettings`: "so that the panel stays short enough to show the pictures under them without scrolling". Nikt tego nie zmierzył na ekranie). Pod zakładkami nic się nie zmieniło: kreska, dwie linie informacyjne i cztery obrazy obok siebie. Kod linia po linii, teoria i scenariusz pokazu: [`renderer/post-process.md`](renderer/post-process.md):

| Element | Rodzaj | Co pokazuje albo zmienia |
|---|---|---|
| pasek zakładek `Tone and bloom`, `Fog and vignette` | dwie zakładki, od trzeciej części M7 | Która czwórka wierszy kontrolek jest widoczna. Pierwsza zakładka niesie siedem kontrolek z wierszy niżej, od `Exposure` do `Depth range`, druga osiem, od `Fog` do `Radius`. Linie informacyjne i obrazy pod zakładkami są wspólne. Ustawienia z zakładki, której nie widać, działają dalej: zakładka chowa kontrolki, a nie efekt |
| `Exposure` | suwak logarytmiczny (od 0,10 do 8, start 1,00) | Liczba, przez którą przebieg składający mnoży kolory sceny przed mapowaniem tonów. 1 niczego nie zmienia. Skala logarytmiczna: dwa razy mniej i dwa razy więcej światła to kroki tej samej długości |
| `Tone mapping` | lista (`None (clamp)`, `Reinhard`, `ACES (fitted)`, start: `ACES (fitted)`) | Krzywa, która sprowadza kolory jaśniejsze niż 1 w zakres ekranu. Podpowiedź po najechaniu mówi też, że widoki do szukania błędów (normalne, UV) są pokazywane bez pięciu rzeczy: jej koniec brzmi od trzeciej części M7 `without exposure, tone mapping, bloom, fog and vignette.` |
| `Bloom` | pole wyboru (przy starcie zaznaczone), od drugiej części M7 | Czy bloom jest rysowany i dodawany do obrazu. Odznaczone daje dokładnie klatkę bez bloomu. Podpowiedź mówi, że widoki do szukania błędów są pokazywane bez niego |
| `Blur iterations` | suwak liczb całkowitych (od 1 do 10, start 6), od drugiej części M7 | Ile razy biegnie rozmycie Gaussa bloomu, każdy raz jako przebieg poziomy i pionowy. Więcej daje szerszą poświatę. Granice to stałe `game::MIN_` i `MAX_BLOOM_BLUR_ITERATIONS` |
| `Threshold` | suwak (od 0 do 4, start 0,80), od drugiej części M7 | Próg jasności bloomu: świeci tylko światło jaśniejsze od tej liczby. 1 to biel ekranu przed ekspozycją. Co zostaje po progu, pokazuje obraz `Bright pass` |
| `Intensity` | suwak (od 0 do 2, start 1,00), od drugiej części M7 | Liczba, przez którą rozmyta poświata jest mnożona przed dodaniem do sceny. 0 nie dodaje nic |
| `Depth range` | suwak (od 2 do 100 m, start 15) | Odległość, którą podgląd głębi pokazuje jako biel. Czerń to 0 m. W pierwszej części M7 stał pod linią `Scene framebuffer`, dziś jest ostatnią kontrolką pierwszej zakładki (komórka obok niego jest pusta) |
| `Fog` | pole wyboru (przy starcie zaznaczone), od trzeciej części M7, pierwsza kontrolka drugiej zakładki | Czy przebieg składający domieszkuje mgłę. Odznaczone daje dokładnie klatkę bez mgły: tekstura głębi nie jest wtedy ani wiązana, ani czytana. Podpowiedź mówi, że dalekie i niskie powierzchnie przechodzą w kolor mgły, że mgła jest liczona w przebiegu składającym z głębi sceny i że widoki do szukania błędów są pokazywane bez niej |
| `Density` | suwak (od 0 do 0,5, start 0,100, pokazywany jako `0.100 /m`), od trzeciej części M7 | Gęstość mgły na metr, na wysokości podstawy i poniżej. Podpowiedź podaje wzór: `amount = 1 - exp(-density * height factor * distance)`, i liczbę: przy 0,1 połowa powierzchni leżącej na ziemi znika po 6,9 m. 0 to brak mgły. Przy górnej granicy połowa znika po 1,4 m |
| `Base height` | suwak (od -2 do 6 m, start 0,50, pokazywany jako `0.50 m`), od trzeciej części M7 | Wysokość świata (y), do której mgła ma pełną gęstość. Podpowiedź mówi dziś, że grunt labiryntu sięga około 0,6 m przy skali wysokości 1 i że ta liczba nie idzie za skalą wysokości (`The ground of the maze reaches about 0.6 m at height scale 1. This number does not follow the height scale.`). Do czwartej części M7 mówiła o gruncie między 0 a 0,5 m, co było nieścisłe. Zakres sięga od poniżej najniższego gruntu do powyżej szczytów ścian (3 m) |
| `Height falloff` | suwak (od 0 do 3, start 0,40, pokazywany jako `0.40 /m`), od trzeciej części M7 | Jak szybko mgła rzednie powyżej podstawy. Wzór z podpowiedzi: `height factor = exp(-falloff * metres above the base)`. 0 daje tę samą mgłę na każdej wysokości, razem z niebem. Przy górnej granicy mgła jest warstwą o grubości około metra |
| `Fog colour` | próbnik koloru (`ColorEdit3`, start sRGB 0,14, 0,18, 0,26), od trzeciej części M7 | Kolor, w który przechodzą powierzchnie, jako wartość sRGB. Gra przelicza go na liniowy raz na klatkę (`gfx::srgbToLinear` w `PostProcess::composite`, pułapka 54). Mgła jest domieszkowana przed ekspozycją i mapowaniem tonów: z krzywą Reinharda albo ACES jest na ekranie ciemniejsza niż próbka, a z `None (clamp)` przy ekspozycji 1 zgadza się z nią. Tak mówi dziś podpowiedź, poprawiona w czwartej części M7 (wcześniej mówiła bez zastrzeżeń, że mgła jest ciemniejsza niż próbka). Próbnik dostaje `&fog.color.x`, adres pierwszej z trzech liczb `float` wektora |
| `Vignette` | pole wyboru (przy starcie zaznaczone), od trzeciej części M7 | Czy rogi gotowego obrazu są przyciemniane, po mapowaniu tonów. Odznaczone daje dokładnie klatkę bez winiety. Podpowiedź mówi, że widoki do szukania błędów są pokazywane bez niej |
| `Strength` | suwak (od 0 do 1, start 0,30), od trzeciej części M7 | Jaką część światła tracą rogi. 0 niczego nie zmienia, 1 robi je czarne |
| `Radius` | suwak (od 0 do 0,65, start 0,40), od trzeciej części M7 | Odległość od środka ekranu, we współrzędnych tekstury, od której zaczyna się przyciemnienie. 0,5 to środek krawędzi, 0,71 to róg. Podpowiedź mówi też, że winieta nie jest poprawiana o kształt okna. Górna granica 0,65 leży celowo poniżej odległości do rogu (`game::VIGNETTE_CORNER_DISTANCE`): przyciemnienie potrzebuje miejsca, żeby narosnąć |
| `Scene framebuffer: W x H px, GL_RGBA16F + GL_DEPTH_COMPONENT24` | odczyt | Rozmiar framebuffera sceny w pikselach i formaty jego dwóch załączników. Rozmiar idzie za oknem |
| `Bloom targets (3): W x H px, GL_RGBA16F` albo `Bloom targets: not drawn (bloom off or a debug view)` | odczyt, od drugiej części M7 | Rozmiar i format trzech celów bloomu: połowa szerokości i wysokości sceny. Drugi napis stoi, gdy bloom nie był rysowany w tej klatce |
| `HDR colour` | obraz (podpowiedź: `The colour attachment of the scene, cut off at 1.`) | Załącznik koloru framebuffera sceny: zawartość bufora zakodowana do sRGB, bez bloomu, bez ekspozycji i bez krzywej, z wartościami powyżej 1 obciętymi. Mgły i winiety też na nim nie ma: obie powstają dopiero w przebiegu składającym, a obraz pokazuje bufor. W pierwszej części M7 podpis brzmiał `Colour (HDR, cut off at 1)` |
| `Depth` | obraz (podpowiedź: `The depth attachment of the scene, as a distance.`) | Załącznik głębi przeliczony na odległość od kamery: od czerni (przy kamerze) do bieli (`Depth range` i dalej). Niebo jest białe. W pierwszej części M7 podpis brzmiał `Depth (as distance)` |
| `Bright pass` | obraz (podpowiedź: `What the scene has above the bloom threshold.`), od drugiej części M7 | Wynik przebiegu jasności: czerń i ostre jasne plamy tam, gdzie scena przekracza próg |
| `Bloom` | obraz (podpowiedź: `The bright pass after the blur, before the intensity.`), od drugiej części M7 | Ten sam obraz po rozmyciu, przed pomnożeniem przez `Intensity`: to, co przebieg składający dodaje do sceny |

W pierwszej klatce po otwarciu panelu w miejscu obrazów stoi napis `(no picture yet)`: gra rysuje je dopiero w następnej klatce. W miejscu dwóch obrazów bloomu może też stać `(not drawn)`: gdy pole `Bloom` jest odznaczone albo włączony jest widok diagnostyczny. Z kontrolek, które PRD wymienia przy tym panelu obok ekspozycji (podgląd bright pass, bloomu i minimapy, próg bloomu), są od drugiej części M7 wszystkie poza minimapą. Mgła i winieta są od trzeciej części M7, w drugiej zakładce. **Nie ma** tylko minimapy: jest planowana w dalszej części M7. Teksty piętnastu podpowiedzi kontrolek (siedem w pierwszej zakładce, osiem w drugiej) i kod panelu linia po linii są w [`renderer/post-process.md`](renderer/post-process.md), sekcja 6.

Ósmy to **Shadows**, od czwartej części M7 (cienie księżyca, 2026-10-05) pokaz tematu 11 (mapowanie cieni). Od piątej części M7 (cień latarki, 2026-10-06) ma dwie zakładki, `Moon` i `Flashlight`: cień rzucają księżyc i latarka, a cieni świateł kryształów nie ma. Startuje **zwinięty** w czwartym rzędzie pasków, jako jeden pasek pod paskiem Framebuffers i tak samo szeroki (sekcja 5.7). Dopóki jest zwinięty albo panele są schowane, gra nie rysuje obrazu podglądu żadnej z map (sekcja 3.2). Same mapy cieni rysuje dalej. Co każda kontrolka znaczy dla cieni (shadow acne, peter panning, PCF) i jak ją pokazać na obronie: [`renderer/shadows.md`](renderer/shadows.md), sekcja 6. Tutaj jest kod panelu, z pliku [`ShadowsPanel.cpp`](../../src/debug/panels/ShadowsPanel.cpp). Funkcja panelu:

```cpp
void drawShadowsPanel(const ShadowMapView& moon, const ShadowMapView& flashlight) {
    // First run only: the fourth row of title bars at the top edge of the window, folded
    // (the constant is in PanelLayout.hpp). Later ImGui remembers the panel in
    // imgui.ini.
    placePanelOnFirstUse(SHADOWS_PLACEMENT);
    // Begin returns false when the panel is folded: no tab is drawn then, and no
    // preview picture is asked for.
    const bool open = ImGui::Begin("Shadows");
    // BeginTabBar returns false when the bar cannot be seen. EndTabBar must not be
    // called then. BeginTabItem returns true for the tab that is selected.
    if (open && ImGui::BeginTabBar("lights")) {
        if (ImGui::BeginTabItem("Moon")) {
            drawShadowMapTab("Depth seen from the moon",
                             "The shadow map: black is near the moon, white is far\n"
                             "from it or empty. The walls are the dark lines.",
                             moon);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Flashlight")) {
            drawShadowMapTab("Distance seen from the flashlight",
                             "The shadow map of the flashlight: black is at the hand,\n"
                             "white is as far as the beam reaches, or empty. The map\n"
                             "has a perspective projection, so its stored depth is\n"
                             "turned back into metres for this picture: shown as it\n"
                             "is, the picture would be almost white. No picture while\n"
                             "the flashlight is off.",
                             flashlight);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
```

| Fragment | Co robi |
|---|---|
| `placePanelOnFirstUse(SHADOWS_PLACEMENT)` | miejsce, rozmiar i stan zwinięcia przy pierwszym uruchomieniu: czwarty rząd pasków, 612 x 324 po rozwinięciu (sekcja 5.7) |
| `const bool open = ImGui::Begin("Shadows");` | wynik `Begin` jest zapamiętany, a nie użyty od razu w `if`: `EndTabBar` wolno wołać tylko wtedy, gdy panel jest rozwinięty. Do czwartej części M7 stała tu jeszcze linia `moon.preview = open;`. Od piątej flagę ustawia `drawShadowMapTab`, tylko dla zakładki, która jest pokazana (niżej) |
| `const ShadowMapView& moon`, `const ShadowMapView& flashlight` | struktura z `ShadowsPanel.hpp`: cztery referencje albo wartości jednego światła, `settings` (`game::ShadowSettings&`, edytowane), `map` (`const game::ShadowMap&`), `lightSpace` (`const scene::LightSpace&`) i `drawn` (`bool`). Sama struktura jest `const`, ale `settings` jest referencją, więc panel przez nią pisze. Budowana co klatkę w `DebugUI::draw`, jak `DebugContext` |
| `ImGui::BeginTabBar("lights")` | pasek zakładek o identyfikatorze `lights`: jedna zakładka na każde światło, które rzuca cień. `EndTabBar` wolno wołać tylko wtedy, gdy `BeginTabBar` zwróciło prawdę, dlatego stoi wewnątrz `if` |
| `ImGui::BeginTabItem("Moon")`, `ImGui::BeginTabItem("Flashlight")` | po jednej zakładce na światło, które rzuca cień. Do czwartej części M7 pasek miał jedną zakładkę i był na zapas. Latarka to dokładnie ten drugi blok `BeginTabItem` z jednym wywołaniem `drawShadowMapTab`, tak jak to było zaplanowane |
| dwa napisy w wywołaniu | podpis nad obrazem i tekst podpowiedzi obrazu. Są argumentami, bo dla innego światła brzmią inaczej: obraz latarki ma podpis `Distance seen from the flashlight` i podpowiedź, że głębia jest przeliczona na metry |
| `ImGui::End()` | poza `if`, jak w każdym panelu |

Zakładka to tabela o dwóch kolumnach:

```cpp
// One tab: the widgets and the facts of a shadow map on the left, its picture on the
// right. One more light with a shadow map is one more call of this function.
void drawShadowMapTab(const char* pictureCaption, const char* pictureTooltip,
                      const ShadowMapView& view) {
    // This tab is the one that is shown: its preview picture is asked for. The game
    // reads the flag in its next frame, and DebugUI::draw clears it before every frame,
    // so the picture of a tab nobody looks at is not drawn.
    view.settings.preview = true;

    // BeginTable returns false when no part of the table can be seen. Nothing is drawn
    // then, and EndTable must not be called.
    if (!ImGui::BeginTable("shadow map", PANEL_COLUMNS)) {
        return;
    }
    ImGui::TableNextColumn();
    drawSettings(view.settings);
    ImGui::Separator();
    drawFacts(view);

    ImGui::TableNextColumn();
    drawPicture(pictureCaption, pictureTooltip, view.map, view.drawn);
    ImGui::EndTable();
}
```

`PANEL_COLUMNS` to 2. Tabela ma jeden wiersz: pierwsze `TableNextColumn` otwiera lewą komórkę, w której stoją kontrolki (`drawSettings`), kreska i trzy linie odczytu (`drawFacts`), drugie otwiera prawą, z obrazem (`drawPicture`). W panelu Framebuffers tabela układa kontrolki w siatkę, tutaj dzieli panel na dwie połowy. Lewa kolumna, od góry:

| Element | Rodzaj | Co pokazuje albo zmienia |
|---|---|---|
| zakładki `Moon` i `Flashlight` | dwie zakładki w pasku `lights` | Ustawienia i obraz mapy cieni księżyca albo latarki. Obie mają te same widżety (`drawSettings`), różnią się wartościami startowymi, podpisem obrazu i dwiema liniami odczytu. Poniższa tabela opisuje widżety wspólne, wartości startowe w nawiasach to wartości księżyca |
| `Shadows` | pole wyboru (przy starcie zaznaczone) | `ShadowSettings::enabled`. Odznaczone: gra nie rysuje mapy cieni tego światła i nic nie jest w jego cieniu, a cienie drugiego światła zostają. Podpowiedź (od piątej części M7): `Off: the shadow map of this light is not drawn and nothing is in its shadow. The shadows of the other light stay.` |
| `Resolution` | lista (`1024 x 1024`, `2048 x 2048`, start: `2048 x 2048`) | Rozmiar mapy cieni w tekselach, pole `resolution`. Podpowiedź: `The size of the shadow map in texels. The map covers the same area at every size, so a smaller map has larger texels: coarser shadow edges, and more bias is needed.` Start zakładki `Flashlight`: `1024 x 1024` (`game::flashlightShadowDefaults()`) |
| `Constant bias` | suwak (od 0 do 0,5 m, start 0,020, pokazywany jako `0.020 m`) | Stała część biasu, w metrach. Podpowiedź (od piątej części M7): `Every surface is compared with the shadow map as if it were this much nearer to the light, in metres for both lights. With both parts at 0 the surfaces shade themselves (shadow acne): stripes with both filters off, an overall darkening with them on.` Start zakładki `Flashlight`: 0,010 |
| `Slope bias` | suwak (od 0 do 1 m, start 0,120, pokazywany jako `0.120 m`) | Część biasu dodawana na powierzchniach, po których światło tylko się ślizga. Start zakładki `Flashlight`: 0,130. Podpowiedź podaje wzór: `Added on top for surfaces the light only grazes: bias = constant + slope * (1 - cos of the angle between the normal and the light). Too much bias lets a shadow come loose from the wall that casts it (peter panning).` |
| `Hardware 2 x 2 filter` | pole wyboru (przy starcie zaznaczone) | Pole `hardwareFilter`: filtr liniowy samplera z porównaniem. Podpowiedź: `The graphics card compares the four texels around a place and blends the four answers (a linear comparison sampler). Off: one texel, and the edges of the shadows show steps.` |
| `PCF` | pole wyboru (przy starcie zaznaczone) | Pole `pcf`: czy shader uśrednia kwadrat porównań. Podpowiedź: `Percentage closer filtering: the comparison is made for a square of texels and the answers are averaged, which makes the edge of a shadow soft.` |
| `Kernel` | lista w tej samej linii co `PCF` (`3 x 3`, `5 x 5`, `7 x 7`, start: `3 x 3`) | Rozmiar kwadratu PCF. Ustawienia trzymają promień (`pcfRadius`, od 1 do 3), lista pokazuje bok. Podpowiedź: `How many texels the PCF square has. A larger kernel is softer and costs more lookups per pixel: 9, 25 or 49.` |
| `Strength` | suwak (od 0 do 1, start 1,00) | Jaką część światła księżyca zabiera cień. Podpowiedź: `The share of the light of this lamp a shadow takes away. 1: none of it is left in a shadow. The ambient light and the other lights are never darkened.` |
| kreska | `ImGui::Separator()` | Oddziela kontrolki od odczytów |
| `Map: W x H, GL_DEPTH_COMPONENT24` albo `Map: not drawn` | odczyt | Rozmiar framebuffera mapy i nazwa formatu głębi (`gfx::depthFormatName`). Przy starcie `Map: 2048 x 2048, GL_DEPTH_COMPONENT24` w zakładce `Moon` i `Map: 1024 x 1024, GL_DEPTH_COMPONENT24` w zakładce `Flashlight` (policzone z ustawień startowych, nikt tego nie oglądał). Drugi napis stoi, gdy mapa nie jest rysowana (pole `drawn` struktury `ShadowMapView`: cienie tego światła wyłączone, a dla latarki także latarka zgaszona klawiszem F albo pustą baterią) albo framebuffera nie udało się utworzyć |
| `Covers X x Y m, Z m deep` (zakładka `Moon`) | odczyt | Trzy wymiary pudełka światła księżyca (`lightSpace.extent`): szerokość i wysokość obszaru, który mapa pokrywa, oraz głębokość, na którą rozkłada się zapisana głębia od 0 do 1. Dla labiryntu startowego przy ustawieniach startowych (policzone): `Covers 64.8 x 54.1 m, 47.0 m deep`. Liczby zmieniają się z suwakami `Moon yaw` i `Moon pitch`, ze skalą wysokości terenu i z nowym labiryntem, a nie z ruchem gracza |
| `One texel: N cm` (zakładka `Moon`) | odczyt | Rozmiar jednego teksela mapy na powierzchni zwróconej do światła (`game::shadowTexelSize`), dla rozdzielczości wybranej na liście, także gdy cienie są wyłączone. Policzone dla labiryntu startowego: 3,2 cm przy 2048 i 6,3 cm przy 1024 |
| `Covers X x Y m at Z m` (zakładka `Flashlight`) | odczyt | Od piątej części M7. Mapa latarki jest ostrosłupem, więc obszar, który pokrywa, rośnie z odległością od światła. Panel podaje szerokość i wysokość **na dalekiej płaszczyźnie** (`lightSpace.extent`, gdzie jest największy) i odległość tej płaszczyzny (`lightSpace.farPlane`, czyli zasięg latarki). Policzone dla ustawień startowych (kąt otwarcia 46 stopni, zasięg 16 m): `Covers 13.6 x 13.6 m at 16.0 m` |
| `One texel: N cm per metre away` (zakładka `Flashlight`) | odczyt | Od piątej części M7. Teksel też rośnie z odległością, więc panel podaje jego rozmiar w odległości 1 m od światła (`game::shadowTexelSizeAt` ze stałą `TEXEL_REFERENCE_DISTANCE`), co jest zarazem tym, o ile rośnie z każdym metrem. Policzone dla 1024 tekseli: `One texel: 0.08 cm per metre away`, czyli 0,3 cm na ścianie w odległości 4 m (nikt tego nie oglądał) |

Szczegóły, które widać dopiero w kodzie `drawSettings`:

```cpp
    int resolutionIndex = static_cast<int>(settings.resolution);
    if (ImGui::Combo("Resolution", &resolutionIndex, RESOLUTION_ITEMS)) {
        settings.resolution = static_cast<game::ShadowResolution>(resolutionIndex);
    }
```

Lista `Combo` pisze do liczby `int`, a pole ma typ wyliczenia `game::ShadowResolution`, stąd dwa rzutowania. Pozycje listy stoją w kolejności wartości wyliczenia (`Low = 0`, `High = 1`), więc numer pozycji jest wartością pola. Mówi to komentarz nad stałą:

```cpp
// The entries of the resolution list, in the order of the enum game::ShadowResolution:
// the number of the chosen entry is the value of the enum. ImGui wants the entries in
// one string, each ended by a zero character.
constexpr const char* RESOLUTION_ITEMS = "1024 x 1024\0"
                                         "2048 x 2048\0";
```

Oba suwaki biasu i suwak `Strength` mają flagę `ImGuiSliderFlags_AlwaysClamp`, więc także liczba wpisana z klawiatury (Ctrl i kliknięcie) zostaje w zakresie:

```cpp
    ImGui::SliderFloat("Constant bias", &settings.constantBias, MIN_BIAS, MAX_CONSTANT_BIAS,
                       "%.3f m", ImGuiSliderFlags_AlwaysClamp);
```

Zakresy to nazwane stałe na górze pliku, z komentarzem, który mówi, po co są tak szerokie:

```cpp
// Range of the two bias sliders, in metres. At 0 the surfaces shade themselves (shadow
// acne): in stripes with both filters off, as an overall darkening with them on. The
// upper ends are far more than the 0.2 m of a wall, enough to see a shadow come loose
// from the wall that casts it (peter panning).
constexpr float MIN_BIAS = 0.0F;
constexpr float MAX_CONSTANT_BIAS = 0.5F;
constexpr float MAX_SLOPE_BIAS = 1.0F;
```

Ten komentarz i podpowiedź suwaka `Constant bias` zostały poprawione w czwartej części M7: shadow acne wygląda jak paski tylko przy obu filtrach wyłączonych, a przy włączonych jak ogólne przyciemnienie powierzchni. Na obronie paski pokazuję więc z odznaczonymi polami `Hardware 2 x 2 filter` i `PCF`.

Pole `PCF` i lista `Kernel` stoją w jednej linii:

```cpp
    // The list of kernel sizes stands on the same line as the switch, to keep the panel
    // short. It shows sizes and the settings hold a radius: entry 0 is radius 1.
    ImGui::SameLine();
    ImGui::SetNextItemWidth(KERNEL_LIST_WIDTH * ImGui::GetStyle().FontScaleDpi);
    int kernelIndex = std::clamp(settings.pcfRadius, game::MIN_PCF_RADIUS, game::MAX_PCF_RADIUS) -
                      game::MIN_PCF_RADIUS;
    if (ImGui::Combo("Kernel", &kernelIndex, PCF_KERNEL_ITEMS)) {
        settings.pcfRadius = kernelIndex + game::MIN_PCF_RADIUS;
    }
```

| Fragment | Co robi |
|---|---|
| `ImGui::SameLine()` | następny widżet staje w tej samej linii co poprzedni (pole `PCF`), a nie pod nim |
| `ImGui::SetNextItemWidth(KERNEL_LIST_WIDTH * ...FontScaleDpi)` | szerokość następnego widżetu: 90 pikseli przy skali 100%, pomnożone przez skalę ekranu. Bez tego lista zajęłaby resztę szerokości kolumny |
| `std::clamp(settings.pcfRadius, ...) - game::MIN_PCF_RADIUS` | promień z ustawień sprowadzony do zakresu od 1 do 3 i zamieniony na numer pozycji od 0 do 2. Pozycja numer i to promień i + `MIN_PCF_RADIUS`: `3 x 3` to promień 1, `5 x 5` promień 2, `7 x 7` promień 3 (bok jądra to 2 * promień + 1) |
| `settings.pcfRadius = kernelIndex + game::MIN_PCF_RADIUS;` | droga odwrotna, tylko gdy użytkownik wybrał pozycję (`Combo` zwraca wtedy prawdę) |

Lista `Kernel` nie jest wyszarzana przy odznaczonym polu `PCF`: można ją zmienić, ale skutku nie widać, dopóki pole jest odznaczone (shader dostaje wtedy promień 0, `game::pcfRadiusInUse`).

Trzy linie odczytu pod kreską buduje `drawFacts`, od piątej części M7 z dwiema gałęziami wybieranymi po rodzaju rzutu mapy (`lightSpace.kind`):

```cpp
    const gfx::Framebuffer& target = view.map.target();
    if (view.drawn && target.isValid()) {
        ImGui::Text("Map: %d x %d, %s", target.width(), target.height(),
                    gfx::depthFormatName(target.depthFormat()));
    } else {
        ImGui::TextUnformatted("Map: not drawn");
    }

    const scene::LightSpace& lightSpace = view.lightSpace;
    // The size of a texel at the resolution that is chosen, also while the map is off.
    const int mapSize = game::shadowMapSize(view.settings.resolution);
    if (lightSpace.kind == scene::LightProjection::Perspective) {
        // A pyramid: what the map covers and the size of its texels grow in proportion
        // to the distance from the light. So the covered area is given at the far
        // plane, where it is largest, and the texel as its size 1 m from the light,
        // which is also how much it grows with every metre.
        ImGui::Text("Covers %.1f x %.1f m at %.1f m", lightSpace.extent.x, lightSpace.extent.y,
                    lightSpace.farPlane);
        const float texelPerMetre =
            game::shadowTexelSizeAt(lightSpace, mapSize, TEXEL_REFERENCE_DISTANCE);
        ImGui::Text("One texel: %.2f cm per metre away", texelPerMetre * CENTIMETRES_PER_METRE);
    } else {
        // A box: the same everywhere.
        ImGui::Text("Covers %.1f x %.1f m, %.1f m deep", lightSpace.extent.x, lightSpace.extent.y,
                    lightSpace.extent.z);
        const float texelSize = game::shadowTexelSize(lightSpace, mapSize);
        ImGui::Text("One texel: %.1f cm", texelSize * CENTIMETRES_PER_METRE);
    }
```

Pierwsza linia czyta rozmiar z framebuffera, czyli pokazuje mapę, która naprawdę istnieje, a trzecia liczy z ustawienia. Dla mapy latarki przestrzeń światła jest liczona w każdej klatce także wtedy, gdy mapy nie rysowano (zgaszona latarka), więc linie `Covers` i `One texel` nie znikają. Po zmianie pozycji na liście `Resolution` mogą się więc różnić przez jedną klatkę: gra tworzy framebuffer w nowym rozmiarze dopiero w swoim następnym przebiegu cieni. Linie druga i trzecia są wypisywane także przy wyłączonych cieniach, bo pudełko światła gra liczy w każdej klatce.

Prawa kolumna to obraz:

```cpp
void drawPicture(const char* caption, const char* tooltip, const game::ShadowMap& map, bool drawn) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(caption);
    const gfx::Framebuffer& preview = map.preview();
    if (drawn && preview.isValid()) {
        // As large as the column is wide or the panel is high, whichever is smaller.
        const ImVec2 room = ImGui::GetContentRegionAvail();
        const float side = std::min(room.x, room.y);
        // ImGui identifies a texture by the id of the texture object. A framebuffer
        // texture has its row v = 0 at the BOTTOM, so the corners are (0, 1) and (1, 0)
        // (see the Framebuffers panel). The picture is a plain GL_RGBA8 texture: the
        // depth texture itself is never handed to ImGui.
        const auto textureId = static_cast<ImTextureID>(preview.colorTextureId());
        ImGui::Image(textureId, {side, side}, {0.0F, 1.0F}, {1.0F, 0.0F});
    } else {
        // The first frame after the panel was opened (the picture is drawn by the game
        // in its next frame), or the map is not drawn: its shadows are switched off, or
        // it is the map of the flashlight and the flashlight is off.
        ImGui::TextUnformatted(drawn ? "(no picture yet)" : "(not drawn)");
    }
    ImGui::EndGroup();
    ImGui::SetItemTooltip("%s", tooltip);
}
```

| Fragment | Co robi |
|---|---|
| `ImGui::BeginGroup()` i `ImGui::EndGroup()` | podpis i obraz (albo napis zastępczy) tworzą jedną grupę, którą ImGui traktuje potem jak jeden widżet. Dzięki temu `SetItemTooltip` po `EndGroup` daje podpowiedź nad całą grupą, także nad napisem `(not drawn)` |
| `ImGui::TextUnformatted(caption)` | podpis `Depth seen from the moon` |
| `drawn` | `view.drawn`. Dla księżyca nadal wartość `moonShadowSettings.enabled`: przy odznaczonym polu `Shadows` gra nie rysuje ani mapy, ani jej obrazu, więc w miejscu obrazu stoi `(not drawn)`, choć stara tekstura podglądu nadal istnieje. Dla latarki (od piątej części M7) `context.flashlightShadowDrawn`, czyli fakt z ostatniej klatki: obraz znika także przy zgaszonej latarce (klawisz F, pusta bateria). Panel pokazuje więc dla latarki to, co gra naprawdę zrobiła, a dla księżyca to, o co ją poproszono |
| `preview.isValid()` | fałsz, dopóki gra ani razu nie narysowała podglądu: pierwsza klatka po pierwszym otwarciu panelu. Wtedy stoi `(no picture yet)` |
| `ImGui::GetContentRegionAvail()` i `std::min(room.x, room.y)` | miejsce, które zostało w kolumnie pod podpisem. Mapa cieni jest kwadratem, więc obraz też: bok to mniejsza z dwóch liczb, szerokości kolumny i wolnej wysokości panelu |
| `preview.colorTextureId()` | identyfikator tekstury koloru małego framebuffera podglądu (256 x 256, `GL_RGBA8`), który należy do `game::ShadowMap`. Rzutowanie na `ImTextureID` jak w panelach Assets i Framebuffers |
| `{0.0F, 1.0F}, {1.0F, 0.0F}` | współrzędne tekstury lewego górnego i prawego dolnego rogu obrazu, z odwróconym `v`: tekstura framebuffera ma wiersz `v = 0` na dole, a ImGui liczy od góry (pułapka 52) |

**Dlaczego obraz to osobny mały przebieg, a nie sama tekstura głębi.** Mapa cieni to tekstura głębi, czyli tekstura z jednym kanałem. `ImGui::Image` rysuje teksturę zwykłym shaderem backendu, który czyta cztery kanały koloru. Z tekstury głębi dostałby głębię w kanale czerwonym, a w zielonym i niebieskim zera, więc obraz wyszedłby czerwony zamiast szarego. Dlatego gra, tylko gdy panel jest otwarty, rysuje jednym trójkątem programem `preview` (dla księżyca tryb `uMode == 2`, `AttachmentPreview::RawDepth`) szary obraz do własnego framebuffera 256 x 256 w formacie `GL_RGBA8` (`game::ShadowMap::drawPreview`), a panel pokazuje jego teksturę koloru. Czerń to bliska płaszczyzna światła, biel daleka i każdy teksel, w który nic nie zostało narysowane. Głębi nie trzeba tu przeliczać na odległość jak w podglądzie `Depth` panelu Framebuffers: rzut światła kierunkowego jest prostokątny, więc zapisana głębia rośnie równo z odległością ([`renderer/post-process.md`](renderer/post-process.md), sekcje 2.10 i 4.3, oraz [`renderer/shadows.md`](renderer/shadows.md), sekcja 2.17). Obraz 256 x 256 jest skalowany do boku wolnego miejsca w kolumnie, więc pojedynczych tekseli mapy 2048 x 2048 na nim nie widać: pokazuje, co mapa obejmuje, a nie jej ostrość.

**Podgląd mapy latarki jest inny niż księżyca.** Zapisana głębia rzutu perspektywicznego nie rośnie równo z odległością: prawie cały zakres wartości jest wykorzystany w pierwszym metrze, więc pokazana wprost dałaby obraz prawie biały. `ShadowMap::drawPreview(previewShader, lightSpace)` dostaje więc przestrzeń światła i dla rzutu perspektywicznego ustawia tryb `AttachmentPreview::Depth` (ten sam, którym panel Framebuffers pokazuje głębię sceny) z bliską i daleką płaszczyzną latarki oraz zasięgiem jako odległością, która jest biała. Czerń to ręka, biel to koniec wiązki albo pusty teksel. Teorię opisuje [`renderer/shadows.md`](renderer/shadows.md), sekcja 2.20.6.

Stan sprawdzenia: żadnej kontrolki tego panelu nikt nie kliknął myszą, przełączania rozdzielczości w działającym programie nikt nie sprawdził, układu przy innym rozmiarze okna też nie, a na macOS nic nie było budowane ani uruchamiane (akapity o czwartej i piątej części M7 niżej). Zakładki `Flashlight` nikt nie otworzył: obrazu mapy latarki, napisu `(not drawn)` po wciśnięciu F i listy `Resolution` w tej zakładce nikt nie zobaczył.

Dziewiąty to **Maze**. Kod linia po linii i rysowanie planu: [`game/maze-generator.md`](game/maze-generator.md), sekcja 6. Droga prośby od panelu do nowego labiryntu: [`game/maze-rendering.md`](game/maze-rendering.md), sekcja 5:

| Element | Rodzaj | Czego uczy |
|---|---|---|
| `Width`, `Height` | edycja (suwaki całkowite, od 2 do 40 komórek) | Rozmiar następnego labiryntu. Sama zmiana suwaka niczego nie przebudowuje |
| `Seed` | edycja (pole liczbowe z przyciskami plus i minus) | Ziarno generatora: ten sam rozmiar i ziarno dają ten sam labirynt |
| `Regenerate` | przycisk | Ustawia flagę prośby. Gra buduje labirynt na początku następnej klatki |
| `Random seed` | przycisk | Losuje ziarno z `std::random_device` i od razu prosi o nowy labirynt |
| `In play`, `Walls`, `pillars` | odczyt | Labirynt, który jest właśnie w grze: rozmiar, ziarno, liczba ścian i słupków |
| `Crystals: N, exit in cell (x, z)` | odczyt | Ile kryształów ma ten labirynt i w której komórce jest wyjście. Dla labiryntu startowego (10 x 10, ziarno 1): 13 kryształów, wyjście w komórce (6, 5). Ile z nich otwiera bramę, to reguła rundy z panelu Gameplay |
| plan | rysunek | Ściany z góry w kolorze bladego kamienia, północ u góry, bursztynowa kropka gracza i kreska kierunku patrzenia. Od M5 także: zielony obrys strefy wyjścia, brama jako gruba linia (w kolorze drewna, dopóki blokuje przejście, i przygaszona po otwarciu) oraz kryształy (turkusowa kropka dla leżącego, przygaszony okrąg w miejscu zebranego). Kolory to stałe motywu od `PLAN_WALL_COLOR` do `PLAN_EXIT_COLOR` (sekcja 5.8.2) |

Dziesiąty to **Collision**, pokaz tematu 14. Kod linia po linii i scenariusz pokazu: [`scene/collision.md`](scene/collision.md), sekcja 6:

| Element | Rodzaj | Czego uczy |
|---|---|---|
| `Draw collision shapes` | pole wyboru | Rysuje kształty kolizji liniami. Legenda pod polem: `Yellow: walls, pillars. Green: player. Orange: gate. Cyan: crystal pickup. Magenta: exit zone.` Pudełka są rysowane jako krawędzie, sfery zbierania jako trzy okręgi |
| `Noclip (key N)` | pole wyboru | To samo pole co klawisz N: lot bez kolizji |
| `Boxes: N walls, N pillars, N gate` | odczyt | Ile pudełek ma labirynt. Brama liczy się jako jedno pudełko, dopóki blokuje przejście (`game::gateBlocks`), potem jako zero |
| `All boxes: N, pickup spheres: N` | odczyt | Ile pudełek gracz sprawdza w każdym kroku (ściany, słupki i zamknięta brama) i ile sfer zbierania zostało: po jednej na każdy niezebrany kryształ |
| grubość pudełka ściany | odczyt | Pudełko kolizji ściany (0,30 m) jest grubsze niż widoczna ściana (0,20 m) |
| `Player box`, `min`, `max` | odczyt | Dwa narożniki pudełka gracza, liczone z jego pozycji w każdej klatce |

Jedenasty to **Assets**, pokaz tematów 4 i 5. Kod linia po linii i scenariusz pokazu dla każdej kontrolki: [`assets/asset-cache.md`](assets/asset-cache.md), sekcja 6:

| Element | Rodzaj | Czego uczy |
|---|---|---|
| `View mode` | lista (`Textured`, `Normals as colour`, `UVs as colour`) | Co wpisuje shader fragmentów: obraz, normalne albo współrzędne tekstury. Widok normalnych pokazuje normalną faktycznie użytą do cieniowania: z map normalnych albo z siatki |
| `Normal mapping` | pole wyboru (przy starcie zaznaczone) z notatką pod spodem | Mapowanie normalnych: te same ściany z reliefem i bez. Działa w trybach `Phong` i `Blinn-Phong` (panel Renderer) i w widoku `Normals as colour`. Tryb `Gouraud` liczy światło w wierzchołkach i z mapy normalnych nie skorzysta |
| `Filter` | lista (`Nearest`, `Bilinear`, `Trilinear`) | Filtrowanie wszystkich tekstur naraz |
| `Anisotropy` | suwak (wyszarzony, gdy sterownik nie ma rozszerzenia) | Filtrowanie anizotropowe na powierzchniach widzianych pod płaskim kątem |
| `Models` | odczyt | Wczytane modele (dziś pięć: ściana, słupek, dwa kryształy i brama. W M5 było ich sześć, szóstym była płytka podłogi, którą w M6 zastąpił teren. Teren nie jest modelem z pliku, więc na liście go nie ma): plik, liczba wierzchołków i trójkątów, części z materiałami. Pod każdą częścią linia `normal map:` z nazwą pliku mapy normalnych albo `none (flat)`, gdy część korzysta z płaskiej mapy zastępczej |
| `Textures` | odczyt z podglądem | Wczytane tekstury: plik, rozmiar, od M7 przestrzeń kolorów (`sRGB` dla obrazów koloru, `linear` dla map normalnych), obrazek 128 x 128. Podgląd tekstury sRGB jest czytany bez dekodowania, więc wygląda jak plik (sekcja 5.11). W grze jest ich osiem: cztery obrazy koloru (kamień ściany, grunt terenu, kryształ, drewno bramy) i cztery mapy normalnych (`wall_stone_normal.png`, `ground_normal.png`, `crystal_normal.png`, `gate_wood_normal.png`). Parę tekstur gruntu (`ground.png`, `ground_normal.png`) wczytuje przez pamięć podręczną `game::TerrainRenderer`: w M6 zastąpiła parę tekstur podłogi. Mapy wysokości `heightmap.png` na liście nie ma: gra czyta ją raz wprost przez `assets::loadImage`, poza pamięcią podręczną, i nie robi z niej tekstury. Podgląd mapy normalnych jest jasnoniebieski, bo większość tekseli to kierunek bliski `(0, 0, 1)`, czyli kolor `(128, 128, 255)` |
| `Failed to load` | odczyt (tylko gdy coś się nie wczytało) | Pliki, których nie udało się wczytać, na czerwono (ta sama stała `ERROR_TEXT_COLOR` co w panelu Shaders) |

Dwunasty to **Lights**, pokaz tematu 6. Kod linia po linii, znaczenie każdej kontrolki i scenariusz pokazu na obronie są w [`scene/lights.md`](scene/lights.md), sekcja 6. Wszystkie kontrolki piszą do pól jednej struktury, `game::LightingSettings` ([`game/flashlight.md`](game/flashlight.md)), a gra buduje z niej światła następnej klatki, więc każdą zmianę widać od razu:

| Element | Rodzaj | Co zmienia |
|---|---|---|
| `Ambient` | kolor | Światło otoczenia: dociera do każdej powierzchni, także tam, gdzie nie świeci żadne źródło. Od M7 wszystkie cztery kolory tego panelu są wartościami sRGB, przeliczanymi na liniowe przy budowie świateł klatki ([`game/flashlight.md`](game/flashlight.md)) |
| grupa `Moon (directional)` | zwijany nagłówek, przy starcie **zwinięty** | Księżyc, światło kierunkowe |
| `Moon yaw`, `Moon pitch` | suwaki (od 0 do 360 stopni i od -90 do -5 stopni) | Kierunek, w którym leci światło księżyca. Od czwartej części M7 te same suwaki obracają cienie: gra liczy pudełko światła księżyca od nowa w każdej klatce z tych dwóch kątów (`NightMazeApp::drawMoonShadowMap`, `game::moonDirection`), więc mapa cieni i światło nie mogą się rozjechać. W kodzie panelu nic się przy tym nie zmieniło |
| `Moon colour`, `Moon intensity` | kolor i suwak (od 0 do 2) | Barwa i natężenie. Natężenie startuje od 0,2 (do trzeciej części M7 od 0,12): tyle, żeby powierzchnia w świetle księżyca była wyraźnie jaśniejsza od tej w cieniu ściany, która ma tylko światło otoczenia |
| grupa `Flashlight (spot)` | zwijany nagłówek, przy starcie rozwinięty | Latarka, reflektor w ręce gracza (do piątej części M7 w oku) |
| `Flashlight on (key F)` | pole wyboru | To samo pole co klawisz F. Przy pustej baterii gra wyłącza latarkę z powrotem w następnym kroku, więc pola nie da się zostawić zaznaczonego. Mówi o tym podpowiedź po najechaniu kursorem: `The battery is empty: collect a crystal first.` |
| `Beam colour`, `Beam intensity` | kolor i suwak (od 0 do 10) | Barwa i natężenie wiązki |
| `Cone` | dwa pola przeciągane w jednym widżecie (`inner` i `outer`, od 1 do 60 stopni) | Połówkowe kąty stożka: wewnętrzny i zewnętrzny. Widżet pilnuje, żeby pierwszy nie przekroczył drugiego |
| `Beam range` | suwak (od 2 do 60 m) | Zasięg latarki. Od piątej części M7 jest to także daleka płaszczyzna mapy cieni latarki. Podpowiedź: `How far the light reaches. It is also the far plane of the shadow map of the flashlight.` |
| `Hand right` | suwak (od 0 do `game::MAX_FLASHLIGHT_HAND_RIGHT`, czyli 0,25 m, start 0,20, pokazywany jako `0.20 m`) | Od piątej części M7. O ile na prawo od oka stoi latarka (wzdłuż poziomego wektora `right` kamery). Górna granica to połowa szerokości ciała gracza (0,3 m) minus bliska płaszczyzna mapy cieni (0,05 m), żeby światło nie wyszło poza pudełko ciała i nie wsunęło się w ścianę. Podpowiedź mówi o tym i o tym, że przy 0 każdy cień chowa się za rzucającym |
| `Hand down` | suwak (od 0 do 0,5 m, start 0,25, pokazywany jako `0.25 m`) | Od piątej części M7. O ile poniżej oka stoi latarka: pionowo w dół w świecie, niezależnie od tego, w co patrzy kamera. Górna granica 0,5 m to stała panelu `MAX_HAND_DOWN` |
| `Converge at` | suwak (od `game::MIN_FLASHLIGHT_CONVERGE_DISTANCE`, czyli 0,5 m, do 20 m, start 4,0, pokazywany jako `4.0 m`) | Od piątej części M7. Wiązka celuje z ręki w punkt na osi widoku tyle metrów przed okiem. Na ścianie w tej odległości plama leży w środku ekranu, na bliższych po prawej i niżej, na dalszych trochę w lewo i wyżej |
| grupa `Point lights (crystals)` | zwijany nagłówek, przy starcie rozwinięty | Światła punktowe nad kryształami, edytowane wszystkie naraz. Do M4 grupa nazywała się inaczej, bo światła wisiały w ślepych zaułkach |
| `Lit: N of M crystals (at most 16)` | odczyt | Ile kryształów jeszcze świeci (niezebrane) i ile jest wszystkich. Zebrany kryształ traci światło. Dla labiryntu startowego 10 x 10 z ziarnem 1 na początku rundy: 13 z 13 |
| `Point colour`, `Point intensity`, `Point radius` | kolor i dwa suwaki (od 0 do 10 i od 0,5 do 12 m) | Barwa, natężenie i zasięg każdego z nich. Kolor jest też kolorem, którym świecą same kryształy (`game::crystalGlow`) |
| grupa `Highlight (specular)` | zwijany nagłówek, przy starcie rozwinięty | Połysk kamienia |
| `Strength`, `Shininess` | suwaki (od 0 do 2 i od 1 do 256, drugi logarytmiczny) | Jasność połysku i wykładnik we wzorze: większy daje mniejszy i ostrzejszy błysk |

Panel dostaje też `const game::Round&`, ale czyta z niego tylko trzy rzeczy: rozmiar listy `crystals`, `collectedCount` i `battery` (dla podpowiedzi przy polu latarki). Widżety, których w poprzednich panelach nie było (`CollapsingHeader`, `DragFloatRange2`), opisuje [`../libraries/imgui.md`](../libraries/imgui.md), sekcja 3.13.

**HUD gry** nie jest panelem, ale rysuje go ten sam moduł. Architektura (flagi okien, kolejność, brak wejścia) jest w sekcji 5.9, a opis z punktu widzenia gracza w [`game/gameplay.md`](game/gameplay.md), sekcja 6:

| Element | Gdzie | Co pokazuje |
|---|---|---|
| `Crystals  N / M (of K)` i czas `m:ss` | pasek u góry, na środku | Zebrane i potrzebne kryształy, liczba wszystkich, czas rundy |
| pasek baterii z procentem | pasek u góry | Ładunek baterii latarki: bursztynowy, a poniżej progu `Flicker below` czerwony |
| `The gate is open. Find the exit.` | pasek u góry | Podpowiedź po otwarciu bramy |
| `Battery empty. Find a crystal.` | pasek u góry | Podpowiedź przy pustej baterii |
| karta `You escaped` z czasem, kryształami i linią `R: play again` | środek okna | Wygrana runda |

PRD (sekcja 10) wymienia tryb noclip przy panelu Camera. W kodzie przełącznik jest w panelu Collision (bo wyłącza kolizje) i pod klawiszem N, a panel Camera pokazuje tylko bieżący tryb.

Stan sprawdzenia paneli ma dziesięć części.

**Sprzed M4 (Windows, 2026-10-05).** Na zrzutach ekranu sprawdzone są plan w panelu Maze (zgodny z widokiem z góry), podglądy tekstur w panelu Assets (nieodwrócone) oraz skutki obu trybów widoku, czterech ustawień filtrowania i brakującej tekstury. Te stany zostały ustawione tymczasowym kodem, którego już nie ma. Po zmianie motywu sprawdzone są na zrzutach: ówczesny układ paneli (bez Lights) przy pierwszym uruchomieniu, kolory i czcionka, polskie litery, tekst błędu w panelach Shaders i Assets w ich ówczesnej postaci oraz czcionka zastępcza (sekcja 5.8.6).

**M4 (Windows, 2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74), historia.** Build Debug i Release przechodził bez ostrzeżeń, testy przechodziły w obu, clang-format i clang-tidy niczego nie zgłaszały, a gra startowała bez linii `[error]` i bez linii `GL_`. Na zrzutach ekranu sprawdzone były: widok startowy, cztery tryby oświetlenia z trzech punktów widzenia, scena z wyłączoną latarką, światło punktowe na końcu korytarza (wisiało wtedy w zaułku bez wyjścia, dziś światła wiszą nad kryształami), strony ścian oświetlone i nieoświetlone przez księżyc oraz błąd wewnątrz `common/lighting.glsl` pokazany z nazwą pliku, podczas gdy poprzedni program rysował dalej.

**Mapy normalnych (Windows, 2026-10-05, ten sam sprzęt).** Skutek przełącznika jest sprawdzony na zrzutach ekranu (stanu pola nie ustawiało kliknięcie w panelu): fugi jako rowki przy włączonym mapowaniu, zrzuty trybów `Gouraud` i `Unlit` identyczne co do piksela przy włączonym i wyłączonym ([`gfx/normal-mapping.md`](gfx/normal-mapping.md), sekcja 5.11). **Pola wyboru `Normal mapping` nikt nie kliknął ręcznie i nikt nie obejrzał ręcznie ostatecznego układu domyślnego panelu Assets z tym polem i notatką na miejscu.** Rozmiar panelu w `PanelLayout.hpp` się nie zmienił (300 x 216), a treści przybyło, więc więcej jej leży poniżej dolnej krawędzi i wymaga przewinięcia.

**M5 (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Build Debug i Release bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach. Obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe wstawki w kodzie, które zostały potem usunięte: widok startowy z ośmioma panelami i HUD, ten sam widok ze schowanymi panelami (HUD zostaje), rozwinięte panele Camera i Gameplay, słaba i pusta bateria, karta wygranej, plan z kryształami, bramą i wyjściem. Stanów tych nie ustawiało klikanie w panele. O clang-format, clang-tidy, sterowniku i karcie graficznej dla M5 nic nie zostało zgłoszone, więc niczego o nich nie twierdzę. Kamień milowy ma kompletny kod na Windowsie i **nie jest zamknięty**.

**M6, część druga: teren i trawa (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Build Debug i Release bez ostrzeżeń i clang-format bez uwag. Obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe wstawki w kodzie, które zostały potem usunięte. Sam uruchomiłem wtedy zbudowane programy testowe: 256 przypadków testowych i 101232 asercje przechodziły w Debug i w Release. Z liczb, które pokazują nowe panele, przeliczyłem z kodu i z pliku mapy wysokości siatkę 97 x 97, 18432 trójkąty, zakres wysokości od 0,00 do 3,37 m i 1843 kępki. Paneli Terrain i Grass nikt nie rozwinął ręcznie, żadnej ich kontrolki nikt nie kliknął, a linii `grass.vert + grass.geom + grass.frag: OK` w panelu Shaders i przycisku `Reload shaders` przy sześciu programach nikt nie sprawdził ręcznie. Zgłoszone jest, że zepsuty `grass.geom` daje błąd z nazwą pliku (`grass.geom(84)`), a gra działa dalej. Kamień milowy M6 ma kompletny kod na Windowsie i nie jest zamknięty: macOS i testy ręczne są otwarte.

**M7, część pierwsza: bufor HDR i gamma (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Bramka `make check` przechodzi (formatowanie, testy Debug i Release, clang-tidy), zero ostrzeżeń, 269 przypadków testowych i 102103 asercje w obu konfiguracjach, a po drugiej części M7 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751. W buildzie Debug nie było błędów OpenGL przy włączonych podglądach, przy zmianie rozmiaru okna na 1400 x 800 ani przy minimalizacji (framebuffer 0 x 0) i przywróceniu. Podglądy tekstur w panelu Assets są identyczne co do piksela z podglądami sprzed zmiany. Niczego z tego nie uruchamiałem sam. Panelu Framebuffers nikt nie rozwinął kliknięciem i żadnej jego kontrolki nikt nie ruszył myszą, przycisku `Reload shaders` (wtedy przy ośmiu programach) nikt nie kliknął, a rozmiaru okna nikt nie zmieniał przeciąganiem krawędzi.

**M7, część druga: bloom (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Bramka `make check` przechodzi, zero ostrzeżeń w Debug i Release, 276 przypadków testowych i 102139 asercji w obu konfiguracjach. Na zrzutach ekranu: panel Framebuffers z czterema podglądami, widok normalnych z napisem `(not drawn)` w miejscu obrazów bloomu, okno zmienione na 1000 x 600 z celami bloomu 500 x 300. Niczego z tego nie uruchamiałem sam. Zrzuty były robione bez myszy: pola `Bloom`, suwaków `Blur iterations`, `Threshold` i `Intensity`, podpowiedzi nad obrazami ani przycisku `Reload shaders` przy dziesięciu programach nikt nie kliknął. Nie wiem też z własnego pomiaru, czy siedem kontrolek w dwóch kolumnach i cztery obrazy mieszczą się w wysokości 344 bez paska przewijania: komentarz w `PanelLayout.hpp` mówi, że tak. M7 był wtedy rozpoczęty, nie skończony: mgła i winieta doszły w części trzeciej (następny akapit), cienie księżyca w czwartej, a minimapy i cienia latarki nie ma do dziś.

**M7, część trzecia: mgła i winieta (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Bramka `make check` przechodzi (formatowanie, buildy Debug i Release, testy w obu, clang-tidy), zero ostrzeżeń, 294 przypadki testowe i 102412 asercji w obu konfiguracjach. Doszło 18 przypadków i 273 asercje, wszystkie w `tests/FogTests.cpp` (11 i 241) i `tests/VignetteTests.cpp` (7 i 32): żaden nie dotyczy panelu. Z oboma efektami wyłączonymi obraz jest identyczny co do piksela z obrazem z drugiej części, a oba widoki do szukania błędów są identyczne z widokami z drugiej części przy ustawieniach startowych, czyli z polami `Fog` i `Vignette` zaznaczonymi. Pomiar FPS z efektami i bez jest w [`renderer/post-process.md`](renderer/post-process.md), sekcja 5.9. Niczego z tego nie uruchamiałem sam. Niczego nie kliknięto myszą: paska zakładek, pól `Fog` i `Vignette`, suwaków `Density`, `Base height`, `Height falloff`, `Strength` i `Radius`, próbnika `Fog colour` ani żadnej z ośmiu nowych podpowiedzi. O zrzucie ekranu panelu z zakładkami nic nie zostało zgłoszone, więc nie wiem, czy pasek zakładek, cztery wiersze kontrolek i cztery obrazy mieszczą się w wysokości 344 bez paska przewijania: zakładki są po to (komentarz nad `drawSettings` w `FramebuffersPanel.cpp`), ale nikt tego nie zmierzył. Układ, stałe w `PanelLayout.hpp` i liczba paneli (wtedy jedenaście) zostały w tej części bez zmian. Na macOS nic z tego nie było budowane ani uruchamiane. M7 był wtedy rozpoczęty, nie skończony: minimapy i cieni nie było.

**M7, część czwarta: cienie księżyca (Windows, 2026-10-05), zgłoszone przez wykonawcę.** Bramka `make check` przechodzi (formatowanie, buildy Debug i Release, testy w obu, clang-tidy), 310 przypadków testowych i 103751 asercji. Doszło 16 przypadków i 1339 asercji, wszystkie w `tests/ShadowTests.cpp`: żaden nie dotyczy panelu. Z wyłączonymi cieniami i natężeniem księżyca cofniętym do 0,12 obraz jest poza pasem HUD identyczny co do piksela z obrazem sprzed tej części (w trybie `Phong` różni się najwyżej o 1/255). Pas HUD jest z tego porównania wyłączony: HUD stoi teraz o jeden rząd niżej. W buildzie Debug nie było błędów OpenGL przy mapie 2048 i 1024. Niczego z tego nie uruchamiałem sam. Niczego nie kliknięto myszą: paska panelu Shadows, żadnej z jego ośmiu kontrolek ani podpowiedzi, a przełączania rozdzielczości na liście `Resolution` w działającym programie nikt nie sprawdził. Nie wiem, czy zawartość mieści się w wysokości 324 bez paska przewijania, ani jak panel wygląda w oknie innego rozmiaru. Przycisku `Reload shaders` przy jedenastu programach nikt nie kliknął. Na macOS nic z tego nie było budowane ani uruchamiane. M7 jest rozpoczęty, nie skończony: cień rzuca tylko księżyc, cienia latarki i minimapy nie ma (są planowane).

**M7, część piąta: cień latarki (Windows, 2026-10-06), zgłoszone przez wykonawcę.** Bramka `make check` przechodzi, 329 przypadków testowych i 104306 asercji. Doszło 19 przypadków i 555 asercji (15 w `tests/ShadowTests.cpp`, 4 w `tests/LightingTests.cpp`): żaden nie dotyczy paneli. Uruchomienie programu Debug na 7 sekund: OpenGL 4.1.0 NVIDIA, zasoby wczytane, pusty strumień błędów, panele schowane. **Nie sprawdzono:** rysowania w trybie Gouraud, zakładki `Flashlight` panelu Shadows (obrazu mapy, napisu `(not drawn)` po zgaszeniu latarki), ścieżki ze zgaszoną latarką ani przycisku `Reload shaders`. Niczego nie kliknięto myszą: ani zakładki `Flashlight`, ani trzech nowych suwaków panelu Lights, ani żadnej z nowych podpowiedzi. Nie wiem, czy zawartość zakładki mieści się w wysokości 324 bez paska przewijania. Układ paneli, stałe w `PanelLayout.hpp` i liczba paneli (dwanaście) nie zmieniły się. Na macOS nic z tego nie było budowane ani uruchamiane.

**Samych kontrolek nikt jeszcze nie klikał.** Dotyczy to starych paneli (przyciski `Regenerate` i `Random seed`, listy `View mode` i `Filter`, suwak `Anisotropy`, pola wyboru panelu Collision, klawisz N), wszystkiego, co doszło w M4 (lista `Lighting`, każdy widżet panelu Lights, klawisz F, rozwinięcie panelu Camera, przycisk `Reload shaders`, pole wyboru `Normal mapping`), i wszystkiego z M5: klawisza R, klawisza F przy pustej baterii, przycisku `Restart round (key R)`, suwaków i pola wyboru panelu Gameplay, rozwinięcia tego panelu, HUD przy schowanych panelach, karty wygranej i migotania oglądanego na żywo. Lista do ręcznego przejścia jest w [`../guides/build-windows.md`](../guides/build-windows.md). Do tej listy dochodzi wszystko z drugiej części M6: rozwinięcie paneli Terrain i Grass, suwak `Height scale` (także przy 0 i 2,5), pole `Wireframe`, pole `Enabled`, suwaki `Density`, `Blade height` i `Wind strength` oraz przycisk `Reload shaders`. Z pierwszej części M7 dochodzą: rozwinięcie panelu Framebuffers, suwaki `Exposure` i `Depth range`, każda z trzech pozycji listy `Tone mapping`, oba podglądy, suwak `Sky brightness` w nowym zakresie i przycisk `Reload shaders` przy dziesięciu programach. Z trzeciej części M7 dochodzą: pasek zakładek panelu Framebuffers (przejście między `Tone and bloom` i `Fog and vignette`) i wszystkie osiem kontrolek drugiej zakładki. Z piątej części M7 dochodzą: zakładka `Flashlight` panelu Shadows (jej pola, lista `Resolution`, obraz z linearyzacją i napis `(not drawn)` po wciśnięciu F) i trzy suwaki panelu Lights: `Hand right`, `Hand down` i `Converge at`. Z czwartej części M7 dochodzą: rozwinięcie panelu Shadows, pola `Shadows`, `Hardware 2 x 2 filter` i `PCF`, listy `Resolution` (razem z przełączeniem rozdzielczości mapy w działającym programie) i `Kernel`, suwaki `Constant bias`, `Slope bias` i `Strength`, obraz mapy z podpowiedzią, suwaki `Moon yaw` i `Moon pitch` oglądane razem z cieniami oraz przycisk `Reload shaders` przy jedenastu programach. Na macOS nic z kodu M4, M5, M6 ani M7 nie było ani budowane, ani uruchamiane.

Zachowanie całej nakładki:

- Gdy kursor jest przechwycony przez kamerę (po kliknięciu w scenę), panele są widoczne i pokazują bieżące wartości, ale nie reagują na mysz: ani na najechanie, ani na kliknięcie (sekcja 5.6). Żeby użyć panelu, trzeba najpierw nacisnąć Escape.

- Klawisz **`~`** (grawis, grave accent, na lewo od `1`, w kodzie `GLFW_KEY_GRAVE_ACCENT`) chowa i pokazuje wszystkie panele (start: widoczne, `m_visible = true`). HUD gry nie jest panelem i zostaje na ekranie (sekcja 5.9). PRD ([`../PRD.pdf`](../PRD.pdf)) nadal podaje w tym miejscu pierwszy klawisz funkcyjny (z górnego rzędu klawiatury). Klawisz został zmieniony celowo i obowiązuje to, co jest w kodzie.
- Gdy aktywny jest widżet panelu (edycja pola, przeciąganie wartości), gra nie widzi klawiatury: Escape nie zamyka programu, `~` nie chowa paneli, N nie przełącza noclip, F nie przełącza latarki, a R nie zaczyna nowej rundy (sekcja 5.6). Dotyczy to na przykład wpisywania ziarna w panelu Maze. Przy wolnym kursorze i bez aktywnego widżetu N, F i R działają.
- Panel można przeciągnąć za pasek tytułu i **zadokować** do krawędzi okna. Środek zostaje przezroczysty dzięki `PassthruCentralNode`.
- Układ paneli ImGui zapisuje w pliku `imgui.ini` w **katalogu roboczym** programu. Plik jest w `.gitignore`, bo to ustawienie lokalne. Skasowanie go przywraca układ domyślny: dwanaście paneli w miejscach z tabeli w sekcji 5.7, z panelami Camera, Gameplay, Terrain, Grass, Framebuffers i Shadows zwiniętymi. HUD nie ma wpisu w tym pliku (`NoSavedSettings`). W M1 panele nie miały pozycji startowych i przy pierwszym uruchomieniu otwierały się jeden na drugim. Stary `imgui.ini` nadal trzyma panele w starych miejscach i rozmiarach: plik sprzed M4 stawia panel Camera pod panelem Lights, a plik z M4 trzyma dolny rząd o 8 jednostek niższy niż dziś, więc po przejściu na nowszą wersję najprościej go skasować. Plik sprzed czwartej części M7 nie zna panelu Shadows: ten jeden panel dostaje miejsce z kodu (zwinięty pasek w czwartym rzędzie), a pozostałe zostają tam, gdzie zapisał je plik.
- W `imgui.ini` jest też stan zwinięcia każdego panelu (linia `Collapsed=`). Panel Camera, Gameplay, Terrain, Grass, Framebuffers albo Shadows rozwinięty raz zostaje rozwinięty przy następnych uruchomieniach (dla Framebuffers i Shadows znaczy to też, że gra od startu rysuje ich podglądy), a każdy inny panel zwinięty ręcznie zostaje zwinięty. Stan czterech zwijanych grup wewnątrz panelu Lights do pliku **nie** trafia: we wpisie okna są tylko pozycja, rozmiar, zwinięcie i dokowanie. Po każdym starcie grupy wracają więc do stanu z kodu: `Moon (directional)` zwinięta, trzy pozostałe rozwinięte.
- Wygląd paneli (kolory, odstępy, czcionka) nie jest zapisywany w `imgui.ini`. Ustawia go kod przy każdym starcie (sekcja 5.8).

## 7. Pułapki

1. **Scena zniknęła pod szarym tłem.** Brak flagi `ImGuiDockNodeFlags_PassthruCentralNode` w `DockSpaceOverViewport`: pusty węzeł centralny jest zamalowany i zasłania scenę.
2. **Shadery ImGui się nie kompilują, paneli nie widać.** Zła wersja GLSL w `ImGui_ImplOpenGL3_Init`. Dla kontekstu 4.1 Core ma być `"#version 410"`.
3. **`Begin` bez `End`.** `ImGui::End()` wstawione do środka `if (ImGui::Begin(...))` powoduje asercję po zwinięciu panelu. `End` zawsze poza `if`.
4. **Widżety poza klatką.** Każde `ImGui::...` rysujące coś musi być między `ImGui::NewFrame()` a `ImGui::Render()`. Dlatego panele wołam tylko z `DebugUI::draw`.
5. **Kolejność niszczenia.** `DebugUI` zniszczone po oknie woła OpenGL i GLFW bez kontekstu. Pole `m_debugUI` musi pozostać polem klasy pochodnej od `core::Application` (dziś `DebugNightMazeApp`), bo pola giną przed klasami bazowymi (zob. [`core/README.md`](core/README.md), sekcja 7).
6. **Własny callback GLFW ustawiony po utworzeniu `DebugUI`.** `glfwSetKeyCallback` i pokrewne **podmieniają** callback zainstalowany przez backend, więc ImGui przestaje dostawać dany rodzaj zdarzeń. Własne callbacki trzeba ustawić przed konstruktorem `DebugUI` (wtedy backend je połączy w łańcuch) albo samemu wołać poprzedni callback zwrócony przez `glfwSet...Callback`.
7. **Gra i ImGui reagowałyby na tę samą mysz.** Bez linii `input().setMouseBlocked(m_debugUI.wantsMouse());` każdy kod gry pytający `core::Input` o mysz widziałby też kliknięcia i ruch przeznaczone dla panelu: przeciąganie suwaka obracałoby jednocześnie kamerę, a kliknięcie w panel byłoby kliknięciem w scenę. Flaga blokady myszy temu zapobiega (sekcja 5.6). Warunek jest jeden: gra musi pytać o mysz przez `input()`, a nie bezpośrednio przez GLFW.
8. **`imgui.ini` zależy od katalogu roboczego.** Uruchomienie z IDE i z terminala może dać dwa różne układy, bo plik ląduje w innym katalogu. Ten sam plik sprawia, że miejsca z `PanelLayout.hpp` "nie działają": `ImGuiCond_FirstUseEver` dotyczy tylko okna, którego w pliku jeszcze nie ma (sekcja 5.7).
9. **Małe panele na Windowsie przy skalowaniu 150% lub 200%.** Na macOS skalę Retiny obsługuje para rozmiar okna i framebuffer. Na Windowsie framebuffer i okno mają ten sam rozmiar w pikselach, więc czcionka ImGui pozostaje mała, dopóki sam jej nie przeskaluję. Robi to `applyTheme` (sekcja 5.8.4). Zostają dwa ograniczenia: okno gry nadal startuje jako 1280 x 720 pikseli, więc przy skali 150% powiększona zawartość nie mieści się w panelach bez powiększenia okna (sekcja 5.7), a skala jest czytana tylko raz, przy starcie.
10. **Błąd OpenGL przypisany nie temu, kto zawinił.** Backend ImGui nie używa mojego `GL_CHECK`. Gdyby zostawił flagę błędu, zgłosi ją pierwszy `GL_CHECK` w następnej klatce (zwykle `glViewport`).

11. **"Klawisz `~` nie chowa paneli".** Aktywny widżet ImGui blokuje klawiaturę gry, więc przełącznik nie reaguje, dopóki trwa edycja albo przeciąganie. To zamierzone. Wystarczy zakończyć edycję (Enter, Escape albo kliknięcie poza polem).
12. **`setKeyboardBlocked` albo `setMouseBlocked` zapomniane w nowym programie.** Blokady nie są częścią `DebugUI::draw`, tylko osobnymi liniami w `main.cpp`. Program, który posiada `DebugUI`, ale nie przekazuje `wantsKeyboard()` i `wantsMouse()` do `core::Input`, wraca do starego zachowania: Escape w polu tekstowym zamyka program, a gra widzi mysz używaną przez panel. To samo dotyczy `setMouseEnabled`: program, który przechwytuje kursor, musi wołać ją sam przed `draw` (pułapka 13).
13. **Przechwycony kursor nie odcina ImGui od myszy.** W trybie `GLFW_CURSOR_DISABLED` backend GLFW nie zmienia kształtu kursora, ale pozycję kursora (wtedy wirtualną) nadal przekazuje do ImGui. Niewidoczny kursor może więc znaleźć się nad panelem, ustawić `WantCaptureMouse` i zamrozić obrót kamery albo kliknąć niewidoczny widżet. Rozwiązanie jest w kodzie: `main.cpp` woła `m_debugUI.setMouseEnabled(!input().isCursorCaptured());` przed `draw`, co ustawia flagę `ImGuiConfigFlags_NoMouse`, a `wantsMouse()` przy tej fladze zwraca `false` (sekcja 5.6). Sama flaga bez tego drugiego warunku nie wystarcza: ImGui nadal zgłasza `WantCaptureMouse`, dopóki trzymany jest przycisk wciśnięty "nad" panelem.
14. **Nowe pole w `DebugContext` bez linii w `main.cpp`.** Pole jest referencją, a referencja musi być zainicjalizowana. Pominięcie go w `debug::DebugContext{...}` kończy się błędem kompilacji (clang: `reference member of type ... uninitialized`). To zamierzona ochrona: nie da się zapomnieć o podaniu danych. Zadziała tylko dla pól referencyjnych. Pole będące zwykłą wartością (na przykład `bool`) pominięte na liście dostałoby po cichu zero. Od M6 jest w strukturze jedno takie pole: `grassTuftCount` typu `std::size_t`. Gdyby linia `.grassTuftCount = grassTuftCount(),` zniknęła z `main.cpp`, kod by się skompilował, a panel Grass pokazywałby `Tufts: 0 (0 blades)` przy trawie widocznej w scenie. Do M6 pole stało na końcu struktury i kompilatory zwykle ostrzegały o brakującym inicjalizatorze. Od pierwszej części M7 stoją za nim cztery pola (trzy referencje i jedna referencja `const`), więc pominięta linia leżałaby w środku listy: na ostrzeżenie nie liczę, bo to i tak ostrzeżenie, nie błąd.
15. **Kolejność inicjalizatorów desygnowanych inna niż kolejność pól.** C++20 wymaga, żeby desygnatory szły w kolejności deklaracji pól (inaczej niż w C99). Kompilatory traktują to różnie: Apple clang z flagami tego projektu (`-Wall -Wextra -Wpedantic`) przyjmuje złą kolejność bez słowa (ostrzega dopiero z `-Wreorder-init-list`), a GCC i MSVC zgłaszają błąd. Kod, który buduje się na Macu, może więc nie zbudować się na Windowsie. Linie w `main.cpp` piszę zawsze w kolejności pól z `DebugContext.hpp`.
16. **`DebugContext` zachowany na później.** Struktura zapisana w polu klasy albo w zmiennej żyjącej dłużej niż klatka trzyma referencje do obiektów, które mogą już nie istnieć (wiszące referencje, dangling references). Kontekst buduję co klatkę i używam go tylko w trakcie `draw`.
17. **"Przecież `context` jest `const`, a panel coś zmienia".** To poprawne i zamierzone: `const` na strukturze nie przechodzi przez pole referencyjne (sekcja 5.2). Chcąc zabronić edycji, zmieniam typ pola na `const ...&`, a nie sposób przekazania struktury.
18. **Panel, którego przycisk wykonuje wywołania OpenGL.** Przycisk "Reload shaders" woła `Shader::reload()` między `ImGui::NewFrame()` a `ImGui::Render()`, czyli w środku klatki ImGui. To bezpieczne, bo ImGui w tym czasie nie woła OpenGL, a backend w `RenderDrawData` sam ustawia swój stan i nie przywraca programu, który został usunięty ([`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6.3). Nie wynika z tego, że panel może robić w OpenGL cokolwiek: funkcja wołana z panelu nie powinna zostawiać zmienionych powiązań (programu, VAO, buforów, tekstur), a sam panel nadal nie woła `gl*` bezpośrednio.
19. **Panel dodany bez pozycji startowej.** Bez wywołania `placePanelOnFirstUse` nowy panel otwiera się w domyślnym miejscu ImGui i przykrywa panel Renderer albo scenę. Miejsce trzeba też dobrać tak, żeby nie nachodziło na pozostałe (tabela w sekcji 5.7), licząc panele Camera i Gameplay jako rozwinięte. Od M5 w oknie 1280 x 720 nie ma już wolnego prostokąta, więc dziewiąty panel wymaga przeliczenia układu (sekcja 5.5, krok 2).
20. **Panel, który sam zmienia stan gry w środku klatki.** Panel Maze nie woła `buildMazeWorld`. Ustawia flagę w `MazeSettings`, a gra wymienia labirynt na początku następnej klatki. Gdyby panel robił to sam, scena tej klatki byłaby już narysowana ze starego labiryntu, a panele Maze i Collision rysowane po nim pokazywałyby nowy. Ten sam wzór ma przycisk `Restart round (key R)` panelu Gameplay: ustawia flagę `GameplaySettings::restart`, a rundę zaczyna gra.
21. **Pole `const` w kontekście, a panel chce pisać.** `mazeWorld` jest `const game::MazeWorld&`. Próba zmiany labiryntu z panelu nie skompiluje się i tak ma być: jedyną drogą do nowego labiryntu jest `mazeSettings`.
22. **Bajty czcionki zwolnione dwa razy albo za wcześnie.** `AddFontFromMemoryTTF` domyślnie przejmuje wskaźnik i sam go zwalnia. Podanie mu `vector.data()` bez `config.FontDataOwnedByAtlas = false` kończy się zwolnieniem tej samej pamięci przez ImGui i przez wektor. Z `false` jest odwrotne ryzyko: wektor lokalny w funkcji zniknąłby przed atlasem, a ImGui 1.92 czyta dane czcionki przez cały czas działania (rysuje znaki dopiero przy pierwszym użyciu). Dlatego bajty są polem `DebugUI` (sekcja 5.8.5).
23. **`AddFontFromFileTTF` i brak pliku.** Ta funkcja przy braku pliku domyślnie wywołuje asercję ImGui, która w buildzie Debug zatrzymuje program. Flaga, która to wyłącza (`ImFontFlags_NoLoadError`), nie jest jeszcze częścią publicznego API. Stąd własne czytanie pliku i `AddFontFromMemoryTTF`.
24. **Kod czcionek ze starego poradnika.** Przed ImGui 1.92 podawało się rozmiar w pikselach i zakresy znaków przy wczytywaniu, a skalę ustawiało polem `io.FontGlobalScale`. W 1.92 rozmiar to `style.FontSizeBase`, skala to `style.FontScaleMain` i `style.FontScaleDpi`, a zakresy są niepotrzebne. Część starych nazw nadal istnieje jako przestarzałe, więc stary kod może się skompilować, ale omija nowy mechanizm.
25. **Jasne tło "pod kursorem".** Naturalny pomysł na ciepły akcent to jasny bursztyn pod kursorem. Tekst ma jednak jeden kolor (`ImGuiCol_Text`), więc na jasnym przycisku byłby nieczytelny. Jasny `AMBER` trafia tylko tam, gdzie nie ma tekstu, a tła stanów są ciemne (`EMBER`, `EMBER_BRIGHT`).
26. **`ScaleAllSizes` wołane drugi raz.** Funkcja mnoży bieżące wartości i zaokrągla je w dół, więc dwa wywołania ze skalą 1,5 dają skalę 2,25 z błędami zaokrągleń. Przy zmianie skali trzeba najpierw ustawić metryki od nowa (`applyMetrics`), a dopiero potem skalować.
27. **Skala z `glfwGetWindowContentScale` na Macu.** Na Retinie zwraca 2, a ImGui i tak rysuje w gęstszych pikselach framebuffera. Użyta jako skala motywu powiększyłaby panele dwukrotnie. Funkcja backendu `ImGui_ImplGlfw_GetContentScaleForWindow` zwraca na platformach Apple 1.
28. **Polska litera wpisana wprost w kodzie panelu.** ImGui oczekuje tekstu w UTF-8, a to, jakie bajty kompilator zapisze dla literału z polskimi literami, zależy od kodowania pliku i od strony kodowej komputera. MSVC bez opcji `/utf-8` (projekt jej nie ustawia) czyta plik bez znacznika BOM w lokalnej stronie kodowej. Zmierzyłem to małym programem poza repozytorium (MSVC 19.44, strona kodowa 1250, plik w UTF-8 bez BOM): zwykły literał `"Zażółć gęślą jaźń ĄĆĘŁŃÓŚŹŻ"` zachował bajty pliku bez zmian (45 bajtów, poprawne UTF-8), czyli działałby, ale tylko dlatego, że kompilator niczego nie przeliczał. Literał `u8"..."` z tymi samymi literami wyszedł zepsuty: 90 bajtów, bo każdy bajt UTF-8 został potraktowany jako osobna litera strony 1250 i zakodowany jeszcze raz. Na komputerze z inną stroną kodową wynik może być inny. Pewne są dwie drogi: literał `u8` z kodami znaków (`\u017C` zamiast `ż`), którego użyłem w tymczasowym napisie testowym i który wyświetlił się poprawnie, tak jak w teście loadera obrazów ([`assets/images.md`](assets/images.md), sekcja 5.7), albo opcja `/utf-8` dla całego projektu. Wszystkie napisy paneli są dziś po angielsku, w ASCII, a polskie litery pojawiają się tylko w ścieżkach plików, które `core::pathText` oddaje w UTF-8.
29. **Nazwa stałej, która jest makrem Windowsa.** Kolor "nic" (alfa 0) prosi się o nazwę `TRANSPARENT`. Nagłówki Windowsa definiują jednak makro o tej nazwie (`#define TRANSPARENT 1` w `wingdi.h`), a makro podmienia tekst, zanim kompilator zobaczy deklarację. `Theme.cpp` tych nagłówków nie dołącza, ale stała o takiej nazwie byłaby pułapką dla pierwszego pliku, który je dołączy. Dlatego stała nazywa się `INVISIBLE`.
30. **Lista `Combo` w innej kolejności niż wyliczenie.** Napis `LIGHTING_MODE_ITEMS` i `enum class game::LightingMode` to dwa miejsca, które muszą mieć tę samą kolejność, bo numer pozycji jest rzutowany wprost na wartość wyliczenia (sekcja 5.3). Nowy tryb dopisany tylko w jednym z nich przesuwa wszystkie następne: panel pokazuje `Phong`, a gra rysuje czymś innym. Kompilator tego nie widzi. Brak ostatniego `\0` w napisie też się skompiluje: ImGui szuka końca listy po dwóch zerach z rzędu, więc po ostatniej pozycji czytałoby wtedy bajty spoza napisu.
31. **"Panel Camera zniknął", "nie ma panelu Gameplay" albo "Lights leży na Camera".** Pierwsze dwa to zwinięte panele: ich paski tytułów stoją obok siebie przy górnej krawędzi, na prawo od panelu Renderer. Drugie to stary `imgui.ini` z wpisem panelu Camera w lewej kolumnie (sekcja 5.7). We wszystkich trzech wypadkach kod działa poprawnie.
32. **Panel Lights "zapomina", że grupa była rozwinięta.** Stan nagłówków `CollapsingHeader` żyje tylko w pamięci ImGui do końca działania programu. Flaga `ImGuiTreeNodeFlags_DefaultOpen` (albo jej brak) ustala stan przy każdym starcie od nowa.
33. **Zmiana na liście `Lighting` "nic nie robi".** Przy widoku `Normals as colour` albo `UVs as colour` z panelu Assets labirynt (z bramą i kryształami) jest zawsze rysowany programem `textured`, bez oświetlenia. Trzeba wrócić do widoku `Textured`. Wyjątek: widok normalnych pokazuje normalne siatki w trybie `Gouraud`, a normalne z map w pozostałych trybach (przy zaznaczonym `Normal mapping`).
34. **Pole `Normal mapping` "nic nie robi".** W trybie `Gouraud` i w trybie `Unlit` przy widoku `Textured` obraz się nie zmienia: pierwszy liczy światło w wierzchołkach, drugi nie ma światła wcale. Efekt widać w trybach `Phong` i `Blinn-Phong` oraz w widoku `Normals as colour`. Mówi to notatka pod polem.
35. **Pasek HUD "zniknął".** Leży pod rozwiniętym panelem Camera albo Gameplay. To zamierzone: okno paska ma flagę `ImGuiWindowFlags_NoBringToFrontOnFocus` i nigdy nie przechodzi przed panel (sekcja 5.9.5). Wystarczy zwinąć panel albo schować panele klawiszem `~`. Stan rundy widać też w panelu Gameplay.
36. **Kontrolek nieba "nie ma".** Stary plik `imgui.ini` trzyma panel Renderer w wysokości sprzed pierwszej części M6 (230), a pole `Skybox` i suwak `Sky brightness` leżą pod jego dolną krawędzią. Układ z kodu (`RENDERER_HEIGHT = 284.0F`) działa tylko przy pierwszym uruchomieniu bez wpisu w pliku. Lekarstwo: przewinąć panel, powiększyć go albo usunąć `imgui.ini`.
37. **`Clear color` "nic nie robi".** Przy zaznaczonym polu `Skybox` niebo zamalowuje każdy piksel, na którym nie ma ściany, więc koloru czyszczenia nie widać. Kontrolka działa: wystarczy odznaczyć `Skybox`.
38. **Suwaki `Moon` nie ruszają księżyca na niebie.** Tarcza jest częścią obrazu nieba i stoi w domyślnym kierunku światła. Suwaki `Moon yaw` i `Moon pitch` panelu Lights zmieniają światło na ścianach i, od czwartej części M7, kierunek cieni, ale nie obraz nieba ([`renderer/skybox.md`](renderer/skybox.md), sekcja 2.9).
36. **Karta `You escaped` jest zasłonięta albo nie widać jej wcale.** Karta nie ma flagi z pułapki 35, ale ma `NoInputs`, więc nie da się jej kliknąć i wyciągnąć na wierzch. Panel kliknięty po jej pojawieniu się przechodzi przed nią. Z kodu ImGui wynika też przypadek, którego nikt nie oglądał na ekranie: przy drugiej wygranej w tym samym uruchomieniu karta może pojawić się od razu pod panelem klikniętym po pierwszej (sekcja 5.9.5). Runda jest wtedy wygrana mimo niewidocznej karty: mówi o tym linia `Round: won` w panelu Gameplay, a klawisz R działa.
37. **"Klawisz `~` nie chowa licznika kryształów".** I nie ma chować: HUD należy do gry, a nie do narzędzi, więc `drawHud` stoi poza `if (m_visible)`. Kto chce zrzut ekranu samej sceny, musi tymczasowo zakomentować wywołanie `drawHud` w `DebugUI::draw`.
38. **Pasek HUD nachodzi na paski tytułów paneli po dodaniu kolejnego rzędu.** Do M5 odległość HUD od góry była ręcznie dobraną liczbą 46 i po każdej zmianie czcionki trzeba ją było poprawiać. Od M6 ta część kłopotu zniknęła: `Hud.cpp` liczy rzędy pasków funkcją `foldedRowsHeight`, która pyta ImGui o prawdziwą wysokość paska, więc zmiana `FONT_SIZE`, `FRAME_PADDING` albo `PANEL_GAP` przesuwa HUD sama (sekcja 5.9.5). Została druga część: **liczba** rzędów to stała `FOLDED_ROW_COUNT` w `PanelLayout.hpp`, której nic nie liczy z pól `foldedRowsBefore`. Panel w nowym rzędzie bez podniesienia `FOLDED_ROW_COUNT` stanie dokładnie tam, gdzie zaczyna się HUD. W pierwszej części M7 obie rzeczy zmieniono razem: panel Framebuffers ma `.foldedRowsBefore = 2`, a `FOLDED_ROW_COUNT` wyniosło 3. W czwartej części M7 tak samo: panel Shadows ma `.foldedRowsBefore = 3`, a `FOLDED_ROW_COUNT` wynosi 4. Piąty rząd (`.foldedRowsBefore = 4`) wymagałby stałej równej 5. Paskowi to nie szkodzi (to panele zasłaniają jego, nie on je), ale górna linia HUD chowa się wtedy pod paskiem tytułu.
39. **Przycisk dodany do HUD nie reaguje.** Okna HUD mają `ImGuiWindowFlags_NoInputs`: mysz przez nie przechodzi, więc żaden widżet w nich nie zostanie najechany ani kliknięty. Kontrolka, w którą da się kliknąć, należy do panelu (tak powstał przycisk `Restart round (key R)` w panelu Gameplay), a w HUD jest tylko napis z nazwą klawisza.
40. **`game::Round&` bez `const` w panelu Gameplay to nie zaproszenie.** Panel dostaje rundę do zapisu z jednego powodu, suwaka `Battery`. Kompilator pozwoli mu zmienić także licznik kryształów albo stan rundy, ale gra liczy je sama w każdym kroku (`game::updateRound`) i liczby zapisane z panelu rozeszłyby się z listą kryształów. Podział na funkcje w `GameplayPanel.cpp` pilnuje tego ręcznie: wszystko poza baterią wypisuje `drawRoundState(const game::Round& round)`.
41. **Nowych paneli Terrain i Grass "nie ma".** Są, ale startują zwinięte: to dwa paski tytułów w drugim rzędzie przy górnej krawędzi, pod paskami Camera i Gameplay. Jeśli w starym `imgui.ini` panel Camera albo Gameplay jest zapisany jako rozwinięty, zakrywa pasek pod sobą. Trzeba zwinąć panel nad nim, odsunąć go albo skasować `imgui.ini`.
42. **Rozwinięty panel Camera zakrywa pasek Terrain (a Gameplay pasek Grass).** To nie błąd rachunku, tylko świadomy wyjątek: komentarz w `PanelLayout.hpp` mówi, że prostokąty się nie nakładają, gdy sześć zwiniętych paneli liczyć jako paski (do trzeciej części M7 pięć). Panele Terrain i Grass rozwinięte **razem** z panelem nad nimi leżą jeden na drugim, a na wierzchu jest ten kliknięty ostatnio. Od pierwszej części M7 to samo dotyczy panelu Framebuffers: jego pasek leży pod wszystkimi czterema, a rozwinięty zajmuje prostokąt, w którym rozwijają się pozostałe. Od czwartej części M7 dochodzi panel Shadows: jego pasek leży pod wszystkimi pięcioma (rozwinięty Framebuffers zakrywa go w całości), a rozwinięty zajmuje ten sam prostokąt środka.
43. **Suwak `Height scale` przestawia świat, a nie tylko obraz.** Skala wysokości nie jest uniformem: każda zmiana buduje teren od nowa na procesorze i przestawia ściany, bramę, kryształy, pudełka kolizji i gracza (`rebuildTerrain`). Przeciąganie suwaka robi to w każdej klatce. Dla labiryntu startowego to 9409 wierzchołków na klatkę, co nie boli, ale przy labiryncie 40 x 40 siatka ma 217 x 217 punktów i 93312 trójkąty, więc przeciąganie może być wyraźnie wolniejsze. Tego nikt nie mierzył.
44. **`Blade height` i `Wind strength` nie zmieniają liczby `Tufts`.** I nie mają: to uniformy shadera geometrii, a nie dane. Liczbę kępek zmienia tylko `Density` (i nowy labirynt albo nowa skala wysokości, po których trawa jest sadzona od nowa w tych samych miejscach na planie, na nowej wysokości).
45. **Liczba źdźbeł w panelu Grass kłamie po zmianie shadera.** `BLADES_PER_TUFT` w `GrassPanel.cpp` jest ręczną kopią `BLADE_COUNT` z `grass.geom`. Po zmianie liczby źdźbeł w shaderze (i `Reload shaders`) obraz się zmieni, a napis nie. Trzeba poprawić obie stałe i zbudować program.
46. **Linia `Height:` w panelu Terrain to nie zakres wysokości pod labiryntem.** `minHeight()` i `maxHeight()` obejmują całą siatkę, ze wzgórzami na marginesie. Przy skali 1 panel pokazuje do 3,37 m, a gracz w labiryncie chodzi po gruncie od 0,085 do 0,461 m.
47. **`glBindSampler` zawołane wprost w kodzie panelu.** Nic nie zmienia w podglądzie: ImGui rysuje dopiero na końcu klatki, a wtedy backend ustawia własny sampler. Zmiana stanu OpenGL dla jednego obrazu ImGui musi być poleceniem w liście rysowania (`ImDrawList::AddCallback`), jak w `RawTextureSampler::begin` (sekcja 5.11.5).
48. **`AddCallback` ze wskaźnikiem na zmienną lokalną i bez rozmiaru.** Bez trzeciego argumentu ImGui zapamiętuje sam wskaźnik, a wywołanie zwrotne biegnie długo po tym, jak zmienna lokalna przestała istnieć: odczyt śmieci zamiast identyfikatora samplera. Z rozmiarem ImGui kopiuje bajty do własnego bufora.
49. **`begin` bez `end`.** Mój sampler zostałby na jednostce 0 dla wszystkiego, co ImGui rysuje później w tej klatce. Skutek byłby mało widoczny (filtr i zawijanie są takie same, a parametr dekodowania nie działa na tekstury, które nie są sRGB), ale każda następna tekstura sRGB pokazana przez ImGui też byłaby czytana bez dekodowania. Para ma być zawsze kompletna.
50. **Stała rozszerzenia użyta bez sprawdzenia rozszerzenia.** `GL_TEXTURE_SRGB_DECODE_EXT` to liczba wpisana ręcznie. Sterownik, który rozszerzenia nie ma, odpowie na nią błędem `GL_INVALID_ENUM`. Dlatego konstruktor najpierw pyta `gfx::hasExtension`.
51. **Podglądy tekstur ciemniejsze niż pliki.** To objaw braku `GL_EXT_texture_sRGB_decode`: `isSupported()` zwraca fałsz i panel Assets pokazuje wartości liniowe. Scena jest przy tym poprawna, bo ją koduje przebieg składający. Nikt tego jeszcze nie widział na prawdziwej maszynie, opis wynika z kodu.
52. **Podglądy w panelu Framebuffers do góry nogami.** Tekstura framebuffera ma wiersz `v = 0` na dole, jak wszystko, co rysuje OpenGL, a ImGui liczy `uv` od lewego górnego rogu. Panel podaje więc `uv0 = (0, 1)` i `uv1 = (1, 0)`. Z wartościami domyślnymi `ImGui::Image` obraz byłby odwrócony ([`renderer/post-process.md`](renderer/post-process.md)).
53. **Panel Framebuffers zostawiony rozwinięty kosztuje.** Dopóki jest otwarty, gra rysuje w każdej klatce dwa dodatkowe przebiegi podglądu. Stan zwinięcia trafia do `imgui.ini`, więc panel rozwinięty raz jest rozwinięty także przy następnym uruchomieniu. Przy pomiarze FPS panele chowam klawiszem `~` albo zwijam ten panel.
54. **Kolor z próbnika wysłany do OpenGL bez przeliczenia.** Od pierwszej części M7 próbnik (`ColorEdit3`) pokazuje wartości sRGB, a scena liczy na liniowych. Nowa kontrolka koloru potrzebuje jednego `gfx::srgbToLinear` po stronie gry, tak jak `Clear color` i kolory panelu Lights ([`gfx/color-space.md`](gfx/color-space.md)). Trzecia część M7 dodała taką kontrolkę, próbnik `Fog colour` w panelu Framebuffers, i trzyma się tej reguły: `PostProcess::composite` przelicza `FogSettings::color` raz na klatkę, zanim wyśle go jako `uFogColor`.
55. **W panelu Shadows zamiast obrazu stoi `(not drawn)`.** Pole `Shadows` jest odznaczone (albo, w zakładce `Flashlight`, latarka jest zgaszona klawiszem F lub pustą baterią): gra nie rysuje wtedy ani mapy cieni, ani jej obrazu, a linia pod kreską mówi `Map: not drawn`. To nie to samo co `(no picture yet)`, które stoi przez jedną klatkę po otwarciu panelu i znika samo.
56. **Tekstura głębi podana wprost do `ImGui::Image`.** Obraz wychodzi czerwony: tekstura głębi ma dane tylko w kanale czerwonym, a ImGui rysuje ją jak teksturę koloru. Dlatego panel Shadows pokazuje teksturę `GL_RGBA8` z osobnego małego przebiegu (`ShadowMap::drawPreview`), a samej mapy cieni nigdy nie podaje do ImGui (sekcja 6).
57. **Panel Shadows zostawiony rozwinięty kosztuje jeden przebieg na klatkę.** To samo co w pułapce 53: dopóki panel jest otwarty, gra rysuje obraz podglądu mapy pokazanej zakładki (od piątej części M7 tylko jednej z dwóch), a stan rozwinięcia trafia do `imgui.ini`. Przy pomiarze FPS panele chowam klawiszem `~` albo zwijam ten panel. Zwinięcie panelu nie wyłącza cieni: do tego służy pole `Shadows`.
58. **Lista `Kernel` "nic nie robi".** Pole `PCF` obok niej jest odznaczone. Lista nie jest wtedy wyszarzona, ale shader dostaje promień 0 i robi jedno porównanie (`game::pcfRadiusInUse`).

## 8. Ćwiczenia

1. **Nowy panel.** Wykonaj kroki z sekcji 5.5 i dodaj panel "Timing". Zadokuj go pod panelem Renderer, zamknij program i sprawdź w `imgui.ini`, co zostało zapisane.
2. **Bez `PassthruCentralNode`.** Zamień flagę na `ImGuiDockNodeFlags_None`, odznacz pole `Skybox` (z niebem koloru czyszczenia nie widać), ustaw jaskrawy `Clear color` i zobacz, co dzieje się ze środkiem okna. Wyjaśnij, czym jest węzeł centralny.
3. **Przełącznik.** Dodaj do `game::NightMazeApp` pole `bool` z chronionym akcesorem, doprowadź je przez nowe pole `DebugContext` do panelu Renderer (krok 6 z sekcji 5.5) jako `ImGui::Checkbox` i użyj go w `NightMazeApp::onRender`, na przykład do pominięcia `glClear`. Zaobserwuj, co zostaje na ekranie, gdy bufor nie jest czyszczony, a panel się porusza.
4. **Demo ImGui.** W `DebugUI::draw`, wewnątrz `if (m_visible)`, dopisz tymczasowo `ImGui::ShowDemoWindow();`. Aby się zlinkowało, dodaj `${imgui_SOURCE_DIR}/imgui_demo.cpp` do celu `imgui` w `cmake/Dependencies.cmake`. Przejrzyj dostępne widżety, a potem wycofaj obie zmiany.

5. **Kto ma klawiaturę.** Kliknij z wciśniętym Ctrl w jedną ze składowych `Clear color`, żeby przejść w tryb wpisywania, i naciśnij kolejno `~` oraz Escape. Zapisz, co się stało z panelami, z polem i z programem. Potem zakomentuj w `main.cpp` linię `input().setKeyboardBlocked(m_debugUI.wantsKeyboard());`, zbuduj i powtórz. Wyjaśnij różnicę, wskazując, w której funkcji `core::Input` zapada decyzja. Przywróć linię.
6. **Kto ma mysz.** Na końcu `DebugNightMazeApp::onRender` w `main.cpp` dopisz tymczasowo (z nagłówkiem `"core/Log.hpp"`, który jest już dołączony) `if (m_debugUI.wantsMouse()) { core::logInfo("ImGui has the mouse"); }`. Przesuwaj kursor nad panelem Renderer, nad pustym środkiem okna i zacznij przeciągać wartość `Clear color`, wyjeżdżając kursorem poza panel. Zapisz, kiedy komunikat się pojawia. Wycofaj zmianę.
7. **Kontekst pod lupą.** W `main.cpp` usuń tymczasowo linię `.clearColor = clearColor(),` z `debug::DebugContext{...}`, zbuduj i przeczytaj błąd kompilatora. Przywróć linię, a potem zamień miejscami linie `.window` i `.clearColor` i zbuduj ponownie: zapisz, czy twój kompilator to zgłosił (sekcja 7, pułapka 15). Na koniec w `DebugContext.hpp` dopisz tymczasowo `const` do pola `clearColor` (`const std::array<float, 3>& clearColor;`) i sprawdź, w której linii którego pliku kompilacja się zatrzymuje. Wyjaśnij dlaczego akurat tam. Wycofaj wszystkie zmiany.
8. **Przechwycony kursor bez `setMouseEnabled`.** Zakomentuj w `main.cpp` linię `m_debugUI.setMouseEnabled(!input().isCursorCaptured());`, zbuduj, rozwiń panel Camera strzałką w jego pasku tytułu i zadokuj go przy prawej krawędzi okna, kliknij w scenę i kręć myszą powoli w prawo. Kiedy obrót się zatrzymuje? Co się dzieje, gdy wtedy klikniesz? Przywróć linię, a zamiast tego usuń warunek z `DebugUI::wantsMouse()` (zostaw samo `return io.WantCaptureMouse;`) i sprawdź, co dzieje się z obrotem, gdy podczas sterowania kamerą przytrzymasz lewy przycisk myszy. Wyjaśnij oba wyniki punktami z sekcji 5.6 i wycofaj zmiany.
9. **Nowa kontrolka kamery.** Dodaj do panelu Camera przycisk `Reset view`, który przywraca yaw 0, pitch 0 i domyślne FOV. Nie potrzebujesz nowego pola w `DebugContext`: dlaczego? Zrób to bez gołych liczb w panelu (podpowiedź: `camera = scene::Camera{};` przywraca wszystkie wartości domyślne struktury). Ta linia zmienia też `camera.position` na `(0, 0, 3)`. Dlaczego gracz mimo to zostaje na miejscu, a pozycja kamery wraca do jego oczu najpóźniej po jednym kroku symulacji (podpowiedź: ostatnia linia `NightMazeApp::onUpdate`)?
10. **Układ domyślny.** Zamknij program, skasuj `imgui.ini` z katalogu, z którego go uruchamiasz, i uruchom ponownie. Porównaj pozycje paneli z tabelą w sekcji 5.7. Potem przesuń panel Maze, zamknij program i znajdź w nowym `imgui.ini` wpis `[Window][Maze]`.
11. **Kontrast na kartce.** Zmień w `Theme.cpp` stałą `EMBER` na jasny bursztyn `colorFromBytes(255, 184, 84)`, zbuduj i najedź kursorem na przycisk `Regenerate`. Policz wzorem z sekcji 5.8.2 kontrast tekstu `MOONLIGHT` (luminancja 0,817) na tym tle (luminancja 0,562) i porównaj z progiem 7. Wyjaśnij, dlaczego nie da się tego naprawić zmianą koloru tekstu tylko dla przycisku pod kursorem. Wycofaj zmianę.
12. **Program bez czcionki.** Zmień nazwę pliku `AtkinsonHyperlegible-Regular.ttf` w katalogu `assets/fonts` **obok programu** (na Windowsie w kopii `build\debug\Debug\assets\fonts`, na macOS to dowiązanie do katalogu w repozytorium, więc po ćwiczeniu sprawdź `git status`). Uruchom program i zapisz: co pojawia się w konsoli, czym różnią się panele i co stałoby się z polską literą w podpowiedzi ze ścieżką. Potem wskaż w `loadFont` linie, które wykonały się w tym uruchomieniu. Przywróć nazwę.
13. **Większy tekst.** Zmień `FONT_SIZE` na `20.0F`, skasuj `imgui.ini`, zbuduj i uruchom. Które panele przestały mieścić zawartość? Wyjaśnij, dlaczego `layoutScale` z sekcji 5.7 tego nie naprawiło (podpowiedź: z czego jest liczone). Wycofaj zmianę.
14. **Zwinięty panel w pliku.** Skasuj `imgui.ini`, uruchom program i zamknij go bez dotykania paneli. Znajdź w nowym pliku wpis `[Window][Camera]` i odczytaj linie `Pos=`, `Size=` i `Collapsed=`. Porównaj je z tabelą w sekcji 5.7. Potem uruchom program, rozwiń panel Camera, zamknij program i sprawdź, która linia się zmieniła. Na koniec zmień w `PanelLayout.hpp` `.collapsed = true` na `false`, zbuduj i uruchom **bez** kasowania pliku: dlaczego nic się nie zmieniło? Wycofaj zmianę.
15. **Kolejność na liście.** Zamień w `RendererPanel.cpp` miejscami napisy `Gouraud` i `Phong` w `LIGHTING_MODE_ITEMS`, zbuduj i wybierz na liście `Lighting` pozycję `Gouraud`. Którym programem gra rysuje teraz labirynt i po czym to poznać w scenie (podpowiedź: [`renderer/lighting-gouraud-phong.md`](renderer/lighting-gouraud-phong.md))? Wyjaśnij, dlaczego kompilator nie zgłosił błędu. Wycofaj zmianę.
16. **HUD i klawisz `~`.** Uruchom grę, naciśnij `~` i sprawdź, co zostało na ekranie. Potem przenieś w `DebugUI::draw` linię `drawHud(context.round, context.gameplay);` do wnętrza bloku `if (m_visible)`, zbuduj i powtórz. Wyjaśnij, dlaczego w obu wersjach `ImGui::NewFrame()` i `ImGui::Render()` muszą zostać poza tym blokiem. Wycofaj zmianę.
17. **HUD, który zabiera mysz.** Usuń w `Hud.cpp` flagę `ImGuiWindowFlags_NoInputs` z `PICTURE_WINDOW_FLAGS` i dopisz w `main.cpp` tymczasowy komunikat z ćwiczenia 6. Zbuduj, najedź kursorem na pasek HUD i kliknij w niego. Zapisz, kiedy pojawia się komunikat i czy kliknięcie przechwytuje kursor. Wyjaśnij wynik funkcją `wantsMouse()` i sekcją 5.9.4. Wycofaj obie zmiany.
18. **Kto leży na wierzchu.** Rozwiń panel Gameplay i zobacz, co stało się z paskiem HUD. Potem usuń w `Hud.cpp` flagę `ImGuiWindowFlags_NoBringToFrontOnFocus` ze `STATUS_WINDOW_FLAGS`, zbuduj i powtórz z rozwiniętym panelem. Co leży na wierzchu zaraz po starcie, a co po kliknięciu w panel? Wyjaśnij oba wyniki trzema regułami z sekcji 5.9.5 i powiedz, dlaczego wybrałem wersję z flagą. Wycofaj zmianę.
19. **Suwak bez `const`.** Zmień w `GameplayPanel.hpp` i `GameplayPanel.cpp` pierwszy parametr `drawGameplayPanel` na `const game::Round& round`, zbuduj i przeczytaj błąd kompilatora: w której linii i dlaczego akurat tam? Co trzeba by zmienić w `DebugContext`, żeby pole `round` mogło być `const`, i co wtedy straci pokaz? Wycofaj zmianę.
20. **Drugi rząd pasków.** Skasuj `imgui.ini`, uruchom program i zamknij go bez dotykania paneli. Odczytaj z nowego pliku linie `Pos=` wpisów `[Window][Camera]` i `[Window][Terrain]` i porównaj różnicę ich współrzędnych y z rachunkiem z sekcji 5.7 (jeden rząd: wysokość paska plus `PANEL_GAP`). Potem zmień w `PanelLayout.hpp` w `TERRAIN_PLACEMENT` `.foldedRowsBefore = 1` na `0`, skasuj plik, zbuduj i zobacz, gdzie stanął pasek Terrain. Wycofaj zmianę.
21. **Piąty rząd i HUD.** Zmień `FOLDED_ROW_COUNT` z 4 na 5, zbuduj i zmierz na zrzucie ekranu, o ile pikseli przesunął się pasek HUD. Porównaj z `ImGui::GetFrameHeight() + PANEL_GAP`. Potem zwiększ w `Theme.cpp` rozmiar czcionki o 4 i sprawdź, czy HUD nadal stoi pod paskami. Wyjaśnij, dlaczego do M5 trzeba by było przy tej zmianie poprawiać stałą ręcznie. Wycofaj zmiany.
22. **Flaga prośby czy praca w panelu.** Zmień tymczasowo sygnaturę `drawGrassPanel` tak, żeby dostawała też `const game::MazeWorld&`, i spróbuj zawołać `game::placeGrass` wprost z panelu. Czego nadal brakuje, żeby trawa na ekranie się zmieniła? Wypisz, jakie nagłówki musiałby dołączyć panel, żeby to dokończyć, i wyjaśnij na tej liście, dlaczego panel ustawia tylko `settings.replant`. Wycofaj zmianę.
23. **Trzy pliki w jednej linii.** Zepsuj celowo `assets/shaders/grass.geom` (na przykład usuń średnik), kliknij `Reload shaders` i odczytaj z panelu Shaders linię programu trawy i tekst błędu pod nią. Najedź kursorem na linię i policz ścieżki w podpowiedzi. Sprawdź, czy trawa w scenie zniknęła, czy rysuje się poprzednią wersją programu, i wyjaśnij to polem `isValid()`. Napraw plik i przeładuj.
24. **Podgląd z dekodowaniem.** W `AssetsPanel.cpp` zakomentuj oba wywołania `rawSampler.begin()` i `rawSampler.end()`, zbuduj i porównaj podgląd `wall_stone.png` z plikiem otwartym w przeglądarce obrazów. Dlaczego podgląd `wall_stone_normal.png` się nie zmienił? Wycofaj.
25. **Wskaźnik zamiast kopii.** W `RawTextureSampler::begin` usuń trzeci argument `AddCallback`. Co dostanie `bindSampler` w `command->UserCallbackData` i dlaczego program może mimo to przez jakiś czas działać poprawnie? Wycofaj.
26. **Flaga podglądów.** Rozwiń panel Framebuffers i schowaj panele klawiszem `~`. W debuggerze albo linią `core::logInfo` w `NightMazeApp::onRender` sprawdź, czy `drawPreviews` jest jeszcze wołane. Która linia `DebugUI::draw` o tym decyduje?

## 9. Pytania kontrolne

1. **Czym jest tryb natychmiastowy i co z niego wynika dla panelu?**
   Interfejs jest opisywany wywołaniami funkcji w każdej klatce, bez trwałych obiektów widżetów. Panel jest funkcją wołaną co klatkę, a jego dane należą do mojego kodu.

2. **Jaką rolę mają dwa backendy?**
   Backend GLFW dostarcza ImGui wejście, rozmiar okna i czas. Backend OpenGL3 rysuje listy przygotowane przez ImGui. Rdzeń biblioteki nie zna ani GLFW, ani OpenGL.

3. **Co oznacza `true` w `ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true)`?**
   Backend sam instaluje callbacki GLFW i woła z nich callbacki ustawione wcześniej (łańcuch). Z `false` musiałbym przekazywać zdarzenia ręcznie.

4. **Dlaczego `"#version 410"`?**
   To wersja GLSL shaderów backendu. Kontekst to OpenGL 4.1 Core, czyli GLSL 4.10. Starsze wersje nie skompilują się w profilu Core na macOS.

5. **Co robi `DockSpaceOverViewport` i po co `PassthruCentralNode`?**
   Tworzy obszar dokowania na całe okno, żeby panele dało się przyczepiać do krawędzi. Flaga sprawia, że pusty środek nie jest zamalowany i przepuszcza mysz, więc widać scenę.

6. **Dlaczego klatka ImGui wykonuje się także przy ukrytym UI?**
   Żeby ImGui dalej opróżniało kolejkę zdarzeń i miało poprawny czas między klatkami. Pomijane są tylko dockspace i panele. Od M5 jest też trzeci powód: w tej klatce rysowany jest HUD gry, którego klawisz `~` nie chowa.

7. **Czym różni się `ImGui::Render()` od `ImGui_ImplOpenGL3_RenderDrawData(...)`?**
   `Render` tylko buduje listy rysowania w pamięci. Dopiero `RenderDrawData` wykonuje wywołania OpenGL.

8. **Dlaczego ImGui rysuję na końcu klatki i czy psuje to stan OpenGL?**
   Na końcu, żeby panele były nad sceną. Backend zapamiętuje stan przed rysowaniem i przywraca go po, więc ustawienia renderera zostają nienaruszone.

9. **Dlaczego `ImGui::End()` jest poza `if (ImGui::Begin(...))`?**
   `Begin` zwraca `false` dla panelu zwiniętego lub zasłoniętego, ale okno i tak zostało otwarte, więc `End` musi być wywołane zawsze.

10. **Dlaczego `DebugUI` musi zostać zniszczone przed oknem i co to gwarantuje?**
    Zamknięcie backendów usuwa obiekty OpenGL i przywraca callbacki GLFW, więc potrzebuje żywego kontekstu i okna. Gwarantuje to C++: pola klasy (`DebugNightMazeApp::m_debugUI`) są niszczone przed jej klasami bazowymi, a okno jest polem `core::Application`.

11. **Jak kolor z panelu trafia do OpenGL?**
    `ColorEdit3` zapisuje przez wskaźnik do `NightMazeApp::m_clearColor` (referencję daje akcesor `clearColor()`). W następnej klatce `NightMazeApp::onRender` woła `glClearColor` z nowymi wartościami, a `glClear` ich używa. Od pierwszej części M7 między próbnikiem a `glClearColor` stoi jeszcze `gfx::srgbToLinear`: kolor z panelu jest wartością sRGB, a czyszczony bufor sceny trzyma wartości liniowe.

12. **Dlaczego gra nie posiada `DebugUI` i gdzie w takim razie ono żyje?**
    `game/` nie może zależeć od `debug/`. `DebugUI` jest polem `DebugNightMazeApp` w `main.cpp`, jedynym pliku, który tworzy obiekty obu warstw. Klasa dziedziczy po grze i po jej `onRender` dorysowuje panele.

13. **Jak program rozstrzyga, czy klawisz trafia do gry, czy do panelu?**
    `DebugUI::wantsKeyboard()` zwraca `ImGui::GetIO().WantCaptureKeyboard`, czyli informację, że aktywny jest jakiś widżet ImGui. `DebugNightMazeApp::onRender` przekazuje ją po `draw` do `input().setKeyboardBlocked(...)`. Od następnej klatki `isKeyDown` i `wasKeyPressed` zwracają `false`, więc Escape nie zamyka programu, a `~` nie chowa paneli. `core/` nie zna ImGui: dostaje tylko neutralną flagę.

14. **Którym klawiszem chowam panele i dlaczego obsługa stoi w `onRender`?**
    Klawiszem `~` na lewo od `1` (`GLFW_KEY_GRAVE_ACCENT`). `wasKeyPressed` to zbocze liczone raz na klatkę, a `onRender` wykonuje się dokładnie raz na klatkę, w odróżnieniu od `onUpdate`.

15. **Jak program rozstrzyga, czy mysz trafia do gry, czy do panelu, i czym warunek różni się od klawiatury?**
    `DebugUI::wantsMouse()` zwraca `ImGui::GetIO().WantCaptureMouse` (albo `false`, gdy mysz ImGui jest wyłączona), a `main.cpp` przekazuje to do `input().setMouseBlocked(...)`. Od następnej klatki `core::Input` odpowiada grze, że żaden przycisk nie jest wciśnięty, a przesunięcie myszy wynosi 0. Przy klawiaturze liczy się aktywny widżet, przy myszy wystarczy kursor nad panelem (albo trwające przeciąganie). Przezroczysty środek obszaru dokowania nie blokuje myszy.

16. **Czym jest `DebugContext` i dlaczego `DebugUI::draw` bierze strukturę, a nie osobne parametry?**
    To zwykła struktura referencji do obiektów aplikacji: wszystko, co panele mogą w tej klatce odczytać albo edytować. `main.cpp` buduje ją co klatkę inicjalizatorami desygnowanymi i przekazuje do `draw`. Dzięki niej nowe dane dla panelu to jedno pole w strukturze i jedna linia w `main.cpp`, a sygnatura `draw` zostaje ta sama. Struktura niczego nie posiada i nie może przeżyć klatki. Panele nadal dostają jawne parametry, żeby z ich sygnatur było widać, co czytają i co edytują.

17. **`DebugUI::draw` bierze `const DebugContext&`. Jak to możliwe, że panel zmienia kolor tła?**
    `const` na strukturze dotyczy jej pól, a polem jest referencja `std::array<float, 3>&`. Stałość nie przechodzi przez referencję na obiekt, na który ona wskazuje, więc przez to pole wolno pisać. Pola `const core::Time&` i `const core::Window&` są tylko do odczytu, bo `const` jest w ich typie. O edytowalności decyduje typ pola, nie stałość struktury.

18. **Jak panel Shaders dostaje jedenaście programów gry?**
    `game::NightMazeApp` ma chronione akcesory `texturedShader()`, `colorShader()`, `litShader()`, `gouraudShader()`, `skyboxShader()`, `grassShader()`, `compositeShader()`, `previewShader()`, `brightPassShader()`, `blurShader()` i `shadowDepthShader()`, `DebugContext` jedenaście pól `gfx::Shader&`, a `main.cpp` jedenaście linii. `DebugUI::draw` buduje z nich `std::array<gfx::Shader*, SHADER_COUNT>` ze stałą `SHADER_COUNT = 11` i przekazuje do `drawShadersPanel(std::span<gfx::Shader* const>)`. Tablica wskaźników, bo tablicy referencji nie ma. Sygnatura `DebugUI::draw` została ta sama, a gra nadal nie dołącza niczego z `debug/`. Pola nie mają `const`, bo przycisk panelu woła `reload()`. W M4 programów też było pięć: piąty rysował kostkę z M1 i zniknął razem z nią w M5. Dzisiejszy piąty to program nieba z pierwszej części M6, a szósty to program trawy z drugiej: jedyny z trzema plikami, bo ma shader geometrii. Siódmy i ósmy doszły w pierwszej części M7: `composite` (obraz HDR do okna) i `preview` (podglądy załączników), oba z tym samym shaderem wierzchołków. Dziewiąty i dziesiąty doszły w drugiej części M7: `bright` (przebieg jasności bloomu) i `blur` (rozmycie Gaussa), też z tym samym shaderem wierzchołków. Jedenasty doszedł w czwartej części M7: `shadow_depth` (przebieg głębi mapy cieni), z własną parą plików.
19. **Jak panel Camera dostaje kamerę, gracza i czułość myszy?**
    Tą samą drogą co kolor tła i shader. `game::NightMazeApp` ma pola `m_camera`, `m_player` i `m_mouseSensitivity` oraz chronione akcesory `camera()`, `player()` i `mouseSensitivity()`. `main.cpp` wpisuje je do trzech pól `DebugContext`, a `DebugUI::draw` woła `drawCameraPanel(context.camera, context.player, context.mouseSensitivity)`. Trzy prędkości są polami gracza, więc przychodzą razem z nim. Panel edytuje pola przez referencje. Gra nie dołącza niczego z `debug/`.

20. **Co się dzieje z panelami, gdy kamera przechwyci kursor, i dlaczego potrzebny jest do tego osobny kod?**
    Panele przestają reagować na mysz. Backend GLFW ImGui przekazuje pozycję kursora także w trybie `GLFW_CURSOR_DISABLED`, więc niewidoczny kursor najeżdżałby na panele i klikał w nie. `main.cpp` woła przed `draw` `m_debugUI.setMouseEnabled(!input().isCursorCaptured())`, co ustawia `ImGuiConfigFlags_NoMouse`: ImGui w `NewFrame` kasuje wtedy informację o oknie pod kursorem. Flaga działa w tej samej klatce, w której została ustawiona.

21. **Dlaczego `wantsMouse()` zwraca `false`, gdy mysz ImGui jest wyłączona, skoro flaga `NoMouse` już jest ustawiona?**
    Bo `NoMouse` nie zeruje `WantCaptureMouse` w każdym przypadku. Przycisk wciśnięty, gdy niewidoczny kursor jest nad panelem, ImGui nadal uznaje za swoje kliknięcie i zgłasza `WantCaptureMouse`, dopóki przycisk jest trzymany. `main.cpp` zablokowałby wtedy grze mysz i obrót kamery by zamarł.

21. **Co robi `ImGuiCond_FirstUseEver` i dlaczego po skopiowaniu repozytorium ze starym `imgui.ini` część paneli stoi w złym miejscu?**
    Warunek każe zastosować pozycję i rozmiar tylko dla okna, o którym ImGui nie ma zapisanych danych. Dane są w `imgui.ini` w katalogu roboczym. Panel, który ma tam wpis, zostaje tam, gdzie był. Panel bez wpisu staje według swojej stałej `..._PLACEMENT` z `PanelLayout.hpp`, którą przekazuje do `placePanelOnFirstUse`.

22. **Dlaczego lista shaderów to tablica wskaźników, a nie referencji, i co znaczy `std::span<gfx::Shader* const>`?**
    Referencja nie jest obiektem, więc nie może być elementem tablicy. `std::span<gfx::Shader* const>` to widok na ciąg stałych wskaźników do niestałych shaderów: listy nie da się zmienić, shadery tak.

23. **Dlaczego w `main.cpp` linie `.litShader` i `.gouraudShader` nie stoją obok `.texturedShader` i `.colorShader`?**
    Inicjalizatory desygnowane muszą iść w kolejności deklaracji pól struktury, a nowe pola `DebugContext` były dopisywane na końcu. Dwa pierwsze programy doszły w M2 + M3, dwa następne w M4, po ośmiu innych polach. Z tego samego powodu po `.gameplay` i `.round` z M5 stoją dwie linie nieba i cztery linie terenu i trawy z M6: `.grassTuftCount` jest ostatnia.

24. **Który panel może zmienić labirynt?**
    Żaden bezpośrednio. Pole `mazeWorld` jest `const`. Panel Maze zapisuje rozmiar, ziarno i flagę `regenerate` w `mazeSettings`, a gra buduje labirynt na początku następnej klatki.

25. **Gdzie ImGui trzyma wygląd paneli i dlaczego motyw to jedna funkcja wołana raz?**
    W strukturze `ImGuiStyle` kontekstu: tabela kolorów `Colors` indeksowana wyliczeniem `ImGuiCol_` i metryki (odstępy, zaokrąglenia, rozmiar czcionki). Widżet nie ma własnego koloru, tylko czyta pozycję tabeli odpowiadającą swojej roli i stanowi. `applyTheme` wypełnia tabelę i metryki w konstruktorze `DebugUI` i odtąd wszystkie panele rysują się w tym stylu.

26. **Dlaczego tło widżetu pod kursorem jest ciemnym brązem, a nie jasnym bursztynem z palety?**
    Bo tekst ma jeden kolor, `ImGuiCol_Text`, niezależnie od tła. Jasny tekst `MOONLIGHT` na jasnym bursztynie miałby kontrast około 1,4. Na `EMBER` ma 9,3, a na `EMBER_BRIGHT` 7,5, czyli powyżej przyjętego progu 7. Jasny bursztyn jest tylko na elementach bez tekstu: ptaszku pola wyboru i kresce nad wybraną zakładką.

27. **Jak liczę kontrast i ile wynosi dla tekstu na tle panelu?**
    Wzorem WCAG: `(L1 + 0,05) / (L2 + 0,05)`, gdzie `L` to luminancja względna, liczona z liniowych składowych z wagami 0,2126, 0,7152 i 0,0722. Tekst `MOONLIGHT` na `NIGHT` daje 15,1. Z białą ścianą prześwitującą przez tło panelu (alfa 0,94) spada do 13,1.

28. **Kto jest właścicielem bajtów czcionki i jak długo muszą żyć?**
    Pole `DebugUI::m_fontBytes`, czyli `std::vector`. ImGui dostaje wskaźnik z `FontDataOwnedByAtlas = false`, więc ich nie zwalnia. Muszą istnieć do `ImGui::DestroyContext()`, bo ImGui 1.92 rysuje znaki do tekstury dopiero przy pierwszym użyciu. Gwarantuje to kolejność niszczenia: ciało destruktora (z `DestroyContext`) wykonuje się przed zniszczeniem pól.

29. **Dlaczego czcionka jest czytana przez własny kod, a nie przez `AddFontFromFileTTF`?**
    Nie z powodu polskich liter w ścieżce: ImGui otwiera plik przez `_wfopen` po zamianie nazwy z UTF-8. Powodem jest brak pliku: `AddFontFromFileTTF` domyślnie wywołuje wtedy asercję, która w buildzie Debug zatrzymuje program, a flaga wyłączająca tę asercję nie jest jeszcze publiczna. Własne czytanie pozwala zalogować jeden błąd i użyć czcionki wbudowanej.

30. **Co się dzieje przy skali ekranu 150% na Windowsie, a co na Retinie?**
    Na Windowsie `ImGui_ImplGlfw_GetContentScaleForWindow` zwraca 1,5, a `applyTheme` mnoży przez to metryki (`ScaleAllSizes`) i czcionkę (`FontScaleDpi`). Na macOS ta sama funkcja zwraca 1, bo gęsty ekran obsługuje framebuffer dwa razy większy od okna i ImGui rysuje ostrzej bez mojego kodu. To drugie nie było jeszcze sprawdzone na Macu.

31. **Jak panel trafia do prawego dolnego rogu okna o dowolnym rozmiarze?**
    `placePanelOnFirstUse` liczy róg głównego viewportu, przesuwa go do środka o `offset` i woła `SetNextWindowPos` z pivotem równym temu rogowi. Pivot `(1, 1)` każe ustawić panel jego prawym dolnym rogiem w podanym punkcie. Dzieje się to raz, w pierwszej klatce, i tylko gdy `imgui.ini` nie ma wpisu panelu.

32. **Ile jest paneli, gdzie stoją w oknie 1280 x 720 i dlaczego sześć z nich startuje zwiniętych?**
    Dwanaście. Lewa kolumna: Renderer (336 x 284) nad Lights (336 x 412, z paskiem przewijania). Prawa kolumna: Maze (300 x 480) nad Assets (300 x 216). Dolny rząd między kolumnami: Collision (312 x 280) i Shaders (292 x 280). Dwie kolumny i dolny rząd mieszczą sześć paneli, więc siódmy i ósmy, Camera (280 x 416 po rozwinięciu) i Gameplay (324 x 416), stoją obok siebie przy górnej krawędzi między kolumnami i startują zwinięte do pasków tytułów: stałe `CAMERA_PLACEMENT` i `GAMEPLAY_PLACEMENT` mają `.collapsed = true`, a `placePanelOnFirstUse` przekazuje to pole do `ImGui::SetNextWindowCollapsed(placement.collapsed, ImGuiCond_FirstUseEver)`. Rozwinięte sięgają do y 424, a dolny rząd zaczyna się w y 432, więc nie zasłaniają żadnego rozwiniętego panelu, tylko scenę, pasek HUD i, od M6, zwinięty pasek pod sobą. Gameplay może startować zwinięty, bo stan rundy pokazuje HUD. Dziewiąty i dziesiąty panel, Terrain (280 x 170) i Grass (324 x 190), stoją w drugim rzędzie pasków, pod Camera i Gameplay (pytania 41 i 42). Jedenasty, Framebuffers (612 x 344), ma jeden szeroki pasek w trzecim rzędzie, a dwunasty, Shadows (612 x 324), taki sam pasek w czwartym (pytanie 48).

33. **Jak lista `Lighting` w panelu Renderer zmienia tryb oświetlenia i co musi się zgadzać, żeby działała poprawnie?**
    `DebugUI::draw` przekazuje do panelu `context.lighting.mode`, czyli referencję do pola `NightMazeApp::m_lighting.mode`. Panel rzutuje wyliczenie na `int`, podaje adres tej liczby do `ImGui::Combo` razem z napisem `"Unlit\0Gouraud\0Phong\0Blinn-Phong\0"`, a gdy `Combo` zwróci `true`, rzutuje numer z powrotem na `game::LightingMode`. Kolejność pozycji w napisie musi być taka jak kolejność wartości wyliczenia, bo numer pozycji staje się wartością bez żadnej tablicy pośredniej. W następnej klatce `NightMazeApp::drawMaze` wybiera według tej wartości program `textured`, `gouraud` albo `lit`, i tym samym programem rysuje bramę i kryształy.

34. **Co z układu paneli ImGui pamięta między uruchomieniami, a czego nie?**
    W `imgui.ini` (katalog roboczy) są dla każdego okna pozycja, rozmiar, stan zwinięcia i dokowanie. Panel Camera albo Gameplay rozwinięty raz zostaje więc rozwinięty. Nie ma tam stanu nagłówków `CollapsingHeader` z panelu Lights ani wyglądu paneli: grupy wracają po starcie do stanu z kodu, a motyw ustawia `applyTheme`. Nie ma tam też okien HUD: mają flagę `NoSavedSettings`, a ich miejsce kod ustawia w każdej klatce.

35. **Czym HUD różni się od panelu i dlaczego mimo to jest w module `debug`?**
    Panel jest narzędziem: da się go schować, przesunąć, zadokować i kliknąć. HUD jest częścią gry: dwa okna bez paska tytułu, bez wejścia, ustawiane przez kod w każdej klatce. Rysuję go ImGui, a `game/` nie może dołączać ImGui, więc funkcja `debug::drawHud` stoi w `src/debug/Hud.cpp`. Dostaje `const game::Round&` i `const game::GameplaySettings&`, czyli tylko czyta.

36. **Dlaczego klawisz `~` chowa panele, a HUD zostaje?**
    `toggleVisible()` zmienia pole `m_visible`, a w `DebugUI::draw` od tego pola zależy tylko blok z dockspace i dwunastoma panelami. Wywołanie `drawHud(context.round, context.gameplay)` stoi za tym blokiem, przed `ImGui::Render()`, więc wykonuje się w każdej klatce.

37. **Dlaczego HUD nie zabiera grze myszy ani klawiatury?**
    Oba okna mają flagę `ImGuiWindowFlags_NoInputs`. ImGui pomija takie okno, gdy szuka okna pod kursorem, więc kursor nad paskiem nie ustawia `WantCaptureMouse` i `wantsMouse()` odpowiada tak, jakby HUD nie było. W HUD nie ma też widżetu, który mógłby stać się aktywny, więc nie ustawia `WantCaptureKeyboard`.

38. **Co leży na wierzchu: panel, pasek HUD czy karta wygranej?**
    Pasek HUD ma flagę `NoBringToFrontOnFocus`: powstaje na tyle listy okien i nigdy nie przechodzi na przód, więc każdy panel go zasłania. Karta tej flagi nie ma: jako nowe okno powstaje na przodzie, na panelach. Nie da się jej jednak kliknąć, więc panel kliknięty później przechodzi przed nią. Kolejność wywołań w `DebugUI::draw` nie ma na to wpływu.

39. **Dlaczego pole `round` w `DebugContext` nie jest `const`, skoro prawie wszyscy tylko je czytają?**
    Z jednego powodu: suwak `Battery` w panelu Gameplay pisze do `round.battery` przez wskaźnik. Maze, Collision, Lights i HUD biorą z tego pola `const game::Round&`. O edytowalności decyduje typ pola kontekstu, a każda funkcja zawęża go w swojej sygnaturze do tego, czego potrzebuje.

40. **Jak przycisk `Restart round (key R)` zaczyna nową rundę?**
    Nie zaczyna jej sam. Ustawia `settings.restart = true` w `game::GameplaySettings`, a `NightMazeApp::onRender` na początku następnej klatki sprawdza tę flagę razem z klawiszem R, zeruje ją i woła `beginRound()`. Stan gry zmienia się więc między krokami symulacji, a nie w środku klatki ImGui. To ten sam wzór co flaga `regenerate` panelu Maze.

36. **Jakie kontrolki nieba ma panel Renderer i dlaczego po ich dodaniu panel Lights się przewija?**
    Pole wyboru `Skybox` (`SkyboxSettings::enabled`) i suwak `Sky brightness` od 0 do 6 (do M6 do 3, `SkyboxSettings::brightness`), oba piszące przez wskaźnik do pól `NightMazeApp::m_skyboxSettings`. Dwa nowe wiersze podniosły `RENDERER_HEIGHT` z 230 do 284, a `LIGHTS_HEIGHT` jest liczone jako reszta kolumny, więc spadło z 466 do 412 i jest mniejsze od zawartości panelu.

37. **Dlaczego po aktualizacji programu nowych kontrolek może nie być widać?**
    Rozmiar startowy z `PanelLayout.hpp` działa tylko wtedy, gdy `imgui.ini` nie ma wpisu panelu (`ImGuiCond_FirstUseEver`). Stary plik pamięta panel Renderer o wysokości 230, a kontrolki nieba leżą niżej. Trzeba panel przewinąć, powiększyć albo usunąć `imgui.ini`.

41. **Ile paneli było po M6 i gdzie stoją dwa, które wtedy doszły?**
    Dziesięć (dziś dwanaście, pytanie 48). Terrain i Grass doszły w drugiej części M6 i startują zwinięte w drugim rzędzie pasków tytułów: Terrain pod paskiem Camera (ta sama szerokość, 280), Grass pod paskiem Gameplay (324). Rozwinięte mają 280 x 170 i 324 x 190. Ich stałe mają `.collapsed = true` i `.foldedRowsBefore = 1`.

42. **Po co pole `foldedRowsBefore`, skoro panel można po prostu postawić niżej większym `offset.y`?**
    Bo wysokość paska tytułu nie jest stałą układu: wynika z czcionki (`ImGui::GetFrameHeight()`, czyli rozmiar czcionki plus dwa razy `FramePadding.y`), a `offset` jest w jednostkach okna odniesienia i mnoży się przez `layoutScale`. Przy skali ekranu 150% w oknie 1280 x 720 `layoutScale` wynosi 1, a pasek jest 1,5 raza wyższy: stały `offset` postawiłby drugi rząd na pierwszym. `placePanelOnFirstUse` dodaje więc do pozycji `foldedRowsHeight(foldedRowsBefore, layoutScale)`, które pyta ImGui o prawdziwą wysokość paska.

43. **Skąd HUD wie, jak nisko ma stanąć?**
    Z sumy dwóch części: `foldedRowsHeight(FOLDED_ROW_COUNT, scale)`, czyli cztery rzędy po (wysokość paska + `PANEL_GAP`), i `HUD_TOP_OFFSET * scale`, czyli dwa odstępy `PANEL_GAP`. Przy skali 100% i pasku 22 to 120 + 16 = 136 jednostek od górnej krawędzi. Do M5 była to stała 46, w M6 wychodziło 76 (dwa rzędy), w pierwszych trzech częściach M7 106 (trzy rzędy), więc każdy nowy rząd zsuwa HUD o 30 jednostek. Liczby 30 nie ma w kodzie, bo zależy od czcionki.

44. **Co się dzieje po przesunięciu suwaka `Height scale` i dlaczego nie dzieje się to w panelu?**
    Panel zapisuje nową liczbę w `TerrainSettings::heightScale` i ustawia `rebuild = true`. Na początku następnej klatki `NightMazeApp::onRender` zeruje flagę i woła `rebuildTerrain()`: nowy teren, ściany i brama zatopione od nowa, kryształy i lista przeszkód przestawione, idący gracz postawiony na gruncie, siatka i trawa wysłane na kartę. Panel nie ma dostępu do tych obiektów (dostaje tylko ustawienia i teren do odczytu), a przebudowa w środku rysowania paneli zmieniałaby świat, który inne panele tej klatki jeszcze czytają. To ten sam wzór co `regenerate`, `restart` i `replant`.

45. **Które suwaki panelu Grass wymagają sadzenia trawy od nowa, a które nie?**
    Tylko `Density`: zmienia liczbę i miejsca punktów w buforze wierzchołków, więc ustawia flagę `replant`, a gra woła `placeGrass` i `GrassRenderer::upload`. `Blade height` i `Wind strength` to uniformy programu trawy: shader geometrii buduje źdźbła od nowa w każdej klatce, więc nowa wartość działa od razu. `Enabled` tylko pomija rysowanie.

46. **Dlaczego `grassTuftCount` jest w `DebugContext` wartością, a nie referencją, i czym to grozi?**
    Bo gra nie ma pola z tą liczbą: akcesor `grassTuftCount()` zwraca wynik `GrassRenderer::tuftCount()` przez wartość, a referencja do wartości tymczasowej nie przeżyłaby wyrażenia. Panel i tak tylko ją wypisuje. Ryzyko: pola będącego wartością wolno nie podać w inicjalizatorze desygnowanym (dostanie zero), więc brak linii w `main.cpp` nie jest błędem kompilacji jak przy referencji (pułapka 14).

47. **Jak panel Shaders pokazuje program z trzema plikami?**
    `drawShaderStatus` skleja nazwy plików znakiem ` + ` w kolejności etapów: plik wierzchołków, plik geometrii (tylko gdy `shader.hasGeometryStage()`), plik fragmentów. Dla trawy wychodzi `grass.vert + grass.geom + grass.frag: OK`. Podpowiedź pokazuje pełne ścieżki, po jednej w linii. Pętla po liście programów i lista w `DebugUI::draw` (dziś jedenaście wskaźników, `SHADER_COUNT = 11`) nie musiały o trzecim pliku wiedzieć.

48. **Ile jest dziś paneli i gdzie stoi najnowszy?**
    Dwanaście. Shadows doszedł w czwartej części M7 i startuje zwinięty w czwartym rzędzie pasków tytułów, jako jeden pasek o szerokości 612 jednostek (`FRAMEBUFFERS_WIDTH`), od y 98 do 120. Rozwinięty ma 612 x 324. Jego stała ma `.collapsed = true` i `.foldedRowsBefore = 3`, a `FOLDED_ROW_COUNT` wzrosło do 4, więc HUD stoi o jeden rząd niżej, w y 136. Poprzedni, Framebuffers, doszedł tą samą drogą w pierwszej części M7: trzeci rząd, pasek od y 68 do 90, 612 x 344 po rozwinięciu, `.foldedRowsBefore = 2` i `FOLDED_ROW_COUNT` podniesione wtedy do 3.

49. **Kiedy gra rysuje podglądy załączników framebuffera i kto o tym decyduje?**
    Tylko gdy pole `PostProcessSettings::previews` jest prawdą. `DebugUI::draw` zeruje je na początku każdej klatki, a panel Framebuffers ustawia je na wynik `ImGui::Begin`, czyli na prawdę tylko wtedy, gdy jest otwarty. Przy schowanych panelach albo zwiniętym panelu flaga zostaje fałszem. Gra czyta ją w następnej klatce, stąd napis `(no picture yet)` w pierwszej klatce po otwarciu.

50. **Dlaczego podgląd tekstury sRGB w ImGui wyszedłby za ciemny?**
    Karta dekoduje teksturę sRGB do wartości liniowych przy odczycie. Scena koduje je z powrotem w ostatnim przebiegu klatki, ale ImGui rysuje po tym przebiegu, prosto do okna, i wpisuje to, co odczytało. Wartości liniowe pokazane bez kodowania są ciemniejsze: bajt 128 wyszedłby jako 55.

51. **Jak `RawTextureSampler` to naprawia?**
    Tworzy obiekt samplera z parametrem `GL_TEXTURE_SRGB_DECODE_EXT` ustawionym na `GL_SKIP_DECODE_EXT` (rozszerzenie `GL_EXT_texture_sRGB_decode`) i na czas jednego obrazu wpina go na jednostkę 0 poleceniem w liście rysowania ImGui. Tekstura jest wtedy czytana bez dekodowania i bajty pliku trafiają do okna bez zmiany.

52. **Dlaczego nie wystarczy zawołać `glBindSampler` w kodzie panelu?**
    Bo panel nie rysuje, tylko dopisuje polecenia do listy. Rysowanie odbywa się na końcu klatki w `RenderDrawData`, które ustawia własny stan. Własną zmianę stanu trzeba wstawić do listy jako wywołanie zwrotne (`ImDrawList::AddCallback`), dokładnie przed obrazem, a drugie, po obrazie, przywraca sampler backendu (`DrawCallback_SetSamplerLinear`).

53. **Po co trzeci argument `AddCallback` w `begin`?**
    Z rozmiarem ImGui kopiuje wskazane bajty (tu identyfikator samplera) do własnego bufora. Bez niego zapamiętałoby wskaźnik na zmienną lokalną, która nie istnieje już w chwili rysowania.

54. **Skąd stałe `0x8A48` i `0x8A4A` i kiedy wolno ich użyć?**
    Ze specyfikacji rozszerzenia `GL_EXT_texture_sRGB_decode`. GLAD projektu jest wygenerowany bez rozszerzeń, więc nagłówek ich nie ma i są wpisane ręcznie. Wolno ich użyć dopiero po tym, jak `gfx::hasExtension` potwierdzi rozszerzenie w działającym sterowniku.

55. **Co się stanie na sterowniku bez tego rozszerzenia?**
    Konstruktor nie utworzy samplera, `isSupported()` zwróci fałsz, a `begin` i `end` nic nie zrobią. Program działa, scena jest poprawna, tylko podglądy tekstur sRGB w panelu Assets są ciemniejsze niż pliki. Tego przypadku nikt nie sprawdził na żadnej maszynie.

56. **Dlaczego panele i HUD nie zmieniły kolorów po wprowadzeniu gammy?**
    Bo ImGui rysuje po przebiegu składającym, prosto do framebuffera okna, a `GL_FRAMEBUFFER_SRGB` jest wyłączone: kodowanie sRGB robi shader przebiegu składającego tylko dla obrazu sceny. Liczby z motywu trafiają na ekran takie, jakie są ([`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md)).

57. **Dlaczego `DebugUI.hpp` dołącza `RawTextureSampler.hpp`, skoro pozostałe nagłówki modułu używają deklaracji wyprzedzających?**
    Bo `DebugUI` ma pole `m_rawTextureSampler` przez wartość, a kompilator musi znać rozmiar pola. Deklaracja wyprzedzająca wystarcza tylko dla referencji i wskaźników.

58. **Kiedy gra rysuje obraz podglądu mapy cieni i czy zwinięcie panelu Shadows wyłącza cienie?**
    Obraz rysuje tylko wtedy, gdy pole `ShadowSettings::preview` jest prawdą. `DebugUI::draw` zeruje je na początku każdej klatki, a panel Shadows ustawia flagę zakładki, która jest właśnie pokazana (od piątej części M7 są dwie, `Moon` i `Flashlight`, i każda ma własną flagę w swoim `ShadowSettings`). To ta sama umowa co flaga `previews` panelu Framebuffers (pytanie 49). Cieni to nie wyłącza: mapę cieni gra rysuje w każdej klatce, dopóki pole `Shadows` (`ShadowSettings::enabled`) jest zaznaczone, także przy schowanych panelach.

59. **Dlaczego panel Shadows nie podaje tekstury głębi mapy cieni wprost do `ImGui::Image`?**
    Tekstura głębi ma dane w jednym kanale, a ImGui rysuje ją jak teksturę koloru: obraz wyszedłby czerwony. Gra rysuje więc osobnym małym przebiegiem (program `preview`, tryb `uMode == 2`) szary obraz do framebuffera 256 x 256 w formacie `GL_RGBA8`, a panel pokazuje jego teksturę koloru, z odwróconym `v`. Głębi nie trzeba przeliczać na odległość, bo rzut światła kierunkowego jest prostokątny i zapisana głębia rośnie równo z odległością.

60. **Jak lista `Kernel` panelu Shadows zamienia pozycję na promień PCF?**
    Lista pokazuje boki jądra (`3 x 3`, `5 x 5`, `7 x 7`), a ustawienia trzymają promień od 1 do 3. Pozycja numer i to promień i + `game::MIN_PCF_RADIUS`, a w drugą stronę panel odejmuje tę stałą od promienia sprowadzonego do zakresu przez `std::clamp`. Bok jądra to 2 * promień + 1.

## 10. Źródła

- Dokument biblioteki w tym repozytorium: [`../libraries/imgui.md`](../libraries/imgui.md).
- Czcionki i skala w naszej wersji ImGui, lokalnie po pierwszej konfiguracji: `build/debug/_deps/imgui-src/docs/FONTS.md` (dynamiczne czcionki od 1.92, wczytywanie z pamięci, własność danych, DPI), `build/debug/_deps/imgui-src/imgui.h` (struktury `ImGuiStyle`, `ImFontConfig`, `ImFontAtlas`, wyliczenie `ImGuiCol_`), `build/debug/_deps/imgui-src/backends/imgui_impl_glfw.cpp` (funkcja `ImGui_ImplGlfw_GetContentScaleForWindow`) i `build/debug/_deps/imgui-src/examples/example_glfw_opengl3/main.cpp` (wzór: `ScaleAllSizes` i `FontScaleDpi`).
- Czcionka Atkinson Hyperlegible: <https://github.com/google/fonts/tree/main/ofl/atkinsonhyperlegible> (plik i licencja), opis źródła i wersji w [`../../assets/fonts/README.md`](../../assets/fonts/README.md).
- SIL Open Font License 1.1: <https://openfontlicense.org>.
- WCAG 2.2, definicje "contrast ratio" i "relative luminance": <https://www.w3.org/TR/WCAG22/>.
- Dear ImGui, repozytorium: <https://github.com/ocornut/imgui> (pliki `imgui.h`, `backends/imgui_impl_glfw.cpp`, `backends/imgui_impl_opengl3.cpp` oraz przykład `example_glfw_opengl3`).
- Dear ImGui, wiki: <https://github.com/ocornut/imgui/wiki> (strony "Getting Started" i "Docking").
- Dokumenty modułu `core`, z którymi ten moduł się styka: [`core/README.md`](core/README.md) (warstwy, kolejność niszczenia), [`core/input.md`](core/input.md) (blokada klawiatury).
- Dokument panelu Shaders: [`gfx/shader-hot-reload.md`](gfx/shader-hot-reload.md), sekcja 6.
- Dokument panelu Camera i sterowania kamerą: [`scene/camera-controls.md`](scene/camera-controls.md), sekcje 5 i 6.
- Dokumenty trzech paneli z M2 + M3: [`game/maze-generator.md`](game/maze-generator.md), sekcja 6 (Maze), [`scene/collision.md`](scene/collision.md), sekcja 6 (Collision), [`assets/asset-cache.md`](assets/asset-cache.md), sekcja 6 (Assets).
- Dokumenty oświetlenia z M4: [`scene/lights.md`](scene/lights.md), sekcja 6 (panel Lights linia po linii), [`renderer/lighting-gouraud-phong.md`](renderer/lighting-gouraud-phong.md) (cztery tryby z listy `Lighting` i wybór programu), [`game/flashlight.md`](game/flashlight.md) (struktura `LightingSettings`, latarka i klawisz F), [`gfx/shader-includes.md`](gfx/shader-includes.md) (nazwa pliku w tekście błędu w panelu Shaders).
- Stan okna w `imgui.ini` w naszej wersji ImGui: `build/debug/_deps/imgui-src/imgui.cpp`, funkcje `WindowSettingsHandler_ReadLine` i `WindowSettingsHandler_WriteAll` (linia `Collapsed=`), oraz w tym samym pliku wzór na wysokość paska tytułu (`TitleBarHeight`).
- Źródła ImGui dokładnie w naszej wersji, lokalnie po pierwszej konfiguracji: `build/debug/_deps/imgui-src/imgui.cpp` (funkcja `UpdateHoveredWindowAndCaptureFlags`: flaga `ImGuiConfigFlags_NoMouse`, `WantCaptureMouse`) i `build/debug/_deps/imgui-src/backends/imgui_impl_glfw.cpp` (historia zmian na górze pliku, funkcja `ImGui_ImplGlfw_UpdateMouseCursor`).
- Dokumenty pierwszej części M7: [`renderer/post-process.md`](renderer/post-process.md) (panel Framebuffers linia po linii, podglądy załączników, przebieg składający, po którym rysuje ImGui), [`gfx/framebuffers.md`](gfx/framebuffers.md) (klasa `gfx::Framebuffer`, której tekstury pokazuje panel), [`gfx/color-space.md`](gfx/color-space.md) (sRGB i wartości liniowe: dlaczego podgląd tekstury sRGB wymaga osobnego samplera i dlaczego kolory z próbników są przeliczane), [`gfx/textures.md`](gfx/textures.md) (`gfx::hasExtension`, obiekt samplera), notatka [`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md) (dlaczego `GL_FRAMEBUFFER_SRGB` jest wyłączone i panele zachowują kolory).
- Rozszerzenie `GL_EXT_texture_sRGB_decode`, specyfikacja: <https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_sRGB_decode.txt> (stałe `GL_TEXTURE_SRGB_DECODE_EXT` = `0x8A48`, `GL_SKIP_DECODE_EXT` = `0x8A4A`).
- Wywołania zwrotne w liście rysowania w naszej wersji ImGui, lokalnie po pierwszej konfiguracji: `build/debug/_deps/imgui-src/imgui.h` (`ImDrawList::AddCallback` z parametrem `userdata_size`, pole `ImDrawCmd::UserCallbackData`, pola `ImGuiPlatformIO::DrawCallback_SetSamplerLinear` i `DrawCallback_SetSamplerNearest`) oraz `build/debug/_deps/imgui-src/backends/imgui_impl_opengl3.cpp` (funkcja `ImGui_ImplOpenGL3_DrawCallback_SetSamplerLinear` i pętla po poleceniach w `ImGui_ImplOpenGL3_RenderDrawData`).
- Dokument rozgrywki z M5: [`game/gameplay.md`](game/gameplay.md), sekcja 6 (HUD i panel Gameplay od strony gry), sekcja 2 (reguły, które zmieniają suwaki) i sekcja 7 (znane ograniczenia).
- Okna HUD w naszej wersji ImGui, lokalnie po pierwszej konfiguracji: `build/debug/_deps/imgui-src/imgui.h` (wyliczenie `ImGuiWindowFlags_`: z czego składają się `NoDecoration` i `NoInputs`) oraz `build/debug/_deps/imgui-src/imgui.cpp` (funkcje `CreateNewWindow`, `FocusWindow` i `FindHoveredWindowEx`: miejsce nowego okna na liście, przechodzenie na przód i pomijanie okien bez myszy).
- Dokumentacja GLFW: <https://www.glfw.org/docs/latest/> (przewodnik o wejściu: callbacki klawiatury i myszy).
- docs.gl (<https://docs.gl>): `glBlendFunc`, `glScissor`, `glDrawElements`, czyli funkcje, na których opiera się backend.
- LearnOpenGL nie ma rozdziału o ImGui. Pomocne tło: "Hello Window" (<https://learnopengl.com/Getting-started/Hello-Window>) i "Blending" (<https://learnopengl.com/Advanced-OpenGL/Blending>).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" oraz "OpenGL. Księga eksperta": tło do mieszania kolorów, testu nożycowego i rzutu prostokątnego, których używa backend.
