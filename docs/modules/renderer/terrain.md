# Moduł renderer: teren z mapy wysokości

Kamień milowy: M6, część druga (teren i trawa). Temat wykładu: 13 (Implementacja podłoża).
Kod: dane i matematyka w [`src/game/Terrain.hpp`](../../../src/game/Terrain.hpp) i [`Terrain.cpp`](../../../src/game/Terrain.cpp), stawianie labiryntu na terenie w [`src/game/MazeWorld.hpp`](../../../src/game/MazeWorld.hpp) i [`MazeWorld.cpp`](../../../src/game/MazeWorld.cpp) (`placeOnTerrain`), wysokość stóp gracza w [`src/game/Player.cpp`](../../../src/game/Player.cpp), rysowanie w [`src/game/TerrainRenderer.hpp`](../../../src/game/TerrainRenderer.hpp) i [`TerrainRenderer.cpp`](../../../src/game/TerrainRenderer.cpp) oraz w [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp) (`drawMesh`), wczytanie mapy i przebudowa w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), panel w [`src/debug/panels/TerrainPanel.cpp`](../../../src/debug/panels/TerrainPanel.cpp), obraz [`assets/textures/heightmap.png`](../../../assets/textures/heightmap.png) i skrypt [`tools/blender/make_heightmap.py`](../../../tools/blender/make_heightmap.py), tekstury [`assets/textures/ground.png`](../../../assets/textures/ground.png) i [`ground_normal.png`](../../../assets/textures/ground_normal.png), testy w [`tests/TerrainTests.cpp`](../../../tests/TerrainTests.cpp).

Dlaczego ten dokument stoi w katalogu `renderer`, chociaż klasy nazywają się `game::Terrain` i `game::TerrainRenderer`, wyjaśnia [`README.md`](README.md). Dokument zakłada znajomość siatki wierzchołków i indeksów ([`../gfx/mesh.md`](../gfx/mesh.md), [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md)), tekstur i zawijania ([`../gfx/textures.md`](../gfx/textures.md)), map normalnych i stycznych ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md)), układu labiryntu w świecie ([`../game/maze-generator.md`](../game/maze-generator.md)) i pudełek kolizji ([`../scene/collision.md`](../scene/collision.md)).

**Stan na dziś:** płaskich płytek podłogi już nie ma. Labirynt stoi na jednej dużej siatce trójkątów, której wysokości pochodzą z obrazu w odcieniach szarości (mapy wysokości). Pod labiryntem podłoże jest łagodnie nierówne, a dookoła przechodzi we wzgórza. Ściany, słupki i brama są opuszczone tak, żeby nigdzie nie było pod nimi szpary, kryształy i strefa wyjścia stoją na wysokości podłoża swojej komórki, a stopy gracza idą po powierzchni. Panel **Terrain** ma suwak `Height scale` i pole `Wireframe`: to są dwa pokazy, które PRD podaje dla tematu 13 ("skala wysokości, wireframe").

**Co zmieniła pierwsza część M7 (2026-10-05).** Kształt terenu, jego wysokości i kolizje się nie zmieniły. Zmieniło się to, jak podłoże trafia na ekran: tekstura `ground.png` jest wczytywana jako sRGB, a jej mapa normalnych jako dane liniowe (sekcja 5.12), programy cieniujące liczą na wartościach liniowych, a teren, jak cała scena, jest rysowany do bufora HDR i dopiero przebieg składający przenosi go do okna ([`post-process.md`](post-process.md), [`../gfx/color-space.md`](../gfx/color-space.md)). Zgłoszone dla tej części: 269 przypadków testowych i 102103 asercje w Debug i Release, a po drugiej części M7 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751. Żaden z przypadków trzech pierwszych części M7 nie dotyczy terenu. W czwartej jeden czyta `Terrain` (sekcja 5.15).

**Co zmieniła czwarta część M7 (cienie księżyca, 2026-10-05).** Kształt terenu, jego wysokości, kolizje i pliki `Terrain.*` oraz `TerrainRenderer.*` się nie zmieniły. Zmieniły się trzy rzeczy wokół nich. Po pierwsze, teren jest rysowany **dwa razy w klatce**: najpierw do mapy cieni księżyca programem `shadow_depth` (same głębie, widok z kierunku światła), potem jak dotąd do sceny. Wzgórza rzucają więc cień, a grunt przyjmuje cień ścian, słupków, bramy i kryształów (w programach `lit` i `gouraud`, nie w `textured`). Po drugie, do mapy cieni teren idzie **zawsze wypełniony**, także przy zaznaczonym polu `Wireframe` (sekcja 2.12). Po trzecie, granice terenu (`minX()`, `maxX()`, `minZ()`, `maxZ()`, `minHeight()`, `maxHeight()`) mają nowego czytelnika: `game::shadowCasterBounds` buduje z nich pudełko, do którego księżyc dopasowuje swoją mapę cieni (sekcja 5.13). Dla labiryntu domyślnego mapa obejmuje 64,8 x 54,1 m przy głębokości 47,0 m, a jeden teksel ma 3,2 cm przy rozdzielczości 2048 (policzone, wartość 0,0316 m pilnuje test). Cień rzuca tylko księżyc: latarka i światła kryształów nie. Całość opisuje [`shadows.md`](shadows.md), dopasowanie pudełka do terenu sekcja 2.3 i notatka [`../../decisions/shadow-box-fitted-to-terrain.md`](../../decisions/shadow-box-fitted-to-terrain.md).

Co jest sprawdzone (2026-10-05, Windows, stan po drugiej części M6):

