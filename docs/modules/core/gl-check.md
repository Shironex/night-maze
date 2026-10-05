# Moduł core: GL_CHECK i błędy OpenGL

Kamień milowy: M0. Temat wykładu: 1 (Pierwszy program OpenGL).
Kod: [`src/core/GlCheck.hpp`](../../../src/core/GlCheck.hpp), [`src/core/GlCheck.cpp`](../../../src/core/GlCheck.cpp), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp).

Część modułu `core`. Wstęp do całego modułu jest w [`README.md`](README.md). Pozostałe części: [`window-context.md`](window-context.md) (okno i kontekst), [`main-loop.md`](main-loop.md) (pętla i czas), [`input.md`](input.md) (klawiatura).

## 1. Po co to jest

OpenGL nie rzuca wyjątków i nie zwraca kodów błędów. Błędne wywołanie jest po cichu ignorowane, a w kontekście ustawia się flaga błędu, którą trzeba samemu odczytać przez `glGetError`. Nowoczesny sposób, czyli callback `glDebugMessageCallback`, wymaga OpenGL 4.3, a macOS kończy się na 4.1. Zostaje więc `glGetError` po każdym wywołaniu, a żeby nie pisać tego ręcznie, mam makro `GL_CHECK`: opakowuję nim każde własne wywołanie `gl*`, a w buildzie Debug dostaję w konsoli nazwę błędu, tekst wywołania, plik i numer linii. Na Macu to razem z panelami ImGui jedyne narzędzie diagnostyczne, bo RenderDoc tam nie działa.

## 2. Teoria

### 2.1 Model błędów OpenGL

Trzy cechy, z których wynika cała konstrukcja makra:

1. **Błąd nie przerywa programu.** Funkcja `gl*`, która dostała złe argumenty, zwykle nie robi nic (wyjątkiem jest `GL_OUT_OF_MEMORY`, po którym stan kontekstu jest nieokreślony). Program działa dalej, tylko obraz jest zły albo pusty.
2. **Błąd jest flagą w kontekście.** Zostaje tam, dopóki ktoś jej nie odczyta. Nie jest związany z wywołaniem, które go spowodowało: `glGetError` mówi "od ostatniego odczytu coś poszło źle", a nie "to wywołanie było złe".
3. **Flag może być kilka.** Specyfikacja OpenGL pozwala implementacji trzymać kilka flag błędów naraz. Jedno wywołanie `glGetError` zwraca jedną flagę i ją kasuje. Dopiero gdy zwróci `GL_NO_ERROR`, wiadomo, że wszystkie są wyczyszczone.

Wniosek: żeby błąd przypisać do konkretnego wywołania, trzeba pytać o błędy **zaraz po** tym wywołaniu i zawsze czytać flagi do końca.

### 2.2 Preprocesor i makra

Makro jest przetwarzane przez **preprocesor**, czyli przed właściwą kompilacją: tekst `GL_CHECK(...)` zostaje zastąpiony tekstem z definicji. To zamiana tekstu, a nie wywołanie funkcji. Dzięki temu makro potrafi dwie rzeczy niedostępne dla funkcji: zamienić swój argument na napis (operator `#`) oraz podać plik i linię **miejsca użycia** (`__FILE__`, `__LINE__`). Cena: makro nie zna typów, nie ma zasięgu i trzeba uważać, do czego się rozwija (sekcja 5.2).

### 2.3 Debug a Release

`NDEBUG` to standardowe makro ("no debug"), to samo, które wyłącza `assert`. CMake definiuje je automatycznie w konfiguracji Release (flaga `-DNDEBUG`). W Debug `NDEBUG` nie istnieje. `GL_CHECK` korzysta z tego rozróżnienia: w Debug sprawdza błędy, w Release zostaje samo wywołanie, bo `glGetError` po każdej funkcji kosztuje (wymusza synchronizację ze sterownikiem).

## 3. Jak to działa w OpenGL

Jedyna funkcja OpenGL w tej części modułu to `glGetError`:

| Wywołanie | Co robi |
|---|---|
| `glGetError()` | Zwraca jedną z ustawionych flag błędu jako `GLenum` i ją kasuje. Gdy żadna nie jest ustawiona, zwraca `GL_NO_ERROR` (wartość 0) |

Kody, które mogą wrócić w OpenGL 4.1:

| Kod błędu | Typowa przyczyna |
|---|---|
| `GL_INVALID_ENUM` | stała, której dana funkcja nie przyjmuje |
| `GL_INVALID_VALUE` | liczba spoza zakresu (ujemny rozmiar, zły indeks) |
| `GL_INVALID_OPERATION` | wywołanie niedozwolone w bieżącym stanie (na przykład rysowanie bez związanego VAO w profilu Core) |
| `GL_INVALID_FRAMEBUFFER_OPERATION` | rysowanie do niekompletnego framebuffera |
| `GL_OUT_OF_MEMORY` | brak pamięci, stan kontekstu po tym błędzie jest nieokreślony |

`glGetError` wymaga bieżącego kontekstu i załadowanych wskaźników GLAD, tak jak każda inna funkcja `gl*` ([`window-context.md`](window-context.md), sekcja 3.1).

## 4. Shadery

Ta część modułu nie ma shaderów. Warto jednak wiedzieć, czego `GL_CHECK` przy shaderach **nie** wykryje: nieudana kompilacja albo linkowanie shadera nie ustawia flagi błędu OpenGL. Wynik trzeba odczytać osobno: status przez `glGetShaderiv(GL_COMPILE_STATUS)` i `glGetProgramiv(GL_LINK_STATUS)`, a tekst błędu z dziennika sterownika. Robi to klasa `gfx::Shader`, opisana w [`../gfx/shader-class.md`](../gfx/shader-class.md) (sekcje 3.3, 5.5 i 5.6).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/core/GlCheck.hpp`](../../../src/core/GlCheck.hpp) | deklaracja `core::checkGlErrors` i makro `GL_CHECK` w dwóch wersjach (Debug, Release). Dołącza `<glad/gl.h>` |
| [`src/core/GlCheck.cpp`](../../../src/core/GlCheck.cpp) | `checkGlErrors`, pomocnicza `glErrorName` i stała `MAX_ERRORS_PER_CHECK` |
| [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) | trzy użycia w `onRender`: `glEnable`, `glClearColor`, `glClear`. `glDrawElements` kostki z M1 zniknęło w M5 razem z kostką, a `glViewport` w pierwszej części M7: obszar rysowania ustawia teraz `gfx::Framebuffer::bind` i `bindDefault` ([`../gfx/framebuffers.md`](../gfx/framebuffers.md)) |

### 5.2 Makro

```cpp
#ifndef NDEBUG
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
        core::checkGlErrors(#call, __FILE__, __LINE__);                                            \
    } while (false)
#else
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
    } while (false)
#endif
```

Znak `\` na końcu linii oznacza "definicja ciągnie się dalej". Po kolei:

- **`call;`** wstawia argument makra jako zwykłą instrukcję. Dla `GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));` powstaje `glClear(GL_COLOR_BUFFER_BIT);`. Przecinki wewnątrz nawiasów wywołania nie rozdzielają argumentów makra, dlatego `GL_CHECK(glViewport(0, 0, w, h))` działa.
- **`#call`** to operator zamiany na napis (stringification): preprocesor wstawia tekst argumentu w cudzysłowie, czyli `"glClear(GL_COLOR_BUFFER_BIT)"`. Dzięki temu log mówi, **które** wywołanie zawiodło, dokładnie tak, jak je napisałem. Tego nie da się zrobić zwykłą funkcją: funkcja dostaje wartości, a nie tekst kodu.
- **`__FILE__` i `__LINE__`** to wbudowane makra preprocesora: ścieżka bieżącego pliku i numer bieżącej linii. Ponieważ makro rozwija się **w miejscu użycia**, wskazują plik i linię, w której napisałem `GL_CHECK`, a nie `GlCheck.hpp`. To drugi powód, dla którego to musi być makro.
- **`do { ... } while (false)`** sprawia, że dwie instrukcje zachowują się jak jedna i wymagają średnika na końcu. Pętla wykonuje się dokładnie raz, a kompilator ją usuwa. Bez tej sztuczki taki kod byłby błędny:

  ```cpp
  if (visible)
      GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
  else
      skip();
  ```

  Gdyby makro rozwijało się do dwóch gołych instrukcji, do `if` należałaby tylko pierwsza. Gdyby rozwijało się do samego bloku `{ ... }`, średnik po nim zakończyłby instrukcję `if` i `else` nie miałoby do czego się odnieść (błąd kompilacji).
