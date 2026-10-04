# Night Maze: dokumentacja

Dokumentacja jest materiałem do nauki: z samej lektury ma się dać nauczyć danego tematu wykładu, przygotować do kartkówki i do obrony, na której tłumaczę każdą linię kodu. Każdy moduł jest skończony dopiero wtedy, gdy ma tutaj swój dokument (PRD, sekcja 7).

Stan: **kamień milowy M0** zrobiony (repozytorium, CMake, okno GLFW z OpenGL 4.1 Core, GLAD, ImGui, `GL_CHECK`), **M1 w toku**: są już mysz, ścieżki do assetów, GLM i klasy `gfx::Shader`, `gfx::Buffer` i `gfx::VertexArray`, ale program jeszcze niczego nie rysuje. Kolejne dokumenty dochodzą razem z kodem kolejnych kamieni milowych, do M9. Plan tematów jest w [`syllabus.md`](syllabus.md).

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
| [`modules/core/main-loop.md`](modules/core/main-loop.md) | `src/core/Application.*`, `src/core/Time.*` | 1. Pętla gry ze stałym krokiem, akumulator, `alpha`, uśredniony FPS |
| [`modules/core/input.md`](modules/core/input.md) | `src/core/Input.*` | 1. Klawiatura i mysz: odpytywanie, zbocze na klatkę, przesunięcie myszy, przechwycenie kursora, blokada klawiatury i myszy na czas pracy z panelem |
| [`modules/core/gl-check.md`](modules/core/gl-check.md) | `src/core/GlCheck.*` | 1. Makro `GL_CHECK`, model błędów `glGetError`, Debug a Release |
| [`modules/core/paths.md`](modules/core/paths.md) | `src/core/Paths.*` | 1. Ścieżki do assetów względem pliku wykonywalnego: katalog roboczy, wywołania systemowe macOS i Windows, `std::filesystem::path`. Pierwszy użytkownik w M1 (shadery) |
| [`modules/gfx/README.md`](modules/gfx/README.md) | `src/gfx/` | 2. Programowalny potok: wstęp do modułu `gfx`, RAII i semantyka przenoszenia dla obiektów OpenGL, miejsce w warstwach, indeks plików |
| [`modules/gfx/shaders.md`](modules/gfx/shaders.md) | `src/gfx/Shader.*` | 2. Potok renderowania, shader wierzchołków i fragmentów, podstawy GLSL, kompilacja i linkowanie, błędy sterownika, wczytywanie na żywo. Klasa jeszcze bez użytkownika w programie |
| [`modules/gfx/buffers-vao.md`](modules/gfx/buffers-vao.md) | `src/gfx/Buffer.*`, `src/gfx/VertexArray.*` | 2. Dane wierzchołków, VBO, VAO i jego stan, układ przeplatany (krok i przesunięcie), EBO, `glDrawArrays` a `glDrawElements`. Klasy jeszcze bez użytkownika w programie |
| [`modules/debug-ui.md`](modules/debug-ui.md) | `src/debug/`, `src/main.cpp` | Narzędzie do wszystkich tematów: architektura paneli ImGui, podpięcie nakładki w `main.cpp`, jak dodać nowy panel |

### Biblioteki (`libraries/`)

| Dokument | Biblioteka | Rola w projekcie |
|---|---|---|
| [`libraries/glfw.md`](libraries/glfw.md) | GLFW | Okno, kontekst OpenGL, wejście |
| [`libraries/glad.md`](libraries/glad.md) | GLAD | Ładowanie funkcji OpenGL 4.1 Core |
| [`libraries/glm.md`](libraries/glm.md) | GLM | Matematyka: wektory, macierze, przekształcenia. Podpięta do buildu, pierwsze użycie w M1 (`Shader`, `Transform`, `Camera`) |
| [`libraries/imgui.md`](libraries/imgui.md) | Dear ImGui (gałąź docking) | Panele debug |

## Kolejność czytania

Kolejność jest zgodna z kolejnością wykładów. Kroki od 1 do 14 to materiał do tematu 1, "Pierwszy program OpenGL", i narzędzia potrzebne przy następnych tematach. Kroki od 15 do 17 to temat 2, "Programowalny potok".

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
| 11 | [`modules/core/paths.md`](modules/core/paths.md) | Jak program znajdzie pliki z `assets/` niezależnie od katalogu roboczego. Na dziś sam mechanizm, bez użytkownika w kodzie |
| 12 | [`libraries/imgui.md`](libraries/imgui.md) | Jak działa biblioteka paneli |
| 13 | [`modules/debug-ui.md`](modules/debug-ui.md) | Jak panele są wpięte w mój projekt i jak dodać własny |
| 14 | [`libraries/glm.md`](libraries/glm.md) | Wektory i macierze, zanim pojawią się shadery, przekształcenia i kamera (M1). Na dziś sama biblioteka, bez kodu w projekcie |
| 15 | [`modules/gfx/README.md`](modules/gfx/README.md) | Temat 2: dlaczego obiekty OpenGL są zamknięte w klasach, RAII i przenoszenie zamiast kopiowania |
| 16 | [`modules/gfx/shaders.md`](modules/gfx/shaders.md) | Programowalny potok, GLSL i klasa `Shader`. Na dziś sama klasa, bez plików shaderów i bez rysowania |
| 17 | [`modules/gfx/buffers-vao.md`](modules/gfx/buffers-vao.md) | Skąd shader wierzchołków bierze dane: bufory, atrybuty, VAO. Na dziś same klasy, bez rysowania |
| 18 | [`syllabus.md`](syllabus.md) | Powtórka: który plik realizuje który temat |

## Jak się uczyć z dokumentu modułu

1. Przeczytaj sekcje od 1 do 3 (po co, teoria, wywołania OpenGL) bez otwierania kodu.
2. Otwórz pliki z sekcji 5 obok dokumentu i przejdź kod linia po linii, porównując z opisem.
3. Uruchom program i sprawdź w panelu ImGui to, co opisuje sekcja 6.
4. Zrób ćwiczenia z sekcji 8. Każde to mała zmiana w kodzie, którą trzeba potem wycofać.
5. Zakryj odpowiedzi i odpowiedz na głos na pytania z sekcji 9. To jest próba obrony.
6. Sekcję 7 (pułapki) przeczytaj jeszcze raz przed kartkówką: to najczęstsze pytania "co by było, gdyby".

## Zasady pisania dokumentów

- Język polski, pojęcia techniczne z angielskim odpowiednikiem w nawiasie przy pierwszym użyciu, identyfikatory tak jak w kodzie.
- Każde stwierdzenie ma zgadzać się z aktualnym kodem. Dokument jest aktualizowany w tym samym commicie co kod.
- Każdy plik źródłowy zaczyna się komentarzem z odnośnikiem do swojego dokumentu (`// See docs/...`).
- Diagramy w Mermaid, bo renderują się na GitHubie.
