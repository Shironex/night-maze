# Woda kałuży stoi na najniższym gruncie pod tarczą, a nie na najwyższym

Data: 2026-10-06. Stan: zastąpiona 2026-10-06 przez [`puddles-follow-the-ground.md`](puddles-follow-the-ground.md). Stan dzisiejszy opisuje [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcje 2.11 i 2.12.
Kod, którego dotyczyła (dziś `puddleWaterLevel`, `PUDDLE_DEPTH` i `puddleModelMatrix` już nie istnieją): [`src/game/Puddles.cpp`](../../src/game/Puddles.cpp) (`puddleWaterLevel`, `puddlesOnGround`), [`src/game/Puddles.hpp`](../../src/game/Puddles.hpp) (`PUDDLE_DEPTH`, `PUDDLE_CORNERS`), [`tests/PuddleTests.cpp`](../../tests/PuddleTests.cpp). Dokument modułu: [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcja 2.12.

## Co zastąpiło tę decyzję (2026-10-06, po pierwszym obejrzeniu obrazu)

Notatka zapowiadała własny koszt: "tarcza na zboczu jest częściowo ukryta, nikt nie obejrzał, jak to wygląda". Pierwsze zrzuty ekranu (agent, wersja Release z commitu `9a33f18`) pokazały go: **cztery z trzynastu kałuż miały widoczne tylko 54 do 69 procent tarczy**, zakończone prostą cięciwą (trzy kolejne lekko obcięte, sześć całych). Właściciel zdecydował (2026-10-06), że **kałuże idą za gruntem**.

**Co jest dziś.**

- Każdy wierzchołek kałuży leży 8 mm (`PUDDLE_LIFT`) nad `Terrain::heightAt` pod nim. Kałuża to pajęczyna: środek i 6 pierścieni po 32 wierzchołki (193 wierzchołki, 352 trójkąty), zbudowana osobno dla każdej kałuży w przestrzeni świata, wszystkie w jednej siatce z macierzą modelu równą jedynce.
- Normalne zostają poziome, `(0, 1, 0)`: woda leży na nierównym gruncie, ale odbija jak stojąca.
- Nowy test mierzy, jak blisko wody podchodzi grunt między wierzchołkami (najwyżej 1,63 mm), i wymaga co najmniej 6 mm z 8.

**Co z rozumowania tej notatki nadal obowiązuje.**

- **Woda ma być pozioma w sensie normalnej**: lustro z normalną gruntu odbijałoby inny kawałek nieba w każdym miejscu. Siatka idzie za gruntem tylko położeniem, nie normalnymi.
- **Wyrównanie terenu pod kałużą jest odrzucone** z tych samych powodów (psuje `heightAt` dla gracza, trawy i ścian).
- **Kałuża zależy od terenu tylko wysokością**: wybór komórek nadal nie pyta terenu, a nowa skala wysokości nie przenosi kałuży do innej komórki.
- **Brak `glPolygonOffset`**: wierzchołki są nad gruntem, więc nie jest potrzebny.

**Co przestało być prawdą.** Wszystko, co poniższe sekcje mówią o poziomie wody "na najniższym gruncie z siedemnastu punktów plus 2 cm", o ukrywaniu tarczy przez test głębi, o liczbach maksimów różnicy pod tarczą (7 cm i 17,5 cm) i o teście `the ground of the game under a puddle`. Reszta notatki zostaje bez zmian, jako zapis tego, co wtedy rozważano.

Nowa decyzja: [`puddles-follow-the-ground.md`](puddles-follow-the-ground.md).

## 1. Kontekst

**Decyzja właściciela projektu (2026-10-06), w całości:** environment mapping jest pokazany na kryształach i w kałużach. Kryształy odbijają i załamują teksturę sześcienną nocnego nieba, a suwak daje wybór między jednym a drugim. Płaskie kałuże w niektórych komórkach korytarzy odbijają to samo niebo.

Notatka dotyczy wyboru wykonawczego: **na jakiej wysokości postawić płaską tarczę**, gdy teren pod nią jest nierówny (teren z mapy wysokości, [`gentle-terrain-under-maze.md`](gentle-terrain-under-maze.md)). Woda jest pozioma, więc tarcza nie może iść za gruntem, a lustro musi mieć normalną prosto w górę, żeby odbicie miało sens.

Liczby z testu `the ground of the game under a puddle: why the water stands on the lowest ground` (prawdziwa mapa wysokości, środek każdej komórki labiryntu startowego, największy promień 0,45 m, siedemnaście próbkowanych punktów: środek i szesnaście narożników brzegu): różnica między najwyższym a najniższym gruntem pod tarczą dochodzi do około 7 cm przy skali wysokości 1,0 i do około 17,5 cm przy 2,5 (test wymaga przedziałów od 6 do 8 i od 16 do 19 cm). Mediana, zgłoszona przez autora i nie mierzona przeze mnie ponownie, to 2,4 cm przy skali 1,0 i 6,1 cm przy 2,5.

## 2. Decyzja

Poziom wody to **najniższy grunt z siedemnastu próbkowanych punktów plus `PUDDLE_DEPTH` (2 cm)**. Tam, gdzie grunt wystaje ponad wodę, test głębi chowa tarczę.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **najniższy grunt plus 2 cm (wybrane)** | tarcza nigdzie z próbkowanych punktów nie stoi wyżej niż 2 cm nad gruntem. Brzeg wody idzie za gruntem, jak woda spływająca w dół. Żadnych dodatkowych mechanizmów | tarcza na zboczu jest **częściowo ukryta**: pokazuje mniej, niż sugeruje jej promień. Jaka część, nie policzyłem |
| najwyższy grunt pod tarczą | cała tarcza widoczna | z drugiej strony tarcza wisi w powietrzu nawet o 7 cm (skala 1,0) albo 17,5 cm (skala 2,5): lustro odklejone od ziemi |
| wysokość gruntu pod środkiem | najprostszy rachunek | zależnie od zbocza jedna strona wisi, a druga tonie. Nic nie gwarantuje, że widać większą część |
| siatka dopasowana do gruntu (wierzchołki na `heightAt` plus głębokość) | cała tarcza widoczna, bez ukrywania | woda przestaje być pozioma, jej normalna już nie jest `(0, 1, 0)` i lustro się przechyla. Kałuża staje się częścią terenu, a nie poziomym lustrem |
| wyrównanie terenu pod kałużą | pozioma tarcza i pełna widoczność | teren musiałby być zmieniany po wyborze komórek, a to psuje `heightAt` dla gracza, trawy i ścian |
| `glPolygonOffset` zamiast głębokości 2 cm | brak migotania na styku | nie rozwiązuje wiszenia ani ukrywania. Kod nie używa go także przy kałużach |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Z wszystkich możliwości, które zostawiają wodę poziomą, tylko ta nie zostawia tarczy w powietrzu. Cenę (ukrywanie) płaci zbocze, a zbocza w labiryncie są łagodne: przy skali 1,0 różnica pod największą tarczą jest rzędu kilku centymetrów.

**Skutki, które przyjmuję.**

- Przy skali 2,5 kałuża na zboczu jest w dużej części ukryta (różnica pod tarczą do 17,5 cm wobec wody 2 cm nad najniższym punktem). Nikt nie obejrzał, jak to wygląda.
- Poziom wody jest liczony z siedemnastu punktów, nie z całego brzegu. Teren jest kawałkami liniowy, więc między dwoma narożnikami grunt może być nieco niższy niż woda. Komentarz w nagłówku mówi "anywhere along its rim", co jest mocniejszym zdaniem niż to, co sprawdzono: test sprawdza punkty, które były próbkowane.
- Kałuża zależy od terenu tylko wysokością: nowa skala wysokości nie przenosi jej do innej komórki, tylko podnosi lub opuszcza jej wodę (test `another height scale moves the puddles up or down and nowhere else`).

## 5. Kiedy wrócić do tej decyzji

- Gdy po obejrzeniu okaże się, że ukryta część tarczy na zboczach jest zbyt duża: wtedy zmniejszenie promienia na zboczach albo siatka dopasowana do gruntu (z przechylonym lustrem).
- Gdy `PUDDLE_DEPTH` albo liczba próbkowanych punktów trzeba zmienić: test `the ground of the game under a puddle` ma zakresy liczb przypisane do aktualnej mapy wysokości, więc zmiana mapy też go dotyka.
