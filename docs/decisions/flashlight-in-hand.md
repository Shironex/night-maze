# Latarka w ręce, nie w oku

Data: 2026-10-05, uzupełniona 2026-10-06. Stan: obowiązuje, **kod jest napisany** (piąta część M7, commitowana razem z tą notatką). Efektu na ekranie nikt nie oglądał.
Kod, którego dotyczy: [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) i [`Lighting.cpp`](../../src/game/Lighting.cpp) (`flashlightPose`, `FlashlightPose`, nowy podpis `buildLightSet`, pola ręki w `LightingSettings`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onRender`), [`src/debug/categories/LightCategory.cpp`](../../src/debug/categories/LightCategory.cpp) (trzy suwaki). Dokumenty modułów: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.20.8 i 2.20.9, i [`../modules/game/flashlight.md`](../modules/game/flashlight.md), sekcje 2.1 i 5.5. Stan M7: [`../guides/m7-status.md`](../guides/m7-status.md). Powiązane notatki: [`flashlight-hand-straight-down.md`](flashlight-hand-straight-down.md), [`flashlight-shadow-bias-in-world-space.md`](flashlight-shadow-bias-in-world-space.md).

## 1. Kontekst

Do piątej części M7 latarka była reflektorem, który stał dokładnie w oku gracza i świecił dokładnie tam, gdzie gracz patrzył. Dopóki latarka nie rzucała cieni, było to najprostsze i wystarczające.

Piąta część M7 dała latarce własną mapę cieni z rzutem perspektywicznym. Tu pojawia się problem geometryczny: światło, które stoi **w tym samym punkcie co kamera**, rzuca cienie dokładnie **za** przedmioty, wzdłuż promieni widzenia. Każdy cień jest wtedy schowany za tym, co go rzuca. Mapa cieni byłaby rysowana i czytana poprawnie, a na ekranie nie zmieniłoby się nic.

## 2. Decyzja

Decyzje właściciela projektu, to jest cała ich treść:

- **2026-10-05:** źródło światła latarki przenosi się z oka do **ręki**, czyli trochę w prawo od oka i trochę poniżej niego. Zmiana wchodzi razem z mapą cieni latarki, w piątej części M7. Mapa cieni latarki ma rzut perspektywiczny.
- **2026-10-06:** wiązka **zbiega się**: celuje z ręki w punkt na osi widzenia przed okiem, a odległość tego punktu jest ustawieniem. Startowe przesunięcia ręki to **0,20 m w prawo i 0,25 m w dół**, oba jako suwaki.

Uwaga o zakresie: treścią decyzji właściciela są dwa punkty wyżej. Kontekst, tabela możliwości, liczby z kodu i uzasadnienie niżej to moja analiza tych decyzji, nie ich część. Wybory, które zrobiłem przy implementacji (dwa z nich mają własne notatki), są w sekcji 4.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **światło w ręce: w prawo i w dół od oka (wybrane)** | cienie latarki wychodzą zza przedmiotów i je widać. Zgadza się z tym, jak człowiek trzyma latarkę | plama światła nie leży w środku ekranu z każdej odległości. Trzeba było ustalić przesunięcie i sprawdzić, czy źródło nie wchodzi w ścianę, gdy gracz stoi przy niej bokiem |
| światło zostaje w oku | nic się nie zmienia w oświetleniu | mapa cieni latarki nie daje żadnego widocznego efektu: koszt bez zysku |
| światło nad głową (czołówka) | też daje widoczne cienie | cienie padają w dół, pod przedmioty. Nie pasuje do latarki trzymanej w ręce |

Do tego dwie możliwości, jak celować wiązką z ręki (druga z nich jest decyzją z 2026-10-06):

| Możliwość | Zalety | Wady |
|---|---|---|
| **wiązka zbiega się z osią widzenia w punkcie przed okiem (wybrane)** | plama jest w środku ekranu w jednej odległości, którą da się ustawić, i blisko środka dla ścian niewiele bliższych. Przy zerowej ręce wiązka pokrywa się z osią widzenia | w innych odległościach plama jest przesunięta: bliżej na prawo i w dół, dalej lekko w lewo i w górę. Potrzebna jest odległość jako ustawienie i przypadek, w którym cel pokrywa się z ręką |
| wiązka równoległa do osi widzenia | najprostsze: kierunek to kierunek kamery, bez dodatkowego ustawienia | plama jest przesunięta o stałe 0,2 m i 0,25 m na ścianie, a na ekranie to coraz więcej, im bliżej ściana. Z bliska plama ucieka ku dołowi i prawej krawędzi (moja analiza, nikt tego nie oglądał) |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Cień latarki ma być czymś, co da się pokazać na obronie przy temacie 11. Do tego źródło światła i kamera muszą stać w różnych punktach. Przesunięcie do ręki jest najmniejszą zmianą, która to zapewnia, i nie wymaga tłumaczenia. Zbieganie wiązki to decyzja właściciela, która trzyma plamę blisko środka ekranu tam, gdzie gracz zwykle patrzy.

**Co zrobiłem w kodzie** (sprawdzone w kodzie):

- `flashlightPose(settings, eye, forward, right)` liczy pozycję ręki (oko plus `flashlightHandRight` wzdłuż wektora "w prawo" kamery minus `flashlightHandDown` wzdłuż osi `Y` świata) i kierunek (od ręki do punktu `flashlightConvergeDistance` przed okiem). `buildLightSet` dostaje wynik gotowy.
- Mapa cieni i światło biorą pozycję i kierunek **z jednego wywołania** na klatkę: `onRender` woła `flashlightPose` raz, przed przebiegami cieni. Tak jak księżyc bierze kierunek z `moonDirection`. Test `the flashlight and its shadow map stand in the same place and look the same way` pilnuje zgodności.
- Do bloku świateł idzie pozycja ręki, ale `LightRig::upload` dostaje dalej **oko**: odbłyski liczą się od kamery.
- Test `the flashlight sits at the eye and points where the camera looks` zmienił nazwę na `the flashlight sits in the hand and is aimed at a point in front of the eye` i sprawdza nowe położenie.

**Czego nie było wiadomo, a jak wyszło** (poprzednia wersja tej notatki zostawiała te pytania otwarte):

| Pytanie | Co wyszło | Czyja to decyzja |
|---|---|---|
| wielkość przesunięcia | 0,20 m w prawo, 0,25 m w dół. Ręka jest 1,45 m nad gruntem (oko 1,7 m, `Player::EYE_HEIGHT`) i 0,32 m od oka | właściciela (2026-10-06) |
| zakresy suwaków | w prawo od 0 do 0,25 m (`MAX_FLASHLIGHT_HAND_RIGHT`), w dół od 0 do 0,5 m | moje. Granica w prawo jest uzasadniona w [`flashlight-hand-straight-down.md`](flashlight-hand-straight-down.md) |
| zachowanie przy ścianie | ciało gracza to pudełko 0,6 m szerokości (`Player::BODY_WIDTH`), do którego nie wchodzi żadna ściana, a oko jest w jego środku. Przesunięcie w prawo jest poziome, więc ręka jest 0,1 m w środku pudełka przy 0,20 m i 0,05 m przy 0,25 m, niezależnie od obrotu gracza. Test `the hand stays inside the body of the player however the camera is turned` sprawdza to dla siedmiu obrotów i pięciu nachyleń | moje |
| czy kierunek jest równoległy do patrzenia | **nie**: wiązka zbiega się z osią widzenia 4 m przed okiem. Ustawienie `Converge at` od 0,5 do 20 m | zbieganie: właściciela. Wartość 4 m (dwie komórki labiryntu) i granice suwaka: moje |
| przypadek zdegenerowany | cel i ręka pokrywają się tylko wtedy, gdy kamera patrzy prosto w dół, a ręka jest tyle poniżej oka, ile cel przed okiem. Funkcja zwraca wtedy kierunek kamery zamiast wektora zerowego | moje |

**Zakres tego, co wiadomo.** Wszystko w tabeli wynika z kodu i z testów, które sprawdzają liczby bez okna. **Nikt nie oglądał** plamy w ręce ani cienia latarki. Zgłoszone jest tylko to, co zrobiła bramka (329 przypadków i 104306 asercji) i siedmiosekundowy start programu ([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 21.1). To, czy plama leży tam, gdzie mówi podpowiedź suwaka, i czy cienie latarki są wyraźnie widoczne dzięki ręce, jest na liście testów ręcznych (sekcja 21.2).

## 5. Kiedy wrócić do tej decyzji

- Po testach ręcznych z listy 21.2: jeśli plama ucieka ze środka ekranu bardziej, niż daje się zaakceptować, albo cień latarki z ręki nie jest czytelniejszy niż z oka.
- Gdyby przesunięte źródło dawało nieprzyjemny efekt przy ścianach (światło wchodzące w ścianę, szczególnie z `noclip` albo z kodem, który ustawi pole `flashlightHandRight` poza granicą suwaka).
- Gdyby gra dostała model ręki albo latarki widoczny na ekranie: wtedy położenie ręki ma być zgodne z modelem.
- Gdyby zmienił się rozmiar ciała gracza (`Player::BODY_WIDTH`) albo bliska płaszczyzna mapy cieni latarki (`SPOT_NEAR_PLANE`): granica suwaka jest od nich wyliczona i test sprawdza sumę.
