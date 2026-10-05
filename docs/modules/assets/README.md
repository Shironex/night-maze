# Moduł assets: z pliku na dysku do danych w pamięci i na kartę

Kamień milowy: M2 + M3, zaktualizowany w M4 (mapy normalnych: styczne, linia `map_Bump`, płaska mapa zastępcza). Temat wykładu: 4 (Wczytywanie OBJ) i część tematu 5 (Tekstury: wczytanie obrazu, pamięć podręczna tekstur, panel z podglądem, mapy normalnych).
Kod: [`src/assets/`](../../../src/assets/), pliki wejściowe w [`assets/`](../../../assets/), testy w [`tests/`](../../../tests/).

Moduł `gfx` umie wysłać na kartę graficzną to, co dostanie: tablicę wierzchołków, tablicę indeksów, tablicę pikseli. Nie wie, skąd te tablice pochodzą. Do tej pory pochodziły z kodu: kostka jest wpisana liczba po liczbie w `NightMazeApp.cpp`. Moduł `assets` jest drugim źródłem: **czyta pliki** z katalogu `assets/` (modele `.obj` z materiałami `.mtl`, obrazy `.png`) i zamienia je na zwykłe dane w pamięci procesora. Trzeci element modułu, pamięć podręczna assetów, robi z tych danych obiekty na karcie graficznej i pilnuje, żeby każdy plik był wczytany raz.

Uwaga na dwie różne rzeczy o tej samej nazwie:

| Nazwa | Co to jest |
|---|---|
| katalog `assets/` w korzeniu repozytorium | **pliki danych**: modele, tekstury, shadery. Nie są kompilowane. Krok budowania umieszcza katalog obok programu ([`../core/paths.md`](../core/paths.md)) |
| katalog `src/assets/` i przestrzeń nazw `assets` | **kod C++**, który te pliki czyta. Ten dokument jest o nim |

**Stan.** Moduł ma dwa loadery (modeli OBJ i obrazów), funkcje liczące styczne wierzchołków (`Tangents`) oraz pamięć podręczną assetów `assets::AssetCache`. Wszystko to jest częścią biblioteki `engine`. Loadery i funkcje stycznych mają testy jednostkowe. Pamięć podręczna ich nie ma, bo tworzy obiekty OpenGL. Program z nich korzysta: przy starcie `game::MazeRenderer` prosi pamięć podręczną o trzy modele labiryntu, a ta wczytuje je loaderem OBJ razem z czterema teksturami: dwoma obrazami koloru i dwiema mapami normalnych. Na Windowsie (2026-10-05) program startuje bez linii `[error]`, a tekstury i relief z map normalnych na ścianach, słupkach i podłodze są sprawdzone na zrzutach ekranu. Na macOS kod nie był budowany ani uruchamiany.

**Co doszło z mapami normalnych (druga część M4).** Loader OBJ czyta linię `map_Bump` pliku MTL i liczy styczne po wczytaniu geometrii. Pamięć podręczna daje każdej części modelu mapę normalnych: własną albo płaską zastępczą. Panel Assets dostał pole `Normal mapping`. Temat jako całość (teoria, shader, styczne linia po linii) ma własny dokument w module `gfx`: [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md). Dokumenty tego modułu opisują swoją część: parser w [`obj-loader.md`](obj-loader.md), pliki PNG i ich testy w [`images.md`](images.md), mapę zastępczą i panel w [`asset-cache.md`](asset-cache.md). Ten plik jest wstępem do modułu: indeks dokumentów i plików, wspólna zasada i miejsce w warstwach.

## 1. Dokumenty modułu

