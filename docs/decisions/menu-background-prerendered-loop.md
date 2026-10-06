# Tłem menu jest zmontowana, wyrenderowana wcześniej pętla wideo, a nie żywa scena

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **kodu odtwarzającego pętlę nie ma** (kod, który pętlę nagrywa, jest: tryb kamery menu). Decyzja właściciela projektu, w całości w sekcji 2; kontekst, tabela możliwości, skutki i warunki powrotu to moja analiza. Domyka punkt 3 decyzji z [`menu-in-rmlui.md`](menu-in-rmlui.md), który przedtem był nierozstrzygnięty.
**Dopisek z 2026-10-06 (M9, część 4):** zdanie "kodu odtwarzającego pętlę nie ma" i zdania o braku odtwarzania w tym nagłówku oraz w dopiskach z części 2 i 3 są **nieaktualne**: odtwarzanie jest w kodzie (biblioteka `video`), pętla jest w repozytorium, a przepis na nagranie jest skryptem. Szczegóły w sekcji 7. Reszta notatki zostaje w brzmieniu z chwili zapisu.
Kod: [`src/game/MenuCamera.hpp`](../../src/game/MenuCamera.hpp) i [`MenuCamera.cpp`](../../src/game/MenuCamera.cpp) (kamera jadąca po labiryncie), [`src/game/StartOptions.hpp`](../../src/game/StartOptions.hpp) i [`StartOptions.cpp`](../../src/game/StartOptions.cpp) (przełączniki `--menu-camera`, `--seed`, `--menu-shot`, `--menu-time`). Odtwarzania wideo w kodzie brak: w `cmake/Dependencies.cmake` nie ma żadnego dekodera. Dokument modułu: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md).

**Dopisek z 2026-10-06 (M9, część 3).** Stan tła nie zmienił się: menu główne (i ustawienia otwarte z niego) pokazują żywy wysoki przelot kamery menu (`usesMenuCamera`), a odtwarzania pętli wideo nadal nie ma. Przelot ma od części 3 długość zależną od poziomu (około 46 s dla 10 na 10, 73 s dla 16 na 16, 100 s dla 22 na 22, policzone z wzorów w [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), sekcja 2.8, nie zmierzone), a pierwszy labirynt za menu ma rozmiar poziomu zapisanego w pliku ustawień. Na `Normal` i `Hard` światła kryształów w tle są wybierane spośród 16 najbliższych oka przelotu ([`nearest-crystals-carry-the-lights.md`](nearest-crystals-carry-the-lights.md)); nikt nie oglądał, jak to wygląda na ekranie.

## 1. Kontekst

Punkt 3 decyzji o menu w RmlUi zostawiał otwarte, czym jest tło menu: żywą sceną gry za menu albo wcześniej wyrenderowaną pętlą wideo. Właściciel chciał porównać obie możliwości na prawdziwej grze, zanim wybierze.

