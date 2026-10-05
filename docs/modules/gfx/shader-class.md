# Moduł gfx: klasa Shader

Kamień milowy: M1, rozszerzona w M4 (dołączanie plików, nazwy plików w błędach, settery `setMat3` i `setFloat`, wiązanie bloków uniformów). Temat wykładu: 2 (Programowalny potok).
Kod: [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp), [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp), preprocesor w [`src/gfx/ShaderSource.hpp`](../../../src/gfx/ShaderSource.hpp), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp).

Część modułu `gfx`. Wstęp do całego modułu, zasada RAII dla obiektów OpenGL i semantyka przenoszenia są w [`README.md`](README.md). Ten dokument jest dalszym ciągiem [`shaders.md`](shaders.md): tam jest potok, język GLSL i shadery `basic.*`, tutaj kod klasy, która buduje z nich program OpenGL. Część funkcji klasy ma własne dokumenty: settery `setMat4`, `setInt`, `setVec3`, `setMat3` i `setFloat` są w [`uniforms.md`](uniforms.md), `bindUniformBlock` i funkcja pomocnicza `applyBlockBinding` w [`uniform-buffers.md`](uniform-buffers.md), dyrektywa `#include` i nazwy plików w błędach w [`shader-includes.md`](shader-includes.md), a `reload` i panel "Shaders" w [`shader-hot-reload.md`](shader-hot-reload.md). Ten dokument korzysta z makra `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)), z logowania ([`../core/window-context.md`](../core/window-context.md), sekcja 5.5) i ze ścieżek do assetów ([`../core/paths.md`](../core/paths.md)).

**Stan na dziś (M4).** Zmierzone na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74): build Debug i Release bez ostrzeżeń, clang-format i clang-tidy bez uwag, gra startuje bez linii `[error]` i bez linii `GL_`, czyli klasa buduje wszystkie pięć programów. Zmierzona jest też nowa postać błędu kompilacji z nazwą pliku w miejscu numeru (sekcja 5.10). Klasa wymaga kontekstu OpenGL i nie ma testów jednostkowych. Testy ma jej część wydzielona do `ShaderSource.*`: 22 przypadki ([`shader-includes.md`](shader-includes.md), sekcja 5.11). Przycisku `Reload shaders` z pięcioma programami nikt nie nacisnął ręcznie, a na macOS nic z M4 nie było budowane ani uruchamiane.

## 1. Po co to jest

W profilu Core nie da się narysować niczego bez programu shaderów ([`shaders.md`](shaders.md), sekcja 1). Ktoś musi więc wczytać tekst shadera z pliku, skompilować go, zlinkować dwa shadery w jeden program i powiedzieć, co poszło źle, gdy coś poszło źle.

Klasa `gfx::Shader` robi dokładnie to. Jest cienkim opakowaniem na **jeden obiekt programu OpenGL** zbudowany z dwóch plików: shadera wierzchołków i shadera fragmentów. Ma pięć cech, z których każda odpowiada na konkretny problem:

| Cecha | Problem, który rozwiązuje |
|---|---|
| Shadery są plikami na dysku, nie napisami w kodzie C++ | Zmiana shadera nie wymaga kompilacji programu, a edytor koloruje składnię GLSL (zasada z PRD: "Shadery jako pliki") |
| `reload()` buduje nowy program i podmienia stary tylko przy sukcesie | Wczytywanie na żywo (hot reload): literówka w shaderze nie zamienia obrazu w czarny ekran, bo stary program działa dalej |
| Błąd jest logowany z nazwą pliku i pełnym tekstem sterownika, a do tego zapamiętany w `lastError()`. Od M4 numer pliku na początku linii sterownika jest zamieniany na nazwę pliku | Błędów kompilacji GLSL nie widzi `glGetError` ani `GL_CHECK`. Bez własnego odczytu nie byłoby żadnej informacji. A odkąd shader może dołączać inne pliki, sam numer linii nie mówi już, w którym pliku jest pomyłka |
| Plik shadera może dołączać inne pliki linią `#include "..."` (od M4) | Kod wspólny dla kilku shaderów (oświetlenie) jest w jednym pliku, choć GLSL nie ma `#include` ([`shader-includes.md`](shader-includes.md)) |
| RAII i tylko przenoszenie (move-only) | Program OpenGL jest zwalniany dokładnie raz, automatycznie, bez ręcznego `glDeleteProgram` w kodzie gry |

## 2. Teoria

Klasa nie wprowadza własnej teorii. Pojęcia, na których stoi, są w dwóch miejscach: obiekt shadera a obiekt programu oraz kompilacja a linkowanie w [`shaders.md`](shaders.md) (sekcje 2.5 i 2.6), a RAII i semantyka przenoszenia w [`README.md`](README.md) (sekcja 2).

## 3. Jak to działa w OpenGL

### 3.1 Wywołania w kolejności

Zbudowanie jednego programu z dwóch plików to następujący ciąg wywołań. Kroki od 1 do 6 wykonują się dwa razy: raz dla shadera wierzchołków, raz dla shadera fragmentów.

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glCreateShader(GL_VERTEX_SHADER)` albo `glCreateShader(GL_FRAGMENT_SHADER)` | Tworzy pusty obiekt shadera danego typu i zwraca jego identyfikator. 0 oznacza niepowodzenie |
| 2 | `glShaderSource(shader, count, strings, lengths)` | Kopiuje tekst źródłowy do obiektu. `strings` to tablica `count` napisów C, które OpenGL skleja w jeden. `lengths` równe `nullptr` znaczy: każdy napis kończy się zerem |
| 3 | `glCompileShader(shader)` | Kompiluje tekst. Nic nie zwraca i **nie ustawia flagi błędu** przy błędzie w GLSL |
| 4 | `glGetShaderiv(shader, GL_COMPILE_STATUS, &status)` | Wpisuje do `status` wartość `GL_TRUE` albo `GL_FALSE`: wynik ostatniej kompilacji |
| 5 | `glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length)` | Wpisuje długość dziennika (info log) w znakach, **razem** z kończącym zerem. 0 oznacza brak dziennika |
| 6 | `glGetShaderInfoLog(shader, maxLength, &written, buffer)` | Kopiuje dziennik do bufora, najwyżej `maxLength` znaków. Do `written` wpisuje liczbę skopiowanych znaków **bez** kończącego zera |
| 7 | `glCreateProgram()` | Tworzy pusty obiekt programu i zwraca jego identyfikator. 0 oznacza niepowodzenie |
| 8 | `glAttachShader(program, shader)` | Dołącza obiekt shadera do programu. Wołane dwa razy, po jednym na etap |
| 9 | `glLinkProgram(program)` | Łączy dołączone shadery w kod wykonywalny dla karty. Tak jak kompilacja, przy błędzie nie ustawia flagi |
| 10 | `glDetachShader(program, shader)` | Odłącza obiekt shadera od programu. Zlinkowany program ma już własny kod i nie potrzebuje obiektów shaderów |
| 11 | `glGetProgramiv(program, GL_LINK_STATUS, &status)` | Wynik linkowania: `GL_TRUE` albo `GL_FALSE` |
| 12 | `glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length)` i `glGetProgramInfoLog(program, maxLength, &written, buffer)` | Dziennik linkowania. Działają jak kroki 5 i 6, ale dla obiektu programu |
| 13 | `glDeleteShader(shader)` | Usuwa obiekt shadera. Jeśli jest jeszcze dołączony do programu, OpenGL tylko oznacza go do usunięcia i zwalnia dopiero po odłączeniu |
| 14 | `glUseProgram(program)` | Ustawia program jako bieżący: używają go wszystkie następne wywołania rysujące, aż do kolejnego `glUseProgram` |
| 15 | `glDeleteProgram(program)` | Usuwa program. Dla wartości 0 nie robi nic i nie zgłasza błędu. Jeśli program jest akurat bieżący, zostaje oznaczony do usunięcia i znika, gdy przestanie być bieżący |

Ustawienie uniformu (`glGetUniformLocation` i `glUniformMatrix4fv`), wykonywane co klatkę po kroku 14, opisuje [`uniforms.md`](uniforms.md) (sekcja 3). Od M4 między krokiem 13 a podmianą programu `reload()` wykonuje dla programów z blokiem uniformów jeszcze trzy wywołania: `glGetUniformBlockIndex`, `glUniformBlockBinding` i `glGetActiveUniformBlockiv`. Opisuje je [`uniform-buffers.md`](uniform-buffers.md).

Tekst podawany w kroku 2 nie jest już dosłowną treścią pliku: przed `glShaderSource` linie `#include` są zastępowane treścią wskazanych plików, a wokół nich stają dyrektywy `#line` ([`shader-includes.md`](shader-includes.md), sekcja 2). To praca na tekście, bez żadnego wywołania OpenGL, więc w tabeli jej nie ma.

Wszystkie te funkcje są w rdzeniu OpenGL od wersji 2.0, więc są dostępne w 4.1 Core i w nagłówku GLAD projektu.

### 3.2 Diagram obiektów

```mermaid
sequenceDiagram
    participant Cpp as Shader.cpp
    participant GL as OpenGL (sterownik)
    Cpp->>GL: glCreateShader(GL_VERTEX_SHADER)
    Cpp->>GL: glShaderSource, glCompileShader
    Cpp->>GL: glGetShaderiv(GL_COMPILE_STATUS)
    Note over Cpp,GL: to samo dla GL_FRAGMENT_SHADER
    Cpp->>GL: glCreateProgram
    Cpp->>GL: glAttachShader x2, glLinkProgram
    Cpp->>GL: glDetachShader x2
    Cpp->>GL: glGetProgramiv(GL_LINK_STATUS)
    Cpp->>GL: glDeleteShader x2
    Note over Cpp,GL: zostaje jeden obiekt: program
    Cpp->>GL: glUseProgram (co klatkę, przed rysowaniem)
    Cpp->>GL: glGetUniformLocation, glUniformMatrix4fv (co klatkę, po glUseProgram)
    Cpp->>GL: glDeleteProgram (destruktor albo udany reload)
```

### 3.3 Błędy GLSL nie są błędami OpenGL

