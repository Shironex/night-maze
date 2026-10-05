# Moduł gfx: uniformy

Kamień milowy: M1. Temat wykładu: 2 (Programowalny potok).
Kod: funkcje `setMat4`, `setInt` i `setVec3` w [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) i [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp), uniformy w [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp).

Część modułu `gfx`. Wstęp do całego modułu jest w [`README.md`](README.md). Ten dokument jest dalszym ciągiem [`shaders.md`](shaders.md) (potok, język GLSL, shader `basic.vert`) i [`shader-class.md`](shader-class.md) (reszta klasy `gfx::Shader`). Skąd biorą się same macierze, opisują [`../scene/transforms.md`](../scene/transforms.md) (macierz modelu) i [`../scene/camera.md`](../scene/camera.md) (macierz widoku i rzutowania). Co dzieje się z uniformami po przeładowaniu shadera, widać w [`shader-hot-reload.md`](shader-hot-reload.md). Ten dokument korzysta z makra `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)) i z typów GLM ([`../../libraries/glm.md`](../../libraries/glm.md)).

## 1. Po co to jest

Shader wierzchołków dostaje pozycję wierzchołka z bufora, ale żeby postawić obiekt w scenie, potrzebuje jeszcze trzech macierzy: modelu, widoku i rzutowania. Te macierze nie są danymi wierzchołka: są takie same dla całej kostki i zmieniają się najwyżej raz na klatkę. Do przekazywania takich wartości z C++ do shadera służą **uniformy**.

W projekcie uniformy to na dziś trzy linie `uniform mat4` w `basic.vert` i jedna funkcja klasy `gfx::Shader`, `setMat4`, którą `NightMazeApp` woła trzy razy w każdej klatce. Od kamienia milowego M2 + M3 klasa ma jeszcze dwa settery, `setInt` i `setVec3` (sekcja 5.4), przygotowane dla tekstur. Gra ich jeszcze nie woła, bo żaden shader projektu nie ma uniformu takiego typu. Kodu jest mało, ale miejsc na pomyłkę, której OpenGL nie zgłosi, jest kilka:

| Pomyłka | Skutek |
|---|---|
| literówka w nazwie uniformu w C++ | pusty ekran bez żadnego komunikatu (pułapka 1) |
| `setMat4` przed `use()` | macierz trafia do innego programu albo wywołanie kończy się błędem (pułapka 2) |
| uniform ustawiony raz, przy starcie | wartość przepada po pierwszym przeładowaniu shadera (pułapka 3) |

Dlatego ten temat ma własny dokument, choć funkcja ma dwie linie.

## 2. Teoria

### 2.1 Atrybut a uniform

Shader ma dwa rodzaje danych wejściowych z C++. **Atrybut** (`in` w shaderze wierzchołków) ma inną wartość dla każdego wierzchołka i pochodzi z bufora. **Uniform** ma jedną wartość dla całego wywołania rysującego: wszystkie wierzchołki i wszystkie fragmenty widzą to samo. Typowe uniformy to macierze, kolor i pozycja światła, czas, numer jednostki teksturującej dla samplera.

| Własność | Znaczenie |
|---|---|
| należy do **programu** | wartość jest zapisana w obiekcie programu, nie w kontekście i nie w VAO. Dwa programy z uniformem o tej samej nazwie mają dwie osobne wartości |
| ma **położenie** (location) | liczbę całkowitą nadaną przy linkowaniu. O położenie pyta się po nazwie: `glGetUniformLocation(program, "uModel")` |
| jest **trwały** | raz ustawiona wartość zostaje w programie do następnego ustawienia. `glUseProgram` jej nie zeruje |
| po linkowaniu ma wartość **zero** | nowy program (także ten po `reload()`) zaczyna z samymi zerami. Macierz zerowa zamienia każdy wierzchołek w punkt `(0, 0, 0, 0)`, czyli nic nie widać |
| może być **nieaktywny** | uniform, który nie wpływa na wynik shadera, kompilator usuwa. Dla OpenGL taki uniform nie istnieje: jego położenie to -1 |

### 2.2 Dwie rodziny funkcji

W OpenGL 4.1 są dwie rodziny funkcji ustawiających uniform:

| Funkcja | Do którego programu pisze | Uwagi |
|---|---|---|
| `glUniformMatrix4fv(location, ...)` i reszta `glUniform*` | do programu **bieżącego**, czyli wybranego ostatnim `glUseProgram` | klasyczna postać, ta z wykładu i z LearnOpenGL. Wymaga `use()` przed ustawieniem |
| `glProgramUniformMatrix4fv(program, location, ...)` i reszta `glProgramUniform*` | do programu podanego w pierwszym argumencie | w rdzeniu od OpenGL 4.1. Nie zależy od bieżącego programu |

Projekt używa pierwszej. Druga byłaby odporniejsza na pomyłkę "zapomniałem `use()`", ale wybrałem postać, którą pokazuje wykład i każdy poradnik, żeby kod dało się porównać z materiałami bez tłumaczenia. Zależność od bieżącego programu jest przy tym rzeczą, którą i tak trzeba rozumieć: tak samo działają bufory i VAO ([`buffers-vao.md`](buffers-vao.md), sekcja 2.2: bufor wierzchołków i cel wiązania).

### 2.3 Położenie -1

Położenie -1 jest w `glUniform*` celowo dozwolone: wywołanie nic nie robi i **nie zgłasza błędu**. Dzięki temu można bezkarnie ustawiać uniform, który kompilator akurat usunął. Ceną jest to, że literówka w nazwie wygląda dokładnie tak samo (pułapka 1).

## 3. Jak to działa w OpenGL

Uniform ustawia się w zlinkowanym programie, który jest bieżący. Zbudowanie programu i wybranie go przez `glUseProgram` to kroki od 1 do 15 w [`shader-class.md`](shader-class.md) (sekcja 3.1). Ustawienie uniformu typu `mat4`, co klatkę, po `glUseProgram` (krok 14 tamtej tabeli):

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glGetUniformLocation(program, name)` | Zwraca położenie aktywnego uniformu o podanej nazwie w zlinkowanym programie albo -1, gdy takiego nie ma. Nie wymaga, żeby program był bieżący |
| 2 | `glUniformMatrix4fv(location, count, transpose, value)` | Kopiuje `count` macierzy 4 x 4 (po 16 liczb `float`) spod wskaźnika `value` do uniformu **bieżącego** programu. `transpose` równe `GL_FALSE` znaczy: liczby leżą kolumnami, tak jak chce OpenGL. Położenie -1 jest ignorowane bez błędu. Gdy żaden program nie jest bieżący: `GL_INVALID_OPERATION` |

