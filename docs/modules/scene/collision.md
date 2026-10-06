# Moduł scene: kolizje, AABB, kule i przesuwanie wzdłuż ścian

Kamień milowy: M2 + M3 (pudełka i ruch), M5 (kule), M8 część 2 (panel pokazuje ostatni promień wskazywania, ściana otwarta dźwignią przestaje być przeszkodą). Temat wykładu: 14 (Wstęp do kolizji).
Kod: [`src/scene/Collider.hpp`](../../../src/scene/Collider.hpp), [`src/scene/Collider.cpp`](../../../src/scene/Collider.cpp), testy w [`tests/ColliderTests.cpp`](../../../tests/ColliderTests.cpp). Rysowanie pudełek i kul: [`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`src/game/ColliderLines.cpp`](../../../src/game/ColliderLines.cpp), shadery [`assets/shaders/color.vert`](../../../assets/shaders/color.vert) i [`assets/shaders/color.frag`](../../../assets/shaders/color.frag), panel w [`src/debug/panels/CollisionPanel.hpp`](../../../src/debug/panels/CollisionPanel.hpp) i [`src/debug/panels/CollisionPanel.cpp`](../../../src/debug/panels/CollisionPanel.cpp). Użytkownicy: [`src/game/MazeLayout.cpp`](../../../src/game/MazeLayout.cpp) (pudełka ścian i słupków), [`src/game/Player.cpp`](../../../src/game/Player.cpp) (ruch gracza) i [`src/game/Round.cpp`](../../../src/game/Round.cpp) (kule: zbieranie kryształów i strefa wyjścia).

Część modułu `scene`. Wstęp do modułu i jego miejsce w warstwach są w [`README.md`](README.md). Ten dokument korzysta z biblioteki GLM ([`../../libraries/glm.md`](../../libraries/glm.md): `vec3`, dodawanie i odejmowanie wektorów) i odwołuje się do stałego kroku symulacji z [`../core/main-loop.md`](../core/main-loop.md), sekcje 2.2 i 2.3. Testy są napisane w bibliotece doctest ([`../../libraries/doctest.md`](../../libraries/doctest.md)). Skąd biorą się pudełka ścian labiryntu, opisuje [`../game/maze-generator.md`](../game/maze-generator.md), sekcje 2.7 i 5.7. Gracza, który z kolizji korzysta, opisuje [`../game/player.md`](../game/player.md), a reguły rundy, które korzystają z kul, [`../game/gameplay.md`](../game/gameplay.md).

## 1. Po co to jest

Kamera z kamienia milowego M1 latała bez przeszkód: nic jej nie zatrzymywało, więc przelatywała przez kostkę, która wtedy stała na scenie. W labiryncie gracz ma chodzić po korytarzach i zatrzymywać się na ścianach. Potrzebne są do tego dwie rzeczy:

1. **wykrywanie kolizji** (collision detection): odpowiedź na pytanie "czy te dwa obiekty na siebie nachodzą",
2. **reakcja na kolizję** (collision response): decyzja, co zrobić z ruchem, który doprowadziłby do nachodzenia.

Obie realizuje plik `Collider`: struktura `scene::Aabb` (pudełko o ścianach równoległych do osi), funkcja `scene::overlaps` (wykrywanie) i funkcja `scene::moveAndSlide` (reakcja: ruch, który zatrzymuje się na przeszkodzie, ale ślizga się wzdłuż niej).

Od M5 w tym samym pliku jest druga bryła: `scene::Sphere` (kula), z testami `overlaps` dla dwóch kul i dla kuli z pudełkiem oraz z funkcją `closestPoint`. Kula służy do rzeczy, które się zbiera albo w które się wchodzi i które nigdy nie blokują drogi: kryształów i strefy wyjścia. Dla kul jest **samo wykrywanie**, bez reakcji: `moveAndSlide` nadal przesuwa pudełko wśród pudełek i nic nie ślizga się po kuli.

Tak jak `Transform` i `Camera`, to zwykłe dane i matematyka: żadnego wywołania OpenGL, żadnej klawiatury, żadnego czasu. Dzięki temu cały kod da się sprawdzić testami jednostkowymi bez okna.

**Zmiana w M8, części 2 (2026-10-06).** Lista przeszkód gracza może teraz **tracić pudełka ścian**: ściana, którą otworzyła dźwignia, wypada z listy `roundObstacles` w chwili pociągnięcia, tak jak brama w chwili otwarcia (do 1,5 s gracz może przejść przez ścianę, którą jeszcze widać). Lista `MazeWorld::colliders` się nie zmienia: ściany są w niej pierwsze, w kolejności `world.walls`, a `roundObstacles` pomija pudełko ściany numer `i`, gdy `openedWallFlags[i]` jest prawdą. Ta sama lista przeszkód służy promieniowi wskazywania, więc otwarta ściana przestaje też zasłaniać dźwignie i kartki za sobą. Reguły: [`../game/gameplay.md`](../game/gameplay.md). Wybór i alternatywy: [`../../decisions/opened-wall-stops-blocking-at-pull.md`](../../decisions/opened-wall-stops-blocking-at-pull.md). Panel Collision pokazuje od M8, części 2 ostatni promień (sekcja 6).

Do pokazu i do szukania błędów dochodzą dwie rzeczy z programu `night_maze`: klasa `game::ColliderLines`, która rysuje pudełka i kule cienkimi liniami (sekcje 3, 4 i 5.8), i panel Collision (sekcja 6).

**Stan na dziś, uczciwie.** Kod kolizji ma testy: 19 przypadków w `tests/ColliderTests.cpp` (12 dla pudełek i ruchu, 7 dla kul), trzy dalsze, razem z labiryntem, w `tests/MazeLayoutTests.cpp`, cztery z graczem w `tests/PlayerTests.cpp`, a użycie kul w regułach rundy sprawdza `tests/RoundTests.cpp`. Pudełka ścian i słupków labiryntu liczy `game::mazeColliders`, `moveAndSlide` woła gracz w każdym kroku chodzenia (`game::Player::update`), a testy kul woła `game::updateRound` w każdym kroku rundy.

Część z M2 + M3 (pudełka, ruch, żółte linie) była zbudowana i uruchomiona na Windowsie, a na zrzucie ekranu z widoku z góry żółte linie pudełek leżały na ścianach i słupkach. Część z M5 (kule, ich linie, brama na liście przeszkód, nowe napisy panelu) jest gotowa w kodzie na Windowsie, ale M5 **nie jest zamknięte**. Według raportu z 2026-10-05 build Debug i Release przechodzi tam bez ostrzeżeń, a po M5 215 przypadków testowych (85098 asercji) przechodziło w obu konfiguracjach. Po drugiej części M6 było ich 256 (101232 asercje), uruchomione tego samego dnia w Debug i Release, a po pierwszej części M7 zgłoszone jest 269 (102103 asercje), a po drugiej 276 (102139 asercji), po trzeciej 294 (102412 asercji), po czwartej 310 (103751 asercji). Pierwsza część M7 (bufor HDR i gamma) nie zmieniła niczego w `scene/Collider.*`. Zmieniła jedną rzecz w rysowaniu linii: `ColliderLines` przelicza kolor z sRGB na liniowy, bo linie trafiają teraz do bufora HDR sceny (sekcje 4.2 i 5.8). M6 postawił labirynt na terenie z mapy wysokości: kod w `scene/Collider.*` się nie zmienił, ale pudełka ścian, słupków i bramy zaczynają się teraz na wysokości gruntu, a gracz stoi na gruncie, a nie na `y = 0` (sekcja 2.13). M6 też jest kompletny w kodzie na Windowsie i nie jest zamknięty. Chodzenia i ślizgania prawdziwymi klawiszami, zbierania kryształów, przejścia przez otwartą bramę, widżetów panelu Collision i linii kul na ekranie **nikt jeszcze nie sprawdził ręcznie**. Na macOS nic z M5 ani z M6 nie było budowane ani uruchamiane.

## 2. Teoria

### 2.1 Bryła otaczająca i AABB

Model ściany ma kilkadziesiąt trójkątów, model postaci tysiące. Sprawdzanie każdego trójkąta z każdym byłoby wolne i trudne. Dlatego do kolizji używa się **bryły otaczającej** (bounding volume): prostej figury, która zawiera cały obiekt i dla której test jest tani. Kolizję obiektów zastępuje kolizja ich brył.

Najprostsze bryły:

| Bryła | Opis | Test z taką samą bryłą | Wada |
|---|---|---|---|
| kula (sphere) | środek i promień | jedna odległość | źle pasuje do długich i płaskich obiektów, na przykład do ściany. W projekcie od M5 (sekcje od 2.9 do 2.12) |
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

### 2.9 Kula: druga bryła, do zbierania i do wchodzenia

Do ścian AABB wystarcza. Kryształy, które gracz zbiera (M5), są małe i mniej więcej okrągłe, a pytanie brzmi inaczej niż przy ścianie: nie "jak daleko wolno iść", tylko "czy gracz jest dość blisko". Do tego lepiej pasuje **kula** (sphere): wszystkie punkty nie dalsze od **środka** niż **promień**. Cały opis to cztery liczby: trzy współrzędne środka i promień.

Kiedy kula jest lepszym kształtem niż pudełko:

- **gdy liczy się sama odległość.** "Dość blisko" znaczy to samo z każdej strony. Kula wygląda tak samo z każdego kierunku, więc zasięg nie zależy od tego, czy gracz podchodzi wzdłuż osi, czy po skosie. Pudełko sięga w narożniku dalej niż na środku ściany: dla sześcianu o połowie boku 0,6 m to 0,6 m na wprost i około 0,85 m po przekątnej w poziomie,
- **gdy bryła niczego nie blokuje.** Bryła do zbierania (pickup) albo strefa, w którą się wchodzi (trigger), nie zatrzymuje ruchu. Wystarcza wtedy sam test "tak albo nie", a ten jest dla kuli najprostszy z możliwych,
- **gdy obiekt się obraca.** Kryształ kręci się wokół osi pionowej. Kula po obrocie jest tą samą kulą, więc nie trzeba niczego przeliczać (AABB trzeba by: pułapka 9).

Kiedy kula jest gorsza: dla ściany. Kula obejmująca segment długi na 2 m i wysoki na 3 m musiałaby mieć promień około 1,8 m i wystawałaby daleko w korytarz. Dlatego projekt ma **obie** bryły i każdej używa do czego innego:

| Bryła | Do czego w grze | Co liczy kod |
|---|---|---|
| `Aabb` | ściany, słupki, zamknięta brama, gracz jako ciało | test nakładania i ruch ze ślizganiem |
| `Sphere` | zasięg gracza, kryształ do zebrania | tylko test nakładania |
| `Aabb` jako strefa | strefa wyjścia za bramą | tylko test nakładania z kulą zasięgu gracza |

### 2.10 Kula z kulą

Dwie kule nachodzą na siebie, gdy ich środki są **bliżej niż suma promieni**.

```text
widok z boku: dwie kule o promieniach r1 i r2, d to odległość środków

   nachodzą na siebie                 rozłączne

      ,---.,---.                     ,---.        ,---.
     /   ,-\-.  \                   /     \      /     \
    |  c1  | c2  |                 |  c1   |    |  c2   |
     \   `-/-'  /                   \     /      \     /
      `---'`---'                     `---'        `---'

     d < r1 + r2                     d > r1 + r2
```

Odległość `d` między punktami `c1` i `c2` to długość wektora `c2 - c1`, czyli pierwiastek z sumy kwadratów jego składowych (twierdzenie Pitagorasa w trzech wymiarach). Pierwiastek jest najdroższym działaniem w tym wzorze, a do samego porównania nie jest potrzebny. Porównuje się **kwadraty**:

```text
dot(c2 - c1, c2 - c1) < (r1 + r2) * (r1 + r2)
```

Dwie rzeczy do wyjaśnienia:

- **`dot(v, v)` to kwadrat długości.** Iloczyn skalarny wektora z samym sobą to `v.x * v.x + v.y * v.y + v.z * v.z`, czyli dokładnie to, co stoi pod pierwiastkiem we wzorze na długość.
- **Dlaczego wolno porównywać kwadraty.** Dla liczb **nieujemnych** podnoszenie do kwadratu zachowuje kolejność (funkcja jest rosnąca): jeśli `a < b`, to `a * a < b * b`, i odwrotnie. Odległość nigdy nie jest ujemna, a suma promieni też nie, dopóki promienie są nieujemne. Dla liczb ujemnych to nie działa: `-3 < 2`, ale `9 > 4`. Stąd wymaganie w komentarzu przy polu `radius`, że promień nie może być ujemny (pułapka 18).

Częsty błąd: po prawej stronie ma stać **kwadrat sumy**, a nie suma kwadratów. Dla promieni 0,5 i 0,5 kwadrat sumy to 1, a suma kwadratów tylko 0,5 (ćwiczenie 14).

Przykłady na liczbach (oba są w testach, sekcja 5.7). Pierwsza kula ma środek `(0, 1, 0)` i promień 0,5:

| Druga kula | `c2 - c1` | Kwadrat odległości | Kwadrat sumy promieni | Wynik |
|---|---|---|---|---|
| środek `(0,8, 1, 0)`, promień 0,5 | `(0,8, 0, 0)` | 0,64 | 1 | nachodzą (0,64 < 1) |
| środek `(1,5, 1, 0)`, promień 0,5 | `(1,5, 0, 0)` | 2,25 | 1 | rozłączne |

Odległość liczy się we wszystkich trzech kierunkach naraz. Dwie kule o promieniu 1, druga przesunięta o 1,2 m na **każdej** osi: kwadrat odległości to `3 * 1,44 = 4,32`, więcej niż `2 * 2 = 4`, więc są rozłączne (w linii prostej dzieli je około 2,08 m). Przy 1,1 m na każdej osi wychodzi `3 * 1,21 = 3,63`, mniej niż 4: nachodzą.

### 2.11 Najbliższy punkt pudełka i kula z pudełkiem

Kula i pudełko nachodzą na siebie, gdy kula sięga do pudełka. Wystarczy sprawdzić jeden punkt pudełka: ten, który leży **najbliżej środka kuli**. Jeśli kula do niego nie sięga, nie sięga do żadnego innego.

**Najbliższy punkt pudełka.** Znajduje się go osobno na każdej osi. Współrzędna punktu leżąca między `min` a `max` zostaje bez zmian. Współrzędna poza przedziałem jest przesuwana do bliższego z dwóch końców. To działanie nazywa się **przycinaniem** (clamp):

```text
clamp(p, min, max):   p < min          ->  min
                      min <= p <= max  ->  p
                      p > max          ->  max
```

```text
widok z góry (płaszczyzna XZ): pudełko od (0, 0) do (2, 4) i trzy punkty

      z
      ^
    9 |                         C (9, 9)
      |
    4 +---------c               c = (2, 4): narożnik, najbliższy dla C
      |         |                   (przycięte x i z)
      |    A    |
    1 |  (1, 1) b      B (5, 1) b = (2, 1): najbliższy dla B (przycięte tylko x)
      +---------+-------------> x
      0         2               A jest w środku: sam jest swoim najbliższym punktem
```

Przykład na liczbach, w trzech wymiarach. Pudełko ma `min = (0, 0, 0)` i `max = (2, 3, 4)`:

| Punkt | Gdzie leży | Przycinanie oś po osi | Najbliższy punkt pudełka |
|---|---|---|---|
| `(1, 1, 1)` | w środku pudełka | żadna współrzędna się nie zmienia | `(1, 1, 1)` |
| `(5, 1, 1)` | przed ścianą x = 2 | x: 5 na 2 | `(2, 1, 1)` |
| `(-1, 5, 1)` | za krawędzią | x: -1 na 0, y: 5 na 3 | `(0, 3, 1)` |
| `(9, 9, 9)` | za narożnikiem | x: 9 na 2, y: 9 na 3, z: 9 na 4 | `(2, 3, 4)`, sam narożnik |

Dlaczego wolno przycinać każdą oś osobno: kwadrat odległości to suma trzech kwadratów, po jednym na oś, a każdy z nich zależy tylko od jednej współrzędnej. Suma jest najmniejsza, gdy każdy składnik z osobna jest najmniejszy.

**Kula z pudełkiem.** Dalej tak samo jak dla dwóch kul, tylko druga "kula" jest punktem:

```text
najbliższy = clamp(środek, box.min, box.max)        (osobno dla x, y i z)
kolizja, gdy dot(najbliższy - środek, najbliższy - środek) < r * r
```

Przykłady dla pudełka od `(0, 0, 0)` do `(2, 2, 2)` i kuli o środku `(2,5, 1, 1)`, czyli 0,5 m przed ścianą x = 2. Najbliższy punkt to `(2, 1, 1)`, kwadrat odległości 0,25:

| Promień | `r * r` | Wynik |
|---|---|---|
| 0,75 | 0,5625 | nachodzą (0,25 < 0,5625) |
| 0,5 | 0,25 | styk: 0,25 nie jest mniejsze od 0,25, więc nie nachodzą |
| 0,25 | 0,0625 | rozłączne |

Jeśli środek kuli jest **w** pudełku, przycinanie go nie zmienia, odległość wychodzi 0 i każdy promień większy od zera daje kolizję.

### 2.12 Styk, narożnik i to, czego kule nie robią

**Styk nie jest nakładaniem, tak jak dla pudełek.** Oba testy kul używają ostrej nierówności `<`. Kule o promieniach 0,5 i 0,5 ze środkami oddalonymi dokładnie o 1 m nie nachodzą na siebie. To ta sama umowa co w sekcji 2.3: bryły, które mają wspólny tylko brzeg, nie są w kolizji. Dla zbierania nie ma to praktycznego znaczenia (gracz i tak wchodzi głębiej), ale jedna reguła dla wszystkich brył jest łatwiejsza do zapamiętania i do przetestowania.

Testy styku używają liczb, które typ `float` przechowuje dokładnie (0,5, 1, 2, 2,5): tylko wtedy "dokładnie styk" jest naprawdę dokładny (sekcja 2.7).

**Przy narożniku kula jest "okrąglejsza" niż pudełko.** To widać na liczbach z przypadku testowego `near a corner of a box the sphere test is rounder than a box test would be`. Pudełko sięga od `(0, 0, 0)` do `(2, 2, 2)`. Środek kuli to `(2,5, 1, 2,5)`: 0,5 m za pionową krawędzią pudełka na osi x **i** 0,5 m na osi z.

```text
widok z góry: krawędź pudełka w N = (2, 2), środek kuli S = (2,5, 2,5)

      z
      ^
  3,1 |        . . . . . . . .
      |        .             .       kropki: pudełko o połowie boku 0,6 wokół S,
  2,5 |        .      S      .               sięga od 1,9 do 3,1 na obu osiach
      |        .             .
    2 +---------N            .       N: najbliższy punkt dużego pudełka
  1,9 |        .|. . . . . . .       odległość od S do N: około 0,71 m
      | pudełko |                    kula o promieniu 0,6 wokół S do N nie sięga
      +---------+--------------> x
      0   1,9   2    2,5    3,1
```

- Najbliższy punkt pudełka to `(2, 1, 2)`. Różnica to `(-0,5, 0, -0,5)`, kwadrat odległości `0,25 + 0,25 = 0,5`, odległość około 0,71 m.
- Kula o promieniu 0,6: `0,6 * 0,6 = 0,36`, a 0,5 nie jest mniejsze od 0,36. **Nie nachodzi.**
- Kula o promieniu 0,75: `0,5625`, a 0,5 jest mniejsze. **Nachodzi.**
- Pudełko o połowie boku 0,6 wokół tego samego środka sięgałoby na osiach x i z od 1,9 do 3,1, czyli nachodziłoby na narożnik o 0,1 m na obu osiach. Test pudełek powiedziałby "kolizja" tam, gdzie kula o tym samym "promieniu" mówi "nie".

Kula ma więc ten sam zasięg we wszystkich kierunkach, a pudełko o tym samym rozmiarze sięga po przekątnej dalej.

**Czego kule w tym projekcie nie robią.**

- **Nie zatrzymują ruchu.** `moveAndSlide` przyjmuje pudełko i listę pudełek. Nie ma funkcji, która liczyłaby, o ile wolno przesunąć kulę albo jak ślizgać się po kuli. Testy kul to wykrywanie dyskretne (sekcja 2.4): jedno pytanie na krok symulacji, o pozycję po ruchu.
- **Nie przemiatają drogi.** W teorii bryłę do zebrania dałoby się przeskoczyć (sekcja 2.5). W grze nie: krok symulacji przy sprincie to około 4,6 cm, a suma promieni zasięgu gracza i kryształu to 0,9 m.
- **Nie mają tolerancji styku.** Tolerancja należy do ruchu (sekcja 2.7), a kule niczego nie przesuwają.

Poza zakresem projektu zostają: bryły obrócone (OBB), kapsuły, siatki trójkątów, struktury przyspieszające (siatka, drzewo BVH) i pełna symulacja fizyki z masą, pędem i odbiciami ([`../../decisions/collision-aabb-sliding.md`](../../decisions/collision-aabb-sliding.md)). Promienie (raycast) istnieją od M8 jako osobna matematyka bez okna w `src/scene/Raycast.*`, opisana w [`picking.md`](picking.md): nie należą do kolizji, panel Collision ich nie pokazuje, a działający program ich nie używa. Oba testy kul opisuje Ericson (sekcja 10), a wersję 2D rozdział "Collision detection" z LearnOpenGL.

### 2.13 Pudełka na nierównym gruncie: co się zmieniło w M6, a co nie

Do M5 podłoga była płaska i wszystko stało na `y = 0`: pudełko ściany sięgało od 0 do 3 m, pudełko gracza od 0 do 1,8 m, więc na osi y nakładały się zawsze i o wysokości nie trzeba było myśleć. Od M6 podłoże to teren z mapy wysokości ([`../renderer/terrain.md`](../renderer/terrain.md)) i obie bryły mają własną wysokość:

| Bryła | Skąd bierze dół | Wysokość |
|---|---|---|
| pudełko ściany i bramy | najniższy grunt pod obrysem pudełka poszerzonym o 0,05 m (`Terrain::lowestHeightUnder`, `FOOTPRINT_MARGIN`) | 3 m (`WALL_HEIGHT`), bez zmian |
| pudełko słupka | to samo dla obrysu słupka | 3,15 m (`PILLAR_HEIGHT`), bez zmian |
| pudełko gracza | grunt dokładnie pod stopami (`Terrain::heightAt`) | 1,8 m (`Player::BODY_HEIGHT`), bez zmian |

**Co się nie zmieniło: kolizje w poziomie.** `game::placeOnTerrain` zmienia w każdej pozycji tylko `y`. `x` i `z` pudełek są takie same jak na płaskim gruncie, a klawisze przesuwają gracza tylko w poziomie. Ruch w planie jest więc identyczny: test `the walls stop the player on uneven ground exactly as on flat ground` prowadzi tego samego gracza tymi samymi klawiszami przez ten sam labirynt raz na płasko i raz na nierównym gruncie przy największej skali wysokości i sprawdza, że `x` i `z` są **równe** po każdym z 3600 kroków.

**Co trzeba było zagwarantować: wspólny zakres wysokości.** Test nakładania jest trójwymiarowy (sekcja 2.2): ściana zatrzymuje gracza tylko wtedy, gdy ich pudełka mają wspólną długość na **każdej** osi, także na y. Gdyby dół pudełka ściany znalazł się wyżej niż głowa gracza albo jej góra niżej niż jego stopy, `moveAndSlide` uznałby, że ściana nie stoi na drodze, i gracz przeszedłby pod nią albo nad nią, choć na ekranie ściana by tam stała.

Rachunek. Niech `f` to wysokość stóp gracza, a `b` wysokość dołu pudełka ściany. Pudełka mają wspólny zakres na y, gdy:

```text
f < b + 3        stopy gracza poniżej góry ściany
f + 1,8 > b      głowa gracza powyżej dołu ściany
```

Wysokość gruntu to `heightScale * relief * próbka`, gdzie próbka mapy wysokości jest liczbą od 0 do 1, a `relief` wewnątrz labiryntu wynosi `MAZE_RELIEF = 0,6` m. Cały grunt pod labiryntem mieści się więc między 0 a `heightScale * 0,6` m. Suwak skali kończy się na `MAX_HEIGHT_SCALE = 2,5`:

```text
największa różnica wysokości gruntu w labiryncie = 2,5 * 0,6 = 1,5 m
```

I `f`, i `b` są wysokościami gruntu w labiryncie, więc różnią się najwyżej o 1,5 m, w dowolną stronę. Obie nierówności są wtedy spełnione z zapasem: `1,5 < 3` i `1,5 < 1,8`. Wspólny zakres ma co najmniej `1,8 - 1,5 = 0,3` m. Tak to opisuje komentarz przy stałej w `Terrain.hpp`: "The limit keeps the ground inside the maze flatter than the player is tall: the collision boxes of the walls then always reach the box of a player who stands next to them".

Dlaczego rachunek idzie od 1,5 m, a nie od tego, co widać w labiryncie startowym. Tam przy skali 1 grunt ma tylko od 0,085 do 0,461 m, bo fragment mapy pod labiryntem 10 na 10 nie sięga od czerni do bieli. Mapa wysokości powtarza się jednak co 48 m w metrach świata, więc większy labirynt obejmuje ją całą i może trafić na pełny zakres próbek od 0 do 1. Gwarancja musi działać dla każdego labiryntu, stąd najgorszy przypadek.

To gwarancja z zapasem, nie opis typowej sytuacji. Ściana stoi na **najniższym** gruncie pod sobą, a gracz tuż obok niej, więc zwykle `b` jest trochę niższe od `f` i różnica to centymetry. Test `on uneven ground the walls, pillars and the gate are sunk until no gap shows` ma podprzypadek `a player next to a wall always overlaps its box in height`: dla każdego pudełka labiryntu 6 na 5 przy największej skali sprawdza w 25 punktach wokół niego, w odległości połowy ciała gracza, że wspólny zakres na y jest większy niż 0,1 m, czyli sto razy większy niż tolerancja styku.

Granica dotyczy gruntu **w labiryncie**. Na zewnątrz rzeźba rośnie do `HILL_RELIEF = 4,5` m, ale tam nie ma ścian poza zewnętrznymi, a tuż przy nich rzeźba jest jeszcze praktycznie taka jak w środku: przejście we wzgórza zaczyna się płasko (krzywa smoothstep).

**Czego nadal nie ma: kolizji z gruntem.** Terenu nie ma na liście przeszkód i `moveAndSlide` nic o nim nie wie. Wysokość gracza jest **czytana** z terenu po każdym kroku (`position.y = terrain.heightAt(...)`), a nie wynikiem zderzenia z nim ([`../game/player.md`](../game/player.md), sekcja 2.5). Skutki: gracz nie może wejść pod grunt ani nad niego wyskoczyć przy chodzeniu, nie zsuwa się ze stoków i nie zwalnia pod górę, a w trybie noclip przelatuje przez grunt tak samo jak przez ściany.

## 3. Jak to działa w OpenGL

Same kolizje to czysta matematyka na procesorze. `Collider.hpp` i `Collider.cpp` nie dołączają GLAD i nie wołają żadnej funkcji `gl*`. OpenGL nie wie, że jakieś pudełka i kule istnieją, i niczego nie sprawdza: karta graficzna narysuje dwa obiekty jeden w drugim bez żadnego błędu.

Związek z renderowaniem jest pośredni: wynik `moveAndSlide` zmienia pozycję gracza, z pozycji gracza powstaje punkt oka, a z niego macierz widoku ([`../game/player.md`](../game/player.md), sekcja 5). Kolizja decyduje więc o tym, **skąd** rysowana jest klatka, a nie o tym, jak. Testy kul decydują o tym, **co** jest rysowane: zebrany kryształ znika razem ze swoim światłem ([`../game/gameplay.md`](../game/gameplay.md), sekcja 5).

OpenGL pojawia się dopiero przy **rysowaniu brył** jako pomocy diagnostycznej (klasa `game::ColliderLines`, sekcja 5.8). Pudełko jest rysowane jako 12 krawędzi sześcianu, kula jako trzy okręgi. Jedno i drugie liniami.

**Prymityw `GL_LINES`.** Modele labiryntu są rysowane trójkątami (`GL_TRIANGLES`: każde trzy indeksy to jeden trójkąt). Pierwszy parametr `glDrawElements` może też wskazać linie: przy `GL_LINES` **każde dwa indeksy to jeden odcinek**. Sześcian ma 8 narożników i 12 krawędzi, więc wystarcza 8 wierzchołków i 24 indeksy. Okrąg jest łamaną zamkniętą z 32 odcinków: 32 wierzchołki i 64 indeksy. Ta sama klasa `gfx::Mesh` obsługuje wszystkie przypadki: rodzaj prymitywu jest parametrem jej konstruktora ([`../gfx/mesh.md`](../gfx/mesh.md), sekcja 5).

Liczby w tabeli są wyliczone z kodu dla labiryntu startowego (10 na 10, ziarno 1) na początku rundy: brama zamknięta, 13 kryształów na miejscu. Nie są zmierzone w działającym programie.

| Krok w klatce (gdy rysowanie brył jest włączone) | Wywołania OpenGL | Ile razy |
|---|---|---|
| `m_colorShader.use()` | `glUseProgram` | 1 |
| `setMat4` dla `uView` i `uProjection` | `glGetUniformLocation`, `glUniformMatrix4fv` | po 1 |
| `setVec3(COLOR_UNIFORM, color)` | `glGetUniformLocation`, `glUniform3fv` | 6: po jednym na listę (labirynt, pudełko gracza, brama, strefa wyjścia, zasięg gracza, kule kryształów) |
| `setMat4(MODEL_UNIFORM, ...)` | `glGetUniformLocation`, `glUniformMatrix4fv` | 287: 245 dla pudełek i 42 dla okręgów |
| `m_unitCube.draw()` | `glBindVertexArray`, `glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, ...)` | 245: 121 ścian, 121 słupków, gracz, brama, strefa wyjścia |
| `m_unitCircle.draw()` | `glBindVertexArray`, `glDrawElements(GL_LINES, 64, GL_UNSIGNED_INT, ...)` | 42: 14 kul (zasięg gracza i 13 kryształów) po 3 okręgi |

Liczby zmieniają się w trakcie rundy: każdy zebrany kryształ to 3 okręgi mniej, a po otwarciu bramy znika jej pudełko razem z jednym ustawieniem koloru.

Pięć szczegółów:

- **Jedna siatka dla wszystkich pudełek.** Na karcie leży jeden sześcian o boku 1. Każde pudełko to ten sześcian z inną macierzą modelu: skala równa rozmiarowi pudełka i przesunięcie do jego narożnika `min`. To ta sama zasada co przy ścianach labiryntu ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 2).
- **Jedna siatka dla wszystkich kul.** Na karcie leży jeden okrąg o promieniu 1 w płaszczyźnie XY. Kula to ten okrąg narysowany trzy razy, z trzema obrotami: bez obrotu (stoi w płaszczyźnie XY), obrócony o 90 stopni wokół osi X (leży płasko, w płaszczyźnie XZ) i obrócony o 90 stopni wokół osi Y (stoi w płaszczyźnie YZ). Do tego skala równa promieniowi i przesunięcie do środka kuli.
- **Dlaczego trzy okręgi wystarczą.** Pełna siatka kuli z południków i równoleżników zasłaniałaby kryształ plątaniną linii. Trzy okręgi, po jednym wokół każdej osi, pokazują dokładnie to, co trzeba odczytać: gdzie jest środek (tam się przecinają ich płaszczyzny) i jak daleko sięga promień w każdym z trzech kierunków. Patrząc wzdłuż dowolnej osi, widać jeden okrąg w pełnym kształcie i dwa jako odcinki, więc rysunek jest czytelny z każdej strony.
- **Szerokość linii to 1 piksel.** Kod nie woła `glLineWidth`. Profil Core nie musi obsługiwać linii szerszych niż 1, a implementacja OpenGL w macOS jest znana z tego, że ich nie obsługuje (tak mówi też komentarz w kodzie, na Macu tego nie sprawdzałem), więc wartość domyślna jest jedyną przenośną.
- **Test głębi zostaje włączony.** Linia za ścianą jest przez nią zasłonięta. Dzięki temu widać, gdzie bryła naprawdę jest, a nie plątaninę wszystkich krawędzi labiryntu naraz.

**Okrąg z 32 odcinków nie jest okręgiem.** To wielokąt wpisany w okrąg: jego wierzchołki leżą na okręgu, a środki boków odrobinę bliżej środka. Dla 32 boków różnica wynosi `1 - cos(180 / 32 stopni)`, czyli około 0,5 procent promienia: dla kuli kryształu o promieniu 0,6 m to około 3 mm. Na ekranie tego nie widać, a liczba 32 jest stałą `CIRCLE_SEGMENTS`.

**Walka o głębię (z-fighting).** Pudełko słupka ma dokładnie szerokość trzonu modelu słupka (0,3 m). Linie narysowane w prawdziwym rozmiarze leżałyby więc **w** powierzchni modelu: dla tych samych pikseli linia i ściana miałyby prawie tę samą głębię, a o tym, co wygra test głębi, decydowałyby błędy zaokrągleń, inne w każdej klatce. Linie migotałyby. Rozwiązanie w projekcie jest najprostsze z możliwych: rysowane pudełko jest większe o 1 cm z każdej strony, więc linie są wyraźnie przed powierzchnią. Kolizje nadal liczą się na prawdziwych pudełkach. Okręgi kul marginesu nie mają i go nie potrzebują: kule wiszą w powietrzu, w środku komórki albo wokół gracza, i z żadną powierzchnią modelu się nie pokrywają.

## 4. Shadery

Same kolizje nie mają shadera. Linie pudełek i kul rysuje najprostsza para w projekcie: [`assets/shaders/color.vert`](../../../assets/shaders/color.vert) i [`assets/shaders/color.frag`](../../../assets/shaders/color.frag). Wszystko, co nimi narysowane, ma jeden kolor. Linie kolizji są dziś jedynym użytkownikiem tej pary. W M4 rysowała jeszcze małe kostki w miejscach świateł punktowych. W M5 ten kod zniknął: źródłem światła jest widoczny kryształ ([`../game/gameplay.md`](../game/gameplay.md), sekcja 4), a komentarze nagłówkowe obu shaderów wymieniają już tylko linie pudełek i kul.

### 4.1 `color.vert`

```glsl
#version 410 core
// Vertex shader for shapes drawn in one flat colour: the lines of the collision boxes
// and spheres.
// See docs/modules/scene/collision.md

