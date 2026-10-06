# Menu gry w RmlUi już w M9, Dear ImGui zostaje przy panelach debug

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **kodu nie ma**.
Kod: brak. W `cmake/Dependencies.cmake` nie ma ani RmlUi, ani FreeType, w `src/` nie ma menu, a w `docs/` nie ma dokumentu biblioteki RmlUi (sprawdzone wyszukiwaniem tych nazw w `src/`, `cmake/`, `CMakeLists.txt` i `docs/`). Zbudowane jest tylko to, co dziś stoi w `Dependencies.cmake`: GLFW, GLM, Dear ImGui i pozostałe biblioteki z [`../libraries/`](../libraries/). Dokument modułu: brak. Powiązane: [`small-calls-after-m6.md`](small-calls-after-m6.md) (punkt 2: pauza dojdzie razem z menu), [`enemy-after-m5.md`](enemy-after-m5.md) (ten sam sposób zapisu decyzji bez kodu).

## 1. Kontekst

PRD (sekcja 2, mechaniki) wymienia "Menu start / koniec rundy w ImGui (czas, zebrane kryształy)" z priorytetem COULD, a ostatni kamień milowy, M9 (sekcja 11), ma w zakresie "Szlif, menu, balans, opcjonalny przeciwnik, README, przygotowanie do code review". Plan tematów w [`../syllabus.md`](../syllabus.md) mówi o M9 podobnie: "Szlif, menu, balans, przygotowanie do code review". Menu jest więc zaplanowane, a PRD zakłada, że będzie zrobione w Dear ImGui, tej samej bibliotece co panele debug.

Dziś gra nie ma żadnego menu ani stanów gry. `RoundState` w [`../../src/game/Round.hpp`](../../src/game/Round.hpp) ma tylko `Playing` i `Won`. Esc zwalnia mysz, ale nie zatrzymuje rundy, bo pauza ma przyjść razem z menu ([`small-calls-after-m6.md`](small-calls-after-m6.md), punkt 2).

PRD ma też "zasadę zero tajemnic": do repozytorium nie trafia kod, którego nie potrafię wytłumaczyć, a kod pisany z pomocą AI czytam, przerabiam i opisuję w `docs/`, zanim go zacommituję. Obrona polega na przeglądzie kodu i pytaniach prowadzącego.

Dnia 2026-10-06 powstał artefakt projektowy z dwoma klikalnymi wariantami menu: w Dear ImGui i w RmlUi. To makieta, a nie kod gry: nie leży w repozytorium i nie zmienia niczego w buildzie (informacja ode mnie, nie do sprawdzenia w repozytorium). Wybrałem po tym, jak go obejrzałem.

## 2. Decyzja

Decyzje właściciela projektu, to jest cała ich treść:

1. Menu gry zbuduję w **RmlUi**, już w M9, a nie najpierw w Dear ImGui. Dear ImGui zostaje przy panelach debug.
2. Powody, moimi słowami: wariant menu w RmlUi był płynniejszy i lepiej animowany. Do gry będzie dochodziło dużo więcej i ma się stać znacznie przyjemniejsza w graniu, poza programem kursu. Wiem, że to oznacza więcej do wyjaśnienia i że na obronie prawdopodobnie nie starczy czasu, żeby wyjaśnić wszystko. Wierzę, że prowadzący doceni chęć nauczenia się więcej. Na obronie wyjaśnię podstawowe pojęcia: panele Dear ImGui, ziarno labiryntu, kamerę, shadery i pozostałe rzeczy zbudowane po drodze.
3. Tło menu **nie jest rozstrzygnięte**: żywa scena gry za menu albo wcześniej wyrenderowana pętla wideo. Chcę porównać prawdziwą żywą scenę z prawdziwą nagraną pętlą z gry, zanim wybiorę.