- **Uruchomione przeze mnie na gotowych plikach wykonywalnych** (żaden plik źródłowy nie był wtedy od nich nowszy): 256 przypadków testowych i 101232 asercje w Debug i w Release, wszystkie zaliczone. `tests/TerrainTests.cpp` ma 27 przypadków.
- **Przeliczone przeze mnie niezależnie**, skryptem w Pythonie, który czyta `heightmap.png` i powtarza wzory z `Terrain.cpp`: siatka 97 x 97 punktów, 18432 trójkąty, podłoże wewnątrz labiryntu startowego od 0,085 m do 0,461 m, najniższy punkt całej siatki 0,0 m, najwyższe wzgórze 3,37 m (punkt `x = 33`, `z = -5,5`), stopy gracza na `y = 0,124` w środku komórki startowej `(1, 1)`, `0,278` w `(9, 1)` i `0,352` w `(17, 1)`. Te same liczby zgłosił autor kodu z działającej gry.
- **Zgłoszone przez autora kodu, nie powtarzane:** build Debug i Release bez ostrzeżeń, clang-format bez uwag, obraz obejrzany na zrzutach ekranu zrobionych tymczasowymi wstawkami, które są usunięte. Liczba klatek w Release przy ustawieniach startowych to około 2000 przed zmianą i po niej: rozrzut między uruchomieniami (od 1438 do 2040) jest większy niż jakakolwiek różnica, więc pomiar mówi tylko tyle, że teren nie jest widocznym kosztem. Ograniczenie do odświeżania ekranu na tej maszynie nie działa.
- **Nikt nie sprawdził ręcznie:** suwaka, pola `Wireframe`, chodzenia po terenie klawiszami. Płynność wysokości oczu przy chodzeniu jest pokryta tylko testami jednostkowymi. Lista do odhaczenia: [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 16.
- **macOS:** nic z tej części nie było budowane ani uruchamiane ([`../../guides/build-macos.md`](../../guides/build-macos.md), sekcja 2).

M6 jest kompletny w kodzie na Windowsie i nie jest zamknięty.

## 1. Po co to jest

Do pierwszej części M6 podłoga labiryntu była zbiorem płaskich płytek 2 x 2 m, jedna na komórkę, wszystkie na `y = 0`. Temat 13 wykładu to "Implementacja podłoża": teren, którego kształt pochodzi z danych, i obiekt, który po nim chodzi. PRD zapisuje to jako "teren z heightmapy pod labiryntem, wysokość gracza z terenu".

Decyzja właściciela projektu z 2026-10-05 ([`../../decisions/gentle-terrain-under-maze.md`](../../decisions/gentle-terrain-under-maze.md)): **łagodny teren pod labiryntem**. Płytki znikają, zastępuje je siatka z mapy wysokości o małej amplitudzie, ściany są zagłębione tak, żeby nie było szpar, wysokość gracza idzie za terenem, a poza labiryntem są wzgórza.

Żeby to działało, potrzeba pięciu rzeczy:

| Rzecz | Gdzie | Sekcja |
|---|---|---|
| obraz czytany jako liczby i reguła, która zamienia liczbę na metry | `game::Heightmap`, `game::terrainRelief`, konstruktor `game::Terrain` | 2.1 do 2.5 |
| siatka trójkątów z normalnymi, UV i stycznymi | `game::buildTerrainMesh` | 2.6, 2.8, 2.9 |
| funkcja "jaka jest wysokość w punkcie `(x, z)`", zgodna co do milimetra z tym, co narysowane | `Terrain::heightAt` | 2.7 |
| wszystko, co stało na `y = 0`, postawione na nowym podłożu | `game::placeOnTerrain`, `Player::update` | 2.10, 2.11 |
| rysowanie i przełączniki | `game::TerrainRenderer`, panel Terrain | 2.12, 3, 6 |

Podział jest taki sam jak w reszcie gry: `Terrain` to dane i matematyka bez OpenGL w bibliotece `game_logic`, więc testy budują teren i pytają go o wysokości bez okna. `TerrainRenderer` to strona OpenGL w programie `night_maze`.

## 2. Teoria

### 2.1 Mapa wysokości: obraz czytany jako liczby

**Mapa wysokości** (heightmap) to obraz, w którym jasność piksela nie jest kolorem, tylko wysokością podłoża w tym miejscu. Czarny piksel to najniższy punkt, biały najwyższy. Obraz jest widokiem terenu z góry: lewa krawędź to zachód (`-X`), prawa to wschód (`+X`), górny wiersz to północ (`-Z`), dolny to południe (`+Z`).

Plik gry to `assets/textures/heightmap.png`: 256 x 256 pikseli, trzy równe kanały po 8 bitów (szary obraz zapisany jako RGB). Gra czyta tylko pierwszy kanał i dzieli go przez 255, więc każda wartość jest liczbą od 0 do 1. Obraz sam nic nie wie o metrach: ile metrów jest warta jedynka, decyduje teren (sekcja 2.3).

Dlaczego obraz, a nie wzór w kodzie albo model z Blendera:

- obraz to tablica liczb, którą umie już wczytać loader z tematu 5 ([`../assets/images.md`](../assets/images.md)),
- kształt terenu można obejrzeć i wymienić bez kompilacji,
- z tej samej tablicy bierze się i siatkę do rysowania, i wysokość pod stopami gracza. Model terenu z pliku OBJ dałby tylko pierwsze.

**Wiersze nie są odwracane.** Loader obrazów domyślnie odwraca wiersze, bo tekstury 2D mają `v = 0` na dole. Tutaj górny wiersz ma być północą, więc mapa jest wczytywana z `assets::RowOrder::TopFirst`, tak jak ściany nieba. Wczytana odwrotnie dałaby poprawny, ale lustrzany teren: północ zamieniona z południem. Nic by się nie zepsuło, tylko obraz przestałby odpowiadać terenowi.

### 2.2 Siatka regularna

Teren to **regularna siatka punktów**: punkty leżą w równych odstępach wzdłuż X i Z, a każdy ma swoją wysokość Y. Trzy stałe z `Terrain.hpp` ustalają jej kształt:

| Stała | Wartość | Znaczenie |
|---|---|---|
| `TERRAIN_STEPS_PER_CELL` | 4 | kroki siatki na jedną komórkę labiryntu, wzdłuż X i wzdłuż Z |
| `TERRAIN_SPACING` | `CELL_SIZE / 4` = 0,5 m | odległość sąsiednich punktów |
| `TERRAIN_MARGIN_CELLS` | 7 | ziemia wokół labiryntu, w komórkach z każdej strony |
| `TERRAIN_MARGIN` | `7 * CELL_SIZE` = 14 m | to samo w metrach |

**Liczba kroków na komórkę jest całkowita celowo.** Komórka ma 2 m, krok 0,5 m, więc każda granica komórki i każdy środek komórki jest punktem siatki. Ściany stoją na granicach komórek, a kryształy, start i wyjście w środkach, więc rzeczy, które mają stać na terenie, trafiają w punkty, w których wysokość jest znana dokładnie.

**Margines jest liczony w komórkach** z tego samego powodu: siatka ziemi wokół labiryntu jest przedłużeniem siatki labiryntu, bez przesunięcia o ułamek kroku.

Liczba punktów wzdłuż jednej osi dla labiryntu o `N` komórkach:

```
punkty = (N + 2 * TERRAIN_MARGIN_CELLS) * TERRAIN_STEPS_PER_CELL + 1
```

Jedynka na końcu: rząd z `K` kroków ma `K + 1` punktów (płot z 3 przęseł ma 4 słupki). Dla labiryntu startowego 10 x 10:

```
(10 + 14) * 4 + 1 = 97 punktów w rzędzie i 97 rzędów
97 * 97 = 9409 punktów (wierzchołków)
96 * 96 = 9216 kwadratów, po 2 trójkąty: 18432 trójkąty
```

Siatka zaczyna się w `x = -14`, `z = -14` (północno-zachodni róg ziemi) i kończy w `x = 34`, `z = 34`. Labirynt zajmuje środek, od 0 do 20 m na obu osiach. Cała ziemia ma 48 x 48 m.

Największy labirynt, jaki oferuje panel Maze (40 x 40), daje siatkę 217 x 217: 47089 wierzchołków i 93312 trójkątów. Na komórkę labiryntu przypadają zawsze `4 * 4 * 2 = 32` trójkąty.

### 2.3 Wzór na wysokość, krok po kroku

Wysokość punktu siatki w miejscu `(x, z)`:

```
wysokość = heightScale * relief(d) * próbka(x, z)
```

Trzy czynniki, każdy ma jedno zadanie:

| Czynnik | Zakres | Co mówi |
|---|---|---|
| `próbka(x, z)` | od 0 do 1 | kształt: wartość mapy wysokości w tym miejscu (sekcja 2.5) |
| `relief(d)` | od 0,6 m do 4,5 m | ile metrów jest warta jedynka w odległości `d` od labiryntu |
| `heightScale` | od 0 do 2,5 | suwak: mnoży wszystko, 0 daje płaski świat, 1 teren zgodny z projektem |

`d` to **odległość punktu od labiryntu** w metrach (`distanceOutsideMaze`): 0 wewnątrz labiryntu i na jego granicy, a na zewnątrz odległość do najbliższego punktu prostokąta labiryntu. Obok labiryntu to po prostu odległość do jego krawędzi, a za narożnikiem odległość w linii prostej do narożnika.

`relief(d)` (`terrainRelief`) rośnie od `MAZE_RELIEF = 0,6` do `HILL_RELIEF = 4,5`:

```
t      = d / TERRAIN_MARGIN, przycięte do zakresu od 0 do 1
relief = MAZE_RELIEF + (HILL_RELIEF - MAZE_RELIEF) * smoothstep(t)
```

Wewnątrz labiryntu `d = 0`, więc `relief = 0,6`: różnica między najciemniejszym a najjaśniejszym pikselem to najwyżej 60 cm. Na zewnętrznej krawędzi ziemi (`d = 14 m`) `relief = 4,5`: wzgórza. Za marginesem wartość zostaje na 4,5.

Skutki wzoru, które warto umieć powiedzieć bez patrzenia w kod:

- najniższe możliwe podłoże jest na `y = 0` (czarny piksel razy cokolwiek to zero), a teren nigdy nie schodzi poniżej zera,
- `heightScale = 0` daje płaski świat na `y = 0`, dokładnie taki, jak był z płytkami,
- podwojenie `heightScale` podwaja każdą wysokość, więc kształt się nie zmienia, tylko rozciąga w pionie.

**Przykład 1: środek komórki startowej, `(1, 1)`.** Punkt jest wewnątrz labiryntu, więc `d = 0` i `relief = 0,6`. Próbka mapy w tym miejscu to 0,2065 (rachunek w sekcji 2.5). Przy `heightScale = 1`:

```
wysokość = 1 * 0,6 * 0,2065 = 0,124 m
```

Tyle wynosi `y` stóp gracza na starcie.

**Przykład 2: punkt na wzgórzach, `(-7, 10)`.** Punkt leży 7 m na zachód od labiryntu, w połowie marginesu.

```
d      = 7
t      = 7 / 14 = 0,5
smoothstep(0,5) = 0,5 * 0,5 * (3 - 2 * 0,5) = 0,25 * 2 = 0,5
relief = 0,6 + (4,5 - 0,6) * 0,5 = 0,6 + 1,95 = 2,55 m
próbka = 0,4693
wysokość = 1 * 2,55 * 0,4693 = 1,197 m
```

Ta sama wartość mapy (około 0,47) wewnątrz labiryntu dałaby `0,6 * 0,47 = 0,28 m`. To jest cały pomysł: jeden obraz, a o tym, czy jest pagórkiem, czy nierównością pod butem, decyduje odległość od labiryntu.

**Dlaczego wewnątrz labiryntu startowego różnica wynosi 0,38 m, a nie 0,6 m.** `MAZE_RELIEF` to wysokość bieli nad czernią. Kawałek mapy, który wypada pod labiryntem startowym, nie sięga od czerni do bieli: jego wartości są od 0,141 do 0,769, co daje podłoże od 0,085 m do 0,461 m. Test w `TerrainTests.cpp` pilnuje, żeby ta różnica była między 0,3 m a 0,5 m.

### 2.4 Smoothstep: przejście bez załamania

`smoothstep` to krzywa

```
smoothstep(t) = 3t² - 2t³ = t * t * (3 - 2t)      dla t od 0 do 1
```

Zaczyna się w 0, kończy w 1, a na obu końcach jest płaska: jej pochodna `6t - 6t²` wynosi 0 dla `t = 0` i dla `t = 1`. Kilka wartości:

| `t` | 0 | 0,25 | 0,5 | 0,75 | 1 |
|---|---|---|---|---|---|
| `smoothstep(t)` | 0 | 0,156 | 0,5 | 0,844 | 1 |

Dlaczego nie zwykła prosta `relief = 0,6 + 3,9 * t`. Prosta ma na granicy labiryntu skok nachylenia: wewnątrz relief jest stały, tuż za granicą zaczyna rosnąć ze stałą prędkością. Na terenie byłoby to widać jako załamanie biegnące dokładnie wzdłuż zewnętrznych ścian. Smoothstep startuje z zerowym nachyleniem, więc łagodne podłoże labiryntu przechodzi we wzgórza bez kantu. Na drugim końcu (krawędź ziemi) płaski koniec krzywej nie ma dziś znaczenia, bo dalej nie ma już terenu.

W kodzie funkcja nazywa się `smoothStep` i jest napisana ręcznie w `Terrain.cpp`. GLSL ma wbudowaną `smoothstep`, ale ten rachunek dzieje się na procesorze.

### 2.5 Odczyt mapy: dwuliniowo i z powtarzaniem

Punkt siatki rzadko trafia dokładnie w piksel mapy. `Heightmap::sample(u, v)` zwraca wartość **między** pikselami:

- `u` biegnie wzdłuż wiersza, `v` od wiersza do wiersza. Krok o 1 to jeden cały obraz,
- wartość numer `i` w wierszu leży w `u = i / width`,
- z `u` i `v` brana jest tylko część ułamkowa, więc obraz powtarza się w obie strony: `u = 1,25` czyta to samo co `u = 0,25`, a `u = -0,25` to samo co `u = 0,75`,
- wynik to **mieszanie dwuliniowe** czterech wartości wokół miejsca: najpierw wzdłuż wiersza w dwóch sąsiednich wierszach, potem między wierszami. Za ostatnią wartością wiersza następna jest znów pierwsza.

Teren woła `sample(x / HEIGHTMAP_SPAN, z / HEIGHTMAP_SPAN)`, gdzie `HEIGHTMAP_SPAN = 48`. Jedno powtórzenie obrazu pokrywa więc kwadrat 48 x 48 m, a piksel mapy ma na ziemi `48 / 256 = 0,1875 m`. Lewy górny róg obrazu leży w początku układu świata, czyli w północno-zachodnim rogu labiryntu, a nie w rogu ziemi: margines na północ i na zachód czyta obraz "z drugiej strony" dzięki powtarzaniu.

**Rachunek dla `(1, 1)`.** `u = v = 1 / 48 = 0,02083`. W pikselach: `0,02083 * 256 = 5,333`. Miejsce leży więc między kolumnami 5 i 6 i między wierszami 5 i 6, w jednej trzeciej drogi. Cztery wartości z pliku (kanał czerwony): wiersz 5 ma 52 i 55, wiersz 6 ma 51 i 54.

```
wiersz 5 (północ):    52 + (55 - 52) * 0,333 = 53
wiersz 6 (południe):  51 + (54 - 51) * 0,333 = 52
między wierszami:     53 + (52 - 53) * 0,333 = 52,667
próbka = 52,667 / 255 = 0,2065
```

**Rachunek dla `(-7, 10)`,** z ujemnym `u`. `u = -7 / 48 = -0,1458`, część ułamkowa to `1 - 0,1458 = 0,8542`, w pikselach 218,667. `v = 10 / 48 = 0,2083`, w pikselach 53,333. Wartości: wiersz 53 ma 122 i 119, wiersz 54 ma 121 i 118.

```
wiersz 53:  122 + (119 - 122) * 0,667 = 120
wiersz 54:  121 + (118 - 121) * 0,667 = 119
między:     120 + (119 - 120) * 0,333 = 119,667
próbka = 119,667 / 255 = 0,4693
```

**Dlaczego mapa jest kafelkowana w metrach świata, a nie rozciągana na cały teren.** Najprostszy wariant to `u = (x - minX) / szerokość terenu`: obraz zawsze pokrywa cały teren raz. Ma poważną wadę: wielkość wzgórz zależałaby od wielkości labiryntu. W labiryncie 40 x 40 ziemia ma 108 m, więc każdy pagórek byłby ponad dwa razy szerszy i ponad dwa razy łagodniejszy niż w labiryncie startowym, a w labiryncie 2 x 2 (32 m) ciaśniejszy i bardziej stromy. Suwak rozmiaru w panelu Maze zmieniałby charakter terenu. Przy kafelkowaniu w metrach pagórek ma zawsze tę samą wielkość (pierwsza oktawa szumu to 12 m), a większy labirynt po prostu widzi więcej powtórzeń obrazu. 48 m to labirynt startowy (20 m) z marginesem z obu stron (2 x 14 m), więc przy ustawieniach startowych wypada dokładnie jedno powtórzenie. Ceną jest warunek dla obrazu: lewa krawędź musi pasować do prawej, a górna do dolnej, inaczej co 48 m byłby uskok. Skrypt, który robi obraz, zapewnia to sam (sekcja 5.14). Notatka: [`../../decisions/heightmap-tiled-in-world-metres.md`](../../decisions/heightmap-tiled-in-world-metres.md).

### 2.6 Trójkąty i konwencja przekątnej

Punkty siatki to dopiero wierzchołki. Powierzchnię robią z nich trójkąty: każdy kwadrat siatki (cztery sąsiednie punkty) jest cięty przekątną na dwa.

Cztery narożniki kwadratu, widziane z góry, z północą u góry:

```
 NW ------- NE         NW = północny zachód: najmniejsze x i z
  | \       |          NE = północny wschód
  |   \     |          SW = południowy zachód
  |     \   |          SE = południowy wschód: największe x i z
 SW ------- SE
```

**Konwencja przekątnej w tym projekcie: od północnego zachodu do południowego wschodu.** Dwa trójkąty to:

- północno-wschodni: `NW`, `SE`, `NE`,
- południowo-zachodni: `NW`, `SW`, `SE`.

Kwadrat można przeciąć też drugą przekątną (od `NE` do `SW`). Jeśli cztery narożniki nie leżą w jednej płaszczyźnie, a prawie nigdy nie leżą, te dwa cięcia dają **różne powierzchnie**: jedno robi z kwadratu grzbiet, drugie dolinę. Dlatego konwencja musi być jedna i obowiązywać w dwóch miejscach naraz: w `buildTerrainMesh`, która układa indeksy do rysowania, i w `Terrain::heightAt`, która liczy wysokość pod stopami. Gdyby się różniły, gracz chodziłby po innej powierzchni niż ta, którą widzi. Komentarz klasy `Terrain` zapisuje konwencję wielkimi literami, a test `heightAt agrees with the triangle of the mesh at random points` sprawdza zgodność na gotowej siatce.

**Kierunek nawijania.** Oba trójkąty są podane przeciwnie do ruchu wskazówek zegara, gdy patrzy się z góry, więc ich przednie strony patrzą w górę. Sprawdzenie dla `NW`, `SE`, `NE` w kwadracie o boku 1 (bez wysokości): `SE - NW = (1, 0, 1)`, `NE - NW = (1, 0, 0)`, a iloczyn wektorowy `(1, 0, 1) × (1, 0, 0) = (0, 1, 0)`: w górę. Gra nie włącza dziś odrzucania tylnych ścian, ale siatka jest na to gotowa.

### 2.7 Wysokość w dowolnym punkcie: `heightAt` na trójkącie

Gracz, kryształ i kępka trawy prawie nigdy nie stoją w punkcie siatki. `heightAt(x, z)` ma zwrócić wysokość powierzchni dokładnie tam, gdzie zostanie narysowana.

**Krok 1: który kwadrat.** Pozycja jest przeliczana na kroki siatki od północno-zachodniego rogu:

```
gridX = (x - minX) / spacing        gridZ = (z - minZ) / spacing
```

Część całkowita to kolumna i wiersz narożnika `NW` kwadratu, część ułamkowa to położenie wewnątrz kwadratu: `alongX` od 0 (zachód) do 1 (wschód) i `alongZ` od 0 (północ) do 1 (południe).

**Krok 2: który trójkąt.** Przekątna od `NW` do `SE` to linia `alongX == alongZ`. Jeśli `alongX >= alongZ`, punkt leży w trójkącie północno-wschodnim, w przeciwnym razie w południowo-zachodnim.

**Krok 3: wysokość na płaskim trójkącie.** Trójkąt jest kawałkiem płaszczyzny, więc wysokość zmienia się liniowo. Wystarczy wystartować z narożnika `NW` i przejść po dwóch krawędziach trójkąta, za każdym razem dodając odpowiednią część różnicy wysokości:

```
trójkąt północno-wschodni (NW, NE, SE):
    h = NW + alongX * (NE - NW) + alongZ * (SE - NE)
    na wschód krawędzią północną, potem na południe krawędzią wschodnią

trójkąt południowo-zachodni (NW, SW, SE):
    h = NW + alongZ * (SW - NW) + alongX * (SE - SW)
    na południe krawędzią zachodnią, potem na wschód krawędzią południową
```

Sprawdzenie w narożnikach pierwszego wzoru: dla `(0, 0)` wychodzi `NW`, dla `(1, 0)` wychodzi `NE`, dla `(1, 1)` wychodzi `NW + (NE - NW) + (SE - NE) = SE`. Na przekątnej (`alongX = alongZ = t`) oba wzory dają to samo, `NW + t * (SE - NW)`, więc powierzchnia nie ma uskoku na przekątnej.

**Przykład na liczbach z gry.** Punkt `(3,3, 1,1)` w labiryncie startowym. `gridX = (3,3 + 14) / 0,5 = 34,6`, `gridZ = (1,1 + 14) / 0,5 = 30,2`. Kwadrat ma narożnik `NW` w kolumnie 34 i wierszu 30 (czyli w `x = 3`, `z = 1`), a `alongX = 0,6`, `alongZ = 0,2`. Wysokości narożników, z pięcioma cyframi, żeby rachunek dało się sprawdzić: `NW = 0,19765`, `NE = 0,20784`, `SW = 0,19294`, `SE = 0,20157`. `0,6 >= 0,2`, więc trójkąt północno-wschodni:

```
h = 0,19765 + 0,6 * (0,20784 - 0,19765) + 0,2 * (0,20157 - 0,20784)
  = 0,19765 + 0,00611 - 0,00125
  = 0,2025 m
```

**Dlaczego nie zwykłe mieszanie dwuliniowe czterech narożników.** Mieszanie dwuliniowe (takie jak w `Heightmap::sample`) daje gładką, lekko wygiętą powierzchnię siodłową. Karta graficzna rysuje co innego: dwa płaskie trójkąty. Obie powierzchnie zgadzają się na krawędziach kwadratu, ale w środku się różnią. W samym środku kwadratu:

```
trójkąty (punkt na przekątnej):  (NW + SE) / 2
dwuliniowo:                      (NW + NE + SW + SE) / 4
różnica:                         (NW + SE - NE - SW) / 4
```

Najprostszy przykład: `NW = 0`, `NE = 1`, `SW = 1`, `SE = 0`. Trójkąty mają w środku wysokość 0 (przekątna łączy dwa zera, to dno doliny), a mieszanie dwuliniowe daje 0,5. Rzecz postawiona wynikiem dwuliniowym wisiałaby pół metra nad narysowaną powierzchnią.

Uczciwie o skali: w terenie gry różnica jest mała, bo podłoże jest gładkie względem kroku 0,5 m. W labiryncie startowym największa różnica w środku kwadratu to 2,4 mm wewnątrz labiryntu i 14 mm na wzgórzach (przy `heightScale = 1`). Powód wyboru jest więc zasadą, nie ratowaniem obrazu: **funkcja wysokości ma opisywać dokładnie tę powierzchnię, która jest rysowana**. Wtedy test może porównać ją z siatką z tolerancją jednej setnej milimetra, a nie "mniej więcej", i żadna zmiana mapy na bardziej poszarpaną niczego nie rozjedzie.

**Poza siatką.** `gridX` i `gridZ` są przycinane do zakresu siatki, więc punkt poza ziemią dostaje wysokość najbliższego punktu jej brzegu. Nic nie rzuca wyjątku: gracz w trybie noclip może wylecieć poza teren.

### 2.8 Normalne z różnic centralnych

Oświetlenie potrzebuje normalnej w każdym wierzchołku. Teren to powierzchnia `y = h(x, z)`, a jej normalna wynika z dwóch nachyleń:

```
nachylenie wzdłuż X:  sx = (h na wschód - h na zachód) / odległość tych dwóch punktów
nachylenie wzdłuż Z:  sz = (h na południe - h na północ) / odległość tych dwóch punktów
normalna = normalize(-sx, 1, -sz)
```

To są **różnice centralne**: nachylenie w punkcie liczone z sąsiadów po obu stronach, a nie z samego punktu i jednego sąsiada. Wynik jest symetryczny i gładszy. Ten sam pomysł robi mapę normalnych z pola wysokości w skrypcie tekstur ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md)).

