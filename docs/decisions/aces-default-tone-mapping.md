# Mapowanie tonów: domyślnie krzywa ACES (dopasowanie Narkowicza), z przełącznikiem na Reinharda i na brak krzywej

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (`toneMapReinhard`, `toneMapAces`, `main`), [`src/game/PostProcess.hpp`](../../src/game/PostProcess.hpp) (`ToneMapping`, `PostProcessSettings`), [`src/debug/categories/PostProcessCategory.cpp`](../../src/debug/categories/PostProcessCategory.cpp) (lista `Tone mapping`), wartości dobrane do krzywej: [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp), [`src/game/Crystals.hpp`](../../src/game/Crystals.hpp) (`CRYSTAL_GLOW_STRENGTH`), [`src/game/Skybox.hpp`](../../src/game/Skybox.hpp) (`SkyboxSettings::brightness`). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md).

## 1. Kontekst

Scena jest rysowana do bufora `GL_RGBA16F` i jej kolory mogą być jaśniejsze niż 1: świecenie kryształu ma w kanale zielonym około 1,97, a intensywności świateł są mnożnikami większymi od 1 (latarka 1,3). Ekran pokazuje zakres od 0 do 1. Ostatni przebieg musi więc zdecydować, co zrobić z nadmiarem. Ta operacja nazywa się mapowaniem tonów i stoi między ekspozycją a kodowaniem sRGB.

Trzeba było wybrać, jaka krzywa jest domyślna, i czy inne w ogóle zostają w kodzie. Zasada przewodnia PRD mówi, że każdy temat ma dać się pokazać "przed i po" w panelu.

## 2. Decyzja

W kodzie są **trzy tryby**, przełączane listą `Tone mapping` w kategorii Post process: `None` (samo przycięcie do 1), `Reinhard` (`x / (1 + x)`) i `Aces` (krzywa dopasowana przez Krzysztofa Narkowicza do krzywej referencyjnej ACES, iloraz dwóch wielomianów kwadratowych). **Domyślny jest `Aces`**, z ekspozycją 1,0. Wszystkie wartości startowe świateł, świecenia kryształów i jasności nieba są dobrane do tej pary.

## 3. Rozważane możliwości

Liczby w tabeli to wynik krzywej dla wartości liniowej na wejściu, policzony ze wzorów z `composite.frag`.

