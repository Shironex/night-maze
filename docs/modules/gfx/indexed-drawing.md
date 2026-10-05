# Moduł gfx: rysowanie z indeksami

Kamień milowy: M1. W M5 przykłady zostały przeniesione na kod, który istnieje dziś. Temat wykładu: 2 (Programowalny potok).
Kod: [`src/gfx/Mesh.hpp`](../../../src/gfx/Mesh.hpp), [`src/gfx/Mesh.cpp`](../../../src/gfx/Mesh.cpp), ręcznie wpisane indeksy w [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp), rysowanie modeli w [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp), dane modeli w [`assets/models/floor_tile.obj`](../../../assets/models/floor_tile.obj) i [`assets/models/wall_straight.obj`](../../../assets/models/wall_straight.obj).

Część modułu `gfx`. Wstęp do całego modułu jest w [`README.md`](README.md). Ten dokument jest ciągiem dalszym [`buffers-vao.md`](buffers-vao.md) i zakłada jego znajomość: czym jest bufor, czym VAO, co to krok i przesunięcie oraz jak działają klasy `gfx::Buffer` i `gfx::VertexArray`. Shadery i sam potok opisuje [`shaders.md`](shaders.md). Podział z [`mesh.md`](mesh.md) jest taki: tutaj jest **pomysł** indeksów, dane indeksów, które da się przeczytać w repozytorium, i wywołania rysujące. Tam jest struktura wierzchołka `gfx::Vertex` i klasa `gfx::Mesh` linia po linii. Każde wywołanie OpenGL jest opakowane w `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)).

## 1. Po co to jest

Bufor i tablica wierzchołków z [`buffers-vao.md`](buffers-vao.md) to narzędzia. Tutaj opisuję, jak rysuję nimi bryły. Potrzebne są do tego trzy rzeczy, których tamten dokument tylko dotyka:

- **indeksy**: osobna tablica liczb całkowitych, która mówi, z których wierzchołków złożyć kolejne trójkąty (albo odcinki), oraz bufor indeksów, w którym ta tablica leży na karcie,
- **dane bryły**: pozycje w przestrzeni lokalnej i kolejność wierzchołków każdego trójkąta, czyli kierunek nawijania,
- **wywołanie rysujące** `glDrawElements` oraz kolejność, w jakiej powstają VAO i bufor indeksów, od której zależy, czy indeksy trafią do właściwego VAO.

**Stan na dziś (M5).** Wszystko w grze jest rysowane z indeksami, przez jedną funkcję: `gfx::Mesh::draw`. Nie ma w projekcie drugiego miejsca, które woła `glDrawElements`, a `glDrawArrays` nie jest wołane nigdzie. Dane indeksów mają trzy źródła:

| Źródło | Co to jest | Prymityw | Gdzie to przeczytać |
|---|---|---|---|
| plik OBJ | sześć modeli: płytka podłogi, ściana, słupek, dwa kryształy, brama. Indeksy układa loader z linii `f` pliku | `GL_TRIANGLES` | sekcja 5.2, [`../assets/obj-loader.md`](../assets/obj-loader.md) |
| tablica wpisana ręcznie | krawędzie sześcianu jednostkowego: 8 narożników, 24 indeksy | `GL_LINES` | sekcja 5.3 |
| pętla | okrąg jednostkowy: 32 punkty, 64 indeksy | `GL_LINES` | sekcja 5.4 |

W M1 tę rolę pełniła kostka wpisana ręcznie w `NightMazeApp.cpp` (24 wierzchołki z kolorami, 36 indeksów, własny VAO i dwa bufory jako pola aplikacji). W M5 została usunięta razem ze swoimi shaderami. Pomiary, które na niej zrobiłem, zostają w sekcji 5.7 jako historia, bo dotyczą reguł OpenGL, a nie samej kostki.

## 2. Teoria

### 2.1 Indeksy i bufor indeksów (EBO)

Prostokąt to dwa trójkąty, czyli 6 wierzchołków, ale tylko 4 różne rogi. Dwa rogi leżą na wspólnej przekątnej i w tablicy wierzchołków musiałyby wystąpić dwa razy. **Indeksy** rozwiązują ten problem: tablica wierzchołków zawiera każdy róg raz, a osobna tablica liczb całkowitych mówi, z których rogów złożyć kolejne trójkąty.

Takim prostokątem jest w grze płytka podłogi, `floor_tile.obj`. Wierzchołek projektu (`gfx::Vertex`) ma 44 bajty, a indeks 4 bajty:

| | Bez indeksów | Z indeksami (tak jest w kodzie) |
|---|---|---|
| płytka podłogi: 2 trójkąty | 6 wierzchołków: 264 bajty | 4 wierzchołki i 6 indeksów: 176 + 24 = 200 bajtów |
| ściana `wall_straight.obj`: 30 trójkątów | 90 wierzchołków: 3960 bajtów | 60 wierzchołków i 90 indeksów: 2640 + 360 = 3000 bajtów |

Zysk rośnie z rozmiarem wierzchołka i z tym, jak często rogi są wspólne. W gładkiej siatce modelu jeden wierzchołek należy zwykle do około sześciu trójkątów. Drugi zysk: shader wierzchołków może wykonać się raz dla wspólnego rogu, a nie raz na każde jego użycie.

Bufor z indeksami to **EBO** (element buffer object, spotyka się też nazwę IBO, index buffer object). Jest zwykłym buforem związanym z celem `GL_ELEMENT_ARRAY_BUFFER`. `gfx::Buffer` obsługuje oba cele tym samym kodem. W projekcie indeksy są typu `std::uint32_t`, czyli na obu platformach `GLuint` (4 bajty, w OpenGL stała `GL_UNSIGNED_INT`). Tę równość typów sprawdza `static_assert` w `Mesh.cpp` ([`mesh.md`](mesh.md), sekcja 5.4).

**Dlaczego ściana ma 60 wierzchołków, skoro w pliku są 24 pozycje.** Indeks wskazuje **cały wierzchołek**, czyli wszystkie jego atrybuty naraz: nie da się wziąć pozycji z jednego miejsca bufora, a normalnej z innego. Róg prostopadłościanu należy do trzech ścian, a każda ściana jest zwrócona w inną stronę, więc ten sam punkt w przestrzeni potrzebuje trzech różnych normalnych (i zwykle trzech różnych współrzędnych tekstury). Jest więc trzema różnymi wierzchołkami. Plik `wall_straight.obj` ma 24 linie `v` (pozycje), ale 60 różnych trójek pozycja/uv/normalna w liniach `f`, i tyle wierzchołków trafia do bufora. Gdyby róg miał jedną wspólną normalną, rasteryzacja rozmywałaby ją między ścianami i oświetlenie zaokrągliłoby kanty muru.

Indeksy nadal się opłacają, tylko mniej niż w gładkiej siatce: wspólne są głównie dwa rogi na przekątnej każdego prostokąta. Tak właśnie jest w płytce podłogi: 4 wierzchołki zamiast 6.

**Kiedy wystarcza wspólny wierzchołek.** Sześcian z krawędzi w `ColliderLines.cpp` ma 8 wierzchołków, po jednym na narożnik, choć każdy narożnik łączy trzy krawędzie. Linie pudełek kolizji są rysowane jednym kolorem z uniformu, bez normalnych i bez tekstury, więc wierzchołek to sama pozycja i nic nie przeszkadza w dzieleniu go między krawędzie:

| Sześcian z krawędzi | Wierzchołki | Indeksy | Razem |
|---|---|---|---|
| z indeksami (tak jest w kodzie) | 8 po 44 bajty: 352 bajty | 24 po 4 bajty: 96 bajtów | 448 bajtów |
| bez indeksów | 24 po 44 bajty: 1056 bajtów | brak | 1056 bajtów |

To ta sama reguła czytana w dwie strony: róg jest jednym wierzchołkiem wtedy, gdy **wszystkie** jego atrybuty są wspólne.

### 2.2 Współrzędne lokalne i kierunek nawijania

Pozycje w buforze są zapisane w **przestrzeni lokalnej** obiektu. Płytka podłogi ma w pliku rogi (-1, 0, -1), (1, 0, -1), (1, 0, 1) i (-1, 0, 1): kwadrat 2 m na 2 m, leżący na wysokości 0, ze środkiem w początku układu. Ściana leży wzdłuż osi X od x = -1 do x = 1, ma wysokość od y = 0 do y = 3 i grubość od z = -0,14 do z = 0,14, a początek układu jest w środku jej podstawy. Dane nie mówią, gdzie obiekt stoi ani skąd jest oglądany. O tym decydują trzy macierze, przez które shader wierzchołków mnoży każdą pozycję ([`../scene/transforms.md`](../scene/transforms.md) i [`../scene/camera.md`](../scene/camera.md)). Dzięki temu bufor wysyłam na kartę raz, a ta sama płytka leży pod każdą komórką labiryntu: 100 płytek labiryntu 10 na 10 to jedna siatka i 100 macierzy modelu ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 2.2).

Gdyby macierzy nie było, pozycje z bufora trafiałyby do `gl_Position` bez zmian i byłyby od razu **znormalizowanymi współrzędnymi urządzenia** (NDC, [`shaders.md`](shaders.md), sekcja 2.2): środek okna to (0, 0), lewa krawędź x = -1, prawa x = 1, dół y = -1, góra y = 1. Płytki podłogi nie byłoby wtedy widać wcale: wszystkie jej wierzchołki mają y = 0, więc oba trójkąty leżałyby na jednej poziomej linii przez środek okna i miały na ekranie zerowe pole.

**Kierunek nawijania** (winding order) to kolejność, w jakiej wierzchołki trójkąta obiegają go na ekranie. OpenGL domyślnie uznaje trójkąt za zwrócony przodem, gdy jego wierzchołki idą **przeciwnie do ruchu wskazówek zegara** (counter clockwise, CCW). Dla bryły zamkniętej reguła brzmi: każda ściana ma być nawinięta przeciwnie do ruchu wskazówek zegara, **gdy patrzę na nią z zewnątrz**. Wtedy ściany zwrócone do kamery są "przodem", a ściany po drugiej stronie bryły "tyłem".

Kierunek nawijania da się policzyć z samych pozycji: iloczyn wektorowy dwóch krawędzi trójkąta, `(b - a) x (c - a)`, wskazuje w tę stronę, z której trójkąt a, b, c widać jako nawinięty przeciwnie do ruchu wskazówek zegara. Dla pierwszego trójkąta płytki podłogi (sekcja 5.2) wychodzi (0, 4, 0), czyli w górę: płytka jest "przodem" dla kogoś, kto patrzy na nią z góry, a tak patrzy gracz. Ten sam kierunek ma normalna zapisana w pliku, (0, 1, 0).

Dziś kierunek nawijania nie ma widocznego skutku, bo odrzucanie tylnych ścian (face culling) jest wyłączone: w `src/` nie ma `glEnable(GL_CULL_FACE)`, rysowane są obie strony każdego trójkąta, a o tym, co zasłania co, decyduje test głębi. Dzięki temu po wlocie kamerą w ścianę (tryb noclip) widać jej wewnętrzne strony. Poprawne nawinięcie zostaje jednak warunkiem na przyszłość: po `glEnable(GL_CULL_FACE)` ściana nawinięta odwrotnie znika (ćwiczenie 6).

## 3. Jak to działa w OpenGL

Pełna lista wywołań, od `glGenVertexArrays` do `glDeleteVertexArrays`, jest w [`buffers-vao.md`](buffers-vao.md), sekcja 3.1: bufor indeksów to tam krok 8, a wywołanie rysujące to krok 11. Kolejność, w jakiej VAO zapisuje bufor indeksów, pokazuje sekcja 3.2 tamtego dokumentu. Tutaj zostaje porównanie dwóch wywołań rysujących.

### 3.1 `glDrawArrays` a `glDrawElements`

| | `glDrawArrays(mode, first, count)` | `glDrawElements(mode, count, type, offset)` |
|---|---|---|
| Skąd bierze wierzchołki | kolejno z buforów: `first`, `first + 1`, ... | w kolejności podanej przez indeksy z EBO bieżącego VAO |
| Co znaczy `count` | **liczba wierzchołków** | **liczba indeksów** |
| Potrzebuje EBO | nie | tak |

Parametr `mode` mówi, jak grupować kolejne wierzchołki (albo indeksy). Projekt używa dwóch wartości:

| `mode` | Grupowanie | Kto tak rysuje |
|---|---|---|
| `GL_TRIANGLES` | każde trzy kolejne indeksy to jeden trójkąt | modele z plików OBJ |
| `GL_LINES` | każde dwa kolejne indeksy to jeden odcinek | `game::ColliderLines`: pudełka i kule kolizji |

`count` nie jest liczbą trójkątów ani liczbą liczb `float`: dla jednego trójkąta to 3, dla płytki podłogi z 2 trójkątów 6, dla ściany z 30 trójkątów 90, a dla sześcianu z 12 krawędzi 24.

Parametr `type` w `glDrawElements` to typ jednego indeksu w buforze: `GL_UNSIGNED_BYTE`, `GL_UNSIGNED_SHORT` albo `GL_UNSIGNED_INT`. Musi zgadzać się z typem tablicy w C++. Ostatni parametr to miejsce pierwszego indeksu w buforze, w bajtach, zapisane jako wskaźnik (sekcja 5.6).

Obie funkcje rysują tym, co jest bieżące w chwili wywołania: bieżącym programem i bieżącym VAO. `glDrawArrays` nie jest w projekcie wołane nigdzie: pierwszy trójkąt z M1 rysował się nim, zanim doszły indeksy.

## 4. Shadery

Ta część modułu nie ma własnych shaderów. Indeksy nie są widoczne w GLSL: shader wierzchołków dostaje atrybuty wierzchołka, który wskazał indeks, i nie wie, czy przyszły po kolei, czy przez bufor indeksów. Modele rysują programy `textured`, `lit` i `gouraud`, a linie program `color`. Najprostszą parę, `color.vert` i `color.frag`, omawia linia po linii [`shaders.md`](shaders.md), sekcja 4. Zgodność numerów atrybutów z `layout(location = N)` omawia [`buffers-vao.md`](buffers-vao.md), sekcja 4, a trzy macierze, przez które shader wierzchołków mnoży pozycje, [`uniforms.md`](uniforms.md), sekcja 4.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`assets/models/floor_tile.obj`](../../../assets/models/floor_tile.obj) | najmniejszy model gry: 4 pozycje, 2 trójkąty. Przykład indeksów trójkątów (sekcja 5.2) |
| [`assets/models/wall_straight.obj`](../../../assets/models/wall_straight.obj) | ściana: 24 pozycje, 30 trójkątów, po wczytaniu 60 wierzchołków i 90 indeksów (sekcja 2.1) |
| [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp) | dane wpisane w kodzie: tablice `UNIT_CUBE_CORNERS` i `UNIT_CUBE_EDGES` (sekcja 5.3), funkcje `unitCirclePoints` i `unitCircleLines` (sekcja 5.4) |
| [`src/gfx/Mesh.hpp`](../../../src/gfx/Mesh.hpp), [`.cpp`](../../../src/gfx/Mesh.cpp) | klasa, która tworzy bufor indeksów przy związanym VAO (sekcja 5.5) i jako jedyna woła `glDrawElements` (sekcja 5.6). Cała klasa: [`mesh.md`](mesh.md), sekcja 5 |
| [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp) | `game::drawModel`: rysuje model częściami, przez `Mesh::draw(firstIndex, indexCount)` |
| [`src/gfx/Buffer.hpp`](../../../src/gfx/Buffer.hpp), [`src/gfx/VertexArray.hpp`](../../../src/gfx/VertexArray.hpp) | klasy, z których `Mesh` jest zbudowany. Opis: [`buffers-vao.md`](buffers-vao.md), sekcja 5 |

### 5.2 Indeksy trójkątów: płytka podłogi

Cała geometria pliku `floor_tile.obj` to dwanaście linii:

```text
v -1.000000 0.000000 1.000000
v 1.000000 0.000000 1.000000
v 1.000000 0.000000 -1.000000
v -1.000000 0.000000 -1.000000
vn -0.0000 1.0000 -0.0000
vt 0.500000 -0.500000
vt -0.500000 0.500000
vt -0.500000 -0.500000
vt 0.500000 0.500000
f 2/1/1 4/2/1 1/3/1
f 2/1/1 3/4/1 4/2/1
```

Linie `v` to cztery pozycje, `vn` jedna normalna (w górę), `vt` cztery współrzędne tekstury, a każda linia `f` to jeden trójkąt. Róg trójkąta ma postać `pozycja/uv/normalna`, z numerami liczonymi od 1. Plik OBJ ma więc **osobne indeksy dla każdego atrybutu**, a OpenGL chce **jednego indeksu na cały wierzchołek** (sekcja 2.1). Tłumaczy to loader: każda trójka, której jeszcze nie widział, staje się nowym wierzchołkiem z kolejnym numerem, a trójka widziana wcześniej dostaje numer nadany za pierwszym razem ([`../assets/obj-loader.md`](../assets/obj-loader.md)).

Dla płytki, w kolejności czytania pliku:

| Trójka w pliku | Nowa? | Numer wierzchołka | Pozycja |
|---|---|---|---|
| `2/1/1` | tak | 0 | (1, 0, 1) |
| `4/2/1` | tak | 1 | (-1, 0, -1) |
| `1/3/1` | tak | 2 | (-1, 0, 1) |
| `2/1/1` | nie, to wierzchołek 0 | 0 | |
| `3/4/1` | tak | 3 | (1, 0, -1) |
| `4/2/1` | nie, to wierzchołek 1 | 1 | |

Bufor wierzchołków ma więc 4 wierzchołki (176 bajtów), a bufor indeksów sześć liczb (24 bajty):

```text
indeksy płytki:   0, 1, 2,   0, 3, 1
trójkąt 1:        wierzchołki 0, 1, 2
trójkąt 2:        wierzchołki 0, 3, 1
```

```text
płytka widziana z góry (oś X w prawo, oś Z w dół rysunku)

   1 ---------- 3        z = -1
   |          / |
   |        /   |        wspólna przekątna łączy wierzchołki 0 i 1
   |      /     |
   2 ---------- 0        z = +1

 x = -1       x = +1
