# Moduł game: labirynt, generator i układ w świecie

Kamień milowy: M2 + M3, w M5 doszły typ `MazeCell`, funkcje `isDeadEnd` i `wallSegmentOn` oraz kryształy, brama i wyjście na planie w panelu Maze. Temat wykładu: żaden wprost. To logika gry, która dostarcza geometrię do tematów 4, 5 i 14 (modele ścian, tekstury, kolizje).
Kod: [`src/game/Maze.hpp`](../../../src/game/Maze.hpp), [`Maze.cpp`](../../../src/game/Maze.cpp), [`src/game/MazeGenerator.hpp`](../../../src/game/MazeGenerator.hpp), [`MazeGenerator.cpp`](../../../src/game/MazeGenerator.cpp), [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`MazeLayout.cpp`](../../../src/game/MazeLayout.cpp), testy w [`tests/MazeTests.cpp`](../../../tests/MazeTests.cpp), [`tests/MazeGeneratorTests.cpp`](../../../tests/MazeGeneratorTests.cpp) i [`tests/MazeLayoutTests.cpp`](../../../tests/MazeLayoutTests.cpp), panel w [`src/debug/panels/MazePanel.hpp`](../../../src/debug/panels/MazePanel.hpp) i [`src/debug/panels/MazePanel.cpp`](../../../src/debug/panels/MazePanel.cpp).

Część modułu `game`. Wstęp do modułu i jego miejsce w warstwach są w [`README.md`](README.md). Ten dokument korzysta z pudełek kolizji z [`../scene/collision.md`](../scene/collision.md) i z konwencji układu współrzędnych z [`../scene/camera.md`](../scene/camera.md), sekcje 2.2 i 2.5. Testy są napisane w bibliotece doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)). Jak z wyników tego kodu powstaje scena (macierze modelu, rysowanie, regeneracja), opisuje [`maze-rendering.md`](maze-rendering.md), kto po labiryncie chodzi, [`player.md`](player.md), a co w nim jest do zrobienia (kryształy, brama, wyjście), [`gameplay.md`](gameplay.md).

## 1. Po co to jest

Night Maze to gra w labiryncie, który za każdym razem jest inny: PRD (sekcja 2) wymaga generatora "recursive backtracker na siatce" sterowanego ziarnem (seed). Ten dokument opisuje trzy rzeczy, które razem dają labirynt gotowy do narysowania i do kolizji:

1. **`game::Maze`**: sam labirynt jako dane. Siatka komórek i informacja, na których krawędziach stoją ściany.
2. **`game::generateMaze`**: algorytm, który z rozmiaru i ziarna buduje losowy labirynt. To samo ziarno daje ten sam labirynt na każdym komputerze i w każdym kompilatorze.
3. **Układ w świecie** (`MazeLayout`): przeliczenie siatki na metry. Gdzie jest środek komórki, gdzie stoi każda ściana i każdy słupek, i jakie mają pudełka kolizji.

Żadna z tych rzeczy nie potrzebuje okna ani OpenGL, więc wszystkie mają testy jednostkowe.

**Stan na dziś, uczciwie.** Kod ma 31 przypadków testowych w trzech plikach (8 w `MazeTests.cpp`, 11 w `MazeGeneratorTests.cpp`, 12 w `MazeLayoutTests.cpp`) i jest używany przez program: `game::buildMazeWorld` woła `generateMaze`, `wallSegments` i `pillarPositions`, a `game::placeOnTerrain` funkcję `colliderBoxes`: z ich wyników powstają rysowany labirynt i lista przeszkód gracza ([`maze-rendering.md`](maze-rendering.md)). Od M6 funkcje układu dają plan na `y = 0`, a labirynt w grze stoi na terenie: wysokość każdej ściany i słupka nadaje dopiero `placeOnTerrain` ([`../renderer/terrain.md`](../renderer/terrain.md)). Gra startuje w labiryncie 10 na 10 komórek z ziarna 1. Panel Maze (sekcja 6) pozwala zamówić inny rozmiar i ziarno i pokazuje plan labiryntu z góry, od M5 z kryształami, bramą i strefą wyjścia. Od M5 na tym kodzie stoi runda: z siatki `Maze` liczone są wyjście (najdalsza komórka), brama i miejsca kryształów ([`gameplay.md`](gameplay.md)). M5 jest na Windowsie kompletny w kodzie i nie jest zamknięty (bez tagu wersji): według zgłoszenia autora kodu z 2026-10-05 build Debug i Release przechodzi bez ostrzeżeń, a wszystkie testy przechodzą w obu konfiguracjach (po M5 215 przypadków i 85098 asercji w całym programie testowym, po drugiej części M6 256 przypadków i 101232 asercje, uruchomione 2026-10-05, po pierwszej części M7 zgłoszone 269 i 102103, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751). M6 też jest kompletny w kodzie na Windowsie i nie jest zamknięty. Z M2 + M3 sprawdzony jest zrzut ekranu, na którym widok z góry zgadza się z planem w panelu. Przycisków panelu nikt jeszcze nie kliknął ręcznie. Na macOS nic z M5 ani z M6 nie było budowane ani uruchamiane.

## 2. Teoria

### 2.1 Siatka komórek i ściany na krawędziach

Labirynt jest prostokątną siatką **komórek** (cells): `width` kolumn wzdłuż osi X i `height` wierszy wzdłuż osi Z. Komórkę wskazują dwie liczby całkowite: kolumna `x` (od 0 do `width - 1`) i wiersz `z` (od 0 do `height - 1`).

Są dwa popularne sposoby zapisu ścian:

| Sposób | Jak wygląda | Skutek |
|---|---|---|
| ściana jako komórka (block maze) | siatka pól, każde pole jest albo podłogą, albo pełnym blokiem | ściany są grube jak korytarze, labirynt `w` na `h` zajmuje siatkę `2w + 1` na `2h + 1` |
| ściana na krawędzi (wall maze) | każda komórka jest podłogą, a ściana stoi **na granicy** między dwiema komórkami | cienkie ściany, cała powierzchnia to korytarze |

Projekt używa drugiego sposobu: widoczne ściany mają 0,2 m przy komórkach 2 na 2 m, co wygląda jak mury, a nie jak bloki.

Każda komórka ma cztery boki, a na każdym boku ściana jest albo jej nie ma (wtedy jest tam **przejście**). Ściana między dwiema sąsiednimi komórkami jest **jedna**, ale widzą ją obie: dla lewej komórki to bok wschodni, dla prawej zachodni. Zapis musi pilnować, żeby obie komórki mówiły to samo.

```text
siatka 3 na 2, widok z góry              ta sama ściana z dwóch stron

      x=0   x=1   x=2                    komórka (0,0)     komórka (1,0)
    +-----+-----+-----+   -> +X          bok East    ==    bok West
z=0 |     |     |     |
    +-----+-----+-----+                  komórka (1,0)     komórka (1,1)
z=1 |     |     |     |                  bok South   ==    bok North
    +-----+-----+-----+
    |
    v +Z
```

### 2.2 Kierunki: ten sam kompas co kamera

Boki komórki nazywają się jak strony świata. Obowiązuje ten sam kompas co dla kąta yaw kamery ([`../scene/camera.md`](../scene/camera.md), sekcja 2.2), więc gracz z yaw 0 patrzy na północ:

| Kierunek | Oś świata | Zmiana kolumny `x` | Zmiana wiersza `z` | Yaw kamery patrzącej w tę stronę |
|---|---|---|---|---|
| North (północ) | -Z | 0 | -1 | 0 |
| East (wschód) | +X | +1 | 0 | 90 |
| South (południe) | +Z | 0 | +1 | 180 |
| West (zachód) | -X | -1 | 0 | 270 |

Na rysunkach z góry północ jest u góry kartki, a oś Z rośnie w dół. To nie pomyłka: w układzie prawoskrętnym z osią Y do góry, patrząc z góry w dół, oś X wskazuje w prawo, a oś Z w stronę patrzącego, czyli na kartce w dół.

### 2.3 Labirynt doskonały

**Labirynt doskonały** (perfect maze) to taki, w którym między każdymi dwiema komórkami istnieje **dokładnie jedna** droga. Wynikają z tego dwie własności:

- **każda komórka jest osiągalna** (istnieje co najmniej jedna droga),
- **nie ma pętli** (istnieje co najwyżej jedna droga).

W języku grafów: komórki to wierzchołki, przejścia to krawędzie, a labirynt doskonały to **drzewo rozpinające** (spanning tree) siatki. Drzewo o `n` wierzchołkach ma zawsze `n - 1` krawędzi, więc:

```text
liczba przejść = width * height - 1
```

Ta liczba jest warunkiem koniecznym, a razem z osiągalnością wszystkich komórek także wystarczającym: graf spójny o `n - 1` krawędziach nie może mieć pętli. Testy sprawdzają właśnie te dwie rzeczy: liczbę przejść i osiągalność (wypełnianiem zalewowym, flood fill).

Dla gry labirynt doskonały ma zaletę (nie ma odciętych obszarów, każdy kryształ da się zebrać) i cechę, którą trzeba znać: nie ma w nim skrótów ani drogi okrężnej.

### 2.4 Recursive backtracker krok po kroku

**Recursive backtracker** to losowe przeszukiwanie w głąb (randomized depth first search). W jednym zdaniu: idź w losowy nieodwiedzony kierunek, wyburzając po drodze ścianę, a gdy nie ma dokąd iść, cofaj się po własnych śladach.

Dokładnie:

1. Zacznij od siatki, w której stoją **wszystkie** ściany. Wybierz komórkę startową, oznacz ją jako odwiedzoną i połóż na stosie.
2. Dopóki stos nie jest pusty, patrz na komórkę na jego szczycie:
   1. zbierz jej sąsiadów, którzy leżą w siatce i **nie byli jeszcze odwiedzeni**,
   2. jeśli nie ma żadnego (ślepy zaułek): zdejmij komórkę ze stosu, czyli cofnij się o jeden krok,
   3. w przeciwnym razie: wylosuj jednego z nich, usuń ścianę między bieżącą komórką a nim, oznacz go jako odwiedzonego i połóż na stosie.
3. Pusty stos oznacza, że algorytm cofnął się aż do startu i nigdzie nie było już nic do odwiedzenia. Każda komórka została odwiedzona.

**Stos** (stack) to lista, z której zdejmuje się zawsze ostatnio położony element. Tutaj stos jest dokładnie **drogą od startu do bieżącej komórki**: położenie komórki to krok naprzód, zdjęcie to krok w tył.

Dlaczego wynik jest labiryntem doskonałym:

- ściana jest usuwana tylko przy wejściu do komórki **nieodwiedzonej**. Każda komórka poza startową jest odwiedzana pierwszy raz dokładnie raz, więc usuniętych ścian jest dokładnie `width * height - 1`,
- każda odwiedzona komórka jest połączona przejściem z tą, z której do niej wszedłem, a przez nią z poprzednią i tak aż do startu. Wszystkie odwiedzone komórki są więc ze sobą połączone,
- algorytm nie kończy się, dopóki istnieje nieodwiedzona komórka sąsiadująca z odwiedzoną (cofanie przechodzi przez każdą odwiedzoną komórkę i każdej daje szansę), a siatka jest spójna, więc odwiedzone zostają wszystkie.

**Przykład: 3 na 3 komórki, ziarno 7.** To prawdziwy przebieg `generateMaze(3, 3, 7)`. Kolumna "liczba" to kolejne wyniki `std::mt19937` z ziarnem 7. Kandydaci są zawsze wypisani w kolejności N, E, S, W, a wybierany jest ten o numerze `liczba % liczba kandydatów` (liczone od zera).

| Krok | Komórka `(x, z)` | Kandydaci | Liczba z generatora | Reszta | Ruch | Stos po kroku |
|---|---|---|---|---|---|---|
| 1 | `(0, 0)` | E, S | 327741615 | `% 2 = 1` | S, do `(0, 1)` | 2 komórki |
| 2 | `(0, 1)` | E, S | 976413892 | `% 2 = 0` | E, do `(1, 1)` | 3 |
| 3 | `(1, 1)` | N, E, S | 3349725721 | `% 3 = 1` | E, do `(2, 1)` | 4 |
| 4 | `(2, 1)` | N, S | 1369975286 | `% 2 = 0` | N, do `(2, 0)` | 5 |
| 5 | `(2, 0)` | W | 1882953283 | `% 1 = 0` | W, do `(1, 0)` | 6 |
| 6 | `(1, 0)` | brak | nie losuję | | cofnięcie do `(2, 0)` | 5 |
| 7 | `(2, 0)` | brak | nie losuję | | cofnięcie do `(2, 1)` | 4 |
| 8 | `(2, 1)` | S | 4201435347 | `% 1 = 0` | S, do `(2, 2)` | 5 |
| 9 | `(2, 2)` | W | 3107259287 | `% 1 = 0` | W, do `(1, 2)` | 6 |
| 10 | `(1, 2)` | W | 1956722279 | `% 1 = 0` | W, do `(0, 2)` | 7 |
| 11 do 17 | `(0, 2)`, potem kolejno w tył | brak | nie losuję | | siedem cofnięć, aż do pustego stosu | 6, 5, ..., 0 |

Co warto zauważyć:

- W kroku 6 komórka `(1, 0)` ma trzech sąsiadów w siatce, ale wszyscy są już odwiedzeni. Ściana między `(1, 0)` a `(0, 0)` zostaje, choć obie komórki są odwiedzone: usunięcie jej zamknęłoby pętlę.
- W kroku 8 algorytm po dwóch cofnięciach znajduje komórkę, z której jest jeszcze dokąd iść. To jest "backtracking".
- Osiem ruchów naprzód to osiem usuniętych ścian: `3 * 3 - 1`. Pętla wykonała się 17 razy: 8 razy naprzód i 9 razy w tył (każda z 9 komórek jest zdejmowana ze stosu dokładnie raz).
- Przy jednym kandydacie liczba też jest losowana, choć wynik jest przesądzony. Kod nie ma osobnej gałęzi dla tego przypadku: jest prostszy, a kolejność losowań i tak jest stała.

Wynik, narysowany tak jak w testach (`+` to narożnik siatki, `--` ściana wzdłuż X, `|` ściana wzdłuż Z, północ u góry):

```text
+--+--+--+
|  |     |
+  +--+  +
|        |
+--+--+  +
|        |
+--+--+--+
```

Tabelę wypisał pomocniczy skrypt w Pythonie z własną, niezależną implementacją generatora Mersenne Twister i tego samego algorytmu. Skrypt nie jest w repozytorium. Jego wynik zgadza się z programem: rysunek powyżej jest przypięty testem `the worked example of the documentation: 3 x 3 cells from seed 7`.

Charakter labiryntów z tego algorytmu: długie, kręte korytarze i mało krótkich odgałęzień, bo algorytm idzie przed siebie, dopóki może.

### 2.5 Dlaczego iteracyjnie, a nie rekurencyjnie

Nazwa algorytmu pochodzi od zapisu podręcznikowego: funkcja `odwiedź(komórka)` woła samą siebie dla wylosowanego sąsiada. Stosem jest wtedy **stos wywołań** (call stack) programu: każde wejście do komórki to jedno zagnieżdżone wywołanie, a powrót z funkcji to cofnięcie.

Problem: głębokość rekurencji jest równa długości najdłuższej drogi, a ta może przejść przez wszystkie komórki. Dla największego dopuszczalnego labiryntu, 256 na 256, to do 65536 zagnieżdżonych wywołań. Stos wywołań ma stały, niewielki rozmiar (domyślnie 1 MB dla głównego wątku programu na Windowsie i 8 MB na macOS), a jego przepełnienie (stack overflow) kończy program bez komunikatu, którego dałoby się obsłużyć.

Wersja iteracyjna robi to samo zwykłą pętlą `while`, a drogę powrotną trzyma we własnym `std::vector` używanym jako stos (`push_back` kładzie, `back` podgląda, `pop_back` zdejmuje). Wektor rośnie na stercie (heap), której rozmiar ogranicza tylko pamięć komputera: 65536 komórek po 8 bajtów to pół megabajta. Algorytm i wynik są identyczne, zmienia się tylko miejsce, w którym leży stos.

### 2.6 Liczby losowe: ten sam labirynt na każdym systemie

Wymaganie: **to samo ziarno ma dać ten sam labirynt na macOS i na Windowsie.** Bez tego nie dałoby się porównać błędu na dwóch komputerach ani pokazać na obronie labiryntu przygotowanego wcześniej. Losowość ma trzy warstwy i każda musi być taka sama wszędzie.

