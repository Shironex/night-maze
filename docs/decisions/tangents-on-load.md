# Styczne: liczone przy wczytaniu modelu, ortogonalizowane na procesorze, bez znaku skrętności

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/assets/Tangents.hpp`](../../src/assets/Tangents.hpp), [`Tangents.cpp`](../../src/assets/Tangents.cpp) (`triangleTangents`, `computeTangents`, `countMirroredTriangles`), [`src/assets/ObjLoader.cpp`](../../src/assets/ObjLoader.cpp) (koniec `parseObj`, ostrzeżenie w `loadObj`), [`src/gfx/Vertex.hpp`](../../src/gfx/Vertex.hpp) (pole `tangent`), [`assets/shaders/common/normal_map.glsl`](../../assets/shaders/common/normal_map.glsl) (`surfaceNormal`). Dokument modułu: [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md), sekcje 2.7, 2.8, 2.9 i od 5.5 do 5.7.

## 1. Kontekst

Mapa normalnych w przestrzeni stycznej wymaga w każdym wierzchołku trzech kierunków: normalnej `N`, stycznej `T` i bitangenty `B`. Plik OBJ zawiera tylko pozycje, współrzędne tekstury i normalne. Stycznych w nim nie ma i format nie ma na nie linii. Trzeba było zdecydować trzy rzeczy naraz:

- skąd wziąć `T`: z pliku w innym formacie, z osobnego pliku obok modelu, czy policzyć przy wczytaniu,
- gdzie zrobić krok, który czyni `T` prostopadłą do `N` (ortogonalizacja Grama-Schmidta): na procesorze raz, czy w shaderze co klatkę,
- czy przechowywać `B` albo znak skrętności, czy odtwarzać `B` w shaderze jako `cross(N, T)`.

Ograniczenia: modele gry to trzy proste bryły z płaskim cieniowaniem (każda ściana ma własne wierzchołki), eksportowane skryptem z Blendera do OBJ, a każdą linię kodu muszę umieć wytłumaczyć na obronie.

## 2. Decyzja

Styczne liczy `assets::computeTangents` na końcu `parseObj`, z pozycji i współrzędnych tekstury trójkątów. Tam też, raz, styczna jest ortogonalizowana względem normalnej i sprowadzana do długości 1. Wierzchołek przechowuje tylko `T` (trzy liczby). `B` liczy shader fragmentów jako `cross(N, T)`, bez znaku skrętności i bez ponownej ortogonalizacji. To, czy model się do tego nadaje, sprawdza `countMirroredTriangles`: wynik trafia do `ObjModel::mirroredTriangleCount`, a `loadObj` wypisuje ostrzeżenie, gdy jest większy od zera.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Liczenie przy wczytaniu, Gram-Schmidt na procesorze, samo `T` w wierzchołku (wybrana)** | pliki modeli zostają bez zmian (bajt w bajt). Jedna funkcja bez OpenGL, w całości pokryta testami. Shader dostaje gotową parę i jest krótki. Wierzchołek rośnie o 12 bajtów, nie o 16 ani 24 | nie obsługuje lustrzanych UV. Shader nie poprawia prostopadłości po interpolacji, co na modelu gładko cieniowanym dałoby drobny błąd. Koszt przy starcie (pomijalny przy 124 wierzchołkach trzech modeli) |
| Styczne w pliku modelu (format z atrybutem stycznej, na przykład glTF) | styczne takie, jakie policzył program do modelowania, zgodne z tym, czym wypalono mapę | nowy format i nowy loader w miejsce OBJ, który jest tematem 4 wykładu. Mapy projektu nie są wypalane z modelu, tylko liczone z pola wysokości, więc zgodność z algorytmem Blendera nic tu nie daje |
| Osobny plik ze stycznymi obok `.obj`, pisany przez skrypt Blendera | OBJ zostaje, styczne z Blendera | drugi plik na model, własny format, dwa miejsca, które muszą się zgadzać co do kolejności wierzchołków. Loader OBJ skleja wierzchołki po swojemu, więc kolejność z Blendera nie przenosi się wprost |
| Gram-Schmidt w shaderze wierzchołków, co klatkę | wersja z większości samouczków. Poprawna także wtedy, gdy macierz modelu ma nierówną skalę i używa się jednej macierzy dla obu wektorów | ten sam wynik liczony od nowa dla każdego wierzchołka w każdej klatce. Projekt i tak przekształca `N` i `T` dwiema właściwymi macierzami, więc para zostaje prostopadła bez tego kroku |
| `T` i `B` w wierzchołku (6 liczb) | shader nie liczy iloczynu wektorowego, lustrzane UV działają | wierzchołek 56 bajtów zamiast 44. `B` jest w modelach gry w pełni wyznaczone przez `N` i `T` |
| `T` ze znakiem skrętności w czwartej składowej (4 liczby) | standardowe rozwiązanie, obsługuje lustrzane UV | czwarta składowa, którą trzeba policzyć, przesłać i wytłumaczyć, a która dla każdego wierzchołka każdego modelu gry miałaby wartość `+1` |

## 4. Uzasadnienie i skutki

**Dlaczego liczenie przy wczytaniu.** Wzór na styczną trójkąta to rozwiązanie układu dwóch równań z dwiema niewiadomymi: mieści się na kartce i da się go pokazać na jednym trójkącie ściany. Loader ma wszystko, czego wzór potrzebuje, w chwili gdy zna już wszystkie trójkąty. Pliki `.obj` nie zmieniły się ani o bajt, a liczba wierzchołków i indeksów została ta sama (ściana i słupek 60 i 90, podłoga 4 i 6).

**Dlaczego Gram-Schmidt na procesorze.** Wynik zależy tylko od danych modelu. Liczony raz, w funkcji bez OpenGL, jest sprawdzony testem na liczbach (`(1, 0, 0)` i normalna `(0,6, 0, 0,8)` dają `(0,8, 0, -0,6)`). Ten sam krok w shaderze byłby nietestowalny i powtarzany co klatkę.

**Dlaczego bez ponownej ortogonalizacji w shaderze.** Interpolacja między wierzchołkami może zepsuć kąt prosty tylko wtedy, gdy miesza **różne** wektory. W modelach gry każda ściana ma własne wierzchołki z tą samą normalną i tą samą styczną, więc interpolacja miesza wektory identyczne. Po stronie macierzy para też jest bezpieczna: normalna idzie przez macierz normalnych, styczna przez `mat3(uModel)`, a te dwie macierze zachowują prostopadłość dla każdej macierzy modelu.

**Dlaczego bez znaku skrętności.** Żaden trójkąt trzech modeli nie ma lustrzanej bazy. Skrypt nakłada UV tak, że na każdej stronie bryły `u` rośnie w prawo dla patrzącego z zewnątrz. Nie jest to założenie na słowo: `countMirroredTriangles` to liczy, a testy trzech modeli wymagają zera. Znak, który zawsze wynosi `+1`, byłby kodem bez pokazu.

**Co przez to tracę.**

- Model z lustrzanymi UV (typowy dla postaci: połowa twarzy odbita) pokaże na odbitych trójkątach relief odwrócony. Dostanie ostrzeżenie w logu, ale nie naprawi się sam.
- Model gładko cieniowany będzie miał po interpolacji `T` i `N` odchylone od kąta prostego o ułamek stopnia. Relief pozostanie czytelny, ale nie będzie ściśle poprawny.
- Styczne nie muszą zgadzać się z tymi, których użyłby Blender (algorytm MikkTSpace). Dla map liczonych z pola wysokości nie ma to znaczenia. Dla mapy wypalonej z modelu o dużej liczbie trójkątów miałoby.
- Wierzchołek ma 44 bajty zamiast 32, także w siatkach, które stycznej nie używają (linie kolizji, kostki świateł).

## 5. Kiedy wrócić do tej decyzji

- Gdy `loadObj` wypisze dla jakiegoś modelu ostrzeżenie o lustrzanych trójkątach: wtedy styczna dostaje czwartą składową ze znakiem, a shader mnoży przez nią bitangentę.
- Gdy dojdzie model gładko cieniowany z mapą normalnych (na przykład model kryształu): wtedy w `surfaceNormal` trzeba dopisać krok Grama-Schmidta po interpolacji.
- Gdy mapy normalnych zaczną być wypalane w Blenderze z modeli o dużej liczbie trójkątów, a nie liczone z pola wysokości: wtedy styczne muszą pochodzić z tego samego algorytmu co przy wypalaniu, czyli z pliku.
- Gdy projekt zmieni format modeli na taki, który styczne przechowuje.
