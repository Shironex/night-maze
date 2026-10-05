# Struktura projektu (stan: M1 kompletny, M2 + M3 w toku)

Kompletna mapa repozytorium Night Maze: co leży w którym katalogu, do czego służy każdy plik
konfiguracyjny i co powstaje dopiero podczas budowania. Dokument opisuje stan faktyczny po
ostatnim kroku kamienia milowego M1 (kod kompletny, zbudowany i uruchomiony na macOS i na
Windowsie, ręczne sprawdzenie sterowania na Windowsie jeszcze otwarte, bez tagu): po M0 doszły mysz, ścieżki do assetów, GLM i warstwa `gfx/` z
klasami `Shader`, `Buffer` i `VertexArray`, katalog `assets/` z pierwszymi shaderami,
warstwa `scene/` ze strukturami `Transform` i `Camera`, kostka rysowana z macierzami
modelu, widoku i rzutowania, latająca kamera sterowana myszą i klawiaturą oraz panel
Camera. Z kamienia milowego M2 + M3 jest już kod bez okna: kolizje (`src/scene/Collider.*`),
labirynt z generatorem i układem w świecie (`src/game/Maze*`), biblioteka `game_logic`,
katalog `tests/` z programem testowym `night_maze_tests` i biblioteka doctest. Ta część jest
zbudowana i przetestowana na Windowsie, na macOS jeszcze nie. Doszły też loader modeli OBJ
(`src/assets/ObjLoader.*`) z testami oraz wierzchołek i siatka (`src/gfx/Vertex.hpp`,
`src/gfx/Mesh.*`), których program jeszcze nie używa. Z tematu 5 doszły loader obrazów
(`src/assets/ImageLoader.*`) z testami, biblioteka stb_image (`external/stb/` i blok w
`cmake/Dependencies.cmake`) oraz klasa tekstury (`src/gfx/Texture2D.*`), też jeszcze bez
użytkownika w programie. Docelową
strukturę (z `renderer/` i pełną warstwą `src/assets/`) opisuje PRD w sekcji 6.

Polecenia budowania są w [`build-macos.md`](build-macos.md) i
[`build-windows.md`](build-windows.md), tutaj ich nie powtarzamy.

## 1. Drzewo katalogów

Stan z systemu plików, bez `build/` i `.git/`:

```text
night-maze/
├── CMakeLists.txt              # główny opis buildu: engine, game_logic, night_maze, testy
├── CMakePresets.json           # presety debug i release
├── Makefile                    # skróty do codziennych poleceń: make run, make test, make check
├── .clang-format               # styl formatowania kodu
├── .clang-tidy                 # reguły analizy statycznej i konwencja nazw
├── .clangd                     # gdzie clangd ma szukać compile_commands.json
├── .gitattributes              # normalizacja końców linii
├── .gitignore                  # czego nie wersjonujemy
├── .vscode/                    # ustawienia obszaru roboczego Cursor i VS Code
│   ├── extensions.json         # rekomendowane rozszerzenia
│   └── settings.json           # clangd, presety CMake, formatowanie przy zapisie, GLSL
├── assets/                     # pliki wczytywane przez program w czasie działania
│   └── shaders/                # shadery GLSL
│       ├── basic.frag              # shader fragmentów: kolor z interpolacji
│       └── basic.vert              # shader wierzchołków: trzy macierze, pozycja i kolor
├── cmake/
│   └── Dependencies.cmake      # FetchContent: GLFW, GLM, Dear ImGui, doctest i stb, targety imgui i stb_image
├── external/
│   ├── glad/                   # wygenerowany loader OpenGL 4.1 Core (kod w repozytorium)
│   │   ├── CMakeLists.txt      # target glad (napisany ręcznie)
│   │   ├── README.md           # jak wygenerować ponownie
│   │   ├── include/
│   │   │   ├── KHR/khrplatform.h   # typy zależne od platformy
│   │   │   └── glad/gl.h           # deklaracje API OpenGL
│   │   └── src/gl.c            # loader wypełniający wskaźniki funkcji
│   └── stb/                    # stb_image: w repozytorium tylko plik z implementacją
│       ├── README.md           # skąd jest nagłówek i jak zmienić wersję
│       └── stb_image.c         # dwie linie: makro i #include pobranego nagłówka
├── src/
│   ├── main.cpp                # punkt wejścia, łączy game/ z debug/
│   ├── assets/                 # wczytywanie plików z assets/ do danych procesora, bez OpenGL
│   │   ├── ImageLoader.hpp/.cpp    # plik obrazu na piksele, dolny wiersz pierwszy
│   │   └── ObjLoader.hpp/.cpp      # parser OBJ i MTL: wierzchołki, indeksy, części, materiały
│   ├── core/                   # warstwa bazowa: okno, wejście, czas, logi, ścieżki, GL_CHECK
│   │   ├── Application.hpp/.cpp    # klasa bazowa programu, pętla główna
│   │   ├── GlCheck.hpp/.cpp        # makro GL_CHECK
│   │   ├── Input.hpp/.cpp          # stan klawiatury i myszy, blokady, kursor
│   │   ├── Log.hpp/.cpp            # logowanie do konsoli
│   │   ├── Paths.hpp/.cpp          # ścieżki do assetów względem programu
│   │   ├── Time.hpp/.cpp           # zegar klatki, stały krok symulacji, FPS
│   │   └── Window.hpp/.cpp         # okno GLFW i kontekst OpenGL (RAII)
│   ├── debug/                  # interfejs debugowy (Dear ImGui)
│   │   ├── DebugContext.hpp        # referencje do danych dla paneli
│   │   ├── DebugUI.hpp/.cpp        # kontekst ImGui i cykl klatki
│   │   └── panels/
│   │       ├── CameraPanel.hpp/.cpp    # panel "Camera": pozycja, kąty, FOV, sterowanie
│   │       ├── RendererPanel.hpp/.cpp  # panel "Renderer"
│   │       └── ShadersPanel.hpp/.cpp   # panel "Shaders", przycisk Reload shaders
│   ├── game/                   # gra
│   │   ├── Maze.hpp/.cpp           # labirynt: siatka komórek, ściany na krawędziach
│   │   ├── MazeGenerator.hpp/.cpp  # generator labiryntu z ziarna, randomBelow
│   │   ├── MazeLayout.hpp/.cpp     # układ w świecie: ściany, słupki, pudełka kolizji
│   │   └── NightMazeApp.hpp/.cpp   # aplikacja Night Maze (na razie rysuje kostkę)
│   ├── gfx/                    # opakowania obiektów OpenGL (RAII, tylko przenoszenie)
│   │   ├── Buffer.hpp/.cpp         # bufor wierzchołków albo indeksów
│   │   ├── Mesh.hpp/.cpp           # siatka: VAO i dwa bufory jednego modelu, draw
│   │   ├── Shader.hpp/.cpp         # program shaderów z dwóch plików, reload, setMat4, setInt, setVec3
│   │   ├── Texture2D.hpp/.cpp      # tekstura 2D z mipmapami i obiekt samplera
│   │   ├── Vertex.hpp              # jeden wierzchołek modelu: pozycja, normalna, uv
│   │   └── VertexArray.hpp/.cpp    # tablica wierzchołków (VAO), opis atrybutów
│   └── scene/                  # opis sceny: dane i matematyka na GLM, bez OpenGL
│       ├── Camera.hpp/.cpp         # kamera: kierunek, macierz widoku i rzutowania
│       ├── Collider.hpp/.cpp       # pudełka AABB, test nakładania, ruch ze ślizganiem
│       └── Transform.hpp/.cpp      # pozycja, obrót, skala i macierz modelu
├── tests/                      # testy jednostkowe (doctest): program night_maze_tests
│   ├── main.cpp                    # punkt wejścia: main() generuje doctest
│   ├── ColliderTests.cpp           # testy scene::Aabb, overlaps i moveAndSlide
│   ├── ImageLoaderTests.cpp        # testy loadImage: tekstury gry, odwracanie wierszy, błędy
│   ├── MazeGeneratorTests.cpp      # testy randomBelow i generateMaze, labirynt wzorcowy
│   ├── MazeLayoutTests.cpp         # testy układu w świecie i kolizji w labiryncie
│   ├── MazeTests.cpp               # testy klasy Maze i kierunków
│   └── ObjLoaderTests.cpp          # testy parseObj, parseMtl i loadObj
└── docs/
    ├── PRD.pdf                 # dokument wymagań
    ├── README.md               # spis treści dokumentacji i kolejność czytania
    ├── syllabus.md             # tabela: temat wykładu, dokument, pliki kodu
    ├── decisions/              # notatki "dlaczego tak, a nie inaczej"
    │   ├── README.md               # czym jest notatka, układ, lista notatek
    │   ├── collision-aabb-sliding.md   # AABB i ślizganie zamiast silnika fizyki
    │   └── deterministic-random.md     # własna randomBelow zamiast rozkładów std
    ├── guides/                 # przewodniki
    │   ├── build-macos.md          # budowanie na macOS
    │   ├── build-windows.md        # budowanie na Windowsie
    │   └── project-structure.md    # ten dokument
    ├── libraries/              # dokumenty bibliotek
    │   ├── doctest.md
    │   ├── glad.md
    │   ├── glfw.md
    │   ├── glm.md
    │   ├── imgui.md
    │   └── stb_image.md
    └── modules/                # dokumenty modułów
        ├── assets/                 # moduł assets: wczytywanie plików
        │   ├── README.md               # wstęp, dane procesora bez OpenGL, warstwy, indeks
        │   ├── images.md               # loader obrazów, odwracanie wierszy, testy
        │   └── obj-loader.md           # format OBJ i MTL, mapa trójek, parser
        ├── core/                   # moduł core, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, warstwy, klatka jako całość, indeks
        │   ├── gl-check.md             # GL_CHECK i błędy OpenGL
        │   ├── input.md                # klawiatura i jej blokada
        │   ├── main-loop.md            # pętla główna, stały krok, FPS
        │   ├── paths.md                # ścieżki do assetów, katalog programu
        │   └── window-context.md       # okno, kontekst, GLAD, vsync, Log
        ├── game/                   # moduł game
        │   ├── README.md               # wstęp, aplikacja a logika bez okna, indeks
        │   └── maze-generator.md       # labirynt, generator, układ w świecie
        ├── gfx/                    # moduł gfx, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, RAII i przenoszenie, warstwy, indeks
        │   ├── buffers-vao.md          # VBO, VAO, krok i przesunięcie, Buffer, VertexArray
        │   ├── indexed-drawing.md      # EBO, glDrawElements, dane kostki
        │   ├── mesh.md                 # Vertex, Mesh, rysowanie zakresu indeksów
        │   ├── shader-class.md         # klasa Shader: kompilacja, linkowanie, błędy
        │   ├── shader-hot-reload.md    # reload, panel Shaders
        │   ├── shaders.md              # potok, GLSL, basic.vert i basic.frag
        │   ├── textures.md             # tekstury, filtry, mipmapy, samplery, Texture2D
        │   └── uniforms.md             # uniformy, setMat4, setInt, setVec3
        ├── scene/                  # moduł scene, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, dane bez OpenGL, warstwy, indeks
        │   ├── camera-controls.md      # sterowanie kamerą, panel Camera
        │   ├── camera.md               # macierz widoku, rzutowanie, Camera
        │   ├── collision.md            # AABB, test nakładania, ruch ze ślizganiem
        │   └── transforms.md           # przestrzenie, macierz modelu, Transform
        └── debug-ui.md             # panele ImGui w projekcie
```