**Warstwa 1: generator.** Komputer nie losuje, tylko liczy: **generator liczb pseudolosowych** startuje od ziarna i każdą kolejną liczbę wylicza z poprzedniego stanu. To samo ziarno daje ten sam ciąg. Używam `std::mt19937` (Mersenne Twister), bo standard C++ opisuje go **co do bitu**: algorytm, stałe i sposób zamiany jednego ziarna na stan początkowy. Standard podaje nawet wartość kontrolną: dziesięciotysięczna liczba z generatora utworzonego bez argumentów ma być równa 4123659995. Każda zgodna biblioteka standardowa musi dać tę liczbę i jeden z testów to sprawdza. `mt19937` zwraca liczby 32-bitowe: od 0 do 4294967295, czyli 2^32 różnych wartości.

**Warstwa 2: zamiana liczby na zakres.** Generator daje liczbę z ogromnego zakresu, a ja potrzebuję "jednego z trzech sąsiadów". Biblioteka standardowa ma do tego `std::uniform_int_distribution`, a do tasowania `std::shuffle`. **Nie używam żadnego z nich**, bo standard opisuje tylko, jaki rozkład mają dać, a **nie opisuje algorytmu**. Każda biblioteka robi to po swojemu: biblioteka clanga na macOS (libc++) i biblioteka MSVC na Windowsie inaczej przeliczają wyniki generatora na zakres i mogą zużyć inną ich liczbę. Ten sam generator z tym samym ziarnem dałby więc na dwóch systemach inne wybory i inne labirynty. Jedną połowę tego zdania zmierzyłem: po wstawieniu `std::uniform_int_distribution` do generatora na Windowsie labirynt dla ziarna 1 jest inny niż z `randomBelow` (ćwiczenie 9). Drugiej połowy, czyli wyniku na macOS, nie mierzyłem. Zamiast nich jest własna, krótka funkcja `randomBelow`, w której są wyłącznie działania na liczbach całkowitych bez znaku, zdefiniowane jednakowo dla każdego kompilatora.

**Warstwa 3: kolejność.** Wylosowana liczba 1 znaczy "drugi kandydat". Który to sąsiad, zależy od kolejności, w jakiej kandydaci zostali zebrani. Dlatego kolejność jest stała i zapisana w jednym miejscu: North, East, South, West (`ALL_DIRECTIONS`). Stała jest też komórka startowa: `(0, 0)`. W generatorze nie ma żadnej liczby zmiennoprzecinkowej ani kontenera o nieokreślonej kolejności.

**Błąd reszty z dzielenia (modulo bias).** Najprostsza zamiana liczby na zakres to `liczba % n`. Jest prawie dobra. Żeby zobaczyć, co jest nie tak, wystarczy mały przykład: generator, który daje liczby od 0 do 7 (osiem wartości), i `n = 3`:

```text
liczba:   0  1  2  3  4  5  6  7
% 3:      0  1  2  0  1  2  0  1

reszta 0 wypada dla 0, 3, 6    (3 liczby)
reszta 1 wypada dla 1, 4, 7    (3 liczby)
reszta 2 wypada dla 2, 5       (2 liczby)   <- rzadziej niż pozostałe
```

Osiem wartości nie dzieli się na trzy równe grupy, więc ostatnia, niepełna grupa faworyzuje małe reszty. Dla `mt19937` jest tak samo, tylko w innej skali: 2^32 dzielone przez 3 daje resztę 1, więc reszta 0 ma 1431655766 liczb, a reszty 1 i 2 po 1431655765.

**Lekarstwo: próbkowanie z odrzucaniem** (rejection sampling). Odcinam niepełną grupę z góry zakresu:

```text
limit = 2^32 - (2^32 % n)        największa wielokrotność n, która mieści się w zakresie
```

Liczba poniżej `limit` jest przyjmowana i wynik to `liczba % n`. Liczba równa `limit` albo większa jest **odrzucana** i losuję następną. Poniżej `limit` każda reszta ma dokładnie `limit / n` liczb, więc wybór jest idealnie równy. W małym przykładzie `limit = 8 - 2 = 6`: liczby 6 i 7 odpadają i każda reszta ma po dwie liczby.

Dla małych wartości `n`:

| `n` (liczba kandydatów) | `2^32 % n` | `limit` | Ile liczb jest odrzucanych |
|---|---|---|---|
| 1 | 0 | 2^32 | żadna |
| 2 | 0 | 2^32 | żadna |
| 3 | 1 | 4294967295 | jedna: 4294967295 |
| 4 | 0 | 2^32 | żadna |

Błąd, który ta funkcja usuwa, jest tu więc mikroskopijny: jedna liczba na ponad cztery miliardy. Zostawiam ją w tej postaci z dwóch powodów. Po pierwsze jest poprawna dla każdego `n`, także dużego, gdzie błąd byłby wyraźny (dla `n` równego trzem miliardom mniejsze reszty wypadałyby dwa razy częściej). Po drugie koszt to trzy linie. Kandydatów jest najwyżej trzech: czterech nieodwiedzonych sąsiadów nie zdarza się nigdy, bo komórka startowa leży w rogu, a do każdej innej wchodzi się z odwiedzonego sąsiada.

Jedno wywołanie `randomBelow` zużywa zwykle jedną liczbę z generatora, a labirynt `width * height - 1` wywołań: po jednym na każde usunięcie ściany.

### 2.7 Układ w świecie: komórki, ściany i słupki

Siatka to liczby całkowite. Żeby coś narysować albo sprawdzić kolizję, trzeba ją przeliczyć na metry. Stałe (PRD, sekcja 9: 1 jednostka to 1 metr, komórka 2 na 2 m, ściana 3 m):

| Stała | Wartość | Znaczenie |
|---|---|---|
| `CELL_SIZE` | 2,0 m | bok kwadratowej komórki, odległość między środkami sąsiednich komórek |
| `WALL_LENGTH` | 2,0 m (`CELL_SIZE`) | długość jednego segmentu ściany: dokładnie jedna krawędź komórki |
| `WALL_HEIGHT` | 3,0 m | wysokość ściany od jej podstawy |
| `PILLAR_SIZE` | 0,3 m | bok kwadratowej podstawy słupka (trzonu) |
| `WALL_VISUAL_THICKNESS` | 0,2 m | grubość widocznego korpusu ściany, czyli modelu `wall_straight`. Ściana stoi na środku krawędzi, więc po 0,1 m wchodzi w każdą z dwóch komórek. Opisuje tylko model: kolizje jej nie używają |
| `WALL_COLLISION_THICKNESS` | 0,3 m (`PILLAR_SIZE`) | grubość **pudełka kolizji** ściany: celowo taka sama jak słupka (wyjaśnienie niżej) |
| `PILLAR_HEIGHT` | 3,15 m | wysokość słupka, 15 cm ponad ścianę |

**Komórki.** Labirynt zaczyna się w początku układu świata i rozciąga w stronę +X i +Z. Funkcje układu z tego pliku dają wszystkie pozycje na wysokości `y = 0`: opisują **plan** labiryntu widziany z góry. Do M5 była to po prostu wysokość płaskiej podłogi. Od M6 podłoże to nierówny teren i `y` każdej rzeczy poprawia `game::placeOnTerrain`, nie ruszając `x` ani `z` ([`maze-rendering.md`](maze-rendering.md), sekcja 2.8).

```text
labirynt 2 na 2, widok z góry, liczby to metry

   x:  0         2         4
z: 0   o---------o---------o        o   narożnik siatki (tu może stać słupek)
       |         |         |        *   środek komórki
       |    *    |    *    |
    1  |  (1,1)  |  (3,1)  |        komórka (x, z) ma środek w
       |         |         |        ((x + 0,5) * 2, 0, (z + 0,5) * 2)
    2  o---------o---------o
       |         |         |        linia siatki numer k leży w k * 2
       |    *    |    *    |
    3  |  (1,3)  |  (3,3)  |
       |         |         |
    4  o---------o---------o
```

Labirynt `width` na `height` zajmuje obszar od `x = 0` do `x = width * 2` i od `z = 0` do `z = height * 2`.

**Ściany.** Jeden segment ściany stoi na jednej krawędzi komórki. Opisują go dwie rzeczy: **pozycja** (środek podstawy segmentu) i **oś**, wzdłuż której biegnie:

| Bok komórki | Oś segmentu | Pozycja segmentu |
|---|---|---|
| North | wzdłuż X (`AlongX`) | `((x + 0,5) * 2, 0, z * 2)` |
| South | wzdłuż X (`AlongX`) | `((x + 0,5) * 2, 0, (z + 1) * 2)` |
| West | wzdłuż Z (`AlongZ`) | `(x * 2, 0, (z + 0,5) * 2)` |
| East | wzdłuż Z (`AlongZ`) | `((x + 1) * 2, 0, (z + 0,5) * 2)` |

Ściana północna i południowa oddzielają wiersze, więc biegną w poprzek osi Z, czyli wzdłuż X. Pozycja jest środkiem podstawy celowo: model ściany ([`../../guides/blender.md`](../../guides/blender.md), sekcja 8) ma początek układu w środku podstawy i jest zbudowany wzdłuż osi X, od `x = -1` do `x = 1`. Segment `AlongX` to model przesunięty w pozycję segmentu. Segment `AlongZ` to ten sam model obrócony o 90 stopni wokół osi Y i przesunięty.

**Każda ściana raz.** Ściana między dwiema komórkami należy do obu, ale w świecie stoi jedna. Reguła, która daje każdą ścianę dokładnie raz: każda komórka zgłasza swoją ścianę **północną i zachodnią**, a południową i wschodnią tylko wtedy, gdy leży w ostatnim wierszu albo w ostatniej kolumnie (tam nie ma sąsiada, który zgłosiłby ją jako swoją północną albo zachodnią).

**Słupki.** W narożniku siatki mogą spotkać się nawet cztery segmenty. Ich końce nachodzą tam na siebie albo zostawiają szczelinę, zależnie od układu. Słupek, grubszy od ściany, zakrywa to miejsce. Słupek stoi w każdym narożniku siatki, w którym **kończy się co najmniej jedna ściana**.

```text
narożnik siatki i cztery ściany, które mogą się w nim kończyć

            ściana wzdłuż Z
            (na północ od narożnika)
                  |
   ściana    -----o-----   ściana wzdłuż X
   wzdłuż X       |        (na wschód od narożnika)
            ściana wzdłuż Z
            (na południe od narożnika)
```

**Ile jest ścian i słupków.** Siatka `w` na `h` ma `w * (h + 1)` krawędzi wzdłuż X i `h * (w + 1)` wzdłuż Z. Labirynt doskonały zamienia `w * h - 1` z nich w przejścia:

```text
ściany = w(h + 1) + h(w + 1) - (wh - 1) = wh + w + h + 1 = (w + 1)(h + 1)
```

W labiryncie doskonałym ściana kończy się w **każdym** narożniku siatki: narożnik wewnętrzny bez żadnej ściany oznaczałby cztery przejścia dookoła niego, czyli pętlę. Słupków jest więc `(w + 1)(h + 1)`. Obie liczby są sobie równe: labirynt doskonały ma tyle samo segmentów ścian co słupków. Dla 16 na 16 to 289 ścian i 289 słupków.

**Pudełka kolizji.** Ściana i słupek stoją na podłodze, więc środek pudełka jest o połowę wysokości nad pozycją:

| Obiekt | Rozmiar pudełka (x, y, z) | Środek pudełka |
|---|---|---|
| ściana `AlongX` | 2,0 na 3,0 na 0,3 | pozycja + `(0, 1,5, 0)` |
| ściana `AlongZ` | 0,3 na 3,0 na 2,0 | pozycja + `(0, 1,5, 0)` |
| słupek | 0,3 na 3,15 na 0,3 | pozycja + `(0, 1,575, 0)` |

Pudełka są prostsze od modeli i **nie mają dokładnie ich wymiarów**. Model ściany ma korpus grubości 0,2 m oraz cokół u dołu i nakrywę u góry o grubości 0,28 m, a model słupka trzon 0,3 na 0,3 m oraz podstawę i głowicę 0,4 na 0,4 m ([`../../guides/blender.md`](../../guides/blender.md), sekcja 8). Pudełko słupka odpowiada trzonowi (0,3 m). Pudełko ściany ma 0,3 m, czyli więcej niż jej korpus.

**Dlaczego pudełko ściany jest grubsze niż ściana.** Pierwsza wersja kodu miała jedną stałą grubości, 0,2 m, używaną i dla modelu, i dla kolizji. Skutek widać z góry:

```text
widok z góry, fragment ściany wzdłuż Z z dwoma słupkami

 pudełko ściany 0,2 m (wcześniej)          pudełko ściany 0,3 m (teraz)

      +---+                                     +---+
      | S |  słupek 0,3                         | S |
      +---+                                     |   |
       | |   ściana 0,2                         |   |   ściana 0,3
       | |      <- gracz sunie tędy             |   |      <- gracz sunie tędy
      +---+     i trafia na słupek,             |   |         i mija słupek:
      | S |     który wystaje 5 cm              | S |         lica są w jednej
      +---+                                     +---+         płaszczyźnie
```

Słupek stoi na każdym łączeniu segmentów, czyli co 2 m. Gracz ślizgający się po ścianie stawał więc co 2 m i musiał odsunąć się o 5 cm. Zachowanie było zmierzone testem. Po podłączeniu gracza okazało się nie do przyjęcia, więc stała została rozdzielona na dwie: `WALL_VISUAL_THICKNESS` opisuje model, a `WALL_COLLISION_THICKNESS` kolizję i jest równa `PILLAR_SIZE`. Lica pudełek ścian i słupków leżą teraz w jednej płaszczyźnie, pudełko gracza tylko styka się ze słupkiem, a styk nie zatrzymuje ruchu ([`../scene/collision.md`](../scene/collision.md), sekcja 2.3). Test, który przypinał zatrzymanie, został zastąpiony testem, który dowodzi ślizgania obok słupków.

Cena: gracz zatrzymuje się 5 cm przed widocznym korpusem ściany (i 1 cm przed cokołem), a podstawa słupka wystaje 5 cm poza pudełko.

## 3. Jak to działa w OpenGL

Nie dotyczy: labirynt, generator i układ to liczby całkowite, kilka stałych i wektory GLM. Pliki nie dołączają GLAD i nie wołają żadnej funkcji `gl*`.

Związek z renderowaniem jest pośredni. Wynik `wallSegments` to lista miejsc, w których trzeba narysować model ściany: z pozycji i osi segmentu `game::buildMazeWorld` liczy macierz modelu, czyli przesunięcie i, dla `AlongZ`, obrót o 90 stopni wokół Y. Tak samo `pillarPositions` dla modelu słupka. `cellCenter` służyło do M5 także płytkom podłogi, które usunął M6: dziś daje środek komórki dla startu, wyjścia i kryształów. Macierze, wywołania OpenGL i koszt rysowania opisuje [`maze-rendering.md`](maze-rendering.md), sekcje 2, 3 i 5.

Panel Maze też nie woła OpenGL bezpośrednio: plan labiryntu rysuje listą rysowania biblioteki ImGui (sekcja 6.3), a dopiero backend ImGui zamienia ją na wywołania OpenGL.

## 4. Shadery

Nie dotyczy: ten kod nie ma shadera. Ściany, słupki i teren pod nimi rysuje jedna z trzech par shaderów (`textured`, `lit` albo `gouraud`, zależnie od trybu cieniowania), a co każda z nich dostaje od rysowania labiryntu, opisuje [`maze-rendering.md`](maze-rendering.md), sekcja 4.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/game/Maze.hpp`](../../../src/game/Maze.hpp), [`.cpp`](../../../src/game/Maze.cpp) | typ `Direction`, stałe `DIRECTION_COUNT` i `ALL_DIRECTIONS`, funkcje `opposite`, `columnStep`, `rowStep`, struktura `MazeCell` (od M5), klasa `Maze`, funkcja `isDeadEnd` (od M5 w tym pliku) |
| [`src/game/MazeGenerator.hpp`](../../../src/game/MazeGenerator.hpp), [`.cpp`](../../../src/game/MazeGenerator.cpp) | funkcje `randomBelow` i `generateMaze` |
| [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`.cpp`](../../../src/game/MazeLayout.cpp) | stałe wymiarów, typy `WallAxis` i `WallSegment`, funkcje `cellCenter`, `wallSegmentOn` (od M5), `wallSegments`, `pillarPositions`, `wallBox`, `pillarBox`, `colliderBoxes` (od M6), `mazeColliders` |
| [`tests/MazeTests.cpp`](../../../tests/MazeTests.cpp) | 8 przypadków testowych: klasa `Maze`, funkcje kierunków, a od M5 `isDeadEnd` i porównanie `MazeCell` |
| [`tests/MazeGeneratorTests.cpp`](../../../tests/MazeGeneratorTests.cpp) | 11 przypadków: generator liczb, `randomBelow`, `generateMaze` |
| [`tests/MazeLayoutTests.cpp`](../../../tests/MazeLayoutTests.cpp) | 12 przypadków: układ w świecie i jego współpraca z kolizjami. Test `wallSegmentOn` jest w [`tests/ExitTests.cpp`](../../../tests/ExitTests.cpp), obok testów bramy, która z tej funkcji korzysta |
| [`src/debug/panels/MazePanel.hpp`](../../../src/debug/panels/MazePanel.hpp), [`.cpp`](../../../src/debug/panels/MazePanel.cpp) | funkcja `debug::drawMazePanel`: panel "Maze" (sekcja 6). Należy do programu `night_maze` |

Sześć plików `Maze.*`, `MazeGenerator.*` i `MazeLayout.*` należy do biblioteki statycznej `game_logic`, razem z `MazeWorld.*`, `Player.*` i plikami rundy `Exit.*`, `Crystals.*`, `Round.*` ([`README.md`](README.md), sekcja 3). Dołączają bibliotekę standardową, a `MazeLayout` także GLM i `scene/Collider.hpp`. Nic z `core/`, `gfx/`, GLAD ani GLFW.

### 5.2 `Direction` i funkcje pomocnicze

Plik `Maze.hpp`:

```cpp
enum class Direction { North, East, South, West };