Skąd `(-sx, 1, -sz)`. Idąc o 1 m na wschód, powierzchnia podnosi się o `sx`, więc wektor leżący na powierzchni wzdłuż X to `(1, sx, 0)`. Wzdłuż Z to `(0, sz, 1)`. Normalna jest prostopadła do obu, czyli jest ich iloczynem wektorowym: `(0, sz, 1) × (1, sx, 0) = (-sx, 1, -sz)`. Słownie: tam, gdzie teren wznosi się ku `+X`, powierzchnia odchyla się w stronę `-X`.

Na brzegu siatki brakuje jednego sąsiada. Zastępuje go sam punkt, a odległość w mianowniku jest wtedy jednym krokiem zamiast dwóch (kod liczy ją z numerów kolumn, więc dzieli zawsze przez prawdziwą odległość).

**Przykład: punkt siatki w `(1, 1)`.** Sąsiad na wschodzie jest o 0,0345 m wyżej niż sąsiad na zachodzie, a sąsiad na południu o 0,0102 m niżej niż na północy. Sąsiedzi są 1 m od siebie (dwa kroki po 0,5 m), więc `sx = 0,0345`, `sz = -0,0102`, a normalna po normalizacji to `(-0,0345, 0,9994, 0,0102)`: prawie prosto w górę, z lekkim odchyleniem na zachód i południe.

Normalna jest jedna na wierzchołek i wspólna dla wszystkich trójkątów, które się w nim stykają (do sześciu), więc cieniowanie jest gładkie: krawędzi trójkątów nie widać w świetle.

### 2.9 Współrzędne tekstury i styczne

Tekstura podłoża ma 512 x 512 pikseli i powtarza się co `GROUND_TEXTURE_SPAN = 4` m:

```
u =  x / 4
v = -z / 4
```

Współrzędne wychodzą daleko poza zakres od 0 do 1 (na ziemi 48 m od -3,5 do 8,5), a powtarzanie robi sampler tekstury z zawijaniem `GL_REPEAT` ([`../gfx/textures.md`](../gfx/textures.md)). Jedno powtórzenie pokrywa cztery komórki labiryntu (2 x 2), więc wzór nie powtarza się w każdej komórce tak, jak powtarzał się na płytkach.

**Minus przy `z`.** Na obrazie `v` rośnie w górę. Patrząc na teren z góry z północą u góry, "w górę obrazu" to północ, czyli `-Z`. Z minusem tekstura leży tak, jak wygląda w pliku. Bez minusa byłaby odbita lustrzanie, a odbicie ma skutek większy niż wygląd: shader mapy normalnych liczy bitangentę jako `cross(N, T)`, co jest poprawne tylko dla tekstury nieodbitej. Z odbitą teksturą nierówności podłoża świeciłyby od złej strony. Pilnuje tego test (`countMirroredTriangles == 0`).

**Styczne** (kierunek na powierzchni, w którym rośnie `u`) liczy ta sama funkcja co dla modeli z plików, `assets::computeTangents`, z krawędzi trójkątów i różnic UV. Na terenie styczna wychodzi w stronę `+X`, przechylona razem z powierzchnią. Mapa normalnych podłoża działa więc tak samo jak na ścianach.

### 2.10 Jak rzeczy stoją na terenie

Zasada dla całego kodu: **nic nie przesuwa się w poziomie, zmienia się tylko wysokość.** Plan labiryntu (które ściany, gdzie słupki, gdzie kryształy) jest liczony tak jak przed terenem, na `y = 0`, a potem funkcja `placeOnTerrain` ustawia wysokości.

| Co | Wysokość | Dlaczego tak |
|---|---|---|
| ściana, słupek, brama | **najniższy narożnik siatki** pod obrysem (z zapasem `FOOTPRINT_MARGIN = 0,05 m`) | sztywny prostopadłościan na nierównym gruncie: albo gdzieś wisi, albo gdzieś jest zakopany. Zakopanie jest niewidoczne |
| start gracza, pozycja wyjścia | `heightAt` w środku komórki | to punkty, nie bryły |
| strefa wyjścia | pudełko stojące na `heightAt` w środku komórki wyjścia | ma obejmować gracza, który tam stoi |
| kryształ | `heightAt` w środku komórki plus `CRYSTAL_FLOAT_HEIGHT = 0,9 m` | unosi się nad ziemią zawsze na tej samej wysokości względem niej |
| stopy gracza | `heightAt` pod stopami, w każdym kroku symulacji | sekcja 2.11 |
| kępka trawy | `heightAt` w miejscu kępki | [`grass-geometry.md`](grass-geometry.md) |

**Dlaczego najniższy narożnik wystarcza.** Powierzchnia nad obrysem składa się z płaskich trójkątów. Wysokość w dowolnym punkcie trójkąta jest średnią ważoną wysokości jego trzech narożników, z wagami nieujemnymi, które sumują się do 1. Średnia ważona nigdy nie jest mniejsza od najmniejszego składnika, więc **płaski trójkąt nigdzie nie jest niżej niż jego najniższy narożnik**. Jeśli podstawa ściany stoi na wysokości najniższego narożnika ze wszystkich kwadratów, których dotyka obrys, to pod żadnym punktem podstawy teren nie jest niżej: szpary nie ma. Nie trzeba badać powierzchni punkt po punkcie.

`lowestHeightUnder(minX, minZ, maxX, maxZ)` bierze kolumny i wiersze siatki, które **obejmują** prostokąt (ostatnia przed początkiem, pierwsza za końcem), i zwraca najmniejszą wysokość wśród ich punktów. Dla ściany wzdłuż X o środku w `(1, 0)`: pudełko kolizji ma w poziomie 2 x 0,3 m, z zapasem 2,1 x 0,4 m, czyli `x` od -0,05 do 2,05 i `z` od -0,2 do 0,2. Obejmujące linie siatki to `x` od -0,5 do 2,5 (7 kolumn) i `z` od -0,5 do 0,5 (3 wiersze): 21 punktów. Dla słupka (0,4 x 0,4 m z zapasem) to 3 x 3 = 9 punktów.

**Po co zapas `FOOTPRINT_MARGIN`.** Modele są przy ziemi szersze niż ich pudełka kolizji: stopa słupka ma 0,4 m, a jego pudełko 0,3 m. Sprawdzany musi być obrys całego modelu, bo szpara pod wystającą stopą też byłaby widoczna. 5 cm z każdej strony to dokładnie ta różnica.

**Cena.** Ściana jest z jednej strony zakopana. W labiryncie startowym przy `heightScale = 1` różnica między najniższym a najwyższym punktem siatki pod obrysem ściany wynosi najwyżej 0,23 m (policzone dla każdej krawędzi komórki, na której może stanąć ściana), a pod słupkiem najwyżej 0,10 m. Ściana ma 3 m, więc w najgorszym miejscu wystaje z ziemi 2,77 m zamiast 3 m. Wysokości pudełek kolizji się nie zmieniają: pudełko przesuwa się w dół razem z modelem. Notatka: [`../../decisions/walls-sunk-to-lowest-corner.md`](../../decisions/walls-sunk-to-lowest-corner.md).

**Brama** jest segmentem ściany, więc stoi tak samo. Gdy się otwiera, zjeżdża o `GATE_SINK_DEPTH = 3,3 m` poniżej pozycji zamkniętej, a ta jest już na najniższym gruncie pod nią, więc otwarta brama znika pod terenem w całości.

### 2.11 Gracz na terenie, kolizje i `MAX_HEIGHT_SCALE`

**Stopy.** Przed terenem `Player::update` ustawiał `position.y = FLOOR_Y`, czyli zero. Teraz czyta wysokość z terenu, dwa razy w kroku: przed ruchem i po nim. Nie ma grawitacji, skoku ani spadania: wysokość jest po prostu **odczytywana**. Gracz nie zderza się z ziemią, tylko jest na niej stawiany.

**Ruch jest nadal poziomy.** Klawisze przesuwają gracza w płaszczyźnie XZ z prędkością `walkSpeed` albo `sprintSpeed`, a wysokość dochodzi potem. Prędkość liczona po ziemi (w rzucie z góry) jest więc taka sama pod górę i w dół. Prawdziwa droga po zboczu jest trochę dłuższa, ale przy nachyleniach w labiryncie (najwyżej 8 cm na metr wzdłuż osi między sąsiednimi punktami siatki w labiryncie startowym) różnica jest poniżej jednego procenta.

**Kolizje w poziomie są bez zmian.** `placeOnTerrain` nie przesuwa niczego w bok, więc rzut każdego pudełka na płaszczyznę XZ jest taki sam jak na płaskim świecie. Test `the walls stop the player on uneven ground exactly as on flat ground` przechodzi ten sam labirynt tymi samymi klawiszami raz na płaskim gruncie, raz na poszarpanym przy największej skali wysokości, i wymaga, żeby `x` i `z` gracza zgadzały się w każdym kroku.

**Ale test nakładania pudełek jest trójwymiarowy.** `scene::overlaps` sprawdza trzy osie: pudełka kolidują tylko wtedy, gdy ich przedziały nakładają się także na Y ([`../scene/collision.md`](../scene/collision.md)). Na płaskim świecie to było oczywiste: i gracz, i ściana stały na zerze. Na terenie ściana stoi na najniższym gruncie pod **swoim** obrysem, a gracz na gruncie pod **swoimi** stopami. Gdyby różnica tych dwóch wysokości mogła przekroczyć wysokość gracza, pudełko gracza znalazłoby się w całości nad albo pod pudełkiem ściany i gracz przeszedłby przez ścianę.

Tego pilnuje `MAX_HEIGHT_SCALE = 2,5`. Wewnątrz labiryntu relief to `MAZE_RELIEF = 0,6 m` na jedynkę skali, więc największa możliwa różnica wysokości między dwoma dowolnymi punktami labiryntu to:

```
0,6 m * 2,5 = 1,5 m
```

Gracz ma `BODY_HEIGHT = 1,8 m`, ściana 3 m. Dwa przypadki:

- gracz stoi **niżej** niż podstawa ściany, najwyżej o 1,5 m: jego pudełko sięga od stóp 1,8 m w górę, więc wchodzi w pudełko ściany co najmniej na 0,3 m,
- gracz stoi **wyżej** niż podstawa ściany, najwyżej o 1,5 m: ściana sięga 3 m w górę od podstawy, więc stopy gracza są co najmniej 1,5 m poniżej jej górnej krawędzi.

W obu przypadkach przedziały na Y się nakładają, więc o kolizji decydują tylko X i Z, jak dawniej. Rachunek celowo używa pełnych 1,5 m, a nie 0,38 m z labiryntu startowego: mapa jest kafelkowana, więc duży labirynt może trafić i na czarny, i na biały piksel.

Granicy pilnują trzy miejsca: suwak (`ImGuiSliderFlags_AlwaysClamp`), `rebuildTerrain` i `regenerateMaze` (oba przycinają wartość przez `std::clamp`, bo do suwaka da się też wpisać liczbę).

**Interpolacja wysokości.** Gra rysuje klatkę z pozycji zmieszanej z dwóch ostatnich kroków symulacji ([`../core/main-loop.md`](../core/main-loop.md)). Wysokość stóp jest mieszana tak samo jak `x` i `z`, więc oczy suną po terenie zamiast poruszać się schodkami. Jest jeden wyjątek: krok tuż po wyłączeniu noclip w powietrzu, w którym stopy spadają na ziemię. To skok, a nie ruch, więc nie jest mieszany (pole `m_playerWasFlying` w `NightMazeApp`). Szczegóły: [`../game/player.md`](../game/player.md).

### 2.12 Wireframe

**Wireframe** (siatka z drutu) to rysowanie samych krawędzi trójkątów. Dla terenu jest to najprostszy sposób, żeby zobaczyć, z czego jest zrobiony: regularne kwadraty przecięte przekątną zawsze w tę samą stronę.

OpenGL ma do tego jedno wywołanie stanu, `glPolygonMode`. Mówi rasteryzatorowi, co zrobić z wielokątem: wypełnić (`GL_FILL`, domyślnie), narysować krawędzie (`GL_LINE`) albo same wierzchołki (`GL_POINT`). Reszta potoku jest bez zmian: te same shadery, te same tekstury i światło, więc linie mają kolor podłoża.

Dwie rzeczy do zapamiętania:

- **w profilu Core pierwszym argumentem może być tylko `GL_FRONT_AND_BACK`.** Dawny OpenGL pozwalał ustawić tryb osobno dla przedniej i tylnej strony. Profil Core to usunął: `GL_FRONT` albo `GL_BACK` daje `GL_INVALID_ENUM`,
- **to stan globalny.** Zostaje do następnej zmiany. Gdyby `TerrainRenderer::draw` go nie przywrócił, ściany, kryształy i niebo narysowane po terenie też stałyby się liniami.

Wireframe pokazuje też, że teren nie ma dna: przez linie widać kolor czyszczenia ekranu i niebo pod horyzontem.

**Wireframe a cienie (czwarta część M7).** Pole `Wireframe` dotyczy tylko przebiegu sceny. Do mapy cieni teren jest rysowany zawsze z wypełnionymi trójkątami: `NightMazeApp::drawShadowCasters` nie przekazuje dalej `m_terrainSettings.wireframe`, tylko stałą.

```cpp
    // The terrain is always drawn filled here: the wireframe switch is a way to look
    // at the ground, and a ground of lines would cast a shadow of lines.
    constexpr bool NO_WIREFRAME = false;
    m_terrainRenderer.draw(m_shadowDepthShader, NO_WIREFRAME);
```

Powód jest w komentarzu: wireframe to sposób oglądania gruntu, a grunt z samych linii rzucałby cień z samych linii. Skutek: po zaznaczeniu pola cienie wzgórz nie znikają, a linie siatki rysowane w scenie nadal są cieniowane mapą cieni jak wypełniony grunt. To wynika z kodu: obrazu z `Wireframe` i cieniami naraz nikt nie zgłosił.

## 3. Jak to działa w OpenGL

Teren nie wprowadza żadnego nowego rodzaju obiektu OpenGL. Jest jedną `gfx::Mesh` ([`../gfx/mesh.md`](../gfx/mesh.md)) rysowaną jednym wywołaniem.

### 3.1 Wysłanie siatki: `TerrainRenderer::upload`

Wywoływane przy starcie, po każdym nowym labiryncie i po każdej zmianie skali wysokości.

| # | Co się dzieje | Wywołania OpenGL |
|---|---|---|
| 1 | powstaje nowa `gfx::Mesh` z wierzchołków i indeksów | `glGenVertexArrays`, `glGenBuffers` dwa razy, `glBufferData` dla `GL_ARRAY_BUFFER` i `GL_ELEMENT_ARRAY_BUFFER`, cztery atrybuty przez `glVertexAttribPointer` |
| 2 | przypisanie przenoszące `m_mesh = gfx::Mesh(...)` usuwa bufory starej siatki | `glDeleteBuffers`, `glDeleteVertexArrays` w destruktorach starych obiektów |

