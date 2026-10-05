# Mgła: niebo bez osobnego przypadku

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (blok mgły w `main` i komentarz o niebie), [`src/game/Fog.hpp`](../../src/game/Fog.hpp) (`worldPositionFromDepth`, `FogSettings::heightFalloff`), [`tests/FogTests.cpp`](../../tests/FogTests.cpp) (`the default fog leaves the moon clear and hides the sky below the horizon`). Dokumenty modułów: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 2.21, [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md).

## 1. Kontekst

Niebo jest rysowane jako ostatnie wywołanie sceny, na głębi dokładnie 1. Mgła czyta głębię każdego piksela i odtwarza z niej punkt w świecie. Dla piksela nieba tym punktem jest miejsce na płaszczyźnie dalekiej kamery, 100 m w kierunku piksela. Nieba nie ma "w żadnej odległości", więc każda liczba, którą dostanie wzór mgły, jest umowna. Trzeba było zdecydować, co z takimi pikselami zrobić.

Oczekiwany obraz: horyzont ginie we mgle, a księżyc i gwiazdy zostają czyste. Poświata księżyca i sama tarcza są ważnym elementem nocy.

## 2. Decyzja

Shader nie rozróżnia nieba. Piksel o głębi 1 przechodzi przez te same cztery linie co ściana: pozycja z głębi, współczynnik wysokości, ilość mgły, `mix`. O tym, ile mgły dostaje niebo, decyduje wyłącznie wysokość punktu na płaszczyźnie dalekiej.

## 3. Rozważane możliwości

Liczby dla wartości startowych, w środku ekranu, policzone ze wzoru.

| Możliwość | Horyzont | 3 stopnie nad nim | Księżyc (50 stopni) | Zalety | Wady |
|---|---|---|---|---|---|
| niebo pomijane: `if (depth < 1.0)` | 0 procent | 0 procent | 0 procent | niebo zawsze takie, jak je namalowano | ostra linia między zamglonymi wzgórzami a czystym niebem tuż nad nimi. Krawędź, na której kończy się siatka terenu, zostaje widoczna |
| niebo zawsze w pełnej mgle (odległość bez wysokości) | 100 procent | 100 procent | 100 procent | horyzont znika | znika też księżyc i gwiazdy |
| **bez osobnego przypadku (wybrana)** | 99,8 procent | 53 procent | 0 procent | zamglony horyzont, miękkie przejście na kilku stopniach, czysty księżyc. Żadnego `if`, żadnego progu głębi. Mgła pod horyzontem zakrywa koniec terenu | ilość mgły na niebie zależy od umownej odległości 100 m, czyli od `farPlane`, i od kąta piksela względem osi kamery (niżej) |
| osobny wzór dla nieba: mgła z samego kąta wzniesienia promienia | niezależny od `farPlane` i od obrotu kamery | | | poprawny i stabilny | druga gałąź, drugi wzór, kierunek promienia w świecie jako dodatkowy rachunek. Trzeba dopasować ręcznie, żeby nie było szwu między wzgórzem a niebem |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Wzór z wysokością sam daje właściwy obraz: daleko i nisko znaczy mgła, daleko i wysoko znaczy czysto. Każda gałąź dla nieba tworzy granicę między pikselami wzgórza a pikselami nieba, a tę granicę trzeba by potem ukrywać. Bez gałęzi takiej granicy nie ma: ostatni piksel wzgórza i pierwszy piksel nieba nad nim dostają podobną ilość mgły, bo oba leżą daleko i na podobnej wysokości. Test sprawdza na wartościach startowych oba końce: kierunek księżyca ma poniżej 1 procenta mgły, kierunek prosto w dół ponad 99.

**Skutek, który trzeba znać.** Płaszczyzna daleka jest płaska i obraca się z kamerą. Ten sam kierunek na niebie trafia w nią 100 m od oka w środku ekranu, 143 m przy lewej albo prawej krawędzi i 155 m w rogu (okno 16:9, kąt widzenia 60 stopni). Dalszy punkt na tym samym promieniu leży wyżej i dostaje mniej mgły: 3 stopnie nad horyzontem to 53 procent w środku ekranu, 36 przy krawędzi i 31 w rogu. Zdanie "mgła nie zależy od obrotu kamery" jest więc prawdą dla geometrii, a dla wąskiego pasa nieba tuż nad horyzontem nie. Pas ma kilka stopni: od 10 stopni w górę mgły jest poniżej 1 procenta wszędzie.

**Pozostałe skutki, które przyjmuję.**

- Suwak `Height falloff` ustawiony na 0 zamalowuje całe niebo kolorem mgły. To oczekiwane zachowanie wzoru i dobry pokaz tego, że nieba nie chroni żaden warunek.
- Zmiana `farPlane` kamery zmienia wygląd mgły na niebie, choć nie zmienia jej na geometrii.
- Poświata księżyca jest dodawana po mgle, więc nie słabnie ([`bloom-from-unfogged-scene.md`](bloom-from-unfogged-scene.md)).
- Przy wyłączonym polu `Skybox` piksele tła też mają głębię 1 (tyle zostawia `glClear`) i dostają tę samą mgłę na kolorze tła.

**Czego nie zmierzyłem.** Wszystkie liczby w tej notatce pochodzą ze wzoru. Tego, czy pas nieba nad horyzontem widocznie zmienia się przy obrocie kamery, nikt jeszcze nie sprawdził na ekranie: wzgórza wokół labiryntu zasłaniają dużą jego część. Jest to punkt na liście testu ręcznego w [`../guides/build-windows.md`](../guides/build-windows.md), sekcja 19.

## 5. Kiedy wrócić do tej decyzji

- Gdy test ręczny pokaże, że pas nad horyzontem widocznie pływa przy obrocie: wtedy osobny wzór z kąta wzniesienia albo odległość sfery zamiast płaszczyzny (punkt 100 m od oka wzdłuż promienia zamiast punktu na płaszczyźnie dalekiej).
- Gdy zmieni się `farPlane` kamery.
- Gdy niebo dostanie chmury albo własną mgłę namalowaną w obrazach: wtedy dwie mgły zaczną się dublować.
