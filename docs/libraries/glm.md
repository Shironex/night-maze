# GLM 1.0.3

Dokument biblioteki dla kamienia milowego M1. Opisuje konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i tę część API, której projekt
używa.

**Stan na dziś: GLM używają struktury `scene::Transform` i `scene::Camera` oraz klasa
`gfx::Shader`.** Struktury z [`src/scene/`](../../src/scene/) (opis w
[`../modules/scene/transforms-camera.md`](../modules/scene/transforms-camera.md)) wołają
`translate`, `rotate`, `scale`, `lookAt`, `perspective`, `radians`, `cross` i `normalize`.
`Shader::setMat4` w [`src/gfx/Shader.cpp`](../../src/gfx/Shader.cpp) woła `value_ptr`
(sekcja 3.9), żeby wysłać macierz do shadera. Wszystko spotyka się w
`game::NightMazeApp`, które co klatkę liczy trzy macierze i wysyła je do `basic.vert`, a
przy sterowaniu kamerą woła `length`, `normalize` i `mix` (sekcja 3.8). Panel Camera
(`src/debug/panels/CameraPanel.cpp`) woła `value_ptr`, żeby ImGui mogło edytować pozycję
kamery.

W dokumencie są dwa rodzaje bloków C++. Blok zaczynający się komentarzem
`// Przykład, nie kod projektu.` to **przykład użycia API**. Blok poprzedzony nazwą pliku to
kod skopiowany z repozytorium. Fragmenty CMake są prawdziwe i skopiowane z repozytorium.

## 1. Czym jest GLM

GLM (OpenGL Mathematics) to biblioteka matematyczna C++ dla grafiki: wektory, macierze i
funkcje, które na nich działają. Dwie cechy są najważniejsze.

**Naśladuje GLSL.** Typy i funkcje mają te same nazwy i to samo zachowanie co w języku
shaderów (GLSL): `vec3`, `mat4`, `normalize`, `dot`, `cross`, `mix`, `radians`. To, czego
nauczę się po stronie C++, działa tak samo w shaderze, i odwrotnie. Różnica w zapisie to
przestrzeń nazw: w C++ piszę `glm::vec3`, w GLSL samo `vec3`.

**Składa się z samych nagłówków (header-only).** Cały kod jest w plikach `.hpp` i `.inl`,
jako szablony i funkcje `inline`. Nie ma niczego do skompilowania osobno ani do linkowania:
kompilator wkleja potrzebny kod do tego pliku `.cpp`, który dołącza nagłówek. Skutki:

- "podpięcie" GLM to wyłącznie dodanie katalogu nagłówków do ścieżek kompilatora,
- po buildzie nie powstaje żadna biblioteka `libglm.a`,
- koszt ponosi czas kompilacji: każdy plik dołączający `<glm/glm.hpp>` kompiluje się dłużej.

### Za co GLM NIE odpowiada

- Nie woła OpenGL. Nie ma w niej ani jednej funkcji `gl*`. Macierz policzoną w GLM trzeba
  samemu wysłać do shadera (`glUniformMatrix4fv`, sekcja 3.9).
- Nie wie nic o oknie, kamerze ani scenie. `glm::lookAt` zwraca macierz, a nie obiekt kamery.
  Klasę kamery piszę sam.
- Nie wykonuje shaderów i nie jest kompilatorem GLSL. Podobieństwo dotyczy tylko nazw i
  zachowania funkcji.
- Nie liczy na karcie graficznej. Wszystko dzieje się na procesorze, w zwykłym kodzie C++.

## 2. Jak podpinamy GLM w CMake

