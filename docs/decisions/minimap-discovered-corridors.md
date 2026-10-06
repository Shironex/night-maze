# Minimapa pokazuje tylko odkryte korytarze

Data: 2026-10-05, uzupełniona 2026-10-06 (dwie dalsze decyzje właściciela i kod szóstej części M7). Stan: obowiązuje, z kodem (2026-10-06). **Nikt nie obejrzał obrazu minimapy** (sekcja 4).
Kod, którego dotyczy: [`src/game/Discovery.*`](../../src/game/), [`src/game/Minimap.*`](../../src/game/), [`src/game/MinimapRenderer.*`](../../src/game/), [`assets/shaders/post/minimap.vert`](../../assets/shaders/post/minimap.vert), [`minimap.frag`](../../assets/shaders/post/minimap.frag), [`minimap_overlay.frag`](../../assets/shaders/post/minimap_overlay.frag), pole `Round::discovery` w [`src/game/Round.*`](../../src/game/). Dokumenty: [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md), [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) (temat 10, rendering pozaekranowy, w trakcie), [`../syllabus.md`](../syllabus.md). Stan M7: [`../guides/m7-status.md`](../guides/m7-status.md).

## 1. Kontekst

PRD wymienia przy temacie 10 (rendering pozaekranowy) minimapę: widok labiryntu rysowany do osobnego framebuffera. To ostatnia brakująca rzecz tego tematu i szósta, ostatnia część M7.

Gra polega na szukaniu drogi w labiryncie. Minimapa, która od początku pokazuje cały labirynt z wyjściem, odpowiada na to pytanie za gracza.

## 2. Decyzja

Decyzja właściciela projektu z 2026-10-05: minimapa pokazuje **tylko korytarze, które gracz już odkrył**. Do tego przełącznik debugowania, który odsłania cały labirynt.

Uwaga o zakresie: treścią decyzji właściciela jest jedno zdanie z części 2 i dwa zdania z części 4 (decyzje z 2026-10-06). Kontekst, tabela możliwości, uzasadnienie i wszystko oznaczone niżej jako wybór wykonawczy to mój zapis, nie część decyzji.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **tylko odkryte korytarze, z przełącznikiem debugowania (wybrane)** | minimapa pomaga wracać i nie błądzić w kółko, ale nie pokazuje drogi do wyjścia. Przełącznik pozwala pokazać na obronie cały obraz z framebuffera | potrzebny jest stan "które komórki są odkryte" i reguła, kiedy komórka się odkrywa |
| cały labirynt od początku | najprostsze: jeden widok z góry bez żadnego stanu | zabiera grze sens: wyjście i droga do niego są widoczne od pierwszej sekundy |
| brak minimapy | zero pracy | temat 10 zostaje bez elementu wymienionego w PRD |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Minimapa ma dwie role i ta decyzja je rozdziela. Dla gracza jest pamięcią przebytej drogi. Dla obrony jest pokazem renderingu do tekstury, i do tego służy przełącznik `Reveal all`, który odsłania wszystko.

**Dwie dalsze decyzje właściciela projektu (2026-10-06), to jest cała ich treść:**

- komórka jest **odkryta przez linię wzroku wzdłuż korytarzy**: odkrywa się komórka, w której gracz stoi, i komórki w linii prostej w czterech kierunkach, aż do ściany,
- minimapa to **schemat rysowany z danych labiryntu** do własnego framebuffera, a nie osobny widok sceny z góry.

**Co kod zrobił z tych decyzji.** Rzeczy, które były otwarte w pierwszej wersji tej notatki (rozmiar i miejsce, wygląd schematu, działanie przełącznika), mają od 2026-10-06 kod. Poniższa tabela oddziela decyzje właściciela od wyborów wykonawczych. Przy wyborach wykonawczych powód pochodzi z komentarzy w kodzie.

