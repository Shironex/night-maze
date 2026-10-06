# Moduł renderer: trawa z shadera geometrii

Kamień milowy: M6, część druga (teren i trawa). Temat wykładu: 9 (Shader geometrii).
Kod: trzy shadery [`assets/shaders/grass.vert`](../../../assets/shaders/grass.vert), [`grass.geom`](../../../assets/shaders/grass.geom) i [`grass.frag`](../../../assets/shaders/grass.frag), miejsca kępek w [`src/game/Grass.hpp`](../../../src/game/Grass.hpp) i [`Grass.cpp`](../../../src/game/Grass.cpp), rysowanie w [`src/game/GrassRenderer.hpp`](../../../src/game/GrassRenderer.hpp) i [`GrassRenderer.cpp`](../../../src/game/GrassRenderer.cpp), etap geometrii w klasie [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) i [`Shader.cpp`](../../../src/gfx/Shader.cpp), wywołanie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`plantGrass`, `drawGrass`), panel w [`src/debug/categories/WorldCategory.cpp`](../../../src/debug/categories/WorldCategory.cpp), testy w [`tests/GrassTests.cpp`](../../../tests/GrassTests.cpp).

Dokument zakłada znajomość potoku i dwóch podstawowych etapów shaderów ([`../gfx/shaders.md`](../gfx/shaders.md)), klasy `gfx::Shader` ([`../gfx/shader-class.md`](../gfx/shader-class.md)), siatki i rodzaju prymitywu ([`../gfx/mesh.md`](../gfx/mesh.md)), świateł i pliku `common/lighting.glsl` ([`../scene/lights.md`](../scene/lights.md)), bloku uniformów ([`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md)) oraz terenu ([`terrain.md`](terrain.md)), na którym trawa stoi. Jak klasa `Shader` kompiluje i linkuje trzeci etap, opisuje linia po linii [`../gfx/shader-class.md`](../gfx/shader-class.md): tutaj jest teoria shadera geometrii i wszystko, co robi z nim gra.

**Stan na dziś:** wzdłuż ścian labiryntu, po obu ich stronach, rosną kępki trawy, a na wzgórzach wokół labiryntu jest ich rzadki rozsiew. Każda kępka to **jeden punkt** w buforze wierzchołków. Trzy źdźbła, z których się składa, buduje na karcie graficznej shader geometrii `grass.geom`, w każdej klatce od nowa, i dlatego trawa kołysze się na wietrze bez wysyłania jakichkolwiek danych. Programów shaderów było od tej części sześć: doszedł `grass`, pierwszy z trzema plikami. Dziś jest ich czternaście (pierwsza część M7 dodała `composite` i `preview`, druga `bright` i `blur`, czwarta `shadow_depth`, szósta `minimap` i `minimap_overlay`, a M8, część 1, `reflect`). Zakładka World / Terrain and grass ma pole `Enabled` i suwak `Density`: to są dwa pokazy, które PRD podaje dla tematu 9 ("gęstość trawy, toggle"), oraz suwaki `Blade height` i `Wind strength`.

**Czego nie ma:** PRD wymienia w temacie 9 obok trawy także iskry wokół kryształów. **Iskier nie zbudowano**: nie ma dla nich kodu ani shadera. Trawa **nie rzuca cienia**. Od czwartej części M7 za to cień księżyca **przyjmuje**: leży w cieniu ścian tak jak grunt, na którym rośnie (akapit niżej i sekcja 2.7). Światła kryształów cieni nie rzucają, więc ich światło dociera do trawy także przez ścianę. Latarka cień rzuca od piątej części M7 (własna mapa cieni, [`shadows.md`](shadows.md)), a trawa go przyjmuje (sekcja 2.7). Mgła jest w grze od trzeciej części M7 i okrywa trawę jak wszystko inne w scenie: jest liczona po scenie, z bufora głębi, w przebiegu składającym, więc w shaderach trawy nie ma dla niej ani jednej linii ([`post-process.md`](post-process.md), sekcje 2.17 do 2.21). Bloom jest w grze od drugiej części M7, ale nie ma dla trawy osobnego kodu: najjaśniejszy kolor źdźbła (czubek) ma liniowo jasność około 0,3, więc bez mocnego światła leży pod progiem poświaty 0,8.

**Co zmieniła pierwsza część M7 (2026-10-05).** Geometria trawy i shader geometrii się nie zmieniły. W `grass.frag` doszło `#include "common/color.glsl"`: dwa końce gradientu są liczbami sRGB i kolor jest przeliczany na liniowy przed oświetleniem, a oba widoki diagnostyczne przechodzą przez `srgbToLinear` (sekcja 4.3). Wynik trafia do bufora HDR sceny, nie do okna ([`post-process.md`](post-process.md), [`../gfx/color-space.md`](../gfx/color-space.md)). Zgłoszone dla tej części: 269 przypadków testowych i 102103 asercje w Debug i Release, a po drugiej części M7 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751. Wyglądu trawy w nowym potoku nikt nie porównał ręcznie z poprzednim.

**Co zmieniła czwarta część M7 (cienie księżyca, 2026-10-05).** Księżyc rzuca cienie z mapy cieni ([`shadows.md`](shadows.md)). Dla trawy znaczy to trzy rzeczy:

- **Trawa przyjmuje cień księżyca.** `grass.frag` dołącza trzeci plik, `common/shadows.glsl`, pyta funkcję `moonShadow(gWorldPosition, moonFacing(GRASS_NORMAL))` o część światła księżyca, której fragment nie dostaje, i odejmuje ją: `light = max(lighting.diffuse - lighting.moonDiffuse * shadow, 0.0)` (sekcja 4.3). Światło otoczenia, latarka i światła kryształów zostają w cieniu takie same. (Wzór jest ze stanu po czwartej części. Od piątej `grass.frag` odejmuje jeszcze, osobno, udział latarki w jej własnym cieniu: dzisiejszy kod jest w sekcji 4.3.)
- **Trawa cienia nie rzuca.** `NightMazeApp::drawShadowCasters` rysuje do mapy cieni (księżyca, a od piątej części M7 także latarki) teren, labirynt, bramę i kryształy oraz, od M8, części 2, dźwignie i kartki, a trawy nie. Źdźbło ma u korzenia 4 cm szerokości (`ROOT_HALF_WIDTH = 0.02` w `grass.geom`) i zwęża się ku czubkowi, a jeden teksel mapy cieni to dla labiryntu startowego około 3,2 cm (policzone dla mapy 2048, przy 1024 około 6,3 cm). Cień źdźbła byłby więc migotaniem pojedynczych tekseli, które do tego rusza się z wiatrem, na ziemi, którą kępka sama zasłania. Decyzja: [`../../decisions/grass-casts-no-shadow.md`](../../decisions/grass-casts-no-shadow.md), opis: [`shadows.md`](shadows.md), sekcja 2.16.
- **`NightMazeApp::drawGrass` ustawia uniformy mapy cieni** w programie trawy: woła `m_grassShader.use()` i (od piątej części M7) `setShadowUniformsOf(...)`, czyli `setShadowUniforms` dla obu map, przed `GrassRenderer::draw` (sekcje 3.3 i 5.4).

Zmienił się też komentarz przy gradiencie w `grass.frag` (sekcja 4.3), a startowa intensywność księżyca wzrosła z 0,12 do 0,2. Geometria trawy, `grass.vert`, `grass.geom`, `placeGrass` i testy trawy są bez zmian. Zgłoszone dla Windowsa, 2026-10-05: bramka `make check` przechodzi (310 przypadków testowych i 103751 asercji, żaden z 16 nowych nie dotyczy trawy), build Debug bez błędów OpenGL. Wyglądu trawy w cieniu nikt nie oceniał ręcznie, a przeładowania shaderów przy jedenastu programach nikt nie kliknął. Na macOS nic z tej części nie było budowane.

Co jest sprawdzone (2026-10-05, Windows, stan po drugiej części M6):

- **Uruchomione przeze mnie na gotowych plikach wykonywalnych:** 256 przypadków testowych i 101232 asercje w Debug i w Release, wszystkie zaliczone. `tests/GrassTests.cpp` ma 9 przypadków.
- **Przeliczone przeze mnie niezależnie**, skryptem w Pythonie z własnym generatorem Mersenne Twister: przy gęstości startowej 2,5 labirynt startowy dostaje 1210 kępek przy ścianach i 633 na wzgórzach, razem **1843 kępki**, czyli 5529 źdźbeł. Tę samą liczbę zgłosił autor kodu z panelu działającej gry.
- **Sprawdzone w specyfikacji GLSL 4.10:** `max_vertices` przyjmuje w tej wersji tylko stałą całkowitą zapisaną wprost, a gwarantowane minimum `gl_MaxGeometryOutputVertices` to 256 i `gl_MaxGeometryTotalOutputComponents` to 1024.
- **Zgłoszone przez autora kodu, nie powtarzane:** build Debug i Release bez ostrzeżeń, obraz obejrzany na zrzutach ekranu z tymczasowych wstawek (usuniętych), około 2000 klatek na sekundę w Release przed dodaniem trawy i po (rozrzut między uruchomieniami od 1438 do 2040 jest większy niż różnica). Zepsuty celowo `grass.geom` daje błąd z nazwą pliku i numerem linii, `grass.geom(84)`, a gra działa dalej. Zgłoszenie nie mówi, czy plik był zepsuty przed startem, czy przed przeładowaniem, więc tego, że trawa rysuje się dalej poprzednim programem, nikt jeszcze nie potwierdził na ekranie.
- **Nikt nie sprawdził ręcznie:** żadnej kontrolki zakładki World / Terrain and grass ani przycisku `Reload shaders` (wtedy przy sześciu programach, dziś przy czternastu). Lista do odhaczenia: [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 16.
- **macOS:** nic. Kompilator GLSL Apple nie widział jeszcze shadera geometrii z tego projektu ([`../../guides/build-macos.md`](../../guides/build-macos.md), sekcja 2).

M6 jest kompletny w kodzie na Windowsie i nie jest zamknięty.

## 1. Po co to jest

Labirynt z kamienia na gołej ziemi wygląda sterylnie. Trawa przy ścianach to tani szczegół, który bardzo zmienia obraz. Pytanie brzmi, jak narysować kilka tysięcy źdźbeł.

Pierwszy pomysł: zbudować każde źdźbło na procesorze i wysłać jako zwykłe trójkąty. Działa, ale ma dwie wady. Bufor rośnie (1843 kępki to 27645 wierzchołków po 44 bajty, ponad megabajt), a każda zmiana kształtu, na przykład wiatr, wymagałaby przeliczenia i wysłania wszystkiego od nowa w każdej klatce.

**Shader geometrii** odwraca ten podział. Bufor zawiera tylko to, czego karta sama nie wymyśli: **gdzie** stoi kępka i jedną liczbę losową, która odróżnia ją od sąsiadki. Kształt (ile źdźbeł, jak wysokie, w którą stronę pochylone, jak wygięte przez wiatr) liczy karta, w etapie potoku, który dostaje jeden prymityw i może w jego miejsce wypisać inne. To jest temat 9 wykładu.

| Rzecz | Gdzie | Sekcja |
|---|---|---|
| etap potoku, który z punktu robi trójkąty | `grass.geom` | 2.1 do 2.4, 4.2 |
| trzeci plik w programie shaderów | `gfx::Shader` z opcjonalnym `geometryPath` | 3.1, [`../gfx/shader-class.md`](../gfx/shader-class.md) |
| siatka z punktów zamiast trójkątów | `gfx::Mesh` z `GL_POINTS`, `game::GrassRenderer` | 3.2, 5.5 |
| miejsca kępek, takie same przy każdym uruchomieniu | `game::placeGrass` | 2.8, 5.3 |
| wiatr i światło | `grass.geom`, `grass.frag` | 2.6, 2.7 |

## 2. Teoria

### 2.1 Czym jest shader geometrii i gdzie stoi w potoku

Dotąd każdy program gry miał dwa etapy: shader wierzchołków (raz na wierzchołek) i shader fragmentów (raz na fragment). Między nimi karta sama składała wierzchołki w prymitywy, przycinała je i rasteryzowała. Shader geometrii to **opcjonalny etap między nimi**:

```
bufor wierzchołków
      |
shader wierzchołków          raz na wierzchołek
      |
składanie prymitywów         wierzchołki łączone w punkty, linie albo trójkąty
      |
SHADER GEOMETRII             raz na prymityw: dostaje cały prymityw, wypisuje nowe
      |
przycinanie, dzielenie przez w, przekształcenie do okna
      |
rasteryzacja                 prymitywy zamieniane na fragmenty
      |
shader fragmentów            raz na fragment
```

(Pełny potok OpenGL 4.1 ma przed shaderem geometrii jeszcze dwa opcjonalne etapy teselacji. Gra ich nie używa.)

Trzy cechy, które odróżniają ten etap od shadera wierzchołków:

- **widzi cały prymityw naraz.** Shader wierzchołków zna jeden wierzchołek i nic o sąsiadach. Shader geometrii dostaje wszystkie wierzchołki prymitywu: jeden dla punktu, dwa dla linii, trzy dla trójkąta,
- **może zmienić liczbę wierzchołków.** Może wypisać więcej, niż dostał (z punktu 15, jak tutaj), mniej, albo nic: wtedy prymityw znika,
- **może zmienić rodzaj prymitywu.** Na wejściu punkty, na wyjściu paski trójkątów. Rodzaje wejścia i wyjścia są od siebie niezależne.

Jest ostatnim etapem, który operuje na wierzchołkach, więc to on musi zapisać do `gl_Position` pozycję w przestrzeni przycinania. Shader wierzchołków przed nim może zostawić pozycję w dowolnej przestrzeni: `gl_Position` jest między tymi dwoma etapami zwykłą zmienną, którą nikt po drodze nie interpretuje.

### 2.2 Deklaracje wejścia i wyjścia

Shader geometrii musi powiedzieć, co dostaje i co wypisuje. Służą do tego dwa kwalifikatory `layout`, bez nazwy zmiennej:

```glsl
layout(points) in;
layout(triangle_strip, max_vertices = 15) out;
```

**Wejście** to jeden z pięciu rodzajów:

| Kwalifikator | Wierzchołków na prymityw | Tryb rysowania w `glDrawElements` |
|---|---|---|
| `points` | 1 | `GL_POINTS` |
| `lines` | 2 | `GL_LINES`, `GL_LINE_STRIP`, `GL_LINE_LOOP` |
| `lines_adjacency` | 4 | `GL_LINES_ADJACENCY`, `GL_LINE_STRIP_ADJACENCY` |
| `triangles` | 3 | `GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN` |
| `triangles_adjacency` | 6 | `GL_TRIANGLES_ADJACENCY`, `GL_TRIANGLE_STRIP_ADJACENCY` |

Rodzaj wejścia **musi pasować do trybu rysowania**. Program z `layout(points) in` narysowany jako `GL_TRIANGLES` daje błąd `GL_INVALID_OPERATION` przy wywołaniu rysującym i nic nie rysuje. Dlatego siatka trawy jest utworzona z `GL_POINTS` (sekcja 3.2).

**Wyjście** to jeden z trzech rodzajów: `points`, `line_strip` albo `triangle_strip`. Nie ma "zwykłych" trójkątów ani linii: wyjściem są zawsze paski, a osobne trójkąty robi się, kończąc pasek po trzech wierzchołkach.

**`max_vertices`** to największa liczba wierzchołków, jaką shader wypisze dla jednego prymitywu wejściowego. To obietnica dla karty, która rezerwuje pod nią pamięć: wierzchołki wypisane ponad tę liczbę są po cichu porzucane. Dwa ograniczenia ze specyfikacji GLSL 4.10:

- liczba nie może przekroczyć `gl_MaxGeometryOutputVertices`, którego gwarantowane minimum to 256,
- `max_vertices` razy liczba składowych wyjściowych jednego wierzchołka nie może przekroczyć `gl_MaxGeometryTotalOutputComponents`, minimum 1024.

Trawa wypisuje 15 wierzchołków, każdy z `gl_Position` (4 składowe), `gWorldPosition` (3) i `gBladeUv` (2), czyli 9 składowych: `15 * 9 = 135`. Daleko od obu granic.

**Dlaczego `#define`, a nie `const int`.** W GLSL 4.10 wartością w `layout(...)` musi być stała całkowita zapisana wprost (gramatyka mówi `max_vertices = integer-constant`). Wyrażenie stałe, czyli na przykład nazwa zmiennej `const int`, jest tam dozwolone dopiero od GLSL 4.40. Projekt jest w wersji 4.10 (macOS nie ma nowszej), więc `grass.geom` używa preprocesora: `#define TUFT_MAX_VERTICES 15` zamienia nazwę na liczbę, zanim kompilator zobaczy linię. Cena: liczby 15 nie da się policzyć z `BLADE_COUNT * VERTICES_PER_BLADE`, trzeba ją trzymać w zgodzie ręcznie.

**Zmienne wejściowe są tablicami.** Każde wyjście shadera wierzchołków przychodzi do shadera geometrii jako tablica z jednym elementem na wierzchołek prymitywu: `out float vRandom` w `grass.vert` to `in float vRandom[]` w `grass.geom`. Dla punktu tablica ma jeden element, `[0]`. Wbudowane `gl_Position` przychodzi jako `gl_in[0].gl_Position`. Między tymi dwoma etapami nic nie jest interpolowane: shader geometrii dostaje dokładnie te wartości, które zapisał shader wierzchołków.

### 2.3 `EmitVertex` i `EndPrimitive`

Shader geometrii nie zwraca tablicy wierzchołków. Wypisuje je po jednym, dwiema funkcjami wbudowanymi:

- **`EmitVertex()`** zatwierdza jeden wierzchołek: bierze **bieżące wartości** wszystkich zmiennych wyjściowych (`gl_Position` i każdej zmiennej `out`) i dopisuje wierzchołek z tymi wartościami do bieżącego prymitywu. Po wywołaniu wartości zmiennych wyjściowych są niezdefiniowane, więc przed każdym `EmitVertex()` trzeba ustawić wszystkie od nowa,
- **`EndPrimitive()`** kończy bieżący pasek. Następny `EmitVertex()` zaczyna nowy, niepołączony z poprzednim.

Wzorzec jest więc zawsze taki sam: ustaw wyjścia, `EmitVertex()`, ustaw wyjścia, `EmitVertex()`, i tak dalej, na końcu paska `EndPrimitive()`. W `grass.geom` pierwsze dwa kroki zamyka funkcja `emitBladeVertex`.

Jeśli `main` skończy się bez `EndPrimitive()`, ostatni pasek jest zamykany sam. Jeśli pasek trójkątów ma mniej niż trzy wierzchołki, nie powstaje z niego nic.

### 2.4 Pasek trójkątów

W pasku (`triangle_strip`) każdy wierzchołek od trzeciego tworzy trójkąt z dwoma poprzednimi. `N` wierzchołków daje `N - 2` trójkąty. Źdźbło ma 5 wierzchołków, czyli 3 trójkąty:

```
        4            tip (czubek)
       / \
      2---3          middle (połowa wysokości)
      | / |
      0---1          root (korzeń)

kolejność wypisywania:  0 lewy dół, 1 prawy dół, 2 lewy środek, 3 prawy środek, 4 czubek
trójkąty:               (0, 1, 2), (1, 2, 3), (2, 3, 4)
```

Ten sam kształt jako osobne trójkąty wymagałby 9 wierzchołków. Pasek jest też jedynym rodzajem trójkątów, jaki shader geometrii umie wypisać.

W pasku co drugi trójkąt ma odwrotną kolejność wierzchołków, a OpenGL sam odwraca ich nawijanie tak, żeby cały pasek był zwrócony w jedną stronę. Dla źdźbła nie ma to znaczenia: jest płaskie i ma być widoczne z obu stron (sekcja 2.7).

Kępka to `BLADE_COUNT = 3` źdźbła, każde jako osobny pasek zakończony `EndPrimitive()`: 15 wierzchołków i 9 trójkątów na punkt. Przy 1843 kępkach karta buduje w każdej klatce 27645 wierzchołków i 16587 trójkątów z bufora, w którym jest 1843 wierzchołków.

### 2.5 Jak punkt staje się źdźbłem: matematyka każdego wierzchołka

Wszystko dzieje się w **przestrzeni świata**, gdzie "w górę" to po prostu `(0, 1, 0)`, a kierunek wiatru jest stały. Macierze widoku i rzutowania są nakładane dopiero na gotowe wierzchołki.

Dane wejściowe jednej kępki: pozycja korzenia `tuftRoot` (punkt na ziemi) i liczba losowa `r` od 0 do 1. Stałe z `grass.geom`:

| Stała | Wartość | Znaczenie |
|---|---|---|
| `BLADE_COUNT` | 3 | źdźbła w kępce |
| `ROOT_HALF_WIDTH` | 0,02 m | połowa szerokości źdźbła przy korzeniu: źdźbło ma tam 4 cm |
| `ROOT_SPREAD` | 0,025 m | o ile korzeń źdźbła jest odsunięty od środka kępki |
| `LEAN` | 0,4 | o ile czubek odchyla się od pionu, jako część wysokości źdźbła |
| `SHORTEST_BLADE` | 0,6 | najkrótsze źdźbło jako część `uBladeHeight` |
| `HALF_LEVEL` | 0,5 | poziom środkowych wierzchołków, od 0 (korzeń) do 1 (czubek) |

**Krok 1: kierunek pochylenia.** Źdźbło numer `b` (0, 1 albo 2) pochyla się w kierunku o kącie

```
turn = (r + b / 3) * 2π
leanDirection = (cos(turn), 0, sin(turn))
```

Trzy źdźbła są rozstawione co jedną trzecią obrotu (120 stopni), a `r` obraca całą kępkę, więc żadne dwie kępki nie są zwrócone tak samo.

**Krok 2: kierunek szerokości.** Źdźbło jest płaskim paskiem. Jego szerokość leży w poziomie, w poprzek pochylenia:

```
sideDirection = (-leanDirection.z, 0, leanDirection.x)
```

To `leanDirection` obrócony o 90 stopni wokół osi pionowej. Sprawdzenie: iloczyn skalarny obu wektorów to `-cos * sin + sin * cos = 0`, więc są prostopadłe, a oba mają długość 1.

**Krok 3: wysokość.** Każde źdźbło ma własną wysokość, od 60 do 100 procent `uBladeHeight`:

```
bladeRandom = fract(r * 7 + b * 0,37)
height = uBladeHeight * mix(0,6, 1, bladeRandom)
```

`fract` zostawia część po przecinku. Pomnożenie `r` przez 7 i dodanie innej wartości dla każdego źdźbła daje drugą liczbę "losową", która ma mało wspólnego z pierwszą. Karta nie ma generatora liczb losowych, więc wszystko, co ma wyglądać losowo, musi być policzone wzorem z tego, co przyszło w wierzchołku.

**Krok 4: linia środkowa na trzech poziomach.**

```
root   = tuftRoot + leanDirection * 0,025
middle = root + UP * (height * 0,5) + (leanDirection * 0,4 * height + windOffset) * 0,25
tip    = root + UP * height         + (leanDirection * 0,4 * height + windOffset)
```

Pochylenie i wiatr rosną z **kwadratem** poziomu: na poziomie 0 (korzeń) jest ich 0, na poziomie 0,5 jest `0,5 * 0,5 = 0,25`, a na poziomie 1 całość. Trzy punkty leżą więc na paraboli, a nie na prostej: źdźbło jest wygięte jak prawdziwe, korzeń stoi tam, gdzie został posadzony, a kołysze się tylko górna część.

**Krok 5: pięć wierzchołków.** Szerokość maleje liniowo: cała przy korzeniu, połowa w połowie wysokości, zero na czubku.

```
0:  root   - sideDirection * 0,02        uv (0, 0)
1:  root   + sideDirection * 0,02        uv (1, 0)
2:  middle - sideDirection * 0,01        uv (0, 0,5)
3:  middle + sideDirection * 0,01        uv (1, 0,5)
4:  tip                                  uv (0,5, 1)
```

`uv` to współrzędna źdźbła: `x` w poprzek (0 na lewej krawędzi, 1 na prawej), `y` wzdłuż (0 przy korzeniu, 1 na czubku). Shader fragmentów używa `y` do gradientu koloru.

**Przykład na liczbach.** Kępka z `r = 0,25`, źdźbło `b = 0`, `uBladeHeight = 0,3`, bez wiatru.

```
turn          = 0,25 * 2π = 90 stopni
leanDirection = (cos 90°, 0, sin 90°) = (0, 0, 1)
sideDirection = (-1, 0, 0)
bladeRandom   = fract(0,25 * 7 + 0) = fract(1,75) = 0,75
height        = 0,3 * (0,6 + 0,4 * 0,75) = 0,3 * 0,9 = 0,27 m
```

Pozycje względem środka kępki, w metrach:

| Wierzchołek | Rachunek | Wynik `(x, y, z)` |
|---|---|---|
| `root` | `(0, 0, 1) * 0,025` | `(0, 0, 0,025)` |
| 0 | `root - (-1, 0, 0) * 0,02` | `(0,02, 0, 0,025)` |
| 1 | `root + (-1, 0, 0) * 0,02` | `(-0,02, 0, 0,025)` |
| `middle` | `root + (0, 0,135, 0) + (0, 0, 0,108) * 0,25` | `(0, 0,135, 0,052)` |
| 2 | `middle - (-1, 0, 0) * 0,01` | `(0,01, 0,135, 0,052)` |
| 3 | `middle + (-1, 0, 0) * 0,01` | `(-0,01, 0,135, 0,052)` |
| 4, `tip` | `root + (0, 0,27, 0) + (0, 0, 0,108)` | `(0, 0,27, 0,133)` |

Źdźbło ma 27 cm wysokości, 4 cm szerokości u dołu i czubek odchylony o 10,8 cm w stronę `+Z`. Źdźbła `b = 1` i `b = 2` tej samej kępki pochylają się pod kątami 210 i 330 stopni i mają wysokości `fract(2,12) = 0,12` i `fract(2,49) = 0,49`, czyli 0,194 m i 0,239 m.

### 2.6 Wiatr

Wiatr to jedno przesunięcie poziome, wspólne dla wszystkich źdźbeł kępki, liczone raz na kępkę:

```
windPhase  = uTime * 1,9 + r * 2π - dot(tuftRoot.xz, WIND_DIRECTION) * 0,9
windPush   = uWindStrength * 0,06 * sin(windPhase)
windOffset = (WIND_DIRECTION.x, 0, WIND_DIRECTION.y) * windPush
```

| Stała | Wartość | Znaczenie |
|---|---|---|
| `WIND_DIRECTION` | `(0,94, 0,34)` | kierunek wiatru w płaszczyźnie XZ, długość prawie dokładnie 1 |
| `WIND_REACH` | 0,06 m | o ile czubek wychyla się w każdą stronę przy sile 1 |
| `WIND_SPEED` | 1,9 rad/s | jak szybko rośnie kąt sinusa |
| `WIND_PHASE_PER_METRE` | 0,9 rad/m | o ile kępka metr dalej z wiatrem jest opóźniona |

Sinus waha się między -1 i 1, więc czubek chodzi tam i z powrotem wzdłuż kierunku wiatru, najdalej o `uWindStrength * 6 cm`. Kąt sinusa ma trzy składniki:

- **czas:** `uTime * 1,9`. Pełne wahnięcie trwa `2π / 1,9 = 3,3 s`,
- **liczba losowa kępki:** `r * 2π` przesuwa start o dowolną część okresu, więc sąsiednie kępki nie kołyszą się równo jak żołnierze,
- **położenie:** `dot(tuftRoot.xz, WIND_DIRECTION)` to odległość kępki mierzona wzdłuż kierunku wiatru. Kępka o metr dalej jest o 0,9 radiana do tyłu. Skutek: wychylenia wędrują po trawie jak fala. Długość fali to `2π / 0,9 = 7 m`, a jej prędkość `1,9 / 0,9 = 2,1 m/s` w kierunku wiatru.

`uTime` to zegar gry w sekundach (`glfwGetTime()`), który tylko rośnie. Nie jest to zegar rundy: wiatr nie staje po wygranej i nie przeskakuje po restarcie.

Wiatr nie zmienia niczego w buforach. Punkty kępek stoją w miejscu, a zmienia się jeden uniform typu `float`. To jest właśnie zysk z budowania kształtu na karcie.

### 2.7 Oświetlenie i dlaczego odrzucanie tylnych ścian jest wyłączone

**Kolor.** Trawa nie ma tekstury: źdźbło jest na ekranie za wąskie, żeby było na nim widać obraz. Kolor to gradient od ciemnej zieleni przy korzeniu do jasnej na czubku, `mix(ROOT_COLOR, TIP_COLOR, gBladeUv.y)`. Od M7 wynik mieszania jest przeliczany z sRGB na wartości liniowe, zanim zostanie pomnożony przez światło (sekcja 4.3).

**Światło.** `grass.frag` dołącza ten sam plik `common/lighting.glsl` co `lit.frag` i woła tę samą funkcję `computeLighting`, więc trawę oświetlają dokładnie te światła co ściany: światło otoczenia, księżyc, światła kryształów i latarka, każde z własnym tłumieniem. Dwie różnice:

- **normalna jest zawsze `(0, 1, 0)`, prosto w górę.** Prawdziwa normalna płaskiego źdźbła wskazuje w bok. Z nią źdźbło byłoby jasne z jednej strony, czarne z drugiej i migałoby, gdy kamera je mija, a trzy źdźbła jednej kępki miałyby trzy różne jasności. Z normalną podłoża cała kępka jest tak jasna jak ziemia wokół niej, co dla trawy widzianej z kilku metrów jest wiarygodniejsze,
- **używana jest tylko część rozproszona** (`.diffuse`). Trawa nie błyszczy, więc odbłysk jest pominięty. Plik `lighting.glsl` i tak go liczy, dlatego `GrassRenderer` ustawia mu bezpieczne wartości: siłę 0 i wykładnik 1 (potęga zera o wykładniku 0 jest w GLSL niezdefiniowana).

**Cień księżyca (czwarta część M7).** Trawa przyjmuje cień księżyca tak samo jak ściany i grunt. `computeLighting` o cieniach nic nie wie: zwraca w polu `moonDiffuse` udział księżyca, który jest już zawarty w `diffuse`. `grass.frag` pyta mapę cieni funkcją `moonShadow` i ten udział, pomnożony przez wynik, odejmuje. Kępka w cieniu ściany ma więc tylko światło otoczenia i to, co dociera z latarki i kryształów, czyli dokładnie tyle co ziemia pod nią. Do biasu cienia idzie ta sama stała normalna `(0, 1, 0)` co do światła: `moonFacing(GRASS_NORMAL)` to cosinus kąta między pionem a kierunkiem do księżyca, przy ustawieniach startowych `sin(50°) = 0,77`, taki sam na całej trawie.

Sama trawa cienia **nie rzuca**: nie jest rysowana do mapy cieni. Liczby: źdźbło ma u korzenia 4 cm szerokości (dwa razy `ROOT_HALF_WIDTH = 0.02`), w połowie wysokości 2 cm, a czubek jest punktem. Teksel mapy cieni księżyca dla labiryntu startowego to około 3,2 cm przy rozdzielczości 2048 i około 6,3 cm przy 1024 (policzone, [`shadows.md`](shadows.md), sekcja 2.4). Źdźbło jest więc mniej więcej tak szerokie jak jeden teksel: jego cień byłby pojedynczymi tekselami, które zapalają się i gasną, gdy wiatr przesuwa źdźbło, i to na ziemi, którą kępka i tak zasłania. Koszt też by wzrósł: shader geometrii budowałby wszystkie źdźbła drugi raz w każdej klatce. Decyzja: [`../../decisions/grass-casts-no-shadow.md`](../../decisions/grass-casts-no-shadow.md), szerzej: [`shadows.md`](shadows.md), sekcja 2.16. W trybie `Unlit` i w obu widokach diagnostycznych trawa cienia nie pokazuje: test cienia stoi w gałęzi `if (uLit)`, a widoki wychodzą z `main` wcześniej.

**Jeden program na wszystkie tryby cieniowania.** Ściany mają osobny program dla trybu Gouraud, który liczy światło w wierzchołkach. Trawa takiego nie ma i mieć nie może w zwykłym sensie: kępka nie ma w żadnym buforze wierzchołków, w których dałoby się policzyć światło, jej wierzchołki powstają dopiero w shaderze geometrii. Trawa jest więc cieniowana **na fragment także w trybie Gouraud**. W trybie `Unlit` uniform `uLit` jest wyłączony i trawa ma pełną jasność, jak reszta sceny ([`../../decisions/grass-lit-with-up-normal.md`](../../decisions/grass-lit-with-up-normal.md)).

**Odrzucanie tylnych ścian** (face culling) wyrzuca trójkąty widziane od tyłu. Ma sens dla brył zamkniętych, których wnętrza nigdy nie widać. Źdźbło jest jednym płaskim paskiem bez "drugiej strony": z włączonym odrzucaniem znikałaby mniej więcej połowa źdźbeł, zależnie od tego, z której strony stoi kamera. Gra dziś nie włącza `GL_CULL_FACE` nigdzie, więc `GrassRenderer::draw` zwykle nic nie musi robić. Sprawdza jednak stan (`glIsEnabled`) i gdyby odrzucanie było włączone, wyłącza je na czas rysowania trawy i włącza z powrotem. To zabezpieczenie na przyszłość, nie kod, który dziś coś zmienia.

Normalna stała w górę ma tu drugą zaletę: nie zależy od tego, którą stronę źdźbła widać, więc obie strony są oświetlone tak samo bez pytania o `gl_FrontFacing`.

### 2.8 Miejsca kępek: algorytm na procesorze

Shader geometrii buduje kształt, ale nie decyduje, **gdzie** rośnie trawa. To robi `game::placeGrass` na procesorze, raz na labirynt, i zwraca listę struktur `GrassTuft` (pozycja i liczba losowa).

**Przy ścianach.** Dla każdego segmentu ściany, po obu jego stronach:

```
kępek na stronę = round(density * WALL_LENGTH)
```

Gęstość to kępki na metr ściany, liczone osobno dla każdej strony. Przy startowej 2,5 i ścianie 2 m wychodzi 5 na stronę, 10 na ścianę. Każda kępka dostaje dwie liczby losowe:

- **wzdłuż ściany:** miejsce od `-REACH` do `+REACH` od środka ściany, gdzie `REACH = WALL_LENGTH / 2 - GRASS_END_CLEARANCE = 1 - 0,3 = 0,7 m`. Kępki trzymają się 30 cm od obu końców ściany, bo tam stoi słupek (jego stopa sięga 0,2 m wzdłuż ściany) albo ściana prostopadła (zajmuje 0,15 m),
- **w poprzek:** odległość od osi ściany, od `STRIP_START` do `STRIP_START + GRASS_STRIP_WIDTH`, gdzie `STRIP_START = WALL_COLLISION_THICKNESS / 2 + GRASS_WALL_GAP = 0,15 + 0,06 = 0,21 m`, a pas ma 0,22 m szerokości. Kępka stoi więc od 21 do 43 cm od osi ściany. Przerwa 6 cm jest trochę większa niż `FOOTPRINT_MARGIN` (5 cm), więc kępka nie stoi w cokole ściany. Losowe miejsce w pasie sprawia, że trawa nie jest linią od linijki.

Dla ściany wzdłuż X pierwsza liczba idzie do `x`, druga do `z`. Dla ściany wzdłuż Z odwrotnie.

**Przy bramie trawy nie ma.** Brama nie jest na liście `world.walls`: otwarty bok komórki wyjścia nie ma ściany, więc nie ma też pasa.

**Na wzgórzach.** Rzadki rozsiew metodą prób: losowane są punkty na całej ziemi (razem z labiryntem), a te, które leżą bliżej labiryntu niż `GRASS_HILL_CLEARANCE = 0,6 m`, są odrzucane. Liczba prób to powierzchnia całej ziemi razy gęstość:

```
gęstość na m² = GRASS_HILL_TUFTS_PER_SQUARE_METRE * density / DEFAULT_GRASS_DENSITY
              = 0,35 * density / 2,5
prób          = round(szerokość ziemi * głębokość ziemi * gęstość na m²)
```

Liczba prób jest stała ("tyle, ile kępek dostałaby cała ziemia"), a nie "dopóki nie znajdę dość kępek". Pętla ma wtedy znaną długość, a to, co zostaje po odrzuceniu, ma żądaną gęstość na ziemi poza labiryntem.

**Na ziemi.** Wysokość każdej kępki to `terrain.heightAt(x, z)`, ta sama funkcja, która stawia gracza ([`terrain.md`](terrain.md), sekcja 2.7), więc korzenie leżą dokładnie na narysowanej powierzchni.

**Rachunek dla labiryntu startowego.** Labirynt doskonały 10 x 10 ma zawsze 121 segmentów ścian: 40 na obwodzie i 81 w środku (180 krawędzi wewnętrznych minus 99 przejść, bo drzewo o 100 komórkach ma 99 połączeń).

```
przy ścianach:  121 ścian * 2 strony * 5 = 1210 kępek
na wzgórzach:   48 m * 48 m * 0,35 = 806,4, czyli 806 prób
                z nich 633 leżą dalej niż 0,6 m od labiryntu
razem:          1210 + 633 = 1843 kępki, 5529 źdźbeł
```

Liczba 633 nie wynika ze wzoru, tylko z ciągu liczb losowych dla ziarna 1. Spodziewana wartość to `806 * (2304 - 400) / 2304 = 666` (ziemia bez labiryntu to 1904 m² z 2304 m²), minus pas 0,6 m wokół labiryntu. Test dopuszcza odchylenie o jedną piątą.

**Skąd 1843, żeby dało się to odtworzyć (sprawdzone 2026-10-06 na kodzie z HEAD).** 1210 i 806 wynikają z wzorów wyżej: `lround(2,5 * 2,0) = 5` kępek na stronę, 121 ścian, ziemia `48 * 48 m` (14 m marginesu z każdej strony labiryntu 20 x 20 m, `TERRAIN_MARGIN`), `lround(2304 * 0,35) = 806`. Liczby 633 nie da się wyliczyć na kartce, tylko przejść ciągiem z kodu: generator `std::mt19937` z ziarnem `1 + 2000003` (ziarno labiryntu plus `GRASS_SEED_OFFSET`), 3630 liczb zużytych przez kępki przy ścianach (`1210 * 3`: miejsce wzdłuż, miejsce w poprzek i liczba kępki, każda przez `randomBelow(generator, 4096) / 4096`), potem dla każdej z 806 prób dwie liczby na `x` i `z` (`-14 + 48 * liczba`), odrzucenie punktu, gdy `distanceOutsideMaze` jest mniejsze niż 0,6, a dla zatrzymanego jeszcze jedna liczba kępki. Sam przeszedłem ten ciąg własnym generatorem Mersenne Twister napisanym w Pythonie (sprawdzonym na pierwszej wartości `mt19937` dla ziarna 5489, 3499211612) i dostałem 633 zatrzymane próby, czyli 1843. To zgadza się z liczbą z zakładki World / Terrain and grass zgłoszoną w drugiej części M6. Samej gry dla tego sprawdzenia nie uruchamiałem.

### 2.9 Powtarzalność: to samo ziarno, ta sama trawa

Ten sam świat i ta sama gęstość dają zawsze te same kępki, na każdym systemie i kompilatorze. Trzy rzeczy na to pracują ([`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md)):

- **generator `std::mt19937`**, którego ciąg liczb jest ustalony przez standard C++,
- **własne ziarno:** generator trawy startuje z `world.seed + GRASS_SEED_OFFSET`, gdzie przesunięcie to 2000003. Dzięki temu trawa nie powtarza liczb, którymi wykuto labirynt ani którymi rozstawiono kryształy,
- **własna funkcja `randomUnit`** zamiast `std::uniform_real_distribution`. To, jak rozkład zużywa generator, różni się między bibliotekami standardowymi, więc to samo ziarno dałoby inną trawę na macOS niż na Windowsie. `randomUnit` bierze liczbę całkowitą z `game::randomBelow(generator, 4096)` i dzieli ją przez 4096. Liczba całkowita podzielona przez potęgę dwójki jest tą samą liczbą `float` wszędzie.

**Kolejność jest częścią wyniku.** Każda kępka zabiera z generatora następne liczby, więc lista ścian jest przechodzona w stałej kolejności (`world.walls`, wiersz po wierszu), dla każdej ściany najpierw strona ujemna, potem dodatnia, a w każdej kępce zawsze w tym samym porządku: miejsce wzdłuż, miejsce w poprzek, liczba losowa kępki. Wzgórza są na końcu. Zamiana dwóch linii w kodzie zmieniłaby całą trawę.

Trawa nie zależy od skali wysokości terenu w poziomie: zmiana suwaka `Height scale` sadzi ją od nowa w tych samych miejscach `(x, z)`, tylko na innej wysokości.

### 2.10 Koszt

Uczciwie: pomiar zgłoszony dla tej części (około 2000 klatek na sekundę przed i po, z rozrzutem większym niż różnica) mówi tylko, że 1843 kępki nie są na tej karcie widocznym kosztem. Rozważania poniżej są ogólne, nie zmierzone w tym projekcie.

- **Shader geometrii nie jest darmowy.** Karta musi zarezerwować miejsce na `max_vertices` wierzchołków dla każdego prymitywu wejściowego, a wypisywanie wierzchołków po jednym słabo się zrównolegla. Stąd reguła z komentarza w `grass.geom`: `max_vertices` ma być małe. Na wielu kartach ten sam efekt taniej dałoby rysowanie instancjami (jedno źdźbło w buforze, tysiące kopii), którego nie ma wśród 15 tematów wykładu (temat 15 to selekcja obiektów, [`../../syllabus.md`](../../syllabus.md)).
- **Koszt rośnie z gęstością liniowo.** Suwak `Density` na maksimum (8) daje 16 kępek na stronę ściany, czyli `121 * 32 = 3872` kępki przy ścianach, i 2580 prób na wzgórzach.
- **Nie ma odrzucania niewidocznych kępek.** Wszystkie punkty idą do karty w jednym wywołaniu rysującym, także te za plecami gracza i za ścianami. Shader geometrii buduje źdźbła także dla nich, a dopiero przycinanie i test głębi je odrzucają.
- **Koszt fragmentów jest mały.** Źdźbło ma na ekranie kilka pikseli szerokości. Każdy jego fragment liczy jednak pełne oświetlenie ze wszystkich świateł, a od czwartej części M7 także test cienia księżyca: przy ustawieniach startowych (PCF o promieniu 1) to 9 odczytów mapy cieni na fragment. Tego kosztu nie mierzyłem.
- **Trawa jest rysowana przed niebem i po labiryncie.** Pisze głębię jak każda nieprzezroczysta rzecz.

## 3. Jak to działa w OpenGL

### 3.1 Program z trzema etapami

Program `grass` powstaje w konstruktorze `gfx::Shader` z trzech plików. Różnica wobec programu z dwóch plików to jeden obiekt shadera więcej:

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glCreateShader(GL_VERTEX_SHADER)`, `glShaderSource`, `glCompileShader` | kompiluje `grass.vert` |
| 2 | `glCreateShader(GL_GEOMETRY_SHADER)`, `glShaderSource`, `glCompileShader` | kompiluje `grass.geom`. Jedyna nowość: stała rodzaju shadera |
| 3 | `glCreateShader(GL_FRAGMENT_SHADER)`, `glShaderSource`, `glCompileShader` | kompiluje `grass.frag`, po rozwinięciu `#include "common/lighting.glsl"` |
| 4 | `glCreateProgram`, `glAttachShader` trzy razy, `glLinkProgram` | linkuje. Linker sprawdza, czy wyjścia każdego etapu pasują do wejść następnego: `vRandom` z `grass.vert` do `vRandom[]` w `grass.geom`, `gWorldPosition` i `gBladeUv` z `grass.geom` do `grass.frag` |
| 5 | `glDetachShader` i `glDeleteShader` trzy razy | obiekty shaderów nie są już potrzebne |
| 6 | `glGetUniformBlockIndex`, `glUniformBlockBinding` | blok `LightBlock` programu dostaje punkt wiązania bufora świateł (`m_lightRig.connect(m_grassShader)`) |

Kolejność etapów w potoku wynika z ich rodzaju, a nie z kolejności dołączania. Ważny skutek punktu 4: gdy program ma shader geometrii, **shader fragmentów dostaje wejścia od niego, a nie od shadera wierzchołków**. `grass.frag` nie widzi `vRandom`. Wszystko, czego potrzebuje, shader geometrii musi mu przekazać sam.

Kod klasy linia po linii: [`../gfx/shader-class.md`](../gfx/shader-class.md). Przeładowanie na żywo czyta wszystkie trzy pliki: [`../gfx/shader-hot-reload.md`](../gfx/shader-hot-reload.md).

### 3.2 Siatka z punktów

`GrassRenderer::upload` tworzy `gfx::Mesh` z prymitywem `GL_POINTS`: jeden wierzchołek na kępkę i jeden indeks na wierzchołek (0, 1, 2 i tak dalej). Klasa `Mesh` zawsze rysuje przez `glDrawElements`, więc indeksy są potrzebne, chociaż tylko liczą w górę.

Wierzchołek to zwykły `gfx::Vertex` (44 bajty), z którego trawa używa dwóch pól: pozycji (atrybut 0) i współrzędnej tekstury (atrybut 2), w której `u` niesie liczbę losową kępki. Normalna i styczna zostają zerami. 1843 kępki to 81 092 bajty wierzchołków i 7372 bajty indeksów.

### 3.3 Klatka: `NightMazeApp::drawGrass` i `GrassRenderer::draw`

Kroki od 1 do 6 doszły w czwartej części M7 i należą do `NightMazeApp::drawGrass` (pięć ostatnich z nich wykonuje `game::setShadowUniforms`). Kroki od 7 do 14 to `GrassRenderer::draw`, bez zmian od M6.

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glUseProgram(grass)` | `m_grassShader.use()` w `drawGrass`: uniform trafia do programu bieżącego, więc program musi być wybrany przed krokiem 2 |
| 2 | `glUniform1i` dla `uMoonShadowMap` (3) i `uMoonShadowEnabled` | numer jednostki tekstur, na której leży mapa cieni księżyca, i przełącznik: czy przebieg cieni wypełnił mapę w tej klatce |
| 3 | `glUniformMatrix4fv` dla `uMoonShadowMatrix` | z przestrzeni świata do przestrzeni przycinania księżyca |
| 4 | `glUniform1f` dla `uMoonShadowConstantBias` i `uMoonShadowSlopeBias` | dwie części biasu, już przeliczone z metrów na różnicę głębi |
| 5 | `glUniform1i` dla `uMoonShadowPcfRadius` | promień jądra PCF w tekselach, 0 przy wyłączonym PCF |
| 6 | `glUniform1f` dla `uMoonShadowStrength` | jaką część światła księżyca cień zabiera |
| 7 | `glUseProgram(grass)` | `shader.use()` w `GrassRenderer::draw`: drugi raz ten sam program, co niczego nie zmienia |
| 8 | `glUniformMatrix4fv` dla `uView` i `uProjection` | macierze klatki, używane w `grass.geom` |
| 9 | `glUniform1f` dla `uTime`, `uBladeHeight`, `uWindStrength` | zegar wiatru i dwa suwaki |
| 10 | `glUniform1i` dla `uLit` i `uViewMode` | tryb cieniowania i widok diagnostyczny |
| 11 | `glUniform1f` dla `uSpecularStrength` (0) i `uShininess` (1) | bezpieczne wartości dla nieużywanego odbłysku |
| 12 | `glIsEnabled(GL_CULL_FACE)`, ewentualnie `glDisable(GL_CULL_FACE)` | trawa jest widoczna z obu stron |
| 13 | `glBindVertexArray`, `glDrawElements(GL_POINTS, 1843, GL_UNSIGNED_INT, ...)` | jedno wywołanie na całą trawę |
| 14 | ewentualnie `glEnable(GL_CULL_FACE)` | stan wraca |

**Rachunek, żeby było jasne, co jest liczone.** Liczę wywołania ustawiające uniformy (`glUniform1i`, `glUniform1f`, `glUniformMatrix4fv`), po jednym na uniform. Do trzeciej części M7 włącznie było ich 9 (kroki od 8 do 11: 2 macierze, 3 liczby, 2 liczby całkowite, 2 liczby). Czwarta część M7 dodała 7, dokładnie tyle, ile robi `setShadowUniforms` w [`src/game/ShadowMap.cpp`](../../../src/game/ShadowMap.cpp): 3 razy `glUniform1i` (`setInt`), 1 raz `glUniformMatrix4fv` (`setMat4`) i 3 razy `glUniform1f` (`setFloat`). Razem 16. Każde z nich poprzedza `glGetUniformLocation`, bo klasa `Shader` szuka położenia przy każdym wywołaniu. Do tego `glUseProgram` jest wołane dwa razy zamiast raz. Wywołanie rysujące jest nadal jedno: trawa nie jest rysowana do mapy cieni. **Uwaga (2026-10-06): rachunek wyżej jest ze stanu po czwartej części M7.** Od piątej `drawGrass` woła `setShadowUniformsOf`, które powtarza kroki od 2 do 6 dla mapy latarki (nazwy z przedrostkiem `uFlashlight`, jednostka 4, to samo 3 + 1 + 3 wywołania) i dodaje `glUniform3fv` dla `uFlashlightShadowLightPosition`. To 8 wywołań więcej, więc w klatce jest 9 + 7 + 8 = 24 wywołania ustawiające uniformy (15 dla cieni), a nie 16. Kroki od 1 do 6 są wykonywane także przy wyłączonych cieniach i w trybie `Unlit` (wtedy `uMoonShadowEnabled` dostaje 0 albo wynik nie jest używany). Pomijane są razem z całą trawą, gdy pole `Enabled` zakładki World / Terrain and grass jest odznaczone, i same, gdy program trawy się nie wczytał.

Światła nie są ustawiane tutaj: są w buforze uniformów, który `onRender` wypełnił przed rysowaniem sceny, a program `grass` czyta go przez blok `LightBlock`, tak samo jak `lit` i `gouraud`. Map cieni w tym bloku nie ma: sampler nie może być składnikiem bloku uniformów, więc macierze i liczby są zwykłymi uniformami, ustawianymi w każdym programie osobno (kroki od 2 do 6, a od piątej części także ich odpowiedniki dla latarki). Mapa księżyca jest podpinana do jednostki tekstur 3, a mapa latarki do jednostki 4, raz na klatkę, po swoich przebiegach cieni, i obie zostają tam, gdy rysowana jest trawa. Trawa nie podpina żadnej własnej tekstury.

Rozmiar punktu (`glPointSize`) nie ma znaczenia, bo żaden punkt nie dociera do rasteryzacji: shader geometrii zamienia każdy na trójkąty.

## 4. Shadery

### 4.1 `grass.vert`

```glsl
#version 410 core

layout(location = 0) in vec3 aPosition; // the root of the tuft, already in world space
layout(location = 2) in vec2 aUv;       // x: the random number of the tuft, 0 to 1

out float vRandom; // the random number of the tuft

void main() {
    gl_Position = vec4(aPosition, 1.0);
    vRandom = aUv.x;
}
```

- **Atrybuty 0 i 2.** Numery to umowa z `gfx::Vertex`: 0 to pozycja, 1 normalna, 2 UV, 3 styczna ([`../gfx/mesh.md`](../gfx/mesh.md)). Shader deklaruje tylko te, których używa. Niezadeklarowane atrybuty 1 i 3 są w buforze, ale nikt ich nie czyta.
- **`gl_Position = vec4(aPosition, 1.0)`.** Bez macierzy. Pozycja zostaje w przestrzeni świata, bo tam shader geometrii buduje źdźbła. W programie z dwoma etapami byłby to błąd (rasteryzator dostałby pozycję świata jako pozycję przycinania). Tutaj `gl_Position` trafia do shadera geometrii, który robi z nią, co chce.
- **`vRandom = aUv.x`.** Przekazanie liczby losowej dalej. Shader geometrii nie ma dostępu do atrybutów wierzchołka, tylko do wyjść shadera wierzchołków.

Ten shader "prawie nic nie robi", jak mówi jego komentarz, i to jest typowe dla programów z shaderem geometrii: praca przenosi się etap dalej.

### 4.2 `grass.geom`

```glsl
#version 410 core

layout(points) in;

const int BLADE_COUNT = 3;
const int VERTICES_PER_BLADE = 5;

#define TUFT_MAX_VERTICES 15

layout(triangle_strip, max_vertices = TUFT_MAX_VERTICES) out;
```

Deklaracje z sekcji 2.2. `BLADE_COUNT` jest używane w pętli i w podziale obrotu. `VERTICES_PER_BLADE` nie jest używane w kodzie: dokumentuje, skąd bierze się 15 (`3 * 5`), i przypomina, co trzeba zmienić razem.

```glsl
in float vRandom[];

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;
uniform float uBladeHeight;
uniform float uWindStrength;

out vec3 gWorldPosition;
out vec2 gBladeUv;
```

Wejście jako tablica (sekcja 2.2). Macierze widoku i rzutowania są uniformami **tego** etapu, nie shadera wierzchołków. Uniform należy do programu, a nie do etapu, więc może go zadeklarować dowolny shader programu, a C++ ustawia go tak samo. Przedrostek `g` w nazwach wyjść znaczy "z shadera geometrii", tak jak `v` znaczy "z shadera wierzchołków".

```glsl
void emitBladeVertex(vec3 worldPosition, vec2 bladeUv) {
    gWorldPosition = worldPosition;
    gBladeUv = bladeUv;
    gl_Position = uProjection * uView * vec4(worldPosition, 1.0);
    EmitVertex();
}
```

Ustawienie trzech wyjść i zatwierdzenie wierzchołka. Tu pozycja przechodzi z przestrzeni świata do przestrzeni przycinania: najpierw widok, potem rzutowanie (macierze mnoży się od prawej). Macierzy modelu nie ma, bo punkty kępek są już w przestrzeni świata. `gWorldPosition` zachowuje pozycję świata dla oświetlenia.

```glsl
void main() {
    vec3 tuftRoot = gl_in[0].gl_Position.xyz;
    float tuftRandom = vRandom[0];
```

`gl_in` to wbudowana tablica z jednym elementem na wierzchołek prymitywu wejściowego. `[0]` to jedyny wierzchołek punktu.

```glsl
    float windPhase = uTime * WIND_SPEED + tuftRandom * TWO_PI -
                      dot(tuftRoot.xz, WIND_DIRECTION) * WIND_PHASE_PER_METRE;
    float windPush = uWindStrength * WIND_REACH * sin(windPhase);
    vec3 windOffset = vec3(WIND_DIRECTION.x, 0.0, WIND_DIRECTION.y) * windPush;
```

Wiatr z sekcji 2.6, liczony raz na kępkę, przed pętlą po źdźbłach. `tuftRoot.xz` to wektor dwuwymiarowy z `x` i `z` pozycji. `WIND_DIRECTION` jest typu `vec2`, więc jego `y` jest tu składową Z świata.

```glsl
    for (int blade = 0; blade < BLADE_COUNT; ++blade) {
        float turn = (tuftRandom + float(blade) / float(BLADE_COUNT)) * TWO_PI;
        vec3 leanDirection = vec3(cos(turn), 0.0, sin(turn));
        vec3 sideDirection = vec3(-leanDirection.z, 0.0, leanDirection.x);

        float bladeRandom =
            fract(tuftRandom * BLADE_RANDOM_FACTOR + float(blade) * BLADE_RANDOM_STEP);
        float height = uBladeHeight * mix(SHORTEST_BLADE, 1.0, bladeRandom);
```

Kroki 1, 2 i 3 z sekcji 2.5. `float(blade)` jest konieczne: GLSL nie dzieli liczby całkowitej przez całkowitą z wynikiem ułamkowym (`1 / 3` to 0). `BLADE_RANDOM_FACTOR = 7,0` i `BLADE_RANDOM_STEP = 0,37` nie mają szczególnego znaczenia: komentarz mówi tylko, że nie mogą być liczbami całkowitymi ani prostymi ułamkami siebie nawzajem.

```glsl
        vec3 root = tuftRoot + leanDirection * ROOT_SPREAD;

        vec3 middle = root + UP * (height * HALF_LEVEL) +
                      (leanDirection * (LEAN * height) + windOffset) * (HALF_LEVEL * HALF_LEVEL);
        vec3 tip = root + UP * height + leanDirection * (LEAN * height) + windOffset;

        vec3 rootSide = sideDirection * ROOT_HALF_WIDTH;
        vec3 middleSide = sideDirection * (ROOT_HALF_WIDTH * (1.0 - HALF_LEVEL));
```

Krok 4 i szerokości z kroku 5. `HALF_LEVEL * HALF_LEVEL` to kwadrat poziomu, 0,25.

```glsl
        emitBladeVertex(root - rootSide, vec2(0.0, 0.0));
        emitBladeVertex(root + rootSide, vec2(1.0, 0.0));
        emitBladeVertex(middle - middleSide, vec2(0.0, HALF_LEVEL));
        emitBladeVertex(middle + middleSide, vec2(1.0, HALF_LEVEL));
        emitBladeVertex(tip, vec2(0.5, 1.0));
        EndPrimitive();
    }
}
```

Pięć wierzchołków paska w kolejności lewy, prawy, lewy, prawy, czubek (sekcja 2.4), a potem koniec paska. Bez `EndPrimitive()` następne źdźbło byłoby dalszym ciągiem tego samego paska i między czubkiem jednego a korzeniem drugiego powstałyby dodatkowe trójkąty.

### 4.3 `grass.frag`

```glsl
#version 410 core

#include "common/lighting.glsl"
#include "common/color.glsl"
#include "common/shadows.glsl"

in vec3 gWorldPosition;
in vec2 gBladeUv;

uniform bool uLit;
uniform int uViewMode;

out vec4 fragColor;
```

- **`#include`** rozwija loader shaderów, nie kompilator GLSL ([`../gfx/shader-includes.md`](../gfx/shader-includes.md)). Pierwszy dołączony plik wnosi blok `LightBlock`, uniformy `uSpecularModel`, `uSpecularStrength`, `uShininess` i funkcję `computeLighting`. Drugi (od M7) wnosi `srgbToLinear` dla kolorów zapisanych w tym pliku liczbami ([`../gfx/color-space.md`](../gfx/color-space.md)). Trzeci (od czwartej części M7) wnosi sampler `uMoonShadowMap` typu `sampler2DShadow`, sześć zwykłych uniformów mapy cieni księżyca i funkcję `moonShadow`. Komentarz nad nim w pliku mówi: "the same file lit.frag includes, so the grass lies in the shadows of the walls like the ground it grows on" ([`shadows.md`](shadows.md), sekcja 4).
- **`out vec4 fragColor`**: komentarz w pliku mówi dziś, że wyjście trafia do bufora HDR sceny jako kolor **liniowy**.
- **Wejścia** mają te same nazwy i typy co wyjścia `grass.geom`. Rasteryzator interpoluje je w obrębie każdego trójkąta źdźbła.
- **`uniform bool uLit`**: C++ ustawia go przez `setInt` wartością 0 albo 1. Dla uniformu typu `bool` OpenGL traktuje zero jako fałsz, a każdą inną wartość jako prawdę.

```glsl
const vec3 ROOT_COLOR = vec3(0.10, 0.20, 0.07);
const vec3 TIP_COLOR = vec3(0.46, 0.64, 0.26);
const vec3 GRASS_NORMAL = vec3(0.0, 1.0, 0.0);
```

Dwa końce gradientu to **liczby sRGB**: komentarz w pliku mówi, że są dobrane na oko na ekranie, jak piksele tekstury, więc `main` przelicza kolor na liniowy przed oświetleniem. Są dość jasne, bo noc robi oświetlenie, a nie kolor. Liczby się nie zmieniły w M7. Do M6 wchodziły do rachunku wprost, bez przeliczenia ([`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md), dziś zastąpiona przez [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md)).

```glsl
void main() {
    if (uViewMode == 1) {
        fragColor = vec4(srgbToLinear(GRASS_NORMAL * 0.5 + 0.5), 1.0);
        return;
    }
    if (uViewMode == 2) {
        fragColor = vec4(srgbToLinear(vec3(gBladeUv, 0.0)), 1.0);
        return;
    }
```

Widoki diagnostyczne, z tymi samymi numerami co w `textured.frag` (wartości `game::ViewMode`). Oba pokazują dane jako kolor, więc przechodzą przez `srgbToLinear`: przebieg składający zakoduje klatkę do sRGB i obie zamiany się zniosą, a na ekran trafią te same liczby co przed M7. Dla tych widoków `onRender` wyłącza też ekspozycję i krzywą mapowania tonów ([`post-process.md`](post-process.md), sekcja 2.9). `srgbToLinear` przyjmuje `vec3`, stąd `vec3(gBladeUv, 0.0)` w drugim widoku. W widoku normalnych trawa ma kolor `(0,5, 1, 0,5)`, jasnozielony: tak koduje się normalną prosto w górę, a teren pod nią ma prawie ten sam kolor. W widoku UV źdźbło pokazuje swoją współrzędną: czerwień rośnie w poprzek, zieleń ku czubkowi.

```glsl
    vec3 color = srgbToLinear(mix(ROOT_COLOR, TIP_COLOR, gBladeUv.y));

    vec3 light = vec3(1.0);
    if (uLit) {
        Lighting lighting = computeLighting(gWorldPosition, GRASS_NORMAL);
        float shadow = moonShadow(gWorldPosition, moonFacing(GRASS_NORMAL));
        float flashlightShade =
            flashlightShadow(gWorldPosition, flashlightFacing(GRASS_NORMAL, gWorldPosition));
        light = max(lighting.diffuse - lighting.moonDiffuse * shadow -
                        lighting.flashlightDiffuse * flashlightShade,
                    0.0);
    }
    fragColor = vec4(color * light, 1.0);
}
```

Gradient, światło i wynik. **Kolejność w pierwszej linii ma znaczenie:** najpierw mieszanie między liczbami sRGB, potem jedno przeliczenie wyniku. Tak gradient wygląda na ekranie tak, jak został dobrany: w połowie wysokości źdźbła mieszanka to `(0.28, 0.42, 0.165)`, liniowo około `(0,064, 0,147, 0,023)`. Odwrotna kolejność (przeliczyć oba końce, potem mieszać) dałaby w tym samym miejscu `(0,094, 0,200, 0,031)`, czyli wyraźnie jaśniejszy środek. Komentarz w pliku (poprawiony w czwartej części M7) mówi to dokładnie: wynik jest przeliczany raz, "like one texel of an sRGB texture", czyli jak jeden teksel tekstury, w której gradient byłby już namalowany. Dodaje też zastrzeżenie w nawiasie: filtrowana tekstura sRGB miesza **po** przeliczeniu, więc jej tony pośrednie wychodzą trochę inne. Poprzednia wersja komentarza ("the same as reading it from an sRGB texture") tego rozróżnienia nie robiła. Wynik `color * light` jest liniowy i trafia do bufora HDR bez obcinania. `computeLighting` zwraca strukturę z polami `diffuse` i `specular`. Pole `diffuse` zawiera już światło otoczenia i składnik Lamberta każdego światła z tłumieniem. Dla światła księżyca przy ustawieniach startowych (pitch -50 stopni) i normalnej w górę składnik Lamberta to `sin(50°) = 0,77`, na całej trawie tak samo.

**Linie w gałęzi `if (uLit)` (czwarta część M7, od piątej z latarką).** Do trzeciej części M7 była tu jedna: `light = computeLighting(gWorldPosition, GRASS_NORMAL).diffuse;`. Po czwartej części były trzy (księżyc), od piątej jest pięć, tak jak w kodzie wyżej:

| Linia | Znaczenie |
|---|---|
| `Lighting lighting = computeLighting(gWorldPosition, GRASS_NORMAL);` | wynik trafia do zmiennej, bo potrzebne są z niego trzy pola: `diffuse`, `moonDiffuse` i `flashlightDiffuse`. Struktura ma od czwartej części cztery pola, a od piątej sześć: `diffuse`, `specular`, `moonDiffuse`, `moonSpecular`, `flashlightDiffuse`, `flashlightSpecular`. Cztery ostatnie to udziały księżyca i latarki, **już zawarte** w dwóch pierwszych ([`../scene/lights.md`](../scene/lights.md), sekcja 4) |
| `float shadow = moonShadow(gWorldPosition, moonFacing(GRASS_NORMAL));` | część światła księżyca, której ten fragment nie dostaje: 0 poza cieniem, `uMoonShadowStrength` w środku cienia, wartości pośrednie na miękkim brzegu. Pierwszy argument to pozycja fragmentu źdźbła w świecie, której funkcja szuka w mapie cieni. Drugi to cosinus kąta między normalną a kierunkiem do księżyca, potrzebny do biasu: dla trawy liczony ze stałej `GRASS_NORMAL`, tej samej co do światła |
| `float flashlightShade = flashlightShadow(gWorldPosition, flashlightFacing(GRASS_NORMAL, gWorldPosition));` (piąta część M7) | to samo dla latarki, z jej własnej mapy cieni. `flashlightFacing` dostaje też pozycję fragmentu, bo kierunek do latarki zależy od miejsca |
| `light = max(lighting.diffuse - lighting.moonDiffuse * shadow - lighting.flashlightDiffuse * flashlightShade, 0.0);` | światło rozproszone bez zacienionej części księżyca i, osobno, bez zacienionej części latarki. Odejmowane są **tylko** te dwa udziały: światło otoczenia i kryształy zostają. `max` jest zabezpieczeniem przed wynikiem o ostatnią cyfrę poniżej zera. Pól `moonSpecular` i `flashlightSpecular` trawa nie potrzebuje, bo odbłysku nie używa wcale |

To te same kroki co w `lit.frag` ([`lighting-gouraud-phong.md`](lighting-gouraud-phong.md), sekcje 2.8 i 4.2), bez odbłysku. Komentarz w pliku nad tymi liniami mówi też, czego trawa nie robi: "It casts none itself: a blade is about as wide as one texel of the shadow map of the moon" (sekcja 2.7).

### 4.4 Strona C++: kto ustawia uniformy

| Uniform | Etap | Kto ustawia | Wartość |
|---|---|---|---|
| `uView`, `uProjection` | geometrii | `GrassRenderer::draw` | macierze klatki |
| `uTime` | geometrii | `GrassRenderer::draw` | `glfwGetTime()` z `NightMazeApp::drawGrass` |
| `uBladeHeight` | geometrii | `GrassRenderer::draw` | `GrassSettings::bladeHeight`, startowo 0,3 |
| `uWindStrength` | geometrii | `GrassRenderer::draw` | `GrassSettings::windStrength`, startowo 1 |
| `uLit` | fragmentów | `GrassRenderer::draw` | 0 w trybie `Unlit`, inaczej 1 |
| `uViewMode` | fragmentów | `GrassRenderer::draw` | wartość `game::ViewMode` |
| `uSpecularStrength`, `uShininess` | fragmentów (z `lighting.glsl`) | `GrassRenderer::draw` | 0 i 1 |
| `uSpecularModel` | fragmentów (z `lighting.glsl`) | nikt | zostaje 0. Nie ma znaczenia przy sile odbłysku 0 |
| blok `LightBlock` | fragmentów (z `lighting.glsl`) | `LightRig::upload`, raz na klatkę dla wszystkich programów | światła sceny |
| `uMoonShadowMap` | fragmentów (z `shadows.glsl`) | `game::setShadowUniforms`, wołane przez `NightMazeApp::setShadowUniformsOf` z `drawGrass` | 3 (`MOON_SHADOW_TEXTURE_UNIT`) |
| `uMoonShadowEnabled` | fragmentów (z `shadows.glsl`) | to samo | 1, gdy przebieg cieni wypełnił mapę w tej klatce (`m_moonShadowDrawn`), inaczej 0 |
| `uMoonShadowMatrix` | fragmentów (z `shadows.glsl`) | to samo | `m_moonLightSpace.matrix()` |
| `uMoonShadowConstantBias`, `uMoonShadowSlopeBias` | fragmentów (z `shadows.glsl`) | to samo | bias z ustawień (startowo 0,02 m i 0,12 m) podzielony przez głębokość pudełka światła |
| `uMoonShadowPcfRadius` | fragmentów (z `shadows.glsl`) | to samo | startowo 1 (jądro 3 x 3), 0 przy wyłączonym PCF |
| `uMoonShadowStrength` | fragmentów (z `shadows.glsl`) | to samo | startowo 1 |
| `uFlashlightShadowMap`, `uFlashlightShadowEnabled`, `uFlashlightShadowMatrix`, `uFlashlightShadowConstantBias`, `uFlashlightShadowSlopeBias`, `uFlashlightShadowPcfRadius`, `uFlashlightShadowStrength`, `uFlashlightShadowLightPosition` (od piątej części M7) | fragmentów (z `shadows.glsl`) | `game::setShadowUniforms` drugi raz, z `NightMazeApp::setShadowUniformsOf` | ósemka jak u księżyca, z jednostką 4 (`FLASHLIGHT_SHADOW_TEXTURE_UNIT`), `m_flashlightShadowDrawn`, `m_flashlightLightSpace` i pozycją światła |

Nazwy są w [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp): cztery nowe stałe (`GRASS_TIME_UNIFORM`, `GRASS_BLADE_HEIGHT_UNIFORM`, `GRASS_WIND_STRENGTH_UNIFORM`, `GRASS_LIT_UNIFORM`), reszta wspólna z innymi programami. Siedem nazw uniformów mapy cieni księżyca stoi tam w jednej stałej strukturalnej `MOON_SHADOW_UNIFORMS` (typ `ShadowUniformNames`), obok stałej `MOON_SHADOW_TEXTURE_UNIT`, a osiem nazw mapy latarki w `FLASHLIGHT_SHADOW_UNIFORMS` (od piątej części M7, jednostka `FLASHLIGHT_SHADOW_TEXTURE_UNIT`): trawa dzieli je z programami `lit`, `gouraud` i `reflect`.

Uwaga do dwóch uniformów odbłysku: `grass.frag` nie używa pola `specular`, więc kompilator ma prawo usunąć cały rachunek odbłysku razem z tymi uniformami. Wtedy `glGetUniformLocation` zwraca dla nich -1, a `glUniform1f` z położeniem -1 jest po cichu pomijane ([`../gfx/uniforms.md`](../gfx/uniforms.md)). Ustawianie ich jest poprawne w obu przypadkach.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`assets/shaders/grass.vert`](../../../assets/shaders/grass.vert), [`grass.geom`](../../../assets/shaders/grass.geom), [`grass.frag`](../../../assets/shaders/grass.frag) | trzy shadery (sekcja 4) |
| [`src/game/Grass.hpp`](../../../src/game/Grass.hpp), [`.cpp`](../../../src/game/Grass.cpp) | stałe, `GrassTuft`, `GrassSettings`, `placeGrass` |
| [`src/game/GrassRenderer.hpp`](../../../src/game/GrassRenderer.hpp), [`.cpp`](../../../src/game/GrassRenderer.cpp) | siatka punktów na karcie, `upload`, `draw` |
| [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp), [`.cpp`](../../../src/gfx/Shader.cpp) | opcjonalny trzeci plik programu ([`../gfx/shader-class.md`](../gfx/shader-class.md)) |
| [`src/gfx/Mesh.hpp`](../../../src/gfx/Mesh.hpp) | komentarze: `GL_POINTS` jako trzeci rodzaj prymitywu. Kod klasy się nie zmienił |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | pola `m_grassShader`, `m_grassRenderer`, `m_grassSettings`, funkcje `plantGrass` i `drawGrass` |
| [`assets/shaders/common/shadows.glsl`](../../../assets/shaders/common/shadows.glsl), [`src/game/ShadowMap.hpp`](../../../src/game/ShadowMap.hpp), [`.cpp`](../../../src/game/ShadowMap.cpp) (czwarta część M7) | plik dołączany przez `grass.frag` z funkcją `moonShadow` i funkcja `game::setShadowUniforms`, którą dwa razy woła (przez `setShadowUniformsOf`) `drawGrass`. Opisuje je [`shadows.md`](shadows.md) |
| [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp) | cztery nazwy uniformów trawy |
| [`src/debug/categories/WorldCategory.hpp`](../../../src/debug/categories/WorldCategory.hpp), [`.cpp`](../../../src/debug/categories/WorldCategory.cpp) | zakładka World / Terrain and grass (sekcja 6) |
| [`src/debug/categories/DiagnosticsCategory.cpp`](../../../src/debug/categories/DiagnosticsCategory.cpp) | linia stanu programu pokazuje trzy pliki |
| [`tests/GrassTests.cpp`](../../../tests/GrassTests.cpp) | 9 przypadków (sekcja 5.7) |

W [`CMakeLists.txt`](../../../CMakeLists.txt) `Grass.*` należy do biblioteki `game_logic` (miejsca kępek to dane i matematyka bez OpenGL, więc mają testy), a `GrassRenderer.*` i `GrassPanel.*` do programu `night_maze`.

### 5.2 Stałe, `GrassTuft`, `GrassSettings`

```cpp
constexpr float GRASS_END_CLEARANCE = 0.3F;
constexpr float GRASS_WALL_GAP = 0.06F;
constexpr float GRASS_STRIP_WIDTH = 0.22F;
constexpr float GRASS_HILL_TUFTS_PER_SQUARE_METRE = 0.35F;
constexpr float GRASS_HILL_CLEARANCE = 0.6F;
constexpr float DEFAULT_GRASS_DENSITY = 2.5F;
constexpr float MAX_GRASS_DENSITY = 8.0F;
```

Wszystkie omówione w sekcji 2.8.

```cpp
struct GrassTuft {
    glm::vec3 position{0.0F};
    float random = 0.0F;
};
```

Cała kępka po stronie procesora: cztery liczby. Źdźbeł tu nie ma i nie będzie.

```cpp
struct GrassSettings {
    bool enabled = true;
    float density = DEFAULT_GRASS_DENSITY;
    float bladeHeight = 0.3F;
    float windStrength = 1.0F;
    bool replant = false;
};
```

Pola dzielą się na dwie grupy, i ten podział jest sednem tematu:

- `bladeHeight` i `windStrength` to **uniformy**. Zmieniają kształt, który shader geometrii buduje w każdej klatce, więc działają natychmiast i nic nie trzeba wysyłać od nowa,
- `density` zmienia **liczbę punktów w buforze**, więc wymaga policzenia miejsc od nowa i nowego bufora. Panel ustawia flagę `replant`, a gra sadzi trawę na początku następnej klatki, tym samym wzorcem co `MazeSettings::regenerate` i `TerrainSettings::rebuild`.

### 5.3 `placeGrass`

```cpp
std::vector<GrassTuft> placeGrass(const MazeWorld& world, float density) {
    std::vector<GrassTuft> tufts;
    if (density <= 0.0F) {
        return tufts;
    }

    std::mt19937 generator(world.seed + GRASS_SEED_OFFSET);

    const int tuftsPerSide = static_cast<int>(std::lround(density * WALL_LENGTH));

    for (const WallSegment& wall : world.walls) {
        plantAlongWall(wall, world.terrain, tuftsPerSide, generator, tufts);
    }
    plantOnHills(world, density, generator, tufts);
    return tufts;
}
```

Gęstość 0 albo ujemna daje pustą listę. `std::lround` zaokrągla do najbliższej liczby całkowitej: przy gęstości 0,2 wychodzi `lround(0,4) = 0` kępek przy ścianach, ale wzgórza nadal dostają swoje.

```cpp
float randomUnit(std::mt19937& generator) {
    return static_cast<float>(randomBelow(generator, RANDOM_STEPS)) /
           static_cast<float>(RANDOM_STEPS);
}
```

`RANDOM_STEPS = 4096`: liczba od 0 do 4095 podzielona przez 4096, czyli od 0 do tuż poniżej 1, w krokach co 1/4096. Dlaczego nie rozkład z biblioteki standardowej: sekcja 2.9.

```cpp
GrassTuft tuftAt(const Terrain& terrain, float x, float z, std::mt19937& generator) {
    return {.position = {x, terrain.heightAt(x, z), z}, .random = randomUnit(generator)};
}
```

Kępka w miejscu `(x, z)`: wysokość z terenu i własna liczba losowa. To trzecia liczba, jaką kępka bierze z generatora.

```cpp
void plantAlongWall(const WallSegment& wall, const Terrain& terrain, int tuftsPerSide,
                    std::mt19937& generator, std::vector<GrassTuft>& tufts) {
    constexpr float REACH = WALL_LENGTH / 2.0F - GRASS_END_CLEARANCE;
    constexpr float STRIP_START = WALL_COLLISION_THICKNESS / 2.0F + GRASS_WALL_GAP;

    for (const float side : WALL_SIDES) {
        for (int i = 0; i < tuftsPerSide; ++i) {
            const float along = glm::mix(-REACH, REACH, randomUnit(generator));
            const float across = side * (STRIP_START + GRASS_STRIP_WIDTH * randomUnit(generator));

            const bool alongX = wall.axis == WallAxis::AlongX;
            const float x = wall.position.x + (alongX ? along : across);
            const float z = wall.position.z + (alongX ? across : along);
            tufts.push_back(tuftAt(terrain, x, z, generator));
        }
    }
}
```

`WALL_SIDES` to `{-1, 1}`: dwie strony ściany. `glm::mix(-REACH, REACH, t)` rozciąga liczbę od 0 do 1 na zakres od -0,7 do 0,7. `side` jako mnożnik przenosi pas na jedną albo drugą stronę. Z pozycji ściany brane są tylko `x` i `z`: jej `y` to najniższy grunt pod obrysem, a kępka ma stać na gruncie w swoim miejscu.

```cpp
void plantOnHills(const MazeWorld& world, float density, std::mt19937& generator,
                  std::vector<GrassTuft>& tufts) {
    const Terrain& terrain = world.terrain;
    const float landWidth = terrain.maxX() - terrain.minX();
    const float landDepth = terrain.maxZ() - terrain.minZ();

    const float tuftsPerSquareMetre =
        GRASS_HILL_TUFTS_PER_SQUARE_METRE * density / DEFAULT_GRASS_DENSITY;
    const int tries = static_cast<int>(std::lround(landWidth * landDepth * tuftsPerSquareMetre));

    for (int i = 0; i < tries; ++i) {
        const float x = terrain.minX() + landWidth * randomUnit(generator);
        const float z = terrain.minZ() + landDepth * randomUnit(generator);
        if (distanceOutsideMaze(x, z, world.maze.width(), world.maze.height()) <
            GRASS_HILL_CLEARANCE) {
            continue;
        }
        tufts.push_back(tuftAt(terrain, x, z, generator));
    }
}
```

`distanceOutsideMaze` to ta sama funkcja, której teren używa do reliefu ([`terrain.md`](terrain.md), sekcja 5.5). Odrzucona próba zużywa dwie liczby z generatora, przyjęta trzy.

### 5.4 `NightMazeApp`: sadzenie i rysowanie

```cpp
void NightMazeApp::plantGrass() {
    m_grassSettings.density = std::clamp(m_grassSettings.density, 0.0F, MAX_GRASS_DENSITY);
    const std::vector<GrassTuft> tufts = placeGrass(m_mazeWorld, m_grassSettings.density);
    m_grassRenderer.upload(tufts);
}
```

Kępki są liczone, kopiowane na kartę i zapominane: `NightMazeApp` nie trzyma ich listy, tylko `GrassRenderer` pamięta, ile ich jest. `plantGrass` jest wołane z `uploadGround` (nowy labirynt, nowa skala terenu) i bezpośrednio z `onRender`, gdy panel ustawił `replant`.

```cpp
void NightMazeApp::drawGrass(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_grassSettings.enabled) {
        return;
    }

    const bool lit = m_lighting.mode != LightingMode::Unlit;
    const auto windSeconds = static_cast<float>(glfwGetTime());

    if (m_grassShader.isValid()) {
        m_grassShader.use();
        setShadowUniformsOf(m_grassShader);
    }
    m_grassRenderer.draw(m_grassShader, view, projection, m_grassSettings, windSeconds, lit,
                         m_viewMode);
}
```

Blok `if (m_grassShader.isValid())` doszedł w czwartej części M7 (listing, jak pozostałe w tej sekcji, jest bez komentarzy z pliku). Komentarz nad nim w kodzie tłumaczy obie linie: trawa leży w cieniu księżyca jak grunt, więc jej program też dostaje uniformy mapy cieni, a "a uniform is written into the program in use, hence use() here: GrassRenderer::draw calls it again, which changes nothing". To pierwszy przypadek, w którym uniformy programu trawy ustawia ktoś poza `GrassRenderer` (sekcja 5.5): mapa cieni należy do aplikacji, a nie do trawy, więc `GrassRenderer` nic o niej nie wie. Warunek `isValid()` jest potrzebny, bo `use()` na programie, który się nie wczytał, nie ma sensu. `GrassRenderer::draw` sprawdza to samo jeszcze raz u siebie. `setShadowUniformsOf` to ta sama funkcja, którą woła `drawLitMaze`: dwa razy `setShadowUniforms`, raz z nazwami uniformów, jednostką tekstur 3, flagą "przebieg cieni wypełnił mapę w tej klatce", ustawieniami i macierzą księżyca z tej klatki, drugi raz to samo dla latarki (jednostka 4). W czwartej części M7 w tym miejscu stało jedno wywołanie `setShadowUniforms` z samym księżycem.

Pole `Enabled` wyłącza samo rysowanie: punkty zostają na karcie, więc włączenie z powrotem nic nie kosztuje. `drawGrass` jest wołane w `onRender` po `drawMaze` i przed liniami kolizji i niebem.

Program powstaje w liście inicjalizacyjnej:

```cpp
      m_grassShader(core::assetPath(GRASS_VERTEX_SHADER_FILE),
                    core::assetPath(GRASS_FRAGMENT_SHADER_FILE),
                    core::assetPath(GRASS_GEOMETRY_SHADER_FILE)),
```

Plik shadera geometrii jest **trzecim** argumentem, chociaż jego etap działa jako drugi: to argument opcjonalny, więc musi stać na końcu. W konstruktorze `m_lightRig.connect(m_grassShader)` łączy blok świateł programu z buforem uniformów.

### 5.5 `GrassRenderer`

```cpp
GrassRenderer::GrassRenderer()
    : m_points(std::span<const gfx::Vertex>{}, std::span<const std::uint32_t>{}, GL_POINTS) {}
```

Pusta siatka punktów. Trzeci argument konstruktora `gfx::Mesh` to rodzaj prymitywu.

```cpp
void GrassRenderer::upload(std::span<const GrassTuft> tufts) {
    std::vector<gfx::Vertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve(tufts.size());
    indices.reserve(tufts.size());
    for (const GrassTuft& tuft : tufts) {
        indices.push_back(static_cast<std::uint32_t>(vertices.size()));
        vertices.push_back({.position = tuft.position, .uv = {tuft.random, 0.0F}});
    }

    m_points = gfx::Mesh(vertices, indices, GL_POINTS);
    m_tuftCount = tufts.size();
}
```

Jeden `gfx::Vertex` na kępkę. Liczba losowa jedzie w `uv.x`, bo punkt nie ma innego użytku dla współrzędnej tekstury, a dzięki temu trawa nie potrzebuje własnego formatu wierzchołka ani własnej klasy siatki. Indeks jest dopisywany **przed** wierzchołkiem, więc `vertices.size()` jest wtedy numerem wierzchołka, który zaraz powstanie. `.position = ...` to inicjalizacja z nazwami pól (C++20): pola niewymienione, normalna i styczna, dostają wartości domyślne.

```cpp
void GrassRenderer::draw(const gfx::Shader& shader, const glm::mat4& view,
                         const glm::mat4& projection, const GrassSettings& settings,
                         float timeSeconds, bool lit, ViewMode viewMode) const {
    if (m_tuftCount == 0 || !shader.isValid()) {
        return;
    }

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    shader.setFloat(GRASS_TIME_UNIFORM, timeSeconds);
    shader.setFloat(GRASS_BLADE_HEIGHT_UNIFORM, settings.bladeHeight);
    shader.setFloat(GRASS_WIND_STRENGTH_UNIFORM, settings.windStrength);
    shader.setInt(GRASS_LIT_UNIFORM, lit ? 1 : 0);
    shader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(viewMode));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, NO_SPECULAR_STRENGTH);
    shader.setFloat(SHININESS_UNIFORM, PLAIN_SHININESS);
```

Inaczej niż `TerrainRenderer` i `MazeRenderer`, ta klasa **sama wybiera program i ustawia wszystkie uniformy, które należą do trawy** (od czwartej części M7 siedem uniformów mapy cieni ustawia przed nią `NightMazeApp::drawGrass`, sekcja 5.4). Tamte dostają program już przygotowany, bo dzielą go z innymi klasami. Program trawy ma jednego użytkownika. Gdy programu nie ma (pierwsze wczytanie się nie powiodło), funkcja po prostu wraca: błąd został wypisany raz, przy tworzeniu shadera.

```cpp
    GLboolean cullingWasOn = GL_FALSE;
    GL_CHECK(cullingWasOn = glIsEnabled(GL_CULL_FACE));
    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glDisable(GL_CULL_FACE));
    }

    m_points.draw();

    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glEnable(GL_CULL_FACE));
    }
}
```

Zapamiętaj, zmień, narysuj, przywróć: ten sam wzorzec co stan głębi w `Skybox::draw` ([`skybox.md`](skybox.md)). `m_points.draw()` to jedno `glDrawElements(GL_POINTS, ...)`.

### 5.6 Błąd w shaderze geometrii i przeładowanie

Trzeci plik przechodzi przez ten sam loader co dwa pozostałe, więc ma to samo zachowanie: `#include`, nazwy plików w błędach kompilacji, przeładowanie z zachowaniem starego programu. Przycisk `Reload shaders` czyta dla programu `grass` trzy pliki, a linia stanu w zakładce Diagnostics / Frame and shaders pokazuje je w kolejności etapów: `grass.vert + grass.geom + grass.frag: OK`.