// Input: only the position. The mesh also carries a normal (location 1), a texture
// coordinate (location 2) and a tangent (location 3), but a shader may leave attributes
// it does not need unread.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the shape

// Uniforms: set from C++ (gfx::Shader::setMat4).
uniform mat4 uModel;      // local space to world space
uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space

void main() {
    // The same chain as in textured.vert: local, world, view, clip space.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `#version 410 core` | GLSL 4.10, profil Core: ta sama wersja co kontekst OpenGL projektu |
| `layout(location = 0) in vec3 aPosition;` | jedyne wejście: pozycja, atrybut numer 0. Numer zgadza się ze stałą `POSITION_ATTRIBUTE` z `src/gfx/Vertex.hpp` |
| brak `aNormal`, `aUv` i `aTangent` | siatka `gfx::Mesh` zawsze opisuje cztery atrybuty (pozycja, normalna, uv, styczna: [`../gfx/mesh.md`](../gfx/mesh.md)). Styczna doszła razem z mapami normalnych ([`../gfx/normal-mapping.md`](../gfx/normal-mapping.md)), a linie jej nie potrzebują. Shader nie musi czytać wszystkich: atrybut włączony w VAO, którego shader nie deklaruje, jest po prostu ignorowany |
| `uniform mat4 uModel;`, `uView`, `uProjection` | te same trzy macierze i te same nazwy co w `textured.vert`, `lit.vert` i `gouraud.vert`, dzięki czemu kod C++ używa dla wszystkich programów tych samych stałych z `ShaderUniforms.hpp` ([`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 5) |
| `gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);` | łańcuch czytany od prawej: przestrzeń lokalna, świat, widok, przycinanie ([`transforms.md`](transforms.md), sekcja 4). `1.0` jako czwarta składowa oznacza punkt, więc przesunięcie z macierzy działa. Komentarz odsyła do `textured.vert`, bo tam łańcuch jest opisany w całości |

Shader nie ma żadnego wyjścia poza `gl_Position`: fragmentom nie trzeba niczego przekazywać, bo kolor jest ten sam dla całego kształtu.

### 4.2 `color.frag`

```glsl
#version 410 core
// Fragment shader for shapes drawn in one flat colour: the lines of the collision boxes
// and spheres.
// See docs/modules/scene/collision.md

// The colour of the whole shape (red, green, blue), set from C++ (gfx::Shader::setVec3)
// as a LINEAR colour: game::ColliderLines converts the colours it is given.
uniform vec3 uColor;

// Output: the color written to the HDR framebuffer of the scene (red, green, blue,
// alpha). The composite pass encodes it for the screen.
out vec4 fragColor;

void main() {
    // Every fragment gets the same colour. Alpha 1 means fully opaque.
    fragColor = vec4(uColor, 1.0);
}
```

| Linia | Znaczenie |
|---|---|
| `uniform vec3 uColor;` | kolor całego kształtu. To uniform, a nie atrybut: jest stały dla wszystkich wierzchołków jednego wywołania rysującego, a zmienia się między wywołaniami. Od pierwszej części M7 jest to kolor **liniowy**: `ColliderLines` przelicza go z wartości sRGB przed wysłaniem (sekcja 5.8) |
| `out vec4 fragColor;` | wyjście shadera fragmentów. Od M7 kolor trafia do framebuffera HDR sceny, a nie do okna: na ekran przenosi go dopiero przebieg składający, który koduje go do sRGB ([`../renderer/post-process.md`](../renderer/post-process.md)) |
| `fragColor = vec4(uColor, 1.0);` | trzy składowe koloru i alfa 1, czyli pełne krycie |

Różnica wobec `textured.frag`: tam shader fragmentów dostaje od shadera wierzchołków współrzędne tekstury, normalną i styczną (zmienne `in`, interpolowane w poprzek trójkąta) i czyta teksturę. Tutaj nie ma żadnego wejścia z shadera wierzchołków: kolor przychodzi jako jedna wartość dla wszystkich fragmentów. Dlatego ta para jest najprostszym przykładem programu shaderów w projekcie.

Kolory ustawia `NightMazeApp` (sekcja 5.8). Jest ich pięć, a zielony służy dwóm bryłom gracza:

| Stała w `NightMazeApp.cpp` | Wartość (R, G, B) | Kolor | Co nim rysuję |
|---|---|---|---|
| `MAZE_COLLIDER_COLOR` | `(1, 0,85, 0,1)` | żółty | pudełka ścian i słupków |
| `PLAYER_COLLIDER_COLOR` | `(0,2, 1, 0,4)` | zielony | pudełko gracza i kula jego zasięgu |
| `GATE_COLLIDER_COLOR` | `(1, 0,45, 0,1)` | pomarańczowy | pudełko bramy, dopóki blokuje drogę |
| `PICKUP_COLLIDER_COLOR` | `(0,2, 0,9, 1)` | cyjan | kule zbierania wokół kryształów, które jeszcze wiszą |
| `EXIT_ZONE_COLOR` | `(1, 0,3, 0,9)` | magenta | pudełko strefy wyjścia |

**Te liczby są wartościami sRGB.** To kolory "nazwane dla ekranu", dobrane na oko, tak jak kolor w próbniku. Bufor sceny trzyma od M7 wartości liniowe, więc `ColliderLines` przelicza każdy kolor funkcją `gfx::srgbToLinear`, zanim wyśle go do `uColor`. Żółty `(1, 0,85, 0,1)` trafia do shadera jako `(1, 0,692, 0,010)`. Na końcu klatki przebieg składający koduje obraz z powrotem do sRGB. Gdyby robił tylko to (mapowanie tonów `None`, ekspozycja 1, a od trzeciej części M7 także wyłączona mgła i winieta, które działają na linie tak samo jak na resztę sceny), ekran pokazałby dokładnie liczby z tabeli. Przy domyślnych ustawieniach (krzywa ACES) linie przechodzą jednak przez tę samą krzywą co cała scena i wychodzą trochę ciemniejsze w jasnych kanałach: żółty jako około `(0,91, 0,86, 0,05)`, zielony jako około `(0,16, 0,91, 0,47)`. Odcień zostaje rozpoznawalny, ale to nie są już te same liczby. Tylko dwa widoki do szukania błędów (normalne i UV) omijają ekspozycję i krzywą, a przy nich linie są rysowane tak samo. Teoria: [`../gfx/color-space.md`](../gfx/color-space.md).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/scene/Collider.hpp`](../../../src/scene/Collider.hpp) | stała `CONTACT_TOLERANCE`, struktura `Aabb` z funkcją `fromCenter`, struktura `Sphere`, deklaracje trzech przeciążeń `overlaps`, funkcji `closestPoint` i `moveAndSlide` |
| [`src/scene/Collider.cpp`](../../../src/scene/Collider.cpp) | stałe osi, funkcje pomocnicze `sharedLength` i `allowedDistance`, definicje sześciu funkcji publicznych: `Aabb::fromCenter`, trzy `overlaps`, `closestPoint`, `moveAndSlide` |
| [`tests/ColliderTests.cpp`](../../../tests/ColliderTests.cpp) | 19 przypadków testowych samych kolizji: 12 dla pudełek i ruchu, 7 dla kul (sekcja 5.7) |
| [`tests/MazeLayoutTests.cpp`](../../../tests/MazeLayoutTests.cpp) | trzy przypadki łączące kolizje z labiryntem (zamknięta komórka, wędrówka po labiryncie, ślizganie obok słupków) |
| [`src/game/MazeLayout.hpp`](../../../src/game/MazeLayout.hpp), [`.cpp`](../../../src/game/MazeLayout.cpp) | pierwszy użytkownik `Aabb`: `wallBox`, `pillarBox`, `mazeColliders` ([`../game/maze-generator.md`](../game/maze-generator.md), sekcja 5.7) |
| [`src/game/Player.hpp`](../../../src/game/Player.hpp), [`.cpp`](../../../src/game/Player.cpp) | użytkownik `moveAndSlide`: `Player::box` i `Player::update` (sekcja 5.6 i [`../game/player.md`](../game/player.md)) |
| [`src/game/Round.hpp`](../../../src/game/Round.hpp), [`.cpp`](../../../src/game/Round.cpp) | użytkownik kul: `playerReach`, zbieranie kryształów i test strefy wyjścia w `updateRound`. Do tego `roundObstacles`, czyli lista przeszkód gracza z bramą (sekcje 5.6 i 5.10, [`../game/gameplay.md`](../game/gameplay.md)) |
| [`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`.cpp`](../../../src/game/ColliderLines.cpp) | rysowanie pudełek i kul liniami (sekcja 5.8) |
| [`assets/shaders/color.vert`](../../../assets/shaders/color.vert), [`color.frag`](../../../assets/shaders/color.frag) | shadery jednego koloru (sekcja 4) |
| [`src/debug/panels/CollisionPanel.hpp`](../../../src/debug/panels/CollisionPanel.hpp), [`.cpp`](../../../src/debug/panels/CollisionPanel.cpp) | panel Collision (sekcja 6) |

Pliki `src/scene/Collider.*` należą do biblioteki `engine`, tak jak reszta `src/scene/`. Nie ma w nich nic specyficznego dla Night Maze: pudełko nie wie, czy jest ścianą, graczem czy skrzynią, a kula nie wie, czy jest kryształem. `ColliderLines.*` i `CollisionPanel.*` należą do programu `night_maze`, bo wymagają kontekstu OpenGL albo ImGui.

Dołączane nagłówki: `<glm/glm.hpp>` i `<span>` w nagłówku, `<algorithm>` (`std::min`, `std::max`), `<array>` i `<cmath>` (`std::abs`) w pliku `.cpp`. Funkcje `glm::dot` i `glm::clamp`, których używają testy kul, przychodzą z `<glm/glm.hpp>`. Nic z `core/`, `gfx/`, GLAD ani GLFW.

Kolejność podsekcji idzie za historią kodu: od 5.2 do 5.8 pudełka i ruch (M2 + M3), a kule z M5 są w 5.9 (kod) i 5.10 (kto ich używa). Rysowanie obu brył jest razem w 5.8, a wszystkie testy razem w 5.7.

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
- **Warunek `min <= max` nie jest sprawdzany.** Pudełko z `min` większym od `max` ma ujemną "wspólną długość" z każdym innym i po cichu z niczym nie koliduje (pułapka 3). Sprawdzanie przy każdym teście kosztowałoby więcej niż sam test. Kolejność narożników jest więc obowiązkiem tego, kto tworzy pudełko: wynik `fromCenter` jest poprawny, dopóki połowy rozmiarów są nieujemne (funkcja tego nie sprawdza), a przy inicjalizatorze z polami `.min` i `.max` trzeba samemu podać mniejszy narożnik jako `min`. Kod gry tworzy pudełka tylko przez `fromCenter` ze stałych dodatnich połów rozmiarów (`game::wallBox`, `game::pillarBox`, `Player::box`, a od M5 także `game::exitZone`).

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

To jedno z trzech przeciążeń `overlaps`: kompilator wybiera właściwe po typach argumentów. Dwa pozostałe, dla dwóch kul i dla kuli z pudełkiem, są w sekcji 5.9.

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

Funkcja sprawdza wszystkie przeszkody z listy, bez żadnego wstępnego odsiewania. Labirynt 16 na 16 komórek ma 578 pudełek (289 ścian i 289 słupków), a od M5 o jedno więcej, dopóki brama jest zamknięta: liczby niżej są dla samych ścian i słupków. Przy ruchu poziomym listę przechodzą dwie osie (trzecia kończy się na pierwszej linii, bo przesunięcie w pionie jest zerem), czyli 1156 przeszkód na krok i około 139 tysięcy na sekundę przy 120 krokach. Dla każdej to kilka porównań liczb. Struktury przyspieszającej (na przykład siatki komórek) nie ma, bo przy takiej skali nie jest potrzebna. Czasu tego kodu nie mierzyłem w działającej grze: liczby wynikają z rozmiaru listy.

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

Funkcja przyjmuje **tylko pudełka**: i to, co się rusza, i przeszkody mają typ `Aabb`. Wersji dla kul nie ma (sekcja 2.12).

Dlaczego funkcja zwraca przesunięcie, a nie przesuwa obiektu: `Aabb` to tylko bryła kolizji. Obiekt gry ma swoją pozycję (gracz: punkt, wokół którego buduje się pudełko i z którego liczy się pozycję kamery), a pudełko jest z niej wyliczane. Funkcja, która zwraca "o ile wolno", nie musi wiedzieć, czym jest przesuwany obiekt.

Dlaczego kolejność x, z, y: ruch w labiryncie jest prawie zawsze poziomy, więc obie osie poziome idą pierwsze, a pionowa na końcu. Dopóki nie ma grawitacji ani skoków, składowa y przesunięcia wynosi zero i trzeci obrót pętli kończy się w pierwszej linii `allowedDistance`.

### 5.6 Jak korzysta z tego gracz

Koniec funkcji `game::Player::update`, czyli krok chodzenia ([`src/game/Player.cpp`](../../../src/game/Player.cpp)):

```cpp
    position.y = terrain.heightAt(position.x, position.z);

    // The keys move the player in the horizontal plane only: direction has no vertical
    // part here, so the speed over the ground is the same uphill and downhill.
    const float speed = input.sprint ? sprintSpeed : walkSpeed;
    const glm::vec3 wanted = direction * (speed * stepSeconds);

    // The walls take away the part of the movement that would go into them and leave the
    // part along them. The box is built anew from the position in every step.
    position += scene::moveAndSlide(box(), wanted, obstacles);

    // The ground is uneven, so the place the step ended at has a height of its own.
    position.y = terrain.heightAt(position.x, position.z);
```

Od M6 wywołanie `moveAndSlide` stoi między dwoma odczytami wysokości z terenu. Pierwszy stawia pudełko gracza na gruncie, zanim zostanie porównane ze ścianami, drugi poprawia wysokość w miejscu, w którym krok się skończył. Samo `moveAndSlide` dostaje przesunięcie bez składowej pionowej i o terenie nie wie nic (sekcja 2.13).

A tak woła ją aplikacja w `NightMazeApp::onUpdate`:

```cpp
    m_player.update(wanted, m_camera.yawDegrees, m_camera.pitchDegrees, static_cast<float>(fixedDt),
                    m_obstacles, m_mazeWorld.terrain);
```

Do M4 listą przeszkód była `m_mazeWorld.colliders`, czyli same ściany i słupki. Od M5 gracz porusza się wśród `m_obstacles` (a od M6 dostaje jeszcze teren, z którego czyta wysokość stóp): to pole aplikacji typu `std::vector<scene::Aabb>`, które buduje `game::roundObstacles` ([`src/game/Round.cpp`](../../../src/game/Round.cpp)):

```cpp
std::vector<scene::Aabb> roundObstacles(const MazeWorld& world, const Round& round) {
    std::vector<scene::Aabb> obstacles = world.colliders;
    if (gateBlocks(world, round)) {
        obstacles.push_back(world.gateBox);
    }
    return obstacles;
}
```

| Linia | Znaczenie |
|---|---|
| `std::vector<scene::Aabb> obstacles = world.colliders;` | kopia listy labiryntu: pudełka wszystkich ścian, potem wszystkich słupków. Ta część nie zmienia się przez cały czas życia labiryntu |
| `if (gateBlocks(world, round))` | brama blokuje, gdy labirynt ją ma i jeszcze się nie otworzyła (`world.hasGate && !round.gateOpen`) |
| `obstacles.push_back(world.gateBox);` | pudełko bramy jako ostatnie na liście. `gateBox` to wynik `wallBox` dla segmentu bramy, czyli dokładnie takie pudełko, jakie miałaby ściana w tym miejscu: 2 m długości, 3 m wysokości, 0,3 m grubości. Od M6 brama, tak jak ściana, stoi na najniższym gruncie pod swoim obrysem i pudełko jest liczone po jej opuszczeniu |

Brama nie jest w `MazeWorld::colliders`, bo w trakcie rundy przestaje być przeszkodą, a `MazeWorld` opisuje to, co się w labiryncie nie zmienia. Dla `moveAndSlide` brama niczym się nie różni od ściany: to jeszcze jedno `Aabb` na liście.

Lista jest budowana w trzech miejscach `NightMazeApp`: w `beginRound` (nowa runda, także po nowym labiryncie), od M6 w `rebuildTerrain` (zmiana skali wysokości terenu przesuwa wszystkie pudełka w pionie) i w `onUpdate`, w kroku, w którym brama się otworzyła:

```cpp
    const bool gateBlockedBefore = gateBlocks(m_mazeWorld, m_round);
    updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn,
                static_cast<float>(fixedDt));
    // The gate has just opened (the only change a step can make here): its box leaves
    // the obstacle list, and the way into the exit cell is free.
    if (gateBlocks(m_mazeWorld, m_round) != gateBlockedBefore) {
        m_obstacles = roundObstacles(m_mazeWorld, m_round);
    }
```

Kod pyta o stan bramy przed krokiem reguł i po nim, a listę kopiuje tylko wtedy, gdy odpowiedź się zmieniła. W zwykłym kroku nie powstaje więc żadna nowa lista (pułapka 8). Kolejność w kroku jest taka: najpierw ruch gracza ze starą listą, potem reguły rundy z pozycją po ruchu. Brama otwarta w tym kroku przestaje blokować od następnego.

Trzy elementy spotykają się w jednym kroku symulacji:

| Element | Skąd pochodzi | Jak często jest liczony |
|---|---|---|
| chciane przesunięcie `wanted` | kierunek z klawiszy razy prędkość razy `stepSeconds`, a `stepSeconds` to zawsze `fixedDt` | co krok |
| pudełko `box()` | z pozycji stóp gracza: 0,6 x 1,8 x 0,6 m | co krok, od nowa |
| lista przeszkód `obstacles` | `m_obstacles`, wynik `game::roundObstacles`: pudełka labiryntu i pudełko bramy, dopóki brama jest zamknięta | na początku rundy i w chwili otwarcia bramy |

Kod pilnuje więc trzech rzeczy, o których mówi teoria: przesunięcie powstaje ze stałego kroku (sekcja 2.8), pudełko jest budowane od nowa z pozycji, a lista przeszkód nie jest liczona w każdym kroku. W trybie noclip gracz w ogóle nie woła `moveAndSlide`. Testy kul z `updateRound` działają jednak także w noclip (sekcja 5.10). Całą funkcję linia po linii omawia [`../game/player.md`](../game/player.md), sekcja 5.

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
| `...handles the vertical axis too` | pudełko 1 m nad płytą (w komentarzach testu "a floor slab": to przeszkoda zbudowana w teście, nie podłoże gry) spada o 5 m, potem idzie po płycie | spada dokładnie o 1 m, po płycie idzie bez przeszkód |
| `...does not hold a box that starts inside an obstacle` | środek pudełka w środku ściany, krok 2 m na zewnątrz | całe przesunięcie |
| `many small steps along a wall never stick...` | 2000 kroków `(0,0004, 0, 0,0251)` przy ścianie w x = 37,3 (kąt około 1 stopnia, współrzędne dalekie od zera) | ruch wzdłuż ściany ani razu nie został obcięty, zagłębienie nigdy nie przekroczyło `CONTACT_TOLERANCE` |
| `a box slides across the joint of two wall segments` | dwa segmenty ściany w jednej linii, stykające się końcami, 100 kroków `(0,02, 0, 0,03)` | pudełko nie zatrzymuje się na łączeniu |
| `documented limit: a step much longer than the boxes...` | słupek na przekątnej: jeden krok `(2, 0, 2)` i ta sama droga w 80 krokach | długi krok mija słupek (sekcja 2.8), krótkie kroki na niego trafiają i muszą go obejść |

Siedem przypadków dla kul, dopisanych w M5 w tym samym pliku (razem jest ich w nim 19):

| Przypadek testowy | Co sprawdza | Wynik |
|---|---|---|
| `two spheres overlap when their centres are closer than the sum of the radii` | kula o promieniu 0,5 z samą sobą, z kulą 0,8 m dalej, z kulą 1,5 m dalej i z kulą o promieniu 0,45 odległą o 0,9 m, w obu kolejnościach argumentów | tak, tak, nie, tak w obie strony |
| `the distance between two spheres is measured in all three directions` | dwie kule o promieniu 1: druga przesunięta o 1,2 m na każdej osi, o 1,1 m na każdej osi, o 1,9 m i o 2,1 m prosto w górę | nie (około 2,08 m), tak (około 1,91 m), tak, nie |
| `spheres that only touch do not overlap` | środki odległe dokładnie o 1 m przy promieniach 0,5 i 0,5. Potem kula o promieniu 0, czyli punkt: wewnątrz innej kuli i z samym sobą | styk to nie nakładanie. Punkt wewnątrz kuli nachodzi na nią, punkt na punkt nie |
| `closestPoint keeps a point inside the box and moves a point outside to its surface` | pudełko od `(0, 0, 0)` do `(2, 3, 4)` i cztery punkty: w środku, przed ścianą, za krawędzią, za narożnikiem | cztery wiersze tabeli z sekcji 2.11 |
| `a sphere overlaps a box when it reaches the closest point of the box` | pudełko od `(0, 0, 0)` do `(2, 2, 2)`: środek kuli w pudełku, 0,5 m przed ścianą x = 2 z promieniem 0,75 i 0,25, nad górną ścianą w odległości 0,5 m i 1 m z promieniem 0,75 | tak, tak, nie, tak, nie |
| `near a corner of a box the sphere test is rounder than a box test would be` | środek `(2,5, 1, 2,5)`, promień 0,6 i 0,75 (sekcja 2.12) | nie, tak |
| `a sphere that only touches a box does not overlap it` | kula `(2,5, 1, 1)` o promieniu 0,5 dotyka ściany x = 2. Potem kula o promieniu 0: na ścianie i w środku pudełka | trzy razy nie |

Dwa ostatnie wiersze z promieniem 0 pokazują drobną niesymetrię, którą warto znać (pułapka 19): punkt wewnątrz **kuli** nachodzi na nią, a punkt wewnątrz **pudełka** nie.

I trzy przypadki z `tests/MazeLayoutTests.cpp`, w których przeszkodami są prawdziwe pudełka labiryntu:

| Przypadek testowy | Co sprawdza | Wynik |
|---|---|---|
| `the colliders of a closed cell keep a box inside it` | pudełko w zamkniętej komórce pchane na wschód przez 400 kroków | staje przy wewnętrznym licu pudełka ściany: środek w x = 1,55 (`2 - 0,15 - 0,3`) |
| `a box wandering through a generated maze...` | labirynt 8 na 8 (ziarno 3), 400 zmian kierunku po 60 kroków, 16 kierunków, część prawie równoległa do ścian | pudełko pomniejszone z każdej strony o dwie tolerancje (2 mm) ani razu nie nachodzi na żadną przeszkodę, a wędrówka oddala się od startu o ponad dwie komórki |
| `a box that hugs a wall slides past the pillars in the middle of it` | korytarz 1 na 3 komórki. Pudełko przytulone do ściany wschodniej idzie prosto na południe (200 kroków), potem to samo ze środka komórki ruchem ukośnym w ścianę (400 kroków) | w obu przypadkach pudełko mija słupki w z = 2 i z = 4 i staje dopiero na ścianie południowej ostatniej komórki: środek w x = 1,55 i z = 5,55 (pułapka 1) |

Ostatni wiersz zastąpił wcześniejszy przypadek, który przypinał zachowanie odwrotne (pudełko stające na słupku). Zmieniło się zachowanie, a nie tylko test: pudełka ścian dostały grubość słupków (pułapka 1).

Cztery dalsze przypadki z prawdziwym graczem są w `tests/PlayerTests.cpp` (`a wall stops the player`, `a player pressing into a wall slides along it and past the pillars`, `a player wandering through a closed maze never leaves it or enters a wall`, `noclip flies through walls`): omawia je [`../game/player.md`](../game/player.md), sekcja 5.

Użycie kul w regułach gry sprawdza `tests/RoundTests.cpp`, między innymi przypadki `the reach of the player is a sphere at the middle of the body`, `a crystal is collected from the middle of its cell, not from the next cell`, `a larger pickup radius reaches a crystal from further away`, `the round is won in the exit zone, but only while the gate is open` i `the player cannot reach the exit zone from in front of the closed gate`. Liczby z nich są w sekcji 5.10, a całość omawia [`../game/gameplay.md`](../game/gameplay.md).

Wyniki na Windowsie z 2026-10-05: cały program testowy (256 przypadków, 101232 asercje, w tym 19 przypadków z `ColliderTests.cpp`) przechodzi w Debug i Release. Po M5 było to 215 przypadków i 85098 asercji, a po pierwszej części M7 zgłoszone jest 269 przypadków i 102103 asercje, po drugiej 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751 (testy kolizji bez zmian; nowy plik `tests/ShadowTests.cpp` z czwartej części M7 czyta pudełka kolizji labiryntu domyślnego, 242 sztuki, ale sprawdza na nich mapę cieni, a nie kolizje: [`../renderer/shadows.md`](../renderer/shadows.md), sekcja 5.10). Build bez ostrzeżeń to zgłoszenie autora kodu. Kolizje na nierównym gruncie (sekcja 2.13) sprawdzają przypadki z `tests/TerrainTests.cpp`: `on uneven ground the walls, pillars and the gate are sunk until no gap shows` (pudełka idą za opuszczonymi pozycjami, nic nie rusza się w bok, gracz przy ścianie dzieli z jej pudełkiem ponad 0,1 m wysokości) i `the walls stop the player on uneven ground exactly as on flat ground`. Na macOS nic z M5 ani z M6 nie było jeszcze kompilowane ani uruchamiane: to pozycja na liście w [`../../guides/build-macos.md`](../../guides/build-macos.md).

### 5.8 Rysowanie pudełek i kul: `ColliderLines`

Klasa z programu `night_maze` ([`src/game/ColliderLines.hpp`](../../../src/game/ColliderLines.hpp), [`.cpp`](../../../src/game/ColliderLines.cpp)). Nie zmienia kolizji w żaden sposób: tylko pokazuje te same pudełka, na których liczy `moveAndSlide`, i te same kule, które porównuje `updateRound`.

```cpp
class ColliderLines {
public:
    /// Uploads the unit cube and the unit circle.
    ColliderLines();

    /// Draws every box in one colour. shader is the flat colour program (color.vert and
    /// color.frag): it must be in use, with uView and uProjection already set. The
    /// function sets uColor, and uModel for every box.
    void draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
              const glm::vec3& color) const;

    /// Draws every sphere in one colour, as three circles around its centre: one lying
    /// flat (in the XZ plane) and two standing upright (in the XY and the YZ plane).
    /// Three circles are enough to read the size and the place of a sphere, and they
    /// are the same picture from every side. shader is prepared as for draw.
    void drawSpheres(const gfx::Shader& shader, std::span<const scene::Sphere> spheres,
                     const glm::vec3& color) const;

private:
    gfx::Mesh m_unitCube;
    gfx::Mesh m_unitCircle;
    gfx::Mesh m_unitLine; // since M8, part 2: a line from (0, 0, 0) to (1, 0, 0)
};
```

Dwa pola: siatka sześcianu o boku 1 i siatka okręgu o promieniu 1. Klasa posiada więc obiekty OpenGL (VAO i dwa bufory wewnątrz każdej `gfx::Mesh`) i musi zostać zniszczona przed oknem. Kopiować się jej nie da, bo `gfx::Mesh` nie da się kopiować.

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
| `gfx::Vertex{.position = {...}}` | wypełniona jest tylko pozycja. Normalna, współrzędne tekstury i styczna zostają zerami (wartości domyślne struktury): linie ich nie potrzebują, a shader `color.vert` ich nie czyta |
| narożniki od 0 do 3 | dolna ściana (y = 0), po kolei dookoła |
| narożniki od 4 do 7 | górna ściana (y = 1), w tej samej kolejności, więc narożnik `n + 4` stoi nad narożnikiem `n` |
| `0, 1, 1, 2, 2, 3, 3, 0` | cztery krawędzie dolnej ściany: każda para to jeden odcinek |
| `0, 4, 1, 5, 2, 6, 3, 7` | cztery krawędzie pionowe |
| `EDGE_COUNT * INDICES_PER_LINE` | 24 indeksy: 12 krawędzi po 2 |

Sześcian sięga od `(0, 0, 0)` do `(1, 1, 1)`, a nie od -0,5 do 0,5, jak sięgała kostka z M1 (usunięta z programu w M5). Powód jest w funkcji `draw`: narożnik w początku układu sprawia, że przesunięcie do narożnika `min` pudełka wystarcza, bez liczenia środka.

Tutaj 8 wierzchołków wystarcza, choć kostka z M1 potrzebowała 24. Tam każdy narożnik miał trzy różne kolory (po jednym na ścianę), więc był trzema wierzchołkami. Z tego samego powodu modele ścian mają osobne wierzchołki dla każdej ściany bryły: różnią się normalną. Tu wierzchołek to sama pozycja, więc narożnik wspólny dla trzech krawędzi jest jednym wierzchołkiem.

```cpp
ColliderLines::ColliderLines()
    : m_unitCube(UNIT_CUBE_CORNERS, UNIT_CUBE_EDGES, GL_LINES),
      m_unitCircle(unitCirclePoints(), unitCircleLines(), GL_LINES) {}
