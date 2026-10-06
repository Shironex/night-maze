# Moduł gfx: uniformy

Kamień milowy: M1, rozszerzony w M2 + M3, w M4 i w M5, w drugiej części M7 o setter tablicy `setFloatArray`, a w czwartej części M7 o siedem uniformów mapy cieni księżyca i jedenasty program. Temat wykładu: 2 (Programowalny potok).
Kod: funkcje `setMat4`, `setInt`, `setVec3`, `setMat3`, `setFloat` i `setFloatArray` w [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp) i [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp), nazwy uniformów w [`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp), uniformy w shaderach z [`assets/shaders/`](../../../assets/shaders/), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp), [`src/game/MazeRenderer.cpp`](../../../src/game/MazeRenderer.cpp), [`src/game/GameplayRenderer.cpp`](../../../src/game/GameplayRenderer.cpp), [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp) i [`src/game/LightRig.cpp`](../../../src/game/LightRig.cpp).

Część modułu `gfx`. Wstęp do całego modułu jest w [`README.md`](README.md). Ten dokument jest dalszym ciągiem [`shaders.md`](shaders.md) (potok, język GLSL) i [`shader-class.md`](shader-class.md) (reszta klasy `gfx::Shader`). Skąd biorą się same macierze, opisują [`../scene/transforms.md`](../scene/transforms.md) (macierz modelu) i [`../scene/camera.md`](../scene/camera.md) (macierz widoku i rzutowania). Co dzieje się z uniformami po przeładowaniu shadera, widać w [`shader-hot-reload.md`](shader-hot-reload.md). Od M4 część danych trafia do shaderów inną drogą, przez blok uniformów w buforze: opisuje to [`uniform-buffers.md`](uniform-buffers.md), a różnicę streszcza sekcja 2.4. Ten dokument korzysta z makra `GL_CHECK` ([`../core/gl-check.md`](../core/gl-check.md)) i z typów GLM ([`../../libraries/glm.md`](../../libraries/glm.md)).

## 1. Po co to jest

Shader wierzchołków dostaje pozycję wierzchołka z bufora, ale żeby postawić obiekt w scenie, potrzebuje jeszcze trzech macierzy: modelu, widoku i rzutowania. Te macierze nie są danymi wierzchołka: są takie same dla wszystkich wierzchołków rysowanego obiektu, a dwie z nich zmieniają się najwyżej raz na klatkę. Do przekazywania takich wartości z C++ do shadera służą **uniformy**.

W M1 uniformy to były trzy linie `uniform mat4` w shaderze wierzchołków kostki demonstracyjnej i jedna funkcja klasy `gfx::Shader`, `setMat4`, wołana trzy razy w każdej klatce. W kamieniu milowym M2 + M3 doszły dwa programy shaderów i settery `setInt` oraz `setVec3` (sekcja 5.4). W M4 doszły dwa programy oświetlenia, mapy normalnych z uniformami `uNormalMap` i `uNormalMapEnabled` oraz settery `setMat3` i `setFloat` (sekcja 5.7). M5 usunęło kostkę razem z jej programem i dodało jeden uniform, `uEmissive`: własne świecenie kryształów (sekcja 4). Pierwsza część M6 dodała piąty program, `skybox`, z dwoma nowymi uniformami: `uSkybox` i `uBrightness`. Druga część M6 dodała szósty, `grass` (trawa z shadera geometrii), z czterema nowymi: `uTime`, `uBladeHeight`, `uWindStrength` i `uLit`. Pierwsza część M7 dodała dwa programy, które nie rysują sceny, `composite` i `preview`, z ośmioma nowymi uniformami: `uScene`, `uExposure`, `uToneMapping` oraz `uSource`, `uMode`, `uNear`, `uFar`, `uDepthRange` ([`../renderer/post-process.md`](../renderer/post-process.md)). Druga część M7 (bloom) dodała dwa kolejne programy bez sceny, `bright` i `blur`, osiem nowych stałych z nazwami (`uBloom`, `uBloomEnabled`, `uBloomIntensity` w programie `composite`, `uScene` i `uThreshold` w `bright`, `uSource`, `uHorizontal` i `uWeights` w `blur`) i szósty setter, `setFloatArray`, dla pierwszej **tablicy** uniformów w grze (sekcja 5.8). Po drugiej części M7 gra miała dziesięć programów i trzydzieści cztery nazwy zwykłych uniformów. Trzecia część M7 (mgła i winieta) dodała jedenaście nazw w programie `composite` i żadnego programu: czterdzieści pięć nazw. Czwarta część M7 (cienie księżyca, [`../renderer/shadows.md`](../renderer/shadows.md)) dodała jedenasty program, `shadow_depth`, który ma tylko trzy macierze, i siedem nowych nazw z pliku `common/shadows.glsl`: `uMoonShadowMap`, `uMoonShadowEnabled`, `uMoonShadowMatrix`, `uMoonShadowConstantBias`, `uMoonShadowSlopeBias`, `uMoonShadowPcfRadius` i `uMoonShadowStrength`, zadeklarowanych w trzech programach naraz (`lit`, `gouraud` i `grass`). Gra miała po czwartej części M7 **jedenaście programów** (po M8, części 1: czternaście) i **pięćdziesiąt dwie nazwy zwykłych uniformów** (sekcja 4: dwadzieścia siedem w sześciu programach sceny i dwadzieścia pięć w czterech programach przebiegów po scenie. Program `shadow_depth` nie ma żadnej własnej), a klasa ma sześć setterów. Wszystkie nazwy są zebrane w jednym nagłówku, `ShaderUniforms.hpp` (sekcja 5.5). Kodu jest mało, ale miejsc na pomyłkę, której OpenGL nie zgłosi, jest kilka:

| Pomyłka | Skutek |
|---|---|
| literówka w nazwie uniformu w C++ | pusty ekran bez żadnego komunikatu (pułapka 1) |
| `setMat4` przed `use()` | macierz trafia do innego programu albo wywołanie kończy się błędem (pułapka 2) |
| uniform ustawiony raz, przy starcie | wartość przepada po pierwszym przeładowaniu shadera (pułapka 3) |
| uniform ustawiony dla jednego obiektu i niecofnięty | wartość zostaje dla następnych obiektów i następnych klatek (pułapka 17) |

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

### 2.4 Zwykły uniform a blok uniformów

Wszystko w tym dokumencie dotyczy **zwykłych uniformów** (plain uniforms): zmiennych zadeklarowanych osobno, na przykład `uniform mat4 uModel;`. Od M4 projekt ma też **blok uniformów** (uniform block) `LightBlock` ze światłami sceny. Różnica w trzech wierszach:

| | Zwykły uniform | Pole bloku uniformów |
|---|---|---|
| gdzie leży wartość | w obiekcie programu | w buforze na karcie graficznej, wspólnym dla wielu programów |
| jak się ją ustawia | `glGetUniformLocation` i `glUniform*`, w każdym programie osobno, po `use()` | jedną wysyłką bajtów do bufora (`glBufferSubData`), bez `use()` |
| położenie (location) | ma | **nie ma**: `glGetUniformLocation` zwraca -1, a settery z tego dokumentu nic nie robią |

Blok opłaca się dla danych, których jest dużo, są wspólne dla kilku programów i zmieniają się raz na klatkę: tak wyglądają światła (58 wartości, dwa programy). Macierze, kolor materiału, własne świecenie powierzchni i trzy parametry odblasku zostały zwykłymi uniformami. Cały mechanizm (punkty wiązania, układ `std140`, klasa `gfx::UniformBuffer`, funkcja `Shader::bindUniformBlock`) opisuje [`uniform-buffers.md`](uniform-buffers.md).

## 3. Jak to działa w OpenGL

Uniform ustawia się w zlinkowanym programie, który jest bieżący. Zbudowanie programu i wybranie go przez `glUseProgram` to kroki od 1 do 15 w [`shader-class.md`](shader-class.md) (sekcja 3.1). Ustawienie uniformu typu `mat4`, co klatkę, po `glUseProgram` (krok 14 tamtej tabeli):

| # | Wywołanie | Co robi |
|---|---|---|
| 1 | `glGetUniformLocation(program, name)` | Zwraca położenie aktywnego uniformu o podanej nazwie w zlinkowanym programie albo -1, gdy takiego nie ma. Nie wymaga, żeby program był bieżący |
| 2 | `glUniformMatrix4fv(location, count, transpose, value)` | Kopiuje `count` macierzy 4 x 4 (po 16 liczb `float`) spod wskaźnika `value` do uniformu **bieżącego** programu. `transpose` równe `GL_FALSE` znaczy: liczby leżą kolumnami, tak jak chce OpenGL. Położenie -1 jest ignorowane bez błędu. Gdy żaden program nie jest bieżący: `GL_INVALID_OPERATION` |

Obie funkcje są w rdzeniu OpenGL od wersji 2.0, więc są dostępne w 4.1 Core i w nagłówku GLAD projektu. Na diagramie sekwencji w [`shader-class.md`](shader-class.md) (sekcja 3.2) stoją zaraz po `glUseProgram`.

## 4. Shadery

Gra ma czternaście programów (czternasty, `reflect` z M8, części 1, deklaruje `uModel`, `uView`, `uProjection` i `uNormalMatrix` w swoim shaderze wierzchołków, a w shaderze fragmentów uniformy własne oraz te z dołączanych `common/lighting.glsl`, `common/normal_map.glsl` i `common/shadows.glsl`, opisane w [`../renderer/env-mapping.md`](../renderer/env-mapping.md); dwa programy z szóstej części M7, `minimap` i `minimap_overlay`, mają po jednym lub dwa uniformy: `uMapToClip` w `minimap.vert` oraz `uMap` i `uOpacity` w `minimap_overlay.frag`) i każdy ma własny zestaw zwykłych uniformów. Tabela pokazuje cztery programy sceny i linii, a w komórkach stoi nazwa pliku, który uniform deklaruje. Piąty program, `skybox`, szósty, `grass`, i jedenasty, `shadow_depth`, mają osobne tabele pod spodem. Cztery programy przebiegów po scenie, `composite` i `preview` (pierwsza część M7) oraz `bright` i `blur` (druga część, bloom), nie mają macierzy modelu, widoku ani rzutowania i żadnego uniformu wspólnego ze sceną: ich dwadzieścia siedem uniformów (siedemnaście w `post/composite.frag`, z czego jedenaście od trzeciej części M7, pięć w `post/preview.frag`, dwa w `post/bright.frag`, trzy w `post/blur.frag`, żadnego w `post/composite.vert`) opisuje [`../renderer/post-process.md`](../renderer/post-process.md), a nazwy stoją w tym samym nagłówku `ShaderUniforms.hpp` pod przedrostkami `COMPOSITE_`, `PREVIEW_`, `BRIGHT_` i `BLUR_`:

| Uniform | Typ w GLSL | `textured` (scena bez oświetlenia i dwa widoki diagnostyczne) | `color` (linie pudełek i kul kolizji) | `lit` (scena, oświetlenie na fragment) | `gouraud` (scena, oświetlenie na wierzchołek) | Setter |
|---|---|---|---|---|---|---|
| `uModel` | `mat4` | `textured.vert` | `color.vert` | `lit.vert` | `gouraud.vert` | `setMat4` |
| `uView` | `mat4` | `textured.vert` | `color.vert` | `lit.vert` | `gouraud.vert` | `setMat4` |
| `uProjection` | `mat4` | `textured.vert` | `color.vert` | `lit.vert` | `gouraud.vert` | `setMat4` |
| `uNormalMatrix` | `mat3` | | | `lit.vert` | `gouraud.vert` | `setMat3` |
| `uTexture` | `sampler2D` | `textured.frag` | | `lit.frag` | `gouraud.frag` | `setInt` |
| `uTint` | `vec3` | `textured.frag` | | `lit.frag` | `gouraud.frag` | `setVec3` |
| `uEmissive` | `vec3` | `textured.frag` | | `lit.frag` | `gouraud.frag` | `setVec3` |
| `uNormalMap` | `sampler2D` | `textured.frag` (przez `#include`) | | `lit.frag` (przez `#include`) | | `setInt` |
| `uNormalMapEnabled` | `bool` | `textured.frag` (przez `#include`) | | `lit.frag` (przez `#include`) | | `setInt` |
| `uViewMode` | `int` | `textured.frag` | | | | `setInt` |
| `uSpecularModel` | `int` | | | `lit.frag` (przez `#include`) | `gouraud.vert` (przez `#include`) | `setInt` |
| `uSpecularStrength` | `float` | | | `lit.frag` (przez `#include`) | `gouraud.vert` (przez `#include`) | `setFloat` |
| `uShininess` | `float` | | | `lit.frag` (przez `#include`) | `gouraud.vert` (przez `#include`) | `setFloat` |
| `uColor` | `vec3` | | `color.frag` | | | `setVec3` |
| `uMoonShadowMap` | `sampler2DShadow` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setInt` |
| `uMoonShadowEnabled` | `bool` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setInt` |
| `uMoonShadowMatrix` | `mat4` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setMat4` |
| `uMoonShadowConstantBias` | `float` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setFloat` |
| `uMoonShadowSlopeBias` | `float` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setFloat` |
| `uMoonShadowPcfRadius` | `int` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setInt` |
| `uMoonShadowStrength` | `float` | | | `lit.frag` (przez `#include`) | `gouraud.frag` (przez `#include`) | `setFloat` |
| razem zwykłych uniformów | | 9 | 4 | 19 (12 do trzeciej części M7) | 17 (10 do trzeciej części M7) | |

Siedem wierszy `uMoonShadow...` doszło w czwartej części M7. Deklaruje je plik `common/shadows.glsl`, dołączany do shaderów **fragmentów** obu programów: także w programie `gouraud`, w którym światło liczy shader wierzchołków, cień jest sprawdzany dla każdego fragmentu. Wszystkie siedem ustawia jedna funkcja, `game::setShadowUniforms` (sekcja 5.2). `sampler2DShadow` ustawia się przez `setInt`, jak każdy sampler: przechowuje numer jednostki teksturującej, tu 3.

Od piątej części M7 (cień latarki, 2026-10-06) ten sam plik ma jeszcze osiem uniformów latarki, `uFlashlightShadowMap`, `uFlashlightShadowEnabled`, `uFlashlightShadowMatrix`, `uFlashlightShadowConstantBias`, `uFlashlightShadowSlopeBias`, `uFlashlightShadowPcfRadius`, `uFlashlightShadowStrength` i `uFlashlightShadowLightPosition` (typy jak wyżej, a ostatni to `vec3`). Dostają je te same trzy programy (`lit`, `gouraud`, `grass`), tym samym `game::setShadowUniforms` (teraz przez `NightMazeApp::setShadowUniformsOf`, które woła go dwa razy). Nazwy siedmiu pierwszych są w drugiej stałej `FLASHLIGHT_SHADOW_UNIFORMS`, a ósma w nowym polu `lightPosition` struktury `ShadowUniformNames` (dla księżyca `nullptr`). Liczby w wierszu "razem" tabeli wyżej opisują stan po czwartej części. Sampler latarki ma jednostkę 4, a dwa biasy są w metrach (`game::biasForShader`), nie w jednostkach głębi.

Program `skybox` (pierwsza część M6, [`../renderer/skybox.md`](../renderer/skybox.md), sekcja 4.3):

| Uniform | Typ w GLSL | Plik | Setter | Uwaga |
|---|---|---|---|---|
| `uView` | `mat4` | `skybox.vert` | `setMat4` | ta sama nazwa i ta sama macierz co w pozostałych programach. Shader używa z niej samego obrotu |
| `uProjection` | `mat4` | `skybox.vert` | `setMat4` | jak w pozostałych programach |
| `uSkybox` | `samplerCube` | `skybox.frag` | `setInt` | nowa nazwa. Sampler tekstury sześciennej: przechowuje numer jednostki teksturującej, jak `sampler2D` |
| `uBrightness` | `float` | `skybox.frag` | `setFloat` | nowa nazwa. Mnożnik koloru nieba |
| `uViewMode` | `int` | `skybox.frag` | `setInt` | ta sama nazwa i te same liczby co w `textured.frag` |

Uniformu `uModel` program nieba nie ma: sześcian nieba nie stoi nigdzie w świecie.

Program `grass` (druga część M6, [`../renderer/grass-geometry.md`](../renderer/grass-geometry.md)). Ma trzy pliki, więc kolumna "Plik" wymienia także shader geometrii:

| Uniform | Typ w GLSL | Plik | Setter | Uwaga |
|---|---|---|---|---|
| `uView` | `mat4` | `grass.geom` | `setMat4` | ta sama nazwa i ta sama macierz co w pozostałych programach, ale zadeklarowana w shaderze **geometrii**, a nie wierzchołków: to on wypisuje `gl_Position` w przestrzeni przycięcia |
| `uProjection` | `mat4` | `grass.geom` | `setMat4` | jak wyżej |
| `uTime` | `float` | `grass.geom` | `setFloat` | nowa nazwa. Zegar wiatru w sekundach: wartość `glfwGetTime()` z `NightMazeApp::drawGrass` |
| `uBladeHeight` | `float` | `grass.geom` | `setFloat` | nowa nazwa. Wysokość najwyższych źdźbeł w metrach, suwak `Blade height` panelu Grass |
| `uWindStrength` | `float` | `grass.geom` | `setFloat` | nowa nazwa. Siła wiatru, 0 to bezruch, suwak `Wind strength` |
| `uLit` | `bool` | `grass.frag` | `setInt` | nowa nazwa. 1: trawa oświetlona światłami sceny, 0: pełna jasność (tryb `Unlit`). `bool` ustawia się przez `setInt`, jak `uNormalMapEnabled` |
| `uViewMode` | `int` | `grass.frag` | `setInt` | ta sama nazwa i te same liczby co w `textured.frag` i `skybox.frag` |
| `uSpecularModel` | `int` | `grass.frag` (przez `#include`) | (nikt) | zadeklarowany w `common/lighting.glsl`. `GrassRenderer` go **nie ustawia**: zostaje przy wartości początkowej 0, a wynik odblasku i tak nie jest używany |
| `uSpecularStrength` | `float` | `grass.frag` (przez `#include`) | `setFloat` | ustawiany na 0 (stała `NO_SPECULAR_STRENGTH`): trawa nie ma odblasku |
| `uShininess` | `float` | `grass.frag` (przez `#include`) | `setFloat` | ustawiany na 1 (stała `PLAIN_SHININESS`), żeby potęgowanie w `computeLighting` miało określony wynik |
| siedem uniformów `uMoonShadow...` (czwarta część M7) | jak w pierwszej tabeli | `grass.frag` (przez `#include "common/shadows.glsl"`) | `game::setShadowUniforms` | trawa leży w cieniu księżyca jak ziemia, na której rośnie. Ustawia je nie `GrassRenderer`, tylko `NightMazeApp::drawGrass`, po własnym `m_grassShader.use()` |

Uniformu `uModel` program trawy też nie ma: punkty kępek są w buforze od razu w przestrzeni świata. Wszystkie siedem uniformów własnych ustawia jedna funkcja, `game::GrassRenderer::draw`, po własnym `use()`. Trzy uniformy odblasku trafiły do programu razem z plikiem `common/lighting.glsl`, z którego trawa bierze tylko część rozproszoną (`computeLighting(...).diffuse`). Czy są **aktywne**, zależy od kompilatora sterownika: skoro wynik odblasku nie wpływa na kolor fragmentu, sterownik może je usunąć, a ich położenie będzie wtedy równe -1 (sekcja 2.3). Dla C++ nie ma to znaczenia, bo ustawienie nieistniejącego uniformu jest ignorowane. Tego, co zgłasza sterownik, nie mierzyłem. Od czwartej części M7 program `grass` ma razem siedemnaście zadeklarowanych uniformów (dziesięć wcześniej).

Program `shadow_depth` (czwarta część M7, [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 4) rysuje scenę z kierunku księżyca do mapy cieni:

| Uniform | Typ w GLSL | Plik | Setter | Uwaga |
|---|---|---|---|---|
| `uModel` | `mat4` | `shadow_depth.vert` | `setMat4` | ta sama nazwa co w programach sceny, celowo: `TerrainRenderer`, `MazeRenderer` i `GameplayRenderer` rysują tym programem bez żadnej zmiany w swoim kodzie |
| `uView` | `mat4` | `shadow_depth.vert` | `setMat4` | ta sama nazwa, **inna macierz**: widok światła (`scene::LightSpace::view`), nie kamery |
| `uProjection` | `mat4` | `shadow_depth.vert` | `setMat4` | rzut ortograficzny światła (`scene::LightSpace::projection`) |

Nowych nazw ten program nie ma, a `shadow_depth.frag` nie ma żadnego uniformu: jego `main` jest puste. Renderery ustawiają przy rysowaniu także `uTexture`, `uNormalMap`, `uEmissive`, `uTint` i `uNormalMatrix`, których ten program nie zna: wszystkie trafiają w położenie -1 i są ignorowane (sekcja 2.3, komentarz w `NightMazeApp::drawShadowCasters`).

W sześciu programach sceny: dwadzieścia siedem nazw i 71 zadeklarowanych uniformów (49 w czterech programach z pierwszej tabeli, 5 w programie `skybox` i 17 w programie `grass`). Do trzeciej części M7 było to dwadzieścia nazw i 50 uniformów: różnica to siedem nazw mapy cieni, zadeklarowanych trzy razy. Z czterema programami przebiegów po scenie (dwadzieścia pięć nazw, 27 uniformów) i z programem `shadow_depth` (3 uniformy, żadnej nowej nazwy) razem: pięćdziesiąt dwie nazwy, jedenaście programów (stan po czwartej części M7; po M8, części 1 programów jest czternaście: `minimap`, `minimap_overlay` i `reflect` doszły później), 101 zadeklarowanych uniformów. Historia: po pierwszej części M7 dwadzieścia osiem nazw, osiem programów i 58 uniformów, po drugiej trzydzieści cztery, dziesięć i 66, po trzeciej czterdzieści pięć, dziesięć i 77. Liczby są policzone z linii `uniform` w plikach shaderów po rozwinięciu `#include`, nie odczytane od sterownika. Tablica `uWeights` liczy się tu jako jeden uniform, chociaż ma siedem elementów. "Scena" znaczy tu teren, ściany i słupki labiryntu razem z kryształami i bramą wyjścia: wszystko to rysuje w jednej klatce ten sam program (sekcja 5.2). Do M5 podłożem były płytki podłogi, od drugiej części M6 jest nim teren z mapy wysokości ([`../renderer/terrain.md`](../renderer/terrain.md)). Dwa uniformy map normalnych są zadeklarowane w pliku [`assets/shaders/common/normal_map.glsl`](../../../assets/shaders/common/normal_map.glsl), dołączanym do `lit.frag` i do `textured.frag` ([`normal-mapping.md`](normal-mapping.md), sekcja 4). Program `gouraud` ich nie ma: liczy światło w wierzchołkach i z mapy normalnych nie korzysta. Trzy uniformy odblasku są zadeklarowane w pliku [`assets/shaders/common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl), który nie jest samodzielnym shaderem: jego tekst trafia do `lit.frag`, do `gouraud.vert` i, od drugiej części M6, do `grass.frag` przez linię `#include "common/lighting.glsl"` ([`shader-includes.md`](shader-includes.md)). Dlatego w programach `lit` i `grass` należą do shadera fragmentów, a w programie `gouraud` do shadera wierzchołków. Dla C++ nie ma to znaczenia: uniform należy do programu jako całości i ustawia się go tak samo. Ten sam plik deklaruje blok `LightBlock`, którego pola (na przykład `uAmbient`) **nie są** zwykłymi uniformami i w tej tabeli ich nie ma (sekcja 2.4).

Trzy macierze mają we wszystkich czterech shaderach wierzchołków te same nazwy, a od czwartej części M7 także w piątym, `shadow_depth.vert`, gdzie `uView` i `uProjection` są macierzami światła. W `textured.vert` i `color.vert` stoją w jednym wyrażeniu: `gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);`. Nad tą linią w `textured.vert` stoi komentarz, który rozpisuje łańcuch krok po kroku:

```glsl
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
```

W M1 ten komentarz stał w shaderze kostki. `color.vert` odsyła dziś do niego jednym zdaniem ("The same chain as in textured.vert"). W `lit.vert` i `gouraud.vert` ten sam łańcuch jest rozbity na dwa kroki, bo oświetlenie potrzebuje pozycji w przestrzeni świata: `vec4 worldPosition = uModel * vec4(aPosition, 1.0);`, a potem `gl_Position = uProjection * uView * worldPosition;`. Potok i język GLSL opisuje [`shaders.md`](shaders.md), parę `textured.*` [`textures.md`](textures.md) (sekcja 4), parę `color.*` [`../scene/collision.md`](../scene/collision.md) (sekcja 4), pary `lit.*` i `gouraud.*` [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md) (sekcja 4), a plik `common/lighting.glsl` [`../scene/lights.md`](../scene/lights.md) (sekcja 4). Para `color.*` jest najprostsza w projekcie: shader wierzchołków ma tylko trzy macierze, a shader fragmentów jeden uniform, `uColor`.

**`uNormalMatrix`.** To jedyny uniform typu `mat3`. Jest to macierz, która przenosi **normalne** z przestrzeni lokalnej do przestrzeni świata: odwrócona i transponowana lewa górna część 3 x 3 macierzy modelu. Liczy ją na procesorze funkcja `scene::normalMatrix` dla każdego rysowanego obiektu w każdej klatce, a `game::drawModel` wysyła ją zaraz po `uModel` (sekcja 5.2). Shader używa jej w jednej linii: `vNormal = uNormalMatrix * aNormal;` w `lit.vert` i `vec3 normal = normalize(uNormalMatrix * aNormal);` w `gouraud.vert`. Dlaczego normalne potrzebują własnej macierzy, tłumaczy [`../scene/lights.md`](../scene/lights.md), a kod funkcji [`../scene/transforms.md`](../scene/transforms.md). `textured.vert` nadal obraca normalną przez `mat3(uModel)` i uniformu `uNormalMatrix` nie ma ([`textures.md`](textures.md), sekcja 4).

**`uEmissive`.** Uniform dodany w M5: światło, które powierzchnia oddaje sama z siebie, zapisane jako kolor. Deklarują go trzy shadery fragmentów sceny. W `lit.frag` dołącza do światła rozproszonego: `fragColor = vec4(surface * (diffuse + uEmissive) + specular, 1.0);`. W `gouraud.frag` stoi ta sama linia, ze światłem policzonym w wierzchołkach. Zmienne `diffuse` i `specular` są od czwartej części M7 światłem po odjęciu cienia księżyca (do trzeciej części stały tam wprost `lighting.diffuse` i `lighting.specular`, a w `gouraud.frag` `vDiffuseLight` i `vSpecularLight`). `uEmissive` jest dodawane **po** odjęciu cienia, więc kryształ świeci tak samo w cieniu ściany i poza nim. W `textured.frag` uniform działa tylko w zwykłym widoku (wartość 0 uniformu `uViewMode`): `fragColor = vec4(texel * uTint * (vec3(1.0) + uEmissive), 1.0);`. Dla terenu, ścian, słupków i bramy wartość jest czarna, czyli `(0, 0, 0)`, i wzory dają to samo co bez tego uniformu. Dla kryształów jest to wynik `game::crystalGlow`. Po co kryształ świeci sam, mówi komentarz w `lit.frag`: światło punktowe kryształu wisi poza jego siatką i oświetla ścianki tylko z jednej strony, więc bez własnego świecenia źródło światła byłoby najciemniejszą rzeczą w okolicy. Reguły gry i funkcję `crystalGlow` opisuje [`../game/gameplay.md`](../game/gameplay.md) (sekcja 4).

**Ta sama nazwa, osobna wartość w każdym programie.** Uniform należy do programu (sekcja 2.1). `uView` w programie `color` i `uView` w programie `lit` to dwa różne uniformy, które przypadkiem nazywają się tak samo. Macierz widoku trzeba więc wysłać do każdego używanego programu osobno, po jego `use()`. Dlatego każda funkcja rysująca w `NightMazeApp` zaczyna od `use()` i od własnych wywołań `setMat4` (sekcja 5.2). W jednej klatce pracują najwyżej **cztery** z sześciu programów: scenę rysuje dokładnie jeden z trójki `textured`, `lit`, `gouraud`, trawę program `grass`, gdy jest włączona, linie pudełek i kul kolizji program `color`, i to tylko wtedy, gdy są włączone, a niebo program `skybox`, gdy pole `Skybox` jest zaznaczone. Programy `skybox` i `grass` nie zaczynają w `NightMazeApp`: swoje `use()` i swoje settery mają w `Skybox::draw` i w `GrassRenderer::draw`.

Wszystkie uniformy z tabeli są aktywne, bo każdy wpływa na wynik swojego programu. Wystarczy usunąć macierz z wyrażenia, żeby kompilator usunął uniform, a jego położenie zmieniło się na -1 (ćwiczenie 1).

**Trzy macierze zamiast jednej.** Shader mógłby dostać jedną gotową macierz, iloczyn wszystkich trzech policzony w C++. Tak robi wiele prawdziwych rendererów: jedno mnożenie macierzy na wierzchołek zamiast trzech. W projekcie macierze są osobno celowo: każdą da się podmienić i obejrzeć skutek (ćwiczenia w sekcji 8 dokumentów [`../scene/transforms.md`](../scene/transforms.md) i [`../scene/camera.md`](../scene/camera.md)). Od M4 jest też powód praktyczny: `lit.vert` i `gouraud.vert` potrzebują samego `uModel * pozycja`, czyli pozycji w przestrzeni świata, w której liczone jest oświetlenie.

## 5. Kod w projekcie

Uniformy dotykają trzech miejsc w kodzie: sześciu setterów klasy `gfx::Shader` w `src/gfx/` (sekcje 5.1, 5.4, 5.7 i 5.8), nagłówka z nazwami `src/game/ShaderUniforms.hpp` (sekcja 5.5) i wywołań setterów w kodzie rysującym (sekcja 5.2). Reszta klasy `gfx::Shader` jest opisana w [`shader-class.md`](shader-class.md) (sekcja 5).

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

**Położenie jest wyszukiwane przy każdym wywołaniu, bez pamięci podręcznej.** Typowa klasa shadera trzyma mapę "nazwa na położenie", żeby nie pytać sterownika przy każdym ustawieniu. Tu jej nie ma: każda pamięć podręczna musiałaby być czyszczona w `reload()`, bo nowy program może nadać uniformom inne położenia, a mapa, o której czyszczeniu można zapomnieć, to źródło błędów trudnych do znalezienia. Decyzja zapadła w M1, gdy wyszukiwań było trzy na klatkę, i wtedy koszt był pomijalny. Dziś jest ich ponad tysiąc na klatkę (od czwartej części M7 scena jest rysowana dwa razy, do mapy cieni i do obrazu) i tej ceny nie zmierzyłem (sekcja 5.6).

**Program musi być w użyciu.** `glUniformMatrix4fv` nie przyjmuje identyfikatora programu: pisze do programu bieżącego (sekcja 2.2). `setMat4` **nie woła** `use()` samo. Gdyby wołało, ustawienie uniformu po cichu zmieniałoby bieżący program, a kilkaset macierzy na klatkę oznaczałoby kilkaset zbędnych `glUseProgram`. Kolejność "najpierw `use()`, potem `setMat4`" należy do wołającego i jest zapisana w komentarzu Doxygen. Jej złamanie nie zawsze daje błąd: macierz trafia wtedy do innego programu (pułapka 2).

**Nieznana nazwa jest ignorowana po cichu.** Dla nazwy, której program nie ma, `glGetUniformLocation` zwraca -1, a `glUniformMatrix4fv` z położeniem -1 nic nie robi i nie zgłasza błędu. `setMat4` tego nie sprawdza i niczego nie loguje. Rozważałem `logWarn` przy -1 i odrzuciłem z dwóch powodów. Funkcja jest wołana co klatkę, więc ostrzeżenie pojawiałoby się 60 razy na sekundę i zalało konsolę. Po drugie -1 nie zawsze jest pomyłką: uniform usunięty przez kompilator jako nieużywany (na przykład w trakcie eksperymentu z shaderem, ćwiczenie 1) też ma położenie -1, a ustawianie go jest poprawne. Skutek trzeba po prostu znać: literówka w nazwie daje pusty ekran bez żadnego komunikatu (pułapka 1).

Te trzy decyzje dotyczą tak samo pięciu pozostałych setterów: `setInt` i `setVec3` (sekcja 5.4), `setMat3` i `setFloat` (sekcja 5.7) oraz `setFloatArray` (sekcja 5.8). Settery innych typów (na przykład `vec4`) dojdą wtedy, gdy shader będzie ich potrzebował. Światła, które miałyby najwięcej takich uniformów, idą inną drogą: przez blok uniformów (sekcja 2.4).

### 5.2 Użycie w kodzie rysującym

Klatkę rysują dwa kroki `NightMazeApp::onRender`: `drawMaze` i `drawColliderLines` (tylko gdy rysowanie kształtów kolizji jest włączone). `drawMaze` sama niczego nie ustawia: wybiera jedną z dwóch funkcji, `drawUnlitMaze` (program `textured`) albo `drawLitMaze` (program `lit` albo `gouraud`). Każda funkcja rysująca wybiera swój program i ustawia jego uniformy. Macierze `view` i `projection` są liczone raz w `onRender` i przekazywane wszystkim ([`../scene/camera.md`](../scene/camera.md), sekcja 5). Który tryb wybiera którą funkcję, opisuje [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md). W M1 był tu jeszcze trzeci krok, `drawCube`, z trzema wywołaniami `setMat4` dla kostki demonstracyjnej, a w M4 czwarty, który rysował znaczniki świateł. Oba usunęło M5.

**Scena bez oświetlenia**, `NightMazeApp::drawUnlitMaze` (tryb `Unlit` oraz oba widoki diagnostyczne w każdym trybie):

```cpp
    // The uniforms belong to the program in use, so use() comes before the setters.
    m_texturedShader.use();
    m_texturedShader.setMat4(VIEW_UNIFORM, view);
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    // The enum values are the numbers textured.frag compares uViewMode with.
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    // Only the view of the normals reads it: that view shows the normals the lighting
    // would use, so with normal mapping the ones from the normal maps.
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);

    // The ground first, then what stands on it. The order does not change the picture
    // (the depth test sorts it out), it only follows the way the scene is built.
    m_terrainRenderer.draw(m_texturedShader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld, m_wallMatrices);
    // The crystals and the gate, with the same program: they show up in the debug
    // views like the walls do. (In the picture without lighting the crystals may be
    // left to the reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(m_texturedShader);
    // The levers and the notes. The highlight of the picked one shows in the picture
    // without lighting. The two debug views show data and ignore it.
    drawInteractables(m_texturedShader);
```

Uwaga (2026-10-06): do M8, części 1 ta funkcja kończyła się trzema wywołaniami `draw` (teren, `m_mazeRenderer.draw(m_texturedShader, m_mazeWorld)` i `m_gameplayRenderer.draw(...)`). Dziś ściany dostają listę macierzy klatki (`m_wallMatrices`, ściany otwarte dźwigniami zapadają się w ziemię), bramę i kryształy rysuje `drawGateAndCrystals` (kryształów nie rysuje, gdy robi to przebieg odbić programem `reflect`), a dźwignie i kartki `drawInteractables`.

**Scena z oświetleniem**, `NightMazeApp::drawLitMaze`:

```cpp
    const gfx::Shader& shader =
        m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;
    if (!shader.isValid()) {
        return;
    }

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    // The material of the stone, which the ground shares. The lights themselves are not
    // set here: they are in the uniform buffer that onRender filled before this call.
    // The enum values are the numbers common/lighting.glsl compares uSpecularModel with.
    shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    shader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    // Normal mapping, the switch of the lit program (1 on, 0 off). The Gouraud program
    // has no such uniform, and usesNormalMap is false for it anyway.
    shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    // The two shadow maps (the moon and the flashlight). Set in every frame, also with
    // the shadows off.
    setShadowUniformsOf(shader);

    // The ground first, then what stands on it, as in drawUnlitMaze.
    m_terrainRenderer.draw(shader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(shader, m_mazeWorld, m_wallMatrices);
    // The crystals and the gate, with the same program and so the same lighting mode.
    // The crystals glow in the colour of their lights. (The crystals may be left to the
    // reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(shader);
    // The levers and the notes, lit like the walls they hang on.
    drawInteractables(shader);
```

Ten sam kod obsługuje dwa programy: `shader` jest referencją do `m_gouraudShader` albo do `m_litShader`. Oba mają te same nazwy uniformów oświetlenia, więc reszta funkcji nie musi wiedzieć, który wybrano. Wyjątkiem jest `uNormalMapEnabled`: ma go tylko program `lit`. W programie `gouraud` to ustawienie trafia w położenie -1 i jest ignorowane (sekcja 2.3), a wysyłana wartość i tak byłaby zerem, bo `game::usesNormalMap` zwraca fałsz dla trybu `Gouraud` ([`../game/flashlight.md`](../game/flashlight.md)). Uniform jest w GLSL typu `bool`, a ustawia go `setInt`: tak ustawia się `bool` w OpenGL (sekcja 5.4). Świateł tu nie ma: są w buforze uniformów, wypełnionym wcześniej w `onRender` ([`uniform-buffers.md`](uniform-buffers.md), sekcja 5).

Linia `setShadowUniforms(...)` doszła w czwartej części M7 (dziś w jej miejscu stoi `setShadowUniformsOf(shader)`, które woła tę funkcję dwa razy: dla mapy księżyca z jednostką 3 i, od piątej części M7, dla mapy latarki z jednostką 4; opis niżej dotyczy jednego wywołania). To wolna funkcja z `src/game/ShadowMap.cpp`, która ustawia siedem zwykłych uniformów mapy cieni: sampler (`setInt`, numer jednostki 3), przełącznik (`setInt`), macierz światła (`setMat4`), dwie części biasu przeliczone z metrów na jednostki głębi (`setFloat`), promień PCF (`setInt`) i siłę cienia (`setFloat`). Nazwy dostaje w strukturze `MOON_SHADOW_UNIFORMS` (sekcja 5.5). Dlaczego macierz światła jest zwykłym uniformem, a nie polem bloku `LightBlock`: sampler nie może być polem bloku uniformów, a liczby, które należą do jednej mapy, mają stać obok jej samplera (komentarz w `common/shadows.glsl`). Blok `LightBlock` się przez to nie zmienił. Funkcja jest wołana w każdej klatce, **także przy wyłączonych cieniach**: po przeładowaniu shaderów każdy uniform wraca do 0, a sampler cienia zostawiony na jednostce 0 dzieliłby ją z samplerem tekstury koloru, czego OpenGL nie pozwala narysować (dwa samplery różnych typów na jednej jednostce). Ta sama funkcja jest wołana też w `NightMazeApp::drawGrass` (dla programu `grass`) i w `drawReflections` (dla programu `reflect`), obie przez `setShadowUniformsOf`.

Obie funkcje ustawiają to, co jest wspólne dla całej klatki, i oddają program trzem klasom, w tej kolejności: `TerrainRenderer` rysuje teren (od drugiej części M6, w miejscu płytek podłogi), `MazeRenderer` ściany i słupki, a `GameplayRenderer` bramę i kryształy (od M8, części 2 dochodzi czwarta klasa, `InteractableRenderer`, dla dźwigni i kartek, wołana przez `drawInteractables`). Wszystkie te metody `draw` dostają shader jako parametr i **nie wołają** `use()`: zakładają, że wołający już wybrał program. To ta sama zasada co w `setMat4` (sekcja 5.1). Kryształy i brama idą tym samym programem co ściany, więc mają ten sam tryb oświetlenia i te same widoki diagnostyczne.

`uModel` nie ma w żadnej z dwóch funkcji `NightMazeApp`: scena to wiele obiektów i każdy ma własną macierz modelu. Wspólną część trzech klas rysujących, czyli samplery, tekstury, kolor materiału, macierz modelu i macierz normalnych, zawierają trzy wolne funkcje w [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp) (trzecia, `drawMesh`, doszła w drugiej części M6 i jest opisana niżej). Pierwsza, `game::setModelSamplers`, ustawia dwa samplery:

```cpp
void setModelSamplers(const gfx::Shader& shader) {
    shader.setInt(TEXTURE_UNIFORM, static_cast<int>(TEXTURE_UNIT));
    shader.setInt(NORMAL_MAP_UNIFORM, static_cast<int>(NORMAL_MAP_UNIT));
}
```

Druga, `game::drawModel`, rysuje jeden model raz dla każdej macierzy z listy. Jej pętla:

```cpp
    for (const assets::ModelPart& part : model->parts) {
        // The normal map first: bind() makes its unit the active one, and binding the
        // colour picture last leaves unit 0 active, as the rest of the program expects.
        // Never null: a part without a normal map has the flat one of the cache.
        part.normalMap->bind(NORMAL_MAP_UNIT);
        part.texture->bind(TEXTURE_UNIT);
        shader.setVec3(TINT_UNIFORM, part.color);

        for (const glm::mat4& modelMatrix : modelMatrices) {
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
            // The lit programs turn the normals with a matrix of their own, derived
            // from the model matrix. It is computed here, on the CPU, once per object:
            // in the shader the inverse would be computed again for every vertex.
            shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
            model->mesh.draw(part.firstIndex, part.indexCount);
        }
    }
```

Trzecia funkcja, `game::drawMesh`, robi to samo dla siatki, która nie pochodzi z pliku modelu. Jedynym użytkownikiem jest teren:

```cpp
void drawMesh(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
              const gfx::Texture2D& normalMap, const glm::vec3& tint,
              const glm::mat4& modelMatrix) {
    // The same steps as for one part of a model and one object: the two textures on
    // their units (the colour picture last, see drawModel), the tint, the two matrices.
    normalMap.bind(NORMAL_MAP_UNIT);
    texture.bind(TEXTURE_UNIT);
    shader.setVec3(TINT_UNIFORM, tint);
    shader.setMat4(MODEL_UNIFORM, modelMatrix);
    shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
    mesh.draw();
}
```

To te same trzy uniformy co w `drawModel` (`uTint`, `uModel`, `uNormalMatrix`), ustawione raz, bez pętli: teren jest jedną siatką i jednym obiektem. `TerrainRenderer::draw` podaje jako `tint` biel (`NO_TINT`, czyli `(1, 1, 1)`: tekstura bez zmiany koloru), a jako `modelMatrix` macierz jednostkową (`IDENTITY`), bo wierzchołki terenu są już w przestrzeni świata. Macierz normalnych z macierzy jednostkowej też jest jednostkowa. Shader nie ma jednak osobnej ścieżki "bez macierzy", więc oba uniformy trzeba ustawić: inaczej zostałyby w nich wartości ostatniej ściany z poprzedniej klatki (uniform pamięta wartość, sekcja 2.1).

`drawModel` nie wie, którym z trzech programów rysuje, więc wysyła `uNormalMatrix` zawsze. Program `textured` takiego uniformu nie ma: położenie to -1, a wywołanie jest po cichu ignorowane (sekcja 2.3). To celowe użycie tej reguły, ale ma cenę: zbędne wyszukanie i zbędne odwrócenie macierzy dla każdego obiektu w trybie bez oświetlenia (sekcja 5.6). Z tego samego powodu `setModelSamplers` wysyła `uNormalMap` do programu `gouraud`, który go nie ma, co mówi wprost komentarz w `ModelDraw.hpp` ("a uniform a program does not have is ignored"). Pętle omawia [`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5.

**`uEmissive`: uniform pamięta wartość.** To najlepszy w projekcie żywy przykład trwałości uniformu (sekcja 2.1). Cała metoda `MazeRenderer::draw` (do M5 rysowała także płytki podłogi, od drugiej części M6 podłoże rysuje przed nią `TerrainRenderer::draw`, które samo ustawia `uEmissive` na czerń z komentarzem "Earth gives off no light of its own."):

```cpp
void MazeRenderer::draw(const gfx::Shader& shader, const MazeWorld& world) const {
    setModelSamplers(shader);
    // Stone gives off no light of its own. A uniform keeps its value from one draw call
    // to the next, and the crystals set this one, so it is set back in every frame.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    drawModel(shader, m_wall, world.wallMatrices);
    drawModel(shader, m_pillar, world.pillarMatrices);
}
```

A w `GameplayRenderer::draw`, które działa zaraz po niej tym samym programem, stoją dwa dalsze ustawienia tego samego uniformu:

```cpp
        // Wood gives off no light.
        shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});
```

przed narysowaniem bramy (tylko gdy brama jest jeszcze widoczna) oraz

```cpp
    // The crystals glow. One uniform for all of them: they pulse together.
    shader.setVec3(EMISSIVE_UNIFORM, crystalGlow);
```

przed pętlą kryształów. Kolejność w klatce jest więc taka: czarny, teren, czarny, ściany i słupki, czarny, brama, świecenie, kryształy. Po ostatnim krysztale uniform zostaje z wartością świecenia i **nikt go nie zeruje na koniec klatki**. Program jest ten sam w następnej klatce, a `glUseProgram` wartości nie rusza, więc gdyby nikt nie ustawiał czerni na początku klatki, teren i ściany następnej klatki byłyby rysowane ze świeceniem kryształów. Od drugiej części M6 pierwszym, kto to robi, jest `TerrainRenderer::draw`, bo teren jest rysowany przed ścianami. Linia w `MazeRenderer::draw` stała się przez to powtórzeniem (czerń ustawił już teren), ale zostaje z tego samego powodu co czerń przed bramą: żadna klasa rysująca nie zależy od tego, kto rysował przed nią. To właśnie mówi komentarz: uniform trzyma wartość od jednego wywołania rysującego do następnego, kryształy go ustawiają, więc trzeba go cofać w każdej klatce. Ustawienie czerni przed bramą jest z tego punktu widzenia powtórzeniem (czerń ustawił już `MazeRenderer`), ale dzięki niemu `GameplayRenderer::draw` nie zależy od tego, kto rysował przed nim.

**Linie kształtów kolizji**, `NightMazeApp::drawColliderLines`:

```cpp
    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);
```

`uColor` i `uModel` ustawiają `ColliderLines::draw` (pudełka) i `ColliderLines::drawSpheres` (kule): kolor raz na listę, macierz modelu dla każdego pudełka, a dla kuli trzy razy, bo kula to trzy okręgi z jednej siatki, każdy z inną macierzą ([`../scene/collision.md`](../scene/collision.md), sekcja 5). `drawColliderLines` woła je sześć razy z pięcioma kolorami: pudełka labiryntu, pudełko gracza, pudełko bramy (dopóki blokuje przejście), strefa wyjścia, kula zasięgu gracza i kule kryształów (gracz ma jeden kolor dla pudełka i dla kuli). To drugi przykład trwałości: każde wywołanie zaczyna od własnego `setVec3(COLOR_UNIFORM, color)`, bo inaczej rysowałoby kolorem poprzedniej listy.

Kto ustawia który uniform i jak często:

| Uniform | Program | Kto ustawia | Jak często |
|---|---|---|---|
| `uView`, `uProjection` | każdy używany w klatce | `drawUnlitMaze` albo `drawLitMaze`, `drawColliderLines` | raz na funkcję rysującą |
| `uViewMode` | `textured` | `NightMazeApp::drawUnlitMaze` | raz na klatkę |
| `uNormalMapEnabled` | `textured` albo `lit` (w `gouraud` bez skutku) | `drawUnlitMaze` albo `drawLitMaze` | raz na klatkę |
| `uSpecularModel`, `uSpecularStrength`, `uShininess` | `lit` albo `gouraud` | `NightMazeApp::drawLitMaze` | raz na klatkę |
| `uTexture` | program sceny (`textured`, `lit` albo `gouraud`) | `game::setModelSamplers`, wołane przez `TerrainRenderer::draw`, `MazeRenderer::draw` i `GameplayRenderer::draw` | trzy razy na klatkę |
| `uNormalMap` | program sceny (w `gouraud` bez skutku) | `game::setModelSamplers` | trzy razy na klatkę |
| `uEmissive` | program sceny | `TerrainRenderer::draw` (czerń), `MazeRenderer::draw` (czerń), `GameplayRenderer::draw` (czerń dla bramy, świecenie dla kryształów) | trzy albo cztery razy na klatkę |
| `uTint` | program sceny | `game::drawModel`, dla terenu `game::drawMesh` | raz na część modelu w każdym wywołaniu `drawModel`, raz dla terenu |
| `uModel` | program sceny | `game::drawModel`, dla terenu `game::drawMesh` | raz na rysowany obiekt |
| `uNormalMatrix` | program sceny (w `textured` bez skutku) | `game::drawModel`, dla terenu `game::drawMesh` | raz na rysowany obiekt |
| `uView`, `uProjection`, `uTime`, `uBladeHeight`, `uWindStrength`, `uLit`, `uViewMode`, `uSpecularStrength`, `uShininess` | `grass` | `GrassRenderer::draw` | raz na klatkę, gdy trawa jest włączona i ma kępki |
| `uColor` | `color` | `ColliderLines::draw`, `ColliderLines::drawSpheres` | raz na listę pudełek albo kul |
| `uModel` | `color` | `ColliderLines::draw` | raz na pudełko |
| `uModel` | `color` | `ColliderLines::drawSpheres` | trzy razy na kulę |

Wszystko jest wysyłane **co klatkę**, także wartości, które się nie zmieniają (macierze modelu ścian, numery jednostek teksturujących, parametry odblasku). Powody są trzy. Macierz rzutowania zależy od rozmiaru okna, który może się zmienić w każdej chwili. Kamera się rusza, więc macierz widoku jest inna w każdej klatce ruchu. A po `reload()` nowy program ma wszystkie uniformy wyzerowane (sekcja 2.1), więc wartości wysłane raz przy starcie przepadłyby po pierwszym naciśnięciu `Reload shaders`. Ten trzeci powód jest zapisany wprost w komentarzu nad `setModelSamplers` w [`ModelDraw.hpp`](../../../src/game/ModelDraw.hpp):

```cpp
/// Tells the two samplers of a program (uTexture and uNormalMap) which texture units to
/// read. shader must be in use. Call it in every frame before drawModel and not once at
/// start-up: after a shader reload all uniforms are back at 0, and both samplers would
/// read unit 0. The gouraud program has no uNormalMap: a uniform a program does not
/// have is ignored.
void setModelSamplers(const gfx::Shader& shader);
```

Dla samplera `uTexture` wyzerowanie akurat nie byłoby widoczne, bo jego jednostka ma numer 0. Dla `uNormalMap` byłoby: jego jednostka ma numer 1, więc z wartością 0 shader czytałby teksturę koloru jako mapę normalnych. Dla `uTint` też: wektor zerowy daje czarną scenę. Dla `uNormalMatrix` też: macierz zerowa daje normalną zerową i oświetlenie bez sensu. Dla `uEmissive` wyzerowanie znaczy czerń, czyli brak świecenia: po przeładowaniu kryształy byłyby ciemne aż do następnego ustawienia, a to następuje w tej samej klatce.

Wiązanie bloku uniformów jest jedynym stanem programu, którego gra **nie** wysyła co klatkę: klasa `Shader` zapamiętuje je i sama powtarza po przeładowaniu ([`uniform-buffers.md`](uniform-buffers.md), sekcja 5).

### 5.3 Jak to zostało sprawdzone

**Kostka i uniformy (stan z M1).** Ta tabela jest historią: kostkę demonstracyjną, jej funkcję `drawCube` i pliki `basic.vert` oraz `basic.frag` usunęło M5, więc prób nie da się dziś powtórzyć na tym samym kodzie. Zostawiam je, bo pokazują zmierzone skutki pomyłek, które działają tak samo w każdym programie. W M1, po zamianie trójkąta na kostkę, sprawdziłem ówczesny kod rysujący testem z ukrytym oknem, który wołał `NightMazeApp::onRender` i czytał obraz przez `glReadPixels`. Wyniki dotyczące shadera i uniformów:

| Próba | Wynik |
|---|---|
| ówczesne shadery kostki i trzy wywołania `setMat4` | środek okna czerwony (ściana przednia), róg w kolorze tła, w klatce dokładnie trzy kolory ścian, `glGetError` czysty |
| literówka w nazwie uniformu w C++ (`"uModle"` zamiast `"uModel"`, w kopii pliku poza repozytorium) | **pusta klatka**: każdy piksel w kolorze tła. `glGetError` czysty, w konsoli żadnej linii `[error]`, program działa dalej |
| `gl_Position = vec4(aPosition, 1.0);` (bez macierzy) | zielony prostokąt na środku, połowa szerokości i wysokości okna. Zielona jest ściana **tylna**: bez macierzy rzutowania mniejsze z znaczy "bliżej". Trzy uniformy stają się nieaktywne, błędu brak |
| `uModel * uView * uProjection` (odwrotna kolejność) | pusta klatka, błędu brak |
| `uView * uModel` (bez rzutowania) | pusta klatka: kostka ma w przestrzeni widoku z od -3,9 do -2,1, czyli poza zakresem od -1 do 1, i jest w całości przycinana |
| zamienione `location = 0` i `location = 1` | pusta klatka, błędu brak |

Program `night_maze` uruchomiony wtedy na około 3 sekundy z katalogu repozytorium wypisał dwie linie `[info]` i żadnej linii `[error]`.

**Trzy programy (stan z M2 + M3).** Na Windowsie (MSVC 19.44, 2026-10-05) gra z trzema programami startowała bez linii `[error]` i bez linii `GL_`, a labirynt z teksturami i żółte linie pudełek kolizji na ścianach i słupkach były widoczne na zrzutach ekranu. Powyższych prób z literówką i z kolejnością mnożenia nie powtarzałem dla nowych shaderów.

**Pięć programów (stan z M4).** Na Windowsie (2026-10-05, MSVC 19.44, NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74) gra z pięcioma programami (piątym był program kostki) budowała się w Debug i Release bez ostrzeżeń i startowała bez linii `[error]` i bez linii `GL_`. Wniosek z tego był węższy, niż wygląda. Makro `GL_CHECK` sprawdza błędy tylko w buildzie Debug (w Release jest samym wywołaniem), a sam start rysował w trybie domyślnym, czyli programem `lit`, programem `color` (wtedy rysował też kostki oznaczające światła) i programem kostki. Dla tych trzech programów w buildzie Debug było więc wiadomo, że żadne wywołanie `glUniform*` nie trafia w uniform innego typu: taki błąd `GL_CHECK` by wypisało. Dla programów `gouraud` i `textured` tego wniosku nie było: wiedziałem o nich tyle, ile widać na zrzutach. Na zrzutach ekranu sprawdzone były cztery tryby oświetlenia z trzech punktów widzenia, czyli obraz programów `textured`, `gouraud` i `lit` z ich uniformami. Prób z literówką nie powtarzałem dla nowych nazw.

**Dwa uniformy map normalnych (druga część M4).** Na tym samym sprzęcie i tego samego dnia: build Debug i Release bez ostrzeżeń, wszystkie ówczesne testy zielone w obu, start bez linii `[error]` i bez linii `GL_`. Start rysował programem `lit`, więc w buildzie Debug było wiadomo, że `setInt` na `uNormalMap` (sampler) i na `uNormalMapEnabled` (`bool`) nie zgłasza błędu typu. Na zrzutach ekranu relief był widoczny pod `Phong` i `Blinn-Phong`, a zrzuty w trybach `Gouraud` i `Unlit` były identyczne piksel w piksel przy mapowaniu włączonym i wyłączonym: ustawienia, które w programie `gouraud` trafiają w położenie -1, nie mają skutku ([`normal-mapping.md`](normal-mapping.md), sekcja 5).

**Cztery programy i `uEmissive` (M5).** M5 jest na Windowsie skończone w kodzie, ale nie zamknięte. Stan zgłoszony 2026-10-05 dla Windowsa: build Debug i Release bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach, a obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowe zaczepy w kodzie, które potem usunięto. Żaden z tych testów nie dotyka uniformów: testy nie tworzą kontekstu OpenGL, więc o setterach mówią tylko zrzuty. Czego nie wiem: nie mam zapisu, czy start w buildzie Debug był bez linii `GL_` (dla M5 tego nie zgłoszono), prób z literówką i z kolejnością mnożenia nikt nie powtarzał na shaderach, które dziś istnieją, a ręcznie nikt jeszcze nie grał (zbieranie kryształów, otwarta brama, przełączanie trybów listą `Lighting`, pole wyboru `Normal mapping`). **Na macOS nic z M5 nie było budowane ani uruchamiane.**

Wcześniejsze próby samej klasy `Shader` (kompilacja, linkowanie, przenoszenie) są w [`shader-class.md`](shader-class.md) (sekcja 5.10).

### 5.4 `setInt` i `setVec3`

Dwa settery dodane w kamieniu milowym M2 + M3 razem z klasą `gfx::Texture2D` ([`textures.md`](textures.md)). Gra woła je w dwunastu miejscach:

| Wywołanie | Gdzie | Uniform |
|---|---|---|
| `setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode))` | `NightMazeApp::drawUnlitMaze` | `int uViewMode` w `textured.frag`: 0, 1 albo 2 |
| `setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)))` | `NightMazeApp::drawLitMaze` | `int uSpecularModel` z `common/lighting.glsl`: 0 (Phong) albo 1 (Blinn-Phong) |
| `setInt(TEXTURE_UNIFORM, static_cast<int>(TEXTURE_UNIT))` | `game::setModelSamplers` | `sampler2D uTexture` w `textured.frag`, `lit.frag` albo `gouraud.frag`: numer jednostki teksturującej |
| `setInt(NORMAL_MAP_UNIFORM, static_cast<int>(NORMAL_MAP_UNIT))` | `game::setModelSamplers` | `sampler2D uNormalMap` z `common/normal_map.glsl` (w `lit.frag` i `textured.frag`): numer drugiej jednostki, 1 |
| `setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0)` | `NightMazeApp::drawUnlitMaze` i `NightMazeApp::drawLitMaze` (dwa miejsca) | `bool uNormalMapEnabled` z tego samego pliku: 1 włącza mapowanie normalnych, 0 wyłącza |
| `setVec3(TINT_UNIFORM, part.color)` | `game::drawModel` | `vec3 uTint` w tych samych trzech plikach: kolor materiału |
| `setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F})` | `TerrainRenderer::draw`, `MazeRenderer::draw` i `GameplayRenderer::draw`, przed bramą (trzy miejsca) | `vec3 uEmissive` w tych samych trzech plikach: czerń, czyli brak własnego świecenia |
| `setVec3(EMISSIVE_UNIFORM, crystalGlow)` | `GameplayRenderer::draw`, przed kryształami | ten sam uniform: świecenie kryształów |
| `setVec3(COLOR_UNIFORM, color)` | `ColliderLines::draw` i `ColliderLines::drawSpheres` (dwa miejsca) | `vec3 uColor` w `color.frag`: kolor linii |

Wszystkie rzutowania `static_cast<int>` mają ten sam powód: `setInt` przyjmuje `int`, a źródłem jest inny typ. `m_viewMode` to typ wyliczeniowy `game::ViewMode`, a `specularModelOf` zwraca `game::SpecularModel` (`enum class` nie zamienia się na liczbę sam). `TEXTURE_UNIT` i `NORMAL_MAP_UNIT` (stałe w `ModelDraw.cpp`) mają typ `GLuint`, bo taki przyjmuje `Texture2D::bind`.

**`bool` w GLSL a `setInt`.** OpenGL nie ma osobnej funkcji `glUniform` dla typu `bool`: uniform tego typu ustawia się funkcją dla liczb całkowitych albo zmiennoprzecinkowych, zero znaczy fałsz, każda inna wartość prawdę. Klasa `Shader` nie ma więc `setBool`, a kod pisze zamianę wprost: `usesNormalMap(m_lighting) ? 1 : 0`. Uniform, którego nikt nie ustawił, ma po linkowaniu wartość 0, czyli fałsz: świeżo zbudowany program zaczyna bez mapowania normalnych, co mówi też komentarz w `common/normal_map.glsl`.

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
| `setMat3` (sekcja 5.7) | `glUniformMatrix3fv(location, 1, GL_FALSE, wskaźnik)` | `mat3` |
| `setFloat` (sekcja 5.7) | `glUniform1f(location, value)` | `float` |
| `setFloatArray` (sekcja 5.8, druga część M7) | `glUniform1fv(location, liczba, wskaźnik)` | tablica `float nazwa[N]` |

| Element | Co robi i dlaczego |
|---|---|
| `int value` | zwykły `int` z C++. `GLint`, którego chce `glUniform1i`, to ten sam typ |
| `const glm::vec3& value` | referencja do stałej, jak macierz w `setMat4`. Przyjmuje też wartość tymczasową, na przykład `glm::vec3(1.0F, 0.0F, 0.0F)` |
| `1` w `glUniform3fv` | liczba wektorów, nie liczba składowych. Trójka jest w nazwie funkcji |
| `glm::value_ptr(value)` | wskaźnik na pierwszą z trzech liczb `float` wektora ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.9) |

**`setInt` a samplery.** To jest powód, dla którego `setInt` powstało razem z teksturami. Uniform typu `sampler2D` nie przechowuje identyfikatora tekstury, tylko **numer jednostki teksturującej** ([`textures.md`](textures.md), sekcja 2.7). `shader.setInt("uTexture", 0)` mówi samplerowi "czytaj z jednostki 0", a `texture.bind(0)` wiąże z tą jednostką teksturę. W GLSL 4.20 numer można wpisać w shaderze (`layout(binding = 0)`), ale projekt używa GLSL 4.10, najnowszej wersji na macOS, gdzie tego zapisu nie ma.

Sampler trzeba ustawiać dokładnie funkcją `glUniform1i`. Zmierzone w programie sprawdzającym z [`textures.md`](textures.md) (sekcja 5): `glUniform1f` i `glUniform1ui` na uniformie `sampler2D` dają `GL_INVALID_OPERATION`. To jedno z miejsc, w których OpenGL **sprawdza** zgodność typu uniformu z funkcją, i `GL_CHECK` taki błąd wypisze.

**Jak to zostało sprawdzone.** W tym samym programie (Windows, ukryte okno, shader z uniformami `sampler2D uTexture` i `vec3 uTint`): `setInt("uTexture", 2)` razem z `texture.bind(2)` daje na ekranie kolory tekstury, po `setVec3("uTint", (1, 0, 0))` zielony piksel tekstury staje się czarny (shader mnoży kolor przez `uTint`), a oba settery wołane z nazwą, której shader nie ma, nie zgłaszają błędu i nic nie zmieniają. W grze oba settery działały na zrzutach ekranu z Windowsa z M2 + M3: labirynt miał tekstury, oba tryby podglądu dawały inny obraz, a linie pudełek labiryntu były żółte ([`textures.md`](textures.md), sekcja 5). Na macOS tego nie uruchamiałem.

### 5.5 `ShaderUniforms.hpp`: nazwy w jednym miejscu

[`src/game/ShaderUniforms.hpp`](../../../src/game/ShaderUniforms.hpp) to plik poniżej, skrócony w jednym miejscu. W M1 trzy nazwy macierzy stały w anonimowej przestrzeni nazw `NightMazeApp.cpp`. Odkąd rysują także inne klasy, nazwy są w nagłówku, który dołącza dziś jedenaście plików: `NightMazeApp.cpp`, `ModelDraw.cpp`, `MazeRenderer.cpp`, `GameplayRenderer.cpp`, `ColliderLines.cpp`, `LightRig.cpp`, od M6 także `Skybox.cpp`, `TerrainRenderer.cpp` i `GrassRenderer.cpp`, od M7 `PostProcess.cpp`, a od czwartej części M7 `ShadowMap.cpp`.

```cpp
// Names of the uniform variables of the shaders in assets/shaders, in one place.
// See docs/modules/gfx/uniforms.md
#pragma once