- **`#ifndef NDEBUG`** wybiera wersję (sekcja 2.3). W Debug makro sprawdza błędy. W Release zostaje samo wywołanie.

Wywołania zwracające wartość zapisuję z przypisaniem w środku: `GL_CHECK(id = glCreateShader(GL_VERTEX_SHADER));` (przykład z komentarza w `GlCheck.hpp`, w kodzie tak zapisane są `glCreateShader` i `glCreateProgram` w [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp)). Zmienna musi być zadeklarowana **przed** makrem: deklaracja wewnątrz `GL_CHECK(...)` trafiłaby do bloku `do { }` i zniknęła razem z nim.

Makro nie należy do przestrzeni nazw (preprocesor ich nie zna), dlatego stoi poza `namespace core` i woła funkcję pełną nazwą `core::checkGlErrors`.

### 5.3 Funkcja sprawdzająca

Kod: [`GlCheck.cpp`](../../../src/core/GlCheck.cpp).

```cpp
void checkGlErrors(const char* call, const char* file, int line) {
    // OpenGL keeps a set of error flags, so one call can leave more than one error behind.
    // glGetError returns and clears one flag at a time until it reports GL_NO_ERROR.
    GLenum error = glGetError();
    int errorCount = 0;
    while (error != GL_NO_ERROR && errorCount < MAX_ERRORS_PER_CHECK) {
        logError(std::string(glErrorName(error)) + " after " + call + " (" + file + ":" +
                 std::to_string(line) + ")");
        errorCount += 1;
        error = glGetError();
    }

    // Still an error after the limit: stop reading instead of spinning forever.
    if (error != GL_NO_ERROR) {
        logError("Stopped reading OpenGL errors after " + std::to_string(MAX_ERRORS_PER_CHECK) +
                 " errors (is the OpenGL context lost or not current?)");
    }
}
```

**Dlaczego pętla.** Jedno wywołanie `glGetError` zwraca jedną flagę i ją kasuje (sekcja 2.1). Gdybym czytał tylko raz, pozostałe flagi zostałyby w kontekście i wyskoczyłyby przy **następnym** `GL_CHECK`, obciążając winą niewinne wywołanie. Pętla "osusza" flagi do zera, więc każdy `GL_CHECK` zaczyna z czystym stanem.

**Dlaczego pętla ma limit.** Warunek `error != GL_NO_ERROR` zakłada, że `glGetError` w końcu zwróci `GL_NO_ERROR`. Tak jest w zdrowym kontekście, ale gdy kontekst OpenGL jest utracony albo nie jest bieżący, `glGetError` może zwracać błąd bez końca i pętla bez limitu zawiesiłaby program w środku klatki. Dlatego liczę odczytane błędy w `errorCount` i przerywam po `MAX_ERRORS_PER_CHECK`:

```cpp
// Most errors that one checkGlErrors call reads. Normally the loop ends much earlier, at
// GL_NO_ERROR. The limit exists because glGetError can keep returning an error forever when
// the OpenGL context is lost or not current, and then an unbounded loop would hang the program.
constexpr int MAX_ERRORS_PER_CHECK = 16;
```

