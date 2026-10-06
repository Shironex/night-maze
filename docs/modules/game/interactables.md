# Moduł game: dźwignie i kartki (rozmieszczenie, reguły, wskazywanie)

Kamień milowy: M8, podstawy bez okna (dane, matematyka i testy), a w M8, części 2: selekcja, dźwignie i kartki, podpięcie do świata, rundy, rysowania i wejścia. Temat wykładu: żaden wprost. To logika gry, która używa tematu 15 (promień z [`../scene/picking.md`](../scene/picking.md)).
Kod: [`src/game/Interactables.hpp`](../../../src/game/Interactables.hpp), [`src/game/Interactables.cpp`](../../../src/game/Interactables.cpp), testy w [`tests/InteractablesTests.cpp`](../../../tests/InteractablesTests.cpp). Część 2 dodaje: [`src/game/Interaction.hpp`](../../../src/game/Interaction.hpp) i [`.cpp`](../../../src/game/Interaction.cpp) (macierze modeli, akcja klawisza), [`src/game/InteractableRenderer.hpp`](../../../src/game/InteractableRenderer.hpp) i [`.cpp`](../../../src/game/InteractableRenderer.cpp) (rysowanie), zmiany w [`src/game/MazeWorld.hpp`](../../../src/game/MazeWorld.hpp), [`src/game/Round.hpp`](../../../src/game/Round.hpp) i [`src/game/Round.cpp`](../../../src/game/Round.cpp) oraz testy w [`tests/InteractionTests.cpp`](../../../tests/InteractionTests.cpp). Korzysta z [`src/scene/Raycast.hpp`](../../../src/scene/Raycast.hpp), [`src/game/Exit.hpp`](../../../src/game/Exit.hpp) (`passageDistances`), [`src/game/Crystals.hpp`](../../../src/game/Crystals.hpp) (`CrystalSpawn`), [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp) (`cellCenter`, `wallSegmentOn`) i [`src/game/Terrain.hpp`](../../../src/game/Terrain.hpp) (`heightAt`).

Część modułu `game`. Wstęp do modułu jest w [`README.md`](README.md). Ten dokument stoi na czterech innych: [`maze-generator.md`](maze-generator.md) (siatka `Maze`, kierunki, `MazeCell`, `randomBelow`, układ w świecie), [`gameplay.md`](gameplay.md) (przeszukiwanie wszerz `passageDistances`, wyjście i brama, kryształy, tasowanie komórek), [`../scene/collision.md`](../scene/collision.md) (pudełka `Aabb`, pudełko ściany grubsze od ściany) i [`../scene/picking.md`](../scene/picking.md) (promień, `nearestHit`). Testy są napisane w bibliotece doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)).

## 1. Po co to jest

Labirynt z M5 ma jedną drogę między dwiema komórkami (labirynt doskonały). M8 dodaje do niego rzeczy, które gracz **wskazuje i uruchamia**:

- **dźwignia** (lever): wisi na ścianie, a pociągnięta obniża **inną** wewnętrzną ścianę labiryntu. Zniknięcie ściany łączy dwie drogi w pętlę, czyli daje **skrót**,
- **kartka** (note): wisi na ścianie, a przeczytana pokazuje jedną linię tekstu: podpowiedź policzoną z labiryntu (gdzie jest wyjście, gdzie jest najbliższy kryształ) albo linię z małej tabeli.

Ten dokument odpowiada na pytania, które z tego wynikają:

| Pytanie | Odpowiedź w kodzie |
|---|---|
| którą ścianę ma otworzyć dźwignia | `game::chooseShortcutWalls` |
| w której komórce i na której ścianie wisi dźwignia albo kartka | `game::placeInteractables` |
| gdzie dokładnie jest dźwignia w świecie i jakie ma pudełko | `game::leverPosition`, `game::leverBox`, `game::notePosition`, `game::noteBox` |
| co oznacza "dźwignia pociągnięta" | `game::InteractableState`, `game::pullLever`, `game::openedWalls` |
| na co patrzy gracz | `game::pickInteractable` (używa `scene::nearestHit`), w grze przez `game::pickInRound` ([`../scene/picking.md`](../scene/picking.md)) |
| co robi pociągnięcie w rundzie | `game::pullRoundLever`, `roundObstacles`, `roundWallMatrices`, `roundMaze` (sekcje 2.15 do 2.18) |
| jak dźwignia i kartka są narysowane | `game::InteractableRenderer`, `game::mountModelMatrix`, `game::leverHandleMatrix` (sekcja 2.19) |
| jak karta kartki jest otwierana i zamykana | `game::readNote`, `game::closeNote`, `game::openNoteText` (sekcja 2.20) |
| co jest napisane na kartce | `game::compassTowards`, `game::compassName`, `game::noteText` |

Kod leży w bibliotece `game_logic` i nie potrzebuje okna ani OpenGL. Test potrafi zbudować labirynt, ustawić dźwignie i kartki, wycelować promieniem i pociągnąć dźwignię, nie tworząc ani okna, ani kontekstu OpenGL.

**Stan na dziś (2026-10-06), uczciwie.** Dźwignie i kartki działają teraz w grze: wiszą w `MazeWorld`, runda zna ich stan, pociągnięta dźwignia opuszcza ścianę, a kartka otwiera kartę HUD. Co jest sprawdzone i czym:

| Co | Czym sprawdzone | Kto |
|---|---|---|
| rozmieszczenie, reguły, wskazywanie (stan z części 1) | 29 przypadków w `tests/InteractablesTests.cpp` (policzone z pliku). W części 2 zmienił się w tym pliku tylko jeden komentarz | bramka projektu, zgłoszona przez autora kodu |
| świat, runda, ściana otwarta dźwignią, karta kartki, macierze modeli, minimapa | 21 nowych przypadków w `tests/InteractionTests.cpp` (policzone z pliku: 21 makr `TEST_CASE`) | bramka projektu, zgłoszona przez autora kodu |
| bramka `make check` | **466 przypadków i 152264 asercji** w Debug i Release (przed tą porcją 445 i 150296). Ja ich nie uruchamiałem: ten dokument opisuje kod z plików | zgłoszone przez autora kodu |
| start Debug | czysty log, wymienia nowe modele i tekstury jako wczytane | zgłoszone przez autora kodu |
| obraz działającej gry | **widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela**: lista w sekcji 5.11 | agent, który napisał kod, uruchomił grę skryptem i oglądał zrzuty |
| ręczny test właściciela, macOS | **otwarte** | listy w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 24.2, i [`../../guides/build-macos.md`](../../guides/build-macos.md) |

Znane problemy w chwili pisania: model dźwigni, który od przodu czytał się jak szara płyta, został przerobiony (ciemna płyta z ramą, jasna gałka, sekcja 2.19), a podpowiedź "E: pull lever" stoi teraz na dole okna pośrodku i nie zasłania wskazanego obiektu. Znane uwagi kosmetyczne: z 1 m na wprost gałka w górnym położeniu zasłania górną trzecią część płyty, a przy wyłączonej latarce i bez podświetlenia płyta jest prawie czarna na ścianie. Znane ograniczenia zachowania: sekcja 2.21.

### 1.1 Decyzje właściciela a wybory implementacji

**Decyzje właściciela (2026-10-06):**

1. Wybieranie obiektów to ray casting: promień z kamery przez środek ekranu, gdy mysz jest przechwycona, albo przez kursor, gdy jest wolna ([`../scene/picking.md`](../scene/picking.md), sekcja 1.1).
2. **Dźwignia otwiera skrót: obniża jeden wewnętrzny segment ściany.**
3. **Kartki pokazują krótką podpowiedź na karcie HUD**: albo policzoną z labiryntu, albo linię z tabeli ("flavour").

To wszystkie trzy decyzje właściciela z tego dnia dla tej porcji. Wszystko inne w tym dokumencie (także cała część 2: kiedy ściana przestaje blokować, kopia labiryntu w rundzie, podświetlenie, zasady kliknięć) to wybory implementacji.

**Wybory implementacji.** Każdy z uzasadnieniem z komentarzy w kodzie. Tam, gdzie kod nie podaje powodu, jest to powiedziane i dodana jest oznaczona jako **analiza** hipoteza:

| Wybór | Uzasadnienie z kodu |
|---|---|
| wynik ściany z odległości liczonych w przejściach od startu (`passageDistances`) | komentarz: ze ścianą otwartą dalsza komórka jest o jedno przejście za bliższą, więc droga do niej skraca się o `far - near - 1` przejść. Dlaczego **od startu**: kod nie podaje powodu. **Analiza:** gracz zaczyna rundę w starcie |
| ściana jest warta dźwigni od wyniku 6 (`LEVER_MIN_STEPS_SAVED`) | komentarz: liczba zawsze parzysta, więc 6 znaczy "marsz co najmniej 7 przejść staje się marszem 1" |
| nigdy ściana komórki wyjścia | komentarz: brama jest jedyną drogą do środka, a drugie otwarcie pozwoliłoby ją obejść |
| po każdym wyborze ściana wypada z **kopii** labiryntu i odległości liczą się od nowa | komentarz: dwie dźwignie nigdy nie otwierają prawie tego samego skrótu |
| remis wyników: wygrywa pierwsza ściana w kolejności wierszy | komentarz: "Nothing is random here: the answer follows from the maze alone" |
| nic nie wisi na ścianie, którą otwiera któraś dźwignia | komentarz: zniknęłoby razem ze ścianą |
| dźwignia ma osobną komórkę, nigdy start ani wyjście; najpierw komórki **bez** kryształu | komentarz: komórki z kryształem tylko wtedy, gdy innych zabraknie. Powodu nie ma. **Analiza:** rzeczy do wskazania nie leżą wtedy w tym samym miejscu co światło i blask kryształu |
| dwa osobne generatory: dla dźwigni i dla kartek | komentarz: inna liczba kartek nigdy nie przesuwa dźwigni |
| przesunięcia ziarna 5000011 i 3000017 | komentarz: nie powtarzają ani liczb, którymi wycięto labirynt, ani liczb kryształów (które dodają 1000003), trawy (2000003) i kałuż (4000037), ani siebie nawzajem. Dowolne liczby różne od tamtych i od siebie by pasowały. Dźwignie miały wcześniej 2000003, czyli tyle samo co trawa: kolizję usunęła zmiana na 5000011 |
| domyślnie 2 dźwignie i 3 kartki | komentarz: dwa skróty zmieniają drogi w labiryncie 10 na 10, nie robiąc z niego otwartego pola, a trzy kartki to po jednej z każdego rodzaju |
| najwyżej 16 dźwigni i 16 kartek | komentarz: tylko chroni przed błędnym ustawieniem, które zapełniłoby labirynt |
| zasięg 2,5 m (`INTERACTION_REACH`) | komentarz: trochę więcej niż jedna komórka (2 m), więc dźwignia na dalszej ścianie własnej komórki jest zawsze w zasięgu, a dwie komórki dalej już nie |
| dźwignia na wysokości 1,2 m, kartka na 1,5 m | komentarz: dźwignia na wysokości ręki, kartka trochę poniżej oczu gracza (1,7 m) |
| pudełka wskazywania głębsze niż 5 cm | komentarz: pudełko kolizji ściany stoi 5 cm przed jej widocznym licem, więc pudełko wskazywania musi wystawać dalej, inaczej ściana zasłaniałaby to, co na niej wisi |
| dźwignia nie jest przeszkodą | komentarz: gracza nigdy nie zatrzymuje jej pudełko |
| tekst kartki liczony na żądanie, nie zapisany w kartce | komentarz: odpowiedź zmienia się w trakcie rundy, a podpowiedź do zebranego kryształu byłaby kłamstwem |
| kompas liczbami całkowitymi, granica 1 do 2 | komentarz: "close to the 22.5 degrees that would cut the circle into eight equal parts, and it needs whole numbers only" |
| rodzaje po kolei: wyjście, kryształ, linia | komentarz: pierwsze trzy kartki to po jednej z rodzaju. Kolejne linie to kolejne wiersze tabeli, "so none repeats before all were used" |
| `InteractableState` jako osobna struktura | komentarz (część 1): żeby mogła stać się polem rundy. Dziś jest nim: `Round::interactables`, zaczynane od nowa z każdą rundą |
| ściana otwarta dźwignią przestaje blokować **w chwili pociągnięcia** | reguła bramy (`gateBlocks`): to, co się otwiera, przestaje być przeszkodą w chwili otwarcia, a model jeszcze opada. Przez 1,5 s gracz może przejść przez ścianę, którą jeszcze widać. Notatka [`../../decisions/opened-wall-stops-blocking-at-pull.md`](../../decisions/opened-wall-stops-blocking-at-pull.md) |
| `sinkProgressAfter` i `sinkDepth` wspólne dla bramy i ścian | brama i ściana opadają w tym samym czasie (`GATE_OPEN_SECONDS`, 1,5 s) i na tę samą głębokość (`GATE_SINK_DEPTH`, 3,3 m) |
| całkiem opadła ściana zostaje na liście rysowania, pod gruntem | lista macierzy ma stały rozmiar i kolejność ścian świata. Teren zasłania ścianę (sekcja 2.18) |
| runda ma **własną kopię labiryntu** (`Round::maze`) | `roundMaze(world, round)` czytają przeszukiwanie (discovery) i minimapa, `world.maze` nigdy się nie zmienia. Notatka [`../../decisions/round-keeps-own-maze-copy.md`](../../decisions/round-keeps-own-maze-copy.md) |
| `roundWallMatrices` liczone raz na klatkę dla trzech przebiegów | dwa przebiegi cienia i przebieg sceny rysują ściany z tej samej listy, więc cień pasuje do ściany (sekcja 2.18) |
| podświetlenie to pulsujące `uEmissive` wskazanego obiektu | bez nowego shadera, nie widać go w widokach Normals i UVs. Notatka [`../../decisions/highlight-as-emissive-pulse.md`](../../decisions/highlight-as-emissive-pulse.md) |
| początek promienia przeniesiony z bliskiej płaszczyzny do oka | zasięg 2,5 m jest od oka. Notatka [`../../decisions/pick-ray-starts-in-the-eye.md`](../../decisions/pick-ray-starts-in-the-eye.md) |
| zasady kliknięć (E albo lewy klik, wolny kursor, panele) | [`../scene/picking.md`](../scene/picking.md), sekcja 5.11. Interakcja tylko w trwającej rundzie |
| karta kartki zamyka się: E, klik, odejście dalej niż 3,0 m, wygrana. Bez licznika czasu | `NOTE_READ_DISTANCE` to trochę więcej niż zasięg promienia (2,5 m), żeby karta nie zamykała się, gdy gracz jeszcze stoi przy kartce |
| uchwyt dźwigni przechyla się między dwoma kątami w 0,3 s | `LEVER_PULL_SECONDS`, szybki ruch ręki, znacznie krótszy niż opadanie ściany |
| uchwyt to osobny model (`lever_handle.obj`) | `drawModel` daje każdej części modelu jedną macierz, a uchwyt jest jedyną częścią, która się rusza |
| przycisk "Pull all levers" to flaga prośby obsługiwana w `onRender` | jak restart: między dwoma krokami symulacji |
| liczby dźwigni i kartek w `MazeSettings::interactables` | zwykły przepływ Regenerate: nowy labirynt, nowa runda |
| `MazeWorld::leverWalls` | numer ściany każdej dźwigni na liście ścian, szukany raz przy budowie świata |
| `MazeRenderer::draw` bierze macierze ścian klatki | ściany opadają, więc lista macierzy świata nie wystarcza |
| `openedWalls` nie jest drogą, którą idzie gra | gra używa `openedWallFlags(world, round)`. `openedWalls` zostało z części 1 (z testem) |

## 2. Teoria

### 2.1 Labirynt doskonały i skrót

Labirynt doskonały ma `N` komórek i `N - 1` przejść, i między każdymi dwiema komórkami **dokładnie jedną** drogę (jest drzewem). Labirynt 4 na 4 z testów ma 16 komórek i 15 przejść. Gdy znika ściana między dwiema komórkami, których nie łączyło bezpośrednie przejście, powstaje druga droga między nimi: **pętla**. Dla gracza oznacza to **skrót**: komórka, do której szedł długą drogą dookoła, jest nagle o jedno przejście dalej od sąsiedniej.

Ściana, która może zniknąć, musi być **wewnętrzna**: stać między dwiema komórkami labiryntu. Ściana zewnętrzna (granica) nie ma po drugiej stronie komórki, więc jej usunięcie robiłoby tylko dziurę w obwodzie. To właśnie sprawdza `game::isInteriorWall`: ściana istnieje (`hasWall`) i komórka za nią leży w labiryncie (`contains`).

### 2.2 Dwie nazwy jednej ściany