Drzewo nie pokazuje jeszcze katalogów `tools/blender/`, `assets/models/` i `assets/textures/`
ani pliku `docs/guides/blender.md` (modele ścian, słupka i podłogi oraz skrypty, które je
budują). Opisuje je [`blender.md`](blender.md).

Zapis `Window.hpp/.cpp` oznacza parę plików `Window.hpp` i `Window.cpp`. Zgodnie z zasadą z
PRD nagłówek i implementacja leżą obok siebie w `src/`, nie ma osobnego katalogu `include/`. Pliki konfiguracyjne z katalogu głównego są
wypisane na początku drzewa, przed katalogami.

### Katalogi

| Katalog | Rola | Kto pisze kod |
|---|---|---|
| `src/` | cały nasz kod C++ poza testami | my |
| `tests/` | testy jednostkowe: osobny program `night_maze_tests`, który woła kod z bibliotek `engine` i `game_logic` i sprawdza wyniki ([`../libraries/doctest.md`](../libraries/doctest.md)) | my |
| `assets/` | pliki, które program wczytuje w czasie działania: dziś shadery GLSL, później modele i tekstury. Nie są kompilowane razem z programem. Krok budowania umieszcza katalog obok pliku wykonywalnego (sekcja 3.1, blok 7) | my |
| `cmake/` | pomocnicze pliki CMake dołączane przez `include(...)` | my |
| `external/` | cudzy kod trzymany w repozytorium (`glad/`) oraz plik, który kompiluje pobraną bibliotekę stb_image (`stb/`) | `glad/`: generator GLAD, nie edytujemy. `stb/`: dwa małe pliki napisane ręcznie |
| `docs/` | dokumentacja do nauki | my |
| `build/` | wszystko, co powstaje podczas budowania | CMake i kompilator, poza Gitem |

### Pliki źródłowe

