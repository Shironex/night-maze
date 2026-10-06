# RmlUi 6.3 i FreeType 2.14.3

Dokument biblioteki dla kamienia milowego M9, części 2 (2026-10-06). Opisuje dwie zależności dodane do `cmake/Dependencies.cmake`: **RmlUi**, bibliotekę interfejsu, w której napisane jest menu gry, i **FreeType**, bibliotekę czcionek, której RmlUi potrzebuje. Dokument o samej warstwie, która z RmlUi korzysta (klasa `ui::UiLayer`, kolejność klatki, kolejka akcji), to [`../modules/ui/README.md`](../modules/ui/README.md). Dokument o ekranach gry to [`../modules/game/game-states.md`](../modules/game/game-states.md).

**Czego ten dokument nie obiecuje.** Wszystko poniżej o samym działaniu RmlUi jest czytane z kodu projektu i z jego komentarzy, z nagłówków i z źródeł RmlUi 6.3 pobranych do przeglądu (ten sam tag), a nie z uruchamiania eksperymentów. Co zostało zobaczone w działającym programie, jest wprost oznaczone, a na macOS nic z tego nie było budowane ani uruchamiane.

## 1. Czym jest RmlUi

RmlUi to biblioteka C++ do budowania interfejsu użytkownika opisanego **dokumentami w stylu HTML i CSS**: elementy w pliku `.rml` (Rocket Markup Language, jak XHTML), styl w pliku `.rcss` (Rocket Cascading Style Sheets, podzbiór CSS z własnymi dodatkami). Biblioteka jest **retained mode**: program raz wczytuje dokument, a biblioteka trzyma drzewo elementów, liczy układ (layout), animacje i przejścia, i dopiero na prośbę rysuje. Przeciwieństwem jest Dear ImGui ([`imgui.md`](imgui.md)), który jest **immediate mode**: kod opisuje interfejs od nowa w każdej klatce. Ta różnica jest głównym powodem, dla którego gra ma obie biblioteki: ImGui jest wygodny do paneli narzędziowych pisanych w kilku liniach, a RmlUi pozwala zrobić menu, które wygląda i rusza się jak część gry.

### Za co RmlUi NIE odpowiada

RmlUi nie otwiera okna, nie czyta plików, nie rysuje pikseli i nie zna zegara. Wszystko to robią **trzy interfejsy**, które dostarcza program:

| Interfejs | Za co odpowiada | W tym projekcie |
|---|---|---|
| `Rml::FileInterface` | otwieranie i czytanie plików (dokumentów, arkuszy stylów, czcionek, obrazów) | własna klasa `ui::AssetFileInterface` (czyta z katalogu `assets/` programu) |
| `Rml::SystemInterface` | czas, schowek, kształt kursora, komunikaty biblioteki | klasa `SystemInterface_GLFW` z backendu RmlUi, z jedną zmianą: komunikaty idą do logu gry |
| `Rml::RenderInterface` | rysowanie list trójkątów z teksturami | klasa `RenderInterface_GL3` z backendu RmlUi, bez zmian |

RmlUi produkuje listy trójkątów i prosi interfejs renderujący o ich narysowanie. To samo robi Dear ImGui (`ImDrawData` i backend renderera), więc zasada jest znana z [`imgui.md`](imgui.md).

### Pojęcia: kontekst, dokument, element

- **Kontekst** (`Rml::Context`) to powierzchnia, na której leżą dokumenty. Ma rozmiar w pikselach i współczynnik pikseli niezależnych od gęstości (dp, sekcja 5). Program ma jeden, o nazwie `menu`.
- **Dokument** (`Rml::ElementDocument`) to jeden wczytany plik `.rml`. Po wczytaniu jest ukryty, dopóki program nie wywoła `Show()`.
- **Element** (`Rml::Element`) to jeden węzeł drzewa: `div`, `button`, `p`, `span`. Ma atrybuty, styl i dzieci.

## 2. Dlaczego RmlUi jest w projekcie