```

**Linia (od M8, części 2).** Konstruktor ma trzecią siatkę: `m_unitLine`, dwa wierzchołki, `(0, 0, 0)` i `(1, 0, 0)`, jedna linia (`UNIT_LINE_ENDS`, `UNIT_LINE_INDICES = {0, 1}`). Funkcja `drawLine(shader, from, to, color)` rysuje ją między dwoma punktami, bez osobnej siatki na każdy odcinek: macierz modelu jest wpisana ręcznie, kolumna X to `to - from` (tam trafia koniec jednostkowej linii), kolumna początku to `from`, a kolumny Y i Z zostają z jednostkowej, bo linia nie ma w tych kierunkach rozmiaru. Kolor jest przeliczany z sRGB na liniowy, jak w `draw`. Używa jej promień wskazywania ([`picking.md`](picking.md)).

Trzeci argument konstruktora `gfx::Mesh` to rodzaj prymitywu. Tablice `std::array` same zamieniają się na `std::span`. Dane okręgu (funkcje `unitCirclePoints` i `unitCircleLines`) są omówione niżej, przy `drawSpheres`.

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
    // color is an sRGB value, a colour named for the screen. The scene buffer holds
    // linear colours, so it is converted here. The composite pass treats the lines like
    // the rest of the scene (fog, bloom, exposure, tone mapping, vignette), so the screen
    // shows the colour exactly as given only without those.
    shader.setVec3(COLOR_UNIFORM, gfx::srgbToLinear(color));

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
| `shader.setVec3(COLOR_UNIFORM, gfx::srgbToLinear(color));` | jeden kolor dla całej listy pudełek, ustawiany raz przed pętlą. Od pierwszej części M7 przeliczany z sRGB na liniowy (`gfx/ColorSpace.hpp`): to jedno z miejsc, w których kolor wpisany w kodzie jest zamieniany dokładnie raz. Komentarz mówił do trzeciej części M7 "the composite pass shows it as given", co było ścisłe tylko bez mapowania tonów i przy ekspozycji 1, a od trzeciej części M7 także bez mgły i winiety (sekcja 4.2). W czwartej części M7 (2026-10-05) komentarz poprawiono i dziś mówi to sam: przebieg składający traktuje linie jak resztę sceny (mgła, bloom, ekspozycja, mapowanie tonów, winieta), więc ekran pokazuje kolor dokładnie taki, jak podany, tylko bez nich. Cienie księżyca linii nie dotyczą: program `color` nie dołącza `common/shadows.glsl`, a linie nie są rysowane do mapy cieni |
| `transform.scale = box.max - box.min + glm::vec3{2.0F * LINE_MARGIN};` | rozmiar pudełka na każdej osi, powiększony o margines z obu stron. Sześcian o boku 1 pomnożony przez rozmiar staje się prostopadłościanem tego rozmiaru |
| `transform.position = box.min - glm::vec3{LINE_MARGIN};` | narożnik `(0, 0, 0)` sześcianu trafia w narożnik `min` pudełka, cofnięty o margines. `glm::vec3{LINE_MARGIN}` to wektor z tą samą wartością w trzech składowych |
| `transform.matrix()` | `translate * rotate * scale`: wierzchołek jest najpierw skalowany, potem przesuwany ([`transforms.md`](transforms.md), sekcja 5). Obrotu nie ma, bo AABB się nie obraca |
| `m_unitCube.draw();` | `glDrawElements(GL_LINES, 24, ...)` |

Przykład na liczbach: pudełko ściany wzdłuż X o środku na linii z = 4 ma `min = (2, 0, 3,85)` i `max = (4, 3, 4,15)`. Skala wychodzi `(2,02, 3,02, 0,32)`, a pozycja `(1,99, -0,01, 3,84)`. Narożnik `(1, 1, 1)` sześcianu ląduje w `(4,01, 3,01, 4,16)`: centymetr poza prawdziwym `max`.

Inaczej niż macierze ścian labiryntu, te macierze **są** liczone w każdej klatce, w pętli rysowania. To świadome uproszczenie: rysowanie brył jest narzędziem diagnostycznym, domyślnie wyłączonym, a pudełko gracza i tak zmienia się co klatkę.

**Dane okręgu.**

```cpp
// A circle is drawn as this many straight pieces. 32 look round at the size of a pickup
// sphere on the screen.
constexpr std::size_t CIRCLE_SEGMENTS = 32;
```

```cpp
std::array<gfx::Vertex, CIRCLE_SEGMENTS> unitCirclePoints() {
    std::array<gfx::Vertex, CIRCLE_SEGMENTS> points{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const float angle =
            glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(CIRCLE_SEGMENTS);
        points[i].position = {std::cos(angle), std::sin(angle), 0.0F};
    }
    return points;
}

