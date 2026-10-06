# Okno debug do przeprojektowania: jedno okno z prawej zamiast trzynastu paneli

Data: 2026-10-06. Stan: obowiązuje jako decyzja o kierunku, **kodu nie ma**: w programie nadal jest trzynaście paneli Dear ImGui. Makieta, na której właściciel to ocenił, leży **poza repozytorium** i nie ma na nią odnośnika. Decyzje właściciela projektu są w sekcji 2; kontekst, tabela i skutki to moja analiza.
Kod, którego dotyczy: [`src/debug/`](../../src/debug/) (`DebugUI.*`, `PanelLayout.*`, trzynaście plików w `panels/`, `Hud.*`, `Theme.*`), a pośrednio [`src/game/Minimap.*`](../../src/game/) (róg minimapy). Dokument: [`../modules/debug-ui.md`](../modules/debug-ui.md).

## 1. Kontekst

Panele debug rosły z każdym kamieniem milowym: M0 miał jeden panel, dziś jest ich trzynaście (Renderer, Shaders, Camera, Gameplay, Terrain, Grass, Framebuffers, Shadows, Environment, Maze, Collision, Assets, Lights), ułożone w pięciu rzędach zwiniętych pasków tytułu przy górnej krawędzi okna ([`../modules/debug-ui.md`](../modules/debug-ui.md)). Z dwunastoma panelami ich układ i HUD zaczęły sobie wchodzić w drogę: pasek HUD stoi o rzędy niżej, gdy panele są widoczne ([`hud-at-top-edge-when-panels-hidden.md`](hud-at-top-edge-when-panels-hidden.md)), a panel Environment wymagał osobnej przeróbki, żeby się zmieścił.

Dziś wygląd i układ tych paneli to wynik kolejnych dodatków, a nie projektu. Z M9 gra dostaje własne menu w RmlUi ([`menu-in-rmlui.md`](menu-in-rmlui.md)), więc narzędzie debug ma być wyraźnie osobną rzeczą, a nie częścią tego, co widzi gracz.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. **Jedno okno po prawej stronie zastępuje trzynaście paneli.**
2. W oknie jest **pasek ikon** (rail), przełączający sekcje.
3. Kolorystyka: **bursztynowy i morski (teal)**.
4. Okno ma **wyszukiwanie i przypinanie** (pin).
5. Rozmiar tekstu: **14 px**.
6. **Domyślny róg minimapy przechodzi na lewy dolny** (dziś prawy dolny), bo prawą stronę zajmuje okno.

Reszta tej notatki to analiza.

## 3. Rozważane możliwości

To jest analiza. Pierwszy wiersz jest decyzją właściciela.

| Możliwość | Zalety | Wady |
|---|---|---|
| **Jedno okno z prawej, pasek ikon, wyszukiwanie i przypinanie (wybrane)** | jedno miejsce na wszystkie ustawienia, nie trzeba pamiętać, który panel co ma; wyszukiwanie znajduje suwak po nazwie; przypięte pozycje zostają widoczne przy pracy nad jedną rzeczą; koniec z rzędami paneli i paskiem HUD przesuwanym pod nie | przepisanie wszystkich trzynastu plików paneli i układu; praca na obszarze, w którym dziś nic nie jest zepsute; Dear ImGui nie ma gotowego widżetu paska ikon ani przypinania (to własny kod), a do wyszukiwania ma pomocnika `ImGuiTextFilter`, który trzeba jeszcze podłączyć do pozycji okna |
| Zostawić trzynaście paneli | zero pracy | układ rośnie z każdym nowym panelem; HUD i panele nadal współdzielą krawędź okna |
| Tylko uporządkować rzędy | mało pracy | nie rozwiązuje wyszukiwania ani przypinania |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Powodów właściciela nie podano mi poza samą decyzją, więc ich nie zapisuję. Sekcja 1 to moja analiza tego, co w układzie paneli dziś przeszkadza (z dokumentów projektu), a nie uzasadnienie właściciela.

**Skutki znane** (analiza):

- **Minimapa.** Dziś `MinimapSettings` ma domyślny róg prawy dolny ([`../modules/renderer/minimap.md`](../modules/renderer/minimap.md)); zmiana na lewy dolny to jedna wartość w ustawieniach, ale testy i dokument minimapy wymienią ten róg i trzeba je poprawić razem z kodem.
- **HUD.** Reguła z [`hud-at-top-edge-when-panels-hidden.md`](hud-at-top-edge-when-panels-hidden.md) (pasek HUD pod rzędami pasków tytułu, gdy panele są widoczne) przestaje mieć sens, gdy rzędów nie będzie. Notatka zostanie zastąpiona.
- **Interakcja z menu.** Okno zajmuje prawą stronę, a menu RmlUi stoi na środku ([`../modules/ui/README.md`](../modules/ui/README.md)). Zasada, że panele wygrywają z menu pod tym samym kursorem (`setMouseEnabled`), zostaje bez zmian.
- **Rozmiar tekstu.** Dziś czcionka paneli ma rozmiar bazowy podawany ze stylu Dear ImGui ([`../libraries/imgui.md`](../libraries/imgui.md), sekcja 3.12). 14 px to wartość z decyzji, a nie z kodu.
- **Wielkość pracy** nie jest oszacowana.

**Czego ta notatka nie przesądza.** Kolejności sekcji, ikon, dokładnych odcieni, szerokości okna, tego, czy okno można zwinąć, ani terminu. Makieta nie jest częścią repozytorium.

## 5. Kiedy wrócić do tej decyzji

- Gdy zacznie się praca nad kodem: zapisać, jak okno rozwiązało pasek ikon i wyszukiwanie w Dear ImGui.
- Gdy ekran ustawień menu ([`menu-scope-for-m9.md`](menu-scope-for-m9.md)) zacznie dublować opcje okna debug: rozdzielić, co jest dla gracza, a co dla narzędzia.
