# Mapa cieni: macierz i liczby jako zwykłe uniformy, nie w bloku świateł

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/common/shadows.glsl`](../../assets/shaders/common/shadows.glsl) (siedem uniformów `uMoonShadow...`), [`src/game/ShaderUniforms.hpp`](../../src/game/ShaderUniforms.hpp) (`ShadowUniformNames`, `MOON_SHADOW_UNIFORMS`, `MOON_SHADOW_TEXTURE_UNIT`), [`src/game/ShadowMap.cpp`](../../src/game/ShadowMap.cpp) (`setShadowUniforms`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawLitMaze`, `drawGrass`). Dokumenty modułów: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 4.2, 4.4 i 5.6, oraz [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md).

## 1. Kontekst

Wszystkie dane świateł trafiają do shaderów jednym blokiem uniformów, `LightBlock`: jeden bufor, wysyłany raz na klatkę, widoczny w każdym programie, który go deklaruje. Mapa cieni księżyca dokłada nowe dane tego samego światła: teksturę, macierz światła, dwie części biasu, promień PCF, siłę cienia i przełącznik. Naturalne pytanie: dlaczego nie dopisać ich do bloku?

Ograniczenie języka: sampler jest w GLSL typem nieprzezroczystym i **nie może być polem bloku uniformów**. Tekstura mapy musi więc być zwykłym uniformem w każdym programie, niezależnie od tego, gdzie pójdzie reszta.

## 2. Decyzja

Wszystkie siedem danych jednej mapy to zwykłe uniformy, zadeklarowane obok siebie w `common/shadows.glsl` i ustawiane jedną funkcją, `setShadowUniforms`, dla każdego programu, który ten plik włącza (`lit`, `gouraud`, `grass`). Blok `LightBlock` nie zmienia się.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **wszystko jako zwykłe uniformy, obok samplera (wybrane)** | dane jednej mapy leżą w jednym miejscu w GLSL i w jednej funkcji w C++. Układ std140 bloku świateł, jego rozmiar i testy zostają nietknięte. Nazwy są parametrem (`ShadowUniformNames`), więc druga mapa to druga stała | siedem uniformów ustawianych co klatkę w trzech programach, czyli 21 wywołań zamiast jednego wysłania bufora |
| macierz i liczby w `LightBlock`, sampler osobno | macierz wysyłana raz dla wszystkich programów | dane jednej mapy rozdarte na dwa mechanizmy. Zmiana układu bloku: nowe przesunięcia std140, zmiana struktury w C++ i testów układu. Sampler i tak trzeba ustawić w każdym programie |
| osobny blok uniformów dla cieni | czyste rozdzielenie, wysyłka raz na klatkę | drugi bufor, drugi punkt wiązania, drugi zestaw reguł std140 dla siedmiu liczb, z czego jedna i tak zostaje poza blokiem |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Skoro sampler musi zostać poza blokiem, pytanie brzmi tylko, gdzie mają leżeć liczby, które bez tej tekstury nic nie znaczą. Trzymanie ich obok niej daje jedną regułę: "mapa cieni to te uniformy". Koszt 21 wywołań `glUniform` na klatkę jest pomijalny obok kilkuset wywołań, które klatka robi dla macierzy modeli.

**Skutek, o którym trzeba pamiętać.** Zwykłe uniformy należą do programu, więc po przeładowaniu shaderów wracają do zera. Dla samplera cieni zero znaczy jednostkę 0, tę samą co sampler tekstury koloru, a OpenGL nie rysuje programem z dwoma samplerami różnych typów na jednej jednostce. Dlatego `setShadowUniforms` jest wołane w każdej klatce, **także przy wyłączonych cieniach**. Blok uniformów tego problemu by nie miał dla liczb, ale dla samplera i tak by został.

**Drugi skutek.** Każdy nowy program, który włączy `common/shadows.glsl`, musi dostać wywołanie `setShadowUniforms`. Zapomniane wywołanie nie daje błędu kompilacji: program rysuje bez cieni albo wcale (konflikt jednostek).

**Czego nie zmierzyłem.** Kosztu 21 wywołań na klatkę nikt nie mierzył osobno.

## 5. Kiedy wrócić do tej decyzji

- Gdy map cieni będzie kilka i programów z cieniem kilka: liczba wywołań rośnie jak ich iloczyn.
- Gdy projekt przejdzie na wersję OpenGL z jawnym `layout(binding = ...)` dla samplerów (4.2): znika wtedy problem jednostki 0 po przeładowaniu.