Ściana stoi na krawędzi między dwiema komórkami, więc ma **dwie nazwy**: wschodnia ściana komórki `(2, 3)` to zachodnia ściana komórki `(3, 3)`. `Maze` pilnuje, żeby obie perspektywy zmieniały się razem ([`maze-generator.md`](maze-generator.md), sekcja 2.1).

Struktura `WallRef` to jedna nazwa: komórka i strona (`Direction`):

```cpp
struct WallRef {
    MazeCell cell;
    Direction side = Direction::North;
    bool operator==(const WallRef& other) const = default;
};
```

Dwie rzeczy do zapamiętania:

- **`operator==` porównuje nazwy, nie ściany.** `{(2, 3), East}` i `{(3, 3), West}` to ta sama ściana, ale `==` mówi "różne". Test `a wall has two names, and sameWall knows both` przypina to dokładnie (`CHECK_FALSE(east == fromBehind)`).
- **`game::sameWall(a, b)`** to właściwe porównanie ścian: prawda, gdy `a == b` albo `a` jest równe `b` widzianemu z komórki po drugiej stronie. "Widzianie z drugiej strony" to komórka za ścianą (`cell + columnStep(side), row + rowStep(side)`) i strona przeciwna (`opposite(side)`).

Funkcja `chooseShortcutWalls` nazywa każdą ścianę **z jej komórki zachodniej albo północnej**, czyli zawsze ze stroną `East` albo `South`. Powód wynika ze stałej `REPORTED_SIDES = {East, South}`: przejście po wszystkich komórkach i obu tych stronach spotyka każdą ścianę wewnętrzną **dokładnie raz** (ściana północna komórki to południowa komórki nad nią, a zachodnia to wschodnia komórki obok, więc te komórki ją "zgłaszają"). Testy sprawdzają, że `lever.opens.side` to `East` albo `South`.

### 2.3 Wynik ściany: ile przejść skraca

Dla każdej wewnętrznej ściany kod porównuje odległości jej dwóch komórek od startu (`game::passageDistances`, przeszukiwanie wszerz: [`gameplay.md`](gameplay.md), sekcja 5.2). Oznaczmy odległość bliższej od startu komórki przez `near`, a dalszej przez `far`. Bez ściany dalsza komórka jest o jedno przejście za bliższą, a z nią było `far - near`. Wynik to oszczędność:

```text
saved = |dist(komórka) - dist(komórka za ścianą)| - 1
```

**Wynik jest zawsze parzysty.** Siatka jest szachownicą: każde przejście zmienia kolor pola, więc odległości (w przejściach) dwóch sąsiednich komórek różnią się o liczbę **nieparzystą**, a po odjęciu 1 zostaje liczba parzysta. Dotyczy to także labiryntu po otwarciu kolejnych ścian, bo to nadal ruch po siatce. Skutek: "wynik co najmniej 6" znaczy to samo co "wynik większy od 5".

Kod robi to drugie: `bestSaved` startuje od `LEVER_MIN_STEPS_SAVED - 1` (czyli 5), a ściana jest brana tylko wtedy, gdy `saved > bestSaved`. Dzięki temu jedna zmienna jest jednocześnie progiem i najlepszym wynikiem dotychczas.

**Ograniczenie:** wynik liczy się od **startu**. Dla gracza, który stoi gdzie indziej, oszczędność jest inna. Kod nie podaje powodu takiego wyboru (sekcja 1.1). Analiza: gracz zaczyna rundę w starcie, a dźwignie są rozmieszczone raz na labirynt.

### 2.4 Wybór kilku ścian: odległości liczone od nowa

Dwie dźwignie, które otwierałyby prawie ten sam skrót, byłyby stratą. Dlatego `chooseShortcutWalls` wybiera ściany **po jednej**:

1. liczy odległości od startu w kopii labiryntu,
2. przegląda wszystkie wewnętrzne ściany i wybiera tę o najwyższym wyniku,
3. usuwa wybraną ścianę z **kopii** (`opened.removeWall`) i wraca do kroku 1.

Pętla kończy się, gdy wybrano tyle ścian, ile zamówiono (najwyżej 16), albo gdy żadna ściana nie ma wyniku co najmniej 6. Labirynt wołającego się nie zmienia: ściany spadają dopiero przy pociągnięciu dźwigni. Test `golden maze...` sprawdza to wprost (`CHECK(maze.hasWall(0, 0, game::Direction::East))` po wyborze).

Koszt: jedno przeszukiwanie wszerz i jedno przejście po ścianach na dźwignię, także dla największego labiryntu 256 na 256 (komentarz w nagłówku i test `the largest maze gets its levers and notes`).

### 2.5 Reguły i remisy

Do wyniku ściana musi spełnić (kod `chooseShortcutWalls`):

- jest wewnętrzna (`isInteriorWall`) w **aktualnej** kopii (ściana otwarta wcześniej już jej nie spełnia),
- **nie należy do komórki wyjścia:** ani `wall.cell == exit`, ani komórka za nią nie jest wyjściem. Komentarz: brama jest jedyną drogą do środka, a drugie otwarcie pozwoliłoby ją obejść,
- obie jej komórki są osiągalne od startu (`UNREACHABLE` w odległościach oznacza komórkę bez drogi, możliwą tylko w labiryncie zbudowanym ręcznie w teście),
- wynik jest co najmniej 6.

**Remisy:** przeglądanie idzie wiersz po wierszu (wiersz 0 od zachodu na wschód, potem wiersz 1 i tak dalej), a w komórce `East` przed `South`. Ściana zastępuje najlepszą dotychczas tylko wtedy, gdy ma **ostro** większy wynik (`saved > bestSaved`), więc przy równych wynikach zostaje wcześniejsza. Dlatego wynik zależy tylko od labiryntu, a nie od niczego losowego. Test `the first shortcut is the wall that saves the most, the first one in row order` sprawdza to dla 10 ziaren: dla każdej innej ściany wynik jest mniejszy (gdy ściana jest wcześniej w kolejności) albo nie większy (gdy później).

Przy liczbie zamówionych ścian ujemnej albo 0 wynik jest pusty, a powyżej 16 zamówienie jest obcinane do 16 (`std::clamp`).

### 2.6 Przykład na liczbach: labirynt 4 na 4 z ziarna 1

To labirynt z testu `golden maze: 4 x 4 cells from seed 1 has exactly one wall worth a lever` (ten sam co w złotym teście generatora). Rysunek z liczbą przejść od startu `(0, 0)` w każdej komórce. `#` to ściana, która będzie najlepszym skrótem. `==` to brama, która stoi na otwartej stronie komórki wyjścia (przejście między `(3, 0)` a `(3, 1)` jest otwarte, brama jest w nim):

```text
  +--+--+--+--+
  | 0#11 12 13|
  +  +  +--+==+
  | 1|10  9|14|
  +  +--+  +--+
  | 2  3| 8  7|
  +--+  +--+  +
  | 5  4  5  6|
  +--+--+--+--+
```

Liczby sprawdzone ręcznie z rysunku ścian testu: od `(0, 0)` do `(0, 2)` w dół, potem `(1, 2)` 3, `(1, 3)` 4, `(0, 3)` i `(2, 3)` po 5, `(3, 3)` 6, `(3, 2)` 7, `(2, 2)` 8, `(2, 1)` 9, `(1, 1)` 10, `(1, 0)` 11, `(2, 0)` 12, `(3, 0)` 13 i `(3, 1)` 14. Komórka `(3, 1)` jest wyjściem: najdalsza (`farthestCell`), jedyna z odległością 14.

**Ściany wewnętrzne, które istnieją** (nazwane z komórki zachodniej albo północnej), ich wyniki i uwagi:

| Ściana | Odległości | Wynik | Uwaga |
|---|---|---|---|
| `(0, 0)` East | 0 i 11 | **10** | najlepsza |
| `(0, 1)` East | 1 i 10 | 8 | |
| `(1, 1)` South | 10 i 3 (komórka `(1, 2)`) | 6 | |
| `(1, 2)` East | 3 i 8 | 4 | poniżej 6 |
| `(2, 0)` South | 12 i 9 | 2 | |
| `(0, 2)` South | 2 i 5 | 2 | |
| `(2, 2)` South | 8 i 5 | 2 | |
| `(2, 1)` East | 9 i 14 | pominięta | ściana komórki wyjścia |
| `(3, 1)` South | 14 i 7 | pominięta | ściana komórki wyjścia |

Test podaje tę samą listę w komentarzu ("1|10 saves 8, 10 over 3 saves 6, 3|8 saves 4, and the rest save 2") i sprawdza trzy wyniki przez `stepsSaved`. **Runda 1:** wygrywa `(0, 0)` East z wynikiem 10 (próg 6 przekroczony).

**Runda 2:** ściana `(0, 0)` East wypada z kopii. Teraz `(1, 0)` jest o 1 od startu, więc nowe odległości: `(1, 0)` 1, `(2, 0)` 2, `(3, 0)` 3, `(3, 1)` 4, `(1, 1)` 2, `(2, 1)` 3, `(2, 2)` 4, `(3, 2)` 5, `(1, 2)` 3 (nadal przez `(0, 1)`, `(0, 2)`). Wyniki pozostałych ścian:

| Ściana | Nowe odległości | Wynik |
|---|---|---|
| `(0, 1)` East | 1 i 2 | 0 |
| `(1, 1)` South | 2 i 3 | 0 |
| `(1, 2)` East | 3 i 4 | 0 |
| `(2, 0)` South | 2 i 3 | 0 |
| `(2, 2)` South | 4 i 5 | 0 |
| `(0, 2)` South | 2 i 5 | 2 |

Najlepszy wynik to 2, poniżej progu 6: **żadna ściana nie jest już warta dźwigni** i pętla się kończy. Wynik: jedna ściana, nawet gdy zamówiono 3. Tak właśnie mówi test: `three.size() == 1U`.

**Przykład, w którym nie ma żadnej dźwigni.** Labirynt 3 na 3 z ziarna 7 (przykład z [`maze-generator.md`](maze-generator.md)) ma wyjście `(0, 2)` z odległością 6. Najlepsza ściana to `(0, 0)` East: odległości 0 i 5, wynik 4, poniżej 6. Pozostałe mają wynik 2, ściana `(0, 1)` South należy do wyjścia. Ten labirynt nie dostaje dźwigni. Ten przykład policzyłem ręcznie z rysunku ścian, żaden test go nie przypina.

### 2.7 Gdzie wisi dźwignia i kartka

Wybór ścian do otwarcia nie jest losowy. Losowy jest dopiero wybór **komórki i ściany**, na której coś **wisi**. Reguły (`placeInteractables`):

**Dźwignie:**

1. Najpierw `chooseShortcutWalls` daje listę ścian do otwarcia: `shortcuts` (od najlepszej).
2. Kandydaci to komórki przeglądane wiersz po wierszu, **bez startu i bez wyjścia** i bez komórek, które nie mają żadnej ściany do powieszenia (komórka na skrzyżowaniu bez ścian). Dzielą się na dwie listy: **bez kryształu** i **z kryształem**.
3. Generator `std::mt19937` zasiany `seed + 5000011` tasuje najpierw listę bez kryształu, potem listę z kryształem. Obie listy są sklejone: **wszystkie komórki bez kryształu przed wszystkimi z kryształem**.
4. Liczba dźwigni to `min(liczba skrótów, liczba kandydatów)`. Dźwignia numer `i` dostaje komórkę numer `i` z sklejonej listy i otwiera skrót numer `i`, więc **najlepsze skróty mają swoje dźwignie**, gdy komórek zabraknie.
5. Strona ściany: spośród czterech stron komórki zostają te, które mają ścianę i **nie są ścianą otwieraną przez żadną dźwignię** (`sameWall` z listą `shortcuts`), w stałej kolejności `North, East, South, West`. Z nich `randomBelow` wybiera jedną, z równymi szansami.

**Kartki:**

1. Kandydaci to komórki wiersz po wierszu, bez startu, wyjścia i **komórek, w których wisi dźwignia**, ze ścianą do powieszenia. Komórka z kryształem **jest dozwolona** (komentarz: "A cell with a crystal is fine for a note"). Reguła "najpierw bez kryształu" dotyczy tylko dźwigni.
2. Generator zasiany `seed + 3000017` tasuje komórki, potem losuje numer pierwszej linii tabeli (`randomBelow(…, 6)`), a potem po jednym losowaniu strony na kartkę.
3. Liczba kartek to `min(zamówiona po obcięciu do 0 do 16, liczba kandydatów)`.

