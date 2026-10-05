# Moduł core: fundament programu

Kamień milowy: M0, uzupełniany w M1 (mysz, ścieżki do assetów, pierwszy użytkownik stałego kroku i myszy: kamera) w M2 + M3 (klasa `NightMazeApp` rysuje labirynt i prowadzi gracza) w M4 (klasa `NightMazeApp` buduje co klatkę światła, wybiera program labiryntu według trybu oświetlenia i ustawia przełącznik mapowania normalnych `uNormalMapEnabled`) i w M5 (klasa `NightMazeApp` prowadzi rundę: w stałym kroku woła `updateRound`, co klatkę obsługuje klawisz R, rysuje kryształy i bramę, a kostka z M1 i znaczniki świateł z M4 zniknęły). Temat wykładu: 1 (Pierwszy program OpenGL).
Kod: [`src/core/`](../../../src/core/), [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), [`src/main.cpp`](../../../src/main.cpp).

Zanim narysuję cokolwiek w OpenGL, muszę mieć trzy rzeczy: okno systemowe, kontekst OpenGL (context) związany z tym oknem oraz pętlę, która co klatkę odbiera zdarzenia, przesuwa symulację i rysuje obraz. Moduł `core` dostarcza dokładnie to i nic więcej: klasę `Window` (okno GLFW z kontekstem OpenGL 4.1 Core i funkcjami załadowanymi przez GLAD), klasę `Application` (pętla główna ze stałym krokiem symulacji), `Input` (stan klawiatury i myszy), `Time` (zegar klatki), `Log` (komunikaty na konsolę), makro `GL_CHECK` (wykrywanie błędów OpenGL w buildzie Debug) i funkcje `executableDir`, `assetPath` oraz `pathText` z `Paths` (ścieżki do plików z `assets/`, liczone od położenia programu, i zamiana ścieżki na tekst). To jest realizacja tematu 1 wykładu, "Pierwszy program OpenGL": po M0 program otwiera okno, czyści je ciemnym granatem i pokazuje FPS. Wszystkie późniejsze moduły (`gfx`, `renderer`, `scene`, `game`) stoją na tej warstwie, a ona sama nie wie o żadnym z nich.

Moduł jest opisany w pięciu dokumentach tematycznych. Ten plik jest ich wspólnym wstępem: pokazuje, jak części pasują do siebie, opisuje klatkę jako całość i to, jak program dziedziczy po `core::Application`. Jest też dokumentem klasy `game::NightMazeApp` (na niego wskazuje komentarz na górze `NightMazeApp.hpp` i `NightMazeApp.cpp`): sekcja 6 przechodzi przez tę klasę linia po linii, a sekcja 7 tłumaczy kolejność jej pól.

## 1. Dokumenty modułu

| Dokument | Co opisuje | Klasy i pliki |
|---|---|---|
| [`window-context.md`](window-context.md) | okno, inicjalizacja GLFW, hinty kontekstu, ładowanie GLAD, vsync, rozmiar okna a rozmiar framebuffera, logowanie | `Window`, `Log` |
| [`main-loop.md`](main-loop.md) | pętla główna, zegar klatki, stały krok czasowy z akumulatorem, ograniczenie 0,25 s, `alpha`, uśredniony FPS | `Application`, `Time` |
| [`input.md`](input.md) | klawiatura i mysz: odpytywanie, stan ciągły i zbocze, `KEY_COUNT`, przesunięcie myszy, przechwycenie kursora, blokada klawiatury i myszy na czas pracy z panelem | `Input` |
| [`gl-check.md`](gl-check.md) | makro `GL_CHECK`, model błędów `glGetError`, różnica Debug i Release | `GlCheck` |
| [`paths.md`](paths.md) | ścieżki do assetów: katalog roboczy a katalog programu, `_NSGetExecutablePath` i `GetModuleFileNameW`, `std::filesystem::path`, znaki szerokie na Windowsie | `Paths` |

Każdy z pięciu dokumentów jest samodzielną jednostką nauki i ma te same dziesięć sekcji: Po co to jest, Teoria, Jak to działa w OpenGL, Shadery, Kod w projekcie, Panel ImGui, Pułapki, Ćwiczenia, Pytania kontrolne, Źródła.

Proponowana kolejność czytania: ten plik, potem `window-context.md`, `main-loop.md`, `input.md`, `gl-check.md`, `paths.md`.

## 2. Indeks: plik kodu, dokument

| Plik kodu | Co zawiera | Dokument |
|---|---|---|
| [`src/core/Window.hpp`](../../../src/core/Window.hpp), [`.cpp`](../../../src/core/Window.cpp) | RAII na okno GLFW i kontekst OpenGL 4.1 Core, ładowanie GLAD | [`window-context.md`](window-context.md) |
| [`src/core/Log.hpp`](../../../src/core/Log.hpp), [`.cpp`](../../../src/core/Log.cpp) | `logInfo`, `logWarn`, `logError` | [`window-context.md`](window-context.md), sekcja 5.5 |
| [`src/core/Application.hpp`](../../../src/core/Application.hpp), [`.cpp`](../../../src/core/Application.cpp) | klasa bazowa programu: posiada `Window`, `Input`, `Time`, prowadzi pętlę | [`main-loop.md`](main-loop.md), dziedziczenie w tym pliku (sekcja 6) |
| [`src/core/Time.hpp`](../../../src/core/Time.hpp), [`.cpp`](../../../src/core/Time.cpp) | czas klatki, akumulator stałego kroku, uśredniony FPS | [`main-loop.md`](main-loop.md) |
| [`src/core/Input.hpp`](../../../src/core/Input.hpp), [`.cpp`](../../../src/core/Input.cpp) | migawka stanu klawiatury i myszy: `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`, `isMouseButtonDown`, `wasMouseButtonPressed`, `mouseDeltaX`, `mouseDeltaY`, `setMouseBlocked`, `setCursorCaptured` | [`input.md`](input.md) |
| [`src/core/GlCheck.hpp`](../../../src/core/GlCheck.hpp), [`.cpp`](../../../src/core/GlCheck.cpp) | makro `GL_CHECK` i funkcja `checkGlErrors` | [`gl-check.md`](gl-check.md) |
| [`src/core/Paths.hpp`](../../../src/core/Paths.hpp), [`.cpp`](../../../src/core/Paths.cpp) | `executableDir` i `assetPath`: ścieżki do plików z `assets/` względem pliku wykonywalnego. Jedyny kod w `src/` z gałęziami `#if` dla macOS i Windows. Woła je konstruktor `NightMazeApp` przy wczytywaniu ośmiu plików shaderów (cztery pary), konstruktor `game::MazeRenderer` przy wczytywaniu trzech modeli labiryntu i konstruktor `game::GameplayRenderer` przy wczytywaniu trzech modeli rundy (dwa kryształy i brama). `pathText`: ścieżka jako tekst UTF-8, dla `gfx::Shader`, `assets::AssetCache` i paneli "Shaders" oraz "Assets" | [`paths.md`](paths.md) |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | gra: dziedziczy po `core::Application`. Posiada pięć programów shaderów, pamięć assetów (`assets::AssetCache`), `MazeRenderer`, `GameplayRenderer`, `ColliderLines`, `LightRig`, od M6 niebo (`Skybox` z ustawieniami `SkyboxSettings`), labirynt (`MazeSettings`, `MazeWorld`), liczby reguł i stan rundy (`GameplaySettings`, `Round`), listę przeszkód rundy, gracza (`Player`), kamerę (`scene::Camera`) i ustawienia oświetlenia (`LightingSettings`). W stałym kroku przesuwa gracza i woła reguły rundy (`updateRound`), co klatkę obsługuje prośbę o nowy labirynt, prośbę o nową rundę i klawisz R, klawisze N i F oraz mysz, ustawia viewport, włącza test głębi, czyści ekran, buduje i wysyła światła i rysuje labirynt razem z kryształami i bramą (z oświetleniem albo bez), na życzenie linie pudełek i kul kolizji, a na końcu niebo ([`../renderer/skybox.md`](../renderer/skybox.md)) | ten plik (sekcje 6 i 7), reguły rundy, kryształy, brama i HUD w [`../game/gameplay.md`](../game/gameplay.md), ruch gracza w [`../game/player.md`](../game/player.md), labirynt w świecie i jego rysowanie w [`../game/maze-rendering.md`](../game/maze-rendering.md), obrót myszą w [`../scene/camera-controls.md`](../scene/camera-controls.md), linie pudełek w [`../scene/collision.md`](../scene/collision.md), czyszczenie w [`window-context.md`](window-context.md), sekcja 3.2, macierze w [`../scene/camera.md`](../scene/camera.md), sekcja 5.7, latarka, klawisz F i zestaw świateł w [`../game/flashlight.md`](../game/flashlight.md), rodzaje świateł w [`../scene/lights.md`](../scene/lights.md), cztery tryby oświetlenia w [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), bufor świateł w [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md) |
| [`src/main.cpp`](../../../src/main.cpp) | klasa `DebugNightMazeApp` (gra plus nakładka debug i HUD) i `main`: tworzy aplikację, woła `run()`, łapie wyjątki | ten plik (sekcje 5 i 6), nakładka w [`../debug-ui.md`](../debug-ui.md) |

W [`CMakeLists.txt`](../../../CMakeLists.txt) pliki `src/core/*` tworzą, razem z `src/assets/*`, `src/gfx/*` i `src/scene/*`, bibliotekę statyczną `engine`. Część `game/`, która nie potrzebuje okna (`Crystals`, `Exit`, `Lighting`, `Maze`, `MazeGenerator`, `MazeLayout`, `MazeWorld`, `Player`, `Round`), tworzy bibliotekę `game_logic`, żeby mogły ją linkować także testy. Program `night_maze` to `main.cpp`, `debug/` i reszta `game/` (`NightMazeApp`, `MazeRenderer`, `GameplayRenderer`, `ModelDraw`, `ColliderLines`, `LightRig`, `ShaderUniforms.hpp`): linkuje obie biblioteki i ImGui. `engine` ma publiczne definicje `GLFW_INCLUDE_NONE` (GLFW nie dołącza systemowego nagłówka OpenGL, robi to GLAD) i `GL_SILENCE_DEPRECATION` (macOS oznacza cały OpenGL jako przestarzały i bez tej definicji zasypuje build ostrzeżeniami).

## 3. Warstwy

Architektura projektu (PRD, sekcja 6) ma warstwy z zależnościami w jedną stronę. Strzałka znaczy "zna i dołącza nagłówki":

```mermaid
flowchart TD
    Main["main.cpp<br/>DebugNightMazeApp, main"] --> Debug["debug/<br/>DebugUI, DebugContext, Hud, osiem paneli"]
    Main --> Game["game/<br/>NightMazeApp, Player, MazeWorld, Round, MazeRenderer, GameplayRenderer, ColliderLines, Lighting, LightRig"]
    Debug --> Core["core/<br/>Application, Window, Input, Time, Log, GL_CHECK, Paths"]
    Debug --> Game
    Game --> Core
    Game --> Assets
    Game --> Gfx
    Game --> Scene
    Debug --> Assets
    Debug --> Gfx
    Debug --> Scene
    Assets["assets/<br/>ObjLoader, ImageLoader, AssetCache"] --> Gfx
    Assets --> Core
    Gfx["gfx/<br/>Shader, ShaderSource, Buffer, UniformBuffer, VertexArray, Mesh, Texture2D"] --> Core
    Gfx --> Glm
    Scene["scene/<br/>Transform, Camera, Collider, Light, LightBlock"] --> Glm["GLM"]
    Debug --> ImGui["Dear ImGui"]
    Core --> Glfw["GLFW"]
    Core --> Glad["GLAD"]
    Gfx --> Glad
```

Sześć rzeczy do zapamiętania:

1. `core/` nie zna ani `gfx/`, ani `game/`, ani `debug/`, ani ImGui. Biblioteka `engine` linkuje tylko `glad`, `glfw` i nagłówki GLM (`glm::glm-header-only`). Samo `core/` z GLM nie korzysta: dołączają je warstwy `scene/` i `gfx/`.
2. `game/` zna `core/`, `gfx/` i `scene/`, ale nie zna `debug/`.
3. `debug/` może zależeć od wszystkiego, ale nic nie może zależeć od `debug/`. Panele dołączają nagłówki z `game/` (na przykład `game/Player.hpp` w panelu "Camera" i `game/MazeWorld.hpp` w panelu "Maze"), bo pokazują i edytują stan gry. Tak samo HUD (`debug/Hud.cpp`) dołącza `game/Round.hpp`: należy do gry, ale leży w `debug/`, bo tylko tam wolno dołączać ImGui. W drugą stronę zależności nie ma: żaden plik w `game/` nie dołącza niczego z `debug/`. Jedynym plikiem, który tworzy obiekty obu warstw i łączy je ze sobą, jest `main.cpp`.
4. `gfx/` (opakowania obiektów OpenGL: klasy `Shader`, `Buffer`, `UniformBuffer`, `VertexArray`, `Mesh` i `Texture2D`, a do tego funkcje `ShaderSource` rozwijające `#include` w plikach shaderów) zna tylko `core/`, GLAD i GLM (typy w setterach `Shader`). Używa go `game/` (rysowanie), `assets/` (pamięć assetów tworzy siatki i tekstury) i `debug/` (panel "Shaders" woła `Shader::reload()`, panel "Assets" pokazuje tekstury). Opis warstwy: [`../gfx/README.md`](../gfx/README.md).
5. `scene/` (struktury `Transform` i `Camera`: macierze modelu, widoku i rzutowania) to sama matematyka na GLM. Może zależeć od `core/` i `gfx/`, dziś dołącza tylko GLM. Do `scene/` należą też pudełka kolizji (`Aabb`, `moveAndSlide`), od M5 kule (`Sphere` i dwa testy `overlaps`), a od M4 światła jako zwykłe dane (`Light.hpp`: trzy rodzaje świateł i `LightSet`) oraz ich układ bajtów dla karty (`LightBlock.hpp`), nadal tylko na GLM. Używa go `game/`: `NightMazeApp` ma kamerę, obraca ją myszą i co klatkę wysyła macierze do shaderów, `Player` przesuwa się przez `moveAndSlide`, `MazeWorld` i `GameplayRenderer` liczą macierze modelu przez `Transform`, `updateRound` sprawdza zebranie kryształu i wejście do wyjścia testami `overlaps`, a `buildLightSet` składa `scene::LightSet`. Używa go też `debug/`: panel "Camera" edytuje pola kamery, a panel "Lights" czyta stałą `scene::MAX_POINT_LIGHTS`. Opis warstwy: [`../scene/README.md`](../scene/README.md).
6. `assets/` (loadery plików i `AssetCache`) zna `core/` (logowanie, ścieżki) i `gfx/` (siatka i tekstura na karcie). Same loadery zwracają dane bez OpenGL, a `AssetCache` robi z nich obiekty `gfx`. Opis warstwy: [`../assets/README.md`](../assets/README.md).

