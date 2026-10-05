# stb_image 2.30

Dokument biblioteki dla kamienia milowego M2 + M3. Opisuje konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake), plik
[`external/stb/stb_image.c`](../../external/stb/stb_image.c) i tę część API, której używa
loader obrazów w [`src/assets/ImageLoader.cpp`](../../src/assets/ImageLoader.cpp).

**Stan na dziś: stb_image woła jeden plik projektu, `src/assets/ImageLoader.cpp`.** Funkcja
`assets::loadImage` zamienia nim plik PNG na tablicę bajtów
([`../modules/assets/images.md`](../modules/assets/images.md)). Gra jej jeszcze nie woła:
wołają ją testy jednostkowe (`tests/ImageLoaderTests.cpp`). Na Windowsie (MSVC 19.44,
2026-10-05) biblioteka kompiluje się bez ostrzeżeń i testy loadera przechodzą. **Na macOS
ten kod nie był jeszcze budowany.**

W dokumencie są dwa rodzaje bloków C++. Blok zaczynający się komentarzem
`// Przykład, nie kod projektu.` to **przykład użycia API**. Blok poprzedzony nazwą pliku to
kod skopiowany z repozytorium. Fragmenty CMake są prawdziwe i skopiowane z repozytorium.

## 1. Czym jest stb_image

Plik obrazu na dysku (PNG, JPEG) nie zawiera gotowych pikseli. Zawiera dane **skompresowane**:
PNG pakuje wiersze algorytmem deflate (tym samym co archiwum zip), JPEG zapisuje obraz jako
współczynniki częstotliwości. Karta graficzna tego nie rozumie: `glTexImage2D` chce zwykłej
tablicy bajtów, piksel po pikselu. Zamiana pliku na taką tablicę to **dekodowanie**
(decoding), a kod, który to robi, to dekoder.

stb_image to dekoder obrazów w jednym pliku nagłówkowym, napisany w C. Należy do zbioru
bibliotek "stb" (autor: Sean Barrett), z których każda jest jednym plikiem. Co daje:

- jedną funkcję, która z bajtów pliku robi tablicę pikseli i podaje szerokość, wysokość i
  liczbę kanałów,
- obsługę formatów JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC i PNM (lista z komentarza na
  początku nagłówka). Projekt używa PNG (tekstury) i PNM (obrazek testowy),
- brak zależności: tylko biblioteka standardowa C.

**Składa się z jednego nagłówka z implementacją w środku (single header).** Plik
`stb_image.h` ma 7988 linii. Zwykłe dołączenie daje same deklaracje funkcji. Gdy przed
dołączeniem zdefiniowane jest makro `STB_IMAGE_IMPLEMENTATION`, ten sam plik dokłada
definicje wszystkich funkcji. To ten sam pomysł co w doctest
([`doctest.md`](doctest.md), sekcja 3.1), z jedną różnicą: tu implementacja jest w C i
trafia do osobnej, małej biblioteki (sekcja 2).

**Licencja.** Do wyboru: domena publiczna albo MIT (plik `LICENSE` w repozytorium stb).

### Za co stb_image NIE odpowiada

- Nie woła OpenGL i nic nie wie o teksturach. Zwraca bajty, a teksturę robi z nich
  `gfx::Texture2D` ([`../modules/gfx/textures.md`](../modules/gfx/textures.md)).
- Nie odwraca wierszy tak, jak chce OpenGL. Zwraca obraz od **górnego** wiersza. Ma do tego
  globalny przełącznik, którego nie używam (sekcja 3.4).
- Nie zapisuje obrazów. Do tego jest osobny plik `stb_image_write.h`, którego projekt nie
  używa.
- W konfiguracji projektu nie otwiera plików: makro `STBI_NO_STDIO` usuwa z niej wszystkie
  funkcje przyjmujące nazwę pliku (sekcja 2).
- Nie jest dekoderem "bezpiecznym dla dowolnych danych z sieci". Autor zaleca go do własnych
  plików gry, i tak go używam.

## 2. Jak podpinamy stb_image w CMake

### Pobranie i target