Cały fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
# ---- GLM: vector and matrix math ------------------------------------------------------
# GLM is header-only. By default its CMake build also compiles a static library that we do
# not need, so switch that off: only the header-only interface target is used.
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# Treat the GLM headers as system headers so they cannot produce warnings in our code.
# glm::glm-header-only is an alias, properties must be set on the real target name.
get_target_property(glm_include_dirs glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(glm-header-only PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${glm_include_dirs}"
)
```

Mechanizm FetchContent (`FetchContent_Declare`, `FetchContent_MakeAvailable`, `GIT_SHALLOW`,
zapis `CACHE BOOL "" FORCE`, powód przypinania wersji) jest opisany w
[`glfw.md`](glfw.md), sekcja 2. Tutaj tylko to, co dla GLM jest inne.

### Jakie targety daje GLM

Repozytorium GLM ma własny `CMakeLists.txt`, więc `FetchContent_MakeAvailable(glm)` dołącza
go jak `add_subdirectory`. Plik `build/debug/_deps/glm-src/glm/CMakeLists.txt` definiuje dwa
targety:

| Target | Alias | Rodzaj | Co zawiera |
|---|---|---|---|
| `glm-header-only` | `glm::glm-header-only` | `INTERFACE` | tylko katalog nagłówków (`_deps/glm-src`). Niczego nie kompiluje |
| `glm` | `glm::glm` | zależy od `GLM_BUILD_LIBRARY` | przy `ON`: biblioteka statyczna z pliku `glm/detail/glm.cpp`. Przy `OFF`: pusty target `INTERFACE`, który linkuje `glm-header-only` |

Target `INTERFACE` to target bez plików do skompilowania. Niesie tylko ustawienia dla tych,
którzy go linkują: tutaj jedną ścieżkę nagłówków. Dla biblioteki z samych nagłówków to
wszystko, czego trzeba.

Wybrałem `glm::glm-header-only`, bo nazwa mówi dokładnie, co dostaję, i target znaczy to samo
niezależnie od opcji. `glm::glm` raz jest biblioteką statyczną, a raz aliasem na nagłówki.

### Trzy opcje

| Opcja | Co robi | Wartość domyślna w GLM 1.0.3 |
|---|---|---|
| `GLM_BUILD_LIBRARY` | kompiluje bibliotekę statyczną `glm` z `glm/detail/glm.cpp` | `ON` |
| `GLM_BUILD_TESTS` | buduje programy testowe GLM | `OFF` |
| `GLM_BUILD_INSTALL` | generuje reguły `install` | `ON` tylko gdy GLM jest projektem głównym |

Realnie coś zmienia tylko pierwsza linia. Bez `GLM_BUILD_LIBRARY OFF` każdy build
kompilowałby `glm.cpp` do biblioteki, której nikt nie linkuje: strata czasu i dodatkowy
plik w katalogu buildu. Dwie pozostałe opcje i tak miałyby u nas wartość `OFF` (GLM nie jest
projektem głównym). Ustawiam je jawnie z tego samego powodu co przy GLFW: żeby było widać
intencję (tylko nagłówki, bez testów, bez instalacji) i żeby wynik nie zależał od wartości
domyślnych przyszłej wersji.

Linie `set(...)` muszą stać **przed** `FetchContent_MakeAvailable(glm)`, bo GLM czyta te
opcje w chwili dołączenia.

### Podłączenie do `engine`

W głównym [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, scene/Transform.hpp,
# scene/Camera.hpp) expose GLM types, so every target that includes them needs the GLM
# include path too.
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
```

"Linkowanie" targetu `INTERFACE` niczego nie dopisuje do linkera. Oznacza tylko: przekaż
`engine` ścieżkę nagłówków GLM.

Dlaczego `PUBLIC`: nagłówki warstw `scene` i `gfx` pokazują typy GLM w swoim API (pozycja
kamery jako `glm::vec3`, macierz widoku jako `glm::mat4`, parametr `const glm::mat4&` w
`Shader::setMat4`). Każdy plik, który dołączy taki nagłówek,
także w `night_maze`, musi znaleźć `<glm/glm.hpp>`. Przy `PRIVATE` ścieżkę znałby tylko
`engine` i kod gry by się nie kompilował.

### Nagłówki GLM jako systemowe

Powód jest ten sam co przy GLFW ([`glfw.md`](glfw.md), sekcja 2): nasze targety kompilują się
z `-Wall -Wextra -Wpedantic` (na MSVC `/W4`), a nie chcę ostrzeżeń z cudzych nagłówków. Dla
GLM jest to ważniejsze niż dla GLFW, bo cały kod biblioteki jest w nagłówkach i kompiluje
się **wewnątrz naszych plików**, z naszymi flagami.

Sposób też jest ten sam: skopiować listę `INTERFACE_INCLUDE_DIRECTORIES` do
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`. Słowo `SYSTEM` w `FetchContent_Declare` wymaga CMake
3.25, a nasze minimum to 3.24.

Jedna różnica: właściwości ustawiam na `glm-header-only`, a linkuję `glm::glm-header-only`.
To ten sam target. Nazwa z `::` jest aliasem (`add_library(glm::glm-header-only ALIAS
glm-header-only)`), a CMake nie pozwala zmieniać właściwości przez alias.

Efekt w `build/debug/compile_commands.json` (polecenie dla `src/core/Time.cpp`, ścieżki
skrócone):

```text
-I.../night-maze/src -isystem .../external/glad/include -isystem .../build/debug/_deps/glfw-src/include -isystem .../build/debug/_deps/glm-src
```

Katalogiem nagłówków jest korzeń repozytorium GLM (`_deps/glm-src`), a nagłówki leżą w jego
podkatalogu `glm/`. Stąd zapis `#include <glm/glm.hpp>`.

Sprawdzenie na Macu (clang, Debug i Release): pliki dołączające GLM
(`src/scene/Transform.cpp`, `src/scene/Camera.cpp`, `src/gfx/Shader.cpp` i przez nagłówki
`src/game/NightMazeApp.cpp`) kompilują się bez żadnego ostrzeżenia, a clang-tidy z regułami
projektu niczego w nich nie zgłasza. Na Windowsie
(MSVC, `/W4`) nie było to jeszcze sprawdzane.

### Co GLM robi w swoim `CMakeLists.txt` i dlaczego nas to nie dotyczy

Główny `CMakeLists.txt` GLM woła `add_compile_options(...)`: na clang dodaje
`-Wno-c++98-compat` i podobne (stąd linia `GLM: Disable -Wc++98-compat warnings` w logu
konfiguracji), a na MSVC `/Za` i `/fp:precise`. `add_compile_options` działa tylko na targety
z tego samego katalogu i jego podkatalogów, czyli na targety GLM. Przy `GLM_BUILD_LIBRARY OFF`
nie ma tam niczego do skompilowania, więc te flagi nie trafiają nigdzie. W poleceniach
kompilacji naszych plików ich nie ma.

## 3. Najważniejsze API

Bloki z komentarzem `// Przykład, nie kod projektu.` to przykłady. Pozostałe bloki C++ są
skopiowane z `src/scene/` i mają nad sobą nazwę pliku.

### 3.1. Które nagłówki dołączać

| Nagłówek | Co daje |
|---|---|
| `<glm/glm.hpp>` | rdzeń zgodny z GLSL: `vec2`, `vec3`, `vec4`, `mat3`, `mat4`, operatory, `radians`, `normalize`, `cross`, `dot`, `mix`, `inverse`, `transpose` |
| `<glm/gtc/matrix_transform.hpp>` | budowanie macierzy: `translate`, `rotate`, `scale`, `lookAt`, `perspective`, `ortho` |
| `<glm/gtc/type_ptr.hpp>` | `value_ptr`: wskaźnik na surowe dane wektora albo macierzy, do przekazania OpenGL |

`gtc` to rozszerzenia stabilne (rzeczy spoza specyfikacji GLSL, ale z ustalonym API). Katalog
`gtx` zawiera rozszerzenia eksperymentalne. Nie używam ich: wymagają makra
`GLM_ENABLE_EXPERIMENTAL` i mogą się zmieniać między wersjami.

W nagłówku `.hpp`, który tylko deklaruje pole albo parametr typu `glm::vec3` lub `glm::mat4`,
wystarcza `<glm/glm.hpp>`. Dwa pozostałe nagłówki dołącza się w pliku `.cpp`, który faktycznie
buduje macierz albo wysyła ją do OpenGL.

Tak jest w `src/scene/`: `Transform.hpp` i `Camera.hpp` dołączają samo `<glm/glm.hpp>`, a
`Transform.cpp` i `Camera.cpp` dodatkowo `<glm/gtc/matrix_transform.hpp>`. W `src/gfx/`
`Shader.hpp` dołącza `<glm/glm.hpp>` (parametr typu `glm::mat4`), a `Shader.cpp`
`<glm/gtc/type_ptr.hpp>` (funkcja `value_ptr`).

### 3.2. Wektory: `vec2`, `vec3`, `vec4`

```cpp
// Przykład, nie kod projektu.
glm::vec3 position(1.0F, 2.0F, 3.0F);
glm::vec3 allOnes(1.0F);            // (1, 1, 1): jeden argument wypełnia wszystkie składowe
glm::vec4 point(position, 1.0F);    // (1, 2, 3, 1): vec3 rozszerzony o w
float height = position.y;
glm::vec3 sum = position + allOnes; // działania składowa po składowej
glm::vec3 twice = position * 2.0F;
```

- `vec2`, `vec3`, `vec4` to dwie, trzy i cztery liczby `float`. `sizeof(glm::vec3)` to 12
  bajtów, bez żadnych dodatków, więc tablicę `vec3` można wprost wysłać do bufora wierzchołków.
- Te same składowe mają trzy zestawy nazw: `x y z w` (pozycja), `r g b a` (kolor),
  `s t p q` (współrzędne tekstury). `v.x` i `v.r` to ta sama liczba.
- Do czego: `vec2` to współrzędne tekstury, `vec3` to pozycja, kierunek, normalna i kolor
  RGB, `vec4` to punkt we współrzędnych jednorodnych (homogeneous coordinates) i kolor RGBA.
- Czwarta składowa `w` rozróżnia punkt od kierunku: `w = 1` to punkt (przesunięcie na niego
  działa), `w = 0` to kierunek (przesunięcie go nie zmienia).
- Operator `*` między dwoma wektorami mnoży **składowa po składowej**. To nie jest iloczyn
  skalarny ani wektorowy (do nich służą `dot` i `cross`, sekcja 3.8).

### 3.3. Macierze: `mat3`, `mat4` i zapis kolumnowy

`mat4` to macierz 4 x 4 liczb `float` (64 bajty), `mat3` to 3 x 3. Macierz `mat4` opisuje
dowolne przekształcenie afiniczne (przesunięcie, obrót, skala) oraz rzutowanie. `mat3`
przyda się później jako macierz normalnych (oświetlenie, M4).

**Macierz jednostkowa (identity matrix)** to przekształcenie "nic nie rób". Zawsze piszę ją
jawnie:

```cpp
// Przykład, nie kod projektu.
glm::mat4 identity(1.0F);   // jedynki na przekątnej, zera poza nią
```

Konstruktor z jedną liczbą wpisuje ją na przekątną. `glm::mat4(1.0F)` to macierz
jednostkowa, `glm::mat4(0.0F)` to same zera. Dlaczego nie samo `glm::mat4 m;`, wyjaśnia
pułapka 2. W projekcie od takiej macierzy zaczyna `Transform::matrix()` (sekcja 3.5).

**Układ kolumnowy (column-major).** GLM przechowuje macierz tak jak GLSL i OpenGL: jako
cztery kolumny, jedna po drugiej. Pierwszy indeks wybiera **kolumnę**, drugi **wiersz**:

```cpp
// Przykład, nie kod projektu.
glm::mat4 m(1.0F);
glm::vec4 thirdColumn = m[2];   // cała kolumna o indeksie 2
m[3][0] = 5.0F;                 // kolumna 3, wiersz 0
```

`m[col][row]` jest odwrotnie niż w zapisie matematycznym, gdzie pierwszy jest wiersz. Dla
macierzy przesunięcia o (tx, ty, tz):

```text
zapis matematyczny          w GLM
| 1  0  0  tx |             m[0] = (1,  0,  0,  0)    pierwsza kolumna
| 0  1  0  ty |             m[1] = (0,  1,  0,  0)
| 0  0  1  tz |             m[2] = (0,  0,  1,  0)
| 0  0  0  1  |             m[3] = (tx, ty, tz, 1)    czwarta kolumna: przesunięcie
```

Przesunięcie leży w ostatniej **kolumnie**, czyli w `m[3]`, a `tx` to `m[3][0]`. W pamięci
16 liczb leży kolumnami: najpierw cała `m[0]`, potem `m[1]`, `m[2]`, `m[3]`. Przesunięcie
zajmuje więc pozycje 12, 13 i 14. Dokładnie takiego układu oczekuje `glUniformMatrix4fv`
(sekcja 3.9).

### 3.4. Mnożenie macierzy: czytamy od prawej do lewej

GLM, tak jak GLSL, traktuje wektor jako kolumnę i mnoży go przez macierz z **lewej** strony:

```cpp
// Przykład, nie kod projektu.
glm::vec4 transformed = matrix * point;
```

Przy kilku macierzach:

```cpp
// Przykład, nie kod projektu.
glm::vec4 clipPosition = projection * view * model * glm::vec4(localPosition, 1.0F);
```

najbliżej wektora stoi `model`, więc to ona działa pierwsza. Wynik trafia do `view`, a jej
wynik do `projection`. Wyrażenie czyta się od prawej do lewej: najpierw model (z układu
lokalnego do świata), potem view (ze świata do układu kamery), na końcu projection (do
przestrzeni przycięcia, clip space). W shaderze wierzchołków zapis jest identyczny.

Mnożenie macierzy **nie jest przemienne**: `A * B` to co innego niż `B * A`. "Przesuń, potem
obróć" i "obróć, potem przesuń" dają różne wyniki. Jest natomiast łączne, więc
`projection * view` można policzyć raz na klatkę i dopiero potem mnożyć przez `model` każdego
obiektu.

### 3.5. `glm::translate`, `glm::rotate`, `glm::scale`

```cpp
// Przykład, nie kod projektu.
glm::mat4 model(1.0F);
model = glm::translate(model, glm::vec3(2.0F, 0.0F, 0.0F));
model = glm::rotate(model, glm::radians(90.0F), glm::vec3(0.0F, 1.0F, 0.0F));
model = glm::scale(model, glm::vec3(0.5F));
```

| Funkcja | Argumenty | Zwraca |
|---|---|---|
| `glm::translate(m, v)` | macierz, wektor przesunięcia | `m * T` |
| `glm::rotate(m, angle, axis)` | macierz, kąt **w radianach**, oś obrotu (funkcja sama ją normalizuje) | `m * R` |
| `glm::scale(m, v)` | macierz, współczynniki skali dla x, y, z | `m * S` |

Każda z tych funkcji **nie modyfikuje** argumentu `m`, tylko zwraca nową macierz. Dlatego
wynik trzeba przypisać (`model = ...`). Samo `glm::translate(model, v);` nie robi nic.

**Kolejność zastosowania.** Każda funkcja mnoży nowe przekształcenie z **prawej** strony.
Po trzech liniach przykładu `model` to `T * R * S`. Z sekcji 3.4 wiadomo, że pierwsza działa
macierz stojąca najbliżej wektora, czyli `S`. Wierzchołek jest więc najpierw skalowany, potem
obracany, a na końcu przesuwany: **odwrotnie niż kolejność linii w kodzie**. Ostatnie
wywołanie w kodzie jest pierwszym przekształceniem wierzchołka.

Kolejność "skala, obrót, przesunięcie" (liczona od strony wierzchołka) jest standardem dla
macierzy modelu: obiekt skaluje się i obraca wokół własnego środka, a dopiero potem trafia na
miejsce w świecie. Gdyby przesunięcie zadziałało przed obrotem, obiekt krążyłby wokół
początku układu świata.

**W projekcie.** Dokładnie ten wzorzec, z trzema obrotami zamiast jednego, to
`Transform::matrix()` w [`src/scene/Transform.cpp`](../../src/scene/Transform.cpp):

```cpp
glm::mat4 Transform::matrix() const {
    // Start from the identity matrix ("change nothing"). Each glm function below
    // multiplies its matrix on the right side, so the last call is the first one applied
    // to a vertex: the lines read top to bottom, the vertex is transformed bottom to top.
    glm::mat4 model(1.0F);
    model = glm::translate(model, position);
    // GLM takes angles in radians.
    model = glm::rotate(model, glm::radians(rotationDegrees.y), AXIS_Y);
    model = glm::rotate(model, glm::radians(rotationDegrees.x), AXIS_X);
    model = glm::rotate(model, glm::radians(rotationDegrees.z), AXIS_Z);
    model = glm::scale(model, scale);
    return model;
}
```

Powstaje `T * Ry * Rx * Rz * S`. Osie obrotu to nazwane stałe z tego samego pliku
(`constexpr glm::vec3 AXIS_Y{0.0F, 1.0F, 0.0F};` i dwie podobne). Omówienie linia po linii:
[`../modules/scene/transforms-camera.md`](../modules/scene/transforms-camera.md), sekcja 5.3.

### 3.6. `glm::lookAt`: macierz widoku

```cpp
// Przykład, nie kod projektu.
glm::mat4 view = glm::lookAt(eye, center, up);
```

| Argument | Znaczenie |
|---|---|
| `eye` | pozycja kamery w świecie |
| `center` | **punkt**, na który kamera patrzy. To nie jest kierunek |
| `up` | wektor wskazujący "górę" świata, zwykle `(0, 1, 0)` |

Wynik to macierz widoku (view matrix): przekształca współrzędne świata na współrzędne
kamery. Kamera FPS zna zwykle pozycję i kierunek patrzenia, więc jako `center` podaje się
`eye + direction`.

GLM domyślnie używa układu prawoskrętnego (right-handed), tak jak OpenGL: `lookAt` to w tej
konfiguracji `lookAtRH`. W przestrzeni kamery kamera stoi w początku układu i patrzy wzdłuż
**ujemnej** osi Z, oś X wskazuje w prawo, a oś Y w górę.

**W projekcie.** `Camera::viewMatrix` w [`src/scene/Camera.cpp`](../../src/scene/Camera.cpp):

```cpp
glm::mat4 Camera::viewMatrix(const glm::vec3& eye) const {
    // lookAt wants a point to look at, not a direction: one step forward from the eye.
    return glm::lookAt(eye, eye + forward(), WORLD_UP);
}
```

`forward()` to kierunek patrzenia policzony z kątów yaw i pitch, a `WORLD_UP` to stała
`(0, 1, 0)`. Co dokładnie buduje `lookAt` (trzy wektory bazy kamery) i dlaczego pozycja oka
jest parametrem: [`../modules/scene/transforms-camera.md`](../modules/scene/transforms-camera.md),
sekcje 2.7 i 5.7.

### 3.7. `glm::perspective` i `glm::radians`

```cpp
// Przykład, nie kod projektu.
glm::mat4 projection = glm::perspective(glm::radians(60.0F), aspect, 0.1F, 100.0F);
```

| Argument | Znaczenie |
|---|---|
| `fovy` | **pionowy** kąt widzenia (field of view), **w radianach** |
| `aspect` | proporcje obrazu: szerokość podzielona przez wysokość (jako `float`) |
| `zNear` | odległość bliskiej płaszczyzny przycinania, musi być większa od zera |
| `zFar` | odległość dalekiej płaszczyzny przycinania |

Co jest ustalone w domyślnej konfiguracji GLM (bez żadnych makr `GLM_FORCE_*`):

- układ **prawoskrętny**: widoczne są punkty o ujemnym Z w przestrzeni kamery, w odległości
  od `zNear` do `zFar`,
- głębia po rzutowaniu mieści się w zakresie **od -1 do 1**. Tego oczekuje OpenGL, więc
  niczego nie trzeba przestawiać. W kodzie GLM domyślne `perspective` to `perspectiveRH_NO`:
  RH to right-handed, NO to "negative one to one".

`zNear` i `zFar` podaje się jako **dodatnie odległości** od kamery, mimo że kamera patrzy
wzdłuż ujemnej osi Z.

`aspect` liczy się z rozmiaru framebuffera, nie okna (te same piksele co `glViewport`,
[`glfw.md`](glfw.md), sekcja 3.8), i trzeba go przeliczać po zmianie rozmiaru okna.

`glm::radians(degrees)` zamienia stopnie na radiany (mnoży przez pi / 180).
`glm::degrees` robi odwrotnie. Wszystkie funkcje GLM przyjmują kąty w radianach.

**W projekcie.** `Camera::projectionMatrix` w
[`src/scene/Camera.cpp`](../../src/scene/Camera.cpp):

```cpp
glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    // GLM takes the field of view in radians.
    return glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
}
```

Pola `fovDegrees`, `nearPlane` i `farPlane` mają wartości domyślne 60, 0,1 i 100. Proporcje
podaje wołający. Zasada projektu: kąty w polach są w stopniach (jednostka jest w nazwie
pola), a `glm::radians` stoi w miejscu użycia. Tak samo zaczyna się `Camera::forward()`:

```cpp
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
```

Teoria rzutowania (bryła widzenia, dzielenie perspektywiczne, nieliniowa głębia):
[`../modules/scene/transforms-camera.md`](../modules/scene/transforms-camera.md), sekcja 2.9.

### 3.8. `normalize`, `cross`, `dot`, `mix`

| Funkcja | Wynik | Do czego |
|---|---|---|
| `glm::normalize(v)` | wektor o tym samym kierunku i długości 1 | kierunki: kamera, normalne, promienie światła |
| `glm::dot(a, b)` | liczba: iloczyn skalarny (dot product). Dla wektorów jednostkowych to cosinus kąta między nimi | oświetlenie (kąt między normalną a światłem), sprawdzanie "czy patrzę w stronę" |
| `glm::cross(a, b)` | wektor prostopadły do obu: iloczyn wektorowy (cross product) | wektor "w prawo" kamery z kierunku patrzenia i góry |
| `glm::mix(x, y, a)` | `x * (1 - a) + y * a`: interpolacja liniowa | płynne przejście między dwiema wartościami |
| `glm::length(v)` | długość wektora | odległości |

`Camera::right` w [`src/scene/Camera.cpp`](../../src/scene/Camera.cpp):

```cpp
glm::vec3 Camera::right() const {
    // The cross product is perpendicular to both vectors. Its length is cos(pitch), not
    // 1, so it has to be normalized. The pitch limit keeps that length above zero.
    return glm::normalize(glm::cross(forward(), WORLD_UP));
}
```

- `cross` zależy od kolejności: `cross(a, b)` to `-cross(b, a)`. W układzie prawoskrętnym
  `cross(x, y)` daje `z`.
- `mix(x, y, 0.0F)` zwraca `x`, `mix(x, y, 1.0F)` zwraca `y`, a `0.5F` punkt w połowie.
  Działa dla liczb i dla wektorów.
- W GLSL te same funkcje nazywają się tak samo i liczą to samo.

`normalize` i `length` w ruchu kamery, `NightMazeApp::onUpdate` w
[`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp):

```cpp
// Two keys at once give a vector longer than 1 (about 1.41 for W and D), which would
// make diagonal movement faster. Normalizing brings the length back to 1. With no key
// held the vector is zero and must be left alone: normalizing it divides by zero.
if (glm::length(direction) > 0.0F) {
    direction = glm::normalize(direction);
}
```

`direction` to suma kierunków wciśniętych klawiszy. `glm::length` zwraca jej długość
(pierwiastek z sumy kwadratów składowych), a `glm::normalize` dzieli wektor przez tę długość.
Warunek jest konieczny: dla wektora zerowego `normalize` dzieli zero przez zero (pułapka 9).

`mix` przy rysowaniu, `NightMazeApp::onRender` w tym samym pliku:

```cpp
// The simulation moves the camera in fixed steps, and this frame is drawn at some
// moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
// between the position before the last step and the position after it keeps the
// movement smooth at any frame rate. m_camera.position itself is not changed.
const glm::vec3 eye =
    glm::mix(m_previousCameraPosition, m_camera.position, static_cast<float>(alpha));
```

Tak liczy się pozycję do narysowania między dwoma krokami symulacji. Trzeci argument musi
mieć typ składowych wektora (`float`), a `alpha` przychodzi jako `double`, stąd
`static_cast<float>`. Pełny opis obu fragmentów:
[`../modules/scene/transforms-camera.md`](../modules/scene/transforms-camera.md), sekcje 2.12
i 5.11 (o `alpha`: [`../modules/core/main-loop.md`](../modules/core/main-loop.md), sekcja 2.4).

### 3.9. `glm::value_ptr` i wysyłanie macierzy do shadera

`Shader::setMat4` w [`src/gfx/Shader.cpp`](../../src/gfx/Shader.cpp):

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

Nagłówek `<glm/gtc/type_ptr.hpp>` jest dołączony na górze tego pliku. Funkcję omawia linia
po linii [`../modules/gfx/shaders.md`](../modules/gfx/shaders.md), sekcja 5.12. Woła ją
`NightMazeApp::onRender`, trzy razy na klatkę:

```cpp
m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());
m_shader.setMat4(VIEW_UNIFORM, m_camera.viewMatrix(eye));
m_shader.setMat4(PROJECTION_UNIFORM, m_camera.projectionMatrix(aspectRatio));
```

OpenGL to API w języku C i nie zna typu `glm::mat4`. Przyjmuje wskaźnik `const GLfloat*` na
16 liczb. `glm::value_ptr(matrix)` zwraca właśnie taki wskaźnik: adres pierwszej składowej,
za którą leżą pozostałe, kolumna po kolumnie.

Argumenty `glUniformMatrix4fv`:

| Argument | Wartość | Znaczenie |
|---|---|---|
| `location` | wynik `glGetUniformLocation` | który uniform ustawić. -1 (nie ma takiego uniformu) jest ignorowane bez błędu |
| `count` | `1` | ile macierzy (więcej niż 1 dla tablicy uniformów) |
| `transpose` | `GL_FALSE` | czy OpenGL ma transponować macierz. GLM przechowuje ją już w układzie kolumnowym, czyli tak, jak chce OpenGL |
| `value` | `glm::value_ptr(matrix)` | wskaźnik na dane |

Wskaźnik jest ważny tak długo, jak żyje obiekt macierzy. Nie wolno brać `value_ptr` od
wartości tymczasowej i używać go w następnej instrukcji. W `setMat4` macierz jest parametrem
(referencją), więc żyje przez całe wywołanie, a OpenGL kopiuje 16 liczb, zanim
`glUniformMatrix4fv` wróci.

Dla wektorów działa to tak samo: `glUniform3fv(location, 1, glm::value_ptr(color))`.

## 4. Pułapki

1. **Stopnie zamiast radianów.** `glm::rotate(m, 90.0F, axis)` obraca o 90 radianów, czyli o
   około 5157 stopni. `glm::perspective(60.0F, ...)` daje bezsensowny kąt widzenia. Zawsze
   `glm::radians(...)`. Kompilator tego nie wykryje, bo oba zapisy to `float`.
2. **`glm::mat4 m;` nie jest macierzą jednostkową.** W GLM 1.0.3 konstruktor domyślny jest
   zadeklarowany jako `= default` i nie ustawia niczego: `glm::mat4 m;` jako zmienna lokalna
   ma wartości nieokreślone, a `glm::mat4 m{};` same zera. Starsze wersje GLM (przed 0.9.9)
   dawały tu macierz jednostkową, więc stare poradniki piszą `glm::mat4 model;` i u nich to
   działa. Zapis `glm::mat4(1.0F)` jest poprawny w każdej wersji. To samo dotyczy wektorów:
   `glm::vec3 v;` nie jest wyzerowany, piszę `glm::vec3 v(0.0F);`.
3. **Kolejność mnożenia.** `model * view * projection` zamiast `projection * view * model`
   kompiluje się bez ostrzeżeń i daje pusty ekran. To samo dotyczy mnożenia w shaderze.
4. **Kolejność `translate` / `rotate` / `scale` w kodzie.** Ostatnie wywołanie działa na
   wierzchołek jako pierwsze (sekcja 3.5). Objaw pomyłki: obiekt krąży wokół początku układu
   zamiast obracać się w miejscu.
5. **Zapomniane przypisanie.** `glm::translate(model, v);` bez `model = ...` nie zmienia
   niczego, bo funkcja zwraca nową macierz.
6. **`m[col][row]`.** Przesunięcie to `m[3]`, nie `m[0][3]`, `m[1][3]`, `m[2][3]`.
7. **`GL_TRUE` w `glUniformMatrix4fv`.** Macierz z GLM wysyła się z `transpose = GL_FALSE`.
   `GL_TRUE` transponuje ją i psuje przekształcenie.
8. **`center` w `lookAt` to punkt.** Podanie kierunku patrzenia zamiast `eye + direction`
   sprawia, że kamera stale patrzy w okolice początku układu.
9. **`normalize` wektora zerowego.** Dzielenie przez długość 0 daje `NaN` we wszystkich
   składowych, a `NaN` rozchodzi się po macierzach i obraz znika. Typowe miejsce: sumowanie
   kierunków ruchu, gdy żaden klawisz nie jest wciśnięty. `NightMazeApp::onUpdate` normalizuje
   sumę tylko wtedy, gdy `glm::length(direction) > 0.0F` (sekcja 3.8).
10. **`aspect` z dzielenia całkowitego albo z zerową wysokością.** `width / height` na
    typach `int` daje 1 zamiast 1.777. Dzielić trzeba liczby `float`. Przy framebufferze o
    wysokości 0 (zminimalizowane okno) wychodzi dzielenie przez zero, więc ten przypadek
    trzeba obsłużyć przed wywołaniem `perspective`.
11. **`zNear` równe 0 albo bardzo małe.** Przy 0 macierz rzutowania jest błędna. Przy bardzo
    małej wartości (albo ogromnym `zFar`) bufor głębi traci precyzję i odległe ściany
    migoczą (z-fighting).
12. **`*` między wektorami.** `a * b` to mnożenie składowa po składowej, nie `dot` i nie
    `cross`.
13. **Makra `GLM_FORCE_*`.** GLM konfiguruje się makrami definiowanymi przed dołączeniem
    nagłówków. Nie definiujemy żadnego:

    | Makro | Co robi | Dlaczego go nie ma |
    |---|---|---|
    | `GLM_FORCE_RADIANS` | w starych wersjach przełączało funkcje na radiany | usunięte z GLM, radiany są jedynym trybem. Stare poradniki nadal je definiują, dziś nie ma żadnego efektu |
    | `GLM_FORCE_DEPTH_ZERO_TO_ONE` | głębia od 0 do 1 zamiast od -1 do 1 | to konwencja Vulkana i Direct3D. W OpenGL 4.1 zakres to zawsze od -1 do 1 |
    | `GLM_FORCE_LEFT_HANDED` | układ lewoskrętny w `lookAt` i `perspective` | OpenGL i wykład używają układu prawoskrętnego |
    | `GLM_FORCE_CTOR_INIT` | konstruktory domyślne zerują wektory i dają macierz jednostkową | wolę jawny zapis `glm::mat4(1.0F)`, który nie zależy od makra |
    | `GLM_ENABLE_EXPERIMENTAL` | odblokowuje rozszerzenia `gtx` | nie używamy `gtx` |
    | `GLM_FORCE_SWIZZLE` | zapis `v.xyz`, `v.zyx` jak w GLSL | wydłuża kompilację, nie jest potrzebne |
    | `GLM_FORCE_INTRINSICS` | instrukcje SIMD | optymalizacja, której projekt nie potrzebuje |

    Jeżeli kiedyś któreś będzie potrzebne, trzeba je zdefiniować w CMake przez
    `target_compile_definitions(engine PUBLIC ...)`, tak jak `GLFW_INCLUDE_NONE`, a nie przez
    `#define` w jednym pliku. Makro zmienia zachowanie funkcji `inline`, więc różna wartość w
    różnych plikach `.cpp` oznacza dwie różne definicje tej samej funkcji w jednym programie.
14. **MSVC.** Nic z tej listy nie było jeszcze sprawdzane na Windowsie.
    - Plikami, które MSVC skompiluje razem z GLM, są `src/scene/Transform.cpp`,
      `src/scene/Camera.cpp`, `src/gfx/Shader.cpp` i każdy plik dołączający ich nagłówki.
      Pliki `scene` oprócz samych nagłówków używają stałych
      `constexpr glm::vec3` (`AXIS_X`, `Camera::WORLD_UP`), czyli konstruktorów GLM
      wykonywanych w czasie kompilacji.
    - Nagłówki GLM używają anonimowych struktur (stąd zamienne nazwy `x` i `r`), przed
      czym MSVC ostrzega pod `/W4` (C4201). GLM samo wyłącza to ostrzeżenie wokół swoich
      definicji (`#pragma warning(disable: 4201)` w `glm/detail/type_vec3.hpp` i
      `type_vec4.hpp`), a dodatkowo całe nagłówki są u nas systemowe.
    - `CMakeLists.txt` GLM dodaje na MSVC flagę `/Za`. Dotyczy ona tylko targetów GLM, a
      przy `GLM_BUILD_LIBRARY OFF` nie ma tam niczego do skompilowania (sekcja 2).
    - To, czy pod `/W4` nie pojawiają się ostrzeżenia z nagłówków GLM i czy katalog GLM
      trafia do kompilatora jako zewnętrzny (`/external:I`), jest punktem listy kontrolnej w
      [`../guides/build-windows.md`](../guides/build-windows.md), sekcja 11.

## 5. Pytania kontrolne

1. **Co znaczy, że GLM jest biblioteką header-only, i jak to widać w naszym CMake?**
   Cały kod jest w nagłówkach, nie ma niczego do skompilowania ani linkowania. Linkujemy
   target `INTERFACE` `glm::glm-header-only`, który niesie tylko ścieżkę nagłówków, a
   `GLM_BUILD_LIBRARY OFF` wyłącza zbędną bibliotekę statyczną.

2. **Dlaczego `engine` linkuje GLM jako `PUBLIC`?**
   Bo nagłówki `engine` (`scene/Transform.hpp`, `scene/Camera.hpp`) pokazują typy GLM w
   swoim API. Każdy target, który je dołącza (`night_maze`), musi znać ścieżkę do
   `<glm/glm.hpp>`.

3. **Po co `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` i dlaczego ustawiamy ją na
   `glm-header-only`, a nie na `glm::glm-header-only`?**
   Oznacza nagłówki GLM jako systemowe (`-isystem`), żeby nie generowały ostrzeżeń przy
   naszych flagach. Nazwa z `::` to alias, a właściwości można ustawiać tylko na prawdziwym
   targecie.

4. **Co oznacza `m[3][1]` w `glm::mat4`?**
   Kolumnę o indeksie 3, wiersz o indeksie 1. W macierzy przekształcenia to składowa y
   przesunięcia.

5. **W jakiej kolejności działają macierze w `projection * view * model * v` i dlaczego?**
   Od prawej: model, view, projection. Wektor jest kolumną mnożoną z lewej strony, więc
   pierwsza działa macierz stojąca najbliżej niego.

6. **Kod woła kolejno `translate`, `rotate`, `scale` na tej samej macierzy. W jakiej
   kolejności przekształcany jest wierzchołek?**
   Skala, obrót, przesunięcie. Każda funkcja mnoży z prawej strony (`m * T`, potem `* R`,
   potem `* S`), więc ostatnio dodane przekształcenie stoi najbliżej wektora.

7. **Jakie argumenty przyjmuje `glm::perspective` i jaki zakres głębi daje?**
   Pionowy kąt widzenia w radianach, proporcje (szerokość przez wysokość), odległość bliskiej
   i dalekiej płaszczyzny. Domyślnie układ prawoskrętny i głębia od -1 do 1, czyli konwencja
   OpenGL.

8. **Dlaczego piszemy `glm::mat4(1.0F)`, a nie `glm::mat4 m;`?**
   Konstruktor domyślny w aktualnym GLM niczego nie inicjalizuje. Zachowanie zmieniło się
   między wersjami, a zapis z `1.0F` zawsze daje macierz jednostkową.

9. **Co zwraca `glm::value_ptr` i dlaczego w `glUniformMatrix4fv` podajemy `GL_FALSE`?**
   Wskaźnik na 16 liczb `float` ułożonych kolumnami. `GL_FALSE` znaczy "nie transponuj":
   układ kolumnowy GLM jest tym, czego oczekuje OpenGL.

10. **Dlaczego nie definiujemy `GLM_FORCE_DEPTH_ZERO_TO_ONE` ani `GLM_FORCE_LEFT_HANDED`?**
    To konwencje innych API (Vulkan, Direct3D). OpenGL 4.1 używa układu prawoskrętnego i
    głębi od -1 do 1, czyli wartości domyślnych GLM.

## 6. Oficjalna dokumentacja

- Repozytorium GLM: <https://github.com/g-truc/glm>
- Podręcznik GLM (manual): <https://github.com/g-truc/glm/blob/1.0.3/manual.md>
- Wydanie 1.0.3: <https://github.com/g-truc/glm/releases/tag/1.0.3>
- Specyfikacja GLSL 4.10 (typy i funkcje, które GLM naśladuje): <https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.10.pdf>
- LearnOpenGL, rozdziały "Transformations", "Coordinate Systems" i "Camera":
  <https://learnopengl.com/Getting-started/Transformations>
- Dokumentacja CMake (FetchContent, biblioteki `INTERFACE`, właściwości targetów): <https://cmake.org/cmake/help/latest/>
- Kopia dokładnie dla naszej wersji leży po pierwszej konfiguracji w
  `build/debug/_deps/glm-src/`: `manual.md` (podręcznik, w tym rozdział 2 o makrach
  `GLM_FORCE_*`), `readme.md` (lista zmian między wersjami) oraz same nagłówki, na przykład
  `glm/ext/matrix_transform.inl` (kod `translate`, `rotate`, `scale`, `lookAt`) i
  `glm/ext/matrix_clip_space.inl` (kod `perspective`).