| Co | Kto zdecydował | Wartość w kodzie | Powód |
|---|---|---|---|
| minimapa pokazuje tylko odkryte korytarze, z przełącznikiem odsłaniającym wszystko | **właściciel** (2026-10-05) | `MinimapSettings::revealAll`, pole `Reveal all` | patrz wyżej |
| reguła odkrywania: linia wzroku wzdłuż korytarzy | **właściciel** (2026-10-06) | `discoverFrom` | patrz wyżej |
| schemat z danych labiryntu do własnego framebuffera | **właściciel** (2026-10-06) | `buildMinimapVertices`, `MinimapRenderer` | patrz wyżej |
| stan odkrycia leży w rundzie i jest zerowany w `startRound` | wykonawczy | `Round::discovery` | odkrycie jest stanem rundy, nowa runda to nowa siatka. Komórka startu jest odkryta już w `startRound`, żeby pierwsza klatka ją pokazywała |
| aktualizacja w `updateRound`, z pozycji stóp po kroku | wykonawczy | `discoverAround` | liczy się to, gdzie gracz stoi po kroku stałym. Trwa po wygranej i przy noclip (liczą się tylko `x` i `z`) |
| ściany czytane z `Maze` w każdym wywołaniu, nic nie zapamiętane | wykonawczy | `discoverFrom` | ściana usunięta w środku rundy otwiera widok od następnego wywołania |
| widok przechodzi przez bramę, otwartą i zamkniętą | wykonawczy | brama nie jest ścianą `Maze` | brama jest częścią świata rozgrywki, a reguła pyta tylko o ściany labiryntu |
| rozmiar 0,28 wysokości framebuffera okna, margines 0,02 | wykonawczy | `MinimapSettings` | część wysokości, a nie piksele, żeby mapa zajmowała ten sam ułamek obrazu w każdym oknie i na Retina |
| narożnik prawy dolny (**zmieniony 2026-10-06 na lewy dolny**, `MinimapCorner::BottomLeft`: decyzja właściciela, [`minimap-default-corner-bottom-left.md`](minimap-default-corner-bottom-left.md)) | wykonawczy, potem właściciel | `MinimapCorner::BottomRight` (do 2026-10-06) | HUD stoi na górze pośrodku i rośnie w dół z podpowiedziami. Prawy róg zajmuje od 2026-10-06 okno debug |
| framebuffer mapy `GL_RGBA8`, bez głębi, rozmiaru kwadratu na ekranie | wykonawczy | `MinimapRenderer::drawMap` | kopiowanie piksel w piksel, bez skalowania. Kolejność listy zastępuje głębię |
| przezroczystość 0,85 | wykonawczy | `MinimapSettings::opacity` | mapa zasłania większość sceny, ale scena nadal prześwituje |
| północ u góry, mapa się nie obraca | wykonawczy | `minimapProjection` | schemat ma stały układ, strzałka gracza się obraca |
| ściana między komórkami rysowana, gdy któraś z nich jest pokazana | wykonawczy | `buildMinimapVertices` | gracz widział ją z jednej strony |
| kształty: ściany cienkie prostokąty, kryształy romby, gracz trójkąt, z minimalnymi rozmiarami w pikselach | wykonawczy | `buildMinimapVertices` | czytelność w dużym labiryncie, prostokąt cieńszy niż piksel może zniknąć |
| `cellAt` używa `floor` | wykonawczy | `MazeLayout::cellAt` | zwykła konwersja na `int` obcinałaby ułamek w stronę zera |
| minimapa nie jest częścią `PostProcess` | wykonawczy | osobna klasa `MinimapRenderer` | `PostProcess` nie zna gry, a mapa jest zrobiona z danych gry (dopisek w [`post-process-in-game-layer.md`](post-process-in-game-layer.md)) |
| rysowana po przebiegu składającym | wykonawczy | `drawMinimap` po `composite` | mgła, bloom i mapowanie tonów są zrobione dla sceny. Szczegóły w [`minimap-srgb-constants-after-composite.md`](minimap-srgb-constants-after-composite.md) |
| kolory jako stałe sRGB, bez konwersji | wykonawczy | stałe `MINIMAP_*_COLOR` | [`minimap-srgb-constants-after-composite.md`](minimap-srgb-constants-after-composite.md) |
| lista wierzchołków odbudowywana i wysyłana co klatkę | wykonawczy | `drawMinimap`, `Buffer::setData` | [`minimap-vertices-rebuilt-every-frame.md`](minimap-vertices-rebuilt-every-frame.md) |
| mapa widoczna w obu widokach diagnostycznych | wykonawczy | `drawMinimap` jest po `composite` | jak HUD, ustawienia przebiegu składającego jej nie dotyczą |