Decyzja właściciela z 2026-10-06: menu gry powstanie w RmlUi, a Dear ImGui zostaje przy panelach debug ([`../decisions/menu-in-rmlui.md`](../decisions/menu-in-rmlui.md)). Powody właściciela są w tamtej notatce (płynność i animacje menu, droga do dalszej rozbudowy gry). Skutek, który notatka zapisuje wprost: zasada "zero tajemnic" z PRD jest dla warstwy menu świadomie poluzowana, bo biblioteka i jej podłączenie są większe, niż zdąży się wytłumaczyć na obronie.

## 3. Przypięte wersje

| Biblioteka | Tag | Data (odczyt przez API GitHub, 2026-10-06) | Licencja |
|---|---|---|---|
| RmlUi | `6.3` | wydanie z 2026-08-22 | MIT |
| FreeType | `VER-2-14-3` | commit tagu z 2026-03-22 | FreeType License (FTL) |

Obie są pobierane w czasie konfiguracji, jak pozostałe zależności (`GIT_SHALLOW TRUE`, bez historii), a nie leżą w repozytorium. Daty to odczyt z API GitHuba zrobiony przy dodawaniu zależności (zgłoszone przez autora kodu, nie do sprawdzenia w repozytorium); nie odczytywałem ich ponownie przy pisaniu tego dokumentu.

## 4. Jak są pobierane i co zostało wyłączone

Plik: [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake). Pozostałe zależności opisuje [`glfw.md`](glfw.md), [`imgui.md`](imgui.md) i inne dokumenty tego katalogu.

### 4.1 FreeType: pięć bibliotek wyłączonych

RmlUi nie czyta plików czcionek sama, tylko prosi FreeType. FreeType potrafi użyć pięciu innych bibliotek, jeśli znajdzie je na komputerze (zlib, bzip2, libpng, HarfBuzz, Brotli). Wszystkie pięć jest wyłączone:

```cmake
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
```

(Plik: `cmake/Dependencies.cmake`.) **Dlaczego:** build ma być taki sam na każdym komputerze i niczego nie linkować tylko dlatego, że jest zainstalowane (komentarz w pliku wskazuje Homebrew na macOS, który ma wszystkie pięć). Bez systemowej biblioteki zlib FreeType używa małej kopii, która jest częścią jego własnych źródeł (dlatego plik `THIRD-PARTY-NOTICES.txt` ma osobną sekcję dla tej kopii zlib). **Co tracimy** (z komentarza w pliku): czcionki kompresowane bzip2, kolorowe emoji zapisane jako PNG, pliki WOFF2 i automatyczne podpowiedzi (hinting) przez HarfBuzz. Gra ładuje zwykły plik TrueType i niczego z tego nie potrzebuje.

### 4.2 FreeType: `OVERRIDE_FIND_PACKAGE` i alias

```cmake
FetchContent_Declare(
    Freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-14-3
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(Freetype)

add_library(Freetype::Freetype ALIAS freetype)
```

(Plik: `cmake/Dependencies.cmake`.) RmlUi szuka FreeType przez `find_package(Freetype)`. Słowo `OVERRIDE_FIND_PACKAGE` sprawia, że to wywołanie dostaje źródło pobrane tutaj, a nigdy FreeType zainstalowany w systemie. Nazwa w `FetchContent_Declare` musi być taka, o jaką pyta RmlUi (`Freetype`). RmlUi linkuje cel `Freetype::Freetype`: zainstalowany FreeType ma cel o tej nazwie, ale FreeType zbudowany ze źródeł jako część innego projektu ma tylko cel `freetype`, więc drugą nazwę dodaje alias.

### 4.3 RmlUi

```cmake
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rmlui
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG 6.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rmlui)
```

(Plik: `cmake/Dependencies.cmake`.) RmlUi domyślnie zbudowałby bibliotekę współdzieloną (DLL). `BUILD_SHARED_LIBS OFF` wymusza statyczną, jak każda inna zależność, więc nie dochodzi żaden plik DLL do paczki. Przykłady (`RMLUI_SAMPLES`) i dowiązania Lua są w RmlUi domyślnie wyłączone i zostają wyłączone (komentarz w pliku, a opcje `RMLUI_SAMPLES` i `RMLUI_LUA_BINDINGS` mają w `CMakeLists.txt` RmlUi 6.3 wartość `OFF`). Nagłówki `rmlui_core` są oznaczone jako systemowe (`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`), żeby nie dawały ostrzeżeń w naszym kodzie i żeby clang-tidy ich nie sprawdzał: jego filtr nagłówków dopasowuje każdą ścieżkę z `src/`, a pobrane źródło leży w katalogu o nazwie `rmlui-src`.

