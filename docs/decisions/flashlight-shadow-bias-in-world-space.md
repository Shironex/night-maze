# Bias cienia latarki: w metrach, przez przesunięcie punktu w przestrzeni świata

Data: 2026-10-06. Stan: obowiązuje. To wybór wykonawczy, nie decyzja właściciela projektu.
Kod: [`assets/shaders/common/shadows.glsl`](../../assets/shaders/common/shadows.glsl) (`flashlightShadow`, `slopeScaledBias`), [`src/game/Shadows.hpp`](../../src/game/Shadows.hpp) i [`Shadows.cpp`](../../src/game/Shadows.cpp) (`biasForShader`, `flashlightShadowDefaults`, stałe `FLASHLIGHT_SHADOW_CONSTANT_BIAS` i `FLASHLIGHT_SHADOW_SLOPE_BIAS`), [`src/game/ShadowMap.cpp`](../../src/game/ShadowMap.cpp) (`setShadowUniforms`), [`tests/ShadowTests.cpp`](../../tests/ShadowTests.cpp). Dokument modułu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.20.2 i 2.20.3. Notatka o biasie księżyca, od której ta się różni: [`shadow-bias-in-metres-in-shader.md`](shadow-bias-in-metres-in-shader.md).

## 1. Kontekst

Bias księżyca to liczba metrów, którą C++ dzieli przez głębię pudełka światła (`extent.z`) i wysyła do shadera jako różnicę zapisanych głębi. To działa, bo głębia rzutu ortograficznego jest liniowa: metr jest wszędzie tym samym ułamkiem zakresu.

Mapa latarki ma rzut perspektywiczny. Jej zapisana głębia to `f (z - n) / (z (f - n))`, czyli zmienia się szybko blisko światła i prawie wcale daleko od niego. Dla bliskiej płaszczyzny 0,05 m i dalekiej 16 m (policzone):

| Odległość od światła | 1 m | 4 m | 10 m |
|---|---|---|---|
| ile metrów to różnica głębi 0,001 | 2,0 cm | 31,9 cm | 1,99 m |

Jedna liczba w jednostkach głębi jest więc włosem przy ręce i grubością ściany pod koniec wiązki. Suwak, który ma być czytelny w metrach, nie może działać na głębi.

## 2. Decyzja

Bias latarki zostaje **w metrach aż do shadera**. Shader przesuwa punkt w przestrzeni świata o `bias` metrów po prostej do światła i dopiero przesunięty punkt rzutuje macierzą mapy. Głębia odniesienia nie jest już zmniejszana. C++ (`game::biasForShader`) wysyła dla rzutu perspektywicznego metry bez zmiany, a dla ortograficznego dalej ułamek głębi pudełka. Bias księżyca się nie zmienia.

Uwaga o zakresie: to mój wybór przy implementacji. Właściciel projektu o biasie nie decydował.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **przesunięcie punktu w świecie po prostej do światła (wybrane)** | długość przesunięcia jest ta sama w metrach w każdym miejscu. Punkt zostaje na tym samym promieniu, więc trafia w ten sam teksel, tylko bliżej światła (test `moving a point towards a spot light keeps its texel and lowers its depth`). Suwaki w metrach, jak dla księżyca | dodatkowe `length`, dzielenie i mnożenie na fragment oraz dwa wyjścia szczególne (`lightDistance <= bias` i `clip.w <= 0`). Rachunek w shaderze jest inny dla dwóch świateł |
| odjęcie stałej liczby od głębi, jak dla księżyca | ten sam kod co dla księżyca, jeden wzór | bias o długości od centymetrów do metrów w zależności od odległości (tabela wyżej). Nie da się go nastawić w metrach |
| przeliczenie głębi na metry, odjęcie biasu i powrót do głębi | wynik taki sam jak przesunięcie punktu (w osi światła) | trzeba w shaderze zamieniać głębię w odległość i z powrotem (`linearDepth` i jej odwrotność), czyli to samo co przesunięcie, tylko bardziej okrężnie |
| `glPolygonOffset` przy rysowaniu mapy | karta zna prawdziwe nachylenie głębi | jednostki zależne od implementacji i od nieliniowej głębi, a decyzja z notatki o biasie księżyca ([`shadow-bias-in-metres-in-shader.md`](shadow-bias-in-metres-in-shader.md)) wykluczyła je dla obu świateł |
| przesunięcie wzdłuż normalnej powierzchni | radzi sobie z powierzchniami stycznymi do światła | przesuwa też miejsce odczytu w mapie, więc cień się kurczy, i wymaga normalnej tam, gdzie shader jej nie ma w funkcji cienia (`gouraud.frag`). Odrzucone już dla księżyca z tych powodów |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Na obronie bias latarki trzeba umieć pokazać tak samo jak bias księżyca: suwak na 0 daje acne, suwak na 0,5 m peter panning, a między nimi jest liczba, którą da się uzasadnić ("teksel ma 3 mm, ściana 20 cm"). Jednostka w metrach i jeden wzór na `facing` pozwalają na to, a przesunięcie w świecie jest jedynym sposobem, który daje tę samą długość biasu przy ręce i na końcu wiązki, bez wzoru na odwrotność głębi w shaderze.

