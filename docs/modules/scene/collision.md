# Moduł scene: kolizje, AABB i przesuwanie wzdłuż ścian

Kamień milowy: M2 + M3. Temat wykładu: 14 (Wstęp do kolizji).
Kod: [`src/scene/Collider.hpp`](../../../src/scene/Collider.hpp), [`src/scene/Collider.cpp`](../../../src/scene/Collider.cpp), testy w [`tests/ColliderTests.cpp`](../../../tests/ColliderTests.cpp). Rysowanie pudełek: [`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp), shadery [`assets/shaders/color.vert`](../../../assets/shaders/color.vert) i [`assets/shaders/color.frag`](../../../assets/shaders/color.frag), panel w [`src/debug/panels/CollisionPanel.hpp`](../../../src/debug/panels/CollisionPanel.hpp) i [`src/debug/panels/CollisionPanel.cpp`](../../../src/debug/panels/CollisionPanel.cpp). Użytkownicy: [`src/game/MazeLayout.cpp`](../../../src/game/MazeLayout.cpp) (pudełka ścian i słupków) i [`src/game/Player.cpp`](../../../src/game/Player.cpp) (ruch gracza).

Część modułu `scene`. Wstęp do modułu i jego miejsce w warstwach są w [`README.md`](README.md). Ten dokument korzysta z biblioteki GLM ([`../../libraries/glm.md`](../../libraries/glm.md): `vec3`, dodawanie i odejmowanie wektorów) i odwołuje się do stałego kroku symulacji z [`../core/main-loop.md`](../core/main-loop.md), sekcje 2.2 i 2.3. Testy są napisane w bibliotece doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)). Skąd biorą się pudełka ścian labiryntu, opisuje [`../game/maze-generator.md`](../game/maze-generator.md), sekcje 2.7 i 5.7. Gracza, który z kolizji korzysta, opisuje [`../game/player.md`](../game/player.md).

## 1. Po co to jest

Kamera z kamienia milowego M1 latała bez przeszkód: nic jej nie zatrzymywało, więc przelatywała przez kostkę. W labiryncie gracz ma chodzić po korytarzach i zatrzymywać się na ścianach. Potrzebne są do tego dwie rzeczy:

1. **wykrywanie kolizji** (collision detection): odpowiedź na pytanie "czy te dwa obiekty na siebie nachodzą",
2. **reakcja na kolizję** (collision response): decyzja, co zrobić z ruchem, który doprowadziłby do nachodzenia.

Obie realizuje plik `Collider`: struktura `scene::Aabb` (pudełko o ścianach równoległych do osi), funkcja `scene::overlaps` (wykrywanie) i funkcja `scene::moveAndSlide` (reakcja: ruch, który zatrzymuje się na przeszkodzie, ale ślizga się wzdłuż niej).

Tak jak `Transform` i `Camera`, to zwykłe dane i matematyka: żadnego wywołania OpenGL, żadnej klawiatury, żadnego czasu. Dzięki temu cały kod da się sprawdzić testami jednostkowymi bez okna.

Do pokazu i do szukania błędów dochodzą dwie rzeczy z programu `night_maze`: klasa `game::ColliderLines`, która rysuje pudełka cienkimi liniami (sekcje 3, 4 i 5.8), i panel Collision (sekcja 6).

**Stan na dziś, uczciwie.** Kod kolizji ma testy: 12 przypadków w `tests/ColliderTests.cpp`, trzy dalsze, razem z labiryntem, w `tests/MazeLayoutTests.cpp` i cztery z graczem w `tests/PlayerTests.cpp`. Pudełka ścian i słupków labiryntu liczy `game::mazeColliders`, a `moveAndSlide` woła gracz w każdym kroku chodzenia (`game::Player::update`). Na Windowsie (2026-10-05, MSVC 19.44) wszystkie testy przechodzą w Debug i Release, a na zrzucie ekranu z widoku z góry żółte linie pudełek leżą na ścianach i słupkach. Chodzenia i ślizgania prawdziwymi klawiszami oraz widżetów panelu Collision **nikt jeszcze nie sprawdził ręcznie**. Na macOS kod nie był budowany ani uruchamiany.

## 2. Teoria

### 2.1 Bryła otaczająca i AABB

Model ściany ma kilkadziesiąt trójkątów, model postaci tysiące. Sprawdzanie każdego trójkąta z każdym byłoby wolne i trudne. Dlatego do kolizji używa się **bryły otaczającej** (bounding volume): prostej figury, która zawiera cały obiekt i dla której test jest tani. Kolizję obiektów zastępuje kolizja ich brył.

Najprostsze bryły:

| Bryła | Opis | Test z taką samą bryłą | Wada |
|---|---|---|---|
| kula (sphere) | środek i promień | jedna odległość | źle pasuje do długich i płaskich obiektów, na przykład do ściany |
| AABB (axis-aligned bounding box) | pudełko o krawędziach równoległych do osi X, Y i Z świata | trzy porównania przedziałów | nie obraca się razem z obiektem |
| OBB (oriented bounding box) | pudełko dowolnie obrócone | do 15 osi rozdzielających | dużo trudniejszy test |

**AABB** to prostopadłościan, którego nie wolno obracać. To ograniczenie jest jego siłą: skoro krawędzie są zawsze równoległe do osi, do pełnego opisu wystarczą **dwa narożniki**: `min` (najmniejsze x, y i z) i `max` (największe x, y i z). Pozostałe sześć narożników to kombinacje ich współrzędnych.

```text
widok z góry (płaszczyzna XZ), jedno pudełko:

        z
        ^            max = (4, _, 3)
      3 |    +---------+
        |    |         |
      1 |    +---------+
        |  min = (1, _, 1)
        +----+---------+----> x
             1         4
```

Ten sam prostopadłościan można opisać inaczej: **środkiem** i **połowami rozmiarów** (half extents). Dla pudełka z rysunku środek to `(2,5, _, 2)`, a połowy rozmiarów to `(1,5, _, 1)`. Zamiana jest prosta: `min = środek - połowy`, `max = środek + połowy`. Postać ze środkiem jest wygodna przy tworzeniu pudełka ("gracz stoi tutaj i ma 0,6 m szerokości"), postać `min` i `max` przy testach.

Labirynt pasuje do AABB idealnie: każda ściana biegnie albo wzdłuż osi X, albo wzdłuż osi Z, więc jej pudełko przylega do niej bez żadnego zapasu. Ściana obrócona o 90 stopni to nadal AABB, tylko z zamienioną długością i grubością.

### 2.2 Test nakładania: przedziały na osiach

Patrząc wzdłuż jednej osi, pudełko jest zwykłym **przedziałem** od `min` do `max`. Dwa przedziały mają część wspólną, która zaczyna się w większym z dwóch początków, a kończy w mniejszym z dwóch końców. Jej długość to:

```text
wspólna długość = min(a.max, b.max) - max(a.min, b.min)
```

```text
oś x:   0    1    2    3    4    5    6    7
A            |==============|                     A: od 1 do 4
B                      |==============|           B: od 3 do 6
wspólne                |====|                     min(4, 6) - max(1, 3) = 4 - 3 = 1    (nakładają się)

A            |=========|                          A: od 1 do 3
B                      |==============|           B: od 3 do 6
wspólne                |                          min(3, 6) - max(1, 3) = 3 - 3 = 0    (stykają się)

A            |====|                               A: od 1 do 2
B                           |=========|           B: od 4 do 6
wspólne                                           min(2, 6) - max(1, 4) = 2 - 4 = -2   (odstęp 2)
```

Jedna liczba mówi wszystko: dodatnia to długość części wspólnej, zero to styk, ujemna to wielkość odstępu.

**Dwa pudełka nachodzą na siebie wtedy i tylko wtedy, gdy ich przedziały nakładają się na wszystkich trzech osiach.** Wystarczy jedna oś z odstępem, żeby pudełka były rozłączne: między nimi mieści się wtedy płaszczyzna prostopadła do tej osi. To szczególny przypadek twierdzenia o osi rozdzielającej (separating axis theorem): dwie wypukłe bryły są rozłączne, jeśli istnieje oś, na której ich rzuty się nie nakładają. Dla AABB wystarczy sprawdzić trzy osie świata.

```text
widok z góry: przedziały nakładają się na x, ale nie na z, więc pudełka są rozłączne

        z
        ^
      6 |         +-------+
        |         |   B   |
      4 |         +-------+
      3 |    +---------+        <- między z = 3 a z = 4 mieści się linia
        |    |    A    |           (w 3D: płaszczyzna) rozdzielająca
      1 |    +---------+
        +----+----+----+--+----> x
             1    3    4  5
```

Koszt testu to sześć porównań i żadnego pierwiastka ani dzielenia.

### 2.3 Dotyk to nie nakładanie

Co, gdy wspólna długość wynosi dokładnie zero, czyli ściana jednego pudełka leży na ścianie drugiego? W tym projekcie **styk nie jest nakładaniem**: `overlaps` wymaga długości większej od zera na każdej osi. Powód jest praktyczny: gracz, który doszedł do ściany, stoi do niej przytulony, a segmenty ścian labiryntu stykają się ze sobą końcami. Gdyby styk liczył się jako kolizja, gracz oparty o ścianę byłby "w kolizji" przez cały czas.

Styk wspólną krawędzią albo samym narożnikiem też nie jest nakładaniem: wtedy zero wychodzi na dwóch albo na trzech osiach.

### 2.4 Wykrywanie dyskretne i z przemiataniem

Są dwa sposoby pytania o kolizję ruchomego obiektu.

**Wykrywanie dyskretne** (discrete): przesuń obiekt na nową pozycję i sprawdź, czy tam na coś nachodzi. Jeśli tak, cofnij go albo wypchnij. To jeden test `overlaps` na krok. Proste, ale widzi tylko **koniec** ruchu, a nie drogę.

**Wykrywanie z przemiataniem** (swept, continuous): zapytaj, **jak daleko** obiekt może się przesunąć, zanim czegoś dotknie. Test bada całą drogę, a wynikiem nie jest "tak albo nie", tylko odległość.

```text
krok dyskretny                         przemiatanie

  przed       ściana     po              przed       ściana
  +---+        ||       +---+            +---+        ||
  | A |        ||       | A |            | A |------->||      "do ściany jest 0,7 m"
  +---+        ||       +---+            +---+  odstęp ||
                                                 0,7
  test na końcu: brak nakładania,
  obiekt przeskoczył ścianę
```

### 2.5 Tunelowanie

Lewa strona rysunku to **tunelowanie** (tunnelling): obiekt przechodzi przez przeszkodę, bo w żadnej sprawdzanej chwili na nią nie nachodzi. W wykrywaniu dyskretnym zdarza się to, gdy krok jest dłuższy niż łączna grubość obiektu i przeszkody: przed krokiem obiekt jest w całości po jednej stronie, po kroku w całości po drugiej.

Liczby dla tego projektu: pudełko ściany ma 0,3 m grubości, pudełko gracza 0,6 m. Krok dyskretny dłuższy niż 0,9 m przeskoczyłby ścianę. Przy chodzie 3 m/s taki krok oznacza czas klatki 0,3 s, a przy sprincie 5,5 m/s już 0,16 s. `core::Time` obcina czas klatki do 0,25 s (`MAX_FRAME_TIME`), więc test dyskretny liczony z czasu klatki chroniłby chód tylko dzięki obcięciu, a sprintu nie chroniłby wcale.

Dwa lekarstwa:

1. **krótkie kroki o stałej długości**: symulacja idzie krokiem `FIXED_DT` (1/120 s), niezależnie od czasu klatki. Przy 3 m/s jeden krok to 2,5 cm,
2. **przemiatanie**: test, który z definicji nie może niczego przeskoczyć.

Projekt używa obu naraz: `moveAndSlide` mierzy odstęp na każdej osi (sekcja 2.6), a ruch ma być liczony w stałym kroku (sekcja 2.8).

### 2.6 Reakcja oś po osi: skąd bierze się ślizganie

Najprostsza reakcja to "jeśli ruch kończy się kolizją, nie ruszaj się". Gracz idący ukosem na ścianę stanąłby wtedy w miejscu, jakby się do niej przykleił. W grze oczekuję czegoś innego: gracz ma **ślizgać się** wzdłuż ściany, tracąc tylko tę część ruchu, która jest skierowana w ścianę.

Sposób: rozłożyć przesunięcie na składowe i każdą oś obsłużyć osobno, jedna po drugiej. Kolejność w projekcie to **x, potem z, potem y**.

```text
widok z góry: ruch ukośny (1,0, _, 0,5) w stronę ściany stojącej 0,7 m dalej na osi x

   z
   ^                         ||
   |        chciany koniec   ||
   |              . *        ||           krok 1, oś x: do ściany jest 0,7,
   |          .              ||                   więc z 1,0 zostaje 0,7
   |      .         ^        ||           krok 2, oś z: na tej osi ściany
   |  .             | 0,5    ||                   nie ma na drodze, zostaje 0,5
   +---+----------->+        ||
   | A |    0,7              ||           wynik: (0,7, _, 0,5)
   +---+                     ||
   +-------------------------------> x
```

Dlaczego to działa. Dla jednej osi pytanie brzmi: **co stoi w korytarzu, który pudełko zakreśla, jadąc wzdłuż tej osi?** Przeszkoda jest w korytarzu tylko wtedy, gdy nakłada się z pudełkiem na obu pozostałych osiach. Jeśli jest w korytarzu i przed pudełkiem, wolna droga to **odstęp** między ścianą pudełka zwróconą w kierunku ruchu a ścianą przeszkody zwróconą do niej. Dozwolony ruch to mniejsza z dwóch liczb: chciany ruch albo najmniejszy odstęp.

Dla pudełka opartego o ścianę:

- na osi prostopadłej do ściany odstęp wynosi zero, więc ruch w ścianę jest w całości obcięty,
- na osi równoległej do ściany ściana **nie leży w korytarzu ruchu**: jest obok, styka się z pudełkiem, ale się z nim nie nakłada. Ruch wzdłuż ściany przechodzi w całości.

To jest całe ślizganie: nie ma żadnego osobnego kodu "ślizgaj się", jest tylko niezależna ocena każdej osi.

Druga ważna rzecz: następna oś zaczyna od miejsca, w którym skończyła poprzednia. Po obsłużeniu osi x pudełko jest przesunięte o dozwolone x, i dopiero dla takiego pudełka liczę oś z.

Test na jednej osi jest **przemiataniem** w jednym wymiarze: mierzy odstęp, zamiast sprawdzać pozycję końcową. Dlatego na pojedynczej osi tunelowanie nie występuje, niezależnie od długości kroku (zmierzone: krok 50 m w stronę ściany testowej o grubości 0,2 m kończy się na ścianie, sekcja 5.7).

### 2.7 Tolerancja styku: liczby `float` nie są dokładne

Typ `float` ma około 7 cyfr znaczących. W okolicy współrzędnej 37 sąsiednie liczby `float` są od siebie oddalone o około 0,000004. Funkcja zwraca przesunięcie, wołający dodaje je do pozycji, a przy następnym kroku buduje pudełko od nowa z pozycji i połowy rozmiarów. Każde z tych działań zaokrągla wynik. Ściana pudełka, która miała stanąć **dokładnie** na ścianie przeszkody, ląduje więc czasem kilka milionowych metra przed nią, a czasem kilka milionowych **w środku**.

Bez zabezpieczenia miałoby to dwa skutki:

- pudełko zagłębione o jedną milionową metra w ścianę nakłada się z nią na osi prostopadłej, więc ściana trafia do korytarza ruchu **wzdłuż** siebie i może ten ruch zablokować. Gracz ślizgający się po ścianie stawałby co jakiś czas bez powodu,
- pudełko zagłębione o jedną milionową nie jest już "przed" ścianą (odstęp jest ujemny), więc ściana mogłaby przestać je zatrzymywać.

Oba skutki są zmierzone: po usunięciu tolerancji z kodu pudełko pchane na ścianę zamkniętej komórki wychodzi z niej na zewnątrz, a trzy przypadki testowe przestają przechodzić (ćwiczenie 7).

Rozwiązaniem jest **tolerancja styku** (stała `CONTACT_TOLERANCE`, 1 mm): w `moveAndSlide` nakładanie płytsze niż 1 mm jest traktowane jak dotyk. Wartość jest dobrana między dwiema skalami: jest kilkaset razy większa od błędu zaokrąglenia (milionowe części metra) i trzysta razy mniejsza od grubości pudełka ściany (0,3 m), a milimetra na ekranie nie widać.

`overlaps` tolerancji nie ma: odpowiada na pytanie geometryczne dokładnie tak, jak je zadano. Tolerancja należy do ruchu, bo to ruch produkuje błędy zaokrągleń.

### 2.8 Ograniczenie: droga po schodkach i stały krok

Obsługa osi jedna po drugiej oznacza, że pudełko nie jedzie po prostej. Najpierw pokonuje całe x, potem całe z, potem całe y: porusza się po krawędziach "schodka".

```text
widok z góry: prosta przekątna a droga faktycznie sprawdzana

   z
   ^
   |                  * koniec
   |               .  ^
   |            #     |          # mały słupek leżący dokładnie na przekątnej
   |         .        |  drugi
   |      .           |  odcinek: z
   |   .              |
   +---+------------->+
   | A | pierwszy odcinek: x
   +---+
   +-------------------------> x
```

Każdy z dwóch odcinków jest sprawdzany dokładnie. Ale słupek z rysunku nie leży w korytarzu żadnego z nich, więc ruch przechodzi, choć prosta przekątna by w niego trafiła. Odwrotna sytuacja też jest możliwa: schodek zahacza o narożnik, który przekątna by ominęła. Wynik przy narożniku wypukłym zależy też od kolejności osi.

Wielkość tego błędu jest taka jak długość kroku. Słupek z rysunku da się "obejść" tylko wtedy, gdy krok na obu osiach naraz jest dłuższy niż pudełko i przeszkoda razem. Dlatego funkcja zakłada **krótkie kroki**:

| Skąd pochodzi przesunięcie | Długość kroku przy 3 m/s | W porównaniu z pudełkiem ściany 0,3 m |
|---|---|---|
| stały krok `FIXED_DT` = 1/120 s | 2,5 cm | 12 razy krótszy |
| czas klatki przy 60 FPS | 5 cm | 6 razy krótszy |
| najdłuższy dopuszczalny czas klatki `MAX_FRAME_TIME` = 0,25 s | 75 cm | 2,5 raza dłuższy |

Przesunięcie dla `moveAndSlide` musi więc powstawać w `onUpdate`, z `fixedDt`, a nie z czasu klatki. To ten sam powód, dla którego cały ruch gracza jest liczony stałym krokiem ([`../core/main-loop.md`](../core/main-loop.md), sekcja 2.2): symulacja ma dawać ten sam wynik przy każdej liczbie klatek na sekundę.

Oba przypadki z rysunku są zmierzone testem (sekcja 5.7): jeden krok 2 m po przekątnej mija słupek, a ta sama droga w 80 krokach po 2,5 cm na niego trafia i musi go obejść.

### 2.9 Co dalej: kula dla kryształów

Do ścian AABB wystarcza. Kryształy, które gracz ma zbierać (M5), są małe i mniej więcej okrągłe, a pytanie brzmi "czy gracz jest dość blisko". Do tego lepiej pasuje **kula**. Tych testów w kodzie jeszcze nie ma, to zapowiedź:

**Kula z kulą.** Dwie kule nachodzą na siebie, gdy odległość ich środków jest mniejsza od sumy promieni. Żeby nie liczyć pierwiastka, porównuje się kwadraty:

```text
dot(c1 - c2, c1 - c2) < (r1 + r2) * (r1 + r2)
```

**Kula z AABB.** Najpierw znajduje się punkt pudełka najbliższy środkowi kuli: każdą współrzędną środka przycina się osobno do przedziału pudełka. Potem sprawdza się, czy ten punkt leży wewnątrz kuli:

```text
najbliższy = clamp(środek, box.min, box.max)        (osobno dla x, y i z)
kolizja, gdy dot(środek - najbliższy, środek - najbliższy) < r * r
```

Jeśli środek kuli jest w pudełku, przycinanie go nie zmienia, odległość wychodzi 0 i test daje kolizję. Oba testy opisuje Ericson (sekcja 10), a wersję 2D rozdział "Collision detection" z LearnOpenGL.

Poza zakresem projektu zostają: bryły obrócone (OBB), siatki trójkątów, struktury przyspieszające (siatka, drzewo BVH) i pełna symulacja fizyki z masą, pędem i odbiciami ([`../../decisions/collision-aabb-sliding.md`](../../decisions/collision-aabb-sliding.md)).

## 3. Jak to działa w OpenGL

Same kolizje to czysta matematyka na procesorze. `Collider.hpp` i `Collider.cpp` nie dołączają GLAD i nie wołają żadnej funkcji `gl*`. OpenGL nie wie, że jakieś pudełka istnieją, i niczego nie sprawdza: karta graficzna narysuje dwa obiekty jeden w drugim bez żadnego błędu.

Związek z renderowaniem jest pośredni: wynik `moveAndSlide` zmienia pozycję gracza, z pozycji gracza powstaje punkt oka, a z niego macierz widoku ([`../game/player.md`](../game/player.md), sekcja 5). Kolizja decyduje więc o tym, **skąd** rysowana jest klatka, a nie o tym, jak.

OpenGL pojawia się dopiero przy **rysowaniu pudełek** jako pomocy diagnostycznej (klasa `game::ColliderLines`, sekcja 5.8). Pudełko jest rysowane jako 12 krawędzi sześcianu, liniami.

**Prymityw `GL_LINES`.** Do tej pory wszystko było rysowane trójkątami (`GL_TRIANGLES`: każde trzy indeksy to jeden trójkąt). Pierwszy parametr `glDrawElements` może też wskazać linie: przy `GL_LINES` **każde dwa indeksy to jeden odcinek**. Sześcian ma 8 narożników i 12 krawędzi, więc wystarcza 8 wierzchołków i 24 indeksy. Ta sama klasa `gfx::Mesh` obsługuje oba przypadki: rodzaj prymitywu jest parametrem jej konstruktora ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5).

| Krok w klatce (gdy rysowanie pudełek jest włączone) | Wywołania OpenGL | Ile razy dla labiryntu 10 na 10 |
|---|---|---|
| `m_colorShader.use()` | `glUseProgram` | 1 |
| `setMat4` dla `uView` i `uProjection` | `glGetUniformLocation`, `glUniformMatrix4fv` | po 1 |
| `setVec3(COLOR_UNIFORM, color)` | `glGetUniformLocation`, `glUniform3fv` | 2 (żółty dla labiryntu, zielony dla gracza) |
| `setMat4(MODEL_UNIFORM, ...)` | `glGetUniformLocation`, `glUniformMatrix4fv` | 243 (121 ścian, 121 słupków, 1 gracz) |
| `m_unitCube.draw()` | `glBindVertexArray`, `glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, ...)` | 243 |

Trzy szczegóły:

- **Jedna siatka dla wszystkich pudełek.** Na karcie leży jeden sześcian o boku 1. Każde pudełko to ten sześcian z inną macierzą modelu: skala równa rozmiarowi pudełka i przesunięcie do jego narożnika `min`. To ta sama zasada co przy ścianach labiryntu ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 2).
- **Szerokość linii to 1 piksel.** Kod nie woła `glLineWidth`. Profil Core nie musi obsługiwać linii szerszych niż 1, a implementacja OpenGL w macOS jest znana z tego, że ich nie obsługuje (tak mówi też komentarz w kodzie, na Macu tego nie sprawdzałem), więc wartość domyślna jest jedyną przenośną.
- **Test głębi zostaje włączony.** Linia za ścianą jest przez nią zasłonięta. Dzięki temu widać, gdzie pudełko naprawdę jest, a nie plątaninę wszystkich krawędzi labiryntu naraz.

**Walka o głębię (z-fighting).** Pudełko słupka ma dokładnie szerokość trzonu modelu słupka (0,3 m). Linie narysowane w prawdziwym rozmiarze leżałyby więc **w** powierzchni modelu: dla tych samych pikseli linia i ściana miałyby prawie tę samą głębię, a o tym, co wygra test głębi, decydowałyby błędy zaokrągleń, inne w każdej klatce. Linie migotałyby. Rozwiązanie w projekcie jest najprostsze z możliwych: rysowane pudełko jest większe o 1 cm z każdej strony, więc linie są wyraźnie przed powierzchnią. Kolizje nadal liczą się na prawdziwych pudełkach.

## 4. Shadery

Same kolizje nie mają shadera. Linie pudełek rysuje najprostsza para w projekcie: [`assets/shaders/color.vert`](../../../assets/shaders/color.vert) i [`assets/shaders/color.frag`](../../../assets/shaders/color.frag). Wszystko, co nimi narysowane, ma jeden kolor. Od M4 ta sama para rysuje jeszcze jedną rzecz: małe kostki w miejscach świateł punktowych (`game::LightRig::drawMarkers`, [`../game/flashlight.md`](../game/flashlight.md), sekcja 5.7). Shadery się przy tym nie zmieniły.

### 4.1 `color.vert`

```glsl
#version 410 core
// Vertex shader for shapes drawn in one flat colour: the lines of the collision boxes.
// See docs/modules/scene/collision.md