To najważniejsza rzecz w tej sekcji. `glGetError` (a więc i `GL_CHECK`) zgłasza **błędne użycie API**: złą stałą, zły identyfikator, wywołanie w złym stanie. Shader z błędem składni nie jest błędnym użyciem API: `glCompileShader` zostało wywołane poprawnie, na poprawnym obiekcie, i poprawnie wykonało swoją pracę, której wynikiem jest "ten tekst się nie kompiluje". Żadna flaga nie zostaje ustawiona.

| Sytuacja | `glGetError` | Gdzie jest informacja |
|---|---|---|
| `glCompileShader(12345)` (nie ma takiego obiektu) | `GL_INVALID_VALUE` | `GL_CHECK` wypisze linię w konsoli |
| Shader z brakującym średnikiem | `GL_NO_ERROR` | tylko w `GL_COMPILE_STATUS` i w dzienniku shadera |
| Shader fragmentów czyta zmienną, której shader wierzchołków nie zapisuje | `GL_NO_ERROR` | tylko w `GL_LINK_STATUS` i w dzienniku programu |

Kto nie odczyta statusu, dostaje program, który "działa" i niczego nie rysuje. Dopiero użycie niezlinkowanego programu w `glUseProgram` kończy się błędem `GL_INVALID_OPERATION`, ale ten błąd nie mówi już, co było nie tak w shaderze.

## 4. Shadery

Klasa nie ma własnych shaderów i nie zna ich treści: dostaje dwie ścieżki i buduje program z dowolnej pary plików. W projekcie jest pięć par (`basic`, `textured`, `color`, `lit`, `gouraud`) i jeden plik dołączany, `common/lighting.glsl`. Przykłady w tym dokumencie pokazuję na parze `basic.vert` i `basic.frag`, opisanej linia po linii w [`shaders.md`](shaders.md) (sekcja 4). Tabela wszystkich pięciu programów jest w [`shaders.md`](shaders.md) (sekcja 1).