#include <glad/gl.h>

namespace game {

// A uniform is found by its name (gfx::Shader::setMat4 and the other setters), so each
// string below must be spelled exactly like the "uniform" line of the shader file. A
// name with a typo is not an error: OpenGL silently ignores it. Keeping the names here,
// once, means that the classes that draw cannot disagree about them.

/// The three matrices. Every vertex shader (textured, color, lit, gouraud) declares
/// them under the same names. skybox.vert has the view and the projection only: the sky
/// is not placed anywhere in the world. The grass has the same two, in grass.geom: its
/// points are already in world space. shadow_depth.vert has all three, and there the
/// view and the projection are the ones of a light (scene::LightSpace).
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";

/// textured.frag, lit.frag and gouraud.frag: the sampler (it holds the number of
/// a texture unit) and the colour the texture is multiplied by.
constexpr const char* TEXTURE_UNIFORM = "uTexture";
constexpr const char* TINT_UNIFORM = "uTint";

/// textured.frag, lit.frag and gouraud.frag: the light a surface gives off by itself,
/// as a linear colour. Black for everything except the crystals.
constexpr const char* EMISSIVE_UNIFORM = "uEmissive";

/// common/normal_map.glsl, so lit.frag and textured.frag: the sampler of the normal map
/// (the number of a second texture unit) and the switch of normal mapping (1 on, 0 off).
constexpr const char* NORMAL_MAP_UNIFORM = "uNormalMap";
constexpr const char* NORMAL_MAP_ENABLED_UNIFORM = "uNormalMapEnabled";

/// textured.frag, skybox.frag and grass.frag: what to show (a value of game::ViewMode).
constexpr const char* VIEW_MODE_UNIFORM = "uViewMode";

/// lit.vert and gouraud.vert: the matrix that takes normals to world space
/// (scene::normalMatrix).
constexpr const char* NORMAL_MATRIX_UNIFORM = "uNormalMatrix";

/// common/lighting.glsl, so lit.frag, gouraud.vert and grass.frag: the highlight formula
/// (a value of game::SpecularModel), how bright the highlight is and its exponent.
constexpr const char* SPECULAR_MODEL_UNIFORM = "uSpecularModel";
constexpr const char* SPECULAR_STRENGTH_UNIFORM = "uSpecularStrength";
constexpr const char* SHININESS_UNIFORM = "uShininess";

/// common/lighting.glsl: the name of the uniform block with the lights, and the uniform
/// buffer binding point it is connected to. Every uniform block of a new program starts
/// at binding point 0. The lights use 1 on purpose: a program whose block was never
/// connected then reads from binding point 0, where no buffer is attached. The OpenGL
/// 4.1 specification leaves the values it gets undefined, so the mistake shows as wrong
/// lighting instead of the program working by accident.
constexpr const char* LIGHT_BLOCK_NAME = "LightBlock";
constexpr GLuint LIGHT_BLOCK_BINDING_POINT = 1;

/// common/shadows.glsl, so lit.frag, gouraud.frag and grass.frag: the names of the
/// uniforms of ONE shadow map. Every light that casts shadows has a set of its own in
/// that file, and a constant of this type here (game::setShadowUniforms takes it).
struct ShadowUniformNames {
    /// The sampler2DShadow of the map (it holds the number of a texture unit).
    const char* map;
    /// Whether the map is read (1) or nothing is in shadow (0).
    const char* enabled;
    /// World space to the clip space of the light (scene::LightSpace::matrix).
    const char* matrix;
    /// The two parts of the bias, as differences of stored depths.
    const char* constantBias;
    const char* slopeBias;
    /// The radius of the PCF kernel in texels. 0: one comparison.
    const char* pcfRadius;
    /// The share of the light a shadow takes away, 0 to 1.
    const char* strength;
};

/// The shadow map of the moon.
constexpr ShadowUniformNames MOON_SHADOW_UNIFORMS{
    .map = "uMoonShadowMap",
    .enabled = "uMoonShadowEnabled",
    .matrix = "uMoonShadowMatrix",
    .constantBias = "uMoonShadowConstantBias",
    .slopeBias = "uMoonShadowSlopeBias",
    .pcfRadius = "uMoonShadowPcfRadius",
    .strength = "uMoonShadowStrength",
};

/// The texture unit of the shadow map of the moon. The models use units 0 (colour
/// picture) and 1 (normal map) in the lit programs, and the composite pass uses 0 to 2,
/// so 3 is the first unit nothing else binds: the map is bound once per frame and stays
/// there while everything lit is drawn. A second shadow map takes the next unit.
constexpr GLuint MOON_SHADOW_TEXTURE_UNIT = 3;

/// color.frag: the one colour of everything drawn, a linear colour.
constexpr const char* COLOR_UNIFORM = "uColor";

/// skybox.frag: the sampler of the cube map (it holds the number of a texture unit) and
/// the number the colour of the sky is multiplied by.
constexpr const char* SKYBOX_UNIFORM = "uSkybox";
constexpr const char* SKYBOX_BRIGHTNESS_UNIFORM = "uBrightness";

/// grass.geom: the clock of the wind in seconds, the height of the tallest blades in
/// metres and how far the wind pushes the tips (0: still air).
constexpr const char* GRASS_TIME_UNIFORM = "uTime";
constexpr const char* GRASS_BLADE_HEIGHT_UNIFORM = "uBladeHeight";
constexpr const char* GRASS_WIND_STRENGTH_UNIFORM = "uWindStrength";

/// grass.frag: whether the grass is lit by the lights of the scene (1) or shown at full
/// brightness (0).
constexpr const char* GRASS_LIT_UNIFORM = "uLit";

} // namespace game
```

(Między `COLOR_UNIFORM` a komentarzem `skybox.frag` listing pomija dwadzieścia siedem stałych przebiegów po scenie, o przedrostkach `COMPOSITE_`, `BRIGHT_`, `BLUR_` i `PREVIEW_`. Reszta jest przepisana z pliku bez zmian.)

Plik miał po części piątej **czterdzieści siedem stałych z nazwami zwykłych uniformów** (szósta część M7 dodała trzy kolejne: `MINIMAP_MAP_TO_CLIP_UNIFORM`, `MINIMAP_OVERLAY_MAP_UNIFORM` i `MINIMAP_OVERLAY_OPACITY_UNIFORM`), dwie stałe bloku uniformów (`LIGHT_BLOCK_NAME` i `LIGHT_BLOCK_BINDING_POINT`) i, od czwartej części M7, trzy rzeczy dla mapy cieni: strukturę `ShadowUniformNames` z siedmioma polami `const char*`, jedną stałą tego typu, `MOON_SHADOW_UNIFORMS`, która trzyma siedem nazw uniformów mapy cieni księżyca, oraz numer jednostki teksturującej `MOON_SHADOW_TEXTURE_UNIT = 3`. Listing wyżej pokazuje początek pliku razem z częścią o cieniach i kończy się na stałych trawy: dwudziestu siedmiu stałych z pierwszych trzech części M7, które w pliku stoją między `COLOR_UNIFORM` a `SKYBOX_UNIFORM`, nie pokazuje. Historia liczby stałych z nazwami: dwadzieścia po M6 (`SKYBOX_UNIFORM` i `SKYBOX_BRIGHTNESS_UNIFORM` doszły w pierwszej części M6, a `GRASS_TIME_UNIFORM`, `GRASS_BLADE_HEIGHT_UNIFORM`, `GRASS_WIND_STRENGTH_UNIFORM` i `GRASS_LIT_UNIFORM` w drugiej), dwadzieścia osiem po pierwszej części M7, trzydzieści sześć po drugiej, czterdzieści siedem po trzeciej. Pierwsza część M7 dodała osiem, dla dwóch programów przebiegów końcowych: `COMPOSITE_SCENE_UNIFORM` (`"uScene"`), `COMPOSITE_EXPOSURE_UNIFORM` (`"uExposure"`), `COMPOSITE_TONE_MAPPING_UNIFORM` (`"uToneMapping"`), `PREVIEW_SOURCE_UNIFORM` (`"uSource"`), `PREVIEW_MODE_UNIFORM` (`"uMode"`), `PREVIEW_NEAR_UNIFORM` (`"uNear"`), `PREVIEW_FAR_UNIFORM` (`"uFar"`) i `PREVIEW_DEPTH_RANGE_UNIFORM` (`"uDepthRange"`). Druga część dodała osiem, z bloomem: `COMPOSITE_BLOOM_UNIFORM` (`"uBloom"`), `COMPOSITE_BLOOM_ENABLED_UNIFORM` (`"uBloomEnabled"`), `COMPOSITE_BLOOM_INTENSITY_UNIFORM` (`"uBloomIntensity"`), `BRIGHT_SCENE_UNIFORM` (`"uScene"`), `BRIGHT_THRESHOLD_UNIFORM` (`"uThreshold"`), `BLUR_SOURCE_UNIFORM` (`"uSource"`), `BLUR_HORIZONTAL_UNIFORM` (`"uHorizontal"`) i `BLUR_WEIGHTS_UNIFORM` (`"uWeights"`). Trzecia część dodała jedenaście, wszystkie dla programu `composite`: sześć dla mgły (`COMPOSITE_FOG_ENABLED_UNIFORM`, `COMPOSITE_DEPTH_UNIFORM`, `COMPOSITE_FOG_DENSITY_UNIFORM`, `COMPOSITE_FOG_BASE_HEIGHT_UNIFORM`, `COMPOSITE_FOG_HEIGHT_FALLOFF_UNIFORM`, `COMPOSITE_FOG_COLOR_UNIFORM`), dwie dla odtworzenia pozycji z głębi (`COMPOSITE_INVERSE_VIEW_PROJECTION_UNIFORM`, `COMPOSITE_EYE_UNIFORM`) i trzy dla winiety (`COMPOSITE_VIGNETTE_ENABLED_UNIFORM`, `COMPOSITE_VIGNETTE_STRENGTH_UNIFORM`, `COMPOSITE_VIGNETTE_RADIUS_UNIFORM`). Dwie stałe powtarzają napis, który plik już ma (`"uScene"` jest w `COMPOSITE_SCENE_UNIFORM` i w `BRIGHT_SCENE_UNIFORM`, `"uSource"` w `PREVIEW_SOURCE_UNIFORM` i w `BLUR_SOURCE_UNIFORM`): stałych z nazwami jest więc czterdzieści siedem, a różnych nazw czterdzieści pięć. Osobna stała dla każdego programu jest celowa: uniform należy do programu, a zmiana nazwy w jednym shaderze nie powinna ruszać drugiego. Wszystkie dwadzieścia siedem stałych z tych trzech części ustawia `game::PostProcess` ([`../renderer/post-process.md`](../renderer/post-process.md)), a tabela niżej ich nie powtarza. Czwarta część M7 poszła inną drogą: zamiast siedmiu kolejnych stałych jest **struktura** z siedmioma nazwami. Powód stoi w komentarzu nad nią: każde światło, które rzuca cień, ma w `common/shadows.glsl` własny zestaw tych siedmiu uniformów, więc druga mapa cieni to druga stała typu `ShadowUniformNames`, a funkcja `game::setShadowUniforms` przyjmuje taką stałą jako parametr i nie zna żadnej nazwy na sztywno. Dziś stała jest jedna (księżyc). Razem z jej siedmioma nazwami plik zna pięćdziesiąt dwie różne nazwy zwykłych uniformów.

| Stała | Wartość | Który plik shadera to deklaruje | Kto jej używa |
|---|---|---|---|
| `MODEL_UNIFORM` | `"uModel"` | cztery shadery wierzchołków: `textured`, `color`, `lit`, `gouraud`, a od czwartej części M7 piąty, `shadow_depth.vert` | `game::drawModel`, `game::drawMesh`, `ColliderLines::draw`, `ColliderLines::drawSpheres` |
| `VIEW_UNIFORM` | `"uView"` | te same pięć, do tego `skybox.vert` i `grass.geom` | `drawUnlitMaze`, `drawLitMaze`, `drawColliderLines` i, od czwartej części M7, `drawShadowCasters` (z macierzą światła) w `NightMazeApp`, `Skybox::draw`, `GrassRenderer::draw` |
| `PROJECTION_UNIFORM` | `"uProjection"` | te same siedem plików (od M8, części 1 także ósmy, `reflect.vert`) | te same sześć funkcji (od M8, części 1 także `drawReflections`) |
| `TEXTURE_UNIFORM` | `"uTexture"` | `textured.frag`, `lit.frag`, `gouraud.frag` | `game::setModelSamplers` |
| `TINT_UNIFORM` | `"uTint"` | te same trzy | `game::drawModel`, `game::drawMesh` |
| `EMISSIVE_UNIFORM` | `"uEmissive"` | te same trzy | `TerrainRenderer::draw`, `MazeRenderer::draw`, `GameplayRenderer::draw` |
| `NORMAL_MAP_UNIFORM` | `"uNormalMap"` | `common/normal_map.glsl` (czyli `lit.frag` i `textured.frag`) | `game::setModelSamplers` |
| `NORMAL_MAP_ENABLED_UNIFORM` | `"uNormalMapEnabled"` | `common/normal_map.glsl` | `NightMazeApp::drawUnlitMaze`, `NightMazeApp::drawLitMaze` |
| `VIEW_MODE_UNIFORM` | `"uViewMode"` | `textured.frag`, `skybox.frag`, `grass.frag` | `NightMazeApp::drawUnlitMaze`, `Skybox::draw`, `GrassRenderer::draw` |
| `NORMAL_MATRIX_UNIFORM` | `"uNormalMatrix"` | `lit.vert`, `gouraud.vert` | `game::drawModel`, `game::drawMesh` |
| `SPECULAR_MODEL_UNIFORM` | `"uSpecularModel"` | `common/lighting.glsl` (czyli `lit.frag`, `gouraud.vert` i `grass.frag`) | `NightMazeApp::drawLitMaze` |
| `SPECULAR_STRENGTH_UNIFORM` | `"uSpecularStrength"` | `common/lighting.glsl` | `NightMazeApp::drawLitMaze`, `GrassRenderer::draw` (zawsze 0) |
| `SHININESS_UNIFORM` | `"uShininess"` | `common/lighting.glsl` | `NightMazeApp::drawLitMaze`, `GrassRenderer::draw` (zawsze 1) |
| `COLOR_UNIFORM` | `"uColor"` | `color.frag` | `ColliderLines::draw`, `ColliderLines::drawSpheres` |
| `SKYBOX_UNIFORM` | `"uSkybox"` | `skybox.frag` | `Skybox::draw` |
| `SKYBOX_BRIGHTNESS_UNIFORM` | `"uBrightness"` | `skybox.frag` | `Skybox::draw` |
| `GRASS_TIME_UNIFORM` | `"uTime"` | `grass.geom` | `GrassRenderer::draw` |
| `GRASS_BLADE_HEIGHT_UNIFORM` | `"uBladeHeight"` | `grass.geom` | `GrassRenderer::draw` |
| `GRASS_WIND_STRENGTH_UNIFORM` | `"uWindStrength"` | `grass.geom` | `GrassRenderer::draw` |
| `GRASS_LIT_UNIFORM` | `"uLit"` | `grass.frag` | `GrassRenderer::draw` |
| `LIGHT_BLOCK_NAME` | `"LightBlock"` | `common/lighting.glsl`: nazwa **bloku**, nie uniformu | `LightRig::connect`, przez `Shader::bindUniformBlock` |
| `LIGHT_BLOCK_BINDING_POINT` | `1` (typ `GLuint`) | nigdzie: GLSL 4.10 nie umie zapisać tego numeru | konstruktor `LightRig`, jako punkt wiązania bufora |
| `MOON_SHADOW_UNIFORMS` (czwarta część M7) | struktura `ShadowUniformNames` z siedmioma napisami: `"uMoonShadowMap"`, `"uMoonShadowEnabled"`, `"uMoonShadowMatrix"`, `"uMoonShadowConstantBias"`, `"uMoonShadowSlopeBias"`, `"uMoonShadowPcfRadius"`, `"uMoonShadowStrength"` | `common/shadows.glsl` (czyli `lit.frag`, `gouraud.frag` i `grass.frag`) | `NightMazeApp::drawLitMaze` i `NightMazeApp::drawGrass`, jako argument `game::setShadowUniforms` |
| `MOON_SHADOW_TEXTURE_UNIT` (czwarta część M7) | `3` (typ `GLuint`) | nigdzie: to numer jednostki, który trafia do samplera `uMoonShadowMap` | te same dwie funkcje i `NightMazeApp::drawMoonShadowMap`, jako argument `ShadowMap::bindForSampling` |

| Element | Znaczenie |
|---|---|
| `#pragma once` | nagłówek jest dołączany przez jedenaście plików `.cpp`. W jednej jednostce kompilacji ma być wczytany raz |
| `#include <glad/gl.h>` | jedyne dołączenie, potrzebne dla typu `GLuint` stałej `LIGHT_BLOCK_BINDING_POINT`. Do M4 plik nie dołączał niczego |
| `namespace game` | plik leży w `src/game/`, bo nazwy należą do shaderów tej gry, a nie do klasy `gfx::Shader`, która przyjmuje dowolny napis. Należy do programu `night_maze` (lista źródeł w [`CMakeLists.txt`](../../../CMakeLists.txt)), nie do biblioteki `engine` |
| `constexpr const char*` | stała znana w czasie kompilacji, wskazująca na literał napisu. To dokładnie typ parametru `name` setterów. `constexpr` w przestrzeni nazw daje stałej wiązanie wewnętrzne, więc definicja w nagłówku dołączanym wiele razy nie powoduje błędu linkowania |
| `constexpr GLuint LIGHT_BLOCK_BINDING_POINT = 1;` | do trzeciej części M7 jedyna stała, która nie jest napisem (od czwartej drugą jest `MOON_SHADOW_TEXTURE_UNIT`): numer punktu wiązania bufora uniformów. Typ `GLuint`, bo taki przyjmują `glBindBufferBase` i `glUniformBlockBinding` |
| `struct ShadowUniformNames` i `constexpr ShadowUniformNames MOON_SHADOW_UNIFORMS{ .map = ..., ... }` (czwarta część M7) | pierwsza struktura w tym pliku. Zbiera nazwy uniformów jednej mapy cieni, żeby funkcja ustawiająca je przyjmowała jeden parametr zamiast siedmiu napisów. Zapis z kropkami to inicjalizatory desygnowane z C++20: każda nazwa stoi przy swoim polu, więc nie da się ich zamienić miejscami przez pomyłkę w kolejności |
| sam nagłówek, bez pliku `.cpp` | plik nie zawiera kodu, tylko stałe i jedną strukturę z danymi: czterdzieści siedem nazw uniformów, dwie stałe bloku i trzy rzeczy mapy cieni (akapit pod listingiem) |