// Every two indices are one line (GL_LINES): each point is joined to the next one, and
// the last point back to the first (that is what the remainder does).
std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> unitCircleLines() {
    std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> indices{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        indices[i * INDICES_PER_LINE] = static_cast<std::uint32_t>(i);
        indices[i * INDICES_PER_LINE + 1] = static_cast<std::uint32_t>((i + 1) % CIRCLE_SEGMENTS);
    }
    return indices;
}
```

| Fragment | Znaczenie |
|---|---|
| `CIRCLE_SEGMENTS = 32` | okrąg jest łamaną z 32 prostych odcinków (sekcja 3) |
| `glm::two_pi<float>()` razy `i`, dzielone przez `CIRCLE_SEGMENTS` | kąt punktu numer `i` w radianach: pełny obrót (2 pi) podzielony na 32 równe części. Punkt 0 ma kąt 0, punkt 8 ma 90 stopni, punkt 16 ma 180 |
| `{std::cos(angle), std::sin(angle), 0.0F}` | punkt na okręgu o promieniu 1 wokół początku układu, w płaszczyźnie XY (z = 0). Punkt 0 to `(1, 0, 0)`, punkt 8 to `(0, 1, 0)` |
| `points{}` i brak normalnej, uv, stycznej | tak jak przy sześcianie: wypełniona jest tylko pozycja, reszta zostaje zerami |
| pierwsza linia w pętli `unitCircleLines` | początek odcinka numer `i`: punkt `i` |
| `(i + 1) % CIRCLE_SEGMENTS` | jego koniec: następny punkt. Reszta z dzielenia zawija ostatni odcinek do punktu 0 (`32 % 32 = 0`), więc łamana się zamyka |
| `CIRCLE_SEGMENTS * INDICES_PER_LINE` | 64 indeksy: 32 odcinki po 2 |

Okrąg jest liczony funkcją, a sześcian stoi w kodzie jako tablica stałych. Powód podaje komentarz nad `unitCirclePoints` w pliku (tutaj pominięty): 32 sinusy i cosinusy łatwiej policzyć, niż wpisać. Funkcje są wołane raz, w konstruktorze `ColliderLines`.

```cpp
// The unit circle lies in the XY plane. Turned by a quarter around the X axis it lies
// flat in the XZ plane, turned by a quarter around the Y axis it stands in the YZ plane.
constexpr float QUARTER_TURN_DEGREES = 90.0F;
constexpr std::array<glm::vec3, 3> CIRCLE_ROTATIONS = {
    glm::vec3{0.0F, 0.0F, 0.0F},
    glm::vec3{QUARTER_TURN_DEGREES, 0.0F, 0.0F},
    glm::vec3{0.0F, QUARTER_TURN_DEGREES, 0.0F},
};
```

Trzy zestawy kątów dla pola `rotationDegrees` struktury `Transform` (kąty wokół osi X, Y i Z, w stopniach):

| Element `CIRCLE_ROTATIONS` | Obrót | Gdzie leży okrąg |
|---|---|---|
| `(0, 0, 0)` | żaden | stoi w płaszczyźnie XY |
| `(90, 0, 0)` | ćwierć obrotu wokół osi X: oś Y ląduje na osi Z | leży płasko, w płaszczyźnie XZ (równolegle do podłogi) |
| `(0, 90, 0)` | ćwierć obrotu wokół osi Y: oś X ląduje na osi Z | stoi w płaszczyźnie YZ |

**`drawSpheres`.**

```cpp
void ColliderLines::drawSpheres(const gfx::Shader& shader, std::span<const scene::Sphere> spheres,
                                const glm::vec3& color) const {
    // Converted to a linear colour, as in draw.
    shader.setVec3(COLOR_UNIFORM, gfx::srgbToLinear(color));

    for (const scene::Sphere& sphere : spheres) {
        // The unit circle has a radius of 1 around the origin, so scaling it by the
        // radius and moving it to the centre lands it on the sphere.
        scene::Transform transform;
        transform.position = sphere.center;
        transform.scale = glm::vec3{sphere.radius};

        for (const glm::vec3& rotation : CIRCLE_ROTATIONS) {
            transform.rotationDegrees = rotation;
            shader.setMat4(MODEL_UNIFORM, transform.matrix());
            m_unitCircle.draw();
        }
    }
}
```

| Linia | Znaczenie |
|---|---|
| `shader.setVec3(COLOR_UNIFORM, color);` | jeden kolor dla całej listy kul, tak jak w `draw` |
| `transform.position = sphere.center;` | okrąg jednostkowy ma środek w początku układu, więc samo przesunięcie do środka kuli stawia go na miejscu. Sześcian miał w początku układu narożnik i dlatego jechał do `min` |
| `transform.scale = glm::vec3{sphere.radius};` | ta sama skala na trzech osiach: okrąg o promieniu 1 staje się okręgiem o promieniu kuli. Skala jednorodna nie zmienia kształtu, więc okrąg zostaje okręgiem także po obrocie |
| `for (const glm::vec3& rotation : CIRCLE_ROTATIONS)` | trzy obroty pętli na kulę: trzy okręgi z jednej siatki |
| `transform.rotationDegrees = rotation;` | zmienia się tylko obrót. Pozycja i skala zostają z linii wyżej |
| `transform.matrix()` | `translate * rotate * scale`: punkt okręgu jest najpierw skalowany do promienia, potem obracany do swojej płaszczyzny, na końcu przesuwany do środka kuli ([`transforms.md`](transforms.md), sekcja 5) |
| `m_unitCircle.draw();` | `glDrawElements(GL_LINES, 64, ...)` |

Przykład na liczbach: kula zasięgu gracza stojącego na starcie, w środku komórki `(0, 0)`, ma środek `(1, 0,9, 1)` i promień 0,3. W okręgu bez obrotu punkt 0, czyli `(1, 0, 0)`, po skali to `(0,3, 0, 0)`, a po przesunięciu `(1,3, 0,9, 1)`. Punkt 8, czyli `(0, 1, 0)`, ląduje w `(1, 1,2, 1)`: 0,3 m nad środkiem.

Marginesu `LINE_MARGIN` tu nie ma: okręgi są rysowane w prawdziwym rozmiarze (sekcja 3). Pusta lista kul jest poprawna: funkcja ustawia kolor i nic nie rysuje.

**Kto woła `draw` i `drawSpheres`.** `NightMazeApp::onRender`, na końcu klatki i tylko wtedy, gdy przełącznik jest włączony:

```cpp
    drawMaze(view, projection);
    if (m_drawColliders) {
        drawColliderLines(view, projection);
    }
