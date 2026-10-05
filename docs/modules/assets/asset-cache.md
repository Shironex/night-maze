# Moduł assets: pamięć podręczna modeli i tekstur, panel Assets

Kamień milowy: M2 + M3. Tematy wykładu: 4 (Wczytywanie OBJ: "cache meshy", lista załadowanych modeli) i 5 (Tekstury: podgląd tekstur, filtrowanie).
Kod: [`src/assets/AssetCache.hpp`](../../../src/assets/AssetCache.hpp), [`src/assets/AssetCache.cpp`](../../../src/assets/AssetCache.cpp), panel w [`src/debug/panels/AssetsPanel.hpp`](../../../src/debug/panels/AssetsPanel.hpp) i [`src/debug/panels/AssetsPanel.cpp`](../../../src/debug/panels/AssetsPanel.cpp), użytkownik: [`src/game/MazeRenderer.cpp`](../../../src/game/MazeRenderer.cpp).

Część modułu `assets`. Wstęp do modułu jest w [`README.md`](README.md). Ten dokument łączy cztery inne: [`obj-loader.md`](obj-loader.md) (plik OBJ i MTL jako dane procesora: `ObjModel`), [`images.md`](images.md) (plik PNG jako piksele: `Image`), [`../gfx/mesh.md`](../gfx/mesh.md) (siatka na karcie: `gfx::Mesh`) i [`../gfx/textures.md`](../gfx/textures.md) (tekstura na karcie: `gfx::Texture2D`, filtry, anizotropia, shadery `textured`). Kto z wczytanych modeli rysuje labirynt, opisuje [`../game/maze-rendering.md`](../game/maze-rendering.md).

## 1. Po co to jest

Loadery z poprzednich kroków zwracają **dane procesora**: wektory wierzchołków, indeksów i pikseli. Klasy `gfx` zamieniają takie dane na **obiekty karty graficznej**. Brakowało miejsca, które robi jedno i drugie po kolei, pamięta wynik i odpowiada na pytanie "daj mi model z tego pliku".

Tym miejscem jest klasa `assets::AssetCache`, **pamięć podręczna assetów** (asset cache). Robi pięć rzeczy:

1. wczytuje każdy model i każdą teksturę **raz**, przy pierwszej prośbie, i przy kolejnych oddaje ten sam obiekt,
2. oddaje wskaźniki, które **pozostają ważne** przez całe jej życie,
3. łączy model z jego materiałami: każda część siatki dostaje od razu kolor i teksturę,
4. znosi błędy bez zatrzymywania programu: brak pliku to linia w logu, wskaźnik pusty albo biała tekstura zastępcza,
5. trzyma jedno wspólne ustawienie filtra i anizotropii dla wszystkich tekstur.

Drugą połową dokumentu jest panel **Assets**: pokaz tematów 4 i 5 na obronie (lista modeli, podgląd tekstur, przełącznik filtra, suwak anizotropii, tryb widoku).

**Stan na dziś, uczciwie.** Gra wczytuje przez pamięć podręczną trzy modele i dwie tekstury. Na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik 610.74) build Debug i Release przechodzi bez ostrzeżeń, a program startuje bez linii `[error]`. Na zrzutach ekranu sprawdzone są: tekstury na ścianach i podłodze ustawione poprawnie, oba widoki diagnostyczne, porównanie filtrów (najbliższy sąsiad, dwuliniowy, trójliniowy, anizotropia 16x na ścianie oglądanej pod płaskim kątem), podglądy tekstur w panelu nieodwrócone oraz biała tekstura zastępcza z jedną linią `[error]` po usunięciu pliku tekstury. Stany filtra, anizotropii i trybu widoku były ustawiane tymczasowym kodem, który został usunięty: **widżetów panelu nikt jeszcze nie kliknął ręcznie**. Klasa nie ma testu jednostkowego, bo każda jej funkcja tworzy obiekty OpenGL. Na macOS kod nie był budowany ani uruchamiany.

## 2. Teoria

### 2.1 Dlaczego wczytywać raz

Słupek i ściana używają tej samej tekstury `wall_stone.png`. Bez pamięci podręcznej każdy model wczytałby ją osobno:

| | Bez pamięci podręcznej | Z pamięcią podręczną |
|---|---|---|
| odczyt i dekodowanie pliku PNG | 2 razy | 1 raz |
| tekstury 512 x 512 z mipmapami na karcie | 2 identyczne | 1 |
| zmiana filtra | trzeba pamiętać o obu | jedna lista |

Przy dwóch plikach to drobiazg. Zasada jest jednak ogólna i w większej grze decyduje o czasie wczytywania i o zajętości pamięci karty: **zasób identyfikuje plik, a nie ten, kto o niego prosi**. Ten sam schemat ma PRD w wierszu tematu 4 ("cache meshy").

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
| `std::map` albo `std::unordered_map` | tak (elementy węzłowe) | potrzebny dopiero przy tysiącach zasobów. Tutaj jest ich pięć |
| `std::deque<LoadedModel>` | tak, przy dopisywaniu na końcu | wybrane: najprostszy kod, kolejność wczytania zachowana dla panelu |

