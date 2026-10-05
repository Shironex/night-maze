# Moduł renderer: post-process, scena w buforze HDR, bloom i przebieg składający

Kamień milowy: M7, część pierwsza (bufor HDR, przebieg składający, ekspozycja, mapowanie tonów, poprawna gamma, podgląd załączników) i część druga (bloom: poświata wokół jasnych miejsc). Mgła, winieta, cienie i minimapa to dalsze części M7 i **nie ma ich w kodzie**. Temat wykładu: 10 (Rendering pozaekranowy).
Kod: klasa [`src/game/PostProcess.hpp`](../../../src/game/PostProcess.hpp) i [`PostProcess.cpp`](../../../src/game/PostProcess.cpp), ustawienia i matematyka bloomu w [`src/game/Bloom.hpp`](../../../src/game/Bloom.hpp) i [`Bloom.cpp`](../../../src/game/Bloom.cpp), shadery [`assets/shaders/post/composite.vert`](../../../assets/shaders/post/composite.vert), [`post/composite.frag`](../../../assets/shaders/post/composite.frag), [`post/preview.frag`](../../../assets/shaders/post/preview.frag), [`post/bright.frag`](../../../assets/shaders/post/bright.frag), [`post/blur.frag`](../../../assets/shaders/post/blur.frag), wspólne pliki [`common/color.glsl`](../../../assets/shaders/common/color.glsl) i [`common/depth.glsl`](../../../assets/shaders/common/depth.glsl), obiekt framebuffera [`src/gfx/Framebuffer.hpp`](../../../src/gfx/Framebuffer.hpp), wywołania w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`onRender`), panel [`src/debug/panels/FramebuffersPanel.cpp`](../../../src/debug/panels/FramebuffersPanel.cpp), nazwy uniformów w [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp), testy [`tests/BloomTests.cpp`](../../../tests/BloomTests.cpp).

Dlaczego ten dokument stoi w katalogu `renderer`, chociaż klasa nazywa się `game::PostProcess` i leży w `src/game/`, wyjaśniają [`README.md`](README.md) i notatka [`../../decisions/post-process-in-game-layer.md`](../../decisions/post-process-in-game-layer.md). Dokument zakłada znajomość tekstur 2D ([`../gfx/textures.md`](../gfx/textures.md)), macierzy rzutowania i testu głębi ([`../scene/camera.md`](../scene/camera.md)) oraz dyrektywy `#include` w shaderach ([`../gfx/shader-includes.md`](../gfx/shader-includes.md)). Dwa tematy mają własne dokumenty i tutaj są tylko używane: obiekt framebuffera i klasę `gfx::Framebuffer` linia po linii opisuje [`../gfx/framebuffers.md`](../gfx/framebuffers.md), a przestrzeń sRGB, wartości liniowe i całą drogę koloru przez potok opisuje [`../gfx/color-space.md`](../gfx/color-space.md).

**Stan na dziś:** scena 3D nie jest już rysowana prosto do okna. Trafia do własnego framebuffera z teksturą koloru `GL_RGBA16F` i teksturą głębi `GL_DEPTH_COMPONENT24`, a do okna przenosi ją ostatni przebieg klatki, **przebieg składający** (composite): jeden trójkąt na cały ekran, który mnoży kolor przez ekspozycję, stosuje krzywą mapowania tonów i koduje wynik do sRGB. Od drugiej części M7 między sceną a przebiegiem składającym stoi **bloom**: przebieg jasności (bright pass) wybiera z obrazu sceny światło jaśniejsze od progu, rozmycie Gaussa rozlewa je na sąsiednie piksele, a przebieg składający dodaje wynik do sceny przed ekspozycją. Wszystko to dzieje się w trzech celach `GL_RGBA16F` o połowie szerokości i połowie wysokości bufora sceny. Panel **Framebuffers** ma suwak ekspozycji, listę krzywych, przełącznik i trzy liczby bloomu, zakres podglądu głębi, rozmiary i formaty bufora sceny i celów bloomu oraz cztery podglądy: załącznika koloru, załącznika głębi, wyniku przebiegu jasności i gotowej poświaty. Programów shaderów jest dziesięć: w pierwszej części doszły `composite` i `preview`, w drugiej `bright` i `blur`.

Temat 10 wykładu jest **w trakcie**. PRD wymienia w nim: scenę w buforze HDR, post-process (bloom, mgła, winieta), minimapę i podgląd załączników. Z tej listy są: bufor HDR, przebieg składający, bloom i podgląd załączników. **Nie ma:** mgły, winiety ani minimapy. Miejsca, w których dojdą, są oznaczone w sekcji 2.16.

Zgłoszone dla Windowsa (2026-10-05) dla **pierwszej części**, nie powtórzone przy pisaniu tego dokumentu: bramka `make check` przechodziła (formatowanie, testy Debug i Release, clang-tidy), zero ostrzeżeń, wtedy 269 przypadków testowych i 102103 asercje w obu konfiguracjach. Build Debug bez błędów OpenGL przy otwartych podglądach, po zmianie rozmiaru okna na 1400 x 800 oraz po zminimalizowaniu (framebuffer 0 x 0) i przywróceniu. Liczba klatek w buildzie Release, bez synchronizacji pionowej, z ukrytymi panelami: około 2700 przed zmianą i około 2500 po niej w 1280 x 720, około 2020 przed i około 1960 po w 2560 x 1440.

Zgłoszone dla Windowsa (2026-10-05) dla **drugiej części**, też nie powtórzone przeze mnie: bramka `make check` przechodzi, zero ostrzeżeń w Debug i Release, **276 przypadków testowych i 102139 asercji** w obu konfiguracjach. Różnica to plik `tests/BloomTests.cpp`: 7 przypadków i 36 asercji, co zgadza się z policzeniem makr w pliku. Poświata jest widoczna na zrzutach ekranu wokół kryształów (w najciemniejszej i w najjaśniejszej chwili pulsu, w trybach Unlit, Gouraud i Blinn-Phong) i wokół tarczy księżyca, a gwiazdy zostają punktami. Liczba klatek w Release z ukrytymi panelami, pomiar niespokojny: 1280 x 720 od 1900 do 2450 bez bloomu i od 1500 do 2150 z bloomem, 2560 x 1440 od 1370 do 1480 bez i od 880 do 925 z bloomem. Wersji kompilatora, karty i sterownika dla żadnego z tych pomiarów nie zapisano. **Nikt jeszcze nie kliknął myszą** kontrolek panelu, nie przeciągał krawędzi okna i nie użył `Reload shaders` przy dziesięciu programach. **Na macOS ten kod nie był ani budowany, ani uruchamiany.** Otwarte obserwacje: plama latarki na ścianie nie daje poświaty nawet z metra, a poświata jest mierzona w tekselach celu o połowie rozdzielczości, więc w 1440p jest na ekranie względnie cieńsza. Szczegóły w sekcji 5.9.

## 1. Po co to jest

Do M6 każde wywołanie rysujące zapisywało piksele prosto do okna. Okno przechowuje 8 bitów na kanał, czyli liczby od 0 do 1 w 256 krokach. Wynikają z tego trzy ograniczenia, których nie da się obejść, dopóki celem rysowania jest okno:

| Ograniczenie okna | Skutek | Co daje własny bufor |
|---|---|---|
| wartości powyżej 1 są obcinane w chwili zapisu | kryształ dwa razy jaśniejszy od bieli i kryształ dziesięć razy jaśniejszy wyglądają tak samo, a informacja "o ile jaśniejszy" znika na zawsze | tekstura zmiennoprzecinkowa `GL_RGBA16F` przechowuje wartość taką, jaka wyszła z rachunku światła |
| obrazu sceny nie da się przeczytać jak tekstury | żaden efekt nie może pracować na gotowym obrazie: rozmycie, poświata, mgła z głębi | kolor i głębia sceny są zwykłymi teksturami, które następny przebieg czyta przez `sampler2D` |
| to, co zapisano, jest od razu tym, co widać | kodowanie do sRGB trzeba by robić w każdym shaderze sceny osobno | jedno miejsce na końcu klatki, w którym obraz jest dopasowywany do ekranu |

Rysowanie do tekstury zamiast do okna nazywa się **renderingiem pozaekranowym** (offscreen rendering). To temat 10 wykładu. Dziś daje on pięć rzeczy:

| Rzecz | Gdzie | Sekcja |
|---|---|---|
| framebuffer sceny z kolorem HDR i głębią jako teksturami | `gfx::Framebuffer`, tworzony przez `PostProcess::beginScene` | 2.1, 2.2, 5.4, klasa w [`../gfx/framebuffers.md`](../gfx/framebuffers.md) |
| przebieg składający: ekspozycja, mapowanie tonów, kodowanie do sRGB | `PostProcess::composite`, shadery `post/composite.*` | 2.4 do 2.7, 4.1, 4.2, 5.6 |
| poprawna gamma w całym potoku | tekstury sRGB, kolory wpisane liczbami przeliczane raz, kodowanie na końcu | 2.3, całość w [`../gfx/color-space.md`](../gfx/color-space.md) |
| podgląd obu załączników w panelu | `PostProcess::drawPreviews`, shader `post/preview.frag`, panel Framebuffers | 2.10, 4.3, 5.5, 6 |
| bloom: poświata wokół tego, co w buforze jest jaśniejsze od progu (druga część M7) | `PostProcess::drawBloom`, shadery `post/bright.frag` i `post/blur.frag`, `game/Bloom.*`, dodanie w `post/composite.frag` | 2.11 do 2.15, 4.6 do 4.8, 5.10, 5.11 |

Pokaz z PRD dla tego tematu to "podgląd załączników FBO". Jest w panelu Framebuffers (sekcja 6). Od drugiej części panel pokazuje też oba kroki bloomu jako obrazy, więc efekt da się rozłożyć na oczach prowadzącego: co zostało po progu, co wyszło z rozmycia i co trafiło na ekran.

## 2. Teoria

### 2.1 Rendering pozaekranowy: scena jako tekstura

**Framebuffer** to cel rysowania: zestaw buforów, do których trafiają wyniki shadera fragmentów (kolor) i testu głębi (głębia). Okno ma swój, tak zwany **domyślny framebuffer** o numerze 0. Program może utworzyć własny obiekt framebuffera (FBO) i podpiąć do niego tekstury jako **załączniki** (attachments). Dopóki taki obiekt jest związany, te same wywołania rysujące wypełniają jego tekstury, a w oknie nic się nie zmienia.

```mermaid
flowchart LR
    S["wywołania rysujące sceny:<br/>teren, ściany, kryształy, trawa, niebo"] --> F["framebuffer sceny"]
    F --> C["załącznik koloru:<br/>tekstura GL_RGBA16F"]
    F --> D["załącznik głębi:<br/>tekstura GL_DEPTH_COMPONENT24"]
    C -->|"sampler2D uScene"| K["przebieg składający:<br/>jeden trójkąt na cały ekran"]
    K --> W["domyślny framebuffer: okno"]
    W --> I["ImGui: panele i HUD"]
```

Klatka ma więc co najmniej dwa przebiegi (passes): przebieg sceny, który rysuje do tekstur, i przebieg składający, który czyta teksturę koloru i rysuje do okna. Każdy efekt post-processingu jest kolejnym takim przebiegiem: czyta wynik poprzedniego i zapisuje do własnego celu. Schemat wyżej pokazuje sam szkielet z pierwszej części. Z bloomem, przy ustawieniach startowych, między sceną a przebiegiem składającym stoi jeszcze 13 przebiegów (sekcja 2.14), a pełna kolejność jest w sekcji 2.8.

Jedna reguła obowiązuje każdy przebieg: **nie wolno czytać tekstury, do której się właśnie rysuje**. Wynik takiej pętli zwrotnej (feedback loop) jest niezdefiniowany. Dlatego podglądy mają własne małe framebuffery, przebieg składający rysuje do okna, a rozmycie bloomu przerzuca obraz między dwoma celami (sekcja 2.14). Sam obiekt framebuffera, załączniki, kompletność i wiązanie opisuje [`../gfx/framebuffers.md`](../gfx/framebuffers.md).

### 2.2 HDR: dlaczego wartości powyżej 1 mają znaczenie

**LDR** (low dynamic range) to obraz, w którym każdy kanał mieści się między 0 a 1: tyle umie pokazać ekran. **HDR** (high dynamic range) to obraz, w którym wartości nie mają górnej granicy: liczba mówi, ile jest światła, a nie jak jasny ma być piksel ekranu.

Rachunek światła produkuje wartości powyżej 1 w sposób naturalny. Przykład z gry, kryształ:

| Krok | Czerwony | Zielony | Niebieski |
|---|---|---|---|
| `LightingSettings::pointColor`, liczby sRGB z panelu | 0,2 | 0,9 | 0,8 |
| po `gfx::srgbToLinear` (w `NightMazeApp::crystalEmissive`) | 0,033 | 0,787 | 0,604 |
| razy `CRYSTAL_GLOW_STRENGTH = 4.0` (w `crystalGlow`, przy pełnym pulsie) | 0,132 | **3,150** | **2,415** |

Ta trójka trafia do shadera jako `uEmissive`, a kolor fragmentu to `tekstura * (światło rozproszone + uEmissive) + odbłysk`. Dla jasnego teksela kryształu zielony i niebieski kanał wychodzą więc wyraźnie powyżej 1, jeszcze zanim dojdzie latarka. Do pierwszej części M7 włącznie stała wynosiła 2,5 i trójka miała wartość (0,083, 1,969, 1,510). Druga część podniosła ją do 4,0 razem z bloomem (sekcja 2.15 i notatka [`../../decisions/crystal-glow-raised-for-bloom.md`](../../decisions/crystal-glow-raised-for-bloom.md)).

Co się z taką wartością dzieje, zależy od bufora:

| Bufor | Co przechowa dla (0,132, 3,150, 2,415) | Co jest stracone |
|---|---|---|
| okno, 8 bitów na kanał | (0,132, 1, 1) | to, że zielonego jest o 30 procent więcej niż niebieskiego, i to, że oba są kilka razy jaśniejsze od bieli |
| `GL_RGBA16F` | (0,132, 3,150, 2,415) | nic |

Z wartością zachowaną w buforze da się potem zrobić trzy rzeczy, których obcięta wartość nie pozwala:

1. **Zmienić ekspozycję.** Po przyciemnieniu obrazu do jednej czwartej (ekspozycja 0,25) kryształ ma (0,033, 0,787, 0,604) i odzyskuje barwę. Z wartości (0,132, 1, 1) wyszłoby (0,033, 0,25, 0,25): szarawa plama o złej barwie.
2. **Sprowadzić zakres do ekranu krzywą**, która jasne partie ściska, zamiast je ucinać (mapowanie tonów, sekcja 2.6).
3. **Znaleźć to, co naprawdę świeci.** Bloom szuka pikseli jaśniejszych od progu (startowo 0,8) i bierze z nich to, co wystaje ponad próg. W buforze LDR biała ściana w świetle latarki i kryształ są nie do odróżnienia: obie mają 1. W buforze HDR kryształ ma kilka razy więcej, więc jego poświata jest kilka razy mocniejsza. Komentarz przy `CRYSTAL_GLOW_STRENGTH` mówi wprost, że po tej sile bloom znajduje kryształy (sekcje 2.11 do 2.15).

**Format `GL_RGBA16F`.** Każdy kanał to 16-bitowa liczba zmiennoprzecinkowa (half float): 1 bit znaku, 5 bitów wykładnika, 10 bitów mantysy. Największa wartość to około 65 tysięcy, więc dla sceny gry granicy w praktyce nie ma. Druga zaleta dotyczy ciemnych tonów: liczba zmiennoprzecinkowa ma tę samą względną dokładność blisko zera co blisko jedynki, a bajt ma w całym zakresie 256 równych kroków. Nocna scena żyje w wartościach liniowych rzędu 0,001 do 0,05 (sekcja 2.6), czyli tam, gdzie bajt miałby kilka poziomów. Cena: 8 bajtów na piksel zamiast 4, czyli 7,0 MiB dla 1280 x 720 i 28,1 MiB dla 2560 x 1440 (sam załącznik koloru, policzone z rozmiaru, nie zmierzone na karcie).

### 2.3 Gamma i przestrzeń sRGB: gdzie to jest opisane

Bufor sceny przechowuje wartości **liniowe**: proporcjonalne do ilości światła. Pliki obrazów, kolory z próbnika w panelu i ekran używają wartości **zakodowanych w sRGB**. Zamiana między nimi (potocznie "korekcja gamma") ma w tej części trzy etapy: karta dekoduje tekstury koloru przy odczycie (`GL_SRGB8`), shadery liczą na wartościach liniowych, a przebieg składający koduje wynik na samym końcu.

Cała teoria jest w osobnym dokumencie, [`../gfx/color-space.md`](../gfx/color-space.md): czym jest sRGB, dokładna funkcja z dwóch kawałków i jej odwrotność z przeliczonymi liczbami, które tekstury są sRGB, a które liniowe, gdzie przeliczany jest każdy kolor wpisany liczbami, typowe błędy i porównanie z poprzednim potokiem. Tam też opisane są pliki [`src/gfx/ColorSpace.hpp`](../../../src/gfx/ColorSpace.hpp), [`ColorSpace.cpp`](../../../src/gfx/ColorSpace.cpp), [`assets/shaders/common/color.glsl`](../../../assets/shaders/common/color.glsl) i testy [`tests/ColorSpaceTests.cpp`](../../../tests/ColorSpaceTests.cpp). Komentarz `// See docs/...` na górze tych plików wskazuje ten dokument (`post-process.md`), bo kodowanie do sRGB jest ostatnim krokiem przebiegu składającego. Kto trafił tu z kodu po teorię gammy, powinien przejść do [`../gfx/color-space.md`](../gfx/color-space.md).

W tym dokumencie zostaje tylko to, co dotyczy samego końca potoku: w którym miejscu shadera stoi kodowanie i dlaczego nie robi go OpenGL (sekcja 2.7).

### 2.4 Trójkąt pełnoekranowy

Przebieg składający nie rysuje sceny. Ma uruchomić shader fragmentów dokładnie raz dla każdego piksela okna. Potrzebna jest do tego geometria, która pokrywa cały ekran. Najprostsza to **jeden trójkąt większy niż ekran**.

Ekran w przestrzeni przycinania (clip space) to kwadrat od -1 do 1 w obu osiach. Trójkąt o wierzchołkach (-1, -1), (3, -1) i (-1, 3) zawiera ten kwadrat w całości:

```text
  y
  3 +                      wierzchołek 2: (-1, 3)
    |\
    | \
    |  \
  1 +---+.                 górna krawędź ekranu
    |   | \
    |EKR| \
    |AN |   \
 -1 +---+----+             wierzchołek 0: (-1, -1), wierzchołek 1: (3, -1)
   -1   1    3   x
```

Przeciwprostokątna biegnie przez punkty (1, 1), (3, -1) i (-1, 3), więc prawy górny narożnik ekranu leży dokładnie na niej. To, co wystaje poza kwadrat, karta odrzuca przy przycinaniu: fragmenty powstają tylko wewnątrz obszaru rysowania (viewport).

Wierzchołki nie pochodzą z bufora. Shader wierzchołków liczy je z wbudowanej zmiennej `gl_VertexID`, numeru wierzchołka w wywołaniu `glDrawArrays(GL_TRIANGLES, 0, 3)`:

| `gl_VertexID` | `% 2` | `/ 2` | `vUv = (x, y)` | pozycja `vUv * 2 - 1` | gdzie to jest |
|---|---|---|---|---|---|
| 0 | 0 | 0 | (0, 0) | (-1, -1) | lewy dolny narożnik ekranu |
| 1 | 1 | 0 | (2, 0) | (3, -1) | daleko za prawą krawędzią |
| 2 | 0 | 1 | (0, 2) | (-1, 3) | daleko nad górną krawędzią |

Reszta z dzielenia przez 2 to najmłodszy bit numeru, dzielenie całkowite przez 2 to następny bit. Dwa bity dają cztery kombinacje, z których trójkąt używa trzech.

**Współrzędne tekstury wychodzą same.** `vUv` w wierzchołkach to (0, 0), (2, 0) i (0, 2), a karta interpoluje je liniowo po trójkącie. Pozycja i `vUv` są związane wzorem `pozycja = vUv * 2 - 1`, więc w punkcie ekranu (-1, -1) jest `vUv = (0, 0)`, a w punkcie (1, 1) jest `vUv = (1, 1)`. Widoczna część trójkąta ma współrzędne od 0 do 1, czyli dokładnie całą teksturę sceny. Wartości do 2 istnieją tylko na częściach, które zostały odcięte.

Odpowiedzi na cztery pytania, które padają przy tym trójkącie:

| Pytanie | Odpowiedź |
|---|---|
| Dlaczego jeden trójkąt, a nie prostokąt z dwóch? | Dwa trójkąty mają wspólną przekątną. Piksele, przez które ona przechodzi, są obsługiwane na granicy dwóch prymitywów, a karta cieniuje fragmenty w blokach 2 x 2, więc wzdłuż przekątnej część pracy wykonuje dwa razy. Jeden trójkąt nie ma szwu. Do tego trzy wierzchołki zamiast sześciu (albo czterech z indeksami) i żadnych danych do wysłania |
| Dlaczego bez bufora wierzchołków? | Trzy narożniki są stałe i dają się policzyć z numeru wierzchołka. Bufor, opis atrybutów i ich wiązanie byłyby kodem bez treści |
| Dlaczego mimo to potrzebny jest VAO? | Profil Core OpenGL nie pozwala rysować bez związanego obiektu tablicy wierzchołków: `glDrawArrays` zgłasza wtedy `GL_INVALID_OPERATION`. `PostProcess` trzyma więc pusty `gfx::VertexArray` (pole `m_triangle`) bez żadnego atrybutu i wiąże go przed rysowaniem |
| Co z głębią i z odrzucaniem ścian? | Pozycja ma `z = 0` i `w = 1`, a przebieg wyłącza test głębi (`glDisable(GL_DEPTH_TEST)`), więc głębia nie gra roli. Odrzucanie tylnych ścian (`GL_CULL_FACE`) nie jest w grze włączane nigdzie: jedyne miejsce, które tego stanu dotyka, to `GrassRenderer`, który go zapamiętuje, wyłącza i przywraca. Trójkąt ma zresztą wierzchołki w kolejności przeciwnej do ruchu wskazówek zegara, czyli jest zwrócony przodem |

### 2.5 Ekspozycja

**Ekspozycja** to jedna liczba, przez którą mnożony jest kolor sceny, zanim trafi na krzywą mapowania tonów: `color *= uExposure`. Działa jak czas naświetlania w aparacie: scena zawiera tyle światła, ile zawiera, a ekspozycja decyduje, jaki wycinek tego zakresu wypadnie w środku skali ekranu.

| `uExposure` | Co robi | W języku fotografii |
|---|---|---|
| 0,5 | o połowę mniej światła | minus 1 stopień (EV) |
| 1,0 | nic, wartość startowa | 0 |
| 2,0 | dwa razy więcej światła | plus 1 stopień |
| 4,0 | cztery razy więcej | plus 2 stopnie |

Jeden stopień to podwojenie albo połowa. Dlatego suwak `Exposure` (od 0,1 do 8) jest **logarytmiczny**: krok z 0,5 na 1 i krok z 1 na 2 mają na nim tę samą długość, tak jak dla oka są tą samą zmianą jasności.

Mnożenie stoi **przed** krzywą i musi działać na wartościach liniowych. Dwa razy więcej światła to dwa razy większa liczba tylko wtedy, gdy liczba jest proporcjonalna do światła. Pomnożenie przez 2 wartości zakodowanej w sRGB zmieniłoby jasność ponad czterokrotnie.

Wartość startowa 1,0 jest częścią wyglądu sceny: komentarz przy `PostProcessSettings::exposure` mówi, że światła są dobrane pod nią. Automatycznego doboru ekspozycji (adaptacji oka) nie ma.

### 2.6 Mapowanie tonów: trzy krzywe

Po ekspozycji kolor nadal może być większy niż 1, a ekran pokaże najwyżej 1. **Mapowanie tonów** (tone mapping) to funkcja, która sprowadza zakres od 0 do nieskończoności do zakresu od 0 do 1. Gra ma trzy do wyboru (`game::ToneMapping`), wszystkie liczone **osobno dla każdego kanału**.

**None: obcięcie.** `clamp(color, 0.0, 1.0)`. Do 1 nic się nie zmienia, powyżej wszystko staje się jedynką. Tak zachowywało się okno przed tą częścią. Jasne partie tracą szczegóły i barwę.

**Reinhard.** Wzór `x / (1 + x)`.

| x | rachunek | wynik |
|---|---|---|
| 0 | 0 / 1 | 0 |
| 1 | 1 / 2 | 0,5 |
| 4 | 4 / 5 | 0,8 |
| 100 | 100 / 101 | 0,990 |

Wynik zbliża się do 1, ale nigdy jej nie osiąga, więc nic nie jest ucinane: dwie różne jasności zawsze dają dwa różne wyniki. Cena: krzywa leży **cała pod prostą** `y = x`. Dla małych x wynik jest prawie równy x (0,01 daje 0,0099), ale środek skali jest wyraźnie ściśnięty: biel o wartości 1 staje się szarością 0,5. Obraz wychodzi ciemniejszy i bardziej płaski.

**ACES (dopasowana).** Krzywa filmowa: krótki wzór, który Krzysztof Narkowicz dopasował do krzywej odniesienia przemysłu filmowego (Academy Color Encoding System). Komentarz w shaderze podaje rok 2015, wpis na blogu autora z tym wzorem ma datę 6 stycznia 2016. To iloraz dwóch wielomianów drugiego stopnia:

```text
            x * (A * x + B)
f(x) = -------------------------      A = 2,51  B = 0,03  C = 2,43  D = 0,59  E = 0,14
         x * (C * x + D) + E
```

Przeliczone przykłady:

| x | licznik | mianownik | wynik |
|---|---|---|---|
| 0,01 | 0,01 * (0,0251 + 0,03) = 0,000551 | 0,01 * (0,0243 + 0,59) + 0,14 = 0,146143 | 0,0038 |
| 0,18 | 0,18 * (0,4518 + 0,03) = 0,086724 | 0,18 * (0,4374 + 0,59) + 0,14 = 0,324932 | 0,267 |
| 1 | 1 * (2,51 + 0,03) = 2,54 | 1 * (2,43 + 0,59) + 0,14 = 3,16 | 0,804 |
| 2,5 | 2,5 * (6,275 + 0,03) = 15,7625 | 2,5 * (6,075 + 0,59) + 0,14 = 16,8025 | 0,938 |

Krzywa ma kształt litery S i przecina prostą `y = x` w dwóch miejscach, około 0,062 i około 0,73:

| Zakres wejścia | Co robi krzywa | Przykład |
|---|---|---|
| poniżej około 0,062 | **przyciemnia**, i to mocno | 0,01 daje 0,0038, czyli niecałe 40 procent wejścia |
| od około 0,062 do około 0,73 | rozjaśnia: środek skali dostaje więcej kontrastu | 0,18 daje 0,267 |
| powyżej około 0,73 | ściska: jasne wartości łagodnie dochodzą do 1 | 2,5 daje 0,938 |

Dwie własności wzoru, o które łatwo zostać zapytanym:

- Dla bardzo dużych x wynik dąży do `A / C = 2,51 / 2,43 = 1,033`, czyli **powyżej** 1. Krzywa osiąga 1 przy x równym około 7,24 i potem ją przekracza. Dlatego w shaderze wynik jest ujęty w `clamp(..., 0.0, 1.0)`: bez tego najjaśniejsze piksele wychodziłyby poza zakres.
- Stała `E = 0,14` w mianowniku sprawia, że dla małych x ułamek jest w przybliżeniu równy `x * B / E = 0,21 * x`. Stąd mocne przyciemnienie cieni. Komentarz w `composite.frag` mówi o ciemnych tonach "przyciśniętych trochę". Liczby mówią więcej: przy 0,01 zostaje 38 procent.

**Trzy krzywe obok siebie.** Wartość liniowa po ekspozycji, wynik każdej krzywej (jeszcze przed kodowaniem do sRGB):

| Wejście | None | Reinhard | ACES |
|---|---|---|---|
| 0,01 | 0,0100 | 0,0099 | 0,0038 |
| 0,05 | 0,0500 | 0,0476 | 0,0443 |
| 0,18 | 0,1800 | 0,1525 | 0,2669 |
| 0,5 | 0,5000 | 0,3333 | 0,6163 |
| 1 | 1 | 0,5000 | 0,8038 |
| 2 | 1 | 0,6667 | 0,9149 |
| 2,5 | 1 | 0,7143 | 0,9381 |
| 4 | 1 | 0,8000 | 0,9734 |
| 8 | 1 | 0,8889 | 1 |

Kryształ z sekcji 2.2, (0,132, 3,150, 2,415), przechodzi przez ACES jako (0,184, 0,957, 0,935): zielony i niebieski są blisko bieli, ale nadal różne. Po obcięciu byłoby (0,132, 1, 1). To jest sam składnik emisyjny przy pełnym pulsie: kolor piksela na ekranie jest jeszcze pomnożony przez teksturę kryształu, która go przyciemnia, i właśnie ta różnica między jasnymi a ciemnymi tekselami daje widoczne ścianki.

