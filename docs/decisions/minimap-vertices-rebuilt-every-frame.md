# Lista wierzchołków minimapy jest budowana i wysyłana co klatkę

Data: 2026-10-06. Stan: obowiązuje (wybór wykonawczy, nie decyzja właściciela).
Kod: [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawMinimap`), [`src/game/Minimap.cpp`](../../src/game/Minimap.cpp) (`buildMinimapVertices`), [`src/game/MinimapRenderer.cpp`](../../src/game/MinimapRenderer.cpp) (`drawMap`), [`src/gfx/Buffer.*`](../../src/gfx/) (`usage`, `setData`). Dokument modułu: [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md), sekcje 2.7 i 5.5.

## 1. Kontekst

Mapa to lista trójkątów: podłogi odkrytych komórek, ściany, brama, kryształy i strzałka gracza. Zmienia się na kilka sposobów: strzałka porusza się i obraca **w każdej klatce**, odkrycie rośnie w trakcie marszu, kryształ znika po zebraniu, brama zmienia kolor, nowy labirynt zmienia wszystko, a ściana usunięta w środku rundy otwiera widok. Dotychczasowe `gfx::Buffer` w grze były wypełniane raz (siatki modeli, `GL_STATIC_DRAW`).

## 2. Decyzja

`drawMinimap` buduje **całą listę od nowa na procesorze w każdej klatce** (`buildMinimapVertices`, plain data pod testami) i `MinimapRenderer::drawMap` kopiuje ją do bufora jednym wywołaniem `glBufferData` (przez `gfx::Buffer::setData`). Bufor jest tworzony z podpowiedzią `GL_DYNAMIC_DRAW`.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **odbudować wszystko co klatkę, `glBufferData` (wybrane)** | nic nie można zapomnieć: odkrycie, kryształ, brama, nowy labirynt i zmiana ściany pokazują się same. Jedna ścieżka dla pierwszego i kolejnych wypełnień, rozmiar może być inny | procesor liczy listę i karta dostaje kopię nawet wtedy, gdy zmieniła się tylko strzałka. Około 1400 wierzchołków po 20 bajtów dla pokazanego w całości labiryntu 10 x 10 (komentarz w kodzie), czyli około 28 KB na klatkę |
| statyczna lista labiryntu i osobna mała lista strzałki | większość danych idzie na kartę rzadko | dwa bufory i dwa wywołania rysujące. Zmiany odkrycia, kryształów i bramy trzeba ręcznie wykrywać i wtedy odbudowywać, a każda zapomniana zmiana to błąd widoczny na ekranie |
| `glBufferSubData` na buforze przydzielonym z zapasem | zapis w istniejącej pamięci | nie powiększa bufora, a lista zmienia rozmiar: potrzebny zapas, przydział większego bufora i dodatkowa logika. `UniformBuffer` używa `glBufferSubData` poprawnie, bo jego rozmiar jest stały |
| mapowanie bufora (`glMapBuffer`) | brak kopiowania pośredniego | to moja analiza, kod nie podaje powodu: dochodzi stan do pilnowania (odmapowanie), a przy liście około 28 KB nie ma czego zyskać. Nie mierzono |
| osobny czytnik odkrycia w shaderze (tekstura odkrycia, jeden prostokąt na cały labirynt) | stały, mały bufor | schemat przestaje być listą kształtów, a ściany, kryształy i strzałka wymagałyby i tak własnej obsługi. Odbiega od decyzji o schemacie z danych |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Powód z komentarza w kodzie: strzałka porusza się cały czas, więc i tak trzeba wysyłać coś co klatkę, a przy odbudowie wszystkiego **nic nie da się zapomnieć**. Procesor liczy około 1400 wierzchołków (kilka tysięcy prostych operacji), co jest drobiazgiem przy reszcie klatki.

**Co dzięki temu mam.**

- `buildMinimapVertices` jest zwykłą funkcją bez OpenGL i testy sprawdzają to, co mapa pokaże, bez okna (19 przypadków w `tests/MinimapTests.cpp`),
- zmiany bufora w `gfx::Buffer` są małe: czwarty argument z domyślną wartością (reszta kodu bez zmian) i jedna metoda.

**Czego to kosztuje i czego nie wiem.**

- **Nie mierzono**, ile czasu zajmuje budowa listy i kopia. Zgłoszona liczba klatek dla tej części nie istnieje: lista kontrolna ma punkt "FPS z mapą i bez niej w jednej sesji" ([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 22.2),
- w największym labiryncie z panelu (40 x 40) lista przy `Reveal all` ma, według mojego przybliżonego liczenia, około 9600 wierzchołków podłóg i około 10100 ścian (labirynt doskonały: 1681 ścian po 6), razem około 20 tysięcy, czyli około 390 KB na klatkę. To około czternaście razy więcej niż w labiryncie 10 x 10. Nikt tego nie sprawdzał,
- `glBufferData` przydziela nową pamięć w każdej klatce. Sterownik może to obsłużyć tanio albo drogo. To jest powód podpowiedzi `GL_DYNAMIC_DRAW`, ale podpowiedź niczego nie gwarantuje.

## 5. Kiedy wrócić do tej decyzji

- Gdyby pomiar w jednej sesji pokazał, że mapa kosztuje zauważalnie: wtedy druga możliwość (statyczny labirynt i osobna mała lista strzałki, odbudowa po zdarzeniu) jest pierwszym krokiem.
- Gdyby labirynty miały być znacznie większe niż 40 x 40.
- Gdyby mapa dostała więcej ruchomych kształtów (na przykład przeciwnik, [`enemy-after-m5.md`](enemy-after-m5.md)): wtedy lista i tak zmieniałaby się co klatkę.