Cały fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
# ---- stb_image: decoding image files (PNG and others) into pixels ----------------------
# The stb repository has no release tags, so the version is pinned to a commit hash (the
# commit that holds stb_image 2.30). GIT_SHALLOW is left out on purpose: a shallow clone
# can only fetch a branch or a tag by name, not an arbitrary commit, so the whole history
# is downloaded once. stb has no CMake build, so only download it and define the target
# ourselves, like ImGui.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
)
FetchContent_MakeAvailable(stb)

# stb_image.h is a single header that also contains its implementation. The implementation
# is compiled exactly once, in external/stb/stb_image.c, into this small library. It is a
# target of its own so that it is built with the compiler's default warnings and not with
# the strict ones of our code (night_maze_enable_warnings is never called for it).
add_library(stb_image STATIC ${CMAKE_CURRENT_LIST_DIR}/../external/stb/stb_image.c)
# SYSTEM: the header must not produce warnings in the files of ours that include it.
target_include_directories(stb_image SYSTEM PUBLIC ${stb_SOURCE_DIR})
# STBI_NO_STDIO removes every function of stb_image that opens a file by name. Our loader
# reads the file itself and hands stb the bytes (see src/assets/ImageLoader.cpp). PUBLIC,
# because the header has to see the same definition in the implementation file and in the
# files that include it.
target_compile_definitions(stb_image PUBLIC STBI_NO_STDIO)
```

Mechanizm FetchContent (`FetchContent_Declare`, `FetchContent_MakeAvailable`, powód
przypinania wersji) jest opisany w [`glfw.md`](glfw.md), sekcja 2. Tutaj tylko to, co dla
stb jest inne.

| Element | Znaczenie |
|---|---|
| `GIT_TAG 2c980bb5...` | pełny skrót (hash) commita zamiast nazwy tagu. Repozytorium stb **nie ma tagów wydań**: każdy plik ma własny numer wersji w komentarzu na górze, a całość żyje na jednej gałęzi. Jedynym sposobem na "zawsze ta sama wersja" jest wskazanie commita. Ten commit zawiera `stb_image.h` w wersji 2.30 (pierwsza linia pliku: `stb_image - v2.30`) |
| brak `GIT_SHALLOW TRUE` | pozostałe biblioteki są klonowane płytko, czyli bez historii. Płytki klon umie pobrać tylko gałąź albo tag po nazwie, a nie dowolny commit, więc tu pobierana jest cała historia. Zmierzone na Windowsie: katalog `_deps/stb-src` zajmuje 12 MB |
| `FetchContent_MakeAvailable(stb)` | repozytorium stb nie ma pliku `CMakeLists.txt`, więc to wywołanie tylko pobiera pliki i ustawia zmienną `stb_SOURCE_DIR`. Targetu nie tworzy, tak jak przy ImGui ([`imgui.md`](imgui.md)) |
| `add_library(stb_image STATIC ...)` | nasz własny target: biblioteka statyczna z **jednego** pliku, `external/stb/stb_image.c` |
| `${CMAKE_CURRENT_LIST_DIR}/../external/stb/stb_image.c` | `CMAKE_CURRENT_LIST_DIR` to katalog pliku CMake, który jest właśnie czytany, czyli `cmake/`. Stąd `..`, żeby dojść do korzenia repozytorium |
| `target_include_directories(stb_image SYSTEM PUBLIC ${stb_SOURCE_DIR})` | katalog z pobranym `stb_image.h` trafia do ścieżek nagłówków. `PUBLIC`: potrzebuje go sama biblioteka i każdy, kto ją linkuje. `SYSTEM`: nagłówek jest traktowany jak systemowy, więc nie daje ostrzeżeń w naszym pliku, który go dołącza |
| `target_compile_definitions(stb_image PUBLIC STBI_NO_STDIO)` | makro widoczne przy kompilacji biblioteki **i** przy kompilacji plików, które ją linkują (opis niżej) |

**Dlaczego target jest w `Dependencies.cmake`, a nie w `external/stb/CMakeLists.txt`.**
GLAD ma własny `CMakeLists.txt` w `external/glad/`, bo cały jego kod leży w repozytorium.
Tu w repozytorium leży tylko jeden mały plik `.c`, a nagłówek jest pobierany. Target
potrzebuje zmiennej `stb_SOURCE_DIR`, która istnieje dopiero po
`FetchContent_MakeAvailable(stb)`. Najprościej zdefiniować go zaraz pod pobraniem, tak samo
jak target `imgui`.

### Plik `external/stb/stb_image.c`

Cały plik [`external/stb/stb_image.c`](../../external/stb/stb_image.c):

```c
/* The one translation unit that holds the implementation of stb_image.
   See docs/libraries/stb_image.md */

