# Moduł gfx: bufory i tablica wierzchołków

Kamień milowy: M1. Temat wykładu: 2 (Programowalny potok).
Kod: [`src/gfx/Buffer.hpp`](../../../src/gfx/Buffer.hpp), [`src/gfx/Buffer.cpp`](../../../src/gfx/Buffer.cpp), [`src/gfx/VertexArray.hpp`](../../../src/gfx/VertexArray.hpp), [`src/gfx/VertexArray.cpp`](../../../src/gfx/VertexArray.cpp), użycie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp).

Część modułu `gfx`. Wstęp do całego modułu, zasada RAII dla obiektów OpenGL i semantyka przenoszenia są w [`README.md`](README.md). Druga część tematu 2, czyli shadery i sam potok, jest w [`shaders.md`](shaders.md): ten dokument zakłada jej znajomość. Ciąg dalszy tego dokumentu to [`indexed-drawing.md`](indexed-drawing.md): indeksy, dane kostki w `NightMazeApp` i rysowanie przez `glDrawElements`. Każde wywołanie OpenGL jest opakowane w `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)).

## 1. Po co to jest

Shader wierzchołków ([`shaders.md`](shaders.md), sekcja 2.2) dostaje na wejściu atrybuty jednego wierzchołka. Skądś muszą się one wziąć. Odpowiedzią są dwa rodzaje obiektów OpenGL:

- **bufor** (buffer object) to blok pamięci na karcie graficznej, do którego kopiuję tablicę liczb z programu: pozycje, kolory, później normalne i współrzędne tekstur,
- **tablica wierzchołków** (vertex array object, VAO) to opis, jak te liczby czytać: który atrybut ma ile składowych, w którym buforze leży, od którego bajtu się zaczyna i co ile bajtów się powtarza.

Bufor to dane bez znaczenia, VAO to znaczenie bez danych. Dopiero razem z programem shaderów dają komplet potrzebny do wywołania rysującego.

Dwie klasy opakowują te obiekty:

| Klasa | Obiekt OpenGL | Co robi |
|---|---|---|
| `gfx::Buffer` | jeden bufor | konstruktor tworzy bufor i wypełnia go raz danymi, `bind()` wiąże go z jego celem, destruktor usuwa |
| `gfx::VertexArray` | jeden VAO | konstruktor tworzy pusty VAO i od razu go wiąże, `setFloatAttribute` opisuje jeden atrybut, `bind()` ustawia VAO jako bieżący, destruktor usuwa |

Obie są cienkimi opakowaniami typu RAII, których nie da się kopiować, a da się przenosić, tak jak `gfx::Shader`. Celowo nie ma tu żadnej abstrakcji "układu wierzchołka": atrybuty opisuję pojedynczymi wywołaniami z jawnym krokiem i przesunięciem w bajtach, bo właśnie te liczby trzeba umieć wytłumaczyć.

Stan na dziś: `game::NightMazeApp` ma jeden `gfx::VertexArray` i dwa obiekty `gfx::Buffer`: bufor wierzchołków z danymi 24 wierzchołków kostki i bufor indeksów z 36 indeksami. Co klatkę rysuje nimi kostkę przez `glDrawElements` ([`indexed-drawing.md`](indexed-drawing.md), sekcja 5). Gdzie kostka stoi i skąd jest oglądana, ustalają macierze opisane w [`../scene/transforms.md`](../scene/transforms.md) (macierz modelu) i [`../scene/camera.md`](../scene/camera.md) (macierze widoku i rzutowania).

## 2. Teoria

### 2.1 Dane wierzchołków i atrybuty

**Wierzchołek** (vertex) w OpenGL to nie tylko punkt w przestrzeni. To zestaw wartości, które shader wierzchołków dostaje dla jednego rogu trójkąta. Każda z tych wartości to **atrybut wierzchołka** (vertex attribute):

| Atrybut | Typ w GLSL | Liczb `float` |
|---|---|---|
| pozycja | `vec3` | 3 |
| kolor | `vec3` | 3 |
| normalna (od tematu 6) | `vec3` | 3 |
| współrzędne tekstury (od tematu 5) | `vec2` | 2 |

Po stronie C++ dane wierzchołków to zwykła, płaska tablica liczb `float`. OpenGL nie wie, że pierwsze trzy to pozycja, a następne trzy to kolor: trzeba mu to opisać.

Atrybuty mają **numery** (indeksy, od 0). Numer jest jedynym łącznikiem między danymi a shaderem: opis w C++ mówi "atrybut 0 to trzy liczby od bajtu 0", a shader mówi `layout(location = 0) in vec3 aPosition;` (sekcja 4).

### 2.2 Bufor wierzchołków (VBO)

Karta graficzna nie czyta pamięci mojego programu. Dane trzeba do niej **skopiować**, a miejscem, do którego trafiają, jest obiekt bufora. Bufor z danymi wierzchołków nazywa się potocznie **VBO** (vertex buffer object). Sam bufor jest tylko ciągiem bajtów o znanej długości. O tym, do czego służy, decyduje **cel** (target), z którym jest związany:

| Cel | Potoczna nazwa bufora | Zawartość |
|---|---|---|
| `GL_ARRAY_BUFFER` | VBO | atrybuty wierzchołków |
| `GL_ELEMENT_ARRAY_BUFFER` | EBO albo IBO | indeksy wierzchołków |

OpenGL 4.1 działa w modelu **zwiąż, potem edytuj** (bind to edit): funkcje nie przyjmują identyfikatora bufora, tylko cel, i działają na buforze, który jest w tej chwili z tym celem związany. Żeby wypełnić bufor, trzeba go więc najpierw związać (`glBindBuffer`), a dopiero potem wysłać dane (`glBufferData`). Funkcje działające wprost na identyfikatorze (direct state access, `glNamedBufferData`) pojawiły się w OpenGL 4.5 i na macOS ich nie ma.

### 2.3 Układ przeplatany, krok i przesunięcie

Gdy wierzchołek ma kilka atrybutów, można je ułożyć w buforze na dwa sposoby: każdy atrybut w osobnym buforze albo wszystkie atrybuty jednego wierzchołka obok siebie, wierzchołek po wierzchołku. Drugi sposób to **układ przeplatany** (interleaved) i jest najczęstszy: dane jednego wierzchołka leżą razem w pamięci.

Bajty jednego wierzchołka z pozycją i kolorem (6 liczb `float`, każda po 4 bajty):

```mermaid
flowchart LR
    subgraph V0["wierzchołek 0: bajty od 0 do 23"]
        direction LR
        P0["pozycja x y z<br/>bajty od 0 do 11"] --- C0["kolor r g b<br/>bajty od 12 do 23"]
    end
    subgraph V1["wierzchołek 1: bajty od 24 do 47"]
        direction LR
        P1["pozycja x y z<br/>bajty od 24 do 35"] --- C1["kolor r g b<br/>bajty od 36 do 47"]
    end
    V0 --- V1
