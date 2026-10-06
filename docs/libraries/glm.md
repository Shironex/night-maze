# GLM 1.0.3

Dokument biblioteki dla kamienia milowego M1. Opisuje konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i tę część API, której projekt
używa.

**Stan na dziś: GLM używają struktury `scene::Transform` i `scene::Camera` oraz klasa
`gfx::Shader`.** Struktury z [`src/scene/`](../../src/scene/) (opis w
[`../modules/scene/transforms.md`](../modules/scene/transforms.md) i
[`../modules/scene/camera.md`](../modules/scene/camera.md)) wołają
`translate`, `rotate`, `scale`, `lookAt`, `perspective`, `radians`, `cross` i `normalize`,
a od czwartej części M7 (plik `src/scene/LightSpace.cpp`) także `ortho`, `min` i `max`.
`Shader::setMat4` w [`src/gfx/Shader.cpp`](../../src/gfx/Shader.cpp) woła `value_ptr`
(sekcja 3.9), żeby wysłać macierz do shadera, a `Shader::setVec3` tak samo wysyła wektor.
Wszystko spotyka się w `game::NightMazeApp`, które co klatkę liczy macierz widoku i macierz
rzutowania, wysyła je do programów shaderów (macierze kamery dostaje siedem programów sceny, a od
M8, części 1 w tym `reflect`; w jednej klatce pracuje ich najwyżej pięć. Od czwartej części
M7 ósmy program, `shadow_depth`, dostaje macierze widoku i rzutowania światła księżyca, a od
piątej także światła latarki. Wszystkich programów jest dziś czternaście) i woła `mix` przy liczeniu pozycji oka (sekcja 3.8). Kategoria Player (`src/debug/categories/PlayerCategory.cpp`) woła `value_ptr`, żeby ImGui
mogło edytować pozycję gracza. Od kamienia milowego M2 + M3 typu `glm::vec3` używają też
kolizje (`scene::Aabb` w [`src/scene/Collider.hpp`](../../src/scene/Collider.hpp): dwa
narożniki, dodawanie i odejmowanie wektorów, dostęp do składowej numerem, sekcja 3.2), układ
labiryntu (`src/game/MazeLayout.*`: pozycje ścian i słupków) i gracz
([`src/game/Player.cpp`](../../src/game/Player.cpp): `length` i `normalize` dla kierunku
ruchu). `src/game/MazeWorld.*` trzyma gotowe macierze modelu jako `std::vector<glm::mat4>`,
a wierzchołek siatki (`gfx::Vertex`) to trzy `glm::vec3` (pozycja, normalna, styczna)
i jeden `glm::vec2` (współrzędne tekstury).

Oświetlenie (M4) dołożyło kilka nowych użyć, wszystkie opisane niżej:

| Co | Gdzie w kodzie | Sekcja |
|---|---|---|
| `glm::mat3`, `glm::inverse`, `glm::transpose`: macierz normalnych (normal matrix) | `scene::normalMatrix` w [`src/scene/Transform.cpp`](../../src/scene/Transform.cpp), wołana przez `game::drawModel` (do M4 przez `MazeRenderer::drawInstances`, funkcję usuniętą w M5) | 3.3 |
| `glm::vec4{vec3, w}`: wektor czterech liczb zbudowany z trzech i jednej | `scene::packLightBlock` w [`src/scene/LightBlock.cpp`](../../src/scene/LightBlock.cpp) | 3.2 |
| `glm::length` i `glm::normalize` dla kierunków świateł | funkcja `unitDirection` w tym samym pliku | 3.8 |
| `glm::radians` dla kątów stożka i kątów księżyca | `coneCosines` i `directionFromAngles` w [`src/scene/Light.cpp`](../../src/scene/Light.cpp) | 3.7 |
| `glm::value_ptr` z `glUniformMatrix3fv` | `Shader::setMat3` w [`src/gfx/Shader.cpp`](../../src/gfx/Shader.cpp) | 3.9 |
| `glm::value_ptr` dla edytora koloru ImGui | `ImGui::ColorEdit3` w [`src/debug/categories/LightCategory.cpp`](../../src/debug/categories/LightCategory.cpp) | 3.9 |

Druga część M4, mapy normalnych, dołożyła jeden plik pełen GLM:
[`src/assets/Tangents.cpp`](../../src/assets/Tangents.cpp) liczy styczne wierzchołków
funkcjami `glm::dot`, `glm::cross`, `glm::length`, `glm::normalize` i `glm::abs` (wartość
bezwzględna każdej składowej wektoru, użyta do wyboru osi najmniej zgodnej z normalną).
Sam rachunek omawia [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md),
sekcje 2.7 i 2.8.

Rozgrywka (M5) dołożyła trzy funkcje GLM, których projekt wcześniej nie wołał, i kilka
nowych użyć znanych typów:

| Co | Gdzie w kodzie | Sekcja |
|---|---|---|
| `glm::dot(v, v)`: kwadrat długości wektora, bez pierwiastka | dwa testy kul `overlaps` w [`src/scene/Collider.cpp`](../../src/scene/Collider.cpp) | 3.8 |
| `glm::clamp(point, min, max)` na wektorach: najbliższy punkt pudełka | `scene::closestPoint` w tym samym pliku | 3.8 |
| `glm::two_pi<float>()`: stała 2π z `<glm/gtc/constants.hpp>` | okrąg z 32 punktów w [`src/game/ColliderLines.cpp`](../../src/game/ColliderLines.cpp), faza kołysania i pulsowania kryształów w [`src/game/Crystals.cpp`](../../src/game/Crystals.cpp) | 3.1 i 3.7 |
| `std::span<const glm::mat4>`: lista macierzy modelu, także jednoelementowa | `game::drawModel` w [`src/game/ModelDraw.cpp`](../../src/game/ModelDraw.cpp), wołana przez `MazeRenderer` i `GameplayRenderer`. Od M6 obok niej stoi `game::drawMesh`, która bierze jedną macierz `const glm::mat4&` i rysuje siatkę terenu | 3.3 |
| `glm::vec3` jako kolor świecenia i jako pozycje świateł | `game::crystalGlow` (kolor razy liczba), `game::crystalLightPositions` (`std::vector<glm::vec3>`) | 3.2 |
| `scene::Sphere`: `glm::vec3` środka i promień | [`src/scene/Collider.hpp`](../../src/scene/Collider.hpp), `game::playerReach` w `src/game/Round.cpp` | 3.2 |

Z M5 zniknęła za to kostka z M1 (`NightMazeApp::drawCube` i jej `Transform`) oraz znaczniki
świateł z M4, więc przykłady, które z nich korzystały, są niżej zastąpione kodem ścian,
bramy i kryształów.

Teren i trawa (druga część M6) nie wołają żadnej nowej funkcji GLM, ale używają znanych w
nowych rolach:

| Co | Gdzie w kodzie | Sekcja |
|---|---|---|
| `glm::mix(a, b, t)` na zwykłych liczbach `float`: trzy mieszania dają odczyt dwuliniowy mapy wysokości | `Heightmap::sample` w [`src/game/Terrain.cpp`](../../src/game/Terrain.cpp) | 3.8 |
| `glm::mix(-REACH, REACH, t)`: liczba losowa od 0 do 1 rozciągnięta na odcinek wzdłuż ściany | `plantAlongWall` w [`src/game/Grass.cpp`](../../src/game/Grass.cpp) | 3.8 |
| `glm::length(glm::vec2{...})`: odległość punktu od labiryntu, za rogiem liczona po skosie | `distanceOutsideMaze` w `src/game/Terrain.cpp` | 3.8 |
| `glm::normalize(glm::vec3{-slopeX, 1.0F, -slopeZ})`: normalna powierzchni z dwóch nachyleń | `Terrain::gridNormal` w tym samym pliku | 3.8 |
| `constexpr glm::mat4 IDENTITY{1.0F}`: macierz jednostkowa jako macierz modelu terenu, którego wierzchołki są już w przestrzeni świata | [`src/game/TerrainRenderer.cpp`](../../src/game/TerrainRenderer.cpp) | 3.3 |
| `glm::vec3` pozycji z wysokością czytaną z terenu | `GrassTuft::position`, `MazeWorld::startPosition`, pozycje ścian i słupków po `placeOnTerrain` | 3.2 |

Co te wzory znaczą, tłumaczą [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md)
i [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md).

