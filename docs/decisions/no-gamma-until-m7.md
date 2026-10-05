# Gamma: bez korekcji gamma i bez tekstur sRGB do M7

Data: 2026-10-05. Stan: zastąpiona 2026-10-05 przez [`gamma-linear-pipeline.md`](gamma-linear-pipeline.md). Stan dzisiejszy opisują [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) i [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md).
Kod, którego dotyczyła: [`assets/shaders/lit.frag`](../../assets/shaders/lit.frag), [`assets/shaders/gouraud.frag`](../../assets/shaders/gouraud.frag), [`src/gfx/Texture2D.cpp`](../../src/gfx/Texture2D.cpp), ustawienia świateł w [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp). Dokumenty modułów: [`../modules/scene/lights.md`](../modules/scene/lights.md), sekcja 2.8, [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md), sekcja 4.2, [`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 2.10.

## Co zastąpiło tę decyzję (M7, część pierwsza)

Ta notatka opisywała stan przejściowy z M4 do M6 i sama zapowiadała swój koniec: "korekcja gamma dojdzie w M7, razem z rysowaniem do framebuffera HDR i mapowaniem tonów, jako jedna zmiana całego końca potoku". Pierwsza część M7 zrobiła dokładnie to.

**Co jest dziś.**

- Tekstury koloru (ściany, brama, kryształy, ziemia, sześć ścian nieba) mają format `GL_SRGB8` i karta dekoduje je przy odczycie. Mapy normalnych zostają w `GL_RGB8`. O formacie decyduje obowiązkowy argument `gfx::ColorSpace` konstruktorów `Texture2D` i `Cubemap` oraz funkcji `AssetCache::texture`.
- Shadery sceny liczą na wartościach liniowych i zapisują wynik do bufora `GL_RGBA16F` (`game::PostProcess`, `gfx::Framebuffer`), także wtedy, gdy przekracza 1.
- Ostatni przebieg klatki (`post/composite.frag`) mnoży przez ekspozycję, stosuje mapowanie tonów (domyślnie ACES) i koduje wynik funkcją `linearToSrgb`. `GL_FRAMEBUFFER_SRGB` zostaje wyłączony.
- Kolory wpisane liczbami (światła, tło, linie kolizji, gradient trawy) są liczbami sRGB i są przeliczane raz, każdy w jednym miejscu.
- Wartości startowe świateł, świecenia kryształów, jasności nieba i koloru tła zostały dobrane od nowa, tak jak przewidywała sekcja 4 tej notatki.

**Co z rozumowania tej notatki nadal obowiązuje.**

- **"Dlaczego nie połowa"** (sekcja 4): samo kodowanie na końcu bez dekodowania tekstur daje obraz wyprany. To dziś jedna z pułapek w dokumencie modułu i powód, dla którego dekodowanie i kodowanie weszły w jednej zmianie.
- **Mapy normalnych nigdy nie są sRGB** (sekcja 5, trzeci punkt): format jest wybierany dla każdej tekstury osobno, dokładnie tak, jak notatka wymagała.
- **Obawa o ImGui** (tabela w sekcji 3, wiersz o `GL_FRAMEBUFFER_SRGB`): kolory paneli rysowanych do tego samego okna zostałyby rozjaśnione. To jest dziś uzasadnienie notatki [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md).
- **"Co zostaje poprawne"** (sekcja 4): geometria światła i różnice między trybami cieniowania nie zależały od gammy i nie zmieniły się.

**Co przestało być prawdą.** Wszystko, co sekcje 1 i 2 mówią o stanie kodu w czasie teraźniejszym: tekstury koloru nie mają już formatu `GL_RGB8`, `lit.frag` nie zapisuje wyniku na ekran, a jego komentarz mówi dziś co innego. Lista strat z sekcji 4 ("Co przez to tracę") opisuje stan sprzed M7. Zdanie "nie porównywałem obrazu z gammą i bez niej" ma dziś odpowiedź: zgłoszone porównanie jest w [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md), w części o porównaniu ze starym potokiem.

Nowe decyzje: [`gamma-linear-pipeline.md`](gamma-linear-pipeline.md) (potok jako całość), [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md) (gdzie stoi kodowanie), [`aces-default-tone-mapping.md`](aces-default-tone-mapping.md) (krzywa mapowania tonów). Reszta tej notatki zostaje bez zmian, jako historia.

## 1. Kontekst

Liczby zapisane w pliku PNG nie są proporcjonalne do jasności światła. Są zakodowane w przestrzeni **sRGB**: krzywą, która w przybliżeniu podnosi jasność do potęgi 1/2,2, żeby 256 poziomów na kanał było rozłożonych tak, jak widzi oko (gęściej w ciemnych tonach). Monitor robi odwrotność. Dopóki program tylko przepisuje kolor z tekstury na ekran, obie krzywe się znoszą i obraz jest poprawny. Tak było do M3.

Od M4 program **liczy** na kolorach: mnoży teksturę przez światło i sumuje światła. Te działania są fizycznie poprawne tylko na wartościach liniowych (proporcjonalnych do jasności). Poprawny potok wygląda tak:

1. tekstura koloru jest odczytywana z zamianą sRGB na liniowe (format wewnętrzny `GL_SRGB8`: karta robi to sama przy próbkowaniu),
2. shader liczy światło na wartościach liniowych,
3. wynik jest zamieniany z liniowego na sRGB przy zapisie (`glEnable(GL_FRAMEBUFFER_SRGB)` z framebufferem sRGB albo `pow(kolor, 1/2,2)` na końcu shadera).

W kodzie M4 żadnego z tych kroków nie ma. Tekstury mają format `GL_RGB8` albo `GL_RGBA8`, w źródłach nie występuje ani `GL_SRGB8`, ani `GL_FRAMEBUFFER_SRGB`, a `lit.frag` zapisuje wynik bez przeliczenia. Komentarz w `lit.frag` mówi to wprost.

Trzeba było zdecydować: wprowadzić korekcję gamma razem z oświetleniem, czy później.

## 2. Decyzja

W M4 **nie ma korekcji gamma i nie ma tekstur sRGB**. Wartości z tekstur są używane tak, jak leżą w pliku, a wynik rachunku światła jest zapisywany tak, jak wyszedł. Korekcja gamma dojdzie w M7, razem z rysowaniem do framebuffera HDR i mapowaniem tonów, jako jedna zmiana całego końca potoku.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Bez gammy do M7 (wybrana)** | shadery tematów 6 i 7 zawierają tylko to, czego dotyczą te tematy. Jedna zmiana końca potoku w M7 zamiast dwóch. Żadnej różnicy między systemami w tym, jak okno traktuje sRGB | rachunek światła jest na wartościach nieliniowych: półcienie są za ciemne, przejścia jasności za ostre. Ustawienia świateł dobrane dziś trzeba będzie dobrać od nowa w M7 |
| `pow(kolor, 1/2,2)` na końcu `lit.frag` i `gouraud.frag`, tekstury bez zmian | jedna linia w dwóch plikach | połowa poprawki: wynik jest kodowany, ale tekstura nadal wchodzi do rachunku nieliniowa, więc cała scena robi się wyblakła i za jasna. Gorzej niż brak korekcji |
| `pow` na końcu i `pow(tekstura, 2,2)` na początku shadera | poprawny rachunek bez zmian w C++ | dwie potęgi na fragment liczone ręcznie, w czterech plikach shaderów (`textured.frag` też, żeby tryb `Unlit` wyglądał tak samo). Filtrowanie tekstur i mipmapy nadal działają na wartościach nieliniowych. W M7 i tak trzeba to usunąć |
| tekstury `GL_SRGB8` i `GL_FRAMEBUFFER_SRGB` na domyślnym framebufferze | poprawnie i bez kodu w shaderach: konwersję robi karta | wymaga, żeby domyślny framebuffer okna był sRGB. To zależy od systemu i sterownika (wskazówka `GLFW_SRGB_CAPABLE`), a projekt działa na macOS i Windowsie. Kolory paneli ImGui, rysowanych do tego samego framebuffera, zostałyby rozjaśnione i motyw z policzonym kontrastem przestałby się zgadzać. Biała tekstura zastępcza i kolor tła też zmieniają znaczenie |
| pełny potok od razu: własny framebuffer HDR, mapowanie tonów, gamma | docelowe rozwiązanie | to temat 10 wykładu i zakres M7 (rendering pozaekranowy). Wciągnięcie go do M4 miesza trzy tematy w jednym kamieniu milowym |

## 4. Uzasadnienie i skutki

**Dlaczego nie teraz.** Korekcja gamma nie jest częścią tematów 6 i 7: wzory Lamberta, Phonga i Blinna-Phonga, tłumienie i stożek są takie same z gammą i bez niej. Poprawne miejsce na konwersję to koniec potoku, a ten koniec zmieni się w M7 w całości: scena będzie rysowana do tekstury zmiennoprzecinkowej, a na ekran trafi przez przebieg post-processingu (PRD, potok renderowania: HDR FBO, bloom, mapowanie tonów). Konwersja na sRGB jest naturalnym ostatnim krokiem tego przebiegu. Dodana dziś do `lit.frag`, musiałaby być w M7 stamtąd usunięta, a ImGui i tak wymagałoby osobnego potraktowania.

**Dlaczego nie połowa.** Samo `pow` na końcu shadera jest najczęstszym błędem przy pierwszym podejściu do gammy: obraz robi się jaśniejszy i wygląda "lepiej", ale jest wyblakły, bo tekstura została zdekodowana przez monitor, a nie przez program. Wolę stan konsekwentnie niepoprawny, który umiem nazwać, niż niekonsekwentny.

**Co przez to tracę.**

- Światła sumują się na wartościach nieliniowych. Widać to jako zbyt ciemne półcienie i zbyt gwałtowne przejście od ciemnego do jasnego na brzegu plamy latarki i na krzywej tłumienia.
- Odbłyski wyglądają na mniejsze i ostrzejsze, niż wynika z wykładnika.
- Wartości startowe w `game::LightingSettings` (światło otoczenia, intensywności, promienie) są dobrane **do tego stanu**. Po wprowadzeniu gammy scena będzie wyraźnie jaśniejsza w ciemnych partiach i trzeba będzie je dobrać od nowa.
- To samo dotyczy liczb z M5: siła własnego świecenia kryształów (`CRYSTAL_GLOW_STRENGTH`, składnik `uEmissive`) i głębokość ich pulsowania są dobrane do obrazu bez gammy i bez HDR, w którym kolor jaśniejszy niż 1 jest po prostu obcinany.
- Wyniku nie da się porównać liczbowo z poprawnym rendererem: "5 procent jasności na promieniu" to 5 procent wartości w rachunku, a nie 5 procent jasności na ekranie.

**Co zostaje poprawne.** Geometria światła: które strony ścian są oświetlone, gdzie jest plama latarki, gdzie jest odbłysk, różnice między Gouraudem a Phongiem i między Phongiem a Blinnem-Phongiem. To jest treść tematów 6 i 7 i to pokazuję na obronie. Tryb `Unlit` (sam odczyt tekstury) jest poprawny tak jak w M3.

**Czego nie zmierzyłem.** Nie porównywałem obrazu z gammą i bez niej: wersji z gammą nie ma. Opis skutków wynika z teorii, a nie ze zrzutów ekranu.

## 5. Kiedy wrócić do tej decyzji

- W M7, przy framebufferze HDR: wtedy tekstury koloru dostają format sRGB, a ostatni przebieg koduje wynik. To zaplanowany koniec tej notatki.
- Wcześniej, gdyby prowadzący wymagał poprawnej gammy już przy temacie 6. Najmniejsza poprawna zmiana to tekstury koloru w `GL_SRGB8` (parametr przy tworzeniu `gfx::Texture2D`) i kodowanie na końcu trzech shaderów fragmentów labiryntu, z ponownym dobraniem świateł.
- Mapy normalnych (druga część M4, już zrobione: [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcja 3 i pułapka 3) mają odwrotną stronę tej decyzji: mapa normalnych przechowuje kierunki, a nie kolory, i **nigdy** nie może być teksturą sRGB. Gdy w M7 tekstury koloru staną się sRGB, format musi być wybierany dla każdej tekstury osobno.
