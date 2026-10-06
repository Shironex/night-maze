# Przeciwnik: cień, który rusza się tylko wtedy, gdy nie jest oświetlony ani widziany

Data: 2026-10-06. Stan: obowiązuje jako decyzja o zachowaniu przeciwnika, **kodu przeciwnika nie ma**. Częściowo zastępuje [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) (sekcja 4 niżej mówi, co się zmienia, a co zostaje) i **wynika z** [`enemy-after-m5.md`](enemy-after-m5.md). Decyzja właściciela projektu jest w całości w sekcji 2; kontekst, tabela, skutki i pytania otwarte to moja analiza.
Kod: brak. Sprawdzone wyszukiwaniem w `src/`: nie ma tam ani jednego wystąpienia słów `enemy` i `przeciwnik`. Dokument modułu: brak.

## 1. Kontekst

Notatka [`enemy-after-m5.md`](enemy-after-m5.md) (2026-10-05) zapisała, że właściciel chce w grze przeciwnika, ale po M5, i wymieniła pytania, na które trzeba będzie odpowiedzieć przed pracą (jak się porusza, co się dzieje przy złapaniu, czy wraca stan przegranej, jak to się ma do światła). Notatka [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) (2026-10-05) zapisała, że **rundy nie da się przegrać**: pusta bateria tylko gasi latarkę, a stan `RoundState` ma dwie wartości, `Playing` i `Won`.

Rozmowa o kierunku dalszych prac (2026-10-06, po M9, części 4) rozstrzygnęła zachowanie przeciwnika.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. Przeciwnik **rusza się tylko wtedy, gdy jest nieoświetlony i niewidziany**.
2. Gdy przeciwnik **złapie** gracza, gracz wraca **na początek tego samego labiryntu**. **Nie ma ekranu przegranej.**
3. Gra ma poziom **Calm**, w którym przeciwnika **nie ma**.

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz jest decyzją właściciela, pozostałe wiersze to możliwości z [`enemy-after-m5.md`](enemy-after-m5.md) i [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md), które ta decyzja przesuwa.

| Możliwość | Zalety | Wady |
|---|---|---|
| **Przeciwnik w cieniu, złapanie cofa na start, bez ekranu przegranej, poziom Calm bez niego (wybrane)** | światło, które jest tematem gry, jest też narzędziem obrony; nie ma trzeciego stanu rundy z własnym ekranem; gracz, który nie chce zagrożenia, ma poziom bez niego | złapanie jest karą bez ekranu, więc gracz musi ją zrozumieć z samej gry; "niewidziany" i "nieoświetlony" trzeba zdefiniować (sekcja 4) |
| Przeciwnik goniący bez przerwy (opis z [`enemy-after-m5.md`](enemy-after-m5.md)) | najprostszy do opisania | nie używa światła; wymagałby stanu przegranej albo takiej samej kary |
| Brak przeciwnika, sama ciemność ([`battery-darkness-no-loss.md`](battery-darkness-no-loss.md)) | zero kodu | to jest stan do dziś; właściciel zdecydował inaczej |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela, poza treścią punktów decyzji, nie mam. Nie dopisuję własnego.

**Co się zmienia w notatce [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md)** (analiza, nic z tego nie jest jeszcze w kodzie):

- **Zostaje:** pusta bateria tylko gasi latarkę i nie kończy rundy; nie ma ekranu przegranej (decyzja właściciela z 2026-10-06 mówi to samo o złapaniu).
- **Przestaje być prawdą, gdy przeciwnik powstanie:** zdanie "rundy nie da się przegrać" i "dwa stany rundy zamiast trzech" w sensie, że nic nie cofa postępu. Złapanie cofa gracza na początek tego samego labiryntu, więc runda ma porażkę, tylko **bez osobnego ekranu**. Czy potrzebny jest do tego nowy stan `RoundState`, czy wystarczy zdarzenie, które uruchamia ponownie rundę, jest do rozstrzygnięcia i **nie rozstrzygam tego tu**.
- **Poziom Calm** to poziom, na którym zdanie z tamtej notatki nadal jest prawdą.

**Co z pytań z [`enemy-after-m5.md`](enemy-after-m5.md), sekcja 5, jest teraz rozstrzygnięte, a co nie** (analiza):

| Pytanie z tamtej notatki | Stan |
|---|---|
| co się dzieje, gdy dogoni gracza, i czy wraca stan przegranej | rozstrzygnięte: powrót na początek tego samego labiryntu, bez ekranu przegranej |
| jak ma się do światła | rozstrzygnięte w zarysie: rusza się tylko, gdy jest nieoświetlony i niewidziany |
| czy w ogóle, do którego kamienia milowego trafia i co z niego wypada | **otwarte** (PRD wymienia opcjonalnego przeciwnika w M9, który jest rozpoczęty; decyzji, w którym kamieniu powstanie, nie zapisano) |
| jak porusza się po labiryncie i skąd wie, gdzie jest gracz | **otwarte** |
| jak to przetestować bez okna | **otwarte** |
| czy jego zachowanie ma być powtarzalne dla ziarna | **otwarte** |

**Pytania otwarte, które ta decyzja stawia** (analiza):

- **Co znaczy "niewidziany"?** Poza polem widzenia kamery, bez linii wzroku do gracza, czy oba? Gra ma już linię wzroku w minimapie ([`../modules/renderer/minimap.md`](../modules/renderer/minimap.md)), ale to jest moje spostrzeżenie o istniejącym kodzie, nie propozycja.
- **Co znaczy "nieoświetlony"?** Stożek latarki gracza, światło kryształów, światło księżyca, wszystko naraz? Księżyc oświetla cały teren, więc wybór wpływa na to, czy przeciwnik w ogóle kiedykolwiek może się ruszyć.
- **Czy Calm to czwarty poziom?** Notatka [`menu-scope-for-m9.md`](menu-scope-for-m9.md) mówi o **trzech** poziomach trudności zmieniających rozmiar labiryntu i liczbę kryształów, a tabela w kodzie ma trzy wiersze ([`../modules/game/difficulty.md`](../modules/game/difficulty.md)). Decyzja właściciela mówi o poziomie Calm, ale nie mówi, czy to czwarty wiersz tabeli, czy jedna z istniejących nazw, czy osobne ustawienie. Do rozstrzygnięcia.
- **Co z rozmiarem i liczbami Calm?** Nie określono.
- **Czy złapanie zeruje licznik kryształów i czas?** "Wraca na początek tego samego labiryntu" mówi o miejscu gracza; czy kryształy i bateria się resetują, nie zostało powiedziane.

**Czego ta notatka nie przesądza.** Kamienia milowego, algorytmu ruchu, definicji "niewidziany" i "nieoświetlony", wyglądu przeciwnika, liczb poziomu Calm ani tego, co dokładnie dzieje się z postępem rundy po złapaniu.

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie projekt przeciwnika: rozstrzygnąć pytania otwarte z sekcji 4 i dopisać je tutaj albo w dokumencie modułu.
- Gdy powstanie kod: zmienić stan notatki [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) z "obowiązuje" na odpowiednio opisany ("obowiązuje na poziomie Calm" albo "zastąpiona"), razem z dokumentem [`../modules/game/gameplay.md`](../modules/game/gameplay.md).
- Jeśli właściciel zmieni zdanie o poziomie Calm: poprawić tu i w [`menu-scope-for-m9.md`](menu-scope-for-m9.md).
