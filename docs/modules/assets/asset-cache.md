# Moduł assets: pamięć podręczna modeli i tekstur, panel Assets

Kamień milowy: M2 + M3, zaktualizowany w M4 (mapy normalnych części, płaska mapa zastępcza, przełącznik `Normal mapping` w panelu), w M5 (drugi użytkownik, `GameplayRenderer`: sześć modeli i osiem tekstur zamiast trzech i czterech, kod klasy i panelu bez zmian) i w drugiej części M6 (teren zamiast płytek podłogi: pięć modeli, trzeci użytkownik `TerrainRenderer`, który prosi o dwie tekstury wprost przez `texture()`, kod klasy i panelu bez zmian) oraz w pierwszej części M7 (każda tekstura ma przestrzeń kolorów: `texture()` dostała drugi, obowiązkowy argument `gfx::ColorSpace`, panel pokazuje `sRGB` albo `linear` i czyta miniatury tekstur sRGB bez dekodowania). Tematy wykładu: 4 (Wczytywanie OBJ: "cache meshy", lista załadowanych modeli) i 5 (Tekstury: podgląd tekstur, filtrowanie, przełącznik map normalnych).
Kod: [`src/assets/AssetCache.hpp`](../../../src/assets/AssetCache.hpp), [`src/assets/AssetCache.cpp`](../../../src/assets/AssetCache.cpp), panel w [`src/debug/panels/AssetsPanel.hpp`](../../../src/debug/panels/AssetsPanel.hpp) i [`src/debug/panels/AssetsPanel.cpp`](../../../src/debug/panels/AssetsPanel.cpp), użytkownicy: [`src/game/MazeRenderer.cpp`](../../../src/game/MazeRenderer.cpp) i [`src/game/GameplayRenderer.cpp`](../../../src/game/GameplayRenderer.cpp), wspólna pętla rysowania w [`src/game/ModelDraw.cpp`](../../../src/game/ModelDraw.cpp).

Część modułu `assets`. Wstęp do modułu jest w [`README.md`](README.md). Ten dokument łączy cztery inne: [`obj-loader.md`](obj-loader.md) (plik OBJ i MTL jako dane procesora: `ObjModel`), [`images.md`](images.md) (plik PNG jako piksele: `Image`), [`../gfx/mesh.md`](../gfx/mesh.md) (siatka na karcie: `gfx::Mesh`) i [`../gfx/textures.md`](../gfx/textures.md) (tekstura na karcie: `gfx::Texture2D`, filtry, anizotropia, shadery `textured`). Kto z wczytanych modeli rysuje labirynt, opisuje [`../game/maze-rendering.md`](../game/maze-rendering.md), a kto rysuje kryształy i bramę: [`../game/gameplay.md`](../game/gameplay.md). Czym jest mapa normalnych i co robi z nią shader, opisuje [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md): tutaj jest tylko to, co robi z nią pamięć podręczna i panel.

## 1. Po co to jest

Loadery z poprzednich kroków zwracają **dane procesora**: wektory wierzchołków, indeksów i pikseli. Klasy `gfx` zamieniają takie dane na **obiekty karty graficznej**. Brakowało miejsca, które robi jedno i drugie po kolei, pamięta wynik i odpowiada na pytanie "daj mi model z tego pliku".

Tym miejscem jest klasa `assets::AssetCache`, **pamięć podręczna assetów** (asset cache). Robi pięć rzeczy:

1. wczytuje każdy model i każdą teksturę **raz**, przy pierwszej prośbie, i przy kolejnych oddaje ten sam obiekt,
2. oddaje wskaźniki, które **pozostają ważne** przez całe jej życie,
3. łączy model z jego materiałami: każda część siatki dostaje od razu kolor, teksturę i mapę normalnych,
4. znosi błędy bez zatrzymywania programu: brak pliku to linia w logu, wskaźnik pusty albo tekstura zastępcza (biała dla koloru, płaska dla mapy normalnych),
5. trzyma jedno wspólne ustawienie filtra i anizotropii dla wszystkich tekstur, w tym map normalnych.

Drugą połową dokumentu jest panel **Assets**: pokaz tematów 4 i 5 na obronie (lista modeli, podgląd tekstur, przełącznik filtra, suwak anizotropii, tryb widoku, pole `Normal mapping`).

**Stan na dziś, uczciwie.** Gra wczytuje przez pamięć podręczną pięć modeli i osiem tekstur: cztery obrazy koloru i cztery mapy normalnych. Do M4 były to trzy modele i cztery tekstury (trzecim modelem była płytka podłogi, usunięta w drugiej części M6), i tego stanu dotyczą pomiary w tym i w następnym akapicie. Na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik 610.74) build Debug i Release przechodzi bez ostrzeżeń, a program startuje bez linii `[error]`. Na zrzutach ekranu sprawdzone są: tekstury na ścianach i podłodze ustawione poprawnie, oba widoki diagnostyczne, porównanie filtrów (najbliższy sąsiad, dwuliniowy, trójliniowy, anizotropia 16x na ścianie oglądanej pod płaskim kątem), podglądy tekstur w panelu nieodwrócone oraz biała tekstura zastępcza z jedną linią `[error]` po usunięciu pliku tekstury. Stany filtra, anizotropii i trybu widoku były ustawiane tymczasowym kodem, który został usunięty: **widżetów panelu nikt jeszcze nie kliknął ręcznie**. Klasa nie ma testu jednostkowego, bo każda jej funkcja tworzy obiekty OpenGL. Na macOS kod nie był budowany ani uruchamiany.

**Po M8, części 2 (2026-10-06).** Pamięć podręczna wczytuje osiem modeli i czternaście tekstur: doszły `lever.obj`, `lever_handle.obj` i `note.obj` oraz tekstury `lever_iron`, `lever_brass` i `note_paper` z mapami normalnych. Kod `AssetCache` się nie zmienił. Trzecim użytkownikiem obok `MazeRenderer` i `GameplayRenderer` jest `game::InteractableRenderer` (konstruktor prosi o trzy modele przez `AssetCache::model`). Zdania o pięciu modelach i ośmiu teksturach w tym dokumencie opisują stan sprzed tej części, w którym wykonano opisane pomiary.

Po dodaniu map normalnych (2026-10-05, ten sam komputer) build Debug i Release nadal przechodzi bez ostrzeżeń i program startuje bez linii `[error]` i `GL_`. Na zrzutach ekranu sprawdzony jest relief na ścianach, słupkach i podłodze przy mapach włączonych i jego brak przy wyłączonych. **Nie jest sprawdzone ręcznie:** kliknięcie pola `Normal mapping`, ostateczny układ panelu z tym polem na swoim miejscu, lista czterech tekstur z podglądami odczytana z ekranu oraz zachowanie przy brakującym pliku mapy normalnych (płaska mapa zastępcza i jedna linia `[error]`). Te punkty są otwarte na liście w [`../../guides/build-windows.md`](../../guides/build-windows.md).

W M5 (2026-10-05) doszły trzy modele rozgrywki (`crystal_a.obj`, `crystal_b.obj`, `gate.obj`) i ich cztery tekstury. Kod `AssetCache` i kod panelu nie zmieniły się: nowe pliki wczytuje ta sama funkcja `model()`, o którą prosi teraz także `game::GameplayRenderer`. Autor kodu zgłosił dla Windowsa build Debug i Release bez ostrzeżeń, przechodzące testy i obraz sprawdzony zrzutami ekranu. **Nie jest sprawdzone ręcznie** nic z M5, w tym lista sześciu modeli i ośmiu tekstur odczytana z panelu: liczby, które ten dokument dla nich podaje, wynikają z kodu panelu i ze skryptu liczącego wierzchołki z plików `.obj` ([`obj-loader.md`](obj-loader.md), sekcja 5.9).

W drugiej części M6 (2026-10-05) zniknął model `floor_tile.obj` z teksturami `floor_stone.png` i `floor_stone_normal.png`: podłoże jest terenem z mapy wysokości ([`../renderer/terrain.md`](../renderer/terrain.md)). Doszedł trzeci użytkownik pamięci podręcznej, `game::TerrainRenderer`, i jest pierwszym, który nie prosi o model: teren nie ma pliku OBJ, więc renderer woła wprost `texture()` dla `ground.png` i `ground_normal.png`, a przy błędzie sięga po `whiteTexture()` i `flatNormalTexture()`. Kod `AssetCache` i kod panelu znowu się nie zmieniły. Liczby po tej zmianie (pięć modeli, osiem tekstur, kolejność list) wynikają z kodu konstruktorów: panelu Assets w tym stanie nikt jeszcze nie odczytał z ekranu. Mapy wysokości `heightmap.png` pamięć podręczna nie wczytuje i panel jej nie pokazuje (sekcja 5.6 w [`images.md`](images.md)).

W pierwszej części M7 (2026-10-05, bufor HDR i gamma) kod klasy zmienił się po raz pierwszy od M4. Tekstury koloru są teraz teksturami sRGB, a mapy normalnych zostają liniowe ([`../gfx/color-space.md`](../gfx/color-space.md), [`../gfx/textures.md`](../gfx/textures.md), sekcja 2.10), więc pamięć podręczna musi wiedzieć, czym jest plik, o który ktoś prosi:

- `texture(path, colorSpace)` ma drugi argument bez wartości domyślnej. Decyduje wołający, bo tylko on wie, do czego użyje tekstury (sekcje 2.5 i 5.6).
- `model()` podaje `gfx::ColorSpace::Srgb` dla pliku z `map_Kd` i `gfx::ColorSpace::Linear` dla pliku z `map_Bump` (sekcja 5.5). `TerrainRenderer` robi to samo dla pary `ground.png` i `ground_normal.png`.
- Biała tekstura zastępcza jest `Srgb`, płaska mapa normalnych `Linear` (sekcja 5.4).
- Prośba o plik, który jest już wczytany w drugiej przestrzeni, wypisuje błąd `Texture is asked for as sRGB and as linear: <plik>` i oddaje teksturę taką, jaka powstała za pierwszym razem (sekcja 5.6).
- Panel Assets pokazuje przestrzeń przy rozmiarze i rysuje miniatury tekstur sRGB przez `debug::RawTextureSampler` (sekcja 6.4).

Kolor `Kd` materiału (`uTint`) **nie jest** przeliczany z sRGB: trafia do shadera jako zwykły mnożnik liniowego koloru tekstury. Wszystkie materiały gry mają biały `Kd`, a biel to 1 w obu przestrzeniach, więc dziś niczego to nie zmienia (komentarz `drawModel` w `src/game/ModelDraw.hpp` mówi to wprost). Model z kolorowym `Kd` wyszedłby ciemniejszy, niż wskazuje liczba w pliku MTL: to otwarty punkt. Zgłoszone dla Windowsa po tej zmianie (nie powtarzałem): bramka `make check` przechodzi, 269 przypadków testowych i 102103 asercje w Debug i Release, a po drugiej części M7 276 i 102139, miniatury w panelu identyczne co do piksela z miniaturami sprzed zmiany. Nowych widżetów nikt nie klikał, na macOS nic z tego nie było budowane.

## 2. Teoria

### 2.1 Dlaczego wczytywać raz

Słupek i ściana używają tej samej tekstury `wall_stone.png`. Bez pamięci podręcznej każdy model wczytałby ją osobno:

| | Bez pamięci podręcznej | Z pamięcią podręczną |
|---|---|---|
| odczyt i dekodowanie pliku PNG | 2 razy | 1 raz |
| tekstury 512 x 512 z mipmapami na karcie | 2 identyczne | 1 |
| zmiana filtra | trzeba pamiętać o obu | jedna lista |

To samo dotyczy mapy normalnych `wall_stone_normal.png`: materiał ściany i materiał słupka wskazują ten sam plik. To samo robią dwa kryształy: `crystal_a.obj` i `crystal_b.obj` mają materiał `crystal` z tymi samymi dwoma plikami. Przy ośmiu plikach to drobiazg. Zasada jest jednak ogólna i w większej grze decyduje o czasie wczytywania i o zajętości pamięci karty: **zasób identyfikuje plik, a nie ten, kto o niego prosi**. Ten sam schemat ma PRD w wierszu tematu 4 ("cache meshy").

Druga korzyść jest mniej oczywista: pamięć podręczna jest **jedynym właścicielem**. Renderer, panel i każdy inny kod dostają wskaźnik do odczytu i nie muszą się zastanawiać, kto i kiedy ma zasób zwolnić.

### 2.2 Klucz: znormalizowana ścieżka

Żeby rozpoznać "ten sam plik", trzeba mieć klucz. Tutaj kluczem jest ścieżka, sprowadzona do jednej postaci. Powód: ten sam plik da się zapisać na wiele sposobów.

```text
assets/models/../textures/wall_stone.png      tak wychodzi ze złożenia katalogu MTL i wpisu map_Kd
assets/textures/wall_stone.png                ta sama ścieżka po normalizacji
```

Normalizacja (`lexically_normal`) usuwa kroki `.` i `..`. Słowo "leksykalnie" jest ważne: funkcja pracuje na **tekście** ścieżki i nie pyta systemu plików. Skutki:

- działa także dla pliku, który nie istnieje (ważne, bo nieudane ścieżki też są zapamiętywane),
- nie rozpoznaje dowiązań symbolicznych ani różnicy wielkości liter. Na Windowsie `Textures/A.png` i `textures/a.png` to ten sam plik, ale dwa różne klucze,
- ścieżka względna i bezwzględna do tego samego pliku to dwa różne klucze.

W grze wszystkie ścieżki powstają w jeden sposób (z `core::assetPath` i z wpisów w plikach MTL), więc te ograniczenia nie mają dziś znaczenia. Trzeba o nich wiedzieć.

### 2.3 Stabilne wskaźniki: dlaczego `std::deque`

Pamięć podręczna oddaje wskaźnik do elementu swojego kontenera. Renderer zapamiętuje go na cały czas działania programu. Co się stanie, gdy do kontenera dojdzie następny element?

**`std::vector`** trzyma elementy w jednym ciągłym bloku pamięci. Gdy blok się zapełni, wektor przydziela większy, **przenosi** tam wszystkie elementy i zwalnia stary. Każdy wskaźnik do elementu pokazuje od tej chwili na zwolnioną pamięć.

```text
std::vector, pojemność 2:                 po dodaniu trzeciego elementu:

 blok A: [model 0][model 1]                blok A: zwolniony        <- wskaźnik renderera
            ^                              blok B: [model 0][model 1][model 2][   ]
            wskaźnik renderera
```

**`std::deque`** trzyma elementy w wielu kawałkach stałej wielkości. Dodanie elementu na końcu dokłada go do ostatniego kawałka albo przydziela nowy kawałek. Istniejące elementy **zostają na swoim miejscu**.

```text
std::deque:                                po dodaniu trzeciego elementu:

 kawałek 1: [model 0][model 1]             kawałek 1: [model 0][model 1]   <- bez zmian
               ^                           kawałek 2: [model 2][        ]
               wskaźnik renderera
```

Gwarancja ze standardu: wstawienie na początku albo na końcu `std::deque` unieważnia iteratory, ale **nie unieważnia wskaźników ani referencji** do elementów. Pamięć podręczna tylko dopisuje na końcu i nigdy nie usuwa, więc wskaźnik oddany raz jest ważny do jej zniszczenia.

Inne możliwości i dlaczego nie one:

| Rozwiązanie | Stabilne wskaźniki | Uwagi |
|---|---|---|
| `std::vector<LoadedModel>` | nie | błąd opisany wyżej, ujawnia się dopiero przy przekroczeniu pojemności |
| `std::vector` z `reserve(N)` | tylko do N elementów | działa do dnia, w którym ktoś wczyta N + 1 zasobów |
| `std::vector<std::unique_ptr<LoadedModel>>` | tak | każdy element to osobny przydział pamięci i dodatkowy wskaźnik w kodzie |
| `std::list<LoadedModel>` | tak | osobny przydział na element, wolniejsze przeglądanie |
| `std::map` albo `std::unordered_map` | tak (elementy węzłowe) | potrzebny dopiero przy tysiącach zasobów. Tutaj jest ich dziś dwadzieścia dwa: osiem modeli i czternaście tekstur (do M8, części 2 trzynaście: pięć i osiem) |
| `std::deque<LoadedModel>` | tak, przy dopisywaniu na końcu | wybrane: najprostszy kod, kolejność wczytania zachowana dla panelu |

Wyszukiwanie to zwykłe przejście po liście i porównanie ścieżek. Przy kilku zasobach jest szybsze niż jakakolwiek tablica mieszająca i nie wymaga drugiej struktury danych.

### 2.4 Model na karcie: siatka i części

Loader oddaje jeden wspólny wektor wierzchołków i indeksów oraz listę **części**: zakresów indeksów, z których każdy ma inny materiał ([`obj-loader.md`](obj-loader.md), sekcja 2). Pamięć podręczna robi z tego:

- jedną siatkę `gfx::Mesh` z całością,
- dla każdej części rekord z tym, co potrzebne do narysowania: zakres indeksów, kolor rozproszenia materiału (linia `Kd` pliku MTL), wskaźnik do tekstury (linia `map_Kd`) i wskaźnik do mapy normalnych (linia `map_Bump`).

Kolor, tekstura i mapa normalnych są wyszukiwane **raz, przy wczytaniu**. W pętli rysowania nie ma już szukania materiału po nazwie: są gotowe wskaźniki i gotowy kolor.

Pięć modeli gry ma po jednej części, każdą z kolorem białym, własną teksturą (kamień ściany, kryształ albo drewno bramy) i mapą normalnych tej tekstury. Czwarta para tekstur, podłoże terenu, nie należy do żadnego modelu.

**Mapa normalnych jest dla pamięci podręcznej prawie zwykłą teksturą.** Ten sam loader obrazów, ta sama funkcja `texture()`, ta sama lista, ten sam filtr. Do M6 pamięć podręczna w ogóle nie wiedziała, że bajty tego obrazu są kierunkami, a nie kolorami. Od pierwszej części M7 wie jedną rzecz, i to nie sama z siebie, tylko od wołającego: przestrzeń kolorów. Obraz koloru jest wczytywany jako `gfx::ColorSpace::Srgb`, mapa normalnych jako `gfx::ColorSpace::Linear` (komentarz klasy w nagłówku mówi to wprost). Druga różnica po jej stronie to, którą teksturę zastępczą dostaje część, gdy pliku nie ma.

### 2.5 Błędy i dwie tekstury zastępcze

Co może pójść źle i co wtedy robi pamięć podręczna:

| Sytuacja | Co wraca | Co widać |
|---|---|---|
| nie ma pliku OBJ albo ma błędną linię | `nullptr` | model nie jest rysowany, reszta sceny tak |
| plik OBJ nie ma ani jednej ściany | `nullptr` | to samo. Taki plik jest poprawny składniowo, ale nie ma czego rysować |
| model jest dobry, ale brakuje pliku jego tekstury | model, część z białą teksturą | powierzchnia w gładkim kolorze materiału |
| materiał nie ma linii `map_Kd` | model, część z białą teksturą | to samo: tak ma być, materiał jest jednokolorowy |
| obraz ma 1 albo 2 kanały (odcienie szarości) | `nullptr` dla tekstury, część z białą teksturą | gładki kolor |
| model jest dobry, ale brakuje pliku jego mapy normalnych | model, część z płaską mapą normalnych | powierzchnia z teksturą, ale bez reliefu: oświetlona normalnymi siatki |
| materiał nie ma linii `map_Bump` | model, część z płaską mapą normalnych | to samo: tak ma być, materiał nie ma reliefu |
| ten sam plik obrazu poproszony raz jako `Srgb`, a raz jako `Linear` (od M7) | tekstura wczytana za pierwszym razem, w pierwszej przestrzeni | jedna linia `[error]` przy każdej takiej prośbie. Drugi użytkownik dostaje teksturę w złej przestrzeni: wyblakły kolor albo przekrzywione normalne |

Dlaczego **biała** tekstura, a nie osobny shader "bez tekstury". Shader fragmentów liczy `tekstura * uTint`. Biały teksel to `(1, 1, 1)`, a mnożenie przez jeden niczego nie zmienia: wychodzi sam kolor materiału. Jedna tekstura 1 x 1 pozwala więc rysować części z teksturą i bez niej **tym samym shaderem i tą samą pętlą**, bez instrukcji warunkowej w GLSL i bez drugiego programu.

**Płaska mapa normalnych** (flat normal map) to ten sam pomysł dla drugiej tekstury części. Shader z włączonymi mapami normalnych zawsze czyta teksel mapy i robi z niego kierunek: `bajt / 255 * 2 - 1` dla każdego kanału ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcje 2.4 i 4.1). Tekstura zastępcza ma jeden teksel o bajtach `(128, 128, 255)`:

| Kanał | Bajt | Po przeliczeniu w shaderze | Znaczenie |
|---|---|---|---|
| czerwony | 128 | `128 / 255 * 2 - 1`, czyli około 0,004 | prawie zero wzdłuż stycznej |
| zielony | 128 | około 0,004 | prawie zero wzdłuż drugiego kierunku na powierzchni |
| niebieski | 255 | dokładnie 1 | cały kierunek "prosto z powierzchni" |

To kierunek `(0, 0, 1)` przestrzeni stycznej, czyli po prostu normalna siatki. Część z płaską mapą jest więc cieniowana tak, jakby mapy normalnych nie było, i shader **nie musi pytać**, czy część ma własną mapę. Dlaczego 128, a nie 127: zero ma w tym kodowaniu wartość `0,5 * 255 = 127,5`, która nie jest liczbą całkowitą. Zaokrąglona daje 128. Stąd drobna niedokładność z tabeli: kierunek jest odchylony od normalnej siatki o ułamek stopnia, czego nie widać. Skrypt tekstur zaokrągla tak samo, więc płaskie teksele prawdziwych map też mają 128.

Dlaczego dwie tekstury zastępcze, a nie jedna: biała `(255, 255, 255)` użyta jako mapa normalnych oznaczałaby kierunek `(1, 1, 1)`, czyli normalną przechyloną o około 55 stopni w stronę rogu. Każda z dwóch tekstur jest "elementem neutralnym" innego działania: biała dla mnożenia kolorów, płaska dla zamiany normalnej.

Każda nieudana ścieżka jest **zapamiętywana**. Powody:

- błąd trafia do logu raz, a nie przy każdej prośbie,
- dysk nie jest odpytywany ponownie o plik, o którym wiadomo, że go nie ma,
- panel może pokazać listę nieudanych wczytań.

Cena: plik naprawiony w trakcie działania programu nie zostanie wczytany, dopóki program nie wystartuje od nowa. Przeładowania assetów na żywo (jak shaderów) pamięć podręczna nie ma.

### 2.6 Własność i kolejność niszczenia

Pamięć podręczna posiada obiekty OpenGL, więc obowiązują ją te same reguły co klasy `gfx` ([`../gfx/README.md`](../gfx/README.md), sekcja 2): potrzebuje kontekstu OpenGL przez całe życie i musi zostać zniszczona przed oknem.

W środku są wskaźniki **między jej własnymi polami**: części modeli pokazują na tekstury z listy tekstur albo na jedną z dwóch tekstur zastępczych (każda część ma dwa takie wskaźniki: `texture` i `normalMap`). Stąd dwie reguły:

1. **Kolejność pól.** Pola klasy są niszczone w kolejności odwrotnej do deklaracji. Obie tekstury zastępcze i lista tekstur są zadeklarowane przed listą modeli, więc giną po niej: żadna część nie pokazuje nigdy na usuniętą teksturę.
2. **Zakaz kopiowania i przenoszenia.** Kopia miałaby części pokazujące na tekstury oryginału. Przeniesiona pamięć podręczna miałaby części pokazujące na tekstury zastępcze starego obiektu, bo te są polami trzymanymi przez wartość i zmieniają adres. Klasa jest więc tworzona raz, w miejscu, i tam zostaje.

### 2.7 Jedno ustawienie filtra dla wszystkich tekstur

Każda `gfx::Texture2D` ma własny obiekt samplera z filtrem i poziomem anizotropii ([`../gfx/textures.md`](../gfx/textures.md), sekcja 2). Do pokazu wygodniej jest przełączać wszystkie naraz: pamięć podręczna pamięta wybrany filtr i poziom, ustawia je na każdej wczytanej teksturze i nadaje każdej teksturze wczytanej później. Dzięki temu tekstura wczytana po zmianie filtra nie wygląda inaczej niż pozostałe.

Mapy normalnych są na tej samej liście, więc lista `Filter` i suwak `Anisotropy` działają także na nie. To ma sens: mapa normalnych oglądana z daleka albo pod płaskim kątem ma te same kłopoty z pomniejszeniem co obraz koloru. Przy filtrze `Nearest` relief w oddali ziarni się i iskrzy, przy `Trilinear` mipmapy uśredniają sąsiednie kierunki i relief łagodnie zanika z odległością ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 7). Iskrzenie w ruchu oceniłem tylko na nieruchomych zrzutach ekranu, więc to opis wynikający z teorii, a nie obserwacja.

Przypomnienie, co oznaczają trzy filtry:

| Filtr | Z bliska (powiększenie) | Z daleka (pomniejszenie) |
|---|---|---|
| `Nearest` | ostre kwadraty tekseli | migotanie i ziarno |
| `Bilinear` | gładkie przejścia | nadal migotanie, bo mipmapy nie są używane |
| `Trilinear` | gładkie przejścia | spokojny obraz, ale rozmyty pod płaskim kątem |

Anizotropia poprawia właśnie ostatni przypadek: powierzchnię oglądaną pod płaskim kątem, na przykład podłoże i ścianę biegnącą w głąb korytarza.

## 3. Jak to działa w OpenGL

`AssetCache.cpp` nie woła bezpośrednio żadnej funkcji `gl*`. Tworzy obiekty klas `gfx`, a one wołają OpenGL. Co się dzieje na karcie przy starcie gry:

| Kiedy | Kod | Co powstaje na karcie |
|---|---|---|
| konstruktor `AssetCache` | `m_whiteTexture(1, 1, 3, ..., Srgb)`, `m_flatNormalTexture(1, 1, 3, ..., Linear)` | dwie tekstury 1 x 1, biała `GL_SRGB8` i płaska mapa normalnych `GL_RGB8`, każda z jednym poziomem mipmap i własnym obiektem samplera |
| `model(wall_straight.obj)` | `texture(wall_stone.png, Srgb)`, `texture(wall_stone_normal.png, Linear)`, potem `gfx::Mesh(...)` | dwie tekstury 512 x 512, obraz koloru `GL_SRGB8` i mapa normalnych `GL_RGB8`, każda z dziesięcioma poziomami mipmap i samplerem, potem VAO, bufor wierzchołków i bufor indeksów |
| `model(wall_pillar.obj)` | oba wywołania `texture(...)` oddają tekstury już wczytane, potem `gfx::Mesh(...)` | tylko druga siatka |
| `model(crystal_a.obj)` | `texture(crystal.png)`, `texture(crystal_normal.png)`, potem `gfx::Mesh(...)` | następne dwie tekstury 512 x 512 i trzecia siatka |
| `model(crystal_b.obj)` | oba wywołania `texture(...)` oddają tekstury już wczytane, potem `gfx::Mesh(...)` | tylko czwarta siatka |
| `model(gate.obj)` | `texture(gate_wood.png)`, `texture(gate_wood_normal.png)`, potem `gfx::Mesh(...)` | następne dwie tekstury 512 x 512 i piąta siatka |
| `texture(ground.png, Srgb)`, `texture(ground_normal.png, Linear)` | dwa wywołania wprost, bez modelu | ostatnie dwie tekstury 512 x 512. Siatki nie ma: siatkę terenu tworzy i trzyma sam `TerrainRenderer` |

Dwie pierwsze prośby pochodzą z konstruktora `MazeRenderer`, trzy następne z konstruktora `GameplayRenderer`, dwie ostatnie z konstruktora `TerrainRenderer`. Kolejność wynika z kolejności pól w `NightMazeApp` (`m_mazeRenderer`, `m_gameplayRenderer`, `m_terrainRenderer`) i z kolejności wpisów na listach inicjalizacyjnych tych konstruktorów. Do M6 pierwszą prośbą była płytka podłogi `floor_tile.obj` z teksturami `floor_stone`.

Razem w pamięci podręcznej: 10 tekstur (osiem z plików, biała i płaska mapa normalnych), 10 obiektów samplera, 5 VAO i 10 buforów (bufor wierzchołków i bufor indeksów na siatkę). Siatki terenu, trawy, nieba i linii kolizji do niej nie należą. Pięć z nich to tekstury sRGB (cztery obrazy koloru i biała), pięć liniowe (cztery mapy normalnych i płaska). Mapa normalnych jest przechowywana w formacie `GL_RGB8`: liniowo, bez żadnego przeliczania, czego kierunki wymagają. Obraz koloru ma od M7 format `GL_SRGB8`: te same bajty, ale karta dekoduje je do wartości liniowych, gdy shader je czyta. W pozostałych wierszach tabeli zapis `texture(...)` pomija drugi argument: jest nim zawsze `Srgb` dla pliku koloru i `Linear` dla pliku z końcówką `_normal`. Dane po stronie procesora (wektory z loaderów) są zwalniane zaraz po wysłaniu.

Zmiana filtra i anizotropii to `glSamplerParameteri` i `glSamplerParameterf` na obiekcie samplera każdej tekstury z listy ([`../gfx/textures.md`](../gfx/textures.md), sekcja 5). Tekstur nie trzeba do tego podpinać ani wysyłać ponownie.

Jeden skutek uboczny wart zapamiętania: utworzenie siatki podpina jej VAO i bufory, a utworzenie tekstury podpina ją do aktywnej jednostki. Wczytanie assetu **zmienia więc stan OpenGL**. Dlatego gra prosi o modele tylko w konstruktorach obu rendererów, zanim cokolwiek zostanie narysowane, a nie w środku klatki (pułapka 9). W M1 dochodził do tego jeszcze jeden wzgląd, kolejność pól względem buforów kostki, ale kostki już nie ma.

## 4. Shadery

Pamięć podręczna nie ma własnego shadera. To, co przygotowuje, trafia do pary `textured.vert` i `textured.frag` ([`../gfx/textures.md`](../gfx/textures.md), sekcja 4):

| Pole `ModelPart` | Uniform | Linia w `textured.frag` |
|---|---|---|
| `texture` (podpinana do jednostki 0) | `uTexture` | `vec3 texel = texture(uTexture, vUv).rgb;` |
| `color` | `uTint` | `fragColor = vec4(texel * uTint * (vec3(1.0) + uEmissive), 1.0);` |

Z tych dwóch linii wynika cały mechanizm białej tekstury zastępczej: gdy `texel` to `(1, 1, 1)`, kolorem fragmentu jest samo `uTint`. Czynnik z `uEmissive` doszedł w M5 i nie pochodzi z pamięci podręcznej: to własne świecenie powierzchni, które ustawia renderer. Dla labiryntu i bramy jest czarne, więc nawias jest jedynką i wynik się nie zmienia. Niezerowe jest tylko dla kryształów ([`../game/gameplay.md`](../game/gameplay.md), sekcja 4).

Trzecie pole, mapa normalnych, trafia do pliku dołączanego `common/normal_map.glsl`, wspólnego dla `lit.frag` i `textured.frag` ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 4.1):