```

Dwie liczby opisują położenie atrybutu w takim buforze:

- **krok** (stride): odległość w bajtach między początkiem jednego wierzchołka a początkiem następnego. Jest **taki sam dla wszystkich atrybutów** tego bufora, bo opisuje rozmiar całego wierzchołka. Tu: 6 liczb po 4 bajty, czyli 24.
- **przesunięcie** (offset): od którego bajtu wewnątrz wierzchołka zaczyna się dany atrybut. Pozycja: 0. Kolor: 12, bo przed nim leżą trzy liczby pozycji.

| Atrybut | Numer | Składowych | Krok | Przesunięcie |
|---|---|---|---|---|
| pozycja | 0 | 3 | 24 | 0 |
| kolor | 1 | 3 | 24 | 12 |

Karta wylicza adres atrybutu dla wierzchołka numer `i` jako `przesunięcie + i * krok`. Kolor wierzchołka 1 zaczyna się więc w bajcie 12 + 1 * 24 = 36, zgodnie z diagramem.

Gdy wierzchołek ma tylko pozycję, krok to 12, a przesunięcie 0.

Układ z tej sekcji (pozycja i kolor, krok 24, przesunięcia 0 i 12) to dokładnie układ wierzchołka w projekcie. Stałe, które go opisują w kodzie, są w [`indexed-drawing.md`](indexed-drawing.md), sekcja 5.2.

### 2.4 Tablica wierzchołków (VAO): co pamięta, a czego nie

Opis atrybutów trzeba gdzieś zapisać. Robi to obiekt tablicy wierzchołków, **VAO**. To najczęściej źle rozumiany obiekt OpenGL, więc dokładnie:

**VAO pamięta:**

| Stan | Jak się go ustawia |
|---|---|
| dla każdego numeru atrybutu: czy jest włączony | `glEnableVertexAttribArray(index)` |
| dla każdego atrybutu: format, czyli liczba składowych, typ, normalizacja, krok, przesunięcie | `glVertexAttribPointer(...)` |
| dla każdego atrybutu: **z którego bufora czyta** | `glVertexAttribPointer` zapisuje bufor, który w chwili wywołania jest związany z `GL_ARRAY_BUFFER` |
| wiązanie `GL_ELEMENT_ARRAY_BUFFER`, czyli bufor indeksów | `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`, gdy ten VAO jest bieżący |

**VAO nie pamięta:**

| Czego nie | Co z tego wynika |
|---|---|
| samego wiązania `GL_ARRAY_BUFFER` | to stan globalny kontekstu, a nie VAO. Liczy się tylko w chwili wywołania `glVertexAttribPointer`. Potem można związać z `GL_ARRAY_BUFFER` cokolwiek innego albo 0 i VAO nadal rysuje z zapisanego bufora |
| danych wierzchołków | VAO trzyma odwołania do buforów, nie ich zawartość |
| programu shaderów | `glUseProgram` jest osobnym stanem |

Te dwie tabele zawierają asymetrię, którą trzeba umieć wytłumaczyć: **wiązanie bufora indeksów należy do VAO, a wiązanie bufora wierzchołków nie**. Bufor wierzchołków jest zapisywany w VAO pośrednio, osobno dla każdego atrybutu, w chwili jego opisywania. Dzięki temu różne atrybuty mogą czytać z różnych buforów. Bufor indeksów jest jeden na VAO, więc jego wiązanie jest po prostu częścią stanu VAO.

```mermaid
flowchart TD
    subgraph Ctx["kontekst OpenGL (stan globalny)"]
        AB["wiązanie GL_ARRAY_BUFFER"]
        CurVao["bieżący VAO"]
        Prog["bieżący program"]
    end
    subgraph Vao["VAO"]
        A0["atrybut 0: włączony, 3 x float, krok 24, przesunięcie 0, bufor nr 1"]
        A1["atrybut 1: włączony, 3 x float, krok 24, przesunięcie 12, bufor nr 1"]
        EB["wiązanie GL_ELEMENT_ARRAY_BUFFER: bufor nr 2"]
    end
    CurVao --> Vao
    A0 --> Vbo["bufor nr 1: wierzchołki"]
    A1 --> Vbo
    EB --> Ebo["bufor nr 2: indeksy"]
    AB -. "czytane tylko w chwili glVertexAttribPointer" .-> A0
```

Sens VAO jest praktyczny: całą konfigurację robię raz, przy tworzeniu geometrii, a przy rysowaniu wystarcza jedno `glBindVertexArray`.

### 2.5 Dlaczego profil Core wymaga VAO

W starym OpenGL i w profilu zgodności (compatibility) istnieje domyślny VAO o numerze 0, więc stan atrybutów można ustawiać bez tworzenia żadnego obiektu. Profil Core ten domyślny obiekt usunął: numer 0 oznacza "brak VAO", a rysowanie bez związanego VAO jest błędem. Zmierzone w teście (sekcja 5.8): `glDrawArrays` bez VAO daje `GL_INVALID_OPERATION` i niczego nie rysuje.

Wiele starszych poradników pomija VAO, bo były pisane dla profilu zgodności. Na macOS dostępny jest tylko profil Core ([`../core/window-context.md`](../core/window-context.md)), więc tam taki kod nie działa.

### 2.6 Podpowiedź użycia: `GL_STATIC_DRAW`

Ostatni parametr `glBufferData` to **podpowiedź** (usage hint) dla sterownika: jak zamierzam bufora używać. Nazwa składa się z dwóch części.

| Pierwsza część | Znaczenie |
|---|---|
| `STATIC` | dane ustawiane raz, używane wiele razy |
| `DYNAMIC` | dane zmieniane wielokrotnie, używane wiele razy |
| `STREAM` | dane ustawiane raz i używane najwyżej kilka razy (na przykład nowe co klatkę) |

| Druga część | Znaczenie |
|---|---|
| `DRAW` | program zapisuje dane, karta ich używa do rysowania |
| `READ` | karta zapisuje dane, program je odczytuje |
| `COPY` | karta zapisuje dane i karta ich używa |

Daje to dziewięć stałych, od `GL_STATIC_DRAW` do `GL_STREAM_COPY`. To tylko podpowiedź: sterownik może na jej podstawie wybrać rodzaj pamięci, ale nie ogranicza ona tego, co wolno z buforem zrobić. `gfx::Buffer` zawsze używa `GL_STATIC_DRAW`, bo bufor jest wypełniany raz, w konstruktorze, i klasa nie ma funkcji zmieniającej dane.

Dwa tematy teorii, które korzystają już z tych pojęć, są w [`indexed-drawing.md`](indexed-drawing.md), sekcja 2: indeksy i bufor indeksów (w tym odpowiedź, dlaczego kostka ma 24 wierzchołki) oraz współrzędne lokalne i kierunek nawijania.

## 3. Jak to działa w OpenGL

### 3.1 Wywołania w kolejności

Przygotowanie geometrii, raz:

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glGenVertexArrays(1, &id)` | Rezerwuje identyfikator nowego VAO i wpisuje go do `id`. Obiekt powstaje naprawdę przy pierwszym związaniu |
| 2 | `glBindVertexArray(id)` | Ustawia VAO jako bieżący. Od tej chwili konfiguracja atrybutów i wiązanie EBO trafiają do niego |
| 3 | `glGenBuffers(1, &id)` | Rezerwuje identyfikator nowego bufora |
| 4 | `glBindBuffer(GL_ARRAY_BUFFER, id)` | Wiąże bufor z celem. Następne funkcje z tym celem działają na tym buforze |
| 5 | `glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW)` | Przydziela `size` bajtów pamięci na karcie i kopiuje do niej dane spod wskaźnika `data`. Poprzednia zawartość bufora przepada |
| 6 | `glEnableVertexAttribArray(index)` | Włącza atrybut o danym numerze w bieżącym VAO. Wyłączony atrybut nie czyta bufora, a shader dostaje wartość stałą |
| 7 | `glVertexAttribPointer(index, size, type, normalized, stride, pointer)` | Zapisuje w bieżącym VAO format atrybutu i bufor związany teraz z `GL_ARRAY_BUFFER`. `size` to liczba składowych (od 1 do 4), `pointer` to przesunięcie w bajtach przebrane za wskaźnik (sekcja 5.6) |
| 8 | `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id)` i `glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...)` | To samo co kroki 4 i 5 dla bufora indeksów. Wiązanie zostaje zapisane w bieżącym VAO. Tylko gdy rysuję z indeksami |

