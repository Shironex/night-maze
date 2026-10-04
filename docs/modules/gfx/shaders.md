# Moduł gfx: shadery i programowalny potok

Kamień milowy: M1. Temat wykładu: 2 (Programowalny potok).
Kod: [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp), [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp), shadery [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert) i [`assets/shaders/basic.frag`](../../../assets/shaders/basic.frag), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), panel w [`src/debug/panels/ShadersPanel.cpp`](../../../src/debug/panels/ShadersPanel.cpp).

Część modułu `gfx`. Wstęp do całego modułu, zasada RAII dla obiektów OpenGL i semantyka przenoszenia są w [`README.md`](README.md). Druga część tematu 2, czyli skąd shader wierzchołków bierze dane (bufory i tablica wierzchołków), jest w [`buffers-vao.md`](buffers-vao.md). Ten dokument korzysta z makra `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)), z logowania ([`../core/window-context.md`](../core/window-context.md), sekcja 5.5) i ze ścieżek do assetów ([`../core/paths.md`](../core/paths.md)).

## 1. Po co to jest

Od OpenGL 3.2 w profilu Core nie da się narysować niczego bez shaderów: stary potok o stałej funkcjonalności (fixed function pipeline) został usunięty, a to, co dzieje się z każdym wierzchołkiem i każdym pikselem, opisują małe programy w języku GLSL, wykonywane przez kartę graficzną. Zanim narysuję pierwszy trójkąt, muszę więc umieć: wczytać tekst shadera z pliku, skompilować go, zlinkować dwa shadery w jeden program i dowiedzieć się, co poszło źle, gdy coś poszło źle.

Klasa `gfx::Shader` robi dokładnie to. Jest cienkim opakowaniem na **jeden obiekt programu OpenGL** zbudowany z dwóch plików: shadera wierzchołków i shadera fragmentów. Ma cztery cechy, z których każda odpowiada na konkretny problem:

| Cecha | Problem, który rozwiązuje |
|---|---|
| Shadery są plikami na dysku, nie napisami w kodzie C++ | Zmiana shadera nie wymaga kompilacji programu, a edytor koloruje składnię GLSL (zasada z PRD: "Shadery jako pliki") |
| `reload()` buduje nowy program i podmienia stary tylko przy sukcesie | Wczytywanie na żywo (hot reload): literówka w shaderze nie zamienia obrazu w czarny ekran, bo stary program działa dalej |
| Błąd jest logowany z nazwą pliku i pełnym tekstem sterownika, a do tego zapamiętany w `lastError()` | Błędów kompilacji GLSL nie widzi `glGetError` ani `GL_CHECK`. Bez własnego odczytu nie byłoby żadnej informacji |
| RAII i tylko przenoszenie (move-only) | Program OpenGL jest zwalniany dokładnie raz, automatycznie, bez ręcznego `glDeleteProgram` w kodzie gry |

Stan na dziś: `game::NightMazeApp` ma jeden obiekt `gfx::Shader`, zbudowany z plików `assets/shaders/basic.vert` i `assets/shaders/basic.frag`, i rysuje nim kostkę o sześciu kolorowych ścianach. Shader jest wczytywany przy starcie programu i ponownie po każdym naciśnięciu przycisku "Reload shaders" w panelu **Shaders** (sekcja 6): zmieniam plik `.frag`, naciskam przycisk i widzę efekt bez zamykania okna. Klasa ma jedną funkcję ustawiającą uniform, `setMat4` (sekcja 5.12): `NightMazeApp` wysyła nią co klatkę macierze modelu, widoku i rzutowania.

## 2. Teoria

### 2.1 Potok renderowania

**Potok renderowania** (rendering pipeline) to stała sekwencja etapów, przez którą przechodzą dane od tablicy liczb w pamięci do kolorów pikseli na ekranie. Dla OpenGL 4.1 i dwóch shaderów, których używam, wygląda tak:

```mermaid
flowchart TD
    Data["Dane wierzchołków<br/>pozycje, kolory, współrzędne tekstur w buforze"] --> VS["Shader wierzchołków (vertex shader)<br/>programowalny, raz na wierzchołek"]
    VS --> PA["Składanie prymitywów (primitive assembly)<br/>wierzchołki łączone w trójkąty, przycinanie, dzielenie przez w"]
    PA --> Rast["Rasteryzacja (rasterization)<br/>trójkąt zamieniany na fragmenty, interpolacja wartości"]
    Rast --> FS["Shader fragmentów (fragment shader)<br/>programowalny, raz na fragment"]
    FS --> Tests["Testy i mieszanie<br/>test nożycowy, szablonu, głębi, mieszanie kolorów (blending)"]
    Tests --> FB["Bufor ramki (framebuffer)<br/>kolory pikseli, głębia"]
```

| Etap | Kto go wykonuje | Co wchodzi | Co wychodzi |
|---|---|---|---|
| Dane wierzchołków | mój kod C++ | tablica liczb wysłana do bufora na karcie | atrybuty jednego wierzchołka (na przykład pozycja) |
| Shader wierzchołków | **mój program GLSL** | atrybuty jednego wierzchołka i uniformy | pozycja w przestrzeni przycięcia (`gl_Position`) i dowolne własne wartości dla dalszych etapów |
| Składanie prymitywów | OpenGL, bez mojego kodu | pozycje kolejnych wierzchołków | trójkąty (albo linie, punkty), przycięte do widocznego obszaru i przeliczone na współrzędne okna |
| Rasteryzacja | OpenGL, bez mojego kodu | jeden trójkąt | po jednym **fragmencie** na każdy piksel, który trójkąt zakrywa, z wartościami interpolowanymi między wierzchołkami |
| Shader fragmentów | **mój program GLSL** | interpolowane wartości jednego fragmentu i uniformy | kolor fragmentu |
| Testy i mieszanie | OpenGL, sterowane stanem (`glEnable`, `glDepthFunc`, `glBlendFunc`) | kolor i głębia fragmentu | decyzja, czy fragment trafia do bufora, i jak miesza się z tym, co już tam jest |
| Bufor ramki | OpenGL | zaakceptowane fragmenty | obraz, który `glfwSwapBuffers` pokazuje na ekranie |

**Programowalne** są dwa etapy z tej listy: shader wierzchołków i shader fragmentów. Pozostałe są stałe: mogę je tylko konfigurować przez stan OpenGL, ale nie mogę podmienić ich kodu. Stąd nazwa tematu: programowalny potok.

W pełnym potoku OpenGL 4.1 między shaderem wierzchołków a składaniem prymitywów są jeszcze dwa etapy opcjonalne: teselacja (tessellation) i shader geometrii (geometry shader). Na diagramie ich nie ma, bo `gfx::Shader` ich jeszcze nie obsługuje. Shader geometrii pojawi się przy temacie 9 (trawa).

**Fragment a piksel.** Fragment to "kandydat na piksel": dane dla jednego piksela pochodzące z jednego trójkąta. Na ten sam piksel może przypaść wiele fragmentów (z trójkątów leżących jeden za drugim), a o tym, który wygra, decyduje test głębi. Dlatego shader nazywa się shaderem fragmentów, a nie pikseli.

### 2.2 Co musi zrobić shader wierzchołków

Shader wierzchołków wykonuje się raz dla każdego wierzchołka i ma jeden obowiązek: zapisać do wbudowanej zmiennej `gl_Position` pozycję wierzchołka w **przestrzeni przycięcia** (clip space). To wektor czterech liczb `(x, y, z, w)`.

Po shaderze OpenGL sam wykonuje dwa kroki:

1. **Przycinanie** (clipping): zostaje to, co spełnia `-w <= x <= w`, `-w <= y <= w`, `-w <= z <= w`.
2. **Dzielenie perspektywiczne** (perspective divide): `x`, `y`, `z` są dzielone przez `w`. Wynik to **znormalizowane współrzędne urządzenia** (normalized device coordinates, NDC), w których widoczny obszar to sześcian od -1 do 1 na każdej osi: x w prawo, y w górę.

Potem `glViewport` zamienia NDC na piksele bufora ([`../core/window-context.md`](../core/window-context.md), sekcja 3.2).

Najprostszy shader wpisuje `w = 1`. Wtedy dzielenie niczego nie zmienia i współrzędne podane w shaderze są od razu współrzędnymi NDC: punkt `(0, 0)` to środek okna, `(-1, -1)` lewy dolny róg, `(1, 1)` prawy górny. Shader projektu mnoży pozycję przez trzy macierze, a ostatnia z nich, macierz rzutowania perspektywicznego, wpisuje do `w` odległość wierzchołka od kamery. Dzielenie przez takie `w` pomniejsza to, co daleko ([`../scene/transforms-camera.md`](../scene/transforms-camera.md), sekcja 2.9).

Poza `gl_Position` shader wierzchołków może przekazać dalej własne wartości (kolor, współrzędne tekstury) przez zmienne `out`.

### 2.3 Co robi shader fragmentów

Shader fragmentów wykonuje się raz dla każdego fragmentu, czyli znacznie częściej niż shader wierzchołków: trójkąt ma trzy wierzchołki, ale może zakrywać setki tysięcy pikseli. Jego wynikiem jest kolor: zmienna `out vec4` z czterema składowymi (czerwona, zielona, niebieska, alfa), każda w zakresie od 0 do 1.

Na wejściu dostaje wartości, które shader wierzchołków zapisał w swoich zmiennych `out`, ale **zinterpolowane**: rasteryzacja wylicza dla każdego fragmentu wartość pośrednią między trzema wierzchołkami trójkąta, proporcjonalnie do położenia fragmentu. Trójkąt z czerwonym, zielonym i niebieskim wierzchołkiem dostaje dzięki temu płynne przejście kolorów bez żadnego mojego kodu.

### 2.4 GLSL w pigułce

GLSL (OpenGL Shading Language) jest podobny do C, z typami wektorowymi i macierzowymi wbudowanymi w język.

| Element | Przykład | Znaczenie |
|---|---|---|
| Wersja | `#version 410 core` | Musi być **pierwszą linią** pliku. 410 to GLSL 4.10, czyli wersja odpowiadająca OpenGL 4.1. `core` wybiera profil Core |
| Wejście | `in vec3 aPosition;` | Zmienna wejściowa etapu. W shaderze wierzchołków to atrybut wierzchołka, w shaderze fragmentów interpolowana wartość z poprzedniego etapu |
| Wyjście | `out vec4 fragColor;` | Zmienna wyjściowa etapu. `out` z shadera wierzchołków łączy się z `in` o tej samej nazwie i typie w shaderze fragmentów |
| Położenie | `layout(location = 0) in vec3 aPosition;` | Numer atrybutu wierzchołka. Ten sam numer podaje kod C++, gdy opisuje układ danych w buforze. Bez `layout` numer przydziela linker i trzeba by o niego pytać |
| Uniform | `uniform mat4 uModel;` | Wartość ustawiana z C++ raz na rysowanie, taka sama dla wszystkich wierzchołków i fragmentów. Na przykład macierz, kolor światła, czas |
| Typy skalarne | `float`, `int`, `uint`, `bool` | Jak w C. Literał `1.0` jest typu `float` |
| Wektory | `vec2`, `vec3`, `vec4` | Wektory liczb `float`. Składowe: `.x .y .z .w` albo `.r .g .b .a`. Można je wybierać po kilka naraz: `color.rgb`, `position.xy` |
| Macierze | `mat3`, `mat4` | Macierze 3x3 i 4x4. Mnożenie `mat4 * vec4` jest wbudowane |
| Tekstury | `sampler2D`, `samplerCube` | Uchwyt do tekstury, zawsze jako uniform |
| Konstruktor | `vec4(aPosition, 1.0)` | Buduje wektor z mniejszych części: tu `vec3` i jedna liczba dają `vec4` |
| Funkcja główna | `void main() { ... }` | Punkt wejścia każdego shadera. Nic nie zwraca: wynik zapisuje do `gl_Position` albo do zmiennych `out` |

Nazwy i zachowanie typów są takie same jak w GLM po stronie C++ ([`../../libraries/glm.md`](../../libraries/glm.md)): `glm::vec3` to odpowiednik `vec3`, `glm::mat4` odpowiednik `mat4`.