/* stb_image.h is a single header. Included normally it only declares its functions.
   With this macro defined first, the same header also emits their definitions. That must
   happen in exactly one source file of the program: this one. Every other file includes
   the header without the macro. */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

Dwie linie kodu. Trzy rzeczy do wyjaśnienia:

- **Dokładnie jeden plik.** Gdyby makro było w dwóch plikach `.cpp`, linker dostałby dwie
  definicje każdej funkcji (`stbi_load_from_memory already defined`). Gdyby nie było go
  nigdzie, linker nie znalazłby żadnej (`unresolved external symbol`).
- **Osobny plik i osobny target, a nie makro w `ImageLoader.cpp`.** Tak też by działało, ale
  wtedy 7988 linii cudzego kodu C kompilowałoby się jako część naszego pliku, pod naszymi
  ścisłymi ostrzeżeniami (`/W4`, `-Wall -Wextra -Wpedantic`). Oznaczenie nagłówka jako
  systemowy ucisza ostrzeżenia w zwykłym użyciu, ale nie chcę na tym polegać dla całej
  implementacji. W osobnym targecie cudzy kod kompiluje się z domyślnymi ustawieniami
  kompilatora, a nasz plik widzi z nagłówka tylko deklaracje.
- **Plik `.c`, nie `.cpp`.** stb_image jest napisane w C i kompiluje się jako C. Projekt ma
  język C włączony już dla GLAD (`project(NightMaze ... LANGUAGES C CXX)`). Komentarze mają
  postać `/* */`, bo to plik C. Nagłówek sam dba o `extern "C"`, gdy dołącza go plik C++,
  więc nazwy funkcji zgadzają się przy linkowaniu.

Obok leży [`external/stb/README.md`](../../external/stb/README.md): krótka notatka, skąd
pochodzi nagłówek i jak zmienić wersję.

### Makro `STBI_NO_STDIO`

stb_image ma dwie rodziny funkcji: `stbi_load(nazwa_pliku, ...)`, która sama otwiera plik, i
`stbi_load_from_memory(bajty, długość, ...)`, która dekoduje dane już wczytane do pamięci.
`STBI_NO_STDIO` usuwa pierwszą rodzinę z deklaracji i z implementacji.

Powód to nazwy plików na Windowsie. Fragment `stb_image.h` (funkcja `stbi__fopen`):

```c
#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
   ...
	if (0 != _wfopen_s(&f, wFilename, wMode))
   ...
#elif defined(_MSC_VER) && _MSC_VER >= 1400
   if (0 != fopen_s(&f, filename, mode))
```

Domyślnie stb otwiera plik funkcją `fopen_s`, która przyjmuje nazwę w **lokalnej stronie
kodowej** Windowsa. Ścieżka z literą spoza tej strony (na przykład katalog użytkownika z
polskimi znakami na systemie z inną stroną kodową) nie da się wtedy zapisać i plik się nie
otworzy. Biblioteka ma na to opcję `STBI_WINDOWS_UTF8` (nazwa w UTF-8, zamieniana na znaki
szerokie w buforze o stałej długości 1024). Wybrałem prostsze wyjście: plik otwiera nasz kod
przez `std::ifstream` z obiektem `std::filesystem::path`, który na Windowsie trzyma znaki
szerokie ([`../modules/core/paths.md`](../modules/core/paths.md), sekcja 2.6), a stb dostaje
gotowe bajty. Skoro funkcji z nazwą pliku nikt nie woła, makro usuwa je całkiem, żeby nie
dało się ich użyć przez pomyłkę.