Kod do M4 włącznie jest zbudowany i przetestowany na Windowsie (2026-10-05). Kod M5 jest na
Windowsie zbudowany w Debug i Release bez ostrzeżeń, a testy przechodziły (215 przypadków,
85098 asercji, stan po M5). Kod M6 (niebo, teren, trawa) jest na Windowsie kompletny: build
bez ostrzeżeń zgłosił wykonawca, a 256 przypadków testowych i 101232 asercje uruchomiłem
sam na programach testowych Debug i Release (2026-10-05). Po pierwszej części M7 zgłoszone
jest 269 przypadków i 102103 asercje, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751 (Windows, 2026-10-05), po piątej 329 i 104306 (Windows, 2026-10-06). Pierwsza część M7 dodała jedno nowe
miejsce użycia GLM: `gfx::srgbToLinear` i `gfx::linearToSrgb` w `src/gfx/ColorSpace.hpp`
przyjmują i zwracają `glm::vec3` (kolor przeliczany kanał po kanale,
[`../modules/gfx/color-space.md`](../modules/gfx/color-space.md)). Trzecia część M7 (mgła i winieta)
dodała dwa pliki z GLM w bibliotece `game_logic`: `src/game/Fog.*` (`glm::length` dla
odległości powierzchni od oka i mnożenie `glm::mat4` przez `glm::vec4` w
`worldPositionFromDepth`) oraz `src/game/Vignette.*` (`glm::length` i `glm::smoothstep`),
a `NightMazeApp::onRender` odwraca iloczyn `projection * view` funkcją `glm::inverse`
([`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.19,
5.12 i 5.13). Czwarta część M7 (cienie księżyca) dodała plik `src/scene/LightSpace.*`
z funkcją `scene::directionalLightSpace`: **pierwsze** użycie `glm::ortho` w projekcie (rzut
prostokątny światła kierunkowego, sześć argumentów: lewa, prawa, dolna i górna krawędź oraz
płaszczyzna bliska i daleka), drugie miejsce użycia `glm::lookAt` (widok światła, obok
`scene::Camera`) oraz `glm::min` i `glm::max` na wektorach, składowa po składowej, którymi
funkcja szuka najmniejszego pudełka wokół ośmiu narożników w przestrzeni światła. Struktura
`scene::LightSpace` trzyma dwie macierze `glm::mat4` i wektor `glm::vec3` z wymiarami pudełka,
a jej funkcja `matrix()` zwraca iloczyn `projection * view`
([`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.2 i 2.3). Na macOS kod M4, M5 i M6 nie był
budowany.

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
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, gfx/Vertex.hpp,
# scene/Transform.hpp, scene/Camera.hpp, scene/Collider.hpp, scene/Light.hpp,
# scene/LightBlock.hpp and others) expose GLM types, so every target that includes them
# needs the GLM include path too.
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
```

"Linkowanie" targetu `INTERFACE` niczego nie dopisuje do linkera. Oznacza tylko: przekaż
`engine` ścieżkę nagłówków GLM.

Dlaczego `PUBLIC`: nagłówki warstw `scene` i `gfx` pokazują typy GLM w swoim API (pozycja
kamery jako `glm::vec3`, macierz widoku jako `glm::mat4`, parametr `const glm::mat4&` w
`Shader::setMat4`). Komentarz wymienia siedem takich nagłówków z dopiskiem "and others":
`gfx/Shader.hpp`, `gfx/Vertex.hpp` (pola wierzchołka, od map normalnych także styczna
`glm::vec3 tangent`), `scene/Transform.hpp`, `scene/Camera.hpp`, `scene/Collider.hpp`,
`scene/Light.hpp` i `scene/LightBlock.hpp`. Każdy plik, który dołączy taki nagłówek,
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
projektu niczego w nich nie zgłasza. Na Windowsie (MSVC 19.44, `/W4 /permissive-`, Debug i
Release, 2026-10-05) te same pliki też kompilują się bez żadnego ostrzeżenia, a katalog
`_deps/glm-src` trafia do kompilatora jako zewnętrzny, przez `/external:I`
([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 5).

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
| `<glm/gtc/constants.hpp>` | stałe matematyczne jako szablony funkcji: `glm::two_pi<float>()` (jedyna, której projekt używa), `glm::pi<float>()`. Typ w nawiasach ostrych wybiera precyzję |

`gtc` to rozszerzenia stabilne (rzeczy spoza specyfikacji GLSL, ale z ustalonym API). Katalog
`gtx` zawiera rozszerzenia eksperymentalne. Nie używam ich: wymagają makra
`GLM_ENABLE_EXPERIMENTAL` i mogą się zmieniać między wersjami.

W nagłówku `.hpp`, który tylko deklaruje pole albo parametr typu `glm::vec3` lub `glm::mat4`,
wystarcza `<glm/glm.hpp>`. Dwa pozostałe nagłówki dołącza się w pliku `.cpp`, który faktycznie
buduje macierz albo wysyła ją do OpenGL.

Tak jest w `src/scene/`: `Transform.hpp` i `Camera.hpp` dołączają samo `<glm/glm.hpp>`, a
`Transform.cpp` i `Camera.cpp` dodatkowo `<glm/gtc/matrix_transform.hpp>`. W `src/gfx/`
`Shader.hpp` dołącza `<glm/glm.hpp>` (parametr typu `glm::mat4`), a `Shader.cpp`
`<glm/gtc/type_ptr.hpp>` (funkcja `value_ptr`). Pliki świateł trzymają się tej samej zasady:
`scene/Light.hpp` i `scene/LightBlock.hpp` dołączają samo `<glm/glm.hpp>`, a ich pliki `.cpp`
niczego więcej z GLM nie potrzebują (`radians`, `length` i `normalize` są w rdzeniu). Kategoria Light (zakładka Lights) (`src/debug/categories/LightCategory.cpp`) dołącza `<glm/gtc/type_ptr.hpp>` dla `value_ptr`.
Od M5 dwa pliki dołączają `<glm/gtc/constants.hpp>`: `src/game/ColliderLines.cpp`
i `src/game/Crystals.cpp`, oba dla `glm::two_pi<float>()`. Testy kul w `scene/Collider.cpp`
nie dołączają niczego nowego: `dot` i `clamp` są w rdzeniu, który przychodzi z `Collider.hpp`.

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
- Do składowej można sięgnąć także **numerem**: `v[0]` to ta sama liczba co `v.x`, `v[1]` to
  `v.y`, `v[2]` to `v.z`. Przydaje się, gdy jedna funkcja ma działać dla dowolnej osi. Tak
  jest w [`src/scene/Collider.cpp`](../../src/scene/Collider.cpp), gdzie numer osi jest
  parametrem ([`../modules/scene/collision.md`](../modules/scene/collision.md), sekcja 5.3).

**`glm::vec4{vec3, w}` w projekcie.** Konstruktor z przykładu wyżej (`vec3` rozszerzony o
czwartą liczbę) pakuje światła do bloku uniformów. Fragment `scene::packLightBlock` w
[`src/scene/LightBlock.cpp`](../../src/scene/LightBlock.cpp):

```cpp
    // glm::vec4{vec3, w}: the three floats of the vec3 followed by the fourth one.
    block.cameraPosition = glm::vec4{cameraPosition, 0.0F};
    block.ambient = glm::vec4{lights.ambient, 0.0F};

    block.directionalDirection = glm::vec4{unitDirection(lights.directional.direction), 0.0F};
    block.directionalColor = glm::vec4{lights.directional.color, lights.directional.intensity};
```

| Linia | Co trafia do czterech liczb |
|---|---|
| `glm::vec4{cameraPosition, 0.0F}` | x, y, z oka kamery, czwarta liczba nieużywana (zero) |
| `glm::vec4{lights.ambient, 0.0F}` | czerwony, zielony, niebieski światła otoczenia, czwarta nieużywana |
| `glm::vec4{unitDirection(...), 0.0F}` | kierunek światła księżyca o długości 1, czwarta nieużywana |
| `glm::vec4{lights.directional.color, lights.directional.intensity}` | kolor, a w czwartej liczbie natężenie światła |

Czwarta liczba nie jest tu współrzędną `w` punktu ani kierunku. Blok używa `vec4` dla
wszystkiego poza licznikiem świateł punktowych po to, żeby każdy taki element miał 16 bajtów i zaczynał się na wielokrotności 16
(`sizeof(glm::vec4) == 16` pilnuje `static_assert` w `src/scene/LightBlock.hpp`), a wolne
miejsce niesie czasem małą dodatkową wartość. Reguły układu `std140` opisuje
[`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md).