Rysowanie, co klatkę:

| # | Wywołanie | Co robi |
|---|---|---|
| 9 | `glUseProgram(program)` | Wybiera program shaderów ([`shader-class.md`](shader-class.md), sekcja 3.1) |
| 10 | `glBindVertexArray(id)` | Przywraca całą zapisaną konfigurację jednym wywołaniem |
| 11 | `glDrawArrays(GL_TRIANGLES, 0, count)` albo `glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr)` | Rysuje. `nullptr` w drugiej funkcji to przesunięcie 0 w buforze indeksów |

Sprzątanie:

| # | Wywołanie | Co robi |
|---|---|---|
| 12 | `glDeleteBuffers(1, &id)` | Usuwa bufor. Identyfikator 0 jest po cichu ignorowany |
| 13 | `glDeleteVertexArrays(1, &id)` | Usuwa VAO. Identyfikator 0 jest po cichu ignorowany |

Kroki 1, 3, 12 i 13 przyjmują liczbę obiektów i wskaźnik do tablicy identyfikatorów, bo potrafią obsłużyć wiele obiektów naraz. Dla jednego obiektu tablicą jest adres pojedynczej zmiennej.

### 3.2 Kolejność, która ma znaczenie

```mermaid
sequenceDiagram
    participant Cpp as kod C++
    participant GL as OpenGL
    Cpp->>GL: glBindVertexArray(vao)
    Note over GL: od teraz konfiguracja trafia do tego VAO
    Cpp->>GL: glBindBuffer(GL_ARRAY_BUFFER, vbo), glBufferData
    Cpp->>GL: glEnableVertexAttribArray(0)
    Cpp->>GL: glVertexAttribPointer(0, ...)
    Note over GL: VAO zapisuje: atrybut 0 czyta z vbo
    Cpp->>GL: glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo), glBufferData
    Note over GL: VAO zapisuje: indeksy są w ebo
```

Trzy zależności, z których każda jest źródłem pułapki (druga: sekcja 7, pułapka 1, pierwsza i trzecia: [`indexed-drawing.md`](indexed-drawing.md), sekcja 7, pułapki 1 i 2):

1. VAO musi być bieżący **przed** `glEnableVertexAttribArray`, `glVertexAttribPointer` i przed związaniem EBO.
2. Właściwy VBO musi być związany z `GL_ARRAY_BUFFER` **w chwili** `glVertexAttribPointer`.
3. Wiązania EBO nie wolno zdejmować (`glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)`), dopóki VAO jest bieżący, bo to też zostanie zapisane w VAO.

Dwa wywołania rysujące z kroku 11 porównuje [`indexed-drawing.md`](indexed-drawing.md), sekcja 3.1.

## 4. Shadery

Ta część modułu nie ma własnych shaderów, ale jest z nimi ściśle związana: **numer atrybutu** podany w C++ musi być tym samym numerem co `layout(location = N)` w shaderze wierzchołków.

Wejścia shadera wierzchołków projektu, [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert) (cały plik omawia [`shaders.md`](shaders.md), sekcja 4.1):

```glsl
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the object
layout(location = 1) in vec3 aColor;    // red, green, blue, each from 0 to 1
```

Odpowiadające im stałe i wywołania w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp):

```cpp
// Attribute numbers: the same as layout(location = N) in basic.vert.
constexpr GLuint POSITION_ATTRIBUTE = 0;
constexpr GLuint COLOR_ATTRIBUTE = 1;
```

```cpp
m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, POSITION_COMPONENTS, VERTEX_STRIDE,
                                POSITION_OFFSET);
m_vertexArray.setFloatAttribute(COLOR_ATTRIBUTE, COLOR_COMPONENTS, VERTEX_STRIDE, COLOR_OFFSET);
```

| Po stronie C++ | Po stronie GLSL | Co musi się zgadzać |
|---|---|---|
| `POSITION_ATTRIBUTE` równe 0 | `layout(location = 0)` przy `aPosition` | numer |
| `COLOR_ATTRIBUTE` równe 1 | `layout(location = 1)` przy `aColor` | numer |
| `POSITION_COMPONENTS` i `COLOR_COMPONENTS` równe 3 | `vec3` | liczba składowych. Gdy bufor daje mniej składowych, niż ma typ w shaderze, brakujące są uzupełniane: `y` i `z` zerem, `w` jedynką |
| `GL_FLOAT` (wewnątrz `setFloatAttribute`) | `vec3` (typ zmiennoprzecinkowy) | rodzaj typu |

OpenGL nie sprawdza tej zgodności. Zły numer nie daje błędu kompilacji ani błędu `glGetError`: shader dostaje po prostu dane innego atrybutu albo wartość domyślną. Numery są w dwóch plikach (jednym C++ i jednym GLSL) i nic poza komentarzem ich nie wiąże, dlatego komentarz nad stałymi wskazuje plik shadera.