**Tasowanie** to tasowanie Fishera i Yatesa napisane ręcznie (ten sam kod co w `Crystals.cpp`, powód w [`gameplay.md`](gameplay.md), sekcja 5.5 i w [`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md): `std::shuffle` może inaczej użyć generatora w każdej bibliotece standardowej). Idzie od końca: ostatnie miejsce dostaje jedną z `n` komórek, przedostatnie jedną z pozostałych `n - 1` i tak dalej, aż do miejsca 1. Miejsce 0 zatrzymuje komórkę, która została.

**Dlaczego dwa generatory.** Kolejność losowań jest **częścią wyniku**: ta sama liczba wywołań `randomBelow` w tej samej kolejności daje te same komórki i strony. Komentarz w kodzie podaje jeden powód osobnych generatorów: inna liczba kartek nie rusza dźwigni. Test `the number of notes does not move the levers` sprawdza to: 0 kartek i 9 kartek dają dokładnie te same dźwignie (`sameLever`). **Analiza:** przy jednym generatorze i dźwigniach losowanych jako pierwsze ta własność też by zachodziła, więc osobne generatory są raczej zabezpieczeniem: własność nie zależy od tego, w jakiej kolejności kod losuje.

**Dlaczego różne przesunięcia ziarna.** Generator labiryntu używa samego ziarna, kryształy `seed + 1000003`, a dźwignie i kartki `seed + 5000011` i `seed + 3000017`. Komentarz w kodzie: dzięki temu nie powtarzają ani liczb, którymi wycięto labirynt, ani liczb kryształów (dodają 1000003), trawy (2000003) i kałuż (4000037), ani siebie nawzajem; pasowałyby dowolne liczby różne od tamtych i od siebie. Dźwignie miały wcześniej 2000003, czyli przesunięcie trawy, i dlatego zmieniono je na 5000011. Dodawanie do `std::uint32_t` zawija się przy `2^32` i jest dobrze zdefiniowane.

Testy potwierdzają zachowanie, a nie konkretne komórki: `the same maze and seed give the same levers and notes` (to samo wejście, to samo wyjście dla 10 ziaren), `another seed gives other levers and notes` (inne ziarno daje inne rozmieszczenie, ale **te same ściany do otwarcia**, bo te wynikają tylko z labiryntu) i `a lever goes into a cell with a crystal only when no other cell is left`. **Nie ma testu "złotego"** z konkretnymi komórkami dźwigni dla konkretnego ziarna, takiego jak złoty labirynt generatora. Zgodność macOS i Windows opiera się więc na `std::mt19937` i `randomBelow` (sekcja 7, pułapka 4).

### 2.8 Pozycja i pudełko wskazywania

**Uwaga o liczbach.** Wymiary pudełka **dźwigni** w tej sekcji, w sekcji 2.10, w ćwiczeniu 7 i w pytaniu 11 odpowiadają stałym z części 1. Pudełko dźwigni po przeróbce modelu się nie zmieniło (0,3 x 0,4 x 0,25 m): gałka sięga w głąb dokładnie do 0,25 m, gdy wskazuje prosto ze ściany. Wymiary pudełka kartki też się nie zmieniły.

**Pozycja.** Dźwignia i kartka wiszą na **widocznym licu** ściany, w środku ściany. Ściana stoi na granicy komórki, a jej widoczna grubość to `WALL_VISUAL_THICKNESS = 0,2 m`, więc lico patrzące do komórki leży `1 - 0,1 = 0,9 m` od środka komórki (`CELL_SIZE / 2 - WALL_VISUAL_THICKNESS / 2`):

```text
position = cellCenter + towardsWall * 0,9 + (0, wysokość_gruntu + wysokość_zawieszenia, 0)
```

`towardsWall` to wektor kroku strony (`columnStep`, 0, `rowStep`). Liczby z testu `a lever hangs on the visible face of its wall...`: komórka `(2, 3)` ma środek `(5, 0, 7)`, grunt 0,5 m, wysokość dźwigni 1,2 m:

| Strona | `position` |
|---|---|
| North | `(5, 1,7, 6,1)` |
| East | `(5,9, 1,7, 7)` |
| South | `(5, 1,7, 7,9)` |
| West | `(4,1, 1,7, 7)` |

**Pudełko.** Pudełko to `Aabb` o trzech rozmiarach: **szerokość wzdłuż ściany**, wysokość i **głębokość od ściany w głąb komórki**. Środek pudełka leży pół głębokości od pozycji, w stronę komórki (`position - towardsWall * depth / 2`). Dla ścian północnej i południowej szerokość biegnie wzdłuż `X`, a głębokość wzdłuż `Z`, dla wschodniej i zachodniej na odwrót.

| Rzecz | Szerokość | Wysokość | Głębokość | Wysokość zawieszenia |
|---|---|---|---|---|
| dźwignia | 0,3 m | 0,4 m | 0,25 m | 1,2 m |
| kartka | 0,4 m | 0,5 m | 0,15 m | 1,5 m |

Liczby z testów: dźwignia na północnej ścianie `(2, 3)` (grunt 0,5) ma pudełko `min = (4,85, 1,5, 6,1)` i `max = (5,15, 1,9, 6,35)`, na wschodniej `min = (5,65, 1,5, 6,85)` i `max = (5,9, 1,9, 7,15)`. Kartka na zachodniej ścianie `(0, 0)` na płaskim gruncie ma pozycję `(0,1, 1,5, 1,0)` i pudełko od `(0,1, 1,25, 0,8)` do `(0,25, 1,75, 1,2)`.

**Dlaczego pudełko musi wystawać poza pudełko kolizji ściany.** Pudełko kolizji ściany ma grubość 0,3 m (`WALL_COLLISION_THICKNESS`), a widoczna ściana 0,2 m ([`../scene/collision.md`](../scene/collision.md), pułapka 1). Pudełko kolizji wystaje więc `(0,3 - 0,2) / 2 = 0,05 m` przed widoczne lico w stronę komórki. Pudełko wskazywania **zaczyna się na licu**. Gdyby było płytsze niż 0,05 m, całe leżałoby **wewnątrz** pudełka ściany, a promień z komórki trafiałby najpierw w ścianę i ściana (jako przesłaniacz, sekcja 2.10) zasłaniałaby to, co na niej wisi. Stała `MOUNT_BOX_MIN_DEPTH = 0,05` i dwa `static_assert` pilnują, że obie głębokości są większe:

- dźwignia: 0,25 m, przed pudełkiem ściany wystaje o `0,25 - 0,05 = 0,20 m`,
- kartka: 0,15 m, wystaje o `0,15 - 0,05 = 0,10 m`.

Test `checkMount` sprawdza dla każdej dźwigni i kartki każdego rozmieszczenia, że przednia ściana pudełka wskazywania jest bliżej środka komórki niż przednia ściana pudełka kolizji ściany (`boxFront < wallFront`), a test `every lever and note of a maze can be picked from the middle of its cell` sprawdza to w praktyce: promień z oka w środku komórki, z listą wszystkich ścian i słupków labiryntu jako przesłaniaczy, trafia w każdą dźwignię i kartkę z 10 ziaren.

**Teren.** Pozycje są liczone dla płaskiego gruntu `y = 0`. Funkcja `placeInteractablesOnTerrain` podnosi każdą dźwignię i kartkę na wysokość terenu w punkcie ściany, na której wisi (`Terrain::heightAt(position.x, position.z)`), i liczy pudełko od nowa. Nic nie przesuwa się w bok, więc można ją wołać ponownie po przebudowie terenu z inną skalą wysokości. Test `levers and notes follow the height of the terrain` sprawdza, że drugie wywołanie niczego nie zmienia (`sameInteractables`), a płaski teren (`game::Terrain{}`) przywraca pozycje płaskie.

### 2.9 Pociągnięcie dźwigni

Stan dźwigni to jedno `std::vector<bool>`: `leverPulled[i]` jest prawdą, gdy dźwignia `i` była pociągnięta. Jest w osobnej strukturze `InteractableState`, żeby mogła stać się polem rundy (komentarz).

- `startInteractables(interactables)` daje stan początkowy: wszystkie fałsz, po jednym wpisie na dźwignię.
- `pullLever(state, interactables, index)` przy **pierwszym** pociągnięciu ustawia flagę i zwraca `{opened = true, wall = ściana, którą otwiera}`. Przy **każdym następnym** niczego nie zmienia i zwraca domyślne `PullResult` (`opened = false`, pole `wall` bez znaczenia).
- Rzuca `std::out_of_range`, gdy nie ma dźwigni o tym numerze albo stan nie został zaczęty dla tych dźwigni (rozmiary się nie zgadzają).
- `isLeverPulled(state, index)` rzuca `std::out_of_range`, gdy stan nie ma dźwigni o tym numerze.
- `openedWalls(interactables, state)` daje ściany pociągniętych dźwigni **w kolejności dźwigni**. Pętla biegnie do krótszej z dwóch list, więc stan zaczęty dla innych dźwigni nie wyczyta niczego poza końcem.

`openedWallSegment(lever)` daje `WallSegment` ściany, którą dźwignia otwiera, na `y = 0`: ten sam segment, który `game::wallSegments` wymienia dla tej ściany, więc w liście da się go znaleźć po `x` i `z` (test `the wall a lever opens is one of the wall segments of the maze` dla 10 ziaren sprawdza, że pasuje dokładnie jeden).

Test `a round starts with no lever pulled, and a lever opens its wall once` przeprowadza cały scenariusz: nic nie pociągnięte, pociągnięcie dźwigni 1 otwiera jej ścianę, drugie pociągnięcie nic nie robi, dźwignia 0 nadal działa, `openedWalls` zwraca ściany w kolejności dźwigni (0, potem 1), a nowy stan z `startInteractables` zamyka wszystko.

**Co z tego robi runda (część 2):** `pullLever` zmienia tylko flagę. Usunięcie ściany z listy przeszkód, kopia labiryntu rundy, opadanie modelu i animacja uchwytu są w `Round` (sekcje 2.15 do 2.18). Gra wywołuje `pullLever` przez `pullRoundLever`, a ścian nie szuka przez `openedWalls`, tylko przez `openedWallFlags` (sekcja 2.16).

### 2.10 Wskazywanie z przesłaniaczami

`pickInteractable(ray, interactables, blockers, reach)` odpowiada: na którą dźwignię albo kartkę wskazuje promień, **jeśli żadna ściana nie stoi na drodze**.

1. Z pudełek dźwigni i z pudełek kartek powstają dwie osobne listy, w kolejności dźwigni i kartek, więc numer trafienia jest numerem dźwigni albo kartki.
2. `scene::nearestHit` szuka najbliższego trafienia w każdej liście w zasięgu `reach`.
3. Z obu wygrywa **bliższe**. Kartka musi być **ostro** bliżej, żeby wygrać: przy równej odległości wygrywa dźwignia.
4. Gdy nic nie trafiono, wynik to `InteractableKind::None`.
5. **Przesłaniacze.** `blockers` to lista pudełek, które zasłaniają: ściany i słupki labiryntu, a zamknięta brama też (opis w nagłówku; budowanie listy to zadanie wołającego). Kod wywołuje `nearestHit` na tej liście z zasięgiem równym **odległości trafionej rzeczy**. Gdy znajdzie przesłaniacz **ostro bliżej** (`blocker.distance < picked.distance`), wynik to `None`: dźwigni nie można pociągnąć przez ścianę. Przesłaniacz **dalej** niż trafiona rzecz nie ma znaczenia (poza zasięgiem tego wyszukiwania), a przesłaniacz **dokładnie w tej samej odległości** też jej nie zasłania.

**Liczby z testu `the picking ray finds the lever or the note it points at`.** W komórce `(0, 0)` (środek `(1, 0, 1)`) wisi ręcznie zbudowana dźwignia na ścianie północnej (pozycja `(1, 1,2, 0,1)`) i kartka na wschodniej. Gracz stoi w środku komórki, jego oko jest na `(1, 1,7, 1)`, a promień idzie do pozycji dźwigni: kierunek `(0, -0,5, -0,9)` znormalizowany, czyli `(0, -0,486, -0,874)`. Pudełko dźwigni sięga `z` od 0,1 do 0,35, `y` od 1,0 do 1,4. Promień wchodzi w nie przez przednią ścianę `z = 0,35` po `t = (1 - 0,35) / 0,874 = 0,744` m (rachunek własny, test sprawdza przedział od 0,65 do 0,9). Przednia ściana pudełka kolizji ściany północnej jest przy `z = 0,15` (grubość 0,3 wokół osi `z = 0`), czyli promień dojdzie do niej dopiero po `t = (1 - 0,15) / 0,874 = 0,972` m. **Dźwignia jest bliżej niż ściana**, więc ściana, na której wisi, jej nie zasłania: tak pokazuje podprzypadek `the wall it hangs on does not hide it`. Podprzypadek `a wall in between hides it` pokazuje odwrotnie: promień z sąsiedniej komórki do kartki na wschodniej ścianie `(0, 0)` najpierw uderza w pudełko tej ściany, więc wynik to `None` (bez ściany w liście ta sama kartka jest znaleziona).

### 2.11 Kompas: kierunek w liczbach całkowitych

`compassTowards(from, to)` odpowiada, w którą stronę **na mapie** (w linii prostej, bez względu na ściany) leży komórka `to` od komórki `from`. Północ to `-Z` (mniejszy numer wiersza), wschód to `+X` (większy numer kolumny), jak w całej grze.

Oznaczmy `columns = |to.x - from.x|` i `rows = |to.z - from.z|`:

1. oba przesunięcia równe 0: `Here`,
2. `columns > 2 * rows`: prosto na `East` (gdy `to.x > from.x`) albo `West`,
3. `rows > 2 * columns`: prosto na `South` (gdy `to.z > from.z`) albo `North`,
4. w przeciwnym razie jedna z czterech przekątnych, według znaków obu przesunięć.

Komentarz w kodzie: granica `STRAIGHT_FACTOR = 2` to nachylenie 1 do 2, około 27 stopni (dokładnie `arctan(1/2) = 26,6 stopnia`), blisko 22,5 stopnia, które podzieliłyby okrąg na osiem równych części, a liczy się samymi liczbami całkowitymi, bez kątów.

| Przesunięcie (kolumny, wiersze) | Wynik | Uwaga |
|---|---|---|
| `(0, 0)` | `Here` | |
| `(0, -4)` | `North` | |
| `(3, -3)` | `NorthEast` | |
| `(4, 0)` | `East` | |
| `(3, 3)` | `SouthEast` | |
| `(0, 4)` | `South` | |
| `(-3, 3)` | `SouthWest` | |
| `(-4, 0)` | `West` | |
| `(-3, -3)` | `NorthWest` | |
| `(2, -1)` | `NorthEast` | **dokładnie dwa razy** to jeszcze przekątna (`2 > 2` jest fałszem) |
| `(3, -1)` | `East` | jeden więcej: prosto |
| `(1, -3)` | `North` | |
| `(-5, 2)` | `West` | |
| `(0, -1)` i `(1, 1)` | `North` i `SouthEast` | sąsiednie komórki |

(Wszystkie wiersze poza dwoma ostatnimi to przypadki z testów `compassTowards names the eight directions and the cell itself` i `a direction is straight when one offset is more than twice the other`.)

Nazwa kierunku do zdania: `compassName` daje `"north"`, `"north-east"`, `"east"`, `"south-east"`, `"south"`, `"south-west"`, `"west"`, `"north-west"` i `"here"`.

### 2.12 Rodzaje kartek i ich teksty

| Rodzaj | Co mówi | Tekst |
|---|---|---|
| `ExitHint` | w którą stronę od kartki leży komórka wyjścia | `"The exit lies to the north-east."` (albo `"The exit lies right here."` dla `Here`) |
| `CrystalHint` | w którą stronę leży **najbliższy kryształ, który został** | `"A crystal glows to the south."`, `"A crystal glows right here."`, a gdy nie został żaden: `"No crystal is left to find."` |
| `Flavour` | linia z tabeli | jedna z sześciu linii (niżej) |

`noteText(note, exit, crystalCells)` liczy tekst **na żądanie**. Parametr `crystalCells` to komórki kryształów, które **nie zostały jeszcze zebrane**: wołający go dostarcza, bo odpowiedź zmienia się w trakcie rundy (komentarz: podpowiedź do zebranego kryształu byłaby kłamstwem). "Najbliższy" to linia prosta na siatce, mierzona kwadratem odległości (liczby całkowite, bez pierwiastka), a z dwóch równo odległych kryształów wygrywa wcześniejszy na liście. Zdanie składa funkcja pomocnicza: podmiot, czasownik i kierunek.

Liczby z testu `a note says where the exit is, where the nearest crystal is, or a flavour line`: kartka w komórce `(5, 5)`, wyjście `(9, 1)`: przesunięcie `(4, -4)`, więc `"The exit lies to the north-east."`. Wyjście `(5, 0)`: `(0, -5)`, `"The exit lies to the north."`. Kryształy `(0, 5)` i `(5, 7)`: kwadraty odległości 25 i 4, więc najbliższy to `(5, 7)`, przesunięcie `(0, 2)`: `"A crystal glows to the south."`. Po zebraniu bliższego zostaje `(0, 5)`: `"A crystal glows to the west."`. Dla `(5, 3)` i `(7, 5)` oba mają kwadrat 4 i wygrywa pierwszy: `"A crystal glows to the north."`.

**Tabela linii (`FLAVOUR_LINES`, po angielsku jak reszta HUD):**

1. `The moon sees every corridor. You see one.`
2. `Dead ends are where the light hides.`
3. `Count your steps. The maze does not.`
4. `A lever moves a wall. Somewhere.`
5. `The gate listens for crystals.`
6. `Save your battery for the way back.`

Test `the flavour lines are short plain text` sprawdza, że każda linia nie jest pusta, ma najwyżej 60 znaków (jedna linia karty HUD), składa się z drukowalnego ASCII (komentarz: font HUD ma tylko zwykłe litery) i że żadna nie powtarza się w tabeli. `flavourLine` rzuca `std::out_of_range` dla numeru spoza tabeli.

**Przydział rodzajów i linii.** Kartka numer `i` ma rodzaj numer `i % 3` (wyjście, kryształ, linia, wyjście, ...). Pierwsza kartka rodzaju `Flavour` bierze linię wylosowaną przez ziarno (`firstLine`), następne kolejne linie tabeli: `flavourIndex = (firstLine + k) % 6`, gdzie `k` to numer kartki wśród kartek `Flavour`. Test `the flavour notes of a maze take different lines until the table runs out` zamawia 12 kartek (cztery `Flavour`) i sprawdza, że linie się nie powtarzają.

**Uwaga:** przy dozwolonym maksimum 16 kartek kartek `Flavour` jest najwyżej 5 (numery 2, 5, 8, 11, 14), a linii w tabeli jest 6, więc przy obecnych stałych żadna linia **nigdy** się nie powtarza i `%` tylko zawija numer.

### 2.13 Małe labirynty

Z testów (`a maze without a wall worth opening gets no lever`):

| Labirynt | Dźwignie | Kartki (przy domyślnych 3) | Dlaczego |
|---|---|---|---|
| 1 na 1 | 0 | 0 | komórka jest startem i wyjściem naraz |
| 7 na 1 albo 1 na 7 (korytarz) | 0 | 3 | w korytarzu nie ma ściany między dwiema komórkami; pięć komórek między startem a wyjściem wystarcza na kartki |
| 2 na 1 | 0 | 0 | są tylko start i wyjście |
| 2 na 2 | 0 | 2 | jedyna wewnętrzna ściana jest ścianą komórki wyjścia (i oszczędza najwyżej 2 przejścia, poniżej progu 6); dwie komórki poza startem i wyjściem dostają kartki |
| 3 na 3, ziarno 7 | 0 | nie pokrywa test | najlepszy wynik 4 (sekcja 2.6) |

W labiryncie 2 na 2 powód jest ogólny (rozumowanie własne, test sprawdza 10 ziaren): drzewo rozpinające czterech komórek to ścieżka, a jedyna ściana wewnętrzna łączy dwa końce tej ścieżki, a jednym z końców jest zawsze wyjście (najdalsza komórka od startu jest końcem). Ściana dotyka więc komórki wyjścia, a jej wynik to najwyżej `|0 - 3| - 1 = 2` (gdy start jest drugim końcem; gdy jest drugą komórką ścieżki, wynik to 0), czyli poniżej progu 6. Odrzucają ją więc obie reguły naraz.

### 2.14 Dźwignie i kartki w świecie: rozmieszczenie i teren

Od części 2 dźwignie i kartki są **polem `MazeWorld`** (`world.interactables`), tak jak kryształy. Co dodało `buildMazeWorld`:

1. po kryształach wywołuje `placeInteractables(maze, seed, START_CELL, exit.cell, world.crystals, interactables)`: dźwignia omija komórki z kryształem, więc kryształy muszą być policzone wcześniej,
2. dla każdej dźwigni szuka **raz** numeru jej ściany na liście `world.walls` i zapisuje go w `world.leverWalls` (`wallIndexOf` porównuje pozycję i oś segmentu z `openedWallSegment(lever)`). Szukanie dzieje się **przed** obniżeniem ścian do gruntu (to zmienia ich `y`), a gdyby ściany nie znaleziono, funkcja rzuca `std::logic_error`: lepszy błąd niż obniżenie złej ściany,
3. `placeOnTerrain` dostaje dodatkowy krok: `placeInteractablesOnTerrain(world.interactables, terrain)`, więc dźwignie i kartki wiszą na wysokości gruntu (plus `LEVER_MOUNT_HEIGHT` i `NOTE_MOUNT_HEIGHT`) także po zmianie skali wysokości. Nic nie przesuwa się w bok.

`world.colliders` ma pudełko każdej ściany **najpierw**, w kolejności `world.walls`, a pudełka słupków po nich. Dlatego ten sam numer znajduje macierz ściany (`wallMatrices[i]`), jej pudełko (`colliders[i]`) i wpis w `leverWalls`: trzy listy są w tej samej kolejności. Z tego korzystają `roundObstacles` i `roundWallMatrices`.

Liczby dźwigni i kartek są polem `MazeSettings::interactables` (typu `InteractableSettings`), więc idą tą samą drogą co rozmiar i ziarno: panel Maze edytuje je, **Regenerate** buduje labirynt z nimi (po przycięciu do `0..MAX_*` w `regenerateMaze`), a to zaczyna nową rundę. Domyślnie 2 dźwignie i 3 kartki. Ziarno dźwigni to ziarno labiryntu plus `LEVER_SEED_OFFSET` (5000011).

Test `a maze world holds the levers and notes of its seed and the wall of every lever` porównuje świat z `placeInteractables` wywołanym ręcznie i sprawdza, że numer w `leverWalls` wskazuje właśnie segment z `openedWallSegment`. `the settings of a maze world decide how many levers and notes it gets` sprawdza 0 i 0, 1 i 7 oraz domyślne wartości `MazeSettings`. `levers and notes of a world hang above its terrain, also after a rebuild` używa terenu o nachyleniu i po zmianie skali wysokości sprawdza `y` (nic nie rusza `x` i `z`).

### 2.15 Stan rundy: co dochodzi do `Round`

`startRound` zaczyna rundę **bez** pociągniętych dźwigni:

| Pole `Round` | Znaczenie |
|---|---|
| `interactables` (`InteractableState`) | które dźwignie są pociągnięte (`startInteractables`), po jednym wpisie na dźwignię |
| `wallProgress` | `std::vector<float>`: jak głęboko opadła ściana każdej dźwigni, 0 stoi, 1 całkiem w gruncie. Zaczyna się od zer |
| `maze` (`std::optional<Maze>`) | **własna kopia labiryntu rundy**: `round.maze = world.maze` w `startRound`. `std::optional`, bo `Maze` nie ma konstruktora domyślnego, a `Round` musi mieć wartość domyślną |
| `noteOpen`, `noteIndex` | czy karta kartki jest otwarta i numer kartki |
| `pullAllLevers` (w `GameplaySettings`, nie w `Round`) | flaga prośby z panelu (sekcja 2.22) |

`roundMaze(world, round)` zwraca `*round.maze`, a dla rundy, która nie była zaczęta przez `startRound` (nie ma kopii), labirynt świata. Czyta ją przeszukiwanie (`discoverAround(round.discovery, roundMaze(world, round), feet)` w każdym kroku) i minimapa (`buildMinimapVertices`). `world.maze` **nigdy** się nie zmienia, więc nowa runda na tym samym labiryncie ma wszystkie ściany. Test `a new round has no lever pulled, every wall standing and no note open` pilnuje stanu początkowego: wszystko puste, lista przeszkód ma tyle pudełek co świat plus brama, macierze ścian identyczne ze światowymi, a `&roundMaze(world, notStarted) == &world.maze`.

### 2.16 Pociągnięcie dźwigni w rundzie

`pullRoundLever(round, world, index)` opakowuje `pullLever`:

1. `pullLever(round.interactables, world.interactables, index)` ustawia flagę (przy pierwszym pociągnięciu) i zwraca ścianę. Dla złego numeru **rzuca `std::out_of_range`**,
2. gdy nic się nie otworzyło (dźwignia była pociągnięta), funkcja zwraca `false`,
3. inaczej usuwa ścianę z kopii labiryntu rundy: `round.maze->removeWall(x, z, side)` (usuwa ją dla **obu** komórek), o ile kopia istnieje,
4. zwraca `true`.

W tej chwili dzieją się naraz trzy rzeczy: (a) ściana znika dla przeszukiwania i minimapy (przez kopię labiryntu), (b) jej pudełko wypada z `roundObstacles` (sekcja 2.17), więc gracz i promień przechodzą, (c) ściana zaczyna opadać (`wallProgress` rośnie w `updateRound`). Jest to ta sama zasada co dla bramy: to, co się otwiera, przestaje być przeszkodą w chwili otwarcia, a model jeszcze opada ([`../../decisions/opened-wall-stops-blocking-at-pull.md`](../../decisions/opened-wall-stops-blocking-at-pull.md)).

Pozostałe funkcje: `pullAllLevers(round, world)` (funkcja `game::`, osobna od pola `GameplaySettings::pullAllLevers`) pociąga wszystkie jeszcze niepociągnięte i zwraca, ile ścian się otworzyło, `pulledLeverCount(round)` liczy pociągnięte, `openedWallFlags(world, round)` daje po fladze na ścianę świata (według `world.leverWalls` i stanu dźwigni, z zabezpieczeniem przed rozmiarami z innego świata).

Testy: `pulling a lever opens its wall once: obstacles, maze of the round, sinking` (drugie pociągnięcie zwraca `false`, zły numer rzuca `std::out_of_range`, z listy znika dokładnie jedno pudełko, kopia traci ścianę z obu komórek, a `world.maze` ją zachowuje), `pull all levers opens every wall and counts them` i `the view passes an opened wall, and a restart brings every wall back` (przeszukiwanie widzi komórkę za otwartą ścianą, a nowa runda ma ścianę z powrotem).

**Uwaga do kodu.** Gdy `round.maze` jest puste (runda nie zaczęta przez `startRound`), `pullRoundLever` pomija `removeWall`: ściana przestaje blokować i opada, ale przeszukiwanie i minimapa nadal ją widzą (obserwacja z kodu, ta ścieżka nie jest w grze użyta, bo każda runda idzie przez `startRound`).

### 2.17 Przeszkody: `roundObstacles`

`roundObstacles(world, round)` buduje listę dla gracza **i** dla promienia: pudełka z `world.colliders` bez pudełek ścian z `openedWallFlags`, a na końcu pudełko bramy, dopóki `gateBlocks` jest prawdą. Pętla korzysta z tego, że pierwsze `walls.size()` pudełek to ściany w kolejności świata. Aplikacja trzyma listę w `m_obstacles` i buduje ją od nowa tylko wtedy, gdy się zmienia: początek rundy, otwarcie bramy, **pociągnięcie dźwigni** (`interact` zwraca `true`, gdy ściana się otworzyła) i zmiana skali wysokości terenu.

Dwa skutki do zapamiętania: panel Collision liczy ściany bez otwartych (`%d walls (%d opened by levers)`), a `drawColliderLines` rysuje `m_obstacles` bez pudełka bramy (bramę rysuje osobno), więc ściana otwarta dźwignią traci też swoje pudełko w widoku debugowym.

### 2.18 Opadająca ściana: wspólne wzory z bramą

Brama (od M5) opada przez `GATE_OPEN_SECONDS` (1,5 s) na głębokość `GATE_SINK_DEPTH` (3,3 m). Części 2 wyodrębniła dwa wzory, których używają brama i ściany:

```cpp
float sinkProgressAfter(float progress, float stepSeconds) {
    return std::min(progress + stepSeconds / GATE_OPEN_SECONDS, 1.0F);
}
float sinkDepth(float progress) { return progress * GATE_SINK_DEPTH; }
```

`updateRound` liczy teraz `gateProgress` przez `sinkProgressAfter`, a dla **każdej pociągniętej dźwigni** to samo robi z `wallProgress[i]` (z zabezpieczeniem rozmiarów). Głębokość 3,3 m jest większa od wysokości ściany (3,0 m, `WALL_HEIGHT`) i słupka (3,15 m, `PILLAR_HEIGHT`): test `the sink formulas reach the full depth in 1.5 s and stop there` pilnuje `GATE_SINK_DEPTH > WALL_HEIGHT` i tego, że 180 kroków po 1/120 s daje 1, a dalszy czas niczego nie zmienia.

**Macierze ścian klatki.** `roundWallMatrices(world, round)` kopiuje `world.wallMatrices` i dla każdej pociągniętej dźwigni liczy macierz ściany od nowa z obniżonego segmentu:

```cpp
WallSegment lowered = world.walls[wall];
lowered.position.y -= sinkDepth(round.wallProgress[i]);
matrices[wall] = wallModelMatrix(lowered);
```

(tak, jak `GameplayRenderer` obniża bramę). Lista ma ten sam rozmiar i kolejność co `world.walls`, a całkiem opadła ściana **zostaje** na niej: leży wtedy poniżej najniższego gruntu pod sobą i teren ją zasłania. Aplikacja liczy listę **raz na klatkę** (`m_wallMatrices`) i przekazuje do `MazeRenderer::draw(shader, world, wallMatrices)` w obu przebiegach cienia i w przebiegu sceny, więc cień opadającej ściany jest cieniem tego, co jeszcze jest nad gruntem. Słupki rysują się nadal ze świata. Test: w połowie opadania macierz ściany daje punkt o `GATE_SINK_DEPTH / 2` niżej, żadna inna ściana się nie rusza, a po całym czasie punkt jest o `GATE_SINK_DEPTH` niżej.

### 2.19 Rysowanie: modele, macierze mocowania i uchwyt

Trzy modele (`lever.obj`, `lever_handle.obj`, `note.obj`, opis w [`../../guides/blender.md`](../../guides/blender.md)) mają jedną konwencję: początek w środku **tyłu**, czyli punktu przymocowanego do ściany, tył leży w płaszczyźnie `z = 0`, a model sterczy wzdłuż `+Z`. To model czegoś na **północnej** ścianie komórki: sięga na południe, w głąb komórki. Płyta (`lever.obj`) ma 0,16 x 0,30 m i 0,02 m grubości, podniesioną ramę z czterech listew po 0,02 m i obudowę osi wystającą na 0,07 m (52 trójkąty). Uchwyt (`lever_handle.obj`) to pręt 0,03 x 0,03 m i 0,14 m długości oraz gałka 0,07 x 0,07 m kończąca się 0,20 m od osi (20 trójkątów).

Macierze mocowania **nie są w `InteractableRenderer`**, tylko w `Interaction.hpp` i `Interaction.cpp` (choć komentarz nagłówka odsyła do picking.md, to ten dokument je opisuje):

- `mountModelMatrix(position, side)`: model przesunięty do `position` (punkt na licu ściany: `Lever::position` albo `Note::position`) i obrócony wokół osi pionowej o `-yawTowards(side)` stopni. `yawTowards` liczy strony zgodnie z ruchem wskazówek zegara patrząc z góry (północ 0, wschód 90, południe 180, zachód 270), a dodatni obrót wokół `+Y` jest przeciwny do ruchu wskazówek, stąd minus. Dla ściany wschodniej to -90 stopni, które wynosi `+Z` na `-X`, czyli na zachód, od wschodniej ściany. Test `a model on a wall points away from the wall on every side of the cell` sprawdza dla czterech stron: początek modelu w punkcie na ścianie, 1 m wzdłuż `+Z` modelu to 1 m od ściany, a góra zostaje górą,
- `leverHandleMatrix(lever, handleProgress)`: początek modelu uchwytu leży w **osi obrotu** `LEVER_PIVOT_DEPTH` (5 cm, w obudowie osi modelu, która sięga od 0,02 do 0,07 m) przed licem, w środku płyty (`lever.position` minus krok w stronę ściany razy ta głębokość). Przechył wokół osi X modelu to `glm::mix(LEVER_HANDLE_UP_DEGREES, LEVER_HANDLE_DOWN_DEGREES, handleProgress)`, czyli od -55 do +55 stopni: dodatni obrót wokół X zwraca `+Z` ku `-Y`, czyli w dół, więc "w górę" to kąt ujemny. `Transform` obraca wokół X przed obrotem wokół Y, więc uchwyt jest przechylony w przestrzeni modelu, a dopiero potem obrócony ze ścianą. Przy 55 stopniach gałka stoi nad krawędzią płyty, a w całym ruchu mieści się w pudełku dźwigni (w głąb sięga dokładnie 0,25 m przy kącie 0). Test `the handle of a lever turns around its pivot: up before the pull, down after` sprawdza dla czterech stron, że początek leży w osi, a koniec uchwytu (0,2 m od osi) jest przed pociągnięciem wyżej, a po pociągnięciu niżej, o ten sam kąt, i **w obu** położeniach wewnątrz pudełka dźwigni,
- `leverHandleProgress(round, index)`: czas od pociągnięcia to `wallProgress * GATE_OPEN_SECONDS`, a postęp uchwytu to ten czas przez `LEVER_PULL_SECONDS` (0,3 s), nie więcej niż 1. Runda ma więc jedną liczbę na dźwignię, a uchwyt jest opuszczony na długo przed końcem opadania ściany. Dla dźwigni, której runda nie ma: 0. Test `the handle of a lever swings down in 0.3 s after the pull`: po 18 krokach (0,15 s) postęp to 0,5.

`InteractableRenderer::draw` rysuje dla każdej dźwigni płytę (macierz mocowania) i uchwyt (macierz uchwytu), a dla każdej kartki jeden model. Wskazany obiekt dostaje `uEmissive` (podświetlenie), reszta zero (opis: [`../scene/picking.md`](../scene/picking.md), sekcja 5.12). Kartka ma teksturę `note_paper.png` pokazaną **raz** na arkuszu (nie powtarza się), a dźwignia dwie: `lever_iron.png` (ciemne żelazo) na płycie i `lever_brass.png` (mosiądz) na uchwycie.

### 2.20 Karta kartki: otwieranie i zamykanie

`readNote(round, world, index)` ustawia `round.noteOpen = true` i `noteIndex` (numer spoza labiryntu jest ignorowany), `closeNote(round)` zamyka kartę. `openNoteText(world, round)` zwraca pusty napis, gdy karta jest zamknięta, a inaczej `noteText(note, exitCell, komórki niezebranych kryształów)`: lista kryształów jest budowana **za każdym razem**, więc podpowiedź w stronę kryształu zmienia się w trakcie czytania, gdy ten kryształ zostanie zebrany. Test `the card of a crystal hint counts only the crystals that are left` sprawdza zebranie pierwszego kryształu i zebranie wszystkich ("No crystal is left to find.").

Karta zamyka się:

| Kiedy | Gdzie w kodzie |
|---|---|
| E, gdy karta jest otwarta (akcja `CloseNote`) | `interact` |
| lewy klik (przy przechwyconym i przy wolnym kursorze), gdy karta jest otwarta | `handleInteraction` (klik zamyka kartę, **nie** przechwytuje przy tym kursora) |
| gracz oddalił się od kartki o **więcej niż `NOTE_READ_DISTANCE` (3,0 m)**, liczone po gruncie (tylko `x` i `z`) | `closeNoteFarAway` w `updateRound` |
| runda wygrana | `updateRound`: karta wygranej zajmuje miejsce karty kartki |
| restart i nowy labirynt | `startRound` zaczyna rundę z `noteOpen = false` (to nie jest zamykanie przez regułę, tylko świeża runda) |

Nie ma licznika czasu: karta nie znika sama. Test `a note opens its card, the key closes it first, and walking away closes it` sprawdza stanie przy kartce (karta zostaje), odejście o `NOTE_READ_DISTANCE + 0,1` m (karta się zamyka) i zamknięcie klawiszem bez promienia. Zamykanie karty przy **wygranej** nie ma własnej asercji: test o nazwie `a won round has nothing to interact with and closes the card` sprawdza tylko, że `interactionFor` daje `None` w rundzie wygranej.

Interakcja działa **tylko w trwającej rundzie** (`RoundState::Playing`): po wygranej `interactionFor` zawsze daje `None`.

### 2.21 Znane ograniczenia

1. **Trawa przy otwartej ścianie zostaje.** Kępki trawy rosną przy ścianach, a ściana opada i znika, ale kępki obok niej stoją dalej. Trawę sadzi się przy budowie labiryntu i przy zmianie jej gęstości, a nie w reakcji na pociągnięcie dźwigni (obserwacja z kodu: nic w pociągnięciu nie wywołuje sadzenia trawy).
2. **Samotny słupek.** Słupek, który stał tylko dlatego, że kończyła go otwarta ściana, zostaje sam w przejściu.
3. **Przejście przez widoczną ścianę.** Przez 1,5 s opadania gracz może przejść przez ścianę, którą jeszcze widać (decyzja implementacji, sekcja 2.16).
4. **Pełna ściana poniżej gruntu rysowana dalej.** Całkiem opadła ściana zostaje na liście rysowania (pod gruntem), co kosztuje jedno wywołanie rysowania.
5. **Czysta rama połowicznie zatopionej ściany** nie była oglądana na zrzucie.
6. **Mniej dźwigni, niż zamówiono** (sekcja 5.2 i pułapka 5).

### 2.22 Panele i debug

Dodatki w debugowym UI (nie rysują niczego w scenie, zmieniają wartości lub pokazują stan):

- **Gameplay:** "Levers: %d pulled of %d", "Note card: open/closed" i przycisk **Pull all levers**, który ustawia `GameplaySettings::pullAllLevers`. Aplikacja obsługuje flagę w `onRender` jak restart (między krokami): `game::pullAllLevers(m_round, m_mazeWorld)` i, gdy coś się otworzyło, odbudowa listy przeszkód. Restart zamyka ściany z powrotem.
- **Maze:** suwaki "Levers" i "Notes" (0 do 16, `ImGuiSliderFlags_AlwaysClamp`), tekst "Levers: %d, notes: %d" (ile labirynt naprawdę dostał), na planie czerwone kwadraty dźwigni, jasne kartek, a ściana otwarta dźwignią narysowana **przyciemniona** (kolor zebranego kryształu). Minimapa w grze robi odwrotnie: ściany otwartej **nie rysuje** ([`../renderer/minimap.md`](../renderer/minimap.md)).
- **Collision:** ostatni promień, pudełka wskazywania i zamrożenie ([`../scene/picking.md`](../scene/picking.md), sekcja 6).

**Minimapa** pokazuje też dźwignie i kartki (nie było tego w opisie zadania): małe kwadraty (połowa boku 0,22 m, co najmniej 1,5 piksela) w komórkach już odkrytych, dźwignia w mocnym pomarańczowoczerwonym kolorze, a po pociągnięciu przyciemniona, kartka w jasnożółtym. Test `the minimap drops an opened wall and marks levers and notes` liczy wierzchołki kolorów przed i po pociągnięciu oraz dla nic nieodkrytej mapy.

## 3. Jak to działa w OpenGL

`Interactables.hpp`, `Interactables.cpp` i `Interaction.cpp` dołączają GLM, bibliotekę standardową i nagłówki gry bez OpenGL: dźwignia i kartka są **danymi** (komórka, strona, pozycja, pudełko, rodzaj), a wskazywanie to matematyka na procesorze ([`../scene/picking.md`](../scene/picking.md)). OpenGL pojawia się dopiero w `InteractableRenderer.cpp`:

| Co | Wywołanie | Gdzie |
|---|---|---|
| model dźwigni, uchwytu i kartki | `drawModel`: dla każdej części modelu `glBindTexture` (kolor i mapa normalnych), jedna macierz modelu (`uModel`) i `glDrawElements` | `InteractableRenderer::draw` przez `game::drawModel` |
| podświetlenie | `shader.setVec3(EMISSIVE_UNIFORM, ...)`, czyli `glUniform3f`, przed każdą dźwignią i kartką i zerowanie na końcu | ta sama funkcja |
| ściany opadające | zmiana **macierzy** w liście, bez żadnego nowego stanu OpenGL | `MazeRenderer::draw` z `m_wallMatrices` |

`drawModel` daje **każdej** części modelu tę samą macierz, dlatego uchwyt (jedyna ruchoma część) jest osobnym modelem. Modele są wczytywane przez pamięć podręczną assetów ([`../assets/asset-cache.md`](../assets/asset-cache.md)): konstruktor `InteractableRenderer` prosi o trzy modele, a model, który się nie wczyta, jest logowany i po prostu nie jest rysowany. Wskazywanie nie dodaje żadnego przebiegu rysowania ani odczytu z karty.

## 4. Shadery

Moduł nie ma własnego shadera i nie zmienia żadnego pliku shadera. Dźwignie i kartki są rysowane **tymi samymi programami co ściany**: `textured` (bez oświetlenia i w widokach debugowych), `lit` (Phong), `gouraud` i program głębi w przebiegach cienia. Dzięki temu dostają ten sam tryb oświetlenia, te same widoki debugowe i te same cienie księżyca i latarki (kod rysuje je także w przebiegach cienia, z pustym `PickState`). Wszystkie trzy programy kolorowe mają uniform `uEmissive`, którego używa podświetlenie ([`../scene/picking.md`](../scene/picking.md), sekcja 4). Nie było oglądane na ekranie, jak wyglądają w trybach Gouraud i Unlit ani ich cienie księżyca (sekcja 5.11).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Biblioteka | Potrzebuje OpenGL | Testy |
|---|---|---|---|
| `src/game/Interactables.hpp`, `src/game/Interactables.cpp` | `game_logic` | nie | 29 przypadków w `tests/InteractablesTests.cpp` |
| `src/scene/Raycast.hpp`, `src/scene/Raycast.cpp` | `engine` | nie | 17 przypadków w `tests/RaycastTests.cpp`, opis w [`../scene/picking.md`](../scene/picking.md) |
| `src/game/Interaction.hpp`, `src/game/Interaction.cpp` (macierze modeli, akcja klawisza, podświetlenie) | `game_logic` | nie | 21 przypadków w `tests/InteractionTests.cpp` (razem ze światem i rundą) |
| `src/game/MazeWorld.*`, `src/game/Round.*` (pola i funkcje dźwigni i kartek) | `game_logic` | nie | `tests/InteractionTests.cpp` |
| `src/game/InteractableRenderer.hpp`, `src/game/InteractableRenderer.cpp` | plik wykonywalny `night_maze` | tak | brak testów jednostkowych. Częściowo widziane na zrzutach przez agenta (2026-10-06), nie przez właściciela (sekcja 5.11) |
| `assets/models/lever.obj`, `lever_handle.obj`, `note.obj` i ich tekstury | dane | wczytywane przez pamięć podręczną | [`../../guides/blender.md`](../../guides/blender.md) |

### 5.2 Stałe

| Stała | Wartość | Znaczenie |
|---|---|---|
| `DEFAULT_LEVER_COUNT`, `DEFAULT_NOTE_COUNT` | 2, 3 | ile dźwigni i kartek dostaje labirynt domyślnie |
| `MAX_LEVER_COUNT`, `MAX_NOTE_COUNT` | 16, 16 | najwyższe zamówienia, reszta jest obcinana |
| `LEVER_MIN_STEPS_SAVED` | 6 | najmniejszy wynik ściany, żeby miała dźwignię |
| `LEVER_MOUNT_HEIGHT`, `NOTE_MOUNT_HEIGHT` | 1,2 m, 1,5 m | wysokość środka dźwigni i kartki nad gruntem |
| `LEVER_BOX_WIDTH`, `_HEIGHT`, `_DEPTH` | 0,3 m, 0,4 m, 0,25 m | pudełko wskazywania dźwigni (po przeróbce modelu bez zmian) |
| `NOTE_BOX_WIDTH`, `_HEIGHT`, `_DEPTH` | 0,4 m, 0,5 m, 0,15 m | pudełko wskazywania kartki |
| `MOUNT_BOX_MIN_DEPTH` | `(0,3 - 0,2) / 2 = 0,05 m` | o tyle pudełko kolizji ściany wystaje przed widoczne lico |
| `INTERACTION_REACH` | 2,5 m | zasięg wskazywania, od oka |
| `LEVER_PULL_SECONDS` (`Round.hpp`) | 0,3 s | czas opuszczania uchwytu |
| `NOTE_READ_DISTANCE` (`Round.hpp`) | 3,0 m | dalej od kartki karta zamyka się sama |
| `GATE_OPEN_SECONDS`, `GATE_SINK_DEPTH` (`Round.hpp`) | 1,5 s, 3,3 m | czas i głębokość opadania bramy i ścian |
| `LEVER_PIVOT_DEPTH` (`Interaction.hpp`) | 0,05 m | oś obrotu uchwytu przed licem ściany, w obudowie modelu |
| `LEVER_HANDLE_UP_DEGREES`, `LEVER_HANDLE_DOWN_DEGREES` | -55, +55 stopni | kąty uchwytu przed i po pociągnięciu (przy 55 stopniach gałka stoi nad krawędzią płyty i wciąż mieści się w pudełku dźwigni) |
| `HIGHLIGHT_COLOR`, `HIGHLIGHT_MIN_GLOW`, `HIGHLIGHT_MAX_GLOW`, `HIGHLIGHT_PULSE_SPEED` | `(1; 0,8; 0,4)`, 0,8, 2,4, 5 rad/s | podświetlenie ([`../scene/picking.md`](../scene/picking.md), sekcja 5.9) |
| `MINIMAP_LEVER_COLOR`, `MINIMAP_LEVER_PULLED_COLOR`, `MINIMAP_NOTE_COLOR` (`Minimap.hpp`) | `(0,93; 0,36; 0,24)`, `(0,42; 0,24; 0,22)`, `(0,90; 0,86; 0,70)` | kwadraty na minimapie |
| `NOTE_KIND_COUNT` | 3 | liczba rodzajów kartek |
| `LEVER_SEED_OFFSET`, `NOTE_SEED_OFFSET` (w `.cpp`) | 5000011, 3000017 | dodawane do ziarna labiryntu. Różne od przesunięć kryształów (1000003), trawy (2000003) i kałuż (4000037) |
| `STRAIGHT_FACTOR` (w `.cpp`) | 2 | granica kompasu: przesunięcie "więcej niż dwa razy" większe od drugiego |
| `REPORTED_SIDES` (w `.cpp`) | `East, South` | strony, z których nazywane są wybrane ściany |

Dwa `static_assert` w nagłówku sprawdzają przy kompilacji, że `LEVER_BOX_DEPTH` i `NOTE_BOX_DEPTH` są większe od `MOUNT_BOX_MIN_DEPTH`.

### 5.3 Struktury

```cpp
struct InteractableSettings { int leverCount = DEFAULT_LEVER_COUNT; int noteCount = DEFAULT_NOTE_COUNT; };
struct WallRef { MazeCell cell; Direction side = Direction::North; };
struct Lever { WallRef mount; glm::vec3 position; scene::Aabb box; WallRef opens; };
enum class NoteKind { ExitHint = 0, CrystalHint, Flavour };
struct Note { WallRef mount; glm::vec3 position; scene::Aabb box; NoteKind kind; int flavourIndex = 0; };
struct Interactables { std::vector<Lever> levers; std::vector<Note> notes; };
struct InteractableState { std::vector<bool> leverPulled; };
struct PullResult { bool opened = false; WallRef wall; };
enum class InteractableKind { None = 0, Lever, Note };
struct PickedInteractable { InteractableKind kind; std::size_t index; float distance; };
enum class Compass { Here = 0, North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest };
```

| Pole | Znaczenie |
|---|---|
| `Lever::mount` | komórka i strona, na której dźwignia **wisi** |
| `Lever::opens` | ściana, która opada po pociągnięciu. Zawsze inna niż `mount` |
| `Lever::position`, `Lever::box` | środek tyłu dźwigni na licu ściany i pudełko wskazywania. Nie jest przeszkodą |
| `Note::flavourIndex` | numer linii tabeli, tylko dla rodzaju `Flavour` (0 do 5), inne rodzaje go nie używają |
| `Interactables` | wszystko, co w labiryncie można wskazać. Należy do labiryntu (jest polem `MazeWorld::interactables`, jak kryształy) i nie zmienia się w trakcie rundy |
| `InteractableState::leverPulled` | po jednym wpisie na dźwignię w kolejności `levers`, prawda po pociągnięciu. Wpis nigdy nie wraca do fałszu w ramach rundy |
| `PullResult::opened` | prawda tylko przy pierwszym pociągnięciu |
| `PickedInteractable::index` | numer dźwigni albo kartki. Bez znaczenia dla `None` |

### 5.4 Funkcje pomocnicze (anonimowa przestrzeń nazw w `.cpp`)

| Funkcja | Co robi |
|---|---|
| `cellIndex` | miejsce komórki w liście wierszy po kolei (`z * width + x`), jak w `passageDistances` |
| `cellBehind`, `seenFromBehind` | komórka po drugiej stronie ściany (może leżeć poza labiryntem dla ściany granicznej) i ta sama ściana nazwana stamtąd |
| `isOneOf` (dwa przeciążenia) | czy ściana (przez `sameWall`) albo komórka jest na liście |
| `shuffleCells` | Fisher i Yates z `randomBelow` |
| `mountableSides` | strony komórki, które mają ścianę, **bez** ścian otwieranych przez dźwignie, w kolejności `ALL_DIRECTIONS` |
| `randomMountSide` | jedna z tych stron, z równymi szansami |
| `clampedCount` | zamówiona liczba obcięta do zakresu od 0 do maksimum |
| `mountPosition`, `mountBox` | wspólna część `leverPosition` i `notePosition` oraz `leverBox` i `noteBox` |
| `nearestCell` | najbliższa komórka z listy, remis dla wcześniejszej |
| `hintSentence` | składa zdanie podpowiedzi |

### 5.5 `chooseShortcutWalls`

Główna pętla (zwięźle, z komentarzami z kodu):

```cpp
Maze opened = maze;
const std::size_t wanted = clampedCount(count, MAX_LEVER_COUNT);

std::vector<WallRef> chosen;
while (chosen.size() < wanted) {
    const std::vector<int> distances = passageDistances(opened, start);

    bool found = false;
    WallRef best;
    int bestSaved = LEVER_MIN_STEPS_SAVED - 1;

    for (int z = 0; z < opened.height(); ++z) {
        for (int x = 0; x < opened.width(); ++x) {
            for (const Direction side : REPORTED_SIDES) {
                const WallRef wall{.cell = {.x = x, .z = z}, .side = side};
                if (!isInteriorWall(opened, wall)) { continue; }
                const MazeCell behind = cellBehind(wall);
                if (wall.cell == exit || behind == exit) { continue; }
                const int distanceHere = distances[cellIndex(opened, wall.cell)];
                const int distanceBehind = distances[cellIndex(opened, behind)];
                if (distanceHere == UNREACHABLE || distanceBehind == UNREACHABLE) { continue; }

                const int saved = std::abs(distanceHere - distanceBehind) - 1;
                if (saved > bestSaved) {
                    found = true; best = wall; bestSaved = saved;
                }
            }
        }
    }
    if (!found) { break; }
    chosen.push_back(best);
    opened.removeWall(best.cell.x, best.cell.z, best.side);
}
return chosen;
```

| Linia | Znaczenie |
|---|---|
| `if (!maze.contains(start...) \|\| !maze.contains(exit...)) throw` (przed pętlą) | `std::out_of_range`, gdy start albo wyjście nie jest komórką labiryntu (test `a start or an exit outside the maze is an error`) |
| `Maze opened = maze;` | kopia: ściany wybranych skrótów spadają z niej, labirynt wołającego zostaje |
| `wanted = clampedCount(...)` | zamówienie obcięte do 0 do 16 |
| `bestSaved = LEVER_MIN_STEPS_SAVED - 1` | próg i najlepszy wynik w jednej zmiennej (sekcja 2.3) |
| trzy pętle `z`, `x`, `side` | wiersz po wierszu, `East` przed `South`: porządek ma znaczenie dla remisów |
| `isInteriorWall(opened, wall)` | ściana istnieje w kopii i leży między dwiema komórkami |
| `wall.cell == exit \|\| behind == exit` | reguła komórki wyjścia |
| `UNREACHABLE` | komórka bez drogi (tylko w labiryncie ręcznym) |
| `std::abs(...) - 1` | wynik |
| `saved > bestSaved` | ostro większy: przy remisie zostaje wcześniejsza |
| `if (!found) break;` | żadna ściana nie jest warta dźwigni: mniej dźwigni niż zamówiono |

### 5.6 `placeInteractables`

Szkic (bez powtórzeń, z fragmentami dźwigni):

```cpp
const std::vector<WallRef> shortcuts = chooseShortcutWalls(maze, start, exit, settings.leverCount);
// cells with a crystal
// cellsWithoutCrystal, cellsWithCrystal: row after row, without start, exit, and cells
// that have no wall to hang on
std::mt19937 leverGenerator(seed + LEVER_SEED_OFFSET);
shuffleCells(cellsWithoutCrystal, leverGenerator);
shuffleCells(cellsWithCrystal, leverGenerator);
std::vector<MazeCell> leverCells = cellsWithoutCrystal;
leverCells.insert(leverCells.end(), cellsWithCrystal.begin(), cellsWithCrystal.end());

const std::size_t leverCount = std::min(shortcuts.size(), leverCells.size());
for (std::size_t i = 0; i < leverCount; ++i) {
    lever.mount.cell = leverCells[i];
    lever.mount.side = randomMountSide(maze, lever.mount.cell, shortcuts, leverGenerator);
    lever.position = leverPosition(lever.mount, 0.0F);
    lever.box = leverBox(lever.position, lever.mount.side);
    lever.opens = shortcuts[i];
}
// then the notes: noteCells without start, exit, lever cells; noteGenerator(seed + NOTE_SEED_OFFSET)
// shuffleCells, firstLine = randomBelow(gen, lineCount), then per note one randomBelow for the side
note.kind = static_cast<NoteKind>(i % NOTE_KIND_COUNT);
if (note.kind == NoteKind::Flavour) {
    note.flavourIndex = static_cast<int>((firstLine + flavourNotes) % lineCount);
    ++flavourNotes;
}
```

| Fragment | Znaczenie |
|---|---|
| `chooseShortcutWalls(...)` | sprawdza też, że start i wyjście są w labiryncie, więc `placeInteractables` rzuca `std::out_of_range` tak samo |
| dwie listy komórek, wiersz po wierszu | porządek przed tasowaniem jest częścią wyniku, jak w `placeCrystals` |
| `mountableSides(maze, cell, shortcuts).empty()` | komórka bez ściany do powieszenia odpada |
| `std::mt19937 leverGenerator(seed + LEVER_SEED_OFFSET)` | własny generator dźwigni |
| dwa tasowania, potem sklejenie | wszystkie bez kryształu przed wszystkimi z kryształem |
| `min(shortcuts.size(), leverCells.size())` | mniej dźwigni, gdy brakuje skrótów albo komórek |
| `randomMountSide(..., leverGenerator)` | po jednym losowaniu strony na dźwignię, **po** obu tasowaniach: **kolejność losowań jest ustalona** (komentarz) |
| `lever.opens = shortcuts[i]` | najlepszy skrót ma pierwszą dźwignię |
| `takenCells` | komórki dźwigni, które nie wchodzą w grę dla kartek |
| `i % NOTE_KIND_COUNT` | rodzaje na zmianę |
| `(firstLine + flavourNotes) % lineCount` | kolejne linie tabeli od wylosowanej, z zawinięciem |

Ziarno jest parametrem funkcji (labirynt go nie niesie): w testach jest to ziarno, z którego powstał labirynt, a generatory dostają je zwiększone o przesunięcia.

### 5.7 Pozycja, pudełko i teren

```cpp
glm::vec3 mountPosition(const WallRef& mount, float groundHeight, float mountHeight) {
    const float toFace = CELL_SIZE / 2.0F - WALL_VISUAL_THICKNESS / 2.0F;
    const glm::vec3 towardsWall{static_cast<float>(columnStep(mount.side)), 0.0F,
                                static_cast<float>(rowStep(mount.side))};
    return cellCenter(mount.cell.x, mount.cell.z) + towardsWall * toFace +
           glm::vec3{0.0F, groundHeight + mountHeight, 0.0F};
}

scene::Aabb mountBox(const glm::vec3& position, Direction side, float width, float height, float depth) {
    const glm::vec3 towardsWall{...};
    const glm::vec3 center = position - towardsWall * (depth / 2.0F);
    const bool wallRunsAlongX = side == Direction::North || side == Direction::South;
    const glm::vec3 halfExtents = wallRunsAlongX
        ? glm::vec3{width / 2.0F, height / 2.0F, depth / 2.0F}
        : glm::vec3{depth / 2.0F, height / 2.0F, width / 2.0F};
    return scene::Aabb::fromCenter(center, halfExtents);
}
```

| Linia | Znaczenie |
|---|---|
| `toFace = CELL_SIZE / 2 - WALL_VISUAL_THICKNESS / 2` | od środka komórki do lica ściany: 0,9 m |
| `towardsWall * toFace` | przesunięcie w stronę ściany |
| `groundHeight + mountHeight` | wysokość podana z zewnątrz, jak w `crystalRestPosition`: funkcja nie potrzebuje terenu |
| `center = position - towardsWall * depth / 2` | środek pudełka leży pół głębokości od ściany, w komórce |
| `wallRunsAlongX` | dla stron `North` i `South` szerokość jest wzdłuż `X`, głębokość wzdłuż `Z`, a dla `East` i `West` odwrotnie |
| `Aabb::fromCenter(center, halfExtents)` | pudełko ze środka i połówek rozmiarów ([`../scene/collision.md`](../scene/collision.md), sekcja 2.1) |

`leverPosition`, `leverBox`, `notePosition` i `noteBox` to cienkie opakowania z odpowiednimi stałymi. `placeInteractablesOnTerrain` przechodzi po dźwigniach i kartkach, czyta grunt (`terrain.heightAt(position.x, position.z)`: `x` i `z` nie zmieniają się, więc stara pozycja wskazuje miejsce) i liczy pozycję oraz pudełko od nowa.

### 5.8 Stan i pociąganie

Pełen opis działania w sekcji 2.9. W kodzie:

```cpp
PullResult pullLever(InteractableState& state, const Interactables& interactables, std::size_t index) {
    if (index >= interactables.levers.size() || index >= state.leverPulled.size()) {
        throw std::out_of_range("pullLever: there is no lever with this number");
    }
    if (state.leverPulled[index]) {
        return {};
    }
    state.leverPulled[index] = true;
    return {.opened = true, .wall = interactables.levers[index].opens};
}
```

| Linia | Znaczenie |
|---|---|
| warunek z `size()` dwóch list | wyjątek, gdy numer nie istnieje albo stan jest za krótki (nie zaczęty dla tych dźwigni) |
| `return {};` | pociągnięta wcześniej: nic się nie dzieje i nic nie jest zgłaszane |
| `{.opened = true, .wall = ...}` | pierwsze pociągnięcie zgłasza ścianę do opadnięcia |

`isLeverPulled`, `openedWalls` i `openedWallSegment` są opisane w sekcji 2.9. W grze `pullLever` woła `pullRoundLever` (sekcja 2.16), a ściany otwarte dźwigniami zna `openedWallFlags`, nie `openedWalls` (które jest już tylko wynikiem części 1 z testem).

### 5.9 `pickInteractable`

Sekcja 2.10 opisuje algorytm. W kodzie dwie listy pudełek (`leverBoxes`, `noteBoxes`) powstają z pętli po `interactables.levers` i `interactables.notes`, potem:

```cpp
const scene::NearestHit lever = scene::nearestHit(ray, leverBoxes, reach);
const scene::NearestHit note = scene::nearestHit(ray, noteBoxes, reach);

PickedInteractable picked;
if (lever.hit) { picked = {.kind = InteractableKind::Lever, .index = lever.index, .distance = lever.distance}; }
if (note.hit && (!lever.hit || note.distance < lever.distance)) {
    picked = {.kind = InteractableKind::Note, .index = note.index, .distance = note.distance};
}
if (picked.kind == InteractableKind::None) { return picked; }

const scene::NearestHit blocker = scene::nearestHit(ray, blockers, picked.distance);
if (blocker.hit && blocker.distance < picked.distance) { return {}; }
return picked;
```

| Linia | Znaczenie |
|---|---|
| `nearestHit(ray, leverBoxes, reach)` | najbliższa dźwignia w zasięgu. Indeks listy to numer dźwigni |
| `note.distance < lever.distance` | kartka wygrywa tylko, gdy jest ostro bliżej |
| `nearestHit(ray, blockers, picked.distance)` | przesłaniacze liczą się tylko do odległości trafionej rzeczy |
| `blocker.distance < picked.distance` | ostro bliżej: wtedy `None`. Przesłaniacz w tej samej odległości jej nie zasłania |

Koszt: dwa przejścia po dźwigniach i kartkach i jedno po przesłaniaczach. Obie listy pudełek są budowane od nowa przy każdym wołaniu.

### 5.10 `compassTowards`, `compassName`, `noteText`

Sekcje 2.11 i 2.12 opisują działanie. W kodzie `compassTowards` to cztery porównania, a `noteText` trzy gałęzie według `note.kind`. `flavourLine` i `flavourLineCount` obsługują tabelę. `nearestCell` liczy kwadraty odległości na liczbach całkowitych, z `-1` jako znacznikiem "jeszcze nic nie znaleziono".

### 5.11 Jak to zostało sprawdzone

Wszystkie 29 przypadków jest w `tests/InteractablesTests.cpp` i nie wymaga okna ani OpenGL. Liczby to wynik policzenia `TEST_CASE` w pliku, a to, że przechodzą, zgłosił autor kodu (sekcja 1).

| Przypadek testowy | Co przypina |
|---|---|
| `the default maze gets two levers and three notes, one of each kind` | stałe domyślne, 2 dźwignie, 3 kartki po jednej z rodzaju, pełna kontrola reguł (`checkPlacement`) |
| `a wall has two names, and sameWall knows both` | dwie nazwy ściany, `==` a `sameWall` |
| `an interior wall exists and stands between two cells of the maze` | granica nie jest wewnętrzna, usunięta ściana nie jest ścianą, komórka poza labiryntem rzuca |
| `golden maze: 4 x 4 cells from seed 1 has exactly one wall worth a lever` | sekcja 2.6: wyniki 10, 8 i 6, jedna ściana nawet przy trzech zamówionych, 0 i liczby ujemne dają pustą listę, labirynt wołającego bez zmian |
| `the first shortcut is the wall that saves the most, the first one in row order` | najlepszy wynik i kolejność wierszy dla 10 ziaren labiryntu 9 na 6 |
| `a maze without a wall worth opening gets no lever` | sekcja 2.13: 1 na 1, korytarze, 2 na 1, 2 na 2 |
| `every rule of the placement holds, over many seeds and sizes` | osiem rozmiarów (od 3 na 3 do 40 na 40) razy 12 ziaren, zamówienie 5 dźwigni i 7 kartek |
| `the default maze size gets all its levers, in cells without a crystal` | 40 ziaren labiryntu 10 na 10: zawsze 2 dźwignie i 3 kartki, dźwignie w komórkach bez kryształu |
| `a lever goes into a cell with a crystal only when no other cell is left` | kryształ w każdej komórce poza jedną: dźwignia musi wziąć tę jedną, a przy kryształach wszędzie dźwignia nie znika |
| `the same maze and seed give the same levers and notes` | powtarzalność, 10 ziaren |
| `another seed gives other levers and notes` | inne ziarno daje inny labirynt albo, dla tego samego labiryntu, **te same ściany** i inne miejsca |
| `the number of notes does not move the levers` | osobne generatory |
| `the wanted numbers are kept between zero and the largest allowed` | 0 i liczby ujemne, 1000 obcięte do 16 |
| `the largest maze gets its levers and notes` | 256 na 256 |
| `a start or an exit outside the maze is an error` | `std::out_of_range` z obu funkcji |
| `a lever hangs on the visible face of its wall, a hand high above the ground` | pozycje i pudełka z sekcji 2.8 |
| `a note hangs like a lever, a little below the eyes, with a box of its own size` | pozycja i pudełko kartki, `NOTE_MOUNT_HEIGHT < Player::EYE_HEIGHT` |
| `levers and notes follow the height of the terrain` | teren z mapą 2 na 2, nic nie przesuwa się w bok, powtórzenie niczego nie zmienia, płaski teren przywraca |
| `a round starts with no lever pulled, and a lever opens its wall once` | sekcja 2.9 |
| `pulling a lever that does not exist is an error` | numer poza zakresem, stan niezaczęty |
| `the wall a lever opens is one of the wall segments of the maze` | `openedWallSegment` zgodne z `wallSegments`, dokładnie jedno dopasowanie |
| `the picking ray finds the lever or the note it points at` | dziewięć scenariuszy wskazywania, w tym przesłaniacze (sekcja 2.10) |
| `every lever and note of a maze can be picked from the middle of its cell` | 10 ziaren, 4 dźwignie i 8 kartek, wszystkie ściany i słupki jako przesłaniacze |
| `compassTowards names the eight directions and the cell itself` | osiem kierunków i `Here` |
| `a direction is straight when one offset is more than twice the other` | granica 1 do 2 |
| `the compass directions have English names` | nazwy |
| `the flavour lines are short plain text` | tabela: długość, ASCII, brak powtórek, wyjątek |
| `a note says where the exit is, where the nearest crystal is, or a flavour line` | teksty kartek (sekcja 2.12) |
| `the flavour notes of a maze take different lines until the table runs out` | 12 kartek, cztery linie bez powtórek |

W teście pomocniczym `checkPlacement` każde rozmieszczenie jest sprawdzane pełną listą reguł: dźwignie nie wiszą w starcie ani w wyjściu, ściana istnieje, żadna dźwignia jej nie otwiera, pozycja jest na licu, pudełko jest w komórce i wystaje przed pudełko kolizji ściany, ściana do otwarcia jest wewnętrzna, nazwana z `East` albo `South`, nie należy do wyjścia, ma wynik co najmniej 6 **po otwarciu wcześniejszych**, żadne dwie dźwignie nie dzielą komórki ani ściany, a kartki mają rodzaje po kolei, własne komórki i prawidłowy numer linii.

**Część 2: 21 nowych przypadków w `tests/InteractionTests.cpp`** (policzone z pliku: 21 makr `TEST_CASE`). Plik używa świata 10 na 10 z ziarna 1 (`defaultWorld`), który ma dźwignie (testy, które ich potrzebują, najpierw to sprawdzają).

| Przypadek testowy | Co przypina | Sekcja |
|---|---|---|
| `a maze world holds the levers and notes of its seed and the wall of every lever` | świat zawiera to, co daje `placeInteractables`, 3 kartki, a `leverWalls` wskazuje właściwy segment | 2.14 |
| `the settings of a maze world decide how many levers and notes it gets` | 0 i 0, 1 i 7, domyślne `MazeSettings` | 2.14 |
| `levers and notes of a world hang above its terrain, also after a rebuild` | wysokość nad gruntem, pudełko dźwigni na środku jej wysokości, `x` i `z` bez zmian | 2.14 |
| `a new round has no lever pulled, every wall standing and no note open` | stan początkowy, `roundMaze` rundy nie zaczętej | 2.15 |
| `the sink formulas reach the full depth in 1.5 s and stop there` | `sinkDepth`, `sinkProgressAfter`, 180 kroków daje 1 | 2.18 |
| `pulling a lever opens its wall once: obstacles, maze of the round, sinking` | pociągnięcie, lista przeszkód, kopia labiryntu, opadanie w połowie i do końca | 2.16, 2.18 |
| `the handle of a lever swings down in 0.3 s after the pull` | 18 kroków daje 0,5, nieznana dźwignia daje 0 | 2.19 |
| `the view passes an opened wall, and a restart brings every wall back` | przeszukiwanie widzi za otwartą ścianą, restart przywraca | 2.16 |
| `pull all levers opens every wall and counts them` | `pullAllLevers`, powtórka daje 0 | 2.16 |
| `the minimap drops an opened wall and marks levers and notes` | liczby wierzchołków kolorów przed i po, nic nieodkryte | 2.22 |
| `the picking ray starts in the eye and keeps its direction` | `rayFromEye` | picking.md, 2.10 |
| `every lever and note of a world is picked from the middle of its cell` | wskazanie każdej dźwigni i kartki | picking.md, 5.14 |
| `a closed wall hides what is behind it, the wall a lever opened does not` | zasłanianie przez ścianę, a po pociągnięciu brak | picking.md, 5.14 |
| `interacting pulls a lever once and tells when a wall opened` | `interact` | picking.md, 5.14 |
| `a note opens its card, the key closes it first, and walking away closes it` | karta | 2.20 |
| `the card of a crystal hint counts only the crystals that are left` | tekst z niezebranych kryształów | 2.20 |
| `a won round has nothing to interact with and closes the card` | tylko `interactionFor` w rundzie wygranej (zamknięcie karty przy wygranej nie jest sprawdzone) | 2.20 |
| `every action has its prompt` | teksty podpowiedzi | picking.md |
| `the highlight pulses between its weakest and its strongest glow` | zakres pulsu | picking.md |
| `a model on a wall points away from the wall on every side of the cell` | macierz mocowania | 2.19 |
| `the handle of a lever turns around its pivot: up before the pull, down after` | macierz uchwytu | 2.19 |

**Czego testy nie obejmują:** rysowania (`InteractableRenderer`), HUD, panelu, pętli klatki (`pickForFrame`, `handleInteraction`), cieni, wyglądu modeli ani tego, że wygrana zamyka kartę.

**Zrzuty ekranu agenta.** Agent, który napisał kod, uruchomił grę, sterował nią skryptem i oglądał zrzuty ekranu. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela:**

- płytka dźwigni na ścianie z pierścieniem celownika i podpowiedzią "E: pull lever",
- po E uchwyt opuszczony i podpowiedź zniknęła,
- podświetlenie pulsuje (silniejsze przy wyłączonej latarce),
- kartka z liniami atramentu, jej karta z podpowiedzią i napisem "E: close", a po odejściu karta zniknęła,
- ściana otwarta, minimapa przestaje ją rysować i pokazuje korytarz za nią, gracz przechodzi,
- po R minimapa wraca do stanu początkowego i ściana znowu blokuje,
- zamrożony promień jako zielona linia, pudełko trafienia na zielono,
- panel Collision z początkiem, kierunkiem i "Hit: lever 0 at 0.72 m",
- klik wolnym kursorem na dźwigni pociągnął ją.

Po przeróbce modelu dźwigni agent widział też (zrzuty z 2026-10-06, nie właściciel): dźwignię od przodu z 1 m przy włączonej latarce, w górnym położeniu i podświetloną (ciemna płyta, jasna bursztynowa gałka nad środkiem, podpowiedź na dole pośrodku); ten sam widok po E (pręt w dół, gałka poniżej krawędzi płyty z cieniem na ścianie, bez podświetlenia i podpowiedzi); widoki z boku w górnym i dolnym położeniu; wyłączoną latarkę (pociągnięta: płyta prawie czarna, gałka ciemna ochra; w górze z podświetleniem: gałka świeci bursztynem); około 2,4 m pod kątem z podświetleniem i podpowiedzią oraz około 2,9 m poza zasięgiem bez podświetlenia; drugą dźwignię; Gouraud (z podświetleniem) i Unlit; widok Normals (dźwignia jako dane, bez podświetlenia, pierścień i podpowiedź nadal są); kartkę z "E: read note" na dole, wolną od arkusza; otwartą kartę kartki bez podpowiedzi i bez nakładania się na minimapę.

**Czego agent NIE widział na ekranie:** uchwyt w połowie ruchu, cień księżyca od dźwigni, Gouraud i Unlit dla kartki, widok na wprost z 2,5 m, dodatki w panelu Gameplay i przycisk "Pull all levers", inne niż domyślne liczby dźwigni i kartek, czysta rama połowicznie zatopionej ściany. Znane uwagi kosmetyczne: z 1 m na wprost gałka w górnym położeniu zasłania górną trzecią część płyty, a przy wyłączonej latarce i bez podświetlenia płyta jest prawie czarna na ścianie i niesie ją tylko gałka. **Ręczny test właściciela i macOS: otwarte** ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 24.2).

### 5.12 Uwagi o komentarzach w kodzie

W części 1 komentarze opisywały plan w czasie teraźniejszym (opis planu zamiast stanu). W części 2 komentarze w `Interactables.hpp` zostały poprawione, żeby mówiły prawdę: `InteractableSettings` (są polem `MazeSettings`, edytuje je panel Maze, czyta `buildMazeWorld`), `Interactables` (jest polem `MazeWorld`), `InteractableState` (jest polem `Round`) i komentarz w `tests/InteractablesTests.cpp` (numer ściany znajduje `buildMazeWorld`, `MazeWorld::leverWalls`). Zostały dwie drobne rzeczy:

- Komentarz do `openedWalls` w nagłówku opisuje budowę listy przeszkód przez kod rundy. Dziś robi to `roundObstacles`, ale przez `openedWallFlags`, a nie przez `openedWalls` (obserwacja: komentarz jest bliski prawdy, ale nazwa funkcji jest inna),
- `Interaction.hpp` odsyła w nagłówku do [`../scene/picking.md`](../scene/picking.md), ale zawiera też macierze modeli, które opisuje sekcja 2.19 tego dokumentu.

### 5.13 Podpięcie do gry: co jest, czego nie ma

Lista z części 1 ("co zostaje do podpięcia") jest **zrobiona**. Gdzie każdy punkt wylądował:

| Dawny punkt | Gdzie jest teraz |
|---|---|
| `MazeWorld`: `placeInteractables`, `placeInteractablesOnTerrain` | `buildMazeWorld`, `placeOnTerrain` (sekcja 2.14) |
| `Round`: `InteractableState` | `Round::interactables`, `startRound` (sekcja 2.15) |
| `roundObstacles` i przesłaniacze | `roundObstacles` bez pudełek otwartych ścian, ta sama lista dla gracza i promienia (sekcja 2.17) |
| animacja opadania ściany | `wallProgress`, `sinkProgressAfter`, `sinkDepth`, `roundWallMatrices` (sekcja 2.18) |
| rysowanie dźwigni i kartek | `InteractableRenderer`, `mountModelMatrix`, `leverHandleMatrix` (sekcja 2.19) |
| wejście | klawisz E i lewy klik, `handleInteraction` ([`../scene/picking.md`](../scene/picking.md), sekcja 5.11) |
| karta HUD z tekstem kartki | `drawNoteCard` w `Hud.cpp`, `openNoteText` (sekcja 2.20) |
| panel | pola Levers i Notes (Maze), "Pull all levers" (Gameplay), ostatni promień (Collision) (sekcja 2.22) |
| oś czasu rundy: restart zeruje stan | `startRound` zaczyna każdą rundę od nowa, także po R i po regeneracji |

Czego nadal **nie ma**: nowych dźwigni po starcie rundy, odwracania dźwigni (pociągnięta zostaje pociągnięta do końca rundy), trawy i słupków dostosowanych do otwartego przejścia (sekcja 2.21), dźwięku i zakończonego, ręcznie sprawdzonego przez właściciela wyglądu modelu dźwigni.

## 6. Panel ImGui

Dodatki w panelach opisuje sekcja 2.22: **Gameplay** ("Levers: %d pulled of %d", "Note card", przycisk "Pull all levers"), **Maze** (suwaki Levers i Notes, plan z kwadratami, przyciemniona otwarta ściana) i **Collision** (ostatni promień, [`../scene/picking.md`](../scene/picking.md), sekcja 6). Karta kartki, celownik i podpowiedź są w HUD ([`../debug-ui.md`](../debug-ui.md)).

**Scenariusz pokazu na obronie.**

1. Otwórz panel **Maze**: suwaki Levers i Notes (domyślnie 2 i 3), na planie czerwone i jasne kwadraty. Ustaw Levers na 4, kliknij **Regenerate**: nowy labirynt, nowa runda. Tekst "Levers: n, notes: n" pokaże, ile labirynt naprawdę dostał (może być mniej).
2. Podejdź do dźwigni, zobacz pierścień celownika i podpowiedź. Naciśnij E: uchwyt opada w 0,3 s, ściana opada w 1,5 s. Plan w panelu Maze rysuje tę ścianę przyciemnioną, minimapa przestaje ją rysować, a w panelu Gameplay jest "Levers: 1 pulled of ...".
3. W panelu Gameplay naciśnij **Pull all levers**: pociągnięte zostają wszystkie. **R** zaczyna rundę od nowa i ściany wracają.
4. Podejdź do kartki i naciśnij E: karta z podpowiedzią. Odejdź na ponad 3 m: karta znika.

Z tego scenariusza agent widział na zrzutach tylko część (sekcja 5.11): nie widział przycisku "Pull all levers" i panelu Gameplay, ani drugiej dźwigni, ani liczb innych niż domyślne. Reszta to zadanie na listę ręczną właściciela.

## 7. Pułapki

1. **Dwie nazwy ściany.** Porównanie `==` na `WallRef` mówi "różne" dla tej samej ściany widzianej z dwóch komórek. Do pytania "czy to ta sama ściana" służy `sameWall`. Pomylenie ich powiesiłoby dźwignię na ścianie, którą otwiera inna dźwignia.
2. **Wynik liczy się od startu.** Ściana, która skraca drogę od startu, nie musi skracać drogi od miejsca, w którym gracz stoi. Wynik opisuje tylko początek rundy.
3. **Wynik jest parzysty.** Próg 6 jest równoważny "więcej niż 5". Próg nieparzysty (na przykład 5) działałby tak samo jak 6.
4. **Brak testu "złotego" dla rozmieszczenia.** Złoty labirynt przypina dokładnie ściany, a test rozmieszczenia sprawdza tylko własności i powtarzalność na jednej maszynie. Zgodność macOS i Windows opiera się na generatorze `std::mt19937` i na `randomBelow` ([`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md)). Kolejność losowań (dwa tasowania, potem po jednym losowaniu strony na dźwignię; tasowanie, numer linii, strony kartek) jest częścią wyniku: zmiana kolejności zmienia rozmieszczenie dla każdego ziarna.
5. **Mniej dźwigni niż zamówiono to normalne.** Mały labirynt często nie ma żadnej, a w labiryncie 4 na 4 z ziarna 1 jest tylko jedna. Wołający nie może zakładać, że `levers.size() == settings.leverCount`.
6. **Dźwignia jest celem dla promienia, a nie przeszkodą dla gracza.** Jej pudełko nie jest na liście przeszkód i gracza nigdy nie zatrzymuje, ale jest na liście pudełek wskazywania.
7. **Pudełko płytsze niż 5 cm** zniknęłoby w pudełku ściany i ściana zasłaniałaby to, co na niej wisi. Pilnują tego dwa `static_assert`. Przy zmianie grubości widocznej ściany albo pudełka kolizji trzeba zmienić `MOUNT_BOX_MIN_DEPTH` razem z nimi (jest liczone z obu stałych, więc zmieni się samo).
8. **Wysokości dla płaskiego gruntu.** `placeInteractables` daje `y` dla gruntu 0. Teren trzeba nałożyć osobno (`placeInteractablesOnTerrain`) i powtórzyć po każdej przebudowie terenu.
9. **`PullResult` przy powtórnym pociągnięciu ma pole `wall` bez znaczenia** (domyślna ściana `(0, 0)` `North`). Zawsze najpierw sprawdzić `opened`.
10. **Stan musi pasować do dźwigni.** `startInteractables` zaczyna stan dla konkretnych `Interactables`. Stan z innej liczby dźwigni rzuca `std::out_of_range` przy `pullLever` i `isLeverPulled`, a `openedWalls` po cichu liczy tylko do krótszej listy.
11. **Podpowiedź do kryształu wymaga listy kryształów, które zostały.** Przekazanie wszystkich komórek kryształów, także zebranych, dałoby kartkę, która wskazuje kryształ, którego już nie ma.
12. **`Here` dla wyjścia nie wystąpi w grze**, bo w komórce wyjścia nic nie wisi (jest w teście tylko dlatego, że ma odpowiedź: `"The exit lies right here."`). `Here` dla kryształu jest możliwe: kartka może wisieć w komórce z kryształem.
13. **Przesłaniacz w tej samej odległości nie zasłania.** `blocker.distance < picked.distance`: ostro. A przesłaniacz, w którym leży początek promienia, ma odległość 0 i zasłania wszystko ([`../scene/picking.md`](../scene/picking.md), pułapka 10).
14. **Zasięg od oka.** W grze `rayFromEye` przenosi początek promienia do oka, więc 2,5 m to odległość od oka. Testy z części 1 też rzucają promienie z oczu ([`../scene/picking.md`](../scene/picking.md), sekcja 2.10).
15. **Pudełko wskazywania, nie model.** Kartka o szerokości 0,4 m i wysokości 0,5 m jest trafiona także w rogu pudełka, w którym model już nie sięga.
16. **Teksty są po angielsku, tylko ASCII, do 60 znaków.** To wymaganie karty HUD, sprawdzane testem. Polskie znaki nie przeszłyby.
17. **Ziarno jest osobnym parametrem.** `placeInteractables` dostaje `seed` jako argument, nie bierze go z labiryntu. W testach podaje się ziarno labiryntu, tak jak dla `placeCrystals`, ale nic w kodzie tego nie wymusza.
18. **Rysunek (i ekran) nie jest dowodem.** Zrzuty agenta pokazują, że rzecz działa na ekranie, ale nie zastępują testu ani ręcznego sprawdzenia właściciela (sekcja 5.11).
19. **Ściana przestaje blokować przed zniknięciem z ekranu.** Przestaje w chwili pociągnięcia, a znika z ekranu po 1,5 s. Przez ten czas gracz przechodzi przez ścianę, którą jeszcze widzi (sekcja 2.16).
20. **`world.maze` nie jest labiryntem rundy.** Kto czyta ściany do logiki rundy (przeszukiwanie, minimapa), ma użyć `roundMaze(world, round)`. `world.maze` zawsze ma wszystkie ściany. Minimapa dostaje z `world.maze` tylko rozmiar (do rzutowania), a wierzchołki z kopii rundy.
21. **`pullAllLevers` to dwie różne rzeczy.** Pole `GameplaySettings::pullAllLevers` to flaga prośby z panelu, a `game::pullAllLevers(round, world)` to funkcja, która pociąga dźwignie.
22. **Lista macierzy ścian ma ten sam rozmiar co ściany świata.** Całkiem opadła ściana jest w niej nadal. Kto wyrzuci ją z listy, przesunie numery ścian względem `leverWalls`.
23. **Karta zamyka się przy kliknięciu bez przechwytywania kursora.** Wolny kursor plus otwarta karta: klik zamyka kartę i kursor zostaje wolny, a pierścień i podpowiedź nie świecą, dopóki karta jest otwarta.
24. **Trawa i słupek przy otwartej ścianie** (sekcja 2.21).

