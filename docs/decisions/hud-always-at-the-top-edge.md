# Pasek HUD zawsze stoi przy górnej krawędzi okna

Data: 2026-10-06. Stan: obowiązuje. Zastępuje [`hud-at-top-edge-when-panels-hidden.md`](hud-at-top-edge-when-panels-hidden.md). To wybór wykonawczy, który wynika z decyzji właściciela o jednym oknie debug ([`debug-window-redesign.md`](debug-window-redesign.md)): rzędy zwiniętych pasków tytułu, nad którymi HUD miał stać, przestały istnieć. Samej reguły "zawsze przy górnej krawędzi" właściciel osobno nie ogłaszał.
Kod: [`src/debug/Hud.cpp`](../../src/debug/Hud.cpp) (`HUD_TOP_OFFSET`, `HUD_FONT_SIZE`, `hudReservedHeight`), [`src/debug/Hud.hpp`](../../src/debug/Hud.hpp), [`src/debug/DebugWindow.cpp`](../../src/debug/DebugWindow.cpp) (`topBelowHud`). Dokument: [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.13.

## 1. Kontekst

Poprzednia notatka zostawiła HUD przy górnej krawędzi tylko wtedy, gdy panele były schowane. Gdy były widoczne, HUD stał pod `FOLDED_ROW_COUNT` rzędami pasków tytułu (166 pikseli przy skali 100 procent, policzone w tamtej notatce), a przy każdym naciśnięciu tyldy **przeskakiwał** między 166 a 16 pikseli. W sekcji 5 tamtej notatki zapisałem dwa powody, dla których wrócę do decyzji: "gdy dojdzie kolejny element przy górnej krawędzi (pasek stanu)" i "gdy skakanie HUD przy klawiszu tyldy przeszkadza". Oba się zdarzyły: okno debug ma pasek stanu w prawym górnym rogu, a rzędów pasków tytułu już nie ma, więc nie ma nad czym HUD miałby stać.

## 2. Decyzja

HUD stoi zawsze `HUD_TOP_OFFSET` = 16 pikseli (przy skali 100 procent) od górnej krawędzi okna, środkiem poziomo. `drawHud` nie ma już argumentu `panelsVisible`, a `FOLDED_ROW_COUNT`, `foldedRowsHeight` i cały `PanelLayout` zostały usunięte. To okno debug schodzi HUD z drogi: zaczyna się poniżej `hudReservedHeight()` (odległość HUD od krawędzi plus wysokość paska z obiema liniami podpowiedzi, około 130 pikseli przy skali 100 procent, policzone ze stałych kodu i motywu) plus 12 pikseli marginesu, czyli około 142 pikseli od góry.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **HUD zawsze przy krawędzi, okno poniżej zarezerwowanej wysokości HUD (wybrane)** | HUD nie skacze przy tyldzie. Okno i pasek stanu nie nachodzą na HUD, także gdy pojawiają się linie podpowiedzi. Znika stała liczona ręcznie przy każdym nowym panelu | okno zaczyna się o około 142 piksele niżej niż sięgałby czysty margines, więc jest niższe. Rezerwa jest stała: nawet bez podpowiedzi okno nie podnosi się wyżej |
| zostawić regułę z `panelsVisible` | najmniej zmian w `Hud.cpp` | reguła nie ma już co liczyć (nie ma rzędów pasków) i zostawiłaby martwy argument |
| okno wyżej, nad rezerwą HUD, i przesuwane z wysokością HUD | więcej miejsca dla okna | okno poruszałoby się za każdym pojawieniem się podpowiedzi, trudno przy takim oknie pracować (komentarz przy `topBelowHud` w kodzie) |
| HUD w rogu okna | nie zasłania środka | zmiana wyglądu paska i jego współpracy z kartami; nie rozważano szczegółowo |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Jest najprostsza i usuwa oba powody powrotu z poprzedniej notatki. Okno debug jest narzędziem przy prawej krawędzi i może mieć stałą rezerwę, a HUD jest częścią gry i nie powinien zależeć od tego, czy narzędzie jest pokazane.

**Skutki.**

- HUD stoi w tym samym miejscu przed i po naciśnięciu tyldy. Z 16 pikseli przy skali 100 procent wynika to z kodu.
- HUD zachowuje **16 px** tekstu (`HUD_FONT_SIZE`), choć motyw okna debug ma 14 px: HUD jest czytany podczas gry z większej odległości. Wywołuje `ImGui::PushFont(nullptr, HUD_FONT_SIZE)`.
- Wysokość `hudReservedHeight()` jest liczona ze stałych (trzy linie tekstu, pasek baterii, odstępy), a nie mierzona w klatce.
- **Co widział agent** (autor kodu, 2026-10-06, "widziane na zrzucie ekranu przez agenta, nie przez właściciela"): okno startuje ukryte, HUD przy górnej krawędzi, po tyldzie okno poniżej HUD, a sam HUD się nie przesunął. Przy baterii zero okno i HUD (z podpowiedzią "Battery empty. Find a crystal.") nie nachodzą na siebie. Nikt nie oglądał innej skali ekranu niż 100 procent ani macOS.

## 5. Kiedy wrócić do tej decyzji

- Gdy HUD dostanie więcej linii podpowiedzi niż dwie: `hudReservedHeight()` liczy wysokość z trzech linii tekstu (licznik i dwie podpowiedzi) i paska baterii, więc trzeba ją poprawić razem z paskiem.
- Gdy okno debug ma stanąć wyżej, na przykład przy niskim oknie gry: wtedy rezerwę trzeba zmniejszyć albo okno dzielić z HUD.
- Gdy dojdzie kolejny element przy górnej krawędzi poza paskiem stanu.
