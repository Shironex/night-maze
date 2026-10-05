# Światła punktowe: w ślepych zaułkach labiryntu, zanim powstaną kryształy

Data: 2026-10-05. Stan: obowiązuje (do zastąpienia w M5, gdy powstaną kryształy).
Kod: [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp), [`Lighting.cpp`](../../src/game/Lighting.cpp) (`isDeadEnd`, `deadEndLightPositions`), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) (`buildMazeWorld`), [`src/game/LightRig.cpp`](../../src/game/LightRig.cpp) (`drawMarkers`). Dokument modułu: [`../modules/game/flashlight.md`](../modules/game/flashlight.md), sekcje 2.3, 2.4 i 5.4.

## 1. Kontekst

Temat 6 wykładu to światło kierunkowe **i punktowe**. PRD przewiduje, że światłami punktowymi gry będą kryształy, które gracz zbiera: "do 16 punktowych", z tłumieniem dobranym tak, żeby kryształ oświetlał około 1,5 komórki labiryntu. Kryształy to jednak rozgrywka: model, rozmieszczenie z ziarna, kolizja kulą, zbieranie, licznik. To zakres M5, a M5 nie jest rozpoczęte.

Oświetlenie (M4) musi mieć światła punktowe już teraz, z trzech powodów:

- bez nich temat 6 jest zrealizowany w połowie (jest tylko księżyc i latarka),
- pętli po światłach w shaderze, tłumienia z promienia i tablicy w bloku uniformów nie da się pokazać ani sprawdzić na obrazie bez choć jednego światła punktowego,
- różnicę między Phongiem a Blinnem-Phongiem najlepiej widać przy świetle, które **nie** jest w oku kamery, a latarka jest dokładnie w oku.

Trzeba było zdecydować, gdzie postawić światła punktowe, dopóki nie ma kryształów.

## 2. Decyzja

Światło punktowe wisi 1,4 m nad środkiem **każdego ślepego zaułka** labiryntu (komórki z dokładnie trzema ścianami), z wyjątkiem komórki startowej. Kolejność jest stała (wierszami), a przy więcej niż 16 zaułkach zostaje 16 wybranych równomiernie z listy. Wszystkie światła mają wspólny kolor, intensywność i promień (turkus, 3 m). Każde jest oznaczone małą kostką rysowaną bez oświetlenia. Gdy w M5 powstaną kryształy, to one staną się źródłem pozycji świateł.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Ślepe zaułki, wybór bez losowania (wybrana)** | pozycje wynikają wprost z labiryntu, bez nowych danych i bez ziarna. Zaułki są rozrzucone po całej planszy. Ten sam labirynt ma te same światła na każdym systemie. Da się przypiąć testem (labirynt wzorcowy: dwa światła, labirynt startowy: 11). Zaułek to naturalne miejsce na coś do znalezienia | liczba świateł zależy od labiryntu i nie da się jej ustawić. Długie korytarze bez odgałęzień zostają ciemne. Reguła nie jest regułą kryształów z PRD |
| Kryształy od razu, bez zbierania | zgodne z PRD | wciąga do M4 model kryształu, rozmieszczenie z ziarna i panel z liczbą kryształów, czyli początek M5. Bez zbierania i tak byłyby to tylko światła z ładniejszym znacznikiem |
| Losowe komórki z ziarna labiryntu | liczba świateł do ustawienia | dodatkowe losowania. Jeśli idą z generatora labiryntu, zmieniają wszystkie labirynty wzorcowe z testów ([`deterministic-random.md`](deterministic-random.md)). Jeśli z osobnego generatora, trzeba go zasiać i przetestować. Światła mogą wypaść obok siebie |
| Stała siatka: światło co trzecią komórkę | prosty kod, równe oświetlenie | nie ma związku z labiryntem: światło w środku korytarza niczego nie mówi. Równe oświetlenie odbiera sens latarce |
| Jedno albo dwa światła wpisane ręcznie przy starcie | najmniej kodu | nie przenoszą się przy nowym labiryncie (mogą wypaść w ścianie), nie pokazują limitu 16 ani pętli po wielu światłach |
| Światła edytowane w panelu (dodaj, usuń, przesuń) | pełny "edytor świateł" z PRD | dużo kodu panelu dla czegoś, co w M5 zostanie zastąpione. Stan panelu ginąłby przy każdej regeneracji |

