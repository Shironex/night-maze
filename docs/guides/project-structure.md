# Struktura projektu (stan: M1 kompletny, kod M2 + M3, M4, M5, M6 i sześciu części M7 na Windowsie, żadna nie zamknięta)

Kompletna mapa repozytorium Night Maze: co leży w którym katalogu, do czego służy każdy plik
konfiguracyjny i co powstaje dopiero podczas budowania. Dokument opisuje stan faktyczny po
czwartej części kamienia milowego M7, która dodała cienie księżyca, czyli pierwszą mapę
cieni (2026-10-05). Poprzedni stan to trzecia część M7, mgła z bufora głębi i winieta
(2026-10-05).

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
panel Lights, lista trybów `Lighting` w panelu Renderer, układ siedmiu paneli (dziś dwunastu) i
cztery nowe pliki testów. Program startuje w nocnej, oświetlonej scenie. W M4 światła
punktowe wisiały w ślepych zaułkach i były oznaczone kostkami: M5 zastąpił je światłami
kryształów i usunął ten kod.

Z M4 (druga część: mapy normalnych) są: dwie mapy normalnych w `assets/textures/`
(mapa ściany `wall_stone_normal.png` i mapa podłogi, którą w M6 zastąpiła mapa gruntu
`ground_normal.png`, liczone przez
`tools/blender/make_textures.py`), linia `map_Bump` w plikach `.mtl` (wtedy trzech) i jej obsługa w
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
asercji ([`build-windows.md`](build-windows.md), sekcja 15).

