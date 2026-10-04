# Sylabus: temat wykładu, dokument, kod, przełącznik w ImGui

Ściąga przed kartkówką i obroną. Gdy prowadzący zapyta o temat, w tej tabeli znajduję plik, który go realizuje, dokument, który go tłumaczy, i miejsce w panelu ImGui, w którym pokażę efekt na żywo.

Stan: **M0 zrobione, M1 w toku**. Zrealizowany jest temat 1. Temat 2 jest zrobiony **częściowo**: istnieją klasy `gfx::Shader`, `gfx::Buffer` i `gfx::VertexArray` z dokumentami, ale nie ma jeszcze plików shaderów, trójkąta ani przycisku w ImGui. Tematy od 3 do 15 są **planowane**: podaję dla nich kamień milowy i zakres z PRD (sekcje 3, 7 i 11), bez odnośników, bo tych plików jeszcze nie ma. Wiersz zostaje uzupełniony w tym samym commicie, w którym powstaje kod tematu.

## Tabela tematów

| # | Temat wykładu | Status | Dokument | Pliki kodu | Przełącznik w ImGui |
|---|---|---|---|---|---|
| 1 | Pierwszy program OpenGL | **zrobione (M0)** | [`modules/core/README.md`](modules/core/README.md) (wstęp), [`window-context.md`](modules/core/window-context.md), [`main-loop.md`](modules/core/main-loop.md), [`input.md`](modules/core/input.md), [`gl-check.md`](modules/core/gl-check.md), [`paths.md`](modules/core/paths.md) | zob. tabela "Temat 1 szczegółowo" niżej | Panel Renderer: `FPS`, `Frame time`, `Framebuffer`, `Window`, `OpenGL`, `GPU`, `Clear color`. Klawisz `~` (na lewo od `1`) chowa i pokazuje panele |
| 2 | Programowalny potok | **częściowo (M1 w toku)** | [`modules/gfx/README.md`](modules/gfx/README.md) (wstęp), [`shaders.md`](modules/gfx/shaders.md), [`buffers-vao.md`](modules/gfx/buffers-vao.md) | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp), [`Buffer.cpp`](../src/gfx/Buffer.cpp), [`VertexArray.cpp`](../src/gfx/VertexArray.cpp) i ich nagłówki, zob. tabela "Temat 2 szczegółowo" niżej. Planowane: pliki `assets/shaders/`, trójkąt w `src/game/` | jeszcze nic. Planowane: panel Shaders z przyciskiem "Reload shaders" i ostatnim błędem |
| 3 | Przekształcenia przestrzeni | planowane, M1 | planowane: `transforms-camera.md` | planowane: `src/scene/` (Transform, Camera) | planowane: pozycja i rotacja kamery, FOV |
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
| Ścieżki do assetów względem pliku wykonywalnego (jeszcze bez użytkownika w kodzie) | [`src/core/Paths.cpp`](../src/core/Paths.cpp), [`Paths.hpp`](../src/core/Paths.hpp) | `executableDir`, `assetPath`, pomocnicza `executableFile` w gałęziach `#if` dla macOS i Windows, stała `ASSETS_DIRECTORY` | [`core/paths.md`](modules/core/paths.md), 2.3 do 2.6 i 5.3 do 5.7 |
| Logowanie | [`src/core/Log.cpp`](../src/core/Log.cpp), [`Log.hpp`](../src/core/Log.hpp) | `logInfo`, `logWarn`, `logError` | [`core/window-context.md`](modules/core/window-context.md), 5.5 |
| Klatka gry: viewport i czyszczenie | [`src/game/NightMazeApp.cpp`](../src/game/NightMazeApp.cpp), [`NightMazeApp.hpp`](../src/game/NightMazeApp.hpp) | `NightMazeApp::onRender`, `clearColor` | [`core/window-context.md`](modules/core/window-context.md), 3.2, oraz [`core/README.md`](modules/core/README.md), 6 |
| Kolejność pól i niszczenia | [`src/core/Application.hpp`](../src/core/Application.hpp), [`src/main.cpp`](../src/main.cpp) | pola `m_window`, `m_input`, `m_time`, pole `m_debugUI` | [`core/README.md`](modules/core/README.md), 7 |
| Loader funkcji OpenGL | [`external/glad/`](../external/glad/) | kod generowany, opis w [`libraries/glad.md`](libraries/glad.md) | [`core/window-context.md`](modules/core/window-context.md), 5.2 |

Dokumenty uzupełniające do tematu 1: [`libraries/glfw.md`](libraries/glfw.md), [`libraries/glad.md`](libraries/glad.md).

## Temat 2 szczegółowo

Realizacja według PRD: klasa Shader, hot-reload GLSL z dysku. Pokaz w ImGui: przycisk "Reload shaders". Stan: istnieją klasy `gfx::Shader`, `gfx::Buffer` i `gfx::VertexArray`, których żaden kod programu jeszcze nie używa. Nie ma plików shaderów, trójkąta ani panelu.