| Pole `ModelPart` | Uniform | Linia w `common/normal_map.glsl` |
|---|---|---|
| `normalMap` (podpinana do jednostki 1) | `uNormalMap` | `vec3 mapped = texture(uNormalMap, uv).rgb * 2.0 - 1.0;` |

Z tej linii wynika mechanizm płaskiej mapy zastępczej: teksel `(128, 128, 255)` daje `mapped` bliskie `(0, 0, 1)`, a to po przejściu przez macierz styczna, bitangenta, normalna jest samą normalną siatki. Linia wykonuje się tylko wtedy, gdy uniform `uNormalMapEnabled` jest prawdą. Program `gouraud` nie ma żadnego z tych dwóch uniformów: funkcje `game::drawModel` i `game::setModelSamplers` i tak podpinają mapę do jednostki 1 i wysyłają numer jednostki, a wywołanie dla nieistniejącego uniformu jest po cichu pomijane.

Tryb widoku z panelu Assets ustawia uniform `uViewMode`: 0 to tekstura razy kolor, 1 to normalna jako kolor (przy działających mapach normalnych: normalna z mapy), 2 to współrzędne tekstury jako kolor. Pole `Normal mapping` z tego samego panelu trafia do shaderów jako `uNormalMapEnabled`, po przejściu przez `game::usesNormalMap` ([`../game/flashlight.md`](../game/flashlight.md)).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/assets/AssetCache.hpp`](../../../src/assets/AssetCache.hpp) | struktury `LoadedTexture`, `ModelPart`, `LoadedModel` i klasa `AssetCache` |
| [`src/assets/AssetCache.cpp`](../../../src/assets/AssetCache.cpp) | stałe obu tekstur zastępczych, funkcje pomocnicze `cacheKey` i `findMaterial`, definicje funkcji klasy |
| [`src/debug/panels/AssetsPanel.hpp`](../../../src/debug/panels/AssetsPanel.hpp), [`.cpp`](../../../src/debug/panels/AssetsPanel.cpp) | funkcja `debug::drawAssetsPanel` (sekcja 6) |
| [`src/game/MazeRenderer.cpp`](../../../src/game/MazeRenderer.cpp) | prosi o dwa modele labiryntu: `models/wall_straight.obj`, `models/wall_pillar.obj` ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5). Do M6 prosił też o `models/floor_tile.obj` |
| [`src/game/GameplayRenderer.cpp`](../../../src/game/GameplayRenderer.cpp) | prosi o trzy modele rozgrywki: `models/crystal_a.obj`, `models/crystal_b.obj`, `models/gate.obj` ([`../game/gameplay.md`](../game/gameplay.md), sekcja 5). Poza tymi dwoma konstruktorami nikt o modele nie prosi |
| [`src/game/InteractableRenderer.cpp`](../../../src/game/InteractableRenderer.cpp) | od M8, części 2: prosi o trzy modele: `models/lever.obj`, `models/lever_handle.obj` i `models/note.obj` (model, którego nie udało się wczytać, jest po prostu nierysowany) ([`../game/interactables.md`](../game/interactables.md)) |
| [`src/game/TerrainRenderer.cpp`](../../../src/game/TerrainRenderer.cpp) | od M6: prosi wprost o dwie tekstury, `textures/ground.png` i `textures/ground_normal.png`, każdą ze swoją przestrzenią kolorów (`Srgb` i `Linear`), a gdy `texture()` odda `nullptr`, bierze `whiteTexture()` albo `flatNormalTexture()` (funkcja pomocnicza `textureOr`). Opis w [`../renderer/terrain.md`](../renderer/terrain.md) |
| [`src/game/ModelDraw.hpp`](../../../src/game/ModelDraw.hpp), [`.cpp`](../../../src/game/ModelDraw.cpp) | `game::drawModel` i `game::setModelSamplers`: jedyny kod, który czyta `LoadedModel::parts` przy rysowaniu. Od M6 jest tam też `game::drawMesh` dla siatki spoza pamięci podręcznej (teren), z teksturami podanymi wprost. Do M4 była to funkcja `drawInstances` klasy `MazeRenderer` |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) | właściciel: pole `m_assets`, akcesor `assets()` |

`AssetCache.*` należą do biblioteki `engine`. To pierwszy plik katalogu `src/assets/`, który dołącza nagłówki `gfx/` i tworzy obiekty OpenGL: loadery `ObjLoader` i `ImageLoader` nadal zwracają same dane procesora i nadal mają testy bez okna ([`README.md`](README.md)).

### 5.2 Trzy struktury danych

```cpp
struct LoadedTexture {
    /// Path of the image file, normalized. This is the key of the cache.
    std::filesystem::path path;

    /// The picture on the graphics card. It knows its colour space
    /// (gfx::Texture2D::colorSpace).
    gfx::Texture2D texture;
};
```

Tekstura razem ze swoim kluczem. Przestrzeni kolorów struktura nie przechowuje osobno: pamięta ją sama `Texture2D` (`colorSpace()`), a pamięć podręczna i panel ją stamtąd czytają. `gfx::Texture2D` nie zna pliku, z którego powstała (przyjmuje gołe piksele), więc ścieżkę trzeba trzymać obok.

```cpp
struct ModelPart {
    /// Name of the material (the usemtl line of the OBJ file). Empty when the faces had
    /// no material.
    std::string material;

    /// The run of indices: gfx::Mesh::draw(firstIndex, indexCount) draws this part.
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;

    /// Diffuse colour of the material (the Kd line). The shader multiplies the texture by
    /// it. White (the default) leaves the texture unchanged.
    glm::vec3 color{1.0F};

    /// Texture to bind for this part. Never null: when the material names no texture, or
    /// the file could not be loaded, it points at the white texture of the cache, and the
    /// part is drawn in its plain colour.
    const gfx::Texture2D* texture = nullptr;

    /// Path of the texture file from the map_Kd line. Empty when the material has none.
    std::filesystem::path texturePath;

    /// False when texture is the white fallback and not the picture of texturePath.
    bool hasOwnTexture = false;

    /// Normal map to bind for this part. Never null: when the material names no normal
    /// map, or the file could not be loaded, it points at the flat normal map of the
    /// cache, and the part is shaded with the normals of its mesh.
    const gfx::Texture2D* normalMap = nullptr;

    /// Path of the normal map from the map_Bump line. Empty when the material has none.
    std::filesystem::path normalMapPath;

    /// False when normalMap is the flat fallback and not the picture of normalMapPath.
    bool hasOwnNormalMap = false;
};
```

| Pole | Skąd pochodzi | Kto z niego korzysta |
|---|---|---|
| `material` | `ObjPart::material` (linia `usemtl`) | panel Assets |
| `firstIndex`, `indexCount` | `ObjPart` | `Mesh::draw(firstIndex, indexCount)` w rendererze |
| `color` | `ObjMaterial::diffuseColor` (linia `Kd`) | uniform `uTint` |
| `texture` | `AssetCache::texture(...)` albo biała tekstura | `bind` w rendererze |
| `texturePath` | `ObjMaterial::diffuseTexture` (linia `map_Kd`, już jako pełna ścieżka) | panel Assets |
| `hasOwnTexture` | ustawiane, gdy tekstura naprawdę się wczytała | panel Assets: odróżnia teksturę własną od zastępczej |
| `normalMap` | `AssetCache::texture(...)` albo płaska mapa normalnych | `bind` w rendererze, na jednostce 1 |
| `normalMapPath` | `ObjMaterial::normalTexture` (linia `map_Bump`, już jako pełna ścieżka) | panel Assets |
| `hasOwnNormalMap` | ustawiane, gdy mapa normalnych naprawdę się wczytała | panel Assets: odróżnia mapę własną od zastępczej |

Trzy nowe pola są lustrzanym odbiciem trzech pól tekstury: wskaźnik, ścieżka i znacznik. Wartości początkowe `texture = nullptr` i `normalMap = nullptr` w strukturze są tylko po to, żeby pola nie były niezainicjalizowane. Komentarz "Never null" opisuje części, które oddaje pamięć podręczna: `model()` ustawia oba wskaźniki na tekstury zastępcze, zanim zrobi cokolwiek innego. Renderer może więc pisać `part.texture->bind(...)` i `part.normalMap->bind(...)` bez sprawdzania.

```cpp
struct LoadedModel {
    /// Path of the OBJ file, normalized. This is the key of the cache.
    std::filesystem::path path;

    /// All vertices and indices of the model.
    gfx::Mesh mesh;

    /// The mesh split by material, in file order.
    std::vector<ModelPart> parts;

    /// Numbers for the debug panel. The mesh itself does not keep its vertex count.
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
};
```

Jedna siatka i lista części. `vertexCount` i `triangleCount` są zapisywane przy wczytaniu, bo po wysłaniu danych na kartę nikt już nie ma wektora wierzchołków, a `gfx::Mesh` pamięta tylko liczbę indeksów.

### 5.3 Klasa `AssetCache`: interfejs i pola

```cpp
class AssetCache {
public:
    /// Creates the two 1 x 1 textures that stand in for missing ones: the white texture
    /// and the flat normal map.
    AssetCache();

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;
```

Usunięcie konstruktora kopiującego i przypisania kopiującego wyłącza także przenoszenie: gdy klasa deklaruje operacje kopiujące (nawet jako usunięte), kompilator nie tworzy sam operacji przenoszących. Klasy nie da się więc ani skopiować, ani przenieść (sekcja 2.6).

Funkcje publiczne:

| Funkcja | Co robi |
|---|---|
| `const LoadedModel* model(const std::filesystem::path& path)` | model z pliku OBJ. Pierwsze wywołanie wczytuje, następne oddają ten sam obiekt. `nullptr` przy błędzie |
| `const gfx::Texture2D* texture(const std::filesystem::path& path, gfx::ColorSpace colorSpace)` | tekstura z pliku obrazu, na tych samych zasadach. `colorSpace` mówi, co jest w pliku: `Srgb` dla obrazu koloru, `Linear` dla mapy normalnych. Plik jest jednym albo drugim: prośba o wczytany już plik w drugiej przestrzeni wypisuje błąd i oddaje teksturę wczytaną za pierwszym razem |
| `const gfx::Texture2D& whiteTexture() const` | biała tekstura 1 x 1, sRGB jak obrazy koloru, które zastępuje (biel to 1 w obu przestrzeniach) |
| `const gfx::Texture2D& flatNormalTexture() const` | płaska mapa normalnych 1 x 1, teksel `(128, 128, 255)`, liniowa jak każda mapa normalnych |
| `void setFilter(gfx::TextureFilter filter)` | filtr wszystkich tekstur, wczytanych i przyszłych |
| `void setAnisotropy(float level)` | poziom anizotropii wszystkich tekstur, przycięty do zakresu od 1 do `maxAnisotropy()` |
| `filter()`, `anisotropy()`, `maxAnisotropy()` | bieżące ustawienia i granica sterownika |
| `models()`, `textures()`, `failedPaths()` | listy do odczytu dla panelu |

Wskaźniki i referencje, które oddaje, są `const`: wołający może rysować i czytać, ale nie zmieni ani siatki, ani tekstury. Filtr zmienia się tylko przez pamięć podręczną.

`model` i `texture` nie są funkcjami `const`, bo mogą dopisać element do listy. Stąd `MazeRenderer` i `GameplayRenderer` dostają w konstruktorze referencję bez `const`.

Pola:

```cpp
    // The stand-ins for a missing texture and a missing normal map. Declared first,
    // because the parts of the models below may point at them.
    gfx::Texture2D m_whiteTexture;
    gfx::Texture2D m_flatNormalTexture;

    // std::deque and not std::vector: adding an element at the end of a deque never moves
    // the elements that are already in it, so the pointers handed out stay valid. A vector
    // moves all of its elements to a new block of memory when it runs out of room.
    //
    // The textures are declared before the models, so they are destroyed after them
    // (members are destroyed bottom to top): no part ever points at a deleted texture.
    std::deque<LoadedTexture> m_textures;
    std::deque<LoadedModel> m_models;
    std::vector<std::filesystem::path> m_failedPaths;

