# Sylabus: temat wykładu, dokument, kod, przełącznik w ImGui

Ściąga przed kartkówką i obroną. Gdy prowadzący zapyta o temat, w tej tabeli znajduję plik, który go realizuje, dokument, który go tłumaczy, i miejsce w panelu ImGui, w którym pokażę efekt na żywo.

Stan: **M0 zrobione, M1 w toku**. Zrealizowane są tematy 1 i 2, temat 3 jest w toku. Temat 2 to klasy `gfx::Shader`, `gfx::Buffer` i `gfx::VertexArray`, shadery `assets/shaders/basic.*`, kostka rysowana przez `game::NightMazeApp` z bufora wierzchołków i bufora indeksów oraz panel Shaders z przyciskiem "Reload shaders". Z tematu 3 są struktury `scene::Transform` i `scene::Camera`, trzy macierze wysyłane do shadera jako uniformy i test głębi. Brakuje sterowania kamerą i panelu Camera. Tematy od 4 do 15 są **planowane**: podaję dla nich kamień milowy i zakres z PRD (sekcje 3, 7 i 11), bez odnośników, bo tych plików jeszcze nie ma. Wiersz zostaje uzupełniony w tym samym commicie, w którym powstaje kod tematu.

## Tabela tematów

| # | Temat wykładu | Status | Dokument | Pliki kodu | Przełącznik w ImGui |
|---|---|---|---|---|---|
| 1 | Pierwszy program OpenGL | **zrobione (M0)** | [`modules/core/README.md`](modules/core/README.md) (wstęp), [`window-context.md`](modules/core/window-context.md), [`main-loop.md`](modules/core/main-loop.md), [`input.md`](modules/core/input.md), [`gl-check.md`](modules/core/gl-check.md), [`paths.md`](modules/core/paths.md) | zob. tabela "Temat 1 szczegółowo" niżej | Panel Renderer: `FPS`, `Frame time`, `Framebuffer`, `Window`, `OpenGL`, `GPU`, `Clear color`. Klawisz `~` (na lewo od `1`) chowa i pokazuje panele |
| 2 | Programowalny potok | **zrobione (M1)** | [`modules/gfx/README.md`](modules/gfx/README.md) (wstęp, droga klatki), [`shaders.md`](modules/gfx/shaders.md), [`buffers-vao.md`](modules/gfx/buffers-vao.md) | [`src/gfx/`](../src/gfx/) (Shader, Buffer, VertexArray), [`assets/shaders/basic.vert`](../assets/shaders/basic.vert), [`basic.frag`](../assets/shaders/basic.frag), [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`src/debug/panels/ShadersPanel.cpp`](../src/debug/panels/ShadersPanel.cpp), zob. tabela "Temat 2 szczegółowo" niżej | Panel Shaders: przycisk `Reload shaders` (wczytanie shaderów na żywo), `Vertex`, `Fragment`, `Program`, `Last load` z tekstem błędu sterownika. Scenariusz pokazu: [`shaders.md`](modules/gfx/shaders.md), sekcja 6.4 |
| 3 | Przekształcenia przestrzeni | **w toku (M1)**: macierze model, view i projection działają (kostka na ekranie), sterowanie kamerą i panel w toku | [`modules/scene/README.md`](modules/scene/README.md) (wstęp), [`transforms-camera.md`](modules/scene/transforms-camera.md) | [`src/scene/`](../src/scene/) (Transform, Camera), [`assets/shaders/basic.vert`](../assets/shaders/basic.vert), [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) (`setMat4`), zob. tabela "Temat 3 szczegółowo" niżej. Brakuje: sterowanie kamerą | planowane: panel Camera (pozycja i rotacja kamery, FOV) |
| 4 | Wczytywanie OBJ | planowane, M2 + M3 | planowane: `obj-loader.md` | planowane: `src/assets/` (ObjLoader, AssetCache) | planowane: lista załadowanych modeli |
| 5 | Tekstury | planowane, M2 + M3 | planowane: `textures.md`, `images.md` | planowane: `src/gfx/` (Texture2D), `src/assets/` (ImageLoader) | planowane: podgląd tekstur, przełącznik normal map |
| 6 | Światło kierunkowe i punktowe | planowane, M4 + M5 | planowane: `lights.md` | planowane: `src/scene/` (Light), `src/game/` (Flashlight, Crystal) | planowane: edytor świateł (kolor, siła, tłumienie) |
| 7 | Gouraud vs Phong | planowane, M4 + M5 | planowane: `lighting-gouraud-phong.md` | planowane: `src/renderer/`, shadery `lit` i `gouraud` | planowane: lista wyboru Gouraud / Phong / Blinn-Phong |
| 8 | Tekstura sześcienna | planowane, M6 | planowane: `skybox.md` | planowane: `src/gfx/` (Cubemap), `src/renderer/` (SkyboxPass) | planowane: przełącznik skyboxa |
| 9 | Shader geometrii | planowane, M6 | planowane: `grass-geometry.md` | planowane: `src/renderer/` (GrassPass), shadery `grass` | planowane: gęstość trawy, przełącznik |
| 10 | Rendering pozaekranowy | planowane, M7 | planowane: `framebuffers.md`, `post-process.md` | planowane: `src/gfx/` (Framebuffer), `src/renderer/` (PostProcess) | planowane: podgląd załączników FBO |
| 11 | Shadow mapping | planowane, M7 | planowane: `shadows.md` | planowane: `src/renderer/` (ShadowPass) | planowane: podgląd shadow mapy, bias |
| 12 | Environment mapping | planowane, M8 | planowane: `env-mapping.md` | planowane: `src/renderer/`, shadery `reflect` | planowane: współczynnik odbicia i refrakcji |
| 13 | Implementacja podłoża | planowane, M6 | planowane: `terrain.md` | planowane: `src/renderer/`, shadery `terrain` | planowane: skala wysokości, wireframe |
| 14 | Wstęp do kolizji | planowane, M2 + M3 | planowane: `collision.md` | planowane: `src/scene/` (Collider) | planowane: rysowanie brył kolizji |
| 15 | Selekcja obiektów | planowane, M8 | planowane: `picking.md` | planowane: `src/scene/` (Raycast) | planowane: podświetlenie wybranego obiektu |

