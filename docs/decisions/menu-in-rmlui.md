# Menu gry w RmlUi już w M9, Dear ImGui zostaje przy panelach debug

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **w chwili zapisu kodu menu nie było, a od 2026-10-06 (M9, część 2) jest: sekcja 7** (wcześniej tego dnia była tylko kamera menu, sekcja 6, i rozstrzygnięte tło menu: [`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md)).
Kod (stan z chwili zapisu, dzisiejszy opisuje sekcja 7): brak. W `cmake/Dependencies.cmake` nie było ani RmlUi, ani FreeType, w `src/` nie ma menu, a w `docs/` nie ma dokumentu biblioteki RmlUi (sprawdzone wyszukiwaniem tych nazw w `src/`, `cmake/`, `CMakeLists.txt` i `docs/`). Zbudowane jest tylko to, co dziś stoi w `Dependencies.cmake`: GLFW, GLM, Dear ImGui i pozostałe biblioteki z [`../libraries/`](../libraries/). Dokument modułu: brak. Powiązane: [`small-calls-after-m6.md`](small-calls-after-m6.md) (punkt 2: pauza dojdzie razem z menu), [`enemy-after-m5.md`](enemy-after-m5.md) (ten sam sposób zapisu decyzji bez kodu).

**Dopisek z 2026-10-06 (M9, część 3).** Kod menu istnieje od części 2, a od części 3 ma cztery dokumenty (dochodzą ustawienia) i arkusz w `vh`: [`../modules/ui/menu-screens.md`](../modules/ui/menu-screens.md). Zdanie "w `src/` nie ma menu" w nagłówku opisuje chwilę zapisu. Tłem menu głównego nadal jest żywy przelot kamery menu, a nie pętla wideo.

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
- Gdy będzie gotowe porównanie tła: prawdziwa żywa scena kontra prawdziwa nagrana pętla z gry. Wtedy punkt 3 decyzji dostanie rozstrzygnięcie, w tej notatce albo w osobnej. (Rozstrzygnięte 2026-10-06: sekcja 6 i [`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md).)

## 6. Dodatek z 2026-10-06: kamera menu istnieje w kodzie, tło menu jest rozstrzygnięte

Sekcje od 1 do 5 zostają w brzmieniu z chwili zapisu. Co się zmieniło tego samego dnia, później:

- **Kod, którego dotyczy ta notatka, nadal nie istnieje:** RmlUi i FreeType nie ma w `cmake/Dependencies.cmake`, menu nie ma w `src/`. Zmienia się to, że **w kodzie jest kamera menu** (pierwsza część M9): tryb pod klawiszem F2 albo przełącznikiem `--menu-camera`, w którym gra pokazuje samą siebie z ukrytym HUD, minimapą i panelami ([`../modules/game/menu-camera.md`](../modules/game/menu-camera.md); `src/game/MenuCamera.*`, `src/game/StartOptions.*`). To nie jest menu: nie ma ekranów, przycisków ani stanów gry.
- **Materiał porównawczy istnieje:** dwa nagrania z gry (ciągłe ujęcie 30 s i zmontowana pętla 24 s z czterech ujęć), poza repozytorium, pokazane właścicielowi (informacja od autora kodu, nie do sprawdzenia w repozytorium).
- **Punkt 3 decyzji jest rozstrzygnięty.** Właściciel zdecydował po obejrzeniu obu nagrań (2026-10-06): tłem menu będzie **zmontowana, wyrenderowana wcześniej pętla wideo**, nie żywa scena, a przełączniki `--menu-shot` i `--menu-time` zostają. Pełna treść, tabela możliwości i skutki (brak dekodera wideo, plik binarny, starzenie się pętli) są w osobnej notatce [`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md). Punkty 1 i 2 decyzji bez zmian: menu w RmlUi.
- Skutek dla ostatniej pozycji sekcji 5 ("Gdy będzie gotowe porównanie tła"): porównanie powstało i wybór jest zapisany w tej notatce.

## 7. Dodatek z 2026-10-06 (M9, część 2): kod menu istnieje

Sekcje od 1 do 6 zostają w brzmieniu z chwili zapisu, a nagłówek i opis "kodu menu nie ma" na górze jest już nieaktualny. Co się zmieniło (odczytane z kodu na `8c99911`):

- **Zależności są.** `cmake/Dependencies.cmake` przypina RmlUi `6.3` i FreeType `VER-2-14-3`. Ze źródeł RmlUi kompilowane są bez zmian dwa pliki backendu (renderer OpenGL 3 i pomocnicy GLFW), z GLAD projektu jako loaderem. Opis: [`../libraries/rmlui.md`](../libraries/rmlui.md).
- **Kod jest.** Warstwa `ui` (`src/ui/`: `UiLayer`, `AssetFileInterface`) i trzy dokumenty w `assets/ui/` (menu główne, pauza, wynik rundy): [`../modules/ui/README.md`](../modules/ui/README.md).
- **Stany gry są.** `GameMode`, `GameEvent`, `nextMode` w `src/game/GameState.*` z testami; Escape pauzuje: [`../modules/game/game-states.md`](../modules/game/game-states.md).
- **Punkty z sekcji 4, które dało się sprawdzić:** kolejność w klatce to scena, minimapa, menu RmlUi, panele ImGui; wejście do menu idzie przez wywołania zwrotne GLFW, które Dear ImGui przekazuje dalej, a panele wygrywają z menu pod kursorem. Co zostaje otwarte: macOS z profilem 4.1 Core (nikt tego nie budował ani nie uruchamiał) i ostrość na ekranie Retina.
- Zasada zero tajemnic została poluzowana tak, jak zapisano w sekcji 4: warstwa menu jest opisana w dokumentach, ale większa, niż zdążę wytłumaczyć na obronie.