Obie funkcje są w rdzeniu OpenGL od wersji 2.0, więc są dostępne w 4.1 Core i w nagłówku GLAD projektu. Na diagramie sekwencji w [`shader-class.md`](shader-class.md) (sekcja 3.2) stoją zaraz po `glUseProgram`.

## 4. Shadery

Uniformy projektu są w shaderze wierzchołków, [`assets/shaders/basic.vert`](../../../assets/shaders/basic.vert). To trzy deklaracje, `uniform mat4 uModel;`, `uniform mat4 uView;` i `uniform mat4 uProjection;`, oraz jedno wyrażenie, które ich używa: `gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);`. Cały plik z opisem każdej linii jest w [`shaders.md`](shaders.md) (sekcja 4.1). Shader fragmentów `basic.frag` nie ma uniformów.

Wszystkie trzy uniformy są aktywne, bo każdy wpływa na `gl_Position`. Wystarczy usunąć macierz z wyrażenia, żeby kompilator usunął uniform, a jego położenie zmieniło się na -1 (ćwiczenie 1).

**Trzy uniformy zamiast jednego.** Shader mógłby dostać jedną gotową macierz, iloczyn wszystkich trzech policzony w C++. Tak robi wiele prawdziwych rendererów: jedno mnożenie macierzy na wierzchołek zamiast trzech. W projekcie macierze są osobno celowo: każdą da się podmienić i obejrzeć skutek (ćwiczenia w sekcji 8 dokumentów [`../scene/transforms.md`](../scene/transforms.md) i [`../scene/camera.md`](../scene/camera.md)), a przy oświetleniu w M4 shader i tak będzie potrzebował pozycji w przestrzeni świata, czyli wyniku samego `uModel`.