    gfx::TextureFilter m_filter = gfx::TextureFilter::Trilinear;
    float m_anisotropy = 1.0F;
```

| Pole | Dlaczego w tym miejscu i tego typu |
|---|---|
| `m_whiteTexture`, `m_flatNormalTexture` | pierwsze: powstają przed wszystkim i giną po wszystkim, bo części modeli mogą na nie pokazywać |
| `m_textures` | `std::deque` dla stabilnych wskaźników (sekcja 2.3). Przed `m_models`, żeby ginęło po nich |
| `m_models` | `std::deque` z tego samego powodu: renderer trzyma wskaźniki do elementów |
| `m_failedPaths` | zwykły `std::vector`: nikt nie trzyma wskaźników do jego elementów, a panel czyta całą listę przez referencję |
| `m_filter`, `m_anisotropy` | wartości startowe takie same jak w `Texture2D`: trójliniowy, anizotropia wyłączona (poziom 1) |

`maxAnisotropy()` pyta białą teksturę: `return m_whiteTexture.maxAnisotropy();`. Granica zależy od sterownika, a nie od tekstury, a biała tekstura istnieje zawsze, także gdy żadna inna się nie wczytała.

Akcesor płaskiej mapy ma w nagłówku komentarz, który jest streszczeniem sekcji 2.5:

```cpp
    /// A 1 x 1 normal map whose one texel is (128, 128, 255): the direction (0, 0, 1) of
    /// tangent space, "straight out of the surface". A shader that reads its normals
    /// from a normal map gets the normal of the mesh with it, so it needs no second code
    /// path for parts without a normal map. Linear, like every normal map: decoded as
    /// sRGB, 128 would no longer mean 0.
    const gfx::Texture2D& flatNormalTexture() const { return m_flatNormalTexture; }
```

Części modeli dostają adres pola wprost w `model()` i akcesora nie potrzebują. Jest dla kodu, który rysuje coś własnego programem z mapami normalnych, tak jak `whiteTexture()` jest dla kodu rysującego gładki kolor. Do M6 nikt w grze żadnego z nich nie wołał. Dziś woła oba `game::TerrainRenderer`, jako wartości zastępcze dla tekstur podłoża:

```cpp
      // The picture of the earth is a colour (sRGB), its normal map is data (linear).
      m_texture(
          textureOr(assets, GROUND_TEXTURE_FILE, gfx::ColorSpace::Srgb, assets.whiteTexture())),
      m_normalMap(textureOr(assets, GROUND_NORMAL_MAP_FILE, gfx::ColorSpace::Linear,
                            assets.flatNormalTexture())) {}
```

`textureOr` woła `assets.texture(core::assetPath(file), colorSpace)` i oddaje wynik albo, gdy jest nim `nullptr`, adres tekstury zastępczej. Model robi to samo w środku `model()`. Teren modelu nie ma, więc robi to jego renderer.

### 5.4 Konstruktor i funkcje pomocnicze

```cpp
// The two stand-in textures are one pixel of 3 bytes (red, green, blue) each.
constexpr int FALLBACK_TEXTURE_SIZE = 1;
constexpr int FALLBACK_TEXTURE_CHANNELS = 3;

// The white one: all three bytes at the maximum.
constexpr unsigned char FULL_BRIGHTNESS = 255;
constexpr std::array<unsigned char, FALLBACK_TEXTURE_CHANNELS> WHITE_PIXEL = {
    FULL_BRIGHTNESS, FULL_BRIGHTNESS, FULL_BRIGHTNESS};

// The flat normal map: the direction (0, 0, 1) of tangent space, written the way
// a normal map stores a direction, byte = (component * 0.5 + 0.5) * 255. 0 becomes 127.5,
// which is rounded to 128, and 1 becomes 255.
constexpr unsigned char HALF_BRIGHTNESS = 128;
constexpr std::array<unsigned char, FALLBACK_TEXTURE_CHANNELS> FLAT_NORMAL_PIXEL = {
    HALF_BRIGHTNESS, HALF_BRIGHTNESS, FULL_BRIGHTNESS};
```

| Stała | Znaczenie |
|---|---|
| `FALLBACK_TEXTURE_SIZE`, `FALLBACK_TEXTURE_CHANNELS` | wspólne dla obu tekstur zastępczych: 1 x 1 teksel, 3 kanały (RGB). Dawniej nazywały się `WHITE_TEXTURE_...`, kiedy tekstura zastępcza była jedna |
| `FULL_BRIGHTNESS` | 255, największa wartość bajtu |
| `WHITE_PIXEL` | `(255, 255, 255)` |
| `HALF_BRIGHTNESS` | 128: tak zapisuje się składowa 0 kierunku (127,5 po zaokrągleniu) |
| `FLAT_NORMAL_PIXEL` | `(128, 128, 255)`: kierunek `(0, 0, 1)`. Trzeci bajt to `FULL_BRIGHTNESS`, bo składowa 1 zapisuje się jako 255 |

```cpp
AssetCache::AssetCache()
    : m_whiteTexture(FALLBACK_TEXTURE_SIZE, FALLBACK_TEXTURE_SIZE, FALLBACK_TEXTURE_CHANNELS,
                     WHITE_PIXEL.data(), gfx::ColorSpace::Srgb),
      m_flatNormalTexture(FALLBACK_TEXTURE_SIZE, FALLBACK_TEXTURE_SIZE, FALLBACK_TEXTURE_CHANNELS,
                          FLAT_NORMAL_PIXEL.data(), gfx::ColorSpace::Linear) {}
```

Dwie tekstury po jednym pikselu. `Texture2D` przyjmuje szerokość, wysokość, liczbę kanałów, wskaźnik na bajty i przestrzeń kolorów, więc tekstura nie musi pochodzić z pliku. Piąty argument doszedł w M7 i jest różny dla obu tekstur, z tego samego powodu co dla plików. Biała zastępuje obrazy koloru, więc jest `Srgb` jak one: wartość 255 dekoduje się na 1,0, czyli tyle samo, ile dałaby tekstura liniowa, więc dla bieli wybór nie zmienia wyniku, tylko trzyma porządek. Płaska mapa zastępuje mapy normalnych, więc musi być `Linear`: zdekodowany bajt 128 dałby 0,216 zamiast 0,502 i "płaska" normalna przestałaby być płaska. OpenGL kopiuje dane, a stałe `WHITE_PIXEL` i `FLAT_NORMAL_PIXEL` zostają w programie. Kolejność na liście inicjalizacyjnej jest taka sama jak kolejność deklaracji pól, bo to kolejność deklaracji decyduje, co powstaje pierwsze.

```cpp
std::filesystem::path cacheKey(const std::filesystem::path& path) {
    return path.lexically_normal();
}
```

Klucz z sekcji 2.2. Osobna funkcja, bo wołają ją obie funkcje wczytujące i ma nazwę, która mówi, po co normalizacja jest.

```cpp
const ObjMaterial* findMaterial(const ObjModel& model, const std::string& name) {
    for (const ObjMaterial& material : model.materials) {
        if (material.name == name) {
            return &material;
        }
    }
    return nullptr;
}
```

Szuka materiału po nazwie w modelu z loadera. Zwraca wskaźnik, bo wynik może nie istnieć: części bez linii `usemtl` mają pustą nazwę i żadnego materiału. Wskaźnik pokazuje na element wektora wewnątrz `source` i jest używany tylko wewnątrz `model()`, dopóki `source` żyje.

### 5.5 `model()` linia po linii

```cpp
const LoadedModel* AssetCache::model(const std::filesystem::path& path) {
    const std::filesystem::path key = cacheKey(path);

    // Loaded before: hand out the same object. A game has a handful of models, so
    // looking through all of them is fast enough and needs no second data structure.
    for (const LoadedModel& loaded : m_models) {
        if (loaded.path == key) {
            return &loaded;
        }
    }
    // Failed before: the error is already in the log, do not read the file again.
    if (hasFailed(key)) {
        return nullptr;
    }
```

| Linia | Znaczenie |
|---|---|
| `const std::filesystem::path key = cacheKey(path);` | od tej linii funkcja używa tylko klucza, także do otwarcia pliku |
| pętla po `m_models` | trafienie: adres elementu `std::deque`, ważny do końca życia pamięci podręcznej |
| `if (hasFailed(key))` | plik zawiódł wcześniej: bez ponownego czytania i bez drugiej linii w logu |

```cpp
    // loadObj logs its own error.
    ObjModel source;
    std::string error;
    if (!loadObj(key, source, error)) {
        m_failedPaths.push_back(key);
        return nullptr;
    }
    // A file without a single face is a valid OBJ file, but there is nothing to draw.
    if (source.indices.empty()) {
        core::logError("Model has no faces: " + core::pathText(key));
        m_failedPaths.push_back(key);
        return nullptr;
    }
```

| Linia | Znaczenie |
|---|---|
| `loadObj(key, source, error)` | czyta plik OBJ i pliki MTL, które wymienia ([`obj-loader.md`](obj-loader.md), sekcja 5). Przy błędzie sam pisze do logu. Zmienna `error` jest wymagana przez sygnaturę, a tutaj nie jest czytana |
| `m_failedPaths.push_back(key);` | zapamiętanie porażki |
| `source.indices.empty()` | loader uznaje plik bez ścian za poprawny. Siatka bez indeksów nie ma czego rysować, więc pamięć podręczna traktuje to jak błąd i sama pisze linię do logu. `core::pathText` zamienia ścieżkę na tekst UTF-8 |

```cpp
    // Each part of the file becomes a part of the model, with the colour, the texture and
    // the normal map of its material looked up now, once, instead of in every frame.
    std::vector<ModelPart> parts;
    for (const ObjPart& sourcePart : source.parts) {
        ModelPart part;
        part.material = sourcePart.material;
        part.firstIndex = sourcePart.firstIndex;
        part.indexCount = sourcePart.indexCount;
        part.texture = &m_whiteTexture;
        part.normalMap = &m_flatNormalTexture;

        // loadObj has checked that every named material exists. Faces without a material
        // (an empty name) keep the defaults: white colour, white texture, flat normal map.
        const ObjMaterial* material = findMaterial(source, sourcePart.material);
        if (material != nullptr) {
            part.color = material->diffuseColor;
            part.texturePath = material->diffuseTexture;
            part.normalMapPath = material->normalTexture;
        }
        if (!part.texturePath.empty()) {
            // texture() logs a failed load. The part then keeps the white texture and is
            // drawn in its plain colour. The picture of map_Kd is a colour: sRGB.
            const gfx::Texture2D* texture = this->texture(part.texturePath, gfx::ColorSpace::Srgb);
            if (texture != nullptr) {
                part.texture = texture;
                part.hasOwnTexture = true;
            }
        }
        if (!part.normalMapPath.empty()) {
            // The same function and the same list as for the colour pictures: a normal
            // map is a texture too. A failed load leaves the flat normal map in place.
            // Its bytes are directions, not colours: linear, or the normals would bend.
            const gfx::Texture2D* normalMap =
                this->texture(part.normalMapPath, gfx::ColorSpace::Linear);
            if (normalMap != nullptr) {
                part.normalMap = normalMap;
                part.hasOwnNormalMap = true;
            }
        }
        parts.push_back(std::move(part));
    }
```

| Linia | Znaczenie |
|---|---|
| `part.texture = &m_whiteTexture;` | najpierw wartość bezpieczna. Każda dalsza gałąź może ją tylko poprawić na lepszą |
| `part.normalMap = &m_flatNormalTexture;` | to samo dla mapy normalnych: od tej linii wskaźnik nigdy nie jest pusty |
| `findMaterial(source, sourcePart.material)` | materiał części. `nullptr` tylko dla części bez nazwy materiału: nazwy nieistniejące odrzucił już `loadObj` |
| `part.color = material->diffuseColor;` | kolor `Kd`, biały, gdy plik MTL go nie podał |
| `part.texturePath = material->diffuseTexture;` | ścieżka obrazu, już złożona przez loader z katalogu pliku MTL i wpisu `map_Kd` |
| `part.normalMapPath = material->normalTexture;` | ścieżka mapy normalnych, złożona tak samo z wpisu `map_Bump`. Pusta, gdy materiał takiej linii nie ma |
| `this->texture(part.texturePath, gfx::ColorSpace::Srgb)` | prośba do **tej samej** pamięci podręcznej. Drugi argument (od M7): obraz z linii `map_Kd` jest kolorem, więc sRGB. Druga część albo drugi model z tą samą teksturą dostanie ten sam wskaźnik. `this->` jest potrzebne, bo zmienna lokalna w tej samej linii też nazywa się `texture` i zasłania funkcję |
| `if (texture != nullptr)` | tekstura się wczytała: część dostaje ją i znacznik `hasOwnTexture`. W przeciwnym razie zostaje biała |
| `if (!part.normalMapPath.empty())` | materiał ma mapę normalnych. Gdy nie ma, część zostaje przy płaskiej i funkcja niczego nie próbuje wczytać, więc nie ma też linii w logu |
| `this->texture(part.normalMapPath, gfx::ColorSpace::Linear)` | **ta sama funkcja** co dla obrazu koloru, z drugą przestrzenią kolorów: bajty mapy są kierunkami, więc muszą dotrzeć do shadera bez dekodowania. Mapa normalnych trafia na tę samą listę `m_textures`, dostaje ten sam filtr i jest tak samo współdzielona: słupek dostaje wskaźnik do mapy wczytanej dla ściany. Tutaj `this->` nie jest konieczne (zmienna nazywa się `normalMap`), stoi dla symetrii z wywołaniem wyżej |
| `if (normalMap != nullptr)` | mapa się wczytała: część dostaje ją i znacznik `hasOwnNormalMap`. Przy błędzie `texture()` wypisała już linię `[error]` i zapamiętała ścieżkę, a część zostaje przy płaskiej mapie |
| `parts.push_back(std::move(part));` | przeniesienie, żeby nie kopiować napisu i ścieżek |

Dwa bloki `if` są niezależne: część może mieć teksturę bez mapy normalnych, mapę normalnych bez tekstury, obie albo żadnej. Brak mapy normalnych nie jest błędem modelu i nie zmienia wyniku `model()`.

```cpp
    const LoadedModel& loaded = m_models.emplace_back(LoadedModel{
        .path = key,
        .mesh = gfx::Mesh(source.vertices, source.indices),
        .parts = std::move(parts),
        .vertexCount = source.vertices.size(),
        .triangleCount = source.indices.size() / INDICES_PER_TRIANGLE,
    });
    core::logInfo("Loaded model: " + core::pathText(key));
    return &loaded;
}
```

| Linia | Znaczenie |
|---|---|
| `gfx::Mesh(source.vertices, source.indices)` | dopiero tutaj dane trafiają na kartę: VAO i dwa bufory. Wektory zamieniają się same na `std::span` |
| `LoadedModel{ .path = ..., ... }` | inicjalizatory desygnowane (C++20). Muszą iść w kolejności deklaracji pól struktury |
| `m_models.emplace_back(...)` | dopisuje element na końcu `std::deque` i zwraca do niego referencję. Obiekt tymczasowy jest przenoszony: `gfx::Mesh` da się przenosić, ale nie kopiować |
| `source.indices.size() / INDICES_PER_TRIANGLE` | liczba trójkątów: trzy indeksy na trójkąt |
| `core::logInfo("Loaded model: " + ...)` | jedna linia w logu na każdy wczytany model |
| `return &loaded;` | adres elementu w `std::deque`, stabilny |

Kolejność ma znaczenie: tekstury są wczytywane **przed** utworzeniem siatki, a model trafia na listę dopiero wtedy, gdy jest kompletny. Nie ma chwili, w której lista zawiera model w połowie zbudowany.

Po powrocie z funkcji `source` jest niszczone: wektory wierzchołków i indeksów po stronie procesora znikają. Zostaje kopia na karcie.

### 5.6 `texture()` linia po linii

```cpp
const gfx::Texture2D* AssetCache::texture(const std::filesystem::path& path,
                                          gfx::ColorSpace colorSpace) {
    const std::filesystem::path key = cacheKey(path);

    for (const LoadedTexture& loaded : m_textures) {
        if (loaded.path == key) {
            // One file cannot be a colour picture and a normal map at once. The texture
            // stays as it was loaded: the mistake is in the model or in the caller.
            if (loaded.texture.colorSpace() != colorSpace) {
                core::logError("Texture is asked for as sRGB and as linear: " +
                               core::pathText(key));
            }
            return &loaded.texture;
        }
    }
    if (hasFailed(key)) {
        return nullptr;
    }

    // loadImage logs its own error.
    Image image;
    std::string error;
    if (!loadImage(key, image, error)) {
        m_failedPaths.push_back(key);
        return nullptr;
    }
```

Ten sam początek co w `model()`: klucz, szukanie wśród wczytanych, sprawdzenie listy porażek, loader. Jedna rzecz jest od M7 inna niż w `model()`: trafienie w pamięć podręczną sprawdza przestrzeń kolorów.

| Linia | Znaczenie |
|---|---|
| `gfx::ColorSpace colorSpace` | drugi argument, bez wartości domyślnej. Kluczem pamięci podręcznej zostaje **sama ścieżka**: przestrzeń nie jest częścią klucza, więc jeden plik nigdy nie powstanie na karcie dwa razy |
| `loaded.texture.colorSpace() != colorSpace` | plik jest już wczytany, ale w innej przestrzeni niż ta, o którą ktoś teraz prosi |
| `core::logError("Texture is asked for as sRGB and as linear: " + ...)` | jedna linia `[error]` z nazwą pliku. Błąd jest w modelu (ten sam plik w `map_Kd` jednego materiału i w `map_Bump` innego) albo w kodzie wołającym, a nie w pamięci podręcznej |
| `return &loaded.texture;` po błędzie | funkcja oddaje teksturę taką, jaka powstała za pierwszym razem, a nie `nullptr`. Gra rysuje dalej, tyle że jeden z dwóch użytkowników dostaje złą przestrzeń. Komunikat pojawia się przy **każdej** takiej prośbie, nie raz: ścieżka nie trafia na listę porażek |

W grze ta gałąź nie jest wykonywana: żaden plik nie występuje w obu rolach. Nikt jej też nie sprawdził osobną próbą.

 `loadImage` dekoduje plik do pikseli z dolnym wierszem na początku ([`images.md`](images.md), sekcja 5).

```cpp
    // The constructor logs an error and leaves the texture not valid when the picture
    // has a channel count it does not accept (grey pictures have 1 or 2 channels).
    gfx::Texture2D texture(image.width, image.height, image.channels, image.pixels.data(),
                           colorSpace);
    if (!texture.isValid()) {
        core::logError("Texture cannot be used: " + core::pathText(key));
        m_failedPaths.push_back(key);
        return nullptr;
    }
    // A texture loaded later must look like the ones loaded before.
    texture.setFilter(m_filter);
    texture.setAnisotropy(m_anisotropy);

    const LoadedTexture& loaded =
        m_textures.emplace_back(LoadedTexture{.path = key, .texture = std::move(texture)});
    core::logInfo("Loaded texture: " + core::pathText(key));
    return &loaded.texture;
}
```

| Linia | Znaczenie |
|---|---|
| `gfx::Texture2D texture(..., colorSpace)` | tworzy teksturę i mipmapy na karcie, w formacie sRGB albo liniowym, zależnie od argumentu przekazanego dalej bez zmian ([`../gfx/textures.md`](../gfx/textures.md), sekcja 5.6). Obiekt jest na razie zmienną lokalną |
| `if (!texture.isValid())` | loader obrazów zachowuje liczbę kanałów z pliku (od 1 do 4), a tekstura przyjmuje tylko 3 albo 4. Obraz w odcieniach szarości dekoduje się poprawnie, ale tekstury z niego nie będzie. W logu są wtedy dwie linie: jedna z konstruktora tekstury (rozmiar i kanały) i ta, która podaje plik |
| `texture.setFilter(m_filter);`, `texture.setAnisotropy(m_anisotropy);` | bieżące ustawienie wspólne (sekcja 2.7) |
| `.texture = std::move(texture)` | przeniesienie do elementu listy: identyfikatory OpenGL przechodzą do nowego obiektu, a zmienna lokalna zostaje z zerami i jej destruktor niczego nie usuwa ([`../gfx/README.md`](../gfx/README.md), sekcja 2) |
| `return &loaded.texture;` | adres pola wewnątrz elementu `std::deque` |

### 5.7 `setFilter`, `setAnisotropy`, `hasFailed`

```cpp
void AssetCache::setFilter(gfx::TextureFilter filter) {
    m_filter = filter;
    for (LoadedTexture& loaded : m_textures) {
        loaded.texture.setFilter(filter);
    }
}

void AssetCache::setAnisotropy(float level) {
    // The same clamping as in Texture2D::setAnisotropy, so that anisotropy() reports the
    // level the textures really use. Without the extension the maximum is 1.
    m_anisotropy = std::clamp(level, NO_ANISOTROPY, maxAnisotropy());
    for (LoadedTexture& loaded : m_textures) {
        loaded.texture.setAnisotropy(m_anisotropy);
    }
}
```

| Linia | Znaczenie |
|---|---|
| `m_filter = filter;` | zapamiętanie dla tekstur wczytanych później |
| pętla po `m_textures` | ustawienie na każdej wczytanej, także na mapach normalnych. Dwie tekstury zastępcze nie są na liście i zostają przy ustawieniach startowych: mają po jednym tekselu, więc filtr niczego by w nich nie zmienił |
| `std::clamp(level, NO_ANISOTROPY, maxAnisotropy())` | przycięcie takie samo jak w teksturze, żeby `anisotropy()` zwracało wartość naprawdę używaną. `NO_ANISOTROPY` to `1.0F`. Bez rozszerzenia anizotropii maksimum to 1, więc wynik to zawsze 1 |

```cpp
bool AssetCache::hasFailed(const std::filesystem::path& path) const {
    // find returns the end of the list when no element is equal to path.
    return std::ranges::find(m_failedPaths, path) != m_failedPaths.end();
}
```

`std::ranges::find` (C++20) przyjmuje cały kontener zamiast pary iteratorów i zwraca iterator do znalezionego elementu albo koniec listy.

### 5.8 Gdzie pamięć podręczna żyje

`NightMazeApp` ma jedną pamięć podręczną jako pole:

```cpp
    assets::AssetCache m_assets;
    MazeRenderer m_mazeRenderer;
    GameplayRenderer m_gameplayRenderer;
    TerrainRenderer m_terrainRenderer;
```

`m_assets` nie ma wpisu w liście inicjalizacyjnej konstruktora (wystarcza konstruktor domyślny), a `m_mazeRenderer(m_assets)`, `m_gameplayRenderer(m_assets)` i `m_terrainRenderer(m_assets)` dostają do niej referencję i każdy od razu prosi o swoje zasoby: dwa modele, trzy modele i dwie tekstury. Pola są tworzone z góry na dół, więc wszystkie trzy renderery stoją po `m_assets` (mówi o tym komentarz nad polami w `NightMazeApp.hpp`). Jako pole klasy pochodnej od `core::Application` pamięć podręczna powstaje po oknie i ginie przed nim, więc kontekst OpenGL istnieje przez całe jej życie. Panel dostaje ją przez akcesor:

```cpp
    /// The loaded models and textures, exposed so the debug UI can list them and change
    /// the texture filtering live.
    assets::AssetCache& assets() { return m_assets; }
```

### 5.9 Jak to zostało sprawdzone

Klasa nie ma testu jednostkowego: konstruktor tworzy teksturę, więc bez kontekstu OpenGL nie da się jej nawet utworzyć. Części składowe mają własne testy bez okna: 19 przypadków loadera OBJ (do M6 20, z przypadkiem płytki podłogi), 10 przypadków loadera obrazów (na prawdziwych plikach ściany i słupka, tekstur ściany i podłoża, w tym na ich mapach normalnych) i 9 przypadków funkcji liczących styczne. Plików z M5 (kryształy, brama i ich tekstury) te testy nie czytają. Cały program testowy: po M5 215 przypadków i 85098 asercji, po drugiej części M6 256 przypadków i 101232 asercje w Debug i w Release (Windows, 2026-10-05), a po czwartej części M7 (cienie księżyca) zgłoszone 310 przypadków i 103751 asercji. Żaden z przypadków dodanych w M7 nie dotyczy pamięci assetów.

Sprawdzenie na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik 610.74) przez uruchomienie programu i zrzuty ekranu, wykonane przy stanie z M4 (trzy modele, cztery tekstury, podłoga z płytek, której dziś nie ma):

| Sprawdzenie | Wynik |
|---|---|
| start programu | brak linii `[error]` i brak linii `GL_` |
| widok startowy w labiryncie | tekstury na ścianach, słupkach i podłodze ustawione poprawnie: nie do góry nogami i nie w lustrze |
| tryb widoku: normalne jako kolor, współrzędne tekstury jako kolor | oba obrazy zgodne z oczekiwaniem |
| filtr: najbliższy sąsiad, dwuliniowy, trójliniowy, trójliniowy z anizotropią 16x | różnice widoczne na ścianie oglądanej pod płaskim kątem |
| podglądy tekstur w panelu | nieodwrócone |
| brak pliku tekstury | powierzchnia w białym kolorze materiału, jedna linia `[error]` |
| mapy normalnych włączone (po ich dodaniu) | fugi czytają się jako wgłębienia na ścianach wzdłuż X, na ścianach wzdłuż Z, na słupku i na podłodze. Przesunięcie światła z lewej na prawą zamienia, które skosy są jasne |
| mapy normalnych włączone i wyłączone | średnia jasność obrazu prawie taka sama: ściana X 43,29 i 44,00, ściana Z 35,85 i 36,16, słupek 35,26 i 35,55, podłoga 22,69 i 22,86 (pierwsza liczba z mapami). Mapa zmienia rozkład światła, a nie jego ilość |
| tryby `Gouraud` i `Unlit` z przełącznikiem włączonym i wyłączonym | zrzuty identyczne co do piksela: w tych trybach mapa normalnych nie bierze udziału w obrazie |

Trzy ostatnie wiersze pochodzą z drugiej części M4. Przełącznik był przy tym ustawiany w kodzie, a nie kliknięciem. **Czego nikt nie sprawdził:** braku pliku mapy normalnych. Z kodu wynika, że część dostaje wtedy płaską mapę, w konsoli jest jedna linia `[error]` (choć mapy potrzebują ściana i słupek, bo nieudana ścieżka jest zapamiętywana), panel pokazuje przy części `normal map: none (flat)`, a plik trafia na listę `Failed to load`. To jest wniosek z czytania kodu, a nie pomiar.

Dla M5 powyższych sprawdzeń nie powtarzałem. Autor kodu zgłosił obraz z kryształami i bramą sprawdzony zrzutami ekranu, ale panelu Assets z sześcioma modelami i ośmioma teksturami nikt jeszcze nie odczytał z ekranu ani nie przeszedł ręcznie.

Dla pierwszej części M7 (przestrzeń kolorów tekstur) zgłoszone jest na Windowsie (2026-10-05, nie powtarzałem): bramka `make check` przechodzi, 269 przypadków testowych i 102103 asercje w Debug i Release, a po drugiej części M7 276 i 102139, zero ostrzeżeń, a miniatury w panelu Assets są identyczne co do piksela z miniaturami sprzed zmiany. Obraz sceny w trybie `Unlit` nie jest identyczny z poprzednim: różni się na fugach cegieł, bo filtrowanie tekstur działa teraz na wartościach liniowych: tak robią dzisiejsze karty, a OpenGL 4.1 tę kolejność (najpierw dekodowanie, potem filtr) zaleca, ale jej nie wymaga ([`../gfx/textures.md`](../gfx/textures.md), sekcja 2.10). Gałęzi z błędem dwóch przestrzeni nikt nie wywołał, a napisu `sRGB` albo `linear` w panelu nikt nie odczytał z ekranu ręcznie.

Stany filtra, anizotropii i trybu widoku były ustawiane tymczasowym kodem (usuniętym), a nie kliknięciem w panel. Ręczne przejście przez widżety jest otwartą pozycją listy kontrolnej w [`../../guides/build-windows.md`](../../guides/build-windows.md). Na macOS nic z tego nie było sprawdzane ([`../../guides/build-macos.md`](../../guides/build-macos.md)).

## 6. Panel ImGui

Panel **Assets** jest pokazem tematów 4 i 5. PRD (sekcja 3) wymienia dla tematu 4 "Lista załadowanych modeli", a dla tematu 5 "Podgląd tekstur, toggle normal map". Wszystkie trzy są: lista modeli, podgląd tekstur i, od drugiej części M4, przełącznik map normalnych (pole wyboru `Normal mapping`). Osobnego panelu o tej nazwie PRD nie przewiduje: nazwa Assets pochodzi z kodu.

Kod: [`src/debug/panels/AssetsPanel.cpp`](../../../src/debug/panels/AssetsPanel.cpp). Jak panel jest podpięty do `DebugUI`, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.

### 6.1 Stałe i funkcja główna

```cpp
// The entries of the two lists, in the order of the enums game::ViewMode and
// gfx::TextureFilter: the number of the chosen entry is the value of the enum. ImGui
// wants the entries in one string, each ended by a zero character.
constexpr const char* VIEW_MODE_ITEMS = "Textured\0Normals as colour\0UVs as colour\0";
constexpr const char* FILTER_ITEMS = "Nearest\0Bilinear\0Trilinear\0";
```

| Stała | Znaczenie |
|---|---|
| `VIEW_MODE_ITEMS` | trzy pozycje listy w jednym napisie, każda zakończona znakiem zerowym (`\0`). Kolejność jest taka sama jak w wyliczeniu `game::ViewMode` (0, 1, 2), więc numer wybranej pozycji **jest** wartością wyliczenia |
| `FILTER_ITEMS` | to samo dla `gfx::TextureFilter`: `Nearest`, `Bilinear`, `Trilinear` |

Pozostałe stałe pliku: `NO_ANISOTROPY` (`1.0F`), `INDICES_PER_TRIANGLE` (3) i `PREVIEW_SIZE` (`128.0F`, bok podglądu w pikselach). Dwie stałe, których panel używa, nie należą do niego: `ASSETS_PLACEMENT` z [`PanelLayout.hpp`](../../../src/debug/PanelLayout.hpp) (miejsce i rozmiar przy pierwszym uruchomieniu: prawa krawędź okna, pod panelem Maze, [`../debug-ui.md`](../debug-ui.md), sekcja 5.7) i `ERROR_TEXT_COLOR` z [`Theme.hpp`](../../../src/debug/Theme.hpp) (łagodna czerwień motywu, ta sama co w panelu Shaders, [`../debug-ui.md`](../debug-ui.md), sekcja 5.8).

```cpp
void drawAssetsPanel(assets::AssetCache& assets, game::ViewMode& viewMode, bool& normalMapping,
                     const RawTextureSampler& rawSampler) {
    // First run only: the right edge of the window, below the Maze panel (the constant
    // is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(ASSETS_PLACEMENT);
    if (ImGui::Begin("Assets")) {
        drawSettings(assets, viewMode, normalMapping);
        drawModels(assets);
        drawTextures(assets, rawSampler);
        drawFailures(assets);
    }
    ImGui::End();
}
```

Trzeci parametr, `normalMapping`, to referencja do pola `game::LightingSettings::normalMapping`. `DebugUI::draw` przekazuje ją jako `context.lighting.normalMapping` ([`../debug-ui.md`](../debug-ui.md)). Panel dostaje samą zmienną `bool`, a nie całą strukturę ustawień świateł: resztą tej struktury zajmuje się panel Lights. Pole leży w ustawieniach oświetlenia, bo mapa normalnych zmienia tylko to, jak światło pada na powierzchnię, a przełącznik stoi w panelu Assets, bo jest pokazem tematu 5 (tekstury). Czwarty parametr, `rawSampler`, doszedł w M7: to obiekt `debug::RawTextureSampler`, pole klasy `DebugUI` (`m_rawTextureSampler`), którym `drawTextures` czyta miniatury tekstur sRGB bez dekodowania (sekcja 6.4). Panel dostaje go przez stałą referencję i tylko przekazuje dalej.

Panel dostaje pamięć podręczną bez `const`, bo dwa widżety wołają `setFilter` i `setAnisotropy`. Trzy z czterech funkcji pomocniczych przyjmują ją już jako `const`: tylko czytają listy. Podział na cztery funkcje odpowiada czterem częściom panelu.

### 6.2 Przełączniki: `drawSettings`

```cpp
    int viewModeIndex = static_cast<int>(viewMode);
    if (ImGui::Combo("View mode", &viewModeIndex, VIEW_MODE_ITEMS)) {
        viewMode = static_cast<game::ViewMode>(viewModeIndex);
    }