Z drugiej części M6 (teren i trawa) są: teren jako siatka wysokości (`src/game/Terrain.*`,
w bibliotece `game_logic`) i klasa, która go rysuje (`src/game/TerrainRenderer.*`), miejsca
kępek trawy (`src/game/Grass.*`, też `game_logic`) i klasa, która je rysuje
(`src/game/GrassRenderer.*`), szósty program shaderów z trzema plikami
(`assets/shaders/grass.vert`, `grass.geom` i `grass.frag`) i opcjonalny etap geometrii w
`src/gfx/Shader.*`, mapa wysokości `assets/textures/heightmap.png` ze skryptem
`tools/blender/make_heightmap.py`, tekstury gruntu `ground.png` i `ground_normal.png`,
panele Terrain i Grass (`src/debug/panels/TerrainPanel.*`, `GrassPanel.*`), funkcja
`game::drawMesh` w `src/game/ModelDraw.*` oraz pliki testów `tests/TerrainTests.cpp` i
`tests/GrassTests.cpp`. Ściany, słupki, brama, kryształy, wyjście i gracz stoją od tej
części na terenie. **Zniknęły płytki podłogi:** model `floor_tile.obj` z plikiem `.mtl`,
skrypt `build_floor_tile.py` i tekstury `floor_stone.png` oraz `floor_stone_normal.png`.
Całość opisują [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) i
[`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md). Na
Windowsie (2026-10-05): build Debug i Release bez ostrzeżeń zgłosił wykonawca, a 256
przypadków testowych i 101232 asercje uruchomiłem sam na obu programach testowych
([`build-windows.md`](build-windows.md)). **M6 ma kompletny kod na Windowsie i nie jest
zamknięty:** na macOS nic z M6 nie było budowane, a testy ręczne są otwarte. Iskier wokół
kryształów, które PRD wymienia przy temacie 9 obok trawy, w kodzie nie ma.

Z pierwszej części M7 (bufor HDR i gamma) są: przestrzeń kolorów jako typ i dwie funkcje
przeliczające (`src/gfx/ColorSpace.*`), pytanie sterownika o rozszerzenie
(`src/gfx/Extensions.*`, wydzielone z `Texture2D.cpp`), framebuffer jako obiekt OpenGL
(`src/gfx/Framebuffer.*`), klasa, która trzyma framebuffer HDR sceny i rysuje przebiegi po
scenie (`src/game/PostProcess.*`), siódmy i ósmy program shaderów w nowym podkatalogu
`assets/shaders/post/` (`composite.vert`, `composite.frag`, `preview.frag`), dwa nowe pliki
dołączane (`assets/shaders/common/color.glsl` i `depth.glsl`), jedenasty panel, Framebuffers
(`src/debug/panels/FramebuffersPanel.*`), sampler podglądów tekstur sRGB
(`src/debug/RawTextureSampler.*`) oraz pliki testów `tests/ColorSpaceTests.cpp` i
`tests/FramebufferTests.cpp`. Zmieniły się: `gfx::Texture2D`, `gfx::Cubemap` i
`assets::AssetCache::texture` (nowy, wymagany argument `gfx::ColorSpace`), `buildLightSet`
i kilka innych miejsc, w których wpisany kolor jest przeliczany z sRGB na liniowy, wartości
startowe świateł, blasku kryształów, jasności nieba i koloru tła, początek i koniec
`NightMazeApp::onRender`. Całość opisują
[`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md),
[`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) i
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md). Zgłoszone dla
Windowsa (2026-10-05), sam tego nie uruchamiałem: bramka `make check` przechodzi
(formatowanie, testy Debug i Release, clang-tidy), zero ostrzeżeń, wtedy 269 przypadków
testowych i 102103 asercje w obu konfiguracjach ([`build-windows.md`](build-windows.md)).

Z drugiej części M7 (bloom) są: ustawienia i matematyka poświaty bez OpenGL
(`src/game/Bloom.*`, w bibliotece `game_logic`), dziewiąty i dziesiąty program shaderów
(`assets/shaders/post/bright.frag` i `blur.frag`, oba z tym samym `composite.vert`) oraz plik
testów `tests/BloomTests.cpp`. Zmieniły się: `game::PostProcess` (funkcja `drawBloom`, trzy
cele bloomu i dwa kolejne podglądy), `post/composite.frag` (dodaje bloom przed ekspozycją),
`common/color.glsl` (funkcja `luminance`), `gfx::Shader` (setter tablicy `setFloatArray`),
`NightMazeApp` (dwa pola programów, wywołanie `drawBloom` w `onRender`), panel Framebuffers
(kontrolki bloomu w tabeli o dwóch kolumnach, cztery obrazy), `DebugContext` (dwa pola, razem
trzydzieści), `DebugUI::draw` (`SHADER_COUNT` równe 10) i jedna stała gry:
`CRYSTAL_GLOW_STRENGTH` wzrosło z 2,5 do 4,0. Opisuje to
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md). Zgłoszone dla
Windowsa (2026-10-05), sam tego nie uruchamiałem: bramka `make check` przechodzi, zero
ostrzeżeń, wtedy 276 przypadków testowych i 102139 asercji w obu konfiguracjach.

Z trzeciej części M7 (mgła i winieta) są: ustawienia i wzory mgły bez OpenGL
(`src/game/Fog.*`) i winiety (`src/game/Vignette.*`), oba w bibliotece `game_logic`, oraz
pliki testów `tests/FogTests.cpp` i `tests/VignetteTests.cpp`. Nie doszedł żaden program
shaderów (wtedy nadal dziesięć), żaden framebuffer ani panel (wtedy nadal jedenaście). Zmieniły się:
`post/composite.frag` (mgła liczona z tekstury głębi przed dodaniem bloomu, winieta po
mapowaniu tonów, siedem kroków w `main`), `game::PostProcess` (pola `fog` i `vignette` w
`PostProcessSettings`, struktura `SceneView`, czwarty parametr funkcji `composite`, tekstura
głębi sceny na trzeciej jednostce teksturującej), `ShaderUniforms.hpp` (jedenaście nowych
nazw, razem czterdzieści siedem), `NightMazeApp::onRender` (budowa `SceneView`, widoki debug
wyłączają pięć rzeczy zamiast trzech), panel Framebuffers (dwie zakładki, osiem nowych
kontrolek), komentarz w `DebugContext.hpp` (pól wtedy nadal trzydzieści) i listy plików w
`CMakeLists.txt`. Opisuje to
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md). Zgłoszone dla
Windowsa (2026-10-05), sam tego nie uruchamiałem: bramka `make check` przechodzi, zero
ostrzeżeń, wtedy 294 przypadki testowe i 102412 asercji w obu konfiguracjach.

Z czwartej części M7 (cienie księżyca) są: przestrzeń światła, czyli macierz widoku i rzut
ortograficzny księżyca (`src/scene/LightSpace.*`, zwykła matematyka w warstwie `scene/`),
sampler z porównaniem jako obiekt OpenGL (`src/gfx/ComparisonSampler.*`), ustawienia i
matematyka cieni bez OpenGL (`src/game/Shadows.*`, w bibliotece `game_logic`), klasa mapy
cieni (`src/game/ShadowMap.*`, w programie, w `src/game/` obok `PostProcess`), jedenasty
program shaderów (`assets/shaders/shadow_depth.vert` i `shadow_depth.frag`), piąty plik
dołączany (`assets/shaders/common/shadows.glsl`), dwunasty panel, Shadows
(`src/debug/panels/ShadowsPanel.*`), i plik testów `tests/ShadowTests.cpp`. Zmieniły się (po czwartej części; piąta część dopisała do tych plików cień latarki, bez nowych plików):
`NightMazeApp` (pierwszy przebieg klatki `drawMoonShadowMap`, funkcja `drawShadowCasters`,
pole programu, pola mapy, ustawień i przestrzeni światła, cztery akcesory),
`game/Lighting.*` (funkcja `moonDirection`, natężenie księżyca 0,2 zamiast 0,12),
`ShaderUniforms.hpp` (struktura `ShadowUniformNames` z siedmioma nazwami, stała
`MOON_SHADOW_UNIFORMS` i jednostka `MOON_SHADOW_TEXTURE_UNIT`), `PostProcess.hpp` (trzecia
wartość `AttachmentPreview::RawDepth`), `common/lighting.glsl` (pola `moonDiffuse` i
`moonSpecular`, funkcja `moonFacing`), `lit.frag`, `gouraud.vert`, `gouraud.frag` i
`grass.frag` (test cienia), `post/preview.frag` (tryb 2), `DebugContext` (cztery pola, razem
trzydzieści cztery), `DebugUI::draw` (`SHADER_COUNT` równe 11, dwanaście paneli),
`PanelLayout.hpp` (`SHADOWS_PLACEMENT`, czwarty rząd pasków), `main.cpp` i listy plików w
`CMakeLists.txt`. Komentarz `// See docs/...` wszystkich nowych plików wskazuje
[`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), a komentarz plików
`ColorSpace` wskazuje od tej części [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md).
Klasę samplera opisuje też
[`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md). Zgłoszone dla
Windowsa (2026-10-05, po czwartej części), sam tego nie uruchamiałem: bramka `make check`
przechodzi, 310 przypadków testowych i 103751 asercji, build Debug bez błędów OpenGL przy
mapie 2048 i przy mapie 1024. Po piątej części (cień latarki, 2026-10-06) zgłoszone jest
329 przypadków i 104306 asercji.

Szósta część M7, minimapa (2026-10-06), dodała pliki: `src/game/Discovery.*`, `Minimap.*`
(oba w bibliotece `game_logic`), `MinimapRenderer.*` (w programie), trzy pliki shaderów
(`assets/shaders/post/minimap.vert`, `minimap.frag` i `minimap_overlay.frag`), dwa pliki
testów (`tests/DiscoveryTests.cpp` i `tests/MinimapTests.cpp`, po 19 przypadków), dokument
[`../modules/renderer/minimap.md`](../modules/renderer/minimap.md) i dwie nowe notatki o
decyzjach. Zmieniły się: `game::Round` (pole `discovery`), `MazeLayout.*` (`cellAt`),
`gfx::Buffer` (czwarty parametr konstruktora i `setData`), `ShaderUniforms.hpp` (trzy stałe),
`NightMazeApp` (`drawMinimap`, klawisz M, cztery akcesory), `DebugContext` (cztery pola,
razem 42), `DebugUI::draw` (`SHADER_COUNT` równe 13), panel Framebuffers (trzecia zakładka),
`main.cpp`, `MazeLayoutTests.cpp` (jeden przypadek więcej) i listy plików w `CMakeLists.txt`.
Zgłoszone: 414 przypadków testowych i 138711 asercji. Paneli jest nadal dwanaście. **Nikt
nie obejrzał minimapy.** **M7 jest kompletny w kodzie na Windowsie i nie jest zamknięty:**
wszystkie sześć części ma kod i żadna nie jest zamknięta, na macOS nic z M7 nie było
budowane, a kontrolek nikt nie klikał. Krótki stan zbiera [`m7-status.md`](m7-status.md).

Poza M7 doszły podstawy M8 bez okna (2026-10-06): promień (`src/scene/Raycast.*`, [`../modules/scene/picking.md`](../modules/scene/picking.md)) oraz dźwignie i kartki (`src/game/Interactables.*`, [`../modules/game/interactables.md`](../modules/game/interactables.md)), każde z plikiem testów (`tests/RaycastTests.cpp`, `tests/InteractablesTests.cpp`: 17 i 29 przypadków). Nic z tego nie jest podpięte do działającej gry: nie ma wejścia, rysowania ani panelu. Łącznie zgłoszone jest 375 przypadków testowych i 138506 asercji (329 i 104306 po piątej części M7 plus 46 nowych przypadków), nie mierzone przeze mnie.

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
│   │   ├── gate.obj/.mtl           # drewniana brama wyjścia, w miejscu segmentu ściany
│   │   ├── wall_pillar.obj/.mtl    # słupek na rogu siatki
│   │   └── wall_straight.obj/.mtl  # segment ściany wzdłuż osi X
│   ├── shaders/                # shadery GLSL: sześć par, trójka trawy, pięć plików przebiegów po scenie i pięć plików dołączanych
│   │   ├── common/
│   │   │   ├── color.glsl          # srgbToLinear, linearToSrgb i luminance, dołączany przez #include (M7)
│   │   │   ├── depth.glsl          # linearDepth: głębia z tekstury na metry, dołączany przez #include (M7)
│   │   │   ├── lighting.glsl       # blok świateł i funkcja computeLighting, dołączany przez #include
│   │   │   ├── normal_map.glsl     # sampler mapy normalnych i funkcja surfaceNormal, dołączany przez #include
│   │   │   └── shadows.glsl        # mapa cieni księżyca: uniformy, bias, PCF i funkcja moonShadow, dołączany przez #include (M7, część 4)
│   │   ├── post/                   # przebiegi po scenie (M7): jeden trójkąt na cały cel
│   │   │   ├── blur.frag           # bloom: jeden kierunek rozmycia Gaussa, wagi w tablicy uniformów (M7, część 2)
│   │   │   ├── bright.frag         # bloom: przebieg jasności, światło ponad progiem z zachowaną barwą (M7, część 2)
│   │   │   ├── composite.frag      # przebieg składający: mgła, bloom, ekspozycja, mapowanie tonów, winieta, kodowanie sRGB
│   │   │   ├── composite.vert      # trójkąt na cały ekran z gl_VertexID, wspólny dla pięciu programów
│   │   │   ├── preview.frag        # podglądy załączników framebuffera sceny, celów bloomu i mapy cieni
│   │   │   ├── minimap.vert        # minimapa: pozycja w metrach labiryntu na obraz mapy macierzą ortograficzną (M7, część 6)
│   │   │   ├── minimap.frag        # minimapa: kolor kształtu zapisany bez zmian do tekstury (M7, część 6)
│   │   │   └── minimap_overlay.frag # minimapa: obraz mapy w rogu okna z przezroczystością (M7, część 6)
│   │   ├── color.frag              # linie pudełek i kul kolizji: jeden kolor z uColor
│   │   ├── color.vert              # linie pudełek i kul kolizji: trzy macierze, sama pozycja
│   │   ├── gouraud.frag            # scena, światło na wierzchołek: światło z wierzchołków razy tekstura, test cienia na fragment
│   │   ├── gouraud.vert            # scena, światło na wierzchołek: tu liczone jest światło
│   │   ├── grass.frag              # trawa: gradient koloru od korzenia do czubka, światło na fragment, cień księżyca
│   │   ├── grass.geom              # trawa: shader geometrii, z punktu kępka trzech źdźbeł, wiatr
│   │   ├── grass.vert              # trawa: punkt kępki w przestrzeni świata i jej liczba losowa
│   │   ├── lit.frag                # scena, światło na fragment: tu liczone jest światło (Phong, Blinn-Phong) i cień księżyca
│   │   ├── lit.vert                # scena, światło na fragment: pozycja, normalna i styczna w świecie
│   │   ├── shadow_depth.frag       # przebieg głębi mapy cieni: pusty, głębię zapisuje karta (M7, część 4)
│   │   ├── shadow_depth.vert       # przebieg głębi mapy cieni: wierzchołek tak, jak widzi go światło (M7, część 4)
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
│   └── textures/               # tekstury PNG: cztery obrazy koloru, ich mapy normalnych i mapa wysokości
│       ├── crystal.png
│       ├── crystal_normal.png
│       ├── gate_wood.png
│       ├── gate_wood_normal.png
│       ├── ground.png                  # grunt terenu, powtarzany co 4 m
│       ├── ground_normal.png
│       ├── heightmap.png               # mapa wysokości terenu 256 x 256 (buduje ją make_heightmap.py)
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
│   │   ├── RawTextureSampler.hpp/.cpp # sampler bez dekodowania sRGB: podgląd tekstury taki jak plik (M7)
│   │   ├── Theme.hpp/.cpp          # motyw paneli: kolory, odstępy, czcionka, skala ekranu
│   │   └── panels/
│   │       ├── AssetsPanel.hpp/.cpp    # panel "Assets": widok, filtr, anizotropia, modele, tekstury
│   │       ├── CameraPanel.hpp/.cpp    # panel "Camera": stopy gracza, kąty, FOV, prędkości
│   │       ├── CollisionPanel.hpp/.cpp # panel "Collision": rysowanie pudełek i kul, noclip, liczby
│   │       ├── FramebuffersPanel.hpp/.cpp # panel "Framebuffers": ekspozycja, mapowanie tonów, bloom, mgła, winieta, cztery podglądy (M7)
│   │       ├── GameplayPanel.hpp/.cpp  # panel "Gameplay": stan rundy, bateria, liczby reguł, restart
│   │       ├── GrassPanel.hpp/.cpp     # panel "Grass": włącznik, gęstość, wysokość źdźbeł, wiatr, liczba kępek
│   │       ├── LightsPanel.hpp/.cpp    # panel "Lights": otoczenie, księżyc, latarka, światła punktowe, połysk
│   │       ├── MazePanel.hpp/.cpp      # panel "Maze": rozmiar, ziarno, Regenerate, plan z kryształami, bramą i wyjściem
│   │       ├── RendererPanel.hpp/.cpp  # panel "Renderer": statystyki, kolor tła, lista Lighting, niebo
│   │       ├── ShadersPanel.hpp/.cpp   # panel "Shaders": trzynaście programów, przycisk Reload shaders
│   │       ├── ShadowsPanel.hpp/.cpp   # panel "Shadows": cienie księżyca, rozdzielczość, bias, PCF, obraz mapy cieni (M7, część 4)
│   │       └── TerrainPanel.hpp/.cpp   # panel "Terrain": skala wysokości, wireframe, rozmiar siatki
│   ├── game/                   # gra
│   │   ├── Bloom.hpp/.cpp          # bloom bez OpenGL: ustawienia, rozmiar celów, wagi rozmycia (M7, część 2)
│   │   ├── ColliderLines.hpp/.cpp  # rysowanie pudełek i kul kolizji liniami (GL_LINES)
│   │   ├── Crystals.hpp/.cpp       # kryształy: ile, w których komórkach, jak się ruszają i świecą
│   │   ├── Discovery.hpp/.cpp      # minimapa: które komórki gracz odkrył i reguła linii wzroku wzdłuż korytarzy (M7, część 6)
│   │   ├── Exit.hpp/.cpp           # wyjście: najdalsza komórka, brama, strefa wygranej
│   │   ├── Fog.hpp/.cpp            # mgła bez OpenGL: ustawienia, współczynnik wysokości, ilość mgły, miejsce w świecie z głębi (M7, część 3)
│   │   ├── GameplayRenderer.hpp/.cpp # rysowanie kryształów i bramy ich modelami
│   │   ├── Grass.hpp/.cpp          # trawa: miejsca kępek z ziarna labiryntu, ustawienia
│   │   ├── GrassRenderer.hpp/.cpp  # rysowanie trawy: punkty (GL_POINTS) dla shadera geometrii
│   │   ├── Interactables.hpp/.cpp  # dźwignie i kartki bez OpenGL: ściany-skróty, rozmieszczenie z ziarna, pociąganie, wskazywanie (M8, tylko podstawy)
│   │   ├── LightRig.hpp/.cpp       # bufor uniformów ze światłami klatki
│   │   ├── Lighting.hpp/.cpp       # tryby i ustawienia oświetlenia, światła klatki
│   │   ├── Maze.hpp/.cpp           # labirynt: siatka komórek, ściany na krawędziach, ślepy zaułek
│   │   ├── MazeGenerator.hpp/.cpp  # generator labiryntu z ziarna, randomBelow
│   │   ├── MazeLayout.hpp/.cpp     # układ w świecie: ściany, słupki, pudełka kolizji
│   │   ├── MazeRenderer.hpp/.cpp   # rysowanie ścian i słupków modelami
│   │   ├── MazeWorld.hpp/.cpp      # jeden labirynt w świecie, na terenie: macierze modelu, pudełka, start, wyjście, kryształy
│   │   ├── Minimap.hpp/.cpp        # minimapa bez OpenGL: ustawienia, kwadrat w rogu okna, rzut ortograficzny, lista trójkątów mapy (M7, część 6)
│   │   ├── MinimapRenderer.hpp/.cpp # minimapa: framebuffer mapy, bufor GL_DYNAMIC_DRAW, przebieg mapy i nakładka w rogu okna (M7, część 6)
│   │   ├── ModelDraw.hpp/.cpp      # rysowanie modelu albo siatki z teksturami, wspólne dla trzech rendererów
│   │   ├── NightMazeApp.hpp/.cpp   # aplikacja Night Maze: labirynt na terenie, runda, gracz, kamera, światła, cienie księżyca, trawa, niebo
│   │   ├── Player.hpp/.cpp         # gracz: pudełko, chodzenie z kolizjami po terenie, noclip
│   │   ├── PostProcess.hpp/.cpp    # framebuffer HDR sceny, podglądy, przebiegi bloomu, przebieg składający z mgłą i winietą (M7)
│   │   ├── Round.hpp/.cpp          # runda: kryształy, brama, bateria, czas i ich reguły
│   │   ├── ShaderUniforms.hpp      # nazwy uniformów shaderów w jednym miejscu
│   │   ├── ShadowMap.hpp/.cpp      # mapa cieni: framebuffer z samą głębią, sampler z porównaniem, podgląd (M7, część 4)
│   │   ├── Shadows.hpp/.cpp        # cienie bez OpenGL: ustawienia, rozmiar mapy, bias, promień PCF, pudełko rzucających cień (M7, część 4)
│   │   ├── Skybox.hpp/.cpp         # niebo: tekstura sześcienna na sześcianie, rysowane na końcu klatki
│   │   ├── Terrain.hpp/.cpp        # teren: mapa wysokości, siatka wysokości, heightAt, siatka trójkątów
│   │   ├── TerrainRenderer.hpp/.cpp # rysowanie terenu: jedna siatka, tekstura gruntu, wireframe
│   │   └── Vignette.hpp/.cpp       # winieta bez OpenGL: ustawienia i współczynnik przyciemnienia narożników (M7, część 3)
│   ├── gfx/                    # opakowania obiektów OpenGL (RAII, tylko przenoszenie)
│   │   ├── Buffer.hpp/.cpp         # bufor wierzchołków albo indeksów
│   │   ├── ColorSpace.hpp/.cpp     # sRGB albo liniowe: typ ColorSpace, srgbToLinear, linearToSrgb (M7)
│   │   ├── ComparisonSampler.hpp/.cpp # obiekt samplera z porównaniem: odczyt mapy cieni jako "oświetlony albo w cieniu" (M7, część 4)
│   │   ├── Cubemap.hpp/.cpp        # tekstura sześcienna (sześć ścian) i obiekt samplera
│   │   ├── Extensions.hpp/.cpp     # hasExtension: czy sterownik ma dane rozszerzenie OpenGL (M7)
│   │   ├── Framebuffer.hpp/.cpp    # framebuffer z teksturą koloru, teksturą głębi albo obiema: cel rysowania poza oknem (M7)
│   │   ├── Mesh.hpp/.cpp           # siatka: VAO i dwa bufory, draw (trójkąty, linie albo punkty)
│   │   ├── Shader.hpp/.cpp         # program shaderów z dwóch albo trzech plików, reload, settery, blok uniformów
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
│       ├── LightSpace.hpp/.cpp     # przestrzeń światła: widok i rzut ortograficzny księżyca, współrzędne w mapie cieni (M7, część 4)
│       ├── Raycast.hpp/.cpp        # promień: test pudełka i kuli, najbliższe trafienie, promień przez punkt obrazu (M8, tylko podstawy)
│       └── Transform.hpp/.cpp      # pozycja, obrót, skala, macierz modelu i macierz normalnych
├── tests/                      # testy jednostkowe (doctest): program night_maze_tests
│   ├── main.cpp                    # punkt wejścia: main() generuje doctest
│   ├── BloomTests.cpp              # testy bloomu bez OpenGL: rozmiar celów, wagi rozmycia, ustawienia (M7, część 2)
│   ├── ColliderTests.cpp           # testy scene::Aabb, scene::Sphere, overlaps i moveAndSlide
│   ├── ColorSpaceTests.cpp         # testy srgbToLinear i linearToSrgb: wartości wzorcowe, odwracalność (M7)
│   ├── CrystalTests.cpp            # testy kryształów: liczba, komórki, ruch i blask
│   ├── DiscoveryTests.cpp          # testy odkrywania komórek: reguła linii wzroku, start i krok rundy (M7, część 6)
│   ├── ExitTests.cpp               # testy wyjścia: odległości, najdalsza komórka, brama, strefa
│   ├── FogTests.cpp                # testy mgły bez OpenGL: współczynnik wysokości, prawo wykładnicze, miejsce w świecie z głębi (M7, część 3)
│   ├── FramebufferTests.cpp        # testy nazw formatów i tekstów stanu framebuffera, bez kontekstu OpenGL (M7)
│   ├── GrassTests.cpp              # testy placeGrass: liczba kępek, pas przy ścianie, rozsiew na wzgórzach
│   ├── ImageLoaderTests.cpp        # testy loadImage: tekstury i mapy normalnych gry (ściana, grunt), odwracanie wierszy, błędy
│   ├── InteractablesTests.cpp      # testy dźwigni i kartek bez okna (M8, tylko podstawy)
│   ├── LightingTests.cpp           # testy ustawień oświetlenia, usesNormalMap i buildLightSet
│   ├── LightTests.cpp              # testy zaniku, stożka reflektora i bajtów bloku świateł
│   ├── MazeGeneratorTests.cpp      # testy randomBelow i generateMaze, labirynt wzorcowy
│   ├── MazeLayoutTests.cpp         # testy układu w świecie i kolizji w labiryncie
│   ├── MazeTests.cpp               # testy klasy Maze, kierunków, isDeadEnd i MazeCell
│   ├── MazeWorldTests.cpp          # testy buildMazeWorld: macierze, pudełka, start
│   ├── MinimapTests.cpp            # testy minimapy bez OpenGL: kwadrat w rogu, rzut ortograficzny, kształty mapy (M7, część 6)
│   ├── ObjLoaderTests.cpp          # testy parseObj, parseMtl i loadObj
│   ├── PlayerTests.cpp             # testy gracza: chodzenie, sprint, ślizganie, noclip
│   ├── RaycastTests.cpp            # testy promienia bez okna: pudełko, kula, najbliższe trafienie, punkt obrazu (M8, tylko podstawy)
│   ├── RoundTests.cpp              # testy rundy: zbieranie, brama, bateria, migotanie, wygrana
│   ├── ShaderSourceTests.cpp       # testy expandIncludes i nameSourceFiles
│   ├── ShadowTests.cpp             # testy cieni bez OpenGL: pudełko światła, współrzędne w mapie, bias, teksel, jądro PCF (M7, część 4)
│   ├── SkyboxTests.cpp             # testy plików nieba: rozmiar, księżyc, gradient, granice ścian
│   ├── TangentTests.cpp            # testy triangleTangents, computeTangents i countMirroredTriangles
│   ├── TerrainTests.cpp            # testy terenu: mapa wysokości, heightAt, siatka, ściany i gracz na gruncie
│   ├── TransformTests.cpp          # testy macierzy normalnych
│   └── VignetteTests.cpp           # testy winiety bez OpenGL: środek, narożniki, promień, brak korekty proporcji (M7, część 3)
└── docs/
    ├── PRD.pdf                 # dokument wymagań
    ├── README.md               # spis treści dokumentacji i kolejność czytania
    ├── syllabus.md             # tabela: temat wykładu, dokument, pliki kodu
    ├── decisions/              # notatki "dlaczego tak, a nie inaczej"
    │   ├── README.md               # czym jest notatka, układ, lista notatek
    │   ├── aces-default-tone-mapping.md # domyślna krzywa mapowania tonów: ACES (M7)
    │   ├── battery-darkness-no-loss.md # pusta bateria to tylko ciemność, bez stanu przegranej
    │   ├── bloom-from-unfogged-scene.md # bloom liczony ze sceny bez mgły i dodawany po niej (M7, część 3)
    │   ├── bloom-half-resolution-three-targets.md # bloom w trzech celach o połowie rozdzielczości (M7, część 2)
    │   ├── blur-weights-computed-on-cpu.md # wagi rozmycia Gaussa liczone w C++, wysyłane tablicą uniformów (M7, część 2)
    │   ├── bright-pass-keeps-hue.md    # przebieg jasności bez skoku na progu i bez zmiany barwy (M7, część 2)
    │   ├── collision-aabb-sliding.md   # AABB i ślizganie zamiast silnika fizyki
    │   ├── crystal-count-and-gate-threshold.md # liczba kryształów z rozmiaru labiryntu, brama po około 70%
    │   ├── crystal-glow-raised-for-bloom.md # świecenie kryształów 4,0 zamiast 2,5, żeby poświata nie znikała (M7, część 2)
    │   ├── dead-end-lights.md          # światła punktowe w ślepych zaułkach (M4, zastąpiona w M5)
    │   ├── depth-attachment-as-texture.md # głębia framebuffera sceny jako tekstura, nie renderbuffer (M7)
    │   ├── deterministic-random.md     # własna randomBelow zamiast rozkładów std
    │   ├── enemy-after-m5.md           # przeciwnik dopiero po M5
    │   ├── exit-farthest-cell.md       # wyjście w komórce najdalszej od startu, nie w rogu
    │   ├── flashlight-in-hand.md       # M7, część 5: latarka w ręce, decyzja właściciela, obowiązuje, kod jest
    │   ├── flashlight-hand-straight-down.md # M7, część 5: ręka latarki, w dół prosto w dół w świecie, granica suwaka w prawo
    │   ├── flashlight-shadow-bias-in-world-space.md # M7, część 5: bias cienia latarki w metrach, przesunięcie punktu w świecie
    │   ├── floor-tiles-retired.md      # płytki podłogi usunięte, podłogą jest teren
    │   ├── fog-distance-from-reconstructed-position.md # mgła z odległości od oka do miejsca odtworzonego z głębi (M7, część 3)
    │   ├── fog-height-at-the-pixel.md  # wysokość mgły brana w pikselu, bez sumowania wzdłuż linii wzroku (M7, część 3)
    │   ├── fog-no-special-case-for-sky.md # niebo bez osobnego przypadku we mgle (M7, część 3)
    │   ├── gamma-linear-pipeline.md    # potok liniowy: tekstury sRGB, rachunek liniowy, kodowanie na końcu (M7)
    │   ├── gentle-terrain-under-maze.md # łagodny teren pod labiryntem, wzgórza dookoła
    │   ├── gouraud-shadow-test-per-fragment.md # w trybie Gouraud światło na wierzchołek, test cienia na fragment (M7, część 4)
    │   ├── grass-casts-no-shadow.md    # trawa przyjmuje cień księżyca i sama go nie rzuca (M7, część 4)
    │   ├── grass-lit-with-up-normal.md # trawa oświetlana normalną gruntu, jeden program na wszystkie tryby
    │   ├── height-scale-rebuilds-terrain.md # skala wysokości przebudowuje teren na procesorze, nie jest uniformem
    │   ├── heightmap-tiled-in-world-metres.md # mapa wysokości powtarzana co 48 m świata
    │   ├── minimap-discovered-corridors.md # M7, część 6: minimapa tylko z odkrytych korytarzy, decyzje właściciela i wybory wykonawcze, z kodem
    │   ├── minimap-srgb-constants-after-composite.md # minimapa po przebiegu składającym, stałe sRGB bez konwersji (M7, część 6)
    │   ├── minimap-vertices-rebuilt-every-frame.md # lista trójkątów minimapy budowana i wysyłana co klatkę (M7, część 6)
    │   ├── no-gamma-until-m7.md        # bez korekcji gamma i tekstur sRGB do M7 (zastąpiona w M7)
    │   ├── painted-moon-fixed-direction.md # księżyc namalowany na niebie, w domyślnym kierunku światła
    │   ├── post-process-in-game-layer.md # PostProcess w src/game/, nadal bez warstwy renderer (M7)
    │   ├── shadow-bias-in-metres-in-shader.md # bias cienia w metrach, liczony w shaderze, bez glPolygonOffset (M7, część 4)
    │   ├── shadow-box-fitted-to-terrain.md # pudełko światła księżyca dopasowane do terenu, niezależne od kamery (M7, część 4)
    │   ├── shadow-matrix-as-plain-uniforms.md # macierz i liczby mapy cieni jako zwykłe uniformy, LightBlock bez zmian (M7, część 4)
    │   ├── shadow-takes-only-moon-light.md # cień odejmuje tylko udział księżyca (M7, część 4)
    │   ├── skybox-in-game-layer.md     # niebo jako game::Skybox, bez warstwy renderer
    │   ├── small-calls-after-m6.md     # cztery drobne rozstrzygnięcia po M6: klawisz paneli, Esc, latarka, rozmiar nieba
    │   ├── srgb-encode-in-shader.md    # kodowanie sRGB w shaderze, GL_FRAMEBUFFER_SRGB wyłączone (M7)
    │   ├── tangents-on-load.md         # styczne liczone przy wczytaniu, bez znaku skrętności
    │   ├── vignette-not-aspect-corrected.md # winieta bez korekty proporcji okna (M7, część 3)
    │   └── walls-sunk-to-lowest-corner.md # ściany, słupki i brama zatopione do najniższego gruntu pod sobą
    ├── guides/                 # przewodniki
    │   ├── blender.md              # modele i tekstury: skrypty Blendera, eksport OBJ
    │   ├── build-macos.md          # budowanie na macOS
    │   ├── build-windows.md        # budowanie na Windowsie
    │   ├── m7-status.md            # gdzie stoi M7: co jest, co zostało, decyzje, ograniczenia, sprawdzenia na macOS
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
        │   ├── interactables.md        # dźwignie i kartki bez okna: skróty, rozmieszczenie, pociąganie, wskazywanie (M8)
        │   ├── maze-generator.md       # labirynt, generator, układ w świecie, panel Maze
        │   ├── maze-rendering.md       # MazeWorld, MazeRenderer, ModelDraw, macierze modelu, regeneracja
        │   └── player.md               # gracz: chodzenie, noclip, oczy a stopy, testy
        ├── gfx/                    # moduł gfx, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, RAII i przenoszenie, warstwy, indeks
        │   ├── buffers-vao.md          # VBO, VAO, krok i przesunięcie, Buffer, VertexArray
        │   ├── color-space.md          # gamma: sRGB i wartości liniowe, ColorSpace, gdzie kolory są przeliczane
        │   ├── comparison-sampler.md   # sampler z porównaniem dla mapy cieni, klasa ComparisonSampler
        │   ├── cubemap.md              # tekstura sześcienna jako obiekt OpenGL, klasa Cubemap
        │   ├── framebuffers.md         # framebuffer, załączniki, kompletność, klasa Framebuffer
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
        │   ├── grass-geometry.md       # shader geometrii, trawa z punktów, grass.*, panel Grass
        │   ├── lighting-gouraud-phong.md   # Gouraud a Phong, Phong a Blinn-Phong, lit.* i gouraud.*
        │   ├── minimap.md              # minimapa: drugi framebuffer, rzut ortograficzny, odkrywanie korytarzy, bufor dynamiczny, mieszanie (M7, część 6)
        │   ├── post-process.md         # scena do bufora HDR, przebieg składający, mapowanie tonów, bloom, mgła, winieta, podglądy, panel Framebuffers
        │   ├── shadows.md              # shadow mapping: mapa cieni księżyca, bias, PCF, shadow_depth.*, shadows.glsl, panel Shadows
        │   ├── skybox.md               # tekstura sześcienna, niebo, skybox.*, make_skybox.py
        │   └── terrain.md              # teren z mapy wysokości, heightAt, wireframe, panel Terrain
        ├── scene/                  # moduł scene, podzielony na dokumenty tematyczne
        │   ├── README.md               # wstęp, dane bez OpenGL, warstwy, indeks
        │   ├── camera-controls.md      # obrót kamery myszą, panel Camera
        │   ├── camera.md               # macierz widoku, rzutowanie, Camera
        │   ├── collision.md            # AABB, kule, ruch ze ślizganiem, linie pudełek i kul, panel Collision
        │   ├── lights.md               # rodzaje świateł, model odbicia, lighting.glsl, panel Lights
        │   ├── picking.md              # promień: pudełko, kula, od punktu obrazu do promienia, ray casting (M8)
        │   └── transforms.md           # przestrzenie, macierz modelu, Transform, macierz normalnych
        └── debug-ui.md             # panele ImGui i HUD w projekcie
```

Drzewo nie pokazuje katalogu `tools/blender/`: to skrypty Pythona dla Blendera, które budują
modele, tekstury, obrazy nieba i mapę wysokości z `assets/`. Program ich nie czyta i CMake ich nie uruchamia. Opisuje je
[`blender.md`](blender.md). Pliki katalogu, z rolą z komentarza na początku każdego:

| Plik | Rola |
|---|---|
| `blender_common.py` | wspólne funkcje skryptów: czyszczenie sceny, budowanie brył, rzutowanie UV, materiał z teksturą i mapą normalnych, eksport OBJ ze stałymi opcjami, rendery do przeglądu |
| `build_wall_straight.py` | model `wall_straight`: segment ściany, 2 m długości i 3 m wysokości |
| `build_wall_pillar.py` | model `wall_pillar`: słupek na rogi siatki, który zakrywa końce segmentów |
| `build_crystal.py` (M5) | dwa modele kryształów: `crystal_a`, jeden wysoki odłamek ze szpicem na obu końcach, i `crystal_b`, grupa trzech odłamków na płaskiej podstawie |
| `build_gate.py` (M5) | model `gate`: zamknięta drewniana brama, która wypełnia jeden bok komórki między dwoma słupkami, w miejscu segmentu ściany |
| `make_textures.py` | osiem tekstur 512 x 512 w `assets/textures`: obrazy koloru `wall_stone.png`, `ground.png`, `gate_wood.png` i `crystal.png` oraz mapa normalnych każdego z nich. Do pierwszej części M6 zamiast gruntu była tekstura podłogi |
| `make_skybox.py` (M6) | sześć obrazów nieba 1024 x 1024 w `assets/skybox`: ściany tekstury sześciennej liczone z kierunku każdego piksela (tło, Droga Mleczna, gwiazdy, księżyc) |
| `make_heightmap.py` (M6) | mapa wysokości terenu `heightmap.png` w `assets/textures`: szary obraz 256 x 256 z gładkiego szumu w trzech oktawach, który powtarza się bez szwu |
| `make_all.py` | wszystko naraz w jednym procesie Blendera: najpierw tekstury, potem modele, na końcu niebo i mapa wysokości |

Skryptu `build_floor_tile.py`, który budował płytkę podłogi, już nie ma: usunął go M6 razem z modelem.

Zapis
`wall_pillar.obj/.mtl` oznacza parę plików `wall_pillar.obj` i `wall_pillar.mtl`.

Katalog `docs/modules/renderer/` istnieje, choć katalogu `src/renderer/` jeszcze nie ma:
dokumenty o cieniowaniu, o niebie, o terenie, o trawie, od pierwszej części M7 o
przebiegach po scenie i, od czwartej, o cieniach są już pod nazwą warstwy z PRD, a kod,
który opisują, leży dziś w `src/game/` (`NightMazeApp.cpp`, `Skybox.*`, `Terrain.*`,
`TerrainRenderer.*`, `Grass.*`, `GrassRenderer.*`, `PostProcess.*`, `Shadows.*`,
`ShadowMap.*`) i w `assets/shaders/`. Dla `PostProcess` zapisuje to
notatka [`../decisions/post-process-in-game-layer.md`](../decisions/post-process-in-game-layer.md).
Wskazała ona przebiegi cieni jako chwilę powrotu do tej decyzji. Przebieg cieni księżyca
już jest, a warstwy nadal nie ma: `game::ShadowMap` stoi w `src/game/` obok `PostProcess`.
Dwa pliki cieni leżą poza `src/game/`, tam gdzie chce reguła warstw: `scene/LightSpace.*`
(sama matematyka) i `gfx/ComparisonSampler.*` (obiekt OpenGL bez wiedzy o grze).

Zapis `Window.hpp/.cpp` oznacza parę plików `Window.hpp` i `Window.cpp`. Zgodnie z zasadą z
PRD nagłówek i implementacja leżą obok siebie w `src/`, nie ma osobnego katalogu `include/`. Pliki konfiguracyjne z katalogu głównego są
wypisane na początku drzewa, przed katalogami.

### Katalogi

| Katalog | Rola | Kto pisze kod |
|---|---|---|
| `src/` | cały nasz kod C++ poza testami | my |
| `tests/` | testy jednostkowe: osobny program `night_maze_tests`, który woła kod z bibliotek `engine` i `game_logic` i sprawdza wyniki ([`../libraries/doctest.md`](../libraries/doctest.md)) | my |
| `assets/` | pliki, które program wczytuje w czasie działania: shadery GLSL (z podkatalogiem `shaders/common/` na pliki dołączane i, od M7, `shaders/post/` na przebiegi po scenie), modele OBJ z materiałami MTL, tekstury PNG, sześć obrazów nieba (`skybox/`, od M6) i czcionka paneli debug. Nie są kompilowane razem z programem. Krok budowania umieszcza katalog obok pliku wykonywalnego (sekcja 3.1, blok 7) | my, z jednym wyjątkiem: `assets/fonts/` to cudzy materiał. Czcionka Atkinson Hyperlegible (Braille Institute of America) jest na licencji SIL Open Font License 1.1, a plik licencji `OFL.txt` leży obok niej i musi tam zostać |
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
| `src/core/Paths.*` | `core::executableDir` i `core::assetPath`: ścieżki do plików z `assets/` liczone od położenia pliku wykonywalnego. `core::pathText`: ścieżka jako tekst UTF-8 do logu i do paneli. `Paths.cpp` to jedyny plik w `src/` z kodem zależnym od systemu (`#if` dla macOS i Windows). Wołają je konstruktor `game::NightMazeApp` (ścieżki shaderów), konstruktory `game::MazeRenderer` i `game::GameplayRenderer` (ścieżki modeli), `assets::AssetCache` (`pathText` w logu) i panele Shaders oraz Assets. Od M6 `assetPath` wołają też `game::Skybox` (sześć obrazów nieba), `game::TerrainRenderer` (dwie tekstury gruntu) i funkcja `loadHeightmap` w `NightMazeApp.cpp` (mapa wysokości) | [`../modules/core/paths.md`](../modules/core/paths.md) |
| `src/gfx/Shader.*` | `gfx::Shader`: obiekt programu OpenGL zbudowany z pliku shadera wierzchołków i pliku shadera fragmentów. `reload` (przy błędzie zostaje stary program), `isValid`, `use`, `setMat4` (uniform typu `mat4`, przez `glGetUniformLocation` i `glUniformMatrix4fv`), `setInt` (uniform typu `int` albo sampler, `glUniform1i`), `setVec3` (uniform typu `vec3`, `glUniform3fv`), `setMat3` (uniform typu `mat3`, `glUniformMatrix3fv`: macierz normalnych), `setFloat` (uniform typu `float`, `glUniform1f`), `bindUniformBlock` (łączy blok uniformów z punktem wiązania przez `glGetUniformBlockIndex` i `glUniformBlockBinding`, zapamiętuje prośbę w strukturze `gfx::UniformBlockBinding` i powtarza ją po każdym `reload`, porównuje rozmiar bloku według sterownika z rozmiarem z C++), `lastError`, `vertexPath`, `fragmentPath`. Przed kompilacją rozwija dyrektywy `#include` (`gfx::expandIncludes`), a w komunikacie błędu zamienia numer napisu źródłowego na nazwę pliku (`gfx::nameSourceFiles`). RAII, tylko przenoszenie. Po M5 `NightMazeApp` miało cztery takie obiekty (`textured`, `color`, `lit`, `gouraud`; piąty, `basic`, zniknął w M5 razem z kostką), dziś ma ich jedenaście (lista w wierszu `src/game/NightMazeApp.*`), a panel "Shaders" woła `reload` na każdym. `setInt` ustawia samplery `uTexture` i `uNormalMap`, tryb `uViewMode`, przełącznik `uNormalMapEnabled` i wzór połysku `uSpecularModel`, `setVec3` kolory `uTint`, `uColor` i (od M5) `uEmissive`, `setFloat` uniformy `uSpecularStrength` i `uShininess`, `setMat3` uniform `uNormalMatrix`. Od drugiej części M6 program może mieć trzeci, opcjonalny etap między tymi dwoma: shader geometrii. Konstruktor ma trzeci parametr `geometryPath` z pustą ścieżką domyślną, `hasGeometryStage()` i `geometryPath()` mówią, czy i z jakiego pliku etap pochodzi, a `reload` czyta wtedy trzy pliki. Jedynym takim programem jest trawa | [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md), settery w [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), `reload` w [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), `bindUniformBlock` w [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md), `#include` w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md), wstęp do warstwy w [`../modules/gfx/README.md`](../modules/gfx/README.md) |
| `src/gfx/ShaderSource.*` | typ `gfx::IncludeReader` (funkcja, która podaje treść dołączanego pliku), struktura `gfx::ShaderSource` (tekst po rozwinięciu i lista plików, z których powstał) oraz funkcje `gfx::expandIncludes` (zastępuje linie `#include "..."` treścią plików, także zagnieżdżone, i dopisuje wokół nich dyrektywy `#line`; zwraca błąd dla brakującego pliku, pliku dołączającego samego siebie, źle zapisanej linii, `#include` przed `#version` i `#version` w pliku dołączanym) i `gfx::nameSourceFiles` (w dzienniku sterownika zamienia numer napisu źródłowego na nazwę pliku, dla formatu NVIDII `1(15)` i formatu `ERROR: 1:15:`, a przy więcej niż jednym pliku dopisuje linię `Source files: ...`). Sama praca na tekście: nie otwiera plików i nie woła OpenGL, nagłówek nie dołącza GLAD, więc ma testy jednostkowe. Woła je `gfx::Shader` | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `src/gfx/UniformBuffer.*` | `gfx::UniformBuffer`: jeden bufor OpenGL używany jako bufor uniformów, czyli pamięć bloku uniformów, który czyta kilka programów. Konstruktor tworzy bufor o podanym rozmiarze (`glGenBuffers`, `glBindBuffer` i `glBufferData` z celem `GL_UNIFORM_BUFFER` i wskazówką `GL_DYNAMIC_DRAW`) i wpina go w punkt wiązania (`glBindBufferBase`). `update` kopiuje bajty na początek bufora (`glBufferSubData`) i loguje błąd, gdy danych jest więcej niż miejsca. Akcesory `bindingPoint` i `sizeInBytes`. RAII, tylko przenoszenie. Klasa przenosi same bajty i nie wie, co znaczą. Posiada ją `game::LightRig`. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/gfx/Buffer.*` | `gfx::Buffer`: jeden bufor OpenGL wypełniany w konstruktorze (`glGenBuffers`, `glBindBuffer`, `glBufferData` z podpowiedzią użycia, domyślnie `GL_STATIC_DRAW`, a od szóstej części M7 do wyboru, na przykład `GL_DYNAMIC_DRAW`) i, od szóstej części, od nowa metodą `setData` (`glBufferData` jeszcze raz), cel `GL_ARRAY_BUFFER` albo `GL_ELEMENT_ARRAY_BUFFER`, `bind`. RAII, tylko przenoszenie. Używa jej `gfx::Mesh` (do M4 także kostka w `NightMazeApp`, usunięta w M5) | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/VertexArray.*` | `gfx::VertexArray`: jeden obiekt tablicy wierzchołków (VAO), wiązany już w konstruktorze, `bind`, `setFloatAttribute` (`glEnableVertexAttribArray`, `glVertexAttribPointer`). RAII, tylko przenoszenie. Używa jej `gfx::Mesh` (do M4 także kostka w `NightMazeApp`, usunięta w M5) | [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) |
| `src/gfx/Texture2D.*` | typ `gfx::TextureFilter` (`Nearest`, `Bilinear`, `Trilinear`) i klasa `gfx::Texture2D`: jedna tekstura 2D z pełnym łańcuchem mipmap i jej obiekt samplera. Konstruktor przyjmuje surowe bajty (szerokość, wysokość, 3 albo 4 kanały, wskaźnik, dolny wiersz pierwszy) i, od pierwszej części M7, wymagany argument `gfx::ColorSpace`: `Srgb` daje format wewnętrzny `GL_SRGB8` albo `GL_SRGB8_ALPHA8` (karta dekoduje teksele do wartości liniowych przy odczycie), `Linear` daje `GL_RGB8` albo `GL_RGBA8`. Woła `glTexImage2D` oraz `glGenerateMipmap`. `bind(unit)` wiąże teksturę i sampler z jednostką teksturującą, `setFilter` i `setAnisotropy` zmieniają próbkowanie w działającym programie, akcesory `filter`, `anisotropy`, `maxAnisotropy`, `id`, `width`, `height`, `colorSpace` (od M7), `isValid`. Filtrowanie anizotropowe jest wykrywane jako rozszerzenie (od M7 przez `gfx::hasExtension`). RAII, tylko przenoszenie. Tworzy ją i trzyma `assets::AssetCache`, wiąże `game::drawModel` (`src/game/ModelDraw.cpp`), a jej `id()` czyta podgląd w panelu Assets. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/textures.md`](../modules/gfx/textures.md), przestrzeń kolorów w [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) |
| `src/gfx/ColorSpace.*` (M7) | typ `gfx::ColorSpace` (`Srgb`, `Linear`): co znaczą liczby koloru. Funkcje `gfx::srgbToLinear` i `gfx::linearToSrgb` dla jednego kanału (`float`) i dla koloru (`glm::vec3`): dokładna, dwuczęściowa funkcja standardu sRGB (odcinek prosty dla najciemniejszych wartości, powyżej krzywa potęgowa z wykładnikiem 2,4), z przycięciem argumentu do zakresu od 0 do 1. Zwykła matematyka bez OpenGL, w bibliotece `engine`, z testami. Wołają je `game::buildLightSet`, `NightMazeApp` (kolor tła, `crystalEmissive`) i `ColliderLines` | [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md). Komentarz `// See docs/...` w obu plikach wskazuje od czwartej części M7 ten sam dokument (do trzeciej wskazywał [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md)) |
| `src/gfx/Extensions.*` (M7) | funkcja `gfx::hasExtension(name)`: pyta sterownik, czy wymienia rozszerzenie o podanej nazwie (`glGetIntegerv(GL_NUM_EXTENSIONS)` i `glGetStringi`). Wydzielona z `Texture2D.cpp`, bo od M7 ma dwóch użytkowników: anizotropię w `Texture2D` i `debug::RawTextureSampler`. Wymaga kontekstu OpenGL, bez testu jednostkowego | [`../modules/gfx/textures.md`](../modules/gfx/textures.md) |
| `src/gfx/Framebuffer.*` (M7) | typy `gfx::ColorFormat` (`None`, `Rgba8`, `Rgba16F`) i `gfx::DepthFormat` (`None`, `Depth24`), struktura `gfx::FramebufferSpec` (szerokość, wysokość, dwa formaty), funkcje `colorFormatName`, `depthFormatName` i `framebufferStatusText` (teksty bez OpenGL, z testami) i klasa `gfx::Framebuffer`: jeden obiekt framebuffera z najwyżej jedną teksturą koloru i najwyżej jedną teksturą głębi. Konstruktor tworzy tekstury, podpina je i sprawdza kompletność (`glCheckFramebufferStatus`). `bind` wiąże framebuffer i ustawia viewport na jego rozmiar, statyczne `bindDefault(width, height)` wraca do okna, `resize` buduje wszystko od nowa w innym rozmiarze, `bindColorTexture` i `bindDepthTexture` wiążą załącznik do odczytu, akcesory `isValid`, `colorTextureId`, `depthTextureId`, `width`, `height`, `colorFormat`, `depthFormat`. RAII, tylko przenoszenie. Tworzy je i trzyma `game::PostProcess`, a od czwartej części M7 także `game::ShadowMap`: mapa cieni to framebuffer z `ColorFormat::None` i `DepthFormat::Depth24`, pierwszy w grze bez tekstury koloru (klasa woła dla niego `glDrawBuffer(GL_NONE)` i `glReadBuffer(GL_NONE)`), a jej podgląd to framebuffer `Rgba8` bez głębi | [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) |
| `src/gfx/ComparisonSampler.*` (M7, część czwarta) | klasa `gfx::ComparisonSampler`: jeden obiekt samplera OpenGL ustawiony do czytania mapy cieni. Ma trzy reguły, których zwykła tekstura głębi nie ma: porównanie (`GL_TEXTURE_COMPARE_MODE` równe `GL_COMPARE_REF_TO_TEXTURE`, funkcja `GL_LEQUAL`: shader podaje własną głębię przez `sampler2DShadow` i dostaje 1 dla "oświetlony" albo 0 dla "w cieniu"), filtr liniowy (karta porównuje cztery teksele wokół miejsca i miesza cztery odpowiedzi, czyli sprzętowy filtr 2 x 2; `setLinearFilter(false)` wraca do jednego porównania) i ramka (`GL_CLAMP_TO_BORDER` z głębią 1, stała `FAR_PLANE_BORDER`: poza mapą wszystko jest oświetlone). `bind(unit)` wpina sampler w jednostkę teksturującą, akcesor `linearFilter`. RAII, bez kopiowania. Reguły są w obiekcie samplera, a nie w teksturze, bo tę samą teksturę głębi czyta też podgląd, bez porównania. Tworzy go i trzyma `game::ShadowMap`. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md), teoria w [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.6 do 2.8. Komentarz `// See docs/...` w obu plikach wskazuje [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md) |
| `src/gfx/Cubemap.*` | klasa `gfx::Cubemap` (M6): jedna tekstura sześcienna i jej obiekt samplera. Konstruktor przyjmuje bok ściany, liczbę kanałów (3 albo 4) i sześć wskaźników na surowe bajty w kolejności +X, -X, +Y, -Y, +Z, -Z (typ `FacePixels`, górny wiersz pierwszy) oraz, od pierwszej części M7, wymagany argument `gfx::ColorSpace` (niebo: `Srgb`, czyli `GL_SRGB8`), woła `glTexImage2D` raz na ścianę, ustawia `GL_TEXTURE_MAX_LEVEL` na 0 (bez mipmap) i daje samplerowi filtr liniowy i `GL_CLAMP_TO_EDGE` na trzech osiach. `bind(unit)` wiąże teksturę i sampler z jednostką, akcesory `isValid`, `id`, `size`, konstruktor domyślny daje obiekt bez tekstury. RAII, tylko przenoszenie. Tworzy ją i trzyma `game::Skybox`. Nie ma testu jednostkowego (wymaga kontekstu OpenGL) | [`../modules/gfx/cubemap.md`](../modules/gfx/cubemap.md) |
| `src/gfx/Vertex.hpp` | `gfx::Vertex`: jeden wierzchołek modelu jako struktura (pola `position`, `normal`, `uv`, `tangent`: 11 liczb `float`, 44 bajty), stałe `POSITION_COMPONENTS`, `NORMAL_COMPONENTS`, `UV_COMPONENTS`, `TANGENT_COMPONENTS` i numery atrybutów `POSITION_ATTRIBUTE` (0), `NORMAL_ATTRIBUTE` (1), `UV_ATTRIBUTE` (2), `TANGENT_ATTRIBUTE` (3), dwa `static_assert` (rozmiar bez dopełnienia, układ standardowy). Sam nagłówek, bez GLAD. Używają jej `gfx::Mesh`, loader OBJ i testy | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcja 5.2 |
| `src/gfx/Mesh.*` | `gfx::Mesh`: siatka jednego modelu na karcie. Posiada `VertexArray`, bufor wierzchołków i bufor indeksów, w konstruktorze wysyła dane ze `std::span` i opisuje cztery atrybuty przez `sizeof(Vertex)` i `offsetof`. `draw()` rysuje całość, `draw(firstIndex, indexCount)` zakres indeksów (`glDrawElements`), prymityw jest parametrem konstruktora (domyślnie `GL_TRIANGLES`). RAII przez pola, tylko przenoszenie. Tworzą ją `assets::AssetCache` (siatka modelu z trójkątów, rysowana zakresami) i `game::ColliderLines` (dwie siatki z `GL_LINES`: krawędzie sześcianu jednostkowego, 8 wierzchołków i 24 indeksy, oraz okrąg jednostkowy, 32 wierzchołki i 64 indeksy). Nie ma testu jednostkowego (wymaga kontekstu OpenGL). Od M6 siatka może być też listą punktów (`GL_POINTS`, jeden indeks na punkt): tak `game::GrassRenderer` trzyma kępki trawy dla shadera geometrii. Siatkę terenu trzyma `game::TerrainRenderer` | [`../modules/gfx/mesh.md`](../modules/gfx/mesh.md), sekcje od 5.3 do 5.5 |
| `src/assets/ObjLoader.*` | struktury `assets::ObjPart`, `assets::ObjMaterial`, `assets::ObjModel` i funkcje `assets::parseObj` (tekst OBJ na wierzchołki, indeksy i części), `assets::parseMtl` (tekst MTL na materiały: `newmtl`, `Kd`, `map_Kd` i linia mapy normalnych `map_Bump` z opcją `-bm`) oraz `assets::loadObj` (plik OBJ razem z plikami MTL, ścieżki tekstur względem pliku MTL). `parseObj` na końcu liczy styczne wierzchołków (`assets::computeTangents`) i zapisuje liczbę trójkątów z lustrzaną teksturą w `ObjModel::mirroredTriangleCount`, a `loadObj` ostrzega w logu, gdy jest większa od zera. Ręcznie napisany parser, bez OpenGL i bez wyjątków: wynik `bool` i tekst błędu z numerem linii. `loadObj` woła `assets::AssetCache::model` | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), wstęp do warstwy w [`../modules/assets/README.md`](../modules/assets/README.md) |
| `src/assets/Tangents.*` | funkcje `assets::triangleTangents` (styczna i bitangenta jednego trójkąta z krawędzi i różnic UV), `assets::computeTangents` (styczna każdego wierzchołka: suma po trójkątach, ortogonalizacja Grama-Schmidta względem normalnej, długość 1, wartość zastępcza zamiast `NaN`) i `assets::countMirroredTriangles` (trójkąty, na których `cross(N, T)` wskazuje przeciwnie do bitangenty). Sama matematyka na GLM, bez OpenGL. Woła je `assets::parseObj` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcje od 5.5 do 5.7, notatka [`../decisions/tangents-on-load.md`](../decisions/tangents-on-load.md) |
| `src/assets/ImageLoader.*` | struktura `assets::Image` (szerokość, wysokość, liczba kanałów, bajty pikseli z dolnym wierszem jako pierwszym), od M6 typ `assets::RowOrder` (`BottomFirst` dla tekstur 2D, domyślny, i `TopFirst` dla ścian tekstury sześciennej, których wierszy się nie odwraca) i funkcja `assets::loadImage`: czyta plik w trybie binarnym, dekoduje go biblioteką stb_image i odwraca kolejność wierszy. Wynik `bool`, tekst błędu przez referencję, jedno logowanie, bez wyjątków. Bez OpenGL. Jedyny plik projektu, który dołącza `<stb_image.h>`. `loadImage` woła `assets::AssetCache::texture`, od M6 także `game::Skybox` (ściany nieba) i funkcja `loadHeightmap` w `NightMazeApp.cpp` (mapa wysokości), obie z `RowOrder::TopFirst` | [`../modules/assets/images.md`](../modules/assets/images.md) |
| `src/assets/AssetCache.*` | struktury `assets::LoadedTexture`, `assets::ModelPart`, `assets::LoadedModel` i klasa `assets::AssetCache`: wczytuje każdy model (`model`) i każdą teksturę (`texture`) raz, pod kluczem będącym uporządkowaną ścieżką. Od pierwszej części M7 `texture(path, colorSpace)` ma drugi, wymagany argument: obraz koloru (`map_Kd`, grunt) jest wczytywany jako `Srgb`, mapa normalnych jako `Linear`, a prośba o plik wczytany już z inną przestrzenią kolorów wypisuje błąd i zwraca teksturę taką, jaka jest. Biała tekstura zastępcza jest `Srgb`, płaska mapa normalnych `Linear`. Pamięć trzyma je w `std::deque` (wskaźniki pozostają ważne), pamięta ścieżki, których nie udało się wczytać (`failedPaths`), ma dwie tekstury zastępcze 1 x 1 (białą `whiteTexture` i płaską mapę normalnych `flatNormalTexture`) oraz `setFilter` i `setAnisotropy` dla wszystkich tekstur naraz. Każda część modelu (`ModelPart`) ma teksturę koloru i mapę normalnych, nigdy puste. Jedyny plik `src/assets/`, który tworzy obiekty OpenGL (przez `gfx::Mesh` i `gfx::Texture2D`), więc nie ma testu jednostkowego. Posiada ją `NightMazeApp` | [`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md) |
| `external/stb/stb_image.c` | jedyny plik, w którym kompiluje się implementacja stb_image: makro `STB_IMAGE_IMPLEMENTATION` i dołączenie nagłówka pobranego przez FetchContent. Plik C, poza naszymi ostrzeżeniami, tworzy bibliotekę `stb_image` | [`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2 |
| `src/scene/Transform.*` | `scene::Transform`: struktura z publicznymi polami `position`, `rotationDegrees` (kąty Eulera w stopniach) i `scale` oraz funkcją `matrix()`, która zwraca macierz modelu `T * Ry * Rx * Rz * S`. Od M4 także funkcja `scene::normalMatrix`: macierz normalnych, czyli odwrotność lewej górnej części 3 x 3 macierzy modelu po transpozycji (`glm::mat3`). Sama matematyka na GLM, bez OpenGL. Struktury używają `game::buildMazeWorld` (macierze podłogi, ścian i słupków, przez `placedAt` i `wallModelMatrix`), `game::GameplayRenderer` (macierz każdego kryształu: pozycja, która się kołysze, i obrót wokół osi Y) i `game::ColliderLines` (macierz każdego pudełka i każdego okręgu kuli), a `normalMatrix` woła `game::drawModel` dla każdego rysowanego obiektu | [`../modules/scene/transforms.md`](../modules/scene/transforms.md), wstęp do warstwy w [`../modules/scene/README.md`](../modules/scene/README.md) |
| `src/scene/Light.*` | światła jako zwykłe dane i ich matematyka: stała `MAX_POINT_LIGHTS` (16), struktury `scene::Attenuation` (trzy składniki zaniku z odległością), `DirectionalLight` (księżyc), `PointLight`, `SpotLight` (latarka, dwa kąty stożka), `LightSet` (wszystkie światła jednej klatki: otoczenie, światło kierunkowe, tablica świateł punktowych z licznikiem, reflektor z wyłącznikiem) i `ConeCosines`, stałe `BRIGHTNESS_AT_RADIUS` (0,05) i `MIN_CONE_COSINE_GAP`, funkcje `attenuationForRadius` (składniki 1, 2 / r i 17 / r^2), `attenuationFactor`, `coneCosines`, `spotFactor` i `directionFromAngles`. Bez OpenGL: te same wzory liczy shader, funkcje C++ przygotowują dane i służą testom | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `src/scene/LightBlock.*` | obraz bloku uniformów `LightBlock` w C++: struktury `scene::PointLightData` (3 razy `glm::vec4`, 48 bajtów) i `scene::LightBlockData` (928 bajtów, pola w kolejności deklaracji w GLSL, z jawnym wypełnieniem po liczniku świateł), asercje `static_assert` z `offsetof` i `sizeof`, które pilnują układu `std140` w czasie kompilacji, oraz funkcja `scene::packLightBlock` (wypełnia blok z `LightSet` i pozycji oka: normalizuje kierunki, zamienia kąty stożka na cosinusy, ogranicza liczbę świateł punktowych). Bez OpenGL. Woła ją `game::LightRig::upload` | [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/scene/LightSpace.*` (M7, część czwarta) | przestrzeń światła, czyli scena tak, jak widzi ją światło: stałe `LIGHT_BOX_MARGIN` (0,5 m) i `VERTICAL_DIRECTION_LIMIT` (0,999), struktura `scene::LightSpace` (macierze `view` i `projection`, wektor `extent`: szerokość, wysokość i głębokość pudełka w metrach, funkcja `matrix()` równa `projection * view`), funkcja `directionalLightSpace` (rzut ortograficzny światła kierunkowego: pudełko obrócone wzdłuż kierunku światła i dopasowane do ośmiu narożników podanego `scene::Aabb` plus margines; wynik zależy tylko od pudełka i kierunku, nie od kamery; dla światła prawie pionowego kierunkiem "w górę" jest -Z, a kierunek o długości 0 jest zastępowany kierunkiem prosto w dół) i funkcja `shadowMapCoordinates` (punkt świata we współrzędnych mapy cieni: `x` i `y` od 0 do 1 wewnątrz mapy, `z` jako głębia, którą mapa zapisuje). Sama matematyka na GLM, bez OpenGL, więc ma testy. Woła je `NightMazeApp::drawMoonShadowMap`, a te same kroki ma `common/shadows.glsl` Od piątej części M7: wyliczenie `LightProjection`, pola `kind`, `position`, `nearPlane` i `farPlane` struktury, stałe `SPOT_NEAR_PLANE` (0,05 m), `SPOT_CONE_MARGIN_DEGREES` (2) i granice kąta otwarcia (1 i 170 stopni) oraz funkcja `spotLightSpace` (rzut perspektywiczny dla latarki). | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.2, 2.3 i 2.5, notatka [`../decisions/shadow-box-fitted-to-terrain.md`](../decisions/shadow-box-fitted-to-terrain.md) |
| `src/scene/Camera.*` | `scene::Camera`: struktura z publicznymi polami `position`, `yawDegrees`, `pitchDegrees`, `fovDegrees`, `nearPlane`, `farPlane`, stałymi `WORLD_UP` i `MAX_PITCH_DEGREES` oraz funkcjami `forward`, `right`, `rotate`, `viewMatrix`, `projectionMatrix`. Sama matematyka na GLM, bez OpenGL i bez wejścia. Używają jej `NightMazeApp` (pole `m_camera`) i `game::Player::update` (jako kalkulator kierunków `forward` i `right`) | [`../modules/scene/camera.md`](../modules/scene/camera.md) |
| `src/scene/Collider.*` | `scene::Aabb` (pudełko o ścianach równoległych do osi: pola `min` i `max`, funkcja `fromCenter`), stała `CONTACT_TOLERANCE`, funkcje `scene::overlaps` dla dwóch pudełek (czy na siebie nachodzą) i `scene::moveAndSlide` (o ile wolno przesunąć pudełko wśród przeszkód, oś po osi, ze ślizganiem po ścianach). Od M5 także drugi kształt: `scene::Sphere` (pola `center` i `radius`), `overlaps` dla dwóch kul (kwadrat odległości środków mniejszy od kwadratu sumy promieni), `overlaps` dla kuli i pudełka oraz `closestPoint` (punkt pudełka najbliższy danemu punktowi, przez `glm::clamp`). Kule, które się tylko stykają, nie nachodzą na siebie. Sama matematyka na GLM, bez OpenGL i bez wejścia. Pudełka tworzą `game/MazeLayout`, `game::exitZone` i `game::Player::box`, `moveAndSlide` woła `game::Player::update` w każdym kroku chodzenia, a kule tworzą `game::playerReach` i zbieranie kryształów w `game::updateRound` | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `src/scene/Raycast.*` (M8, tylko podstawy bez okna) | promień jako dane i matematyka: struktury `scene::Ray`, `RayHit` i `NearestHit`, funkcje `scene::intersect` (promień z pudełkiem metodą płyt i promień z kulą), `scene::nearestHit` (najbliższe pudełko w zasięgu) i `scene::screenPointRay` (promień przez punkt obrazu: odwrócenie `y`, macierz odwrotna, dzielenie przez `w`). Sama matematyka na GLM, bez OpenGL. Użytkownik: `game::pickInteractable`. Nie podpięte do pętli klatki ani do panelu Collision | [`../modules/scene/picking.md`](../modules/scene/picking.md) |
| `src/game/Maze.*` | typ `game::Direction` (North, East, South, West), stałe `DIRECTION_COUNT` i `ALL_DIRECTIONS`, funkcje `opposite`, `columnStep`, `rowStep`, struktura `game::MazeCell` (komórka jako para `x`, `z`, z porównaniem `==`, od M5), klasa `game::Maze`: siatka komórek ze ścianami na krawędziach (`width`, `height`, `contains`, `hasWall`, `removeWall`, stała `MAX_SIZE`) i funkcja `isDeadEnd` (komórka ze ścianami z dokładnie trzech stron, w M5 przeniesiona tu z `Lighting.*`). Bez OpenGL. Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| `src/game/MazeGenerator.*` | `game::randomBelow` (losowa liczba poniżej granicy, taka sama na każdym systemie) i `game::generateMaze` (labirynt doskonały z rozmiaru i ziarna, algorytm recursive backtracker z własnym stosem). Część biblioteki `game_logic` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 5.4 i 5.5 |
| `src/game/MazeLayout.*` | stałe wymiarów w metrach (`CELL_SIZE`, `WALL_LENGTH`, `WALL_HEIGHT`, `PILLAR_SIZE`, `WALL_VISUAL_THICKNESS`, `WALL_COLLISION_THICKNESS`, `PILLAR_HEIGHT`), typy `WallAxis` i `WallSegment`, funkcje `cellCenter`, `wallSegments`, `wallSegmentOn` (segment na wskazanym boku komórki, także otwartym: tak powstaje brama, od M5), `pillarPositions`, `wallBox`, `pillarBox`, `mazeColliders`. Część biblioteki `game_logic`. Woła je `game::buildMazeWorld`. Od M6 funkcje układu dają pozycje na `y = 0`, a na teren stawia je `game::placeOnTerrain`. Doszła funkcja `colliderBoxes` (pudełka z gotowych list ścian i słupków), którą woła `mazeColliders` i `placeOnTerrain` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5 |
| `src/game/MazeWorld.*` | stałe `DEFAULT_MAZE_WIDTH`, `DEFAULT_MAZE_HEIGHT` (10) i `DEFAULT_MAZE_SEED` (1), struktura `game::MazeSettings` (prośba o następny labirynt: rozmiar, ziarno, flaga `regenerate`), struktura `game::MazeWorld` (labirynt, ziarno, segmenty ścian, słupki, macierze modelu ścian i słupków (macierzy płytek podłogi nie ma od M6), pudełka kolizji ścian i słupków w `colliders`, pozycja startu, yaw startu, a od M5 komórka wyjścia `exitCell` i jej środek `exitPosition`, brama `hasGate`, `gate` i jej pudełko `gateBox`, strefa wyjścia `exitZone` i lista kryształów `crystals`), funkcje `yawTowards`, `wallModelMatrix` (macierz modelu segmentu ściany, używana także dla bramy) i `buildMazeWorld`. Pola z pozycjami świateł punktowych, które było tu w M4, już nie ma. Zwykłe dane bez OpenGL. Część biblioteki `game_logic`. Od drugiej części M6 świat stoi na terenie: pole `terrain` (`game::Terrain`), stała `FOOTPRINT_MARGIN` (0,05 m), funkcje `groundHeightAt` (wysokość gruntu w środku komórki) i `placeOnTerrain` (buduje teren i zatapia ściany, słupki i bramę do najniższego gruntu pod nimi, przelicza macierze, pudełka, start, wyjście i strefę wyjścia) oraz drugie przeciążenie `buildMazeWorld` z mapą wysokości i skalą. Przeciążenie bez mapy daje płaski grunt na `y = 0` | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), wyjście i kryształy w [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `src/game/Exit.*` | stałe `UNREACHABLE` (-1) i `EXIT_ZONE_HALF_SIZE` (0,5 m), funkcje `game::passageDistances` (dla każdej komórki liczba przejść na najkrótszej drodze od startu, przeszukiwanie wszerz), `farthestCell` (komórka najdalsza od startu, przy remisie pierwsza w kolejności wierszy), struktura `ExitPlacement` (`cell`, `hasGate`, `gate`), `placeExit` (wyjście w najdalszej komórce, brama na jej pierwszym otwartym boku w kolejności `ALL_DIRECTIONS`) i `exitZone` (pudełko 1 x 1 m na środku komórki wyjścia, wysokie jak ściany). Bez OpenGL. Część biblioteki `game_logic`. Woła je `game::buildMazeWorld`. Od M6 `exitZone` dostaje drugi argument, wysokość gruntu w środku komórki: strefa stoi na terenie | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), notatka [`../decisions/exit-farthest-cell.md`](../decisions/exit-farthest-cell.md) |
| `src/game/Crystals.*` | stałe `CELLS_PER_CRYSTAL` (8), `CRYSTAL_VARIANT_COUNT` (2) i stałe ruchu i blasku kryształu (`CRYSTAL_HEIGHT`, `CRYSTAL_FLOAT_HEIGHT`, `CRYSTAL_BOB_AMPLITUDE`, `CRYSTAL_BOB_SECONDS`, `CRYSTAL_SPIN_DEGREES_PER_SECOND`, `CRYSTAL_LIGHT_CLEARANCE`, `CRYSTAL_PULSE_DEPTH`, `CRYSTAL_PULSE_SECONDS`, `CRYSTAL_GLOW_STRENGTH`), struktura `game::CrystalSpawn` (komórka i wariant modelu), funkcje `crystalCountFor` (jeden kryształ na osiem komórek, od 1 do `scene::MAX_POINT_LIGHTS`), `placeCrystals` (komórki kryształów z ziarna: nigdy start ani wyjście, najpierw ślepe zaułki), `crystalRestPosition`, `crystalCenter`, `crystalLightPosition`, `crystalBobPosition`, `crystalSpinDegrees`, `crystalPulse` i `crystalGlow` (wartość uniformu `uEmissive`: od pierwszej części M7 liczona z koloru liniowego, a `CRYSTAL_GLOW_STRENGTH` wynosi od drugiej części M7 4,0 (w pierwszej 2,5, do M6 1), więc blask jest w buforze HDR jaśniejszy niż biel i po nim znajduje kryształy bloom). Bez OpenGL. Część biblioteki `game_logic`. Od M6 `crystalRestPosition` dostaje drugi argument, wysokość gruntu w środku komórki: kryształ unosi się `CRYSTAL_FLOAT_HEIGHT` nad terenem | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), notatka [`../decisions/crystal-count-and-gate-threshold.md`](../decisions/crystal-count-and-gate-threshold.md) |
| `src/game/Round.*` | stałe `GATE_OPEN_SECONDS` (1,5), `GATE_SINK_DEPTH` (3,3 m), `PLAYER_REACH_HEIGHT` i `PLAYER_REACH_RADIUS`, struktura `game::GameplaySettings` (liczby reguł do zmiany w działającej grze: `requiredFraction`, `batteryLifetimeSeconds`, `batteryPerCrystal`, `lowBatteryThreshold`, `pickupRadius`, `batteryDrains` i prośba `restart`), typ `RoundState` (`Playing`, `Won`: stanu przegranej nie ma), struktury `RoundCrystal` i `Round` (stan jednej rundy: kryształy, liczniki, brama, bateria, dwa zegary), funkcje `requiredCrystalCount`, `startRound`, `playerReach`, `updateRound` (jeden stały krok reguł: zegary, opadanie bramy, bateria, zbieranie, otwarcie bramy, wygrana, wyłączenie latarki przy pustej baterii), `gateBlocks`, `gateVisible`, `gateSinkDepth`, `roundObstacles`, `flashlightFlicker`, `lightingForFrame` i `crystalLightPositions`. Bez OpenGL. Część biblioteki `game_logic`. Od M6 funkcja `restCrystalsOnGround` przestawia pozycje spoczynku kryształów rundy po przebudowie terenu, bez zmiany tego, które są zebrane | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), bateria i światła w [`../modules/game/flashlight.md`](../modules/game/flashlight.md), notatka [`../decisions/battery-darkness-no-loss.md`](../decisions/battery-darkness-no-loss.md) |
| `src/game/Interactables.*` (M8, tylko podstawy bez okna) | dźwignie i kartki jako dane i matematyka: `game::WallRef`, `Lever`, `Note`, `Interactables`, `InteractableState`, `PullResult`, `PickedInteractable`, funkcje `chooseShortcutWalls`, `placeInteractables`, `leverBox`, `noteBox`, `pullLever`, `openedWalls`, `pickInteractable`, `compassTowards`, `noteText`. Nie podpięte do `MazeWorld`, rundy ani rysowania | [`../modules/game/interactables.md`](../modules/game/interactables.md) |
| `src/game/Lighting.*` | typy `game::LightingMode` (`Unlit`, `Gouraud`, `Phong`, `BlinnPhong`: pozycje listy `Lighting` w panelu Renderer) i `game::SpecularModel` (wartości uniformu `uSpecularModel`), funkcja `specularModelOf`, struktura `game::LightingSettings` (wszystko, co da się zmienić w działającej grze: tryb, światło otoczenia, kąty, kolor i natężenie księżyca, wyłącznik, kolor, natężenie, stożek i zasięg latarki, kolor, natężenie i promień świateł punktowych, siła i wykładnik połysku), pole `normalMapping` (przełącznik map normalnych, startowo włączony) i funkcja `usesNormalMap` (włączone i tryb inny niż `Gouraud`) oraz `buildLightSet` (światła jednej klatki z ustawień, oka, kierunku patrzenia i listy pozycji świateł punktowych, którą od M5 podaje `game::crystalLightPositions`). Od pierwszej części M7 cztery kolory ustawień są wartościami sRGB, a `buildLightSet` przelicza je na liniowe (`gfx::srgbToLinear`). Wartości startowe dobrane od nowa: otoczenie `(0,105, 0,135, 0,225)`, natężenie księżyca 0,12 (od czwartej części M7 0,2, żeby miejsce w świetle księżyca było wyraźnie jaśniejsze od miejsca w cieniu ściany), latarki 1,3, świateł punktowych 0,9. Od czwartej części M7 także funkcja `moonDirection`: kierunek, w którym leci światło księżyca, z dwóch kątów ustawień. Biorą go z niej i `buildLightSet`, i mapa cieni, więc nie mogą się rozjechać. Kod świateł w ślepych zaułkach z M4 (stała wysokości światła i funkcja wybierająca zaułki) został usunięty w M5, a `isDeadEnd` przeniesiona do `Maze.*`. Zwykłe dane i matematyka bez OpenGL. Część biblioteki `game_logic` Od piątej części M7: struktura `FlashlightPose`, funkcja `flashlightPose` (latarka w ręce: 0,2 m na prawo, 0,25 m w dół, wiązka zbieżna z osią widoku 4 m przed okiem), stałe `MAX_FLASHLIGHT_HAND_RIGHT` (0,25) i `MIN_FLASHLIGHT_CONVERGE_DISTANCE` (0,5), trzy nowe pola `LightingSettings` i nowa sygnatura `buildLightSet(settings, pose, pointPositions)`. | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `src/game/Player.*` | struktura `game::PlayerInput` (siedem pól `bool`: czego gracz chce w jednym kroku) i struktura `game::Player`: stałe ciała i prędkości (`BODY_WIDTH`, `BODY_HEIGHT`, `EYE_HEIGHT`, `WALK_SPEED`, `SPRINT_SPEED`, `FLY_SPEED`; stałą `FLOOR_Y` usunął M6), pola `position` (stopy), `noclip`, `walkSpeed`, `sprintSpeed`, `flySpeed`, funkcje `box`, `eyePosition` i `update` (jeden stały krok: chodzenie przez `scene::moveAndSlide` albo lot bez kolizji. Od M6 dostaje też `const Terrain&` i po ruchu stawia stopy na wysokości `Terrain::heightAt`). Bez OpenGL, bez klawiatury i bez zegara. Część biblioteki `game_logic` | [`../modules/game/player.md`](../modules/game/player.md) |
| `src/game/MazeRenderer.*` | typ `game::ViewMode` (`Textured`, `Normals`, `Uvs`: wartości uniformu `uViewMode`) i klasa `game::MazeRenderer`: prosi pamięć podręczną o dwa modele labiryntu, ścianę i słupek (do M5 trzy, z płytką podłogi) i rysuje `MazeWorld`, jedno wywołanie rysujące na obiekt. Program shaderów dostaje z zewnątrz: `textured` albo jeden z dwóch oświetlonych (`lit`, `gouraud`). Ustawia `uEmissive` na czerń (kamień sam nie świeci) i woła `game::drawModel` dla ścian i słupków. Grunt pod nimi rysuje od M6 `game::TerrainRenderer`. Niczego nie posiada. Część programu `night_maze` (potrzebuje kontekstu OpenGL) | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `src/game/ModelDraw.*` | funkcje wspólne dla klas rysujących modele i teren. Dwie pierwsze, dla `MazeRenderer` i `GameplayRenderer`: `game::setModelSamplers` (mówi samplerom `uTexture` i `uNormalMap`, z których jednostek teksturujących czytać: 0 i 1) i `game::drawModel` (rysuje jeden model raz dla każdej macierzy z listy: dla każdej części modelu wiąże mapę normalnych i obraz koloru i wysyła `uTint`, dla każdego obiektu wysyła `uModel` i `uNormalMatrix`, czyli `scene::normalMatrix` liczoną na procesorze w każdej klatce). W M4 ten kod był prywatną funkcją klasy `MazeRenderer`. Część programu `night_maze`. Od M6 trzecia funkcja, `game::drawMesh`: rysuje raz całą siatkę, która nie pochodzi z pliku modelu, z podaną teksturą, mapą normalnych, kolorem i macierzą. Woła ją `TerrainRenderer`, a `setModelSamplers` wspólnie wszystkie trzy klasy | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `src/game/GameplayRenderer.*` | klasa `game::GameplayRenderer`: prosi pamięć podręczną o dwa modele kryształów i model bramy i rysuje to, co zmienia się w rundzie. Bramę rysuje macierzą `wallModelMatrix` z pozycją obniżoną o `gateSinkDepth`, dopóki `gateVisible` jest prawdą, z `uEmissive` równym czerni. Każdy niezebrany kryształ rysuje własną macierzą (`crystalBobPosition`, `crystalSpinDegrees`) z `uEmissive` równym `crystalGlow`. Program shaderów jest ten sam, którym narysowano labirynt. Niczego nie posiada. Część programu `night_maze` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `src/game/LightRig.*` | klasa `game::LightRig`: strona OpenGL oświetlenia. Posiada bufor uniformów (`gfx::UniformBuffer` o rozmiarze `scene::LightBlockData`, punkt wiązania `LIGHT_BLOCK_BINDING_POINT`) i nic więcej. `connect` łączy blok `LightBlock` programu z tym buforem (`Shader::bindUniformBlock`), `upload` pakuje `scene::LightSet` (`scene::packLightBlock`) i kopiuje bajty do bufora raz na klatkę. Siatki kostki znacznika i funkcji, która w M4 rysowała te kostki, już nie ma: widocznym źródłem każdego światła punktowego jest kryształ, rysowany przez `game::GameplayRenderer`. Część programu `night_maze` | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `src/game/ColliderLines.*` | klasa `game::ColliderLines`: posiada dwie siatki z `GL_LINES`, 12 krawędzi sześcianu o boku 1 (`UNIT_CUBE_CORNERS`, `UNIT_CUBE_EDGES`) i okrąg o promieniu 1 (`CIRCLE_SEGMENTS` równe 32). `draw` rysuje dowolną listę pudełek `scene::Aabb` w jednym kolorze (od M7 kolor jest wartością sRGB, przeliczaną na liniową przed wysłaniem do `uColor`), każde powiększone o 1 cm. `drawSpheres` (od M5) rysuje każdą kulę `scene::Sphere` jako trzy okręgi: jeden poziomy i dwa pionowe (`CIRCLE_ROTATIONS`). Część programu `night_maze` | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `src/game/Skybox.*` | struktura `game::SkyboxSettings` (`enabled`, `brightness`, startowo 2,2, do M6 1,0) i klasa `game::Skybox` (M6): niebo. Konstruktor wczytuje sześć plików z `assets/skybox/` loaderem obrazów z `RowOrder::TopFirst`, sprawdza, że są kwadratami jednej wielkości, tworzy z nich `gfx::Cubemap` (od M7 z `gfx::ColorSpace::Srgb`) i siatkę sześcianu (`gfx::Mesh`, 8 wierzchołków, 36 indeksów). `draw` wybiera program `skybox`, ustawia jego pięć uniformów, wiąże teksturę sześcienną z jednostką 0, włącza `GL_TEXTURE_CUBE_MAP_SEAMLESS`, na czas jednego wywołania rysującego ustawia test głębi `GL_LEQUAL` i wyłącza zapis głębi, a potem przywraca `GL_LESS` i zapis. Gdy plików albo programu brakuje, nic nie rysuje. Wymaga kontekstu OpenGL, więc jest w programie, a nie w `game_logic`, i nie ma testu jednostkowego | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md) |
| `src/game/Terrain.*` | teren jako dane (M6): stałe (`TERRAIN_STEPS_PER_CELL` 4, `TERRAIN_SPACING` 0,5 m, `TERRAIN_MARGIN_CELLS` 7, `TERRAIN_MARGIN` 14 m, `HEIGHTMAP_SPAN` 48 m, `MAZE_RELIEF` 0,6, `HILL_RELIEF` 4,5, `DEFAULT_HEIGHT_SCALE` 1, `MAX_HEIGHT_SCALE` 2,5, `GROUND_TEXTURE_SPAN` 4 m), struktury `game::TerrainSettings` (`heightScale`, `wireframe`, flaga `rebuild`) i `game::Heightmap` (liczby od 0 do 1 z odczytem dwuliniowym `sample`, który się powtarza), funkcje `heightmapFromImage`, `distanceOutsideMaze` i `terrainRelief`, klasa `game::Terrain` (siatka wysokości: `gridPoint`, `gridNormal`, `heightAt` na trójkącie siatki, `lowestHeightUnder`, `minHeight`, `maxHeight`, `triangleCount`), struktura `game::TerrainMeshData` i funkcja `buildTerrainMesh` (wierzchołki z normalnymi, UV i stycznymi oraz indeksy). Bez OpenGL. Część biblioteki `game_logic` | [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) |
| `src/game/TerrainRenderer.*` | klasa `game::TerrainRenderer` (M6): strona OpenGL terenu. Posiada jedną siatkę (`gfx::Mesh`), a dwie tekstury gruntu (`textures/ground.png`, `textures/ground_normal.png`) bierze z pamięci assetów, z białą teksturą i płaską mapą normalnych jako wyjściem awaryjnym. `upload` wymienia siatkę po każdej przebudowie terenu, `draw` rysuje ją programem podanym z zewnątrz (`textured`, `lit` albo `gouraud`) przez `game::drawMesh`, z macierzą jednostkową, a przy `wireframe` przełącza na czas tego wywołania `glPolygonMode` na linie. Część programu `night_maze`, bez testu jednostkowego | [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) |
| `src/game/Grass.*` | trawa jako dane (M6): stałe (`GRASS_END_CLEARANCE`, `GRASS_WALL_GAP`, `GRASS_STRIP_WIDTH`, `GRASS_HILL_TUFTS_PER_SQUARE_METRE`, `GRASS_HILL_CLEARANCE`, `DEFAULT_GRASS_DENSITY` 2,5, `MAX_GRASS_DENSITY` 8), struktury `game::GrassTuft` (pozycja korzenia i liczba losowa od 0 do 1) i `game::GrassSettings` (`enabled`, `density`, `bladeHeight`, `windStrength`, flaga `replant`), funkcja `game::placeGrass`: miejsca kępek wzdłuż obu stron każdej ściany i rzadki rozsiew na wzgórzach, wybierane z ziarna labiryntu przez `std::mt19937` i `randomBelow`. Źdźbeł tu nie ma: buduje je shader geometrii. Bez OpenGL. Część biblioteki `game_logic` | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md) |
| `src/game/GrassRenderer.*` | klasa `game::GrassRenderer` (M6): strona OpenGL trawy. Posiada siatkę punktów (`gfx::Mesh` z `GL_POINTS`), po jednym wierzchołku na kępkę: pozycja to korzeń kępki w przestrzeni świata, a liczba losowa kępki jedzie we współrzędnej `u`. `upload` wymienia siatkę, `tuftCount` zwraca liczbę kępek, `draw` wybiera program trawy, ustawia jego uniformy (`uView`, `uProjection`, `uTime`, `uBladeHeight`, `uWindStrength`, `uLit`, `uViewMode`, dwie liczby połysku) i rysuje wszystkie punkty jednym wywołaniem, a na czas rysowania wyłącza odrzucanie tylnych ścian, jeśli było włączone. Część programu `night_maze`, bez testu jednostkowego | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md) |
| `src/game/PostProcess.*` (M7) | wyliczenia `game::ToneMapping` (`None`, `Reinhard`, `Aces`: wartości uniformu `uToneMapping` i pozycje listy w panelu) i `game::AttachmentPreview` (`Color`, `Depth`, od czwartej części M7 także `RawDepth`, głębia tak, jak jest zapisana, dla podglądu mapy cieni: wartości uniformu `uMode`), struktura `game::PostProcessSettings` (`exposure` 1,0, `toneMapping` `Aces`, `previews`, `depthPreviewRange` 15 m, od drugiej części M7 `bloom` typu `BloomSettings`, od trzeciej `fog` typu `FogSettings` i `vignette` typu `VignetteSettings`), od trzeciej części M7 struktura `game::SceneView` (`inverseViewProjection`, macierz odwrotna do `projection * view`, i `eye`, pozycja oka: to, czego mgła potrzebuje, żeby z głębi odtworzyć miejsce w świecie) i klasa `game::PostProcess`: posiada framebuffer sceny (`GL_RGBA16F` i `GL_DEPTH_COMPONENT24`), od drugiej części M7 trzy cele bloomu (`GL_RGBA16F` bez głębi, połowa rozmiaru sceny: `m_brightPass`, `m_blurHorizontal`, `m_bloom`), cztery małe framebuffery podglądów (`GL_RGBA8`, wysokość 180 pikseli) i pusty obiekt tablicy wierzchołków dla trójkąta na cały ekran. `beginScene(size)` wiąże framebuffer sceny i tworzy go od nowa, gdy rozmiar okna się zmienił, `drawPreviews` rysuje dwa obrazy załączników programem `preview`, `drawBloom` (druga część M7) rysuje przebieg jasności programem `bright`, rozmycie programem `blur` (od 1 do 10 powtórzeń, każde jako przebieg poziomy i pionowy, z ping-pongiem między dwoma celami) i przy otwartym panelu dwa podglądy, `composite` wraca do framebuffera okna i przenosi obraz programem `composite` (od trzeciej części M7 mgła liczona z tekstury głębi sceny na trzeciej jednostce teksturującej, wiązanej tylko przy włączonej mgle, potem bloom z drugiej jednostki, ekspozycja, mapowanie tonów, winieta, kodowanie sRGB; czwarty parametr to `const SceneView&`, a kolor mgły jest tu raz na klatkę przeliczany z sRGB funkcją `gfx::srgbToLinear`), akcesory `sceneTarget`, `preview`, `bloomDrawn`, `bloomTarget`, `brightPassPreview` i `bloomPreview`. Wymaga kontekstu OpenGL, więc jest w programie, a nie w `game_logic`, i nie ma testu jednostkowego | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), notatka [`../decisions/post-process-in-game-layer.md`](../decisions/post-process-in-game-layer.md) |
| `src/game/Bloom.*` (M7, część druga) | bloom jako dane i matematyka, bez OpenGL: stałe `BLOOM_DOWNSCALE` (2), `BLOOM_BLUR_RADIUS` (6), `BLOOM_BLUR_WEIGHT_COUNT` (7), `BLOOM_BLUR_SIGMA` (3,0), `MIN_BLOOM_BLUR_ITERATIONS` (1) i `MAX_BLOOM_BLUR_ITERATIONS` (10), struktura `game::BloomSettings` (`enabled` prawda, `threshold` 0,8, `intensity` 1,0, `blurIterations` 6), funkcje `bloomTargetExtent` (rozmiar celu: połowa sceny, dzielenie całkowite, nie mniej niż 1) i `bloomBlurWeights` (siedem wag rozdzielnego rozmycia Gaussa, znormalizowanych do sumy 1). Część biblioteki `game_logic`, więc ma testy. Same przebiegi rysuje `PostProcess` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 5.10, notatki [`../decisions/bloom-half-resolution-three-targets.md`](../decisions/bloom-half-resolution-three-targets.md) i [`../decisions/blur-weights-computed-on-cpu.md`](../decisions/blur-weights-computed-on-cpu.md) |
| `src/game/Fog.*` (M7, część trzecia) | mgła jako dane i matematyka, bez OpenGL: struktura `game::FogSettings` (`enabled` prawda, `density` 0,1 na metr, `baseHeight` 0,5 m, `heightFalloff` 0,4 na metr, `color` jako wartość sRGB `(0,14, 0,18, 0,26)`), funkcje `fogHeightFactor` (`exp(-heightFalloff * max(height - baseHeight, 0))`: 1 na wysokości bazowej i pod nią), `fogAmount` (`1 - exp(-density * heightFactor * distance)`), `fogAmountAt` (obie razem dla punktu świata widzianego z oka, z komentarzem `KNOWN LIMIT`: wysokość jest brana tylko w punkcie, nie wzdłuż linii wzroku) i `worldPositionFromDepth` (miejsce w świecie ze współrzędnej tekstury, głębi i macierzy odwrotnej do `projection * view`). Te same wzory pod tymi samymi nazwami ma `post/composite.frag`: oba pliki muszą się zgadzać. Część biblioteki `game_logic`, więc ma testy. Samą mgłę liczy shader przebiegu składającego | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.17 do 2.21 i 5.12, notatki [`../decisions/fog-distance-from-reconstructed-position.md`](../decisions/fog-distance-from-reconstructed-position.md), [`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md) i [`../decisions/fog-no-special-case-for-sky.md`](../decisions/fog-no-special-case-for-sky.md) |
| `src/game/Vignette.*` (M7, część trzecia) | winieta jako dane i matematyka, bez OpenGL: stałe `SCREEN_CENTER` (`(0,5, 0,5)`) i `VIGNETTE_CORNER_DISTANCE` (0,70710678, pierwiastek z 0,5: odległość od środka ekranu do narożnika we współrzędnych tekstury), struktura `game::VignetteSettings` (`enabled` prawda, `strength` 0,3, `radius` 0,4) i funkcja `vignetteFactor` (`1 - strength * smoothstep(radius, VIGNETTE_CORNER_DISTANCE, distance)`: 1 wewnątrz promienia, `1 - strength` w narożnikach, bez korekty proporcji okna). Te same stałe i ten sam wzór ma `post/composite.frag`. Część biblioteki `game_logic`, więc ma testy | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.22 i 5.13, notatka [`../decisions/vignette-not-aspect-corrected.md`](../decisions/vignette-not-aspect-corrected.md) |
| `src/game/Shadows.*` (M7, część czwarta) | cienie jako dane i matematyka, bez OpenGL: wyliczenie `game::ShadowResolution` (`Low`, `High`: pozycje listy w panelu), stałe `SHADOW_MAP_SIZE_LOW` (1024) i `SHADOW_MAP_SIZE_HIGH` (2048), `MIN_PCF_RADIUS` (1), `MAX_PCF_RADIUS` (3, ta sama stała jest w `common/shadows.glsl`) i `DEFAULT_PCF_RADIUS` (1), struktura `game::ShadowSettings` (`enabled` prawda, `resolution` `High`, `constantBias` 0,02 m, `slopeBias` 0,12 m, `hardwareFilter` prawda, `pcf` prawda, `pcfRadius` 1, `strength` 1,0 i `preview`, które w każdej klatce ustawia interfejs debugowy), funkcje `shadowMapSize`, `pcfKernelSide` (`2 * radius + 1`), `shadowCasterBounds` (pudełko całego terenu, od najniższego gruntu do `PILLAR_HEIGHT` nad najwyższym: do niego księżyc dopasowuje swoją mapę), `shadowBias` (`constantBias + slopeBias * (1 - facing)`, ten sam wzór co `slopeScaledBias` w GLSL), `biasInDepthUnits` (bias w metrach podzielony przez głębokość pudełka światła), `shadowTexelSize` (większy bok pokrytego obszaru podzielony przez liczbę tekseli) i `pcfRadiusInUse` (promień w granicach albo 0 przy wyłączonym PCF). Część biblioteki `game_logic`, więc ma testy. Teksturę głębi trzyma `ShadowMap` Od piątej części M7: `flashlightShadowDefaults` (rozdzielczość 1024, bias 0,01 m i 0,13 m), stałe `FLASHLIGHT_SHADOW_CONSTANT_BIAS` i `FLASHLIGHT_SHADOW_SLOPE_BIAS`, `biasForShader` (głębia dla pudełka, metry dla ostrosłupa) i `shadowTexelSizeAt` (teksel rośnie z odległością). | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.4, 2.9 do 2.12 i 5, notatka [`../decisions/shadow-bias-in-metres-in-shader.md`](../decisions/shadow-bias-in-metres-in-shader.md) |
| `src/game/ShadowMap.*` (M7, część czwarta) | klasa `game::ShadowMap`: strona OpenGL jednej mapy cieni (księżyc ma jedną). Posiada kwadratowy framebuffer bez tekstury koloru (`gfx::ColorFormat::None`, głębia `GL_DEPTH_COMPONENT24`), sampler z porównaniem (`gfx::ComparisonSampler`), mały framebuffer podglądu (`GL_RGBA8`, 256 x 256, bez głębi) i pusty obiekt tablicy wierzchołków dla trójkąta podglądu. `beginDepthPass(size)` tworzy framebuffer od nowa, gdy rozmiar jest inny niż poprzednio, wiąże go (viewport na rozmiar mapy), włącza test głębi i czyści głębię, a zwraca fałsz, gdy framebuffera nie udało się utworzyć. `bindForSampling(unit, linearFilter)` wiąże z jednostką teksturującą teksturę głębi, a po niej sampler z porównaniem, i zostawia aktywną jednostkę 0. `drawPreview` rysuje programem `preview` w trybie `RawDepth` zapisane głębie jako odcienie szarości i czyta przy tym teksturę bez obiektu samplera. Akcesory `target` i `preview`. Wolna funkcja `game::setShadowUniforms` ustawia w programie siedem uniformów jednej mapy (nazwy z `ShadowUniformNames`): jednostkę samplera, przełącznik, macierz światła, dwie części biasu przeliczone na różnicę zapisanych głębi, promień PCF i siłę przyciętą do zakresu od 0 do 1. Wymaga kontekstu OpenGL, więc jest w programie, a nie w `game_logic`, i nie ma testu jednostkowego. PRD przewidywał w tym miejscu klasę `ShadowPass` w `src/renderer/` Od piątej części M7 gra ma dwa obiekty tej klasy (księżyc i latarka), `drawPreview(previewShader, lightSpace)` wybiera tryb obrazu po rodzaju rzutu, a `setShadowUniforms` ustawia dla mapy z `lightPosition` ósmy uniform, pozycję światła. | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.17, 3 i 5, sampler w [`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md) |
| `src/game/Discovery.*` (M7, część szósta) | minimapa, stan i reguła, bez OpenGL: klasa `game::Discovery` (jeden bajt na komórkę, `contains`, `isDiscovered`, `discover`, `count`), funkcje `discoverFrom` (komórka i linia prosta w czterech kierunkach aż do ściany, ściany czytane z `Maze` przy każdym wywołaniu) i `discoverAround` (z pozycji w świecie przez `cellAt`). Biblioteka `game_logic`. [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md) |
| `src/game/Minimap.*` (M7, część szósta) | minimapa jako dane i matematyka, bez OpenGL: `MinimapSettings` (`enabled`, `revealAll`, `size` 0,28, `margin` 0,02, `corner` prawy dolny, `opacity` 0,85) i granice suwaków, `minimapRect` (kwadrat w pikselach framebuffera), `MinimapVertex` (5 `float`, 20 bajtów), dziewięć stałych kolorów sRGB, `minimapHalfExtent`, `minimapProjection` (rzut ortograficzny, północ u góry), `minimapMetresPerPixel`, `buildMinimapVertices` (podłogi, ściany, brama, kryształy, strzałka gracza). Biblioteka `game_logic`. [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md) |
| `src/game/MinimapRenderer.*` (M7, część szósta) | klasa `game::MinimapRenderer`: strona OpenGL minimapy. Posiada framebuffer `GL_RGBA8` bez głębi (tworzony przy nowym rozmiarze kwadratu), bufor `GL_DYNAMIC_DRAW` z listą trójkątów (wysyłany `Buffer::setData` co klatkę), tablicę wierzchołków i pustą tablicę dla trójkąta nakładki. `drawMap` (rysowanie schematu do własnego framebuffera) i `drawOverlay` (kopia w róg okna z mieszaniem). Program `night_maze` |
| `src/game/ShaderUniforms.hpp` | nazwy uniformów jako stałe, czterdzieści siedem po trzeciej części M7 (szósta dodała trzy: `MINIMAP_MAP_TO_CLIP_UNIFORM`, `MINIMAP_OVERLAY_MAP_UNIFORM`, `MINIMAP_OVERLAY_OPACITY_UNIFORM`): `MODEL_UNIFORM`, `VIEW_UNIFORM`, `PROJECTION_UNIFORM`, `TEXTURE_UNIFORM`, `TINT_UNIFORM`, `EMISSIVE_UNIFORM` (od M5), `NORMAL_MAP_UNIFORM`, `NORMAL_MAP_ENABLED_UNIFORM`, `VIEW_MODE_UNIFORM`, `NORMAL_MATRIX_UNIFORM`, `SPECULAR_MODEL_UNIFORM`, `SPECULAR_STRENGTH_UNIFORM`, `SHININESS_UNIFORM`, `COLOR_UNIFORM`, a od M6 `SKYBOX_UNIFORM` i `SKYBOX_BRIGHTNESS_UNIFORM` (niebo) oraz `GRASS_TIME_UNIFORM`, `GRASS_BLADE_HEIGHT_UNIFORM`, `GRASS_WIND_STRENGTH_UNIFORM` i `GRASS_LIT_UNIFORM` (trawa: `uTime`, `uBladeHeight`, `uWindStrength`, `uLit`), a od pierwszej części M7 `COMPOSITE_SCENE_UNIFORM`, `COMPOSITE_EXPOSURE_UNIFORM` i `COMPOSITE_TONE_MAPPING_UNIFORM` (przebieg składający: `uScene`, `uExposure`, `uToneMapping`) oraz `PREVIEW_SOURCE_UNIFORM`, `PREVIEW_MODE_UNIFORM`, `PREVIEW_NEAR_UNIFORM`, `PREVIEW_FAR_UNIFORM` i `PREVIEW_DEPTH_RANGE_UNIFORM` (podglądy: `uSource`, `uMode`, `uNear`, `uFar`, `uDepthRange`), a od drugiej części M7 osiem stałych bloomu: `COMPOSITE_BLOOM_UNIFORM`, `COMPOSITE_BLOOM_ENABLED_UNIFORM` i `COMPOSITE_BLOOM_INTENSITY_UNIFORM` (`uBloom`, `uBloomEnabled`, `uBloomIntensity`), `BRIGHT_SCENE_UNIFORM` i `BRIGHT_THRESHOLD_UNIFORM` (`uScene`, `uThreshold`) oraz `BLUR_SOURCE_UNIFORM`, `BLUR_HORIZONTAL_UNIFORM` i `BLUR_WEIGHTS_UNIFORM` (`uSource`, `uHorizontal`, `uWeights`), a od trzeciej części M7 jedenaście stałych mgły i winiety w przebiegu składającym: `COMPOSITE_FOG_ENABLED_UNIFORM`, `COMPOSITE_DEPTH_UNIFORM`, `COMPOSITE_FOG_DENSITY_UNIFORM`, `COMPOSITE_FOG_BASE_HEIGHT_UNIFORM`, `COMPOSITE_FOG_HEIGHT_FALLOFF_UNIFORM` i `COMPOSITE_FOG_COLOR_UNIFORM` (`uFogEnabled`, `uDepth`, `uFogDensity`, `uFogBaseHeight`, `uFogHeightFalloff`, `uFogColor`), `COMPOSITE_INVERSE_VIEW_PROJECTION_UNIFORM` i `COMPOSITE_EYE_UNIFORM` (`uInverseViewProjection`, `uEye`) oraz `COMPOSITE_VIGNETTE_ENABLED_UNIFORM`, `COMPOSITE_VIGNETTE_STRENGTH_UNIFORM` i `COMPOSITE_VIGNETTE_RADIUS_UNIFORM` (`uVignetteEnabled`, `uVignetteStrength`, `uVignetteRadius`). Czwarta część M7 nie dodała kolejnych stałych tego rodzaju (jest ich nadal czterdzieści siedem), tylko strukturę `ShadowUniformNames` z siedmioma nazwami uniformów jednej mapy cieni (`map`, `enabled`, `matrix`, `constantBias`, `slopeBias`, `pcfRadius`, `strength`), jedną stałą tego typu, `MOON_SHADOW_UNIFORMS` (`uMoonShadowMap`, `uMoonShadowEnabled`, `uMoonShadowMatrix`, `uMoonShadowConstantBias`, `uMoonShadowSlopeBias`, `uMoonShadowPcfRadius`, `uMoonShadowStrength`), i jednostkę teksturującą mapy księżyca, `MOON_SHADOW_TEXTURE_UNIT` (3: modele używają jednostek 0 i 1, a przebieg składający jednostek od 0 do 2). Do tego nazwa bloku uniformów `LIGHT_BLOCK_NAME` (`"LightBlock"`) i jego punkt wiązania `LIGHT_BLOCK_BINDING_POINT` (1, typu `GLuint`, stąd `<glad/gl.h>` w nagłówku). Sam nagłówek, wspólny dla `NightMazeApp`, `MazeRenderer`, `GameplayRenderer`, `ModelDraw`, `ColliderLines`, `LightRig`, `Skybox`, `TerrainRenderer`, `GrassRenderer`, `PostProcess` i `ShadowMap` Od piątej części M7: struktura `ShadowUniformNames` ma ósme pole `lightPosition` (dla księżyca `nullptr`), jest stała `FLASHLIGHT_SHADOW_UNIFORMS` i jednostka `FLASHLIGHT_SHADOW_TEXTURE_UNIT` (4). | [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), blok w [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) |
| `src/game/NightMazeApp.*` | `game::NightMazeApp` (od szóstej części M7 także trzynaście programów shaderów, `m_minimapShader`, `m_minimapOverlayShader`, `m_minimapSettings`, `m_minimapRenderer`, `drawMinimap` i klawisz M): kolor czyszczenia (`{0.022F, 0.033F, 0.088F}`, od M7 wartość sRGB przeliczana na liniową, od M6 tło tylko wtedy, gdy niebo jest wyłączone), jedenaście programów shaderów (`textured`, `color`, `lit`, `gouraud`, `skybox`, `grass`, od M7 `composite`, `preview`, `bright`, `blur` i `shadow_depth`), pamięć podręczna assetów, `MazeRenderer`, `GameplayRenderer`, od M6 `TerrainRenderer` i `GrassRenderer`, `ColliderLines`, `LightRig`, `Skybox` z ustawieniami `SkyboxSettings`, od M7 `PostProcess` z ustawieniami `PostProcessSettings` i `ShadowMap` z ustawieniami `ShadowSettings`, `MazeSettings`, od M6 mapa wysokości `Heightmap` z ustawieniami `TerrainSettings`, `MazeWorld` i ustawienia trawy `GrassSettings`, liczby reguł `GameplaySettings` i stan rundy `Round`, lista przeszkód rundy `m_obstacles`, gracz z pozycją sprzed ostatniego kroku, `scene::Camera`, ustawienia oświetlenia `LightingSettings`, tryb widoku, przełącznik rysowania kształtów kolizji i czułość myszy. Konstruktor wczytuje shadery i modele, łączy oba oświetlone programy i program trawy z buforem świateł (`m_lightRig.connect`), buduje pierwszy labirynt (10 na 10, ziarno 1) i woła `beginRound`, która zaczyna rundę (`startRound`), buduje listę przeszkód (`roundObstacles`), włącza latarkę i stawia gracza na starcie. `onUpdate` w każdym stałym kroku zbiera klawisze do `PlayerInput` (tylko przy przechwyconym kursorze), woła `Player::update` z listą przeszkód rundy, ustawia kamerę w oczach gracza i woła `updateRound` (bateria, zbieranie kryształów, brama, wyjście). Gdy brama właśnie się otworzyła, buduje listę przeszkód od nowa. `onRender` buduje nowy labirynt, gdy panel o to poprosił (`regenerateMaze`), zaczyna rundę od nowa po klawiszu R albo po prośbie panelu (`beginRound`), obsługuje klawisze N (noclip) i F (latarka), przechwytuje kursor po kliknięciu w scenę i obraca kamerę myszą, pomija klatkę przy framebufferze 0 x 0, od czwartej części M7 rysuje jako pierwszy przebieg klatki mapę cieni księżyca (`drawMoonShadowMap`), wiąże framebuffer HDR sceny (`m_postProcess.beginScene`, które ustawia też viewport), włącza test głębi, czyści kolor i głębię tego framebuffera, liczy proporcje z rozmiaru framebuffera i pozycję oka między dwoma krokami symulacji, buduje światła klatki (`lightingForFrame`, `crystalLightPositions`, `buildLightSet`) i wysyła je do bufora uniformów (`m_lightRig.upload`, w każdej klatce, także w trybie `Unlit`), a potem rysuje części klatki: `drawMaze` (bez światła w `drawUnlitMaze` programem `textured`, gdy tryb to `Unlit` albo wybrany jest widok debug, w pozostałych przypadkach `drawLitMaze` programem `gouraud` albo `lit`; obie funkcje rysują teren przez `TerrainRenderer`, labirynt przez `MazeRenderer`, a kryształy i bramę przez `GameplayRenderer`, tym samym programem), potem `drawGrass` (trawa programem `grass`, gdy pole `Enabled` jest zaznaczone) i, gdy włączone, `drawColliderLines` (pudełka i kule programem `color`), potem, gdy pole `Skybox` jest zaznaczone, `m_skybox.draw` (niebo programem `skybox`), a od pierwszej części M7 po scenie jeszcze `m_postProcess.drawPreviews` (tylko przy otwartym panelu Framebuffers, programem `preview`), od drugiej części M7 `m_postProcess.drawBloom` (przebieg jasności programem `bright` i rozmycie programem `blur`, w celach o połowie rozmiaru sceny, wołane w każdej klatce z kopią ustawień, w której widok debug wyłącza bloom, a od trzeciej części M7 także mgłę i winietę) i na samym końcu `m_postProcess.composite` (obraz sceny z mgłą, dodaną poświatą i winietą do okna programem `composite`; od trzeciej części M7 dostaje strukturę `SceneView`, zbudowaną tuż przed nim z `glm::inverse(projection * view)` i pozycji oka). W jednej klatce pracuje najwyżej dziewięć z jedenastu programów (policzone z kodu: `shadow_depth`, jeden program sceny, `grass`, `color`, `skybox`, `preview`, `bright`, `blur` i `composite`). Chronione akcesory `clearColor()`, `texturedShader()`, `colorShader()`, `litShader()`, `gouraudShader()`, `skyboxShader()`, `grassShader()`, `compositeShader()`, `previewShader()`, od drugiej części M7 `brightPassShader()` i `blurShader()`, od czwartej `shadowDepthShader()`, `moonShadowSettings()`, `moonShadowMap()` i `moonLightSpace()`, `postProcessSettings()`, `postProcess()`, `skyboxSettings()`, `terrainSettings()`, `grassSettings()`, `grassTuftCount()`, `lighting()`, `camera()`, `mouseSensitivity()`, `player()`, `mazeSettings()`, `mazeWorld()`, `gameplaySettings()`, `round()`, `assets()`, `viewMode()` i `drawColliders()` udostępniają stan panelom debug i HUD. Kostki z M1 (danych wierzchołków, własnych buforów, funkcji rysującej i akcesora jej programu) ani funkcji rysującej znaczniki świateł już nie ma. Druga część M6: szósty program `m_grassShader` (trzy pliki), pola `m_terrainRenderer`, `m_grassRenderer`, `m_heightmap`, `m_terrainSettings`, `m_grassSettings` i `m_playerWasFlying`, funkcje `rebuildTerrain`, `uploadGround`, `plantGrass` i `drawGrass`, cztery akcesory (`grassShader`, `terrainSettings`, `grassSettings`, `grassTuftCount`). Konstruktor wczytuje mapę wysokości (`loadHeightmap`) i buduje labirynt od razu na terenie, `onRender` obsługuje flagi `rebuild` i `replant`, obie funkcje rysujące labirynt zaczynają od terenu, a po nich idzie trawa. Pierwsza część M7: programy `m_compositeShader` i `m_previewShader`, pola `m_postProcess` i `m_postProcessSettings`, funkcja `crystalEmissive` (blask kryształów z koloru przeliczonego na liniowy), cztery akcesory. Czwarta część M7: program `m_shadowDepthShader`, pola `m_moonShadowMap`, `m_moonShadow`, `m_moonLightSpace` i `m_moonShadowDrawn`, funkcje `drawMoonShadowMap` (liczy pudełko światła z terenu i z kątów księżyca, nigdy z kamery, rysuje przebieg głębi, wiąże mapę z jednostką teksturującą 3 i przy otwartym panelu Shadows rysuje jej podgląd) i `drawShadowCasters` (teren, zawsze wypełniony, labirynt, brama i kryształy programem `shadow_depth`; trawy nie rysuje), wywołania `setShadowUniforms` w `drawLitMaze` i `drawGrass`, cztery akcesory. Opis w [`../modules/core/README.md`](../modules/core/README.md), sekcja 6 Od piątej części M7 (2026-10-06): na początku `onRender` liczone są oko, `lightingForFrame` i `flashlightPose`, potem dwa przebiegi cieni (`drawMoonShadowMap`, `drawFlashlightShadowMap`), pola `m_flashlightShadowMap`, `m_flashlightShadow`, `m_flashlightLightSpace` i `m_flashlightShadowDrawn`, funkcja `setShadowUniformsOf` i cztery akcesory dla panelu. | [`../modules/core/README.md`](../modules/core/README.md), macierze w [`../modules/scene/camera.md`](../modules/scene/camera.md), obrót kamery myszą w [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), ruch gracza w [`../modules/game/player.md`](../modules/game/player.md), rysowanie labiryntu i regeneracja w [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), runda w [`../modules/game/gameplay.md`](../modules/game/gameplay.md), latarka i światła klatki w [`../modules/game/flashlight.md`](../modules/game/flashlight.md), przełącznik trybu cieniowania w [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md), przebieg cieni w [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md) |
| `assets/shaders/textured.vert`, `textured.frag` | para shaderów modeli z teksturą: atrybuty `aPosition` (0), `aNormal` (1), `aUv` (2), `aTangent` (3), czyli pola `gfx::Vertex`, trzy macierze, sampler `uTexture`, kolor `uTint`, blask własny `uEmissive` (od M5: w widoku 0 mnoży kolor przez `vec3(1.0) + uEmissive`) i tryb `uViewMode` (0: tekstura razy `uTint`, 1: normalna jako kolor, 2: współrzędne UV jako kolor). Od pierwszej części M7 wynik trafia do bufora HDR jako kolor liniowy, a oba widoki debug przechodzą przez `srgbToLinear` z `common/color.glsl`, żeby po kodowaniu na końcu klatki ekran pokazał te same liczby. `textured.frag` dołącza `common/normal_map.glsl` i `common/color.glsl`, więc widok normalnych pokazuje normalną z mapy normalnych, gdy mapy są włączone. Bez oświetlenia: rysuje scenę w trybie `Unlit` i oba widoki debug w każdym trybie. Normalną liczy nadal przez `mat3(uModel)` i nie ma uniformu `uNormalMatrix`. Od M5 w `textured.vert` jest też długi komentarz o łańcuchu przestrzeni (lokalna, świata, widoku, przycięcia), który wcześniej stał w `basic.vert`. To nie są pliki C++: nie są na żadnej liście w `CMakeLists.txt`, program czyta je przy starcie i po naciśnięciu "Reload shaders". To samo dotyczy pozostałych plików shaderów poniżej | [`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 4, potok i GLSL w [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md) |
| `assets/shaders/color.vert`, `color.frag` | para shaderów jednego koloru dla linii pudełek i kul kolizji: atrybut pozycji, trzy macierze i uniform `uColor` (od M7 kolor liniowy). Najprostsza para w projekcie. Do M4 rysowała też kostki oznaczające światła | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 4, [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md) |
| `assets/shaders/lit.vert`, `lit.frag` | para shaderów oświetlonego labiryntu ze światłem liczonym dla każdego fragmentu (tryby `Phong` i `Blinn-Phong`). `lit.vert` przekazuje pozycję, normalną (uniform `uNormalMatrix`) i styczną (`mat3(uModel)`) w przestrzeni świata oraz współrzędne tekstury, `lit.frag` dołącza `common/lighting.glsl` i `common/normal_map.glsl`, bierze normalną fragmentu z `surfaceNormal`, woła `computeLighting` i składa kolor: tekstura razy `uTint` razy suma światła rozproszonego i blasku własnego `uEmissive` (od M5, dla kryształów), plus połysk. Wzór połysku wybiera uniform `uSpecularModel`. Od czwartej części M7 `lit.frag` dołącza też `common/shadows.glsl` i przed złożeniem koloru odejmuje od światła rozproszonego i od połysku udział księżyca razy `moonShadow`. Bias cienia liczy z normalnej modelu (`normalize(vNormal)`), nie z normalnej z mapy normalnych | [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `assets/shaders/gouraud.vert`, `gouraud.frag` | para shaderów oświetlonego labiryntu ze światłem liczonym w wierzchołkach (tryb `Gouraud`). `gouraud.vert` dołącza `common/lighting.glsl` i woła tę samą funkcję `computeLighting` dla wierzchołka, `gouraud.frag` mnoży sumę rozciągniętego po trójkącie światła i blasku własnego `uEmissive` (od M5) przez teksturę i dodaje połysk. Bez map normalnych: stycznej nie czyta, a komentarz w `gouraud.vert` mówi dlaczego. Od czwartej części M7 `gouraud.vert` ma cztery nowe wyjścia (`vMoonDiffuseLight`, `vMoonSpecularLight`, `vWorldPosition`, `vMoonFacing`), a `gouraud.frag` dołącza `common/shadows.glsl` i robi test cienia dla każdego fragmentu: światło jest nadal liczone w wierzchołkach, cień już nie | [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `assets/shaders/skybox.vert`, `skybox.frag` | para shaderów nieba (M6). `skybox.vert` czyta tylko atrybut pozycji, usuwa przesunięcie z macierzy widoku (`mat4(mat3(uView))`), przekazuje pozycję narożnika jako kierunek `vDirection` i ustawia głębię na 1,0 (`gl_Position = position.xyww`). `skybox.frag` czyta teksturę sześcienną `uSkybox` (`samplerCube`) kierunkiem i mnoży kolor (od M7 liniowy: tekstura sześcienna jest sRGB) przez `uBrightness`, a w widokach debug (`uViewMode` różne od 0) pokazuje kierunek jako kolor, od M7 przeliczony przez `srgbToLinear` z `common/color.glsl` | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcja 4 |
| `assets/shaders/grass.vert`, `grass.geom`, `grass.frag` | trójka shaderów trawy (M6), jedyny program z shaderem geometrii. `grass.vert` przepuszcza punkt kępki w przestrzeni świata i jej liczbę losową. `grass.geom` zamienia każdy punkt (`layout(points) in`) na trzy źdźbła, każde jako pasek trójkątów z pięciu wierzchołków (`layout(triangle_strip, max_vertices = 15) out`), z pochyleniem i wiatrem zależnym od `uTime`, i tu mnoży przez `uView` i `uProjection`. `grass.frag` miesza kolor od korzenia do czubka (od M7 miesza liczby sRGB i wynik przelicza raz na liniowy, `common/color.glsl`) i oświetla go składnikiem rozproszonym z `common/lighting.glsl`, z normalną skierowaną w górę. Przy `uLit` równym 0 pokazuje pełną jasność. Od czwartej części M7 dołącza `common/shadows.glsl` i odejmuje od światła udział księżyca tam, gdzie źdźbło stoi w cieniu. Sama trawa cienia nie rzuca | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md), etap geometrii w klasie w [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md) |
| `assets/shaders/common/lighting.glsl` | plik dołączany, nie samodzielny shader (nie ma linii `#version`): stała `MAX_POINT_LIGHTS`, struktura `PointLight`, blok uniformów `layout(std140) uniform LightBlock`, uniformy materiału `uSpecularModel`, `uSpecularStrength`, `uShininess`, struktura `Lighting` (od czwartej części M7 z polami `moonDiffuse` i `moonSpecular`: to udział księżyca, zawarty już w `diffuse` i `specular`) i funkcje `diffuseFactor`, `specularFactor`, `attenuationFactor`, `addLight`, `computeLighting` i, od czwartej części M7, `moonFacing` (cosinus kąta między normalną a kierunkiem do księżyca). O cieniach `computeLighting` nic nie wie: udział księżyca odejmuje shader, który ją woła. Treść wstawia w miejsce linii `#include "common/lighting.glsl"` kod `gfx::expandIncludes` | [`../modules/scene/lights.md`](../modules/scene/lights.md), mechanizm dołączania w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `assets/shaders/common/normal_map.glsl` | plik dołączany, nie samodzielny shader (nie ma linii `#version`): sampler `uNormalMap` (jednostka 1), przełącznik `uNormalMapEnabled` i funkcja `surfaceNormal`, która z normalnej i stycznej modelu oraz teksela mapy normalnych składa normalną fragmentu w przestrzeni świata (macierz TBN). Dołączają go `lit.frag` i `textured.frag` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 4.1 |
| `assets/shaders/common/color.glsl` (M7) | plik dołączany, bez linii `#version`: pięć stałych standardu sRGB i funkcje `srgbToLinear` oraz `linearToSrgb` dla `vec3`, te same wzory co `gfx::srgbToLinear` i `gfx::linearToSrgb` w C++. Od drugiej części M7 także stała `REC709_LUMINANCE_WEIGHTS` i funkcja `luminance`: jasność liniowego koloru jedną liczbą, dla progu bloomu. Dołączają go `textured.frag`, `skybox.frag` i `grass.frag` (ścieżką `common/color.glsl`) oraz `post/composite.frag`, `post/preview.frag` i `post/bright.frag` (ścieżką `../common/color.glsl`, liczoną od ich katalogu) | [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md), [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) |
| `assets/shaders/common/depth.glsl` (M7) | plik dołączany, bez linii `#version`: funkcja `linearDepth(stored, near, far)`, która z liczby zapisanej w teksturze głębi odtwarza odległość w metrach. Dołącza go tylko `post/preview.frag` (podgląd głębi). Mgła z trzeciej części M7 go nie używa: nie liczy głębi wzdłuż osi widoku, tylko odległość od oka do miejsca w świecie, które odtwarza `worldPositionFromDepth` w `post/composite.frag` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) |
| `assets/shaders/post/composite.vert`, `composite.frag` (M7) | para shaderów przebiegu składającego, ostatniego przebiegu klatki. `composite.vert` nie ma wejść: z `gl_VertexID` liczy trzy narożniki jednego trójkąta, który przykrywa cały ekran, i współrzędną `vUv`. `composite.frag` robi w `main` siedem kroków w stałej kolejności: czyta teksturę koloru sceny (`uScene`), od trzeciej części M7 miesza z nią mgłę (gdy `uFogEnabled` to 1: głębia z `uDepth`, miejsce w świecie z `worldPositionFromDepth` i `uInverseViewProjection`, odległość od `uEye`, funkcje `fogHeightFactor` i `fogAmount`, `mix` z `uFogColor`), od drugiej części M7 dodaje rozmytą poświatę (`uBloom` razy `uBloomIntensity`, gdy `uBloomEnabled` to 1), mnoży przez `uExposure`, stosuje krzywą wybraną przez `uToneMapping` (0: przycięcie, 1: Reinhard, 2: ACES), od trzeciej części M7 mnoży przez winietę (`vignetteFactor`, gdy `uVignetteEnabled` to 1) i koduje wynik do sRGB. Ma trzy samplery (`uScene`, `uBloom`, `uDepth`) i stałe `SCREEN_CENTER` i `VIGNETTE_CORNER_DISTANCE`, te same co w `src/game/Vignette.hpp` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) |
| `assets/shaders/post/preview.frag` (M7) | shader fragmentów podglądów załączników, używany z `post/composite.vert` jako ósmy program. Tryb `uMode` 0: załącznik koloru zakodowany do sRGB, bez ekspozycji i krzywej. Tryb 1: głębia przeliczona na metry (`linearDepth`) i pokazana od czerni do bieli w zakresie `uDepthRange`. Trybem 0 rysowane są od drugiej części M7 także podglądy dwóch celów bloomu. Od czwartej części M7 jest tryb 2: głębia pokazana tak, jak jest zapisana, bez przeliczania na metry. Rysuje nim podgląd mapy cieni `game::ShadowMap::drawPreview` (rzut ortograficzny zapisuje głębię równomiernie, więc przeliczenie nie jest potrzebne) | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), tryb 2 w [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.17 |
| `assets/shaders/post/bright.frag` (M7, część druga) | shader fragmentów przebiegu jasności, pierwszego kroku bloomu, używany z `post/composite.vert` jako dziewiąty program. Liczy jasność piksela sceny (`luminance`) i mnoży jego kolor przez udział jasności ponad progiem `uThreshold`: pod progiem czerń, nad progiem kolor o tej samej barwie | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 4.6 |
| `assets/shaders/post/minimap.vert`, `minimap.frag` (M7, część szósta) | para shaderów rysowania mapy do jej framebuffera: pozycja w metrach labiryntu przez `uMapToClip`, kolor sRGB zapisany bez zmian. Dwunasty program |
| `assets/shaders/post/minimap_overlay.frag` (M7, część szósta) | shader fragmentów nakładki: tekstura mapy bez zmian koloru i alfa równa `uOpacity`. Używany z `post/composite.vert` jako trzynasty program |
| `assets/shaders/post/blur.frag` (M7, część druga) | shader fragmentów rozmycia bloomu, używany z `post/composite.vert` jako dziesiąty program. Jeden kierunek rozdzielnego rozmycia Gaussa (`uHorizontal` 1 albo 0): 13 odczytów tekstury z wagami z tablicy `uWeights`. Stała `BLUR_RADIUS` (6) musi być równa `game::BLOOM_BLUR_RADIUS` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 4.7 |
| `assets/shaders/shadow_depth.vert`, `shadow_depth.frag` (M7, część czwarta) | para shaderów przebiegu głębi mapy cieni, jedenasty program. `shadow_depth.vert` czyta tylko atrybut pozycji i mnoży go przez `uProjection * uView * uModel`: te same trzy nazwy co w `lit.vert`, żeby klasy rysujące labirynt działały z tym programem bez zmian, ale `uView` i `uProjection` to tutaj widok i rzut światła (`scene::LightSpace`). `shadow_depth.frag` ma pustą funkcję `main`: framebuffer mapy cieni nie ma tekstury koloru, a głębię fragmentu zapisuje karta | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 4 |
| `assets/shaders/common/shadows.glsl` (M7, część czwarta) | plik dołączany, bez linii `#version`: stała `MAX_PCF_RADIUS` (3), siedem uniformów mapy cieni księżyca (`uMoonShadowMap` typu `sampler2DShadow`, `uMoonShadowMatrix`, `uMoonShadowEnabled`, `uMoonShadowConstantBias`, `uMoonShadowSlopeBias`, `uMoonShadowPcfRadius`, `uMoonShadowStrength`) i funkcje `slopeScaledBias`, `shadowMapVisibility` (jedno porównanie albo jądro PCF; głębia powyżej 1 od razu daje "oświetlony") i `moonShadow(worldPosition, facing)`, która zwraca udział światła księżyca zabierany przez cień, od 0 do `uMoonShadowStrength`. To zwykłe uniformy, a nie pola bloku `LightBlock`: sampler nie może być polem bloku. Dołączają go `lit.frag`, `gouraud.frag` i `grass.frag` | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 4, notatka [`../decisions/shadow-matrix-as-plain-uniforms.md`](../decisions/shadow-matrix-as-plain-uniforms.md) |
| `assets/models/*.obj`, `*.mtl`, `assets/textures/*.png` | pięć modeli (`wall_straight`, `wall_pillar`, a od M5 `crystal_a`, `crystal_b` i `gate`; szósty, płytkę podłogi, usunął M6), każdy z jednym materiałem, cztery tekstury koloru (`ground.png`, `wall_stone.png`, `crystal.png`, `gate_wood.png`; ścianę i słupek pokrywa ta sama, oba kryształy też, a `ground.png` nie należy do żadnego modelu: nakłada ją na teren `game::TerrainRenderer`) i cztery mapy normalnych o tych samych nazwach z końcówką `_normal`, które materiały nazywają linią `map_Bump` (mapę gruntu `ground_normal.png` nazywa kod, nie materiał). Dziewiąty plik w `assets/textures/` to `heightmap.png`: szara mapa wysokości terenu 256 x 256, którą gra czyta jako liczby i z której nie robi tekstury. Budują je skrypty z `tools/blender/` | [`blender.md`](blender.md), [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), [`../modules/assets/images.md`](../modules/assets/images.md) |
| `assets/skybox/*.png` | sześć ścian nieba (M6): `px`, `nx`, `py`, `ny`, `pz`, `nz`, każda 1024 x 1024, RGB, razem 5 278 627 bajtów. Nie należą do żadnego modelu ani materiału. Wczytuje je `game::Skybox`, bez odwracania wierszy i bez pamięci podręcznej assetów. Buduje je `tools/blender/make_skybox.py`, a ich zawartość sprawdza `tests/SkyboxTests.cpp` | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), [`blender.md`](blender.md), sekcja 7.7 |
| `src/debug/DebugContext.hpp` | `debug::DebugContext`: struktura referencji do danych, które panele i HUD czytają albo edytują, 42 pola (stan po szóstej części M7: do 38 pól po piątej doszły cztery z minimapy, `minimapShader`, `minimapOverlayShader`, `minimapSettings` i `minimap`; lista kończyła się na czwartej części, od piątej doszły `flashlightShadowSettings`, `flashlightShadowMap`, `flashlightLightSpace` i `flashlightShadowDrawn`, drugie pole będące wartością) (`time`, `window`, `clearColor`, `camera`, `mouseSensitivity`, `texturedShader`, `colorShader`, `player`, `mazeSettings`, `mazeWorld`, `assets`, `viewMode`, `drawColliders`, `litShader`, `gouraudShader`, `lighting`, od M5 `gameplay` i `round`, a od M6 `skyboxShader`, `skybox`, `grassShader`, `terrain`, `grass` i `grassTuftCount`, jedyne pole będące liczbą, a nie referencją, a od pierwszej części M7 `compositeShader`, `previewShader`, `postProcessSettings` i `postProcess`, a od drugiej części M7 `brightPassShader` i `blurShader`; trzecia część M7 zmieniła tylko komentarz pola `postProcessSettings`; od czwartej części M7 `shadowDepthShader`, `moonShadowSettings` oraz dwa pola tylko do odczytu, `moonShadowMap` i `moonLightSpace`; pole `shader` zniknęło razem z programem `basic`). Sam nagłówek | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5 |
| `src/debug/DebugUI.*` | `debug::DebugUI`: inicjalizacja i zamknięcie ImGui, zastosowanie motywu i wczytanie czcionki, bajty czcionki, od M7 pole `m_rawTextureSampler`, `draw` (zeruje flagę podglądów `postProcessSettings.previews` i, od czwartej części M7, flagę podglądu mapy cieni `moonShadowSettings.preview`, rysuje dwanaście paneli, gdy są widoczne, a panelowi Shaders podaje tablicę trzynastu programów (od szóstej części M7), stała `SHADER_COUNT`; potem zawsze rysuje HUD, `drawHud`), `toggleVisible` (chowa i pokazuje panele, HUD zostaje), `wantsKeyboard`, `wantsMouse`, `setMouseEnabled` (ImGui ignoruje mysz, gdy kursor jest przechwycony). Od pierwszej części M7 `draw` woła jedenaście funkcji paneli, a od drugiej podaje panelowi Shaders listę dziesięciu programów (w pierwszej: ośmiu). Od czwartej części M7 funkcji paneli jest dwanaście (doszło `drawShadowsPanel`), a programów na liście jedenaście | [`../modules/debug-ui.md`](../modules/debug-ui.md), [`../libraries/imgui.md`](../libraries/imgui.md) |
| `src/debug/RawTextureSampler.*` (M7) | klasa `debug::RawTextureSampler`: obiekt samplera OpenGL z wyłączonym dekodowaniem sRGB (rozszerzenie `GL_EXT_texture_sRGB_decode`, stałe wpisane ręcznie, sprawdzane przez `gfx::hasExtension`). `begin` i `end` dopisują do listy rysowania ImGui wywołania zwrotne, które na czas jednego obrazu wpinają ten sampler na jednostkę 0 i potem przywracają sampler backendu. Dzięki temu podgląd tekstury sRGB w panelu Assets wygląda jak plik. Bez rozszerzenia obiekt nic nie robi. Posiada go `DebugUI` | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.11 |
| `src/debug/Hud.*` | `debug::drawHud`: HUD gry, rysowany przez ImGui w każdej klatce, także przy ukrytych panelach. Pasek u góry okna (napis `Crystals` z liczbą zebranych, potrzebnych i wszystkich kryształów, czas rundy, pasek baterii, który robi się czerwony poniżej progu, podpowiedzi `The gate is open. Find the exit.` i `Battery empty. Find a crystal.`) i karta `You escaped` po wygranej. Nie przyjmuje myszy ani klawiatury. Leży w `src/debug/`, bo tylko tam wolno dołączać ImGui. Od M6 pasek stoi pod rzędami zwiniętych pasków tytułów (w M6 pod dwoma, od pierwszej części M7 pod trzema, od czwartej pod czterema): jego odległość od góry liczy `debug::foldedRowsHeight`, a `HUD_TOP_OFFSET` to już tylko dwa odstępy `PANEL_GAP` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 6 |
| `src/debug/Theme.*` | motyw paneli: `debug::colorFromBytes`, stałe `ERROR_TEXT_COLOR`, `PLAN_WALL_COLOR` i `PLAN_PLAYER_COLOR`, a od M5 kolory kryształów, bramy i wyjścia na planie (`PLAN_CRYSTAL_COLOR`, `PLAN_COLLECTED_COLOR`, `PLAN_GATE_COLOR`, `PLAN_EXIT_COLOR`) i kolory HUD (`HUD_CRYSTAL_COLOR`, `HUD_BATTERY_COLOR`, `HUD_BATTERY_LOW_COLOR`), `debug::applyTheme` (tabela kolorów, metryki, skala ekranu) i `debug::loadFont` (czcionka z `assets/fonts`, z czcionką wbudowaną jako wyjściem awaryjnym) | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8 |
| `src/debug/PanelLayout.*` | układ paneli przy pierwszym uruchomieniu: struktura `debug::PanelPlacement` (róg okna, odsunięcie, rozmiar i pole `collapsed`: czy panel startuje zwinięty do paska tytułu), stałe wymiarów (między innymi `LEFT_COLUMN_WIDTH`, `BOTTOM_ROW_HEIGHT`), dwanaście stałych `..._PLACEMENT` (od M5 z `GAMEPLAY_PLACEMENT`, od M6 z `TERRAIN_PLACEMENT` i `GRASS_PLACEMENT`, od pierwszej części M7 z `FRAMEBUFFERS_PLACEMENT`, od czwartej z `SHADOWS_PLACEMENT`; te pięć i `CAMERA_PLACEMENT` mają `collapsed = true`, czyli sześć paneli startuje zwiniętych, Terrain i Grass mają także `foldedRowsBefore = 1`, czyli stoją w drugim rzędzie pasków, Framebuffers `foldedRowsBefore = 2`, trzeci rząd, a Shadows `foldedRowsBefore = 3`, czwarty rząd, z rozmiarem `FRAMEBUFFERS_WIDTH` na `SHADOWS_HEIGHT`, czyli 324), stała `FOLDED_ROW_COUNT` (4, w pierwszych trzech częściach M7 3, do M6 2), funkcja `debug::foldedRowsHeight` (wysokość rzędów pasków z `ImGui::GetFrameHeight()`) i `debug::placePanelOnFirstUse`, która ustawia pozycję, rozmiar i stan zwinięcia z warunkiem `ImGuiCond_FirstUseEver` | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.7 |
| `assets/fonts/AtkinsonHyperlegible-Regular.ttf`, `OFL.txt`, `README.md` | czcionka paneli debug (Atkinson Hyperlegible 1.006, 54 348 bajtów, plik niezmieniony), jej licencja SIL Open Font License 1.1 i opis źródła z sumą kontrolną. Cudzy materiał: nie jest kodem i nie powstaje ze skryptów projektu | [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.8.5 |
| `src/debug/panels/CameraPanel.*` | `debug::drawCameraPanel`: panel "Camera", przy pierwszym uruchomieniu zwinięty do paska tytułu (linia trybu, stopy gracza, oko tylko do odczytu, yaw, pitch, FOV, bliska i daleka płaszczyzna, czułość myszy, prędkość chodu, sprintu i lotu) | [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md), sekcja 6 |
| `src/debug/panels/MazePanel.*` | `debug::drawMazePanel`: panel "Maze" (suwaki rozmiaru, ziarno, przyciski "Regenerate" i "Random seed", liczby ścian i słupków, od M5 linia z liczbą kryształów i komórką wyjścia, plan labiryntu z góry z graczem, kryształami, bramą i strefą wyjścia). Od M5 czyta też `game::Round` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 6 |
| `src/debug/panels/CollisionPanel.*` | `debug::drawCollisionPanel`: panel "Collision" (pola wyboru "Draw collision shapes" i "Noclip (key N)", legenda kolorów linii, liczby pudełek ścian, słupków i bramy, liczba kul kryształów, pudełko gracza). Od M5 czyta też `game::Round` | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 6 |
| `src/debug/panels/GameplayPanel.*` | `debug::drawGameplayPanel`: panel "Gameplay", przy pierwszym uruchomieniu zwinięty do paska tytułu (stan rundy, liczniki kryształów, stan bramy, przycisk "Restart round (key R)", suwak "Battery", pole "Battery drains", suwaki "Crystals needed", "Battery lifetime", "Recharge", "Flicker below" i "Pickup radius"). Edytuje `game::GameplaySettings`, a w `game::Round` tylko ładunek baterii | [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 6 |
| `src/debug/panels/TerrainPanel.*` | `debug::drawTerrainPanel` (M6): panel "Terrain" (suwak "Height scale" od 0 do 2,5, pole "Wireframe", linie z rozmiarem siatki, liczbą trójkątów i zakresem wysokości). Edytuje `game::TerrainSettings`, a zmiana skali ustawia flagę `rebuild`. `game::Terrain` tylko czyta. Startuje zwinięty w drugim rzędzie pasków | [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), kod panelu w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.10 |
| `src/debug/panels/GrassPanel.*` | `debug::drawGrassPanel` (M6): panel "Grass" (pole "Enabled", suwaki "Density", "Blade height" i "Wind strength", linia z liczbą kępek i źdźbeł). Edytuje `game::GrassSettings`, a zmiana gęstości ustawia flagę `replant`. Startuje zwinięty w drugim rzędzie pasków | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md), kod panelu w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.10 |
| `src/debug/panels/FramebuffersPanel.*` (M7) | `debug::drawFramebuffersPanel`: panel "Framebuffers" (od szóstej części M7 z trzecią zakładką `Minimap` i dwoma dodatkowymi argumentami). Od trzeciej części M7 ustawienia stoją w pasku zakładek (`drawSettings`), do szóstej części dwóch, każda z tabelą o dwóch kolumnach i czterech wierszach. Zakładka "Tone and bloom" (`drawToneAndBloomSettings`): suwak logarytmiczny "Exposure" od 0,1 do 8, lista "Tone mapping" z pozycjami `None (clamp)`, `Reinhard`, `ACES (fitted)`, od drugiej części M7 pole "Bloom" i suwaki "Blur iterations" od 1 do 10, "Threshold" od 0 do 4 i "Intensity" od 0 do 2, suwak "Depth range" od 2 do 100 m. Zakładka "Fog and vignette" (`drawFogAndVignetteSettings`): pole "Fog", suwaki "Density" od 0 do 0,5 na metr, "Base height" od -2 do 6 m i "Height falloff" od 0 do 3 na metr, edytor koloru "Fog colour", pole "Vignette", suwaki "Strength" od 0 do 1 i "Radius" od 0 do 0,65. Razem piętnaście kontrolek z podpowiedziami. Pod zakładkami bez zmian linie z rozmiarem i formatami framebuffera sceny i celów bloomu i cztery obrazy podglądu obok siebie: kolor, głębia, przebieg jasności i bloom. Ustawia `PostProcessSettings::previews` na prawdę tylko wtedy, gdy jest otwarty | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), skrót w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 6 |
| `src/debug/panels/AssetsPanel.*` | `debug::drawAssetsPanel`: panel "Assets" (lista "View mode", pole "Normal mapping", lista "Filter", suwak "Anisotropy", modele z częściami i ich mapami normalnych, tekstury z podglądem i od M7 z dopiskiem `sRGB` albo `linear`, lista nieudanych wczytań). Od M7 czwarty parametr, `const RawTextureSampler&`: podglądy tekstur sRGB są czytane bez dekodowania | [`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md), sekcja 6 |
| `src/debug/panels/RendererPanel.*` | `debug::drawRendererPanel`: panel "Renderer" (FPS i czas klatki, rozmiar framebuffera i okna, wersja OpenGL i karta, edytor "Clear color", lista "Lighting" z pozycjami `Unlit`, `Gouraud`, `Phong`, `Blinn-Phong`, która zapisuje wybór w `game::LightingMode`, oraz, od M6, pole wyboru "Skybox" i suwak "Sky brightness" (od M7 do 6), które piszą do `game::SkyboxSettings`) | [`../modules/debug-ui.md`](../modules/debug-ui.md), tryby w [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| `src/debug/panels/LightsPanel.*` | `debug::drawLightsPanel`: panel "Lights" (edytor koloru "Ambient" i cztery grupy: "Moon (directional)" z kątami, kolorem i natężeniem, "Flashlight (spot)" z polem "Flashlight on (key F)", kolorem, natężeniem, stożkiem "Cone" i zasięgiem "Beam range", "Point lights (crystals)" z liczbą świecących kryształów, kolorem, natężeniem i promieniem "Point radius", "Highlight (specular)" z suwakami "Strength" i "Shininess"). Pole latarki ma podpowiedź, gdy bateria jest pusta. Edytuje `game::LightingSettings`, a `game::Round` tylko czyta | [`../modules/scene/lights.md`](../modules/scene/lights.md), sekcja 6 |
| `src/debug/panels/ShadersPanel.*` | `debug::drawShadersPanel`: panel "Shaders" (jeden przycisk "Reload shaders" dla wszystkich programów, a dla każdego z jedenastu jedna linia: nazwy jego plików (dwóch, a dla programu trawy trzech, z plikiem shadera geometrii w środku) i `OK` albo, na czerwono, `FAILED` z komunikatem błędu pod spodem. Podpowiedź pokazuje pełne ścieżki) | [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6, komunikaty błędów w [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `src/debug/panels/ShadowsPanel.*` (M7, część czwarta) | `debug::drawShadowsPanel`: panel "Shadows" z jedną zakładką na każde światło, które rzuca cień, dziś dwiema, "Moon" i od piątej części M7 "Flashlight" (struktura `ShadowMapView` opisuje jedno światło). Pole "Shadows", lista "Resolution", suwaki "Constant bias" i "Slope bias" (w metrach), pole "Hardware 2 x 2 filter", pole "PCF" z listą "Kernel", suwak "Strength", linie `Map: ... x ..., ...`, `Covers ... x ... m, ... m deep` i `One texel: ... cm` oraz obraz mapy cieni. Edytuje `game::ShadowSettings`, a `game::ShadowMap` i `scene::LightSpace` tylko czyta. Ustawia `ShadowSettings::preview` na prawdę tylko dla zakładki, która jest pokazana. Startuje zwinięty w czwartym rzędzie pasków | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 6, skrót w [`../modules/debug-ui.md`](../modules/debug-ui.md) |
| `tests/main.cpp` | punkt wejścia programu testowego: makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` i dołączenie nagłówka doctest, który generuje `main()` | [`../libraries/doctest.md`](../libraries/doctest.md), sekcja 3.1 |
| `tests/ColliderTests.cpp` | 19 przypadków testowych: 12 dla `scene::Aabb`, `overlaps` pudełek i `moveAndSlide`, a od M5 siedem dla kul (`scene::Sphere`, oba `overlaps` z kulą, `closestPoint`, sam dotyk nie jest nakładaniem) | [`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 5 |
| `tests/ImageLoaderTests.cpp` | 10 przypadków testowych loadera obrazów (od M6 także wczytanie bez odwracania wierszy, `RowOrder::TopFirst`): dwie powtarzalne tekstury gry, `wall_stone.png` i od M6 `ground.png` w miejscu tekstury podłogi (512 x 512, 3 kanały), ich mapy normalnych (rozmiar, średnia blisko `(128, 128, 255)`, konwencja OpenGL kanału zielonego, niebieski zawsze powyżej 128), odwracanie wierszy na obrazku 2 x 3 zapisanym przez test, ścieżka ze znakami spoza ASCII, brak pliku, plik niebędący obrazem, pusty plik | [`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7 |
| `tests/MazeTests.cpp`, `MazeGeneratorTests.cpp`, `MazeLayoutTests.cpp` | 32 przypadki testowe labiryntu (8, 11 i 13, od szóstej części M7 w układzie o jeden więcej: `cellAt`): klasa `Maze`, od M5 także `isDeadEnd` i porównanie `MazeCell`, generator (w tym labirynt wzorcowy 4 na 4 z ziarna 1), układ w świecie i jego współpraca z kolizjami | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcja 5 |
| `tests/MazeWorldTests.cpp` | 8 przypadków testowych `game::buildMazeWorld`: wartości domyślne, `yawTowards`, liczba macierzy i pudełek, powtarzalność, macierze słupków i ścian w obu ustawieniach (test macierzy podłogi zniknął w M6 razem z płytkami), pozycja i kierunek startu | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md), sekcja 5 |
| `tests/PlayerTests.cpp` | 13 przypadków testowych gracza: stałe, pudełko i oczy, chodzenie wzdłuż yaw, klawisze boczne i przeciwne, ruch po skosie, sprint, zatrzymanie na ścianie, ślizganie wzdłuż ściany obok słupków, wędrówka po zamkniętym labiryncie, noclip, powrót stóp na grunt. Od M6 każdy test podaje graczowi teren: płaski, stałą `flatGround`, czyli domyślny `game::Terrain` | [`../modules/game/player.md`](../modules/game/player.md), sekcja 5 |
| `tests/ObjLoaderTests.cpp` | 19 przypadków testowych loadera OBJ (do pierwszej części M6 było 20, z testem płytki podłogi): reguły formatu na napisach wpisanych w kod (w tym linia mapy normalnych i styczne po `parseObj`), przypadki błędów z numerem linii i dwa prawdziwe modele z `assets/models/`, `wall_straight.obj` i `wall_pillar.obj` (mapa normalnych, styczne, brak lustrzanych trójkątów) | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), sekcja 5.9 |
| `tests/ShaderSourceTests.cpp` | 22 przypadki testowe `gfx::expandIncludes` i `gfx::nameSourceFiles`: shader bez `#include`, wstawienie pliku między dwie dyrektywy `#line`, pliki zagnieżdżone, końce linii Windows, `#include` w komentarzu, błędy (brak pliku, plik dołączający siebie, cykl, źle zapisana linia, `#include` przed `#version`, `#version` w pliku dołączanym) oraz zamiana numeru na nazwę pliku w formacie NVIDII i w formacie Apple. Pliki dołączane są w testach napisami w mapie, nie plikami na dysku | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `tests/LightTests.cpp` | 20 przypadków testowych świateł: `attenuationFactor` i `attenuationForRadius` (5 procent jasności na promieniu), `coneCosines` i `spotFactor`, `directionFromAngles`, pusty `LightSet` oraz `packLightBlock` (kamera, otoczenie, księżyc, reflektor, światła punktowe, kierunek o długości zero, rozmiar i przesunięcia bloku `std140`) | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `tests/TangentTests.cpp` | 9 przypadków testowych stycznych: `triangleTangents` (tekstura prosto, obrócona, powtórzona, zdegenerowane UV), `computeTangents` (długość 1, Gram-Schmidt, średnia na wspólnym wierzchołku, brak `NaN`, złe indeksy) i `countMirroredTriangles` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 5.10 |
| `tests/LightingTests.cpp` | 15 przypadków testowych oświetlenia gry (od piątej części M7, 11 po pierwszej): wartości domyślne `LightingSettings`, numery trybów, `specularModelOf`, `usesNormalMap` i wartość startowa `normalMapping`, `buildLightSet` (otoczenie, księżyc, latarka w ręce od piątej części M7, a wcześniej w oku, i cztery przypadki `flashlightPose`, wyłącznik, stożek, światła punktowe z podanych pozycji, limit 16 i, od pierwszej części M7, przeliczenie kolorów z sRGB na liniowe przy nietkniętych intensywnościach). W M4 przypadków było 17: w M5 test funkcji `isDeadEnd` przeszedł do `MazeTests.cpp`, a sześć testów świateł w ślepych zaułkach zniknęło razem z tym kodem | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `tests/ExitTests.cpp` | 11 przypadków testowych wyjścia: `passageDistances` (korytarz, komórka nieosiągalna, droga przez przejścia), `farthestCell` (remis), labirynt wzorcowy 4 na 4 z ziarna 1 (wyjście w ślepym zaułku (3, 1)), wyjście jako ślepy zaułek z bramą dla 25 ziaren, labirynt z jednej komórki, `wallSegmentOn`, `exitZone`, pola wyjścia w `MazeWorld` | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/CrystalTests.cpp` | 14 przypadków testowych kryształów: `crystalCountFor`, labirynt wzorcowy (dokładnie dwa kryształy), labirynt startowy (13 kryształów, wyjście w komórce (6, 5)), różne komórki bez startu i wyjścia, ślepe zaułki najpierw, oba warianty modelu, powtarzalność, za mało wolnych komórek, błędne argumenty, pozycja spoczynku i światła, kołysanie, obrót, pulsowanie i blask | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/RoundTests.cpp` | 25 przypadków testowych rundy: wartości domyślne reguł, `requiredCrystalCount`, nowa runda, zasięg gracza, zbieranie i promień zbierania, bateria (zużycie tylko przy włączonej latarce, pusta wyłącza latarkę, doładowanie kryształem, kryształ w kroku wyczerpania), brama (otwarcie, opadanie w 1,5 s, zmiana progu w trakcie rundy), wygrana tylko przy otwartej bramie, zegary po wygranej, labirynt bez kryształów i z jednej komórki, migotanie, `lightingForFrame`, `crystalLightPositions`, limit świateł | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `tests/InteractablesTests.cpp` (M8) | 29 przypadków testowych dźwigni i kartek bez okna | [`../modules/game/interactables.md`](../modules/game/interactables.md) |
| `tests/RaycastTests.cpp` (M8) | 17 przypadków testowych promienia bez okna (pudełko, kula, najbliższe trafienie, punkt obrazu) | [`../modules/scene/picking.md`](../modules/scene/picking.md) |
| `tests/SkyboxTests.cpp` | 5 przypadków testowych plików nieba (M6): sześć kwadratów jednej wielkości z trzema kanałami, reguła wyboru ściany i teksela przepisana ze specyfikacji OpenGL (`facePointOf`), tarcza księżyca tam, skąd leci domyślne światło księżyca z `game::LightingSettings`, niebo jaśniejsze przy horyzoncie niż w zenicie i zgodność koloru po obu stronach każdej z dwunastu krawędzi sześcianu. Test czyta pliki loaderem, bez OpenGL | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcja 5.8 |
| `tests/TerrainTests.cpp` | 27 przypadków testowych terenu (M6): stałe, `Heightmap::sample` i `heightmapFromImage`, `terrainRelief`, rozmiar siatki, wzór wysokości i skala (0 daje płaski świat), `heightAt` w punktach siatki, na krawędziach, na przekątnej, poza siatką i w losowych punktach względem trójkąta siatki, `buildTerrainMesh` (liczby, kierunek nawijania, normalne, UV, styczne), `gridNormal`, `lowestHeightUnder`, prawdziwy plik `heightmap.png`, świat bez mapy na płaskim gruncie, ściany, słupki i brama zatopione bez szczelin, kryształy nad gruntem, stopy gracza na gruncie i kolizje takie same jak na płaskim | [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) |
| `tests/GrassTests.cpp` | 9 przypadków testowych trawy (M6): stałe i ustawienia domyślne, gęstość 0, powtarzalność z ziarna i inne ziarno, liczba kępek na ścianę i stronę z pasem, w którym stoją, podwojenie gęstości, żadna kępka w ścianie, słupku ani bramie, kępki na gruncie z liczbą losową od 0 do 1, rozsiew na wzgórzach z dala od labiryntu | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md) |
| `tests/BloomTests.cpp` (M7, część druga) | 7 przypadków testowych `game/Bloom.*`: rozmiar celu dla parzystej i nieparzystej sceny, nie mniej niż 1 piksel, suma wag jądra równa 1, wagi malejące i dodatnie, zgodność z funkcją Gaussa (i dwie liczby policzone ręcznie: 0,1370 i 0,0185), wartości startowe ustawień w zakresach suwaków. Samych przebiegów (shaderów) testy nie uruchamiają | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 5.8 |
| `tests/FogTests.cpp` (M7, część trzecia) | 11 przypadków testowych `game/Fog.*`: współczynnik wysokości równy 1 na wysokości bazowej i pod nią, malejący nad nią (połowa co `ln(2) / heightFalloff` metrów, liczba policzona ręcznie 0,4966), zanik 0 daje to samo na każdej wysokości, brak mgły przy odległości 0, gęstości 0 i współczynniku 0, mgła rosnąca z każdym metrem do 100 m i nie większa niż 1, prawo wykładnicze (połowa po `ln(2) / density` metrach, liczba policzona ręcznie 0,5507, odcinki 10 m i 5 m razem jak 15 m), `fogAmountAt` z odległością od oka i wysokością punktu, `worldPositionFromDepth` (punkt świata odzyskany z miejsca na ekranie i głębi, środek ekranu przy głębi 0 i 1 na płaszczyznach przycinania) i wartości startowe (księżyc czysty, niebo pod horyzontem zakryte, podłoże 8 m dalej z mgłą między 0,2 a 0,6). Samego shadera testy nie uruchamiają | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 5.8 i 5.12 |
| `tests/VignetteTests.cpp` (M7, część trzecia) | 7 przypadków testowych `game/Vignette.*`: `VIGNETTE_CORNER_DISTANCE` równe pierwiastkowi z 0,5, środek ekranu i wszystko wewnątrz promienia bez zmiany, siła 0 nie zmienia niczego, cztery narożniki z tym samym wynikiem `1 - strength`, współczynnik malejący od promienia do narożnika (w połowie drogi `1 - strength / 2`), środek prawej i środek górnej krawędzi z tym samym wynikiem (brak korekty proporcji okna), wartości startowe (narożniki zachowują co najmniej połowę światła). Samego shadera testy nie uruchamiają | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 5.8 i 5.13 |
| `tests/DiscoveryTests.cpp` (M7, część szósta) | 19 przypadków testowych `game/Discovery.*`, `startRound` i `updateRound` w części odkrywania: pusta siatka, `discover`, korytarz do ściany, róg, jednakowy zasięg w czterech kierunkach, odnoga z linii, brzeg labiryntu, pozycja spoza labiryntu, ściana usunięta później, nowa runda, wygrana |
| `tests/MinimapTests.cpp` (M7, część szósta) | 19 przypadków testowych `game/Minimap.*`: wartości startowe ustawień, kwadrat w czterech rogach, Retina, mały framebuffer, rzut ortograficzny, podłogi, `revealAll`, ściany, brama, kryształy, strzałka, minimalne rozmiary w pikselach, największy labirynt |
| `tests/ShadowTests.cpp` (M7, część czwarta) | 31 przypadków testowych `scene/LightSpace.*` i `game/Shadows.*` (16 po czwartej części, 15 doszło w piątej: `spotLightSpace`, `biasForShader`, `shadowTexelSizeAt`, `flashlightShadowDefaults`): pudełko światła kierunkowego mieści każdy narożnik podanego pudełka i zostawia wolny tylko margines, światło prosto w dół i prawie prosto w dół, kierunek o długości zero, długość kierunku nie zmienia pudełka, punkty na jednym promieniu światła trafiają w ten sam teksel i różnią się tylko głębią, punkt poza pudełkiem ląduje poza mapą, pudełko rzucających cień mieści teren i każde pudełko kolizji labiryntu startowego, teksel mapy księżyca jest dość mały dla ścian labiryntu startowego, rozmiar teksela liczony z większego boku, bias jako część stała plus część nachylenia, bias w metrach jako udział głębokości mapy, rozmiary obu rozdzielczości, jądro PCF, `moonDirection` zgodne z kierunkiem świateł klatki. Samych shaderów i klasy `ShadowMap` testy nie uruchamiają | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 5.10 |
| `tests/ColorSpaceTests.cpp` (M7) | 9 przypadków testowych `gfx::srgbToLinear` i `gfx::linearToSrgb`: czerń i biel, wartości wzorcowe standardu (0,5 w sRGB to 0,21404 światła, 0,5 światła to 0,73536 w sRGB), odcinek prosty i jego styk z krzywą, różnica wobec potęgi 2,2, odwracalność dla każdego z 256 bajtów, zachowanie kolejności, przycinanie poza zakresem, kolor kanał po kanale | [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) |
| `tests/FramebufferTests.cpp` (M7) | 3 przypadki testowe części `gfx::Framebuffer`, które nie wymagają kontekstu OpenGL: nazwy formatów, pusty `FramebufferSpec`, osobny tekst dla każdego stanu z `glCheckFramebufferStatus`. Samej klasy (tworzenie, wiązanie, zmiana rozmiaru) testy nie obejmują | [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) |
| `tests/TransformTests.cpp` | 4 przypadki testowe `scene::normalMatrix`: obiekt tylko przesunięty, obiekt obrócony, skala różna na osiach (tylko macierz normalnych zachowuje prostopadłość) i skala równa | [`../modules/scene/transforms.md`](../modules/scene/transforms.md) |