## 5. Kod w projekcie

Uniformy dotykają dwóch miejsc w kodzie: funkcji `Shader::setMat4` w `src/gfx/` i jej trzech wywołań w `NightMazeApp::onRender`. Reszta klasy `gfx::Shader` jest opisana w [`shader-class.md`](shader-class.md) (sekcja 5).

### 5.1 `Shader::setMat4`

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

**Program musi być w użyciu.** `glUniformMatrix4fv` nie przyjmuje identyfikatora programu: pisze do programu bieżącego (sekcja 2.2). `setMat4` **nie woła** `use()` samo. Gdyby wołało, ustawienie uniformu po cichu zmieniałoby bieżący program, a trzy macierze oznaczałyby trzy zbędne `glUseProgram`. Kolejność "najpierw `use()`, potem `setMat4`" należy do wołającego i jest zapisana w komentarzu Doxygen. Jej złamanie nie zawsze daje błąd: macierz trafia wtedy do innego programu (pułapka 2).

**Nieznana nazwa jest ignorowana po cichu.** Dla nazwy, której program nie ma, `glGetUniformLocation` zwraca -1, a `glUniformMatrix4fv` z położeniem -1 nic nie robi i nie zgłasza błędu. `setMat4` tego nie sprawdza i niczego nie loguje. Rozważałem `logWarn` przy -1 i odrzuciłem z dwóch powodów. Funkcja jest wołana co klatkę, więc ostrzeżenie pojawiałoby się 60 razy na sekundę i zalało konsolę. Po drugie -1 nie zawsze jest pomyłką: uniform usunięty przez kompilator jako nieużywany (na przykład w trakcie eksperymentu z shaderem, ćwiczenie 1) też ma położenie -1, a ustawianie go jest poprawne. Skutek trzeba po prostu znać: literówka w nazwie daje pusty ekran bez żadnego komunikatu (pułapka 1).

Te trzy decyzje dotyczą tak samo dwóch pozostałych setterów, `setInt` i `setVec3` (sekcja 5.4). Uniformy innych typów (`float`, `vec4`) dojdą wtedy, gdy shader będzie ich potrzebował.

### 5.2 Użycie w `NightMazeApp`

