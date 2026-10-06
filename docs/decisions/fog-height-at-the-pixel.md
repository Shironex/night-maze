# Mgła: wysokość brana w miejscu piksela, bez całki wzdłuż promienia

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (`fogHeightFactor`, `fogAmount`), [`src/game/Fog.hpp`](../../src/game/Fog.hpp) (`FogSettings`, komentarz `KNOWN LIMIT` przy `fogAmountAt`), [`src/game/Fog.cpp`](../../src/game/Fog.cpp). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 2.20.

## 1. Kontekst

Mgła w grze ma leżeć nisko: gęsta w korytarzach labiryntu, rzadsza przy szczytach ścian, żadna na niebie. Gęstość zależy więc od wysokości i maleje wykładniczo nad wysokością bazową. Promień od oka do powierzchni przechodzi jednak przez **różne** wysokości. Ściśle biorąc, trzeba zsumować gęstość wzdłuż całej drogi i dopiero sumę wstawić do `exp`.

Gra toczy się na ziemi, z oczami na 1,7 m, w korytarzach o ścianach wysokości 3 m. Jedynym sposobem, żeby znaleźć się wysoko nad mgłą, jest tryb noclip, czyli narzędzie do debugowania.

## 2. Decyzja

Współczynnik wysokości jest liczony **raz**, dla wysokości punktu, który pokazuje piksel, i mnożony przez całą odległość: `amount = 1 - exp(-density * heightFactor(y powierzchni) * distance)`. Całki wzdłuż promienia nie ma. Ograniczenie jest zapisane w kodzie komentarzem `KNOWN LIMIT`.

## 3. Rozważane możliwości

Liczby dla wartości startowych (gęstość 0,1, baza 0,5 m, spadek 0,4), policzone z obu wzorów.

| Możliwość | Ziemia 8 m przed graczem | Szczyt ściany 8 m dalej | Ziemia z 30 m w górze | Zalety | Wady |
|---|---|---|---|---|---|
| mgła bez wysokości | 56 procent | 56 procent | 95 procent | najprostszy wzór | niebo znika w całości: punkt 100 m dalej ma prawie 100 procent mgły |
| **wysokość w pikselu (wybrana)** | 56 procent | 26 procent | 95 procent | dwa `exp` na piksel, wzór do wytłumaczenia w dwóch zdaniach, mgła leży nisko, niebo czyste | z wysoka za dużo mgły, na wysokich rzeczach oglądanych z dołu za mało |
| całka analityczna wzdłuż promienia | około 49 procent | około 32 procent | około 22 procent | poprawna dla każdej pozycji oka, nadal bez pętli: dla gęstości wykładniczej w wysokości jest gotowy wzór | dłuższy wzór z dzieleniem przez różnicę wysokości (osobny przypadek dla promienia poziomego) i podział odcinka na część nad bazą i pod nią. Trudniejszy do obrony linia po linii |
| marsz po promieniu (kilkanaście próbek na piksel) | jak całka, z dokładnością do liczby próbek | | | działa dla dowolnego rozkładu gęstości, także z szumem | kilkanaście razy droższy, pasy przy małej liczbie próbek, nic nie daje przy tak prostym rozkładzie |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Z perspektywy gracza na ziemi błąd jest mały i ma dobry kierunek: podłoga w dali tonie we mgle trochę mocniej (56 zamiast 49 procent), szczyty ścian trochę słabiej (26 zamiast 32), więc gradient "gęsto przy ziemi, rzadziej wyżej" jest nawet wyraźniejszy niż w wersji ścisłej. Komentarz w kodzie mówi, co skrót zachowuje: mgła leży nisko, a niebo zostaje czyste. Prosty wzór mieści się w dwóch krótkich funkcjach, które mają bliźniaki w C++ i testy.

**Gdzie widać błąd.** W widoku z góry. Z 30 m prosto nad podłogą kod liczy całą drogę jak w pełnej gęstości i daje 95 procent mgły, a całka dałaby około 22. W trybie noclip labirynt oglądany z wysoka prawie znika, chociaż kamera jest daleko ponad mgłą. To jest zgłoszone razem z kodem i świadomie zostawione.

**Skutki, które przyjmuję.**

- Widok z góry w noclipie nie nadaje się do oglądania labiryntu przy włączonej mgle. Kto chce zrzut ekranu z góry, odznacza `Fog`.
- Minimapa (widok z góry w osobnym framebufferze) nie może przejść przez ten sam przebieg z włączoną mgłą. **Dopisek z 2026-10-06:** minimapa jest schematem z danych labiryntu i omija przebieg składający (rysuje się po nim), więc tego ograniczenia nie ma ([`minimap-srgb-constants-after-composite.md`](minimap-srgb-constants-after-composite.md)).
- Jedna wartość `Density` nie pasuje jednocześnie do widoku z korytarza i z góry: strojenie pod jeden psuje drugi.
- `baseHeight` jest stałą w metrach świata i nie idzie za suwakiem skali wysokości terenu.

**Czego nie zmierzyłem.** Liczby z kolumn "całka" pochodzą z rachunku, nie z działającej wersji: wersji z całką nikt nie napisał. Tego, jak naprawdę wygląda widok z góry, sam nie oglądałem: znam zgłoszenie (około 95 procent z 30 m, labirynt prawie zakryty) i wzór, który daje tę samą liczbę.

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie inny widok z góry, który ma pokazywać mgłę (minimapa z 2026-10-06 jest schematem bez mgły i tego nie wymaga).
- Gdy gra dostanie miejsca, z których patrzy się w dół z wysoka (wieża, wzgórze nad labiryntem) w zwykłej rozgrywce, a nie w noclipie.
- Gdy mgła ma dostać nierówną gęstość (kłęby, szum): wtedy i tak potrzebny jest marsz po promieniu.