Przedrostki w nazwach to tylko konwencja, a nie wymóg języka: `a` dla atrybutu (`aPosition`), `u` dla uniformu (`uModel`), `v` dla wartości przekazywanej z shadera wierzchołków do fragmentów (`vColor`).

### 2.5 Obiekt shadera a obiekt programu

OpenGL ma dwa różne rodzaje obiektów i łatwo je pomylić, bo potocznie oba nazywa się "shaderem":

| | Obiekt shadera (shader object) | Obiekt programu (program object) |
|---|---|---|
| Co reprezentuje | jeden etap: kod wierzchołków **albo** fragmentów | komplet etapów gotowy do rysowania |
| Tworzenie | `glCreateShader(typ)` | `glCreateProgram()` |
| Co się z nim robi | dostaje tekst źródłowy i jest **kompilowany** | dostaje dołączone obiekty shaderów i jest **linkowany** |
| Do czego służy potem | do niczego, to produkt pośredni | `glUseProgram` wybiera go do rysowania |
| Usuwanie | `glDeleteShader` | `glDeleteProgram` |

To ten sam podział co w C++: pliki `.cpp` kompiluje się osobno do plików obiektowych, a linker skleja je w program. Po zlinkowaniu pliki obiektowe nie są już potrzebne do uruchomienia programu. Tak samo obiekty shaderów nie są potrzebne po `glLinkProgram`, więc od razu je usuwam. Klasa `gfx::Shader` wbrew nazwie przechowuje więc identyfikator **programu**, a obiekty shaderów żyją tylko wewnątrz jednej funkcji pomocniczej.

Oba rodzaje obiektów są identyfikowane liczbą typu `GLuint`, nazywaną w dokumentacji OpenGL nazwą (name). Wartość 0 nigdy nie jest poprawnym obiektem: `glCreateShader` i `glCreateProgram` zwracają 0 przy niepowodzeniu, a `glUseProgram(0)` znaczy "żaden program".

### 2.6 Kompilacja a linkowanie

| Krok | Co sprawdza | Typowy błąd |
|---|---|---|
| Kompilacja (`glCompileShader`) | jeden shader w izolacji: składnię, typy, istnienie użytych zmiennych i funkcji | brak średnika, nieznana nazwa, `vec3` przypisany do `vec4`, zła albo brakująca linia `#version` |
| Linkowanie (`glLinkProgram`) | czy etapy pasują do siebie: każda zmienna `in` shadera fragmentów musi mieć zmienną `out` o tej samej nazwie i typie w shaderze wierzchołków, a każdy etap musi mieć `main` | shader fragmentów czyta `in vec3 vColor`, którego shader wierzchołków nie zapisuje |

Kompilator GLSL jest częścią **sterownika karty graficznej**, a nie mojego programu. Dlatego ten sam shader może dać różne komunikaty (a czasem różny wynik) na różnych kartach, a format tekstu błędu zależy od producenta:

| Sterownik | Przykładowa linia błędu |
|---|---|
| Apple (macOS) | `ERROR: 0:5: '}' : syntax error: syntax error` |
| NVIDIA | `0(5) : error C0000: syntax error, unexpected '}'` |

W formacie Apple `0:5` to numer napisu źródłowego (zawsze 0, bo podaję jeden napis) i numer linii. Pierwsza linia tabeli pochodzi z testu klasy na Macu (sekcja 5.11), druga jest przykładem formatu NVIDII. Ponieważ formatów jest wiele, klasa `Shader` nie próbuje tekstu sterownika czytać ani poprawiać: przekazuje go w całości.

Zarówno wynik kompilacji, jak i linkowania trzeba **odczytać samemu**: OpenGL nie zgłasza ich przez `glGetError` (sekcja 3.3).

### 2.7 Wczytywanie na żywo

**Wczytywanie na żywo** (hot reload) to podmiana shadera w działającym programie: zmieniam plik `.frag` w edytorze, zapisuję, każę programowi wczytać shadery ponownie i od następnej klatki widzę efekt, bez zamykania okna i bez kompilacji C++. Przy nauce shaderów to największe przyspieszenie pracy, jakie można mieć.

Żeby to było bezpieczne, podmiana musi być **wszystko albo nic**. Shader w trakcie edycji bardzo często się nie kompiluje. Gdybym najpierw usunął stary program, a potem próbował zbudować nowy, każda literówka kończyłaby się czarnym ekranem. Dlatego kolejność jest odwrotna: najpierw buduję nowy program obok starego, a stary usuwam dopiero wtedy, gdy nowy na pewno działa.

```mermaid
flowchart TD
    Start["reload()"] --> Build["buildProgram: wczytaj oba pliki, skompiluj, zlinkuj<br/>NOWY program, stary nietknięty"]
    Build --> Ok{"udało się?"}
    Ok -- nie --> Keep["zapisz błąd w m_lastError, logError<br/>m_program bez zmian, zwróć false"]
    Ok -- tak --> Swap["glDeleteProgram(stary), m_program = nowy<br/>wyczyść m_lastError, zwróć true"]
```

### 2.8 Uniformy

Shader ma dwa rodzaje danych wejściowych z C++. **Atrybut** (`in` w shaderze wierzchołków) ma inną wartość dla każdego wierzchołka i pochodzi z bufora. **Uniform** ma jedną wartość dla całego wywołania rysującego: wszystkie wierzchołki i wszystkie fragmenty widzą to samo. Typowe uniformy to macierze, kolor i pozycja światła, czas, numer tekstury.

| Własność | Znaczenie |
|---|---|
| należy do **programu** | wartość jest zapisana w obiekcie programu, nie w kontekście i nie w VAO. Dwa programy z uniformem o tej samej nazwie mają dwie osobne wartości |
| ma **położenie** (location) | liczbę całkowitą nadaną przy linkowaniu. O położenie pyta się po nazwie: `glGetUniformLocation(program, "uModel")` |
| jest **trwały** | raz ustawiona wartość zostaje w programie do następnego ustawienia. `glUseProgram` jej nie zeruje |
| po linkowaniu ma wartość **zero** | nowy program (także ten po `reload()`) zaczyna z samymi zerami. Macierz zerowa zamienia każdy wierzchołek w punkt `(0, 0, 0, 0)`, czyli nic nie widać |
| może być **nieaktywny** | uniform, który nie wpływa na wynik shadera, kompilator usuwa. Dla OpenGL taki uniform nie istnieje: jego położenie to -1 |

W OpenGL 4.1 są dwie rodziny funkcji ustawiających uniform:

| Funkcja | Do którego programu pisze | Uwagi |
|---|---|---|
| `glUniformMatrix4fv(location, ...)` i reszta `glUniform*` | do programu **bieżącego**, czyli wybranego ostatnim `glUseProgram` | klasyczna postać, ta z wykładu i z LearnOpenGL. Wymaga `use()` przed ustawieniem |
| `glProgramUniformMatrix4fv(program, location, ...)` i reszta `glProgramUniform*` | do programu podanego w pierwszym argumencie | w rdzeniu od OpenGL 4.1. Nie zależy od bieżącego programu |

Projekt używa pierwszej. Druga byłaby odporniejsza na pomyłkę "zapomniałem `use()`", ale wybrałem postać, którą pokazuje wykład i każdy poradnik, żeby kod dało się porównać z materiałami bez tłumaczenia. Zależność od bieżącego programu jest przy tym rzeczą, którą i tak trzeba rozumieć: tak samo działają bufory i VAO ([`buffers-vao.md`](buffers-vao.md), sekcja 2.2).

Położenie -1 jest w `glUniform*` celowo dozwolone: wywołanie nic nie robi i **nie zgłasza błędu**. Dzięki temu można bezkarnie ustawiać uniform, który kompilator akurat usunął. Ceną jest to, że literówka w nazwie wygląda dokładnie tak samo (pułapka 8).

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

Ustawienie uniformu typu `mat4`, co klatkę, po kroku 14:

| # | Wywołanie | Co robi |
|---|---|---|
| 16 | `glGetUniformLocation(program, name)` | Zwraca położenie aktywnego uniformu o podanej nazwie w zlinkowanym programie albo -1, gdy takiego nie ma. Nie wymaga, żeby program był bieżący |
| 17 | `glUniformMatrix4fv(location, count, transpose, value)` | Kopiuje `count` macierzy 4 x 4 (po 16 liczb `float`) spod wskaźnika `value` do uniformu **bieżącego** programu. `transpose` równe `GL_FALSE` znaczy: liczby leżą kolumnami, tak jak chce OpenGL. Położenie -1 jest ignorowane bez błędu. Gdy żaden program nie jest bieżący: `GL_INVALID_OPERATION` |

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

Projekt ma na dziś jedną parę shaderów w katalogu [`assets/shaders/`](../../../assets/shaders/). Nazwa `basic` jest celowo neutralna: to najprostsza para, która umie postawić obiekt w scenie (trzy macierze) i pokolorować go kolorem z wierzchołków.

### 4.1 `basic.vert`: shader wierzchołków

Cały plik [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert):

```glsl
#version 410 core
// Vertex shader: runs once for every vertex and decides where it lands on the screen.
// See docs/modules/gfx/shaders.md

// Inputs: the attributes of one vertex, read from the vertex buffer. The location numbers
// are the attribute indices that the C++ code uses when it describes the vertex layout.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the object
layout(location = 1) in vec3 aColor;    // red, green, blue, each from 0 to 1

// Uniforms: set from C++ (gfx::Shader::setMat4), the same for every vertex of one draw call.
// See docs/modules/scene/transforms-camera.md
uniform mat4 uModel;      // local space to world space: where the object stands
uniform mat4 uView;       // world space to view space: where the camera is and looks
uniform mat4 uProjection; // view space to clip space: perspective

// Output to the fragment shader. The rasterizer blends it between the three vertices of
// a triangle, so every fragment receives its own in-between color.
out vec3 vColor;

void main() {
    // gl_Position is the built-in output every vertex shader must write: the position in
    // clip space. The expression is read from right to left, the matrix nearest to the
    // vector is applied first:
    //   vec4(aPosition, 1.0)  the vertex in local space, w = 1 because it is a point
    //   uModel * ...          the vertex in world space
    //   uView * ...           the vertex in view space, as seen from the camera
    //   uProjection * ...     the vertex in clip space
    // After this shader the graphics card divides x, y and z by w (the distance from the
    // camera), which is what makes distant things small.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    // Pass the color through unchanged.
    vColor = aColor;
}
```

| Linia | Co robi |
|---|---|
| `#version 410 core` | GLSL 4.10, profil Core. Musi być pierwszą linią pliku, dlatego komentarz z opisem stoi dopiero pod nią |
| `layout(location = 0) in vec3 aPosition;` | Atrybut wierzchołka numer 0: trzy liczby `float`, pozycja w przestrzeni lokalnej obiektu. Numer 0 to stała `POSITION_ATTRIBUTE` w `NightMazeApp.cpp` |
| `layout(location = 1) in vec3 aColor;` | Atrybut numer 1: trzy liczby `float`, kolor. Numer 1 to stała `COLOR_ATTRIBUTE` |
| `uniform mat4 uModel;` | Macierz modelu: z przestrzeni lokalnej do przestrzeni świata. Ustawiana z C++ przez `setMat4("uModel", ...)`, wartość z `scene::Transform::matrix()` |
| `uniform mat4 uView;` | Macierz widoku: ze świata do przestrzeni kamery. Wartość z `scene::Camera::viewMatrix()` |
| `uniform mat4 uProjection;` | Macierz rzutowania: z przestrzeni kamery do przestrzeni przycięcia. Wartość z `scene::Camera::projectionMatrix()` |
| `out vec3 vColor;` | Wyjście do następnego etapu. Shader wierzchołków zapisuje tu kolor swojego wierzchołka, a rasteryzacja interpoluje go między trzema wierzchołkami trójkąta |
| `void main() {` | Funkcja wykonywana raz dla każdego wierzchołka wskazanego przez indeksy. Kostka ma 24 wierzchołki |
| `gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);` | Pozycja w przestrzeni przycięcia. Z `vec3` robię `vec4`, dopisując `w = 1` (bo to punkt), i mnożę kolejno przez trzy macierze. Wyrażenie czyta się **od prawej do lewej**: najpierw `uModel`, potem `uView`, na końcu `uProjection` |
| `vColor = aColor;` | Kolor przechodzi bez zmian |