Zgłoszone z Windowsa (nie powtarzane przeze mnie): po celowym zepsuciu `grass.geom` błąd sterownika wskazuje `grass.geom(84)`, czyli plik i linię, a gra działa dalej. Z kodu wynika reszta: po nieudanym przeładowaniu `Shader::reload` zostawia poprzedni program i trawa rysuje się nim dalej, a po nieudanym pierwszym wczytaniu programu nie ma i `GrassRenderer::draw` po prostu wraca, więc trawy nie widać. Oba przypadki są na liście testów ręcznych. Szczegóły: [`../gfx/shader-hot-reload.md`](../gfx/shader-hot-reload.md).

### 5.7 Testy

[`tests/GrassTests.cpp`](../../../tests/GrassTests.cpp): 9 przypadków. Sprawdzają `placeGrass`, czyli część na procesorze. Świat większości testów to labirynt 6 x 5 z ziarnem 21 na nierównym gruncie (`roughHeightmap`, 8 x 8 wartości ze wzoru).

| Przypadek testowy | Co sprawdza |
|---|---|
| `the grass constants and the default settings` | wartości stałych i wartości domyślne `GrassSettings` |
| `a density of 0 or less plants nothing` | pusta lista dla gęstości 0 i ujemnej |
| `the same world and density give the same tufts` | powtarzalność: dwa wywołania dają tę samą listę |
| `another seed grows the grass in other places` | labirynty z ziarnem 21 i 22 mają pierwszą kępkę w innym miejscu |
| `the tufts of the walls come first: density per metre, on both sides` | pierwsze `ściany * 10` kępek należy do ścian w ich kolejności, każda leży w pasie swojej ściany (wzdłuż najwyżej 0,7 m od środka, w poprzek od 0,21 do 0,43 m), pierwsze pięć po jednej stronie, następne pięć po drugiej |
| `twice the density plants twice the tufts along the walls` | gęstość 2 daje 8 kępek na ścianę, gęstość 4 daje 16, a cała liczba rośnie mniej więcej dwukrotnie |
| `no tuft stands inside a wall, a pillar or the gate` | przy największej gęstości żadna kępka nie leży w obrysie pudełka ściany ani słupka powiększonym o `FOOTPRINT_MARGIN`, ani w obrysie bramy |
| `every tuft stands on the ground and has a random number from 0 to 1` | `position.y` równa się `heightAt`, kępka leży na ziemi terenu, liczba losowa jest w zakresie i naprawdę go wypełnia (najmniejsza poniżej 0,05, największa powyżej 0,95) |
| `the hills get a sparse scatter that keeps away from the maze` | kępki po ścianach leżą co najmniej 0,6 m od labiryntu, a jest ich tyle, ile wynika z powierzchni i gęstości, z dokładnością do jednej piątej |