Nazwy planowanych dokumentów i klas pochodzą z PRD (sekcje 6 i 7) i mogą się zmienić w trakcie pracy. Obowiązuje to, co jest w tabeli po oznaczeniu tematu jako zrobiony.

## Temat 1 szczegółowo

Realizacja według PRD: okno GLFW, kontekst 4.1 Core, pętla gry ze stałym krokiem (fixed timestep). Pokaz w ImGui: FPS i czas klatki.

| Zagadnienie | Plik | Najważniejsze miejsce w kodzie | Dokument i sekcja |
|---|---|---|---|
| Punkt wejścia, obsługa błędów startu, podpięcie nakładki debug | [`src/main.cpp`](../src/main.cpp) | `main`, klasa `DebugNightMazeApp` | [`core/README.md`](modules/core/README.md), 5 i 6 |
| Okno, kontekst 4.1 Core, GLAD, vsync | [`src/core/Window.cpp`](../src/core/Window.cpp), [`Window.hpp`](../src/core/Window.hpp) | konstruktor `Window::Window`, `framebufferSize`, `windowSize` | [`core/window-context.md`](modules/core/window-context.md), 3.1 i 5.2 |
| Pętla główna | [`src/core/Application.cpp`](../src/core/Application.cpp), [`Application.hpp`](../src/core/Application.hpp) | `Application::run`, `onUpdate`, `onRender` | [`core/main-loop.md`](modules/core/main-loop.md), 2.1 i 5.2 |
| Stały krok, akumulator, `alpha`, FPS | [`src/core/Time.cpp`](../src/core/Time.cpp), [`Time.hpp`](../src/core/Time.hpp) | `beginFrame`, `consumeFixedStep`, `alpha`, stałe `FIXED_DT` i `MAX_FRAME_TIME` | [`core/main-loop.md`](modules/core/main-loop.md), 2.2 do 2.4 i 5.4 |
| Klawiatura, blokada klawiatury | [`src/core/Input.cpp`](../src/core/Input.cpp), [`Input.hpp`](../src/core/Input.hpp) | `update`, `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`, `KEY_COUNT` | [`core/input.md`](modules/core/input.md), 5.2 do 5.6 |
| Mysz: przyciski, przesunięcie, przechwycenie kursora, blokada myszy | [`src/core/Input.cpp`](../src/core/Input.cpp), [`Input.hpp`](../src/core/Input.hpp), [`src/core/Application.cpp`](../src/core/Application.cpp) | `update`, `isMouseButtonDown`, `wasMouseButtonPressed`, `mouseDeltaX`, `mouseDeltaY`, `setCursorCaptured`, `setMouseBlocked`, `MOUSE_BUTTON_COUNT`, obsługa Escape w `Application::run` | [`core/input.md`](modules/core/input.md), 2.4 do 2.8 i 5.7 do 5.10 |
| Błędy OpenGL | [`src/core/GlCheck.hpp`](../src/core/GlCheck.hpp), [`GlCheck.cpp`](../src/core/GlCheck.cpp) | makro `GL_CHECK`, `checkGlErrors` | [`core/gl-check.md`](modules/core/gl-check.md), 5.2 i 5.3 |
| Ścieżki do assetów względem pliku wykonywalnego, katalog `assets` obok programu | [`src/core/Paths.cpp`](../src/core/Paths.cpp), [`Paths.hpp`](../src/core/Paths.hpp) | `executableDir`, `assetPath`, `pathText`, pomocnicza `executableFile` w gałęziach `#if` dla macOS i Windows, stała `ASSETS_DIRECTORY` | [`core/paths.md`](modules/core/paths.md), 2.3 do 2.6 i 5.3 do 5.7 |
| Logowanie | [`src/core/Log.cpp`](../src/core/Log.cpp), [`Log.hpp`](../src/core/Log.hpp) | `logInfo`, `logWarn`, `logError` | [`core/window-context.md`](modules/core/window-context.md), 5.5 |
| Klatka gry: viewport i czyszczenie (rysowanie kostki: tematy 2 i 3) | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`NightMazeApp.hpp`](../src/game/NightMazeApp.hpp) | `NightMazeApp::onRender`, `clearColor` | [`core/window-context.md`](modules/core/window-context.md), 3.2, oraz [`core/README.md`](modules/core/README.md), 6 |
| Kolejność pól i niszczenia | [`src/core/Application.hpp`](../src/core/Application.hpp), [`src/main.cpp`](../src/main.cpp) | pola `m_window`, `m_input`, `m_time`, pole `m_debugUI` | [`core/README.md`](modules/core/README.md), 7 |
| Loader funkcji OpenGL | [`external/glad/`](../external/glad/) | kod generowany, opis w [`libraries/glad.md`](libraries/glad.md) | [`core/window-context.md`](modules/core/window-context.md), 5.2 |