### 4.4 Backendy: dwa pliki RmlUi kompilowane bez zmian

RmlUi sam produkuje tylko listy trójkątów. Kod, który je rysuje, i kod, który tłumaczy zdarzenia okna, to **backendy**: pliki dołączone do RmlUi, ale niebędące częścią jego biblioteki, jak backendy Dear ImGui. Projekt kompiluje dwa z nich, bez zmian, w statycznej bibliotece `rmlui_backend`:

```cmake
add_library(rmlui_backend STATIC
    ${rmlui_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.cpp
    ${rmlui_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp
)
target_include_directories(rmlui_backend SYSTEM PUBLIC ${rmlui_SOURCE_DIR}/Backends)
target_link_libraries(rmlui_backend PUBLIC RmlUi::RmlUi glad glfw)
target_compile_definitions(rmlui_backend PRIVATE
    "RMLUI_GL3_CUSTOM_LOADER=<glad/gl.h>"
    GLFW_INCLUDE_NONE
)
```

(Plik: `cmake/Dependencies.cmake`, bez komentarzy.) Znaczenie:

- **Renderer GL3** (`RenderInterface_GL3`) rysuje dla OpenGL 3.3 i nowszego, więc pasuje do kontekstu 4.1 Core. Jego shadery zaczynają się od `#version 330` (w źródle backendu, w gałęzi dla pulpitowego OpenGL), co kontekst 4.1 Core przyjmuje. Nie są to shadery z `assets/shaders/` i nie przechodzą przez `gfx::Shader`: RmlUi kompiluje własne w konstruktorze renderera, dlatego liczba programów shaderów gry (czternaście) się nie zmieniła i panel Shaders ich nie przeładowuje.
- **Platforma GLFW** (`RmlUi_Platform_GLFW`) dostarcza `SystemInterface_GLFW` oraz funkcje `RmlGLFW::Process...Callback`, które tłumaczą zdarzenia GLFW na zdarzenia RmlUi.
- **`RMLUI_GL3_CUSTOM_LOADER=<glad/gl.h>`.** Renderer GL3 ma domyślnie własną kopię loadera GLAD dla OpenGL 3.3. Ta definicja każe mu dołączyć wskazany nagłówek: **GLAD tego projektu** ([`glad.md`](glad.md)), który `core::Window` już załadował dla OpenGL 4.1 Core. Jeden loader, jeden zestaw wskaźników funkcji.
- **`GLFW_INCLUDE_NONE`** zabrania GLFW dołączania własnego nagłówka OpenGL obok GLAD (komentarz w pliku: jeden loader).
- **Pliku `RmlUi_Backend_GLFW_GL3.cpp` projekt NIE używa.** Ten backend tworzy własne okno i własną pętlę główną, a program ma jedno i drugie ([`../modules/core/window-context.md`](../modules/core/window-context.md), [`../modules/core/main-loop.md`](../modules/core/main-loop.md)).

Do projektu `night_maze` linkuje się tylko biblioteka `ui`, a ona linkuje `rmlui_backend` jako zależność prywatną. Ani `engine`, ani `game_logic`, ani `night_maze_tests` nie linkują RmlUi ani FreeType ([`../modules/ui/README.md`](../modules/ui/README.md), sekcja 5.1).

## 5. RML i RCSS w kilku słowach

Dokument menu głównego, [`assets/ui/main_menu.rml`](../../assets/ui/main_menu.rml), w całości:

```xml
<rml>
<head>
    <title>Main menu</title>
    <link type="text/rcss" href="menu.rcss"/>
</head>
<body>
<div id="panel">
    <h1>Night Maze</h1>
    <p>Find the crystals, open the gate, get out.</p>
    <button data-action="play">Play</button>
    <button data-action="quit">Quit</button>
</div>
</body>
</rml>
```