Stała stoi w anonimowej przestrzeni nazw w `GlCheck.cpp`, obok `glErrorName`. Wartość 16 jest z dużym zapasem: OpenGL 4.1 ma tylko pięć rodzajów błędów (tabela w sekcji 3), więc w zdrowym kontekście pętla kończy się po kilku obrotach. Po pętli sprawdzam, dlaczego się skończyła. Jeśli `error` to `GL_NO_ERROR`, flagi są wyczyszczone i nic więcej nie robię. Jeśli `error` nadal jest błędem, pętlę zatrzymał limit, więc wypisuję jedną dodatkową linię: `[error] Stopped reading OpenGL errors after 16 errors (is the OpenGL context lost or not current?)`. Ostatni odczytany błąd nie jest już wypisywany z nazwą, zastępuje go właśnie ta linia.

Parametry to `const char*`, bo dokładnie taki typ mają napis z `#call` i `__FILE__` (literały napisowe), a `line` to `int`, jak `__LINE__`. Komunikat składam ze `std::string` i wysyłam do `logError` ([`window-context.md`](window-context.md), sekcja 5.5).

Przykładowy komunikat (ścieżka i numer linii zależą od miejsca użycia): `[error] GL_INVALID_ENUM after glEnable(GL_COLOR_BUFFER_BIT) (/.../src/game/NightMazeApp.cpp:29)`.

### 5.4 `glErrorName`

```cpp
const char* glErrorName(GLenum error) {
    switch (error) {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "unknown OpenGL error";
    }
}
```

`glGetError` zwraca liczbę (`GLenum`), a w logu chcę nazwę z dokumentacji OpenGL. `switch` zamienia jedno na drugie, a gałąź `default` obsługuje kod, którego nie znam, zamiast zwracać pusty wskaźnik. Funkcja zwraca wskaźnik na literał napisowy (żyje przez cały czas działania programu) i siedzi w anonimowej przestrzeni nazw, więc jest widoczna tylko w `GlCheck.cpp`.

### 5.5 Gdzie makro jest używane

```cpp
const core::Size framebuffer = window().framebufferSize();

if (framebuffer.width == 0 || framebuffer.height == 0) {
    return;
}

drawMoonShadowMap();

if (!m_postProcess.beginScene(framebuffer)) {
    return;
}

GL_CHECK(glEnable(GL_DEPTH_TEST));

const glm::vec3 clearColor =
    gfx::srgbToLinear(glm::vec3{m_clearColor[0], m_clearColor[1], m_clearColor[2]});
GL_CHECK(glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0F));
GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
```

To początek każdej klatki (komentarze pominięte, opis samych funkcji: [`window-context.md`](window-context.md), sekcja 3.2). Od pierwszej części M7 linii `glViewport` tu nie ma: scena jest rysowana do własnego framebuffera HDR, a wywołanie `glBindFramebuffer` razem z `glViewport` stoi w `gfx::Framebuffer::bind`, wołanym przez `m_postProcess.beginScene` (oba w makrze, [`../gfx/framebuffers.md`](../gfx/framebuffers.md)). Linia `drawMoonShadowMap();` doszła w czwartej części M7 (cienie księżyca, 2026-10-05): to przebieg cieni, który przed sceną rysuje ją z kierunku księżyca do tekstury głębi. Jego dwa własne wywołania OpenGL, `glEnable(GL_DEPTH_TEST)` i `glClear(GL_DEPTH_BUFFER_BIT)` w `ShadowMap::beginDepthPass`, też stoją w makrze ([`../renderer/shadows.md`](../renderer/shadows.md)). Po tym początku `onRender` rysuje labirynt, bramę, kryształy, trawę, linie kolizji i niebo, a na końcu przebieg składający (`m_postProcess.composite`) przenosi obraz do okna ([`../renderer/post-process.md`](../renderer/post-process.md)). Od M5 w `NightMazeApp.cpp` nie ma już żadnego wywołania rysującego napisanego wprost (do M4 była nim linia `glDrawElements` kostki z M1): wszystko rysuje `gfx::Mesh::draw`, w którym `glDrawElements` stoi w makrze, a `glUseProgram`, `glGetUniformLocation`, `glUniformMatrix4fv` i `glBindVertexArray` są opakowane w makro wewnątrz klas `gfx` ([`../gfx/README.md`](../gfx/README.md), sekcja 6). W makro opakowane jest też każde wywołanie w `src/gfx/`. Wywołania `glGetString` w konstruktorze `Window` nie są opakowane: stoją przed pierwszą klatką, a ich wynik jest i tak sprawdzany pod kątem `nullptr`. Backend ImGui woła OpenGL własnym loaderem i mojego makra nie używa.