Czego testy **nie** sprawdzają:

- **żadnego shadera.** Kształt źdźbła, wiatr, światło, cień księżyca na trawie, `max_vertices`, zgodność wejść i wyjść etapów: tego nie da się sprawdzić bez karty graficznej. Jedynym sprawdzeniem jest kompilacja i linkowanie przy starcie gry oraz obraz,
- klasy `GrassRenderer` i tego, że liczba losowa trafia do `uv.x`,
- tego, że liczby losowe są takie same na macOS. Wynika to z konstrukcji (`mt19937`, `randomBelow`), ale nikt nie porównał wyniku na dwóch systemach. Liczba 1843 dla labiryntu startowego jest dobrym punktem do porównania,
- zgodności liczby `BLADES_PER_TUFT = 3` w panelu z `BLADE_COUNT` w shaderze.

## 6. Okno debugowania (dawniej panel ImGui)

**Stan na 2026-10-06.** Panel Grass zastąpiło okno debugowania ([`../debug-ui.md`](../debug-ui.md)). Kontrolki trawy są w kategorii **World**, w zakładce **Terrain and grass**, w karcie **Grass** ([`src/debug/categories/WorldCategory.cpp`](../../../src/debug/categories/WorldCategory.cpp)): `Grass` (dawniej `Enabled`), `Density` (zmiana ustawia flagę `replant`), `Blade height`, `Wind strength` i odczyt `Tufts`. `drawGrassPanel`, `GRASS_PLACEMENT` i rzędy zwiniętych pasków nie istnieją; kod i opis układu niżej pochodzą z panelu sprzed zmiany i są zachowane jako historia.