**Nazwy uniformów** w anonimowej przestrzeni nazw [`NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp):

```cpp
// Names of the matrix uniforms: the same as the "uniform mat4" lines in basic.vert.
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";
```

To jedyny łącznik między kodem C++ a liniami `uniform mat4 ...` w `basic.vert`: zwykłe napisy. Kompilator C++ nie wie nic o shaderze, więc literówki nie wykryje (pułapka 1).

**Wysłanie macierzy** w `NightMazeApp::onRender`. Po `m_shader.use();` stoją trzy wywołania: `m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());`, `m_shader.setMat4(VIEW_UNIFORM, m_camera.viewMatrix(eye));` i `m_shader.setMat4(PROJECTION_UNIFORM, m_camera.projectionMatrix(aspectRatio));`. Cały fragment `onRender` z opisem każdej linii jest w [`shaders.md`](shaders.md) (sekcja 5.1), a skąd pochodzą same macierze, opisują [`../scene/transforms.md`](../scene/transforms.md) (sekcja 5.3) i [`../scene/camera.md`](../scene/camera.md) (sekcja 5.5).

Macierze są wysyłane **co klatkę**, choć kostka się nie rusza. Powody są trzy: macierz rzutowania zależy od rozmiaru okna, który może się zmienić w każdej chwili. Po `reload()` nowy program ma wszystkie uniformy wyzerowane (sekcja 2.1), więc wartości wysłane raz przy starcie przepadłyby po pierwszym naciśnięciu "Reload shaders". A kamera lata i obraca się, więc macierz widoku jest inna w każdej klatce ruchu.

### 5.3 Jak to zostało sprawdzone

**Kostka i uniformy.** Po zamianie trójkąta na kostkę sprawdziłem prawdziwy kod rysujący testem z ukrytym oknem, który wołał `NightMazeApp::onRender` i czytał obraz przez `glReadPixels` (pełna tabela: [`indexed-drawing.md`](indexed-drawing.md), sekcja 5.7). Wyniki dotyczące shadera i uniformów:

| Próba | Wynik |
|---|---|
| prawdziwe `basic.vert`, `basic.frag` i trzy wywołania `setMat4` | środek okna czerwony (ściana przednia), róg w kolorze tła, w klatce dokładnie trzy kolory ścian, `glGetError` czysty |
| literówka w nazwie uniformu w C++ (`"uModle"` zamiast `"uModel"`, w kopii pliku poza repozytorium) | **pusta klatka**: każdy piksel w kolorze tła. `glGetError` czysty, w konsoli żadnej linii `[error]`, program działa dalej |
| `gl_Position = vec4(aPosition, 1.0);` (bez macierzy) | zielony prostokąt na środku, połowa szerokości i wysokości okna. Zielona jest ściana **tylna**: bez macierzy rzutowania mniejsze z znaczy "bliżej". Trzy uniformy stają się nieaktywne, błędu brak |
| `uModel * uView * uProjection` (odwrotna kolejność) | pusta klatka, błędu brak |
| `uView * uModel` (bez rzutowania) | pusta klatka: kostka ma w przestrzeni widoku z od -3,9 do -2,1, czyli poza zakresem od -1 do 1, i jest w całości przycinana |
| zamienione `location = 0` i `location = 1` | pusta klatka, błędu brak |

Program `night_maze` uruchomiony na około 3 sekundy z katalogu repozytorium wypisał dwie linie `[info]` i żadnej linii `[error]`.

Wcześniejsze próby samej klasy `Shader` (kompilacja, linkowanie, przenoszenie) są w [`shader-class.md`](shader-class.md) (sekcja 5.10).

### 5.4 `setInt` i `setVec3`

Dwa settery dodane w kamieniu milowym M2 + M3 razem z klasą `gfx::Texture2D` ([`textures.md`](textures.md)). **Gra ich jeszcze nie woła**: `basic.vert` i `basic.frag` nie mają uniformu typu `int`, `sampler2D` ani `vec3`. Pierwszym użytkownikiem będzie shader z teksturą w następnym kroku.

Deklaracje w [`Shader.hpp`](../../../src/gfx/Shader.hpp):

```cpp
/// Sets the uniform variable of type int called name to value (glUniform1i). This is
/// also the setter for sampler uniforms (sampler2D): a sampler holds the NUMBER OF A
/// TEXTURE UNIT, not a texture id, so setInt("uTexture", 0) together with
/// Texture2D::bind(0) connects the sampler to that texture. GLSL 4.20 can write the
/// unit in the shader, layout(binding = 0), but GLSL 4.10 (the newest on macOS)
/// cannot, so it is set from C++. The rules of setMat4 apply: use() first, and an
/// unknown name is ignored.
void setInt(const char* name, int value) const;