**Dlaczego `PUBLIC`.** Makro zmienia zawartość nagłówka. Musi mieć tę samą wartość tam, gdzie
kompiluje się implementacja (`stb_image.c`), i tam, gdzie nagłówek jest dołączany
(`ImageLoader.cpp`). Inaczej plik widziałby deklarację funkcji, której w bibliotece nie ma.
`PUBLIC` na targecie załatwia oba miejsca jedną linią. Zmierzone na Windowsie w
`compile_commands.json`: `-DSTBI_NO_STDIO` jest w poleceniu kompilacji obu plików.

### Podłączenie do `engine`

W głównym [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# stb_image is PRIVATE: only assets/ImageLoader.cpp includes its header, no header of
# engine does, so the targets that use engine do not need its include path.
target_link_libraries(engine PRIVATE stb_image)
```

`PRIVATE`, inaczej niż GLAD, GLFW i GLM. Tamte są `PUBLIC`, bo nagłówki `engine` pokazują
ich typy. `assets/ImageLoader.hpp` nie pokazuje niczego ze stb: zwraca własną strukturę
`assets::Image`. Gra i testy nie potrzebują więc ścieżki do `stb_image.h` i nie mogą go
dołączyć. Sama biblioteka i tak trafia do programu: przy linkowaniu biblioteki statycznej
CMake przekazuje jej prywatne zależności dalej, do programu, który ją linkuje.

### Nagłówek jako systemowy

Zmierzone na Windowsie (generator Ninja, `compile_commands.json`): katalog `_deps/stb-src`
trafia do kompilacji `ImageLoader.cpp` jako `-external:I...` razem z `-external:W0`, czyli
ostrzeżenia z tego nagłówka są wyłączone, a nasz plik kompiluje się pod `/W4 /permissive-`
bez ostrzeżeń. Plik `stb_image.c` kompiluje się **bez** `/W4` (w poleceniu nie ma żadnej
flagi poziomu ostrzeżeń) i też nie daje ostrzeżeń. Na macOS oczekuję flagi `-isystem`, tak
jak dla GLFW i GLM. Tego nie mierzyłem.

## 3. Najważniejsze API

Projekt woła trzy funkcje. Wszystkie są w
[`src/assets/ImageLoader.cpp`](../../src/assets/ImageLoader.cpp), a cały plik linia po linii
omawia [`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.

### 3.1. `stbi_load_from_memory`

`src/assets/ImageLoader.cpp`:

```cpp
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(fileBytes.data(), static_cast<int>(fileBytes.size()),
                                             &width, &height, &channels, KEEP_FILE_CHANNELS);
```

| Parametr | Znaczenie |
|---|---|
| `fileBytes.data()` | wskaźnik na bajty **pliku** (skompresowane), typ `const stbi_uc*` |
| `static_cast<int>(fileBytes.size())` | liczba tych bajtów. Funkcja chce `int`, stąd rzutowanie i wcześniejsze sprawdzenie, że rozmiar mieści się w `int` |
| `&width`, `&height` | tu funkcja wpisuje rozmiar obrazu w pikselach |
| `&channels` | tu funkcja wpisuje liczbę kanałów **w pliku**: 1 (szarość), 2 (szarość i alfa), 3 (RGB), 4 (RGBA) |
| `KEEP_FILE_CHANNELS` (0) | żądana liczba kanałów wyniku. 0 znaczy "tyle, ile ma plik". Wartość od 1 do 4 kazałaby bibliotece przeliczyć piksele |

`stbi_uc` to `unsigned char`: jeden bajt, wartości od 0 do 255.

Wynik to wskaźnik na blok `width * height * channels` bajtów przydzielony przez bibliotekę
(funkcją `malloc`) albo `nullptr`, gdy danych nie da się zdekodować. Piksele leżą wierszami,
**górny wiersz pierwszy**, kanały jednego piksela obok siebie (R, G, B, potem następny
piksel), bez dopełnienia między wierszami. PNG z 16 bitami na kanał jest przy tym zamieniany
na 8 bitów.

### 3.2. `stbi_failure_reason`

```cpp
        return fail(error, "Image file cannot be decoded: " + core::pathText(path) + " (" +
                               stbi_failure_reason() + ")");
```

Zwraca krótki tekst w C (`const char*`) z powodem ostatniego nieudanego dekodowania.
Zmierzone: dla pliku tekstowego podanego jako obraz jest to `unknown image type`. Tekst
należy do biblioteki, nie zwalnia się go.

### 3.3. `stbi_image_free`

```cpp
    stbi_image_free(decoded);
```

Zwalnia blok zwrócony przez `stbi_load_from_memory`. Blok przydzieliła biblioteka, więc
biblioteka ma go zwolnić: `delete[]` byłoby tu błędem, bo pamięć nie pochodzi z `new[]`.
Loader woła tę funkcję dopiero po skopiowaniu pikseli do `std::vector`.

### 3.4. Czego nie używam

| Element | Do czego służy | Dlaczego go nie ma |
|---|---|---|
| `stbi_load(nazwa, ...)` | dekodowanie prosto z pliku | nazwy plików na Windowsie (sekcja 2). Usunięte przez `STBI_NO_STDIO` |
| `stbi_set_flip_vertically_on_load(1)` | każe bibliotece odwracać wiersze | to **ukryty stan globalny**: ustawienie zmienia wynik każdego następnego wywołania w całym programie. Odwracanie robi jawna pętla w loaderze, którą widać i którą umiem wyjaśnić ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.5) |
| `stbi_info_from_memory` | rozmiar obrazu bez dekodowania | niepotrzebne, dekoduję od razu |
| `stbi_loadf`, `stbi_load_16` | wynik jako `float` (HDR) albo 16 bitów na kanał | tekstury gry mają 8 bitów na kanał |
| `STBI_ONLY_PNG` i podobne | wyłączają dekodery innych formatów | test odwracania wierszy zapisuje obrazek PNM, więc ten dekoder musi zostać |
| `STBI_WINDOWS_UTF8` | nazwy plików w UTF-8 na Windowsie | plik otwiera nasz kod (sekcja 2) |

