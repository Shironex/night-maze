# Winieta bez poprawki o proporcje okna

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (`vignetteFactor`, `SCREEN_CENTER`, `VIGNETTE_CORNER_DISTANCE`), [`src/game/Vignette.hpp`](../../src/game/Vignette.hpp) i [`Vignette.cpp`](../../src/game/Vignette.cpp), [`tests/VignetteTests.cpp`](../../tests/VignetteTests.cpp) (`the vignette is measured in texture coordinates, the same in both directions`). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 2.22.

## 1. Kontekst

Winieta przyciemnia gotowy obraz tym mocniej, im dalej piksel leży od środka ekranu. "Dalej" trzeba zmierzyć. Przebieg składający zna dla każdego piksela współrzędną tekstury `vUv`, która biegnie od 0 do 1 w poziomie i od 0 do 1 w pionie, niezależnie od tego, czy okno jest kwadratem, szerokim prostokątem czy wąskim paskiem. Okno gry startuje jako 1280 x 720 i można je dowolnie rozciągać.

Odległość policzona wprost z `vUv` traktuje połowę szerokości i połowę wysokości jako ten sam dystans 0,5, chociaż w pikselach to różne długości.

## 2. Decyzja

Odległość od środka jest liczona we współrzędnych tekstury, bez mnożenia przez proporcje okna: `length(uv - (0,5, 0,5))`. Przyciemnienie rośnie funkcją `smoothstep` od promienia z ustawień do stałej `VIGNETTE_CORNER_DISTANCE = 0,70710678`, czyli odległości do rogu. Jasny środek jest elipsą o kształcie okna.

## 3. Rozważane możliwości

Liczby dla okna 1280 x 720, promienia 0,4 i siły 0,3.

| Możliwość | Kształt jasnego środka | Środek górnej krawędzi | Środek bocznej krawędzi | Róg | Zalety | Wady |
|---|---|---|---|---|---|---|
| **współrzędne tekstury, bez poprawki (wybrana)** | elipsa 512 na 288 pikseli | 0,925 | 0,925 | 0,70 | cztery rogi i cztery krawędzie zawsze takie same, przy każdym kształcie okna. Odległość do rogu jest stałą. Wzór bez żadnego uniformu z rozmiarem okna | to nie jest koło, czyli nie to, co robi obiektyw |
| poprawka o proporcje: odległość w poziomie razy szerokość przez wysokość | koło o promieniu 288 pikseli | 0,925 | około 0,70, pełna siła już na krawędzi | 0,70 | okrągła jak w aparacie | w szerokim oknie boki mają pełne przyciemnienie na szerokim pasie, a góra i dół zostają takie jak dziś, więc obraz jest ciemny po bokach i jasny w pionie. Odległość do rogu zależy od proporcji (1,02 dla 16:9), więc stała przestaje wystarczać i potrzebny jest uniform z proporcjami |
| odległość w pikselach od środka | koło o promieniu w pikselach | zależy od rozmiaru okna | zależy od rozmiaru okna | zależy od rozmiaru okna | brak | wygląd zmienia się z rozdzielczością i na wyświetlaczu Retina: ta sama wada, którą ma szerokość poświaty bloomu |

Wiersz drugi policzyłem tak: środek bocznej krawędzi ma po poprawce odległość `0,5 * 16 / 9 = 0,889`, czyli więcej niż 0,7071, więc `smoothstep` daje tam już 1.

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Winieta ma w tej grze jedną rolę: prowadzić oko do środka, gdzie świeci latarka, bez widocznej ramki. Do tego wystarcza, żeby brzegi były ciemniejsze od środka i żeby przejście było gładkie. Wersja bez poprawki daje to samo przy każdym kształcie okna i ma jedną dodatkową zaletę przy obronie: odległość do rogu jest liczbą, którą da się policzyć na kartce (pierwiastek z 0,5) i która nie zależy od niczego. Komentarz przy `VignetteSettings` zapisuje tę własność wielkimi literami, a test trzyma ją na stałe: środek prawej krawędzi i środek górnej krawędzi dają ten sam współczynnik.

**Skutki, które przyjmuję.**

- W szerokim oknie jasny obszar jest wyraźnie szerszy niż wyższy. Przy startowej sile 0,3 różnicy między elipsą a kołem prawie nie widać. Przy sile 1 widać ją od razu.
- Promień z suwaka `Radius` nie jest liczbą pikseli, tylko ułamkiem szerokości i wysokości naraz. Podpowiedź kontrolki mówi to słowami `Not corrected for the shape of the window.`
- Promień musi zostać poniżej 0,7071, bo `smoothstep` z pierwszą krawędzią nie mniejszą od drugiej jest nieokreślony. Pilnuje tego górna granica suwaka (0,65).
- Stałe `SCREEN_CENTER` i `VIGNETTE_CORNER_DISTANCE` są zapisane dwa razy, w `Vignette.hpp` i w shaderze.

**Czego nie zmierzyłem.** Wersji z poprawką nikt nie uruchomił: liczby w tabeli to rachunek. Dokument modułu ma ćwiczenie, które pozwala ją obejrzeć (sekcja 8, ćwiczenie 28).

## 5. Kiedy wrócić do tej decyzji

- Gdy gra ma działać na bardzo szerokich ekranach (21:9 i więcej): elipsa robi się wtedy tak płaska, że boki są prawie nieprzyciemnione na dużej części szerokości.
- Gdy winieta ma udawać konkretny obiektyw (na przykład w trybie zdjęć): wtedy koło i poprawka o proporcje.
- Gdy winieta dostanie rolę w rozgrywce (zawężanie pola widzenia przy słabej baterii): wtedy kształt trzeba zaprojektować od nowa.
