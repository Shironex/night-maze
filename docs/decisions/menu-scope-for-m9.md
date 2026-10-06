# Zakres menu w M9: cztery ekrany, angielskie teksty, trzy poziomy trudności

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **kod jest** (dopisek z M9, części 3, na końcu tej notatki): wszystkie cztery ekrany istnieją, a poziomy trudności mają liczby, które są **propozycją autora kodu do dopracowania przez właściciela**. Poniższe sekcje 1 do 5 opisują stan z części 2. Decyzje właściciela projektu są w sekcji 2; kontekst, tabela i skutki to moja analiza. Uzupełnia [`menu-in-rmlui.md`](menu-in-rmlui.md), która rozstrzygała bibliotekę, ale nie zakres.
Kod: [`assets/ui/`](../../assets/ui/) (cztery dokumenty i arkusz stylów; w części 2 trzy), [`src/game/GameState.hpp`](../../src/game/GameState.hpp) (`GameMode`, `NewGame`), [`src/game/Difficulty.hpp`](../../src/game/Difficulty.hpp) (tabela poziomów, od części 3), [`src/game/Settings.hpp`](../../src/game/Settings.hpp) (ustawienia, od części 3), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`startNewGame`). Dokumenty: [`../modules/game/game-states.md`](../modules/game/game-states.md), [`../modules/ui/README.md`](../modules/ui/README.md).

## 1. Kontekst

Decyzja o RmlUi mówiła, że menu będzie, ale nie ile ekranów, w jakim języku i co z poziomami trudności. Właściciel ocenił po grze, że runda jest za łatwa i za krótka. Labirynt 10 na 10 z 13 kryształami i baterią na 180 sekund świecenia ([`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md), [`../modules/game/gameplay.md`](../modules/game/gameplay.md)) kończy się szybko, a przeciwnika, który dodałby presji, w grze nie ma ([`enemy-after-m5.md`](enemy-after-m5.md)).

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. W M9 menu ma **cztery ekrany**: menu główne, pauzę, koniec rundy i ustawienia.
2. **Teksty menu są po angielsku.** (Dokumentacja projektu zostaje po polsku.)
3. Gra dostaje **trzy poziomy trudności**. Zmieniają rozmiar labiryntu i liczbę kryształów. Powód: właściciel uważa, że runda jest za łatwa i za krótka.
4. **Liczby poziomów nie są ustalone.** Zostaną dobrane przy pracy nad balansem.

Reszta tej notatki to analiza.

## 3. Rozważane możliwości

To jest analiza. Pierwszy wiersz każdej grupy jest decyzją właściciela.

| Pytanie | Wybrane | Inna możliwość | Uwaga |
|---|---|---|---|
| Ekrany | menu główne, pauza, koniec rundy, ustawienia | bez ustawień | ustawienia dają miejsce na wybór poziomu i dalsze opcje; zakres ustawień (co w nich jest) nie jest ustalony |
| Język tekstów | angielski, jeden | polski albo przełącznik | jeden język oszczędza tabelę tekstów; wcześniejszy wybór małych własnych pomocników zamiast biblioteki lokalizacji pasuje do tego samego kierunku |
| Poziomy trudności | trzy, zmieniają rozmiar labiryntu i liczbę kryształów | zmiana baterii albo przeciwnik | przeciwnika nie ma w kodzie; bateria jest trzecią liczbą, o którą kod już pyta (komentarz `Difficulty` w `GameState.hpp` wymienia rozmiar, kryształy i baterię), ale decyzja właściciela mówi o dwóch pierwszych |

## 4. Uzasadnienie i skutki

**Co jest w kodzie dziś** (sprawdzone w plikach):

- Trzy dokumenty RML w `assets/ui/` (`main_menu.rml`, `pause.rml`, `round_end.rml`) z angielskimi tekstami i jeden arkusz `menu.rcss`. **Ekranu ustawień nie ma.**
- Typ `game::Difficulty` ma wartości `Easy`, `Normal`, `Hard`, a `game::NewGame` niesie wybrany poziom i ziarno. `NightMazeApp::startNewGame` **nie czyta** poziomu: komentarz w kodzie mówi, że liczby nie są jeszcze wybrane, więc każdy poziom gra tak samo. Przycisku wyboru poziomu nie ma.
- Przyciski i ich zdarzenia są testowane bez okna, a poziom trudności nie ma jeszcze żadnych liczb do testowania.

**Co z decyzji wynika** (analiza):

- Rozmiar labiryntu (`MazeSettings::width`, `height`) i liczba kryształów (liczona z rozmiaru, jeden na osiem komórek, najwyżej 16 ze względu na bufor świateł) są powiązane: większy labirynt daje więcej kryształów do granicy 16. Przy projektowaniu trzech poziomów warto to mieć na uwadze, bo górna granica świateł zatrzymuje liczbę kryształów, a nie rozmiar. To obserwacja z kodu ([`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md)), nie propozycja liczb.
- Dłuższa runda zmienia bilans baterii (180 s pełnej baterii, zebrany kryształ oddaje 25 procent): bez ruszania baterii większy labirynt jest trudniejszy w sposób, którego nikt jeszcze nie sprawdził w grze. Nie ruszam tej liczby: decyzja właściciela o niej nie mówi.
- Nowa gra buduje labirynt od nowa (`regenerateMaze`), więc zmiana rozmiaru przy `Play` jest możliwa bez nowej infrastruktury.