## 6. Panel ImGui

`GL_CHECK` nie ma elementu w panelu. Jego wyjściem jest konsola: linie `[error] ...` na standardowym wyjściu błędów. Panel Renderer przydaje się pośrednio: edytowalny `Clear color` pozwala od razu zobaczyć, czy klatka jest w ogóle rysowana, gdy konsola pokazuje błędy.

## 7. Pułapki

1. **`GL_CHECK` obwinia nie to wywołanie.** Flaga błędu zostaje w kontekście, dopóki ktoś jej nie odczyta. Jeśli błąd spowoduje wywołanie **bez** `GL_CHECK` (także wewnątrz cudzej biblioteki, na przykład backendu ImGui), zgłosi go dopiero najbliższy `GL_CHECK` przy zupełnie innej funkcji (w M0 zwykle `glViewport` w następnej klatce, dziś pierwsze wywołanie w `gfx::Framebuffer::bind`). Wniosek: każde własne `gl*` opakowuję w makro.
2. **`GL_CHECK` nie działa w Release.** To celowe, ale oznacza, że błąd widoczny tylko w Release trzeba szukać, przełączając się na Debug.
3. **Brak debug callbacku.** `glDebugMessageCallback` to OpenGL 4.3. Wiele poradników go używa, na macOS to się nie skompiluje (GLAD 4.1 nie ma tej funkcji). Stąd `GL_CHECK`.
4. **Deklaracja wewnątrz makra.** `GL_CHECK(GLuint id = glCreateShader(...));` kompiluje się, ale `id` istnieje tylko wewnątrz bloku `do { }`. Zmienną deklaruję przed makrem.
5. **Efekt uboczny w argumencie.** Argument makra jest wklejany jako tekst. W obecnej definicji występuje raz jako instrukcja i raz jako napis (`#call`), więc wykonuje się raz. Gdyby ktoś przerobił makro tak, że `call` pojawia się dwa razy jako kod, wywołanie OpenGL wykonałoby się dwukrotnie.
6. **Zalew komunikatów.** Błędne wywołanie w `onRender` wypisuje linię w każdej klatce, czyli 60 lub więcej linii na sekundę. Program najlepiej od razu zatrzymać i przeczytać pierwszą linię.
7. **Błędy shaderów.** Nieudana kompilacja ani nieudane linkowanie shadera nie są błędem `glGetError` (sekcja 4). Jak się je odczytuje: [`../gfx/shader-class.md`](../gfx/shader-class.md), sekcja 3.3.
8. **Pętla `glGetError` bez limitu.** W wielu poradnikach jest samo `while (glGetError() != GL_NO_ERROR)`. Gdy kontekst jest utracony albo nie jest bieżący, `glGetError` może zwracać błąd za każdym razem i taka pętla nigdy się nie kończy: program wisi i nic nie wypisuje. U mnie pętla kończy się najpóźniej po `MAX_ERRORS_PER_CHECK` odczytach. Linia `Stopped reading OpenGL errors after 16 errors` w konsoli oznacza więc problem z kontekstem, a nie 16 osobnych pomyłek w kodzie.

## 8. Ćwiczenia

1. **Celowy błąd OpenGL.** W `NightMazeApp::onRender` dopisz `GL_CHECK(glEnable(GL_COLOR_BUFFER_BIT));`. Zbuduj Debug i przeczytaj komunikat: jaka nazwa błędu, jaki tekst wywołania, jaka linia? Następnie zbuduj Release i sprawdź, że komunikat zniknął. Usuń linię.
2. **Wina przypisana komu innemu.** Dopisz to samo błędne wywołanie, ale **bez** makra: `glEnable(GL_COLOR_BUFFER_BIT);`, na końcu `NightMazeApp::onRender`, po rysowaniu sceny. Zbuduj Debug i sprawdź, które wywołanie i która linia pojawiają się w komunikacie. Wyjaśnij, dlaczego log wskazuje poprawną linię kodu jako winną. Usuń linię.
3. **Rozwinięcie makra.** Napisz na kartce, do czego preprocesor rozwinie `GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));` w Debug i w Release. Porównaj z wynikiem `clang++ -E` (opcja `-E` kończy pracę po preprocesorze): ścieżki nagłówków i makra dla `NightMazeApp.cpp` weź z `build/debug/compile_commands.json`.