## 8. Ćwiczenia

Ćwiczenia od 1 do 8, 13 i 14 robi się na kartce. Ćwiczenia od 9 do 12 i 15 to zmiany w kodzie albo w testach: po każdej zmianie w kodzie zbuduj projekt i uruchom testy (`cmake --build --preset debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure`), a na końcu wycofaj swoją zmianę. Wyniki oznaczone jako "przewidywanie" są wyprowadzone z kodu, nie zmierzone.

1. **Nazwy ściany.** Podaj drugą nazwę ściany wschodniej komórki `(4, 2)` i południowej komórki `(1, 1)`. Odpowiedź: zachodnia komórki `(5, 2)` i północna komórki `(1, 2)`.
2. **Wynik ściany.** Dwie sąsiednie komórki mają odległości od startu 3 i 12. Jaki wynik ma ściana między nimi? Odpowiedź: `|3 - 12| - 1 = 8`. Czy odległości 3 i 11 są możliwe? Odpowiedź: nie, bo różnica sąsiednich komórek jest nieparzysta (szachownica), a 8 jest parzyste.
3. **Z rysunku.** W labiryncie 4 na 4 z sekcji 2.6 policz wynik ściany wschodniej komórki `(1, 2)` i południowej komórki `(2, 2)`. Odpowiedź: odległości 3 i 8, wynik 4, oraz 8 i 5, wynik 2.
4. **Po otwarciu.** Po otwarciu ściany `(0, 0)` East w tym labiryncie podaj nowe odległości `(1, 1)` i `(2, 1)`. Odpowiedź: 2 i 3. Jaki jest teraz wynik ściany `(1, 1)` South (druga komórka `(1, 2)` ma odległość 3)? Odpowiedź: `|2 - 3| - 1 = 0`.
5. **Labirynt 3 na 3.** Dla labiryntu 3 na 3 z ziarna 7 (rysunek w `maze-generator.md`) policz odległości od `(0, 0)`, wyjście i najlepszy wynik. Odpowiedź: odległości `(0,1)` 1, `(1,1)` 2, `(2,1)` 3, `(2,0)` 4, `(2,2)` 4, `(1,0)` 5, `(1,2)` 5, `(0,2)` 6, wyjście `(0, 2)`, najlepsza ściana `(0, 0)` East z wynikiem 4: brak dźwigni.
6. **Kompas.** Podaj `compassTowards` od `(3, 3)` do `(9, 5)`, do `(6, 7)`, do `(3, 3)` i do `(1, 4)`. Odpowiedź: przesunięcia `(6, 2)`: `6 > 4`, więc `East`. `(3, 4)`: `3 > 8` nie, `4 > 6` nie, przekątna na południowy wschód: `SouthEast`. `(0, 0)`: `Here`. `(-2, 1)`: `2 > 2` nie, `1 > 4` nie, przekątna na południowy zachód: `SouthWest`.
7. **Pudełko.** Dźwignia wisi na wschodniej ścianie komórki `(1, 1)` na gruncie 0. Podaj `position` i pudełko. Odpowiedź: środek komórki `(3, 0, 3)`, `position = (3,9, 1,2, 3)`, środek pudełka `(3,775, 1,2, 3)`, połówki `(0,125, 0,2, 0,15)`, więc `min = (3,65, 1,0, 2,85)` i `max = (3,9, 1,4, 3,15)`.
8. **Tasowanie.** Lista komórek to `[A, B, C, D]`, a `randomBelow` zwraca kolejno 1, 0, 0 dla `last` równego 4, 3 i 2. Jaka jest lista po tasowaniu? Odpowiedź: `last = 4`, wybrane 1: zamiana miejsc 3 i 1, `[A, D, C, B]`. `last = 3`, wybrane 0: zamiana miejsc 2 i 0, `[C, D, A, B]`. `last = 2`, wybrane 0: zamiana miejsc 1 i 0, `[D, C, A, B]`.
9. **Za głęboki próg.** W `Interactables.hpp` zmień `LEVER_MIN_STEPS_SAVED` na 2 i uruchom testy. Przewidywanie: nie przechodzi `golden maze: 4 x 4 cells from seed 1...`, bo po otwarciu pierwszego skrótu ściana `(0, 2)` South ma wynik 2 i trzecie wywołanie (`three`) zwraca więcej niż jedną ścianę (`REQUIRE(three.size() == 1U)`). Sprawdź, czy tylko ten przypadek.
10. **Bez reguły komórki wyjścia.** Usuń z `chooseShortcutWalls` warunek `wall.cell == exit || behind == exit` i uruchom testy. Przewidywanie: w tym labiryncie 2 na 2 nic się nie zmieni (ściana oszczędza najwyżej 2 przejścia, a próg to 6), więc przypadek `a maze without a wall worth opening gets no lever` nadal przejdzie. Natomiast wyjście to najdalsza komórka, a jej sąsiad za ścianą bywa blisko startu, więc taka ściana ma wysoki wynik i może zostać wybrana: spodziewam się porażki sprawdzeń `CHECK_FALSE(lever.opens.cell == test.exit)` i `CHECK_FALSE(cellBehind(lever.opens) == test.exit)` w `checkPlacement` dla niektórych ziaren. Sprawdź, dla których, i wyjaśnij, dlaczego ta reguła w ogóle istnieje (komentarz w kodzie o bramie).
11. **Za płytkie pudełko.** Zmień `LEVER_BOX_DEPTH` na `0.04F` i zbuduj. Przewidywanie: błąd kompilacji z `static_assert(LEVER_BOX_DEPTH > MOUNT_BOX_MIN_DEPTH)`. Dlaczego kompilator, a nie test, pilnuje tej reguły?
12. **Własny test.** Dopisz w `tests/InteractablesTests.cpp` przypadek, w którym kartka i dźwignia leżą w dokładnie tej samej odległości od początku promienia, i sprawdź, że `pickInteractable` zwraca dźwignię. Wzoruj się na podprzypadku `of a note and a lever in the line of the ray the nearer one is picked` (dziś sprawdza tylko, że **bliższa** wygrywa, a nie remis).

