# Wyjście: komórka najdalsza od startu w liczbie przejść, a nie przeciwległy narożnik

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Exit.hpp`](../../src/game/Exit.hpp), [`Exit.cpp`](../../src/game/Exit.cpp) (`passageDistances`, `farthestCell`, `placeExit`, `exitZone`), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) (`buildMazeWorld`), testy w [`tests/ExitTests.cpp`](../../tests/ExitTests.cpp). Dokument modułu: [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcje 2 i 5.

## 1. Kontekst

Runda kończy się wejściem w strefę wyjścia za bramą (PRD: "Brama otwiera się po zebraniu N kryształów; wejście w strefę kończy rundę"). PRD nie mówi, **gdzie** w labiryncie jest wyjście.

Od M2 do M4 wyjściem była komórka w narożniku przeciwległym do startu. Był to znacznik miejsca: nic tam nie stało i nic się nie kończyło. W M5 wyjście dostało bramę i strefę, więc trzeba było wybrać komórkę naprawdę.

Wymagania:

- brama ma zamykać wyjście w całości, jednym segmentem,
- droga do wyjścia ma być długa, bo po drodze gracz zbiera kryształy,
- ten sam labirynt ma mieć to samo wyjście na każdym systemie,
- wybór ma działać dla każdego rozmiaru, także 1 na 1.

## 2. Decyzja

Wyjściem jest komórka najdalsza od startu, gdy odległość liczy się w przejściach między komórkami, a nie w linii prostej. Odległości liczy przeszukiwanie wszerz (BFS) od komórki startowej. Przy remisie wygrywa komórka pierwsza w kolejności wierszy. Brama staje na jedynym otwartym boku tej komórki.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Najdalsza komórka według BFS (wybrana)** | najdłuższa możliwa droga od startu. W labiryncie doskonałym to zawsze ślepy zaułek, więc ma dokładnie jeden otwarty bok i jedna brama go zamyka. Bez losowania. BFS to kilkanaście linii, które da się wyjaśnić | wyjście może leżeć blisko startu w linii prostej, za ścianą. Gracz nie wie z góry, w którą stronę iść |
| Przeciwległy narożnik (znacznik z M2 do M4) | zero kodu, gracz zna kierunek | narożnik nie musi być zaułkiem: może mieć dwa otwarte boki i jedna brama go nie zamknie. Droga do niego bywa krótka: w labiryncie wzorcowym 4 na 4 narożnik `(3, 3)` jest 6 przejść od startu, a najdalsza komórka 14 |
| Losowy ślepy zaułek z ziarna | różnorodność, zawsze jeden otwarty bok | może wypaść tuż obok startu. Dodatkowe losowanie do zasiania i przypięcia testem. Długość rundy zależałaby od szczęścia |
| Dwa końce najdłuższej drogi w całym labiryncie (średnica) | najdłuższa droga, jaka w ogóle istnieje | trzeba by przenieść też start, a start w `(0, 0)` jest częścią "formatu" labiryntu: od niego zaczyna generator i na nim stoją testy. Dwa przeszukiwania zamiast jednego |
| Komórka najdalsza w linii prostej | prosty wzór | to prawie zawsze okolice przeciwległego narożnika, z tymi samymi wadami |

## 4. Uzasadnienie i skutki

**Dlaczego zawsze zaułek.** W labiryncie doskonałym między dwiema komórkami jest dokładnie jedna droga. Gdyby najdalsza komórka miała drugie przejście, prowadziłoby ono do komórki o jedno przejście dalszej, więc tamta byłaby dalsza. Najdalsza komórka ma zatem jedno przejście: to, którym się do niej wchodzi. Test sprawdza to na labiryntach 9 na 6 z 25 ziaren: komórka wyjścia jest zaułkiem (`isDeadEnd`), ma jeden otwarty bok i żadna komórka nie jest od niej dalsza.

**Dlaczego to rozwiązuje bramę.** Jeden otwarty bok znaczy, że jeden segment na granicy komórki zamyka ją w całości. Brama stoi dokładnie tam, gdzie stałaby ściana, i ma pudełko kolizji policzone tą samą funkcją co ściana (`wallBox`). `placeExit` bierze pierwszy otwarty bok w stałej kolejności kierunków, a skoro jest jeden, kolejność niczego nie zmienia.

**Dlaczego remis rozstrzyga kolejność wierszy.** Dwie komórki mogą być równie daleko. `farthestCell` przegląda wiersz po wierszu i zamienia kandydata tylko na ściśle dalszego, więc zostaje pierwszy. Wynik nie zależy od kolejności, w jakiej BFS odwiedzał komórki. Jest na to test na labiryncie zbudowanym ręcznie.

**Liczby, które są przypięte testami.** Labirynt wzorcowy 4 na 4 z ziarna 1: wyjście w `(3, 1)`, 14 przejść od startu, brama na północnym boku. Labirynt startowy 10 na 10 z ziarna 1: wyjście w `(6, 5)`, brama na wschodnim boku. Labirynt 1 na 1: wyjściem jest start, bramy nie ma, a stanie w nim wygrywa rundę.

**Co z tego wynika dla reszty.**

- Strefa wyjścia (`exitZone`) to pudełko 1 na 1 m w środku komórki, wysokie jak ściany. Gracz musi wejść za linię bramy, nie tylko jej dotknąć.
- Kryształ nigdy nie stoi w komórce wyjścia: byłby za bramą ([`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md)).
- Generator labiryntu nie dostał żadnego losowania: wyjście wynika z gotowej siatki ([`deterministic-random.md`](deterministic-random.md)).
- BFS liczy się raz na labirynt, w `buildMazeWorld`.

**Co przez to tracę.**

- Wyjście nie ma stałego miejsca. Na planie w panelu Maze je widać, w grze trzeba go szukać.
- "Najdalej w przejściach" nie znaczy "daleko w metrach": w labiryncie wzorcowym wyjście `(3, 1)` leży trzy kolumny i jeden wiersz od startu.
- Najdalsza od startu nie znaczy najdalsza od kryształów: ostatni potrzebny kryształ może wisieć w korytarzu obok bramy.

## 5. Kiedy wrócić do tej decyzji

- Gdy labirynt przestanie być doskonały (pętle albo komórki odcięte). Najdalsza komórka nie musi wtedy być zaułkiem, a `placeExit` stawia bramę tylko na pierwszym otwartym boku: drugi bok zostałby otwarty.
- Gdy start przestanie być stałą komórką `(0, 0)`: wtedy warto wrócić do średnicy labiryntu.
- Gdy dojdzie przeciwnik ([`enemy-after-m5.md`](enemy-after-m5.md)) albo minimapa i miejsce wyjścia zacznie mieć znaczenie dla czegoś więcej niż długość drogi. (Minimapa z 2026-10-06 koloruje komórkę wyjścia, ale ustawia ją tam, gdzie stoi `MazeWorld::exitCell`, i nie zmienia wyboru wyjścia.)
