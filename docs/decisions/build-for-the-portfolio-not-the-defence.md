# Budujemy to, co chcemy, dla portfolio, a nie pod obronę

Data: 2026-10-06. Stan: obowiązuje jako decyzja o kierunku pracy. Decyzja właściciela projektu jest w całości w sekcji 2 (dosłowny cytat); skutki, tabela i warunki powrotu to moja analiza. Rozszerza poluzowanie reguły zapisane po raz pierwszy w [`menu-in-rmlui.md`](menu-in-rmlui.md).
Kod: brak (to decyzja o kolejności i celu pracy, nie o kodzie). Dotyczy PRD (zasada zero tajemnic) i sformułowań w [`../README.md`](../README.md).

## 1. Kontekst

Projekt ma dwa cele: zaliczenie przedmiotu (15 tematów wykładu, obrona kodu) i kod do portfolio ([`enemy-after-m5.md`](enemy-after-m5.md), sekcja 1: "drugi cel sięga dalej niż plan wykładu"). Zasada zero tajemnic z PRD mówi, że do repozytorium nie trafia kod, którego autor nie potrafi wytłumaczyć, a obrona polega na przeglądzie kodu i pytaniach prowadzącego ([`menu-in-rmlui.md`](menu-in-rmlui.md), sekcja 1).

Ta zasada została poluzowana raz: [`menu-in-rmlui.md`](menu-in-rmlui.md), sekcja 2, punkt 2: właściciel wybrał RmlUi, wiedząc, że "na obronie prawdopodobnie nie starczy czasu, żeby wyjaśnić wszystko", i że na obronie wyjaśni "podstawowe pojęcia". To było poluzowanie dla **warstwy menu**.

## 2. Decyzja

Decyzja właściciela projektu (2026-10-06), jego słowami, w całości:

> "we just build what we want we don't care about defence as we going to make it my game for portfolio i will just tell in defence main concepts and lessons required etc"

To jest cała treść decyzji. Wszystko poniżej to analiza.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz jest decyzją właściciela.

| Możliwość | Zalety | Wady |
|---|---|---|
| **Budować to, co poprawia grę; na obronie opowiedzieć główne pojęcia i wymagane tematy (wybrane)** | decyzje o zakresie (fabuła, przeciwnik, dźwięk, wideo w menu) zapadają według tego, co daje lepszą grę | część kodu będzie poza tym, co da się wytłumaczyć linia po linii na obronie |
| Zostać przy regule zero tajemnic dla całego projektu | każdy wiersz kodu da się wytłumaczyć | zakres ogranicza się do tego, co się zmieści w obronie; gra do portfolio rośnie wolniej |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienie jest w cytacie z sekcji 2 i nie dodaję do niego nic od siebie.

**Skutki** (analiza, mój odczyt decyzji; właściciel może go sprostować):

- **Kolejność pracy** wyznacza to, co poprawia grę, a nie to, co wymaga plan obrony.
- **Obrona** obejmuje **główne pojęcia i tematy wykładu**, które są wymagane. Nie obejmuje każdej linii kodu.
- **Dokumenty modułów zostają pisane tak jak dotąd** (dziesięć sekcji, kod z objaśnieniem, pytania kontrolne), bo to materiał do nauki i do wyjaśnienia głównych pojęć. Decyzja nie mówi, że dokumentacja ma być pominięta.
- **Sformułowania w [`../README.md`](../README.md) i w PRD**, że na obronie tłumaczę każdą linię kodu, opisują wcześniejszy cel. Ta notatka ich nie zmienia i nie rozstrzyga, czy zmieniać; to pytanie do właściciela.
- **Nowe elementy wykraczające poza plan wykładu** (premisa fabularna [`story-premise-the-last-lamp.md`](story-premise-the-last-lamp.md), przeciwnik [`enemy-light-shy-shadow.md`](enemy-light-shy-shadow.md), dźwięk [`audio-on-miniaudio.md`](audio-on-miniaudio.md), wideo w tle menu [`video-through-os-decoders-with-still-fallback.md`](video-through-os-decoders-with-still-fallback.md)) mają tę samą podstawę.

**Czego ta notatka nie przesądza.** Czy tematy wykładu przestają być sprawdzane, jak wygląda obrona w szczegółach ani czy PRD zostanie zmienione.

## 5. Kiedy wrócić do tej decyzji

- Gdy termin obrony się przybliży: sprawdzić, czy wszystkie wymagane tematy z listy 15 są opisane i da się je omówić (to jest zakres "main concepts and lessons required").
- Gdy właściciel zmieni zdanie o roli obrony.