Dlaczego mnożenie czyta się od prawej, czym są te trzy przestrzenie i co dokładnie dzieje się z jednym wierzchołkiem kostki na liczbach, opisuje [`../scene/transforms-camera.md`](../scene/transforms-camera.md) (sekcje 2.1, 4 i 5.10). Nazwy uniformów w shaderze muszą być identyczne z napisami w C++ (stałe `MODEL_UNIFORM`, `VIEW_UNIFORM`, `PROJECTION_UNIFORM` w `NightMazeApp.cpp`, sekcja 5.10).

**Trzy uniformy zamiast jednego.** Shader mógłby dostać jedną gotową macierz, iloczyn wszystkich trzech policzony w C++. Tak robi wiele prawdziwych rendererów: jedno mnożenie macierzy na wierzchołek zamiast trzech. W projekcie macierze są osobno celowo: każdą da się podmienić i obejrzeć skutek (ćwiczenia w [`../scene/transforms-camera.md`](../scene/transforms-camera.md), sekcja 8), a przy oświetleniu w M4 shader i tak będzie potrzebował pozycji w przestrzeni świata, czyli wyniku samego `uModel`.

### 4.2 `basic.frag`: shader fragmentów

Cały plik [`assets/shaders/basic.frag`](../../../assets/shaders/basic.frag):

```glsl
#version 410 core
// Fragment shader: runs once for every fragment (pixel candidate) and decides its color.
// See docs/modules/gfx/shaders.md

// Input from the vertex shader: same name and type as its "out vec3 vColor". The value is
// already interpolated for this fragment.
in vec3 vColor;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // Alpha 1 means fully opaque.
    fragColor = vec4(vColor, 1.0);
}
```

| Linia | Co robi |
|---|---|
| `in vec3 vColor;` | Wejście: ta sama nazwa i ten sam typ co `out vec3 vColor` w shaderze wierzchołków. Po tej nazwie linker łączy oba etapy. Wartość jest już zinterpolowana dla tego fragmentu |
| `out vec4 fragColor;` | Wyjście shadera: kolor fragmentu. Nazwa jest dowolna. Shader fragmentów z jednym wyjściem zapisuje je do pierwszego bufora koloru |
| `fragColor = vec4(vColor, 1.0);` | Kolor z trzech składowych i alfa równa 1, czyli pełne krycie |

### 4.3 Skąd biorą się kolory na ekranie

Dane wierzchołków w `NightMazeApp.cpp` dają każdej ścianie kostki cztery wierzchołki o **tym samym** kolorze: przednia jest czerwona, tylna zielona, lewa niebieska, prawa żółta, górna turkusowa, dolna purpurowa ([`buffers-vao.md`](buffers-vao.md), sekcja 5.7). Shader wierzchołków przepisuje kolor do `vColor`. Dla każdego piksela wewnątrz trójkąta rasteryzacja wylicza `vColor` jako średnią ważoną trzech wierzchołków, z wagami zależnymi od odległości. Średnia z trzech równych wartości to ta sama wartość, więc ściana jest jednolita. Interpolacja nadal działa, tylko nie ma czego mieszać: wystarczy dać jednemu wierzchołkowi inny kolor, żeby zobaczyć płynne przejście ([`buffers-vao.md`](buffers-vao.md), ćwiczenie 2). W żadnym z dwóch shaderów nie ma ani jednej linii, która to przejście liczy: robi je etap stały potoku.

Rozszerzenia plików (`.vert`, `.frag`) nie mają dla OpenGL żadnego znaczenia: o typie shadera decyduje stała podana do `glCreateShader`, a nie nazwa pliku. Rozszerzenia są dla ludzi i dla edytora, który według nich włącza kolorowanie składni GLSL ([`../../guides/project-structure.md`](../../guides/project-structure.md), sekcja 3.10).

Jak pliki z `assets/` trafiają obok programu, opisuje [`../core/paths.md`](../core/paths.md) (sekcja 5.8) i [`../../guides/project-structure.md`](../../guides/project-structure.md) (sekcja 3.1, blok 7).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) | klasa `gfx::Shader`: konstruktor, destruktor, zablokowane kopiowanie, przenoszenie, `reload`, `isValid`, `use`, `setMat4`, `lastError`, `vertexPath`, `fragmentPath`. Dołącza `<glad/gl.h>` (typ `GLuint`), `<glm/glm.hpp>` (typ `glm::mat4`), `<filesystem>` i `<string>` |
| [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp) | implementacja i sześć funkcji pomocniczych w anonimowej przestrzeni nazw: `readTextFile`, `shaderInfoLog`, `programInfoLog`, `compileShader`, `linkProgram`, `buildProgram` |
| [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert), [`basic.frag`](../../../assets/shaders/basic.frag) | jedyna para shaderów projektu (sekcja 4) |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | właściciel obiektu: pole `m_shader`, wczytanie w konstruktorze, `isValid()`, `use()` i trzy razy `setMat4()` w `onRender`, chroniony akcesor `shader()` (sekcja 5.10) |
| [`src/debug/panels/ShadersPanel.hpp`](../../../src/debug/panels/ShadersPanel.hpp), [`.cpp`](../../../src/debug/panels/ShadersPanel.cpp) | funkcja `debug::drawShadersPanel`: panel "Shaders" z przyciskiem "Reload shaders" (sekcja 6). Należy do programu `night_maze`, nie do biblioteki `engine` |

Oba pliki klasy są na liście źródeł biblioteki `engine` w [`CMakeLists.txt`](../../../CMakeLists.txt). Klasa zależy tylko od `core` (`GL_CHECK`, `logError`, `pathText`), GLAD, GLM (typ macierzy w `setMat4`) i biblioteki standardowej. Nie wie nic o panelu ani o ImGui.

```mermaid
flowchart TD
    Ctor["Shader(vertexPath, fragmentPath)"] --> Reload["reload()"]
    Reload --> Build["buildProgram(vertexPath, fragmentPath, error)"]
    Build --> CompV["compileShader(GL_VERTEX_SHADER, ...)"]
    Build --> CompF["compileShader(GL_FRAGMENT_SHADER, ...)"]
    Build --> Link["linkProgram(vertexShader, fragmentShader, infoLog)"]
    CompV --> Read["readTextFile(path, text)"]
    CompF --> Read
    CompV --> SLog["shaderInfoLog(shader)"]
    CompF --> SLog
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
};
```

| Element | Dlaczego tak |
|---|---|
| ścieżki jako `std::filesystem::path` | `Shader` nie wie nic o katalogu `assets/`. Pełną ścieżkę buduje wołający przez `core::assetPath` ([`../core/paths.md`](../core/paths.md)). Dzięki temu klasa nadaje się też do zadań laboratoryjnych, w których pliki leżą gdzie indziej |
| ścieżki są zapamiętane w polach | `reload()` nie ma parametrów: obiekt sam wie, z których plików powstał. Panel debug odczytuje je przez `vertexPath()` i `fragmentPath()` |
| `m_program = 0` | 0 to "nie ma programu". Jedno pole pełni rolę identyfikatora i flagi poprawności, bez osobnego `bool` |
| `m_lastError` | ten sam tekst, który trafił do konsoli, zostaje w obiekcie, żeby panel debug mógł go pokazać |
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

Oba akcesory ścieżek zwracają `const&` do pola: nic nie jest kopiowane, a wołający nie może ścieżki zmienić. Zwracają `std::filesystem::path`, a nie gotowy napis, bo to wołający wie, czego potrzebuje: panel bierze z nich samą nazwę pliku (`filename()`) do etykiety i całą ścieżkę do podpowiedzi (sekcja 6.1).

```cpp
void Shader::use() const {
    GL_CHECK(glUseProgram(m_program));
}
```

`use()` nie sprawdza `isValid()`. Dla obiektu bez programu wykona `glUseProgram(0)`, czyli "żaden program", a rysowanie w takim stanie nie daje określonego wyniku. Sprawdzenie należy do wołającego. `use()` jest `const`, bo nie zmienia obiektu C++, zmienia stan kontekstu OpenGL.

Szósta funkcja, `setMat4`, ustawia uniform. Opisuje ją sekcja 5.12.

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

```cpp
// Reads one shader file and compiles it. type is GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
// Returns the id of the shader object, or 0 on failure with the message in error.
GLuint compileShader(GLenum type, const std::filesystem::path& path, std::string& error) {
    std::string source;
    if (!readTextFile(path, source)) {
        error = "Shader file cannot be opened: " + core::pathText(path);
        return 0;
    }

    GLuint shader = 0;
    GL_CHECK(shader = glCreateShader(type));

    // glShaderSource takes an array of C strings. Here the array has one element.
    // nullptr as the array of lengths means that every string ends with a zero.
    const char* sourceText = source.c_str();
    GL_CHECK(glShaderSource(shader, 1, &sourceText, nullptr));
    GL_CHECK(glCompileShader(shader));

    // A compile error does not set an OpenGL error flag, so GL_CHECK cannot see it.
    // The result has to be asked for.
    GLint status = GL_FALSE;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status != GL_TRUE) {
        error = "Shader compilation failed: " + core::pathText(path) + "\n" + shaderInfoLog(shader);
        GL_CHECK(glDeleteShader(shader));
        return 0;
    }
    return shader;
}
```

| Fragment | Co robi i dlaczego |
|---|---|
| `if (!readTextFile(path, source))` | Pierwszy z trzech rodzajów błędu: pliku nie da się otworzyć. Żaden obiekt OpenGL jeszcze nie powstał, więc nie ma czego sprzątać |
| `GLuint shader = 0;` potem `GL_CHECK(shader = glCreateShader(type));` | Wywołanie zwracające wartość w `GL_CHECK`: przypisanie jest w środku makra, a zmienna jest zadeklarowana przed nim ([`../core/gl-check.md`](../core/gl-check.md), sekcja 5.2) |
| `const char* sourceText = source.c_str();` | `glShaderSource` chce **tablicy** wskaźników (`const GLchar* const*`). Mam jeden napis, więc robię zmienną ze wskaźnikiem i podaję jej adres: `&sourceText` to tablica o jednym elemencie. Nie da się napisać `&source.c_str()`, bo nie można wziąć adresu wartości tymczasowej |
| `glShaderSource(shader, 1, &sourceText, nullptr)` | `1` to liczba napisów. `nullptr` zamiast tablicy długości: napis kończy się zerem, co `c_str()` gwarantuje. OpenGL **kopiuje** tekst, więc `source` może zniknąć po tym wywołaniu |
| `GLint status = GL_FALSE;` | Wartość początkowa to "nie udało się". Gdyby `glGetShaderiv` samo zawiodło (na przykład `shader` równe 0), zmienna zostanie nietknięta i kod pójdzie ścieżką błędu |
| `if (status != GL_TRUE)` | Drugi rodzaj błędu: kompilacja. Komunikat to nazwa pliku, znak nowej linii i dziennik sterownika bez żadnych zmian |
| `GL_CHECK(glDeleteShader(shader)); return 0;` | Nieudany obiekt shadera usuwam od razu. Kolejność ma znaczenie: dziennik trzeba odczytać **przed** usunięciem obiektu |

Wartość zwracana 0 jest naturalnym sygnałem błędu, bo OpenGL nigdy nie nadaje obiektowi identyfikatora 0.

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
| `GL_LINK_STATUS` | Trzeci rodzaj błędu: linkowanie. Tak jak przy kompilacji, bez flagi `glGetError` |
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
| nie udał się shader wierzchołków | żaden (`compileShader` posprzątał po sobie) | nie powstał | plik albo kompilacja, ścieżka shadera wierzchołków |
| nie udał się shader fragmentów | shader wierzchołków usunięty tutaj, shader fragmentów przez `compileShader` | nie powstał | plik albo kompilacja, ścieżka shadera fragmentów |
| nie udało się linkowanie | oba usunięte tutaj | usunięty przez `linkProgram` | obie ścieżki i dziennik programu |
| sukces | oba usunięte tutaj | zwrócony wołającemu | pusty |