Jedyne, co klasa rozumie z treści shadera, to dwie dyrektywy: `#include "plik"`, którą sama wykonuje, i `#version`, której położenie sprawdza przy dołączaniu ([`shader-includes.md`](shader-includes.md), sekcja 5.6).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) | struktura `gfx::UniformBlockBinding` i klasa `gfx::Shader`: konstruktor, destruktor, zablokowane kopiowanie, przenoszenie, `reload`, `isValid`, `use`, `setMat4`, `setInt`, `setVec3`, `setMat3`, `setFloat`, `bindUniformBlock`, `lastError`, `vertexPath`, `fragmentPath`. Dołącza `<glad/gl.h>` (typ `GLuint`), `<glm/glm.hpp>` (typy `glm::mat4`, `glm::mat3` i `glm::vec3`), `<cstddef>` (typ `std::size_t`), `<filesystem>`, `<string>` i `<vector>` |
| [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp) | implementacja i siedem funkcji pomocniczych w anonimowej przestrzeni nazw: `readTextFile`, `shaderInfoLog`, `programInfoLog`, `compileShader`, `linkProgram`, `buildProgram`, `applyBlockBinding` |
| [`src/gfx/ShaderSource.hpp`](../../../src/gfx/ShaderSource.hpp), [`.cpp`](../../../src/gfx/ShaderSource.cpp) | `gfx::expandIncludes` i `gfx::nameSourceFiles`: preprocesor `#include` i nazwy plików w dzienniku sterownika. Sam tekst, bez OpenGL. Woła je tylko `compileShader` ([`shader-includes.md`](shader-includes.md)) |
| [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert), [`basic.frag`](../../../assets/shaders/basic.frag) | pierwsza z pięciu par shaderów projektu i ta, na której pokazane są przykłady w tym dokumencie ([`shaders.md`](shaders.md), sekcja 4). Cztery pozostałe, `textured.*`, `color.*`, `lit.*` i `gouraud.*`, buduje ta sama klasa ([`textures.md`](textures.md), sekcja 4, [`../scene/collision.md`](../scene/collision.md), sekcja 4, i [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md)) |
| [`assets/shaders/common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl) | jedyny plik dołączany: nie ma własnego obiektu `Shader`, trafia do programów `lit` i `gouraud` przez linię `#include` w `lit.frag` i `gouraud.vert` |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | właściciel pięciu obiektów: pola `m_shader`, `m_texturedShader`, `m_colorShader`, `m_litShader` i `m_gouraudShader`, wczytanie w konstruktorze, `isValid()`, `use()` i settery w funkcjach `drawCube`, `drawUnlitMaze`, `drawLitMaze`, `drawLightMarkers` i `drawColliderLines`, chronione akcesory `shader()`, `texturedShader()`, `colorShader()`, `litShader()` i `gouraudShader()` ([`shaders.md`](shaders.md), sekcja 5.1) |
| [`src/debug/panels/ShadersPanel.hpp`](../../../src/debug/panels/ShadersPanel.hpp), [`.cpp`](../../../src/debug/panels/ShadersPanel.cpp) | funkcja `debug::drawShadersPanel`: panel "Shaders" z przyciskiem "Reload shaders" ([`shader-hot-reload.md`](shader-hot-reload.md), sekcja 6). Należy do programu `night_maze`, nie do biblioteki `engine` |

Pliki klasy i pliki `ShaderSource.*` są na liście źródeł biblioteki `engine` w [`CMakeLists.txt`](../../../CMakeLists.txt). Klasa zależy tylko od `core` (`GL_CHECK`, `logError`, `pathText`), GLAD, GLM (typy macierzy i wektora w setterach) i biblioteki standardowej. Nie wie nic o panelu ani o ImGui.

```mermaid
flowchart TD
    Ctor["Shader(vertexPath, fragmentPath)"] --> Reload["reload()"]
    Reload --> Build["buildProgram(vertexPath, fragmentPath, error)"]
    Reload --> Apply["applyBlockBinding(program, binding)<br/>dla każdego wpisu z m_blockBindings"]
    BindBlock["bindUniformBlock(blockName, bindingPoint, sizeInBytes)"] --> Apply
    Build --> CompV["compileShader(GL_VERTEX_SHADER, ...)"]
    Build --> CompF["compileShader(GL_FRAGMENT_SHADER, ...)"]
    Build --> Link["linkProgram(vertexShader, fragmentShader, infoLog)"]
    CompV --> Read["readTextFile(path, text)"]
    CompF --> Read
    CompV --> Expand["expandIncludes(...)<br/>z gfx/ShaderSource.hpp"]
    CompF --> Expand
    Expand -->|"lambda readInclude"| Read
    CompV --> SLog["shaderInfoLog(shader)"]
    CompF --> SLog
    CompV --> Name["nameSourceFiles(infoLog, files)<br/>z gfx/ShaderSource.hpp"]
    CompF --> Name
    Link --> PLog["programInfoLog(program)"]
    CompV --> PText["core::pathText(path)<br/>z core/Paths.hpp"]
    CompF --> PText
    Build --> PText
```

Wszystkie funkcje pomocnicze zgłaszają niepowodzenie tak samo, bez wyjątków: zwracają `false` albo identyfikator 0, a opis błędu wpisują do parametru `std::string&`.

### 5.2 Nagłówek klasy

```cpp
class Shader {
public:
    /// Remembers both file paths and tries to load the program. It does not throw: when
    /// loading fails the error is logged, isValid() returns false and lastError() holds
    /// the message.
    Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /// Takes over the program of other. other is left without a program (not valid).
    Shader(Shader&& other) noexcept;
    /// Deletes the program this object owns, then takes over the program of other.
    Shader& operator=(Shader&& other) noexcept;
```

```cpp
private:
    std::filesystem::path m_vertexPath;
    std::filesystem::path m_fragmentPath;
    // Name (id) of the OpenGL program object. 0 is never a real program: it means "none".
    GLuint m_program = 0;
    std::string m_lastError;
    // The requests made with bindUniformBlock, repeated on every newly built program.
    std::vector<UniformBlockBinding> m_blockBindings;
};
```

Nad klasą, w tym samym nagłówku, stoi od M4 mała struktura, której elementy trzyma ostatnie pole:

```cpp
/// Which binding point a uniform block of a program reads from. A Shader keeps one of
/// these for every block it was told about (Shader::bindUniformBlock).
struct UniformBlockBinding {
    /// Name of the block as written in the shader: "uniform LightBlock { ... }".
    std::string blockName;
    /// The uniform buffer binding point the block reads from (see gfx::UniformBuffer).
    GLuint bindingPoint = 0;
    /// Size of the block in bytes as the C++ code fills it. Compared with the size the
    /// driver reports, to catch a C++ struct and a GLSL block that do not match.
    std::size_t sizeInBytes = 0;
};
```

| Element | Dlaczego tak |
|---|---|
| ścieżki jako `std::filesystem::path` | `Shader` nie wie nic o katalogu `assets/`. Pełną ścieżkę buduje wołający przez `core::assetPath` ([`../core/paths.md`](../core/paths.md)). Dzięki temu klasa nadaje się też do zadań laboratoryjnych, w których pliki leżą gdzie indziej |
| ścieżki są zapamiętane w polach | `reload()` nie ma parametrów: obiekt sam wie, z których plików powstał. Panel debug odczytuje je przez `vertexPath()` i `fragmentPath()`. Katalog każdej z nich jest też miejscem, od którego liczone są nazwy w liniach `#include` tego pliku: `compileShader` jest wołane osobno dla każdego z dwóch plików i bierze katalog z własnej ścieżki (sekcja 5.5). Dziś oba pliki każdej pary leżą w `assets/shaders`, więc to ten sam katalog |
| `m_program = 0` | 0 to "nie ma programu". Jedno pole pełni rolę identyfikatora i flagi poprawności, bez osobnego `bool` |
| `m_lastError` | ten sam tekst, który trafił do konsoli, zostaje w obiekcie, żeby panel debug mógł go pokazać |
| `m_blockBindings` (od M4) | lista próśb "blok o tej nazwie ma czytać z tego punktu wiązania". Obiekt programu OpenGL przechowuje to połączenie sam, ale tylko do następnego `reload()`, które tworzy nowy obiekt programu. Klasa musi więc pamiętać prośby po swojej stronie, żeby je powtórzyć (sekcja 5.8). To jedyny stan klasy, który opisuje **konfigurację** programu, a nie jego pochodzenie albo wynik wczytania |
| `UniformBlockBinding` jako struktura z trzema polami | jedna prośba to trzy wartości, które zawsze idą razem: nazwa bloku, numer punktu i rozmiar w bajtach do porównania z tym, co zgłasza sterownik. Znaczenie pól i sprawdzenie rozmiaru opisuje [`uniform-buffers.md`](uniform-buffers.md) |
| `= delete` przy kopiowaniu | kopia miałaby ten sam identyfikator programu i oba destruktory wołałyby `glDeleteProgram` dla tego samego obiektu ([`README.md`](README.md), sekcja 2.2) |
| `noexcept` przy przenoszeniu | obietnica, że te funkcje nie rzucają wyjątków. Kontenery biblioteki standardowej (na przykład `std::vector` przy powiększaniu) przenoszą elementy tylko wtedy, gdy przeniesienie jest `noexcept`. Przy typie, którego nie da się kopiować, to konieczność |

Pięć krótkich funkcji jest zdefiniowanych w nagłówku albo ma jedną linię w `.cpp`:

```cpp
bool isValid() const { return m_program != 0; }
```

```cpp
const std::string& lastError() const { return m_lastError; }
```

```cpp
/// File the vertex shader is read from, as given to the constructor.
const std::filesystem::path& vertexPath() const { return m_vertexPath; }

/// File the fragment shader is read from, as given to the constructor.
const std::filesystem::path& fragmentPath() const { return m_fragmentPath; }
```

Oba akcesory ścieżek zwracają `const&` do pola: nic nie jest kopiowane, a wołający nie może ścieżki zmienić. Zwracają `std::filesystem::path`, a nie gotowy napis, bo to wołający wie, czego potrzebuje: panel bierze z nich same nazwy plików (`filename()`) do linii programu i całe ścieżki do podpowiedzi ([`shader-hot-reload.md`](shader-hot-reload.md), sekcja 6.1).

```cpp
void Shader::use() const {
    GL_CHECK(glUseProgram(m_program));
}
```

`use()` nie sprawdza `isValid()`. Dla obiektu bez programu wykona `glUseProgram(0)`, czyli "żaden program", a rysowanie w takim stanie nie daje określonego wyniku. Sprawdzenie należy do wołającego. `use()` jest `const`, bo nie zmienia obiektu C++, zmienia stan kontekstu OpenGL.

Pozostałe funkcje publiczne mają własne dokumenty:

| Funkcja | Co robi | Gdzie opisana |
|---|---|---|
| `setMat4`, `setInt`, `setVec3` | ustawiają uniform typu `mat4`, `int` (także sampler) i `vec3` | [`uniforms.md`](uniforms.md), sekcje 5.1 i 5.4 |
| `setMat3`, `setFloat` (od M4) | ustawiają uniform typu `mat3` (macierz normalnych, `glUniformMatrix3fv`) i `float` (`glUniform1f`). Ten sam wzór co `setMat4`: wyszukanie położenia przy każdym wywołaniu, bez pamięci podręcznej, położenie -1 ignorowane | [`uniforms.md`](uniforms.md), część o setterach |
| `bindUniformBlock` (od M4) | łączy blok uniformów programu z punktem wiązania i zapamiętuje tę prośbę w `m_blockBindings` | [`uniform-buffers.md`](uniform-buffers.md), razem z funkcją pomocniczą `applyBlockBinding` |
| `reload` | buduje program z plików i podmienia stary tylko przy sukcesie | [`shader-hot-reload.md`](shader-hot-reload.md), sekcja 5.2, a zmiana z M4 także tutaj, w sekcji 5.8 |

Wszystkie settery są `const`, tak jak `use()`: zmieniają stan programu OpenGL, a nie obiektu C++. `bindUniformBlock` **nie jest** `const`, bo dopisuje element do `m_blockBindings`.

### 5.3 Wczytanie pliku: `readTextFile` i `core::pathText`

```cpp
// Reads a whole text file into text. Returns false when the file cannot be opened.
bool readTextFile(const std::filesystem::path& path, std::string& text) {
    // The stream closes the file in its destructor.
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // rdbuf() is the buffer the stream reads the file through. Sending it to another
    // stream with << copies everything up to the end of the file.
    std::ostringstream contents;
    contents << file.rdbuf();
    text = contents.str();
    return true;
}
```

| Linia | Co robi |
|---|---|
| `std::ifstream file(path);` | Otwiera plik do czytania w trybie tekstowym. Konstruktor przyjmuje `std::filesystem::path` wprost, bez zamiany na napis, więc na Windowsie ścieżka ze znakami spoza strony kodowej działa. Plik zamknie destruktor strumienia (RAII) |
| `if (!file.is_open())` | Nie ma pliku, nie ma uprawnień albo ścieżka jest zła. Strumienie domyślnie nie rzucają wyjątków, więc trzeba zapytać |
| `std::ostringstream contents;` | Strumień, który pisze do napisu w pamięci |
| `contents << file.rdbuf();` | `rdbuf()` zwraca wskaźnik do bufora strumienia pliku. Operator `<<` dla takiego wskaźnika przepisuje wszystko, co da się z niego przeczytać, do końca pliku. To standardowy sposób na "wczytaj cały plik" bez pętli i bez znajomości rozmiaru |
| `text = contents.str();` | `str()` zwraca zebrany tekst jako `std::string` |

Wynik wraca przez parametr `std::string& text`, a wartość zwracana `bool` mówi tylko "udało się albo nie". To celowo prosty kształt: bez `std::optional` i bez wyjątków.

Tryb tekstowy ma na Windowsie jedną konsekwencję: końce linii `\r\n` są przy czytaniu zamieniane na `\n`. Kompilatorowi GLSL jest to obojętne.

Ścieżkę na tekst do komunikatu błędu zamienia `core::pathText` z [`src/core/Paths.hpp`](../../../src/core/Paths.hpp), opisane linia po linii w [`../core/paths.md`](../core/paths.md) (sekcja 5.7):

```cpp
std::string pathText(const std::filesystem::path& path);
```

Funkcja była najpierw prywatną funkcją pomocniczą w `Shader.cpp`. Przeniosłem ją do `core`, gdy tej samej zamiany zaczął potrzebować panel "Shaders" (nazwy plików w etykietach): `debug/` nie ma dostępu do anonimowej przestrzeni nazw w `Shader.cpp`, a kopia tych samych trzech linii w panelu byłaby powtórzeniem. Dla `Shader` ważne są dwie jej własności. Po pierwsze **nie rzuca wyjątku** dla ścieżki, której nie da się zapisać w stronie kodowej Windowsa (inaczej niż `path.string()`), a konstruktor `Shader` obiecuje, że nie rzuca, więc komunikat o błędzie nie może sam być źródłem wyjątku. Po drugie zwraca UTF-8, czyli to, czego oczekuje ImGui, więc `lastError()` da się wyświetlić w panelu bez dalszych zamian.

### 5.4 Dziennik sterownika: `shaderInfoLog` i `programInfoLog`

```cpp
// Text the driver wrote while compiling a shader: errors and warnings with line numbers.
std::string shaderInfoLog(GLuint shader) {
    // Length of the log in characters, including the terminating zero. 0 means no log.
    GLint length = 0;
    GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
    if (length <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(length), '\0');
    // written receives the number of characters copied, without the terminating zero.
    GLsizei written = 0;
    GL_CHECK(glGetShaderInfoLog(shader, length, &written, log.data()));
    log.resize(static_cast<std::size_t>(written));
    return log;
}
```

| Linia | Co robi |
|---|---|
| `GLint length = 0;` i `glGetShaderiv(..., GL_INFO_LOG_LENGTH, &length)` | Pytam o długość dziennika. OpenGL zwraca wyniki przez wskaźnik, więc zmienna musi istnieć wcześniej. Wartość początkowa 0 zostaje, gdyby wywołanie się nie powiodło |
| `if (length <= 0) { return {}; }` | Brak dziennika: zwracam pusty napis. `return {};` tworzy domyślny (pusty) `std::string` |
| `std::string log(static_cast<std::size_t>(length), '\0');` | Bufor o dokładnie potrzebnej długości, wypełniony zerami. `length` jest typu `GLint` (ze znakiem), a konstruktor chce `std::size_t` (bez znaku), stąd jawne rzutowanie. Wcześniejszy warunek gwarantuje, że liczba jest dodatnia |
| `glGetShaderInfoLog(shader, length, &written, log.data())` | Kopiuje dziennik do bufora. `log.data()` daje `char*` do pamięci napisu |
| `log.resize(static_cast<std::size_t>(written));` | `length` liczyło kończące zero, `written` go nie liczy. Skracam napis, żeby zero nie zostało na końcu tekstu |

To ten sam wzorzec "zapytaj o rozmiar, przydziel bufor, wypełnij, przytnij" co przy `_NSGetExecutablePath` w [`../core/paths.md`](../core/paths.md) (sekcja 5.5).

`programInfoLog` jest kopią tej funkcji z dwiema różnicami: woła `glGetProgramiv` i `glGetProgramInfoLog`, bo obiekt programu ma własną parę funkcji.

```cpp
// Text the driver wrote while linking a program. Same steps as shaderInfoLog, but
// a program object has its own pair of functions.
std::string programInfoLog(GLuint program) {
    GLint length = 0;
    GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
    if (length <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    GL_CHECK(glGetProgramInfoLog(program, length, &written, log.data()));
    log.resize(static_cast<std::size_t>(written));
    return log;
}
```

Dwie prawie identyczne funkcje są tu świadomym wyborem. Wspólna wersja wymagałaby przekazywania wskaźników do funkcji OpenGL albo szablonu, a to byłoby trudniejsze do wytłumaczenia niż dwanaście powtórzonych linii.

### 5.5 Kompilacja jednego shadera: `compileShader`

W M4 funkcja dostała środkową część: między wczytaniem pliku a `glCreateShader` rozwija linie `#include`, a przy błędzie kompilacji zamienia w dzienniku numery plików na nazwy.

```cpp
// Reads one shader file, puts the files it includes into it and compiles the result.
// type is GL_VERTEX_SHADER or GL_FRAGMENT_SHADER. Returns the id of the shader object,
// or 0 on failure with the message in error.
GLuint compileShader(GLenum type, const std::filesystem::path& path, std::string& error) {
    std::string fileText;
    if (!readTextFile(path, fileText)) {
        error = "Shader file cannot be opened: " + core::pathText(path);
        return 0;
    }

    // The name in an #include line is relative to the directory of the shader file:
    // "common/lighting.glsl" in assets/shaders/lit.frag is the file
    // assets/shaders/common/lighting.glsl. The lambda is called once for every #include
    // line, on every load, so a reload reads the included files again too.
    const std::filesystem::path includeDirectory = path.parent_path();
    const IncludeReader readInclude = [&includeDirectory](const std::string& name,
                                                          std::string& text) {
        return readTextFile(includeDirectory / name, text);
    };

    ShaderSource source;
    std::string includeError;
    if (!expandIncludes(core::pathText(path.filename()), fileText, readInclude, source,
                        includeError)) {
        error = "Shader include failed: " + core::pathText(path) + "\n" + includeError;
        return 0;
    }

    GLuint shader = 0;
    GL_CHECK(shader = glCreateShader(type));

    // glShaderSource takes an array of C strings. Here the array has one element.
    // nullptr as the array of lengths means that every string ends with a zero.
    const char* sourceText = source.text.c_str();
    GL_CHECK(glShaderSource(shader, 1, &sourceText, nullptr));
    GL_CHECK(glCompileShader(shader));

    // A compile error does not set an OpenGL error flag, so GL_CHECK cannot see it.
    // The result has to be asked for.
    GLint status = GL_FALSE;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status != GL_TRUE) {
        // The driver names a file by its number in source.files. nameSourceFiles writes
        // the name instead, so an error inside an included file is reported against it.
        error = "Shader compilation failed: " + core::pathText(path) + "\n" +
                nameSourceFiles(shaderInfoLog(shader), source.files);
        GL_CHECK(glDeleteShader(shader));
        return 0;
    }
    return shader;
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `std::string fileText;` i `if (!readTextFile(path, fileText))` | Pierwszy z czterech rodzajów błędu: samego pliku shadera nie da się otworzyć. Żaden obiekt OpenGL jeszcze nie powstał, więc nie ma czego sprzątać. Zmienna nazywa się `fileText`, bo to dosłowna treść pliku, jeszcze z liniami `#include` |
| `const std::filesystem::path includeDirectory = path.parent_path();` | Katalog pliku shadera, liczony **raz**. Dla `<katalog programu>/assets/shaders/lit.frag` jest to `<katalog programu>/assets/shaders`. Od niego liczone są nazwy we wszystkich liniach `#include`, także tych w plikach dołączanych |
| `const IncludeReader readInclude = [&includeDirectory](const std::string& name, std::string& text) { ... };` | Lambda, czyli funkcja bez nazwy zapisana w miejscu użycia. `[&includeDirectory]` przechwytuje jedną zmienną lokalną przez referencję. `IncludeReader` to `std::function` o sygnaturze, której chce `expandIncludes`. Dzięki temu preprocesor nie otwiera plików sam i da się go testować bez dysku |
| `return readTextFile(includeDirectory / name, text);` | Plik dołączany czyta ta sama funkcja co plik shadera. `operator/` skleja katalog z nazwą z cudzysłowów |
| `ShaderSource source;` | Wynik rozwinięcia: `source.text` (tekst dla sterownika) i `source.files` (lista plików, w której indeks jest numerem pliku używanym w dyrektywach `#line`) |
| `expandIncludes(core::pathText(path.filename()), fileText, readInclude, source, includeError)` | Pierwszy argument to **sama nazwa pliku** (`lit.frag`), nie pełna ścieżka: trafi do `source.files[0]` i do błędów. Dla shadera bez dołączeń funkcja oddaje tekst bez zmian (poza końcami linii) i listę z jednym plikiem |
| `error = "Shader include failed: " + core::pathText(path) + "\n" + includeError;` | Drugi rodzaj błędu, nowy w M4: preprocesor odmówił (brak pliku dołączanego, cykl, zły zapis, `#include` przed `#version`, `#version` w pliku dołączanym). Pierwsza linia komunikatu to przedrostek i pełna ścieżka shadera, druga to tekst preprocesora w kształcie `plik:linia: opis`. Nadal nie powstał żaden obiekt OpenGL |
| `GLuint shader = 0;` potem `GL_CHECK(shader = glCreateShader(type));` | Wywołanie zwracające wartość w `GL_CHECK`: przypisanie jest w środku makra, a zmienna jest zadeklarowana przed nim ([`../core/gl-check.md`](../core/gl-check.md), sekcja 5.2) |
| `const char* sourceText = source.text.c_str();` | Do sterownika idzie tekst **po rozwinięciu**. `glShaderSource` chce **tablicy** wskaźników (`const GLchar* const*`). Mam jeden napis, więc robię zmienną ze wskaźnikiem i podaję jej adres: `&sourceText` to tablica o jednym elemencie. Nie da się napisać `&source.text.c_str()`, bo nie można wziąć adresu wartości tymczasowej |
| `glShaderSource(shader, 1, &sourceText, nullptr)` | `1` to liczba napisów, także gdy shader składa się z kilku plików: są już sklejone w jeden tekst. `nullptr` zamiast tablicy długości: napis kończy się zerem, co `c_str()` gwarantuje. OpenGL **kopiuje** tekst, więc `source` może zniknąć po tym wywołaniu |
| `GLint status = GL_FALSE;` | Wartość początkowa to "nie udało się". Gdyby `glGetShaderiv` samo zawiodło (na przykład `shader` równe 0), zmienna zostanie nietknięta i kod pójdzie ścieżką błędu |
| `if (status != GL_TRUE)` | Trzeci rodzaj błędu: kompilacja w sterowniku |
| `nameSourceFiles(shaderInfoLog(shader), source.files)` | Dziennik sterownika przechodzi przez funkcję, która na początku każdej linii w znanym formacie zamienia numer pliku na jego nazwę: `0(15)` na `basic.frag(15)`, `1(63)` na `common/lighting.glsl(63)`. Reszta każdej linii zostaje tak, jak napisał ją sterownik. Gdy plików jest więcej niż jeden, na końcu dochodzi linia `Source files: 0 = ..., 1 = ...` |
| `GL_CHECK(glDeleteShader(shader)); return 0;` | Nieudany obiekt shadera usuwam od razu. Kolejność ma znaczenie: dziennik trzeba odczytać **przed** usunięciem obiektu, i tak jest, bo `shaderInfoLog(shader)` wykonuje się w linii wyżej |

Wartość zwracana 0 jest naturalnym sygnałem błędu, bo OpenGL nigdy nie nadaje obiektowi identyfikatora 0.

Czwarty rodzaj błędu, linkowanie, powstaje piętro wyżej (sekcje 5.6 i 5.7). Trzy komunikaty tej funkcji różnią się przedrostkiem, i po nim widać, kto odmówił:

| Przedrostek | Kto odmówił | Co stoi w następnej linii |
|---|---|---|
| `Shader file cannot be opened: <ścieżka>` | system plików | nic |
| `Shader include failed: <ścieżka>` | mój preprocesor | `lit.frag:9: included file cannot be opened: "common/lighting.glsl"` albo inny z pięciu komunikatów ([`shader-includes.md`](shader-includes.md), sekcja 5.6) |
| `Shader compilation failed: <ścieżka>` | kompilator GLSL w sterowniku | dziennik sterownika z nazwami plików |

Kod preprocesora (`expandIncludes`, `nameSourceFiles` i ich funkcje pomocnicze) jest opisany linia po linii w [`shader-includes.md`](shader-includes.md) (sekcje od 5.2 do 5.9).

### 5.6 Linkowanie: `linkProgram`

```cpp
// Links two compiled shaders into a new program. Returns the id of the program object,
// or 0 on failure with the driver's text in infoLog.
GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& infoLog) {
    GLuint program = 0;
    GL_CHECK(program = glCreateProgram());
    GL_CHECK(glAttachShader(program, vertexShader));
    GL_CHECK(glAttachShader(program, fragmentShader));
    GL_CHECK(glLinkProgram(program));

    // A linked program keeps its own executable code, so it no longer needs the shader
    // objects. Detaching them lets glDeleteShader really free them.
    GL_CHECK(glDetachShader(program, vertexShader));
    GL_CHECK(glDetachShader(program, fragmentShader));

    // Like compiling, a failed link sets no OpenGL error flag.
    GLint status = GL_FALSE;
    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (status != GL_TRUE) {
        infoLog = programInfoLog(program);
        GL_CHECK(glDeleteProgram(program));
        return 0;
    }
    return program;
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `glAttachShader` dwa razy | Program dowiaduje się, z których obiektów shaderów ma powstać. Kolejność dołączania nie ma znaczenia |
| `glLinkProgram(program)` | Sprawdza zgodność etapów i tworzy kod dla karty |
| `glDetachShader` dwa razy, zaraz po linkowaniu | Odłączam niezależnie od wyniku linkowania. Zlinkowany program ma własną kopię kodu. Dopóki obiekt shadera jest dołączony do programu, `glDeleteShader` tylko oznacza go do usunięcia. Po odłączeniu zostanie zwolniony naprawdę |
| `GL_LINK_STATUS` | Czwarty rodzaj błędu: linkowanie. Tak jak przy kompilacji, bez flagi `glGetError` |
| `infoLog = programInfoLog(program);` przed `glDeleteProgram` | Najpierw dziennik, potem usunięcie nieudanego programu |

Funkcja zwraca przez `infoLog` **sam tekst sterownika**, bez nazw plików: dostaje tylko identyfikatory i ścieżek nie zna. Pełny komunikat składa funkcja piętro wyżej.

### 5.7 Całość: `buildProgram`

```cpp
// Builds a complete program from two files: compile, compile, link. Returns the id of
// the new program, or 0 on failure with the message in error. Whatever happens, no shader
// object is left behind.
GLuint buildProgram(const std::filesystem::path& vertexPath,
                    const std::filesystem::path& fragmentPath, std::string& error) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath, error);
    if (vertexShader == 0) {
        return 0;
    }

    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath, error);
    if (fragmentShader == 0) {
        GL_CHECK(glDeleteShader(vertexShader));
        return 0;
    }

    std::string infoLog;
    const GLuint program = linkProgram(vertexShader, fragmentShader, infoLog);

    // The shader objects were only an intermediate step, linked or not.
    GL_CHECK(glDeleteShader(vertexShader));
    GL_CHECK(glDeleteShader(fragmentShader));

    if (program == 0) {
        error = "Shader linking failed: " + core::pathText(vertexPath) + " + " +
                core::pathText(fragmentPath) + "\n" + infoLog;
    }
    return program;
}
```

Funkcja ma cztery wyjścia i przy każdym trzeba umieć powiedzieć, jakie obiekty OpenGL istnieją:

| Wyjście | Obiekty shaderów | Obiekt programu | `error` |
|---|---|---|---|
| nie udał się shader wierzchołków | żaden (`compileShader` posprzątał po sobie) | nie powstał | plik, dołączanie albo kompilacja, ścieżka shadera wierzchołków |
| nie udał się shader fragmentów | shader wierzchołków usunięty tutaj, shader fragmentów przez `compileShader` | nie powstał | plik, dołączanie albo kompilacja, ścieżka shadera fragmentów |
| nie udało się linkowanie | oba usunięte tutaj | usunięty przez `linkProgram` | obie ścieżki i dziennik programu |
| sukces | oba usunięte tutaj | zwrócony wołającemu | pusty |

Żadne wyjście nie zostawia po sobie obiektu, którego nikt nie pamięta. To jest właśnie warunek z opisu `reload()`: "przy niepowodzeniu nowe obiekty są usuwane".

Przy błędzie w shaderze wierzchołków shader fragmentów nie jest w ogóle wczytywany. W konsoli pojawi się więc błąd tylko jednego pliku naraz. Po jego poprawieniu i ponownym `reload()` wyjdzie ewentualny błąd drugiego.

Komunikat o błędzie linkowania wymienia **oba pliki**, bo błąd linkowania z natury dotyczy pary: jeden shader czegoś oczekuje, a drugi tego nie dostarcza.

Dziennik linkowania trafia do komunikatu **bez** `nameSourceFiles`: `infoLog` jest doklejany tak, jak napisał go sterownik, bez zamiany numerów na nazwy i bez linii `Source files:`. Lista plików każdego shadera żyje tylko wewnątrz `compileShader` i w tym miejscu już jej nie ma. Nie dałoby się jej też użyć jednoznacznie, bo każdy z dwóch shaderów ma własną numerację: 0 to `lit.vert` w jednym i `lit.frag` w drugim.

### 5.8 Konstruktor i destruktor

```cpp
// The paths arrive by value and are moved into the members, so a caller that passes
// a temporary (the result of core::assetPath) pays for no copy.
Shader::Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath)
    : m_vertexPath(std::move(vertexPath)), m_fragmentPath(std::move(fragmentPath)) {
    // The first load is the same work as a reload, starting from "no program".
    reload();
}
```

- Parametry są przekazane **przez wartość**, a potem przeniesione do pól przez `std::move`. Wołający zwykle poda wynik `core::assetPath(...)`, czyli obiekt tymczasowy: taki obiekt jest przenoszony do parametru, a z parametru do pola, więc ścieżka nie jest kopiowana ani razu. Przy `const std::filesystem::path&` kopia do pola byłaby nieunikniona. `std::move` samo niczego nie przenosi: to rzutowanie, które mówi "ten obiekt można opróżnić" ([`README.md`](README.md), sekcja 2.3).
- Konstruktor woła `reload()` i **ignoruje jego wynik**. Nie rzuca wyjątku przy błędzie shadera: obiekt powstaje zawsze, a czy ma program, mówi `isValid()`. Powód: literówka w pliku GLSL nie powinna zamykać całego programu, skoro da się ją poprawić i wczytać shader ponownie bez restartu. To inna decyzja niż w `core::Window`, którego konstruktor rzuca, bo bez okna nie ma czego ratować.

Co dokładnie robi `reload()` (buduje nowy program obok starego i podmienia go tylko przy sukcesie) i w jakich stanach może być obiekt po wczytaniu, opisuje [`shader-hot-reload.md`](shader-hot-reload.md) (sekcja 5.2). Dla konstruktora ważne jest jedno: pierwsze wczytanie to ta sama praca co przeładowanie, tylko zaczynająca od "braku programu".

**`reload` po M4: wiązania bloków na nowym programie.** Cała funkcja:

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

    // A binding point of a uniform block is stored in the program object, and this one
    // is new: every block is back at binding point 0. Set them again.
    for (const UniformBlockBinding& binding : m_blockBindings) {
        applyBlockBinding(program, binding);
    }

    // Only now replace the old program (glDeleteProgram(0) is ignored on the first load).
    GL_CHECK(glDeleteProgram(m_program));
    m_program = program;
    m_lastError.clear();
    return true;
}
```

