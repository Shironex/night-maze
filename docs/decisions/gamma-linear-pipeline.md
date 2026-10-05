# Gamma od M7: tekstury sRGB, rachunek liniowy w buforze HDR, kodowanie na końcu klatki

Data: 2026-10-05. Stan: obowiązuje. Zastępuje [`no-gamma-until-m7.md`](no-gamma-until-m7.md).
Kod: [`src/gfx/ColorSpace.hpp`](../../src/gfx/ColorSpace.hpp), [`ColorSpace.cpp`](../../src/gfx/ColorSpace.cpp), [`src/gfx/Texture2D.cpp`](../../src/gfx/Texture2D.cpp) (`internalFormatFor`), [`src/gfx/Cubemap.cpp`](../../src/gfx/Cubemap.cpp), [`src/assets/AssetCache.cpp`](../../src/assets/AssetCache.cpp), [`src/game/Lighting.cpp`](../../src/game/Lighting.cpp) (`buildLightSet`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onRender`, `crystalEmissive`), [`src/game/ColliderLines.cpp`](../../src/game/ColliderLines.cpp), [`assets/shaders/common/color.glsl`](../../assets/shaders/common/color.glsl), [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag). Dokumenty modułów: [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md), [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md).

## 1. Kontekst

Od M4 do M6 gra liczyła światło na liczbach z plików PNG, czyli na liczbach sRGB, i zapisywała wynik prosto do okna. Notatka [`no-gamma-until-m7.md`](no-gamma-until-m7.md) odłożyła poprawkę do chwili, w której zmieni się cały koniec potoku: do M7, gdy scena zaczyna być rysowana do bufora zmiennoprzecinkowego.

Ta chwila nadeszła z pierwszą częścią M7. Scena trafia teraz do tekstury `GL_RGBA16F`, a na ekran przenosi ją osobny przebieg. Bufor HDR ma sens tylko dla wartości liniowych: "jaśniejsze niż biel" znaczy coś dopiero wtedy, gdy liczba jest proporcjonalna do ilości światła, a mapowanie tonów i planowany bloom są zdefiniowane na takich liczbach. Trzeba było więc rozstrzygnąć naraz kilka rzeczy: skąd biorą się wartości liniowe, którą krzywą się je liczy, kto mówi, czym jest tekstura, i co zrobić z kolorami, które nie pochodzą z tekstur.

## 2. Decyzja

Potok ma **trzy etapy i każdy kolor przechodzi je raz**: tekstury koloru są teksturami sRGB i karta dekoduje je przy odczycie, shadery liczą na wartościach liniowych i zapisują je bez zmian do bufora `GL_RGBA16F`, a ostatni przebieg klatki koduje wynik z powrotem na sRGB.

Do tego pięć mniejszych rozstrzygnięć, które z tej decyzji wynikają:

1. Przeliczanie używa **dokładnej funkcji normy sRGB** (odcinek prosty i potęga 2,4), nie przybliżenia `pow(x, 2.2)`.
2. Przestrzeń kolorów tekstury jest **obowiązkowym argumentem** (`gfx::ColorSpace`) i wybiera ją wołający. Klasa nie zgaduje z nazwy pliku.
3. Kolory wpisane liczbami i kolory z paneli **zostają liczbami sRGB** i są przeliczane raz, w miejscu użycia.
4. Widoki diagnostyczne (normalne, UV) **omijają ekspozycję i mapowanie tonów**, a ich shadery stosują przeliczenie odwrotne do kodowania.
5. `Kd` materiału **nie jest przeliczane**: wszystkie materiały gry mają białe `Kd`.

Gdzie stoi samo kodowanie (shader, nie `GL_FRAMEBUFFER_SRGB`), zapisuje osobna notatka [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md).

## 3. Rozważane możliwości

Potok jako całość:

| Możliwość | Zalety | Wady |
|---|---|---|
| **Trzy etapy od razu z buforem HDR (wybrana)** | rachunek światła jest poprawny. Bufor HDR, mapowanie tonów i późniejsze efekty dostają wartości, na których są zdefiniowane. Jedna zmiana końca potoku, tak jak zapowiadała poprzednia notatka | zmienia się wygląd całej sceny, więc wszystkie wartości startowe świateł trzeba dobrać od nowa. Zmienia się sygnatura trzech funkcji i każde ich wywołanie. Obraz nie jest już identyczny ze starym nawet w trybie `Unlit` |
| Bufor HDR bez gammy: `GL_RGBA16F`, ale tekstury dalej `GL_RGB8` i bez kodowania | najmniej zmian, wygląd jak w M6 | wartości w buforze nie są liniowe, więc "ponad 1" nic nie znaczy fizycznie. Krzywe mapowania tonów dałyby przypadkowy wynik. Bloom dobrany do tego stanu trzeba by dobrać drugi raz |
| Ręczne `pow(tekstura, 2.2)` w shaderach zamiast formatu sRGB | bez zmian w C++ | potęga na fragment w każdym shaderze sceny. Filtr tekstury i mipmapy dalej działają na liczbach sRGB. Inna krzywa niż ta, którą zna karta |

Mniejsze rozstrzygnięcia:

| Pytanie | Wybrane | Druga możliwość | Dlaczego nie druga |
|---|---|---|---|
| Która krzywa | dokładna funkcja normy, ta sama w `ColorSpace.cpp` i w `common/color.glsl` | `pow(x, 2.2)` i `pow(x, 1 / 2.2)` | karta dekoduje `GL_SRGB8` funkcją dokładną. Z przybliżeniem po jednej stronie kolor wpisany i ten sam kolor z tekstury dają różne wartości liniowe (przy 0,2 różnica to około jednej ósmej), a tekstura bez światła nie wraca na ekran jako te same bajty |
| Kto wybiera przestrzeń tekstury | wołający, argumentem bez wartości domyślnej | zgadywanie z nazwy pliku (`_normal`) albo wartość domyślna `Srgb` | nazwa pliku to umowa, której nikt nie sprawdza. Wartość domyślna pozwala zapomnieć o argumencie przy mapie normalnych, a taki błąd nie daje komunikatu, tylko złe światło. Bez wartości domyślnej zapomnienie się nie kompiluje |
| W jakiej przestrzeni trzymać kolory ustawień | sRGB w `LightingSettings` i w `m_clearColor`, przeliczenie w `buildLightSet`, przed `glClearColor`, w `crystalEmissive` i w `ColliderLines` | trzymać w ustawieniach wartości liniowe | selektor koloru ImGui pokazuje i zapisuje liczby tak, jak są w pamięci, i rysuje z nich próbkę koloru. Z wartościami liniowymi próbka w panelu miałaby inny kolor niż światło w scenie. Liczby sRGB są też tymi, które umiem dobrać na oko |
| Widoki diagnostyczne | bez ekspozycji i mapowania tonów (kopia ustawień w `onRender`), `srgbToLinear` w shaderze | zostawić je w pełnym potoku | widok pokazuje dane, nie światło. Krzywa ACES i kodowanie zmieniłyby liczby i normalna `(0, 1, 0)` nie byłaby już kolorem `(0.5, 1.0, 0.5)`, z którego się ją odczytuje |
| `Kd` materiału | używane jako mnożnik bez przeliczenia | przeliczać w `AssetCache` przy wczytaniu | wszystkie pięć materiałów ma `Kd 1 1 1`, a biel to 1 w obu przestrzeniach. Przeliczenie nie zmieniłoby dziś ani jednego piksela. Ograniczenie jest zapisane w komentarzu przy `drawModel` |

## 4. Uzasadnienie i skutki

**Dlaczego razem z buforem HDR.** Tak przewidywała poprzednia notatka i jej rozumowanie się potwierdziło: kodowanie jest naturalnym ostatnim krokiem przebiegu, który i tak musiał powstać, żeby przenieść bufor na ekran. Zrobione wcześniej, w `lit.frag`, byłoby teraz usuwane.

**Co dzięki temu dostaję.**

- Światła sumują się i mnożą na wartościach proporcjonalnych do światła. Dwie latarki dają dwa razy więcej światła, a nie biel.
- Filtr tekstury i mipmapy uśredniają wartości liniowe (o ile sterownik dekoduje przed filtrem, co specyfikacja zaleca, ale nie wymusza).
- Wartości ponad 1 mają znaczenie: kryształ świeci w kanale zielonym około dwa razy jaśniej niż biel i mapowanie tonów ma z czego zrobić miękkie przejście.
- Miejsce na błąd jest wąskie: jedno kodowanie w klatce, jedno przeliczenie na każdy wpisany kolor, argument, którego nie da się pominąć.

**Co przez to tracę.**

- Wszystkie wartości startowe zostały dobrane od nowa: światło otoczenia, intensywności księżyca, latarki i kryształów, `CRYSTAL_GLOW_STRENGTH`, jasność nieba, kolor tła. Tabela starych i nowych liczb jest w [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md).
- Obraz w trybie `Unlit` różni się od obrazu sprzed M7 na granicach jasnych i ciemnych tekseli (zgłoszone: najwyżej 22 poziomy, średnio 1,1, tylko na spoinach cegieł). To skutek poprawniejszego filtrowania, ale porównanie "piksel w piksel" ze starymi zrzutami ekranu przestało być możliwe.
- Stałe normy są zapisane dwa razy, w C++ i w GLSL, i żaden test nie pilnuje ich zgodności.
- Podglądy tekstur sRGB w panelu Assets wymagają rozszerzenia `GL_EXT_texture_sRGB_decode`, żeby wyglądać jak pliki. Bez niego są ciemniejsze.
- `pointColor` jest przeliczany w dwóch miejscach (światło i świecenie kryształu), które muszą zostać zgodne.

**Co z poprzedniej notatki nadal obowiązuje.** Argument przeciw "połowie poprawki" (samo kodowanie bez dekodowania daje obraz wyprany) jest dziś pułapką opisaną w dokumencie modułu. Zdanie, że mapa normalnych nigdy nie może być teksturą sRGB i że format trzeba wybierać dla każdej tekstury osobno, stało się obowiązkowym argumentem `ColorSpace`. Obawa o kolory paneli ImGui jest powodem, dla którego kodowanie stoi w shaderze.

**Czego nie zmierzyłem.** Porównanie obrazu ze starym potokiem i liczby testów (269 przypadków, 102103 asercje) są zgłoszone dla Windowsa, nie powtarzałem ich. Na macOS ten potok nie był uruchamiany. Nie sprawdzałem na żadnej karcie, czy dekodowanie sRGB odbywa się przed filtrem, inaczej niż przez zgłoszone porównanie obrazów.

## 5. Kiedy wrócić do tej decyzji

- Gdy pojawi się model z kolorowym `Kd`: wtedy `uTint` trzeba przeliczać (najprościej raz, przy wczytaniu materiału w `AssetCache`).
- Gdy kolory ustawień zaczną być zapisywane do pliku albo wymieniane z innym programem: trzeba wtedy zapisać w formacie, w której przestrzeni są.
- Gdy dojdzie tekstura, która jest kolorem **i** danymi naraz (na przykład kolor z połyskiem w kanale alfa): alfa nie jest dekodowana, więc taki układ działa, ale dane w kanałach RGB już nie.
- Gdyby macOS pokazał inny obraz niż Windows: pierwszym podejrzanym jest framebuffer okna (notatka [`srgb-encode-in-shader.md`](srgb-encode-in-shader.md)).
- Przy bloomie i mgle (kolejne części M7): oba efekty mają działać na wartościach liniowych, przed ekspozycją. Bloom z drugiej części M7 tak działa ([`bloom-half-resolution-three-targets.md`](bloom-half-resolution-three-targets.md), [`bright-pass-keeps-hue.md`](bright-pass-keeps-hue.md)). Jeśli któryś zostanie dobrany na oko po kodowaniu, ta decyzja została złamana.