Prawdziwe użycie `w = 0` jako znacznika kierunku jest w teście
[`tests/TransformTests.cpp`](../../tests/TransformTests.cpp): funkcja pomocnicza
`transformDirection` liczy `glm::vec3{matrix * glm::vec4{direction, 0.0F}}`, czyli mnoży
kierunek przez macierz modelu tak, żeby przesunięcie go nie ruszyło, i z wyniku bierze
pierwsze trzy liczby (konstruktor `glm::vec3` z `glm::vec4` odcina czwartą).

### 3.3. Macierze: `mat3`, `mat4` i zapis kolumnowy

`mat4` to macierz 4 x 4 liczb `float` (64 bajty), `mat3` to 3 x 3. Macierz `mat4` opisuje
dowolne przekształcenie afiniczne (przesunięcie, obrót, skala) oraz rzutowanie. `mat3`
(9 liczb, 36 bajtów) służy w projekcie jako macierz normalnych (normal matrix): opis na
końcu tej sekcji.

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

**`mat3` w projekcie: macierz normalnych.** Od M4 istnieje jako funkcja `scene::normalMatrix`
w [`src/scene/Transform.cpp`](../../src/scene/Transform.cpp):

```cpp
glm::mat3 normalMatrix(const glm::mat4& modelMatrix) {
    // glm::mat3(mat4) keeps the upper left 3 x 3 part: rotation and scale, without the
    // translation in the fourth column.
    return glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
}
```

Wyrażenie czyta się od środka:

| Krok | Funkcja GLM | Co robi |
|---|---|---|
| 1 | `glm::mat3(modelMatrix)` | konstruktor `mat3` z `mat4`: bierze lewą górną część 3 x 3, czyli obrót i skalę. Czwarta kolumna z przesunięciem odpada, bo normalna jest kierunkiem i przesunięcie nie może jej ruszyć |
| 2 | `glm::inverse(...)` | macierz odwrotna. Macierz modelu musi być odwracalna, czyli żaden współczynnik skali nie może być zerem |
| 3 | `glm::transpose(...)` | transpozycja: zamiana wierszy z kolumnami |

