# Fabuła: kierunek "Inheritance", słowa w grze, kolejność budowy i intro na żywo

Data: 2026-10-07. Stan: obowiązuje jako zapis decyzji, **kodu ani tekstu fabularnego w grze jeszcze nie ma**. Rozszerza [`story-premise-the-last-lamp.md`](story-premise-the-last-lamp.md). Decyzje właściciela projektu są w całości w sekcji 2; kontekst, tabele, skutki i warunki powrotu to moja analiza.
Kod: brak. Szkic, z którego właściciel wybierał, jest w repozytorium jako [`../story/the-last-lamp.md`](../story/the-last-lamp.md) (po angielsku, bo to tekst do gry). Dokument modułu: brak.

## 1. Kontekst

Notatka [`story-premise-the-last-lamp.md`](story-premise-the-last-lamp.md) (2026-10-06) zapisała, że premisą jest "The Last Lamp", ale nie zapisała jej treści. Szkic fabuły ([`../story/the-last-lamp.md`](../story/the-last-lamp.md)) rozwija ją w trzech wariantach tej samej premisy ("Inheritance", "Lamp's Back", "The One Before"), podaje listę rzeczy do zbudowania w kodzie (sekcja 5 szkicu) i kończy się sześcioma pytaniami do właściciela (sekcja 6 szkicu).

Właściciel (2026-10-07) wybrał spośród opcji z listy. **Nazwa "Inheritance" oznacza wariant ze szkicu, nie jedną z trzech propozycji premisy z 2026-10-06**, których treści nie mam (tamta notatka mówi to wprost).

## 2. Decyzja

Decyzje właściciela projektu (2026-10-07), podane z listy opcji:

1. Kierunkiem fabuły jest **"Inheritance"**: kartki są zwykłymi radami latarnika, a cień jest dziurą, którą spadły kawałek zostawił w księżycu, i zagląda graczowi do kieszeni.
2. Interfejs gry zachowuje słowo **"Crystals"**, a kartki mówią **"splinters"**.
3. Kolejność budowy: **najpierw kartki, potem ekran z kartą tekstu i intro, kampania później.**
4. Intro jest odtwarzane **na żywo w silniku**, a nie jako nagrane wideo.
5. Płeć latarnika **pozostaje niewypowiedziana**.

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz każdej tabeli jest decyzją właściciela. Pozostałe wiersze są wzięte ze szkicu (sekcje 2, 5 i 6 szkicu), który jest propozycją, a nie decyzją.

**Kierunek (punkt 1):**

| Możliwość | Opis według szkicu |
|---|---|
| **Inheritance (wybrane)** | emocjonalny środek to praca przekazana dalej: zwykła rada od znanej ręki; według szkicu każda istniejąca mechanika ma w niej uzasadnienie |
| "Lamp's Back" | cień gra w dziecięcą grę wedle tych samych zasad; według szkicu to urocze na jedną linię, a potem odbiera grze zagrożenie |
| "The One Before" | kartki od poprzednich latarników, cień jest tym, co zostało z jednego z nich; według szkicu wymaga odkrycia, które psuje się przy kolejności kartek zależnej od ziarna |

**Słownictwo (punkt 2):**

| Możliwość | Uwaga |
|---|---|
| **Interfejs: "Crystals", kartki: "splinters" (wybrane)** | według szkicu ludzie w opowieści mówią "splinters", a gra nazywa je kryształami; szkic przewiduje zdanie w intro, które to tłumaczy |
| Jedno słowo wszędzie | według szkicu to zmiana jednego napisu w HUD |

**Kolejność (punkt 3) i intro (punkt 4):**

| Możliwość | Uwaga według szkicu |
|---|---|
| **Kartki, potem ekran z kartą tekstu i intro, kampania później (wybrane)** | pozycje 1 do 5 i 8 do 9 z tabeli szkicu nie czekają na przeciwnika; kampania to jedyna duża pozycja |
| Kampania od razu | pozycja 10 z tabeli szkicu, oznaczona jako duża |
| **Intro na żywo w silniku (wybrane)** | pozycja 9: stan intro, skrypt ujęć, kamera jest już funkcją labiryntu, ustawień i czasu; koszt to zbudowanie labiryntu dla intro, a potem właściwego, i flaga "intro obejrzane" w ustawieniach |
| Intro jako nagrane wideo | pozycja 9b: tańsze, ale drugi plik binarny, który się starzeje razem z grafiką |

**Płeć latarnika (punkt 5):** wybrano brak określenia. Według szkicu każda linia jest w pierwszej osobie albo mówi "the lamplighter", a nadanie płci nic nie kosztuje później, podczas gdy jej odebranie kosztuje.

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnień właściciela, poza tym, co zawiera sama lista opcji, nie mam. Nie dopisuję własnych.

**Skutki znane i otwarte** (analiza):

- **Szkic w repozytorium nie jest zatwierdzonym tekstem.** Jego status ("proposal for the owner") i jego własne domyślne odpowiedzi nie są decyzjami. W szczególności **propozycje ze szkicu, których właściciel tu nie wymienił, zostają propozycjami**: podniesienie domyślnej liczby kartek z 3 do 6, rozdawanie linii od najbliższej startu, licznik "następna nieprzeczytana linia" w pliku ustawień, 24 linie kartek i pięciu nocy kampanii. Czy je budować, nie jest rozstrzygnięte.
- **Pytania ze szkicu, na które odpowiedź jest gdzie indziej.** Pytania 2, 3 i 4 ze szkicu (reset po złapaniu, Calm jako przełącznik, tylko latarka zatrzymuje cień) właściciel rozstrzygnął tego samego dnia w [`enemy-light-shy-shadow.md`](enemy-light-shy-shadow.md). Pytanie 1 (słowo "Crystals") i 6 (płeć latarnika) rozstrzyga ta notatka. Pytanie 5 (kampania czy najpierw kartki i intro) rozstrzyga punkt 3 tej notatki; jego pozostałe elementy ze szkicu (patrz wyżej) nie są rozstrzygnięte.
- **Szkic zakłada, że cień istnieje.** Linie oznaczone w nim jako zależne od cienia nie mogą trafić do gry, dopóki przeciwnika nie ma ([`enemy-light-shy-shadow.md`](enemy-light-shy-shadow.md)). Szkic proponuje w takim razie wersję bez nich; to jego propozycja.
- **Dwie wcześniejsze notatki o tle menu są niezależne.** Intro na żywo nie dotyczy tła menu ([`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md)), które zostaje nagraną pętlą.
- **Ekran z kartą tekstu nie istnieje.** Nowy dokument RmlUi i nowy stan ekranu to kod, którego nie ma ([`../modules/ui/README.md`](../modules/ui/README.md), [`../modules/game/game-states.md`](../modules/game/game-states.md)).

**Czego ta notatka nie przesądza.** Treści kartek i intro, liczby kartek, liczby i rozmiarów nocy kampanii, kiedy powstanie ekran z kartą tekstu, ani tego, czy kampania w ogóle powstanie.

## 5. Kiedy wrócić do tej decyzji

- Gdy zacznie się pisanie tekstu: rozstrzygnąć, które linie ze szkicu wchodzą do gry, i przenieść zatwierdzone do dokumentu modułu (na przykład [`../modules/game/interactables.md`](../modules/game/interactables.md)).
- Gdy powstanie ekran z kartą tekstu: zapisać jego wygląd i zachowanie osobną notatką.
- Jeśli właściciel zechce jednego słowa w całym interfejsie albo nazwie latarnika: zmienić punkt 2 lub 5.
