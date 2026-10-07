# Przeciwnik: cień, który rusza się tylko wtedy, gdy nie jest oświetlony ani widziany

Data: 2026-10-06, uzupełniona 2026-10-07 (trzy pytania otwarte rozstrzygnięte przez właściciela, sekcja 2a). Stan: obowiązuje jako decyzja o zachowaniu przeciwnika, **kodu przeciwnika nie ma**. Częściowo zastępuje [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) (sekcja 4 niżej mówi, co się zmienia, a co zostaje) i **wynika z** [`enemy-after-m5.md`](enemy-after-m5.md). Decyzja właściciela projektu jest w całości w sekcji 2; kontekst, tabela, skutki i pytania otwarte to moja analiza.
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

## 2a. Uzupełnienie z 2026-10-07: trzy decyzje właściciela o pytaniach otwartych

Decyzje właściciela projektu (2026-10-07), w całości. Sekcja 2 wyżej zostaje bez zmian.

1. **Złapanie resetuje wszystko, tak samo jak restart**: kryształy, baterię, dźwignie i minimapę.
2. **Calm jest przełącznikiem, który działa na każdym rozmiarze labiryntu**, a nie czwartym poziomem trudności.
3. **Tylko latarka zatrzymuje cień.** Światło księżyca i blask kryształów go nie zatrzymują.

Nic ponad to nie zostało rozstrzygnięte w tych decyzjach. Poniższe zdania tej sekcji to analiza.

- **Co te decyzje zamykają** (analiza): punkt 1 odpowiada na pytanie "Czy złapanie zeruje licznik kryształów i czas?" w części, którą wylicza (kryształy, bateria, dźwignie, minimapa); o czasie rundy decyduje słowo "tak samo jak restart", a co restart robi z czasem, opisuje [`../modules/game/gameplay.md`](../modules/game/gameplay.md). Punkt 2 odpowiada na pytanie "Czy Calm to czwarty poziom?". Punkt 3 rozstrzyga **część** pytania "Co znaczy nieoświetlony?" (tylko latarka; księżyc i kryształy nie liczą się), więc zniknęła obawa z tej notatki, że księżyc oświetlający cały teren uniemożliwi ruch przeciwnika.
- **Co zostaje otwarte** (analiza): "niewidziany" nie jest zdefiniowane. W "nieoświetlony" nie wiadomo jeszcze, co dokładnie znaczy "oświetlony latarką" (cały stożek, jego część, ściana między latarką a cieniem). Kamień milowy, algorytm ruchu, wygląd, testy i powtarzalność dla ziarna są dalej otwarte.
- **Co z liczbami Calm** (analiza): skoro Calm jest przełącznikiem na każdym rozmiarze, nie ma własnego wiersza w tabeli poziomów, więc pytanie "Co z rozmiarem i liczbami Calm?" w dużej mierze przestaje mieć sens. Nie zapisuję go jako rozstrzygniętego: właściciel o liczbach nic nie powiedział.
- **Skutek dla innych notatek** (analiza, bez edycji tamtych notatek): [`menu-scope-for-m9.md`](menu-scope-for-m9.md) mówi o trzech poziomach trudności w tabeli, i ta tabela się nie zmienia, bo Calm nie jest jej czwartym wierszem. Przełącznika jeszcze nie ma w menu ani w kodzie.

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
| co się dzieje, gdy dogoni gracza, i czy wraca stan przegranej | rozstrzygnięte: powrót na początek tego samego labiryntu, bez ekranu przegranej; od 2026-10-07 także: resetuje się wszystko jak przy restarcie (sekcja 2a) |
| jak ma się do światła | rozstrzygnięte w zarysie: rusza się tylko, gdy jest nieoświetlony i niewidziany; od 2026-10-07 "oświetlony" znaczy "oświetlony latarką" (sekcja 2a), "niewidziany" otwarte |
| czy w ogóle, do którego kamienia milowego trafia i co z niego wypada | **otwarte** (PRD wymienia opcjonalnego przeciwnika w M9, który jest rozpoczęty; decyzji, w którym kamieniu powstanie, nie zapisano) |
| jak porusza się po labiryncie i skąd wie, gdzie jest gracz | **otwarte** |
| jak to przetestować bez okna | **otwarte** |
| czy jego zachowanie ma być powtarzalne dla ziarna | **otwarte** |

**Pytania otwarte, które ta decyzja stawia** (analiza):

- **Co znaczy "niewidziany"?** *(Nadal otwarte po 2026-10-07.)* Poza polem widzenia kamery, bez linii wzroku do gracza, czy oba? Gra ma już linię wzroku w minimapie ([`../modules/renderer/minimap.md`](../modules/renderer/minimap.md)), ale to jest moje spostrzeżenie o istniejącym kodzie, nie propozycja.
- **Co znaczy "nieoświetlony"?** *(Częściowo rozstrzygnięte 2026-10-07: tylko latarka, księżyc i kryształy nie liczą się; sekcja 2a. Poniżej stan z 2026-10-06.)* Stożek latarki gracza, światło kryształów, światło księżyca, wszystko naraz? Księżyc oświetla cały teren, więc wybór wpływa na to, czy przeciwnik w ogóle kiedykolwiek może się ruszyć.
- **Czy Calm to czwarty poziom?** *(Rozstrzygnięte 2026-10-07: przełącznik na każdym rozmiarze, nie czwarty poziom; sekcja 2a. Poniżej stan z 2026-10-06.)* Notatka [`menu-scope-for-m9.md`](menu-scope-for-m9.md) mówi o **trzech** poziomach trudności zmieniających rozmiar labiryntu i liczbę kryształów, a tabela w kodzie ma trzy wiersze ([`../modules/game/difficulty.md`](../modules/game/difficulty.md)). Decyzja właściciela mówi o poziomie Calm, ale nie mówi, czy to czwarty wiersz tabeli, czy jedna z istniejących nazw, czy osobne ustawienie. Do rozstrzygnięcia.
- **Co z rozmiarem i liczbami Calm?** Nie określono. *(2026-10-07: przy przełączniku na każdym rozmiarze w dużej mierze bez znaczenia, ale nie rozstrzygnięte.)*
- **Czy złapanie zeruje licznik kryształów i czas?** *(Rozstrzygnięte 2026-10-07: wszystko jak przy restarcie; sekcja 2a. Poniżej stan z 2026-10-06.)* "Wraca na początek tego samego labiryntu" mówi o miejscu gracza; czy kryształy i bateria się resetują, nie zostało powiedziane.

**Czego ta notatka nie przesądza.** Kamienia milowego, algorytmu ruchu, definicji "niewidziany" i "nieoświetlony", wyglądu przeciwnika, liczb poziomu Calm ani tego, co dokładnie dzieje się z postępem rundy po złapaniu. *(Stan z 2026-10-06; od 2026-10-07 postęp rundy po złapaniu i część definicji "nieoświetlony" są rozstrzygnięte, sekcja 2a.)*

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie projekt przeciwnika: rozstrzygnąć pytania otwarte z sekcji 4 i dopisać je tutaj albo w dokumencie modułu.
- Gdy powstanie kod: zmienić stan notatki [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) z "obowiązuje" na odpowiednio opisany ("obowiązuje na poziomie Calm" albo "zastąpiona"), razem z dokumentem [`../modules/game/gameplay.md`](../modules/game/gameplay.md).
- Jeśli właściciel zmieni zdanie o Calm (od 2026-10-07 przełącznik na każdym rozmiarze, nie poziom): poprawić tu i w [`menu-scope-for-m9.md`](menu-scope-for-m9.md).
