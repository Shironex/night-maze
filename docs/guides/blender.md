# Blender: od skryptu do pliku OBJ

Przewodnik po tym, jak powstają modele i tekstury gry. Modeli nie rysuję ręcznie w Blenderze:
każdy powstaje ze skryptu w Pythonie, który Blender wykonuje bez okna (headless) i który na
końcu zapisuje plik OBJ. Polecenia dla Windowsa z sekcji 3 zostały uruchomione w konfiguracji:

| Element | Wersja |
|---|---|
| Blender | 5.2.1 LTS |
| Python wbudowany w Blendera | 3.13.13 |
| numpy wbudowane w Blendera | 2.3.4 |
| System | Windows 11 |

Polecenia dla macOS (sekcja 3) **nie były jeszcze uruchomione na Macu**.

Stan: skrypty, trzy modele i dwie tekstury są w repozytorium. Kod C++, który wczytuje pliki
OBJ, jeszcze w nim nie istnieje, więc gra tych plików na razie nie rysuje. Sekcja 5 opisuje
dokładnie, co jest w plikach, właśnie po to, żeby dało się do nich napisać własny parser.

## 1. Skrypt jest źródłem, plik OBJ jest wynikiem

```mermaid
flowchart LR
    T["make_textures.py"] --> P["assets/textures/*.png"]
    P --> B["build_nazwa.py"]
    C["blender_common.py"] --> B
    B --> O["assets/models/nazwa.obj"]
    B --> M["assets/models/nazwa.mtl"]
    M -.->|"ścieżka względna w map_Kd"| P
```

W katalogu [`tools/blender/`](../../tools/blender/) leżą **źródła (source)**: skrypty. W
katalogach [`assets/models/`](../../assets/models/) i [`assets/textures/`](../../assets/textures/)
leży **wynik (output)**: pliki `.obj`, `.mtl` i `.png`. Zasada jest taka sama jak przy kodzie
C++ i pliku wykonywalnym: zmieniam źródło i generuję wynik od nowa. **Plików wynikowych nie
poprawiam ręcznie**, bo następne uruchomienie skryptu nadpisze poprawkę.

Różnica względem programu: wynik jest zapisany w repozytorium. Dzięki temu grę da się zbudować
i uruchomić bez instalowania Blendera.

Dlaczego skrypty, a nie modelowanie myszą:

- **Da się to wytłumaczyć.** Ściana to trzy prostopadłościany o wymiarach zapisanych jako
  stałe. Na obronie pokazuję linię, z której bierze się każdy wierzchołek. Pliku `.blend`
  nie da się przeczytać.
- **Wymiary są dokładne.** Komórka labiryntu ma 2 m, a ściana 3 m, co do szóstego miejsca po
  przecinku, bo to liczby wpisane w kod, a nie przeciągnięcia myszą.
- **Wynik jest powtarzalny.** To samo polecenie daje te same bajty. Dwa uruchomienia pod rząd
  na Windowsie dały identyczne pliki `.obj`, `.mtl` i `.png` (porównane programem `diff`).
- **Historia zmian jest czytelna.** `git diff` skryptu pokazuje, że cokół urósł z 0.25 do
  0.30 m. Różnicy dwóch plików binarnych nie pokazuje nic.
- **Eksport nie zależy od pamięci.** Opcje eksportu są zapisane w jednym miejscu
  (`export_obj`), więc nie da się raz zapomnieć o triangulacji albo o zamianie osi.

Pliki skryptów:

| Plik | Co robi |
|---|---|
| [`blender_common.py`](../../tools/blender/blender_common.py) | wspólne funkcje: czyszczenie sceny, budowanie prostopadłościanu, UV, materiał z teksturą, eksport, rendery kontrolne |
| [`make_textures.py`](../../tools/blender/make_textures.py) | generuje `wall_stone.png` i `floor_stone.png` |
| [`build_wall_straight.py`](../../tools/blender/build_wall_straight.py) | model odcinka ściany |
| [`build_wall_pillar.py`](../../tools/blender/build_wall_pillar.py) | model słupa |
| [`build_floor_tile.py`](../../tools/blender/build_floor_tile.py) | model płyty podłogi |
| [`make_all.py`](../../tools/blender/make_all.py) | uruchamia wszystko po kolei: najpierw tekstury, potem modele |
| [`.gitignore`](../../tools/blender/.gitignore) | pomija katalog `__pycache__`, który Python tworzy przy imporcie modułów |

## 2. Konwencje

Konwencje pochodzą z PRD, sekcja 9. Każda ma powód:

| Konwencja | Wartość | Dlaczego |
|---|---|---|
| Układ współrzędnych gry | prawoskrętny, Y w górę, -Z do przodu | taki sam jak w kamerze i macierzach gry ([`../modules/scene/README.md`](../modules/scene/README.md)). Model wczytany z pliku nie wymaga wtedy żadnego dodatkowego obrotu |
| Jednostka | 1 jednostka = 1 metr | prędkość kamery i rozmiary labiryntu są w metrach, więc model ma od razu właściwą wielkość |
| Komórka labiryntu | 2 x 2 m, ściany 3 m | z tych liczb wynikają wymiary wszystkich trzech modeli |
| Początek układu modelu (origin) | środek podstawy, podłoga to y = 0 | model stawiam przesunięciem o (x, 0, z), bez liczenia połowy wysokości |
| Trójkąty | wszystkie ściany modelu są trójkątami | OpenGL w profilu Core rysuje trójkąty. Parser nie musi dzielić wielokątów |
| Normalne | jedna na ścianę, cieniowanie płaskie (flat shading) | twarde krawędzie pasują do stylu low-poly. Przydadzą się, gdy dojdzie oświetlenie |
| UV | 1 jednostka UV = 2 m na każdej ścianie | stała gęstość tekseli (texel density): kamień ma wszędzie tę samą wielkość (sekcja 6) |
| Przekształcenia i modyfikatory | zapisane w wierzchołkach | plik nie niesie macierzy, więc pozycje w pliku są pozycjami modelu |
| Nazwy | `snake_case` | jeden styl dla plików, obiektów i materiałów. Nazwa pliku, nazwa po `o` i nazwa skryptu są takie same |
| Tekstury | PNG, rozmiar będący potęgą dwójki, 8 bitów na kanał, RGB | PNG nie traci jakości, a rozmiar 512 dzieli się na połowy aż do 1 piksela, co jest potrzebne mipmapom |
| Ścieżka do tekstury w `.mtl` | względna, z ukośnikami `/` | ten sam plik działa na Windowsie i na macOS, niezależnie od miejsca repozytorium na dysku |

Blender ma inny układ niż gra: też prawoskrętny, ale **Z w górę**. Geometria w skryptach jest
zapisana w układzie Blendera, a zamianę robi eksporter (sekcja 4). Dlatego w skryptach wysokość
to trzecia liczba, a w pliku `.obj` druga.

## 3. Uruchamianie

Wszystkie polecenia wykonuję w katalogu głównym repozytorium. Skrypty same znajdują katalog
`assets/` na podstawie własnego położenia, więc działają też z innego katalogu roboczego
(sprawdzone na Windowsie).

### Windows (PowerShell)

Blender nie jest dodany do `PATH`, więc podaję pełną ścieżkę. Znak `&` jest w PowerShellu
operatorem wywołania: bez niego ścieżka w cudzysłowie byłaby zwykłym napisem.

Wszystko naraz:

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python tools/blender/make_all.py
```

Wszystko naraz i do tego rendery kontrolne:

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python tools/blender/make_all.py -- --shots
```

Jeden skrypt (same tekstury albo jeden model):

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python tools/blender/make_textures.py
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python tools/blender/build_wall_straight.py -- --shots
```

### macOS (jeszcze nie uruchomione na Macu)

Zwykle program leży w `/Applications/Blender.app/Contents/MacOS/Blender`:

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup --python tools/blender/make_all.py
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup --python tools/blender/make_all.py -- --shots
```

Do sprawdzenia na Macu:

1. Czy powyższe polecenie kończy się bez błędu w tej samej wersji Blendera (5.2.1).
2. Czy `git status` po uruchomieniu nie pokazuje zmian w `assets/`. Pliki `.obj` i `.mtl`
   powinny wyjść identyczne. Dla plików `.png` tego nie wiem: zależy to od tego, czy numpy
   i biblioteka zapisująca PNG dają na obu systemach te same bajty.
3. Czy rendery kontrolne powstają także bez okna.

### Co znaczą argumenty

| Argument | Znaczenie |
|---|---|
| `--background` | bez okna. Blender wykonuje skrypt i kończy pracę |
| `--factory-startup` | ustawienia fabryczne zamiast moich prywatnych: bez dodatków i bez własnego pliku startowego. Dzięki temu wynik nie zależy od konfiguracji Blendera na danym komputerze |
| `--python plik.py` | skrypt do wykonania |
| `--` | koniec argumentów Blendera. Wszystko dalej Blender ignoruje i zostawia skryptowi w `sys.argv` |
| `--shots` | argument moich skryptów: zapisz rendery kontrolne |

### Rendery kontrolne (review renders)