```

- Wierzchołki 0 i 1 występują w indeksach dwa razy: to dwa rogi na wspólnej przekątnej. Dzięki nim bufor ma 4 wierzchołki, a nie 6.
- Oba trójkąty obiegają płytkę w tę samą stronę. Iloczyn wektorowy krawędzi pierwszego, `(v1 - v0) x (v2 - v0)`, to `(-2, 0, -2) x (-2, 0, 0) = (0, 4, 0)`. Dla drugiego, `(v3 - v0) x (v1 - v0)`, to `(0, 0, -2) x (-2, 0, -2) = (0, 4, 0)`. Oba wskazują w górę, zgodnie z normalną z pliku.
- Wartości indeksów muszą być mniejsze od liczby wierzchołków (4). OpenGL tego nie sprawdza (pułapka 6). Dla modeli pilnuje tego loader, który odrzuca plik z numerem spoza listy.
- Panel Assets pokazuje te liczby dla każdego wczytanego modelu, w linii `%d vertices, %d triangles`.

Ściana idzie tą samą drogą, tylko jest większa: 30 linii `f`, 60 różnych trójek, 90 indeksów. Liczby dla wszystkich sześciu modeli, policzone z plików OBJ (każda linia `f` w tych plikach to trójkąt):

| Model | Pozycji w pliku (`v`) | Trójkątów (`f`) | Wierzchołków w buforze | Indeksów |
|---|---|---|---|---|
| `floor_tile.obj` | 4 | 2 | 4 | 6 |
| `wall_straight.obj` | 24 | 30 | 60 | 90 |
| `wall_pillar.obj` | 24 | 30 | 60 | 90 |
| `crystal_a.obj` | 14 | 24 | 60 | 72 |
| `crystal_b.obj` | 39 | 66 | 144 | 198 |
| `gate.obj` | 56 | 70 | 148 | 210 |

Kolumnę "wierzchołków w buforze" policzyłem jako liczbę różnych trójek w liniach `f`, czyli tak, jak liczy loader. Dla ściany zgadza się ona z liczbami używanymi w [`mesh.md`](mesh.md) (ćwiczenie 1).

### 5.3 Indeksy wpisane ręcznie: sześcian z krawędzi

Jedyne dane wierzchołków i indeksów, które są w kodzie C++ jako tablice, należą do `game::ColliderLines`, czyli do rysowania pudełek kolizji ([`../scene/collision.md`](../scene/collision.md), sekcja 5). Z anonimowej przestrzeni nazw w `ColliderLines.cpp`:

```cpp
// Sizes of the two arrays below. std::size_t, because that is the type of an array size.
constexpr std::size_t CORNER_COUNT = 8;
constexpr std::size_t EDGE_COUNT = 12;
constexpr std::size_t INDICES_PER_LINE = 2;