Dla labiryntu startowego to 9409 wierzchołków po 44 bajty (413 996 bajtów) i 55 296 indeksów po 4 bajty (221 184 bajty). `gfx::Mesh` jest wypełniana raz, przy utworzeniu, więc zmiana terenu to nowa siatka, a nie podmiana danych w starej. Przy przeciąganiu suwaka `Height scale` dzieje się to w każdej klatce, w której wartość się zmieniła.

### 3.2 Klatka: `TerrainRenderer::draw`

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `setModelSamplers(shader)` | `uTexture` czyta jednostkę 0, `uNormalMap` jednostkę 1 |
| 2 | `shader.setVec3("uEmissive", 0)` | ziemia nie świeci sama |
| 3 | `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`, tylko gdy `wireframe` | rasteryzator rysuje krawędzie |
| 4 | `drawMesh(...)`: wiąże `ground_normal.png` na jednostce 1 i `ground.png` na jednostce 0, ustawia `uTint`, `uModel`, `uNormalMatrix`, woła `mesh.draw()` | `glActiveTexture`, `glBindTexture`, `glBindSampler`, `glUniform*`, `glBindVertexArray`, `glDrawElements(GL_TRIANGLES, 55296, GL_UNSIGNED_INT, ...)` |
| 5 | `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)`, tylko gdy `wireframe` | stan wraca do wypełniania |

Jedno wywołanie rysujące na cały teren w przebiegu sceny. Od czwartej części M7 dochodzi drugie, takie samo, w przebiegu cieni: teren jest rysowany do mapy cieni programem `shadow_depth` tą samą funkcją `TerrainRenderer::draw`, więc przy włączonych cieniach kosztuje dwa `glDrawElements` na klatkę (kroki 3 i 5 w przebiegu cieni nie występują, bo `wireframe` jest tam zawsze `false`). Dla porównania: płytki podłogi kosztowały jedno wywołanie na komórkę, czyli 100 w labiryncie startowym.

### 3.3 Miejsce w klatce

Teren jest rysowany jako pierwsza rzecz sceny, w `drawUnlitMaze` albo `drawLitMaze`, tym samym programem co ściany. Od czwartej części M7 jest też pierwszą rzeczą przebiegu cieni, który stoi przed sceną. Kolejność w `onRender`:

1. przebieg cieni księżyca do mapy cieni (`drawMoonShadowMap`, przy włączonych cieniach): teren, ściany i słupki, brama i kryształy, bez trawy,
2. teren, ściany i słupki, kryształy i brama (`drawMaze`),
3. trawa (`drawGrass`),
4. linie pudełek kolizji, jeśli włączone,
5. niebo.

Kolejność terenu względem ścian nie zmienia obrazu, bo o tym, co jest z przodu, decyduje test głębi. Komentarz w kodzie mówi to wprost: "ziemia najpierw, potem to, co na niej stoi" to tylko porządek zgodny z budową sceny.

## 4. Shadery

Teren **nie ma własnych shaderów**. Jest rysowany jak model: programem `textured` w trybie `Unlit` i w widokach diagnostycznych, programem `lit` w trybach `Phong` i `Blinn-Phong`, programem `gouraud` w trybie `Gouraud`. Dostaje więc za darmo wszystko, co mają ściany: cztery tryby cieniowania, mapę normalnych, widoki `Normals as colour` i `UVs as colour`, a od czwartej części M7 także cień księżyca w programach `lit` i `gouraud` (program `textured` mapy cieni nie czyta, więc w trybie `Unlit` i w widokach diagnostycznych cieni nie ma). Od tej samej części jest rysowany jeszcze czwartym programem, `shadow_depth`, do mapy cieni ([`shadows.md`](shadows.md), sekcje 2.14 i 4). Shadery opisują [`../gfx/textures.md`](../gfx/textures.md) (`textured`), [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) (`lit`, `gouraud`) i [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md).

Uniformy, które `TerrainRenderer::draw` ustawia dla terenu:

| Uniform | Wartość | Dlaczego |
|---|---|---|
| `uModel` | macierz jednostkowa | wierzchołki terenu są już w przestrzeni świata: `buildTerrainMesh` wpisuje prawdziwe `x`, `y`, `z` |
| `uNormalMatrix` | `scene::normalMatrix(jednostkowa)`, czyli jednostkowa 3 x 3 | normalne też są już w przestrzeni świata |
| `uTint` | `(1, 1, 1)` | podłoże ma kolory swojej tekstury, biały odcień niczego nie zmienia |
| `uEmissive` | `(0, 0, 0)` | ziemia nie świeci. Uniform zachowuje wartość między wywołaniami, a kryształy go ustawiają, więc trzeba go zerować w każdej klatce |
| `uTexture`, `uNormalMap` | 0 i 1 | numery jednostek teksturujących |

Materiał (siła i wykładnik odbłysku, `uSpecularStrength` i `uShininess`) jest ustawiany raz na program w `drawLitMaze`, więc podłoże dzieli go z kamieniem ścian. Tak samo siedem uniformów mapy cieni księżyca (`setShadowUniforms`, od czwartej części M7): raz na program, przed terenem.

W przebiegu cieni ta sama funkcja ustawia te same uniformy w programie `shadow_depth`. Ten program ma z nich tylko `uModel` (i dwie macierze światła, ustawione wcześniej w `drawShadowCasters`). Pozostałych nie ma, więc `glUniform*` dostaje lokalizację -1 i OpenGL takie wywołanie po cichu pomija. Tekstury gruntu są mimo to wiązane w jednostkach 0 i 1, choć program głębi ich nie czyta: to cena użycia klasy rysującej bez zmian.