    // Checkbox flips the bool through the pointer. The game reads it in every frame, so
    // the scene changes at once: the same walls with and without their relief. It stands
    // next to the view mode, because the two together decide which normals are shown.
    ImGui::Checkbox("Normal mapping", &normalMapping);
    ImGui::TextWrapped("Shows under Phong and Blinn-Phong lighting (Renderer panel) and in "
                       "the view \"Normals as colour\". Gouraud lights per vertex and cannot "
                       "use a normal map.");

    int filterIndex = static_cast<int>(assets.filter());
    if (ImGui::Combo("Filter", &filterIndex, FILTER_ITEMS)) {
        assets.setFilter(static_cast<gfx::TextureFilter>(filterIndex));
    }
```

| Linia | Znaczenie |
|---|---|
| `int viewModeIndex = static_cast<int>(viewMode);` | `Combo` pracuje na numerze pozycji typu `int`, a stan jest wyliczeniem: kopia do zmiennej lokalnej |
| `ImGui::Combo("View mode", &viewModeIndex, VIEW_MODE_ITEMS)` | lista rozwijana. Zwraca prawdę tylko w klatce, w której użytkownik wybrał inną pozycję |
| `viewMode = static_cast<game::ViewMode>(viewModeIndex);` | zapis z powrotem. `viewMode` to referencja do pola `m_viewMode` aplikacji, które `drawMaze` wysyła jako `uViewMode` |
| `ImGui::Checkbox("Normal mapping", &normalMapping);` | pole wyboru. Dostaje **wskaźnik** na zmienną i samo ją przełącza przy kliknięciu. Wartości zwracanej (prawda w klatce kliknięcia) kod nie potrzebuje: gra czyta zmienną w każdej klatce, więc scena zmienia się od razu. Inaczej niż przy `Combo` nie trzeba kopii w zmiennej lokalnej, bo stan jest już typu `bool` |
| `ImGui::TextWrapped("Shows under Phong and Blinn-Phong ...")` | notka pod polem: mapy normalnych widać przy oświetleniu `Phong` i `Blinn-Phong` (lista `Lighting` w panelu Renderer) oraz w widoku `Normals as colour`. `Gouraud` liczy światło w wierzchołkach i mapy normalnych użyć nie może. `\"` to cudzysłów wewnątrz napisu C++ |
| `assets.setFilter(...)` | wołane tylko przy zmianie, a nie w każdej klatce: funkcja przechodzi po wszystkich teksturach i woła OpenGL |

**Pole wyboru a to, co naprawdę widać.** Pole ustawia tylko `normalMapping`. O tym, czy mapy biorą udział w obrazie, decyduje funkcja `game::usesNormalMap`: pole włączone **i** tryb oświetlenia inny niż `Gouraud` ([`../game/flashlight.md`](../game/flashlight.md)). Stąd cztery przypadki:

| Tryb `Lighting` | `View mode` | Skutek włączonego pola |
|---|---|---|
| `Phong`, `Blinn-Phong` | `Textured` | relief w oświetleniu: fugi i nierówności pod latarką i światłami |
| `Gouraud` | `Textured` | żaden. Obraz jest taki sam jak przy polu wyłączonym |
| `Unlit` | `Textured` | żaden. Bez światła normalna nie ma na co wpływać |
| `Unlit`, `Phong`, `Blinn-Phong` | `Normals as colour` | widok pokazuje normalne z map: na płaskich kolorach ścian pojawia się wzór fug |
| `Gouraud` | `Normals as colour` | żaden. Widok pokazuje normalne siatki, bo w tym trybie cieniowanie ich używa |

Pole stoi zaraz pod listą `View mode`, bo te dwie kontrolki razem decydują, które normalne widać. Kliknięcia w pole i wyglądu panelu z polem na miejscu **nikt jeszcze nie sprawdził ręcznie**: stany z tabeli były ustawiane w kodzie i oglądane na zrzutach ekranu (sekcja 5.9).

```cpp
    const float maxAnisotropy = assets.maxAnisotropy();
    const bool anisotropySupported = maxAnisotropy > NO_ANISOTROPY;
    ImGui::BeginDisabled(!anisotropySupported);
    float anisotropy = assets.anisotropy();
    if (ImGui::SliderFloat("Anisotropy", &anisotropy, NO_ANISOTROPY, maxAnisotropy, "%.0fx",
                           ImGuiSliderFlags_AlwaysClamp)) {
        assets.setAnisotropy(anisotropy);
    }
    ImGui::EndDisabled();
    if (!anisotropySupported) {
        ImGui::TextWrapped("Anisotropic filtering is not offered by this graphics driver.");
    }
    ImGui::TextWrapped("The filter and the anisotropy apply to all textures. Their effect "
                       "shows in the scene, not in the previews below.");
```

| Linia | Znaczenie |
|---|---|
| `maxAnisotropy > NO_ANISOTROPY` | sterownik bez rozszerzenia anizotropii zgłasza maksimum 1 |
| `ImGui::BeginDisabled(!anisotropySupported)` ... `EndDisabled()` | widżety między tymi wywołaniami są wyszarzone i nie reagują, gdy argument jest prawdą. Suwak jest widoczny zawsze, żeby było jasne, że funkcja istnieje, tylko sterownik jej nie daje |
| `SliderFloat("Anisotropy", ..., NO_ANISOTROPY, maxAnisotropy, "%.0fx", ...)` | suwak od 1 do maksimum sterownika. Format `"%.0fx"` pokazuje wartość bez części ułamkowej z literą x: `16x` |
| `assets.setAnisotropy(anisotropy);` | tylko gdy suwak się ruszył |
| ostatni `TextWrapped` | przypomnienie pod przełącznikami: skutek widać w scenie, nie w podglądach (sekcja 6.4) |

Suwak jest ciągły, więc daje także wartości pośrednie (na przykład 5,3). Specyfikacja rozszerzenia dopuszcza dowolną liczbę zmiennoprzecinkową od 1 do maksimum, a co z nią zrobi sterownik, zależy od sterownika.

### 6.3 Listy: `drawModels`, `drawFailures` i `drawFileName`

```cpp
void drawFileName(const std::filesystem::path& path) {
    const std::string fileName = core::pathText(path.filename());
    const std::string fullPath = core::pathText(path);
    ImGui::TextUnformatted(fileName.c_str());
    ImGui::SetItemTooltip("%s", fullPath.c_str());
}
```

Linia z samą nazwą pliku i pełną ścieżką w dymku po najechaniu myszą. `core::pathText` daje UTF-8, którego oczekuje ImGui ([`../core/paths.md`](../core/paths.md)). Ścieżka idzie jako argument formatu `"%s"`, a nie jako sam format: znak procentu w nazwie katalogu nie zostanie wtedy potraktowany jak polecenie formatowania.

```cpp
void drawModels(const assets::AssetCache& assets) {
    ImGui::SeparatorText("Models");
    for (const assets::LoadedModel& model : assets.models()) {
        drawFileName(model.path);
        ImGui::Text("  %d vertices, %d triangles", static_cast<int>(model.vertexCount),
                    static_cast<int>(model.triangleCount));
        for (const assets::ModelPart& part : model.parts) {
            // A part without its own texture is drawn with the white one, in its colour.
            const std::string textureName = part.hasOwnTexture
                                                ? core::pathText(part.texturePath.filename())
                                                : std::string("no texture (white)");
            ImGui::Text("  part '%s': %d triangles, %s", part.material.c_str(),
                        static_cast<int>(part.indexCount / INDICES_PER_TRIANGLE),
                        textureName.c_str());
            // A part without its own normal map is shaded with the flat one: with the
            // normals of its mesh.
            const std::string normalMapName = part.hasOwnNormalMap
                                                  ? core::pathText(part.normalMapPath.filename())
                                                  : std::string("none (flat)");
            ImGui::Text("    normal map: %s", normalMapName.c_str());
        }
    }
}
```

| Linia | Znaczenie |
|---|---|
| `ImGui::SeparatorText("Models")` | pozioma linia z nagłówkiem |
| `static_cast<int>(model.vertexCount)` | `%d` oczekuje `int`, a liczniki są typu `std::size_t` |
| `part.hasOwnTexture ? ... : std::string("no texture (white)")` | część z teksturą zastępczą jest opisana wprost. Dzięki temu na liście widać od razu, której części brakuje tekstury |
| `part.indexCount / INDICES_PER_TRIANGLE` | trójkąty części |
| `part.hasOwnNormalMap ? ... : std::string("none (flat)")` | ten sam wzór dla mapy normalnych: nazwa pliku albo napis mówiący, że część ma płaską mapę zastępczą |
| `ImGui::Text("    normal map: %s", ...)` | osobna linia pod częścią, wcięta o dwie spacje głębiej niż linia części |

Dla gry lista miała do M8, części 2 pięć modeli, każdy z jedną częścią, w kolejności wczytania (od M8, części 2 ma ich osiem, wiersze `lever`, `lever_handle` i `note` poniżej tabeli):

| Plik | Linia `vertices, triangles` | Część | Tekstura | Mapa normalnych |
|---|---|---|---|---|
| `wall_straight.obj` | 60 i 30 | `wall_stone` | `wall_stone.png` | `wall_stone_normal.png` |
| `wall_pillar.obj` | 60 i 30 | `wall_stone` | `wall_stone.png` | `wall_stone_normal.png` |
| `crystal_a.obj` | 60 i 24 | `crystal` | `crystal.png` | `crystal_normal.png` |
| `crystal_b.obj` | 144 i 66 | `crystal` | `crystal.png` | `crystal_normal.png` |
| `gate.obj` | 148 i 70 | `gate_wood` | `gate_wood.png` | `gate_wood_normal.png` |

Po M8, części 2 doszły wiersze `lever.obj` (część `lever_iron`, tekstura `lever_iron.png`), `lever_handle.obj` (część `lever_brass`, `lever_brass.png`) i `note.obj` (część `note_paper`, `note_paper.png`, 10 trójkątów policzonych ze skryptu), każdy z mapą normalnych z końcówką `_normal`. Liczba trójkątów po przeróbce modelu (policzone z linii `f` plików `.obj`, nie z panelu): `lever.obj` 52, `lever_handle.obj` 20. Panelu Assets z nową listą nikt nie oglądał.

Do M6 pierwszym wierszem była płytka podłogi (`floor_tile.obj`, 4 i 2). Terenu na liście nie ma i nie będzie: lista pokazuje `AssetCache::models()`, a siatka terenu nie pochodzi z pliku i należy do `TerrainRenderer`. Liczby terenu (punkty siatki, trójkąty) pokazuje panel Terrain.

Liczby wierzchołków to nie liczby linii `v` z pliku: loader robi osobny wierzchołek z każdej różnej trójki `pozycja/uv/normalna`, więc `crystal_a.obj` ma 14 pozycji i 60 wierzchołków. Liczby dwóch modeli kamiennych potwierdzają testy loadera. Liczby kryształów i bramy policzyłem skryptem, który powtarza tę regułę na liniach `f` pliku ([`obj-loader.md`](obj-loader.md), sekcje 2.4 i 5.9): nikt ich jeszcze nie odczytał z panelu. To jest pokaz tematu 4: trzy listy indeksów pliku OBJ zamienione na jedną siatkę z podziałem na materiały.

`drawFailures` nie rysuje nic, gdy lista porażek jest pusta. W przeciwnym razie pokazuje nagłówek `Failed to load` i nazwy plików na czerwono (`PushStyleColor` i `PopStyleColor` wokół pętli).

### 6.4 Podgląd tekstur: `drawTextures` i odwrócone UV

```cpp
void drawTextures(const assets::AssetCache& assets, const RawTextureSampler& rawSampler) {
    ImGui::SeparatorText("Textures");
    for (const assets::LoadedTexture& loaded : assets.textures()) {
        drawFileName(loaded.path);
        // sRGB: a colour picture, decoded to linear values when a shader reads it.
        // Linear: data that is read as it is stored (a normal map).
        const bool isSrgb = loaded.texture.colorSpace() == gfx::ColorSpace::Srgb;
        ImGui::Text("  %d x %d px, %s", loaded.texture.width(), loaded.texture.height(),
                    isSrgb ? "sRGB" : "linear");

        const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
        if (isSrgb) {
            rawSampler.begin();
        }
        ImGui::Image(textureId, {PREVIEW_SIZE, PREVIEW_SIZE}, {0.0F, 1.0F}, {1.0F, 0.0F});
        if (isSrgb) {
            rawSampler.end();
        }
    }
}
```

W pliku nad linią z `textureId` stoi dłuższy komentarz po angielsku: wyjaśnia rzutowanie, odwrócone współrzędne, własny sampler ImGui i, od M7, dlaczego tekstura sRGB jest czytana bez dekodowania.

**Co zmieniła pierwsza część M7.** Trzy rzeczy. Linia z rozmiarem pokazuje też przestrzeń kolorów, na przykład `512 x 512 px, sRGB` dla `wall_stone.png` i `512 x 512 px, linear` dla `wall_stone_normal.png`. Funkcja dostała drugi parametr. A wywołanie `ImGui::Image` jest dla tekstur sRGB otoczone parą `begin` i `end`:

| Linia | Znaczenie |
|---|---|
| `loaded.texture.colorSpace() == gfx::ColorSpace::Srgb` | pyta teksturę, czym jest. Wynik steruje i napisem, i sposobem odczytu miniatury |
| `isSrgb ? "sRGB" : "linear"` | napis do formatu `%s` |
| `rawSampler.begin()` | od tego miejsca obrazki dodawane do bieżącego okna ImGui są czytane przez obiekt samplera z wyłączonym dekodowaniem sRGB |
| `rawSampler.end()` | powrót do samplera, którym ImGui rysuje wszystko inne |

**Po co to.** ImGui rysuje miniaturę prosto do okna, już po przebiegu składającym, który koduje scenę na sRGB ([`../renderer/post-process.md`](../renderer/post-process.md)). Tekstura sRGB czytana zwykłym samplerem oddaje wartości **liniowe**, a ImGui zapisuje to, co przeczyta, bez kodowania: miniatura byłaby wyraźnie ciemniejsza niż plik (bajt 128 wyszedłby jako poziom 55). `RawTextureSampler` każe karcie pominąć dekodowanie na czas tego jednego obrazka, więc na ekran trafiają bajty z pliku, jak przed M7. Mapy normalnych są liniowe, niczego się przy nich nie dekoduje i idą zwykłą drogą.

Mechanizm używa rozszerzenia `GL_EXT_texture_sRGB_decode`, którego nie ma w rdzeniu OpenGL 4.1. Gdzie sterownik go nie podaje, `begin` i `end` nic nie robią, a miniatury obrazów koloru są ciemniejsze niż pliki: to znane ograniczenie, a nie błąd tekstur (w scenie wyglądają poprawnie). Klasę, jej stałe i sposób, w jaki wpina się w listę poleceń rysowania ImGui, opisuje [`../debug-ui.md`](../debug-ui.md). Zgłoszone dla Windowsa (sterownik z rozszerzeniem): miniatury są identyczne co do piksela z miniaturami sprzed M7. Na macOS dostępności rozszerzenia nie sprawdzono.

Pętla po teksturach nie zmieniła się ani przy dodaniu map normalnych, ani w M5, a mimo to lista ma teraz **osiem** pozycji (w M2 + M3 dwie, w M4 cztery): `wall_stone.png`, `wall_stone_normal.png`, `crystal.png`, `crystal_normal.png`, `gate_wood.png`, `gate_wood_normal.png`, `ground.png` i `ground_normal.png`, w kolejności wczytania (do M6 listę otwierały `floor_stone.png` i `floor_stone_normal.png`, a tekstur podłoża nie było). Dwie ostatnie trafiły na listę inną drogą niż reszta, przez bezpośrednie `texture()` z `TerrainRenderer`, ale lista tego nie rozróżnia. Pliku `heightmap.png` na niej nie ma. Wszystkie mają 512 x 512 pikseli (odczytane z nagłówków plików PNG). Mapy normalnych są na liście `m_textures` jak każda inna tekstura. Ich podgląd to obraz taki, jaki jest zapisany, bez żadnego przeliczania: w większości jasnoniebieski, bo większość tekseli ma kierunek bliski `(0, 0, 1)`, czyli kolor `(128, 128, 255)`, a na nim wzór fug w odcieniach różu, zieleni i fioletu (skosy odchylone w różne strony). Ten opis dotyczy dwóch map kamienia (ściany i dawnej podłogi), które widziałem na zrzutach. Podglądów map kryształu, drewna i podłoża nie oglądałem: z kodowania wynika tylko, że płaskie miejsca też są jasnoniebieskie. Tekstur zastępczych na liście nie ma.

| Linia | Znaczenie |
|---|---|
| `loaded.texture.id()` | numer (nazwa) obiektu tekstury OpenGL. Dla backendu OpenGL biblioteki ImGui właśnie ten numer identyfikuje teksturę |
| `static_cast<ImTextureID>(...)` | `ImTextureID` to typ całkowity ImGui na identyfikator tekstury. Rzutowanie tylko poszerza `GLuint` |
| `ImGui::Image(id, rozmiar, uv0, uv1)` | prostokąt z teksturą. `uv0` to współrzędne tekstury lewego górnego rogu obrazka, `uv1` prawego dolnego |
| `{0.0F, 1.0F}`, `{1.0F, 0.0F}` | odwrócenie w pionie, wyjaśnione niżej |

**Dlaczego UV są odwrócone.** Są tu dwie konwencje:

```text
tekstura w OpenGL (i w całym projekcie)          obrazek w ImGui (jak ekran)

 v = 1  +-----------+   górny wiersz obrazu       uv0 = lewy górny róg
        |           |                               +-----------+
        |           |                               |           |
 v = 0  +-----------+   dolny wiersz obrazu         |           |
        u = 0     u = 1                             +-----------+
                                                          uv1 = prawy dolny róg
```

Loader obrazów odwraca wiersze przy wczytaniu, żeby `v = 0` było dolnym wierszem ([`images.md`](images.md), sekcja 2). ImGui domyślnie zakłada `uv0 = (0, 0)` i `uv1 = (1, 1)`, czyli że `v = 0` to **górny** wiersz. Z wartościami domyślnymi podgląd byłby do góry nogami. Lewy górny róg obrazka ma w konwencji OpenGL współrzędne `(0, 1)`, a prawy dolny `(1, 0)`, i dokładnie te wartości dostaje `Image`. Oś u zostaje bez zmian, więc obraz nie jest odbity w poziomie.

**Podgląd nie reaguje na filtr.** ImGui rysuje swoje prostokąty własnym shaderem i z własnym obiektem samplera. Wersja backendu w projekcie (ImGui 1.92.9b) przy rysowaniu podpina do jednostki 0 swój sampler z filtrem liniowym i zawijaniem `GL_CLAMP_TO_EDGE`, a obiekt samplera podpięty do jednostki ma pierwszeństwo przed ustawieniami tekstury. Podgląd jest więc zawsze liniowy, bez mipmap i bez anizotropii, niezależnie od pozycji listy `Filter`. Mówi o tym też ostatnia linia tekstu w części z przełącznikami. Dla tekstury 512 x 512 pokazanej w 128 pikselach oznacza to pomniejszenie czterokrotne bez mipmap, więc podgląd może być lekko ziarnisty: to cecha podglądu, a nie tekstury.

### 6.5 Co pokazać na obronie

Kroki z klikaniem nie były jeszcze wykonane ręcznie. Kolumna "co widać" opisuje to, co wynika z kodu shadera i klas. Na zrzutach ekranu z Windowsa, gdzie te same stany były ustawione kodem, sprawdzone są: oba widoki diagnostyczne, porównanie filtrów z anizotropią 16x, nieodwrócone podglądy, biała tekstura zastępcza oraz relief z mapami normalnych włączonymi i wyłączonymi (sekcja 5.9). Wiersze o liście sześciu modeli, o liście ośmiu tekstur i o linii `normal map: ...` opisują to, co wynika z kodu panelu: nikt ich jeszcze nie odczytał z ekranu. Wrażeń z ruchu (migotanie) na nieruchomych zrzutach ocenić się nie da.

| Widżet | Co robię | Co widać | Co to pokazuje (temat) |
|---|---|---|---|
| lista `Models` | czytam pięć wpisów: liczby wierzchołków i trójkątów, część `wall_stone`, `crystal` albo `gate_wood` z nazwą tekstury i linią `normal map: ...` | po 30 trójkątów ściany i słupka, 24 i 66 kryształów, 70 bramy (tabela w sekcji 6.3), pod każdą częścią plik mapy normalnych | 4: wynik parsera OBJ i MTL (linie `map_Kd` i `map_Bump`), jedna siatka z podziałem na materiały |
| lista `Textures` | pokazuję osiem wpisów 512 x 512 i ich podglądy | osiem tekstur, choć modeli jest pięć i każdy ma dwie, a do tego dwie ma teren: cztery obrazy koloru i cztery mapy normalnych | 4 i 5: tekstura i mapa normalnych wspólne dla ściany i słupka są na karcie raz, tak samo wspólne dla obu kryształów. Mapa normalnych to zwykły obraz RGB |
| `Normal mapping` | przy trybie `Blinn-Phong` staję blisko ściany, świecę latarką pod płaskim kątem i odznaczam, a potem zaznaczam pole | bez map ściana wygląda jak tapeta: płaska, z namalowanymi fugami. Z mapami fugi są wgłębieniami, a skosy kamieni łapią światło | 5: mapa normalnych zmienia światło, nie kształt. Pełny scenariusz jest w [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 6 |
| podgląd | najeżdżam na nazwę pliku | pełna ścieżka w dymku. Obraz nie jest do góry nogami | 5: `v = 0` na dole, odwrócone UV w `ImGui::Image` |
| `Filter` | staję przed ścianą, potem patrzę wzdłuż długiego korytarza i przełączam `Nearest`, `Bilinear`, `Trilinear` | z bliska `Nearest` daje kwadraty tekseli, z daleka `Nearest` i `Bilinear` migoczą w ruchu, `Trilinear` jest spokojny | 5: filtr powiększenia i pomniejszenia, mipmapy |
| `Anisotropy` | przy `Trilinear` patrzę na podłoże albo ścianę pod płaskim kątem i przesuwam suwak od 1x do maksimum | rozmyty pas w głębi staje się ostry | 5: filtrowanie anizotropowe jako rozszerzenie |
| `View mode`: `UVs as colour` | przełączam i podchodzę do ściany | czerwień rośnie wzdłuż u, zieleń wzdłuż v, wzór zaczyna się od czerni tam, gdzie tekstura się powtarza | 5: współrzędne tekstury i zawijanie `GL_REPEAT` |
| `View mode`: `Normals as colour` | przełączam, rozglądam się, a potem odznaczam i zaznaczam `Normal mapping` | kolor to `normalna * 0,5 + 0,5`: podłoże zielonkawe (normalna bliska +Y, na zboczach terenu lekko odchylona), ściana zwrócona w stronę +X czerwonawa, w stronę +Z niebieskawa, a zwrócone w strony przeciwne są w tym kanale ciemne (-X wychodzi morska, -Z oliwkowa). Z polem odznaczonym każda ściana ma jeden gładki kolor, a teren kolor zmieniający się łagodnie z nachyleniem. Z polem zaznaczonym na tych kolorach widać wzór fug: to normalne z mapy, już w przestrzeni świata | 4: normalne z pliku OBJ, obrócone razem z modelem. 5: te same normalne po zamianie przez mapę |
| `Failed to load` | przed uruchomieniem zmieniam nazwę pliku tekstury w skopiowanym katalogu `assets` | biała powierzchnia zamiast kamienia, jedna linia `[error]` w konsoli, czerwony wpis w panelu, a przy części napis `no texture (white)` | 4 i 5: obsługa błędów bez zatrzymania programu, tekstura zastępcza |

Dobra kolejność: najpierw listy (co jest wczytane), potem `Normal mapping` przy latarce, potem `Filter` i `Anisotropy` przy tym samym ujęciu korytarza, na końcu dwa widoki diagnostyczne. Uwaga na jedną niespodziankę: przy trybie `Gouraud` widok `Normals as colour` nie pokazuje reliefu nawet z zaznaczonym polem, bo `usesNormalMap` jest wtedy fałszem (tabela w sekcji 6.2).

## 7. Pułapki

1. **`std::vector` zamiast `std::deque`.** Program działa, dopóki wektor nie przekroczy pojemności. Potem renderer rysuje ze wskaźnika na zwolnioną pamięć. Błąd nie zależy od kodu, który go wywołał, tylko od liczby wczytanych zasobów, więc pojawia się dużo później niż zmiana, która go wprowadziła.
2. **Usuwanie albo wstawianie w środku `std::deque`.** Gwarancja stabilnych wskaźników dotyczy tylko dopisywania na początku albo na końcu. Funkcja "usuń zasób" unieważniłaby wskaźniki do pozostałych.
3. **Odwrócona kolejność pól.** Lista modeli zadeklarowana przed listą tekstur ginęłaby po niej. Dziś nic by się nie stało (destruktor części nie dotyka tekstury), ale reguła "wskazywany żyje dłużej niż wskazujący" przestałaby obowiązywać i pierwszy destruktor, który sięgnie po teksturę, trafiłby na usunięty obiekt.
4. **Przechowanie wskaźnika dłużej niż pamięć podręczna.** `MazeRenderer` i `GameplayRenderer` trzymają gołe wskaźniki. Poprawność zależy od kolejności pól w `NightMazeApp`, a nie od typu wskaźnika.
5. **Ten sam plik pod dwiema ścieżkami.** Normalizacja jest leksykalna: różna wielkość liter albo ścieżka względna i bezwzględna dają dwa klucze i dwa wczytania.
6. **Naprawiony plik nie wraca.** Ścieżka, która raz zawiodła, nie jest próbowana ponownie. Po przywróceniu brakującej tekstury trzeba uruchomić program od nowa.
7. **Brak tekstury wygląda jak zamierzony kolor.** Biała tekstura zastępcza sprawia, że program działa dalej, ale też że błąd łatwo przeoczyć. Śladem jest linia `[error]`, wpis `Failed to load` i napis `no texture (white)` przy części.
8. **Obraz w odcieniach szarości.** Dekoduje się poprawnie, ale ma 1 albo 2 kanały, a `Texture2D` przyjmuje 3 albo 4. Skutek jest taki jak przy braku pliku. Tekstury trzeba zapisywać jako RGB albo RGBA.
9. **Wczytanie assetu w środku rysowania.** `model()` i `texture()` podpinają VAO, bufory i teksturę. Wywołane między ustawieniem stanu a `glDrawElements` innego obiektu zepsułyby ten stan. Gra prosi o modele tylko w konstruktorach obu rendererów.
10. **Podgląd z domyślnymi UV.** `ImGui::Image(id, rozmiar)` bez dwóch ostatnich argumentów pokazuje teksturę do góry nogami, bo ImGui liczy v od góry, a projekt od dołu.
11. **Ocenianie filtra po podglądzie.** Podgląd rysuje ImGui własnym samplerem liniowym. Filtr i anizotropię widać tylko w scenie.
12. **Kolejność pozycji listy a wyliczenie.** `VIEW_MODE_ITEMS` i `FILTER_ITEMS` muszą mieć pozycje w kolejności wartości wyliczeń. Nowa wartość dopisana w środku wyliczenia przesuwa wszystkie następne i lista zaczyna wybierać nie to, co pokazuje.
13. **`setFilter` w każdej klatce.** `Combo` zwraca prawdę tylko przy zmianie i tylko wtedy kod woła `setFilter`. Wołanie bezwarunkowe ustawiałoby parametry samplerów wszystkich tekstur w każdej klatce, bez żadnego skutku.
14. **Biała tekstura jako zastępcza mapa normalnych.** Biel `(255, 255, 255)` to w kodowaniu mapy normalnych kierunek `(1, 1, 1)`: normalna przechylona w stronę rogu, czyli złe światło na całej części. Mapa normalnych ma własną teksturę zastępczą, `(128, 128, 255)`.
15. **Pusty wskaźnik `normalMap`.** Funkcja `game::drawModel` woła `part.normalMap->bind(...)` bez sprawdzania. Działa to tylko dlatego, że `model()` ustawia wskaźnik na płaską mapę w pierwszych liniach pętli. Część zbudowana ręcznie, poza pamięcią podręczną, miałaby `nullptr` z wartości początkowej struktury.
16. **Brak mapy normalnych wygląda jak zamierzona płaskość.** Tak jak przy białej teksturze, program działa dalej i błąd łatwo przeoczyć: ściana ma teksturę, tylko bez reliefu. Śladem jest linia `[error]`, wpis `Failed to load` i napis `normal map: none (flat)` przy części.
17. **Zaznaczone pole, a reliefu nie ma.** Pole `Normal mapping` jest zaznaczone, ale tryb oświetlenia to `Gouraud` albo `Unlit`. To nie błąd: tak działa `usesNormalMap` i mówi o tym notka pod polem.
18. **Ocenianie mapy normalnych po podglądzie.** Podgląd pokazuje bajty mapy jako kolory. Widać na nim, że plik się wczytał i gdzie są fugi, ale nie widać, czy relief jest wgłębieniem, czy grzbietem. To widać dopiero w scenie, pod światłem.
19. **Filtr `Nearest` a mapy normalnych.** Filtr z panelu dotyczy także map normalnych. Przy `Nearest` relief w oddali jest ziarnisty, co łatwo wziąć za błąd mapy.
20. **Zła przestrzeń kolorów przy `texture()`.** Od M7 drugi argument decyduje o formacie na karcie. `Linear` dla obrazu koloru daje powierzchnię wyblakłą i za jasną (nikt nie zdekodował, a przebieg składający i tak zakodował), `Srgb` dla mapy normalnych przekrzywia wszystkie normalne. Kompilator pilnuje tylko tego, że argument jest. Panel Assets pokazuje wynik słowem `sRGB` albo `linear` przy każdej teksturze: mapa normalnych z napisem `sRGB` to błąd.
21. **Kolorowy `Kd` nie jest przeliczany.** `part.color` trafia do `uTint` tak, jak stoi w pliku MTL, i mnoży liniowy kolor tekstury. Liczby w MTL są wybierane na ekranie, czyli w sRGB, więc poprawnie należałoby je zamienić na liniowe, tak jak kolory świateł. Dziś wszystkie materiały mają biały `Kd`, a biel to 1 w obu przestrzeniach, więc błędu nie widać. Pierwszy model z kolorowym `Kd` wyjdzie jaśniejszy, niż wynikałoby z poprawnego przeliczenia: to znany, otwarty punkt.
22. **Miniatury ciemniejsze niż pliki.** Na sterowniku bez `GL_EXT_texture_sRGB_decode` miniatury tekstur sRGB w panelu są ciemniejsze od plików. To ograniczenie podglądu, a nie błąd wczytania: w scenie te same tekstury wyglądają poprawnie.

## 8. Ćwiczenia

Ćwiczenia od 1 do 3 robi się na kartce. Pozostałe to zmiany w kodzie albo w skopiowanych assetach: po zmianie kodu zbuduj projekt (`cmake --build --preset debug`) i uruchom program, a na końcu wycofaj zmianę (`git checkout src`). Ćwiczeń od 4 do 12 nie wykonywałem: opisy skutków wynikają z czytania kodu, poza ćwiczeniem 4, którego skutek jest na zrzucie ekranu z Windowsa.

1. **Ile obiektów.** Gra prosi o `wall_straight.obj`, `wall_pillar.obj`, `crystal_a.obj`, `crystal_b.obj` i `gate.obj`, a `TerrainRenderer` wprost o `ground.png` i `ground_normal.png`. Ile razy wołana jest funkcja `texture()` i ile tekstur powstaje na karcie (z zastępczymi)? Odpowiedź: 12 wywołań (po dwa na część pięciu modeli, czyli 10, i dwa z terenu), 8 wczytań (słupek i drugi kryształ trafiają w pamięć podręczną), 10 tekstur (osiem z plików, biała i płaska).
2. **Klucz.** Co zwraca `lexically_normal` dla `C:/gra/assets/models/../textures/./wall_stone.png`? Odpowiedź: `C:/gra/assets/textures/wall_stone.png` (na Windowsie z odwrotnymi ukośnikami).
3. **Kolor części.** Materiał ma `Kd 1.0 0.5 0.0` i nie ma `map_Kd`. Jaki kolor ma fragment? Odpowiedź: `(1, 1, 1) * (1, 0,5, 0)`, czyli pomarańczowy `(1, 0,5, 0)`: biały teksel razy `uTint`. Od M7 to jest wartość **liniowa** zapisana do bufora sceny. Na ekran trafia po przebiegu składającym: w trybie `Unlit` z mapowaniem tonów `None` kanał zielony 0,5 jest kodowany na 0,735, więc piksel ma kolor `(1, 0,735, 0)`, jaśniejszy pomarańczowy, niż sugeruje plik MTL (pułapka 21).
4. **Brak tekstury.** W katalogu `assets` obok pliku wykonywalnego (na Windowsie `build/debug/Debug/assets`) zmień nazwę `wall_stone.png` i uruchom program. Ile linii `[error]` jest w konsoli, choć tekstury potrzebują dwa modele? Co pokazuje panel Assets? Przywróć nazwę (albo odśwież kopię: `cmake --build --preset debug --target copy_assets`).
5. **Wektor zamiast kolejki.** Zamień `std::deque<LoadedModel>` na `std::vector<LoadedModel>` (i typ zwracany przez `models()`). Czy program od razu przestaje działać przy pięciu modelach? Od czego to zależy? Dopisz przed pierwszym wczytaniem `m_models.reserve(1)` i sprawdź ponownie. Dlaczego taki błąd jest groźniejszy niż błąd kompilacji?
6. **Kolorowa tekstura zastępcza.** Zmień `WHITE_PIXEL` na jaskrawy róż `{255, 0, 255}` i powtórz ćwiczenie 4. Co zyskujesz przy szukaniu brakujących tekstur, a co tracisz dla materiałów, które celowo nie mają tekstury?
7. **Licznik wczytań.** Dodaj tymczasowo `core::logInfo` na początku `texture()` z kluczem. Ile linii pojawia się przy starcie i które z nich są trafieniami w pamięć podręczną?
8. **Podgląd do góry nogami.** W `drawTextures` usuń dwa ostatnie argumenty `ImGui::Image`. Porównaj podgląd z teksturą na ścianie. Potem zamień tylko u (`{1.0F, 1.0F}`, `{0.0F, 0.0F}`): co się zmieniło?
9. **Filtr na jednej teksturze.** Dodaj do pamięci podręcznej funkcję, która ustawia filtr tylko tekstury o podanej ścieżce, i wywołaj ją tymczasowo dla `ground.png` z filtrem `Nearest`. Dlaczego wspólne pole `m_filter` przestaje wtedy opisywać stan wszystkich tekstur i co powinien pokazywać panel?
10. **Brak mapy normalnych.** Powtórz ćwiczenie 4 dla pliku `wall_stone_normal.png`. Ile linii `[error]` jest w konsoli? Co pokazuje panel przy częściach ściany i słupka i ile pozycji ma lista `Textures`? Jak wyglądają ściany pod latarką w porównaniu z podłożem? (Oczekiwane z kodu: jedna linia, `normal map: none (flat)` przy obu częściach, siedem tekstur na liście, ściany bez reliefu, podłoże z reliefem.)
11. **Zła tekstura zastępcza.** Zmień tymczasowo w `model()` linię `part.normalMap = &m_flatNormalTexture;` na `part.normalMap = &m_whiteTexture;` i powtórz ćwiczenie 10. Policz na kartce, jaki kierunek shader odczyta z białego teksela i o ile stopni odchyla się on od normalnej. (`(1, 1, 1)`, po normalizacji około 55 stopni od osi z.)
12. **127 czy 128.** Zmień `HALF_BRIGHTNESS` na 127. Jaki kierunek odczyta teraz shader z płaskiej mapy i czy różnicę da się zobaczyć? Dlaczego żadna z tych dwóch wartości nie daje dokładnie zera? (`127 / 255 * 2 - 1` to około -0,004 zamiast +0,004. Zero wymagałoby bajtu 127,5.)
13. **Dwie przestrzenie, jeden plik.** W `TerrainRenderer.cpp` zamień `gfx::ColorSpace::Linear` przy `GROUND_NORMAL_MAP_FILE` na `Srgb`, zbuduj i uruchom. Czy w konsoli jest linia `Texture is asked for as sRGB and as linear`? Dlaczego nie (kto jeszcze prosi o `ground_normal.png`)? Co pokazuje panel Assets przy tym pliku i jak wygląda światło na podłożu? Potem pomyśl, jak wywołać ten komunikat naprawdę: który plik musiałby zostać poproszony dwa razy? (Tego ćwiczenia nikt jeszcze nie wykonał: odpowiedzi wynikają z kodu. O `ground_normal.png` prosi tylko teren, więc komunikatu nie ma, panel pokazuje `sRGB`, a normalne podłoża są przekrzywione.) Wycofaj zmianę.

## 9. Pytania kontrolne

1. **Po co pamięć podręczna assetów?**
   Żeby każdy plik był wczytany raz i istniał na karcie raz, niezależnie od tego, ile obiektów go używa, oraz żeby zasoby miały jednego właściciela, który je zwalnia.

2. **Co jest kluczem i jakie ma ograniczenia?**
   Ścieżka pliku po `lexically_normal`, czyli bez kroków `.` i `..`. Normalizacja działa na tekście: nie rozpoznaje dowiązań, różnej wielkości liter ani tego, że ścieżka względna i bezwzględna wskazują ten sam plik.

3. **Dlaczego `std::deque`, a nie `std::vector`?**
   Pamięć podręczna oddaje wskaźniki do swoich elementów. Wektor przy powiększaniu przenosi wszystkie elementy do nowego bloku i stare wskaźniki przestają być ważne. Kolejka dwustronna przy dopisywaniu na końcu nie rusza istniejących elementów, więc wskaźniki i referencje pozostają ważne.

4. **Co zwraca `model()`, gdy pliku nie ma?**
   `nullptr`. Błąd trafia do logu raz (pisze go loader), a ścieżka trafia na listę porażek i nie jest próbowana ponownie. Renderer pomija model, którego wskaźnik jest pusty.

5. **Co się dzieje, gdy model jest dobry, ale brakuje jego tekstury?**
   Model się wczytuje. Część, której tekstura zawiodła, dostaje wskaźnik na białą teksturę 1 x 1 i jest rysowana w kolorze materiału. `hasOwnTexture` zostaje fałszem.

6. **Dlaczego tekstura zastępcza jest biała?**
   Shader mnoży teksel przez kolor materiału. Biały teksel to jedynki, więc wynik to sam kolor materiału. Ten sam shader i ta sama pętla rysują części z teksturą i bez niej.

7. **Dlaczego tekstury zastępcze są pierwszymi polami klasy?**
   Pola giną w kolejności odwrotnej do deklaracji. Części modeli mogą na nie pokazywać, więc muszą zginąć po liście modeli. Z tego samego powodu lista tekstur stoi przed listą modeli.

8. **Dlaczego klasy nie da się skopiować ani przenieść?**
   Części modeli trzymają wskaźniki do tekstur tej samej pamięci podręcznej, w tym do białej tekstury i płaskiej mapy normalnych, które są polami trzymanymi przez wartość. Kopia albo obiekt przeniesiony miałyby wskaźniki do pól starego obiektu.

9. **Kiedy materiał części jest wyszukiwany po nazwie?**
   Raz, w `model()`, przy wczytaniu. W pętli rysowania część ma już gotowy kolor i gotowe wskaźniki do tekstury i do mapy normalnych.

10. **Jak działa wspólny filtr?**
    `setFilter` zapamiętuje filtr i ustawia go na każdej wczytanej teksturze. `texture()` nadaje zapamiętany filtr i anizotropię każdej nowej teksturze. Zmiana dotyczy obiektów samplera, więc nie wymaga podpinania ani ponownego wysyłania tekstur.

11. **Co pokazuje suwak `Anisotropy` na karcie bez rozszerzenia?**
    Jest wyszarzony (`BeginDisabled`), a pod nim stoi napis, że sterownik nie oferuje filtrowania anizotropowego. `maxAnisotropy()` zwraca wtedy 1.

12. **Dlaczego `ImGui::Image` dostaje UV `(0, 1)` i `(1, 0)`?**
    W projekcie `v = 0` to dolny wiersz tekstury, a ImGui domyślnie przyjmuje, że `v = 0` to górny wiersz obrazka. Lewy górny róg ma więc w konwencji OpenGL współrzędne `(0, 1)`, a prawy dolny `(1, 0)`. Bez tego podgląd byłby do góry nogami.

13. **Dlaczego podgląd nie zmienia się po przełączeniu filtra?**
    ImGui rysuje go własnym shaderem i z własnym obiektem samplera o filtrze liniowym, a sampler podpięty do jednostki ma pierwszeństwo. Filtr i anizotropię widać w scenie.

14. **Jak lista `VIEW_MODE_ITEMS` wiąże się z wyliczeniem `ViewMode` i z shaderem?**
    Pozycje listy są w kolejności wartości wyliczenia, więc numer wybranej pozycji jest wartością wyliczenia. Ta wartość jest wysyłana jako `uViewMode`, a shader fragmentów porównuje ją z liczbami 1 i 2.

15. **Dlaczego `AssetCache` nie ma testu jednostkowego?**
    Konstruktor tworzy dwie tekstury, a każde wczytanie tworzy siatkę albo teksturę: wszystko to wymaga kontekstu OpenGL. Testy bez okna mają loadery, z których pamięć podręczna korzysta. Samą klasę sprawdza uruchomienie programu.

16. **Czym dla pamięci podręcznej różni się mapa normalnych od zwykłej tekstury?**
    Dwiema rzeczami: teksturą zastępczą i, od M7, przestrzenią kolorów. Wczytuje ją ta sama funkcja `texture()`, ale z argumentem `gfx::ColorSpace::Linear` zamiast `Srgb`, więc na karcie ma format `GL_RGB8`, a nie `GL_SRGB8`. Poza tym trafia na tę samą listę, dostaje ten sam filtr i anizotropię i jest tak samo współdzielona między modelami. To, że jej bajty są kierunkami, wie shader i wołający, który wybrał przestrzeń.

17. **Co to jest płaska mapa normalnych i dlaczego ma teksel `(128, 128, 255)`?**
    Tekstura 1 x 1, która zastępuje brakującą mapę normalnych. Mapa zapisuje składową kierunku jako `bajt = (składowa * 0,5 + 0,5) * 255`, więc kierunek `(0, 0, 1)`, czyli "prosto z powierzchni", to `(127,5, 127,5, 255)`, po zaokrągleniu `(128, 128, 255)`. Shader odczytuje z niej normalną siatki, więc nie potrzebuje osobnej ścieżki dla części bez mapy.

18. **Dlaczego nie użyć białej tekstury także jako zastępczej mapy normalnych?**
    Bo biel oznacza w tym kodowaniu kierunek `(1, 1, 1)`, czyli normalną mocno przechyloną, a nie "brak zmiany". Biała tekstura jest neutralna dla mnożenia kolorów, płaska mapa dla zamiany normalnej.

19. **Co się dzieje, gdy brakuje pliku mapy normalnych?**
    Model się wczytuje. `texture()` wypisuje jedną linię `[error]` i zapamiętuje ścieżkę, a część zostaje przy płaskiej mapie: `hasOwnNormalMap` jest fałszem i panel pokazuje `normal map: none (flat)`. Powierzchnia ma teksturę koloru, ale jest oświetlona normalnymi siatki.

20. **Po co `texture()` ma argument `colorSpace` i kto go wybiera?**
    Mówi, czym jest obraz: kolorem (`Srgb`, format `GL_SRGB8`, dekodowany przez kartę przy odczycie) czy danymi (`Linear`, format `GL_RGB8`, czytany bez zmian). Wybiera wołający, bo tylko on wie, do czego użyje tekstury: `model()` podaje `Srgb` dla `map_Kd` i `Linear` dla `map_Bump`, `TerrainRenderer` tak samo dla swojej pary plików. Wartości domyślnej nie ma, żeby wyboru nie dało się pominąć.

21. **Co się dzieje, gdy ten sam plik zostanie poproszony w dwóch przestrzeniach?**
    Kluczem pamięci podręcznej jest sama ścieżka, więc plik powstaje na karcie raz, w przestrzeni z pierwszej prośby. Druga prośba wypisuje błąd `Texture is asked for as sRGB and as linear` i dostaje tę samą teksturę. Funkcja nie zwraca `nullptr` i nie dopisuje ścieżki do listy porażek.

22. **Dlaczego biała tekstura zastępcza jest sRGB, a płaska mapa normalnych liniowa?**
    Każda ma przestrzeń tego, co zastępuje. Dla bieli wybór nie zmienia wyniku (255 to 1,0 w obu przestrzeniach). Dla płaskiej mapy zmienia: bajt 128 zdekodowany jako sRGB dałby 0,216 zamiast 0,502 i kierunek przestałby być `(0, 0, 1)`.

23. **Dlaczego miniatury tekstur sRGB w panelu są rysowane przez `RawTextureSampler`?**
    ImGui rysuje prosto do okna i zapisuje to, co przeczyta. Tekstura sRGB oddaje wartości liniowe, których nikt już nie zakoduje, więc miniatura byłaby za ciemna. Sampler z wyłączonym dekodowaniem (`GL_EXT_texture_sRGB_decode`) oddaje bajty z pliku. Bez rozszerzenia miniatury są ciemniejsze.

20. **Co robi pole `Normal mapping` i dlaczego samo nie wystarcza, żeby relief było widać?**
    Ustawia `LightingSettings::normalMapping`. Shader dostaje jednak wynik `usesNormalMap`: pole włączone i tryb oświetlenia inny niż `Gouraud`. W trybie `Gouraud` światło jest liczone w wierzchołkach, więc normalna na teksel nie ma jak wziąć w nim udziału, a w trybie `Unlit` nie ma światła (mapy widać wtedy tylko w widoku `Normals as colour`).

21. **Ile tekstur pokazuje lista `Textures` i ile jest ich na karcie?**
    Lista pokazuje osiem: cztery obrazy koloru i cztery mapy normalnych. Na karcie jest dziesięć, bo dochodzą dwie tekstury zastępcze 1 x 1, które nie są na liście.

## 10. Źródła

- cppreference, `std::deque`: <https://en.cppreference.com/w/cpp/container/deque> (sekcja "Iterator invalidation": wstawianie na końcach nie unieważnia wskaźników ani referencji), `std::vector`: <https://en.cppreference.com/w/cpp/container/vector>.
- cppreference, `std::filesystem::path::lexically_normal`: <https://en.cppreference.com/w/cpp/filesystem/path/lexically_normal>.
- cppreference, reguła trzech, pięciu i zera: <https://en.cppreference.com/w/cpp/language/rule_of_three> (kiedy kompilator nie tworzy operacji przenoszących).
- LearnOpenGL, rozdział "Model": <https://learnopengl.com/Model-Loading/Model> (optymalizacja: tekstura wczytana raz i używana przez wiele siatek).
- LearnOpenGL, rozdział "Textures": <https://learnopengl.com/Getting-started/Textures> (filtry, mipmapy, zawijanie).
- Rozszerzenie `GL_EXT_texture_filter_anisotropic`: <https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt>.
- Dear ImGui, wiki "Image Loading and Displaying Examples": <https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples> (`ImGui::Image`, `ImTextureID`), oraz plik `backends/imgui_impl_opengl3.cpp` w pobranych źródłach (własne samplery backendu).
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `assets`), [`obj-loader.md`](obj-loader.md), [`images.md`](images.md), [`../gfx/mesh.md`](../gfx/mesh.md), [`../gfx/textures.md`](../gfx/textures.md), [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md) (mapy normalnych: teoria, shader, scenariusz pokazu), [`../game/maze-rendering.md`](../game/maze-rendering.md) i [`../game/gameplay.md`](../game/gameplay.md) (dwaj użytkownicy pamięci podręcznej), [`../debug-ui.md`](../debug-ui.md) (podpięcie panelu), [`../../libraries/imgui.md`](../../libraries/imgui.md).
- Rozszerzenie `GL_EXT_texture_sRGB_decode`: <https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_sRGB_decode.txt> (odczyt tekstury sRGB bez dekodowania, używany dla miniatur).
- Dokumenty z M7: [`../gfx/color-space.md`](../gfx/color-space.md) (sRGB i wartości liniowe), [`../debug-ui.md`](../debug-ui.md) (`RawTextureSampler`), notatka [`../../decisions/gamma-linear-pipeline.md`](../../decisions/gamma-linear-pipeline.md).
- LearnOpenGL, rozdział "Normal Mapping": <https://learnopengl.com/Advanced-Lighting/Normal-Mapping> (kodowanie kierunku w kolorze, skąd niebieski wygląd mapy).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 3 (tematy 4 i 5 i ich pokaz w ImGui, "toggle normal map").