Nowe są trzy linie pętli w środku. Reszta jest taka jak w M1.

| Linia | Co robi i dlaczego |
|---|---|
| `for (const UniformBlockBinding& binding : m_blockBindings)` | przechodzi po wszystkich zapamiętanych prośbach. Zmienna pętli jest referencją do stałej: element ma w środku `std::string`, więc kopia na każdy obrót byłaby zbędnym przydziałem pamięci |
| `applyBlockBinding(program, binding);` | pierwszy argument to **zmienna lokalna `program`**, czyli nowy, właśnie zbudowany obiekt, a nie `m_program`. Funkcja pyta nowy program o indeks bloku o tej nazwie, ustawia mu punkt wiązania i porównuje rozmiar bloku ze strony sterownika z rozmiarem po stronie C++ ([`uniform-buffers.md`](uniform-buffers.md)) |
| miejsce pętli: po `if (program == 0)`, przed `glDeleteProgram(m_program)` | pętla wykonuje się tylko dla programu, który się zbudował, i kończy, zanim nowy program stanie się programem obiektu. Od pierwszej klatki po podmianie nowy program ma już komplet wiązań. Przy nieudanym wczytaniu nic z tego nie zachodzi: stary program i jego wiązania zostają nietknięte |
| brak `use()` | `glUniformBlockBinding` dostaje identyfikator programu jako argument, więc program nie musi być bieżący. To różnica wobec zwykłych uniformów, gdzie `glUniform*` pisze do programu bieżącego |