Gdyby w shaderze nie było `layout(location = ...)`, numery przydzieliłby linker i trzeba by o nie pytać funkcją `glGetAttribLocation` po zlinkowaniu programu. Jawne numery w shaderze są prostsze: VAO można skonfigurować, nie znając programu.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/gfx/Buffer.hpp`](../../../src/gfx/Buffer.hpp) | klasa `gfx::Buffer`: konstruktor z celem, danymi i rozmiarem, destruktor, zablokowane kopiowanie, przenoszenie, `bind`. Dołącza `<glad/gl.h>` i `<cstddef>` |
| [`src/gfx/Buffer.cpp`](../../../src/gfx/Buffer.cpp) | implementacja |
| [`src/gfx/VertexArray.hpp`](../../../src/gfx/VertexArray.hpp) | klasa `gfx::VertexArray`: konstruktor domyślny, destruktor, zablokowane kopiowanie, przenoszenie, `bind`, `setFloatAttribute` |
| [`src/gfx/VertexArray.cpp`](../../../src/gfx/VertexArray.cpp) | implementacja |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | jedyny użytkownik obu klas: pola `m_vertexArray`, `m_vertexBuffer` i `m_indexBuffer`, dane wierzchołków `VERTICES`, indeksy `INDICES`, stałe układu, konfiguracja w konstruktorze, rysowanie w `onRender` ([`indexed-drawing.md`](indexed-drawing.md), sekcja 5) |

Wszystkie cztery pliki są na liście źródeł biblioteki `engine` w [`CMakeLists.txt`](../../../CMakeLists.txt). Obie klasy zależą tylko od `core` (`GL_CHECK`), GLAD i biblioteki standardowej. Żadna nie ma funkcji pomocniczych ani stałych: całość to konstruktor, destruktor, dwie funkcje przenoszące i jedna albo dwie funkcje robocze.

Żadna z klas nie ma też funkcji zwracającej identyfikator ani funkcji "odwiąż" (`unbind`). Identyfikatora nikt jeszcze nie potrzebuje, a odwiązywanie nie jest konieczne: każdy kod, który czegoś potrzebuje, wiąże to sam przed użyciem.

### 5.2 `Buffer`: nagłówek

```cpp
class Buffer {
public:
    /// Creates a buffer, binds it to target and copies sizeInBytes bytes from data into it.
    /// target is GL_ARRAY_BUFFER (vertex data) or GL_ELEMENT_ARRAY_BUFFER (indices).
    /// OpenGL takes its own copy, so data may be freed right after the call.
    ///
    /// The buffer is left bound to target. For GL_ELEMENT_ARRAY_BUFFER this matters: that
    /// binding is not global, it is stored in the vertex array that is bound at the moment.
    /// So create the element buffer while its VertexArray is the bound one, otherwise the
    /// indices are attached to no vertex array (or to the wrong one). The VertexArray
    /// constructor binds, so creating the vertex array just before the buffer is enough.
    Buffer(GLenum target, const void* data, std::size_t sizeInBytes);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /// Takes over the buffer of other. other is left without a buffer.
    Buffer(Buffer&& other) noexcept;
    /// Deletes the buffer this object owns, then takes over the buffer of other.
    Buffer& operator=(Buffer&& other) noexcept;

    /// Binds the buffer to the target it was created for (glBindBuffer).
    void bind() const;

private:
    // Binding point given to the constructor, remembered so that bind() needs no argument.
    GLenum m_target;
    // Name (id) of the OpenGL buffer object. 0 is never a real buffer: it means "none".
    GLuint m_id = 0;
};
```

| Element | Dlaczego tak |
|---|---|
| `GLenum target` | jedna klasa dla obu rodzajów bufora. Kod jest identyczny, różni się tylko cel |
| `const void* data` | wskaźnik "na cokolwiek". Bufor to bajty: dla wierzchołków podam tablicę `float`, dla indeksów tablicę `GLuint`. Każdy wskaźnik do danych zamienia się na `const void*` niejawnie. `const`, bo funkcja danych nie zmienia |
| `std::size_t sizeInBytes` | rozmiar w **bajtach**, nie liczba elementów. `std::size_t` to typ, który zwraca `sizeof`. Nazwa parametru przypomina o jednostce (sekcja 7, pułapka 4) |
| `m_target` | zapamiętany cel, żeby `bind()` nie miało parametru i nie dało się związać bufora ze złym celem |
| `m_id = 0` | 0 to "nie ma bufora", jak `m_program` w `Shader` |
| brak konstruktora domyślnego | nie da się utworzyć pustego `Buffer`. Bufor bez danych nie ma w tym projekcie zastosowania |

Klasa nie jest szablonem i nie przyjmuje `std::vector` ani `std::array`. Wołający podaje wskaźnik i rozmiar wprost, na przykład `VERTICES.data()` i `VERTICES.size() * sizeof(float)`. To jeden jawny zapis więcej w miejscu użycia, w zamian za klasę bez szablonów.

### 5.3 `Buffer`: konstruktor, destruktor, `bind`

```cpp
Buffer::Buffer(GLenum target, const void* data, std::size_t sizeInBytes) : m_target(target) {
    // glGenBuffers writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenBuffers(1, &m_id));
    // OpenGL 4.1 can only fill the buffer that is bound, so bind first.
    GL_CHECK(glBindBuffer(m_target, m_id));
    // Allocates sizeInBytes bytes on the graphics card and copies the data there.
    // The size parameter is a signed type (GLsizeiptr), hence the cast.
    // GL_STATIC_DRAW is a hint: the data is set once and used for drawing many times.
    GL_CHECK(glBufferData(m_target, static_cast<GLsizeiptr>(sizeInBytes), data, GL_STATIC_DRAW));
}
```

| Linia | Co robi |
|---|---|
| `: m_target(target)` | Lista inicjalizacyjna zapamiętuje cel. `m_id` ma już wartość 0 z deklaracji pola |
| `glGenBuffers(1, &m_id)` | "Daj mi 1 nowy identyfikator i wpisz go pod ten adres". Funkcja chce tablicy, a adres jednej zmiennej jest tablicą o jednym elemencie |
| `glBindBuffer(m_target, m_id)` | Model "zwiąż, potem edytuj" (sekcja 2.2). Dopiero przy pierwszym związaniu OpenGL tworzy właściwy obiekt bufora |
| `static_cast<GLsizeiptr>(sizeInBytes)` | `glBufferData` przyjmuje rozmiar jako `GLsizeiptr`, typ **ze znakiem** o szerokości wskaźnika. `std::size_t` jest bez znaku, więc bez rzutowania kompilator ostrzegałby o niejawnej zmianie znaku |
| `data` | OpenGL kopiuje bajty podczas tego wywołania. Tablica w programie może potem zniknąć |
| `GL_STATIC_DRAW` | Podpowiedź użycia (sekcja 2.6) |

Po konstruktorze bufor **zostaje związany** ze swoim celem. Dla `GL_ARRAY_BUFFER` to wygodne: zaraz potem woła się `setFloatAttribute`, które potrzebuje właśnie tego wiązania. Dla `GL_ELEMENT_ARRAY_BUFFER` to jest sedno sprawy: samo utworzenie bufora indeksów zapisuje go w bieżącym VAO. Dlatego komentarz w nagłówku każe tworzyć bufor indeksów wtedy, gdy bieżącym VAO jest ten właściwy. Konstruktor `VertexArray` wiąże swój obiekt (sekcja 5.5), więc wystarcza utworzyć VAO tuż przed buforem.

```cpp
Buffer::~Buffer() {
    // OpenGL silently ignores the id 0 in glDeleteBuffers, so an object that was moved
    // from needs no special case.
    GL_CHECK(glDeleteBuffers(1, &m_id));
}
```

```cpp
void Buffer::bind() const {
    GL_CHECK(glBindBuffer(m_target, m_id));
}
```

`bind()` jest `const`, bo nie zmienia obiektu C++, tylko stan kontekstu OpenGL, tak jak `Shader::use()`.

### 5.4 `Buffer`: przenoszenie

Ogólne wyjaśnienie przenoszenia jest w [`README.md`](README.md), sekcja 2, a ten sam wzorzec dla `Shader` w [`shader-class.md`](shader-class.md), sekcja 5.9.

```cpp
// Move constructor: the new object takes the buffer id, and other gives it up.
Buffer::Buffer(Buffer&& other) noexcept : m_target(other.m_target), m_id(other.m_id) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
}