13. **Opadanie na kartce.** Pociągnięto dźwignię, a od tej chwili minęło 0,6 s. Jaki jest `wallProgress`, jak głęboko jest ściana i jaki jest postęp uchwytu? Odpowiedź: `0,6 / 1,5 = 0,4`, głębokość `0,4 * 3,3 = 1,32` m, uchwyt `min(0,6 / 0,3; 1) = 1` (opuszczony).
14. **Trzy listy, jeden numer.** Dźwignia 1 ma `leverWalls[1] = 7`. Którą macierz, które pudełko i którą flagę z `openedWallFlags` zmienia jej pociągnięcie? Odpowiedź: macierz `wallMatrices[7]` (w `roundWallMatrices`), pudełko `colliders[7]` (wypada z `roundObstacles`) i flagę numer 7. Wszystkie trzy listy są w kolejności `world.walls`.
15. **Własna zmiana.** W `NOTE_READ_DISTANCE` ustaw 2,0 i przeczytaj test `a note opens its card, the key closes it first, and walking away closes it`. Przewidywanie: nadal przechodzi (test używa stałej, a nie liczby). Zastanów się, czy 2,0 m jest rozsądne wobec zasięgu 2,5 m: karta zamykałaby się, zanim gracz wyjdzie poza zasięg wskazywania. Wycofaj zmianę.