Żadne wyjście nie zostawia po sobie obiektu, którego nikt nie pamięta. To jest właśnie warunek z opisu `reload()`: "przy niepowodzeniu nowe obiekty są usuwane".

Przy błędzie w shaderze wierzchołków shader fragmentów nie jest w ogóle wczytywany. W konsoli pojawi się więc błąd tylko jednego pliku naraz. Po jego poprawieniu i ponownym `reload()` wyjdzie ewentualny błąd drugiego.

Komunikat o błędzie linkowania wymienia **oba pliki**, bo błąd linkowania z natury dotyczy pary: jeden shader czegoś oczekuje, a drugi tego nie dostarcza.

### 5.8 Konstruktor, `reload` i destruktor

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
      m_lastError(std::move(other.m_lastError)) {
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
    other.m_program = 0;
    return *this;
}
```

Przypisanie różni się od konstruktora jednym: obiekt po lewej stronie **już istnieje i może mieć własny program**. Trzy kroki:

1. **Przypisanie do samego siebie.** `this == &other` porównuje adresy. Bez tego warunku `shader = std::move(shader)` usunęłoby program, a potem "przejęło" identyfikator już nieistniejącego obiektu i na koniec wyzerowało go: obiekt zostałby bez programu.
2. **Zwolnienie własnego programu.** Bez tej linii stary identyfikator zostałby nadpisany i nikt nie zawołałby już dla niego `glDeleteProgram`: wyciek obiektu OpenGL.
3. **Przejęcie** pól `other` i wyzerowanie jego identyfikatora, tak jak w konstruktorze.

`return *this;` zwraca referencję do obiektu po lewej, jak każdy operator przypisania, żeby dało się pisać `a = b = c`.

### 5.10 Gdzie klasa jest używana

Właścicielem obiektu jest `game::NightMazeApp`: pięć miejsc poniżej. Szóstym jest chroniony akcesor, przez który obiekt trafia do panelu debug.

**Pole** w [`NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp):

```cpp
gfx::Shader m_shader;
```

Jako pole klasy pochodnej od `core::Application` obiekt powstaje po oknie i kontekście OpenGL, a ginie przed nimi ([`../core/README.md`](../core/README.md), sekcja 7).

**Nazwy plików** w anonimowej przestrzeni nazw [`NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp):

```cpp
// Shader files, relative to the assets directory.
constexpr const char* VERTEX_SHADER_FILE = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/basic.frag";
```

**Nazwy uniformów** w tym samym miejscu:

```cpp
// Names of the matrix uniforms: the same as the "uniform mat4" lines in basic.vert.
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";
```

To jedyny łącznik między kodem C++ a liniami `uniform mat4 ...` w `basic.vert`: zwykłe napisy. Kompilator C++ nie wie nic o shaderze, więc literówki nie wykryje (pułapka 8).

**Wczytanie** na liście inicjalizacyjnej konstruktora:

```cpp
m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)),
```

`core::assetPath` zamienia nazwę względną na pełną ścieżkę w katalogu `assets/` obok pliku wykonywalnego ([`../core/paths.md`](../core/paths.md)), więc shadery znajdują się niezależnie od katalogu roboczego. Wynik `assetPath` jest obiektem tymczasowym, który trafia do parametru konstruktora `Shader` przez przeniesienie (sekcja 5.8). Konstruktor `Shader` nie rzuca przy błędzie w shaderze. Wyjątek może natomiast rzucić samo `core::assetPath`, gdy system nie potrafi podać położenia programu: wtedy konstruktor aplikacji zostaje przerwany, a wyjątek łapie `catch` w `main` i program kończy się linią `[error] Fatal: ...`.

**Rysowanie** w `NightMazeApp::onRender`, po `glClear`:

```cpp
// A minimized window can have a framebuffer of size 0 x 0. The aspect ratio would
// then be 0 / 0, which is NaN (not a number): glm::perspective stops the program with
// an assert in a Debug build and returns a matrix with NaN in it in a Release build.
// There is nothing to draw in such a frame anyway.
if (framebuffer.height == 0) {
    return;
}

// Without a shader program there is nothing to draw with. The load error was logged
// once, when the shader was created, so the frame stays at the clear color.
if (!m_shader.isValid()) {
    return;
}

// Width divided by height of the same pixels the viewport covers. The casts make it
// a division of floats: 1280 / 720 as integers would be 1.
const float aspectRatio =
    static_cast<float>(framebuffer.width) / static_cast<float>(framebuffer.height);

// The uniforms belong to the program in use, so use() comes before setMat4.
m_shader.use();
m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());
m_shader.setMat4(VIEW_UNIFORM, m_camera.viewMatrix(m_camera.position));
m_shader.setMat4(PROJECTION_UNIFORM, m_camera.projectionMatrix(aspectRatio));

m_vertexArray.bind();
// Draws INDEX_COUNT indices from the element buffer recorded in the vertex array,
// every three of them form one triangle. GL_UNSIGNED_INT is the type of one index
// (GLuint). The last parameter has the type "pointer" for historical reasons, like in
// glVertexAttribPointer: with an element buffer bound it is the byte offset of the
// first index inside that buffer, and nullptr means offset 0, the start of the buffer.
GL_CHECK(glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr));
```

| Linia | Co robi i dlaczego |
|---|---|
| `if (framebuffer.height == 0) { return; }` | Zminimalizowane okno: nie ma pikseli do narysowania, a proporcji nie da się policzyć. Szczegóły: [`../scene/transforms-camera.md`](../scene/transforms-camera.md), sekcja 5.9 |
| `if (!m_shader.isValid()) { return; }` | Gdy shader się nie wczytał, rysowanie jest pomijane w całości. Klatka to wtedy samo tło, a panele debug działają normalnie, bo rysuje je `DebugNightMazeApp::onRender` po powrocie z tej funkcji. Błąd został wypisany **raz**, przez `reload()` wołane z konstruktora, a nie co klatkę |
| `const float aspectRatio = ...` | proporcje obrazu dla macierzy rzutowania, z rozmiaru framebuffera |
| `m_shader.use();` | `glUseProgram`: wybiera program dla następnych wywołań. Stoi **przed** `setMat4`, bo `glUniform*` pisze do programu bieżącego. Wołane co klatkę, bo backend ImGui ustawia przy rysowaniu paneli własny program |
| `m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());` | macierz modelu kostki trafia do `uModel` |
| `m_shader.setMat4(VIEW_UNIFORM, m_camera.viewMatrix(m_camera.position));` | macierz widoku trafia do `uView` |
| `m_shader.setMat4(PROJECTION_UNIFORM, m_camera.projectionMatrix(aspectRatio));` | macierz rzutowania trafia do `uProjection` |
| `m_vertexArray.bind();` | Wybiera opis danych wierzchołków i bufor indeksów ([`buffers-vao.md`](buffers-vao.md)) |
| `glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr)` | Uruchamia potok z sekcji 2.1 dla 36 indeksów, czyli 12 trójkątów ([`buffers-vao.md`](buffers-vao.md), sekcja 5.7) |

Macierze są wysyłane **co klatkę**, choć kostka i kamera dziś się nie ruszają. Powody są trzy: macierz rzutowania zależy od rozmiaru okna, który może się zmienić w każdej chwili. Po `reload()` nowy program ma wszystkie uniformy wyzerowane (sekcja 2.8), więc wartości wysłane raz przy starcie przepadłyby po pierwszym naciśnięciu "Reload shaders". A gdy kamera zacznie się poruszać, i tak będą inne w każdej klatce.

**Akcesor** w [`NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), obok `clearColor()`:

```cpp
/// Shader program of the cube, exposed so the debug UI can reload it live.
gfx::Shader& shader() { return m_shader; }
```

Zwraca referencję bez `const`, bo wołający ma móc zawołać `reload()`. Jest chroniony (`protected`), więc sięgnie po niego tylko klasa pochodna: `DebugNightMazeApp` w `main.cpp`, które przekazuje referencję do panelu przez `DebugContext` (sekcja 6.2). Gra nie dołącza przy tym niczego z `debug/`.

`reload()` jest więc wołane w dwóch miejscach: w konstruktorze `Shader` (pierwsze wczytanie) i w panelu "Shaders" po naciśnięciu przycisku. Sama gra go nie woła.

### 5.11 Jak to zostało sprawdzone

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

Te uruchomienia sprawdzały wyjście tekstowe, a nie obraz w oknie. Obraz sprawdzał wtedy osobny test z ukrytym oknem i `glReadPixels` ([`buffers-vao.md`](buffers-vao.md), sekcja 5.9).

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

**Kostka i uniformy.** Po zamianie trójkąta na kostkę sprawdziłem prawdziwy kod rysujący testem z ukrytym oknem, który wołał `NightMazeApp::onRender` i czytał obraz przez `glReadPixels` (pełna tabela: [`buffers-vao.md`](buffers-vao.md), sekcja 5.9). Wyniki dotyczące shadera i uniformów:

| Próba | Wynik |
|---|---|
| prawdziwe `basic.vert`, `basic.frag` i trzy wywołania `setMat4` | środek okna czerwony (ściana przednia), róg w kolorze tła, w klatce dokładnie trzy kolory ścian, `glGetError` czysty |
| literówka w nazwie uniformu w C++ (`"uModle"` zamiast `"uModel"`, w kopii pliku poza repozytorium) | **pusta klatka**: każdy piksel w kolorze tła. `glGetError` czysty, w konsoli żadnej linii `[error]`, program działa dalej |
| `gl_Position = vec4(aPosition, 1.0);` (bez macierzy) | zielony prostokąt na środku, połowa szerokości i wysokości okna. Zielona jest ściana **tylna**: bez macierzy rzutowania mniejsze z znaczy "bliżej". Trzy uniformy stają się nieaktywne, błędu brak |
| `uModel * uView * uProjection` (odwrotna kolejność) | pusta klatka, błędu brak |
| `uView * uModel` (bez rzutowania) | pusta klatka: kostka ma w przestrzeni widoku z od -3,9 do -2,1, czyli poza zakresem od -1 do 1, i jest w całości przycinana |
| zamienione `location = 0` i `location = 1` | pusta klatka, błędu brak |

Program `night_maze` uruchomiony na około 3 sekundy z katalogu repozytorium wypisał dwie linie `[info]` i żadnej linii `[error]`.

Na Windowsie klasa i panel nie były jeszcze kompilowane ani uruchamiane.

### 5.12 Uniformy: `setMat4`

Deklaracja w [`Shader.hpp`](../../../src/gfx/Shader.hpp):

```cpp
/// Sets the uniform variable of type mat4 called name to matrix (glUniformMatrix4fv).
/// The program must be in use: call use() first, because OpenGL writes the value into
/// the program that is current. A name the program does not have (a typo, or a uniform
/// the compiler removed because the shader never reads it) is ignored without an error.
void setMat4(const char* name, const glm::mat4& matrix) const;
```

Implementacja w [`Shader.cpp`](../../../src/gfx/Shader.cpp):

```cpp
void Shader::setMat4(const char* name, const glm::mat4& matrix) const {
    // The location is the number of the uniform inside this program. It is looked up on
    // every call: a few lookups per frame cost nothing, and there is no cache that could
    // go stale after reload(). -1 means the program has no active uniform with this name.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // 1: one matrix. GL_FALSE: do not transpose, GLM stores a matrix column by column,
    // which is the order OpenGL expects. value_ptr gives the address of its 16 floats.
    // OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}
```