// Move assignment: this object already owns a buffer, which has to go first.
Buffer& Buffer::operator=(Buffer&& other) noexcept {
    // buffer = std::move(buffer): nothing to do. Without this check the buffer would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the buffer owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteBuffers(1, &m_id));

    m_target = other.m_target;
    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}
```

Oba pola to liczby, więc są kopiowane, a nie przenoszone przez `std::move`. Przeniesieniem czyni to jedna linia: `other.m_id = 0;`. `m_target` w obiekcie źródłowym zostaje, bo niczemu nie szkodzi: destruktor patrzy tylko na `m_id`. Przypisanie ma te same trzy kroki co w `Shader`: sprawdzenie przypisania do siebie, zwolnienie własnego bufora, przejęcie.

Przeniesienie **nie zmienia niczego po stronie OpenGL**: identyfikator jest ten sam, więc VAO, który zapisał ten bufor, nadal na niego wskazuje.

### 5.5 `VertexArray`: nagłówek i proste funkcje

```cpp
class VertexArray {
public:
    /// Creates an empty vertex array (no attribute is enabled yet) and binds it, so that
    /// a Buffer created after it is recorded in this vertex array.
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    /// Takes over the vertex array of other. other is left without one.
    VertexArray(VertexArray&& other) noexcept;
    /// Deletes the vertex array this object owns, then takes over the one of other.
    VertexArray& operator=(VertexArray&& other) noexcept;
```

```cpp
VertexArray::VertexArray() {
    // glGenVertexArrays writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenVertexArrays(1, &m_id));
    // Bind it right away. An element (index) buffer records itself in the vertex array
    // that is bound at the moment the buffer is created, so this vertex array has to be
    // the bound one before the next object, usually a Buffer, is constructed.
    GL_CHECK(glBindVertexArray(m_id));
}

VertexArray::~VertexArray() {
    // OpenGL silently ignores the id 0 in glDeleteVertexArrays, so an object that was
    // moved from needs no special case.
    GL_CHECK(glDeleteVertexArrays(1, &m_id));
}
```

```cpp
void VertexArray::bind() const {
    GL_CHECK(glBindVertexArray(m_id));
}
```

Konstruktor rezerwuje identyfikator (`glGenVertexArrays`) i **od razu wiąże** nowy VAO (`glBindVertexArray`). Sam VAO tego nie potrzebuje: w odróżnieniu od bufora nie ma danych, którymi trzeba go wypełnić. Wiązanie jest dla obiektu, który powstanie **po nim**. Bufor indeksów zapisuje się w tym VAO, który jest bieżący w chwili tworzenia bufora (sekcja 2.4), a obiekty OpenGL w `NightMazeApp` są polami klasy, konstruowanymi jedno po drugim na liście inicjalizacyjnej. Między konstruktorem jednego pola a konstruktorem następnego nie ma miejsca na wywołanie `m_vertexArray.bind()`: ciało konstruktora klasy wykonuje się dopiero po wszystkich polach. Gdyby konstruktor VAO nie wiązał, bufor indeksów utworzony jako następne pole trafiłby do żadnego albo do cudzego VAO ([`indexed-drawing.md`](indexed-drawing.md), sekcja 7, pułapka 1).

Zasada wynikająca z tego dla wołającego: **najpierw `VertexArray`, zaraz po nim jego bufory**. `setFloatAttribute` i tak wiąże VAO samo, więc dla atrybutów kolejność tworzenia nie ma znaczenia.

`glBindVertexArray` ma jeden parametr, bez celu: bieżący VAO jest zawsze jeden.

Przenoszenie jest takie samo jak w `Buffer`, tylko z jednym polem:

```cpp
// Move constructor: the new object takes the vertex array id, and other gives it up.
VertexArray::VertexArray(VertexArray&& other) noexcept : m_id(other.m_id) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
}

// Move assignment: this object already owns a vertex array, which has to go first.
VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    // vertexArray = std::move(vertexArray): nothing to do. Without this check the vertex
    // array would be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the vertex array owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteVertexArrays(1, &m_id));

    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}
```

### 5.6 `setFloatAttribute`

```cpp
void setFloatAttribute(GLuint index, GLint componentCount, GLsizei strideInBytes,
                       std::size_t offsetInBytes);
```

```cpp
void VertexArray::setFloatAttribute(GLuint index, GLint componentCount, GLsizei strideInBytes,
                                    std::size_t offsetInBytes) {
    // Attribute state belongs to the vertex array that is bound, so make sure it is this one.
    bind();
    GL_CHECK(glEnableVertexAttribArray(index));

    // The last parameter of glVertexAttribPointer has the type "pointer" for historical
    // reasons: in old OpenGL it could be the address of an array in the program's memory.
    // With a buffer bound to GL_ARRAY_BUFFER it is a byte offset into that buffer, so the
    // number is passed disguised as a pointer and nothing is ever read through it.
    // clang-tidy normally forbids turning a number into a pointer. Here the API demands
    // it, so the check is switched off for this one line.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const void* offsetAsPointer = reinterpret_cast<const void*>(offsetInBytes);

    // GL_FLOAT: each component is a 32 bit float. GL_FALSE: the values are used as they
    // are (normalization only applies to integer data).
    GL_CHECK(glVertexAttribPointer(index, componentCount, GL_FLOAT, GL_FALSE, strideInBytes,
                                   offsetAsPointer));
}
```

Parametry mają typy dokładnie takie, jakich chce OpenGL, żeby w środku nie było rzutowań poza jednym:

| Parametr | Typ | Znaczenie |
|---|---|---|
| `index` | `GLuint` | numer atrybutu, ten sam co `layout(location = index)` w shaderze |
| `componentCount` | `GLint` | liczba liczb `float` na wierzchołek, od 1 do 4. W OpenGL ten parametr nazywa się `size`, co myli z rozmiarem w bajtach, stąd inna nazwa |
| `strideInBytes` | `GLsizei` | krok (sekcja 2.3) |
| `offsetInBytes` | `std::size_t` | przesunięcie atrybutu wewnątrz wierzchołka (sekcja 2.3) |

| Linia | Co robi i dlaczego |
|---|---|
| `bind();` | Stan atrybutów trafia do **bieżącego** VAO. Funkcja składowa ma konfigurować swój obiekt, więc najpierw czyni go bieżącym. Bez tej linii wynik zależałby od tego, co wołający związał wcześniej |
| `glEnableVertexAttribArray(index)` | Atrybuty są domyślnie wyłączone. Opisany, ale niewłączony atrybut nie czyta bufora. Włączam od razu, bo opisanie atrybutu bez włączenia nie ma zastosowania |
| `reinterpret_cast<const void*>(offsetInBytes)` | Zamiana liczby na wskaźnik o tej samej wartości bitowej. Wyjaśnienie niżej |
| `GL_FLOAT` | Typ jednej składowej w buforze: 32 bitowa liczba zmiennoprzecinkowa, czyli `float` w C++ |
| `GL_FALSE` | Parametr `normalized`. Dotyczy tylko danych całkowitych (na przykład kolor w bajtach od 0 do 255 przeliczany na zakres od 0 do 1). Dla `GL_FLOAT` nie ma znaczenia |

**Dlaczego przesunięcie jest wskaźnikiem.** To historyczna osobliwość API. W OpenGL 1.1 nie było buforów na karcie: ostatni parametr `glVertexAttribPointer` (i jej poprzedniczek) był prawdziwym adresem tablicy w pamięci programu, stąd typ `const void*` i słowo "Pointer" w nazwie. Gdy doszły bufory, funkcji nie zmieniono, tylko nadano parametrowi drugie znaczenie: jeśli z `GL_ARRAY_BUFFER` związany jest bufor, wartość "wskaźnika" jest odczytywana jako **liczba bajtów od początku tego bufora**. W profilu Core pierwsze znaczenie już nie istnieje, zostało tylko drugie, ale typ parametru pozostał. Trzeba więc liczbę (na przykład 12) przekazać jako wskaźnik o wartości 12. Nikt nigdy nie odczytuje pamięci pod tym "adresem".

`reinterpret_cast` to rzutowanie, które każe kompilatorowi potraktować te same bity jako inny typ. Jest w projekcie używane rzadko i zawsze z komentarzem. Drugie miejsce to `glString` w `Window.cpp`.

**`NOLINTNEXTLINE`.** clang-tidy ma kontrolę `performance-no-int-to-ptr`, która zgłasza każdą zamianę liczby na wskaźnik (bo zwykle jest to błąd albo przeszkoda dla optymalizacji). Tutaj zamiana jest wymagana przez API i nie da się jej uniknąć. Komentarz `// NOLINTNEXTLINE(performance-no-int-to-ptr)` wyłącza tę jedną kontrolę dla jednej, następnej linii. To jedyne takie wyłączenie w `src/`.