Dokumenty uzupełniające do tematu 1: [`libraries/glfw.md`](libraries/glfw.md), [`libraries/glad.md`](libraries/glad.md).

## Temat 2 szczegółowo

Realizacja według PRD: klasa Shader, hot-reload GLSL z dysku. Pokaz w ImGui: przycisk "Reload shaders". Stan: `game::NightMazeApp` rysuje kostkę o sześciu kolorowych ścianach klasami `gfx::Shader`, `gfx::Buffer` (bufor wierzchołków i bufor indeksów) i `gfx::VertexArray` oraz shaderami `basic.vert` i `basic.frag`. Shader jest wczytywany przy starcie i ponownie po każdym naciśnięciu przycisku "Reload shaders" w panelu Shaders.

| Zagadnienie | Plik | Najważniejsze miejsce w kodzie | Dokument i sekcja |
|---|---|---|---|
| Potok renderowania, etapy programowalne, GLSL | [`assets/shaders/basic.vert`](../assets/shaders/basic.vert), [`basic.frag`](../assets/shaders/basic.frag) | `layout(location = N) in`, `uniform mat4`, `out vec3 vColor`, `gl_Position`, `fragColor` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.1 do 2.4 i 4 |
| RAII i przenoszenie dla obiektów OpenGL | [`src/gfx/Shader.hpp`](../src/gfx/Shader.hpp), [`Shader.cpp`](../src/gfx/Shader.cpp) | destruktor, `= delete`, konstruktor przenoszący, przypisanie przenoszące | [`gfx/README.md`](modules/gfx/README.md), 2, oraz [`gfx/shaders.md`](modules/gfx/shaders.md), 5.9 |
| Wczytanie pliku shadera | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp), [`src/core/Paths.cpp`](../src/core/Paths.cpp) | `readTextFile`, `core::pathText` | [`gfx/shaders.md`](modules/gfx/shaders.md), 5.3, [`core/paths.md`](modules/core/paths.md), 5.7 |
| Kompilacja, status i dziennik shadera | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `compileShader`, `shaderInfoLog` | [`gfx/shaders.md`](modules/gfx/shaders.md), 3.1, 3.3, 5.4 i 5.5 |
| Linkowanie, status i dziennik programu | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `linkProgram`, `programInfoLog`, `buildProgram` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.5, 2.6, 5.6 i 5.7 |
| Wczytywanie na żywo z zachowaniem starego programu | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `Shader::reload`, `lastError`, `isValid` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.7 i 5.8 |
| Uniformy: położenie, `glUniformMatrix4fv`, program w użyciu, położenie -1 | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp), [`Shader.hpp`](../src/gfx/Shader.hpp), [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp) | `Shader::setMat4`, stałe `MODEL_UNIFORM`, `VIEW_UNIFORM`, `PROJECTION_UNIFORM` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.8, 3.1, 5.10 i 5.12 |
| Pokaz w ImGui: panel Shaders, przycisk "Reload shaders", przeładowanie w środku klatki ImGui | [`src/debug/panels/ShadersPanel.cpp`](../src/debug/panels/ShadersPanel.cpp), [`ShadersPanel.hpp`](../src/debug/panels/ShadersPanel.hpp), [`src/game/NightMazeApp.hpp`](../src/game/NightMazeApp.hpp), [`src/gfx/Shader.hpp`](../src/gfx/Shader.hpp) | `drawShadersPanel`, `ERROR_TEXT_COLOR`, akcesor `NightMazeApp::shader`, `Shader::vertexPath`, `Shader::fragmentPath` | [`gfx/shaders.md`](modules/gfx/shaders.md), 6 |
| Bufor wierzchołków i bufor indeksów, `glBufferData`, podpowiedź użycia | [`src/gfx/Buffer.cpp`](../src/gfx/Buffer.cpp), [`Buffer.hpp`](../src/gfx/Buffer.hpp) | konstruktor `Buffer::Buffer`, `bind`, pole `m_target` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.2, 2.6, 2.8 i 5.3 |
| Tablica wierzchołków: co pamięta VAO, dlaczego Core jej wymaga | [`src/gfx/VertexArray.cpp`](../src/gfx/VertexArray.cpp), [`VertexArray.hpp`](../src/gfx/VertexArray.hpp) | konstruktor (wiąże nowy VAO), `bind` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.4, 2.5 i 5.5 |
| Atrybuty wierzchołka, krok i przesunięcie, przesunięcie jako wskaźnik | [`src/gfx/VertexArray.cpp`](../src/gfx/VertexArray.cpp) | `VertexArray::setFloatAttribute` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.3, 4 i 5.6 |
| Dane kostki: 24 wierzchołki, 36 indeksów, stałe układu (krok, przesunięcia), kolejność tworzenia obiektów | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`NightMazeApp.hpp`](../src/game/NightMazeApp.hpp) | `VERTICES`, `INDICES`, `VERTEX_STRIDE`, `COLOR_OFFSET`, `INDEX_COUNT`, konstruktor `NightMazeApp`, pola `m_shader`, `m_vertexArray`, `m_vertexBuffer`, `m_indexBuffer` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 5.7 |
| Rysowanie: `glDrawElements`, bufor indeksów, współrzędne lokalne, kierunek nawijania | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp) | `NightMazeApp::onRender` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.7, 2.9 i 5.7, [`gfx/shaders.md`](modules/gfx/shaders.md), 5.10 |
| Cała klatka: od tablicy liczb do pikseli | wszystkie powyższe | diagram | [`gfx/README.md`](modules/gfx/README.md), 6 |
| Shadery jako pliki obok programu | [`CMakeLists.txt`](../CMakeLists.txt), [`src/core/Paths.cpp`](../src/core/Paths.cpp) | blok `assets` (dowiązanie przez `POST_BUILD` na macOS, kopia przez target `copy_assets` na Windowsie), `core::assetPath` | [`core/paths.md`](modules/core/paths.md), 5.8, [`guides/project-structure.md`](guides/project-structure.md), 3.1 blok 7 |