Z argumentem `--shots` każdy skrypt modelu zapisuje dwa obrazy 960 x 720 z różnych stron do
katalogu tymczasowego systemu i wypisuje ich ścieżki. Na Windowsie jest to
`C:\Users\<nazwa>\AppData\Local\Temp\night_maze_review\`, pliki `wall_straight_1.png`,
`wall_straight_2.png` i tak dalej. Rendery **nigdy nie trafiają do repozytorium**: służą tylko
do obejrzenia modelu po zmianie skryptu.

Render robi silnik Workbench (ten sam, który rysuje podgląd w oknie Blendera) z kolorem
z tekstury i oświetleniem "studio", które cieniuje ściany zależnie od kierunku. W grze
oświetlenia jeszcze nie ma, więc tam wszystkie ściany modelu mają jasność samej tekstury.
Kamerę skrypt dodaje dopiero po eksporcie. Kolejność jest ważna: eksport zapisuje całą scenę,
więc kamera dodana wcześniej trafiłaby do pliku `.obj`.

## 4. Opcje eksportu

Eksport to jedno wywołanie operatora `bpy.ops.wm.obj_export` w funkcji `export_obj` w
[`blender_common.py`](../../tools/blender/blender_common.py). Ustawiam jawnie każdą opcję, która
wpływa na plik, także wtedy, gdy wartość jest domyślna w Blenderze 5.2. Inna wersja Blendera
może mieć inne wartości domyślne, a wtedy plik zmieniłby się po cichu.

| Parametr | Wartość | Znaczenie |
|---|---|---|
| `filepath` | `assets/models/<nazwa>.obj` | plik wynikowy. Plik `.mtl` powstaje obok, pod tą samą nazwą |
| `up_axis` | `'Y'` | która oś gry wskazuje w górę |
| `forward_axis` | `'NEGATIVE_Z'` | która oś gry wskazuje do przodu. Razem z `up_axis` daje zamianę punktu Blendera (x, y, z) na punkt gry (x, z, -y) |
| `global_scale` | `1.0` | bez skalowania: 1 jednostka Blendera to 1 metr |
| `apply_transform` | `True` | położenie, obrót i skala obiektu są wliczone w pozycje wierzchołków |
| `apply_modifiers` | `True` | modyfikatory są wliczone w siatkę. Moje modele ich nie mają, opcja zabezpiecza przyszłe |
| `export_eval_mode` | `'DAG_EVAL_VIEWPORT'` | którą wersję obiektu brać, gdy modyfikator ma inne ustawienia dla podglądu i dla renderu |
| `export_selected_objects` | `False` | eksportuję całą scenę. Jest w niej tylko model, bo skrypt zaczyna od pustej sceny |
| `export_triangulated_mesh` | `True` | każdy czworokąt jest zapisany jako dwa trójkąty |
| `export_uv` | `True` | linie `vt` |
| `export_normals` | `True` | linie `vn` |
| `export_materials` | `True` | plik `.mtl` oraz linie `mtllib` i `usemtl` |
| `export_pbr_extensions` | `False` | bez rozszerzeń PBR w `.mtl`. Włączona w próbie dopisała linie `Pr`, `Pm`, `Ps`, `Pc`, `Pcr`, `aniso` i `anisor`, a pominęła `Ns` i `Ka` |
| `path_mode` | `'RELATIVE'` | ścieżka tekstury w `.mtl` liczona względem katalogu pliku `.mtl` |
| `export_colors` | `False` | bez kolorów wierzchołków. Włączona w próbie dopisała trzy liczby na końcu każdej linii `v` |
| `export_animation` | `False` | jeden plik. Włączona w próbie zapisała osobną parę plików na każdą klatkę, z numerem klatki w nazwie |
| `export_curves_as_nurbs` | `False` | dotyczy krzywych, których w modelach nie ma. Nie sprawdzałem, co zmienia |
| `export_object_groups` | `False` | obiekt zaczyna linia `o`. Włączona w próbie zapisała zamiast niej linię `g` |
| `export_material_groups` | `False` | bez linii `g` przed `usemtl`. Włączona w próbie dopisała `g <obiekt>_<materiał>` |
| `export_vertex_groups` | `False` | bez linii `g` z nazwą grupy wierzchołków. Modele nie mają takich grup |
| `export_smooth_groups` | `False` | bez numerowanych grup wygładzania. Linia `s 0` jest w pliku tak czy inaczej |

"W próbie" znaczy: wyeksportowałem jednorazowo próbny kwadrat z daną opcją włączoną i
przeczytałem plik. Tych plików nie ma w repozytorium.

Dwie rzeczy, które wyszły dopiero przy czytaniu wyniku:

- Tryb `'RELATIVE'` zapisuje na Windowsie `../textures/wall_stone.png`, z ukośnikami `/`, czyli
  dokładnie to, czego chcę. Domyślny tryb `'AUTO'` zapisał w próbie ścieżkę bezwzględną
  zaczynającą się od `C:/Users/...`.
- Blender **nie zapisuje linii `Kd`**, gdy kolor materiału pochodzi z tekstury (materiał bez
  tekstury dostał w próbie `Kd 0.800000 0.800000 0.800000`). Konwencja
  projektu wymaga jej w każdym materiale, więc funkcja `add_diffuse_color` dopisuje po
  eksporcie linię `Kd 1.000000 1.000000 1.000000` przed linią `map_Kd`. Biały kolor pomnożony
  przez teksturę zostawia teksturę bez zmian. To poprawka w skrypcie, a nie ręczna: działa
  przy każdym uruchomieniu.

## 5. Co jest w plikach

### 5.1 Plik `.obj`

Cały plik [`assets/models/floor_tile.obj`](../../assets/models/floor_tile.obj):

```text
# Blender 5.2.1 LTS
# www.blender.org
mtllib floor_tile.mtl
o floor_tile
v -1.000000 0.000000 1.000000
v 1.000000 0.000000 1.000000
v 1.000000 0.000000 -1.000000
v -1.000000 0.000000 -1.000000
vn -0.0000 1.0000 -0.0000
vt 0.500000 -0.500000
vt -0.500000 0.500000
vt -0.500000 -0.500000
vt 0.500000 0.500000
s 0
usemtl floor_stone
f 2/1/1 4/2/1 1/3/1
f 2/1/1 3/4/1 4/2/1
```

| Linia | Znaczenie |
|---|---|
| `# ...` | komentarz. Dwie pierwsze linie pliku podają wersję Blendera |
| `mtllib floor_tile.mtl` | nazwa pliku z materiałami, względem katalogu pliku `.obj` |
| `o floor_tile` | początek obiektu o tej nazwie. W każdym pliku jest jeden obiekt |
| `v x y z` | pozycja wierzchołka w metrach, sześć miejsc po przecinku |
| `vn x y z` | normalna, cztery miejsca po przecinku. Jedna na każdy kierunek ściany, a nie na wierzchołek |
| `vt u v` | współrzędne tekstury. Mogą być ujemne i większe od 1 |
| `s 0` | grupy wygładzania wyłączone. Występuje raz, zawsze jako `s 0` (nie `s off`) |
| `usemtl floor_stone` | materiał dla wszystkich następnych linii `f` |
| `f a/b/c a/b/c a/b/c` | trójkąt: trzy narożniki, każdy jako trzy indeksy `pozycja/uv/normalna` |