Ta reguła tłumaczy dwie decyzje opisane niżej: dlaczego nakładka debug jest podpinana w `main.cpp` (sekcja 6) i dlaczego `core::Input` dostaje od `main.cpp` neutralne flagi "klawiatura zablokowana" i "mysz zablokowana", zamiast samemu pytać ImGui ([`input.md`](input.md), sekcje 5.6 i 5.10).

## 4. Klatka jako całość

Jeden obrót pętli głównej w działającym programie, czyli w obiekcie klasy `DebugNightMazeApp`. Szczegóły każdego kroku są w dokumentach tematycznych, tutaj chodzi o kolejność:

```mermaid
sequenceDiagram
    participant Run as Application run
    participant Win as Window
    participant In as Input
    participant T as Time
    participant App as DebugNightMazeApp
    participant UI as DebugUI
    Note over Run,T: raz przed pętlą: m_time.reset()
    Run->>Win: shouldClose()
    Run->>Win: pollEvents()
    Run->>In: update()
    Run->>In: wasKeyPressed(GLFW_KEY_ESCAPE)
    alt Escape i kursor przechwycony
        Run->>In: setCursorCaptured(false)
    else Escape i kursor wolny
        Run->>Win: requestClose()
    end
    Run->>T: beginFrame()
    loop dopóki consumeFixedStep() zwraca true
        Run->>App: onUpdate(Time::FIXED_DT)
        App->>In: isCursorCaptured(), isKeyDown(W, S, A, D, spacja, lewy Shift), czyli PlayerInput
        App->>App: m_player.update(...) z listą m_obstacles, czyli krok gracza z kolizjami, potem kamera w oczach gracza
        App->>App: updateRound(...), czyli reguły rundy, a gdy brama się otworzyła, nowa lista m_obstacles
    end
    Run->>T: alpha()
    Run->>App: onRender(alpha)
    App->>App: NightMazeApp onRender, na początku prośba o nowy labirynt (m_mazeSettings.regenerate)
    App->>In: m_gameplay.restart albo wasKeyPressed(GLFW_KEY_R), czyli beginRound
    App->>In: wasKeyPressed(GLFW_KEY_N), czyli przełączenie noclip
    App->>In: wasKeyPressed(GLFW_KEY_F), czyli przełączenie latarki
    App->>In: kursor wolny, to wasMouseButtonPressed(lewy) i setCursorCaptured(true)
    App->>In: kursor przechwycony, to mouseDeltaX(), mouseDeltaY(), czyli obrót kamery
    App->>App: glViewport, glEnable(GL_DEPTH_TEST), glClearColor, glClear
    App->>App: gdy jest co rysować, to stopy z glm mix i alpha, oko, macierze view i projection
    App->>App: lightingForFrame, crystalLightPositions, buildLightSet z oka i kierunku kamery, m_lightRig.upload
    App->>App: drawMaze, czyli drawUnlitMaze albo drawLitMaze według trybu oświetlenia, w obu labirynt, brama i kryształy
    App->>App: przy włączonym przełączniku drawColliderLines
    App->>In: wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)
    App->>In: isCursorCaptured()
    App->>UI: setMouseEnabled(kursor nieprzechwycony)
    App->>UI: draw(DebugContext z 18 referencjami do stanu gry), czyli panele, gdy są widoczne, i zawsze HUD
    App->>UI: wantsKeyboard()
    App->>In: setKeyboardBlocked(...)
    App->>UI: wantsMouse()
    App->>In: setMouseBlocked(...)
    Run->>Win: swapBuffers()
```

