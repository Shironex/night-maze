# Runda trzyma własną kopię labiryntu, a `MazeWorld::maze` nigdy się nie zmienia

Data: 2026-10-06. Stan: obowiązuje.
Kod: [`src/game/Round.hpp`](../../src/game/Round.hpp) i [`Round.cpp`](../../src/game/Round.cpp) (`Round::maze`, `startRound`, `roundMaze`, `pullRoundLever`), [`src/game/Minimap.cpp`](../../src/game/Minimap.cpp) (`buildMinimapVertices`), testy w [`tests/InteractionTests.cpp`](../../tests/InteractionTests.cpp). Dokumenty modułów: [`../modules/game/interactables.md`](../modules/game/interactables.md), [`../modules/game/gameplay.md`](../modules/game/gameplay.md), [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md).

## 1. Kontekst

Dźwignia zabiera ścianę z labiryntu. Kto czyta ściany z siatki `Maze` (`hasWall`), musi to zobaczyć: odkrywanie komórek ([`minimap-discovered-corridors.md`](minimap-discovered-corridors.md): linia wzroku biegnie wzdłuż korytarzy, a otwarta ściana otwiera nowy) i minimapa. Z drugiej strony `MazeWorld` jest budowany raz na labirynt, jego pola (ściany, macierze, pudełka, wyjście, kryształy) są policzone z ziarna, a restart rundy ma przywrócić wszystkie ściany bez budowania świata od nowa.

## 2. Decyzja

`Round` ma pole `std::optional<Maze> maze`: kopię labiryntu świata, zrobioną w `startRound` i pozbawioną ściany każdej pociągniętej dźwigni (`Maze::removeWall`, która aktualizuje obie komórki po obu stronach ściany). Odkrywanie i minimapa czytają ściany funkcją `roundMaze(world, round)`. `MazeWorld::maze` nigdy się nie zmienia. To wybór wykonawczy.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Kopia w rundzie (wybrana)** | świat jest stały podczas rundy, a stan rundy leży w `Round`, jak stan kryształów. Nowa runda to nowa kopia, więc restart przywraca ściany sam. Dwie funkcje: `pullRoundLever` usuwa, `roundMaze` czyta | druga siatka w pamięci (mała: labirynt to tablica komórek). Dwa miejsca, w których ściany są prawdą: wołający musi użyć `roundMaze`, a nie `world.maze` |
| Zmieniać `world.maze` i przy restarcie budować go od nowa | brak drugiej siatki | restart musiałby odtworzyć labirynt z ziarna i przeliczyć wszystko, co z niego wynika, albo trzymać oryginał obok. Świat przestaje być stały, a `const MazeWorld&` w panelach i rendererach byłoby nieprawdą |
| Filtrować ściany predykatem w każdym miejscu, które je czyta | żadnej kopii | każde miejsce (odkrywanie, minimapa, kolejne) musi znać listę otwartych ścian i pamiętać o sprawdzeniu. Zapomnienie jednego daje ścianę, która jest na mapie, a której nie ma w grze |
| Liczyć z `openedWalls` (funkcja w `Interactables.hpp`) | funkcja już była, z testami | zwraca listę ścian, a odkrywanie czyta siatkę. Siatkę trzeba by budować z listy przy każdym wywołaniu. `openedWalls` nie jest już drogą, którą idzie gra |

## 4. Uzasadnienie i skutki

**Dlaczego `optional`.** `Maze` nie ma konstruktora domyślnego, a `Round` ma być zwykłą strukturą, którą da się utworzyć pustą (`Round{}`). `std::optional` jest pudełkiem, które może nie zawierać wartości: runda niewystartowana nie ma kopii, a `roundMaze` wraca wtedy do `world.maze`. Test `a new round has no lever pulled, every wall standing and no note open` sprawdza oba przypadki: runda niewystartowana daje adres `world.maze`, wystartowana adres inny.

**Co dostaję.**

- Restart: `startRound` robi nową kopię z `world.maze`, więc po restarcie każda ściana wraca, bez kodu "cofania" (test `the view passes an opened wall, and a restart brings every wall back`).
- Odkrywanie widzi otwarcie od następnego kroku (`updateRound` woła `discoverAround` z `roundMaze(world, round)`), a minimapa gubi ścianę i pokazuje korytarz za nią (`buildMinimapVertices` czyta `roundMaze`).
- `pullRoundLever` usuwa ścianę z kopii dla obu komórek, więc `hasWall` zgadza się z obu stron.

**Co tracę.**

- Trzeba pamiętać, że prawdą o ścianach w trakcie rundy jest `roundMaze`, a nie `world.maze`. Plan w panelu Maze rysuje ściany świata, z otwartymi przyciemnionymi (`openedWallFlags`), a minimapa tylko ściany rundy. Obie rzeczy są zamierzone, ale łatwo je pomylić.
- `NightMazeApp::drawMinimap` wciąż podaje `m_mazeWorld.maze` do `minimapProjection` i `minimapMetresPerPixel`. Liczy się tam tylko rozmiar siatki, który się nie zmienia, więc jest to nieszkodliwe, ale nie jest to "minimapa czyta wyłącznie labirynt rundy": wierzchołki idą z `roundMaze`, rozmiar ze świata.
- `openedWalls` i `pullLever` z M8 (podstawy bez okna) zostały w kodzie i w testach, ale gra idzie drogą `pullRoundLever` i `MazeWorld::leverWalls`.

## 5. Kiedy wrócić do tej decyzji

- Gdy świat zacznie się zmieniać w trakcie rundy z innych powodów niż ściany (na przykład ruchome przeszkody): wtedy warto rozważyć jeden wspólny stan zmian rundy.
- Gdy kopia okaże się kosztowna. Nie była mierzona, a labirynt jest tablicą komórek.
- Gdy kolejna część gry zacznie czytać ściany bezpośrednio z `world.maze`: to znak, że `roundMaze` trzeba wymusić (na przykład ukryć `world.maze`).