## Temat 3 szczegółowo

Realizacja według PRD: Model/View/Projection, kamera FPS, hierarchia transformów. Pokaz w ImGui: pozycja i rotacja kamery, FOV. Stan: struktury `scene::Transform` i `scene::Camera` liczą trzy macierze, `NightMazeApp` wysyła je co klatkę do `basic.vert` i rysuje obróconą kostkę z testem głębi. Kamera stoi w miejscu: sterowania i panelu Camera jeszcze nie ma. Hierarchii transformów nie ma.

| Zagadnienie | Plik | Najważniejsze miejsce w kodzie | Dokument i sekcja |
|---|---|---|---|
| Przestrzenie współrzędnych, współrzędne jednorodne, konwencja układu | teoria | diagram łańcucha przestrzeni | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.1, 2.2 i 2.11 |
| Macierz modelu: przesunięcie, obrót, skala i ich kolejność | [`src/scene/Transform.cpp`](../src/scene/Transform.cpp), [`Transform.hpp`](../src/scene/Transform.hpp) | `Transform::matrix`, pola `position`, `rotationDegrees`, `scale`, stałe `AXIS_X`, `AXIS_Y`, `AXIS_Z` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.3 do 2.6, 5.2 i 5.3 |
| Kierunek patrzenia z kątów yaw i pitch, wektor w prawo | [`src/scene/Camera.cpp`](../src/scene/Camera.cpp), [`Camera.hpp`](../src/scene/Camera.hpp) | `Camera::forward`, `Camera::right`, stała `WORLD_UP` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.8 i 5.5 |
| Obrót kamery: zawijanie yaw, ograniczenie pitch | [`src/scene/Camera.cpp`](../src/scene/Camera.cpp) | `Camera::rotate`, stałe `MAX_PITCH_DEGREES`, `FULL_TURN_DEGREES` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.8 i 5.6 |
| Macierz widoku, `lookAt` | [`src/scene/Camera.cpp`](../src/scene/Camera.cpp) | `Camera::viewMatrix` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.7 i 5.7 |
| Rzutowanie perspektywiczne, FOV, proporcje, bliska i daleka płaszczyzna, głębia | [`src/scene/Camera.cpp`](../src/scene/Camera.cpp), [`Camera.hpp`](../src/scene/Camera.hpp) | `Camera::projectionMatrix`, pola `fovDegrees`, `nearPlane`, `farPlane` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 2.9, 2.10 i 5.7 |
| Trzy macierze w shaderze, kolejność mnożenia | [`assets/shaders/basic.vert`](../assets/shaders/basic.vert) | `uniform mat4 uModel`, `uView`, `uProjection`, linia `gl_Position = ...` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 4, [`gfx/shaders.md`](modules/gfx/shaders.md), 4.1 |
| Wysłanie macierzy, proporcje z framebuffera, okno zminimalizowane | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`NightMazeApp.hpp`](../src/game/NightMazeApp.hpp) | `NightMazeApp::onRender`, pola `m_cubeTransform` i `m_camera`, stałe `CUBE_ROTATION_X_DEGREES`, `CUBE_ROTATION_Y_DEGREES` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 5.9 |
| Test głębi i czyszczenie bufora głębi | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp) | `glEnable(GL_DEPTH_TEST)`, `glClear` z `GL_DEPTH_BUFFER_BIT` w `onRender` | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 3 i 5.9 |
| Droga jednego wierzchołka od bufora do piksela, na liczbach | wszystkie powyższe | wierzchołek 2 kostki | [`scene/transforms-camera.md`](modules/scene/transforms-camera.md), 5.10 |
| Miejsce warstwy `scene` w architekturze | [`CMakeLists.txt`](../CMakeLists.txt) | lista źródeł targetu `engine` | [`scene/README.md`](modules/scene/README.md), 2 i 3 |