Fakty ważne dla parsera, sprawdzone na wszystkich trzech plikach:

- Występują tylko linie `#`, `mtllib`, `o`, `v`, `vn`, `vt`, `s`, `usemtl` i `f`. Linii `g` nie
  ma. Kolejność jest zawsze taka jak wyżej: komentarze, `mtllib`, `o`, wszystkie `v`,
  wszystkie `vn`, wszystkie `vt`, `s 0`, `usemtl`, wszystkie `f`. **Linie `vn` są przed `vt`**,
  choć w indeksach linii `f` kolejność jest odwrotna: uv jest w środku.
- Każda linia `f` ma dokładnie trzy narożniki, a każdy narożnik komplet trzech indeksów.
- Indeksy liczą się **od 1**, nie od 0, i są dodatnie (format dopuszcza też ujemne, liczone od
  końca, ale Blender ich nie pisze).
- Trzy listy mają różne długości: w `wall_straight.obj` są 24 pozycje, 24 pary uv i 6
  normalnych. Narożnik `5/1/1` nie jest więc gotowym wierzchołkiem dla OpenGL. Bufor
  wierzchołków ma jeden wspólny indeks, więc parser musi dla każdej trójki `a/b/c` złożyć
  wierzchołek z trzech list.
- Wszystkie trzy narożniki trójkąta mają ten sam indeks normalnej: to jest właśnie cieniowanie
  płaskie.
- Narożniki są podane przeciwnie do ruchu wskazówek zegara, patrząc z zewnątrz modelu.
  Sprawdziłem to liczbowo: iloczyn wektorowy dwóch krawędzi każdego trójkąta wskazuje w tę
  samą stronę co jego normalna.
- W normalnych pojawia się `-0.0000`. To zwykłe zero ze znakiem minus.
- Liczby mają kropkę dziesiętną i nie mają wykładnika. Pola dzieli jedna spacja.
- Końce linii to sam znak LF, także w pliku zapisanym na Windowsie, a plik kończy się znakiem
  nowej linii.

### 5.2 Jak Z w górę stało się Y w górę

W skrypcie [`build_floor_tile.py`](../../tools/blender/build_floor_tile.py) narożniki płyty są
zapisane w układzie Blendera. Eksporter zamienia punkt (x, y, z) na (x, z, -y):

| Narożnik w skrypcie (Blender) | Linia w pliku (gra) |
|---|---|
| `(-1, -1, 0)` | `v -1.000000 0.000000 1.000000` |
| `(1, -1, 0)` | `v 1.000000 0.000000 1.000000` |
| `(1, 1, 0)` | `v 1.000000 0.000000 -1.000000` |
| `(-1, 1, 0)` | `v -1.000000 0.000000 -1.000000` |

Wysokość (trzecia liczba w Blenderze) trafiła na drugie miejsce. Oś Y Blendera stała się osią
Z gry z przeciwnym znakiem. Znak musi się zmienić, bo oba układy są prawoskrętne: sama zamiana
dwóch osi miejscami dałaby układ lewoskrętny, czyli lustrzane odbicie modelu. Ta sama zamiana
dotyczy normalnych: w Blenderze płyta patrzy w +Z, w pliku jest `vn -0.0000 1.0000 -0.0000`,
czyli +Y.

To samo widać w ścianie. Linie z [`assets/models/wall_straight.obj`](../../assets/models/wall_straight.obj):

```text
v -1.000000 0.000000 0.140000
v 1.000000 0.000000 0.140000
...
v 1.000000 3.000000 -0.140000
```

Druga liczba biegnie od 0 do 3: to wysokość ściany. Pierwsza od -1 do 1: długość. Trzecia od
-0.14 do 0.14: grubość.

### 5.3 Plik `.mtl`

Cały plik [`assets/models/wall_straight.mtl`](../../assets/models/wall_straight.mtl):

