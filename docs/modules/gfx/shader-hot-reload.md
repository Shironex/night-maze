# Moduł gfx: wczytywanie shaderów na żywo

Kamień milowy: M1. Temat wykładu: 2 (Programowalny potok).
Kod: funkcja `reload` w [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) i [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp), panel w [`src/debug/panels/ShadersPanel.hpp`](../../../src/debug/panels/ShadersPanel.hpp) i [`src/debug/panels/ShadersPanel.cpp`](../../../src/debug/panels/ShadersPanel.cpp), shadery [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert) i [`assets/shaders/basic.frag`](../../../assets/shaders/basic.frag), użycie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`src/main.cpp`](../../../src/main.cpp).

Część modułu `gfx`. Wstęp do całego modułu jest w [`README.md`](README.md). Ten dokument jest dalszym ciągiem [`shaders.md`](shaders.md) (potok, GLSL, dwa shadery projektu) i [`shader-class.md`](shader-class.md) (kompilacja, linkowanie i reszta klasy `gfx::Shader`). Tutaj jest podmiana programu w działającej aplikacji i panel "Shaders". Trzecia część tematu, uniformy, jest w [`uniforms.md`](uniforms.md). Jak nakładka z panelami jest wpięta w program, opisuje [`../debug-ui.md`](../debug-ui.md), a skąd program bierze pliki z `assets/`, [`../core/paths.md`](../core/paths.md).

## 1. Po co to jest

Shader pisze się metodą prób: zmieniam jedną linię i patrzę na obraz. Gdyby każda próba wymagała zamknięcia programu i uruchomienia go od nowa, nauka GLSL trwałaby kilka razy dłużej. Shadery projektu są plikami na dysku, czytanymi w czasie działania (zasada z PRD: "Shadery jako pliki"), więc program może wczytać je ponownie bez kompilacji C++ i bez zamykania okna.

Odpowiadają za to dwa elementy:

| Element | Co daje |
|---|---|
| `Shader::reload()` | buduje nowy program z tych samych dwóch plików i podmienia stary tylko przy sukcesie. Literówka w shaderze nie zamienia obrazu w czarny ekran, bo stary program działa dalej |
| panel **Shaders** z przyciskiem "Reload shaders" | woła `reload()` na żądanie i pokazuje wynik: nazwy plików, stan programu i tekst ostatniego błędu sterownika |

Program nie obserwuje dysku: przeładowanie następuje wtedy, gdy nacisnę przycisk. To jest też pokaz tematu 2 na obronie (sekcja 6.4).

## 2. Teoria

**Wczytywanie na żywo** (hot reload) to podmiana shadera w działającym programie: zmieniam plik `.frag` w edytorze, zapisuję, każę programowi wczytać shadery ponownie i od następnej klatki widzę efekt, bez zamykania okna i bez kompilacji C++. Przy nauce shaderów to największe przyspieszenie pracy, jakie można mieć.

Żeby to było bezpieczne, podmiana musi być **wszystko albo nic**. Shader w trakcie edycji bardzo często się nie kompiluje. Gdybym najpierw usunął stary program, a potem próbował zbudować nowy, każda literówka kończyłaby się czarnym ekranem. Dlatego kolejność jest odwrotna: najpierw buduję nowy program obok starego, a stary usuwam dopiero wtedy, gdy nowy na pewno działa.

```mermaid
flowchart TD
    Start["reload()"] --> Build["buildProgram: wczytaj oba pliki, skompiluj, zlinkuj<br/>NOWY program, stary nietknięty"]
    Build --> Ok{"udało się?"}
    Ok -- nie --> Keep["zapisz błąd w m_lastError, logError<br/>m_program bez zmian, zwróć false"]
    Ok -- tak --> Swap["glDeleteProgram(stary), m_program = nowy<br/>wyczyść m_lastError, zwróć true"]
```

## 3. Jak to działa w OpenGL

Przeładowanie nie ma własnych funkcji OpenGL. `reload()` wykonuje ten sam ciąg wywołań co pierwsze wczytanie, od `glCreateShader` do `glDeleteShader` ([`shader-class.md`](shader-class.md), sekcja 3.1, kroki od 1 do 13), a potem usuwa stary program przez `glDeleteProgram`. O tym, że podmiana jest bezpieczna, decydują cztery własności:

| Wywołanie | Własność, na której polega przeładowanie |
|---|---|
| `glDeleteProgram(0)` | jest ignorowane bez błędu, więc pierwsze wczytanie (bez starego programu) nie wymaga osobnej gałęzi |
| `glDeleteProgram(program)` dla programu bieżącego | nie usuwa go od razu, tylko oznacza do usunięcia. Program znika, gdy przestanie być bieżący (pułapka 1) |
| `glUseProgram(program)` | wybiera program dla następnych wywołań rysujących. Gra woła je co klatkę, więc nowy identyfikator zaczyna działać od następnej klatki |
| `glIsProgram(program)` | mówi, czy identyfikator jest nazwą istniejącego programu. Mój kod tej funkcji nie woła, używa jej backend ImGui (sekcja 6.3) |

Błąd kompilacji albo linkowania nowego programu nie jest błędem OpenGL: `glGetError` go nie zgłosi, a jedyną informacją jest status i dziennik sterownika ([`shader-class.md`](shader-class.md), sekcja 3.3).

## 4. Shadery

