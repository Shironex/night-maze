# doctest 2.5.3

Dokument biblioteki dla kamienia milowego M2 + M3. Opisuje konfigurację z
[`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) i
[`CMakeLists.txt`](../../CMakeLists.txt) oraz tę część API, której używają testy w katalogu
[`tests/`](../../tests/).

**Stan na dziś (po szóstej części M7, 2026-10-06): doctest używa jeden program, `night_maze_tests`, z trzydziestu plików `.cpp` (`tests/main.cpp`, dwadzieścia siedem plików z tabeli niżej i dwa pliki z M8, `RaycastTests.cpp` i `InteractablesTests.cpp`, które tabela pomija).** Opis poniżej, do tabeli włącznie, pisano po piątej części M7, kiedy tabela miała dwadzieścia pięć plików; szósta część dodała do niej dwa, `DiscoveryTests.cpp` i `MinimapTests.cpp`, i jeden przypadek do `MazeLayoutTests.cpp`. **M8, część 2 (selekcja, dźwignie i kartki, 2026-10-06):** doszedł jeden plik, `InteractionTests.cpp` (21 przypadków), więc program ma trzydzieści trzy pliki `.cpp` z `tests/main.cpp`. **M8, część 1 (environment mapping, 2026-10-06):** do programu doszły dwa pliki, `EnvironmentMappingTests.cpp` (11 przypadków) i `PuddleTests.cpp` (20, od poprawek z 2026-10-06 21), więc po scaleniu jest ich trzydzieści dwa z `tests/main.cpp` (trzydzieści jeden plików z testami), a zgłoszona liczba przypadków dla scalonego drzewa to 445 (414 plus 31).

Stan po piątej części M7: Składa się z dwudziestu
sześciu plików: `tests/main.cpp` (punkt wejścia) i dwudziestu pięciu plików z testami
(siedemnasty, `SkyboxTests.cpp`, doszedł w pierwszej części M6, `TerrainTests.cpp` i
`GrassTests.cpp` w drugiej, `ColorSpaceTests.cpp` i `FramebufferTests.cpp`
w pierwszej części M7, `BloomTests.cpp` w drugiej części M7, `FogTests.cpp`
i `VignetteTests.cpp` w trzeciej części M7, a ostatni, `ShadowTests.cpp`, w czwartej
(cienie księżyca, 2026-10-05)). Osiem pierwszych
wierszy tabeli to pliki z M2 + M3, cztery następne doszły z oświetleniem (pierwsza część M4),
trzynasty z mapami normalnych (druga część M4), trzy następne z rozgrywką (M5), siedemnasty z niebem (pierwsza część M6), dwa następne z terenem i trawą (druga część M6), dwa kolejne z buforem HDR i gammą (pierwsza część M7, która dodała też jeden przypadek do `LightingTests.cpp`), następny z bloomem (druga część M7), dwa następne z mgłą i winietą (trzecia część M7), a ostatni z cieniami księżyca (czwarta część M7). M5 zmieniło
też liczby w trzech starszych plikach: `ColliderTests.cpp` dostał siedem przypadków o kulach,
`MazeTests.cpp` dwa (w tym przeniesiony test `isDeadEnd`), a z `LightingTests.cpp` ubyło
siedem, bo światła w ślepych zaułkach zostały usunięte z gry. Druga część M6 zabrała jeden
przypadek z `ObjLoaderTests.cpp` (test modelu płytki podłogi, usuniętego razem z plikiem) i
zmieniła treść kilku starszych testów bez zmiany ich liczby: testy gracza, świata, rundy,
kryształów i wyjścia podają dziś teren albo wysokość gruntu, a `ImageLoaderTests.cpp` czyta
tekstury gruntu (`ground.png`, `ground_normal.png`) zamiast tekstur podłogi:

| Plik | Przypadków | Co sprawdza | Dokument |
|---|---|---|---|
| `ColliderTests.cpp` | 19 | kolizje: `Aabb`, `overlaps`, `moveAndSlide`, od M5 także kule (`Sphere`, dwa testy `overlaps` z kulą, `closestPoint`) | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| `MazeTests.cpp` | 8 | klasa `Maze` i kierunki, od M5 także `isDeadEnd` (przeniesione z `Lighting`) i porównywanie `MazeCell` | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| `MazeGeneratorTests.cpp` | 11 | `randomBelow`, `generateMaze`, labirynt wzorcowy | tamże |
| `MazeLayoutTests.cpp` | 13 | układ w świecie i pudełka kolizji labiryntu, od szóstej części M7 także `cellAt` | tamże |
| `MazeWorldTests.cpp` | 8 | `buildMazeWorld`: macierze modelu, pudełka, start, od M5 także wyjście, brama i kryształy świata | [`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) |
| `PlayerTests.cpp` | 13 | gracz: chodzenie, sprint, ślizganie, noclip | [`../modules/game/player.md`](../modules/game/player.md) |
| `ObjLoaderTests.cpp` | 19 | loader OBJ i MTL, od M4 także linia mapy normalnych `map_Bump` i styczne modeli gry. Do pierwszej części M6 było 20: dwudziesty sprawdzał model płytki podłogi | [`../modules/assets/obj-loader.md`](../modules/assets/obj-loader.md) |
| `ImageLoaderTests.cpp` | 10 | loader obrazów, od M4 także zawartość map normalnych (średnia, konwencja kanału zielonego), od M6 wczytanie bez odwracania wierszy (`RowOrder::TopFirst`) | [`../modules/assets/images.md`](../modules/assets/images.md) |
| `ShaderSourceTests.cpp` | 22 | tekst shadera: `expandIncludes` (dyrektywa `#include`, linie `#line`, błędy) i `nameSourceFiles` (nazwy plików w komunikatach sterownika) | [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md) |
| `LightTests.cpp` | 20 | matematyka świateł: zanik z odległością, stożek reflektora, `directionFromAngles`, bajty bloku świateł (`packLightBlock`) | [`../modules/scene/lights.md`](../modules/scene/lights.md) |
| `LightingTests.cpp` | 15 | ustawienia oświetlenia gry: wartości domyślne, `usesNormalMap`, `flashlightPose`, `buildLightSet`. Od piątej części M7 latarka stoi w ręce: `buildLightSet` dostaje pozę (`FlashlightPose`), przypadek o latarce w oku został przemianowany na `the flashlight sits in the hand and is aimed at a point in front of the eye`, a doszły cztery: z zerowymi przesunięciami latarka jest w oku jak dawniej, wiązka przechodzi przez punkt na osi widoku, ręka zostaje w ciele gracza przy każdym obrocie kamery i latarka zawsze ma kierunek (także dla zbieżności 0 i dla kamery patrzącej prosto w dół). Od M7 (część pierwsza) `buildLightSet` przelicza kolory z sRGB na liniowe: starsze przypadki porównują wynik z `gfx::srgbToLinear(...)`, a jedenasty sprawdza liczbę 0,21404 dla szarości 0,5 i to, że intensywność nie jest przeliczana. Testy świateł w ślepych zaułkach z M4 zniknęły razem z tym kodem | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| `TransformTests.cpp` | 4 | macierz normalnych (`scene::normalMatrix`) | [`../modules/scene/transforms.md`](../modules/scene/transforms.md) |
| `TangentTests.cpp` | 9 | styczne wierzchołków: `triangleTangents`, `computeTangents`, `countMirroredTriangles` | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md) |
| `ExitTests.cpp` | 11 | wyjście: odległości liczone w przejściach (`passageDistances`), najdalsza komórka (`farthestCell`), brama (`placeExit`, `wallSegmentOn`), strefa wyjścia (`exitZone`) | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| `CrystalTests.cpp` | 14 | kryształy: ile ich jest (`crystalCountFor`), gdzie stoją (`placeCrystals`), kołysanie, obrót, pulsowanie i świecenie | tamże |
| `RoundTests.cpp` | 25 | cała runda bez okna: `startRound`, `updateRound` (zbieranie, bateria, brama, wygrana), `flashlightFlicker`, `lightingForFrame`, `crystalLightPositions` | tamże |
| `SkyboxTests.cpp` | 5 | od M6: sześć plików nieba jako ściany tekstury sześciennej (rozmiary, reguła wyboru ściany, miejsce księżyca, gradient tła, zgodność na dwunastu krawędziach sześcianu) | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md) |
| `TerrainTests.cpp` | 27 | od M6: teren z mapy wysokości. `Heightmap::sample` i `heightmapFromImage`, rzeźba `terrainRelief`, siatka i wzór wysokości, `heightAt` na trójkącie siatki, `lowestHeightUnder`, siatka z `buildTerrainMesh` (wierzchołki, normalne, UV, styczne, kierunek nawijania), prawdziwy plik `heightmap.png`, ściany, słupki i brama zatopione w gruncie, kryształy nad gruntem, stopy gracza na gruncie w górę i w dół zbocza | [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) |
| `GrassTests.cpp` | 9 | od M6: miejsca kępek trawy (`placeGrass`). Stałe i ustawienia domyślne, gęstość 0, powtarzalność z ziarna, liczba kępek na ścianę i na stronę, pas przy ścianie, żadna kępka w ścianie, słupku ani bramie, kępki na gruncie, rzadki rozsiew na wzgórzach z dala od labiryntu | [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md) |
| `ColorSpaceTests.cpp` | 9 | od M7: `gfx::srgbToLinear` i `gfx::linearToSrgb`. Czerń i biel, znane wartości standardu (bajt 128 to 0,21586, 0,5 to 0,21404, a liniowe 0,5 to 0,73536), prosty odcinek dla najciemniejszych wartości i ciągłość na progu, różnica od potęgi 2,2, powrót każdego z 256 bajtów po zdekodowaniu i zakodowaniu, zachowanie kolejności jasności, przycinanie poza zakresem od 0 do 1, kolor kanał po kanale | [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) |
| `FramebufferTests.cpp` | 3 | od M7: części `gfx::Framebuffer` bez OpenGL. Nazwy formatów, pusta `FramebufferSpec`, osobny tekst dla każdego stanu kompletności | [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) |
| `BloomTests.cpp` | 7 | od drugiej części M7: części bloomu bez OpenGL (`game/Bloom.*`). Rozmiar celu (`bloomTargetExtent`: połowa sceny, reszta z dzielenia przepada, nie mniej niż 1 piksel), wagi rozmycia Gaussa (`bloomBlurWeights`: suma całego jądra równa 1, wagi maleją i są dodatnie, stosunki zgodne z `exp(-d * d / (2 * sigma * sigma))`, dwie liczby policzone ręcznie), wartości startowe `BloomSettings` w zakresach. Przykład `REQUIRE` przed pętlą, która indeksuje tablicę | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 5.8 |
| `FogTests.cpp` | 11 | od trzeciej części M7: mgła bez OpenGL (`game/Fog.*`). Współczynnik wysokości (`fogHeightFactor`: 1 na wysokości bazowej i pod nią, połowa co `ln(2) / heightFalloff` metrów nad nią, zanik 0 daje to samo na każdej wysokości), ilość mgły (`fogAmount`: zero przy odległości 0, gęstości 0 i współczynniku 0, rośnie z każdym metrem do 100 m i nie przekracza 1, prawo wykładnicze z liczbą policzoną ręcznie 0,5507 i z odcinkami 10 m i 5 m, które razem dają to samo co 15 m), `fogAmountAt` (odległość od oka i wysokość samego punktu), `worldPositionFromDepth` (punkt świata odzyskany z miejsca na ekranie i głębi, środek ekranu przy głębi 0 i 1 na płaszczyznach przycinania), wartości startowe `FogSettings` (księżyc czysty, niebo pod horyzontem zakryte, podłoże 8 m dalej widoczne) | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 5.8 i 5.12 |
| `VignetteTests.cpp` | 7 | od trzeciej części M7: winieta bez OpenGL (`game/Vignette.*`). Stała `VIGNETTE_CORNER_DISTANCE` równa pierwiastkowi z 0,5, środek ekranu bez zmiany, siła 0 nie zmienia niczego, cztery narożniki tracą dokładnie udział `strength`, współczynnik maleje od promienia do narożnika (i w połowie drogi wynosi `1 - strength / 2`), środek prawej i środek górnej krawędzi dają ten sam wynik (brak korekty proporcji okna), wartości startowe `VignetteSettings` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 5.8 i 5.13 |
| `ShadowTests.cpp` | 31 | od czwartej części M7 (16 przypadków), od piątej (2026-10-06) z 15 kolejnymi, o ostrosłupie latarki: `spotLightSpace` (punkt na osi w środku mapy, cały stożek w mapie z marginesem, głębia od bliskiej do dalekiej płaszczyzny i jej nierówność, pozycja, płaszczyzny i rozmiar, kierunek prosto w górę i w dół, kierunek dowolnej długości i zerowy, stożek i zasięg poza granicami, przesunięcie punktu ku światłu zachowuje teksel i obniża głębię, punkt za światłem ma ujemne `w`), `LightSpace::kind` dla księżyca, zgodność pozycji i kierunku światła z jego mapą, `biasForShader` (głębia dla pudełka, metry dla ostrosłupa), `shadowTexelSizeAt` (teksel rośnie z odległością), `flashlightShadowDefaults` i to, że bias startowy pokrywa grunt do 10 m przed graczem. Od czwartej części: cienie bez OpenGL (`scene/LightSpace.*`, `game/Shadows.*`). Pudełko światła kierunkowego (`directionalLightSpace`: mieści wszystkie narożniki, jest dopasowane z samym marginesem, światło prosto w dół, prawie prosto w dół i kierunek o długości zero, długość kierunku bez znaczenia), współrzędne w mapie cieni (punkty na jednym promieniu trafiają w ten sam teksel i różnią się tylko głębią, punkt poza pudełkiem wypada poza mapę), pudełko rzucających cień (`shadowCasterBounds`) sprawdzane na każdym pudełku kolizji labiryntu startowego, rozmiar teksela (`shadowTexelSize`), bias (`shadowBias`, `biasInDepthUnits`), rozmiary mapy, jądro PCF i zgodność `moonDirection` ze światłami klatki | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md) |
| `DiscoveryTests.cpp` | 19 | od szóstej części M7: odkrywanie komórek minimapy (`game/Discovery.*`). Pusta siatka, `discover` liczy komórkę raz, korytarz do ściany, brak odkrycia przez ścianę i za rogiem, jednakowy zasięg w czterech kierunkach, odnoga widoczna dopiero z linii, otwarty bok na brzegu, komórka spoza labiryntu, pozycja z dowolną wysokością, ściana usunięta później, `startRound`, `updateRound`, nowa runda, wygrana | [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md) |
| `MinimapTests.cpp` | 19 | od szóstej części M7: dane i matematyka minimapy (`game/Minimap.*`). Wartości startowe ustawień, kwadrat w czterech rogach (202 i 14 pikseli przy 720), Retina, wysokość zamiast szerokości, mały framebuffer, ustawienia poza granicami, półbok i metry na piksel, północ u góry, labirynt niekwadratowy, podłogi, `revealAll`, ściany, brama, kryształy, strzałka, minimalne rozmiary w pikselach, największy labirynt | tamże |
| `EnvironmentMappingTests.cpp` | 11 | od M8, części 1: formuły environment mappingu bez OpenGL (`game/EnvironmentMapping.*`). Wartości startowe ustawień, odbicie od poziomego lustra, długość i kąt promienia odbitego, prawo Snella dla pięciu kątów, `eta` równe 1 i promień wzdłuż normalnej, współczynniki materiałów, kąt krytyczny 41,81 stopnia i brak promienia za nim, `refractOrReflect` na całej siatce suwaka, Fresnel (wartości brzegowe, monotoniczność) i liczby dla kałuży przy oczach 1,7 m nad gruntem. Tabela nie wlicza go do sumy 329 (poza tabelą jak pliki M8 bez okna) |
| `PuddleTests.cpp` | 21 | od M8, części 1 (przerobione 2026-10-06, było 20): kałuże (`game/Puddles.*`). Liczba kałuż z udziału, 13 w labiryncie startowym, test złoty labiryntu 4 na 4 z ziarna 1, brak kałuży na starcie, wyjściu i pod kryształem, powtarzalność, większy udział zachowuje istniejące kałuże, limity rozmiaru i przesunięcia, wyjątek dla startu poza labiryntem, narożniki brzegu, środek kałuży `PUDDLE_LIFT` nad gruntem na płaskim i nierównym terenie, skala wysokości przesuwa tylko `y`, brak kontaktu ze ścianami, siatka z 193 wierzchołkami i 352 trójkątami, każdy wierzchołek `PUDDLE_LIFT` nad gruntem pod nim, kilka kałuż w jednej siatce, odstęp od gruntu powyżej 6 mm na prawdziwej mapie wysokości. Tabela nie wlicza go do sumy 329 |
| `InteractionTests.cpp` | 21 | od M8, części 2: dźwignie i kartki w świecie, rundzie i kadrze (`game/Interaction.*`, `Round.*`, `MazeWorld.*`, `Minimap.*`). Dźwignie i kartki świata na gładkim i na nierównym terenie, liczby z ustawień, nowa runda bez pociągniętych dźwigni, wzory opadania (1,5 s, zatrzymanie na pełnej głębokości), pociągnięcie otwiera ścianę raz (obstacles, labirynt rundy, opadanie), rączka w 0,3 s, widok przez otwartą ścianę i restart, `pullAllLevers`, minimapa bez otwartej ściany ze znacznikami, promień z oka, każda dźwignia i kartka trafiona ze środka komórki, ściana przesłania a otwarta nie, `interact` raz, karta kartki (zamknięcie klawiszem i odejściem), podpowiedź o kryształach, wygrana, podpowiedzi, podświetlenie, macierze modeli. Tabela nie wlicza go do sumy 329 |
| `MenuCameraTests.cpp` | 23 | od M9, części 1: kamera menu bez okna (`game/MenuCamera.*`). Domyślne ustawienia, trasa jako zamknięta droga przez wszystkie cele, cele (kryształy i komórka przed bramą), każde przejście drzewa raz w każdą stronę, brak celów, start poza labiryntem, korytarz, pierścień, ścieżka bez ścian, odstęp od ścian (co najmniej 0,5 m), punkty na gruncie, wyszukiwanie po odległości, determinizm, stała prędkość, wysokość oczu, przesunięcie czasu, widok bez skoków (najwyżej 50 stopni na sekundę), zamknięcie pętli obu ujęć, całkowita liczba obrotów, prędkość zero, labirynt jednej komórki, pusta ścieżka, wysoki przelot nad ścianami | tamże |
| `StartOptionsTests.cpp` | 7 | od M9, części 1: przełączniki wiersza poleceń (`game/StartOptions.*`). Brak przełączników, każdy w dowolnej kolejności, `walk` i największe ziarno, nieznany przełącznik, brak wartości, wartości niezrozumiałe (w tym `-3`, `4294967296`, `inf`), lista przełączników | tamże |

