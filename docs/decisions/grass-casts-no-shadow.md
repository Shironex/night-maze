# Trawa przyjmuje cień księżyca, ale go nie rzuca

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawShadowCasters` bez trawy, `drawGrass` z `setShadowUniforms`), [`assets/shaders/grass.frag`](../../assets/shaders/grass.frag) (`moonShadow`), [`assets/shaders/grass.geom`](../../assets/shaders/grass.geom) (`ROOT_HALF_WIDTH`). Dokumenty modułów: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.16, i [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md).

## 1. Kontekst

Trawa to 1843 kępki w labiryncie startowym, rosnące wzdłuż ścian. Źdźbła powstają w shaderze geometrii z pojedynczych punktów i kołyszą się na wietrze. Źdźbło ma u nasady 4 cm szerokości i zwęża się do zera na czubku. Teksel mapy cieni księżyca ma przy rozdzielczości 2048 około 3,2 cm.

Są dwa osobne pytania: czy trawa ma **leżeć** w cieniu ścian i czy ma sama **rzucać** cień.

## 2. Decyzja

Trawa **przyjmuje** cień: `grass.frag` odejmuje udział księżyca tak samo jak ściany i ziemia. Trawa **nie rzuca** cienia: przebieg głębi jej nie rysuje.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **przyjmuje, nie rzuca (wybrane)** | kępki pod ścianą są ciemne jak ziemia pod nimi. Żadnego nowego programu. Żadnego migotania | trawa na otwartym gruncie nie ma cienia pod sobą |
| ani nie przyjmuje, ani nie rzuca | zero zmian w shaderach trawy | jasne kępki świecące na zacienionej ziemi: najbardziej widoczny błąd, bo trawa rośnie właśnie przy ścianach |
| przyjmuje i rzuca | pełna spójność | źdźbło o szerokości jednego teksela daje cień z pojedynczych tekseli, który miga z wiatrem. Cień pada na ziemię, którą sama kępka zasłania. Wymaga osobnego programu głębi z shaderem geometrii i z uniformami wiatru |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Przyjmowanie cienia kosztuje jedną linię `#include` i trzy linie w shaderze fragmentów, a naprawia błąd, który byłoby widać w każdym korytarzu. Rzucanie cienia kosztowałoby nowy program (program `shadow_depth` nie ma etapu geometrii, a punkty trawy bez niego nie mają kształtu) i dałoby wynik gorszy niż brak cienia: przy geometrii drobniejszej niż teksel mapa cieni nie odtwarza kształtu, tylko szum.

**Skutki, które przyjmuję.**

- Trawa nie zacienia ani ziemi, ani samej siebie.
- Kępka jest sprawdzana w mapie cieni dla pozycji każdego fragmentu źdźbła, a bias liczy się z normalnej podłoża `(0, 1, 0)`, tej samej, którą trawa jest oświetlana. Czubek źdźbła wystający ponad cień ściany jest więc jasny, a nasada ciemna: tak jak powinno być.
- Program `grass` dostaje co klatkę siedem uniformów mapy cieni, także gdy cienie są wyłączone.

**Czego nie zmierzyłem.** Wersji z trawą rzucającą cień nikt nie napisał: "migotanie" wynika z porównania szerokości źdźbła z rozmiarem teksela, nie z obserwacji. Wyglądu trawy w cieniu nikt osobno nie zgłosił: jest na liście testów ręcznych.

## 5. Kiedy wrócić do tej decyzji

- Gdy mapa cieni dostanie rozdzielczość, przy której źdźbło ma kilka tekseli szerokości (kaskada z bliska albo dużo mniejsze pudełko).
- Gdy trawa zmieni się w coś większego (krzaki, trzciny), co ma czytelną sylwetkę.