| Plik | Co zawiera | Dokument |
|---|---|---|
| `src/main.cpp` | `main()` z obsługą wyjątków oraz klasę `DebugNightMazeApp`, która dokłada interfejs debugowy do gry | [`../modules/debug-ui.md`](../modules/debug-ui.md) |
| `src/core/Application.*` | `core::Application`: posiada `Window`, `Input`, `Time`, prowadzi pętlę główną, obsługuje Esc (zwolnienie przechwyconego kursora albo zamknięcie programu) | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Window.*` | `core::Window`: inicjalizacja GLFW, okno, kontekst 4.1 Core, `gladLoadGL`, vsync | [`../modules/core/window-context.md`](../modules/core/window-context.md), [`../libraries/glfw.md`](../libraries/glfw.md) |
| `src/core/Input.*` | `core::Input`: odpytywanie klawiszy i myszy. Klawiatura: `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`. Mysz: `isMouseButtonDown`, `wasMouseButtonPressed`, `mouseDeltaX`, `mouseDeltaY`, `setMouseBlocked`, `setCursorCaptured`, `isCursorCaptured` | [`../modules/core/input.md`](../modules/core/input.md) |
| `src/core/Time.*` | `core::Time`: delta czasu, akumulator stałego kroku (`FIXED_DT`), uśrednione FPS | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Log.*` | `logInfo`, `logWarn`, `logError` | [`../modules/core/window-context.md`](../modules/core/window-context.md) |
| `src/core/GlCheck.*` | makro `GL_CHECK` i funkcja `checkGlErrors` | [`../modules/core/gl-check.md`](../modules/core/gl-check.md) |
| `src/core/Paths.*` | `core::executableDir` i `core::assetPath`: ścieżki do plików z `assets/` liczone od położenia pliku wykonywalnego. `core::pathText`: ścieżka jako tekst UTF-8 do logu i do paneli. `Paths.cpp` to jedyny plik w `src/` z kodem zależnym od systemu (`#if` dla macOS i Windows). Woła je konstruktor `game::NightMazeApp` przy wczytywaniu shaderów | [`../modules/core/paths.md`](../modules/core/paths.md) |
| `src/gfx/Shader.*` | `gfx::Shader`: obiekt programu OpenGL zbudowany z pliku shadera wierzchołków i pliku shadera fragmentów. `reload` (przy błędzie zostaje stary program), `isValid`, `use`, `setMat4` (uniform typu `mat4`, przez `glGetUniformLocation` i `glUniformMatrix4fv`), `setInt` (uniform typu `int` albo sampler, `glUniform1i`), `setVec3` (uniform typu `vec3`, `glUniform3fv`), `lastError`, `vertexPath`, `fragmentPath`. RAII, tylko przenoszenie. Używa jej `NightMazeApp`, a panel "Shaders" woła `reload`. `setInt` i `setVec3` nie mają jeszcze użytkownika | [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), `setMat4`, `setInt` i `setVec3` w [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), `reload` w [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), wstęp do warstwy w [`../modules/gfx/README.md`](../modules/gfx/README.md) |
| `src/gfx/Buffer.*` | `gfx::Buffer`: jeden bufor OpenGL wypełniany raz w konstruktorze (`glGenBuffers`, `glBindBuffer`, `glBufferData` z `GL_STATIC_DRAW`), cel `GL_ARRAY_BUFFER` albo `GL_ELEMENT_ARRAY_BUFFER`, `bind`. RAII, tylko przenoszenie. Używa jej `NightMazeApp` | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/VertexArray.*` | `gfx::VertexArray`: jeden obiekt tablicy wierzchołków (VAO), wiązany już w konstruktorze, `bind`, `setFloatAttribute` (`glEnableVertexAttribArray`, `glVertexAttribPointer`). RAII, tylko przenoszenie. Używa jej `NightMazeApp` | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/Texture2D.*` | typ `gfx::TextureFilter` (`Nearest`, `Bilinear`, `Trilinear`) i klasa `gfx::Texture2D`: jedna tekstura 2D z pełnym łańcuchem mipmap i jej obiekt samplera. Konstruktor przyjmuje surowe bajty (szerokość, wysokość, 3 albo 4 kanały, wskaźnik, dolny wiersz pierwszy) i woła `glTexImage2D` oraz `glGenerateMipmap`. `bind(unit)` wiąże teksturę i sampler z jednostką teksturującą, `setFilter` i `setAnisotropy` zmieniają próbkowanie w działającym programie, akcesory `filter`, `anisotropy`, `maxAnisotropy`, `id`, `width`, `height`, `isValid`. Filtrowanie anizotropowe jest wykrywane jako rozszerzenie. RAII, tylko przenoszenie. Nie ma jeszcze użytkownika ani testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/textures.md`](../modules/gfx/textures.md) |
| `src/gfx/Vertex.hpp` | `gfx::Vertex`: jeden wierzchołek modelu jako struktura (pola `position`, `normal`, `uv`: 8 liczb `float`, 32 bajty), stałe `POSITION_COMPONENTS`, `NORMAL_COMPONENTS`, `UV_COMPONENTS` i numery atrybutów `POSITION_ATTRIBUTE` (0), `NORMAL_ATTRIBUTE` (1), `UV_ATTRIBUTE` (2), dwa `static_assert` (rozmiar bez dopełnienia, układ standardowy). Sam nagłówek, bez GLAD. Używają jej `gfx::Mesh`, loader OBJ i testy | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcja 5.2 |
| `src/gfx/Mesh.*` | `gfx::Mesh`: siatka jednego modelu na karcie. Posiada `VertexArray`, bufor wierzchołków i bufor indeksów, w konstruktorze wysyła dane ze `std::span` i opisuje trzy atrybuty przez `sizeof(Vertex)` i `offsetof`. `draw()` rysuje całość, `draw(firstIndex, indexCount)` zakres indeksów (`glDrawElements`), prymityw jest parametrem konstruktora (domyślnie `GL_TRIANGLES`). RAII przez pola, tylko przenoszenie. W programie nikt jej jeszcze nie tworzy | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcje od 5.3 do 5.5 |
| `src/assets/ObjLoader.*` | struktury `assets::ObjPart`, `assets::ObjMaterial`, `assets::ObjModel` i funkcje `assets::parseObj` (tekst OBJ na wierzchołki, indeksy i części), `assets::parseMtl` (tekst MTL na materiały) oraz `assets::loadObj` (plik OBJ razem z plikami MTL, ścieżki tekstur względem pliku MTL). Ręcznie napisany parser, bez OpenGL i bez wyjątków: wynik `bool` i tekst błędu z numerem linii. W programie nikt jeszcze nie woła `loadObj` | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), wstęp do warstwy w [`../modules/assets/README.md`](../modules/assets/README.md) |
| `src/assets/ImageLoader.*` | struktura `assets::Image` (szerokość, wysokość, liczba kanałów, bajty pikseli z dolnym wierszem jako pierwszym) i funkcja `assets::loadImage`: czyta plik w trybie binarnym, dekoduje go biblioteką stb_image i odwraca kolejność wierszy. Wynik `bool`, tekst błędu przez referencję, jedno logowanie, bez wyjątków. Bez OpenGL. Jedyny plik projektu, który dołącza `<stb_image.h>`. W programie nikt jej jeszcze nie woła | [`../modules/assets/images.md`](../modules/assets/images.md) |
| `external/stb/stb_image.c` | jedyny plik, w którym kompiluje się implementacja stb_image: makro `STB_IMAGE_IMPLEMENTATION` i dołączenie nagłówka pobranego przez FetchContent. Plik C, poza naszymi ostrzeżeniami, tworzy bibliotekę `stb_image` | [`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2 |
| `src/scene/Transform.*` | `scene::Transform`: struktura z publicznymi polami `position`, `rotationDegrees` (kąty Eulera w stopniach) i `scale` oraz funkcją `matrix()`, która zwraca macierz modelu `T * Ry * Rx * Rz * S`. Sama matematyka na GLM, bez OpenGL. Używa jej `NightMazeApp` (pole `m_cubeTransform`) | [`../modules/scene/transforms.md`](../modules/scene/transforms.md), wstęp do warstwy w [`../modules/scene/README.md`](../modules/scene/README.md) |
| `src/scene/Camera.*` | `scene::Camera`: struktura z publicznymi polami `position`, `yawDegrees`, `pitchDegrees`, `fovDegrees`, `nearPlane`, `farPlane`, stałymi `WORLD_UP` i `MAX_PITCH_DEGREES` oraz funkcjami `forward`, `right`, `rotate`, `viewMatrix`, `projectionMatrix`. Sama matematyka na GLM, bez OpenGL i bez wejścia. Używa jej `NightMazeApp` (pole `m_camera`) | [`../modules/scene/camera.md`](../modules/scene/camera.md) |
| `src/scene/Collider.*` | `scene::Aabb` (pudełko o ścianach równoległych do osi: pola `min` i `max`, funkcja `fromCenter`), stała `CONTACT_TOLERANCE`, funkcje `scene::overlaps` (czy dwa pudełka na siebie nachodzą) i `scene::moveAndSlide` (o ile wolno przesunąć pudełko wśród przeszkód, oś po osi, ze ślizganiem po ścianach). Sama matematyka na GLM, bez OpenGL i bez wejścia. Używają jej `game/MazeLayout` i testy. W programie nikt jeszcze nie woła `moveAndSlide` | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `src/game/Maze.*` | typ `game::Direction` (North, East, South, West), stałe `DIRECTION_COUNT` i `ALL_DIRECTIONS`, funkcje `opposite`, `columnStep`, `rowStep`, klasa `game::Maze`: siatka komórek ze ścianami na krawędziach (`width`, `height`, `contains`, `hasWall`, `removeWall`, stała `MAX_SIZE`). Bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 5.2 i 5.3 |
| `src/game/MazeGenerator.*` | `game::randomBelow` (losowa liczba poniżej granicy, taka sama na każdym systemie) i `game::generateMaze` (labirynt doskonały z rozmiaru i ziarna, algorytm recursive backtracker z własnym stosem). Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 5.4 i 5.5 |
| `src/game/MazeLayout.*` | stałe wymiarów w metrach (`CELL_SIZE`, `WALL_LENGTH`, `WALL_HEIGHT`, `WALL_THICKNESS`, `PILLAR_SIZE`, `PILLAR_HEIGHT`), typy `WallAxis` i `WallSegment`, funkcje `cellCenter`, `wallSegments`, `pillarPositions`, `wallBox`, `pillarBox`, `mazeColliders`. Część biblioteki `game_logic`. Program jeszcze ich nie woła | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 5.6 i 5.7 |
| `src/game/NightMazeApp.*` | `game::NightMazeApp`: kolor tła, shader `basic`, dane kostki (24 wierzchołki ze stałymi układu, 36 indeksów), tablica wierzchołków, bufor wierzchołków i bufor indeksów, `scene::Transform` kostki i `scene::Camera`, ustawienia sterowania kamerą (czułość myszy, prędkość ruchu). Konstruktor wczytuje shader, wysyła dane i ustawia obrót kostki. `onUpdate` przesuwa kamerę klawiszami W, A, S, D, spacja i lewy Shift stałym krokiem. `onRender` przechwytuje kursor po kliknięciu w scenę i obraca kamerę myszą, ustawia viewport, włącza test głębi, czyści kolor i głębię, liczy proporcje z rozmiaru framebuffera i pozycję oka między dwoma krokami symulacji, wysyła macierze modelu, widoku i rzutowania i rysuje kostkę (`glDrawElements`). Chronione akcesory `clearColor()`, `shader()`, `camera()`, `mouseSensitivity()` i `moveSpeed()` udostępniają stan panelom debug | [`../modules/core/README.md`](../modules/core/README.md), rysowanie w [`../modules/gfx/README.md`](../modules/gfx/README.md), sekcja 6, dane kostki w [`../modules/gfx/indexed-drawing.md`](../modules/gfx/indexed-drawing.md), sekcja 5, macierze w [`../modules/scene/camera.md`](../modules/scene/camera.md), sekcja 5.7, sterowanie kamerą w [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcja 5 |
| `assets/shaders/basic.vert`, `basic.frag` | para shaderów GLSL `#version 410 core`: atrybuty pozycji i koloru, uniformy `uModel`, `uView`, `uProjection` (macierze), kolor interpolowany między wierzchołkami. To nie są pliki C++: nie są na żadnej liście w `CMakeLists.txt`, program czyta je przy starcie i po naciśnięciu "Reload shaders" | [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 4 |
| `src/debug/DebugContext.hpp` | `debug::DebugContext`: struktura referencji do danych, które panele czytają albo edytują (`time`, `window`, `clearColor`, `shader`, `camera`, `mouseSensitivity`, `moveSpeed`). Sam nagłówek | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.2 |
| `src/debug/DebugUI.*` | `debug::DebugUI`: inicjalizacja i zamknięcie ImGui, `draw`, `wantsKeyboard`, `wantsMouse`, `setMouseEnabled` (ImGui ignoruje mysz, gdy kursor jest przechwycony) | [`../modules/debug-ui.md`](../modules/debug-ui.md), [`../libraries/imgui.md`](../libraries/imgui.md) |
| `src/debug/panels/CameraPanel.*` | `debug::drawCameraPanel`: panel "Camera" (pozycja, yaw, pitch, FOV, bliska i daleka płaszczyzna, czułość myszy, prędkość ruchu) | [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcja 6 |
| `src/debug/panels/RendererPanel.*` | `debug::drawRendererPanel`: panel "Renderer" | [`../modules/debug-ui.md`](../modules/debug-ui.md) |
| `src/debug/panels/ShadersPanel.*` | `debug::drawShadersPanel`: panel "Shaders" (pliki programu shaderów, przycisk "Reload shaders", ostatni błąd wczytania) | [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6 |
| `tests/main.cpp` | punkt wejścia programu testowego: makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` i dołączenie nagłówka doctest, który generuje `main()` | [`../libraries/doctest.md`](../libraries/doctest.md), sekcja 3.1 |
| `tests/ColliderTests.cpp` | 12 przypadków testowych `scene::Aabb`, `overlaps` i `moveAndSlide` | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 5.7 |
| `tests/ImageLoaderTests.cpp` | 7 przypadków testowych loadera obrazów: obie tekstury gry (512 x 512, 3 kanały), odwracanie wierszy na obrazku 2 x 3 zapisanym przez test, ścieżka ze znakami spoza ASCII, brak pliku, plik niebędący obrazem, pusty plik | [`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7 |
| `tests/MazeTests.cpp`, `MazeGeneratorTests.cpp`, `MazeLayoutTests.cpp` | 29 przypadków testowych labiryntu: klasa `Maze`, generator (w tym labirynt wzorcowy 4 na 4 z ziarna 1), układ w świecie i jego współpraca z kolizjami | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5.8 |
| `tests/ObjLoaderTests.cpp` | 18 przypadków testowych loadera OBJ: reguły formatu na napisach wpisanych w kod, przypadki błędów z numerem linii i trzy prawdziwe modele z `assets/models/` | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), sekcja 5.9 |

Każdy plik źródłowy zaczyna się komentarzem z jednym zdaniem opisu i odnośnikiem
`See docs/modules/...`. To wymaganie z PRD (sekcja 7). Dotyczy też plików shaderów, w
których komentarz stoi pod linią `#version`, bo ta musi być pierwsza. Odnośnik wskazuje najbardziej
szczegółowy dokument, czyli ten z kolumny "Dokument" powyżej, na przykład
`// See docs/modules/core/input.md` w `Input.hpp`. Pliki testów mają taki sam nagłówek i
wskazują dokument kodu, który sprawdzają.

## 2. Warstwy i targety

### Reguła warstw

Zależności są jednokierunkowe. Niższa warstwa nigdy nie wie o wyższej.

```text
main.cpp   łączy game/ i debug/
   │
   ├── debug/   zależy od core/ i gfx/ (i od Dear ImGui), nic nie zależy od debug/
   │
   └── game/    zależy od scene/, gfx/ i core/, nie zna debug/
          │
        scene/  może zależeć od gfx/ i core/, dziś dołącza tylko GLM. Nie zna game/ ani debug/
          │
        gfx/    zależy od core/ (i od GLM), nie zna scene/, game/ ani debug/
          │
        core/   nie zna ani gfx/, ani scene/, ani game/, ani debug/
```

Pełny łańcuch z PRD to `core <- gfx <- renderer <- scene <- game`. Warstwy `renderer/`
jeszcze nie ma, więc dziś łańcuch to `core <- gfx <- scene <- game`, a `game/` korzysta z
`gfx/` bezpośrednio: samo wysyła macierze do shadera i samo woła `glDrawElements`.

Od kamienia milowego M2 + M3 dochodzą dwie rzeczy, których rysunek nie pokazuje. Katalog
`game/` ma część bez okna (`Maze`, `MazeGenerator`, `MazeLayout`), która zależy tylko od
`scene/` i GLM. Katalog `tests/` stoi na samej górze, obok `main.cpp`: zależy od tej części
`game/` i od `scene/`, a od niego nie zależy nic.

Doszła też warstwa `assets/` (wczytywanie plików), której rysunek również nie pokazuje.
Stoi nad `gfx/` i `core/`: dołącza `gfx/Vertex.hpp`, `core/Log.hpp` i `core/Paths.hpp`, a
OpenGL nie woła. Z `src/` nikt jej jeszcze nie dołącza: jedynym użytkownikiem są testy.
Opis: [`../modules/assets/README.md`](../modules/assets/README.md), sekcja 4.

Jak to widać w kodzie:

- `src/core/` dołącza tylko własne nagłówki, GLAD, GLFW i bibliotekę standardową. Jedynym
  wyjątkiem jest `Paths.cpp`, który dołącza nagłówek systemowy (`<mach-o/dyld.h>` na macOS,
  `<windows.h>` na Windowsie), bo położenie programu zna tylko system operacyjny. Także
  blokada klawiatury i myszy na czas pracy z panelem jest zrobiona bez ImGui w `core/`:
  `core::Input` ma neutralne flagi `setKeyboardBlocked` i `setMouseBlocked`, a ustawia je
  `main.cpp`. W drugą stronę działa to tak samo: `core::Input` wie tylko, czy kursor jest
  przechwycony, a o tym, że ImGui ma wtedy ignorować mysz, decyduje `main.cpp`.
- Nagłówki w `src/gfx/` dołączają `<glad/gl.h>` i bibliotekę standardową, a pliki `.cpp` do
  tego `core/GlCheck.hpp` (`Shader.cpp` także `core/Log.hpp` i `core/Paths.hpp`). `Shader.hpp`
  dołącza też `<glm/glm.hpp>`, a `Shader.cpp` `<glm/gtc/type_ptr.hpp>`, bo `setMat4`
  przyjmuje `glm::mat4`. Nic z GLFW, `scene/`, `game/` ani `debug/`.
- Nagłówki w `src/scene/` dołączają tylko `<glm/glm.hpp>` (a `Collider.hpp` do tego `<span>`
  z biblioteki standardowej). Pliki `Transform.cpp` i `Camera.cpp` dołączają też
  `<glm/gtc/matrix_transform.hpp>`. Z biblioteki standardowej: `Camera.cpp` bierze
  `<algorithm>` i `<cmath>`, a `Collider.cpp` `<algorithm>`, `<array>` i `<cmath>`. Nic z GLAD,
  GLFW, `core/`, `gfx/`, `game/` ani `debug/`.
- Dwa nowe nagłówki `src/gfx/` różnią się od reszty. `Vertex.hpp` **nie dołącza GLAD**: tylko
  `<glm/glm.hpp>`, `<cstdint>` i `<type_traits>`, żeby mogły go używać loader i testy.
  `Mesh.hpp` dołącza `gfx/Buffer.hpp`, `gfx/Vertex.hpp`, `gfx/VertexArray.hpp`,
  `<glad/gl.h>`, `<cstdint>` i `<span>`, a `Mesh.cpp` do tego `core/GlCheck.hpp`.
- `src/gfx/Texture2D.hpp` dołącza tylko `<glad/gl.h>`, a `Texture2D.cpp` do tego
  `core/GlCheck.hpp`, `core/Log.hpp` i bibliotekę standardową. Nie dołącza niczego z
  `assets/`: teksturę tworzy się z surowych bajtów, a nie z `assets::Image`.
- `src/assets/ImageLoader.hpp` dołącza tylko bibliotekę standardową, a `ImageLoader.cpp` do
  tego `core/Log.hpp`, `core/Paths.hpp` i `<stb_image.h>`. To jedyny plik w `src/`, który
  zna stb_image. Nic z GLAD, GLFW, GLM, `gfx/`, `scene/`, `game/` ani `debug/`.
- `src/assets/ObjLoader.hpp` dołącza `gfx/Vertex.hpp`, `<glm/glm.hpp>` i bibliotekę
  standardową, a `ObjLoader.cpp` do tego `core/Log.hpp` i `core/Paths.hpp`. Nic z GLAD,
  GLFW, `scene/`, `game/` ani `debug/`: dlatego loader daje się testować bez okna.
- Pliki logiki labiryntu w `src/game/` (`Maze.*`, `MazeGenerator.*`, `MazeLayout.*`) dołączają
  bibliotekę standardową, a `MazeLayout.hpp` także `<glm/glm.hpp>`, `game/Maze.hpp` i
  `scene/Collider.hpp`. Nic z GLAD, GLFW, `core/`, `gfx/` ani `debug/`: dlatego dają się
  testować bez okna.
- Pliki w `tests/` dołączają `<doctest/doctest.h>`, nagłówki testowanego kodu
  (`scene/Collider.hpp`, `game/Maze.hpp`, `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`, `assets/ObjLoader.hpp`, `assets/ImageLoader.hpp`) i
  bibliotekę standardową. Żaden plik w `src/` nie dołącza niczego z `tests/` ani nagłówka
  doctest.
- `src/game/NightMazeApp.hpp` dołącza `core/Application.hpp`, trzy nagłówki z `gfx/`
  (`Buffer.hpp`, `Shader.hpp`, `VertexArray.hpp`), dwa ze `scene/` (`Camera.hpp`,
  `Transform.hpp`), `<glm/glm.hpp>` (pole typu `glm::vec3`) i nic z `debug/`. Komentarz w
  klasie mówi wprost: "It knows nothing about the debug UI". `NightMazeApp.cpp` dołącza do
  tego `<GLFW/glfw3.h>`, ale tylko dla stałych klawiszy i przycisku myszy (`GLFW_KEY_W`,
  `GLFW_MOUSE_BUTTON_LEFT`): o stan wejścia pyta wyłącznie `core::Input`.
- `src/debug/DebugUI.cpp` dołącza `core/Window.hpp`, `debug/DebugContext.hpp`, nagłówki trzech
  paneli i nagłówki ImGui.
- `src/debug/panels/CameraPanel.cpp` dołącza `scene/Camera.hpp` i
  `<glm/gtc/type_ptr.hpp>`: to pierwszy plik w `debug/`, który zna `scene/`.
- `src/debug/panels/ShadersPanel.cpp` dołącza `core/Paths.hpp` i `gfx/Shader.hpp`: to
  pierwszy plik w `debug/`, który zna `gfx/`. Kierunek jest dozwolony, bo `debug/` może
  zależeć od każdej warstwy.
- `src/main.cpp` jest jedynym plikiem, który dołącza jednocześnie `game/NightMazeApp.hpp` i
  nagłówki z `debug/` (`debug/DebugContext.hpp`, `debug/DebugUI.hpp`). Definiuje klasę
  `DebugNightMazeApp final : public game::NightMazeApp`, która posiada
  `debug::DebugUI m_debugUI{window()}` i w `onRender` najpierw woła
  `game::NightMazeApp::onRender(alpha)`, potem obsługuje klawisz `~` (przełącznik paneli),
  wyłącza mysz w ImGui, gdy kursor jest przechwycony przez kamerę
  (`m_debugUI.setMouseEnabled`), buduje `debug::DebugContext`, rysuje panele i przekazuje do
  `core::Input` informację, czy ImGui używa klawiatury i myszy.

Po co ta dyscyplina: grę da się zbudować i zrozumieć bez paneli debugowych, a panele można
rozbudowywać bez dotykania logiki gry. W docelowej architekturze między `gfx/` a `scene/`
dojdzie warstwa `renderer/`, a `debug/` nadal będzie zależeć od wszystkich i nikt od niego.

### Targety: `engine`, `game_logic`, `night_maze` i `night_maze_tests`

| Target | Rodzaj | Pliki | Linkuje |
|---|---|---|---|
| `engine` | biblioteka statyczna | `src/assets/*`, `src/core/*`, `src/gfx/*`, `src/scene/*` | `glad`, `glfw`, `glm::glm-header-only` (`PUBLIC`), `stb_image` (`PRIVATE`) |
| `game_logic` | biblioteka statyczna | `src/game/Maze.*`, `src/game/MazeGenerator.*`, `src/game/MazeLayout.*` | `engine` (`PUBLIC`) |
| `night_maze` | program | `src/main.cpp`, `src/game/NightMazeApp.*`, `src/debug/*` | `engine`, `game_logic`, `imgui` (`PRIVATE`) |
| `night_maze_tests` | program | `tests/*.cpp` | `game_logic`, `doctest::doctest` (`PRIVATE`) |
| `glad` | biblioteka statyczna | `external/glad/src/gl.c` | nic |
| `glfw` | biblioteka statyczna | pobrana przez FetchContent | biblioteki systemowe |
| `glm-header-only` (alias `glm::glm-header-only`) | target `INTERFACE`: same nagłówki, nic się nie kompiluje | pobrany przez FetchContent | nic |
| `imgui` | biblioteka statyczna | pobrana przez FetchContent, lista plików w `Dependencies.cmake` | `glfw` |
| `doctest` (alias `doctest::doctest`) | target `INTERFACE`: jeden nagłówek, nic się nie kompiluje | pobrany przez FetchContent | nic |
| `stb_image` | biblioteka statyczna | `external/stb/stb_image.c`, nagłówek pobrany przez FetchContent. Target zdefiniowany w `Dependencies.cmake` | nic |

**Dlaczego `engine` jest osobną biblioteką.** Warstwy wielokrotnego użytku (teraz `core`,
`gfx`, `scene` i `assets`, później `renderer`) nie zawierają niczego specyficznego dla Night
Maze. Jako osobny target da się je bez zmian podłączyć do innego programu, w szczególności
do zadań laboratoryjnych z tego samego kursu: nowy plik `main.cpp`, własna klasa pochodna po
`core::Application`, `target_link_libraries(zadanie PRIVATE engine)` i okno z kontekstem
4.1 Core, pętlą, `GL_CHECK`, klasami `gfx` oraz strukturami `Transform` i `Camera` jest
gotowe. Granica targetu pilnuje też reguły warstw: gdyby plik z `core/`, `gfx/` albo `scene/`
spróbował dołączyć coś z `game/` albo ImGui, `engine` nie linkuje tych rzeczy i błąd
wyszedłby szybko. Granic między `core/`, `gfx/` i `scene/` target nie pilnuje, bo te warstwy
są w tej samej bibliotece: tu obowiązuje sama dyscyplina dyrektyw `#include`.

**Dlaczego `game_logic` jest osobną biblioteką.** Program testowy jest drugim, osobnym
programem i może dolinkować tylko kod z biblioteki: kod skompilowany wprost w programie
`night_maze` jest dla niego niedostępny. Dlatego ta część gry, która nie potrzebuje okna
(labirynt, generator, układ w świecie), jest biblioteką statyczną, którą linkują i gra, i
testy. `NightMazeApp` zostaje w programie, bo potrzebuje okna i kontekstu OpenGL, których
test nie ma. `game_logic` nie trafia do `engine`, bo `engine` ma nie zawierać niczego
specyficznego dla Night Maze. Więcej: [`../modules/game/README.md`](../modules/game/README.md),
sekcja 3.

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
- `include` wkleja zawartość `cmake/Dependencies.cmake`, który tworzy targety `glfw`,
  `glm-header-only`, `imgui`, `doctest` i `stb_image`.

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
ją tylko dla naszych czterech targetów: `engine`, `game_logic`, `night_maze` i
`night_maze_tests`. Cudzy kod (GLAD, GLFW, ImGui, stb_image) kompiluje się ze swoimi
domyślnymi ustawieniami, bo jego ostrzeżeń nie będziemy poprawiać. GLM i doctest nie mają
własnych plików do skompilowania (same nagłówki), więc ich ostrzeżenia wycisza wyłącznie
oznaczenie nagłówków jako systemowe ([`../libraries/glm.md`](../libraries/glm.md),
[`../libraries/doctest.md`](../libraries/doctest.md)).

- clang i GCC: `-Wall -Wextra` włączają szeroki zestaw ostrzeżeń, `-Wpedantic` ostrzega przed
  odstępstwami od standardu.
- MSVC: `/W4` i `/permissive-`, opis w [`build-windows.md`](build-windows.md).
- `PRIVATE`: flagi dotyczą tylko tego targetu, nie przenoszą się na targety, które go linkują.

**Blok 5: target `engine`**

```cmake
add_library(engine STATIC
    src/assets/ImageLoader.cpp
    src/assets/ImageLoader.hpp
    src/assets/ObjLoader.cpp
    src/assets/ObjLoader.hpp
    src/core/Application.cpp
    src/core/Application.hpp
    ...
    src/core/Window.cpp
    src/core/Window.hpp
    src/gfx/Buffer.cpp
    src/gfx/Buffer.hpp
    src/gfx/Mesh.cpp
    src/gfx/Mesh.hpp
    src/gfx/Shader.cpp
    src/gfx/Shader.hpp
    src/gfx/Texture2D.cpp
    src/gfx/Texture2D.hpp
    src/gfx/Vertex.hpp
    src/gfx/VertexArray.cpp
    src/gfx/VertexArray.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/Collider.cpp
    src/scene/Collider.hpp
    src/scene/Transform.cpp
    src/scene/Transform.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, scene/Transform.hpp,
# scene/Camera.hpp, scene/Collider.hpp) expose GLM types, so every target that includes
# them needs the GLM include path too.
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
# stb_image is PRIVATE: only assets/ImageLoader.cpp includes its header, no header of
# engine does, so the targets that use engine do not need its include path.
target_link_libraries(engine PRIVATE stb_image)
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
- `target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)`: `engine` używa
  GLAD, GLFW i GLM. GLM dołączają pliki z `src/scene/` (`Transform`, `Camera` i `Collider`) oraz
  `src/gfx/Shader.*` (macierz jako parametr `setMat4`). `glm::glm-header-only`
  to target `INTERFACE` (same nagłówki), więc "linkowanie" go oznacza tylko dodanie ścieżki
  nagłówków ([`../libraries/glm.md`](../libraries/glm.md), sekcja 2).
- `target_link_libraries(engine PRIVATE stb_image)`: druga linia linkowania, tym razem
  `PRIVATE`. Nagłówek `stb_image.h` dołącza tylko `assets/ImageLoader.cpp`, żaden nagłówek
  `engine` go nie pokazuje, więc targety korzystające z `engine` nie potrzebują jego ścieżki
  ([`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2).
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
automatycznie, linkując tylko `engine`. Z tego samego powodu `PUBLIC` jest GLM: nagłówki
warstw `gfx` i `scene` (`gfx/Shader.hpp`, `scene/Transform.hpp`, `scene/Camera.hpp`,
`scene/Collider.hpp`) pokazują typy `glm::vec3` i `glm::mat4` w swoim API.

Dwie definicje `PUBLIC`:

- **`GLFW_INCLUDE_NONE`**: zabrania nagłówkowi `GLFW/glfw3.h` dołączania systemowego nagłówka
  OpenGL. Deklaracje OpenGL mają pochodzić wyłącznie z GLAD. Szczegóły w
  [`../libraries/glfw.md`](../libraries/glfw.md) i [`../libraries/glad.md`](../libraries/glad.md).
- **`GL_SILENCE_DEPRECATION`**: Apple oznaczyło całe OpenGL jako przestarzałe i jego nagłówki
  generują ostrzeżenie przy każdej funkcji. To makro je wycisza. Na Windowsie nie ma efektu.

Definiowanie makr w CMake zamiast `#define` w plikach ma jedną ważną zaletę: nie da się o
nich zapomnieć w nowym pliku.

**Blok 5a: target `game_logic`**

Bloki 5a i 6a doszły w kamieniu milowym M2 + M3. Mają numery z literą, żeby odnośniki
"blok 7" z innych dokumentów pozostały prawdziwe. W pliku stoją w tej kolejności: 5, 5a, 6,
6a, 7.

```cmake
# ---- game_logic: the rules of Night Maze that need no window and no OpenGL -------------
# The maze, its generator and its layout in the world are plain data and math. They live
# in a library of their own, and not in the night_maze executable, so that the test
# program can link them too: a test cannot link code that is inside another executable.
add_library(game_logic STATIC
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
)
# PUBLIC: game/MazeLayout.hpp includes scene/Collider.hpp and GLM, so whoever includes it
# needs the include paths of engine. The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)
```

- `add_library(game_logic STATIC ...)`: druga nasza biblioteka statyczna, z sześciu plików
  logiki labiryntu. Lista jest jawna, tak jak przy `engine`.
- `target_link_libraries(game_logic PUBLIC engine)`: `game_logic` używa `scene::Aabb` z
  `engine`. `PUBLIC`, bo nagłówek `game/MazeLayout.hpp` sam dołącza `scene/Collider.hpp` i
  GLM: każdy, kto go dołączy, potrzebuje ścieżek nagłówków `engine`. Dzięki temu nie ma tu
  osobnego `target_include_directories`: korzeń `src/` przychodzi z `engine`.
- `night_maze_enable_warnings(game_logic)`: te same ścisłe ostrzeżenia co dla `engine`.

**Blok 6: target `night_maze`**

```cmake
# ---- night_maze: the application and its debug UI -------------------------------------
# NightMazeApp stays in the executable: it needs a window and an OpenGL context, so it is
# not something a test can run.
add_executable(night_maze
    src/main.cpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/panels/CameraPanel.cpp
    src/debug/panels/CameraPanel.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
    src/debug/panels/ShadersPanel.cpp
    src/debug/panels/ShadersPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine game_logic imgui)
night_maze_enable_warnings(night_maze)
```

- `add_executable` tworzy program. Bez słowa `WIN32`, więc na Windowsie jest to aplikacja
  konsolowa (opis w [`build-windows.md`](build-windows.md)).
- `PRIVATE engine game_logic imgui`: program niczego dalej nie przekazuje, więc `PRIVATE`
  wystarcza. ImGui linkuje tylko `night_maze`, nigdy `engine` ani `game_logic`.
- `game_logic` jest na liście, choć `NightMazeApp` jeszcze nie woła labiryntu: zacznie w
  następnym kroku tego kamienia milowego. Samo `engine` wystarczyłoby dziś do zbudowania
  programu.
- Z kodu gry w programie zostają `NightMazeApp` i cały katalog `debug/`. Reszta `game/` jest
  w bibliotece `game_logic` (blok 5a).

**Blok 6a: target `night_maze_tests`**

```cmake
# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/ColliderTests.cpp
    tests/ImageLoaderTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/ObjLoaderTests.cpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests read the real models and textures. A test must not depend on the
# directory it is started from, so the absolute path of assets/ in the repository is
# compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)
```

Drugi program w projekcie. Każdą linię omawia
[`../libraries/doctest.md`](../libraries/doctest.md), sekcja 2. W skrócie:

- `enable_testing()` włącza zapis listy testów do katalogu buildu, gdzie znajduje ją program
  `ctest`.
- `add_executable(night_maze_tests ...)` buduje program testowy przy każdym zwykłym buildzie
  (jest częścią targetu domyślnego), więc testy zawsze się kompilują.
- `target_link_libraries(... PRIVATE game_logic doctest::doctest)`: kod testowany i
  biblioteka testów. `engine` przychodzi przez `game_logic`.
- `target_compile_definitions(night_maze_tests PRIVATE NIGHT_MAZE_ASSETS_DIR="...")`:
  definiuje makro preprocesora o wartości będącej napisem, bezwzględną ścieżką katalogu
  `assets` w repozytorium (`CMAKE_SOURCE_DIR` to korzeń repozytorium). Testy loaderów
  czytają prawdziwe modele i tekstury, a nie mogą zależeć od katalogu, z którego je
  uruchomiono. `PRIVATE`: makro widzą tylko pliki testów
  ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7).