## 4. Uzasadnienie i skutki

**Dlaczego zaułki.** Labirynt doskonały ma zaułków dużo i są wszędzie, więc światła rozkładają się po planszy same. Zaułek jest też miejscem z własną dramaturgią: gracz widzi z korytarza poświatę za rogiem, idzie do niej i trafia na ścianę. To samo miejsce nadaje się potem na kryształ.

**Dlaczego bez losowania.** Pozycje świateł są częścią `MazeWorld`, tak jak macierze ścian. Skoro wynikają tylko z siatki, ich poprawność sprawdza test na labiryncie wzorcowym, a generator labiryntu nie dostał ani jednego dodatkowego losowania. Wszystkie labirynty wzorcowe z M2 + M3 zostały bez zmian.

**Dlaczego bez komórki startowej.** Start labiryntu wygenerowanego jest zaułkiem: generator zaczyna w narożnej komórce `(0, 0)`, a przeszukiwanie w głąb odwiedza przez pierwszego sąsiada całą resztę siatki, zanim wróci do startu, więc start dostaje jedno przejście (wyjątkiem jest labirynt 1 na 1, który przejść nie ma). W labiryncie wzorcowym 4 na 4 potwierdza to test. Światło 30 cm pod oczami gracza zalałoby pierwszą klatkę i zakryło latarkę, którą gracz ma na starcie zobaczyć.

**Dlaczego wybór równomierny przy nadmiarze.** Tablica w shaderze ma 16 miejsc (tyle przewiduje PRD). Pierwsze 16 zaułków z listy uporządkowanej wierszami leżałoby w górnej części dużego labiryntu. Wzór `i * liczba / 16` bierze co któryś zaułek z całej listy, bez losowania i bez powtórzeń.

**Dlaczego parametry kryształów.** Promień 3 m to półtorej komórki, czyli zasięg, który PRD podaje dla kryształów. Kolor turkusowy jest moim wyborem na kolor kryształów (tak mówi komentarz przy `pointColor` w `Lighting.hpp`), PRD koloru nie podaje. Scena ma więc wyglądać podobnie jak z kryształami, a zmiana w M5 dotyczy głównie tego, skąd biorą się pozycje.

**Co przez to tracę.**

- Liczby świateł nie da się ustawić: labirynt startowy ma ich 11, inne ziarna inaczej.
- Światła są częścią `MazeWorld`, czyli danych poziomu, a kryształy będą obiektami rozgrywki, które znikają po zebraniu. W M5 pole `pointLightPositions` albo zniknie, albo zmieni znaczenie.
- Zaułków ponad limit 16 nic nie oświetla i nic nie oznacza. Panel Lights pokazuje wtedy `In this maze: 16 (at most 16)`.
- Kostka znacznika to tymczasowa grafika: nie jest modelem kryształu i nie ma kolizji.

**Co zostaje po zmianie w M5.** Cała reszta: struktura `scene::PointLight`, tablica w `LightSet`, blok uniformów, pętla w `common/lighting.glsl`, `buildLightSet` (przyjmuje listę pozycji i nie wie, skąd pochodzi), ustawienia w panelu Lights. Wymienia się jedna funkcja dostarczająca pozycje.

## 5. Kiedy wrócić do tej decyzji

- W M5, gdy powstaną kryształy: światła mają iść za kryształami i gasnąć po zebraniu. Funkcje `isDeadEnd` i `deadEndLightPositions` mogą wtedy zostać jako jedna z reguł rozmieszczania kryształów albo zniknąć.
- Gdy labirynty używane na pokazie będą miały dużo więcej niż 16 zaułków i ciemne zaułki zaczną przeszkadzać: wtedy wybór 16 świateł najbliższych graczowi w każdej klatce byłby lepszy niż stały wybór na labirynt.
- Gdy dojdą cienie (M7): światło w zaułku przestanie świecić przez ściany i może się okazać, że korytarze są za ciemne. Wtedy trzeba będzie wrócić do rozmieszczenia albo do promienia.