```text
# Blender 5.2.1 LTS MTL File: 'None'
# www.blender.org

newmtl wall_stone
Ns 250.000000
Ka 1.000000 1.000000 1.000000
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 2
Kd 1.000000 1.000000 1.000000
map_Kd ../textures/wall_stone.png
```

| Linia | Znaczenie | Czy jej potrzebuję |
|---|---|---|
| `# ... MTL File: 'None'` | komentarz. W tym miejscu Blender wpisuje zapewne nazwę pliku `.blend`, a scena ze skryptu nie jest zapisana w żadnym pliku | nie |
| pusta linia | odstęp przed materiałem | nie |
| `newmtl wall_stone` | początek materiału o tej nazwie. Do niej odwołuje się `usemtl` w pliku `.obj` | tak |
| `Ns` | wykładnik połysku (shininess) | dopiero przy oświetleniu |
| `Ka` | kolor światła otoczenia (ambient) | dopiero przy oświetleniu |
| `Ks` | kolor odbłysku (specular) | dopiero przy oświetleniu |
| `Ke` | kolor emisji | nie |
| `Ni` | współczynnik załamania światła | nie |
| `d` | nieprzezroczystość (dissolve), 1 to pełna | nie |
| `illum 2` | numer modelu oświetlenia w formacie MTL | nie |
| `Kd 1.000000 1.000000 1.000000` | kolor rozproszony (diffuse). Dopisuje go mój skrypt (sekcja 4) | tak |
| `map_Kd ../textures/wall_stone.png` | tekstura koloru rozproszonego. Ścieżka względem katalogu pliku `.mtl` | tak |

Wartości `Ns`, `Ka`, `Ks`, `Ke`, `Ni`, `d` i `illum` to domyślne ustawienia materiału w
Blenderze. Niczego w nich nie ustawiam. Parser musi umieć **pominąć linię, której nie zna**.

Pliki `wall_straight.mtl` i `wall_pillar.mtl` są identyczne co do bajta: oba definiują materiał
`wall_stone` z tą samą teksturą. Ten sam obraz nie powinien być wczytywany do karty dwa razy.

## 6. UV i gęstość tekseli

**Gęstość tekseli (texel density)** to liczba pikseli tekstury przypadająca na metr
powierzchni. U mnie jest stała: jedno powtórzenie tekstury zajmuje 2 m, a tekstura ma 512
pikseli, czyli wychodzi 256 pikseli na metr na każdej ścianie każdego modelu. Gdyby ściana
i słup miały różną gęstość, te same kamienie byłyby na słupie większe albo mniejsze niż na
ścianie obok.

UV liczy funkcja `box_project_uvs`. To **rzut pudełkowy (box projection)**: każda ściana modelu
jest prostopadła do jednej osi, więc patrzę na nią wzdłuż tej osi, a dwie pozostałe
współrzędne dzielę przez 2 m. W układzie gry wychodzi:

| Ściana patrzy w | u | v |
|---|---|---|
| +Z | x / 2 | y / 2 |
| -Z | -x / 2 | y / 2 |
| +X | -z / 2 | y / 2 |
| -X | z / 2 | y / 2 |
| +Y (góra) | x / 2 | -z / 2 |
| -Y (spód) | x / 2 | z / 2 |

Co z tego wynika:

- Na ścianach pionowych `v` to wysokość podzielona przez 2, więc tekstura stoi prosto, a
  podłoga (y = 0) to `v = 0`. Ściana o wysokości 3 m ma `v` od 0 do 1.5: tekstura powtarza
  się półtora raza.
- Ściana o długości 2 m ma `u` od -0.5 do 0.5, czyli dokładnie jedno powtórzenie. Ujemne
  wartości są poprawne: przy `GL_REPEAT` liczy się tylko część ułamkowa.
- Znak przy `u` jest inny po dwóch przeciwnych stronach, żeby `u` rosło w prawo dla kogoś, kto
  patrzy na ścianę z zewnątrz. Bez tego tekstura po jednej stronie byłaby lustrzanym odbiciem.
  Na kamieniu tego nie widać, ale napis albo strzałka wyszłyby odwrócone.
- UV zależy tylko od pozycji punktu, a nie od tego, który to model. Dwa odcinki ściany
  postawione obok siebie (przesunięte o 2 m, czyli o całe powtórzenie) mają więc wzór, który
  przechodzi z jednego w drugi bez szwu.
- Nic nie jest rozciągnięte. Sprawdziłem liczbowo: dla każdej krawędzi każdego trójkąta
  długość w metrach podzielona przez długość w UV daje dokładnie 2.

UV jest zapisane dla narożnika ściany (w Blenderze: loop), a nie dla wierzchołka. Ten sam
wierzchołek prostopadłościanu należy do trzech ścian i na każdej ma inne UV.

## 7. Tekstury

Tekstury generuje [`make_textures.py`](../../tools/blender/make_textures.py) samym numpy, bez
malowania. Obie mają 512 x 512 pikseli, 8 bitów na kanał, RGB bez kanału alfa.