// Input: only the position. The mesh also carries a normal (location 1) and a texture
// coordinate (location 2), but a shader may leave attributes it does not need unread.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the shape

// Uniforms: set from C++ (gfx::Shader::setMat4).
uniform mat4 uModel;      // local space to world space
uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space

void main() {
    // The same chain as in basic.vert: local, world, view, clip space.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `#version 410 core` | GLSL 4.10, profil Core: ta sama wersja co kontekst OpenGL projektu |
| `layout(location = 0) in vec3 aPosition;` | jedyne wejście: pozycja, atrybut numer 0. Numer zgadza się ze stałą `POSITION_ATTRIBUTE` z `src/gfx/Vertex.hpp` |
| brak `aNormal` i `aUv` | siatka `gfx::Mesh` zawsze opisuje trzy atrybuty (pozycja, normalna, uv). Shader nie musi czytać wszystkich: atrybut włączony w VAO, którego shader nie deklaruje, jest po prostu ignorowany |
| `uniform mat4 uModel;`, `uView`, `uProjection` | te same trzy macierze i te same nazwy co w `basic.vert` i `textured.vert`, dzięki czemu kod C++ używa dla wszystkich programów tych samych stałych z `ShaderUniforms.hpp` ([`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 5) |
| `gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);` | łańcuch czytany od prawej: przestrzeń lokalna, świat, widok, przycinanie ([`transforms.md`](transforms.md), sekcja 4). `1.0` jako czwarta składowa oznacza punkt, więc przesunięcie z macierzy działa |

Shader nie ma żadnego wyjścia poza `gl_Position`: fragmentom nie trzeba niczego przekazywać, bo kolor jest ten sam dla całego kształtu.

### 4.2 `color.frag`

```glsl
#version 410 core
// Fragment shader for shapes drawn in one flat colour: the lines of the collision boxes.
// See docs/modules/scene/collision.md

// The colour of the whole shape (red, green, blue), set from C++ (gfx::Shader::setVec3).
uniform vec3 uColor;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // Every fragment gets the same colour. Alpha 1 means fully opaque.
    fragColor = vec4(uColor, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `uniform vec3 uColor;` | kolor całego kształtu. To uniform, a nie atrybut: jest stały dla wszystkich wierzchołków jednego wywołania rysującego, a zmienia się między wywołaniami |
| `out vec4 fragColor;` | wyjście shadera fragmentów: kolor zapisywany do framebuffera |
| `fragColor = vec4(uColor, 1.0);` | trzy składowe koloru i alfa 1, czyli pełne krycie |

Różnica wobec `basic.frag`: tam kolor przychodził z wierzchołków i był interpolowany, tutaj przychodzi jako jedna liczba dla wszystkich fragmentów. Różnica wobec `textured.frag`: nie ma tekstury ani żadnego wejścia z shadera wierzchołków.

Kolory ustawia `NightMazeApp` (sekcja 5.8): żółty `(1, 0,85, 0,1)` dla pudełek labiryntu i zielony `(0,2, 1, 0,4)` dla pudełka gracza.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/scene/Collider.hpp`](../../../src/scene/Collider.hpp) | stała `CONTACT_TOLERANCE`, struktura `Aabb` z funkcją `fromCenter`, deklaracje `overlaps` i `moveAndSlide` |
| [`src/scene/Collider.cpp`](../../../src/scene/Collider.cpp) | stałe osi, funkcje pomocnicze `sharedLength` i `allowedDistance`, definicje trzech funkcji publicznych |
| [`tests/ColliderTests.cpp`](../../../tests/ColliderTests.cpp) | 12 przypadków testowych samych kolizji |
| [`tests/MazeLayoutTests.cpp`](../../../tests/MazeLayoutTests.cpp) | trzy przypadki łączące kolizje z labiryntem (zamknięta komórka, wędrówka po labiryncie, ślizganie obok słupków) |
| [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`.cpp`](../../../src/game/MazeLayout.cpp) | pierwszy użytkownik `Aabb`: `wallBox`, `pillarBox`, `mazeColliders` ([`../game/maze-generator.md`](../game/maze-generator.md), sekcja 5.7) |
| [`src/game/Player.hpp`](../../../src/game/Player.hpp), [`.cpp`](../../../src/game/Player.cpp) | użytkownik `moveAndSlide`: `Player::box` i `Player::update` (sekcja 5.6 i [`../game/player.md`](../game/player.md)) |
| [`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`.cpp`](../../../src/game/ColliderLines.cpp) | rysowanie pudełek liniami (sekcja 5.8) |
| [`assets/shaders/color.vert`](../../../assets/shaders/color.vert), [`color.frag`](../../../assets/shaders/color.frag) | shadery jednego koloru (sekcja 4) |
| [`src/debug/panels/CollisionPanel.hpp`](../../../src/debug/panels/CollisionPanel.hpp), [`.cpp`](../../../src/debug/panels/CollisionPanel.cpp) | panel Collision (sekcja 6) |

Pliki `src/scene/Collider.*` należą do biblioteki `engine`, tak jak reszta `src/scene/`. Nie ma w nich nic specyficznego dla Night Maze: pudełko nie wie, czy jest ścianą, graczem czy skrzynią. `ColliderLines.*` i `CollisionPanel.*` należą do programu `night_maze`, bo wymagają kontekstu OpenGL albo ImGui.

Dołączane nagłówki: `<glm/glm.hpp>` i `<span>` w nagłówku, `<algorithm>` (`std::min`, `std::max`), `<array>` i `<cmath>` (`std::abs`) w pliku `.cpp`. Nic z `core/`, `gfx/`, GLAD ani GLFW.

### 5.2 `CONTACT_TOLERANCE` i struktura `Aabb`

```cpp
constexpr float CONTACT_TOLERANCE = 0.001F;
```

Jeden milimetr (sekcja 2.7). Stała jest publiczna, w nagłówku, bo korzystają z niej testy: sprawdzają, że pudełko nigdy nie wchodzi w przeszkodę głębiej, niż ona pozwala.

```cpp
struct Aabb {
    /// The corner with the smallest x, y and z.
    glm::vec3 min{0.0F};

    /// The corner with the largest x, y and z. Every component must not be smaller than
    /// the same component of min.
    glm::vec3 max{0.0F};

    /// Builds a box from its centre and its half extents: half of the width (x), half of
    /// the height (y) and half of the depth (z).
    static Aabb fromCenter(const glm::vec3& center, const glm::vec3& halfExtents);
};
```

| Element | Znaczenie |
|---|---|
| `glm::vec3 min{0.0F};` | narożnik o najmniejszych współrzędnych. `{0.0F}` wypełnia zerem wszystkie trzy składowe: `glm::vec3` bez inicjalizatora miałby wartości nieokreślone ([`../../libraries/glm.md`](../../libraries/glm.md), pułapka 2) |
| `glm::vec3 max{0.0F};` | narożnik o największych współrzędnych |
| `static Aabb fromCenter(...)` | funkcja statyczna: woła się ją na typie, `scene::Aabb::fromCenter(...)`, bez istniejącego obiektu. Zastępuje konstruktor "ze środka" |

Trzy decyzje:

- **Struktura z publicznymi polami, bez konstruktora.** `Aabb` to dwie trójki liczb bez żadnego zasobu, tak jak `Transform` i `Camera` ([`README.md`](README.md), sekcja 3). Dzięki temu jest agregatem i można ją tworzyć inicjalizatorami desygnowanymi z C++20: `scene::Aabb{.min = {1.0F, 0.0F, -5.0F}, .max = {1.2F, 3.0F, 5.0F}}`. Nazwy pól w takim zapisie chronią przed zamianą narożników miejscami.
- **`fromCenter` jako funkcja statyczna, a nie konstruktor.** Konstruktor z dwoma `glm::vec3` wyglądałby tak samo jak "min i max" i łatwo byłoby je pomylić. Nazwa mówi, które dwie trójki podaję. Zwykły konstruktor odebrałby też strukturze status agregatu.
- **Warunek `min <= max` nie jest sprawdzany.** Pudełko z `min` większym od `max` ma ujemną "wspólną długość" z każdym innym i po cichu z niczym nie koliduje (pułapka 3). Sprawdzanie przy każdym teście kosztowałoby więcej niż sam test. Kolejność narożników jest więc obowiązkiem tego, kto tworzy pudełko: wynik `fromCenter` jest poprawny, dopóki połowy rozmiarów są nieujemne (funkcja tego nie sprawdza), a przy inicjalizatorze z polami `.min` i `.max` trzeba samemu podać mniejszy narożnik jako `min`. Kod gry tworzy pudełka tylko przez `fromCenter` ze stałych dodatnich połów rozmiarów (`game::wallBox`, `game::pillarBox`).

```cpp
Aabb Aabb::fromCenter(const glm::vec3& center, const glm::vec3& halfExtents) {
    return {.min = center - halfExtents, .max = center + halfExtents};
}
```

Odejmowanie i dodawanie wektorów działa składowa po składowej, więc jedna linia liczy wszystkie trzy osie.

### 5.3 `sharedLength` i `overlaps`

Stałe i funkcje pomocnicze stoją w anonimowej przestrzeni nazw pliku `.cpp`, czyli są niewidoczne poza nim.

```cpp
constexpr int AXIS_X = 0;
constexpr int AXIS_Y = 1;
constexpr int AXIS_Z = 2;
constexpr int AXIS_COUNT = 3;

constexpr std::array<int, AXIS_COUNT> AXIS_ORDER = {AXIS_X, AXIS_Z, AXIS_Y};
```

`glm::vec3` pozwala sięgać do składowych numerem: `v[0]` to ta sama liczba co `v.x`, `v[1]` to `v.y`, `v[2]` to `v.z`. Dzięki temu jedna funkcja z parametrem `axis` obsługuje dowolną z trzech osi i nie trzeba pisać trzech prawie identycznych kopii. `AXIS_ORDER` to kolejność obsługi osi w `moveAndSlide`: dwie poziome, na końcu pionowa.

```cpp
float sharedLength(const Aabb& a, const Aabb& b, int axis) {
    return std::min(a.max[axis], b.max[axis]) - std::max(a.min[axis], b.min[axis]);
}
```

Wzór z sekcji 2.2, dla jednej osi: koniec części wspólnej (mniejszy z końców) minus jej początek (większy z początków). Wynik dodatni to długość nakładania, zero to styk, ujemny to odstęp. Na tej jednej funkcji stoją obie funkcje publiczne.

```cpp
bool overlaps(const Aabb& a, const Aabb& b) {
    // Two boxes share volume only when their intervals overlap on all three axes. One
    // axis with a gap (or with mere contact) is enough to keep them apart: a plane
    // perpendicular to that axis fits between them.
    return sharedLength(a, b, AXIS_X) > 0.0F && sharedLength(a, b, AXIS_Y) > 0.0F &&
           sharedLength(a, b, AXIS_Z) > 0.0F;
}
```

| Fragment | Znaczenie |
|---|---|
| `sharedLength(a, b, AXIS_X) > 0.0F` | przedziały nakładają się na osi x. Ostra nierówność: zero (styk) to jeszcze nie nakładanie (sekcja 2.3) |
| `&&` | wszystkie trzy warunki naraz. Operator `&&` przerywa na pierwszym fałszu, więc dla pudełek odległych na osi x pozostałe osie nie są nawet liczone |

Funkcja jest symetryczna: `overlaps(a, b)` i `overlaps(b, a)` dają to samo, bo `std::min` i `std::max` nie zależą od kolejności argumentów.

### 5.4 `allowedDistance`: jedna oś

Serce pliku. Odpowiada na pytanie z sekcji 2.6: jak daleko pudełko może pojechać wzdłuż jednej osi.

```cpp
float allowedDistance(const Aabb& box, float distance, int axis, std::span<const Aabb> obstacles) {
    // Without this early return the code below would treat "no movement" as movement in
    // the negative direction.
    if (distance == 0.0F) {
        return 0.0F;
    }

    // The two axes the box does not move along.
    const int sideAxisA = (axis + 1) % AXIS_COUNT;
    const int sideAxisB = (axis + 2) % AXIS_COUNT;

    // The loop below works with the length of the path, the direction is put back at the
    // end. That keeps one set of comparisons for both directions.
    const bool movesForward = distance > 0.0F;
    float allowed = std::abs(distance);
```

| Linia | Znaczenie |
|---|---|
| `if (distance == 0.0F) { return 0.0F; }` | brak ruchu na tej osi. Bez tej linii `movesForward` byłoby fałszem i reszta kodu potraktowałaby zero jak ruch w stronę ujemną. Wynik i tak wyszedłby 0, ale po przejściu całej listy przeszkód |
| `(axis + 1) % AXIS_COUNT`, `(axis + 2) % AXIS_COUNT` | dwie pozostałe osie. Reszta z dzielenia przez 3 zawija numer: dla osi x (0) to 1 i 2, czyli y i z, dla osi z (2) to 0 i 1, czyli x i y |
| `const bool movesForward = distance > 0.0F;` | kierunek ruchu: w stronę rosnących współrzędnych albo malejących |
| `float allowed = std::abs(distance);` | od tego miejsca funkcja liczy na **długości** drogi (liczba nieujemna). Znak wraca w ostatniej linii. Dzięki temu pętla ma jeden zestaw porównań zamiast dwóch lustrzanych |

```cpp
    for (const Aabb& obstacle : obstacles) {
        // Moving along the axis, the box sweeps a corridor. Only an obstacle inside that
        // corridor can be hit: it has to share a part of both side axes with the box.
        // A shared part within CONTACT_TOLERANCE is a wall the box rests against (plus
        // the rounding error of a float), and such a wall must not stop the slide.
        if (sharedLength(box, obstacle, sideAxisA) <= CONTACT_TOLERANCE ||
            sharedLength(box, obstacle, sideAxisB) <= CONTACT_TOLERANCE) {
            continue;
        }
```

**Czy przeszkoda jest w korytarzu ruchu.** Pudełko jadące wzdłuż osi zakreśla korytarz o przekroju takim jak ono samo. Trafić może tylko w coś, co nakłada się z nim na obu pozostałych osiach. Nakładanie nie większe niż tolerancja to ściana, o którą pudełko jest oparte (sekcja 2.7): taka przeszkoda jest pomijana (`continue` przechodzi do następnej). To jest linia, która daje ślizganie.

```cpp
        // The free space between the face of the box that leads the movement and the
        // face of the obstacle that looks at it.
        const float gap =
            movesForward ? obstacle.min[axis] - box.max[axis] : box.min[axis] - obstacle.max[axis];
```

**Odstęp.** Przy ruchu w stronę dodatnią prowadzi ściana `max` pudełka, a naprzeciw niej stoi ściana `min` przeszkody. Przy ruchu w stronę ujemną odwrotnie: ściana `min` pudełka i ściana `max` przeszkody. W obu przypadkach wynik jest dodatni, gdy przeszkoda jest przed pudełkiem.

```cpp
        // A clearly negative gap: the obstacle is behind the box, or the box is already
        // inside it. Neither stops this movement. In the second case that is what lets
        // a box that got into an obstacle walk out again.
        if (gap < -CONTACT_TOLERANCE) {
            continue;
        }
```

**Przeszkoda za plecami albo pudełko już w środku.** Odstęp wyraźnie ujemny znaczy, że ściana przeszkody nie jest przed pudełkiem: albo cała przeszkoda została z tyłu, albo pudełko jest już w nią wbite. Ani jedno, ani drugie nie blokuje ruchu. Drugi przypadek jest celowy: pudełko, które z jakiegoś powodu znalazło się w ścianie (na przykład zostało tam postawione z panelu), nie jest uwięzione, tylko może wyjść w dowolną stronę.

```cpp
        // The obstacle is ahead: the box may go as far as its face and no further.
        // A gap just below zero (rounding again) is treated as zero, so the result never
        // points backwards.
        allowed = std::min(allowed, std::max(gap, 0.0F));
    }

    return movesForward ? allowed : -allowed;
}
```

| Fragment | Znaczenie |
|---|---|
| `std::max(gap, 0.0F)` | odstęp minimalnie ujemny (do -1 mm, czyli zaokrąglenie) liczy się jak zero. Bez tego wynik mógłby wyjść ujemny, czyli pudełko cofałoby się, zamiast stać |
| `allowed = std::min(allowed, ...)` | najbliższa przeszkoda wygrywa: zostaje najmniejsza z dotychczasowej wartości i odstępu do tej przeszkody. Zaczynam od chcianej długości, więc wynik nigdy jej nie przekroczy |
| `return movesForward ? allowed : -allowed;` | długość z powrotem dostaje znak kierunku |

Funkcja sprawdza wszystkie przeszkody z listy, bez żadnego wstępnego odsiewania. Labirynt 16 na 16 komórek ma 578 pudełek (289 ścian i 289 słupków). Przy ruchu poziomym listę przechodzą dwie osie (trzecia kończy się na pierwszej linii, bo przesunięcie w pionie jest zerem), czyli 1156 przeszkód na krok i około 139 tysięcy na sekundę przy 120 krokach. Dla każdej to kilka porównań liczb. Struktury przyspieszającej (na przykład siatki komórek) nie ma, bo przy takiej skali nie jest potrzebna. Czasu tego kodu nie mierzyłem w działającej grze: liczby wynikają z rozmiaru listy.

### 5.5 `moveAndSlide`

```cpp
glm::vec3 moveAndSlide(const Aabb& mover, const glm::vec3& displacement,
                       std::span<const Aabb> obstacles) {
    // (komentarz o ślizganiu i o ograniczeniu, omówiony w sekcjach 2.6 i 2.8)
    Aabb box = mover;
    glm::vec3 allowed{0.0F};
    for (const int axis : AXIS_ORDER) {
        allowed[axis] = allowedDistance(box, displacement[axis], axis, obstacles);
        // The next axis starts from where this one ended.
        box.min[axis] += allowed[axis];
        box.max[axis] += allowed[axis];
    }
    return allowed;
}
```

W pliku na miejscu linii z nawiasem stoi dłuższy komentarz po angielsku: wyjaśnia, dlaczego obsługa osi po kolei daje ślizganie i jakie ma ograniczenie.

| Linia | Znaczenie |
|---|---|
| `const Aabb& mover` | pudełko **przed** ruchem. Funkcja go nie zmienia |
| `const glm::vec3& displacement` | chciane przesunięcie w tym kroku, w metrach |
| `std::span<const Aabb> obstacles` | lista przeszkód. `std::span` (C++20) to "widok" na ciąg elementów leżących obok siebie w pamięci: wskaźnik i liczba elementów. Przyjmuje bez kopiowania `std::vector<Aabb>`, `std::array<Aabb, N>` i pustą listę `{}`. `const Aabb` znaczy, że funkcja przeszkód nie zmienia |
| `Aabb box = mover;` | robocza kopia, którą funkcja przesuwa oś po osi |
| `glm::vec3 allowed{0.0F};` | wynik, na początku zero na każdej osi |
| `for (const int axis : AXIS_ORDER)` | trzy obroty pętli: x, z, y |
| `allowed[axis] = allowedDistance(box, displacement[axis], axis, obstacles);` | dozwolona droga na tej osi, liczona dla pudełka już przesuniętego na osiach wcześniejszych |
| `box.min[axis] += allowed[axis];` i to samo dla `max` | przesunięcie roboczej kopii. Oba narożniki dostają to samo, więc rozmiar pudełka się nie zmienia |
| `return allowed;` | funkcja zwraca **przesunięcie**, nie nową pozycję. Wołający sam dodaje je do pozycji obiektu |

Dlaczego funkcja zwraca przesunięcie, a nie przesuwa obiektu: `Aabb` to tylko bryła kolizji. Obiekt gry ma swoją pozycję (gracz: punkt, wokół którego buduje się pudełko i z którego liczy się pozycję kamery), a pudełko jest z niej wyliczane. Funkcja, która zwraca "o ile wolno", nie musi wiedzieć, czym jest przesuwany obiekt.

Dlaczego kolejność x, z, y: ruch w labiryncie jest prawie zawsze poziomy, więc obie osie poziome idą pierwsze, a pionowa na końcu. Dopóki nie ma grawitacji ani skoków, składowa y przesunięcia wynosi zero i trzeci obrót pętli kończy się w pierwszej linii `allowedDistance`.

### 5.6 Jak korzysta z tego gracz

Koniec funkcji `game::Player::update`, czyli krok chodzenia ([`src/game/Player.cpp`](../../../src/game/Player.cpp)):

```cpp
    const float speed = input.sprint ? sprintSpeed : walkSpeed;
    const glm::vec3 wanted = direction * (speed * stepSeconds);

    // The walls take away the part of the movement that would go into them and leave the
    // part along them. The box is built anew from the position in every step.
    position += scene::moveAndSlide(box(), wanted, obstacles);
```

A tak woła ją aplikacja w `NightMazeApp::onUpdate`:

```cpp
    m_player.update(wanted, m_camera.yawDegrees, m_camera.pitchDegrees, static_cast<float>(fixedDt),
                    m_mazeWorld.colliders);
```

Trzy elementy spotykają się w jednym kroku symulacji:

| Element | Skąd pochodzi | Jak często jest liczony |
|---|---|---|
| chciane przesunięcie `wanted` | kierunek z klawiszy razy prędkość razy `stepSeconds`, a `stepSeconds` to zawsze `fixedDt` | co krok |
| pudełko `box()` | z pozycji stóp gracza: 0,6 x 1,8 x 0,6 m | co krok, od nowa |
| lista przeszkód `obstacles` | `m_mazeWorld.colliders`, wynik `game::mazeColliders` | raz na labirynt |

Kod pilnuje więc trzech rzeczy, o których mówi teoria: przesunięcie powstaje ze stałego kroku (sekcja 2.8), pudełko jest budowane od nowa z pozycji, a lista przeszkód nie jest liczona w każdym kroku. W trybie noclip gracz w ogóle nie woła `moveAndSlide`. Całą funkcję linia po linii omawia [`../game/player.md`](../game/player.md), sekcja 5.

### 5.7 Jak to zostało sprawdzone

Testy jednostkowe w bibliotece doctest, uruchamiane przez `ctest` ([`../../libraries/doctest.md`](../../libraries/doctest.md), sekcja 4). Przypadki z `tests/ColliderTests.cpp`:

| Przypadek testowy | Co sprawdza | Wynik |
|---|---|---|
| `Aabb::fromCenter puts the corners...` | środek `(1, 2, 3)` i połowy `(0,5, 1, 2)` | `min = (0,5, 1, 1)`, `max = (1,5, 3, 5)` |
| `overlaps is true only when the boxes share volume` | pudełka przecinające się, jedno w drugim, z odstępem na jednej osi, stykające się ścianą, krawędzią i narożnikiem | nakładanie tylko w dwóch pierwszych przypadkach, w obie strony wywołania |
| `moveAndSlide allows the whole displacement...` | brak przeszkód (pusta lista) i przeszkoda poza zasięgiem | całe przesunięcie |
| `...stops a box that runs straight into a wall` | pudełko 0,7 m przed ścianą: krok 2 m, krok w stronę ujemną z drugiej strony, krok 50 m, krok z pozycji styku | 0,7, potem -1,5, potem 0,7 (bez tunelowania), potem 0 |
| `...lets a box slide along a wall` | krok ukośny `(1, 0, 0,5)`, ruch wzdłuż ściany z pozycji styku, odejście od ściany | `(0,7, 0, 0,5)`, pełny ruch wzdłuż, pełne odejście |
| `...stops a box in a corner on both axes` | dwie ściany, krok `(3, 0, 3)` | `(0,7, 0, 0,7)`, a kolejne pchanie w narożnik daje zero |
| `...returns zero for a zero displacement` | zerowe przesunięcie, także w styku z dwiema ścianami | zero |
| `...handles the vertical axis too` | pudełko 1 m nad podłogą spada o 5 m, potem idzie po podłodze | spada dokładnie o 1 m, po podłodze idzie bez przeszkód |
| `...does not hold a box that starts inside an obstacle` | środek pudełka w środku ściany, krok 2 m na zewnątrz | całe przesunięcie |
| `many small steps along a wall never stick...` | 2000 kroków `(0,0004, 0, 0,0251)` przy ścianie w x = 37,3 (kąt około 1 stopnia, współrzędne dalekie od zera) | ruch wzdłuż ściany ani razu nie został obcięty, zagłębienie nigdy nie przekroczyło `CONTACT_TOLERANCE` |
| `a box slides across the joint of two wall segments` | dwa segmenty ściany w jednej linii, stykające się końcami, 100 kroków `(0,02, 0, 0,03)` | pudełko nie zatrzymuje się na łączeniu |
| `documented limit: a step much longer than the boxes...` | słupek na przekątnej: jeden krok `(2, 0, 2)` i ta sama droga w 80 krokach | długi krok mija słupek (sekcja 2.8), krótkie kroki na niego trafiają i muszą go obejść |

I trzy przypadki z `tests/MazeLayoutTests.cpp`, w których przeszkodami są prawdziwe pudełka labiryntu:

| Przypadek testowy | Co sprawdza | Wynik |
|---|---|---|
| `the colliders of a closed cell keep a box inside it` | pudełko w zamkniętej komórce pchane na wschód przez 400 kroków | staje przy wewnętrznym licu pudełka ściany: środek w x = 1,55 (`2 - 0,15 - 0,3`) |
| `a box wandering through a generated maze...` | labirynt 8 na 8 (ziarno 3), 400 zmian kierunku po 60 kroków, 16 kierunków, część prawie równoległa do ścian | pudełko pomniejszone z każdej strony o dwie tolerancje (2 mm) ani razu nie nachodzi na żadną przeszkodę, a wędrówka oddala się od startu o ponad dwie komórki |
| `a box that hugs a wall slides past the pillars in the middle of it` | korytarz 1 na 3 komórki. Pudełko przytulone do ściany wschodniej idzie prosto na południe (200 kroków), potem to samo ze środka komórki ruchem ukośnym w ścianę (400 kroków) | w obu przypadkach pudełko mija słupki w z = 2 i z = 4 i staje dopiero na ścianie południowej ostatniej komórki: środek w x = 1,55 i z = 5,55 (pułapka 1) |

Ostatni wiersz zastąpił wcześniejszy przypadek, który przypinał zachowanie odwrotne (pudełko stające na słupku). Zmieniło się zachowanie, a nie tylko test: pudełka ścian dostały grubość słupków (pułapka 1).

Cztery dalsze przypadki z prawdziwym graczem są w `tests/PlayerTests.cpp` (`a wall stops the player`, `a player pressing into a wall slides along it and past the pillars`, `a player wandering through a closed maze never leaves it or enters a wall`, `noclip flies through walls`): omawia je [`../game/player.md`](../game/player.md), sekcja 5.

Wyniki na Windowsie (MSVC 19.44, `/W4 /permissive-`, 2026-10-05): build Debug i Release bez ostrzeżeń, wszystkie testy przechodzą w obu konfiguracjach. Stan całego programu testowego po M4 (2026-10-05): 149 przypadków i 61240 asercji, w tym te same przypadki kolizji. Na macOS kod nie był jeszcze kompilowany ani uruchamiany: to pozycja na liście w [`../../guides/build-macos.md`](../../guides/build-macos.md).

### 5.8 Rysowanie pudełek: `ColliderLines`

Klasa z programu `night_maze` ([`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`.cpp`](../../../src/game/ColliderLines.cpp)). Nie zmienia kolizji w żaden sposób: tylko pokazuje te same pudełka, na których liczy `moveAndSlide`.

```cpp
class ColliderLines {
public:
    /// Uploads the unit cube.
    ColliderLines();

    /// Draws every box in one colour. shader is the flat colour program (color.vert and
    /// color.frag): it must be in use, with uView and uProjection already set. The
    /// function sets uColor, and uModel for every box.
    void draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
              const glm::vec3& color) const;

private:
    gfx::Mesh m_unitCube;
};
```

Jedno pole: siatka sześcianu o boku 1. Klasa posiada więc obiekty OpenGL (VAO i dwa bufory wewnątrz `gfx::Mesh`) i musi zostać zniszczona przed oknem. Kopiować się jej nie da, bo `gfx::Mesh` nie da się kopiować.

**Dane sześcianu.**

```cpp
constexpr std::size_t CORNER_COUNT = 8;
constexpr std::size_t EDGE_COUNT = 12;
constexpr std::size_t INDICES_PER_LINE = 2;
```

```cpp
constexpr std::array<gfx::Vertex, CORNER_COUNT> UNIT_CUBE_CORNERS = {
    gfx::Vertex{.position = {0.0F, 0.0F, 0.0F}}, // 0
    gfx::Vertex{.position = {1.0F, 0.0F, 0.0F}}, // 1
    gfx::Vertex{.position = {1.0F, 0.0F, 1.0F}}, // 2
    gfx::Vertex{.position = {0.0F, 0.0F, 1.0F}}, // 3
    gfx::Vertex{.position = {0.0F, 1.0F, 0.0F}}, // 4
    gfx::Vertex{.position = {1.0F, 1.0F, 0.0F}}, // 5
    gfx::Vertex{.position = {1.0F, 1.0F, 1.0F}}, // 6
    gfx::Vertex{.position = {0.0F, 1.0F, 1.0F}}, // 7
};

// Every two indices are one line (GL_LINES): the two corners an edge joins.
constexpr std::array<std::uint32_t, EDGE_COUNT * INDICES_PER_LINE> UNIT_CUBE_EDGES = {
    0, 1, 1, 2, 2, 3, 3, 0, // bottom face
    4, 5, 5, 6, 6, 7, 7, 4, // top face
    0, 4, 1, 5, 2, 6, 3, 7, // the four vertical edges
};
```

| Fragment | Znaczenie |
|---|---|
| `gfx::Vertex{.position = {...}}` | wypełniona jest tylko pozycja. Normalna i współrzędne tekstury zostają zerami (wartości domyślne struktury): linie ich nie potrzebują, a shader `color.vert` ich nie czyta |
| narożniki od 0 do 3 | dolna ściana (y = 0), po kolei dookoła |
| narożniki od 4 do 7 | górna ściana (y = 1), w tej samej kolejności, więc narożnik `n + 4` stoi nad narożnikiem `n` |
| `0, 1, 1, 2, 2, 3, 3, 0` | cztery krawędzie dolnej ściany: każda para to jeden odcinek |
| `0, 4, 1, 5, 2, 6, 3, 7` | cztery krawędzie pionowe |
| `EDGE_COUNT * INDICES_PER_LINE` | 24 indeksy: 12 krawędzi po 2 |

Sześcian sięga od `(0, 0, 0)` do `(1, 1, 1)`, a nie od -0,5 do 0,5 jak kostka z M1. Powód jest w funkcji `draw`: narożnik w początku układu sprawia, że przesunięcie do narożnika `min` pudełka wystarcza, bez liczenia środka.

Tutaj 8 wierzchołków wystarcza, choć kostka z M1 potrzebowała 24. Tam każdy narożnik miał trzy różne kolory (po jednym na ścianę), więc był trzema wierzchołkami. Tu wierzchołek to sama pozycja, więc narożnik wspólny dla trzech krawędzi jest jednym wierzchołkiem.

```cpp
ColliderLines::ColliderLines() : m_unitCube(UNIT_CUBE_CORNERS, UNIT_CUBE_EDGES, GL_LINES) {}
```

Trzeci argument konstruktora `gfx::Mesh` to rodzaj prymitywu. Tablice `std::array` same zamieniają się na `std::span`.

**`draw`.**

```cpp
// The drawn box is this much larger than the real one on every side, in metres (1 cm).
// The box of a pillar is exactly as wide as the shaft of the pillar model, so lines at
// the true size would lie in the surface of the model and flicker in and out of it
// (z-fighting). The margin puts them just in front. Only the drawing is changed: the
// collisions use the true boxes.
constexpr float LINE_MARGIN = 0.01F;
```

```cpp
void ColliderLines::draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
                         const glm::vec3& color) const {
    shader.setVec3(COLOR_UNIFORM, color);

    // The line width is left at its default of 1 pixel on purpose: an OpenGL Core
    // profile is not required to support wider lines, and macOS does not.
    for (const scene::Aabb& box : boxes) {
        // The unit cube has its corner (0, 0, 0) in the origin, so scaling it by the
        // size of the box and then moving it to the min corner lands it on the box.
        scene::Transform transform;
        transform.position = box.min - glm::vec3{LINE_MARGIN};
        transform.scale = box.max - box.min + glm::vec3{2.0F * LINE_MARGIN};

        shader.setMat4(MODEL_UNIFORM, transform.matrix());
        m_unitCube.draw();
    }
}
```

| Linia | Znaczenie |
|---|---|
| `shader.setVec3(COLOR_UNIFORM, color);` | jeden kolor dla całej listy pudełek, ustawiany raz przed pętlą |
| `transform.scale = box.max - box.min + glm::vec3{2.0F * LINE_MARGIN};` | rozmiar pudełka na każdej osi, powiększony o margines z obu stron. Sześcian o boku 1 pomnożony przez rozmiar staje się prostopadłościanem tego rozmiaru |
| `transform.position = box.min - glm::vec3{LINE_MARGIN};` | narożnik `(0, 0, 0)` sześcianu trafia w narożnik `min` pudełka, cofnięty o margines. `glm::vec3{LINE_MARGIN}` to wektor z tą samą wartością w trzech składowych |
| `transform.matrix()` | `translate * rotate * scale`: wierzchołek jest najpierw skalowany, potem przesuwany ([`transforms.md`](transforms.md), sekcja 5). Obrotu nie ma, bo AABB się nie obraca |
| `m_unitCube.draw();` | `glDrawElements(GL_LINES, 24, ...)` |

Przykład na liczbach: pudełko ściany wzdłuż X o środku na linii z = 4 ma `min = (2, 0, 3,85)` i `max = (4, 3, 4,15)`. Skala wychodzi `(2,02, 3,02, 0,32)`, a pozycja `(1,99, -0,01, 3,84)`. Narożnik `(1, 1, 1)` sześcianu ląduje w `(4,01, 3,01, 4,16)`: centymetr poza prawdziwym `max`.

Inaczej niż macierze ścian labiryntu, te macierze **są** liczone w każdej klatce, w pętli rysowania. To świadome uproszczenie: rysowanie pudełek jest narzędziem diagnostycznym, domyślnie wyłączonym, a pudełko gracza i tak zmienia się co klatkę.

**Kto woła `draw`.** `NightMazeApp::onRender`, na końcu klatki i tylko wtedy, gdy przełącznik jest włączony:

```cpp
    drawMaze(view, projection);
    drawCube(view, projection);
    if (m_drawColliders) {
        drawColliderLines(view, projection);
    }
```

```cpp
// Colours of the collision box lines (red, green, blue): the boxes of the maze in
// yellow, the box of the player in green.
constexpr glm::vec3 MAZE_COLLIDER_COLOR{1.0F, 0.85F, 0.1F};
constexpr glm::vec3 PLAYER_COLLIDER_COLOR{0.2F, 1.0F, 0.4F};
```

```cpp
void NightMazeApp::drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The depth test stays on: a line behind a wall is hidden by it, which shows where
    // each box really is. The box of the player is drawn at the simulation position (the
    // last fixed step), the camera at a blend of two steps, so while moving the box runs
    // ahead of the camera by a fraction of one step.
    m_colliderLines.draw(m_colorShader, m_mazeWorld.colliders, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);
}
```

| Linia | Znaczenie |
|---|---|
| `if (!m_colorShader.isValid()) { return; }` | bez programu nie ma czym rysować. Błąd wczytania shadera był w logu przy starcie, a reszta klatki rysuje się normalnie |
| `use()`, potem `uView` i `uProjection` | te same macierze co dla labiryntu i kostki: linie są widziane z tego samego oka |
| `draw(m_colorShader, m_mazeWorld.colliders, MAZE_COLLIDER_COLOR)` | dokładnie ta lista, którą dostaje `Player::update`. Wektor sam zamienia się na `std::span` |
| `const scene::Aabb playerBox = m_player.box();` | pudełko gracza w pozycji symulacji |
| `std::span<const scene::Aabb>(&playerBox, 1)` | widok na jeden element: wskaźnik i liczba 1. Dzięki temu `draw` ma jedną wersję, dla listy |

Linie są rysowane **po** labiryncie i kostce, z włączonym testem głębi. Kolejność nie wpływa na to, co jest zasłonięte (o tym decyduje głębia), ale gwarantuje, że głębie ścian są już w buforze, gdy linie są z nimi porównywane.

Na zrzucie ekranu z Windowsa (widok z góry, tryb noclip) żółte pudełka leżą na ścianach i słupkach. Pudełka gracza na zrzutach nie oceniałem, a przełącznika nikt jeszcze nie kliknął ręcznie.

## 6. Panel ImGui

Panel **Collision** jest pokazem tematu 14. Kod: [`src/debug/panels/CollisionPanel.cpp`](../../../src/debug/panels/CollisionPanel.cpp). Jak panel jest podpięty do `DebugUI`, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.

PRD (sekcja 10) opisuje panel Collision jako "Debug draw AABB i sfer, wynik ostatniego raycasta", a tryb noclip wymienia przy panelu Camera. W programie jest rysowanie pudełek AABB. Kul i promieni (raycast) jeszcze nie ma, bo nie ma jeszcze kodu, który by ich używał (sekcja 2.9). Przełącznik noclip trafił do tego panelu, bo znaczy "wyłącz kolizje", a panel Camera pokazuje tylko bieżący tryb ([`camera-controls.md`](camera-controls.md), sekcja 6).

### 6.1 Kod panelu

```cpp
void drawCollisionPanel(const game::MazeWorld& world, game::Player& player, bool& drawColliders) {
    // First run only: the bottom edge of the window, right of the left column (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(COLLISION_PLACEMENT);
    if (ImGui::Begin("Collision")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Draw collision boxes", &drawColliders);
        ImGui::TextWrapped("Yellow: walls and pillars. Green: the player.");

        // The same switch as the N key. In noclip mode the boxes below are ignored.
        ImGui::Checkbox("Noclip (key N)", &player.noclip);

        ImGui::Separator();
        // world.colliders holds the box of every wall first and the box of every pillar
        // after them, so the two counts are the sizes of the lists they were made from.
        ImGui::Text("Wall boxes: %d", static_cast<int>(world.walls.size()));
        ImGui::Text("Pillar boxes: %d", static_cast<int>(world.pillars.size()));
        ImGui::Text("All boxes: %d", static_cast<int>(world.colliders.size()));
        ImGui::TextWrapped("Wall box: %.2f m thick (the visible wall: %.2f m)",
                           game::WALL_COLLISION_THICKNESS, game::WALL_VISUAL_THICKNESS);

        ImGui::Separator();
        // The box is computed from the position of the player in every frame, exactly as
        // the movement code does it.
        const scene::Aabb box = player.box();
        ImGui::TextUnformatted("Player box");
        ImGui::Text("min: %.2f, %.2f, %.2f", box.min.x, box.min.y, box.min.z);
        ImGui::Text("max: %.2f, %.2f, %.2f", box.max.x, box.max.y, box.max.z);
    }
    ImGui::End();
}
```

| Element | Znaczenie |
|---|---|
| `const game::MazeWorld& world` | labirynt tylko do odczytu: panel liczy pudełka, niczego w nich nie zmienia |
| `game::Player& player` | gracz bez `const`: pole wyboru pisze do `player.noclip` |
| `bool& drawColliders` | referencja do pola `m_drawColliders` aplikacji. Panel tylko ustawia flagę, a rysuje `NightMazeApp::onRender` w następnej klatce |
| `placePanelOnFirstUse(COLLISION_PLACEMENT)` | miejsce i rozmiar przy pierwszym uruchomieniu: dolna krawędź okna, na prawo od lewej kolumny paneli. Stała leży w [`PanelLayout.hpp`](../../../src/debug/PanelLayout.hpp), a funkcja ustawia pozycję i rozmiar z warunkiem `ImGuiCond_FirstUseEver`. Wpis w `imgui.ini` ma pierwszeństwo ([`../debug-ui.md`](../debug-ui.md), sekcja 5.7). Plik panelu nie ma już własnych stałych, więc nie ma w nim anonimowej przestrzeni nazw |
| `ImGui::Checkbox("Draw collision boxes", &drawColliders)` | pole wyboru czyta i zapisuje `bool` przez wskaźnik |
| `ImGui::Checkbox("Noclip (key N)", &player.noclip)` | to samo pole, które przełącza klawisz N. Dwa sposoby zmiany jednej zmiennej, więc nie mogą się rozjechać |
| `world.walls.size()`, `world.pillars.size()`, `world.colliders.size()` | liczniki. Trzeci jest sumą dwóch pierwszych, bo `mazeColliders` dodaje jedno pudełko na ścianę i jedno na słupek |
| `static_cast<int>(...)` | `%d` oczekuje `int`, a `size()` zwraca `std::size_t` |
| `game::WALL_COLLISION_THICKNESS`, `game::WALL_VISUAL_THICKNESS` | 0,30 i 0,20: panel sam mówi, że pudełko ściany jest grubsze niż widoczna ściana (pułapka 1). To jedyne miejsce w kodzie programu, które czyta `WALL_VISUAL_THICKNESS` |
| `player.box()` | pudełko liczone w każdej klatce z pozycji gracza, tą samą funkcją, której używa ruch |

### 6.2 Kontrolki i czego uczą

| Kontrolka | Co zmienia albo pokazuje | Czego uczy |
|---|---|---|
| `Draw collision boxes` | `m_drawColliders` | bryła otaczająca jest prostsza niż model: żółte pudełko ściany to sześć płaszczyzn zamiast 30 trójkątów |
| `Noclip (key N)` | `player.noclip` | kolizje są osobnym krokiem, który można pominąć: w trybie noclip lista pudełek nie jest czytana |
| `Wall boxes`, `Pillar boxes`, `All boxes` | odczyt | ile przeszkód sprawdza każdy krok. Dla labiryntu 10 na 10: 121, 121 i 242 |
| `Wall box: 0.30 m thick (the visible wall: 0.20 m)` | odczyt | bryła kolizji nie musi mieć wymiarów modelu |
| `Player box`, `min`, `max` | odczyt | pudełko jest liczone z pozycji: na starcie `min: 0.70, 0.00, 0.70` i `max: 1.30, 1.80, 1.30` |

### 6.3 Scenariusz pokazu na obronie

Kroki nie były jeszcze wykonane ręcznie. Opisują to, co wynika z kodu i z testów, a krok 2 także ze zrzutu ekranu.

1. **Liczby.** Otwieram panel Collision: 121 pudełek ścian, 121 słupków, razem 242. Mówię, że każdy krok chodzenia sprawdza całą listę na dwóch osiach i że przy tej skali nie potrzeba struktury przyspieszającej.
2. **Pudełka.** Włączam `Draw collision boxes`, naciskam N i wzlatuję nad labirynt. Żółte pudełka leżą na ścianach i słupkach. Pokazuję, że pudełko ściany jest grubsze niż ściana i że lica pudełek ścian i słupków tworzą jedną płaszczyznę.
3. **Pudełko gracza.** Wracam na podłogę (N) i patrzę w dół: zielone linie wokół mnie. Odczytuję `min` i `max` i pokazuję, że różnią się o 0,6, 1,8 i 0,6.
4. **Zatrzymanie.** Idę prosto na ścianę. Staję, a w `Player box` współrzędna od strony ściany przestaje się zmieniać.
5. **Ślizganie.** Idę ukosem w ścianę (W i A albo W i D). Sunę wzdłuż niej i mijam słupki bez zatrzymania. Mówię: osie są obsługiwane po kolei, ściana zabiera tylko składową skierowaną w nią.
6. **Narożnik.** Wchodzę w róg korytarza. Staję na obu osiach.
7. **Noclip.** Zaznaczam `Noclip (key N)` i przechodzę przez ścianę. Odznaczam w środku ściany: mogę wyjść w dowolną stronę, bo `moveAndSlide` nie trzyma pudełka, które zaczyna w przeszkodzie.
8. **Testy.** W terminalu uruchamiam `ctest --test-dir build/debug -C Debug --output-on-failure` i mówię, że to testy, a nie obraz, są dowodem poprawności.

## 7. Pułapki

1. **Pudełko ściany jest grubsze niż ściana, i to celowo.** Widoczny korpus ściany ma 0,2 m (`WALL_VISUAL_THICKNESS`), słupek 0,3 m (`PILLAR_SIZE`). Gdyby pudełko ściany miało 0,2 m, pudełko słupka wystawałoby 5 cm przed jej lico z każdej strony, a słupek stoi na każdym łączeniu segmentów, czyli co 2 m. Pudełko ślizgające się po ścianie trafiałoby wtedy na słupek i stawało. Tak było w pierwszej wersji kodu i przypinał to test. To nie był błąd `moveAndSlide`, tylko skutek wymiarów. Poprawka jest w danych, nie w algorytmie: pudełko ściany ma grubość słupka (`WALL_COLLISION_THICKNESS = PILLAR_SIZE`), więc lica pudełek ścian i słupków leżą w jednej płaszczyźnie, a pudełko sunące po ścianie tylko **styka się** ze słupkiem, a styk nie zatrzymuje (sekcja 2.3). Cena: gracz staje 5 cm przed widocznym korpusem ściany ([`../game/maze-generator.md`](../game/maze-generator.md), sekcja 5.7).
2. **Przesunięcie liczone z czasu klatki.** `moveAndSlide` zakłada krótkie kroki (sekcja 2.8). Przesunięcie policzone z `deltaSeconds()` zamiast z `fixedDt` po jednej wolnej klatce może mieć 75 cm i wtedy droga po schodkach wyraźnie różni się od prostej.
3. **Pudełko z `min` większym od `max`.** Nic tego nie sprawdza. Takie pudełko ma ujemną wspólną długość z każdym innym, więc `overlaps` zawsze zwraca fałsz, a `moveAndSlide` go nie widzi. Typowe źródło: ujemna połowa rozmiaru podana do `fromCenter` albo pomylone narożniki przy ręcznym tworzeniu.
4. **Start wewnątrz przeszkody.** Funkcja nie wypycha pudełka, które już jest w przeszkodzie: pozwala mu wyjść w dowolną stronę, ale też pozwala iść dalej w głąb. Pozycja startowa gracza musi leżeć w wolnym miejscu (środek komórki, `game::cellCenter`).
5. **Dwie definicje "dotyku".** `overlaps` jest ścisłe: zero to styk, cokolwiek powyżej zera to nakładanie. `moveAndSlide` traktuje jak styk wszystko do 1 mm. Po serii kroków pudełko może być w ścianie o ułamek milimetra i `overlaps` powie wtedy "tak". Do pytania "czy gracz jest w ścianie" po ruchu trzeba więc użyć pudełka pomniejszonego o tolerancję z zapasem: test wędrówki pomniejsza je o `2 * CONTACT_TOLERANCE` z każdej strony.
6. **Porównywanie pozycji przez `==`.** Po 2000 dodawań `float` suma różni się od iloczynu na czwartym miejscu po przecinku (zmierzone przy pisaniu testu: 30,2004 zamiast 30,2). Testy porównują przez `doctest::Approx` albo sprawdzają pojedynczy krok.
7. **Kolejność osi ma znaczenie przy narożniku wypukłym.** Pudełko idące ukosem dokładnie na róg przeszkody przejdzie po tej stronie, którą wyznacza oś obsługiwana pierwsza (x). Wynik jest poprawny (bez wchodzenia w przeszkodę), ale nie jest symetryczny.
8. **`std::span` niczego nie posiada.** To tylko widok. Wywołanie `moveAndSlide(box, step, game::mazeColliders(maze))` jest poprawne, bo tymczasowy wektor żyje do końca instrukcji, ale jest też powolne: buduje całą listę przy każdym kroku. Listę trzeba policzyć raz i trzymać w polu. Zapamiętanie samego `std::span` do wektora, który potem znika, to wiszący wskaźnik.
9. **AABB nie obraca się z obiektem.** Pudełko obiektu obróconego o kąt inny niż wielokrotność 90 stopni trzeba policzyć od nowa, większe, tak żeby objęło obrócony kształt. W labiryncie problem nie występuje: ściany stoją tylko w dwóch ustawieniach i `game::wallBox` ma dla każdego osobne połowy rozmiarów.
10. **Nie ma grawitacji.** Oś y jest obsługiwana tak samo jak pozostałe (jest na to test), ale nic nie ciągnie pudełka w dół. Gracz jest trzymany na wysokości podłogi przez kod gry (`position.y = FLOOR_Y` w `Player::update`), nie przez kolizje: podłogi nie ma na liście przeszkód.
11. **Rysunek nie jest dowodem.** OpenGL narysuje obiekt w ścianie bez żadnego błędu. Dowodem poprawności kolizji są testy. Rysowanie pudełek w panelu Collision pomaga zobaczyć, **gdzie** pudełka są, ale nie sprawdza, czy ruch ich przestrzega.
12. **Rysowane pudełko jest o centymetr większe od prawdziwego.** Margines `LINE_MARGIN` chroni linie przed migotaniem na powierzchni modelu (sekcja 3). Kto mierzy coś na ekranie po żółtych liniach, mierzy z błędem 1 cm z każdej strony.
13. **Linie w prawdziwym rozmiarze migoczą.** Bez marginesu linie pudełka słupka leżą w powierzchni trzonu modelu i walczą z nim o głębię. Podobnie zachowałaby się każda inna geometria narysowana dokładnie w płaszczyźnie innej.
14. **`glLineWidth` nie pogrubi linii przenośnie.** Profil Core gwarantuje tylko szerokość 1. Grubsze linie trzeba by rysować jako wąskie prostokąty z trójkątów.
15. **Zielone pudełko gracza wyprzedza kamerę.** Jest rysowane w pozycji z ostatniego kroku symulacji, a kamera w punkcie między dwoma krokami. W ruchu różnica to ułamek kroku, najwyżej 2,5 cm przy 3 m/s.
16. **Brak ściany na ekranie nie znaczy braku kolizji.** Pudełka powstają z siatki labiryntu, a nie z modeli. Gdy plik modelu ściany się nie wczyta, ściany znikają z obrazu, ale nadal zatrzymują gracza.

## 8. Ćwiczenia

Ćwiczenia od 1 do 5 robi się na kartce. Ćwiczenia od 6 do 13 to zmiany w kodzie albo w testach: po każdej zbuduj projekt i uruchom testy (`cmake --build --preset debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure`), a na końcu wycofaj zmianę (`git checkout src tests`).

1. **Wspólna długość.** Policz wspólną długość przedziałów `[2, 5]` i `[4, 9]`, potem `[2, 5]` i `[5, 9]`, potem `[2, 5]` i `[7, 9]`. Odpowiedzi: 1, 0, -2.
2. **Nakładanie.** Pudełko A ma `min = (0, 0, 0)` i `max = (2, 3, 2)`, pudełko B ma `min = (1, 1, 2)` i `max = (4, 2, 5)`. Czy nachodzą na siebie? Odpowiedź: nie. Na x wspólna długość to 1, na y to 1, na z to 0: stykają się ścianami.
3. **Ze środka.** Gracz stoi w `(5, 0,9, 3)`, a jego pudełko ma połowy rozmiarów `(0,3, 0,9, 0,3)`. Podaj `min` i `max`. Odpowiedź: `(4,7, 0, 2,7)` i `(5,3, 1,8, 3,3)`.
4. **Ślizganie.** Pudełko z ćwiczenia 3 chce się przesunąć o `(0,5, 0, -0,2)`. Jedyną przeszkodą jest ściana od `(5,4, 0, 0)` do `(5,6, 3, 10)`. Podaj wynik `moveAndSlide`. Odpowiedź: `(0,1, 0, -0,2)`: na x odstęp to `5,4 - 5,3 = 0,1`, a na z ściana nie leży w korytarzu ruchu (po kroku x styka się z pudełkiem, wspólna długość na x to zero).
5. **Tunelowanie.** Gracz ma pudełko szerokości 0,6 m i prędkość 5,5 m/s (sprint), pudełko ściany ma 0,3 m. Przy jakim czasie kroku test dyskretny zacząłby przeskakiwać ścianę? Odpowiedź: gdy krok przekroczy 0,9 m, czyli przy czasie ponad 0,164 s (około 6 FPS). Ile wynosi krok przy `FIXED_DT`? Odpowiedź: około 4,6 cm.
6. **Dotyk jako kolizja.** W `overlaps` zamień trzy razy `> 0.0F` na `>= 0.0F` i uruchom testy. Wynik (zmierzony): nie przechodzi jeden przypadek, `overlaps is true only when the boxes share volume`, w podprzypadku `touching is not overlapping` (trzy sprawdzenia: ściana, krawędź, narożnik). Dlaczego wszystkie testy `moveAndSlide` nadal przechodzą (podpowiedź: czy `moveAndSlide` woła `overlaps`)?
7. **Bez tolerancji.** W `allowedDistance` zamień oba `<= CONTACT_TOLERANCE` na `<= 0.0F`, a `gap < -CONTACT_TOLERANCE` na `gap < 0.0F`. Wynik zmierzony na wcześniejszej wersji kodu (pudełka ścian 0,2 m): przestały przechodzić trzy przypadki, między innymi `the colliders of a closed cell keep a box inside it`, w którym pudełko przeszło przez ścianę i szło dalej, oraz test wędrówki, w którym pudełko trafiło do wnętrza przeszkody. Po zmianie grubości pudełek ścian pomiaru nie powtórzyłem, więc dokładne liczby i lista przypadków mogą być dziś inne: sprawdź sam. Wyjaśnij każdy wynik sekcją 2.7: w którym kroku ściana przestała być "przed" pudełkiem?
8. **Inna kolejność osi.** Zmień `AXIS_ORDER` na `{AXIS_Z, AXIS_X, AXIS_Y}`. Wynik (zmierzony): nie przechodzi jeden przypadek, `documented limit: a step much longer than the boxes can go around an obstacle`, w części z krótkimi krokami: pudełko kończy w z = 2 zamiast poniżej 1,5. Słupek stoi dokładnie na przekątnej, więc o tym, którą stroną pudełko go obejdzie, decyduje oś obsługiwana pierwsza (pułapka 7). Narysuj obie drogi.
9. **Własny test.** Dopisz w `tests/ColliderTests.cpp` przypadek dla sufitu: pudełko pod płytą, ruch w górę o 2 m przy odstępie 0,4 m. Wzoruj się na teście `moveAndSlide handles the vertical axis too`.
10. **Cienkie pudełka ścian.** W `src/game/MazeLayout.hpp` zmień `WALL_COLLISION_THICKNESS` na `WALL_VISUAL_THICKNESS` i uruchom testy. Które przypadki przestają przechodzić i dlaczego (pułapka 1)? Wyniku nie mierzyłem po ostatniej zmianie: spodziewam się porażki testów ślizgania obok słupków w `MazeLayoutTests.cpp` i `PlayerTests.cpp` oraz testu stałych.
11. **Linie bez marginesu.** W `ColliderLines.cpp` ustaw `LINE_MARGIN` na `0.0F`, uruchom program, włącz `Draw collision boxes` i obejrzyj słupek z bliska, poruszając kamerą. Co się dzieje z liniami na trzonie i dlaczego nie na korpusie ściany?
12. **Sześcian od -0,5 do 0,5.** Zmień narożniki `UNIT_CUBE_CORNERS` tak, żeby sześcian był wyśrodkowany. Co trzeba zmienić w `ColliderLines::draw`, żeby pudełka nadal trafiały na swoje miejsca?
13. **Linie przez ściany.** W `drawColliderLines` wyłącz test głębi przed rysowaniem linii (`glDisable(GL_DEPTH_TEST)`) i włącz go z powrotem po nim. Co widać i kiedy taki widok jest przydatny?

## 9. Pytania kontrolne

1. **Co to jest AABB i dlaczego wystarczą mu dwa narożniki?**
   Prostopadłościan o krawędziach równoległych do osi świata. Skoro nie może być obrócony, każda ściana leży na stałej wartości jednej współrzędnej, więc sześć liczb (najmniejsze i największe x, y, z) opisuje go w całości.

2. **Jak sprawdzić, czy dwa AABB na siebie nachodzą?**
   Na każdej osi pudełko jest przedziałem. Liczę wspólną długość `min(a.max, b.max) - max(a.min, b.min)`. Pudełka nachodzą na siebie, gdy jest dodatnia na wszystkich trzech osiach. Jedna oś z odstępem wystarcza, żeby były rozłączne.

3. **Dlaczego styk nie liczy się jako nakładanie?**
   Gracz oparty o ścianę i segmenty ścian stykające się końcami byłyby cały czas "w kolizji". `overlaps` używa ostrej nierówności, więc wspólna długość zero to jeszcze nie kolizja.

4. **Czym różni się wykrywanie dyskretne od przemiatania?**
   Dyskretne sprawdza tylko pozycję końcową ruchu. Przemiatanie bada całą drogę i odpowiada, jak daleko wolno się przesunąć. Dyskretne jest prostsze, ale może przeoczyć przeszkodę, którą obiekt w całości przeskoczył.

5. **Co to jest tunelowanie i kiedy występuje?**
   Przejście obiektu przez przeszkodę bez wykrycia kolizji. W teście dyskretnym występuje, gdy krok jest dłuższy niż łączna grubość obiektu i przeszkody. Zapobiegają mu krótkie kroki o stałej długości i przemiatanie.

6. **Czy `moveAndSlide` może przeskoczyć ścianę?**
   Na pojedynczej osi nie: funkcja mierzy odstęp do przeszkody leżącej w korytarzu ruchu, więc długość kroku nie ma znaczenia (test z krokiem 50 m). Ograniczeniem jest co innego: droga po schodkach.

7. **Dlaczego obsługa osi po kolei daje ślizganie?**
   Ruch ukośny w ścianę ma część skierowaną w ścianę i część równoległą. Na osi prostopadłej odstęp wynosi zero i ta część jest obcinana. Na osi równoległej ściana nie leży w korytarzu ruchu (styka się z pudełkiem, ale się nie nakłada), więc ta część przechodzi cała.

8. **Jakie ograniczenie ma obsługa osi po kolei i jak się ma do stałego kroku?**
   Pudełko jedzie po krawędziach schodka (całe x, potem całe z, potem całe y), a nie po przekątnej. Przy długim kroku może obejść małą przeszkodę leżącą na przekątnej albo zahaczyć o narożnik, który przekątna omija. Błąd jest rzędu długości kroku, więc przesunięcie musi pochodzić ze stałego kroku: 2,5 cm przy 3 m/s i `FIXED_DT` = 1/120 s.

9. **Po co `CONTACT_TOLERANCE` i dlaczego `overlaps` jej nie używa?**
   Po dodaniu przesunięcia do pozycji ściana pudełka ląduje przez zaokrąglenia `float` kilka milionowych metra przed ścianą albo w niej. Bez tolerancji takie zagłębienie blokowałoby ruch wzdłuż ściany. `moveAndSlide` traktuje więc nakładanie do 1 mm jak styk. `overlaps` odpowiada na czyste pytanie geometryczne i niczego nie przesuwa, więc nie ma czego tolerować.

10. **Dlaczego `moveAndSlide` zwraca przesunięcie, a nie nową pozycję?**
    Pudełko jest tylko bryłą kolizji, a obiekt gry ma własną pozycję, z której pudełko jest wyliczane. Funkcja mówi "o ile wolno", a wołający dodaje to do pozycji.

11. **Co robi `(axis + 1) % AXIS_COUNT`?**
    Daje numer następnej osi z zawinięciem: po z (2) wraca do x (0). Razem z `(axis + 2) % AXIS_COUNT` wyznacza dwie osie, wzdłuż których pudełko się **nie** porusza, czyli przekrój korytarza ruchu.

12. **Co się stanie z pudełkiem, które zaczyna krok wewnątrz przeszkody?**
    Przeszkoda go nie trzyma: odstęp jest wyraźnie ujemny, więc `allowedDistance` ją pomija i pudełko może wyjść. Funkcja nie wypycha go sama.

13. **Dlaczego kolizje są w `scene/`, a nie w `game/`, i czego nie dołączają?**
    Pudełko i dwie funkcje nie wiedzą nic o labiryncie ani o graczu, więc należą do biblioteki `engine` i nadają się do innych programów. Dołączają tylko GLM i bibliotekę standardową: żadnego OpenGL, okna ani wejścia, dzięki czemu testy działają bez okna.

14. **Jak sprawdzić kolizję kuli z AABB?**
    Przyciąć środek kuli do przedziałów pudełka (osobno x, y, z), co daje najbliższy punkt pudełka, i porównać kwadrat odległości od niego z kwadratem promienia. W projekcie tego testu jeszcze nie ma: jest zaplanowany dla kryształów (późniejszy kamień milowy).

15. **Dlaczego pudełko kolizji ściany ma 0,3 m, skoro ściana ma 0,2 m?**
    Żeby lica pudełek ścian i słupków (0,3 m) leżały w jednej płaszczyźnie. Przy pudełku 0,2 m słupki wystawałyby 5 cm przed ścianę i gracz sunący po ścianie stawałby co 2 metry. Teraz pudełko gracza tylko styka się ze słupkiem, a styk nie zatrzymuje ruchu.

16. **Jak gracz używa `moveAndSlide`?**
    W każdym kroku chodzenia liczy chciane przesunięcie (kierunek razy prędkość razy stały krok), buduje pudełko ze swojej pozycji i woła `moveAndSlide` z listą pudełek labiryntu, policzoną raz przy generowaniu. Wynik dodaje do pozycji. W trybie noclip funkcji nie woła.

17. **Jak rysowane są pudełka kolizji?**
    Jedną siatką sześcianu o boku 1 (8 wierzchołków, 24 indeksy) rysowaną prymitywem `GL_LINES`: każde dwa indeksy to jedna krawędź. Dla każdego pudełka macierz modelu skaluje sześcian do rozmiaru pudełka i przesuwa go do narożnika `min`. Kolor przychodzi jako uniform `uColor`.

18. **Czym `GL_LINES` różni się od `GL_TRIANGLES` w `glDrawElements`?**
    Sposobem grupowania indeksów: przy liniach każde dwa indeksy to odcinek, przy trójkątach każde trzy to trójkąt. Bufory i atrybuty są takie same.

19. **Po co margines 1 cm przy rysowaniu pudełek?**
    Pudełko słupka ma dokładnie szerokość trzonu modelu, więc linie w prawdziwym rozmiarze leżałyby w powierzchni modelu i migotały (walka o głębię). Margines wysuwa je przed powierzchnię. Dotyczy tylko rysowania, kolizje liczą się na prawdziwych pudełkach.

20. **Dlaczego `color.vert` czyta tylko pozycję, skoro siatka ma też normalną i uv?**
    Shader nie musi deklarować wszystkich atrybutów, które VAO udostępnia. Linie w jednym kolorze nie potrzebują normalnej ani współrzędnych tekstury.

21. **Co robi przełącznik `Noclip (key N)` w panelu Collision?**
    Zmienia pole `player.noclip`, to samo, które przełącza klawisz N. W trybie noclip gracz lata wzdłuż kierunku patrzenia i nie woła `moveAndSlide`. Po wyłączeniu najbliższy krok stawia go na podłodze.

## 10. Źródła

- LearnOpenGL, rozdział "Collision detection": <https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection> (AABB z AABB, AABB z kołem przez przycinanie do najbliższego punktu) i "Collision resolution": <https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-resolution>.
- Christer Ericson, "Real-Time Collision Detection" (Morgan Kaufmann, 2005): rozdział 4.2 (AABB i ich reprezentacje), 5.2.5 (test kuli z AABB), 5.5 (testy obiektów w ruchu), 7.1 (siatki jako struktury przyspieszające).
- MDN, "3D collision detection": <https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection> (AABB i kula, krótkie wprowadzenie).
- Glenn Fiedler, "Fix Your Timestep!": <https://gafferongames.com/post/fix_your_timestep/> (dlaczego symulacja idzie stałym krokiem).
- cppreference, `std::span`: <https://en.cppreference.com/w/cpp/container/span>, inicjalizatory desygnowane: <https://en.cppreference.com/w/cpp/language/aggregate_initialization>.
- Dokumentacja OpenGL, `glDrawElements` (prymityw `GL_LINES`): <https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawElements.xhtml>, `glLineWidth`: <https://registry.khronos.org/OpenGL-Refpages/gl4/html/glLineWidth.xhtml>.
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `scene`), [`../game/maze-generator.md`](../game/maze-generator.md) (skąd biorą się pudełka labiryntu), [`../game/player.md`](../game/player.md) (gracz jako użytkownik kolizji), [`../game/maze-rendering.md`](../game/maze-rendering.md) (jedna siatka, wiele macierzy), [`../gfx/mesh.md`](../gfx/mesh.md) (rodzaj prymitywu w `Mesh`), [`../gfx/uniforms.md`](../gfx/uniforms.md) (nazwy uniformów), [`../core/main-loop.md`](../core/main-loop.md) (stały krok, `FIXED_DT`, `MAX_FRAME_TIME`), [`../../libraries/doctest.md`](../../libraries/doctest.md) (testy), [`../../decisions/collision-aabb-sliding.md`](../../decisions/collision-aabb-sliding.md) (dlaczego nie silnik fizyki).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 3 (temat 14 i jego pokaz w ImGui), sekcja 6 (zawartość warstwy `scene/`).