Porównanie powstało 2026-10-06. Pierwsza część M9 dodała tryb kamery menu ([`../modules/game/menu-camera.md`](../modules/game/menu-camera.md)): gra pokazuje samą siebie, a kamera jedzie przez labirynt. Z niego powstały **dwa nagrania** (poza repozytorium, pokazane właścicielowi w makiecie projektowej, informacja od autora kodu, nie do sprawdzenia w repozytorium): ciągłe ujęcie 30 s (9,5 MB) i zmontowana pętla 24 s z czterech ujęć po 7 s z przenikaniami po 1 s (10,6 MB w 720p30; w jakości docelowej CRF 14 ta sama pętla miała 19,7 MB). Agent oglądał klatki z gry (nie właściciel, nie ekran w ruchu): żywy spacer jest dobry w prostych korytarzach i słaby w ciasnych zakrętach oraz tam, gdzie kamera zawraca przy celu (mniej więcej połowa spaceru to dobre widoki, reszta to ściany z bliska). Zmontowana pętla jest czystsza, bo ujęcia są **wybrane**, a nie jadą po całej trasie.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. Tłem menu będzie **zmontowana, wyrenderowana wcześniej pętla wideo**, nie żywa scena. Powody właściciela: wygląda czyściej i podoba mu się widok z góry, który pokazuje pętla (wysoki przelot nad labiryntem).
2. Dwa dodatkowe przełączniki wiersza poleceń, `--menu-shot` i `--menu-time`, **zostają**, żeby klipy dało się nagrać ponownie albo nagrać nowe do intra gry i do launchera.
3. Wciąż obowiązuje: menu powstanie w RmlUi w M9 ([`menu-in-rmlui.md`](menu-in-rmlui.md)).

Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Wcześniej wyrenderowana pętla (wybrane)** | ujęcia dobrane ręcznie, bez słabych zakrętów i zawracania; przenikania i montaż; tło kosztuje tyle co odtworzenie wideo, a nie cała scena (cienie, bloom, mgła) za menu; wygląda tak samo na każdym komputerze | gra nie ma dekodera wideo, więc M9 musi wybrać sposób odtwarzania; plik binarny w repozytorium (zmierzone 19,7 MB w jakości docelowej); pętla **starzeje się**, gdy zmieni się grafika (w pierwszej brakuje kałuż i bramy) |
| Żywa scena, jedno ciągłe ujęcie | działa już dziś (tryb F2); zawsze aktualna grafika; zero nowych zależności i zero plików | z całej trasy około połowy to dobre ujęcia, reszta to ściany z bliska; menu zajmuje GPU pełną sceną; trasa jest długa (autor zgłasza około 500 s w labiryncie 10 na 10) |
| Żywa scena, cięcia między wybranymi ujęciami | grafika aktualna i ujęcia wybrane | trzeba napisać montaż w kodzie (wybór ujęć, przenikania), którego dziś nie ma; wciąż pełna scena za menu |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienie jest w punkcie 1 decyzji i nie dodaję do niego nic od siebie.

**Skutki znane i otwarte** (analiza):

