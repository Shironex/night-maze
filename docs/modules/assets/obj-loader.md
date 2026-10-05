# Moduł assets: wczytywanie modeli OBJ

Kamień milowy: M2 + M3. Temat wykładu: 4 (Wczytywanie OBJ).
Kod: [`src/assets/ObjLoader.hpp`](../../../src/assets/ObjLoader.hpp), [`src/assets/ObjLoader.cpp`](../../../src/assets/ObjLoader.cpp), testy w [`tests/ObjLoaderTests.cpp`](../../../tests/ObjLoaderTests.cpp), pliki wejściowe w [`assets/models/`](../../../assets/models/).

Część modułu `assets`. Wstęp do modułu jest w [`README.md`](README.md). Skąd biorą się pliki `.obj` i `.mtl` i jakie mają konwencje, opisuje [`../../guides/blender.md`](../../guides/blender.md). Dokąd trafia wynik, opisują [`../gfx/mesh.md`](../gfx/mesh.md) (siatka na karcie) i [`asset-cache.md`](asset-cache.md) (kto woła loader i co robi z materiałami).

**Stan.** Loader jest częścią biblioteki `engine` i ma 18 przypadków testowych, w tym wczytanie trzech prawdziwych modeli gry. Program go woła: `assets::AssetCache::model` wczytuje nim przy starcie trzy modele labiryntu (płytkę podłogi, ścianę i słupek), tworzy z wyniku siatki `gfx::Mesh` i tekstury, a `game::MazeRenderer` rysuje je shaderami `textured.vert` i `textured.frag` (sekcje 3 i 4). Wczytane modele pokazuje panel Assets (sekcja 6). Parser jest napisany ręcznie, bez biblioteki Assimp ani żadnej innej: zrozumienie formatu jest celem tego tematu.

## 1. Po co to jest

Kostka z tematu 2 ma 24 wierzchołki wpisane ręcznie w kod. Dla ściany z cokołem i nakrywą (60 wierzchołków) to już byłoby pisanie setek liczb, a każda zmiana kształtu wymagałaby przeliczania ich od nowa. Modele powstają więc w programie do modelowania (u mnie: ze skryptów Blendera) i są zapisywane do **pliku**, a gra czyta ten plik przy starcie.

Loader robi jedną rzecz: zamienia tekst pliku OBJ (i towarzyszącego pliku MTL) na dane w pamięci procesora:

- tablicę wierzchołków `gfx::Vertex` (pozycja, normalna, współrzędna tekstury),
- tablicę indeksów, po trzy na trójkąt,
- listę części: który zakres indeksów ma który materiał,
- listę materiałów: nazwa, kolor, ścieżka do pliku tekstury.

Nie tworzy żadnego obiektu OpenGL i nie wczytuje obrazów. Dzięki temu działa bez okna i da się go sprawdzić testami jednostkowymi.

## 2. Teoria

### 2.1 Co musi opisać plik z modelem

Model to **siatka trójkątów** (triangle mesh). Żeby ją narysować z teksturą, trzeba znać:

| Dane | Po co |
|---|---|
| pozycje punktów | kształt |
| które punkty tworzą które trójkąty, w jakiej kolejności | powierzchnia i jej strona przednia |
| normalne | oświetlenie (tematy 6 i 7) |
| współrzędne tekstury | który fragment obrazu leży na którym trójkącie (temat 5) |
| materiał | jaka tekstura i jaki kolor |

Formatów jest wiele (OBJ, glTF, FBX, PLY). **Wavefront OBJ** jest z nich najprostszy: zwykły tekst, jedna informacja w jednej linii, bez animacji, bez hierarchii, bez kamer. Da się go przeczytać w edytorze i napisać do niego parser w jeden wieczór, dlatego jest tematem wykładu.

### 2.2 Format OBJ linia po linii

Cały plik [`assets/models/floor_tile.obj`](../../../assets/models/floor_tile.obj), płyta podłogi z dwóch trójkątów:

```text
# Blender 5.2.1 LTS
# www.blender.org
mtllib floor_tile.mtl
o floor_tile
v -1.000000 0.000000 1.000000
v 1.000000 0.000000 1.000000
v 1.000000 0.000000 -1.000000
v -1.000000 0.000000 -1.000000
vn -0.0000 1.0000 -0.0000
vt 0.500000 -0.500000
vt -0.500000 0.500000
vt -0.500000 -0.500000
vt 0.500000 0.500000
s 0
usemtl floor_stone
f 2/1/1 4/2/1 1/3/1
f 2/1/1 3/4/1 4/2/1
```

Każda linia zaczyna się **słowem kluczowym** (keyword), po którym idą pola oddzielone spacjami.

| Linia | Znaczenie | Co robi z nią parser |
|---|---|---|
| `# ...` | komentarz, do końca linii | pomija |
| `mtllib floor_tile.mtl` | nazwa pliku z materiałami, względem katalogu pliku OBJ | zapamiętuje nazwę. Plik otwiera dopiero `loadObj` |
| `o floor_tile` | początek obiektu o tej nazwie | pomija: cały plik staje się jedną siatką |
| `v x y z` | pozycja (vertex), trzy liczby | dopisuje do listy pozycji |
| `vn x y z` | normalna (vertex normal) | dopisuje do listy normalnych |
| `vt u v` | współrzędna tekstury (vertex texture) | dopisuje do listy uv |
| `s 0` | grupa wygładzania (smoothing group) wyłączona | pomija: normalne biorę z linii `vn` takie, jakie są |
| `usemtl floor_stone` | materiał dla wszystkich następnych linii `f` | zapamiętuje jako bieżący materiał |
| `f a/b/c a/b/c a/b/c` | ściana (face): narożniki, każdy jako indeksy `pozycja/uv/normalna` | buduje wierzchołki i trójkąty (sekcje 2.4 i 2.6) |

Trzy rzeczy, które trzeba zapamiętać o liniach `f`:

- **Indeksy liczą się od 1.** `2/1/1` to druga pozycja, pierwsza para uv, pierwsza normalna. W C++ tablice liczą się od 0, więc parser odejmuje 1.
- **Kolejność w narożniku to pozycja, uv, normalna**, choć w tym pliku linie `vn` stoją przed liniami `vt`. Kolejność linii w pliku nie ma związku z kolejnością pól w narożniku.
- **Narożnik może mieć mniej pól.** Format dopuszcza cztery postacie:

| Postać | Co zawiera | Kto tak pisze |
|---|---|---|
| `a/b/c` | pozycja, uv, normalna | Blender z moimi opcjami eksportu |
| `a//c` | pozycja i normalna, bez uv (dwa ukośniki pod rząd) | modele bez tekstury |
| `a/b` | pozycja i uv, bez normalnej | modele bez oświetlenia |
| `a` | sama pozycja | najprostsze pliki |

Parser obsługuje wszystkie cztery. Brakujące uv albo normalna stają się zerami.

Format ma więcej słów kluczowych (krzywe, powierzchnie, linie `l`, punkty `p`, grupy `g`). Tych, których nie potrzebuję, parser nie rozumie i pomija (sekcja 5.8).

### 2.3 Format MTL

Plik OBJ nie zawiera kolorów ani tekstur. Odsyła do **biblioteki materiałów** (material library), osobnego pliku tekstowego. Cały plik [`assets/models/wall_straight.mtl`](../../../assets/models/wall_straight.mtl):

```text
# Blender 5.2.1 LTS MTL File: 'None'
# www.blender.org

newmtl wall_stone
Ns 250.000000
Ka 1.000000 1.000000 1.000000
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 2
Kd 1.000000 1.000000 1.000000
map_Kd ../textures/wall_stone.png
```

| Linia | Znaczenie | Parser |
|---|---|---|
| `newmtl wall_stone` | początek materiału o tej nazwie. Następne linie go opisują, aż do kolejnego `newmtl` | tworzy nowy materiał |
| `Kd r g b` | kolor rozproszony (diffuse), trzy liczby od 0 do 1 | zapisuje w `diffuseColor` |
| `map_Kd ścieżka` | tekstura koloru rozproszonego, ścieżka względem katalogu pliku MTL | zapisuje w `diffuseTexture` |
| `Ns`, `Ka`, `Ks`, `Ke`, `Ni`, `d`, `illum` | połysk, kolor otoczenia, odbłysk, emisja, załamanie, przezroczystość, model oświetlenia | pomija. Oświetlenie z M4 ich nie potrzebuje: jasność i wykładnik odblasku są wspólne dla całego labiryntu i pochodzą z ustawień `game::LightingSettings` (panel Lights), a nie z pliku MTL |

Powiązanie między plikami jest przez **nazwę**: linia `usemtl wall_stone` w pliku OBJ wskazuje materiał `newmtl wall_stone` w pliku MTL.

Dwie ścieżki, dwa punkty odniesienia:

```text
assets/models/wall_straight.obj     mtllib wall_straight.mtl          -> assets/models/wall_straight.mtl
assets/models/wall_straight.mtl     map_Kd ../textures/wall_stone.png -> assets/textures/wall_stone.png
```

Nazwa z `mtllib` jest liczona od katalogu pliku OBJ, a ścieżka z `map_Kd` od katalogu pliku MTL. Nigdy od katalogu roboczego programu.

### 2.4 Trzy listy indeksów a jeden indeks OpenGL

To jest najważniejsza myśl całego tematu.

Plik OBJ ma **trzy osobne listy**: pozycji, uv i normalnych. Narożnik ściany wybiera po jednym elemencie z każdej, trzema niezależnymi indeksami. Listy mają różne długości: w `wall_straight.obj` są 24 pozycje, 24 pary uv i tylko 6 normalnych (po jednej na kierunek: lewo, prawo, przód, tył, góra, dół). To oszczędny zapis: normalna "w górę" jest w pliku raz, choć używa jej wiele narożników.