| Zagadnienie | Plik | Najważniejsze miejsce w kodzie | Dokument i sekcja |
|---|---|---|---|
| Potok renderowania, etapy programowalne, GLSL | brak kodu, sama teoria | brak | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.1 do 2.4 |
| RAII i przenoszenie dla obiektów OpenGL | [`src/gfx/Shader.hpp`](../src/gfx/Shader.hpp), [`Shader.cpp`](../src/gfx/Shader.cpp) | destruktor, `= delete`, konstruktor przenoszący, przypisanie przenoszące | [`gfx/README.md`](modules/gfx/README.md), 2, oraz [`gfx/shaders.md`](modules/gfx/shaders.md), 5.9 |
| Wczytanie pliku shadera | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `readTextFile`, `pathText` | [`gfx/shaders.md`](modules/gfx/shaders.md), 5.3 |
| Kompilacja, status i dziennik shadera | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `compileShader`, `shaderInfoLog` | [`gfx/shaders.md`](modules/gfx/shaders.md), 3.1, 3.3, 5.4 i 5.5 |
| Linkowanie, status i dziennik programu | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `linkProgram`, `programInfoLog`, `buildProgram` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.5, 2.6, 5.6 i 5.7 |
| Wczytywanie na żywo z zachowaniem starego programu | [`src/gfx/Shader.cpp`](../src/gfx/Shader.cpp) | `Shader::reload`, `lastError`, `isValid` | [`gfx/shaders.md`](modules/gfx/shaders.md), 2.7 i 5.8 |
| Bufor wierzchołków i bufor indeksów, `glBufferData`, podpowiedź użycia | [`src/gfx/Buffer.cpp`](../src/gfx/Buffer.cpp), [`Buffer.hpp`](../src/gfx/Buffer.hpp) | konstruktor `Buffer::Buffer`, `bind`, pole `m_target` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.2, 2.6, 2.8 i 5.3 |
| Tablica wierzchołków: co pamięta VAO, dlaczego Core jej wymaga | [`src/gfx/VertexArray.cpp`](../src/gfx/VertexArray.cpp), [`VertexArray.hpp`](../src/gfx/VertexArray.hpp) | konstruktor, `bind` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.4, 2.5 i 5.5 |
| Atrybuty wierzchołka, krok i przesunięcie, przesunięcie jako wskaźnik | [`src/gfx/VertexArray.cpp`](../src/gfx/VertexArray.cpp) | `VertexArray::setFloatAttribute` | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.3, 4 i 5.6 |
| `glDrawArrays` a `glDrawElements`, NDC, kierunek nawijania | brak kodu, sama teoria | brak | [`gfx/buffers-vao.md`](modules/gfx/buffers-vao.md), 2.7 i 2.9 |

## Panele ImGui (narzędzie do wszystkich tematów)

| Zagadnienie | Dokument | Pliki kodu |
|---|---|---|
| Cykl życia ImGui, klatka ImGui, dockspace | [`modules/debug-ui.md`](modules/debug-ui.md) | [`src/debug/DebugUI.cpp`](../src/debug/DebugUI.cpp), [`DebugUI.hpp`](../src/debug/DebugUI.hpp) |
| Dane dla paneli: struktura `DebugContext` | [`modules/debug-ui.md`](modules/debug-ui.md) (sekcja 5.2) | [`src/debug/DebugContext.hpp`](../src/debug/DebugContext.hpp), [`src/main.cpp`](../src/main.cpp) |
| Panel Renderer | [`modules/debug-ui.md`](modules/debug-ui.md) | [`src/debug/panels/RendererPanel.cpp`](../src/debug/panels/RendererPanel.cpp), [`RendererPanel.hpp`](../src/debug/panels/RendererPanel.hpp) |
| Podpięcie nakładki do gry, klawisz `~`, blokada klawiatury i myszy gry | [`modules/debug-ui.md`](modules/debug-ui.md) (sekcje 5.4 i 5.6), [`modules/core/input.md`](modules/core/input.md) (sekcje 5.6 i 5.10) | [`src/main.cpp`](../src/main.cpp) |
| Biblioteka | [`libraries/imgui.md`](libraries/imgui.md) | [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake) |

## Kamienie milowe a tematy

| Kamień milowy | Zakres według PRD | Tematy wykładu |
|---|---|---|
| M0 (zrobione) | Repozytorium, CMake i FetchContent, okno GLFW 4.1, GLAD, ImGui, `GL_CHECK` | 1 |
| M1 (w toku) | `gfx`: Shader, Buffer, VAO. Trójkąt, potem kostka z MVP. Kamera FPS. Zrobione: mysz, ścieżki do assetów, GLM, klasy `Shader`, `Buffer`, `VertexArray` | 2, 3 |
| M2 + M3 | Generator labiryntu, kolizje AABB, tekstury, loader OBJ, pierwsze modele | 4, 5, 14 |
| M4 + M5 | Księżyc, latarka, kryształy, Gouraud vs Phong, zbieranie, bateria, brama | 6, 7 |
| M6 | Skybox, teren z heightmapy, trawa w shaderze geometrii | 8, 9, 13 |
| M7 | HDR FBO, bloom, mgła, minimapa, shadow mapping latarki i księżyca | 10, 11 |
| M8 | Environment mapping, selekcja obiektów, dźwignie i notatki | 12, 15 |
| M9 | Szlif, menu, balans, przygotowanie do code review | powtórka wszystkich |
