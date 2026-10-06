# Głębia bufora sceny jako tekstura, a nie renderbuffer

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/gfx/Framebuffer.hpp`](../../src/gfx/Framebuffer.hpp), [`Framebuffer.cpp`](../../src/gfx/Framebuffer.cpp) (`create`, `createAttachmentTexture`, `bindDepthTexture`), [`src/game/PostProcess.cpp`](../../src/game/PostProcess.cpp) (`beginScene`, `drawPreviews`), [`assets/shaders/post/preview.frag`](../../assets/shaders/post/preview.frag), [`assets/shaders/common/depth.glsl`](../../assets/shaders/common/depth.glsl). Dokumenty modułów: [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md), [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md).

## 1. Kontekst

Scena jest rysowana do własnego framebuffera. Żeby test głębi działał, framebuffer potrzebuje załącznika głębi. OpenGL pozwala dołączyć w tym miejscu dwa rodzaje obiektów:

- **renderbuffer** (`glGenRenderbuffers`, `glRenderbufferStorage`, `glFramebufferRenderbuffer`): pamięć, do której można tylko rysować. Shader nie może jej przeczytać.
- **teksturę** (`glTexImage2D` z formatem `GL_DEPTH_COMPONENT24`, `glFramebufferTexture2D`): pamięć, do której można rysować i którą późniejszy przebieg może próbkować jak każdą inną teksturę.

Większość samouczków pokazuje renderbuffer, bo w pierwszym przykładzie głębi nikt potem nie czyta. Tu było wiadomo z góry, że ktoś będzie: PRD przewiduje mgłę "liczoną w post-processie z bufora głębi", podgląd załączników w kategorii Post process i mapy cieni, które są samą głębią.

## 2. Decyzja

Klasa `gfx::Framebuffer` dołącza głębię **zawsze jako teksturę** `GL_DEPTH_COMPONENT24`, z filtrem `GL_NEAREST`. Renderbufferów w projekcie nie ma w ogóle: klasa zna jeden rodzaj załącznika.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Tekstura głębi `GL_DEPTH_COMPONENT24` (wybrana)** | głębię sceny może przeczytać shader. Już dziś robi to podgląd głębi w kategorii Post process (`preview.frag` przez `bindDepthTexture`). Jeden rodzaj załącznika w klasie: ta sama funkcja `createAttachmentTexture` tworzy kolor i głębię. Ta sama droga posłuży mapom cieni | teksturze trzeba ustawić parametry odczytu (filtr, zawijanie, `GL_TEXTURE_MAX_LEVEL`), których renderbuffer nie ma. Sterownik może optymalizować renderbuffer lepiej, bo wie, że nikt go nie czyta. Tej różnicy nie mierzyłem |
| Renderbuffer `GL_DEPTH_COMPONENT24` | najprostszy poprawny załącznik głębi, tak jak w większości przykładów | brak odczytu w shaderze: podgląd głębi i mgła wymagałyby i tak przejścia na teksturę, czyli drugiej ścieżki w klasie albo przeróbki |
| Oba rodzaje, wybierane polem w `FramebufferSpec` | każdy cel dostaje to, czego potrzebuje | dwie ścieżki tworzenia, wiązania i sprzątania dla korzyści, której dziś nikt nie potrzebuje. Więcej kodu do wytłumaczenia |
| Tekstura `GL_DEPTH24_STENCIL8` | głębia i szablon w jednym załączniku | szablonu gra nie używa. Shader czytałby tylko część wartości, a format komplikuje odczyt |
| Tekstura `GL_DEPTH_COMPONENT32F` | większa precyzja głębi | 24 bity to zwykła precyzja głębi okna (tak mówi komentarz przy `DepthFormat::Depth24`), a do M6 scena była rysowana właśnie do okna, z płaszczyznami 0,1 m i 100 m. Nie mam objawu, który większa precyzja miałaby usunąć |

## 4. Uzasadnienie i skutki

**Dlaczego tekstura.** Bo ma dziś czytelnika i będzie miała następnych. `PostProcess::drawPreviews` wiąże teksturę głębi sceny i rysuje z niej obraz odległości. Mgła i cienie są w planie M7 i obie czytają głębię. Zaczynanie od renderbuffera oznaczałoby przeróbkę klasy w następnej części tego samego kamienia milowego.

**Dlaczego bez renderbufferów w ogóle.** Klasa, która zna jeden rodzaj załącznika, jest krótsza i ma jedną ścieżkę błędu. Renderbuffer daje korzyść przy wielopróbkowaniu (MSAA) i przy buforach, których nikt nie czyta. Pierwszego gra nie ma, drugie to dziś zero przypadków.

**Skutki, które przyjmuję.**

- Tekstura głębi ma filtr `GL_NEAREST`: średnia z głębi dwóch powierzchni nie jest głębią żadnej z nich. Shader czyta ją przez zwykły `sampler2D` jako jedną liczbę w kanale czerwonym, od 0 (bliska płaszczyzna) do 1 (daleka).
- Wartość w teksturze nie jest odległością. Jest nieliniowa: przy płaszczyznach 0,1 m i 100 m ściana odległa o 2 m ma już 0,951. Każdy shader, który chce metrów, musi ją przeliczyć funkcją `linearDepth` z `common/depth.glsl`.
- Nie wolno czytać tej tekstury w przebiegu, który rysuje do framebuffera, do którego jest dołączona (pętla sprzężenia zwrotnego, wynik nieokreślony). Podglądy rysują do własnych framebufferów, więc reguła jest zachowana. Mgła liczona w przebiegu `composite` też ją zachowa, bo celem jest wtedy okno.
- Rozmiar tekstury głębi idzie za rozmiarem okna: przy zmianie rozmiaru jest usuwana i tworzona od nowa razem z teksturą koloru.
- Ścieżka "tylko głębia" (`ColorFormat::None`, czyli `glDrawBuffer(GL_NONE)` i `glReadBuffer(GL_NONE)`) jest napisana z myślą o mapach cieni, ale **nigdy nie była wykonana**: żaden kod jej dziś nie woła. **Dopisek z 2026-10-05 (M7, część czwarta):** już jest wykonywana. `game::ShadowMap::beginDepthPass` tworzy tą ścieżką mapę cieni księżyca (2048 x 2048 albo 1024 x 1024), a build Debug na Windowsie nie zgłosił przy tym błędu OpenGL (zgłoszone). Na macOS nikt jej nie uruchomił.

**Czego nie zmierzyłem.** Nie porównywałem szybkości tekstury głębi z renderbufferem. Zgłoszony koszt całej zmiany M7 na Windowsie (z około 2700 do 2500 klatek na sekundę w 1280 x 720) obejmuje wszystko naraz. Na macOS framebuffer z tym zestawem formatów nie był jeszcze tworzony.

## 5. Kiedy wrócić do tej decyzji

- Gdy dojdzie wielopróbkowanie (MSAA): załączniki wielopróbkowe to inny rodzaj tekstury albo renderbuffer, a obraz trzeba potem rozwiązać do zwykłej tekstury.
- Gdyby profilowanie na MacBooku w 1440p pokazało, że odczytywalna głębia kosztuje wyraźnie więcej niż renderbuffer.
- Przy mapach cieni: pierwsze wykonanie ścieżki bez koloru. Mapa cieni chce też innych parametrów odczytu (porównanie głębi w samplerze, obramowanie), więc `createAttachmentTexture` może wymagać rozszerzenia. **Dopisek z 2026-10-05 (M7, część czwarta):** mapa cieni księżyca powstała i `createAttachmentTexture` nie wymagało zmiany. Porównanie głębi i obramowanie trafiły do osobnego obiektu samplera, `gfx::ComparisonSampler`, bo ta sama tekstura głębi jest czytana także bez porównania, przez podgląd ([`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.7). Uściślenie do zdania z części 4: mapa cieni nie czyta głębi **sceny**, jest własną teksturą głębi w osobnym framebufferze. Korzysta z tej samej decyzji (głębia jako tekstura), a nie z tej samej tekstury.
- Gdyby potrzebny był bufor szablonu (na przykład do obrysu wybranego obiektu w M8).
