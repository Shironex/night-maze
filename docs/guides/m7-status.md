# Stan M7 na koniec 2026-10-05

Krótka ściąga: co z kamienia milowego M7 jest w kodzie, co zostało, jakie decyzje już zapadły dla brakujących części i w jakiej kolejności sprawdzać wszystko na Macu. Szczegóły są w dokumentach, do których prowadzą odnośniki. Ten plik nie zastępuje list kontrolnych w [`build-windows.md`](build-windows.md) i [`build-macos.md`](build-macos.md).

**Jednym zdaniem:** cztery części z sześciu są kompletne w kodzie na Windowsie, **żadna nie jest zamknięta** (testy ręczne i macOS są otwarte dla wszystkich), tagu nie ma.

## 1. Co jest zbudowane

| Część | Co daje | Commity | Dokument | Lista kontrolna |
|---|---|---|---|---|
| 1. Bufor HDR i gamma | scena rysowana do framebuffera `GL_RGBA16F` z teksturą głębi, przebieg składający z ekspozycją i trzema krzywymi mapowania tonów, tekstury sRGB, kodowanie na końcu klatki, panel Framebuffers | `0c76316` `feat(gfx): add colour space conversion between srgb and linear`, `56f5832` `feat(gfx): add a framebuffer class with colour and depth textures`, `8f87850` `feat(game): render the scene to an hdr buffer with tone mapping and gamma` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md), [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) | [`build-windows.md`](build-windows.md), sekcja 17 |
| 2. Bloom | przebieg jasności i rozdzielne rozmycie Gaussa w trzech celach o połowie rozdzielczości, dodane przed mapowaniem tonów | `bfc70f3` `feat(game): add bloom from a half resolution bright pass and gaussian blur` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | sekcja 18 |
| 3. Mgła i winieta | mgła wykładnicza z wysokością, liczona z bufora głębi w przebiegu składającym, i winieta | `e8e1822` `feat(game): add ground fog from the depth buffer and a vignette` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | sekcja 19 |
| 4. Cienie księżyca | mapa cieni 2048 x 2048 z rzutem ortograficznym dopasowanym do terenu, `sampler2DShadow` z obiektem samplera, filtr sprzętowy 2 x 2, PCF do 7 x 7, bias w metrach, panel Shadows | commitowana razem z tym dokumentem | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), [`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md) | sekcja 20 |

Stan liczb po czwartej części (zgłoszone dla Windowsa, 2026-10-05): `make check` przechodzi, 310 przypadków testowych i 103751 asercji, jedenaście programów shaderów, dwanaście paneli.

Tematy wykładu: 10 (rendering pozaekranowy) jest **w toku**, bo brakuje minimapy. 11 (shadow mapping) jest **w toku**, bo brakuje cienia latarki. Żaden nie jest odhaczony w [`../syllabus.md`](../syllabus.md).

## 2. Co zostało

1. **Część 5: cień latarki.** Druga mapa cieni, z rzutem perspektywicznym. Kodu nie ma.
2. **Część 6: minimapa.** Widok labiryntu rysowany do osobnego framebuffera. Kodu nie ma.
3. **Zamknięcie M7:**
   - testy ręczne na Windowsie: listy 17.2, 18.2, 19.2 i 20.2 w [`build-windows.md`](build-windows.md) (i listy dwóch brakujących części, gdy powstaną),
   - build, testy i listy na macOS ([`build-macos.md`](build-macos.md), kolejność w sekcji 6 niżej),
   - odhaczenie tematów 10 i 11 w [`../syllabus.md`](../syllabus.md),
   - tag.

## 3. Decyzje właściciela dla brakujących części (2026-10-05)

| Część | Decyzja | Notatka |
|---|---|---|
| cień latarki | światło latarki przenosi się z oka do **ręki**: trochę w prawo i trochę poniżej oka | [`../decisions/flashlight-in-hand.md`](../decisions/flashlight-in-hand.md) |
| minimapa | pokazuje **tylko odkryte korytarze**, z przełącznikiem debugowania, który odsłania cały labirynt | [`../decisions/minimap-discovered-corridors.md`](../decisions/minimap-discovered-corridors.md) |

Wyjaśnienie do pierwszej (moje, nie treść decyzji): światło stojące w oku rzuca cienie dokładnie za przedmioty, więc nie byłoby ich widać. Nic poza tymi dwoma zdaniami nie jest ustalone: ani przesunięcie ręki w liczbach, ani rozmiar mapy latarki, ani reguła odkrywania komórek.

## 4. Co część 5 dostaje gotowe z części 4

Z notatek z implementacji, sprawdzone w kodzie. Szerzej: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.20.

**Do użycia bez zmian:**

- klasa `game::ShadowMap` (jeden obiekt na światło: framebuffer głębi, sampler, podgląd),
- `gfx::ComparisonSampler`,
- struktura `game::ShadowSettings` (jedna na światło),
- `game::ShadowUniformNames` i funkcja `setShadowUniforms`: drugi zestaw nazw to druga stała obok `MOON_SHADOW_UNIFORMS`,
- funkcja GLSL `shadowMapVisibility(map, coordinates, pcfRadius)`: mapa jest jej parametrem,
- `NightMazeApp::drawShadowCasters(lightSpace)`: dostaje przestrzeń światła jako argument,
- `drawShadowMapTab` w panelu Shadows: druga zakładka to drugie wywołanie,
- jednostka teksturująca 4 (księżyc ma 3).

**Do dopisania:**

- perspektywiczna przestrzeń światła (`scene::LightSpace` ma dziś tylko `directionalLightSpace`),
- przeliczenie biasu, które nie opiera się na `extent.z`: głębia rzutu perspektywicznego nie jest liniowa, więc dzielenie metrów przez głębię pudełka nie działa,
- tryb podglądu z linearyzacją głębi (dzisiejszy `RawDepth` pokazałby dla mapy perspektywicznej prawie samą biel),
- pola struktury `Lighting` z udziałem reflektora (rozproszone i odbłysk) i pasujące zmienne w programie `gouraud`, tak jak dziś ma je księżyc,
- pozycja latarki w ręce zamiast w oku (`buildLightSet`, test `the flashlight sits at the eye and points where the camera looks`).

## 5. Znane ograniczenia i otwarte obserwacje

Zebrane ze wszystkich czterech części. "Znane" znaczy: zapisane, świadomie zostawione albo czekające na sprawdzenie.

| Co | Skąd | Stan |
|---|---|---|
| mgła z góry zakrywa labirynt: wysokość jest brana w miejscu piksela, bez całki wzdłuż promienia (z 30 m około 95 procent mgły zamiast około 22) | część 3, [`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md) | świadomie zostawione. Dotknie minimapy: jej widok nie może przejść przez przebieg z mgłą |
| mgła na niebie tuż nad horyzontem zależy od miejsca na ekranie: daleka płaszczyzna jest płaska i obraca się z kamerą | część 3, [`../decisions/fog-no-special-case-for-sky.md`](../decisions/fog-no-special-case-for-sky.md) | policzone, nikt tego nie oglądał |
| plama latarki na ścianie nie daje poświaty, nawet z metra | część 2, [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | otwarta obserwacja |
| poświata jest względnie o połowę cieńsza w 1440p niż w 720p: jądro rozmycia ma promień w pikselach celu | część 2, [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | otwarta obserwacja, w 1440p sprawdzono tylko wycinek obrazu |
| **niewyjaśniony spadek liczby klatek przy wyłączonych cieniach**: około 1250 i 630 klatek na sekundę (720p i 1440p) w sesji części 4 wobec około 1880 i 1145 w sesji części 3 | część 4, [`build-windows.md`](build-windows.md), sekcja 20.1 | **do ponownego pomiaru**. Sesji nie wolno porównywać. Potrzebny jest pomiar obu commitów w jednej sesji (ostatni punkt listy 20.2) |
| bias cieni nie zależy od rozmiaru teksela ani od jądra PCF: jądra `5 x 5` i `7 x 7` oraz mapa 1024 mogą lekko przyciemniać oświetlony grunt | część 4, [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.11 | policzone, nikt tego nie oglądał. Punkt listy 20.2 |
| cień rzuca tylko księżyc. Latarka i kryształy świecą przez ściany | część 4 | latarka: część 5. Kryształy: nie ma w planie |
| jedna mapa cieni bez kaskad, pudełko stałe względem terenu, trawa nie rzuca cienia | część 4, [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.19 | świadomie zostawione |
| namalowana tarcza księżyca nie idzie za suwakami `Moon yaw` i `Moon pitch`, a od części 4 cienie idą | M6 i część 4, [`../decisions/painted-moon-fixed-direction.md`](../decisions/painted-moon-fixed-direction.md) | świadomie zostawione |
| środkowe commity każdego kamienia milowego nie budują się samodzielnie | historia repozytorium | do pomiarów i do `git bisect` nadaje się tylko ostatni commit każdej części |
| żadnej kontrolki paneli z M7 nikt nie kliknął myszą | wszystkie części | listy 17.2 do 20.2 |

## 6. macOS: kolejność sprawdzania

Na Macu nie był budowany ani uruchamiany żaden kod od M2. Listy są w [`build-macos.md`](build-macos.md), po jednej na część, wszystkie w całości otwarte. Poniżej kolejność od rzeczy, które najłatwiej mogą nie zadziałać na sterowniku OpenGL 4.1 firmy Apple, bo każda z nich blokuje to, co po niej.

| # | Co sprawdzić najpierw | Dlaczego to ryzyko | Sekcja w `build-macos.md` |
|---|---|---|---|
| 0 | `Makefile`: czy `make` i `make check` działają na macOS po zmianach zrobionych dla Windowsa (`b9298ec` `fix(make): run the makefile targets on windows from the developer shell`) | bez tego nie ma bramki. Zmiany powstały na Windowsie i na Macu nikt ich nie uruchomił | "Skróty: `make`" |
| 1 | **shader geometrii trawy** (`grass.geom`) | jedyny program z etapem geometrii. Kompilator GLSL Apple nigdy go nie widział | "M6, część 2 (teren i trawa)" |
| 2 | **cel rysowania `GL_RGBA16F`** | od części 1 cała scena idzie przez ten framebuffer: jeśli jest niekompletny, nie ma obrazu | "M7, część 1 (bufor HDR i gamma)" |
| 3 | **formaty tekstur sRGB i `GL_EXT_texture_sRGB_decode`** | formaty `GL_SRGB8` i `GL_SRGB8_ALPHA8` decydują o kolorach wszystkich tekstur sceny. Rozszerzenie dotyczy tylko podglądów tekstur w panelu Assets (`debug::RawTextureSampler`): bez niego podglądy są ciemniejsze, scena nie | "M7, część 1 (bufor HDR i gamma)" |
| 4 | **framebuffer z samą głębią i sampler cieni** (`glDrawBuffer(GL_NONE)`, `sampler2DShadow` z obiektem samplera, `GL_CLAMP_TO_BORDER`, podgląd głębi) | cztery rzeczy użyte pierwszy raz w części 4. Każda może wyłączyć cienie w całości | "M7, część 4 (cienie księżyca)" |
| 5 | **rozmiary framebufferów na ekranie Retina** | okno 1280 x 720 ma bufor 2560 x 1440: scena, cele bloomu, podglądy i `glViewport` po każdym przebiegu. Do tego wydajność: PRD wymaga 60 klatek w 1440p na MacBooku | listy części 1 do 4 M7, punkty o Retinie |
| 6 | **`glPolygonMode`** (pole `Wireframe` terenu) | tryb linii w profilu Core na sterowniku Apple | "M6, część 2 (teren i trawa)" |
| 7 | reszta list, od M2 do części 4 M7, w kolejności dokumentu | wygląd, panele, sterowanie | wszystkie sekcje "na macOS: lista w całości otwarta" |

Wskazówka do punktu 0: jeśli `make` nie działa, wszystkie kroki da się wykonać samymi presetami CMake, które są opisane w tym samym dokumencie.