Kolejność z konstruktorem ma jeden skutek, który trzeba umieć wytłumaczyć. Konstruktor woła `reload()`, gdy `m_blockBindings` jest jeszcze **puste**: przy pierwszym wczytaniu pętla nie robi nic. Prośby przychodzą dopiero później, z `bindUniformBlock`, wołanego w ciele konstruktora `NightMazeApp` przez `LightRig::connect`. Ta funkcja dopisuje wpis do listy i, jeśli program już istnieje, od razu stosuje go na bieżącym programie:

```cpp
void Shader::bindUniformBlock(std::string blockName, GLuint bindingPoint, std::size_t sizeInBytes) {
    m_blockBindings.push_back({.blockName = std::move(blockName),
                               .bindingPoint = bindingPoint,
                               .sizeInBytes = sizeInBytes});
    // The program that exists now gets the binding at once. Without a program (a failed
    // first load) the request waits for the next successful reload.
    if (isValid()) {
        applyBlockBinding(m_program, m_blockBindings.back());
    }
}
```

Są więc dwie drogi do `applyBlockBinding` i razem pokrywają wszystkie przypadki: program istniejący w chwili prośby dostaje wiązanie od razu, a każdy program zbudowany później dostaje je w `reload()`. Także ten, którego pierwsze wczytanie się nie udało (literówka w `lit.frag` przy starcie): wpis czeka na liście, a po poprawieniu pliku i naciśnięciu `Reload shaders` trafia na pierwszy poprawny program. Lista tylko rośnie: klasa nie ma funkcji, która usuwa wpis, i nie sprawdza powtórzeń, więc dwie prośby o ten sam blok dałyby dwa wpisy i dwa wywołania przy każdym przeładowaniu. W projekcie każdy z dwóch programów oświetlenia dostaje jedną prośbę, raz.

```cpp
Shader::~Shader() {
    // OpenGL silently ignores glDeleteProgram(0), so an object without a program
    // (a failed load, or one that was moved from) needs no special case.
    GL_CHECK(glDeleteProgram(m_program));
}
```

Destruktor ma jedną linię, bez `if (m_program != 0)`. Specyfikacja OpenGL mówi, że `glDeleteProgram` dla wartości 0 jest po cichu ignorowane (bez błędu), i na tym polegam w trzech miejscach: tutaj, w `reload()` i w przypisaniu przenoszącym.

### 5.9 Przenoszenie

Ogólne wyjaśnienie, czym jest przeniesienie i dlaczego opakowania obiektów OpenGL nie wolno kopiować, jest w [`README.md`](README.md), sekcja 2. Tutaj sam kod.