| Krok | Kod | Dokument |
|---|---|---|
| Start zegara, raz przed pętlą (nie należy do obrotu) | `m_time.reset()` | [`main-loop.md`](main-loop.md), sekcja 5.4 |
| Zdarzenia systemu | `m_window.pollEvents()` | [`window-context.md`](window-context.md) |
| Migawka klawiatury i myszy, Escape (zwalnia przechwycony kursor albo zamyka program) | `m_input.update()`, `wasKeyPressed(GLFW_KEY_ESCAPE)`, `isCursorCaptured()` | [`input.md`](input.md), sekcja 5.7 |
| Pomiar czasu, kroki symulacji | `m_time.beginFrame()`, `consumeFixedStep()`, `onUpdate` | [`main-loop.md`](main-loop.md) |
| Symulacja: krok gracza | `NightMazeApp::onUpdate`: zapamiętanie poprzedniej pozycji gracza, przy przechwyconym kursorze `isKeyDown` dla sześciu klawiszy wpisane do `PlayerInput`, `m_player.update(...)` z listą `m_obstacles`, potem kamera w oczach gracza | sekcja 6.5, [`../game/player.md`](../game/player.md), sekcja 5 |
| Symulacja: krok rundy | `NightMazeApp::onUpdate`: `updateRound(...)` z pozycją gracza po ruchu, z przełącznikiem latarki przez referencję i z `fixedDt`, a gdy `gateBlocks` zmieniło wynik, nowa lista `m_obstacles` z `roundObstacles` | sekcja 6.5, [`../game/gameplay.md`](../game/gameplay.md) |
| Nowy labirynt, jeśli panel o niego poprosił | początek `NightMazeApp::onRender`: `m_mazeSettings.regenerate`, `regenerateMaze()` | sekcje 6.4 i 6.6, [`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5 |
| Nowa runda na tym samym labiryncie | `m_gameplay.restart` (przycisk `Restart round (key R)` panelu "Gameplay") albo `wasKeyPressed(RESTART_KEY)`: `beginRound()` | sekcje 6.4 i 6.6, [`../game/gameplay.md`](../game/gameplay.md) |
| Klawisz N | `wasKeyPressed(NOCLIP_KEY)` przełącza `m_player.noclip` | sekcja 6.6, [`../game/player.md`](../game/player.md), sekcja 5 |
| Klawisz F | `wasKeyPressed(FLASHLIGHT_KEY)` przełącza `m_lighting.flashlightOn` | sekcja 6.6, [`../game/flashlight.md`](../game/flashlight.md) |
| Mysz kamery | kliknięcie w scenę przechwytuje kursor, przy przechwyconym kursorze przesunięcie myszy obraca kamerę | [`input.md`](input.md), sekcja 5.9, [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 5 |
| Rysowanie gry: stan i tło | `NightMazeApp::onRender`: `glViewport`, `glEnable(GL_DEPTH_TEST)`, `glClearColor`, `glClear` (kolor i głębia) w `GL_CHECK` | [`window-context.md`](window-context.md), [`gl-check.md`](gl-check.md) |
| Rysowanie gry: oko, macierze, światła | pominięte, gdy framebuffer ma szerokość albo wysokość 0. Inaczej pozycja stóp z `glm::mix` i `alpha`, oko o `Player::EYE_HEIGHT` wyżej, macierze `view` i `projection` liczone raz, potem `lightingForFrame(...)` (kopia ustawień z baterią i pulsowaniem), `crystalLightPositions(m_round)`, `buildLightSet(...)` z tego samego oka i z `m_camera.forward()` oraz `m_lightRig.upload(lights, eye)`: światła klatki trafiają do bufora uniformów, w każdym trybie oświetlenia | sekcja 6.6, [`../scene/camera.md`](../scene/camera.md), sekcja 5.7, [`../game/flashlight.md`](../game/flashlight.md), [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md) |
| Rysowanie gry: scena | `drawMaze` (wybiera `drawUnlitMaze` albo `drawLitMaze`, a każda z nich rysuje labirynt przez `m_mazeRenderer` i zaraz po nim bramę i kryształy przez `m_gameplayRenderer`) i, gdy `m_drawColliders` jest prawdą, `drawColliderLines`. Każda funkcja rysująca pomija swoją część, gdy jej program shaderów nie jest poprawny | sekcje 6.6 i 6.7, [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md) |
| Przełącznik paneli, mysz dla ImGui, panele i HUD | `wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)`, `m_debugUI.setMouseEnabled(!input().isCursorCaptured())`, `m_debugUI.draw(...)`: panele tylko wtedy, gdy są widoczne, HUD (`debug::drawHud`) w każdej klatce | [`../debug-ui.md`](../debug-ui.md), sekcja 5.6 |
| Blokada klawiatury na następną klatkę | `input().setKeyboardBlocked(m_debugUI.wantsKeyboard())` | [`input.md`](input.md), sekcja 5.6 |
| Blokada myszy na następną klatkę | `input().setMouseBlocked(m_debugUI.wantsMouse())` | [`input.md`](input.md), sekcja 5.10 |
| Zamiana buforów | `m_window.swapBuffers()` | [`window-context.md`](window-context.md) |

## 5. Od `main` do pierwszej klatki

```cpp
int main() {
    try {
        DebugNightMazeApp app;
        app.run();
    } catch (const std::exception& error) {
        // Startup failures (no window, no OpenGL 4.1) arrive here as exceptions.
        core::logError(std::string("Fatal: ") + error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

`app` jest zwykłą zmienną lokalną wewnątrz bloku `try`. Konstruktor może rzucić `std::runtime_error` (brak GLFW, brak kontekstu 4.1, nieudany GLAD) i wtedy `catch` wypisuje powód oraz zwraca kod błędu. Przy normalnym wyjściu z `run()` destruktor `app` uruchamia się na końcu bloku `try` i sprząta ImGui, okno oraz GLFW. Nie ma tu ani jednego `new`, ani jednej zmiennej globalnej. Czym jest `DebugNightMazeApp`, wyjaśnia następna sekcja.

## 6. Jak program dziedziczy po `core::Application`

### 6.1 Dwa haki klasy bazowej

`Application` implementuje wzorzec **metody szablonowej** (template method): klasa bazowa ustala niezmienny szkielet klatki w `run()`, a klasa pochodna wypełnia dwa "haki":

```cpp
virtual void onUpdate(double fixedDt) = 0;
virtual void onRender(double alpha) = 0;
```

`= 0` oznacza funkcję czysto wirtualną: `Application` jest klasą abstrakcyjną i nie da się utworzyć jej obiektu. `NightMazeApp` nadpisuje obie funkcje (słowo `override` każe kompilatorowi sprawdzić, że sygnatura naprawdę zgadza się z bazową). Dostęp do okna, wejścia i zegara klasa pochodna ma przez chronione akcesory `window()`, `input()`, `time()`, a same pola są prywatne, więc pochodna nie może ich podmienić ani zniszczyć. `window()` i `input()` zwracają zwykłe referencje, a `time()` referencję `const`: klasa pochodna może zegar czytać, ale nie może wołać `reset()`, `beginFrame()` ani `consumeFixedStep()`, bo zegar ustawia i przesuwa wyłącznie `run()`.

### 6.2 Klasa `NightMazeApp`: co posiada i co udostępnia

```cpp
/// The game itself: a generated maze of textured walls, pillars and floor tiles at
/// night, and a player who walks through it in first person without passing through the
/// walls. The mouse turns the camera, the keyboard moves the player. The maze is lit by
/// the moon, by the flashlight of the player and by the glowing crystals.
///
/// A round: the player collects crystals, each one charges the battery of the
/// flashlight, and when enough of them are collected the gate of the exit opens.
/// Walking through it wins the round. The rules are in game/Round.hpp, this class feeds
/// them the position of the player and draws their state.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels and the HUD on top of the frame.
class NightMazeApp : public core::Application {
```

Klasa jest miejscem, w którym spotykają się wszystkie warstwy: shadery, siatki i bufor uniformów z `gfx/`, modele i tekstury z `assets/`, kamera, kolizje i światła z `scene/`, labirynt, gracz, runda i ustawienia oświetlenia z `game/`. Sama zawiera mało logiki. Jej zadaniem jest **kolejność**: co powstaje po czym (sekcje 6.3 i 7), co dzieje się w stałym kroku (sekcja 6.5), a co raz na klatkę (sekcje 6.6 i 6.7). Drugi akapit komentarza mówi to samo o rundzie: reguły są w `game/Round.hpp` (zwykłe funkcje bez OpenGL, z testami), a ta klasa tylko podaje im pozycję gracza i rysuje ich stan. Same reguły opisuje [`../game/gameplay.md`](../game/gameplay.md), sekcja 2.

Do M4 klasa trzymała też kostkę z M1 (własny VAO, dwa bufory, program `basic`) i rysowała ją nad narożną komórką jako znacznik przyszłego wyjścia. W M5 kostka zniknęła razem z programem `basic`: wyjście ma teraz bramę, a ręcznie pisane dane wierzchołków zostały w projekcie tylko w [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp).

Stan gry jest prywatny. Klasa pochodna (`DebugNightMazeApp` z `main.cpp`) dostaje go przez szesnaście chronionych akcesorów, tu w kolejności z nagłówka. Każdy zwraca referencję do jednego pola, więc panel edytuje oryginał, a nie kopię:

| Akcesor | Typ wyniku | Pole | Kto z niego korzysta |
|---|---|---|---|
| `clearColor()` | `std::array<float, 3>&` | `m_clearColor` | panel "Renderer" |
| `texturedShader()` | `gfx::Shader&` | `m_texturedShader` (program sceny bez oświetlenia i widoków do szukania błędów, `textured.*`) | panel "Shaders" |
| `colorShader()` | `gfx::Shader&` | `m_colorShader` (program linii pudełek i kul kolizji, `color.*`) | panel "Shaders" |
| `litShader()` | `gfx::Shader&` | `m_litShader` (program sceny z oświetleniem liczonym dla fragmentu: tryby Phong i Blinn-Phong, `lit.*`) | panel "Shaders" |
| `gouraudShader()` | `gfx::Shader&` | `m_gouraudShader` (program sceny z oświetleniem liczonym dla wierzchołka: tryb Gouraud, `gouraud.*`) | panel "Shaders" |
| `lighting()` | `LightingSettings&` | `m_lighting` (tryb oświetlenia i ustawienia wszystkich świateł) | panele "Lights" i "Renderer" (lista `Lighting` zmienia pole `mode`) |
| `camera()` | `scene::Camera&` | `m_camera` | panele "Camera" i "Maze" |
| `mouseSensitivity()` | `float&` | `m_mouseSensitivity` | panel "Camera" |
| `player()` | `Player&` | `m_player` | panele "Camera", "Maze" i "Collision" |
| `mazeSettings()` | `MazeSettings&` | `m_mazeSettings` | panel "Maze" |
| `mazeWorld()` | `const MazeWorld&` | `m_mazeWorld` | panele "Maze" i "Collision" |
| `gameplaySettings()` | `GameplaySettings&` | `m_gameplay` (liczby reguł rundy i flaga `restart`) | panel "Gameplay" i HUD (czyta próg słabej baterii) |
| `round()` | `Round&` | `m_round` (stan rundy) | HUD oraz panele "Gameplay", "Maze", "Collision" i "Lights" |
| `assets()` | `assets::AssetCache&` | `m_assets` | panel "Assets" |
| `viewMode()` | `ViewMode&` | `m_viewMode` | panel "Assets" |
| `drawColliders()` | `bool&` | `m_drawColliders` | panel "Collision" |

Jedyny akcesor z `const` to `mazeWorld()`:

```cpp
/// The maze in play, read only: the debug UI draws its plan and counts its boxes.
const MazeWorld& mazeWorld() const { return m_mazeWorld; }
```

Panel nie może zmienić labiryntu, w którym gracz właśnie stoi. Może tylko zapisać prośbę w `MazeSettings`, a o tym, kiedy labirynt zostanie wymieniony, decyduje gra (sekcja 6.6).

Dwa akcesory z M5:

```cpp
    /// The numbers of the rules of a round and the request for a restart, exposed so
    /// the debug UI can edit them live.
    GameplaySettings& gameplaySettings() { return m_gameplay; }

    /// The round in play, exposed so the HUD can show it and the debug UI can set the
    /// charge of the battery.
    Round& round() { return m_round; }
```

`round()` nie ma `const`, choć prawie wszyscy tylko czytają rundę: panel "Gameplay" ma suwak `Battery`, który zapisuje pole `Round::battery`. Pozostałe panele i HUD dostają od `DebugUI::draw` referencję `const`. Nową rundę panel zamawia tak samo jak nowy labirynt, flagą: `GameplaySettings::restart` (sekcja 6.6).

Jak te referencje trafiają do paneli, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.

**Kolor tła** zmienił się w M4 razem ze sceną. Pole i jego wartość startowa:

```cpp
    // The night sky: a very dark blue, darker than the ambient light on the stone, so
    // the walls stand out against it.
    std::array<float, 3> m_clearColor{0.01F, 0.015F, 0.04F};
```

Wcześniej było `{0.02F, 0.03F, 0.08F}`, czyli dwa razy jaśniej. Powód stoi w komentarzu: niebo ma być ciemniejsze niż kamień oświetlony samym światłem otoczenia (`LightingSettings::ambient` to `{0.035F, 0.045F, 0.075F}`), żeby ściany odcinały się od tła także tam, gdzie nie dociera żadne inne światło. Kolor nadal da się zmienić w panelu "Renderer".

**Pole stanu z M4: `m_lighting`.**

```cpp
    // The lighting: how the scene is shaded and the settings of every light. The lights
    // of a frame are built from it in onRender.
    LightingSettings m_lighting;
```

To zwykła struktura z wartościami domyślnymi nocnej sceny (tryb `BlinnPhong`, latarka włączona), bez OpenGL. Stoi między `m_camera` a `m_viewMode`. Jej pola opisuje [`../game/flashlight.md`](../game/flashlight.md). Od M5 jedno jej pole zmienia też sama gra: `flashlightOn` włącza `beginRound` (sekcja 6.4), a wyłącza `updateRound`, gdy bateria jest pusta (sekcja 6.5).

**Pola stanu z M5: `m_gameplay`, `m_round`, `m_obstacles`.**

```cpp
    // The numbers of the rules (edited by the debug UI) and the round in play. The round
    // is simulation state: onUpdate advances it in fixed steps. It starts empty and is
    // filled by beginRound in the constructor.
    GameplaySettings m_gameplay;
    Round m_round;

    // What the player cannot walk through in this round: the boxes of the maze, plus the
    // box of the gate while it is closed (game::roundObstacles). A copy that is rebuilt
    // only when it changes: at the start of a round and when the gate opens.
    std::vector<scene::Aabb> m_obstacles;
```

| Pole | Co to jest | Kto je zmienia |
|---|---|---|
| `m_gameplay` | liczby reguł, które da się zmieniać w biegu (`requiredFraction`, `batteryLifetimeSeconds`, `batteryPerCrystal`, `lowBatteryThreshold`, `pickupRadius`, `batteryDrains`), i flaga `restart` | panel "Gameplay", a flagę `restart` gasi `onRender` |
| `m_round` | stan jednej rozgrywki: kryształy i to, które są zebrane, liczniki, brama, bateria, dwa zegary. To stan **symulacji**, tak jak gracz | `beginRound` (cały, przez przypisanie), `updateRound` w każdym stałym kroku, suwak `Battery` panelu "Gameplay" |
| `m_obstacles` | lista pudełek, przez które gracz nie przejdzie: pudełka labiryntu i, dopóki brama jest zamknięta, pudełko bramy na końcu | `beginRound` i `onUpdate` w kroku, w którym brama się otworzyła |

Wszystkie trzy to zwykłe dane bez OpenGL. `m_round` zaczyna jako pusta struktura (konstruktor domyślny: zero kryształów) i dostaje prawdziwą zawartość dopiero w ciele konstruktora, w `beginRound`. `m_obstacles` jest **kopią**: `MazeWorld::colliders` trzyma tylko przeszkody, które nigdy się nie zmieniają (ściany i słupki), a brama przestaje być przeszkodą w trakcie rundy, więc lista dla gracza musi być osobna. Kopię buduję dwa razy na rundę, a nie w każdym kroku.

### 6.3 Konstruktor

```cpp
NightMazeApp::NightMazeApp()
    : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze"),
      m_texturedShader(core::assetPath(TEXTURED_VERTEX_SHADER_FILE),
                       core::assetPath(TEXTURED_FRAGMENT_SHADER_FILE)),
      m_colorShader(core::assetPath(COLOR_VERTEX_SHADER_FILE),
                    core::assetPath(COLOR_FRAGMENT_SHADER_FILE)),
      m_litShader(core::assetPath(LIT_VERTEX_SHADER_FILE),
                  core::assetPath(LIT_FRAGMENT_SHADER_FILE)),
      m_gouraudShader(core::assetPath(GOURAUD_VERTEX_SHADER_FILE),
                      core::assetPath(GOURAUD_FRAGMENT_SHADER_FILE)),
      m_skyboxShader(core::assetPath(SKYBOX_VERTEX_SHADER_FILE),
                     core::assetPath(SKYBOX_FRAGMENT_SHADER_FILE)),
      m_mazeRenderer(m_assets),
      m_gameplayRenderer(m_assets),
      m_mazeWorld(
          buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed)) {
    // The two lit programs read the lights from the uniform buffer of m_lightRig. Each
    // program is told once: the shader repeats it by itself after a reload.
    m_lightRig.connect(m_litShader);
    m_lightRig.connect(m_gouraudShader);

    // The first maze was built in the initializer list, because MazeWorld cannot be
    // created empty. What is left is the same as after every later regeneration.
    beginRound();
}
```

| Element listy | Znaczenie |
|---|---|
| `core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze")` | klasa bazowa tworzy okno 1280 x 720 i kontekst OpenGL. `INITIAL_WIDTH` i `INITIAL_HEIGHT` to stałe `constexpr` w anonimowej przestrzeni nazw pliku `.cpp`: mają nazwy (żadnych magicznych liczb) i są niewidoczne poza tym plikiem |
| `m_texturedShader(...)`, `m_colorShader(...)`, `m_litShader(...)`, `m_gouraudShader(...)`, `m_skyboxShader(...)` | pięć programów, każdy z pary plików z `assets/shaders/` (`textured`, `color`, `lit`, `gouraud`, a od M6 `skybox`). Nazwy plików to dziesięć stałych `constexpr const char*` na górze pliku `.cpp`. `core::assetPath` zamienia nazwę względną na ścieżkę obok pliku wykonywalnego ([`paths.md`](paths.md)). Programy `lit` i `gouraud` dołączają wspólny plik `common/lighting.glsl`, którego konstruktor tu nie wymienia: znajduje go sam loader shaderów ([`../gfx/shader-includes.md`](../gfx/shader-includes.md)). Nieudane wczytanie nie rzuca wyjątku: program jest wtedy niepoprawny, a jego część klatki nie jest rysowana (sekcja 6.7) |
| (brak `m_assets`) | pola `m_assets` nie ma na liście, więc działa jego konstruktor domyślny: tworzy białą teksturę 1 x 1. Powstaje mimo to w swojej kolejności, między `m_gouraudShader` a `m_mazeRenderer`, bo o kolejności decydują deklaracje (sekcja 7) |
| `m_mazeRenderer(m_assets)` | prosi pamięć assetów o trzy modele labiryntu. `m_assets` już istnieje, bo jest zadeklarowane wyżej ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5) |
| `m_gameplayRenderer(m_assets)` | prosi tę samą pamięć assetów o trzy modele rundy: `models/crystal_a.obj`, `models/crystal_b.obj` i `models/gate.obj`. Model, którego nie udało się wczytać, daje pusty wskaźnik i po prostu nie jest rysowany ([`../game/gameplay.md`](../game/gameplay.md), sekcja 5) |
| (brak `m_colliderLines`, `m_lightRig` i `m_skybox`) | konstruktory domyślne (`Skybox` wczytuje w swoim sześć obrazów nieba i tworzy teksturę sześcienną oraz siatkę sześcianu, [`../renderer/skybox.md`](../renderer/skybox.md), sekcja 5.4): `ColliderLines` wysyła na kartę dwie małe siatki linii (krawędzie sześcianu o boku 1 i okrąg o promieniu 1), `LightRig` tworzy bufor uniformów na światła (rozmiar struktury `scene::LightBlockData`, punkt wiązania `LIGHT_BLOCK_BINDING_POINT`, czyli 1). Od M5 `LightRig` nie ma już żadnej siatki |
| `m_mazeWorld(buildMazeWorld(...))` | pierwszy labirynt: rozmiar i ziarno z `m_mazeSettings`, którego wartości domyślne to 10 x 10 komórek i ziarno 1 (`DEFAULT_MAZE_WIDTH`, `DEFAULT_MAZE_HEIGHT`, `DEFAULT_MAZE_SEED`). `m_mazeSettings` jest zadeklarowane tuż nad `m_mazeWorld`, więc w tej chwili ma już swoje wartości. Od M5 `buildMazeWorld` wybiera też komórkę wyjścia, stawia bramę i rozmieszcza kryształy (pola `exitCell`, `gate`, `gateBox`, `exitZone`, `crystals`), więc każdy nowy labirynt przychodzi od razu z tym, czego potrzebuje runda. W M4 były tu pozycje świateł w ślepych zaułkach: dziś światła wiszą nad kryształami, a ich pozycje liczy co klatkę `crystalLightPositions` z rundy (sekcja 6.6) |
| (brak `m_gameplay`, `m_round`, `m_obstacles` i pozostałych) | wartości z deklaracji: `m_gameplay` ma domyślne liczby reguł, `m_round` i `m_obstacles` są puste do `beginRound()` |

**Dlaczego labirynt powstaje na liście, a nie w ciele konstruktora.** `MazeWorld` ma tylko konstruktor `explicit MazeWorld(Maze generatedMaze)`, a `Maze` nie ma konstruktora domyślnego (labirynt bez rozmiaru nie ma sensu). Pola bez konstruktora domyślnego nie da się "zostawić pustego i wypełnić później": musi dostać wartość na liście inicjalizacyjnej. Dlatego na ciało konstruktora zostaje już tylko `beginRound()`, czyli to samo, co dzieje się po każdej późniejszej wymianie labiryntu.

Ciało konstruktora robi dwie rzeczy: łączy dwa programy z oświetleniem z buforem świateł i woła `beginRound()`. Do M4 opisywało tu jeszcze atrybuty kostki i ustawiało jej obrót. Po usunięciu kostki w ciele nie ma ani jednego wywołania, które zależałoby od tego, co jest akurat związane w OpenGL.

**Dwa wywołania `m_lightRig.connect(...)`.**

```cpp
void LightRig::connect(gfx::Shader& shader) const {
    shader.bindUniformBlock(LIGHT_BLOCK_NAME, m_lightBuffer.bindingPoint(),
                            m_lightBuffer.sizeInBytes());
}
```

| Pytanie | Odpowiedź |
|---|---|
| Co robi jedno wywołanie | mówi programowi, że jego blok uniformów `LightBlock` ma czytać dane z punktu wiązania, pod którym `m_lightRig` trzyma swój bufor. Samych świateł jeszcze nie wysyła: to robi `upload` w każdej klatce |
| Dlaczego dwa razy | powiązanie bloku z punktem wiązania jest stanem **programu**, a programy z oświetleniem są dwa (`lit` i `gouraud`). Bufor jest jeden i oba czytają z niego to samo |
| Dlaczego tylko raz, w konstruktorze, a nie co klatkę | `gfx::Shader` zapamiętuje tę prośbę i powtarza ją sam na nowym programie po każdym `reload()` (komentarz w kodzie). Bez tego przycisk "Reload shaders" zostawiałby program z oświetleniem bez świateł |
| Dlaczego parametr to `gfx::Shader&` bez `const` | shader zapisuje u siebie zapamiętaną prośbę, więc się zmienia. `connect` jest przy tym funkcją `const` klasy `LightRig`: sam `LightRig` się nie zmienia |
| Dlaczego nie ma `connect` dla `textured` i `color` | te programy nie mają bloku `LightBlock`. Komentarz w `LightRig.hpp` mówi, że program bez bloku zostałby pominięty, ale kod nawet nie próbuje |
| Dlaczego to stoi w ciele, a nie na liście inicjalizacyjnej | to wywołania funkcji na gotowych polach, nie konstrukcja pola. `m_lightRig`, `m_litShader` i `m_gouraudShader` istnieją już wszystkie, gdy ciało się zaczyna |

Mechanizm (blok uniformów, punkt wiązania, `glUniformBlockBinding`, dlaczego nie `layout(binding = N)`) opisuje [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md).

### 6.4 `regenerateMaze` i `beginRound`

```cpp
void NightMazeApp::regenerateMaze() {
    // generateMaze throws for a size outside 1 to Maze::MAX_SIZE. The request comes from
    // a panel, where any number can be typed, so it is brought into the range here and
    // written back for the panel to show.
    m_mazeSettings.width = std::clamp(m_mazeSettings.width, 1, Maze::MAX_SIZE);
    m_mazeSettings.height = std::clamp(m_mazeSettings.height, 1, Maze::MAX_SIZE);

    // Replaces the maze, the model matrices, the collision boxes, the exit and the
    // crystals in one assignment. A new maze is a new round.
    m_mazeWorld = buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed);
    beginRound();
}
```

| Linia | Znaczenie |
|---|---|
| `std::clamp(m_mazeSettings.width, 1, Maze::MAX_SIZE)` | `generateMaze` rzuca `std::invalid_argument` dla rozmiaru spoza zakresu od 1 do `Maze::MAX_SIZE` (256). Wyjątek w środku klatki zakończyłby program w `main`, więc gra sama sprowadza prośbę do zakresu i zapisuje wynik z powrotem, żeby panel pokazał to, co naprawdę zostało użyte. Suwaki panelu "Maze" mają węższy zakres (od 2 do 40) i same przycinają wpisaną wartość, ale gra nie polega na tym, co robi panel |
| `m_mazeWorld = buildMazeWorld(...)` | jedno przypisanie wymienia siatkę, listy ścian i słupków, macierze modelu, pudełka kolizji, wyjście z bramą i kryształy. Nie ma chwili, w której część danych jest stara, a część nowa |
| `beginRound();` | nowy labirynt to nowa runda. Reszta pracy jest wspólna z konstruktorem i z klawiszem R. Bez tego wywołania `m_round` trzymałoby kryształy starego labiryntu, a `m_obstacles` jego pudełka |

```cpp
void NightMazeApp::beginRound() {
    // The state of the round: every crystal back, a full battery, the gate closed.
    m_round = startRound(m_mazeWorld, m_gameplay);
    m_obstacles = roundObstacles(m_mazeWorld, m_round);
    // A round starts with the light on, also after one that ended in the dark.
    m_lighting.flashlightOn = true;

    // The player goes to the start. After a regeneration the old position may be inside
    // a wall of the new maze, or outside of it.
    m_player.position = m_mazeWorld.startPosition;
    // Both positions at once: otherwise the next frame would be drawn from a point
    // between the old place and the new one, a visible swoop through the walls.
    m_previousPlayerPosition = m_player.position;

    m_camera.position = m_player.eyePosition();
    m_camera.yawDegrees = m_mazeWorld.startYawDegrees;
    m_camera.pitchDegrees = LEVEL_PITCH_DEGREES;
}
```

| Linia | Znaczenie |
|---|---|
| `m_round = startRound(m_mazeWorld, m_gameplay);` | nowy stan rundy w jednym przypisaniu: każdy kryształ z `MazeWorld::crystals` z powrotem na miejscu, pełna bateria, brama zamknięta, oba zegary na 0. Liczba kryształów potrzebnych do otwarcia bramy jest liczona tutaj z `m_gameplay.requiredFraction`. Dla labiryntu startowego (10 x 10, ziarno 1) to 13 kryształów, z których 10 otwiera bramę |
| `m_obstacles = roundObstacles(m_mazeWorld, m_round);` | lista przeszkód tej rundy: kopia `m_mazeWorld.colliders` i pudełko zamkniętej bramy (`MazeWorld::gateBox`) jako ostatnie. Musi stać **po** linii wyżej, bo `roundObstacles` pyta rundę, czy brama blokuje drogę |
| `m_lighting.flashlightOn = true;` | runda zaczyna się z włączoną latarką, także wtedy, gdy poprzednia skończyła się w ciemności z pustą baterią. To jedyne pole `m_lighting`, które `beginRound` rusza |
| `m_player.position = m_mazeWorld.startPosition;` | stopy gracza na środku komórki (0, 0), czyli w `(1, 0, 1)` |
| `m_previousPlayerPosition = m_player.position;` | obie pozycje pary do interpolacji naraz. Gdyby poprzednia została stara, najbliższa klatka byłaby narysowana z punktu między starym a nowym miejscem |
| `m_camera.position = m_player.eyePosition();` | oko 1,7 m nad stopami: `(1, 1,7, 1)` |
| `m_camera.yawDegrees = m_mazeWorld.startYawDegrees;` | gracz patrzy w pierwszy otwarty bok komórki startowej |
| `m_camera.pitchDegrees = LEVEL_PITCH_DEGREES;` | wzrok poziomo (`0.0F`) |

**Kto woła `beginRound`.** Trzy miejsca, wymienione w komentarzu w nagłówku: konstruktor (pierwszy labirynt), `regenerateMaze` (każdy następny) i początek `onRender`, gdy gracz nacisnął R albo panel "Gameplay" ustawił flagę `restart` (sekcja 6.6). W dwóch pierwszych labirynt jest nowy, w trzecim ten sam: `beginRound` czyta `m_mazeWorld` i go nie zmienia, więc kryształy wracają dokładnie na te same miejsca.

**Czego `beginRound` nie rusza.** Tryb noclip, trzy prędkości gracza, tryb widoku (`m_viewMode`), przełącznik rysowania kształtów kolizji (`m_drawColliders`), czułość myszy, liczby reguł w `m_gameplay` i wszystkie ustawienia oświetlenia poza przełącznikiem latarki (tryb, kolory, moce) zostają takie, jakie były. Nowa runda wymienia stan rundy i listę przeszkód, włącza latarkę i przestawia pozycję gracza, pozycję poprzednią, pozycję kamery, yaw i pitch. Skutek: po wymianie labiryntu w trybie noclip gracz dalej lata, tylko z nowego miejsca, a suwaki panelu "Gameplay" nie wracają do wartości domyślnych.

Do M4 ta funkcja nazywała się `enterMaze` i poza graczem i kamerą ustawiała tylko kostkę nad komórką wyjścia.

Skąd biorą się `startPosition` i `startYawDegrees`, opisuje [`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5, a co dokładnie robią `startRound` i `roundObstacles`, [`../game/gameplay.md`](../game/gameplay.md), sekcja 5.

### 6.5 `onUpdate`: jeden stały krok

```cpp
void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the player was before this step. It is done in every step, also
    // when the player does not move, so that onRender never blends with an old position.
    m_previousPlayerPosition = m_player.position;

    // The keys reach the player only while the cursor is captured: one click in the scene
    // switches on both mouse look and movement, Escape switches both off. Without the
    // capture the struct stays as it is created: nothing is held.
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = input().isKeyDown(GLFW_KEY_W);
        wanted.backward = input().isKeyDown(GLFW_KEY_S);
        wanted.left = input().isKeyDown(GLFW_KEY_A);
        wanted.right = input().isKeyDown(GLFW_KEY_D);
        wanted.up = input().isKeyDown(GLFW_KEY_SPACE);
        // Left Shift has one meaning per mode: sprint when walking, down when flying.
        // The player uses the field that belongs to its mode and ignores the other.
        wanted.down = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
        wanted.sprint = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
    }

    // The step runs also with nothing held: it is what brings the feet back to the floor
    // after noclip was switched off in a panel.
    m_player.update(wanted, m_camera.yawDegrees, m_camera.pitchDegrees, static_cast<float>(fixedDt),
                    m_obstacles);

    // Walking never changes the height, with one exception: the step right after noclip
    // was switched off in mid-air, which puts the feet back on the floor. That is a jump
    // and not a movement, so it must not be blended: without this line one frame would
    // be drawn from a point part of the way down.
    if (!m_player.noclip) {
        m_previousPlayerPosition.y = m_player.position.y;
    }

    // The camera stands where the eyes of the player are. onRender does not draw from
    // this position directly (it blends two steps), but the debug UI shows it.
    m_camera.position = m_player.eyePosition();

    // The rules of the round, with the position the player has after this step: the
    // battery, the crystals within reach, the gate and the exit. The switch of the
    // flashlight goes in by reference, because an empty battery turns it off.
    const bool gateBlockedBefore = gateBlocks(m_mazeWorld, m_round);
    updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn,
                static_cast<float>(fixedDt));
    // The gate has just opened (the only change a step can make here): its box leaves
    // the obstacle list, and the way into the exit cell is free.
    if (gateBlocks(m_mazeWorld, m_round) != gateBlockedBefore) {
        m_obstacles = roundObstacles(m_mazeWorld, m_round);
    }
}
```

Tutaj jest kolejność części i podział pracy między aplikację, gracza i rundę. Sam ruch (kierunki, normalizacja, prędkości, `moveAndSlide`) opisuje [`../game/player.md`](../game/player.md), sekcja 5, a reguły rundy [`../game/gameplay.md`](../game/gameplay.md), sekcja 2.

| Część | Co robi aplikacja | Dlaczego tutaj |
|---|---|---|
| `m_previousPlayerPosition = m_player.position;` | zapamiętuje pozycję sprzed kroku, w każdym kroku, także bez ruchu | para (poprzednia, bieżąca) do interpolacji w `onRender` ([`main-loop.md`](main-loop.md), sekcja 5.5) |
| `PlayerInput wanted;` i siedem pól | tłumaczy klawisze na pola logiczne. `isKeyDown` to stan ciągły, bezpieczny w `onUpdate` ([`input.md`](input.md), sekcja 5.5). Bez przechwyconego kursora struktura zostaje taka, jak ją stworzono: wszystkie pola fałszywe | gracz nie wie nic o klawiaturze, więc test może "trzymać klawisz", ustawiając pole. Lewy Shift trafia do dwóch pól naraz (`down` i `sprint`), a gracz czyta to, które należy do jego trybu |
| `m_player.update(...)` | jeden krok gracza: kąty kamery, czas kroku jako `float` i lista pudełek `m_obstacles` (ściany, słupki i zamknięta brama). Krok wykonuje się **zawsze**, także z pustym wejściem | właśnie ten krok stawia stopy z powrotem na podłodze po wyłączeniu noclip, również wtedy, gdy noclip wyłączono w panelu przy wolnym kursorze |
| `if (!m_player.noclip) { ... }` | wyrównuje wysokość poprzedniej pozycji do bieżącej | chodzenie nie zmienia wysokości, poza jednym krokiem: pierwszym po wyłączeniu noclip w powietrzu. To skok, nie ruch, więc nie może być interpolowany |
| `m_camera.position = m_player.eyePosition();` | kamera staje w oczach gracza | `onRender` z tej pozycji nie rysuje (miesza dwa kroki), ale panel "Camera" ją pokazuje |
| `const bool gateBlockedBefore = gateBlocks(m_mazeWorld, m_round);` | zapamiętuje, czy brama blokowała drogę **przed** krokiem rundy | żeby po kroku dało się poznać zmianę bez zaglądania do środka `updateRound` |
| `updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn, static_cast<float>(fixedDt))` | jeden krok reguł: zegary, opadanie otwartej bramy, zużycie baterii, zbieranie kryształów w zasięgu, otwarcie bramy, wygrana | stoi **po** ruchu gracza, bo reguły mają widzieć pozycję po tym kroku (komentarz). Dostaje `fixedDt`, więc bateria ubywa o tyle samo na krok przy każdej liczbie klatek na sekundę ([`main-loop.md`](main-loop.md)) |
| `m_lighting.flashlightOn` jako argument | parametr to `bool&`: funkcja czyta przełącznik (bateria ubywa tylko przy włączonej latarce) i ustawia go na `false`, gdy bateria jest pusta | pustej baterii nie da się włączyć ani klawiszem F, ani polem wyboru w panelu "Lights": najbliższy krok znów ją wyłącza |
| `if (gateBlocks(...) != gateBlockedBefore) { m_obstacles = roundObstacles(...); }` | gdy wynik `gateBlocks` się zmienił, buduje listę przeszkód od nowa, już bez pudełka bramy | jedyna zmiana, jaką krok może tu zrobić, to otwarcie bramy (komentarz): pole `Round::gateOpen` w trakcie rundy nigdy nie wraca do `false`. Od następnego kroku gracz przechodzi przez miejsce bramy |

`fixedDt` jest typu `double` (tak liczy zegar), a gracz i runda liczą na `float`, stąd dwa razy `static_cast<float>(fixedDt)`. `m_obstacles` to `std::vector<scene::Aabb>`, a parametr `Player::update` to `std::span<const scene::Aabb>`: wektor zamienia się na widok bez kopiowania.

**Opóźnienie o jeden krok, którego nie widać.** Pudełko bramy znika z listy pod koniec kroku, w którym gracz zebrał ostatni potrzebny kryształ, więc ruch tego samego kroku liczył się jeszcze z zamkniętą bramą. To jeden stały krok (`Time::FIXED_DT`), a kryształy nigdy nie leżą w komórce wyjścia, więc gracz zwykle jest wtedy daleko od bramy.

**Runda idzie także przy wolnym kursorze.** `updateRound` nie stoi za warunkiem `isCursorCaptured()`. Po naciśnięciu Escape gracz przestaje się ruszać, ale zegar rundy płynie dalej, a bateria ubywa, jeśli latarka jest włączona. Gra nie ma pauzy.

### 6.6 `onRender`: jedna klatka

Funkcja ma trzy etapy: obsługa zdarzeń, które opisują jedną klatkę, przygotowanie stanu OpenGL i narysowanie sceny. Trzeci etap urósł w M4 (przed rysowaniem buduje i wysyła światła) i zmienił się w M5: światła przechodzą najpierw przez rundę (bateria, pulsowanie, pozycje nad kryształami), a rysowanie ma już tylko dwie części, scenę i linie kolizji.

**Etap 1: prośba o labirynt, prośba o nową rundę i klawisz R, klawisze N i F, mysz.**

```cpp
void NightMazeApp::onRender(double alpha) {
    // A new maze asked for by the debug UI is built here, at the start of a frame and
    // outside of the fixed steps, so no step ever sees a half replaced maze.
    if (m_mazeSettings.regenerate) {
        m_mazeSettings.regenerate = false;
        regenerateMaze();
    }

    // A new round on the same maze, asked for with the restart key or by the debug UI.
    // It is started here for the same reason: between two fixed steps, never inside one.
    // wasKeyPressed is true for one frame, so the key is read once per frame.
    if (m_gameplay.restart || input().wasKeyPressed(RESTART_KEY)) {
        m_gameplay.restart = false;
        beginRound();
    }

    // The noclip key. wasKeyPressed is true for one frame, so it is read here, once per
    // frame, and not in onUpdate, which runs zero or more times per frame.
    if (input().wasKeyPressed(NOCLIP_KEY)) {
        m_player.noclip = !m_player.noclip;
    }

    // The flashlight key, read once per frame for the same reason. Like the noclip key
    // it works whether or not the cursor is captured. With an empty battery the key
    // still sets the switch, but the next fixed step turns it off again
    // (game::updateRound), and no frame is drawn with the light of an empty battery
    // (game::lightingForFrame).
    if (input().wasKeyPressed(FLASHLIGHT_KEY)) {
        m_lighting.flashlightOn = !m_lighting.flashlightOn;
    }
```

| Linia | Znaczenie |
|---|---|
| `if (m_mazeSettings.regenerate)` | flagę ustawia panel "Maze" w trakcie `DebugUI::draw`, czyli **po** tym, jak `NightMazeApp::onRender` skończyło rysować tę klatkę. Gra odczytuje ją więc na początku **następnej** klatki. Kroki symulacji tej następnej klatki wykonały się już wcześniej (pętla woła `onUpdate` przed `onRender`, [`main-loop.md`](main-loop.md), sekcja 5.2), jeszcze na starym labiryncie i w całości. Żaden krok nie widzi labiryntu wymienionego w połowie, bo wymiana dzieje się poza krokami |
| `m_mazeSettings.regenerate = false;` | flaga jest jednorazowa: gasi ją ten, kto ją obsłużył |
| `if (m_gameplay.restart \|\| input().wasKeyPressed(RESTART_KEY))` | dwie drogi do tego samego: flaga `GameplaySettings::restart`, którą ustawia przycisk `Restart round (key R)` panelu "Gameplay", albo klawisz R (`RESTART_KEY` to `GLFW_KEY_R`). Flaga działa jak `regenerate`: panel ją zapisuje pod koniec klatki, gra czyta ją na początku następnej. Powód miejsca jest ten sam co dla labiryntu: runda zaczyna się między dwoma stałymi krokami, nigdy w środku kroku |
| `m_gameplay.restart = false;` | gaszenie flagi. Linia wykonuje się także wtedy, gdy powodem był klawisz, co nic nie psuje |
| `beginRound();` | nowa runda na **tym samym** labiryncie (sekcja 6.4). R działa w każdym stanie rundy, także po wygranej (karta `You escaped` podpowiada `R: play again`) i także w środku gry. Tak jak N i F nie wymaga przechwyconego kursora i nie działa, gdy klawiaturę ma ImGui |
| kolejność dwóch pierwszych bloków | gdy w jednej klatce przyjdą obie prośby, najpierw powstaje nowy labirynt (z własnym `beginRound`), a potem runda zaczyna się jeszcze raz na nim. Wynik jest ten sam co po samej wymianie labiryntu |
| `input().wasKeyPressed(NOCLIP_KEY)` | `NOCLIP_KEY` to `GLFW_KEY_N`. Zbocze jest prawdą przez jedną klatkę, więc wolno je czytać tylko raz na klatkę ([`input.md`](input.md), sekcja 5.5). Warunku `isCursorCaptured()` tu nie ma: N działa także przy wolnym kursorze. Nie działa tylko wtedy, gdy klawiaturę ma ImGui (blokada z `main.cpp`), na przykład podczas wpisywania ziarna |
| `m_player.noclip = !m_player.noclip;` | to samo pole przełącza pole wyboru w panelu "Collision" |
| `input().wasKeyPressed(FLASHLIGHT_KEY)` | `FLASHLIGHT_KEY` to `GLFW_KEY_F`, stała `constexpr int` w anonimowej przestrzeni nazw pliku, tuż pod `NOCLIP_KEY`. To też zbocze, więc stoi w `onRender` z tego samego powodu co N. Tak samo nie ma warunku `isCursorCaptured()`: F działa przy wolnym kursorze, a nie działa tylko wtedy, gdy klawiaturę ma ImGui (`core::Input` odpowiada wtedy `false`) |
| `m_lighting.flashlightOn = !m_lighting.flashlightOn;` | odwraca jedno pole `bool` w ustawieniach oświetlenia. To samo pole przełącza pole wyboru `Flashlight on (key F)` w panelu "Lights". Skutek widać jeszcze w tej samej klatce, bo światła są budowane niżej w tym samym `onRender` ([`../game/flashlight.md`](../game/flashlight.md)) |
| F przy pustej baterii | klawisz nie sprawdza baterii: ustawia przełącznik na `true`. Dwa miejsca pilnują, żeby nic z tego nie wynikło (komentarz): `lightingForFrame` rysuje klatkę z latarką wyłączoną, gdy bateria jest pusta, a najbliższy stały krok (`updateRound`) ustawia przełącznik z powrotem na `false`. Reguły baterii: [`../game/gameplay.md`](../game/gameplay.md), sekcja 2 |

Dalej idzie obrót myszą: kliknięcie w scenę przechwytuje kursor, a przy przechwyconym kursorze przesunięcie myszy razy `m_mouseSensitivity` trafia do `m_camera.rotate`. Ten fragment nie zmienił się od M1, stoi po obsłudze klawiszy i jest opisany linia po linii w [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 5.

**Etap 2: stan OpenGL i tło.** `glViewport` z rozmiaru framebuffera, `glEnable(GL_DEPTH_TEST)`, `glClearColor` i `glClear` dla koloru i głębi, wszystko w `GL_CHECK`. Potem strażnik rozmiaru:

```cpp
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return;
    }