**Liczby startowe.** Stała część 0,01 m, nachylenie 0,13 m (księżyc ma 0,02 i 0,12). Teksel latarki ma 0,83 mm na każdy metr od ręki przy mapie 1024, czyli 3,3 mm na ścianie 4 m dalej (księżyc: 31,6 mm przy 2048). Ręka jest 1,45 m nad gruntem, więc światło muska grunt coraz bardziej płasko, im dalej pada. Dla gruntu przed graczem (policzone; test `the default bias of the flashlight covers the ground up to 10 m ahead` sprawdza wiersze do 10 m):

| Grunt przed graczem | Błąd dwóch tekseli (jądro 3 x 3 z filtrem sprzętowym) | Bias startowy |
|---|---|---|
| 4 m | 1,9 cm | 9,6 cm |
| 10 m | 11,6 cm | 12,1 cm |
| 12 m | 16,6 cm | 12,4 cm |

Do około 10 m bias jest większy od błędu, dalej mniejszy. Największy możliwy bias to 0,14 m, mniej niż 0,2 m grubości ściany (test `the shadows of the flashlight start with the small map and a bias of their own`), więc cień nie odkleja się od ściany.

**Skutki, które przyjmuję.**

- Dalej niż około 10 m przed graczem grunt może pokazywać acne albo przyciemnienie. Światła dochodzi tam już niewiele (wzór tłumienia daje około 11 procent jasności na 10 m i 5 procent na 16 m, bez stożka). To jest **policzone, nie zaobserwowane**, i stoi na liście testów ręcznych ([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 21.2).
- Bias, jak dla księżyca, nie zna promienia PCF ani rozmiaru teksela. Przy jądrze `5 x 5` i `7 x 7` zapas maleje, a przy mapie 2048 rośnie.
- Wzór biasu jest w dwóch miejscach (GLSL i C++), a testy pilnują tylko strony C++. Samego przesunięcia punktu i sprawdzenia `w <= 0` w GLSL żaden test nie sprawdza. Testy sprawdzają matematykę, na której się opierają (ten sam teksel, mniejsza głębia, znak `w`).
- Shader robi rachunek cienia latarki dla każdego fragmentu, także poza stożkiem światła, o ile latarka świeci. Koszt nie był mierzony.

## 5. Kiedy wrócić do tej decyzji

- Gdy test ręczny potwierdzi acne albo przyciemnienie gruntu dalej niż 10 m. Naturalną poprawką jest bias proporcjonalny do rozmiaru teksela w danej odległości (`shadowTexelSizeAt` już go liczy), a nie zmiana tej decyzji.
- Gdy gra dostanie drugie światło z mapą perspektywiczną: ta sama funkcja pasuje, ale ustawienia startowe trzeba policzyć od nowa.
- Gdy mapa latarki dostanie większy zasięg suwakiem (do 60 m): teksel na dalekiej płaszczyźnie rośnie do około 5 cm, a bias startowy jest wtedy za mały.
- Gdy koszt cieni latarki na fragment okaże się za duży i potrzebne będzie wczesne wyjście dla fragmentów poza stożkiem.