Każdy plik źródłowy zaczyna się komentarzem z jednym zdaniem opisu i odnośnikiem
`See docs/modules/...`. To wymaganie z PRD (sekcja 7). Dotyczy też plików shaderów, w
których komentarz stoi pod linią `#version`, bo ta musi być pierwsza. Wyjątkiem są pliki
dołączane z `assets/shaders/common/` (dziś pięć, z `shadows.glsl`): nie mają linii `#version`, więc zaczynają się od komentarza. Odnośnik wskazuje najbardziej
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
  (`glm::mat4`, `glm::mat3`, `glm::vec3`). Od drugiej części M7 `Shader.hpp` dołącza też
  `<span>`: setter tablicy `setFloatArray` przyjmuje `std::span<const float>`. Nic z GLFW, `scene/`, `game/` ani `debug/`.
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
  `Light.cpp` `<algorithm>` i `<cmath>`, a `LightBlock.cpp` `<algorithm>` i `<cstddef>`.
  `LightSpace.hpp` (M7, część czwarta) dołącza samo `<glm/glm.hpp>`, a typ `Aabb` zapowiada
  deklaracją. `LightSpace.cpp` dołącza `scene/Collider.hpp`, `<glm/gtc/matrix_transform.hpp>`,
  `<algorithm>`, `<array>` i `<cmath>`. Nic
  z GLAD, GLFW, `core/`, `gfx/`, `game/` ani `debug/`: blok `LightBlock` jest w `scene/`
  opisany jako zwykłe bajty, a na kartę wysyła go dopiero `game::LightRig`.