## 4. Pułapki

1. **`STB_IMAGE_IMPLEMENTATION` w drugim pliku.** Daje podwójne definicje przy linkowaniu.
   Makro jest w `external/stb/stb_image.c` i tylko tam. `ImageLoader.cpp` dołącza nagłówek
   bez niego.
2. **Zwolnienie bloku przez `delete[]` albo `free` zamiast `stbi_image_free`.** Pamięć trzeba
   oddać tą samą drogą, którą przyszła. Brak zwolnienia to wyciek całego obrazu: 786432
   bajty dla każdej z naszych tekstur.
3. **Wiersze od góry.** Obraz ze stb wysłany prosto do OpenGL jest do góry nogami względem
   współrzędnych UV modelu. Loader odwraca wiersze sam
   ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 2.3).
4. **`stbi_set_flip_vertically_on_load` wołane "gdzieś indziej".** Gdyby jakiś przyszły kod
   włączył ten przełącznik, loader odwróciłby obraz po raz drugi i tekstury znów stałyby na
   głowie. W projekcie nikt go nie woła i nie powinien.
5. **Liczba kanałów pliku a liczba kanałów, której chce odbiorca.** Loader zostawia tyle
   kanałów, ile ma plik. PNG w odcieniach szarości da 1 albo 2 kanały, a `gfx::Texture2D`
   przyjmuje 3 albo 4 i odmówi. Tekstury gry są zapisywane jako RGB
   ([`../guides/blender.md`](../guides/blender.md), sekcja 7).
6. **Makro konfiguracji tylko w jednym miejscu.** `STBI_NO_STDIO` zdefiniowane tylko przy
   implementacji albo tylko przy użyciu daje niezgodne deklaracje. Dlatego jest ustawione na
   targecie jako `PUBLIC`, a nie przez `#define` w pliku.