**Który bufor.** Funkcja nie ma parametru "bufor". `glVertexAttribPointer` zapisuje w VAO ten bufor, który **w chwili wywołania** jest związany z `GL_ARRAY_BUFFER`. Wołający musi więc mieć związany właściwy `Buffer`: albo dopiero co go utworzył (konstruktor zostawia go związanego), albo zawołał `bind()`. Rozważałem przekazywanie `const Buffer&` jako parametru, żeby funkcja wiązała bufor sama. Zostałem przy obecnej postaci, bo jest wiernym odbiciem tego, jak działa OpenGL, a zależność od wiązania jest i tak rzeczą, którą trzeba rozumieć.

### 5.7 Czas życia: bufor a VAO, który go używa

VAO przechowuje odwołanie do bufora, a nie jego kopię. Co się dzieje, gdy `Buffer` zostanie zniszczony, a `VertexArray` nadal istnieje, zależy od tego, czy VAO jest w tej chwili bieżący. Zmierzone w teście (sekcja 5.8):

| Sytuacja | Skutek |
|---|---|
| bufor usunięty, gdy używający go VAO **nie jest** bieżący | VAO nadal rysuje poprawnie. OpenGL zwalnia nazwę bufora, ale dane trzyma, dopóki VAO się do nich odwołuje |
| bufor usunięty, gdy używający go VAO **jest** bieżący | OpenGL odłącza bufor od bieżącego VAO. Następne rysowanie daje `GL_INVALID_OPERATION` i niczego nie rysuje |

Nie warto na tych regułach polegać. Zasada dla projektu: `Buffer` żyje co najmniej tak długo jak `VertexArray`, który z niego czyta. Najprościej trzymać je jako pola tej samej klasy, tak jak `m_vertexArray`, `m_vertexBuffer` i `m_indexBuffer` w `NightMazeApp`. Przy zamykaniu programu oba bufory giną tam tuż przed VAO, co jest bez znaczenia, bo po nim nikt już nie rysuje.

### 5.8 Jak to zostało sprawdzone

**Test samych klas.** Zanim klasy dostały użytkownika, sprawdziłem je na Macu małym programem testowym poza repozytorium: ukryte okno GLFW z kontekstem 4.1 Core, biblioteka `engine` z buildu Debug, para shaderów z atrybutami pozycji i koloru (bez macierzy) oraz trójkąt o pozycjach (-0,5, -0,5), (0,5, -0,5), (0, 0,5), cały w kolorze czerwonym na niebieskim tle. Konstruktor `VertexArray` jeszcze wtedy nie wiązał, więc test wołał `bind()` sam. Wynik rysowania odczytywałem funkcją `glReadPixels`. Wyniki (sterownik Apple, `GL_VERSION` 4.1 Metal):

| Próba | Wynik |
|---|---|
| `glDrawArrays` bez związanego VAO | `GL_INVALID_OPERATION`, nic nie narysowane |
| `VertexArray`, `Buffer(GL_ARRAY_BUFFER, ...)`, dwa razy `setFloatAttribute` | `glGetError` czysty |
| rysowanie po odwiązaniu `GL_ARRAY_BUFFER` (wiązanie 0) i ponownym związaniu VAO | środek okna czerwony, róg niebieski: VAO pamięta bufor per atrybut |
| `Buffer(GL_ELEMENT_ARRAY_BUFFER, ...)` przy bieżącym VAO, potem `glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING)` | przy VAO 0: wartość 0. Po związaniu VAO: identyfikator bufora indeksów. Wiązanie należy do VAO |
| `glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr)` | środek okna czerwony |
| `Buffer(GL_ELEMENT_ARRAY_BUFFER, ...)` bez związanego VAO | sterownik Apple nie zgłosił błędu ([`indexed-drawing.md`](indexed-drawing.md), sekcja 7, pułapka 1) |
| konstruktor przenoszący, przypisanie przenoszące i przypisanie do siebie dla obu klas, potem rysowanie | środek okna czerwony, `glGetError` czysty |
| `Buffer::bind()`, potem `glGetIntegerv(GL_ARRAY_BUFFER_BINDING)` | identyfikator tego bufora |
| usunięcie bufora, gdy jego VAO nie jest bieżący, i gdy jest bieżący | jak w tabeli z sekcji 5.7 |
| `glGetError` po zniszczeniu wszystkich obiektów | `GL_NO_ERROR` |

**Trójkąt.** Pierwszą geometrią projektu był jeden trójkąt rysowany przez `glDrawArrays`, bez macierzy i bez indeksów. Test z ukrytym oknem potwierdził wtedy układ przeplatany (krok 24, przesunięcia 0 i 12): rogi miały kolory swoich wierzchołków, a środek ciężkości jedną trzecią każdego. Tego kodu już nie ma, zastąpiła go kostka.

Test gotowej kostki, czyli prawdziwego kodu rysującego z `NightMazeApp`, jest w [`indexed-drawing.md`](indexed-drawing.md), sekcja 5.7.

Na Windowsie klasy nie były jeszcze kompilowane ani uruchamiane.

## 6. Panel ImGui

`Buffer` i `VertexArray` nie mają elementu w panelu i nie jest on planowany: nie mają stanu, który warto oglądać albo zmieniać na żywo. Skutkiem ich błędnego użycia jest brak geometrii na ekranie albo linia `[error] GL_INVALID_OPERATION after glDrawElements(...)` w konsoli od `GL_CHECK`.

## 7. Pułapki