- Korzeniem jest `<rml>` z `<head>` i `<body>`. `<link type="text/rcss" href="menu.rcss"/>` dołącza arkusz stylów. Ścieżka `menu.rcss` jest względna do dokumentu, a plik czyta nasz interfejs plików (sekcja 7).
- Elementy są jak w HTML: `div`, `h1`, `p`, `button`. Atrybut `id` służy do wyszukiwania elementu z kodu (`GetElementById`).
- `data-action="play"` to **zwykły atrybut, który wymyśla projekt**: RmlUi nic o nim nie wie. Kod warstwy menu szuka go przy kliknięciu i zapisuje jego wartość na liście nazw kliknięć ([`../modules/ui/README.md`](../modules/ui/README.md), sekcja 5.4).

Fragment arkusza [`assets/ui/menu.rcss`](../../assets/ui/menu.rcss):

```css
body {
    display: block;
    width: 100%;
    height: 100%;
    font-family: "Atkinson Hyperlegible";
    font-size: 18dp;
    color: #e8ecf4;
    background-color: #05081099;
}
button {
    display: block;
    width: 220dp;
    margin: 14dp auto 0 auto;
    padding: 10dp 0;
    color: #dfe6f5;
    background-color: #24345c;
    border: 1dp #5b7cc4;
    border-radius: 6dp;
    cursor: pointer;
    transition: background-color color 0.2s cubic-out;
}
button:hover {
    color: #101828;
    background-color: #e0a030;
}
```

(Plik: `assets/ui/menu.rcss`, z pominiętymi komentarzami i regułami `div, h1, p`, `#panel`, `h1`, `p` i `button:active`.)

- **Jednostka `dp`** to piksel niezależny od gęstości: 1 dp to 1 piksel przy skalowaniu ekranu 100 procent, 1,5 piksela przy 150 procentach na Windowsie, 2 piksele na ekranie Retina. Dzięki niej menu ma ten sam rozmiar na każdym ekranie. Współczynnik jest ustawiany w każdej klatce z `glfwGetWindowContentScale` (kod w `UiLayer::draw`).
- **Kolor `#05081099`** to osiem cyfr szesnastkowych: czerwony, zielony, niebieski i **alfa** (`99` to około 60 procent). Dlatego tło `body` jest półprzezroczyste i widać pod nim przyciemniony obraz gry.
- **`button:hover`** zmienia kolor pod kursorem, a `transition` rozkłada tę zmianę na 0,2 s z krzywą `cubic-out`. To jedyna animacja w menu dziś.
- **Czcionka** `"Atkinson Hyperlegible"` to nazwa rodziny, pod którą RmlUi zna plik załadowany przez `Rml::LoadFontFace(core::TEXT_FONT_FILE)`. Ten sam plik czcionki co w panelach debug, `assets/fonts/AtkinsonHyperlegible-Regular.ttf` ([`../modules/debug-ui.md`](../modules/debug-ui.md)).
- **RmlUi nie ma wbudowanego arkusza stylów.** Komentarz w pliku: bez reguły `display: block` każdy element jest w linii (inline). Dlatego `body` i `div, h1, p` mają `display: block` zapisane jawnie.

## 6. Z czego projekt korzysta, a z czego nie

**Używa:**

- jeden kontekst, trzy dokumenty (`main_menu.rml`, `pause.rml`, `round_end.rml`), jeden arkusz `menu.rcss`,
- `LoadDocument`, `Show` i `Hide`, `GetElementById`, `SetInnerRML` (dwa teksty na ekranie wyniku),
- jeden `Rml::EventListener` na zdarzenie `click` na kontekście,
- przejścia (`transition`) w arkuszu i pseudoklasy `:hover` i `:active`,
- renderer GL3 i platformę GLFW z backendów, jeden plik czcionki.

**Nie używa:** wiązań danych (data binding), pluginu debuggera, Lua, wielu kontekstów, animacji `@keyframes`, filtrów ani `backdrop-filter`, obrazów w dokumentach, pól tekstowych (kod rozpoznaje `input` i `textarea` w `wantsKeyboard`, ale żaden dokument ich nie ma), kilku czcionek, obsługi dotyku, zmiany czcionek w czasie działania. To, co jest w tym akapicie jako "nie używa", sprawdziłem wyszukiwaniem w `assets/ui/` i `src/ui/`.

