# Ręka latarki: w prawo poziomo, w dół prosto w dół w świecie, z granicą suwaka

Data: 2026-10-06. Stan: obowiązuje. To wybór wykonawczy, nie decyzja właściciela projektu (właściciel zdecydował o ręce "w prawo i w dół" i o ich wartościach startowych, [`flashlight-in-hand.md`](flashlight-in-hand.md)).
Kod: [`src/game/Lighting.cpp`](../../src/game/Lighting.cpp) (`flashlightPose`), [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) (`flashlightHandRight`, `flashlightHandDown`, `MAX_FLASHLIGHT_HAND_RIGHT`), [`src/debug/panels/LightsPanel.cpp`](../../src/debug/panels/LightsPanel.cpp) (suwaki), [`tests/LightingTests.cpp`](../../tests/LightingTests.cpp). Dokumenty modułów: [`../modules/game/flashlight.md`](../modules/game/flashlight.md), sekcje 2.1 i 5.5, i [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.20.9.

## 1. Kontekst

Decyzja właściciela mówi, że ręka jest "trochę w prawo i trochę poniżej oka" i podaje liczby 0,20 m i 0,25 m. Nie mówi, w którym układzie: "poniżej" można odczytać jako "w dół kamery" (prostopadle do osi widzenia) albo jako "w dół w świecie".

Ograniczenia, które przesądzają:

- ciało gracza to pudełko 0,6 m szerokości (`Player::BODY_WIDTH`), do którego nie wchodzi żadna ściana, a oko jest w jego środku. Światło wewnątrz pudełka nie może być za ścianą,
- bliska płaszczyzna mapy cieni latarki to 0,05 m (`scene::SPOT_NEAR_PLANE`): ściana bliższa światłu niż to nie trafia do mapy,
- wektor "w prawo" kamery (`scene::Camera::right`) jest zawsze poziomy, bez względu na pochylenie.

## 2. Decyzja

Ręka to oko plus `flashlightHandRight` wzdłuż wektora "w prawo" kamery minus `flashlightHandDown` wzdłuż osi `Y` świata. "W dół" znaczy więc **prosto w dół w świecie**, bez względu na to, w którą stronę patrzy kamera. Suwak "w prawo" ma górną granicę `MAX_FLASHLIGHT_HAND_RIGHT` = 0,25 m, czyli połowa ciała (0,3 m) minus bliska płaszczyzna (0,05 m). Suwak "w dół" sięga 0,5 m.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **w prawo poziomo, w dół w świecie, suwak z granicą (wybrane)** | ręka zawsze w środku pudełka ciała: poziome przesunięcie od środka to najwyżej 0,25 m, niezależnie od obrotu i nachylenia. Prosty wzór. Test pilnuje granicy | gracz patrzący w górę albo w dół nie widzi, że ręka zachowuje się jak przy ciele sztywnym, a nie jak przy kamerze. Czy to wygląda dobrze, nikt nie oglądał |
| w dół kamery (`cross(right, forward)`) | "poniżej oka" w układzie ekranu: plama ma stałe przesunięcie w obrazie | gracz patrzący w ziemię trzyma rękę 0,25 m **za** okiem, a oba przesunięcia razem (`sqrt(0,2^2 + 0,25^2)` = 0,32 m) mogą wyjść z pudełka ciała (połowa to 0,3 m) i światło wejdzie w ścianę |
| przycinanie ręki do pudełka w kodzie | działa dla dowolnych wartości ustawień | dodatkowy kod z ruchem światła skokami przy ścianie, a pudełko ciała jest w `game::Player`, nie w oświetleniu. Za dużo jak na jeden suwak |
| bez granicy suwaka | nic do uzasadniania | z `Hand right` równym 0,5 m ręka jest za ścianą, o którą gracz stoi bokiem: całe światło idzie przez ścianę |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Ręka musi być w środku pudełka ciała, żeby światło nie mogło znaleźć się za ścianą. Poziome przesunięcie w prawo to zapewnia z samej konstrukcji (wektor "w prawo" jest poziomy). Przesunięcie w dół po osi `Y` nie zmienia poziomego położenia, więc tego nie psuje. Granica 0,25 m zostawia między światłem a bokiem ciała 0,05 m, czyli dokładnie bliską płaszczyznę mapy: ściana, o którą gracz stoi bokiem (jest 0,3 m od środka), leży najwyżej na granicy bliskiej płaszczyzny mapy: nie jest ani za światłem, ani ucięta przez tę płaszczyznę (tak mówi komentarz przy stałej w `Lighting.hpp`).

**Liczby (z kodu i testów).**

| Wielkość | Wartość | Skąd |
|---|---|---|
| poziome przesunięcie ręki od środka ciała | `flashlightHandRight`, najwyżej 0,25 m | `right` jest poziomy i ma długość 1 |
| pół szerokości ciała | 0,3 m | `Player::BODY_WIDTH` / 2 |
| zapas do boku ciała przy wartości startowej | 0,1 m | 0,3 - 0,2 |
| zapas do boku ciała przy granicy suwaka | 0,05 m | 0,3 - 0,25 = bliska płaszczyzna |
| wysokość ręki nad gruntem przy wartościach startowych | 1,45 m | `Player::EYE_HEIGHT` (1,7 m) - 0,25 m |
| odległość ręki od oka przy wartościach startowych | 0,32 m | `sqrt(0,2^2 + 0,25^2)` |

Test `the hand stays inside the body of the player however the camera is turned` sprawdza dwie rzeczy: że `MAX_FLASHLIGHT_HAND_RIGHT` jest równe połowie ciała minus `SPOT_NEAR_PLANE`, i że dla siedmiu obrotów i pięciu nachyleń poziome przesunięcie ręki nie przekracza tej granicy na żadnej z osi, a przesunięcie w pionie to dokładnie `flashlightHandDown`.

**Co przyjmuję (skutki).**

- Granica chroni tylko wtedy, gdy wartości pochodzą z suwaka. `flashlightPose` niczego nie obcina: kod, który wpisze do `LightingSettings` większą liczbę, dostanie światło poza pudełkiem ciała. Podobnie `noclip` pozwala graczowi przejść przez ścianę, więc światło może stać w ścianie. Ani jednego, ani drugiego przypadku nikt nie sprawdzał na ekranie.
- Gdy gracz patrzy w górę albo w dół, ręka zostaje w tym samym miejscu względem stóp (1,45 m nad nimi, 0,2 m w prawo), a nie obraca się razem z kamerą. Jak to wygląda, wynika z kodu, a nie z obrazu.
- Granica 0,5 m dla suwaka "w dół" to wysokość ręki trzymanej przy biodrze (komentarz w panelu). Nie ma uzasadnienia liczbowego poza tym komentarzem.

## 5. Kiedy wrócić do tej decyzji

- Gdy test ręczny (`noclip` ze światłem w ścianie albo suwak na granicy przy ścianie) pokaże problem, który granica miała wykluczyć.
- Gdy zmieni się `Player::BODY_WIDTH` albo `SPOT_NEAR_PLANE`: test sprawdza ich sumę, więc zmiana jednej z nich go złamie.
- Gdy gra dostanie widoczny model ręki albo latarki: położenie ręki w świecie zostanie wtedy wyznaczone przez model.