**Komentarz nad `LIGHT_BLOCK_NAME`.** Program, którego bloku nikt nie podłączył, czyta z punktu wiązania 0, a tam żaden bufor nie jest podpięty. Specyfikacja OpenGL 4.1 zostawia wartości, które wtedy dostaje shader, nieokreślone: nie ma gwarancji ani zer, ani "braku świateł". Wybór punktu 1 sprawia, że taka pomyłka pokazuje się jako błędne oświetlenie, zamiast działać przypadkiem ([`uniform-buffers.md`](uniform-buffers.md), sekcja 7, pułapka 3). Wcześniejsza wersja komentarza mówiła "reads no lights at all", co było nieścisłe, i została poprawiona.

**Dlaczego jeden nagłówek.** Napis z nazwą jest jedynym łącznikiem między C++ a linią `uniform ...` w shaderze. Kompilator C++ nie wie nic o shaderze, a OpenGL ignoruje nieznaną nazwę po cichu (pułapka 1). Gdyby każda klasa rysująca miała własne literały, literówka w jednej z nich dałaby pusty ekran dla jednego rodzaju obiektów, bez żadnego komunikatu. Jeden nagłówek nie chroni przed literówką w samej stałej ani przed zmianą nazwy tylko w pliku shadera. Sprawia tylko, że pomyłkę robi się i poprawia w jednym miejscu.

