# Minimapa startuje w lewym dolnym rogu

Data: 2026-10-06. Stan: obowiązuje. Decyzja właściciela projektu (punkt 6 jego decyzji o oknie debug z tego dnia, w całości w [`debug-window-redesign.md`](debug-window-redesign.md), sekcja 2). Zmienia domyślną wartość opisaną w [`minimap-discovered-corridors.md`](minimap-discovered-corridors.md), gdzie róg był wyborem wykonawczym (prawy dolny).
Kod: [`src/game/Minimap.hpp`](../../src/game/Minimap.hpp) (`MinimapSettings::corner`), [`tests/MinimapTests.cpp`](../../tests/MinimapTests.cpp) (test "the minimap settings start with the agreed values"). Dokument: [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md).

## 1. Kontekst

Minimapa miała domyślny róg prawy dolny. Okno debug stoi przy prawej krawędzi okna gry i sięga od wysokości paska HUD do dolnego marginesu ([`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5.4), więc przy pokazanym oknie przykryłoby minimapę w jej dawnym miejscu.

## 2. Decyzja

`MinimapSettings::corner` ma wartość domyślną `MinimapCorner::BottomLeft`. Pozostałe wartości domyślne (rozmiar 0,28, margines 0,02, przezroczystość 0,85) bez zmian. Lista `Corner` w kategorii Gameplay, karta Minimap, nadal pozwala wybrać dowolny z czterech rogów.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **lewy dolny (wybrane)** | po lewej stronie nic nie stoi: okno jest po prawej, HUD u góry pośrodku. Minimapa nie koliduje z oknem | w grze bez okna debug minimapa stoi po lewej, a nie po prawej: zwykłe przyzwyczajenie, nie wada techniczna |
| prawy dolny (poprzednio) | żadnej zmiany | przykryta oknem, gdy jest pokazane |
| lewy górny | po lewej, nic nie stoi | w rogu, w którym zwykle czyta się podpowiedzi; nikt tego nie rozważał szczegółowo |

Dwa ostatnie wiersze to możliwości dopisane przy pisaniu notatki. Właściciel wskazał lewy dolny.

## 4. Uzasadnienie i skutki

**Dlaczego ta.** To decyzja właściciela. Powód z kontekstu (okno po prawej) jest moją analizą, taki sam komentarz stoi w kodzie przy polu `corner`.

**Skutki.**

- Zmieniła się jedna wartość domyślna i jedna asercja w teście. Test `MinimapTests.cpp` sprawdza nową wartość (zgłoszone przez bramkę na gałęzi okna debug: 526 przypadków testowych, 219214 asercji w całym programie testowym).
- Dokumenty, które wymieniały prawy dolny róg jako domyślny, są poprawione na lewy dolny albo mają datowaną uwagę.
- **Co widział agent** (autor kodu, 2026-10-06, nie właściciel): po starcie minimapa w lewym dolnym rogu, przy pokazanym oknie debug nie nachodzą na siebie.

## 5. Kiedy wrócić do tej decyzji

- Gdy okno debug zmieni stronę albo gdy minimapa dostanie własne miejsce w HUD.
- Gdy menu lub ekran ustawień pozwoli graczowi wybrać róg: wtedy wartość domyślna jest ustawieniem gracza, a nie narzędzia.