- Dwa nowe nagłówki `src/gfx/` różnią się od reszty. `Vertex.hpp` **nie dołącza GLAD**: tylko
  `<glm/glm.hpp>`, `<cstdint>` i `<type_traits>`, żeby mogły go używać loader i testy.
  `Mesh.hpp` dołącza `gfx/Buffer.hpp`, `gfx/Vertex.hpp`, `gfx/VertexArray.hpp`,
  `<glad/gl.h>`, `<cstdint>` i `<span>`, a `Mesh.cpp` do tego `core/GlCheck.hpp`.
- `src/gfx/Texture2D.hpp` dołącza `<glad/gl.h>` i, od pierwszej części M7,
  `gfx/ColorSpace.hpp`, a `Texture2D.cpp` do tego `core/GlCheck.hpp`, `core/Log.hpp`, od M7
  `gfx/Extensions.hpp` i bibliotekę standardową. Nie dołącza niczego z
  `assets/`: teksturę tworzy się z surowych bajtów, a nie z `assets::Image`.
- Pliki `gfx/` z pierwszej części M7: `ColorSpace.hpp` dołącza tylko `<glm/glm.hpp>`, a
  `ColorSpace.cpp` do tego `<algorithm>` i `<cmath>`: to zwykła matematyka bez GLAD, więc
  mogą jej używać `game_logic` i testy. `Extensions.hpp` nie dołącza niczego, a
  `Extensions.cpp` dołącza `core/GlCheck.hpp`, `<glad/gl.h>` i `<cstring>`. `Framebuffer.hpp`
  dołącza tylko `<glad/gl.h>`, a `Framebuffer.cpp` do tego `core/GlCheck.hpp`, `core/Log.hpp`
  i `<string>`. `Cubemap.hpp` dołącza od M7 także `gfx/ColorSpace.hpp`.
