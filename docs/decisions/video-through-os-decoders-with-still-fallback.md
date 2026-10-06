# Wideo tła menu przez dekodery systemu operacyjnego, z nieruchomym obrazem na wypadek porażki

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **kodu odtwarzającego wideo nie ma**. Decyzja właściciela projektu jest w całości w sekcji 2; kontekst, tabela możliwości, skutki i warunki powrotu to moja analiza. Domyka punkt otwarty z [`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md): tam wybór pętli wideo był rozstrzygnięty, a sposób jej odtwarzania nie.
Kod: brak. W `cmake/Dependencies.cmake` nie ma żadnego dekodera wideo (sprawdzone wyszukiwaniem w `cmake/`, `CMakeLists.txt` i `src/`). Gotowy jest tylko jeden przygotowany punkt zaczepienia: funkcja `game::drawsScene(mode, fullscreenBackground)` w [`src/game/GameState.cpp`](../../src/game/GameState.cpp), która ma test i której renderer jeszcze nie woła ([`../modules/game/game-states.md`](../modules/game/game-states.md)). Dokumenty: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md) (nagrywanie pętli), [`../modules/ui/README.md`](../modules/ui/README.md) (warstwa menu).

## 1. Kontekst

Właściciel wybrał wcześniej wyrenderowaną pętlę wideo jako tło menu ([`menu-background-prerendered-loop.md`](menu-background-prerendered-loop.md)). Ta notatka zostawiała otwarte, jak gra ją odtworzy, i wymieniała trzech kandydatów bez wyboru: dużą bibliotekę dekodującą w rodzaju FFmpeg, mały jednoplikowy dekoder MPEG-1 i własny ciąg skompresowanych klatek.

Przed decyzją porównano dwie z tych dróg na tej samej pętli. Pomiar wykonano poza repozytorium (liczby zgłoszone, nie do sprawdzenia w repozytorium, nie powtarzałem ich): odtwarzanie przez FFmpeg i odtwarzanie przez dekodery wbudowane w system operacyjny dają ten sam obraz, ale pierwsze oznacza około 130 MB bibliotek DLL, a drugie około 10 KB.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. Pętla tła menu będzie odtwarzana przez **dekodery wideo, które ma system operacyjny**, a nie przez FFmpeg ani przez dekoder dołączony do projektu.
2. Gdy odtwarzanie się nie uda (brak dekodera, brak pliku, błąd), tłem menu jest **nieruchomy obraz** zamiast pętli. Menu ma działać zawsze, także bez wideo.

Powód właściciela: ten sam obraz co z FFmpeg za około 10 KB zamiast około 130 MB bibliotek DLL. Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

To jest analiza. Pierwszy wiersz jest decyzją właściciela, trzy następne to możliwości z poprzedniej notatki.

| Możliwość | Zalety | Wady |
|---|---|---|
| **Dekodery systemu operacyjnego + nieruchomy obraz zapasowy (wybrane)** | zero nowych bibliotek w paczce gry; ten sam obraz co FFmpeg (pomiar poza repozytorium); około 10 KB zamiast około 130 MB bibliotek DLL (pomiar poza repozytorium); wersja na każdym systemie używa jego własnego, już zainstalowanego dekodera | kod zależny od platformy: dwa różne interfejsy systemowe, a w projekcie jest dziś jeden plik z kodem zależnym od systemu (`src/core/Paths.cpp`, [`../modules/core/paths.md`](../modules/core/paths.md)); format pliku musi być taki, który oba systemy odtworzą; na macOS nic z tego nie było uruchamiane |
| FFmpeg | zna każdy format; jeden kod na wszystkich systemach | około 130 MB DLL w paczce (pomiar poza repozytorium), licencja do sprawdzenia (zależy od sposobu budowy), duża zależność wbrew wyborowi małych własnych pomocników |
| Mały dekoder MPEG-1 w jednym pliku | jedna zależność, jeden kod na obu systemach | koszt w jakości albo rozmiarze pliku, nie sprawdzony |
| Własny ciąg skompresowanych klatek | pełna kontrola, brak zależności | własny kod i własny format do napisania, utrzymania i wytłumaczenia; nie sprawdzony |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienie jest w punkcie 2 decyzji i nie dodaję do niego nic od siebie.

**Skutki znane i otwarte** (analiza):

- **Interfejsy systemowe.** Nazwy to moje przypuszczenie, nie ustalenie z kodu, którego nie ma: na Windowsie Media Foundation, na macOS AVFoundation. Wybór interfejsu, formatu kontenera i kodeka należy do pracy nad M9. Trzeba sprawdzić, że wybrany format odtwarza się na obu systemach, także na komputerze bez dodatkowych kodeków.
- **Droga obrazu do ekranu.** Dekoder daje klatki w pamięci. Do ekranu muszą trafić jako tekstura narysowana pod menu RmlUi, na początku klatki, zamiast sceny. Do tego służy `drawsScene(mode, fullscreenBackground)`: przy menu głównym z tłem na całe okno scena nie jest rysowana, więc jej przebiegi (cienie, bloom, mgła) można pominąć. Dziś tła nie ma i funkcja zwraca `true` dla każdego ekranu.
- **Stan przejściowy.** Do chwili, gdy odtwarzanie powstanie, menu główne pokazuje **żywy wysoki przelot** kamery menu nad labiryntem (`usesMenuCamera`, [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md)). To nie jest wybór tła, tylko to, co jest pod ręką.
- **Pętla nadal starzeje się** i nadal trzeba ją nagrać według przepisu z [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), sekcja 5.9. Rozmiar pliku w repozytorium (zmierzone wcześniej 19,7 MB w jakości docelowej) pozostaje otwarty.
- **Nieruchomy obraz** to np. jedna klatka pętli zapisana jako zwykły obraz. Skąd się weźmie i jak duży będzie, nie jest ustalone.

**Czego ta notatka nie przesądza.** Interfejsu systemowego, formatu, rozdzielczości, liczby ujęć ani wyglądu obrazu zapasowego.

## 5. Kiedy wrócić do tej decyzji

- Gdy powstanie kod odtwarzania: zapisać wybrany interfejs, format i zmierzony koszt (rozmiar pliku, czas dekodowania, zużycie procesora).
- Jeśli któryś z systemów nie odtworzy wybranego formatu bez dodatkowych kodeków: wrócić do tabeli, w pierwszej kolejności do małego dekodera MPEG-1.
- Jeśli kod zależny od platformy okaże się za drogi w utrzymaniu: nieruchomy obraz zostaje i tak jako stan końcowy menu, a pętlę można odłożyć.