// The corners of a cube that reaches from (0, 0, 0) to (1, 1, 1). Only the position is
// filled in: lines have no use for a normal or a texture coordinate, and they stay zero.
// Corners 0 to 3 are the bottom face (y = 0), corners 4 to 7 the top face (y = 1), each
// going around the face.
constexpr std::array<gfx::Vertex, CORNER_COUNT> UNIT_CUBE_CORNERS = {
    gfx::Vertex{.position = {0.0F, 0.0F, 0.0F}}, // 0
    gfx::Vertex{.position = {1.0F, 0.0F, 0.0F}}, // 1
    gfx::Vertex{.position = {1.0F, 0.0F, 1.0F}}, // 2
    gfx::Vertex{.position = {0.0F, 0.0F, 1.0F}}, // 3
    gfx::Vertex{.position = {0.0F, 1.0F, 0.0F}}, // 4
    gfx::Vertex{.position = {1.0F, 1.0F, 0.0F}}, // 5
    gfx::Vertex{.position = {1.0F, 1.0F, 1.0F}}, // 6
    gfx::Vertex{.position = {0.0F, 1.0F, 1.0F}}, // 7
};

// Every two indices are one line (GL_LINES): the two corners an edge joins.
constexpr std::array<std::uint32_t, EDGE_COUNT * INDICES_PER_LINE> UNIT_CUBE_EDGES = {
    0, 1, 1, 2, 2, 3, 3, 0, // bottom face
    4, 5, 5, 6, 6, 7, 7, 4, // top face
    0, 4, 1, 5, 2, 6, 3, 7, // the four vertical edges
};
```

| Element | Wyjaśnienie |
|---|---|
| `CORNER_COUNT`, `EDGE_COUNT`, `INDICES_PER_LINE` | 8, 12 i 2. Żadna z liczb nie jest wpisana wprost w nawiasach ostrych `std::array`: rozmiar tablicy indeksów to `EDGE_COUNT * INDICES_PER_LINE`, czyli 24. Typ `std::size_t`, bo taki jest typ rozmiaru tablicy |
| `gfx::Vertex{.position = {...}}` | inicjalizacja z nazwanym polem (designated initializer, C++20): ustawiam tylko pozycję, a normalna, uv i styczna zostają zerami z definicji struktury ([`mesh.md`](mesh.md), sekcja 5.2). Program `color` ich nie czyta |
| komentarz `// 0` do `// 7` | numer wierzchołka, czyli wartość, którą wpisuje się w tablicy indeksów |
| sześcian od (0, 0, 0) do (1, 1, 1) | przestrzeń lokalna tej siatki. Początek układu jest w **narożniku**, a nie w środku: skala o rozmiar pudełka i przesunięcie do jego rogu `min` kładą wtedy sześcian dokładnie na pudełku ([`../scene/collision.md`](../scene/collision.md), sekcja 5) |
| `UNIT_CUBE_EDGES`, wiersz 1 | cztery krawędzie dolnej ściany: 0 do 1, 1 do 2, 2 do 3, 3 do 0. Każdy narożnik występuje dwa razy: jako koniec jednej krawędzi i początek następnej |
| wiersz 2 | to samo dla górnej ściany, narożniki od 4 do 7 |
| wiersz 3 | cztery krawędzie pionowe: narożnik dolny i narożnik nad nim (numer większy o 4) |
| `constexpr` i anonimowa przestrzeń nazw | obie tablice są stałymi czasu kompilacji, widocznymi tylko w tym pliku |
| przyrostek `F` | literał typu `float`. Bez niego `1.0` byłoby typu `double` |

