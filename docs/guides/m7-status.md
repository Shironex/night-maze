# Stan M7 na koniec 2026-10-06

Krótka ściąga: co z kamienia milowego M7 jest w kodzie, co zostało, jakie decyzje już zapadły dla brakującej części i w jakiej kolejności sprawdzać wszystko na Macu. Pierwsza wersja powstała na koniec 2026-10-05 (cztery części z sześciu), ta jest z końca 2026-10-06 (pięć części). Daty przy decyzjach i liczbach mówią, kiedy co zapadło. Szczegóły są w dokumentach, do których prowadzą odnośniki. Ten plik nie zastępuje list kontrolnych w [`build-windows.md`](build-windows.md) i [`build-macos.md`](build-macos.md).

**Jednym zdaniem:** pięć części z sześciu jest kompletnych w kodzie na Windowsie, **żadna nie jest zamknięta** (testy ręczne i macOS są otwarte dla wszystkich), tagu nie ma. Brakuje części szóstej, minimapy: jej kodu nie ma, a dwie decyzje o niej zapadły (sekcja 3).

Poza M7 istnieją w kodzie bezokienkowe fundamenty M8 (rzutowanie promieni, rozmieszczanie dźwigni i notatek). Nie są częścią M7 i są opisane osobno, w swoich dokumentach: [`../modules/scene/picking.md`](../modules/scene/picking.md) i [`../modules/game/interactables.md`](../modules/game/interactables.md).

## 1. Co jest zbudowane