```cpp
// Move constructor: the new object takes the program id, and other gives it up.
Shader::Shader(Shader&& other) noexcept
    : m_vertexPath(std::move(other.m_vertexPath)),
      m_fragmentPath(std::move(other.m_fragmentPath)),
      m_program(other.m_program),
      m_lastError(std::move(other.m_lastError)),
      m_blockBindings(std::move(other.m_blockBindings)) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_program = 0;
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `Shader&& other` | Referencja do r-wartości (rvalue reference): parametr przyjmuje obiekt tymczasowy albo taki, który wołający oznaczył przez `std::move`. To sygnał "z tego obiektu wolno zabrać zawartość" |
| `std::move(other.m_vertexPath)` i pozostałe | Ścieżki i napis błędu są przenoszone ich własnymi konstruktorami przenoszącymi: nowy obiekt przejmuje ich pamięć bez kopiowania znaków |
| `m_program(other.m_program)` | Identyfikator to zwykła liczba, więc jest po prostu kopiowany. `std::move` dla liczby nic by nie zmieniło |
| `m_blockBindings(std::move(other.m_blockBindings))` (od M4) | Lista próśb o wiązania idzie **razem z programem**. Wiązania są zapisane w obiekcie programu, który właśnie zmienia właściciela, więc nowy właściciel musi też znać listę: inaczej jego pierwsze `reload()` zbudowałoby program bez wiązań. Przeniesienie wektora przejmuje jego pamięć i zostawia `other` z pustą listą. Linia stoi na końcu listy inicjalizacyjnej, bo pole jest zadeklarowane jako ostatnie, a pola są inicjalizowane w kolejności deklaracji |
| `other.m_program = 0;` | **Najważniejsza linia.** Po skopiowaniu liczby oba obiekty mają ten sam identyfikator. Zerując go w `other`, sprawiam, że właściciel jest jeden, a destruktor `other` wykona `glDeleteProgram(0)`, czyli nic |

Bez ostatniej linii konstruktor przenoszący byłby kopiującym w przebraniu, z dokładnie tym błędem, przed którym chroni `= delete`: dwa destruktory, jeden program.

```cpp
// Move assignment: this object already owns a program, which has to go first.
Shader& Shader::operator=(Shader&& other) noexcept {
    // shader = std::move(shader): nothing to do. Without this check the program would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the program owned so far (ignored by OpenGL when it is 0).
    GL_CHECK(glDeleteProgram(m_program));

    m_vertexPath = std::move(other.m_vertexPath);
    m_fragmentPath = std::move(other.m_fragmentPath);
    m_program = other.m_program;
    m_lastError = std::move(other.m_lastError);
    m_blockBindings = std::move(other.m_blockBindings);
    other.m_program = 0;
    return *this;
}
```

Przypisanie różni się od konstruktora jednym: obiekt po lewej stronie **już istnieje i może mieć własny program**. Trzy kroki:

1. **Przypisanie do samego siebie.** `this == &other` porównuje adresy. Bez tego warunku `shader = std::move(shader)` usunęłoby program, a potem "przejęło" identyfikator już nieistniejącego obiektu i na koniec wyzerowało go: obiekt zostałby bez programu.
2. **Zwolnienie własnego programu.** Bez tej linii stary identyfikator zostałby nadpisany i nikt nie zawołałby już dla niego `glDeleteProgram`: wyciek obiektu OpenGL.
3. **Przejęcie** pól `other` i wyzerowanie jego identyfikatora, tak jak w konstruktorze. Od M4 także `m_blockBindings = std::move(other.m_blockBindings);`: stara lista obiektu po lewej znika razem z jego starym programem (dotyczyła programu, który przed chwilą został usunięty), a w jej miejsce wchodzi lista obiektu po prawej, która opisuje przejmowany program.

Po obu operacjach przenoszenia obowiązuje ta sama zasada: identyfikator programu i lista jego wiązań zawsze zmieniają właściciela razem. Gdyby lista została w starym obiekcie, błąd nie pokazałby się od razu (przejęty program ma wiązania ustawione wcześniej), tylko po pierwszym przeładowaniu nowego właściciela. W projekcie obiekty `Shader` są polami `NightMazeApp` i nie są przenoszone, więc ta ścieżka wynika z kodu i nie była sprawdzana.

`return *this;` zwraca referencję do obiektu po lewej, jak każdy operator przypisania, żeby dało się pisać `a = b = c`.

### 5.10 Jak to zostało sprawdzone

**Test samej klasy.** Zanim klasa dostała użytkownika, sprawdziłem ją na Macu małym programem testowym poza repozytorium: ukryte okno GLFW z kontekstem 4.1 Core, biblioteka `engine` z buildu Debug i kilka plików shaderów w katalogu tymczasowym. Wyniki (sterownik Apple, `GL_VERSION` 4.1 Metal):

| Próba | Wynik |
|---|---|
| najprostsza poprawna para (jeden atrybut pozycji, stały kolor) | `isValid()` prawda, `lastError()` pusty |
| nieistniejący plik | `isValid()` fałsz, `Shader file cannot be opened: <ścieżka>` |
| brak średnika w shaderze wierzchołków | `Shader compilation failed: <ścieżka>`, potem `ERROR: 0:5: '}' : syntax error: syntax error` |
| brak linii `#version` | `ERROR: 0:1: '' :  #version required and missing.` |
| `#version 330 core` | kompiluje się |
| `#version 420 core` i `#version 460 core` | `ERROR: 0:1: '' :  version '420' is not supported` (odpowiednio `'460'`) |
| shader fragmentów z `in vec3 vColor`, którego shader wierzchołków nie zapisuje | `Shader linking failed: <ścieżka> + <ścieżka>`, potem `ERROR: Input of fragment shader 'vColor' not written by vertex shader` |
| `reload()` po zepsuciu pliku | zwraca `false`, `isValid()` nadal prawda, `glGetIntegerv(GL_CURRENT_PROGRAM)` po `use()` daje ten sam identyfikator co przed próbą |
| `reload()` po naprawieniu pliku | zwraca `true`, `lastError()` pusty, nowy identyfikator |
| konstruktor przenoszący, przypisanie przenoszące, przypisanie do samego siebie | obiekt docelowy poprawny, obiekt źródłowy z `isValid()` fałsz, po przypisaniu do siebie obiekt nadal poprawny |
| `glGetError` na końcu testu | `GL_NO_ERROR` |

Zwraca uwagę trzeci wiersz: brakujący średnik był w linii 4, a sterownik wskazał linię 5, bo błąd zauważył dopiero przy następnym znaku (`}`). Numer linii w dzienniku to miejsce, w którym kompilator się zgubił, a nie zawsze miejsce pomyłki.

**Program `night_maze` (wersja z trójkątem).** Po dodaniu pierwszej geometrii, jednego trójkąta, uruchomiłem program na Macu trzy razy, każdorazowo na około 3 sekundy, i przeczytałem jego wyjście (`<repo>` to katalog repozytorium):

| Próba | Wyjście programu | Wynik |
|---|---|---|
| start z katalogu repozytorium | dwie linie `[info]` z `GL_VERSION` i `GL_RENDERER`, żadnej linii `[error]` | program działa |
| start z katalogu `/tmp` (inny katalog roboczy) | to samo | shadery znalezione, bo ścieżka idzie przez `core::assetPath` |
| usunięty średnik w linii 14 pliku `basic.frag` | jak niżej | błąd wypisany **raz**, program działa dalej (nie zamknął się przez 3 sekundy) |

```text
[info] GL_VERSION:  4.1 Metal - 90.5
[info] GL_RENDERER: Apple M3
[error] Shader compilation failed: <repo>/build/debug/assets/shaders/basic.frag
ERROR: 0:15: '}' : syntax error: syntax error
```

Ścieżka w komunikacie prowadzi przez `build/debug/assets`, czyli przez dowiązanie obok programu, a nie wprost do katalogu repozytorium: to jest ścieżka, którą zbudowało `core::assetPath`. Sterownik wskazuje linię 15, choć średnika brakuje w linii 14 (uwaga pod tabelą wyżej).

Te uruchomienia sprawdzały wyjście tekstowe, a nie obraz w oknie. Obraz sprawdzał wtedy osobny test z ukrytym oknem i `glReadPixels` ([`buffers-vao.md`](buffers-vao.md), sekcja 5.8).

Dwie dalsze serie prób są opisane przy swoich tematach: test panelu Shaders i przeładowania kliknięciem w [`shader-hot-reload.md`](shader-hot-reload.md) (sekcja 5.3), a test kostki, trzech macierzy i literówki w nazwie uniformu w [`uniforms.md`](uniforms.md) (sekcja 5.3).

**Windows (2026-10-05), wersja z M1 z samą kostką.** `Shader.cpp` i `ShadersPanel.cpp` kompilują się w MSVC 19.44 pod `/W4 /permissive-` bez ostrzeżeń (Debug i Release). Program `night_maze` uruchomiony na karcie NVIDIA wypisał dwie linie `[info]`, żadnej linii `[error]` i narysował kostkę, a z usuniętym średnikiem w pliku `basic.frag` wypisał błąd **raz** i działał dalej z samym tłem i panelami:

```text
[info] GL_VERSION:  4.1.0 NVIDIA 610.74
[info] GL_RENDERER: NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2
[error] Shader compilation failed: <repo>\build\debug\Debug\assets\shaders/basic.frag
0(15) : error C0000: syntax error, unexpected '}', expecting ',' or ';' at token "}"
```

Linia sterownika ma inny format niż na Macu ([`shaders.md`](shaders.md), sekcja 2.6), ale wskazuje linię 15, tak jak sterownik Apple w próbie wyżej. Ścieżka ma mieszane ukośniki: wsteczne w części z katalogu programu i zwykły przed nazwą pliku, bo nazwa `shaders/basic.frag` jest w kodzie zapisana z `/` ([`../core/paths.md`](../core/paths.md), sekcja 5.8). Pozostałych prób z tabel wyżej (test klasy z ukrytym oknem, brak pliku, błąd linkowania) na Windowsie nie powtarzałem, a przycisku `Reload shaders` nikt tam jeszcze nie nacisnął ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 11).

**Windows (2026-10-05), wersja z M2 + M3.** Klasa się wtedy nie zmieniła, ale dostała trzy obiekty. Program z trzema parami shaderów budował się w MSVC 19.44 bez ostrzeżeń (Debug i Release) i startował bez linii `[error]` i bez linii `GL_`, czyli pliki `textured.*` i `color.*` kompilują się i linkują na sterowniku NVIDIA. Próby z zepsutym plikiem nie powtarzałem wtedy dla nowych shaderów.

**Wszystkie wyjścia powyżej pochodzą sprzed M4.** Linie sterownika zaczynają się w nich od numeru napisu źródłowego: `ERROR: 0:15:` na Macu i `0(15)` na Windowsie. Dzisiejszy kod zamienia ten numer na nazwę pliku, więc ta sama próba dałaby `ERROR: basic.frag:15: ...` (wynika z kodu i z testu jednostkowego, na macOS nieuruchomione) i `basic.frag(15) : error C0000: ...` (format z nawiasem jest na Windowsie zmierzony, niżej).

**Windows (2026-10-05), wersja z M4.** Klasa ma pięć obiektów, rozwija dołączenia i odtwarza wiązania bloków. Zmierzone (MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74): build Debug i Release bez ostrzeżeń, clang-format i clang-tidy bez uwag, start gry bez linii `[error]` i bez linii `GL_`. Start bez błędów znaczy tu trzy rzeczy: pliki `lit.*` i `gouraud.*` kompilują się i linkują, tekst z dyrektywami `#line` jest dla sterownika poprawny, a rozmiar bloku `LightBlock` zgłoszony przez sterownik zgadza się z rozmiarem po stronie C++ (inaczej `applyBlockBinding` wypisałoby linię `[error]`). Zmierzone są też dwie postaci błędu kompilacji:

| Sytuacja | Surowa linia sterownika | Co wypisuje program |
|---|---|---|
| błąd w `common/lighting.glsl`, dołączonym do `lit.frag` | `1(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"` | `common/lighting.glsl(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"` |
| błąd w shaderze bez dołączeń (`basic.frag`) | zaczyna się od `0(4)` | zaczyna się od `basic.frag(4)` |

Na zrzucie ekranu sprawdzone jest, że przy błędzie w `common/lighting.glsl` panel Shaders pokazuje komunikat z nazwą tego pliku, a labirynt rysuje poprzedni program. Nie były sprawdzane: błąd linkowania i brak pliku w wersji z M4, komunikat `Shader include failed` w działającym programie (jego treść sprawdzają testy jednostkowe preprocesora), powrót wiązania bloku po przeładowaniu kliknięciem i przenoszenie obiektu z niepustą listą `m_blockBindings`. Przycisku `Reload shaders` z pięcioma programami nikt nie nacisnął ręcznie. Na macOS osiem plików shaderów dodanych po M1 i plik `common/lighting.glsl` nie były kompilowane przez sterownik Apple: nic z M4 nie było tam budowane ani uruchamiane.

Gdzie obiekty klasy są tworzone i używane w klatce, opisuje [`shaders.md`](shaders.md) (sekcja 5.1).

## 6. Panel ImGui

Klasa `gfx::Shader` ma własny panel debug, **Shaders**: przycisk wołający `reload()` dla wszystkich pięciu programów i jedna linia na program, z nazwami obu plików z `vertexPath()` i `fragmentPath()` i wynikiem ostatniego wczytania. `OK`, gdy `lastError()` jest pusty. W przeciwnym razie czerwone `FAILED` z dopiskiem zależnym od `isValid()` (`the previous program stays in use` albo `there is no program to draw with`) i tekst z `lastError()` pod spodem. Kod panelu i droga referencji do shadera są w [`shader-hot-reload.md`](shader-hot-reload.md) (sekcja 6), a to, jak panel pokazuje błąd z pliku dołączanego, w [`shader-includes.md`](shader-includes.md) (sekcja 6).

Wiązania bloków uniformów nie mają w panelu własnej linii. Niezgodność rozmiaru bloku trafia tylko do konsoli, jako linia `[error] Uniform block ...`, i nie zmienia `lastError()`.

## 7. Pułapki

1. **Błędy kompilacji i linkowania są niewidoczne dla `glGetError`.** `GL_CHECK(glCompileShader(shader))` nigdy nie zgłosi błędu składni GLSL. Jedynym źródłem informacji jest `GL_COMPILE_STATUS`, `GL_LINK_STATUS` i dziennik (sekcja 3.3). Program, który ich nie czyta, po prostu niczego nie rysuje.
2. **Destruktor bez kontekstu.** `~Shader` woła `glDeleteProgram`, a każda funkcja `gl*` wymaga bieżącego kontekstu. Obiekt `Shader` żyjący dłużej niż okno (zmienna globalna, zmienna lokalna w `main` zadeklarowana przed aplikacją) wywoła OpenGL po zniszczeniu kontekstu. Poprawne miejsce to pole klasy pochodnej od `core::Application` ([`../core/README.md`](../core/README.md), sekcja 7). Z tego samego powodu obiektu nie można utworzyć **przed** powstaniem okna.
3. **Kopiowanie opakowania.** Gdyby kopiowanie nie było zablokowane, `Shader b = a;` dałoby dwa obiekty z tym samym identyfikatorem i drugi destruktor usuwałby już usunięty program (albo, co gorsza, nowy obiekt, który dostał ten sam numer). Dzięki `= delete` taka linia się nie kompiluje. Typowa sytuacja, w której to wychodzi: przekazanie `Shader` do funkcji przez wartość. Przekazuję przez `const Shader&`.
4. **Użycie obiektu po przeniesieniu.** Po `Shader b = std::move(a);` obiekt `a` ma `isValid() == false`. `a.use()` ustawi wtedy program 0.
5. **`use()` bez sprawdzenia `isValid()`.** Dla obiektu bez programu `use()` ustawia program 0 i rysowanie nie daje określonego wyniku (zwykle nic nie widać). Po nieudanym wczytaniu w konstruktorze trzeba albo pominąć rysowanie, albo poprawić plik i zawołać `reload()`.
6. **Dziennik czytany po usunięciu obiektu.** `glGetShaderInfoLog` dla usuniętego shadera zwraca błąd OpenGL zamiast tekstu. W `compileShader` i `linkProgram` dziennik jest odczytywany przed `glDeleteShader` i `glDeleteProgram`.
7. **Numer linii w błędzie wskazuje za daleko.** Brak średnika w linii 4 sterownik zgłasza w linii 5 (sekcja 5.10). Trzeba patrzeć też linię wyżej.
8. **Shader pod złym typem.** `glCreateShader(GL_VERTEX_SHADER)` z tekstem shadera fragmentów zwykle kończy się mylącym błędem kompilacji albo linkowania. O typie decyduje kolejność argumentów konstruktora `Shader` (najpierw wierzchołków, potem fragmentów), a nie rozszerzenie pliku.
9. **Nazwa pliku w pierwszej linii to nie zawsze plik z błędem.** `Shader compilation failed: ...lit.frag` mówi, **który shader** sterownik odrzucił. Gdzie jest pomyłka, mówi dopiero następna linia: `common/lighting.glsl(63)` wskazuje plik dołączany. Kto poprawia `lit.frag` po przeczytaniu samej pierwszej linii, szuka w złym pliku.
10. **Błąd linkowania ma surowy dziennik.** Przy `Shader linking failed` numery plików nie są zamieniane na nazwy i nie ma linii `Source files:` (sekcja 5.7). Dzienniki linkowania zwykle nie zawierają numerów linii, więc rzadko to przeszkadza.
11. **Ostrzeżeń kompilatora GLSL nie widać.** `compileShader` czyta dziennik tylko przy `status != GL_TRUE`. Shader, który się skompilował z ostrzeżeniem, nie zostawia śladu ani w konsoli, ani w panelu.
12. **Przeniesienie bez listy wiązań.** Konstruktor przenoszący albo przypisanie, które przeniosłyby `m_program`, a pominęły `m_blockBindings`, działałyby poprawnie aż do pierwszego `reload()` nowego właściciela. Każde nowe pole klasy trzeba dopisać w **trzech** miejscach: w deklaracji, w konstruktorze przenoszącym i w przypisaniu przenoszącym. Kompilator o brakującej linii nie przypomni.
13. **Wiązanie bloku ustawione poza klasą ginie po przeładowaniu.** `reload()` powtarza tylko prośby z `m_blockBindings`, czyli te, które przeszły przez `bindUniformBlock` (sekcja 5.8).

Pułapki dotyczące języka GLSL są w [`shaders.md`](shaders.md) (sekcja 7), uniformów w [`uniforms.md`](uniforms.md) (sekcja 7), dołączania plików w [`shader-includes.md`](shader-includes.md) (sekcja 7), a przeładowania i panelu Shaders w [`shader-hot-reload.md`](shader-hot-reload.md) (sekcja 7).

## 8. Ćwiczenia

Zasady pracy z przyciskiem `Reload shaders` są opisane w [`shaders.md`](shaders.md) (sekcja 8). Ćwiczenia 1 i 4 wymagają uruchomienia programu od nowa, a 3, 5, 6 i 7 robi się na kartce.

1. **Literówka przy starcie.** Zamknij program, usuń średnik po `fragColor = vec4(vColor, 1.0)` w `basic.frag` i uruchom program od nowa (na Windowsie najpierw odśwież kopię: `cmake --build --preset debug --target copy_assets`). Przeczytaj linię `[error]`: która część pochodzi z `Shader.cpp`, która ze sterownika, a który fragment linii sterownika podmienił mój kod? Którą linię wskazuje sterownik i dlaczego nie tę ze średnikiem? Czego brakuje w scenie (kostka unosi się nad narożną komórką labiryntu naprzeciw startu, widać ją z góry w trybie noclip), jaki dopisek stoi po `FAILED` w linii `basic.vert + basic.frag` panelu Shaders i czym różni się od dopisku po nieudanym przeładowaniu? Czy labirynt i panele działają? Wskaż w `NightMazeApp::drawCube` linię, dzięki której program się nie wysypał. Powtórz to samo z `lit.frag`: czego brakuje teraz? A z `textured.frag`: dlaczego po starcie nie brakuje niczego, choć w konsoli jest błąd? Na koniec, nie zamykając programu, przywróć średnik i naciśnij `Reload shaders`: co się zmieniło w oknie i w panelu?
2. **Błąd linkowania.** W `basic.frag` zmień nazwę `vColor` na `vColour` w obu liniach, w których występuje. Naciśnij `Reload shaders`. Czym różni się komunikat od poprzedniego i dlaczego wymienia oba pliki?
3. **Ścieżki przez `buildProgram`.** Dla każdego z czterech wyjść funkcji `buildProgram` (sekcja 5.7) wypisz po kolei wszystkie wywołania `glCreate*` i `glDelete*`, które się wykonają, i sprawdź, że każdemu `glCreate*` odpowiada `glDelete*` albo zwrócenie identyfikatora. Które wyjście wystąpiło w ćwiczeniu 1, a które w ćwiczeniu 2?
4. **Brak pliku.** W `NightMazeApp.cpp` zmień `VERTEX_SHADER_FILE` na nieistniejącą nazwę, zbuduj i uruchom. Jaka linia pojawia się w konsoli i ile razy? Wycofaj zmianę.
5. **Przeniesienie na kartce.** Dla kodu `Shader a(p1, p2); Shader b = std::move(a);` zapisz wartość `m_program` w obu obiektach po każdej linii (przyjmij, że program dostał identyfikator 3). Ile razy i z jakim argumentem zostanie zawołane `glDeleteProgram`, gdy oba obiekty wyjdą z zasięgu? Powtórz, zakładając, że w konstruktorze przenoszącym brakuje linii `other.m_program = 0;`.
6. **Przypisanie do siebie.** Prześledź na kartce `a = std::move(a);` dla obiektu z programem 3, najpierw z warunkiem `if (this == &other)`, potem bez niego. W jakim stanie zostaje obiekt w drugim przypadku?
7. **Lista wiązań na kartce.** Obiekt `lit` powstaje (program 5), dostaje `bindUniformBlock("LightBlock", 1, 928)`, a potem dwa razy udane `reload()` (programy 6 i 7). Zapisz po każdym kroku: zawartość `m_blockBindings`, wartość `m_program` i dla którego identyfikatora programu wykonało się `applyBlockBinding`. Powtórz dla przypadku, w którym pierwsze wczytanie w konstruktorze się nie udało.
8. **Trzy przedrostki.** Wywołaj po kolei każdy z trzech komunikatów `compileShader` dla programu `lit`: zmień nazwę `lit.frag` (na Windowsie w kopii obok programu), potem zmień nazwę w linii `#include`, potem usuń średnik w `common/lighting.glsl`. Po każdej zmianie naciśnij `Reload shaders` i zapisz pierwszą linię komunikatu. W którym z trzech przypadków powstał obiekt shadera OpenGL?