- `src/gfx/ComparisonSampler.hpp` (M7, część czwarta) dołącza tylko `<glad/gl.h>`, a
  `ComparisonSampler.cpp` do tego `core/GlCheck.hpp` i `<array>` (cztery liczby koloru
  ramki). Nic ze `scene/`, `game/` ani `debug/`: klasa nie wie, że czyta mapę cieni księżyca.
- `src/game/Lighting.cpp` (biblioteka `game_logic`) dołącza od M7 `gfx/ColorSpace.hpp`. To
  dozwolony kierunek (`game` zna `gfx`) i nie wciąga OpenGL do logiki gry, bo ten nagłówek
  nie zna GLAD. Tak samo `ColliderLines.cpp` i `NightMazeApp.cpp`.
- `src/game/PostProcess.hpp` dołącza `core/Window.hpp` (typ `core::Size`),
  `gfx/Framebuffer.hpp` i `gfx/VertexArray.hpp`, a od drugiej części M7 także
  `game/Bloom.hpp` (pole `BloomSettings` w `PostProcessSettings`), a od trzeciej `game/Fog.hpp`,
  `game/Vignette.hpp` (pola `FogSettings` i `VignetteSettings`) i `<glm/glm.hpp>` (macierz i
  wektor w `SceneView`). `PostProcess.cpp` do tego
  `core/GlCheck.hpp`, `game/ShaderUniforms.hpp`, `gfx/Shader.hpp`, `<algorithm>`, `<cmath>`,
  od drugiej części M7 `<array>` (tablica wag rozmycia) i, od trzeciej, `gfx/ColorSpace.hpp`
  (kolor mgły przeliczany z sRGB). Nic z `debug/`.