| Element | Co robi i dlaczego |
|---|---|
| `const char* name` | nazwa uniformu dokładnie taka jak w shaderze, na przykład `"uModel"`. Zwykły napis C, bo taki przyjmuje `glGetUniformLocation` i taki jest literał w kodzie wołającym |
| `const glm::mat4& matrix` | referencja do stałej: 64 bajty macierzy nie są kopiowane, a funkcja nie może jej zmienić. Przyjmuje też wartość tymczasową, na przykład wynik `m_camera.projectionMatrix(aspectRatio)` |
| `const` na końcu deklaracji | funkcja nie zmienia obiektu C++ (`m_program` zostaje ten sam). Zmienia stan obiektu programu po stronie OpenGL, tak jak `use()` zmienia stan kontekstu |
| `GLint location = -1;` | położenie jest liczbą ze znakiem, bo -1 znaczy "nie ma". Wartość początkowa -1 zostaje, gdyby wywołanie się nie powiodło |
| `GL_CHECK(location = glGetUniformLocation(m_program, name));` | pytam sterownik o położenie uniformu w **moim** programie. Wywołanie zwraca wartość, więc przypisanie stoi wewnątrz makra ([`../core/gl-check.md`](../core/gl-check.md)) |
| `1` | liczba macierzy. Więcej niż 1 tylko dla uniformu będącego tablicą |
| `GL_FALSE` | parametr `transpose`: nie transponuj. GLM trzyma macierz kolumnami (column-major), czyli w tym samym układzie, w jakim czyta ją OpenGL ([`../../libraries/glm.md`](../../libraries/glm.md), sekcje 3.3 i 3.9). `GL_TRUE` zamieniłoby wiersze z kolumnami i zepsuło przekształcenie |
| `glm::value_ptr(matrix)` | wskaźnik `const float*` na pierwszą z 16 liczb macierzy. OpenGL to API w C i nie zna typu `glm::mat4`. Funkcja jest w `<glm/gtc/type_ptr.hpp>`, dołączanym tylko w `Shader.cpp` |

Trzy decyzje, które trzeba umieć obronić:

**Położenie jest wyszukiwane przy każdym wywołaniu, bez pamięci podręcznej.** Typowa klasa shadera trzyma mapę "nazwa na położenie", żeby nie pytać sterownika co klatkę. Tu jej nie ma: trzy wyszukiwania na klatkę to koszt niemierzalny, a każda pamięć podręczna musiałaby być czyszczona w `reload()`, bo nowy program może nadać uniformom inne położenia. Mapa, o której czyszczeniu można zapomnieć, to gorsza wymiana niż trzy wywołania.

**Program musi być w użyciu.** `glUniformMatrix4fv` nie przyjmuje identyfikatora programu: pisze do programu bieżącego (sekcja 2.8). `setMat4` **nie woła** `use()` samo. Gdyby wołało, ustawienie uniformu po cichu zmieniałoby bieżący program, a trzy macierze oznaczałyby trzy zbędne `glUseProgram`. Kolejność "najpierw `use()`, potem `setMat4`" należy do wołającego i jest zapisana w komentarzu Doxygen. Jej złamanie nie zawsze daje błąd: macierz trafia wtedy do innego programu (pułapka 20).

**Nieznana nazwa jest ignorowana po cichu.** Dla nazwy, której program nie ma, `glGetUniformLocation` zwraca -1, a `glUniformMatrix4fv` z położeniem -1 nic nie robi i nie zgłasza błędu. `setMat4` tego nie sprawdza i niczego nie loguje. Rozważałem `logWarn` przy -1 i odrzuciłem z dwóch powodów. Funkcja jest wołana co klatkę, więc ostrzeżenie pojawiałoby się 60 razy na sekundę i zalało konsolę. Po drugie -1 nie zawsze jest pomyłką: uniform usunięty przez kompilator jako nieużywany (na przykład w trakcie eksperymentu z shaderem, ćwiczenie 7) też ma położenie -1, a ustawianie go jest poprawne. Skutek trzeba po prostu znać: literówka w nazwie daje pusty ekran bez żadnego komunikatu (pułapka 8).

Klasa ma tylko `setMat4`. Uniformy innych typów (`vec3`, `float`, `int`) dojdą wtedy, gdy shader będzie ich potrzebował.

## 6. Panel ImGui

Panel **Shaders** (kod: [`ShadersPanel.cpp`](../../../src/debug/panels/ShadersPanel.cpp)) jest pokazem tematu 2 na obronie: przycisk "Reload shaders" wczytuje shadery ponownie w działającym programie. Panel pokazuje jeden program, ten z pola `NightMazeApp::m_shader`. Jak nakładka z panelami jest wpięta w program, opisuje [`../debug-ui.md`](../debug-ui.md).

| Element | Rodzaj | Skąd wartość | Czego uczy |
|---|---|---|---|
| `Vertex: basic.vert`, `Fragment: basic.frag` | odczyt | `vertexPath()` i `fragmentPath()`, zamienione na tekst przez `core::pathText` | Program powstaje z dwóch plików, po jednym na etap. Po najechaniu kursorem na linię pojawia się podpowiedź (tooltip) z pełną ścieżką: widać w niej, że program czyta pliki z katalogu `assets` obok pliku wykonywalnego |
| `Program: valid` albo `Program: not valid` | odczyt | `isValid()` | Czy jest zlinkowany program, którym można rysować. Po nieudanym przeładowaniu zostaje `valid`, bo działa poprzedni program |
| `Reload shaders` | przycisk | woła `reload()` | Wczytywanie na żywo (sekcja 2.7): pliki są czytane, kompilowane i linkowane od nowa, bez zamykania okna i bez kompilacji C++ |
| `Last load: OK` albo `Last load: failed` i czerwony tekst pod spodem | odczyt | `lastError()` | Błąd kompilacji GLSL nie jest błędem OpenGL (sekcja 3.3): jedyną informacją jest tekst sterownika, który klasa zapamiętała. Ten sam tekst jest w konsoli jako linia `[error]` |

Dwie ostatnie linie odpowiadają na dwa różne pytania (tabela stanów w sekcji 5.8). `Program: valid` razem z czerwonym błędem to nie sprzeczność, tylko dokładnie ten stan, dla którego `reload()` zostało tak napisane: nowe pliki się nie kompilują, a obraz rysuje poprzedni program.

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
2. **`reload()` nie zmienia stanu, na którym polega backend.** Tworzy obiekty shaderów i program, kompiluje, linkuje i usuwa (sekcja 3.1). Nie woła `glUseProgram`, nie wiąże buforów, tekstur ani VAO.
3. **Backend ustawia własny stan od zera.** `RenderDrawData` zapamiętuje bieżący stan, potem samo woła `glUseProgram` dla swojego programu, wiąże swoje VAO i bufory. Nie zakłada, że ktoś zostawił mu poprawny program.
4. **Backend jest przygotowany na usunięty program.** Po udanym przeładowaniu stary program jest jeszcze bieżący (gra ustawiła go w tej klatce), więc `glDeleteProgram` tylko oznacza go do usunięcia (sekcja 7, pułapka 4). Backend zapamiętuje go jako "poprzedni program", przełącza się na własny i w tej chwili stary program znika naprawdę. Na końcu backend przywraca poprzedni program tylko wtedy, gdy ten jeszcze istnieje. W źródle jest to linia `if (last_program == 0 || glIsProgram(last_program)) glUseProgram(last_program);` z komentarzem, że bez tego sprawdzenia przywrócenie programu oczekującego na usunięcie dałoby błąd OpenGL.
5. **Gra nie zostaje z usuniętym programem.** Po takiej klatce bieżącym programem jest program ImGui. W następnej klatce `NightMazeApp::onRender` woła `m_shader.use()` przed ustawieniem macierzy i przed `glDrawElements`, a `use()` i `setMat4()` czytają aktualne `m_program`, czyli już nowy identyfikator. Nowy program zaczyna z wyzerowanymi uniformami, ale trzy macierze są wysyłane co klatkę, więc dostaje je przed pierwszym rysowaniem. Nikt poza klasą `Shader` nie przechowuje identyfikatora programu.

Przy nieudanym przeładowaniu nic z tego nie zachodzi: `m_program` się nie zmienia, żaden używany program nie jest usuwany, a backend przywraca ten sam program co zwykle.

Punkty 4 i 5 potwierdził test z sekcji 5.11: po kliknięciu `glIsProgram` dla starego identyfikatora zwraca fałsz, a `glGetError` po żadnej klatce nie zgłasza błędu.

### 6.4 Pokaz na obronie krok po kroku

Wersja dla macOS, gdzie `build/debug/assets` jest dowiązaniem do katalogu w repozytorium. Różnica na Windowsie jest w sekcji 6.5.

1. Uruchom program (`make run`). W panelu Shaders: `Vertex: basic.vert`, `Fragment: basic.frag`, `Program: valid`, `Last load: OK`. Najedź kursorem na linię `Fragment`, żeby pokazać pełną ścieżkę.
2. Nie zamykając programu, otwórz w edytorze [`assets/shaders/basic.frag`](../../../assets/shaders/basic.frag) i zamień linię `fragColor = vec4(vColor, 1.0);` na `fragColor = vec4(1.0, 0.5, 0.0, 1.0);`. Zapisz plik. **Obraz się nie zmienia**: program nie obserwuje dysku.
3. Naciśnij `Reload shaders`. Cała kostka staje się jednolicie pomarańczowa: ściany przestają się od siebie różnić i zostaje sama sylwetka. W panelu zostaje `Last load: OK`. Kod C++ nie był kompilowany, okno nie było zamykane.
4. Wprowadź literówkę: usuń średnik na końcu zmienionej linii. Zapisz i naciśnij `Reload shaders`.
5. W panelu pojawia się `Last load: failed` i czerwony tekst: `Shader compilation failed: <ścieżka>/basic.frag`, a pod nim linia sterownika (na Macu `ERROR: 0:15: '}' : syntax error: syntax error`). Ten sam tekst jest w konsoli jako linia `[error]`. Linia `Program` nadal pokazuje `valid`, a **kostka jest nadal pomarańczowa**: rysuje ją poprzedni program.
6. Przywróć plik do pierwotnej postaci (`git checkout assets/shaders`), naciśnij `Reload shaders`. Wraca `Last load: OK` i kostka z trzema widocznymi ścianami: czerwoną, niebieską i turkusową.

Co przy tym mówię: krok 2 pokazuje, że shader jest plikiem czytanym w czasie działania (zasada "Shadery jako pliki"). Krok 3 to cały potok budowania programu z sekcji 3.1 wykonany na żądanie. Kostka nie znika ani na jedną klatkę, choć nowy program ma wyzerowane uniformy: macierze są wysyłane co klatkę (sekcja 5.10). Krok 5 pokazuje dwie rzeczy naraz: że błąd GLSL trzeba odczytać samemu z dziennika sterownika (sekcja 3.3) i że `reload()` jest operacją "wszystko albo nic" (sekcja 2.7). Numer linii 15 przy średniku brakującym w linii 14 tłumaczy pułapka 11.

### 6.5 Różnica na Windowsie