- `add_test(...)` rejestruje jeden test CTest: uruchomienie całego programu.

Na Windowsie blok 7 kopiuje katalog `assets` tylko obok `night_maze`. Program testowy leży w
tym samym katalogu, ale z tej kopii nie korzysta: testy loaderów czytają katalog `assets` z
repozytorium, przez makro `NIGHT_MAZE_ASSETS_DIR`.

**Blok 7: katalog `assets` obok programu**

```cmake
if(WIN32)
    # Windows: copy the directory. Symbolic links need Developer Mode or administrator
    # rights there. copy_assets is a custom target without output files, so its command
    # runs on every build of the default (ALL) target: "cmake --build --preset debug"
    # refreshes the copy even when no C++ file changed. Building only the night_maze
    # target does not run it.
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:night_maze>/assets
        COMMENT "Copying assets next to the executable"
        VERBATIM
    )
    # copy_assets does not depend on night_maze (copy_directory creates the destination
    # directory itself), so "cmake --build --preset debug --target copy_assets" refreshes
    # the copy without building the program. That matters while the program is running:
    # Windows locks a running .exe and the Visual Studio generator then fails to link it,
    # also when no C++ file changed.
else()
    # macOS: a symbolic link to the directory in the repository, made each time
    # night_maze has been linked (POST_BUILD). A shader edited in assets/ is seen by the
    # next reload, without building.
    add_custom_command(TARGET night_maze POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:night_maze>/assets
        COMMENT "Linking assets next to the executable"
        VERBATIM
    )
endif()
```