Razem 329 przypadków testowych w tabeli po piątej części M7 (po szóstej 368 z tabeli: 329 plus 19 z `DiscoveryTests.cpp`, 19 z `MinimapTests.cpp` i 1 nowy w `MazeLayoutTests.cpp`; z dwoma plikami M8, 17 i 29 przypadków, daje to 414, liczbę zgłoszoną po szóstej części): tyle daje policzenie makr `TEST_CASE` w plikach tabeli, w jej kolejności (policzyłem sam: 310 po czwartej części plus 4 w `LightingTests.cpp` i 15 w `ShadowTests.cpp`). Dwa pliki testów z dalszej części M8, `RaycastTests.cpp` i `InteractablesTests.cpp`, leżą w drzewie, ale nie są częścią tej liczby i tej tabeli:
19 + 8 + 11 + 12 + 8 + 13 + 19 + 10 + 22 + 20 + 15 + 4 + 9 + 11 + 14 + 25 + 5 + 27 + 9 + 9 + 3 + 7 + 11 + 7 + 31.
Szósta część M7 (2026-10-06): bramka `make check` zgłosiła 414 przypadków i 138711 asercji (przed częścią 375 i 138506), przyrost 39 przypadków i 205 asercji, nie powtórzyłem. Surowy `grep TEST_CASE` w katalogu `tests` daje 415 linii, bo jedną z nich jest komentarz w `tests/main.cpp`.
**Stan na koniec 2026-10-06 (M9, część 1: kamera menu):** zgłoszone przez bramkę, nie powtórzyłem: `make check` przechodzi, **497 przypadków testowych i 219050 asercji** (przed częścią 467 i 158006). Przybyło 30 przypadków w dwóch nowych plikach, `MenuCameraTests.cpp` (23) i `StartOptionsTests.cpp` (7), policzone z plików (467 + 30 = 497); liczby asercji nie da się policzyć z plików, jest tylko zgłoszona. Program `night_maze_tests` ma od tej części trzydzieści pięć plików `.cpp` w `tests/` (z `main.cpp`). Liczby w akapitach poniżej to stan sprzed tej części.