constexpr int DIRECTION_COUNT = 4;

constexpr std::array<Direction, DIRECTION_COUNT> ALL_DIRECTIONS = {
    Direction::North, Direction::East, Direction::South, Direction::West};
```

| Element | Znaczenie |
|---|---|
| `enum class Direction` | typ wyliczeniowy z własnym zakresem nazw: pisze się `Direction::North`, a wartość nie zamienia się sama na `int`. Wartości dostają kolejne liczby od zera: North 0, East 1, South 2, West 3 |
| `DIRECTION_COUNT` | liczba kierunków, używana jako rozmiar tablic |
| `ALL_DIRECTIONS` | wszystkie kierunki w jednej, stałej kolejności. Pętla `for (const Direction direction : ALL_DIRECTIONS)` przechodzi po bokach komórki zawsze tak samo. To jest "warstwa 3" z sekcji 2.6 |

Plik `Maze.cpp`, anonimowa przestrzeń nazw:

```cpp
constexpr std::array<int, DIRECTION_COUNT> COLUMN_STEPS = {0, 1, 0, -1};
constexpr std::array<int, DIRECTION_COUNT> ROW_STEPS = {-1, 0, 1, 0};

constexpr int OPPOSITE_OFFSET = 2;

std::size_t indexOf(Direction direction) {
    return static_cast<std::size_t>(direction);
}
```

Dwie tablice to tabela z sekcji 2.2 zapisana w kodzie: element o numerze kierunku mówi, o ile zmienia się kolumna i wiersz. `indexOf` zamienia kierunek na jego numer: `enum class` nie robi tego sam, stąd jawne `static_cast`.

```cpp
Direction opposite(Direction direction) {
    // From the enum value to its number, two places on (wrapping around after West), and
    // back to an enum value.
    const int number = (static_cast<int>(direction) + OPPOSITE_OFFSET) % DIRECTION_COUNT;
    return static_cast<Direction>(number);
}

int columnStep(Direction direction) {
    return COLUMN_STEPS[indexOf(direction)];
}

int rowStep(Direction direction) {
    return ROW_STEPS[indexOf(direction)];
}
```

W typie `Direction` kierunek przeciwny stoi zawsze dwa miejsca dalej, licząc w kółko: North (0) i South (2), East (1) i West (3). `% DIRECTION_COUNT` zawija numer: dla South `(2 + 2) % 4 = 0`, czyli North. Dwa rzutowania zamieniają wartość typu wyliczeniowego na liczbę i z powrotem. Funkcja zależy tylko od kolejności wartości w definicji `enum class Direction`, a nie od kolejności w `ALL_DIRECTIONS`.

**`MazeCell` (od M5).** Plik `Maze.hpp`:

```cpp
/// One cell of a maze by its place on the grid. (0, 0) is the north-west corner.
struct MazeCell {
    int x = 0; ///< column: grows towards +X (east)
    int z = 0; ///< row: grows towards +Z (south)

    /// Two cells are the same when both numbers match. "= default" lets the compiler
    /// write that comparison.
    bool operator==(const MazeCell& other) const = default;
};
```

| Element | Znaczenie |
|---|---|
| `int x = 0;`, `int z = 0;` | kolumna i wiersz, te same dwie liczby, które przyjmują `hasWall` i `cellCenter`. Wartości domyślne sprawiają, że `MazeCell{}` to róg północno-zachodni |
| `bool operator==(const MazeCell& other) const = default;` | porównanie pisane przez kompilator (C++20): dwie komórki są równe, gdy równe są oba pola. Bez tej linii `a == b` dla struktur by się nie skompilowało. Z `==` kompilator sam wyprowadza też `!=` |

Do M4 komórka była w kodzie zawsze dwiema osobnymi liczbami. Runda potrzebuje komórki jako **jednej wartości**: funkcja ma ją zwrócić (`farthestCell` zwraca komórkę wyjścia), przyjąć jako parametr (`placeExit(maze, start)`, `placeCrystals(maze, seed, start, exit)`), zapamiętać w polu (`MazeWorld::exitCell`, `CrystalSpawn::cell`) i porównać z inną ("czy to komórka startowa?"). Stąd struktura i operator. Klasa `Maze` nadal przyjmuje `x` i `z` osobno: `MazeCell` jej nie zmienił. Kto z niego korzysta, opisuje [`gameplay.md`](gameplay.md), sekcja 5.

### 5.3 Klasa `Maze`

```cpp
class Maze {
public:
    static constexpr int MAX_SIZE = 256;

    Maze(int width, int height);

    int width() const { return m_width; }
    int height() const { return m_height; }

    bool contains(int x, int z) const;
    bool hasWall(int x, int z, Direction side) const;
    void removeWall(int x, int z, Direction side);

private:
    std::size_t cellIndex(int x, int z) const;

    int m_width;
    int m_height;

    std::vector<std::array<bool, DIRECTION_COUNT>> m_walls;
};
```

Blok pokazuje deklaracje bez komentarzy Doxygen, które w pliku stoją nad każdą funkcją.

**Dlaczego klasa, a nie struktura z publicznymi polami.** `Transform`, `Camera` i `Aabb` są strukturami, bo każda kombinacja wartości ich pól ma sens. `Maze` ma **niezmiennik** (invariant), czyli warunek, który musi być prawdziwy zawsze: obie komórki mówią to samo o ścianie między sobą. Gdyby tablica ścian była publiczna, każdy mógłby zmienić jedną stronę ściany i zapomnieć o drugiej. Pola są więc prywatne (prefiks `m_`), a jedyną drogą zmiany jest `removeWall`, która pilnuje obu stron.

**Zapis ścian.** `m_walls` ma jeden element na komórkę, a element to cztery wartości `bool`: "czy stoi ściana" dla boków w kolejności `Direction`. Komórki leżą wierszami: najpierw cały wiersz 0, potem wiersz 1.

```cpp
int checkedSize(int size) {
    if (size < 1 || size > Maze::MAX_SIZE) {
        throw std::invalid_argument("Maze: width and height must be between 1 and MAX_SIZE");
    }
    return size;
}
```

```cpp
Maze::Maze(int width, int height)
    : m_width(checkedSize(width)),
      m_height(checkedSize(height)),
      m_walls(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height),
              {true, true, true, true}) {}
```

| Fragment | Znaczenie |
|---|---|
| `checkedSize(width)` | zwraca rozmiar bez zmian albo rzuca wyjątek `std::invalid_argument`, gdy jest mniejszy od 1 albo większy od `MAX_SIZE`. Wyjątek z listy inicjalizacyjnej przerywa tworzenie obiektu: labirynt o złym rozmiarze nigdy nie powstaje |
| kolejność na liście | pola są inicjalizowane w kolejności **deklaracji w klasie** (`m_width`, `m_height`, `m_walls`), nie w kolejności zapisu na liście. Tu obie są takie same, więc `m_walls` może użyć już sprawdzonych `m_width` i `m_height` |
| `static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height)` | liczba komórek. Rzutowanie przed mnożeniem, bo rozmiar wektora ma typ `std::size_t` (bez znaku) |
| `{true, true, true, true}` | wartość każdego elementu: wszystkie cztery ściany stoją |

`MAX_SIZE` równe 256 trzyma liczbę komórek (najwyżej 65536) daleko od granicy typu `int`, a pamięć na poziomie 256 KB. Labirynt 256 na 256 miałby 512 na 512 metrów, czyli dużo więcej, niż gra potrzebuje.

```cpp
bool Maze::contains(int x, int z) const {
    return x >= 0 && x < m_width && z >= 0 && z < m_height;
}

bool Maze::hasWall(int x, int z, Direction side) const {
    return m_walls[cellIndex(x, z)][indexOf(side)];
}
```

`hasWall` to dwa indeksy: który element wektora (komórka) i która z czterech wartości (bok).

```cpp
void Maze::removeWall(int x, int z, Direction side) {
    m_walls[cellIndex(x, z)][indexOf(side)] = false;

    // The same wall seen from the other cell: the neighbour on that side has it on its
    // opposite side. A border cell has no neighbour there.
    const int neighbourX = x + columnStep(side);
    const int neighbourZ = z + rowStep(side);
    if (contains(neighbourX, neighbourZ)) {
        m_walls[cellIndex(neighbourX, neighbourZ)][indexOf(opposite(side))] = false;
    }
}
```

| Linia | Znaczenie |
|---|---|
| `m_walls[cellIndex(x, z)][indexOf(side)] = false;` | ściana znika z tej komórki |
| `x + columnStep(side)`, `z + rowStep(side)` | sąsiad po tej stronie |
| `if (contains(neighbourX, neighbourZ))` | komórka na brzegu nie ma sąsiada po stronie zewnętrznej. Wtedy znika tylko ściana zewnętrzna. Gra nigdy tego nie robi: wyjście z M5 nie jest otworem w murze zewnętrznym, tylko komórką wewnątrz labiryntu, zamkniętą bramą ([`gameplay.md`](gameplay.md)) |
| `indexOf(opposite(side))` | ta sama ściana widziana z drugiej komórki: jej bok przeciwny |

To jest jedyne miejsce, które zmienia ściany, i zawsze zmienia obie strony. Stąd bierze się niezmiennik.

```cpp
std::size_t Maze::cellIndex(int x, int z) const {
    // The check has to look at both coordinates: x = width in row 0 would otherwise
    // quietly land on the first cell of row 1.
    if (!contains(x, z)) {
        throw std::out_of_range("Maze: cell is outside the maze");
    }
    // Rows are stored one after another, so row z starts after z full rows.
    return static_cast<std::size_t>(z) * static_cast<std::size_t>(m_width) +
           static_cast<std::size_t>(x);
}
```

Numer komórki w wektorze to `z * width + x`: przed wierszem `z` leży `z` pełnych wierszy po `width` komórek. Sprawdzenie zakresu musi dotyczyć obu współrzędnych osobno. Sam numer nie wystarczy: dla siatki 3 na 2 komórka `(3, 0)` dałaby numer 3, który istnieje, ale należy do komórki `(0, 1)`.

**`isDeadEnd` (od M5 w `Maze.*`).** Wolna funkcja obok klasy:

```cpp
// A dead end is closed on three of its four sides.
constexpr int DEAD_END_WALL_COUNT = 3;
```

```cpp
bool isDeadEnd(const Maze& maze, int x, int z) {
    int wallCount = 0;
    for (const Direction side : ALL_DIRECTIONS) {
        if (maze.hasWall(x, z, side)) {
            ++wallCount;
        }
    }
    return wallCount == DEAD_END_WALL_COUNT;
}
```

| Linia | Znaczenie |
|---|---|
| `for (const Direction side : ALL_DIRECTIONS)` | cztery boki komórki |
| `if (maze.hasWall(x, z, side)) { ++wallCount; }` | licznik ścian. `hasWall` rzuca `std::out_of_range` dla komórki spoza siatki, więc `isDeadEnd` też |
| `return wallCount == DEAD_END_WALL_COUNT;` | **dokładnie** trzy ściany: jedno wejście i żadnej drogi dalej. Komórka z czterema ścianami (labirynt 1 na 1 albo świeża siatka przed generowaniem) nie jest ślepym zaułkiem, bo nie da się do niej wejść |

Funkcja jest wolna, a nie metodą klasy, bo korzysta tylko z publicznego `hasWall`: niezmiennika klasy nie dotyka. W M4 leżała w `game/Lighting.*`, bo jedynym użytkownikiem były światła punktowe wieszane w ślepych zaułkach. W M5 te światła zniknęły (wiszą teraz nad kryształami), a pytanie "czy to ślepy zaułek" zadaje `placeCrystals`: kryształy trafiają najpierw do ślepych zaułków ([`gameplay.md`](gameplay.md), sekcja 2). Pytanie dotyczy samego labiryntu, więc funkcja przeniosła się do pliku labiryntu, razem ze swoim testem.

### 5.4 `randomBelow`

```cpp
constexpr std::uint64_t GENERATOR_RANGE = std::uint64_t{1} << 32U;
```

Liczba różnych wyników `std::mt19937`: 2^32, zapisane jako jedynka przesunięta o 32 bity w lewo. Ta liczba nie mieści się w 32 bitach (największa wartość 32-bitowa to 2^32 - 1), dlatego stała i rachunki na niej mają typ 64-bitowy.

```cpp
std::uint32_t randomBelow(std::mt19937& generator, std::uint32_t bound) {
    if (bound == 0U) {
        throw std::invalid_argument("randomBelow: bound must be at least 1");
    }

    const std::uint64_t limit = GENERATOR_RANGE - GENERATOR_RANGE % bound;

    std::uint64_t value = generator();
    while (value >= limit) {
        value = generator();
    }
    return static_cast<std::uint32_t>(value % bound);
}
```

Blok pomija dwa komentarze, które w pliku tłumaczą błąd reszty i pętlę.

| Linia | Znaczenie |
|---|---|
| `std::mt19937& generator` | generator jest przekazany przez referencję, bo każde losowanie **zmienia jego stan**. Kopia sprawiłaby, że wołający dostawałby w kółko tę samą liczbę |
| `if (bound == 0U) { throw ...; }` | "liczba mniejsza od zera" nie istnieje wśród liczb bez znaku, a `% 0` jest niezdefiniowane, więc zero jest odrzucane wyjątkiem |
| `limit = GENERATOR_RANGE - GENERATOR_RANGE % bound` | największa wielokrotność `bound` w zakresie generatora (sekcja 2.6) |
| `std::uint64_t value = generator();` | jedno losowanie. Wynik ma najwyżej 32 bity, nawet tam, gdzie typ zwracany przez generator jest szerszy (na niektórych systemach to typ 64-bitowy), więc w zmiennej 64-bitowej mieści się zawsze |
| `while (value >= limit) { value = generator(); }` | odrzucanie: liczba z niepełnej grupy jest zastępowana następną. Pętla się kończy, bo odrzucana jest zawsze mniej niż połowa zakresu |
| `static_cast<std::uint32_t>(value % bound)` | reszta jest mniejsza od `bound`, więc mieści się w 32 bitach. Rzutowanie jest jawne, żeby kompilator nie ostrzegał o zawężeniu typu |

Funkcja jest publiczna (w nagłówku), a nie schowana w pliku `.cpp`, z dwóch powodów: testy sprawdzają ją osobno, a korzysta z niej wszystko, co w grze losuje powtarzalnie. Od M5 jest drugi taki użytkownik: `placeCrystals` w `Crystals.cpp` tasuje nią komórki i losuje model każdego kryształu ([`gameplay.md`](gameplay.md), sekcja 5).

### 5.5 `generateMaze`

```cpp
constexpr int START_COLUMN = 0;
constexpr int START_ROW = 0;

struct Cell {
    int x;
    int z;
};

std::size_t visitedIndex(const Maze& maze, Cell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}
```

`Cell` to para współrzędnych, element stosu. `visitedIndex` liczy numer komórki na liście odwiedzonych tym samym wzorem `z * width + x` co `Maze::cellIndex`. `Cell`, `START_COLUMN` i `START_ROW` są prywatne dla pliku `MazeGenerator.cpp` (anonimowa przestrzeń nazw). Od M5 w `Maze.hpp` jest publiczna struktura `MazeCell` o tych samych dwóch polach (sekcja 5.2), a `MazeWorld.cpp` ma własną stałą `START_CELL`: generator nie został na nie przepisany i nadal używa swoich lokalnych nazw. Obie stałe wskazują tę samą komórkę `(0, 0)`, ale nic w kodzie tego nie wymusza.

```cpp
Maze generateMaze(int width, int height, std::uint32_t seed) {
    // All walls are present at first. The constructor also rejects a wrong size.
    Maze maze(width, height);

    // The Mersenne Twister. Both its algorithm and the way one number seeds it are
    // written down in the C++ standard, so every compiler produces the same sequence.
    std::mt19937 generator(seed);
```

Krok 1 algorytmu z sekcji 2.4: siatka ze wszystkimi ścianami. Zły rozmiar kończy funkcję wyjątkiem z konstruktora `Maze`. Generator jest zmienną lokalną: żyje tylko przez czas generowania i nie ma żadnego stanu globalnego, więc dwa wywołania z tym samym ziarnem nie mogą na siebie wpłynąć.

```cpp
    std::vector<bool> visited(
        static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height()), false);
    std::vector<Cell> path;

    const Cell start{.x = START_COLUMN, .z = START_ROW};
    visited[visitedIndex(maze, start)] = true;
    path.push_back(start);