Uwaga o zakresie: treścią decyzji właściciela są trzy punkty wyżej i nic ponad nie. Kontekst, tabela możliwości, skutki dla kodu i warunki powrotu w sekcjach 3 do 5 to moja analiza, nie część decyzji. Oznaczam w nich, co sprawdziłem w repozytorium, a co jest zgłoszone z pamięci albo od właściciela.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Wybór z pierwszego wiersza jest decyzją właściciela, pozostałe wiersze to możliwości, które rozważyłem obok niej.

| Możliwość | Zalety | Wady |
|---|---|---|
| **RmlUi w M9 (wybrane)** | interfejs opisany dokumentami w stylu HTML i CSS, z animacjami i przejściami, więc menu może wyglądać i ruszać się jak część gry, a nie jak okno narzędzia. Wariant z makiety był według mnie płynniejszy. Droga do dalszej rozbudowy gry: ekrany, ustawienia, HUD. Nowa biblioteka do poznania, czyli zgodne z tym, że chcę się więcej nauczyć | nowa zależność i druga biblioteka interfejsu w jednej klatce. Mniej czasu na wyjaśnienie wszystkiego na obronie: zasada zero tajemnic jest tu świadomie poluzowana dla warstwy menu (sekcja 4). Do zbudowania: interfejs renderujący i podłączenie wejścia, stany gry, a przed tym sprawdzenie na macOS z profilem 4.1 Core |
| Menu w Dear ImGui na obronę, RmlUi dopiero po niej | zero nowych zależności przed obroną, biblioteka jest już w buildzie i opisana w [`../libraries/imgui.md`](../libraries/imgui.md). Zgodne z PRD. Najmniej do wyjaśnienia | menu zrobione dwa razy: ekrany, stany i wejście przerabiane po obronie. Ten dokument biblioteki ImGui sam zaznacza, że biblioteka nie jest pomyślana jako interfejs samej gry. Wygląd menu ograniczony tym, co ImGui robi najłatwiej |
| Tylko Dear ImGui | najprostsze, jedna biblioteka interfejsu, najmniej ryzyka na macOS, pełna zgodność z PRD | menu wygląda jak narzędzie debug. Rozbudowa gry w stronę, którą chcę, byłaby przy tej bibliotece uciążliwa |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienie jest w punkcie 2 decyzji i nie dodaję do niego nic od siebie. To właściciel, a nie ta analiza, rozstrzyga, że płynność menu i dalszy rozwój gry ważą więcej niż prostota.

**Skutek, który zapisuję wprost.** Zasada zero tajemnic z PRD mówi, że do repozytorium nie trafia kod, którego nie potrafię wytłumaczyć. Ta decyzja świadomie ją poluzowuje dla warstwy menu: biblioteka RmlUi i kod, który ją podłącza, będą większe, niż zdążę wytłumaczyć na obronie. Zapisuję to tak, jak to widzę: wiem o tym, godzę się na to i na obronie wyjaśnię podstawowe pojęcia (panele Dear ImGui, ziarno labiryntu, kamera, shadery i reszta rzeczy zbudowanych po drodze). Pozostałe części kodu opiszę w dokumentach, jak zawsze.

**Co jest znane dziś i trzeba sprawdzić, gdy zacznie się praca** (analiza, nie decyzja):