Ten temat nie ma własnych shaderów. Przeładowywana jest jedyna para projektu, `basic.vert` i `basic.frag`, opisana linia po linii w [`shaders.md`](shaders.md) (sekcja 4). Pokaz na obronie (sekcja 6.4) zmienia jedną linię w `basic.frag`.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp), [`.cpp`](../../../src/gfx/Shader.cpp) | funkcja `reload` klasy `gfx::Shader` (sekcja 5.2) oraz `isValid` i `lastError`, którymi panel czyta jej wynik. Reszta klasy: [`shader-class.md`](shader-class.md), sekcja 5 |
| [`src/debug/panels/ShadersPanel.hpp`](../../../src/debug/panels/ShadersPanel.hpp), [`.cpp`](../../../src/debug/panels/ShadersPanel.cpp) | funkcja `debug::drawShadersPanel`: panel "Shaders" z przyciskiem "Reload shaders" (sekcja 6.1). Należy do programu `night_maze`, nie do biblioteki `engine` |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) | chroniony akcesor `shader()`, przez który obiekt trafia do panelu (sekcja 6.2) |
| [`src/debug/DebugContext.hpp`](../../../src/debug/DebugContext.hpp), [`src/main.cpp`](../../../src/main.cpp) | pole `shader` struktury `DebugContext` i linia, która je wypełnia (sekcja 6.2) |

`reload()` wołają dwa miejsca: konstruktor `Shader` przy pierwszym wczytaniu ([`shader-class.md`](shader-class.md), sekcja 5.8) i panel "Shaders" po naciśnięciu przycisku.

### 5.2 `Shader::reload`: nowy program obok starego

Funkcja pomocnicza `buildProgram`, która wczytuje oba pliki, kompiluje je i linkuje, jest opisana w [`shader-class.md`](shader-class.md) (sekcja 5.7). Zwraca identyfikator nowego programu albo 0 z komunikatem w `error` i przy żadnym wyjściu nie zostawia po sobie obiektów OpenGL. `reload()` dokłada do niej decyzję, co zrobić ze starym programem:

```cpp
bool Shader::reload() {
    // Build the new program completely before touching the one in use.
    std::string error;
    const GLuint program = buildProgram(m_vertexPath, m_fragmentPath, error);
    if (program == 0) {
        // m_program is not changed: the previous program, if there is one, keeps working.
        m_lastError = error;
        core::logError(m_lastError);
        return false;
    }

    // Only now replace the old program (glDeleteProgram(0) is ignored on the first load).
    GL_CHECK(glDeleteProgram(m_program));
    m_program = program;
    m_lastError.clear();
    return true;
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `const GLuint program = buildProgram(...)` | Nowy program powstaje w zmiennej **lokalnej**. `m_program` do tej chwili nie zostało dotknięte |
| `if (program == 0)` | Niepowodzenie na którymkolwiek etapie. Wszystkie nowe obiekty zostały już usunięte przez funkcje pomocnicze |
| `m_lastError = error;` i `core::logError(m_lastError);` | Ten sam tekst idzie w dwa miejsca: do pola (dla panelu debug) i do konsoli jako linia `[error] ...` |
| `return false;` bez zmiany `m_program` | **Stary program działa dalej.** Po nieudanym `reload()` obiekt jest w tym samym stanie co przedtem, tylko z ustawionym `lastError()` |
| `GL_CHECK(glDeleteProgram(m_program));` | Dopiero po sukcesie usuwam stary program. Przy pierwszym wczytaniu `m_program` to 0 i OpenGL takie wywołanie ignoruje |
| `m_lastError.clear();` | Po udanym wczytaniu nie ma błędu do pokazania |

Możliwe stany obiektu:

| Sytuacja | `isValid()` | `lastError()` |
|---|---|---|
| konstruktor, pliki poprawne | `true` | pusty |
| konstruktor, błąd w pliku | `false` | komunikat |
| udany `reload()` | `true` | pusty |
| nieudany `reload()` po wcześniejszym sukcesie | `true` (stary program) | komunikat |
| obiekt, z którego przeniesiono | `false` | nieokreślony (zwykle pusty) |

Czwarty wiersz jest wart zapamiętania: `isValid()` i pusty `lastError()` to **dwa różne pytania**. Pierwsze mówi "czy jest czym rysować", drugie "czy ostatnie wczytanie się udało".

### 5.3 Jak to zostało sprawdzone

Samą funkcję `reload()` sprawdził test klasy opisany w [`shader-class.md`](shader-class.md) (sekcja 5.10): po zepsuciu pliku zwraca `false`, a bieżący program ma ten sam identyfikator co przed próbą. Po naprawieniu pliku zwraca `true`, a program dostaje nowy identyfikator.

**Panel Shaders (wersja z trójkątem).** Przycisku nie da się kliknąć z automatu w prawdziwym programie, więc ścieżkę kodu panelu sprawdziłem na Macu osobnym programem testowym poza repozytorium, gdy program rysował jeszcze trójkąt bez macierzy. Kod panelu i `Shader::reload` od tamtej pory się nie zmienił. Ukryte okno GLFW, ImGui zainicjalizowane tymi samymi wywołaniami co w `DebugUI`, prawdziwe `drawShadersPanel` z `ShadersPanel.cpp`, a w każdej klatce ta sama kolejność co w ówczesnym programie: `use()`, `bind()`, `glDrawArrays`, potem klatka ImGui z panelem i `RenderDrawData`. Kliknięcie było wstrzyknięte do ImGui jako zdarzenia myszy (`ImGuiIO::AddMousePosEvent`, `AddMouseButtonEvent`), a pliki shaderów były kopią w katalogu tymczasowym.

| Próba | Wynik |
|---|---|
| zapisanie zmienionego `basic.frag` bez kliknięcia | ten sam identyfikator programu, obraz bez zmian |
| kolor zmieniony na `vec4(1.0, 0.5, 0.0, 1.0)`, kliknięcie | nowy identyfikator programu po `use()`, `glIsProgram(stary)` fałsz, `lastError()` pusty, piksel wewnątrz trójkąta z `glReadPixels`: (255, 128, 0) zamiast (64, 128, 64) |
| usunięty średnik w tej linii, kliknięcie | ten sam identyfikator programu co przed kliknięciem, `isValid()` prawda, `lastError()` z tekstem jak niżej, piksel nadal pomarańczowy, panel rysuje więcej tekstu (1200 wierzchołków zamiast 356) |
| plik przywrócony, kliknięcie | nowy identyfikator, poprzedni usunięty, `lastError()` pusty, kolory interpolowane wróciły, panel rysuje tyle tekstu co na początku |
| `glGetError` po scenie i po `RenderDrawData`, w każdej klatce testu | `GL_NO_ERROR` |

```text
[error] Shader compilation failed: <katalog testu>/shaders/basic.frag
ERROR: 0:15: '}' : syntax error: syntax error
```

Test nie obejmuje `DebugUI::draw` ani `main.cpp` (te sprawdza kompilacja i uruchomienie programu: start bez linii `[error]`), nie sprawdza wyglądu panelu (kolor tekstu błędu, zawijanie, podpowiedź z pełną ścieżką) i nie zastępuje kliknięcia prawdziwą myszą w prawdziwym oknie. To zostaje do sprawdzenia ręcznego (sekcja 6.4).

## 6. Panel ImGui

Panel **Shaders** (kod: [`ShadersPanel.cpp`](../../../src/debug/panels/ShadersPanel.cpp)) jest pokazem tematu 2 na obronie: przycisk "Reload shaders" wczytuje shadery ponownie w działającym programie. Panel pokazuje jeden program, ten z pola `NightMazeApp::m_shader`. Jak nakładka z panelami jest wpięta w program, opisuje [`../debug-ui.md`](../debug-ui.md).

| Element | Rodzaj | Skąd wartość | Czego uczy |
|---|---|---|---|
| `Vertex: basic.vert`, `Fragment: basic.frag` | odczyt | `vertexPath()` i `fragmentPath()`, zamienione na tekst przez `core::pathText` | Program powstaje z dwóch plików, po jednym na etap. Po najechaniu kursorem na linię pojawia się podpowiedź (tooltip) z pełną ścieżką: widać w niej, że program czyta pliki z katalogu `assets` obok pliku wykonywalnego |
| `Program: valid` albo `Program: not valid` | odczyt | `isValid()` | Czy jest zlinkowany program, którym można rysować. Po nieudanym przeładowaniu zostaje `valid`, bo działa poprzedni program |
| `Reload shaders` | przycisk | woła `reload()` | Wczytywanie na żywo (sekcja 2): pliki są czytane, kompilowane i linkowane od nowa, bez zamykania okna i bez kompilacji C++ |
| `Last load: OK` albo `Last load: failed` i czerwony tekst pod spodem | odczyt | `lastError()` | Błąd kompilacji GLSL nie jest błędem OpenGL ([`shader-class.md`](shader-class.md), sekcja 3.3): jedyną informacją jest tekst sterownika, który klasa zapamiętała. Ten sam tekst jest w konsoli jako linia `[error]` |

Dwie ostatnie linie odpowiadają na dwa różne pytania (tabela stanów w sekcji 5.2). `Program: valid` razem z czerwonym błędem to nie sprzeczność, tylko dokładnie ten stan, dla którego `reload()` zostało tak napisane: nowe pliki się nie kompilują, a obraz rysuje poprzedni program.

### 6.1 Kod panelu

Cała funkcja z [`ShadersPanel.cpp`](../../../src/debug/panels/ShadersPanel.cpp) i stała nad nią:

```cpp
// Text color of a failed load (red, green, blue, alpha): a light red that stands out from
// the white text of the rest of the panel.
constexpr ImVec4 ERROR_TEXT_COLOR{1.0F, 0.4F, 0.4F, 1.0F};
```

```cpp
void drawShadersPanel(gfx::Shader& shader) {
    if (ImGui::Begin("Shaders")) {
        // The label shows only the file name. The full path appears as a tooltip when the
        // mouse rests on the line. ImGui expects UTF-8, which core::pathText returns.
        const std::string vertexFile = core::pathText(shader.vertexPath().filename());
        const std::string vertexFullPath = core::pathText(shader.vertexPath());
        ImGui::Text("Vertex: %s", vertexFile.c_str());
        ImGui::SetItemTooltip("%s", vertexFullPath.c_str());

        const std::string fragmentFile = core::pathText(shader.fragmentPath().filename());
        const std::string fragmentFullPath = core::pathText(shader.fragmentPath());
        ImGui::Text("Fragment: %s", fragmentFile.c_str());
        ImGui::SetItemTooltip("%s", fragmentFullPath.c_str());

        // Valid means that there is a linked program to draw with. After a failed reload
        // it is still the previous program.
        ImGui::Text("Program: %s", shader.isValid() ? "valid" : "not valid");

        ImGui::Separator();
        // Button returns true only in the frame in which it was clicked. The result of
        // reload() is not needed here: the lines below read it from lastError().
        if (ImGui::Button("Reload shaders")) {
            shader.reload();
        }

        if (shader.lastError().empty()) {
            ImGui::TextUnformatted("Last load: OK");
        } else {
            ImGui::TextUnformatted("Last load: failed");
            // The message contains text written by the driver, so it goes in as an
            // argument of "%s" and never as the format string itself.
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
            ImGui::TextWrapped("%s", shader.lastError().c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `gfx::Shader& shader` bez `const` | Panel woła `reload()`, które zmienia obiekt. Z samej sygnatury widać, że panel nie tylko czyta ([`../debug-ui.md`](../debug-ui.md), sekcja 5.2, decyzja 3) |
| `shader.vertexPath().filename()` | `filename()` zwraca ostatni element ścieżki jako nowy obiekt `path`: z `<repo>/build/debug/assets/shaders/basic.vert` zostaje `basic.vert` |
| `core::pathText(...)` | Zamiana `path` na tekst w UTF-8 ([`../core/paths.md`](../core/paths.md), sekcja 5.7). Panel nie woła `path::string()`, które na Windowsie potrafi rzucić wyjątek |
| `const std::string vertexFile = ...` | Wynik `pathText` zapisuję w nazwanej zmiennej, żeby linia z `ImGui::Text` była krótka i czytelna. `%s` chce napisu C, stąd `.c_str()` |
| `ImGui::SetItemTooltip("%s", ...)` | Dotyczy **poprzedniego** widżetu, czyli linii z nazwą pliku. Podpowiedź pojawia się, gdy kursor chwilę nad nią stoi |
| `shader.isValid() ? "valid" : "not valid"` | Operator warunkowy wybiera jeden z dwóch literałów. Oba są stałymi napisami C, więc pasują do `%s` |
| `if (ImGui::Button("Reload shaders")) { shader.reload(); }` | Tryb natychmiastowy: `Button` rysuje przycisk i zwraca `true` tylko w tej klatce, w której został kliknięty. Nie ma callbacka ani zdarzenia ([`../../libraries/imgui.md`](../../libraries/imgui.md)) |
| wynik `reload()` jest ignorowany | `reload()` zwraca `bool`, ale panel go nie potrzebuje: te same informacje są w `lastError()` i `isValid()`, które linie niżej czytają już po przeładowaniu, jeszcze w tej samej klatce |
| `ImGui::TextUnformatted("Last load: OK")` | Stały tekst bez znaczników `%`. `TextUnformatted` wypisuje napis dokładnie tak, jak go dostał |
| `PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR)` i `PopStyleColor()` | Zmiana koloru tekstu dla widżetów między tymi dwiema liniami. Każde `Push` musi mieć swoje `Pop`, inaczej kolor zostałby na resztę klatki, a ImGui zgłasza niedopasowanie jako błąd. Kolor jest nazwaną stałą, a nie czterema liczbami w środku wywołania |
| `ImGui::TextWrapped("%s", shader.lastError().c_str())` | Komunikat ma kilka linii i długą ścieżkę, więc jest zawijany do szerokości panelu. `"%s"` jest tu konieczne (niżej) |

**Dlaczego `"%s"`, a nie sam napis.** `ImGui::Text` i `ImGui::TextWrapped` działają jak `printf`: pierwszy argument to **napis formatujący**, w którym znak `%` rozpoczyna znacznik. Tekst błędu pochodzi od sterownika karty i może zawierać znak `%` (na przykład w nazwie albo w komunikacie). Podany jako napis formatujący kazałby funkcji czytać argumenty, których nie ma, co jest niezdefiniowanym zachowaniem. Podany jako argument dla `"%s"` jest tylko kopiowany. Kompilator też tego pilnuje: `ImGui::TextWrapped(shader.lastError().c_str())` daje w clang ostrzeżenie `format string is not a string literal (potentially insecure)`.

Panel trzyma się zasad wszystkich paneli ([`../debug-ui.md`](../debug-ui.md), sekcja 5.5): jest wolną funkcją bez stanu, nie ma zmiennych globalnych ani `static`, i sam nie woła żadnej funkcji `gl*`. Wywołania OpenGL wykonuje `Shader::reload`, panel tylko o nie prosi.

### 6.2 Jak shader trafia do panelu

`debug/` nie zna `game/`, więc panel nie sięga po `m_shader` sam. Referencja idzie tą samą drogą co kolor tła ([`../debug-ui.md`](../debug-ui.md), sekcja 5.5, krok 5):

```mermaid
flowchart LR
    Field["NightMazeApp::m_shader<br/>pole prywatne"] --> Acc["NightMazeApp::shader()<br/>chroniony akcesor"]
    Acc --> Ctx["DebugContext::shader<br/>pole gfx::Shader&"]
    Ctx --> Draw["DebugUI::draw<br/>drawShadersPanel(context.shader)"]
    Draw --> Panel["drawShadersPanel(gfx::Shader& shader)"]
    Panel -->|"przycisk"| Reload["Shader::reload()"]
```

Akcesor w [`NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp):

```cpp
/// Shader program of the cube, exposed so the debug UI can reload it live.
gfx::Shader& shader() { return m_shader; }
```

Pole w [`DebugContext.hpp`](../../../src/debug/DebugContext.hpp):

```cpp
/// Shader program the game draws with, editable: the Shaders panel reloads it.
gfx::Shader& shader;
```

Linia w `DebugNightMazeApp::onRender` w [`main.cpp`](../../../src/main.cpp) i wywołanie w `DebugUI::draw`:

```cpp
.shader = shader(),
```

```cpp
drawShadersPanel(context.shader);
```

Gra nadal nie dołącza niczego z `debug/`: udostępnia chroniony akcesor i nie wie, kto z niego skorzysta. `DebugContext.hpp` i `ShadersPanel.hpp` nie dołączają `gfx/Shader.hpp`, wystarcza im deklaracja wyprzedzająca `class Shader;`, bo używają typu tylko przez referencję. Pełny nagłówek dołącza `ShadersPanel.cpp`, które woła funkcje klasy.

### 6.3 Przeładowanie w środku klatki ImGui

Przycisk jest widżetem, więc `reload()` wykonuje się **wewnątrz** klatki ImGui: po `ImGui::NewFrame()`, a przed `ImGui::Render()` i `ImGui_ImplOpenGL3_RenderDrawData(...)`. To wywołania OpenGL w miejscu, w którym reszta kodu paneli żadnych nie robi. Sprawdziłem w źródle backendu (`imgui_impl_opengl3.cpp`, ImGui 1.92.9b), że nie przeszkadza to ani ImGui, ani grze:

```mermaid
sequenceDiagram
    participant Game as NightMazeApp::onRender
    participant Panel as drawShadersPanel
    participant GL as OpenGL
    participant Backend as backend ImGui
    Game->>GL: glUseProgram(stary), glUniformMatrix4fv x3, glDrawElements
    Note over Panel: klatka ImGui, kliknięty przycisk
    Panel->>GL: reload() buduje nowy program
    Panel->>GL: glDeleteProgram(stary)
    Note over GL: stary jest bieżący, więc tylko oznaczony do usunięcia
    Backend->>GL: RenderDrawData zapamiętuje GL_CURRENT_PROGRAM (stary)
    Backend->>GL: glUseProgram(program ImGui)
    Note over GL: stary przestał być bieżący i znika naprawdę
    Backend->>GL: glIsProgram(stary) zwraca fałsz, backend go nie przywraca
    Note over Game: następna klatka
    Game->>GL: glUseProgram(nowy), glUniformMatrix4fv x3, glDrawElements
```

1. **Między `NewFrame` a `Render` ImGui nie woła OpenGL.** Widżety tylko dopisują geometrię do list w pamięci. `ImGui_ImplOpenGL3_NewFrame()` wykonało się wcześniej, a rysowanie następuje dopiero w `RenderDrawData`. `reload()` nie trafia więc w środek żadnej operacji backendu.
2. **`reload()` nie zmienia stanu, na którym polega backend.** Tworzy obiekty shaderów i program, kompiluje, linkuje i usuwa ([`shader-class.md`](shader-class.md), sekcja 3.1). Nie woła `glUseProgram`, nie wiąże buforów, tekstur ani VAO.
3. **Backend ustawia własny stan od zera.** `RenderDrawData` zapamiętuje bieżący stan, potem samo woła `glUseProgram` dla swojego programu, wiąże swoje VAO i bufory. Nie zakłada, że ktoś zostawił mu poprawny program.
4. **Backend jest przygotowany na usunięty program.** Po udanym przeładowaniu stary program jest jeszcze bieżący (gra ustawiła go w tej klatce), więc `glDeleteProgram` tylko oznacza go do usunięcia (sekcja 7, pułapka 1). Backend zapamiętuje go jako "poprzedni program", przełącza się na własny i w tej chwili stary program znika naprawdę. Na końcu backend przywraca poprzedni program tylko wtedy, gdy ten jeszcze istnieje. W źródle jest to linia `if (last_program == 0 || glIsProgram(last_program)) glUseProgram(last_program);` z komentarzem, że bez tego sprawdzenia przywrócenie programu oczekującego na usunięcie dałoby błąd OpenGL.
5. **Który program jest bieżący po klatce.** Zależy to od tego, czy program gry przetrwał klatkę. W źródle backendu `RenderDrawData` zapamiętuje `GL_CURRENT_PROGRAM`, w `ImGui_ImplOpenGL3_SetupRenderState` woła `glUseProgram` dla własnego programu (zawsze, także gdy nie ma żadnego panelu do narysowania), a na końcu wykonuje linię z punktu 4. Wynikają z tego trzy przypadki:

   | Klatka | Co robi backend na końcu | Bieżący program po klatce |
   |---|---|---|
   | zwykła (bez przeładowania albo z nieudanym) | zapamiętany program gry istnieje, więc `glIsProgram` zwraca prawdę i backend go **przywraca** | program gry, ten sam co przed rysowaniem paneli |
   | z udanym przeładowaniem | zapamiętany stary program został usunięty w chwili przełączenia na program ImGui, `glIsProgram` zwraca fałsz, backend **niczego nie przywraca** | program ImGui |
   | okno o zerowym rozmiarze (zminimalizowane) | `RenderDrawData` wraca na samym początku, zanim cokolwiek zapamięta albo ustawi | bez zmian: backend w takiej klatce nie dotyka stanu OpenGL |

   Żaden z tych przypadków nie szkodzi grze, bo gra nie polega na tym, co zostało po poprzedniej klatce. W następnej klatce `NightMazeApp::onRender` woła `m_shader.use()` przed ustawieniem macierzy i przed `glDrawElements`, a `use()` i `setMat4()` czytają aktualne `m_program`, czyli już nowy identyfikator. Nowy program zaczyna z wyzerowanymi uniformami, ale trzy macierze są wysyłane co klatkę, więc dostaje je przed pierwszym rysowaniem. Nikt poza klasą `Shader` nie przechowuje identyfikatora programu.

Przy nieudanym przeładowaniu nic z tego nie zachodzi: `m_program` się nie zmienia, żaden używany program nie jest usuwany, a backend przywraca ten sam program co zwykle.

Punkty 4 i 5 potwierdził test z sekcji 5.3: po kliknięciu `glIsProgram` dla starego identyfikatora zwraca fałsz, a `glGetError` po żadnej klatce nie zgłasza błędu.

### 6.4 Pokaz na obronie krok po kroku

Wersja dla macOS, gdzie `build/debug/assets` jest dowiązaniem do katalogu w repozytorium. Różnica na Windowsie jest w sekcji 6.5.

1. Uruchom program (`make run`). W panelu Shaders: `Vertex: basic.vert`, `Fragment: basic.frag`, `Program: valid`, `Last load: OK`. Najedź kursorem na linię `Fragment`, żeby pokazać pełną ścieżkę.
2. Nie zamykając programu, otwórz w edytorze [`assets/shaders/basic.frag`](../../../assets/shaders/basic.frag) i zamień linię `fragColor = vec4(vColor, 1.0);` na `fragColor = vec4(1.0, 0.5, 0.0, 1.0);`. Zapisz plik. **Obraz się nie zmienia**: program nie obserwuje dysku.
3. Naciśnij `Reload shaders`. Cała kostka staje się jednolicie pomarańczowa: ściany przestają się od siebie różnić i zostaje sama sylwetka. W panelu zostaje `Last load: OK`. Kod C++ nie był kompilowany, okno nie było zamykane.
4. Wprowadź literówkę: usuń średnik na końcu zmienionej linii. Zapisz i naciśnij `Reload shaders`.
5. W panelu pojawia się `Last load: failed` i czerwony tekst: `Shader compilation failed: <ścieżka>/basic.frag`, a pod nim linia sterownika (na Macu `ERROR: 0:15: '}' : syntax error: syntax error`). Ten sam tekst jest w konsoli jako linia `[error]`. Linia `Program` nadal pokazuje `valid`, a **kostka jest nadal pomarańczowa**: rysuje ją poprzedni program.
6. Przywróć plik do pierwotnej postaci (`git checkout assets/shaders`), naciśnij `Reload shaders`. Wraca `Last load: OK` i kostka z trzema widocznymi ścianami: czerwoną, niebieską i turkusową.

Co przy tym mówię: krok 2 pokazuje, że shader jest plikiem czytanym w czasie działania (zasada "Shadery jako pliki"). Krok 3 to cały potok budowania programu z [`shader-class.md`](shader-class.md) (sekcja 3.1) wykonany na żądanie. Kostka nie znika ani na jedną klatkę, choć nowy program ma wyzerowane uniformy: macierze są wysyłane co klatkę ([`uniforms.md`](uniforms.md), sekcja 5.2). Krok 5 pokazuje dwie rzeczy naraz: że błąd GLSL trzeba odczytać samemu z dziennika sterownika ([`shader-class.md`](shader-class.md), sekcja 3.3) i że `reload()` jest operacją "wszystko albo nic" (sekcja 2). Numer linii 15 przy średniku brakującym w linii 14 tłumaczy pułapka 7 w [`shader-class.md`](shader-class.md).

### 6.5 Różnica na Windowsie

Na Windowsie katalog `assets` obok programu jest **kopią**, a nie dowiązaniem ([`../core/paths.md`](../core/paths.md), sekcja 5.8). Przycisk czyta kopię, więc po zapisaniu pliku w `assets\shaders\` trzeba najpierw ją odświeżyć:

1. zapisz plik shadera w repozytorium,
2. w drugim terminalu wykonaj `cmake --build --preset debug`,
3. naciśnij `Reload shaders`.

Oczekuję, że krok 2 da się wykonać przy działającym programie: gdy nie zmienił się żaden plik C++, budowanie nie linkuje `night_maze.exe` od nowa, tylko kopiuje katalog `assets`. Nie było to jeszcze sprawdzone na PC (punkt na liście kontrolnej w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 11).

Bez kroku 2 panel pokaże `Last load: OK`, a obraz się nie zmieni, bo program wczytał poprawnie stary plik. Podpowiedź z pełną ścieżką w panelu pokazuje, który plik jest czytany. Szczegóły i wariant dla Visual Studio: [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 7. Na Windowsie panel nie był jeszcze uruchamiany.

## 7. Pułapki

1. **Usunięcie programu, który jest w użyciu.** `glDeleteProgram` dla bieżącego programu nie usuwa go od razu, tylko oznacza do usunięcia. Program znika, gdy przestanie być bieżący. Po udanym `reload()` stary program jest więc jeszcze "bieżący" do najbliższego `glUseProgram` z innym programem. W programie `night_maze` jest nim rysowanie paneli przez backend ImGui jeszcze w tej samej klatce, a w następnej `use()` ustawia nowy program (sekcja 6.3). W pętli gry `use()` jest wołane co klatkę, więc niczego nie trzeba robić. Błędem byłoby zapamiętać identyfikator programu poza klasą i używać go po `reload()`.
2. **Stary obraz po zmianie pliku.** Zapisanie pliku shadera samo niczego nie zmienia w działającym programie: `Shader` nie obserwuje dysku. Trzeba zawołać `reload()`, czyli nacisnąć `Reload shaders` w panelu Shaders (albo uruchomić program ponownie).
3. **Windows: program czyta kopię shaderów.** Na macOS katalog `assets` obok programu jest dowiązaniem do katalogu w repozytorium, więc program widzi plik zaraz po zapisaniu. Na Windowsie jest to **kopia**, robiona od nowa przy każdym budowaniu: po zmianie pliku w `assets\shaders\` trzeba najpierw zbudować (`cmake --build --preset debug`), a dopiero potem nacisnąć `Reload shaders` ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 7). Objaw pominięcia budowania: panel pokazuje `Last load: OK`, a obraz się nie zmienia (sekcja 6.5).
4. **Tekst sterownika jako napis formatujący.** `ImGui::TextWrapped(shader.lastError().c_str())` traktuje komunikat jak format `printf`: znak `%` w tekście sterownika kazałby funkcji czytać nieistniejące argumenty. Poprawnie: `ImGui::TextWrapped("%s", shader.lastError().c_str())` albo `ImGui::TextUnformatted` (sekcja 6.1).
5. **`Program: valid` i czerwony błąd jednocześnie.** To nie jest błąd panelu. `isValid()` mówi, czy jest czym rysować, a `lastError()`, czy ostatnie wczytanie się udało. Po nieudanym przeładowaniu oba są prawdziwe naraz: rysuje poprzedni program (sekcja 5.2).
6. **`Last load: OK`, a obraz bez zmian.** Program wczytał poprawnie plik, tylko nie ten, który przed chwilą zmieniłem. Na Windowsie to nieodświeżona kopia `assets` (pułapka 3). Na obu systemach: zmiana zapisana w innym pliku niż ten z podpowiedzi w panelu albo niezapisany plik w edytorze.
7. **Błąd tylko jednego pliku naraz.** Gdy zepsute są oba pliki, panel pokazuje błąd shadera wierzchołków, bo `buildProgram` kończy pracę na pierwszym niepowodzeniu ([`shader-class.md`](shader-class.md), sekcja 5.7). Błąd shadera fragmentów pojawi się po naprawieniu pierwszego i kolejnym kliknięciu.

Pułapki dotyczące kompilacji, linkowania i samej klasy `Shader` są w [`shader-class.md`](shader-class.md) (sekcja 7), a dotyczące uniformów po przeładowaniu w [`uniforms.md`](uniforms.md) (sekcja 7).

## 8. Ćwiczenia

Zasady pracy są takie same jak w [`shaders.md`](shaders.md) (sekcja 8): program działa przez cały czas, a po każdym ćwiczeniu przywróć pliki (`git checkout assets/shaders`) i naciśnij `Reload shaders` jeszcze raz. Na Windowsie przed naciśnięciem przycisku wykonaj `cmake --build --preset debug` (sekcja 6.5).

1. **Literówka w działającym programie.** Przy działającym programie usuń średnik po `fragColor = vec4(vColor, 1.0)` w `basic.frag` i naciśnij `Reload shaders`. Porównaj z ćwiczeniem 1 z [`shader-class.md`](shader-class.md) (literówka przy starcie): co pokazuje linia `Program`, co widać w oknie, ile linii `[error]` jest w konsoli po trzech kliknięciach? Wskaż w `Shader::reload` linię, przez którą kostka nie zniknęła.
2. **Brak pliku w działającym programie.** Przy działającym programie zmień nazwę pliku `assets/shaders/basic.frag` na `basic2.frag` i naciśnij `Reload shaders`. Jaki komunikat pokazuje panel i czym różni się od błędu kompilacji? Przywróć nazwę i naciśnij przycisk ponownie.
3. **Napis formatujący.** W `ShadersPanel.cpp` zamień tymczasowo `ImGui::TextWrapped("%s", shader.lastError().c_str());` na `ImGui::TextWrapped(shader.lastError().c_str());` i zbuduj. Przeczytaj ostrzeżenie kompilatora. Wyjaśnij, co by się stało, gdyby komunikat sterownika zawierał `%d`. Wycofaj zmianę.
4. **Droga referencji.** Bez zaglądania do sekcji 6.2 wypisz pliki, przez które referencja do `m_shader` przechodzi od pola w `NightMazeApp` do wywołania `shader.reload()` w panelu. Dla każdego pliku podaj, czy dołącza `gfx/Shader.hpp`, czy wystarcza mu deklaracja wyprzedzająca, i dlaczego.
5. **Kolejność w klatce.** W `ShadersPanel.cpp` linie pokazujące `lastError()` stoją **pod** przyciskiem. Co pokazałby panel w klatce kliknięcia, gdyby stały nad nim? Czy użytkownik zauważyłby różnicę i dlaczego?

## 9. Pytania kontrolne

1. **Co robi `reload()` przy błędzie i dlaczego w takiej kolejności?**
   Buduje nowy program w zmiennej lokalnej. Przy błędzie zapisuje komunikat w `m_lastError`, loguje go i zwraca `false`, nie dotykając `m_program`, więc stary program działa dalej. Stary program jest usuwany dopiero po udanym zbudowaniu nowego. Dzięki temu literówka w shaderze nie daje czarnego ekranu.

2. **Kiedy wczytywany jest shader i co trzeba zrobić po zmianie pliku `.frag`?**
   Przy starcie, w konstruktorze `NightMazeApp` (konstruktor `Shader` woła `reload()`), i po każdym naciśnięciu `Reload shaders` w panelu Shaders. Po zmianie pliku zapisuję go i naciskam przycisk. Kompilacja C++ nie jest potrzebna, bo shader jest plikiem czytanym w czasie działania. Na Windowsie przed naciśnięciem trzeba zbudować, żeby odświeżyć kopię katalogu `assets`.

3. **Co pokazuje panel Shaders i skąd bierze każdą wartość?**
   Nazwy obu plików (`vertexPath()`, `fragmentPath()`, zamienione na tekst przez `core::pathText`, pełna ścieżka w podpowiedzi), stan programu (`isValid()`), przycisk wołający `reload()` oraz wynik ostatniego wczytania: `OK`, gdy `lastError()` jest pusty, albo jego tekst na czerwono. Panel nie ma własnego stanu: wszystko czyta co klatkę z obiektu `Shader`.

4. **Jak panel z `debug/` dostaje shader, który jest prywatnym polem gry?**
   `NightMazeApp` udostępnia chroniony akcesor `shader()`. `DebugNightMazeApp` w `main.cpp` wpisuje jego wynik do pola `shader` struktury `DebugContext`, a `DebugUI::draw` przekazuje `context.shader` do `drawShadersPanel`. Gra nie dołącza niczego z `debug/`, a `debug/` niczego z `game/`.

5. **Po nieudanym przeładowaniu panel pokazuje `Program: valid` i czerwony błąd. Czy to sprzeczność?**
   Nie. `isValid()` odpowiada na pytanie, czy jest program, którym można rysować, i jest nim poprzedni program. `lastError()` odpowiada na pytanie, czy ostatnie wczytanie się udało. `reload()` celowo nie dotyka `m_program` przy błędzie.

6. **`reload()` wykonuje się w środku klatki ImGui. Dlaczego to bezpieczne?**
   Między `NewFrame` a `Render` ImGui nie woła OpenGL, a `reload()` nie zmienia powiązań (program, VAO, bufory, tekstury). Backend w `RenderDrawData` sam ustawia swój program i stan. Stary program, usunięty jako bieżący, jest tylko oznaczony do usunięcia. Backend przy przywracaniu stanu sprawdza `glIsProgram` i nie przywraca programu, którego już nie ma. Gra w następnej klatce woła `use()` z nowym identyfikatorem.

7. **Dlaczego tekst błędu jest przekazywany jako argument `"%s"`?**
   Funkcje tekstowe ImGui traktują pierwszy argument jak format `printf`. Tekst pochodzi od sterownika i może zawierać `%`, co kazałoby funkcji czytać argumenty, których nie ma. Jako argument `"%s"` tekst jest tylko kopiowany.

8. **Jak wygląda przeładowanie shadera na Windowsie i dlaczego inaczej niż na macOS?**
   Program czyta tam kopię katalogu `assets` obok pliku `.exe`, a nie pliki z repozytorium. Po zapisaniu pliku trzeba więc wykonać `cmake --build --preset debug`, które odświeża kopię, i dopiero wtedy nacisnąć `Reload shaders`. Na macOS obok programu jest dowiązanie do katalogu w repozytorium, więc wystarcza sam przycisk.

## 10. Źródła

- LearnOpenGL, rozdział "Shaders" (<https://learnopengl.com/Getting-started/Shaders>): własna klasa shadera wczytująca pliki z dysku.
- docs.gl (<https://docs.gl>), strony dla OpenGL 4: `glDeleteProgram` (zdanie o ignorowaniu wartości 0 i o programie będącym w użyciu), `glUseProgram`, `glIsProgram`.
- Dear ImGui, plik `backends/imgui_impl_opengl3.cpp` (funkcja `ImGui_ImplOpenGL3_RenderDrawData`: zapamiętanie i przywrócenie stanu, sprawdzenie `glIsProgram`) oraz `imgui.h` (`Button`, `TextUnformatted`, `TextWrapped`, `PushStyleColor`, `SetItemTooltip`).
- Dokumenty w tym repozytorium: [`shaders.md`](shaders.md) (potok, GLSL), [`shader-class.md`](shader-class.md) (budowanie programu, odczyt błędów), [`uniforms.md`](uniforms.md) (uniformy po przeładowaniu), [`../debug-ui.md`](../debug-ui.md) (jak panele są wpięte w program), [`../core/paths.md`](../core/paths.md) (dowiązanie i kopia katalogu `assets`), [`../../libraries/imgui.md`](../../libraries/imgui.md), [`../../guides/build-windows.md`](../../guides/build-windows.md) (odświeżanie kopii shaderów).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): zasada "Shadery jako pliki".