```

| Linia | Znaczenie |
|---|---|
| `std::vector<bool> visited(..., false)` | jedna flaga na komórkę: czy algorytm już w niej był. Na początku wszędzie fałsz |
| `std::vector<Cell> path;` | stos, czyli droga od startu do bieżącej komórki (sekcja 2.5) |
| `const Cell start{.x = START_COLUMN, .z = START_ROW};` | komórka startowa, róg północno-zachodni. Inicjalizator desygnowany (C++20) podaje nazwy pól |
| dwie ostatnie linie | start jest od razu odwiedzony i leży na stosie |

```cpp
    while (!path.empty()) {
        const Cell current = path.back();

        // Collect the directions that lead to an unvisited cell, always in the order
        // North, East, South, West. The order is part of the result: with another order
        // the same random number would pick another neighbour.
        std::array<Direction, DIRECTION_COUNT> candidates{};
        std::uint32_t candidateCount = 0;
        for (const Direction direction : ALL_DIRECTIONS) {
            const Cell neighbour{.x = current.x + columnStep(direction),
                                 .z = current.z + rowStep(direction)};
            if (maze.contains(neighbour.x, neighbour.z) &&
                !visited[visitedIndex(maze, neighbour)]) {
                candidates[candidateCount] = direction;
                ++candidateCount;
            }
        }
```

| Linia | Znaczenie |
|---|---|
| `while (!path.empty())` | pętla główna: dopóki jest dokąd wracać |
| `const Cell current = path.back();` | komórka na szczycie stosu. `back()` tylko ją podgląda, nie zdejmuje |
| `std::array<Direction, DIRECTION_COUNT> candidates{};` i `candidateCount` | tablica o stałym rozmiarze 4 i licznik zajętych miejsc. Kandydatów jest najwyżej czterech, więc nie potrzeba wektora ani przydziału pamięci w każdym obrocie pętli |
| `for (const Direction direction : ALL_DIRECTIONS)` | cztery kierunki w stałej kolejności |
| `maze.contains(...) && !visited[...]` | sąsiad musi leżeć w siatce i być nieodwiedzony. Kolejność warunków jest ważna: `&&` nie liczy prawej strony, gdy lewa jest fałszem, więc `visitedIndex` nigdy nie dostaje komórki spoza siatki |
| `candidates[candidateCount] = direction; ++candidateCount;` | dopisanie kandydata na pierwsze wolne miejsce |

```cpp
        // Dead end: step back to the previous cell of the path.
        if (candidateCount == 0U) {
            path.pop_back();
            continue;
        }

        // Pick one of the candidates, each with the same chance.
        const Direction chosen = candidates[randomBelow(generator, candidateCount)];
        const Cell next{.x = current.x + columnStep(chosen), .z = current.z + rowStep(chosen)};

        // Every cell except the start is entered exactly once, and each time exactly one
        // wall is removed. That gives width * height - 1 passages and no loops: a wall to
        // an already visited cell is never removed.
        maze.removeWall(current.x, current.z, chosen);
        visited[visitedIndex(maze, next)] = true;
        path.push_back(next);
    }

    return maze;
}
```

| Linia | Znaczenie |
|---|---|
| `path.pop_back(); continue;` | ślepy zaułek: komórka schodzi ze stosu, a następny obrót pętli zobaczy tę, z której do niej wszedłem |
| `candidates[randomBelow(generator, candidateCount)]` | losowy numer od 0 do `candidateCount - 1` wybiera kandydata |
| `maze.removeWall(current.x, current.z, chosen);` | przejście między bieżącą komórką a wybraną. `removeWall` usuwa ścianę po obu stronach |
| `visited[...] = true; path.push_back(next);` | krok naprzód: nowa komórka jest odwiedzona i staje się szczytem stosu |
| `return maze;` | labirynt wraca przez wartość. Kompilator przenosi go do wołającego bez kopiowania wektora |

### 5.6 Układ w świecie: stałe, `WallSegment`, `cellCenter`, `wallSegmentOn`, `wallSegments`

Plik `MazeLayout.hpp`:

```cpp
constexpr float CELL_SIZE = 2.0F;
constexpr float WALL_LENGTH = CELL_SIZE;
constexpr float WALL_HEIGHT = 3.0F;
constexpr float PILLAR_SIZE = 0.3F;
constexpr float WALL_VISUAL_THICKNESS = 0.2F;
constexpr float WALL_COLLISION_THICKNESS = PILLAR_SIZE;
constexpr float PILLAR_HEIGHT = 3.15F;
```

W pliku każda stała ma nad sobą komentarz. Znaczenie każdej podaje tabela w sekcji 2.7. `PILLAR_SIZE` stoi przed grubościami ściany, bo `WALL_COLLISION_THICKNESS` jest zdefiniowana przez nią: zapis `= PILLAR_SIZE` zamiast drugiego `0.3F` sprawia, że obu liczb nie da się zmienić osobno przez pomyłkę. Stałe są w nagłówku, bo korzysta z nich także kod poza tym plikiem: panel Maze (`CELL_SIZE`, `WALL_LENGTH`), panel Collision (obie grubości) i testy. `WALL_VISUAL_THICKNESS` nie bierze udziału w żadnym obliczeniu gry: dokumentuje wymiar modelu i jest pokazywana w panelu Collision.

```cpp
enum class WallAxis {
    AlongX, ///< on the north or south edge of a cell
    AlongZ, ///< on the west or east edge of a cell
};

struct WallSegment {
    glm::vec3 position{0.0F};
    WallAxis axis = WallAxis::AlongX;
};
```

`WallSegment` to zwykłe dane: gdzie stoi segment (środek jego podstawy) i wzdłuż której osi biegnie. W pliku nad każdym polem stoi komentarz Doxygen, który opisuje też, jak ustawić model. Komentarz pola `position` mówi od M6 wprost, kto nadaje wysokość: "The layout functions of this file give y = 0. A maze that stands on a terrain lowers every segment to the ground under it (game::placeOnTerrain)".

Plik `MazeLayout.cpp`, funkcje pomocnicze:

```cpp
constexpr float HALF_CELL = 0.5F;

float gridLine(int line) {
    return static_cast<float>(line) * CELL_SIZE;
}

float cellMiddle(int cell) {
    return (static_cast<float>(cell) + HALF_CELL) * CELL_SIZE;
}
```

`gridLine(k)` to współrzędna linii siatki numer `k` (brzeg komórki), `cellMiddle(k)` to współrzędna środka kolumny albo wiersza numer `k`. Obie funkcje służą i osi X, i osi Z, bo komórka jest kwadratem. `static_cast<float>` zamienia numer całkowity na liczbę zmiennoprzecinkową jawnie.

```cpp
glm::vec3 cellCenter(int x, int z) {
    return {cellMiddle(x), 0.0F, cellMiddle(z)};
}
```

Środek komórki na wysokości `y = 0` (komentarz w nagłówku mówił do M5 "at floor level", dziś "at y = 0"). Wysokość gruntu w tym miejscu daje osobna funkcja `groundHeightAt` z `MazeWorld.hpp`. Funkcja nie sprawdza, czy komórka należy do jakiegoś labiryntu: to czysty wzór.

**`wallSegmentOn` (od M5).** Do M4 wzory na pozycję segmentu stały cztery razy wewnątrz `wallSegments`. W M5 zostały wyjęte do osobnej, publicznej funkcji:

```cpp
WallSegment wallSegmentOn(int x, int z, Direction side) {
    // The north edge of a cell is the row line z, the south edge the row line z + 1.
    // The west edge is the column line x, the east edge the column line x + 1.
    if (side == Direction::North) {
        return {.position = {cellMiddle(x), 0.0F, gridLine(z)}, .axis = WallAxis::AlongX};
    }
    if (side == Direction::South) {
        return {.position = {cellMiddle(x), 0.0F, gridLine(z + 1)}, .axis = WallAxis::AlongX};
    }
    if (side == Direction::West) {
        return {.position = {gridLine(x), 0.0F, cellMiddle(z)}, .axis = WallAxis::AlongZ};
    }
    // East is what is left.
    return {.position = {gridLine(x + 1), 0.0F, cellMiddle(z)}, .axis = WallAxis::AlongZ};
}
```

To tabela z sekcji 2.7 zapisana w kodzie, wiersz po wierszu: bok północny leży na linii wierszy numer `z`, południowy na linii `z + 1`, zachodni na linii kolumn numer `x`, wschodni na linii `x + 1`. Cztery `return` z inicjalizatorem desygnowanym budują `WallSegment` od razu w miejscu zwrotu. Ostatni nie ma warunku: po trzech sprawdzonych kierunkach zostaje tylko wschód.

Najważniejsze zdanie stoi w komentarzu nagłówka:

```cpp
/// The segment that stands on one side of the cell in column x and row z: on its north
/// or south edge it runs along X, on its west or east edge along Z. The function does
/// not ask whether the maze has a wall there: the gate of the exit uses it for a side
/// that is open.
WallSegment wallSegmentOn(int x, int z, Direction side);
```

Funkcja **nie dostaje labiryntu** i nie pyta, czy ściana tam stoi. To czysta geometria: "gdzie stałby segment na tym boku tej komórki". Dzięki temu ma dwóch użytkowników. `wallSegments` woła ją dla boków, na których ściana jest. `placeExit` (`Exit.cpp`) woła ją dla boku, na którym ściany **nie ma**: brama stoi na otwartym boku komórki wyjścia, dokładnie tam, gdzie stałby segment ściany ([`gameplay.md`](gameplay.md), sekcja 5). Brama i ściany mają więc pozycje z jednego wzoru i ten sam typ `WallSegment`, a dalej te same funkcje: `wallBox` dla pudełka kolizji i `wallModelMatrix` dla macierzy modelu ([`maze-rendering.md`](maze-rendering.md), sekcja 5.5).

**`wallSegments`.**

```cpp
std::vector<WallSegment> wallSegments(const Maze& maze) {
    std::vector<WallSegment> segments;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            // Every cell reports its north and its west wall.
            if (maze.hasWall(x, z, Direction::North)) {
                segments.push_back(wallSegmentOn(x, z, Direction::North));
            }
            if (maze.hasWall(x, z, Direction::West)) {
                segments.push_back(wallSegmentOn(x, z, Direction::West));
            }
```

Dwie pętle przechodzą po komórkach wierszami. Każda komórka zgłasza ścianę północną (leży na linii wierszy numer `z`, w połowie kolumny `x`) i zachodnią (na linii kolumn numer `x`, w połowie wiersza `z`). Samo "gdzie" liczy teraz `wallSegmentOn`, a `wallSegments` odpowiada tylko za "które": pyta `hasWall` i pilnuje, żeby każda ściana trafiła na listę raz.

```cpp
            // The south wall of a cell is the north wall of the cell below it, and the
            // east wall is the west wall of the cell to the right: those cells report
            // them. That is how a shared wall ends up in the list once. Only the last
            // row and the last column have no such neighbour and report the wall
            // themselves.
            if (z == maze.height() - 1 && maze.hasWall(x, z, Direction::South)) {
                segments.push_back(wallSegmentOn(x, z, Direction::South));
            }
            if (x == maze.width() - 1 && maze.hasWall(x, z, Direction::East)) {
                segments.push_back(wallSegmentOn(x, z, Direction::East));
            }
        }
    }
    return segments;
}
```

Ściany południowa i wschodnia są zgłaszane tylko na brzegu siatki: `z == maze.height() - 1` to ostatni wiersz, `x == maze.width() - 1` to ostatnia kolumna. W każdym innym miejscu zgłosi je sąsiad jako swoją północną albo zachodnią. Dzięki temu ściana wspólna trafia na listę **raz**, bez żadnego usuwania duplikatów.

Kolejność segmentów na liście jest stała (wiersz po wierszu, komórka po komórce, w kolejności czterech warunków), więc ten sam labirynt daje zawsze tę samą listę. Wyjęcie wzorów do `wallSegmentOn` jej nie zmieniło: lista jest co do elementu taka jak w M4, a brama na nią nie trafia (bok, na którym stoi, nie ma ściany).

### 5.7 `pillarPositions`, pudełka, `colliderBoxes` i `mazeColliders`

```cpp
bool cellHasWall(const Maze& maze, int x, int z, Direction side) {
    return maze.contains(x, z) && maze.hasWall(x, z, side);
}
```

"Komórka istnieje i ma ścianę po tej stronie". Dla komórki spoza siatki zwraca fałsz, zamiast rzucać wyjątek: `&&` nie woła `hasWall`, gdy `contains` jest fałszem. Narożniki na brzegu siatki mają wokół siebie mniej niż cztery komórki i właśnie tu są one pomijane.

```cpp
bool cornerHasWall(const Maze& maze, int cornerX, int cornerZ) {
    // The cell north-west of the corner touches it with its south and east walls.
    if (cellHasWall(maze, cornerX - 1, cornerZ - 1, Direction::South) ||
        cellHasWall(maze, cornerX - 1, cornerZ - 1, Direction::East)) {
        return true;
    }
    // North-east of the corner: south and west walls.
    if (cellHasWall(maze, cornerX, cornerZ - 1, Direction::South) ||
        cellHasWall(maze, cornerX, cornerZ - 1, Direction::West)) {
        return true;
    }
    // South-west of the corner: north and east walls.
    if (cellHasWall(maze, cornerX - 1, cornerZ, Direction::North) ||
        cellHasWall(maze, cornerX - 1, cornerZ, Direction::East)) {
        return true;
    }
    // South-east of the corner: north and west walls.
    return cellHasWall(maze, cornerX, cornerZ, Direction::North) ||
           cellHasWall(maze, cornerX, cornerZ, Direction::West);
}
```

Narożnik siatki `(cornerX, cornerZ)` to przecięcie linii kolumn numer `cornerX` z linią wierszy numer `cornerZ`. Dotykają go najwyżej cztery komórki, a każda z nich ma dwie ściany, które się w nim kończą: te na bokach zwróconych w stronę narożnika.

```text
          kolumna cornerX - 1      kolumna cornerX

wiersz    +------------------+------------------+
cornerZ   |                  |                  |
 - 1      |     ściany       |     ściany       |
          |   South i East   |   South i West   |
          +------------------o------------------+      o   narożnik
wiersz    |     ściany       |     ściany       |
cornerZ   |   North i East   |   North i West   |
          |                  |                  |
          +------------------+------------------+
```

Każda z czterech ścian wychodzących z narożnika jest tu sprawdzana z obu swoich stron (na przykład ściana na północ od narożnika to bok East lewej górnej komórki i bok West prawej górnej). To nadmiar, ale nieszkodliwy: obie strony mówią to samo, a dzięki niemu funkcja działa także na brzegu, gdzie jedna z dwóch komórek nie istnieje.

```cpp
std::vector<glm::vec3> pillarPositions(const Maze& maze) {
    std::vector<glm::vec3> positions;
    // A grid of width by height cells has one more line than cells in each direction,
    // hence "<=": corners run from 0 to width and from 0 to height.
    for (int cornerZ = 0; cornerZ <= maze.height(); ++cornerZ) {
        for (int cornerX = 0; cornerX <= maze.width(); ++cornerX) {
            if (cornerHasWall(maze, cornerX, cornerZ)) {
                positions.emplace_back(gridLine(cornerX), 0.0F, gridLine(cornerZ));
            }
        }
    }
    return positions;
}
```

Pętle mają `<=`, a nie `<`: siatka o `width` kolumnach ma `width + 1` linii, od 0 do `width` włącznie. `emplace_back` buduje `glm::vec3` z trzech liczb od razu w wektorze.

```cpp
// Half extents of the boxes, as scene::Aabb::fromCenter wants them. A wall along Z is
// the same box with its length and its thickness swapped. The half thickness of a wall is
// written exactly like the half size of a pillar below (the constant divided by 2), so
// both give the same float and the faces of the two kinds of boxes meet without a step.
constexpr glm::vec3 WALL_ALONG_X_HALF_EXTENTS{WALL_LENGTH / 2.0F, WALL_HEIGHT / 2.0F,
                                              WALL_COLLISION_THICKNESS / 2.0F};