Na Windowsie katalog `assets` obok programu jest **kopią**, a nie dowiązaniem ([`../core/paths.md`](../core/paths.md), sekcja 5.8). Przycisk czyta kopię, więc po zapisaniu pliku w `assets\shaders\` trzeba najpierw ją odświeżyć:

1. zapisz plik shadera w repozytorium,
2. w drugim terminalu wykonaj `cmake --build --preset debug`,
3. naciśnij `Reload shaders`.

Oczekuję, że krok 2 da się wykonać przy działającym programie: gdy nie zmienił się żaden plik C++, budowanie nie linkuje `night_maze.exe` od nowa, tylko kopiuje katalog `assets`. Nie było to jeszcze sprawdzone na PC (punkt na liście kontrolnej w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 11).

Bez kroku 2 panel pokaże `Last load: OK`, a obraz się nie zmieni, bo program wczytał poprawnie stary plik. Podpowiedź z pełną ścieżką w panelu pokazuje, który plik jest czytany. Szczegóły i wariant dla Visual Studio: [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 7. Na Windowsie panel nie był jeszcze uruchamiany.

## 7. Pułapki

1. **Błędy kompilacji i linkowania są niewidoczne dla `glGetError`.** `GL_CHECK(glCompileShader(shader))` nigdy nie zgłosi błędu składni GLSL. Jedynym źródłem informacji jest `GL_COMPILE_STATUS`, `GL_LINK_STATUS` i dziennik (sekcja 3.3). Program, który ich nie czyta, po prostu niczego nie rysuje.
2. **Brak `#version` albo `#version` nie w pierwszej linii.** Bez tej dyrektywy kompilator przyjmuje GLSL 1.10, w którym nie ma `layout`, `in` ani `out` w dzisiejszym znaczeniu. Sterownik Apple zgłasza wprost `#version required and missing`. Przed `#version` mogą stać tylko komentarze i białe znaki, żaden kod.
3. **Wersja GLSL z poradnika.** LearnOpenGL używa `#version 330 core`: na Macu to się kompiluje (kontekst 4.1 przyjmuje też starsze wersje Core), ale nie ma wtedy funkcji GLSL 4.x, więc w projekcie piszę `#version 410 core`. Poradniki dla Windowsa używają często `#version 420`, `430`, `450` albo `460`: te na macOS **nie kompilują się wcale** (`version '460' is not supported`), bo macOS kończy się na OpenGL 4.1. Na PC z nowszym sterownikiem taki shader zadziała, więc błąd wychodzi dopiero po przeniesieniu kodu na Maca.
4. **Usunięcie programu, który jest w użyciu.** `glDeleteProgram` dla bieżącego programu nie usuwa go od razu, tylko oznacza do usunięcia. Program znika, gdy przestanie być bieżący. Po udanym `reload()` stary program jest więc jeszcze "bieżący" do najbliższego `glUseProgram` z innym programem. W programie `night_maze` jest nim rysowanie paneli przez backend ImGui jeszcze w tej samej klatce, a w następnej `use()` ustawia nowy program (sekcja 6.3). W pętli gry `use()` jest wołane co klatkę, więc niczego nie trzeba robić. Błędem byłoby zapamiętać identyfikator programu poza klasą i używać go po `reload()`.
5. **Destruktor bez kontekstu.** `~Shader` woła `glDeleteProgram`, a każda funkcja `gl*` wymaga bieżącego kontekstu. Obiekt `Shader` żyjący dłużej niż okno (zmienna globalna, zmienna lokalna w `main` zadeklarowana przed aplikacją) wywoła OpenGL po zniszczeniu kontekstu. Poprawne miejsce to pole klasy pochodnej od `core::Application` ([`../core/README.md`](../core/README.md), sekcja 7). Z tego samego powodu obiektu nie można utworzyć **przed** powstaniem okna.
6. **Kopiowanie opakowania.** Gdyby kopiowanie nie było zablokowane, `Shader b = a;` dałoby dwa obiekty z tym samym identyfikatorem i drugi destruktor usuwałby już usunięty program (albo, co gorsza, nowy obiekt, który dostał ten sam numer). Dzięki `= delete` taka linia się nie kompiluje. Typowa sytuacja, w której to wychodzi: przekazanie `Shader` do funkcji przez wartość. Przekazuję przez `const Shader&`.
7. **Użycie obiektu po przeniesieniu.** Po `Shader b = std::move(a);` obiekt `a` ma `isValid() == false`. `a.use()` ustawi wtedy program 0.
8. **Położenie -1: literówka w nazwie uniformu albo uniform usunięty przez kompilator.** `glGetUniformLocation` zwraca -1 w dwóch sytuacjach, których nie da się od siebie odróżnić. Pierwsza to nazwa, której w shaderze nie ma: `setMat4("uModle", ...)`. Druga to uniform, który kompilator GLSL usunął, bo nie wpływa na wynik shadera (zadeklarowany, ale nieużyty, albo użyty tylko w obliczeniu, którego wynik jest potem ignorowany). Ustawianie uniformu o położeniu -1 jest po cichu ignorowane: nie ma błędu OpenGL, nie ma linii w konsoli, `setMat4` też niczego nie loguje (sekcja 5.12). Wartość po prostu "nie dochodzi", a uniform zostaje z zerami. Zmierzone w teście (sekcja 5.11): literówka w `"uModel"` daje macierz zerową w shaderze, wszystkie wierzchołki w jednym punkcie i **pusty ekran bez żadnego komunikatu**. Gdy kostka znika po zmianie w kodzie C++, pierwszą rzeczą do sprawdzenia są trzy napisy z nazwami uniformów.
9. **`use()` bez sprawdzenia `isValid()`.** Dla obiektu bez programu `use()` ustawia program 0 i rysowanie nie daje określonego wyniku (zwykle nic nie widać). Po nieudanym wczytaniu w konstruktorze trzeba albo pominąć rysowanie, albo poprawić plik i zawołać `reload()`.
10. **Dziennik czytany po usunięciu obiektu.** `glGetShaderInfoLog` dla usuniętego shadera zwraca błąd OpenGL zamiast tekstu. W `compileShader` i `linkProgram` dziennik jest odczytywany przed `glDeleteShader` i `glDeleteProgram`.
11. **Numer linii w błędzie wskazuje za daleko.** Brak średnika w linii 4 sterownik zgłasza w linii 5 (sekcja 5.11). Trzeba patrzeć też linię wyżej.
12. **Shader pod złym typem.** `glCreateShader(GL_VERTEX_SHADER)` z tekstem shadera fragmentów zwykle kończy się mylącym błędem kompilacji albo linkowania. O typie decyduje kolejność argumentów konstruktora `Shader` (najpierw wierzchołków, potem fragmentów), a nie rozszerzenie pliku.
13. **Stary obraz po zmianie pliku.** Zapisanie pliku shadera samo niczego nie zmienia w działającym programie: `Shader` nie obserwuje dysku. Trzeba zawołać `reload()`, czyli nacisnąć `Reload shaders` w panelu Shaders (albo uruchomić program ponownie).
14. **Windows: program czyta kopię shaderów.** Na macOS katalog `assets` obok programu jest dowiązaniem do katalogu w repozytorium, więc program widzi plik zaraz po zapisaniu. Na Windowsie jest to **kopia**, robiona od nowa przy każdym budowaniu: po zmianie pliku w `assets\shaders\` trzeba najpierw zbudować (`cmake --build --preset debug`), a dopiero potem nacisnąć `Reload shaders` ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 7). Objaw pominięcia budowania: panel pokazuje `Last load: OK`, a obraz się nie zmienia (sekcja 6.5).
15. **Niezgodne nazwy `out` i `in`.** `out vec3 vColor` w `basic.vert` i `in vec3 vColor` w `basic.frag` są łączone po nazwie. Literówka w jednej z nich nie jest błędem kompilacji żadnego z plików, tylko błędem **linkowania**.
16. **Tekst sterownika jako napis formatujący.** `ImGui::TextWrapped(shader.lastError().c_str())` traktuje komunikat jak format `printf`: znak `%` w tekście sterownika kazałby funkcji czytać nieistniejące argumenty. Poprawnie: `ImGui::TextWrapped("%s", shader.lastError().c_str())` albo `ImGui::TextUnformatted` (sekcja 6.1).
17. **`Program: valid` i czerwony błąd jednocześnie.** To nie jest błąd panelu. `isValid()` mówi, czy jest czym rysować, a `lastError()`, czy ostatnie wczytanie się udało. Po nieudanym przeładowaniu oba są prawdziwe naraz: rysuje poprzedni program (sekcja 5.8).
18. **`Last load: OK`, a obraz bez zmian.** Program wczytał poprawnie plik, tylko nie ten, który przed chwilą zmieniłem. Na Windowsie to nieodświeżona kopia `assets` (pułapka 14). Na obu systemach: zmiana zapisana w innym pliku niż ten z podpowiedzi w panelu albo niezapisany plik w edytorze.
19. **Błąd tylko jednego pliku naraz.** Gdy zepsute są oba pliki, panel pokazuje błąd shadera wierzchołków, bo `buildProgram` kończy pracę na pierwszym niepowodzeniu (sekcja 5.7). Błąd shadera fragmentów pojawi się po naprawieniu pierwszego i kolejnym kliknięciu.
20. **`setMat4` przed `use()`.** `glUniform*` pisze do programu bieżącego. Bez `use()` macierz trafia do programu, który akurat jest bieżący, czyli do tego, który ktoś wybrał ostatnio. Jeśli tamten program nie ma uniformu pod tym położeniem albo ma uniform innego typu, OpenGL zgłasza `GL_INVALID_OPERATION` i `GL_CHECK` to wypisze. Jeśli typ się zgadza, błędu nie ma, a zepsuty zostaje cudzy shader. Gdy bieżącego programu nie ma wcale, błąd jest zawsze.
21. **Uniformy po `reload()`.** Nowy program zaczyna z samymi zerami (sekcja 2.8). Kod, który ustawia uniform raz, przy starcie, traci tę wartość po pierwszym przeładowaniu shadera. W projekcie macierze są wysyłane co klatkę, więc problemu nie ma, ale każdy uniform ustawiany "raz" trzeba po `reload()` ustawić ponownie.
22. **`GL_TRUE` jako `transpose`.** Macierz z GLM jest już w układzie kolumnowym. `GL_TRUE` transponuje ją: przesunięcie ląduje w ostatnim wierszu zamiast w ostatniej kolumnie i obraz znika albo jest zdeformowany.
23. **Zła kolejność mnożenia w shaderze.** `uModel * uView * uProjection * vec4(...)` kompiluje się bez ostrzeżeń i daje pusty ekran (zmierzone, sekcja 5.11). Macierz najbliżej wektora działa pierwsza, więc poprawna kolejność to `uProjection * uView * uModel`.

## 8. Ćwiczenia

Program może działać przez cały czas: po każdej zmianie pliku `.vert` albo `.frag` zapisz plik i naciśnij `Reload shaders` w panelu Shaders. Kompilacja C++ nie jest potrzebna. Na macOS przycisk od razu widzi zmianę, bo `build/debug/assets` jest dowiązaniem do katalogu w repozytorium. Na Windowsie przed naciśnięciem przycisku trzeba wykonać `cmake --build --preset debug`, które odświeża kopię shaderów obok programu. Ćwiczenia 3 i 11 dotyczą błędu **przy starcie**, więc tam program trzeba uruchomić od nowa. Po każdym ćwiczeniu przywróć plik (`git checkout assets/shaders`) i naciśnij przycisk jeszcze raz.

1. **Potok na kartce.** Narysuj z pamięci diagram z sekcji 2.1. Zaznacz etapy programowalne. Trzy widoczne ściany kostki zakrywają w oknie 1280 x 720 około 73 tysięcy punktów (na ekranie Retina cztery razy więcej pikseli: zmierzone 291 620). Kostka ma 24 wierzchołki i 36 indeksów. Ile razy na klatkę wykonuje się `main` z `basic.vert`? Ile razy co najmniej wykonuje się `main` z `basic.frag` i dlaczego może więcej (pomyśl o ścianach tylnych i teście głębi)?
2. **Stały kolor.** W `basic.frag` zamień `vec4(vColor, 1.0)` na `vec4(1.0, 0.5, 0.2, 1.0)`. Naciśnij `Reload shaders`. Jak wygląda kostka i dlaczego nie widać już krawędzi między ścianami? Czy shader nadal się linkuje, mimo że `vColor` nie jest już używane?
3. **Literówka przy starcie.** Zamknij program, usuń średnik po `fragColor = vec4(vColor, 1.0)` w `basic.frag` i uruchom program od nowa. Przeczytaj linię `[error]`: która część pochodzi z `Shader.cpp`, a która ze sterownika? Którą linię wskazuje sterownik i dlaczego nie tę ze średnikiem? Co widać w oknie, co pokazuje linia `Program` w panelu Shaders i czy panele działają? Wskaż w `NightMazeApp::onRender` linię, dzięki której program się nie wysypał. Na koniec, nie zamykając programu, przywróć średnik i naciśnij `Reload shaders`: co się zmieniło w oknie i w panelu?
4. **Błąd linkowania.** W `basic.frag` zmień nazwę `vColor` na `vColour` w obu liniach, w których występuje. Naciśnij `Reload shaders`. Czym różni się komunikat od poprzedniego i dlaczego wymienia oba pliki?
5. **Zamienione numery atrybutów.** W `basic.vert` zamień `location = 0` z `location = 1` (pozycja dostaje 1, kolor 0). Naciśnij `Reload shaders`. Shader czyta teraz kolory jako pozycje, a pozycje jako kolory. Kostka znika. Wyjaśnij to, patrząc na dane: gdzie lądują cztery wierzchołki jednej ściany, skoro mają ten sam kolor? Dlaczego nie ma żadnego błędu w konsoli?
6. **Pozycja jako kolor.** W `basic.vert` zamień `vColor = aColor;` na `vColor = aPosition + 0.5;`. Ściany przestały być jednolite: dlaczego właśnie teraz widać interpolację? Jaki kolor ma róg (0,5, 0,5, 0,5), a jaki róg (-0,5, -0,5, -0,5)? Co by było bez `+ 0.5`?
7. **Bez macierzy.** W `basic.vert` zamień linię z `gl_Position` na `gl_Position = vec4(aPosition, 1.0);` i naciśnij `Reload shaders`. Na środku jest zielony prostokąt o połowie szerokości i wysokości okna. Wyjaśnij trzy rzeczy: dlaczego prostokąt, a nie kwadrat, dlaczego widać ścianę **tylną** (zieloną), a nie przednią, i jakie położenie mają teraz uniformy `uModel`, `uView`, `uProjection` (sekcja 2.8). Dlaczego program C++, który nadal woła `setMat4`, nie zgłasza błędu?
8. **Przesunięcie w przestrzeni lokalnej.** W `basic.vert` zamień `vec4(aPosition, 1.0)` na `vec4(aPosition + vec3(1.0, 0.0, 0.0), 1.0)`. Kostka przesunęła się, ale nie dokładnie w prawo ekranu. Dlaczego? W którym miejscu wyrażenia trzeba by dodać przesunięcie, żeby było przesunięciem w przestrzeni świata? Nie zmieniaj kodu C++.
9. **Wersja GLSL.** Zmień pierwszą linię `basic.vert` na `#version 460 core`, potem usuń ją całkiem. Zapisz oba komunikaty. Który z nich pojawiłby się także na PC z nowym sterownikiem?
10. **Ścieżki przez `buildProgram`.** Dla każdego z czterech wyjść funkcji `buildProgram` (sekcja 5.7) wypisz po kolei wszystkie wywołania `glCreate*` i `glDelete*`, które się wykonają, i sprawdź, że każdemu `glCreate*` odpowiada `glDelete*` albo zwrócenie identyfikatora. Które wyjście wystąpiło w ćwiczeniu 3, a które w ćwiczeniu 4?
11. **Brak pliku.** W `NightMazeApp.cpp` zmień `VERTEX_SHADER_FILE` na nieistniejącą nazwę, zbuduj i uruchom. Jaka linia pojawia się w konsoli i ile razy? Wycofaj zmianę.
12. **Przeniesienie na kartce.** Dla kodu `Shader a(p1, p2); Shader b = std::move(a);` zapisz wartość `m_program` w obu obiektach po każdej linii (przyjmij, że program dostał identyfikator 3). Ile razy i z jakim argumentem zostanie zawołane `glDeleteProgram`, gdy oba obiekty wyjdą z zasięgu? Powtórz, zakładając, że w konstruktorze przenoszącym brakuje linii `other.m_program = 0;`.
13. **Przypisanie do siebie.** Prześledź na kartce `a = std::move(a);` dla obiektu z programem 3, najpierw z warunkiem `if (this == &other)`, potem bez niego. W jakim stanie zostaje obiekt w drugim przypadku?
14. **Literówka w działającym programie.** Przy działającym programie usuń średnik po `fragColor = vec4(vColor, 1.0)` w `basic.frag` i naciśnij `Reload shaders`. Porównaj z ćwiczeniem 3: co pokazuje linia `Program`, co widać w oknie, ile linii `[error]` jest w konsoli po trzech kliknięciach? Wskaż w `Shader::reload` linię, przez którą kostka nie zniknęła.
15. **Brak pliku w działającym programie.** Przy działającym programie zmień nazwę pliku `assets/shaders/basic.frag` na `basic2.frag` i naciśnij `Reload shaders`. Jaki komunikat pokazuje panel i czym różni się od błędu kompilacji? Przywróć nazwę i naciśnij przycisk ponownie.
16. **Napis formatujący.** W `ShadersPanel.cpp` zamień tymczasowo `ImGui::TextWrapped("%s", shader.lastError().c_str());` na `ImGui::TextWrapped(shader.lastError().c_str());` i zbuduj. Przeczytaj ostrzeżenie kompilatora. Wyjaśnij, co by się stało, gdyby komunikat sterownika zawierał `%d`. Wycofaj zmianę.
17. **Droga referencji.** Bez zaglądania do sekcji 6.2 wypisz pliki, przez które referencja do `m_shader` przechodzi od pola w `NightMazeApp` do wywołania `shader.reload()` w panelu. Dla każdego pliku podaj, czy dołącza `gfx/Shader.hpp`, czy wystarcza mu deklaracja wyprzedzająca, i dlaczego.
18. **Kolejność w klatce.** W `ShadersPanel.cpp` linie pokazujące `lastError()` stoją **pod** przyciskiem. Co pokazałby panel w klatce kliknięcia, gdyby stały nad nim? Czy użytkownik zauważyłby różnicę i dlaczego?
19. **Literówka w nazwie uniformu.** W `NightMazeApp.cpp` zmień `MODEL_UNIFORM` na `"uModle"`, zbuduj i uruchom. Co widać w oknie, co w konsoli, co w panelu Shaders (`Program`, `Last load`)? Wyjaśnij, jaką wartość ma `uModel` w shaderze i gdzie lądują wierzchołki. Wycofaj zmianę.
20. **`setMat4` przed `use()`.** W `NightMazeApp::onRender` przenieś linię `m_shader.use();` pod trzy wywołania `setMat4`. Zbuduj i uruchom. Czy w konsoli jest błąd i po którym wywołaniu? Co widać w pierwszej klatce, a co w następnych? Wycofaj zmianę.
21. **Transpozycja.** W `Shader::setMat4` zamień `GL_FALSE` na `GL_TRUE`. Zbuduj i uruchom. Opisz obraz. Dla macierzy modelu samej kostki (sam obrót) transpozycja to obrót w przeciwną stronę: dlaczego? Która z trzech macierzy psuje się najbardziej i dlaczego? Wycofaj zmianę.
22. **Uniform na kartce.** Dopisz na kartce do `basic.frag` uniform `uniform vec3 uTint;` i pomnóż przez niego kolor. Zapisz deklarację i implementację funkcji `setVec3` w stylu `setMat4` (wskazówka: `glUniform3fv(location, 1, glm::value_ptr(value))`). Co zobaczysz, jeśli zapomnisz ją zawołać, i dlaczego?

