# Moduł scene: światła

Kamień milowy: M4 (część "oświetlenie", uzupełniony w części "mapy normalnych": sekcja 2.9), w M5 doszły światła kryształów i składnik emisyjny (sekcja 2.2), w pierwszej części M7 rachunek światła przeszedł na wartości liniowe z wynikiem w buforze HDR (sekcja 2.8), w czwartej części M7 (cienie księżyca, 2026-10-05) doszedł udział księżyca w wyniku `computeLighting`, funkcja `moonFacing` i struktura `scene::LightSpace` (sekcje 2.8, 4.3, 4.4, 4.6 i 5.7). Temat wykładu: 6 (Światło kierunkowe i punktowe).
Kod: [`src/scene/Light.hpp`](../../../src/scene/Light.hpp), [`src/scene/Light.cpp`](../../../src/scene/Light.cpp), plik dołączany do shaderów [`assets/shaders/common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl), panel [`src/debug/panels/LightsPanel.hpp`](../../../src/debug/panels/LightsPanel.hpp) i [`LightsPanel.cpp`](../../../src/debug/panels/LightsPanel.cpp), testy [`tests/LightTests.cpp`](../../../tests/LightTests.cpp). Od czwartej części M7 także [`src/scene/LightSpace.hpp`](../../../src/scene/LightSpace.hpp) i [`src/scene/LightSpace.cpp`](../../../src/scene/LightSpace.cpp) z testami w [`tests/ShadowTests.cpp`](../../../tests/ShadowTests.cpp).

Część modułu `scene`. Wstęp do modułu jest w [`README.md`](README.md). Ten dokument zakłada znajomość przekształceń ([`transforms.md`](transforms.md)), kamery ([`camera.md`](camera.md)), shaderów i uniformów ([`../gfx/shaders.md`](../gfx/shaders.md), [`../gfx/uniforms.md`](../gfx/uniforms.md)) oraz tekstur ([`../gfx/textures.md`](../gfx/textures.md)).

Oświetlenie jest rozłożone na siedem dokumentów (siódmy, o cieniach, doszedł w czwartej części M7). Każdy plik kodu jest omawiany linia po linii w jednym z nich:

| Dokument | Co omawia |
|---|---|
| ten | rodzaje świateł, model odbicia Phonga, tłumienie, stożek, macierz normalnych (teoria), struktury `scene::Light`, plik `common/lighting.glsl`, panel Lights |
| [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md) | temat 7: gdzie liczone jest światło (wierzchołek albo fragment), Phong a Blinn-Phong, shadery `lit` i `gouraud`, przełącznik trybu |
| [`../game/flashlight.md`](../game/flashlight.md) | światła gry: `LightingSettings`, latarka, jej bateria i klawisz F, światła nad kryształami, `lightingForFrame`, `buildLightSet`, `LightRig` |
| [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md) | jak światła trafiają do shadera: blok uniformów `LightBlock`, układ `std140`, `scene::LightBlockData`, `gfx::UniformBuffer` |
| [`../gfx/shader-includes.md`](../gfx/shader-includes.md) | jak działa linia `#include "common/lighting.glsl"` |
| [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md) | skąd program `lit` bierze normalną fragmentu: mapy normalnych, przestrzeń styczna, macierz TBN, plik `common/normal_map.glsl` |
| [`../renderer/shadows.md`](../renderer/shadows.md) | temat 11: mapa cieni księżyca, przestrzeń światła (`scene::LightSpace`) z pełną matematyką, plik `common/shadows.glsl`, bias, PCF, panel Shadows |

**Stan na dziś:** gra startuje jako scena nocna. Labirynt oświetlają trzy rodzaje świateł: księżyc (światło kierunkowe), latarka gracza (reflektor) i światła punktowe nad kryształami, których gracz jeszcze nie zebrał. Kryształy same też świecą: shadery mają od M5 składnik emisyjny (uniform `uEmissive`).

Część M4 (rodzaje świateł, wzory, panel) była zmierzona na Windowsie 2026-10-05 (MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDII 610.74): start gry bez linii `[error]` i bez linii `GL_`, a na zrzutach ekranu widok startowy, cztery tryby cieniowania z trzech miejsc, latarka wyłączona, strony ścian oświetlone i nieoświetlone przez księżyc. Światła punktowe wisiały wtedy w ślepych zaułkach.

M5 (światła nad kryształami, puls, składnik emisyjny, bateria latarki) jest gotowy w kodzie na Windowsie i **nie jest zamknięty**. Zgłoszone dla Windowsa 2026-10-05 po M5: build Debug i Release bez ostrzeżeń, 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach (po drugiej części M6 256 przypadków i 101232 asercje, po pierwszej części M7 zgłoszone 269 przypadków i 102103 asercje, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751), obraz był sprawdzony na zrzutach ekranu robionych przez tymczasowe zaczepy w kodzie, które potem usunięto. **Żadnego widżetu panelu Lights nikt jeszcze nie kliknął ręcznie**, klawisz F też nie był naciskany, a zbierania kryształów i gasnących świateł nikt nie oglądał w działającej grze. **Na macOS ten kod nie był ani budowany, ani uruchamiany**: ani część M4, ani nic z M5, M6 i pierwszej części M7.

W drugiej części M6 pod światłami zmieniło się podłoże: płytki podłogi zastąpił teren z mapy wysokości ([`../renderer/terrain.md`](../renderer/terrain.md)), a plik `common/lighting.glsl` dostał trzeciego użytkownika, shader fragmentów trawy `grass.frag` ([`../renderer/grass-geometry.md`](../renderer/grass-geometry.md)). Wzory świateł, struktury i panel Lights zostały bez zmian.

W pierwszej części M7 (bufor HDR i gamma, zgłoszona jako zbudowana i sprawdzona na Windowsie 2026-10-05, nowych kontrolek nikt nie klikał) wzory świateł, struktury i plik `common/lighting.glsl` zostały bez zmian, ale zmieniło się to, **na jakich liczbach** liczą: kolory świateł trafiają do shadera jako wartości liniowe (przelicza je `game::buildLightSet`), tekstury koloru są dekodowane z sRGB przy odczycie, a wynik idzie do bufora zmiennoprzecinkowego, w którym wartość powyżej 1 nie jest obcinana. Wartości startowe świateł zostały dobrane od nowa (sekcja 2.8).

W czwartej części M7 (cienie księżyca, 2026-10-05) **księżyc dostał cień**: przed sceną wszystko, co rzuca cień, jest rysowane z kierunku księżyca do tekstury głębi (mapy cieni), a programy oświetlenia sprawdzają w niej każdy fragment. Całą technikę opisuje [`../renderer/shadows.md`](../renderer/shadows.md). W tym dokumencie zmieniło się pięć rzeczy. Struktura `Lighting` w `common/lighting.glsl` ma dwa nowe pola, `moonDiffuse` i `moonSpecular`, czyli udział księżyca trzymany osobno (sekcja 4.3). Gałąź księżyca w `computeLighting` nie woła już `addLight`, tylko sama zapisuje oba wyrazy (sekcja 4.6). Doszła funkcja `moonFacing` (sekcja 4.4). Moduł `scene` ma nowy plik `LightSpace` z widokiem i rzutowaniem światła (sekcja 5.7). Intensywność startowa księżyca wzrosła z 0,12 do 0,2 (sekcja 2.8). Blok `LightBlock`, struktury świateł w `Light.hpp` i panel Lights zostały bez zmian. Zgłoszone dla Windowsa, 2026-10-05: bramka `make check` przechodzi, 310 przypadków testowych i 103751 asercji, build Debug nie zalogował błędów OpenGL przy mapie 2048 i 1024. **Nie sprawdzone:** kontrolki panelu Shadows myszą, przełączanie rozdzielczości mapy w działającej grze, przeładowanie shaderów (dziś jedenaście programów), macOS (nic nie było budowane ani uruchamiane).

Czego nadal nie ma: **cieni świateł kryształów**. Światła punktowe nadal przechodzą przez ściany (sekcja 2.8). Cień latarki doszedł w piątej części M7 (2026-10-06): struktura `Lighting` ma dwa kolejne pola, `flashlightDiffuse` i `flashlightSpecular`, jest funkcja `flashlightFacing`, `LightSpace` ma drugą funkcję, `spotLightSpace` (rzut perspektywiczny), a latarka stoi w ręce gracza, nie w oku ([`../game/flashlight.md`](../game/flashlight.md)). Blok `LightBlock`, struktury w `Light.hpp` i panel Lights (poza trzema nowymi suwakami ręki) się nie zmieniły. **Korekcja gamma i tekstury sRGB**, które do M6 stały na tej liście, są już w kodzie ([`../gfx/color-space.md`](../gfx/color-space.md), notatka [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md), która zastąpiła [`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md)). Mapy normalnych, które w pierwszej części M4 były na tej liście, są już w kodzie: normalną, którą dostają wzory z tego dokumentu, opisuje sekcja 2.9.

## 1. Po co to jest

Do M3 kolor piksela był kolorem tekstury pomnożonym przez kolor materiału. Scena była równo jasna: ściana na wprost i ściana w głębi korytarza wyglądały tak samo, a kształtu słupka nie dało się odczytać inaczej niż z rysunku tekstury. **Oświetlenie** odpowiada na pytanie: ile światła dociera do tego punktu powierzchni i ile z niego odbija się w stronę oka.

Potrzebne są do tego trzy rzeczy:

| Rzecz | Gdzie | Dokument |
|---|---|---|
| opis świateł jako danych: kierunek, pozycja, kolor, zasięg, stożek | struktury w `scene/Light.hpp` | ten, sekcja 5 |
| wzory, które z jednego światła i jednego punktu powierzchni robią jasność | funkcje w `common/lighting.glsl` | ten, sekcja 4 |
| normalna powierzchni w przestrzeni świata | atrybut `aNormal` razy macierz normalnych | ten (teoria, sekcja 2.7), kod w [`transforms.md`](transforms.md) |

Struktury w `scene/Light.hpp` to same dane i matematyka: plik nie dołącza GLAD i nie woła żadnej funkcji `gl*`, więc ma testy jednostkowe. Funkcje z `Light.cpp` (tłumienie, stożek) liczą to samo, co shader liczy dla każdego fragmentu. Shader nie woła kodu C++: istnieją po to, żeby przygotować dane dla shadera i żeby wzory dało się sprawdzić testem.

## 2. Teoria

### 2.1 Trzy rodzaje świateł

| Rodzaj | Ma pozycję | Ma kierunek | Słabnie z odległością | W grze |
|---|---|---|---|---|
| **kierunkowe** (directional light) | nie | tak, jeden dla całej sceny | nie | księżyc |
| **punktowe** (point light) | tak | nie, świeci we wszystkie strony | tak | światła nad kryształami |
| **reflektor** (spot light) | tak | tak, oś stożka | tak, i dodatkowo poza stożkiem jest zero | latarka |

**Światło kierunkowe** to źródło tak odległe, że jego promienie są równoległe. Księżyc jest 384 tysiące kilometrów od labiryntu o boku 20 m: kierunek do niego jest z każdego miejsca labiryntu praktycznie ten sam i odległość też. Dlatego takie światło nie ma pozycji i nie ma tłumienia.

**Światło punktowe** świeci z jednego punktu we wszystkie strony. Kierunek do światła jest inny dla każdego punktu powierzchni (trzeba go policzyć: pozycja światła minus pozycja punktu), a jasność spada z odległością.

**Reflektor** to światło punktowe, które świeci tylko w stożek. Dochodzi oś stożka i dwa kąty (sekcja 2.6).

```mermaid
flowchart LR
    subgraph K["kierunkowe"]
        direction TB
        K1["promienie równoległe,<br>ten sam kierunek wszędzie"]
    end
    subgraph P["punktowe"]
        direction TB
        P1["promienie z jednego punktu,<br>kierunek = pozycja światła - punkt"]
    end
    subgraph S["reflektor"]
        direction TB
        S1["jak punktowe,<br>ale tylko wewnątrz stożka"]
    end
    K --> W["wektor do światła L,<br>kolor światła po tłumieniu"]
    P --> W
    S --> W
    W --> F["te same wzory: Lambert i odbłysk"]
```

Diagram pokazuje najważniejszą rzecz w budowie shadera: trzy rodzaje świateł różnią się tylko tym, **jak powstaje wektor do światła i ile światła dociera**. Dalej każde jest liczone tymi samymi wzorami. W kodzie to funkcja `addLight` (sekcja 4.5).

### 2.2 Model odbicia Phonga: trzy składniki

Prawdziwe światło odbija się wiele razy od wszystkiego dookoła. Liczenie tego w czasie rzeczywistym jest za drogie, więc klasyczny **model odbicia Phonga** (Phong reflection model) udaje je sumą trzech prostych składników:

| Składnik | Co udaje | Od czego zależy | W kodzie |
|---|---|---|---|
| **otoczenia** (ambient) | światło odbite wiele razy, które dociera wszędzie | od niczego: stała | `uAmbient` |
| **rozproszony** (diffuse) | matową powierzchnię, która odbija światło równo we wszystkie strony | od kąta między normalną a kierunkiem do światła | `diffuseFactor` |
| **zwierciadlany, odbłysk** (specular) | połysk: odbicie źródła światła | od kierunku do światła, normalnej **i kierunku do oka** | `specularFactor` |

Wektory, których używają wzory. Wszystkie mają długość 1 i **zaczynają się w punkcie powierzchni**:

| Symbol | Nazwa w shaderze | Znaczenie |
|---|---|---|
| `N` | `normal` | normalna: kierunek prostopadły do powierzchni |
| `L` | `toLight` | kierunek od punktu do światła |
| `V` | `toEye` | kierunek od punktu do oka (kamery) |
| `R` | `reflected` | promień światła odbity lustrzanie względem normalnej |
| `H` | `halfway` | wektor połówkowy: dokładnie między `L` i `V` |

Kolor fragmentu w shaderach projektu:

```text
kolor = kolor_powierzchni * (otoczenie + suma po światłach: światło * Lambert)
      + suma po światłach: światło * odbłysk
```

Dwie decyzje widać w tym wzorze:

- **Kolor powierzchni (tekstura razy kolor materiału) mnoży tylko część rozproszoną i otoczenia.** Czerwona ściana odbija czerwoną część światła. Odbłysk jest dodawany na wierzch **w kolorze światła**: połysk na kamieniu oświetlonym ciepłą latarką jest ciepły, a nie w kolorze kamienia.
- **Suma po światłach.** Światła się dodają. Wynik może przekroczyć 1. Do M6 framebuffer okna obcinał wtedy kolor do 1 (prześwietlenie). Od pierwszej części M7 scena jest rysowana do bufora zmiennoprzecinkowego (`GL_RGBA16F`), który przechowuje także wartości większe niż 1, a w zakres ekranu sprowadza je dopiero mapowanie tonów w ostatnim przebiegu klatki ([`../renderer/post-process.md`](../renderer/post-process.md)). Panel Lights pozwala wywołać duże wartości celowo (sekcja 6).

**Czwarty składnik: emisyjny.** Model Phonga ma trzy składniki. Równanie oświetlenia potoku stałego starego OpenGL dokładało do nich czwarty, najprostszy: **emisję** (emissive term), czyli światło, które powierzchnia **oddaje sama z siebie**. Nie zależy od żadnego światła sceny, od normalnej ani od oka: żarówka, ekran albo świecący kryształ są jasne także w zupełnej ciemności. W potoku stałym był to parametr materiału `GL_EMISSION`, a w plikach MTL jest to linia `Ke` (loader projektu ją pomija: emisja nie pochodzi tu z pliku materiału, tylko z kodu gry).

W projekcie od M5 pełny wzór koloru fragmentu ma jeden dodatkowy wyraz, uniform `uEmissive`:

```text
kolor = kolor_powierzchni * (otoczenie + suma po światłach: światło * Lambert + emisja)
      + suma po światłach: światło * odbłysk
```

| Pytanie | Odpowiedź |
|---|---|
| gdzie jest `uEmissive` | w czterech shaderach fragmentów: `lit.frag`, `gouraud.frag`, `textured.frag` i, od M8, części 1, `reflect.frag`. **Nie** w `common/lighting.glsl` i nie w bloku świateł: to cecha rysowanego obiektu, a nie światło sceny |
| kto go ustawia | klasy rysujące, przed swoimi obiektami: `MazeRenderer::draw` na czerń (kamień nie świeci), `GameplayRenderer::draw` na czerń dla bramy i na `game::crystalGlow(...)` dla kryształów |
| jaką ma wartość dla kryształów | kolor świateł punktowych (`LightingSettings::pointColor`, startowo turkus `(0,2, 0,9, 0,8)`) (od M7 przeliczony na wartości liniowe) razy `CRYSTAL_GLOW_STRENGTH` (dziś 4,0, do M6 było 1) razy puls (od 0,7 do 1) |
| dlaczego jest **dodany do światła rozproszonego**, a nie do gotowego koloru | emisja zachowuje się wtedy jak światło, które powierzchnia dostaje sama od siebie: w ciemnym kącie `otoczenie + Lambert` jest bliskie zera, a `emisja` zostaje. Dla wszystkiego poza kryształami wyraz jest zerem i wzór jest dokładnie tym z M4 |
| dlaczego jest **mnożony przez kolor powierzchni** | żeby kryształ nie stał się płaską plamą jednego koloru. Emisja dodana na wierzch dałaby każdemu fragmentowi tę samą liczbę i zatarła rysunek tekstury. Pomnożona przez teksturę zostawia jej jasne i ciemne miejsca. Cena: tam, gdzie tekstura jest czarna, kryształ nie świeci |
| czy emisja **oświetla coś innego** | **nie**. Zmienia kolor tylko tych fragmentów, które są rysowane z niezerowym `uEmissive`. Ściana obok kryształu nic o nim nie wie |

Skąd w takim razie turkusowy blask na ścianach wokół kryształu? Z osobnego **światła punktowego**, które gra wiesza 0,15 m nad czubkiem każdego niezebranego kryształu ([`../game/flashlight.md`](../game/flashlight.md), sekcja 2.5). Emisja i światło punktowe to dwa mechanizmy, które razem udają jedną rzecz: mają ten sam kolor (`pointColor`) i pulsują tym samym mnożnikiem (`game::crystalPulse`). Prawdziwe źródło światła robi obie rzeczy naraz, w modelu Phonga trzeba je złożyć z dwóch części.

Po co emisja jest potrzebna, skoro kryształ ma własne światło: to światło wisi **nad** kryształem i poza jego siatką (światło w środku zamkniętej siatki pada na jej ścianki od tyłu i nie oświetla żadnej). Ścianki boczne i dolne dostają z niego mało albo nic, więc bez emisji źródło światła byłoby najciemniejszą rzeczą w swoim kącie. Siłę blasku ustawia stała `CRYSTAL_GLOW_STRENGTH` w `game/Crystals.hpp`. Do M6 wynosiła 1: mocniejszy blask zamieniał wtedy cały kryształ w jedną płaską plamę najjaśniejszego koloru, jaki ekran umie pokazać, bo wszystko powyżej 1 było obcinane. Od pierwszej części M7 wynosiła 2,5, a od drugiej wynosi 4,0, celowo powyżej 1: scena jest rysowana do bufora HDR, w którym kolor może być jaśniejszy niż biel, a świecący kryształ jest jedyną rzeczą w labiryncie, która taka ma być. Mapowanie tonów sprowadza go potem w zakres ekranu bez zlewania ścianek w płaską plamę (liczby: [`../game/gameplay.md`](../game/gameplay.md)). Podwyżka do 4,0 przyszła razem z bloomem: to, co zostaje po pomnożeniu przez teksturę kryształu, ma leżeć powyżej progu poświaty także w najciemniejszej chwili pulsu ([`../renderer/post-process.md`](../renderer/post-process.md), sekcja 2.15). Shadery z `uEmissive` linia po linii: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), sekcje 4.2 i 4.4. Kto i kiedy ustawia `uEmissive`: [`../game/gameplay.md`](../game/gameplay.md), sekcja 4. Rysowanie kryształów: tamże, sekcja 5.

Odbłysk ma dwa warianty wzoru, Phonga i Blinna-Phonga. Różnicę, geometrię i to, jak ją pokazać, omawia dokument tematu 7: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md). Tu wystarczy wiedzieć, że oba dają liczbę od 0 do 1, największą tam, gdzie odbicie światła trafia prosto w oko.

### 2.3 Prawo cosinusów Lamberta

Matowa powierzchnia jest najjaśniejsza, gdy stoi przodem do światła, i ciemnieje, gdy się od niego odwraca. Powód jest geometryczny: ta sama wiązka światła padająca ukośnie rozkłada się na większy kawałek powierzchni, więc na każdy jego fragment przypada mniej.

```text
   światło na wprost            światło pod kątem

      | | | |                      \ \ \ \
      v v v v                       \ \ \ \
   -------------                  -------------------
   wiązka o szerokości 1          ta sama wiązka pada
   pada na odcinek 1              na odcinek 1 / cos(kąt)
```

Jasność jest proporcjonalna do **cosinusa kąta między normalną a kierunkiem do światła**:

```text
Lambert = max(dot(N, L), 0)
```

- Dla dwóch wektorów o długości 1 iloczyn skalarny (dot product) **jest** cosinusem kąta między nimi. Dlatego wektory muszą być znormalizowane: inaczej wynik byłby pomnożony przez ich długości.
- Kąt 0 stopni: cosinus 1, pełna jasność. 60 stopni: 0,5. 90 stopni: 0, światło ślizga się po powierzchni.
- Powyżej 90 stopni cosinus jest ujemny: światło jest **za** powierzchnią. `max(..., 0)` zamienia to na "brak światła". Bez `max` ujemna wartość odejmowałaby światło pochodzące z innych źródeł.

Przykład na liczbach z gry. Światło księżyca przy ustawieniach startowych leci w kierunku około `(0,27, -0,77, -0,58)` (sekcja 5.5), więc `L = (-0,27, 0,77, 0,58)`:

| Powierzchnia | Normalna `N` | `dot(N, L)` | Wynik |
|---|---|---|---|
| poziomy grunt | `(0, 1, 0)` | 0,77 | jasny |
| lico ściany zwrócone ku `+Z` | `(0, 0, 1)` | 0,58 | średnia |
| lico ściany zwrócone ku `-X` | `(-1, 0, 0)` | 0,27 | ciemna |
| lico ściany zwrócone ku `+X` | `(1, 0, 0)` | -0,27, po `max` 0 | tylko światło otoczenia |
| lico ściany zwrócone ku `-Z` | `(0, 0, -1)` | -0,58, po `max` 0 | tylko światło otoczenia |

Pierwszy wiersz to grunt idealnie poziomy, taki jak płytki podłogi do M5. Teren z M6 jest pod labiryntem łagodnie nierówny, więc jego normalne odchylają się od `(0, 1, 0)` o kilka stopni i liczba 0,77 zmienia się z miejsca na miejsce o kilka setnych: zbocze zwrócone ku księżycowi jest trochę jaśniejsze, odwrócone trochę ciemniejsze. Normalną `(0, 1, 0)` dokładnie ma za to trawa: `grass.frag` oświetla wszystkie źdźbła stałą normalną skierowaną w górę.

Kąt obrotu księżyca (yaw 25 stopni) celowo nie jest wielokrotnością 45: ściany patrzą w cztery strony i dzięki temu każda z dwóch oświetlonych stron dostaje inną jasność. Widać to na zrzucie ekranu z Windowsa (strony oświetlone i nieoświetlone).

### 2.4 Odbłysk w skrócie

Odbłysk zależy od położenia oka: gdy idę wzdłuż ściany, jasna plama przesuwa się razem ze mną, a część rozproszona stoi w miejscu. Wzór podnosi cosinus pewnego kąta do potęgi:

```text
odbłysk = siła * max(cos, 0) ^ połysk
```

| Parametr | Uniform | Znaczenie |
|---|---|---|
| siła (strength) | `uSpecularStrength` | jak jasny jest odbłysk w porównaniu ze światłem, które go wywołuje. 0 wyłącza odbłysk |
| połysk (shininess) | `uShininess` | wykładnik. Większy daje mniejszą, ostrzejszą plamę: cosinus bliski 1 podniesiony do dużej potęgi zostaje bliski 1, a każdy mniejszy szybko spada do zera |

Kamień jest szorstki, więc wartości startowe to siła 0,25 i połysk 32. Który cosinus jest podnoszony do potęgi (Phong: `dot(R, V)`, Blinn-Phong: `dot(N, H)`), omawia dokument tematu 7.

### 2.5 Tłumienie: jak światło słabnie z odległością

Światło z punktu rozchodzi się po powierzchni kuli. Powierzchnia kuli rośnie z kwadratem promienia, więc fizycznie jasność spada jak `1 / d²`. Sam ten wzór jest niewygodny: przy `d` bliskim zera daje nieskończoność, a daleko nigdy nie dochodzi do zera. Grafika czasu rzeczywistego używa wzoru z trzema współczynnikami:

```text
tłumienie(d) = 1 / (constant + linear * d + quadratic * d * d)
```

| Współczynnik | Rola |
|---|---|
| `constant` | część niezależna od odległości. Z wartością 1 światło ma w odległości 0 pełną jasność i nigdy nie jest jaśniejsze |
| `linear` | część rosnąca z odległością: szybki spadek blisko źródła |
| `quadratic` | część rosnąca z kwadratem odległości: ta fizyczna, sprowadza światło w dół daleko od źródła |

**Tłumienie z promienia.** Trzy współczynniki trudno dobrać ręcznie. Wygodniej powiedzieć "to światło ma sięgać 3 metry". Funkcja `scene::attenuationForRadius` liczy współczynniki z promienia `r`:

```text
constant = 1        linear = 2 / r        quadratic = 17 / r²
```

W odległości `d = r` mianownik wynosi `1 + 2 + 17 = 20`, więc zostaje `1 / 20`, czyli **5 procent** jasności, niezależnie od promienia. Krzywa ma dla każdego promienia ten sam kształt, tylko rozciągnięty:

| Odległość | Mianownik | Zostaje | Dla światła punktowego (`r` = 3 m) | Dla latarki (`r` = 16 m) |
|---|---|---|---|---|
| 0 | 1 | 100 % | 0 m | 0 m |
| ćwierć promienia | 1 + 0,5 + 1,0625 | 39 % | 0,75 m | 4 m |
| pół promienia | 1 + 1 + 4,25 | 16 % | 1,5 m | 8 m |
| trzy czwarte | 1 + 1,5 + 9,5625 | 8,3 % | 2,25 m | 12 m |
| promień | 1 + 2 + 17 | 5 % | 3 m | 16 m |
| półtora promienia | 1 + 3 + 38,25 | 2,4 % | 4,5 m | 24 m |
| dwa promienie | 1 + 4 + 68 | 1,4 % | 6 m | 32 m |

Dla `r = 3` współczynniki to `linear = 0,667` i `quadratic = 1,889`, dla `r = 16` to `0,125` i `0,0664`.

Dwie uwagi do tabeli:

- **Światło nigdy nie dochodzi do zera.** `1 / (coś)` jest zawsze dodatnie. Za promieniem światło nadal jest, tylko za słabe, żeby je zobaczyć. "Promień" to umowa: miejsce, w którym zostaje 5 procent.
- **Promień 3 m to półtorej komórki** labiryntu (komórka ma 2 m). Tyle PRD przewiduje dla kryształów i taki promień mają ich światła.

Światło kierunkowe nie ma tłumienia: w shaderze nie jest dla niego w ogóle liczone.

### 2.6 Stożek reflektora i dlaczego porównuje się cosinusy

Reflektor ma oś (kierunek, w którym świeci) i dwa kąty, oba mierzone **od osi do boku stożka**, czyli połówki pełnego kąta rozwarcia:

```text
                      oś stożka
        latarka  o-----------------------------------------> 
                  \ `-.                    stożek wewnętrzny (13 stopni):
                   \   `-.                 pełna jasność
                    \     `-._
                     \        `-._         między stożkami:
                      \           `-._     jasność spada do zera (miękki brzeg)
                       \              
                        \                  poza stożkiem zewnętrznym (21 stopni):
                                           zero
```

Dla punktu powierzchni liczy się kąt między osią a promieniem od latarki do tego punktu. Wewnątrz stożka wewnętrznego czynnik wynosi 1, poza zewnętrznym 0, a pomiędzy zmienia się płynnie. Gdyby był jeden kąt, plama miałaby ostrą krawędź jak wycięta nożyczkami.

**Dlaczego cosinusy.** Shader ma dwa wektory o długości 1: oś stożka i kierunek od światła do punktu. Ich iloczyn skalarny to cosinus kąta między nimi i kosztuje trzy mnożenia. Żeby dostać sam kąt, trzeba by wołać `acos` dla każdego fragmentu i każdego reflektora. Zamiast tego **kąty stożka zamienia się na cosinusy raz, w C++**, a shader porównuje cosinus z cosinusem.

Trzeba tylko pamiętać, że cosinus **maleje**, gdy kąt rośnie:

| Kąt | Cosinus |
|---|---|
| 0 stopni (na osi) | 1 |
| 13 stopni (stożek wewnętrzny latarki) | 0,974 |
| 21 stopni (stożek zewnętrzny latarki) | 0,934 |
| 90 stopni (w bok) | 0 |

Stożek wewnętrzny ma więc **większy** cosinus niż zewnętrzny, a "wewnątrz stożka" znaczy "cosinus większy od progu". Wzór miękkiego brzegu:

```text
czynnik = clamp((cosKąta - cosZewnętrzny) / (cosWewnętrzny - cosZewnętrzny), 0, 1)
```

Licznik mówi, jak daleko cosinus jest od progu zewnętrznego, mianownik to szerokość pasa przejścia. Na progu zewnętrznym wychodzi 0, na wewnętrznym 1, a `clamp` przycina wszystko poza pasem. Przejście jest liniowe **w cosinusie**, nie w kącie: dla wąskiego pasa różnicy nie widać.

Mianownik nie może być zerem. Gdy oba kąty są równe (stożek o ostrej krawędzi), `scene::coneCosines` ustawia cosinus wewnętrzny o `MIN_CONE_COSINE_GAP` (0,001) powyżej zewnętrznego (sekcja 5.4).

Jak duża jest plama latarki? Promień koła światła w odległości `d` to `d * tan(kąt)`: dla kątów startowych `0,23 * d` (stożek wewnętrzny) i `0,38 * d` (zewnętrzny). Na ścianie odległej o 2 m pełna jasność ma promień 0,46 m, a cała plama 0,77 m. Te liczby wracają w dokumencie tematu 7 przy pytaniu, dlaczego cieniowanie Gourauda gubi plamę latarki.

### 2.7 Macierz normalnych i dlaczego odwrotna transponowana

Normalna z pliku modelu jest w przestrzeni lokalnej. Światła i kamera są w przestrzeni świata, więc normalną też trzeba tam przenieść. Pozycję przenosi macierz modelu. Normalnej **nie wolno** mnożyć tą samą macierzą z dwóch powodów.

**Powód pierwszy: przesunięcie.** Normalna jest kierunkiem, a nie punktem. Przesunięcie obiektu o 10 m nie zmienia tego, w którą stronę patrzy jego ściana. Bierze się więc tylko lewą górną część 3 na 3 macierzy modelu (obrót i skala, bez czwartej kolumny z przesunięciem).

**Powód drugi: nierówna skala.** Część 3 na 3 jest poprawna, dopóki skala jest taka sama na wszystkich osiach. Przy nierównej skali normalna przestaje być prostopadła do powierzchni:

```text
   przed skalowaniem                 po rozciągnięciu 4 razy wzdłuż X

        N  ^                           tą samą macierzą:     właściwa normalna:
        \  |                            N' leży prawie         stoi na powierzchni
         \ | /  powierzchnia            na powierzchni
          \|/   (skos 45 stopni)       <------.                    ^
   --------+--------                           `--._____           |   ____---
                                        powierzchnia teraz        _|---
                                        jest prawie płaska      --
```

Rozciągnięcie obiektu wzdłuż osi X **kładzie** skośną powierzchnię (staje się bardziej pozioma). Jej normalna powinna się więc **podnieść**. Tymczasem ta sama macierz rozciąga normalną wzdłuż X, czyli też ją kładzie, w złą stronę.

**Wyprowadzenie.** Niech `M` będzie częścią 3 na 3 macierzy modelu, `t` dowolnym wektorem stycznym do powierzchni (leżącym w niej), a `n` normalną: `dot(n, t) = 0`. Styczna przekształca się jak pozycje: `t' = M t`. Szukam macierzy `G` dla normalnej, takiej że po przekształceniu nadal jest prostopadła:

```text
dot(G n, M t) = 0
(G n)^T (M t) = n^T (G^T M) t = 0      dla każdej stycznej t
```

To jest spełnione, gdy `G^T M` jest macierzą jednostkową, bo wtedy zostaje `n^T t`, czyli zero. Stąd:

```text
G^T = M^-1        czyli        G = (M^-1)^T
```

Macierz normalnych to **odwrotność części 3 na 3 macierzy modelu, transponowana** (inverse transpose).

Co z tego wynika:

- **Dla samego obrotu nic się nie zmienia.** Macierz obrotu jest ortogonalna: jej odwrotność jest jej transpozycją, więc odwrotna transponowana to ona sama.
- **Dla równej skali `s`** wychodzi obrót pomnożony przez `1 / s`: kierunek dobry, długość nie. Dlatego shader normalizuje wynik.
- **Dla nierównej skali** tylko ta macierz daje normalną prostopadłą.
- Macierz modelu musi być odwracalna: skala równa 0 na którejś osi nie ma odwrotności.

**W projekcie:** liczy ją funkcja `scene::normalMatrix` na procesorze, raz na obiekt, i wysyła jako uniform `uNormalMatrix`. Kod i cztery testy omawia [`transforms.md`](transforms.md), miejsce wywołania [`../game/maze-rendering.md`](../game/maze-rendering.md). Uczciwie: **dziś żaden obiekt sceny nie jest skalowany** (ściany i brama są tylko przesunięte i obrócone o 90 stopni, kryształy przesunięte i obracane wokół osi Y), więc macierz normalnych jest równa samej części obrotowej i obraz byłby taki sam z `mat3(uModel)`. Wzór jest pełny, żeby pierwszy rozciągnięty obiekt nie dostał po cichu złego światła. Poprawność przy nierównej skali sprawdza test jednostkowy, a nie obraz.

Dlaczego na procesorze, a nie `transpose(inverse(mat3(uModel)))` w shaderze: shader wierzchołków liczyłby odwrotność od nowa dla każdego wierzchołka, a wynik jest ten sam dla całego obiektu.

### 2.8 Cienie: księżyc i latarka je mają, kryształy nie. Co już jest: gamma i HDR

Do trzeciej części M7 ta sekcja nazywała się "Czego jeszcze nie ma: cienie" i mówiła, że żadne światło nie ma cienia. Od czwartej części M7 (cienie księżyca, 2026-10-05) dotyczyło to już tylko dwóch z trzech rodzajów świateł, a od piątej (cień latarki, 2026-10-06) dotyczy tylko świateł kryształów.

**Cienie.** Wzory z tej sekcji pytają tylko o kąt i odległość. Nie pytają, czy między światłem a punktem coś stoi. Funkcja `computeLighting` nadal liczy więc każde światło tak, jakby nic nie stało mu na drodze: komentarz w pliku mówi "This function knows nothing about shadows". Pytanie "czy coś zasłania" zadaje osobna technika, mapa cieni (temat 11 wykładu), i dziś mają ją **dwa** światła (do piątej części M7 jedno):

| Światło | Cień | Skutek |
|---|---|---|
| księżyc (kierunkowe) | **jest**, od czwartej części M7 | grunt u stóp ściany, która zasłania księżyc, traci jego światło i zostaje mu światło otoczenia (i to, co dochodzi od latarki i kryształów) |
| latarka (reflektor) | **jest**, od piątej części M7, z własną mapą o rzucie perspektywicznym (`scene::spotLightSpace`) i światłem w ręce gracza | plama latarki nie pada na to, co stoi za słupkiem albo za rogiem ściany. Do piątej części M7 pada: mapy nie było. Obrazu z cieniem latarki nikt jeszcze nie oglądał |
| światła kryształów (punktowe) | **nie ma** | światło kryształu rozjaśnia także grunt korytarza **za ścianą** |

Jak cień księżyca wchodzi do rachunku: `computeLighting` zwraca udział księżyca osobno (pola `moonDiffuse` i `moonSpecular`, sekcja 4.3), a wołający (`lit.frag`, `gouraud.frag`, `grass.frag`) pyta funkcję `moonShadow` z pliku `common/shadows.glsl`, jaka część światła księżyca nie dociera do fragmentu, i odejmuje tylko ją: `max(lighting.diffuse - lighting.moonDiffuse * shadow, 0.0)`. Światło otoczenia, światła kryształów i emisja nigdy nie są przyciemniane, a latarka tylko jej własnym cieniem (od piątej części M7, `flashlightShadow` z mapy latarki, odejmuje tylko `flashlightDiffuse` i `flashlightSpecular`). Mapa cieni, bias i filtrowanie: [`../renderer/shadows.md`](../renderer/shadows.md), które światło jest cieniowane i dlaczego tylko ono: sekcja 2.14 tamtego dokumentu i notatka [`../../decisions/shadow-takes-only-moon-light.md`](../../decisions/shadow-takes-only-moon-light.md). Brak cienia kryształów to nie błąd shadera, tylko brak kolejnych map cieni. Na obronie mówię to wprost.

**Gamma: stan do M6.** Tekstury były czytane tak, jak leżą w pliku, a wynik był zapisywany bez korekcji. Rachunek światła odbywał się więc na liczbach, które nie są proporcjonalne do jasności. Dlaczego tak było, zapisuje notatka [`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md), dziś zastąpiona.

**Gamma: stan od pierwszej części M7.** Mnożenie koloru przez światło i dodawanie świateł jest poprawne tylko na wartościach **liniowych**, czyli proporcjonalnych do ilości światła. Dlatego wszystko, co wchodzi do wzorów z tej sekcji, jest dziś liniowe:

| Co wchodzi do wzoru | Skąd jest liniowe |
|---|---|
| kolor powierzchni z tekstury | tekstura koloru ma format sRGB (`GL_SRGB8`), więc karta dekoduje ją przy odczycie |
| kolory świateł i światła otoczenia w bloku `LightBlock` | w ustawieniach (`game::LightingSettings`) są wartościami sRGB, takimi, jakie pokazuje próbnik koloru. `game::buildLightSet` przelicza każdy z czterech kolorów raz, funkcją `gfx::srgbToLinear` ([`../game/flashlight.md`](../game/flashlight.md)) |
| intensywności | to zwykłe mnożniki, nie kolory: nie są przeliczane |
| blask kryształu `uEmissive` | liczony z liniowego koloru świateł punktowych (`NightMazeApp::crystalEmissive`, [`../game/gameplay.md`](../game/gameplay.md)) |

Wynik wzoru jest zapisywany bez żadnego przeliczenia do bufora HDR sceny. Kodowanie do sRGB (korekcja gamma) dzieje się raz, na końcu klatki, w przebiegu składającym, po ekspozycji i mapowaniu tonów. Pełna teoria, funkcja sRGB z liczbami i lista miejsc, w których kolory są przeliczane: [`../gfx/color-space.md`](../gfx/color-space.md). Przebieg składający: [`../renderer/post-process.md`](../renderer/post-process.md). Decyzja: [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md).

**Wartości startowe świateł po tej zmianie.** Stare liczby były dobrane do obrazu bez gammy, więc dobrano je od nowa:

| Pole `LightingSettings` | Do M6 | Od pierwszej części M7 | Co naprawdę trafia do shadera (liniowe) |
|---|---|---|---|
| `ambient` | `(0,035, 0,045, 0,075)` | `(0,105, 0,135, 0,225)` | `(0,0108, 0,0163, 0,0414)` |
| `moonColor`, `moonIntensity` | `(0,55, 0,65, 1,0)` i 0,3 | ten sam kolor i 0,12, a od czwartej części M7 0,2 | kolor `(0,263, 0,380, 1,0)` razy 0,2, czyli około `(0,053, 0,076, 0,2)` (policzone; do trzeciej części M7 razy 0,12) |
| `flashlightColor`, `flashlightIntensity` | `(1,0, 0,9, 0,72)` i 1,6 | ten sam kolor i 1,3 | kolor `(1,0, 0,787, 0,477)` razy 1,3 |
| `pointColor`, `pointIntensity` | `(0,2, 0,9, 0,8)` i 2,0 | ten sam kolor i 0,9 | kolor `(0,033, 0,787, 0,604)` razy 0,9 |

Dwie rzeczy widać w tej tabeli. Po pierwsze liczba sRGB wygląda na dużo większą niż światło, które oznacza: 0,105 to po przeliczeniu 0,0108, czyli około jednej setnej bieli. Po drugie latarka w osi stożka i blisko ściany daje w czerwonym kanale 1,3, czyli więcej niż biel. Do M6 byłoby to obcięte do 1. Dziś zostaje w buforze jako 1,3, a krzywa mapowania tonów decyduje, jak to pokazać.

**Księżyc 0,2 zamiast 0,12 (czwarta część M7).** Odkąd księżyc rzuca cień, jego intensywność decyduje o tym, jak wyraźny jest ten cień: w cieniu ściany zostaje samo światło otoczenia, a obok niego jest otoczenie plus księżyc. Komentarz w `src/game/Lighting.hpp` mówi, że intensywność jest niska celowo (to noc), ale na tyle wysoka, żeby powierzchnia w świetle księżyca była wyraźnie jaśniejsza od tej w cieniu ściany: "about five times on level ground". Rachunek dla poziomego gruntu (policzone, wartości liniowe, sam księżyc i otoczenie, bez latarki i kryształów): czynnik Lamberta to `-sin(-50) = 0,766`, więc księżyc daje `(0,263, 0,380, 1,0) * 0,2 * 0,766 = (0,0403, 0,0582, 0,1532)`. Razem z otoczeniem `(0,0108, 0,0163, 0,0414)` to `(0,0511, 0,0745, 0,1946)`, czyli w kolejnych kanałach 4,73, 4,56 i 4,70 razy więcej niż samo otoczenie w cieniu. "Około pięć razy" z komentarza to zaokrąglenie w górę: dokładniej jest od około 4,6 do 4,7 razy. Dla intensywności 0,12 ten sam rachunek daje od około 3,1 do 3,2 razy (policzone dla 0,12).

### 2.9 Która normalna trafia do wzorów: siatka albo mapa normalnych

Wszystkie wzory tej sekcji biorą normalną `N` jako daną. Funkcja `computeLighting(vec3 position, vec3 normal)` nie wie, skąd wołający ją wziął, i to jest celowe: źródło normalnej można wymienić bez dotykania wzorów. W projekcie są trzy przypadki:

| Program | Skąd normalna | Ile różnych normalnych na licu ściany |
|---|---|---|
| `gouraud` | atrybut `aNormal` razy macierz normalnych, znormalizowany w shaderze wierzchołków | jedna (cztery wierzchołki lica mają tę samą) |
| `lit`, mapowanie normalnych wyłączone | ta sama normalna, interpolowana i znormalizowana w shaderze fragmentów | jedna |
| `lit`, mapowanie normalnych włączone (stan startowy) | **mapa normalnych** (normal map): tekstura, której każdy teksel przechowuje kierunek normalnej w przestrzeni stycznej. Shader przenosi go do przestrzeni świata macierzą TBN | inna dla każdego teksela |

W `lit.frag` jest to jedna linia: `vec3 normal = surfaceNormal(vNormal, vTangent, vUv);`. Funkcja `surfaceNormal` leży w osobnym pliku dołączanym `common/normal_map.glsl` i zwraca normalną w przestrzeni świata, o długości 1, czyli dokładnie to, czego `computeLighting` wymaga. Dzięki mapie płaskie lico ściany dostaje pod światłem fugi i nierówności: geometria się nie zmienia, zmienia się tylko `dot(N, L)` w każdym fragmencie. Całą technikę (kodowanie, przestrzeń styczna, wyliczanie stycznych, macierz TBN linia po linii) omawia [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md).

**Gouraud nie ma mapowania normalnych.** Mapa normalnych przechowuje jedną normalną na teksel, a `gouraud.vert` liczy światło w wierzchołkach, 4 na lico ściany: teksel leżący między nimi nie ma jak wziąć udziału w obliczeniach. Komentarz w `gouraud.vert` mówi to wprost ([`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), sekcje 2.7 i 4.3). Regułę "włączone i tryb inny niż `Gouraud`" zapisuje funkcja `game::usesNormalMap` ([`../game/flashlight.md`](../game/flashlight.md), sekcja 5.2).

**Podgląd `Normals as colour`** z panelu Assets pokazuje normalną faktycznie użytą: z map normalnych w trybach `Unlit`, `Phong` i `Blinn-Phong` przy zaznaczonym polu `Normal mapping`, a normalną siatki w trybie `Gouraud` albo przy polu odznaczonym.

## 3. Jak to działa w OpenGL

OpenGL w profilu Core **nie ma świateł**. Stare funkcje `glLight` i `glMaterial` należały do potoku stałego i w profilu Core nie istnieją. Światło to dziś zwykłe dane, które program sam wysyła do własnych shaderów, i zwykła arytmetyka w GLSL.

| Co | Jak trafia do shadera | Wywołania | Ile razy na klatkę |
|---|---|---|---|
| wszystkie światła i pozycja oka | jeden blok uniformów `LightBlock`, 928 bajtów | `glBindBuffer`, `glBufferSubData` (w `gfx::UniformBuffer::update`) | 1 |
| model odbłysku, siła, połysk | zwykłe uniformy `uSpecularModel`, `uSpecularStrength`, `uShininess` | `glUniform1i`, `glUniform1f` | po 1, w programie, który rysuje labirynt |
| macierz normalnych | zwykły uniform `uNormalMatrix` | `glUniformMatrix3fv` | raz na obiekt |
| emisja powierzchni (sekcja 2.2) | zwykły uniform `uEmissive` | `glUniform3fv` | raz w `MazeRenderer::draw` (czerń) i do dwóch razy w `GameplayRenderer::draw` (czerń dla bramy, blask dla kryształów) |
| normalna wierzchołka | atrybut numer 1 w `gfx::Vertex` | ustawione raz w VAO siatki | 0 |
| styczna wierzchołka (tylko dla mapowania normalnych) | atrybut numer 3 w `gfx::Vertex` | ustawione raz w VAO siatki | 0 |
| mapa normalnych i jej przełącznik (tylko program `lit`) | tekstura na jednostce 1, uniformy `uNormalMap` i `uNormalMapEnabled` | `glActiveTexture`, `glBindTexture`, `glBindSampler`, `glUniform1i` | raz na część modelu i po 1 ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 3) |
| mapa cieni księżyca (od czwartej części M7; programy `lit`, `gouraud` i `grass`, od M8, części 1 także `reflect`) | tekstura głębi z samplerem porównującym na jednostce 3 (`MOON_SHADOW_TEXTURE_UNIT`) i siedem zwykłych uniformów `uMoonShadow...` | wiązanie w `ShadowMap::bindForSampling`, uniformy w `game::setShadowUniforms` (od piątej części M7 wołanej dwa razy, dla księżyca i dla latarki, przez `NightMazeApp::setShadowUniformsOf`): trzy razy `glUniform1i`, raz `glUniformMatrix4fv`, trzy razy `glUniform1f` | wiązanie raz, uniformy raz w `drawLitMaze` i raz w `drawGrass`, gdy trawa jest włączona ([`../renderer/shadows.md`](../renderer/shadows.md), sekcje 3 i 4) |

Blok uniformów czytają oba programy oświetlenia (`lit` i `gouraud`), a od M6 także program trawy `grass`, wszystkie z tego samego bufora na karcie. Dlaczego blok, a nie 60 osobnych uniformów, i jak bajty z C++ trafiają dokładnie tam, gdzie shader ich szuka, omawia [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md).

**Blok `LightBlock` nie zmienił się w czwartej części M7.** Ma nadal 928 bajtów i te same składowe, chociaż księżyc dostał mapę cieni. Macierz, którą shader przenosi punkt do przestrzeni księżyca (`uMoonShadowMatrix`), i liczby porównania (bias, promień PCF, siła cienia) podróżują jako zwykłe uniformy, ustawiane osobno w każdym programie. Powody są dwa. Sampler nie może być składową bloku uniformów, więc `uMoonShadowMap` i tak musi być zwykłym uniformem. A liczby jednej mapy cieni trzymam obok jej samplera, w jednym pliku (`common/shadows.glsl`) i w jednej funkcji C++ (`game::setShadowUniforms`), zamiast dzielić je między blok a program. Komentarz w `common/shadows.glsl` mówi to tak: "A sampler cannot be a member of a uniform block, so the numbers that belong to the map stay next to it instead of joining the light block". Decyzja: [`../../decisions/shadow-matrix-as-plain-uniforms.md`](../../decisions/shadow-matrix-as-plain-uniforms.md).

Wszystkie obliczenia światła są w **przestrzeni świata**: pozycje świateł, pozycja oka, pozycja fragmentu i normalna (także ta z mapy normalnych: `surfaceNormal` przenosi ją z przestrzeni stycznej do świata, zanim trafi do wzorów). Drugą częstą konwencją jest przestrzeń widoku (oko w punkcie zero). Wybrałem świat, bo światła gry są zdefiniowane w świecie (komórki labiryntu) i nie trzeba ich co klatkę mnożyć przez macierz widoku.

## 4. Shadery

Wzory z sekcji 2 są w jednym pliku: [`assets/shaders/common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl). **To nie jest samodzielny shader.** Nie ma linii `#version` ani funkcji `main`. Loader shaderów wstawia jego tekst w miejsce linii `#include "common/lighting.glsl"` w czterech plikach: `lit.frag` i, od M6, `grass.frag` (światło liczone dla każdego fragmentu), od M8, części 1 `reflect.frag` (też dla każdego fragmentu, z nagłówkiem pliku, który go nie wymienia) oraz `gouraud.vert` (dla każdego wierzchołka). Jeden plik, cztery miejsca użycia (cztery programy: `lit`, `grass`, `reflect`, `gouraud`): oba tryby cieniowania ścian liczą dokładnie tymi samymi wzorami, a różni je tylko miejsce wywołania. Pierwsze linie pliku mówią to od czwartej części M7 wprost:

```glsl
// Lighting shared by lit.frag and grass.frag (per fragment) and gouraud.vert (per
// vertex): the light block and the functions that turn the lights into the brightness of
// one surface point.
// This file is not a shader of its own. It has no #version line: the shader loader puts
// its text in place of the line  #include "common/lighting.glsl"  (gfx/ShaderSource.hpp).
// See docs/modules/scene/lights.md
```

Do trzeciej części M7 ten nagłówek wymieniał tylko `lit.frag` i `gouraud.vert`, choć trawa dołączała plik już od M6. Mechanizm dołączania: [`../gfx/shader-includes.md`](../gfx/shader-includes.md). Shadery, które ten plik dołączają: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md).

### 4.1 Stała i struktura światła punktowego

```glsl
// Length of the array of point lights. The same number as scene::MAX_POINT_LIGHTS in
// src/scene/Light.hpp.
const int MAX_POINT_LIGHTS = 16;

// One point light. vec4 everywhere, so that every member is 16 bytes and the C++ struct
// scene::PointLightData has the same layout without any hidden gaps.
struct PointLight {
    vec4 position;    // xyz: position in world space
    vec4 color;       // rgb: colour, a: intensity
    vec4 attenuation; // x: constant, y: linear, z: quadratic term
};
```

| Linia | Znaczenie |
|---|---|
| `const int MAX_POINT_LIGHTS = 16;` | stała czasu kompilacji GLSL: długość tablicy świateł. Ta sama liczba jest w C++ (`scene::MAX_POINT_LIGHTS`). Zgodności obu pilnuje pośrednio sprawdzenie rozmiaru bloku (pułapka 6) |
| `struct PointLight` | struktura GLSL: trzy `vec4`. Pozycja i kolor potrzebują trzech liczb, czwarta niesie dodatek (intensywność w `color.a`) albo nic. Powód użycia `vec4` zamiast `vec3` to układ `std140` ([`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md)) |
| `.rgb`, `.a`, `.xyz` | GLSL pozwala czytać składowe wektora pod nazwami `xyzw` albo `rgba`. To te same cztery liczby: `color.a` i `color.w` to to samo |

### 4.2 Blok świateł i uniformy materiału

```glsl
layout(std140) uniform LightBlock {
    vec4 uCameraPosition;       // xyz: the eye in world space
    vec4 uAmbient;              // rgb: light that reaches every surface
    vec4 uDirectionalDirection; // xyz: the way the moon light travels, length 1
    vec4 uDirectionalColor;     // rgb: colour, a: intensity
    vec4 uSpotPosition;         // xyz: the flashlight in world space
    vec4 uSpotDirection;        // xyz: the axis of its cone, length 1
    vec4 uSpotColor;            // rgb: colour, a: intensity
    vec4 uSpotAttenuation;      // x: constant, y: linear, z: quadratic term
    vec4 uSpotCone;             // x: cos(inner angle), y: cos(outer angle), z: 1 on, 0 off
    int uPointCount;            // how many elements of uPoints are in use
    PointLight uPoints[MAX_POINT_LIGHTS];
};
```

Blok uniformów (uniform block) to grupa uniformów, których wartości nie są ustawiane pojedynczo przez `glUniform`, tylko czytane z bufora na karcie. Składowych używa się w shaderze jak zwykłych uniformów, bez przedrostka: `uAmbient`, a nie `LightBlock.uAmbient`. Pełna tabela przesunięć i kod wypełniający blok są w [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md). Tu ważne jest znaczenie pól:

| Składowa | Co niesie | Kto to liczy w C++ |
|---|---|---|
| `uCameraPosition.xyz` | oko w przestrzeni świata: potrzebne do odbłysku | interpolowana pozycja oka z `NightMazeApp::onRender` |
| `uAmbient.rgb` | światło otoczenia | `LightingSettings::ambient` |
| `uDirectionalDirection.xyz` | kierunek, w którym światło księżyca **leci**, długość 1 | `directionFromAngles`, znormalizowane w `packLightBlock` |
| `uDirectionalColor` | `rgb`: kolor, `a`: intensywność | ustawienia księżyca |
| `uSpotPosition.xyz`, `uSpotDirection.xyz` | latarka: wierzchołek stożka i jego oś, długość 1 | oko i `Camera::forward()` |
| `uSpotColor`, `uSpotAttenuation` | kolor z intensywnością, trzy współczynniki tłumienia | ustawienia latarki, `attenuationForRadius`. Intensywność jest przy słabej baterii pomnożona przez mnożnik migotania (`game::lightingForFrame`) |
| `uSpotCone` | `x`: cosinus kąta wewnętrznego, `y`: zewnętrznego, `z`: 1 włączona, 0 wyłączona | `coneCosines`, przełącznik latarki. Przy pustej baterii zawsze 0 |
| `uPointCount` | ile elementów `uPoints` jest w użyciu | liczba kryształów, które nie są jeszcze zebrane, najwyżej 16 |
| `uPoints[i]` | pozycja, kolor z intensywnością, tłumienie jednego światła punktowego | `buildLightSet` z pozycji od `game::crystalLightPositions`. Intensywność jest pomnożona przez puls kryształów |

Kierunki są normalizowane **w C++**, więc shader nie woła dla nich `normalize`. Przełącznik latarki to liczba `float` (1 albo 0), a nie `bool`: powód jest w dokumencie o buforach uniformów.

```glsl
uniform int uSpecularModel;
// How bright the highlight is compared with the light that makes it (0: no highlight).
uniform float uSpecularStrength;
// The exponent of the highlight: a larger number gives a smaller, sharper highlight.
uniform float uShininess;
```

Te trzy to **zwykłe uniformy**, poza blokiem: opisują materiał i sposób liczenia, a nie światła. Ustawia je `NightMazeApp::drawLitMaze` przez `setInt` i `setFloat` po `use()` właściwego programu. `uSpecularModel` ma wartości typu `game::SpecularModel`: 0 to Phong, 1 to Blinn-Phong.

### 4.3 Struktura wyniku

```glsl
// The light that reaches one point of a surface, in two parts, because they are used
// differently: diffuse is multiplied by the colour of the surface (the texture), the
// highlight is added on top and keeps the colour of the light.
//
// The share of the moon is ALSO kept on its own. diffuse and specular already contain
// it. The moon is the light with a shadow map: where a surface lies in its shadow, the
// caller takes that share away again (see common/shadows.glsl and the main function of
// lit.frag). Nothing else is ever taken away, so a shadow of the moon never darkens the
// ambient light, the flashlight, the crystals or a glowing surface.
struct Lighting {
    vec3 diffuse;      // ambient light plus the Lambert term of every light
    vec3 specular;     // the highlight of every light
    vec3 moonDiffuse;  // the part of diffuse that comes from the moon
    vec3 moonSpecular; // the part of specular that comes from the moon
};
```

Wynik ma dwie części, bo są używane inaczej (sekcja 2.2): `diffuse` jest mnożone przez kolor powierzchni, `specular` dodawane na wierzch. Gdyby funkcja zwracała jedną sumę, shader nie mógłby już pomnożyć przez teksturę tylko jednej z nich. Światło otoczenia jest wliczone w `diffuse`.

**Dwa pola księżyca (czwarta część M7).** Do trzeciej części M7 struktura miała tylko `diffuse` i `specular`. Dziś ma cztery pola:

| Pole | Co zawiera | Kto go używa |
|---|---|---|
| `diffuse` | otoczenie plus część rozproszona **wszystkich** świateł, razem z księżycem | każdy wołający |
| `specular` | odbłysk **wszystkich** świateł, razem z księżycem | `lit.frag` i `gouraud.vert` (trawa odbłysku nie używa) |
| `moonDiffuse` | sama część rozproszona księżyca: to, co księżyc dołożył do `diffuse` | wołający, przy odejmowaniu cienia |
| `moonSpecular` | sam odbłysk księżyca: to, co księżyc dołożył do `specular` | wołający, przy odejmowaniu cienia |

Najważniejsze zdanie komentarza: "diffuse and specular already contain it". Pola księżyca **nie są** trzecim i czwartym składnikiem do dodania, tylko kopią tego, co już jest w sumie. Służą do jednego: tam, gdzie fragment leży w cieniu księżyca, wołający odejmuje z sumy dokładnie tyle, ile księżyc do niej dołożył (pomnożone przez udział cienia od 0 do 1). Dzięki temu cień księżyca nie może przyciemnić niczego poza światłem księżyca. Gdyby cień mnożył całe `diffuse`, w cieniu ściany gasłaby też latarka i światło otoczenia, a kąty labiryntu byłyby czarne. Kod wołających: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), sekcja 4, funkcja `moonShadow`: [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 4.

### 4.4 `diffuseFactor`, `specularFactor`, `attenuationFactor`

```glsl
float diffuseFactor(vec3 normal, vec3 toLight) {
    return max(dot(normal, toLight), 0.0);
}
```

Prawo Lamberta z sekcji 2.3 w jednej linii. Oba argumenty muszą mieć długość 1.

```glsl
float specularFactor(vec3 normal, vec3 toLight, vec3 toEye) {
    // A surface that faces away from the light has no highlight either.
    if (dot(normal, toLight) <= 0.0) {
        return 0.0;
    }

    float cosine = 0.0;
    if (uSpecularModel == 0) {
        // Phong: reflect() mirrors the incoming ray (from the light to the surface,
        // hence the minus) at the normal. The highlight is strongest where the
        // mirrored ray goes straight into the eye.
        vec3 reflected = reflect(-toLight, normal);
        cosine = dot(reflected, toEye);
    } else {
        // Blinn-Phong: the halfway vector points exactly between the light and the
        // eye. The highlight is strongest where the normal points along it. The angle
        // measured here is about half of the Phong one, so with the same exponent the
        // highlight is wider, and it does not cut off when the light and the eye are
        // far apart (Phong: more than 90 degrees between reflected and toEye gives 0).
        vec3 halfway = normalize(toLight + toEye);
        cosine = dot(normal, halfway);
    }
    // max() before pow(): the power of a negative number is not defined in GLSL.
    return uSpecularStrength * pow(max(cosine, 0.0), uShininess);
}
```

| Linia | Znaczenie |
|---|---|
| `if (dot(normal, toLight) <= 0.0) return 0.0;` | powierzchnia odwrócona od światła nie ma odbłysku. Bez tej linii wzór Blinna-Phonga potrafi dać odbłysk na stronie, do której światło nie dociera: wektor połówkowy zależy od oka i może być bliski normalnej, chociaż światło jest za powierzchnią |
| `reflect(-toLight, normal)` | funkcja wbudowana GLSL: odbija wektor **padający** względem normalnej. `toLight` wskazuje od powierzchni do światła, a promień padający leci odwrotnie, stąd minus. Wynik wskazuje od powierzchni, w stronę lustrzanego odbicia |
| `cosine = dot(reflected, toEye);` | Phong: cosinus kąta między odbitym promieniem a kierunkiem do oka |
| `normalize(toLight + toEye)` | suma dwóch wektorów o długości 1 wskazuje dokładnie między nimi, ale ma inną długość, więc trzeba ją znormalizować |
| `cosine = dot(normal, halfway);` | Blinn-Phong: cosinus kąta między normalną a wektorem połówkowym |
| `pow(max(cosine, 0.0), uShininess)` | potęga z sekcji 2.4. `max` przed `pow`: wynik `pow(x, y)` dla ujemnego `x` jest w GLSL niezdefiniowany |
| `uSpecularStrength * ...` | siła odbłysku z panelu |

Geometrię obu wzorów i porównanie na liczbach omawia [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md).

```glsl
float attenuationFactor(vec4 terms, float lightDistance) {
    return 1.0 / (terms.x + terms.y * lightDistance + terms.z * lightDistance * lightDistance);
}
```

Wzór z sekcji 2.5. `terms.x`, `.y`, `.z` to `constant`, `linear`, `quadratic`. Ten sam wzór ma w C++ `scene::attenuationFactor` (sekcja 5.3) i to jego sprawdzają testy.

**`moonFacing` (czwarta część M7).** Między `attenuationFactor` a `addLight` stoi od tej części czwarta mała funkcja:

```glsl
// How much a surface with this normal (length 1) faces the moon: the cosine of the angle
// between the normal and the direction to the moon. 1 facing it, 0 grazed by its light,
// below 0 facing away. The shadow bias grows as this number falls (common/shadows.glsl).
float moonFacing(vec3 normal) {
    return dot(normal, -uDirectionalDirection.xyz);
}
```

| Element | Znaczenie |
|---|---|
| `dot(normal, -uDirectionalDirection.xyz)` | ten sam iloczyn skalarny co w prawie Lamberta dla księżyca, ale **bez `max`**: wynik może być ujemny (powierzchnia odwrócona od księżyca). Minus z tego samego powodu co w `computeLighting`: `uDirectionalDirection` to kierunek lotu światła, a potrzebny jest kierunek do światła |
| wynik 1, 0, poniżej 0 | powierzchnia zwrócona wprost do księżyca, muśnięta jego światłem, odwrócona |
| po co osobna funkcja | wynik nie trafia do światła, tylko do **biasu cienia**: im bardziej powierzchnia jest pochylona względem księżyca, tym większą poprawkę głębi dostaje przy porównaniu z mapą cieni (`slopeScaledBias` w `common/shadows.glsl`, która sama obcina wartość do zakresu od 0 do 1). Funkcja leży w tym pliku, a nie w `shadows.glsl`, bo czyta składową bloku świateł |

Wołają ją cztery miejsca, każde z inną normalną (od M8, części 1 także `reflect.frag`, z normalną modelu jak `lit.frag`): `lit.frag` z normalną **modelu** (`moonFacing(normalize(vNormal))`, nie z mapy normalnych), `gouraud.vert` z normalną wierzchołka (wynik idzie do fragmentów jako `vMoonFacing`) i `grass.frag` ze stałą `GRASS_NORMAL`. Teoria biasu: [`../renderer/shadows.md`](../renderer/shadows.md), sekcje 2.10 i 2.11.

### 4.5 `addLight`: jedno światło do wyniku

```glsl
void addLight(inout Lighting lighting, vec3 normal, vec3 toLight, vec3 toEye, vec3 radiance) {
    lighting.diffuse += radiance * diffuseFactor(normal, toLight);
    lighting.specular += radiance * specularFactor(normal, toLight, toEye);
}
```

| Element | Znaczenie |
|---|---|
| `inout Lighting lighting` | GLSL nie ma referencji ani wskaźników. Kwalifikator `inout` znaczy: wartość jest kopiowana do funkcji przy wejściu i z powrotem przy wyjściu, więc funkcja może dopisać do przekazanej struktury |
| `radiance` | kolor światła **takiego, jakie dociera do punktu**: kolor razy intensywność, już po tłumieniu i po stożku. Dzięki temu `addLight` nie wie, z jakiego rodzaju światła pochodzi |
| dwie linie | część rozproszona i odbłysk tego samego światła, obie w jego kolorze |

To jest miejsce z diagramu w sekcji 2.1: rodzaje świateł schodzą się do jednej funkcji. Od czwartej części M7 wołają ją światła punktowe i latarka, a księżyc liczy te same dwa wyrazy sam, żeby je zachować (sekcja 4.6).

### 4.6 `computeLighting`: wszystkie światła dla jednego punktu

```glsl
// The light at one point of a surface. position and normal are in world space, normal
// has length 1. lit.frag and grass.frag call this per fragment, gouraud.vert per vertex:
// the same function, and the lit and the gouraud program differ only in where it runs.
// This function knows nothing about shadows: it computes every light as if nothing stood
// in its way. The shadow of the moon is applied by the caller, with the two moon fields
// of the result.
Lighting computeLighting(vec3 position, vec3 normal) {
    vec3 toEye = normalize(uCameraPosition.xyz - position);

    Lighting lighting;
    lighting.diffuse = uAmbient.rgb;
    lighting.specular = vec3(0.0);
```

`position` i `normal` są w przestrzeni świata, `normal` ma długość 1 (dba o to wołający: `gouraud.vert` przez `normalize`, `lit.frag` przez funkcję `surfaceNormal`, która zwraca normalną siatki albo normalną z mapy normalnych, sekcja 2.9). `toEye` to wektor `V`: od punktu do oka. Suma zaczyna od światła otoczenia i zerowego odbłysku. Pola `moonDiffuse` i `moonSpecular` nie są tu zerowane: dostają wartość w następnym kroku, bezwarunkowo.

Komentarz nad funkcją miał do trzeciej części M7 zdanie "There are no shadows yet: a light also reaches surfaces that stand behind a wall". Dziś mówi co innego: funkcja nadal nic nie wie o cieniach, ale cień księżyca nakłada wołający, z dwóch pól księżyca. Komentarz wymienia wszystkich trzech wołających: `lit.frag` i `grass.frag` na fragment, `gouraud.vert` na wierzchołek.

```glsl
    // The moon, a directional light: the same direction everywhere, no attenuation.
    // uDirectionalDirection is the way the light travels, so the way TO the light is
    // the opposite. These lines do what addLight does, and keep the two terms.
    vec3 toMoon = -uDirectionalDirection.xyz;
    vec3 moonRadiance = uDirectionalColor.rgb * uDirectionalColor.a;
    lighting.moonDiffuse = moonRadiance * diffuseFactor(normal, toMoon);
    lighting.moonSpecular = moonRadiance * specularFactor(normal, toMoon, toEye);
    lighting.diffuse += lighting.moonDiffuse;
    lighting.specular += lighting.moonSpecular;
```

**Księżyc.** `uDirectionalDirection` to kierunek, w którym światło leci, więc kierunek **do** światła jest przeciwny: minus. Nie ma pozycji, nie ma odległości, nie ma tłumienia: `moonRadiance` to kolor razy intensywność. Księżyca nie da się wyłączyć przełącznikiem: wyłącza go intensywność 0.

Do trzeciej części M7 były tu dwie linie: `addLight(lighting, normal, -uDirectionalDirection.xyz, toEye, uDirectionalColor.rgb * uDirectionalColor.a);`. Od czwartej części M7 gałąź księżyca **nie woła `addLight`**, tylko robi to samo ręcznie:

| Linia | Znaczenie |
|---|---|
| `vec3 toMoon = -uDirectionalDirection.xyz;` | kierunek do księżyca, wektor `L` |
| `vec3 moonRadiance = uDirectionalColor.rgb * uDirectionalColor.a;` | kolor razy intensywność: przy ustawieniach startowych około `(0,053, 0,076, 0,2)` (policzone) |
| `lighting.moonDiffuse = moonRadiance * diffuseFactor(normal, toMoon);` | część rozproszona księżyca, zapisana **osobno**. Ten sam wzór co pierwsza linia `addLight` |
| `lighting.moonSpecular = moonRadiance * specularFactor(normal, toMoon, toEye);` | odbłysk księżyca, zapisany osobno. Ten sam wzór co druga linia `addLight` |
| `lighting.diffuse += lighting.moonDiffuse;` i `lighting.specular += lighting.moonSpecular;` | oba wyrazy trafiają też do sumy, jak u każdego innego światła |

Powód: `addLight` dopisuje wynik wprost do sumy i nie zostawia po sobie obu wyrazów, a wołający potrzebuje ich osobno, żeby odjąć cień (sekcja 4.3). Wynik w polach `diffuse` i `specular` jest ten sam co przed zmianą: przy wyłączonych cieniach obraz nie różni się od obrazu sprzed tej części (zgłoszone dla Windowsa, 2026-10-05, z księżycem ustawionym z powrotem na 0,12: piksele identyczne poza pasem HUD, w trybie Phong różnica najwyżej 1/255). Światła punktowe i latarka nadal idą przez `addLight`, więc diagram z sekcji 2.1 pozostaje prawdziwy dla wzorów, a w kodzie do wspólnej funkcji schodzą się dziś dwa rodzaje świateł z trzech.

```glsl
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= uPointCount) {
            break;
        }
        vec3 offset = uPoints[i].position.xyz - position;
        float lightDistance = length(offset);
        vec3 radiance = uPoints[i].color.rgb * uPoints[i].color.a *
                        attenuationFactor(uPoints[i].attenuation, lightDistance);
        addLight(lighting, normal, offset / lightDistance, toEye, radiance);
    }
```

**Światła punktowe.**

| Linia | Znaczenie |
|---|---|
| `for (int i = 0; i < MAX_POINT_LIGHTS; ++i)` i `if (i >= uPointCount) break;` | pętla ma stałą górną granicę i wychodzi wcześniej. Komentarz w pliku mówi dokładnie, skąd ta forma: **GLSL 4.10 tego nie wymaga** ("GLSL 4.10 does not require that (the rule comes from GLSL ES 1.00), but a constant limit with an early exit is the form every compiler accepts") i `i < uPointCount` byłoby tu poprawne. Wymóg pętli o długości znanej przy kompilacji ma GLSL ES 1.00 (stare urządzenia mobilne i WebGL 1). Forma ze stałą granicą i wcześniejszym wyjściem jest tą, którą przyjmie każdy kompilator, i ma jeszcze jedną zaletę: pętla nigdy nie wyjdzie poza tablicę, nawet gdyby w `uPointCount` znalazły się śmieci |
| `offset = pozycja światła - position` | wektor od punktu do światła, jeszcze nie znormalizowany |
| `lightDistance = length(offset)` | jego długość: odległość do światła w metrach |
| `radiance = kolor * intensywność * tłumienie` | światło, które dociera na tę odległość |
| `offset / lightDistance` | to samo co `normalize(offset)`, ale długość jest już policzona, więc dzielenie jest tańsze |

```glsl
    if (uSpotCone.z > 0.5) {
        vec3 offset = uSpotPosition.xyz - position;
        float lightDistance = length(offset);
        vec3 toLight = offset / lightDistance;

        // The cosine of the angle between the axis of the cone and the ray from the
        // light to this point. 1 on the axis, smaller towards the side.
        float cosAngle = dot(-toLight, uSpotDirection.xyz);
        // 1 inside the inner cone, 0 outside the outer one, a ramp in between: the soft
        // edge. The same formula as scene::spotFactor. The C++ code makes sure that the
        // two cosines differ, so this never divides by zero.
        float cone = clamp((cosAngle - uSpotCone.y) / (uSpotCone.x - uSpotCone.y), 0.0, 1.0);

        vec3 radiance = uSpotColor.rgb * uSpotColor.a * cone *
                        attenuationFactor(uSpotAttenuation, lightDistance);
        addLight(lighting, normal, toLight, toEye, radiance);
    }

    return lighting;
}
```

**Latarka.**

| Linia | Znaczenie |
|---|---|
| `if (uSpotCone.z > 0.5)` | przełącznik: 1 włączona, 0 wyłączona. Porównanie z 0,5, a nie `== 1.0`: liczb zmiennoprzecinkowych nie porównuje się na równość |
| `offset`, `lightDistance`, `toLight` | jak dla światła punktowego |
| `dot(-toLight, uSpotDirection.xyz)` | `toLight` wskazuje od punktu do latarki. Oś stożka wskazuje od latarki w scenę. Żeby zmierzyć kąt między osią a promieniem **od latarki do punktu**, trzeba `toLight` odwrócić: stąd minus |
| `cone = clamp(...)` | miękki brzeg z sekcji 2.6. `uSpotCone.x` to cosinus wewnętrzny, `.y` zewnętrzny |
| `radiance = kolor * intensywność * cone * tłumienie` | dwa osłabienia naraz: stożek i odległość |

Poza stożkiem `cone` wynosi 0, więc `radiance` jest zerem i `addLight` niczego nie dodaje. Shader nie pomija wtedy obliczeń: liczy i mnoży przez zero.

**Bez cieni w tej funkcji.** Funkcja nie sprawdza, czy coś stoi między światłem a punktem (sekcja 2.8). Dla latarki i świateł kryształów nikt tego nie sprawdza. Dla księżyca robi to wołający, po powrocie z funkcji.

### 4.7 Czego w tym pliku nie ma: `uEmissive`

Składnik emisyjny z sekcji 2.2 **nie jest** częścią `common/lighting.glsl`. Struktura `Lighting` nie ma dla niej pola (ma cztery: dwa wspólne i dwa księżyca, sekcja 4.3), a `computeLighting` nie wie nic o tym, czy powierzchnia sama świeci. Emisja dochodzi dopiero w shaderze fragmentów, w linii, która łączy światło z kolorem powierzchni:

```glsl
uniform vec3 uEmissive;
```

```glsl
    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    fragColor = vec4(surface * (diffuse + uEmissive) + specular, 1.0);
```

To fragment `lit.frag`. Zmienne `diffuse` i `specular` to od czwartej części M7 światło **po odjęciu cienia księżyca** (`max(lighting.diffuse - lighting.moonDiffuse * shadow, 0.0)` i to samo dla odbłysku). Do trzeciej części M7 stały tu wprost `lighting.diffuse` i `lighting.specular`. `gouraud.frag` ma dziś tę samą linię co do znaku, a jego `diffuse` i `specular` powstają z wartości interpolowanych `vDiffuseLight` i `vSpecularLight`. Emisja stoi w nawiasie obok światła już po odjęciu cienia, więc cień księżyca nigdy jej nie przyciemnia. Emisja jest jedną stałą dla całego rysowanego obiektu i nie zależy od żadnego światła, więc kryształ świeci tak samo w trybie `Gouraud` i w trybie `Phong`: nie ma w niej nic, co liczenie światła w wierzchołkach mogłoby zgubić. Oba pliki w całości omawia [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), sekcje 4.2 i 4.4.

Powód, dla którego emisja nie trafiła do bloku `LightBlock`: blok opisuje światła, wspólne dla całej klatki, a emisja jest inna dla każdego rysowanego obiektu (czerń dla ściany, turkus dla kryształu). Taka wartość musi być zwykłym uniformem, ustawianym między wywołaniami rysowania.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/scene/Light.hpp`](../../../src/scene/Light.hpp) | struktury `Attenuation`, `DirectionalLight`, `PointLight`, `SpotLight`, `LightSet`, `ConeCosines`, stałe `MAX_POINT_LIGHTS`, `BRIGHTNESS_AT_RADIUS`, `MIN_CONE_COSINE_GAP`, deklaracje pięciu funkcji |
| [`src/scene/Light.cpp`](../../../src/scene/Light.cpp) | `attenuationForRadius`, `attenuationFactor`, `coneCosines`, `spotFactor`, `directionFromAngles` |
| [`assets/shaders/common/lighting.glsl`](../../../assets/shaders/common/lighting.glsl) | blok świateł i wzory po stronie karty (sekcja 4) |
| [`src/debug/panels/LightsPanel.hpp`](../../../src/debug/panels/LightsPanel.hpp), [`.cpp`](../../../src/debug/panels/LightsPanel.cpp) | panel Lights (sekcja 6) |
| [`tests/LightTests.cpp`](../../../tests/LightTests.cpp) | 20 przypadków testowych (sekcja 5.6) |
| [`src/scene/LightSpace.hpp`](../../../src/scene/LightSpace.hpp), [`.cpp`](../../../src/scene/LightSpace.cpp) (od czwartej części M7) | struktura `LightSpace` (od piątej części z polami `kind`, `position`, `nearPlane` i `farPlane`), wyliczenie `LightProjection`, stałe `LIGHT_BOX_MARGIN`, `VERTICAL_DIRECTION_LIMIT`, `SPOT_NEAR_PLANE`, `SPOT_CONE_MARGIN_DEGREES` i granice kąta otwarcia, funkcje `directionalLightSpace`, od piątej części `spotLightSpace`, i `shadowMapCoordinates` (sekcja 5.7, pełna matematyka w [`../renderer/shadows.md`](../renderer/shadows.md)) |
| [`tests/ShadowTests.cpp`](../../../tests/ShadowTests.cpp) (od czwartej części M7) | 31 przypadków testowych cieni (16 od czwartej części, 15 doszło w piątej), z czego osiem pierwszych sprawdza `directionalLightSpace`, a dziewięć od piątej części `spotLightSpace` (sekcja 5.7) |
| [`assets/shaders/common/shadows.glsl`](../../../assets/shaders/common/shadows.glsl) (od czwartej części M7) | mapa cieni księżyca po stronie karty: `moonShadow`. Omawia [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 4 |
| [`src/scene/LightBlock.hpp`](../../../src/scene/LightBlock.hpp), [`.cpp`](../../../src/scene/LightBlock.cpp) | zamiana `LightSet` na bajty bloku: omawia [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md) |
| [`src/game/Lighting.hpp`](../../../src/game/Lighting.hpp), [`.cpp`](../../../src/game/Lighting.cpp) | ustawienia świateł gry i budowanie `LightSet` co klatkę: omawia [`../game/flashlight.md`](../game/flashlight.md) |

Droga danych od suwaka do piksela:

```mermaid
flowchart TD
    Panel["panel Lights<br>edytuje game::LightingSettings"] --> Frame
    Frame["game::lightingForFrame<br>kopia na jedną klatkę: migotanie latarki, puls kryształów"] --> Build
    Eye["oko i wektory kamery<br>z NightMazeApp::onRender"] --> Pose["game::flashlightPose<br>ręka i kierunek wiązki"]
    Pose --> Build
    Crystals["game::crystalLightPositions<br>nad niezebranymi kryształami"] --> Build
    Build["game::buildLightSet<br>ustawienia na scene::LightSet"] --> Pack
    Pack["scene::packLightBlock<br>LightSet na 928 bajtów std140"] --> Ubo
    Ubo["gfx::UniformBuffer::update<br>bajty na kartę"] --> Glsl
    Glsl["common/lighting.glsl<br>computeLighting"] --> Frag["lit.frag: dla każdego fragmentu"]
    Glsl --> Vert["gouraud.vert: dla każdego wierzchołka"]
    Glsl --> Grass["grass.frag: dla każdego fragmentu trawy"]
```

Ten dokument omawia pudełko panelu Lights i pudełko `common/lighting.glsl` oraz struktury, które płyną między nimi. Pudełka z `game::` w nazwie omawia [`../game/flashlight.md`](../game/flashlight.md). Diagram pokazuje drogę świateł, a nie cienia: mapa cieni księżyca ma własną drogę (kierunek księżyca i pudełko terenu, `scene::directionalLightSpace`, przebieg głębi, `game::setShadowUniforms`), opisaną w [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 2.18.

### 5.2 Struktury świateł

```cpp
constexpr int MAX_POINT_LIGHTS = 16;

struct Attenuation {
    float constant = 1.0F;
    float linear = 0.0F;
    float quadratic = 0.0F;
};
```

(Komentarze z pliku są tu pominięte, żeby fragment był krótki: w pliku każde pole ma opis.)

Domyślne `Attenuation` to mianownik równy 1: światło bez tłumienia. `MAX_POINT_LIGHTS` to długość tablicy w `LightSet` i w shaderze. PRD (sekcja "Model oświetlenia") przewiduje dokładnie taki zestaw: jedno światło kierunkowe, do 16 punktowych, jeden reflektor.

```cpp
struct DirectionalLight {
    /// The direction the light TRAVELS in, from the light towards the scene, in world
    /// space. (0, -1, 0) shines straight down. It does not have to be of length 1.
    glm::vec3 direction{0.0F, -1.0F, 0.0F};
    /// Red, green, blue, each from 0 to 1.
    glm::vec3 color{1.0F};
    /// The colour is multiplied by this. 0 switches the light off.
    float intensity = 1.0F;
};
```

**Najważniejsza umowa w tym pliku:** `direction` to kierunek, w którym światło **leci** (od źródła w scenę), a nie kierunek do źródła. Wzory potrzebują kierunku do światła, więc shader go odwraca (`-uDirectionalDirection`). Pomylenie tych dwóch konwencji oświetla złe strony ścian (pułapka 1). Kolor i intensywność są osobno: intensywność zmienia jasność bez ruszania barwy i jest wygodna jako jeden suwak.

```cpp
struct PointLight {
    /// Position in world space.
    glm::vec3 position{0.0F};
    glm::vec3 color{1.0F};
    float intensity = 1.0F;
    Attenuation attenuation;
};

struct SpotLight {
    /// Position of the tip of the cone in world space.
    glm::vec3 position{0.0F};
    /// The direction the cone points in (its axis), in world space. It does not have to
    /// be of length 1.
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
    glm::vec3 color{1.0F};
    float intensity = 1.0F;
    Attenuation attenuation;
    /// Both angles are measured from the axis of the cone to its side, in degrees (half
    /// of the full opening angle). Inside the inner cone the light has its full
    /// brightness, outside the outer cone there is none, and between the two it fades.
    /// The inner angle must not be larger than the outer one.
    float innerConeDegrees = 12.0F;
    float outerConeDegrees = 18.0F;
};
```

`SpotLight` to `PointLight` z osią i dwoma kątami. Kąty są w **stopniach** i są **połówkami** pełnego rozwarcia: człowiek myśli w stopniach, shader dostanie cosinusy. Wartości domyślne struktury (12 i 18) nie są wartościami latarki w grze: te pochodzą z `LightingSettings` (13 i 21).

```cpp
struct LightSet {
    glm::vec3 ambient{0.0F};

    DirectionalLight directional;

    /// Only the first pointCount elements are lights, the rest is ignored.
    std::array<PointLight, MAX_POINT_LIGHTS> points;
    int pointCount = 0;

    SpotLight spot;
    /// False switches the spot light off without changing its settings.
    bool spotEnabled = true;
};
```

`LightSet` to **wszystkie światła jednej klatki**. Tablica ma stałą długość 16 (`std::array`, bez alokacji pamięci), a `pointCount` mówi, ile jej elementów jest światłami. Tak samo jest w shaderze: `uPoints[16]` i `uPointCount`. `spotEnabled` wyłącza latarkę bez kasowania jej ustawień: po włączeniu świeci tak jak przedtem.

### 5.3 Tłumienie: `attenuationForRadius` i `attenuationFactor`

```cpp
constexpr float RADIUS_LINEAR_PART = 2.0F;
constexpr float RADIUS_QUADRATIC_PART = 17.0F;
```

```cpp
Attenuation attenuationForRadius(float radius) {
    // Dividing by a radius of 0 is not possible, and a negative one has no meaning.
    if (radius <= 0.0F) {
        return {};
    }
    return {
        .constant = 1.0F,
        .linear = RADIUS_LINEAR_PART / radius,
        .quadratic = RADIUS_QUADRATIC_PART / (radius * radius),
    };
}

float attenuationFactor(const Attenuation& attenuation, float distance) {
    return 1.0F / (attenuation.constant + attenuation.linear * distance +
                   attenuation.quadratic * distance * distance);
}
```

| Linia | Znaczenie |
|---|---|
| `RADIUS_LINEAR_PART`, `RADIUS_QUADRATIC_PART` | liczby 2 i 17 z sekcji 2.5. Razem z 1 dają 20, odwrotność `BRIGHTNESS_AT_RADIUS` (0,05). Podział między nimi kształtuje krzywą: część liniowa daje szybki spadek blisko źródła, kwadratowa sprowadza światło w dół przy promieniu |
| `if (radius <= 0.0F) return {};` | przez zero nie da się dzielić, a ujemny promień nie ma sensu. `{}` to domyślne `Attenuation`: światło bez tłumienia |
| `.constant = ...` (designated initializers, C++20) | inicjalizacja z nazwami pól: widać, która liczba idzie gdzie |
| `attenuationFactor` | wzór z sekcji 2.5, ten sam co w shaderze (sekcja 4.4). Shader liczy swoją kopią, ta służy testom |

Stała `BRIGHTNESS_AT_RADIUS` w nagłówku (0,05) nie jest używana we wzorze: zapisuje wynik, który z niego wynika, a test sprawdza, że rzeczywiście tyle wychodzi.

### 5.4 Stożek: `coneCosines` i `spotFactor`

```cpp
struct ConeCosines {
    float inner = 1.0F;
    float outer = 0.0F;
};

constexpr float MIN_CONE_COSINE_GAP = 0.001F;
```

```cpp
ConeCosines coneCosines(float innerDegrees, float outerDegrees) {
    // The standard library takes angles in radians.
    const float outer = std::cos(glm::radians(outerDegrees));
    const float inner = std::cos(glm::radians(innerDegrees));
    return {
        .inner = std::max(inner, outer + MIN_CONE_COSINE_GAP),
        .outer = outer,
    };
}

float spotFactor(const ConeCosines& cone, float cosAngle) {
    // How far cosAngle is on the way from the outer cosine (0) to the inner one (1).
    // Comparing cosines instead of angles saves the shader an acos per fragment.
    return std::clamp((cosAngle - cone.outer) / (cone.inner - cone.outer), 0.0F, 1.0F);
}
```

| Linia | Znaczenie |
|---|---|
| `std::cos(glm::radians(...))` | `std::cos` przyjmuje radiany, a kąty są w stopniach |
| `std::max(inner, outer + MIN_CONE_COSINE_GAP)` | gwarancja `inner > outer`. Gdy kąt wewnętrzny nie jest mniejszy od zewnętrznego (ostra krawędź albo złe dane), cosinus wewnętrzny dostaje wartość o 0,001 większą od zewnętrznego. Dzielenie w `spotFactor` i w shaderze nigdy nie jest przez zero |
| `spotFactor` | wzór miękkiego brzegu z sekcji 2.6. Shader ma własną kopię |

`coneCosines` woła `packLightBlock` przy pakowaniu latarki do bloku, więc gwarancja obowiązuje dla każdej klatki.

### 5.5 `directionFromAngles`: kierunek księżyca z dwóch kątów

```cpp
glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees) {
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);

    // The same formula as scene::Camera::forward: pitch splits the unit vector into
    // a vertical part, sin(pitch), and a horizontal part of length cos(pitch), and yaw
    // turns the horizontal part from -Z (yaw 0) towards +X (yaw 90).
    const float horizontal = std::cos(pitch);
    return {horizontal * std::sin(yaw), std::sin(pitch), -horizontal * std::cos(yaw)};
}
```

Kierunek światła łatwiej ustawić dwoma suwakami niż trzema liczbami, które muszą razem mieć długość 1. Konwencja jest ta sama co w kamerze ([`camera.md`](camera.md)): yaw 0 wskazuje `-Z`, 90 wskazuje `+X`, 180 `+Z`, 270 `-X`. Pitch 0 to poziom, dodatni w górę, ujemny w dół. Wynik ma zawsze długość 1, bo `cos²(pitch) * (sin²(yaw) + cos²(yaw)) + sin²(pitch) = 1`.

Dla ustawień startowych (yaw 25, pitch -50):

```text
horizontal = cos(-50) = 0,643
x =  0,643 * sin(25) =  0,272
y =  sin(-50)        = -0,766
z = -0,643 * cos(25) = -0,583
```

To jest wektor z tabeli w sekcji 2.3. Księżyc "wisi" więc po przeciwnej stronie: nad `-X` i `+Z`.

**Kto woła tę funkcję dla księżyca (czwarta część M7).** Do trzeciej części M7 wołała ją wprost `game::buildLightSet`. Dziś między nimi stoi jedna mała funkcja gry, `game::moonDirection` z `src/game/Lighting.cpp`:

```cpp
glm::vec3 moonDirection(const LightingSettings& settings) {
    return scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees);
}
```

Bierze z niej kierunek `buildLightSet` (pole `.direction = moonDirection(settings)`, czyli światło, którym shader oświetla) i `NightMazeApp::drawMoonShadowMap` (kierunek, z którego rysowana jest mapa cieni). Komentarz w `Lighting.hpp` podaje powód: "The lights of a frame and the shadow map of the moon both take it from here, so they can never disagree". Gdyby oba miejsca liczyły kierunek osobno i jedno z nich się zmieniło, cienie padałyby w inną stronę niż ta, z której świeci księżyc. Zgodności pilnuje test `the moon direction of the settings is the one the lights are built with` w `tests/ShadowTests.cpp`. Resztę `Lighting.*` omawia [`../game/flashlight.md`](../game/flashlight.md).

### 5.6 Jak to zostało sprawdzone

Testy jednostkowe w `tests/LightTests.cpp`, 20 przypadków ([`../../libraries/doctest.md`](../../libraries/doctest.md)):

| Przypadek testowy | Co sprawdza | Wynik |
|---|---|---|
| `the default attenuation leaves a light as it is at every distance` | domyślne `Attenuation` w odległości 0 i 100 | czynnik równy dokładnie 1 |
| `attenuationFactor is one divided by constant plus linear plus quadratic part` | wzór na liczbach dobranych tak, żeby mianownik wyszedł 3 | 1/3 |
| `attenuationForRadius gives the documented terms` | promień 3 | `constant = 1`, `linear = 2/3`, `quadratic = 17/9` |
| `a light made for a radius has 5 % of its brightness left at that radius` | kilka promieni | w odległości równej promieniowi zawsze `BRIGHTNESS_AT_RADIUS`, czyli 0,05 |
| `a light made for a radius is full at its source, falls with distance, never to zero` | rosnące odległości | 1 w źródle, każda następna wartość mniejsza i większa od zera |
| `a radius that is not positive gives a light without attenuation` | promień zero i ujemny | `1, 0, 0` |
| `coneCosines turns the two angles into their cosines` | kąty 0 i 60 | 1 i 0,5, `inner > outer` |
| `coneCosines never gives two equal cosines` | kąty równe albo odwrócone, zewnętrzny 20 stopni | `outer = 0,9397`, `inner = outer + MIN_CONE_COSINE_GAP` |
| `spotFactor is 1 inside the inner cone, 0 outside the outer one, a ramp between` | stożek o cosinusach 0,9 i 0,7 | 1 dla 1 i 0,9, potem 0,5 dla 0,8, 0,25 dla 0,75, 0 dla 0,7, 0,2 i -1 |
| `spotFactor of a cone with a hard edge is 0 or 1, never not a number` | stożek o równych kątach | 1 na osi, 0 z boku, bez `NaN` |
| `directionFromAngles follows the compass of the camera` | yaw 0, 90, 180, 270 przy poziomie, pitch -90 i 90 | `-Z`, `+X`, `+Z`, `-X`, prosto w dół, prosto w górę |
| `directionFromAngles gives a vector of length 1` | 3 kąty yaw razy 4 kąty pitch | długość 1 |
| `an empty light set has no point lights and a spot light that is on` | domyślne `LightSet` | `pointCount = 0`, `spotEnabled`, tablica 16 elementów |
| `packLightBlock copies the camera, the ambient light and the moon` | kierunek `(0, -4, 3)` | w bloku `(0, -0,8, 0,6)`: długość 1. Intensywność w czwartej liczbie koloru |
| `packLightBlock packs the spot light with cosines and its switch` | latarka z kątami 0 i 60 | `spotCone = (1, 0,5, 1, 0)`, po wyłączeniu `z = 0` |
| `packLightBlock keeps the cone cosines apart` | oba kąty 25 | `x - y > 0` |
| `packLightBlock replaces a direction of length zero` | kierunki zerowe | `(0, -1, 0)` zamiast `NaN` |
| `packLightBlock packs the point lights in use and leaves the rest zero` | dwa światła w użyciu, trzecie wypełnione, ale nieliczone | dwa pierwsze spakowane, trzecie zerowe, dopełnienie zerowe |
| `packLightBlock limits the number of point lights to the array` | `pointCount` równe 21 i -3 | 16 i 0 |
| `the light block has the size and the offsets of the std140 block` | `sizeof` i `offsetof` | 48, 144, 160, 928, wielokrotność 16 |

Osiem ostatnich dotyczy `packLightBlock` i układu bloku: kod, który sprawdzają, omawia [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md).

**Czego testy nie sprawdzają.** Kodu GLSL. Funkcje C++ (`attenuationFactor`, `spotFactor`) i ich kopie w `lighting.glsl` to dwa osobne zapisy tego samego wzoru: test przechodzi także wtedy, gdy ktoś zmieni tylko shader. Zgodność obu zapisów sprawdziłem czytając kod, a obraz na zrzutach ekranu z Windowsa. Panel Lights wymaga okna i nie ma testów.

Czwarta część M7 nie zmieniła `tests/LightTests.cpp`: nadal ma 20 przypadków. Nowy plik modułu, `LightSpace`, ma testy w `tests/ShadowTests.cpp` (sekcja 5.7). Nowych pól `moonDiffuse` i `moonSpecular` ani funkcji `moonFacing` żaden test nie sprawdza, bo to kod GLSL.

### 5.7 `LightSpace`: scena widziana ze światła (czwarta część M7, 2026-10-05)

Mapa cieni to obraz sceny zrobiony ze światła: każdy teksel zapisuje, jak daleko od światła jest najbliższa powierzchnia. Żeby taki obraz zrobić, światło potrzebuje tego samego, co kamera: macierzy widoku i macierzy rzutowania. Tę parę nazywa się **przestrzenią światła** (light space) i w projekcie liczy ją nowy plik modułu `scene`: [`src/scene/LightSpace.hpp`](../../../src/scene/LightSpace.hpp) i [`.cpp`](../../../src/scene/LightSpace.cpp). Jak reszta modułu, to sama matematyka bez OpenGL, więc ma testy jednostkowe. Ta sekcja jest krótka celowo: pokazuje, co jest w pliku. Pełną matematykę (dlaczego rzut ortograficzny, jak pudełko dopasowuje się do terenu, ile metrów ma teksel) ma [`../renderer/shadows.md`](../renderer/shadows.md), sekcje 2.2, 2.3 i 2.5.

```cpp
/// The view and the projection of a light.
struct LightSpace {
    /// World space to the space of the light: the light looks along -Z, like a camera.
    glm::mat4 view{1.0F};
    /// The space of the light to clip space.
    glm::mat4 projection{1.0F};
    /// The size of the orthographic box of a directional light in metres: x and y are
    /// the width and the height of the area the shadow map covers, z is the distance
    /// from its near plane to its far plane.
    glm::vec3 extent{0.0F};

    /// World space to the clip space of the light in one matrix. A vertex shader
    /// applies the view first and the projection second, so the projection stands on
    /// the left.
    glm::mat4 matrix() const { return projection * view; }
};
```

| Składowa | Znaczenie | Kto jej używa |
|---|---|---|
| `view` | ze świata do przestrzeni światła. Światło patrzy wzdłuż `-Z`, jak kamera ([`camera.md`](camera.md)) | przebieg głębi: uniform `uView` programu `shadow_depth` |
| `projection` | z przestrzeni światła do przestrzeni przycięcia | przebieg głębi: uniform `uProjection` programu `shadow_depth` |
| `extent` | rozmiar pudełka ortograficznego w metrach: `x` i `y` to szerokość i wysokość obszaru, który pokrywa mapa, `z` to odległość od bliskiej do dalekiej płaszczyzny | `game::shadowTexelSize` (rozmiar teksela z `x` i `y`), `game::setShadowUniforms` (bias w metrach dzielony przez `extent.z`), panel Shadows |
| `kind`, `position`, `nearPlane`, `farPlane` (od piątej części M7) | rodzaj rzutu (`LightProjection::Orthographic` albo `Perspective`), dla perspektywicznego miejsce światła w świecie i dwie płaszczyzny w metrach od światła. Dla księżyca `position` i obie płaszczyzny zostają 0. W perspektywicznym `extent.x` i `extent.y` są mierzone **na dalekiej płaszczyźnie** | `game::biasForShader`, `game::shadowTexelSizeAt`, `ShadowMap::drawPreview`, `setShadowUniforms` (pozycja światła), panel Shadows |
| `matrix()` | obie macierze w jednej: `projection * view`. Rzutowanie stoi po lewej, bo wierzchołek mnoży się najpierw przez widok | uniform `uMoonShadowMatrix` w programach oświetlenia, `shadowMapCoordinates` |

Dwie stałe:

```cpp
constexpr float LIGHT_BOX_MARGIN = 0.5F;
```

Wolne miejsce wokół pudełka światła po wszystkich sześciu stronach, w metrach. Obiekt leżący dokładnie na ścianie pudełka (najniższy grunt, czubek słupka) jest dzięki temu pewnie w środku i nie obetnie go błąd zaokrąglenia.

```cpp
constexpr float VERTICAL_DIRECTION_LIMIT = 0.999F;
```

Powyżej tej wartości `|y|` kierunek światła (o długości 1) liczy się jako pionowy. `acos(0,999)` to około 2,56 stopnia (policzone; komentarz w pliku zaokrągla do "about 2.5 degrees"), więc chodzi o światło odchylone od pionu o mniej niż tyle. Suwak `Moon pitch` sięga do -90, czyli taki kierunek da się ustawić z panelu Lights.

```cpp
LightSpace directionalLightSpace(const Aabb& bounds, const glm::vec3& lightDirection);
```

Przestrzeń światła **kierunkowego**, czyli księżyca. Rzut jest ortograficzny, bo promienie takiego światła są równoległe i nic nie maleje z odległością. `bounds` to pudełko `scene::Aabb` ([`collision.md`](collision.md)) obejmujące wszystko, co może rzucić cień. `lightDirection` to kierunek, w którym światło **leci**, o dowolnej długości. Kroki funkcji:

| Krok | Kod | Znaczenie |
|---|---|---|
| 1 | `glm::length(lightDirection) < MIN_DIRECTION_LENGTH ? FALLBACK_DIRECTION : glm::normalize(lightDirection)` | kierunek do długości 1. Kierunek o długości zero (krótszy niż 0,0001) jest zastępowany kierunkiem **prosto w dół**, `(0, -1, 0)`: ta sama reguła co w `packLightBlock`, żeby `normalize` nie dało `NaN` |
| 2 | `std::abs(direction.y) > VERTICAL_DIRECTION_LIMIT ? UP_FOR_VERTICAL_LIGHT : WORLD_UP` | wektor "w górę" dla widoku. Zwykle `+Y`. Dla światła (prawie) pionowego `+Y` byłoby równoległe do kierunku patrzenia i `lookAt` dzieliłoby przez zero, więc wtedy "górą" jest `-Z` |
| 3 | `glm::lookAt(center - direction, center, up)` | widok: światło patrzy na środek pudełka wzdłuż kierunku swoich promieni. Światło kierunkowe nie ma pozycji, więc oko stoi po prostu jeden krok przed środkiem. Gdzie dokładnie, nie ma znaczenia: płaszczyzny są mierzone od niego w kroku 5 |
| 4 | pętla po ośmiu narożnikach `bounds`, `glm::min` i `glm::max`, potem `LIGHT_BOX_MARGIN` | osiem narożników trafia do przestrzeni światła, a najmniejsze i największe `x`, `y`, `z` wśród nich to ściany pudełka, powiększone o margines |
| 5 | `glm::ortho(smallest.x, largest.x, smallest.y, largest.y, -largest.z, -smallest.z)` | rzut ortograficzny dokładnie tego pudełka. Minusy: widok patrzy wzdłuż `-Z`, więc narożnik najbliższy światłu ma **największe** `z`, a `glm::ortho` chce obu płaszczyzn jako odległości przed okiem |
| 6 | `.extent = largest - smallest` | rozmiar pudełka w metrach |

Wynik zależy tylko od `bounds` i od kierunku, **nie od kamery**. Mapa cieni pokrywa więc ten sam grunt w każdej klatce i cienie nie migoczą, gdy gracz się rusza. Pudełko dopasowane do zawartości nie marnuje tekseli na pustą przestrzeń. W grze `bounds` to `game::shadowCasterBounds(terrain)` (cały teren od najniższego gruntu do wysokości słupka nad najwyższym), a kierunek to `game::moonDirection` (sekcja 5.5). Woła to `NightMazeApp::drawMoonShadowMap` w każdej klatce. Dla labiryntu domyślnego (10 na 10 komórek, ziarno 1, skala wysokości 1, księżyc yaw 25 i pitch -50) pudełko pokrywa 64,8 na 54,1 m i ma 47,0 m głębi (policzone, nie zmierzone w grze). Skąd te liczby i dlaczego pudełko jest dopasowane do terenu, a nie do kamery: [`../renderer/shadows.md`](../renderer/shadows.md), sekcje 2.2 i 2.3, i notatka [`../../decisions/shadow-box-fitted-to-terrain.md`](../../decisions/shadow-box-fitted-to-terrain.md).

```cpp
glm::vec3 shadowMapCoordinates(const glm::mat4& lightSpaceMatrix, const glm::vec3& worldPosition) {
    const glm::vec4 clip = lightSpaceMatrix * glm::vec4{worldPosition, 1.0F};
    // The perspective division. For an orthographic projection w is 1 and nothing
    // changes. It is done all the same, so the function is right for every light.
    const glm::vec3 ndc = glm::vec3{clip} / clip.w;
    // Normalised device coordinates run from -1 to 1, texture coordinates and stored
    // depths from 0 to 1.
    return ndc * 0.5F + 0.5F;
}
```

Gdzie punkt świata ląduje w mapie cieni: `x` i `y` to współrzędna tekstury (od 0 do 1 wewnątrz mapy), `z` to głębia, którą mapa zapisałaby dla powierzchni w tym punkcie (0 na bliskiej płaszczyźnie światła, 1 na dalekiej). Trzy kroki: macierz, dzielenie przez `w`, z zakresu od -1 do 1 do zakresu od 0 do 1. **Gra tej funkcji nie woła przy rysowaniu.** Te same trzy kroki robi shader, funkcja `moonShadow` w `common/shadows.glsl` (`clip.xyz / clip.w * 0.5 + 0.5`). Wersja C++ istnieje po to, żeby drogę punktu do mapy dało się sprawdzić testem, tak jak `attenuationFactor` i `spotFactor` z sekcji 5.3 i 5.4. To znów dwie kopie jednego wzoru (pułapka 11). Teoria: [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 2.5.

**Testy.** Osiem pierwszych przypadków `tests/ShadowTests.cpp` dotyczy tego pliku (pozostałe osiem: pudełko terenu, rozmiar teksela, bias, rozdzielczości, PCF i `moonDirection`, omawia je [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 5.10). Pudełko testowe to `min = (-4, 0, 2)`, `max = (10, 6, 30)`:

| Przypadek testowy | Co sprawdza |
|---|---|
| `the box of a directional light holds every corner of its bounds` | każdy z ośmiu narożników ma wszystkie trzy współrzędne mapy ściśle między 0 a 1 |
| `the box of a directional light fits its bounds: only the margin is left free` | na każdej z sześciu stron któryś narożnik zbliża się do ściany pudełka na `LIGHT_BOX_MARGIN`: nic nie jest marnowane |
| `a light that shines straight down sees the bounds from above` | kierunek `(0, -1, 0)`: macierz ma same skończone liczby, `extent` to 14, 28 i 6 m plus margines z obu stron, góra pudełka jest bliżej światła niż dół i leży w tym samym tekselu |
| `a light that shines almost straight down still gets a usable matrix` | pitch -90, -89,9, -89, -87, -85 i -5 przy yaw 25 (cały zakres suwaka): macierz skończona, narożniki w mapie |
| `a light direction of length zero is replaced by straight down` | kierunek zerowy daje tę samą macierz co `(0, -1, 0)` |
| `the length of the light direction does not change the box` | kierunek 25 razy dłuższy daje te same współrzędne punktu |
| `points on one ray of the light share a texel and differ in depth only` | punkt na szczycie ściany i punkt 3 m dalej wzdłuż promienia mają te same `x` i `y`, a głębia różni się o `3 / extent.z`: to jest zasada działania mapy cieni |
| `a point outside the bounds lands outside the shadow map` | punkt 100 m na wschód ma `x > 1`, punkt 100 m pod pudełkiem ma `z > 1` |

Zgłoszone dla Windowsa, 2026-10-05: wszystkie przechodzą w Debug i Release w ramach `make check` (310 przypadków i 103751 asercji całego programu testowego). Na macOS nic nie było budowane.

## 6. Panel ImGui

Światła mają panel **Lights**. PRD (sekcja 10) opisuje go tak: "Lista świateł, kolor, intensywność, tłumienie, kąty latarki, kierunek księżyca". Kod: [`src/debug/panels/LightsPanel.cpp`](../../../src/debug/panels/LightsPanel.cpp). Panel stoi w lewej kolumnie pod panelem Renderer ([`../debug-ui.md`](../debug-ui.md)).

### 6.1 Kod panelu

```cpp
void drawLightsPanel(game::LightingSettings& lighting, const game::Round& round) {
    // First run only: the left edge of the window, below the Renderer panel (the constant
    // is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(LIGHTS_PLACEMENT);
    if (ImGui::Begin("Lights")) {
        ImGui::ColorEdit3("Ambient", glm::value_ptr(lighting.ambient));
        drawMoon(lighting);
        drawFlashlight(lighting, round);
        drawPointLights(lighting, round);
        drawHighlight(lighting);
    }
    ImGui::End();
}
```

| Linia | Znaczenie |
|---|---|
| `game::LightingSettings& lighting` | referencja do ustawień gry: panel edytuje je w miejscu. Gra buduje z nich światła następnej klatki ([`../game/flashlight.md`](../game/flashlight.md)), więc każda zmiana jest widoczna od razu |
| `const game::Round& round` | runda w toku, tylko do odczytu. Panel bierze z niej dwie rzeczy: ile kryształów jeszcze niesie światło i czy bateria latarki jest pusta. W M4 w tym miejscu był `const game::MazeWorld&` |
| `placePanelOnFirstUse(LIGHTS_PLACEMENT)` | miejsce i rozmiar przy pierwszym uruchomieniu |
| `ImGui::ColorEdit3("Ambient", glm::value_ptr(lighting.ambient))` | `ColorEdit3` czyta i zapisuje trzy liczby `float` przez wskaźnik. `glm::value_ptr` daje adres trzech liczb wektora `glm::vec3` |
| cztery wywołania | cztery grupy, każda w osobnej funkcji w anonimowej przestrzeni nazw pliku. Dwie z nich (`drawFlashlight`, `drawPointLights`) dostają rundę |

Grupy mają wspólny schemat. Przykład, księżyc:

```cpp
void drawMoon(game::LightingSettings& lighting) {
    // CollapsingHeader draws a title bar that folds its group away. It returns true
    // while the group is open. This group starts folded: the panel is exactly as tall
    // as the other three groups together, and the moon is the light that is changed
    // least often. A click on the bar opens it (the panel then scrolls).
    if (!ImGui::CollapsingHeader("Moon (directional)")) {
        return;
    }
    // The two angles say which way the light travels (scene::directionFromAngles).
    ImGui::SliderFloat("Moon yaw", &lighting.moonYawDegrees, MIN_MOON_YAW_DEGREES,
                       MAX_MOON_YAW_DEGREES, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Moon pitch", &lighting.moonPitchDegrees, MIN_MOON_PITCH_DEGREES,
                       MAX_MOON_PITCH_DEGREES, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    // ColorEdit3 reads and writes three floats through the pointer. value_ptr gives the
    // address of the three floats of a glm::vec3.
    ImGui::ColorEdit3("Moon colour", glm::value_ptr(lighting.moonColor));
    ImGui::SliderFloat("Moon intensity", &lighting.moonIntensity, MIN_INTENSITY, MAX_MOON_INTENSITY,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
}
```

| Linia | Znaczenie |
|---|---|
| `ImGui::CollapsingHeader("Moon (directional)")` | pasek z tytułem, który zwija grupę. Zwraca prawdę, gdy grupa jest otwarta. Bez flagi grupa startuje **zwinięta**. Pozostałe trzy grupy podają `ImGuiTreeNodeFlags_DefaultOpen` i startują otwarte |
| `if (!...) return;` | zwinięta grupa nie rysuje widżetów |
| `SliderFloat(..., "%.0f deg", ImGuiSliderFlags_AlwaysClamp)` | suwak z zakresem. `AlwaysClamp` pilnuje zakresu także wtedy, gdy liczbę wpisano z klawiatury (Ctrl i kliknięcie) |
| zakres `Moon pitch` od -90 do -5 | -90 świeci prosto w dół, blisko 0 światło tylko muska grunt. Powyżej 0 księżyc świeciłby spod ziemi, więc suwak tam nie sięga |

Dwa miejsca panelu czytają rundę. Pierwsze jest w `drawFlashlight`:

```cpp
    // The same switch as the F key. With an empty battery the game turns it off again
    // in its next step, so the box cannot stay ticked: the tooltip says why.
    ImGui::Checkbox("Flashlight on (key F)", &lighting.flashlightOn);
    if (round.battery <= 0.0F) {
        ImGui::SetItemTooltip("The battery is empty: collect a crystal first.");
    }
```

| Linia | Znaczenie |
|---|---|
| `ImGui::Checkbox("Flashlight on (key F)", &lighting.flashlightOn)` | pole wyboru pisze prosto do przełącznika latarki, tego samego, który zmienia klawisz F |
| `if (round.battery <= 0.0F)` | przy pustej baterii gra wyłącza przełącznik w następnym kroku symulacji (`game::updateRound`), więc pola nie da się zaznaczyć na stałe. Bez wyjaśnienia wyglądałoby to na zepsuty widżet |
| `ImGui::SetItemTooltip(...)` | podpowiedź doklejona do **ostatnio narysowanego** widżetu, czyli do pola wyboru: pokazuje się po najechaniu na nie kursorem |

Drugie jest w `drawPointLights`:

```cpp
    // Every crystal that is not collected yet carries one light.
    const int crystalCount = static_cast<int>(round.crystals.size());
    ImGui::Text("Lit: %d of %d crystals (at most %d)", crystalCount - round.collectedCount,
                crystalCount, scene::MAX_POINT_LIGHTS);
```

| Linia | Znaczenie |
|---|---|
| `round.crystals.size()` | wszystkie kryształy rundy, zebrane i niezebrane. `size()` zwraca typ bez znaku, a `%d` chce `int`, stąd rzutowanie |
| `crystalCount - round.collectedCount` | ile kryształów jeszcze świeci: tyle świateł punktowych ma ta klatka |
| `scene::MAX_POINT_LIGHTS` | 16: długość tablicy świateł w shaderze. Liczba kryształów nigdy jej nie przekracza (`game::crystalCountFor`) |

Dla labiryntu startowego linia brzmi na początku rundy `Lit: 13 of 13 crystals (at most 16)`.

Dwa widżety z innych grup są nowe w projekcie:

```cpp
    ImGui::DragFloatRange2("Cone", &lighting.flashlightInnerDegrees,
                           &lighting.flashlightOuterDegrees, CONE_DRAG_SPEED, MIN_CONE_DEGREES,
                           MAX_CONE_DEGREES, "inner %.1f deg", "outer %.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
```

Jeden widżet dla **dwóch** liczb: dwa pola przeciągane myszą. `DragFloatRange2` pilnuje, żeby pierwsza wartość nie przekroczyła drugiej, więc stożek wewnętrzny nie będzie szerszy od zewnętrznego. Drugie zabezpieczenie jest w `buildLightSet`, trzecie w `coneCosines`.

```cpp
    ImGui::SliderFloat("Shininess", &lighting.shininess, MIN_SHININESS, MAX_SHININESS, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
```

Suwak **logarytmiczny**: połowa jego długości przypada na małe wykładniki, gdzie jeden krok zmienia rozmiar odbłysku najbardziej. Zakres od 1 do 256: poniżej 1 wzór przestaje wyglądać jak odbłysk, a `pow(0, 0)` jest niezdefiniowane.

### 6.2 Kontrolki i czego uczą

| Grupa | Kontrolka | Zakres | Pole w `LightingSettings` | Czego uczy |
|---|---|---|---|---|
| (bez grupy) | `Ambient` | kolor | `ambient` | składnik otoczenia: jedyne światło w miejscach, do których nic nie świeci. Czarny daje czarne cienie. Jak wszystkie cztery kolory panelu, jest to wartość sRGB (to, co pokazuje próbnik), przeliczana na liniową w `buildLightSet` |
| `Moon (directional)`, zwinięta | `Moon yaw` | od 0 do 360 stopni | `moonYawDegrees` | światło kierunkowe: obrót zmienia, które strony ścian są jasne (prawo Lamberta), a nic nie zależy od miejsca. Od czwartej części M7 razem ze światłem obracają się cienie ścian, bo mapa cieni bierze kierunek z tych samych dwóch kątów (`game::moonDirection`) |
| | `Moon pitch` | od -90 do -5 stopni | `moonPitchDegrees` | -90: grunt najjaśniejszy, ściany ciemne. Blisko -5: odwrotnie |
| | `Moon colour`, `Moon intensity` | kolor, od 0 do 2 | `moonColor`, `moonIntensity` | kolor razy intensywność. 0 wyłącza księżyc. Wartość startowa intensywności to od czwartej części M7 0,2 (wcześniej 0,12). Od tej części intensywność decyduje też o kontraście cienia księżyca: przy 0 cienia nie widać, bo nie ma czego odejmować |
| `Flashlight (spot)` | `Flashlight on (key F)` | pole wyboru | `flashlightOn` | to samo pole, które przełącza klawisz F. Przy pustej baterii ma podpowiedź `The battery is empty: collect a crystal first.` i samo się odznacza |
| | `Beam colour`, `Beam intensity` | kolor, od 0 do 10 | `flashlightColor`, `flashlightIntensity` | powyżej pewnej wartości środek plamy się prześwietla. Od M7 wartości powyżej 1 zostają w buforze HDR, a o tym, kiedy plama robi się płasko biała, decyduje krzywa mapowania tonów z panelu Framebuffers: przy `None (clamp)` obcięcie następuje przy 1 jak dawniej, przy domyślnej `ACES (fitted)` krzywa dochodzi do bieli dopiero w okolicy 7 |
| | `Cone` | dwa kąty od 1 do 60 stopni | `flashlightInnerDegrees`, `flashlightOuterDegrees` | stożek wewnętrzny i zewnętrzny. Równe wartości dają ostrą krawędź, duża różnica szeroki miękki brzeg |
| | `Beam range` | od 2 do 60 m | `flashlightRange` | tłumienie z promienia: jak daleko w korytarz sięga światło |
| `Point lights (crystals)` | tekst `Lit: 13 of 13 crystals (at most 16)` | odczyt | `round.crystals.size()`, `round.collectedCount` | liczba świateł to liczba niezebranych kryształów, z limitem tablicy w shaderze. 13 dla labiryntu startowego na początku rundy |
| | `Point colour`, `Point intensity` | kolor, od 0 do 10 | `pointColor`, `pointIntensity` | wszystkie światła punktowe mają wspólne ustawienia. Kolor zmienia też blask samych kryształów (`uEmissive`), intensywność nie: przy 0 ściany wokół kryształu gasną, a kryształ świeci dalej. To pokaz różnicy między światłem a emisją |
| | `Point radius` | od 0,5 do 12 m | `pointRadius` | tłumienie: przy 3 m światło gaśnie w półtorej komórki, przy 12 m zalewa pół labiryntu, także przez ściany |
| `Highlight (specular)` | `Strength` | od 0 do 2 | `specularStrength` | siła odbłysku. 0 zostawia sam składnik rozproszony |
| | `Shininess` | od 1 do 256, logarytmiczny | `shininess` | wykładnik: mały daje szeroką plamę, duży małą i ostrą |

Dolne granice zasięgów (2 m i 0,5 m) trzymają promień z dala od zera, przez które `attenuationForRadius` nie może dzielić. Górne granice intensywności są celowo dużo powyżej wartości startowych: da się prześwietlić scenę. Najlepiej robić to z otwartym panelem Framebuffers i przełączać `Tone mapping` między `None (clamp)` a `ACES (fitted)`: ta sama scena raz gubi szczegóły w białej plamie, raz je zachowuje.

Tryb cieniowania (Unlit, Gouraud, Phong, Blinn-Phong) przełącza lista `Lighting` w panelu **Renderer**, nie tutaj: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), sekcja 6.

### 6.3 Scenariusz pokazu na obronie

**Kroków nikt jeszcze nie wykonał ręcznie.** Opisują to, co wynika z kodu. Na zrzutach ekranu z Windowsa z M4 widać było tylko stan startowy z kroku 1, zgaszoną latarkę i różnicę jasności stron ścian z kroku 3 (przy ustawieniach startowych, bez ruszania suwaków). Lista do odhaczenia jest w [`../../guides/build-windows.md`](../../guides/build-windows.md).

1. **Trzy rodzaje świateł naraz.** Start gry. Mówię: ciepła plama na wprost to latarka (reflektor), zimna poświata na podłożu to księżyc (kierunkowe), turkusowe światło w głębi to kryształ (punktowe).
2. **Składnik otoczenia.** Ustawiam `Ambient` na czarny: miejsca bez światła stają się zupełnie czarne. Przywracam.
3. **Światło kierunkowe i Lambert.** Naciskam F (latarka gaśnie), żeby nie przeszkadzała. Rozwijam `Moon (directional)` i podnoszę `Moon intensity` do 1. Obracam `Moon yaw`: jasne stają się kolejne strony ścian, a po przejściu w inne miejsce nic się nie zmienia, bo kierunek jest wszędzie ten sam. Ustawiam `Moon pitch` na -90: ściany gasną (cosinus 0), grunt jest najjaśniejszy. Od czwartej części M7 przy obracaniu `Moon yaw` po gruncie wędrują też cienie ścian, a przy pitch -90 kurczą się pod same ściany (wynika z kodu, nie oglądane ręcznie).
4. **Światło punktowe i tłumienie.** Podchodzę do kryształu i zatrzymuję się około metra przed nim, żeby go nie zebrać (gracz zbiera kryształ, gdy stoi bliżej niż około 0,86 m od środka jego komórki: [`../game/gameplay.md`](../game/gameplay.md), sekcja 2). Zmieniam `Point radius` z 3 na 1, potem na 8. Mówię: w odległości równej promieniowi zostaje 5 procent, a krzywa ma zawsze ten sam kształt.
5. **Cień ma księżyc i latarka, kryształy nie.** Przy promieniu 8 pokazuję grunt za ścianą, przy której wisi kryształ: jest rozjaśniony. Mówię, że wzór pyta tylko o kąt i odległość, a cień to osobna technika (mapa cieni), którą od czwartej części M7 ma księżyc, a światła kryształów nie. Od piątej części M7 latarka ma własną mapę, więc tę część pokazu robię inaczej: cień księżyca odejmuje tylko światło księżyca, więc plama latarki jest w cieniu księżyca tak samo jasna jak poza nim, a cień latarki odejmuje tylko światło latarki. Pełny pokaz cieni: [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 6.
6. **Emisja a światło.** Przywracam `Point radius` 3 i przesuwam `Point intensity` do 0: ściany wokół kryształu gasną, a sam kryształ świeci dalej. Zmieniam `Point colour`: zmienia się i kryształ, i (po przywróceniu intensywności) blask na ścianach. Mówię: kryształ świeci składnikiem emisyjnym, a ściany oświetla osobne światło punktowe nad nim. Pokazuję linię `Lit: 13 of 13 crystals (at most 16)`.
7. **Reflektor.** Włączam latarkę (F). Staję przed ścianą. W `Cone` ustawiam oba kąty na 15: ostra krawędź. Potem 5 i 30: szeroki miękki brzeg. Mówię o porównywaniu cosinusów.
8. **Zasięg.** Patrzę w długi korytarz i przesuwam `Beam range` od 4 do 40.
9. **Odbłysk.** Ustawiam `Strength` 1 i `Shininess` 16, staję na wprost ściany i ruszam myszą: jasna plama przesuwa się z kierunkiem patrzenia. Przesuwam `Shininess` do 128: plama maleje. To wstęp do tematu 7.
10. **Testy.** `ctest --test-dir build/debug -C Debug --output-on-failure`: wzory tłumienia i stożka są sprawdzone liczbami, nie tylko obrazem.

## 7. Pułapki

1. **Kierunek światła a kierunek do światła.** `DirectionalLight::direction` i `uDirectionalDirection` mówią, dokąd światło **leci**. Wzory chcą kierunku **do** światła. Shader ma ten minus (`-uDirectionalDirection.xyz`) od czwartej części M7 w dwóch miejscach: w `computeLighting` (linia `vec3 toMoon = ...`) i w `moonFacing`. Minus dopisany drugi raz albo usunięty w pierwszym oświetla strony ścian odwrócone od księżyca, bez żadnego błędu. Usunięty w drugim psuje tylko bias cienia: powierzchnie zwrócone do księżyca dostają największą poprawkę zamiast najmniejszej.
2. **Wektory bez normalizacji.** `dot(N, L)` jest cosinusem tylko dla wektorów o długości 1. Normalna po interpolacji między wierzchołkami jest krótsza, a po macierzy normalnych ze skalą ma dowolną długość. Stąd `normalize` w `gouraud.vert` i w funkcji `surfaceNormal`, którą woła `lit.frag` (dwa razy: raz dla interpolowanej normalnej siatki, drugi raz dla normalnej z mapy, bo filtr tekstury i mipmapy też skracają wektory). Bez niego światło jest za ciemne i nierówne.
3. **Brak `max(..., 0)`.** Ujemny cosinus odejmuje światło. Ściana odwrócona od księżyca byłaby ciemniejsza, niż pozwala na to światło otoczenia.
4. **`pow` z ujemną podstawą.** W GLSL wynik `pow(x, y)` dla `x < 0` jest niezdefiniowany: na jednym sterowniku czerń, na innym `NaN` i migające piksele. `max` stoi przed `pow`.
5. **Stożek: większy kąt to mniejszy cosinus.** Warunek "wewnątrz stożka" to `cosAngle > próg`, nie `<`. Zamiana `uSpotCone.x` z `.y` daje ujemny mianownik i latarkę, która świeci wszędzie poza stożkiem.
6. **`MAX_POINT_LIGHTS` w trzech miejscach.** `scene/Light.hpp`, `common/lighting.glsl` i asercje rozmiaru w `scene/LightBlock.hpp`. Zmiana tylko w shaderze zmienia rozmiar bloku: program loguje wtedy przy starcie `Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code` ([`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md)). Zmiana tylko w C++ zatrzymuje build na `static_assert`.
7. **Dzielenie przez odległość równą zero.** `offset / lightDistance` daje `NaN`, gdy punkt powierzchni leży dokładnie w pozycji światła. W grze to się nie zdarza: latarka jest w ręce, w pudełku ciała gracza (limit suwaka `Hand right` pilnuje, żeby nie wyszła poza nie), a ściany nie wchodzą w to pudełko; bliska płaszczyzna obcinania nie dopuszcza powierzchni do oka, światła punktowe wiszą nad kryształami, około 1,55 m nad środkiem komórki, z dala od ścian i 0,15 m nad czubkiem samego kryształu. Światło wstawione **w** powierzchnię dałoby czarny albo migający piksel.
8. **Kierunek o długości zero.** `normalize` wektora zerowego to `NaN`, a jedno `NaN` w bloku robi czarny każdy oświetlony piksel. `packLightBlock` zamienia taki kierunek na `(0, -1, 0)`.
9. **Światło przechodzi przez ściany.** Dotyczy świateł kryształów: nie mają mapy cieni, i to jest brak techniki, a nie błąd (sekcja 2.8). Duży `Point radius` pokazuje to najwyraźniej. Księżyc od czwartej części M7 i latarka od piątej przez ściany nie przechodzą.
10. **Prześwietlenie.** Światła się sumują. Do M6 framebuffer obcinał wynik do 1: przy dużych intensywnościach środek plamy latarki robił się płaską białą plamą i znikała w niej tekstura. Od pierwszej części M7 rozwiązaniem jest bufor HDR z mapowaniem tonów ([`../renderer/post-process.md`](../renderer/post-process.md)). Stary obraz da się przywołać: `Tone mapping` ustawione na `None (clamp)` w panelu Framebuffers obcina jak dawniej.
11. **Dwie kopie wzorów.** Tłumienie i stożek są zapisane w C++ (testowane) i w GLSL (nietestowane). Zmiana w jednym miejscu nie zmienia drugiego i żaden test tego nie wykryje. Od czwartej części M7 to samo dotyczy drogi punktu do mapy cieni: `scene::shadowMapCoordinates` w C++ i trzy kroki w `moonShadow` w GLSL (sekcja 5.7).
12. **Uniformy materiału ustawione w złym programie.** `uSpecularModel`, `uSpecularStrength` i `uShininess` to zwykłe uniformy: należą do programu. `lit` i `gouraud` mają własne kopie. Ustawia je `drawLitMaze` po `use()` programu, którym zaraz rysuje.
13. **Odbłysk bez pozycji oka.** `uCameraPosition` musi być okiem, z którego rysowana jest klatka (interpolowanym), a nie pozycją z ostatniego kroku symulacji. Inaczej odbłysk drga przy ruchu.
14. **Światło liczone w złej przestrzeni.** Wszystko jest w przestrzeni świata. Normalna pomnożona przez macierz widoku albo pozycja fragmentu w przestrzeni widoku dałyby światło, które obraca się razem z kamerą.
15. **Kolor światła wysłany bez przeliczenia.** Do M6 korekcji gamma nie było wcale i suma świateł na wartościach sRGB była ciemniejsza w półcieniach, niż byłaby poprawnie. Od pierwszej części M7 rachunek jest liniowy, a pułapka zmieniła postać: kolor z próbnika jest wartością sRGB i musi być przeliczony **dokładnie raz**. Wysłany do shadera bez `gfx::srgbToLinear` da światło za jasne i wyblakłe, przeliczony dwa razy za ciemne i zbyt nasycone. W grze jedynym miejscem przeliczenia kolorów świateł jest `buildLightSet`, a pilnuje tego test `buildLightSet converts the colours from sRGB to linear and leaves the rest` ([`../gfx/color-space.md`](../gfx/color-space.md)).
16. **Emisja wzięta za światło.** `uEmissive` rozjaśnia tylko obiekt, który jest z nim rysowany. Kto ustawi duży blask i oczekuje jaśniejszych ścian, nie zobaczy żadnej zmiany: do tego służy światło punktowe. I odwrotnie: uniform trzyma wartość między wywołaniami rysowania, więc gdyby `MazeRenderer::draw` nie ustawiał go na czerń, ściany narysowane po kryształach poprzedniej klatki świeciłyby na turkusowo.
17. **Emisja pomnożona przez kolor powierzchni.** We wzorze projektu emisja przechodzi przez teksturę i `uTint`. Czarny teksel nie świeci, a biała emisja na czerwonej teksturze jest czerwona. Kto przenosi wzór z podręcznika (emisja dodana na końcu), dostanie inny obraz.
18. **Pola księżyca wzięte za dodatkowe światło.** `moonDiffuse` i `moonSpecular` są już wliczone w `diffuse` i `specular` (sekcja 4.3). Kto doda je do wyniku jeszcze raz, dostanie księżyc dwa razy jaśniejszy. Służą wyłącznie do odejmowania cienia.
19. **Cień pomnożony przez całe światło.** `diffuse * (1.0 - shadow)` zamiast `diffuse - moonDiffuse * shadow` gasi w cieniu księżyca także otoczenie, latarkę i światła kryształów. Obraz wygląda na pierwszy rzut oka wiarygodnie (cienie są ciemniejsze), ale latarka przestaje świecić w cieniu ściany.
20. **macOS, niesprawdzone.** Kompilator GLSL Apple może inaczej potraktować plik dołączany, pętlę z `break` albo blok `std140`. Nic z tego nie było uruchamiane na Macu: lista jest w [`../../guides/build-macos.md`](../../guides/build-macos.md).

## 8. Ćwiczenia

Ćwiczenia od 1 do 5 i ćwiczenie 15 są na kartce, od 6 do 14 w działającej grze. Po zmianie pliku shadera na Windowsie trzeba odświeżyć kopię katalogu `assets` (`cmake --build --preset debug --target copy_assets`) i nacisnąć `Reload shaders` ([`../gfx/shader-hot-reload.md`](../gfx/shader-hot-reload.md)). Po ćwiczeniu wycofaj zmianę (`git checkout src assets`).

1. **Lambert na kartce.** Światło kierunkowe leci w kierunku `(0, -1, 0)`. Policz czynnik Lamberta dla poziomego gruntu, dla pionowej ściany i dla dachu nachylonego o 30 stopni. (Odpowiedź: 1, 0, około 0,87.)
2. **Tłumienie na kartce.** Światło ma promień 4 m. Policz współczynniki i jasność w odległości 1 m, 2 m i 4 m. (Odpowiedź: `linear = 0,5`, `quadratic = 1,0625`, jasność 39 %, 16 %, 5 %.)
3. **Stożek na kartce.** Latarka ma kąty 10 i 20 stopni. Punkt leży pod kątem 15 stopni od osi. Policz czynnik stożka. (Wskazówka: `cos 10 = 0,985`, `cos 15 = 0,966`, `cos 20 = 0,940`. Odpowiedź: około 0,58, a nie 0,5, bo przejście jest liniowe w cosinusie.)
4. **Macierz normalnych na kartce.** Obiekt jest skalowany `(2, 1, 1)` bez obrotu. Normalna powierzchni skośnej to `(1, 1, 0) / sqrt(2)`. Policz normalną po pomnożeniu przez `mat3(model)` i przez macierz normalnych. Która jest prostopadła do stycznej `(-2, 1, 0)`?
5. **Kierunek księżyca na kartce.** Policz `directionFromAngles(90, -45)`. Które strony ścian będą oświetlone? (Odpowiedź: `(0,71, -0,71, 0)`, ściany zwrócone ku `-X` i grunt.)
6. **Samo światło otoczenia.** W panelu ustaw intensywność księżyca, latarki i świateł punktowych na 0. Co widać i dlaczego ściany nie mają żadnego kształtu?
7. **Bez `max`.** W `diffuseFactor` zamień ciało na `return dot(normal, toLight);`. Przeładuj shadery, wyłącz latarkę. Które ściany pociemniały i dlaczego?
8. **Ostra krawędź.** W `computeLighting` zamień linię `float cone = clamp(...)` na `float cone = cosAngle > uSpotCone.y ? 1.0 : 0.0;`. Jak wygląda brzeg plamy? Który z dwóch kątów panelu przestał mieć znaczenie?
9. **Samo `1 / d²`.** W `attenuationFactor` zamień wzór na `1.0 / (lightDistance * lightDistance)`. Co widać na ścianie tuż przy krysztale i dlaczego?
10. **Odwrócony kierunek.** W `computeLighting` usuń minus w linii `vec3 toMoon = -uDirectionalDirection.xyz;`. Które strony ścian są teraz jasne? Porównaj z tabelą z sekcji 2.3. Gdzie leżą cienie ścian względem jasnych stron i dlaczego się nie przesunęły? (Wskazówka: mapa cieni nie czyta tej linii, bierze kierunek z `game::moonDirection`.)
11. **Odbłysk w kolorze powierzchni.** W `lit.frag` zamień ostatnią linię na `fragColor = vec4(surface * (diffuse + specular), 1.0);`. Ustaw `Strength` 1. Jak zmienił się kolor odbłysku na ciemnych fugach tekstury i dlaczego?
12. **Emisja na wierzch.** W `lit.frag` zamień ostatnią linię na `fragColor = vec4(surface * diffuse + uEmissive + specular, 1.0);`. Podejdź do kryształu w trybie `Phong`. Co stało się z rysunkiem jego tekstury i dlaczego w projekcie emisja jest w nawiasie?
13. **Emisja bez zerowania.** W `MazeRenderer::draw` usuń linię `shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});` i zbuduj program. Co świeci od drugiej klatki i dlaczego? (Wskazówka: uniform pamięta ostatnią wartość, a kryształy są rysowane po ścianach.)
14. **Cień na całe światło.** W `lit.frag` zamień linię `vec3 diffuse = max(lighting.diffuse - lighting.moonDiffuse * shadow, 0.0);` na `vec3 diffuse = lighting.diffuse * (1.0 - shadow);`. Stań w cieniu ściany od księżyca i poświeć latarką w grunt. Co się stało z plamą latarki i z kolorem samego cienia? (Pułapka 19.)
15. **Pudełko światła na kartce.** Pudełko `bounds` ma `min = (-4, 0, 2)` i `max = (10, 6, 30)`, światło leci prosto w dół. Podaj `extent` z `directionalLightSpace`. (Odpowiedź: `(15, 29, 7)`: 14, 28 i 6 m plus `LIGHT_BOX_MARGIN` 0,5 m z każdej strony. To liczby z testu `a light that shines straight down sees the bounds from above`.)

## 9. Pytania kontrolne

1. **Czym różnią się światło kierunkowe, punktowe i reflektor?**
   Kierunkowe ma tylko kierunek, ten sam w całej scenie, i nie słabnie z odległością. Punktowe ma pozycję, świeci we wszystkie strony i słabnie z odległością. Reflektor to punktowe ograniczone do stożka: dochodzi oś i dwa kąty.

2. **Z jakich składników składa się model odbicia Phonga?**
   Z trzech: otoczenia (stała, udaje światło odbite wiele razy), rozproszonego (prawo Lamberta, zależy od kąta między normalną a kierunkiem do światła) i zwierciadlanego (odbłysk, zależy także od kierunku do oka).

3. **Co mówi prawo Lamberta i dlaczego we wzorze jest iloczyn skalarny?**
   Jasność matowej powierzchni jest proporcjonalna do cosinusa kąta między normalną a kierunkiem do światła, bo ta sama wiązka padająca ukośnie rozkłada się na większą powierzchnię. Dla wektorów o długości 1 iloczyn skalarny jest tym cosinusem.

4. **Po co `max(dot(N, L), 0)`?**
   Ujemny cosinus znaczy, że światło jest za powierzchnią. `max` zamienia go na zero. Bez tego ujemna wartość odejmowałaby światło innych źródeł.

5. **Dlaczego kolor tekstury mnoży część rozproszoną, a odbłysk jest dodawany?**
   Powierzchnia pochłania część barw światła rozproszonego, więc jej kolor je filtruje (mnożenie). Odbłysk to odbicie samego źródła od powierzchni, więc ma kolor światła i jest dodawany na wierzch.

6. **Jak wygląda wzór tłumienia i skąd bierze się część kwadratowa?**
   `1 / (constant + linear * d + quadratic * d * d)`. Część kwadratowa jest fizyczna: światło z punktu rozchodzi się po powierzchni kuli, a ta rośnie z kwadratem promienia. Stała 1 daje pełną jasność w źródle, część liniowa kształtuje spadek blisko źródła.

7. **Jak projekt liczy współczynniki tłumienia z promienia?**
   `constant = 1`, `linear = 2 / r`, `quadratic = 17 / r²`. W odległości `r` mianownik to 20, więc zostaje 5 procent jasności, dla każdego promienia.

8. **Czy światło punktowe kończy się na swoim promieniu?**
   Nie. Wzór nigdy nie daje zera. Promień to miejsce, w którym zostaje 5 procent. Dalej światło jest, tylko za słabe, żeby je widzieć.

9. **Po co reflektor ma dwa kąty?**
   Żeby brzeg plamy był miękki. Wewnątrz stożka wewnętrznego jasność jest pełna, poza zewnętrznym zerowa, a pomiędzy zmienia się płynnie. Jeden kąt dałby ostrą krawędź.

10. **Dlaczego shader porównuje cosinusy, a nie kąty?**
    Cosinus kąta między osią a kierunkiem do punktu to jeden iloczyn skalarny. Sam kąt wymagałby `acos` dla każdego fragmentu. Kąty stożka są zamieniane na cosinusy raz, w C++.

11. **Stożek wewnętrzny ma większy czy mniejszy cosinus niż zewnętrzny?**
    Większy, bo cosinus maleje, gdy kąt rośnie. "Wewnątrz stożka" znaczy "cosinus większy od progu".

12. **Co się stanie, gdy oba kąty stożka są równe?**
    Wzór dzieliłby przez zero. `coneCosines` podnosi wtedy cosinus wewnętrzny o 0,001 nad zewnętrzny, więc wychodzi ostra krawędź bez `NaN`.

13. **Dlaczego normalnej nie mnoży się przez macierz modelu?**
    Po pierwsze normalna jest kierunkiem i przesunięcie nie może jej zmieniać: bierze się część 3 na 3. Po drugie przy nierównej skali ta część przekrzywia normalną, która przestaje być prostopadła do powierzchni.

14. **Jaka macierz jest właściwa i dlaczego?**
    Odwrotność części 3 na 3 macierzy modelu, transponowana. Wynika to z warunku, że normalna ma zostać prostopadła do każdej stycznej: `G^T M` musi być macierzą jednostkową.

15. **Kiedy macierz normalnych jest równa zwykłej części 3 na 3?**
    Gdy obiekt jest tylko obrócony i przesunięty, bo odwrotność macierzy obrotu to jej transpozycja. Tak jest dziś dla wszystkich obiektów sceny: ścian, bramy i kryształów.

16. **Gdzie projekt liczy macierz normalnych i dlaczego tam?**
    Na procesorze, w `scene::normalMatrix`, raz na obiekt, i wysyła jako `uNormalMatrix`. W shaderze odwrotność byłaby liczona od nowa dla każdego wierzchołka, a wynik jest ten sam dla całego obiektu.

17. **W jakiej przestrzeni liczone jest światło?**
    W przestrzeni świata: pozycje świateł, oko, pozycja punktu i normalna. Normalna z mapy normalnych jest zapisana w przestrzeni stycznej, więc `surfaceNormal` przenosi ją do świata przed wzorami.

18. **Co to jest `LightSet` i jak trafia do shadera?**
    Struktura ze wszystkimi światłami jednej klatki: otoczenie, jedno kierunkowe, do 16 punktowych z licznikiem, jeden reflektor z przełącznikiem. `packLightBlock` zamienia ją na 928 bajtów w układzie `std140`, a `UniformBuffer::update` kopiuje je na kartę raz na klatkę.

19. **Dlaczego `direction` światła kierunkowego to kierunek lotu światła, a shader ma minus?**
    Bo tak łatwiej myśleć o źródle: "księżyc świeci w dół". Wzory potrzebują kierunku od punktu do światła, czyli przeciwnego.

20. **Co robi `addLight` i dlaczego jest jedną funkcją dla trzech rodzajów świateł?**
    Dodaje do wyniku część rozproszoną i odbłysk jednego światła. Rodzaje świateł różnią się tylko tym, jak powstaje wektor do światła i ile światła dociera (`radiance`). Dalej wzory są wspólne. Od czwartej części M7 wołają ją światła punktowe i latarka. Księżyc liczy te same dwa wyrazy tymi samymi funkcjami (`diffuseFactor`, `specularFactor`), ale sam, bo musi je zachować osobno.

21. **Dlaczego światło kryształu widać za ścianą?**
    Bo światła kryształów nie mają cieni. Wzory nie sprawdzają, czy między światłem a punktem coś stoi. Mapę cieni mają od czwartej części M7 księżyc, a od piątej latarka. Cienia świateł punktowych w planie nie ma.

22. **Co oznacza `inout` w `addLight`?**
    Parametr jest kopiowany do funkcji przy wejściu i z powrotem przy wyjściu. GLSL nie ma referencji, a funkcja musi dopisać do struktury wołającego.

23. **Dlaczego pętla po światłach ma stałą granicę i `break`?**
    To ostrożna forma: GLSL 4.10 jej nie wymaga, wymaga jej GLSL ES 1.00. Zaletą jest to, że pętla nigdy nie wyjdzie poza tablicę 16 elementów, niezależnie od wartości `uPointCount`.

24. **Co w tym kodzie jest sprawdzone testami, a co nie?**
    Testami: wzory tłumienia i stożka w C++, kierunek z kątów, pakowanie bloku i jego rozmiar, a od czwartej części M7 także `LightSpace` (osiem przypadków w `tests/ShadowTests.cpp`). Nie: kod GLSL (druga kopia wzorów, w tym pola księżyca i `moonFacing`) i panel. Obraz jest sprawdzony na zrzutach ekranu z Windowsa, a widżetów nikt nie klikał.

25. **Skąd gra bierze światła punktowe?**
    Z kryształów rundy: `game::crystalLightPositions` daje pozycję 0,15 m nad czubkiem każdego kryształu, który nie jest zebrany. Zebrany kryształ traci światło. Najwyżej 16, bo tyle kryształów może mieć labirynt i tyle ma tablica w shaderze.

26. **Co to jest składnik emisyjny i jak wygląda w projekcie?**
    Światło, które powierzchnia oddaje sama, niezależnie od świateł sceny. W projekcie to uniform `uEmissive` w shaderach fragmentów: `kolor = powierzchnia * (światło rozproszone + emisja) + odbłysk`. Dla kryształów ma kolor ich świateł, dla wszystkiego innego jest czarny.

27. **Czy emisja kryształu oświetla ściany?**
    Nie. Zmienia tylko kolor fragmentów samego kryształu. Ściany oświetla osobne światło punktowe nad kryształem, w tym samym kolorze i z tym samym pulsem.

28. **Dlaczego emisja jest mnożona przez kolor powierzchni?**
    Żeby tekstura kryształu została widoczna: emisja dodana na wierzch dałaby każdemu fragmentowi tę samą wartość i zatarła jej rysunek.

29. **W jakiej przestrzeni kolorów są kolory świateł w panelu, a w jakiej w shaderze?**
    W panelu i w `LightingSettings` są wartościami sRGB, takimi, jakie pokazuje próbnik koloru. `game::buildLightSet` przelicza cztery kolory (otoczenie, księżyc, latarka, światła punktowe) funkcją `gfx::srgbToLinear`, więc do bloku `LightBlock` trafiają wartości liniowe. Intensywności są mnożnikami i nie są przeliczane.

30. **Dlaczego światło trzeba liczyć na wartościach liniowych?**
    Bo mnożenie przez światło i dodawanie świateł opisują ilość światła, a liczby sRGB nie są do niej proporcjonalne: 0,5 w sRGB to tylko około 0,214 światła bieli. Suma liczona na wartościach sRGB daje za ciemne półcienie i za ostre przejścia.

31. **Co się dzieje, gdy suma świateł przekroczy 1?**
    Od pierwszej części M7 nic nie ginie: bufor sceny jest zmiennoprzecinkowy (`GL_RGBA16F`) i przechowuje wartość taką, jaka wyszła. W zakres od 0 do 1 sprowadza ją krzywa mapowania tonów w przebiegu składającym. Do M6 framebuffer okna obcinał ją do 1.

32. **Po co struktura `Lighting` ma pola `moonDiffuse` i `moonSpecular`, skoro księżyc jest już w `diffuse` i `specular`?**
    Żeby wołający mógł odjąć cień księżyca. W cieniu od sumy odejmuje się dokładnie to, co księżyc do niej dołożył, razy udział cienia. Bez osobnych pól trzeba by przyciemnić całą sumę, czyli także otoczenie, latarkę i światła kryształów.

33. **Które światła gry rzucają cień?**
    Księżyc (od czwartej części M7) i latarka (od piątej, z własną mapą o rzucie perspektywicznym). Światła kryształów nie: ich światło przechodzi przez ściany.

34. **Dlaczego gałąź księżyca w `computeLighting` nie woła `addLight`?**
    `addLight` dopisuje oba wyrazy wprost do sumy i ich nie zwraca. Księżyc potrzebuje ich osobno, więc liczy je tymi samymi funkcjami, zapisuje w polach `moonDiffuse` i `moonSpecular`, a potem dodaje do sumy.

35. **Co zwraca `moonFacing` i do czego służy?**
    Cosinus kąta między normalną a kierunkiem do księżyca, bez obcinania do zera. Nie trafia do światła, tylko do biasu cienia: powierzchnia pochylona względem księżyca dostaje większą poprawkę głębi.

36. **Co to jest `scene::LightSpace`?**
    Widok i rzutowanie światła, czyli to, co dla kamery dają `viewMatrix` i `projectionMatrix`, plus rozmiar pudełka w metrach. Dla księżyca rzut jest ortograficzny, bo promienie są równoległe. Od piątej części M7 dla latarki jest perspektywiczny (`spotLightSpace`): jej promienie wychodzą z jednego punktu, z ręki. Tymi macierzami rysuje się mapę cieni i tymi samymi się ją czyta.

37. **Od czego zależy pudełko światła księżyca, a od czego nie?**
    Od pudełka, które ma objąć (teren z zapasem na wysokość słupka), i od kierunku księżyca. Nie zależy od kamery, więc mapa pokrywa ten sam grunt w każdej klatce i cienie nie migoczą przy ruchu gracza.

38. **Co robi `directionalLightSpace`, gdy światło świeci prosto w dół albo kierunek ma długość zero?**
    Przy kierunku prawie pionowym (`|y| > 0,999`, mniej niż około 2,56 stopnia od pionu) bierze jako "górę" widoku `-Z` zamiast `+Y`, bo `lookAt` z górą równoległą do kierunku patrzenia dzieli przez zero. Kierunek o długości zero zastępuje kierunkiem prosto w dół.

39. **Dlaczego macierz cienia nie jest w bloku `LightBlock`?**
    Sampler mapy cieni nie może być składową bloku uniformów, więc i tak jest zwykłym uniformem. Liczby tej samej mapy (macierz, bias, promień PCF, siła) trzymam obok niego. Blok się nie zmienił: nadal 928 bajtów.

## 10. Źródła

- LearnOpenGL, "Basic Lighting" (<https://learnopengl.com/Lighting/Basic-Lighting>): składniki otoczenia, rozproszony i zwierciadlany, normalne, macierz normalnych.
- LearnOpenGL, "Light casters" (<https://learnopengl.com/Lighting/Light-casters>): światło kierunkowe, punktowe z tłumieniem, reflektor z miękkim brzegiem i porównywaniem cosinusów. Rozdział podaje tabelę gotowych współczynników dla wybranych zasięgów. Projekt jej nie używa: liczy współczynniki z promienia własnym wzorem (2 i 17).
- LearnOpenGL, "Multiple lights" (<https://learnopengl.com/Lighting/Multiple-lights>): jedna funkcja na rodzaj światła i suma wyników.
- LearnOpenGL, "Advanced Lighting" (<https://learnopengl.com/Advanced-Lighting/Advanced-Lighting>): Blinn-Phong.
- LearnOpenGL, "Shadow Mapping" (<https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping>): przestrzeń światła, rzut ortograficzny dla światła kierunkowego, droga punktu do mapy cieni (sekcja 5.7).
- docs.gl (<https://docs.gl>), strony funkcji GLSL 4: `reflect` (<https://docs.gl/sl4/reflect>), `dot`, `normalize`, `pow`, `clamp`, `length`.
- Specyfikacja GLSL 4.10 (<https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.10.pdf>): kwalifikator `inout`, struktury, funkcje wbudowane, niezdefiniowany wynik `pow` dla ujemnej podstawy.
- Bui Tuong Phong, "Illumination for Computer Generated Pictures" (1975): model odbicia. Johann Heinrich Lambert, "Photometria" (1760): prawo cosinusów.
- Dokumenty w tym repozytorium: [`../renderer/lighting-gouraud-phong.md`](../renderer/lighting-gouraud-phong.md), [`../game/flashlight.md`](../game/flashlight.md), [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md), [`../gfx/shader-includes.md`](../gfx/shader-includes.md), [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md) (normalna z mapy normalnych), [`transforms.md`](transforms.md) (`normalMatrix`), [`camera.md`](camera.md) (yaw, pitch, `forward`), [`../debug-ui.md`](../debug-ui.md) (panele), [`../game/gameplay.md`](../game/gameplay.md) (kryształy, `crystalGlow`, `GameplayRenderer`), [`../gfx/color-space.md`](../gfx/color-space.md) (sRGB i wartości liniowe, gdzie kolory są przeliczane), [`../renderer/post-process.md`](../renderer/post-process.md) (bufor HDR, ekspozycja, mapowanie tonów), [`../renderer/shadows.md`](../renderer/shadows.md) (mapa cieni księżyca, od czwartej części M7), notatki [`../../decisions/shadow-takes-only-moon-light.md`](../../decisions/shadow-takes-only-moon-light.md), [`../../decisions/shadow-matrix-as-plain-uniforms.md`](../../decisions/shadow-matrix-as-plain-uniforms.md) i [`../../decisions/shadow-box-fitted-to-terrain.md`](../../decisions/shadow-box-fitted-to-terrain.md) (cienie), notatki [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md) (obowiązuje od pierwszej części M7), [`../../decisions/no-gamma-until-m7.md`](../../decisions/no-gamma-until-m7.md) (zastąpiona, historia) i [`../../decisions/crystal-count-and-gate-threshold.md`](../../decisions/crystal-count-and-gate-threshold.md) (ile kryształów, a więc ile świateł punktowych). Notatka [`../../decisions/dead-end-lights.md`](../../decisions/dead-end-lights.md) opisuje rozwiązanie z M4 (światła w ślepych zaułkach), które M5 zastąpił.
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o oświetleniu).