```text
        7 ---------- 6
       /|           /|         y
      / |          / |         |
     4 ---------- 5  |         |
     |  3 --------|- 2         +------ x
     | /          | /         /
     |/           |/         z
     0 ---------- 1
```

Każdy narożnik należy do trzech krawędzi, więc w 24 indeksach każdy numer od 0 do 7 występuje dokładnie trzy razy. Bufor wierzchołków ma 8 razy 44 bajty, czyli 352 bajty, a bufor indeksów 24 razy 4 bajty, czyli 96 bajtów.

Linie nie mają kierunku nawijania: odcinek z 0 do 1 i odcinek z 1 do 0 to ten sam odcinek. Kolejność w parze nie ma więc znaczenia, inaczej niż kolejność w trójce trójkąta.

### 5.4 Indeksy liczone w pętli: okrąg

Kulę kolizji (zasięg gracza, kule zbierania kryształów) rysuję jako trzy okręgi. Okrąg to druga siatka `ColliderLines`. Jej danych nie ma w kodzie jako tabeli, tylko jako dwie krótkie funkcje:

```cpp
// A circle is drawn as this many straight pieces. 32 look round at the size of a pickup
// sphere on the screen.
constexpr std::size_t CIRCLE_SEGMENTS = 32;

std::array<gfx::Vertex, CIRCLE_SEGMENTS> unitCirclePoints() {
    std::array<gfx::Vertex, CIRCLE_SEGMENTS> points{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const float angle =
            glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(CIRCLE_SEGMENTS);
        points[i].position = {std::cos(angle), std::sin(angle), 0.0F};
    }
    return points;
}

// Every two indices are one line (GL_LINES): each point is joined to the next one, and
// the last point back to the first (that is what the remainder does).
std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> unitCircleLines() {
    std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> indices{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        indices[i * INDICES_PER_LINE] = static_cast<std::uint32_t>(i);
        indices[i * INDICES_PER_LINE + 1] = static_cast<std::uint32_t>((i + 1) % CIRCLE_SEGMENTS);
    }
    return indices;
}
```

(Komentarz nad `unitCirclePoints` jest tu pominięty.)

- **Punkty.** Punkt numer `i` leży pod kątem `i / 32` pełnego obrotu, czyli co 11,25 stopnia. `glm::two_pi<float>()` to 2 pi, pełny obrót w radianach. Punkt o kącie `a` na okręgu o promieniu 1 ma współrzędne `(cos a, sin a)`, a trzecia współrzędna to 0: okrąg leży w płaszczyźnie XY, ze środkiem w początku układu. To znowu przestrzeń lokalna: promień i miejsce kuli są w macierzy modelu.
- **Indeksy.** Odcinek numer `i` łączy punkt `i` z punktem `i + 1`: pary (0, 1), (1, 2), (2, 3) i tak dalej. Ostatni odcinek, numer 31, musi wrócić do punktu 0, a nie iść do punktu 32, którego nie ma. Robi to reszta z dzielenia: `(31 + 1) % 32` to 0. Dla wszystkich wcześniejszych `i` reszta niczego nie zmienia, bo `i + 1` jest mniejsze od 32.
- **Liczby.** 32 punkty i 64 indeksy: 32 odcinki po 2. Każdy punkt występuje w indeksach dwa razy, jako koniec jednego odcinka i początek następnego. Bez indeksów trzeba by 64 wierzchołków.
- **Dlaczego funkcja, a nie tabela.** Komentarz w pliku mówi to wprost: 32 sinusy i kosinusy łatwiej policzyć, niż wpisać. `std::cos` i `std::sin` nie są `constexpr` w C++20, więc tablice powstają przy starcie programu, a nie w czasie kompilacji.

Jedna siatka okręgu wystarcza na wszystkie trzy okręgi każdej kuli: `ColliderLines::drawSpheres` rysuje ją trzy razy z trzema obrotami z tablicy `CIRCLE_ROTATIONS` (bez obrotu, 90 stopni wokół osi X, 90 stopni wokół osi Y). To przykład macierzy modelu z obrotem, opisany w [`../scene/collision.md`](../scene/collision.md), sekcja 5.

### 5.5 Bufor indeksów powstaje przy związanym VAO

Wiązanie `GL_ELEMENT_ARRAY_BUFFER` należy do bieżącego VAO ([`buffers-vao.md`](buffers-vao.md), sekcja 2.4). Bufor indeksów trzeba więc utworzyć wtedy, gdy bieżącym VAO jest ten właściwy. W projekcie pilnuje tego jedno miejsce: kolejność pól w `gfx::Mesh`.

```cpp
    // Order matters: members are constructed top to bottom.
    //   1. m_vertexArray: its constructor binds it, so the two buffers below are created
    //      while it is the bound vertex array.
    //   2. m_vertexBuffer: stays bound to GL_ARRAY_BUFFER, and that is how the attribute
    //      setup in the constructor body tells the vertex array which buffer to read from.
    //   3. m_indexBuffer: binding it to GL_ELEMENT_ARRAY_BUFFER records it in the bound
    //      vertex array.
    VertexArray m_vertexArray;
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
```

Kolejność zdarzeń jest wyznaczona przez kolejność **deklaracji** pól, a nie przez kolejność na liście inicjalizacyjnej. Dla siatki sześcianu z krawędzi (8 wierzchołków, 24 indeksy):

| # | Co się wykonuje | Wywołania OpenGL | Stan po tym kroku |
|---|---|---|---|
| 1 | `m_vertexArray`, konstruktor domyślny (nie ma go na liście inicjalizacyjnej, więc wykonuje się sam, jako pierwszy) | `glGenVertexArrays`, `glBindVertexArray` | VAO siatki istnieje i **jest bieżący** |
| 2 | `m_vertexBuffer(GL_ARRAY_BUFFER, ...)` | `glGenBuffers`, `glBindBuffer(GL_ARRAY_BUFFER, ...)`, `glBufferData` | 352 bajty na karcie, bufor związany z `GL_ARRAY_BUFFER`. VAO jeszcze o nim nie wie |
| 3 | `m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)` | `glGenBuffers`, `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`, `glBufferData` | 96 bajtów na karcie. Samo związanie **zapisało bufor indeksów w bieżącym VAO** z kroku 1. Wiązanie `GL_ARRAY_BUFFER` z kroku 2 jest nietknięte, bo to inny cel |
| 4 | ciało konstruktora: cztery razy `setFloatAttribute` | `glBindVertexArray`, `glEnableVertexAttribArray`, `glVertexAttribPointer` | cztery atrybuty czytają z bufora z kroku 2 |

Szczegóły, o które można zostać zapytanym:

- **Krok 3 zależy od kroku 1.** Bufor indeksów trafia do VAO, który jest bieżący w chwili `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`. Dlatego `m_vertexArray` jest zadeklarowany przed oboma buforami, a jego konstruktor wiąże ([`buffers-vao.md`](buffers-vao.md), sekcja 5.5). Deklaracja `m_indexBuffer` nad `m_vertexArray` kompiluje się, a indeksy trafiają do cudzego VAO albo do żadnego (pułapka 8).
- **Krok 4 zależy od kroku 2**, ale nie od kolejności VAO i bufora wierzchołków. Wiązanie `GL_ARRAY_BUFFER` jest stanem globalnym kontekstu i liczy się tylko to, co jest związane **w chwili** `setFloatAttribute`. Między krokiem 2 a 4 nikt go nie zmienia: krok 3 używa innego celu.
- **Wszystko dzieje się w jednym konstruktorze.** Między utworzeniem buforów a opisem atrybutów nie może powstać żadna inna siatka, więc nikt nie zabierze wiązania. W M1 tak nie było: kostka miała VAO i bufory jako trzy pola `NightMazeApp`, a atrybuty opisywała dopiero w ciele konstruktora aplikacji, więc pola tworzące siatki musiały stać przed nią. Po usunięciu kostki w M5 ta zależność zniknęła, a komentarz w `NightMazeApp.hpp` mówi już tylko o jednej: oba renderery proszą `m_assets` o modele, więc stoją po nim.
- Dane wierzchołków i indeksy są wysyłane na kartę raz, przy tworzeniu siatki. W klatce nie ma żadnego `glBufferData`. Jedynym buforem zmienianym w klatce jest bufor świateł, przez `glBufferSubData` ([`uniform-buffers.md`](uniform-buffers.md)).
- Bufor uniformów w `game::LightRig` wiąże się z celem `GL_UNIFORM_BUFFER`. To inny cel niż `GL_ARRAY_BUFFER` i `GL_ELEMENT_ARRAY_BUFFER`, a VAO go nie zapamiętuje, więc jego utworzenie nie dotyka żadnej siatki.

Konstruktor `Mesh` linia po linii jest w [`mesh.md`](mesh.md), sekcja 5.4.

### 5.6 Rysowanie: `glDrawElements`

Jedyne `glDrawElements` projektu stoi na końcu `Mesh::draw(firstIndex, indexCount)`:

```cpp
    // One call brings back the whole description: the four attributes, the vertex buffer
    // they read from and the index buffer.
    m_vertexArray.bind();
```

```cpp
    const std::size_t offsetInBytes = static_cast<std::size_t>(firstIndex) * sizeof(std::uint32_t);
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const void* offsetAsPointer = reinterpret_cast<const void*>(offsetInBytes);

    // The count is a number of indices, not of triangles. Its parameter is a signed type.
    GL_CHECK(
        glDrawElements(m_primitive, static_cast<GLsizei>(indexCount), INDEX_TYPE, offsetAsPointer));
```

| Element | Co robi |
|---|---|
| `m_vertexArray.bind();` | Jedno wywołanie przywraca cały opis: cztery włączone atrybuty, ich format, bufor wierzchołków **i bufor indeksów**. Żadnego z buforów nie wiążę osobno, bo VAO je pamięta |
| `m_primitive` | `GL_TRIANGLES` albo `GL_LINES`, podane w konstruktorze siatki |
| `indexCount` | liczba **indeksów**. Dla ściany 90, nie liczba trójkątów (30) i nie liczba wierzchołków (60) |
| `INDEX_TYPE` | stała równa `GL_UNSIGNED_INT`: typ jednego indeksu w buforze. Musi odpowiadać typowi tablicy indeksów (`std::uint32_t`) |
| `offsetAsPointer` | przesunięcie pierwszego indeksu w buforze indeksów, w bajtach. Wyjaśnienie niżej |

**Dlaczego ostatni parametr jest wskaźnikiem.** To ta sama historyczna osobliwość co w `glVertexAttribPointer` ([`buffers-vao.md`](buffers-vao.md), sekcja 5.6). W starym OpenGL ostatni parametr `glDrawElements` był adresem tablicy indeksów w pamięci programu, stąd typ `const void*`. Gdy bieżący VAO ma zapisany bufor indeksów, wartość "wskaźnika" jest odczytywana jako **liczba bajtów od początku tego bufora**, od której zaczyna się pierwszy indeks. Indeks numer `firstIndex` zaczyna się w bajcie `firstIndex * 4`. Dla `firstIndex` równego 0 wychodzi wskaźnik o wartości 0, czyli "zacznij od początku bufora" (w poradnikach zapisuje się to jako `nullptr`). Żeby narysować tylko górną ścianę sześcianu z krawędzi, trzeba podać 8 indeksów i przesunięcie 32 bajtów (8 indeksów dolnej ściany po 4 bajty): `draw(8, 8)`.

Są dwie funkcje `draw` i trzy miejsca, które je wołają:

| Wywołanie | Gdzie | Prymityw | Co rysuje |
|---|---|---|---|
| `model->mesh.draw(part.firstIndex, part.indexCount)` | `game::drawModel` w `ModelDraw.cpp` | `GL_TRIANGLES` | jedną część modelu (zakres indeksów jednego materiału) dla jednej macierzy modelu. Tak rysowane są płytki, ściany, słupki, brama i kryształy |
| `m_unitCube.draw()` | `ColliderLines::draw` | `GL_LINES` | 12 krawędzi jednego pudełka: wszystkie 24 indeksy |
| `m_unitCircle.draw()` | `ColliderLines::drawSpheres` | `GL_LINES` | jeden okrąg: wszystkie 64 indeksy. Trzy razy na kulę |

`draw()` bez parametrów to `draw(0, m_indexCount)`, czyli cała siatka. Wszystkie sześć modeli gry ma po jednej części (jedna linia `usemtl` w każdym pliku OBJ), więc `part.firstIndex` jest dziś zawsze zerem, a `part.indexCount` obejmuje całą siatkę. Sprawdzenie zakresu, mnożenie przez 4 i rzutowanie na wskaźnik omawia linia po linii [`mesh.md`](mesh.md), sekcja 5.5.

**Ile to wywołań.** Labirynt domyślny (10 na 10, ziarno 1) to 342 wywołania `glDrawElements` na klatkę: 100 płytek, 121 ścian i 121 słupków ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 2.5). Na początku rundy dochodzi jedno dla bramy i 13 dla kryształów, razem 356. Zebrany kryształ przestaje być rysowany, a brama znika z listy, gdy po otwarciu schowa się pod podłogę ([`../game/gameplay.md`](../game/gameplay.md), sekcja 5). Linie pudełek i kul dochodzą tylko wtedy, gdy włączy je panel Collision. Każde z tych wywołań uruchamia potok z [`shaders.md`](shaders.md) (sekcja 2.1): shader wierzchołków dla wierzchołków wskazanych przez indeksy, składanie trójkątów albo odcinków, rasteryzacja, shader fragmentów, test głębi.

`bind()` jest wołane przed każdym rysowaniem i jest konieczne: każde `Mesh::draw` wiąże VAO swojej siatki, więc po płytkach bieżący jest VAO płytki, a ściany potrzebują własnego. Backend ImGui przy rysowaniu paneli też wiąże własny VAO i własny program. Przed rysowaniem ustawiam więc wszystko, czego potrzebuję.

### 5.7 Jak to zostało sprawdzone

Same klasy `Buffer` i `VertexArray` oraz pierwszy trójkąt sprawdziłem osobno ([`buffers-vao.md`](buffers-vao.md), sekcja 5.8). Rysowanie z indeksami ma dwa rodzaje dowodów: stary pomiar na kostce i obraz dzisiejszej gry.