## 9. Pytania kontrolne

1. **Co to jest skrót w labiryncie doskonałym?**
   Pętla powstała przez usunięcie ściany wewnętrznej między dwiema komórkami, które nie były połączone bezpośrednio. Daje drugą drogę między nimi i skraca drogę do dalszej z nich.

2. **Dlaczego ściana ma dwie nazwy i czym różni się `==` od `sameWall`?**
   Ściana stoi na krawędzi między dwiema komórkami, więc każda ją widzi ze swojej strony: wschód jednej to zachód drugiej. `==` na `WallRef` porównuje nazwy, więc te dwie są różne, a `sameWall` zna obie i mówi prawdę o ścianie.

3. **Jak liczony jest wynik ściany?**
   Odległości dwóch komórek ściany od startu (przeszukiwanie wszerz) odejmuję, biorę wartość bezwzględną i odejmuję 1: `|a - b| - 1`. To liczba przejść, o którą skraca się droga do dalszej komórki.

4. **Dlaczego wynik jest zawsze parzysty?**
   Siatka jest szachownicą i każde przejście zmienia kolor pola, więc odległości sąsiednich komórek różnią się o liczbę nieparzystą. Po odjęciu 1 wynik jest parzysty. Dlatego próg 6 znaczy tyle co "więcej niż 5".