**Dlaczego ACES jest krzywą domyślną i co to znaczy dla nocnej sceny.** Wybór uzasadnia notatka [`../../decisions/aces-default-tone-mapping.md`](../../decisions/aces-default-tone-mapping.md). Skutek uboczny jest ważny dla tej gry. Nocna scena jest ciemna: światło otoczenia po przeliczeniu na wartości liniowe to (0,011, 0,016, 0,041), a pomnożone przez kolor kamienia daje wartości jeszcze mniejsze. Większość pikseli leży więc **poniżej** pierwszego przecięcia 0,062, czyli tam, gdzie ACES przyciemnia. Wartość 0,01 po ACES i po kodowaniu do sRGB pojawia się na ekranie jako 0,048, a po samym obcięciu jako 0,100: połowa jasności. Dlatego wartości startowe zostały w pierwszej części M7 dobrane od nowa razem z krzywą (jedna z nich, siła świecenia kryształu, zmieniła się jeszcze raz w drugiej części):

| Ustawienie | Przed M7 | Dziś | Gdzie |
|---|---|---|---|
| światło otoczenia (liczby sRGB) | (0,035, 0,045, 0,075), używane wprost | (0,105, 0,135, 0,225), liniowo (0,011, 0,016, 0,041) | `LightingSettings::ambient` |
| intensywność księżyca | 0,3 | 0,12 | `LightingSettings::moonIntensity` |
| intensywność latarki | 1,6 | 1,3 | `LightingSettings::flashlightIntensity` |
| intensywność światła kryształów | 2,0 | 0,9 | `LightingSettings::pointIntensity` |
| siła świecenia kryształu | 1,0 | 4,0 (w pierwszej części M7: 2,5) | `CRYSTAL_GLOW_STRENGTH` |
| jasność nieba | 1,0 (suwak do 3) | 2,2 (suwak do 6) | `SkyboxSettings::brightness` |
| kolor tła (liczby sRGB) | (0,01, 0,015, 0,04) | (0,022, 0,033, 0,088) | `NightMazeApp::m_clearColor` |

Liczb sprzed M7 i dzisiejszych nie da się porównać wprost: wtedy trafiały do rachunku jako wartości nieliniowe i wynik szedł na ekran bez kodowania, dziś kolory są najpierw przeliczane na liniowe, a wynik przechodzi przez krzywą i kodowanie ([`../gfx/color-space.md`](../gfx/color-space.md)).

**Ograniczenie krzywych liczonych na kanał.** Każdy kanał jest ściskany osobno, więc bardzo jasny kolor nasycony traci nasycenie i przesuwa barwę w stronę bieli: (0,132, 3,150, 2,415) ma proporcję zielonego do niebieskiego 1,30, a po ACES 1,02. Dla świecącego kryształu to pożądany wygląd (rozżarzony środek jest prawie biały), ale jest to własność metody, a nie wierne odwzorowanie barwy.

### 2.7 Kodowanie do sRGB: ostatnia linia i wyłączone `GL_FRAMEBUFFER_SRGB`

Po mapowaniu tonów kolor jest liniowy i mieści się między 0 a 1. Ekran oczekuje liczb zakodowanych w sRGB. Ostatnia linia `composite.frag` robi tę zamianę funkcją `linearToSrgb` z `common/color.glsl`:

```glsl
    fragColor = vec4(linearToSrgb(color), 1.0);
```

To jest **jedyne** miejsce, w którym klatka jest kodowana. Kolejność trzech kroków nie jest dowolna:

| Kolejność | Dlaczego |
|---|---|
| ekspozycja przed krzywą | mnożenie przez liczbę ma sens tylko na wartościach liniowych, a krzywa ma dostać zakres już przesunięty |
| krzywa przed kodowaniem | krzywe mapowania tonów są zdefiniowane na wartościach liniowych. Kodowanie przyjmuje zakres od 0 do 1, który dopiero krzywa zapewnia |
| kodowanie na końcu | po nim liczby przestają być proporcjonalne do światła i żaden rachunek na nich nie jest już poprawny |

OpenGL umie zakodować wynik sam: po `glEnable(GL_FRAMEBUFFER_SRGB)` każdy zapis do framebuffera, który jest oznaczony jako sRGB, przechodzi przez tę samą funkcję. Gra tego **nie** używa. `PostProcess::composite` woła wręcz `glDisable(GL_FRAMEBUFFER_SRGB)`, chociaż przełącznik jest domyślnie wyłączony: linia zapisuje w kodzie, że przebieg na tym polega. Powody w skrócie:

- **ImGui.** Panele i HUD są rysowane po przebiegu składającym do tego samego okna, a ich kolory to gotowe liczby sRGB. Z włączonym przełącznikiem zostałyby zakodowane drugi raz: motyw wyszedłby rozjaśniony i wyblakły.
- **Jedno zachowanie na dwóch systemach.** To, czy framebuffer okna jest sRGB, zależy od systemu i sterownika. Kodowanie w shaderze daje te same liczby wszędzie.
- **Podwójne kodowanie.** Gdyby okno było sRGB i przełącznik był włączony, obraz zakodowany przez shader zostałby zakodowany jeszcze raz.

Pełne uzasadnienie i rozważane możliwości są w notatce [`../../decisions/srgb-encode-in-shader.md`](../../decisions/srgb-encode-in-shader.md). Całą drogę koloru od pliku do ekranu opisuje [`../gfx/color-space.md`](../gfx/color-space.md).

### 2.8 Kolejność klatki

```mermaid
flowchart TD
    A["onRender: rozmiar framebuffera okna"] --> B{"0 x 0?"}
    B -->|tak| Z["koniec klatki: nic nie jest rysowane"]
    B -->|nie| C["beginScene: framebuffer sceny jest celem,<br/>viewport na jego rozmiar"]
    C --> D["glEnable(GL_DEPTH_TEST),<br/>glClearColor z koloru liniowego, glClear"]
    D --> E["scena: drawMaze, drawGrass,<br/>linie kolizji, na końcu niebo"]
    E --> F{"settings.previews?"}
    F -->|tak| G["drawPreviews: dwa małe framebuffery,<br/>kolor i głębia"]
    F -->|nie| K
    G --> K["kopia ustawień: w widoku diagnostycznym<br/>bez ekspozycji, krzywej i bloomu"]
    K --> L{"bloom włączony<br/>w kopii?"}
    L -->|tak| M["drawBloom: przebieg jasności,<br/>rozmycie Gaussa, trzy cele o połowie rozmiaru"]
    L -->|nie| H
    M --> H["composite: okno jest celem,<br/>bloom, ekspozycja, mapowanie tonów, sRGB"]
    H --> I["main.cpp: ImGui, panele i HUD,<br/>prosto do okna"]
```

`drawBloom` jest wołane w **każdej** klatce, także przy wyłączonym bloomie: wtedy od razu wraca i zapisuje, że w tej klatce poświaty nie ma. Romb na schemacie to pierwsza linia tej funkcji, a nie `if` w `onRender`.

To samo jako lista kroków `NightMazeApp::onRender` (kod w sekcji 5.7):

| # | Krok | Cel rysowania |
|---|---|---|
| 1 | odczyt rozmiaru framebuffera okna. Przy 0 x 0 (zminimalizowane okno) cała klatka jest pomijana | brak |
| 2 | `m_postProcess.beginScene(framebuffer)`: tworzy bufor sceny przy pierwszej klatce i po zmianie rozmiaru, wiąże go i ustawia viewport. Gdy zwróci fałsz, klatka jest pomijana | framebuffer sceny |
| 3 | `glEnable(GL_DEPTH_TEST)`, kolor tła przeliczony przez `gfx::srgbToLinear`, `glClear` koloru i głębi | framebuffer sceny |
| 4 | macierze, światła, `drawMaze`, `drawGrass`, opcjonalnie `drawColliderLines` | framebuffer sceny |
| 5 | niebo, jako ostatnie wywołanie rysujące **sceny** | framebuffer sceny |
| 6 | `drawPreviews`, tylko gdy `m_postProcessSettings.previews` jest prawdą (otwarty panel Framebuffers) | dwa framebuffery podglądu |
| 7 | kopia ustawień dla widoków diagnostycznych (sekcja 2.9) | brak |
| 8 | `m_postProcess.drawBloom(...)` z tą kopią: przebieg jasności, potem rozmycie, a przy otwartym panelu dwa podglądy (sekcje 2.11 do 2.14, kod w 5.11) | trzy cele bloomu, potem dwa framebuffery podglądu |
| 9 | `m_postProcess.composite(...)`: okno staje się celem, trójkąt pełnoekranowy dodaje bloom do sceny i przenosi obraz | okno |
| 10 | po powrocie z `onRender`: `DebugUI::draw` w [`src/main.cpp`](../../../src/main.cpp) rysuje panele i HUD | okno |

Bloom stoi **po** kopii ustawień, bo to kopia mówi mu, czy ma w ogóle rysować: w widoku diagnostycznym jest w niej wyłączony. Stoi **po** podglądach sceny, ale kolejność tych dwóch kroków nie ma znaczenia dla obrazu: oba tylko czytają bufor sceny. Miejsce na przyszłe przebiegi wskazuje komentarz w `onRender` między krokami 5 i 6: efekty liczone z gotowej sceny czytają tekstury bufora sceny i rysują do własnych framebufferów, a przebieg składający zostaje ostatni.

### 2.9 Widoki diagnostyczne omijają ekspozycję, krzywą i bloom

Lista `View` w panelu Assets ma dwa widoki, które nie pokazują światła, tylko **dane jako kolor**: normalne (`normal * 0.5 + 0.5`) i współrzędne tekstury. Liczba 0,5 w kanale ma znaczyć na ekranie dokładnie 0,5. Potok po scenie by to zepsuł w trzech miejscach, więc każde z nich ma swoją poprawkę:

| Co by zepsuło wynik | Poprawka | Gdzie |
|---|---|---|
| kodowanie do sRGB: 0,5 wyszłoby na ekranie jako 0,735 | shader sceny zapisuje `srgbToLinear(dane)`, a kodowanie na końcu to znosi: `linearToSrgb(srgbToLinear(x)) = x` | `textured.frag`, `grass.frag`, `skybox.frag` |
| ekspozycja i krzywa: zmieniłyby liczby dowolnie | dla klatki w widoku diagnostycznym `onRender` robi **kopię** ustawień z ekspozycją 1 (`NEUTRAL_EXPOSURE`) i `ToneMapping::None` | `NightMazeApp::onRender` |
| bloom: jasne dane dostałyby poświatę. W widoku UV róg, w którym obie współrzędne dochodzą do 1, to żółć (1, 1, 0) o jasności 0,93, czyli powyżej progu 0,8, więc te rogi rozlewałyby się na sąsiednie piksele i fałszowały ich liczby | w tej samej kopii `bloom.enabled = false`. `drawBloom` nic wtedy nie rysuje, a `composite` nie czyta tekstury bloomu | `NightMazeApp::onRender` |

Kopia, a nie zmiana pól: suwak, lista i pole `Bloom` w panelu Framebuffers nadal pokazują to, co ustawił użytkownik, i wracają do działania po przełączeniu widoku z powrotem na `Textured`. Panel pokazuje w takiej klatce `Bloom targets: not drawn (bloom off or a debug view)` i napis `(not drawn)` zamiast dwóch obrazów bloomu, chociaż pole `Bloom` jest nadal zaznaczone. Zgłoszony wynik porównania z poprzednim commitem: widoki normalnych i UV różnią się najwyżej o 1 poziom na 255.

### 2.10 Podgląd załączników i głębia liniowa

Panel pokazuje dwa obrazy bufora sceny: załącznik koloru i załącznik głębi (od drugiej części M7 obok nich stoją dwa obrazy bloomu, sekcja 6). ImGui rysuje teksturę bez żadnego przeliczenia, a żaden z załączników nie nadaje się do pokazania wprost: kolor jest liniowy i może przekraczać 1, głębia jest jedną liczbą o bardzo nierównym rozkładzie. Dlatego `PostProcess::drawPreviews` rysuje każdy załącznik trójkątem pełnoekranowym do **własnego małego framebuffera** `GL_RGBA8` (wysokość 180 pikseli, szerokość z proporcji sceny: 320 przy 16:9), shaderem `post/preview.frag`. Panel pokazuje teksturę koloru tego małego framebuffera.

**Podgląd koloru** to samo kodowanie: `linearToSrgb(texture(uSource, vUv).rgb)`. Bez ekspozycji i bez krzywej, bo ma pokazać **zawartość bufora**, a nie gotową klatkę. Funkcja `linearToSrgb` przycina wejście do zakresu od 0 do 1, więc wszystko, co w buforze jest jaśniejsze od 1, wychodzi jako biel. Stąd tekst podpowiedzi przy obrazie `HDR colour`: `The colour attachment of the scene, cut off at 1.` (w pierwszej części M7 to zdanie stało w samym podpisie obrazu).

**Podgląd głębi** wymaga odwrócenia rzutowania. Tekstura głębi nie przechowuje odległości, tylko liczbę od 0 (płaszczyzna bliska) do 1 (płaszczyzna daleka), która zmienia się bardzo szybko blisko kamery i prawie wcale daleko. Funkcja `linearDepth` z `common/depth.glsl` zamienia ją z powrotem na metry. Wyprowadzenie krok po kroku, dla `n` (płaszczyzna bliska) i `f` (daleka):

**Krok 1: co robi macierz rzutowania.** `glm::perspective` buduje macierz, której trzeci i czwarty wiersz to (zapis jak na tablicy: wiersze, kolumny):

```text
wiersz 3:   0   0   -(f + n) / (f - n)   -2 f n / (f - n)
wiersz 4:   0   0   -1                    0
```

Punkt w przestrzeni kamery ma współrzędną `z_e`, ujemną przed kamerą (kamera patrzy wzdłuż -Z). Jego odległość od płaszczyzny kamery to `d = -z_e`. Po pomnożeniu przez macierz:

```text
z_clip = -(f + n) / (f - n) * z_e - 2 f n / (f - n)
       =  (f + n) / (f - n) * d   - 2 f n / (f - n)
w_clip = -z_e = d
```

**Krok 2: dzielenie perspektywiczne.** Karta dzieli przez `w_clip` i dostaje współrzędną znormalizowaną od -1 do 1:

```text
ndc = z_clip / w_clip = (f + n) / (f - n) - 2 f n / ((f - n) * d)
```

Odległość `d` stoi w **mianowniku**: stąd cała nieliniowość. Sprawdzenie na końcach: dla `d = n` wychodzi `(f + n - 2f) / (f - n) = -1`, dla `d = f` wychodzi `(f + n - 2n) / (f - n) = 1`.

**Krok 3: zapis do bufora.** Przy domyślnym `glDepthRange(0, 1)` (gra tego nie zmienia) do tekstury trafia `stored = (ndc + 1) / 2`, czyli zakres od 0 do 1.

**Krok 4: odwrócenie.** Najpierw z powrotem do `ndc`, potem równanie z kroku 2 rozwiązane względem `d`:

```text
ndc = 2 * stored - 1

ndc * (f - n) = (f + n) - 2 f n / d
2 f n / d     = (f + n) - ndc * (f - n)
d             = 2 f n / (f + n - ndc * (f - n))
```

To są dokładnie dwie linie funkcji `linearDepth`. Dla kamery gry (`scene::Camera`: `nearPlane = 0.1`, `farPlane = 100.0`):

| Odległość od płaszczyzny kamery | Wartość w teksturze głębi |
|---|---|
| 0,1 m (płaszczyzna bliska) | 0 |
| 0,2 m | 0,5005 |
| 1 m | 0,9009 |
| 2 m | 0,9510 |
| 5 m | 0,9810 |
| 15 m | 0,9943 |
| 100 m (płaszczyzna daleka) | 1 |

Połowa zakresu liczb jest zużyta na pierwsze 10 centymetrów za płaszczyzną bliską, a wszystko dalej niż 2 metry mieści się w ostatnich 5 procentach. Surowa głębia pokazana jako szarość byłaby więc prawie białym obrazem. Po przeliczeniu na metry shader dzieli wynik przez `uDepthRange` (suwak `Depth range`, startowo 15 m) i przycina do zakresu od 0 do 1: czerń to kamera, biel to 15 metrów i dalej.

Trzy uwagi do tego podglądu:

- **Niebo jest białe.** Skybox jest rysowany na głębi 1 ([`skybox.md`](skybox.md), sekcja 2.7), a `linearDepth(1)` zwraca dokładnie `f`, czyli 100 m, więcej niż każdy zakres suwaka poza samym końcem.
- **To nie jest odległość od oka w linii prostej**, tylko odległość od płaszczyzny kamery (wzdłuż kierunku patrzenia). Ściana prostopadła do kierunku patrzenia ma jedną szarość na całej szerokości, chociaż jej brzegi są dalej od oka niż środek.
- **Szarość nie jest kodowana do sRGB.** Komentarz w shaderze mówi dlaczego: to miara, a nie światło. Wartość 0,5 ma być na ekranie liczbą 0,5, czyli połową zakresu suwaka.

Tekstura głębi jest czytana przez zwykły `sampler2D` jako jedna liczba w kanale czerwonym, z filtrem `GL_NEAREST` ([`../gfx/framebuffers.md`](../gfx/framebuffers.md)). To, że głębia w ogóle jest teksturą, a nie renderbufferem, uzasadnia notatka [`../../decisions/depth-attachment-as-texture.md`](../../decisions/depth-attachment-as-texture.md).

### 2.11 Bloom: skąd poświata i dlaczego potrzebuje HDR

**Bloom** to poświata wokół bardzo jasnych miejsc obrazu: światło "rozlewa się" poza obrys tego, co świeci. W aparacie i w oku bierze się to stąd, że soczewka nie jest idealna i część bardzo mocnego światła rozprasza się na sąsiednie miejsca matrycy albo siatkówki. Ekran tego sam nie zrobi, bo nie umie świecić jaśniej niż własna biel. Poświata jest więc dla widza **jedynym sygnałem**, że coś jest jaśniejsze od bieli: biała plama z poświatą wygląda jak źródło światła, biała plama bez niej jak kartka papieru.

Efekt ma trzy kroki. Każdy to jeden albo kilka przebiegów trójkąta pełnoekranowego (sekcja 2.4), z tym samym shaderem wierzchołków `post/composite.vert`:

| # | Krok | Shader | Co czyta | Co zapisuje |
|---|---|---|---|---|
| 1 | **przebieg jasności** (bright pass): zostaw tylko światło ponad progiem | `post/bright.frag` | kolor sceny | `m_brightPass` |
| 2 | **rozmycie Gaussa**, powtórzone kilka razy, każde powtórzenie jako przebieg poziomy i pionowy | `post/blur.frag` | wynik poprzedniego przebiegu | na zmianę `m_blurHorizontal` i `m_bloom` |
| 3 | **dodanie** rozmytego obrazu do sceny | `post/composite.frag` | kolor sceny i `m_bloom` | okno |

```mermaid
flowchart LR
    S["kolor sceny<br/>GL_RGBA16F, pełny rozmiar"] -->|"bright.frag"| B["m_brightPass<br/>połowa rozmiaru"]
    B -->|"blur.frag, poziomo<br/>(tylko pierwsza iteracja)"| H["m_blurHorizontal"]
    H -->|"blur.frag, pionowo"| G["m_bloom"]
    G -->|"blur.frag, poziomo<br/>(kolejne iteracje)"| H
    S -->|"uScene, jednostka 0"| C["composite.frag"]
    G -->|"uBloom, jednostka 1"| C
    C --> W["okno"]
```

**Dlaczego bloom potrzebuje bufora HDR.** Przebieg jasności ma odróżnić to, co świeci, od tego, co jest tylko jasne. W buforze LDR obie rzeczy mają tę samą liczbę. Porównanie dla progu 0,8, dla jasnej ściany w świetle latarki (jasność 0,9) i dla składnika emisyjnego kryształu z sekcji 2.2 (jasność 2,455, rachunek w sekcji 2.12):

| Piksel | Jasność w buforze LDR | Nadwyżka ponad próg 0,8 | Jasność w buforze HDR | Nadwyżka ponad próg 0,8 |
|---|---|---|---|---|
| ściana w świetle latarki | 0,9 | 0,1 | 0,9 | 0,1 |
| kryształ | 1 (obcięte) | 0,2 | 2,455 | 1,655 |

W buforze LDR kryształ dawałby poświatę tylko dwa razy mocniejszą niż ściana, a każda rzecz jaśniejsza od bieli dokładnie taką samą. W buforze HDR nadwyżka kryształu jest ponad szesnaście razy większa niż nadwyżka ściany, więc próg da się ustawić tak, żeby ściany nie świeciły wcale, a kryształy świeciły mocno. To jest trzeci z powodów, dla których scena jest w `GL_RGBA16F` (sekcja 2.2), i powód, dla którego cele bloomu też mają ten format: rozmyta poświata światła kilka razy jaśniejszego od bieli ma zostać jaśniejsza niż poświata białej ściany.

Bloom na wartościach LDR to najczęstszy błąd przy tym efekcie (pułapka 17): świeci wtedy wszystko, co jasne, czyli także niebo, biała tekstura i panel interfejsu.

### 2.12 Jasność (luminancja) i przebieg jasności

**Jedna liczba zamiast trzech.** Próg jest jedną liczbą, a piksel ma trzy kanały. Trzeba więc umieć powiedzieć jedną liczbą, jak jasny jest kolor. Ta liczba nazywa się **luminancją** (luminance) i jest sumą ważoną kanałów:

```text
L = 0,2126 * R + 0,7152 * G + 0,0722 * B
```

Wagi pochodzą z normy Rec. 709, tej samej, z której sRGB bierze swoje barwy podstawowe czerwieni, zieleni i błękitu. W kodzie to stała `REC709_LUMINANCE_WEIGHTS` i funkcja `luminance` w `common/color.glsl` (sekcja 4.8). Trzy rzeczy, o które łatwo zostać zapytanym:

| Pytanie | Odpowiedź |
|---|---|
| Dlaczego wagi nie są równe (jedna trzecia każda)? | Oko nie jest równie czułe na trzy barwy. Najbardziej na zieleń, najmniej na błękit. Czysta zieleń (0, 1, 0) ma jasność 0,7152, czysty błękit (0, 0, 1) tylko 0,0722: dziesięć razy mniej, chociaż liczba w kanale jest ta sama |
| Dlaczego wagi sumują się do 1? | `0,2126 + 0,7152 + 0,0722 = 1`. Biel (1, 1, 1) ma wtedy jasność dokładnie 1, a szarość (x, x, x) jasność x. Skala jasności jest tą samą skalą co skala kanałów |
| Na jakich wartościach wolno to liczyć? | Tylko na **liniowych**. Wagi mówią, ile światła każdej barwy składa się na wrażenie jasności, a liczba zakodowana w sRGB nie jest proporcjonalna do światła. Bufor sceny jest liniowy, więc w `bright.frag` wszystko się zgadza. Policzenie tego samego na liczbach sRGB to pułapka 19 |

**Wzór przebiegu jasności.** Dla progu `T` (uniform `uThreshold`, startowo 0,8) i koloru piksela `c` o jasności `L`:

```text
udział = max(L - T, 0) / L
wynik  = c * udział
```

Słowami: licznik to **nadwyżka** jasności ponad próg (zero, gdy piksel jest pod progiem), a udział mówi, jaką częścią całej jasności piksela jest ta nadwyżka. Kolor jest mnożony przez udział, czyli wszystkie trzy kanały są zmniejszane **tyle samo razy**.

Luminancja jest sumą ważoną, więc pomnożenie koloru przez liczbę mnoży jego jasność przez tę samą liczbę. Jasność wyniku to zatem `L * udział = L - T`: **dokładnie nadwyżka ponad próg**. Przeliczone przykłady dla `T = 0,8`:

| Piksel `c` | Jasność `L` | Udział `max(L - 0,8, 0) / L` | Wynik `c * udział` | Jasność wyniku |
|---|---|---|---|---|
| (0,5, 0,5, 0,5), pod progiem | 0,5 | 0 / 0,5 = 0 | (0, 0, 0) | 0 |
| (0,8, 0,8, 0,8), dokładnie na progu | 0,8 | 0 / 0,8 = 0 | (0, 0, 0) | 0 |
| (0,2, 1,0, 0,9), tuż nad progiem | 0,0425 + 0,7152 + 0,0650 = 0,8227 | 0,0227 / 0,8227 = 0,0276 | (0,0055, 0,0276, 0,0248) | 0,0227 |
| (0,3, 2,0, 1,5), wyraźnie nad progiem | 0,0638 + 1,4304 + 0,1083 = 1,6025 | 0,8025 / 1,6025 = 0,5008 | (0,150, 1,002, 0,751) | 0,8025 |
| (0,132, 3,150, 2,415), składnik emisyjny kryształu przy pełnym pulsie | 0,0281 + 2,2529 + 0,1744 = 2,455 | 1,655 / 2,455 = 0,674 | (0,089, 2,123, 1,628) | 1,655 |

Dwie własności widać w tabeli od razu:

- **Nie ma skoku na progu.** Piksel dokładnie na progu daje zero, piksel o włos nad progiem daje prawie zero (udział 0,03), a dopiero piksel dużo jaśniejszy oddaje większość siebie. Kryształ pulsuje: jego jasność zmienia się płynnie i poświata razem z nią. Komentarz przy `CRYSTAL_GLOW_STRENGTH` nazywa to po imieniu: poświata ma "oddychać", a nie mrugać.
- **Barwa zostaje.** Proporcja zielonego do niebieskiego w wierszu czwartym to `2,0 / 1,5 = 1,33` przed i `1,002 / 0,751 = 1,33` po.

**Dlaczego dzielenie przez `L`, a nie coś prostszego.** Trzy sposoby wycięcia jasnych partii, na tym samym pikselu (0,3, 2,0, 1,5) i progu 0,8:

| Sposób | Wynik | Co jest nie tak |
|---|---|---|
| twardy próg: `L > T ? c : 0` | (0,3, 2,0, 1,5) | **skok**. Piksel o jasności 0,79 nie daje nic, a o jasności 0,81 oddaje cały swój kolor. Poświata pulsującego kryształu zapalałaby się i gasła, a krawędzie jasnych plam migotałyby przy ruchu kamery |
| odjęcie progu od każdego kanału: `max(c - T, 0)` | (0, 1,2, 0,7) | **zmiana barwy**. Czerwony zniknął w całości, a proporcja zielonego do niebieskiego zmieniła się z 1,33 na 1,71: turkusowy kryształ dostałby poświatę bardziej zieloną niż on sam |
| **udział jasności ponad progiem (w kodzie)** | (0,150, 1,002, 0,751) | bez skoku i bez zmiany barwy. Cena: jedno dzielenie na piksel i zabezpieczenie przed dzieleniem przez zero |

Dzielenie przez `L` jest więc tym, co zamienia nadwyżkę (jedną liczbę) z powrotem w kolor **o barwie oryginału**: zamiast odejmować od kanałów, skaluje się cały kolor.

**Zabezpieczenie przed zerem.** Czarny piksel ma `L = 0` i rachunek `0 / 0` dałby NaN ("nie liczba"). NaN w buforze zmiennoprzecinkowym nie znika: rozmycie rozniosłoby go na wszystkie piksele w zasięgu jądra. W shaderze mianownik to `max(L, MIN_LUMINANCE)` ze stałą `MIN_LUMINANCE = 0.0001`. Dla czarnego piksela wychodzi `0 / 0,0001 = 0`, a dla każdego piksela nad progiem `L` jest dużo większe od tej stałej, więc niczego ona nie zmienia.

**W jakich jednostkach jest próg.** Próg jest jasnością w liniowych wartościach bufora sceny, **przed ekspozycją**. Wartość 1 to biel ekranu przy ekspozycji 1 i bez krzywej. Wynikają z tego dwie rzeczy: suwak `Exposure` nie zmienia tego, **co** świeci (bloom jest liczony z bufora, zanim ekspozycja go dotknie), a tylko jasność całości, razem z poświatą. I odwrotnie: próg 1,0 nie znaczy "to, co na ekranie jest białe", bo na ekran prowadzi jeszcze ekspozycja i krzywa.

**Gdzie leży próg startowy.** Komentarz przy `BloomSettings::threshold` mówi, że 0,8 leży poniżej świecących kryształów (w każdej chwili pulsu), tarczy księżyca i najjaśniejszych gwiazd, a powyżej kamiennych ścian w świetle latarki. Zgłoszona obserwacja zgadza się z drugą połową tego zdania aż za dobrze: plama latarki na ścianie nie daje poświaty nawet z odległości metra. To otwarta kwestia wyglądu, nie błąd rachunku (sekcja 5.9).

**Próg działa po zmniejszeniu obrazu.** Cel przebiegu jasności ma połowę szerokości i połowę wysokości sceny (sekcja 2.14), więc jeden jego piksel odpowiada czterem pikselom sceny. Shader czyta scenę filtrem liniowym w punkcie, który przy parzystym rozmiarze sceny leży dokładnie na styku tych czterech pikseli, i dostaje ich **średnią**. Dopiero ta średnia jest porównywana z progiem. Dla rzeczy większych niż kilka pikseli nic to nie zmienia. Dla jasnego punktu o rozmiarze jednego piksela na ciemnym tle zmienia dużo: po uśrednieniu z trzema czarnymi sąsiadami zostaje ćwierć jego jasności, więc gwiazda musiałaby mieć w buforze ponad 3,2, żeby średnia przekroczyła 0,8. Zgłoszony wygląd jest z tym zgodny: gwiazdy zostają punktami, a wyraźną poświatę ma tarcza księżyca. Komentarz przy progu wymienia najjaśniejsze gwiazdy wśród rzeczy leżących powyżej progu. Czy i jak mocno one świecą, nie mierzyłem. Przy nieparzystym rozmiarze sceny punkt odczytu nie trafia dokładnie w styk i średnia ma nierówne wagi.

### 2.13 Rozmycie Gaussa: wzór, sigma, promień i wagi