Problem, który ten blok rozwiązuje: program szuka katalogu `assets` obok własnego pliku
wykonywalnego (`core::assetPath`, [`../modules/core/paths.md`](../modules/core/paths.md)),
czyli w `build/debug/`, a pliki leżą w repozytorium, w `assets/`. Każdy system dostaje inne
rozwiązanie i inny mechanizm CMake.

Elementy wspólne:

| Element | Znaczenie |
|---|---|
| `if(WIN32)` | prawda przy budowaniu dla Windowsa. Gałąź `else()` obejmuje macOS |
| `COMMAND ${CMAKE_COMMAND} -E ...` | `${CMAKE_COMMAND}` to pełna ścieżka do programu `cmake`, a tryb `-E` udostępnia małe, przenośne narzędzia (kopiowanie, usuwanie, dowiązania). Dzięki temu nie wołam `ln -s` ani `xcopy`, które istnieją tylko na jednym systemie |
| `${CMAKE_SOURCE_DIR}/assets` | `CMAKE_SOURCE_DIR` to katalog głównego `CMakeLists.txt`, czyli korzeń repozytorium, jako ścieżka **bezwzględna** |
| `$<TARGET_FILE_DIR:night_maze>` | **wyrażenie generatora** (generator expression): katalog, w którym leży plik wykonywalny targetu. Jest wyliczane dopiero podczas generowania i budowania, a nie przy czytaniu `CMakeLists.txt`. To konieczne, bo generator Visual Studio dokłada podkatalog konfiguracji: `build/debug/Debug/`. Zwykła zmienna, na przykład `${CMAKE_BINARY_DIR}`, wskazałaby `build/debug/`, czyli nie katalog pliku `.exe` |
| `COMMENT "..."` | tekst wypisywany w trakcie budowania, gdy polecenie się wykonuje |
| `VERBATIM` | argumenty polecenia są przekazywane dokładnie tak, jak je zapisano, z poprawnym cytowaniem dla danej powłoki (na przykład gdy ścieżka zawiera spację) |

Gałąź macOS, **polecenie doklejone do targetu**:

| Element | Znaczenie |
|---|---|
| `add_custom_command(TARGET night_maze ...)` | dokleja własne polecenie do budowania istniejącego targetu. Nie tworzy nowego targetu |
| `POST_BUILD` | polecenie wykonuje się **po zlinkowaniu** `night_maze`. Gdy program jest aktualny i nie jest linkowany, polecenie się nie wykonuje. Dla dowiązania to wystarcza: raz utworzone, zawsze prowadzi do aktualnych plików |
| `create_symlink <cel> <nazwa>` | tworzy dowiązanie symboliczne `<nazwa>` wskazujące na `<cel>`. Istniejące dowiązanie o tej nazwie jest zastępowane, więc krok można powtarzać |

Gałąź Windows, **osobny target**:

| Element | Znaczenie |
|---|---|
| `add_custom_target(copy_assets ...)` | tworzy nowy target o nazwie `copy_assets`, który niczego nie kompiluje, tylko wykonuje polecenie. Taki target nie ma plików wynikowych, po których CMake mógłby poznać, że jest aktualny, więc jego polecenie wykonuje się **przy każdym budowaniu** tego targetu |
| `ALL` | dołącza `copy_assets` do targetu domyślnego, czyli do tego, co buduje `cmake --build --preset debug` bez opcji `--target`. Bez `ALL` trzeba by go budować osobno |
| `copy_directory <skąd> <dokąd>` | kopiuje katalog z całą zawartością. Katalog docelowy tworzy samo, jeśli go nie ma. Istniejące pliki nadpisuje. Plików usuniętych w źródle **nie** usuwa z kopii |

