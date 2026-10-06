# Promień wskazywania zaczyna się w oku, a nie na bliskiej płaszczyźnie

Data: 2026-10-06. Stan: obowiązuje.
Kod: [`src/game/Interaction.cpp`](../../src/game/Interaction.cpp) (`rayFromEye`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`pickForFrame`), [`src/scene/Raycast.cpp`](../../src/scene/Raycast.cpp) (`screenPointRay`), [`src/game/Interactables.hpp`](../../src/game/Interactables.hpp) (`INTERACTION_REACH`), test `the picking ray starts in the eye and keeps its direction` w [`tests/InteractionTests.cpp`](../../tests/InteractionTests.cpp). Dokument modułu: [`../modules/scene/picking.md`](../modules/scene/picking.md).

## 1. Kontekst

`scene::screenPointRay` odwraca macierz `projection * view` i zwraca promień, który zaczyna się na **bliskiej płaszczyźnie** przycinania: 0,1 m przed okiem dla punktu w środku obrazu i około 0,155 m dla punktu przy rogu (liczone dla kąta widzenia 60 stopni i proporcji 16 do 9, rachunek w [`../modules/scene/picking.md`](../modules/scene/picking.md)). Zasięg ręki gracza, `INTERACTION_REACH` = 2,5 m, ma być mierzony **od oka**: tak go rozumie gracz i tak jest opisany w kodzie.

## 2. Decyzja

Początek promienia jest przenoszony z bliskiej płaszczyzny do oka (`rayFromEye`: ten sam kierunek, `origin = eye`). To wybór wykonawczy. Funkcja `screenPointRay` zostaje bez zmian.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Przeniesienie początku do oka, kierunek bez zmian (wybrana)** | zasięg 2,5 m jest od oka, jak w opisie. Jedna mała funkcja i jeden test. `screenPointRay` zostaje ogólna i przetestowana osobno | wołający musi znać oko (w `onRender` to oko zmieszane z klatki, to samo, z którego liczy się widok) |
| Zostawić początek na płaszczyźnie i dodać 0,1 m do zasięgu | zero zmian w kodzie promienia | błąd zależy od punktu obrazu (od 0,1 do około 0,155 m), więc stała poprawka nie jest dokładna. Zasięg przestaje być zasięgiem ręki |
| Użyć tylko kierunku patrzenia kamery (`forward`) | prosto | nie działa dla kursora: z wolnym kursorem promień idzie przez dowolny punkt obrazu, nie tylko przez środek |

## 4. Uzasadnienie i skutki

**Dlaczego ten sam piksel.** W rzutowaniu perspektywicznym każdy promień przez punkt obrazu jest prostą wychodzącą z oka. Punkt na bliskiej płaszczyźnie i oko leżą na tej samej prostej, więc promień z oka w tym samym kierunku przechodzi przez ten sam punkt płaszczyzny i pokazuje ten sam piksel. Test sprawdza to wprost: oko plus kierunek razy odległość do płaszczyzny daje początek promienia ekranowego.

**Co dostaję.** Zasięg jest zgodny z liczbą, a pudełko dokładnie w zasięgu liczy się, jak w `nearestHit`. Przesłanianie ścianą (ta sama lista przeszkód, zasięg równy odległości trafionej rzeczy) też liczy się od oka.

**Co tracę.** Punkt obrazu i jego rozmiar pochodzą z **okna** (kursor GLFW jest w jednostkach okna), a proporcje macierzy rzutowania z **bufora ramki**. Są ze sobą zgodne tylko dlatego, że stosunek szerokości do wysokości jest w obu taki sam. Na ekranie HiDPI (Retina) liczby są różne, ale ich stosunek nie. To ryzyko jest na otwartej liście dla macOS ([`../guides/build-macos.md`](../guides/build-macos.md)), gdzie kod nie był budowany ani uruchamiany.

## 5. Kiedy wrócić do tej decyzji

- Gdy kamera dostanie pozycję inną niż oko gracza (na przykład widok z trzeciej osoby): "oko" musi wtedy znaczyć punkt, od którego liczy się zasięg.
- Gdy obraz sceny będzie zajmował tylko część okna (dwa widoki, ramka): rozmiar okna i bufora przestanie mieć ten sam stosunek.