Po przebiegu jasności obraz jest czarny z ostrymi jasnymi plamami. Poświata powstaje przez **rozmycie**: każdy piksel staje się średnią ważoną siebie i sąsiadów. Zestaw wag nazywa się **jądrem** (kernel). Jasna plama oddaje wtedy część światła pikselom dookoła, a sama trochę ciemnieje.

Wagi pochodzą z **funkcji Gaussa**, czyli krzywej dzwonowej:

```text
G(d) = exp(-d * d / (2 * sigma * sigma))
```

`d` to odległość od środka w pikselach, a `sigma` (odchylenie standardowe) to szerokość dzwonu: przy `d = sigma` krzywa ma 61 procent wysokości, przy `d = 2 * sigma` ma 14 procent. Dzwon dlatego, że daje poświatę gładką i okrągłą: najwięcej światła zostaje blisko źródła i płynnie go ubywa. Średnia o równych wagach (rozmycie pudełkowe) daje poświatę o kwadratowym kształcie i ostrym brzegu.

W kodzie (`game/Bloom.hpp`) są trzy stałe:

| Stała | Wartość | Znaczenie |
|---|---|---|
| `BLOOM_BLUR_SIGMA` | 3,0 | szerokość dzwonu w pikselach celu bloomu |
| `BLOOM_BLUR_RADIUS` | 6 | ile pikseli jest czytanych z **każdej** strony piksela zapisywanego. Razem z nim samym `2 * 6 + 1 = 13` odczytów tekstury na przebieg |
| `BLOOM_BLUR_WEIGHT_COUNT` | 7 (`BLOOM_BLUR_RADIUS + 1`) | liczba **różnych** wag: jedna dla środka i po jednej dla odległości od 1 do 6. Jądro jest symetryczne, więc waga dla odległości 2 służy pikselowi 2 w lewo i pikselowi 2 w prawo |

**Wagi policzone.** `2 * sigma * sigma = 18`, więc wysokość dzwonu to `exp(-d * d / 18)`:

| Odległość `d` | `d * d / 18` | Wysokość dzwonu `exp(...)` | Waga po podzieleniu przez 7,2981 |
|---|---|---|---|
| 0 | 0 | 1,0000 | 0,1370 |
| 1 | 0,0556 | 0,9460 | 0,1296 |
| 2 | 0,2222 | 0,8007 | 0,1097 |
| 3 | 0,5000 | 0,6065 | 0,0831 |
| 4 | 0,8889 | 0,4111 | 0,0563 |
| 5 | 1,3889 | 0,2494 | 0,0342 |
| 6 | 2,0000 | 0,1353 | 0,0185 |

**Normalizacja.** Suma wysokości dla całego jądra, czyli środek raz i każda inna odległość dwa razy, to `1 + 2 * (0,9460 + 0,8007 + 0,6065 + 0,4111 + 0,2494 + 0,1353) = 1 + 2 * 3,1490 = 7,2981`. Każda wysokość jest dzielona przez tę sumę i dopiero to są wagi. Sprawdzenie: `0,1370 + 2 * (0,1296 + 0,1097 + 0,0831 + 0,0563 + 0,0342 + 0,0185) = 1,0000`.

Suma równa 1 znaczy, że rozmycie **przesuwa światło, ale go nie dodaje ani nie gubi**: suma jasności całego obrazu jest po przebiegu taka sama jak przed nim. To nie jest kosmetyka, bo przebiegów jest domyślnie dwanaście z rzędu. Gdyby suma wag wynosiła 1,1, po dwunastu przebiegach obraz byłby `1,1^12 = 3,14` raza jaśniejszy. Przy sumie 0,9 zostałoby `0,9^12 = 0,28` jasności. Z tego samego powodu we wzorze nie ma znanego z podręczników czynnika `1 / (sigma * sqrt(2 * pi))`: jest stały dla wszystkich wag, więc dzielenie przez sumę i tak by go skróciło.

**Dlaczego promień to dwie sigmy.** Dzwon nie kończy się nigdy, a shader może przeczytać tylko skończoną liczbę pikseli. Przy `d = 6` krzywa ma 13,5 procent wysokości (komentarz przy `BLOOM_BLUR_SIGMA` zaokrągla to do 14), a dalej szybko gaśnie. Suma całego, nieuciętego dzwonu po pikselach to 7,5199, suma uciętego to 7,2981: za promieniem zostaje 2,9 procent. Normalizacja rozdziela tę brakującą część na trzynaście wag, które są. Skutek uboczny: ucięte jądro jest odrobinę węższe, niż mówi stała. Jego rzeczywiste odchylenie standardowe, policzone z wag z tabeli, to 2,73 piksela, a nie 3.

**Iteracje: wielokrotne rozmycie zamiast większego jądra.** Pole `BloomSettings::blurIterations` (startowo 6, zakres od `MIN_BLOOM_BLUR_ITERATIONS = 1` do `MAX_BLOOM_BLUR_ITERATIONS = 10`) mówi, ile razy rozmycie jest powtarzane. Rozmycie Gaussa zastosowane dwa razy daje znowu rozmycie Gaussa, tylko szersze. Szerokości nie dodają się wprost, dodają się ich **kwadraty** (wariancje), więc po `n` powtórzeniach `sigma_n = sigma * sqrt(n)`:

| Iteracje `n` | Przebiegi rozmycia `2n` | `3 * sqrt(n)`, szerokość według stałej | Szerokość z uciętego jądra (`2,73 * sqrt(n)`) | To samo w pikselach ekranu (razy 2) | Najdalszy zasięg światła, `6n` pikseli celu |
|---|---|---|---|---|---|
| 1 | 2 | 3,0 | 2,7 | 5,5 | 6 |
| 2 | 4 | 4,2 | 3,9 | 7,7 | 12 |
| 6 (startowo) | 12 | 7,3 | 6,7 | 13,4 | 36 |
| 10 | 20 | 9,5 | 8,6 | 17,3 | 60 |

Komentarz przy `blurIterations` podaje dla wartości startowej "około 7 pikseli celu, czyli około 15 pikseli sceny dwa razy większej": to liczba z drugiej kolumny wzoru, `3 * sqrt(6) = 7,35`. Z poprawką na ucięcie wychodzi 6,7 i 13,4. Różnica nie ma znaczenia dla wyglądu, ale na obronie trzymam się tego, co wynika z wag.

Dwa wnioski z tabeli. Podwojenie liczby iteracji **nie** podwaja szerokości poświaty: z 6 na 10 iteracji szerokość rośnie o 29 procent, a koszt o 67 procent. I druga rzecz: większe jądro (na przykład promień 15) dałoby tę samą szerokość w jednym powtórzeniu, ale promień jest stałą wpisaną w shader (rozmiar tablicy `uWeights`), a liczba iteracji jest liczbą z suwaka. Dlatego szerokością steruje się iteracjami.

Wagi liczy C++ (`game::bloomBlurWeights`), a shader dostaje je jako tablicę uniformów `uWeights`. Dlaczego nie są wpisane w shader jako stałe, zapisuje notatka [`../../decisions/blur-weights-computed-on-cpu.md`](../../decisions/blur-weights-computed-on-cpu.md).

### 2.14 Rozmycie rozdzielne, ping-pong, trzy cele i połowa rozdzielczości

**Rozmycie rozdzielne (separable).** Poświata ma być okrągła, więc jądro powinno być dwuwymiarowe: kwadrat 13 x 13 wag, czyli `13 * 13 = 169` odczytów tekstury na każdy piksel. Funkcja Gaussa ma własność, która pozwala tego uniknąć. Dwuwymiarowy dzwon rozkłada się na **iloczyn** dwóch jednowymiarowych:

```text
exp(-(x * x + y * y) / (2 * sigma * sigma)) = exp(-x * x / (2 * sigma * sigma)) * exp(-y * y / (2 * sigma * sigma))
```

Waga piksela przesuniętego o `(x, y)` to więc waga dla `x` razy waga dla `y`. Z tego wynika, że rozmycie wierszy, a potem rozmycie kolumn **wyniku**, daje dokładnie ten sam obraz co jedno rozmycie pełnym kwadratem. Na małym przykładzie, dla jądra trzech wag `(1/4, 1/2, 1/4)`:

```text
                 1/4                    1  2  1
(1/4 1/2 1/4) x  1/2   =   1/16  *     2  4  2
                 1/4                    1  2  1
```

Po prawej stoi dwuwymiarowe jądro 3 x 3, które powstaje z przemnożenia każdej wagi poziomej przez każdą pionową. Przebieg poziomy rozlewa piksel na trzy w wierszu, przebieg pionowy rozlewa każdy z tych trzech na trzy w kolumnie: razem dziewięć pikseli z wagami z tabelki.

Koszt dla `N = 13` odczytów w jednym kierunku:

| Sposób | Odczytów tekstury na piksel | Dla celu 640 x 360 (230 400 pikseli), jedna iteracja | Sześć iteracji |
|---|---|---|---|
| jeden przebieg pełnym kwadratem | `N * N = 169` | 38 937 600 | 233 625 600 |
| **dwa przebiegi, poziomy i pionowy (w kodzie)** | `N + N = 26` | 5 990 400 | 35 942 400 |

Sześć i pół raza mniej odczytów za ten sam obraz. Cena to drugi przebieg i **cel pośredni**: wynik przebiegu poziomego musi gdzieś trafić, zanim przeczyta go pionowy. Nie każde jądro da się tak rozłożyć. Gauss tak, i to jeden z powodów, dla których jest standardem.

**Ping-pong.** Przebieg nie może czytać tekstury, do której rysuje (sekcja 2.1). Rozmycie potrzebuje więc dwóch celów, które zamieniają się rolami: z pierwszego do drugiego, z drugiego do pierwszego i tak dalej. Nazwa bierze się z odbijania obrazu tam i z powrotem. W tym kodzie:

```text
iteracja 1:   m_brightPass     --poziomo-->  m_blurHorizontal  --pionowo-->  m_bloom
iteracja 2:   m_bloom          --poziomo-->  m_blurHorizontal  --pionowo-->  m_bloom
iteracja 3:   m_bloom          --poziomo-->  m_blurHorizontal  --pionowo-->  m_bloom
...
```

Przebieg poziomy zawsze pisze do `m_blurHorizontal`, pionowy zawsze do `m_bloom`. Po każdej iteracji gotowy wynik leży w `m_bloom` i stamtąd bierze go przebieg składający. W żadnym przebiegu źródło i cel nie są tym samym obiektem.

**Dlaczego trzy cele, a nie dwa.** Do samego ping-ponga wystarczyłyby dwa: przebieg jasności mógłby pisać od razu do `m_bloom`. Wtedy jednak już druga połowa pierwszej iteracji zamazałaby wynik przebiegu jasności, a panel ma go pokazać. Trzeci cel, `m_brightPass`, jest czytany tylko przez pierwszy przebieg poziomy i **nigdy nie jest celem rozmycia**, więc podgląd `Bright pass` pokazuje dokładnie to, co zostało po progu. Cena to jedna tekstura `GL_RGBA16F` więcej: 1,76 MiB przy oknie 1280 x 720 i 7,03 MiB przy 2560 x 1440 (policzone z rozmiaru, nie zmierzone na karcie). Uzasadnienie i rozważane możliwości są w notatce [`../../decisions/bloom-half-resolution-three-targets.md`](../../decisions/bloom-half-resolution-three-targets.md).

**Połowa rozdzielczości.** Stała `BLOOM_DOWNSCALE = 2` mówi, że każdy z trzech celów ma połowę szerokości i połowę wysokości bufora sceny. Rozmiar liczy `game::bloomTargetExtent`: dzielenie całkowite (reszta przepada), ale nie mniej niż 1.

| Bufor sceny | Cel bloomu | Pikseli w celu | Trzy cele razem |
|---|---|---|---|
| 1280 x 720 | 640 x 360 | 230 400 | 5,27 MiB |
| 2560 x 1440 | 1280 x 720 | 921 600 | 21,09 MiB |
| 1000 x 600 | 500 x 300 | 150 000 | 3,43 MiB |
| 1281 x 719 | 640 x 359 | 229 760 | 5,26 MiB |

(Pamięć policzona z rozmiaru, 8 bajtów na piksel.) Połowa w każdym kierunku to **ćwierć pikseli**. Co to daje:

| Zysk | Dlaczego |
|---|---|
| cztery razy mniejszy koszt | każdy przebieg rozmycia cieniuje ćwierć fragmentów. Sześć iteracji w pełnej rozdzielczości 1280 x 720 to 143 769 600 odczytów tekstury, w połowie 35 942 400 |
| dwa razy szersza poświata z tego samego jądra | jeden piksel celu to dwa piksele ekranu, więc sześć pikseli promienia sięga na ekranie na dwanaście |
| trochę rozmycia za darmo, w dwóch miejscach | **przy zmniejszaniu**: przebieg jasności czyta scenę filtrem liniowym i dostaje średnią czterech pikseli (sekcja 2.12). **Przy powiększaniu**: przebieg składający czyta `m_bloom` filtrem liniowym na całym ekranie, więc każdy piksel ekranu dostaje gładką mieszankę czterech najbliższych pikseli celu zamiast kwadratów 2 x 2 |

W samych przebiegach rozmycia filtr liniowy **nic nie daje**: źródło i cel mają ten sam rozmiar, a przesunięcia są całkowitą liczbą pikseli, więc każdy odczyt trafia dokładnie w środek jednego teksela i zwraca go bez mieszania.

Niższej rozdzielczości nie widać, bo rozmyty obraz nie ma ostrych szczegółów, które mogłyby wyjść kanciasto. To ten sam pomysł, o którym PRD wspomina jako o "post-processie w połowie rozdzielczości".

**Cena połowy rozdzielczości: poświata mierzona w tekselach.** Jądro ma stały promień w pikselach **celu**, a nie w ułamku ekranu. Poświata o szerokości 13,4 piksela ekranu to 1,9 procent wysokości okna 720 pikseli, ale tylko 0,9 procent przy 1440. Na większym ekranie ta sama scena ma więc poświatę względnie o połowę cieńszą. To zgłoszona otwarta obserwacja: w 1440p sprawdzono tylko wycinek obrazu. Kod nie skaluje liczby iteracji ani sigmy z rozdzielczością (pułapka 20).

**Brzegi obrazu.** Tekstury załączników mają zawijanie `GL_CLAMP_TO_EDGE` ([`../gfx/framebuffers.md`](../gfx/framebuffers.md)). Odczyt poza krawędź zwraca piksel brzegowy, więc jasna rzecz przy samej krawędzi ekranu nie traci światła "za ekran": brzegowy piksel jest liczony kilka razy.

**Ile przebiegów ma klatka.** Każdy przebieg to jedno wywołanie `glDrawArrays` z trzema wierzchołkami:

| Przebieg | Rozmiar celu | Ile razy w klatce, ustawienia startowe | Zakres |
|---|---|---|---|
| scena | pełny | 1 (wiele wywołań rysujących) | 1 |
| przebieg jasności | połowa | 1 | 0 albo 1 |
| rozmycie | połowa | `2 * 6 = 12` | od 2 (jedna iteracja) do 20 (dziesięć) |
| przebieg składający | okno | 1 | 1 |
| podglądy sceny (tylko przy otwartym panelu) | 320 x 180 | 2 | 0 albo 2 |
| podglądy bloomu (tylko przy otwartym panelu i narysowanym bloomie) | 320 x 180 | 2 | 0 albo 2 |

Bloom to przy ustawieniach startowych **13 przebiegów** w połowie rozdzielczości. Razem z przebiegiem składającym klatka ma 14 trójkątów pełnoekranowych, a z otwartym panelem 18. Liczba odczytów tekstury w samym bloomie dla okna 1280 x 720 to `230 400 * (1 + 12 * 13) = 36 172 800` na klatkę, dla 2560 x 1440 cztery razy tyle.

Zgłoszony koszt (Release, panele ukryte, pomiar niespokojny) jest w sekcji 5.9: w 2560 x 1440 czas klatki rośnie z około 0,7 ms do około 1,1 ms.

### 2.15 Dodanie do sceny: dlaczego przed ekspozycją i mapowaniem tonów

Ostatni krok bloomu jest jedną linią w `composite.frag`:

```glsl
        color += texture(uBloom, vUv).rgb * uBloomIntensity;
```

**Dodawanie, nie mieszanie.** Poświata jest światłem, a światło z dwóch źródeł się sumuje. Scena nie jest przy tym przyciemniana ani zastępowana: jasna plama zostaje tam, gdzie była, a rozmyta kopia jej nadwyżki dochodzi do niej i do sąsiadów. Uczciwie: bloom **dodaje energię**. Światło ponad progiem jest w obrazie dwa razy, raz w scenie i drugi raz rozlane dookoła. To efekt wyglądu, nie model fizyczny. Suwak `Intensity` (`uBloomIntensity`, od 0 do 2, startowo 1) mnoży poświatę przed dodaniem: 0 nie dodaje nic, 2 podwaja.

**Dlaczego przed ekspozycją i krzywą.** Komentarz funkcji `main` ustawia bloom jako krok 2, między odczytem sceny a ekspozycją. Trzy powody:

| Powód | Wyjaśnienie |
|---|---|
| dodawanie światła ma sens tylko na wartościach liniowych | suma dwóch liczb proporcjonalnych do światła jest proporcjonalna do sumy świateł. Po krzywej i po kodowaniu liczby tej własności nie mają |
| ekspozycja i krzywa mają potraktować poświatę jak resztę obrazu | po zmianie suwaka `Exposure` poświata jaśnieje i ciemnieje razem ze sceną. Dodana po ekspozycji miałaby stałą jasność niezależnie od suwaka |
| krzywa sprowadza sumę do zakresu ekranu | składnik emisyjny kryształu ma w zielonym 3,150, a poświata najwyżej 2,1 (tyle zostawia przebieg jasności, rozmycie to jeszcze zmniejsza). Nawet suma 5,3 przechodzi przez ACES jako 0,99: prawie biel, ale nie obcięcie. Dodanie poświaty **po** krzywej dałoby `0,957` plus poświata, czyli wynik obcinany do 1 wszędzie tam, gdzie scena jest już jasna: płaską białą plamę bez barwy |

Drugi przykład, dla ciemnego piksela tuż obok kryształu: scena 0,05, poświata 0,4. Dodane przed krzywą: `0,45`, po ACES 0,58. Sam piksel sceny po ACES to 0,044. Poświata jest więc tym, co rozjaśnia otoczenie kryształu na ekranie, i przechodzi przez tę samą krzywą co wszystko inne.

**Odczyt tekstury o połowie rozmiaru.** `uBloom` jest czytane tym samym `vUv` co scena. Współrzędne tekstury biegną od 0 do 1 niezależnie od rozmiaru, więc mniejsza tekstura jest po prostu rozciągana na cały ekran filtrem liniowym.

**Bloom wyłączony to dokładnie klatka bez bloomu.** Uniform `uBloomEnabled` ma 1 tylko wtedy, gdy bloom jest włączony w ustawieniach **i** `drawBloom` narysowało go w tej klatce (`bloomDrawn()`). Przy 0 shader w ogóle nie czyta tekstury bloomu, więc do koloru nic nie jest dodawane. Zgłoszone: z wyłączonym bloomem obraz był identyczny co do piksela z obrazem pierwszej części w sześciu widokach. Pomiar zrobiono **przed** zmianą siły świecenia kryształów, więc dzisiejsza klatka bez bloomu różni się od pierwszej części właśnie kryształami.

**Siła świecenia kryształów wzrosła razem z bloomem.** `CRYSTAL_GLOW_STRENGTH` zmieniło się z 2,5 na 4,0. Rachunek dla samego składnika emisyjnego (`uEmissive`), przed pomnożeniem przez teksturę kryształu, w najjaśniejszej chwili pulsu (mnożnik 1) i w najciemniejszej (mnożnik `1 - CRYSTAL_PULSE_DEPTH = 0,7`):

| Siła | Puls | `uEmissive` | Jasność `L` | O ile tekstura może przyciemnić, zanim piksel spadnie pod próg 0,8 |
|---|---|---|---|---|
| 2,5 | 1,0 | (0,083, 1,969, 1,510) | 1,534 | o 48 procent |
| 2,5 | 0,7 | (0,058, 1,378, 1,057) | 1,074 | o 26 procent |
| 4,0 | 1,0 | (0,132, 3,150, 2,415) | 2,455 | o 67 procent |
| 4,0 | 0,7 | (0,093, 2,205, 1,691) | 1,719 | o 53 procent |

Kolor piksela to `tekstura * (światło rozproszone + uEmissive) + odbłysk`, więc tekstura kryształu przyciemnia świecenie. Komentarz przy stałej mówi, że zabiera ponad połowę. Przy sile 2,5 w najciemniejszej chwili pulsu zapas wynosił 26 procent, więc piksele spadały pod próg i poświata znikała: tak brzmi zgłoszona obserwacja, i poświata "mrugałaby zamiast oddychać". Przy 4,0 zapas w najciemniejszej chwili to 53 procent. Liczby w ostatniej kolumnie wynikają z samego wzoru. Tekstury kryształu nie mierzyłem, więc to, ile naprawdę zabiera, znam tylko z komentarza i ze zgłoszonych zrzutów (poświata widoczna w obu skrajnych chwilach pulsu, w trzech trybach cieniowania, ze ściankami nadal widocznymi). Skutek uboczny: kryształy są bledsze także **bez** bloomu, bo jaśniejszy kolor ląduje wyżej na krzywej (sekcja 2.6). Decyzję zapisuje notatka [`../../decisions/crystal-glow-raised-for-bloom.md`](../../decisions/crystal-glow-raised-for-bloom.md).

### 2.16 Planowane: czego jeszcze nie ma

Poniższe efekty są w PRD (temat 10 i potok renderowania) i **nie istnieją w kodzie**. Każdy dostanie tu własną sekcję razem z kodem swojej części M7. Na razie jedyne, co o nich wiadomo z kodu, to miejsce w kolejności, zapisane w komentarzu funkcji `main` w `composite.frag`:

| Efekt | Stan | Miejsce w kolejności według komentarza w `composite.frag` |
|---|---|---|
| **Planowane: mgła** (z bufora głębi) | brak kodu | przed ekspozycją, w kroku 1 komentarza: efekt działa na samym świetle i potrzebuje wartości liniowych, które nie są obcięte. Funkcja `linearDepth` w `common/depth.glsl` jest wspólna właśnie po to, żeby czytać głębię w metrach |
| **Planowane: winieta** | brak kodu | po mapowaniu tonów, przed kodowaniem: efekt działa na gotowym obrazie |
| **Planowane: minimapa** (widok z góry w osobnym framebufferze) | brak kodu | osobny przebieg do własnego celu |
| **Planowane: cienie** (temat 11, shadow mapping) | brak kodu | osobne przebiegi przed sceną. `gfx::ColorFormat::None` (framebuffer z samą głębią) jest przygotowany, ale ta ścieżka nie została nigdy wykonana |

Nie ma też: drugiego załącznika koloru, automatycznej ekspozycji, bloomu w kilku rozdzielczościach naraz (łańcucha coraz mniejszych celów, który dałby szeroką poświatę taniej). Połowa rozdzielczości jest od drugiej części M7, ale tylko dla celów bloomu: bufor sceny ma nadal pełny rozmiar okna.

## 3. Jak to działa w OpenGL

Wywołania jednej klatki, w kolejności. Tworzenie framebuffera (kroki oznaczone "raz") opisuje szczegółowo [`../gfx/framebuffers.md`](../gfx/framebuffers.md), tutaj jest tylko to, co widać z poziomu przebiegu.

**Początek sceny** (`beginScene`):

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | konstruktor `gfx::Framebuffer` z `Rgba16F` i `Depth24`, raz na rozmiar okna | tworzy obiekt framebuffera i dwie tekstury, sprawdza kompletność, zostawia związany domyślny framebuffer |
| 2 | `glBindFramebuffer(GL_FRAMEBUFFER, id)` | od teraz rysowanie trafia do tekstur sceny |
| 3 | `glViewport(0, 0, width, height)` | obszar rysowania na cały bufor sceny. Viewport należy do kontekstu, nie do framebuffera |

**Scena** (`onRender`): `glEnable(GL_DEPTH_TEST)`, `glClearColor`, `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` i wywołania rysujące jak przed tą częścią. `glClear` czyści teraz dwie tekstury bufora sceny, nie okno.

**Podglądy** (`drawPreviews`, tylko przy otwartym panelu), dla każdego z dwóch obrazów:

| # | Wywołanie | Co robi |
|---|---|---|
| 4 | `glDisable(GL_DEPTH_TEST)` | jeden płaski trójkąt, nie ma z czym porównywać głębi |
| 5 | `glUseProgram(preview)`, `glUniform1i(uSource, 0)`, `glUniform1f` dla `uNear`, `uFar`, `uDepthRange` | program podglądu i jego stałe na tę klatkę |
| 6 | `glBindFramebuffer(GL_FRAMEBUFFER, podgląd)` z `glViewport` na jego rozmiar | celem jest mały framebuffer `GL_RGBA8` |
| 7 | `glActiveTexture(GL_TEXTURE0)`, `glBindTexture(GL_TEXTURE_2D, załącznik)`, `glBindSampler(0, 0)` | tekstura koloru albo głębi sceny na jednostce 0, bez obiektu samplera |
| 8 | `glUniform1i(uMode, 0 albo 1)` | co pokazuje ten obraz |
| 9 | `glBindVertexArray(pusty VAO)`, `glDrawArrays(GL_TRIANGLES, 0, 3)` | trójkąt pełnoekranowy |

**Bloom** (`drawBloom`, w każdej klatce, w której bloom jest włączony):

| # | Wywołanie | Co robi |
|---|---|---|
| B1 | trzy konstruktory `gfx::Framebuffer` z `Rgba16F` i bez głębi, raz (potem `resize` przy zmianie rozmiaru) | trzy cele o połowie szerokości i wysokości sceny |
| B2 | `glDisable(GL_DEPTH_TEST)` | płaskie trójkąty, cele nie mają załącznika głębi |
| B3 | `glUseProgram(bright)`, `glUniform1i(uScene, 0)`, `glUniform1f(uThreshold, ...)` | program przebiegu jasności i próg z suwaka |
| B4 | `glBindFramebuffer(GL_FRAMEBUFFER, m_brightPass)` z `glViewport(0, 0, szerokość / 2, wysokość / 2)` | celem jest mniejsza tekstura. Trójkąt nadal pokrywa cały viewport, więc obraz sceny jest do niego **zmniejszany** |
| B5 | `glActiveTexture(GL_TEXTURE0)`, `glBindTexture(GL_TEXTURE_2D, kolor sceny)`, `glBindSampler(0, 0)`, potem `glDrawArrays` | przebieg jasności |
| B6 | `glUseProgram(blur)`, `glUniform1i(uSource, 0)`, `glUniform1fv(uWeights, 7, wskaźnik)` | program rozmycia i siedem wag, wysłane **raz na klatkę** jednym wywołaniem ([`../gfx/uniforms.md`](../gfx/uniforms.md)) |
| B7 | `glBindFramebuffer(m_blurHorizontal)`, `glBindTexture(źródło)`, `glUniform1i(uHorizontal, 1)`, `glDrawArrays` | przebieg poziomy. Źródłem jest `m_brightPass` w pierwszej iteracji, potem `m_bloom` |
| B8 | `glBindFramebuffer(m_bloom)`, `glBindTexture(m_blurHorizontal)`, `glUniform1i(uHorizontal, 0)`, `glDrawArrays` | przebieg pionowy. Kroki B7 i B8 powtarzają się tyle razy, ile jest iteracji |
| B9 | tylko przy otwartym panelu: `glUseProgram(preview)`, `uMode = 0`, dwa razy wiązanie małego framebuffera, tekstury i `glDrawArrays` | podglądy `m_brightPass` i `m_bloom` |

W krokach B7 i B8 **najpierw** zmieniany jest cel, a **potem** wiązana tekstura źródłowa. Między tymi dwiema liniami na jednostce 0 wisi jeszcze tekstura, która właśnie stała się celem, ale przed `glDrawArrays` zostaje zastąpiona źródłem. Liczy się stan w chwili rysowania.

**Przebieg składający** (`composite`):

| # | Wywołanie | Co robi |
|---|---|---|
| 10 | `glBindFramebuffer(GL_FRAMEBUFFER, 0)` z `glViewport` na rozmiar framebuffera okna | celem znowu jest okno |
| 11 | `glDisable(GL_DEPTH_TEST)` | bufor głębi okna nie jest już nigdy czyszczony, więc test mógłby odrzucić trójkąt |
| 12 | `glDisable(GL_FRAMEBUFFER_SRGB)` | OpenGL nie koduje przy zapisie: robi to shader |
| 13 | `glUseProgram(composite)`, `glUniform1i(uBloomEnabled, 0 albo 1)`, `glUniform1i(uBloom, 1)`, `glUniform1f(uBloomIntensity, ...)` | trzy uniformy bloomu. Sampler `uBloom` dostaje numer **drugiej** jednostki |
| 14 | tylko gdy bloom jest dodawany: `glActiveTexture(GL_TEXTURE1)`, `glBindTexture(GL_TEXTURE_2D, m_bloom)`, `glBindSampler(1, 0)` | rozmyta poświata na jednostce 1 |
| 15 | `glUniform1i(uScene, 0)`, `glActiveTexture(GL_TEXTURE0)`, `glBindTexture(GL_TEXTURE_2D, kolor sceny)`, `glBindSampler(0, 0)` | tekstura HDR sceny na jednostce 0 |
| 16 | `glUniform1f(uExposure, ...)`, `glUniform1i(uToneMapping, ...)` | dwa ustawienia z panelu |
| 17 | `glBindVertexArray(pusty VAO)`, `glDrawArrays(GL_TRIANGLES, 0, 3)` | trójkąt pełnoekranowy do okna |