- *Nowa zależność.* Pozostałe biblioteki są pobierane przez CMake `FetchContent` i przypięte do tagu (w `Dependencies.cmake`: GLFW 3.4, GLM 1.0.3, Dear ImGui v1.92.9b-docking i dwie następne). RmlUi dojdzie tą samą drogą, z przypiętym tagiem. Ma własną zależność, FreeType (do czcionek), więc będą dwie nowe pozycje, a nie jedna. Zgłoszone z pamięci: czy RmlUi buduje się z `FetchContent` bez dodatkowych kroków i jakiego FreeType potrzebuje, sprawdzę na przypiętej wersji.
- *Interfejs renderujący i wejście.* Zgłoszone z pamięci znajomości biblioteki, **do sprawdzenia na przypiętej wersji przed napisaniem kodu**: w przykładach RmlUi są interfejs renderujący dla OpenGL 3 i backend platformy dla GLFW, które mogą posłużyć za punkt wyjścia. Nie wiem, czy zakładają ten sam loader GL co projekt (GLAD) i czy nie wymagają kontekstu nowszego niż 4.1.
- *OpenGL 4.1 Core i macOS.* Projekt zostaje przy `#version 410 core` na obu systemach. Czy RmlUi z tym profilem działa na macOS, trzeba sprawdzić na obu systemach, zanim cokolwiek na nim się zbuduje. Ma to swoją pozycję w sekcji 5.
- *Dwa systemy interfejsu w jednej klatce.* Dziś na końcu klatki rysuje się minimapa i na wierzchu ImGui ([`srgb-encode-in-shader.md`](srgb-encode-in-shader.md): ImGui rysuje po przebiegu składającym do tego samego okna). Menu RmlUi musi mieć ustaloną kolejność względem nich (po przebiegu składającym, a przed czy po panelach debug) i stan GL zostawiony tak, żeby oba systemy sobie nie przeszkadzały. Do ustalenia też routing wejścia: gdy menu jest otwarte, wejście ma iść do menu, a nie do gry ani paneli. Dziś `core::Input` dostaje od panelu ImGui neutralną flagę, że klawiatura i mysz są zajęte ([`../modules/core/input.md`](../modules/core/input.md)); dla menu będzie potrzebna druga taka flaga albo wspólny mechanizm.
- *Rozmycie sceny za menu.* Zgłoszone jako pomysł, nie sprawdzone: rozmyta scena może pochodzić z własnego przebiegu rozmycia gry (rozmycie bloomu w [`../../src/game/PostProcess.hpp`](../../src/game/PostProcess.hpp) i `Bloom.*`), a nie z biblioteki interfejsu. To zależy od rozstrzygnięcia punktu 3 decyzji: przy pętli wideo żadne rozmycie sceny nie jest potrzebne.
- *Stany gry.* Nie istnieją. `RoundState` ma tylko `Playing` i `Won` (sprawdzone w `Round.hpp`). Menu, gra, pauza i koniec rundy to stany, których dziś nie ma, a pauza, zapowiedziana w [`small-calls-after-m6.md`](small-calls-after-m6.md), przychodzi razem z menu. Nowe stany będą wymagały własnych testów bez okna, tak jak reguły rundy.
- *Dokumentacja zostaje obowiązkiem.* Dokument biblioteki w `docs/libraries/` (jak [`../libraries/imgui.md`](../libraries/imgui.md)) powstaje razem z pierwszym kodem, a dokument modułu menu, gdy kod istnieje. Ta notatka niczego z tego nie zastępuje.

**Czego ta notatka nie przesądza.** Tła menu (punkt 3 decyzji), wyglądu, liczby ekranów ani tego, czy HUD gry, który dziś jest oknem ImGui, przejdzie kiedyś do RmlUi. Decyzja dotyczy menu.

## 5. Kiedy wrócić do tej decyzji

- Gdy zacznie się M9: przypiąć wersję RmlUi i FreeType, sprawdzić interfejs renderujący i backend GLFW na przypiętej wersji, sprawdzić oba na obu systemach.
- Jeśli RmlUi nie działa na macOS z profilem 4.1 Core: wtedy wybór trzeba rozstrzygnąć od nowa, bo projekt musi się budować na macOS i Windowsie.
- Jeśli czasu przed obroną zabraknie: planem awaryjnym jest menu w Dear ImGui (drugi wiersz tabeli), które jest zgodne z PRD i mieści się w tym, co już jest w buildzie.
- Gdy będzie gotowe porównanie tła: prawdziwa żywa scena kontra prawdziwa nagrana pętla z gry. Wtedy punkt 3 decyzji dostanie rozstrzygnięcie, w tej notatce albo w osobnej.
