# Struktura projektu (stan: M1 kompletny, kod M2 + M3, M4 i M5 na Windowsie, M6 w toku: skybox)

Kompletna mapa repozytorium Night Maze: co leży w którym katalogu, do czego służy każdy plik
konfiguracyjny i co powstaje dopiero podczas budowania. Dokument opisuje stan faktyczny po
kamieniu milowym M5, który dodał rozgrywkę (2026-10-05). Poprzedni
stan to druga część M4, która dodała mapy normalnych (2026-10-05).

Z M1 są: mysz, ścieżki do assetów, GLM, warstwa `gfx/` z klasami `Shader`, `Buffer` i
`VertexArray`, warstwa `scene/` ze strukturami `Transform` i `Camera` oraz panele Shaders i
Camera. Pierwszym rysowanym obiektem była w M1 kostka z macierzami modelu, widoku i
rzutowania, z własną parą shaderów `basic`: M5 usunął ją razem z tymi plikami, a te same
tematy pokazuje dziś labirynt. Kod M1 jest zbudowany i
uruchomiony na macOS i na Windowsie, ręczne sprawdzenie sterowania na Windowsie jest otwarte,
tagu nie ma.

Z M2 + M3 są: kolizje (`src/scene/Collider.*`), labirynt z generatorem i układem w świecie
(`src/game/Maze*`), gracz (`src/game/Player.*`), biblioteka `game_logic`, katalog `tests/` z
programem `night_maze_tests` i biblioteka doctest, loader modeli OBJ
(`src/assets/ObjLoader.*`), wierzchołek i siatka (`src/gfx/Vertex.hpp`, `src/gfx/Mesh.*`),
loader obrazów (`src/assets/ImageLoader.*`) z biblioteką stb_image (`external/stb/` i blok w
`cmake/Dependencies.cmake`), klasa tekstury (`src/gfx/Texture2D.*`), pamięć podręczna assetów
(`src/assets/AssetCache.*`), labirynt w świecie i jego rysowanie (`src/game/MazeWorld.*`,
`src/game/MazeRenderer.*`), linie pudełek kolizji (`src/game/ColliderLines.*`), dwie nowe
pary shaderów (`textured`, `color`) oraz panele Maze, Collision i Assets. Program startuje w
oteksturowanym labiryncie 10 na 10, po którym gracz chodzi z kolizjami. Ten kod jest
zbudowany, przetestowany i uruchomiony na Windowsie (2026-10-05). Na macOS nie był budowany,
a ręczne sprawdzenie chodzenia i paneli jest otwarte na obu systemach, więc kamień milowy
nie jest zamknięty i nie ma tagu.

Z M4 (pierwsza część: oświetlenie) są: światła jako dane i ich matematyka
(`src/scene/Light.*`), bajty bloku uniformów ze światłami (`src/scene/LightBlock.*`), bufor
uniformów (`src/gfx/UniformBuffer.*`), dyrektywa `#include` w shaderach i nazwy plików w
komunikatach błędów (`src/gfx/ShaderSource.*`), nowe settery i wiązanie bloku uniformów w
klasie `Shader`, macierz normalnych (`scene::normalMatrix` w `src/scene/Transform.*`),
ustawienia oświetlenia (`src/game/Lighting.*`), klasa
wysyłająca światła na kartę (`src/game/LightRig.*`), dwie nowe pary shaderów (`lit`,
`gouraud`) z jednym wspólnym plikiem dołączanym (`assets/shaders/common/lighting.glsl`),
panel Lights, lista trybów `Lighting` w panelu Renderer, układ siedmiu paneli (dziś ośmiu) i
cztery nowe pliki testów. Program startuje w nocnej, oświetlonej scenie. W M4 światła
punktowe wisiały w ślepych zaułkach i były oznaczone kostkami: M5 zastąpił je światłami
kryształów i usunął ten kod.

