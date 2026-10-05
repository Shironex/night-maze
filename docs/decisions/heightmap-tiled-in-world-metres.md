# Mapa wysokości powtarzana co 48 m w metrach świata, a nie rozciągana na teren

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Terrain.hpp`](../../src/game/Terrain.hpp) (`HEIGHTMAP_SPAN`, komentarz `Heightmap::sample`), [`src/game/Terrain.cpp`](../../src/game/Terrain.cpp) (`fraction`, `Heightmap::sample`, konstruktor `Terrain`), [`tools/blender/make_heightmap.py`](../../tools/blender/make_heightmap.py) (`value_noise`), [`tests/TerrainTests.cpp`](../../tests/TerrainTests.cpp) (przypadek `Heightmap::sample blends the four values around a place and repeats`). Dokument modułu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md), sekcje 2.5 i 5.14.

## 1. Kontekst

Teren czyta wysokości z jednego obrazu 256 x 256. Rozmiar terenu nie jest stały: zależy od labiryntu, a suwaki panelu Maze pozwalają ustawić od 2 do 40 komórek na bok. Ziemia ma wtedy od 32 m do 108 m boku. Trzeba było zdecydować, jak punkt świata `(x, z)` zamienia się na miejsce w obrazie.

## 2. Decyzja

Współrzędna obrazu to pozycja w metrach podzielona przez stałą: `u = x / 48`, `v = z / 48` (`HEIGHTMAP_SPAN`). Obraz powtarza się jak kafelkowana tekstura: jedno powtórzenie pokrywa zawsze kwadrat 48 x 48 m, niezależnie od rozmiaru labiryntu. Początek obrazu leży w początku układu świata, czyli w północno-zachodnim rogu labiryntu.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Kafelkowanie w metrach świata (wybrana)** | pagórki mają tę samą wielkość w każdym labiryncie. Piksel mapy ma zawsze 0,19 m, więc gładkość terenu nie zależy od rozmiaru. Ta sama komórka labiryntu ma tę samą wysokość bez względu na to, jak duży jest labirynt | obraz musi się powtarzać bez szwu. W dużym labiryncie widać powtórzenie co 48 m, jeśli ktoś go szuka z góry |
| Rozciągnięcie obrazu na cały teren | każdy obraz się nadaje, także niekafelkowany. Nie ma powtórzeń | wielkość i stromość wzgórz zależą od rozmiaru labiryntu: przy 40 x 40 pagórki są ponad dwa razy szersze i łagodniejsze niż przy 10 x 10, przy 2 x 2 ciaśniejsze i bardziej strome. Suwak rozmiaru zmieniałby charakter terenu |
| Szum liczony w kodzie C++ zamiast obrazu | teren bez powtórzeń i bez pliku | to już nie jest mapa wysokości z pliku, o którą chodzi w temacie 13. Kształtu nie da się obejrzeć ani wymienić bez kompilacji |
| Osobny obraz na każdy rozmiar labiryntu | pełna kontrola nad każdym terenem | rozmiarów jest 39 na 39. Niewykonalne |

## 4. Uzasadnienie i skutki

**Dlaczego 48 m.** To labirynt startowy (20 m) z marginesem z obu stron (2 x 14 m). Przy ustawieniach startowych teren widzi dokładnie jedno powtórzenie obrazu, więc scena, którą gra zaczyna i którą pokazuję na obronie, nie ma żadnego powtórzenia.

**Jak spełniony jest warunek powtarzania.** Skrypt buduje obraz z szumu wartości na siatce, w której węzeł za ostatnim jest znów pierwszym, więc lewa krawędź przechodzi w prawą, a górna w dolną z konstrukcji. Po stronie gry `sample` bierze część ułamkową współrzędnych (także dla ujemnych) i miesza ostatnią wartość wiersza z pierwszą.

**Co dzięki temu dostaję.** Stałe `MAZE_RELIEF` i `HILL_RELIEF` znaczą to samo w każdym labiryncie, bo nachylenia się nie zmieniają. Test "różnica w labiryncie startowym od 0,3 do 0,5 m" opisuje teren, który gracz zobaczy w podobnej postaci przy innych rozmiarach.

**Co przez to tracę.**

- Mapy nie da się podmienić dowolnym obrazem: niekafelkowany da uskok co 48 m.
- W dużym labiryncie pod labiryntem wypada cały zakres mapy, od czerni do bieli, a nie tylko jej łagodny fragment. Dlatego rachunek granicy `MAX_HEIGHT_SCALE` używa pełnych 0,6 m reliefu, a nie 0,38 m z labiryntu startowego.
- W labiryncie większym niż 10 x 10 teren się powtarza. Relief zależny od odległości od labiryntu maskuje to na wzgórzach (te same kształty mają tam inną wysokość), ale pod labiryntem powtórzenie jest dokładne.

## 5. Kiedy wrócić do tej decyzji

- Gdy powtórzenie zacznie być widać w grze, na przykład przy widoku z góry albo z mapą w HUD.
- Gdy teren ma być projektowany ręcznie pod konkretny poziom, a nie generowany: wtedy obraz na cały teren jest naturalniejszy.
- Gdy rozmiar labiryntu przestanie być zmienny.