| Część | Co daje | Commity | Dokument | Lista kontrolna |
|---|---|---|---|---|
| 1. Bufor HDR i gamma | scena rysowana do framebuffera `GL_RGBA16F` z teksturą głębi, przebieg składający z ekspozycją i trzema krzywymi mapowania tonów, tekstury sRGB, kodowanie na końcu klatki, panel Framebuffers | `0c76316` `feat(gfx): add colour space conversion between srgb and linear`, `56f5832` `feat(gfx): add a framebuffer class with colour and depth textures`, `8f87850` `feat(game): render the scene to an hdr buffer with tone mapping and gamma` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md), [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) | [`build-windows.md`](build-windows.md), sekcja 17 |
| 2. Bloom | przebieg jasności i rozdzielne rozmycie Gaussa w trzech celach o połowie rozdzielczości, dodane przed mapowaniem tonów | `bfc70f3` `feat(game): add bloom from a half resolution bright pass and gaussian blur` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | sekcja 18 |
| 3. Mgła i winieta | mgła wykładnicza z wysokością, liczona z bufora głębi w przebiegu składającym, i winieta | `e8e1822` `feat(game): add ground fog from the depth buffer and a vignette` | [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | sekcja 19 |
| 4. Cienie księżyca | mapa cieni 2048 x 2048 z rzutem ortograficznym dopasowanym do terenu, `sampler2DShadow` z obiektem samplera, filtr sprzętowy 2 x 2, PCF do 7 x 7, bias w metrach, panel Shadows | `26c21c4` `feat(game): cast moon shadows with a shadow map, pcf and bias` | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), [`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md) | sekcja 20 |
| 5. Cień latarki i latarka w ręce | druga mapa cieni 1024 x 1024 z **rzutem perspektywicznym** (`scene::spotLightSpace`), nieliniowa głębia i bias w metrach przesuwający punkt w świecie, sprawdzenie `w <= 0`, światło latarki w ręce (`flashlightPose`: 0,20 m w prawo, 0,25 m w dół, wiązka zbiega się z osią widzenia), podgląd ze zlinearyzowaną głębią, zakładka `Flashlight` w panelu Shadows i trzy suwaki ręki w panelu Lights | commitowana razem z tym dokumentem | [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md) (sekcja 2.20), [`../modules/game/flashlight.md`](../modules/game/flashlight.md) | sekcja 21 |

Stan liczb po piątej części (zgłoszone dla Windowsa, 2026-10-06, nie powtarzałem): bramka `make check` zgłosiła 329 przypadków testowych i 104306 asercji, jedenaście programów shaderów (bez zmian), dwanaście paneli (bez zmian), `DebugContext` ma 38 pól (było 34). Debug exe uruchomiony na 7 sekund: OpenGL 4.1.0 NVIDIA, zasoby wczytane, stderr pusty, panele ukryte. **Nie było ćwiczone**: rysowanie w trybie Gouraud, podgląd w zakładce `Flashlight`, ścieżka ze zgaszoną latarką i `Reload shaders`. **Nikt nie oglądał obrazu** tej części. Dla porównania po czwartej części (2026-10-05): 310 i 103751.

Tematy wykładu: 10 (rendering pozaekranowy) jest **w toku**, bo brakuje minimapy. 11 (shadow mapping) jest **w toku**: kod obu map jest kompletny, ale testy ręczne na Windowsie i cały macOS są otwarte, a obrazu cieni latarki nikt nie oglądał. Żaden nie jest odhaczony w [`../syllabus.md`](../syllabus.md).

## 2. Co zostało

1. **Część 6: minimapa.** Widok labiryntu rysowany do osobnego framebuffera. Kodu nie ma, decyzje o niej są w sekcji 3.
2. **Zamknięcie M7:**
   - testy ręczne na Windowsie: listy 17.2, 18.2, 19.2, 20.2 i 21.2 w [`build-windows.md`](build-windows.md) (i lista części szóstej, gdy powstanie),
   - build, testy i listy na macOS ([`build-macos.md`](build-macos.md), kolejność w sekcji 6 niżej),
   - odhaczenie tematów 10 i 11 w [`../syllabus.md`](../syllabus.md),
   - tag.

## 3. Decyzje właściciela

| Część | Decyzja | Data | Notatka |
|---|---|---|---|
| cień latarki | światło latarki przenosi się z oka do **ręki**: trochę w prawo i trochę poniżej oka. Mapa cieni latarki ma rzut perspektywiczny | 2026-10-05 | [`../decisions/flashlight-in-hand.md`](../decisions/flashlight-in-hand.md) |
| cień latarki | wiązka **zbiega się**: celuje z ręki w punkt na osi widzenia przed okiem, a odległość tego punktu jest ustawieniem. Ręka startuje **0,20 m w prawo i 0,25 m w dół**, oba jako suwaki | 2026-10-06 | [`../decisions/flashlight-in-hand.md`](../decisions/flashlight-in-hand.md) |
| minimapa | pokazuje **tylko odkryte korytarze**, z przełącznikiem debugowania, który odsłania cały labirynt | 2026-10-05 | [`../decisions/minimap-discovered-corridors.md`](../decisions/minimap-discovered-corridors.md) |
| minimapa | komórka jest odkryta przez **linię wzroku wzdłuż korytarzy**: odkrywa się komórka, w której gracz stoi, i komórki w linii prostej w czterech kierunkach, aż do ściany | 2026-10-06 | [`../decisions/minimap-discovered-corridors.md`](../decisions/minimap-discovered-corridors.md) |
| minimapa | minimapa to **schemat rysowany z danych labiryntu** do własnego framebuffera, a nie osobny widok sceny z góry | 2026-10-06 | [`../decisions/minimap-discovered-corridors.md`](../decisions/minimap-discovered-corridors.md) |

Tabela jest listą tego, co zdecydował właściciel. **Kod minimapy nie istnieje**: dwie decyzje z 2026-10-06 są zapisane, ale nic z nich nie jest zbudowane ani sprawdzone. Rozmiar i miejsce minimapy na ekranie oraz wygląd schematu nie są ustalone.

Wybory wykonawcze przy części piątej (moje, **nie** decyzje właściciela; uzasadnienia w dokumentach): "w dół" ręki to w dół w świecie i granica suwaka "w prawo" 0,25 m ([`../decisions/flashlight-hand-straight-down.md`](../decisions/flashlight-hand-straight-down.md)), bias latarki w metrach przesuwający punkt w świecie ([`../decisions/flashlight-shadow-bias-in-world-space.md`](../decisions/flashlight-shadow-bias-in-world-space.md)), zbieganie 4 m startowo, zapas kąta mapy 2 stopnie, bliska płaszczyzna 0,05 m, mapa 1024, osobny uniform z pozycją światła, podgląd w trybie `Depth`, podgląd tylko dla wybranej zakładki ([`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.20.9).

