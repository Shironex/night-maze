# Bias cienia: w metrach, liczony w shaderze, bez `glPolygonOffset` i bez zmiany odrzucania ścian

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/common/shadows.glsl`](../../assets/shaders/common/shadows.glsl) (`slopeScaledBias`, `moonShadow`), [`src/game/Shadows.hpp`](../../src/game/Shadows.hpp) (`ShadowSettings::constantBias`, `slopeBias`), [`src/game/Shadows.cpp`](../../src/game/Shadows.cpp) (`shadowBias`, `biasInDepthUnits`), [`src/game/ShadowMap.cpp`](../../src/game/ShadowMap.cpp) (`setShadowUniforms`), [`assets/shaders/common/lighting.glsl`](../../assets/shaders/common/lighting.glsl) (`moonFacing`). Dokument modułu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.10 do 2.12.

## 1. Kontekst

Mapa cieni bez poprawki daje shadow acne: oświetlona powierzchnia zacienia samą siebie, bo teksel mapy przechowuje jedną głębię dla kawałka pochyłej powierzchni. Każda implementacja potrzebuje biasu, czyli zapasu, o który fragment jest przysuwany do światła przed porównaniem. Za mały zostawia acne, za duży odkleja cień od rzucającego (peter panning). Bias można wprowadzić w kilku miejscach potoku i w różnych jednostkach.

Dodatkowe ograniczenie tej gry: głębia pudełka światła zmienia się z kątem księżyca (od około 7,5 m przy księżycu prosto nad głową do około 65 m przy księżycu nisko), a suwak ma być czytelny dla kogoś, kto pierwszy raz widzi panel.

## 2. Decyzja

Bias jest liczony **tylko w shaderze sceny**, wzorem `constantBias + slopeBias * (1 - cos kąta między normalną a kierunkiem do światła)`, a oba ustawienia są w **metrach** (startowo 0,02 i 0,12). C++ przelicza je na jednostki głębi dzieleniem przez głębię pudełka światła. Przebieg głębi rysuje geometrię bez żadnej poprawki: nie ma `glPolygonOffset`, a odrzucanie ścian nie jest włączane ani zmieniane.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **bias w shaderze, w metrach, stały plus zależny od pochylenia (wybrane)** | jedno miejsce w kodzie. Jednostka, którą da się porównać z grubością ściany. To samo ustawienie działa przy każdym kącie księżyca. Wzór ma bliźniaka w C++ i testy | część zależna od pochylenia rośnie liniowo z `1 - cos`, a błąd rośnie jak tangens: przy świetle prawie stycznym bias jest za mały. Bias nie zna rozmiaru teksela ani promienia PCF |
| stała liczba w jednostkach głębi (typowe 0,005) | najkrótszy kod | znaczy co innego przy każdym pudełku: tu 0,005 to od 4 cm do 33 cm zależnie od kąta księżyca |
| `glPolygonOffset(factor, units)` przy rysowaniu mapy | karta zna prawdziwe nachylenie głębi w trójkącie, więc poprawka idzie dokładnie za pochyleniem | jednostka `units` zależy od implementacji i formatu głębi. Dwie liczby bez wymiaru zamiast metrów. Stan OpenGL, który trzeba włączać i wyłączać |
| rysowanie do mapy tylnych ścian (`glCullFace(GL_FRONT)`) | acne znika z oświetlonych ścian bez biasu | wymaga zamkniętych brył i włączonego odrzucania ścian, którego gra nigdzie nie używa. Teren jest pojedynczą powierzchnią bez tylnej strony. Błąd przenosi się na styk ściany z ziemią |
| bias wzdłuż normalnej (przesunięcie punktu przed odczytem) | dobrze radzi sobie ze światłem stycznym | przesuwa także miejsce odczytu w mapie, więc cień lekko się kurczy. Druga poprawka do strojenia |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Na obronie bias trzeba umieć pokazać: suwak na 0 daje acne, suwak na 0,5 m daje peter panning, a pomiędzy jest liczba, którą da się uzasadnić ("teksel ma 3 cm, ściana 20 cm"). Metry i jeden wzór w shaderze na to pozwalają. Przeliczenie na jednostki głębi jest jednym dzieleniem, bo głębia rzutu ortograficznego jest liniowa.

**Normalna.** Do biasu idzie normalna modelu, a nie normalna z mapy normalnych: bias należy do trójkąta narysowanego do mapy cieni.

**Skutki, które przyjmuję.**

- Bias nie rośnie z rozmiarem teksela ani z jądrem PCF. Z rachunku wynika, że jądra `5 x 5` i `7 x 7` przy mapie 2048 oraz każde jądro przy mapie 1024 sięgają na gruncie dalej, niż pokrywa bias startowy (4,8 cm), więc oświetlony grunt może być wtedy lekko przyciemniony. To jest **policzone, nie zaobserwowane**, i stoi na liście testów ręcznych. Jeśli się potwierdzi, poprawką jest pomnożenie biasu przez rozmiar teksela i promień jądra, a nie zmiana tej decyzji.
- Zbocza odwrócone od księżyca prawie stycznie mogą pokazywać acne mimo biasu. Ich udział światła księżyca jest wtedy bliski zeru, więc błąd jest słabo widoczny.
- Wzór jest w dwóch miejscach (GLSL i C++), a testy pilnują tylko strony C++.
- Przeliczenie przez głębię pudełka nie zadziała dla rzutu perspektywicznego (mapa latarki): tam głębia nie jest liniowa. **Dopisek z 2026-10-06 (M7, część piąta):** mapa latarki ma osobny sposób: bias zostaje w metrach i przesuwa punkt w przestrzeni świata ([`flashlight-shadow-bias-in-world-space.md`](flashlight-shadow-bias-in-world-space.md)). Decyzja z tej notatki dotyczy dalej księżyca.

**Co jest zgłoszone z działającej gry (Windows, 2026-10-05).** Przy samych suwakach biasu na 0 acne wygląda jak ogólne przyciemnienie, a wyraźne prążki pojawiają się po wyłączeniu także PCF i filtra sprzętowego. Przy biasie 0,5 m widać światło przeciekające na ścianach.

## 5. Kiedy wrócić do tej decyzji

- Gdy test ręczny potwierdzi przyciemnienie przy dużym jądrze albo małej mapie.
- Gdy powstanie mapa cieni latarki: potrzebne będzie inne przeliczenie na jednostki głębi. **Powstała (2026-10-06):** `game::biasForShader` wybiera przeliczenie według rodzaju rzutu.
- Gdy gra dostanie cienkie rzucające (cieńsze niż bias startowy 0,14 m w najgorszym przypadku).