**Stan na koniec 2026-10-06 (M9, część 2: warstwa menu i ekrany gry):** `make check` przechodzi (zgłoszone przez bramkę na commicie `8c99911`, nie powtórzyłem): **519 przypadków testowych i 219195 asercji** (przed częścią 497 i 219050). Liczba przypadków zgadza się z plikami: suma makr `TEST_CASE` w `tests/*.cpp` to 519 (policzyłem). Przybyło 22: 21 w nowym pliku `GameStateTests.cpp` i 1 w `StartOptionsTests.cpp` (który ma dziś 8). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona. Biblioteki `ui` (RmlUi, FreeType) program testowy nie linkuje: `game_logic` i `night_maze_tests` jej nie widzą.

**Stan po M9, części 3 (2026-10-06, HEAD `7988ae0`):** bramka na gałęzi menu (przed scaleniem z oknem debug) zgłosiła **557 przypadków testowych i 220100 asercji** (nie powtarzałem), z czego 8 przypadków to `DifficultyTests.cpp` i 17 `SettingsTests.cpp`, a pozostałe są w pięciu zmienionych plikach (`CrystalTests`, `GameStateTests`, `LightingTests`, `RoundTests`, `StartOptionsTests`). Bramka na gałęzi okna debug zgłosiła osobno 526 i 219214. **Dla scalonego drzewa żadna bramka nie zgłosiła liczb**: pełna bramka nie została na nim uruchomiona (przebieg przerwał system z braku pamięci, a właściciel zdecydował, że tego dnia go pomija). Liczby asercji nie sumuję. Program testowy ma w `CMakeLists.txt` 41 pozycji (policzone z pliku). Pozycje: `tests/main.cpp`, trzydzieści osiem plików z testami i dwa pliki `src/debug/Search.*`. **Uzupełnienie z 2026-10-06 (po M9, części 4):** pełna bramka została uruchomiona na scalonym drzewie: `make check` przeszedł na `0f8d3b9` (**564 przypadki testowe i 220119 asercji**, zgłoszone przez bramkę, nie powtarzałem). Zdanie powyżej o braku bramki dla scalonego drzewa opisuje stan sprzed tego przebiegu i zostaje jako historia.

**Stan po M9, części 4 (2026-10-06, HEAD `fc8324e`):** bramka (zgłoszone, nie powtarzałem) na gałęzi wideo: **583 przypadki testowe i 220420 asercji**, czyli 19 przypadków więcej niż na `0f8d3b9` (564 i 220119): pięć w `tests/CoverFitTests.cpp`, siedem w `tests/VideoClockTests.cpp`, sześć w `tests/MenuBackgroundTests.cpp` i jeden w `tests/StartOptionsTests.cpp` (policzone z plików: 5 + 7 + 6 + 1). Program testowy ma w `CMakeLists.txt` **46 pozycji** (policzone z pliku): `tests/main.cpp`, czterdzieści jeden plików z testami, dwa pliki `src/debug/Search.*` i dwa pliki `src/video/VideoClock.*`, kompilowane do programu testowego bezpośrednio, bo linkowanie biblioteki `video` wciągnęłoby Media Foundation ([`../modules/video/README.md`](../modules/video/README.md)).


**Stan po oknie debug (2026-10-06, HEAD `f6c6cd5`):** `make check` na gałęzi tej zmiany zgłosił **526 przypadków testowych i 219214 asercji**, czyli 7 przypadków i 19 asercji więcej niż wyżej (nie powtarzałem). Wszystkie nowe są w `tests/SearchTests.cpp` i sprawdzają czystą funkcję szukania `debug::matchesSearch` (plus `hasSearchWords`) z `src/debug/Search.*`, która nie zna ImGui, więc kompiluje się do programu testowego bez okna. Plik `tests/MinimapTests.cpp` zmienił jedną asercję: domyślny róg minimapy to teraz `BottomLeft`.

**Stan na koniec 2026-10-06 (po poprawkach kałuż, ramki minimapy i paska HUD):** zgłoszone przez bramkę, nie powtórzyłem: `make check` przechodzi, **467 przypadków testowych i 158006 asercji** (przed poprawkami 466 i 152264). Przybył jeden przypadek, w `PuddleTests.cpp` (21 zamiast 20, policzone z pliku), a liczby asercji zmieniły się także w przerobionych testach kałuż, więc różnicy 5742 asercji nie przypisuję jednemu przypadkowi. Surowy `grep TEST_CASE` w `tests/` daje 468 linii, bo jedną z nich jest komentarz w `tests/main.cpp` (policzone; 467 linii zaczyna się od `TEST_CASE`).
M8, część 2 (2026-10-06): zgłoszone przez autora kodu, nie powtórzyłem: bramka `make check` przechodzi w Debug i Release, **466 przypadków testowych i 152264 asercji** (przed częścią 445 i 150296, nowych 21 przypadków policzonych z `InteractionTests.cpp`, `InteractablesTests.cpp` zmienił tylko komentarz). Surowy `grep TEST_CASE` w `tests/` daje 467 linii, bo jedną z nich jest komentarz w `tests/main.cpp`.
M8, część 1 (2026-10-06): zgłoszone dla scalonego drzewa (po szóstej części M7), nie powtórzyłem: 445 przypadków testowych (414 plus 31: 11 w `EnvironmentMappingTests.cpp` i 20 w `PuddleTests.cpp`, policzone z plików) i 150296 asercji (138711 plus 11585 asercji tej części, potwierdzone bramką scalonego drzewa 2026-10-06). W drzewie z samą tą częścią, przed scaleniem, zgłoszono 406 i 150091 (przed częścią 375 i 138506). Podziału asercji między dwa pliki nie liczyłem.
Zgłoszone dla Windowsa po piątej części M7 (2026-10-06, tych uruchomień nie powtarzałem): bramka `make check` przechodzi, 329 przypadków i 104306 asercji. Przyrost wobec czwartej części (310 i 103751) to 19 przypadków i 555 asercji: 15 w `ShadowTests.cpp` i 4 w `LightingTests.cpp` (podziału asercji między pliki nie liczyłem).