## 9. Pytania kontrolne

1. **Z jakich etapów składa się potok renderowania i które są programowalne?**
   Dane wierzchołków, shader wierzchołków, składanie prymitywów (z przycinaniem i dzieleniem przez w), rasteryzacja, shader fragmentów, testy i mieszanie, bufor ramki. Programowalne są shader wierzchołków i shader fragmentów (oraz opcjonalne etapy teselacji i geometrii, których jeszcze nie używam). Reszta jest stała i tylko konfigurowana stanem OpenGL.

2. **Co musi zapisać shader wierzchołków i w jakiej przestrzeni?**
   Zmienną `gl_Position`: pozycję wierzchołka w przestrzeni przycięcia, jako `vec4`. OpenGL sam przycina i dzieli `x`, `y`, `z` przez `w`, co daje NDC, czyli sześcian od -1 do 1. Przy `w = 1` współrzędne z shadera są od razu NDC.

3. **Czym fragment różni się od piksela?**
   Fragment to dane dla jednego piksela pochodzące z jednego prymitywu. Na jeden piksel może przypaść wiele fragmentów, a testy (głębi, szablonu) i mieszanie decydują, co trafi do bufora.

4. **Co oznaczają `in`, `out`, `uniform` i `layout(location = 0)`?**
   `in` to wejście etapu (atrybut wierzchołka albo interpolowana wartość), `out` to wyjście (dla shadera fragmentów kolor). `uniform` to wartość ustawiana z C++, wspólna dla całego rysowania. `layout(location = 0)` nadaje atrybutowi numer, którym posługuje się kod C++ przy opisie danych w buforze.

5. **Czym różni się obiekt shadera od obiektu programu?**
   Obiekt shadera to jeden skompilowany etap, produkt pośredni. Obiekt programu to zlinkowany komplet etapów, którego używa `glUseProgram`. Po linkowaniu obiekty shaderów odłączam i usuwam, a `gfx::Shader` przechowuje identyfikator programu.

6. **Jaki błąd wykrywa kompilacja, a jaki linkowanie?**
   Kompilacja sprawdza jeden shader: składnię, typy, nazwy. Linkowanie sprawdza, czy etapy do siebie pasują: czy każde `in` shadera fragmentów ma `out` o tej samej nazwie i typie w shaderze wierzchołków i czy jest `main`.

7. **Dlaczego `GL_CHECK` nie wystarczy do wykrycia błędu w shaderze?**
   `glGetError` zgłasza błędne użycie API. Shader z błędem składni to poprawnie wywołane `glCompileShader` z wynikiem "nie kompiluje się", więc żadna flaga nie jest ustawiana. Wynik trzeba odczytać przez `glGetShaderiv(GL_COMPILE_STATUS)` i `glGetProgramiv(GL_LINK_STATUS)`, a tekst błędu przez `glGetShaderInfoLog` i `glGetProgramInfoLog`.

8. **Dlaczego `glShaderSource` dostaje `&sourceText`, a nie `source.c_str()`?**
   Funkcja przyjmuje tablicę napisów C (wskaźnik na wskaźnik) i ich liczbę. Mam jeden napis, więc zapisuję wskaźnik w zmiennej i podaję jej adres jako tablicę jednoelementową. `nullptr` jako tablica długości oznacza napisy zakończone zerem.

9. **Jak odczytywany jest dziennik i po co `resize` na końcu?**
   `GL_INFO_LOG_LENGTH` podaje długość razem z kończącym zerem. Tworzę `std::string` tej długości, `glGetShaderInfoLog` wypełnia go i wpisuje do `written` liczbę znaków bez zera, a `resize(written)` obcina zero z końca napisu.

10. **Co robi `reload()` przy błędzie i dlaczego w takiej kolejności?**
    Buduje nowy program w zmiennej lokalnej. Przy błędzie zapisuje komunikat w `m_lastError`, loguje go i zwraca `false`, nie dotykając `m_program`, więc stary program działa dalej. Stary program jest usuwany dopiero po udanym zbudowaniu nowego. Dzięki temu literówka w shaderze nie daje czarnego ekranu.

11. **Dlaczego konstruktor `Shader` nie rzuca wyjątku przy błędzie w shaderze?**
    Bo błąd w pliku GLSL da się naprawić bez restartu: poprawić plik i zawołać `reload()`. Obiekt powstaje zawsze, `isValid()` mówi, czy ma program, a `lastError()` dlaczego nie. Zamknięcie programu z powodu literówki w shaderze przekreślałoby sens wczytywania na żywo.

12. **Co zawiera komunikat błędu i dlaczego dziennik sterownika nie jest przetwarzany?**
    Rodzaj błędu, ścieżkę pliku (przy linkowaniu obie ścieżki) i dziennik sterownika z numerem linii. Format dziennika jest inny u każdego producenta (Apple: `ERROR: 0:12: ...`, NVIDIA: `0(12) : error ...`), więc próba jego rozbioru działałaby tylko na jednej karcie.

13. **Dlaczego `Shader` nie da się kopiować?**
    Kopia miałaby ten sam identyfikator programu. Oba destruktory zawołałyby `glDeleteProgram` dla tego samego obiektu: drugi usuwałby coś, czego już nie ma, albo nowy obiekt, który dostał ten sam numer. Obiekt OpenGL ma jednego właściciela, więc opakowanie można tylko przenosić.

14. **Co robi konstruktor przenoszący i która linia jest w nim najważniejsza?**
    Przenosi ścieżki i napis błędu, kopiuje identyfikator programu i zeruje go w obiekcie źródłowym: `other.m_program = 0;`. Dzięki temu właściciel jest jeden, a destruktor obiektu źródłowego woła `glDeleteProgram(0)`, które OpenGL ignoruje.