Obie funkcje, `inverse` i `transpose`, są w rdzeniu (`<glm/glm.hpp>`), więc `Transform.cpp`
nie dołącza dla nich niczego nowego. Dlaczego odwrotność i transpozycja, a nie samo
`mat3(model)`: przy skali różnej na osiach normalna pomnożona przez `mat3(model)` przestaje
być prostopadła do powierzchni. Dla samego obrotu odwrotność transponowana jest tym samym
obrotem. Dziś nic w scenie (ściana, słupek, teren, brama, kryształ) nie jest skalowane, więc
wynik równa się części obrotowej macierzy modelu. Teoria i cztery testy z `tests/TransformTests.cpp`:
[`../modules/scene/transforms.md`](../modules/scene/transforms.md) (macierz normalnych).

Funkcję woła `game::drawModel` w
[`src/game/ModelDraw.cpp`](../../src/game/ModelDraw.cpp), dla każdego obiektu w każdej
klatce, zaraz po wysłaniu macierzy modelu. Do M4 ta pętla była prywatną funkcją
`MazeRenderer::drawInstances`. W M5 stała się wolną funkcją, bo tak samo rysowane są
kryształy i brama (`GameplayRenderer`):

```cpp
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
            // The lit programs turn the normals with a matrix of their own, derived
            // from the model matrix. It is computed here, on the CPU, once per object:
            // in the shader the inverse would be computed again for every vertex.
            shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
```

Wynik trafia do uniformu `uNormalMatrix` typu `mat3` w `lit.vert` i `gouraud.vert`. Ta sama
linia wykonuje się także wtedy, gdy scenę rysuje program `textured`, który takiego
uniformu nie ma: `setMat3` dostaje wtedy lokalizację -1 i OpenGL ją ignoruje (sekcja 3.9).
`textured.vert` nadal liczy normalną przez `mat3(uModel)`: to ten sam konstruktor co w
kroku 1, tylko w GLSL.

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
[`../modules/scene/transforms.md`](../modules/scene/transforms.md), sekcja 5.3.

Kto dziś woła `Transform::matrix()`. Ściana i brama, `game::wallModelMatrix` w
[`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp): przesunięcie na miejsce odcinka
i, dla odcinka wzdłuż osi Z, obrót o ćwierć obrotu wokół osi pionowej.

```cpp
glm::mat4 wallModelMatrix(const WallSegment& segment) {
    scene::Transform transform;
    transform.position = segment.position;
    if (segment.axis == WallAxis::AlongZ) {
        transform.rotationDegrees = WALL_ALONG_Z_ROTATION;
    }
    return transform.matrix();
}
```

Kryształ, `GameplayRenderer::draw` w
[`src/game/GameplayRenderer.cpp`](../../src/game/GameplayRenderer.cpp): przesunięcie, które
kołysze się w pionie, i obrót wokół osi Y, który rośnie z czasem. Macierz jest liczona od
nowa w każdej klatce, dla każdego niezebranego kryształu.

```cpp
        scene::Transform transform;
        transform.position =
            crystalBobPosition(crystal.restPosition, index, round.animationSeconds);
        transform.rotationDegrees = {0.0F, crystalSpinDegrees(index, round.animationSeconds), 0.0F};
        const glm::mat4 crystalMatrix = transform.matrix();
```

Trzeci użytkownik, `ColliderLines`, buduje macierz ze skali i przesunięcia (sześcian o boku 1
rozciągnięty do rozmiaru pudełka kolizji, okrąg o promieniu 1 do promienia kuli i obrócony
na trzy sposoby, po jednym okręgu wokół każdej osi). Do M4
czwartym była kostka z M1 z obrotem wokół dwóch osi: usunięta w M5.

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
jest parametrem: [`../modules/scene/camera.md`](../modules/scene/camera.md),
sekcje 2.1 i 5.5.

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

Tę samą zasadę stosują światła. W [`src/scene/Light.cpp`](../../src/scene/Light.cpp)
`directionFromAngles` zaczyna się od tych samych dwóch linii z `glm::radians` (kierunek
światła księżyca z kątów `Moon yaw` i `Moon pitch`), a `coneCosines` zamienia kąty stożka
latarki na radiany tuż przed policzeniem cosinusa:

```cpp
    // The standard library takes angles in radians.
    const float outer = std::cos(glm::radians(outerDegrees));
    const float inner = std::cos(glm::radians(innerDegrees));
```

`std::cos` to funkcja biblioteki standardowej C++, nie GLM, ale też liczy w radianach.
Opis obu funkcji: [`../modules/scene/lights.md`](../modules/scene/lights.md).

**Pełny obrót jako stała: `glm::two_pi`.** Tam, gdzie kąt nie przychodzi w stopniach z pola,
tylko jest częścią pełnego obrotu, nie ma czego zamieniać przez `glm::radians`: wystarczy
pomnożyć ułamek obrotu przez 2π. GLM ma tę stałą w `<glm/gtc/constants.hpp>`. Punkty okręgu
do rysowania kul kolizji, [`src/game/ColliderLines.cpp`](../../src/game/ColliderLines.cpp):

```cpp
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const float angle =
            glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(CIRCLE_SEGMENTS);
        points[i].position = {std::cos(angle), std::sin(angle), 0.0F};
    }