## 9. Pytania kontrolne

1. **Jak działa `GL_CHECK` i dlaczego to makro, a nie funkcja?**
   Wykonuje wywołanie, a w Debug woła `checkGlErrors(#call, __FILE__, __LINE__)`. Tylko makro potrafi zamienić kod na napis (`#call`) i podać plik oraz linię miejsca użycia. `do { } while (false)` robi z tego jedną instrukcję. W Release (`NDEBUG`) zostaje samo wywołanie.

2. **Dlaczego `glGetError` jest wołane w pętli?**
   OpenGL może trzymać kilka flag błędów, a jedno `glGetError` zwraca i kasuje tylko jedną. Pętla do `GL_NO_ERROR` czyści wszystkie, żeby błąd nie został przypisany późniejszemu wywołaniu.

3. **Dlaczego ta pętla ma limit `MAX_ERRORS_PER_CHECK`?**
   Bo przy utraconym albo niebieżącym kontekście `glGetError` może zwracać błąd bez końca i pętla bez limitu zawiesiłaby program. Liczę odczytane błędy w `errorCount`, po 16 przerywam i wypisuję jedną linię z informacją, że limit został osiągnięty. W zdrowym kontekście limit nie ma znaczenia, bo rodzajów błędów jest pięć.

4. **Dlaczego nie używam `glDebugMessageCallback`?**
   To funkcja z OpenGL 4.3, a projekt celuje w 4.1 Core, najwyższą wersję na macOS. W nagłówku GLAD wygenerowanym dla 4.1 tej funkcji nie ma.

5. **Czym różni się `GL_CHECK` w Debug i w Release i skąd program wie, która to konfiguracja?**
   W Debug po wywołaniu sprawdza `glGetError` i loguje błędy, w Release zostaje samo wywołanie. Decyduje `#ifndef NDEBUG`: CMake definiuje `NDEBUG` w konfiguracji Release.

6. **Po co `do { ... } while (false)`?**
   Żeby makro złożone z dwóch instrukcji zachowywało się jak jedna i wymagało średnika. Dzięki temu jest bezpieczne w `if` bez klamer, także z `else`.

7. **Log wskazuje błąd przy poprawnym `glViewport`. Co się stało?**
   Flagę zostawiło wcześniejsze wywołanie bez `GL_CHECK` (własne albo z biblioteki). Flaga czeka w kontekście do pierwszego odczytu, a pierwszym odczytem był `GL_CHECK` przy `glViewport`.

8. **Jak opakować wywołanie, które zwraca wartość?**
   Z przypisaniem w środku: `GL_CHECK(id = glCreateShader(GL_VERTEX_SHADER));`, a zmienną `id` deklaruję przed makrem.

## 10. Źródła

- docs.gl (<https://docs.gl>), strona funkcji `glGetError` dla OpenGL 4.
- LearnOpenGL, rozdział "Debugging" (<https://learnopengl.com/In-Practice/Debugging>): `glGetError`, makro z `__FILE__` i `__LINE__`, debug output dostępny od 4.3.
- Dokument biblioteki w tym repozytorium: [`../../libraries/glad.md`](../../libraries/glad.md) (dlaczego funkcji z 4.2+ nie ma w nagłówku).
- Przewodnik budowania: [`../../guides/build-macos.md`](../../guides/build-macos.md), sekcja 5 (flagi Debug i Release).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o pierwszym programie).
- "OpenGL. Księga eksperta" (rozdziały wprowadzające: maszyna stanów, obsługa błędów).
