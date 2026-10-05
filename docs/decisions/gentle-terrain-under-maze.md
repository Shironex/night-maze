# Łagodny teren pod labiryntem zamiast płaskich płytek

Data: 2026-10-05. Stan: obowiązuje. Decyzja właściciela projektu.
Kod: [`src/game/Terrain.hpp`](../../src/game/Terrain.hpp) (`MAZE_RELIEF`, `HILL_RELIEF`, `TERRAIN_MARGIN_CELLS`, `terrainRelief`), [`src/game/Terrain.cpp`](../../src/game/Terrain.cpp) (konstruktor `Terrain`), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) (`placeOnTerrain`), [`src/game/Player.cpp`](../../src/game/Player.cpp) (`update`), [`tests/TerrainTests.cpp`](../../tests/TerrainTests.cpp) (przypadek `the heightmap of the game loads and gives gentle ground inside the default maze`). Dokument modułu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcje 2.3, 2.4, 2.10 i 2.11.

## 1. Kontekst

Temat 13 wykładu to "Implementacja podłoża". PRD zapisuje go jako "teren z heightmapy pod labiryntem, wysokość gracza z terenu", z pokazem "skala wysokości, wireframe". Do pierwszej części M6 podłoga gry była zbiorem płaskich płytek 2 x 2 m na `y = 0`, a cały kod (ściany, słupki, brama, kryształy, strefa wyjścia, gracz, kolizje) zakładał tę wysokość.

Teren i labirynt ciągną w przeciwne strony. Teren jest ciekawy wtedy, gdy ma wyraźne wzniesienia. Labirynt to proste ściany o stałej wysokości na regularnej siatce, a jego kolizje to pudełka wyrównane do osi: na stromym zboczu ściana albo wisi, albo jest zakopana po dach, a gracz i pudełko ściany rozjeżdżają się w pionie.

Trzeba było zdecydować, gdzie w grze jest teren i jak bardzo jest nierówny.

## 2. Decyzja

Teren leży **pod labiryntem i wokół niego**, jako jedna siatka z mapy wysokości, która zastępuje płytki. Pod labiryntem amplituda jest mała (`MAZE_RELIEF = 0,6 m` między czernią a bielą mapy, w labiryncie startowym około 0,38 m między najniższym a najwyższym miejscem), ściany są zagłębione tak, żeby nie było szpar, wysokość gracza idzie za terenem, a poza labiryntem ta sama mapa rośnie we wzgórza (`HILL_RELIEF = 4,5 m`). Przejście między jednym a drugim robi krzywa smoothstep na marginesie 14 m.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Łagodny teren pod labiryntem, wzgórza dookoła (wybrana)** | temat 13 jest spełniony dosłownie: gracz chodzi po terenie z mapy wysokości. Labirynt pozostaje czytelny i grywalny. Wzgórza dają tło nad ścianami i pokazują, że to prawdziwy teren. Suwak skali ma co pokazywać, a zero daje dawny płaski świat | każda rzecz w grze, która miała `y = 0`, musiała dostać wysokość. Ściany są z jednej strony zakopane. Potrzebna granica skali, żeby kolizje działały |
| Płaska podłoga w labiryncie, teren tylko dookoła | zero zmian w labiryncie, kolizjach i graczu | "wysokość gracza z terenu" byłaby nieprawdą: gracz nigdy nie stanąłby na terenie, chyba że w trybie noclip. Temat 13 byłby dekoracją |
| Wyraźne wzgórza także w labiryncie | efektowny teren, mocny pokaz | ściany o stałej wysokości na zboczu wymagają dopasowania kształtu do terenu albo schodkowania. Pudełka AABB przestają pasować do gracza w pionie. Latarka i kryształy znikają za garbami. To inna gra |
| Osobna scena z terenem, przełączana w panelu | teren dowolnie stromy, bez wpływu na labirynt | dwa światy do utrzymania, a gra nie zyskuje nic: teren byłby pokazem obok gry, a nie jej częścią |

## 4. Uzasadnienie i skutki

**Dlaczego pod labiryntem.** PRD mówi wprost "pod labiryntem" i "wysokość gracza z terenu". Wariant z płaską podłogą tego nie spełnia, a obrona polega na pokazaniu działającego mechanizmu w grze, nie obok niej.

**Dlaczego łagodny.** Amplituda 0,6 m to tyle, żeby nierówność było widać i czuć przy chodzeniu, a ściany nadal wyglądały jak ściany. Test trzyma różnicę w labiryncie startowym między 0,3 m a 0,5 m. Jedna mapa i relief zależny od odległości od labiryntu dają oba rodzaje terenu bez drugiego obrazu.

**Co dzięki temu dostaję.**

- Jeden mechanizm (`heightAt`) stawia gracza, kryształy, wyjście i trawę.
- Suwak `Height scale` od 0 do 2,5: przy zerze świat jest płaski jak z płytkami, co jest też najprostszym sprawdzeniem, że nic się nie zepsuło.
- Jedno wywołanie rysujące zamiast jednego na komórkę.

**Co przez to tracę.**

- Ściany, słupki i brama są zagłębione z jednej strony ([`walls-sunk-to-lowest-corner.md`](walls-sunk-to-lowest-corner.md)).
- Skala wysokości ma twardą granicę: `MAZE_RELIEF * MAX_HEIGHT_SCALE = 1,5 m` musi być mniejsze od wzrostu gracza (1,8 m), inaczej pudełko gracza mogłoby minąć pudełko ściany w pionie.
- Gracz nie ma grawitacji ani kolizji z ziemią: wysokość jest odczytywana. Wystarcza to tylko dlatego, że teren jest łagodny.
- Płytki, ich model, skrypt i tekstury zostały usunięte ([`floor-tiles-retired.md`](floor-tiles-retired.md)).

## 5. Kiedy wrócić do tej decyzji

- Gdy gra dostanie skok, spadanie albo schody: wtedy odczyt wysokości przestaje wystarczać i potrzebna jest prawdziwa kolizja z terenem.
- Gdy labirynt ma mieć piętra albo rampy: łagodna nierówność pod płaskim planem już tego nie opisze.
- Gdy w M7 dojdą cienie: cień ściany na nierównym gruncie pokaże zagłębienie wyraźniej niż dziś i może się okazać, że amplitudę trzeba zmniejszyć.
- Gdy wzgórza mają być dostępne dla gracza bez trybu noclip: prędkość pozioma niezależna od nachylenia przestanie być wiarygodna.
