# Night Maze: dokumentacja

Dokumentacja jest materiałem do nauki: z samej lektury ma się dać nauczyć danego tematu wykładu, przygotować do kartkówki i do obrony, na której tłumaczę każdą linię kodu. Każdy moduł jest skończony dopiero wtedy, gdy ma tutaj swój dokument (PRD, sekcja 7).

Stan: **kamień milowy M0** zrobiony (repozytorium, CMake, okno GLFW z OpenGL 4.1 Core, GLAD, ImGui, `GL_CHECK`), **kamień milowy M1**: kod jest kompletny (mysz, ścieżki do assetów, GLM, klasy `gfx::Shader`, `gfx::Buffer` i `gfx::VertexArray`, katalog `assets/` z pierwszą parą shaderów, panel Shaders z przyciskiem "Reload shaders", struktury `scene::Transform` i `scene::Camera`, kostka rysowana z macierzami modelu, widoku i rzutowania, latająca kamera sterowana myszą i klawiaturą, panel Camera). Kod jest zbudowany i uruchomiony na macOS i na Windowsie (2026-10-05, MSVC, Debug i Release, bez ostrzeżeń), a automatyczna część listy kontrolnej z [`guides/build-windows.md`](guides/build-windows.md) (sekcja 11) jest zaliczona. M1 nie jest jeszcze zamknięty: sterowanie kamerą zostało sprawdzone ręcznie na macOS, ale na Windowsie ręczne sprawdzenie sterowania i paneli jest nadal otwarte, a tagu wersji nie ma. **Kamień milowy M2 + M3 jest zaczęty**: istnieje kod bez okna i jego testy jednostkowe, czyli pudełka kolizji AABB z ruchem ślizgającym się po ścianach (`scene::Aabb`, `scene::moveAndSlide`), labirynt, jego generator z ziarna i układ ścian w świecie (`game::Maze`, `game::generateMaze`, `game::wallSegments`) oraz program testowy `night_maze_tests` w bibliotece doctest. Na Windowsie build Debug i Release przechodzi bez ostrzeżeń, a 41 przypadków testowych przechodzi w obu konfiguracjach (2026-10-05). Na macOS ten kod nie był jeszcze budowany. Program go jeszcze nie używa: nadal rysuje kostkę, a gracza, rysowania labiryntu, loadera OBJ, tekstur i paneli Maze oraz Collision jeszcze nie ma. Kolejne dokumenty dochodzą razem z kodem kolejnych kamieni milowych, do M9. Plan tematów jest w [`syllabus.md`](syllabus.md).

## Spis treści

### Plan i mapa

| Dokument | Co zawiera |
|---|---|
| [`PRD.pdf`](PRD.pdf) | Założenia projektu: zakres, mapowanie na 15 tematów wykładu, architektura, kamienie milowe |
| [`syllabus.md`](syllabus.md) | Tabela: temat wykładu, dokument, pliki kodu, przełącznik w ImGui. Ściąga przed kartkówką i obroną |

### Przewodniki (`guides/`)

| Dokument | Co zawiera |
|---|---|
| [`guides/project-structure.md`](guides/project-structure.md) | Struktura katalogów repozytorium oraz do czego służy każdy plik konfiguracyjny i generowany |
| [`guides/build-macos.md`](guides/build-macos.md) | Budowanie i uruchamianie na macOS |
| [`guides/build-windows.md`](guides/build-windows.md) | Budowanie i uruchamianie na Windowsie |

### Moduły (`modules/`)

Każdy dokument modułu ma te same dziesięć sekcji: Po co to jest, Teoria, Jak to działa w OpenGL, Shadery, Kod w projekcie, Panel ImGui, Pułapki, Ćwiczenia, Pytania kontrolne, Źródła. Duży moduł jest katalogiem: plik `README.md` jest wstępem i indeksem, a każdy dokument tematyczny w katalogu ma pełny zestaw dziesięciu sekcji.