Czego w nagłówku nie ma: numerów jednostek teksturujących modeli i przebiegów po scenie (`TEXTURE_UNIT` i `NORMAL_MAP_UNIT` to prywatne stałe `ModelDraw.cpp`, a trzy jednostki przebiegu składającego prywatne stałe `PostProcess.cpp`. Wyjątkiem jest od czwartej części M7 `MOON_SHADOW_TEXTURE_UNIT`: stoi w nagłówku, bo numer musi znać i kod, który wiąże mapę, i każde miejsce, które ustawia sampler), wartości trybu podglądu (typ `game::ViewMode` jest w `MazeRenderer.hpp`), wartości wzoru odblasku (typ `game::SpecularModel` jest w `Lighting.hpp`) ani nazw pól bloku `LightBlock`. Tych ostatnich C++ w ogóle nie zna z nazwy: zna tylko ich miejsce w bajtach bufora.

### 5.6 Ile wyszukiwań na klatkę

Każdy setter woła `glGetUniformLocation` przy każdym wywołaniu (sekcja 5.1). W M1 były to trzy wyszukiwania na klatkę. Dziś liczba wynika z liczby rysowanych obiektów. Labirynt domyślny (10 x 10 komórek, ziarno 1) to teren (jedna siatka), 121 ścian i 121 słupków, a runda zaczyna się w nim z 13 kryształami i zamkniętą bramą. Każdy obiekt jest rysowany osobnym wywołaniem z własnym `uModel` i własnym `uNormalMatrix`. Do M5 w miejscu terenu było 100 płytek podłogi, każda z własnymi dwiema macierzami: zastąpienie ich jedną siatką zmniejszyło sumę z 742 do 547. Liczby poniżej są **policzone z kodu** dla początku rundy, a nie zmierzone licznikiem w działającym programie.