Wyszukiwanie to zwykłe przejście po liście i porównanie ścieżek. Przy kilku zasobach jest szybsze niż jakakolwiek tablica mieszająca i nie wymaga drugiej struktury danych.

### 2.4 Model na karcie: siatka i części

Loader oddaje jeden wspólny wektor wierzchołków i indeksów oraz listę **części**: zakresów indeksów, z których każdy ma inny materiał ([`obj-loader.md`](obj-loader.md), sekcja 2). Pamięć podręczna robi z tego:

- jedną siatkę `gfx::Mesh` z całością,
- dla każdej części rekord z tym, co potrzebne do narysowania: zakres indeksów, kolor rozproszenia materiału (linia `Kd` pliku MTL) i wskaźnik do tekstury (linia `map_Kd`).

Kolor i tekstura są wyszukiwane **raz, przy wczytaniu**. W pętli rysowania nie ma już szukania materiału po nazwie: jest gotowy wskaźnik i gotowy kolor.

Trzy modele gry mają po jednej części, każdą z kolorem białym i teksturą kamienia.

### 2.5 Błędy i biała tekstura zastępcza

Co może pójść źle i co wtedy robi pamięć podręczna:

| Sytuacja | Co wraca | Co widać |
|---|---|---|
| nie ma pliku OBJ albo ma błędną linię | `nullptr` | model nie jest rysowany, reszta sceny tak |
| plik OBJ nie ma ani jednej ściany | `nullptr` | to samo. Taki plik jest poprawny składniowo, ale nie ma czego rysować |
| model jest dobry, ale brakuje pliku jego tekstury | model, część z białą teksturą | powierzchnia w gładkim kolorze materiału |
| materiał nie ma linii `map_Kd` | model, część z białą teksturą | to samo: tak ma być, materiał jest jednokolorowy |
| obraz ma 1 albo 2 kanały (odcienie szarości) | `nullptr` dla tekstury, część z białą teksturą | gładki kolor |

Dlaczego **biała** tekstura, a nie osobny shader "bez tekstury". Shader fragmentów liczy `tekstura * uTint`. Biały teksel to `(1, 1, 1)`, a mnożenie przez jeden niczego nie zmienia: wychodzi sam kolor materiału. Jedna tekstura 1 x 1 pozwala więc rysować części z teksturą i bez niej **tym samym shaderem i tą samą pętlą**, bez instrukcji warunkowej w GLSL i bez drugiego programu.

Każda nieudana ścieżka jest **zapamiętywana**. Powody:

- błąd trafia do logu raz, a nie przy każdej prośbie,
- dysk nie jest odpytywany ponownie o plik, o którym wiadomo, że go nie ma,
- panel może pokazać listę nieudanych wczytań.

Cena: plik naprawiony w trakcie działania programu nie zostanie wczytany, dopóki program nie wystartuje od nowa. Przeładowania assetów na żywo (jak shaderów) pamięć podręczna nie ma.

### 2.6 Własność i kolejność niszczenia

Pamięć podręczna posiada obiekty OpenGL, więc obowiązują ją te same reguły co klasy `gfx` ([`../gfx/README.md`](../gfx/README.md), sekcja 2): potrzebuje kontekstu OpenGL przez całe życie i musi zostać zniszczona przed oknem.

W środku są wskaźniki **między jej własnymi polami**: części modeli pokazują na tekstury z listy tekstur albo na białą teksturę. Stąd dwie reguły:

1. **Kolejność pól.** Pola klasy są niszczone w kolejności odwrotnej do deklaracji. Biała tekstura i lista tekstur są zadeklarowane przed listą modeli, więc giną po niej: żadna część nie pokazuje nigdy na usuniętą teksturę.
2. **Zakaz kopiowania i przenoszenia.** Kopia miałaby części pokazujące na tekstury oryginału. Przeniesiona pamięć podręczna miałaby części pokazujące na białą teksturę starego obiektu, bo ta jest polem trzymanym przez wartość i zmienia adres. Klasa jest więc tworzona raz, w miejscu, i tam zostaje.

### 2.7 Jedno ustawienie filtra dla wszystkich tekstur

Każda `gfx::Texture2D` ma własny obiekt samplera z filtrem i poziomem anizotropii ([`../gfx/textures.md`](../gfx/textures.md), sekcja 2). Do pokazu wygodniej jest przełączać wszystkie naraz: pamięć podręczna pamięta wybrany filtr i poziom, ustawia je na każdej wczytanej teksturze i nadaje każdej teksturze wczytanej później. Dzięki temu tekstura wczytana po zmianie filtra nie wygląda inaczej niż pozostałe.

Przypomnienie, co oznaczają trzy filtry:

| Filtr | Z bliska (powiększenie) | Z daleka (pomniejszenie) |
|---|---|---|
| `Nearest` | ostre kwadraty tekseli | migotanie i ziarno |
| `Bilinear` | gładkie przejścia | nadal migotanie, bo mipmapy nie są używane |
| `Trilinear` | gładkie przejścia | spokojny obraz, ale rozmyty pod płaskim kątem |