**Teren a tryb Gouraud.** Gouraud liczy światło w wierzchołkach. Płytka podłogi miała cztery wierzchołki na 2 x 2 m, więc stożek latarki gubił się na niej prawie w całości. Teren ma wierzchołek co 0,5 m, czyli szesnaście razy więcej punktów na tej samej powierzchni, więc światło liczone w wierzchołkach ma na nim znacznie gęstsze próbki. Jak wygląda na nim stożek latarki w trybie Gouraud, nikt jeszcze nie oglądał na żywo. Mapy normalnych w trybie Gouraud nie ma, jak na ścianach. Cień księżyca jest w tym trybie jedyną rzeczą liczoną na fragment (czwarta część M7), więc krawędź cienia ściany na gruncie nie zależy od gęstości siatki terenu i wypada tam, gdzie w trybie Phong ([`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), [`shadows.md`](shadows.md), sekcja 2.15).

Jedyna zmiana w plikach shaderów związana z terenem to słowo w komentarzu `lit.frag` (`floor` na `ground`).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/game/Terrain.hpp`](../../../src/game/Terrain.hpp), [`.cpp`](../../../src/game/Terrain.cpp) | stałe, `TerrainSettings`, `Heightmap`, `heightmapFromImage`, `distanceOutsideMaze`, `terrainRelief`, klasa `Terrain`, `TerrainMeshData`, `buildTerrainMesh` |
| [`src/game/MazeWorld.hpp`](../../../src/game/MazeWorld.hpp), [`.cpp`](../../../src/game/MazeWorld.cpp) | pole `MazeWorld::terrain`, `FOOTPRINT_MARGIN`, `groundHeightAt`, `placeOnTerrain`, druga wersja `buildMazeWorld` |
| [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`.cpp`](../../../src/game/MazeLayout.cpp) | `colliderBoxes(walls, pillars)`: pudełka z podanych, już opuszczonych ścian i słupków |
| [`src/game/Crystals.*`](../../../src/game/Crystals.cpp), [`Exit.*`](../../../src/game/Exit.cpp), [`Round.*`](../../../src/game/Round.cpp) | parametr `groundHeight` w `crystalRestPosition` i `exitZone`, funkcja `restCrystalsOnGround` |
| [`src/game/Player.hpp`](../../../src/game/Player.hpp), [`.cpp`](../../../src/game/Player.cpp) | parametr `terrain` w `update`, stała `FLOOR_Y` usunięta |
| [`src/game/TerrainRenderer.hpp`](../../../src/game/TerrainRenderer.hpp), [`.cpp`](../../../src/game/TerrainRenderer.cpp) | siatka na karcie, dwie tekstury, `upload`, `draw` |
| [`src/game/ModelDraw.hpp`](../../../src/game/ModelDraw.hpp), [`.cpp`](../../../src/game/ModelDraw.cpp) | `drawMesh`: jedna siatka z podanymi teksturami |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | `loadHeightmap`, pola `m_heightmap`, `m_terrainSettings`, `m_terrainRenderer`, `m_playerWasFlying`, funkcje `rebuildTerrain` i `uploadGround` |
| [`src/debug/panels/TerrainPanel.hpp`](../../../src/debug/panels/TerrainPanel.hpp), [`.cpp`](../../../src/debug/panels/TerrainPanel.cpp) | panel Terrain (sekcja 6) |
| [`assets/textures/heightmap.png`](../../../assets/textures/heightmap.png) | mapa wysokości, 256 x 256 |
| [`assets/textures/ground.png`](../../../assets/textures/ground.png), [`ground_normal.png`](../../../assets/textures/ground_normal.png) | tekstura koloru i mapa normalnych podłoża, 512 x 512, ze skryptu [`make_textures.py`](../../../tools/blender/make_textures.py) |
| [`tools/blender/make_heightmap.py`](../../../tools/blender/make_heightmap.py) | generator mapy wysokości (sekcja 5.14) |
| [`tests/TerrainTests.cpp`](../../../tests/TerrainTests.cpp) | 27 przypadków (sekcja 5.15) |

W [`CMakeLists.txt`](../../../CMakeLists.txt) `Terrain.*` należy do biblioteki `game_logic` (linkują ją testy), `TerrainRenderer.*` i `TerrainPanel.*` do programu `night_maze`.

Usunięte razem z płytkami: `assets/models/floor_tile.obj` i `.mtl`, `tools/blender/build_floor_tile.py`, `assets/textures/floor_stone.png` i `floor_stone_normal.png`, pole `MazeWorld::floorMatrices`, pole `MazeRenderer::m_floorTile` ([`../../decisions/floor-tiles-retired.md`](../../decisions/floor-tiles-retired.md)).

### 5.2 Stałe i ustawienia

```cpp
constexpr int TERRAIN_STEPS_PER_CELL = 4;
constexpr float TERRAIN_SPACING = CELL_SIZE / static_cast<float>(TERRAIN_STEPS_PER_CELL);
constexpr int TERRAIN_MARGIN_CELLS = 7;
constexpr float TERRAIN_MARGIN = static_cast<float>(TERRAIN_MARGIN_CELLS) * CELL_SIZE;
constexpr float HEIGHTMAP_SPAN = 48.0F;
constexpr float MAZE_RELIEF = 0.6F;
constexpr float HILL_RELIEF = 4.5F;
constexpr float DEFAULT_HEIGHT_SCALE = 1.0F;
constexpr float MAX_HEIGHT_SCALE = 2.5F;
constexpr float GROUND_TEXTURE_SPAN = 4.0F;
```

Wszystkie są omówione w sekcji 2. `TERRAIN_SPACING` i `TERRAIN_MARGIN` są liczone z pozostałych, więc zmiana `TERRAIN_STEPS_PER_CELL` albo `CELL_SIZE` nie rozjedzie siatki z labiryntem.

```cpp
struct TerrainSettings {
    float heightScale = DEFAULT_HEIGHT_SCALE;
    bool wireframe = false;
    bool rebuild = false;
};
```

Struktura tego samego rodzaju co `MazeSettings`: panel edytuje pola, gra czyta. `wireframe` działa od razu, bo to tylko argument `draw`. `heightScale` wymaga zbudowania terenu od nowa, więc panel ustawia flagę `rebuild`, a gra przebudowuje na początku następnej klatki i flagę zeruje. To ten sam wzorzec co `MazeSettings::regenerate`: panel jest rysowany w środku klatki i nie powinien podmieniać świata, z którego ta klatka właśnie korzysta.

### 5.3 `Heightmap` i `sample`

```cpp
struct Heightmap {
    int width = 1;
    int height = 1;
    std::vector<float> values{0.0F};
    float sample(float u, float v) const;
};
```

Nowa `Heightmap` to jedna wartość 0: płaski grunt. Dzięki temu kod i testy, które terenu nie potrzebują, dostają płaski świat bez żadnego pliku.

```cpp
float Heightmap::sample(float u, float v) const {
    const float column = fraction(u) * static_cast<float>(width);
    const float row = fraction(v) * static_cast<float>(height);
```

`fraction(value)` to `value - std::floor(value)`: część po przecinku, także dla liczby ujemnej (`-0,25` daje `0,75`). To ta linia robi powtarzanie obrazu. `column` i `row` są w pikselach, od 0 do `width` i do `height`.

```cpp
    const int column0 = std::min(static_cast<int>(column), width - 1);
    const int row0 = std::min(static_cast<int>(row), height - 1);
    const int column1 = (column0 + 1) % width;
    const int row1 = (row0 + 1) % height;
```

`column0`, `row0` to wartość na zachód i północ od miejsca. `std::min` jest zabezpieczeniem przed zaokrągleniem: `fraction` zwraca liczbę poniżej 1, ale po pomnożeniu przez `width` wynik w arytmetyce `float` może wypaść dokładnie na `width`. `column1` i `row1` to sąsiedzi na wschód i południe, z resztą z dzielenia: za ostatnią wartością jest pierwsza.

```cpp
    const float alongX = column - static_cast<float>(column0);
    const float alongZ = row - static_cast<float>(row0);
    ...
    const float north = glm::mix(valueAt(column0, row0), valueAt(column1, row0), alongX);
    const float south = glm::mix(valueAt(column0, row1), valueAt(column1, row1), alongX);
    return glm::mix(north, south, alongZ);
}
```

`glm::mix(a, b, t)` to `a + (b - a) * t`. Trzy mieszania: dwa wzdłuż wiersza, jedno między wierszami. Lambda `valueAt` zamienia kolumnę i wiersz na indeks `row * width + column`.

### 5.4 Od pliku do `Heightmap`

`loadHeightmap` w `NightMazeApp.cpp`:

```cpp
Heightmap loadHeightmap() {
    const std::filesystem::path path = core::assetPath(HEIGHTMAP_FILE);
    assets::Image image;
    std::string error;
    if (!assets::loadImage(path, image, error, assets::RowOrder::TopFirst)) {
        return {};
    }
    core::logInfo("Loaded heightmap: " + core::pathText(path));
    return heightmapFromImage(image);
}
```

- `HEIGHTMAP_FILE` to `"textures/heightmap.png"`.
- `RowOrder::TopFirst`: bez odwracania wierszy (sekcja 2.1).
- Gdy pliku nie da się wczytać, `loadImage` wypisuje błąd, a funkcja zwraca pustą `Heightmap`, czyli płaski grunt. Gra działa dalej, tylko bez nierówności.
- Mapa jest wczytywana **poza `assets::AssetCache`**. Pamięć podręczna trzyma tekstury na karcie graficznej, a mapa wysokości nigdy na kartę nie trafia: to dane dla procesora. Skutek uboczny: panel Assets jej nie pokazuje.

`heightmapFromImage` w `Terrain.cpp` bierze pierwszy bajt każdego piksela (`image.pixels[pixel * channels]`) i dzieli przez `MAX_CHANNEL_VALUE = 255`. Zanim to zrobi, sprawdza, czy obraz ma piksele i czy tablica bajtów jest tak długa, jak obiecują wymiary. Jeśli nie, zwraca płaską mapę zamiast czytać poza końcem tablicy.

Pole `m_heightmap` jest wypełniane raz, w liście inicjalizacyjnej `NightMazeApp`, i używane przy każdym nowym labiryncie i każdej zmianie skali. Plik jest czytany raz na uruchomienie.

### 5.5 Konstruktor `Terrain`

```cpp
Terrain::Terrain(int mazeWidth, int mazeHeight, const Heightmap& heightmap, float heightScale)
    : m_columns(0), m_rows(0), m_spacing(TERRAIN_SPACING),
      m_minX(-TERRAIN_MARGIN), m_minZ(-TERRAIN_MARGIN) {
    if (mazeWidth < 1 || mazeHeight < 1) {
        throw std::invalid_argument("Terrain: the maze must be at least 1 by 1 cells");
    }
    m_columns = (mazeWidth + 2 * TERRAIN_MARGIN_CELLS) * TERRAIN_STEPS_PER_CELL + 1;
    m_rows = (mazeHeight + 2 * TERRAIN_MARGIN_CELLS) * TERRAIN_STEPS_PER_CELL + 1;
```

Wzór na liczbę punktów z sekcji 2.2. Labirynt zaczyna się w początku układu świata, więc siatka zaczyna się o margines wcześniej, w `-14`.

```cpp
    for (int row = 0; row < m_rows; ++row) {
        for (int column = 0; column < m_columns; ++column) {
            const float x = m_minX + static_cast<float>(column) * m_spacing;
            const float z = m_minZ + static_cast<float>(row) * m_spacing;
            const float sample = heightmap.sample(x / HEIGHTMAP_SPAN, z / HEIGHTMAP_SPAN);
            const float relief = terrainRelief(distanceOutsideMaze(x, z, mazeWidth, mazeHeight));
            m_heights.push_back(heightScale * relief * sample);
        }
    }
```

Wzór z sekcji 2.3, linia w linię. Wysokości lądują w jednym wektorze, wiersz po wierszu: punkt `(column, row)` ma indeks `row * m_columns + column`.

```cpp
    const auto [lowest, highest] = std::ranges::minmax_element(m_heights);
    m_minHeight = *lowest;
    m_maxHeight = *highest;
}
```

`std::ranges::minmax_element` (wersja algorytmu, która bierze cały kontener zamiast pary `begin()` i `end()`) zwraca dwa iteratory, a `auto [a, b]` (structured binding) rozpakowuje ją na dwie nazwy. Najniższą i najwyższą wysokość pokazuje panel. Od czwartej części M7 czyta je też `game::shadowCasterBounds`, razem z `minX()`, `maxX()`, `minZ()` i `maxZ()`: z tych sześciu liczb powstaje pudełko, do którego księżyc dopasowuje mapę cieni (sekcja 5.13).

Konstruktor domyślny `Terrain()` robi płaski teren z jednego kwadratu 1 x 1 m na `y = 0`. Jego rozmiar nie ma znaczenia: punkt poza siatką dostaje wysokość brzegu, a cały brzeg jest na zerze.

Funkcje pomocnicze:

```cpp
float distanceOutsideRange(float coordinate, float size) {
    return std::max({-coordinate, coordinate - size, 0.0F});
}
```

Jak daleko współrzędna leży poza przedziałem od 0 do `size`: `-coordinate` jest dodatnie na lewo od zera, `coordinate - size` na prawo od końca, a w środku obie są ujemne i wygrywa 0. `distanceOutsideMaze` liczy to dla X i dla Z i zwraca długość wektora z tych dwóch liczb (`glm::length`). Obok labiryntu jedna z nich jest zerem i wynik to ta druga, a za narożnikiem obie są dodatnie i wychodzi odległość do narożnika z twierdzenia Pitagorasa.

```cpp
float terrainRelief(float distanceOutside) {
    const float t = std::clamp(distanceOutside / TERRAIN_MARGIN, 0.0F, 1.0F);
    return MAZE_RELIEF + (HILL_RELIEF - MAZE_RELIEF) * smoothStep(t);
}
```

### 5.6 `heightAt`

```cpp
float Terrain::heightAt(float x, float z) const {
    const float gridX =
        std::clamp((x - m_minX) / m_spacing, 0.0F, static_cast<float>(m_columns - 1));
    const float gridZ = std::clamp((z - m_minZ) / m_spacing, 0.0F, static_cast<float>(m_rows - 1));
```

Pozycja w krokach siatki, przycięta do siatki: punkt na zewnątrz jest przesuwany na brzeg.

```cpp
    const int column = std::min(static_cast<int>(gridX), m_columns - 2);
    const int row = std::min(static_cast<int>(gridZ), m_rows - 2);
```

Narożnik `NW` kwadratu. `static_cast<int>` obcina część ułamkową. `std::min` z `m_columns - 2` obsługuje punkt dokładnie na ostatniej kolumnie: kwadratu zaczynającego się w ostatniej kolumnie nie ma, więc taki punkt należy do ostatniego kwadratu i ma w nim `alongX = 1`.

```cpp
    const float alongX = gridX - static_cast<float>(column);
    const float alongZ = gridZ - static_cast<float>(row);

    const float northWest = clampedHeight(column, row);
    const float northEast = clampedHeight(column + 1, row);
    const float southWest = clampedHeight(column, row + 1);
    const float southEast = clampedHeight(column + 1, row + 1);

    if (alongX >= alongZ) {
        return northWest + alongX * (northEast - northWest) + alongZ * (southEast - northEast);
    }
    return northWest + alongZ * (southWest - northWest) + alongX * (southEast - southWest);
}
```

Dwa wzory z sekcji 2.7. Punkt dokładnie na przekątnej (`alongX == alongZ`) trafia do pierwszej gałęzi. Nie ma to znaczenia, bo na przekątnej oba wzory dają tę samą liczbę.

`clampedHeight(column, row)` przycina kolumnę i wiersz do siatki i czyta wysokość. `heightIndex` robi to samo bez przycinania i rzuca `std::out_of_range` dla punktu spoza siatki: używają go `gridPoint` i `gridNormal`, które są wołane tylko z poprawnymi indeksami.

### 5.7 `lowestHeightUnder`

```cpp
float Terrain::lowestHeightUnder(float minX, float minZ, float maxX, float maxZ) const {
    const int firstColumn = static_cast<int>(std::floor((minX - m_minX) / m_spacing));
    const int lastColumn = static_cast<int>(std::ceil((maxX - m_minX) / m_spacing));
    const int firstRow = static_cast<int>(std::floor((minZ - m_minZ) / m_spacing));
    const int lastRow = static_cast<int>(std::ceil((maxZ - m_minZ) / m_spacing));

    float lowest = clampedHeight(firstColumn, firstRow);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            lowest = std::min(lowest, clampedHeight(column, row));
        }
    }
    return lowest;
}
```

`std::floor` dla początku i `std::ceil` dla końca: linie siatki, które prostokąt mają w środku, a nie te, które są w nim. Tu `std::floor` zamiast rzutowania na `int` jest konieczne: rzutowanie obcina w stronę zera, więc dla ujemnej liczby zaokrągliłoby w górę. Prostokąt wystający poza siatkę obsługuje `clampedHeight`. Dlaczego najniższy narożnik wystarcza: sekcja 2.10.

### 5.8 `gridNormal`

```cpp
glm::vec3 Terrain::gridNormal(int column, int row) const {
    heightIndex(column, row);

    const int west = std::max(column - 1, 0);
    const int east = std::min(column + 1, m_columns - 1);
    const int north = std::max(row - 1, 0);
    const int south = std::min(row + 1, m_rows - 1);

    const float slopeX = (clampedHeight(east, row) - clampedHeight(west, row)) /
                         (static_cast<float>(east - west) * m_spacing);
    const float slopeZ = (clampedHeight(column, south) - clampedHeight(column, north)) /
                         (static_cast<float>(south - north) * m_spacing);

    return glm::normalize(glm::vec3{-slopeX, 1.0F, -slopeZ});
}
```

Pierwsza linia woła `heightIndex` tylko po to, żeby sprawdzić punkt (wynik jest porzucany): sąsiedzi są przycinani, ale sam punkt musi istnieć. `east - west` to 2 wewnątrz siatki i 1 na brzegu, więc dzielenie jest zawsze przez prawdziwą odległość. Siatka ma co najmniej 2 punkty w każdą stronę, więc mianownik nigdy nie jest zerem.

### 5.9 `buildTerrainMesh`

```cpp
struct TerrainMeshData {
    std::vector<gfx::Vertex> vertices;
    std::vector<std::uint32_t> indices;
};
```

Zwykłe dane, bez OpenGL: testy sprawdzają siatkę bez karty graficznej.

```cpp
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            gfx::Vertex vertex;
            vertex.position = terrain.gridPoint(column, row);
            vertex.normal = terrain.gridNormal(column, row);
            vertex.uv = {vertex.position.x / GROUND_TEXTURE_SPAN,
                         -vertex.position.z / GROUND_TEXTURE_SPAN};
            mesh.vertices.push_back(vertex);
        }
    }
```

Jeden wierzchołek na punkt siatki, wiersz po wierszu, tak samo jak wysokości. Dzięki temu indeks wierzchołka punktu `(column, row)` to znów `row * columns + column`:

```cpp
    const auto vertexIndex = [columns](int column, int row) {
        return static_cast<std::uint32_t>(row * columns + column);
    };
```

Pętla po kwadratach (o jeden mniej niż punktów w każdą stronę):

```cpp
    for (int row = 0; row < rows - 1; ++row) {
        for (int column = 0; column < columns - 1; ++column) {
            const std::uint32_t northWest = vertexIndex(column, row);
            const std::uint32_t northEast = vertexIndex(column + 1, row);
            const std::uint32_t southWest = vertexIndex(column, row + 1);
            const std::uint32_t southEast = vertexIndex(column + 1, row + 1);

            mesh.indices.insert(mesh.indices.end(), {northWest, southEast, northEast});
            mesh.indices.insert(mesh.indices.end(), {northWest, southWest, southEast});
        }
    }
```

Sześć indeksów na kwadrat, cztery wierzchołki: dwa narożniki przekątnej są wspólne dla obu trójkątów. To jest dokładnie ten zysk z indeksów, który opisuje [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md), tylko w dużej skali: 9409 wierzchołków zamiast `18432 * 3 = 55296`.

Przykład: kwadrat, w którym stoi gracz na starcie, ma narożnik `NW` w kolumnie 30 i wierszu 30 (punkt `x = 1`, `z = 1`). Przy 97 kolumnach: `NW = 30 * 97 + 30 = 2940`, `NE = 2941`, `SW = 31 * 97 + 30 = 3037`, `SE = 3038`. Indeksy tego kwadratu to `2940, 3038, 2941` i `2940, 3037, 3038`.

```cpp
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
```

Styczne na końcu, gdy pozycje, UV i indeksy są gotowe ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md)).

### 5.10 `placeOnTerrain`

```cpp
void placeOnTerrain(MazeWorld& world, const Heightmap& heightmap, float heightScale) {
    world.terrain = Terrain(world.maze.width(), world.maze.height(), heightmap, heightScale);
    const Terrain& terrain = world.terrain;

    world.wallMatrices.clear();
    for (WallSegment& segment : world.walls) {
        lowerToGround(terrain, segment);
        world.wallMatrices.push_back(wallModelMatrix(segment));
    }
    world.pillarMatrices.clear();
    for (glm::vec3& position : world.pillars) {
        position.y = lowestGroundUnder(terrain, pillarBox(position));
        world.pillarMatrices.push_back(placedAt(position));
    }
    world.colliders = colliderBoxes(world.walls, world.pillars);
```

Nowy teren, a potem ściany i słupki: zmienia się tylko `y`. Macierze modelu i pudełka kolizji są budowane od nowa z opuszczonych pozycji, więc model i jego pudełko zawsze są razem. `clear()` przed pętlą jest potrzebne, bo funkcję można wołać drugi raz na tym samym świecie.

```cpp
float lowestGroundUnder(const Terrain& terrain, const scene::Aabb& box) {
    return terrain.lowestHeightUnder(box.min.x - FOOTPRINT_MARGIN, box.min.z - FOOTPRINT_MARGIN,
                                     box.max.x + FOOTPRINT_MARGIN, box.max.z + FOOTPRINT_MARGIN);
}

void lowerToGround(const Terrain& terrain, WallSegment& segment) {
    segment.position.y = lowestGroundUnder(terrain, wallBox(segment));
}
```

Obrys pudełka nie zależy od jego wysokości, więc o pudełko można zapytać, zanim wysokość jest znana: `wallBox(segment)` z dowolnym `y` ma te same `min.x`, `max.x`, `min.z`, `max.z`.

```cpp
    world.startPosition = cellCenter(START_CELL.x, START_CELL.z);
    world.startPosition.y = groundHeightAt(world, START_CELL);
    world.exitPosition = cellCenter(world.exitCell.x, world.exitCell.z);
    world.exitPosition.y = groundHeightAt(world, world.exitCell);
    world.exitZone = exitZone(world.exitCell, world.exitPosition.y);

    if (world.hasGate) {
        lowerToGround(terrain, world.gate);
        world.gateBox = wallBox(world.gate);
    }
}
```

`groundHeightAt(world, cell)` to `heightAt` w środku komórki. `crystalRestPosition` i `exitZone` nie znają terenu: dostają wysokość gruntu jako liczbę, więc `Crystals.cpp` i `Exit.cpp` nie muszą dołączać `Terrain.hpp`.

`buildMazeWorld` ma dwie wersje. Trzyargumentowa (szerokość, wysokość, ziarno) woła pięcioargumentową z pustą `Heightmap`, czyli daje labirynt na płaskim gruncie: używają jej starsze testy. Pięcioargumentowa najpierw liczy plan (ściany, słupki, wyjście, brama, kryształy), a na końcu woła `placeOnTerrain`.

**Co kopiuje wysokości ze świata.** `placeOnTerrain` poprawia sam `MazeWorld`. Trzy rzeczy trzymają kopie i musi je poprawić wywołujący: kryształy rundy (`restCrystalsOnGround`), lista przeszkód (`roundObstacles`) i gracz. Robi to `rebuildTerrain` (sekcja 5.13).

### 5.11 Gracz

```cpp
    position.y = terrain.heightAt(position.x, position.z);

    const float speed = input.sprint ? sprintSpeed : walkSpeed;
    const glm::vec3 wanted = direction * (speed * stepSeconds);

    position += scene::moveAndSlide(box(), wanted, obstacles);

    position.y = terrain.heightAt(position.x, position.z);
```

Pierwszy odczyt stawia stopy na ziemi **przed** testem kolizji. Ma to znaczenie w kroku po wyłączeniu noclip w powietrzu: pudełko gracza musi być między ścianami, zanim zostanie z nimi porównane (sekcja 2.11). Drugi odczyt jest po ruchu: miejsce, w którym krok się skończył, ma własną wysokość. `direction` w trybie chodzenia nie ma składowej pionowej. W trybie noclip funkcja wraca wcześniej i terenu nie czyta. Reszta: [`../game/player.md`](../game/player.md).

### 5.12 `TerrainRenderer`

