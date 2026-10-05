# Skala wysokości przebudowuje teren na procesorze, zamiast mnożyć wysokość w shaderze

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Terrain.hpp`](../../src/game/Terrain.hpp) (`TerrainSettings::heightScale`, `rebuild`, `MAX_HEIGHT_SCALE`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`rebuildTerrain`, `uploadGround`, obsługa flagi w `onRender`), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) (`placeOnTerrain`), [`src/game/Round.cpp`](../../src/game/Round.cpp) (`restCrystalsOnGround`), [`src/debug/panels/TerrainPanel.cpp`](../../src/debug/panels/TerrainPanel.cpp), [`tests/TerrainTests.cpp`](../../tests/TerrainTests.cpp) (przypadki `the height scale multiplies every height, and 0 gives a flat world` i `placeOnTerrain can be called again, and a height scale of 0 brings the flat world back`). Dokument modułu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcje 5.2, 5.10 i 5.13.

## 1. Kontekst

PRD podaje dla tematu 13 pokaz "skala wysokości": suwak, który na żywo rozciąga teren w pionie. Skala jest we wzorze zwykłym mnożnikiem, więc narzuca się rozwiązanie z jednym uniformem: shader wierzchołków mnoży `y` przez liczbę i suwak nic nie kosztuje.

W tej grze teren nie jest jednak tylko obrazem. Z jego wysokości korzystają rzeczy liczone na procesorze: podstawy ścian, słupków i bramy, ich pudełka kolizji, miejsca kryształów, strefa wyjścia, stopy gracza i korzenie trawy.

## 2. Decyzja

Zmiana skali **buduje teren od nowa na procesorze**. Panel ustawia flagę `rebuild`, a gra na początku następnej klatki woła `rebuildTerrain`: nowe wysokości siatki, ściany i brama opuszczone na nowy grunt, nowe pudełka kolizji, kryształy i gracz przestawieni, nowa siatka trójkątów i nowe punkty trawy wysłane na kartę. Runda trwa dalej.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Przebudowa na procesorze, na żądanie z panelu (wybrana)** | jedna prawda: to, co narysowane, i to, po czym chodzi gracz, pochodzi z tych samych liczb. Normalne są liczone dla nowego kształtu. Żadnej zmiany w shaderach: teren używa programów ścian | każda zmiana suwaka to nowe wysokości, siatka, styczne, trawa i nowe bufory. Dla labiryntu startowego 9409 wierzchołków na klatkę przeciągania |
| Uniform w shaderze wierzchołków, mnożący `y` terenu | suwak za darmo, bez wysyłania danych | ściany, kryształy, kolizje, gracz i trawa zostałyby na starej wysokości: ściany wisiałyby nad ziemią albo w niej tonęły. Normalne przestałyby pasować do kształtu. Teren potrzebowałby własnego programu albo dodatkowego uniformu w trzech wspólnych |
| Uniform w shaderze i ta sama skala powtórzona w kodzie procesora | obraz terenu za darmo, reszta poprawna | dwie kopie tego samego wzoru, w GLSL i w C++, które muszą się zgadzać co do milimetra. Ściany i trawę i tak trzeba przeliczyć i wysłać, więc zysk dotyczy tylko jednego bufora |
| Przebudowa po puszczeniu suwaka, nie w trakcie | jedna przebudowa na zmianę | pokaz traci płynność: teren skacze dopiero po puszczeniu myszy |

## 4. Uzasadnienie i skutki

**Dlaczego procesor.** Najcenniejszą własnością terenu w tej grze jest to, że `heightAt` zwraca dokładnie wysokość narysowanego trójkąta. Uniform w shaderze rozdzieliłby te dwie rzeczy: kształt na ekranie zależałby od liczby, której kod gry nie widzi. Test porównujący `heightAt` z siatką straciłby sens.

**Dlaczego przez flagę.** Panel jest rysowany w środku klatki, która właśnie korzysta ze świata. Podmiana terenu w tym miejscu zmieniłaby dane pod ręką kodu rysującego. Flaga odkłada pracę na początek następnej klatki, tak jak `MazeSettings::regenerate` dla nowego labiryntu.

**Co musi zrobić wywołujący.** `placeOnTerrain` poprawia `MazeWorld`. Kopie wysokości mają jeszcze kryształy rundy, lista przeszkód i gracz: `rebuildTerrain` poprawia wszystkie trzy, a idącego gracza stawia na nowym gruncie w obu pozycjach (bieżącej i poprzedniej), żeby klatka nie była narysowana z punktu pośredniego.

**Co przez to tracę.**

- Koszt przeciągania suwaka. Nie został zmierzony. Dla największego labiryntu z panelu (40 x 40) to 47089 wierzchołków na przebudowę.
- `rebuildTerrain` jako całość nie ma testu: jest w `NightMazeApp`, która wymaga okna. Testy pokrywają jej części.
- Zmiana skali w tej samej klatce co nowy labirynt buduje teren dwa razy. Komentarz w `onRender` to odnotowuje.

## 5. Kiedy wrócić do tej decyzji

- Gdy przebudowa zacznie szarpać przy przeciąganiu suwaka w dużym labiryncie: pierwszym krokiem jest przebudowa najwyżej raz na kilka klatek, a nie uniform.
- Gdy skala wysokości miałaby się zmieniać w rozgrywce w każdej klatce (animowany teren): wtedy liczenie na karcie staje się konieczne, razem z przeniesieniem tam wszystkiego, co na terenie stoi.
- Gdy teren zostanie przeniesiony do shadera teselacji albo przesunięcia wierzchołków z tekstury wysokości.