- **Odtwarzanie wideo w grze.** Dziś gra nie ma dekodera wideo (sprawdzone wyszukiwaniem w `src/`, `cmake/` i `CMakeLists.txt`). M9 musi wybrać, jak pętla będzie odtwarzana. Trzy **kandydaci do oceny, żaden nie wybrany i żaden nie sprawdzony**: duża biblioteka dekodująca, taka jak FFmpeg (duża zależność, ale zna wszystkie formaty); mały jednoplikowy dekoder MPEG-1, taki jak pl_mpeg (pasuje do wyboru małych własnych pomocników zamiast dużych bibliotek, ale kosztuje jakość albo rozmiar); ciąg skompresowanych klatek (np. tekstur) odtwarzany własnym kodem. Przy każdym trzeba sprawdzić działanie na macOS z profilem 4.1 Core, licencję i koszt czasu.
- **Plik binarny w repozytorium.** Zmierzona pętla 720p30 w jakości docelowej miała 19,7 MB. Repozytorium już zaakceptowało wzrost przy niebie (sześć obrazów, 5,28 MB, [`small-calls-after-m6.md`](small-calls-after-m6.md), punkt 4), a tam wskazano Git LFS dla `assets/` jako pierwszy krok, gdyby rozmiar zaczął przeszkadzać. To samo dotyczy pętli.
- **Pętla się starzeje.** Każda zmiana grafiki (kałuże, brama, oświetlenie) czyni nagranie nieaktualnym. Dlatego przełączniki zostają, a **przepis na ponowne nagranie musi być zapisany** ([`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), sekcja 5.9). Dokładnych poleceń `ffmpeg` w repozytorium nie ma: zostaną zapisane w `tools/`, gdy pętla będzie nagrywana naprawdę.
- **Rola trybu kamery menu zmienia się**: z tła menu na **narzędzie do nagrywania** (i pokaz pod F2).

**Czego ta notatka nie przesądza.** Sposobu odtwarzania, formatu, rozdzielczości ani liczby ujęć w docelowej pętli.

## 5. Kiedy wrócić do tej decyzji

- Gdy M9 wybierze sposób odtwarzania pętli: zapisać wybór i jego koszt (rozmiar zależności, rozmiar pliku, jakość).
- Jeśli rozmiar pliku albo dekoder okaże się za drogi: plan awaryjny to żywa scena, która już działa (druga i trzecia możliwość z tabeli).
- Gdy zmieni się grafika: nagrać pętlę od nowa według przepisu.

## 6. Dodatek z 2026-10-06 (M9, część 2): menu istnieje, odtwarzania pętli nadal brak

Sekcje od 1 do 5 zostają w brzmieniu z chwili zapisu. Co się zmieniło:

- **Menu główne jest w kodzie** ([`../modules/ui/README.md`](../modules/ui/README.md), [`../modules/game/game-states.md`](../modules/game/game-states.md)). Do czasu, aż pętla będzie odtwarzana, jego tłem jest **żywy wysoki przelot** kamery menu: `NightMazeApp` pożycza kamerę menu dla ekranu głównego (`usesMenuCamera`) i zawsze ustawia ujęcie `HighGlide`, bez włączania flagi `m_menuCamera.enabled`. To rozwiązanie przejściowe, a nie zmiana decyzji.
- **Otwarty punkt "sposób odtwarzania" jest rozstrzygnięty** decyzją właściciela z 2026-10-06 w osobnej notatce: [`video-through-os-decoders-with-still-fallback.md`](video-through-os-decoders-with-still-fallback.md). Kodu odtwarzania nadal nie ma (sprawdzone: w `src/` i `cmake/` nie ma dekodera wideo).
- Funkcja `game::drawsScene(mode, fullscreenBackground)` jest w kodzie i ma test, ale renderer jeszcze jej nie używa: tła na cały ekran nie ma.

## 7. Dodatek z 2026-10-06 (M9, część 4): odtwarzanie i pętla istnieją

Sekcje od 1 do 6 zostają w brzmieniu z chwili zapisu. **Decyzja właściciela z sekcji 2 się nie zmienia.** Co się zmieniło w kodzie (opis w [`../modules/video/README.md`](../modules/video/README.md)):

- **Odtwarzanie jest w kodzie.** Menu główne i ustawienia otwarte z niego pokazują pętlę wideo, odtwarzaną przez dekoder systemu (decyzja właściciela z [`video-through-os-decoders-with-still-fallback.md`](video-through-os-decoders-with-still-fallback.md)), a gdy to się nie uda, nieruchomy obraz, a gdy i to się nie uda, żywą scenę. `game::drawsScene` jest wpięta: za wideo albo obrazem scena nie jest rysowana w ogóle. Żywy przelot zostaje jako `--menu-background scene`.
- **Pętla jest w repozytorium:** `assets/video/menu_loop.mp4` (17 646 243 bajtów, 30 s, 1280 x 720, 4,71 Mbit/s) i `menu_still.png`. Sekcja 4 mówiła o 19,7 MB w jakości docelowej dla pętli 24 s (CRF 14); dzisiejsza pętla ma 30 s i CRF 16, więc to inny plik, a nie to samo zmierzone jeszcze raz. Przy CRF 18 miała 13,18 MB (pomiar autora kodu).
- **Przepis jest skryptem:** `tools/record_menu_loop.py` (sekcja 4 mówiła, że "dokładnych poleceń `ffmpeg` w repozytorium nie ma", a skutki, że zostaną zapisane w `tools/`). Opis: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), sekcja 5.9.
- **Rola trybu kamery menu** jest taka, jak zapisano w sekcji 4: narzędzie do nagrywania (i pokaz pod F2), a nie tło.

**Co zostaje otwarte** (analiza):

- **Odbicia w kałuży w pętli nie ma.** Zadanie nagrania go wymagało, ale kamery menu nie patrzą w dół i żadnego nie dało się pokazać. To punkt do decyzji właściciela (czy dodać ujęcie z kamerą patrzącą w dół, co wymaga nowego ustawienia kamery).
- **Rozmiar pliku w git.** 17,6 MB dochodzi do repozytorium przy każdym nagraniu na nowo; ścieżka awaryjna z sekcji 4 (Git LFS dla `assets/`) jest nadal opisana jako pierwszy krok, gdyby rozmiar zaczął przeszkadzać.
- **Pętla się starzeje** (jak w sekcji 4); odnowienie to jedno polecenie skryptu, ale tylko na Windowsie.
- **macOS:** dekoder jest napisany i nigdy nie skompilowany ([`../guides/build-macos.md`](../guides/build-macos.md)).