**Kostka z M1: pomiar, którego kodu już nie ma.** W M1 jedyną bryłą projektu była kostka wpisana ręcznie w `NightMazeApp.cpp`: 24 wierzchołki (pozycja i kolor, po 4 na ścianę, każda ściana w innym kolorze), 36 indeksów, własny VAO i dwa bufory jako pola aplikacji, rysowana jednym `glDrawElements`. Test z ukrytym oknem dziedziczył po `game::NightMazeApp`, wołał prawdziwe `onRender` i czytał obraz przez `glReadPixels`. Ten kod został usunięty w M5 i testu nie da się już powtórzyć w tej postaci. Wyniki zostawiam, bo pokazują reguły, które dotyczą także dzisiejszych siatek. Framebuffer 2560 x 1440 (okno 1280 x 720 na ekranie Retina), Mac, sterownik Apple. Kolory jako (czerwony, zielony, niebieski) od 0 do 255:

| Próba | Wynik |
|---|---|
| piksel w środku okna | (229, 51, 51): czerwony ściany przedniej |
| piksel w rogu okna | (5, 8, 20): kolor tła |
| wszystkie kolory w klatce | tło i dokładnie trzy kolory ścian: czerwony na 152 819 pikselach, niebieski na 99 544, turkusowy na 39 257. Było widać ścianę przednią, lewą i górną |
| ta sama klatka z włączonym `glEnable(GL_CULL_FACE)` | obraz identyczny piksel w piksel: wszystkie widoczne ściany były nawinięte przeciwnie do ruchu wskazówek zegara, patrząc z zewnątrz |
| `glCullFace(GL_FRONT)`: rysowane tylko ściany zwrócone tyłem | trzy pozostałe kolory ścian, w tym samym prostokącie |
| test głębi unieszkodliwiony (`glDepthFunc(GL_ALWAYS)`) | pięć kolorów naraz: ściany rysowane później zamalowywały wcześniejsze |
| `glGetError` po każdej próbie | `GL_NO_ERROR` |
| kopia nagłówka z polem bufora indeksów zadeklarowanym **nad** polem VAO | pusta klatka (samo tło) i w każdej klatce linia `[error] GL_INVALID_OPERATION after glDrawElements(...)`: VAO nie miał bufora indeksów |

Ostatni wiersz jest pomiarem pułapki 8. Wtedy żaden VAO nie był związany w chwili tworzenia bufora indeksów. W `gfx::Mesh` skutek tej samej zamiany pól byłby inny (ćwiczenie 7), ale przyczyna jest ta sama.

**Dzisiejszy kod: sprawdzony na obrazie, nie testem.** `gfx::Mesh` nie ma testu jednostkowego, bo wymaga kontekstu OpenGL ([`mesh.md`](mesh.md), sekcja 5.6). Co wiadomo:

| Co | Jak sprawdzone |
|---|---|
| indeksy trójkątów modeli labiryntu | na obrazie, na Windowsie, w M2 + M3 i w M4: ściany, słupki i podłoga na zrzutach ekranu zgadzają się z planem w panelu Maze ([`mesh.md`](mesh.md), sekcja 5.6) |
| indeksy odcinków sześcianu (`GL_LINES`) | na obrazie, w M2 + M3: żółte pudełka leżą na ścianach i słupkach |
| M5: brama, kryształy, okrąg | dla Windowsa zgłoszone 2026-10-05: build Debug i Release bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach, obraz sprawdzony zrzutami ekranu. Żaden z tych testów nie dotyka `Mesh` ani `ColliderLines`: to testy logiki bez OpenGL |
| rysowanie zakresu, który nie zaczyna się od indeksu 0 | **niesprawdzone**: wszystkie modele mają po jednej części, więc `firstIndex` jest zawsze zerem |
| kierunek nawijania modeli | policzony na kartce dla płytki podłogi (sekcja 5.2). Z włączonym odrzucaniem tylnych ścian gry nie uruchamiałem |
| macOS | nic z M5 nie było tam budowane ani uruchamiane |
| ręcznie | nikt jeszcze nie przeszedł gry z M5 ręcznie, więc także nie włączał linii w panelu Collision |

## 6. Panel ImGui

Indeksy nie mają własnego panelu. Widać je pośrednio w dwóch miejscach. Panel **Assets** pokazuje dla każdego wczytanego modelu liczbę wierzchołków i trójkątów (linia `%d vertices, %d triangles`): to liczby z tabeli w sekcji 5.2, a liczba indeksów to trzy razy liczba trójkątów ([`../assets/asset-cache.md`](../assets/asset-cache.md), sekcja 6). Panel **Collision** ma pole `Draw collision shapes`, które włącza rysowanie obu siatek z odcinków: sześcianu i okręgu ([`../scene/collision.md`](../scene/collision.md), sekcja 6).

## 7. Pułapki

1. **Bufor indeksów utworzony bez związanego VAO.** Wiązanie `GL_ELEMENT_ARRAY_BUFFER` należy do bieżącego VAO. Gdy żaden VAO nie jest związany, bufor powstaje i dostaje dane, ale **żaden VAO o nim nie wie**: późniejsze `glDrawElements` nie ma indeksów. Na Macu samo utworzenie nie daje żadnego błędu ([`buffers-vao.md`](buffers-vao.md), sekcja 5.8), więc pomyłka wychodzi dopiero przy rysowaniu. Inne sterowniki mogą zgłosić `GL_INVALID_OPERATION` już przy wiązaniu. Podobnie groźny wariant: związany jest **inny** VAO i bufor indeksów po cichu trafia do niego. W projekcie chroni przed tym konstruktor `VertexArray`, który wiąże nowy VAO, oraz kolejność pól w `Mesh`: VAO tuż przed swoimi buforami (sekcja 5.5).
2. **Odwiązanie EBO przy bieżącym VAO.** `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)` "dla porządku" po konfiguracji, gdy VAO jest jeszcze związany, zapisuje w nim "brak bufora indeksów". Odwiązywać wolno dopiero po `glBindVertexArray(0)`, a najlepiej wcale. Z `GL_ARRAY_BUFFER` jest odwrotnie: jego odwiązanie po `glVertexAttribPointer` niczego nie psuje, bo VAO tego wiązania nie przechowuje.
3. **`count` w wywołaniu rysującym.** W `glDrawArrays` to liczba wierzchołków, w `glDrawElements` liczba indeksów. Nigdy liczba trójkątów ani liczb `float`. Dla ściany 90, nie 30 i nie 60. Za mała wartość rysuje część geometrii, za duża czyta poza buforem indeksów. `Mesh::draw` odrzuca zakres wystający poza bufor ([`mesh.md`](mesh.md), sekcja 5.5), więc w projekcie druga pomyłka kończy się brakiem obiektu, a nie czytaniem cudzej pamięci.
4. **Ściana nawinięta zgodnie z ruchem wskazówek zegara.** Dziś widoczna, bo odrzucanie tylnych ścian jest wyłączone. Zniknie po `glEnable(GL_CULL_FACE)` (sekcja 2.2). Typowa pomyłka przy pisaniu danych ręcznie: ściana tylna, lewa albo dolna przepisana "tak jak przednia", bez odwrócenia kierunku patrzenia. W modelach z Blendera pilnuje tego eksporter, o ile normalne w scenie są zwrócone na zewnątrz.
5. **Typ indeksu niezgodny z danymi.** Tablica `std::uint32_t` (4 bajty na indeks) narysowana z `GL_UNSIGNED_SHORT` jest czytana po 2 bajty: co drugi "indeks" to zero, bryła zamienia się w wachlarz trójkątów zbiegających się w wierzchołku 0. Żadnego błędu OpenGL. W projekcie typ jest w jednym miejscu (`INDEX_TYPE` w `Mesh.cpp`) i pilnuje go `static_assert`.
6. **Indeks spoza bufora wierzchołków.** Wartość 8 albo większa przy 8 wierzchołkach każe karcie czytać poza buforem. OpenGL tego nie sprawdza: wynik jest nieokreślony, zwykle trójkąt albo odcinek ciągnący się do przypadkowego punktu. Tak skończyłby się okrąg bez reszty z dzielenia w `unitCircleLines`: ostatni indeks miałby wartość 32 przy 32 punktach.
7. **Wspólny wierzchołek tam, gdzie atrybuty się różnią.** Indeks wybiera wierzchołek razem ze wszystkimi atrybutami. Prostopadłościan z 8 wierzchołkami ma 8 normalnych, po jednej na róg, i każda ściana jest cieniowana płynnym przejściem między czterema z nich. Płaskie ściany wymagają osobnych wierzchołków dla każdej ściany (sekcja 2.1). Sześcian z krawędzi może mieć 8, bo jego wierzchołek to sama pozycja.
8. **Bufor indeksów zadeklarowany przed VAO.** Pola klasy powstają w kolejności deklaracji. `Buffer m_indexBuffer;` nad `VertexArray m_vertexArray;` tworzy bufor indeksów, zanim istnieje VAO, do którego miał trafić. Kod się kompiluje, a `glDrawElements` nie ma indeksów albo ma cudze. Zmierzone w M1 na Macu dla kostki: pusta klatka i `GL_INVALID_OPERATION` po każdym `glDrawElements` (sekcja 5.7).
9. **Własne bufory obok siatek.** Każdy obiekt `gfx::Mesh` wiąże przy tworzeniu własny VAO i własne bufory i zostawia je związane. Kod, który tworzy VAO i bufory osobno, a atrybuty opisuje później, traci wiązanie `GL_ARRAY_BUFFER`, gdy między tymi krokami powstanie jakakolwiek siatka. Tak była zbudowana kostka w M1 i dlatego kolejność pól w `NightMazeApp.hpp` miała wtedy znaczenie. Dziś jedynym użytkownikiem `Buffer` i `VertexArray` jest `Mesh`, który robi wszystko w jednym konstruktorze ([`buffers-vao.md`](buffers-vao.md), pułapka 12).
10. **Brak testu głębi przy bryle.** To nie błąd buforów, ale wygląda jak błąd danych: bez `glEnable(GL_DEPTH_TEST)` ściany rysowane później zamalowują wcześniejsze i bryła wygląda jak wywrócona na lewą stronę ([`../scene/camera.md`](../scene/camera.md), sekcja 3).