Okno **nie jest czyszczone** przed krokiem 17: trójkąt pokrywa każdy piksel, więc `glClear` byłoby pracą wyrzuconą.

**Dwie jednostki teksturujące w jednym przebiegu.** Przebieg składający jest jedynym przebiegiem po scenie, który czyta dwa obrazy naraz. Każdy sampler shadera trzyma numer jednostki: `uScene` numer 0 (stała `SOURCE_TEXTURE_UNIT`), `uBloom` numer 1 (`BLOOM_TEXTURE_UNIT`). Gdyby oba dostały ten sam numer, oba czytałyby tę samą teksturę i poświata byłaby kopią sceny.

**Kroki 7, B5, 14 i 15: dlaczego `glBindSampler(unit, 0)`.** Każda `Texture2D` zostawia na jednostce swój obiekt samplera z `GL_REPEAT` i mipmapami ([`../gfx/textures.md`](../gfx/textures.md), sekcja 2.8). Obiekt samplera należy do jednostki i zastępuje parametry każdej tekstury przez nią czytanej. Tekstura załącznika ma jeden poziom, więc czytana cudzym samplerem z filtrem mipmap byłaby niekompletna i dałaby czerń. `Framebuffer::bindColorTexture` i `bindDepthTexture` odpinają więc sampler i tekstura jest czytana własnymi parametrami.

**Co zostaje po klatce.** Związany domyślny framebuffer, viewport na rozmiar okna, test głębi **wyłączony**, na jednostce 0 tekstura koloru sceny bez obiektu samplera, na jednostce 1 tekstura `m_bloom` bez obiektu samplera (gdy bloom był dodawany), związany pusty VAO, program `composite` w użyciu. Następna klatka włącza test głębi sama (krok 3 tabeli w sekcji 2.8), a każdy przebieg sceny wiąże swoje tekstury, swój VAO i swój program.

**Czy tekstura zostawiona na jednostce to pętla zwrotna?** Na początku następnej klatki tekstura koloru sceny nadal wisi na jednostce 0, a scena jest do niej rysowana. Tak samo `m_bloom` wisi na jednostce 1, kiedy następne `drawBloom` do niego rysuje. To **nie** jest pętla zwrotna. Niezdefiniowany wynik daje dopiero **odczyt** tekstury, która jest celem, a nie samo jej związanie. Programy `bright` i `blur` mają po jednym samplerze, który wskazuje jednostkę 0, a tam w chwili rysowania leży źródło, więc żaden ich sampler nie patrzy na teksturę celu. Shadery sceny wiążą na jednostkach, z których czytają, własne tekstury przed rysowaniem.

Wszystkie użyte funkcje są w rdzeniu OpenGL 4.1: obiekty framebuffera i tekstury zmiennoprzecinkowe od 3.0, obiekty samplera od 3.3, `gl_VertexID` i `textureSize` w GLSL od 1.30, `glUniform1fv` od 2.0.

## 4. Shadery

Pięć plików w `assets/shaders/post/` tworzy cztery programy: `composite` (`composite.vert` + `composite.frag`), `preview` (`composite.vert` + `preview.frag`) i, od drugiej części M7, `bright` (`composite.vert` + `bright.frag`) oraz `blur` (`composite.vert` + `blur.frag`). Shader wierzchołków jest wspólny dla wszystkich czterech. Dwa pliki dołączane leżą w `assets/shaders/common/`: `color.glsl` (kodowanie opisuje [`../gfx/color-space.md`](../gfx/color-space.md), funkcję `luminance` sekcja 4.8) i `depth.glsl` (sekcja 4.4). Shadery bloomu mają numery 4.6 i 4.7, po sekcji o stronie C++, żeby numery sekcji z pierwszej części zostały te same.

### 4.1 `composite.vert`

```glsl
#version 410 core
// Vertex shader of the post-processing passes: one triangle that covers the whole
// screen, made without any vertex data.
// See docs/modules/renderer/post-process.md

// There is no input. The draw call is glDrawArrays(GL_TRIANGLES, 0, 3) with a vertex
// array object that has no attributes (a Core profile still needs one bound), and the
// three corners are computed from gl_VertexID, the number of the vertex: 0, 1, 2.

// Output to the fragment shader: the texture coordinate of the screen, (0, 0) in the
// bottom left corner and (1, 1) in the top right one.
out vec2 vUv;

void main() {
    // Vertex 0 -> (0, 0), vertex 1 -> (2, 0), vertex 2 -> (0, 2). The first line takes
    // bit 0 of the number for x, the second bit 1 for y.
    float x = float(gl_VertexID % 2) * 2.0;
    float y = float(gl_VertexID / 2) * 2.0;
    vUv = vec2(x, y);

    // From 0..2 to -1..3 in clip space: the corners (-1, -1), (3, -1) and (-1, 3). The
    // screen is the square from -1 to 1, and this one triangle covers it completely.
    // What sticks out is clipped away. One triangle instead of two that share
    // a diagonal: no pixel is shaded twice along a seam. Depth 0 and w 1: the pass is
    // drawn with the depth test off.
    gl_Position = vec4(vUv * 2.0 - 1.0, 0.0, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| brak `layout(location = ...) in ...` | shader nie ma żadnego wejścia. To jedyny shader wierzchołków gry bez atrybutów |
| `out vec2 vUv;` | współrzędna tekstury ekranu, interpolowana dla każdego fragmentu. Nazwa i typ muszą się zgadzać z `in vec2 vUv;` we wszystkich czterech shaderach fragmentów |
| `gl_VertexID` | wbudowana zmienna typu `int`: numer wierzchołka w wywołaniu rysującym. Dla `glDrawArrays(GL_TRIANGLES, 0, 3)` to 0, 1, 2 |
| `float(gl_VertexID % 2) * 2.0` | bit 0 numeru, zamieniony na `float` i pomnożony przez 2: daje 0, 2, 0 |
| `float(gl_VertexID / 2) * 2.0` | dzielenie całkowite, czyli bit 1: daje 0, 0, 2 |
| `vUv = vec2(x, y);` | (0, 0), (2, 0), (0, 2). Na widocznej części trójkąta wartości są od 0 do 1 (sekcja 2.4) |
| `gl_Position = vec4(vUv * 2.0 - 1.0, 0.0, 1.0);` | z zakresu 0 do 2 na zakres -1 do 3. `w = 1`, więc dzielenie perspektywiczne nic nie zmienia. `z = 0` to środek zakresu głębi, bez znaczenia przy wyłączonym teście |

W shaderze nie ma żadnej macierzy: trójkąt jest podany od razu w przestrzeni przycinania.

### 4.2 `composite.frag`

```glsl
#version 410 core
// Fragment shader of the composite pass, the last pass of a frame: reads the picture of
// the scene from the HDR texture and writes it to the window as colours a screen can
// show.
// See docs/modules/renderer/post-process.md

// linearToSrgb. The path is relative to this file.
#include "../common/color.glsl"

// Input from composite.vert: the texture coordinate of this pixel of the screen.
in vec2 vUv;

// The picture of the scene: linear colours, not limited to 1 (GL_RGBA16F). A sampler
// holds the number of a texture unit, set from C++ with glUniform1i.
uniform sampler2D uScene;

// The bloom: the bright parts of the scene, blurred (linear HDR colours, half the size
// of the scene). uBloomEnabled is 1 when it is added to the scene and 0 when the frame
// is drawn without it: the texture is not read then. The glow is multiplied by
// uBloomIntensity first.
uniform sampler2D uBloom;
uniform int uBloomEnabled;
uniform float uBloomIntensity;

// The colours are multiplied by this number, like a longer or a shorter exposure of
// a camera: 1 changes nothing, 2 doubles the light.
uniform float uExposure;

// How the range of the scene is brought into 0..1. The numbers are the values of
// game::ToneMapping in C++.
//   0: none, values above 1 are cut off
//   1: Reinhard
//   2: ACES (a fitted curve)
uniform int uToneMapping;

// Output: the color written to the window (red, green, blue, alpha).
out vec4 fragColor;
```

| Linia | Znaczenie |
|---|---|
| `#include "../common/color.glsl"` | dyrektywa własnego loadera, nie GLSL ([`../gfx/shader-includes.md`](../gfx/shader-includes.md)). Ścieżka jest względna do pliku, który ją zawiera: plik leży w `post/`, więc wychodzi poziom wyżej. Shadery sceny dołączają ten sam plik jako `"common/color.glsl"` |
| `uniform sampler2D uScene;` | sampler trzyma numer jednostki teksturującej. Typ `sampler2D` działa także dla tekstury zmiennoprzecinkowej: `texture()` zwraca wtedy wartości bez ograniczenia do 1 |
| `uniform sampler2D uBloom;` | drugi sampler, z numerem **innej** jednostki niż `uScene` (1). Tekstura ma połowę szerokości i wysokości sceny, ale sampler o tym nie wie: współrzędne od 0 do 1 obejmują ją całą. "Half the size" w komentarzu znaczy połowę w każdym kierunku, czyli ćwierć pikseli |
| `uniform int uBloomEnabled;` | 1 albo 0. `int`, a nie `bool`, bo po stronie C++ ustawia go `setInt`, tak jak każdy przełącznik w shaderach gry |
| `uniform float uBloomIntensity;` | mnożnik z suwaka `Intensity` |
| `uniform float uExposure;` | mnożnik z suwaka `Exposure` |
| `uniform int uToneMapping;` | liczba z listy `Tone mapping`. Wartości 0, 1, 2 to wartości `enum class ToneMapping` rzutowane na `int` |
| `out vec4 fragColor;` | jedyny shader gry, którego wyjście trafia do okna. Wszystkie pozostałe piszą do bufora sceny albo do podglądu |

Dwie krzywe:

```glsl
// Reinhard: x / (1 + x), per channel. 0 stays 0, 1 becomes one half, and however bright
// the input is, the result stays below 1: nothing is cut off, bright areas keep detail.
// The price is that the whole picture gets darker and flatter.
vec3 toneMapReinhard(vec3 color) {
    return color / (vec3(1.0) + color);
}

// ACES: the look of the film industry reference curve, as the short formula Krzysztof
// Narkowicz fitted to it (2015). A quotient of two quadratic polynomials shaped like an
// S: dark tones are pressed down a little (more contrast), the middle is almost
// straight, and bright values bend softly towards 1. The five numbers are the fit.
vec3 toneMapAces(vec3 color) {
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `color / (vec3(1.0) + color)` | dzielenie wektorów w GLSL działa składowa po składowej: trzy osobne ułamki dla czerwonego, zielonego i niebieskiego |
| `const float A = 2.51;` i cztery następne | pięć liczb dopasowania. Nie mają znaczenia fizycznego: to współczynniki, przy których krótki wzór najlepiej przybliża krzywą odniesienia |
| `(color * (A * color + B)) / (color * (C * color + D) + E)` | iloraz dwóch wielomianów drugiego stopnia, na kanał. Przeliczone przykłady są w sekcji 2.6 |
| `clamp(..., 0.0, 1.0)` | konieczne: granica wzoru to `A / C = 1,033`, a 1 jest przekraczana od około 7,24 |

Komentarz mówi "przyciśnięte trochę" o ciemnych tonach i "środek prawie prosty". Liczby z sekcji 2.6 pokazują, że przy wartości 0,01 zostaje 38 procent wejścia, a środek skali jest podniesiony (0,18 daje 0,267). Na obronie trzymam się liczb.

Funkcja główna:

```glsl
void main() {
    // The steps run in a fixed order, and each one works on the result of the one
    // before. Later effects have their place in it:
    //
    //   1. the scene, in linear HDR colours
    //      (effects that work on light itself, like fog, are added here, before the
    //      exposure: they need linear values that are not cut off)
    //   2. bloom, such an effect: the glow of the bright parts is added
    //   3. exposure
    //   4. tone mapping: from 0..infinity to 0..1
    //      (effects that work on the finished picture, like a vignette, come here)
    //   5. encoding to sRGB, always last
    vec3 color = texture(uScene, vUv).rgb;

    // Bloom: the glow is light, so it is ADDED to the light of the scene, and it is
    // added before the exposure and the tone mapping, which then treat it like the
    // rest of the picture. The bloom texture is half as large as the screen: the linear
    // filter stretches it, and the blur has left nothing sharp in it to look blocky.
    if (uBloomEnabled == 1) {
        color += texture(uBloom, vUv).rgb * uBloomIntensity;
    }

    color *= uExposure;

    if (uToneMapping == 1) {
        color = toneMapReinhard(color);
    } else if (uToneMapping == 2) {
        color = toneMapAces(color);
    } else {
        color = clamp(color, 0.0, 1.0);
    }

    // The screen expects sRGB encoded numbers. This is the one place where the frame
    // is encoded (gamma correction). GL_FRAMEBUFFER_SRGB stays off, so OpenGL does not
    // encode a second time, and the debug UI, drawn after this pass straight into the
    // window, keeps the colours of its theme.
    fragColor = vec4(linearToSrgb(color), 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| komentarz z pięcioma krokami | zapisana kolejność i miejsca na efekty, których jeszcze nie ma (sekcja 2.16). W pierwszej części M7 kroków było cztery, a bloom stał w nawiasie kroku 1 jako efekt planowany. Dziś jest krokiem 2 |
| `vec3 color = texture(uScene, vUv).rgb;` | krok 1, odczyt piksela sceny. Bufor sceny ma rozmiar framebuffera okna, więc jeden piksel ekranu to jeden teksel i filtr liniowy niczego nie miesza. Alfa bufora sceny nie jest używana |
| `if (uBloomEnabled == 1) {` | krok 2 jest warunkowy. Przy 0 tekstura bloomu nie jest czytana wcale: nie ma mnożenia przez zero, tylko brak odczytu, więc na jednostce 1 może leżeć cokolwiek |
| `color += texture(uBloom, vUv).rgb * uBloomIntensity;` | odczyt poświaty tym samym `vUv`, mnożenie przez suwak, **dodanie** do koloru sceny. Tu filtr liniowy pracuje: tekstura ma połowę rozmiaru, więc każdy piksel ekranu dostaje mieszankę czterech najbliższych tekseli (sekcje 2.14 i 2.15) |
| `color *= uExposure;` | krok 3. Na wartościach liniowych, już z poświatą |
| `if (uToneMapping == 1) ... else if (== 2) ... else` | krok 4. Gałąź `else` łapie 0 i każdą inną liczbę: obcięcie jest zachowaniem bezpiecznym |
| `color = clamp(color, 0.0, 1.0);` | tryb `None`. Samo `linearToSrgb` też przycina, więc ta linia nie zmienia obrazu. Zapisuje wprost, co ten tryb robi |
| `fragColor = vec4(linearToSrgb(color), 1.0);` | krok 5, jedyne kodowanie klatki. Alfa 1: okno nie jest przezroczyste |

`if` na uniformie nie kosztuje tyle co `if` na danych piksela: warunek jest ten sam dla wszystkich fragmentów klatki. Dotyczy to obu warunków, `uBloomEnabled` i `uToneMapping`.

### 4.3 `preview.frag`

```glsl
#version 410 core
// Fragment shader of the attachment previews: turns the colour texture or the depth
// texture of the scene framebuffer into a small picture the debug UI can show as it is.
// Used with post/composite.vert.
// See docs/modules/renderer/post-process.md

// linearToSrgb and linearDepth. The paths are relative to this file.
#include "../common/color.glsl"
#include "../common/depth.glsl"

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// The attachment to show (the number of a texture unit): the colour texture in mode 0,
// the depth texture in mode 1.
uniform sampler2D uSource;

// What uSource is. The numbers are the values of game::AttachmentPreview in C++.
//   0: HDR colour
//   1: depth
uniform int uMode;

// For the depth: the clipping planes of the camera and the distance in metres that is
// shown as white.
uniform float uNear;
uniform float uFar;
uniform float uDepthRange;

// Output: the color written to the preview texture (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    if (uMode == 1) {
        // The stored depth as it is would be an almost white picture: everything
        // farther than a few metres is above 0.95 (see linearDepth). So it is turned
        // back into metres and shown from black (at the camera) to white (uDepthRange
        // metres away or more). The sky, at the far plane, is white. The grey is
        // written as it is: it is a measure, not light, so it is not encoded.
        float metres = linearDepth(texture(uSource, vUv).r, uNear, uFar);
        fragColor = vec4(vec3(clamp(metres / uDepthRange, 0.0, 1.0)), 1.0);
    } else {
        // The colour attachment holds linear values that may be above 1. The debug UI
        // draws a texture without any conversion, so the picture is encoded here.
        // Without exposure and tone mapping: this is the content of the buffer, with
        // everything above 1 cut off, not the finished frame.
        fragColor = vec4(linearToSrgb(texture(uSource, vUv).rgb), 1.0);
    }
}
```

| Linia | Znaczenie |
|---|---|
| `uniform sampler2D uSource;` | jeden sampler dla obu trybów. Tekstura głębi czytana przez `sampler2D` zwraca głębię w kanale czerwonym |
| `uniform int uMode;` | 0 albo 1, wartości `enum class AttachmentPreview` |
| `uNear`, `uFar` | płaszczyzny przycinania kamery, którą narysowano scenę. Muszą być te same, inaczej metry wyjdą złe |
| `texture(uSource, vUv).r` | głębia zapisana w teksturze, od 0 do 1 |
| `linearDepth(..., uNear, uFar)` | z powrotem na metry (sekcje 2.10 i 4.4) |
| `clamp(metres / uDepthRange, 0.0, 1.0)` | 0 m to czerń, `uDepthRange` metrów i więcej to biel |
| `vec4(vec3(szarość), 1.0)` | ta sama liczba w trzech kanałach. Bez `linearToSrgb` |
| gałąź `else` | podgląd koloru: samo kodowanie, które przy okazji przycina do 1 |

Podgląd ma 320 x 180 pikseli, a bufor sceny 1280 x 720 albo więcej, więc tu filtr liniowy tekstury koloru **pracuje**: każdy piksel podglądu jest mieszanką czterech sąsiednich tekseli sceny (przy pomniejszeniu większym niż dwukrotne część tekseli jest pomijana, bo załącznik nie ma mipmap). Głębia jest czytana filtrem najbliższego sąsiada.

### 4.4 `common/depth.glsl`

```glsl
float linearDepth(float stored, float near, float far) {
    float ndc = 2.0 * stored - 1.0;
    return 2.0 * near * far / (far + near - ndc * (far - near));
}
```

Plik nie ma linii `#version`: nie jest shaderem, tylko tekstem wklejanym w miejsce `#include`. Dwie linie funkcji to kroki 3 i 4 wyprowadzenia z sekcji 2.10. Komentarz nad funkcją podaje przykład "ściana 2 m dalej ma już 0,95": zgadza się z tabelą (0,9510 dla płaszczyzn 0,1 m i 100 m). Dziś jedynym użytkownikiem jest `preview.frag`. Bloom głębi nie czyta.

### 4.5 Strona C++: kto ustawia uniformy

Nazwy uniformów są stałymi w [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp):

| Uniform | Stała | Kto ustawia | Skąd wartość |
|---|---|---|---|
| `uScene` | `COMPOSITE_SCENE_UNIFORM` | `PostProcess::composite`, `setInt` | `SOURCE_TEXTURE_UNIT`, czyli 0 |
| `uBloom` | `COMPOSITE_BLOOM_UNIFORM` | `PostProcess::composite`, `setInt` | `BLOOM_TEXTURE_UNIT`, czyli 1 |
| `uBloomEnabled` | `COMPOSITE_BLOOM_ENABLED_UNIFORM` | `PostProcess::composite`, `setInt` | 1, gdy `settings.bloom.enabled && m_bloomDrawn`, inaczej 0 |
| `uBloomIntensity` | `COMPOSITE_BLOOM_INTENSITY_UNIFORM` | `PostProcess::composite`, `setFloat` | `BloomSettings::intensity` |
| `uExposure` | `COMPOSITE_EXPOSURE_UNIFORM` | `PostProcess::composite`, `setFloat` | `PostProcessSettings::exposure` (albo 1 w widoku diagnostycznym) |
| `uToneMapping` | `COMPOSITE_TONE_MAPPING_UNIFORM` | `PostProcess::composite`, `setInt` | `PostProcessSettings::toneMapping` rzutowane na `int` |
| `uSource` | `PREVIEW_SOURCE_UNIFORM` | `PostProcess::drawPreviews`, `setInt` | `SOURCE_TEXTURE_UNIT`, czyli 0 |
| `uMode` | `PREVIEW_MODE_UNIFORM` | `PostProcess::drawPreviews`, `setInt`, dwa razy w klatce. Od drugiej części także `PostProcess::drawBloom`, raz | `AttachmentPreview::Color` i `AttachmentPreview::Depth`. W `drawBloom` zawsze `Color` |
| `uScene` w `bright.frag` | `BRIGHT_SCENE_UNIFORM` | `PostProcess::drawBloom`, `setInt` | `SOURCE_TEXTURE_UNIT`, czyli 0 |
| `uThreshold` | `BRIGHT_THRESHOLD_UNIFORM` | `PostProcess::drawBloom`, `setFloat` | `BloomSettings::threshold` |
| `uSource` w `blur.frag` | `BLUR_SOURCE_UNIFORM` | `PostProcess::drawBloom`, `setInt` | `SOURCE_TEXTURE_UNIT`, czyli 0 |
| `uHorizontal` | `BLUR_HORIZONTAL_UNIFORM` | `PostProcess::drawBloom`, `setInt`, dwa razy na iterację | `BLUR_HORIZONTAL` (1) i `BLUR_VERTICAL` (0) |
| `uWeights` | `BLUR_WEIGHTS_UNIFORM` | `PostProcess::drawBloom`, `setFloatArray`, raz na klatkę | siedem liczb z `game::bloomBlurWeights()` |
| `uNear`, `uFar` | `PREVIEW_NEAR_UNIFORM`, `PREVIEW_FAR_UNIFORM` | `PostProcess::drawPreviews`, `setFloat` | `m_camera.nearPlane`, `m_camera.farPlane` |
| `uDepthRange` | `PREVIEW_DEPTH_RANGE_UNIFORM` | `PostProcess::drawPreviews`, `setFloat` | `PostProcessSettings::depthPreviewRange` |

Osiem stałych z pierwszej części i osiem z drugiej (trzy `COMPOSITE_BLOOM_*`, dwie `BRIGHT_*`, trzy `BLUR_*`): plik `ShaderUniforms.hpp` ma dziś 36 stałych z nazwami zwykłych uniformów (i dwie stałe bloku świateł). `uScene` i `uSource` występują w dwóch shaderach każdy i mają po dwie stałe o tym samym napisie. To celowe: stała mówi, **którego** programu dotyczy, a zmiana nazwy w jednym shaderze nie rusza drugiego.

Programy powstają w konstruktorze `NightMazeApp` jako pola `m_compositeShader`, `m_previewShader`, `m_brightPassShader` i `m_blurShader`, wszystkie cztery z tym samym plikiem wierzchołków (`FULLSCREEN_VERTEX_SHADER_FILE`, czyli `shaders/post/composite.vert`). Pliki fragmentów to stałe `BRIGHT_PASS_FRAGMENT_SHADER_FILE` (`shaders/post/bright.frag`) i `BLUR_FRAGMENT_SHADER_FILE` (`shaders/post/blur.frag`).

**Uniformy a przeładowanie shaderów.** `Reload shaders` tworzy nowy obiekt programu, w którym wszystkie uniformy mają wartość 0. Żaden z uniformów tej tabeli nie jest ustawiany "raz na starcie": wszystkie, razem z tablicą wag, są wysyłane w każdej klatce, więc po przeładowaniu obraz jest poprawny od następnej klatki.

### 4.6 `bright.frag`

```glsl
#version 410 core
// Fragment shader of the bright pass, the first pass of the bloom: keeps the light of
// the scene that is brighter than a threshold and writes black everywhere else.
// Used with post/composite.vert.
// See docs/modules/renderer/post-process.md

// luminance. The path is relative to this file.
#include "../common/color.glsl"

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// The picture of the scene: linear HDR colours (GL_RGBA16F). The target of this pass
// is half as large in each direction, so one pixel here lies between four pixels of the
// scene, and the linear filter of the texture returns their average.
uniform sampler2D uScene;

// Brightness (luminance) above which light takes part in the bloom.
uniform float uThreshold;

// Output: the color written to the bright pass texture (red, green, blue, alpha), as
// a linear HDR colour.
out vec4 fragColor;

// A brightness below this counts as black. It keeps the division below away from 0 / 0.
const float MIN_LUMINANCE = 0.0001;

void main() {
    vec3 color = texture(uScene, vUv).rgb;
    float brightness = luminance(color);

    // The share of the brightness that lies above the threshold: 0 for a pixel at or
    // under it, close to 1 for a very bright one. The colour is multiplied by that
    // share, so all three channels shrink by the same factor and the hue stays. There
    // is no jump at the threshold: a pixel just above it keeps almost nothing.
    float share = max(brightness - uThreshold, 0.0) / max(brightness, MIN_LUMINANCE);
    fragColor = vec4(color * share, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `#include "../common/color.glsl"` | po funkcję `luminance` (sekcja 4.8). Z tego samego pliku `composite.frag` bierze `linearToSrgb` |
| `uniform sampler2D uScene;` | ta sama nazwa co w `composite.frag`, ale to inny program, więc osobny uniform i osobna stała w C++ (`BRIGHT_SCENE_UNIFORM`) |
| komentarz o czterech pikselach | cel ma połowę rozmiaru w każdym kierunku, więc środek jego piksela wypada na styku czterech pikseli sceny i filtr liniowy zwraca ich średnią. To prawda dla **parzystych** rozmiarów sceny. Przy nieparzystym (1281 daje cel 640) środki nie trafiają dokładnie w styk i wagi czterech pikseli nie są równe |
| `uniform float uThreshold;` | próg z suwaka `Threshold`, w liniowych wartościach bufora |
| `const float MIN_LUMINANCE = 0.0001;` | zabezpieczenie mianownika (sekcja 2.12) |
| `vec3 color = texture(uScene, vUv).rgb;` | kolor sceny, już uśredniony z czterech pikseli |
| `float brightness = luminance(color);` | jedna liczba: jasność tej średniej |
| `max(brightness - uThreshold, 0.0)` | nadwyżka ponad próg. `max` z zerem sprawia, że piksel pod progiem daje 0, a nie liczbę ujemną. Ujemne światło w buforze zmiennoprzecinkowym przetrwałoby rozmycie i **przyciemniało** otoczenie |
| `/ max(brightness, MIN_LUMINANCE)` | dzielenie przez jasność: z nadwyżki robi się udział, przez który mnożony jest cały kolor |
| `fragColor = vec4(color * share, 1.0);` | trzy kanały pomnożone przez tę samą liczbę: barwa zostaje. Wynik jest liniowy i może być większy niż 1, dlatego cel jest `GL_RGBA16F` |

Shader nie ma żadnej gałęzi `if`: `max` robi to samo bez skoku. Przeliczone przykłady są w tabeli sekcji 2.12.

### 4.7 `blur.frag`

```glsl
#version 410 core
// Fragment shader of the blur passes of the bloom: one direction of a Gaussian blur.
// Used with post/composite.vert.
// See docs/modules/renderer/post-process.md

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// How many pixels are read on each side of this one. The same number as
// game::BLOOM_BLUR_RADIUS in C++ (src/game/Bloom.hpp): the two must agree.
const int BLUR_RADIUS = 6;

// The picture to blur: linear HDR colours (GL_RGBA16F), read with a linear filter and
// clamped to the edge, so a read past the border repeats the border pixel.
uniform sampler2D uSource;

// The direction of this pass: 1 reads the neighbours to the left and to the right,
// 0 the ones below and above.
uniform int uHorizontal;

// The weights of the kernel, computed in C++ (game::bloomBlurWeights) from the
// Gaussian function: element 0 for this pixel, element d for each of the two pixels
// d pixels away. Together (the centre once, the others twice) they add up to 1.
uniform float uWeights[BLUR_RADIUS + 1];

// Output: the color written to the blur target (red, green, blue, alpha), as a linear
// HDR colour.
out vec4 fragColor;

void main() {
    // A two dimensional Gaussian blur of 13 x 13 pixels would need 169 texture reads.
    // The Gaussian function is SEPARABLE: blurring the rows first and then the columns
    // of the result gives the same picture with 13 + 13 reads. This shader is one of
    // those two passes.

    // The step to the next pixel in texture coordinates: 1 / size. textureSize returns
    // the size of level 0 of the texture in pixels.
    vec2 texel = 1.0 / vec2(textureSize(uSource, 0));
    vec2 texelStep = uHorizontal == 1 ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    // The weighted sum: this pixel, then the pairs of neighbours at distance 1, 2, ...
    vec3 sum = texture(uSource, vUv).rgb * uWeights[0];
    for (int pixels = 1; pixels <= BLUR_RADIUS; ++pixels) {
        vec2 offset = texelStep * float(pixels);
        sum += texture(uSource, vUv + offset).rgb * uWeights[pixels];
        sum += texture(uSource, vUv - offset).rgb * uWeights[pixels];
    }
    fragColor = vec4(sum, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `const int BLUR_RADIUS = 6;` | promień jądra. Stała GLSL, a nie uniform, bo od niej zależy **rozmiar tablicy** `uWeights`, a rozmiar tablicy musi być znany przy kompilacji shadera. Ta sama liczba stoi w C++ jako `game::BLOOM_BLUR_RADIUS`: dwa miejsca, które trzeba zmieniać razem (pułapka 21) |
| `uniform sampler2D uSource;` | obraz do rozmycia: `m_brightPass`, `m_blurHorizontal` albo `m_bloom`, zależnie od przebiegu. "Clamped to the edge" odnosi się do parametru `GL_CLAMP_TO_EDGE` tekstur załączników |
| `uniform int uHorizontal;` | kierunek przebiegu. Jeden shader obsługuje oba kierunki: różnią się tylko tym, w którą stronę idzie krok |
| `uniform float uWeights[BLUR_RADIUS + 1];` | tablica siedmiu liczb zmiennoprzecinkowych. Rozmiar to wyrażenie stałe, `6 + 1`. To jedyna **tablica uniformów** w shaderach gry ustawiana przez `glUniform1fv` ([`../gfx/uniforms.md`](../gfx/uniforms.md)) |
| `textureSize(uSource, 0)` | wbudowana funkcja GLSL: rozmiar poziomu 0 tekstury w pikselach, jako `ivec2`. Dzięki niej shader nie potrzebuje osobnego uniformu z rozmiarem i sam nadąża za zmianą rozmiaru okna |
| `vec2 texel = 1.0 / vec2(...)` | krok o jeden piksel we współrzędnych tekstury. Dla celu 640 x 360 to (0,0015625, 0,0027778). Rzutowanie na `vec2` jest konieczne: `1.0 / ivec2` nie skompilowałoby się |
| `uHorizontal == 1 ? vec2(texel.x, 0.0) : vec2(0.0, texel.y)` | krok tylko w jednej osi: w prawo o piksel albo w górę o piksel |
| `vec3 sum = texture(uSource, vUv).rgb * uWeights[0];` | środek jądra, raz. Waga 0,1370 |
| pętla od 1 do `BLUR_RADIUS` | sześć obrotów, w każdym dwa odczyty: piksel `pixels` kroków w jedną stronę i tyle samo w drugą, oba z tą samą wagą (symetria). Razem `1 + 6 * 2 = 13` odczytów. Granica pętli jest stałą, więc kompilator może ją rozwinąć |
| `vUv + offset`, `vUv - offset` | przesunięcie o całkowitą liczbę pikseli, więc odczyt trafia w środek teksela i filtr liniowy niczego nie miesza. Poza krawędzią tekstury zwracany jest piksel brzegowy |
| `fragColor = vec4(sum, 1.0);` | suma ważona. Wagi sumują się do 1, więc jednolicie jasny obszar zostaje tak samo jasny |

Czego ten shader **nie** robi: nie korzysta z triku, w którym dwa sąsiednie odczyty zastępuje się jednym, postawionym między tekselami, żeby filtr liniowy policzył średnią ważoną za darmo (7 odczytów zamiast 13). Tu każdy odczyt jest jednym tekselem z jedną wagą, tak jak we wzorze.

### 4.8 `common/color.glsl`: `luminance`

Druga część M7 dopisała na końcu pliku stałą i funkcję:

```glsl
// How much each colour channel adds to the brightness the eye sees (luminance): the
// weights of the Rec. 709 standard, the one sRGB takes its red, green and blue from.
// They add up to 1. Green counts most and blue least: the eye is most sensitive to
// green light. They are for LINEAR colours.
const vec3 REC709_LUMINANCE_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

// The brightness of a linear colour as one number: 0 for black, 1 for the white of the
// screen, more than 1 for HDR colours brighter than that.
float luminance(vec3 linear) {
    return dot(linear, REC709_LUMINANCE_WEIGHTS);
}
```

| Linia | Znaczenie |
|---|---|
| `const vec3 REC709_LUMINANCE_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);` | trzy wagi normy Rec. 709, w kolejności czerwony, zielony, niebieski. Suma 1 |
| `dot(linear, REC709_LUMINANCE_WEIGHTS)` | iloczyn skalarny: `R * 0,2126 + G * 0,7152 + B * 0,0722`. Suma ważona zapisana jednym wywołaniem |
| nazwa parametru `linear` | przypomnienie, że funkcja ma sens tylko dla wartości liniowych |

Jedynym użytkownikiem jest `bright.frag`. Plik `color.glsl` dołączają też shadery sceny, `composite.frag` i `preview.frag`: dostają stałą i funkcję, których nie wołają, co nic nie kosztuje (kompilator usuwa nieużywany kod). Odpowiednika tej funkcji w C++ nie ma i żaden test jej nie sprawdza.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/game/PostProcess.hpp`](../../../src/game/PostProcess.hpp) | `enum class ToneMapping`, `enum class AttachmentPreview`, `struct PostProcessSettings` (z polem `bloom`), klasa `PostProcess` |
| [`src/game/PostProcess.cpp`](../../../src/game/PostProcess.cpp) | siedem stałych, funkcje pomocnicze `fitTarget` i `previewWidthFor`, implementacja klasy z `drawBloom` |
| [`src/game/Bloom.hpp`](../../../src/game/Bloom.hpp), [`Bloom.cpp`](../../../src/game/Bloom.cpp) | druga część M7: stałe bloomu, `struct BloomSettings`, funkcje `bloomTargetExtent` i `bloomBlurWeights`. Bez OpenGL, w bibliotece `game_logic` (sekcja 5.10) |
| [`assets/shaders/post/composite.vert`](../../../assets/shaders/post/composite.vert) | trójkąt pełnoekranowy, wspólny dla czterech programów |
| [`assets/shaders/post/composite.frag`](../../../assets/shaders/post/composite.frag) | dodanie bloomu, ekspozycja, mapowanie tonów, kodowanie do sRGB |
| [`assets/shaders/post/preview.frag`](../../../assets/shaders/post/preview.frag) | podgląd koloru i głębi. Tryb koloru służy też podglądom bloomu |
| [`assets/shaders/post/bright.frag`](../../../assets/shaders/post/bright.frag) | druga część M7: przebieg jasności (sekcja 4.6) |
| [`assets/shaders/post/blur.frag`](../../../assets/shaders/post/blur.frag) | druga część M7: jeden kierunek rozmycia Gaussa (sekcja 4.7) |
| [`assets/shaders/common/depth.glsl`](../../../assets/shaders/common/depth.glsl) | `linearDepth` |
| [`assets/shaders/common/color.glsl`](../../../assets/shaders/common/color.glsl) | `srgbToLinear`, `linearToSrgb` w GLSL ([`../gfx/color-space.md`](../gfx/color-space.md)) i, od drugiej części, `luminance` (sekcja 4.8) |
| [`src/gfx/Framebuffer.hpp`](../../../src/gfx/Framebuffer.hpp), [`.cpp`](../../../src/gfx/Framebuffer.cpp) | obiekt framebuffera ([`../gfx/framebuffers.md`](../gfx/framebuffers.md)) |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | pola `m_compositeShader`, `m_previewShader`, `m_brightPassShader`, `m_blurShader`, `m_postProcess`, `m_postProcessSettings`, akcesory `compositeShader()`, `previewShader()`, `brightPassShader()`, `blurShader()`, `postProcessSettings()`, `postProcess()`, funkcja `crystalEmissive`, stała `NEUTRAL_EXPOSURE`, kolejność klatki w `onRender` |
| [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp) | szesnaście nazw uniformów tych przebiegów (sekcja 4.5) |
| [`src/game/Crystals.hpp`](../../../src/game/Crystals.hpp) | `CRYSTAL_GLOW_STRENGTH`, w drugiej części podniesione z 2,5 do 4,0 (sekcja 2.15) |
| [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp), [`.cpp`](../../../src/gfx/Shader.cpp) | nowa metoda `setFloatArray` (`glUniform1fv`), którą wysyłane są wagi ([`../gfx/shader-class.md`](../gfx/shader-class.md), [`../gfx/uniforms.md`](../gfx/uniforms.md)) |
| [`src/debug/panels/FramebuffersPanel.hpp`](../../../src/debug/panels/FramebuffersPanel.hpp), [`.cpp`](../../../src/debug/panels/FramebuffersPanel.cpp) | panel Framebuffers (sekcja 6) |
| [`src/debug/DebugContext.hpp`](../../../src/debug/DebugContext.hpp), [`src/debug/DebugUI.cpp`](../../../src/debug/DebugUI.cpp), [`src/main.cpp`](../../../src/main.cpp) | cztery pola kontekstu z pierwszej części (`compositeShader`, `previewShader`, `postProcessSettings`, `postProcess`) i dwa z drugiej (`brightPassShader`, `blurShader`), `SHADER_COUNT = 10`, zerowanie flagi `previews`, wywołanie panelu ([`../debug-ui.md`](../debug-ui.md)) |
| [`tests/BloomTests.cpp`](../../../tests/BloomTests.cpp) | siedem przypadków testowych dla `Bloom.*` (sekcja 5.8) |