1. **Zły bufor związany w chwili `setFloatAttribute`.** Atrybut czyta z bufora związanego z `GL_ARRAY_BUFFER` w chwili wywołania. Utworzenie drugiego `Buffer(GL_ARRAY_BUFFER, ...)` między utworzeniem pierwszego a `setFloatAttribute` zmienia to wiązanie i atrybut zostaje przypisany do drugiego bufora. Gdy z `GL_ARRAY_BUFFER` nie jest związane nic, a przesunięcie jest różne od zera, `glVertexAttribPointer` zgłasza `GL_INVALID_OPERATION`.
2. **Zły krok albo przesunięcie.** Nie dają żadnego błędu OpenGL. Shader dostaje liczby z niewłaściwych miejsc bufora: bryła jest zdeformowana, kolory są pozycjami, a przy zbyt dużym kroku karta czyta poza buforem (wynik nieokreślony). Typowe pomyłki: krok podany w liczbie `float` zamiast w bajtach (6 zamiast 24), krok równy rozmiarowi jednego atrybutu zamiast całego wierzchołka, przesunięcie w liczbie `float` zamiast w bajtach (3 zamiast 12).
3. **`sizeof` na wskaźniku zamiast na tablicy.** `sizeof(vertices)` daje rozmiar całej tablicy w bajtach tylko wtedy, gdy `vertices` jest tablicą (`float[9]` albo `std::array<float, 9>`). Gdy tablica została przekazana do funkcji jako `const float*`, `sizeof` zwraca rozmiar **wskaźnika** (8 bajtów) i do bufora trafiają dwie liczby `float`. Dla `std::vector` `sizeof` zwraca rozmiar samego obiektu wektora (zwykle 24 bajty), a nie danych: tam trzeba `vertices.size() * sizeof(float)`. Projekt liczy rozmiar zawsze tym drugim sposobem (`VERTICES.size() * sizeof(float)`), bo działa on dla każdego kontenera.
4. **Liczba elementów zamiast bajtów w konstruktorze `Buffer`.** `Buffer(GL_ARRAY_BUFFER, VERTICES.data(), VERTICES.size())` wysyła 144 bajty zamiast 576, czyli 6 wierzchołków zamiast 24. Parametr nazywa się `sizeInBytes` właśnie po to.
5. **Niewłączony atrybut.** Samo `glVertexAttribPointer` bez `glEnableVertexAttribArray` nie wystarcza: atrybut pozostaje wyłączony i shader dostaje wartość stałą (domyślnie zera z `w = 1`). `setFloatAttribute` robi oba kroki, więc ta pułapka dotyczy kodu pisanego z pominięciem klasy.
6. **Numer atrybutu niezgodny z shaderem.** `setFloatAttribute(1, ...)` przy `layout(location = 0)` w shaderze nie daje błędu. Shader czyta atrybut 0, który jest wyłączony, i wszystkie wierzchołki lądują w tym samym punkcie: trójkąty o zerowym polu, czyli nic.
7. **Usunięcie bufora, którego VAO jeszcze używa.** Skutek zależy od tego, czy VAO jest bieżący (sekcja 5.7). `Buffer` zadeklarowany jako zmienna lokalna w funkcji przygotowującej geometrię ginie na jej końcu, a VAO zostaje bez danych albo z danymi, których nazwa już nie istnieje. Bufor ma żyć tak długo jak VAO.
8. **Zniszczenie po kontekście albo utworzenie przed nim.** Destruktory wołają `glDeleteBuffers` i `glDeleteVertexArrays`, konstruktory `glGen*`. Wszystkie wymagają bieżącego kontekstu. Obiekty mają być polami klasy pochodnej od `core::Application` ([`README.md`](README.md), sekcja 5).
9. **Rysowanie bez VAO.** W profilu Core `glDrawArrays` i `glDrawElements` bez związanego VAO daje `GL_INVALID_OPERATION` (sekcja 2.5). Kod z poradnika dla profilu zgodności, który konfiguruje atrybuty bez VAO, na Macu nie narysuje nic.
10. **`GL_FLOAT` a typ danych w C++.** `setFloatAttribute` zakłada, że w buforze leżą liczby `float` (4 bajty). Tablica `double` wysłana do bufora zostanie odczytana jako dwa razy więcej bezsensownych liczb `float`.
11. **Kopiowanie opakowania.** Jak w `Shader`: kopia miałaby ten sam identyfikator i dwa destruktory usuwałyby ten sam obiekt. `= delete` zamienia to w błąd kompilacji ([`README.md`](README.md), sekcja 2.2).

Pułapki dotyczące bufora indeksów, liczby i typu indeksów, kierunku nawijania i kolejności pól w `NightMazeApp` są w [`indexed-drawing.md`](indexed-drawing.md), sekcja 7.

## 8. Ćwiczenia

Zmiany w `NightMazeApp.cpp` wymagają zbudowania programu (`make run`). Po każdym ćwiczeniu wycofaj zmianę (`git checkout src/game`).

1. **Zły krok.** Zmień `VERTEX_STRIDE` na `POSITION_COMPONENTS * sizeof(float)` (12 zamiast 24). Zanim uruchomisz, policz na kartce, jaką pozycję i jaki kolor odczyta karta dla wierzchołków 0, 1 i 2 (wierzchołek numer `i` zaczyna się w bajcie `przesunięcie + i * 12`). Uruchom i porównaj. Czy w konsoli jest błąd?
2. **Złe przesunięcie.** Przywróć krok i zmień `COLOR_OFFSET` na 0. Jakie kolory mają teraz rogi i skąd się wzięły? (Ujemna składowa koloru jest przycinana do 0.) Dlaczego ściany przestały być jednolite?
3. **Krok w złych jednostkach.** Ustaw `VERTEX_STRIDE` na 6 (liczba `float`, a nie bajtów). Opisz wynik. To jedna z najczęstszych pomyłek.
4. **Rozmiar w elementach.** W konstruktorze podaj jako rozmiar bufora wierzchołków samo `VERTICES.size()` (144 zamiast 576 bajtów). Ile pełnych wierzchołków trafiło na kartę? Co widać i dlaczego wynik może być inny przy każdym uruchomieniu?
5. **Wyłączony atrybut.** Usuń drugie wywołanie `setFloatAttribute` (kolor). Jaki kolor ma kostka i skąd ta wartość (sekcja 7, pułapka 5)?
6. **Krok i przesunięcie na kartce.** Wierzchołek ma pozycję (`vec3`), normalną (`vec3`) i współrzędne tekstury (`vec2`), w tej kolejności, w układzie przeplatanym. Podaj krok oraz przesunięcie każdego atrybutu w bajtach i zapisz stałe w stylu `COLOR_OFFSET`. W którym bajcie bufora zaczynają się współrzędne tekstury wierzchołka numer 2?
7. **Przeniesienie na kartce.** Dla `gfx::Buffer a(GL_ARRAY_BUFFER, data, size); gfx::Buffer b = std::move(a);` zapisz `m_id` i `m_target` obu obiektów po każdej linii (przyjmij identyfikator 1). Ile razy i z jaką wartością zostanie zawołane `glDeleteBuffers`? Czy VAO, który zapisał bufor 1 przed przeniesieniem, trzeba konfigurować ponownie?
8. **Przesunięcie jako wskaźnik.** Jaką wartość ma `offsetAsPointer` w drugim wywołaniu `setFloatAttribute` z konstruktora? Co by się stało, gdyby OpenGL naprawdę odczytał pamięć pod tym adresem, i dlaczego tego nie robi? Jaką wartość ma ostatni argument `glDrawElements` i co ona znaczy?