/// Sets the uniform variable of type vec3 called name to value (glUniform3fv): a
/// color, a position or a direction. The rules of setMat4 apply: use() first, and an
/// unknown name is ignored.
void setVec3(const char* name, const glm::vec3& value) const;
```

Implementacja w [`Shader.cpp`](../../../src/gfx/Shader.cpp):

```cpp
void Shader::setInt(const char* name, int value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1i: one value of type int. A sampler uniform must be set with exactly this
    // function: the float version (glUniform1f) raises GL_INVALID_OPERATION for a sampler.
    // OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniform1i(location, value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // 1: one vector (more only for a uniform that is an array). value_ptr gives the
    // address of its 3 floats. OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniform3fv(location, 1, glm::value_ptr(value)));
}
```

Obie funkcje mają tę samą budowę co `setMat4`: wyszukanie położenia, potem jedno wywołanie z rodziny `glUniform*`. Różni się tylko to wywołanie. Nazwy funkcji tej rodziny czyta się tak: liczba to liczba składowych, litera to typ (`i` to `int`, `f` to `float`), a `v` na końcu znaczy, że wartości podaje się wskaźnikiem, a nie osobnymi argumentami.

| Funkcja klasy | Wywołanie OpenGL | Typ uniformu w GLSL |
|---|---|---|
| `setMat4` | `glUniformMatrix4fv(location, 1, GL_FALSE, wskaźnik)` | `mat4` |
| `setInt` | `glUniform1i(location, value)` | `int`, `bool` i wszystkie samplery (`sampler2D`) |
| `setVec3` | `glUniform3fv(location, 1, wskaźnik)` | `vec3` |

| Element | Co robi i dlaczego |
|---|---|
| `int value` | zwykły `int` z C++. `GLint`, którego chce `glUniform1i`, to ten sam typ |
| `const glm::vec3& value` | referencja do stałej, jak macierz w `setMat4`. Przyjmuje też wartość tymczasową, na przykład `glm::vec3(1.0F, 0.0F, 0.0F)` |
| `1` w `glUniform3fv` | liczba wektorów, nie liczba składowych. Trójka jest w nazwie funkcji |
| `glm::value_ptr(value)` | wskaźnik na pierwszą z trzech liczb `float` wektora ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.9) |

**`setInt` a samplery.** To jest powód, dla którego `setInt` powstało razem z teksturami. Uniform typu `sampler2D` nie przechowuje identyfikatora tekstury, tylko **numer jednostki teksturującej** ([`textures.md`](textures.md), sekcja 2.7). `shader.setInt("uTexture", 0)` mówi samplerowi "czytaj z jednostki 0", a `texture.bind(0)` wiąże z tą jednostką teksturę. W GLSL 4.20 numer można wpisać w shaderze (`layout(binding = 0)`), ale projekt używa GLSL 4.10, najnowszej wersji na macOS, gdzie tego zapisu nie ma.

Sampler trzeba ustawiać dokładnie funkcją `glUniform1i`. Zmierzone w programie sprawdzającym z [`textures.md`](textures.md) (sekcja 5.9): `glUniform1f` i `glUniform1ui` na uniformie `sampler2D` dają `GL_INVALID_OPERATION`. To jedno z miejsc, w których OpenGL **sprawdza** zgodność typu uniformu z funkcją, i `GL_CHECK` taki błąd wypisze.

**Jak to zostało sprawdzone.** W tym samym programie (Windows, ukryte okno, shader z uniformami `sampler2D uTexture` i `vec3 uTint`): `setInt("uTexture", 2)` razem z `texture.bind(2)` daje na ekranie kolory tekstury, po `setVec3("uTint", (1, 0, 0))` zielony piksel tekstury staje się czarny (shader mnoży kolor przez `uTint`), a oba settery wołane z nazwą, której shader nie ma, nie zgłaszają błędu i nic nie zmieniają. Na macOS tego nie uruchamiałem.

## 6. Panel ImGui

Uniformy nie mają własnego panelu: żaden panel nie pokazuje ich położeń ani wartości. Pośrednio widać je w dwóch miejscach. Panel "Camera" ([`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 6) zmienia pola kamery, z których co klatkę powstają macierze dla `uView` i `uProjection`. Przycisk "Reload shaders" w panelu "Shaders" ([`shader-hot-reload.md`](shader-hot-reload.md), sekcja 6) tworzy nowy program z wyzerowanymi uniformami, a obraz nie znika tylko dlatego, że macierze są wysyłane co klatkę (sekcja 5.2).

## 7. Pułapki