| Dokument | Temat wykładu | Co opisuje | Funkcje i pliki |
|---|---|---|---|
| [`obj-loader.md`](obj-loader.md) | 4. Wczytywanie OBJ | format OBJ i MTL linia po linii, trzy listy indeksów a jeden indeks OpenGL, mapa trójek, indeksy ujemne, triangulacja wachlarzem, kierunek nawijania, układ współrzędnych, czytanie liczb niezależnie od locale, zgłaszanie błędów z numerem linii, linia mapy normalnych `map_Bump` i jej opcja `-bm`, styczne liczone na końcu `parseObj`, licznik trójkątów z odbitą teksturą, testy na prawdziwych modelach gry | `parseObj`, `parseMtl`, `loadObj`, `ObjModel` |
| [`images.md`](images.md) | 5. Tekstury | wczytanie pliku obrazu do tablicy pikseli: loader obrazów oparty na bibliotece stb_image. Kolejność wierszy a zielony kanał mapy normalnych, testy na plikach map normalnych | `loadImage`, `Image` |
| [`asset-cache.md`](asset-cache.md) | 4 i 5 | pamięć podręczna: wczytywanie raz, klucz ze znormalizowanej ścieżki, stabilne wskaźniki i `std::deque`, model jako siatka z częściami, błędy i dwie tekstury zastępcze (biała i płaska mapa normalnych), własność i kolejność niszczenia, wspólny filtr i anizotropia, panel Assets z podglądem tekstur i przełącznikiem `Normal mapping` | `AssetCache`, `LoadedModel`, `ModelPart`, `LoadedTexture`, `drawAssetsPanel` |
| [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md) (dokument modułu `gfx`) | 5. Tekstury | mapy normalnych jako całość. Z kodu tego modułu omawia linia po linii pliki `Tangents.hpp` i `Tangents.cpp` | `triangleTangents`, `computeTangents`, `countMirroredTriangles` |

Każdy dokument tematyczny ma te same dziesięć sekcji co dokumenty pozostałych modułów: Po co to jest, Teoria, Jak to działa w OpenGL, Shadery, Kod w projekcie, Panel ImGui, Pułapki, Ćwiczenia, Pytania kontrolne, Źródła.

Proponowana kolejność czytania: [`../../guides/blender.md`](../../guides/blender.md) (skąd są pliki i co w nich jest), ten plik, [`obj-loader.md`](obj-loader.md), potem [`../gfx/mesh.md`](../gfx/mesh.md) (dokąd trafiają wierzchołki i indeksy), [`images.md`](images.md) i [`../gfx/textures.md`](../gfx/textures.md) (dokąd trafiają piksele), na końcu [`asset-cache.md`](asset-cache.md), który łączy wszystkie cztery. Wcześniej warto znać [`../gfx/buffers-vao.md`](../gfx/buffers-vao.md) i [`../gfx/indexed-drawing.md`](../gfx/indexed-drawing.md): czym jest wierzchołek, indeks i bufor.

## 2. Indeks: plik kodu, dokument