Ćwiczenia na danych kostki, indeksach i wywołaniu `glDrawElements` są w [`indexed-drawing.md`](indexed-drawing.md), sekcja 8.

## 9. Pytania kontrolne

1. **Czym jest VBO, a czym VAO?**
   VBO to bufor, czyli blok pamięci na karcie z danymi wierzchołków. VAO to obiekt z opisem, jak te dane czytać: które atrybuty są włączone, ich format, z którego bufora każdy czyta, oraz który bufor zawiera indeksy. VBO to dane bez znaczenia, VAO to znaczenie bez danych.

2. **Co dokładnie zapisuje VAO, a czego nie?**
   Zapisuje dla każdego atrybutu: włączenie, liczbę składowych, typ, normalizację, krok, przesunięcie i bufor, z którego atrybut czyta. Zapisuje też wiązanie `GL_ELEMENT_ARRAY_BUFFER`. Nie zapisuje wiązania `GL_ARRAY_BUFFER` (to stan globalny, czytany tylko w chwili `glVertexAttribPointer`), danych ani programu shaderów.

3. **Co to jest krok i przesunięcie? Podaj wartości dla pozycji i koloru po trzy `float`.**
   Krok to odległość w bajtach między początkami kolejnych wierzchołków: 6 * 4 = 24, taki sam dla obu atrybutów. Przesunięcie to początek atrybutu wewnątrz wierzchołka: 0 dla pozycji, 12 dla koloru.

4. **Dlaczego ostatni parametr `glVertexAttribPointer` jest wskaźnikiem i co do niego trafia?**
   Z powodów historycznych: przed buforami był to adres tablicy w pamięci programu. Gdy z `GL_ARRAY_BUFFER` związany jest bufor, wartość jest odczytywana jako przesunięcie w bajtach od początku bufora. Przekazuję więc liczbę zamienioną przez `reinterpret_cast` na `const void*`. Niczego się przez ten wskaźnik nie odczytuje.

5. **Skąd `setFloatAttribute` wie, z którego bufora ma czytać atrybut?**
   Nie dostaje go jako parametru. `glVertexAttribPointer` zapisuje w VAO bufor związany z `GL_ARRAY_BUFFER` w chwili wywołania. Wołający musi mieć związany właściwy `Buffer`: konstruktor zostawia go związanego, a `bind()` wiąże ponownie.

6. **Dlaczego `setFloatAttribute` zaczyna od `bind()`?**
   `glEnableVertexAttribArray` i `glVertexAttribPointer` zmieniają stan bieżącego VAO. Funkcja składowa ma konfigurować swój obiekt niezależnie od tego, co było związane wcześniej.

7. **Dlaczego profil Core wymaga VAO?**
   W profilu Core nie ma domyślnego VAO: numer 0 oznacza brak obiektu. Stan atrybutów nie ma gdzie być zapisany, a rysowanie bez związanego VAO kończy się `GL_INVALID_OPERATION`.

8. **Co robi `glBufferData` i co oznacza `GL_STATIC_DRAW`?**
   Przydziela pamięć bufora związanego z danym celem i kopiuje do niej dane z programu. `GL_STATIC_DRAW` to podpowiedź dla sterownika: dane ustawiane raz, używane wiele razy do rysowania. `DYNAMIC` oznacza częste zmiany, `STREAM` dane używane najwyżej kilka razy. To tylko podpowiedź, nie ograniczenie.

9. **Dlaczego rozmiar w `glBufferData` jest rzutowany?**
   Funkcja przyjmuje `GLsizeiptr`, typ ze znakiem, a `sizeof` i mój parametr dają `std::size_t`, typ bez znaku. `static_cast` czyni zamianę jawną i usuwa ostrzeżenie kompilatora.

10. **Jak numer atrybutu w C++ łączy się z shaderem?**
    `index` w `setFloatAttribute` to ten sam numer co `layout(location = index)` przy zmiennej `in` shadera wierzchołków. OpenGL nie sprawdza zgodności liczby składowych ani numeru: pomyłka daje złe dane, a nie błąd.

11. **Co się stanie, gdy `Buffer` zginie, a `VertexArray`, który z niego czyta, żyje dalej?**
    Jeśli VAO nie jest wtedy bieżący, OpenGL trzyma dane bufora, dopóki VAO się do nich odwołuje, i rysowanie działa. Jeśli jest bieżący, bufor zostaje od niego odłączony i rysowanie daje `GL_INVALID_OPERATION`. Dlatego bufor ma żyć tak długo jak VAO.

12. **Dlaczego `Buffer` pamięta `m_target`?**
    Żeby `bind()` nie potrzebowało parametru i żeby bufora nie dało się związać z innym celem niż ten, dla którego powstał.

13. **Dlaczego w `VertexArray.cpp` jest komentarz `NOLINTNEXTLINE`?**
    clang-tidy zgłasza zamianę liczby na wskaźnik (`performance-no-int-to-ptr`). Tutaj wymaga jej API OpenGL, więc kontrola jest wyłączona dla tej jednej linii, z komentarzem wyjaśniającym powód.

Pytania o indeksy, kostkę i `glDrawElements` są w [`indexed-drawing.md`](indexed-drawing.md), sekcja 9.

## 10. Źródła

- LearnOpenGL, rozdział "Hello Triangle" (<https://learnopengl.com/Getting-started/Hello-Triangle>): VBO, VAO, `glVertexAttribPointer`, rysunki z krokiem i przesunięciem.
- LearnOpenGL, rozdział "Shaders" (<https://learnopengl.com/Getting-started/Shaders>), część "More attributes": układ przeplatany z pozycją i kolorem.
- docs.gl (<https://docs.gl>), strony dla OpenGL 4: `glGenBuffers`, `glBindBuffer`, `glBufferData` (tabela podpowiedzi użycia), `glDeleteBuffers`, `glGenVertexArrays`, `glBindVertexArray`, `glDeleteVertexArrays`, `glEnableVertexAttribArray`, `glVertexAttribPointer`.
- Khronos OpenGL Wiki: "Vertex Specification" (<https://www.khronos.org/opengl/wiki/Vertex_Specification>, co przechowuje VAO, wiązanie bufora indeksów, przesunięcie jako wskaźnik), "Buffer Object" (<https://www.khronos.org/opengl/wiki/Buffer_Object>), "OpenGL Object" (<https://www.khronos.org/opengl/wiki/OpenGL_Object>, usuwanie obiektu dołączonego do innego obiektu).
- Dokumenty w tym repozytorium: [`README.md`](README.md) (RAII i przenoszenie w `gfx`), [`shaders.md`](shaders.md), [`shader-class.md`](shader-class.md), [`indexed-drawing.md`](indexed-drawing.md) (ciąg dalszy: indeksy i kostka), [`../core/gl-check.md`](../core/gl-check.md), [`../../guides/project-structure.md`](../../guides/project-structure.md) (sekcja 3.6 o clang-tidy).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o tablicach wierzchołków i obiektach buforowych).
- "OpenGL. Księga eksperta" (rozdziały o buforach wierzchołków i tablicach wierzchołków).