Tryb domyślny (`BlinnPhong`, program `lit`):

| Skąd | Ustawień uniformów na klatkę |
|---|---|
| `drawLitMaze`: `uView`, `uProjection`, `uSpecularModel`, `uSpecularStrength`, `uShininess`, `uNormalMapEnabled` | 6 |
| `TerrainRenderer::draw`: `uTexture`, `uNormalMap`, `uEmissive`, a w `drawMesh` `uTint`, `uModel`, `uNormalMatrix` | 6 |
| `MazeRenderer::draw`: `uTexture`, `uNormalMap`, `uEmissive` | 3 |
| `uTint` labiryntu | 2 (dwa modele, każdy ma jedną część) |
| `uModel` labiryntu | 121 + 121 = **242** |
| `uNormalMatrix` labiryntu | **242** |
| `GameplayRenderer::draw`: `uTexture`, `uNormalMap` | 2 |
| brama: `uEmissive`, `uTint`, `uModel`, `uNormalMatrix` | 4 |
| kryształy: `uEmissive` raz, a potem `uTint`, `uModel` i `uNormalMatrix` dla każdego z 13 | 1 + 39 = 40 |
| razem program sceny, bez linii kształtów kolizji, stan do trzeciej części M7 | **547** |
| od czwartej części M7: siedem uniformów mapy cieni w `drawLitMaze` (`game::setShadowUniforms`) | 7, razem **554** |
| od czwartej części M7, przebieg głębi mapy cieni programem `shadow_depth`, gdy cienie są włączone: `uView` i `uProjection` w `drawShadowCasters` i te same wywołania trzech rendererów co wyżej (6 + 489 + 46) | 2 + 541 = **543**, z czego skutek mają 259 (rozpisane niżej) |
| trawa, gdy jest włączona: dziewięć ustawień w `GrassRenderer::draw`, w osobnym programie `grass`, a od czwartej części M7 siedem uniformów mapy cieni w `drawGrass` | 9 + 7 = 16 |
| niebo, gdy jest włączone: pięć ustawień w `Skybox::draw`, w osobnym programie `skybox` | 5 |
| linie kształtów kolizji, gdy są włączone | 295 (rozpisane niżej) |