## 7. Licencja

- **RmlUi: MIT** (`LICENSE.txt` w źródłach: Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors oraz (c) 2019-2026 The RmlUi Team, and contributors). Wymaga zachowania noty w rozpowszechnianej kopii. W RmlUi są dołączone kontenery (`itlib`, `robin_hood`) z własnym plikiem licencji (MIT).
- **FreeType: FreeType License (FTL)**, którą wybrano spośród dwóch licencji, na jakich FreeType jest dostępny (druga to GPLv2). FTL wymaga wiersza podziękowania ("Portions of this software are copyright (C) 2026 The FreeType Project (www.freetype.org). All rights reserved.") w dokumentacji rozpowszechnianej gry. Kopia zlib wewnątrz FreeType ma własną notę.
- **Gdzie to jest:** plik `THIRD-PARTY-NOTICES.txt` w korzeniu repozytorium, **generowany** skryptem `node launcher/scripts/build-notices.mjs --deps build/release/_deps` (cztery sekcje doszły w tej części: RmlUi, kontenery RmlUi, FreeType z linią podziękowania i tekstem FTL, kopia zlib z FreeType; razem plik ma dziesięć sekcji: GLFW, GLM, Dear ImGui, stb_image, RmlUi, kontenery RmlUi, FreeType, kopia zlib, GLAD i czcionka Atkinson Hyperlegible, policzone z zatwierdzonego pliku). Trzeba go wygenerować od nowa, gdy zmieni się wersja w `Dependencies.cmake`. Czcionka Atkinson Hyperlegible ma własną licencję SIL OFL w `assets/fonts/OFL.txt`.

## 8. Pułapki