```

`drawMaze` rysuje labirynt, a razem z nim kryształy i bramę. Między tymi dwiema liniami stoi dziś jeszcze `drawGrass`. Miejsce linii w klatce od pierwszej części M7: po terenie, labiryncie i trawie, przed niebem, wszystko do framebuffera HDR sceny (`m_postProcess.beginScene` na początku klatki). Linie są więc częścią obrazu, który czyta przebieg składający, i widać je także na podglądzie koloru w panelu Framebuffers. Kostki z M1, którą do M4 rysowało osobne wywołanie między tymi dwiema liniami, w programie już nie ma.

```cpp
// Colours of the collision lines (red, green, blue): the boxes of the maze in yellow,
// the box and the reach of the player in green, the box of the gate in orange, the
// pickup spheres of the crystals in cyan and the exit zone in magenta.
constexpr glm::vec3 MAZE_COLLIDER_COLOR{1.0F, 0.85F, 0.1F};
constexpr glm::vec3 PLAYER_COLLIDER_COLOR{0.2F, 1.0F, 0.4F};
constexpr glm::vec3 GATE_COLLIDER_COLOR{1.0F, 0.45F, 0.1F};
constexpr glm::vec3 PICKUP_COLLIDER_COLOR{0.2F, 0.9F, 1.0F};
constexpr glm::vec3 EXIT_ZONE_COLOR{1.0F, 0.3F, 0.9F};
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
    //
    // (Since M8, part 2) The boxes of the maze come from the obstacle list of the round,
    // so a wall that a lever has opened is not drawn. While the gate blocks, its box is
    // the last one of that list: it is left out here and drawn in its own colour below.
    const std::size_t gateBoxes = gateBlocks(m_mazeWorld, m_round) ? 1 : 0;
    const std::span<const scene::Aabb> mazeBoxes =
        std::span<const scene::Aabb>(m_obstacles).first(m_obstacles.size() - gateBoxes);
    m_colliderLines.draw(m_colorShader, mazeBoxes, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);

    // The gate, while it is an obstacle, and the zone behind it that wins the round.
    if (gateBlocks(m_mazeWorld, m_round)) {
        m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.gateBox, 1),
                             GATE_COLLIDER_COLOR);
    }
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.exitZone, 1),
                         EXIT_ZONE_COLOR);

    // The spheres of the pickup test: the reach of the player and, around every crystal
    // that is still there, the sphere the reach has to overlap. They stay on the
    // resting place of the crystal while the crystal itself bobs.
    const scene::Sphere reach = playerReach(m_player.position);
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&reach, 1),
                                PLAYER_COLLIDER_COLOR);
    std::vector<scene::Sphere> pickupSpheres;
    for (const RoundCrystal& crystal : m_round.crystals) {
        if (!crystal.collected) {
            pickupSpheres.push_back(
                {.center = crystalCenter(crystal.restPosition), .radius = m_gameplay.pickupRadius});
        }
    }
    m_colliderLines.drawSpheres(m_colorShader, pickupSpheres, PICKUP_COLLIDER_COLOR);
}
```

| Linia | Znaczenie |
|---|---|
| `if (!m_colorShader.isValid()) { return; }` | bez programu nie ma czym rysować. Błąd wczytania shadera był w logu przy starcie, a reszta klatki rysuje się normalnie |
| `use()`, potem `uView` i `uProjection` | te same macierze co dla labiryntu: linie są widziane z tego samego oka |
| `draw(m_colorShader, mazeBoxes, MAZE_COLLIDER_COLOR)` | żółte: ściany i słupki. Do M8, części 2 było to `m_mazeWorld.colliders`, teraz `m_obstacles` bez ostatniego pudełka, gdy brama blokuje (jest rysowana osobno na pomarańczowo). Dlatego ściana otwarta dźwignią **traci swoje żółte pudełko** razem z przeszkodą. Widok `std::span` na początek listy: `first(rozmiar - pudełka bramy)` |
| `const scene::Aabb playerBox = m_player.box();` | pudełko gracza w pozycji symulacji, na zielono |
| `std::span<const scene::Aabb>(&playerBox, 1)` | widok na jeden element: wskaźnik i liczba 1. Dzięki temu `draw` ma jedną wersję, dla listy |
| `if (gateBlocks(m_mazeWorld, m_round))` | pomarańczowe pudełko bramy jest rysowane tylko wtedy, gdy brama jest na liście przeszkód. Ten sam warunek stoi w `roundObstacles`, więc **żółte i pomarańczowe linie razem to dokładnie lista `m_obstacles`**, którą dostaje `Player::update` |
| ostatnie wywołanie `draw`, z `m_mazeWorld.exitZone` i `EXIT_ZONE_COLOR` | strefa wyjścia na magentę, zawsze. To pudełko **nie** jest przeszkodą: gracz w nie wchodzi, a test kuli z pudełkiem kończy rundę (sekcja 5.10) |
| `const scene::Sphere reach = playerReach(m_player.position);` | kula zasięgu gracza, tą samą funkcją, której używa `updateRound`. Zielona jak pudełko gracza: obie bryły należą do gracza |
| `if (!crystal.collected)` | kula zbierania tylko dla kryształów, które jeszcze wiszą |
| `crystalCenter(crystal.restPosition)` | środek kuli to środek kryształu **w miejscu spoczynku**. Rysowany model kołysze się w górę i w dół o najwyżej 8 cm (`CRYSTAL_BOB_AMPLITUDE`), a kula stoi: tak samo liczy ją test w `collectCrystals` |
| `m_gameplay.pickupRadius` | promień z ustawień rozgrywki (domyślnie 0,6 m). Suwak `Pickup radius` w panelu Gameplay zmienia naraz test i rysunek |
| `drawSpheres(m_colorShader, pickupSpheres, PICKUP_COLLIDER_COLOR)` | wszystkie kule kryształów jednym wywołaniem, w kolorze cyjan |

Lista `pickupSpheres` powstaje od nowa w każdej klatce, w której rysowanie jest włączone. Kule nie są nigdzie przechowywane: i reguły, i rysunek liczą je za każdym razem z pozycji spoczynku kryształu i z promienia w ustawieniach, więc nie mogą się rozjechać.

Linie są rysowane **po** labiryncie, kryształach i bramie, z włączonym testem głębi. Kolejność nie wpływa na to, co jest zasłonięte (o tym decyduje głębia), ale gwarantuje, że głębie ścian są już w buforze, gdy linie są z nimi porównywane.

Na zrzucie ekranu z Windowsa z M3 (widok z góry, tryb noclip) żółte pudełka leżały na ścianach i słupkach. Linii z M5 (pomarańczowych, cyjanowych, magentowych i zielonej kuli) nie oceniałem na żadnym zrzucie, a przełącznika nikt jeszcze nie kliknął ręcznie.

### 5.9 Kule: `Sphere`, dwa `overlaps` i `closestPoint`

Kod z M5, w tych samych dwóch plikach co pudełka. W nagłówku:

```cpp
/// A sphere: every point within radius of center. The second kind of collision shape,
/// used for things that are picked up or entered and never block the way: walking close
/// enough is all that counts, from whatever side.
///
/// Plain data, like Aabb.
struct Sphere {
    /// The middle of the sphere, in world space.
    glm::vec3 center{0.0F};