`PostProcess.*` jest na liście źródeł programu `night_maze` w [`CMakeLists.txt`](../../../CMakeLists.txt), obok pozostałych klas rysujących: potrzebuje okna i kontekstu OpenGL, więc nie należy do bibliotek, które da się testować. `Framebuffer.*` i `ColorSpace.*` są w bibliotece `engine`. `Bloom.*` jest w bibliotece `game_logic`: to powód, dla którego ustawienia i matematyka bloomu są osobnym plikiem, a nie częścią `PostProcess.*`. Program testowy może linkować bibliotekę, a nie może linkować kodu, który siedzi w innym programie.

### 5.2 Ustawienia i dwa wyliczenia

```cpp
enum class ToneMapping {
    None = 0,     ///< no curve: everything above 1 is cut off (clamped)
    Reinhard = 1, ///< x / (1 + x): nothing is cut off, the picture gets flatter
    Aces = 2,     ///< a fitted film curve: more contrast, bright values bend softly to 1
};

enum class AttachmentPreview {
    Color = 0, ///< the HDR colour attachment
    Depth = 1, ///< the depth attachment, as a distance
};

struct PostProcessSettings {
    float exposure = 1.0F;
    ToneMapping toneMapping = ToneMapping::Aces;
    bool previews = false;
    float depthPreviewRange = 15.0F;
    BloomSettings bloom;
};
```

(Komentarze nad polami struktury są tu pominięte, ich treść jest w tabeli.)

| Element | Dlaczego tak |
|---|---|
| jawne wartości `= 0`, `= 1`, `= 2` | liczby są umową z shaderem (`uToneMapping`, `uMode`) i z kolejnością wpisów listy w panelu. Zapisane wprost, żeby dopisanie wpisu w środku nie przesunęło pozostałych po cichu |
| `exposure = 1.0F` | mnożenie przez 1 nic nie zmienia. Światła sceny są dobrane pod tę wartość |
| `toneMapping = ToneMapping::Aces` | krzywa domyślna ([`../../decisions/aces-default-tone-mapping.md`](../../decisions/aces-default-tone-mapping.md)) |
| `previews = false` | podglądy kosztują dwa małe przebiegi, więc są rysowane tylko wtedy, gdy ktoś na nie patrzy. Pole ustawia interfejs debugowania **co klatkę** (sekcja 6.2) |
| `depthPreviewRange = 15.0F` | 15 metrów jako biel: siedem i pół komórki labiryntu, czyli mniej więcej to, co widać w korytarzu |
| `BloomSettings bloom;` | struktura w strukturze: cztery ustawienia bloomu z `game/Bloom.hpp` (sekcja 5.10). Jest polem, a nie osobnym argumentem funkcji, więc kopia ustawień dla widoku diagnostycznego (sekcja 2.9) obejmuje ją automatycznie |

Struktura jest polem `NightMazeApp::m_postProcessSettings`, a panel edytuje ją przez referencję z `DebugContext`. Komentarz przy polu `previews` mówi nadal o "dwóch obrazach" i "dwóch małych przebiegach". Od drugiej części ta sama flaga włącza też dwa podglądy bloomu, więc obrazów jest do czterech.

### 5.3 Klasa `PostProcess`

```cpp
class PostProcess {
public:
    PostProcess() = default;

    bool beginScene(core::Size size);

    void drawPreviews(const gfx::Shader& shader, const PostProcessSettings& settings,
                      float nearPlane, float farPlane);

    void drawBloom(const gfx::Shader& brightShader, const gfx::Shader& blurShader,
                   const gfx::Shader& previewShader, const PostProcessSettings& settings);

    void composite(const gfx::Shader& shader, const PostProcessSettings& settings,
                   core::Size windowSize) const;

    const gfx::Framebuffer& sceneTarget() const { return m_scene; }

    const gfx::Framebuffer& preview(AttachmentPreview which) const {
        return which == AttachmentPreview::Depth ? m_depthPreview : m_colorPreview;
    }

    bool bloomDrawn() const { return m_bloomDrawn; }

    const gfx::Framebuffer& bloomTarget() const { return m_bloom; }

    const gfx::Framebuffer& brightPassPreview() const { return m_brightPassPreview; }
    const gfx::Framebuffer& bloomPreview() const { return m_bloomPreview; }

private:
    void drawFullscreenTriangle() const;

    gfx::Framebuffer m_scene;
    core::Size m_requestedSize;

    gfx::Framebuffer m_colorPreview;
    gfx::Framebuffer m_depthPreview;

    gfx::Framebuffer m_brightPass;
    gfx::Framebuffer m_blurHorizontal;
    gfx::Framebuffer m_bloom;
    bool m_bloomDrawn = false;

    gfx::Framebuffer m_brightPassPreview;
    gfx::Framebuffer m_bloomPreview;

    gfx::VertexArray m_triangle;
};
```

(Komentarze Doxygen są tu pominięte. Ich treść jest omówiona w sekcjach 5.4 do 5.6 i 5.11.)

| Element | Dlaczego tak |
|---|---|
| `PostProcess() = default;` | konstruktor nie tworzy framebufferów: rozmiar okna nie jest jeszcze potrzebny. Osiem pól `gfx::Framebuffer` zaczyna jako obiekty puste (`isValid()` zwraca fałsz). Jedyny obiekt OpenGL, który powstaje od razu, to `m_triangle`: konstruktor domyślny `gfx::VertexArray` tworzy VAO |
| programy shaderów jako parametry, nie pola | programy są polami `NightMazeApp`, tak jak wszystkie pozostałe, bo panel Shaders przeładowuje je z jednej listy. Ten sam układ ma `game::Skybox` |
| `beginScene` zwraca `bool` | wołający musi wiedzieć, czy jest do czego rysować |
| `composite` jest `const`, `drawPreviews` i `drawBloom` nie | `drawPreviews` i `drawBloom` tworzą i zmieniają rozmiar framebufferów (pola obiektu), a `drawBloom` zapisuje też `m_bloomDrawn`. `composite` zmienia tylko stan kontekstu OpenGL |
| `drawBloom` bierze trzy programy | przebieg jasności, rozmycie i program podglądu dla dwóch małych obrazów. Programy są parametrami z tego samego powodu co wyżej |
| `sceneTarget()` | dla przyszłych przebiegów, które czytają kolor albo głębię sceny, i dla panelu (rozmiar i formaty) |
| `preview(which)` | panel dostaje framebuffer podglądu i pokazuje jego teksturę koloru |
| `bloomDrawn()` | jedna informacja dla dwóch odbiorców: `composite` (czy dodać bloom) i panel (czy pokazać obrazy i rozmiar). Fałsz przed pierwszym `drawBloom` |
| `bloomTarget()` | cel z gotową poświatą, dla panelu: rozmiar i format. Nieważny (`isValid()` fałsz), dopóki `drawBloom` ani razu nie rysowało |
| `brightPassPreview()`, `bloomPreview()` | dwa małe obrazy `GL_RGBA8` dla panelu, odpowiedniki `preview(which)` |
| trzy cele: `m_brightPass`, `m_blurHorizontal`, `m_bloom` | wszystkie `GL_RGBA16F`, bez głębi, tego samego rozmiaru (połowa sceny w każdym kierunku). Role opisuje sekcja 2.14 |
| `bool m_bloomDrawn = false;` | czy **ostatnie** `drawBloom` narysowało bloom. Pole obiektu, a nie wartość zwracana, bo czytają je dwie osobne funkcje później w klatce |
| `m_requestedSize` osobno od rozmiaru `m_scene` | sekcja 5.4 |
| `gfx::VertexArray m_triangle;` | pusty VAO dla trójkąta pełnoekranowego (sekcja 2.4) |

Kopiowanie klasy nie jest zadeklarowane i nie jest możliwe: pola `gfx::Framebuffer` i `gfx::VertexArray` mają kopiowanie usunięte, więc kompilator nie wygeneruje go też dla `PostProcess`. Obiekt jest polem `NightMazeApp::m_postProcess`, ostatnim z obiektów OpenGL (po `m_skybox`), więc powstaje po oknie i ginie przed nim.

Stałe pliku `.cpp`:

```cpp
// The texture unit the passes read their input from. Every pass binds what it needs.
constexpr GLuint SOURCE_TEXTURE_UNIT = 0;

// The composite pass reads two pictures: the scene from the unit above and the bloom
// from this one.
constexpr GLuint BLOOM_TEXTURE_UNIT = 1;

// The values of the uniform uHorizontal in post/blur.frag: the direction of a blur pass.
constexpr int BLUR_HORIZONTAL = 1;
constexpr int BLUR_VERTICAL = 0;

// One triangle: three vertices, starting with number 0.
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

// Height of a preview picture in pixels. The width follows from the shape of the
// window. Small on purpose: the debug UI shows the pictures at about this size.
constexpr int PREVIEW_HEIGHT = 180;
```

Siedem stałych: trzy doszły w drugiej części. `BLUR_HORIZONTAL` i `BLUR_VERTICAL` to nazwy dla liczb 1 i 0, które shader porównuje z `uHorizontal`. Bez nich w kodzie stałoby `setInt(..., 1)` i trzeba by pamiętać, co jedynka znaczy.

### 5.4 `beginScene`

```cpp
bool PostProcess::beginScene(core::Size size) {
    if (size.width < 1 || size.height < 1) {
        return false;
    }

    // A new size: the first frame, or the window was resized.
    if (size.width != m_requestedSize.width || size.height != m_requestedSize.height) {
        m_requestedSize = size;
        // GL_RGBA16F for the colour: linear values that may pass 1 (HDR). A depth
        // texture and not a renderbuffer, so that later passes can read the depth.
        // A new object instead of resize(): it also covers the first frame and a
        // framebuffer that could not be created at the size before.
        m_scene = gfx::Framebuffer({.width = size.width,
                                    .height = size.height,
                                    .color = gfx::ColorFormat::Rgba16F,
                                    .depth = gfx::DepthFormat::Depth24});
    }
    if (!m_scene.isValid()) {
        return false;
    }

    m_scene.bind();
    return true;
}
```

| Linia | Znaczenie |
|---|---|
| `if (size.width < 1 \|\| size.height < 1) return false;` | zminimalizowane okno. `onRender` sprawdza to samo wcześniej, więc tutaj jest to druga linia obrony: funkcja nie polega na wołającym |
| porównanie z `m_requestedSize` | "czy prośba jest inna niż poprzednia", a nie "czy bufor ma inny rozmiar". Różnica ma znaczenie po porażce: nieudany framebuffer ma rozmiar 0 x 0, więc porównanie z rozmiarem `m_scene` kazałoby próbować od nowa w **każdej** klatce i zasypywałoby log. Z `m_requestedSize` błąd jest wypisany raz na rozmiar |
| `m_requestedSize = size;` przed tworzeniem | zapamiętane także wtedy, gdy tworzenie się nie uda |
| `m_scene = gfx::Framebuffer({...});` | nowy obiekt i przypisanie przenoszące: stary framebuffer i jego tekstury są usuwane, nowy zajmuje ich miejsce. Inicjalizatory z nazwami pól (`.width = ...`) to składnia C++20 |
| dlaczego nie `m_scene.resize(...)` | komentarz podaje powód: `resize` nic nie robi dla obiektu, który nie jest ważny, a tu trzeba obsłużyć także pierwszą klatkę i odbudowę po nieudanej próbie |
| `if (!m_scene.isValid()) return false;` | sterownik odmówił (błąd jest w logu). Klatka zostanie pominięta |
| `m_scene.bind();` | `glBindFramebuffer` i `glViewport` na rozmiar bufora ([`../gfx/framebuffers.md`](../gfx/framebuffers.md)) |

Rozmiar pochodzi z `window().framebufferSize()`, czyli z pikseli, a nie ze współrzędnych ekranu. Na wyświetlaczu Retina framebuffer okna 1280 x 720 ma 2560 x 1440 pikseli i taki musi być bufor sceny. Tego przypadku nikt nie uruchomił (macOS otwarty).

### 5.5 `fitTarget`, `previewWidthFor` i `drawPreviews`

```cpp
void fitTarget(gfx::Framebuffer& target, int width, int height, gfx::ColorFormat format) {
    if (!target.isValid()) {
        // Colour only: one flat triangle needs no depth test.
        target = gfx::Framebuffer(
            {.width = width, .height = height, .color = format, .depth = gfx::DepthFormat::None});
    } else {
        target.resize(width, height);
    }
}

int previewWidthFor(const gfx::Framebuffer& scene) {
    const float aspectRatio =
        static_cast<float>(scene.width()) / static_cast<float>(scene.height());
    return std::max(
        1, static_cast<int>(std::lround(static_cast<float>(PREVIEW_HEIGHT) * aspectRatio)));
}
```

Dwie funkcje z anonimowej przestrzeni nazw. `fitTarget` tworzy framebuffer przy pierwszym użyciu, a potem tylko dopasowuje rozmiar (`resize` nic nie robi, gdy rozmiar się nie zmienił). W pierwszej części nazywała się `fitPreview` i miała format `GL_RGBA8` wpisany na stałe. Druga część dodała argument `format`, bo tej samej funkcji używają teraz cele bloomu (`Rgba16F`) i podglądy (`Rgba8`). Zawsze bez głębi: w każdy taki cel rysowany jest jeden płaski trójkąt. `previewWidthFor` to wydzielony rachunek szerokości podglądu, wcześniej wpisany w `drawPreviews`, a dziś potrzebny też w `drawBloom`.

Jedna własność `fitTarget`, o której warto wiedzieć: gdy sterownik odmówi utworzenia framebuffera, obiekt zostaje nieważny i następna klatka próbuje od nowa. Bufor sceny ma na to zabezpieczenie (`m_requestedSize`, sekcja 5.4), cele bloomu i podglądy go nie mają, więc w takiej sytuacji błąd byłby wypisywany w logu co klatkę. Nikt takiego przypadku nie zgłosił.

```cpp
void PostProcess::drawPreviews(const gfx::Shader& shader, const PostProcessSettings& settings,
                               float nearPlane, float farPlane) {
    if (!m_scene.isValid() || !shader.isValid()) {
        return;
    }

    const int previewWidth = previewWidthFor(m_scene);
    fitTarget(m_colorPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    fitTarget(m_depthPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    if (!m_colorPreview.isValid() || !m_depthPreview.isValid()) {
        return;
    }

    // One flat triangle per picture: nothing to test the depth against.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    shader.use();
    shader.setInt(PREVIEW_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    shader.setFloat(PREVIEW_NEAR_UNIFORM, nearPlane);
    shader.setFloat(PREVIEW_FAR_UNIFORM, farPlane);
    shader.setFloat(PREVIEW_DEPTH_RANGE_UNIFORM, settings.depthPreviewRange);

    // The colour attachment. Reading a texture of the scene framebuffer is allowed
    // here because another framebuffer is the target: a pass must never read the
    // texture it is drawing into.
    m_colorPreview.bind();
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    shader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Color));
    drawFullscreenTriangle();

    // The depth attachment.
    m_depthPreview.bind();
    m_scene.bindDepthTexture(SOURCE_TEXTURE_UNIT);
    shader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Depth));
    drawFullscreenTriangle();
}
```

| Linia | Znaczenie |
|---|---|
| pierwszy `return` | bez sceny albo bez działającego programu (błąd kompilacji shadera) nie ma czego rysować |
| `aspectRatio` z rzutowaniami (w `previewWidthFor`) | dzielenie liczb całkowitych `1280 / 720` dałoby 1 |
| `std::lround(180 * aspectRatio)` | zaokrąglenie do najbliższej liczby całkowitej: 320 dla 16:9, 315 dla okna 1400 x 800, 300 dla okna 1000 x 600 |
| `std::max(1, ...)` | bardzo wąskie okno nie może dać szerokości 0: framebuffer o rozmiarze 0 nie powstanie |
| `fitTarget(..., gfx::ColorFormat::Rgba8)` | format `GL_RGBA8`, bo obraz podglądu jest już gotowy do pokazania |
| drugi `return` | któryś podgląd się nie utworzył |
| `glDisable(GL_DEPTH_TEST)` | podglądy nie mają załącznika głębi, a trójkąt nie ma czego zasłaniać. Test zostaje wyłączony także po funkcji |
| `shader.use()` i cztery uniformy | wspólne dla obu obrazów, ustawione raz |
| `m_colorPreview.bind();` **przed** `m_scene.bindColorTexture(...)` | najpierw zmiana celu, potem odczyt. Gdyby celem nadal był bufor sceny, shader czytałby teksturę, do której rysuje (pętla zwrotna, sekcja 2.1) |
| `setInt(PREVIEW_MODE_UNIFORM, ...)` dwa razy | ten sam program, dwa tryby |
| `drawFullscreenTriangle()` | sekcja 5.6 |

Komentarz w nagłówku ostrzega: funkcja zostawia związany jeden z framebufferów podglądu, więc po niej **musi** nastąpić `composite` albo inne wiązanie. W `onRender` następuje: najpierw `drawBloom`, które wiąże własne cele, potem `composite`.

### 5.6 `composite` i `drawFullscreenTriangle`

```cpp
void PostProcess::composite(const gfx::Shader& shader, const PostProcessSettings& settings,
                            core::Size windowSize) const {
    // From here on everything lands in the window: this pass, and the debug UI after it.
    gfx::Framebuffer::bindDefault(windowSize.width, windowSize.height);
    if (!m_scene.isValid() || !shader.isValid()) {
        return;
    }

    // The triangle covers every pixel, so the window is not cleared first. The depth
    // test has to be off: the depth buffer of the window is never cleared any more and
    // holds whatever it was created with, so a test against it could reject the
    // triangle.
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    // The shader encodes to sRGB itself. With this switch on, OpenGL would encode once
    // more on writing into an sRGB capable window. It is off by default: the line says
    // that the pass relies on it.
    GL_CHECK(glDisable(GL_FRAMEBUFFER_SRGB));

    shader.use();

    // The bloom is added only when it was asked for AND drawBloom has drawn it in this
    // frame. Otherwise the shader does not read the bloom texture at all, so the
    // picture is exactly the one of a frame without bloom. The sampler gets its unit
    // either way.
    const bool addBloom = settings.bloom.enabled && m_bloomDrawn;
    shader.setInt(COMPOSITE_BLOOM_ENABLED_UNIFORM, addBloom ? 1 : 0);
    shader.setInt(COMPOSITE_BLOOM_UNIFORM, static_cast<int>(BLOOM_TEXTURE_UNIT));
    shader.setFloat(COMPOSITE_BLOOM_INTENSITY_UNIFORM, settings.bloom.intensity);
    if (addBloom) {
        m_bloom.bindColorTexture(BLOOM_TEXTURE_UNIT);
    }

    // The sampler of the shader gets the number of the texture unit (glUniform1i), and
    // the colour texture of the scene is bound to that unit.
    shader.setInt(COMPOSITE_SCENE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    shader.setFloat(COMPOSITE_EXPOSURE_UNIFORM, settings.exposure);
    // The enum values are the numbers post/composite.frag compares uToneMapping with.
    shader.setInt(COMPOSITE_TONE_MAPPING_UNIFORM, static_cast<int>(settings.toneMapping));

    drawFullscreenTriangle();
}

void PostProcess::drawFullscreenTriangle() const {
    m_triangle.bind();
    // No buffer and no attribute: the vertex shader makes the corners from gl_VertexID.
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));
}
```

| Linia | Znaczenie |
|---|---|
| `gfx::Framebuffer::bindDefault(...)` **przed** sprawdzeniem `isValid` | okno staje się celem zawsze, także gdy przebieg nie ma czego rysować. ImGui, rysowane zaraz potem, musi trafić do okna, a nie do bufora sceny albo podglądu |
| `return` po sprawdzeniu | bez sceny albo bez programu okno zostaje z tym, co w nim było. Panele nadal się rysują, więc błąd shadera da się przeczytać w panelu Shaders |
| `glDisable(GL_DEPTH_TEST)` | scena zostawia test włączony. Bufor głębi okna nie jest już czyszczony przez nikogo, więc jego zawartość jest przypadkowa |
| `glDisable(GL_FRAMEBUFFER_SRGB)` | sekcja 2.7 |
| `const bool addBloom = settings.bloom.enabled && m_bloomDrawn;` | dwa warunki naraz. Pierwszy: ustawienia tej klatki chcą bloomu (w widoku diagnostycznym kopia mówi "nie"). Drugi: `drawBloom` naprawdę go narysowało. Drugi jest potrzebny, bo `drawBloom` może wrócić wcześniej (zepsuty shader, nieudany cel), a wtedy w `m_bloom` leży obraz z dawnej klatki albo nic |
| `setInt(COMPOSITE_BLOOM_ENABLED_UNIFORM, addBloom ? 1 : 0)` | `bool` z C++ zamieniony na 1 albo 0 dla uniformu `int` |
| `setInt(COMPOSITE_BLOOM_UNIFORM, ...)` poza `if` | sampler dostaje numer jednostki zawsze, także gdy bloom nie jest dodawany (komentarz: "either way"). Skutek: `uScene` i `uBloom` nigdy nie wskazują tej samej jednostki, także zaraz po przeładowaniu shadera, kiedy oba startują z zerem |
| `if (addBloom) m_bloom.bindColorTexture(BLOOM_TEXTURE_UNIT);` | tekstura poświaty na jednostce 1, tylko gdy będzie czytana. `bindColorTexture` zostawia jednostkę 1 jako aktywną, a następne wywołanie dla sceny przełącza aktywną z powrotem na 0 |
| `static_cast<int>(SOURCE_TEXTURE_UNIT)` | stała ma typ `GLuint`, bo taki przyjmuje `bindColorTexture`. `setInt` przyjmuje `int` |
| `static_cast<int>(settings.toneMapping)` | `enum class` nie zamienia się na liczbę samo. Wartości wyliczenia są liczbami, z którymi porównuje shader |
| `m_triangle.bind();` | pusty VAO: profil Core nie rysuje bez związanego |
| `glDrawArrays(GL_TRIANGLES, 0, 3)` | trzy wierzchołki bez danych. `glDrawArrays`, a nie `glDrawElements`: nie ma bufora indeksów |