```cpp
TerrainRenderer::TerrainRenderer(assets::AssetCache& assets)
    : m_mesh(std::span<const gfx::Vertex>{}, std::span<const std::uint32_t>{}),
      // The picture of the earth is a colour (sRGB), its normal map is data (linear).
      m_texture(
          textureOr(assets, GROUND_TEXTURE_FILE, gfx::ColorSpace::Srgb, assets.whiteTexture())),
      m_normalMap(textureOr(assets, GROUND_NORMAL_MAP_FILE, gfx::ColorSpace::Linear,
                            assets.flatNormalTexture())) {}
```

(Komentarz nad `m_mesh` jest tu pominięty.)

Siatka zaczyna pusta (zero wierzchołków, zero indeksów): rysowanie jej nic nie rysuje, dopóki nie zostanie wywołane `upload`. Dwie tekstury pochodzą z pamięci podręcznej assetów. `textureOr` zwraca teksturę z pliku albo, gdy pliku nie udało się wczytać, zastępczą: białą dla koloru i płaską mapę normalnych. Wskaźniki nigdy nie są puste, więc `draw` nie musi niczego sprawdzać.

**Przestrzeń kolorów (od M7).** `textureOr` ma dodatkowy parametr `gfx::ColorSpace colorSpace` i podaje go dalej do `assets.texture(core::assetPath(file), colorSpace)`. Wołający musi powiedzieć, czym są bajty pliku, bo sama tekstura tego nie wie:

| Plik | Argument | Format na karcie | Dlaczego |
|---|---|---|---|
| `ground.png` | `gfx::ColorSpace::Srgb` | `GL_SRGB8` | to kolor malowany dla ekranu. Karta dekoduje go do wartości liniowej przy odczycie, a rachunek światła dostaje to, czego potrzebuje |
| `ground_normal.png` | `gfx::ColorSpace::Linear` | `GL_RGB8` | to kierunki, a nie kolory. Zdekodowane jak sRGB przestałyby być kierunkami: bajt 128, który znaczy 0, wyszedłby jako 0,216 zamiast 0,502 i normalne by się pochyliły |

Obie tekstury zastępcze pasują do tego podziału: biała jest sRGB (biel to 1 w obu przestrzeniach), płaska mapa normalnych jest liniowa. Całość: [`../gfx/color-space.md`](../gfx/color-space.md) i [`../assets/asset-cache.md`](../assets/asset-cache.md).

```cpp
void TerrainRenderer::upload(const TerrainMeshData& mesh) {
    m_mesh = gfx::Mesh(mesh.vertices, mesh.indices);
}
```

```cpp
void TerrainRenderer::draw(const gfx::Shader& shader, bool wireframe) const {
    setModelSamplers(shader);
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    if (wireframe) {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    }

    drawMesh(shader, m_mesh, *m_texture, *m_normalMap, NO_TINT, IDENTITY);

    if (wireframe) {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    }
}
```

Kroki z sekcji 3.2. `NO_TINT` to `glm::vec3{1.0F}`, `IDENTITY` to `glm::mat4{1.0F}`.

`drawMesh` w `ModelDraw.cpp` jest nową, trzecią funkcją wspólnego kodu rysującego. `drawModel` rysuje model z pliku (części, każda ze swoimi teksturami, raz na każdą macierz modelu). `drawMesh` rysuje jedną siatkę z podanymi teksturami, raz:

```cpp
void drawMesh(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
              const gfx::Texture2D& normalMap, const glm::vec3& tint,
              const glm::mat4& modelMatrix) {
    normalMap.bind(NORMAL_MAP_UNIT);
    texture.bind(TEXTURE_UNIT);
    shader.setVec3(TINT_UNIFORM, tint);
    shader.setMat4(MODEL_UNIFORM, modelMatrix);
    shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
    mesh.draw();
}
```

Te same kroki i te same numery jednostek co w `drawModel`, więc teren, ściany i kryształy zgadzają się co do tego, gdzie jest tekstura koloru, a gdzie mapa normalnych.

### 5.13 `NightMazeApp`: start, nowy labirynt, zmiana skali

Kolejność pól ma znaczenie, bo pola są tworzone z góry na dół:

```cpp
    Heightmap m_heightmap;
    TerrainSettings m_terrainSettings;
    MazeWorld m_mazeWorld;
```

`m_heightmap` i `m_terrainSettings` stoją przed `m_mazeWorld`, bo pierwszy labirynt jest budowany z nich w liście inicjalizacyjnej:

```cpp
      m_heightmap(loadHeightmap()),
      m_mazeWorld(buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale)) {
```

Trzy drogi, którymi teren trafia na kartę:

| Zdarzenie | Funkcja | Co robi |
|---|---|---|
| start programu | konstruktor | `buildMazeWorld` z mapą, `uploadGround`, `beginRound` |
| nowy labirynt (panel Maze) | `regenerateMaze` | przycina skalę, `buildMazeWorld` z mapą, `uploadGround`, `beginRound` |
| nowa skala (panel Terrain) | `rebuildTerrain` | patrz niżej |

```cpp
void NightMazeApp::rebuildTerrain() {
    m_terrainSettings.heightScale =
        std::clamp(m_terrainSettings.heightScale, MIN_HEIGHT_SCALE, MAX_HEIGHT_SCALE);

    placeOnTerrain(m_mazeWorld, m_heightmap, m_terrainSettings.heightScale);

    restCrystalsOnGround(m_round, m_mazeWorld);
    m_obstacles = roundObstacles(m_mazeWorld, m_round);

    if (!m_player.noclip) {
        m_player.position.y =
            m_mazeWorld.terrain.heightAt(m_player.position.x, m_player.position.z);
        m_previousPlayerPosition.y = m_player.position.y;
        m_camera.position = m_player.eyePosition();
    }

    uploadGround();
}
```

Runda trwa dalej: nic nie jest zbierane ani zerowane. Kryształy zachowują stan (zebrany albo nie), zmienia się tylko miejsce, w którym odpoczywają. Idący gracz jest stawiany na nowym gruncie w **obu** pozycjach naraz (bieżącej i poprzedniej), żeby następna klatka nie była narysowana z punktu między starą a nową wysokością. Lecący gracz zostaje tam, gdzie jest.

```cpp
void NightMazeApp::uploadGround() {
    m_terrainRenderer.upload(buildTerrainMesh(m_mazeWorld.terrain));
    plantGrass();
}
```

Trawa stoi na terenie, więc nowy grunt oznacza nowe miejsca kępek.

Żądanie z panelu jest obsługiwane na początku `onRender`, zaraz po żądaniu nowego labiryntu:

```cpp
    if (m_terrainSettings.rebuild) {
        m_terrainSettings.rebuild = false;
        rebuildTerrain();
    }
```

Jeśli w tej samej klatce powstał nowy labirynt, jest on już zbudowany z nową skalą, a `rebuildTerrain` robi to drugi raz. Komentarz mówi wprost: kosztuje to raz trochę czasu i niczego nie zmienia.

**Teren a mapa cieni (czwarta część M7, cienie księżyca, 2026-10-05).** Żadna z trzech dróg opisanych wyżej (start, nowy labirynt, zmiana skali) nie dostała kodu dla cieni. Nie musiała: `NightMazeApp::drawMoonShadowMap` liczy pudełko światła od nowa w **każdej klatce**, z terenu, który akurat jest w `m_mazeWorld`:

```cpp
    m_moonLightSpace = scene::directionalLightSpace(shadowCasterBounds(m_mazeWorld.terrain),
                                                    moonDirection(m_lighting));
```

Komentarz nad tą linią nazywa powód: to kilkadziesiąt mnożeń i nic, o czym dałoby się zapomnieć po zbudowaniu nowego labiryntu, zmianie skali wysokości albo przesunięciu księżyca w panelu Lights. Pudełko, z którego to powstaje, jest w `src/game/Shadows.cpp`:

```cpp
scene::Aabb shadowCasterBounds(const Terrain& terrain) {
    // The pillars are the tallest things that stand on the ground. One of them on the
    // highest point of the land is higher than anything really is: the maze lies in
    // the low middle. That costs a little depth range and keeps the rule simple.
    return {
        .min = {terrain.minX(), terrain.minHeight(), terrain.minZ()},
        .max = {terrain.maxX(), terrain.maxHeight() + PILLAR_HEIGHT, terrain.maxZ()},
    };
}
```

| Element | Znaczenie |
|---|---|
| `terrain.minX()`, `maxX()`, `minZ()`, `maxZ()` | cały teren w planie, razem z marginesem wzgórz wokół labiryntu. Dla labiryntu startowego to 48 x 48 m (97 punktów co 0,5 m) |
| `terrain.minHeight()` | najniższy grunt: nic, co rzuca cień, nie leży niżej |
| `terrain.maxHeight() + PILLAR_HEIGHT` | najwyższy grunt plus wysokość słupka (`PILLAR_HEIGHT = 3.15F` w `MazeLayout.hpp`), najwyższej rzeczy stojącej na gruncie. To celowo za dużo: słupki stoją w niskim środku, a nie na szczycie wzgórza. Dla labiryntu startowego przy skali 1 góra pudełka wypada na `3,37 + 3,15 = 6,52 m` (policzone z liczb z początku dokumentu) |

Większa skala wysokości podnosi `maxHeight()`, więc pudełko rośnie w pionie, a z nim zakres głębi mapy. Bias cieni jest podawany w metrach i dzielony przez ten zakres dopiero przy wysyłaniu do shadera, więc suwak `Height scale` nie rozstraja cieni. Resztę (obrót pudełka w stronę światła, margines 0,5 m, teksele na metr) opisuje [`shadows.md`](shadows.md), sekcje 2.2 do 2.4, i [`../scene/lights.md`](../scene/lights.md).

### 5.14 Skrypt `make_heightmap.py`

Obraz jest wynikiem skryptu, jak wszystkie assety gry ([`../../guides/blender.md`](../../guides/blender.md)). Uruchomienie z katalogu repozytorium:

```
blender --background --factory-startup --python tools/blender/make_heightmap.py
```

Blender służy tu tylko do zapisania pliku PNG. Wysokości liczy NumPy.

| Stała | Wartość | Znaczenie |
|---|---|---|
| `SIZE` | 256 | bok obrazu w pikselach |
| `HEIGHTMAP_SEED` | 53 | ziarno: to samo daje ten sam obraz |
| `BASE_CELLS` | 4 | komórki siatki szumu w pierwszej oktawie: pagórki szerokie na ćwierć obrazu, 12 m przy 48 m |
| `OCTAVE_COUNT` | 3 | liczba oktaw |
| `OCTAVE_GAIN` | 0,45 | każda następna oktawa ma tyle wysokości poprzedniej |

**Szum wartości** (value noise): losowe liczby w węzłach rzadkiej siatki, gładko zmieszane między węzłami. Jedna taka warstwa to kilka szerokich pagórków. **Oktawa** to ta sama rzecz z dwa razy gęstszą siatką i mniejszą wysokością: drobniejsze nierówności na pagórkach. Trzy oktawy mają 4, 8 i 16 komórek na obraz i wagi 1, 0,45 i 0,2025.

`value_noise(rng, cells)` losuje tablicę `cells x cells`, dla każdego piksela znajduje jego komórkę i miesza cztery narożniki, z ułamkiem przepuszczonym przez `smooth_step` (ta sama krzywa co w sekcji 2.4, tu po to, żeby pagórki nie miały kantów na liniach siatki). **Węzeł za ostatnim jest znów pierwszym** (`(cell + 1) % cells`), więc obraz powtarza się bez szwu: lewa krawędź przechodzi w prawą, górna w dolną. To jest warunek kafelkowania z sekcji 2.5, spełniony konstrukcją, a nie poprawką.

`height_field` sumuje oktawy i **rozciąga wynik do pełnego zakresu**: najniższy piksel staje się zerem, najwyższy jedynką. Dzięki temu metry, przez które mnoży gra (`MAZE_RELIEF`, `HILL_RELIEF`), są naprawdę osiągane. Test sprawdza, że najciemniejszy piksel pliku to 0, a najjaśniejszy 1.

`save_png` zaokrągla wartości do 256 poziomów sama (żeby bajty w pliku nie zależały od zaokrąglania Blendera), odwraca wiersze (Blender trzyma obraz od dolnego wiersza, a plik ma mieć północ u góry) i zapisuje tę samą wartość do trzech kanałów.

Wywołanie `make_heightmap.build()` stoi na końcu [`make_all.py`](../../../tools/blender/make_all.py).

### 5.15 Testy

[`tests/TerrainTests.cpp`](../../../tests/TerrainTests.cpp): 27 przypadków. Plik nie potrzebuje okna. Ma trzy pomocnicze mapy: `roughHeightmap` (16 x 16 wartości skaczących w górę i w dół według wzoru, żeby każdy kwadrat siatki był wygięty), `constantHeightmap` i `eastwardSlope` (dwie kolumny, 0 i 1).

| Grupa | Przypadki | Co sprawdzają |
|---|---|---|
| stałe | `the terrain constants are the agreed sizes` | wartości stałych z sekcji 5.2 |
| mapa | `Heightmap::sample blends the four values around a place and repeats`, `heightmapFromImage reads the first channel of every pixel` | mieszanie, powtarzanie w obie strony, czytanie pierwszego kanału |
| relief | `the relief is gentle inside the maze and grows into hills outside` | 0,6 w labiryncie, 4,5 na krawędzi i dalej, wartości pośrednie rosną |
| siatka | `a default terrain is flat ground at y = 0 everywhere`, `the grid covers the maze and the margin around it, 4 steps per cell`, `a flat heightmap gives a flat terrain`, `the height scale multiplies every height, and 0 gives a flat world`, `the height of a grid point follows the formula of the terrain`, `the same heightmap, size and scale give the same terrain` | rozmiar siatki, wzór na wysokość, skala, powtarzalność |
| `heightAt` | `heightAt returns the height of a grid point at that grid point`, `heightAt is a straight line along the edges and along the diagonal of a square`, `heightAt outside the grid is the height of the nearest border point`, `heightAt agrees with the triangle of the mesh at random points` | sekcja 2.7 |
| siatka trójkątów | `the mesh has one vertex per grid point and two triangles per square`, `every triangle of the mesh faces up, and its texture is not mirrored`, `the vertices of the mesh carry normals, texture coordinates and tangents`, `gridNormal leans away from rising ground` | liczby wierzchołków i indeksów, nawijanie, UV, styczne, normalne |
| stawianie | `lowestHeightUnder is never above the surface over its rectangle`, `a maze world without a heightmap stands on flat ground`, `on uneven ground the walls, pillars and the gate are sunk until no gap shows`, `placeOnTerrain can be called again, and a height scale of 0 brings the flat world back`, `the crystals of a round float above the ground of their cells` | sekcja 2.10 |
| plik gry | `the heightmap of the game loads and gives gentle ground inside the default maze` | 256 x 256, 3 kanały, pełny zakres, różnica w labiryncie od 0,3 do 0,5 m, wzgórza powyżej 2 m i nie wyżej niż `HILL_RELIEF` |
| gracz | `a walking player keeps the feet on the ground, uphill and downhill`, `the height of the feet changes a little in every step, never in a jump`, `the walls stop the player on uneven ground exactly as on flat ground` | sekcja 2.11 |

