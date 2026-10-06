# Ściany, słupki i brama opuszczone do najniższego narożnika siatki pod obrysem

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Terrain.cpp`](../../src/game/Terrain.cpp) (`lowestHeightUnder`), [`src/game/MazeWorld.hpp`](../../src/game/MazeWorld.hpp) (`FOOTPRINT_MARGIN`), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) (`lowestGroundUnder`, `lowerToGround`, `placeOnTerrain`), [`src/game/MazeLayout.cpp`](../../src/game/MazeLayout.cpp) (`colliderBoxes`), [`tests/TerrainTests.cpp`](../../tests/TerrainTests.cpp) (przypadki `lowestHeightUnder is never above the surface over its rectangle` i `on uneven ground the walls, pillars and the gate are sunk until no gap shows`). Dokument modułu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcje 2.10 i 5.10.

## 1. Kontekst

Ściana jest modelem o prostej podstawie, długim na 2 m. Teren pod nią nie jest płaski ([`gentle-terrain-under-maze.md`](gentle-terrain-under-maze.md)). Sztywny prostopadłościan na nierównym gruncie można postawić tylko na jednej wysokości, więc gdzieś pod nim teren jest niżej albo wyżej niż podstawa. Szpara pod ścianą jest błędem widocznym od razu: prześwituje przez nią światło i sąsiedni korytarz. Ściana zagłębiona w ziemi wygląda normalnie.

Ograniczenia: model ściany ma zostać jednym modelem dla wszystkich segmentów (jedna siatka, wiele macierzy), a pudełko kolizji ma zostać pudełkiem wyrównanym do osi, razem z modelem.

## 2. Decyzja

Każda ściana, każdy słupek i brama stoją na wysokości **najniższego punktu siatki terenu** spośród wszystkich kwadratów siatki, których dotyka ich obrys widziany z góry, powiększony o `FOOTPRINT_MARGIN = 0,05 m` z każdej strony. Zmienia się tylko `y`: model i pudełko kolizji przesuwają się w dół razem, w poziomie nic się nie rusza.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Najniższy narożnik siatki pod obrysem (wybrana)** | szpary nie ma z dowodu, a nie z pomiaru: płaski trójkąt nigdy nie jest niżej niż jego najniższy narożnik. Od kilku do kilkudziesięciu odczytów na obiekt. Jeden model ściany, jedno pudełko, żadnej zmiany w shaderach ani w kolizjach poziomych | ściana jest z jednej strony zakopana (w labiryncie startowym najwyżej o 0,23 m), więc wystaje z ziemi mniej niż 3 m. Dolny pas tekstury znika pod ziemią |
| Wysokość terenu w środku ściany | jeden odczyt, ściana "średnio" na dobrej wysokości | z jednego końca szpara, z drugiego zakopanie. Na każdej ścianie, która stoi na zboczu |
| Najwyższy punkt pod obrysem | ściana w całości nad ziemią | szpara pod całą resztą podstawy: najgorszy wariant |
| Najniższa wartość z gęstego próbkowania `heightAt` pod obrysem | nie zależy od tego, jak zbudowana jest siatka | więcej odczytów i nadal bez gwarancji: minimum powierzchni z trójkątów leży w wierzchołku, a próbka może go ominąć |
| Wyrównanie terenu pod ścianami (spłaszczenie mapy wzdłuż krawędzi komórek) | ściana stoi na płaskim, nic nie jest zakopane | teren przestaje być mapą wysokości z pliku: zależy od labiryntu, więc każdy nowy labirynt zmienia kształt gruntu, a wzdłuż ścian powstają widoczne półki |
| Podstawa ściany dopasowana do terenu (dolne wierzchołki przesuwane w shaderze albo osobna siatka na segment) | brak szpar i brak zakopania | koniec z jednym modelem dla wszystkich ścian, a pudełko AABB i tak nie umie być skośne |

## 4. Uzasadnienie i skutki

**Dlaczego najniższy narożnik wystarcza.** Powierzchnia terenu to płaskie trójkąty. Wysokość w dowolnym punkcie trójkąta jest średnią ważoną jego trzech narożników, więc nie może być mniejsza od najmniejszego z nich. Jeśli podstawa stoi na najniższym narożniku wszystkich kwadratów pod obrysem, to pod żadnym jej punktem teren nie jest niżej. Test sprawdza to drugą drogą: próbkuje powierzchnię pod każdym pudełkiem w siatce 9 x 9 punktów przy największej skali wysokości.

**Dlaczego obrys z zapasem.** Modele są przy ziemi szersze niż pudełka kolizji: stopa słupka ma 0,4 m, pudełko 0,3 m. Bez zapasu 5 cm szpara mogłaby się pojawić pod wystającą częścią modelu.

**Dlaczego pudełko idzie razem z modelem.** Macierze modelu i pudełka kolizji są liczone z tych samych, już opuszczonych pozycji (`colliderBoxes(world.walls, world.pillars)`), więc linie pudełek rysowane po zaznaczeniu pola w zakładce Diagnostics / Collision and picking nadal pokrywają modele.

**Co przez to tracę.**

- Widoczna wysokość ściany zależy od miejsca: od 3 m do około 2,77 m w labiryncie startowym przy skali 1, a mniej przy większej skali.
- Wysokość podstawy ściany i wysokość stóp gracza obok niej to dwie różne liczby. Stąd granica `MAX_HEIGHT_SCALE` ([`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcja 2.11).
- Reguła zależy od budowy terenu: jest prawdziwa tylko dla powierzchni z płaskich trójkątów rozpiętych na punktach siatki.

## 5. Kiedy wrócić do tej decyzji

- Gdy teren pod labiryntem stanie się na tyle stromy, że zakopanie będzie widoczne jako ściany różnej wysokości.
- Gdy powierzchnia terenu przestanie być płaskimi trójkątami na siatce (teselacja, przesunięcie wierzchołków w shaderze): dowód przestaje działać.
- Gdy ściany dostaną elementy, które muszą być widoczne przy ziemi (drzwi, otwory, napisy).
- Gdy w M7 cienie pokażą, że ściany stojące na różnych wysokościach wyglądają źle w świetle księżyca.