Funkcja zostawia test głębi wyłączony. Nagłówek mówi to wprost, a `onRender` włącza go na początku każdej klatki.

### 5.7 Miejsce w klatce: `onRender`

Fragmenty `NightMazeApp::onRender`, które należą do tego modułu (kod sceny między nimi jest pominięty i oznaczony komentarzem `// ...`):

```cpp
    const core::Size framebuffer = window().framebufferSize();

    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return;
    }

    if (!m_postProcess.beginScene(framebuffer)) {
        return;
    }

    GL_CHECK(glEnable(GL_DEPTH_TEST));

    const glm::vec3 clearColor =
        gfx::srgbToLinear(glm::vec3{m_clearColor[0], m_clearColor[1], m_clearColor[2]});
    GL_CHECK(glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // ... macierze, światła, drawMaze, drawGrass, linie kolizji, niebo ...

    // The pictures of the attachments, only while the debug UI shows them.
    if (m_postProcessSettings.previews) {
        m_postProcess.drawPreviews(m_previewShader, m_postProcessSettings, m_camera.nearPlane,
                                   m_camera.farPlane);
    }

    PostProcessSettings compositeSettings = m_postProcessSettings;
    if (m_viewMode != ViewMode::Textured) {
        compositeSettings.exposure = NEUTRAL_EXPOSURE;
        compositeSettings.toneMapping = ToneMapping::None;
        compositeSettings.bloom.enabled = false;
    }

    // The bloom: the bright parts of the finished scene, blurred in targets of half the
    // size. It is called in every frame, also with the bloom switched off: it then
    // draws nothing and tells the composite pass so.
    m_postProcess.drawBloom(m_brightPassShader, m_blurShader, m_previewShader, compositeSettings);

    m_postProcess.composite(m_compositeShader, compositeSettings, framebuffer);
```

| Linia | Znaczenie |
|---|---|
| `if (framebuffer.width == 0 \|\| framebuffer.height == 0) return;` | przed tą częścią to sprawdzenie stało **po** `glClear`. Przeniosło się na początek, bo teraz nie ma też do czego rysować: tekstury o rozmiarze 0 nie da się podpiąć do framebuffera. Bufor sceny zachowuje ostatni rozmiar i jest użyty ponownie, gdy okno wróci |
| `if (!m_postProcess.beginScene(framebuffer)) return;` | zastąpiło dawne `glViewport(0, 0, width, height)`: viewport ustawia teraz `Framebuffer::bind` |
| `gfx::srgbToLinear(glm::vec3{...})` | kolor tła jest liczbą sRGB z próbnika w panelu Renderer, a bufor przechowuje wartości liniowe. Startowe (0,022, 0,033, 0,088) to liniowo (0,0017, 0,0026, 0,0083) |
| `glClear(...)` | czyści dwie tekstury bufora sceny |
| `m_camera.nearPlane`, `m_camera.farPlane` | te same pola, z których `projectionMatrix` zbudowało macierz tej klatki |
| `PostProcessSettings compositeSettings = m_postProcessSettings;` | kopia całej struktury: pięć pól, w tym struktura `bloom` z czterema własnymi |
| `if (m_viewMode != ViewMode::Textured)` | każdy widok poza zwykłym obrazem jest widokiem danych (sekcja 2.9) |
| `NEUTRAL_EXPOSURE` | nazwana stała 1,0 z anonimowej przestrzeni nazw pliku |
| `compositeSettings.bloom.enabled = false;` | trzecia rzecz wyłączana dla widoku danych, w tej samej kopii. Pole `Bloom` w panelu zostaje zaznaczone |
| `drawBloom(..., compositeSettings)` | dostaje **kopię**, nie oryginał: dzięki temu widzi wyłączenie dla widoku danych. Kopia niesie też flagę `previews`, więc `drawBloom` wie, czy rysować swoje dwa podglądy |
| `drawBloom` bez `if` wokół | funkcja jest wołana zawsze, bo to ona zeruje `m_bloomDrawn`. Gdyby przy wyłączonym bloomie nie była wołana, pole zostałoby z wartością z poprzedniej klatki |
| `composite(..., compositeSettings, framebuffer)` | te same ustawienia co bloom i ten sam rozmiar, z którym zaczęła się scena, jako rozmiar viewportu okna |

Gdy `onRender` wraca wcześniej (okno 0 x 0 albo brak bufora sceny), ani `drawBloom`, ani `composite` nie są wołane, ale `DebugUI::draw` w `main.cpp` i tak rysuje panele. `m_bloomDrawn` zostaje wtedy z ostatniej narysowanej klatki, więc panel pokazuje ostatnie obrazy. Związany jest wtedy framebuffer z poprzedniej klatki, czyli okno (ostatnie wiązanie zrobiło `composite` albo konstruktor `Framebuffer`, który zostawia domyślny).

Funkcja `crystalEmissive`, wydzielona w pierwszej części M7 z dwóch identycznych wywołań, przelicza kolor światła kryształów na liniowy i podaje go do `crystalGlow`. Opis jest w [`../gfx/color-space.md`](../gfx/color-space.md) i w [`../game/gameplay.md`](../game/gameplay.md).

### 5.8 Testy

Klasa `PostProcess` **nie ma testów jednostkowych**: każda jej funkcja woła OpenGL, a program testowy nie tworzy okna. To samo dotyczy shaderów. Matematykę wokół niej pokrywają cztery pliki:

| Plik | Co sprawdza | Co z tego dotyczy tego modułu |
|---|---|---|
| [`tests/BloomTests.cpp`](../../../tests/BloomTests.cpp) | `game::bloomTargetExtent`, `game::bloomBlurWeights`, wartości startowe `BloomSettings` | wszystko: to jedyne testy napisane dla bloomu. Siedem przypadków, lista niżej |
| [`tests/ColorSpaceTests.cpp`](../../../tests/ColorSpaceTests.cpp) | `gfx::srgbToLinear` i `gfx::linearToSrgb` w C++ | ta sama formuła co `linearToSrgb` w `common/color.glsl`. Test "encoding undoes decoding for every byte of a picture" sprawdza, że dekodowanie i kodowanie znoszą się dla każdego z 256 bajtów z dokładnością lepszą niż ćwierć kroku. Na tym stoi poprawka widoków diagnostycznych (sekcja 2.9). Wersji GLSL test nie uruchamia |
| [`tests/FramebufferTests.cpp`](../../../tests/FramebufferTests.cpp) | nazwy formatów i tekst stanu framebuffera | napisy, które pokazuje linia informacyjna panelu (`GL_RGBA16F`, `GL_DEPTH_COMPONENT24`) |
| [`tests/LightingTests.cpp`](../../../tests/LightingTests.cpp) | `buildLightSet` przelicza cztery kolory z sRGB na liniowe i nie rusza intensywności | wartości, które trafiają do bufora HDR |

Siedem przypadków `BloomTests.cpp`:

| Przypadek | Co sprawdza | Dlaczego to ważne |
|---|---|---|
| `a bloom target is half the scene in each direction` | 1280 daje 640, 720 daje 360, 2560 daje 1280 | podstawowy rachunek rozmiaru |
| `an odd scene size is halved with the rest dropped` | 1281 daje 640, 719 daje 359, 3 daje 1 | dzielenie całkowite: reszta przepada, nie ma zaokrąglania w górę |
| `a bloom target is never smaller than one pixel` | 2, 1 i 0 dają 1 | okno ściągnięte do paska: tekstury o rozmiarze 0 nie da się podpiąć do framebuffera |
| `the blur kernel adds up to one` | waga środka plus dwa razy każda pozostała to 1, z tolerancją 0,00001 | rozmycie nie dodaje i nie gubi światła (sekcja 2.13) |
| `the blur weights fall with the distance and stay above zero` | tablica ma `BLOOM_BLUR_RADIUS + 1` elementów (`REQUIRE`), każda waga jest mniejsza od poprzedniej i dodatnia | kształt dzwonu. `REQUIRE` zamiast `CHECK`: przy złym rozmiarze dalsze indeksowanie nie miałoby sensu |
| `the blur weights follow the Gaussian function` | stosunek każdej wagi do wagi środka to `exp(-d * d / (2 * sigma * sigma))`, a do tego dwie liczby policzone ręcznie: waga 0 to 0,1370, waga 6 to 0,0185 | dzielenie przez sumę skaluje wszystkie wagi tak samo, więc **stosunki** zostają stosunkami dzwonu. Liczby z komentarza testu (1, 0,946, 0,801, 0,607, 0,411, 0,249, 0,135 i suma `1 + 2 * 3,149 = 7,298`) zgadzają się z tabelą w sekcji 2.13 |
| `the bloom settings start inside their ranges` | bloom startuje włączony, próg i intensywność są dodatnie, liczba iteracji mieści się między `MIN_` a `MAX_BLOOM_BLUR_ITERATIONS` | wartości startowe nie mogą leżeć poza zakresem suwaka |

Razem 36 asercji: 3, 3, 3, 1, 13 (jedno `REQUIRE` i sześć obrotów pętli po dwa `CHECK`), 8 (sześć obrotów pętli i dwa `CHECK`) i 5.

Czego **żaden** test nie sprawdza: wzorów Reinharda i ACES (istnieją tylko w GLSL), `linearDepth` (tylko w GLSL), tabeli trójkąta z `gl_VertexID`, kolejności kroków `onRender`, zgodności liczb wyliczenia `ToneMapping` z shaderem i z kolejnością wpisów listy w panelu. Z bloomu: wzoru przebiegu jasności i funkcji `luminance` (tylko w GLSL), pętli rozmycia w shaderze, kolejności ping-ponga w `drawBloom`, dodania w `composite.frag` i tego, że `BLUR_RADIUS` w `blur.frag` jest równe `BLOOM_BLUR_RADIUS` w C++ (pułapka 21). Komentarz na górze pliku testów mówi to wprost: same przebiegi są shaderami i sprawdza się je przez uruchomienie gry. Liczby w sekcjach 2.6, 2.10 i od 2.11 do 2.15 tego dokumentu zostały przeliczone osobnym skryptem z tych samych wzorów, nie odczytane z działającej gry.

### 5.9 Jak to zostało sprawdzone

Wszystko poniżej jest **zgłoszone** przez osobę, która pisała kod, dla Windowsa, 2026-10-05. Przy pisaniu dokumentu nie było powtarzane. Najpierw pierwsza część M7, potem druga.

**Pierwsza część M7 (bufor HDR, przebieg składający):**

- **Bramka.** `make check` przechodziła: formatowanie, testy w Debug i Release, clang-tidy. Zero ostrzeżeń. Wtedy 269 przypadków testowych i 102103 asercje w obu konfiguracjach.
- **Błędy OpenGL.** Build Debug (z `GL_CHECK`) bez błędów przy otwartych podglądach, po zmianie rozmiaru okna na 1400 x 800, po zminimalizowaniu (0 x 0) i po przywróceniu.
- **Porównanie z poprzednim potokiem.** Tryb `Unlit` z `Tone mapping: None` i ekspozycją 1, porównany z poprzednim commitem, **nie** jest identyczny co do piksela: ściany i podłoże różnią się najwyżej o 22 poziomy na 255 (średnio 1,1), tylko na spoinach cegieł. Powód (filtrowanie działa teraz na wartościach liniowych) jest wyjaśniony w [`../gfx/color-space.md`](../gfx/color-space.md). Pomiar zrobiono przed ponownym dobraniem świateł: jasność nieba i świecenie kryształów różnią się dziś celowo. Widoki normalnych i UV różnią się najwyżej o 1 poziom. Podglądy tekstur w panelu Assets są identyczne co do piksela.
- **Wydajność.** Release, bez synchronizacji pionowej, panele ukryte:

| Rozdzielczość | Przed | Po | Zmiana |
|---|---|---|---|
| 1280 x 720 | około 2700 klatek na sekundę | około 2500 | około 7 procent mniej |
| 2560 x 1440 | około 2020 | około 1960 | około 3 procent mniej |

  W czasie klatki to około 0,37 ms przed i 0,40 ms po w mniejszej rozdzielczości. Wersji karty i sterownika nie zapisano, więc liczby mówią o rzędzie wielkości kosztu, a nie o konkretnym sprzęcie.
- **Nie sprawdzone:** macOS i wyświetlacz Retina (nic z tej części nie było tam budowane ani uruchamiane), klikanie nowych kontrolek myszą, `Reload shaders`, zmiana rozmiaru okna przez przeciąganie krawędzi. Ścieżka framebuffera z samą głębią nie została nigdy wykonana (ta klasa jej nie używa). Lista do odhaczenia jest w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 17.
- **Nie zmierzone:** koszt samych podglądów, pamięć karty zajęta przez bufor sceny, różnica między krzywymi na zrzutach ekranu.

**Druga część M7 (bloom):**

- **Bramka.** `make check` przechodzi: formatowanie, buildy Debug i Release z zerem ostrzeżeń, clang-tidy. **276 przypadków testowych i 102139 asercji** w obu konfiguracjach. Przyrost o 7 przypadków i 36 asercji zgadza się z plikiem `tests/BloomTests.cpp` (sekcja 5.8). Suma makr `TEST_CASE` we wszystkich plikach `tests/*Tests.cpp` w stanie tej części to 276: to jedyna z tych liczb, którą policzyłem sam.
- **Bloom wyłączony.** Z odznaczonym polem `Bloom` obraz był identyczny co do piksela z obrazem pierwszej części, w sześciu widokach. Pomiar zrobiono **przed** podniesieniem `CRYSTAL_GLOW_STRENGTH` z 2,5 do 4,0. Dzisiejsza klatka bez bloomu różni się więc od pierwszej części kryształami, i to celowo.
- **Zrzuty ekranu.** Poświata kryształu w najciemniejszej i w najjaśniejszej chwili pulsu, w trybach Unlit, Gouraud i Blinn-Phong, ze ściankami kryształu nadal widocznymi. Poświata tarczy księżyca. Gwiazdy zostają punktami. Próg 0,3 i próg 2,0. Panel z czterema podglądami. Okno zmienione na 1000 x 600 (cele bloomu 500 x 300). Widok normalnych z napisem `(not drawn)` w miejscu obrazów bloomu.
- **Wydajność.** Release, panele ukryte. Pomiar jest niespokojny, dlatego zakresy zamiast jednej liczby:

| Rozdzielczość | Bloom wyłączony | Bloom włączony | W czasie klatki |
|---|---|---|---|
| 1280 x 720 | od 1900 do 2450 klatek na sekundę | od 1500 do 2150 | od 0,41 do 0,53 ms bez, od 0,47 do 0,67 ms z bloomem |
| 2560 x 1440 | od 1370 do 1480 | od 880 do 925 | od 0,68 do 0,73 ms bez, od 1,08 do 1,14 ms z bloomem |

  W większej rozdzielczości bloom kosztuje więc około 0,4 ms na klatkę. W mniejszej zakresy zachodzą na siebie i koszt da się oszacować tylko zgrubnie: rzędu 0,1 ms, w granicach rozrzutu pomiaru. Tych liczb **nie należy** zestawiać z tabelą pierwszej części wyżej: to inne sesje pomiarowe, a sama wartość bez bloomu w 2560 x 1440 (od 1370 do 1480) odbiega od tamtych "około 1960" bardziej, niż wyniósłby koszt jakiejkolwiek zmiany w kodzie. Wersji karty i sterownika nie zapisano. Wniosek, który z tych liczb wynika uczciwie: na tej maszynie bloom mieści się z dużym zapasem w budżecie 60 klatek na sekundę (16,7 ms). Dla MacBooka nie wynika z nich nic.
- **Otwarte obserwacje.** Plama latarki na ścianie nie daje poświaty nawet z odległości metra: ściana w świetle latarki zostaje pod progiem 0,8. Czy tak ma być, to decyzja o wyglądzie, której nikt jeszcze nie podjął (niższy próg zapaliłby też inne rzeczy, a jaśniejsza latarka zmieniłaby całą scenę). Druga: poświata jest mierzona w tekselach celu o połowie rozdzielczości, więc w 2560 x 1440 jest względem ekranu o połowę cieńsza niż w 1280 x 720 (sekcja 2.14). W większej rozdzielczości sprawdzono tylko wycinek obrazu.
- **Nie sprawdzone:** macOS i wyświetlacz Retina (nic z tej części nie było tam budowane ani uruchamiane), klikanie kontrolek panelu myszą (zrzuty były robione bez niej), `Reload shaders` przy dziesięciu programach. Lista do odhaczenia jest w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 18.
- **Nie zmierzone:** pamięć karty zajęta przez trzy cele (liczby w sekcji 2.14 są policzone z rozmiaru), koszt dwóch podglądów bloomu, zależność kosztu od liczby iteracji, jasność tekstury kryształu (sekcja 2.15).

### 5.10 `Bloom.hpp` i `Bloom.cpp`: ustawienia i matematyka bez OpenGL

Plik nagłówkowy zaczyna się od zdania, które tłumaczy jego istnienie: "Plain data and math without OpenGL, like the rest of the game_logic library, so tests can use it. The passes themselves are drawn by game::PostProcess." Stałe:

```cpp
constexpr int BLOOM_DOWNSCALE = 2;

constexpr int BLOOM_BLUR_RADIUS = 6;

constexpr int BLOOM_BLUR_WEIGHT_COUNT = BLOOM_BLUR_RADIUS + 1;

constexpr float BLOOM_BLUR_SIGMA = 3.0F;

constexpr int MIN_BLOOM_BLUR_ITERATIONS = 1;
constexpr int MAX_BLOOM_BLUR_ITERATIONS = 10;
```

(Komentarze Doxygen są pominięte, ich treść jest w tabeli i w sekcjach 2.13 i 2.14.)

| Stała | Dlaczego tak |
|---|---|
| `BLOOM_DOWNSCALE = 2` | połowa w każdym kierunku, ćwierć pikseli. Komentarz wymienia trzy skutki: ćwierć kosztu, to samo jądro sięga dwa razy dalej na ekranie, rozmycie ukrywa niższą rozdzielczość |
| `BLOOM_BLUR_RADIUS = 6` | 13 odczytów na przebieg. Komentarz ostrzega, że ta sama liczba stoi w `post/blur.frag` jako `BLUR_RADIUS` |
| `BLOOM_BLUR_WEIGHT_COUNT` | policzone z promienia, żeby rozmiar tablicy wag nie był trzecim miejscem z tą samą liczbą |
| `BLOOM_BLUR_SIGMA = 3.0F` | połowa promienia: na ostatnim czytanym pikselu dzwon ma 14 procent wysokości, więc niewiele jest ucinane |
| `MIN_`, `MAX_BLOOM_BLUR_ITERATIONS` | zakres suwaka i zakres, do którego `drawBloom` przycina liczbę przed pętlą. Są w nagłówku, a nie w panelu, bo korzystają z nich trzy miejsca: panel, `drawBloom` i test |

Ustawienia:

```cpp
struct BloomSettings {
    bool enabled = true;
    float threshold = 0.8F;
    float intensity = 1.0F;
    int blurIterations = 6;
};
```

| Pole | Wartość startowa | Znaczenie |
|---|---|---|
| `enabled` | `true` | czy przebiegi bloomu są rysowane i czy wynik jest dodawany. Wyłączony daje dokładnie klatkę bez bloomu |
| `threshold` | 0,8 | próg jasności w liniowych wartościach bufora sceny (sekcja 2.12) |
| `intensity` | 1,0 | mnożnik poświaty przed dodaniem do sceny. 0 nie dodaje nic |
| `blurIterations` | 6 | ile razy rozmycie jest powtarzane, każde powtórzenie to przebieg poziomy i pionowy (sekcja 2.13) |

Komentarz nad strukturą mówi, że wartości startowe są częścią wyglądu nocy i zostały dobrane razem ze świeceniem kryształów (`CRYSTAL_GLOW_STRENGTH`) i jasnością nieba. Zmiana jednej z tych trzech rzeczy wymaga obejrzenia pozostałych.

Dwie funkcje:

```cpp
int bloomTargetExtent(int sceneExtent) {
    // Integer division drops the rest: 1281 / 2 is 640. For a scene of one pixel it
    // gives 0, which std::max lifts back to 1.
    return std::max(MIN_TARGET_EXTENT, sceneExtent / BLOOM_DOWNSCALE);
}

std::array<float, BLOOM_BLUR_WEIGHT_COUNT> bloomBlurWeights() {
    std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights{};

    // The height of the bell at every distance, and the sum over the whole kernel. The
    // centre is read once, every other distance twice (left and right, or up and down).
    float sum = 0.0F;
    for (std::size_t distance = 0; distance < weights.size(); ++distance) {
        const auto d = static_cast<float>(distance);
        weights[distance] = std::exp(-(d * d) / (2.0F * BLOOM_BLUR_SIGMA * BLOOM_BLUR_SIGMA));
        sum += distance == 0 ? weights[distance] : 2.0F * weights[distance];
    }

    // Divided by the sum, the kernel adds up to 1.
    for (float& weight : weights) {
        weight /= sum;
    }
    return weights;
}
```

| Linia | Znaczenie |
|---|---|
| `sceneExtent / BLOOM_DOWNSCALE` | dzielenie liczb całkowitych: `1281 / 2 = 640`, reszta przepada. "Extent" to rozmiar w jednym kierunku: funkcja jest wołana osobno dla szerokości i dla wysokości |
| `std::max(MIN_TARGET_EXTENT, ...)` | `MIN_TARGET_EXTENT` to stała 1 z anonimowej przestrzeni nazw. Bez tego scena o szerokości 1 dałaby cel o szerokości 0, którego nie da się utworzyć |
| `std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights{};` | tablica o stałym rozmiarze, zwracana przez wartość. Klamry zerują elementy. `std::array`, a nie `std::vector`: siedem liczb nie potrzebuje pamięci ze sterty |
| `const auto d = static_cast<float>(distance);` | indeks pętli jest liczbą całkowitą bez znaku, wzór liczy na `float` |
| `std::exp(-(d * d) / (2.0F * BLOOM_BLUR_SIGMA * BLOOM_BLUR_SIGMA))` | funkcja Gaussa z sekcji 2.13, jeszcze bez normalizacji |
| `sum += distance == 0 ? weights[distance] : 2.0F * weights[distance];` | suma **całego** jądra: środek raz, każda inna odległość dwa razy, bo shader czyta ją po obu stronach |
| `weight /= sum;` | normalizacja. Pętla po referencji (`float&`), więc dzieli elementy tablicy, a nie ich kopie |

Funkcja jest wołana w każdej klatce, w której rysowany jest bloom: siedem wywołań `std::exp` na klatkę. Wynik zawsze ten sam, bo zależy tylko od stałych. Kod wybiera prostotę (żadnego stanu do zapamiętania), a koszt jest niemierzalny wobec trzynastu przebiegów.

### 5.11 `drawBloom`

```cpp
void PostProcess::drawBloom(const gfx::Shader& brightShader, const gfx::Shader& blurShader,
                            const gfx::Shader& previewShader, const PostProcessSettings& settings) {
    // Until the passes below have run, this frame has no bloom.
    m_bloomDrawn = false;
    if (!settings.bloom.enabled || !m_scene.isValid() || !brightShader.isValid() ||
        !blurShader.isValid()) {
        return;
    }

    // The three targets follow the size of the scene framebuffer, so a resized window
    // resizes them in the same frame. GL_RGBA16F like the scene: the glow of a light
    // far brighter than white must stay brighter than the glow of a white wall.
    const int width = bloomTargetExtent(m_scene.width());
    const int height = bloomTargetExtent(m_scene.height());
    fitTarget(m_brightPass, width, height, gfx::ColorFormat::Rgba16F);
    fitTarget(m_blurHorizontal, width, height, gfx::ColorFormat::Rgba16F);
    fitTarget(m_bloom, width, height, gfx::ColorFormat::Rgba16F);
    if (!m_brightPass.isValid() || !m_blurHorizontal.isValid() || !m_bloom.isValid()) {
        return;
    }

    // One flat triangle per pass: nothing to test the depth against.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    // Step 1, the bright pass: scene colour in, the light above the threshold out.
    // bind() sets the viewport to the smaller size of the target. The triangle still
    // covers it, so the picture of the scene is shrunk to it.
    brightShader.use();
    brightShader.setInt(BRIGHT_SCENE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    brightShader.setFloat(BRIGHT_THRESHOLD_UNIFORM, settings.bloom.threshold);
    m_brightPass.bind();
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();
```

| Linia | Znaczenie |
|---|---|
| `m_bloomDrawn = false;` jako **pierwsza** linia | każda droga wyjścia przed końcem pętli zostawia fałsz. Prawda jest ustawiana w jednym miejscu, po rozmyciu |
| pierwszy `return` | cztery powody, żeby nie rysować: bloom wyłączony (z panelu albo przez widok diagnostyczny), brak bufora sceny, zepsuty program przebiegu jasności, zepsuty program rozmycia. Program podglądu **nie** jest tu sprawdzany: bez niego bloom nadal działa, tylko panel nie dostanie obrazów |
| `bloomTargetExtent(m_scene.width())` | rozmiar celów z rozmiaru **bufora sceny**, nie okna. Bufor sceny idzie za oknem, więc cele też, w tej samej klatce |
| trzy `fitTarget(..., Rgba16F)` | trzy cele HDR. Komentarz podaje powód formatu: poświata światła dużo jaśniejszego od bieli ma zostać jaśniejsza niż poświata białej ściany. W `GL_RGBA8` wynik przebiegu jasności zostałby obcięty do 1 i wracamy do problemu z sekcji 2.11 |
| drugi `return` | sterownik odmówił któregoś celu. `m_bloomDrawn` zostaje fałszem i `composite` rysuje klatkę bez bloomu |
| `glDisable(GL_DEPTH_TEST)` | scena zostawia test włączony, a cele bloomu nie mają głębi |
| `brightShader.use()` przed `setInt` i `setFloat` | uniform jest ustawiany w programie, który jest w użyciu |
| `m_brightPass.bind();` **przed** `m_scene.bindColorTexture(...)` | najpierw cel, potem źródło. `bind()` ustawia też viewport na 640 x 360 (dla okna 1280 x 720), więc trójkąt "pełnoekranowy" pokrywa mniejszy obszar i obraz sceny jest do niego zmniejszany |

```cpp
    // Step 2, the blur. The weights are computed on the CPU and are the same for every
    // pass, so they are set once.
    blurShader.use();
    blurShader.setInt(BLUR_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    const std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights = bloomBlurWeights();
    blurShader.setFloatArray(BLUR_WEIGHTS_UNIFORM, weights);

    // The number comes from a slider, where anything can be typed.
    const int iterations = std::clamp(settings.bloom.blurIterations, MIN_BLOOM_BLUR_ITERATIONS,
                                      MAX_BLOOM_BLUR_ITERATIONS);
    // What the next horizontal pass reads: the bright pass first, later the result of
    // the iteration before. The bright pass itself is never drawn over, so its preview
    // shows it as it was.
    const gfx::Framebuffer* source = &m_brightPass;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        // Horizontal: every pixel becomes the weighted sum of its row neighbours.
        m_blurHorizontal.bind();
        source->bindColorTexture(SOURCE_TEXTURE_UNIT);
        blurShader.setInt(BLUR_HORIZONTAL_UNIFORM, BLUR_HORIZONTAL);
        drawFullscreenTriangle();

        // Vertical, on the result of the horizontal pass: together a round blur.
        m_bloom.bind();
        m_blurHorizontal.bindColorTexture(SOURCE_TEXTURE_UNIT);
        blurShader.setInt(BLUR_HORIZONTAL_UNIFORM, BLUR_VERTICAL);
        drawFullscreenTriangle();

        // The next iteration blurs the bloom again. It reads m_bloom while it draws
        // into m_blurHorizontal, and then the other way round: never both at once.
        source = &m_bloom;
    }
    m_bloomDrawn = true;
```