```

Zminimalizowane okno może mieć framebuffer 0 x 0. Proporcje wyszłyby wtedy `0 / 0`, czyli `NaN`, a `glm::perspective` kończy program asercją w buildzie Debug. W takiej klatce i tak nie ma czego rysować. Opis tych wywołań: [`window-context.md`](window-context.md), sekcja 3.2, i [`../scene/camera.md`](../scene/camera.md), sekcja 5.7.

**Etap 3: oko, dwie macierze, światła i części sceny.**

```cpp
    const glm::vec3 feet =
        glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha));
    const glm::vec3 eye = feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F};

    // The two matrices that are the same for everything drawn in this frame.
    const glm::mat4 view = m_camera.viewMatrix(eye);
    const glm::mat4 projection = m_camera.projectionMatrix(aspectRatio);

    // The lights of this frame. They are built here, after the mouse has turned the
    // camera and from the same eye the view matrix uses: the flashlight then sits
    // exactly where the picture is taken from, and its cone stays in the middle of the
    // screen. From m_camera.position (the last fixed step) it would trail behind while
    // the player moves. The copy to the graphics card happens once, and both lit
    // programs read it.
    //
    // The round changes two things for this frame only: a low battery dims the
    // flashlight (an empty one switches it off) and the crystal lights pulse. That
    // happens in a copy, so the settings the debug UI shows stay as they were set. The
    // point lights hang above the crystals that are still there.
    const LightingSettings frameLighting = lightingForFrame(m_lighting, m_round, m_gameplay);
    const std::vector<glm::vec3> crystalLights = crystalLightPositions(m_round);
    const scene::LightSet lights =
        buildLightSet(frameLighting, eye, m_camera.forward(), crystalLights);
    m_lightRig.upload(lights, eye);

    drawMaze(view, projection);
    if (m_drawColliders) {
        drawColliderLines(view, projection);
    }

    // The sky comes LAST, after everything that writes depth. (...)
    if (m_skyboxSettings.enabled) {
        m_skybox.draw(m_skyboxShader, view, projection, m_skyboxSettings, m_viewMode);
    }
}
```

(Komentarz nad ostatnim blokiem jest tu skrócony. W całości, z omówieniem, jest w [`../renderer/skybox.md`](../renderer/skybox.md), sekcja 5.6.)

| Linia | Znaczenie |
|---|---|
| `glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha))` | pozycja stóp między dwoma krokami symulacji ([`main-loop.md`](main-loop.md), sekcje 2.4 i 5.5). `m_player.position` nie jest zmieniane: rysowanie tylko czyta stan symulacji |
| `feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F}` | oko jest stałe 1,7 m nad stopami, więc zmieszanie stóp i pójście w górę daje ten sam punkt co zmieszanie dwóch pozycji oczu |
| `m_camera.viewMatrix(eye)`, `m_camera.projectionMatrix(aspectRatio)` | obie macierze liczę **raz** i przekazuję do funkcji rysujących. Są takie same dla wszystkiego, co jest rysowane w tej klatce, ale każdy program ma własne uniformy `uView` i `uProjection`, które trzeba ustawić osobno |
| `lightingForFrame(m_lighting, m_round, m_gameplay)` | zwraca **kopię** ustawień oświetlenia na tę jedną klatkę. W kopii latarka jest wyłączona, gdy bateria jest pusta, jej moc jest pomnożona przez migotanie słabej baterii (`flashlightFlicker`), a moc świateł punktowych przez pulsowanie kryształów (`crystalPulse`). `m_lighting` zostaje nietknięte, więc suwaki panelu "Lights" pokazują to, co ustawił użytkownik, a nie wartość drgającą z klatki na klatkę |
| `crystalLightPositions(m_round)` | pozycje świateł punktowych tej chwili: po jednym nad każdym kryształem, który nie jest jeszcze zebrany, razem z jego kołysaniem. Zebrany kryształ traci światło, bo nie trafia na tę listę. To `std::vector<glm::vec3>` budowany w każdej klatce: kryształów jest najwyżej 16 (`scene::MAX_POINT_LIGHTS`) |
| `buildLightSet(frameLighting, eye, m_camera.forward(), crystalLights)` | składa z ustawień `scene::LightSet`, czyli światła tej klatki jako zwykłe dane: księżyc, latarkę w punkcie `eye` skierowaną wzdłuż `m_camera.forward()` i światła punktowe w podanych pozycjach. Dostaje kopię z linii wyżej, nie `m_lighting`. Funkcja nie wie, skąd są pozycje: w M4 były to ślepe zaułki policzone przy budowie labiryntu, dziś kryształy rundy. Jest w bibliotece `game_logic`, bez OpenGL, i ma testy ([`../game/flashlight.md`](../game/flashlight.md)) |
| czas w tych trzech liniach | wszystko, co się samo rusza, czyta `m_round.animationSeconds`, a ten zegar przesuwa `updateRound` w stałych krokach. `onRender` niczego tu nie odlicza i nie używa `alpha`: animacja kryształów i migotanie zmieniają się więc co krok symulacji, nie co klatkę |
| dlaczego tutaj | komentarz mówi wprost: po obrocie myszą (etap 1) i z tego samego oka co macierz widoku. Latarka stoi wtedy dokładnie w punkcie, z którego robiony jest obraz, i jej stożek zostaje w środku ekranu. Z `m_camera.position`, czyli z oczu po ostatnim stałym kroku, zostawałaby w ruchu za obrazem. To wynika z kodu: zachowania stożka w ruchu nikt jeszcze nie sprawdził ręcznie |
| `m_lightRig.upload(lights, eye)` | pakuje światła i pozycję oka do struktury o układzie bajtów zgodnym z blokiem `LightBlock` w shaderze i kopiuje ją do bufora uniformów. Jedno kopiowanie na klatkę, a czytają je oba programy z oświetleniem ([`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md)) |
| brak warunku przy liniach wyżej | światła są budowane i wysyłane w **każdej** klatce, także w trybie `Unlit`, w którym żaden program ich nie czyta. Kod jest dzięki temu prosty, a koszt to jedno małe kopiowanie |
| `drawMaze(view, projection)` | cała scena: labirynt, brama i kryształy, z oświetleniem albo bez (sekcja 6.7) |
| `drawColliderLines` | kolejność: scena, potem linie. O tym, co zasłania co, decyduje test głębi, a nie kolejność rysowania. Linie są nakładką do szukania błędów, więc idą po scenie |
| `if (m_skyboxSettings.enabled) { m_skybox.draw(...); }` | od pierwszej części M6 ostatnie wywołanie rysujące sceny: niebo. Rysowane na największej głębi z testem `GL_LEQUAL`, więc wypełnia tylko piksele, na których nic nie narysowano. Tu kolejność **ma** znaczenie, ale dla kosztu, a nie dla obrazu: narysowane na końcu, niebo nie jest cieniowane tam, gdzie zasłaniają je ściany ([`../renderer/skybox.md`](../renderer/skybox.md), sekcja 2.7) |
| `if (m_drawColliders)` | pole przełącza pole wyboru `Draw collision shapes` w panelu "Collision". Domyślnie `false` |
| czego już nie ma | do M4 między tymi dwoma wywołaniami stały `drawLightMarkers` (małe sześciany w miejscach świateł punktowych) i `drawCube` (kostka z M1). Widocznym źródłem światła punktowego jest teraz sam kryształ, który świeci własnym kolorem przez uniform `uEmissive` ([`../game/gameplay.md`](../game/gameplay.md), sekcja 4) |