Anizotropia poprawia właśnie ostatni przypadek: powierzchnię oglądaną pod płaskim kątem, na przykład podłogę i ścianę biegnącą w głąb korytarza.

## 3. Jak to działa w OpenGL

`AssetCache.cpp` nie woła bezpośrednio żadnej funkcji `gl*`. Tworzy obiekty klas `gfx`, a one wołają OpenGL. Co się dzieje na karcie przy starcie gry:

| Kiedy | Kod | Co powstaje na karcie |
|---|---|---|
| konstruktor `AssetCache` | `m_whiteTexture(1, 1, 3, ...)` | tekstura 1 x 1 `GL_RGB8` z jednym poziomem mipmap i jej obiekt samplera |
| `model(floor_tile.obj)` | `texture(floor_stone.png)`, potem `gfx::Mesh(...)` | tekstura 512 x 512 `GL_RGB8` z dziesięcioma poziomami mipmap i samplerem, potem VAO, bufor wierzchołków i bufor indeksów |
| `model(wall_straight.obj)` | `texture(wall_stone.png)`, potem `gfx::Mesh(...)` | druga tekstura 512 x 512 i druga siatka |
| `model(wall_pillar.obj)` | `texture(wall_stone.png)` oddaje teksturę już wczytaną, potem `gfx::Mesh(...)` | tylko trzecia siatka |

Razem: 3 tekstury (z białą), 3 obiekty samplera, 3 VAO i 6 buforów. Dane po stronie procesora (wektory z loaderów) są zwalniane zaraz po wysłaniu.

Zmiana filtra i anizotropii to `glSamplerParameteri` i `glSamplerParameterf` na obiekcie samplera każdej tekstury z listy ([`../gfx/textures.md`](../gfx/textures.md), sekcja 5). Tekstur nie trzeba do tego podpinać ani wysyłać ponownie.

Jeden skutek uboczny wart zapamiętania: utworzenie siatki podpina jej VAO i bufory, a utworzenie tekstury podpina ją do aktywnej jednostki. Wczytanie assetu **zmienia więc stan OpenGL**. Dlatego w `NightMazeApp` pamięć podręczna i renderer labiryntu stoją przed buforami kostki ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5).

## 4. Shadery

Pamięć podręczna nie ma własnego shadera. To, co przygotowuje, trafia do pary `textured.vert` i `textured.frag` ([`../gfx/textures.md`](../gfx/textures.md), sekcja 4):

| Pole `ModelPart` | Uniform | Linia w `textured.frag` |
|---|---|---|
| `texture` (podpinana do jednostki 0) | `uTexture` | `vec3 texel = texture(uTexture, vUv).rgb;` |
| `color` | `uTint` | `fragColor = vec4(texel * uTint, 1.0);` |

Z tych dwóch linii wynika cały mechanizm białej tekstury zastępczej: gdy `texel` to `(1, 1, 1)`, kolorem fragmentu jest samo `uTint`.

Tryb widoku z panelu Assets ustawia trzeci uniform, `uViewMode`: 0 to tekstura razy kolor, 1 to normalna jako kolor, 2 to współrzędne tekstury jako kolor.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/assets/AssetCache.hpp`](../../../src/assets/AssetCache.hpp) | struktury `LoadedTexture`, `ModelPart`, `LoadedModel` i klasa `AssetCache` |
| [`src/assets/AssetCache.cpp`](../../../src/assets/AssetCache.cpp) | stałe białej tekstury, funkcje pomocnicze `cacheKey` i `findMaterial`, definicje funkcji klasy |
| [`src/debug/panels/AssetsPanel.hpp`](../../../src/debug/panels/AssetsPanel.hpp), [`.cpp`](../../../src/debug/panels/AssetsPanel.cpp) | funkcja `debug::drawAssetsPanel` (sekcja 6) |
| [`src/game/MazeRenderer.cpp`](../../../src/game/MazeRenderer.cpp) | jedyny kod, który prosi o modele ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5) |
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) | właściciel: pole `m_assets`, akcesor `assets()` |

`AssetCache.*` należą do biblioteki `engine`. To pierwszy plik katalogu `src/assets/`, który dołącza nagłówki `gfx/` i tworzy obiekty OpenGL: loadery `ObjLoader` i `ImageLoader` nadal zwracają same dane procesora i nadal mają testy bez okna ([`README.md`](README.md)).

### 5.2 Trzy struktury danych

```cpp
struct LoadedTexture {
    /// Path of the image file, normalized. This is the key of the cache.
    std::filesystem::path path;

    /// The picture on the graphics card.
    gfx::Texture2D texture;
};
```

Tekstura razem ze swoim kluczem. `gfx::Texture2D` nie zna pliku, z którego powstała (przyjmuje gołe piksele), więc ścieżkę trzeba trzymać obok.

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

Wartość początkowa `texture = nullptr` w strukturze jest tylko po to, żeby pole nie było niezainicjalizowane. Komentarz "Never null" opisuje części, które oddaje pamięć podręczna: `model()` ustawia wskaźnik na białą teksturę, zanim zrobi cokolwiek innego. Renderer może więc pisać `part.texture->bind(...)` bez sprawdzania.

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
    /// Creates the 1 x 1 white texture that stands in for a missing one.
    AssetCache();

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;
```