    /// Distance from the middle to the surface, in metres. Must not be negative.
    float radius = 0.0F;
};
```

| Element | Znaczenie |
|---|---|
| `glm::vec3 center{0.0F};` | środek kuli we współrzędnych świata. `{0.0F}` zeruje trzy składowe, tak jak w `Aabb` |
| `float radius = 0.0F;` | promień w metrach. Wartość domyślna 0 daje punkt |
| "Must not be negative" | warunek, którego kod **nie sprawdza**, tak jak nie sprawdza `min <= max` w pudełku (pułapka 18) |
| brak konstruktora i funkcji | struktura jest agregatem: tworzy się ją inicjalizatorem desygnowanym, `scene::Sphere{.center = {0.0F, 1.0F, 0.0F}, .radius = 0.5F}`. Nazwy pól chronią przed pomyleniem kolejności |

Kula nie ma odpowiednika `fromCenter`, bo środek i promień to już jej naturalny opis.

**Kula z kulą.**

```cpp
bool overlaps(const Sphere& a, const Sphere& b) {
    // The squares are compared instead of the distances themselves: the square root that
    // a distance needs is the expensive part, and for numbers that are not negative
    // "smaller" means the same before and after squaring. dot(v, v) is the squared
    // length of v.
    const glm::vec3 offset = b.center - a.center;
    const float reach = a.radius + b.radius;
    return glm::dot(offset, offset) < reach * reach;
}
```

| Linia | Znaczenie |
|---|---|
| `const glm::vec3 offset = b.center - a.center;` | wektor od środka pierwszej kuli do środka drugiej. Odejmowanie działa składowa po składowej |
| `const float reach = a.radius + b.radius;` | suma promieni: tak daleko od siebie mogą być środki, żeby kule się jeszcze stykały |
| `glm::dot(offset, offset)` | kwadrat odległości środków: `x * x + y * y + z * z` (sekcja 2.10) |
| `reach * reach` | kwadrat sumy promieni. Mnożenie zamiast funkcji potęgi: jest tańsze i dokładne |
| `<` | ostra nierówność: przy równości kule tylko się stykają i wynik to fałsz (sekcja 2.12) |

Funkcja nie woła `std::sqrt` ani `glm::length`. Jest symetryczna: zamiana `a` z `b` zmienia tylko znak `offset`, a kwadrat długości znaku nie widzi.

**Najbliższy punkt pudełka.**

```cpp
glm::vec3 closestPoint(const Aabb& box, const glm::vec3& point) {
    // Axis by axis: a coordinate between min and max stays, one outside is moved to the
    // nearer of the two. glm::clamp does that for all three components at once.
    return glm::clamp(point, box.min, box.max);
}
```

| Linia | Znaczenie |
|---|---|
| `glm::clamp(point, box.min, box.max)` | wersja `clamp` dla wektorów: przycina `point.x` do przedziału od `box.min.x` do `box.max.x` i tak samo y i z. Wynik to nowy wektor ([`../../libraries/glm.md`](../../libraries/glm.md)) |
| wynik dla punktu w pudełku | ten sam punkt: żadna współrzędna nie wychodzi poza swój przedział |
| wynik dla punktu poza pudełkiem | punkt na ścianie, krawędzi albo w narożniku pudełka, zależnie od tego, ile współrzędnych zostało przyciętych (jedna, dwie albo trzy) |

Funkcja jest publiczna, choć w programie woła ją tylko test kuli z pudełkiem. Dzięki temu ma własny przypadek testowy na liczbach, a test kuli z pudełkiem sprawdza już tylko porównanie.

**Kula z pudełkiem.**

```cpp
bool overlaps(const Sphere& sphere, const Aabb& box) {
    // The sphere reaches the box exactly when it reaches the point of the box nearest to
    // its centre. A centre inside the box is its own nearest point: the distance is 0,
    // and any radius above 0 overlaps.
    const glm::vec3 offset = closestPoint(box, sphere.center) - sphere.center;
    return glm::dot(offset, offset) < sphere.radius * sphere.radius;
}
```

| Linia | Znaczenie |
|---|---|
| `closestPoint(box, sphere.center)` | punkt pudełka najbliższy środkowi kuli |
| `- sphere.center` | wektor od środka kuli do tego punktu. Dla środka w pudełku to wektor zerowy |
| `glm::dot(offset, offset) < sphere.radius * sphere.radius` | kwadrat odległości przeciw kwadratowi promienia, znów bez pierwiastka i znów z ostrą nierównością |

Kolejność argumentów jest jedna: najpierw kula, potem pudełko. Przeciążenia `overlaps(box, sphere)` nie ma, więc wywołanie w odwrotnej kolejności się nie skompiluje.

Trzy rzeczy, które trzeba umieć powiedzieć o tym kodzie:

- **To samo wykrywanie, inne bryły.** Wszystkie trzy `overlaps` odpowiadają `bool` i wszystkie traktują styk jako brak nakładania.
- **Żadnej reakcji.** Żadna z funkcji dla kul nie zwraca przesunięcia ani kierunku wypchnięcia. `moveAndSlide` ich nie woła i nie przyjmuje kul.
- **Żadnej tolerancji.** `CONTACT_TOLERANCE` nie występuje w tych funkcjach (sekcja 2.7).

### 5.10 Kto używa kul: zbieranie kryształów i strefa wyjścia

Oba zastosowania są w [`src/game/Round.cpp`](../../../src/game/Round.cpp), w funkcji `updateRound`, wołanej raz na krok symulacji z pozycją gracza **po** ruchu. Reguły rundy w całości (bateria, brama, wygrana) opisuje [`../game/gameplay.md`](../game/gameplay.md), sekcje 2 i 5. Tutaj tylko to, co dotyczy brył.

**Kula zasięgu gracza.**

```cpp
constexpr float PLAYER_REACH_HEIGHT = 0.9F;
constexpr float PLAYER_REACH_RADIUS = 0.3F;
```

```cpp
scene::Sphere playerReach(const glm::vec3& feetPosition) {
    return {.center = feetPosition + glm::vec3{0.0F, PLAYER_REACH_HEIGHT, 0.0F},
            .radius = PLAYER_REACH_RADIUS};
}
```

Gracz ma więc dwie bryły. Pudełko `Player::box()` (0,6 na 1,8 na 0,6 m) służy do ruchu wśród ścian. Kula zasięgu ma środek 0,9 m nad stopami, czyli w połowie wysokości ciała, i promień 0,3 m, czyli połowę jego szerokości. Służy do pytań "czy gracz czegoś dosięga".

**Zbieranie: kula z kulą.**

```cpp
void collectCrystals(Round& round, const GameplaySettings& settings, const scene::Sphere& reach) {
    for (RoundCrystal& crystal : round.crystals) {
        if (crystal.collected) {
            continue;
        }
        const scene::Sphere pickup{.center = crystalCenter(crystal.restPosition),
                                   .radius = settings.pickupRadius};
        if (scene::overlaps(reach, pickup)) {
            crystal.collected = true;
            ++round.collectedCount;
            round.battery = std::min(round.battery + settings.batteryPerCrystal, 1.0F);
        }
    }
}
```

| Linia | Znaczenie |
|---|---|
| `if (crystal.collected) { continue; }` | zebrany kryształ nie ma już kuli: nie da się go zebrać drugi raz |
| `crystalCenter(crystal.restPosition)` | środek kuli zbierania: środek kryształu w miejscu spoczynku, czyli 1,15 m nad gruntem w środku komórki (podstawa na 0,9 m plus połowa wysokości 0,5 m). Kula nie kołysze się razem z modelem, więc droga do przejścia nie zależy od chwili |
| `settings.pickupRadius` | promień kuli zbierania, domyślnie 0,6 m |
| `scene::overlaps(reach, pickup)` | test kuli z kulą z sekcji 5.9 |
| trzy linie w środku `if` | skutki dla rundy: flaga zebrania, licznik i ładowanie baterii. To już reguły gry, nie kolizje |

Liczby dla ustawień domyślnych. Suma promieni to `0,3 + 0,6 = 0,9` m, kwadrat 0,81. Środek kuli kryształu wisi `1,15 - 0,9 = 0,25` m nad środkiem kuli gracza, co zabiera z kwadratu `0,0625`. W poziomie zostaje pierwiastek z `0,7475`, czyli około 0,86 m: z takiej odległości od środka komórki gracz zbiera kryształ. Dwa wnioski, oba przypięte testem `a crystal is collected from the middle of its cell, not from the next cell`:

- gracz przytulony do ściany w rogu komórki jest najwyżej 0,55 m od jej środka na każdej osi (1 m do linii ściany minus 0,15 m połowy pudełka ściany i 0,3 m połowy ciała), czyli około 0,78 m po przekątnej. Wzdłuż ścian komórki i w jej rogach kryształ zbiera się więc zawsze. Od strony otwartego boku ściany nie ma: gracz wchodzący do komórki zbiera kryształ dopiero wtedy, gdy jego środek jest bliżej niż około 0,86 m od środka komórki, czyli około 14 cm za jej granicą,
- granica komórki jest 1 m od jej środka, a gracz stojący za ścianą ma środek co najmniej 1,45 m dalej. Kryształu nie da się zebrać, stojąc środkiem ciała w sąsiedniej komórce, ani przez ścianę. Test kul nie wie nic o ścianach: wynika to wyłącznie z odległości.

**Wyjście: kula z pudełkiem.**

```cpp
        if (round.gateOpen && scene::overlaps(reach, world.exitZone)) {
            round.state = RoundState::Won;
        }