- `src/game/ShadowMap.hpp` (M7, część czwarta, program `night_maze`) dołącza
  `gfx/ComparisonSampler.hpp`, `gfx/Framebuffer.hpp`, `gfx/VertexArray.hpp` i `<glad/gl.h>`,
  a typy `gfx::Shader`, `scene::LightSpace`, `ShadowSettings` i `ShadowUniformNames`
  zapowiada deklaracjami. `ShadowMap.cpp` dołącza do tego `core/GlCheck.hpp`,
  `game/PostProcess.hpp` (typ `AttachmentPreview`), `game/ShaderUniforms.hpp`,
  `game/Shadows.hpp`, `gfx/Shader.hpp`, `scene/LightSpace.hpp` i `<algorithm>`. Nic z `debug/`.
- `src/game/Bloom.hpp` (M7, część druga, biblioteka `game_logic`) dołącza tylko `<array>`,
  a `Bloom.cpp` do tego `<algorithm>`, `<cmath>` i `<cstddef>`. Nic z GLAD, GLM, `core/`,
  `gfx/` ani `scene/`: to najmniej zależny plik warstwy `game`, i dlatego test może go
  linkować bez okna.
- `src/game/Fog.hpp` i `src/game/Vignette.hpp` (M7, część trzecia, biblioteka `game_logic`)
  dołączają tylko `<glm/glm.hpp>` (wektory, a w `Fog.hpp` także macierz). `Fog.cpp` do
  tego `<algorithm>` i `<cmath>`, a `Vignette.cpp` niczego więcej. Nic z GLAD, `core/`,
  `gfx/` ani `scene/`: GLM przychodzi przez `engine`, a test linkuje oba pliki bez okna.