constexpr glm::vec3 WALL_ALONG_Z_HALF_EXTENTS{WALL_COLLISION_THICKNESS / 2.0F, WALL_HEIGHT / 2.0F,
                                              WALL_LENGTH / 2.0F};
constexpr glm::vec3 PILLAR_HALF_EXTENTS{PILLAR_SIZE / 2.0F, PILLAR_HEIGHT / 2.0F,
                                        PILLAR_SIZE / 2.0F};
```

Połowy rozmiarów pudełek, policzone w czasie kompilacji ze stałych z nagłówka. Ściana wzdłuż Z to to samo pudełko co wzdłuż X, z zamienioną długością i grubością: tak wygląda obrót AABB o 90 stopni.

Komentarz zwraca uwagę na szczegół liczb zmiennoprzecinkowych. Połowa grubości pudełka ściany i połowa boku słupka są liczone **tym samym wyrażeniem** z tej samej wartości (`0.3F / 2.0F`), więc dają bit w bit tę samą liczbę `float`. Lico pudełka ściany na linii siatki `g` to `g + 0,15` i lico pudełka słupka w tym samym węźle to też `g + 0,15`, policzone tak samo. Gdyby jedna z połówek była wpisana jako osobny literał albo policzona inną drogą, lica mogłyby się różnić o ostatni bit i między pudełkami powstałby mikroskopijny schodek.

```cpp
scene::Aabb wallBox(const WallSegment& segment) {
    // The position is the base, the centre of the box is half of the height above it.
    const glm::vec3 center = segment.position + glm::vec3{0.0F, WALL_HEIGHT / 2.0F, 0.0F};
    const glm::vec3 halfExtents =
        segment.axis == WallAxis::AlongX ? WALL_ALONG_X_HALF_EXTENTS : WALL_ALONG_Z_HALF_EXTENTS;
    return scene::Aabb::fromCenter(center, halfExtents);
}

scene::Aabb pillarBox(const glm::vec3& position) {
    const glm::vec3 center = position + glm::vec3{0.0F, PILLAR_HEIGHT / 2.0F, 0.0F};
    return scene::Aabb::fromCenter(center, PILLAR_HALF_EXTENTS);
}
```

Pozycja segmentu i słupka to środek podstawy, a `Aabb::fromCenter` chce środka bryły, stąd przesunięcie o połowę wysokości w górę. Dół pudełka wypada wtedy dokładnie na wysokości pozycji: w `y = 0` dla pozycji prosto z funkcji układu, a na opuszczonej wysokości dla ściany postawionej na terenie. Funkcje nie wiedzą, skąd pozycja pochodzi. Komentarze w nagłówku mówią dlatego od M6 "standing on the position of the segment" i "whose base is at position", a nie "standing on the floor".

```cpp
std::vector<scene::Aabb> colliderBoxes(std::span<const WallSegment> walls,
                                       std::span<const glm::vec3> pillars) {
    std::vector<scene::Aabb> boxes;
    boxes.reserve(walls.size() + pillars.size());
    for (const WallSegment& segment : walls) {
        boxes.push_back(wallBox(segment));
    }
    for (const glm::vec3& position : pillars) {
        boxes.push_back(pillarBox(position));
    }
    return boxes;
}

std::vector<scene::Aabb> mazeColliders(const Maze& maze) {
    return colliderBoxes(wallSegments(maze), pillarPositions(maze));
}
```

Gotowa lista przeszkód dla `scene::moveAndSlide`: najpierw pudełka wszystkich ścian, potem wszystkich słupków. Podłoża na liście nie ma: wysokość gracza jest czytana z terenu, a nie wynikiem kolizji ([`player.md`](player.md), sekcja 2.5).

Do M5 była jedna funkcja, `mazeColliders(maze)`, która sama wołała `wallSegments` i `pillarPositions`. M6 wyjął z niej pętle do `colliderBoxes`, która przyjmuje **gotowe listy**. Powód: labirynt na terenie ma listy ścian i słupków z już opuszczonymi wysokościami (`MazeWorld::walls`, `MazeWorld::pillars`), a pudełka muszą powstać z nich, nie z siatki, bo siatka wysokości nie zna. `mazeColliders` została jako skrót "cały labirynt stojący na `y = 0`": w `src/` nikt jej dziś nie woła, używają jej testy.

| Linia | Znaczenie |
|---|---|
| `std::span<const WallSegment> walls`, `std::span<const glm::vec3> pillars` | widoki na ciągi elementów: przyjmują `std::vector` bez kopiowania. Stąd nowe `#include <span>` w nagłówku |
| `boxes.reserve(walls.size() + pillars.size());` | wynik ma znaną długość, więc wektor rezerwuje pamięć raz, zamiast rosnąć po kawałku |
| dwie pętle | pudełko każdej ściany (`wallBox`), potem każdego słupka (`pillarBox`), w kolejności list |
| `return colliderBoxes(wallSegments(maze), pillarPositions(maze));` | dwa tymczasowe wektory zamieniają się na `std::span` na czas wywołania |

Listę liczy się raz na teren i trzyma do następnej generacji albo zmiany skali wysokości: robi to `game::placeOnTerrain`, a wynik leży w `MazeWorld::colliders` ([`maze-rendering.md`](maze-rendering.md), sekcja 5). Jak korzysta z niej gracz, pokazuje [`../scene/collision.md`](../scene/collision.md), sekcja 5.6.

### 5.8 Jak to zostało sprawdzone

Testy jednostkowe w bibliotece doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)).

**Klasa `Maze`** (`tests/MazeTests.cpp`): kierunki zgodne z kompasem, nowy labirynt ma wszystkie ściany, `contains`, wyjątek dla rozmiarów 0, ujemnych i 257 (oraz brak wyjątku dla 1 i 256), usunięcie ściany znika z obu komórek i nie zmienia żadnej innej (22 z 24 boków zostają), ściana zewnętrzna nie ma drugiej strony, wyjątek `std::out_of_range` dla komórki spoza siatki. Od M5 dwa przypadki więcej (razem 8):

| Przypadek testowy | Co sprawdza |
|---|---|
| `a cell is a dead end when it has exactly three walls` | siatka 2 na 1. Świeża komórka z czterema ścianami nie jest ślepym zaułkiem. Po jednym przejściu między komórkami obie są (po trzy ściany). Po drugim otworze w prawej komórce lewa nadal jest, prawa już nie (dwie ściany: korytarz). Komórka `(2, 0)` spoza siatki rzuca `std::out_of_range`. Ten przypadek przyszedł z `tests/LightingTests.cpp` razem z funkcją |
| `two maze cells are equal when column and row match` | `MazeCell{2, 3}` równa się `MazeCell{2, 3}` i nie równa się `MazeCell{3, 2}`, a `MazeCell{}` to `(0, 0)` |

**Generator** (`tests/MazeGeneratorTests.cpp`):

| Przypadek testowy | Co sprawdza |
|---|---|
| `std::mt19937 gives the sequence the C++ standard promises` | dziesięciotysięczna liczba generatora domyślnego to 4123659995 |
| `randomBelow stays below the bound` | granica 1 daje zawsze 0, granica 3 daje w 3000 losowań każdą z wartości 0, 1, 2 (każdą częściej niż 750 i rzadziej niż 1500 razy), granica 0 rzuca wyjątek |
| `randomBelow gives the same numbers on every system` | wartości przypięte: dla ziarna 1 i granicy 3 pierwsze dwanaście wyników to 1, 2, 0, 2, 1, 1, 2, 2, 2, 0, 2, 0 |
| `generateMaze makes a perfect maze for every size and seed` | 8 rozmiarów (1 na 1, 1 na 7, 7 na 1, 2 na 2, 5 na 3, 3 na 5, 16 na 16, 31 na 20) razy 25 ziaren: wypełnianie zalewowe dociera do każdej komórki, a przejść jest `width * height - 1` |
| `generateMaze keeps the outer border closed` | te same 200 labiryntów: każda komórka brzegowa ma ścianę zewnętrzną |
| `neighbouring cells of a generated maze agree...` | te same 200 labiryntów: bok East komórki i bok West jej sąsiadki mówią to samo, tak samo South i North |
| `the same seed gives the same maze, another seed another one` | ziarno 2024 dwa razy daje identyczne ściany, ziarno 2025 inne, a 20 kolejnych ziaren daje 20 różnych labiryntów 8 na 8 |
| `the smallest mazes: one cell, one row, one column` | 1 na 1 zachowuje cztery ściany, 6 na 1 i 1 na 3 są prostym korytarzem dla każdego z 25 ziaren |
| `generateMaze rejects a wrong size` | wyjątek dla szerokości 0, wysokości 0, wartości ujemnych i 257 |
| `golden maze: 4 x 4 cells from seed 1 has exactly these walls` | labirynt przypięty co do ściany (niżej) |
| `the worked example of the documentation: 3 x 3 cells from seed 7` | rysunek z sekcji 2.4 |

**Labirynt wzorcowy** (golden test). Ten test istnieje po to, żeby udowodnić, że macOS i Windows generują ten sam labirynt. `generateMaze(4, 4, 1)` musi dać dokładnie to:

```text
+--+--+--+--+
|  |        |
+  +  +--+  +
|  |     |  |
+  +--+  +--+
|     |     |
+--+  +--+  +
|           |
+--+--+--+--+
```

Skąd wiadomo, że to dobry wzorzec, skoro powstał z uruchomienia programu: ten sam rysunek dał niezależny skrypt w Pythonie (własna implementacja Mersenne Twister według opisu algorytmu i ten sam przebieg po siatce), który potwierdził też wartość 4123659995, dwanaście wartości `randomBelow` i przykład z sekcji 2.4. Dwie niezależne implementacje zgadzają się co do każdej ściany.

**Układ** (`tests/MazeLayoutTests.cpp`): wartości stałych, `cellCenter(0, 0)` to `(1, 0, 1)`, a `cellCenter(3, 2)` to `(7, 0, 5)`. Zamknięta komórka 1 na 1 ma 4 segmenty i 4 słupki w oczekiwanych miejscach. Siatka 2 na 1 ma 7 segmentów, a ściana wspólna w `(2, 0, 1)` występuje raz. Po usunięciu ściany wspólnej zostaje 6 segmentów i nadal 6 słupków. Siatka 2 na 2 bez ścian wewnętrznych ma 8 segmentów i 8 słupków (brak środkowego), a z jedną ścianą wewnętrzną środkowy słupek wraca. Labirynty 9 na 6 dla 10 ziaren mają po 70 segmentów i 70 słupków. Stała `WALL_COLLISION_THICKNESS` jest równa `PILLAR_SIZE`. Pudełko ściany wzdłuż X w `(3, 0, 4)` sięga od `(2, 0, 3,85)` do `(4, 3, 4,15)`, pudełko słupka w `(2, 0, 6)` od `(1,85, 0, 5,85)` do `(2,15, 3,15, 6,15)`. Trzy ostatnie przypadki łączą układ z kolizjami i są opisane w [`../scene/collision.md`](../scene/collision.md), sekcja 5.7. Plik ma nadal 12 przypadków: `wallSegmentOn` ma test w `tests/ExitTests.cpp` (`wallSegmentOn gives the segment on each of the four sides of a cell`), a pośrednio sprawdza ją każdy test `wallSegments`, bo lista powstaje teraz przez tę funkcję.

**Wyniki.** Windows, 2026-10-05: wszystkie testy przechodzą w Debug i Release (256 przypadków i 101232 asercje w całym programie testowym po drugiej części M6, po M5 było to 215 i 85098, po pierwszej części M7 zgłoszone jest 269 i 102103, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751). Build bez ostrzeżeń to zgłoszenie autora kodu. W M6 trzy przypadki `MazeLayoutTests.cpp` zmieniły tylko nazwy, razem z komentarzami w kodzie: `cellCenter is the middle of the cell at y = 0`, `wallBox is 2 m long, 3 m high and 0.3 m thick, standing on its position` i `pillarBox is 0.3 m square and 3.15 m high, standing on its position`. Że labirynt wzorcowy jest ten sam w Debug i w Release, wynika z tego, że ten sam test przechodzi w obu. **Na macOS ten kod nie był jeszcze kompilowany ani uruchamiany.** Zgodność labiryntu między systemami jest więc na dziś uzasadniona (standard C++, niezależny skrypt), ale nie zmierzona: zmierzy ją pierwsze uruchomienie testów na Macu ([`../../guides/build-macos.md`](../../guides/build-macos.md), sekcja 2).

## 6. Panel ImGui

Panel **Maze** pozwala zamówić nowy labirynt i pokazuje ten, który jest w grze. Kod: [`src/debug/panels/MazePanel.cpp`](../../../src/debug/panels/MazePanel.cpp). Jak panel jest podpięty do `DebugUI`, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5. Co aplikacja robi z prośbą panelu (regeneracja), opisuje [`maze-rendering.md`](maze-rendering.md), sekcje 2 i 5.

PRD (sekcja 10) opisuje panel Maze jako "Seed, rozmiar, liczba kryształów, przycisk Regeneruj, widok siatki z góry". Od M5 jest wszystko: liczbę kryształów pokazuje linia `Crystals: %d, exit in cell (%d, %d)`, a kryształy, brama i strefa wyjścia są narysowane na planie. Nowy labirynt zamówiony w tym panelu zaczyna też nową rundę ([`maze-rendering.md`](maze-rendering.md), sekcja 2.7).

Na Windowsie (2026-10-05) plan ścian w panelu jest sprawdzony na zrzucie ekranu z M2 + M3: zgadza się z widokiem labiryntu z góry. Suwaków i przycisków nikt jeszcze nie kliknął ręcznie, a kryształów znikających z planu po zebraniu i bramy zmieniającej kolor po otwarciu nikt jeszcze nie oglądał w ręcznym teście.

### 6.1 Sygnatura i stałe

```cpp
void drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world,
                   const game::Round& round, const game::Player& player,
                   const scene::Camera& camera);
```

| Parametr | Dostęp | Do czego służy |
|---|---|---|
| `settings` | edycja | prośba o następny labirynt: panel zmienia rozmiar, ziarno i flagę `regenerate` |
| `world` | tylko odczyt | labirynt w grze: rozmiar, ziarno, liczniki, lista ścian do planu, a od M5 komórka wyjścia, brama, strefa wyjścia i liczba kryształów |
| `round` (od M5) | tylko odczyt | stan rundy: które kryształy są już zebrane i czy brama jeszcze stoi na drodze. Od tego zależy, jak plan rysuje kryształy i bramę |
| `player` | tylko odczyt | pozycja kropki na planie |
| `camera` | tylko odczyt | yaw: kierunek kreski przy kropce |

Z sygnatury widać najważniejszą własność panelu: **nie może zmienić labiryntu ani rundy**. Ma do nich stałe referencje. Może tylko zapisać prośbę. Komentarz w nagłówku kończy się zdaniem "A new maze also starts a new round": prośba o labirynt jest tym samym prośbą o nową rundę, bo `regenerateMaze` kończy się wywołaniem `beginRound`.

Podział danych między `world` i `round` jest ten sam co w całej grze: **gdzie** leżą kryształy i brama, wynika z labiryntu (`MazeWorld`), a **w jakim są stanie**, wie runda (`Round`).

```cpp
// Limits of the size sliders, in cells. The game draws every wall and pillar with its
// own draw call, about two per cell, and the terrain has 32 triangles per cell, so a much
// larger maze would make the frame slow. game::Maze itself accepts up to Maze::MAX_SIZE.
constexpr int MIN_MAZE_SIZE = 2;
constexpr int MAX_MAZE_SIZE = 40;

// The seed field changes by this much for one click on its + or - button.
constexpr std::uint32_t SEED_STEP = 1;

// Sizes on the plan, in pixels: the dot of the player and the line that shows where the
// camera looks.
constexpr float PLAYER_DOT_RADIUS = 3.0F;
constexpr float HEADING_LENGTH = 10.0F;

// A crystal on the plan is a dot of this radius, and the gate a line of this thickness,
// both in pixels. The walls are 1 pixel thick, so the gate stands out among them.
constexpr float CRYSTAL_DOT_RADIUS = 2.5F;
constexpr float GATE_LINE_THICKNESS = 3.0F;

// Free pixels around the plan, so that the border walls are not drawn on the very edge
// of the reserved area.
constexpr float PLAN_PADDING = 4.0F;
```