```

`world.exitZone` to `Aabb` z funkcji `game::exitZone`: pudełko 1 na 1 m w środku komórki wyjścia (2 na 2 m), wysokie jak ściany (3 m). To pudełko nigdy nie trafia na listę przeszkód. Jest **strefą** (trigger volume): wejście w nią coś uruchamia, ale niczego nie zatrzymuje.

Liczby. Strefa zaczyna się 0,5 m od środka komórki, a zasięg gracza ma 0,3 m, więc przy podejściu prosto od bramy rundę wygrywa gracz, którego środek jest bliżej niż 0,8 m od środka komórki. Test `the round is won in the exit zone, but only while the gate is open` sprawdza punkt 0,85 m od środka: brakuje 5 cm i runda trwa. Przed zamkniętą bramą gracz staje 1,45 m od środka komórki (1 m do linii bramy, 0,15 m połowy pudełka bramy, 0,3 m połowy ciała): do strefy zostaje 0,95 m, ponad trzy razy więcej niż zasięg. Sprawdza to test `the player cannot reach the exit zone from in front of the closed gate`.

Warunek `round.gateOpen` stoi przed testem brył z powodu trybu noclip. Z kolizjami gracz nie dostanie się do strefy, dopóki brama jest zamknięta, ale w noclip przelatuje przez nią, a to nie może wygrywać rundy. Kolizje pilnują więc drogi, a reguła pilnuje wyniku.

**Co z tego wynika.**

- Testy kul działają także w trybie noclip. `updateRound` dostaje pozycję gracza niezależnie od tego, jak się poruszył, więc przelatując przez kryształ, gracz go zbiera (pułapka 20).
- Brama jest pudełkiem na liście przeszkód (sekcja 5.6), a strefa za nią pudełkiem do testu z kulą. To dwie różne role tej samej struktury `Aabb`.
- Te same dwie kule i to samo pudełko strefy rysuje `drawColliderLines` (sekcja 5.8), z tych samych funkcji i z tych samych liczb.

## 6. Panel ImGui

Panel **Collision** jest pokazem tematu 14. Kod: [`src/debug/panels/CollisionPanel.cpp`](../../../src/debug/panels/CollisionPanel.cpp). Jak panel jest podpięty do `DebugUI`, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.

PRD (sekcja 10) opisuje panel Collision jako "Debug draw AABB i sfer, wynik ostatniego raycasta", a tryb noclip wymienia przy panelu Camera. W programie jest rysowanie pudełek AABB i, od M5, kul. Linia PRD o wyniku ostatniego raycasta jest od M8, części 2 **spełniona**: panel ma sekcję `Last picking ray`, a promień wskazywania (matematyka z `src/scene/Raycast.*`, [`picking.md`](picking.md)) jest budowany w każdej klatce. Panel pokazuje, przez co poszedł promień, jego początek i kierunek, trafienie i działanie klawisza E, a dwa pola wyboru rysują pudełka wskazywania i promień w scenie. Przełącznik noclip trafił do tego panelu, bo znaczy "wyłącz kolizje", a panel Camera pokazuje tylko bieżący tryb ([`camera-controls.md`](camera-controls.md), sekcja 6).

### 6.1 Kod panelu

```cpp
void drawCollisionPanel(const game::MazeWorld& world, const game::Round& round,
                        game::Player& player, bool& drawColliders, const game::PickState& pick,
                        game::PickDebugSettings& pickDebug) {
    // First run only: the bottom edge of the window, right of the left column (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(COLLISION_PLACEMENT);
    if (ImGui::Begin("Collision")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Draw collision shapes", &drawColliders);
        ImGui::TextWrapped("Yellow: walls, pillars. Green: player. Orange: gate. "
                           "Cyan: crystal pickup. Magenta: exit zone.");

        // The same switch as the N key. In noclip mode the boxes below are ignored.
        ImGui::Checkbox("Noclip (key N)", &player.noclip);

        ImGui::Separator();
        // world.colliders holds the box of every wall first and the box of every pillar
        // after them, so the two counts are the sizes of the lists they were made from.
        // The gate is one more box while it is closed (game::roundObstacles).
        const int gateBoxes = game::gateBlocks(world, round) ? 1 : 0;
        ImGui::Text("Boxes: %d walls, %d pillars, %d gate", static_cast<int>(world.walls.size()),
                    static_cast<int>(world.pillars.size()), gateBoxes);
        // One pickup sphere around every crystal that is not collected yet.
        ImGui::Text("All boxes: %d, pickup spheres: %d",
                    static_cast<int>(world.colliders.size()) + gateBoxes,
                    static_cast<int>(round.crystals.size()) - round.collectedCount);
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
| `const game::Round& round` | stan rundy tylko do odczytu, nowy parametr w M5: z niego panel wie, czy brama jeszcze blokuje i ile kryształów zostało |
| `game::Player& player` | gracz bez `const`: pole wyboru pisze do `player.noclip` |
| `bool& drawColliders` | referencja do pola `m_drawColliders` aplikacji. Panel tylko ustawia flagę, a rysuje `NightMazeApp::onRender` w następnej klatce |
| `placePanelOnFirstUse(COLLISION_PLACEMENT)` | miejsce i rozmiar przy pierwszym uruchomieniu: dolna krawędź okna, na prawo od lewej kolumny paneli. Stała leży w [`PanelLayout.hpp`](../../../src/debug/PanelLayout.hpp), a funkcja ustawia pozycję i rozmiar z warunkiem `ImGuiCond_FirstUseEver`. Wpis w `imgui.ini` ma pierwszeństwo ([`../debug-ui.md`](../debug-ui.md), sekcja 5.7). Plik panelu nie ma własnych stałych, więc nie ma w nim anonimowej przestrzeni nazw |
| `ImGui::Checkbox("Draw collision shapes", &drawColliders)` | pole wyboru czyta i zapisuje `bool` przez wskaźnik. Do M4 napis brzmiał "Draw collision boxes": zmienił się, bo przełącznik włącza teraz także kule |
| `ImGui::TextWrapped("Yellow: ... Magenta: exit zone.")` | legenda pięciu kolorów (sekcja 4.2). Dwa sąsiednie literały napisów kompilator skleja w jeden |
| `ImGui::Checkbox("Noclip (key N)", &player.noclip)` | to samo pole, które przełącza klawisz N. Dwa sposoby zmiany jednej zmiennej, więc nie mogą się rozjechać |
| `game::gateBlocks(world, round) ? 1 : 0` | 1, dopóki brama jest przeszkodą, potem 0. To ten sam warunek, którego używają `roundObstacles` i rysowanie pomarańczowego pudełka |
| `world.walls.size()`, `world.pillars.size()` | liczby ścian i słupków. `mazeColliders` dodaje jedno pudełko na ścianę i jedno na słupek |
| `world.colliders.size() + gateBoxes` | `All boxes`: tyle pudełek ma lista `m_obstacles`, którą sprawdza każdy krok chodzenia. Panel nie czyta samej listy, tylko liczy ją tak samo, jak powstaje |
| `round.crystals.size() - round.collectedCount` | liczba kul zbierania: jedna na każdy kryształ, który jeszcze wisi |
| `static_cast<int>(...)` | `%d` oczekuje `int`, a `size()` zwraca `std::size_t` |
| `game::WALL_COLLISION_THICKNESS`, `game::WALL_VISUAL_THICKNESS` | 0,30 i 0,20: panel sam mówi, że pudełko ściany jest grubsze niż widoczna ściana (pułapka 1) |
| `player.box()` | pudełko liczone w każdej klatce z pozycji gracza, tą samą funkcją, której używa ruch |

Czego panel **nie** liczy: strefy wyjścia (to pudełko, ale nie przeszkoda, więc nie należy do `All boxes`) i kuli zasięgu gracza (zawsze jedna). Obie są tylko rysowane.

### 6.2 Kontrolki i czego uczą

| Kontrolka | Co zmienia albo pokazuje | Czego uczy |
|---|---|---|
| `Draw collision shapes` | `m_drawColliders` | bryła otaczająca jest prostsza niż model: żółte pudełko ściany to sześć płaszczyzn zamiast 30 trójkątów, a kula kryształu to środek i promień |
| legenda kolorów | odczyt | każda bryła ma swoją rolę: przeszkoda stała (żółty), przeszkoda do czasu (pomarańczowy), gracz (zielony), coś do zebrania (cyjan), strefa (magenta) |
| `Noclip (key N)` | `player.noclip` | kolizje ruchu są osobnym krokiem, który można pominąć: w trybie noclip lista pudełek nie jest czytana. Testy kul działają dalej |
| `Boxes: ... walls (... opened by levers), ... pillars, ... gate` | odczyt | z czego składa się lista przeszkód. Dla labiryntu startowego na początku rundy: `Boxes: 121 walls (0 opened by levers), 121 pillars, 1 gate`. Pociągnięta dźwignia zmniejsza pierwszą liczbę o 1 i zwiększa drugą o 1 (`pulledLeverCount`) |
| sekcja `Last picking ray` | odczyt | `Ray through: the middle of the picture` albo `the cursor`, `origin`, `direction` (jednostkowy), `Hit: lever 0 at 0.72 m` albo `Hit: nothing within 2.5 m`, `Key E: pull the lever` (też `read the note`, `close the note card`, `nothing`). Gdy promienia nie ma: `Ray: none (cursor over a panel or outside)`. Pokazuje promień **ostatniej klatki**: początek jest w oku (`rayFromEye`), więc wysokość `y` to wysokość oczu gracza |
| `Draw pick boxes and ray` | pole wyboru (`pickDebug.drawShapes`) | rysuje pudełka wskazywania dźwigni (czerwone) i kartek (białe), pudełko trafione (zielone) i promień (zielony po trafieniu, szary bez), z małą kulą na końcu. Z oka promień jest jednym punktem |
| `Freeze the drawn ray` | pole wyboru (`pickDebug.freezeRay`) | rysowany promień zostaje tam, gdzie był, a wskazywanie toczy się dalej: dopiero zatrzymany promień da się obejrzeć z boku (chodzi o kopię `m_shownPick`, nie o sam wynik, którego używa gra) |
| `All boxes: ..., pickup spheres: ...` | odczyt | (po M8, części 2 pierwsza liczba maleje też o każdą ścianę otwartą dźwignią) ile przeszkód sprawdza każdy krok i ile kul sprawdza każdy krok rundy. Na początku: `All boxes: 243, pickup spheres: 13`. Po otwarciu bramy pierwsza liczba spada do 242, a druga maleje z każdym kryształem |
| `Wall box: 0.30 m thick (the visible wall: 0.20 m)` | odczyt | bryła kolizji nie musi mieć wymiarów modelu |
| `Player box`, `min`, `max` | odczyt | pudełko jest liczone z pozycji: na starcie `min: 0.70, 0.00, 0.70` i `max: 1.30, 1.80, 1.30` |

Promień kul zbierania zmienia suwak `Pickup radius` w panelu Gameplay ([`../game/gameplay.md`](../game/gameplay.md), sekcja 6): cyjanowe okręgi rosną i maleją razem z nim.

### 6.3 Scenariusz pokazu na obronie

Kroki nie były jeszcze wykonane ręcznie. Opisują to, co wynika z kodu i z testów, a krok 2 w części o żółtych pudełkach także ze zrzutu ekranu z M3.

1. **Liczby.** Otwieram panel Collision: 121 ścian, 121 słupków, 1 brama, razem 243 pudełka i 13 kul zbierania. Mówię, że każdy krok chodzenia sprawdza całą listę pudełek na dwóch osiach, a każdy krok rundy wszystkie kule, i że przy tej skali nie potrzeba struktury przyspieszającej.
2. **Pudełka.** Włączam `Draw collision shapes`, naciskam N i wzlatuję nad labirynt. Żółte pudełka leżą na ścianach i słupkach. Pokazuję, że pudełko ściany jest grubsze niż ściana i że lica pudełek ścian i słupków tworzą jedną płaszczyznę.
3. **Kule.** Z góry widać cyjanowe kule w komórkach z kryształami: każda to trzy okręgi. Mówię, dlaczego tu jest kula, a nie pudełko (liczy się odległość, bryła niczego nie blokuje), i że kula stoi w miejscu, choć kryształ się kołysze.
4. **Pudełko i zasięg gracza.** Wracam na grunt (N) i patrzę w dół: zielone linie pudełka wokół mnie, a w nich zielone okręgi kuli zasięgu. Odczytuję `min` i `max` i pokazuję, że różnią się o 0,6, 1,8 i 0,6.
5. **Zatrzymanie.** Idę prosto na ścianę. Staję, a w `Player box` współrzędna od strony ściany przestaje się zmieniać.
6. **Ślizganie.** Idę ukosem w ścianę (W i A albo W i D). Sunę wzdłuż niej i mijam słupki bez zatrzymania. Mówię: osie są obsługiwane po kolei, ściana zabiera tylko składową skierowaną w nią.
7. **Zbieranie.** Podchodzę do kryształu. Gdy zielone okręgi wejdą w cyjanowe, kryształ znika razem ze swoją kulą i światłem, a licznik `pickup spheres` spada o 1. Mówię: to test kuli z kulą, kwadrat odległości przeciw kwadratowi sumy promieni, i nic mnie przy tym nie zatrzymało.
8. **Brama i strefa.** Przy wyjściu pokazuję pomarańczowe pudełko bramy i magentowe pudełko strefy za nim. Brama zatrzymuje mnie jak ściana. Po zebraniu wymaganej liczby kryształów pomarańczowe linie znikają, a w panelu jest `0 gate` i `All boxes: 242`. Wchodzę w strefę: karta `You escaped`. Mówię: brama była przeszkodą dla `moveAndSlide`, strefa jest tylko testem kuli z pudełkiem.
9. **Noclip.** Zaznaczam `Noclip (key N)` i przechodzę przez ścianę. Odznaczam w środku ściany: mogę wyjść w dowolną stronę, bo `moveAndSlide` nie trzyma pudełka, które zaczyna w przeszkodzie.
10. **Testy.** W terminalu uruchamiam `ctest --test-dir build/debug -C Debug --output-on-failure` i mówię, że to testy, a nie obraz, są dowodem poprawności.

## 7. Pułapki

1. **Pudełko ściany jest grubsze niż ściana, i to celowo.** Widoczny korpus ściany ma 0,2 m (`WALL_VISUAL_THICKNESS`), słupek 0,3 m (`PILLAR_SIZE`). Gdyby pudełko ściany miało 0,2 m, pudełko słupka wystawałoby 5 cm przed jej lico z każdej strony, a słupek stoi na każdym łączeniu segmentów, czyli co 2 m. Pudełko ślizgające się po ścianie trafiałoby wtedy na słupek i stawało. Tak było w pierwszej wersji kodu i przypinał to test. To nie był błąd `moveAndSlide`, tylko skutek wymiarów. Poprawka jest w danych, nie w algorytmie: pudełko ściany ma grubość słupka (`WALL_COLLISION_THICKNESS = PILLAR_SIZE`), więc lica pudełek ścian i słupków leżą w jednej płaszczyźnie, a pudełko sunące po ścianie tylko **styka się** ze słupkiem, a styk nie zatrzymuje (sekcja 2.3). Cena: gracz staje 5 cm przed widocznym korpusem ściany ([`../game/maze-generator.md`](../game/maze-generator.md), sekcja 5.7).
2. **Przesunięcie liczone z czasu klatki.** `moveAndSlide` zakłada krótkie kroki (sekcja 2.8). Przesunięcie policzone z `deltaSeconds()` zamiast z `fixedDt` po jednej wolnej klatce może mieć 75 cm i wtedy droga po schodkach wyraźnie różni się od prostej.
3. **Pudełko z `min` większym od `max`.** Nic tego nie sprawdza. Takie pudełko ma ujemną wspólną długość z każdym innym, więc `overlaps` zawsze zwraca fałsz, a `moveAndSlide` go nie widzi. Typowe źródło: ujemna połowa rozmiaru podana do `fromCenter` albo pomylone narożniki przy ręcznym tworzeniu.
4. **Start wewnątrz przeszkody.** Funkcja nie wypycha pudełka, które już jest w przeszkodzie: pozwala mu wyjść w dowolną stronę, ale też pozwala iść dalej w głąb. Pozycja startowa gracza musi leżeć w wolnym miejscu (środek komórki, `game::cellCenter`).
5. **Dwie definicje "dotyku".** `overlaps` jest ścisłe: zero to styk, cokolwiek powyżej zera to nakładanie. `moveAndSlide` traktuje jak styk wszystko do 1 mm. Po serii kroków pudełko może być w ścianie o ułamek milimetra i `overlaps` powie wtedy "tak". Do pytania "czy gracz jest w ścianie" po ruchu trzeba więc użyć pudełka pomniejszonego o tolerancję z zapasem: test wędrówki pomniejsza je o `2 * CONTACT_TOLERANCE` z każdej strony.
6. **Porównywanie pozycji przez `==`.** Po 2000 dodawań `float` suma różni się od iloczynu na czwartym miejscu po przecinku (zmierzone przy pisaniu testu: 30,2004 zamiast 30,2). Testy porównują przez `doctest::Approx` albo sprawdzają pojedynczy krok.
7. **Kolejność osi ma znaczenie przy narożniku wypukłym.** Pudełko idące ukosem dokładnie na róg przeszkody przejdzie po tej stronie, którą wyznacza oś obsługiwana pierwsza (x). Wynik jest poprawny (bez wchodzenia w przeszkodę), ale nie jest symetryczny.
8. **`std::span` niczego nie posiada.** To tylko widok. Wywołanie `moveAndSlide(box, step, game::mazeColliders(maze))` jest poprawne, bo tymczasowy wektor żyje do końca instrukcji, ale jest też powolne: buduje całą listę przy każdym kroku. Listę trzeba policzyć raz i trzymać w polu. Tak robi aplikacja: `m_obstacles` jest wektorem, który `roundObstacles` wypełnia na początku rundy, w chwili otwarcia bramy i od M6 po przebudowie terenu, a nie w każdym kroku. Zapamiętanie samego `std::span` do wektora, który potem znika, to wiszący wskaźnik.
9. **AABB nie obraca się z obiektem.** Pudełko obiektu obróconego o kąt inny niż wielokrotność 90 stopni trzeba policzyć od nowa, większe, tak żeby objęło obrócony kształt. W labiryncie problem nie występuje: ściany stoją tylko w dwóch ustawieniach i `game::wallBox` ma dla każdego osobne połowy rozmiarów.
10. **Nie ma grawitacji.** Oś y jest obsługiwana tak samo jak pozostałe (jest na to test), ale nic nie ciągnie pudełka w dół. Gracz jest trzymany na gruncie przez kod gry (`position.y = terrain.heightAt(position.x, position.z)` w `Player::update`, do M5 `position.y = FLOOR_Y`), nie przez kolizje: terenu nie ma na liście przeszkód (sekcja 2.13).
11. **Rysunek nie jest dowodem.** OpenGL narysuje obiekt w ścianie bez żadnego błędu. Dowodem poprawności kolizji są testy. Rysowanie brył z panelu Collision pomaga zobaczyć, **gdzie** pudełka i kule są, ale nie sprawdza, czy ruch i reguły ich przestrzegają.
12. **Rysowane pudełko jest o centymetr większe od prawdziwego.** Margines `LINE_MARGIN` chroni linie przed migotaniem na powierzchni modelu (sekcja 3). Kto mierzy coś na ekranie po liniach pudełek, mierzy z błędem 1 cm z każdej strony. Okręgi kul marginesu nie mają.
13. **Linie w prawdziwym rozmiarze migoczą.** Bez marginesu linie pudełka słupka leżą w powierzchni trzonu modelu i walczą z nim o głębię. Podobnie zachowałaby się każda inna geometria narysowana dokładnie w płaszczyźnie innej.
14. **`glLineWidth` nie pogrubi linii przenośnie.** Profil Core gwarantuje tylko szerokość 1. Grubsze linie trzeba by rysować jako wąskie prostokąty z trójkątów.
15. **Zielone pudełko gracza wyprzedza kamerę.** Razem z zieloną kulą zasięgu jest rysowane w pozycji z ostatniego kroku symulacji, a kamera w punkcie między dwoma krokami. W ruchu różnica to ułamek kroku, najwyżej 2,5 cm przy 3 m/s.
16. **Brak ściany na ekranie nie znaczy braku kolizji.** Pudełka powstają z siatki labiryntu, a nie z modeli. Gdy plik modelu ściany się nie wczyta, ściany znikają z obrazu, ale nadal zatrzymują gracza. To samo dotyczy bramy i kryształów: pudełko bramy i kule zbierania wynikają z danych rundy, a nie z plików modeli.
17. **Kule niczego nie zatrzymują.** `moveAndSlide` przyjmuje tylko pudełka. Kula dodana "jako przeszkoda" nie ma jak trafić na listę, a testy `overlaps` dla kul mówią tylko "tak albo nie", bez przesunięcia i bez kierunku. Kryształ nie jest więc przeszkodą: gracz przez niego przechodzi i przy okazji go zbiera.
18. **Ujemny promień.** Nic go nie sprawdza, tak jak nic nie sprawdza `min <= max` w pudełku, a podnoszenie do kwadratu gubi znak. W teście kuli z pudełkiem promień -0,5 działa dokładnie jak 0,5. W teście dwóch kul ujemna jest dopiero suma: promienie -1 i 0 dają sumę -1, jej kwadrat to 1, więc środki odległe o 0,5 m "nachodzą na siebie". Suwak `Pickup radius` ma dolną granicę 0,1, więc z panelu ujemnego promienia nie da się ustawić.
19. **Kula o promieniu 0 zachowuje się inaczej wobec kuli i wobec pudełka.** Punkt leżący wewnątrz kuli nachodzi na nią (odległość mniejsza od jej promienia). Punkt leżący wewnątrz pudełka **nie** nachodzi na nie: odległość do najbliższego punktu to 0, promień to 0, a `0 < 0` jest fałszem. Oba wyniki są przypięte testami. Komentarze w nagłówku mówią w obu przypadkach o "wspólnej objętości", co dla punktu w kuli nie jest ścisłe. W grze promienie są dodatnie, więc różnica nie ma skutków.
20. **Noclip zbiera kryształy, ale nie wygrywa przez zamkniętą bramę.** Tryb noclip wyłącza `moveAndSlide`, a nie reguły rundy: `updateRound` dostaje pozycję gracza tak czy inaczej. Kula zasięgu sięga wtedy kryształów także w locie i przez ściany. Strefa wyjścia liczy się dopiero przy otwartej bramie (`round.gateOpen`), więc przelot przez zamkniętą bramę rundy nie kończy.
21. **Brama przestaje blokować od razu, a model tonie jeszcze półtorej sekundy.** `gateBlocks` zwraca fałsz od chwili otwarcia, a `gateVisible` dopiero po `GATE_OPEN_SECONDS` (1,5 s). W tym czasie przez tonący model da się przejść, a pomarańczowego pudełka już nie ma. Zwykle nikt tego nie widzi, bo brama otwiera się, gdy gracz zbiera kryształ gdzie indziej. Zobaczy to ktoś, kto stoi przy bramie i przesuwa suwak `Crystals needed` w dół, albo ktoś, komu ostatni potrzebny kryształ wisiał w korytarzu przed bramą. Komentarz przy `gateBlocks` w `Round.hpp` mówi, że brama "has usually sunk before the player gets to it": to opis typowego przypadku, nie gwarancja.
22. **Cyjanowa kula nie kołysze się razem z kryształem.** Kula zbierania stoi w miejscu spoczynku, a model porusza się w górę i w dół o najwyżej 8 cm. Środek okręgów i środek widocznego kryształu rozjeżdżają się więc o tyle. To nie błąd rysowania: tak liczy test.
23. **Testy kul są dyskretne.** Sprawdzają pozycję po kroku, a nie drogę (sekcja 2.4). Przy krokach gry (około 4,6 cm przy sprincie) i sumie promieni 0,9 m niczego nie da się przeskoczyć. Przy promieniu rzędu kilku centymetrów i dużej prędkości w noclip już by się dało.
24. **Strefa wyjścia jest pudełkiem, ale nie przeszkodą.** `MazeWorld::exitZone` ma typ `Aabb`, lecz nie należy ani do `MazeWorld::colliders`, ani do `m_obstacles`. Kto dopisze ją do listy przeszkód, zepsuje wyjście: gracz zatrzyma się na jej ścianie, a jego kula zasięgu ma promień równy połowie szerokości ciała (0,3 m), więc tylko zetknie się ze strefą. O wygranej decydowałyby wtedy błędy zaokrągleń i tolerancja styku.

**Pułapki, które doszły w M6:**

- **Pudełko ściany na `y = 0`, model na gruncie.** `game::mazeColliders(maze)` daje pudełka dla labiryntu stojącego na zerze. W grze pudełka muszą powstać z pozycji już opuszczonych na teren (`colliderBoxes(world.walls, world.pillars)` w `placeOnTerrain`), inaczej żółte linie i prawdziwe przeszkody wiszą nad ścianami albo pod nimi.
- **Wysokość to też oś testu.** Na płaskiej podłodze łatwo zapomnieć, że `overlaps` i `moveAndSlide` patrzą na trzy osie. Na terenie ściana, której pudełko nie ma wspólnego zakresu wysokości z pudełkiem gracza, przestaje go zatrzymywać bez żadnego błędu. Pilnuje tego granica `MAX_HEIGHT_SCALE` i test z sekcji 2.13. Kto podniesie `MAZE_RELIEF` albo granicę suwaka tak, że ich iloczyn zbliży się do 1,8 m, traci tę gwarancję.
- **Linie pudełek w ziemi.** Dół pudełka ściany leży na najniższym gruncie pod nią, więc tam, gdzie grunt jest wyższy, dolna ramka żółtych linii chowa się pod powierzchnią terenu. To poprawny obraz, a nie błąd rysowania.

## 8. Ćwiczenia

Ćwiczenia od 1 do 5 i od 14 do 16 robi się na kartce. Ćwiczenia od 6 do 13 i od 17 do 20 to zmiany w kodzie, w testach albo w ustawieniach działającego programu: po każdej zmianie w kodzie zbuduj projekt i uruchom testy (`cmake --build --preset debug`, potem `ctest --test-dir build/debug -C Debug --output-on-failure`), a na końcu wycofaj swoją zmianę. Polecenie `git checkout src tests` cofa **wszystkie** niezatwierdzone zmiany w tych katalogach, więc używaj go tylko wtedy, gdy przed ćwiczeniem nie było w nich nic do zachowania.

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
11. **Linie bez marginesu.** W `ColliderLines.cpp` ustaw `LINE_MARGIN` na `0.0F`, uruchom program, włącz `Draw collision shapes` i obejrzyj słupek z bliska, poruszając kamerą. Co się dzieje z liniami na trzonie i dlaczego nie na korpusie ściany?
12. **Sześcian od -0,5 do 0,5.** Zmień narożniki `UNIT_CUBE_CORNERS` tak, żeby sześcian był wyśrodkowany. Co trzeba zmienić w `ColliderLines::draw`, żeby pudełka nadal trafiały na swoje miejsca?
13. **Linie przez ściany.** W `drawColliderLines` wyłącz test głębi przed rysowaniem linii (`glDisable(GL_DEPTH_TEST)`) i włącz go z powrotem po nim. Co widać i kiedy taki widok jest przydatny?
14. **Kula z kulą.** Kula A ma środek `(0, 0, 0)` i promień 0,5, kula B środek `(0,6, 0, 0,8)` i promień 0,4. Czy nachodzą na siebie? Odpowiedź: nie. Kwadrat odległości to `0,36 + 0,64 = 1`, kwadrat sumy promieni to `0,9 * 0,9 = 0,81`. A gdy B ma promień 0,6? Odpowiedź: tak, bo `1,1 * 1,1 = 1,21`. Co powiedziałby błędny wzór z sumą kwadratów promieni? Odpowiedź: `0,25 + 0,36 = 0,61`, czyli "nie", choć kule wyraźnie na siebie nachodzą.
15. **Najbliższy punkt i kula z pudełkiem.** Pudełko ma `min = (0, 0, 0)` i `max = (2, 3, 4)`. Podaj najbliższy punkt pudełka dla `(3, -1, 2)`. Odpowiedź: `(2, 0, 2)`. Czy kula o tym środku i promieniu 1,5 nachodzi na pudełko? Odpowiedź: tak, kwadrat odległości to `1 + 1 = 2`, a `1,5 * 1,5 = 2,25`. A o promieniu 1,4? Odpowiedź: nie, `1,96` jest mniejsze od 2.
16. **Zasięg zbierania.** Dla `pickupRadius` równego 1,0 policz, z jakiej odległości w poziomie gracz zbiera kryształ. Odpowiedź: suma promieni to 1,3, kwadrat 1,69, minus `0,0625` za różnicę wysokości 0,25 m, pierwiastek z `1,6275` to około 1,28 m. Zgadza się to z testem `a larger pickup radius reaches a crystal from further away`, w którym gracz stoi 1,2 m od kryształu.
17. **Styk kul jako kolizja.** W obu przeciążeniach `overlaps` dla kul zamień `<` na `<=` i uruchom testy. Przewidywanie (nie pomiar): przestają przechodzić dwa przypadki, `spheres that only touch do not overlap` i `a sphere that only touches a box does not overlap it`. Sprawdź, czy tylko te, i wyjaśnij, dlaczego testy zbierania w `RoundTests.cpp` nie powinny tego zauważyć.
18. **Suma kwadratów zamiast kwadratu sumy.** W `overlaps` dla dwóch kul zamień `reach * reach` na sumę kwadratów obu promieni. Przewidywanie (nie pomiar): zawodzą sprawdzenia z kulami, które naprawdę na siebie nachodzą, na przykład odległość 0,8 m przy promieniach 0,5 i 0,5 (`0,64 < 0,5` jest fałszem), a przypadek o samym styku przechodzi dalej. Dlaczego akurat test styku nie wykrywa tego błędu?
19. **Promień z suwaka.** Uruchom program, włącz `Draw collision shapes` i w panelu Gameplay ustaw `Pickup radius` na 2,0. Co dzieje się z cyjanowymi okręgami i z jakiej komórki da się teraz zebrać kryształ? Przewidywanie z liczb: suma promieni 2,3 m to więcej niż 2 m między środkami sąsiednich komórek, więc kryształ da się zebrać zza ściany. Testy kul nie wiedzą nic o ścianach.
20. **Odwrotna kolejność argumentów.** Dopisz przeciążenie `overlaps(const Aabb&, const Sphere&)`, które woła istniejące, i przypadek testowy, który sprawdza, że obie kolejności dają to samo.

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
    Pudełko, kula i ich funkcje nie wiedzą nic o labiryncie, o graczu ani o kryształach, więc należą do biblioteki `engine` i nadają się do innych programów. Dołączają tylko GLM i bibliotekę standardową: żadnego OpenGL, okna ani wejścia, dzięki czemu testy działają bez okna.

14. **Jak sprawdzić kolizję kuli z AABB?**
    Przyciąć środek kuli do przedziałów pudełka (osobno x, y, z), co daje najbliższy punkt pudełka (`closestPoint`), i porównać kwadrat odległości od niego z kwadratem promienia. Od M5 robi to `overlaps(const Sphere&, const Aabb&)`, a gra używa go do strefy wyjścia: kula zasięgu gracza przeciw pudełku strefy.

15. **Dlaczego pudełko kolizji ściany ma 0,3 m, skoro ściana ma 0,2 m?**
    Żeby lica pudełek ścian i słupków (0,3 m) leżały w jednej płaszczyźnie. Przy pudełku 0,2 m słupki wystawałyby 5 cm przed ścianę i gracz sunący po ścianie stawałby co 2 metry. Teraz pudełko gracza tylko styka się ze słupkiem, a styk nie zatrzymuje ruchu.

16. **Jak gracz używa `moveAndSlide`?**
    W każdym kroku chodzenia liczy chciane przesunięcie (kierunek razy prędkość razy stały krok), buduje pudełko ze swojej pozycji i woła `moveAndSlide` z listą przeszkód rundy (`m_obstacles`): pudełkami labiryntu, policzonymi raz przy generowaniu, i pudełkiem bramy, dopóki jest zamknięta. Wynik dodaje do pozycji. W trybie noclip funkcji nie woła.

17. **Jak rysowane są pudełka kolizji?**
    Jedną siatką sześcianu o boku 1 (8 wierzchołków, 24 indeksy) rysowaną prymitywem `GL_LINES`: każde dwa indeksy to jedna krawędź. Dla każdego pudełka macierz modelu skaluje sześcian do rozmiaru pudełka i przesuwa go do narożnika `min`. Kolor przychodzi jako uniform `uColor`.

18. **Czym `GL_LINES` różni się od `GL_TRIANGLES` w `glDrawElements`?**
    Sposobem grupowania indeksów: przy liniach każde dwa indeksy to odcinek, przy trójkątach każde trzy to trójkąt. Bufory i atrybuty są takie same.

19. **Po co margines 1 cm przy rysowaniu pudełek?**
    Pudełko słupka ma dokładnie szerokość trzonu modelu, więc linie w prawdziwym rozmiarze leżałyby w powierzchni modelu i migotały (walka o głębię). Margines wysuwa je przed powierzchnię. Dotyczy tylko rysowania, kolizje liczą się na prawdziwych pudełkach.

20. **Dlaczego `color.vert` czyta tylko pozycję, skoro siatka ma też normalną, uv i styczną?**
    Shader nie musi deklarować wszystkich atrybutów, które VAO udostępnia. Linie w jednym kolorze nie potrzebują normalnej, współrzędnych tekstury ani stycznej.

21. **Co robi przełącznik `Noclip (key N)` w panelu Collision?**
    Zmienia pole `player.noclip`, to samo, które przełącza klawisz N. W trybie noclip gracz lata wzdłuż kierunku patrzenia i nie woła `moveAndSlide`. Po wyłączeniu najbliższy krok stawia go na podłodze.

22. **Kiedy kula jest lepszą bryłą niż AABB?**
    Gdy liczy się sama odległość ("czy gracz jest dość blisko") i bryła niczego nie blokuje: przy zbieraniu i przy strefach. Kula sięga tak samo daleko w każdym kierunku i nie zmienia się przy obrocie obiektu. Dla ścian jest gorsza, bo długi i płaski kształt wypełnia źle.

23. **Jak sprawdzić, czy dwie kule na siebie nachodzą, i dlaczego porównuje się kwadraty?**
    Odległość środków ma być mniejsza od sumy promieni. Kod porównuje `dot(offset, offset)`, czyli kwadrat odległości, z kwadratem sumy promieni. Dzięki temu nie liczy pierwiastka, a wynik jest ten sam, bo dla liczb nieujemnych podnoszenie do kwadratu zachowuje kolejność.

24. **Co to jest `dot(v, v)`?**
    Iloczyn skalarny wektora z samym sobą: `x * x + y * y + z * z`, czyli kwadrat jego długości.

25. **Jak znaleźć punkt pudełka najbliższy danemu punktowi?**
    Przyciąć każdą współrzędną punktu osobno do przedziału od `min` do `max` pudełka (`glm::clamp`). Punkt leżący w pudełku jest swoim własnym najbliższym punktem. Dla pudełka od `(0, 0, 0)` do `(2, 3, 4)` i punktu `(5, 1, 1)` wychodzi `(2, 1, 1)`.

26. **Czy stykające się kule są w kolizji? A kula dotykająca pudełka?**
    Nie. Oba testy używają ostrej nierówności `<`, tak jak test dwóch pudełek: bryły, które mają wspólny tylko brzeg, nie nachodzą na siebie.

27. **Dlaczego mówi się, że kula przy narożniku pudełka jest "okrąglejsza" niż pudełko?**
    Kula ma ten sam zasięg we wszystkich kierunkach, a pudełko sięga po przekątnej dalej. W teście: środek `(2,5, 1, 2,5)` przy pudełku od `(0, 0, 0)` do `(2, 2, 2)` jest około 0,71 m od jego krawędzi (kwadrat odległości 0,5). Kula o promieniu 0,6 nie nachodzi (`0,36`), kula o promieniu 0,75 nachodzi (`0,5625`), a pudełko o połowie boku 0,6 nachodziłoby o 0,1 m na obu osiach.

28. **Czy kula może zatrzymać gracza albo czy gracz może się po niej ślizgać?**
    Nie. Dla kul jest tylko test nakładania. `moveAndSlide` przesuwa pudełko wśród pudełek i kul nie przyjmuje.

29. **Do czego gra używa kul?**
    Do dwóch pytań w `updateRound`. Zbieranie: kula zasięgu gracza (środek 0,9 m nad stopami, promień 0,3 m) przeciw kuli kryształu (środek kryształu w miejscu spoczynku, promień `pickupRadius`, domyślnie 0,6 m). Wygrana: ta sama kula zasięgu przeciw pudełku strefy wyjścia, i tylko przy otwartej bramie.

30. **Z jakiej odległości gracz zbiera kryształ przy ustawieniach domyślnych?**
    Suma promieni to 0,9 m. Kryształ wisi 0,25 m nad środkiem kuli gracza, więc w poziomie zostaje około 0,86 m od środka komórki. To wystarcza wzdłuż ścian i w rogach komórki kryształu (tam gracz jest najwyżej 0,55 m od środka na każdej osi) i nie wystarcza z sąsiedniej komórki ani zza ściany. Wchodząc przez otwarty bok, gracz zbiera kryształ około 14 cm za granicą komórki.

31. **Co się zmieniło w liście przeszkód gracza w M5?**
    Do M4 była to lista `MazeWorld::colliders`: ściany i słupki. Teraz to `m_obstacles` z funkcji `roundObstacles`: ta sama lista plus pudełko bramy, dopóki brama jest zamknięta. Lista jest budowana na początku rundy i ponownie w kroku, w którym brama się otworzyła.

32. **Czym różnią się brama i strefa wyjścia, skoro obie są `Aabb`?**
    Rolą. Pudełko bramy jest na liście przeszkód i zatrzymuje ruch w `moveAndSlide`. Pudełko strefy nie jest na żadnej liście przeszkód: służy tylko do testu z kulą zasięgu gracza, a wejście w nie kończy rundę.

33. **Jak rysowana jest kula i dlaczego trzema okręgami?**
    Jedną siatką okręgu o promieniu 1 (32 wierzchołki, 64 indeksy, `GL_LINES`), rysowaną trzy razy: bez obrotu, po ćwierć obrotu wokół osi X i po ćwierć obrotu wokół osi Y (`CIRCLE_ROTATIONS`). Macierz modelu skaluje okrąg do promienia kuli i przesuwa do jej środka. Trzy okręgi pokazują środek i zasięg w każdym z trzech kierunków i nie zasłaniają tego, co jest w środku.

34. **Co znaczą kolory linii kolizji?**
    Żółty: ściany i słupki. Zielony: pudełko gracza i kula jego zasięgu. Pomarańczowy: brama, dopóki blokuje. Cyjan: kule zbierania wokół kryształów, które jeszcze wiszą (w miejscu spoczynku, bez kołysania). Magenta: strefa wyjścia.

35. **Czy w trybie noclip da się zebrać kryształ albo wygrać rundę?**
    Zebrać tak: noclip wyłącza tylko ruch z kolizjami, a `updateRound` nadal porównuje kule. Wygrać tylko przy otwartej bramie: test strefy jest poprzedzony warunkiem `round.gateOpen`.

36. **Co M6 zmienił w pudełkach kolizji ścian, a czego nie?**
    Zmienił ich wysokość nad zerem: dół pudełka ściany, słupka i bramy leży na najniższym gruncie pod obrysem. Nie zmienił rozmiarów (3 m, 3,15 m) ani położenia w planie: `x` i `z` są takie same jak na płaskim gruncie, więc kolizje w poziomie działają identycznie.

37. **Co chroni stała `MAX_HEIGHT_SCALE = 2,5`?**
    Wspólny zakres wysokości pudełka gracza i pudełek ścian. Test nakładania jest trójwymiarowy, więc ściana zatrzymuje tylko wtedy, gdy pudełka nakładają się także na y. Grunt w labiryncie ma najwyżej `2,5 * 0,6 = 1,5` m różnicy wysokości, mniej niż 1,8 m ciała gracza i mniej niż 3 m ściany, więc gracz stojący gdziekolwiek obok ściany zawsze dzieli z jej pudełkiem co najmniej 0,3 m.

38. **Czy gracz zderza się z terenem?**
    Nie. Terenu nie ma na liście przeszkód. Wysokość stóp jest czytana z `Terrain::heightAt` przed ruchem i po nim, a `moveAndSlide` dostaje przesunięcie tylko w poziomie.

## 10. Źródła

- LearnOpenGL, rozdział "Collision detection": <https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection> (AABB z AABB, AABB z kołem przez przycinanie do najbliższego punktu) i "Collision resolution": <https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-resolution>.
- Christer Ericson, "Real-Time Collision Detection" (Morgan Kaufmann, 2005): rozdział 4.2 (AABB i ich reprezentacje), 4.3 (kule i test kuli z kulą), 5.1.3 (punkt AABB najbliższy danemu punktowi), 5.2.5 (test kuli z AABB), 5.5 (testy obiektów w ruchu), 7.1 (siatki jako struktury przyspieszające).
- MDN, "3D collision detection": <https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection> (AABB i kula, krótkie wprowadzenie).
- Glenn Fiedler, "Fix Your Timestep!": <https://gafferongames.com/post/fix_your_timestep/> (dlaczego symulacja idzie stałym krokiem).
- cppreference, `std::span`: <https://en.cppreference.com/w/cpp/container/span>, inicjalizatory desygnowane: <https://en.cppreference.com/w/cpp/language/aggregate_initialization>.
- Dokumentacja OpenGL, `glDrawElements` (prymityw `GL_LINES`): <https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawElements.xhtml>, `glLineWidth`: <https://registry.khronos.org/OpenGL-Refpages/gl4/html/glLineWidth.xhtml>.
- Dokumenty w tym repozytorium: [`README.md`](README.md) (moduł `scene`), [`../game/maze-generator.md`](../game/maze-generator.md) (skąd biorą się pudełka labiryntu), [`../game/player.md`](../game/player.md) (gracz jako użytkownik kolizji), [`../game/gameplay.md`](../game/gameplay.md) (reguły rundy: zbieranie kryształów, brama, strefa wyjścia), [`../game/maze-rendering.md`](../game/maze-rendering.md) (jedna siatka, wiele macierzy), [`../gfx/mesh.md`](../gfx/mesh.md) (rodzaj prymitywu w `Mesh`), [`../gfx/uniforms.md`](../gfx/uniforms.md) (nazwy uniformów), [`../core/main-loop.md`](../core/main-loop.md) (stały krok, `FIXED_DT`, `MAX_FRAME_TIME`), [`../../libraries/doctest.md`](../../libraries/doctest.md) (testy), [`../../decisions/collision-aabb-sliding.md`](../../decisions/collision-aabb-sliding.md) (dlaczego nie silnik fizyki), [`../../decisions/exit-farthest-cell.md`](../../decisions/exit-farthest-cell.md) (dlaczego jedna brama zamyka wyjście).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 3 (temat 14 i jego pokaz w ImGui), sekcja 6 (zawartość warstwy `scene/`).
