# Kamera menu jedzie jedną zamkniętą pętlą przez wszystkie kryształy, a nie po ręcznie wybranych ujęciach

Data: 2026-10-06. Stan: obowiązuje, **wybór wykonawczy** (autor kodu), nie decyzja właściciela. Pytanie właściciela, czy spacer ma zostać zamieniony na wybrane proste korytarze z przenikaniami, jest **otwarte**.
Kod: [`src/game/MenuCamera.cpp`](../../src/game/MenuCamera.cpp) (`menuCameraRoute`, `menuCameraTargets`, `laneCorners`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onUpdate`, `onRender`). Dokument modułu: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md).

## 1. Kontekst

Kamera menu ma jechać po labiryncie sama, powtarzalnie i bez skoku w miejscu zamknięcia pętli. Pozycja ma być funkcją labiryntu, ustawień i czasu (bez losowości i bez własnego zegara), żeby nagranie z tego samego ziarna dało te same klatki ([`deterministic-random.md`](deterministic-random.md)). Tłem menu jest od tego samego dnia zmontowana pętla wideo ([`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md)), a ten tryb ją nagrywa.

## 2. Decyzja

Spacer to **jedna zamknięta pętla**: obejście drzewa najkrótszych dróg od startu do każdego kryształu i do komórki przed bramą, każde przejście raz w każdą stronę, z regułą jednej ręki na ścianie. Prędkość jest stała (0,7 m/s), a runda w tym czasie stoi, poza zegarem animacji (kryształy dalej się kołyszą). To mój wybór; właściciel nie decydował o tym, jak tryb działa.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Jedna zamknięta pętla przez wszystkie cele (wybrane)** | zero ręcznego wyboru, działa dla każdego ziarna i rozmiaru; zamyka się bez skoku; deterministyczna; każdy kryształ i brama są w trasie | mniej więcej połowa trasy to ściany z bliska w ciasnych zakrętach i przy zawracaniu (widziane na klatkach przez agenta); długa (zgłoszone około 500 s w labiryncie 10 na 10) |
| Ręcznie wybrane proste korytarze z przenikaniami | same dobre ujęcia | wybór dla jednego ziarna, nie uogólnia się na inne labirynty; trzeba napisać montaż. Do nagrania i tak służy zmontowana pętla, więc wybór można zrobić na nagraniu |
| Losowa wędrówka po labiryncie | prosta | niedeterministyczna albo zależna od ziarna drugiej losowości, niepowtarzalna, nie zamyka się |
| Ścieżka tylko do wyjścia i z powrotem | krótka | pomija kryształy, które są ciekawe w kadrze |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Jedna pętla jest najprostszym zachowaniem, które jest powtarzalne, zamknięte i obejmuje to, co w grze jest warte pokazania (kryształy, brama). Wybór dobrych ujęć zostaje na etapie nagrywania i montażu, gdzie przełączniki (`--seed`, `--menu-shot`, `--menu-time`) wskazują, który kawałek pętli się nagrywa.

**Zamrożenie rundy, ale nie zegara animacji.** `onUpdate` kończy się wcześnie w trybie, więc gracz, bateria, zbieranie i czas rundy stoją; dodawany jest tylko krok do `animationSeconds`, bo kryształy bez kołysania i pulsowania wyglądałyby na wklejone w obraz. Koszt: brama i ściany po dźwigniach, które opadały w chwili włączenia, zatrzymują się w połowie, bo ich postęp liczy `updateRound`.

**Znane koszty.** Zmiana prędkości suwakiem w trakcie przeskakuje kamerę (pozycja to `(sekundy + przesunięcie) * prędkość`).

## 5. Kiedy wrócić do tej decyzji

- Gdy właściciel zdecyduje, że spacer ma zostać zamieniony na wybrane proste korytarze z przenikaniami.
- Gdy pętla w labiryncie 10 na 10 okaże się za długa do nagrywania.
- Gdy pojawi się potrzeba zmiany prędkości w trakcie bez przeskoku (wtedy zegar trzeba przeliczać).