Dawny panel Grass był dziesiątym panelem (od pierwszej części M7 paneli było jedenaście: doszedł Framebuffers, a od czwartej części M7 jest ich dwanaście: doszedł Shadows). Startował zwinięty do paska tytułu, w drugim rzędzie pasków przy górnej krawędzi okna, pod panelem Gameplay. Od M7 pod nim, w trzecim rzędzie, stał pasek panelu Framebuffers, a od czwartej części M7 w czwartym rzędzie pasek panelu Shadows (`FOLDED_ROW_COUNT` równe 4 w tamtej części, sześć paneli startuje zwiniętych; dziś `FOLDED_ROW_COUNT` jest równe 5, bo od M8, części 1, w piątym rzędzie stał pasek panelu Environment, a zwiniętych paneli startowało siedem): rozwinięty panel Grass (dziś zakładka World / Terrain and grass) zakrywał swoją część obu tych pasków ([`../debug-ui.md`](../debug-ui.md)).

```cpp
void drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount) {
    placePanelOnFirstUse(GRASS_PLACEMENT);
    if (ImGui::Begin("Grass")) {
        ImGui::Checkbox("Enabled", &settings.enabled);

        if (ImGui::SliderFloat("Density", &settings.density, MIN_DENSITY, game::MAX_GRASS_DENSITY,
                               "%.1f per m", ImGuiSliderFlags_AlwaysClamp)) {
            settings.replant = true;
        }
        ...
        ImGui::SliderFloat("Blade height", &settings.bladeHeight, MIN_BLADE_HEIGHT,
                           MAX_BLADE_HEIGHT, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Wind strength", &settings.windStrength, MIN_WIND_STRENGTH,
                           MAX_WIND_STRENGTH, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();
        const int tufts = static_cast<int>(tuftCount);
        ImGui::Text("Tufts: %d (%d blades)", tufts, tufts * BLADES_PER_TUFT);
    }
    ImGui::End();
}
```

