# Latarka w ręce, nie w oku (planowane razem z cieniem latarki)

Data: 2026-10-05. Stan: obowiązuje jako decyzja, **kod nie jest jeszcze napisany**.
Kod, którego dotyczy: dziś [`src/game/Lighting.cpp`](../../src/game/Lighting.cpp) (`buildLightSet` stawia reflektor w oku gracza) i [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp). Dokumenty modułów: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.20, i [`../modules/game/flashlight.md`](../modules/game/flashlight.md). Stan M7: [`../guides/m7-status.md`](../guides/m7-status.md).

## 1. Kontekst

Latarka jest reflektorem, który stoi dokładnie w oku gracza i świeci dokładnie tam, gdzie gracz patrzy. Dopóki latarka nie rzucała cieni, było to najprostsze i wystarczające.

Następna część M7 (piąta) ma dać latarce własną mapę cieni z rzutem perspektywicznym. Tu pojawia się problem geometryczny: światło, które stoi **w tym samym punkcie co kamera**, rzuca cienie dokładnie **za** przedmioty, wzdłuż promieni widzenia. Każdy cień jest wtedy schowany za tym, co go rzuca. Mapa cieni byłaby rysowana i czytana poprawnie, a na ekranie nie zmieniłoby się nic.

## 2. Decyzja

Decyzja właściciela projektu z 2026-10-05: źródło światła latarki przenosi się z oka do **ręki**, czyli trochę w prawo od oka i trochę poniżej niego. Zmiana wejdzie razem z mapą cieni latarki, w piątej części M7.

Uwaga o zakresie: treścią decyzji właściciela jest jedno zdanie z części 2. Kontekst, tabela możliwości i uzasadnienie niżej to moja analiza tej decyzji, nie jej część.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **światło w ręce: w prawo i w dół od oka (wybrane)** | cienie latarki wychodzą zza przedmiotów i je widać. Zgadza się z tym, jak człowiek trzyma latarkę | plama światła nie leży dokładnie w środku ekranu z bliska. Trzeba ustalić przesunięcie i sprawdzić, czy źródło nie wchodzi w ścianę, gdy gracz stoi przy niej bokiem |
| światło zostaje w oku | nic się nie zmienia w oświetleniu | mapa cieni latarki nie daje żadnego widocznego efektu: koszt bez zysku |
| światło nad głową (czołówka) | też daje widoczne cienie | cienie padają w dół, pod przedmioty. Nie pasuje do latarki trzymanej w ręce |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Cień latarki ma być czymś, co da się pokazać na obronie przy temacie 11. Do tego źródło światła i kamera muszą stać w różnych punktach. Przesunięcie do ręki jest najmniejszą zmianą, która to zapewnia, i nie wymaga tłumaczenia.

**Co z tego wynika dla kodu (do zrobienia, nie zrobione).**

- `buildLightSet` będzie stawiać reflektor w punkcie przesuniętym względem oka, w układzie kamery (w prawo i w dół).
- Mapa cieni latarki będzie rysowana z tego samego punktu: światło i jego mapa muszą brać pozycję z jednego miejsca, tak jak dziś księżyc bierze kierunek z `moonDirection`.
- Test `the flashlight sits at the eye and points where the camera looks` w [`tests/LightingTests.cpp`](../../tests/LightingTests.cpp) sprawdza dziś, że reflektor stoi w oku: zmieni oczekiwaną pozycję.

**Czego dziś nie wiadomo.** Wielkość przesunięcia, zachowanie przy ścianie i to, czy kierunek świecenia zostaje równoległy do kierunku patrzenia, czy celuje w punkt przed graczem, nie są ustalone. Zostaną zapisane tutaj, gdy powstanie kod.

## 5. Kiedy wrócić do tej decyzji

- Przy pisaniu piątej części M7: uzupełnić tę notatkę o liczby i o to, jak wyszło.
- Gdyby przesunięte źródło dawało nieprzyjemny efekt przy ścianach (światło wchodzące w ścianę, plama uciekająca ze środka ekranu).