| Linia | Znaczenie |
|---|---|
| `blurShader.setFloatArray(BLUR_WEIGHTS_UNIFORM, weights);` | siedem wag jednym wywołaniem `glUniform1fv`. `std::array` zamienia się na `std::span<const float>` sama. Raz na klatkę, przed pętlą: wagi są te same dla wszystkich przebiegów |
| `std::clamp(settings.bloom.blurIterations, MIN_..., MAX_...)` | druga linia obrony. Suwak w panelu ma `AlwaysClamp`, ale funkcja nie polega na panelu: liczba 1000 wpisana gdzie indziej zamroziłaby grę, a 0 dałoby pętlę bez obrotów i `m_bloom` z obrazem sprzed kilku klatek |
| `const gfx::Framebuffer* source = &m_brightPass;` | wskaźnik, a nie referencja, bo w pętli zmienia się to, **na co** wskazuje. Referencji nie da się przestawić na inny obiekt. `const` dotyczy framebuffera (tylko odczyt), nie samego wskaźnika |
| `m_blurHorizontal.bind();` potem `source->bindColorTexture(...)` | przebieg poziomy: cel to `m_blurHorizontal`, źródło to `m_brightPass` (pierwszy obrót) albo `m_bloom` (następne) |
| `setInt(BLUR_HORIZONTAL_UNIFORM, BLUR_HORIZONTAL)` | kierunek ustawiany przed każdym przebiegiem, bo ten sam program rysuje oba |
| `m_bloom.bind();` potem `m_blurHorizontal.bindColorTexture(...)` | przebieg pionowy: cel to `m_bloom`, źródło to wynik poziomego |
| `source = &m_bloom;` | od drugiej iteracji rozmywany jest wynik poprzedniej. `m_brightPass` nie jest już czytany i nigdy nie był celem |
| `m_bloomDrawn = true;` | jedyne miejsce, w którym pole staje się prawdą: po ostatnim przebiegu pionowym, kiedy `m_bloom` na pewno zawiera poświatę tej klatki |

Sprawdzenie reguły "nie czytaj tego, do czego rysujesz" dla wszystkich przebiegów pętli: poziomy czyta `m_brightPass` albo `m_bloom` i pisze do `m_blurHorizontal`, pionowy czyta `m_blurHorizontal` i pisze do `m_bloom`. W żadnym źródło i cel nie są tym samym obiektem.

```cpp
    // Step 3, the pictures for the debug UI: the two HDR targets, encoded like the
    // colour attachment of the scene (mode Color of post/preview.frag).
    if (!settings.previews || !previewShader.isValid()) {
        return;
    }
    const int previewWidth = previewWidthFor(m_scene);
    fitTarget(m_brightPassPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    fitTarget(m_bloomPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    if (!m_brightPassPreview.isValid() || !m_bloomPreview.isValid()) {
        return;
    }

    previewShader.use();
    previewShader.setInt(PREVIEW_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    previewShader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Color));

    m_brightPassPreview.bind();
    m_brightPass.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();

    m_bloomPreview.bind();
    m_bloom.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();
}
```

| Linia | Znaczenie |
|---|---|
| `if (!settings.previews \|\| !previewShader.isValid()) return;` | podglądy tylko przy otwartym panelu. `m_bloomDrawn` jest już prawdą, więc ten `return` nie wyłącza bloomu, tylko obrazy dla panelu |
| `previewWidthFor(m_scene)` | podglądy bloomu mają ten sam rozmiar co podglądy sceny (320 x 180 przy 16:9), żeby cztery obrazy w panelu stały równo |
| `AttachmentPreview::Color` | tryb koloru `preview.frag`: samo `linearToSrgb`, które przy okazji przycina do 1. Oba cele bloomu są liniowe i HDR, tak jak kolor sceny, więc pasuje ten sam tryb |
| brak `uNear`, `uFar`, `uDepthRange` | tryb koloru ich nie czyta. Gdy panel jest otwarty, `drawPreviews` ustawiło je wcześniej w tej samej klatce |
| dwa razy `bind`, `bindColorTexture`, `drawFullscreenTriangle` | najpierw wynik przebiegu jasności, potem gotowa poświata |

Podglądy pokazują **zawartość celów**, czyli poświatę przed pomnożeniem przez `Intensity`: suwak `Intensity` nie zmienia obrazu `Bloom` w panelu, tak jak suwak `Exposure` nie zmienia obrazu `HDR colour`. Funkcja zostawia związany jeden z własnych framebufferów (`m_bloom` albo `m_bloomPreview`), więc po niej musi nastąpić `composite`. Nagłówek mówi to wprost.


## 6. Panel ImGui

Panel **Framebuffers** rysuje funkcja `debug::drawFramebuffersPanel` z [`src/debug/panels/FramebuffersPanel.cpp`](../../../src/debug/panels/FramebuffersPanel.cpp). To jedenasty panel interfejsu debugowania. Druga część M7 nie dodała panelu: kontrolki i obrazy bloomu doszły do tego samego. Jego miejsce wśród pozostałych i mechanikę paneli opisuje [`../debug-ui.md`](../debug-ui.md).

### 6.1 Kod panelu

Stałe:

```cpp
constexpr const char* TONE_MAPPING_ITEMS = "None (clamp)\0Reinhard\0ACES (fitted)\0";

constexpr float MIN_EXPOSURE = 0.1F;
constexpr float MAX_EXPOSURE = 8.0F;

constexpr float MIN_BLOOM_THRESHOLD = 0.0F;
constexpr float MAX_BLOOM_THRESHOLD = 4.0F;

constexpr float MIN_BLOOM_INTENSITY = 0.0F;
constexpr float MAX_BLOOM_INTENSITY = 2.0F;

constexpr float MIN_DEPTH_RANGE = 2.0F;
constexpr float MAX_DEPTH_RANGE = 100.0F;

constexpr int SETTING_COLUMNS = 2;

constexpr float PREVIEW_COLUMNS = 4.0F;
```

Wpisy listy stoją w jednym napisie, każdy zakończony znakiem zera: tak chce `ImGui::Combo`. Ich kolejność jest kolejnością wyliczenia `game::ToneMapping`, więc numer wybranego wpisu **jest** wartością wyliczenia.

Zakresy suwaków bloomu należą do panelu, nie do `Bloom.hpp`: to granice wygody, a nie granice poprawności. Próg 0 znaczy, że w bloomie bierze udział cały obraz. Komentarz przy górnej granicy 4 mówi, że leży ona powyżej wszystkiego, co scena rysuje przy domyślnych światłach, więc tam bloom nic nie znajduje. Tego zdania nie sprawdzałem: z rachunku w sekcji 2.15 składnik emisyjny kryształu ma jasność do 2,455, ale to liczba sprzed mnożenia przez teksturę i bez światła latarki. Zakres liczby iteracji jest inaczej: to stałe `game::MIN_BLOOM_BLUR_ITERATIONS` i `MAX_BLOOM_BLUR_ITERATIONS` z `Bloom.hpp`, bo tych samych liczb używa `drawBloom`.

Kontrolki stoją od drugiej części M7 w osobnej funkcji, w tabeli o dwóch kolumnach:

```cpp
void drawSettings(game::PostProcessSettings& settings) {
    // BeginTable returns false when no part of the table can be seen (it is scrolled out
    // of the panel). Nothing is drawn then, and EndTable must not be called.
    if (!ImGui::BeginTable("settings", SETTING_COLUMNS)) {
        return;
    }

    // TableNextColumn moves on to the next cell, and from the last cell of a row to the
    // first cell of a new row. So the widgets fill the table row by row.
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Exposure", &settings.exposure, MIN_EXPOSURE, MAX_EXPOSURE, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
    ImGui::TableNextColumn();
    int toneMappingIndex = static_cast<int>(settings.toneMapping);
    if (ImGui::Combo("Tone mapping", &toneMappingIndex, TONE_MAPPING_ITEMS)) {
        settings.toneMapping = static_cast<game::ToneMapping>(toneMappingIndex);
    }

    game::BloomSettings& bloom = settings.bloom;
    ImGui::TableNextColumn();
    ImGui::Checkbox("Bloom", &bloom.enabled);
    ImGui::TableNextColumn();
    ImGui::SliderInt("Blur iterations", &bloom.blurIterations, game::MIN_BLOOM_BLUR_ITERATIONS,
                     game::MAX_BLOOM_BLUR_ITERATIONS, "%d", ImGuiSliderFlags_AlwaysClamp);

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Threshold", &bloom.threshold, MIN_BLOOM_THRESHOLD, MAX_BLOOM_THRESHOLD,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Intensity", &bloom.intensity, MIN_BLOOM_INTENSITY, MAX_BLOOM_INTENSITY,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Depth range", &settings.depthPreviewRange, MIN_DEPTH_RANGE, MAX_DEPTH_RANGE,
                       "%.0f m", ImGuiSliderFlags_AlwaysClamp);

    ImGui::EndTable();
}
```

(Siedem wywołań `ImGui::SetItemTooltip`, po jednym za każdą kontrolką, jest tu pominiętych. Ich teksty są w tabeli sekcji 6.3.)

| Linia | Znaczenie |
|---|---|
| `ImGui::BeginTable("settings", SETTING_COLUMNS)` | tabela ImGui o dwóch kolumnach. Zwraca fałsz, gdy żadna jej część nie jest widoczna, i wtedy **nie wolno** wołać `EndTable`: stąd wczesny `return`. To inna umowa niż przy `Begin` i `End` okna, gdzie `End` woła się zawsze |
| `ImGui::TableNextColumn()` | przejście do następnej komórki, a z ostatniej komórki wiersza do pierwszej komórki nowego wiersza. Siedem kontrolek wypełnia więc tabelę wierszami: `Exposure` i `Tone mapping`, `Bloom` i `Blur iterations`, `Threshold` i `Intensity`, `Depth range` i pusta komórka |
| po co tabela | komentarz przy `SETTING_COLUMNS`: żeby panel był na tyle krótki, że cztery obrazy mieszczą się pod kontrolkami bez przewijania. Wysokość panelu (`FRAMEBUFFERS_HEIGHT = 344` w `PanelLayout.hpp`) nie zmieniła się w tej części |
| `ImGuiSliderFlags_Logarithmic` | suwak ekspozycji w skali logarytmicznej (sekcja 2.5). `AlwaysClamp` trzyma w zakresie także wartość wpisaną z klawiatury |
| `int toneMappingIndex = static_cast<int>(...)` i rzutowanie z powrotem | `Combo` pracuje na `int`, a pole jest `enum class`. `Combo` zwraca prawdę tylko w klatce, w której wybór się zmienił |
| `game::BloomSettings& bloom = settings.bloom;` | krótsza nazwa dla czterech następnych kontrolek. Referencja, więc kontrolki piszą do prawdziwych ustawień |
| `ImGui::Checkbox("Bloom", &bloom.enabled)` | pole wyboru pisze przez wskaźnik do `bool` |
| `ImGui::SliderInt("Blur iterations", ...)` | suwak liczb całkowitych, format `"%d"`. Granice z `Bloom.hpp` |
| `SliderFloat("Threshold", ...)`, `SliderFloat("Intensity", ...)` | zwykłe suwaki liniowe, dwa miejsca po przecinku |
| `SliderFloat("Depth range", ...)` | ten sam suwak co w pierwszej części, przeniesiony z miejsca pod linią informacyjną do tabeli |

Funkcja panelu:

```cpp
void drawFramebuffersPanel(game::PostProcessSettings& settings,
                           const game::PostProcess& postProcess) {
    placePanelOnFirstUse(FRAMEBUFFERS_PLACEMENT);
    // Begin returns false when the panel is folded. The previews are asked for only
    // while it is open: the game reads the flag in its next frame.
    const bool open = ImGui::Begin("Framebuffers");
    settings.previews = open;
    if (open) {
        drawSettings(settings);

        // The framebuffer the scene is drawn into, and the smaller targets of the bloom.
        const gfx::Framebuffer& scene = postProcess.sceneTarget();
        const gfx::Framebuffer& bloom = postProcess.bloomTarget();
        const bool bloomDrawn = postProcess.bloomDrawn();
        ImGui::Separator();
        ImGui::Text("Scene framebuffer: %d x %d px, %s + %s", scene.width(), scene.height(),
                    gfx::colorFormatName(scene.colorFormat()),
                    gfx::depthFormatName(scene.depthFormat()));
        if (bloomDrawn) {
            ImGui::Text("Bloom targets (3): %d x %d px, %s", bloom.width(), bloom.height(),
                        gfx::colorFormatName(bloom.colorFormat()));
        } else {
            ImGui::TextUnformatted("Bloom targets: not drawn (bloom off or a debug view)");
        }

        // The four pictures, side by side, sharing the width of the panel: the two
        // attachments of the scene framebuffer, then the two steps of the bloom.
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float previewWidth =
            (ImGui::GetContentRegionAvail().x - (PREVIEW_COLUMNS - 1.0F) * spacing) /
            PREVIEW_COLUMNS;
        drawPreview("HDR colour", "The colour attachment of the scene, cut off at 1.",
                    postProcess.preview(game::AttachmentPreview::Color), previewWidth, true);
        ImGui::SameLine();
        drawPreview("Depth", "The depth attachment of the scene, as a distance.",
                    postProcess.preview(game::AttachmentPreview::Depth), previewWidth, true);
        ImGui::SameLine();
        drawPreview("Bright pass", "What the scene has above the bloom threshold.",
                    postProcess.brightPassPreview(), previewWidth, bloomDrawn);
        ImGui::SameLine();
        drawPreview("Bloom", "The bright pass after the blur, before the intensity.",
                    postProcess.bloomPreview(), previewWidth, bloomDrawn);
    }
    ImGui::End();
}
```

| Linia | Znaczenie |
|---|---|
| `placePanelOnFirstUse(FRAMEBUFFERS_PLACEMENT)` | tylko przy pierwszym uruchomieniu (bez `imgui.ini`): panel startuje **zwinięty**, w trzecim rzędzie belek tytułowych u góry okna |
| `const bool open = ImGui::Begin("Framebuffers");` | `Begin` zwraca fałsz dla zwiniętego panelu |
| `settings.previews = open;` | protokół podglądów, sekcja 6.2 |
| `drawSettings(settings);` | wszystkie kontrolki, w tabeli |
| `ImGui::Text("Scene framebuffer: ...")` | linia informacyjna: rozmiar i formaty bufora sceny, na przykład `1280 x 720 px, GL_RGBA16F + GL_DEPTH_COMPONENT24`. Przed pierwszą udaną klatką pokazałaby `0 x 0 px, none + none` |
| `ImGui::Text("Bloom targets (3): ...")` | druga linia informacyjna, na przykład `Bloom targets (3): 640 x 360 px, GL_RGBA16F`. Rozmiar i format czytane z `m_bloom`, a trzy cele są takie same. Liczba 3 jest wpisana w napis |
| gałąź `else` | `Bloom targets: not drawn (bloom off or a debug view)`: gdy `bloomDrawn()` jest fałszem. Bez tego panel pokazywałby rozmiar celów, które w tej klatce nie były odświeżane |
| `(GetContentRegionAvail().x - (PREVIEW_COLUMNS - 1.0F) * spacing) / PREVIEW_COLUMNS` | cztery obrazy dzielą szerokość panelu po równo, z trzema odstępami między nimi. W pierwszej części były dwa obrazy i jeden odstęp |
| `ImGui::SameLine()` trzy razy | każdy następny obraz obok poprzedniego, nie pod nim |
| ostatni argument `true` albo `bloomDrawn` | podglądy sceny są zawsze aktualne, gdy panel jest otwarty. Podglądy bloomu tylko wtedy, gdy bloom był rysowany |

Jeden podgląd:

```cpp
void drawPreview(const char* caption, const char* tooltip, const gfx::Framebuffer& preview,
                 float width, bool drawn) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(caption);
    if (drawn && preview.isValid()) {
        // The picture keeps the shape of the framebuffer it shows.
        const float height =
            width * static_cast<float>(preview.height()) / static_cast<float>(preview.width());
        const auto textureId = static_cast<ImTextureID>(preview.colorTextureId());
        ImGui::Image(textureId, {width, height}, {0.0F, 1.0F}, {1.0F, 0.0F});
    } else {
        // The first frame after the panel was opened (the pictures are drawn by the
        // game in its next frame), or a pass that is switched off.
        ImGui::TextUnformatted(drawn ? "(no picture yet)" : "(not drawn)");
    }
    ImGui::EndGroup();
    ImGui::SetItemTooltip("%s", tooltip);
}
```

(Dłuższy komentarz nad `ImGui::Image` jest tu pominięty, jego treść jest niżej.)

| Linia | Znaczenie |
|---|---|
| `BeginGroup` / `EndGroup` | podpis i obraz tworzą jeden element, żeby `SameLine` postawiło obok siebie całe kolumny |
| `if (drawn && preview.isValid())` | dwa powody, żeby nie pokazać obrazu: przebieg nie biegł w tej klatce (`drawn` fałsz) albo framebuffer podglądu jeszcze nie istnieje |
| `height = width * h / w` | obraz zachowuje proporcje framebuffera, który pokazuje |
| `static_cast<ImTextureID>(preview.colorTextureId())` | ImGui rozpoznaje teksturę po identyfikatorze obiektu OpenGL |
| `{0.0F, 1.0F}, {1.0F, 0.0F}` | współrzędne tekstury lewego górnego i prawego dolnego rogu obrazu. Tekstura framebuffera ma wiersz `v = 0` na **dole**, jak wszystko, co rysuje OpenGL, a ImGui liczy od góry. Z wartościami domyślnymi `(0, 0)` i `(1, 1)` obraz byłby do góry nogami |
| `drawn ? "(no picture yet)" : "(not drawn)"` | dwa różne napisy na dwa różne powody. `(no picture yet)`: pierwsza klatka po otwarciu panelu. `(not drawn)`: przebieg jest wyłączony, obrazu nie będzie, dopóki ktoś go nie włączy. Stary obraz nie jest pokazywany, chociaż framebuffer podglądu nadal go trzyma: wprowadzałby w błąd |
| `ImGui::SetItemTooltip("%s", tooltip);` po `EndGroup` | podpowiedź dla całej grupy, czyli dla podpisu i obrazu razem. `"%s"` zamiast samego `tooltip`: funkcja przyjmuje napis formatujący, a tekst podany wprost jako format byłby błędem, gdyby zawierał znak procentu |

ImGui czyta teksturę podglądu własnym samplerem (filtr liniowy, przycinanie do krawędzi), który jego backend OpenGL wiąże na jednostce 0. Podgląd jest teksturą `GL_RGBA8`, nie sRGB, więc nie potrzebuje obejścia, którego wymagają podglądy tekstur w panelu Assets (`debug::RawTextureSampler`, opisane w [`../debug-ui.md`](../debug-ui.md)).

### 6.2 Protokół flagi `previews`

Podglądy rysuje gra, a pokazuje panel. Łączy je jedno pole, `PostProcessSettings::previews`, i kolejność wywołań w klatce:

| Kiedy | Kto | Co robi |
|---|---|---|
| klatka N, `onRender` | gra | czyta `previews` (ustawione w klatce N-1). Jeśli prawda, `drawPreviews` rysuje dwa podglądy sceny, a `drawBloom` (gdy bloom jest rysowany) dwa podglądy bloomu |
| klatka N, początek `DebugUI::draw` | interfejs | zeruje `previews` |
| klatka N, `drawFramebuffersPanel` | panel | ustawia `previews` na to, co zwróciło `Begin` |

Zerowanie na początku `DebugUI::draw` obsługuje przypadek, w którym panele są w ogóle ukryte (klawisz akcentu): wtedy `drawFramebuffersPanel` nie jest wołane, nikt flagi nie ustawia i gra przestaje rysować podglądy. Skutek uboczny protokołu to **jedna klatka opóźnienia**: w klatce, w której panel został otwarty, podglądów jeszcze nie ma i panel pokazuje `(no picture yet)`. Po zamknięciu panelu framebuffery podglądu zostają w pamięci z ostatnim obrazem, tylko nikt ich nie odświeża.

Podglądy bloomu mają drugi warunek, niezależny od flagi: `bloomDrawn()`. Panel pyta o niego w tej samej klatce, w której gra go ustawiła (`onRender` biegnie przed `DebugUI::draw`), więc panel zawsze opisuje to, co gra narysowała w tej klatce. Sama zmiana pola `Bloom` działa jedną klatkę później: po odznaczeniu napis `(not drawn)` pojawia się w następnej klatce, bo dopiero wtedy `drawBloom` zobaczy nowe ustawienie.

### 6.3 Kontrolki i czego uczą

| Panel | Kontrolka | Co zmienia | Czego uczy obserwacja |
|---|---|---|---|
| Framebuffers | suwak `Exposure`, 0,1 do 8, startowo 1,00 | `uExposure` | scena ma jedną ilość światła, a ekspozycja wybiera, który jej wycinek widać. Przy 0,25 kryształ odzyskuje barwę, a korytarz znika w czerni. Przy 4 korytarz jest czytelny, a plama latarki jest biała. Poświata jaśnieje i ciemnieje razem ze sceną, ale to, **co** świeci, się nie zmienia: próg działa przed ekspozycją |
| Framebuffers | lista `Tone mapping`: `None (clamp)`, `Reinhard`, `ACES (fitted)`, startowo ACES | `uToneMapping` | `None`: plama latarki i kryształy są płaskimi plamami. `Reinhard`: nic nie jest wypalone, obraz jest szarawy. `ACES`: głębsze cienie, mocniejszy środek |
| Framebuffers | linia `Scene framebuffer: ...` | nic, tylko odczyt | rozmiar bufora zmienia się razem z oknem. Format `GL_RGBA16F` to dowód, że scena jest w HDR |
| Framebuffers | pole `Bloom`, startowo zaznaczone | `BloomSettings::enabled`, przez nie `uBloomEnabled` | najprostsze "przed i po": odznaczone daje dokładnie klatkę bez bloomu. Kryształy i księżyc tracą poświatę, reszta obrazu się nie zmienia. Linia `Bloom targets` i dwa obrazy przechodzą na `not drawn` |
| Framebuffers | suwak `Blur iterations`, 1 do 10, startowo 6 | `BloomSettings::blurIterations`, liczba obrotów pętli w `drawBloom` | przy 1 poświata jest wąską obwódką, przy 10 szeroką mgiełką. Szerokość rośnie jak pierwiastek z liczby iteracji, a koszt liniowo (sekcja 2.13) |
| Framebuffers | suwak `Threshold`, 0 do 4, startowo 0,80 | `uThreshold` | przy 0 świeci cały obraz i scena robi się mleczna. Przy 2 zostają tylko najjaśniejsze miejsca. Obraz `Bright pass` pokazuje na żywo, co przechodzi przez próg |
| Framebuffers | suwak `Intensity`, 0 do 2, startowo 1,00 | `uBloomIntensity` | zmienia obraz w oknie, ale **nie** obraz `Bloom` w panelu: podgląd pokazuje cel przed mnożeniem. Przy 0 obraz w oknie jest taki sam jak z odznaczonym polem `Bloom`, tylko przebiegi nadal są rysowane |
| Framebuffers | suwak `Depth range`, 2 do 100 m, startowo 15 m | `uDepthRange` | przy 100 m prawie cały labirynt jest czarny: tak mała część zakresu kamery jest naprawdę używana |
| Framebuffers | linia `Bloom targets (3): ...` | nic, tylko odczyt | połowa rozmiaru sceny w każdym kierunku, ten sam format `GL_RGBA16F`. Zmienia się razem z oknem |
| Framebuffers | obraz `HDR colour` (podpowiedź: `The colour attachment of the scene, cut off at 1.`) | nic | zawartość bufora przed bloomem, ekspozycją i krzywą. Zmiana suwaka `Exposure` **nie** zmienia tego obrazu. Poświaty na nim nie ma: bloom nie jest zapisywany do bufora sceny |
| Framebuffers | obraz `Depth` (podpowiedź: `The depth attachment of the scene, as a distance.`) | nic | głębia jako odległość: bliskie ściany ciemne, dalekie jasne, niebo białe |
| Framebuffers | obraz `Bright pass` (podpowiedź: `What the scene has above the bloom threshold.`) | nic | czarny obraz z kilkoma jasnymi plamami: kryształy, tarcza księżyca. Ostre krawędzie, bo to stan **przed** rozmyciem. Dowód, że trzeci cel nie jest zamazywany |
| Framebuffers | obraz `Bloom` (podpowiedź: `The bright pass after the blur, before the intensity.`) | nic | te same plamy rozlane w miękkie koła. To jest dokładnie to, co przebieg składający dodaje do sceny (razy `Intensity`) |
| Assets | lista `View` | widok diagnostyczny | po przełączeniu na normalne albo UV suwak `Exposure`, lista `Tone mapping` i kontrolki bloomu przestają wpływać na obraz, a obrazy bloomu pokazują `(not drawn)` (sekcja 2.9) |
| Shaders | `Reload shaders` | przeładowanie dziesięciu programów | cztery ostatnie linie listy to programy `composite`, `preview`, `bright` i `blur`, wszystkie z plikiem `composite.vert` |

### 6.4 Scenariusz pokazu na obronie

1. **Dowód, że scena nie idzie prosto do okna.** Otwieram panel `Framebuffers` (trzeci rząd belek u góry). Pokazuję linię `Scene framebuffer: 1280 x 720 px, GL_RGBA16F + GL_DEPTH_COMPONENT24` i dwa pierwsze obrazy: to są załączniki bufora, do którego rysowana jest scena.
2. **Zmiana rozmiaru.** Przeciągam krawędź okna: liczby w obu liniach zmieniają się razem z oknem (cele bloomu mają zawsze połowę), obrazy zachowują proporcje.
3. **HDR.** Staję przed kryształem. `Tone mapping: None`: kryształ jest płaską jasną plamą. Zmniejszam `Exposure` do 0,25: ścianki kryształu wracają, bo bufor pamiętał wartości powyżej 1. W oknie 8-bitowym nie byłoby z czego ich odzyskać.
4. **Trzy krzywe.** `Exposure` z powrotem na 1. Przełączam `None`, `Reinhard`, `ACES` i mówię, co robi każda (sekcja 2.6).
5. **Podgląd koloru a gotowa klatka.** Ruszam suwakiem `Exposure`: okno się zmienia, obraz `HDR colour` w panelu nie. Podgląd pokazuje bufor, okno pokazuje wynik przebiegu składającego.
6. **Głębia.** Pokazuję obraz `Depth`, przesuwam `Depth range` z 15 na 100 i z powrotem. Tłumaczę, dlaczego surowa głębia byłaby biała (tabela w sekcji 2.10).
7. **Bloom, przed i po.** Staję kilka metrów od kryształu. Odznaczam pole `Bloom`: poświata znika, kryształ zostaje. Zaznaczam z powrotem.
8. **Bloom rozłożony na kroki.** Pokazuję trzeci i czwarty obraz. `Bright pass`: czarno, tylko kryształ i księżyc, ostre. `Bloom`: to samo po rozmyciu. Mówię, że okno to scena plus ten czwarty obraz.
9. **Próg.** Przesuwam `Threshold` na 0,3: w obrazie `Bright pass` pojawiają się ściany w świetle latarki i niebo, scena robi się mleczna. Na 2: zostaje sam środek kryształu. Wracam na 0,8 i tłumaczę wzór z udziałem jasności (sekcja 2.12).
10. **Iteracje.** `Blur iterations` z 6 na 1 i na 10. Mówię, ile to przebiegów (2 i 20) i dlaczego szerokość rośnie jak pierwiastek.
11. **Widok danych.** W panelu Assets wybieram widok normalnych. Ruszam `Exposure`: obraz się nie zmienia. Obrazy bloomu pokazują `(not drawn)`. Wracam do `Textured`.
12. **Kod.** Otwieram `composite.frag`: pięć kroków w komentarzu funkcji `main`, linia z `+=` dla bloomu i ostatnia linia z `linearToSrgb`. Potem `bright.frag` (jedna linia wzoru) i `blur.frag` (pętla z trzynastoma odczytami).

Kroków od 7 do 10 nikt jeszcze nie przeszedł myszą: zrzuty ekranu dla progu 0,3 i 2,0 są zgłoszone, ale były robione bez klikania.

## 7. Pułapki