Przebieg głębi (czwarta część M7) używa `TerrainRenderer::draw`, `MazeRenderer::draw` i `GameplayRenderer::draw` bez zmian, więc wykonuje te same 541 wywołań setterów co program sceny bez sześciu z `drawLitMaze` (6 terenu, 489 labiryntu, 46 bramy i kryształów). Program `shadow_depth` ma tylko trzy uniformy, więc skutek ma 257 ustawień `uModel` (1 terenu, 242 labiryntu, 1 bramy, 13 kryształów) i dwa z `drawShadowCasters`: razem 259. Pozostałe 284 trafiają w położenie -1, a każde i tak kosztuje wyszukanie po nazwie, a 257 z nich także policzenie macierzy normalnych na procesorze. W trybie domyślnym z cieniami scena i przebieg głębi to razem 554 + 543 = 1097 wywołań setterów na klatkę, policzonych z kodu.

Linie, gdy są włączone: `uView` i `uProjection` (2), pudełka labiryntu (`uColor` i 242 razy `uModel`, czyli 243), pudełko gracza (2), pudełko bramy (2), strefa wyjścia (2), kula zasięgu gracza (`uColor` i trzy razy `uModel`, czyli 4), kule 13 kryształów (`uColor` i 39 razy `uModel`, czyli 40). Razem 295.

Pozostałe tryby:

| Tryb | Program sceny | Razem bez linii | Skąd różnica |
|---|---|---|---|
| `Gouraud`, `Phong`, `BlinnPhong` | `gouraud` albo `lit` | 554 (547 do trzeciej części M7) | te same wywołania setterów pod tymi samymi nazwami, razem z siedmioma uniformami mapy cieni, które mają oba programy. W programie `gouraud` cztery z nich (`uNormalMap` trzy razy i `uNormalMapEnabled` raz) trafiają w położenie -1 |
| `Unlit` albo widok diagnostyczny (`Normals`, `UVs`) w dowolnym trybie | `textured` | 545 | zamiast trzech uniformów odblasku jest `uViewMode`, więc funkcja `drawUnlitMaze` ustawia 4 uniformy, a nie 6. Program `textured` nie dołącza `common/shadows.glsl`, więc uniformów mapy cieni nie dostaje i cieni w tym trybie nie ma. Przebieg głębi (543 wywołania) jest mimo to wykonywany, dopóki cienie są włączone w panelu: `drawMoonShadowMap` nie patrzy na tryb oświetlenia |

Liczby zmieniają się w trakcie rundy. Każdy zebrany kryształ to trzy ustawienia mniej (i trzy mniej w liniach), a brama, która zapadła się do końca, przestaje być rysowana i zabiera cztery. Kryształy są przy tym rysowane pojedynczo: jedno wywołanie `drawModel` na kryształ, więc `uTint` jest dla nich ustawiany 13 razy, choć dwa modele kryształów mają razem dwie części.

Dwie rzeczy, które z tych tabel wynikają. Po pierwsze, M4 **podwoiło** liczbę ustawień: macierz normalnych idzie w parze z macierzą modelu. Mapy normalnych dołożyły do tego tylko sampler i przełącznik oraz jedno dodatkowe wiązanie tekstury na część modelu, a M5 dołożyło 46 ustawień dla bramy i kryształów i jedno `uEmissive` dla labiryntu. Po drugie, w programie `textured` 257 ustawień `uNormalMatrix` (1 terenu, 242 labiryntu, 1 bramy i 13 kryształów) trafia w położenie -1 i nie ma żadnego skutku, a mimo to każde kosztuje wyszukanie po nazwie i policzenie macierzy odwrotnej na procesorze (sekcja 5.2).

Każde ustawienie to dwa wywołania sterownika: wyszukanie po nazwie (porównywanie napisów) i samo `glUniform*`. **Czasu tego nie mierzyłem** i nie podaję żadnej liczby milisekund. Komentarz w `Shader::setMat4` ("a few hundred lookups per frame ... are still cheap") jest oceną, a nie wynikiem pomiaru, i powstał, gdy ustawień było o połowę mniej. Gdyby pomiar pokazał, że wyszukiwanie ma znaczenie, są trzy wyjścia: zapamiętać położenia `uModel` i `uNormalMatrix` raz na klatkę poza pętlą obiektów, dodać do klasy mapę położeń czyszczoną w `reload()` albo policzyć macierze normalnych labiryntu raz, razem z macierzami modelu, zamiast w każdej klatce. Większy zysk dałoby zmniejszenie samej liczby wywołań rysujących ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5).

Światła nie dokładają do tych liczb nic: 58 wartości bloku `LightBlock` to jedna wysyłka bufora na klatkę, bez żadnego wyszukiwania po nazwie ([`uniform-buffers.md`](uniform-buffers.md), sekcja 5).

### 5.7 `setMat3` i `setFloat`

Dwa settery dodane w M4 razem z oświetleniem. Gra woła je w trzech miejscach:

| Wywołanie | Gdzie | Uniform |
|---|---|---|
| `setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix))` | `game::drawModel` | `mat3 uNormalMatrix` w `lit.vert` i `gouraud.vert`: macierz dla normalnych |
| `setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength)` | `NightMazeApp::drawLitMaze` | `float uSpecularStrength`: jasność odblasku, domyślnie 0,25 |
| `setFloat(SHININESS_UNIFORM, m_lighting.shininess)` | `NightMazeApp::drawLitMaze` | `float uShininess`: wykładnik odblasku, domyślnie 32 |

Deklaracje w [`Shader.hpp`](../../../src/gfx/Shader.hpp):

```cpp
/// Sets the uniform variable of type mat3 called name to matrix (glUniformMatrix3fv):
/// the normal matrix. The rules of setMat4 apply: use() first, and an unknown name is
/// ignored.
void setMat3(const char* name, const glm::mat3& matrix) const;

/// Sets the uniform variable of type float called name to value (glUniform1f). The
/// rules of setMat4 apply: use() first, and an unknown name is ignored.
void setFloat(const char* name, float value) const;
```

Implementacja w [`Shader.cpp`](../../../src/gfx/Shader.cpp):

```cpp
void Shader::setMat3(const char* name, const glm::mat3& matrix) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // The 3 x 3 version of the call in setMat4: one matrix, not transposed, 9 floats.
    GL_CHECK(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setFloat(const char* name, float value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1f: one value of type float. OpenGL ignores location -1 without an error.
    GL_CHECK(glUniform1f(location, value));
}
```

`setMat3`, linia po linii:

| Linia | Co robi i dlaczego |
|---|---|
| `const char* name, const glm::mat3& matrix` | nazwa jak w shaderze i referencja do stałej macierzy 3 x 3 (36 bajtów). Przyjmuje wartość tymczasową: w grze argumentem jest wynik `scene::normalMatrix(modelMatrix)` |
| `const` na końcu | jak w `setMat4`: zmienia stan programu po stronie OpenGL, a nie pola obiektu C++ |
| `GLint location = -1;` | położenie ze znakiem, -1 znaczy "nie ma" |
| `GL_CHECK(location = glGetUniformLocation(m_program, name));` | to samo wyszukanie co w `setMat4`, bez pamięci podręcznej. Dla programu `textured` wynik to -1, bo ten program nie ma `uNormalMatrix` |
| `glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix))` | `Matrix3` w nazwie: macierz 3 x 3. `f`: liczby `float`. `v`: przez wskaźnik. `1`: jedna macierz. `GL_FALSE`: bez transpozycji, bo GLM trzyma macierz kolumnami tak jak OpenGL. `glm::value_ptr` daje wskaźnik na pierwszą z **dziewięciu** liczb |
| położenie -1 | ignorowane bez błędu, jak w każdej funkcji `glUniform*`. Komentarz w kodzie o tym nie wspomina, ale na tym właśnie polega wysyłanie `uNormalMatrix` do programu `textured` |

`setFloat`, linia po linii:

| Linia | Co robi i dlaczego |
|---|---|
| `float value` | zwykły `float` z C++, przez wartość. `GLfloat`, którego chce `glUniform1f`, to ten sam typ |
| wyszukanie położenia | identyczne jak w pozostałych setterach |
| `glUniform1f(location, value)` | `1`: jedna składowa. `f`: `float`. Bez `v`, więc wartość jest argumentem, a nie wskaźnikiem |

**`setFloat` to nie `setInt`.** Typ uniformu i funkcja muszą do siebie pasować. `glUniform1f` na uniformie typu `int` (na przykład na `uSpecularModel`) i `glUniform1i` na uniformie typu `float` (na przykład na `uShininess`) to według opisu `glUniform` błąd `GL_INVALID_OPERATION`, a wartość się nie zmienia. `GL_CHECK` taki błąd wypisze. Dla samplera zmierzyłem to wcześniej (sekcja 5.4). Dla `int` i `float` tego nie mierzyłem. Wiem tylko, że gra w buildzie Debug startuje w trybie domyślnym bez żadnej linii `GL_`, czyli że w programie `lit` każdy z nowych uniformów dostaje właściwą funkcję (sekcja 5.3). Program `gouraud` idzie przez ten sam kod w `drawLitMaze` i `drawModel`, z tymi samymi typami w GLSL. Te obserwacje pochodzą z M4 (sekcja 5.3).

**`mat3` jako zwykły uniform a `mat3` w bloku.** `setMat3` wysyła dziewięć liczb leżących ciasno jedna za drugą: tak trzyma je `glm::mat3` i tak przyjmuje je `glUniformMatrix3fv`. W bloku uniformów o układzie `std140` ta sama macierz zajmowałaby 48 bajtów, bo każda kolumna jest tam wyrównana do 16 ([`uniform-buffers.md`](uniform-buffers.md), sekcja 2.5). To jeden z powodów, dla których `uNormalMatrix` jest zwykłym uniformem. Drugi jest prostszy: zmienia się dla każdego obiektu, a blok jest wysyłany raz na klatkę.

**Skąd bierze się macierz.** `scene::normalMatrix(modelMatrix)` zwraca `transpose(inverse(mat3(modelMatrix)))`. Żaden obiekt sceny nie jest skalowany (ściany, brama i kryształy mają tylko przesunięcie i obrót), więc dziś wynik jest równy samej części obrotowej macierzy modelu i obraz byłby taki sam z `mat3(uModel)`. Funkcja jest wołana dla każdego obiektu w każdej klatce, choć macierze modelu labiryntu się nie zmieniają: wyniku nikt nie zapamiętuje. Dla kryształów liczenie co klatkę jest potrzebne, bo ich macierz modelu zmienia się w czasie (obrót i kołysanie). Kod funkcji i jej testy opisuje [`../scene/transforms.md`](../scene/transforms.md).

**Jak to zostało sprawdzone.** W M4 na Windowsie (2026-10-05): build bez ostrzeżeń, start bez linii `[error]` i `GL_`, a na zrzutach ekranu ściany zwrócone do księżyca były jaśniejsze od odwróconych, czyli normalne po przejściu przez `uNormalMatrix` wskazywały właściwe strony. Zmiany `Strength` i `Shininess` suwakami panelu Lights nikt jeszcze nie robił ręcznie. Na macOS nie sprawdzono niczego. Osobnego programu sprawdzającego, jak dla `setInt` i `setVec3`, dla tych dwóch setterów nie pisałem.

### 5.8 `setFloatArray`: tablica uniformów

Szósty setter, dodany w drugiej części M7 dla bloomu. Shader `post/blur.frag` deklaruje `uniform float uWeights[BLUR_RADIUS + 1];`, czyli tablicę siedmiu liczb: wagi rozmycia Gaussa, które liczy C++ ([`../renderer/post-process.md`](../renderer/post-process.md), sekcje 2.13 i 5.10). To pierwsza i jedyna tablica wśród zwykłych uniformów gry. Tablice świateł są w bloku uniformów i idą inną drogą (sekcja 2.4).

Deklaracja w [`src/gfx/Shader.hpp`](../../../src/gfx/Shader.hpp):

```cpp
/// Sets the uniform array of floats called name, declared in the shader as
/// "uniform float name[N]", to values (glUniform1fv): the first value goes to
/// element 0. Pass as many values as the array has elements: with fewer the rest
/// of the array keeps its old values, and OpenGL ignores the ones past its end.
/// The rules of setMat4 apply: use() first, and an unknown name is ignored.
void setFloatArray(const char* name, std::span<const float> values) const;
```

Implementacja w [`src/gfx/Shader.cpp`](../../../src/gfx/Shader.cpp):

```cpp
void Shader::setFloatArray(const char* name, std::span<const float> values) const {
    // Same lookup as in setMat4. The name of an array without an index gives the
    // location of its element 0.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1fv: count values of type float, read from the pointer and written to
    // the elements of the array, starting with the one at location.
    GL_CHECK(glUniform1fv(location, static_cast<GLsizei>(values.size()), values.data()));
}
```

| Linia | Znaczenie |
|---|---|
| `std::span<const float> values` | "widok" na ciąg liczb: wskaźnik i długość w jednym argumencie, bez kopiowania. Przyjmuje `std::array`, `std::vector` i zwykłą tablicę. `const float`, bo funkcja tylko czyta. Nagłówek `<span>` jest z C++20 i doszedł do `Shader.hpp` razem z tą funkcją |
| `glGetUniformLocation(m_program, name)` z nazwą `"uWeights"` | nazwa tablicy **bez indeksu** daje położenie jej elementu 0. To samo położenie dałaby nazwa `"uWeights[0]"` |
| `glUniform1fv(location, liczba, wskaźnik)` | `1`: każdy element ma jedną składową. `f`: `float`. `v`: wartości podane wskaźnikiem. Drugi argument to **liczba elementów** do zapisania, począwszy od tego pod `location`. W `setVec3` ten argument jest zawsze 1, tu jest 7 |
| `static_cast<GLsizei>(values.size())` | `size()` zwraca `std::size_t` (bez znaku, 64 bity), a OpenGL chce `GLsizei` (`int`). Rzutowanie zapisuje zamianę wprost, bez niego kompilator ostrzega o zwężeniu |
| `values.data()` | wskaźnik na pierwszą liczbę. Liczby leżą w pamięci jedna za drugą, tak jak chce OpenGL |