**`copy_assets` nie zależy od `night_maze`.** W bloku nie ma linii `add_dependencies`, więc
między tymi dwoma targetami nie ma żadnej zależności, w żadną stronę. Budowanie samego
`copy_assets` nie buduje programu, a budowanie samego `night_maze` nie kopiuje katalogu.
Samo wyrażenie `$<TARGET_FILE_DIR:night_maze>` zależności też nie tworzy: dokumentacja CMake
(`add_custom_target`) obiecuje automatyczną zależność od targetu tylko dla wyrażeń
`TARGET_FILE`, `TARGET_LINKER_FILE`, `TARGET_SONAME_FILE` i `TARGET_PDB_FILE`, a
`TARGET_FILE_DIR` na tej liście nie ma. Zgadza się to z pomiarem na Windowsie:
`cmake --build --preset debug --target copy_assets` kończy się powodzeniem przy działającym
programie, czyli nie próbuje go linkować.

Wcześniej blok zawierał linię `add_dependencies(copy_assets night_maze)`, która kazała
budować `copy_assets` po programie, żeby katalog pliku wykonywalnego istniał przed
kopiowaniem. Usunąłem ją po pierwszym buildzie na Windowsie z dwóch powodów. Po pierwsze
jest niepotrzebna: `copy_directory` samo tworzy katalog docelowy (zmierzone: target odtwarza
usunięty katalog `assets`). Po drugie szkodziła: żeby odświeżyć shadery, trzeba było budować
także program, a z generatorem Visual Studio build przy działającym programie kończy się
błędem `LINK : fatal error LNK1168`, także wtedy, gdy żaden plik C++ się nie zmienił
(Windows blokuje plik `.exe` działającego programu, a MSBuild próbuje go zlinkować od nowa).

**Dlaczego dwie gałęzie.** Dowiązanie jest lepsze: zajmuje zero miejsca i zawsze prowadzi do
aktualnych plików, więc shader zmieniony w edytorze jest widoczny przy następnym wczytaniu,
bez budowania. Na Windowsie utworzenie dowiązania symbolicznego wymaga jednak włączonego
trybu dewelopera albo uprawnień administratora, a build ma działać na świeżo skonfigurowanym
komputerze. Dlatego tam katalog jest kopiowany.

**Dlaczego na Windowsie target, a nie `POST_BUILD`.** Kopia się starzeje: po zmianie pliku w
`assets/` trzeba ją zrobić od nowa. Polecenie `POST_BUILD` wykonuje się tylko przy linkowaniu
programu, a zmiana shadera nie zmienia żadnego pliku C++, więc niczego nie linkuje. Target
`copy_assets` wykonuje się przy każdym swoim budowaniu, więc na Windowsie obowiązuje prosta
reguła: **skopiuj (`--target copy_assets`), potem wczytaj shadery ponownie**.

| Polecenie | Czy odświeża kopię | Przy działającym programie (generator Visual Studio) |
|---|---|---|
| `cmake --build --preset debug --target copy_assets` | tak. Programu nie buduje | działa: kod wyjścia 0, kopia odświeżona |
| `cmake --build --preset debug` | tak, zawsze, także gdy żaden plik C++ się nie zmienił (`copy_assets` jest w `ALL`) | **błąd** `LNK1168` |
| `cmake --build --preset debug --target night_maze` | **nie**: `night_maze` nie zależy od `copy_assets` | nie mierzyłem |

Wszystkie wypełnione komórki tej tabeli są zmierzone na Windowsie 2026-10-05
([`build-windows.md`](build-windows.md), sekcja 7). Tam też jest wynik dla generatora Ninja:
pełny build przy działającym programie przeszedł, bo wykonał tylko krok kopiowania.

Trzeci wiersz ma znaczenie dla IDE: uruchomienie programu klawiszem F5 w Visual Studio może
zbudować tylko projekt startowy, czyli sam `night_maze`. Czy tak jest, trzeba jeszcze
sprawdzić w Visual Studio ([`build-windows.md`](build-windows.md), sekcja 11): na PC, na
którym mierzyłem, są same Build Tools, bez IDE.

Ten mechanizm sprawdziłem najpierw na Macu, wymuszając tymczasowo gałąź Windows (warunek
zmieniony na `if(TRUE)`, jeszcze z linią `add_dependencies`): pierwszy build utworzył
prawdziwy katalog `build/debug/assets` z kopią plików, drugi build po zmianie samego
komentarza w shaderze wypisał `Copying assets next to the executable` i odświeżył kopię bez
linkowania, a budowanie z `--target night_maze` kopii nie odświeżyło. Na Windowsie te trzy
wyniki się powtórzyły (program zatrzymany), a doszedł czwarty, którego Mac nie mógł
pokazać: błąd `LNK1168` przy działającym programie.

Jeden katalog buildu nigdy nie przechodzi z jednego mechanizmu na drugi (to dwa różne
systemy operacyjne), więc blok nie zawiera żadnego sprzątania po "tym drugim" wariancie.

Pliki z `assets/` nie są na żadnej liście źródeł: kompilator C++ ich nie widzi. Nowy plik
shadera nie wymaga więc zmiany w `CMakeLists.txt` (sekcja 5).

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
wydań (stb, które tagów nie ma, do commita). Plik jest osobno, żeby główny `CMakeLists.txt` opisywał tylko nasze targety.

| Fragment | Co robi | Szczegółowy opis |
|---|---|---|
| `include(FetchContent)` | wczytuje moduł pobierania | [`../libraries/glfw.md`](../libraries/glfw.md) |
| cztery linie `set(GLFW_... OFF CACHE BOOL "" FORCE)` | wyłączają dokumentację, testy, przykłady i instalację GLFW | [`../libraries/glfw.md`](../libraries/glfw.md) |
| `FetchContent_Declare(glfw ... GIT_TAG 3.4 ...)` i `FetchContent_MakeAvailable(glfw)` | pobierają GLFW 3.4 i tworzą target `glfw` | [`../libraries/glfw.md`](../libraries/glfw.md) |
| `get_target_property` i `set_target_properties(... INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ...)` | oznaczają nagłówki GLFW jako systemowe (bez ostrzeżeń) | [`../libraries/glfw.md`](../libraries/glfw.md) |
| trzy linie `set(GLM_BUILD_... OFF CACHE BOOL "" FORCE)` | wyłączają bibliotekę statyczną, testy i instalację GLM: zostają same nagłówki | [`../libraries/glm.md`](../libraries/glm.md) |
| `FetchContent_Declare(glm ... GIT_TAG 1.0.3 ...)` i `FetchContent_MakeAvailable(glm)` | pobierają GLM 1.0.3 i tworzą target `glm-header-only` (alias `glm::glm-header-only`) | [`../libraries/glm.md`](../libraries/glm.md) |
| `get_target_property` i `set_target_properties(glm-header-only ... INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ...)` | oznaczają nagłówki GLM jako systemowe (bez ostrzeżeń) | [`../libraries/glm.md`](../libraries/glm.md) |
| `FetchContent_Declare(imgui ... GIT_TAG v1.92.9b-docking ...)` i `FetchContent_MakeAvailable(imgui)` | pobierają Dear ImGui, bez tworzenia targetu | [`../libraries/imgui.md`](../libraries/imgui.md) |
| `add_library(imgui STATIC ...)`, `target_include_directories`, `target_link_libraries(imgui PUBLIC glfw)` | ręcznie zdefiniowany target `imgui` z rdzenia i dwóch backendów | [`../libraries/imgui.md`](../libraries/imgui.md) |
| trzy linie `set(DOCTEST_... CACHE BOOL "" FORCE)` | wyłączają bibliotekę statyczną z gotowym `main`, testy samego doctest i reguły instalacji | [`../libraries/doctest.md`](../libraries/doctest.md) |
| `FetchContent_Declare(doctest ... GIT_TAG v2.5.3 ...)` i `FetchContent_MakeAvailable(doctest)` | pobierają doctest 2.5.3 i tworzą target `doctest` (alias `doctest::doctest`). Nagłówek jest systemowy bez naszego kroku: tak deklaruje go `CMakeLists.txt` samego doctest | [`../libraries/doctest.md`](../libraries/doctest.md) |
| `FetchContent_Declare(stb ... GIT_TAG 2c980bb5...)` i `FetchContent_MakeAvailable(stb)` | pobierają repozytorium stb w jednym, wskazanym commicie (repozytorium nie ma tagów, a płytki klon nie umie pobrać commita, więc bez `GIT_SHALLOW`). Targetu nie tworzą | [`../libraries/stb_image.md`](../libraries/stb_image.md) |
| `add_library(stb_image STATIC ...)`, `target_include_directories(... SYSTEM PUBLIC ...)`, `target_compile_definitions(stb_image PUBLIC STBI_NO_STDIO)` | ręcznie zdefiniowany target `stb_image`: jeden plik `external/stb/stb_image.c`, nagłówek jako systemowy, bez funkcji otwierających pliki po nazwie | [`../libraries/stb_image.md`](../libraries/stb_image.md) |

Pierwsza linia komentarza w pliku przypomina, dlaczego nie ma tu GLAD: to kod wygenerowany,
który leży w `external/glad`.

Zmiana wersji biblioteki to zmiana jednej linii `GIT_TAG` i ponowna konfiguracja.

Obok bloku stb leży w repozytorium katalog `external/stb/` z dwoma plikami: `stb_image.c`
(dwie linie, które kompilują implementację pobranego nagłówka) i `README.md` (skąd jest
nagłówek i jak zmienić wersję). To nasz kod, nie wygenerowany: w odróżnieniu od
`external/glad/` katalog nie ma własnego `CMakeLists.txt`, bo target potrzebuje zmiennej
`stb_SOURCE_DIR`, która powstaje dopiero w `Dependencies.cmake`.

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
  `src/`. Nagłówki GLFW, GLAD, GLM i ImGui są pomijane.

Konwencja nazw z `CheckOptions`:

| Co | Styl | Przykład z kodu |
|---|---|---|
| przestrzeń nazw (`NamespaceCase`) | `lower_case` | `core`, `gfx`, `scene`, `game`, `debug` |
| klasa, struktura, enum (`ClassCase`, `StructCase`, `EnumCase`) | `CamelCase` | `Window`, `Size`, `DebugUI`, `Camera` |
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

