# Płytki podłogi usunięte: podłoże rysuje teren

Data: 2026-10-05. Stan: obowiązuje.
Kod: usunięte pliki `assets/models/floor_tile.obj` i `floor_tile.mtl`, `tools/blender/build_floor_tile.py`, `assets/textures/floor_stone.png` i `floor_stone_normal.png`, pole `MazeWorld::floorMatrices`, pole `MazeRenderer::m_floorTile`, stała `Player::FLOOR_Y`, przypadek testowy `loadObj: floor_tile.obj`. W ich miejsce: [`src/game/TerrainRenderer.cpp`](../../src/game/TerrainRenderer.cpp), [`assets/textures/ground.png`](../../assets/textures/ground.png) i [`ground_normal.png`](../../assets/textures/ground_normal.png), funkcja `drawMesh` w [`src/game/ModelDraw.cpp`](../../src/game/ModelDraw.cpp). Dokument modułu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcje 3 i 5.1.

## 1. Kontekst

Od M2 + M3 podłoga labiryntu była modelem OBJ: kwadrat 2 x 2 m z czterech wierzchołków, rysowany raz na komórkę z własną macierzą modelu. Miała teksturę kamiennych płyt i od M4 mapę normalnych. Była też najprostszym modelem w grze, więc dokumentacja loadera OBJ, indeksów i Blendera używała jej jako przykładu.

W drugiej części M6 doszedł teren z mapy wysokości pod labiryntem ([`gentle-terrain-under-maze.md`](gentle-terrain-under-maze.md)). Trzeba było zdecydować, co z płytkami.

## 2. Decyzja

Płytki zostały **usunięte w całości**: model, materiał, skrypt Blendera, obie tekstury, pola i stałe w kodzie oraz test. Podłoże rysuje jedna siatka terenu z nową teksturą ziemi (`ground.png`) i jej mapą normalnych. Przy skali wysokości 0 teren jest płaski na `y = 0`, więc dawny płaski świat da się nadal zobaczyć, tylko z inną teksturą.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Usunąć płytki, podłoże to teren (wybrana)** | jedno podłoże, jedna prawda o wysokości. Jedno wywołanie rysujące zamiast jednego na komórkę. Żadnego martwego kodu ani assetów | znika najprostszy model, na którym opierały się przykłady w dokumentacji i jeden test loadera. Dokumenty trzeba było przerobić na inny przykład |
| Zostawić płytki i położyć je na terenie (każda na wysokości swojej komórki) | kamienna posadzka zostaje | płaska płytka 2 x 2 m na nierównym gruncie: szpary i uskoki między sąsiednimi płytkami, a teren pod nimi niewidoczny. Temat 13 byłby schowany pod podłogą |
| Zostawić płytki jako przełącznik "stara podłoga" w panelu | porównanie przed i po na żywo | dwie drogi rysowania podłoża do utrzymania, a płytki działałyby tylko przy skali 0. Suwak `Height scale` na zerze pokazuje to samo taniej |
| Zostawić pliki w repozytorium, usunąć tylko użycie | przykłady w dokumentacji nadal mają swój plik | assety, których nic nie wczytuje, i dokumentacja opisująca kod, którego gra nie wykonuje |
| Teksturę płyt (`floor_stone`) położyć na terenie | brak nowej tekstury | regularne płyty na falującym gruncie wyglądają jak rozciągnięta tapeta, a fugi nie trafiają w nic. Ziemia z kamykami i mchem pasuje do nierówności i do trawy |

## 4. Uzasadnienie i skutki

**Dlaczego całkowicie.** Projekt ma zasadę, że dokumentacja opisuje kod, który działa. Pliki zostawione "na przykład" byłyby wyjątkiem, który ktoś na obronie mógłby otworzyć i zapytać, gdzie są używane.

**Co dzięki temu dostaję.**

- W labiryncie startowym ubywa 100 wywołań rysujących (jedna płytka na komórkę), a przybywa jedno.
- `MazeRenderer` rysuje dwa modele zamiast trzech, a `MazeWorld` nie liczy macierzy podłogi.
- Podłoże ma wierzchołek co 0,5 m zamiast co 2 m.

**Co przez to tracę.**

- Najprostszy przykład modelu OBJ. Dokumenty [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md), [`../modules/gfx/indexed-drawing.md`](../modules/gfx/indexed-drawing.md) i [`../guides/blender.md`](../guides/blender.md) używają teraz innych przykładów.
- Test loadera, który sprawdzał normalną w górę, styczną `+X` i nawijanie na płaskim kwadracie. Te same własności sprawdza dziś dla terenu `tests/TerrainTests.cpp` (przypadki `every triangle of the mesh faces up, and its texture is not mirrored` i `the vertices of the mesh carry normals, texture coordinates and tangents`).
- Pomiary i zrzuty ekranu z M2 do M5 robione na płytkach zostają w dokumentach jako historia: nie da się ich już powtórzyć na tym samym obiekcie.

## 5. Kiedy wrócić do tej decyzji

- Gdy gra dostanie wnętrza z prawdziwą posadzką (sala, most, platforma): wtedy płaska płytka jako model znów ma sens, obok terenu, a nie zamiast niego.
- Gdy okaże się, że na obronie potrzebny jest model prostszy niż słupek do pokazania formatu OBJ: wystarczy wtedy plik przykładowy w testach, bez przywracania podłogi do gry.