## 9. Pytania kontrolne

1. **Dlaczego `GL_CHECK` nie wystarczy do wykrycia błędu w shaderze?**
   `glGetError` zgłasza błędne użycie API. Shader z błędem składni to poprawnie wywołane `glCompileShader` z wynikiem "nie kompiluje się", więc żadna flaga nie jest ustawiana. Wynik trzeba odczytać przez `glGetShaderiv(GL_COMPILE_STATUS)` i `glGetProgramiv(GL_LINK_STATUS)`, a tekst błędu przez `glGetShaderInfoLog` i `glGetProgramInfoLog`.

2. **Dlaczego `glShaderSource` dostaje `&sourceText`, a nie `source.c_str()`?**
   Funkcja przyjmuje tablicę napisów C (wskaźnik na wskaźnik) i ich liczbę. Mam jeden napis, więc zapisuję wskaźnik w zmiennej i podaję jej adres jako tablicę jednoelementową. `nullptr` jako tablica długości oznacza napisy zakończone zerem.

3. **Jak odczytywany jest dziennik i po co `resize` na końcu?**
   `GL_INFO_LOG_LENGTH` podaje długość razem z kończącym zerem. Tworzę `std::string` tej długości, `glGetShaderInfoLog` wypełnia go i wpisuje do `written` liczbę znaków bez zera, a `resize(written)` obcina zero z końca napisu.

4. **Dlaczego konstruktor `Shader` nie rzuca wyjątku przy błędzie w shaderze?**
   Bo błąd w pliku GLSL da się naprawić bez restartu: poprawić plik i zawołać `reload()`. Obiekt powstaje zawsze, `isValid()` mówi, czy ma program, a `lastError()` dlaczego nie. Zamknięcie programu z powodu literówki w shaderze przekreślałoby sens wczytywania na żywo.

5. **Co zawiera komunikat błędu i ile z dziennika sterownika zmienia mój kod?**
   Rodzaj błędu, ścieżkę pliku (przy linkowaniu obie ścieżki) i dziennik sterownika z numerem linii. Format dziennika jest inny u każdego producenta (Apple: `ERROR: 0:12: ...`, NVIDIA: `0(12) : error ...`), więc kod nie próbuje go rozbierać. Zmienia jedną rzecz: w linii, która zaczyna się od numeru pliku w jednym z dwóch znanych kształtów (`numer(linia)` albo `numer:linia`), zastępuje ten numer nazwą pliku. Linię w każdym innym kształcie oddaje bez zmian, a przy więcej niż jednym pliku dopisuje na końcu legendę `Source files: ...`. Dziennika linkowania nie zmienia wcale.

6. **Dlaczego `Shader` nie da się kopiować?**
   Kopia miałaby ten sam identyfikator programu. Oba destruktory zawołałyby `glDeleteProgram` dla tego samego obiektu: drugi usuwałby coś, czego już nie ma, albo nowy obiekt, który dostał ten sam numer. Obiekt OpenGL ma jednego właściciela, więc opakowanie można tylko przenosić.

7. **Co robi konstruktor przenoszący i która linia jest w nim najważniejsza?**
   Przenosi ścieżki, napis błędu i listę wiązań bloków, kopiuje identyfikator programu i zeruje go w obiekcie źródłowym: `other.m_program = 0;`. Dzięki temu właściciel jest jeden, a destruktor obiektu źródłowego woła `glDeleteProgram(0)`, które OpenGL ignoruje.

8. **Czym przypisanie przenoszące różni się od konstruktora przenoszącego?**
   Obiekt docelowy już istnieje i może mieć program, więc najpierw trzeba go zwolnić (`glDeleteProgram(m_program)`), inaczej byłby wyciek. Trzeba też obsłużyć przypisanie do samego siebie (`this == &other`), bo bez tego obiekt usunąłby własny program i został z zerem.

9. **Dlaczego destruktor nie ma warunku `if (m_program != 0)`?**
   Specyfikacja OpenGL gwarantuje, że `glDeleteProgram(0)` jest po cichu ignorowane. Obiekt bez programu (nieudane wczytanie, obiekt po przeniesieniu) nie wymaga więc osobnej gałęzi.

10. **Co się stanie, gdy obiekt `Shader` przeżyje okno?**
    Destruktor zawoła `glDeleteProgram` bez bieżącego kontekstu OpenGL, co jest błędem (w praktyce awaria albo zignorowane wywołanie i wyciek). Dlatego `Shader` ma być polem klasy pochodnej od `core::Application`: pola giną przed klasą bazową, która posiada okno.

11. **Dlaczego `core::pathText` używa `u8string()`, a nie `string()`?**
    Na Windowsie `string()` zamienia ścieżkę na lokalną stronę kodową i rzuca wyjątek, gdy znaku nie da się w niej zapisać. Konstruktor `Shader` ma nie rzucać, więc tekst do komunikatu powstaje z UTF-8, który mieści każdą ścieżkę. Dodatkowo UTF-8 jest tym, czego oczekuje ImGui.

12. **Jakie cztery rodzaje błędu może zgłosić wczytanie programu i po czym je odróżnić?**
    Po pierwszej linii komunikatu. `Shader file cannot be opened:` (nie ma pliku shadera), `Shader include failed:` (preprocesor: brak pliku dołączanego, cykl, zły zapis, zła kolejność z `#version`), `Shader compilation failed:` (sterownik odrzucił tekst jednego shadera) i `Shader linking failed:` (dwa shadery do siebie nie pasują, komunikat wymienia obie ścieżki). Przy dwóch pierwszych nie powstaje żaden obiekt OpenGL.

13. **Co `compileShader` robi z tekstem pliku, zanim poda go sterownikowi?**
    Woła `expandIncludes`, które zastępuje każdą linię `#include "plik"` treścią tego pliku i otacza ją dyrektywami `#line`, tak żeby numery linii w błędach pozostały numerami z oryginalnych plików. Pliki dołączane czyta lambda `readInclude`, względem katalogu pliku shadera. Do `glShaderSource` idzie jeden napis: `source.text`.

14. **Po co klasie pole `m_blockBindings` i gdzie jest używane?**
    Punkt wiązania bloku uniformów jest zapisany w obiekcie programu, a `reload()` tworzy nowy obiekt, w którym każdy blok jest na punkcie 0. Pole pamięta prośby z `bindUniformBlock`. `reload()` powtarza je na nowym programie przed podmianą, a konstruktor przenoszący i przypisanie przenoszące przekazują listę razem z programem.

15. **Dlaczego w `reload()` pętla z `applyBlockBinding` dostaje `program`, a nie `m_program`?**
    `program` to nowy, właśnie zbudowany obiekt. `m_program` to w tej chwili jeszcze stary program, który ma swoje wiązania i zaraz zostanie usunięty. Wiązania trzeba ustawić na nowym, zanim zacznie być używany.

## 10. Źródła

- LearnOpenGL, rozdziały "Hello Triangle" (<https://learnopengl.com/Getting-started/Hello-Triangle>) i "Shaders" (<https://learnopengl.com/Getting-started/Shaders>): kompilacja, linkowanie, odczyt dziennika, własna klasa shadera.
- docs.gl (<https://docs.gl>), strony dla OpenGL 4: `glCreateShader`, `glShaderSource`, `glCompileShader`, `glGetShader` (`glGetShaderiv`), `glGetShaderInfoLog`, `glCreateProgram`, `glAttachShader`, `glDetachShader`, `glLinkProgram`, `glGetProgram` (`glGetProgramiv`), `glGetProgramInfoLog`, `glUseProgram`, `glDeleteShader`, `glDeleteProgram` (w tym zdanie o ignorowaniu wartości 0 i o programie będącym w użyciu).
- Khronos OpenGL Wiki: "Shader Compilation" (<https://www.khronos.org/opengl/wiki/Shader_Compilation>), "GLSL Object" (<https://www.khronos.org/opengl/wiki/GLSL_Object>).
- cppreference: semantyka przenoszenia (<https://en.cppreference.com/w/cpp/language/move_constructor>, <https://en.cppreference.com/w/cpp/language/move_assignment>), `std::filesystem::path::u8string`, `std::basic_ifstream`.
- docs.gl, strony dla OpenGL 4: `glGetUniformBlockIndex`, `glUniformBlockBinding` (punkt wiązania jako stan obiektu programu).
- Dokumenty w tym repozytorium: [`README.md`](README.md) (RAII i przenoszenie w `gfx`), [`shaders.md`](shaders.md), [`uniforms.md`](uniforms.md), [`uniform-buffers.md`](uniform-buffers.md) (bloki uniformów, `bindUniformBlock`, `applyBlockBinding`), [`shader-includes.md`](shader-includes.md) (preprocesor `#include`, dyrektywa `#line`, nazwy plików w błędach), [`shader-hot-reload.md`](shader-hot-reload.md), [`../core/gl-check.md`](../core/gl-check.md), [`../core/paths.md`](../core/paths.md).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o shaderach i języku GLSL).
- "OpenGL. Księga eksperta" (rozdziały o potoku programowalnym i shaderach).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): zasada "RAII dla obiektów GL".