7. **Zmiana wersji.** Wersję zmienia się przez podmianę skrótu commita w
   `Dependencies.cmake`. Gałąź (`GIT_TAG master`) dawałaby inny kod przy każdej świeżej
   konfiguracji.
8. **Rozmiar w `int`.** API stb liczy rozmiary w `int`. Loader sprawdza, czy plik mieści się
   w tym typie, a rozmiar tablicy pikseli liczy już w `std::size_t`.

## 5. Pytania kontrolne

1. **Po co w ogóle biblioteka do obrazów?**
   Plik PNG zawiera dane skompresowane, a OpenGL chce zwykłej tablicy bajtów. stb_image
   dekoduje plik: zamienia jego bajty na piksele i podaje rozmiar oraz liczbę kanałów.

2. **Co znaczy "single header" i gdzie jest implementacja stb_image w projekcie?**
   Cała biblioteka to jeden plik `stb_image.h`. Dołączony zwyczajnie daje deklaracje, a z
   makrem `STB_IMAGE_IMPLEMENTATION` także definicje. Makro jest w jednym pliku,
   `external/stb/stb_image.c`, który tworzy bibliotekę statyczną `stb_image`.

3. **Dlaczego implementacja jest w osobnym targecie?**
   Żeby cudzy kod kompilował się z domyślnymi ostrzeżeniami kompilatora, a nie z naszymi
   ścisłymi, i żeby skompilował się dokładnie raz.

4. **Dlaczego wersja jest przypięta do commita, a nie do tagu, i dlaczego bez
   `GIT_SHALLOW`?**
   Repozytorium stb nie ma tagów wydań. Płytki klon umie pobrać tylko gałąź albo tag po
   nazwie, więc dla commita pobierana jest cała historia.

5. **Co robi `STBI_NO_STDIO` i dlaczego jest `PUBLIC`?**
   Usuwa funkcje otwierające plik po nazwie. Plik otwiera nasz kod, bo `std::ifstream` z
   `std::filesystem::path` radzi sobie na Windowsie z każdą nazwą, a domyślne `fopen_s` w
   stb zależy od strony kodowej. `PUBLIC`, bo makro zmienia nagłówek i musi być takie samo
   przy implementacji i przy użyciu.

6. **Co zwraca `stbi_load_from_memory` i kto zwalnia wynik?**
   Wskaźnik na blok `szerokość * wysokość * kanały` bajtów, górny wiersz pierwszy, albo
   `nullptr` przy błędzie. Blok zwalnia `stbi_image_free`.

7. **Co znaczy ostatni argument równy 0?**
   "Zostaw liczbę kanałów pliku". Wartość od 1 do 4 wymusiłaby przeliczenie.

8. **Dlaczego `engine` linkuje `stb_image` jako `PRIVATE`?**
   Żaden nagłówek `engine` nie dołącza `stb_image.h`. Używa go tylko `ImageLoader.cpp`, więc
   inne targety nie potrzebują jego ścieżki nagłówków.

## 6. Oficjalna dokumentacja

- Repozytorium stb: <https://github.com/nothings/stb>
- Dokumentacją stb_image jest komentarz na początku samego nagłówka:
  <https://github.com/nothings/stb/blob/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_image.h>
  (lista formatów, opis `stbi_load`, makra konfiguracji, uwagi o nazwach plików na
  Windowsie przy `STBI_WINDOWS_UTF8`).
- Dlaczego biblioteki stb są jednym plikiem:
  <https://github.com/nothings/stb/blob/2c980bb59875b0d32144a71867fbdebb2f77cd20/docs/stb_howto.txt>
- Dokumentacja CMake, `FetchContent` i ograniczenie `GIT_SHALLOW` (opis `GIT_TAG` i
  `GIT_SHALLOW` w module `ExternalProject`):
  <https://cmake.org/cmake/help/latest/module/ExternalProject.html>
- Kopia dokładnie dla naszej wersji leży po pierwszej konfiguracji w
  `build/debug/_deps/stb-src/`: plik `stb_image.h` i plik `LICENSE`.
