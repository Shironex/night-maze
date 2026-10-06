# Okno debug do przeprojektowania: jedno okno z prawej zamiast trzynastu paneli

Data: 2026-10-06. Stan: obowiązuje, **z kodem** (dodatek na końcu notatki: okno istnieje, trzynaście paneli zniknęło). Makieta, na której właściciel to ocenił, leży **poza repozytorium** i nie ma na nią odnośnika. Decyzje właściciela projektu są w sekcji 2; kontekst, tabela i skutki to moja analiza, napisana przed kodem i zostawiona w brzmieniu z tamtej chwili.
Kod, którego dotyczy: [`src/debug/`](../../src/debug/) (`DebugWindow.*`, `Widgets.*`, `Categories.hpp`, `categories/`, `Search.*`, `Theme.*`, `Hud.*`; do 2026-10-06 także `DebugUI.*`, `PanelLayout.*` i trzynaście plików w `panels/`, tych trzech grup już nie ma), a pośrednio [`src/game/Minimap.*`](../../src/game/) (róg minimapy). Dokument: [`../modules/debug-ui.md`](../modules/debug-ui.md).

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

## Dodatek z 2026-10-06: kod istnieje

Sekcje 1 do 5 zostają w brzmieniu z chwili zapisu, kiedy kodu jeszcze nie było. Poniżej to, co powstało, w dwunastu commitach (`5b6a38f` do `f6c6cd5`, lista w [`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja "Historia").

**Co zbudowano.** Jedno okno debug (`DebugWindow`) przy prawej krawędzi okna gry, z paskiem ikon o siedmiu kategoriach (Render, Light, Post process, World, Player, Gameplay, Diagnostics), nagłówkiem z polem wyszukiwania i przyciskiem przypięcia, zakładkami w trzech kategoriach i kartami z wierszami ustawień. Pasek stanu stoi w prawym górnym rogu. Własne widżety (przełącznik w kształcie pigułki, suwak z dwunastoma kreskami, ikony, karta podpowiedzi) są rysowane przez `ImDrawList`, bez czcionki ikon i bez nowej biblioteki. Wyszukiwanie to czysta funkcja `matchesSearch` z siedmioma przypadkami testowymi. Przypięcie zamienia okno na zwykłe, przesuwalne i dokowalne okno ImGui z jedną kategorią. Wszystkie 114 kontrolek i wszystkie odczyty trzynastu paneli przeniesiono, a `static_assert` w `Categories.hpp` pilnuje sumy 114. Kolory (bursztyn i morski), tekst 14 px i róg minimapy (lewy dolny) są zgodne z punktami 3, 5 i 6 decyzji. Pasek HUD zachowuje 16 px tekstu: [`hud-always-at-the-top-edge.md`](hud-always-at-the-top-edge.md). Róg minimapy: [`minimap-default-corner-bottom-left.md`](minimap-default-corner-bottom-left.md).

**Odstępstwa od makiety** (lista od koordynatora prac. Samej makiety nie ma w repozytorium i jej nie widziałem, więc nie porównuję z nią sam):

- Brak wyskakujących okienek z ikoną koła zębatego przy wierszach: każdy wiersz ma suwak, przełącznik albo pole od razu, a dłuższy opis jest w podpowiedzi.
- Obie mapy cieni (księżyca i latarki) są proszone o obraz **w tej samej klatce**, bo zakładka `Shadows` pokazuje je obok siebie. Stary panel miał po jednej zakładce na światło i prosił o jedną mapę naraz. Koszt: jeden dodatkowy przebieg podglądu, gdy zakładka jest otwarta.
- Wartość suwaka da się wpisać z klawiatury przez **Ctrl i kliknięcie** (mechanizm `ImGui::SliderScalar`). Nie ma osobnego pola do wpisywania.
- Nie ma czcionki pogrubionej ani czcionki ikon: jest jedna czcionka w jednym rozmiarze (tytuł okna 16 px, napis na pasku ikon 10 px), a ikony to linie rysowane w kodzie.
- Suwak `Sky brightness` przycina teraz wartość wpisaną z klawiatury do przedziału od 0 do 6. Stary suwak nie miał `AlwaysClamp` i wpisana liczba mogła wyjść poza przedział. Zakres paska bez zmian.

**Wybór koordynatora: okno startuje ukryte.** `DebugUI::m_visible` zaczyna od `false` (było `true`), a klawisz tyldy pokazuje okno. To nie jest punkt decyzji właściciela z sekcji 2: dopisał go koordynator prac przy kodzie, bo gra otwiera się menu głównym, które okno przykryłoby w prawej części. **Do potwierdzenia przez właściciela.** Skutek uboczny: kamera menu i `main.cpp` nie zmieniły się, ale po wyłączeniu kamery menu wraca stan zapamiętany przy jej włączeniu, czyli teraz domyślnie ukryte okno. Wrócić do widocznego okna na starcie to jedna wartość w `DebugUI.hpp`.

**Co z sekcji 5 się spełniło.** Rozwiązanie paska ikon i wyszukiwania w Dear ImGui jest opisane w dokumencie modułu: pasek to okno potomne z przyciskami rysowanymi przez `ImDrawList`, a wyszukiwanie to filtr nakładany na te same funkcje kategorii (nie osobna lista), z własną funkcją dopasowania zamiast `ImGuiTextFilter`. Drugi punkt z sekcji 5 (czy ekran ustawień menu dubluje opcje okna) nie jest jeszcze rozstrzygnięty: ekranu ustawień nie dotykano.

**Co widziano.** Okno widział na zrzutach ekranu agent, który je napisał (2026-10-06), nie właściciel: lista, w tym czego nie widział nikt, jest w [`../guides/build-windows.md`](../guides/build-windows.md) (sekcja o oknie debug) i [`../guides/build-macos.md`](../guides/build-macos.md).