| Dokument | Kod | Temat wykładu |
|---|---|---|
| [`modules/core/README.md`](modules/core/README.md) | `src/core/`, `src/game/NightMazeApp.*`, `src/main.cpp` | 1. Pierwszy program OpenGL: wstęp do modułu `core`, diagram warstw i klas, klatka jako całość, dziedziczenie po `core::Application`, indeks plików |
| [`modules/core/window-context.md`](modules/core/window-context.md) | `src/core/Window.*`, `src/core/Log.*` | 1. Okno GLFW, kontekst 4.1 Core, GLAD, vsync, rozmiar okna a framebuffera, logowanie |
| [`modules/core/main-loop.md`](modules/core/main-loop.md) | `src/core/Application.*`, `src/core/Time.*` | 1. Pętla gry ze stałym krokiem, akumulator, `alpha` i interpolacja (pierwszy użytkownik: ruch kamery), uśredniony FPS |
| [`modules/core/input.md`](modules/core/input.md) | `src/core/Input.*` | 1. Klawiatura i mysz: odpytywanie, zbocze na klatkę, przesunięcie myszy, przechwycenie kursora, blokada klawiatury i myszy na czas pracy z panelem, przechwycony kursor a ImGui |
| [`modules/core/gl-check.md`](modules/core/gl-check.md) | `src/core/GlCheck.*` | 1. Makro `GL_CHECK`, model błędów `glGetError`, Debug a Release |
| [`modules/core/paths.md`](modules/core/paths.md) | `src/core/Paths.*` | 1. Ścieżki do assetów względem pliku wykonywalnego: katalog roboczy, wywołania systemowe macOS i Windows, `std::filesystem::path`, katalog `assets` obok programu, ścieżka jako tekst UTF-8 (`pathText`). Użytkownik: wczytywanie shaderów w `NightMazeApp` |
| [`modules/gfx/README.md`](modules/gfx/README.md) | `src/gfx/` | 2. Programowalny potok: wstęp do modułu `gfx`, RAII i semantyka przenoszenia dla obiektów OpenGL, miejsce w warstwach, indeks plików, droga jednej klatki od danych do pikseli |
| [`modules/gfx/shaders.md`](modules/gfx/shaders.md) | `assets/shaders/basic.*`, `src/game/NightMazeApp.*` | 2. Potok renderowania, shader wierzchołków i fragmentów, podstawy GLSL, obiekt shadera a obiekt programu, kompilacja a linkowanie jako pojęcia, shadery `assets/shaders/basic.*` linia po linii, skąd biorą się kolory, użycie programu w `NightMazeApp` |
| [`modules/gfx/shader-class.md`](modules/gfx/shader-class.md) | `src/gfx/Shader.*` | 2. Klasa `gfx::Shader` linia po linii: wywołania OpenGL w kolejności, błędy sterownika i dziennik, wczytanie pliku, kompilacja, linkowanie, konstruktor i destruktor, przenoszenie, testy klasy |
| [`modules/gfx/uniforms.md`](modules/gfx/uniforms.md) | `src/gfx/Shader.*` (`setMat4`), `assets/shaders/basic.vert`, `src/game/NightMazeApp.*` | 2. Uniformy: atrybut a uniform, `glGetUniformLocation` i `glUniformMatrix4fv`, `Shader::setMat4` linia po linii, trzy macierze w `basic.vert`, położenie -1, `use()` przed `setMat4` |
| [`modules/gfx/shader-hot-reload.md`](modules/gfx/shader-hot-reload.md) | `src/gfx/Shader.*` (`reload`), `src/debug/panels/ShadersPanel.*` | 2. Wczytywanie shaderów na żywo z zachowaniem starego programu, przeładowanie w środku klatki ImGui, panel Shaders z przyciskiem "Reload shaders", scenariusz pokazu na obronie, różnica na Windowsie |
| [`modules/gfx/buffers-vao.md`](modules/gfx/buffers-vao.md) | `src/gfx/Buffer.*`, `src/gfx/VertexArray.*` | 2. Dane wierzchołków i atrybuty, VBO, VAO i jego stan, układ przeplatany (krok i przesunięcie), dlaczego profil Core wymaga VAO, podpowiedź użycia, klasy `Buffer` i `VertexArray` linia po linii, `setFloatAttribute`, czas życia bufora i VAO |
| [`modules/gfx/indexed-drawing.md`](modules/gfx/indexed-drawing.md) | `src/game/NightMazeApp.*` | 2. Indeksy i bufor indeksów (EBO), `glDrawArrays` a `glDrawElements`, współrzędne lokalne i kierunek nawijania, dane kostki (24 wierzchołki, 36 indeksów), kolejność tworzenia obiektów i rysowanie w `NightMazeApp` |
| [`modules/scene/README.md`](modules/scene/README.md) | `src/scene/` | 3. Przekształcenia przestrzeni: wstęp do modułu `scene`, czym struktury z danymi różnią się od klas `gfx`, miejsce w warstwach, konwencja układu współrzędnych, indeks plików |
| [`modules/scene/transforms.md`](modules/scene/transforms.md) | `src/scene/Transform.*`, `assets/shaders/basic.vert`, `src/game/NightMazeApp.*` | 3. Przestrzenie współrzędnych, współrzędne jednorodne, macierze przesunięcia, obrotu i skali, kolejność przekształceń, kąty Eulera, macierz modelu, struktura `Transform` linia po linii, trzy macierze w shaderze, obrót kostki |
| [`modules/scene/camera.md`](modules/scene/camera.md) | `src/scene/Camera.*`, `src/game/NightMazeApp.*` | 3. Macierz widoku i `lookAt`, kamera FPS (yaw, pitch, wektor kierunku, ograniczenie pitch), rzutowanie perspektywiczne i nieliniowa głębia, z NDC do pikseli, konwencja układu, struktura `Camera` linia po linii, trzy macierze w `onRender`, test głębi, proporcje okna, droga jednego wierzchołka kostki na liczbach |
| [`modules/scene/camera-controls.md`](modules/scene/camera-controls.md) | `src/game/NightMazeApp.*`, `src/debug/panels/CameraPanel.*` | 3. Sterowanie kamerą (obrót myszą raz na klatkę, ruch klawiszami stałym krokiem, normalizacja kierunku, interpolacja z `alpha`), panel Camera i scenariusz pokazu, panel a przechwycony kursor |
| [`modules/scene/collision.md`](modules/scene/collision.md) | `src/scene/Collider.*`, `tests/ColliderTests.cpp` | 14. Wstęp do kolizji: bryły otaczające i AABB, test nakładania przedziałów na osiach, wykrywanie dyskretne a przemiatanie, tunelowanie, ruch oś po osi i ślizganie, tolerancja styku, droga po schodkach a stały krok, kula jako następny krok, `Aabb`, `overlaps` i `moveAndSlide` linia po linii, testy. Gracz i panel Collision jeszcze nie istnieją |
| [`modules/game/README.md`](modules/game/README.md) | `src/game/`, `tests/` | Logika gry (poza tematami wykładu): wstęp do modułu `game`, podział na aplikację i logikę bez okna, biblioteka `game_logic` i program testowy, miejsce w warstwach, indeks plików |
| [`modules/game/maze-generator.md`](modules/game/maze-generator.md) | `src/game/Maze.*`, `src/game/MazeGenerator.*`, `src/game/MazeLayout.*`, `tests/Maze*.cpp` | Logika gry: siatka komórek ze ścianami na krawędziach, labirynt doskonały, recursive backtracker krok po kroku z przykładem, wersja iteracyjna, liczby losowe takie same na każdym systemie (`std::mt19937`, `randomBelow`, błąd reszty z dzielenia), układ w świecie (komórki, ściany, słupki, pudełka kolizji), labirynt wzorcowy w testach |
| [`modules/debug-ui.md`](modules/debug-ui.md) | `src/debug/`, `src/main.cpp` | Narzędzie do wszystkich tematów: architektura paneli ImGui (Renderer, Shaders, Camera), podpięcie nakładki w `main.cpp`, kto ma klawiaturę i mysz (blokady, `setMouseEnabled`), jak dodać nowy panel |