Usunięcie konstruktora kopiującego i przypisania kopiującego wyłącza także przenoszenie: gdy klasa deklaruje operacje kopiujące (nawet jako usunięte), kompilator nie tworzy sam operacji przenoszących. Klasy nie da się więc ani skopiować, ani przenieść (sekcja 2.6).

Funkcje publiczne:

| Funkcja | Co robi |
|---|---|
| `const LoadedModel* model(const std::filesystem::path& path)` | model z pliku OBJ. Pierwsze wywołanie wczytuje, następne oddają ten sam obiekt. `nullptr` przy błędzie |
| `const gfx::Texture2D* texture(const std::filesystem::path& path)` | tekstura z pliku obrazu, na tych samych zasadach |
| `const gfx::Texture2D& whiteTexture() const` | biała tekstura 1 x 1 |
| `void setFilter(gfx::TextureFilter filter)` | filtr wszystkich tekstur, wczytanych i przyszłych |
| `void setAnisotropy(float level)` | poziom anizotropii wszystkich tekstur, przycięty do zakresu od 1 do `maxAnisotropy()` |
| `filter()`, `anisotropy()`, `maxAnisotropy()` | bieżące ustawienia i granica sterownika |
| `models()`, `textures()`, `failedPaths()` | listy do odczytu dla panelu |

Wskaźniki i referencje, które oddaje, są `const`: wołający może rysować i czytać, ale nie zmieni ani siatki, ani tekstury. Filtr zmienia się tylko przez pamięć podręczną.

`model` i `texture` nie są funkcjami `const`, bo mogą dopisać element do listy. Stąd `MazeRenderer` dostaje w konstruktorze referencję bez `const`.

Pola:

```cpp
    // The stand-in for a missing texture. Declared first, because the parts of the models
    // below may point at it.
    gfx::Texture2D m_whiteTexture;

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
| `m_whiteTexture` | pierwsze: powstaje przed wszystkim i ginie po wszystkim, bo części modeli mogą na nie pokazywać |
| `m_textures` | `std::deque` dla stabilnych wskaźników (sekcja 2.3). Przed `m_models`, żeby ginęło po nich |
| `m_models` | `std::deque` z tego samego powodu: renderer trzyma wskaźniki do elementów |
| `m_failedPaths` | zwykły `std::vector`: nikt nie trzyma wskaźników do jego elementów, a panel czyta całą listę przez referencję |
| `m_filter`, `m_anisotropy` | wartości startowe takie same jak w `Texture2D`: trójliniowy, anizotropia wyłączona (poziom 1) |

`maxAnisotropy()` pyta białą teksturę: `return m_whiteTexture.maxAnisotropy();`. Granica zależy od sterownika, a nie od tekstury, a biała tekstura istnieje zawsze, także gdy żadna inna się nie wczytała.

### 5.4 Konstruktor i funkcje pomocnicze

```cpp
// The white stand-in texture: one pixel of 3 bytes (red, green, blue), all at the maximum.
constexpr int WHITE_TEXTURE_SIZE = 1;
constexpr int WHITE_TEXTURE_CHANNELS = 3;
constexpr unsigned char FULL_BRIGHTNESS = 255;
constexpr std::array<unsigned char, WHITE_TEXTURE_CHANNELS> WHITE_PIXEL = {
    FULL_BRIGHTNESS, FULL_BRIGHTNESS, FULL_BRIGHTNESS};
```

```cpp
AssetCache::AssetCache()
    : m_whiteTexture(WHITE_TEXTURE_SIZE, WHITE_TEXTURE_SIZE, WHITE_TEXTURE_CHANNELS,
                     WHITE_PIXEL.data()) {}
```

Jeden piksel, trzy bajty po 255. `Texture2D` przyjmuje szerokość, wysokość, liczbę kanałów i wskaźnik na bajty, więc tekstura nie musi pochodzić z pliku. OpenGL kopiuje dane, a stała `WHITE_PIXEL` zostaje w programie.

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
    std::vector<ModelPart> parts;
    for (const ObjPart& sourcePart : source.parts) {
        ModelPart part;
        part.material = sourcePart.material;
        part.firstIndex = sourcePart.firstIndex;
        part.indexCount = sourcePart.indexCount;
        part.texture = &m_whiteTexture;

        // loadObj has checked that every named material exists. Faces without a material
        // (an empty name) keep the defaults: white colour, white texture.
        const ObjMaterial* material = findMaterial(source, sourcePart.material);
        if (material != nullptr) {
            part.color = material->diffuseColor;
            part.texturePath = material->diffuseTexture;
        }
        if (!part.texturePath.empty()) {
            // texture() logs a failed load. The part then keeps the white texture and is
            // drawn in its plain colour.
            const gfx::Texture2D* texture = this->texture(part.texturePath);
            if (texture != nullptr) {
                part.texture = texture;
                part.hasOwnTexture = true;
            }
        }
        parts.push_back(std::move(part));
    }
```