```

Punkt numer `i` leży pod kątem `i / 32` pełnego obrotu. `glm::two_pi<float>()` jest
szablonem funkcji `constexpr`, więc może też inicjalizować stałą, jak w
[`src/game/Crystals.cpp`](../../src/game/Crystals.cpp):

```cpp
constexpr float FULL_TURN_RADIANS = glm::two_pi<float>();
```

Tej stałej używają kołysanie kryształu i pulsowanie jego światła: faza ruchu (ułamek cyklu
od 0 do 1) razy `FULL_TURN_RADIANS` trafia do `std::sin`
([`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcja 2).

Teoria rzutowania (bryła widzenia, dzielenie perspektywiczne, nieliniowa głębia):
[`../modules/scene/camera.md`](../modules/scene/camera.md), sekcja 2.3.

### 3.8. `normalize`, `cross`, `dot`, `mix`

| Funkcja | Wynik | Do czego |
|---|---|---|
| `glm::normalize(v)` | wektor o tym samym kierunku i długości 1 | kierunki: kamera, normalne, promienie światła |
| `glm::dot(a, b)` | liczba: iloczyn skalarny (dot product). Dla wektorów jednostkowych to cosinus kąta między nimi | oświetlenie (kąt między normalną a światłem), sprawdzanie "czy patrzę w stronę" |
| `glm::cross(a, b)` | wektor prostopadły do obu: iloczyn wektorowy (cross product) | wektor "w prawo" kamery z kierunku patrzenia i góry |
| `glm::mix(x, y, a)` | `x * (1 - a) + y * a`: interpolacja liniowa | płynne przejście między dwiema wartościami |
| `glm::length(v)` | długość wektora | odległości |
| `glm::clamp(x, lo, hi)` | `x` sprowadzone do przedziału od `lo` do `hi`. Dla wektorów działa składowa po składowej | najbliższy punkt pudełka, ograniczanie wartości |

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

`normalize` i `length` w ruchu gracza, `Player::update` w
[`src/game/Player.cpp`](../../src/game/Player.cpp):

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

Ta sama para pilnuje kierunków świateł. Funkcja pomocnicza `unitDirection` w
[`src/scene/LightBlock.cpp`](../../src/scene/LightBlock.cpp), przez którą `packLightBlock`
przepuszcza kierunek księżyca i oś stożka latarki:

```cpp
// direction with length 1.
glm::vec3 unitDirection(const glm::vec3& direction) {
    if (glm::length(direction) < MIN_DIRECTION_LENGTH) {
        return FALLBACK_DIRECTION;
    }
    return glm::normalize(direction);
}
```

| Linia | Znaczenie |
|---|---|
| `glm::length(direction) < MIN_DIRECTION_LENGTH` | kierunek krótszy niż 0,0001 nie da się sprowadzić do długości 1 |
| `return FALLBACK_DIRECTION;` | zamiast niego wraca stała `(0, -1, 0)`, czyli prosto w dół |
| `return glm::normalize(direction);` | zwykły przypadek: ten sam kierunek o długości 1 |

Shader zakłada, że kierunki w bloku mają długość 1, i sam ich nie normalizuje. Jedna wartość
`NaN` w bloku zrobiłaby czarny każdy oświetlony piksel, stąd wyjście awaryjne zamiast
dzielenia przez zero. Sprawdza to test `packLightBlock replaces a direction of length zero`
w `tests/LightTests.cpp`.

**`dot` i `clamp` w testach kul (M5).** Zbieranie kryształów i wejście do wyjścia to testy
"czy kula nachodzi na kulę" i "czy kula nachodzi na pudełko". Oba są w
[`src/scene/Collider.cpp`](../../src/scene/Collider.cpp):