Wyjaśnienie do decyzji o ręce (moje, nie treść decyzji): światło stojące w oku rzuca cienie dokładnie za przedmioty, więc nie byłoby ich widać.

## 4. Czego potrzebuje część 6 (minimapa)

To jest moja analiza dwóch decyzji z 2026-10-06 i tego, co już jest w kodzie (kodu minimapy nie ma i nie było notatek z jej implementacji). Część 5 użyła prawie wszystkiego, co część 4 zostawiła gotowe, bez zmian kształtu (klasa `ShadowMap`, `ShadowSettings`, `setShadowUniforms`, `drawShadowCasters(lightSpace)`, zakładki panelu, nowa jednostka teksturująca), więc dawna lista "do użycia w części 5" jest historią i została usunięta.

**Do użycia bez zmian:**

- `gfx::Framebuffer` z kolorem: cel rysowania do tekstury ([`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md)),
- pokazywanie tekstury w ImGui: sposób z paneli Framebuffers i Shadows (`ImGui::Image` z teksturą koloru),
- teren labiryntu w danych: `game::MazeWorld` i siatka komórek (układ komórek 2 m), z której schemat da się narysować bez sceny.

**Do dopisania** (nic z tego nie istnieje):

- stan odkrycia komórek i reguła z linii wzroku wzdłuż korytarzy (komórka gracza i komórki w czterech kierunkach do ściany),
- rysowanie schematu z danych labiryntu do własnego framebuffera, razem z przełącznikiem debugowania, który odsłania wszystko,
- rozmiar, miejsce na ekranie i wygląd minimapy (nieustalone),
- panel albo sekcja panelu do przełącznika, i wiersz w liście paneli, jeśli będzie nowy panel.

Ograniczenie mgły ([`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md)) dotyczyło widoku sceny z góry. Schemat rysowany z danych labiryntu do własnego framebuffera nie musi przechodzić przez przebieg składający, więc go **nie musi dotyczyć** (moja analiza: zależy od tego, jak powstanie kod, którego nie ma).

## 5. Znane ograniczenia i otwarte obserwacje

Zebrane ze wszystkich pięciu części. "Znane" znaczy: zapisane, świadomie zostawione albo czekające na sprawdzenie.