Pułapki dotyczące kroku, przesunięcia, rozmiaru bufora, atrybutów i czasu życia obiektów są w [`buffers-vao.md`](buffers-vao.md), sekcja 7, a pułapki samej klasy `Mesh` w [`mesh.md`](mesh.md), sekcja 7.

## 8. Ćwiczenia

Ćwiczenia od 1 do 3 robi się na kartce. Pozostałe zmieniają kod i wymagają zbudowania programu. Linie pudełek i kul włącza pole `Draw collision shapes` w panelu Collision. Po każdym ćwiczeniu wycofaj zmianę (`git checkout src`). Tych ćwiczeń na dzisiejszym kodzie nikt jeszcze nie wykonał, więc nie podaję, co widać na ekranie: przewidź wynik, a potem go sprawdź.

1. **Bajty płytki i sześcianu na kartce.** Ile bajtów zajmuje bufor wierzchołków i bufor indeksów płytki podłogi? W którym bajcie bufora indeksów zaczyna się drugi trójkąt? Ile bajtów zajęłyby indeksy sześcianu z krawędzi, gdyby były typu `GLubyte`, i dlaczego projekt mimo to używa jednego typu indeksu wszędzie? Odpowiedzi: 176 i 24, bajt 12, 24 bajty zamiast 96.
2. **Tłumaczenie trójek na kartce.** Zrób dla pierwszych czterech linii `f` pliku `wall_straight.obj` taką tabelę jak w sekcji 5.2. Ile nowych wierzchołków daje każda linia? Dlaczego dwie linie `f` jednej płaskiej ściany dają razem 4 wierzchołki, a nie 6?
3. **Kierunek nawijania na kartce.** Policz iloczyn wektorowy krawędzi drugiego trójkąta płytki podłogi (wierzchołki 0, 3, 1). Potem zamień w myślach kolejność na 0, 1, 3 i policz jeszcze raz. Co zmieniło się w wyniku i co by to znaczyło po włączeniu `GL_CULL_FACE`?
4. **Pół sześcianu.** W `ColliderLines::draw` zamień `m_unitCube.draw();` na `m_unitCube.draw(0, 8);`, potem na `m_unitCube.draw(8, 8);`, potem na `m_unitCube.draw(16, 8);`. Które krawędzie każdego pudełka zostały? Jakie przesunięcie w bajtach dostaje `glDrawElements` w każdym z trzech przypadków? Co zrobi `m_unitCube.draw(20, 8);` i dlaczego (wskazówka: [`mesh.md`](mesh.md), sekcja 5.5)?
5. **Przekątna zamiast krawędzi.** W `UNIT_CUBE_EDGES` zamień pierwszą parę `0, 1` na `0, 6`. Której krawędzi brakuje i gdzie pojawił się nowy odcinek? Potem usuń cały trzeci wiersz tablicy (cztery krawędzie pionowe). Program nadal się kompiluje: jaką wartość mają brakujące elementy `std::array` i jakie odcinki z nich powstają?
6. **Kierunek nawijania i odrzucanie ścian.** Dopisz tymczasowo w `NightMazeApp::onRender`, zaraz po `glEnable(GL_DEPTH_TEST)`, linię `GL_CHECK(glEnable(GL_CULL_FACE));`. Czy labirynt wygląda tak samo? Wleć w trybie noclip (klawisz N) do środka ściany i pod podłogę: co widać teraz, a co było widać przedtem? Dopisz jeszcze `GL_CHECK(glCullFace(GL_FRONT));` i opisz obraz. Co mówi to o kierunku nawijania modeli z Blendera?
7. **Kolejność pól.** Przenieś w `Mesh.hpp` deklarację `m_indexBuffer` **nad** `m_vertexArray` i nie zmieniaj niczego więcej. Zanim zbudujesz, prześledź na kartce, co jest bieżącym VAO w chwili tworzenia bufora indeksów pierwszej siatki, a co w chwili tworzenia bufora indeksów drugiej. Do którego VAO trafiają indeksy każdej siatki? Potem zbuduj (kompilator może ostrzec o kolejności na liście inicjalizacyjnej) i porównaj z konsolą i ekranem.
8. **Typ indeksu na kartce.** Wypisz pierwsze 8 "indeksów", które karta odczytałaby z bufora `UNIT_CUBE_EDGES`, gdyby `INDEX_TYPE` było równe `GL_UNSIGNED_SHORT` (procesor jest little endian: liczba 1 typu `std::uint32_t` to bajty 01 00 00 00). Jakie odcinki z nich powstają?
9. **Okrąg z sześciu odcinków.** Zmień `CIRCLE_SEGMENTS` na 6. Ile punktów i ile indeksów ma teraz siatka okręgu? Wypisz wszystkie indeksy. Który z nich zmieniłby się, gdyby usunąć `% CIRCLE_SEGMENTS`, i co by wtedy czytała karta (nie uruchamiaj tej wersji, wystarczy odpowiedź na kartce)?

Ćwiczenia o kroku, przesunięciu, rozmiarze bufora i przenoszeniu są w [`buffers-vao.md`](buffers-vao.md), sekcja 8, a ćwiczenia na klasie `Mesh` w [`mesh.md`](mesh.md), sekcja 8.

## 9. Pytania kontrolne