Pojedynczą diagnostykę można wyłączyć w kodzie komentarzem `// NOLINTNEXTLINE(nazwa-kontroli)`
w linii poprzedzającej. W `src/` jest jedno takie miejsce: zamiana przesunięcia na wskaźnik w
`src/gfx/VertexArray.cpp` (kontrola `performance-no-int-to-ptr`), opisana w
[`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md), sekcja 5.6.

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
GLFW, GLAD, GLM i ImGui, `-std=c++20`, makra `GLFW_INCLUDE_NONE` i `GL_SILENCE_DEPRECATION`).
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
| `files.associations` | `*.vert`, `*.frag`, `*.geom`, `*.glsl` na `glsl` | pliki shaderów są traktowane jako język GLSL (kolorowanie składni). Dziś są to `assets/shaders/basic.vert` i `basic.frag` |

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

### 3.12. `Makefile`

Plik ze **skrótami** do poleceń, których używa się codziennie. Niczego sam nie kompiluje:
każdy cel (target) woła CMake przez presety albo narzędzie (`clang-format`, `clang-tidy`).
Dzięki temu jest jedno źródło prawdy o budowaniu (`CMakeLists.txt` i `CMakePresets.json`),
a `Makefile` tylko oszczędza pisania. Nie należy go mylić z plikiem
`build/<preset>/Makefile`, który generuje CMake (sekcja 4.1): tamten jest artefaktem buildu,
ten jest napisany ręcznie i leży w Gicie.

| Polecenie | Co wykonuje | Kiedy używać |
|---|---|---|
| `make` albo `make help` | wypisuje listę celów | gdy nie pamiętam nazw |
| `make debug` | `cmake --preset debug`, potem `cmake --build --preset debug` | zwykły build w trakcie pracy |
| `make release` | to samo dla presetu `release` | pomiar wydajności, wersja do pokazania |
| `make run` | `make debug`, potem uruchamia `build/debug/night_maze` | najczęstsze polecenie |
| `make run-release` | `make release`, potem uruchamia `build/release/night_maze` | sprawdzenie 60 FPS |
| `make test` | `make debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure` | testy jednostkowe po zmianie w kolizjach albo labiryncie |
| `make test-release` | `make release`, potem `ctest --test-dir build/release -C Release --output-on-failure` | te same testy na kodzie z optymalizacjami |
| `make format` | `clang-format -i` na wszystkich plikach `.cpp` i `.hpp` z `src/` i `tests/` | naprawia formatowanie w miejscu |
| `make format-check` | `clang-format --dry-run --Werror` na tych samych plikach | tylko sprawdza, niczego nie zmienia |
| `make tidy` | `make debug`, potem `clang-tidy` na plikach `.cpp` z `src/` i `tests/` | analiza statyczna |
| `make check` | `format-check`, `test`, `test-release`, `tidy` | wszystko przed commitem i przed tagiem |
| `make clean` | usuwa katalog `build/` | gdy konfiguracja się zepsuła albo chcę czystego buildu |

Elementy pliku, które trzeba umieć wyjaśnić:

- **Reguła** ma postać `cel: zależności`, a pod nią polecenia. Linie poleceń **muszą zaczynać
  się od znaku tabulacji**, nie od spacji. To najczęstszy błąd przy edycji (`missing
  separator`).
- **Zależności** to cele wykonywane wcześniej. `run: debug` znaczy: najpierw zbuduj Debug,
  potem uruchom. `check: format-check test test-release tidy` wykonuje cztery cele po kolei i
  zatrzymuje się na pierwszym, który zakończy się błędem. Oba buildy nie znikły z `check`:
  `test` ma zależność `debug`, a `test-release` zależność `release`, więc build poprzedza
  testy.
- **`ctest ... -C Debug`** w celu `test`: `-C` wskazuje konfigurację do przetestowania.
  Generator Visual Studio jej wymaga (jeden katalog buildu mieści tam Debug i Release),
  generatory jednokonfiguracyjne ją ignorują, więc jedna linia działa na obu systemach.
- **`.PHONY`** mówi, że wymienione nazwy to polecenia, a nie pliki. `make` z natury sprawdza,
  czy plik o nazwie celu istnieje i jest aktualny. Bez `.PHONY` katalog albo plik o nazwie
  `debug` sprawiłby, że `make debug` nic by nie zrobił.
- **`:=`** to przypisanie zmiennej wyliczane od razu, raz. `$(shell ...)` uruchamia polecenie
  powłoki i wstawia jego wynik, na przykład listę plików z `find src tests -name '*.cpp'`.
- **`@` przed poleceniem** wyłącza wypisanie samego polecenia. Używam go tylko przy `echo`,
  żeby tekst nie pojawiał się dwa razy. Pozostałe polecenia są wypisywane, więc widać, co
  dokładnie zostało uruchomione.
- **`ifeq ($(OS),Windows_NT)`** wybiera ścieżkę programu. Na Windowsie generator Visual Studio
  dodaje podkatalog konfiguracji i rozszerzenie `.exe` (sekcja 4.2).
- **`CLANG_TIDY`** bierze `clang-tidy` z `PATH`, a gdy go tam nie ma, z pakietu `llvm`
  Homebrew, który celowo nie jest dodawany do `PATH` (sekcja 3.6). `$$` w pliku Makefile to
  jeden znak `$` przekazany do powłoki.
- **`TIDY_EXTRA_ARGS`** na macOS dopisuje ścieżkę do SDK (`xcrun --show-sdk-path`), bez której
  clang-tidy z Homebrew nie znajduje nagłówków biblioteki standardowej.
- **`--warnings-as-errors='*'`** w celu `tidy` zamienia każdą diagnostykę w błąd, żeby
  `make check` zatrzymał się na niej. Samo `.clang-tidy` ma `WarningsAsErrors: ''`, czyli w
  edytorze clang-tidy tylko doradza. Bramką jest dopiero `make check`.
- **`cmake -E rm -rf build`** w celu `clean` to wbudowane w CMake, przenośne usuwanie
  katalogu, działające tak samo na macOS i na Windowsie.

Program jest uruchamiany z katalogu głównego repozytorium, więc `imgui.ini` zawsze trafia w
to samo miejsce (sekcja 4.3).

Stan na M0: na Macu sprawdzone zostały `make`, `make check`, `make run` oraz to, że
`make format-check` kończy się błędem dla źle sformatowanego pliku. Na Windowsie plik nie
był uruchamiany (na PC, na którym budowałem projekt 2026-10-05, nie ma `make`): wymaga
programu `make` (na przykład z Git Bash, MSYS2 albo Chocolatey), a
cele `format`, `format-check` i `tidy` korzystają z poleceń `find` i `command -v`, więc
potrzebują powłoki typu Unix. Bez `make` wszystkie polecenia z tabeli można wpisać ręcznie.

Stan po zmianach z M2 + M3 (cele `test` i `test-release`, katalog `tests/` w listach plików,
nowy skład `check`): **plik w tej postaci nie był jeszcze uruchamiany** ani na Macu, ani na
Windowsie. Polecenia, które wołają nowe cele, są zmierzone osobno na Windowsie:
`ctest --test-dir build/debug -C Debug --output-on-failure` i to samo dla Release
przechodzą, `clang-format --dry-run --Werror` na plikach z `src/` i `tests/` nie zgłasza
niczego. Przebieg clang-tidy (LLVM 19.1.5, z bazy poleceń generatora Ninja) nie zgłasza
niczego w nowych plikach ani w testach. Zgłasza natomiast jedną diagnostykę w starszym
pliku, w gałęzi kompilowanej tylko na Windowsie: `modernize-return-braced-init-list` w
`src/core/Paths.cpp`. Z `--warnings-as-errors='*'` cel `tidy` zatrzymałby się więc na niej na
Windowsie. Na Macu ta gałąź nie jest kompilowana.

## 4. Artefakty generowane podczas budowania (poza Gitem)

Wszystko poniżej powstaje automatycznie i jest w `.gitignore`. Można to w każdej chwili
usunąć i odtworzyć konfiguracją oraz buildem.

### 4.1. Katalog `build/<preset>/`

Układ `build/debug` na Macu (generator Unix Makefiles), odczytany z dysku:

```text
build/debug/
├── night_maze                  # program
├── assets -> <repo>/assets     # dowiązanie symboliczne do katalogu assets/ z repozytorium
├── libengine.a                 # biblioteka statyczna engine (src/core, src/gfx, src/scene)
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
    ├── glm-src/                # kod źródłowy GLM 1.0.3 (same nagłówki w glm-src/glm/)
    ├── glm-build/              # tylko pliki robocze CMake: GLM niczego nie kompiluje
    ├── glm-subbuild/           # pomocniczy projekt, który wykonał pobranie
    ├── imgui-src/              # kod źródłowy Dear ImGui v1.92.9b-docking
    ├── imgui-build/            # pusty: ImGui nie ma własnego CMakeLists.txt
    └── imgui-subbuild/         # pomocniczy projekt, który wykonał pobranie
```

| Artefakt | Skąd się bierze | Do czego służy |
|---|---|---|
| `night_maze` | linkowanie targetu `night_maze` | program, który uruchamiamy |
| `assets` | polecenie `POST_BUILD` targetu `night_maze` (sekcja 3.1, blok 7) | dowiązanie symboliczne do `<repo>/assets` ze ścieżką bezwzględną. Tędy program znajduje shadery. Usunięcie katalogu `build/` usuwa samo dowiązanie, pliki w repozytorium zostają |
| `libengine.a` | target `engine` | skompilowany kod `src/core`, `src/gfx` i `src/scene`, wklejany do programu |
| `libimgui.a` | target `imgui` z `Dependencies.cmake` | skompilowany rdzeń ImGui i dwa backendy |
| `external/glad/libglad.a` | target `glad` | skompilowany `gl.c` |
| `_deps/glfw-build/src/libglfw3.a` | target `glfw` | skompilowane GLFW |
| `_deps/glm-src/glm/` | FetchContent | nagłówki GLM. Biblioteki `.a` dla GLM nie ma: to target `INTERFACE`, a `GLM_BUILD_LIBRARY` jest wyłączone |
| `compile_commands.json` | `CMAKE_EXPORT_COMPILE_COMMANDS=ON` z presetu `base` | wejście dla clangd i clang-tidy |
| `CMakeCache.txt` | konfiguracja | trwałe zmienne CMake: ścieżka kompilatora, generator, opcje `GLFW_BUILD_*` |
| `CMakeFiles/` | konfiguracja i build | pliki obiektowe (`.o`), zależności między plikami, log konfiguracji |
| `_deps/*-src` | FetchContent | pobrany kod. Przydatny do czytania: `imgui-src/imgui_demo.cpp`, `glfw-src/docs/`, `glm-src/manual.md` |

Wszystkie biblioteki są statyczne, więc program `night_maze` jest jednym plikiem
wykonywalnym. Do działania potrzebuje bibliotek systemowych i katalogu `assets` obok
siebie.

Położenie plików `.a` odzwierciedla drzewo źródeł: target zdefiniowany w głównym
`CMakeLists.txt` trafia do korzenia katalogu buildu, target z `external/glad` do
`external/glad/`, a GLFW do `_deps/glfw-build/`.

Układ powyżej pochodzi sprzed kamienia milowego M2 + M3. Po nim w katalogu buildu dochodzą:
biblioteka `game_logic`, program `night_maze_tests`, katalogi `_deps/doctest-src`,
`_deps/doctest-build` i `_deps/doctest-subbuild` oraz plik `CTestTestfile.cmake` (lista
testów dla `ctest`). Dochodzą też biblioteka `stb_image` i katalogi `_deps/stb-src`
(12 MB, całe repozytorium stb z historią), `_deps/stb-build` i `_deps/stb-subbuild`. Na Macu tego układu jeszcze nie odczytałem z dysku. Oczekiwane nazwy to
`libgame_logic.a` i `night_maze_tests` w korzeniu `build/debug`. Na Windowsie jest zmierzony
(sekcja 4.2).

`build/release` ma taki sam układ, osobne `_deps` i inne flagi kompilacji. Różnice między
Debug a Release opisuje [`build-macos.md`](build-macos.md), sekcja 5.

### 4.2. Windows: podkatalogi `Debug\` i `Release\`

Generator Visual Studio jest wielokonfiguracyjny, więc wyniki każdej konfiguracji trafiają
do dodatkowego podkatalogu. Program leży w `build\debug\Debug\night_maze.exe` i
`build\release\Release\night_maze.exe`, a biblioteki mają rozszerzenie `.lib` zamiast `.a`.
W katalogu buildu zamiast `Makefile` jest rozwiązanie `.sln` i pliki projektów `.vcxproj`, a
`compile_commands.json` nie powstaje. Katalog `assets` obok programu
(`build\debug\Debug\assets\`) jest tam zwykłym katalogiem z **kopią** plików, a nie
dowiązaniem. Kopię robi target `copy_assets` (sekcja 3.1, blok 7).

Od M2 + M3 w tym samym podkatalogu leżą też `game_logic.lib` i program testowy:
`build\debug\Debug\night_maze_tests.exe` i `build\release\Release\night_maze_tests.exe`
(zmierzone). W korzeniu katalogu buildu są `CTestTestfile.cmake` i projekt `RUN_TESTS.vcxproj`,
który CMake dodaje po `enable_testing()`.

Zmierzone na Windowsie 2026-10-05: oba położenia programu, generator `Visual Studio 17 2022`
i katalogi `assets\shaders\` z kopią obu shaderów obok każdego z programów. Reszta akapitu
(rozszerzenie `.lib`, pliki `.sln` i `.vcxproj`) wynika z dokumentacji CMake i nie była
osobno sprawdzana. Z generatorem Ninja podkatalogu konfiguracji nie ma (zmierzone: program w
`build\ninja-debug\night_maze.exe`) i powstaje `compile_commands.json`. Wyjaśnienie i lista
kontrolna są w [`build-windows.md`](build-windows.md).

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
   | podstawa programu, bez OpenGL poza `GL_CHECK`: okno, pętla, wejście, czas, logi, ścieżki | `src/core/` | `add_library(engine STATIC ...)` |
   | opakowanie jednego obiektu OpenGL (RAII, tylko przenoszenie), bez wiedzy o grze i bez ImGui | `src/gfx/` | `add_library(engine STATIC ...)` |
   | opis sceny: dane i matematyka (transformy, kamera), bez wiedzy o grze, bez wejścia i bez ImGui | `src/scene/` | `add_library(engine STATIC ...)` |
   | wczytywanie plików z `assets/` do danych procesora, bez OpenGL, bez wiedzy o grze i bez ImGui | `src/assets/` | `add_library(engine STATIC ...)` |
   | inny kod wielokrotnego użytku, bez wiedzy o grze i bez ImGui | później `src/renderer/` | `add_library(engine STATIC ...)` |
   | logika Night Maze, która nie potrzebuje okna ani OpenGL (ma dać się testować) | `src/game/` | `add_library(game_logic STATIC ...)` |
   | kod gry, który potrzebuje okna, kontekstu OpenGL albo wejścia | `src/game/` | `add_executable(night_maze ...)` |
   | test jednostkowy | `tests/` | `add_executable(night_maze_tests ...)` |
   | panel debugowy | `src/debug/panels/` | `add_executable(night_maze ...)` |

2. **Utwórz parę plików** `Nazwa.hpp` i `Nazwa.cpp` obok siebie. Pierwsze linie każdego
   pliku to komentarz z jednym zdaniem opisu i odnośnikiem `See docs/modules/<dokument>.md`
   (dla modułu podzielonego na katalog: `See docs/modules/<moduł>/<dokument>.md`).
   Nagłówek zaczyna się od `#pragma once`, kod jest w przestrzeni nazw warstwy (`core`,
   `gfx`, `scene`, `game`, `debug`). Publiczne API dostaje komentarze Doxygen (`///`).

3. **Dopisz oba pliki do właściwej listy w `CMakeLists.txt`**, zachowując kolejność
   alfabetyczną. Przykład dla wymyślonej klasy `Random` w `core` (takiego pliku w projekcie nie
   ma, to tylko ilustracja):

   ```cmake
   add_library(engine STATIC
       ...
       src/core/Paths.cpp
       src/core/Paths.hpp
       src/core/Random.cpp     # nowy plik
       src/core/Random.hpp     # nowy plik
       src/core/Time.cpp
       ...
   )
   ```

   Bez tego kroku plik nie zostanie skompilowany, a objawem będzie błąd linkera
   `undefined symbol`.

4. **Dołączaj nagłówki ścieżką od `src/`**: `#include "core/Random.hpp"` (dla przykładu z
   kroku 3).

5. **Zbuduj.** CMake wykryje zmianę w `CMakeLists.txt` i sam powtórzy konfigurację
   ([`build-macos.md`](build-macos.md), sekcja 2).

6. **Sformatuj** kod narzędziem clang-format ([`build-macos.md`](build-macos.md), sekcja 7).

7. **Sprawdź regułę warstw**: plik w `core/` nie może dołączać niczego z `gfx/`, `scene/`,
   `game/` ani `debug/`, plik w `gfx/` niczego z `scene/`, `game/` ani `debug/`, plik w
   `scene/` niczego z `game/` ani `debug/`, plik w `game/` niczego z `debug/`. Plik biblioteki
   `game_logic` dodatkowo niczego z GLAD, GLFW, `core/` ani `gfx/`.

Nowy panel debugowy ma dodatkowe kroki (wywołanie w `DebugUI::draw`, a dla nowych danych
pole w `debug::DebugContext` i linia w `main.cpp`). Opisuje je
[`../modules/debug-ui.md`](../modules/debug-ui.md).

### Nowy test

1. **Do istniejącego pliku**: nowy blok `TEST_CASE("...") { ... }` w odpowiednim pliku w
   `tests/`. Niczego więcej nie trzeba: test rejestruje się sam
   ([`../libraries/doctest.md`](../libraries/doctest.md), sekcja 1).
2. **Nowy plik testowy**: `tests/<Nazwa>Tests.cpp` z nagłówkiem (zdanie opisu i
   `See docs/...`), dołączeniem testowanego nagłówka i `<doctest/doctest.h>`. Makra
   `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` **nie** powtarzać: jest tylko w `tests/main.cpp`.
3. **Dopisz plik do `add_executable(night_maze_tests ...)`** w `CMakeLists.txt`. Bez tego
   plik nie jest kompilowany, a jego testy po cichu nie istnieją.
4. **Kod, który ma być testowany, musi być w bibliotece** (`engine` albo `game_logic`), nie w
   programie `night_maze`.
5. **Zbuduj i uruchom**: `cmake --build --preset debug`, potem
   `ctest --test-dir build/debug -C Debug --output-on-failure`. Sprawdź w wyjściu programu
   testowego, że liczba przypadków wzrosła.
6. **Sformatuj** plik testu tak jak każdy inny (`make format` obejmuje `tests/`).

### Nowy plik shadera albo inny asset

1. **Utwórz plik w `assets/`**, shader w `assets/shaders/`. Nazwa pary shaderów jest wspólna,
   różni się rozszerzeniem: `.vert` dla shadera wierzchołków, `.frag` dla shadera
   fragmentów. Edytor rozpoznaje je jako GLSL (sekcja 3.10).
2. **Pierwsza linia shadera to `#version 410 core`.** Pod nią komentarz z jednym zdaniem
   opisu i odnośnikiem `See docs/modules/...`, tak jak w plikach C++.
3. **Niczego nie dopisuj w `CMakeLists.txt`.** Krok z bloku 7 obejmuje cały katalog `assets/`.
4. **W kodzie buduj ścieżkę przez `core::assetPath`**, z nazwą względną wobec `assets/`, na
   przykład `core::assetPath("shaders/basic.vert")`. Wzór: stałe `VERTEX_SHADER_FILE` i
   `FRAGMENT_SHADER_FILE` w `src/game/NightMazeApp.cpp`.
5. **macOS:** nic więcej, dowiązanie `build/<preset>/assets` widzi nowy plik od razu.
   **Windows:** kopię obok `night_maze.exe` odświeża
   `cmake --build --preset debug --target copy_assets`, a także każdy pełny build przy
   zamkniętym programie ([`build-windows.md`](build-windows.md), sekcja 7).
6. **Opisz plik** w dokumencie modułu (sekcja 4 szablonu, "Shadery") i w drzewie na początku
   tego dokumentu.

### Nowa biblioteka zewnętrzna

- Pobierana (ma repozytorium z tagami wydań): nowy blok `FetchContent_Declare` i
  `FetchContent_MakeAvailable` w `cmake/Dependencies.cmake`, przypięty do tagu, potem
  `target_link_libraries` przy targecie, który jej używa.
- Wygenerowana lub jednoplikowa, trzymana w repozytorium: nowy katalog w `external/` z
  własnym `CMakeLists.txt` i `add_subdirectory` w głównym pliku.
- Pobierana, ale bez własnego CMake i z implementacją w nagłówku (jak stb_image): blok
  FetchContent, pod nim ręcznie zdefiniowany target z jednym plikiem z `external/`, który
  kompiluje implementację. Bez tagów wydań: `GIT_TAG` z pełnym skrótem commita i bez
  `GIT_SHALLOW`. Wzór: blok stb w `cmake/Dependencies.cmake`.
- W obu przypadkach nagłówki oznaczamy jako `SYSTEM`, a bibliotece nie włączamy naszych
  ostrzeżeń.
- Biblioteka z samych nagłówków (jak GLM) nie ma niczego do skompilowania: linkujemy jej
  target `INTERFACE`, który niesie tylko ścieżkę nagłówków. Wzór: blok GLM w
  `cmake/Dependencies.cmake`.

### Dokumentacja

Według PRD moduł jest skończony dopiero wtedy, gdy ma dokument w `docs/`, aktualizowany w
tym samym commicie co kod.

| Co dodajesz | Gdzie trafia dokument |
|---|---|
| nowy moduł lub klasa w istniejącym module | `docs/modules/<moduł>.md` (szablon 10 sekcji z PRD, sekcja 7). Duży moduł ma katalog `docs/modules/<moduł>/` z plikiem `README.md` (wstęp i indeks) i dokumentami tematycznymi, z których każdy ma pełne 10 sekcji. Wzór: `docs/modules/core/`, `docs/modules/gfx/`, `docs/modules/scene/` i `docs/modules/game/` |
| nowa biblioteka | `docs/libraries/<biblioteka>.md` |
| zmiana w budowaniu, narzędziach lub strukturze | `docs/guides/` (ten plik, `build-macos.md`, `build-windows.md`) |
| decyzja "dlaczego tak, a nie inaczej" | `docs/decisions/<temat>.md`, według układu z [`../decisions/README.md`](../decisions/README.md), plus wiersz na liście notatek w tym pliku |

Po dodaniu pliku, katalogu albo targetu trzeba też zaktualizować drzewo i tabele w tym
dokumencie.