1. **Pliki otwiera nasz interfejs, nie RmlUi.** RmlUi nigdy nie otwiera pliku sam: o każdy dokument, arkusz, czcionkę i obraz pyta `FileInterface`. Domyślny interfejs RmlUi otwiera pliki przez wąski napis i względem katalogu roboczego, więc przy ścieżce instalacji ze znakami spoza ASCII na Windowsie albo przy uruchomieniu z innego katalogu nie znalazłby niczego. Projekt ma własną klasę czytającą z katalogu `assets/` obok programu, z UTF-8 ([`../modules/ui/README.md`](../modules/ui/README.md), sekcja 5.2).
2. **`backdrop-filter` nie rozmyje klatki gry.** Wniosek z czytania źródła renderera GL3 w RmlUi 6.3, **nie sprawdzony na ekranie**: renderer rysuje dokument do własnych warstw (framebufferów), a `EndFrame` dopiero kopiuje wynik na okno. Klatka gry leży w domyślnym framebufferze okna, poza tymi warstwami, więc `backdrop-filter` miałby do rozmycia tylko to, co RmlUi samo narysowało pod spodem. Gdyby menu miało rozmywać scenę, musiałaby to zrobić gra własnym przebiegiem (rozmycie bloomu jest w `game::PostProcess`) przed `UiLayer::draw`. Przy tle w postaci pętli wideo ([`../decisions/video-through-os-decoders-with-still-fallback.md`](../decisions/video-through-os-decoders-with-still-fallback.md)) rozmycie sceny nie jest potrzebne.
3. **Bez czcionki nie ma tekstu.** RmlUi nie ma wbudowanej czcionki. Jeśli `Rml::LoadFontFace` zawiedzie, dokumenty wczytują się, ale napisów nie widać. Konstruktor `UiLayer` zapisuje w logu błąd i przerywa, a gra startuje bez menu.
4. **`BeginFrame` zapisuje stan OpenGL, a `Clear` renderera kasuje klatkę.** Przed `Render` kod woła `BeginFrame` (zapisuje stan i wiąże własny framebuffer), po nim `EndFrame` (nakłada wynik na okno i przywraca stan). Metody `Clear` renderera **nie wolno** wołać: wymazałaby klatkę gry. Komentarz w `UiLayer::draw` mówi to wprost.
5. **Tekst w `setText` jest RML.** `SetInnerRML` traktuje argument jako RML, więc tekst z `<` albo `&` zostałby zinterpretowany. Dwie liczby ekranu wyniku (czas i kryształy) nie zawierają takich znaków.
6. **Jedno wywołanie zwrotne każdego rodzaju na okno.** GLFW ma jedną funkcję zwrotną każdego rodzaju na okno, a `UiLayer` ustawia sześć (klawisz, znak, wejście kursora, pozycja kursora, przycisk myszy, kółko). Dear ImGui instaluje swoje później i, według komentarza w kodzie, przekazuje każde zdarzenie dalej do tych poprzednich, więc obie biblioteki widzą każde zdarzenie. Dlatego `UiLayer` musi powstać **przed** debug UI i zostać zniszczony **po** nim ([`../modules/ui/README.md`](../modules/ui/README.md), sekcja 5.3).
7. **Kolejność niszczenia.** `Rml::Shutdown` niszczy kontekst i dokumenty i zwalnia tekstury przez interfejs renderujący. Interfejsy muszą więc żyć dłużej niż `Shutdown`: są składowymi `UiLayer` i niszczą się po ciele destruktora.
8. **macOS.** Renderer GL3 z kontekstem 4.1 Core na Apple, FreeType i RmlUi budowane Apple clang oraz ostrość i pozycje trafień na Retina **nie były sprawdzone** ([`../guides/build-macos.md`](../guides/build-macos.md)).
9. **Obraz.** To, co poniżej, jest **widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela** (Windows, Release, 1280 x 720): menu główne, pauza i ekran wyniku (ten ostatni tylko przez tymczasową linię wymuszającą wygraną) wyświetlają się, przycisk pod kursorem zmienia kolor na pomarańczowy, kliknięcia dochodzą do gry, a panele debug działają nad menu ([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 26).

## 9. Pytania kontrolne

1. **Czym różni się RmlUi od Dear ImGui?**
   RmlUi jest retained: wczytuje dokument, trzyma drzewo elementów i liczy układ, animacje i przejścia. Dear ImGui jest immediate: kod opisuje interfejs od nowa w każdej klatce.
2. **Za co odpowiadają trzy interfejsy RmlUi i które z nich napisano w projekcie?**
   Pliki, system i rysowanie. Własny jest tylko interfejs plików (`ui::AssetFileInterface`), a interfejs systemu (z jedną zmianą w logu) i renderer pochodzą z backendów RmlUi.
3. **Po co `RMLUI_GL3_CUSTOM_LOADER`?**
   Żeby renderer GL3 używał GLAD tego projektu, który `core::Window` już załadował, zamiast własnej kopii loadera.
4. **Dlaczego pięć bibliotek FreeType jest wyłączonych?**
   Żeby build był taki sam na każdym komputerze i nie linkował niczego tylko dlatego, że jest zainstalowane.
5. **Co oznacza `220dp` w arkuszu i skąd RmlUi zna współczynnik?**
   Piksel niezależny od gęstości. Współczynnik gra podaje w każdej klatce z `glfwGetWindowContentScale`.
6. **Dlaczego nie wolno wołać `Clear` renderera?**
   Wymazałby klatkę gry, na którą menu jest nakładane.
7. **Dlaczego `backdrop-filter` nie rozmyje gry?**
   Bo renderer rysuje do własnych warstw, a klatka gry leży poza nimi (wniosek z kodu biblioteki, nie sprawdzony na ekranie).

## 10. Źródła

- Dokumentacja RmlUi: <https://mikke89.github.io/RmlUiDoc/> (podręcznik, RML, RCSS, interfejsy).
- Źródła RmlUi 6.3: <https://github.com/mikke89/RmlUi> (tag `6.3`; backendy w katalogu `Backends`).
- FreeType: <https://freetype.org> (licencja FTL w `docs/FTL.TXT` źródeł).
- W projekcie: [`../decisions/menu-in-rmlui.md`](../decisions/menu-in-rmlui.md), [`../modules/ui/README.md`](../modules/ui/README.md), [`../modules/game/game-states.md`](../modules/game/game-states.md), [`imgui.md`](imgui.md), [`glad.md`](glad.md), [`glfw.md`](glfw.md).