- `src/game/Shadows.hpp` (M7, część czwarta, biblioteka `game_logic`) dołącza tylko
  `scene/Collider.hpp` (typ `scene::Aabb`), a `scene::LightSpace` i `Terrain` zapowiada
  deklaracjami. `Shadows.cpp` dołącza do tego `game/MazeLayout.hpp` (stała `PILLAR_HEIGHT`),
  `game/Terrain.hpp`, `scene/LightSpace.hpp` i `<algorithm>`. Nic z GLAD, `core/` ani `gfx/`:
  to, co wymaga OpenGL, jest w `ShadowMap.*`, a test linkuje `Shadows` bez okna.
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
- Pliki logiki gry bez okna w `src/game/` (`Crystals.*`, `Exit.*`, `Grass.*`, `Lighting.*`,
  `Maze.*`, `MazeGenerator.*`, `MazeLayout.*`, `MazeWorld.*`, `Player.*`, `Round.*`,
  `Terrain.*`) dołączają
  bibliotekę standardową, GLM (bezpośrednio wszystkie poza `Maze.*`, `MazeGenerator.*` i
  `Exit.*`), nagłówki z `game/` i ze `scene/`: `MazeLayout.hpp`, `Player.hpp`, `Exit.hpp`,
  `MazeWorld.hpp` i `Round.hpp` dołączają `scene/Collider.hpp`, `MazeWorld.hpp` do tego
  `game/Crystals.hpp`, `game/Maze.hpp` i `game/MazeLayout.hpp`, `MazeWorld.cpp`
  `game/Exit.hpp`, `game/MazeGenerator.hpp` i `scene/Transform.hpp`, `Player.cpp`
  `scene/Camera.hpp`, `Lighting.hpp` `scene/Light.hpp` (nagłówka `game/Maze.hpp` już nie:
  o labiryncie nic nie wie), `Crystals.cpp` `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`
  i `scene/Light.hpp` (dla stałej `MAX_POINT_LIGHTS`), `Round.hpp` `game/Lighting.hpp` i
  `game/MazeWorld.hpp`, a `Round.cpp` `game/Crystals.hpp`. Od M6: `MazeWorld.hpp` dołącza
  też `game/Terrain.hpp`, `Player.cpp` `game/Terrain.hpp` (w nagłówku `Player.hpp` wystarcza
  deklaracja `class Terrain;`), `Grass.hpp` samo `<glm/glm.hpp>` i `<vector>`, a `Grass.cpp`
  `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`, `game/MazeWorld.hpp` i `game/Terrain.hpp`.
  Nic z GLAD, GLFW, `core/` ani `debug/`: dlatego dają się testować bez okna. Do M5 lista
  zakazów obejmowała też `gfx/` i `assets/`. Od M6 jest jeden wyjątek, `Terrain.*`:
  `Terrain.hpp` dołącza `gfx/Vertex.hpp` (wierzchołek siatki terenu) i
  `assets/ImageLoader.hpp` (obraz, z którego powstaje mapa wysokości), a `Terrain.cpp`
  `assets/Tangents.hpp` (styczne). Wszystkie trzy to nagłówki samych danych i matematyki:
  żaden nie dołącza GLAD, więc teren dalej nie potrzebuje okna. Kierunek zależności jest
  dozwolony (`game/` stoi nad `gfx/` i `assets/`), a oba nagłówki wymienia komentarz przy
  `target_link_libraries(game_logic PUBLIC engine)` w `CMakeLists.txt`.
- Klasy i funkcje rysujące w `src/game/` należą do programu, nie do `game_logic`.
  `MazeRenderer.hpp` nie dołącza niczego (typy z `assets/` i `gfx/`
  zapowiada deklaracjami), a `MazeRenderer.cpp` `assets/AssetCache.hpp`, `core/Paths.hpp`,
  `game/MazeWorld.hpp`, `game/ModelDraw.hpp`, `game/ShaderUniforms.hpp` i `gfx/Shader.hpp`.
  `ModelDraw.hpp` dołącza tylko `<glm/glm.hpp>` i `<span>`, a `ModelDraw.cpp`
  `assets/AssetCache.hpp`, `game/ShaderUniforms.hpp`, `gfx/Shader.hpp`, od M6
  `gfx/Mesh.hpp` i `gfx/Texture2D.hpp` (dla `drawMesh`) i
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
  `scene/LightBlock.hpp`. Dwie klasy z M6: `TerrainRenderer.hpp` dołącza tylko `gfx/Mesh.hpp`
  (resztę zapowiada deklaracjami), a `TerrainRenderer.cpp` `assets/AssetCache.hpp`,
  `core/GlCheck.hpp`, `core/Paths.hpp`, `game/ModelDraw.hpp`, `game/ShaderUniforms.hpp`,
  `game/Terrain.hpp`, `gfx/Shader.hpp` i `gfx/Texture2D.hpp`. `GrassRenderer.hpp` dołącza
  `gfx/Mesh.hpp`, `<glm/glm.hpp>`, `<cstddef>` i `<span>`, a `GrassRenderer.cpp`
  `core/GlCheck.hpp`, `game/Grass.hpp`, `game/MazeRenderer.hpp` (dla typu `ViewMode`),
  `game/ShaderUniforms.hpp`, `gfx/Shader.hpp` i `gfx/Vertex.hpp`.
- `src/game/ShaderUniforms.hpp` dołącza `<glad/gl.h>` (typ `GLuint` punktu wiązania), więc
  należy do programu, nie do `game_logic`.
- Pliki w `tests/` dołączają `<doctest/doctest.h>`, nagłówki testowanego kodu
  (`scene/Collider.hpp`, `game/Maze.hpp`, `game/MazeGenerator.hpp`, `game/MazeLayout.hpp`,
  `game/MazeWorld.hpp`, `game/Player.hpp`, `assets/ObjLoader.hpp`, `assets/ImageLoader.hpp`,
  od M4 `gfx/ShaderSource.hpp`, `scene/Light.hpp`, `scene/LightBlock.hpp`,
  `scene/Transform.hpp` i `game/Lighting.hpp`, od M5 `game/Exit.hpp`,
  `game/Crystals.hpp` i `game/Round.hpp`, od M6 `game/Terrain.hpp`, `game/Grass.hpp` i
  `assets/Tangents.hpp`, a od czwartej części M7 `game/Shadows.hpp` i `scene/LightSpace.hpp`) i bibliotekę standardową. Żaden plik w `src/` nie dołącza niczego z `tests/` ani nagłówka
  doctest.
- `src/game/NightMazeApp.hpp` dołącza `assets/AssetCache.hpp`, `core/Application.hpp`,
  szesnaście nagłówków z `game/` (od M7 z `PostProcess.hpp`, od czwartej części M7 z `ShadowMap.hpp` i `Shadows.hpp`: `ColliderLines.hpp`, `GameplayRenderer.hpp`, `Grass.hpp`,
  `GrassRenderer.hpp`, `LightRig.hpp`, `Lighting.hpp`, `MazeRenderer.hpp`, `MazeWorld.hpp`,
  `Player.hpp`, `PostProcess.hpp`, `Round.hpp`, `ShadowMap.hpp`, `Shadows.hpp`, `Skybox.hpp`, `Terrain.hpp`, `TerrainRenderer.hpp`), jeden z
  `gfx/` (`Shader.hpp`; `Buffer.hpp` i `VertexArray.hpp` odpadły razem z kostką), trzy ze
  `scene/` (`Camera.hpp`, `Collider.hpp`, od czwartej części M7 `LightSpace.hpp`), `<glm/glm.hpp>`, `<array>`, `<cstddef>`, `<vector>` i nic z
  `debug/`.
  Komentarz w klasie mówi wprost: "It knows nothing about the debug UI". `NightMazeApp.cpp`
  dołącza do tego `core/GlCheck.hpp`, `core/Paths.hpp`, `game/Crystals.hpp`,
  `game/ShaderUniforms.hpp`, od M6 `assets/ImageLoader.hpp` i `core/Log.hpp` (wczytanie mapy
  wysokości i linia w logu), od M7 `gfx/ColorSpace.hpp` (przeliczenie koloru tła i blasku kryształów) i
  `<GLFW/glfw3.h>`, ten ostatni tylko dla stałych klawiszy i przycisku myszy (`GLFW_KEY_W`,
  `GLFW_KEY_N`, `GLFW_KEY_F`, `GLFW_KEY_R`, `GLFW_MOUSE_BUTTON_LEFT`): o stan wejścia pyta wyłącznie `core::Input`.
  Od M6 jest jedno wywołanie funkcji GLFW wprost: `glfwGetTime()` w `drawGrass`, zegar wiatru.
- `src/debug/DebugUI.cpp` dołącza `core/Window.hpp`, `debug/DebugContext.hpp`,
  `debug/Hud.hpp`, `debug/Theme.hpp`, nagłówki dwunastu paneli, `game/Lighting.hpp` (panel
  Renderer dostaje
  pole `mode` struktury `LightingSettings`), od M6 `game/Skybox.hpp` i `game/MazeWorld.hpp`
  (panel Terrain dostaje pole `terrain` struktury `MazeWorld`), od pierwszej części M7 `game/PostProcess.hpp` (zeruje pole `previews` struktury `PostProcessSettings`), od czwartej `game/Shadows.hpp` (zeruje pole `preview` struktury `ShadowSettings`) i nagłówki ImGui. `DebugUI.hpp` dołącza od M7 `debug/RawTextureSampler.hpp` (pole przez wartość), a przez niego `<glad/gl.h>`.
- `src/debug/Theme.cpp` dołącza `core/Log.hpp` i `core/Paths.hpp` (błąd wczytania czcionki i
  ścieżka do `assets/fonts`). `Theme.hpp` i `PanelLayout.hpp` dołączają `<imgui.h>`, bo
  pokazują typy `ImVec4` i `ImVec2`: to jedyne nagłówki projektu z nagłówkiem ImGui i
  dołączają je tylko pliki `.cpp` z `src/debug/`. Każdy z dwunastu plików paneli dołącza
  `debug/PanelLayout.hpp`, a panele Shaders, Assets i Maze także `debug/Theme.hpp`.
  `Hud.cpp` dołącza `debug/Theme.hpp` (kolory), `game/Round.hpp` i od M6
  `debug/PanelLayout.hpp` (wysokość rzędów pasków, pod którymi stoi HUD), a `Hud.hpp` nie dołącza
  niczego: oba typy gry zapowiada deklaracjami.
- `src/debug/panels/CameraPanel.cpp` dołącza `game/Player.hpp`, `scene/Camera.hpp` i
  `<glm/gtc/type_ptr.hpp>`. Panele Maze i Collision dołączają `game/MazeLayout.hpp`,
  `game/MazeWorld.hpp`, `game/Player.hpp` i `game/Round.hpp` (Maze także `scene/Camera.hpp`,
  Collision
  `scene/Collider.hpp`), a panel Assets `assets/AssetCache.hpp`, `core/Paths.hpp`,
  `game/MazeRenderer.hpp`, `gfx/Texture2D.hpp` i od M7 `debug/RawTextureSampler.hpp`. Panel Framebuffers (M7) dołącza `game/PostProcess.hpp` i `gfx/Framebuffer.hpp`, panel Shadows (M7, część czwarta) `game/ShadowMap.hpp`, `game/Shadows.hpp`, `gfx/Framebuffer.hpp`, `scene/LightSpace.hpp` i `<algorithm>`, a `RawTextureSampler.cpp` `core/GlCheck.hpp`, `gfx/Extensions.hpp` i `<imgui.h>`. Panel Lights dołącza `game/Lighting.hpp`,
  `game/Round.hpp`, `scene/Light.hpp` i `<glm/gtc/type_ptr.hpp>`, panel Gameplay tylko
  `game/Round.hpp`, panel Terrain tylko `game/Terrain.hpp`, panel Grass tylko
  `game/Grass.hpp`, a panel Renderer
  `core/Time.hpp`, `core/Window.hpp`, `game/Lighting.hpp` i od M6 `game/Skybox.hpp`.
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
| `engine` | biblioteka statyczna | `src/assets/*`, `src/core/*`, `src/gfx/*`, `src/scene/*` (od M8 także `Raycast.*`) | `glad`, `glfw`, `glm::glm-header-only` (`PUBLIC`), `stb_image` (`PRIVATE`) |
| `game_logic` | biblioteka statyczna | `src/game/Bloom.*` (M7, część druga), `src/game/Crystals.*`, `src/game/Discovery.*` (M7, część szósta), `src/game/Exit.*`, `src/game/Fog.*` (M7, część trzecia), `src/game/Grass.*`, `src/game/Interactables.*` (M8, tylko podstawy bez okna), `src/game/Lighting.*`, `src/game/Maze.*`, `src/game/MazeGenerator.*`, `src/game/MazeLayout.*`, `src/game/MazeWorld.*`, `src/game/Minimap.*` (M7, część szósta), `src/game/Player.*`, `src/game/Round.*`, `src/game/Shadows.*` (M7, część czwarta), `src/game/Terrain.*`, `src/game/Vignette.*` (M7, część trzecia) | `engine` (`PUBLIC`) |
| `night_maze` | program | `src/main.cpp`, `src/game/NightMazeApp.*`, `src/game/MazeRenderer.*`, `src/game/GameplayRenderer.*`, `src/game/TerrainRenderer.*`, `src/game/GrassRenderer.*`, `src/game/ModelDraw.*`, `src/game/ColliderLines.*`, `src/game/LightRig.*`, `src/game/Skybox.*`, `src/game/PostProcess.*`, `src/game/ShadowMap.*` (M7, część czwarta), `src/game/MinimapRenderer.*` (M7, część szósta), `src/game/ShaderUniforms.hpp`, `src/debug/*` (kontekst, `DebugUI`, HUD, układ, motyw, sampler podglądów, dwanaście paneli) | `engine`, `game_logic`, `imgui` (`PRIVATE`) |
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

Co doszło do targetów z terenem i trawą (M6, część druga):

| Target | Nowe pliki |
|---|---|
| `engine` | żadnego nowego pliku: etap geometrii doszedł do istniejącego `src/gfx/Shader.*`, a punkty do `src/gfx/Mesh.hpp` (sam komentarz: klasa rysowała już dowolny prymityw) |
| `game_logic` | `src/game/Terrain.*`, `src/game/Grass.*` |
| `night_maze` | `src/game/TerrainRenderer.*`, `src/game/GrassRenderer.*`, `src/debug/panels/TerrainPanel.*`, `src/debug/panels/GrassPanel.*` |
| `night_maze_tests` | `tests/TerrainTests.cpp`, `tests/GrassTests.cpp` |

Linia podziału jest ta sama, i tu widać ją najwyraźniej: teren i trawa są rozcięte na dwie
pary. Wysokości, trójkąty i miejsca kępek to zwykłe dane i matematyka (`Terrain`, `Grass`),
więc są w `game_logic` i mają 36 przypadków testowych. Siatki na karcie i wywołania rysujące
(`TerrainRenderer`, `GrassRenderer`) są w programie. Z repozytorium nie ubył żaden plik
źródłowy C++: płytki podłogi były tylko modelem, teksturami i skryptem, a w kodzie polem i
linią w `MazeRenderer` oraz listą macierzy w `MazeWorld`.