Dokument uzupełniający do tematu 3: [`libraries/glm.md`](libraries/glm.md).

## Panele ImGui (narzędzie do wszystkich tematów)

| Zagadnienie | Dokument | Pliki kodu |
|---|---|---|
| Cykl życia ImGui, klatka ImGui, dockspace | [`modules/debug-ui.md`](modules/debug-ui.md) | [`src/debug/DebugUI.cpp`](../src/debug/DebugUI.cpp), [`DebugUI.hpp`](../src/debug/DebugUI.hpp) |
| Dane dla paneli: struktura `DebugContext` | [`modules/debug-ui.md`](modules/debug-ui.md) (sekcja 5.2) | [`src/debug/DebugContext.hpp`](../src/debug/DebugContext.hpp), [`src/main.cpp`](../src/main.cpp) |
| Panel Renderer | [`modules/debug-ui.md`](modules/debug-ui.md) | [`src/debug/panels/RendererPanel.cpp`](../src/debug/panels/RendererPanel.cpp), [`RendererPanel.hpp`](../src/debug/panels/RendererPanel.hpp) |
| Panel Shaders | [`modules/gfx/shaders.md`](modules/gfx/shaders.md) (sekcja 6), [`modules/debug-ui.md`](modules/debug-ui.md) (sekcje 5.5 i 6) | [`src/debug/panels/ShadersPanel.cpp`](../src/debug/panels/ShadersPanel.cpp), [`ShadersPanel.hpp`](../src/debug/panels/ShadersPanel.hpp) |
| Podpięcie nakładki do gry, klawisz `~`, blokada klawiatury i myszy gry | [`modules/debug-ui.md`](modules/debug-ui.md) (sekcje 5.4 i 5.6), [`modules/core/input.md`](modules/core/input.md) (sekcje 5.6 i 5.10) | [`src/main.cpp`](../src/main.cpp) |
| Biblioteka | [`libraries/imgui.md`](libraries/imgui.md) | [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake) |