### 6.7 Części sceny

Funkcji rysujących są trzy, a czwarta, `drawMaze`, tylko wybiera jedną z dwóch. W jednej klatce wykonują się najwyżej dwie: jedna z pary `drawUnlitMaze` i `drawLitMaze` oraz, na życzenie, `drawColliderLines`. Deklaracje w nagłówku:

```cpp
    /// The parts of a frame. Each one selects its own shader program and sets its
    /// uniforms. drawMaze draws the maze together with the crystals and the gate, and
    /// has two ways to do it: without lighting (the textured program, also used for the
    /// debug views of the normals and the texture coordinates) and with lighting.
    void drawMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const;
```

Wszystkie są `const`: rysowanie nie zmienia stanu gry. Każda z trzech rysujących ma tę samą budowę: sprawdza, czy jej program jest poprawny, wybiera go (`use()`), ustawia `uView` i `uProjection`, a potem rysuje. Kolejność `use()` przed setterami jest obowiązkowa, bo uniform należy do programu, który jest właśnie używany ([`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 7). Nazwy uniformów (`VIEW_UNIFORM`, `PROJECTION_UNIFORM` i pozostałe) są w jednym miejscu, w [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp) ([`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 5).

**Wybór: `drawMaze`.**

```cpp
void NightMazeApp::drawMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // The two debug views (normals and texture coordinates as colours) only exist in the
    // textured program, and they show data, not light. So they are drawn without
    // lighting whatever the lighting mode is. The view of the normals still follows the
    // lighting in one thing: it shows the normals the chosen mode shades with.
    if (m_lighting.mode == LightingMode::Unlit || m_viewMode != ViewMode::Textured) {
        drawUnlitMaze(view, projection);
    } else {
        drawLitMaze(view, projection);
    }
}
```

| `m_lighting.mode` | `m_viewMode` | Która funkcja | Program |
|---|---|---|---|
| `Unlit` | dowolny | `drawUnlitMaze` | `textured` |
| `Gouraud`, `Phong` albo `BlinnPhong` | `Normals` albo `Uvs` (widok do szukania błędów) | `drawUnlitMaze` | `textured` |
| `Gouraud` | `Textured` | `drawLitMaze` | `gouraud` |
| `Phong` albo `BlinnPhong` | `Textured` | `drawLitMaze` | `lit` |

Operator `||` czyta się "albo jedno, albo drugie": wystarczy jeden z dwóch warunków, żeby scena poszła bez oświetlenia. Nazwa `drawMaze` została z czasów, gdy w scenie był sam labirynt: dziś obie gałęzie rysują też bramę i kryształy. Widoki normalnych i współrzędnych tekstury istnieją tylko w programie `textured` (uniform `uViewMode`) i pokazują dane, a nie światło, więc mają pierwszeństwo przed trybem oświetlenia. Ten podział i same shadery opisuje [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md).

**Bez oświetlenia: `drawUnlitMaze`.** To dawne ciało `drawMaze` z M2 + M3, przeniesione do własnej funkcji. Razem z mapami normalnych doszła w nim jedna linia, ustawienie `uNormalMapEnabled`, a w M5 ostatnie wywołanie, `m_gameplayRenderer.draw`:

```cpp
void NightMazeApp::drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, and the rest of the frame is still drawn.
    if (!m_texturedShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_texturedShader.use();
    m_texturedShader.setMat4(VIEW_UNIFORM, view);
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    // The enum values are the numbers textured.frag compares uViewMode with.
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    // Only the view of the normals reads it: that view shows the normals the lighting
    // would use, so with normal mapping the ones from the normal maps.
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);

    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld);
    // The crystals and the gate, with the same program: they show up in the debug
    // views like the walls do.
    m_gameplayRenderer.draw(m_texturedShader, m_mazeWorld, m_round,
                            crystalGlow(m_lighting.pointColor, m_round.animationSeconds));
}
```

`drawUnlitMaze` ustawia to, co jest wspólne dla całej sceny: dwie macierze, tryb widoku (0 obraz, 1 normalne jako kolor, 2 współrzędne tekstury jako kolor) i przełącznik mapowania normalnych. Ten ostatni to `usesNormalMap(m_lighting) ? 1 : 0`: uniform typu `bool` ustawia się liczbą całkowitą, a w programie `textured` czyta go tylko widok normalnych, który dzięki temu pokazuje normalne, jakimi cieniowałby wybrany tryb oświetlenia (z map normalnych albo z siatki, [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 4.3). Resztę, czyli `uTexture` i `uNormalMap`, obie tekstury i `uTint` dla każdej części oraz `uModel` dla każdego obiektu, ustawia `MazeRenderer::draw` ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5), a dla bramy i kryształów `GameplayRenderer::draw`. Oba wywołania dostają ten sam program, już wybrany i z ustawionymi macierzami, więc brama i kryształy pokazują się w widokach normalnych i współrzędnych tekstury tak samo jak ściany. Shader `textured.frag` jest opisany w [`../gfx/textures.md`](../gfx/textures.md), sekcja 4.

**Z oświetleniem: `drawLitMaze`.**

```cpp
void NightMazeApp::drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Gouraud has a program of its own (the light is computed in its vertex shader).
    // Phong and Blinn-Phong share the other one and differ in one uniform.
    const gfx::Shader& shader =
        m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;
    if (!shader.isValid()) {
        return;
    }

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    // The material of the stone. The lights themselves are not set here: they are in
    // the uniform buffer that onRender filled before this call.
    // The enum values are the numbers common/lighting.glsl compares uSpecularModel with.
    shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    shader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    // Normal mapping, the switch of the lit program (1 on, 0 off). The Gouraud program
    // has no such uniform, and usesNormalMap is false for it anyway.
    shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);

    m_mazeRenderer.draw(shader, m_mazeWorld);
    // The crystals and the gate, with the same program and so the same lighting mode.
    // The crystals glow in the colour of their lights.
    m_gameplayRenderer.draw(shader, m_mazeWorld, m_round,
                            crystalGlow(m_lighting.pointColor, m_round.animationSeconds));
}
```

| Linia | Znaczenie |
|---|---|
| `const gfx::Shader& shader = m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;` | operator warunkowy wybiera jeden z dwóch programów i daje do niego referencję, bez kopiowania (shadera nie da się skopiować). Dalszy kod jest dzięki temu jeden dla obu programów |
| `if (!shader.isValid()) { return; }` | ten sam strażnik co wszędzie: bez programu ta część klatki nie jest rysowana, reszta tak |
| `shader.setMat4(VIEW_UNIFORM, view)`, `shader.setMat4(PROJECTION_UNIFORM, projection)` | te same dwie macierze co w pozostałych funkcjach |
| `shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)))` | który wzór na połysk: 0 dla Phonga, 1 dla Blinna-Phonga. `specularModelOf` tłumaczy tryb oświetlenia na `game::SpecularModel` (dla trybu `Gouraud` zwraca wzór Phonga) |
| `shader.setFloat(SPECULAR_STRENGTH_UNIFORM, ...)`, `shader.setFloat(SHININESS_UNIFORM, ...)` | dwie liczby materiału z ustawień, edytowane w grupie `Highlight (specular)` panelu "Lights". Komentarz w kodzie mówi o materiale kamienia, ale te same dwie liczby obowiązują też dla bramy i kryształów, bo są rysowane tym samym programem zaraz potem |
| `shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0)` | przełącznik mapowania normalnych programu `lit`: 1, gdy pole `Normal mapping` z panelu "Assets" jest zaznaczone i tryb jest inny niż `Gouraud` (`game::usesNormalMap`, [`../game/flashlight.md`](../game/flashlight.md), sekcja 5.2). Program `gouraud` tego uniformu nie ma: ustawienie jest po cichu ignorowane |
| czego tu nie ma | żadnego światła. Światła są w buforze uniformów, który `onRender` wypełnił przed tym wywołaniem. Ta funkcja ustawia tylko zwykłe uniformy programu |
| `m_mazeRenderer.draw(shader, m_mazeWorld)` | ta sama klasa i ta sama pętla po obiektach co bez oświetlenia. `MazeRenderer` dostaje program w argumencie i nie wie, który to |
| `m_gameplayRenderer.draw(shader, m_mazeWorld, m_round, crystalGlow(...))` | brama (dopóki choć trochę wystaje nad podłogę) i każdy niezebrany kryształ, tym samym programem, więc w tym samym trybie oświetlenia co ściany. Pozycje bierze z `m_mazeWorld` i `m_round`, niczego nie posiada |
| `crystalGlow(m_lighting.pointColor, m_round.animationSeconds)` | kolor, którym kryształ świeci sam z siebie: kolor świateł punktowych razy pulsowanie z zegara animacji. Trafia do uniformu `uEmissive`. Bierze kolor z `m_lighting`, a nie z kopii `frameLighting` z `onRender`, bo pulsowanie dokłada sama funkcja `crystalGlow` |

Wzory, shadery `lit.*` i `gouraud.*` oraz różnicę między liczeniem światła dla wierzchołka i dla fragmentu opisuje [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), rodzaje świateł i plik `common/lighting.glsl` [`../scene/lights.md`](../scene/lights.md), mapy normalnych i plik `common/normal_map.glsl` [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), a `GameplayRenderer`, animację kryształów, opadanie bramy i `uEmissive` [`../game/gameplay.md`](../game/gameplay.md), sekcje 4 i 5.

**Światła nie widać: widać tylko to, co oświetla.** W M4 w miejscu każdego światła punktowego stał mały sześcian w płaskim kolorze, rysowany osobną funkcją programem `color`. W M5 tej funkcji nie ma: źródłem światła, które widać, jest kryształ, a ponieważ ma świecić także tam, gdzie nie pada na niego żadne światło, dostaje własny składnik koloru (`uEmissive`). Kryształ jest więc częścią zwykłej, oświetlonej sceny, a nie osobną nakładką.

**Linie kolizji: `drawColliderLines`.**

```cpp
void NightMazeApp::drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The depth test stays on: a line behind a wall is hidden by it, which shows where
    // each box really is. The box of the player is drawn at the simulation position (the
    // last fixed step), the camera at a blend of two steps, so while moving the box runs
    // ahead of the camera by a fraction of one step.
    m_colliderLines.draw(m_colorShader, m_mazeWorld.colliders, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);

    // The gate, while it is an obstacle, and the zone behind it that wins the round.
    if (gateBlocks(m_mazeWorld, m_round)) {
        m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.gateBox, 1),
                             GATE_COLLIDER_COLOR);
    }
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.exitZone, 1),
                         EXIT_ZONE_COLOR);

    // The spheres of the pickup test: the reach of the player and, around every crystal
    // that is still there, the sphere the reach has to overlap. They stay on the
    // resting place of the crystal while the crystal itself bobs.
    const scene::Sphere reach = playerReach(m_player.position);
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&reach, 1),
                                PLAYER_COLLIDER_COLOR);
    std::vector<scene::Sphere> pickupSpheres;
    for (const RoundCrystal& crystal : m_round.crystals) {
        if (!crystal.collected) {
            pickupSpheres.push_back(
                {.center = crystalCenter(crystal.restPosition), .radius = m_gameplay.pickupRadius});
        }
    }
    m_colliderLines.drawSpheres(m_colorShader, pickupSpheres, PICKUP_COLLIDER_COLOR);
}
```

Pięć kolorów to stałe `constexpr glm::vec3` na górze pliku `.cpp`. Te same kolory wymienia legenda w panelu "Collision" (`Yellow: walls, pillars. Green: player. Orange: gate. Cyan: crystal pickup. Magenta: exit zone.`).

| Linia | Znaczenie |
|---|---|
| `m_colliderLines.draw(m_colorShader, m_mazeWorld.colliders, MAZE_COLLIDER_COLOR)` | wszystkie stałe pudełka labiryntu (ściany i słupki) na żółto (`{1.0F, 0.85F, 0.1F}`). Tu idzie `m_mazeWorld.colliders`, a nie `m_obstacles`: brama jest rysowana osobno i w innym kolorze |
| `const scene::Aabb playerBox = m_player.box();` | pudełko gracza z jego pozycji symulacyjnej (ostatni stały krok). Kamera jest rysowana z punktu między dwoma krokami, więc w ruchu pudełko wyprzedza kamerę o ułamek kroku |
| `std::span<const scene::Aabb>(&playerBox, 1)` | `draw` przyjmuje listę pudełek. Widok zbudowany ze wskaźnika i liczby 1 to lista z jednym elementem, bez tworzenia wektora |
| `PLAYER_COLLIDER_COLOR` | zielony (`{0.2F, 1.0F, 0.4F}`), dla pudełka gracza i dla kuli jego zasięgu |
| `if (gateBlocks(m_mazeWorld, m_round)) { ... GATE_COLLIDER_COLOR ... }` | pudełko bramy na pomarańczowo (`{1.0F, 0.45F, 0.1F}`), tylko dopóki brama jest przeszkodą. Ten sam warunek decyduje o tym, czy pudełko jest na liście `m_obstacles`, więc linie znikają dokładnie wtedy, gdy da się przejść, choć model bramy opada jeszcze przez `GATE_OPEN_SECONDS` |
| `m_colliderLines.draw(..., &m_mazeWorld.exitZone, 1), EXIT_ZONE_COLOR)` | strefa wyjścia na purpurowo (`{1.0F, 0.3F, 0.9F}`), zawsze. To nie przeszkoda, tylko pudełko, którego dotknięcie przy otwartej bramie wygrywa rundę |
| `const scene::Sphere reach = playerReach(m_player.position);` | kula, którą gracz "sięga" po kryształy i po wyjście: środek 0,9 m nad stopami, promień 0,3 m. Liczona tą samą funkcją, której używa `updateRound`, i z tej samej pozycji symulacyjnej |
| `m_colliderLines.drawSpheres(...)` | kula jako trzy okręgi (jeden leżący, dwa stojące), z jednej wspólnej siatki okręgu |
| pętla po `m_round.crystals` | dla każdego **niezebranego** kryształu kula zbierania: środek w `crystalCenter(crystal.restPosition)`, promień `m_gameplay.pickupRadius` (suwak `Pickup radius`), kolor cyjan (`{0.2F, 0.9F, 1.0F}`). Kula stoi w miejscu spoczynku kryształu i nie kołysze się razem z modelem. Wektor powstaje w każdej klatce, w której linie są włączone: to nakładka do szukania błędów, a kryształów jest najwyżej 16 |

Test głębi zostaje włączony, więc linia za ścianą jest przez nią zasłonięta. Jak `ColliderLines` rysuje jedno pudełko i jedną kulę oraz co robi shader `color`, opisuje [`../scene/collision.md`](../scene/collision.md), sekcje 4 i 5.

**Co jest sprawdzone.** Sprzed M4 (Windows, 2026-10-05, MSVC 19.44, build Debug): na zrzutach ekranu sprawdzone zostały widok startowy z teksturami, widok z góry w trybie noclip z żółtymi pudełkami na ścianach i słupkach oraz oba tryby widoku do szukania błędów. Tamte zrzuty pokazują scenę bez oświetlenia.

M4, po dodaniu oświetlenia (Windows, 2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74): build Debug i Release przechodził bez ostrzeżeń, testy przechodziły w obu, clang-format i clang-tidy niczego nie zgłaszały, a gra startowała bez linii `[error]` i bez linii `GL_`. Na zrzutach ekranu sprawdzone zostały: widok startowy, cztery tryby oświetlenia z trzech punktów widzenia, scena z wyłączoną latarką, światło w ślepym zaułku (tam wtedy stały światła punktowe), strony ścian oświetlone i nieoświetlone przez księżyc oraz błąd wewnątrz `common/lighting.glsl` pokazany z nazwą pliku, podczas gdy poprzedni program rysował dalej. Po dodaniu map normalnych (ten sam dzień i sprzęt) na zrzutach ekranu fugi czytają się jako rowki na ścianach, słupku i podłodze ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 5.11).