1. **Położenie -1: literówka w nazwie uniformu albo uniform usunięty przez kompilator.** `glGetUniformLocation` zwraca -1 w dwóch sytuacjach, których nie da się od siebie odróżnić. Pierwsza to nazwa, której w shaderze nie ma: `setMat4("uModle", ...)`. Druga to uniform, który kompilator GLSL usunął, bo nie wpływa na wynik shadera (zadeklarowany, ale nieużyty, albo użyty tylko w obliczeniu, którego wynik jest potem ignorowany). Ustawianie uniformu o położeniu -1 jest po cichu ignorowane: nie ma błędu OpenGL, nie ma linii w konsoli, `setMat4` też niczego nie loguje (sekcja 5.1). Wartość po prostu "nie dochodzi", a uniform zostaje z zerami. Zmierzone w teście (sekcja 5.3): literówka w `"uModel"` daje macierz zerową w shaderze, wszystkie wierzchołki w jednym punkcie i **pusty ekran bez żadnego komunikatu**. Gdy kostka znika po zmianie w kodzie C++, pierwszą rzeczą do sprawdzenia są trzy napisy z nazwami uniformów.
2. **`setMat4` przed `use()`.** `glUniform*` pisze do programu bieżącego. Bez `use()` macierz trafia do programu, który akurat jest bieżący, czyli do tego, który ktoś wybrał ostatnio. Jeśli tamten program nie ma uniformu pod tym położeniem albo ma uniform innego typu, OpenGL zgłasza `GL_INVALID_OPERATION` i `GL_CHECK` to wypisze. Jeśli typ się zgadza, błędu nie ma, a zepsuty zostaje cudzy shader. Gdy bieżącego programu nie ma wcale, błąd jest zawsze.
3. **Uniformy po `reload()`.** Nowy program zaczyna z samymi zerami (sekcja 2.1). Kod, który ustawia uniform raz, przy starcie, traci tę wartość po pierwszym przeładowaniu shadera. W projekcie macierze są wysyłane co klatkę, więc problemu nie ma, ale każdy uniform ustawiany "raz" trzeba po `reload()` ustawić ponownie.
4. **`GL_TRUE` jako `transpose`.** Macierz z GLM jest już w układzie kolumnowym. `GL_TRUE` transponuje ją: przesunięcie ląduje w ostatnim wierszu zamiast w ostatniej kolumnie i obraz znika albo jest zdeformowany.
5. **Zła kolejność mnożenia w shaderze.** `uModel * uView * uProjection * vec4(...)` kompiluje się bez ostrzeżeń i daje pusty ekran (zmierzone, sekcja 5.3). Macierz najbliżej wektora działa pierwsza, więc poprawna kolejność to `uProjection * uView * uModel`.
6. **Sampler ustawiony złą funkcją.** Uniform typu `sampler2D` przyjmuje tylko `glUniform1i`. `glUniform1f` i `glUniform1ui` dają `GL_INVALID_OPERATION` (zmierzone, sekcja 5.4). W projekcie służy do tego `setInt`.
7. **Do samplera trafia identyfikator tekstury.** Sampler chce numeru jednostki teksturującej, czyli tej samej liczby, którą dostało `Texture2D::bind`. `setInt("uTexture", texture.id())` nie zgłasza błędu, a sampler wskazuje jednostkę, na której zwykle nic nie ma: obraz jest czarny ([`textures.md`](textures.md), sekcja 7, pułapka 4).

Pułapki dotyczące języka GLSL są w [`shaders.md`](shaders.md) (sekcja 7), a samej klasy `Shader` w [`shader-class.md`](shader-class.md) (sekcja 7).

## 8. Ćwiczenia

Zasady pracy są takie same jak w [`shaders.md`](shaders.md) (sekcja 8). Po zmianie pliku `.vert` zapisz go i naciśnij `Reload shaders` w panelu Shaders, bez kompilacji C++. Ćwiczenia 3, 4 i 5 zmieniają kod C++, więc wymagają zbudowania i uruchomienia programu od nowa. Po każdym ćwiczeniu przywróć pliki.

