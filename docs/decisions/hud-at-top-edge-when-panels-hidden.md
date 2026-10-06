# Pasek HUD stoi przy górnej krawędzi okna, gdy panele debug są schowane

Data: 2026-10-06. Stan: obowiązuje. Decyzja właściciela projektu (punkt 3 jego decyzji z tego dnia, w całości w sekcji 1); sposób wykonania (jeden dodatkowy argument funkcji) to wybór wykonawczy. Zmienia regułę opisaną w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.9.5, zgodnie z którą HUD stał pod wszystkimi rzędami pasków zawsze.
Kod: [`src/debug/Hud.cpp`](../../src/debug/Hud.cpp) (`drawStatus`, `drawHud`), [`src/debug/Hud.hpp`](../../src/debug/Hud.hpp), [`src/debug/DebugUI.cpp`](../../src/debug/DebugUI.cpp) (wywołanie `drawHud` z `m_visible`), [`src/debug/PanelLayout.hpp`](../../src/debug/PanelLayout.hpp) (`FOLDED_ROW_COUNT`).

## 1. Kontekst

Pasek HUD (kryształy, czas, bateria, podpowiedź) stoi na środku górnej krawędzi okna. Panele debug, które startują zwinięte, zajmują tam rzędy pasków tytułu, a HUD miał stać pod nimi, więc jego odległość od krawędzi liczył się ze stałej `FOLDED_ROW_COUNT` (5 od panelu Environment): przy skali ekranu 100 procent `5 * (22 + 8) + 16 = 166` pikseli (liczby z dokumentu panelu, policzone), czyli około 23 procent wysokości okna 720. Stała nie liczyła pasków, które naprawdę są na ekranie, więc HUD stał tak samo nisko także wtedy, gdy panele były schowane klawiszem tyldy i żadnego paska nie było. Dokument panelu zapisywał to jako konsekwencję stałej: HUD nie skacze przy chowaniu paneli, ale przy schowanych wisi niżej, niż by musiał.

Przy schowanych panelach, czyli w trybie, w którym się gra, pasek wisiał więc w górnej ćwiartce okna, **nad środkiem obrazu, tam gdzie świeci latarka**. Tak samo mówi komentarz w kodzie: przy schowanych panelach nie ma pasków, nad którymi HUD miałby stać, a nie ma on stać ćwierć okna niżej, nad środkiem obrazu, gdzie świeci latarka.

**Decyzje właściciela projektu (2026-10-06), w całości:** (1) kałuże idą za gruntem, (2) woda ma być lepiej widoczna: jaśniejszy odcień, mocniej odbijająca, miękki brzeg, więcej narożników, (3) HUD stoi przy górnej krawędzi, dopóki panele debug są schowane. Punkt 1 jest w notatce [`puddles-follow-the-ground.md`](puddles-follow-the-ground.md), punkt 2 w uzupełnieniu notatki [`visible-effect-over-physical-values.md`](visible-effect-over-physical-values.md).

## 2. Decyzja

`drawHud` dostaje argument `panelsVisible`. Przy `true` HUD stoi tak jak dotąd, pod `FOLDED_ROW_COUNT` rzędami pasków plus `HUD_TOP_OFFSET`. Przy `false` wysokość rzędów jest 0, a HUD stoi `HUD_TOP_OFFSET` (dwa odstępy paneli, 16 pikseli przy skali 100 procent) od górnej krawędzi. `DebugUI::draw` przekazuje `m_visible`.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **górna krawędź, gdy panele schowane (wybrane)** | HUD nie wisi nad środkiem obrazu w trybie gry. Jedna linia kodu liczy wysokość rzędów, stała `FOLDED_ROW_COUNT` zostaje | HUD **skacze** o `FOLDED_ROW_COUNT` rzędów przy każdym przełączeniu klawiszem tyldy (to była jedyna zaleta starej reguły). Karty (zwycięstwo, kartka) i podpowiedź mają własne miejsca i się nie zmieniły |
| zawsze pod rzędami pasków (poprzednio) | HUD stoi w tym samym miejscu przy schowanych i widocznych panelach, nie nachodzi na paski | pasek nad środkiem obrazu w trybie gry |
| zawsze przy górnej krawędzi, a panele przesunięte pod niego | HUD nie skacze | trzeba zmienić położenie wszystkich paneli i ich zapisany układ (`imgui.ini` ma pierwszeństwo przed wartościami domyślnymi). Nikt tego nie rozważał szczegółowo |
| HUD w rogu okna | nie zasłania środka | zmiana wyglądu paska i jego współpracy z kartami. Nikt tego nie rozważał szczegółowo |

Dwa ostatnie wiersze to możliwości zapisane przy pisaniu notatki, których właściciel nie oceniał.

## 4. Uzasadnienie i skutki

**Dlaczego ta.** To decyzja właściciela. Wykonanie jest najmniejsze, jakie ją spełnia: panele są narzędziem i przy ich schowaniu nic już nie ma nad HUD.

**Skutki, które przyjmuję.**

- HUD przeskakuje przy klawiszu tyldy (z 166 pikseli na 16 przy skali 100 procent).
- Zdanie z dokumentów paneli "HUD jest odsuwany o wszystkie rzędy zawsze, także gdy panele są schowane" przestało być prawdą i jest poprawione tam, gdzie opisuje stan obecny. Starsze opisy części M5 do M7 ("po tej części HUD stał pod czwartym rzędem") zostają jako historia: nadal opisują stan z panelami widocznymi.
- Komentarz przy stałej `HUD_TOP_OFFSET` w `Hud.cpp` mówi nadal, że HUD stoi pod rzędami pasków; dotyczy to teraz tylko trybu z widocznymi panelami.
- **Co widział agent** (autor poprawek, build jego worktree **przed scaleniem** z kodem dźwigni; "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela"): HUD przy górnej krawędzi przy schowanych panelach i pod pięcioma rzędami pasków tytułu przy widocznych. Nikt nie oglądał HUD przy innej skali ekranu niż 100 procent, przy 1440p ani po zmianie rozmiaru okna.
- **Zgłoszone przez bramkę:** `make check` przechodzi (467 przypadków, 158006 asercji). Zmiany w `Hud.*` nie mają testu jednostkowego (potrzebują ImGui).

## 5. Kiedy wrócić do tej decyzji

- Gdy skakanie HUD przy klawiszu tyldy przeszkadza przy pokazie: wtedy układ z panelami przesuniętymi pod HUD.
- Gdy dojdzie kolejny element przy górnej krawędzi (pasek stanu, kompas): miejsce HUD trzeba będzie ustalić z nim.
- Gdy `FOLDED_ROW_COUNT` albo `HUD_TOP_OFFSET` zmienią się: ten opis liczb (166 i 16 pikseli) trzeba poprawić.

## Dodatek z 2026-10-06 (M9, część 2)

Sekcje 1 do 5 zostają w brzmieniu z chwili zapisu. Nowe: HUD jest rysowany tylko na ekranie `Playing` (`context.hudVisible` w `DebugUI::draw`, wartość z `NightMazeApp::hudVisible()`), a nie "zawsze, poza kamerą menu". Reguła tej notatki (gdzie stoi pasek) dotyczy tylko chwil, w których HUD jest w ogóle rysowany. Powód ograniczenia: Dear ImGui rysuje po dokumentach menu, więc HUD w pauzie leżałby na przyciskach ([`../modules/game/game-states.md`](../modules/game/game-states.md)).