**Czego ta notatka nie przesądza.** Liczb poziomów, zawartości ekranu ustawień, wyglądu ekranów ani tego, czy poziom jest zapamiętywany między uruchomieniami (zapisu ustawień w grze nie ma).

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie ekran ustawień: zapisać, co w nim jest.
- Gdy zostaną dobrane liczby poziomów: zapisać je tutaj albo w nowej notatce razem z tym, jak je sprawdzono w grze.
- Gdy po balansie runda nadal będzie za krótka: wrócić do pytania o przeciwnika ([`enemy-after-m5.md`](enemy-after-m5.md)).

## 6. Dopisek z 2026-10-06 (M9, część 3): zbudowane, liczby do dopracowania

**Co jest w kodzie** (sprawdzone w plikach, nie powtórzone na ekranie przy pisaniu): cztery ekrany istnieją, w tym ekran ustawień (czułość myszy, pole widzenia, pełny ekran, rozmiar okna), a ustawienia leżą w pliku `night-maze-settings.txt` ([`../modules/game/settings.md`](../modules/game/settings.md)). Poziomy trudności mają liczby w jednej tabeli ([`../modules/game/difficulty.md`](../modules/game/difficulty.md)): `Easy` 10 na 10 z 13 kryształami, `Normal` 16 na 16 z 26 i `Hard` 22 na 22 z 40. Opis ekranów: [`../modules/ui/menu-screens.md`](../modules/ui/menu-screens.md).

**Co jest decyzją właściciela, a co propozycją.** Decyzje właściciela z sekcji 2 zostały zbudowane tak, jak je zapisano: cztery ekrany, angielskie teksty, trzy poziomy zmieniające rozmiar labiryntu i liczbę kryształów. **Liczby poziomów są propozycją autora kodu, którą właściciel dopracuje po zagraniu** (punkt 4 decyzji pozostaje otwarty). Propozycja idzie dalej niż decyzja: zmienia też baterię (180, 150 i 120 sekund) i próg bramy (70, 70 i 80 procent), a `Normal` i `Hard` mają mniej kryształów, niż dałaby reguła "jeden na osiem komórek" (26 i 40 zamiast 32 i 61). Pomiary autora dla trzech poziomów (ziarna od 1 do 50: droga do wyjścia, najkrótsza runda, texel cienia, czasy klatek) są w [`../modules/game/difficulty.md`](../modules/game/difficulty.md) jako pomiary autora z gałęzi menu przed scaleniem. Limit 16 kryształów, o którym mówi sekcja 4, nie obowiązuje: [`nearest-crystals-carry-the-lights.md`](nearest-crystals-carry-the-lights.md).

**Relacja właściciela (2026-10-06).** Właściciel zagrał na `Hard` w buildzie Debug i zgłosił, że menu, okno debug i gra działają, własnymi słowami: "it was great". To jest **relacja właściciela, a nie zamknięta lista kontrolna i nie zatwierdzenie liczb poziomów**: lista ręczna w [`../guides/build-windows.md`](../guides/build-windows.md), sekcja 28.2, pozostaje otwarta, a na macOS kod nie był budowany.

**Co jest nadal otwarte.** Liczby poziomów (po rozegraniu rund na wszystkich trzech); to, czy poziom ma zmieniać baterię i próg bramy; wygląd ekranów w porównaniu z makietą (makiety nie ma w repozytorium); czy rozmycie tła ma być zrobione własnym przebiegiem gry (RmlUi go nie umie); pełna bramka na scalonym drzewie okna debug i ekranów menu, która nie została uruchomiona. (Dopisek z 2026-10-06, po M9, części 4: pełna bramka została uruchomiona, `make check` przeszedł na `0f8d3b9`, 564 przypadki testowe i 220119 asercji, zgłoszone przez bramkę; ten punkt jest zamknięty.)