| Kontrolka | Zakres | Co robi | Co się dzieje na karcie |
|---|---|---|---|
| pole `Enabled` | startuje zaznaczone | włącza i wyłącza rysowanie trawy | wywołanie rysujące jest pomijane, bufor zostaje |
| suwak `Density` | od 0 do 8, startuje na 2,5 | kępki na metr ściany, po każdej stronie. Rozsiew na wzgórzach idzie proporcjonalnie | miejsca liczone od nowa, nowy bufor punktów |
| suwak `Blade height` | od 0,05 do 0,8 m, startuje na 0,3 | wysokość najwyższych źdźbeł | zmienia się jeden uniform |
| suwak `Wind strength` | od 0 do 3, startuje na 1 | jak daleko wiatr wychyla czubki. 0 to bezruch | zmienia się jeden uniform |
| `Tufts: N (M blades)` | tylko odczyt | liczba kępek na karcie i trzy razy tyle źdźbeł. Przy starcie `Tufts: 1843 (5529 blades)` | nic |

Ostatnia kolumna to najlepszy materiał na obronę: dwa suwaki działają bez dotykania danych, jeden wymaga nowego bufora, i z kodu widać dlaczego.

Liczba źdźbeł w ostatniej linii jest liczona ze stałej `BLADES_PER_TUFT = 3` w `GrassPanel.cpp`, która jest kopią `BLADE_COUNT` z `grass.geom`. C++ nie może przeczytać stałej z pliku GLSL. Komentarz przy stałej mówi, że służy tylko do wyświetlenia liczby, więc błędna wartość nie zmienia niczego, co jest rysowane.