Zgłoszone dla Windowsa po czwartej części M7 (2026-10-05, tych uruchomień nie powtarzałem):
310 przypadków i 103751 asercji, wszystkie zaliczone, bramka `make check` przechodzi. Przyrost
wobec trzeciej części M7 (294 przypadki i 102412 asercji) to 16 przypadków i 1339 asercji,
czyli dokładnie jeden nowy plik, `ShadowTests.cpp`: 129 asercji, które nie zależą od
labiryntu, i po 5 na każde z 242 pudełek kolizji labiryntu startowego (`129 + 5 * 242 = 1339`).
Zgłoszone dla Windowsa po trzeciej części M7 (2026-10-05, tych uruchomień nie powtarzałem):
294 przypadki i 102412 asercji w Debug i w Release, wszystkie zaliczone, bramka `make check`
przechodzi. Przyrost wobec drugiej części M7 (276 przypadków i 102139 asercji) to 18
przypadków i 273 asercje, czyli dokładnie dwa nowe pliki: `FogTests.cpp` (241 asercji: w jego
jedenastu przypadkach, licząc z kodu, 3, 5, 2, 3, 200, 5, 3, 9, 3, 2 i 6) i `VignetteTests.cpp`
(32 asercje: 2, 3, 2, 5, 15, 1 i 4). Przyrost drugiej części wobec pierwszej (269 przypadków
i 102103 asercje) to 7 przypadków i 36 asercji, czyli dokładnie plik `BloomTests.cpp`: w jego
siedmiu przypadkach wykonuje się 3, 3, 3, 1, 13, 8 i 5 asercji. Po drugiej części M6 było 256 przypadków i 101232 asercje (oba programy uruchomiłem
wtedy sam, 2026-10-05): dziewiętnaście plików z testami, `LightingTests.cpp` z 10 przypadkami.
Po pierwszej części M6 było 221 przypadków i 85175 asercji (siedemnaście plików z testami,
`ObjLoaderTests.cpp` z 20 przypadkami). Po M5 było 215 przypadków i 85098 asercji (szesnaście plików, `ImageLoaderTests.cpp` z 9
przypadkami): przykłady wyjścia programu niżej w tym dokumencie pochodzą z tamtego stanu.
Poprzednie stany: po M4 trzynaście plików z testami, 163 przypadki i 62220 asercji, po
pierwszej części M4 dwanaście plików, a po M2 + M3 osiem (wszystkie 2026-10-05). **Na macOS
testy nie były jeszcze budowane ani uruchamiane.** Kod, który wymaga kontekstu OpenGL
(`gfx::Mesh`, `gfx::Texture2D`, `gfx::UniformBuffer`, `gfx::Shader`, `assets::AssetCache`,
klasy rysujące, panele i HUD), testów jednostkowych nie ma.

W dokumencie są dwa rodzaje bloków C++. Blok zaczynający się komentarzem
`// Przykład, nie kod projektu.` to **przykład użycia API**. Blok poprzedzony nazwą pliku to
kod skopiowany z repozytorium. Fragmenty CMake są prawdziwe i skopiowane z repozytorium.

## 1. Czym jest doctest

doctest to biblioteka do **testów jednostkowych** (unit tests) w C++. Test jednostkowy to
mały kawałek kodu, który woła jedną funkcję programu z konkretnymi danymi i sprawdza, czy
wynik jest taki, jak powinien. Testy uruchamia się jednym poleceniem po każdej zmianie:
jeśli coś, co działało, przestało działać, dowiaduję się o tym od razu, a nie na obronie.

Co daje biblioteka:

- makra do zapisywania testów (`TEST_CASE`, `SUBCASE`) i sprawdzeń (`CHECK`, `REQUIRE`),
- gotową funkcję `main`, która znajduje wszystkie testy w programie i je uruchamia,
- czytelny raport: który plik, która linia, jakie wartości miały obie strony porównania,
- kod wyjścia programu: 0, gdy wszystko przeszło, inny w razie błędu. Na tym opiera się
  `ctest`.

**Składa się z jednego nagłówka (single header).** Cała biblioteka to plik
`doctest/doctest.h`. Nie ma niczego do skompilowania osobno. W jednym pliku `.cpp` programu
definiuje się makro, które każe nagłówkowi wygenerować także implementację (sekcja 3.1).

**Testy rejestrują się same.** Nie ma listy testów do uzupełniania. Makro `TEST_CASE` tworzy
funkcję i obiekt globalny, którego konstruktor dopisuje tę funkcję do rejestru biblioteki,
zanim ruszy `main`. Nowy test to nowy blok `TEST_CASE` w dowolnym pliku testowym.

### Za co doctest NIE odpowiada

- Nie buduje programu testowego i nie decyduje, które pliki do niego należą. To robi CMake.
- Nie jest programem `ctest`. `ctest` to osobne narzędzie z pakietu CMake, które uruchamia
  programy testowe i patrzy na ich kod wyjścia. O doctest nic nie wie.
- Nie otwiera okna i nie tworzy kontekstu OpenGL. Dlatego testami objęty jest tylko kod,
  który ich nie potrzebuje: kolizje, logika labiryntu, gracz, dwa loadery plików (OBJ i
  obrazy), a od M4 także przetwarzanie tekstu shadera w `gfx` (samo składanie napisów, bez
  kompilacji GLSL), matematyka świateł i macierz normalnych w `scene` oraz ustawienia
  oświetlenia w `game`. Od M5 dochodzą reguły rozgrywki: wyjście, kryształy i cała runda
  (`game/Exit`, `game/Crystals`, `game/Round`) oraz kule w `scene/Collider`. Klas `gfx`,
  które tworzą obiekty OpenGL, ani `NightMazeApp` w testach nie ma.
- Nie mierzy pokrycia kodu testami i niczego nie udowadnia o kodzie, którego żaden test nie
  woła.

## 2. Jak podpinamy doctest w CMake

### Pobranie biblioteki

Cały fragment z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake):

```cmake
# ---- doctest: unit tests ---------------------------------------------------------------
# doctest is a single header. Its CMake build can also compile a small static library that
# contains only main(): we write that one line ourselves in tests/main.cpp, so the library
# is switched off. So are the tests and examples of doctest itself and its install rules.
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

# The doctest target is an interface target that only carries the include path. Unlike
# GLFW and GLM it needs no extra step here: when doctest is not the main project, its own
# CMakeLists.txt already declares that path as SYSTEM, so the header cannot produce
# warnings in our tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)
```

Mechanizm FetchContent (`FetchContent_Declare`, `FetchContent_MakeAvailable`, `GIT_SHALLOW`,
zapis `CACHE BOOL "" FORCE`, powód przypinania wersji) jest opisany w
[`glfw.md`](glfw.md), sekcja 2. Tutaj tylko to, co dla doctest jest inne.

Repozytorium doctest ma własny `CMakeLists.txt`
(`build/debug/_deps/doctest-src/CMakeLists.txt`), więc `FetchContent_MakeAvailable(doctest)`
dołącza go jak `add_subdirectory`. Plik definiuje:

| Target | Alias | Rodzaj | Co zawiera |
|---|---|---|---|
| `doctest` | `doctest::doctest` | `INTERFACE` | tylko ścieżki nagłówków. Niczego nie kompiluje |
| `doctest_with_main` | `doctest::doctest_with_main` | biblioteka statyczna | plik `doctest/doctest.cpp` skompilowany z makrem `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, czyli gotowe `main`. Powstaje tylko przy `DOCTEST_WITH_MAIN_IN_STATIC_LIB` równym `ON` |

Trzy opcje:

| Opcja | Co robi | Wartość domyślna w doctest 2.5.3 | U nas |
|---|---|---|---|
| `DOCTEST_WITH_MAIN_IN_STATIC_LIB` | buduje bibliotekę `doctest_with_main` | `ON` | `OFF` |
| `DOCTEST_WITH_TESTS` | buduje testy i przykłady samego doctest | `ON` tylko gdy doctest jest projektem głównym | `OFF` |
| `DOCTEST_NO_INSTALL` | pomija reguły `install` | `OFF` | `ON` |

Realnie coś zmienia pierwsza i trzecia. Bez pierwszej każdy build kompilowałby bibliotekę,
której nikt nie linkuje, i to cudzy kod poza naszymi ustawieniami ostrzeżeń. Funkcję `main`
wolę mieć we własnym pliku `tests/main.cpp`: to jedna linia, którą widać i którą umiem
wyjaśnić (sekcja 3.1). Druga opcja i tak miałaby u nas wartość `OFF`. Ustawiam ją jawnie z
tego samego powodu co przy GLFW i GLM: żeby było widać intencję i żeby wynik nie zależał od
wartości domyślnych przyszłej wersji.

Linie `set(...)` muszą stać **przed** `FetchContent_MakeAvailable(doctest)`, bo doctest czyta
te opcje w chwili dołączenia.

### Nagłówek jako systemowy: tym razem bez naszego kroku

Przy GLFW i GLM sami kopiujemy listę `INTERFACE_INCLUDE_DIRECTORIES` do
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` ([`glm.md`](glm.md), sekcja 2). Przy doctest ten krok
jest zbędny. Jego `CMakeLists.txt` sprawdza, czy jest projektem głównym, i jeśli nie jest,
sam deklaruje ścieżki ze słowem `SYSTEM`:

```cmake
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/doctest/>
)
```

To fragment pliku doctest, nie naszego. Ścieżki są dwie: korzeń repozytorium (stąd zapis
`#include <doctest/doctest.h>`) i jego podkatalog `doctest/` (zadziałałoby też
`#include <doctest.h>`, którego nie używam).

Zmierzone na Windowsie w wygenerowanym projekcie `night_maze_tests.vcxproj`: oba katalogi
trafiają do kompilatora przez `/external:I`, a `ExternalWarningLevel` ma wartość
`TurnOffAllWarnings`. Pod `/W4 /permissive-` z nagłówka doctest i z rozwinięć jego makr w
naszych plikach testowych nie pojawia się żadne ostrzeżenie. Na macOS oczekuję flagi
`-isystem`, tak jak dla GLFW i GLM. Tego nie mierzyłem.

Plik doctest zaczyna się od `cmake_minimum_required(VERSION 3.14)`. Ma to znaczenie dla
Maca: CMake 4 odrzuca projekty, które deklarują minimum poniżej 3.5. Wersja 3.14 jest
powyżej tej granicy, więc konfiguracja w CMake 4.3 nie powinna mieć z tym problemu. To
wniosek z lektury pliku, nie pomiar.

### Program testowy