### Biblioteki (`libraries/`)

| Dokument | Biblioteka | Rola w projekcie |
|---|---|---|
| [`libraries/glfw.md`](libraries/glfw.md) | GLFW | Okno, kontekst OpenGL, wejście |
| [`libraries/glad.md`](libraries/glad.md) | GLAD | Ładowanie funkcji OpenGL 4.1 Core |
| [`libraries/glm.md`](libraries/glm.md) | GLM | Matematyka: wektory, macierze, przekształcenia. Używają jej `scene::Transform`, `scene::Camera`, `scene::Aabb` i układ labiryntu w `game/MazeLayout` |
| [`libraries/imgui.md`](libraries/imgui.md) | Dear ImGui (gałąź docking) | Panele debug |
| [`libraries/doctest.md`](libraries/doctest.md) | doctest | Testy jednostkowe kodu bez okna: kolizji i labiryntu. Program `night_maze_tests`, uruchamiany przez `ctest` |

### Decyzje (`decisions/`)

Krótkie notatki "dlaczego tak, a nie inaczej". Czym jest notatka i jak ją napisać: [`decisions/README.md`](decisions/README.md).

| Dokument | Decyzja |
|---|---|
| [`decisions/README.md`](decisions/README.md) | Wstęp: czym notatka o decyzji różni się od dokumentu modułu, układ notatki, lista notatek |
| [`decisions/collision-aabb-sliding.md`](decisions/collision-aabb-sliding.md) | Kolizje jako własne pudełka AABB i ruch oś po osi ze ślizganiem, bez silnika fizyki |
| [`decisions/deterministic-random.md`](decisions/deterministic-random.md) | Losowość z `std::mt19937` i własnej funkcji `randomBelow`, bez rozkładów z biblioteki standardowej |