| Plik | Wzór | Kamień | Fuga (joint) | Ziarno losowe (seed) |
|---|---|---|---|---|
| `wall_stone.png` | szare bloki w wiązaniu wozówkowym (running bond): co drugi rząd przesunięty o pół bloku | 128 x 64 px, czyli 0.5 x 0.25 m | 6 px | 11 |
| `floor_stone.png` | kwadratowe płyty w prostej siatce, ciemniejsze i cieplejsze od ściany | 128 x 128 px, czyli 0.5 x 0.5 m | 8 px | 23 |

Rząd bloków ma 0.25 m, tyle samo co cokół ściany, więc cokół to dokładnie jeden rząd, a w 3 m
ściany mieści się równo dwanaście rzędów.

Jak powstaje obraz (funkcja `stone_texture`):

1. Dla każdego piksela liczę, do którego kamienia należy (wiersz i kolumna) i jak daleko ma do
   najbliższej krawędzi swojego kamienia.
2. Każdy kamień dostaje jedną losową jasność.
3. Mnożę ją przez dwa szumy: duże miękkie plamy i drobne ziarno.
4. Przy krawędzi kamień jest ciemniejszy. To udaje zaokrąglenie krawędzi.
5. Piksele najbliżej krawędzi to fuga: dostają ciemny kolor fugi.

W grze nie ma jeszcze oświetlenia, więc cały kontrast musi być w samym obrazie. Stąd ciemne
fugi, ciemniejsze brzegi kamieni i wyraźne różnice jasności między kamieniami. Podłoga różni
się od ściany kolorem (ciepły brąz zamiast szarości), jasnością i wzorem (kwadraty w prostej
siatce zamiast podłużnych bloków z przesunięciem).

Dlaczego tekstury się **kafelkują (tile)**, czyli lewy brzeg pasuje do prawego, a dolny do
górnego:

- Rozmiary kamieni dzielą 512 bez reszty, więc na brzegu obrazu nie ma uciętego kamienia.
- Liczba rzędów ściany (8) jest parzysta, więc przesunięcie co drugiego rzędu zgadza się także
  między górnym a dolnym brzegiem.
- Blok przesuniętego rzędu, który przechodzi przez prawy brzeg, jest tym samym blokiem co
  kawałek przy lewym brzegu: numer kolumny liczę modulo liczba kolumn, więc obie połówki mają
  tę samą jasność.
- Szum jest wygładzany uśrednianiem z sąsiadami, a `np.roll` przenosi to, co wypada za jeden
  brzeg, na brzeg przeciwny. Piksel przy brzegu jest więc uśredniany z pikselami z drugiej
  strony obrazu.

Sprawdziłem to, składając z każdej tekstury obraz 2 x 2 i oglądając go: szwów nie widać.

Dlaczego wynik jest powtarzalny: jedyne źródło losowości to `np.random.default_rng(seed)` ze
stałym ziarnem. Zmiana ziarna daje inny układ jasnych i ciemnych kamieni.

Obraz zapisuje Blender (`image.save()`), a nie osobna biblioteka. Dwa szczegóły:

- Blender trzyma obraz od **dolnego** wiersza. Wiersz 0 tablicy to dół obrazu i `v = 0`.
- Zaokrąglenie do 256 poziomów robię sam (`np.round(color * 255) / 255`), żeby bajty w pliku
  nie zależały od sposobu zaokrąglania w Blenderze.

## 8. Lista assetów

Wymiary są w układzie gry i zostały odczytane z linii `v` gotowych plików.

| Model | Skrypt | x | y | z | Trójkąty | Tekstura |
|---|---|---|---|---|---|---|
| `wall_straight` | `build_wall_straight.py` | od -1 do 1 | od 0 do 3 | od -0.14 do 0.14 | 30 | `wall_stone.png` |
| `wall_pillar` | `build_wall_pillar.py` | od -0.2 do 0.2 | od 0 do 3.15 | od -0.2 do 0.2 | 30 | `wall_stone.png` |
| `floor_tile` | `build_floor_tile.py` | od -1 do 1 | 0 | od -1 do 1 | 2 | `floor_stone.png` |

**`wall_straight`**: odcinek ściany wzdłuż osi X. Trzy prostopadłościany jeden na drugim:

| Część | Wysokość (y) | Grubość (z) |
|---|---|---|
| cokół (plinth) | od 0 do 0.25 | 0.28 |
| korpus (body) | od 0.25 do 2.85 | 0.20 |
| nakrywa (coping) | od 2.85 do 3.0 | 0.28 |

Wszystkie trzy mają tę samą długość, więc oba końce (x = -1 i x = 1) są płaskie i odcinek
dochodzi równo do słupa. Pominięte są ściany, których nigdy nie widać: spód cokołu (leży na
podłodze) oraz spód i góra korpusu (zakryte przez cokół i nakrywę). Stąd 30 trójkątów zamiast
36.

**`wall_pillar`**: kwadratowy słup stawiany w węzłach siatki, tam gdzie spotykają się odcinki
ścian. Zasłania ich końce.

