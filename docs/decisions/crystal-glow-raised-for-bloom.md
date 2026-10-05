# Świecenie kryształów podniesione z 2,5 do 4,0 razem z bloomem

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Crystals.hpp`](../../src/game/Crystals.hpp) (`CRYSTAL_GLOW_STRENGTH`, `CRYSTAL_PULSE_DEPTH`), [`src/game/Crystals.cpp`](../../src/game/Crystals.cpp) (`crystalGlow`, `crystalPulse`), [`src/game/Bloom.hpp`](../../src/game/Bloom.hpp) (`BloomSettings::threshold`). Dokumenty modułów: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 2.15, [`../modules/game/gameplay.md`](../modules/game/gameplay.md).

## 1. Kontekst

Kryształ świeci składnikiem emisyjnym: liniowy kolor świateł punktowych razy `CRYSTAL_GLOW_STRENGTH` razy puls, który chodzi między 0,7 a 1. Pierwsza część M7 ustawiła siłę na 2,5 ([`aces-default-tone-mapping.md`](aces-default-tone-mapping.md)). Bloom z drugiej części bierze z obrazu to, co leży ponad progiem 0,8, i miał dać kryształom poświatę.

Zgłoszona obserwacja po włączeniu bloomu: przy sile 2,5 poświata znikała w najciemniejszej chwili pulsu. Powód jest w kolejności mnożeń. Składnik emisyjny jest w shaderze mnożony przez teksturę kryształu, a komentarz przy stałej mówi, że tekstura zabiera ponad połowę. To, co zostaje, przy sile 2,5 i pulsie 0,7 spadało pod próg.

## 2. Decyzja

`CRYSTAL_GLOW_STRENGTH` wynosi **4,0**. Próg bloomu zostaje na 0,8, a głębokość pulsu na 0,3.

## 3. Rozważane możliwości

Liczby to jasność (luminancja) samego składnika emisyjnego dla domyślnego koloru świateł, przed mnożeniem przez teksturę, policzona ze wzorów.

| Możliwość | Jasność przy pulsie 1 | Jasność przy pulsie 0,7 | Zalety | Wady |
|---|---|---|---|---|
| zostawić 2,5 | 1,534 | 1,074 | nic się nie zmienia w wyglądzie bez bloomu | tekstura może przyciemnić najwyżej o 26 procent, zanim piksel w dolnej chwili pulsu spadnie pod próg 0,8. Zgłoszone: poświata znikała |
| **podnieść siłę do 4,0 (wybrana)** | 2,455 | 1,719 | w dolnej chwili pulsu zapas to 53 procent, w górnej 67. Poświata jest w całym cyklu i zmienia się płynnie | kryształy są jaśniejsze i bledsze także **bez** bloomu, bo jaśniejszy kolor ląduje wyżej na krzywej ACES |
| obniżyć próg bloomu | 1,534 | 1,074 | kryształ zostaje, jaki był | próg jest wspólny dla całej sceny: niższy zapaliłby też niebo i ściany w świetle latarki. Komentarz przy progu ustawia go celowo powyżej ścian |
| zmniejszyć głębokość pulsu | 1,534 | bliżej 1,534 | poświata przestaje znikać | puls jest częścią wyglądu kryształu i jego światła punktowego. Mniej pulsu to mniej życia w scenie |
| rozjaśnić teksturę kryształu | 1,534 | 1,074 | naprawia przyczynę, a nie skutek | nowy zasób graficzny zamiast jednej liczby, a tekstura jest też oświetlana latarką i światłem otoczenia |
| osobny mnożnik tylko dla bloomu (kryształ pisze swoją jasność do drugiego załącznika) | pełna kontrola: wygląd i poświata niezależne | drugi załącznik koloru w buforze sceny i zmiany we wszystkich shaderach sceny. Framebuffer ma dziś jeden załącznik |

## 4. Uzasadnienie i skutki

**Dlaczego siła, a nie próg.** Próg rozstrzyga o całej scenie, a siła świecenia o jednej rzeczy. Kryształ jest jedyną rzeczą w labiryncie, która ma być wyraźnie jaśniejsza od bieli, i komentarz przy stałej mówił to już w pierwszej części.

**Dlaczego 4,0.** Nie mam zapisu, jak dokładnie dobrano tę liczbę. Komentarz przy `BloomSettings` mówi, że wartości startowe bloomu są częścią wyglądu nocy i zostały dobrane razem ze świeceniem kryształów i jasnością nieba. Rachunek pokazuje tyle: przy 4,0 zapas w dolnej chwili pulsu (53 procent) jest dwa razy większy niż przy 2,5 (26 procent).

**Skutki, które przyjmuję.**

- Składnik emisyjny przy pełnym pulsie to (0,132, 3,150, 2,415) zamiast (0,083, 1,969, 1,510). Po krzywej ACES (0,184, 0,957, 0,935) zamiast (0,096, 0,913, 0,878): zielony i niebieski są bliżej siebie i bliżej bieli.
- Klatka z wyłączonym bloomem nie jest już identyczna z klatką pierwszej części. Zgłoszone porównanie co do piksela zrobiono przed tą zmianą.
- Liczby w starszych notatkach ([`aces-default-tone-mapping.md`](aces-default-tone-mapping.md), [`gamma-linear-pipeline.md`](gamma-linear-pipeline.md)) opisują stan z siłą 2,5 i zostają jako zapis tamtej decyzji.
- Trzy rzeczy są teraz związane: `CRYSTAL_GLOW_STRENGTH`, `CRYSTAL_PULSE_DEPTH` i `BloomSettings::threshold`. Zmiana jednej wymaga sprawdzenia poświaty w dolnej chwili pulsu.

**Czego nie zmierzyłem.** Jasności tekstury kryształu nie mierzyłem: zdanie "zabiera ponad połowę" pochodzi z komentarza w kodzie. Jeśli zabiera wyraźnie więcej niż 53 procent, to ciemniejsze teksele spadają pod próg także przy 4,0, a poświatę w dolnej chwili pulsu dają tylko te jaśniejsze. Zgłoszone zrzuty pokazują poświatę w obu skrajnych chwilach pulsu w trzech trybach cieniowania, ze ściankami kryształu nadal widocznymi. Sam ich nie oglądałem. Na ekranie MacBooka nikt tego nie widział.

## 5. Kiedy wrócić do tej decyzji

- Gdy zmieni się próg bloomu, głębokość pulsu albo tekstura kryształu.
- Gdy zmieni się domyślny kolor świateł punktowych: jasność zależy głównie od kanału zielonego (waga 0,72), więc czerwony albo niebieski kryształ o tej samej sile miałby dużo mniejszą jasność i mógłby nie świecić wcale.
- Gdy kryształy okażą się za blade bez bloomu albo na innym ekranie: wtedy osobna jasność dla bloomu zamiast wspólnej.
- Gdy framebuffer sceny dostanie drugi załącznik koloru z innego powodu.
