# Kryształy: liczba rośnie z labiryntem, brama otwiera się przy około 70 procentach

Data: 2026-10-05. Stan: obowiązuje. Zastępuje [`dead-end-lights.md`](dead-end-lights.md).
Kod: [`src/game/Crystals.hpp`](../../src/game/Crystals.hpp), [`Crystals.cpp`](../../src/game/Crystals.cpp) (`crystalCountFor`, `placeCrystals`), [`src/game/Round.hpp`](../../src/game/Round.hpp), [`Round.cpp`](../../src/game/Round.cpp) (`requiredCrystalCount`, `crystalLightPositions`), [`src/debug/panels/GameplayPanel.cpp`](../../src/debug/panels/GameplayPanel.cpp) (suwak `Crystals needed`), testy w [`tests/CrystalTests.cpp`](../../tests/CrystalTests.cpp) i [`tests/RoundTests.cpp`](../../tests/RoundTests.cpp). Dokument modułu: [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcje 2 i 5.

## 1. Kontekst

PRD mówi: "po zebraniu N kryształów wyjście się otwiera". Liczby N nie podaje, liczby kryształów w labiryncie też nie. Podaje za to limit świateł: jeden bufor uniformów z "do 16 punktowych", a kryształy są światłami punktowymi.

Trzeba było ustalić dwie liczby: ile kryształów dostaje labirynt i ile z nich otwiera bramę. Ograniczenia:

- rozmiar labiryntu ustawia się w panelu Maze (suwaki od 2 do 40 komórek na bok), a sama klasa `Maze` przyjmuje od 1 do 256, więc liczby muszą mieć sens dla każdego rozmiaru,
- każdy kryształ niesie światło punktowe, a tablica w shaderze ma 16 miejsc (`scene::MAX_POINT_LIGHTS`),
- ten sam labirynt i to samo ziarno mają dawać te same kryształy na obu systemach ([`deterministic-random.md`](deterministic-random.md)).

W M4 miejsce kryształów zajmowały światła w ślepych zaułkach ([`dead-end-lights.md`](dead-end-lights.md)). Ta notatka zastępuje tamtą.

## 2. Decyzja

Labirynt dostaje jeden kryształ na 8 komórek (`CELLS_PER_CRYSTAL`), zaokrąglone do najbliższej liczby całkowitej, nie mniej niż 1 i nie więcej niż 16. Brama otwiera się po zebraniu 70 procent z nich (`requiredFraction`), zaokrąglone w górę, nie mniej niż 1. Ułamek da się zmienić w trakcie rundy suwakiem `Crystals needed` w panelu Gameplay.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Jeden na 8 komórek, od 1 do 16, brama przy 70 procentach (wybrana)** | mały labirynt nie jest zasypany kryształami, duży nie jest pusty. Limit 16 pokrywa się z tablicą świateł, więc każdy kryształ na planszy świeci. Zapas 30 procent: nie trzeba znaleźć ostatniego kryształu w ostatnim zaułku. Dwie liczby, obie do pokazania i do strojenia | od 12 na 12 komórek liczba staje na 16 i kryształy rzedną. Próg w procentach daje dla małych liczb skoki (2 z 2, 10 z 13) |
| Stała liczba kryształów i stałe N | najprostsze, zgodne z literą PRD ("N kryształów") | labirynt 4 na 4 ma 14 wolnych komórek, a 30 na 30 ma ich 898: żadna stała nie pasuje do obu. Liczba większa niż liczba wolnych komórek wymagałaby osobnej reguły |
| Wszystkie kryształy otwierają bramę | jedna liczba mniej, jasna reguła | runda kończy się szukaniem jednego pominiętego kryształu. Po zebraniu wszystkich gasną wszystkie ich światła, więc drogę do wyjścia gracz zawsze pokonywałby w najciemniejszej scenie |
| Kryształ w każdym ślepym zaułku i nigdzie indziej (reguła świateł z M4) | bez losowania, wynika wprost z siatki | liczby nie da się ustawić i zależy od ziarna. Wyjście też jest zaułkiem. Ponad 16 zaułków oznaczałoby kryształy bez światła albo zaułki bez kryształu |
| Więcej niż 16 kryształów, a światło tylko dla 16 najbliższych graczowi | gęstość stała w każdym rozmiarze | wybór świateł w każdej klatce, światła zapalające się i gasnące w polu widzenia. Kod i pułapki dla labiryntów, których na obronie nie pokażę |

## 4. Uzasadnienie i skutki

**Skąd 8 komórek.** Labirynt startowy 10 na 10 dostaje 13 kryształów: `(100 + 4) / 8`. Miał 11 ślepych zaułków poza startem, jeden z nich jest dziś wyjściem, więc kryształy wypełniają zaułki i zostają jeszcze 3 na inne komórki (to wyliczenie, nie wynik przypięty testem). Labirynt wzorcowy 4 na 4 dostaje 2.

**Skąd limit 16.** To nie liczba z gry, tylko z shadera: `MAX_POINT_LIGHTS` w `scene/Light.hpp` i tablica tej samej długości w `common/lighting.glsl`. Skoro kryształów nigdy nie jest więcej niż miejsc w tablicy, `buildLightSet` nigdy żadnego nie pomija. Limit jest osiągany od 124 komórek, czyli od labiryntu 12 na 12.

**Skąd 70 procent, w górę i co najmniej 1.** Zapas ma być wyraźny, ale większość kryształów trzeba znaleźć. Zaokrąglenie w górę sprawia, że "70 procent" nigdy nie znaczy mniej niż 70 procent. Dolna granica 1 pilnuje, żeby brama nigdy nie była otwarta bez żadnego kryształu, także gdy suwak stoi na minimum. Dla 13 kryształów wychodzi 10, dla 16 wychodzi 12, dla 2 wychodzą 2.

**Gdzie stoją kryształy.** Nigdy w komórce startowej i nigdy w komórce wyjścia, najwyżej jeden na komórkę. Najpierw ślepe zaułki w kolejności potasowanej ziarnem, potem pozostałe komórki, też potasowane. To, co stara notatka odrzuciła jako "losowe komórki z ziarna", jest tu zrobione tak, żeby jej zastrzeżenia nie miały zastosowania: kryształy mają własny generator (ziarno labiryntu plus stała) i tasowanie napisane ręcznie na `randomBelow`, więc generator labiryntu nie dostał ani jednego losowania i labirynty wzorcowe się nie zmieniły.

**Co jest w kodzie.**

- `crystalCountFor` liczy na liczbach całkowitych: dodaje połowę dzielnika przed dzieleniem.
- `placeCrystals` daje mniej kryształów, gdy wolnych komórek jest mniej: labirynt z jedną albo dwiema komórkami nie dostaje żadnego.
- `requiredCrystalCount` odejmuje przed zaokrągleniem w górę mały zapas (`ROUNDING_GUARD`), bo `0,7F * 10` w typie `float` wychodzi minimalnie powyżej 7 i dałoby 8.
- Wymagana liczba jest liczona w każdym kroku od nowa, więc suwak działa w trakcie rundy. Brama raz otwarta zostaje otwarta.
- Labirynt bez kryształów zaczyna z otwartą bramą.
- Światło wisi nad każdym niezebranym kryształem. Zebrany traci je od razu.

**Co przez to tracę.**

- Powyżej 12 na 12 kryształów jest wciąż 16: w labiryncie 30 na 30 to jeden na 56 komórek. Duże labirynty są ciemniejsze i dłuższe.
- Liczby kryształów nie ustawia się osobno: wynika z rozmiaru. Stroić można tylko próg.
- Reguły nie oceniłem jeszcze w ręcznej grze. Liczby 8 i 0,7 są punktem wyjścia, nie wynikiem prób.

## 5. Kiedy wrócić do tej decyzji

- Gdy ręczna gra pokaże, że runda na labiryncie startowym jest za krótka albo za długa: najpierw suwak `Crystals needed`, potem `CELLS_PER_CRYSTAL`.
- Gdy na pokazie będą potrzebne labirynty dużo większe niż 12 na 12: wtedy wraca możliwość z wyborem 16 najbliższych świateł.
- Gdy tablica świateł w shaderze zmieni długość: limit kryształów idzie za nią sam, ale liczby w tej notatce trzeba przeliczyć.
- Gdy labirynt przestanie być doskonały (pętle): zaułków będzie mniej i kolejność "najpierw zaułki" straci znaczenie.