OpenGL ma **jeden indeks na wierzchołek**. Indeks w buforze indeksów wybiera cały wierzchołek naraz: pozycję, normalną i uv spod tego samego numeru ([`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md), sekcja 2.1). Nie da się powiedzieć karcie "pozycja numer 5, ale normalna numer 2".

Parser musi więc **przepakować** dane: dla każdej różnej trójki `(pozycja, uv, normalna)` użytej w pliku utworzyć jeden wierzchołek wyjściowy, złożony z trzech list. Dwa narożniki o tej samej trójce dostają ten sam wierzchołek, a narożniki różniące się choćby jednym indeksem dostają różne.

Żeby wiedzieć, czy trójka już była, parser prowadzi **mapę**: klucz to trójka indeksów, wartość to numer wierzchołka wyjściowego. Przykład na płycie podłogi (dwie linie `f`, sześć narożników):

| Narożnik w pliku | Trójka (pozycja, uv, normalna) | Czy jest w mapie | Wierzchołek wyjściowy | Mapa po tym kroku |
|---|---|---|---|---|
| `2/1/1` | (2, 1, 1) | nie | nowy: **0** | (2,1,1) → 0 |
| `4/2/1` | (4, 2, 1) | nie | nowy: **1** | + (4,2,1) → 1 |
| `1/3/1` | (1, 3, 1) | nie | nowy: **2** | + (1,3,1) → 2 |
| `2/1/1` | (2, 1, 1) | **tak** | istniejący: **0** | bez zmian |
| `3/4/1` | (3, 4, 1) | nie | nowy: **3** | + (3,4,1) → 3 |
| `4/2/1` | (4, 2, 1) | **tak** | istniejący: **1** | bez zmian |

Wynik: 4 wierzchołki i indeksy `0, 1, 2, 0, 3, 1`. Sześć narożników, cztery różne trójki. Wierzchołek 0 to pozycja `(1, 0, 1)` (druga linia `v`), uv `(0.5, -0.5)` (pierwsza linia `vt`) i normalna `(0, 1, 0)`.

(W tabeli indeksy są zapisane tak jak w pliku, od 1. W kodzie kluczem mapy są indeksy już przeliczone na liczone od 0: sekcja 5.5.)

Liczby dla trzech modeli gry, policzone osobnym skryptem z samych linii `f` i potwierdzone testami:

| Model | Linie `v` | Linie `vt` | Linie `vn` | Narożniki (3 na trójkąt) | Różne trójki, czyli wierzchołki wyjściowe |
|---|---|---|---|---|---|
| `floor_tile.obj` | 4 | 4 | 1 | 6 | 4 |
| `wall_straight.obj` | 24 | 24 | 6 | 90 | 60 |
| `wall_pillar.obj` | 24 | 16 | 6 | 90 | 60 |

Ściana ma 24 pozycje, ale 60 wierzchołków: róg prostopadłościanu należy do kilku ścian o różnych normalnych, więc ta sama pozycja występuje w kilku trójkach. To samo zjawisko co 24 wierzchołki kostki przy 8 rogach. Odwrotnie też bywa: 90 narożników daje tylko 60 wierzchołków, bo dwa trójkąty jednego prostokąta mają dwa narożniki wspólne.

### 2.5 Indeksy ujemne

Specyfikacja OBJ dopuszcza indeksy ujemne, czyli **względne**: `-1` to element zdefiniowany jako ostatni **przed tą linią**, `-2` przedostatni i tak dalej. Pozwala to dopisywać fragmenty do pliku bez przeliczania numerów:

```text
v 0 0 0
v 1 0 0
v 0 1 0
f -3 -2 -1      # to samo co f 1 2 3
v 0 0 5
v 1 0 5
v 0 1 5
f -3 -2 -1      # to samo co f 4 5 6: lista urosła
```

Blender ich nie pisze, ale inne narzędzia tak. Ważne jest "przed tą linią": liczy się długość listy w chwili czytania ściany, a nie długość na końcu pliku. Przeliczenie jest proste: dla listy o `n` elementach indeks `-k` to element o numerze `n - k`, licząc od 0.

### 2.6 Wielokąty i wachlarz trójkątów

Linia `f` może mieć więcej niż trzy narożniki: czworokąt (quad), pięciokąt. OpenGL w profilu Core rysuje tylko trójkąty (prymitywu `GL_QUADS` tam nie ma), więc wielokąt trzeba podzielić: to **triangulacja** (triangulation).

Najprostsza metoda to **wachlarz** (triangle fan): pierwszy narożnik łączę z każdą kolejną parą sąsiadów.

```text
wielokąt 0 1 2 3 4           trójkąty: (0,1,2)  (0,2,3)  (0,3,4)

      3
    /   \                    każdy trójkąt zaczyna się w narożniku 0,
  4       2                  a dwa pozostałe to sąsiedzi na obwodzie
  |       |
  0 ----- 1
```

Wielokąt o `n` narożnikach daje `n - 2` trójkątów. Wszystkie obiegają swoje narożniki w tę samą stronę co cały wielokąt, więc kierunek nawijania się nie zmienia. Wachlarz jest poprawny dla wielokątów **wypukłych** (convex) i płaskich. Dla wklęsłego (na przykład w kształcie litery L) część trójkątów wyszłaby poza kształt: taki przypadek wymaga algorytmu "obcinania uszu" (ear clipping), którego nie implementuję.

Moje modele są eksportowane już jako trójkąty (opcja `export_triangulated_mesh`), więc wachlarz wykonuje się dla nich "na pusto": trójkąt przechodzi przez pętlę raz. Obsługa czworokątów jest po to, żeby plik z innego narzędzia nie kończył się błędem.

### 2.7 Kierunek nawijania i normalne

Plik OBJ podaje narożniki ściany w kolejności **przeciwnej do ruchu wskazówek zegara**, gdy patrzę na ścianę z zewnątrz. To ta sama konwencja co domyślny "przód" w OpenGL ([`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md), sekcja 2.2), więc parser zapisuje indeksy w kolejności z pliku i niczego nie odwraca.

Normalna jest w pliku osobną informacją i **nie wynika z kolejności narożników**: plik może podać dowolną. W poprawnym modelu zgadza się z nimi: iloczyn wektorowy dwóch krawędzi trójkąta wskazuje w tę samą stronę. Test płyty podłogi sprawdza dokładnie to (sekcja 5.9).

Moje modele mają **cieniowanie płaskie** (flat shading): wszystkie trzy narożniki trójkąta mają ten sam indeks normalnej. Przy cieniowaniu gładkim (smooth shading) każdy narożnik miałby własną, uśrednioną normalną. Dla parsera to bez różnicy: bierze z listy to, co wskazuje indeks. Nie liczy normalnych sam, nie uśrednia ich, nie normalizuje i nie przetwarza grup wygładzania z linii `s`. Gdy plik nie ma linii `vn`, wierzchołki mają normalną zerową.

### 2.8 Układ współrzędnych i związek z eksportem z Blendera

Format OBJ nie mówi, która oś jest górą ani jaka jest jednostka. To umowa między eksporterem a programem. Loader **niczego nie przelicza**: pozycje z pliku trafiają do wierzchołków bez zmian. Cała zgodność jest ustalona po stronie eksportu ([`../../guides/blender.md`](../../guides/blender.md), sekcje 2 i 4):

| Ustalenie | Wartość | Kto za to odpowiada |
|---|---|---|
| góra | +Y | opcja eksportu `up_axis='Y'` |
| przód | -Z | opcja eksportu `forward_axis='NEGATIVE_Z'` |
| jednostka | 1 to 1 metr | `global_scale=1.0` |
| początek układu modelu | środek podstawy, podłoga w y = 0 | skrypt modelu |
| `v = 0` w teksturze | **dolny** wiersz obrazu | konwencja OBJ, taka sama jak w OpenGL |

Ostatni wiersz ma konsekwencję poza loaderem modeli. Pliki PNG zapisują obraz od **górnego** wiersza, a współrzędna `v` rośnie od dołu. Odwrócenie trzeba zrobić w jednym miejscu, przy wczytywaniu obrazu, a nie w loaderze OBJ: to sprawa loadera obrazów ([`images.md`](images.md)). Loader OBJ zostawia `v` takie, jakie jest w pliku.

### 2.9 Liczby w tekście i ustawienia regionalne

Plik zawiera liczby jako tekst: `-0.140000`. Zamiana tekstu na `float` wygląda na trywialną, ale ma pułapkę: **ustawienia regionalne** (locale). W Polsce separatorem dziesiętnym jest przecinek. Funkcje `atof`, `strtof`, `std::stof` i `sscanf` czytają liczbę według **globalnego locale programu**. Gdy jest nim polskie, oczekują `0,14`, a tekst `0.14` czytają jako `0` i zatrzymują się na kropce. Program sam z siebie startuje z locale "C" (z kropką), ale wystarczy, że jakaś biblioteka wywoła `setlocale(LC_ALL, "")`, i model nagle staje się płaski.

Plik modelu ma zawsze kropkę, niezależnie od komputera. Parser musi więc czytać liczby **niezależnie od locale**. Jak to robi i dlaczego nie przez `std::from_chars`, mówi sekcja 5.4.

## 3. Jak to działa w OpenGL

Loader nie woła żadnej funkcji `gl*`. `ObjLoader.hpp` i `ObjLoader.cpp` nie dołączają GLAD. Wynik jest zaprojektowany tak, żeby dało się go oddać karcie bez żadnej przeróbki:

```mermaid
flowchart LR
    F["wall_straight.obj<br/>wall_straight.mtl"] -->|"assets::loadObj"| M["assets::ObjModel<br/>vertices, indices,<br/>parts, materials"]
    M -->|"vertices, indices"| G["gfx::Mesh<br/>VAO, VBO, EBO"]
    M -->|"materials[i].diffuseTexture"| T["assets::loadImage,<br/>potem gfx::Texture2D"]
    M -->|"parts[i].firstIndex,<br/>parts[i].indexCount"| D["Mesh::draw(first, count)"]
    G --> D
    T --> D
```

| Pole `ObjModel` | Dokąd trafia w OpenGL |
|---|---|
| `vertices` (`std::vector<gfx::Vertex>`) | bufor wierzchołków: `glBufferData(GL_ARRAY_BUFFER, ...)`, 32 bajty na wierzchołek |
| `indices` (`std::vector<std::uint32_t>`) | bufor indeksów: `glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...)`, rysowany jako `GL_UNSIGNED_INT` |
| `parts[i].firstIndex`, `parts[i].indexCount` | argumenty `glDrawElements`: liczba indeksów i przesunięcie (`firstIndex * 4` bajtów) |
| `materials[i].diffuseTexture` | ścieżka pliku, z którego powstaje tekstura `gfx::Texture2D`, wiązana z jednostką 0 przed narysowaniem części |
| `materials[i].diffuseColor` | wartość uniformu `uTint` w `textured.frag` |

Całe to połączenie jest w jednej funkcji, `assets::AssetCache::model` ([`asset-cache.md`](asset-cache.md), sekcja 5). Loader jest tam wołany tak:

```cpp
    // loadObj logs its own error.
    ObjModel source;
    std::string error;
    if (!loadObj(key, source, error)) {
        m_failedPaths.push_back(key);
        return nullptr;
    }
```

Dwa pierwsze wiersze tabeli to jedno wywołanie konstruktora, `gfx::Mesh(source.vertices, source.indices)` ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5.7): `std::vector` zamienia się na `std::span` sam. Części z `source.parts` są przepisywane do struktur `assets::ModelPart`, każda z kolorem i teksturą swojego materiału, a `ObjModel` ginie na końcu funkcji: karta ma już własną kopię danych. Trzy rzeczy, które pamięć podręczna dokłada do wyniku loadera: model bez żadnej ściany jest odrzucany z własnym komunikatem, część bez tekstury albo z teksturą, której nie dało się wczytać, dostaje białą teksturę zastępczą, a plik, który raz się nie wczytał, nie jest czytany ponownie.

## 4. Shadery

Loader nie ma własnego shadera, ale wczytane modele rysuje para [`assets/shaders/textured.vert`](../../../assets/shaders/textured.vert) i [`textured.frag`](../../../assets/shaders/textured.frag), opisana linia po linii w [`../gfx/textures.md`](../gfx/textures.md) (sekcja 4). Każde pole wyniku loadera ma w niej swoje miejsce:

| Dane z pliku | Pole wyniku loadera | Gdzie w shaderze |
|---|---|---|
| linie `v` | `Vertex::position` | `layout(location = 0) in vec3 aPosition;` |
| linie `vn` | `Vertex::normal` | `layout(location = 1) in vec3 aNormal;` |
| linie `vt` | `Vertex::uv` | `layout(location = 2) in vec2 aUv;` |
| `Kd` z pliku MTL | `ObjMaterial::diffuseColor` | `uniform vec3 uTint;` |
| `map_Kd` z pliku MTL | `ObjMaterial::diffuseTexture` | `uniform sampler2D uTexture;` (tekstura związana z jednostką, której numer jest w samplerze) |

Numery atrybutów są ustalone w `gfx/Vertex.hpp`: pozycja 0, normalna 1, uv 2 ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 2.4). Pierwsza para shaderów projektu, `basic.vert` i `basic.frag`, do modeli się nie nadaje: czyta pozycję i **kolor**, a nie pozycję, normalną i uv.

Od M4 normalne z pliku **służą do oświetlenia**. Programy `lit` i `gouraud` deklarują te same trzy atrybuty co `textured.vert` i liczą z normalnej, ile światła pada na powierzchnię ([`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), wzory w [`../scene/lights.md`](../scene/lights.md)). Loader nie zmienił się przy tym ani o linię: normalne z linii `vn` były w wierzchołkach od początku. Z tego wynika nowe wymaganie wobec modeli, którego wcześniej nie było widać: normalna musi wskazywać na zewnątrz bryły, bo ściana z odwróconą normalną jest oświetlona od złej strony (długość poprawia sam shader, który normalizuje normalną). Program `textured` nadal umie normalne tylko pokazać jako kolor, w trybie podglądu `Normals as colour`. Drugi tryb, `UVs as colour`, pokazuje tak samo współrzędne z linii `vt`. Oba są sposobem na obejrzenie na ekranie tego, co loader wczytał.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/assets/ObjLoader.hpp`](../../../src/assets/ObjLoader.hpp) | struktury `ObjPart`, `ObjMaterial`, `ObjModel`, deklaracje `parseObj`, `parseMtl`, `loadObj` |
| [`src/assets/ObjLoader.cpp`](../../../src/assets/ObjLoader.cpp) | stałe, funkcje pomocnicze do cięcia tekstu i czytania liczb, struktura `ObjParser`, definicje trzech funkcji publicznych |
| [`tests/ObjLoaderTests.cpp`](../../../tests/ObjLoaderTests.cpp) | 18 przypadków testowych |
| [`src/gfx/Vertex.hpp`](../../../src/gfx/Vertex.hpp) | struktura wierzchołka, którą loader wypełnia ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5.2) |

Pliki `src/assets/ObjLoader.*` są na liście źródeł biblioteki `engine` w [`CMakeLists.txt`](../../../CMakeLists.txt). Dołączane nagłówki projektu: `gfx/Vertex.hpp` w nagłówku, `core/Log.hpp` i `core/Paths.hpp` w pliku `.cpp`. Nic z GLAD ani GLFW.

Podział na trzy funkcje ma jeden powód: **testowalność**.

| Funkcja | Wejście | Czyta pliki | Loguje |
|---|---|---|---|
| `parseObj` | tekst OBJ (`std::string_view`) | nie | nie |
| `parseMtl` | tekst MTL (`std::string_view`) | nie | nie |
| `loadObj` | ścieżka pliku OBJ | tak: plik OBJ i pliki MTL | tak, raz przy błędzie |

Dwie pierwsze są **czystymi funkcjami**: ten sam tekst daje zawsze ten sam wynik i nic poza wynikiem się nie zmienia. Test podaje im napis wpisany w kod i sprawdza wynik, bez plików tymczasowych.

### 5.2 Struktury wyniku

```cpp
struct ObjPart {
    /// Name given by the usemtl line. Empty for faces that come before any usemtl line.
    std::string material;
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
};
```

Część (part) to ciąg trójkątów o jednym materiale: `indexCount` indeksów, zaczynając od indeksu numer `firstIndex`. Te dwie liczby to wprost argumenty `gfx::Mesh::draw(firstIndex, indexCount)`.

```cpp
struct ObjMaterial {
    std::string name;
    glm::vec3 diffuseColor{1.0F};
    std::filesystem::path diffuseTexture;
};
```

(Komentarze z pliku są tu pominięte.)

- `diffuseColor` ma domyślnie wartość białą `(1, 1, 1)`. Blender pomija linię `Kd`, gdy kolor pochodzi z tekstury ([`../../guides/blender.md`](../../guides/blender.md), sekcja 4). Biały kolor pomnożony przez teksturę zostawia teksturę bez zmian, więc materiał bez `Kd` wygląda rozsądnie.
- `diffuseTexture` jest pusta, gdy materiał nie ma linii `map_Kd`. Sprawdza się to przez `diffuseTexture.empty()`. Po `parseMtl` zawiera ścieżkę tak, jak stoi w pliku (`../textures/wall_stone.png`). Po `loadObj` zawiera ścieżkę do pliku obrazu, liczoną od katalogu pliku MTL i uporządkowaną (sekcja 5.7).

```cpp
struct ObjModel {
    std::vector<gfx::Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<ObjPart> parts;
    std::vector<std::string> materialLibraries;
    std::vector<ObjMaterial> materials;
    std::size_t unknownLineCount = 0;
};
```

(Komentarze z pliku są tu pominięte.)

| Pole | Kto wypełnia | Zawartość |
|---|---|---|
| `vertices` | `parseObj` | jeden wierzchołek na każdą różną trójkę indeksów, w kolejności pierwszego wystąpienia |
| `indices` | `parseObj` | trzy indeksy na trójkąt, w kolejności z pliku |
| `parts` | `parseObj` | zakresy indeksów według materiału, w kolejności z pliku. Razem pokrywają wszystkie indeksy |
| `materialLibraries` | `parseObj` | nazwy z linii `mtllib`, tak jak w pliku |
| `materials` | `loadObj` | materiały ze wszystkich bibliotek. Po samym `parseObj` lista jest pusta, bo ta funkcja nie otwiera plików |
| `unknownLineCount` | `parseObj` | liczba linii o nieznanym słowie kluczowym |

Wszystkie trzy struktury to zwykłe dane z publicznymi polami, jak struktury warstwy `scene`: kopiują się, nie mają związku z OpenGL.

### 5.3 Sposób zgłaszania błędów

Wszystkie trzy funkcje mają ten sam kształt:

```cpp
bool parseObj(std::string_view text, ObjModel& model, std::string& error);
bool parseMtl(std::string_view text, std::vector<ObjMaterial>& materials, std::string& error);
bool loadObj(const std::filesystem::path& path, ObjModel& model, std::string& error);
```

| Wynik | `model` (albo `materials`) | `error` |
|---|---|---|
| `true` | wypełniony | bez zmian |
| `false` | **bez zmian**: taki, jaki był przed wywołaniem | powód, dla parserów zaczynający się od `line N: ` |

To ten sam styl co w `gfx::Shader` ([`../gfx/shader-class.md`](../gfx/shader-class.md)): żadnych wyjątków, wynik do sprawdzenia i tekst powodu. Funkcje pomocnicze `Shader.cpp` mają identyczny kształt (`compileShader(..., std::string& error)`). Zły plik modelu nie jest sytuacją wyjątkową: zdarza się przy każdej pomyłce w eksporcie, a program ma wtedy wypisać, co jest nie tak, i działać dalej.

"Bez zmian przy błędzie" jest zrobione tak samo jak w `Shader::reload`: wynik powstaje w zmiennej lokalnej i dopiero na samym końcu, gdy wszystko się udało, jest przenoszony do parametru wyjściowego (`model = std::move(...)`).

Parser zatrzymuje się na **pierwszej** złej linii. Numer linii liczy się od 1, jak w edytorze tekstu, i obejmuje linie puste i komentarze.

Jedyny wyjątek, który teoretycznie może wylecieć, to `std::bad_alloc` przy braku pamięci. Tego nie obsługuję nigdzie w projekcie.

### 5.4 Narzędzia do tekstu i liczb

Funkcje pomocnicze stoją w anonimowej przestrzeni nazw w `ObjLoader.cpp`. Wszystkie pracują na `std::string_view`: to **widok** na cudzy tekst (wskaźnik i długość), bez kopiowania. Odcinanie początku widoku (`remove_prefix`) przesuwa wskaźnik i nie rusza samego tekstu.

**Stałe:**

```cpp
constexpr std::string_view BLANKS = " \t\r";
constexpr char COMMENT_START = '#';
constexpr char INDEX_SEPARATOR = '/';
constexpr std::size_t MAX_CORNER_FIELDS = 3;
constexpr std::size_t COLOR_COMPONENTS = 3;
constexpr std::size_t MIN_FACE_CORNERS = 3;
constexpr std::uint32_t NO_INDEX = std::numeric_limits<std::uint32_t>::max();
```

(Komentarze z pliku są tu pominięte.)

`BLANKS` to znaki rozdzielające pola: spacja, tabulator i `\r`. Znak `\r` (carriage return) jest tu z powodu końców linii Windowsa: plik zapisany w tym systemie kończy każdą linię parą `\r\n`. Po cięciu na `\n` na końcu linii zostaje `\r`, który trzeba potraktować jak spację. Bez tego nazwa materiału brzmiałaby `stone\r` i nie pasowała do żadnego `newmtl`. Moje pliki mają same `\n`, ale Git na Windowsie z ustawieniem `core.autocrlf=true` potrafi je zamienić przy pobraniu.

**`takeLine`** odcina od tekstu pierwszą linię:

```cpp
std::string_view takeLine(std::string_view& rest) {
    const std::size_t end = rest.find('\n');
    if (end == std::string_view::npos) {
        // No more line breaks: everything that is left is the last line.
        const std::string_view line = rest;
        rest = {};
        return line;
    }
    const std::string_view line = rest.substr(0, end);
    rest.remove_prefix(end + 1);
    return line;
}
```

`find` zwraca pozycję znaku albo specjalną wartość `npos`, gdy go nie ma. Gałąź `npos` obsługuje **plik bez znaku nowej linii na końcu**: ostatnia linia to po prostu wszystko, co zostało. `rest` jest referencją, więc funkcja zmienia widok wołającego: po wywołaniu `rest` zaczyna się od następnej linii.

**`withoutComment`** obcina linię na pierwszym znaku `#`. `substr(0, npos)` zwraca całość, więc linia bez komentarza przechodzi bez zmian. Komentarz może więc stać też na końcu linii z danymi.

**`takeToken`** odcina następne pole:

```cpp
std::string_view takeToken(std::string_view& rest) {
    const std::size_t first = rest.find_first_not_of(BLANKS);
    if (first == std::string_view::npos) {
        rest = {};
        return {};
    }
    rest.remove_prefix(first);

    // npos (no blank found) makes substr take everything that is left.
    const std::size_t length = rest.find_first_of(BLANKS);
    const std::string_view token = rest.substr(0, length);
    rest.remove_prefix(token.size());
    return token;
}
```