M5 (Windows, 2026-10-05): kod jest kompletny, ale kamień milowy nie jest zamknięty. Build Debug i Release przechodzi bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu. Obraz został sprawdzony na zrzutach ekranu robionych przez tymczasowe haki w kodzie, które potem usunięto.

Nikt jeszcze nie sprawdził ręcznie: chodzenia prawdziwymi klawiszami, klawisza N, klawisza F (także przy pustej baterii), klawisza R i przycisku `Restart round (key R)`, suwaków panelu "Gameplay", przejścia przez otwartą bramę, zbierania kryształów, karty wygranej, migotania latarki na ekranie, HUD przy ukrytych panelach, obrotu myszą w labiryncie, tego, czy stożek latarki trzyma się środka ekranu w ruchu, listy `Lighting`, widżetów panelu "Lights", pola wyboru `Normal mapping` w panelu "Assets" i przycisków wymiany labiryntu. To otwarte pozycje listy kontrolnej w [`../../guides/build-windows.md`](../../guides/build-windows.md). Na macOS kod M4 i M5 nie był ani budowany, ani uruchamiany.

### 6.8 Trzy poziomy dziedziczenia

W programie są trzy klasy, każda w innej warstwie:

```mermaid
classDiagram
    class Application {
        <<core>>
        -Window m_window
        -Input m_input
        -Time m_time
        +run()
        #onUpdate(fixedDt)*
        #onRender(alpha)*
    }
    class NightMazeApp {
        <<game>>
        -m_clearColor
        -Shader m_texturedShader
        -Shader m_colorShader
        -Shader m_litShader
        -Shader m_gouraudShader
        -Shader m_skyboxShader
        -AssetCache m_assets
        -MazeRenderer m_mazeRenderer
        -GameplayRenderer m_gameplayRenderer
        -ColliderLines m_colliderLines
        -LightRig m_lightRig
        -Skybox m_skybox
        -MazeSettings m_mazeSettings
        -MazeWorld m_mazeWorld
        -GameplaySettings m_gameplay
        -Round m_round
        -vector~Aabb~ m_obstacles
        -Player m_player
        -vec3 m_previousPlayerPosition
        -Camera m_camera
        -LightingSettings m_lighting
        -ViewMode m_viewMode
        -bool m_drawColliders
        -SkyboxSettings m_skyboxSettings
        -float m_mouseSensitivity
        -regenerateMaze()
        -beginRound()
        -drawMaze(view, projection)
        -drawUnlitMaze(view, projection)
        -drawLitMaze(view, projection)
        -drawColliderLines(view, projection)
        #onUpdate(fixedDt)
        #onRender(alpha)
        #szesnaście akcesorów()
    }
    class DebugNightMazeApp {
        <<main>>
        -DebugUI m_debugUI
        #onRender(alpha)
    }
    Application <|-- NightMazeApp
    NightMazeApp <|-- DebugNightMazeApp
```

W nawiasach ostrych jest warstwa: `core::Application`, `game::NightMazeApp` i `DebugNightMazeApp` z `main.cpp`. Gwiazdka oznacza funkcję czysto wirtualną.

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

**Dlaczego nakładka debug jest w `main.cpp`, a nie w grze.** Zgodnie z regułą warstw `debug/` może zależeć od wszystkiego, ale nic nie może zależeć od `debug/`. Ktoś jednak musi na końcu każdej klatki zawołać `DebugUI::draw`. Gdyby robiło to `game::NightMazeApp`, gra dołączałaby `debug/DebugUI.hpp` i reguła byłaby złamana. Rozwiązaniem jest miejsce stojące **ponad** obiema warstwami: `main.cpp`. To jedyny plik, który tworzy obiekty obu warstw, i tam skleja je klasa `DebugNightMazeApp`. Zysk jest praktyczny: wersję gry bez paneli dostaję, pisząc w `main` po prostu `game::NightMazeApp app;`, bez dotykania kodu gry.

Szczegóły tej klasy, o które można zostać zapytanym:

- `game::NightMazeApp::onRender(alpha);` to wywołanie **z kwalifikacją nazwą klasy**. Wyłącza ono mechanizm wirtualny i woła dokładnie wersję bazową (rysowanie gry). Samo `onRender(alpha)` wywołałoby wirtualnie tę samą funkcję, czyli nieskończoną rekurencję.
- Kolejność w `onRender`: najpierw gra obsługuje prośbę o nowy labirynt, prośbę o nową rundę, klawisze R, N i F oraz mysz kamery, buduje światła i rysuje scenę, potem `main.cpp` mówi ImGui, czy wolno mu używać myszy (`setMouseEnabled`: nie, gdy kursor jest przechwycony przez kamerę), panele i HUD lądują na wierzchu, a na końcu `main.cpp` przekazuje do `core::Input` informację, czy ImGui używa klawiatury i czy używa myszy.
- Przełącznik paneli to klawisz na lewo od `1` (na klawiaturze US z napisami `` ` `` i `~`, w GLFW `GLFW_KEY_GRAVE_ACCENT`). Sprawdzam go przez `wasKeyPressed` w `onRender`, czyli dokładnie raz na klatkę (dlaczego nie w `onUpdate`: [`input.md`](input.md), sekcja 5.5).
- Ten klawisz chowa **panele**, nie HUD. `DebugUI::draw` rysuje panele tylko przy `m_visible`, a `drawHud(context.round, context.gameplay)` woła zawsze, zaraz po nich: HUD należy do gry, nie do narzędzi (komentarz w `DebugUI.cpp`). Gra sama HUD narysować nie może, bo `game/` nie dołącza ImGui, więc funkcja `debug::drawHud` leży w `src/debug/`, a dane dostaje przez te same dwa pola kontekstu co panele. Skutek uboczny tego podziału: `game::NightMazeApp` uruchomione bez `DebugNightMazeApp` nie miałoby ani paneli, ani HUD. Co HUD pokazuje: [`../game/gameplay.md`](../game/gameplay.md), sekcja 6.
- Linia `setMouseEnabled` i dwie ostatnie linie `onRender` to całe powiązanie klawiatury i myszy gry z ImGui. `core/` nie wie, kto i dlaczego blokuje wejście, a `debug/` nie wie, co gra zrobi z tą informacją ani dlaczego ma zignorować mysz. Pełny opis: [`input.md`](input.md), sekcje 5.6, 5.10 i 5.11.
- `final` zabrania dalszego dziedziczenia po tej klasie. Anonimowa przestrzeń nazw sprawia, że klasa jest widoczna tylko w `main.cpp`.
- Klasa nie ma własnego konstruktora. Kompilator generuje domyślny: buduje część bazową (`NightMazeApp`), a potem pole `m_debugUI` z inicjalizatora przy deklaracji, `{window()}`.
- `clearColor()` to chroniony akcesor w `NightMazeApp` zwracający referencję do prywatnego `m_clearColor`. Tak samo działa pozostałych piętnaście (tabela w sekcji 6.2). Gra udostępnia swój stan klasie pochodnej, nie wiedząc, kto i po co go użyje.
- Dwadzieścia pól `DebugContext` to dwa akcesory klasy bazowej (`time()`, `window()`) i osiemnaście akcesorów gry (dwa ostatnie, `skyboxShader()` i `skyboxSettings()`, doszły w pierwszej części M6 i stoją na końcu listy). Inicjalizatory desygnowane muszą stać w kolejności deklaracji pól w strukturze, dlatego `.texturedShader` jest dopiero po `.mouseSensitivity`, trzy linie z M4 (`.litShader`, `.gouraudShader`, `.lighting`) stoją po `.drawColliders`, choć w nagłówku gry ich akcesory są zaraz po `colorShader()`, a dwie linie z M5 (`.gameplay`, `.round`) na samym końcu. Linia `.shader = shader()` zniknęła razem z programem kostki ([`../debug-ui.md`](../debug-ui.md), sekcja 5).
- `texturedShader()`, `colorShader()`, `litShader()`, `gouraudShader()` i `skyboxShader()` zwracają `gfx::Shader&`, przez które panel "Shaders" woła `reload()` na wszystkich pięciu programach (`SHADER_COUNT = 5` w `DebugUI.cpp`) ([`../gfx/shader-hot-reload.md`](../gfx/shader-hot-reload.md), sekcja 6).
- `lighting()` zwraca `LightingSettings&`. `DebugUI::draw` daje całą strukturę panelowi "Lights", panelowi "Renderer" tylko jej pole `mode`, a panelowi "Assets" pole `normalMapping` ([`../debug-ui.md`](../debug-ui.md), sekcja 5.2).

Dwie rzeczy warte uwagi w samej klasie bazowej:

- Destruktor bazowy jest wirtualny (`virtual ~Application() = default;`). Każda klasa z funkcjami wirtualnymi, po której się dziedziczy, powinna go mieć, bo inaczej usunięcie obiektu pochodnego przez wskaźnik na bazę nie wywołałoby destruktora pochodnej.
- Kopiowanie jest zablokowane (`= delete`) w `Application` i w `Window`. Kopia `Window` miałaby ten sam `m_handle` i oba destruktory próbowałyby zniszczyć to samo okno (podwójne zwolnienie).

Dzięki temu podziałowi warstwa `engine` nadaje się też do zadań laboratoryjnych: wystarczy nowa klasa pochodna z własnym `onUpdate` i `onRender`.

## 7. Kolejność pól, od której zależy poprawność

W C++ pola klasy są **konstruowane w kolejności deklaracji** w klasie (nie w kolejności na liście inicjalizacyjnej konstruktora) i **niszczone w kolejności odwrotnej**. W tym projekcie dwa miejsca polegają na tej regule.

**`Application`:**

```cpp
// Order matters: members are constructed top to bottom, and Input needs the window.
Window m_window;
Input m_input;
Time m_time;
```

```cpp
Application::Application(int width, int height, const std::string& title)
    : m_window(width, height, title), m_input(m_window.nativeHandle()) {}
```

`m_input` potrzebuje uchwytu okna, więc `m_window` musi powstać pierwsze. Gdybym zamienił deklaracje miejscami, `m_input` zostałoby skonstruowane **przed** `m_window`, a `m_window.nativeHandle()` odczytałoby pole obiektu, który jeszcze nie istnieje (niezdefiniowane zachowanie, w praktyce śmieciowy wskaźnik). Kompilator w najlepszym razie ostrzeże o niezgodnej kolejności na liście inicjalizacyjnej. Przy niszczeniu kolejność jest odwrotna: `m_time`, `m_input`, a `m_window` na końcu, więc okno i GLFW znikają jako ostatnie.

**`DebugNightMazeApp` (w `main.cpp`):**

```cpp
// Members are destroyed before base classes, so ImGui shuts down while the window
// and its OpenGL context (owned by core::Application) still exist.
debug::DebugUI m_debugUI{window()};
```

Tu działa druga część tej samej reguły: najpierw konstruowana jest **część bazowa** obiektu (od najgłębszej klasy bazowej), potem pola klasy pochodnej, a niszczenie idzie dokładnie odwrotnie: **pola są niszczone przed klasami bazowymi**.

| Konstrukcja (z góry na dół) | Niszczenie (z góry na dół) |
|---|---|
| `Application::m_window` (GLFW, okno, kontekst, GLAD) | `DebugNightMazeApp::m_debugUI` (zamknięcie ImGui, okno i kontekst jeszcze żyją) |
| `Application::m_input` | `NightMazeApp::m_mouseSensitivity`, `m_skyboxSettings`, `m_drawColliders`, `m_viewMode`, `m_lighting`, `m_camera`, `m_previousPlayerPosition`, `m_player` (zwykłe dane, nic do zwolnienia) |
| `Application::m_time` | `NightMazeApp::m_obstacles`, `m_round`, `m_gameplay`, `m_mazeWorld`, `m_mazeSettings` (wektory i liczby, bez OpenGL) |
| `NightMazeApp::m_clearColor` | `NightMazeApp::m_skybox` (tekstura sześcienna nieba z samplerem i siatka sześcianu), potem `m_lightRig` (bufor uniformów: `glDeleteBuffers`) |
| `NightMazeApp::m_texturedShader`, `m_colorShader`, `m_litShader`, `m_gouraudShader`, `m_skyboxShader` (kompilacja i linkowanie, potrzebują kontekstu) | `NightMazeApp::m_colliderLines` (dwie siatki linii, każda to VAO i dwa bufory) |
| `NightMazeApp::m_assets` (biała tekstura 1 x 1 i jej sampler) | `NightMazeApp::m_gameplayRenderer` (nic nie posiada: trzy wskaźniki do modeli z pamięci assetów) |
| `NightMazeApp::m_mazeRenderer` (wczytuje przez `m_assets` trzy modele labiryntu i ich tekstury: siatki i tekstury powstają na karcie) | `NightMazeApp::m_mazeRenderer` (nic nie posiada: trzy wskaźniki do modeli z pamięci assetów) |
| `NightMazeApp::m_gameplayRenderer` (wczytuje przez `m_assets` dwa modele kryształów i model bramy z ich teksturami) | `NightMazeApp::m_assets` (siatki modeli, potem tekstury, na końcu biała tekstura) |
| `NightMazeApp::m_colliderLines` (siatka krawędzi sześcianu: 8 wierzchołków i 24 indeksy, siatka okręgu: 32 wierzchołki i 64 indeksy) | `NightMazeApp::m_skyboxShader`, `m_gouraudShader`, `m_litShader`, `m_colorShader`, `m_texturedShader` (`glDeleteProgram`) |
| `NightMazeApp::m_lightRig` (bufor uniformów na światła: `glGenBuffers`, `glBufferData`, `glBindBufferBase`), potem `m_skybox` (sześć obrazów nieba z dysku, tekstura sześcienna, siatka sześcianu) | `NightMazeApp::m_clearColor` |
| `NightMazeApp::m_mazeSettings`, `m_mazeWorld` (generowanie labiryntu, wyjścia i kryształów, bez OpenGL), `m_gameplay`, `m_round` i `m_obstacles` (oba jeszcze puste), `m_player`, `m_previousPlayerPosition` (kopia pozycji gracza, dlatego po nim), `m_camera`, `m_lighting`, `m_viewMode`, `m_drawColliders`, `m_skyboxSettings`, `m_mouseSensitivity` | `Application::m_time` |
| ciało konstruktora `NightMazeApp` (dwa razy `m_lightRig.connect(...)`, potem `beginRound()`, które wypełnia `m_round` i `m_obstacles`) | `Application::m_input` |
| `DebugNightMazeApp::m_debugUI` (potrzebuje okna i kontekstu) | `Application::m_window` (okno, kontekst, `glfwTerminate`) |

Obie kolumny czyta się osobno, z góry na dół: wiersz nie łączy pola z lewej z polem z prawej.

Dlatego `m_debugUI{window()}` jest bezpieczne (cała część bazowa, a więc i okno, już istnieje), a destruktor `DebugUI`, który zwalnia obiekty OpenGL backendu ImGui, ma jeszcze żywy kontekst. Ta sama zasada dotyczy pól `NightMazeApp` posiadających zasoby OpenGL: pięciu programów shaderów, `m_assets` (siatki modeli i tekstury), `m_colliderLines` (dwie siatki linii), `m_lightRig` (bufor uniformów) i `m_skybox` (tekstura sześcienna i siatka sześcianu). Jako pola klasy pochodnej od `Application` powstają po oknie i są niszczone przed nim, więc każde `glGen*`, `glCreate*` i `glDelete*` ma żywy kontekst. `m_mazeRenderer` i `m_gameplayRenderer` zasobów nie posiadają: trzymają wskaźniki do modeli, których właścicielem jest `m_assets`, i dlatego muszą być zadeklarowane po nim (giną wcześniej, więc nigdy nie wskazują na usunięty model).

**`NightMazeApp` (w `NightMazeApp.hpp`):**

```cpp
    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: members are constructed top to bottom. The two renderers ask m_assets for
    // their models in their constructors, so they come after it.
    gfx::Shader m_texturedShader;
    gfx::Shader m_colorShader;
    gfx::Shader m_litShader;
    gfx::Shader m_gouraudShader;
    gfx::Shader m_skyboxShader;
    assets::AssetCache m_assets;
    MazeRenderer m_mazeRenderer;
    GameplayRenderer m_gameplayRenderer;
    ColliderLines m_colliderLines;
    LightRig m_lightRig;
    Skybox m_skybox;
```

Komentarz wymienia dziś jedną zależność, i to ona jest w tej grupie jedyną, od której zależy poprawność:

| Pole | Co robi jego konstruktor | Od czego zależy jego miejsce |
|---|---|---|
| pięć programów shaderów | kompiluje i linkuje program z pary plików | od niczego w tej klasie: potrzebuje tylko kontekstu, a ten daje klasa bazowa |
| `m_assets` | tworzy białą teksturę 1 x 1 | musi stać **przed** oboma rendererami |
| `m_mazeRenderer`, `m_gameplayRenderer` | dostają `m_assets` przez referencję i od razu proszą o modele (każdy o trzy) | **po** `m_assets`. Pole zadeklarowane wyżej dostałoby referencję do obiektu, który jeszcze nie powstał. Działa to też w drugą stronę: giną przed `m_assets`, więc ich wskaźniki nigdy nie wskazują na usunięty model |
| `m_colliderLines` | wysyła na kartę siatkę krawędzi sześcianu i siatkę okręgu | od niczego: siatki są jego własne |
| `m_lightRig` | tworzy bufor uniformów i przypina go do punktu wiązania 1 (cel `GL_UNIFORM_BUFFER`) | od niczego. Programy, które z niego czytają, są łączone z nim dopiero w ciele konstruktora, a ciało wykonuje się po wszystkich polach |
| `m_skybox` (M6) | wczytuje sześć obrazów nieba loaderem obrazów, tworzy z nich teksturę sześcienną i siatkę sześcianu | od niczego: nie korzysta z `m_assets` (czyta pliki sam) ani z programu `skybox`, który dostaje dopiero w argumencie `draw` |

**Czego ta lista już nie wymaga.** Do M4 pod tymi polami stały trzy pola kostki z M1: `gfx::VertexArray` i dwa `gfx::Buffer`. Kostka była zbudowana "na raty": VAO i bufory powstawały na liście inicjalizacyjnej, a opis atrybutów dopiero w ciele konstruktora, i ten opis polegał na tym, że VAO kostki i jej bufor wierzchołków były nadal związane. Wiązanie (binding) to stan globalny kontekstu: w każdej chwili jest jeden związany VAO i jeden bufor w celu `GL_ARRAY_BUFFER`, a każde nowe wiązanie zastępuje poprzednie. Każde pole tworzące siatkę, dopisane pod polami kostki, zabrałoby jej to wiązanie, a kompilator ani OpenGL nie zgłosiłyby błędu. Komentarz w nagłówku miał wtedy pięć punktów o kolejności. Po usunięciu kostki w M5 ta pułapka zniknęła z klasy: wszystkie siatki projektu to dziś `gfx::Mesh`, który opisuje atrybuty we własnym konstruktorze, od razu po utworzeniu swoich buforów ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5), więc nie zostawia niczego "na później" i nie zależy od tego, co powstanie po nim. Sama reguła wiązań obowiązuje dalej, tylko nie ma już w `NightMazeApp` kodu, który by na niej polegał.

Gdyby natomiast ktoś trzymał obiekt z zasobami GL dłużej niż `Application` (na przykład jako zmienną globalną albo lokalną w `main` zadeklarowaną przed `app`), jego destruktor wołałby `glDelete*` bez kontekstu.

Drugie miejsce w tej klasie, gdzie liczy się kolejność, nie dotyczy OpenGL:

```cpp
    Player m_player;
    // Position of the player before the last fixed step. onRender draws from a point
    // between this one and m_player.position. It starts equal to the position of the
    // player, so the frames before the first step are drawn from where the player stands.
    // Declared after m_player, because members are initialized top to bottom.
    glm::vec3 m_previousPlayerPosition = m_player.position;
```

Inicjalizator `= m_player.position` czyta pole innego pola, więc `m_player` musi być zadeklarowany wyżej. Tak samo `m_mazeWorld` (budowany z `m_mazeSettings`) stoi pod `m_mazeSettings`.

Pola rundy (`m_gameplay`, `m_round`, `m_obstacles`) stoją pod `m_mazeWorld`, bo opisują rundę na tym labiryncie, ale ich miejsce nie decyduje o poprawności: żadne nie ma inicjalizatora czytającego inne pole. `m_round` i `m_obstacles` zaczynają puste i dostają zawartość w `beginRound()`, czyli w ciele konstruktora, gdy `m_mazeWorld`, `m_gameplay`, `m_player`, `m_camera` i `m_lighting` już istnieją. To jest powód, dla którego `beginRound` jest zwykłą funkcją wołaną w ciele, a nie wyrażeniem na liście inicjalizacyjnej: czyta dwa pola i zapisuje sześć (`m_round`, `m_obstacles`, `m_lighting`, `m_player`, `m_previousPlayerPosition`, `m_camera`).

Miejsce `m_lighting` (między `m_camera` a `m_viewMode`) nie ma takiego znaczenia: struktura nie czyta żadnego innego pola i nie dotyka OpenGL. Liczy się tylko to, że `m_lightRig`, `m_litShader` i `m_gouraudShader` istnieją, zanim ciało konstruktora zawoła `connect`, a to jest prawdą dla każdej kolejności pól, bo ciało wykonuje się po wszystkich.

**Pułapka do tej sekcji.** Obiekt z zasobami OpenGL żyjący dłużej niż `Window` woła funkcje `gl*` bez kontekstu. Podobnie zmiana kolejności pól w `Application` psuje konstrukcję `Input`. Dotyczy to dziś wprost dziewięciu pól `NightMazeApp` z zasobami OpenGL (pięć programów, `m_assets`, `m_colliderLines`, `m_lightRig` i `m_skybox`). Trzecia odmiana tej samej pułapki: renderer zadeklarowany nad `m_assets` dostaje w konstruktorze referencję do pamięci assetów, która jeszcze nie istnieje (wyżej).

## 8. Różnice między systemami w jednym miejscu

| Temat | macOS | Windows |
|---|---|---|
| Najwyższy OpenGL | 4.1 Core | zwykle 4.6 |
| Wersja w panelu po prośbie o 4.1 | 4.1 | często wyższa (sterownik oddaje nowszy, zgodny kontekst) |
| Framebuffer a okno | 2 razy większy na Retinie | zwykle równe |
| Ostrzeżenia o przestarzałym OpenGL | tak, wyciszane przez `GL_SILENCE_DEPRECATION` | nie |
| Debug callback (4.3) | niedostępny | dostępny, ale nie używam go, żeby kod był jeden |
| Vsync | zwykle respektowany | może być nadpisany w ustawieniach sterownika |

## 9. Pytania kontrolne: gdzie są

Pytania z odpowiedziami są w sekcji 9 każdego dokumentu tematycznego. Razem tworzą zestaw do tematu 1.

| Dokument | Czego dotyczą pytania |
|---|---|
| [`window-context.md`](window-context.md), sekcja 9 | kontekst i `glfwMakeContextCurrent`, `gladLoadGL(glfwGetProcAddress)`, hinty, ręczne `glfwTerminate()` przed `throw`, `reinterpret_cast` przy `glGetString`, rozmiar okna a framebuffera, vsync, `std::flush` w logach |
| [`main-loop.md`](main-loop.md), sekcja 9 | kolejność kroków pętli, stały krok z akumulatorem, spirala śmierci, `alpha()`, `std::chrono::duration<double>`, uśredniony FPS, zero kroków w klatce |
| [`input.md`](input.md), sekcja 9 | `KEY_COUNT` i `static_assert`, `isKeyDown` a `wasKeyPressed`, zakaz `wasKeyPressed` w `onUpdate`, blokada klawiatury i jej opóźnienie, brak fałszywego zbocza po odblokowaniu, odpytywanie myszy, przesunięcie i problem pierwszego odczytu, tryby kursora, surowy ruch myszy, Escape a przechwycony kursor |
| [`gl-check.md`](gl-check.md), sekcja 9 | działanie `GL_CHECK`, dlaczego makro, pętla `glGetError`, Debug a Release, błąd przypisany nie temu wywołaniu |
| [`paths.md`](paths.md), sekcja 9 | katalog roboczy, dlaczego nie `argv[0]`, dwa wywołania systemowe za `#if`, `canonical`, `GetModuleFileNameW` i znaki szerokie, `operator/` zamiast sklejania napisów |
| ten plik, niżej | kolejność pól i niszczenia, miejsce `DebugUI` w architekturze, klasa `NightMazeApp` |

Pytania dotyczące treści tego pliku:

1. **Dlaczego kolejność pól `m_window`, `m_input`, `m_time` ma znaczenie?**
   Pola powstają w kolejności deklaracji, a `Input` potrzebuje uchwytu z już istniejącego `Window`. Niszczenie idzie odwrotnie, więc okno i kontekst znikają na końcu, po wszystkim, co z nich korzysta (w tym po `DebugUI`, które jest polem klasy pochodnej w `main.cpp`, a pola giną przed klasami bazowymi).

2. **Dlaczego `DebugUI` jest polem klasy w `main.cpp`, a nie w `game::NightMazeApp`?**
   Bo `game/` nie może zależeć od `debug/` (warstwy z PRD). `main.cpp` to jedyne miejsce, które łączy obie warstwy: `DebugNightMazeApp` dziedziczy po grze, w `onRender` woła najpierw `game::NightMazeApp::onRender(alpha)`, a potem rysuje panele i przekazuje do `core::Input` flagi blokady klawiatury i myszy.

3. **Dlaczego `m_mazeRenderer` i `m_gameplayRenderer` są zadeklarowane po `m_assets`?**
   Bo ich konstruktory dostają `m_assets` przez referencję i od razu proszą o modele, a pola powstają w kolejności deklaracji. Przy niszczeniu kolejność jest odwrotna: oba renderery giną przed pamięcią assetów, więc ich wskaźniki do modeli nigdy nie wskazują na usunięty obiekt. Do M4 lista miała jeszcze jedną zależność (pola tworzące siatkę musiały stać przed kostką z M1), która zniknęła razem z kostką.

4. **Dlaczego pierwszy labirynt powstaje na liście inicjalizacyjnej konstruktora?**
   `MazeWorld` nie ma konstruktora domyślnego, bo zawiera `Maze`, który wymaga rozmiaru. Pole bez konstruktora domyślnego musi dostać wartość na liście. W ciele konstruktora zostają dwa wywołania `m_lightRig.connect(...)` i `beginRound()`.

5. **Co dokładnie zmienia `beginRound`, a czego nie?**
   Przypisuje nowy stan rundy (`startRound`: wszystkie kryształy na miejscu, pełna bateria, brama zamknięta, zegary na 0), buduje listę przeszkód (`roundObstacles`), włącza latarkę, ustawia pozycję gracza i pozycję poprzednią na start, pozycję kamery na oczy gracza, yaw na otwarty bok komórki startowej i pitch na 0. Nie zmienia labiryntu, trybu noclip, prędkości, trybu widoku, rysowania kształtów kolizji, czułości myszy, liczb reguł w `m_gameplay` ani pozostałych ustawień oświetlenia. Wołają ją konstruktor, `regenerateMaze` i `onRender` po klawiszu R albo fladze `restart`.

6. **Dlaczego prośba o nowy labirynt i prośba o nową rundę są obsługiwane na początku `onRender`, a nie w panelu?**
   Panel tylko zapisuje dane (`MazeSettings::regenerate`). Gdyby sam wymieniał labirynt, robiłby to w środku klatki, po narysowaniu sceny ze starego labiryntu, a warstwa `debug/` decydowałaby o stanie gry. Na początku `onRender` wymiana dzieje się poza krokami symulacji i przed rysowaniem, więc klatka jest w całości z nowego labiryntu. Flaga `GameplaySettings::restart` działa tak samo i z tego samego powodu: runda zaczyna się między dwoma stałymi krokami, nigdy w środku kroku.

7. **Dlaczego klawisze R, N i F są czytane w `onRender`, a W, S, A, D w `onUpdate`?**
   R, N i F to zbocza (`wasKeyPressed`): są prawdą przez jedną klatkę, a `onUpdate` wykonuje się od zera do wielu razy na klatkę, więc naciśnięcie mogłoby przepaść albo zadziałać kilka razy. Klawisze ruchu to stan ciągły (`isKeyDown`), a ruch zależy od czasu, który płynie stałymi krokami. R woła `beginRound()`, N przełącza `m_player.noclip`, F przełącza `m_lighting.flashlightOn`. Wszystkie trzy działają też przy wolnym kursorze, a nie działają, gdy klawiaturę ma ImGui.

8. **Dlaczego macierze `view` i `projection` są liczone raz, a ustawiane kilka razy?**
   Liczone raz, bo kamera jest jedna dla całej klatki. Ustawiane w każdej funkcji rysującej, bo uniformy należą do programu, a każdy program ma własne `uView` i `uProjection`. Programów jest pięć, w jednej klatce pracują najwyżej trzy różne (jeden program sceny, przy włączonych liniach kolizji `color` i, przy włączonym niebie, `skybox`), więc macierze są ustawiane najwyżej trzy razy: raz w `drawUnlitMaze` albo `drawLitMaze`, raz w `drawColliderLines` i raz w `Skybox::draw`. `MazeRenderer` i `GameplayRenderer` dostają program już z macierzami i ustawiają tylko `uModel` i uniformy materiału.

9. **Jakie pola i funkcje doszły w `NightMazeApp` z oświetleniem (M4) i w jakiej kolejności stoją?**
   Programy `m_litShader` i `m_gouraudShader` zaraz po dwóch wcześniejszych, `m_lightRig` po `m_colliderLines` (posiada bufor uniformów), `m_lighting` po `m_camera` (zwykłe dane). Konstruktor woła dwa razy `m_lightRig.connect(...)`, po razie dla każdego programu z oświetleniem. `onRender` czyta klawisz F, buduje `buildLightSet(...)` z interpolowanego oka i `m_camera.forward()`, woła `m_lightRig.upload(lights, eye)`, a `drawMaze` wybiera `drawUnlitMaze` albo `drawLitMaze`. Doszły trzy akcesory: `litShader()`, `gouraudShader()`, `lighting()`. Funkcja `drawLightMarkers` z M4 została usunięta w M5, a `buildLightSet` dostaje dziś kopię ustawień z `lightingForFrame` i pozycje z `crystalLightPositions`.

10. **Od czego zależy, którym programem rysowany jest labirynt?**
    Od dwóch pól: `m_lighting.mode` i `m_viewMode`. Tryb `Unlit` albo widok inny niż `Textured` daje `drawUnlitMaze` i program `textured`. W pozostałych przypadkach `drawLitMaze` bierze `m_gouraudShader` dla trybu `Gouraud` i `m_litShader` dla trybów `Phong` i `BlinnPhong`, które różnią się tylko wartością uniformu `uSpecularModel`.

11. **Dlaczego światła są budowane w `onRender`, po macierzach, a nie w `onUpdate`?**
    Latarka ma stać w punkcie, z którego rysowana jest klatka, i świecić tam, gdzie patrzy kamera po obrocie myszą z tej klatki. Tym punktem jest interpolowane oko, które istnieje tylko w `onRender`. Z pozycji po ostatnim stałym kroku stożek zostawałby w ruchu za obrazem.

12. **Co doszło w `NightMazeApp` z rozgrywką (M5) i gdzie stoi w klatce?**
    Pola `m_gameplayRenderer` (po `m_mazeRenderer`, bo też czyta `m_assets`), `m_gameplay`, `m_round` i `m_obstacles` (po `m_mazeWorld`), funkcja `beginRound` w miejsce `enterMaze` i akcesory `gameplaySettings()` oraz `round()`. W stałym kroku: gracz rusza się względem `m_obstacles`, potem `updateRound` dostaje jego nową pozycję, przełącznik latarki przez referencję i `fixedDt`, a gdy `gateBlocks` zmieniło wynik, lista przeszkód jest budowana od nowa. Raz na klatkę: flaga `restart` albo klawisz R wołają `beginRound`, `lightingForFrame` i `crystalLightPositions` przygotowują światła, a `m_gameplayRenderer.draw` rysuje bramę i kryształy tym samym programem co labirynt.

13. **Dlaczego reguły rundy są wołane w `onUpdate`, a nie w `onRender`?**
    Runda jest stanem symulacji, tak jak gracz: bateria ubywa z czasem, a zebranie kryształu zależy od pozycji gracza po kroku. W stałym kroku wynik nie zależy od liczby klatek na sekundę i da się go sprawdzić testem bez okna (`tests/RoundTests.cpp` woła `updateRound` z własnym czasem kroku). W `onRender` zostaje tylko to, co dotyczy jednej klatki: zbocza klawiszy i rysowanie.

14. **Dlaczego klawisz chowający panele nie chowa HUD?**
    `DebugUI::draw` rysuje panele tylko wtedy, gdy `m_visible` jest prawdą, a `drawHud` woła zawsze. HUD jest częścią gry, a w `src/debug/` leży tylko dlatego, że tam jest ImGui, którego `game/` nie może dołączać.

## 10. Źródła

- LearnOpenGL, rozdziały "Creating a window" (<https://learnopengl.com/Getting-started/Creating-a-window>) i "Hello Window" (<https://learnopengl.com/Getting-started/Hello-Window>).
- Glenn Fiedler, "Fix Your Timestep!", Gaffer on Games: <https://gafferongames.com/post/fix_your_timestep/>.
- Dokumenty bibliotek w tym repozytorium: [`../../libraries/glfw.md`](../../libraries/glfw.md), [`../../libraries/glad.md`](../../libraries/glad.md), [`../../libraries/imgui.md`](../../libraries/imgui.md).
- Szczegółowe źródła do każdego zagadnienia są w sekcji 10 dokumentów tematycznych.
