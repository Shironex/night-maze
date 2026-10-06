# Kałuże idą za gruntem: warstwa wody 8 mm nad terenem, a nie płaska tarcza

Data: 2026-10-06. Stan: obowiązuje. Decyzja właściciela projektu (punkt 1 jego decyzji z tego dnia, w całości w sekcji 1); siatka, liczby i test to wybory wykonawcze. Zastępuje [`puddle-on-lowest-ground.md`](puddle-on-lowest-ground.md).
Kod: [`src/game/Puddles.hpp`](../../src/game/Puddles.hpp) i [`Puddles.cpp`](../../src/game/Puddles.cpp) (`buildPuddleMesh`, `puddlesOnGround`, `PUDDLE_LIFT`, `PUDDLE_RINGS`, `PUDDLE_CORNERS`), [`src/game/PuddleRenderer.cpp`](../../src/game/PuddleRenderer.cpp) (`upload`), [`tests/PuddleTests.cpp`](../../tests/PuddleTests.cpp). Dokument modułu: [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcje 2.11 i 2.12.

## 1. Kontekst

Pierwsza wersja kładła na terenie płaską tarczę na najniższym gruncie pod nią plus 2 cm ([`puddle-on-lowest-ground.md`](puddle-on-lowest-ground.md)). Cena, którą ta notatka zapowiadała ("tarcza na zboczu jest częściowo ukryta, nikt nie obejrzał, jak to wygląda"), wyszła na pierwszych zrzutach ekranu: agent uruchomił grę (wersja Release z commitu `9a33f18`, 1280 x 720) i zobaczył, że **cztery z trzynastu kałuż pokazywały tylko 54 do 69 procent tarczy**, zakończone prostą cięciwą (trzy kolejne były lekko obcięte, sześć całych). To jest "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela".

**Decyzje właściciela projektu (2026-10-06), w całości, po obejrzeniu tych ustaleń:**

1. kałuże idą za gruntem,
2. woda ma być lepiej widoczna: jaśniejszy odcień, mocniej odbijająca, miękki brzeg, więcej narożników,
3. pasek HUD stoi przy górnej krawędzi okna, dopóki panele debug są schowane.

Ta notatka dotyczy punktu 1. Punkt 2 jest w uzupełnieniu notatki [`visible-effect-over-physical-values.md`](visible-effect-over-physical-values.md), punkt 3 w notatce [`hud-at-top-edge-when-panels-hidden.md`](hud-at-top-edge-when-panels-hidden.md). Dwie dalsze zmiany z tego samego dnia (zakładka World / Reflections w dwóch kolumnach i cienka ramka wokół minimapy) są poprawkami wykonawczymi, nie decyzjami.

Ograniczenia: teren z mapy wysokości ma siatkę 0,5 m ([`gentle-terrain-under-maze.md`](gentle-terrain-under-maze.md)); wybór komórek kałuż nie pyta terenu i ma tak zostać (test `another height scale moves the puddles up or down and nowhere else`, własność "większy udział zachowuje istniejące kałuże").

## 2. Decyzja

**Każdy wierzchołek kałuży leży 8 mm nad gruntem pod nim** (`Terrain::heightAt(x, z) + PUDDLE_LIFT`). Kałuża to pajęczyna: wierzchołek w środku i 6 pierścieni po 32 wierzchołki (193 wierzchołki, 352 trójkąty), zbudowana dla każdej kałuży osobno w przestrzeni świata, wszystkie w jednej siatce z macierzą modelu równą jedynce. **Normalne zostają poziome**, `(0, 1, 0)`: kałuża leży na nierównym gruncie, ale odbija jak woda stojąca.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **warstwa idąca za gruntem, normalne w górę (wybrane)** | żaden fragment nie jest ukryty ani nie wisi. Nie zmienia ani wyboru komórek, ani terenu. Poziomy odbity promień zostaje (woda wygląda jak woda) | cienka warstwa (8 mm) jest płaska między wierzchołkami, więc grunt zbliża się do niej o do 1,6 mm i trzeba było dodać test odstępu. Siatka jest gęstsza (193 wierzchołki zamiast 17) i budowana na nowo przy każdej zmianie kałuż albo terenu. Normalna nie zgadza się z gruntem, więc na zboczu woda i ziemia są oświetlane inaczej, co jest zamierzone |
| płaska tarcza tylko tam, gdzie grunt jest płaski | zachowuje poziomą wodę bez żadnej nowej siatki | wybór komórek musiałby pytać teren: zmieniłby się test "nowa skala wysokości nie przesuwa kałuży" i własność zachowywania kałuż przy zmianie udziału. Przy skali 2,5 miejsc płaskich byłoby mało, a liczba kałuż (13 w labiryncie startowym) przestałaby być pewna. Nikt tego nie policzył |
| wyrównanie terenu pod kałużą | pozioma tarcza i pełna widoczność | teren musiałby być zmieniony po wyborze komórek, a to psuje `heightAt` dla gracza, trawy, ścian i cieni (notatka o najniższym gruncie odrzuciła to samo) |
| zostawić jak było (płaska tarcza na najniższym gruncie) | zero zmian | wprost to, co pokazały zrzuty: cztery kałuże obcięte do 54 do 69 procent |
| siatka idąca za gruntem **z normalnymi gruntu** | najprostszy rachunek | każda kałuża odbijałaby inny kawałek nieba i była oświetlana jak ziemia, a nie jak woda |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Tylko ona usuwa obcinanie bez dotykania wyboru komórek ani terenu, a poziome normalne zachowują to, co kałuża ma robić: być lustrem. Kosztem są liczba wierzchołków i jeden test więcej.

**Skutki, które przyjmuję.**

- **Odstęp od gruntu jest zmierzony, nie założony.** Test `the ground of the game never pokes through the water of a puddle` próbkuje każdy trójkąt siatki na prawdziwej mapie wysokości. Zmierzony przez autora poprawek najmniejszy odstęp: 7,56 mm (skala 1,0) i 6,89 mm (skala 2,5) dla 13 kałuż domyślnego labiryntu, 7,35 mm i 6,37 mm dla największej kałuży w dziewięciu miejscach każdej komórki. Test wymaga więcej niż 6 mm. Komentarz przy `PUDDLE_LIFT` zaokrągla odstępy gruntu w górę do 0,5 i 1,2 mm oraz 0,7 i 1,7 mm.
- **Wysokość kałuży zależy od terenu wszędzie, nie tylko w środku**, więc siatka musi być zbudowana od nowa po każdej zmianie terenu (skala wysokości) i po każdej zmianie kałuż (udział): robi to `PuddleRenderer::upload(terrain, puddles)`.
- **Bez `glPolygonOffset`.** 8 mm jest zapasem, który sam odróżnia powierzchnie w buforze głębi (komentarz kodu: bufor 24-bitowy z płaszczyznami 0,1 m i 100 m odróżnia powierzchnie odległe o 6 mm do około 30 m).
- **Co widział agent po zmianie** (autor poprawek, build jego worktree **przed scaleniem** z kodem dźwigni; "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela"): wszystkie 13 kałuż z góry całe, okrągłe i miękkie przy skali 1,0, a przy 2,5 całe i nic nie wystaje spod nich. Koordynator obejrzał zestawienie trzynastu kałuż przed i po i to potwierdza.
- **Czego nikt nie widział:** fasetek warstwy z bliska, kałuży przy skali 2,5 z wysokości chodzenia, i tego, czy właściciel uważa kształt za dobry (jego listy ręczne są otwarte).
- **Zgłoszone przez bramkę:** scalone drzewo przechodzi `make check` (467 przypadków i 158006 asercji, przed poprawkami 466 i 152264), a start Debug na 8 sekund wypisał 31 linii logu, pusty strumień błędów i żadnej linii błędu. Liczby klatek po zmianie nie mierzono.

## 5. Kiedy wrócić do tej decyzji

- Gdy z bliska widać fasetki warstwy (płaskie trójkąty do 7,5 cm) na grzbietach terenu: więcej pierścieni, kosztem wierzchołków.
- Gdy zmieni się mapa wysokości albo skala wysokości poza 2,5: test odstępu ma liczby przypisane do aktualnej mapy i może wymagać zmiany progu albo `PUDDLE_LIFT`.
- Gdy zmieni się płaszczyzna bliska kamery albo format bufora głębi: 8 mm było liczone dla 24 bitów i 0,1 m.
- Gdy po obejrzeniu przez właściciela kałuże okażą się za słabo widoczne poza wiązką latarki (agent po zmianie: z 2 m i 4 m widoczna plama, z 8 m nie): to jest sprawa wyglądu wody (uzupełnienie [`visible-effect-over-physical-values.md`](visible-effect-over-physical-values.md)), a nie wysokości.