## Kamienie milowe a tematy

| Kamień milowy | Zakres według PRD | Tematy wykładu |
|---|---|---|
| M0 (zrobione) | Repozytorium, CMake i FetchContent, okno GLFW 4.1, GLAD, ImGui, `GL_CHECK` | 1 |
| M1 (w toku) | `gfx`: Shader, Buffer, VAO. Trójkąt, potem kostka z MVP. Kamera FPS. Zrobione: mysz, ścieżki do assetów, GLM, klasy `Shader`, `Buffer`, `VertexArray`, panel Shaders z przyciskiem "Reload shaders", struktury `scene::Transform` i `scene::Camera`, kostka z macierzami model, view i projection. Zostało: sterowanie kamerą, panel Camera | 2, 3 |
| M2 + M3 | Generator labiryntu, kolizje AABB, tekstury, loader OBJ, pierwsze modele | 4, 5, 14 |
| M4 + M5 | Księżyc, latarka, kryształy, Gouraud vs Phong, zbieranie, bateria, brama | 6, 7 |
| M6 | Skybox, teren z heightmapy, trawa w shaderze geometrii | 8, 9, 13 |
| M7 | HDR FBO, bloom, mgła, minimapa, shadow mapping latarki i księżyca | 10, 11 |
| M8 | Environment mapping, selekcja obiektów, dźwignie i notatki | 12, 15 |
| M9 | Szlif, menu, balans, przygotowanie do code review | powtórka wszystkich |