| Stała | Znaczenie |
|---|---|
| `MAZE_PLACEMENT` (z [`PanelLayout.hpp`](../../../src/debug/PanelLayout.hpp), nie z tego pliku) | miejsce i rozmiar przy pierwszym uruchomieniu: prawy górny róg okna. Wpis w `imgui.ini` ma pierwszeństwo ([`../debug-ui.md`](../debug-ui.md), sekcja 5) |
| `MIN_MAZE_SIZE = 2`, `MAX_MAZE_SIZE = 40` | granice suwaków rozmiaru. Górna jest granicą wygody, nie poprawności: `Maze` przyjmuje do 256, ale gra rysuje każdą ścianę i każdy słupek osobnym wywołaniem (około dwóch na komórkę), a teren ma 32 trójkąty na komórkę. 40 na 40 to już 3363 wywołania dla labiryntu z terenem, 3380 z bramą i 16 kryształami, i 93312 trójkąty terenu ([`maze-rendering.md`](maze-rendering.md), sekcja 2.5). Do M5 komentarz mówił o trzech wywołaniach na komórkę, bo trzecim była płytka podłogi. Dolnej granicy 2 komentarz w kodzie nie uzasadnia. Moje odczytanie: suwak pomija labirynt z jedną komórką i korytarze o szerokości jednej komórki, których używają testy, a które do pokazu się nie nadają |
| `SEED_STEP = 1` | o ile zmienia ziarno kliknięcie w przycisk plus albo minus obok pola. Typ musi być taki sam jak typ ziarna |
| `PLAN_WALL_COLOR`, `PLAN_PLAYER_COLOR` (z [`Theme.hpp`](../../../src/debug/Theme.hpp), nie z tego pliku) | kolory planu jako kolory motywu: ściany w bladym kolorze kamienia w świetle księżyca `colorFromBytes(176, 190, 216)`, gracz w bursztynie latarki `colorFromBytes(255, 184, 84)` ([`../debug-ui.md`](../debug-ui.md), sekcja 5) |
| `PLAN_CRYSTAL_COLOR`, `PLAN_COLLECTED_COLOR`, `PLAN_GATE_COLOR`, `PLAN_EXIT_COLOR` (od M5, też z `Theme.hpp`) | kolory rzeczy rundy na planie, tabela niżej |
| `PLAYER_DOT_RADIUS`, `HEADING_LENGTH`, `PLAN_PADDING` | rozmiary w pikselach ekranu, niezależne od skali planu: kropka gracza ma zawsze 3 piksele promienia, nawet w labiryncie 40 na 40 |
| `CRYSTAL_DOT_RADIUS = 2.5F` (od M5) | promień kropki kryształu w pikselach, stały niezależnie od skali planu. Jest o pół piksela mniejszy niż promień kropki gracza (3). Komentarz w kodzie tej różnicy nie uzasadnia. Moje odczytanie: gracz ma być na planie najwyraźniejszą kropką |
| `GATE_LINE_THICKNESS = 3.0F` (od M5) | grubość linii bramy w pikselach. Ściany mają 1 piksel, więc brama wyróżnia się wśród nich grubością, a nie tylko kolorem |

Kolory planu w `Theme.hpp`:

| Stała | Wartość | Co rysuje | Uzasadnienie z komentarza |
|---|---|---|---|
| `PLAN_WALL_COLOR` | `colorFromBytes(176, 190, 216)` | ściany | blady kamień w świetle księżyca |
| `PLAN_PLAYER_COLOR` | `colorFromBytes(255, 184, 84)` | kropka gracza i kreska kierunku | ciepłe światło latarki |
| `PLAN_CRYSTAL_COLOR` | `colorFromBytes(86, 214, 202)` | kryształ, który jeszcze wisi | cyjan, którym kryształy świecą. Ta sama wartość to `HUD_CRYSTAL_COLOR` paska HUD |
| `PLAN_COLLECTED_COLOR` | `colorFromBytes(44, 104, 112)` | kryształ już zebrany i otwarta brama | ciemny ślad miejsca, w którym coś było |
| `PLAN_GATE_COLOR` | `colorFromBytes(214, 142, 82)` | zamknięta brama | brąz jej drewna, rozjaśniony tak, żeby odcinał się od ścian |
| `PLAN_EXIT_COLOR` | `colorFromBytes(132, 220, 140)` | obrys strefy wyjścia | miękka zieleń: "tędy do wyjścia" |

Kolory są stałymi motywu, a nie liczbami wpisanymi w panelu, z tego samego powodu co dwa starsze: plan i reszta interfejsu mają jedną paletę ([`../debug-ui.md`](../debug-ui.md)).

### 6.2 Widżety: prośba o nowy labirynt

```cpp
    // First run only: the top right corner of the window (the constant is in
    // PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(MAZE_PLACEMENT);
    if (ImGui::Begin("Maze")) {
        // The three widgets edit the request, not the maze: nothing happens until one of
        // the buttons sets settings.regenerate.
        ImGui::SliderInt("Width", &settings.width, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderInt("Height", &settings.height, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                         ImGuiSliderFlags_AlwaysClamp);
        // InputScalar edits a number of any type through a pointer: the type is named by
        // the second argument and must match the variable, here a 32 bit unsigned.
        ImGui::InputScalar("Seed", ImGuiDataType_U32, &settings.seed, &SEED_STEP);
```

| Linia | Znaczenie |
|---|---|
| `ImGui::SliderInt("Width", &settings.width, ...)` | suwak liczb całkowitych: liczba kolumn (komórek wzdłuż X). `"%d cells"` to format wyświetlanej wartości |
| `ImGui::SliderInt("Height", &settings.height, ...)` | liczba wierszy (komórek wzdłuż Z). "Height" to wysokość siatki na planie, a nie wysokość ścian |
| `ImGuiSliderFlags_AlwaysClamp` | wartość wpisana z klawiatury (Ctrl i kliknięcie) też jest przycinana do granic suwaka |
| `ImGui::InputScalar("Seed", ImGuiDataType_U32, &settings.seed, &SEED_STEP)` | pole liczbowe z przyciskami minus i plus. ImGui nie ma osobnego widżetu dla `std::uint32_t`, więc używam ogólnego: drugi argument nazywa typ danych, trzeci to wskaźnik na zmienną, czwarty wskaźnik na krok. Typ z drugiego argumentu **musi** zgadzać się z typem zmiennej, bo ImGui czyta i zapisuje przez goły wskaźnik ([`../../libraries/imgui.md`](../../libraries/imgui.md)) |

Te trzy widżety piszą do `MazeSettings` i na tym koniec. Labirynt w grze się nie zmienia.

```cpp
        if (ImGui::Button("Regenerate")) {
            settings.regenerate = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Random seed")) {
            // std::random_device asks the operating system for a number that cannot be
            // predicted. It only picks the seed: the maze itself is still built by the
            // seeded generator, so writing the seed down brings the same maze back.
            std::random_device device;
            settings.seed = static_cast<std::uint32_t>(device());
            settings.regenerate = true;
        }
```

| Linia | Znaczenie |
|---|---|
| `if (ImGui::Button("Regenerate"))` | `Button` zwraca prawdę tylko w klatce kliknięcia |
| `settings.regenerate = true;` | cała praca przycisku: flaga. Aplikacja odczyta ją na początku następnej klatki, zbuduje labirynt i ją wyzeruje |
| `ImGui::SameLine();` | następny widżet staje w tej samej linii, obok |
| `std::random_device device;` | źródło liczb nieprzewidywalnych, dostarczane przez system operacyjny. To jedyne miejsce w projekcie, gdzie losowość nie pochodzi z ziarna |
| `settings.seed = static_cast<std::uint32_t>(device());` | wylosowane ziarno trafia do pola `Seed`, więc widać je w panelu i można je zapisać |

Podział ról jest ważny i łatwo go pomylić: `std::random_device` wybiera **które** ziarno, a labirynt z tego ziarna buduje nadal deterministyczny `generateMaze` (sekcja 2.6). Po kliknięciu `Random seed` w polu stoi liczba, która odtworzy ten sam labirynt na każdym komputerze.

```cpp
        ImGui::Separator();
        // The maze in play, which may differ from the request above until a button
        // is clicked.
        ImGui::Text("In play: %d x %d cells, seed %u", world.maze.width(), world.maze.height(),
                    static_cast<unsigned int>(world.seed));
        ImGui::Text("Walls: %d, pillars: %d", static_cast<int>(world.walls.size()),
                    static_cast<int>(world.pillars.size()));
        // The crystals are chosen from the seed together with the maze. How many of them
        // open the gate is a rule of the round (the Gameplay panel).
        ImGui::Text("Crystals: %d, exit in cell (%d, %d)", static_cast<int>(world.crystals.size()),
                    world.exitCell.x, world.exitCell.z);

        drawPlan(world, round, player, camera);
    }
    ImGui::End();
```

Trzy linie tekstu czytają `world`, a nie `settings`: pokazują labirynt, który naprawdę jest w grze. Na starcie to `In play: 10 x 10 cells, seed 1`, `Walls: 121, pillars: 121` i `Crystals: 13, exit in cell (6, 5)`. `%u` oczekuje `unsigned int`, stąd rzutowanie ziarna.

Trzecia linia (od M5) pokazuje **liczbę kryształów labiryntu** (`world.crystals.size()`), a nie liczbę zebranych ani potrzebnych do otwarcia bramy. Komentarz mówi dlaczego: kryształy są losowane z ziarna razem z labiryntem, więc należą do tego panelu, a to, ile z nich otwiera bramę, jest regułą rundy i stoi w panelu Gameplay oraz na pasku HUD ([`gameplay.md`](gameplay.md), sekcja 6). Komórka wyjścia to para (kolumna, wiersz) w tej samej numeracji co w całym dokumencie: `(0, 0)` to róg północno-zachodni, lewy górny na planie.

### 6.3 Plan z góry: `drawPlan`

Plan to rysunek techniczny labiryntu: każda ściana jako odcinek, gracz jako kropka, a od M5 także strefa wyjścia jako prostokąt, brama jako gruba linia i kryształy jako kropki. Nie jest teksturą ani osobnym renderem sceny. Rysuje go ImGui, z tej samej listy `world.walls`, z której powstają macierze modeli i pudełka kolizji, i z tych samych pól `MazeWorld` i `Round`, z których korzysta gra.

**Układ planu.** Północ (-Z) jest u góry:

```text
świat widziany z góry                  ekran (panel)

  -Z (północ)                           origin
   ^                                      +--------> x ekranu  =  x świata * scale
   |                                      |
   +-----> +X (wschód)                    v
                                          y ekranu  =  z świata * scale
```

Oś x świata biegnie w prawo ekranu. Oś z świata rośnie na południe, a oś y ekranu rośnie w dół, więc południe wypada na dole planu **bez odwracania znaku**. To szczęśliwy skutek konwencji projektu (północ to -Z).

```cpp
void drawPlan(const game::MazeWorld& world, const game::Round& round, const game::Player& player,
              const scene::Camera& camera) {
    // Size of the maze in metres.
    const float worldWidth = static_cast<float>(world.maze.width()) * game::CELL_SIZE;
    const float worldDepth = static_cast<float>(world.maze.height()) * game::CELL_SIZE;

    // The plan is as wide as the panel allows. One scale (pixels per metre) for both axes
    // keeps the cells square. The longer side of the maze decides it.
    const float availableWidth = ImGui::GetContentRegionAvail().x - 2.0F * PLAN_PADDING;
    const float scale = std::max(availableWidth, 1.0F) / std::max(worldWidth, worldDepth);
```

| Linia | Znaczenie |
|---|---|
| `worldWidth`, `worldDepth` | rozmiar labiryntu w metrach: liczba komórek razy 2 m. Dla 10 na 10 to 20 na 20 m |
| `ImGui::GetContentRegionAvail().x` | ile pikseli szerokości zostało w panelu na zawartość. Plan dopasowuje się do szerokości panelu: po rozciągnięciu panelu plan rośnie |
| `- 2.0F * PLAN_PADDING` | odstęp po obu stronach |
| `std::max(availableWidth, 1.0F)` | panel zwężony prawie do zera dałby szerokość ujemną i plan odbity. Jeden piksel to bezpieczne minimum |
| `/ std::max(worldWidth, worldDepth)` | **jedna skala** w pikselach na metr dla obu osi, żeby komórki zostały kwadratowe. Decyduje dłuższy bok labiryntu, więc plan zawsze mieści się w szerokości |

Przykład: panel o szerokości 300 pikseli ma około 284 pikseli na zawartość (reszta to marginesy okna ImGui, wartość orientacyjna). Po odjęciu odstępów zostaje 276, a skala dla labiryntu 20 m wychodzi 13,8 piksela na metr.

```cpp
    // The top left corner of the plan on the screen. The cursor is the place where ImGui
    // would put the next widget.
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 origin{cursor.x + PLAN_PADDING, cursor.y + PLAN_PADDING};

    // Turns a point of the world (x and z, the height does not matter from above) into
    // a point on the screen. A lambda: a small function that can use scale and origin.
    const auto toScreen = [scale, origin](const glm::vec3& point) {
        return ImVec2{origin.x + point.x * scale, origin.y + point.z * scale};
    };

    // The draw list of the panel takes shapes in screen coordinates. They are drawn
    // with the panel and clipped to it.
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // A draw list takes a colour packed into one 32 bit number. GetColorU32 packs a
    // colour of the theme (Theme.hpp) and applies the opacity of the style to it.
    const ImU32 wallColor = ImGui::GetColorU32(PLAN_WALL_COLOR);
    const ImU32 playerColor = ImGui::GetColorU32(PLAN_PLAYER_COLOR);
    const ImU32 crystalColor = ImGui::GetColorU32(PLAN_CRYSTAL_COLOR);
    const ImU32 collectedColor = ImGui::GetColorU32(PLAN_COLLECTED_COLOR);
    const ImU32 gateColor = ImGui::GetColorU32(PLAN_GATE_COLOR);
    const ImU32 exitColor = ImGui::GetColorU32(PLAN_EXIT_COLOR);
```

| Linia | Znaczenie |
|---|---|
| `ImGui::GetCursorScreenPos()` | "kursor" ImGui to nie kursor myszy, tylko miejsce, w którym stanąłby następny widżet, we współrzędnych ekranu. Tam zaczyna się plan |
| `origin` | lewy górny róg planu: punkt świata `(0, _, 0)`, czyli północno-zachodni narożnik labiryntu |
| `[scale, origin](const glm::vec3& point) { ... }` | lambda: mała funkcja zdefiniowana w miejscu użycia. W nawiasach kwadratowych stoi lista przechwyceń: lambda dostaje kopie `scale` i `origin` i może ich używać w środku |
| `origin.x + point.x * scale`, `origin.y + point.z * scale` | przeliczenie metrów na piksele. Składowa y punktu (wysokość) jest pomijana: z góry jej nie widać |
| `ImGui::GetWindowDrawList()` | lista rysowania bieżącego panelu. Przyjmuje kształty we współrzędnych ekranu. Są rysowane razem z panelem i przycinane do jego obszaru |
| `ImGui::GetColorU32(PLAN_WALL_COLOR)` | lista rysowania przyjmuje kolor jako jedną liczbę 32-bitową (`ImU32`: po jednym bajcie na czerwony, zielony, niebieski i alfę), a stałe motywu to cztery liczby `float` (`ImVec4`). Funkcja przelicza jedno na drugie i mnoży alfę przez przezroczystość ze stylu, więc plan wewnątrz wyszarzonego bloku też byłby wyszarzony. Wynik zależy od stylu, dlatego jest liczony w funkcji, a nie jako stała `constexpr` |

```cpp
    constexpr float HALF_WALL = game::WALL_LENGTH / 2.0F;
    const auto halfLineOf = [](const game::WallSegment& segment) {
        return segment.axis == game::WallAxis::AlongX ? glm::vec3{HALF_WALL, 0.0F, 0.0F}
                                                      : glm::vec3{0.0F, 0.0F, HALF_WALL};
    };
    for (const game::WallSegment& wall : world.walls) {
        const glm::vec3 halfLine = halfLineOf(wall);
        drawList->AddLine(toScreen(wall.position - halfLine), toScreen(wall.position + halfLine),
                          wallColor);
    }
```

| Linia | Znaczenie |
|---|---|
| `HALF_WALL` | połowa długości segmentu: 1 m |
| `halfLineOf` (od M5) | druga lambda. Lista przechwyceń jest pusta (`[]`), bo `HALF_WALL` to stała `constexpr`, której lambda może użyć bez przechwytywania. Do M4 to wyrażenie stało wprost w pętli. Zostało wyjęte, bo od M5 potrzebują go dwa miejsca: ściany i brama |
| `halfLine` | wektor od środka segmentu do jego końca: wzdłuż X albo wzdłuż Z, zależnie od osi segmentu |
| `wall.position - halfLine`, `wall.position + halfLine` | dwa końce segmentu w świecie: dwa sąsiednie węzły siatki |
| `drawList->AddLine(a, b, kolor)` | odcinek o grubości jednego piksela |

Segment w `(3, 0, 4)` o osi `AlongX` to odcinek od `(2, 0, 4)` do `(4, 0, 4)`. Słupków na planie nie ma: byłyby kropkami w każdym węźle i tylko zaciemniałyby rysunek.