1. **Czarny ekran po dodaniu framebuffera.** Najczęstsze przyczyny: przebieg składający nie został wywołany (scena jest w teksturze, której nikt nie przeniósł do okna), tekstura sceny czytana cudzym samplerem z mipmapami (niekompletna, czyli czerń), test głębi włączony podczas trójkąta pełnoekranowego. Kod ma zabezpieczenie na każdą: kolejność w `onRender`, `glBindSampler(unit, 0)` w `bindColorTexture`, `glDisable(GL_DEPTH_TEST)` w `composite`.
2. **Czytanie tekstury, do której się rysuje.** Pętla zwrotna, wynik niezdefiniowany. W `drawPreviews` i w `drawBloom` cel jest zmieniany **przed** związaniem tekstury źródłowej. Każdy przebieg, który czyta scenę, ma własny framebuffer. Wersja tej pułapki dla rozmycia to punkt 18.
3. **Podwójne kodowanie.** `linearToSrgb` w shaderze i włączone `GL_FRAMEBUFFER_SRGB` na oknie sRGB dają obraz zakodowany dwa razy: wyblakły, bez czerni. Stąd jawne `glDisable` w `composite`.
4. **Kodowanie przed krzywą albo ekspozycja po kodowaniu.** Kolejność jest sztywna: ekspozycja, krzywa, kodowanie. Każda inna daje obraz, który "jakoś wygląda", ale suwaki przestają znaczyć to, co znaczą.
5. **Brak `clamp` we wzorze ACES.** Granica wzoru to 1,033. Bez przycięcia najjaśniejsze piksele wyszłyby poza zakres. W tej grze dalej jest `linearToSrgb`, które też przycina, więc błąd byłby niewidoczny, dopóki ktoś nie dopisze kroku między krzywą a kodowaniem (winieta).
6. **Krzywe na kanał zmieniają barwę.** Bardzo jasny nasycony kolor bieleje (sekcja 2.6). To własność metody.
7. **Test głębi zostaje wyłączony.** Po `composite`, po `drawPreviews` i po `drawBloom` `GL_DEPTH_TEST` jest wyłączony. Kod sceny, który polegałby na tym, że ktoś go wcześniej włączył, rysowałby ściany w kolejności wywołań. `onRender` włącza test w każdej klatce.
8. **Bufor głębi okna nie jest czyszczony.** Nikt już nie woła `glClear` z oknem jako celem. Cokolwiek narysowane do okna z włączonym testem głębi porównywałoby się z przypadkową zawartością.
9. **Okno nie jest czyszczone.** Przebieg składający zakłada, że trójkąt pokrywa wszystko. Gdy program `composite` się nie skompiluje, okno pokazuje to, co w nim zostało (poprzednią klatkę albo śmieci), a nie kolor tła.
10. **Rozmiar okna zamiast rozmiaru framebuffera.** Bufor sceny i viewport muszą być w pikselach (`framebufferSize()`). Na wyświetlaczu Retina rozmiar okna jest o połowę mniejszy i scena zajęłaby ćwierć ekranu. Nie sprawdzone na macOS.
11. **Rozmiar 0 x 0.** Zminimalizowane okno. Framebuffer o rozmiarze 0 nie powstanie, a proporcja 0 / 0 to NaN. `onRender` pomija całą klatkę przed jakimkolwiek wywołaniem OpenGL.
12. **Podgląd do góry nogami.** Tekstura framebuffera ma `v = 0` na dole, ImGui na górze. Współrzędne w `ImGui::Image` są odwrócone w pionie.
13. **Wspólny shader wierzchołków.** `composite.vert` należy do czterech programów. Błąd w tym pliku psuje przy przeładowaniu wszystkie cztery naraz: okno zostaje z ostatnią klatką, a bloom i podglądy znikają.
14. **Liczby wyliczenia a shader.** `ToneMapping` i `AttachmentPreview` są przekazywane jako `int`. Wpis dopisany w środku wyliczenia bez zmiany `if` w shaderze i napisu `TONE_MAPPING_ITEMS` wybierałby po cichu inną krzywą. Żaden test tego nie pilnuje.
15. **Płaszczyzny kamery w podglądzie głębi.** `uNear` i `uFar` muszą być tymi, którymi narysowano scenę. Inne wartości dają złe metry bez żadnego błędu.
16. **Znane ograniczenia.** Bufor sceny ma pełną rozdzielczość okna, w połowie rozdzielczości pracuje tylko bloom. Framebuffer ma jeden załącznik koloru. Ekspozycja jest ręczna. Kolory `Kd` materiałów nie są przeliczane z sRGB (wszystkie modele gry mają `Kd` białe, więc różnicy nie ma). Mgły, winiety, minimapy i cieni nie ma.
17. **Bloom na wartościach LDR.** Jeśli scena jest obcięta do 1, zanim trafi do przebiegu jasności (bufor 8-bitowy, bloom policzony po mapowaniu tonów albo cele bloomu w `GL_RGBA8`), próg nie odróżnia kryształu od białej ściany: obie mają 1 (tabela w sekcji 2.11). Świeci wtedy wszystko, co jasne, tak samo mocno. W tym kodzie pilnują tego trzy rzeczy: scena w `GL_RGBA16F`, cele bloomu w `GL_RGBA16F` i miejsce bloomu przed ekspozycją i krzywą.
18. **Rozmycie, które czyta i pisze tę samą teksturę.** Kusząca "optymalizacja": jeden cel i rozmycie w miejscu. Wynik jest niezdefiniowany: część pikseli byłaby czytana już po zapisie, część przed, zależnie od karty. Stąd dwa cele i ping-pong (sekcja 2.14). Pokrewny błąd: pomylenie kolejności `bind()` i `bindColorTexture()` albo zamiana `source` na zły cel w pętli. `GL_CHECK` tego nie złapie, bo OpenGL nie zgłasza pętli zwrotnej jako błędu.
19. **Próg w złej przestrzeni kolorów.** Wagi Rec. 709 i próg 0,8 mają sens dla wartości **liniowych**. Policzenie jasności z liczb zakodowanych w sRGB daje inną liczbę: liniowe 0,8 to w sRGB 0,91, a szarość zakodowana jako 0,8 ma liniowo tylko 0,60. Próg ustawiony "na oko" na obrazie sRGB przepuszczałby więc dużo więcej. Druga wersja tego błędu: zakodowanie wyniku przebiegu jasności przez `linearToSrgb` "żeby podgląd wyglądał dobrze". Kodowanie należy do `preview.frag` i `composite.frag`, a cele bloomu zostają liniowe.
20. **Szerokość poświaty zależy od rozdzielczości.** Jądro ma promień w pikselach celu, więc na ekranie 1440p poświata jest względnie o połowę cieńsza niż na 720p, a na wyświetlaczu Retina (framebuffer dwa razy większy niż okno) tak samo. Kod tego nie wyrównuje. Otwarte, nie sprawdzone na macOS.
21. **Promień w dwóch miejscach.** `BLOOM_BLUR_RADIUS` w `Bloom.hpp` i `BLUR_RADIUS` w `blur.frag` muszą być równe i żaden test tego nie pilnuje. Większy promień w C++ niż w shaderze: `glUniform1fv` wysyła więcej wag, niż tablica ma elementów, OpenGL nadmiar pomija, shader używa początku jądra, którego suma jest mniejsza od 1, i poświata ciemnieje z każdą iteracją. Większy w shaderze: ostatnie elementy tablicy zostają zerem, obraz jest poprawny, ale shader robi odczyty z wagą 0.
22. **Wagi, które nie sumują się do 1.** Każda zmiana `bloomBlurWeights` musi zachować normalizację. Przy dwunastu przebiegach błąd 10 procent w sumie daje obraz ponad trzy razy za jasny albo prawie cztery razy za ciemny (sekcja 2.13). Tego akurat pilnuje test `the blur kernel adds up to one`.
23. **NaN z dzielenia przez zero.** Bez `max(brightness, MIN_LUMINANCE)` czarny piksel dałby `0 / 0`. Jeden NaN w celu rozmycia zaraża wszystkie piksele w zasięgu jądra, a po sześciu iteracjach kwadrat o boku ponad siedemdziesięciu pikseli.
24. **Liczba iteracji bez ograniczenia.** Koszt rośnie liniowo: dziesięć iteracji to dwadzieścia przebiegów. `drawBloom` przycina liczbę do zakresu z `Bloom.hpp` niezależnie od suwaka.
25. **Stary obraz w `m_bloom`.** Gdy `drawBloom` wraca wcześniej, w celu leży poświata z dawnej klatki. Dlatego `composite` pyta o `m_bloomDrawn`, a nie o `m_bloom.isValid()`, i dlatego `drawBloom` trzeba wołać w każdej klatce: to ono zeruje flagę.
26. **Bloom dodaje światło.** Scena z bloomem jest jaśniejsza niż bez niego, bo nadwyżka ponad próg jest w obrazie dwa razy. Wartości świateł dobrane przy włączonym bloomie wyglądają inaczej po jego wyłączeniu i odwrotnie.

## 8. Ćwiczenia

Ćwiczenia od 1 do 4 i od 10 do 13 są na kartce, pozostałe w działającej grze. Po zmianie pliku shadera na Windowsie: `cmake --build --preset debug --target copy_assets`, potem `Reload shaders`. Po ćwiczeniu wycofaj zmianę w pliku ręcznie.

1. **Reinhard na kartce.** Policz `x / (1 + x)` dla 0,5, 3 i 9. Dla jakiego wejścia wynik to 0,9? (Odpowiedzi: 0,333, 0,75, 0,9. Wejście 9.)
2. **ACES na kartce.** Policz wzór dla x = 0,5. (Odpowiedź: licznik `0,5 * (1,255 + 0,03) = 0,6425`, mianownik `0,5 * (1,215 + 0,59) + 0,14 = 1,0425`, wynik 0,616.)
3. **Głębia na kartce.** Dla `n = 0,1` i `f = 100` policz, jaka wartość trafi do tekstury głębi dla ściany 4 m od płaszczyzny kamery. (Odpowiedź: `ndc = 100,1 / 99,9 - 20 / (99,9 * 4) = 1,0020 - 0,0501 = 0,9520`, `stored = 0,9760`.)
4. **Trójkąt na kartce.** Narysuj kwadrat ekranu i trójkąt (-1, -1), (3, -1), (-1, 3). Jakie `vUv` ma środek ekranu? Jakie `vUv` ma punkt (3, -1) i dlaczego nigdy nie trafia do shadera fragmentów?
5. **Bez kodowania.** W `composite.frag` zamień ostatnią linię na `fragColor = vec4(color, 1.0);`. Jak zmienił się obraz i dlaczego ciemne partie ucierpiały najbardziej? Czy panele ImGui się zmieniły?
6. **Kodowanie przed krzywą.** Przenieś `linearToSrgb` przed `if` z krzywymi (i usuń je z końca). Porównaj `ACES` z wersją poprawną. Co mówi to o kolejności kroków?
7. **Surowa głębia.** W `preview.frag` zamień `metres / uDepthRange` na `texture(uSource, vUv).r`. Co widać w podglądzie? Podejdź nosem do ściany: kiedy obraz zaczyna ciemnieć?
8. **Podgląd z krzywą.** Dodaj do gałęzi koloru w `preview.frag` mnożenie przez stałą 0,25 przed kodowaniem. Co pojawiło się w podglądzie w miejscu kryształów i plamy latarki? Dlaczego to dowód, że bufor przechowuje wartości powyżej 1?
9. **Dwa trójkąty.** Zastanów się (bez pisania), co trzeba by zmienić w `composite.vert` i w `drawFullscreenTriangle`, żeby rysować prostokąt z dwóch trójkątów przez `gl_VertexID`. Ile wierzchołków? Jakie wywołanie rysujące?
10. **Luminancja na kartce.** Policz jasność kolorów (1, 0, 0), (0, 1, 0), (0, 0, 1) i (0,5, 0,5, 0,5). Który z czystych kolorów przekracza próg 0,8 przy wartości kanału 1, a jaką wartość musiałby mieć kanał niebieski, żeby czysty błękit go przekroczył? (Odpowiedzi: 0,2126, 0,7152, 0,0722, 0,5. Żaden. Ponad `0,8 / 0,0722 = 11,1`.)
11. **Przebieg jasności na kartce.** Dla progu 0,8 policz wynik dla piksela (1,0, 1,0, 1,0) i dla piksela (0,4, 1,6, 0,2). (Odpowiedzi: jasność 1, udział 0,2, wynik (0,2, 0,2, 0,2). Jasność `0,0850 + 1,1443 + 0,0144 = 1,2438`, udział `0,4438 / 1,2438 = 0,357`, wynik (0,143, 0,571, 0,071).) Sprawdź, że proporcja zielonego do czerwonego jest w wyniku taka sama jak na wejściu.
12. **Wagi na kartce.** Dla `sigma = 1` i promienia 2 policz trzy wagi. (Odpowiedź: wysokości 1, `exp(-0,5) = 0,6065`, `exp(-2) = 0,1353`, suma `1 + 2 * 0,7418 = 2,4837`, wagi 0,4026, 0,2442, 0,0545.) Sprawdź, że środek plus dwa razy pozostałe daje 1.
13. **Koszt na kartce.** Ile przebiegów rozmycia i ile odczytów tekstury na piksel celu daje `Blur iterations = 3`? Ile odczytów dałoby to samo jądro bez rozdzielenia? (Odpowiedzi: 6 przebiegów, `6 * 13 = 78` odczytów. Bez rozdzielenia `3 * 169 = 507`.)
14. **Twardy próg.** W `bright.frag` zamień linię z `share` na `float share = brightness > uThreshold ? 1.0 : 0.0;`. Stań przed kryształem i patrz na jego brzeg podczas pulsowania, potem rusz powoli kamerą. Co się dzieje z poświatą i z obrazem `Bright pass`?
15. **Próg na kanał.** Zamień ostatnie dwie linie `bright.frag` na `fragColor = vec4(max(color - vec3(uThreshold), 0.0), 1.0);`. Jak zmieniła się barwa poświaty kryształu względem samego kryształu?
16. **Tylko jeden kierunek.** W `drawBloom` nic nie zmieniaj, a w `blur.frag` wpisz na stałe `vec2 texelStep = vec2(texel.x, 0.0);`. Jaki kształt ma teraz poświata i dlaczego jest w poziomie szersza niż przedtem? (Podpowiedź: oba przebiegi każdej iteracji rozmywają teraz w tę samą stronę, a wariancje się dodają, więc szerokość rośnie o pierwiastek z 2.)
17. **Bloom po krzywej.** W `composite.frag` przenieś blok `if (uBloomEnabled == 1)` za blok mapowania tonów (przed `linearToSrgb`). Porównaj środek kryształu i jego otoczenie z wersją poprawną. Rusz suwakiem `Exposure`: co robi teraz poświata?
18. **Wagi bez normalizacji.** W `Bloom.cpp` zakomentuj pętlę, która dzieli przez `sum`, zbuduj i uruchom testy. Które przypadki nie przechodzą? Uruchom grę: co widać i dlaczego obraz zmienia się tak gwałtownie z liczbą iteracji? (Suma nieznormalizowanego jądra to 7,298, więc każdy przebieg mnoży jasność przez tę liczbę.)
19. **Pełna rozdzielczość.** Zmień `BLOOM_DOWNSCALE` na 1 (trzeba przebudować program). Porównaj szerokość poświaty i liczbę klatek z ukrytymi panelami. Ile iteracji byłoby trzeba, żeby poświata miała starą szerokość (cztery razy tyle: szerokość rośnie jak pierwiastek z liczby iteracji), i dlaczego suwak na to nie pozwoli?

## 9. Pytania kontrolne

1. **Co to jest rendering pozaekranowy?**
   Rysowanie do własnego framebuffera, którego załącznikami są tekstury, zamiast do okna. Wynik da się potem czytać jak każdą teksturę w następnym przebiegu.

2. **Jakie załączniki ma framebuffer sceny i w jakich formatach?**
   Teksturę koloru `GL_RGBA16F` (16-bitowe liczby zmiennoprzecinkowe na kanał) i teksturę głębi `GL_DEPTH_COMPONENT24`. Obie mają rozmiar framebuffera okna w pikselach.

3. **Co znaczy HDR i po co grze wartości powyżej 1?**
   Obraz, w którym wartości kanałów nie są ograniczone do 1. Dzięki nim da się zmienić ekspozycję bez utraty jasnych partii, sprowadzić zakres krzywą zamiast go ucinać i odróżnić to, co naprawdę świeci (kryształ: zielony 1,97), od tego, co jest tylko białe.

4. **Jak trzy wierzchołki pokrywają cały ekran?**
   Trójkąt (-1, -1), (3, -1), (-1, 3) w przestrzeni przycinania zawiera kwadrat ekranu od -1 do 1. Nadmiar jest odcinany. Współrzędne tekstury (0, 0), (2, 0), (0, 2) dają na widocznej części zakres od 0 do 1.

5. **Skąd shader wierzchołków bierze pozycje, skoro nie ma bufora?**
   Z `gl_VertexID`: reszta z dzielenia przez 2 daje x, dzielenie całkowite przez 2 daje y, oba pomnożone przez 2.

6. **Po co pusty VAO?**
   Profil Core nie pozwala rysować bez związanego obiektu tablicy wierzchołków. VAO nie ma atrybutów, ale musi istnieć i być związany.

7. **Dlaczego jeden trójkąt, a nie dwa?**
   Nie ma wspólnej przekątnej, wzdłuż której karta wykonywałaby część pracy dwa razy, i nie potrzeba żadnych danych wierzchołków.

8. **Co robi ekspozycja i dlaczego stoi przed mapowaniem tonów?**
   Mnoży liniowy kolor sceny przez liczbę, jak czas naświetlania. Podwojenie to jeden stopień. Musi działać na wartościach liniowych i przed krzywą, bo to krzywa sprowadza przesunięty już zakres do ekranu.

9. **Podaj wzór Reinharda i jego wadę.**
   `x / (1 + x)`. Nic nie ucina (wynik zawsze poniżej 1), ale cała krzywa leży pod prostą `y = x`: biel 1 staje się 0,5, obraz jest ciemniejszy i płaski.

10. **Czym jest krzywa ACES w tym kodzie?**
    Dopasowaniem Narkowicza: ilorazem dwóch wielomianów drugiego stopnia z pięcioma stałymi (2,51, 0,03, 2,43, 0,59, 0,14). Ma kształt S: przyciemnia wartości poniżej około 0,06, podnosi środek, łagodnie ściska jasne.

11. **Dlaczego wynik ACES jest ujęty w `clamp`?**
    Granica wzoru dla dużych wejść to `2,51 / 2,43 = 1,033`, więc od około 7,24 wynik przekracza 1.

12. **Dlaczego po wprowadzeniu ACES trzeba było dobrać światła od nowa?**
    Nocna scena ma większość wartości liniowych poniżej 0,06, a tam krzywa przyciemnia (0,01 daje 0,0038). Do tego kolory są teraz przeliczane z sRGB na liniowe. Stąd nowe wartości światła otoczenia, intensywności, świecenia kryształów i jasności nieba.

13. **Gdzie klatka jest kodowana do sRGB?**
    W ostatniej linii `composite.frag`, funkcją `linearToSrgb`. To jedyne miejsce.

14. **Dlaczego nie `glEnable(GL_FRAMEBUFFER_SRGB)`?**
    Kodowałoby także ImGui, rysowane potem do tego samego okna, i rozjaśniłoby panele. Działa tylko, gdy framebuffer okna jest sRGB, co zależy od systemu. Z kodowaniem w shaderze groziłoby podwójnym kodowaniem.

15. **W jakiej kolejności idzie klatka?**
    `beginScene`, czyszczenie, scena z niebem na końcu, podglądy (tylko przy otwartym panelu), kopia ustawień, `drawBloom`, `composite`, ImGui.

16. **Co się dzieje, gdy okno jest zminimalizowane?**
    Framebuffer okna ma 0 x 0. `onRender` kończy się od razu: nic nie jest rysowane, bufor sceny zachowuje ostatni rozmiar.

17. **Dlaczego widoki diagnostyczne omijają ekspozycję, krzywą i bloom?**
    Pokazują dane (normalną, współrzędną tekstury), a nie światło. Krzywa zmieniłaby liczby, a bloom rozlałby jasne dane na sąsiednie piksele. `onRender` robi kopię ustawień z ekspozycją 1, `ToneMapping::None` i wyłączonym bloomem, a shadery sceny zapisują `srgbToLinear(dane)`, żeby kodowanie na końcu oddało te same liczby.

18. **Dlaczego surowa głębia jest prawie biała?**
    Rzutowanie perspektywiczne zapisuje wartość, w której odległość stoi w mianowniku. Dla płaszczyzn 0,1 m i 100 m ściana 2 m dalej ma już 0,951, a 15 m dalej 0,994.

19. **Wyprowadź wzór na głębię liniową.**
    `ndc = (f + n) / (f - n) - 2fn / ((f - n) d)`, bufor przechowuje `(ndc + 1) / 2`. Odwrotnie: `ndc = 2 * stored - 1`, `d = 2fn / (f + n - ndc (f - n))`.

20. **Dlaczego podglądy mają własne framebuffery?**
    ImGui pokazuje teksturę bez przeliczenia, a załączniki sceny nie nadają się do pokazania wprost: kolor jest liniowy i ponad 1, głębia nieliniowa. Do tego nie wolno czytać tekstury, do której się rysuje, więc wynik musi trafić gdzie indziej.

21. **Kiedy podglądy są rysowane?**
    Tylko gdy `PostProcessSettings::previews` jest prawdą. Flagę zeruje `DebugUI::draw` na początku klatki, a ustawia panel Framebuffers, gdy jest otwarty. Gra czyta ją w następnej klatce.

22. **Dlaczego obraz w `ImGui::Image` ma odwrócone współrzędne pionowe?**
    Tekstura framebuffera ma `v = 0` na dole, a ImGui rysuje od góry.

23. **Czego z tematu 10 jeszcze nie ma?**
    Mgły, winiety i minimapy. Są w planie dalszych części M7. Miejsca mgły i winiety w kolejności kroków są zapisane w komentarzu `composite.frag`.

24. **Co to jest bloom i z jakich kroków się składa?**
    Poświata wokół miejsc jaśniejszych od progu. Trzy kroki: przebieg jasności zostawia światło ponad progiem, rozmycie Gaussa rozlewa je na sąsiadów, przebieg składający dodaje wynik do sceny.

25. **Dlaczego bloom potrzebuje bufora HDR?**
    W buforze obciętym do 1 kryształ i biała ściana mają tę samą liczbę, więc próg ich nie odróżni i nie wiadomo, o ile coś jest jaśniejsze od bieli. W `GL_RGBA16F` kryształ ma jasność około 2,5, a nadwyżka ponad próg mówi, jak mocna ma być poświata.

26. **Jak liczona jest jasność koloru i skąd wagi?**
    `L = 0,2126 R + 0,7152 G + 0,0722 B`, wagi normy Rec. 709. Zielony liczy się najbardziej, bo oko jest na niego najczulsze. Wagi sumują się do 1 i mają sens tylko dla wartości liniowych.

27. **Podaj wzór przebiegu jasności i powiedz, dlaczego dzieli się przez jasność.**
    `wynik = kolor * max(L - T, 0) / L`. Licznik to nadwyżka ponad próg. Dzielenie przez `L` zamienia ją w udział, przez który mnożony jest cały kolor, więc trzy kanały maleją tyle samo razy i barwa zostaje. Jasność wyniku to dokładnie `L - T`, więc na progu nie ma skoku.

28. **Co by się stało przy twardym progu albo przy odjęciu progu od każdego kanału?**
    Twardy próg: poświata zapalałaby się i gasła skokiem, a krawędzie migotałyby. Odjęcie od kanałów: kanały poniżej progu znikają w całości i poświata ma inną barwę niż źródło.

29. **Podaj wzór funkcji Gaussa użytej w rozmyciu i jej parametry w tym kodzie.**
    `exp(-d * d / (2 * sigma * sigma))`, `sigma = 3`, promień 6, czyli 13 odczytów i 7 różnych wag: od 0,1370 w środku do 0,0185 na brzegu.

30. **Po co normalizacja wag?**
    Żeby suma całego jądra wynosiła 1: rozmycie przesuwa wtedy światło, ale go nie dodaje ani nie gubi. Przy dwunastu przebiegach z rzędu każdy błąd sumy rósłby wykładniczo.

31. **Co znaczy, że rozmycie Gaussa jest rozdzielne, i ile to oszczędza?**
    Dwuwymiarowy dzwon jest iloczynem dwóch jednowymiarowych, więc rozmycie wierszy, a potem kolumn wyniku daje to samo co jądro 13 x 13. Zamiast 169 odczytów na piksel jest 26.

32. **Co to jest ping-pong i dlaczego w kodzie są trzy cele?**
    Dwa cele zamieniają się rolami źródła i celu, bo przebieg nie może czytać tekstury, do której rysuje. Trzeci cel trzyma wynik przebiegu jasności, którego rozmycie nigdy nie zamazuje, żeby panel mógł go pokazać.

33. **Dlaczego cele bloomu mają połowę rozdzielczości?**
    Ćwierć pikseli to ćwierć kosztu, to samo jądro sięga na ekranie dwa razy dalej, a rozmyty obraz nie ma szczegółów, po których byłoby widać mniejszy rozmiar. Filtr liniowy uśrednia przy zmniejszaniu i wygładza przy powiększaniu.

34. **Ile przebiegów ma bloom w jednej klatce?**
    Jeden przebieg jasności i dwa na każdą iterację rozmycia. Przy sześciu iteracjach 13, w zakresie suwaka od 3 do 21. Do tego dwa podglądy przy otwartym panelu.

35. **Jak zmienia się szerokość poświaty z liczbą iteracji?**
    Jak pierwiastek: `sigma * sqrt(n)`. Sześć iteracji z `sigma = 3` to około 7 pikseli celu, czyli około 14 pikseli ekranu.

36. **Dlaczego bloom jest dodawany przed ekspozycją i mapowaniem tonów?**
    Poświata jest światłem, a światło dodaje się na wartościach liniowych. Ekspozycja i krzywa traktują ją potem jak resztę obrazu: krzywa sprowadza sumę do zakresu ekranu, zamiast ją obcinać, a suwak ekspozycji zmienia poświatę razem ze sceną.

37. **Jak wagi trafiają do shadera?**
    Liczy je `game::bloomBlurWeights` w C++, a `Shader::setFloatArray` wysyła jednym wywołaniem `glUniform1fv` do tablicy `uniform float uWeights[7]`. Promień jest zapisany w dwóch miejscach, w `Bloom.hpp` i w `blur.frag`, i muszą się zgadzać.

38. **Co pokazują dwa obrazy bloomu w panelu i kiedy widać `(not drawn)`?**
    `Bright pass` pokazuje to, co zostało po progu, `Bloom` to samo po rozmyciu, przed pomnożeniem przez intensywność. `(not drawn)` pojawia się, gdy bloom jest wyłączony w panelu albo gdy włączony jest widok diagnostyczny.

39. **Dlaczego `CRYSTAL_GLOW_STRENGTH` wzrosło z 2,5 do 4,0?**
    Świecenie jest mnożone przez teksturę kryształu, a w najciemniejszej chwili pulsu jeszcze przez 0,7. Przy 2,5 to, co zostawało, spadało pod próg 0,8 i poświata znikała w rytm pulsu. Przy 4,0 zapas jest dwa razy większy.

40. **Czy poświata ma tę samą szerokość w każdej rozdzielczości?**
    Nie. Jest mierzona w pikselach celu, więc na większym ekranie jest względnie cieńsza. To znane, otwarte ograniczenie.

## 10. Źródła

- LearnOpenGL, "Framebuffers" (<https://learnopengl.com/Advanced-OpenGL/Framebuffers>): obiekt framebuffera, załączniki, rysowanie sceny do tekstury i prostokąt pełnoekranowy.
- LearnOpenGL, "HDR" (<https://learnopengl.com/Advanced-Lighting/HDR>): bufor zmiennoprzecinkowy, mapowanie tonów Reinharda, ekspozycja.
- LearnOpenGL, "Gamma Correction" (<https://learnopengl.com/Advanced-Lighting/Gamma-Correction>): tekstury sRGB i kodowanie na końcu potoku.
- LearnOpenGL, "Bloom" (<https://learnopengl.com/Advanced-Lighting/Bloom>): przebieg jasności, rozdzielne rozmycie Gaussa z ping-pongiem między dwoma framebufferami, dodanie przed mapowaniem tonów. Różnice wobec tego kodu: tam jasne piksele wybiera drugi załącznik koloru i twardy próg, a wagi są wpisane w shader.
- Rekomendacja ITU-R BT.709 (Rec. 709): współczynniki luminancji 0,2126, 0,7152 i 0,0722 użyte w `REC709_LUMINANCE_WEIGHTS`.
- Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve", wpis na blogu z 6 stycznia 2016 (<https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/>): wzór i pięć stałych użytych w `toneMapAces`.
- Erik Reinhard, Michael Stark, Peter Shirley, James Ferwerda, "Photographic Tone Reproduction for Digital Images", SIGGRAPH 2002: operator `x / (1 + x)`.
- Khronos OpenGL Wiki, "Framebuffer Object" (<https://www.khronos.org/opengl/wiki/Framebuffer_Object>) i "Depth Buffer Precision" (<https://www.khronos.org/opengl/wiki/Depth_Buffer_Precision>): załączniki, pętla zwrotna, nieliniowość głębi.
- docs.gl: `glBindFramebuffer` (<https://docs.gl/gl4/glBindFramebuffer>), `glDrawArrays`, `glDepthRange`, `glEnable` (`GL_FRAMEBUFFER_SRGB`), `glUniform` (<https://docs.gl/gl4/glUniform>, wariant `glUniform1fv` dla tablic), funkcja GLSL `textureSize` (<https://docs.gl/sl4/textureSize>).
- Specyfikacja OpenGL 4.1 Core (<https://registry.khronos.org/OpenGL/specs/gl/glspec41.core.pdf>): obiekty framebuffera, kodowanie sRGB przy zapisie.
- Dokumenty w tym repozytorium: [`../gfx/framebuffers.md`](../gfx/framebuffers.md) (klasa `Framebuffer`), [`../gfx/color-space.md`](../gfx/color-space.md) (sRGB, wartości liniowe, droga koloru), [`../debug-ui.md`](../debug-ui.md) (panele, `RawTextureSampler`), [`skybox.md`](skybox.md) (niebo na głębi 1), [`lighting-gouraud-phong.md`](lighting-gouraud-phong.md) (shadery, które wypełniają bufor sceny), [`../scene/camera.md`](../scene/camera.md) (macierz rzutowania), [`../gfx/shader-includes.md`](../gfx/shader-includes.md) (`#include`).
- Notatki o decyzjach: [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md), [`../../decisions/srgb-encode-in-shader.md`](../../decisions/srgb-encode-in-shader.md), [`../../decisions/aces-default-tone-mapping.md`](../../decisions/aces-default-tone-mapping.md), [`../../decisions/depth-attachment-as-texture.md`](../../decisions/depth-attachment-as-texture.md), [`../../decisions/post-process-in-game-layer.md`](../../decisions/post-process-in-game-layer.md). Z drugiej części M7: [`../../decisions/bloom-half-resolution-three-targets.md`](../../decisions/bloom-half-resolution-three-targets.md), [`../../decisions/bright-pass-keeps-hue.md`](../../decisions/bright-pass-keeps-hue.md), [`../../decisions/blur-weights-computed-on-cpu.md`](../../decisions/blur-weights-computed-on-cpu.md), [`../../decisions/crystal-glow-raised-for-bloom.md`](../../decisions/crystal-glow-raised-for-bloom.md).