| Co | Skąd | Stan |
|---|---|---|
| mgła z góry zakrywa labirynt: wysokość jest brana w miejscu piksela, bez całki wzdłuż promienia (z 30 m około 95 procent mgły zamiast około 22) | część 3, [`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md) | świadomie zostawione. Dotknie minimapy: jej widok nie może przejść przez przebieg z mgłą |
| mgła na niebie tuż nad horyzontem zależy od miejsca na ekranie: daleka płaszczyzna jest płaska i obraca się z kamerą | część 3, [`../decisions/fog-no-special-case-for-sky.md`](../decisions/fog-no-special-case-for-sky.md) | policzone, nikt tego nie oglądał |
| plama latarki na ścianie nie daje poświaty, nawet z metra | część 2, [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | otwarta obserwacja |
| poświata jest względnie o połowę cieńsza w 1440p niż w 720p: jądro rozmycia ma promień w pikselach celu | część 2, [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) | otwarta obserwacja, w 1440p sprawdzono tylko wycinek obrazu |
| **niewyjaśniony spadek liczby klatek przy wyłączonych cieniach**: około 1250 i 630 klatek na sekundę (720p i 1440p) w sesji części 4 wobec około 1880 i 1145 w sesji części 3 | część 4, [`build-windows.md`](build-windows.md), sekcja 20.1 | **do ponownego pomiaru**. Sesji nie wolno porównywać. Potrzebny jest pomiar obu commitów w jednej sesji (ostatni punkt listy 20.2). Część 5 dodaje drugi taki pomiar (lista 21.2): koszt drugiego przebiegu głębi, mierzony w jednej sesji względem poprzedniego commitu |
| bias cieni nie zależy od rozmiaru teksela ani od jądra PCF: jądra `5 x 5` i `7 x 7` oraz mapa 1024 mogą lekko przyciemniać oświetlony grunt | część 4, [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.11 | policzone, nikt tego nie oglądał. Punkt listy 20.2 |
| cień rzucają księżyc i latarka. Światła kryształów świecą przez ściany | część 4 i 5 | kryształy: nie ma w planie |
| jedna mapa księżyca bez kaskad, pudełko stałe względem terenu, trawa nie rzuca cienia (ani w mapie księżyca, ani latarki) | część 4 i 5, [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.19 | świadomie zostawione |
| namalowana tarcza księżyca nie idzie za suwakami `Moon yaw` i `Moon pitch`, a od części 4 cienie idą | M6 i część 4, [`../decisions/painted-moon-fixed-direction.md`](../decisions/painted-moon-fixed-direction.md) | świadomie zostawione |
| środkowe commity każdego kamienia milowego nie budują się samodzielnie | historia repozytorium | do pomiarów i do `git bisect` nadaje się tylko ostatni commit każdej części |
| bias cienia latarki startowo pokrywa grunt do około 10 m przed graczem. Dalej jest za mały (policzone: błąd dwóch tekseli 16,6 cm w 12 m, bias 12,4 cm), a światła dochodzi tam około 11 procent jasności na 10 m | część 5, [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.20.3 | policzone, nikt tego nie oglądał. Lista 21.2 (jądra 3 x 3, 5 x 5, 7 x 7, bias 0) |
| mapa latarki rusza się z ręką, więc siatka tekseli jest przyklejona do światła, nie do świata: krawędzie cienia mogą migotać przy chodzeniu. Kod nie ma środka przeciw temu | część 5, sekcja 2.20.7 | moja analiza, nikt tego nie oglądał. Lista 21.2 |
| dalej niż zasięg latarki (16 m, daleka płaszczyzna mapy) cień nie jest sprawdzany, a światło latarki nie wygasa do zera (5 procent przy zasięgu), więc tam idzie przez ściany. Większy suwak `Beam range` rozciąga mapę (przy 60 m teksel ma około 5 cm) | część 5, sekcje 2.13, 2.19 i 2.20.1 | świadomie zostawione, policzone |
| wektor "w górę" mapy latarki przełącza się skokiem, gdy wiązka jest odchylona od pionu o mniej niż 2,56 stopnia (`abs(y) > 0,999`). Przy ustawieniach startowych **nie wystąpi** nawet przy nachyleniu kamery 89 stopni (wiązka ma wtedy 2,85 stopnia od pionu w górę i 3,24 w dół, bo ręka jest 0,2 m w prawo). Wystąpi przy `Converge at` powyżej około 4,6 m (w górę) albo 5,2 m (w dół) albo przy mniejszym `Hand right` | część 5, sekcja 2.20.1 | policzone, nikt tego nie oglądał. Lista 21.2 podaje, jak to wywołać |
| shader sprawdza cień latarki dla każdego fragmentu, także poza stożkiem światła (nie ma wczesnego wyjścia), a przebieg głębi rysuje te same obiekty trzeci raz w klatce | część 5, sekcja 2.20.5 i 2.20.7 | koszt niezmierzony. Lista 21.2, pomiar FPS w jednej sesji |
| pozycji ręki nie obcina nic poza zakresem suwaka: kod albo `noclip` mogą postawić światło w ścianie | część 5, [`../decisions/flashlight-hand-straight-down.md`](../decisions/flashlight-hand-straight-down.md) | nikt tego nie sprawdzał. Lista 21.2 |
| panel Shadows pokazuje obraz mapy księżyca według ustawienia, a mapy latarki według faktu (`flashlightShadowDrawn`), więc zakładki zachowują się inaczej | część 5, sekcja 2.19 | znane, drobne |
| żadnej kontrolki paneli z M7 nikt nie kliknął myszą | wszystkie części | listy 17.2 do 21.2 |

## 6. macOS: kolejność sprawdzania

Na Macu nie był budowany ani uruchamiany żaden kod od M2. Listy są w [`build-macos.md`](build-macos.md), po jednej na część (od części 1 do 5), wszystkie w całości otwarte. Poniżej kolejność od rzeczy, które najłatwiej mogą nie zadziałać na sterowniku OpenGL 4.1 firmy Apple, bo każda z nich blokuje to, co po niej.

| # | Co sprawdzić najpierw | Dlaczego to ryzyko | Sekcja w `build-macos.md` |
|---|---|---|---|
| 0 | `Makefile`: czy `make` i `make check` działają na macOS po zmianach zrobionych dla Windowsa (`b9298ec` `fix(make): run the makefile targets on windows from the developer shell`) | bez tego nie ma bramki. Zmiany powstały na Windowsie i na Macu nikt ich nie uruchomił | "Skróty: `make`" |
| 1 | **shader geometrii trawy** (`grass.geom`) | jedyny program z etapem geometrii. Kompilator GLSL Apple nigdy go nie widział | "M6, część 2 (teren i trawa)" |
| 2 | **cel rysowania `GL_RGBA16F`** | od części 1 cała scena idzie przez ten framebuffer: jeśli jest niekompletny, nie ma obrazu | "M7, część 1 (bufor HDR i gamma)" |
| 3 | **formaty tekstur sRGB i `GL_EXT_texture_sRGB_decode`** | formaty `GL_SRGB8` i `GL_SRGB8_ALPHA8` decydują o kolorach wszystkich tekstur sceny. Rozszerzenie dotyczy tylko podglądów tekstur w panelu Assets (`debug::RawTextureSampler`): bez niego podglądy są ciemniejsze, scena nie | "M7, część 1 (bufor HDR i gamma)" |
| 4 | **framebuffer z samą głębią i sampler cieni** (`glDrawBuffer(GL_NONE)`, `sampler2DShadow` z obiektem samplera, `GL_CLAMP_TO_BORDER`, podgląd głębi) | cztery rzeczy użyte pierwszy raz w części 4. Każda może wyłączyć cienie w całości | "M7, część 4 (cienie księżyca)" |
| 4a | **drugi framebuffer z samą głębią i odczyt mapy z rzutem perspektywicznym** (druga `ShadowMap` na jednostce 4, dwa `sampler2DShadow` w jednym programie, ramka samplera poza ostrosłupem, podgląd w trybie `Depth` z płaszczyznami światła, sprawdzenie `w <= 0` przed dzieleniem) | nowe w części 5. Sterownik, który zniesie jedną mapę, może inaczej traktować dwa samplery cieni w jednym programie albo głębię 24-bitową z bliską płaszczyzną 5 cm. Wada tu wyłącza cień latarki (albo cały obraz, jeśli program się nie zlinkuje) | "M7, część 5 (cień latarki)" |
| 5 | **rozmiary framebufferów na ekranie Retina** | okno 1280 x 720 ma bufor 2560 x 1440: scena, cele bloomu, podglądy i `glViewport` po każdym przebiegu. Do tego wydajność: PRD wymaga 60 klatek w 1440p na MacBooku | listy części 1 do 4 M7, punkty o Retinie |
| 6 | **`glPolygonMode`** (pole `Wireframe` terenu) | tryb linii w profilu Core na sterowniku Apple | "M6, część 2 (teren i trawa)" |
| 7 | reszta list, od M2 do części 5 M7, w kolejności dokumentu | wygląd, panele, sterowanie | wszystkie sekcje "na macOS: lista w całości otwarta" |

Wskazówka do punktu 0: jeśli `make` nie działa, wszystkie kroki da się wykonać samymi presetami CMake, które są opisane w tym samym dokumencie.