```cpp
bool overlaps(const Sphere& a, const Sphere& b) {
    // The squares are compared instead of the distances themselves: the square root that
    // a distance needs is the expensive part, and for numbers that are not negative
    // "smaller" means the same before and after squaring. dot(v, v) is the squared
    // length of v.
    const glm::vec3 offset = b.center - a.center;
    const float reach = a.radius + b.radius;
    return glm::dot(offset, offset) < reach * reach;
}

glm::vec3 closestPoint(const Aabb& box, const glm::vec3& point) {
    // Axis by axis: a coordinate between min and max stays, one outside is moved to the
    // nearer of the two. glm::clamp does that for all three components at once.
    return glm::clamp(point, box.min, box.max);
}

bool overlaps(const Sphere& sphere, const Aabb& box) {
    // The sphere reaches the box exactly when it reaches the point of the box nearest to
    // its centre. A centre inside the box is its own nearest point: the distance is 0,
    // and any radius above 0 overlaps.
    const glm::vec3 offset = closestPoint(box, sphere.center) - sphere.center;
    return glm::dot(offset, offset) < sphere.radius * sphere.radius;
}
```

| Linia | Znaczenie |
|---|---|
| `b.center - a.center` | odejmowanie wektorów: wektor od środka jednej kuli do środka drugiej |
| `glm::dot(offset, offset)` | iloczyn skalarny wektora z samym sobą to `x*x + y*y + z*z`, czyli **kwadrat** jego długości. `glm::length` policzyłoby z tego jeszcze pierwiastek |
| `< reach * reach` | porównuję kwadraty. Dla liczb nieujemnych `a < b` znaczy to samo co `a*a < b*b`, więc pierwiastek nie jest potrzebny. Znak `<`, a nie `<=`: kule, które się tylko stykają, nie nachodzą na siebie |
| `glm::clamp(point, box.min, box.max)` | trzy argumenty typu `glm::vec3`: każda składowa punktu jest osobno sprowadzana do przedziału tej samej składowej pudełka. Współrzędna wewnątrz pudełka zostaje, współrzędna na zewnątrz ląduje na bliższej ścianie. Wynik to punkt pudełka najbliższy podanemu |
| `closestPoint(box, sphere.center) - sphere.center` | wektor od środka kuli do najbliższego punktu pudełka. Jeśli jest krótszy niż promień, kula sięga pudełka |

Te funkcje mają siedem przypadków w `tests/ColliderTests.cpp`. Teorię i rysunki ma
[`../modules/scene/collision.md`](../modules/scene/collision.md), a to, kto ich używa w grze
(`updateRound`), [`../modules/game/gameplay.md`](../modules/game/gameplay.md).

`mix` przy rysowaniu, `NightMazeApp::onRender` w
[`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp):

> Uwaga (2026-10-06, M9 część 1): fragment `onRender` poniżej pochodzi sprzed kamery menu i jest skrócony. Dziś `onRender` woła na początku `updateMenuCameraSwitch()`, a macierze, kierunek latarki i podglądy bufora głębi bierze z kopii kamery `frameCamera` (kamera gracza albo poza kamery menu), a oko `eye` bywa podmienione na oko kamery menu. Klawisze R, N, F i M, obrót myszą i wskazywanie mają warunek `!menuCamera`, `frameLighting` nie jest `const` (w trybie menu latarka jest ustawiana osobno), a minimapa nie jest rysowana. Opis: [`menu-camera.md`](../modules/game/menu-camera.md), sekcja 5.4.

```cpp
    const glm::vec3 feet =
        glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha));
    const glm::vec3 eye = feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F};
