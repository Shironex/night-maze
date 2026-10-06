# Escape zatrzymuje grę i cofa o jeden ekran, wyjście z programu to przycisk menu

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **z kodem** (commit `4f7562a`). Decyzja właściciela projektu jest w sekcji 2; kontekst, tabela możliwości i skutki to moja analiza. Zastępuje punkt 2 z [`small-calls-after-m6.md`](small-calls-after-m6.md) (tam Escape tylko zwalniał mysz, a pauza miała przyjść razem z menu).
Kod: [`src/core/Application.cpp`](../../src/core/Application.cpp) (`onEscapePressed`, domyślnie zamyka okno), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onEscapePressed`, `handleGameEvent`), [`src/game/GameState.cpp`](../../src/game/GameState.cpp) (`nextMode`), [`tests/GameStateTests.cpp`](../../tests/GameStateTests.cpp). Dokumenty: [`../modules/game/game-states.md`](../modules/game/game-states.md), [`../modules/core/main-loop.md`](../modules/core/main-loop.md).

## 1. Kontekst

Do M9 klawisz Escape robił w pętli głównej dwie rzeczy: przy przechwyconym kursorze oddawał kursor, przy wolnym zamykał okno. Runda biegła dalej, a przypadkowe dwa naciśnięcia kończyły program. Gdy gra dostała ekrany (menu główne, pauza, koniec rundy), trzeba było rozstrzygnąć, co Escape znaczy na każdym z nich i jak się z gry wychodzi.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. **Escape zatrzymuje grę i cofa o jeden ekran**: z rundy do menu pauzy, z menu pauzy z powrotem do rundy, z ekranu wyniku do menu głównego.
2. **Wyjście z programu to przycisk `Quit` w menu głównym.** Escape nigdy nie zamyka programu.

Reszta tej notatki to analiza.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Escape cofa o jeden ekran, wyjście przyciskiem (wybrane)** | jedna prosta reguła ("wstecz"); nie da się zamknąć programu przypadkiem w trakcie rundy; pauza jest wbudowana w ten sam klawisz | w menu głównym Escape nie robi nic, więc wyjście wymaga myszy |
| Escape zamyka program (stan sprzed M9) | zero kodu, jak dotąd | jedno przypadkowe naciśnięcie kończy rundę bez pytania; nie da się zrobić pauzy na tym samym klawiszu |
| Escape tylko pauzuje, z wyniku nie cofa | mniej przypadków do testowania | ekran wyniku miałby wyjście tylko przyciskiem, a reszta gry cofa klawiszem: niespójne |

## 4. Uzasadnienie i skutki

- **Reguła jest tabelą.** `game::nextMode` przyjmuje ekran i zdarzenie i zwraca następny ekran. Wiersze z Escape: `Playing` do `Paused`, `Paused` do `Playing`, `RoundEnd` do `MainMenu`. W `MainMenu` Escape niczego nie zmienia (test `in the main menu Escape and the buttons of other screens do nothing`). Całą tabelę opisuje [`../modules/game/game-states.md`](../modules/game/game-states.md).
- **Wyjście z programu** to zdarzenie `Quit` z przycisku `Quit` menu głównego (`data-action="quit"`). Tylko menu główne może przejść do `Quitting` (test `only the main menu can reach the quitting state`). Kod `NightMazeApp::handleGameEvent` zamyka wtedy okno, a pętla główna kończy się po tej klatce.
- **Escape jest poleceniem programu, a nie pętli.** `core::Application::run` wywołuje wirtualną `onEscapePressed()`. Domyślna wersja zamyka okno, więc program, który jej nie nadpisuje, działa jak dawniej. `NightMazeApp` nadpisuje ją i wysyła zdarzenie `Escape`. Funkcja nie jest wołana, gdy klawiatura jest zablokowana (edycja pola tekstowego).
- **Kursor** nie jest już sprawą Escape: przechwytuje go ekran `Playing`, a każdy ekran z menu go oddaje (`NightMazeApp::showScreen`).
- **Bez dokumentów menu** (błąd wczytania) gra startuje w rundzie, a Escape może wejść w pauzę, która nic nie pokazuje, i z niej wyjść. Menu główne i ekran wyniku są wtedy zastąpione nową rundą, bo nie miałyby klawisza wyjścia. To obsługa błędu, nie decyzja właściciela.
- **Koszt.** Escape nadal oddaje kursor, ale przez otwarcie pauzy: runda wtedy stoi. Nie da się już oddać kursora klawiszem Escape tak, żeby runda biegła dalej. Do paneli debug w trakcie biegnącej rundy służy tylda (po jej naciśnięciu kursor jest oddany, a runda biegnie).

**Czego ta notatka nie przesądza.** Wyglądu pauzy, tego, czy ekran ustawień (patrz [`menu-scope-for-m9.md`](menu-scope-for-m9.md)) też cofa Escape (nie ma go jeszcze w kodzie), ani potwierdzenia przy wyjściu.

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie ekran ustawień: zapisać, dokąd z niego prowadzi Escape (z menu głównego i z pauzy).
- Gdy ktoś zgłosi, że w menu głównym brakuje skrótu klawiaturowego do wyjścia.