| Plik kodu | Co zawiera | Dokument |
|---|---|---|
| [`src/assets/ObjLoader.hpp`](../../../src/assets/ObjLoader.hpp), [`.cpp`](../../../src/assets/ObjLoader.cpp) | struktury `ObjPart`, `ObjMaterial`, `ObjModel`, funkcje `parseObj` (tekst OBJ), `parseMtl` (tekst MTL) i `loadObj` (plik OBJ razem z plikami MTL, rozwiązanie ścieżek) | [`obj-loader.md`](obj-loader.md), sekcja 5 |
| [`src/assets/Tangents.hpp`](../../../src/assets/Tangents.hpp), [`.cpp`](../../../src/assets/Tangents.cpp) | funkcje `triangleTangents`, `computeTangents` i `countMirroredTriangles`: styczna każdego wierzchołka z pozycji i współrzędnych uv trójkątów. Sama matematyka, bez OpenGL. Woła je `parseObj` | [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcje od 5.5 do 5.7 |
| [`src/assets/ImageLoader.hpp`](../../../src/assets/ImageLoader.hpp), [`.cpp`](../../../src/assets/ImageLoader.cpp) | struktura `Image` i funkcja `loadImage` | [`images.md`](images.md) |
| [`src/assets/AssetCache.hpp`](../../../src/assets/AssetCache.hpp), [`.cpp`](../../../src/assets/AssetCache.cpp) | struktury `LoadedTexture`, `ModelPart`, `LoadedModel` i klasa `AssetCache`: `model`, `texture`, `whiteTexture`, `flatNormalTexture`, `setFilter`, `setAnisotropy` | [`asset-cache.md`](asset-cache.md), sekcja 5 |
| [`src/debug/panels/AssetsPanel.hpp`](../../../src/debug/panels/AssetsPanel.hpp), [`.cpp`](../../../src/debug/panels/AssetsPanel.cpp) | `debug::drawAssetsPanel`: panel "Assets". Należy do programu `night_maze`, nie do `engine` | [`asset-cache.md`](asset-cache.md), sekcja 6 |
| [`src/gfx/Vertex.hpp`](../../../src/gfx/Vertex.hpp) | struktura `gfx::Vertex`, którą loader OBJ wypełnia. Należy do `gfx/`, ale jest wspólnym formatem obu modułów | [`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5.2 |
| [`tests/ObjLoaderTests.cpp`](../../../tests/ObjLoaderTests.cpp) | 20 przypadków testowych loadera OBJ | [`obj-loader.md`](obj-loader.md), sekcja 5.9 |
| [`tests/TangentTests.cpp`](../../../tests/TangentTests.cpp) | 9 przypadków testowych funkcji z `Tangents` | [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), sekcja 5.10 |
| [`tests/ImageLoaderTests.cpp`](../../../tests/ImageLoaderTests.cpp) | 9 przypadków testowych loadera obrazów, w tym dwa na plikach map normalnych | [`images.md`](images.md), sekcja 5.7 |
| [`assets/models/`](../../../assets/models/), [`assets/textures/`](../../../assets/textures/) | pliki wejściowe: trzy modele z materiałami i cztery tekstury (`wall_stone.png`, `floor_stone.png` i ich mapy normalnych `wall_stone_normal.png`, `floor_stone_normal.png`) | [`../../guides/blender.md`](../../guides/blender.md), sekcje 5, 7 i 8 |

## 3. Wspólna zasada loaderów: wynik to dane procesora, bez OpenGL

Loader **nie tworzy żadnego obiektu OpenGL**. Zwraca zwykłe struktury z wektorami: wierzchołki, indeksy, piksele. To samo dotyczy funkcji z `Tangents`: dostają i oddają tablice w pamięci procesora. Obiekt na karcie graficznej (siatkę, teksturę) tworzy z nich dopiero klasa z modułu `gfx`.

```mermaid
flowchart LR
    Disk["plik na dysku<br/>.obj, .mtl, .png"] -->|"assets: czytanie i rozbiór"| Cpu["dane w pamięci procesora<br/>ObjModel, Image"]
    Cpu -->|"gfx: glBufferData, glTexImage2D"| Gpu["obiekt na karcie graficznej<br/>Mesh, Texture2D"]
```

Obie strzałki wykonuje po kolei `AssetCache`: woła loader, a z jego wyniku tworzy `gfx::Mesh` albo `gfx::Texture2D` i zapamiętuje go pod ścieżką pliku. To jedyny plik w `src/assets/`, który wymaga kontekstu OpenGL, i dlatego jedyny bez testu jednostkowego ([`asset-cache.md`](asset-cache.md)).

| Krok | Moduł | Wymaga kontekstu OpenGL | Da się testować bez okna |
|---|---|---|---|
| plik na dane | `assets` | nie | tak |
| dane na obiekt karty | `gfx` | tak | nie |

Po co ten podział:

1. **Testy.** Program testowy nie tworzy okna. Rozbiór pliku to najbardziej podatna na błędy część (indeksy od 1, trzy listy, końce linii), a dzięki podziałowi właśnie ona jest sprawdzana testami jednostkowymi. `gfx::Mesh` wymaga kontekstu i testu jednostkowego nie ma.
2. **Jedno zadanie na klasę.** `gfx::Mesh` nie wie, co to plik OBJ, a loader nie wie, co to VAO. Każdą z tych rzeczy tłumaczę osobno.
3. **Dane zostają dostępne.** Wczytany model można obejrzeć (liczba wierzchołków, pudełko otaczające), zanim trafi na kartę, albo użyć go bez rysowania.

Loadery **nie rzucają wyjątków** przy złym pliku. Brakujący albo uszkodzony plik to zwykła sytuacja przy pracy nad assetami: loader zwraca `false`, podaje tekst powodu i wypisuje błąd raz przez `core::logError`. To ten sam styl co `gfx::Shader::reload`, które też zwraca `bool`, zostawia poprzedni stan i zapisuje powód.

Oba loadery plików mają ten sam kształt, więc wystarczy nauczyć się go raz:

| Funkcja | Wynik | Błąd |
|---|---|---|
| `bool loadObj(path, ObjModel& model, std::string& error)` | `true` i wypełniony `model` | `false`, powód w `error`, `model` bez zmian |
| `bool loadImage(path, Image& image, std::string& error)` | `true` i wypełniony `image` | `false`, powód w `error`, `image` bez zmian |

## 4. Miejsce w warstwach

```mermaid
flowchart TD
    Game["game/<br/>NightMazeApp, MazeRenderer"] --> Assets
    Debug["debug/<br/>AssetsPanel"] --> Assets
    Tests["tests/<br/>ObjLoaderTests, ImageLoaderTests,<br/>TangentTests"] --> Assets
    Assets["assets/<br/>ObjLoader, Tangents, ImageLoader,<br/>AssetCache"] --> Gfx["gfx/<br/>Vertex.hpp, Mesh, Texture2D"]
    Assets --> Core["core/<br/>Log, Paths"]
    Assets --> Glm["GLM"]
    Assets --> Stb["stb_image"]
    Gfx --> Core
```

Strzałka znaczy "zna i dołącza nagłówki". Diagram pokazuje stan faktyczny. Testy dołączają tylko loadery i `assets/Tangents.hpp`. `game/` dołącza `assets/AssetCache.hpp` (pole `m_assets` w `NightMazeApp`, prośby o modele w `MazeRenderer`), a `debug/` dołącza go w panelu Assets.

Zasady dla `assets`:

1. `assets/` stoi **nad** `gfx/` i `core/`: może dołączać ich nagłówki. Z `gfx/` loader OBJ i `Tangents` dołączają tylko `gfx/Vertex.hpp`, nagłówek bez OpenGL. Z `core/` dołącza `core/Log.hpp` (logowanie błędu) i `core/Paths.hpp` (`core::pathText` do komunikatów). `AssetCache` dołącza dodatkowo `gfx/Mesh.hpp` i `gfx/Texture2D.hpp`.
2. Loadery i `Tangents` **nie wołają OpenGL** i nie dołączają GLAD ani GLFW. `AssetCache` też nie woła funkcji `gl*` bezpośrednio, ale tworzy obiekty klas `gfx`, więc potrzebuje kontekstu OpenGL przez całe swoje życie.
3. `gfx/` i `core/` nie znają `assets/`. `gfx::Mesh` przyjmuje `std::span<const Vertex>` i nie wie, czy dane przyszły z pliku, czy z tablicy w kodzie.
4. `assets/` nie zna `scene/`, `game/` ani `debug/`. Nic w nim nie jest specyficzne dla Night Maze: loader OBJ wczyta model z dowolnego programu, o ile ten trzyma się obsługiwanej części formatu.
5. O tym, **który** plik wczytać, decyduje wołający. Loader dostaje gotową ścieżkę. Zbudowanie jej przez `core::assetPath` to sprawa gry, tak jak przy shaderach.

W [`CMakeLists.txt`](../../../CMakeLists.txt) pliki `src/assets/*` należą do biblioteki statycznej `engine`, razem z `core`, `gfx` i `scene`. Program testowy dostaje je przez `game_logic`, która linkuje `engine`. Granicy między `assets/` a resztą `engine` nie pilnuje więc linker, tylko dyscyplina dyrektyw `#include`.

PRD (sekcja 6) umieszcza w tej warstwie także `AssetCache`: pamięć podręczną, która pilnuje, żeby ten sam plik był wczytany raz. Loader OBJ jej to ułatwia: ścieżki tekstur zwraca znormalizowane, więc dwa modele z tą samą teksturą podają identyczną ścieżkę, a pamięć podręczna rozpoznaje ją jako ten sam klucz ([`asset-cache.md`](asset-cache.md), sekcja 2).

## 5. Pytania kontrolne

Pytania z odpowiedziami do formatów i kodu są w sekcji 9 dokumentów tematycznych. Pięć pytań dotyczących treści tego pliku:

1. **Czym różni się `assets/` od `src/assets/`?**
   Pierwszy to katalog z plikami danych (modele, tekstury, shadery), które program czyta w czasie działania. Drugi to kod C++ w przestrzeni nazw `assets`, który te pliki rozbiera na dane.

2. **Dlaczego loader nie tworzy od razu obiektu OpenGL?**
   Bo wtedy wymagałby okna i kontekstu, a nie dałoby się go testować w programie testowym. Loader zwraca dane procesora (`ObjModel`, `Image`), a obiekt na karcie tworzy z nich klasa `gfx` (`Mesh`, `Texture2D`). Rozbiór pliku i wysyłanie na kartę to dwa osobne zadania. Łączy je dopiero `AssetCache`.

3. **Od czego zależy moduł `assets` i co zależy od niego?**
   Loadery dołączają `gfx/Vertex.hpp`, `core/Log.hpp`, `core/Paths.hpp`, GLM i bibliotekę stb_image, a `AssetCache` także `gfx/Mesh.hpp` i `gfx/Texture2D.hpp`. Żaden plik modułu nie dołącza GLFW. Moduł dołączają `game/` (aplikacja i renderer labiryntu), `debug/` (panel Assets) i testy (loadery i funkcje stycznych). `core/` i `gfx/` go nie znają.

4. **Po co pamięć podręczna, skoro są loadery?**
   Loader wczytuje plik za każdym razem, gdy zostanie wywołany, i oddaje dane procesora. Pamięć podręczna wczytuje każdy plik raz, tworzy z danych obiekt na karcie, jest jego jedynym właścicielem i oddaje stabilny wskaźnik, więc tekstura wspólna dla ściany i słupka istnieje na karcie raz. To samo dotyczy ich wspólnej mapy normalnych.

5. **Dlaczego kod stycznych leży w `assets/`, a jego dokument w `gfx/`?**
   Styczne liczy się raz, przy wczytaniu modelu, z danych procesora i bez OpenGL, więc kod należy do warstwy, która czyta pliki, i woła go `parseObj`. Styczna ma jednak sens tylko razem z mapą normalnych i shaderem, który z niej korzysta, a to jest temat modułu `gfx`. Dokument idzie za tematem, żeby całe wyjaśnienie było w jednym miejscu.

## 6. Źródła

- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 6 (warstwa `assets/`: ObjLoader, ImageLoader, AssetCache), sekcja 9 (konwencje modeli).
- Przewodnik po assetach: [`../../guides/blender.md`](../../guides/blender.md).
- Dokumenty w tym repozytorium: [`asset-cache.md`](asset-cache.md), [`../gfx/mesh.md`](../gfx/mesh.md), [`../gfx/textures.md`](../gfx/textures.md), [`../gfx/normal-mapping.md`](../gfx/normal-mapping.md), [`../game/maze-rendering.md`](../game/maze-rendering.md) (kto prosi o modele).
- Dokument biblioteki: [`../../libraries/stb_image.md`](../../libraries/stb_image.md).
- Szczegółowe źródła do każdego zagadnienia są w sekcji 10 dokumentów tematycznych.