Program trawy widać też w zakładce Diagnostics / Frame and shaders: szósta linia stanu z czternastu, z trzema plikami.

Cień księżyca na trawie nie ma kontrolki w zakładce World / Terrain and grass. Włącza go i stroi zakładkę Light / Shadows (od czwartej części M7, [`shadows.md`](shadows.md), sekcja 6): pole `Shadows`, lista `Resolution`, suwaki biasu, filtr i `Strength` działają na trawę tak samo jak na grunt, bo `drawGrass` wysyła do programu trawy te same ustawienia.

### 6.1 Scenariusz pokazu na obronie

**Kroków nikt jeszcze nie wykonał ręcznie.** Lista do odhaczenia jest w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 16.

1. **Przełącznik.** Staję w korytarzu, patrzę na podstawę ściany. Rozwijam zakładkę World / Terrain and grass i odznaczam `Enabled`: trawa znika. Zaznaczam z powrotem. Mówię: to jedno wywołanie rysujące, a w buforze jest 1843 punktów, nie źdźbeł.
2. **Punkt staje się kępką.** Pokazuję w `grass.geom` linie `layout(points) in;` i `layout(triangle_strip, max_vertices = 15) out;`. Mówię: shader dostaje jeden punkt i wypisuje trzy paski po pięć wierzchołków, w każdej klatce od nowa.
3. **Uniform a bufor.** Przeciągam `Blade height` i `Wind strength`: trawa rośnie i kołysze się mocniej, licznik `Tufts` stoi. Przeciągam `Density`: licznik się zmienia. Mówię: pierwsze dwa to uniformy shadera geometrii, trzeci zmienia liczbę punktów, więc procesor liczy miejsca od nowa.
4. **Wiatr.** Ustawiam `Wind strength` na 3 i patrzę wzdłuż długiej ściany: wychylenia wędrują po trawie falą. Ustawiam 0: trawa stoi. Mówię: sinus z czasu, z liczby losowej kępki i z położenia wzdłuż kierunku wiatru. Żadne dane nie są wysyłane.
5. **Światło.** Świecę latarką na trawę, wyłączam ją klawiszem F, podchodzę do kryształu. Mówię: `grass.frag` dołącza ten sam plik `lighting.glsl` co ściany i czyta ten sam blok świateł, z normalną prosto w górę.
6. **Tryby cieniowania.** W kategorii Render przełączam `Lighting` przez cztery tryby. W `Unlit` trawa ma pełną jasność. W `Gouraud` ściany są cieniowane na wierzchołek, a trawa nadal na fragment. Mówię: kępka nie ma w buforze wierzchołków, w których dałoby się policzyć światło.
7. **Powtarzalność.** W zakładce World / Maze generuję labirynt z innym ziarnem, potem wracam do ziarna 1: licznik znów pokazuje 1843. Mówię: ziarno trawy to ziarno labiryntu plus stała, a liczby losowe pochodzą z własnej funkcji, takiej samej na każdym systemie.
8. **Na żywo.** W `grass.geom` zmieniam `BLADE_COUNT` na 2 (albo kolor w `grass.frag`), kopiuję assety (na Windowsie `cmake --build --preset debug --target copy_assets`) i klikam `Reload shaders`. Potem wpisuję błąd składni i klikam jeszcze raz: zakładka Diagnostics / Frame and shaders pokazuje błąd z nazwą `grass.geom` i numerem linii, a trawa rysuje się dalej starym programem. Wycofuję zmiany.
9. **Trawa w cieniu (czwarta część M7).** Gaszę latarkę i staję przy ścianie, której cień pada na grunt z trawą. Kępki w cieniu są ciemniejsze, tak samo jak ziemia pod nimi, a kępki w świetle księżyca jaśniejsze. W zakładce Light / Shadows odznaczam `Shadows`: różnica znika. Mówię: `grass.frag` dołącza ten sam plik `common/shadows.glsl` co ściany i odejmuje zacieniony udział księżyca. Pokazuję, że sama trawa cienia nie rzuca, i mówię dlaczego: źdźbło ma 4 cm u korzenia, teksel mapy około 3 cm, a źdźbła ruszają się z wiatrem.