5. **Dlaczego odległości liczy się od nowa po każdym wybranym skrócie?**
   Żeby dwie dźwignie nie otwierały prawie tego samego skrótu: po pierwszym otwarciu odległości części komórek maleją, i ściany, które wcześniej dużo oszczędzały, mogą oszczędzać już mało albo nic. W przykładzie 4 na 4 po pierwszym skrócie żadna ściana nie przekracza progu.

6. **Dlaczego nigdy nie otwiera się ściany komórki wyjścia?**
   Brama jest jedyną drogą do wyjścia. Drugie otwarcie pozwoliłoby ją obejść (komentarz w kodzie).

7. **Jak rozstrzygany jest remis wyników?**
   Wygrywa pierwsza ściana w kolejności przeglądania: wiersz po wierszu od zachodu na wschód, w komórce `East` przed `South`. Działa tak, bo porównanie jest ostre (`>`), więc równa nie zastępuje wcześniejszej. Wynik zależy tylko od labiryntu.

8. **Jak wybierana jest komórka dźwigni?**
   Kandydaci to komórki bez startu, wyjścia i bez komórek bez ściany do powieszenia. Dzielą się na bez kryształu i z kryształem, każda lista jest tasowana generatorem zasianym `seed + 5000011`, a obie są sklejone: najpierw bez kryształu. Dźwignia `i` dostaje komórkę `i` z listy.