Najpierw pomija wszystkie znaki z `BLANKS` (dowolnie wiele spacji i tabulatorów, także na początku linii), potem bierze znaki do następnego takiego znaku. Gdy pól już nie ma, zwraca pusty widok. Dzięki temu `v   0\t0  0   ` czyta się tak samo jak `v 0 0 0`.

**`trim`** zwraca tekst bez znaków z `BLANKS` na obu końcach. Służy do nazw, które są "resztą linii" i mogą zawierać spacje w środku: nazwa materiału, nazwa pliku.

**`parseFloat`**: tekst na liczbę, niezależnie od locale (sekcja 2.9).

```cpp
bool parseFloat(std::string_view token, float& value) {
    if (token.empty()) {
        return false;
    }
    std::istringstream stream{std::string(token)};
    stream.imbue(std::locale::classic());
    stream >> value;
    // The whole field must have been used: "1.5abc" is not a number. After a successful
    // read that stops at the end of the text, peek() finds nothing more to read.
    return !stream.fail() && stream.peek() == std::istringstream::traits_type::eof();
}
```

| Linia | Co robi |
|---|---|
| `std::istringstream stream{std::string(token)}` | strumień czytający z kopii pola. Kopia jest potrzebna, bo strumień chce `std::string`, a nie widoku |
| `stream.imbue(std::locale::classic())` | **najważniejsza linia**: ustawia strumieniowi locale "klasyczne", czyli locale języka C z kropką dziesiętną. Każdy strumień ma własne locale, więc globalne ustawienie programu przestaje mieć znaczenie |
| `stream >> value` | czyta liczbę. Rozumie znak, część ułamkową i wykładnik (`1e-3`) |
| `!stream.fail()` | odczyt się udał |
| `stream.peek() == ...eof()` | po liczbie nie zostało nic. Bez tego warunku `1.5abc` i `0,5` (czytane jako `0`) uchodziłyby za liczby |

**Dlaczego nie `std::from_chars`.** Funkcja `std::from_chars` z nagłówka `<charconv>` jest stworzona dokładnie do tego: nie patrzy na locale, nie alokuje pamięci, nie rzuca wyjątków. Dla liczb całkowitych jest wszędzie. Z przeciążeniami dla `float` i `double` jest gorzej: biblioteka standardowa używana na macOS (libc++) ma je dopiero od wersji LLVM 20 (tak podaje jej tabela stanu C++17, wiersz P0067R5), a projekt jest budowany na Macu kompilatorem Apple clang 17. **Nie potwierdziłem, czy biblioteka dostarczana z tym kompilatorem je ma**: ten kod powstał na Windowsie, gdzie MSVC ma oba przeciążenia. Wybrałem więc sposób, który na pewno działa w obu bibliotekach: strumień z klasycznym locale. Kosztem jest szybkość (strumień i kopia napisu na każdą liczbę), bez znaczenia dla modeli o kilkudziesięciu wierzchołkach. Punkt do sprawdzenia jest na liście w [`../../guides/build-macos.md`](../../guides/build-macos.md).

**`parseInteger`**: tu `std::from_chars` wystarcza.

```cpp
bool parseInteger(std::string_view token, long long& value) {
    const char* const begin = token.data();
    const char* const end = begin + token.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    // ec is the default (no error) on success, ptr is where the reading stopped.
    return result.ec == std::errc() && result.ptr == end;
}
```

`std::from_chars` dostaje początek i koniec tekstu oraz zmienną na wynik. Zwraca strukturę z dwoma polami: `ec` (kod błędu: pusty przy sukcesie, `invalid_argument` gdy tekst nie jest liczbą, `result_out_of_range` gdy liczba nie mieści się w typie) i `ptr` (miejsce, w którym skończyła czytać). Warunek `ptr == end` znaczy to samo co `peek() == eof` wyżej: całe pole zostało zużyte, więc `3a` i `3.5` nie są liczbami całkowitymi. Typ `long long` ma znak, bo indeksy mogą być ujemne.

**`takeFloats`** dostaje `std::span<float>`, czyli widok na tablicę liczb, i czyta tyle kolejnych pól, ile elementów ma ten widok. Zwraca `false`, gdy pola brakuje albo nie jest liczbą. Pola po nich zostawia nietknięte: dlatego czwarta współrzędna w linii `v` (albo kolory wierzchołków, które Blender dopisuje przy pewnej opcji eksportu) są ignorowane.

**`lineError`** składa komunikat: `line 12: v needs three numbers`.

### 5.5 Indeks, narożnik i mapa trójek

**`resolveIndex`** zamienia indeks zapisany w pliku na indeks liczony od 0:

```cpp
bool resolveIndex(std::string_view text, std::size_t count, std::uint32_t& index) {
    long long written = 0;
    if (!parseInteger(text, written)) {
        return false;
    }

    // Signed arithmetic, so that a negative index needs no special cases.
    const auto size = static_cast<long long>(count);
    const long long zeroBased = written > 0 ? written - 1 : size + written;
    // written == 0 gives zeroBased == size and is rejected here too: 0 is not a valid
    // index in an OBJ file.
    if (zeroBased < 0 || zeroBased >= size) {
        return false;
    }
    index = static_cast<std::uint32_t>(zeroBased);
    return true;
}
```

`count` to liczba elementów listy **w tej chwili**. Jedna linia obsługuje obie reguły formatu:

| W pliku (`written`) | Lista ma 4 elementy (`size`) | `zeroBased` | Wynik |
|---|---|---|---|
| `1` | | `1 - 1 = 0` | pierwszy element |
| `4` | | `4 - 1 = 3` | ostatni element |
| `5` | | `4`, nie jest mniejsze od `size` | błąd: poza listą |
| `-1` | | `4 + (-1) = 3` | ostatni element |
| `-4` | | `4 + (-4) = 0` | pierwszy element |
| `-5` | | `-1`, ujemne | błąd: przed początkiem |
| `0` | | `4 + 0 = 4`, nie jest mniejsze od `size` | błąd: indeksu 0 nie ma |

Rachunek jest na liczbach ze znakiem, żeby odejmowanie nie mogło się przekręcić. Dopiero sprawdzony wynik jest rzutowany na typ bez znaku.

**Struktura `ObjParser`** trzyma stan czytania jednego pliku:

```cpp
struct ObjParser {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    std::map<CornerKey, std::uint32_t> vertexOfCorner;

    std::string currentMaterial;

    std::vector<std::uint32_t> faceCorners;

    ObjModel model;
```

(Komentarze i deklaracje funkcji są tu pominięte.)

| Pole | Rola |
|---|---|
| `positions`, `uvs`, `normals` | trzy listy z pliku, w kolejności linii `v`, `vt`, `vn`. To dane **wejściowe**: po zakończeniu parsowania są wyrzucane |
| `vertexOfCorner` | mapa z sekcji 2.4: trójka indeksów na numer wierzchołka wyjściowego |
| `currentMaterial` | nazwa z ostatniej linii `usemtl`, pusta przed pierwszą |
| `faceCorners` | numery wierzchołków wyjściowych bieżącej ściany. Pole, a nie zmienna lokalna, żeby jego pamięć była używana ponownie dla każdej ściany |
| `model` | budowany wynik |

Struktura jest lokalna dla pliku `.cpp` (anonimowa przestrzeń nazw) i żyje tylko przez czas jednego wywołania `parseObj`. To nie jest stan globalny.

Klucz mapy:

```cpp
using CornerKey = std::array<std::uint32_t, MAX_CORNER_FIELDS>;
```

Tablica trzech liczb: `{pozycja, uv, normalna}`, już liczonych od 0. `std::map` wymaga, żeby klucze dało się porównywać operatorem `<`, a `std::array` ma go wbudowanego (porównuje element po elemencie, jak słowa w słowniku), więc nie trzeba pisać niczego własnego. `std::unordered_map` byłaby szybsza dla dużych modeli, ale wymagałaby własnej funkcji skrótu dla tablicy. Dla 60 wierzchołków różnicy nie ma, a `std::map` nie wymaga dodatkowego kodu.

Ponieważ kluczem są indeksy **po przeliczeniu**, narożnik `-1` i narożnik `3` (przy trzech pozycjach) dają ten sam klucz i ten sam wierzchołek.

Brak uv albo normalnej oznacza w kluczu stała `NO_INDEX`: największa wartość typu `std::uint32_t` (ponad cztery miliardy), której prawdziwy indeks nie osiągnie. Narożnik `5` i narożnik `5/1` mają więc różne klucze i są różnymi wierzchołkami: pierwszy ma uv zerowe, drugi z pliku.

**`readCorner`** zamienia tekst jednego narożnika na numer wierzchołka wyjściowego. Trzy kroki. Pierwszy: podział na pola przy ukośnikach.

```cpp
    std::array<std::string_view, MAX_CORNER_FIELDS> fields;
    std::size_t fieldCount = 0;
    std::string_view rest = corner;
    while (true) {
        if (fieldCount == MAX_CORNER_FIELDS) {
            message = "face corner '" + std::string(corner) + "' has more than three indices";
            return false;
        }
        const std::size_t separator = rest.find(INDEX_SEPARATOR);
        fields[fieldCount] = rest.substr(0, separator);
        ++fieldCount;
        if (separator == std::string_view::npos) {
            break;
        }
        rest.remove_prefix(separator + 1);
    }
```

| Narożnik | `fields[0]` | `fields[1]` | `fields[2]` |
|---|---|---|---|
| `7/3/2` | `7` | `3` | `2` |
| `7//2` | `7` | pusty | `2` |
| `7/3` | `7` | `3` | pusty (pętla skończyła się wcześniej) |
| `7` | `7` | pusty | pusty |
| `7/3/2/9` | błąd przy czwartym polu | | |

Puste pole w środku (`7//2`) i pole, do którego pętla nie doszła (`7/3`), wyglądają potem tak samo: pusty widok. Dlatego wszystkie cztery postacie obsługuje jeden kod.

Drugi krok: przeliczenie indeksów.

```cpp
    CornerKey key{NO_INDEX, NO_INDEX, NO_INDEX};
    // The position is required. The list sizes are the ones at this line of the file:
    // an index may only refer to an element defined above the face.
    if (!resolveIndex(fields[0], positions.size(), key[0])) {
```

Pozycja jest obowiązkowa: puste `fields[0]` nie jest liczbą, więc `resolveIndex` zwraca `false`. Uv i normalna są przeliczane tylko wtedy, gdy pole nie jest puste (`!fields[1].empty() && !resolveIndex(...)`), inaczej w kluczu zostaje `NO_INDEX`. Do `resolveIndex` trafia `positions.size()` z tej chwili: stąd reguła "przed tą linią" dla indeksów ujemnych i stąd błąd, gdy ściana odwołuje się do pozycji zdefiniowanej niżej w pliku.

Komunikat błędu podaje narożnik i długość listy, na przykład `face corner '4' has a bad position index (3 positions defined so far)`.