## 7. Pułapki

1. **Tryb rysowania nie pasuje do wejścia.** Program z `layout(points) in` narysowany siatką `GL_TRIANGLES` daje `GL_INVALID_OPERATION` i pusty obraz. Siatka trawy musi być utworzona z `GL_POINTS`, w obu miejscach: w konstruktorze i w `upload`.
2. **Za małe `max_vertices`.** Zwiększenie `BLADE_COUNT` bez zmiany `TUFT_MAX_VERTICES` nie daje błędu kompilacji. Wierzchołki ponad limit są porzucane i ostatnie źdźbła po cichu znikają albo są ucięte.
3. **`const int` w `layout`.** `layout(max_vertices = BLADE_COUNT * VERTICES_PER_BLADE)` jest w GLSL 4.10 niepoprawne. Sterownik, który trzyma się specyfikacji, odrzuci taką linię, a pobłażliwy może ją przyjąć, i wtedy błąd wyjdzie dopiero na innej maszynie. Stąd `#define`.
4. **Brak `EndPrimitive()`.** Trzy źdźbła zlewają się w jeden pasek: między czubkiem jednego a korzeniem następnego pojawiają się długie, cienkie trójkąty.
5. **Niezapisane wyjście przed `EmitVertex()`.** Po `EmitVertex()` wartości wyjść są niezdefiniowane. Ustawienie `gBladeUv` raz przed pętlą zamiast przy każdym wierzchołku dałoby śmieci w kolorze na części kart, a poprawny obraz na innych.
6. **Macierze w złym etapie.** Przeniesienie `uProjection * uView` do `grass.vert` wygląda niewinnie, ale wtedy shader geometrii dostaje pozycję w przestrzeni przycinania i "w górę" przestaje być `(0, 1, 0)`: źdźbła rosłyby w stronę zależną od kamery.
7. **Wejścia shadera fragmentów.** `grass.frag` nie może zadeklarować `in float vRandom`: przy shaderze geometrii w programie jego wejścia pochodzą z `grass.geom`. Linker zgłosi brak pasującego wyjścia.
8. **`1 / 3` w GLSL.** Dzielenie dwóch liczb całkowitych daje liczbę całkowitą. Bez `float(blade) / float(BLADE_COUNT)` wszystkie trzy źdźbła kępki leżałyby jedno na drugim.
9. **Rozkład z biblioteki standardowej.** `std::uniform_real_distribution` w `randomUnit` działałby i przeszedłby wszystkie testy na jednym systemie, a dałby inną trawę na drugim.
10. **Kolejność losowania.** Zamiana kolejności linii `along` i `across` albo przejście ścian w innym porządku zmienia każdą kępkę. Test powtarzalności tego nie wykryje (dwa wywołania nadal się zgadzają), a test pasa wykryje tylko część takich zmian.
11. **Dwie kopie liczby 3.** `BLADE_COUNT` w shaderze i `BLADES_PER_TUFT` w panelu. Zmiana pierwszej bez drugiej daje błędną liczbę źdźbeł w panelu.
12. **Liczba 15 trzeci raz.** `TUFT_MAX_VERTICES` musi być iloczynem `BLADE_COUNT` i `VERTICES_PER_BLADE`. Pilnuje tego tylko komentarz.
13. **Trawa przechodzi przez gracza i ściany.** Kępki nie mają kolizji. Źdźbło pochylone w stronę ściany może wejść w nią czubkiem: pas zaczyna się 21 cm od osi ściany, a czubek odchyla się do 40 procent wysokości plus wiatr. Przy wysokości 0,8 m i wietrze 3 jest to nieuniknione.
14. **Wiatr nie zna ścian.** Fala idzie przez cały świat w jednym kierunku, także w zamkniętych zaułkach.
15. **Pełne oświetlenie na każdy fragment.** `computeLighting` przechodzi przez wszystkie światła (do 16 punktowych) dla każdego piksela trawy. Przy dużej gęstości i kamerze tuż przy ziemi to jest najdroższa część trawy.
16. **Trawa przyjmuje cień księżyca (a od piątej części M7 także latarki) i sama żadnego nie rzuca.** Do trzeciej części M7 cieni w grze nie było wcale i trawa za ścianą była oświetlona przez wszystko. Od czwartej części M7 cień księżyca na trawie jest (sekcja 2.7). Światła kryształów cieni nie rzucają, więc ich światło dociera do trawy także przez ścianę. Od piątej części M7 (2026-10-06) `grass.frag` odejmuje też udział latarki w jej własnym cieniu (`flashlightShadow` z `flashlightFacing(GRASS_NORMAL, ...)`), a trawa nie jest rysowana także do mapy latarki. Brak cienia źdźbeł na ziemi to decyzja, nie błąd ([`../../decisions/grass-casts-no-shadow.md`](../../decisions/grass-casts-no-shadow.md)).
17. **Stary `imgui.ini`.** Jak przy każdym nowym panelu: najprościej usunąć plik przed pokazem.
18. **macOS, niesprawdzone.** Shadery geometrii należą do OpenGL 4.1 Core, więc sterownik Apple powinien je przyjąć. Nikt tego nie sprawdził dla tych trzech plików, a `#define` wewnątrz `layout` i format błędów dla pliku `.geom` są tym, co warto obejrzeć najpierw.
19. **Kolory gradientu to liczby sRGB.** Od M7 `main` przelicza je przez `srgbToLinear`. Kolor wpisany do shadera liczbami i użyty bez przeliczenia zostałby na końcu klatki zakodowany, chociaż nigdy nie był zdekodowany: trawa wyszłaby wyblakła i za jasna. Przeliczenie dwa razy dałoby trawę prawie czarną.
20. **Uniformy cienia nieustawione w programie trawy.** `lit`, `gouraud` i `grass` mają osobne kopie uniformów z `common/shadows.glsl`. Bez wywołania `setShadowUniformsOf` w `drawGrass` samplery `uMoonShadowMap` i `uFlashlightShadowMap` programu trawy zostałby na jednostce 0, a po `Reload shaders` wróciłby tam na pewno. Program trawy nie ma innego samplera, więc nie byłoby błędu OpenGL, tylko zły obraz: trawa czytałaby jako mapę cieni to, co akurat leży na jednostce 0 (zwykłą teksturę koloru, dla której wynik odczytu z porównaniem jest niezdefiniowany). Dlatego uniformy są ustawiane w każdej klatce, także przy wyłączonych cieniach.
21. **`use()` przed `setShadowUniforms`.** Uniform trafia do programu bieżącego. Bez `m_grassShader.use()` w `drawGrass` kilkanaście wartości (siedem dla księżyca i osiem dla latarki) poszłoby do programu, którym przed chwilą rysowano labirynt.

## 8. Ćwiczenia

Ćwiczenia od 1 do 5 są na kartce, pozostałe w działającej grze. Po zmianie pliku shadera na Windowsie: `cmake --build --preset debug --target copy_assets`, potem `Reload shaders`. Po ćwiczeniu wycofaj zmianę.