**Ograniczenie, które minimapa odziedziczyła i ominęła.** Mgła liczy wysokość w miejscu piksela i z góry zakrywa labirynt prawie w całości ([`fog-height-at-the-pixel.md`](fog-height-at-the-pixel.md)). Widok minimapy nie przechodzi przez przebieg składający, więc tego ograniczenia nie ma: schemat nie jest widokiem sceny.

**Czego nie wiadomo.** Kod ma zgłoszoną bramkę (414 przypadków testowych, 138711 asercji) i siedmiosekundowy start bez błędów w `stderr`. **Nikt nie oglądał obrazu**: kolory, grubości, rozmiar strzałki, czytelność w dużym labiryncie i położenie względem HUD nie są ocenione. Nie ćwiczono ruchu gracza, klawisza M, zakładki Minimap, `Reveal all`, `Reload shaders` ani zmiany rozmiaru okna. Lista do wykonania: [`../guides/build-windows.md`](../guides/build-windows.md), sekcja 22.

## Dopisek, 2026-10-06 (M8, część 2): ściany z labiryntu rundy

Decyzja o odkrywaniu przez linię wzroku wzdłuż korytarzy nie zmieniła się. Zmieniło się to, **z czego** funkcje czytają ściany. Odkrywanie (`discoverAround`) i minimapa (`buildMinimapVertices`) czytają teraz `roundMaze(world, round)`: kopię labiryntu, którą runda robi w `startRound` i z której `pullRoundLever` usuwa ścianę otwartą dźwignią (po obu stronach). Dwa skutki dla tej decyzji:

- **Ściana otwarta dźwignią zmienia linię wzroku.** Reguła czyta ściany w każdym wywołaniu i niczego nie zapamiętuje (sekcja 4), więc od kroku po pociągnięciu odkrywanie przechodzi przez otwór i odkrywa korytarz za nim. Minimapa przestaje rysować tę ścianę. Ta własność, która do M8 była tylko testem (`a wall that is removed later opens the view from the next call on`), jest od teraz używana w grze.
- **Nowa runda znowu ma każdą ścianę.** Zmienia się kopia rundy, nie `MazeWorld::maze`. Wybór kopii opisuje [`round-keeps-own-maze-copy.md`](round-keeps-own-maze-copy.md).

Doszły też kwadraty dźwigni i kartek na mapie (rysowane tylko w pokazanych komórkach), co nie zmienia zasady "mapa pokazuje to, co gracz odkrył". Obraz minimapy widział po raz pierwszy agent, który pisał kod M8, części 2, na zrzutach ekranu (widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela): ściana znikająca z mapy po pociągnięciu dźwigni i powrót mapy do stanu początkowego po klawiszu R. Właściciel mapy nie oglądał, więc zdanie na górze noty o oglądaniu obrazu pozostaje prawdziwe w odniesieniu do niego.

## 5. Kiedy wrócić do tej decyzji

- Gdyby testy z graczami pokazały, że bez widoku całości gra jest za trudna albo za długa.
- Gdyby obejrzenie mapy pokazało, że linia wzroku wzdłuż korytarzy odkrywa za dużo (długi korytarz na całą szerokość labiryntu) albo za mało (odnogi zostają nieznane aż do wejścia w linię). Reguła jest decyzją właściciela, ale parametr (zasięg) byłby tylko dodatkiem.
- Gdyby w labiryncie 40 x 40 mapa okazała się nieczytelna (komórka około 5 pikseli przy domyślnym rozmiarze): wtedy rozmiar i minimalne rozmiary kształtów są pierwszymi pokrętłami.