Co się dzieje przy złej liczbie wartości, mówi komentarz w nagłówku i warto to umieć powtórzyć:

| Przypadek | Skutek |
|---|---|
| tyle wartości, ile tablica ma elementów | cała tablica ustawiona. Tak woła to gra: `BLOOM_BLUR_WEIGHT_COUNT` to 7, a tablica w shaderze ma `BLUR_RADIUS + 1 = 7` elementów |
| mniej wartości | ustawione są pierwsze elementy, reszta **zostaje ze starą wartością** (po przeładowaniu shadera: z zerem) |
| więcej wartości | OpenGL zapisuje tyle, ile tablica mieści, a nadmiar pomija. Bez błędu |
| nieznana nazwa | położenie -1, wywołanie jest pomijane bez błędu, jak w każdym setterze (sekcja 2.3) |

Dwa środkowe wiersze są powodem, dla którego rozjazd stałych `BLOOM_BLUR_RADIUS` (C++) i `BLUR_RADIUS` (GLSL) nie daje żadnego komunikatu ([`../renderer/post-process.md`](../renderer/post-process.md), pułapka 21).

Jedyne wywołanie w grze, w `PostProcess::drawBloom`:

```cpp
    const std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights = bloomBlurWeights();
    blurShader.setFloatArray(BLUR_WEIGHTS_UNIFORM, weights);
```

`std::array` zamienia się na `std::span<const float>` sama. Wywołanie stoi po `blurShader.use()` i przed pętlą przebiegów: raz na klatkę. Dlaczego wagi są tablicą uniformów, a nie stałą w shaderze, zapisuje notatka [`../../decisions/blur-weights-computed-on-cpu.md`](../../decisions/blur-weights-computed-on-cpu.md).

**Tablica w GLSL a aktywne uniformy.** Shader czyta elementy w pętli po indeksie (`uWeights[pixels]`), więc wszystkie siedem jest używanych. Element tablicy, którego shader nigdy nie czyta, kompilator sterownika może usunąć, tak jak nieużywany zwykły uniform.

**Jak to zostało sprawdzone.** Funkcja nie ma testu jednostkowego: woła OpenGL. Zgłoszone dla Windowsa (2026-10-05): build Debug i Release bez ostrzeżeń i poświata bloomu widoczna na zrzutach ekranu, czyli wagi dotarły do shadera (przy samych zerach cel rozmycia byłby czarny). Same liczby sprawdza `tests/BloomTests.cpp`, ale po stronie C++, przed wysłaniem. Na macOS nie sprawdzono niczego.

## 6. Panel ImGui