Dalej idą trzy rzeczy rundy (od M5), w tej kolejności: strefa wyjścia, brama, kryształy. Kolejność rysowania to kolejność przykrywania: co narysowane później, leży na wierzchu. Gracz jest rysowany na końcu, więc jego kropka przykrywa wszystko.

```cpp
    // The exit zone: the outline of the box that wins the round, seen from above. The
    // min and max corners of the box are its north-west and south-east corners here.
    drawList->AddRect(toScreen(world.exitZone.min), toScreen(world.exitZone.max), exitColor);
```

| Fragment | Znaczenie |
|---|---|
| `world.exitZone` | pudełko `scene::Aabb` w środku komórki wyjścia, 1 na 1 m (`game::exitZone`) |
| `toScreen(world.exitZone.min)`, `toScreen(world.exitZone.max)` | `min` ma najmniejsze x i z, czyli z góry to narożnik północno-zachodni, a na ekranie lewy górny. `max` to narożnik południowo-wschodni, prawy dolny. Wysokość (y) lambda pomija |
| `drawList->AddRect(a, b, kolor)` | sam obrys prostokąta między dwoma przeciwległymi narożnikami, linią o grubości 1 piksela. Wypełniony byłby `AddRectFilled` |

Dla labiryntu startowego strefa to kwadrat od `(12,5, 10,5)` do `(13,5, 11,5)` w metrach: metr na metr w środku komórki (6, 5), która zajmuje obszar od `(12, 10)` do `(14, 12)`.

```cpp
    // The gate: a thick line where it stands, in the colour of wood while it is closed
    // and dim once it has opened. A maze of one cell has none.
    if (world.hasGate) {
        const glm::vec3 halfLine = halfLineOf(world.gate);
        drawList->AddLine(
            toScreen(world.gate.position - halfLine), toScreen(world.gate.position + halfLine),
            game::gateBlocks(world, round) ? gateColor : collectedColor, GATE_LINE_THICKNESS);
    }
```

| Fragment | Znaczenie |
|---|---|
| `if (world.hasGate)` | labirynt z jedną komórką nie ma bramy. Pole `world.gate` nic wtedy nie znaczy, więc nie wolno go rysować |
| `halfLineOf(world.gate)` | brama to `WallSegment`, więc jej odcinek liczy ta sama lambda co odcinki ścian |
| `game::gateBlocks(world, round) ? gateColor : collectedColor` | kolor zależy od **reguły**, a nie od animacji: brama jest w kolorze drewna, dopóki zatrzymuje gracza, i przygasa w tej samej chwili, w której jej pudełko znika z listy przeszkód. Model w scenie zapada się potem jeszcze przez 1,5 s. Plan odpowiada więc na pytanie "czy da się przejść", tą samą funkcją, której używa gra |
| ostatni argument `GATE_LINE_THICKNESS` | czwarty, opcjonalny parametr `AddLine`: grubość w pikselach. Ściany go nie podają i mają 1 piksel |

```cpp
    // The crystals: a bright dot for one that is still there, a dim ring where one was
    // collected.
    for (const game::RoundCrystal& crystal : round.crystals) {
        const ImVec2 place = toScreen(crystal.restPosition);
        if (crystal.collected) {
            drawList->AddCircle(place, CRYSTAL_DOT_RADIUS, collectedColor);
        } else {
            drawList->AddCircleFilled(place, CRYSTAL_DOT_RADIUS, crystalColor);
        }
    }
```

| Fragment | Znaczenie |
|---|---|
| `round.crystals` | pętla idzie po kryształach **rundy**, nie labiryntu: tylko runda wie, które są zebrane |
| `crystal.restPosition` | miejsce spoczynku kryształu: środek jego komórki, 0,9 m nad gruntem. Wysokość lambda pomija, więc kropka wypada w środku komórki. Kryształ w scenie unosi się i opada wokół tego punktu, kropka na planie stoi |
| `AddCircleFilled(place, CRYSTAL_DOT_RADIUS, crystalColor)` | kryształ, który jeszcze wisi: wypełniona kropka w kolorze kryształów |
| `AddCircle(place, CRYSTAL_DOT_RADIUS, collectedColor)` | kryształ zebrany: sam okrąg (pierścień) w ciemnym kolorze. Miejsce zostaje widoczne, więc z planu da się odczytać, gdzie gracz już był |

Plan jest narzędziem debugowania i pokazuje wszystko od razu: wszystkie kryształy i wyjście, także te, do których gracz jeszcze nie doszedł. To nie jest mapa dla gracza.

```cpp
    // The player: a dot, and a line towards where the camera looks. Yaw 0 looks north
    // (up on the plan) and grows clockwise, so the direction on the screen is
    // (sin yaw, -cos yaw): the same formula as the x and z of Camera::forward.
    const ImVec2 dot = toScreen(player.position);
    const float yaw = glm::radians(camera.yawDegrees);
    const ImVec2 headingEnd{dot.x + std::sin(yaw) * HEADING_LENGTH,
                            dot.y - std::cos(yaw) * HEADING_LENGTH};
    drawList->AddLine(dot, headingEnd, playerColor);
    drawList->AddCircleFilled(dot, PLAYER_DOT_RADIUS, playerColor);
```

| Linia | Znaczenie |
|---|---|
| `toScreen(player.position)` | stopy gracza na planie. Wysokość jest pomijana, więc w trybie noclip kropka pokazuje punkt pod graczem |
| `glm::radians(camera.yawDegrees)` | funkcje `std::sin` i `std::cos` przyjmują radiany |
| `std::sin(yaw)`, `-std::cos(yaw)` | kierunek patrzenia w poziomie: te same wzory co składowe x i z w `Camera::forward` przy pitch 0. Yaw 0 daje `(0, -1)`, czyli w górę ekranu (północ). Yaw 90 daje `(1, 0)`, w prawo (wschód) |
| `* HEADING_LENGTH` | kreska ma stałą długość 10 pikseli, niezależnie od skali |
| `AddLine`, potem `AddCircleFilled` | najpierw kreska, potem kropka, żeby kropka przykryła początek kreski |

Minus przy kosinusie nie jest odwróceniem osi ekranu. To ten sam minus, który stoi w `Camera::forward`: yaw 0 patrzy wzdłuż **-Z**.

```cpp
    // The draw list does not move the cursor. Dummy is an invisible widget of the given
    // size: it reserves the area of the plan, so the panel knows how tall its contents
    // are and scrolls correctly.
    ImGui::Dummy(
        {worldWidth * scale + 2.0F * PLAN_PADDING, worldDepth * scale + 2.0F * PLAN_PADDING});
}
```

Lista rysowania działa obok zwykłego układu widżetów: kształty pojawiają się na ekranie, ale ImGui nie wie, że zajmują miejsce. `ImGui::Dummy` to niewidzialny widżet o podanym rozmiarze. Rezerwuje prostokąt planu razem z odstępami, dzięki czemu panel zna wysokość swojej zawartości i w razie potrzeby pokazuje pasek przewijania.

### 6.4 Co pokazać na obronie

Kroki z klikaniem nie były jeszcze wykonane ręcznie. Opisują to, co wynika z kodu.

1. **Plan a świat.** Pokazuję plan i mówię: to lista `walls`, ta sama, z której powstają macierze modeli i pudełka kolizji. Kropka to gracz, kreska to yaw kamery.
2. **Kompas.** Obracam kamerę myszą: kreska obraca się w tę samą stronę. Przy yaw 0 wskazuje górę planu, czyli północ, czyli -Z.
3. **Widok z góry.** Naciskam N, wzlatuję i patrzę w dół. Układ ścian w scenie i na planie jest ten sam. Od trzeciej części M7 najpierw wyłączam pole `Fog` w zakładce `Fog and vignette` panelu Framebuffers: mgła bierze wysokość w samym pikselu, więc z dużej wysokości zakrywa podłoże prawie całe (według wzoru około 95 procent z 30 m, [`../renderer/post-process.md`](../renderer/post-process.md), sekcja 2.20).
4. **Determinizm.** Wpisuję rozmiar 4 na 4 i ziarno 1, klikam `Regenerate`. Powstaje labirynt wzorcowy z testu `golden maze: 4 x 4 cells from seed 1 has exactly these walls`: porównuję plan z rysunkiem w `tests/MazeGeneratorTests.cpp`. Liczniki pokazują 25 ścian i 25 słupków.
5. **Prośba a stan.** Przesuwam `Width` na 20 i nie klikam: linia `In play` nadal pokazuje stary rozmiar. Klikam `Regenerate`: zmienia się.
6. **Losowe ziarno.** Klikam `Random seed` kilka razy. Za każdym razem inny labirynt, a jego ziarno stoi w polu `Seed`. Zapisuję jedno, klikam dalej, wpisuję zapisane i `Regenerate`: wraca ten sam labirynt.
7. **Własności.** Na dowolnym planie pokazuję, że nie ma zamkniętych obszarów ani pętli: labirynt doskonały (sekcja 2.3).
8. **Runda na planie.** Pokazuję linię `Crystals: 13, exit in cell (6, 5)` i na planie: 13 cyjanowych kropek, zielony kwadrat strefy wyjścia w komórce (6, 5) i grubą brązową linię bramy na jej wschodnim boku. Zwracam uwagę, że kropki siedzą przede wszystkim w ślepych zaułkach (tam `placeCrystals` kładzie kryształy najpierw, a pozostałe komórki dostają je dopiero, gdy zaułków zabraknie) i że wyjście nie leży w rogu, tylko tam, dokąd droga od startu jest najdłuższa.
9. **Stan rundy.** Zbieram kryształ: jego kropka zmienia się w ciemny pierścień. Po zebraniu tylu, ile trzeba (na starcie 10 z 13), linia bramy przygasa.

## 7. Pułapki

1. **Rozkłady z biblioteki standardowej.** `std::uniform_int_distribution` i `std::shuffle` dają inne wyniki w bibliotece clanga i w bibliotece MSVC przy tym samym generatorze i ziarnie. Wstawienie któregokolwiek do generatora psuje zgodność labiryntów między systemami, a na jednym systemie wszystko wygląda dobrze. Wykryje to dopiero test labiryntu wzorcowego uruchomiony na drugim systemie.
2. **Kolejność kierunków i komórka startowa są częścią formatu.** Zmiana kolejności w `ALL_DIRECTIONS`, zmiana `START_COLUMN` albo dodanie jednego losowania "po drodze" zmienia **każdy** labirynt dla każdego ziarna. Test labiryntu wzorcowego wtedy nie przechodzi i taki jest jego cel: zmiana ma być świadoma.
3. **`generator() % n` bez odrzucania.** Daje wynik lekko nierówny (sekcja 2.6). Dla małych `n` błąd jest niewidoczny, więc łatwo uznać ten zapis za dobry i przenieść go tam, gdzie `n` jest duże.
4. **Kopia generatora.** Parametr `std::mt19937 generator` zamiast `std::mt19937& generator` kopiuje stan. Funkcja losuje wtedy z kopii, a oryginał stoi w miejscu i następne wywołanie dostaje te same liczby.
5. **Północ to -Z i wiersz o numerze mniejszym.** Krok na północ zmniejsza `z`. Na rysunku z góry północ jest u góry, ale w świecie gry to kierunek "do przodu" kamery z yaw 0. Pomylenie znaku w `ROW_STEPS` daje labirynt odbity lustrzanie, który nadal jest poprawnym labiryntem, więc testy własności tego nie złapią. Złapie to test kierunków i labirynt wzorcowy.
6. **Jedna grubość dla modelu i dla kolizji.** Tak było na początku: 0,2 m dla obu. Pudełko słupka (0,3 m) wystawało wtedy 5 cm przed pudełko ściany i gracz idący przy ścianie stawał na słupku co 2 m. Dziś są dwie stałe: `WALL_VISUAL_THICKNESS` dla modelu i `WALL_COLLISION_THICKNESS`, równa `PILLAR_SIZE`, dla kolizji (sekcja 2.7). Kto zmieni bok słupka, zmienia tym samym grubość pudełek ścian. Kto wpisze przy pudełku ściany własną liczbę zamiast `PILLAR_SIZE`, przywraca zahaczanie o słupki, a wykryją to testy ślizgania w `MazeLayoutTests.cpp` i `PlayerTests.cpp`.
7. **Pozycja segmentu to nie środek pudełka.** `WallSegment::position` i wyniki `pillarPositions` to środek podstawy (prosto z funkcji układu na `y = 0`, w grze na opuszczonej wysokości terenu), bo tam jest początek układu modelu. Środek pudełka kolizji jest o połowę wysokości wyżej i liczą go `wallBox` oraz `pillarBox`. Podanie pozycji segmentu wprost do `Aabb::fromCenter` zakopałoby połowę pudełka pod gruntem.
8. **Obrót ściany `AlongZ`.** Model jest zbudowany wzdłuż X. Dla segmentu `AlongZ` trzeba go obrócić o 90 stopni wokół Y: robi to `wallModelMatrix` w `MazeWorld.cpp` ([`maze-rendering.md`](maze-rendering.md), sekcja 5). Ta sama funkcja ustawia bramę. Kierunek obrotu (90 albo -90) nie ma znaczenia dla pudełka kolizji i, przy modelu symetrycznym względem środka, także dla obrazu.
9. **`hasWall` rzuca wyjątek dla komórki spoza siatki.** Kod, który zagląda do sąsiadów, musi najpierw zapytać `contains`. Tak robią generator i `cellHasWall`.
10. **Usunięcie ściany zewnętrznej otwiera labirynt.** `removeWall` na boku brzegowym jest dozwolone przez klasę, ale gra nigdy tego nie robi. Generator nie, bo sąsiad spoza siatki nie jest kandydatem. Wyjście z M5 też nie: jest komórką wewnątrz zamkniętego muru, a nie otworem w nim. Otwór w brzegu prowadziłby donikąd: przeszukiwanie w `passageDistances` nie wychodzi poza siatkę (przypina to test `a cell behind walls is unreachable, and an opening in the border leads nowhere` w `tests/ExitTests.cpp`).
11. **Wersja rekurencyjna.** Przepisanie generatora na funkcję wołającą samą siebie działa dla małych labiryntów i kończy program przepełnieniem stosu dla dużych (sekcja 2.5).
12. **To samo ziarno, inny rozmiar.** Ziarno 7 dla 3 na 3 i dla 4 na 4 daje dwa niezwiązane labirynty: już pierwsze kroki napotykają inne zestawy kandydatów.
13. **`std::vector<bool>` to nie zwykły wektor.** Przechowuje flagi jako pojedyncze bity, a `visited[i]` zwraca obiekt pośredniczący, nie `bool&`. Do odczytu i zapisu przez indeks, jak w generatorze, działa normalnie. Nie da się natomiast wziąć adresu ani referencji do elementu.
14. **Pudełko kolizji nie ma wymiarów modelu.** Pudełko ściany (0,3 m) jest grubsze niż jej korpus (0,2 m) i cokół (0,28 m), a pudełko słupka (0,3 m) węższe niż jego podstawa (0,4 m). Gracz staje więc 5 cm przed korpusem ściany, a podstawa słupka może wejść 5 cm w jego pudełko. To świadome uproszczenie: jedno pudełko na obiekt, z licami w jednej płaszczyźnie.
15. **Suwaki panelu nie zmieniają labiryntu.** `Width`, `Height` i `Seed` edytują prośbę (`MazeSettings`), a nie labirynt w grze. Bez kliknięcia `Regenerate` nic się nie dzieje. Linia `In play` pokazuje, co naprawdę jest w grze.
16. **Plan narysowany z odwróconą osią.** Na planie północ (-Z) jest u góry. Oś y ekranu rośnie w dół i oś z świata rośnie na południe, więc z przelicza się na y bez zmiany znaku. Odruchowe odwrócenie osi "bo w OpenGL y rośnie w górę" dałoby plan odbity w pionie, który nadal wygląda jak poprawny labirynt.
17. **Lista rysowania nie przesuwa kursora.** Kształty dodane przez `ImDrawList` nie zajmują miejsca w układzie panelu. Bez `ImGui::Dummy` o rozmiarze planu następny widżet stanąłby na planie, a panel nie wiedziałby, jak wysoką ma zawartość.
18. **`std::random_device` w generatorze.** W panelu losuje **ziarno**, i tylko tam. Wstawiony do `generateMaze` zniszczyłby powtarzalność: tego samego labiryntu nie dałoby się odtworzyć.
19. **`wallSegmentOn` nie sprawdza ściany.** Funkcja odda segment dla każdego boku każdej komórki, także otwartego i także dla komórki spoza siatki (to czysty wzór, jak `cellCenter`). Pytanie "czy tam stoi ściana" zadaje ten, kto ją woła: `wallSegments` przez `hasWall`. Brama korzysta właśnie z tego, że pytania nie ma.
20. **Ślepy zaułek to dokładnie trzy ściany.** Warunek `>= 3` uznałby za ślepy zaułek zamkniętą komórkę labiryntu 1 na 1 (cztery ściany), do której nie ma wejścia.
21. **Plan rysowany z `world.crystals` zamiast z `round.crystals`.** Lista w `MazeWorld` mówi tylko, gdzie kryształy są na początku. Plan rysowany z niej nigdy nie pokazałby zebranego kryształu.
22. **Brama na planie według animacji.** Kolor bramy wybiera `gateBlocks`, a nie `gateVisible`. Przez 1,5 s po otwarciu model jeszcze widać w scenie, ale przejść już można, i plan pokazuje właśnie to drugie.