```

Tak liczy się pozycję do narysowania między dwoma krokami symulacji: najpierw stopy gracza,
potem oko o stałą wysokość wyżej. Trzeci argument `mix` musi mieć typ składowych wektora
(`float`), a `alpha` przychodzi jako `double`, stąd `static_cast<float>`. Pełny opis obu
fragmentów: [`../modules/game/player.md`](../modules/game/player.md), sekcja 5, i
[`../modules/core/main-loop.md`](../modules/core/main-loop.md), sekcje 2.4 i 5.5.

### 3.9. `glm::value_ptr` i wysyłanie macierzy do shadera

`Shader::setMat4` w [`src/gfx/Shader.cpp`](../../src/gfx/Shader.cpp):

```cpp
void Shader::setMat4(const char* name, const glm::mat4& matrix) const {
    // The location is the number of the uniform inside this program. It is looked up on
    // every call: a few hundred lookups per frame (one per drawn object) are still cheap,
    // and there is no cache that could go stale after reload(). -1 means the program has
    // no active uniform with this name.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // 1: one matrix. GL_FALSE: do not transpose, GLM stores a matrix column by column,
    // which is the order OpenGL expects. value_ptr gives the address of its 16 floats.
    // OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}
```

Nagłówek `<glm/gtc/type_ptr.hpp>` jest dołączony na górze tego pliku. Funkcję omawia linia
po linii [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md), sekcja 5. Wołają ją
trzy funkcje rysujące `NightMazeApp` (`drawUnlitMaze`, `drawLitMaze`, `drawColliderLines`)
dla macierzy widoku i rzutowania oraz `game::drawModel` i `ColliderLines` dla macierzy modelu
(po razie dla każdego rysowanego obiektu). Przykład z `NightMazeApp::drawLitMaze`:

```cpp
    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
```

Trzecia macierz, modelu, idzie tą samą funkcją z pętli w `game::drawModel`
([`src/game/ModelDraw.cpp`](../../src/game/ModelDraw.cpp)):

```cpp
        for (const glm::mat4& modelMatrix : modelMatrices) {
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
```

Do M4 wszystkie trzy wywołania stały obok siebie w `NightMazeApp::drawCube`, funkcji
rysującej kostkę z M1. Kostki już nie ma.

`view` i `projection` to macierze policzone raz na klatkę w `onRender`
(`m_camera.viewMatrix(eye)` i `m_camera.projectionMatrix(aspectRatio)`), a nazwy uniformów
są stałymi z [`src/game/ShaderUniforms.hpp`](../../src/game/ShaderUniforms.hpp).

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

**Macierz 3 x 3: `Shader::setMat3`.** Macierz normalnych (sekcja 3.3) idzie do shadera
bliźniaczą funkcją z tego samego pliku:

```cpp
void Shader::setMat3(const char* name, const glm::mat3& matrix) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // The 3 x 3 version of the call in setMat4: one matrix, not transposed, 9 floats.
    GL_CHECK(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}
```

Różnice względem `setMat4` są dwie: typ parametru (`glm::mat3`) i funkcja OpenGL
(`glUniformMatrix3fv`). `glm::value_ptr` zwraca tu wskaźnik na 9 liczb `float`, znowu
kolumna po kolumnie, a `GL_FALSE` znaczy to samo co wyżej: nie transponuj. Słowo
"transpose" pojawia się więc w dwóch różnych miejscach i nie wolno ich mylić: `glm::transpose`
w `scene::normalMatrix` jest częścią wzoru, a argument `transpose` funkcji OpenGL dotyczy
tylko układu liczb w pamięci.

**`value_ptr` poza OpenGL: edytor koloru w oknie debug.** Wskaźnika na surowe liczby potrzebuje
też ImGui. Kategoria Light (zakładka Lights) w
[`src/debug/categories/LightCategory.cpp`](../../src/debug/categories/LightCategory.cpp):

```cpp
    page.color("Moon colour", glm::value_ptr(lighting.moonColor), "The colour of the moon light.");
```

`Page::color` (`src/debug/Widgets.cpp`) woła `ImGui::ColorEdit3`, który przyjmuje `float*` na trzy liczby i zapisuje przez niego nowy kolor.
`lighting.moonColor` nie jest tu stałą, więc `value_ptr` zwraca wskaźnik do zapisu (dla
obiektu `const` zwróciłby `const float*`). Tak samo edytowane są `Ambient`, `Beam colour`
i `Point colour`. Kategoria Player używa `value_ptr` w ten sam sposób dla pozycji gracza.

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
   kierunków ruchu, gdy żaden klawisz nie jest wciśnięty. `Player::update` normalizuje
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
14. **MSVC.** Zmierzony jest wynik: build Debug i Release w MSVC 19.44 pod
    `/W4 /permissive-` przechodzi bez żadnego ostrzeżenia. Wyjaśnienia w podpunktach niżej
    (C4201, `/Za`) pochodzą z lektury źródeł GLM i nie były osobno sprawdzane.
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
    - Zmierzone: pod `/W4` nie pojawia się żadne ostrzeżenie z nagłówków GLM, a katalog
      `_deps/glm-src` trafia do kompilatora jako zewnętrzny (`/external:I`, z
      `ExternalWarningLevel` równym `TurnOffAllWarnings`). Punkt listy kontrolnej w
      [`../guides/build-windows.md`](../guides/build-windows.md), sekcja 11, jest odhaczony.

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

11. **Jak w GLM powstaje macierz normalnych i jak trafia do shadera?**
    `glm::transpose(glm::inverse(glm::mat3(modelMatrix)))` w `scene::normalMatrix`:
    konstruktor `mat3` z `mat4` odcina przesunięcie, potem odwrotność i transpozycja.
    Wysyła ją `Shader::setMat3` przez `glUniformMatrix3fv` z `glm::value_ptr` (9 liczb,
    `transpose = GL_FALSE`) do uniformu `uNormalMatrix`.

12. **Co robi zapis `glm::vec4{color, intensity}` w `packLightBlock`?**
    Buduje wektor czterech liczb z `vec3` i jednej liczby `float`: trzy składowe koloru i
    natężenie w czwartej. Blok świateł używa `vec4` dla wszystkiego poza licznikiem
    `uPointCount`, żeby każdy taki element miał 16 bajtów.

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