1. **Ile trójkątów.** Ile wierzchołków i trójkątów wypisuje shader dla jednej kępki? Ile dla całej trawy przy starcie? Ile wierzchołków jest w buforze? (Odpowiedź: 15 i 9. `1843 * 15 = 27645` i `1843 * 9 = 16587`. W buforze 1843.)
2. **Wierzchołki źdźbła.** Dla `r = 0`, `b = 0`, `uBladeHeight = 0,5`, bez wiatru policz `leanDirection`, `sideDirection`, wysokość i pozycję czubka względem kępki. (Odpowiedź: `(1, 0, 0)`, `(0, 0, 1)`, `fract(0) = 0`, więc `0,5 * 0,6 = 0,3 m`. Czubek: `(0,025 + 0,12, 0,3, 0) = (0,145, 0,3, 0)`.)
3. **Kępki przy ścianach.** Ile kępek dostaje jedna ściana przy gęstości 1,2? A przy 0,2? (Odpowiedź: `lround(2,4) = 2` na stronę, czyli 4. `lround(0,4) = 0`: przy ścianach nic, zostają wzgórza.)
4. **Wiatr.** Dwie kępki z tą samą liczbą losową stoją 3,5 m od siebie wzdłuż kierunku wiatru. O jaką część okresu różnią się ich wahnięcia? (Odpowiedź: `3,5 * 0,9 = 3,15` radiana, prawie dokładnie pół okresu: kołyszą się w przeciwne strony.)
5. **Granice.** O ile można zwiększyć `BLADE_COUNT`, zanim `max_vertices` przekroczy gwarantowane minimum 256? A zanim przekroczona zostanie granica składowych 1024? (Odpowiedź: `256 / 5 = 51` źdźbeł. Składowe: `1024 / 9 = 113` wierzchołków, czyli 22 źdźbła. Wcześniej trafia się w drugą granicę.)
6. **Pięć źdźbeł.** Zmień `BLADE_COUNT` na 5 i przeładuj. Co widać? Potem popraw `TUFT_MAX_VERTICES` na 25. Co się zmieniło? Co pokazuje teraz licznik w panelu i dlaczego się myli?
7. **Bez `EndPrimitive()`.** Usuń wywołanie i przeładuj. Opisz, co widać między źdźbłami jednej kępki.
8. **Liniowo zamiast kwadratowo.** W linii liczącej `middle` zamień `HALF_LEVEL * HALF_LEVEL` na `HALF_LEVEL`. Jak zmienił się kształt źdźbła? Dlaczego wygląda sztywniej?
9. **Prawdziwa normalna.** W `grass.geom` dodaj wyjście z normalną źdźbła (`cross(sideDirection, tip - root)`, znormalizowaną) i użyj jej w `grass.frag` zamiast `GRASS_NORMAL`. Obejdź kępkę dookoła z włączoną latarką. Co się dzieje z jasnością?
10. **Trawa z linii.** Zmień wyjście na `layout(line_strip, max_vertices = 9) out;` i wypisuj dla każdego źdźbła tylko `root`, `middle` i `tip`. Ile wierzchołków i ile odcinków powstaje z kępki?
11. **Macierze za wcześnie.** Przenieś `uProjection * uView` do `grass.vert` (i usuń z `emitBladeVertex`). Obróć kamerę i opisz, co się stało z trawą.
12. **Inne ziarno.** W `Grass.cpp` zmień `GRASS_SEED_OFFSET` na 0 i uruchom testy. Które przechodzą? Czy trawa wygląda gorzej? Dlaczego mimo to przesunięcie jest potrzebne?
13. **Zepsuty etap.** Wpisz błąd składni do `grass.geom`, potem do `grass.frag`, potem zmień nazwę wyjścia `gBladeUv` tylko w `grass.geom`. Za każdym razem przeładuj i przeczytaj komunikat w zakładce Diagnostics / Frame and shaders. Który błąd jest błędem kompilacji, a który linkowania? Jak wygląda różnica w komunikacie?
14. **Trawa bez cienia.** W `grass.frag` zamień linię z `max(...)` na `light = lighting.diffuse;` i przeładuj shadery. Stań przy cieniu ściany przy zgaszonej latarce. Co widać na granicy cienia? (Oczekiwane z kodu: grunt w cieniu jest ciemny, a trawa na nim jasna jak w pełnym świetle księżyca.) Wycofaj zmianę.
15. **Teksel a źdźbło, na kartce.** Mapa cieni księżyca pokrywa dla labiryntu startowego 64,8 na 54,1 m. Ile centymetrów ma teksel przy 2048 i przy 1024 tekselach na bok? Ile tekseli szerokości miałby cień źdźbła u korzenia (4 cm) i w połowie wysokości (2 cm)? (Odpowiedź: `64,8 / 2048 = 3,2 cm` i `64,8 / 1024 = 6,3 cm`. Przy 2048 trochę ponad jeden teksel u korzenia i mniej niż jeden w połowie, przy 1024 mniej niż jeden wszędzie.)

## 9. Pytania kontrolne

1. **Czym jest shader geometrii?**
   Opcjonalnym etapem potoku między shaderem wierzchołków a rasteryzacją. Działa raz na prymityw: dostaje wszystkie jego wierzchołki i wypisuje w jego miejsce nowe prymitywy, także innego rodzaju i w innej liczbie.

2. **Czym różni się od shadera wierzchołków?**
   Shader wierzchołków widzi jeden wierzchołek i zawsze wypisuje jeden. Shader geometrii widzi cały prymityw i może wypisać więcej wierzchołków, mniej albo żadnego.

3. **Co deklarują `layout(points) in` i `layout(triangle_strip, max_vertices = 15) out`?**
   Pierwsza: prymitywem wejściowym jest punkt, więc tablice wejściowe mają jeden element. Druga: wyjściem są paski trójkątów, a jeden punkt da najwyżej 15 wierzchołków.

4. **Co się stanie, gdy program z `layout(points) in` narysuję jako `GL_TRIANGLES`?**
   Wywołanie rysujące zgłosi `GL_INVALID_OPERATION` i nic nie narysuje. Rodzaj wejścia shadera geometrii musi pasować do trybu rysowania.

5. **Do czego służy `max_vertices` i jakie ma granice?**
   To obietnica, ile wierzchołków shader najwyżej wypisze dla jednego prymitywu wejściowego. Karta rezerwuje na nie miejsce, a nadmiarowe porzuca. GLSL 4.10 gwarantuje co najmniej 256 wierzchołków i 1024 składowe wyjściowe łącznie.

6. **Dlaczego `max_vertices` jest ustawione przez `#define`, a nie `const int`?**
   GLSL 4.10 wymaga w `layout(...)` stałej zapisanej wprost. Wyrażenia stałe są tam dozwolone od GLSL 4.40. Preprocesor wstawia liczbę przed kompilacją.

7. **Co robią `EmitVertex()` i `EndPrimitive()`?**
   `EmitVertex()` dopisuje do bieżącego prymitywu wierzchołek z bieżącymi wartościami wszystkich wyjść. `EndPrimitive()` kończy bieżący pasek, a następny wierzchołek zaczyna nowy.

8. **Ile trójkątów daje pasek z pięciu wierzchołków i dlaczego?**
   Trzy. W pasku każdy wierzchołek od trzeciego tworzy trójkąt z dwoma poprzednimi, więc `N` wierzchołków daje `N - 2` trójkąty.

9. **Dlaczego `grass.vert` nie mnoży pozycji przez żadną macierz?**
   Bo pozycja ma dotrzeć do shadera geometrii w przestrzeni świata, w której "w górę" i kierunek wiatru są proste. Macierze widoku i rzutowania nakłada shader geometrii na wierzchołki, które sam tworzy.

10. **Skąd shader fragmentów trawy bierze wejścia?**
    Z shadera geometrii. Gdy program ma ten etap, wyjścia shadera wierzchołków trafiają do niego, a nie do shadera fragmentów.

11. **Jak z jednej liczby losowej powstają trzy różne źdźbła?**
    Kierunek pochylenia to `(r + b / 3) * 2π`, więc źdźbła są rozstawione co 120 stopni, a `r` obraca całą kępkę. Wysokość bierze się z `fract(r * 7 + b * 0,37)`, drugiej liczby pseudolosowej policzonej wzorem.

12. **Dlaczego pochylenie i wiatr rosną z kwadratem wysokości?**
    Żeby źdźbło było wygięte, a nie proste: korzeń stoi w miejscu, w połowie wysokości jest ćwierć wychylenia, na czubku całość.

13. **Jak działa wiatr i dlaczego nic nie kosztuje po stronie procesora?**
    Przesunięcie czubka to sinus z kąta, który rośnie z czasem, zaczyna się w innym miejscu dla każdej kępki i opóźnia się z odległością wzdłuż kierunku wiatru. Zmienia się tylko uniform `uTime`: bufor z punktami jest ten sam.

14. **Dlaczego trawa jest oświetlana normalną `(0, 1, 0)`, a nie normalną źdźbła?**
    Normalna płaskiego źdźbła wskazuje w bok: źdźbło byłoby jasne z jednej strony i czarne z drugiej i migałoby przy ruchu kamery. Z normalną podłoża kępka ma jasność ziemi wokół niej.

15. **Dlaczego trawa jest cieniowana na fragment także w trybie Gouraud?**
    Tryb Gouraud liczy światło w wierzchołkach, a kępka ma w buforze jeden punkt. Jej wierzchołki powstają w shaderze geometrii. Trawa ma jeden program, a tryb `Unlit` obsługuje uniformem `uLit`.

16. **Dlaczego dla trawy odrzucanie tylnych ścian musi być wyłączone?**
    Źdźbło to jeden płaski pasek widoczny z obu stron. Z odrzucaniem znikałaby ta połowa źdźbeł, którą kamera widzi od tyłu. Gra dziś nie włącza odrzucania, a `GrassRenderer::draw` i tak sprawdza stan i w razie potrzeby wyłącza je na czas rysowania.

17. **Co jest w buforze wierzchołków trawy?**
    Jeden `gfx::Vertex` na kępkę: pozycja korzenia w przestrzeni świata i liczba losowa w `uv.x`. Normalna i styczna są zerami i nie są czytane.

18. **Które suwaki zakładki World / Terrain and grass zmieniają bufor, a które tylko uniform?**
    `Density` zmienia liczbę punktów, więc miejsca są liczone od nowa i powstaje nowy bufor. `Blade height` i `Wind strength` to uniformy shadera geometrii.

19. **Jak wybierane są miejsca kępek?**
    Po obu stronach każdej ściany, `round(gęstość * 2 m)` kępek na stronę, każda w losowym miejscu pasa od 21 do 43 cm od osi ściany i nie bliżej niż 30 cm od jej końców. Do tego rzadki rozsiew na wzgórzach, nie bliżej niż 0,6 m od labiryntu.

20. **Skąd wiadomo, że to samo ziarno da tę samą trawę na macOS i na Windowsie?**
    Generator `std::mt19937` ma ciąg ustalony przez standard, a liczby od 0 do 1 powstają z własnej funkcji: liczba całkowita z `randomBelow` podzielona przez 4096. Rozkłady z biblioteki standardowej różnią się między implementacjami. Na dwóch systemach wynik nie był jeszcze porównany.

21. **Dlaczego generator trawy ma ziarno przesunięte o stałą?**
    Żeby nie powtarzał ciągu liczb, którym wykuto labirynt i rozstawiono kryształy. Bez przesunięcia miejsca trawy byłyby związane z kształtem labiryntu.

22. **Ile kępek ma labirynt startowy i skąd ta liczba?**
    1843: 121 ścian razy 10 kępek to 1210, a z 806 prób na wzgórzach 633 leżą dość daleko od labiryntu.

23. **Jak `gfx::Shader` wie, że program ma shader geometrii?**
    Konstruktor ma trzeci, opcjonalny argument ze ścieżką pliku. Pusta ścieżka oznacza program z dwóch etapów. Niepusta dodaje do listy etapów `GL_GEOMETRY_SHADER` między shaderem wierzchołków a fragmentów.

24. **Jaki jest koszt shadera geometrii i jaka jest alternatywa?**
    Karta rezerwuje pamięć na `max_vertices` wierzchołków dla każdego prymitywu i wypisuje je po jednym, co słabo się zrównolegla. Alternatywą dla wielu kopii tego samego kształtu jest rysowanie instancjami (tematu spoza listy 15 tematów wykładu: temat 15 to selekcja obiektów). W tym projekcie kosztu trawy nie udało się zmierzyć: różnica ginie w rozrzucie pomiarów.

25. **Co PRD przewiduje w temacie 9, a czego nie zbudowano?**
    Iskry wokół kryształów. Jest tylko trawa.

25. **Czy trawa leży w cieniu ścian?**
    W cieniu księżyca tak, od czwartej części M7. `grass.frag` dołącza `common/shadows.glsl`, pyta `moonShadow(gWorldPosition, moonFacing(GRASS_NORMAL))` i odejmuje od światła rozproszonego zacieniony udział księżyca: `max(lighting.diffuse - lighting.moonDiffuse * shadow, 0.0)`. Latarka i światła kryształów cieni nie rzucają.

26. **Dlaczego trawa sama nie rzuca cienia?**
    Nie jest rysowana do mapy cieni (`NightMazeApp::drawShadowCasters` ją pomija). Źdźbło ma u korzenia 4 cm, a teksel mapy około 3,2 cm, więc cień byłby migotaniem pojedynczych tekseli, ruchomym przez wiatr, na ziemi zasłoniętej przez samą kępkę.

27. **Kto ustawia uniformy mapy cieni w programie trawy?**
    `NightMazeApp::drawGrass`: woła `m_grassShader.use()`, a potem `setShadowUniforms`, zanim odda sterowanie do `GrassRenderer::draw`. W czwartej części M7 było to 7 wywołań ustawiających uniformy więcej na klatkę (16 zamiast 9). Od piątej części `drawGrass` woła `setShadowUniformsOf`, które ustawia uniformy obu map: 15 wywołań więcej, czyli 24 zamiast 9.

## 10. Źródła

- LearnOpenGL, "Geometry Shader" (<https://learnopengl.com/Advanced-OpenGL/Geometry-Shader>): etap geometrii, deklaracje `layout`, `EmitVertex` i `EndPrimitive`, przykłady z punktów do pasków trójkątów.
- Specyfikacja GLSL 4.10 (<https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.10.pdf>): kwalifikatory `layout` wejścia i wyjścia shadera geometrii (`max_vertices = integer-constant`), funkcje `EmitVertex` i `EndPrimitive`, stałe `gl_MaxGeometryOutputVertices` i `gl_MaxGeometryTotalOutputComponents`.
- Specyfikacja OpenGL 4.1 Core (<https://registry.khronos.org/OpenGL/specs/gl/glspec41.core.pdf>): część "Geometry Shaders" (rodzaje prymitywów wejściowych i wyjściowych, zgodność z trybem rysowania).
- Khronos OpenGL Wiki, "Geometry Shader" (<https://www.khronos.org/opengl/wiki/Geometry_Shader>): tabela wejść i wyjść, ograniczenia, uwagi o wydajności.
- docs.gl: `glCreateShader` (<https://docs.gl/gl4/glCreateShader>, stała `GL_GEOMETRY_SHADER`), `glDrawElements`, `glIsEnabled`, `EmitVertex` (<https://docs.gl/sl4/EmitVertex>), `EndPrimitive` (<https://docs.gl/sl4/EndPrimitive>), `fract`, `mix`.
- Dokumenty w tym repozytorium: [`terrain.md`](terrain.md) (teren, `heightAt`, `distanceOutsideMaze`), [`../gfx/shaders.md`](../gfx/shaders.md) (potok), [`../gfx/shader-class.md`](../gfx/shader-class.md) (trzeci etap w klasie `Shader`), [`../gfx/shader-hot-reload.md`](../gfx/shader-hot-reload.md), [`../gfx/shader-includes.md`](../gfx/shader-includes.md), [`../gfx/mesh.md`](../gfx/mesh.md) (`GL_POINTS`), [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md) (blok świateł), [`../scene/lights.md`](../scene/lights.md) (`computeLighting`), [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) (tryby cieniowania), [`shadows.md`](shadows.md) (mapa cieni księżyca, sekcja 2.16 o trawie), [`../../decisions/grass-casts-no-shadow.md`](../../decisions/grass-casts-no-shadow.md), [`../game/maze-generator.md`](../game/maze-generator.md) (`randomBelow`, lista ścian), [`../debug-ui.md`](../debug-ui.md) (panele), [`README.md`](README.md).
- Notatki o decyzjach: [`../../decisions/grass-lit-with-up-normal.md`](../../decisions/grass-lit-with-up-normal.md), [`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md), [`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdział o shaderach geometrii).
