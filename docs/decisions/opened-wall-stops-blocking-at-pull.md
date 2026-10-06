# Otwarta ściana przestaje blokować w chwili pociągnięcia dźwigni, a nie po zatonięciu

Data: 2026-10-06. Stan: obowiązuje.
Kod: [`src/game/Round.cpp`](../../src/game/Round.cpp) (`pullRoundLever`, `openedWallFlags`, `roundObstacles`, `gateBlocks`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`handleInteraction`), testy w [`tests/InteractionTests.cpp`](../../tests/InteractionTests.cpp). Dokumenty modułów: [`../modules/game/interactables.md`](../modules/game/interactables.md), [`../modules/game/gameplay.md`](../modules/game/gameplay.md).

## 1. Kontekst

Dźwignia opuszcza jedną wewnętrzną ścianę labiryntu (decyzja właściciela projektu z 2026-10-06). Ściana nie znika w jednej klatce: tonie w ziemi tak samo jak brama, przez `GATE_OPEN_SECONDS` = 1,5 s, na głębokość `GATE_SINK_DEPTH` = 3,3 m. Trzeba było rozstrzygnąć, **kiedy** jej pudełko kolizji przestaje być przeszkodą. Od tego zależą trzy rzeczy naraz: czy gracz może przejść, czy promień wskazywania widzi to, co jest za ścianą, i co rysuje debugowy widok pudełek.

Lista przeszkód dla gracza (`roundObstacles`) jest też listą przesłaniaczy dla promienia wskazywania. Jedna lista, jedna odpowiedź na wszystko.

## 2. Decyzja

Pudełko otwartej ściany znika z listy przeszkód **w chwili pociągnięcia dźwigni** (`pullRoundLever`), a nie wtedy, gdy ściana zatonie. To ta sama zasada co dla bramy (`gateBlocks`: brama blokuje do chwili otwarcia, a nie do końca opadania). Skutek: przez 1,5 s gracz może przejść przez ścianę, którą jeszcze widać, a promień wskazywania przechodzi przez nią od razu. To wybór wykonawczy, nie decyzja właściciela.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Pudełko znika przy pociągnięciu (wybrana)** | jedna reguła dla bramy i ściany. Lista przeszkód zmienia się raz, przy pociągnięciu, i jest przebudowywana tylko wtedy. Nic nie zależy od czasu klatek. Test ma prostą asercję: pudełka ściany nie ma na liście | przez 1,5 s widać ścianę, którą można przejść. Gracz może wejść w ścianę, która jeszcze tonie |
| Pudełko zostaje do końca zatonięcia | obraz i kolizja zgodne przez cały czas | trzeba śledzić postęp każdej ściany i usunąć jej pudełko po zatonięciu (albo przebudowywać listę co klatkę). Dwie reguły: jedna dla bramy, druga dla ściany. Gracz czeka 1,5 s przed własnym skrótem |
| Pudełko maleje razem z postępem tonięcia | obraz i kolizja zgodne w każdej chwili | lista przeszkód zależna od czasu, przebudowywana co klatkę. Gracz stojący w ścianie, której pudełko się zmienia, wymagałby osobnej obsługi. Nowy kod i nowe przypadki brzegowe dla efektu trwającego sekundę |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Brama już tak działa ([`../modules/game/gameplay.md`](../modules/game/gameplay.md)): `gateBlocks` jest fałszem od chwili otwarcia, a model jeszcze tonie. Ściana z dźwigni jest tym samym zjawiskiem, więc dostaje tę samą regułę, a oba tonące elementy dzielą funkcje `sinkProgressAfter` i `sinkDepth`. Zasadę "co się otwiera, przestaje być przeszkodą w chwili otwarcia" łatwo wytłumaczyć na obronie i ma jedno miejsce w kodzie.

**Co dostaję.** Lista przeszkód jest przeliczana tylko wtedy, gdy zmienia się stan: aplikacja woła `roundObstacles` po udanym `interact` i po `pullAllLevers`. Promień wskazywania (`pickInRound`) bierze tę samą listę, więc po pociągnięciu widzi kartkę albo dźwignię za otwartą ścianą (test `a closed wall hides what is behind it, the wall a lever opened does not`). Debugowe linie kolizji też biorą tę listę, więc otwarta ściana traci swoje pudełko od razu.

**Co tracę.**

- Przez 1,5 s widać ścianę, która nie blokuje. Gracz idący na wprost zobaczy, że przeszedł przez ścianę, zanim zniknęła. To znany skutek tej decyzji, nie błąd do naprawienia.
- Jak to wygląda w ruchu, nikt jeszcze nie sprawdził ręcznie: przejście przez ścianę w trakcie tonięcia jest na otwartej liście kontrolnej w [`../guides/build-windows.md`](../guides/build-windows.md).

## 5. Kiedy wrócić do tej decyzji

- Gdy 1,5 s przechodzenia przez widoczną ścianę okaże się w grze zauważalną wadą. Zmiana powinna objąć bramę i ścianę razem, bo dzielą regułę.
- Gdy dojdzie przeciwnik albo coś innego, co ma się zatrzymywać na tonącej ścianie.
- Gdy ściany zaczną się podnosić z powrotem (dziś dźwignię można pociągnąć raz na rundę).