1. **Bez macierzy.** W `basic.vert` zamień linię z `gl_Position` na `gl_Position = vec4(aPosition, 1.0);` i naciśnij `Reload shaders`. Na środku jest zielony prostokąt o połowie szerokości i wysokości okna. Wyjaśnij trzy rzeczy: dlaczego prostokąt, a nie kwadrat, dlaczego widać ścianę **tylną** (zieloną), a nie przednią, i jakie położenie mają teraz uniformy `uModel`, `uView`, `uProjection` (sekcja 2.1). Dlaczego program C++, który nadal woła `setMat4`, nie zgłasza błędu?
2. **Przesunięcie w przestrzeni lokalnej.** W `basic.vert` zamień `vec4(aPosition, 1.0)` na `vec4(aPosition + vec3(1.0, 0.0, 0.0), 1.0)`. Kostka przesunęła się, ale nie dokładnie w prawo ekranu. Dlaczego? W którym miejscu wyrażenia trzeba by dodać przesunięcie, żeby było przesunięciem w przestrzeni świata? Nie zmieniaj kodu C++.
3. **Literówka w nazwie uniformu.** W `NightMazeApp.cpp` zmień `MODEL_UNIFORM` na `"uModle"`, zbuduj i uruchom. Co widać w oknie, co w konsoli, co w panelu Shaders (`Program`, `Last load`)? Wyjaśnij, jaką wartość ma `uModel` w shaderze i gdzie lądują wierzchołki. Wycofaj zmianę.
4. **`setMat4` przed `use()`.** W `NightMazeApp::onRender` przenieś linię `m_shader.use();` pod trzy wywołania `setMat4`. Zbuduj i uruchom. Czy w konsoli jest błąd i po którym wywołaniu? Co widać w pierwszej klatce, a co w następnych? Wycofaj zmianę.
5. **Transpozycja.** W `Shader::setMat4` zamień `GL_FALSE` na `GL_TRUE`. Zbuduj i uruchom. Opisz obraz. Dla macierzy modelu samej kostki (sam obrót) transpozycja to obrót w przeciwną stronę: dlaczego? Która z trzech macierzy psuje się najbardziej i dlaczego? Wycofaj zmianę.
6. **Uniform `vec3`.** Dopisz do `basic.frag` uniform `uniform vec3 uTint;` i pomnóż przez niego kolor. W `NightMazeApp::onRender`, po `m_shader.use()`, wyślij wartość funkcją `setVec3` (sekcja 5.4), na przykład `glm::vec3(1.0F, 0.5F, 0.5F)`. Zbuduj i uruchom. Potem usuń wywołanie `setVec3` i zostaw uniform w shaderze: co widać i dlaczego? Wycofaj zmiany.
7. **Rodzina `glUniform` na kartce.** Zapisz deklarację i implementację funkcji `setFloat` w stylu `setInt`. Którą funkcję OpenGL zawoła? Którą zawołałaby funkcja `setVec4`, a którą funkcja wysyłająca tablicę pięciu wektorów `vec3`?

## 9. Pytania kontrolne

1. **`glGetUniformLocation` zwraca -1, choć nazwa jest poprawna. Co się stało?**
   Kompilator usunął uniform, bo nie wpływa na wynik shadera (jest nieużyty albo jego użycie zostało zoptymalizowane). Dla OpenGL taki uniform nie istnieje. Ustawianie położenia -1 jest ignorowane bez błędu.

2. **Co robi `setMat4`, linia po linii?**
   Pyta sterownik o położenie uniformu o podanej nazwie w programie obiektu (`glGetUniformLocation`), a potem kopiuje do niego 16 liczb macierzy (`glUniformMatrix4fv`): jedna macierz, bez transpozycji, wskaźnik z `glm::value_ptr`. Położenie jest wyszukiwane za każdym razem, bez pamięci podręcznej.

3. **Dlaczego przed `setMat4` musi stać `use()`?**
   `glUniformMatrix4fv` nie przyjmuje identyfikatora programu, tylko pisze do programu bieżącego, wybranego przez `glUseProgram`. Bez `use()` macierz trafiłaby do innego programu albo wywołanie skończyłoby się `GL_INVALID_OPERATION`. W OpenGL 4.1 jest też `glProgramUniform*` z programem jako argumentem, ale projekt używa postaci z wykładu.

