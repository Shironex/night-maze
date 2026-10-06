# Minimapa omija przebieg składający i zapisuje stałe sRGB wprost

Data: 2026-10-06. Stan: obowiązuje (wybór wykonawczy, nie decyzja właściciela).
Kod: [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawMinimap` po `composite`), [`src/game/MinimapRenderer.cpp`](../../src/game/MinimapRenderer.cpp), [`src/game/Minimap.hpp`](../../src/game/Minimap.hpp) (stałe `MINIMAP_*_COLOR`), [`assets/shaders/post/minimap.frag`](../../assets/shaders/post/minimap.frag), [`minimap_overlay.frag`](../../assets/shaders/post/minimap_overlay.frag). Dokument modułu: [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md), sekcje 2.8 i 2.9.

## 1. Kontekst

Od pierwszej części M7 kolory sceny są liczone liniowo w buforze `GL_RGBA16F`, a **jedno miejsce** na końcu klatki, `post/composite.frag`, stosuje ekspozycję, krzywą mapowania tonów i koduje wynik do sRGB ([`gamma-linear-pipeline.md`](gamma-linear-pipeline.md), [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md)). Minimapa jest schematem z danych labiryntu (decyzja właściciela, [`minimap-discovered-corridors.md`](minimap-discovered-corridors.md)): nic na niej nie jest oświetlone, nie ma mgły, nie ma jasności ponad 1. Trzeba było zdecydować, czy mapa wchodzi do potoku, czy go omija.

## 2. Decyzja

Mapa jest rysowana **po** przebiegu składającym (`drawMinimap` w `onRender`), do własnego framebuffera `GL_RGBA8`, a jej kolory to **stałe wartości sRGB** (`MINIMAP_*_COLOR`), zapisywane bez konwersji w obu przebiegach i kopiowane bez konwersji do okna. `GL_FRAMEBUFFER_SRGB` zostaje wyłączone. Mieszanie z przezroczystością odbywa się na wartościach sRGB.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **po `composite`, stałe sRGB wprost (wybrane)** | mgła, bloom, ekspozycja i krzywa tonów nie ruszają schematu. Kolor na mapie to dokładnie liczby ze stałej. Brak konwersji do pomylenia | mieszanie nakładki liczy się na wartościach sRGB, więc nie jest fizycznie poprawne (tak samo jak HUD i panele) |
| mapa jako tekstura `GL_RGBA16F` liniowa, kodowana przez `composite.frag` | jedna ścieżka koloru dla wszystkiego, mieszanie w przestrzeni liniowej | do `composite.frag` musiałaby wejść druga tekstura i warunek, a ekspozycja i krzywa zmieniłyby kolory schematu. Kolory trzeba by przeliczać z sRGB na liniowe (stałe dobrane na ekranie) i z powrotem |
| mapa rysowana do bufora sceny przed `composite` | najmniej kodu w przebiegach | mgła i winieta ją zakryją, bloom ją rozmyje, krzywa zmieni kolory. Dokładnie to, czego schemat nie ma znosić |
| tekstura `GL_SRGB8_ALPHA8` zamiast `GL_RGBA8`, kolory liniowe | OpenGL koduje i dekoduje sam | wymaga włączenia `GL_FRAMEBUFFER_SRGB` przy zapisie do tekstury i wyłączenia przy zapisie do okna, a dwa przełączenia łatwo pomylić. Kolory trzeba by wpisywać jako liniowe |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Kolory mapy są **dobrane na ekranie**, jako liczby, które ekran ma dostać. Najkrótsza droga z tych liczb do ekranu to brak konwersji. Obie możliwości z liniową mapą wymagają, żeby ktoś przeliczył kolory w jedną stronę i drugą, bez żadnego zysku, bo na mapie nie ma rachunku, który korzystałby z liniowości.

**Co dzięki temu mam.**

- kod rysujący mapę nie zna ekspozycji, krzywej ani mgły, a testy kształtów nie zależą od ustawień przebiegu składającego,
- oba widoki diagnostyczne (UV, normalne) pokazują mapę tak samo jak widok z teksturami, bo mapa jest po `composite`,
- panel Framebuffers pokazuje teksturę mapy tak, jak leży w framebufferze, bo ImGui rysuje do okna bez kodowania.

**Czego to kosztuje.**

- mieszanie `kolor * alfa + scena * (1 - alfa)` liczy się na wartościach zakodowanych. Przy przezroczystości 0,85 różnica względem mieszania liniowego jest w szczególności w ciemnych partiach sceny. **Nie porównywałem** obu wersji, a nikt nie oglądał obrazu,
- pułapka: dodanie `linearToSrgb` w `minimap_overlay.frag` albo włączenie `GL_FRAMEBUFFER_SRGB` zakoduje kolory drugi raz i mapa zbieleje (sekcja 7 dokumentu modułu),
- kolejność w `onRender` ma znaczenie: przeniesienie `drawMinimap` przed `composite` przepuściłoby mapę przez mgłę.

## 5. Kiedy wrócić do tej decyzji

- Gdyby mapa miała pokazywać coś oświetlonego (na przykład podgląd sceny z góry): wtedy potrzebny jest potok liniowy i pierwsza z odrzuconych możliwości.
- Gdyby porównanie z mieszaniem liniowym pokazało widoczną różnicę, która przeszkadza.
- Gdy dojdzie przełączanie `GL_FRAMEBUFFER_SRGB` z innego powodu (notatka [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md)): sprawdzić, czy mapa nadal dostaje te same liczby.