Z M4 (druga część: mapy normalnych) są: dwie mapy normalnych w `assets/textures/`
(`wall_stone_normal.png`, `floor_stone_normal.png`, liczone przez
`tools/blender/make_textures.py`), linia `map_Bump` w trzech plikach `.mtl` i jej obsługa w
`src/assets/ObjLoader.*`, styczne wierzchołków (`src/assets/Tangents.*`), czwarte pole
struktury `gfx::Vertex` i czwarty atrybut w `gfx::Mesh`, mapa normalnych każdej części modelu
i płaska mapa zastępcza w `src/assets/AssetCache.*`, drugi plik dołączany do shaderów
(`assets/shaders/common/normal_map.glsl`), druga jednostka teksturująca w
`src/game/MazeRenderer.*`, przełącznik `LightingSettings::normalMapping` z funkcją
`usesNormalMap` w `src/game/Lighting.*`, pole `Normal mapping` w panelu Assets i jeden nowy
plik testów (`tests/TangentTests.cpp`). Całość opisuje
[`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md).

Kod obu części M4 jest zbudowany, przetestowany i uruchomiony na Windowsie (2026-10-05,
wtedy 163 przypadki testowe i 62220 asercji). Na macOS nie był budowany, a ręczne sprawdzenie klawisza
F, pola `Normal mapping` i pozostałych widżetów jest otwarte
([`build-windows.md`](build-windows.md), sekcja 13).

Z M5 (rozgrywka) są: kule jako drugi kształt kolizji (`scene::Sphere` w
`src/scene/Collider.*`), wyjście w komórce najdalszej od startu z bramą i strefą wyjścia
(`src/game/Exit.*`), kryształy (`src/game/Crystals.*`), stan i reguły rundy z baterią
latarki (`src/game/Round.*`), rysowanie kryształów i bramy (`src/game/GameplayRenderer.*`) i
wspólne rysowanie modeli (`src/game/ModelDraw.*`), pasek HUD z kartą wygranej
(`src/debug/Hud.*`), ósmy panel, Gameplay (`src/debug/panels/GameplayPanel.*`), trzy nowe
modele (`crystal_a`, `crystal_b`, `gate`) z czterema nowymi teksturami i dwoma skryptami
Blendera, uniform `uEmissive` w trzech shaderach fragmentów, klawisz R i trzy nowe pliki
testów. Zniknęły: kostka z M1 z plikami `assets/shaders/basic.vert` i `basic.frag`, kostki
oznaczające światła i światła w ślepych zaułkach. Całość opisuje
[`../modules/game/gameplay.md`](../modules/game/gameplay.md).

Kod M5 jest zbudowany i przetestowany na Windowsie (2026-10-05, Debug i Release bez
ostrzeżeń, 215 przypadków testowych i 85098 asercji). Na macOS nie był budowany, a
rozgrywki nikt jeszcze nie przeszedł ręcznie ([`build-windows.md`](build-windows.md),
sekcja 14). Tagu nie ma.

Z pierwszej części M6 (skybox) są: tekstura sześcienna (`src/gfx/Cubemap.*`), niebo
(`src/game/Skybox.*`), piąta para shaderów (`assets/shaders/skybox.vert` i `skybox.frag`),
nowy katalog `assets/skybox/` z sześcioma obrazami nieba i skrypt, który je generuje
(`tools/blender/make_skybox.py`), typ `assets::RowOrder` w loaderze obrazów, pole `Skybox`
i suwak `Sky brightness` w panelu Renderer oraz plik testów `tests/SkyboxTests.cpp`. Całość
opisuje [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md). Zgłoszone dla
Windowsa (2026-10-05): Debug i Release bez ostrzeżeń, 221 przypadków testowych i 85175
asercji ([`build-windows.md`](build-windows.md), sekcja 15). M6 nie jest zamknięty: terenu
i trawy ten dokument jeszcze nie opisuje, na macOS nic z M6 nie było budowane.

Docelową strukturę (z `renderer/`) opisuje PRD w sekcji 6.

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
├── .clang-tidy                 # reguły analizy statycznej, konwencja nazw, jedna dodatkowa flaga
├── .clangd                     # gdzie clangd ma szukać compile_commands.json, ta sama flaga
├── .gitattributes              # normalizacja końców linii
├── .gitignore                  # czego nie wersjonujemy
├── .vscode/                    # ustawienia obszaru roboczego Cursor i VS Code
│   ├── extensions.json         # rekomendowane rozszerzenia
│   └── settings.json           # clangd, presety CMake, formatowanie przy zapisie, GLSL
├── assets/                     # pliki wczytywane przez program w czasie działania
│   ├── fonts/                  # czcionka paneli debug (cudzy materiał, licencja OFL)
│   │   ├── AtkinsonHyperlegible-Regular.ttf  # Atkinson Hyperlegible 1.006, plik niezmieniony
│   │   ├── OFL.txt                 # licencja czcionki: SIL Open Font License 1.1
│   │   └── README.md               # skąd jest plik, wersja, suma kontrolna
│   ├── models/                 # modele OBJ z materiałami MTL (budują je skrypty z tools/blender/)
│   │   ├── crystal_a.obj/.mtl      # kryształ: jeden wysoki odłamek
│   │   ├── crystal_b.obj/.mtl      # kryształ: grupa trzech odłamków na podstawie
│   │   ├── floor_tile.obj/.mtl     # płytka podłogi 2 x 2 m
│   │   ├── gate.obj/.mtl           # drewniana brama wyjścia, w miejscu segmentu ściany
│   │   ├── wall_pillar.obj/.mtl    # słupek na rogu siatki
│   │   └── wall_straight.obj/.mtl  # segment ściany wzdłuż osi X
│   ├── shaders/                # shadery GLSL: pięć par i dwa pliki dołączane
│   │   ├── common/
│   │   │   ├── lighting.glsl       # blok świateł i funkcja computeLighting, dołączany przez #include
│   │   │   └── normal_map.glsl     # sampler mapy normalnych i funkcja surfaceNormal, dołączany przez #include
│   │   ├── color.frag              # linie pudełek i kul kolizji: jeden kolor z uColor
│   │   ├── color.vert              # linie pudełek i kul kolizji: trzy macierze, sama pozycja
│   │   ├── gouraud.frag            # scena, światło na wierzchołek: światło z wierzchołków razy tekstura
│   │   ├── gouraud.vert            # scena, światło na wierzchołek: tu liczone jest światło
│   │   ├── lit.frag                # scena, światło na fragment: tu liczone jest światło (Phong, Blinn-Phong)
│   │   ├── lit.vert                # scena, światło na fragment: pozycja, normalna i styczna w świecie
│   │   ├── skybox.frag             # niebo: odczyt tekstury sześciennej kierunkiem, jasność
│   │   ├── skybox.vert             # niebo: widok bez przesunięcia, głębia 1.0 (xyww)
│   │   ├── textured.frag           # modele bez światła: tekstura razy uTint, dwa widoki debug
│   │   └── textured.vert           # modele bez światła: trzy macierze, pozycja, normalna, uv, styczna
│   ├── skybox/                 # sześć ścian nieba, tekstura sześcienna (buduje je tools/blender/make_skybox.py)
│   │   ├── nx.png                  # ściana -X
│   │   ├── ny.png                  # ściana -Y (dół)
│   │   ├── nz.png                  # ściana -Z (przód przy yaw 0)
│   │   ├── px.png                  # ściana +X
│   │   ├── py.png                  # ściana +Y (zenit, tu jest tarcza księżyca)
│   │   └── pz.png                  # ściana +Z
│   └── textures/               # tekstury PNG: cztery obrazy koloru i ich mapy normalnych
│       ├── crystal.png
│       ├── crystal_normal.png
│       ├── floor_stone.png
│       ├── floor_stone_normal.png
│       ├── gate_wood.png
│       ├── gate_wood_normal.png
│       ├── wall_stone.png
│       └── wall_stone_normal.png
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
│   ├── assets/                 # wczytywanie plików z assets/: loadery bez OpenGL i pamięć podręczna
│   │   ├── AssetCache.hpp/.cpp     # pamięć podręczna: każdy model i tekstura raz, na karcie
│   │   ├── ImageLoader.hpp/.cpp    # plik obrazu na piksele, dolny wiersz pierwszy (albo górny: RowOrder)
│   │   ├── ObjLoader.hpp/.cpp      # parser OBJ i MTL: wierzchołki, indeksy, części, materiały
│   │   └── Tangents.hpp/.cpp       # styczne wierzchołków z pozycji i UV, dla map normalnych
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
│   │   ├── Hud.hpp/.cpp            # HUD gry: licznik kryształów, pasek baterii, podpowiedzi, karta wygranej
│   │   ├── PanelLayout.hpp/.cpp    # układ paneli przy pierwszym uruchomieniu
│   │   ├── Theme.hpp/.cpp          # motyw paneli: kolory, odstępy, czcionka, skala ekranu
│   │   └── panels/
│   │       ├── AssetsPanel.hpp/.cpp    # panel "Assets": widok, filtr, anizotropia, modele, tekstury
│   │       ├── CameraPanel.hpp/.cpp    # panel "Camera": stopy gracza, kąty, FOV, prędkości
│   │       ├── CollisionPanel.hpp/.cpp # panel "Collision": rysowanie pudełek i kul, noclip, liczby
│   │       ├── GameplayPanel.hpp/.cpp  # panel "Gameplay": stan rundy, bateria, liczby reguł, restart
│   │       ├── LightsPanel.hpp/.cpp    # panel "Lights": otoczenie, księżyc, latarka, światła punktowe, połysk
│   │       ├── MazePanel.hpp/.cpp      # panel "Maze": rozmiar, ziarno, Regenerate, plan z kryształami, bramą i wyjściem
│   │       ├── RendererPanel.hpp/.cpp  # panel "Renderer": statystyki, kolor tła, lista Lighting
│   │       └── ShadersPanel.hpp/.cpp   # panel "Shaders": pięć programów, przycisk Reload shaders
│   ├── game/                   # gra
│   │   ├── ColliderLines.hpp/.cpp  # rysowanie pudełek i kul kolizji liniami (GL_LINES)
│   │   ├── Crystals.hpp/.cpp       # kryształy: ile, w których komórkach, jak się ruszają i świecą
│   │   ├── Exit.hpp/.cpp           # wyjście: najdalsza komórka, brama, strefa wygranej
│   │   ├── GameplayRenderer.hpp/.cpp # rysowanie kryształów i bramy ich modelami
│   │   ├── LightRig.hpp/.cpp       # bufor uniformów ze światłami klatki
│   │   ├── Lighting.hpp/.cpp       # tryby i ustawienia oświetlenia, światła klatki
│   │   ├── Maze.hpp/.cpp           # labirynt: siatka komórek, ściany na krawędziach, ślepy zaułek
│   │   ├── MazeGenerator.hpp/.cpp  # generator labiryntu z ziarna, randomBelow
│   │   ├── MazeLayout.hpp/.cpp     # układ w świecie: ściany, słupki, pudełka kolizji
│   │   ├── MazeRenderer.hpp/.cpp   # rysowanie podłogi, ścian i słupków modelami
│   │   ├── MazeWorld.hpp/.cpp      # jeden labirynt w świecie: macierze modelu, pudełka, start, wyjście, kryształy
│   │   ├── ModelDraw.hpp/.cpp      # rysowanie wczytanego modelu z teksturami, wspólne dla obu rendererów
│   │   ├── NightMazeApp.hpp/.cpp   # aplikacja Night Maze: labirynt, runda, gracz, kamera, światła
│   │   ├── Player.hpp/.cpp         # gracz: pudełko, chodzenie z kolizjami, noclip
│   │   ├── Round.hpp/.cpp          # runda: kryształy, brama, bateria, czas i ich reguły
│   │   ├── ShaderUniforms.hpp      # nazwy uniformów shaderów w jednym miejscu
│   │   └── Skybox.hpp/.cpp         # niebo: tekstura sześcienna na sześcianie, rysowane na końcu klatki
│   ├── gfx/                    # opakowania obiektów OpenGL (RAII, tylko przenoszenie)
│   │   ├── Buffer.hpp/.cpp         # bufor wierzchołków albo indeksów
│   │   ├── Cubemap.hpp/.cpp        # tekstura sześcienna (sześć ścian) i obiekt samplera
│   │   ├── Mesh.hpp/.cpp           # siatka: VAO i dwa bufory jednego modelu, draw
│   │   ├── Shader.hpp/.cpp         # program shaderów z dwóch plików, reload, settery, blok uniformów
│   │   ├── ShaderSource.hpp/.cpp   # tekst shadera: #include, dyrektywy #line, nazwy plików w błędach
│   │   ├── Texture2D.hpp/.cpp      # tekstura 2D z mipmapami i obiekt samplera
│   │   ├── UniformBuffer.hpp/.cpp  # bufor uniformów: pamięć bloku uniformów na karcie
│   │   ├── Vertex.hpp              # jeden wierzchołek modelu: pozycja, normalna, uv, styczna
│   │   └── VertexArray.hpp/.cpp    # tablica wierzchołków (VAO), opis atrybutów
│   └── scene/                  # opis sceny: dane i matematyka na GLM, bez OpenGL
│       ├── Camera.hpp/.cpp         # kamera: kierunek, macierz widoku i rzutowania
│       ├── Collider.hpp/.cpp       # pudełka AABB i kule, testy nakładania, ruch ze ślizganiem
│       ├── Light.hpp/.cpp          # światła jako dane: kierunkowe, punktowe, reflektor, zanik, stożek
│       ├── LightBlock.hpp/.cpp     # bajty bloku uniformów LightBlock (std140) i packLightBlock
│       └── Transform.hpp/.cpp      # pozycja, obrót, skala, macierz modelu i macierz normalnych
├── tests/                      # testy jednostkowe (doctest): program night_maze_tests
│   ├── main.cpp                    # punkt wejścia: main() generuje doctest
│   ├── ColliderTests.cpp           # testy scene::Aabb, scene::Sphere, overlaps i moveAndSlide
│   ├── CrystalTests.cpp            # testy kryształów: liczba, komórki, ruch i blask
│   ├── ExitTests.cpp               # testy wyjścia: odległości, najdalsza komórka, brama, strefa
│   ├── ImageLoaderTests.cpp        # testy loadImage: tekstury i mapy normalnych gry, odwracanie wierszy, błędy
│   ├── LightingTests.cpp           # testy ustawień oświetlenia, usesNormalMap i buildLightSet
│   ├── LightTests.cpp              # testy zaniku, stożka reflektora i bajtów bloku świateł
│   ├── MazeGeneratorTests.cpp      # testy randomBelow i generateMaze, labirynt wzorcowy
│   ├── MazeLayoutTests.cpp         # testy układu w świecie i kolizji w labiryncie
│   ├── MazeTests.cpp               # testy klasy Maze, kierunków, isDeadEnd i MazeCell
│   ├── MazeWorldTests.cpp          # testy buildMazeWorld: macierze, pudełka, start
│   ├── ObjLoaderTests.cpp          # testy parseObj, parseMtl i loadObj
│   ├── PlayerTests.cpp             # testy gracza: chodzenie, sprint, ślizganie, noclip
│   ├── RoundTests.cpp              # testy rundy: zbieranie, brama, bateria, migotanie, wygrana
│   ├── ShaderSourceTests.cpp       # testy expandIncludes i nameSourceFiles
│   ├── SkyboxTests.cpp             # testy plików nieba: rozmiar, księżyc, gradient, granice ścian
│   ├── TangentTests.cpp            # testy triangleTangents, computeTangents i countMirroredTriangles
│   └── TransformTests.cpp          # testy macierzy normalnych
└── docs/
    ├── PRD.pdf                 # dokument wymagań
    ├── README.md               # spis treści dokumentacji i kolejność czytania
    ├── syllabus.md             # tabela: temat wykładu, dokument, pliki kodu
    ├── decisions/              # notatki "dlaczego tak, a nie inaczej"
    │   ├── README.md               # czym jest notatka, układ, lista notatek
    │   ├── battery-darkness-no-loss.md # pusta bateria to tylko ciemność, bez stanu przegranej
    │   ├── collision-aabb-sliding.md   # AABB i ślizganie zamiast silnika fizyki
    │   ├── crystal-count-and-gate-threshold.md # liczba kryształów z rozmiaru labiryntu, brama po około 70%
    │   ├── dead-end-lights.md          # światła punktowe w ślepych zaułkach (M4, zastąpiona w M5)
    │   ├── deterministic-random.md     # własna randomBelow zamiast rozkładów std
    │   ├── enemy-after-m5.md           # przeciwnik dopiero po M5
    │   ├── exit-farthest-cell.md       # wyjście w komórce najdalszej od startu, nie w rogu
    │   ├── no-gamma-until-m7.md        # bez korekcji gamma i tekstur sRGB do M7
    │   ├── painted-moon-fixed-direction.md # księżyc namalowany na niebie, w domyślnym kierunku światła
    │   ├── skybox-in-game-layer.md     # niebo jako game::Skybox, bez warstwy renderer
    │   └── tangents-on-load.md         # styczne liczone przy wczytaniu, bez znaku skrętności
    ├── guides/                 # przewodniki
    │   ├── blender.md              # modele i tekstury: skrypty Blendera, eksport OBJ
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
        │   ├── asset-cache.md          # AssetCache, dwie tekstury zastępcze, panel Assets
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
        │   ├── flashlight.md           # latarka, klawisz F, bateria, LightingSettings, światła kryształów, LightRig
        │   ├── gameplay.md             # runda: wyjście, kryształy, brama, bateria, HUD, panel Gameplay
        │   ├── maze-generator.md       # labirynt, generator, układ w świecie, panel Maze
        │   ├── maze-rendering.md       # MazeWorld, MazeRenderer, ModelDraw, macierze modelu, regeneracja
        │   └── player.md               # gracz: chodzenie, noclip, oczy a stopy, testy
        ├── gfx/                    # moduł gfx, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, RAII i przenoszenie, warstwy, indeks
        │   ├── buffers-vao.md          # VBO, VAO, krok i przesunięcie, Buffer, VertexArray
        │   ├── cubemap.md              # tekstura sześcienna jako obiekt OpenGL, klasa Cubemap
        │   ├── indexed-drawing.md      # EBO, glDrawElements, indeksy w gfx::Mesh i w danych linii
        │   ├── mesh.md                 # Vertex, Mesh, rysowanie zakresu indeksów
        │   ├── normal-mapping.md       # mapy normalnych: przestrzeń styczna, styczne, TBN, normal_map.glsl
        │   ├── shader-class.md         # klasa Shader: kompilacja, linkowanie, błędy
        │   ├── shader-hot-reload.md    # reload, panel Shaders
        │   ├── shader-includes.md      # #include w shaderach, #line, nazwy plików w błędach
        │   ├── shaders.md              # potok, GLSL, pary shaderów color i textured
        │   ├── textures.md             # tekstury, filtry, mipmapy, samplery, Texture2D, textured.*
        │   ├── uniform-buffers.md      # bloki uniformów, std140, UniformBuffer, LightBlock
        │   └── uniforms.md             # uniformy, setMat4, setInt, setVec3
        ├── renderer/               # techniki rysowania: dokumenty tematów, kod jest w game/ i w shaderach
        │   ├── README.md               # wstęp i indeks
        │   ├── lighting-gouraud-phong.md   # Gouraud a Phong, Phong a Blinn-Phong, lit.* i gouraud.*
        │   └── skybox.md               # tekstura sześcienna, niebo, skybox.*, make_skybox.py
        ├── scene/                  # moduł scene, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, dane bez OpenGL, warstwy, indeks
        │   ├── camera-controls.md      # obrót kamery myszą, panel Camera
        │   ├── camera.md               # macierz widoku, rzutowanie, Camera
        │   ├── collision.md            # AABB, kule, ruch ze ślizganiem, linie pudełek i kul, panel Collision
        │   ├── lights.md               # rodzaje świateł, model odbicia, lighting.glsl, panel Lights
        │   └── transforms.md           # przestrzenie, macierz modelu, Transform, macierz normalnych
        └── debug-ui.md             # panele ImGui i HUD w projekcie
```

Drzewo nie pokazuje katalogu `tools/blender/`: to skrypty Pythona dla Blendera, które budują
modele, tekstury i obrazy nieba z `assets/`. Program ich nie czyta i CMake ich nie uruchamia. Opisuje je
[`blender.md`](blender.md). Pliki katalogu, z rolą z komentarza na początku każdego:

| Plik | Rola |
|---|---|
| `blender_common.py` | wspólne funkcje skryptów: czyszczenie sceny, budowanie brył, rzutowanie UV, materiał z teksturą i mapą normalnych, eksport OBJ ze stałymi opcjami, rendery do przeglądu |
| `build_wall_straight.py` | model `wall_straight`: segment ściany, 2 m długości i 3 m wysokości |
| `build_wall_pillar.py` | model `wall_pillar`: słupek na rogi siatki, który zakrywa końce segmentów |
| `build_floor_tile.py` | model `floor_tile`: podłoga jednej komórki, płaski kwadrat 2 x 2 m |
| `build_crystal.py` (M5) | dwa modele kryształów: `crystal_a`, jeden wysoki odłamek ze szpicem na obu końcach, i `crystal_b`, grupa trzech odłamków na płaskiej podstawie |
| `build_gate.py` (M5) | model `gate`: zamknięta drewniana brama, która wypełnia jeden bok komórki między dwoma słupkami, w miejscu segmentu ściany |
| `make_textures.py` | osiem tekstur 512 x 512 w `assets/textures`: obrazy koloru `wall_stone.png`, `floor_stone.png`, `gate_wood.png` i `crystal.png` oraz mapa normalnych każdego z nich |
| `make_skybox.py` (M6) | sześć obrazów nieba 1024 x 1024 w `assets/skybox`: ściany tekstury sześciennej liczone z kierunku każdego piksela (tło, Droga Mleczna, gwiazdy, księżyc) |
| `make_all.py` | wszystko naraz w jednym procesie Blendera: najpierw tekstury, potem modele, na końcu niebo |

Zapis
`floor_tile.obj/.mtl` oznacza parę plików `floor_tile.obj` i `floor_tile.mtl`.

Katalog `docs/modules/renderer/` istnieje, choć katalogu `src/renderer/` jeszcze nie ma:
dokumenty o cieniowaniu i o niebie są już pod nazwą warstwy z PRD, a kod, który opisują, leży
dziś w `src/game/` (`NightMazeApp.cpp`, `Skybox.*`) i w `assets/shaders/`.

Zapis `Window.hpp/.cpp` oznacza parę plików `Window.hpp` i `Window.cpp`. Zgodnie z zasadą z
PRD nagłówek i implementacja leżą obok siebie w `src/`, nie ma osobnego katalogu `include/`. Pliki konfiguracyjne z katalogu głównego są
wypisane na początku drzewa, przed katalogami.

### Katalogi

| Katalog | Rola | Kto pisze kod |
|---|---|---|
| `src/` | cały nasz kod C++ poza testami | my |
| `tests/` | testy jednostkowe: osobny program `night_maze_tests`, który woła kod z bibliotek `engine` i `game_logic` i sprawdza wyniki ([`../libraries/doctest.md`](../libraries/doctest.md)) | my |
| `assets/` | pliki, które program wczytuje w czasie działania: shadery GLSL (z podkatalogiem `shaders/common/` na pliki dołączane), modele OBJ z materiałami MTL, tekstury PNG, sześć obrazów nieba (`skybox/`, od M6) i czcionka paneli debug. Nie są kompilowane razem z programem. Krok budowania umieszcza katalog obok pliku wykonywalnego (sekcja 3.1, blok 7) | my, z jednym wyjątkiem: `assets/fonts/` to cudzy materiał. Czcionka Atkinson Hyperlegible (Braille Institute of America) jest na licencji SIL Open Font License 1.1, a plik licencji `OFL.txt` leży obok niej i musi tam zostać |
| `cmake/` | pomocnicze pliki CMake dołączane przez `include(...)` | my |
| `external/` | cudzy kod trzymany w repozytorium (`glad/`) oraz plik, który kompiluje pobraną bibliotekę stb_image (`stb/`) | `glad/`: generator GLAD, nie edytujemy. `stb/`: dwa małe pliki napisane ręcznie |
| `docs/` | dokumentacja do nauki | my |
| `build/` | wszystko, co powstaje podczas budowania | CMake i kompilator, poza Gitem |

### Pliki źródłowe

| Plik | Co zawiera | Dokument |
|---|---|---|
| `src/main.cpp` | `main()` z obsługą wyjątków oraz klasę `DebugNightMazeApp`, która dokłada do gry interfejs debugowy i HUD | [`../modules/debug-ui.md`](../modules/debug-ui.md) |
| `src/core/Application.*` | `core::Application`: posiada `Window`, `Input`, `Time`, prowadzi pętlę główną, obsługuje Esc (zwolnienie przechwyconego kursora albo zamknięcie programu) | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Window.*` | `core::Window`: inicjalizacja GLFW, okno, kontekst 4.1 Core, `gladLoadGL`, vsync | [`../modules/core/window-context.md`](../modules/core/window-context.md), [`../libraries/glfw.md`](../libraries/glfw.md) |
| `src/core/Input.*` | `core::Input`: odpytywanie klawiszy i myszy. Klawiatura: `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`. Mysz: `isMouseButtonDown`, `wasMouseButtonPressed`, `mouseDeltaX`, `mouseDeltaY`, `setMouseBlocked`, `setCursorCaptured`, `isCursorCaptured` | [`../modules/core/input.md`](../modules/core/input.md) |
| `src/core/Time.*` | `core::Time`: delta czasu, akumulator stałego kroku (`FIXED_DT`), uśrednione FPS | [`../modules/core/main-loop.md`](../modules/core/main-loop.md) |
| `src/core/Log.*` | `logInfo`, `logWarn`, `logError` | [`../modules/core/window-context.md`](../modules/core/window-context.md) |
| `src/core/GlCheck.*` | makro `GL_CHECK` i funkcja `checkGlErrors` | [`../modules/core/gl-check.md`](../modules/core/gl-check.md) |
| `src/core/Paths.*` | `core::executableDir` i `core::assetPath`: ścieżki do plików z `assets/` liczone od położenia pliku wykonywalnego. `core::pathText`: ścieżka jako tekst UTF-8 do logu i do paneli. `Paths.cpp` to jedyny plik w `src/` z kodem zależnym od systemu (`#if` dla macOS i Windows). Wołają je konstruktor `game::NightMazeApp` (ścieżki shaderów), konstruktory `game::MazeRenderer` i `game::GameplayRenderer` (ścieżki modeli), `assets::AssetCache` (`pathText` w logu) i panele Shaders oraz Assets | [`../modules/core/paths.md`](../modules/core/paths.md) |
| `src/gfx/Shader.*` | `gfx::Shader`: obiekt programu OpenGL zbudowany z pliku shadera wierzchołków i pliku shadera fragmentów. `reload` (przy błędzie zostaje stary program), `isValid`, `use`, `setMat4` (uniform typu `mat4`, przez `glGetUniformLocation` i `glUniformMatrix4fv`), `setInt` (uniform typu `int` albo sampler, `glUniform1i`), `setVec3` (uniform typu `vec3`, `glUniform3fv`), `setMat3` (uniform typu `mat3`, `glUniformMatrix3fv`: macierz normalnych), `setFloat` (uniform typu `float`, `glUniform1f`), `bindUniformBlock` (łączy blok uniformów z punktem wiązania przez `glGetUniformBlockIndex` i `glUniformBlockBinding`, zapamiętuje prośbę w strukturze `gfx::UniformBlockBinding` i powtarza ją po każdym `reload`, porównuje rozmiar bloku według sterownika z rozmiarem z C++), `lastError`, `vertexPath`, `fragmentPath`. Przed kompilacją rozwija dyrektywy `#include` (`gfx::expandIncludes`), a w komunikacie błędu zamienia numer napisu źródłowego na nazwę pliku (`gfx::nameSourceFiles`). RAII, tylko przenoszenie. `NightMazeApp` ma cztery takie obiekty (`textured`, `color`, `lit`, `gouraud`; piąty, `basic`, zniknął w M5 razem z kostką), a panel "Shaders" woła `reload` na każdym. `setInt` ustawia samplery `uTexture` i `uNormalMap`, tryb `uViewMode`, przełącznik `uNormalMapEnabled` i wzór połysku `uSpecularModel`, `setVec3` kolory `uTint`, `uColor` i (od M5) `uEmissive`, `setFloat` uniformy `uSpecularStrength` i `uShininess`, `setMat3` uniform `uNormalMatrix` | [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), settery w [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), `reload` w [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), `bindUniformBlock` w [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md), `#include` w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md), wstęp do warstwy w [`../modules/gfx/README.md`](../modules/gfx/README.md) |
| `src/gfx/ShaderSource.*` | typ `gfx::IncludeReader` (funkcja, która podaje treść dołączanego pliku), struktura `gfx::ShaderSource` (tekst po rozwinięciu i lista plików, z których powstał) oraz funkcje `gfx::expandIncludes` (zastępuje linie `#include "..."` treścią plików, także zagnieżdżone, i dopisuje wokół nich dyrektywy `#line`; zwraca błąd dla brakującego pliku, pliku dołączającego samego siebie, źle zapisanej linii, `#include` przed `#version` i `#version` w pliku dołączanym) i `gfx::nameSourceFiles` (w dzienniku sterownika zamienia numer napisu źródłowego na nazwę pliku, dla formatu NVIDII `1(15)` i formatu `ERROR: 1:15:`, a przy więcej niż jednym pliku dopisuje linię `Source files: ...`). Sama praca na tekście: nie otwiera plików i nie woła OpenGL, nagłówek nie dołącza GLAD, więc ma testy jednostkowe. Woła je `gfx::Shader` | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `src/gfx/UniformBuffer.*` | `gfx::UniformBuffer`: jeden bufor OpenGL używany jako bufor uniformów, czyli pamięć bloku uniformów, który czyta kilka programów. Konstruktor tworzy bufor o podanym rozmiarze (`glGenBuffers`, `glBindBuffer` i `glBufferData` z celem `GL_UNIFORM_BUFFER` i wskazówką `GL_DYNAMIC_DRAW`) i wpina go w punkt wiązania (`glBindBufferBase`). `update` kopiuje bajty na początek bufora (`glBufferSubData`) i loguje błąd, gdy danych jest więcej niż miejsca. Akcesory `bindingPoint` i `sizeInBytes`. RAII, tylko przenoszenie. Klasa przenosi same bajty i nie wie, co znaczą. Posiada ją `game::LightRig`. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/gfx/Buffer.*` | `gfx::Buffer`: jeden bufor OpenGL wypełniany raz w konstruktorze (`glGenBuffers`, `glBindBuffer`, `glBufferData` z `GL_STATIC_DRAW`), cel `GL_ARRAY_BUFFER` albo `GL_ELEMENT_ARRAY_BUFFER`, `bind`. RAII, tylko przenoszenie. Używa jej `gfx::Mesh` (do M4 także kostka w `NightMazeApp`, usunięta w M5) | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/VertexArray.*` | `gfx::VertexArray`: jeden obiekt tablicy wierzchołków (VAO), wiązany już w konstruktorze, `bind`, `setFloatAttribute` (`glEnableVertexAttribArray`, `glVertexAttribPointer`). RAII, tylko przenoszenie. Używa jej `gfx::Mesh` (do M4 także kostka w `NightMazeApp`, usunięta w M5) | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/Texture2D.*` | typ `gfx::TextureFilter` (`Nearest`, `Bilinear`, `Trilinear`) i klasa `gfx::Texture2D`: jedna tekstura 2D z pełnym łańcuchem mipmap i jej obiekt samplera. Konstruktor przyjmuje surowe bajty (szerokość, wysokość, 3 albo 4 kanały, wskaźnik, dolny wiersz pierwszy) i woła `glTexImage2D` oraz `glGenerateMipmap`. `bind(unit)` wiąże teksturę i sampler z jednostką teksturującą, `setFilter` i `setAnisotropy` zmieniają próbkowanie w działającym programie, akcesory `filter`, `anisotropy`, `maxAnisotropy`, `id`, `width`, `height`, `isValid`. Filtrowanie anizotropowe jest wykrywane jako rozszerzenie. RAII, tylko przenoszenie. Tworzy ją i trzyma `assets::AssetCache`, wiąże `game::drawModel` (`src/game/ModelDraw.cpp`), a jej `id()` czyta podgląd w panelu Assets. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/textures.md`](../modules/gfx/textures.md) |
| `src/gfx/Cubemap.*` | klasa `gfx::Cubemap` (M6): jedna tekstura sześcienna i jej obiekt samplera. Konstruktor przyjmuje bok ściany, liczbę kanałów (3 albo 4) i sześć wskaźników na surowe bajty w kolejności +X, -X, +Y, -Y, +Z, -Z (typ `FacePixels`, górny wiersz pierwszy), woła `glTexImage2D` raz na ścianę, ustawia `GL_TEXTURE_MAX_LEVEL` na 0 (bez mipmap) i daje samplerowi filtr liniowy i `GL_CLAMP_TO_EDGE` na trzech osiach. `bind(unit)` wiąże teksturę i sampler z jednostką, akcesory `isValid`, `id`, `size`, konstruktor domyślny daje obiekt bez tekstury. RAII, tylko przenoszenie. Tworzy ją i trzyma `game::Skybox`. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/cubemap.md`](../modules/gfx/cubemap.md) |
| `src/gfx/Vertex.hpp` | `gfx::Vertex`: jeden wierzchołek modelu jako struktura (pola `position`, `normal`, `uv`, `tangent`: 11 liczb `float`, 44 bajty), stałe `POSITION_COMPONENTS`, `NORMAL_COMPONENTS`, `UV_COMPONENTS`, `TANGENT_COMPONENTS` i numery atrybutów `POSITION_ATTRIBUTE` (0), `NORMAL_ATTRIBUTE` (1), `UV_ATTRIBUTE` (2), `TANGENT_ATTRIBUTE` (3), dwa `static_assert` (rozmiar bez dopełnienia, układ standardowy). Sam nagłówek, bez GLAD. Używają jej `gfx::Mesh`, loader OBJ i testy | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcja 5.2 |
| `src/gfx/Mesh.*` | `gfx::Mesh`: siatka jednego modelu na karcie. Posiada `VertexArray`, bufor wierzchołków i bufor indeksów, w konstruktorze wysyła dane ze `std::span` i opisuje cztery atrybuty przez `sizeof(Vertex)` i `offsetof`. `draw()` rysuje całość, `draw(firstIndex, indexCount)` zakres indeksów (`glDrawElements`), prymityw jest parametrem konstruktora (domyślnie `GL_TRIANGLES`). RAII przez pola, tylko przenoszenie. Tworzą ją `assets::AssetCache` (siatka modelu z trójkątów, rysowana zakresami) i `game::ColliderLines` (dwie siatki z `GL_LINES`: krawędzie sześcianu jednostkowego, 8 wierzchołków i 24 indeksy, oraz okrąg jednostkowy, 32 wierzchołki i 64 indeksy). Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcje od 5.3 do 5.5 |
| `src/assets/ObjLoader.*` | struktury `assets::ObjPart`, `assets::ObjMaterial`, `assets::ObjModel` i funkcje `assets::parseObj` (tekst OBJ na wierzchołki, indeksy i części), `assets::parseMtl` (tekst MTL na materiały: `newmtl`, `Kd`, `map_Kd` i linia mapy normalnych `map_Bump` z opcją `-bm`) oraz `assets::loadObj` (plik OBJ razem z plikami MTL, ścieżki tekstur względem pliku MTL). `parseObj` na końcu liczy styczne wierzchołków (`assets::computeTangents`) i zapisuje liczbę trójkątów z lustrzaną teksturą w `ObjModel::mirroredTriangleCount`, a `loadObj` ostrzega w logu, gdy jest większa od zera. Ręcznie napisany parser, bez OpenGL i bez wyjątków: wynik `bool` i tekst błędu z numerem linii. `loadObj` woła `assets::AssetCache::model` | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), wstęp do warstwy w [`../modules/assets/README.md`](../modules/assets/README.md) |
| `src/assets/Tangents.*` | funkcje `assets::triangleTangents` (styczna i bitangenta jednego trójkąta z krawędzi i różnic UV), `assets::computeTangents` (styczna każdego wierzchołka: suma po trójkątach, ortogonalizacja Grama-Schmidta względem normalnej, długość 1, wartość zastępcza zamiast `NaN`) i `assets::countMirroredTriangles` (trójkąty, na których `cross(N, T)` wskazuje przeciwnie do bitangenty). Sama matematyka na GLM, bez OpenGL. Woła je `assets::parseObj` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcje od 5.5 do 5.7, notatka [`../decisions/tangents-on-load.md`](../decisions/tangents-on-load.md) |
| `src/assets/ImageLoader.*` | struktura `assets::Image` (szerokość, wysokość, liczba kanałów, bajty pikseli z dolnym wierszem jako pierwszym), od M6 typ `assets::RowOrder` (`BottomFirst` dla tekstur 2D, domyślny, i `TopFirst` dla ścian tekstury sześciennej, których wierszy się nie odwraca) i funkcja `assets::loadImage`: czyta plik w trybie binarnym, dekoduje go biblioteką stb_image i odwraca kolejność wierszy. Wynik `bool`, tekst błędu przez referencję, jedno logowanie, bez wyjątków. Bez OpenGL. Jedyny plik projektu, który dołącza `<stb_image.h>`. `loadImage` woła `assets::AssetCache::texture` | [`../modules/assets/images.md`](../modules/assets/images.md) |
| `src/assets/AssetCache.*` | struktury `assets::LoadedTexture`, `assets::ModelPart`, `assets::LoadedModel` i klasa `assets::AssetCache`: wczytuje każdy model (`model`) i każdą teksturę (`texture`) raz, pod kluczem będącym uporządkowaną ścieżką, trzyma je w `std::deque` (wskaźniki pozostają ważne), pamięta ścieżki, których nie udało się wczytać (`failedPaths`), ma dwie tekstury zastępcze 1 x 1 (białą `whiteTexture` i płaską mapę normalnych `flatNormalTexture`) oraz `setFilter` i `setAnisotropy` dla wszystkich tekstur naraz. Każda część modelu (`ModelPart`) ma teksturę koloru i mapę normalnych, nigdy puste. Jedyny plik `src/assets/`, który tworzy obiekty OpenGL (przez `gfx::Mesh` i `gfx::Texture2D`), więc nie ma testu jednostkowego. Posiada ją `NightMazeApp` | [`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md) |
| `external/stb/stb_image.c` | jedyny plik, w którym kompiluje się implementacja stb_image: makro `STB_IMAGE_IMPLEMENTATION` i dołączenie nagłówka pobranego przez FetchContent. Plik C, poza naszymi ostrzeżeniami, tworzy bibliotekę `stb_image` | [`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2 |
| `src/scene/Transform.*` | `scene::Transform`: struktura z publicznymi polami `position`, `rotationDegrees` (kąty Eulera w stopniach) i `scale` oraz funkcją `matrix()`, która zwraca macierz modelu `T * Ry * Rx * Rz * S`. Od M4 także funkcja `scene::normalMatrix`: macierz normalnych, czyli odwrotność lewej górnej części 3 x 3 macierzy modelu po transpozycji (`glm::mat3`). Sama matematyka na GLM, bez OpenGL. Struktury używają `game::buildMazeWorld` (macierze podłogi, ścian i słupków, przez `placedAt` i `wallModelMatrix`), `game::GameplayRenderer` (macierz każdego kryształu: pozycja, która się kołysze, i obrót wokół osi Y) i `game::ColliderLines` (macierz każdego pudełka i każdego okręgu kuli), a `normalMatrix` woła `game::drawModel` dla każdego rysowanego obiektu | [`../modules/scene/transforms.md`](../modules/scene/transforms.md), wstęp do warstwy w [`../modules/scene/README.md`](../modules/scene/README.md) |
| `src/scene/Light.*` | światła jako zwykłe dane i ich matematyka: stała `MAX_POINT_LIGHTS` (16), struktury `scene::Attenuation` (trzy składniki zaniku z odległością), `DirectionalLight` (księżyc), `PointLight`, `SpotLight` (latarka, dwa kąty stożka), `LightSet` (wszystkie światła jednej klatki: otoczenie, światło kierunkowe, tablica świateł punktowych z licznikiem, reflektor z wyłącznikiem) i `ConeCosines`, stałe `BRIGHTNESS_AT_RADIUS` (0,05) i `MIN_CONE_COSINE_GAP`, funkcje `attenuationForRadius` (składniki 1, 2 / r i 17 / r^2), `attenuationFactor`, `coneCosines`, `spotFactor` i `directionFromAngles`. Bez OpenGL: te same wzory liczy shader, funkcje C++ przygotowują dane i służą testom | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `src/scene/LightBlock.*` | obraz bloku uniformów `LightBlock` w C++: struktury `scene::PointLightData` (3 razy `glm::vec4`, 48 bajtów) i `scene::LightBlockData` (928 bajtów, pola w kolejności deklaracji w GLSL, z jawnym wypełnieniem po liczniku świateł), asercje `static_assert` z `offsetof` i `sizeof`, które pilnują układu `std140` w czasie kompilacji, oraz funkcja `scene::packLightBlock` (wypełnia blok z `LightSet` i pozycji oka: normalizuje kierunki, zamienia kąty stożka na cosinusy, ogranicza liczbę świateł punktowych). Bez OpenGL. Woła ją `game::LightRig::upload` | [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/scene/Camera.*` | `scene::Camera`: struktura z publicznymi polami `position`, `yawDegrees`, `pitchDegrees`, `fovDegrees`, `nearPlane`, `farPlane`, stałymi `WORLD_UP` i `MAX_PITCH_DEGREES` oraz funkcjami `forward`, `right`, `rotate`, `viewMatrix`, `projectionMatrix`. Sama matematyka na GLM, bez OpenGL i bez wejścia. Używają jej `NightMazeApp` (pole `m_camera`) i `game::Player::update` (jako kalkulator kierunków `forward` i `right`) | [`../modules/scene/camera.md`](../modules/scene/camera.md) |
| `src/scene/Collider.*` | `scene::Aabb` (pudełko o ścianach równoległych do osi: pola `min` i `max`, funkcja `fromCenter`), stała `CONTACT_TOLERANCE`, funkcje `scene::overlaps` dla dwóch pudełek (czy na siebie nachodzą) i `scene::moveAndSlide` (o ile wolno przesunąć pudełko wśród przeszkód, oś po osi, ze ślizganiem po ścianach). Od M5 także drugi kształt: `scene::Sphere` (pola `center` i `radius`), `overlaps` dla dwóch kul (kwadrat odległości środków mniejszy od kwadratu sumy promieni), `overlaps` dla kuli i pudełka oraz `closestPoint` (punkt pudełka najbliższy danemu punktowi, przez `glm::clamp`). Kule, które się tylko stykają, nie nachodzą na siebie. Sama matematyka na GLM, bez OpenGL i bez wejścia. Pudełka tworzą `game/MazeLayout`, `game::exitZone` i `game::Player::box`, `moveAndSlide` woła `game::Player::update` w każdym kroku chodzenia, a kule tworzą `game::playerReach` i zbieranie kryształów w `game::updateRound` | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `src/game/Maze.*` | typ `game::Direction` (North, East, South, West), stałe `DIRECTION_COUNT` i `ALL_DIRECTIONS`, funkcje `opposite`, `columnStep`, `rowStep`, struktura `game::MazeCell` (komórka jako para `x`, `z`, z porównaniem `==`, od M5), klasa `game::Maze`: siatka komórek ze ścianami na krawędziach (`width`, `height`, `contains`, `hasWall`, `removeWall`, stała `MAX_SIZE`) i funkcja `isDeadEnd` (komórka ze ścianami z dokładnie trzech stron, w M5 przeniesiona tu z `Lighting.*`). Bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| `src/game/MazeGenerator.*` | `game::randomBelow` (losowa liczba poniżej granicy, taka sama na każdym systemie) i `game::generateMaze` (labirynt doskonały z rozmiaru i ziarna, algorytm recursive backtracker z własnym stosem). Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 5.4 i 5.5 |
| `src/game/MazeLayout.*` | stałe wymiarów w metrach (`CELL_SIZE`, `WALL_LENGTH`, `WALL_HEIGHT`, `PILLAR_SIZE`, `WALL_VISUAL_THICKNESS`, `WALL_COLLISION_THICKNESS`, `PILLAR_HEIGHT`), typy `WallAxis` i `WallSegment`, funkcje `cellCenter`, `wallSegments`, `wallSegmentOn` (segment na wskazanym boku komórki, także otwartym: tak powstaje brama, od M5), `pillarPositions`, `wallBox`, `pillarBox`, `mazeColliders`. Część biblioteki `game_logic`. Woła je `game::buildMazeWorld` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5 |
| `src/game/MazeWorld.*` | stałe `DEFAULT_MAZE_WIDTH`, `DEFAULT_MAZE_HEIGHT` (10) i `DEFAULT_MAZE_SEED` (1), struktura `game::MazeSettings` (prośba o następny labirynt: rozmiar, ziarno, flaga `regenerate`), struktura `game::MazeWorld` (labirynt, ziarno, segmenty ścian, słupki, macierze modelu podłogi, ścian i słupków, pudełka kolizji ścian i słupków w `colliders`, pozycja startu, yaw startu, a od M5 komórka wyjścia `exitCell` i jej środek `exitPosition`, brama `hasGate`, `gate` i jej pudełko `gateBox`, strefa wyjścia `exitZone` i lista kryształów `crystals`), funkcje `yawTowards`, `wallModelMatrix` (macierz modelu segmentu ściany, używana także dla bramy) i `buildMazeWorld`. Pola z pozycjami świateł punktowych, które było tu w M4, już nie ma. Zwykłe dane bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), wyjście i kryształy w [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `src/game/Exit.*` | stałe `UNREACHABLE` (-1) i `EXIT_ZONE_HALF_SIZE` (0,5 m), funkcje `game::passageDistances` (dla każdej komórki liczba przejść na najkrótszej drodze od startu, przeszukiwanie wszerz), `farthestCell` (komórka najdalsza od startu, przy remisie pierwsza w kolejności wierszy), struktura `ExitPlacement` (`cell`, `hasGate`, `gate`), `placeExit` (wyjście w najdalszej komórce, brama na jej pierwszym otwartym boku w kolejności `ALL_DIRECTIONS`) i `exitZone` (pudełko 1 x 1 m na środku komórki wyjścia, wysokie jak ściany). Bez OpenGL. Część biblioteki `game_logic`. Woła je `game::buildMazeWorld` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), notatka [`../decisions/exit-farthest-cell.md`](../decisions/exit-farthest-cell.md) |
| `src/game/Crystals.*` | stałe `CELLS_PER_CRYSTAL` (8), `CRYSTAL_VARIANT_COUNT` (2) i stałe ruchu i blasku kryształu (`CRYSTAL_HEIGHT`, `CRYSTAL_FLOAT_HEIGHT`, `CRYSTAL_BOB_AMPLITUDE`, `CRYSTAL_BOB_SECONDS`, `CRYSTAL_SPIN_DEGREES_PER_SECOND`, `CRYSTAL_LIGHT_CLEARANCE`, `CRYSTAL_PULSE_DEPTH`, `CRYSTAL_PULSE_SECONDS`, `CRYSTAL_GLOW_STRENGTH`), struktura `game::CrystalSpawn` (komórka i wariant modelu), funkcje `crystalCountFor` (jeden kryształ na osiem komórek, od 1 do `scene::MAX_POINT_LIGHTS`), `placeCrystals` (komórki kryształów z ziarna: nigdy start ani wyjście, najpierw ślepe zaułki), `crystalRestPosition`, `crystalCenter`, `crystalLightPosition`, `crystalBobPosition`, `crystalSpinDegrees`, `crystalPulse` i `crystalGlow` (wartość uniformu `uEmissive`). Bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), notatka [`../decisions/crystal-count-and-gate-threshold.md`](../decisions/crystal-count-and-gate-threshold.md) |
| `src/game/Round.*` | stałe `GATE_OPEN_SECONDS` (1,5), `GATE_SINK_DEPTH` (3,3 m), `PLAYER_REACH_HEIGHT` i `PLAYER_REACH_RADIUS`, struktura `game::GameplaySettings` (liczby reguł do zmiany w działającej grze: `requiredFraction`, `batteryLifetimeSeconds`, `batteryPerCrystal`, `lowBatteryThreshold`, `pickupRadius`, `batteryDrains` i prośba `restart`), typ `RoundState` (`Playing`, `Won`: stanu przegranej nie ma), struktury `RoundCrystal` i `Round` (stan jednej rundy: kryształy, liczniki, brama, bateria, dwa zegary), funkcje `requiredCrystalCount`, `startRound`, `playerReach`, `updateRound` (jeden stały krok reguł: zegary, opadanie bramy, bateria, zbieranie, otwarcie bramy, wygrana, wyłączenie latarki przy pustej baterii), `gateBlocks`, `gateVisible`, `gateSinkDepth`, `roundObstacles`, `flashlightFlicker`, `lightingForFrame` i `crystalLightPositions`. Bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), bateria i światła w [`../modules/game/flashlight.md`](../modules/game/flashlight.md), notatka [`../decisions/battery-darkness-no-loss.md`](../decisions/battery-darkness-no-loss.md) |
| `src/game/Lighting.*` | typy `game::LightingMode` (`Unlit`, `Gouraud`, `Phong`, `BlinnPhong`: pozycje listy `Lighting` w panelu Renderer) i `game::SpecularModel` (wartości uniformu `uSpecularModel`), funkcja `specularModelOf`, struktura `game::LightingSettings` (wszystko, co da się zmienić w działającej grze: tryb, światło otoczenia, kąty, kolor i natężenie księżyca, wyłącznik, kolor, natężenie, stożek i zasięg latarki, kolor, natężenie i promień świateł punktowych, siła i wykładnik połysku), pole `normalMapping` (przełącznik map normalnych, startowo włączony) i funkcja `usesNormalMap` (włączone i tryb inny niż `Gouraud`) oraz `buildLightSet` (światła jednej klatki z ustawień, oka, kierunku patrzenia i listy pozycji świateł punktowych, którą od M5 podaje `game::crystalLightPositions`). Kod świateł w ślepych zaułkach z M4 (stała wysokości światła i funkcja wybierająca zaułki) został usunięty w M5, a `isDeadEnd` przeniesiona do `Maze.*`. Zwykłe dane i matematyka bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `src/game/Player.*` | struktura `game::PlayerInput` (siedem pól `bool`: czego gracz chce w jednym kroku) i struktura `game::Player`: stałe ciała i prędkości (`BODY_WIDTH`, `BODY_HEIGHT`, `EYE_HEIGHT`, `WALK_SPEED`, `SPRINT_SPEED`, `FLY_SPEED`, `FLOOR_Y`), pola `position` (stopy), `noclip`, `walkSpeed`, `sprintSpeed`, `flySpeed`, funkcje `box`, `eyePosition` i `update` (jeden stały krok: chodzenie przez `scene::moveAndSlide` albo lot bez kolizji). Bez OpenGL, bez klawiatury i bez zegara. Część biblioteki `game_logic` | [`../modules/game/player.md`](../modules/game/player.md) |
| `src/game/MazeRenderer.*` | typ `game::ViewMode` (`Textured`, `Normals`, `Uvs`: wartości uniformu `uViewMode`) i klasa `game::MazeRenderer`: prosi pamięć podręczną o trzy modele labiryntu i rysuje `MazeWorld`, jedno wywołanie rysujące na obiekt. Program shaderów dostaje z zewnątrz: `textured` albo jeden z dwóch oświetlonych (`lit`, `gouraud`). Ustawia `uEmissive` na czerń (kamień sam nie świeci) i woła `game::drawModel` dla podłogi, ścian i słupków. Niczego nie posiada. Część programu `night_maze` (potrzebuje kontekstu OpenGL) | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `src/game/ModelDraw.*` | dwie funkcje wspólne dla `MazeRenderer` i `GameplayRenderer`: `game::setModelSamplers` (mówi samplerom `uTexture` i `uNormalMap`, z których jednostek teksturujących czytać: 0 i 1) i `game::drawModel` (rysuje jeden model raz dla każdej macierzy z listy: dla każdej części modelu wiąże mapę normalnych i obraz koloru i wysyła `uTint`, dla każdego obiektu wysyła `uModel` i `uNormalMatrix`, czyli `scene::normalMatrix` liczoną na procesorze w każdej klatce). W M4 ten kod był prywatną funkcją klasy `MazeRenderer`. Część programu `night_maze` | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `src/game/GameplayRenderer.*` | klasa `game::GameplayRenderer`: prosi pamięć podręczną o dwa modele kryształów i model bramy i rysuje to, co zmienia się w rundzie. Bramę rysuje macierzą `wallModelMatrix` z pozycją obniżoną o `gateSinkDepth`, dopóki `gateVisible` jest prawdą, z `uEmissive` równym czerni. Każdy niezebrany kryształ rysuje własną macierzą (`crystalBobPosition`, `crystalSpinDegrees`) z `uEmissive` równym `crystalGlow`. Program shaderów jest ten sam, którym narysowano labirynt. Niczego nie posiada. Część programu `night_maze` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `src/game/LightRig.*` | klasa `game::LightRig`: strona OpenGL oświetlenia. Posiada bufor uniformów (`gfx::UniformBuffer` o rozmiarze `scene::LightBlockData`, punkt wiązania `LIGHT_BLOCK_BINDING_POINT`) i nic więcej. `connect` łączy blok `LightBlock` programu z tym buforem (`Shader::bindUniformBlock`), `upload` pakuje `scene::LightSet` (`scene::packLightBlock`) i kopiuje bajty do bufora raz na klatkę. Siatki kostki znacznika i funkcji, która w M4 rysowała te kostki, już nie ma: widocznym źródłem każdego światła punktowego jest kryształ, rysowany przez `game::GameplayRenderer`. Część programu `night_maze` | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `src/game/ColliderLines.*` | klasa `game::ColliderLines`: posiada dwie siatki z `GL_LINES`, 12 krawędzi sześcianu o boku 1 (`UNIT_CUBE_CORNERS`, `UNIT_CUBE_EDGES`) i okrąg o promieniu 1 (`CIRCLE_SEGMENTS` równe 32). `draw` rysuje dowolną listę pudełek `scene::Aabb` w jednym kolorze, każde powiększone o 1 cm. `drawSpheres` (od M5) rysuje każdą kulę `scene::Sphere` jako trzy okręgi: jeden poziomy i dwa pionowe (`CIRCLE_ROTATIONS`). Część programu `night_maze` | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `src/game/Skybox.*` | struktura `game::SkyboxSettings` (`enabled`, `brightness`) i klasa `game::Skybox` (M6): niebo. Konstruktor wczytuje sześć plików z `assets/skybox/` loaderem obrazów z `RowOrder::TopFirst`, sprawdza, że są kwadratami jednej wielkości, tworzy z nich `gfx::Cubemap` i siatkę sześcianu (`gfx::Mesh`, 8 wierzchołków, 36 indeksów). `draw` wybiera program `skybox`, ustawia jego pięć uniformów, wiąże teksturę sześcienną z jednostką 0, włącza `GL_TEXTURE_CUBE_MAP_SEAMLESS`, na czas jednego wywołania rysującego ustawia test głębi `GL_LEQUAL` i wyłącza zapis głębi, a potem przywraca `GL_LESS` i zapis. Gdy plików albo programu brakuje, nic nie rysuje. Wymaga kontekstu OpenGL, więc jest w programie, a nie w `game_logic`, i nie ma testu jednostkowego | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md) |
| `src/game/ShaderUniforms.hpp` | nazwy uniformów jako stałe, szesnaście: `MODEL_UNIFORM`, `VIEW_UNIFORM`, `PROJECTION_UNIFORM`, `TEXTURE_UNIFORM`, `TINT_UNIFORM`, `EMISSIVE_UNIFORM` (od M5), `NORMAL_MAP_UNIFORM`, `NORMAL_MAP_ENABLED_UNIFORM`, `VIEW_MODE_UNIFORM`, `NORMAL_MATRIX_UNIFORM`, `SPECULAR_MODEL_UNIFORM`, `SPECULAR_STRENGTH_UNIFORM`, `SHININESS_UNIFORM`, `COLOR_UNIFORM`, a od M6 `SKYBOX_UNIFORM` i `SKYBOX_BRIGHTNESS_UNIFORM`. Do tego nazwa bloku uniformów `LIGHT_BLOCK_NAME` (`"LightBlock"`) i jego punkt wiązania `LIGHT_BLOCK_BINDING_POINT` (1, typu `GLuint`, stąd `<glad/gl.h>` w nagłówku). Sam nagłówek, wspólny dla `NightMazeApp`, `MazeRenderer`, `GameplayRenderer`, `ModelDraw`, `ColliderLines`, `LightRig` i `Skybox` | [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), blok w [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/game/NightMazeApp.*` | `game::NightMazeApp`: kolor czyszczenia (`{0.01F, 0.015F, 0.04F}`, od M6 tło tylko wtedy, gdy niebo jest wyłączone), pięć programów shaderów (`textured`, `color`, `lit`, `gouraud`, `skybox`), pamięć podręczna assetów, `MazeRenderer`, `GameplayRenderer`, `ColliderLines`, `LightRig`, `Skybox` z ustawieniami `SkyboxSettings`, `MazeSettings` i `MazeWorld`, liczby reguł `GameplaySettings` i stan rundy `Round`, lista przeszkód rundy `m_obstacles`, gracz z pozycją sprzed ostatniego kroku, `scene::Camera`, ustawienia oświetlenia `LightingSettings`, tryb widoku, przełącznik rysowania kształtów kolizji i czułość myszy. Konstruktor wczytuje shadery i modele, łączy oba oświetlone programy z buforem świateł (`m_lightRig.connect`), buduje pierwszy labirynt (10 na 10, ziarno 1) i woła `beginRound`, która zaczyna rundę (`startRound`), buduje listę przeszkód (`roundObstacles`), włącza latarkę i stawia gracza na starcie. `onUpdate` w każdym stałym kroku zbiera klawisze do `PlayerInput` (tylko przy przechwyconym kursorze), woła `Player::update` z listą przeszkód rundy, ustawia kamerę w oczach gracza i woła `updateRound` (bateria, zbieranie kryształów, brama, wyjście). Gdy brama właśnie się otworzyła, buduje listę przeszkód od nowa. `onRender` buduje nowy labirynt, gdy panel o to poprosił (`regenerateMaze`), zaczyna rundę od nowa po klawiszu R albo po prośbie panelu (`beginRound`), obsługuje klawisze N (noclip) i F (latarka), przechwytuje kursor po kliknięciu w scenę i obraca kamerę myszą, ustawia viewport, włącza test głębi, czyści kolor i głębię, liczy proporcje z rozmiaru framebuffera i pozycję oka między dwoma krokami symulacji, buduje światła klatki (`lightingForFrame`, `crystalLightPositions`, `buildLightSet`) i wysyła je do bufora uniformów (`m_lightRig.upload`, w każdej klatce, także w trybie `Unlit`), a potem rysuje części klatki: `drawMaze` (bez światła w `drawUnlitMaze` programem `textured`, gdy tryb to `Unlit` albo wybrany jest widok debug, w pozostałych przypadkach `drawLitMaze` programem `gouraud` albo `lit`; obie funkcje rysują labirynt przez `MazeRenderer`, a kryształy i bramę przez `GameplayRenderer`, tym samym programem) i, gdy włączone, `drawColliderLines` (pudełka i kule programem `color`), a na samym końcu, gdy pole `Skybox` jest zaznaczone, `m_skybox.draw` (niebo programem `skybox`). W jednej klatce pracują najwyżej trzy z pięciu programów (jeden program sceny, `color` i `skybox`), w najwyżej trzech wywołaniach `use()`. Chronione akcesory `clearColor()`, `texturedShader()`, `colorShader()`, `litShader()`, `gouraudShader()`, `skyboxShader()`, `skyboxSettings()`, `lighting()`, `camera()`, `mouseSensitivity()`, `player()`, `mazeSettings()`, `mazeWorld()`, `gameplaySettings()`, `round()`, `assets()`, `viewMode()` i `drawColliders()` udostępniają stan panelom debug i HUD. Kostki z M1 (danych wierzchołków, własnych buforów, funkcji rysującej i akcesora jej programu) ani funkcji rysującej znaczniki świateł już nie ma | [`../modules/core/README.md`](../modules/core/README.md), macierze w [`../modules/scene/camera.md`](../modules/scene/camera.md), obrót kamery myszą w [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), ruch gracza w [`../modules/game/player.md`](../modules/game/player.md), rysowanie labiryntu i regeneracja w [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), runda w [`../modules/game/gameplay.md`](../modules/game/gameplay.md), latarka i światła klatki w [`../modules/game/flashlight.md`](../modules/game/flashlight.md), przełącznik trybu cieniowania w [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `assets/shaders/textured.vert`, `textured.frag` | para shaderów modeli z teksturą: atrybuty `aPosition` (0), `aNormal` (1), `aUv` (2), `aTangent` (3), czyli pola `gfx::Vertex`, trzy macierze, sampler `uTexture`, kolor `uTint`, blask własny `uEmissive` (od M5: w widoku 0 mnoży kolor przez `vec3(1.0) + uEmissive`) i tryb `uViewMode` (0: tekstura razy `uTint`, 1: normalna jako kolor, 2: współrzędne UV jako kolor). `textured.frag` dołącza `common/normal_map.glsl`, więc widok normalnych pokazuje normalną z mapy normalnych, gdy mapy są włączone. Bez oświetlenia: rysuje scenę w trybie `Unlit` i oba widoki debug w każdym trybie. Normalną liczy nadal przez `mat3(uModel)` i nie ma uniformu `uNormalMatrix`. Od M5 w `textured.vert` jest też długi komentarz o łańcuchu przestrzeni (lokalna, świata, widoku, przycięcia), który wcześniej stał w `basic.vert`. To nie są pliki C++: nie są na żadnej liście w `CMakeLists.txt`, program czyta je przy starcie i po naciśnięciu "Reload shaders". To samo dotyczy pozostałych plików shaderów poniżej | [`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 4, potok i GLSL w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md) |
| `assets/shaders/color.vert`, `color.frag` | para shaderów jednego koloru dla linii pudełek i kul kolizji: atrybut pozycji, trzy macierze i uniform `uColor`. Najprostsza para w projekcie. Do M4 rysowała też kostki oznaczające światła | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 4, [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md) |
| `assets/shaders/lit.vert`, `lit.frag` | para shaderów oświetlonego labiryntu ze światłem liczonym dla każdego fragmentu (tryby `Phong` i `Blinn-Phong`). `lit.vert` przekazuje pozycję, normalną (uniform `uNormalMatrix`) i styczną (`mat3(uModel)`) w przestrzeni świata oraz współrzędne tekstury, `lit.frag` dołącza `common/lighting.glsl` i `common/normal_map.glsl`, bierze normalną fragmentu z `surfaceNormal`, woła `computeLighting` i składa kolor: tekstura razy `uTint` razy suma światła rozproszonego i blasku własnego `uEmissive` (od M5, dla kryształów), plus połysk. Wzór połysku wybiera uniform `uSpecularModel` | [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `assets/shaders/gouraud.vert`, `gouraud.frag` | para shaderów oświetlonego labiryntu ze światłem liczonym w wierzchołkach (tryb `Gouraud`). `gouraud.vert` dołącza `common/lighting.glsl` i woła tę samą funkcję `computeLighting` dla wierzchołka, `gouraud.frag` mnoży sumę rozciągniętego po trójkącie światła i blasku własnego `uEmissive` (od M5) przez teksturę i dodaje połysk. Bez map normalnych: stycznej nie czyta, a komentarz w `gouraud.vert` mówi dlaczego | [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `assets/shaders/skybox.vert`, `skybox.frag` | para shaderów nieba (M6). `skybox.vert` czyta tylko atrybut pozycji, usuwa przesunięcie z macierzy widoku (`mat4(mat3(uView))`), przekazuje pozycję narożnika jako kierunek `vDirection` i ustawia głębię na 1,0 (`gl_Position = position.xyww`). `skybox.frag` czyta teksturę sześcienną `uSkybox` (`samplerCube`) kierunkiem i mnoży kolor przez `uBrightness`, a w widokach debug (`uViewMode` różne od 0) pokazuje kierunek jako kolor | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcja 4 |
| `assets/shaders/common/lighting.glsl` | plik dołączany, nie samodzielny shader (nie ma linii `#version`): stała `MAX_POINT_LIGHTS`, struktura `PointLight`, blok uniformów `layout(std140) uniform LightBlock`, uniformy materiału `uSpecularModel`, `uSpecularStrength`, `uShininess`, struktura `Lighting` i funkcje `diffuseFactor`, `specularFactor`, `attenuationFactor`, `addLight` i `computeLighting`. Treść wstawia w miejsce linii `#include "common/lighting.glsl"` kod `gfx::expandIncludes` | [`../modules/scene/lights.md`](../modules/scene/lights.md), mechanizm dołączania w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `assets/shaders/common/normal_map.glsl` | plik dołączany, nie samodzielny shader (nie ma linii `#version`): sampler `uNormalMap` (jednostka 1), przełącznik `uNormalMapEnabled` i funkcja `surfaceNormal`, która z normalnej i stycznej modelu oraz teksela mapy normalnych składa normalną fragmentu w przestrzeni świata (macierz TBN). Dołączają go `lit.frag` i `textured.frag` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 4.1 |
| `assets/models/*.obj`, `*.mtl`, `assets/textures/*.png` | sześć modeli (`floor_tile`, `wall_straight`, `wall_pillar`, a od M5 `crystal_a`, `crystal_b` i `gate`), każdy z jednym materiałem, cztery tekstury koloru (`floor_stone.png`, `wall_stone.png`, `crystal.png`, `gate_wood.png`; ścianę i słupek pokrywa ta sama, oba kryształy też) i cztery mapy normalnych o tych samych nazwach z końcówką `_normal`, które materiały nazywają linią `map_Bump`. Budują je skrypty z `tools/blender/` | [`blender.md`](blender.md), [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), [`../modules/assets/images.md`](../modules/assets/images.md) |
| `assets/skybox/*.png` | sześć ścian nieba (M6): `px`, `nx`, `py`, `ny`, `pz`, `nz`, każda 1024 x 1024, RGB, razem 5 278 627 bajtów. Nie należą do żadnego modelu ani materiału. Wczytuje je `game::Skybox`, bez odwracania wierszy i bez pamięci podręcznej assetów. Buduje je `tools/blender/make_skybox.py`, a ich zawartość sprawdza `tests/SkyboxTests.cpp` | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), [`blender.md`](blender.md), sekcja 7.7 |
| `src/debug/DebugContext.hpp` | `debug::DebugContext`: struktura referencji do danych, które panele i HUD czytają albo edytują, 20 pól (`time`, `window`, `clearColor`, `camera`, `mouseSensitivity`, `texturedShader`, `colorShader`, `player`, `mazeSettings`, `mazeWorld`, `assets`, `viewMode`, `drawColliders`, `litShader`, `gouraudShader`, `lighting`, od M5 `gameplay` i `round`, a od M6 `skyboxShader` i `skybox`; pole `shader` zniknęło razem z programem `basic`). Sam nagłówek | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5 |
| `src/debug/DebugUI.*` | `debug::DebugUI`: inicjalizacja i zamknięcie ImGui, zastosowanie motywu i wczytanie czcionki, bajty czcionki, `draw` (rysuje osiem paneli, gdy są widoczne, a panelowi Shaders podaje tablicę pięciu programów, stała `SHADER_COUNT`; potem zawsze rysuje HUD, `drawHud`), `toggleVisible` (chowa i pokazuje panele, HUD zostaje), `wantsKeyboard`, `wantsMouse`, `setMouseEnabled` (ImGui ignoruje mysz, gdy kursor jest przechwycony) | [`../modules/debug-ui.md`](../modules/debug-ui.md), [`../libraries/imgui.md`](../libraries/imgui.md) |
| `src/debug/Hud.*` | `debug::drawHud`: HUD gry, rysowany przez ImGui w każdej klatce, także przy ukrytych panelach. Pasek u góry okna (napis `Crystals` z liczbą zebranych, potrzebnych i wszystkich kryształów, czas rundy, pasek baterii, który robi się czerwony poniżej progu, podpowiedzi `The gate is open. Find the exit.` i `Battery empty. Find a crystal.`) i karta `You escaped` po wygranej. Nie przyjmuje myszy ani klawiatury. Leży w `src/debug/`, bo tylko tam wolno dołączać ImGui | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 6 |
| `src/debug/Theme.*` | motyw paneli: `debug::colorFromBytes`, stałe `ERROR_TEXT_COLOR`, `PLAN_WALL_COLOR` i `PLAN_PLAYER_COLOR`, a od M5 kolory kryształów, bramy i wyjścia na planie (`PLAN_CRYSTAL_COLOR`, `PLAN_COLLECTED_COLOR`, `PLAN_GATE_COLOR`, `PLAN_EXIT_COLOR`) i kolory HUD (`HUD_CRYSTAL_COLOR`, `HUD_BATTERY_COLOR`, `HUD_BATTERY_LOW_COLOR`), `debug::applyTheme` (tabela kolorów, metryki, skala ekranu) i `debug::loadFont` (czcionka z `assets/fonts`, z czcionką wbudowaną jako wyjściem awaryjnym) | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8 |
| `src/debug/PanelLayout.*` | układ paneli przy pierwszym uruchomieniu: struktura `debug::PanelPlacement` (róg okna, odsunięcie, rozmiar i pole `collapsed`: czy panel startuje zwinięty do paska tytułu), stałe wymiarów (między innymi `LEFT_COLUMN_WIDTH`, `BOTTOM_ROW_HEIGHT`), osiem stałych `..._PLACEMENT` (od M5 z `GAMEPLAY_PLACEMENT`; ona i `CAMERA_PLACEMENT` mają `collapsed = true`) i `debug::placePanelOnFirstUse`, która ustawia pozycję, rozmiar i stan zwinięcia z warunkiem `ImGuiCond_FirstUseEver` | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.7 |
| `assets/fonts/AtkinsonHyperlegible-Regular.ttf`, `OFL.txt`, `README.md` | czcionka paneli debug (Atkinson Hyperlegible 1.006, 54 348 bajtów, plik niezmieniony), jej licencja SIL Open Font License 1.1 i opis źródła z sumą kontrolną. Cudzy materiał: nie jest kodem i nie powstaje ze skryptów projektu | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8.5 |
| `src/debug/panels/CameraPanel.*` | `debug::drawCameraPanel`: panel "Camera", przy pierwszym uruchomieniu zwinięty do paska tytułu (linia trybu, stopy gracza, oko tylko do odczytu, yaw, pitch, FOV, bliska i daleka płaszczyzna, czułość myszy, prędkość chodu, sprintu i lotu) | [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcja 6 |
| `src/debug/panels/MazePanel.*` | `debug::drawMazePanel`: panel "Maze" (suwaki rozmiaru, ziarno, przyciski "Regenerate" i "Random seed", liczby ścian i słupków, od M5 linia z liczbą kryształów i komórką wyjścia, plan labiryntu z góry z graczem, kryształami, bramą i strefą wyjścia). Od M5 czyta też `game::Round` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 6 |
| `src/debug/panels/CollisionPanel.*` | `debug::drawCollisionPanel`: panel "Collision" (pola wyboru "Draw collision shapes" i "Noclip (key N)", legenda kolorów linii, liczby pudełek ścian, słupków i bramy, liczba kul kryształów, pudełko gracza). Od M5 czyta też `game::Round` | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 6 |
| `src/debug/panels/GameplayPanel.*` | `debug::drawGameplayPanel`: panel "Gameplay", przy pierwszym uruchomieniu zwinięty do paska tytułu (stan rundy, liczniki kryształów, stan bramy, przycisk "Restart round (key R)", suwak "Battery", pole "Battery drains", suwaki "Crystals needed", "Battery lifetime", "Recharge", "Flicker below" i "Pickup radius"). Edytuje `game::GameplaySettings`, a w `game::Round` tylko ładunek baterii | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 6 |
| `src/debug/panels/AssetsPanel.*` | `debug::drawAssetsPanel`: panel "Assets" (lista "View mode", pole "Normal mapping", lista "Filter", suwak "Anisotropy", modele z częściami i ich mapami normalnych, tekstury z podglądem, lista nieudanych wczytań) | [`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md), sekcja 6 |
| `src/debug/panels/RendererPanel.*` | `debug::drawRendererPanel`: panel "Renderer" (FPS i czas klatki, rozmiar framebuffera i okna, wersja OpenGL i karta, edytor "Clear color", lista "Lighting" z pozycjami `Unlit`, `Gouraud`, `Phong`, `Blinn-Phong`, która zapisuje wybór w `game::LightingMode`, oraz, od M6, pole wyboru "Skybox" i suwak "Sky brightness", które piszą do `game::SkyboxSettings`) | [`../modules/debug-ui.md`](../modules/debug-ui.md), tryby w [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `src/debug/panels/LightsPanel.*` | `debug::drawLightsPanel`: panel "Lights" (edytor koloru "Ambient" i cztery grupy: "Moon (directional)" z kątami, kolorem i natężeniem, "Flashlight (spot)" z polem "Flashlight on (key F)", kolorem, natężeniem, stożkiem "Cone" i zasięgiem "Beam range", "Point lights (crystals)" z liczbą świecących kryształów, kolorem, natężeniem i promieniem "Point radius", "Highlight (specular)" z suwakami "Strength" i "Shininess"). Pole latarki ma podpowiedź, gdy bateria jest pusta. Edytuje `game::LightingSettings`, a `game::Round` tylko czyta | [`../modules/scene/lights.md`](../modules/scene/lights.md), sekcja 6 |
| `src/debug/panels/ShadersPanel.*` | `debug::drawShadersPanel`: panel "Shaders" (jeden przycisk "Reload shaders" dla wszystkich programów, a dla każdego z pięciu jedna linia: nazwy obu plików i `OK` albo, na czerwono, `FAILED` z komunikatem błędu pod spodem. Podpowiedź pokazuje pełne ścieżki) | [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6, komunikaty błędów w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `tests/main.cpp` | punkt wejścia programu testowego: makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` i dołączenie nagłówka doctest, który generuje `main()` | [`../libraries/doctest.md`](../libraries/doctest.md), sekcja 3.1 |
| `tests/ColliderTests.cpp` | 19 przypadków testowych: 12 dla `scene::Aabb`, `overlaps` pudełek i `moveAndSlide`, a od M5 siedem dla kul (`scene::Sphere`, oba `overlaps` z kulą, `closestPoint`, sam dotyk nie jest nakładaniem) | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 5 |
| `tests/ImageLoaderTests.cpp` | 10 przypadków testowych loadera obrazów (od M6 także wczytanie bez odwracania wierszy, `RowOrder::TopFirst`): obie tekstury gry (512 x 512, 3 kanały), obie mapy normalnych (rozmiar, średnia blisko `(128, 128, 255)`, konwencja OpenGL kanału zielonego, niebieski zawsze powyżej 128), odwracanie wierszy na obrazku 2 x 3 zapisanym przez test, ścieżka ze znakami spoza ASCII, brak pliku, plik niebędący obrazem, pusty plik | [`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7 |
| `tests/MazeTests.cpp`, `MazeGeneratorTests.cpp`, `MazeLayoutTests.cpp` | 31 przypadków testowych labiryntu (8, 11 i 12): klasa `Maze`, od M5 także `isDeadEnd` i porównanie `MazeCell`, generator (w tym labirynt wzorcowy 4 na 4 z ziarna 1), układ w świecie i jego współpraca z kolizjami | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5 |
| `tests/MazeWorldTests.cpp` | 8 przypadków testowych `game::buildMazeWorld`: wartości domyślne, `yawTowards`, liczba macierzy i pudełek, powtarzalność, macierze podłogi, słupków i ścian w obu ustawieniach, pozycja i kierunek startu | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), sekcja 5 |
| `tests/PlayerTests.cpp` | 13 przypadków testowych gracza: stałe, pudełko i oczy, chodzenie wzdłuż yaw, klawisze boczne i przeciwne, ruch po skosie, sprint, zatrzymanie na ścianie, ślizganie wzdłuż ściany obok słupków, wędrówka po zamkniętym labiryncie, noclip, powrót stóp na podłogę | [`../modules/game/player.md`](../modules/game/player.md), sekcja 5 |
| `tests/ObjLoaderTests.cpp` | 20 przypadków testowych loadera OBJ: reguły formatu na napisach wpisanych w kod (w tym linia mapy normalnych i styczne po `parseObj`), przypadki błędów z numerem linii i trzy prawdziwe modele z `assets/models/` (mapa normalnych, styczne, brak lustrzanych trójkątów) | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), sekcja 5.9 |
| `tests/ShaderSourceTests.cpp` | 22 przypadki testowe `gfx::expandIncludes` i `gfx::nameSourceFiles`: shader bez `#include`, wstawienie pliku między dwie dyrektywy `#line`, pliki zagnieżdżone, końce linii Windows, `#include` w komentarzu, błędy (brak pliku, plik dołączający siebie, cykl, źle zapisana linia, `#include` przed `#version`, `#version` w pliku dołączanym) oraz zamiana numeru na nazwę pliku w formacie NVIDII i w formacie Apple. Pliki dołączane są w testach napisami w mapie, nie plikami na dysku | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `tests/LightTests.cpp` | 20 przypadków testowych świateł: `attenuationFactor` i `attenuationForRadius` (5 procent jasności na promieniu), `coneCosines` i `spotFactor`, `directionFromAngles`, pusty `LightSet` oraz `packLightBlock` (kamera, otoczenie, księżyc, reflektor, światła punktowe, kierunek o długości zero, rozmiar i przesunięcia bloku `std140`) | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `tests/TangentTests.cpp` | 9 przypadków testowych stycznych: `triangleTangents` (tekstura prosto, obrócona, powtórzona, zdegenerowane UV), `computeTangents` (długość 1, Gram-Schmidt, średnia na wspólnym wierzchołku, brak `NaN`, złe indeksy) i `countMirroredTriangles` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 5.10 |
| `tests/LightingTests.cpp` | 10 przypadków testowych oświetlenia gry: wartości domyślne `LightingSettings`, numery trybów, `specularModelOf`, `usesNormalMap` i wartość startowa `normalMapping`, `buildLightSet` (otoczenie, księżyc, latarka w oku, wyłącznik, stożek, światła punktowe z podanych pozycji, limit 16). W M4 przypadków było 17: w M5 test funkcji `isDeadEnd` przeszedł do `MazeTests.cpp`, a sześć testów świateł w ślepych zaułkach zniknęło razem z tym kodem | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `tests/ExitTests.cpp` | 11 przypadków testowych wyjścia: `passageDistances` (korytarz, komórka nieosiągalna, droga przez przejścia), `farthestCell` (remis), labirynt wzorcowy 4 na 4 z ziarna 1 (wyjście w ślepym zaułku (3, 1)), wyjście jako ślepy zaułek z bramą dla 25 ziaren, labirynt z jednej komórki, `wallSegmentOn`, `exitZone`, pola wyjścia w `MazeWorld` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/CrystalTests.cpp` | 14 przypadków testowych kryształów: `crystalCountFor`, labirynt wzorcowy (dokładnie dwa kryształy), labirynt startowy (13 kryształów, wyjście w komórce (6, 5)), różne komórki bez startu i wyjścia, ślepe zaułki najpierw, oba warianty modelu, powtarzalność, za mało wolnych komórek, błędne argumenty, pozycja spoczynku i światła, kołysanie, obrót, pulsowanie i blask | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/RoundTests.cpp` | 25 przypadków testowych rundy: wartości domyślne reguł, `requiredCrystalCount`, nowa runda, zasięg gracza, zbieranie i promień zbierania, bateria (zużycie tylko przy włączonej latarce, pusta wyłącza latarkę, doładowanie kryształem, kryształ w kroku wyczerpania), brama (otwarcie, opadanie w 1,5 s, zmiana progu w trakcie rundy), wygrana tylko przy otwartej bramie, zegary po wygranej, labirynt bez kryształów i z jednej komórki, migotanie, `lightingForFrame`, `crystalLightPositions`, limit świateł | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/SkyboxTests.cpp` | 5 przypadków testowych plików nieba (M6): sześć kwadratów jednej wielkości z trzema kanałami, reguła wyboru ściany i teksela przepisana ze specyfikacji OpenGL (`facePointOf`), tarcza księżyca tam, skąd leci domyślne światło księżyca z `game::LightingSettings`, niebo jaśniejsze przy horyzoncie niż w zenicie i zgodność koloru po obu stronach każdej z dwunastu krawędzi sześcianu. Test czyta pliki loaderem, bez OpenGL | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcja 5.8 |
| `tests/TransformTests.cpp` | 4 przypadki testowe `scene::normalMatrix`: obiekt tylko przesunięty, obiekt obrócony, skala różna na osiach (tylko macierz normalnych zachowuje prostopadłość) i skala równa | [`../modules/scene/transforms.md`](../modules/scene/transforms.md) |

Każdy plik źródłowy zaczyna się komentarzem z jednym zdaniem opisu i odnośnikiem
`See docs/modules/...`. To wymaganie z PRD (sekcja 7). Dotyczy też plików shaderów, w
których komentarz stoi pod linią `#version`, bo ta musi być pierwsza. Wyjątkiem jest plik
dołączany `common/lighting.glsl`: nie ma linii `#version`, więc zaczyna się od komentarza. Odnośnik wskazuje najbardziej
szczegółowy dokument, czyli ten z kolumny "Dokument" powyżej, na przykład
`// See docs/modules/core/input.md` w `Input.hpp`. Pliki testów mają taki sam nagłówek i
wskazują dokument kodu, który sprawdzają.

## 2. Warstwy i targety

### Reguła warstw

Zależności są jednokierunkowe. Niższa warstwa nigdy nie wie o wyższej.

```text
main.cpp   łączy game/ i debug/
   │
   ├── debug/   może zależeć od każdej warstwy (i od Dear ImGui), nic nie zależy od debug/
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
`gfx/` bezpośrednio: samo wysyła macierze do shadera, a modele i linie rysuje przez
`gfx::Mesh::draw`. Własnego wywołania `glDrawElements` w `game/` nie ma od M5: miała je
tylko kostka z M1.

Od kamienia milowego M2 + M3 dochodzą trzy rzeczy, których rysunek nie pokazuje. Katalog
`game/` ma część bez okna (`Maze`, `MazeGenerator`, `MazeLayout`, `MazeWorld`, `Player`,
od M4 także `Lighting`, a od M5 `Exit`, `Crystals` i `Round`), która zależy tylko od `scene/` i GLM. Katalog `tests/` stoi na samej górze, obok `main.cpp`:
zależy od tej części `game/`, od `scene/` i od loaderów z `assets/`, a od niego nie zależy
nic.

Doszła też warstwa `assets/` (wczytywanie plików), której rysunek również nie pokazuje.
Stoi nad `gfx/` i `core/`. Dwa loadery (`ObjLoader`, `ImageLoader`) dołączają
`gfx/Vertex.hpp`, `core/Log.hpp` i `core/Paths.hpp`, a OpenGL nie wołają. `AssetCache`
dołącza `gfx/Mesh.hpp` i `gfx/Texture2D.hpp` i tworzy obiekty na karcie, więc wymaga
kontekstu OpenGL. Z `src/` warstwę dołączają `game/NightMazeApp.hpp`,
`game/MazeRenderer.cpp`, `game/GameplayRenderer.cpp`, `game/ModelDraw.cpp` i panel Assets. Opis:
[`../modules/assets/README.md`](../modules/assets/README.md), sekcja 4.

Trzecia rzecz: panele w `debug/` czytają teraz dane gry, więc dołączają nagłówki z `game/`
(`Player.hpp`, `MazeWorld.hpp`, `MazeLayout.hpp`, panel Assets `MazeRenderer.hpp` dla typu
`ViewMode`, panele Lights i Renderer `Lighting.hpp` dla `LightingSettings` i
`LightingMode`, a od M5 panele Gameplay, Maze, Collision i Lights oraz HUD `Round.hpp` dla
`Round` i `GameplaySettings`). Kierunek jest dozwolony: `debug/` może zależeć od każdej warstwy, a `game/`
nadal nie dołącza niczego z `debug/`.

Jak to widać w kodzie:

- `src/core/` dołącza tylko własne nagłówki, GLAD, GLFW i bibliotekę standardową. Jedynym
  wyjątkiem jest `Paths.cpp`, który dołącza nagłówek systemowy (`<mach-o/dyld.h>` na macOS,
  `<windows.h>` na Windowsie), bo położenie programu zna tylko system operacyjny. Także
  blokada klawiatury i myszy na czas pracy z panelem jest zrobiona bez ImGui w `core/`:
  `core::Input` ma neutralne flagi `setKeyboardBlocked` i `setMouseBlocked`, a ustawia je
  `main.cpp`. W drugą stronę działa to tak samo: `core::Input` wie tylko, czy kursor jest
  przechwycony, a o tym, że ImGui ma wtedy ignorować mysz, decyduje `main.cpp`.
- Nagłówki w `src/gfx/` dołączają `<glad/gl.h>` i bibliotekę standardową, a pliki `.cpp` do
  tego `core/GlCheck.hpp` (`Shader.cpp` także `core/Log.hpp`, `core/Paths.hpp` i
  `gfx/ShaderSource.hpp`, a `UniformBuffer.cpp` `core/Log.hpp`). `Shader.hpp` dołącza też
  `<glm/glm.hpp>`, a `Shader.cpp` `<glm/gtc/type_ptr.hpp>`, bo settery przyjmują typy GLM
  (`glm::mat4`, `glm::mat3`, `glm::vec3`). Nic z GLFW, `scene/`, `game/` ani `debug/`.
- `src/gfx/ShaderSource.hpp` **nie dołącza GLAD**, tak jak `Vertex.hpp`: tylko
  `<functional>`, `<span>`, `<string>` i `<vector>`, a `ShaderSource.cpp` do tego
  `<algorithm>`, `<cstddef>`, `<sstream>` i `<string_view>`. Niczego z `core/`: plików nie
  otwiera, funkcję czytającą plik dostaje od `Shader.cpp`. Dlatego da się go testować bez
  okna.
- Nagłówki w `src/scene/` dołączają `<glm/glm.hpp>` i najwyżej bibliotekę standardową
  (`Collider.hpp` do tego `<span>`, `Light.hpp` `<array>`, a `LightBlock.hpp` `<array>`,
  `<cstddef>`, `<cstdint>`, `<type_traits>` i jeden nagłówek własnej warstwy,
  `scene/Light.hpp`). Pliki `Transform.cpp` i `Camera.cpp` dołączają też
  `<glm/gtc/matrix_transform.hpp>`. Z biblioteki standardowej: `Camera.cpp` bierze
  `<algorithm>` i `<cmath>`, `Collider.cpp` `<algorithm>`, `<array>` i `<cmath>`,
  `Light.cpp` `<algorithm>` i `<cmath>`, a `LightBlock.cpp` `<algorithm>` i `<cstddef>`. Nic
  z GLAD, GLFW, `core/`, `gfx/`, `game/` ani `debug/`: blok `LightBlock` jest w `scene/`
  opisany jako zwykłe bajty, a na kartę wysyła go dopiero `game::LightRig`.
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
  standardową, a `ObjLoader.cpp` do tego `assets/Tangents.hpp`, `core/Log.hpp` i
  `core/Paths.hpp`. Nic z GLAD, GLFW, `scene/`, `game/` ani `debug/`: dlatego loader daje
  się testować bez okna.
- `src/assets/Tangents.hpp` dołącza `gfx/Vertex.hpp`, `<glm/glm.hpp>`, `<cstddef>`,
  `<cstdint>` i `<span>`, a `Tangents.cpp` do tego `<cmath>` i `<vector>`. Nic poza GLM i
  biblioteką standardową.
- `src/assets/AssetCache.hpp` dołącza `gfx/Mesh.hpp`, `gfx/Texture2D.hpp`, `<glm/glm.hpp>` i
  bibliotekę standardową, a `AssetCache.cpp` do tego `assets/ImageLoader.hpp`,
  `assets/ObjLoader.hpp`, `core/Log.hpp` i `core/Paths.hpp`. Przez `Mesh.hpp` i
  `Texture2D.hpp` przychodzi GLAD: to jedyny plik `assets/`, którego nie da się użyć bez
  okna.
- Pliki logiki gry bez okna w `src/game/` (`Crystals.*`, `Exit.*`, `Lighting.*`, `Maze.*`,
  `MazeGenerator.*`, `MazeLayout.*`, `MazeWorld.*`, `Player.*`, `Round.*`) dołączają
  bibliotekę standardową, GLM (bezpośrednio wszystkie poza `Maze.*`, `MazeGenerator.*` i
  `Exit.*`), nagłówki z `game/` i ze `scene/`: `MazeLayout.hpp`, `Player.hpp`, `Exit.hpp`,
  `MazeWorld.hpp` i `Round.hpp` dołączają `scene/Collider.hpp`, `MazeWorld.hpp` do tego
  `game/Crystals.hpp`, `game/Maze.hpp` i `game/MazeLayout.hpp`, `MazeWorld.cpp`
  `game/Exit.hpp`, `game/MazeGenerator.hpp` i `scene/Transform.hpp`, `Player.cpp`
  `scene/Camera.hpp`, `Lighting.hpp` `scene/Light.hpp` (nagłówka `game/Maze.hpp` już nie:
  o labiryncie nic nie wie), `Crystals.cpp` `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`
  i `scene/Light.hpp` (dla stałej `MAX_POINT_LIGHTS`), `Round.hpp` `game/Lighting.hpp` i
  `game/MazeWorld.hpp`, a `Round.cpp` `game/Crystals.hpp`. Nic z GLAD, GLFW, `core/`,
  `gfx/`, `assets/` ani `debug/`: dlatego dają się testować bez okna.
- Klasy i funkcje rysujące w `src/game/` należą do programu, nie do `game_logic`.
  `MazeRenderer.hpp` nie dołącza niczego (typy z `assets/` i `gfx/`
  zapowiada deklaracjami), a `MazeRenderer.cpp` `assets/AssetCache.hpp`, `core/Paths.hpp`,
  `game/MazeWorld.hpp`, `game/ModelDraw.hpp`, `game/ShaderUniforms.hpp` i `gfx/Shader.hpp`.
  `ModelDraw.hpp` dołącza tylko `<glm/glm.hpp>` i `<span>`, a `ModelDraw.cpp`
  `assets/AssetCache.hpp`, `game/ShaderUniforms.hpp`, `gfx/Shader.hpp` i
  `scene/Transform.hpp` (dla `scene::normalMatrix`). `GameplayRenderer.hpp` dołącza
  `game/Crystals.hpp`, `<glm/glm.hpp>`, `<array>` i `<cstddef>`, a `GameplayRenderer.cpp`
  `assets/AssetCache.hpp`, `core/Paths.hpp`, `game/MazeWorld.hpp`, `game/ModelDraw.hpp`,
  `game/Round.hpp`, `game/ShaderUniforms.hpp`, `gfx/Shader.hpp` i `scene/Transform.hpp`.
  `ColliderLines.hpp` dołącza `gfx/Mesh.hpp`, `scene/Collider.hpp`, `<glm/glm.hpp>` i
  `<span>`, a `ColliderLines.cpp` do tego `game/ShaderUniforms.hpp`,
  `gfx/Shader.hpp`, `gfx/Vertex.hpp`, `scene/Transform.hpp` i `<glm/gtc/constants.hpp>`
  (liczba pi dla okręgu). `LightRig.hpp` dołącza
  `gfx/UniformBuffer.hpp` i `<glm/glm.hpp>`, a `LightRig.cpp` do
  tego `game/ShaderUniforms.hpp`, `gfx/Shader.hpp`, `scene/Light.hpp` i
  `scene/LightBlock.hpp`.
- `src/game/ShaderUniforms.hpp` dołącza `<glad/gl.h>` (typ `GLuint` punktu wiązania), więc
  należy do programu, nie do `game_logic`.
- Pliki w `tests/` dołączają `<doctest/doctest.h>`, nagłówki testowanego kodu
  (`scene/Collider.hpp`, `game/Maze.hpp`, `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`,
  `game/MazeWorld.hpp`, `game/Player.hpp`, `assets/ObjLoader.hpp`, `assets/ImageLoader.hpp`,
  od M4 `gfx/ShaderSource.hpp`, `scene/Light.hpp`, `scene/LightBlock.hpp`,
  `scene/Transform.hpp` i `game/Lighting.hpp`, a od M5 `game/Exit.hpp`,
  `game/Crystals.hpp` i `game/Round.hpp`) i bibliotekę standardową. Żaden plik w `src/` nie dołącza niczego z `tests/` ani nagłówka
  doctest.
- `src/game/NightMazeApp.hpp` dołącza `assets/AssetCache.hpp`, `core/Application.hpp`, osiem
  nagłówków z `game/` (`ColliderLines.hpp`, `GameplayRenderer.hpp`, `LightRig.hpp`,
  `Lighting.hpp`, `MazeRenderer.hpp`, `MazeWorld.hpp`, `Player.hpp`, `Round.hpp`), jeden z
  `gfx/` (`Shader.hpp`; `Buffer.hpp` i `VertexArray.hpp` odpadły razem z kostką), dwa ze
  `scene/` (`Camera.hpp`, `Collider.hpp`), `<glm/glm.hpp>`, `<array>`, `<vector>` i nic z
  `debug/`.
  Komentarz w klasie mówi wprost: "It knows nothing about the debug UI". `NightMazeApp.cpp`
  dołącza do tego `core/GlCheck.hpp`, `core/Paths.hpp`, `game/Crystals.hpp`,
  `game/ShaderUniforms.hpp` i
  `<GLFW/glfw3.h>`, ten ostatni tylko dla stałych klawiszy i przycisku myszy (`GLFW_KEY_W`,
  `GLFW_KEY_N`, `GLFW_KEY_F`, `GLFW_KEY_R`, `GLFW_MOUSE_BUTTON_LEFT`): o stan wejścia pyta wyłącznie `core::Input`.
- `src/debug/DebugUI.cpp` dołącza `core/Window.hpp`, `debug/DebugContext.hpp`,
  `debug/Hud.hpp`, `debug/Theme.hpp`, nagłówki ośmiu paneli, `game/Lighting.hpp` (panel
  Renderer dostaje
  pole `mode` struktury `LightingSettings`) i nagłówki ImGui.
- `src/debug/Theme.cpp` dołącza `core/Log.hpp` i `core/Paths.hpp` (błąd wczytania czcionki i
  ścieżka do `assets/fonts`). `Theme.hpp` i `PanelLayout.hpp` dołączają `<imgui.h>`, bo
  pokazują typy `ImVec4` i `ImVec2`: to jedyne nagłówki projektu z nagłówkiem ImGui i
  dołączają je tylko pliki `.cpp` z `src/debug/`. Każdy z ośmiu plików paneli dołącza
  `debug/PanelLayout.hpp`, a panele Shaders, Assets i Maze także `debug/Theme.hpp`.
  `Hud.cpp` dołącza `debug/Theme.hpp` (kolory) i `game/Round.hpp`, a `Hud.hpp` nie dołącza
  niczego: oba typy gry zapowiada deklaracjami.
- `src/debug/panels/CameraPanel.cpp` dołącza `game/Player.hpp`, `scene/Camera.hpp` i
  `<glm/gtc/type_ptr.hpp>`. Panele Maze i Collision dołączają `game/MazeLayout.hpp`,
  `game/MazeWorld.hpp`, `game/Player.hpp` i `game/Round.hpp` (Maze także `scene/Camera.hpp`,
  Collision
  `scene/Collider.hpp`), a panel Assets `assets/AssetCache.hpp`, `core/Paths.hpp`,
  `game/MazeRenderer.hpp` i `gfx/Texture2D.hpp`. Panel Lights dołącza `game/Lighting.hpp`,
  `game/Round.hpp`, `scene/Light.hpp` i `<glm/gtc/type_ptr.hpp>`, panel Gameplay tylko
  `game/Round.hpp`, a panel Renderer
  `core/Time.hpp`, `core/Window.hpp` i `game/Lighting.hpp`.
- `src/debug/panels/ShadersPanel.cpp` dołącza `core/Paths.hpp` i `gfx/Shader.hpp`: to
  pierwszy plik w `debug/`, który zna `gfx/`. Kierunek jest dozwolony, bo `debug/` może
  zależeć od każdej warstwy.
- `src/main.cpp` jest jedynym plikiem, który dołącza jednocześnie `game/NightMazeApp.hpp` i
  nagłówki z `debug/` (`debug/DebugContext.hpp`, `debug/DebugUI.hpp`). Definiuje klasę
  `DebugNightMazeApp final : public game::NightMazeApp`, która posiada
  `debug::DebugUI m_debugUI{window()}` i w `onRender` najpierw woła
  `game::NightMazeApp::onRender(alpha)`, potem obsługuje klawisz `~` (przełącznik paneli),
  wyłącza mysz w ImGui, gdy kursor jest przechwycony przez kamerę
  (`m_debugUI.setMouseEnabled`), buduje `debug::DebugContext`, rysuje panele i HUD i przekazuje do
  `core::Input` informację, czy ImGui używa klawiatury i myszy.

Po co ta dyscyplina: grę da się zbudować i zrozumieć bez paneli debugowych, a panele można
rozbudowywać bez dotykania logiki gry. W docelowej architekturze między `gfx/` a `scene/`
dojdzie warstwa `renderer/`, a `debug/` nadal będzie zależeć od wszystkich i nikt od niego.

### Targety: `engine`, `game_logic`, `night_maze` i `night_maze_tests`

| Target | Rodzaj | Pliki | Linkuje |
|---|---|---|---|
| `engine` | biblioteka statyczna | `src/assets/*`, `src/core/*`, `src/gfx/*`, `src/scene/*` | `glad`, `glfw`, `glm::glm-header-only` (`PUBLIC`), `stb_image` (`PRIVATE`) |
| `game_logic` | biblioteka statyczna | `src/game/Crystals.*`, `src/game/Exit.*`, `src/game/Lighting.*`, `src/game/Maze.*`, `src/game/MazeGenerator.*`, `src/game/MazeLayout.*`, `src/game/MazeWorld.*`, `src/game/Player.*`, `src/game/Round.*` | `engine` (`PUBLIC`) |
| `night_maze` | program | `src/main.cpp`, `src/game/NightMazeApp.*`, `src/game/MazeRenderer.*`, `src/game/GameplayRenderer.*`, `src/game/ModelDraw.*`, `src/game/ColliderLines.*`, `src/game/LightRig.*`, `src/game/Skybox.*`, `src/game/ShaderUniforms.hpp`, `src/debug/*` (kontekst, `DebugUI`, HUD, układ, motyw, osiem paneli) | `engine`, `game_logic`, `imgui` (`PRIVATE`) |
| `night_maze_tests` | program | `tests/*.cpp` | `game_logic`, `doctest::doctest` (`PRIVATE`) |
| `glad` | biblioteka statyczna | `external/glad/src/gl.c` | nic |
| `glfw` | biblioteka statyczna | pobrana przez FetchContent | biblioteki systemowe |
| `glm-header-only` (alias `glm::glm-header-only`) | target `INTERFACE`: same nagłówki, nic się nie kompiluje | pobrany przez FetchContent | nic |
| `imgui` | biblioteka statyczna | pobrana przez FetchContent, lista plików w `Dependencies.cmake` | `glfw` |
| `doctest` (alias `doctest::doctest`) | target `INTERFACE`: jeden nagłówek, nic się nie kompiluje | pobrany przez FetchContent | nic |
| `stb_image` | biblioteka statyczna | `external/stb/stb_image.c`, nagłówek pobrany przez FetchContent. Target zdefiniowany w `Dependencies.cmake` | nic |

Co doszło do targetów z oświetleniem (M4), plik po pliku z list w `CMakeLists.txt`:

| Target | Nowe pliki |
|---|---|
| `engine` | `src/gfx/ShaderSource.*`, `src/gfx/UniformBuffer.*`, `src/scene/Light.*`, `src/scene/LightBlock.*` |
| `game_logic` | `src/game/Lighting.*` |
| `night_maze` | `src/game/LightRig.*`, `src/debug/panels/LightsPanel.*` |
| `night_maze_tests` | `tests/LightTests.cpp`, `tests/LightingTests.cpp`, `tests/ShaderSourceTests.cpp`, `tests/TransformTests.cpp` |

Podział idzie tą samą linią co wcześniej: kod bez OpenGL, który ma mieć testy, jest w
bibliotekach (`ShaderSource`, `Light`, `LightBlock`, `Lighting`), kod z OpenGL, ale bez
wiedzy o grze, też w `engine` (`UniformBuffer`), a kod gry z OpenGL i panel w programie
(`LightRig`, `LightsPanel`).

Co doszło do targetów z rozgrywką (M5):

| Target | Nowe pliki |
|---|---|
| `engine` | żadnego nowego pliku: kule doszły do istniejącego `src/scene/Collider.*` |
| `game_logic` | `src/game/Exit.*`, `src/game/Crystals.*`, `src/game/Round.*` |
| `night_maze` | `src/game/GameplayRenderer.*`, `src/game/ModelDraw.*`, `src/debug/Hud.*`, `src/debug/panels/GameplayPanel.*` |
| `night_maze_tests` | `tests/ExitTests.cpp`, `tests/CrystalTests.cpp`, `tests/RoundTests.cpp` |

Linia podziału jest ta sama: reguły rundy są zwykłymi danymi i matematyką, więc trafiły do
`game_logic` i mają testy, a to, co rysuje (modelem albo przez ImGui), jest w programie.
Z programu nie ubył żaden plik: kostka z M1 żyła w `NightMazeApp.*`, a kostki znaczników w
`LightRig.*`.

Co doszło do targetów z niebem (M6, część pierwsza):

| Target | Nowe pliki |
|---|---|
| `engine` | `src/gfx/Cubemap.*` |
| `game_logic` | żadnego nowego pliku |
| `night_maze` | `src/game/Skybox.*` |
| `night_maze_tests` | `tests/SkyboxTests.cpp` |

Ta sama linia podziału: `Cubemap` to kod z OpenGL bez wiedzy o grze, więc jest w `engine`.
`Skybox` wie, które pliki wczytać i kiedy rysować, i potrzebuje kontekstu OpenGL, więc jest
w programie. Test nieba nie używa żadnego z nich: czyta pliki PNG loaderem z `engine`
i bierze domyślny kierunek księżyca z `game_logic`.

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
(labirynt, generator, układ w świecie, `MazeWorld`, gracz, ustawienia oświetlenia, a od M5
wyjście, kryształy i reguły rundy), jest
biblioteką statyczną, którą linkują i gra, i testy. `NightMazeApp` i kod rysujący
(`MazeRenderer`, `GameplayRenderer`, funkcje z `ModelDraw`, `ColliderLines`, `LightRig`, `Skybox`) zostają w programie, bo potrzebują okna i kontekstu OpenGL, których test
nie ma. `game_logic` nie trafia do `engine`, bo `engine` ma nie zawierać niczego
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
    src/assets/AssetCache.cpp
    src/assets/AssetCache.hpp
    src/assets/ImageLoader.cpp
    src/assets/ImageLoader.hpp
    src/assets/ObjLoader.cpp
    src/assets/ObjLoader.hpp
    src/assets/Tangents.cpp
    src/assets/Tangents.hpp
    src/core/Application.cpp
    src/core/Application.hpp
    ...
    src/core/Window.cpp
    src/core/Window.hpp
    src/gfx/Buffer.cpp
    src/gfx/Buffer.hpp
    src/gfx/Cubemap.cpp
    src/gfx/Cubemap.hpp
    src/gfx/Mesh.cpp
    src/gfx/Mesh.hpp
    src/gfx/Shader.cpp
    src/gfx/Shader.hpp
    src/gfx/ShaderSource.cpp
    src/gfx/ShaderSource.hpp
    src/gfx/Texture2D.cpp
    src/gfx/Texture2D.hpp
    src/gfx/UniformBuffer.cpp
    src/gfx/UniformBuffer.hpp
    src/gfx/Vertex.hpp
    src/gfx/VertexArray.cpp
    src/gfx/VertexArray.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/Collider.cpp
    src/scene/Collider.hpp
    src/scene/Light.cpp
    src/scene/Light.hpp
    src/scene/LightBlock.cpp
    src/scene/LightBlock.hpp
    src/scene/Transform.cpp
    src/scene/Transform.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, gfx/Vertex.hpp,
# scene/Transform.hpp, scene/Camera.hpp, scene/Collider.hpp, scene/Light.hpp,
# scene/LightBlock.hpp and others) expose GLM types, so every target that includes them
# needs the GLM include path too.
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
  GLAD, GLFW i GLM. GLM dołączają pliki z `src/scene/` (`Transform`, `Camera`, `Collider`,
  `Light` i `LightBlock`) oraz `src/gfx/Shader.*` (macierz jako parametr `setMat4`). `glm::glm-header-only`
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
# The maze, its generator, its layout in the world, the player, the settings of the
# lighting and the rules of a round (exit, crystals, battery) are plain data and math.
# They live in a library of their own, and not in the night_maze executable, so that
# the test program can link them too: a test cannot link code that is inside another
# executable.
add_library(game_logic STATIC
    src/game/Crystals.cpp
    src/game/Crystals.hpp
    src/game/Exit.cpp
    src/game/Exit.hpp
    src/game/Lighting.cpp
    src/game/Lighting.hpp
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
    src/game/MazeWorld.cpp
    src/game/MazeWorld.hpp
    src/game/Player.cpp
    src/game/Player.hpp
    src/game/Round.cpp
    src/game/Round.hpp
)
# PUBLIC: the headers of this library (Exit.hpp, Lighting.hpp, MazeLayout.hpp,
# MazeWorld.hpp, Player.hpp, Round.hpp) include headers of engine (scene/Collider.hpp, scene/Light.hpp) and GLM,
# so whoever includes them needs the include paths of engine.
# The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)
```

- `add_library(game_logic STATIC ...)`: druga nasza biblioteka statyczna, z osiemnastu plików
  logiki gry bez okna (labirynt, `MazeWorld`, gracz, od M4 `Lighting`, a od M5 `Exit`,
  `Crystals` i `Round`). Lista jest jawna, tak jak przy
  `engine`.
- `target_link_libraries(game_logic PUBLIC engine)`: `game_logic` używa `scene::Aabb` z
  `engine`. `PUBLIC`, bo nagłówek `game/MazeLayout.hpp` sam dołącza `scene/Collider.hpp` i
  GLM: każdy, kto go dołączy, potrzebuje ścieżek nagłówków `engine`. Dzięki temu nie ma tu
  osobnego `target_include_directories`: korzeń `src/` przychodzi z `engine`.
- `night_maze_enable_warnings(game_logic)`: te same ścisłe ostrzeżenia co dla `engine`.

**Blok 6: target `night_maze`**

```cmake
# ---- night_maze: the application and its debug UI -------------------------------------
# NightMazeApp and the classes that draw (MazeRenderer, GameplayRenderer, ColliderLines,
# LightRig, Skybox, with the shared ModelDraw) stay in the executable: they need a window
# and an OpenGL context, so they are not something a test can run.
add_executable(night_maze
    src/main.cpp
    src/game/ColliderLines.cpp
    src/game/ColliderLines.hpp
    src/game/GameplayRenderer.cpp
    src/game/GameplayRenderer.hpp
    src/game/LightRig.cpp
    src/game/LightRig.hpp
    src/game/MazeRenderer.cpp
    src/game/MazeRenderer.hpp
    src/game/ModelDraw.cpp
    src/game/ModelDraw.hpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/game/ShaderUniforms.hpp
    src/game/Skybox.cpp
    src/game/Skybox.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/Hud.cpp
    src/debug/Hud.hpp
    src/debug/PanelLayout.cpp
    src/debug/PanelLayout.hpp
    src/debug/Theme.cpp
    src/debug/Theme.hpp
    src/debug/panels/AssetsPanel.cpp
    src/debug/panels/AssetsPanel.hpp
    src/debug/panels/CameraPanel.cpp
    src/debug/panels/CameraPanel.hpp
    src/debug/panels/CollisionPanel.cpp
    src/debug/panels/CollisionPanel.hpp
    src/debug/panels/GameplayPanel.cpp
    src/debug/panels/GameplayPanel.hpp
    src/debug/panels/LightsPanel.cpp
    src/debug/panels/LightsPanel.hpp
    src/debug/panels/MazePanel.cpp
    src/debug/panels/MazePanel.hpp
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
- `game_logic` jest potrzebne: `NightMazeApp` buduje labirynt (`buildMazeWorld`) i światła
  klatki (`buildLightSet`), prowadzi rundę (`startRound`, `updateRound`), ma pola typu
  `Player`, `LightingSettings`, `GameplaySettings` i `Round`, a panele i HUD czytają
  `MazeWorld`, `Player`, `LightingSettings` i `Round`.
- Z kodu gry w programie zostają `NightMazeApp`, `MazeRenderer`, `GameplayRenderer`,
  `ModelDraw`, `ColliderLines`, `LightRig`, `Skybox`,
  nagłówek `ShaderUniforms.hpp` i cały katalog `debug/`. Reszta `game/` jest w bibliotece
  `game_logic` (blok 5a).

**Blok 6a: target `night_maze_tests`**

```cmake
# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/ColliderTests.cpp
    tests/CrystalTests.cpp
    tests/ExitTests.cpp
    tests/ImageLoaderTests.cpp
    tests/LightTests.cpp
    tests/LightingTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/MazeWorldTests.cpp
    tests/ObjLoaderTests.cpp
    tests/PlayerTests.cpp
    tests/RoundTests.cpp
    tests/ShaderSourceTests.cpp
    tests/SkyboxTests.cpp
    tests/TangentTests.cpp
    tests/TransformTests.cpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests (of images and of OBJ models) read the real files of the game. A test
# must not depend on the directory it is started from, so the absolute path of assets/ in
# the repository is compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
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
- `add_executable(night_maze_tests ...)` buduje program testowy z siedemnastu plików
  (`tests/main.cpp` i szesnaście plików z testami) przy każdym zwykłym buildzie (jest częścią
  targetu domyślnego), więc testy zawsze się kompilują.
- `target_link_libraries(... PRIVATE game_logic doctest::doctest)`: kod testowany i
  biblioteka testów. `engine` przychodzi przez `game_logic`.
- `target_compile_definitions(night_maze_tests PRIVATE NIGHT_MAZE_ASSETS_DIR="...")`:
  definiuje makro preprocesora o wartości będącej napisem, bezwzględną ścieżką katalogu
  `assets` w repozytorium (`CMAKE_SOURCE_DIR` to korzeń repozytorium). Testy loaderów
  czytają prawdziwe modele i tekstury, a nie mogą zależeć od katalogu, z którego je
  uruchomiono. `PRIVATE`: makro widzą tylko pliki testów
  ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7).
- `add_test(...)` rejestruje jeden test CTest: uruchomienie całego programu.

Program testowy czyta assety: testy loaderów (`ObjLoaderTests.cpp`, `ImageLoaderTests.cpp`)
wczytują prawdziwe modele i tekstury gry. Biorą je z katalogu `assets` w repozytorium, przez
makro `NIGHT_MAZE_ASSETS_DIR`, a nie z kopii obok programu. Na Windowsie blok 7 kopiuje
katalog `assets` obok `night_maze`, a program testowy leży w tym samym katalogu, ale z tej
kopii nie korzysta, więc jej stan nie ma wpływu na wynik testów.

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
shadera nie wymaga więc zmiany w `CMakeLists.txt` (sekcja 5). Dotyczy to także podkatalogu
`assets/shaders/common/` z plikiem dołączanym: `copy_directory` kopiuje katalog z
podkatalogami, a dowiązanie na macOS obejmuje go z natury.

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
- `ExtraArgs: ['-D_CRT_USE_BUILTIN_OFFSETOF']`: dodatkowy argument kompilatora, dopisywany
  do flag każdego sprawdzanego pliku (opis niżej). Doszedł w M4.
- `HeaderFilterRegex: 'src/.*'`: diagnostyki z nagłówków pokazujemy tylko dla plików z
  `src/`. Nagłówki GLFW, GLAD, GLM i ImGui są pomijane.

**`ExtraArgs` i `offsetof`.** Fragment pliku:

```yaml
# Only matters on Windows, where clang-tidy reads the headers of the Microsoft C library.
# Their offsetof macro is written with a pointer cast, which MSVC accepts inside
# static_assert and clang does not. This switch of that library makes offsetof the
# built-in of the compiler instead. scene/LightBlock.hpp needs it: it checks the layout of
# the light block with static_assert(offsetof(...) == ...). The macro cannot be set in
# CMakeLists.txt: MSVC refuses to define it (warning C4117, a reserved name).
ExtraArgs: ['-D_CRT_USE_BUILTIN_OFFSETOF']
```

| Element | Znaczenie |
|---|---|
| `ExtraArgs` | lista argumentów, które clang-tidy dopisuje do polecenia kompilacji z `compile_commands.json`. Działa jak opcja `--extra-arg` w linii poleceń, tylko zapisana w pliku |
| `-D_CRT_USE_BUILTIN_OFFSETOF` | `-D` definiuje makro preprocesora. To konkretne makro jest przełącznikiem biblioteki C Microsoftu: z nim jej nagłówki definiują `offsetof` jako funkcję wbudowaną kompilatora |

Po co to jest. [`src/scene/LightBlock.hpp`](../../src/scene/LightBlock.hpp) sprawdza w
czasie kompilacji, czy struktura C++ ma pola dokładnie tam, gdzie układ `std140` stawia pola
bloku uniformów, liniami takimi jak `static_assert(offsetof(LightBlockData, points) == 160);`.
Wyrażenie w `static_assert` musi być stałą czasu kompilacji. Na Windowsie clang-tidy (czyli
clang) czyta nagłówki biblioteki C Microsoftu, w których `offsetof` jest domyślnie makrem
zapisanym rzutowaniem wskaźnika. MSVC uznaje taki zapis za stałą, clang nie: bez
przełącznika clang-tidy nie przyjmuje pliku, który MSVC kompiluje bez uwag.
Makra nie da się ustawić raz, w `CMakeLists.txt`, dla wszystkich narzędzi: MSVC odmawia
definiowania tej nazwy (ostrzeżenie C4117, nazwa zastrzeżona). Dlatego stoi w konfiguracji
narzędzi opartych na clangu: tutaj i w `.clangd` (sekcja 3.9).

Stan: na Windowsie przebieg clang-tidy z tą linią nie zgłasza niczego (2026-10-05,
[`build-windows.md`](build-windows.md), sekcja 13.1). Na macOS argument trafia do clang-tidy
tak samo. Nagłówków Microsoftu tam nie ma, więc makro nie powinno mieć żadnego skutku, ale
nikt tego jeszcze nie uruchomił. Opis bloku i asercji:
[`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md).

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
w linii poprzedzającej. W `src/` są dwa takie miejsca, oba dla kontroli
`performance-no-int-to-ptr` i oba z tego samego powodu (OpenGL przyjmuje przesunięcie w
bajtach przebrane za wskaźnik): w `src/gfx/VertexArray.cpp`
([`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md), sekcja 5) i w
`src/gfx/Mesh.cpp` ([`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcja 5).

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
  # The same switch as ExtraArgs in .clang-tidy, explained there: on Windows it lets
  # clang accept static_assert(offsetof(...) == ...).
  Add: [-D_CRT_USE_BUILTIN_OFFSETOF]
```

| Klucz | Wartość | Znaczenie |
|---|---|---|
| `CompileFlags` | sekcja | ustawienia dotyczące flag kompilacji, których clangd używa do analizy plików |
| `CompilationDatabase` | `build/debug` | katalog, w którym clangd ma szukać `compile_commands.json`. Ścieżka względna liczy się od katalogu, w którym leży plik `.clangd` |
| `Add` | `[-D_CRT_USE_BUILTIN_OFFSETOF]` | lista flag dopisywanych do polecenia kompilacji każdego pliku. Ten sam przełącznik co `ExtraArgs` w `.clang-tidy` (sekcja 3.6): na Windowsie pozwala clangowi przyjąć `static_assert(offsetof(...) == ...)` w `src/scene/LightBlock.hpp`. Doszedł w M4 |

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

Linia `Add` jest potrzebna z tego samego powodu co `ExtraArgs` w `.clang-tidy`: clangd to
też clang, więc na Windowsie bez przełącznika pokazywałby w edytorze błąd w
`LightBlock.hpp`. Czy tak jest i czy przełącznik go usuwa, nie oglądałem: clangd w edytorze
na Windowsie nie był jeszcze uruchamiany ([`build-windows.md`](build-windows.md), sekcja 4).
Na macOS flaga trafia do clangd tak samo i nie powinna niczego zmieniać (niesprawdzone).

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
| `files.associations` | `*.vert`, `*.frag`, `*.geom`, `*.glsl` na `glsl` | pliki shaderów są traktowane jako język GLSL (kolorowanie składni). Dziś jest ich dwanaście: pary `textured`, `color`, `lit`, `gouraud` i `skybox` w `assets/shaders/` oraz dwa pliki dołączane w `assets/shaders/common/`, `lighting.glsl` i `normal_map.glsl` (to dla nich jest wzorzec `*.glsl`) |

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
niczego w nowych plikach ani w testach. Zgłaszał natomiast jedną diagnostykę w starszym
pliku, w gałęzi kompilowanej tylko na Windowsie: `modernize-return-braced-init-list` w
`src/core/Paths.cpp`. Ta linia jest już poprawiona (`return {buffer};`), a przebieg
clang-tidy na plikach zmienionych w kroku łączącym M2 + M3 nie zgłasza niczego (Windows,
2026-10-05). Na Macu ta gałąź `Paths.cpp` nie jest kompilowana.

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
i katalogi `assets\shaders\` z kopią shaderów obok każdego z programów (pomiar z M1, gdy shaderów było dwa). Reszta akapitu
(rozszerzenie `.lib`, pliki `.sln` i `.vcxproj`) wynika z dokumentacji CMake i nie była
osobno sprawdzana. Z generatorem Ninja podkatalogu konfiguracji nie ma (zmierzone: program w
`build\ninja-debug\night_maze.exe`) i powstaje `compile_commands.json`. Wyjaśnienie i lista
kontrolna są w [`build-windows.md`](build-windows.md).

### 4.3. `imgui.ini`

Plik tekstowy, w którym Dear ImGui zapamiętuje pozycje, rozmiary i zadokowanie paneli.
Powstaje w **katalogu roboczym** procesu: przy uruchomieniu `./build/debug/night_maze` z
katalogu repozytorium jest to katalog główny repozytorium. Jest w `.gitignore`. Usunięcie go
przywraca domyślny układ paneli. Więcej w [`../libraries/imgui.md`](../libraries/imgui.md).

Od M2 + M3 każdy panel ma w kodzie miejsce i rozmiar startowy, ułożone dla okna 1280 x 720.
Dziś paneli jest osiem i jest to osiem stałych `..._PLACEMENT` w jednym pliku,
`src/debug/PanelLayout.hpp`, ustawianych przez `placePanelOnFirstUse` z warunkiem
`ImGuiCond_FirstUseEver` i liczonych od rogów okna ([`../modules/debug-ui.md`](../modules/debug-ui.md)):
Renderer nad Lights w lewej kolumnie, Maze nad Assets w prawej, Collision i
Shaders na dole między kolumnami, a Camera i Gameplay u góry, między kolumnami, obok
siebie, zwinięte do pasków tytułu (pole `collapsed`). Ten warunek działa tylko wtedy, gdy
`imgui.ini` nie ma jeszcze
wpisu dla danego panelu. Plik zapisany przez starszą wersję programu trzyma panele na
starych miejscach i w starych rozmiarach, a plik sprzed M4 nie ma wpisu panelu Lights, więc
ten jeden panel dostaje miejsce z kodu i nachodzi na panele ze starego układu. Plik z M4
nie ma wpisu panelu Gameplay: ten panel dostaje miejsce z kodu, u góry obok panelu Camera,
gdzie w układzie z M4 nic nie stoi. Plik sprzed pierwszej części M6 trzyma panel Renderer w
wysokości 230, a ten ma dziś dwa wiersze więcej (pole `Skybox` i suwak `Sky brightness`,
wysokość startowa 284): nowe kontrolki są wtedy pod dolną krawędzią panelu (wszystkie trzy
opisy to wnioski z kodu, nie obserwacje). Żeby zobaczyć domyślny układ,
trzeba przed uruchomieniem usunąć `imgui.ini` z katalogu, z którego program startuje.
Kolorów, odstępów ani czcionki w tym pliku nie ma: ustawia je kod przy każdym starcie. Nie
ma w nim też paska HUD ani karty wygranej: ich okna mają flagę
`ImGuiWindowFlags_NoSavedSettings`, a miejsce dostają z kodu w każdej klatce.

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
   | wczytywanie plików z `assets/`: loadery do danych procesora bez OpenGL, a nad nimi pamięć podręczna, która tworzy obiekty `gfx`. Bez wiedzy o grze i bez ImGui | `src/assets/` | `add_library(engine STATIC ...)` |
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
   `game_logic` dodatkowo niczego z GLAD, GLFW, `core/`, `gfx/` ani `assets/`.

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
   opisu i odnośnikiem `See docs/modules/...`, tak jak w plikach C++. Kod wspólny dla kilku
   shaderów idzie do pliku `.glsl` w `assets/shaders/common/`, **bez** linii `#version`, a
   shader dołącza go linią `#include "common/nazwa.glsl"`, która musi stać za linią
   `#version` (nazwa liczy się od katalogu pliku shadera). Wzór: `common/lighting.glsl`,
   dołączany przez `lit.frag` i `gouraud.vert`
   ([`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md)).
3. **Niczego nie dopisuj w `CMakeLists.txt`.** Krok z bloku 7 obejmuje cały katalog `assets/`.
4. **W kodzie buduj ścieżkę przez `core::assetPath`**, z nazwą względną wobec `assets/`, na
   przykład `core::assetPath("shaders/textured.vert")`. Wzór: stałe
   `TEXTURED_VERTEX_SHADER_FILE` i
   `TEXTURED_FRAGMENT_SHADER_FILE` w `src/game/NightMazeApp.cpp` (nowa para shaderów to tam nowa para
   stałych, nowe pole `gfx::Shader`, akcesor, pole w `debug::DebugContext` i wpis w tablicy
   programów w `DebugUI::draw` razem z większą stałą `SHADER_COUNT`, żeby panel Shaders ją
   przeładowywał), a dla modeli stałe
   `FLOOR_TILE_MODEL_FILE`, `WALL_MODEL_FILE` i `PILLAR_MODEL_FILE` w
   `src/game/MazeRenderer.cpp` oraz `CRYSTAL_A_MODEL_FILE`, `CRYSTAL_B_MODEL_FILE` i
   `GATE_MODEL_FILE` w `src/game/GameplayRenderer.cpp`. Model i jego tekstury wczytuje
   `assets::AssetCache`, a rysuje `game::drawModel`.
5. **macOS:** nic więcej, dowiązanie `build/<preset>/assets` widzi nowy plik od razu.
   **Windows:** kopię obok `night_maze.exe` odświeża
   `cmake --build --preset debug --target copy_assets`, a także każdy pełny build przy
   zamkniętym programie ([`build-windows.md`](build-windows.md), sekcja 7).
6. **Opisz plik** w dokumencie modułu (sekcja 4 szablonu, "Shadery") i w drzewie na początku
   tego dokumentu.
7. **Cudzy plik** (czcionka, obraz, model, którego nie zrobiłem sam) trafia do repozytorium
   tylko razem z plikiem licencji i z plikiem `README.md`, w którym jest źródło, wersja i
   data pobrania. Wzór: `assets/fonts/`.

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