Trzeci krok: mapa.

```cpp
    // Has this exact combination been used before? Then the vertex exists already.
    const auto found = vertexOfCorner.find(key);
    if (found != vertexOfCorner.end()) {
        vertexIndex = found->second;
        return true;
    }

    // A new combination: build the vertex from the three lists. A missing uv or normal
    // stays at zero, the default of gfx::Vertex.
    gfx::Vertex vertex;
    vertex.position = positions[key[0]];
    if (key[1] != NO_INDEX) {
        vertex.uv = uvs[key[1]];
    }
    if (key[2] != NO_INDEX) {
        vertex.normal = normals[key[2]];
    }

    vertexIndex = static_cast<std::uint32_t>(model.vertices.size());
    model.vertices.push_back(vertex);
    vertexOfCorner.emplace(key, vertexIndex);
    return true;
```

`find` zwraca iterator na znaleziony wpis albo `end()`, gdy klucza nie ma. `found->second` to wartość wpisu, czyli numer wierzchołka. Nowy wierzchołek dostaje numer równy dotychczasowej liczbie wierzchołków (pierwszy to 0), jest dopisywany na koniec tablicy i zapamiętywany w mapie. To jest w kodzie dokładnie tabela z sekcji 2.4.

Indeksowanie `positions[key[0]]` jest bezpieczne, bo `resolveIndex` sprawdził zakres. Z tego samego powodu każdy indeks w `model.indices` wskazuje istniejący wierzchołek: numery pochodzą wyłącznie z tej funkcji.

### 5.6 Ściana, wachlarz i części

**`readPosition`, `readUv`, `readNormal`** są prawie identyczne:

```cpp
bool ObjParser::readPosition(std::string_view rest, std::string& message) {
    // A fourth number (w) or vertex colours after the three are ignored.
    std::array<float, gfx::POSITION_COMPONENTS> values{};
    if (!takeFloats(rest, values)) {
        message = "v needs three numbers";
        return false;
    }
    positions.emplace_back(values[0], values[1], values[2]);
    return true;
}
```

Liczby trafiają najpierw do małej tablicy `std::array<float, 3>` (jej rozmiar to stała `gfx::POSITION_COMPONENTS`, ta sama, której `Mesh` używa przy opisie atrybutu), a `std::array` zamienia się na `std::span<float>` sam. Dopiero gdy wszystkie trzy się udały, `emplace_back` buduje z nich `glm::vec3` na końcu listy. Linia `vt` wymaga dwóch liczb, `vn` trzech. Normalna jest zapisywana tak, jak stoi w pliku: parser nie zmienia jej długości.

**`readFace`**:

```cpp
    faceCorners.clear();
    for (std::string_view corner = takeToken(rest); !corner.empty(); corner = takeToken(rest)) {
        std::uint32_t vertexIndex = 0;
        if (!readCorner(corner, vertexIndex, message)) {
            return false;
        }
        faceCorners.push_back(vertexIndex);
    }
    if (faceCorners.size() < MIN_FACE_CORNERS) {
        message = "f needs at least three corners";
        return false;
    }
```

Pętla bierze pole po polu, aż `takeToken` zwróci pusty widok. Każde pole to jeden narożnik. `clear()` opróżnia wektor, ale zostawia jego pamięć.

```cpp
    // The first face after a change of material starts a new part. An usemtl line that
    // no face follows therefore creates nothing.
    if (model.parts.empty() || model.parts.back().material != currentMaterial) {
        ObjPart part;
        part.material = currentMaterial;
        part.firstIndex = static_cast<std::uint32_t>(model.indices.size());
        model.parts.push_back(part);
    }
```

Części powstają **przy ścianach, a nie przy liniach `usemtl`**. Linia `usemtl` tylko zmienia `currentMaterial`. Nowa część zaczyna się wtedy, gdy przychodzi ściana, a ostatnia część ma inny materiał niż bieżący (albo części jeszcze nie ma). Z tej jednej reguły wynika wszystko:

| Sytuacja w pliku | Wynik |
|---|---|
| `usemtl A`, ściany, `usemtl B`, ściany | dwie części |
| ściany przed pierwszym `usemtl` | część z pustą nazwą materiału |
| `usemtl A`, od razu `usemtl B`, ściany | jedna część `B`: `A` nie miało ścian |
| `usemtl A` na końcu pliku, bez ścian | nic |
| `usemtl A`, ściany, znowu `usemtl A`, ściany | jedna część: materiał się nie zmienił |
| `A`, `B`, znowu `A` | trzy części: parser nie przestawia trójkątów, żeby połączyć pierwszą z trzecią |

`firstIndex` to liczba indeksów zapisanych do tej pory, czyli numer pierwszego indeksu nowej części.

```cpp
    // Triangle fan: a polygon with the corners 0, 1, 2, ..., n - 1 becomes the triangles
    // (0, 1, 2), (0, 2, 3), ..., (0, n - 2, n - 1). Every triangle keeps the winding of
    // the polygon. A triangle (n = 3) goes through the loop once and is copied as it is.
    for (std::size_t i = 1; i + 1 < faceCorners.size(); ++i) {
        model.indices.push_back(faceCorners[0]);
        model.indices.push_back(faceCorners[i]);
        model.indices.push_back(faceCorners[i + 1]);
    }
    model.parts.back().indexCount =
        static_cast<std::uint32_t>(model.indices.size()) - model.parts.back().firstIndex;
    return true;
```

Wachlarz z sekcji 2.6. Dla trójkąta (`size() == 3`) pętla wykonuje się dla `i = 1` i kończy, bo `2 + 1 < 3` jest fałszem. Dla czworokąta wykonuje się dla `i = 1` i `i = 2`: trójkąty `(0, 1, 2)` i `(0, 2, 3)`. Warunek jest zapisany jako `i + 1 < size`, a nie `i < size - 1`, żeby nie odejmować od liczby bez znaku.

Na końcu długość bieżącej części jest liczona na nowo: wszystko od jej początku do końca tablicy indeksów. Czworokąt dodaje więc do części 6 indeksów, a nie 4.

### 5.7 `parseObj`, `parseMtl` i `loadObj`

**`parseObj`** to pętla po liniach i rozdzielnia według słowa kluczowego:

```cpp
    while (!rest.empty()) {
        ++lineNumber;
        // Work on the line without its comment. takeToken skips the blanks, including
        // the '\r' of a Windows line ending.
        std::string_view line = withoutComment(takeLine(rest));
        const std::string_view keyword = takeToken(line);

        std::string message;
        bool ok = true;
        if (keyword.empty() || keyword == "o" || keyword == "g" || keyword == "s") {
            // Nothing to do. An empty keyword is a blank line or a line with only a
            // comment. o, g and s (object name, group name, smoothing group) are known and
            // not needed: the whole file becomes one mesh, and normals are taken from the
            // vn lines as they are.
        } else if (keyword == "v") {
            ok = parser.readPosition(line, message);
        } else if (keyword == "vt") {
            ok = parser.readUv(line, message);
        } else if (keyword == "vn") {
            ok = parser.readNormal(line, message);
        } else if (keyword == "f") {
            ok = parser.readFace(line, message);
        } else if (keyword == "usemtl") {
```

Po `takeToken(line)` zmienna `line` zawiera już tylko pola po słowie kluczowym. Dalsze gałęzie:

| Słowo kluczowe | Co się dzieje |
|---|---|
| puste albo `o`, `g`, `s` | pierwsza gałąź: nic. Puste słowo to linia pusta albo sam komentarz, a `o`, `g` i `s` są znane i pomijane celowo |
| `v`, `vt`, `vn`, `f` | odpowiednia funkcja `read...` |
| `usemtl` | reszta linii po `trim` staje się `currentMaterial`. Pusta nazwa to błąd |
| `mtllib` | reszta linii po `trim` jest dopisywana do `materialLibraries`. Pusta nazwa to błąd. Cała reszta linii to **jedna** nazwa pliku, więc może zawierać spacje |
| cokolwiek innego | pomijane, ale licznik `unknownLineCount` rośnie o 1 |

Po każdej linii: gdy `ok` jest fałszem, funkcja zapisuje `lineError(lineNumber, message)` do `error` i wraca z `false`. Po ostatniej linii przenosi `parser.model` do parametru `model`.

Porównanie `keyword == "v"` porównuje **całe** słowo, więc `vt` i `vn` nie są mylone z `v`.

**`parseMtl`** ma tę samą pętlę, ale tylko trzy słowa kluczowe:

| Słowo | Co się dzieje |
|---|---|
| `newmtl` | nowy materiał o nazwie z reszty linii. Pusta nazwa to błąd |
| `Kd` | trzy liczby do `diffuseColor` ostatniego materiału. Przed pierwszym `newmtl` to błąd |
| `map_Kd` | reszta linii jako ścieżka do `diffuseTexture` ostatniego materiału. Przed pierwszym `newmtl` albo bez nazwy to błąd |
| wszystko inne | pomijane bez liczenia |

Materiały zbiera w lokalnym wektorze `parsed` i dopiero na końcu **dopisuje** je do parametru `materials`. Dopisuje, a nie zastępuje, bo model może mieć kilka linii `mtllib` i wszystkie materiały trafiają do jednej listy.

Ścieżka z pliku staje się obiektem `std::filesystem::path` w funkcji `pathFromText`:

```cpp
std::filesystem::path pathFromText(std::string_view text) {
    const std::u8string utf8(text.begin(), text.end());
    return {utf8};
}
```

Tekst pliku traktuję jako UTF-8. Konstruktor `path` z typu `std::u8string` mówi to wprost (`return {utf8};` buduje zwracaną ścieżkę z tego napisu). Konstruktor ze zwykłego `std::string` użyłby na Windowsie strony kodowej systemu i nazwa pliku z polskimi literami zostałaby odczytana źle. To odwrotność funkcji `core::pathText` ([`../core/paths.md`](../core/paths.md)).

**`loadObj`** składa całość. Robotę wykonuje pomocnicza `loadObjFiles`, a `loadObj` dodaje logowanie:

```cpp
bool loadObj(const std::filesystem::path& path, ObjModel& model, std::string& error) {
    if (!loadObjFiles(path, model, error)) {
        // The one place that logs, so that every failure appears in the log exactly once.
        core::logError(error);
        return false;
    }
```

Jedno miejsce z `core::logError` na cały moduł: każdy błąd pojawia się w konsoli dokładnie raz. Po udanym wczytaniu funkcja wypisuje jeszcze ostrzeżenie (`core::logWarn`), jeśli `unknownLineCount` jest większe od zera.

Kroki `loadObjFiles`:

| # | Krok | Błąd |
|---|---|---|
| 1 | `readFile(path, objText)`: cały plik do napisu | `OBJ file cannot be opened: <ścieżka>` |
| 2 | `parseObj(objText, loaded, parseError)` | `<ścieżka>: line N: ...` |
| 3 | dla każdej nazwy z `materialLibraries`: `mtlPath = objDirectory / nazwa`, wczytanie i `parseMtl` | `MTL file cannot be opened: <ścieżka> (named by <plik OBJ>)` albo `<plik MTL>: line N: ...` |
| 4 | dla każdego materiału z teksturą: `(mtlDirectory / diffuseTexture).lexically_normal()` | brak |
| 5 | każda część z niepustą nazwą materiału musi mieć materiał na liście | `<ścieżka>: material 'x' is used but not defined in any material library` |
| 6 | `model = std::move(loaded)` | |