**Najważniejszy test: `heightAt agrees with the triangle of the mesh at random points`.** Funkcja pomocnicza `meshHeightAt` nie wie nic o siatce ani o przekątnej. Dostaje tylko wierzchołki i indeksy, tak jak karta graficzna, przegląda wszystkie trójkąty, znajduje ten, w którego rzucie na płaszczyznę XZ leży punkt (współrzędnymi barycentrycznymi), i liczy wysokość z jego trzech narożników. Test porównuje ten wynik z `heightAt` w losowych punktach z tolerancją 0,00001 m. Zamiana przekątnej w jednej z dwóch funkcji albo powrót do mieszania dwuliniowego sprawiłyby, że ten test nie przechodzi.

Czego testy **nie** sprawdzają:

- niczego, co wymaga karty: `TerrainRenderer`, `drawMesh`, `glPolygonMode`, wyglądu tekstury na terenie,
- tego, że gra wczytuje mapę z `RowOrder::TopFirst` (test wczytuje ją sam),
- `rebuildTerrain` w `NightMazeApp`: tego, że kryształy, przeszkody i gracz są poprawiane po zmianie skali w działającej grze. Sprawdzone są klocki (`placeOnTerrain`, `restCrystalsOnGround`), nie ich złożenie,
- płynności obrazu przy chodzeniu.

**Przypadek spoza tego pliku (czwarta część M7).** `tests/ShadowTests.cpp` ma przypadek `the caster bounds of the moon hold the land and everything that stands on it`. Buduje labirynt domyślny przeciążeniem `buildMazeWorld` bez mapy wysokości (teren płaski), woła `shadowCasterBounds(world.terrain)` i sprawdza sześć granic pudełka wprost z akcesorów `Terrain` (`minX`, `maxX`, `minZ`, `maxZ`, `minHeight`, `maxHeight() + PILLAR_HEIGHT`), a potem to, że każde pudełko kolizji labiryntu mieści się w nim w planie i nie wystaje górą. Teren z prawdziwej mapy wysokości nie jest w tym przypadku sprawdzany. `tests/TerrainTests.cpp` się nie zmienił.

### 5.16 Jak to zostało sprawdzone

Lista z początku dokumentu, tutaj z podziałem na źródło:

- **Testy** (uruchomione przeze mnie 2026-10-05 na plikach z `build/debug` i `build/release`, po drugiej części M6): 256 przypadków, 101232 asercje, wszystkie zaliczone w obu konfiguracjach. Po pierwszej części M7 zgłoszone 269 i 102103, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751, tych nie uruchamiałem.
- **Po czwartej części M7** (cienie księżyca, zgłoszone dla Windowsa, 2026-10-05): bramka `make check` przechodzi, a build Debug nie zalogował błędów OpenGL przy mapie cieni 2048 i 1024. Z wyłączonymi cieniami i intensywnością księżyca ustawioną z powrotem na 0,12 obraz jest poza paskiem HUD identyczny co do piksela z obrazem sprzed tej części, więc drugi przebieg terenu niczego w scenie nie psuje. Nikt nie zgłosił osobno: terenu z `Wireframe` przy włączonych cieniach ani zmiany `Height scale` przy włączonych cieniach (to, że pudełko światła idzie za terenem, wynika z kodu).
- **Po pierwszej części M7** (zgłoszone dla Windowsa, 2026-10-05): w trybie `Unlit` z `Tone mapping: None` i ekspozycją 1 podłoże i ściany różnią się od poprzedniego commita najwyżej o 22 poziomy na 255 (średnio 1,1), tylko na spoinach cegieł. Obraz nie jest identyczny, bo filtrowanie tekstury działa teraz na wartościach liniowych: tak robią dzisiejsze karty, a OpenGL 4.1 tę kolejność (najpierw dekodowanie, potem filtr) zaleca, ale jej nie wymaga ([`../gfx/color-space.md`](../gfx/color-space.md)).
- **Liczby terenu** (przeliczone niezależnie od kodu C++, skryptem czytającym plik PNG): 97 x 97, 18432 trójkąty, od 0,085 do 0,461 m w labiryncie, 3,37 m na wzgórzach, wysokości w trzech komórkach, przykłady z sekcji 2.
- **Build, format, obraz, liczba klatek** (zgłoszone przez autora kodu): bez ostrzeżeń, clang-format czysty, zrzuty ekranu, około 2000 klatek na sekundę w Release przed i po.
- **Nie zapisano:** wersji kompilatora, karty i sterownika dla tego pomiaru, wyniku clang-tidy.
- **Nie sprawdzono ręcznie:** niczego interaktywnego. **macOS:** nic.

## 6. Panel ImGui

Panel **Terrain** jest dziewiątym panelem. Startuje zwinięty do paska tytułu, w drugim rzędzie pasków przy górnej krawędzi okna, pod panelem Camera ([`../debug-ui.md`](../debug-ui.md)).

```cpp
void drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain) {
    placePanelOnFirstUse(TERRAIN_PLACEMENT);
    if (ImGui::Begin("Terrain")) {
        if (ImGui::SliderFloat("Height scale", &settings.heightScale, MIN_HEIGHT_SCALE,
                               game::MAX_HEIGHT_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
            settings.rebuild = true;
        }
        ...
        ImGui::Checkbox("Wireframe", &settings.wireframe);
        ...
        ImGui::Separator();
        ImGui::Text("Grid: %d x %d points, %.2f m apart", terrain.columns(), terrain.rows(),
                    terrain.spacing());
        ImGui::Text("Triangles: %d", static_cast<int>(terrain.triangleCount()));
        ImGui::Text("Height: %.2f m to %.2f m", terrain.minHeight(), terrain.maxHeight());
    }
    ImGui::End();
}
```

| Kontrolka | Zakres | Co robi |
|---|---|---|
| suwak `Height scale` | od 0 do 2,5, startuje na 1 | mnoży każdą wysokość terenu. `SliderFloat` zwraca `true` w każdej klatce, w której wartość się zmieniła, więc teren idzie za suwakiem podczas przeciągania. Panel tylko ustawia `rebuild`: przebudowę robi gra na początku następnej klatki |
| pole `Wireframe` | wyłączone | rysuje krawędzie trójkątów terenu zamiast ich wnętrz. Reszta sceny zostaje wypełniona. Mapy cieni to pole nie dotyczy: tam teren jest zawsze wypełniony (sekcja 2.12) |
| `Grid: ...` | tylko odczyt | liczba punktów siatki i odstęp. Dla labiryntu startowego `97 x 97 points, 0.50 m apart` |
| `Triangles: ...` | tylko odczyt | 18432 dla labiryntu startowego |
| `Height: ... m to ... m` | tylko odczyt | najniższy i najwyższy punkt **całej** siatki, razem ze wzgórzami: `0.00 m to 3.37 m` przy ustawieniach startowych. To nie jest zakres wewnątrz labiryntu |

`ImGuiSliderFlags_AlwaysClamp` przycina także wartość wpisaną z klawiatury (Ctrl i kliknięcie w suwak).

### 6.1 Scenariusz pokazu na obronie

**Kroków nikt jeszcze nie wykonał ręcznie.** Lista do odhaczenia jest w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 16. Przed pokazem trzeba usunąć stary `imgui.ini`, żeby nowe panele stanęły na swoich miejscach.

1. **Teren zamiast płytek.** Staję na starcie i patrzę w dół korytarza: podłoże lekko faluje. Mówię: to jedna siatka, 97 na 97 punktów, a wysokości pochodzą z obrazu 256 x 256.
2. **Wireframe.** Rozwijam panel Terrain i zaznaczam `Wireframe`. Widać kwadraty po 0,5 m przecięte przekątną zawsze w tę samą stronę. Mówię: cztery kroki na komórkę, więc ściany stoją dokładnie na liniach siatki, a przekątna biegnie z północnego zachodu na południowy wschód. To jedno wywołanie `glPolygonMode`, przywracane zaraz po narysowaniu terenu, dlatego ściany są nadal wypełnione.
3. **Skala wysokości.** Przeciągam `Height scale` do 0: świat jest płaski, ściany stoją równo jak dawniej. Wracam do 1, potem do 2,5. Ściany, kryształy i gracz idą za terenem od razu. Mówię: każda wysokość to skala razy relief razy wartość z obrazu, a teren jest budowany od nowa na procesorze, bo za nim muszą pójść pudełka kolizji i gracz.
4. **Brak szpar.** Przy skali 2,5 podchodzę do ściany i patrzę na jej podstawę. Mówię: ściana stoi na najniższym narożniku siatki pod swoim obrysem, a płaski trójkąt nigdy nie jest niżej niż jego najniższy narożnik.
5. **Wzgórza.** Klawisz N (noclip), lecę w górę i patrzę na labirynt z zewnątrz. Mówię: ten sam obraz, ale relief rośnie od 0,6 m w labiryncie do 4,5 m na krawędzi, krzywą smoothstep, żeby na granicy nie było załamania.
6. **Wysokość gracza.** Wyłączam noclip nad labiryntem: stopy wracają na ziemię. Idę korytarzem i pokazuję w panelu Camera, że `y` stóp się zmienia. Mówię: wysokość jest czytana z tego samego trójkąta, który jest rysowany, a nie mieszana dwuliniowo.
7. **Tryby.** W panelu Assets przełączam `View mode` na `Normals as colour`: teren jest prawie cały zielony (normalne w górę) z lekkimi odcieniami na zboczach. Wracam do `Textured` i w panelu Renderer przełączam `Lighting` na `Gouraud`, a potem z powrotem: na ziemi światło jest w tym trybie liczone w wierzchołkach, czyli co 0,5 m, a nie w każdym pikselu.

## 7. Pułapki