Co doszło do targetów z buforem HDR i gammą (M7, część pierwsza):

| Target | Nowe pliki |
|---|---|
| `engine` | `src/gfx/ColorSpace.*`, `src/gfx/Extensions.*`, `src/gfx/Framebuffer.*` |
| `game_logic` | żadnego nowego pliku: `Lighting.cpp` zaczęło wołać `gfx::srgbToLinear` |
| `night_maze` | `src/game/PostProcess.*`, `src/debug/RawTextureSampler.*`, `src/debug/panels/FramebuffersPanel.*` |
| `night_maze_tests` | `tests/ColorSpaceTests.cpp`, `tests/FramebufferTests.cpp` |

Co doszło z bloomem (M7, część druga):

| Target | Nowe pliki |
|---|---|
| `engine` | żadnego nowego pliku: `gfx::Shader` dostało funkcję `setFloatArray` |
| `game_logic` | `src/game/Bloom.*` |
| `night_maze` | żadnego nowego pliku: `PostProcess.*`, `NightMazeApp.*`, `ShaderUniforms.hpp` i panel Framebuffers zostały rozbudowane |
| `night_maze_tests` | `tests/BloomTests.cpp` |

`Bloom.*` jest w `game_logic`, chociaż bloom to efekt rysowania: plik zawiera tylko stałe,
strukturę ustawień i dwie funkcje liczące (rozmiar celu, wagi rozmycia), bez jednego
wywołania OpenGL. Dzięki temu test może je linkować. Przebiegi rysuje `PostProcess`, które
zostaje w programie.

Co doszło z mgłą i winietą (M7, część trzecia):

| Target | Nowe pliki |
|---|---|
| `engine` | żadnego nowego pliku i żadnej zmiany |
| `game_logic` | `src/game/Fog.*`, `src/game/Vignette.*` |
| `night_maze` | żadnego nowego pliku: `PostProcess.*`, `NightMazeApp.*`, `ShaderUniforms.hpp` i panel Framebuffers zostały rozbudowane |
| `night_maze_tests` | `tests/FogTests.cpp`, `tests/VignetteTests.cpp` |

`Fog.*` i `Vignette.*` są w `game_logic` z tego samego powodu co `Bloom.*`: zawierają tylko
struktury ustawień, stałe i funkcje liczące (współczynnik wysokości, ilość mgły, miejsce w
świecie z głębi, współczynnik winiety), bez jednego wywołania OpenGL. Te same wzory pod tymi
samymi nazwami ma shader `post/composite.frag`: testy sprawdzają wersję C++, a zgodności obu
wersji pilnuje tylko człowiek. Nowego przebiegu nie ma: oba efekty rysuje istniejące
`PostProcess::composite`.

Linia podziału znów ta sama. `Framebuffer` to obiekt OpenGL bez wiedzy o grze, więc jest w
`engine`, obok `Texture2D` i `Cubemap`. `ColorSpace` to matematyka bez OpenGL: też `engine`,
i dlatego ma testy (9 przypadków), a korzysta z niej nawet `game_logic`. `PostProcess` wie,
jakie formaty ma mieć bufor sceny i w jakiej kolejności idą przebiegi, i potrzebuje
kontekstu OpenGL, więc jest w programie. Z klasy `Framebuffer` testy obejmują tylko trzy
funkcje, które nie wołają OpenGL (3 przypadki).

Co doszło z cieniami księżyca (M7, część czwarta):

| Target | Nowe pliki |
|---|---|
| `engine` | `src/gfx/ComparisonSampler.*`, `src/scene/LightSpace.*` |
| `game_logic` | `src/game/Shadows.*` |
| `night_maze` | `src/game/ShadowMap.*`, `src/debug/panels/ShadowsPanel.*` |
| `night_maze_tests` | `tests/ShadowTests.cpp` |

Cienie są rozcięte na cztery pary plików tą samą linią co wszystko wcześniej. `LightSpace` to
sama matematyka na GLM bez wiedzy o grze, więc jest w warstwie `scene/` biblioteki `engine`.
`ComparisonSampler` to obiekt OpenGL bez wiedzy o grze: też `engine`, warstwa `gfx/`, obok
`Texture2D` i `Framebuffer`. `Shadows` zna teren i wymiary labiryntu, ale nie woła OpenGL,
więc jest w `game_logic` i razem z `LightSpace` ma 31 przypadków testowych (16 po czwartej części). `ShadowMap`
posiada framebuffer i sampler i potrzebuje kontekstu OpenGL, więc jest w programie, w
`src/game/` obok `PostProcess`. Warstwy `src/renderer/` z klasą `ShadowPass`, którą planował
PRD, nadal nie ma. Po tej części listy w `CMakeLists.txt` mają (policzone z pliku): `engine`
59 plików, `game_logic` 30, `night_maze` 59 i `night_maze_tests` 26.

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
(labirynt, generator, układ w świecie, `MazeWorld`, gracz, ustawienia oświetlenia, od M5
wyjście, kryształy i reguły rundy, od M6 teren i miejsca kępek trawy, a od M7 ustawienia i
matematyka bloomu, mgły, winiety i cieni), jest
biblioteką statyczną, którą linkują i gra, i testy. `NightMazeApp` i kod rysujący
(`MazeRenderer`, `GameplayRenderer`, `TerrainRenderer`, `GrassRenderer`, funkcje z `ModelDraw`, `ColliderLines`, `LightRig`, `Skybox`, od M7 `PostProcess` i `ShadowMap`) zostają w programie, bo potrzebują okna i kontekstu OpenGL, których test
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
    src/gfx/ColorSpace.cpp
    src/gfx/ColorSpace.hpp
    src/gfx/ComparisonSampler.cpp
    src/gfx/ComparisonSampler.hpp
    src/gfx/Cubemap.cpp
    src/gfx/Cubemap.hpp
    src/gfx/Extensions.cpp
    src/gfx/Extensions.hpp
    src/gfx/Framebuffer.cpp
    src/gfx/Framebuffer.hpp
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
    src/scene/LightSpace.cpp
    src/scene/LightSpace.hpp
    src/scene/Raycast.cpp
    src/scene/Raycast.hpp
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
  `Light`, `LightBlock` i, od czwartej części M7, `LightSpace`) oraz `src/gfx/Shader.*` (macierz jako parametr `setMat4`). `glm::glm-header-only`
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
# The maze, its generator, its layout in the world, the terrain it stands on, the places
# of the grass, the player, the settings of the lighting, of the bloom, of the fog, of
# the vignette and of the shadows and the rules of a round (exit, crystals, battery) are
# plain data and math.
# They live in a library of their own, and not in the night_maze executable, so that
# the test program can link them too: a test cannot link code that is inside another
# executable.
add_library(game_logic STATIC
    src/game/Bloom.cpp
    src/game/Bloom.hpp
    src/game/Crystals.cpp
    src/game/Crystals.hpp
    src/game/Exit.cpp
    src/game/Exit.hpp
    src/game/Fog.cpp
    src/game/Fog.hpp
    src/game/Grass.cpp
    src/game/Grass.hpp
    src/game/Interactables.cpp
    src/game/Interactables.hpp
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
    src/game/Shadows.cpp
    src/game/Shadows.hpp
    src/game/Terrain.cpp
    src/game/Terrain.hpp
    src/game/Vignette.cpp
    src/game/Vignette.hpp
)
# PUBLIC: the headers of this library (Crystals.hpp, Exit.hpp, Lighting.hpp,
# MazeLayout.hpp, MazeWorld.hpp, Player.hpp, Round.hpp, Shadows.hpp, Terrain.hpp) include
# headers of engine (scene/Collider.hpp, scene/Light.hpp, gfx/Vertex.hpp, assets/ImageLoader.hpp)
# and GLM, so whoever includes them needs the include paths of engine.
# The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)
```

- `add_library(game_logic STATIC ...)`: druga nasza biblioteka statyczna, z trzydziestu dwóch
  plików logiki gry bez okna (labirynt, `MazeWorld`, gracz, od M4 `Lighting`, od M5 `Exit`,
  `Crystals` i `Round`, od M6 `Terrain` i `Grass`, od drugiej części M7 `Bloom`, od trzeciej `Fog` i `Vignette`, od czwartej `Shadows`, a od M8 `Interactables`). Lista jest jawna, tak jak przy
  `engine`.
- `target_link_libraries(game_logic PUBLIC engine)`: `game_logic` używa `scene::Aabb` z
  `engine`. `PUBLIC`, bo nagłówek `game/MazeLayout.hpp` sam dołącza `scene/Collider.hpp` i
  GLM: każdy, kto go dołączy, potrzebuje ścieżek nagłówków `engine`. Komentarz w pliku
  wymienia od M6 dwa nowe nagłówki `engine`, które dołącza `Terrain.hpp`: `gfx/Vertex.hpp`
  i `assets/ImageLoader.hpp`. Dzięki temu nie ma tu
  osobnego `target_include_directories`: korzeń `src/` przychodzi z `engine`.
- `night_maze_enable_warnings(game_logic)`: te same ścisłe ostrzeżenia co dla `engine`.

**Blok 6: target `night_maze`**

```cmake
# ---- night_maze: the application and its debug UI -------------------------------------
# NightMazeApp and the classes that draw (MazeRenderer, GameplayRenderer, TerrainRenderer,
# GrassRenderer, ColliderLines, LightRig, Skybox, PostProcess, ShadowMap, with the shared
# ModelDraw) stay in the executable: they need a window and an OpenGL context, so they
# are not something a test can run.
add_executable(night_maze
    src/main.cpp
    src/game/ColliderLines.cpp
    src/game/ColliderLines.hpp
    src/game/GameplayRenderer.cpp
    src/game/GameplayRenderer.hpp
    src/game/GrassRenderer.cpp
    src/game/GrassRenderer.hpp
    src/game/LightRig.cpp
    src/game/LightRig.hpp
    src/game/MazeRenderer.cpp
    src/game/MazeRenderer.hpp
    src/game/ModelDraw.cpp
    src/game/ModelDraw.hpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/game/PostProcess.cpp
    src/game/PostProcess.hpp
    src/game/ShaderUniforms.hpp
    src/game/ShadowMap.cpp
    src/game/ShadowMap.hpp
    src/game/Skybox.cpp
    src/game/Skybox.hpp
    src/game/TerrainRenderer.cpp
    src/game/TerrainRenderer.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/Hud.cpp
    src/debug/Hud.hpp
    src/debug/PanelLayout.cpp
    src/debug/PanelLayout.hpp
    src/debug/RawTextureSampler.cpp
    src/debug/RawTextureSampler.hpp
    src/debug/Theme.cpp
    src/debug/Theme.hpp
    src/debug/panels/AssetsPanel.cpp
    src/debug/panels/AssetsPanel.hpp
    src/debug/panels/CameraPanel.cpp
    src/debug/panels/CameraPanel.hpp
    src/debug/panels/CollisionPanel.cpp
    src/debug/panels/CollisionPanel.hpp
    src/debug/panels/FramebuffersPanel.cpp
    src/debug/panels/FramebuffersPanel.hpp
    src/debug/panels/GameplayPanel.cpp
    src/debug/panels/GameplayPanel.hpp
    src/debug/panels/GrassPanel.cpp
    src/debug/panels/GrassPanel.hpp
    src/debug/panels/LightsPanel.cpp
    src/debug/panels/LightsPanel.hpp
    src/debug/panels/MazePanel.cpp
    src/debug/panels/MazePanel.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
    src/debug/panels/ShadersPanel.cpp
    src/debug/panels/ShadersPanel.hpp
    src/debug/panels/ShadowsPanel.cpp
    src/debug/panels/ShadowsPanel.hpp
    src/debug/panels/TerrainPanel.cpp
    src/debug/panels/TerrainPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine game_logic imgui)
night_maze_enable_warnings(night_maze)
```

- `add_executable` tworzy program. Bez słowa `WIN32`, więc na Windowsie jest to aplikacja
  konsolowa (opis w [`build-windows.md`](build-windows.md)).
- `PRIVATE engine game_logic imgui`: program niczego dalej nie przekazuje, więc `PRIVATE`
  wystarcza. ImGui linkuje tylko `night_maze`, nigdy `engine` ani `game_logic`.
- `game_logic` jest potrzebne: `NightMazeApp` buduje labirynt (`buildMazeWorld`) i światła
  klatki (`buildLightSet`), prowadzi rundę (`startRound`, `updateRound`), od M6 buduje
  teren i sadzi trawę (`placeOnTerrain`, `buildTerrainMesh`, `placeGrass`), ma pola typu
  `Player`, `LightingSettings`, `GameplaySettings`, `Round`, `Heightmap`, `TerrainSettings`
  i `GrassSettings`, a panele i HUD czytają
  `MazeWorld`, `Player`, `LightingSettings`, `Round`, `Terrain` i `GrassSettings`. Od czwartej
  części M7 dochodzi pudełko światła księżyca (`shadowCasterBounds`, `moonDirection`,
  `shadowMapSize`) i pole typu `ShadowSettings`, które edytuje panel Shadows.
- Z kodu gry w programie zostają `NightMazeApp`, `MazeRenderer`, `GameplayRenderer`,
  `TerrainRenderer`, `GrassRenderer`, `ModelDraw`, `ColliderLines`, `LightRig`, `Skybox`,
  od M7 `PostProcess` i `ShadowMap`,
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
    tests/BloomTests.cpp
    tests/ColliderTests.cpp
    tests/ColorSpaceTests.cpp
    tests/CrystalTests.cpp
    tests/ExitTests.cpp
    tests/FogTests.cpp
    tests/FramebufferTests.cpp
    tests/GrassTests.cpp
    tests/ImageLoaderTests.cpp
    tests/InteractablesTests.cpp
    tests/LightTests.cpp
    tests/LightingTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/MazeWorldTests.cpp
    tests/ObjLoaderTests.cpp
    tests/PlayerTests.cpp
    tests/RaycastTests.cpp
    tests/RoundTests.cpp
    tests/ShaderSourceTests.cpp
    tests/ShadowTests.cpp
    tests/SkyboxTests.cpp
    tests/TangentTests.cpp
    tests/TerrainTests.cpp
    tests/TransformTests.cpp
    tests/VignetteTests.cpp
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
- `add_executable(night_maze_tests ...)` buduje program testowy z dwudziestu sześciu plików
  (`tests/main.cpp` i dwadzieścia siedem plików z testami, od pierwszej części M7 z `ColorSpaceTests.cpp` i `FramebufferTests.cpp`, od drugiej z `BloomTests.cpp`, od trzeciej z `FogTests.cpp` i `VignetteTests.cpp`, od czwartej z `ShadowTests.cpp`, od M8 z `RaycastTests.cpp` i `InteractablesTests.cpp`) przy każdym zwykłym buildzie (jest częścią
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
| `files.associations` | `*.vert`, `*.frag`, `*.geom`, `*.glsl` na `glsl` | pliki shaderów są traktowane jako język GLSL (kolorowanie składni). Dziś jest ich dwadzieścia pięć (policzone z dysku): pary `textured`, `color`, `lit`, `gouraud`, `skybox` i `shadow_depth` w `assets/shaders/`, trójka `grass` (jej plik `grass.geom`, shader geometrii z M6, jest pierwszym plikiem projektu, dla którego potrzebny jest wzorzec `*.geom`), pięć plików przebiegów po scenie w `assets/shaders/post/` oraz pięć plików dołączanych w `assets/shaders/common/`: `color.glsl`, `depth.glsl`, `lighting.glsl`, `normal_map.glsl` i `shadows.glsl` (to dla nich jest wzorzec `*.glsl`) |

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
| `make run` | `make debug`, potem uruchamia `build/debug/night_maze` (na Windowsie `build/debug/Debug/night_maze.exe`) | najczęstsze polecenie |
| `make run-release` | `make release`, potem uruchamia `build/release/night_maze` (na Windowsie `build/release/Release/night_maze.exe`) | sprawdzenie 60 FPS |
| `make test` | `make debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure` | testy jednostkowe po zmianie w kolizjach albo labiryncie |
| `make test-release` | `make release`, potem `ctest --test-dir build/release -C Release --output-on-failure` | te same testy na kodzie z optymalizacjami |
| `make format` | `clang-format -i` na wszystkich plikach `.cpp` i `.hpp` z `src/` i `tests/` | naprawia formatowanie w miejscu |
| `make format-check` | `clang-format --dry-run --Werror` na tych samych plikach | tylko sprawdza, niczego nie zmienia |
| `make tidy` | `make debug`, potem `clang-tidy` na plikach `.cpp` z `src/` i `tests/`. Na Windowsie przedtem `cmake --preset debug -G Ninja -B build/ninja-debug` | analiza statyczna |
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
- **`$(wildcard ...)`** zastępuje `find` na Windowsie. Tam `make` uruchomiony z PowerShella
  nie ma powłoki typu Unix, a `find` to inny program (szuka tekstu w plikach), więc lista
  plików wychodziła pusta. `$(wildcard)` wykonuje sam `make`, ale nie schodzi w podkatalogi:
  każdy wzorzec to jeden poziom (`src/*.cpp`, `src/*/*.cpp`, `src/*/*/*.cpp`). Głębszy
  katalog wymaga dopisania kolejnego wzorca.
- **`@` przed poleceniem** wyłącza wypisanie samego polecenia. Używam go tylko przy `echo`,
  żeby tekst nie pojawiał się dwa razy. Pozostałe polecenia są wypisywane, więc widać, co
  dokładnie zostało uruchomione.
- **`ifeq ($(OS),Windows_NT)`** wybiera ścieżkę programu. Na Windowsie generator Visual Studio
  dodaje podkatalog konfiguracji i rozszerzenie `.exe` (sekcja 4.2).
- **`CLANG_TIDY`** bierze `clang-tidy` z `PATH`, a gdy go tam nie ma, z pakietu `llvm`
  Homebrew, który celowo nie jest dodawany do `PATH` (sekcja 3.6). `$$` w pliku Makefile to
  jeden znak `$` przekazany do powłoki. Na Windowsie `CLANG_FORMAT` i `CLANG_TIDY` wskazują
  programy dołączone do Build Tools, przez zmienną `VCINSTALLDIR`, którą ustawia środowisko
  deweloperskie (te programy nie trafiają do `PATH` nawet tam).
- **`TIDY_BUILD_DIR`** to katalog, z którego `clang-tidy` czyta `compile_commands.json`. Na
  Macu jest nim `build/debug`. Generator Visual Studio tego pliku nie zapisuje, więc na
  Windowsie cel `tidy` konfiguruje osobny katalog `build/ninja-debug` generatorem Ninja.
  Sama konfiguracja wystarcza, nic się tam nie kompiluje.
- **`uname` i `xcrun`** są pytane tylko poza Windowsem (`ifneq ($(OS),Windows_NT)`), bo na
  Windowsie tych programów nie ma.
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
Dziś paneli jest dwanaście i jest to dwanaście stałych `..._PLACEMENT` w jednym pliku,
`src/debug/PanelLayout.hpp`, ustawianych przez `placePanelOnFirstUse` z warunkiem
`ImGuiCond_FirstUseEver` i liczonych od rogów okna ([`../modules/debug-ui.md`](../modules/debug-ui.md)):
Renderer nad Lights w lewej kolumnie, Maze nad Assets w prawej, Collision i
Shaders na dole między kolumnami, a Camera i Gameplay u góry, między kolumnami, obok
siebie, zwinięte do pasków tytułu (pole `collapsed`). Od M6 pod nimi stoi drugi rząd takich
pasków: Terrain pod Camera i Grass pod Gameplay (pole `foldedRowsBefore`), od pierwszej części M7 trzeci rząd z jednym szerokim paskiem panelu Framebuffers, a od czwartej części M7 czwarty rząd z tak samo szerokim paskiem panelu Shadows. Ten warunek działa tylko wtedy, gdy
`imgui.ini` nie ma jeszcze
wpisu dla danego panelu. Plik zapisany przez starszą wersję programu trzyma panele na
starych miejscach i w starych rozmiarach, a plik sprzed M4 nie ma wpisu panelu Lights, więc
ten jeden panel dostaje miejsce z kodu i nachodzi na panele ze starego układu. Plik z M4
nie ma wpisu panelu Gameplay: ten panel dostaje miejsce z kodu, u góry obok panelu Camera,
gdzie w układzie z M4 nic nie stoi. Plik sprzed pierwszej części M6 trzyma panel Renderer w
wysokości 230, a ten ma dziś dwa wiersze więcej (pole `Skybox` i suwak `Sky brightness`,
wysokość startowa 284): nowe kontrolki są wtedy pod dolną krawędzią panelu. Plik sprzed
drugiej części M6 nie ma wpisów paneli Terrain i Grass: oba dostają miejsce z kodu, w drugim
rzędzie pasków, a jeśli stary plik trzyma panel Camera albo Gameplay rozwinięty, ten panel
zakrywa nowy pasek pod sobą (wszystkie cztery
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
   `game_logic` dodatkowo niczego z GLAD, GLFW ani `core/`, a z `gfx/` i `assets/` tylko
   nagłówki samych danych, które nie dołączają GLAD. Dziś korzystają z tego dwa miejsca:
   `Terrain.*` (`gfx/Vertex.hpp`, `assets/ImageLoader.hpp`, `assets/Tangents.hpp`) i, od
   pierwszej części M7, `Lighting.cpp` (`gfx/ColorSpace.hpp`). Klasa
   `gfx`, która posiada obiekt OpenGL (`Mesh`, `Texture2D`, `Shader`), w `game_logic` się
   nie zlinkuje w teście bez kontekstu, więc nie ma tam czego szukać.

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
   fragmentów. Program z shaderem geometrii ma trzeci plik o tej samej nazwie i rozszerzeniu
   `.geom` (wzór: `grass.vert`, `grass.geom`, `grass.frag`). Edytor rozpoznaje je jako GLSL
   (sekcja 3.10).
2. **Pierwsza linia shadera to `#version 410 core`.** Pod nią komentarz z jednym zdaniem
   opisu i odnośnikiem `See docs/modules/...`, tak jak w plikach C++. Kod wspólny dla kilku
   shaderów idzie do pliku `.glsl` w `assets/shaders/common/`, **bez** linii `#version`, a
   shader dołącza go linią `#include "common/nazwa.glsl"`, która musi stać za linią
   `#version` (nazwa liczy się od katalogu pliku shadera). Wzór: `common/lighting.glsl`,
   dołączany przez `lit.frag`, `gouraud.vert` i, od M6, `grass.frag`
   ([`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md)). Shader z
   podkatalogu, na przykład `assets/shaders/post/`, dołącza wspólny plik ścieżką
   `../common/nazwa.glsl` (wzór: `post/composite.frag` i `common/color.glsl`). Shader
   fragmentów sceny zapisuje od pierwszej części M7 kolor **liniowy** do bufora HDR: kolor
   wpisany w shaderze jako liczby dobrane na oko trzeba przeliczyć funkcją `srgbToLinear`
   z `common/color.glsl` ([`../modules/gfx/color-space.md`](../modules/gfx/color-space.md)).
3. **Niczego nie dopisuj w `CMakeLists.txt`.** Krok z bloku 7 obejmuje cały katalog `assets/`.
4. **W kodzie buduj ścieżkę przez `core::assetPath`**, z nazwą względną wobec `assets/`, na
   przykład `core::assetPath("shaders/textured.vert")`. Wzór: stałe
   `TEXTURED_VERTEX_SHADER_FILE` i
   `TEXTURED_FRAGMENT_SHADER_FILE` w `src/game/NightMazeApp.cpp` (nowa para shaderów to tam nowa para
   stałych, nowe pole `gfx::Shader`, akcesor, pole w `debug::DebugContext` i wpis w tablicy
   programów w `DebugUI::draw` razem z większą stałą `SHADER_COUNT`, żeby panel Shaders ją
   przeładowywał. Program z shaderem geometrii podaje jego ścieżkę jako **trzeci** argument
   konstruktora `gfx::Shader`, wzór: `m_grassShader`), a dla modeli stałe
   `WALL_MODEL_FILE` i `PILLAR_MODEL_FILE` w
   `src/game/MazeRenderer.cpp` oraz `CRYSTAL_A_MODEL_FILE`, `CRYSTAL_B_MODEL_FILE` i
   `GATE_MODEL_FILE` w `src/game/GameplayRenderer.cpp`. Model i jego tekstury wczytuje
   `assets::AssetCache`, a rysuje `game::drawModel`. Tekstura bez modelu (wzór: grunt terenu,
   stałe `GROUND_TEXTURE_FILE` i `GROUND_NORMAL_MAP_FILE` w `src/game/TerrainRenderer.cpp`)
   idzie przez `AssetCache::texture`, które od pierwszej części M7 wymaga drugiego argumentu: `gfx::ColorSpace::Srgb` dla obrazu koloru, `gfx::ColorSpace::Linear` dla mapy normalnych i innych danych. Obraz, który nie ma być teksturą (wzór: mapa
   wysokości, `HEIGHTMAP_FILE` w `NightMazeApp.cpp`), wprost przez `assets::loadImage`.
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
| nowy moduł lub klasa w istniejącym module | `docs/modules/<moduł>.md` (szablon 10 sekcji z PRD, sekcja 7). Duży moduł ma katalog `docs/modules/<moduł>/` z plikiem `README.md` (wstęp i indeks) i dokumentami tematycznymi, z których każdy ma pełne 10 sekcji. Wzór: `docs/modules/core/`, `docs/modules/gfx/`, `docs/modules/scene/`, `docs/modules/game/` i `docs/modules/renderer/` |
| nowa biblioteka | `docs/libraries/<biblioteka>.md` |
| zmiana w budowaniu, narzędziach lub strukturze | `docs/guides/` (ten plik, `build-macos.md`, `build-windows.md`) |
| decyzja "dlaczego tak, a nie inaczej" | `docs/decisions/<temat>.md`, według układu z [`../decisions/README.md`](../decisions/README.md), plus wiersz na liście notatek w tym pliku |

Po dodaniu pliku, katalogu albo targetu trzeba też zaktualizować drzewo i tabele w tym
dokumencie.