Uniformy nie mają własnego panelu: żaden panel nie pokazuje ich położeń ani wartości. Pośrednio widać je w dwóch miejscach. Panel "Camera" ([`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 6) zmienia pola kamery, z których co klatkę powstają macierze dla `uView` i `uProjection`. Przycisk "Reload shaders" w panelu "Shaders" ([`shader-hot-reload.md`](shader-hot-reload.md), sekcja 6) tworzy nowe programy z wyzerowanymi uniformami, a obraz nie znika tylko dlatego, że wszystkie uniformy są wysyłane co klatkę (sekcja 5.2). Trzecie miejsce to lista `View mode` w panelu "Assets" ([`../assets/asset-cache.md`](../assets/asset-cache.md), sekcja 6): wybrana pozycja trafia co klatkę do uniformu `uViewMode`, a pole wyboru `Normal mapping` pod nią decyduje (razem z trybem oświetlenia) o wartości `uNormalMapEnabled`. Czwarte to lista `Lighting` w panelu "Renderer": od wybranego trybu zależy, który program rysuje labirynt, a więc które uniformy są w ogóle ustawiane, oraz wartość `uSpecularModel`. Piąte to grupa `Highlight (specular)` w panelu "Lights": jej dwa pola trafiają co klatkę do `uSpecularStrength` i `uShininess`. Szóste to pole `Point colour` w tym samym panelu: jego kolor idzie do bloku świateł, ale też przez `game::crystalGlow` do zwykłego uniformu `uEmissive`, więc zmienia kolor świecenia samych kryształów. Pozostałe widżety panelu "Lights" nie zmieniają zwykłych uniformów, tylko bajty bloku `LightBlock` ([`uniform-buffers.md`](uniform-buffers.md), sekcja 6). Oba panele opisuje [`../scene/lights.md`](../scene/lights.md), sekcja 6, i [`../debug-ui.md`](../debug-ui.md).

## 7. Pułapki

1. **Położenie -1: literówka w nazwie uniformu albo uniform usunięty przez kompilator.** `glGetUniformLocation` zwraca -1 w dwóch sytuacjach, których nie da się od siebie odróżnić. Pierwsza to nazwa, której w shaderze nie ma: `setMat4("uModle", ...)`. Druga to uniform, który kompilator GLSL usunął, bo nie wpływa na wynik shadera (zadeklarowany, ale nieużyty, albo użyty tylko w obliczeniu, którego wynik jest potem ignorowany). Ustawianie uniformu o położeniu -1 jest po cichu ignorowane: nie ma błędu OpenGL, nie ma linii w konsoli, `setMat4` też niczego nie loguje (sekcja 5.1). Wartość po prostu "nie dochodzi", a uniform zostaje z zerami. Zmierzone w teście (sekcja 5.3): literówka w `"uModel"` daje macierz zerową w shaderze, wszystkie wierzchołki w jednym punkcie i **pusty ekran bez żadnego komunikatu**. Gdy obraz znika po zmianie w kodzie C++ albo w shaderze, pierwszą rzeczą do sprawdzenia są napisy w `ShaderUniforms.hpp` i linie `uniform` w plikach shaderów.
2. **`setMat4` przed `use()`.** `glUniform*` pisze do programu bieżącego. Bez `use()` macierz trafia do programu, który akurat jest bieżący, czyli do tego, który ktoś wybrał ostatnio. Jeśli tamten program nie ma uniformu pod tym położeniem albo ma uniform innego typu, OpenGL zgłasza `GL_INVALID_OPERATION` i `GL_CHECK` to wypisze. Jeśli typ się zgadza, błędu nie ma, a zepsuty zostaje cudzy shader. Gdy bieżącego programu nie ma wcale, błąd jest zawsze.
3. **Uniformy po `reload()`.** Nowy program zaczyna z samymi zerami (sekcja 2.1). Kod, który ustawia uniform raz, przy starcie, traci tę wartość po pierwszym przeładowaniu shadera. W projekcie wszystkie uniformy są wysyłane co klatkę, więc problemu nie ma, ale każdy uniform ustawiany "raz" trzeba po `reload()` ustawić ponownie.
4. **`GL_TRUE` jako `transpose`.** Macierz z GLM jest już w układzie kolumnowym. `GL_TRUE` transponuje ją: przesunięcie ląduje w ostatnim wierszu zamiast w ostatniej kolumnie i obraz znika albo jest zdeformowany.
5. **Zła kolejność mnożenia w shaderze.** `uModel * uView * uProjection * vec4(...)` kompiluje się bez ostrzeżeń i daje pusty ekran (zmierzone, sekcja 5.3). Macierz najbliżej wektora działa pierwsza, więc poprawna kolejność to `uProjection * uView * uModel`.
6. **Sampler ustawiony złą funkcją.** Uniform typu `sampler2D` przyjmuje tylko `glUniform1i`. `glUniform1f` i `glUniform1ui` dają `GL_INVALID_OPERATION` (zmierzone, sekcja 5.4). W projekcie służy do tego `setInt`.
7. **Do samplera trafia identyfikator tekstury.** Sampler chce numeru jednostki teksturującej, czyli tej samej liczby, którą dostało `Texture2D::bind`. `setInt("uTexture", texture.id())` nie zgłasza błędu, a sampler wskazuje jednostkę, na której zwykle nic nie ma: obraz jest czarny ([`textures.md`](textures.md), sekcja 7, pułapka 4).
8. **Uniform ustawiony w jednym programie, a potrzebny w sześciu.** `uView` i `uProjection` istnieją osobno w każdym z sześciu programów sceny (sekcja 4. Cztery programy z M7, `composite`, `preview`, `bright` i `blur`, macierzy nie mają). Wysłanie macierzy tylko do programu `lit` zostawia w programie `color` zera: scena jest, a linii kształtów kolizji nie ma. To samo dotyczy przełączenia trybu: po zmianie z `Phong` na `Gouraud` rysuje inny program, który ma własne, osobne wartości.
9. **Zmiana nazwy tylko po jednej stronie.** Zmiana napisu w `ShaderUniforms.hpp` bez zmiany w pliku shadera (albo odwrotnie) daje położenie -1 i żadnego błędu. Stała `MODEL_UNIFORM` jest przy tym wspólna dla czterech shaderów wierzchołków: zmiana nazwy w jednym pliku `.vert` wymaga zmiany w trzech pozostałych albo osobnej stałej. Trzy nazwy odblasku są z kolei zapisane w jednym pliku GLSL (`common/lighting.glsl`), z którego korzystają dwa programy.
10. **`setInt` z typem wyliczeniowym bez rzutowania.** `enum class` nie zamienia się na `int` sam, więc `setInt(VIEW_MODE_UNIFORM, m_viewMode)` się nie skompiluje: to błąd kompilacji, a nie pułapka w działaniu. Pułapką jest co innego: liczby w `game::ViewMode` i liczby w `textured.frag` muszą być te same, a tego nikt nie sprawdza ([`textures.md`](textures.md), pułapka 19).
11. **Zła funkcja dla typu.** `setFloat` na uniformie typu `int` albo `setInt` na uniformie typu `float` to `GL_INVALID_OPERATION` i wartość bez zmian (sekcja 5.7). Łatwo o to przy `uSpecularModel` (liczba całkowita) i `uShininess` (zmiennoprzecinkowa), które stoją w kodzie linia pod linią.
12. **Liczby wzoru odblasku w dwóch miejscach.** `common/lighting.glsl` porównuje `uSpecularModel` z zerem, a C++ wysyła `static_cast<int>` wartości `game::SpecularModel` (`Phong = 0`, `BlinnPhong = 1`). Zamiana kolejności w typie wyliczeniowym bez zmiany shadera nie daje żadnego błędu: tryby `Phong` i `BlinnPhong` zamieniają się wzorami. To ta sama umowa co przy `uViewMode` (pułapka 10).
13. **Pole bloku ustawiane setterem.** `setVec3("uAmbient", ...)` nic nie robi i nie zgłasza błędu: `uAmbient` jest polem bloku `LightBlock`, a pola bloku nie mają położenia (sekcja 2.4). Wygląda to dokładnie jak literówka z pułapki 1.
14. **`uNormalMatrix` wysyłany do programu, który go nie ma.** W programie `textured` to działa tylko dlatego, że położenie -1 jest ignorowane. Kto doda do `textured.vert` uniform o tej nazwie, ale innego typu, dostanie `GL_INVALID_OPERATION` w każdym wywołaniu rysującym scenę.
15. **Nieustawiony `uNormalMatrix`.** Macierz zerowa po linkowaniu daje normalną `(0, 0, 0)`. W `lit.frag` normalizacja wektora zerowego daje wartości nieokreślone, więc nie da się przewidzieć obrazu. Błędu OpenGL nie ma. Tego przypadku nie mierzyłem.
16. **Dwa samplery, jedna wartość domyślna.** Każdy sampler ma po linkowaniu wartość 0. Program z dwoma samplerami, w którym ustawiono tylko jeden albo żadnego, czyta obie tekstury z jednostki 0 i nie zgłasza błędu. Stąd `setInt` dla `uNormalMap` w każdej klatce, obok `uTexture` (sekcja 5.2).
17. **Uniform ustawiony dla jednego obiektu zostaje dla następnych.** Uniform trzyma wartość do następnego ustawienia, także między klatkami (sekcja 2.1). `GameplayRenderer::draw` ustawia `uEmissive` na świecenie kryształów i na tym kończy klatkę. Gdyby nikt nie ustawiał go z powrotem na czerń, teren i ściany następnej klatki dostałyby świecenie kryształów: błędu OpenGL nie ma, obraz jest po prostu za jasny. Dziś cofają go dwie klasy, każda dla siebie: `TerrainRenderer::draw`, pierwsze w klatce, i zaraz po nim `MazeRenderer::draw`. Tego wariantu nie uruchamiałem, wynika z kodu. Reguła na przyszłość: kto ustawia uniform tylko dla części obiektów, ten odpowiada za wartość dla pozostałych.

Pułapki dotyczące języka GLSL są w [`shaders.md`](shaders.md) (sekcja 7), a samej klasy `Shader` w [`shader-class.md`](shader-class.md) (sekcja 7).

## 8. Ćwiczenia

Zasady pracy są takie same jak w [`shaders.md`](shaders.md) (sekcja 8). Po zmianie pliku `.vert` zapisz go i naciśnij `Reload shaders` w panelu Shaders, bez kompilacji C++. Ćwiczenia 3, 4, 5, 6, 9, 10, 11, 12 i 13 zmieniają kod C++, więc wymagają zbudowania i uruchomienia programu od nowa. Ćwiczenia 1, 2, 4 i 6 były w M1 pisane dla kostki demonstracyjnej, której już nie ma: dziś działają na liniach kształtów kolizji (pole wyboru `Draw collision shapes` w panelu Collision) i na scenie. W tej postaci nikt ich jeszcze nie wykonał, tak samo jak ćwiczeń 10, 11, 12 i 13: odpowiedzi wynikają z kodu. Po każdym ćwiczeniu przywróć pliki.

1. **Bez macierzy.** Włącz `Draw collision shapes`. W `color.vert` zamień linię z `gl_Position` na `gl_Position = vec4(aPosition, 1.0);` i naciśnij `Reload shaders`. Wszystkie pudełka i wszystkie kule lądują w tym samym miejscu ekranu. Zanim spojrzysz, przewidź z danych w `ColliderLines.cpp` (sześcian jednostkowy od `(0, 0, 0)` do `(1, 1, 1)`, okrąg o promieniu 1 w płaszczyźnie XY): w której ćwiartce okna leży zarys sześcianu, dlaczego jest prostokątem, a nie kwadratem, i dlaczego okrąg wygląda jak elipsa dotykająca krawędzi okna. Dlaczego trzy okręgi kuli stały się jednym? Jakie położenie mają teraz uniformy `uModel`, `uView`, `uProjection` (sekcja 2.1)? Dlaczego program C++, który nadal woła `setMat4`, nie zgłasza błędu? (W M1 ta sama zmiana w shaderze kostki dawała prostokąt na środku okna, sekcja 5.3.)
2. **Przesunięcie w przestrzeni lokalnej.** W `lit.vert` (tryb domyślny rysuje programem `lit`) zamień `vec4(aPosition, 1.0)` na `vec4(aPosition + vec3(1.0, 0.0, 0.0), 1.0)` i naciśnij `Reload shaders`. Teren i słupki przesunęły się o metr wzdłuż osi X świata, ale ściany nie wszystkie w tę samą stronę, a kryształy zaczęły krążyć po okręgach. Dlaczego? (Wskazówka: `game::wallModelMatrix` obraca ściany biegnące wzdłuż Z o 90 stopni, a macierz kryształu obraca się w czasie.) W którym miejscu wyrażenia trzeba by dodać przesunięcie, żeby było przesunięciem w przestrzeni świata, takim samym dla wszystkich? Nie zmieniaj kodu C++. Zauważ, że kolizje zostały na starym miejscu: shader zmienia tylko obraz.
3. **Literówka w nazwie uniformu.** W `ShaderUniforms.hpp` zmień `MODEL_UNIFORM` na `"uModle"`, zbuduj i uruchom. Co widać w oknie, co w konsoli, co w panelu Shaders (linia każdego z czternastu programów)? Dlaczego znika wszystko naraz: labirynt, kryształy, brama i linie kolizji? Co dzieje się z cieniami (program `shadow_depth` też ma `uModel`)? Teren znika razem z nimi, chociaż jego macierz modelu jest jednostkowa: dlaczego? Co zostaje na ekranie (które programy sceny nie mają `uModel`)? Wyjaśnij, jaką wartość ma `uModel` w shaderach i gdzie lądują wierzchołki. Wycofaj zmianę.
4. **`setMat4` przed `use()`.** W `NightMazeApp::drawColliderLines` przenieś linię `m_colorShader.use();` pod dwa wywołania `setMat4`. Zbuduj, uruchom i włącz `Draw collision shapes`. Który program jest bieżący w chwili tych wywołań (która funkcja rysująca działa przed `drawColliderLines`)? Czy w konsoli buildu Debug jest błąd? Od czego to zależy (sekcja 7, pułapka 2)? Co dzieje się z liniami i dlaczego scena wygląda dobrze, choć dwie macierze trafiły do jej programu (wskazówka: kto i kiedy ustawia uniformy programu sceny w następnej klatce)? Wycofaj zmianę.
5. **Transpozycja.** W `Shader::setMat4` zamień `GL_FALSE` na `GL_TRUE`. Zbuduj i uruchom. Opisz obraz. Dla macierzy, która jest samym obrotem, transpozycja to obrót w przeciwną stronę: dlaczego? Która z trzech macierzy psuje się najbardziej i dlaczego? Wycofaj zmianę.
6. **Uniform `vec3`.** Dopisz do `color.frag` uniform `uniform vec3 uTint;` i pomnóż przez niego kolor. W `NightMazeApp::drawColliderLines`, po `m_colorShader.use()`, wyślij wartość funkcją `setVec3` (sekcja 5.4) pod nazwą `TINT_UNIFORM`, na przykład `glm::vec3(1.0F, 0.5F, 0.5F)`. Zbuduj, uruchom i włącz linie. Potem usuń wywołanie `setVec3` i zostaw uniform w shaderze: co widać i dlaczego? Wycofaj zmiany.
7. **Rodzina `glUniform` na kartce.** Zapisz deklarację i implementację funkcji `setVec4` w stylu `setVec3`. Którą funkcję OpenGL zawoła? Którą zawołałaby funkcja `setVec2`, a którą funkcja wysyłająca tablicę pięciu wektorów `vec3`? (Odpowiedź: `glUniform4fv(location, 1, wskaźnik)`, `glUniform2fv(location, 1, wskaźnik)`, `glUniform3fv(location, 5, wskaźnik na 15 liczb)`.)
8. **Liczenie wyszukiwań.** Labirynt ma 16 x 16 komórek. Labirynt doskonały o wymiarach `w` na `h` ma `(w + 1) * (h + 1)` ścian i tyle samo słupków. Ile razy na klatkę `MazeRenderer` ustawi `uModel`, a ile razy wszystkie uniformy zależne od obiektu? Ile dołoży `GameplayRenderer` na początku rundy, jeśli `game::crystalCountFor` daje jeden kryształ na 8 komórek, ale najwyżej 16? (Odpowiedź: 256 + 289 + 289 = 834 razy `uModel` i tyle samo razy `uNormalMatrix`, razem 1668. Kryształów jest 16, bo 256 / 8 = 32 przekracza górną granicę, do tego brama: 17 razy `uModel` i 17 razy `uNormalMatrix`.)
9. **Sampler bez `setInt`.** W `game::setModelSamplers` (`ModelDraw.cpp`) usuń linię z `setInt(TEXTURE_UNIFORM, ...)`, zbuduj i uruchom. Obraz się nie zmienia. Wyjaśnij dlaczego (jaką wartość ma uniform po linkowaniu i jaki numer ma `TEXTURE_UNIT`). Przywróć linię i usuń sąsiednią, z `setInt(NORMAL_MAP_UNIFORM, ...)`. Teraz obraz pod `Phong` powinien się zmienić: z której jednostki shader czyta mapę normalnych i co na niej leży? (Tego wariantu nie uruchamiałem: odpowiedź wynika z kodu.) Wycofaj zmianę.
10. **Zły setter.** W `NightMazeApp::drawLitMaze` zamień `setFloat(SHININESS_UNIFORM, ...)` na `setInt(SHININESS_UNIFORM, 32)`. Zbuduj i uruchom w trybie `Phong`. Co wypisuje konsola i jak często? Jaką wartość ma `uShininess` w shaderze i jak wygląda odblask (wskazówka: `pow(x, 0.0)`)? Wycofaj zmianę.
11. **Bez macierzy normalnych.** W `game::drawModel` usuń linię z `setMat3`. Zbuduj i porównaj tryby `Unlit` i `Phong`. Dlaczego pierwszy się nie zmienił? Wycofaj zmianę.
12. **Uniform czy pole bloku.** Dopisz w `drawLitMaze` linię `shader.setFloat("uPointCount", 0.0F);`. Czy znikają światła punktowe? Czy jest błąd? Wyjaśnij jednym zdaniem z sekcji 2.4. Wycofaj zmianę.
13. **Uniform, który pamięta.** Usuń linię z `setVec3(EMISSIVE_UNIFORM, ...)` najpierw tylko w `MazeRenderer::draw`. Dlaczego obraz się nie zmienia (kto ustawił czerń tuż przedtem)? Potem usuń ją także w `TerrainRenderer::draw`. Zbuduj i uruchom w trybie domyślnym. Przewidź obraz, zanim spojrzysz: jaką wartość ma `uEmissive` w chwili rysowania terenu i ścian w pierwszej klatce, a jaką w każdej następnej? Co zmieni się po zebraniu wszystkich kryształów (wskazówka: linia przed pętlą kryształów w `GameplayRenderer::draw` wykonuje się zawsze, a brama po otwarciu znika)? Co zmieni naciśnięcie `Reload shaders` i na jak długo? Wycofaj zmianę.

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

7. **Dlaczego uniformy są wysyłane co klatkę, także te, które się nie zmieniają?**
   Macierz rzutowania zależy od proporcji okna, które mogą się zmienić. Po `reload()` nowy program ma uniformy wyzerowane, więc wartości wysłane raz by przepadły: dlatego nawet numery jednostek dla samplerów `uTexture` i `uNormalMap` są ustawiane w każdej klatce. A ruchoma kamera i tak zmienia macierz widoku w każdej klatce.

8. **Do czego służy `setInt` i dlaczego powstało razem z teksturami?**
   Ustawia uniform typu `int` przez `glUniform1i`. Tą samą funkcją ustawia się uniformy typu sampler: sampler przechowuje numer jednostki teksturującej, a nie identyfikator tekstury. GLSL 4.10 nie ma zapisu `layout(binding = N)`, więc numer trzeba wysłać z C++.

9. **Co oznaczają cyfra i litery w nazwie `glUniform3fv`?**
   `3` to trzy składowe, `f` to typ `float`, `v` to wartości podane wskaźnikiem. Drugi argument (u nas 1) to liczba wektorów, większa od 1 tylko dla uniformu będącego tablicą. Jedyne takie wywołanie w grze to `glUniform1fv` z liczbą 7 w `setFloatArray` (sekcja 5.8).

10. **Co się stanie, gdy sampler zostanie ustawiony przez `glUniform1f`?**
    OpenGL zgłosi `GL_INVALID_OPERATION` i wartości nie zmieni. Typ uniformu i funkcja muszą do siebie pasować, a dla samplerów jedyną dozwoloną funkcją jest `glUniform1i`.

11. **Po co jest `ShaderUniforms.hpp` i przed czym nie chroni?**
    Zbiera czterdzieści siedem stałych z nazwami zwykłych uniformów (czterdzieści pięć różnych nazw), od czwartej części M7 siedem nazw uniformów mapy cieni w jednej stałej `MOON_SHADOW_UNIFORMS` i numer jej jednostki teksturującej, oraz nazwę i punkt wiązania bloku `LightBlock` w jednym miejscu, żeby jedenaście plików, które z nich korzystają (`NightMazeApp.cpp`, `ModelDraw.cpp`, `MazeRenderer.cpp`, `GameplayRenderer.cpp`, `ColliderLines.cpp`, `LightRig.cpp`, `Skybox.cpp`, `TerrainRenderer.cpp`, `GrassRenderer.cpp`, od M7 `PostProcess.cpp` i od czwartej części M7 `ShadowMap.cpp`), nie mogło się co do nich różnić. Nie chroni przed literówką w samej stałej ani przed zmianą nazwy tylko w pliku shadera: wtedy położenie to -1 i nie ma żadnego błędu.

12. **Sześć programów ma uniform `uView`. Ile razy trzeba go ustawić w klatce?**
    Raz w każdym programie, który w tej klatce rysuje, po jego `use()`: najwyżej w pięciu (stan sprzed M8, części 1: w czterech), bo scenę rysuje jeden z trójki `textured`, `lit`, `gouraud`, a do tego dochodzi `grass`, gdy włączona jest trawa, `color`, gdy linie kształtów kolizji są włączone, `skybox`, gdy włączone jest niebo, i od M8, części 1 `reflect`, gdy rysuje kryształy i kałuże (`shadow_depth` pracuje osobno, w przebiegach cieni przed sceną). Uniform należy do programu, więc ta sama nazwa w siedmiu programach sceny (sześciu do M8, części 1) to siedem osobnych wartości (dwa programy z M7, `composite` i `preview`, tego uniformu nie mają). W programie trawy deklaruje go shader geometrii, a nie wierzchołków, co dla C++ niczego nie zmienia.

13. **Które uniformy ustawia się funkcją `setInt`, a które `setVec3`?**
    `setInt`: samplery `uTexture` i `uNormalMap` (numery jednostek teksturujących 0 i 1), `uViewMode` (tryb podglądu, wartość typu `game::ViewMode` zrzutowana na `int`), `uSpecularModel` (wzór odblasku, wartość typu `game::SpecularModel`) i `uNormalMapEnabled` (typ `bool` w GLSL, 1 albo 0). `setVec3`: `uTint` (kolor materiału w trzech shaderach fragmentów sceny), `uEmissive` (własne świecenie powierzchni w tych samych trzech) i `uColor` (kolor linii w `color.frag`). Do tego `setMat3` dla `uNormalMatrix` i `setFloat` dla `uSpecularStrength` i `uShininess`.

14. **Ile razy na klatkę jest ustawiany `uModel` dla labiryntu domyślnego i co z tego wynika?**
    243 razy: raz dla terenu, 121 razy dla ścian i 121 dla słupków, każdy obiekt ma własne wywołanie rysujące. Tyle samo razy ustawiany jest `uNormalMatrix`, a z bramą i 13 kryształami cała klatka programu sceny w trybie domyślnym na początku rundy to 554 ustawienia (547 do trzeciej części M7, a do M5, ze stu płytkami podłogi zamiast terenu, było 342 i 742). Od czwartej części M7 te same obiekty są rysowane jeszcze raz do mapy cieni: kolejne 257 ustawień `uModel` w programie `shadow_depth`, w 543 wywołaniach setterów (sekcja 5.6). Trawa i niebo dokładają 16 i 5 ustawień we własnych programach. Każde wyszukuje położenie po nazwie, bo klasa nie ma pamięci podręcznej. Kosztu nie zmierzyłem. To cena prostego kodu, do poprawienia dopiero wtedy, gdy pomiar pokaże problem.

15. **Co robi `setMat3` i do czego służy w grze?**
    Wyszukuje położenie i woła `glUniformMatrix3fv`: jedna macierz 3 x 3, dziewięć liczb `float`, bez transpozycji. Gra wysyła nim `uNormalMatrix`, macierz dla normalnych policzoną przez `scene::normalMatrix` dla każdego obiektu.

16. **Dlaczego `uNormalMatrix` jest wysyłany także do programu `textured`, który go nie ma, i co się wtedy dzieje?**
    `game::drawModel` rysuje tym samym kodem trzema programami i nie wie, którym. W programie `textured` położenie to -1, a `glUniform*` z położeniem -1 jest ignorowane bez błędu. Kosztuje to zbędne wyszukanie i zbędne odwrócenie macierzy.

17. **Czym różni się `setFloat` od `setInt` i co się stanie po ich pomyleniu?**
    `setFloat` woła `glUniform1f`, `setInt` woła `glUniform1i`. Funkcja musi pasować do typu uniformu w GLSL: pomyłka to `GL_INVALID_OPERATION`, a wartość zostaje bez zmian.

18. **Czym różni się zwykły uniform od pola bloku uniformów?**
    Zwykły uniform leży w programie, ma położenie i ustawia się go przez `glUniform*` w każdym programie osobno. Pole bloku leży w buforze wspólnym dla programów, nie ma położenia (`glGetUniformLocation` zwraca -1) i zmienia się tylko przez zapis do bufora. W grze blokiem są światła, a zwykłymi uniformami wszystko inne.

19. **Gdzie są zadeklarowane `uSpecularModel`, `uSpecularStrength` i `uShininess` i do którego shadera należą?**
    W pliku `common/lighting.glsl`, dołączanym przez `#include`. W programie `lit` trafiają do shadera fragmentów, w programie `gouraud` do shadera wierzchołków. Dla C++ to bez różnicy: uniform należy do programu.

20. **Dlaczego `MazeRenderer::draw` ustawia `uEmissive` na czerń w każdej klatce, skoro kamień nigdy nie świeci?**
    Bo uniform trzyma wartość do następnego ustawienia, także między klatkami, a ten sam program rysuje potem kryształy, dla których `GameplayRenderer::draw` ustawia świecenie. Bez cofnięcia ściany następnej klatki byłyby rysowane z wartością zostawioną przez kryształy. Od drugiej części M6 to samo robi wcześniej `TerrainRenderer::draw` dla terenu, więc linia w `MazeRenderer` jest dziś zabezpieczeniem: klasa nie polega na tym, że ktoś przed nią ustawił czerń. Drugi powód jest ten sam co dla wszystkich uniformów: po `reload()` wartości wracają do zera i trzeba je wysłać ponownie.

21. **Co robi `uEmissive` we wzorze koloru i dlaczego jest dodawany do światła rozproszonego, a nie do wyniku?**
    W `lit.frag` kolor to `surface * (diffuse + uEmissive) + specular`, gdzie `diffuse` i `specular` to od czwartej części M7 światło po odjęciu cienia księżyca. Świecenie jest więc mnożone przez kolor powierzchni (teksturę i `uTint`), tak jak światło rozproszone: kryształ świeci swoim kolorem i widać jego ścianki, a nie jednolitą plamę. Nie zależy od żadnego światła sceny, więc kryształ jest jasny także w ciemnym kącie. W `textured.frag` odpowiednikiem jest `texel * uTint * (vec3(1.0) + uEmissive)`: bez oświetlenia powierzchnia jest pokazana jak pod białym światłem o sile 1, a świecenie do niego dochodzi.

## 10. Źródła

- LearnOpenGL, rozdział "Shaders" (<https://learnopengl.com/Getting-started/Shaders>): uniformy, `glGetUniformLocation`, ustawianie uniformu po `glUseProgram`.
- docs.gl, OpenGL 4: `glGetUniformLocation` (<https://docs.gl/gl4/glGetUniformLocation>), `glUniform` (<https://docs.gl/gl4/glUniform>, w tym `glUniformMatrix4fv`, `glUniformMatrix3fv`, `glUniform1i`, `glUniform1f`, `glUniform3fv`, zasada, że sampler przyjmuje tylko `glUniform1i`, błąd `GL_INVALID_OPERATION` przy funkcji niezgodnej z typem uniformu i zachowanie dla położenia -1), `glProgramUniform` (<https://docs.gl/gl4/glProgramUniform>).
- Khronos OpenGL Wiki: "Uniform (GLSL)" (<https://www.khronos.org/opengl/wiki/Uniform_(GLSL)>, o uniformach nieaktywnych).
- Specyfikacja OpenGL 4.1 Core Profile (<https://registry.khronos.org/OpenGL/specs/gl/glspec41.core.pdf>), sekcja 2.11.7: domyślny blok uniformów a bloki nazwane, położenie -1 dla pól bloku.
- Dokumenty w tym repozytorium: [`uniform-buffers.md`](uniform-buffers.md) (blok `LightBlock`, bufor uniformów), [`shader-includes.md`](shader-includes.md) (`#include` w shaderach), [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md) (programy `lit` i `gouraud`), [`../scene/lights.md`](../scene/lights.md) (plik `common/lighting.glsl`), [`../game/flashlight.md`](../game/flashlight.md) (`LightRig`), [`../game/gameplay.md`](../game/gameplay.md) (kryształy, brama, `uEmissive`), [`shaders.md`](shaders.md) (potok, GLSL), [`shader-class.md`](shader-class.md) (klasa `Shader`), [`shader-hot-reload.md`](shader-hot-reload.md) (uniformy po przeładowaniu), [`textures.md`](textures.md) (samplery, jednostki teksturujące, shadery `textured.*`), [`../scene/collision.md`](../scene/collision.md) (shadery `color.*`), [`../game/maze-rendering.md`](../game/maze-rendering.md) (pętla rysująca labirynt), [`../scene/transforms.md`](../scene/transforms.md) i [`../scene/camera.md`](../scene/camera.md) (skąd biorą się macierze), [`../core/gl-check.md`](../core/gl-check.md), [`../../libraries/glm.md`](../../libraries/glm.md) (zapis kolumnowy, `value_ptr`).
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o shaderach i języku GLSL).
- "OpenGL. Księga eksperta" (rozdziały o potoku programowalnym i shaderach).