1. **Dwie przekątne.** Zmiana kolejności indeksów w `buildTerrainMesh` bez zmiany `heightAt` (albo odwrotnie) nie daje błędu kompilacji ani błędu OpenGL. Gracz chodzi wtedy kilka milimetrów nad albo pod narysowaną powierzchnią. Wykrywa to tylko test.
2. **Odwrócone wiersze mapy.** Wczytanie `heightmap.png` z domyślnym `RowOrder::BottomFirst` daje teren odbity w osi północ-południe. Wszystko działa, tylko nie zgadza się z obrazem.
3. **Obraz, który się nie kafelkuje.** Mapa narysowana ręcznie albo wzięta z internetu prawie na pewno ma różne krawędzie. W labiryncie startowym szew wypadłby na liniach `x = 0` i `z = 0`, czyli pod zewnętrznymi ścianami, więc mógłby ujść uwadze. W większym labiryncie byłby uskok co 48 m w środku korytarzy.
4. **`glPolygonMode` zostawione na `GL_LINE`.** Wszystko, co scena rysuje po terenie (ściany, kryształy, trawa, niebo), zamieniłoby się w linie, a w następnej klatce także sam teren przy wyłączonym polu. Panele ImGui by ocalały: ich backend sam ustawia `GL_FILL` przed rysowaniem, a po nim przywraca tryb, który zastał.
5. **`GL_FRONT` zamiast `GL_FRONT_AND_BACK`.** W profilu Core to `GL_INVALID_ENUM`. W buildzie Debug zgłosi to `GL_CHECK`.
6. **Skala powyżej `MAX_HEIGHT_SCALE`.** Podniesienie stałej bez przeliczenia sekcji 2.11 może pozwolić graczowi przejść przez ścianę w miejscu, gdzie różnica wysokości przekracza 1,8 m. Błąd pojawiłby się tylko w niektórych miejscach niektórych labiryntów.
7. **`MAZE_RELIEF` a `MAX_HEIGHT_SCALE`.** Te dwie stałe są sprzężone iloczynem: liczy się `MAZE_RELIEF * MAX_HEIGHT_SCALE`, który ma być mniejszy od `Player::BODY_HEIGHT`. Kod tego nie sprawdza (nie ma `static_assert`), pilnuje tylko komentarz.
8. **Zapomniana kopia wysokości.** `placeOnTerrain` poprawia `MazeWorld`. Kryształy rundy, lista przeszkód i gracz mają kopie. Kto zawoła `placeOnTerrain` bez trzech linii z `rebuildTerrain`, dostanie kryształy wiszące na starej wysokości i kolizje ze ścianami, których tam już nie ma.
9. **Rzutowanie zamiast `std::floor`.** `static_cast<int>(-0.3F)` to 0, a nie -1. W `lowestHeightUnder` współrzędne względem `m_minX` są zwykle dodatnie, ale prostokąt wystający za zachodnią krawędź terenu dałby liczbę ujemną i o jedną kolumnę za mało.
10. **Suwak a czas przebudowy.** Przeciąganie `Height scale` buduje teren, siatkę, styczne i trawę w każdej klatce, w której wartość się zmienia. Dla labiryntu startowego to 9409 wierzchołków. Dla 40 x 40 to 47089 i przeciąganie może szarpać. Nie zmierzono.
11. **Zakopane modele.** Tekstura ściany zaczyna się u jej podstawy, więc w miejscach, gdzie ściana jest zagłębiona, dolny pas cokołu znika pod ziemią. To cena braku szpar, nie błąd.
12. **Stary `imgui.ini`.** Położenie z `PanelLayout` działa tylko przy pierwszym użyciu panelu, czyli gdy panelu nie ma w pliku. Nowe panele Terrain i Grass staną więc na swoich miejscach, ale stare zostaną tam, gdzie zapisał je plik, a HUD jest od tej części niżej. Przed pokazem najprościej usunąć plik.
13. **Teren nie ma kolizji.** Wysokość jest odczytywana. Na wzgórzach (do 4,5 m na jedynkę skali) gracz wejdzie na dowolnie strome zbocze z pełną prędkością poziomą. Do wzgórz da się dojść tylko w trybie noclip i po jego wyłączeniu poza labiryntem, bo labirynt jest zamknięty ścianami.
14. **Tekstura podłoża jest sRGB.** Do M6 `ground.png` była dobrana na oko dla obrazu bez korekcji gamma ([`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md), dziś zastąpiona przez [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md)). Od M7 jest wczytywana jako sRGB, a jej mapa normalnych jako dane liniowe. Zamiana tych dwóch argumentów w `TerrainRenderer` nie zgłasza błędu: podłoże wyszłoby wyblakłe, a jego nierówności oświetlone krzywo. `AssetCache::texture` loguje błąd tylko wtedy, gdy ten sam plik zostanie zamówiony raz jako sRGB, a raz jako liniowy.
15. **macOS, niesprawdzone.** `glPolygonMode` z `GL_LINE` należy do profilu Core 4.1, ale sterownik Apple jeszcze tego kodu nie widział.
16. **"Wireframe wyłącza cienie terenu".** Nie wyłącza. Do mapy cieni teren idzie zawsze wypełniony (sekcja 2.12), więc wzgórza rzucają cień także wtedy, gdy w scenie widać z nich same linie.
17. **"Teren kosztuje jedno wywołanie rysujące".** Od czwartej części M7 dwa na klatkę przy włączonych cieniach: jedno do mapy cieni, jedno do sceny (sekcja 3.2).

## 8. Ćwiczenia

Ćwiczenia od 1 do 5 są na kartce, pozostałe w kodzie. Po ćwiczeniu wycofaj zmianę.

1. **Rozmiar siatki.** Ile punktów, kwadratów i trójkątów ma teren labiryntu 4 x 6? (Odpowiedź: `(4 + 14) * 4 + 1 = 73` kolumny, `(6 + 14) * 4 + 1 = 81` wierszy, 5913 punktów, `72 * 80 = 5760` kwadratów, 11520 trójkątów.)
2. **Relief.** Policz `relief` dla punktu 3,5 m od labiryntu. (Odpowiedź: `t = 0,25`, `smoothstep = 0,25 * 0,25 * (3 - 0,5) = 0,156`, `relief = 0,6 + 3,9 * 0,156 = 1,21 m`.)
3. **Odległość za narożnikiem.** Ile wynosi `distanceOutsideMaze` dla punktu `(-3, -4)` przy dowolnym labiryncie? A dla `(-3, 1)`? (Odpowiedź: 5 i 3. Pierwszy punkt leży za narożnikiem, więc liczy się odległość do narożnika `(0, 0)`. Drugi leży obok labiryntu, bo każdy labirynt ma co najmniej 2 m głębokości.)
4. **Wysokość na trójkącie.** Kwadrat ma narożniki `NW = 1,0`, `NE = 2,0`, `SW = 0,0`, `SE = 1,0`. Policz wysokość w `alongX = 0,25`, `alongZ = 0,75`. Który to trójkąt? Ile dałoby mieszanie dwuliniowe? (Odpowiedź: południowo-zachodni, `1 + 0,75 * (0 - 1) + 0,25 * (1 - 0) = 0,5`. Dwuliniowo też 0,5: te cztery narożniki leżą w jednej płaszczyźnie.)
5. **Kiedy różnica jest największa.** Dla `NW = 0`, `NE = 1`, `SW = 1`, `SE = 0` policz wysokość w środku kwadratu obiema metodami. A gdyby przekątna biegła od `NE` do `SW`? (Odpowiedź: 0 i 0,5. Przy drugiej przekątnej trójkąty dałyby 1.)
6. **Druga przekątna.** W `buildTerrainMesh` zamień indeksy na `{northWest, southWest, northEast}` i `{northEast, southWest, southEast}`. Uruchom testy. Który przestał przechodzić? Włącz `Wireframe` i obejrzyj siatkę.
7. **Dwuliniowo.** Zastąp ciało `heightAt` mieszaniem dwuliniowym czterech narożników. Które testy to wykrywają i o ile milimetrów się mylą?
8. **Prosta zamiast smoothstep.** W `terrainRelief` zastąp `smoothStep(t)` samym `t`. Przeleć w noclip nad zewnętrzną ścianę i poszukaj załamania terenu na granicy labiryntu.
9. **Rozciąganie zamiast kafelkowania.** W konstruktorze `Terrain` zastąp `x / HEIGHTMAP_SPAN` przez `(x - m_minX) / (maxX() - m_minX)` i to samo dla `z`. Wygeneruj labirynty 4 x 4 i 30 x 30 i porównaj wielkość pagórków.
10. **Ściany na środku zamiast na najniższym narożniku.** W `lowerToGround` użyj `terrain.heightAt(segment.position.x, segment.position.z)`. Ustaw skalę 2,5 i poszukaj szpar pod ścianami. Który test przestał przechodzić?
11. **Stan, który wycieka.** W `TerrainRenderer::draw` usuń drugie `glPolygonMode` i włącz `Wireframe`. Co się stało z resztą sceny? Odznacz pole: czy teren wrócił do wypełnionych trójkątów? Dlaczego panele wyglądają normalnie?
12. **Gęstsza siatka.** Zmień `TERRAIN_STEPS_PER_CELL` na 8. Ile trójkątów pokazuje panel? Czy teren wygląda inaczej? Dlaczego prawie nie? (Wskazówka: piksel mapy ma 0,19 m, a najdrobniejsza oktawa szumu 3 m.)
13. **Inne wzgórza** (wymaga Blendera). W `make_heightmap.py` zmień `HEIGHTMAP_SEED`, uruchom skrypt i testy. Który test może przestać przechodzić i dlaczego nie jest to błąd kodu?

## 9. Pytania kontrolne

1. **Czym jest mapa wysokości?**
   Obrazem, w którym jasność piksela jest wysokością podłoża w tym miejscu, a nie kolorem. Gra czyta pierwszy kanał każdego piksela i dzieli go przez 255, więc dostaje liczby od 0 do 1.

2. **Jaki jest wzór na wysokość punktu terenu?**
   `wysokość = heightScale * relief(d) * próbka(x, z)`. Próbka to wartość mapy od 0 do 1, relief mówi, ile metrów jest warta jedynka w odległości `d` od labiryntu (od 0,6 m do 4,5 m), a skala to suwak.

3. **Po co jest relief zależny od odległości?**
   Żeby jeden obraz dał dwa rodzaje terenu: łagodne nierówności pod labiryntem, po których da się chodzić między ścianami, i wzgórza dookoła.

4. **Co to jest smoothstep i dlaczego jest tu użyty?**
   Krzywa `3t² - 2t³`, która rośnie od 0 do 1 i ma zerowe nachylenie na obu końcach. Relief zaczyna rosnąć na granicy labiryntu bez skoku nachylenia, więc teren nie ma tam załamania.

5. **Ile punktów ma siatka i skąd ta liczba?**
   `(N + 14) * 4 + 1` w każdą stronę dla labiryntu o `N` komórkach: komórki labiryntu plus 7 komórek marginesu z każdej strony, 4 kroki na komórkę, a rząd z `K` kroków ma `K + 1` punktów. Dla 10 x 10 to 97 x 97.

6. **Dlaczego liczba kroków na komórkę jest całkowita?**
   Żeby każda granica i każdy środek komórki były punktami siatki. Ściany, słupki, kryształy, start i wyjście stoją właśnie tam.

7. **Dlaczego mapa wysokości jest wczytywana z `RowOrder::TopFirst`?**
   Bo górny wiersz obrazu ma być północną krawędzią terenu. Loader domyślnie odwraca wiersze dla tekstur, a tutaj obraz nie jest teksturą, tylko tablicą wysokości czytaną przez procesor.

8. **Dlaczego mapa jest kafelkowana co 48 m, a nie rozciągana na cały teren?**
   Żeby pagórki miały tę samą wielkość niezależnie od rozmiaru labiryntu. Przy rozciąganiu duży labirynt miałby szerokie, łagodne wzgórza, a mały ciasne i strome. Ceną jest wymóg, żeby obraz powtarzał się bez szwu.

9. **Jak `Heightmap::sample` czyta wartość między pikselami?**
   Dwuliniowo: bierze cztery wartości wokół miejsca, miesza wzdłuż wiersza w dwóch wierszach, a potem między wierszami. Z `u` i `v` bierze tylko część ułamkową, a za ostatnią wartością jest znów pierwsza, więc obraz się powtarza.

10. **Na czym polega konwencja przekątnej?**
    Każdy kwadrat siatki jest cięty na dwa trójkąty przekątną od narożnika północno-zachodniego do południowo-wschodniego. Tej samej przekątnej używają `buildTerrainMesh` (indeksy) i `heightAt` (wysokość), więc gra liczy wysokość na tej samej powierzchni, którą rysuje karta.

11. **Jak `heightAt` liczy wysokość w punkcie?**
    Znajduje kwadrat siatki, potem trójkąt (porównując `alongX` z `alongZ`), a potem idzie od narożnika `NW` po dwóch krawędziach trójkąta, dodając części różnic wysokości. To równanie płaszczyzny trójkąta.

12. **Dlaczego nie mieszanie dwuliniowe czterech narożników?**
    Bo daje inną powierzchnię niż dwa płaskie trójkąty: zgadza się z nimi na krawędziach kwadratu, ale nie w środku. Różnica w środku to `(NW + SE - NE - SW) / 4`. Rzeczy stawiane wynikiem dwuliniowym lekko by wisiały albo tonęły.

13. **Jak liczona jest normalna wierzchołka terenu?**
    Z różnic centralnych: nachylenie wzdłuż X to różnica wysokości sąsiada wschodniego i zachodniego podzielona przez ich odległość, tak samo wzdłuż Z. Normalna to `normalize(-sx, 1, -sz)`.

14. **Dlaczego `v = -z / 4`, z minusem?**
    Żeby tekstura leżała na ziemi tak, jak wygląda w pliku, a nie odbita. Odbita tekstura zepsułaby mapę normalnych, bo shader liczy bitangentę jako `cross(N, T)`, co jest poprawne tylko dla tekstury nieodbitej.

15. **Na jakiej wysokości stoi ściana i dlaczego?**
    Na najniższym narożniku siatki spośród kwadratów, których dotyka jej obrys powiększony o 5 cm. Płaski trójkąt nigdy nie jest niżej niż jego najniższy narożnik, więc pod podstawą ściany nigdzie nie ma szpary. Z drugiej strony ściana jest wtedy trochę zakopana, czego nie widać.

16. **Po co `FOOTPRINT_MARGIN`?**
    Modele są przy ziemi szersze niż pudełka kolizji (stopa słupka 0,4 m, pudełko 0,3 m). Zapas 5 cm z każdej strony obejmuje cały model.

17. **Czy teren zmienił kolizje gracza ze ścianami?**
    W poziomie nie: nic nie zostało przesunięte w bok, więc rzuty pudełek na płaszczyznę XZ są te same. Test przechodzi ten sam labirynt na płaskim i na nierównym gruncie i wymaga tych samych `x` i `z` w każdym kroku.

18. **Co chroni `MAX_HEIGHT_SCALE = 2,5`?**
    Nakładanie się pudełek na osi Y. Test kolizji jest trójwymiarowy, a ściana i gracz stoją teraz na różnych wysokościach. Największa różnica wysokości w labiryncie to `0,6 * 2,5 = 1,5 m`, mniej niż 1,8 m wzrostu gracza, więc pudełko gracza zawsze sięga pudełka ściany, przy której stoi.

19. **Skąd gracz zna swoją wysokość?**
    `Player::update` czyta `terrain.heightAt(x, z)` przed ruchem i po nim. Nie ma grawitacji ani kolizji z ziemią: wysokość jest odczytywana, a ruch klawiszami jest poziomy.

20. **Dlaczego zmiana skali buduje teren od nowa na procesorze, zamiast mnożyć wysokość w shaderze?**
    Bo wysokości potrzebuje nie tylko obraz. Za terenem muszą pójść pudełka kolizji ścian, kryształy, strefa wyjścia, stopy gracza i trawa, a wszystko to liczy procesor. Uniform w shaderze przesunąłby tylko narysowane wierzchołki ([`../../decisions/height-scale-rebuilds-terrain.md`](../../decisions/height-scale-rebuilds-terrain.md)).

21. **Co robi `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`?**
    Każe rasteryzatorowi rysować krawędzie trójkątów zamiast je wypełniać. Shadery, tekstury i światło działają bez zmian. To stan globalny, więc po narysowaniu terenu jest przywracany na `GL_FILL`.

22. **Dlaczego `GL_FRONT_AND_BACK`, a nie `GL_FRONT`?**
    Profil Core dopuszcza tylko tę wartość: tryb jest zawsze wspólny dla obu stron trójkąta.

23. **Jakimi shaderami jest rysowany teren?**
    Tymi samymi co ściany: `textured`, `lit` albo `gouraud`, zależnie od trybu cieniowania. Macierz modelu jest jednostkowa, bo wierzchołki są już w przestrzeni świata.

24. **Dlaczego mapy wysokości nie ma w panelu Assets?**
    Jest wczytywana poza `AssetCache`, funkcją `loadHeightmap`. Pamięć podręczna trzyma tekstury na karcie, a mapa wysokości na kartę nie trafia.

25. **Co się dzieje, gdy pliku `heightmap.png` brakuje?**
    Loader wypisuje błąd, `loadHeightmap` zwraca pustą `Heightmap` (jedna wartość 0) i świat jest płaski. Gra działa.

26. **Ile wywołań rysujących kosztuje teren i ile kosztowały płytki?**
    Jedno w przebiegu sceny, niezależnie od rozmiaru labiryntu. Od czwartej części M7 drugie w przebiegu cieni, gdy cienie są włączone. Płytki kosztowały jedno na komórkę, czyli 100 w labiryncie startowym.

27. **Co teren ma wspólnego z mapą cieni księżyca?**
    Trzy rzeczy. Jest rysowany do mapy cieni, zawsze wypełniony, więc wzgórza rzucają cień. Przyjmuje cień jak ściany, bo rysują go te same programy `lit` i `gouraud`. I wyznacza obszar mapy: `shadowCasterBounds` bierze z niego sześć granic (`minX`, `maxX`, `minZ`, `maxZ`, `minHeight`, `maxHeight` plus wysokość słupka), a `directionalLightSpace` dopasowuje do tego pudełka rzut ortograficzny księżyca. Liczone w każdej klatce, więc nowy labirynt i nowa skala wysokości nie wymagają osobnego kodu.

## 10. Źródła

- LearnOpenGL, "Height Map" w części "Guest Articles" (<https://learnopengl.com/Guest-Articles/2021/Tessellation/Height-map>): mapa wysokości jako obraz, siatka z wierzchołków, kolejność indeksów. Artykuł buduje paski trójkątów, tutaj jest lista trójkątów.
- Specyfikacja OpenGL 4.1 Core (<https://registry.khronos.org/OpenGL/specs/gl/glspec41.core.pdf>): część "Polygon Rasterization and Depth Offset" (`PolygonMode`, dopuszczalna wartość `FRONT_AND_BACK`).
- docs.gl: `glPolygonMode` (<https://docs.gl/gl4/glPolygonMode>), `glDrawElements` (<https://docs.gl/gl4/glDrawElements>).
- Khronos, strona referencyjna GLSL `smoothstep` (<https://registry.khronos.org/OpenGL-Refpages/gl4/html/smoothstep.xhtml>): ta sama krzywa, którą `Terrain.cpp` liczy ręcznie.
- Dokumenty w tym repozytorium: [`grass-geometry.md`](grass-geometry.md) (trawa, która stoi na tym terenie), [`../gfx/mesh.md`](../gfx/mesh.md) i [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md) (siatka i indeksy), [`../gfx/textures.md`](../gfx/textures.md) (`GL_REPEAT`), [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md) (styczne, mapa normalnych), [`../assets/images.md`](../assets/images.md) (`RowOrder`), [`../game/maze-rendering.md`](../game/maze-rendering.md) (`MazeWorld`, `ModelDraw`), [`../game/player.md`](../game/player.md) (gracz), [`../game/gameplay.md`](../game/gameplay.md) (kryształy, brama, wyjście), [`../scene/collision.md`](../scene/collision.md) (test nakładania na trzech osiach), [`../debug-ui.md`](../debug-ui.md) (panele), [`../../guides/blender.md`](../../guides/blender.md) (skrypty), [`README.md`](README.md) (dlaczego ten katalog).
- Notatki o decyzjach: [`../../decisions/gentle-terrain-under-maze.md`](../../decisions/gentle-terrain-under-maze.md), [`../../decisions/walls-sunk-to-lowest-corner.md`](../../decisions/walls-sunk-to-lowest-corner.md), [`../../decisions/heightmap-tiled-in-world-metres.md`](../../decisions/heightmap-tiled-in-world-metres.md), [`../../decisions/floor-tiles-retired.md`](../../decisions/floor-tiles-retired.md), [`../../decisions/height-scale-rebuilds-terrain.md`](../../decisions/height-scale-rebuilds-terrain.md).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o prymitywach i trybach rysowania wielokątów).
