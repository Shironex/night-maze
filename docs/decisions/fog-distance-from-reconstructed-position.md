# Mgła: odległość od oka z pozycji odtworzonej z głębi, a nie głębia w metrach

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (`worldPositionFromDepth`, `fogAmount`, uniformy `uInverseViewProjection` i `uEye`), [`src/game/Fog.hpp`](../../src/game/Fog.hpp) i [`Fog.cpp`](../../src/game/Fog.cpp) (`worldPositionFromDepth`, `fogAmountAt`), [`src/game/PostProcess.hpp`](../../src/game/PostProcess.hpp) (`SceneView`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onRender`). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.18, 2.19, 4.9 i 5.12.

## 1. Kontekst

Mgła jest liczona w ostatnim przebiegu klatki z gotowego obrazu sceny. Wzór `1 - exp(-density * d)` potrzebuje `d`: ile metrów mgły leży między okiem a powierzchnią, którą pokazuje piksel. Przebieg ma do dyspozycji teksturę głębi sceny, czyli dla każdego piksela jedną nieliniową liczbę od 0 do 1.

Od pierwszej części M7 w repozytorium leży funkcja `linearDepth` w `common/depth.glsl`, która zamienia tę liczbę na metry. Dokumentacja pierwszych dwóch części zapowiadała wprost, że mgła z niej skorzysta. Do tego mgła ma leżeć nisko, więc potrzebuje też **wysokości** punktu nad ziemią.

## 2. Decyzja

Shader odtwarza dla każdego piksela pełną pozycję w świecie: współrzędna ekranu i głębia do NDC, mnożenie przez odwrotność `projection * view`, dzielenie przez `w`. Odległością we wzorze mgły jest długość odcinka od oka do tej pozycji. Odwrotność macierzy liczy C++ raz na klatkę i podaje razem z pozycją oka w strukturze `SceneView`. Funkcji `linearDepth` mgła nie używa.

## 3. Rozważane możliwości

Liczby w tabeli: punkt ściany 10 m od oka, gęstość 0,1, okno 16:9, pionowy kąt widzenia 60 stopni.

| Możliwość | Zalety | Wady |
|---|---|---|
| liczba z tekstury głębi wprost | zero rachunku | to nie metry: 2 m dają 0,951, a 30 m 0,998. Mgła prawie jednakowa na całym ekranie |
| `linearDepth`: odległość od płaszczyzny kamery | funkcja już istnieje i jest opisana. Jeden odczyt, kilka działań, żadnej macierzy | mgła zależy od obrotu kamery: ten sam punkt ma 63 procent mgły w środku ekranu i 50 procent przy krawędzi, więc mgła pływa po ścianach przy obracaniu się. Nie daje wysokości |
| `linearDepth` podzielone przez cosinus kąta piksela | poprawna odległość, nadal bez macierzy | trzeba podać shaderowi kąty widzenia i proporcje okna, a wysokości nadal nie ma: do niej potrzebny byłby jeszcze kierunek promienia w świecie |
| **pozycja w świecie z odwrotności macierzy (wybrana)** | odległość i wysokość z jednego rachunku. Trzy linie shadera. Te same trzy linie da się zapisać w C++ i sprawdzić testem na prawdziwej kamerze | jedno odwrócenie macierzy 4 x 4 na klatkę na procesorze, jedno mnożenie macierzy przez wektor na piksel, dwa nowe uniformy i nowy parametr `composite` |
| pozycja zapisana przez shadery sceny do drugiego załącznika koloru | bez odwracania czegokolwiek, pełna dokładność | drugi załącznik `GL_RGB16F` albo `GL_RGB32F` na cały ekran, zmiana wszystkich shaderów sceny i klasy `Framebuffer`, która ma dziś jeden załącznik koloru |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Mgła, która zmienia się przy obrocie w miejscu, jest błędem widocznym w pierwszej sekundzie gry w labiryncie: gracz stoi i rozgląda się częściej, niż idzie. To wyklucza głębię w metrach. Skoro i tak potrzebna jest wysokość punktu, najkrótszą drogą do obu liczb jest pełna pozycja. Koszt jest pomijalny: zgłoszona różnica liczby klatek z mgłą i winietą razem to około 1 procent, w granicach rozrzutu pomiaru.

**Dlaczego odwrotność po stronie C++.** Macierz jest ta sama dla wszystkich pikseli klatki. `onRender` ma już `projection` i `view` jako zmienne, którymi narysowano scenę, więc odwrotność powstaje z dokładnie tych samych liczb i nie może się z nimi rozjechać.

**Skutki, które przyjmuję.**

- `common/depth.glsl` zostaje z jednym użytkownikiem, podglądem głębi. Zapowiedź z wcześniejszej dokumentacji była błędna i została poprawiona.
- `PostProcess::composite` dostał czwarty parametr, a przebieg składający trzecią teksturę (głębię, jednostka 2).
- Wzór istnieje dwa razy, w GLSL i w C++, i zgodności nie pilnuje nic poza autorem. Testy sprawdzają wersję C++.
- Dokładność zależy od głębi: przy płaszczyznach 0,1 m i 100 m jeden krok 24-bitowej głębi to około 6 mm na samym końcu zakresu, czyli nic. Przy dużo dalszej płaszczyźnie dalekiej przestałoby to być prawdą.
- Dla pikseli nieba "pozycja" to punkt na płaszczyźnie dalekiej, a ta obraca się z kamerą. Skutek opisuje osobna notatka [`fog-no-special-case-for-sky.md`](fog-no-special-case-for-sky.md).

**Czego nie zmierzyłem.** Wersji z `linearDepth` nikt w tej grze nie uruchomił: jej wada to rachunek (63 wobec 50 procent), nie obserwacja. Dokument modułu ma ćwiczenie, które pozwala ją obejrzeć (sekcja 8, ćwiczenie 24).

## 5. Kiedy wrócić do tej decyzji

- Gdy dojdzie drugi załącznik koloru z innego powodu (na przykład normalne dla efektu ekranowego): wtedy pozycję albo głębię liniową można zapisać przy okazji.
- Gdy płaszczyzna daleka kamery zostanie mocno oddalona: trzeba wtedy sprawdzić pasy we mgle daleko od oka.
- Gdy mgła przestanie potrzebować wysokości (jednolita mgła): wtedy wystarczy głębia w metrach z poprawką na kąt.