| Część | Wysokość (y) | Przekrój |
|---|---|---|
| podstawa (base) | od 0 do 0.35 | 0.4 x 0.4 |
| trzon (shaft) | od 0.35 do 2.90 | 0.3 x 0.3 |
| głowica (cap) | od 2.90 do 3.15 | 0.4 x 0.4 |

Trzon (0.3 m) jest szerszy od najgrubszej części ściany (0.28 m), więc koniec odcinka chowa
się w słupie. Słup jest o 0.15 m wyższy od ściany, a jego poziome ściany (0.35, 2.90 i 3.15)
leżą na innych wysokościach niż poziome ściany odcinka (0.25, 2.85 i 3.0). Żadne dwie nie
leżą w jednej płaszczyźnie, więc nie migoczą (sekcja 10).

**`floor_tile`**: płaski kwadrat 2 x 2 m na wysokości y = 0, zwrócony w górę (normalna +Y),
dwa trójkąty. Jedna płyta na komórkę labiryntu.

## 9. Blender MCP

Blenderem można też sterować na żywo z Claude Code, przez oficjalny serwer MCP z Blender Lab
(źródła: <https://projects.blender.org/lab/blender_mcp>). Uwaga na nazwę: pakiet `blender-mcp`
w PyPI to inny projekt, innego autora. Oficjalny serwer też instaluje program o nazwie
`blender-mcp`, więc o tym, który to serwer, decyduje źródło instalacji: katalog ze sklonowanego
repozytorium (`pip install C:/blsrc/mcp`), a nie nazwa pakietu z PyPI.

**Zapisane w repozytorium assety pochodzą zawsze ze skryptów z sekcji 3**, bez względu na to,
czy to połączenie działa. Połączenie na żywo, dalej nazywane mostem (bridge), służy do
oglądania modelu i szybkich prób. Cokolwiek tam ustalę (wymiar, kolor, proporcje), muszę
wpisać z powrotem do skryptu i wygenerować pliki od nowa. Inaczej zmiana istnieje tylko w
otwartym Blenderze i znika razem z nim.

### Windows

Instalacja serwera:

```powershell
git clone --depth 1 https://projects.blender.org/lab/blender_mcp.git C:/blsrc
python -m pip install C:/blsrc/mcp
```

Krótka ścieżka `C:/blsrc` jest konieczna. W głęboko zagnieżdżonym katalogu instalacja
przekracza limit 260 znaków ścieżki w Windowsie i kończy się mylącym komunikatem "No such file
or directory".

Zbudowanie i zainstalowanie dodatku (add-on) do Blendera (`blender` oznacza tu pełną ścieżkę
z sekcji 3):

```powershell
blender --command extension build --source-dir C:/blsrc/addon/blender_mcp_addon --output-dir C:/blsrc
blender --command extension install-file --repo user_default --enable C:/blsrc/mcp-1.0.0.zip
```

Dodatek nasłuchuje na `127.0.0.1:9876` i startuje razem z Blenderem. Dwa warunki:

- Blender musi być uruchomiony (zwykły, z oknem).
- W preferencjach Blendera musi być włączone "Allow Online Access". Bez tego dodatek **po
  cichu się nie uruchamia**: nie ma żadnego komunikatu o błędzie.

Rejestracja serwera w Claude Code:

```powershell
claude mcp add blender --scope local -- C:/Users/<nazwa>/AppData/Local/Programs/Python/Python313/Scripts/blender-mcp.exe
```

`--scope local` oznacza: tylko ten projekt i tylko ten komputer. Celowo nie ma tego wpisu w
pliku `.mcp.json` w repozytorium, bo ścieżka do programu jest inna na komputerze z Windowsem
i na Macu. Narzędzia serwera pojawiają się dopiero w nowej sesji Claude Code.

### macOS: do zrobienia

Na Macu most nie jest jeszcze skonfigurowany. Lista kroków:

1. Sklonować repozytorium serwera i zainstalować pakiet z katalogu `mcp` przez `pip`.
2. Zbudować dodatek poleceniem `extension build` i zainstalować go poleceniem
   `extension install-file`, używając programu
   `/Applications/Blender.app/Contents/MacOS/Blender`.
3. Włączyć "Allow Online Access" w preferencjach Blendera i uruchomić Blendera.
4. Znaleźć ścieżkę do programu `blender-mcp` na Macu i zarejestrować serwer poleceniem
   `claude mcp add blender --scope local -- <ścieżka>`.
5. Otworzyć nową sesję Claude Code i sprawdzić, czy narzędzia są widoczne.

## 10. Pułapki

| Pułapka | Co się dzieje | Jak tego unikam |
|---|---|---|
| Ręczna poprawka w pliku `.obj`, `.mtl` albo `.png` | następne uruchomienie skryptu ją nadpisuje | zmieniam skrypt i generuję od nowa |
| Scena fabryczna Blendera | zawiera sześcian, kamerę i światło, które trafiłyby do pliku | `reset_scene()` na początku każdego skryptu |
| Kamera dodana przed eksportem | `export_selected_objects=False` eksportuje całą scenę | rendery kontrolne są zawsze po `export_obj` |
| `path_mode='AUTO'` | w `.mtl` ląduje ścieżka bezwzględna z nazwą użytkownika, plik działa tylko na jednym komputerze | `path_mode='RELATIVE'` |
| Brak `Kd` w `.mtl` | Blender pomija `Kd`, gdy kolor pochodzi z tekstury | `add_diffuse_color` dopisuje białe `Kd` |
| Import modułu pomocniczego | Blender nie dodaje katalogu skryptu do ścieżki modułów, `import blender_common` się nie udaje | `sys.path.append(...)` na początku każdego skryptu |
| Katalog `__pycache__` | Python tworzy go w `tools/blender/` przy imporcie | wpis w `tools/blender/.gitignore` |
| Indeksy od 1 | indeksy w liniach `f` zaczynają się od 1, tablice w C++ od 0 | parser odejmuje 1 |
| Trzy indeksy na narożnik | OpenGL ma jeden indeks na wierzchołek | parser składa wierzchołek z pozycji, uv i normalnej dla każdej trójki |
| `v = 0` to dół obrazu | w konwencji OBJ i OpenGL `v` rośnie w górę, a plik PNG zaczyna się od górnego wiersza | kod wczytujący obraz musi odwrócić kolejność wierszy. Na kamieniu błędu prawie nie widać, na teksturze z napisem od razu |
| UV poza zakresem od 0 do 1 | z `GL_CLAMP_TO_EDGE` tekstura rozmazałaby się w pasy | tekstury tych modeli wymagają `GL_REPEAT` |
| Ściany w jednej płaszczyźnie (z-fighting) | dwie ściany na tej samej głębi migoczą, bo test głębi raz wybiera jedną, raz drugą | wysokości słupa są inne niż wysokości ściany (sekcja 8) |
| Końce linii | pliki mają LF. Git na Windowsie z włączonym `core.autocrlf=true` zamieniłby je przy pobraniu na CRLF | parser powinien odcinać znak `\r` z końca linii |
| Inna wersja Blendera | zmienia komentarz w pierwszej linii plików, a może też formatowanie liczb | generuję tą samą wersją (5.2.1) na obu komputerach |
| Przestrzeń barw | kolory w PNG są zapisane w sRGB | do rozstrzygnięcia przy wczytywaniu tekstur i oświetleniu |
| Rendery w repozytorium | obrazy kontrolne zaśmiecałyby historię | zapis tylko do katalogu tymczasowego |

## 11. Jak dodać nowy model

1. Skopiuj [`build_wall_pillar.py`](../../tools/blender/build_wall_pillar.py) jako
   `build_<nazwa>.py`. Nazwa w `snake_case`.
2. Zmień stałą `NAME`, komentarz na górze pliku i stałe z wymiarami. Wymiary w metrach, w
   układzie Blendera: Z to wysokość, podłoga to z = 0, środek podstawy w punkcie (0, 0, 0).
3. Zbuduj geometrię. Prostopadłościany dodaje `common.add_box`. Ściany, których nigdy nie
   widać, wypisz w `skip`. Inny kształt to własna lista wierzchołków i ścian, tak jak w
   [`build_floor_tile.py`](../../tools/blender/build_floor_tile.py): narożniki każdej ściany
   przeciwnie do ruchu wskazówek zegara, patrząc z zewnątrz.
4. Zostaw kolejność wywołań: `create_mesh_object`, `box_project_uvs`,
   `assign_textured_material`, `export_obj`, a na końcu opcjonalnie `render_review_shots`.
   Rzut pudełkowy pasuje do ścian prostopadłych do osi. Model ze skośnymi ścianami będzie
   potrzebował innego sposobu liczenia UV.
5. Jeśli model potrzebuje nowej tekstury, dodaj wywołanie `stone_texture` i `save_png` w
   funkcji `build` w [`make_textures.py`](../../tools/blender/make_textures.py). Rozmiary
   kamieni muszą dzielić 512 bez reszty.
6. Dopisz `import` i wywołanie `build(shots)` w
   [`make_all.py`](../../tools/blender/make_all.py).
7. Uruchom skrypt z `-- --shots` i obejrzyj rendery.
8. Otwórz plik `.obj` jako tekst: sprawdź, czy wysokość jest w drugiej liczbie linii `v` i czy
   wymiary się zgadzają. W pliku `.mtl` sprawdź `Kd` i względną ścieżkę w `map_Kd`.
9. Uruchom skrypt drugi raz i sprawdź, że `git status` nie pokazuje nowych zmian: wynik ma
   być powtarzalny.
10. Dopisz model do tabeli w sekcji 8 tego dokumentu.

## 12. Powiązane dokumenty

- Układ współrzędnych gry: [`../modules/scene/README.md`](../modules/scene/README.md)
- Struktura repozytorium: [`project-structure.md`](project-structure.md)
- Skąd program bierze katalog `assets`: [`../modules/core/paths.md`](../modules/core/paths.md)
- Dokumentacja Blendera, API Pythona: <https://docs.blender.org/api/current/>
- Opis formatu OBJ i MTL: <https://paulbourke.net/dataformats/obj/> oraz
  <https://paulbourke.net/dataformats/mtl/>