4. **Co się stanie przy literówce w nazwie uniformu?**
   `glGetUniformLocation` zwróci -1, a `glUniformMatrix4fv` z położeniem -1 jest ignorowane bez błędu. Uniform w shaderze zostaje z zerami. Dla macierzy modelu oznacza to wszystkie wierzchołki w jednym punkcie i pusty ekran, bez żadnego komunikatu. To samo położenie -1 ma uniform usunięty przez kompilator jako nieużywany.

5. **Dlaczego `transpose` to `GL_FALSE`?**
   GLM przechowuje macierz kolumnami, tak samo jak oczekuje jej OpenGL i GLSL. Nie ma czego transponować.

6. **Dlaczego shader ma trzy osobne macierze, a nie jedną?**
   Dla nauki: każdą da się podmienić osobno i zobaczyć skutek, a wyrażenie `uProjection * uView * uModel * vec4(aPosition, 1.0)` pokazuje wprost drogę wierzchołka przez przestrzenie. Prawdziwy renderer często wysyła jeden gotowy iloczyn. Macierz modelu osobno przyda się też przy oświetleniu.

7. **Dlaczego macierze są wysyłane co klatkę, skoro kostka stoi w miejscu?**
   Macierz rzutowania zależy od proporcji okna, które mogą się zmienić. Po `reload()` nowy program ma uniformy wyzerowane, więc wartości wysłane raz by przepadły. A ruchoma kamera i tak zmienia macierz widoku w każdej klatce.

8. **Do czego służy `setInt` i dlaczego powstało razem z teksturami?**
   Ustawia uniform typu `int` przez `glUniform1i`. Tą samą funkcją ustawia się uniformy typu sampler: sampler przechowuje numer jednostki teksturującej, a nie identyfikator tekstury. GLSL 4.10 nie ma zapisu `layout(binding = N)`, więc numer trzeba wysłać z C++.

9. **Co oznaczają cyfra i litery w nazwie `glUniform3fv`?**
   `3` to trzy składowe, `f` to typ `float`, `v` to wartości podane wskaźnikiem. Drugi argument (u nas 1) to liczba wektorów, większa od 1 tylko dla uniformu będącego tablicą.

10. **Co się stanie, gdy sampler zostanie ustawiony przez `glUniform1f`?**
    OpenGL zgłosi `GL_INVALID_OPERATION` i wartości nie zmieni. Typ uniformu i funkcja muszą do siebie pasować, a dla samplerów jedyną dozwoloną funkcją jest `glUniform1i`.

## 10. Źródła

- LearnOpenGL, rozdział "Shaders" (<https://learnopengl.com/Getting-started/Shaders>): uniformy, `glGetUniformLocation`, ustawianie uniformu po `glUseProgram`.
- docs.gl, OpenGL 4: `glGetUniformLocation` (<https://docs.gl/gl4/glGetUniformLocation>), `glUniform` (<https://docs.gl/gl4/glUniform>, w tym `glUniformMatrix4fv`, `glUniform1i`, `glUniform3fv`, zasada, że sampler przyjmuje tylko `glUniform1i`, i zachowanie dla położenia -1), `glProgramUniform` (<https://docs.gl/gl4/glProgramUniform>).
- Khronos OpenGL Wiki: "Uniform (GLSL)" (<https://www.khronos.org/opengl/wiki/Uniform_(GLSL)>, o uniformach nieaktywnych).
- Dokumenty w tym repozytorium: [`shaders.md`](shaders.md) (potok, GLSL), [`shader-class.md`](shader-class.md) (klasa `Shader`), [`shader-hot-reload.md`](shader-hot-reload.md) (uniformy po przeładowaniu), [`textures.md`](textures.md) (samplery i jednostki teksturujące), [`../scene/transforms.md`](../scene/transforms.md) i [`../scene/camera.md`](../scene/camera.md) (skąd biorą się macierze), [`../core/gl-check.md`](../core/gl-check.md), [`../../libraries/glm.md`](../../libraries/glm.md) (zapis kolumnowy, `value_ptr`).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o shaderach i języku GLSL).
- "OpenGL. Księga eksperta" (rozdziały o potoku programowalnym i shaderach).