Szczegóły:

- **`readFile`** otwiera plik w trybie binarnym (`std::ios::binary`): bajty przychodzą takie, jakie są w pliku, na każdym systemie. W trybie tekstowym Windows sam zamieniałby `\r\n` na `\n`, a macOS nie. Wolę jedno zachowanie i obsługę `\r` w parserze.
- **`path.parent_path()`** to katalog pliku. Operator `/` klasy `path` łączy katalog z nazwą względną.
- **`lexically_normal()`** porządkuje ścieżkę "na papierze", bez pytania systemu plików: usuwa kroki `..` razem z poprzedzającym katalogiem. `assets/models/../textures/wall_stone.png` staje się `assets/textures/wall_stone.png`. Dwa modele wskazujące tę samą teksturę dostają dzięki temu **identyczną** ścieżkę, co pozwoli przyszłej pamięci podręcznej assetów wczytać obraz raz. Ścieżka jest bezwzględna tylko wtedy, gdy bezwzględna była ścieżka podana do `loadObj`.
- **Istnienie pliku tekstury nie jest sprawdzane.** Zgłosi to kod, który będzie otwierał obraz, tak jak `core::assetPath` nie sprawdza istnienia pliku.
- **Krok 5** dotyczy tylko części z nazwą. Model bez `mtllib` i bez `usemtl` wczytuje się poprawnie, z pustą listą materiałów i jedną częścią o pustej nazwie.
- **Brakujący plik MTL jest błędem**, a nie ostrzeżeniem. Model bez materiałów narysowałby się bez tekstury i wyglądałby na błąd shadera. Wolę jasny komunikat przy wczytaniu.

### 5.8 Co parser pomija, a czego nie umie

| Rzecz | Zachowanie |
|---|---|
| `#`, linie puste, `o`, `g`, `s` | pomijane celowo, nieliczone |
| inne słowa kluczowe OBJ (`l`, `p`, `vp`, `curv`, ...) | pomijane, liczone w `unknownLineCount`, jedno ostrzeżenie w logu |
| inne słowa kluczowe MTL (`Ns`, `Ka`, `Ks`, `map_Bump`, ...) | pomijane, nieliczone |
| czwarta liczba w `v` (`w`, kolory), trzecia w `vt` | ignorowana |
| linia `vt` z jedną liczbą | **błąd** (specyfikacja dopuszcza samo `u`, ja wymagam dwóch) |
| opcje przed nazwą pliku w `map_Kd` (`-s 2 2 1 plik.png`) | **nieobsługiwane**: cała reszta linii jest brana jako nazwa pliku |
| kilka plików w jednej linii `mtllib` | **nieobsługiwane**: reszta linii to jedna nazwa |
| znak `#` w nazwie pliku albo materiału | obcina nazwę, bo `#` zawsze zaczyna komentarz |
| kontynuacja linii znakiem `\` na końcu | **nieobsługiwana** |
| wielokąty wklęsłe | dzielone wachlarzem, wynik może być błędny (sekcja 2.6) |
| grupy wygładzania, liczenie normalnych | brak: normalne tylko z linii `vn` |
| styczne (tangents) | brak: mapy normalnych są odłożone do następnej części M4 |
| kilka obiektów (`o`) w pliku | wszystkie trafiają do jednej siatki |
| znacznik BOM na początku pliku | nieobsługiwany: pierwsza linia miałaby nieznane słowo kluczowe |

### 5.9 Jak to zostało sprawdzone

Testy jednostkowe w doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)), plik [`tests/ObjLoaderTests.cpp`](../../../tests/ObjLoaderTests.cpp): 18 przypadków testowych, 804 asercje.

Uruchomienie samych testów loadera (Windows, build w `build/debug`):

```powershell
.\build\debug\Debug\night_maze_tests.exe --source-file=*ObjLoaderTests*
```

Testy parserów podają tekst wpisany w kod:

| Przypadek testowy | Co sprawdza |
|---|---|
| `parseObj: identical position/uv/normal triples share one vertex` | kwadrat z dwóch trójkątów: 6 narożników, 4 wierzchołki, indeksy `0, 1, 2, 0, 2, 3`, zawartość wierzchołków |
| `...the same position with another uv or normal is another vertex` | te same pozycje z innym uv albo normalną: 6 wierzchołków |
| `...the four forms of a face corner` | `v/vt/vn`, `v//vn`, `v/vt`, `v`: brakujące pola są zerami. Narożnik bez uv i z uv to różne wierzchołki |
| `...negative indices count back from the end of the list so far` | `-1` to ostatni element. Indeks ujemny i dodatni tego samego elementu dają jeden wierzchołek. Druga ściana po dopisaniu pozycji wskazuje nowe pozycje |
| `...a polygon is split into a fan of triangles` | czworokąt: `0, 1, 2, 0, 2, 3`. Pięciokąt: trzy trójkąty |
| `...line endings, blanks and comments` | CRLF (i brak `\r` w nazwie materiału), tabulatory i powtórzone spacje, brak końca ostatniej linii, komentarze także po danych, pusty tekst |
| `...numbers` | znak, ułamek, `-0.0000`, wykładnik `1e-3` i `2.5E2`, nadmiarowe liczby |
| `...every run of faces after usemtl is one part` | dwa materiały i ich zakresy `(0, 6)` i `(6, 3)`, czworokąt w zakresie, ściany przed `usemtl`, `usemtl` bez ścian, powtórzony materiał, powrót materiału, nazwa ze spacją |
| `...mtllib names are collected, other keywords are skipped` | dwie nazwy `mtllib` (jedna ze spacją), `o`, `g`, `s` nieliczone, `l` i `curv` policzone: 2 |
| `...a bad line is reported with its line number` | indeks poza listą, indeks 0, indeks ujemny za daleko, zły indeks uv i normalnej, odwołanie do pozycji zdefiniowanej niżej, indeks niebędący liczbą całkowitą (`x`, `3.5`, `3a`, liczba 20-cyfrowa), brak pozycji, cztery pola, mniej niż trzy narożniki, zła liczba (`abc`, `1.5x`, `--1`, `0,5`), za mało liczb, `usemtl` i `mtllib` bez nazwy, numer linii przy CRLF i liniach pustych |
| `...a failed parse leaves the model of the caller unchanged` | model wypełniony wcześniej nie zmienia się po nieudanym wywołaniu |
| `parseMtl: newmtl, Kd and map_Kd` | materiał w postaci z Blendera, kilka materiałów z CRLF i bez końca linii, domyślny biały kolor, ścieżka ze spacjami, dopisywanie do listy, pusty tekst |
| `parseMtl: a bad line is reported with its line number` | `Kd` i `map_Kd` przed `newmtl`, zła liczba, za mało liczb, brak nazwy, nic nie jest dopisane po błędzie |

Testy `loadObj` czytają pliki. Trzy pierwsze wczytują **prawdziwe modele gry** z katalogu `assets/models` repozytorium. Ścieżkę do katalogu `assets` program testowy dostaje od CMake jako definicję kompilacji `NIGHT_MAZE_ASSETS_DIR`, więc test nie zależy od katalogu, z którego jest uruchamiany.

| Model | Wierzchołki | Indeksy | Trójkąty | Pudełko otaczające (min, max) | Materiał | Tekstura |
|---|---|---|---|---|---|---|
| `wall_straight.obj` | 60 | 90 | 30 | `(-1, 0, -0.14)`, `(1, 3, 0.14)` | `wall_stone` | `assets/textures/wall_stone.png` |
| `wall_pillar.obj` | 60 | 90 | 30 | `(-0.2, 0, -0.2)`, `(0.2, 3.15, 0.2)` | `wall_stone` | `assets/textures/wall_stone.png` |
| `floor_tile.obj` | 4 | 6 | 2 | `(-1, 0, -1)`, `(1, 0, 1)` | `floor_stone` | `assets/textures/floor_stone.png` |

To są wartości zmierzone przez testy na Windowsie. Liczby wierzchołków zgadzają się z liczbą różnych trójek policzoną niezależnie, skryptem czytającym same linie `f` (sekcja 2.4). Dla każdego modelu test sprawdza też: każdy indeks jest mniejszy od liczby wierzchołków, każda normalna ma długość 1 (loader ich nie normalizuje, więc to pomiar pliku), jest dokładnie jedna część obejmująca wszystkie indeksy, `Kd` jest białe, `unknownLineCount` wynosi 0, a ścieżka tekstury jest równa `assets/textures/<nazwa>.png` i **plik istnieje**. Dla płyty podłogi dodatkowo: wszystkie normalne to `(0, 1, 0)`, a iloczyn wektorowy krawędzi obu trójkątów wskazuje w górę, czyli nawinięcie jest przeciwne do ruchu wskazówek zegara, patrząc z góry.

Pozostałe dwa przypadki:

| Przypadek testowy | Co sprawdza |
|---|---|
| `loadObj: material libraries and texture paths of files written by the test` | pliki zapisane przez test w katalogu tymczasowym systemu: dwa `mtllib` (jeden w podkatalogu), `map_Kd` z `../../` rozwiązane względem katalogu pliku MTL, model bez materiałów, brakujący plik MTL, niezdefiniowany materiał, zła linia w OBJ i w MTL zgłoszona z nazwą pliku i numerem linii |
| `loadObj: a file that does not exist is reported, not thrown` | brak pliku: `false`, komunikat z nazwą, model pusty |

Testy błędów `loadObj` celowo wywołują logowanie, więc w wyjściu programu testowego pojawiają się linie `[error] ...`. To nie są niepowodzenia testów.

**Wyniki.** Windows 11, MSVC 19.44, `/W4 /permissive-`, 2026-10-05: build Debug bez ostrzeżeń generatorem Ninja i generatorem Visual Studio, 18 przypadków i 804 asercje przechodzą. **Na macOS kod nie był kompilowany ani uruchamiany.** Otwarte punkty (czytanie liczb przez strumień w libc++, ścieżki z `std::u8string`, ostrzeżenia clang) są na liście w [`../../guides/build-macos.md`](../../guides/build-macos.md).

**Czego testy nie obejmują:** działania przy globalnym locale innym niż "C" (uzasadnienie w sekcji 5.4 opiera się na dokumentacji `imbue`, a nie na teście z polskim locale), bardzo dużych plików i szybkości.

## 6. Panel ImGui

Wynik loadera pokazuje panel **Assets** (kod: [`src/debug/panels/AssetsPanel.cpp`](../../../src/debug/panels/AssetsPanel.cpp)). PRD w sekcji 3 wymienia dla tematu 4 pokaz "Lista załadowanych modeli": to część tego panelu pod nagłówkiem `Models`. Panel linia po linii i scenariusz pokazu są w [`asset-cache.md`](asset-cache.md), sekcja 6. Tu jest to, co dotyczy loadera:

| Element panelu | Skąd pochodzi | Co pokazuje dla modeli gry |
|---|---|---|
| nazwa pliku modelu, pełna ścieżka w podpowiedzi | `LoadedModel::path` | `floor_tile.obj`, `wall_straight.obj`, `wall_pillar.obj`, w kolejności wczytania |
| `... vertices, ... triangles` | liczba elementów `ObjModel::vertices` i jedna trzecia liczby `ObjModel::indices`, zapamiętane przy wczytaniu | 4 i 2 dla płytki, 60 i 30 dla ściany, 60 i 30 dla słupka: te same liczby co w tabeli z sekcji 5.9 |
| `part '...': ... triangles, ...` | `ObjPart::material`, `ObjPart::indexCount` podzielone przez 3, nazwa pliku z `ObjMaterial::diffuseTexture` | jedna część na model, materiał `floor_stone` albo `wall_stone` i plik tekstury. Część bez własnej tekstury ma napis `no texture (white)` |
| lista `View mode` | uniform `uViewMode` shadera | normalne z linii `vn` albo współrzędne z linii `vt` jako kolor |
| lista pod nagłówkiem `Failed to load` | ścieżki, dla których `loadObj` albo `loadImage` zwróciło `false` | pusta, gdy wszystko się wczytało. Nagłówek pojawia się tylko wtedy, gdy jest co pokazać |

Liczby z drugiego i trzeciego wiersza to wniosek z kodu panelu i z wyników testów loadera. Samych wartości w panelu nikt jeszcze nie odczytał z ekranu. Na Windowsie (2026-10-05) sprawdzone jest na zrzutach ekranu, że modele rysują się z teksturami we właściwej orientacji, że oba tryby podglądu działają i że brak pliku tekstury daje białą teksturę zastępczą i jedną linię `[error]`. Widżetów panelu nikt jeszcze nie klikał ręcznie, a na macOS nie sprawdzono niczego.

## 7. Pułapki

1. **Indeksy od 1.** `f 1 2 3` to trzy pierwsze pozycje, a w C++ to elementy 0, 1 i 2. Zapomniane `- 1` przesuwa cały model o jeden wierzchołek i kończy się czytaniem poza tablicą przy ostatnim. Parser robi to w jednym miejscu (`resolveIndex`), a indeks 0 odrzuca jako błąd.
2. **Trzy listy różnej długości.** Nie wolno założyć, że pozycja numer 5 idzie w parze z normalną numer 5. W `wall_straight.obj` są 24 pozycje i 6 normalnych. Wierzchołek powstaje dopiero z trójki indeksów narożnika (sekcja 2.4).
3. **Indeksy pliku wysłane wprost do OpenGL.** Najczęstszy błąd pierwszego loadera: tablica pozycji jako bufor wierzchołków, a indeksy pozycji z linii `f` jako bufor indeksów. Kształt wychodzi dobry, a tekstury i normalne złe, bo uv i normalne mają inne indeksy.
4. **Kolejność pól w narożniku.** `a/b/c` to pozycja, **uv**, normalna, choć w pliku z Blendera linie `vn` stoją przed `vt`.
5. **Końce linii CRLF.** Bez obsługi `\r` ostatnie pole każdej linii ma na końcu niewidoczny znak: `usemtl stone\r` nie pasuje do `newmtl stone`, a liczba `0.14\r` nie jest liczbą. Błąd pojawia się tylko na plikach z Windowsa, czyli "u mnie działa".
6. **Przecinek dziesiętny z locale.** `atof` i `std::stof` zależą od globalnego locale. Przy polskim `0.14` czyta się jako `0`. Model robi się płaski albo znika, bez żadnego komunikatu. Parser czyta liczby strumieniem z klasycznym locale (sekcja 5.4).
7. **Indeksy ujemne.** Liczą się od końca listy **w chwili czytania ściany**. Przeliczanie ich po wczytaniu całego pliku daje złe elementy w pliku, w którym ściany przeplatają się z wierzchołkami.
8. **Czworokąty.** Plik z innego narzędzia może mieć linie `f` z czterema narożnikami. Loader, który czyta zawsze trzy, po cichu gubi połowę każdej ściany. Parser dzieli wielokąty wachlarzem, ale tylko wypukłe wychodzą na pewno dobrze.
9. **Tekstura do góry nogami.** `v = 0` to dół obrazu, a plik PNG zaczyna się od górnego wiersza. To nie jest błąd loadera OBJ: format OBJ ma tę samą konwencję co OpenGL (`v` rośnie w górę), więc parser przepisuje `vt` do `Vertex::uv` bez zmian, a jedyną rzeczą niezgodną jest kolejność wierszy w pliku obrazu. Poprawkę robi więc `assets::loadImage`, które kopiuje wiersze w odwrotnej kolejności ([`images.md`](images.md), sekcja 2). Zamiana `v` na `1 - v` w loaderze OBJ albo w shaderze dałaby **ten sam obraz**, także dla UV spoza zakresu od 0 do 1, których moje modele mają dużo (na przykład `-0.5` i `1.5` w `wall_straight.obj`): przy zawijaniu `GL_REPEAT` liczy się tylko część ułamkowa współrzędnej, a `1 - v` to odbicie lustrzane i przesunięcie o całą liczbę powtórzeń, więc zawijanie niczego tu nie psuje. Powody, żeby odwracać obraz, a nie współrzędne, są inne. Odwrócenie wierszy robi się raz, na procesorze, przy wczytaniu. Wszyscy odbiorcy tekstury (każdy shader, a także `ImGui::Image` w panelu) mają jedną konwencję i żaden nie musi pamiętać o poprawce. Zgadza się to z opisem pola `gfx::Vertex::uv` ("v = 0 is the bottom row of the image"). Prawdziwa pułapka to **podwójna poprawka**: odwrócony obraz i dodatkowo `1 - v` gdziekolwiek dają znów teksturę do góry nogami.
10. **Ścieżka tekstury względem złego katalogu.** `map_Kd` jest względem pliku MTL, `mtllib` względem pliku OBJ. Otwieranie ich względem katalogu roboczego działa tylko wtedy, gdy program startuje z jednego konkretnego miejsca ([`../core/paths.md`](../core/paths.md)).
11. **`v` a `vt` i `vn`.** Sprawdzanie tylko pierwszego znaku linii (`line[0] == 'v'`) wrzuca normalne i uv do listy pozycji. Trzeba porównywać całe słowo kluczowe.
12. **Brak znaku nowej linii na końcu pliku.** Pętla "czytaj do `\n`" gubi ostatnią linię, czyli ostatni trójkąt. `takeLine` ma na to osobną gałąź.
13. **Ta sama tekstura wczytana dwa razy.** `wall_straight.mtl` i `wall_pillar.mtl` wskazują ten sam plik PNG. Loader zwraca dla obu identyczną, uporządkowaną ścieżkę, ale sam niczego nie zapamiętuje: o to, żeby obraz trafił na kartę raz, dba kod, który z loadera korzysta. Robi to `assets::AssetCache`, dla którego ta uporządkowana ścieżka jest kluczem ([`asset-cache.md`](asset-cache.md), sekcja 2).
14. **`-0.0000` w normalnych.** To zwykłe zero ze znakiem minus. Czyta się poprawnie i w porównaniach jest równe `0.0`.
15. **Spacje w nazwach.** Nazwa materiału i nazwa pliku to reszta linii, nie jedno pole. `map_Kd old stone.png` cięte na pola dałoby plik `old`.
16. **Poprawny plik, którego nie da się narysować.** Plik OBJ bez ani jednej linii `f` jest dla `loadObj` poprawny: funkcja zwraca `true` i pusty model. Loader nie ocenia, czy wynik się do czegoś nadaje. Odrzuca go dopiero `AssetCache::model`, z komunikatem `Model has no faces: ...`.
17. **Loader nie sprawdza, czy plik tekstury istnieje.** `ObjMaterial::diffuseTexture` to tylko ścieżka zbudowana z tekstu. Model z literówką w `map_Kd` wczytuje się bez błędu, a brak pliku wychodzi dopiero przy wczytywaniu obrazu: w grze część jest wtedy rysowana białą teksturą i w konsoli jest jedna linia `[error]` (zmierzone na Windowsie).

## 8. Ćwiczenia

Testy uruchamia `ctest --test-dir build/debug -C Debug --output-on-failure`. Po każdym ćwiczeniu wycofaj zmiany (`git checkout src tests assets`).

1. **Mapa trójek na kartce.** Dla pliku poniżej wypisz tabelę jak w sekcji 2.4. Ile wierzchołków i jakie indeksy? Potem sprawdź testem wzorowanym na pierwszym przypadku z `ObjLoaderTests.cpp`.

   ```text
   v 0 0 0
   v 1 0 0
   v 1 1 0
   v 0 1 0
   vn 0 0 1
   vn 0 1 0
   f 1//1 2//1 3//1
   f 1//1 3//2 4//1
   ```

   Odpowiedź: 5 wierzchołków, indeksy `0, 1, 2, 0, 3, 4`. Narożnik `3//2` ma tę samą pozycję co `3//1`, ale inną normalną.
2. **Bez mapy.** Zakomentuj w `readCorner` pięć linii, które szukają klucza w mapie (od `const auto found` do zamykającej klamry instrukcji `if`). Uruchom testy. Które przypadki przestają przechodzić i ile wierzchołków ma teraz `wall_straight.obj`? Czy model narysowałby się poprawnie? (Tak: 90 wierzchołków zamiast 60, tylko więcej pamięci.)
3. **Bez `- 1`.** Zmień w `resolveIndex` wyrażenie `written - 1` na `written`. Które testy zgłaszają błąd i jaki komunikat dostaje `floor_tile.obj`?
4. **CRLF.** Usuń `\r` ze stałej `BLANKS`. Który podprzypadek przestaje przechodzić i jaki jest komunikat błędu? Dlaczego błąd dotyczy liczby, a nie nazwy materiału?
5. **Zepsuj model.** W kopii `floor_tile.obj` zmień `f 2/1/1 4/2/1 1/3/1` na `f 2/1/1 4/2/1 5/3/1`. Jaki komunikat wypisze `loadObj`? Z której linii pliku?
6. **Czworokąt.** Zamień dwie linie `f` w kopii `floor_tile.obj` na jedną z czterema narożnikami, tak żeby wynik miał nadal 4 wierzchołki i normalną w górę. W jakiej kolejności muszą iść narożniki? (Przeciwnie do ruchu wskazówek zegara, patrząc z góry: `f 1/3/1 2/1/1 3/4/1 4/2/1`.)
7. **Dwa materiały.** Dopisz do kopii `wall_straight.obj` linię `usemtl floor_stone` przed ostatnimi dziesięcioma liniami `f` i do pliku MTL materiał `floor_stone`. Ile części ma model i jakie są ich `firstIndex` i `indexCount`? (Dwie: `(0, 60)` i `(60, 30)`.)
8. **Locale.** Napisz mały test, który przed `parseObj` woła `std::setlocale(LC_ALL, "pl_PL.UTF-8")` (na Windowsie `"Polish"`) i sprawdza, że `v 0.5 0 0` daje `x == 0.5`. Potem zamień tymczasowo `parseFloat` na wersję z `std::strtof` i porównaj. Przywróć locale `"C"` na końcu testu.
9. **Nowe słowo kluczowe.** Dodaj do `parseMtl` obsługę `Ks` (kolor odbłysku) jako pola `specularColor` w `ObjMaterial` i test. Które istniejące testy trzeba poprawić? (Żadnego: dotąd linia była pomijana.)