## Kolejność czytania

Kolejność jest zgodna z kolejnością wykładów. Kroki od 1 do 14 to materiał do tematu 1, "Pierwszy program OpenGL", i narzędzia potrzebne przy następnych tematach. Kroki od 15 do 21 to temat 2, "Programowalny potok". Kroki od 22 do 25 to temat 3, "Przekształcenia przestrzeni". Kroki od 26 do 30 to początek kamienia milowego M2 + M3: testy jednostkowe, temat 14, "Wstęp do kolizji", i labirynt.

| Krok | Dokument | Po co na tym etapie |
|---|---|---|
| 1 | [`PRD.pdf`](PRD.pdf), sekcje 3, 4 i 6 | Co buduję, dlaczego OpenGL 4.1 Core i jak podzielony jest kod na warstwy |
| 2 | [`guides/project-structure.md`](guides/project-structure.md) | Orientacja w repozytorium, zanim otworzę kod |
| 3 | [`guides/build-macos.md`](guides/build-macos.md) albo [`guides/build-windows.md`](guides/build-windows.md) | Program musi się budować i uruchamiać, bo ćwiczenia polegają na zmienianiu kodu |
| 4 | [`libraries/glfw.md`](libraries/glfw.md) | Skąd się bierze okno i kontekst |
| 5 | [`libraries/glad.md`](libraries/glad.md) | Skąd się biorą funkcje `gl*` |
| 6 | [`modules/core/README.md`](modules/core/README.md) | Temat 1 z lotu ptaka: warstwy, klatka jako całość, od `main` do pierwszej klatki. Kroki od 6 do 11 to najważniejsza część M0 |
| 7 | [`modules/core/window-context.md`](modules/core/window-context.md) | Okno, kontekst, GLAD, vsync |
| 8 | [`modules/core/main-loop.md`](modules/core/main-loop.md) | Pętla główna i stały krok czasowy |
| 9 | [`modules/core/input.md`](modules/core/input.md) | Klawiatura, mysz i ich blokada |
| 10 | [`modules/core/gl-check.md`](modules/core/gl-check.md) | Wykrywanie błędów OpenGL |
| 11 | [`modules/core/paths.md`](modules/core/paths.md) | Jak program znajduje pliki z `assets/` niezależnie od katalogu roboczego i skąd ten katalog bierze się obok programu |
| 12 | [`libraries/imgui.md`](libraries/imgui.md) | Jak działa biblioteka paneli |
| 13 | [`modules/debug-ui.md`](modules/debug-ui.md) | Jak panele są wpięte w mój projekt i jak dodać własny |
| 14 | [`libraries/glm.md`](libraries/glm.md) | Wektory i macierze, zanim pojawią się shadery, przekształcenia i kamera. Fragmenty kodu projektu pochodzą z `src/scene/` (kroki od 22 do 25) |
| 15 | [`modules/gfx/README.md`](modules/gfx/README.md) | Temat 2: dlaczego obiekty OpenGL są zamknięte w klasach, RAII i przenoszenie zamiast kopiowania, droga klatki od danych do pikseli |
| 16 | [`modules/gfx/shaders.md`](modules/gfx/shaders.md) | Programowalny potok, GLSL, shadery `basic.vert` oraz `basic.frag`, użycie programu w klatce |
| 17 | [`modules/gfx/shader-class.md`](modules/gfx/shader-class.md) | Klasa `Shader`: jak pliki shaderów stają się programem OpenGL i jak odczytać błąd sterownika |
| 18 | [`modules/gfx/uniforms.md`](modules/gfx/uniforms.md) | Uniformy i `Shader::setMat4`: jak wartości z C++ trafiają do shadera |
| 19 | [`modules/gfx/shader-hot-reload.md`](modules/gfx/shader-hot-reload.md) | Przeładowanie shaderów na żywo i panel Shaders |
| 20 | [`modules/gfx/buffers-vao.md`](modules/gfx/buffers-vao.md) | Skąd shader wierzchołków bierze dane: bufory, atrybuty, VAO |
| 21 | [`modules/gfx/indexed-drawing.md`](modules/gfx/indexed-drawing.md) | Indeksy, dane kostki i `glDrawElements`. Po tym kroku wyjaśnione jest wszystko w kodzie rysującym poza macierzami |
| 22 | [`modules/scene/README.md`](modules/scene/README.md) | Temat 3: po co warstwa `scene`, dlaczego jej struktury to same dane i matematyka bez OpenGL, konwencja układu współrzędnych |
| 23 | [`modules/scene/transforms.md`](modules/scene/transforms.md) | Przestrzenie współrzędnych, macierze przesunięcia, obrotu i skali, macierz modelu, struktura `Transform` |
| 24 | [`modules/scene/camera.md`](modules/scene/camera.md) | Macierze view i projection, kamera FPS, struktura `Camera`. Po tym kroku cały kod rysujący kostkę jest wyjaśniony, razem z drogą wierzchołka od bufora do piksela |
| 25 | [`modules/scene/camera-controls.md`](modules/scene/camera-controls.md) | Sterowanie kamerą (sekcje 2 i 5, do których przydają się kroki 8, 9 i 13: pętla ze stałym krokiem, wejście i panele) i panel Camera |
| 26 | [`libraries/doctest.md`](libraries/doctest.md) | Jak czytać i uruchamiać testy jednostkowe, zanim pojawi się kod, który jest sprawdzany tylko nimi |
| 27 | [`modules/scene/collision.md`](modules/scene/collision.md) | Temat 14: pudełka AABB, test nakładania, ruch ze ślizganiem po ścianach. Przydaje się krok 8 (stały krok symulacji) |
| 28 | [`modules/game/README.md`](modules/game/README.md) | Moduł `game`: co jest logiką gry, dlaczego część bez okna jest osobną biblioteką |
| 29 | [`modules/game/maze-generator.md`](modules/game/maze-generator.md) | Labirynt: zapis, generator z ziarna, układ ścian i słupków w świecie, pudełka kolizji z kroku 27 |
| 30 | [`decisions/README.md`](decisions/README.md) i dwie notatki | Dlaczego własne AABB zamiast silnika fizyki i własna funkcja losująca zamiast rozkładów standardowych: pytania "dlaczego tak" na obronę |
| 31 | [`syllabus.md`](syllabus.md) | Powtórka: który plik realizuje który temat |

