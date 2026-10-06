# Podświetlenie wskazanego obiektu to pulsujące `uEmissive`, a nie kontur

Data: 2026-10-06. Stan: obowiązuje.
Kod: [`src/game/Interaction.hpp`](../../src/game/Interaction.hpp) i [`Interaction.cpp`](../../src/game/Interaction.cpp) (`highlightGlow`, `HIGHLIGHT_COLOR`, `HIGHLIGHT_MIN_GLOW`, `HIGHLIGHT_MAX_GLOW`, `HIGHLIGHT_PULSE_SPEED`), [`src/game/InteractableRenderer.cpp`](../../src/game/InteractableRenderer.cpp) (`draw`), shadery [`lit.frag`](../../assets/shaders/lit.frag), [`gouraud.frag`](../../assets/shaders/gouraud.frag), [`textured.frag`](../../assets/shaders/textured.frag), [`reflect.frag`](../../assets/shaders/reflect.frag), test `the highlight pulses between its weakest and its strongest glow` w [`tests/InteractionTests.cpp`](../../tests/InteractionTests.cpp). Dokumenty modułów: [`../modules/scene/picking.md`](../modules/scene/picking.md), [`../modules/game/interactables.md`](../modules/game/interactables.md).

## 1. Kontekst

Temat 15 w PRD wymienia podświetlenie wskazanego obiektu. Dźwignia i kartka wiszą na ścianie, mają wyglądać jak część świata, a gracz ma widzieć, na co patrzy, zanim naciśnie E. Klatka ma już program tekstur i dwa programy oświetlone (Phong i Gouraud), a każdy z nich zna uniform `uEmissive` (świecenie własne, którym świecą kryształy).

## 2. Decyzja

Wskazany obiekt, który gracz może użyć (dźwignia nie pociągnięta albo kartka), jest rysowany z `uEmissive` ustawionym na `highlightGlow(czas)`: ciepły żółty kolor `HIGHLIGHT_COLOR` (1,0; 0,8; 0,4, liniowo) razy czynnik, który pulsuje sinusem między `HIGHLIGHT_MIN_GLOW` = 0,8 a `HIGHLIGHT_MAX_GLOW` = 2,4, z szybkością `HIGHLIGHT_PULSE_SPEED` = 5 rad/s (trochę mniej niż jeden puls na sekundę). Bez nowego shadera i bez nowego przebiegu. To wybór wykonawczy.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Pulsujące `uEmissive` (wybrana)** | zero nowego kodu GPU: ten sam uniform co świecenie kryształów, ustawiany na jeden obiekt przed rysowaniem. Działa w każdym trybie oświetlenia, także Unlit. Puls odróżnia podświetlenie od stałej jasności tekstury | cały obiekt się rozświetla, a nie tylko jego obrys. Nie widać go w widokach diagnostycznych Normals i UVs (te pokazują dane, nie kolory) |
| Kontur przez bufor szablonu (drugi przebieg powiększonego obiektu) | wyraźny obrys, niezależny od oświetlenia | drugi przebieg i stan szablonu dla jednego obiektu, nowy program, kolejność względem map cieni i przebiegu końcowego |
| Kontur odwróconą bryłą (inverted hull) | prosty przebieg | wymaga drugiego modelu albo przesunięcia wzdłuż normalnych, a dźwignia i kartka są cienkie: kontur zasłaniałby model |
| Uniform z kolorem domieszki (tint) | prosty | `uTint` jest mnożnikiem tekstury: nie rozjaśnia ponad teksturę i nie odróżnia się od zwykłego koloru |

## 4. Uzasadnienie i skutki

**Dlaczego `uEmissive`.** Każdy program, który rysuje modele, już go dodaje: w `lit.frag` i `gouraud.frag` jako `surface * (diffuse + uEmissive) + specular`, w `reflect.frag` po zmieszaniu odbicia, a w `textured.frag` jako mnożnik `texel * uTint * (1 + uEmissive)`. Dlatego podświetlenie widać w Unlit, a w widokach Normals i UVs go nie ma: tam shader zwraca normalną albo współrzędne tekstury i gałąź z `uEmissive` jest pominięta. `InteractableRenderer::draw` ustawia uniform przed każdą dźwignią i kartką (wskazana dostaje podświetlenie, reszta czerń) i na końcu przywraca czerń, bo uniform trzyma wartość między wywołaniami.

**Dlaczego maksimum daleko ponad 1.** Komentarz w kodzie: w wiązce latarki powierzchnia jest już oświetlona z siłą większą niż 1, więc słabsze świecenie nie dałoby się odróżnić od tego światła. Minimum 0,8 jest dodatnie: wskazany obiekt zawsze się wyróżnia, także w najsłabszej chwili pulsu. Agent widział na zrzutach ekranu (2026-10-06), nie właściciel, że podświetlenie pulsuje i jest mocniejsze przy wyłączonej latarce. Test sprawdza tylko liczby: czynnik mieści się między 0,8 a 2,4 i oba krańce są osiągane.

**Co tracę.**

- Brak obrysu: podświetlenie to jaśniejszy kolor całego obiektu. Dla płytki dźwigni i kartki to wystarcza, dla bardziej kształtnych obiektów mogłoby nie wystarczyć.
- Podświetlenia nie ma w widokach Normals i UVs.
- Po przeróbce modelu podświetlenie czyta się na zrzutach agenta dobrze na dźwigni (ciemna płyta, bursztynowa gałka), ale przy wyłączonej latarce i bez podświetlenia płyta jest prawie czarna na ścianie. To obserwacja agenta ze zrzutów, nie właściciela.

## 5. Kiedy wrócić do tej decyzji

- Gdy wskazywane obiekty staną się większe i bardziej kształtne (drzwi, skrzynie): kontur przez szablon będzie wtedy czytelniejszy.
- Gdy `uEmissive` dostanie w tym samym obiekcie inne zadanie (świecenie własne rzeczy, które można wskazać): podświetlenie trzeba będzie wtedy dodać do świecenia, a nie ustawić.
- Gdy podświetlenie okaże się nieczytelne na ostatecznym modelu dźwigni albo w pełnym świetle wiązki.