1. **Dlaczego bufor indeksów trzeba tworzyć po związaniu VAO?**
   Bo wiązanie `GL_ELEMENT_ARRAY_BUFFER` jest częścią stanu bieżącego VAO. Konstruktor `Buffer` wiąże bufor, żeby go wypełnić, i to wiązanie zostaje zapisane w VAO, który jest wtedy bieżący. Bez VAO bufor indeksów nie zostaje przypisany do niczego. Dlatego konstruktor `VertexArray` od razu wiąże nowy VAO, a pole `m_vertexArray` stoi w `gfx::Mesh` przed polem `m_indexBuffer`.

2. **Po co są indeksy i ile pamięci oszczędzają na płytce podłogi?**
   Żeby wspólny róg kilku trójkątów był w buforze raz. Płytka: 6 wierzchołków po 44 bajty to 264 bajty, a 4 wierzchołki i 6 indeksów po 4 bajty to 200 bajtów. Zysk rośnie z rozmiarem wierzchołka i liczbą wspólnych rogów.

3. **Czym różni się `glDrawArrays` od `glDrawElements` i co znaczy `count` w każdej z nich?**
   Pierwsza bierze wierzchołki kolejno z buforów i `count` to liczba wierzchołków. Druga bierze je w kolejności indeksów z EBO bieżącego VAO i `count` to liczba indeksów. Dla jednego trójkąta w obu przypadkach 3. Projekt rysuje wszystko przez `glDrawElements`, z jednego miejsca: `Mesh::draw`.

4. **W jakiej przestrzeni są pozycje w plikach modeli i jak nawinięte są trójkąty?**
   W przestrzeni lokalnej modelu: płytka to kwadrat od -1 do 1 w osiach X i Z na wysokości 0, ściana leży wzdłuż osi X z początkiem układu w środku podstawy. Na ekran przenoszą je macierze model, view i projection w shaderze wierzchołków. Trójkąty płytki są nawinięte przeciwnie do ruchu wskazówek zegara dla patrzącego z góry: iloczyn wektorowy ich krawędzi wskazuje w +Y.

5. **Plik OBJ ma 24 pozycje ściany, a w buforze jest 60 wierzchołków. Dlaczego?**
   Indeks OpenGL wybiera cały wierzchołek, czyli pozycję razem z normalną i współrzędną tekstury. Róg należy do kilku ścian o różnych normalnych, więc jest kilkoma różnymi wierzchołkami. Loader tworzy nowy wierzchołek dla każdej różnej trójki pozycja/uv/normalna z linii `f`, a takich trójek jest 60.

6. **Dlaczego sześcian z krawędzi ma tylko 8 wierzchołków?**
   Bo linie są rysowane jednym kolorem z uniformu i jego wierzchołek to sama pozycja. Nie ma atrybutu, który różniłby się między krawędziami wychodzącymi z jednego narożnika, więc narożnik jest jednym wierzchołkiem, używanym przez trzy krawędzie.

7. **Jak czytać tablicę `UNIT_CUBE_EDGES`?**
   Parami: każde dwa kolejne indeksy to jeden odcinek (`GL_LINES`). Pierwszy wiersz to cztery krawędzie dolnej ściany, drugi cztery krawędzie górnej, trzeci cztery krawędzie pionowe, razem 12 krawędzi i 24 indeksy.

8. **Po co w `unitCircleLines` reszta z dzielenia?**
   Odcinek numer `i` łączy punkt `i` z punktem `i + 1`. Dla ostatniego odcinka `i + 1` to 32, a punktów jest 32, o numerach od 0 do 31. Reszta z dzielenia przez 32 zamienia 32 na 0, więc ostatni odcinek zamyka okrąg, a indeks nie wychodzi poza bufor wierzchołków.

9. **Dlaczego w kodzie rysującym nie ma wiązania bufora wierzchołków ani bufora indeksów?**
   Bo VAO zapamiętał bufor wierzchołków dla każdego atrybutu w chwili `setFloatAttribute`, a bufor indeksów w chwili jego utworzenia. Do rysowania wystarcza związanie VAO, które robi `Mesh::draw`. Wiązanie `GL_ARRAY_BUFFER` nie ma wpływu na wywołanie rysujące.

10. **Co znaczą argumenty `glDrawElements` w `Mesh::draw`?**
    Prymityw (`GL_TRIANGLES`: każde trzy indeksy to trójkąt, `GL_LINES`: każde dwa to odcinek), liczba indeksów do narysowania, typ jednego indeksu (`GL_UNSIGNED_INT`, zgodny z `std::uint32_t`) i przesunięcie pierwszego indeksu w buforze indeksów w bajtach, zapisane jako wskaźnik z powodów historycznych: 0 znaczy początek bufora.

11. **Co by się stało, gdyby przy rysowaniu ściany zamiast 90 podać 60, a co gdyby 180?**
    Przy 60 rysowanych jest pierwszych 20 trójkątów: ściana bez części powierzchni, bez błędu. Liczbę 180 `Mesh::draw` odrzuca, bo zakres wystaje poza bufor indeksów, i nie rysuje nic. Samo `glDrawElements` z taką liczbą czytałoby indeksy spoza bufora: wynik nieokreślony.

12. **Jak podzielona jest płytka na trójkąty i dlaczego oba mają ten sam kierunek?**
    Indeksy 0, 1, 2 oraz 0, 3, 1: dwa trójkąty ze wspólną przekątną między wierzchołkami 0 i 1. Oba obiegają płytkę w tę samą stronę, co widać po iloczynie wektorowym krawędzi: w obu przypadkach (0, 4, 0).

13. **Ile wywołań `glDrawElements` wykonuje klatka i skąd ta liczba?**
    Jedno na każdą część każdego obiektu. Labirynt domyślny to 100 płytek, 121 ścian i 121 słupków, czyli 342. Na początku rundy dochodzi brama i 13 kryształów, razem 356. Gdy panel Collision włączy linie, dochodzi jedno na pudełko i trzy na kulę.

14. **Co zastąpiło kostkę z M1 jako przykład rysowania z indeksami?**
    Trzy rzeczy. Modele z plików OBJ, z płytką podłogi jako najmniejszym przykładem indeksów trójkątów. Sześcian z krawędzi w `ColliderLines.cpp` jako dane wierzchołków i indeksów wpisane ręcznie. Okrąg jako indeksy policzone w pętli. Wszystkie rysuje `gfx::Mesh`, która ma w środku te same trzy obiekty co dawna kostka: VAO, bufor wierzchołków i bufor indeksów.

Pytania o bufory, VAO i klasy `Buffer` oraz `VertexArray` są w [`buffers-vao.md`](buffers-vao.md), sekcja 9, a o strukturę `Vertex` i klasę `Mesh` w [`mesh.md`](mesh.md), sekcja 9.

## 10. Źródła

- LearnOpenGL, rozdział "Hello Triangle" (<https://learnopengl.com/Getting-started/Hello-Triangle>): EBO.
- docs.gl (<https://docs.gl>), strony dla OpenGL 4: `glDrawArrays`, `glDrawElements`.
- Khronos OpenGL Wiki: "Vertex Specification" (<https://www.khronos.org/opengl/wiki/Vertex_Specification>, wiązanie bufora indeksów), "Face Culling" (<https://www.khronos.org/opengl/wiki/Face_Culling>).
- Dokumenty w tym repozytorium: [`README.md`](README.md) (droga jednej klatki), [`buffers-vao.md`](buffers-vao.md) (bufory, VAO i obie klasy), [`mesh.md`](mesh.md) (struktura wierzchołka i klasa `Mesh`), [`shaders.md`](shaders.md), [`uniforms.md`](uniforms.md), [`../assets/obj-loader.md`](../assets/obj-loader.md) (skąd biorą się wierzchołki i indeksy modeli), [`../scene/collision.md`](../scene/collision.md) (do czego służą sześcian i okrąg), [`../core/gl-check.md`](../core/gl-check.md).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o tablicach wierzchołków i obiektach buforowych).
- "OpenGL. Księga eksperta" (rozdziały o buforach wierzchołków i tablicach wierzchołków).