Fragmenty z [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# ---- game_logic: the rules of Night Maze that need no window and no OpenGL -------------
# The maze, its generator, its layout in the world, the player, the settings of the
# lighting and the rules of a round (exit, crystals, battery) are plain data and math.
# They live in a library of their own, and not in the night_maze executable, so that
# the test program can link them too: a test cannot link code that is inside another
# executable.
add_library(game_logic STATIC
    src/game/Crystals.cpp
    src/game/Crystals.hpp
    src/game/Exit.cpp
    src/game/Exit.hpp
    src/game/Lighting.cpp
    src/game/Lighting.hpp
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
    src/game/MazeWorld.cpp
    src/game/MazeWorld.hpp
    src/game/Player.cpp
    src/game/Player.hpp
    src/game/Round.cpp
    src/game/Round.hpp
)
# PUBLIC: the headers of this library (Exit.hpp, Lighting.hpp, MazeLayout.hpp,
# MazeWorld.hpp, Player.hpp, Round.hpp) include headers of engine (scene/Collider.hpp, scene/Light.hpp) and GLM,
# so whoever includes them needs the include paths of engine.
# The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)
```

Testy mogą wołać tylko kod, który da się do nich **dolinkować**, czyli kod z biblioteki. Kod
skompilowany wprost w programie `night_maze` jest dla innego programu niedostępny. Dlatego
logika labiryntu trafiła do osobnej biblioteki statycznej `game_logic`, którą linkują i gra,
i testy ([`../modules/game/README.md`](../modules/game/README.md), sekcja 3). Kolizje są w
`engine`, które `game_logic` linkuje jako `PUBLIC`, więc testy dostają je razem z nią.

Z oświetleniem (M4) podział jest ten sam. Kod bez okna, który ma mieć testy, trafił do
bibliotek: `gfx/ShaderSource`, `scene/Light`, `scene/LightBlock` i `scene::normalMatrix`
(`scene/Transform`) są w `engine`, a `game/Lighting` w `game_logic`. Kod, który woła OpenGL
(`gfx::UniformBuffer`, `game::LightRig`, panel Lights), testów nie ma. Z mapami normalnych tak
samo: matematyka stycznych (`assets/Tangents`) jest w `engine` i ma własny plik testów, a
funkcja `usesNormalMap` jest w `game/Lighting`. Kod GLSL (`common/normal_map.glsl`) i wiązanie
drugiej tekstury testów nie mają.

Rozgrywka (M5) jest podzielona tak samo, i to z myślą o testach. Reguły są zwykłymi danymi
i wolnymi funkcjami w `game_logic`: `game/Exit` (gdzie jest wyjście), `game/Crystals` (ile
kryształów, gdzie stoją i jak się ruszają) i `game/Round` (stan rundy i jeden krok reguł,
`updateRound`). Żadna z nich nie dołącza OpenGL ani `core::Input`: `updateRound` dostaje
pozycję gracza, przełącznik latarki i czas kroku jako argumenty. Dzięki temu
`tests/RoundTests.cpp` rozgrywa rundę od początku do wygranej bez okna: buduje labirynt z
ziarna, w pętli woła `updateRound` z pozycją pod kolejnymi kryształami i sprawdza baterię,
bramę i stan rundy. Kule (`scene::Sphere` i testy `overlaps`) są w `engine`. To, co rysuje
rundę i ją pokazuje (`game::GameplayRenderer`, `game::drawModel`, HUD, panel Gameplay,
uniform `uEmissive` w shaderach) oraz spięcie w `NightMazeApp` (klawisz R, kolejność w
`onUpdate`), testów nie ma.

```cmake
# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/BloomTests.cpp
    tests/ColliderTests.cpp
    tests/ColorSpaceTests.cpp
    tests/CrystalTests.cpp
    tests/ExitTests.cpp
    tests/FogTests.cpp
    tests/FramebufferTests.cpp
    tests/GrassTests.cpp
    tests/ImageLoaderTests.cpp
    tests/LightTests.cpp
    tests/LightingTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/MazeWorldTests.cpp
    tests/ObjLoaderTests.cpp
    tests/PlayerTests.cpp
    tests/RoundTests.cpp
    tests/ShaderSourceTests.cpp
    tests/SkyboxTests.cpp
    tests/TangentTests.cpp
    tests/TerrainTests.cpp
    tests/TransformTests.cpp
    tests/VignetteTests.cpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests (of images and of OBJ models) read the real files of the game. A test
# must not depend on the directory it is started from, so the absolute path of assets/ in
# the repository is compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)
```

| Linia | Znaczenie |
|---|---|
| `enable_testing()` | włącza obsługę testów w CMake: podczas generowania powstaje w katalogu buildu plik z listą testów, który czyta `ctest`. Musi stać w głównym `CMakeLists.txt`, bo `ctest` szuka listy w korzeniu katalogu buildu |
| `add_executable(night_maze_tests ...)` | zwykły program z dwudziestu sześciu plików (`tests/main.cpp` i dwadzieścia pięć plików z testami). Nie ma słowa `EXCLUDE_FROM_ALL`, więc buduje go każde `cmake --build --preset debug`. Dzięki temu testy zawsze się kompilują: zmiana w API, która je psuje, wychodzi przy pierwszym buildzie |
| `target_link_libraries(... PRIVATE game_logic doctest::doctest)` | kod testowany i biblioteka testów. "Linkowanie" targetu `INTERFACE` `doctest::doctest` oznacza tylko dodanie ścieżek nagłówków |
| `night_maze_enable_warnings(night_maze_tests)` | testy kompilują się z tymi samymi ścisłymi ostrzeżeniami co reszta naszego kodu (`/W4 /permissive-` albo `-Wall -Wextra -Wpedantic`) |
| `target_compile_definitions(night_maze_tests PRIVATE NIGHT_MAZE_ASSETS_DIR="...")` | makro preprocesora z bezwzględną ścieżką katalogu `assets` w repozytorium. Testy loaderów czytają nim prawdziwe modele i tekstury niezależnie od katalogu, z którego uruchomiono program ([`../modules/assets/images.md`](../modules/assets/images.md), sekcja 5.7) |
| `add_test(NAME night_maze_tests COMMAND night_maze_tests)` | rejestruje **jeden** test CTest: "uruchom ten program". `COMMAND` z nazwą targetu CMake zamienia na pełną ścieżkę pliku wykonywalnego, także z podkatalogiem `Debug\` generatora Visual Studio |

Dla `ctest` cały program jest jednym testem: przechodzi, gdy kod wyjścia to 0. Liczbę
przypadków widać dopiero w wyjściu samego programu (sekcja 4). doctest ma moduł CMake, który
rejestruje każdy `TEST_CASE` jako osobny test CTest (`doctest_discover_tests`). Nie używam
go: jedna linia `add_test` jest prostsza, a szczegóły i tak pokazuje `--output-on-failure`.

Program testowy linkuje przez `engine` także GLFW i GLAD, choć żaden test ich nie woła. To
koszt tego, że `scene/Collider` leży w tej samej bibliotece co okno. Program nie tworzy okna
i nie potrzebuje karty graficznej.

## 3. Najważniejsze API

### 3.1. `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`: punkt wejścia

Cały plik [`tests/main.cpp`](../../tests/main.cpp):

```cpp
// Entry point of the test program night_maze_tests.
// See docs/libraries/doctest.md

// doctest is a single header. In exactly one .cpp file of the program this macro makes
// the header also emit its implementation and a main() function that runs every
// TEST_CASE of every file. The other test files include the header without the macro.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```

Nagłówek `doctest.h` ma dwie części. Zwykłe dołączenie daje tylko deklaracje i makra. Gdy
przed dołączeniem zdefiniowane jest `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, nagłówek dokłada
definicje wszystkich funkcji biblioteki i funkcję `main`. To musi się stać w **dokładnie
jednym** pliku `.cpp` programu: w zerze plików linker nie znajdzie `main`, w dwóch znajdzie
dwie definicje tej samej funkcji.

`#define` stoi przed `#include`, bo preprocesor czyta plik od góry: makro musi już istnieć,
gdy nagłówek sprawdza, czy je zdefiniowano.

### 3.2. `TEST_CASE` i `CHECK`

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("contains tells cells of the maze from everything else") {
    const game::Maze maze(3, 2);

    CHECK(maze.contains(0, 0));
    CHECK(maze.contains(2, 1));
    CHECK_FALSE(maze.contains(-1, 0));
    CHECK_FALSE(maze.contains(0, -1));
    CHECK_FALSE(maze.contains(3, 0));
    CHECK_FALSE(maze.contains(0, 2));
}
```

| Makro | Znaczenie |
|---|---|
| `TEST_CASE("nazwa")` | jeden **przypadek testowy** (test case): funkcja z nazwą w postaci zdania. Nazwa pojawia się w raporcie, więc ma mówić, co ma być prawdą |
| `CHECK(wyrażenie)` | **asercja** (assertion): sprawdza, że wyrażenie jest prawdą. Gdy nie jest, zapisuje błąd i **idzie dalej**, więc jeden przebieg pokazuje wszystkie nieudane sprawdzenia |
| `CHECK_FALSE(wyrażenie)` | sprawdza, że wyrażenie jest fałszem |

Nazwy testów są po angielsku, tak jak identyfikatory i komentarze w kodzie.

doctest rozkłada wyrażenie z porównaniem na lewą i prawą stronę i w razie błędu wypisuje
obie wartości. Tak wyglądał prawdziwy komunikat z chwili, gdy test oczekiwał jeszcze złych
liczb (ścieżka pliku skrócona):

```text
tests\MazeGeneratorTests.cpp(186): ERROR: CHECK( game::randomBelow(generator, BOUND) == expected ) is NOT correct!
  values: CHECK( 1 == 0 )
```

Pierwsza linia podaje plik, numer linii i tekst wyrażenia, druga wartości obu stron.

### 3.3. `REQUIRE`: gdy dalej nie ma sensu iść

`tests/MazeLayoutTests.cpp`:

```cpp
    REQUIRE(segments.size() == 4U);
    // North and south walls run along X, west and east walls along Z.
    CHECK(countSegments(segments, {1.0F, 0.0F, 0.0F}, game::WallAxis::AlongX) == 1);
```

`REQUIRE` działa jak `CHECK`, ale po nieudanym sprawdzeniu **przerywa cały przypadek
testowy** (rzuca wyjątek, który doctest łapie). Używam go, gdy dalsze sprawdzenia nie
miałyby sensu albo byłyby niebezpieczne: jeśli lista ma złą długość, sięganie do jej
elementów jest bez znaczenia. `REQUIRE_FALSE` to wersja dla fałszu.

Zasada: `CHECK` domyślnie, `REQUIRE` dla warunków, od których zależy reszta testu.

### 3.4. `SUBCASE`: wspólny początek, kilka zakończeń

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("removing a wall removes it for both cells that share it") {
    game::Maze maze(3, 2);

    SUBCASE("east side of a cell is the west side of its right neighbour") {
        maze.removeWall(0, 0, game::Direction::East);
        CHECK_FALSE(maze.hasWall(0, 0, game::Direction::East));
        CHECK_FALSE(maze.hasWall(1, 0, game::Direction::West));
    }

    SUBCASE("north side of a cell is the south side of the cell above it") {
        maze.removeWall(2, 1, game::Direction::North);
        CHECK_FALSE(maze.hasWall(2, 1, game::Direction::North));
        CHECK_FALSE(maze.hasWall(2, 0, game::Direction::South));
    }
```

(dalej w pliku są jeszcze dwa podprzypadki tego samego testu).

**Podprzypadek** (subcase) to gałąź wewnątrz przypadku testowego. doctest uruchamia cały
`TEST_CASE` **od początku osobno dla każdego podprzypadku**: za pierwszym razem wchodzi
tylko do pierwszego bloku `SUBCASE`, za drugim tylko do drugiego. Kod przed blokami
(tutaj `game::Maze maze(3, 2);`) wykonuje się więc za każdym razem na nowo.

Skutek: każdy podprzypadek dostaje **świeży labirynt** ze wszystkimi ścianami. Usunięcie
ściany w pierwszym nie wpływa na drugi. To zastępuje osobne funkcje "przygotuj" i "posprzątaj"
znane z innych bibliotek testowych.

### 3.5. `doctest::Approx`: porównywanie liczb zmiennoprzecinkowych

`tests/ColliderTests.cpp`:

```cpp
void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}
```

Liczby `float` nie przechowują większości ułamków dziesiętnych dokładnie: `0.1F + 0.2F` nie
jest równe `0.3F` co do bitu. Porównanie przez `==` zawodziłoby więc w poprawnym kodzie.
`doctest::Approx(wartość)` tworzy obiekt, dla którego `==` znaczy "równe z dokładnością do
małego błędu względnego". Domyślna tolerancja pokrywa zwykłe błędy zaokrągleń, a zmienia się
ją metodą `.epsilon(...)`. Używa jej jeden plik, `tests/TransformTests.cpp`, którego własna
funkcja `checkVector` porównuje składowe przez `doctest::Approx(expected.x).epsilon(0.0001)`.
Pozostałe testy zostają przy tolerancji domyślnej.

`checkVector` to zwykła funkcja pomocnicza, nie element biblioteki. Makra `CHECK` wolno
wołać z funkcji pomocniczych. W raporcie błędu jest wtedy linia wewnątrz `checkVector`, a nie
linia testu, który ją zawołał, i to jest cena tej wygody.

Zwykłego `==` na liczbach `float` używam w testach w dwóch miejscach świadomie: gdy wartość
ma przejść przez funkcję **bez zmiany** (`REQUIRE(allowed.z == step.z)`) i gdy liczby są
sumami i iloczynami 0,5, 1 i 2, które `float` przechowuje dokładnie (pozycje ścian).

### 3.6. `CHECK_THROWS_AS` i `CHECK_NOTHROW`: wyjątki

`tests/MazeTests.cpp`:

```cpp
TEST_CASE("a maze with a wrong size cannot be created") {
    CHECK_THROWS_AS(game::Maze(0, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, 0), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(-3, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(game::Maze::MAX_SIZE + 1, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, game::Maze::MAX_SIZE + 1), std::invalid_argument);

    // The limits themselves are fine.
    CHECK_NOTHROW(game::Maze(1, 1));
    CHECK_NOTHROW(game::Maze(game::Maze::MAX_SIZE, game::Maze::MAX_SIZE));
}
```

| Makro | Znaczenie |
|---|---|
| `CHECK_THROWS_AS(wyrażenie, Typ)` | wyrażenie ma rzucić wyjątek typu `Typ` (albo pochodnego). Brak wyjątku i wyjątek innego typu to błąd testu |
| `CHECK_NOTHROW(wyrażenie)` | wyrażenie nie może rzucić żadnego wyjątku |

Tak testuje się obsługę złych danych: nie wystarczy, że dobry rozmiar działa, zły ma być
odrzucony w określony sposób.

### 3.7. `CAPTURE`: który obrót pętli zawiódł

`tests/MazeGeneratorTests.cpp`:

```cpp
    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size.width, size.height, seed);
```

Asercja w pętli wykonuje się setki razy z różnymi danymi. Gdy zawiedzie, sam komunikat
"oczekiwano 15, jest 14" nie mówi, dla którego labiryntu. `CAPTURE(zmienna)` zapamiętuje
nazwę i wartość zmiennej i dopisuje je do komunikatu **każdej nieudanej** asercji w tym
samym bloku. Przy sukcesie niczego nie wypisuje. Z raportu wiadomo wtedy od razu: szerokość
5, wysokość 3, ziarno 17.

### 3.8. Czego nie używam

| Element | Do czego służy | Dlaczego go nie ma |
|---|---|---|
| `TEST_SUITE` | grupowanie przypadków w nazwane zestawy | pliki są małe i tematyczne, nazwa pliku wystarcza |
| `TEST_CASE_FIXTURE` | klasa ze wspólnym stanem dla testów | `SUBCASE` robi to samo prościej |
| `WARN(...)` | sprawdzenie, które tylko ostrzega | test ma przechodzić albo nie |
| `doctest_discover_tests` (CMake) | osobny test CTest na każdy `TEST_CASE` | jedna linia `add_test` wystarcza (sekcja 2) |
| `DOCTEST_CONFIG_DISABLE` | usuwa testy z kompilacji | testy są w osobnym programie, w grze nie ma ich wcale |

## 4. Jak uruchamiać testy

Program testowy buduje się razem z całym projektem. Polecenia wykonuje się w katalogu
głównym repozytorium, po konfiguracji i buildzie
([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 2,
[`../guides/build-macos.md`](../guides/build-macos.md), sekcja 2).

### Przez `ctest`

Windows (w terminalu ze środowiskiem deweloperskim) i macOS, te same polecenia:

```sh
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```

| Argument | Znaczenie |
|---|---|
| `--test-dir build/debug` | katalog buildu, w którym `ctest` ma szukać listy testów |
| `-C Debug` | konfiguracja do przetestowania. **Wymagana z generatorem Visual Studio**, bo jeden katalog buildu mieści tam kilka konfiguracji i `ctest` musi wiedzieć, który program uruchomić. Generatory jednokonfiguracyjne (Unix Makefiles na Macu, Ninja) ten argument ignorują, więc polecenie może być wspólne |
| `--output-on-failure` | gdy test nie przejdzie, `ctest` wypisuje całe wyjście programu testowego, czyli raport doctest. Bez tego widać tylko słowo `Failed` |

Wynik zmierzony na Windowsie (Debug, 2026-10-05, przed dodaniem testów oświetlenia):

```text
    Start 1: night_maze_tests
1/1 Test #1: night_maze_tests .................   Passed    0.40 sec

100% tests passed, 0 tests failed out of 1
```

"1 test" to cały program (sekcja 2). Kod wyjścia `ctest` to 0. W Release ten sam test trwał
wtedy około 0,1 s. Dla programu po M5 z 215 przypadkami (2026-10-05) znane są liczby
z raportu doctest niżej. Program po drugiej części M6 kończył raport liniami
`test cases: 256 | 256 passed` i `assertions: 101232 | 101232 passed`. Po pierwszej części M7 zgłoszone były liczby 269 i 102103, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751, a dla dzisiejszego stanu (po piątej części M7, 2026-10-06) 329 i 104306. Wyjścia `ctest` z tego dnia nie zapisałem, więc blok wyżej zostaje z
datą swojego pomiaru: jego postać się nie zmienia, inny może być tylko czas.

W pliku [`Makefile`](../../Makefile) są do tego skróty: `make test` (build Debug i testy),
`make test-release`, a `make check` uruchamia oba razem z resztą kontroli
([`../guides/project-structure.md`](../guides/project-structure.md), sekcja 3.12). Na
Windowsie skróty działają: jest tam GNU Make 4.4.1 (scoop), `make test` przeszedł naprawdę,
a bramka `make check` jest zgłaszana jako zielona w każdej części
([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 3). Na Macu skrótów nikt
jeszcze nie uruchomił, bo ten kod nie był tam jeszcze budowany.

### Bezpośrednio

Program testowy można uruchomić samodzielnie. Wtedy widać raport doctest:

```bat
build\debug\Debug\night_maze_tests.exe
```

```sh
./build/debug/night_maze_tests
```

Wynik z Windowsa po M5, 2026-10-05 (te same liczby w Debug i w Release):

```text
[doctest] doctest version is "2.5.3"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:   215 |   215 passed | 0 failed | 0 skipped
[doctest] assertions: 85098 | 85098 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Z tego uruchomienia znane są obie liczby i to, że wszystkie testy przeszły. Postać raportu
(nagłówek, kreska, odstępy przed liczbami) odtworzyłem z wcześniejszego raportu z
2026-10-05, który pokazywał 163 przypadki i 62220 asercji: doctest wyrównuje obie liczby do
szerokości dłuższej z nich.

Nad tym raportem program wypisuje kilka linii `[error]`: pochodzą z testów, które celowo
podają loaderom zły plik, i nie oznaczają nieudanego testu.

Asercji jest dużo więcej niż przypadków, bo wiele z nich stoi w pętlach: własności labiryntu
są sprawdzane dla 200 labiryntów, komórka po komórce. Testy z M5 robią to samo dla wyjścia
i kryształów (pętle po ziarnach w `ExitTests.cpp` i `CrystalTests.cpp`).

Przydatne opcje programu (pełna lista: `--help`):

| Opcja | Co robi |
|---|---|
| `-ltc` albo `--list-test-cases` | wypisuje nazwy wszystkich przypadków, niczego nie uruchamia |
| `-tc="golden*"` albo `--test-case="golden*"` | uruchamia tylko przypadki o pasującej nazwie. `*` zastępuje dowolny tekst. Zmierzone, gdy program miał mniej przypadków: `-tc="golden*"` uruchamia 1 przypadek i pomija wszystkie pozostałe |
| `-sf="*Collider*"` albo `--source-file=...` | filtruje po nazwie pliku źródłowego z testami |
| `-s` albo `--success` | wypisuje także asercje, które przeszły |
| `-d` albo `--duration` | wypisuje czas każdego przypadku |

### Kiedy uruchamiać

Po każdej zmianie w `src/scene/Collider.*`, `src/game/Maze*`, `src/game/Player.*`, w
loaderach z `src/assets/`, a od M4 także w `src/gfx/ShaderSource.*`, `src/scene/Light.*`,
`src/scene/LightBlock.*`, `src/scene/Transform.*` i `src/game/Lighting.*`, przed każdym commitem (na
Macu robi to `make check`) i po przejściu na drugi system. To ostatnie ma tu szczególne
znaczenie: test labiryntu wzorcowego istnieje po to, żeby wykryć różnicę między macOS a
Windowsem ([`../modules/game/maze-generator.md`](../modules/game/maze-generator.md),
sekcja 5.8).

## 5. Pułapki

1. **Makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` w dwóch plikach albo w żadnym.** W dwóch:
   błąd linkera o podwójnej definicji `main` i funkcji doctest. W żadnym: błąd linkera o
   braku `main`. Makro jest w `tests/main.cpp` i tylko tam.
2. **Nowy plik testowy bez wpisu w `CMakeLists.txt`.** Lista plików jest jawna. Plik, którego
   nie ma w `add_executable(night_maze_tests ...)`, nie jest kompilowany, a jego testy po
   cichu nie istnieją: raport pokazuje mniej przypadków, ale nadal `SUCCESS`. Po dodaniu
   pliku warto sprawdzić, czy liczba przypadków wzrosła.
3. **`ctest` bez `-C` z generatorem Visual Studio.** `ctest` nie wie wtedy, którą konfigurację
   uruchomić, i test nie jest wykonywany. Zmierzone na Windowsie: komunikat
   `Test not available without configuration.  (Missing "-C <config>"?)`, wynik
   `***Not Run` i kod wyjścia 8. Zawsze `-C Debug` albo `-C Release`.
4. **`ctest` uruchamia to, co jest zbudowane.** `ctest` niczego nie buduje. Po zmianie kodu
   trzeba najpierw wykonać `cmake --build`, inaczej testowany jest stary program.
5. **`==` na liczbach `float`.** Poprawny kod nie przechodzi testu przez błąd zaokrąglenia na
   ostatniej cyfrze. Do wyników obliczeń zmiennoprzecinkowych służy `doctest::Approx`
   (sekcja 3.5).
6. **Sumowanie w pętli.** Po 2000 dodawań `float` suma różni się od iloczynu na tyle, że
   nawet `Approx` z domyślną tolerancją zgłasza błąd (zmierzone przy pisaniu testów: 30,2004
   zamiast 30,2). Lepiej sprawdzać pojedynczy krok niż sumę wielu.
7. **Różne typy po obu stronach porównania.** `CHECK(vector.size() == 4)` porównuje liczbę
   bez znaku z liczbą ze znakiem wewnątrz szablonów doctest, co pod ścisłymi ostrzeżeniami
   może dać ostrzeżenie o mieszaniu znaków. W testach stała ma przyrostek `U` (`4U`) albo
   typ `std::size_t`.
8. **`REQUIRE` w funkcji pomocniczej albo w destruktorze.** `REQUIRE` przerywa test
   wyjątkiem. W zwykłej funkcji pomocniczej działa, ale w destruktorze i w funkcji
   `noexcept` kończy program.
9. **Stan wspólny między podprzypadkami.** Zmienna zadeklarowana przed blokami `SUBCASE`
   jest tworzona od nowa dla każdego z nich, ale zmienna globalna albo statyczna nie.
   Testy nie mają żadnego stanu globalnego: generator liczb losowych jest zawsze zmienną
   lokalną z jawnym ziarnem.
10. **Asercja w ciasnej pętli.** Każde `CHECK` jest liczone i kosztuje czas. Test wędrówki po
    labiryncie miał najpierw asercję dla każdej przeszkody w każdym kroku, co dawało prawie
    4 miliony asercji. Teraz zbiera wynik do jednej zmiennej `bool` i sprawdza ją raz na
    serię kroków.
11. **Nazwa testu jako filtr.** Opcja `-tc` traktuje przecinek jako separator wzorców, więc
    nazwa z przecinkiem wymaga poprzedzenia go ukośnikiem wstecznym. Prościej filtrować
    początkiem nazwy z gwiazdką.
12. **Testy nie obejmują niczego z OpenGL.** Zielony wynik testów mówi o kolizjach,
    labiryncie, graczu, loaderach plików, matematyce świateł, stycznych, składaniu tekstu
    shadera i, od M5, o regułach rundy (wyjście, kryształy, bateria, brama, wygrana). O
    kompilacji shaderów, buforach i rysowaniu nie mówi nic: te rzeczy sprawdza się
    uruchomieniem programu. Dwa przykłady z M4. `tests/ShaderSourceTests.cpp` sprawdza, że
    `#include` jest zastępowany treścią pliku i że numer w komunikacie błędu zamienia się w
    nazwę pliku, ale żaden test nie podaje tego tekstu kompilatorowi GLSL: format błędów
    sterownika Apple (`ERROR: 1:15:`) jest w testach wpisanym napisem, a nie wyjściem
    prawdziwego sterownika. `tests/LightTests.cpp` sprawdza przesunięcia pól struktury
    `scene::LightBlockData`, ale tego, czy sterownik układa blok `LightBlock` w tylu samych
    bajtach, pilnuje dopiero porównanie rozmiarów w działającej grze
    (`Shader::bindUniformBlock`). Przykład z M5: `tests/RoundTests.cpp` sprawdza, że pusta
    bateria wyłącza latarkę i że brama przestaje blokować po zebraniu dość kryształów, ale
    tego, czy kryształ naprawdę świeci na ekranie, czy brama opada w grunt i czy HUD
    pokazuje właściwe liczby, żaden test nie widzi.

## 6. Pytania kontrolne

1. **Co to jest test jednostkowy i po co go pisać?**
   Mały kawałek kodu, który woła jedną funkcję z konkretnymi danymi i sprawdza wynik.
   Uruchamiany po każdej zmianie wykrywa od razu, że coś, co działało, przestało działać.

2. **Co znaczy, że doctest jest biblioteką z jednego nagłówka, i skąd bierze się `main`?**
   Cała biblioteka to plik `doctest/doctest.h`. W jednym pliku `.cpp` (`tests/main.cpp`)
   przed dołączeniem nagłówka zdefiniowane jest makro `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`,
   które każe nagłówkowi wygenerować implementację i funkcję `main`.

3. **Skąd program wie, jakie testy ma uruchomić?**
   Makro `TEST_CASE` tworzy funkcję i obiekt globalny, który przed startem `main` dopisuje
   ją do rejestru biblioteki. Nie ma ręcznej listy testów.

4. **Czym różni się `CHECK` od `REQUIRE`?**
   Po nieudanym `CHECK` test idzie dalej, po nieudanym `REQUIRE` cały przypadek testowy jest
   przerywany. `REQUIRE` jest dla warunków, bez których reszta testu nie ma sensu.

5. **Jak działa `SUBCASE`?**
   Przypadek testowy jest uruchamiany od początku osobno dla każdego podprzypadku, za każdym
   razem z wejściem tylko do jednego bloku. Kod przed blokami wykonuje się za każdym razem,
   więc każdy podprzypadek dostaje świeże dane.

6. **Po co `doctest::Approx`?**
   Liczby `float` mają błędy zaokrągleń, więc `==` zawodzi w poprawnym kodzie. `Approx`
   porównuje z małą tolerancją względną.

7. **Dlaczego logika labiryntu jest w bibliotece `game_logic`?**
   Program testowy może dolinkować tylko kod z biblioteki. Kod skompilowany wprost w
   programie `night_maze` byłby dla testów niedostępny.

8. **Co robią `enable_testing()` i `add_test`?**
   `enable_testing()` włącza zapis listy testów do katalogu buildu. `add_test` dopisuje do
   niej jeden test: uruchomienie programu `night_maze_tests`. Program `ctest` czyta listę,
   uruchamia program i uznaje test za zaliczony, gdy kod wyjścia to 0.

9. **Po co `-C Debug` w poleceniu `ctest`?**
   Generator Visual Studio trzyma w jednym katalogu buildu kilka konfiguracji, więc `ctest`
   musi wiedzieć, którą uruchomić. Generatory jednokonfiguracyjne ignorują ten argument.

10. **Dlaczego nagłówek doctest nie daje ostrzeżeń pod `/W4`?**
    Jest dołączany jako nagłówek systemowy: `CMakeLists.txt` doctest deklaruje ścieżki ze
    słowem `SYSTEM`, gdy doctest nie jest projektem głównym. Na Windowsie daje to
    `/external:I` z wyłączonymi ostrzeżeniami.

11. **Czego testy w tym projekcie nie sprawdzają?**
    Niczego, co potrzebuje okna albo kontekstu OpenGL: klas `gfx` tworzących obiekty
    OpenGL, kompilacji shaderów, rysowania, sterowania, HUD i paneli. Sprawdzają kod, który
    jest samą matematyką, logiką i pracą na tekście (w `gfx` to jeden plik, `ShaderSource`).

12. **Jak da się przetestować całą rundę gry bez okna?**
    Reguły rundy są wolnymi funkcjami w bibliotece `game_logic` i dostają wszystko w
    argumentach: `updateRound(round, world, settings, feetPosition, flashlightOn,
    stepSeconds)`. Test sam podaje pozycję gracza i czas kroku, więc nie potrzebuje ani
    klawiatury, ani pętli głównej, ani OpenGL. `tests/RoundTests.cpp` buduje labirynt z
    ziarna, "stawia" gracza pod kolejnymi kryształami i sprawdza baterię, bramę i wygraną.

## 7. Oficjalna dokumentacja

- Repozytorium doctest: <https://github.com/doctest/doctest>
- Samouczek: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/tutorial.md>
- Asercje (w tym `Approx` i makra wyjątków): <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/assertions.md>
- Przypadki testowe i podprzypadki: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/testcases.md>
- Opcje wiersza poleceń: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/commandline.md>
- Własna funkcja `main` i makra konfiguracji: <https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/main.md>
- Dokumentacja CMake, `enable_testing`, `add_test` i program `ctest`: <https://cmake.org/cmake/help/latest/command/add_test.html>, <https://cmake.org/cmake/help/latest/manual/ctest.1.html>
- Kopia dokładnie dla naszej wersji leży po pierwszej konfiguracji w
  `build/debug/_deps/doctest-src/`: katalog `doc/markdown/` (te same dokumenty), plik
  `CMakeLists.txt` (targety i opcje z sekcji 2) i sam nagłówek `doctest/doctest.h`.