## 9. Pytania kontrolne

1. **Co zwraca loader OBJ i czego nie robi?**
   Strukturę `ObjModel`: tablicę wierzchołków `gfx::Vertex`, tablicę indeksów, listę części (materiał, pierwszy indeks, liczba indeksów) i listę materiałów (nazwa, kolor `Kd`, ścieżka tekstury z `map_Kd`). Nie tworzy obiektów OpenGL, nie wczytuje obrazów i nie sprawdza, czy plik tekstury istnieje.

2. **Co oznaczają linie `v`, `vt`, `vn` i `f`?**
   Pozycję (trzy liczby), współrzędną tekstury (dwie), normalną (trzy) i ścianę. Narożnik ściany to do trzech indeksów oddzielonych ukośnikami: pozycja, uv, normalna. Indeksy liczą się od 1.

3. **Dlaczego nie można wysłać danych z pliku OBJ wprost do buforów OpenGL?**
   Bo OBJ ma trzy osobne listy z trzema niezależnymi indeksami na narożnik, a OpenGL ma jeden indeks, który wybiera pozycję, normalną i uv naraz. Trzeba utworzyć jeden wierzchołek dla każdej różnej trójki indeksów.

4. **Jak parser rozpoznaje, że wierzchołek już istnieje?**
   Prowadzi `std::map`, w której kluczem jest trójka indeksów (po przeliczeniu na liczone od 0), a wartością numer wierzchołka wyjściowego. Dla każdego narożnika szuka klucza: gdy jest, bierze zapisany numer, gdy nie ma, składa nowy wierzchołek z trzech list, dopisuje go i zapamiętuje.

5. **Dlaczego `wall_straight.obj` ma 24 pozycje, a wynik 60 wierzchołków?**
   Bo jedna pozycja występuje w kilku trójkach: róg prostopadłościanu należy do kilku ścian o różnych normalnych i różnych uv. Różnych trójek jest 60. Z drugiej strony 90 narożników to tylko 60 wierzchołków, bo dwa trójkąty prostokąta mają dwa narożniki wspólne.

6. **Jakie postacie może mieć narożnik ściany i co się dzieje z brakującymi polami?**
   `a/b/c`, `a//c`, `a/b` i `a`. Brakujące uv albo normalna są w wierzchołku zerami, a w kluczu mapy stałą `NO_INDEX`, więc narożnik bez uv nie zlewa się z narożnikiem z uv.

7. **Jak działają indeksy ujemne?**
   Liczą się od końca listy w chwili czytania ściany: `-1` to element zdefiniowany ostatnio. Dla listy o `n` elementach indeks `-k` to element `n - k`, licząc od 0. Parser liczy to w `resolveIndex` jednym wyrażeniem: `written > 0 ? written - 1 : size + written`.

8. **Jak parser dzieli czworokąt i dlaczego kierunek nawijania zostaje?**
   Wachlarzem: narożniki `0, 1, 2, 3` dają trójkąty `(0, 1, 2)` i `(0, 2, 3)`. Każdy obiega narożniki w tej samej kolejności co wielokąt. Wielokąt o `n` narożnikach daje `n - 2` trójkątów. Metoda jest poprawna dla wielokątów wypukłych.

9. **Skąd wiadomo, która strona trójkąta jest przednia?**
   Z kolejności narożników: przeciwnie do ruchu wskazówek zegara, patrząc z zewnątrz. To konwencja OBJ i domyślna konwencja OpenGL, więc parser zachowuje kolejność z pliku. Normalna jest osobną daną i parser jej nie liczy ani nie poprawia.

10. **Co to jest część (`ObjPart`) i kiedy powstaje nowa?**
    Zakres indeksów o jednym materiale: `firstIndex` i `indexCount`, czyli argumenty rysowania zakresu. Nowa część powstaje przy pierwszej ścianie po zmianie materiału, a nie przy samej linii `usemtl`, więc `usemtl` bez ścian nie tworzy niczego.

11. **Względem czego liczone są ścieżki z `mtllib` i `map_Kd`?**
    `mtllib` względem katalogu pliku OBJ, `map_Kd` względem katalogu pliku MTL. `loadObj` łączy je operatorem `/` i porządkuje przez `lexically_normal()`, które usuwa kroki `..`.

12. **Jak parser czyta liczby i dlaczego nie przez `atof` albo `std::stof`?**
    Liczby zmiennoprzecinkowe czyta `std::istringstream` z ustawionym klasycznym locale (`imbue(std::locale::classic())`), a całkowite `std::from_chars`. `atof` i `std::stof` zależą od globalnego locale programu: przy polskim oczekują przecinka i `0.14` czytają jako `0`. Oba sposoby sprawdzają też, czy całe pole zostało zużyte.

13. **Dlaczego liczby zmiennoprzecinkowe nie są czytane przez `std::from_chars`?**
    Bo biblioteka standardowa używana na macOS (libc++) ma jego przeciążenia dla `float` dopiero od LLVM 20 i nie mam potwierdzenia, że są w wersji dostarczanej z kompilatorem, którym projekt jest budowany na Macu. Strumień z klasycznym locale działa w obu bibliotekach. Wersja dla liczb całkowitych jest dostępna wszędzie.

14. **Jak loader zgłasza błąd?**
    Zwraca `false`, wpisuje powód do parametru `error` (dla parserów z numerem linii: `line 6: ...`) i zostawia model wołającego bez zmian. Nie rzuca wyjątków. `loadObj` dodatkowo wypisuje błąd raz przez `core::logError` i poprzedza go ścieżką pliku.

15. **Co parser robi z linią, której nie zna?**
    Pomija ją. Linie `o`, `g`, `s`, komentarze i linie puste pomija celowo. Inne nieznane słowa kluczowe liczy w `unknownLineCount`, a `loadObj` wypisuje jedno ostrzeżenie z tą liczbą. W pliku MTL wszystko poza `newmtl`, `Kd` i `map_Kd` jest pomijane bez liczenia.

16. **Po co obsługa `\r`, skoro moje pliki mają same `\n`?**
    Bo Git na Windowsie może zamienić końce linii na `\r\n` przy pobraniu, a pliki z innych narzędzi bywają tak zapisane. Bez tego nazwa materiału i ostatnia liczba każdej linii miałyby na końcu niewidoczny znak.

17. **Czym mój loader różni się od Assimp?**
    Assimp to biblioteka czytająca kilkadziesiąt formatów do wspólnej struktury sceny (węzły, siatki, materiały, animacje) i umiejąca je przetwarzać: triangulować, liczyć normalne i styczne, łączyć wierzchołki. Mój loader czyta jeden format i sześć słów kluczowych, w kilkuset liniach, które umiem wytłumaczyć. Nie obsługuje animacji, hierarchii ani wielokątów wklęsłych.

18. **Dlaczego parsowanie jest oddzielone od czytania plików?**
    Żeby dało się je testować bez plików i bez okna: `parseObj` i `parseMtl` dostają tekst i zwracają dane, więc test podaje napis wpisany w kod. Pliki, katalogi i logowanie są tylko w `loadObj`.

19. **Kto w programie woła `loadObj` i co dzieje się z wynikiem?**
    `assets::AssetCache::model`, raz dla każdego pliku. Z `vertices` i `indices` powstaje `gfx::Mesh`, części są przepisywane do `ModelPart` z kolorem `Kd` i teksturą swojego materiału, a sam `ObjModel` ginie na końcu funkcji. Rysuje `MazeRenderer` shaderami `textured.*`.

20. **Dlaczego orientację tekstury poprawia loader obrazów, a nie `1 - v` w loaderze OBJ albo w shaderze?**
    Wynik na ekranie byłby ten sam, także dla UV spoza zakresu od 0 do 1: przy `GL_REPEAT` liczy się część ułamkowa, a `1 - v` to odbicie i przesunięcie o całe powtórzenia. Powody są organizacyjne: odwrócenie wierszy robi się raz przy wczytaniu, każdy odbiorca tekstury ma tę samą konwencję (`v = 0` na dole, jak w OBJ i w OpenGL), a współrzędne z pliku modelu zostają nietknięte.

21. **Gdzie w shaderze lądują linie `v`, `vn`, `vt`, `Kd` i `map_Kd`?**
    `v`, `vn` i `vt` to atrybuty wierzchołka numer 0, 1 i 2 (`aPosition`, `aNormal`, `aUv` w `textured.vert`). `Kd` to uniform `uTint`, przez który mnożony jest kolor tekstury. `map_Kd` to plik tekstury wiązanej z jednostką, której numer ma sampler `uTexture`.

## 10. Źródła

- Specyfikacja formatu OBJ firmy Wavefront (dodatek B1 dokumentacji Advanced Visualizer), kopia archiwalna: <https://paulbourke.net/dataformats/obj/>. Indeksy od 1, indeksy ujemne, postacie narożnika.
- Specyfikacja formatu MTL firmy Wavefront, kopia archiwalna: <https://paulbourke.net/dataformats/mtl/>. `newmtl`, `Kd`, `map_Kd` i jego opcje.
- LearnOpenGL, rozdziały "Assimp" i "Model" (<https://learnopengl.com/Model-Loading/Assimp>, <https://learnopengl.com/Model-Loading/Model>): to samo zadanie rozwiązane gotową biblioteką, dla porównania.
- Tabela stanu C++17 biblioteki libc++, wiersz P0067R5 (<https://libcxx.llvm.org/Status/Cxx17.html>): `std::from_chars` dla liczb całkowitych od wersji 7, dla `float` i `double` od wersji 20.
- cppreference: `std::from_chars` (<https://en.cppreference.com/w/cpp/utility/from_chars>), `std::locale::classic` (<https://en.cppreference.com/w/cpp/locale/locale/classic>), `std::basic_ios::imbue` (<https://en.cppreference.com/w/cpp/io/basic_ios/imbue>), `std::string_view` (<https://en.cppreference.com/w/cpp/string/basic_string_view>), `std::filesystem::path::lexically_normal` (<https://en.cppreference.com/w/cpp/filesystem/path/lexically_normal>).
- Dokumenty w tym repozytorium: [`README.md`](README.md), [`../../guides/blender.md`](../../guides/blender.md) (skąd są pliki, sekcja 5: co w nich jest), [`../gfx/mesh.md`](../gfx/mesh.md), [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md), [`../core/paths.md`](../core/paths.md), [`../../libraries/doctest.md`](../../libraries/doctest.md).