9. **Dlaczego dźwignie i kartki mają osobne generatory?**
   Żeby inna liczba kartek nigdy nie przesuwała dźwigni. Jeden wspólny generator rozjechałby kolejność losowań.

10. **Dlaczego w tasowaniu nie użyto `std::shuffle`?**
    Standard określa ciąg liczb generatora `std::mt19937`, ale nie to, jak `std::shuffle` i rozkłady go używają. Dwie biblioteki standardowe (MSVC i clang) robią to inaczej i to samo ziarno dałoby inne rozmieszczenie na macOS i na Windowsie. Dlatego własny Fisher i Yates z `randomBelow`.

11. **Gdzie dokładnie wisi dźwignia i jak powstaje jej pudełko?**
    Na widocznym licu ściany, w jej środku, 0,9 m od środka komórki w stronę ściany, na wysokości 1,2 m nad gruntem. Pudełko ma 0,3 m szerokości wzdłuż ściany, 0,4 m wysokości i 0,25 m głębokości w głąb komórki, a jego środek leży pół głębokości od lica.

12. **Dlaczego pudełko musi być głębsze niż 5 cm?**
    Pudełko kolizji ściany (0,3 m) jest grubsze niż widoczna ściana (0,2 m), więc stoi 5 cm przed jej licem. Pudełko wskazywania zaczyna się na licu. Płytsze niż 5 cm leżałoby w pudełku ściany i promień z komórki trafiałby najpierw w ścianę.

13. **Co robi `pullLever` przy drugim pociągnięciu tej samej dźwigni?**
    Niczego nie zmienia i zwraca domyślny wynik (`opened = false`). Pierwsze pociągnięcie ustawia flagę i zwraca ścianę do opadnięcia.

14. **Kiedy `pullLever` rzuca wyjątek?**
    Gdy nie ma dźwigni o tym numerze albo gdy stan nie został zaczęty dla tych dźwigni (jest krótszy niż lista dźwigni).

15. **Jak działa `pickInteractable`?**
    Dwa razy `nearestHit` (dźwignie i kartki) w zasięgu, wybór bliższej (przy remisie dźwignia), a potem trzecie `nearestHit` na liście przesłaniaczy z zasięgiem równym odległości trafionej rzeczy. Przesłaniacz ostro bliżej oznacza `None`.

16. **Dlaczego ściana, na której wisi dźwignia, jej nie zasłania?**
    Bo pudełko dźwigni wystaje przed pudełko kolizji tej ściany: promień z komórki wchodzi w pudełko dźwigni wcześniej, niż dotrze do ściany. Test z ręcznie zbudowaną dźwignią i test z 10 ziaren sprawdzają to z listą wszystkich ścian i słupków jako przesłaniaczy.

17. **Jak działa kompas z liczbami całkowitymi?**
    Biorę wartości bezwzględne przesunięcia kolumn i wierszy. Gdy jedno jest więcej niż dwa razy większe od drugiego, kierunek jest prosty (wschód, zachód, północ, południe), w przeciwnym razie jest to jedna z czterech przekątnych według znaków. Granica 1 do 2 jest blisko 22,5 stopnia, które dałyby osiem równych sektorów.

18. **Co robi dokładnie podwojone przesunięcie, na przykład `(2, -1)`?**
    To jeszcze przekątna (`NorthEast`), bo `2 > 2 * 1` jest fałszem. Dopiero `(3, -1)` jest `East`.

19. **Dlaczego tekst kartki nie jest zapisany w kartce?**
    Bo odpowiedź zmienia się w trakcie rundy: najbliższy kryształ znika, gdy zostanie zebrany, i podpowiedź do niego byłaby kłamstwem. Tekst liczy `noteText` na żądanie z komórek kryształów, które zostały.

20. **Jak przydzielane są rodzaje i linie kartek?**
    Kartka `i` ma rodzaj `i % 3`: wyjście, kryształ, linia. Pierwsza kartka z linią bierze wylosowany numer tabeli, następne kolejne wiersze tabeli z zawinięciem.

21. **Czemu labirynt 2 na 2 nigdy nie ma dźwigni?**
    Drzewo rozpinające czterech komórek to ścieżka, jedyna ściana wewnętrzna łączy jej końce (jednym jest wyjście), a jej wynik to najwyżej 2, poniżej progu 6. Odrzucają ją obie reguły: komórki wyjścia i próg.

22. **Czemu labirynt 3 na 3 z ziarna 7 nie dostaje dźwigni?**
    Najlepsza ściana oszczędza 4 przejścia, a próg to 6.

23. **Co robi pociągnięcie dźwigni w rundzie?**
    `pullRoundLever` ustawia flagę (`pullLever`), usuwa ścianę z kopii labiryntu rundy (`Round::maze`, obie komórki), a `roundObstacles` nie zawiera już jej pudełka (aplikacja odbudowuje listę). Ściana opada przez 1,5 s (`wallProgress`), uchwyt opuszcza się w 0,3 s. Drugie pociągnięcie nic nie zmienia, a zły numer rzuca `std::out_of_range`.

24. **Co w tym module jest decyzją właściciela, a co wyborem implementacji?**
    Właściciel zdecydował (2026-10-06) o ray castingu (środek ekranu przy przechwyconej myszy, kursor przy wolnej), o tym, że dźwignia otwiera skrót, obniżając jeden wewnętrzny segment ściany, i o tym, że kartka pokazuje krótką podpowiedź na karcie HUD. Reszta (wynik z odległości od startu, próg 6, reguła komórki wyjścia, domyślne liczby, zasięg, rozmiary pudełek, osobne ziarna, tekst na żądanie, kompas, a w części 2 moment przestawania blokowania, kopia labiryntu rundy, wspólne wzory opadania, podświetlenie, zasady kliknięć, uchwyt jako osobny model) to wybory implementacji z uzasadnieniami w sekcji 1.1.

25. **Dlaczego runda ma własną kopię labiryntu?**
    Przeszukiwanie i minimapa czytają ściany labiryntu, a otwarta ściana ma zniknąć dla obu. Kopia (`Round::maze`) traci ścianę, a `world.maze` nigdy się nie zmienia, więc nowa runda na tym samym labiryncie ma wszystkie ściany bez odbudowy świata ([`../../decisions/round-keeps-own-maze-copy.md`](../../decisions/round-keeps-own-maze-copy.md)).

26. **Które funkcje mają wspólne brama i ściany otwarte dźwignią?**
    `sinkProgressAfter` i `sinkDepth` ze stałymi `GATE_OPEN_SECONDS` (1,5 s) i `GATE_SINK_DEPTH` (3,3 m). Macierz ściany jest liczona od nowa z obniżonego segmentu, tak jak `GameplayRenderer` obniża bramę.

27. **Dlaczego uchwyt dźwigni jest osobnym modelem?**
    `drawModel` daje wszystkim częściom modelu jedną macierz, a uchwyt jest jedyną częścią, która się rusza (przechyla się między dwoma kątami wokół osi obrotu tuż przed licem ściany; wartości w sekcji 5.2).

28. **Co zamyka kartę kartki?**
    E, lewy klik, odejście na ponad 3,0 m (po gruncie) i wygrana. Nie ma licznika czasu. Restart i nowy labirynt zaczynają rundę od nowa, więc karta też jest zamknięta.

29. **Co sprawdzono w części 2, a co nie?**
    Bramka projektu (zgłoszona przez autora): 466 przypadków i 152264 asercji, w tym 21 nowych w `InteractionTests.cpp`. Agent widział na zrzutach działanie (sekcja 5.11), czego nie widział, jest wymienione. Test właściciela i macOS są otwarte.

## 10. Źródła

- Thomas H. Cormen i in., "Wprowadzenie do algorytmów" (Introduction to Algorithms), rozdział o przeszukiwaniu wszerz (BFS): odległości w przejściach.
- Donald E. Knuth, "The Art of Computer Programming", tom 2, algorytm P (tasowanie, Fisher i Yates): tasowanie komórek.
- cppreference, `std::mersenne_twister_engine` (`std::mt19937`): <https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine>, `std::clamp`: <https://en.cppreference.com/w/cpp/algorithm/clamp>, `std::span`: <https://en.cppreference.com/w/cpp/container/span>.
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `game`), [`../renderer/minimap.md`](../renderer/minimap.md) (ściany z labiryntu rundy, znaczniki), [`../renderer/shadows.md`](../renderer/shadows.md) (macierze ścian w przebiegach cienia), [`maze-rendering.md`](maze-rendering.md) (macierze ścian klatki), [`../../decisions/opened-wall-stops-blocking-at-pull.md`](../../decisions/opened-wall-stops-blocking-at-pull.md), [`../../decisions/round-keeps-own-maze-copy.md`](../../decisions/round-keeps-own-maze-copy.md), [`../../decisions/highlight-as-emissive-pulse.md`](../../decisions/highlight-as-emissive-pulse.md), [`../../decisions/pick-ray-starts-in-the-eye.md`](../../decisions/pick-ray-starts-in-the-eye.md), [`maze-generator.md`](maze-generator.md) (siatka, kierunki, układ w świecie, `randomBelow`), [`gameplay.md`](gameplay.md) (`passageDistances`, wyjście, brama, kryształy, tasowanie), [`../scene/picking.md`](../scene/picking.md) (promień i `nearestHit`), [`../scene/collision.md`](../scene/collision.md) (pudełko ściany grubsze od ściany), [`../renderer/terrain.md`](../renderer/terrain.md) (`Terrain::heightAt`), [`../../decisions/deterministic-random.md`](../../decisions/deterministic-random.md) (dlaczego własne `randomBelow`), [`../../decisions/exit-farthest-cell.md`](../../decisions/exit-farthest-cell.md) (wyjście i brama), [`../../libraries/doctest.md`](../../libraries/doctest.md).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 3 (temat 15).