| Możliwość | 0,01 | 0,18 | 1 | 2,5 | Zalety | Wady |
|---|---|---|---|---|---|---|
| `None`: przycięcie | 0,01 | 0,18 | 1 | 1 | nic nie zmienia poniżej 1, więc tekstura bez światła wraca na ekran jako plik. Najprostsze do wytłumaczenia | wszystko ponad 1 staje się płaską bielą. Kryształ i środek plamy latarki tracą kształt, a bufor HDR nie daje nic widocznego |
| `Reinhard`: `x / (1 + x)` | 0,0099 | 0,153 | 0,5 | 0,714 | nigdy nie dochodzi do 1, więc niczego nie obcina. Jeden wzór, łatwy do policzenia w pamięci | biel sceny (1) ląduje w połowie skali, więc cały obraz jest ciemniejszy i bardziej płaski. Jasne światła nie dochodzą do bieli ekranu |
| **`Aces`, dopasowanie Narkowicza (wybrana jako domyślna)** | 0,0038 | 0,267 | 0,804 | 0,938 | krzywa w kształcie litery S: średnie tony są podbite (więcej kontrastu), a jasne wartości wyginają się miękko w stronę 1. Kryształ jest wyraźnie najjaśniejszy, ale jego ścianki dają się odróżnić | najciemniejsze tony są mocno ściskane (0,01 staje się 0,0038), co w nocnej scenie widać najbardziej: niebo i światło otoczenia trzeba było podnieść. Pięć liczb dopasowania, których nie da się wyprowadzić, tylko przytoczyć. Krzywa działa na każdy kanał osobno, więc bardzo jasne kolory tracą nasycenie i przesuwają odcień |
| Jedna krzywa na stałe, bez przełącznika | mniej kodu, żadnego uniformu | nie da się pokazać na obronie, po co krzywa jest: bez porównania z przycięciem różnica jest niewidoczna dla kogoś, kto nie zna sceny |
| Inne znane krzywe (na przykład filmowa krzywa Hable'a z gry Uncharted 2) | podobny charakter do ACES | nie budowałem ich i nie porównywałem. Trzy tryby wystarczają, żeby pokazać ideę: brak krzywej, najprostsza krzywa, krzywa o kształcie S |

## 4. Uzasadnienie i skutki

**Dlaczego ACES jako domyślna.** Gra jest nocna: większość obrazu jest ciemna, a kilka rzeczy (kryształy, plama latarki, księżyc) ma być wyraźnie jasnych. Krzywa S robi dokładnie to: ściska ciemne tony, rozciąga średnie i miękko zagina jasne. Reinhard z ekspozycją 1 sprowadza biel sceny do połowy skali i odbiera światłom siłę. Samo przycięcie zamienia każdy kryształ w plamę jednego koloru, co opisywał już komentarz przy `CRYSTAL_GLOW_STRENGTH` sprzed M7.

**Dlaczego przełącznik zostaje.** Trzy tryby to pokaz tematu 10: ten sam bufor HDR, trzy sposoby sprowadzenia go do ekranu. Tryb `None` jest też narzędziem: z ekspozycją 1 pokazuje, co naprawdę jest w buforze poniżej bieli, a widoki diagnostyczne są zawsze rysowane właśnie nim ([`gamma-linear-pipeline.md`](gamma-linear-pipeline.md), rozstrzygnięcie 4).

**Skutki, które przyjmuję.**

- Wartości startowe są związane z krzywą. Jasność nieba wzrosła z 1,0 do 2,2 (górna granica suwaka z 3 do 6), a komentarz przy `SkyboxSettings::brightness` mówi dlaczego: krzywa ściska ciemne tony, więc obrazy nieba trzeba podnieść, a gwiazdy i księżyc przekraczają wtedy 1 w buforze. Przełączenie na `Reinhard` albo `None` bez zmiany świateł daje scenę o innej jasności. To oczekiwane, nie błąd.
- Granica krzywej to `2.51 / 2.43`, czyli 1,033, więc wzór kończy się przycięciem do 1. Krzywa osiąga 1 przy wejściu około 7,24: wszystko jaśniejsze jest bielą.
- Krzywa przecina prostą `y = x` przy około 0,062 i 0,73: poniżej pierwszej wartości przyciemnia, między nimi rozjaśnia, powyżej drugiej znowu przyciemnia.
- Mapowanie tonów działa na kanał, nie na jasność. Cyjanowy kryształ `(0.083, 1.969, 1.510)` staje się `(0.096, 0.913, 0.878)`: zielony i niebieski zbliżają się do siebie, kolor blednie w stronę bieli. Przy świeceniu to pożądany wygląd, ale to własność krzywej, nie fizyki.
- Nie ma automatycznej ekspozycji. Ekspozycja jest jedną liczbą z suwaka.

**Czego nie zmierzyłem.** Wybór jest oceną wyglądu sceny na jednym monitorze, na Windowsie. Nie mam porównania liczbowego krzywych na zrzutach ekranu i nie widziałem tej sceny na ekranie MacBooka. Liczby w tabeli pochodzą ze wzorów, nie z odczytu pikseli.

## 5. Kiedy wrócić do tej decyzji

- Przy bloomie (kolejna część M7): poświata dodaje światło przed krzywą i może wymagać innej ekspozycji albo innych wartości świecenia. Stało się: druga część M7 podniosła `CRYSTAL_GLOW_STRENGTH` z 2,5 do 4,0, krzywa i ekspozycja zostały. Liczby kryształu w tej notatce opisują stan z siłą 2,5, a nowe są w [`crystal-glow-raised-for-bloom.md`](crystal-glow-raised-for-bloom.md).
- Przy mgle: mgła rozjaśnia ciemne tony, czyli dokładnie ten zakres, który ACES ściska najmocniej.
- Gdyby na ekranie MacBooka noc okazała się nieczytelna: pierwszym krokiem jest ekspozycja, drugim światło otoczenia, dopiero trzecim zmiana krzywej.
- Gdyby utrata nasycenia jasnych kolorów zaczęła przeszkadzać (na przykład przy kolorowych światłach z M8): wtedy warto rozważyć mapowanie jasności zamiast kanałów.