| Linia | Znaczenie |
|---|---|
| `part.texture = &m_whiteTexture;` | najpierw wartość bezpieczna. Każda dalsza gałąź może ją tylko poprawić na lepszą |
| `findMaterial(source, sourcePart.material)` | materiał części. `nullptr` tylko dla części bez nazwy materiału: nazwy nieistniejące odrzucił już `loadObj` |
| `part.color = material->diffuseColor;` | kolor `Kd`, biały, gdy plik MTL go nie podał |
| `part.texturePath = material->diffuseTexture;` | ścieżka obrazu, już złożona przez loader z katalogu pliku MTL i wpisu `map_Kd` |
| `this->texture(part.texturePath)` | prośba do **tej samej** pamięci podręcznej. Druga część albo drugi model z tą samą teksturą dostanie ten sam wskaźnik. `this->` jest potrzebne, bo zmienna lokalna w tej samej linii też nazywa się `texture` i zasłania funkcję |
| `if (texture != nullptr)` | tekstura się wczytała: część dostaje ją i znacznik `hasOwnTexture`. W przeciwnym razie zostaje biała |
| `parts.push_back(std::move(part));` | przeniesienie, żeby nie kopiować napisu i ścieżki |

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
const gfx::Texture2D* AssetCache::texture(const std::filesystem::path& path) {
    const std::filesystem::path key = cacheKey(path);

    for (const LoadedTexture& loaded : m_textures) {
        if (loaded.path == key) {
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

Ten sam początek co w `model()`: klucz, szukanie wśród wczytanych, sprawdzenie listy porażek, loader. `loadImage` dekoduje plik do pikseli z dolnym wierszem na początku ([`images.md`](images.md), sekcja 5).

```cpp
    // The constructor logs an error and leaves the texture not valid when the picture
    // has a channel count it does not accept (grey pictures have 1 or 2 channels).
    gfx::Texture2D texture(image.width, image.height, image.channels, image.pixels.data());
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
| `gfx::Texture2D texture(...)` | tworzy teksturę i mipmapy na karcie. Obiekt jest na razie zmienną lokalną |
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
| pętla po `m_textures` | ustawienie na każdej wczytanej. Biała tekstura nie jest na liście i zostaje przy ustawieniach startowych: ma jeden teksel, więc filtr niczego by w niej nie zmienił |
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
```

`m_assets` nie ma wpisu w liście inicjalizacyjnej konstruktora (wystarcza konstruktor domyślny), a `m_mazeRenderer(m_assets)` dostaje do niej referencję i od razu prosi o trzy modele. Jako pole klasy pochodnej od `core::Application` pamięć podręczna powstaje po oknie i ginie przed nim, więc kontekst OpenGL istnieje przez całe jej życie. Panel dostaje ją przez akcesor:

```cpp
    /// The loaded models and textures, exposed so the debug UI can list them and change
    /// the texture filtering live.
    assets::AssetCache& assets() { return m_assets; }
```

### 5.9 Jak to zostało sprawdzone

Klasa nie ma testu jednostkowego: konstruktor tworzy teksturę, więc bez kontekstu OpenGL nie da się jej nawet utworzyć. Części składowe mają własne testy bez okna: 18 przypadków loadera OBJ i 7 przypadków loadera obrazów, oba na prawdziwych plikach gry.

Sprawdzenie na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik 610.74) przez uruchomienie programu i zrzuty ekranu:

| Sprawdzenie | Wynik |
|---|---|
| start programu | brak linii `[error]` i brak linii `GL_` |
| widok startowy w labiryncie | tekstury na ścianach, słupkach i podłodze ustawione poprawnie: nie do góry nogami i nie w lustrze |
| tryb widoku: normalne jako kolor, współrzędne tekstury jako kolor | oba obrazy zgodne z oczekiwaniem |
| filtr: najbliższy sąsiad, dwuliniowy, trójliniowy, trójliniowy z anizotropią 16x | różnice widoczne na ścianie oglądanej pod płaskim kątem |
| podglądy tekstur w panelu | nieodwrócone |
| brak pliku tekstury | powierzchnia w białym kolorze materiału, jedna linia `[error]` |

Stany filtra, anizotropii i trybu widoku były ustawiane tymczasowym kodem (usuniętym), a nie kliknięciem w panel. Ręczne przejście przez widżety jest otwartą pozycją listy kontrolnej w [`../../guides/build-windows.md`](../../guides/build-windows.md). Na macOS nic z tego nie było sprawdzane ([`../../guides/build-macos.md`](../../guides/build-macos.md)).

## 6. Panel ImGui

Panel **Assets** jest pokazem tematów 4 i 5. PRD (sekcja 3) wymienia dla tematu 4 "Lista załadowanych modeli", a dla tematu 5 "Podgląd tekstur, toggle normal map". Lista modeli i podgląd tekstur są. Przełącznika map normalnych nie ma, bo map normalnych jeszcze nie ma (M4). Osobnego panelu o tej nazwie PRD nie przewiduje: nazwa Assets pochodzi z kodu.

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
void drawAssetsPanel(assets::AssetCache& assets, game::ViewMode& viewMode) {
    // First run only: the right edge of the window, below the Maze panel (the constant
    // is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(ASSETS_PLACEMENT);
    if (ImGui::Begin("Assets")) {
        drawSettings(assets, viewMode);
        drawModels(assets);
        drawTextures(assets);
        drawFailures(assets);
    }
    ImGui::End();
}
```

Panel dostaje pamięć podręczną bez `const`, bo dwa widżety wołają `setFilter` i `setAnisotropy`. Trzy z czterech funkcji pomocniczych przyjmują ją już jako `const`: tylko czytają listy. Podział na cztery funkcje odpowiada czterem częściom panelu.

### 6.2 Przełączniki: `drawSettings`

```cpp
    int viewModeIndex = static_cast<int>(viewMode);
    if (ImGui::Combo("View mode", &viewModeIndex, VIEW_MODE_ITEMS)) {
        viewMode = static_cast<game::ViewMode>(viewModeIndex);
    }

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
| `assets.setFilter(...)` | wołane tylko przy zmianie, a nie w każdej klatce: funkcja przechodzi po wszystkich teksturach i woła OpenGL |

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

Dla gry lista ma trzy modele, każdy z jedną częścią. Liczby trójkątów to 2 dla płytki i po 30 dla ściany i słupka. To jest pokaz tematu 4: trzy listy indeksów pliku OBJ zamienione na jedną siatkę z podziałem na materiały.

`drawFailures` nie rysuje nic, gdy lista porażek jest pusta. W przeciwnym razie pokazuje nagłówek `Failed to load` i nazwy plików na czerwono (`PushStyleColor` i `PopStyleColor` wokół pętli).

### 6.4 Podgląd tekstur: `drawTextures` i odwrócone UV

```cpp
void drawTextures(const assets::AssetCache& assets) {
    ImGui::SeparatorText("Textures");
    for (const assets::LoadedTexture& loaded : assets.textures()) {
        drawFileName(loaded.path);
        ImGui::Text("  %d x %d px", loaded.texture.width(), loaded.texture.height());

        const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
        ImGui::Image(textureId, {PREVIEW_SIZE, PREVIEW_SIZE}, {0.0F, 1.0F}, {1.0F, 0.0F});
    }
}
```

W pliku nad dwiema ostatnimi liniami stoi dłuższy komentarz po angielsku: wyjaśnia rzutowanie, odwrócone współrzędne i własny sampler ImGui.

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

Kroki z klikaniem nie były jeszcze wykonane ręcznie. Kolumna "co widać" opisuje to, co wynika z kodu shadera i klas. Na zrzutach ekranu z Windowsa, gdzie te same stany były ustawione kodem, sprawdzone są: oba widoki diagnostyczne, porównanie filtrów z anizotropią 16x, nieodwrócone podglądy i biała tekstura zastępcza (sekcja 5.9). Wrażeń z ruchu (migotanie) na nieruchomych zrzutach ocenić się nie da.

| Widżet | Co robię | Co widać | Co to pokazuje (temat) |
|---|---|---|---|
| lista `Models` | czytam trzy wpisy: liczby wierzchołków i trójkątów, część `wall_stone` albo `floor_stone` z nazwą tekstury | 2 trójkąty płytki, po 30 ściany i słupka | 4: wynik parsera OBJ i MTL, jedna siatka z podziałem na materiały |
| lista `Textures` | pokazuję dwa wpisy 512 x 512 i ich podglądy | dwie tekstury, choć modele są trzy | 4 i 5: tekstura wspólna dla ściany i słupka jest na karcie raz |
| podgląd | najeżdżam na nazwę pliku | pełna ścieżka w dymku. Obraz nie jest do góry nogami | 5: `v = 0` na dole, odwrócone UV w `ImGui::Image` |
| `Filter` | staję przed ścianą, potem patrzę wzdłuż długiego korytarza i przełączam `Nearest`, `Bilinear`, `Trilinear` | z bliska `Nearest` daje kwadraty tekseli, z daleka `Nearest` i `Bilinear` migoczą w ruchu, `Trilinear` jest spokojny | 5: filtr powiększenia i pomniejszenia, mipmapy |
| `Anisotropy` | przy `Trilinear` patrzę na podłogę albo ścianę pod płaskim kątem i przesuwam suwak od 1x do maksimum | rozmyty pas w głębi staje się ostry | 5: filtrowanie anizotropowe jako rozszerzenie |
| `View mode`: `UVs as colour` | przełączam i podchodzę do ściany | czerwień rośnie wzdłuż u, zieleń wzdłuż v, wzór zaczyna się od czerni tam, gdzie tekstura się powtarza | 5: współrzędne tekstury i zawijanie `GL_REPEAT` |
| `View mode`: `Normals as colour` | przełączam i rozglądam się | kolor to `normalna * 0,5 + 0,5`: podłoga zielonkawa (normalna +Y), ściana zwrócona w stronę +X czerwonawa, w stronę +Z niebieskawa, a zwrócone w strony przeciwne są w tym kanale ciemne (-X wychodzi morska, -Z oliwkowa) | 4: normalne z pliku OBJ, obrócone razem z modelem |
| `Failed to load` | przed uruchomieniem zmieniam nazwę pliku tekstury w skopiowanym katalogu `assets` | biała powierzchnia zamiast kamienia, jedna linia `[error]` w konsoli, czerwony wpis w panelu, a przy części napis `no texture (white)` | 4 i 5: obsługa błędów bez zatrzymania programu, tekstura zastępcza |

Dobra kolejność: najpierw listy (co jest wczytane), potem `Filter` i `Anisotropy` przy tym samym ujęciu korytarza, na końcu dwa widoki diagnostyczne.

## 7. Pułapki

1. **`std::vector` zamiast `std::deque`.** Program działa, dopóki wektor nie przekroczy pojemności. Potem renderer rysuje ze wskaźnika na zwolnioną pamięć. Błąd nie zależy od kodu, który go wywołał, tylko od liczby wczytanych zasobów, więc pojawia się dużo później niż zmiana, która go wprowadziła.
2. **Usuwanie albo wstawianie w środku `std::deque`.** Gwarancja stabilnych wskaźników dotyczy tylko dopisywania na początku albo na końcu. Funkcja "usuń zasób" unieważniłaby wskaźniki do pozostałych.
3. **Odwrócona kolejność pól.** Lista modeli zadeklarowana przed listą tekstur ginęłaby po niej. Dziś nic by się nie stało (destruktor części nie dotyka tekstury), ale reguła "wskazywany żyje dłużej niż wskazujący" przestałaby obowiązywać i pierwszy destruktor, który sięgnie po teksturę, trafiłby na usunięty obiekt.
4. **Przechowanie wskaźnika dłużej niż pamięć podręczna.** `MazeRenderer` trzyma gołe wskaźniki. Poprawność zależy od kolejności pól w `NightMazeApp`, a nie od typu wskaźnika.
5. **Ten sam plik pod dwiema ścieżkami.** Normalizacja jest leksykalna: różna wielkość liter albo ścieżka względna i bezwzględna dają dwa klucze i dwa wczytania.
6. **Naprawiony plik nie wraca.** Ścieżka, która raz zawiodła, nie jest próbowana ponownie. Po przywróceniu brakującej tekstury trzeba uruchomić program od nowa.
7. **Brak tekstury wygląda jak zamierzony kolor.** Biała tekstura zastępcza sprawia, że program działa dalej, ale też że błąd łatwo przeoczyć. Śladem jest linia `[error]`, wpis `Failed to load` i napis `no texture (white)` przy części.
8. **Obraz w odcieniach szarości.** Dekoduje się poprawnie, ale ma 1 albo 2 kanały, a `Texture2D` przyjmuje 3 albo 4. Skutek jest taki jak przy braku pliku. Tekstury trzeba zapisywać jako RGB albo RGBA.
9. **Wczytanie assetu w środku rysowania.** `model()` i `texture()` podpinają VAO, bufory i teksturę. Wywołane między ustawieniem stanu a `glDrawElements` innego obiektu zepsułyby ten stan. Gra prosi o modele tylko w konstruktorze renderera.
10. **Podgląd z domyślnymi UV.** `ImGui::Image(id, rozmiar)` bez dwóch ostatnich argumentów pokazuje teksturę do góry nogami, bo ImGui liczy v od góry, a projekt od dołu.
11. **Ocenianie filtra po podglądzie.** Podgląd rysuje ImGui własnym samplerem liniowym. Filtr i anizotropię widać tylko w scenie.
12. **Kolejność pozycji listy a wyliczenie.** `VIEW_MODE_ITEMS` i `FILTER_ITEMS` muszą mieć pozycje w kolejności wartości wyliczeń. Nowa wartość dopisana w środku wyliczenia przesuwa wszystkie następne i lista zaczyna wybierać nie to, co pokazuje.
13. **`setFilter` w każdej klatce.** `Combo` zwraca prawdę tylko przy zmianie i tylko wtedy kod woła `setFilter`. Wołanie bezwarunkowe ustawiałoby parametry samplerów wszystkich tekstur w każdej klatce, bez żadnego skutku.

## 8. Ćwiczenia

Ćwiczenia od 1 do 3 robi się na kartce. Pozostałe to zmiany w kodzie albo w skopiowanych assetach: po zmianie kodu zbuduj projekt (`cmake --build --preset debug`) i uruchom program, a na końcu wycofaj zmianę (`git checkout src`). Ćwiczeń od 4 do 9 nie wykonywałem: opisy skutków wynikają z czytania kodu, poza ćwiczeniem 4, którego skutek jest na zrzucie ekranu z Windowsa.

1. **Ile obiektów.** Gra prosi o `floor_tile.obj`, `wall_straight.obj` i `wall_pillar.obj`. Ile razy wołana jest funkcja `texture()` i ile tekstur powstaje na karcie (z białą)? Odpowiedź: 3 wywołania (po jednym na część), 2 wczytania, 3 tekstury.
2. **Klucz.** Co zwraca `lexically_normal` dla `C:/gra/assets/models/../textures/./wall_stone.png`? Odpowiedź: `C:/gra/assets/textures/wall_stone.png` (na Windowsie z odwrotnymi ukośnikami).
3. **Kolor części.** Materiał ma `Kd 1.0 0.5 0.0` i nie ma `map_Kd`. Jaki kolor ma fragment? Odpowiedź: `(1, 1, 1) * (1, 0,5, 0)`, czyli pomarańczowy `(1, 0,5, 0)`: biały teksel razy `uTint`.
4. **Brak tekstury.** W katalogu `assets` obok pliku wykonywalnego (na Windowsie `build/debug/Debug/assets`) zmień nazwę `wall_stone.png` i uruchom program. Ile linii `[error]` jest w konsoli, choć tekstury potrzebują dwa modele? Co pokazuje panel Assets? Przywróć nazwę (albo odśwież kopię: `cmake --build --preset debug --target copy_assets`).
5. **Wektor zamiast kolejki.** Zamień `std::deque<LoadedModel>` na `std::vector<LoadedModel>` (i typ zwracany przez `models()`). Czy program od razu przestaje działać przy trzech modelach? Od czego to zależy? Dopisz przed pierwszym wczytaniem `m_models.reserve(1)` i sprawdź ponownie. Dlaczego taki błąd jest groźniejszy niż błąd kompilacji?
6. **Kolorowa tekstura zastępcza.** Zmień `WHITE_PIXEL` na jaskrawy róż `{255, 0, 255}` i powtórz ćwiczenie 4. Co zyskujesz przy szukaniu brakujących tekstur, a co tracisz dla materiałów, które celowo nie mają tekstury?
7. **Licznik wczytań.** Dodaj tymczasowo `core::logInfo` na początku `texture()` z kluczem. Ile linii pojawia się przy starcie i które z nich są trafieniami w pamięć podręczną?
8. **Podgląd do góry nogami.** W `drawTextures` usuń dwa ostatnie argumenty `ImGui::Image`. Porównaj podgląd z teksturą na ścianie. Potem zamień tylko u (`{1.0F, 1.0F}`, `{0.0F, 0.0F}`): co się zmieniło?
9. **Filtr na jednej teksturze.** Dodaj do pamięci podręcznej funkcję, która ustawia filtr tylko tekstury o podanej ścieżce, i wywołaj ją tymczasowo dla `floor_stone.png` z filtrem `Nearest`. Dlaczego wspólne pole `m_filter` przestaje wtedy opisywać stan wszystkich tekstur i co powinien pokazywać panel?

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

7. **Dlaczego biała tekstura jest pierwszym polem klasy?**
   Pola giną w kolejności odwrotnej do deklaracji. Części modeli mogą na nią pokazywać, więc musi zginąć po liście modeli. Z tego samego powodu lista tekstur stoi przed listą modeli.

8. **Dlaczego klasy nie da się skopiować ani przenieść?**
   Części modeli trzymają wskaźniki do tekstur tej samej pamięci podręcznej, w tym do białej tekstury, która jest polem trzymanym przez wartość. Kopia albo obiekt przeniesiony miałyby wskaźniki do pól starego obiektu.

9. **Kiedy materiał części jest wyszukiwany po nazwie?**
   Raz, w `model()`, przy wczytaniu. W pętli rysowania część ma już gotowy kolor i gotowy wskaźnik do tekstury.

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
    Konstruktor tworzy teksturę, a każde wczytanie tworzy siatkę albo teksturę: wszystko to wymaga kontekstu OpenGL. Testy bez okna mają loadery, z których pamięć podręczna korzysta. Samą klasę sprawdza uruchomienie programu.

## 10. Źródła

- cppreference, `std::deque`: <https://en.cppreference.com/w/cpp/container/deque> (sekcja "Iterator invalidation": wstawianie na końcach nie unieważnia wskaźników ani referencji), `std::vector`: <https://en.cppreference.com/w/cpp/container/vector>.
- cppreference, `std::filesystem::path::lexically_normal`: <https://en.cppreference.com/w/cpp/filesystem/path/lexically_normal>.
- cppreference, reguła trzech, pięciu i zera: <https://en.cppreference.com/w/cpp/language/rule_of_three> (kiedy kompilator nie tworzy operacji przenoszących).
- LearnOpenGL, rozdział "Model": <https://learnopengl.com/Model-Loading/Model> (optymalizacja: tekstura wczytana raz i używana przez wiele siatek).
- LearnOpenGL, rozdział "Textures": <https://learnopengl.com/Getting-started/Textures> (filtry, mipmapy, zawijanie).
- Rozszerzenie `GL_EXT_texture_filter_anisotropic`: <https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt>.
- Dear ImGui, wiki "Image Loading and Displaying Examples": <https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples> (`ImGui::Image`, `ImTextureID`), oraz plik `backends/imgui_impl_opengl3.cpp` w pobranych źródłach (własne samplery backendu).
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `assets`), [`obj-loader.md`](obj-loader.md), [`images.md`](images.md), [`../gfx/mesh.md`](../gfx/mesh.md), [`../gfx/textures.md`](../gfx/textures.md), [`../game/maze-rendering.md`](../game/maze-rendering.md) (użytkownik pamięci podręcznej), [`../debug-ui.md`](../debug-ui.md) (podpięcie panelu), [`../../libraries/imgui.md`](../../libraries/imgui.md).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 3 (tematy 4 i 5 i ich pokaz w ImGui).