## Jak się uczyć z dokumentu modułu

1. Przeczytaj sekcje od 1 do 3 (po co, teoria, wywołania OpenGL) bez otwierania kodu.
2. Otwórz pliki z sekcji 5 obok dokumentu i przejdź kod linia po linii, porównując z opisem.
3. Uruchom program i sprawdź w panelu ImGui to, co opisuje sekcja 6. Dla kodu, który nie ma jeszcze panelu (kolizje, labirynt), uruchom zamiast tego testy ([`libraries/doctest.md`](libraries/doctest.md), sekcja 4).
4. Zrób ćwiczenia z sekcji 8. Każde to mała zmiana w kodzie, którą trzeba potem wycofać.
5. Zakryj odpowiedzi i odpowiedz na głos na pytania z sekcji 9. To jest próba obrony.
6. Sekcję 7 (pułapki) przeczytaj jeszcze raz przed kartkówką: to najczęstsze pytania "co by było, gdyby".

## Zasady pisania dokumentów

- Język polski, pojęcia techniczne z angielskim odpowiednikiem w nawiasie przy pierwszym użyciu, identyfikatory tak jak w kodzie.
- Każde stwierdzenie ma zgadzać się z aktualnym kodem. Dokument jest aktualizowany w tym samym commicie co kod.
- Każdy plik źródłowy zaczyna się komentarzem z odnośnikiem do swojego dokumentu (`// See docs/...`).
- Diagramy w Mermaid, bo renderują się na GitHubie.