## 8. Ćwiczenia

Ćwiczenia od 1 do 6 oraz 11 i 12 robi się na kartce. Ćwiczenia od 7 do 10 to zmiany w kodzie albo w testach: po każdej zbuduj projekt i uruchom testy (`cmake --build --preset debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure`), a na końcu wycofaj zmianę (`git checkout src tests`).

1. **Liczba przejść i ścian.** Ile przejść, segmentów ścian i słupków ma labirynt doskonały 5 na 3? Odpowiedź: 14 przejść, 24 segmenty, 24 słupki.
2. **Własny przebieg.** Siatka 2 na 2, start w `(0, 0)`. Kolejne wyniki `randomBelow` to 0, 0, 0. Narysuj labirynt. Odpowiedź: z `(0, 0)` kandydaci to E i S, wynik 0 wybiera E. Z `(1, 0)` jedynym kandydatem jest S. Z `(1, 1)` jedynym kandydatem jest W. Przejścia: `(0,0)-(1,0)`, `(1,0)-(1,1)`, `(1,1)-(0,1)`. Zostaje ściana między `(0, 0)` a `(0, 1)`.
3. **Odrzucanie.** Generator daje liczby od 0 do 15, a potrzebna jest liczba mniejsza od 5. Podaj `limit` i liczby odrzucane. Ile liczb przypada na każdą resztę? Odpowiedź: `16 % 5 = 1`, `limit = 15`, odrzucana jest tylko 15, na każdą resztę przypadają 3 liczby.
4. **Środek komórki.** Podaj środek komórki `(5, 2)` i obszar, który zajmuje labirynt 8 na 6. Odpowiedź: `(11, 0, 5)`. Od `x = 0` do 16 i od `z = 0` do 12.
5. **Segment.** Komórka `(2, 3)` ma ścianę wschodnią. Podaj pozycję i oś segmentu oraz narożniki `min` i `max` jego pudełka kolizji. Odpowiedź: pozycja `(6, 0, 7)`, oś `AlongZ`, pudełko od `(5,85, 0, 6)` do `(6,15, 3, 8)`.
6. **Słupek.** W którym narożniku siatki 2 na 2 nie stanie słupek, jeśli usunąć wszystkie cztery ściany wewnętrzne? Dlaczego w labiryncie doskonałym taka sytuacja nie występuje? Odpowiedź: w środkowym, `(2, 0, 2)`. Cztery przejścia wokół jednego narożnika tworzą pętlę.
7. **Inna kolejność kierunków.** Zamień w `ALL_DIRECTIONS` miejscami `East` i `West`. Wynik (zmierzony): nie przechodzą dwa przypadki, labirynt wzorcowy i przykład 3 na 3. Wszystkie testy własności przechodzą nadal. Wyjaśnij, dlaczego labirynty są nadal doskonałe, ale inne (pułapka 2), i dlaczego `opposite` nadal działa.
8. **Bez odrzucania.** Usuń z `randomBelow` pętlę `while`, zostawiając samo `value % bound`. Wynik (zmierzony): wszystkie testy przechodzą. Policz, jak często dla granicy 3 wynik różniłby się od poprawnego (sekcja 2.6), i wyjaśnij, dlaczego żaden test nie ma szans tego wykryć. Czy to znaczy, że pętla jest zbędna?
9. **Rozkład standardowy.** Zamień `randomBelow(generator, candidateCount)` w `generateMaze` na `std::uniform_int_distribution<std::uint32_t>(0, candidateCount - 1)(generator)`. Wynik zmierzony na Windowsie (MSVC): nie przechodzą dwa przypadki, labirynt wzorcowy i przykład 3 na 3, a testy własności przechodzą. Labirynty są więc poprawne, ale inne. Uruchom to samo na Macu i porównaj rysunek z komunikatu testu z wynikiem z Windowsa: czego dowodzi różnica, a czego dowodziłby jej brak?
10. **Pętle.** Dopisz w teście funkcję, która po wygenerowaniu labiryntu usuwa jedną dodatkową ścianę wewnętrzną, i sprawdź, które asercje z testu `generateMaze makes a perfect maze for every size and seed` przestają przechodzić. Odpowiedź: liczba przejść (jest o jedno za dużo), osiągalność nadal przechodzi.
11. **Segment bramy.** Komórka wyjścia labiryntu startowego to (6, 5), a jej jedyny otwarty bok to wschód. Co zwraca `wallSegmentOn(6, 5, Direction::East)`? Odpowiedź: pozycja `(14, 0, 11)` (linia kolumn numer 7 leży w `7 * 2 = 14`, środek wiersza 5 w `(5 + 0,5) * 2 = 11`), oś `AlongZ`. Tę wartość przypina test `the default maze has 13 crystals and its exit in the cell (6, 5)` w `tests/CrystalTests.cpp`.
12. **Ślepe zaułki labiryntu wzorcowego.** Na rysunku labiryntu 4 na 4 z ziarna 1 (sekcja 5.8) wskaż wszystkie komórki, dla których `isDeadEnd` zwraca prawdę. Odpowiedź: `(0, 0)`, `(3, 1)` i `(0, 3)`: każda ma ściany na trzech bokach. Pierwsza to start, druga to wyjście (najdalsza komórka, 14 przejść od startu), więc wolny ślepy zaułek jest jeden, `(0, 3)`, i tam trafia pierwszy z dwóch kryształów tego labiryntu ([`gameplay.md`](gameplay.md)).

## 9. Pytania kontrolne

1. **Jak zapisany jest labirynt i dlaczego ściany są na krawędziach?**
   Jako siatka komórek, z których każda pamięta, na których z czterech boków stoi ściana (`std::vector` tablic czterech wartości `bool`). Ściany na krawędziach są cienkie i nie zabierają komórek, a przy zapisie blokowym miałyby grubość korytarza.

2. **Co to jest labirynt doskonały i jak to sprawdzić?**
   Labirynt, w którym między każdymi dwiema komórkami jest dokładnie jedna droga: drzewo rozpinające siatki. Sprawdzam dwie rzeczy: wypełnianie zalewowe dociera do wszystkich komórek, a przejść jest dokładnie `width * height - 1`.

3. **Opisz algorytm recursive backtracker.**
   Start od siatki ze wszystkimi ścianami. Komórkę startową oznaczam jako odwiedzoną i kładę na stosie. W pętli: jeśli komórka na szczycie stosu ma nieodwiedzonych sąsiadów, losuję jednego, usuwam ścianę do niego, oznaczam go i kładę na stosie. Jeśli nie ma, zdejmuję ją ze stosu. Koniec, gdy stos jest pusty.

4. **Dlaczego wynik nie ma pętli i ma wszystkie komórki?**
   Ściana jest usuwana tylko przy wejściu do komórki nieodwiedzonej, a do każdej wchodzi się raz, więc przejść jest `n - 1`. Każda nowa komórka jest połączona z poprzednią, więc całość jest spójna. Spójny graf o `n - 1` krawędziach jest drzewem.

5. **Dlaczego generator jest iteracyjny?**
   W wersji rekurencyjnej głębokość wywołań równa się długości drogi, czyli nawet liczbie wszystkich komórek, a stos wywołań ma mały, stały rozmiar. Wersja iteracyjna trzyma drogę we własnym wektorze na stercie.

6. **Dlaczego `std::mt19937`, a nie `rand()` albo `std::default_random_engine`?**
   Standard opisuje `mt19937` co do bitu, razem z zasianiem jedną liczbą i wartością kontrolną 4123659995 dla dziesięciotysięcznego wyniku. `rand()` i `std::default_random_engine` są zdefiniowane przez implementację, więc dają różne ciągi w różnych bibliotekach.

7. **Dlaczego nie `std::uniform_int_distribution` ani `std::shuffle`?**
   Standard określa, jaki rozkład mają dać, ale nie algorytm. Biblioteki clanga i MSVC liczą to inaczej, więc to samo ziarno dałoby inny labirynt na macOS i na Windowsie.

8. **Co to jest modulo bias i jak radzi sobie z nim `randomBelow`?**
   Gdy zakres generatora nie dzieli się równo przez `n`, małe reszty mają o jedną liczbę więcej. `randomBelow` liczy `limit`, największą wielokrotność `n` w zakresie, i odrzuca liczby od `limit` w górę, losując ponownie. Poniżej `limit` każda reszta ma tyle samo liczb.

9. **Od czego jeszcze, poza ziarnem, zależy wygenerowany labirynt?**
   Od rozmiaru, od komórki startowej i od kolejności, w jakiej zbierani są kandydaci (North, East, South, West). Wszystkie trzy są stałe i zapisane w kodzie.

10. **Jak siatka przelicza się na metry?**
    Komórka ma 2 m. Linia siatki numer `k` leży w `k * 2`, środek komórki `(x, z)` w `((x + 0,5) * 2, 0, (z + 0,5) * 2)`. Labirynt zaczyna się w początku układu i rośnie w stronę +X i +Z. Funkcje układu dają `y = 0`, a wysokość nad terenem nadaje potem `placeOnTerrain`.

11. **Jak `wallSegments` unika podwójnych ścian?**
    Każda komórka zgłasza swoją ścianę północną i zachodnią. Południową i wschodnią zgłasza tylko w ostatnim wierszu i ostatniej kolumnie, bo wszędzie indziej zgłosi je sąsiad jako swoją północną albo zachodnią.

12. **Gdzie stoją słupki i ile ich jest w labiryncie doskonałym?**
    W każdym narożniku siatki, w którym kończy się co najmniej jedna ściana. W labiryncie doskonałym jest to każdy narożnik, czyli `(w + 1)(h + 1)`, i tyle samo co segmentów ścian.

13. **Czym pozycja segmentu różni się od środka jego pudełka?**
    Pozycja leży na podłodze, w środku podstawy, tak jak początek układu modelu. Środek pudełka jest o połowę wysokości wyżej (1,5 m dla ściany). Przeliczenie robi `wallBox`.

14. **Dlaczego `Maze` jest klasą z prywatnymi polami, skoro `Aabb` jest strukturą?**
    `Maze` ma niezmiennik: dwie komórki zgadzają się co do ściany między sobą. Prywatne pola i jedna funkcja zmieniająca (`removeWall`) gwarantują, że nikt nie zmieni jednej strony bez drugiej. `Aabb` nie ma takiego warunku, który klasa mogłaby wymusić tanio.

15. **Dlaczego ściana ma dwie stałe grubości?**
    `WALL_VISUAL_THICKNESS` (0,2 m) opisuje model, `WALL_COLLISION_THICKNESS` (0,3 m, równa `PILLAR_SIZE`) pudełko kolizji. Przy jednej wspólnej grubości 0,2 m słupki wystawały 5 cm przed pudełka ścian i gracz sunący po ścianie stawał co 2 m. Teraz lica pudełek ścian i słupków leżą w jednej płaszczyźnie.

16. **Co robi przycisk `Regenerate` w panelu Maze?**
    Ustawia flagę `settings.regenerate`. Nic więcej: labirynt buduje aplikacja na początku następnej klatki, z rozmiaru i ziarna zapisanych w `MazeSettings`.

17. **Czym `Random seed` różni się od losowania labiryntu?**
    Losuje tylko ziarno (`std::random_device`), zapisuje je w polu `Seed` i prosi o regenerację. Sam labirynt nadal powstaje z ziarna w `generateMaze`, więc wpisanie tej samej liczby odtwarza go dokładnie.

18. **Jak powstaje plan labiryntu w panelu?**
    Z listy `world.walls`: każdy segment to odcinek od `pozycja - połowa długości` do `pozycja + połowa długości` wzdłuż swojej osi. Punkt świata `(x, z)` trafia na ekran jako `(origin.x + x * scale, origin.y + z * scale)`, z jedną skalą dla obu osi. Gracz to kropka, a kierunek patrzenia kreska w stronę `(sin(yaw), -cos(yaw))`. Od M5 między ścianami a graczem rysowane są: obrys strefy wyjścia (`AddRect` z narożników `exitZone`), brama (gruba linia, kolor według `gateBlocks`) i kryształy rundy (kropka albo, po zebraniu, pierścień).

19. **Po co test labiryntu wzorcowego, skoro są testy własności?**
    Testy własności mówią, że labirynt jest doskonały, ale przejdą także wtedy, gdy na drugim systemie powstanie **inny** doskonały labirynt. Test wzorcowy przypina konkretne ściany dla ziarna 1 i nie przejdzie, jeśli wynik zależy od kompilatora albo ktoś zmieni algorytm.

20. **Co to jest `MazeCell` i po co powstał, skoro `Maze` przyjmuje `x` i `z` osobno?**
    Para (kolumna, wiersz) z operatorem `==` pisanym przez kompilator. Kod rundy potrzebuje komórki jako jednej wartości: żeby ją zwrócić z funkcji (komórka wyjścia), przekazać (start, wyjście), zapamiętać w polu i porównać. Klasa `Maze` się nie zmieniła.

21. **Kiedy komórka jest ślepym zaułkiem i kto o to pyta?**
    Gdy ma ścianę na dokładnie trzech z czterech boków (`isDeadEnd` w `Maze.cpp`). Pyta `placeCrystals`: kryształy trafiają najpierw do ślepych zaułków. W M4 pytały o to światła punktowe i funkcja leżała w `Lighting.*`.

22. **Czym `wallSegmentOn` różni się od `wallSegments`?**
    `wallSegmentOn(x, z, side)` to wzór: gdzie stałby segment na danym boku danej komórki, bez pytania, czy ściana tam jest. `wallSegments(maze)` to lista ścian, które naprawdę stoją, każda raz: pyta `hasWall` i woła `wallSegmentOn`. Z samego wzoru korzysta brama, która stoi na boku otwartym.

23. **Skąd plan wie, że kryształ jest zebrany, a brama otwarta?**
    Od M5 panel dostaje stałą referencję do `game::Round`. Kryształy rysuje z `round.crystals` (pole `collected`), a kolor bramy wybiera wynik `game::gateBlocks(world, round)`, czyli ta sama funkcja, która decyduje o liście przeszkód gracza.

## 10. Źródła

- Jamis Buck, "Maze Generation: Recursive Backtracking": <https://weblog.jamisbuck.org/2010/12/27/maze-generation-recursive-backtracking> oraz książka "Mazes for Programmers" (Pragmatic Bookshelf, 2015): algorytmy generowania labiryntów, labirynt doskonały, charakter korytarzy.
- Wikipedia, "Maze generation algorithm": <https://en.wikipedia.org/wiki/Maze_generation_algorithm> (randomized depth-first search, wersja rekurencyjna i iteracyjna).
- cppreference, `std::mersenne_twister_engine`: <https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine> (parametry `mt19937`, wartość kontrolna dziesięciotysięcznego wyniku), `std::uniform_int_distribution`: <https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution>.
- Standard C++, rozdział [rand] (biblioteka liczb losowych): wymagania dla silników są dokładne, a dla rozkładów algorytm jest pozostawiony implementacji.
- Daniel Lemire, "Fast Random Integer Generation in an Interval" (2019): <https://arxiv.org/abs/1805.10941> (błąd reszty z dzielenia i metody bez niego, w tym odrzucanie).
- M. Matsumoto, T. Nishimura, "Mersenne Twister: A 623-dimensionally equidistributed uniform pseudo-random number generator" (1998): opis algorytmu, z którego powstał skrypt kontrolny.
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `game`), [`../scene/collision.md`](../scene/collision.md) (pudełka i `moveAndSlide`), [`../scene/camera.md`](../scene/camera.md) (kompas kąta yaw, konwencja układu), [`maze-rendering.md`](maze-rendering.md) (macierze modelu, rysowanie, regeneracja), [`player.md`](player.md) (gracz), [`gameplay.md`](gameplay.md) (runda: wyjście, brama, kryształy, kto korzysta z `MazeCell`, `isDeadEnd` i `wallSegmentOn`), [`../debug-ui.md`](../debug-ui.md) (podpięcie panelu i motyw), [`../../guides/blender.md`](../../guides/blender.md) (modele ściany, słupka i podłogi), [`../../libraries/doctest.md`](../../libraries/doctest.md) (testy), [`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md) (dlaczego własna funkcja losująca).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 2 (generator labiryntu), sekcja 9 (skala: komórka 2 na 2 m, ściana 3 m), sekcja 10 (panel Maze).