15. **Czym przypisanie przenoszące różni się od konstruktora przenoszącego?**
    Obiekt docelowy już istnieje i może mieć program, więc najpierw trzeba go zwolnić (`glDeleteProgram(m_program)`), inaczej byłby wyciek. Trzeba też obsłużyć przypisanie do samego siebie (`this == &other`), bo bez tego obiekt usunąłby własny program i został z zerem.

16. **Dlaczego destruktor nie ma warunku `if (m_program != 0)`?**
    Specyfikacja OpenGL gwarantuje, że `glDeleteProgram(0)` jest po cichu ignorowane. Obiekt bez programu (nieudane wczytanie, obiekt po przeniesieniu) nie wymaga więc osobnej gałęzi.

17. **Co się stanie, gdy obiekt `Shader` przeżyje okno?**
    Destruktor zawoła `glDeleteProgram` bez bieżącego kontekstu OpenGL, co jest błędem (w praktyce awaria albo zignorowane wywołanie i wyciek). Dlatego `Shader` ma być polem klasy pochodnej od `core::Application`: pola giną przed klasą bazową, która posiada okno.

18. **Dlaczego `core::pathText` używa `u8string()`, a nie `string()`?**
    Na Windowsie `string()` zamienia ścieżkę na lokalną stronę kodową i rzuca wyjątek, gdy znaku nie da się w niej zapisać. Konstruktor `Shader` ma nie rzucać, więc tekst do komunikatu powstaje z UTF-8, który mieści każdą ścieżkę. Dodatkowo UTF-8 jest tym, czego oczekuje ImGui.

19. **Shader z `#version 460` działa na PC, a na Macu nie. Dlaczego?**
    macOS obsługuje OpenGL najwyżej 4.1, czyli GLSL 4.10. Wyższe wersje sterownik Apple odrzuca już na linii `#version`. Projekt używa `#version 410 core` na obu systemach.

20. **`glGetUniformLocation` zwraca -1, choć nazwa jest poprawna. Co się stało?**
    Kompilator usunął uniform, bo nie wpływa na wynik shadera (jest nieużyty albo jego użycie zostało zoptymalizowane). Dla OpenGL taki uniform nie istnieje. Ustawianie położenia -1 jest ignorowane bez błędu.

21. **Kiedy wczytywany jest shader i co trzeba zrobić po zmianie pliku `.frag`?**
    Przy starcie, w konstruktorze `NightMazeApp` (konstruktor `Shader` woła `reload()`), i po każdym naciśnięciu `Reload shaders` w panelu Shaders. Po zmianie pliku zapisuję go i naciskam przycisk. Kompilacja C++ nie jest potrzebna, bo shader jest plikiem czytanym w czasie działania. Na Windowsie przed naciśnięciem trzeba zbudować, żeby odświeżyć kopię katalogu `assets`.

22. **Co się dzieje w klatce, gdy shader się nie wczytał?**
    `m_shader.isValid()` zwraca fałsz i `onRender` kończy się (`return`) przed `use()`, `setMat4()`, `bind()` i `glDrawElements`. Klatka to samo tło i panele. Błąd został wypisany raz, przy wczytaniu, a nie co klatkę.

23. **Dlaczego każda ściana kostki ma jednolity kolor, skoro rasteryzacja interpoluje `vColor`?**
    Shader wierzchołków zapisuje `vColor` dla trzech wierzchołków trójkąta, a rasteryzacja interpoluje tę wartość dla każdego fragmentu. Wszystkie cztery wierzchołki jednej ściany mają w danych ten sam kolor, więc wartość pośrednia jest tym samym kolorem. Dlatego kostka ma 24 wierzchołki, po 4 na ścianę, a nie 8 wspólnych.

24. **Co pokazuje panel Shaders i skąd bierze każdą wartość?**
    Nazwy obu plików (`vertexPath()`, `fragmentPath()`, zamienione na tekst przez `core::pathText`, pełna ścieżka w podpowiedzi), stan programu (`isValid()`), przycisk wołający `reload()` oraz wynik ostatniego wczytania: `OK`, gdy `lastError()` jest pusty, albo jego tekst na czerwono. Panel nie ma własnego stanu: wszystko czyta co klatkę z obiektu `Shader`.

25. **Jak panel z `debug/` dostaje shader, który jest prywatnym polem gry?**
    `NightMazeApp` udostępnia chroniony akcesor `shader()`. `DebugNightMazeApp` w `main.cpp` wpisuje jego wynik do pola `shader` struktury `DebugContext`, a `DebugUI::draw` przekazuje `context.shader` do `drawShadersPanel`. Gra nie dołącza niczego z `debug/`, a `debug/` niczego z `game/`.

26. **Po nieudanym przeładowaniu panel pokazuje `Program: valid` i czerwony błąd. Czy to sprzeczność?**
    Nie. `isValid()` odpowiada na pytanie, czy jest program, którym można rysować, i jest nim poprzedni program. `lastError()` odpowiada na pytanie, czy ostatnie wczytanie się udało. `reload()` celowo nie dotyka `m_program` przy błędzie.

27. **`reload()` wykonuje się w środku klatki ImGui. Dlaczego to bezpieczne?**
    Między `NewFrame` a `Render` ImGui nie woła OpenGL, a `reload()` nie zmienia powiązań (program, VAO, bufory, tekstury). Backend w `RenderDrawData` sam ustawia swój program i stan. Stary program, usunięty jako bieżący, jest tylko oznaczony do usunięcia. Backend przy przywracaniu stanu sprawdza `glIsProgram` i nie przywraca programu, którego już nie ma. Gra w następnej klatce woła `use()` z nowym identyfikatorem.

28. **Dlaczego tekst błędu jest przekazywany jako argument `"%s"`?**
    Funkcje tekstowe ImGui traktują pierwszy argument jak format `printf`. Tekst pochodzi od sterownika i może zawierać `%`, co kazałoby funkcji czytać argumenty, których nie ma. Jako argument `"%s"` tekst jest tylko kopiowany.

29. **Jak wygląda przeładowanie shadera na Windowsie i dlaczego inaczej niż na macOS?**
    Program czyta tam kopię katalogu `assets` obok pliku `.exe`, a nie pliki z repozytorium. Po zapisaniu pliku trzeba więc wykonać `cmake --build --preset debug`, które odświeża kopię, i dopiero wtedy nacisnąć `Reload shaders`. Na macOS obok programu jest dowiązanie do katalogu w repozytorium, więc wystarcza sam przycisk.

27. **Co robi `setMat4`, linia po linii?**
    Pyta sterownik o położenie uniformu o podanej nazwie w programie obiektu (`glGetUniformLocation`), a potem kopiuje do niego 16 liczb macierzy (`glUniformMatrix4fv`): jedna macierz, bez transpozycji, wskaźnik z `glm::value_ptr`. Położenie jest wyszukiwane za każdym razem, bez pamięci podręcznej.

28. **Dlaczego przed `setMat4` musi stać `use()`?**
    `glUniformMatrix4fv` nie przyjmuje identyfikatora programu, tylko pisze do programu bieżącego, wybranego przez `glUseProgram`. Bez `use()` macierz trafiłaby do innego programu albo wywołanie skończyłoby się `GL_INVALID_OPERATION`. W OpenGL 4.1 jest też `glProgramUniform*` z programem jako argumentem, ale projekt używa postaci z wykładu.

29. **Co się stanie przy literówce w nazwie uniformu?**
    `glGetUniformLocation` zwróci -1, a `glUniformMatrix4fv` z położeniem -1 jest ignorowane bez błędu. Uniform w shaderze zostaje z zerami. Dla macierzy modelu oznacza to wszystkie wierzchołki w jednym punkcie i pusty ekran, bez żadnego komunikatu. To samo położenie -1 ma uniform usunięty przez kompilator jako nieużywany.

30. **Dlaczego `transpose` to `GL_FALSE`?**
    GLM przechowuje macierz kolumnami, tak samo jak oczekuje jej OpenGL i GLSL. Nie ma czego transponować.

31. **Dlaczego shader ma trzy osobne macierze, a nie jedną?**
    Dla nauki: każdą da się podmienić osobno i zobaczyć skutek, a wyrażenie `uProjection * uView * uModel * vec4(aPosition, 1.0)` pokazuje wprost drogę wierzchołka przez przestrzenie. Prawdziwy renderer często wysyła jeden gotowy iloczyn. Macierz modelu osobno przyda się też przy oświetleniu.

32. **Dlaczego macierze są wysyłane co klatkę, skoro kostka stoi w miejscu?**
    Macierz rzutowania zależy od proporcji okna, które mogą się zmienić. Po `reload()` nowy program ma uniformy wyzerowane, więc wartości wysłane raz by przepadły. A ruchoma kamera i tak zmienia macierz widoku w każdej klatce.

## 10. Źródła

- LearnOpenGL, rozdział "Hello Triangle" (<https://learnopengl.com/Getting-started/Hello-Triangle>): potok graficzny, shader wierzchołków i fragmentów, kompilacja, linkowanie, odczyt dziennika.
- LearnOpenGL, rozdział "Shaders" (<https://learnopengl.com/Getting-started/Shaders>): GLSL, typy, `in` i `out`, uniformy, własna klasa shadera wczytująca pliki.
- docs.gl, OpenGL 4: `glGetUniformLocation` (<https://docs.gl/gl4/glGetUniformLocation>), `glUniform` (<https://docs.gl/gl4/glUniform>, w tym `glUniformMatrix4fv` i zachowanie dla położenia -1), `glProgramUniform` (<https://docs.gl/gl4/glProgramUniform>).
- docs.gl (<https://docs.gl>), strony dla OpenGL 4: `glCreateShader`, `glShaderSource`, `glCompileShader`, `glGetShader` (`glGetShaderiv`), `glGetShaderInfoLog`, `glCreateProgram`, `glAttachShader`, `glDetachShader`, `glLinkProgram`, `glGetProgram` (`glGetProgramiv`), `glGetProgramInfoLog`, `glUseProgram`, `glDeleteShader`, `glDeleteProgram` (w tym zdanie o ignorowaniu wartości 0 i o programie będącym w użyciu).
- Khronos OpenGL Wiki: "Rendering Pipeline Overview" (<https://www.khronos.org/opengl/wiki/Rendering_Pipeline_Overview>), "Shader Compilation" (<https://www.khronos.org/opengl/wiki/Shader_Compilation>), "GLSL Object" (<https://www.khronos.org/opengl/wiki/GLSL_Object>), "Uniform (GLSL)" (<https://www.khronos.org/opengl/wiki/Uniform_(GLSL)>, o uniformach nieaktywnych).
- cppreference: semantyka przenoszenia (<https://en.cppreference.com/w/cpp/language/move_constructor>, <https://en.cppreference.com/w/cpp/language/move_assignment>), `std::filesystem::path::u8string`, `std::basic_ifstream`.
- Dokumenty w tym repozytorium: [`README.md`](README.md) (RAII i przenoszenie w `gfx`), [`../core/gl-check.md`](../core/gl-check.md), [`../core/paths.md`](../core/paths.md), [`../debug-ui.md`](../debug-ui.md) (jak panele są wpięte w program), [`../../libraries/imgui.md`](../../libraries/imgui.md), [`../../libraries/glad.md`](../../libraries/glad.md) (dlaczego tylko 4.1), [`../../libraries/glm.md`](../../libraries/glm.md).
- Dear ImGui, plik `backends/imgui_impl_opengl3.cpp` (funkcja `ImGui_ImplOpenGL3_RenderDrawData`: zapamiętanie i przywrócenie stanu, sprawdzenie `glIsProgram`) oraz `imgui.h` (`Button`, `TextUnformatted`, `TextWrapped`, `PushStyleColor`, `SetItemTooltip`).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o shaderach i języku GLSL).
- "OpenGL. Księga eksperta" (rozdziały o potoku programowalnym i shaderach).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): temat 2 w mapowaniu na wykłady oraz zasady "RAII dla obiektów GL" i "Shadery jako pliki".
