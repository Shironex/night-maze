# Budowanie na Windowsie

> **Od 2026-10-06 trzynaście paneli to jedno okno debug.** Nazwy w rodzaju "panel Renderer", "panel Lights" albo "panel Framebuffers"
> w tym dokumencie oznaczają miejsca w oknie debug (kategoria / zakładka / karta): tabela w sekcji 27.3, lista kontrolna
> okna w sekcji 27. Okno startuje **ukryte**, więc przed każdym krokiem z kontrolkami naciśnij `~`. W punktach `[ ]` nazwy
> paneli są już zamienione na nowe miejsca, a kroki o zwijaniu paneli, rzędach pasków tytułu, układzie kolumn i dokowaniu
> stałych paneli mają dopisek "bez odpowiednika w oknie debug". Punkty `[x]` i opisy pomiarów zostają z nazwami z dnia pomiaru.
> Żaden punkt nie został odhaczony ani odznaczony przy tej zmianie. Minimapa startuje od 2026-10-06 w lewym dolnym rogu (była
> w prawym dolnym), a pasek HUD stoi zawsze przy górnej krawędzi.

> **Stan na 2026-10-05: build z terminala jest sprawdzony na Windowsie, część ręczna nie.**
> Kod M0 i M1 powstał na macOS (patrz [`build-macos.md`](build-macos.md)). Na Windowsie 11
> zbudowałem go i uruchomiłem po raz pierwszy 2026-10-05, samymi narzędziami "Visual Studio
> Build Tools 2022", bez środowiska Visual Studio (IDE).
>
> **Zmierzone 2026-10-05:** konfiguracja i build Debug oraz Release (zero ostrzeżeń pod
> `/W4`), uruchomienie programu, kopia katalogu `assets`, błąd kompilacji shadera przy
> starcie, generator Ninja. Kod z M2 + M3 (kolizje, labirynt, gracz, modele, tekstury, panele
> Maze, Collision i Assets) powstał na tym PC: build Debug, Release i Ninja bez ostrzeżeń,
> testy jednostkowe przechodzące w obu konfiguracjach, start gry w oteksturowanym labiryncie bez linii
> `[error]` i zrzuty ekranu kilku stanów (sekcja 12).
>
> **Zmierzone 2026-10-05 (M4: oświetlenie i mapy normalnych):** build Debug i Release bez
> ostrzeżeń, 163 przypadki testowe i 62220 asercji w obu konfiguracjach, clang-format i
> clang-tidy bez uwag, start gry w oświetlonej nocnej scenie bez linii `[error]`, w tym bez
> żadnej z nazwą błędu OpenGL (`GL_...`), zrzuty ekranu czterech trybów oświetlenia i
> kilku innych stanów (sekcja 13.1) oraz zrzuty ekranu map normalnych: kierunek reliefu,
> reakcja na kierunek światła, tryby `Gouraud` i `Unlit` bez zmian (sekcja 13.3).
>
> **Zmierzone 2026-10-05 (M5: rozgrywka):** build Debug i Release bez ostrzeżeń, 215
> przypadków testowych i 85098 asercji w obu konfiguracjach, obraz sprawdzony na zrzutach
> ekranu zrobionych tymczasowymi wstawkami w kodzie, które są już usunięte (sekcja 14.1).
> Wersji kompilatora, karty graficznej i sterownika, wyniku clang-format i clang-tidy ani
> listy zrzutów dla M5 nie zapisano, więc ich tu nie podaję.
>
> **Zmierzone 2026-10-05 (M6, część 1: skybox):** build Debug i Release bez ostrzeżeń, 221
> przypadków testowych i 85175 asercji w obu konfiguracjach, orientacja nieba sprawdzona na
> zrzucie ekranu (sekcja 15.1). Wersji kompilatora, karty graficznej i sterownika ani wyniku
> clang-format i clang-tidy dla tej części nie zapisano. Terenu i trawy ten pomiar nie
> obejmuje: mają własny akapit niżej.
>
> **Zmierzone 2026-10-05 (M6, część 2: teren i trawa):** 256 przypadków testowych i 101232
> asercje w Debug i w Release (uruchomione na istniejących programach testowych, żaden plik
> źródłowy nie jest od nich nowszy), a liczby terenu i trawy przeliczone niezależnie z pliku
> `heightmap.png` i ze wzorów kodu (sekcja 16.1). Build bez ostrzeżeń, clang-format, liczba
> klatek na sekundę i zrzuty ekranu są zgłoszonym wynikiem, którego nie powtarzałem. Wersji
> kompilatora, karty graficznej i sterownika ani wyniku clang-tidy dla tej części nie
> zapisano.
>
> **Zgłoszone 2026-10-05 (M7, część 1: bufor HDR i gamma):** bramka `make check`
> przechodzi (formatowanie, testy w Debug i w Release, clang-tidy), zero ostrzeżeń, 269
> przypadków testowych i 102103 asercje w obu konfiguracjach, brak błędów OpenGL w buildzie
> Debug przy podglądach załączników, po zmianie rozmiaru okna i po minimalizacji, porównanie
> obrazu z poprzednim commitem i liczba klatek na sekundę (sekcja 17.1). Żadnego z tych
> pomiarów nie powtarzałem. Wersji kompilatora, karty graficznej i sterownika dla tej
> części nie zapisano.
>
> **Zgłoszone 2026-10-05 (M7, część 2: bloom):** bramka `make check` przechodzi, zero
> ostrzeżeń w Debug i w Release, 276 przypadków testowych i 102139 asercji w obu
> konfiguracjach, z wyłączonym bloomem obraz identyczny co do piksela z pierwszą częścią
> (zmierzone przed zmianą świecenia kryształów), poświata kryształów i księżyca na zrzutach
> ekranu i liczba klatek na sekundę z bloomem i bez (sekcja 18.1). Żadnego z tych pomiarów
> nie powtarzałem. Wersji kompilatora, karty graficznej i sterownika dla tej części nie
> zapisano. Po tej części M7 był rozpoczęty, nie kompletny: mgły, winiety, minimapy
> i cieni nie było.
>
> **Zgłoszone 2026-10-05 (M7, część 3: mgła i winieta):** bramka `make check` przechodzi,
> zero ostrzeżeń w Debug i w Release, 294 przypadki testowe i 102412 asercji w obu
> konfiguracjach, z wyłączoną mgłą i winietą obraz identyczny co do piksela z drugą
> częścią, oba widoki diagnostyczne identyczne przy włączonych wartościach startowych
> i liczba klatek na sekundę z oboma efektami i bez nich (sekcja 19.1). Żadnego z tych
> pomiarów nie powtarzałem. Wersji kompilatora, karty graficznej i sterownika dla tej
> części nie zapisano. Po tej części M7 był rozpoczęty, nie kompletny: były bufor HDR,
> bloom, mgła i winieta, minimapy i cieni nie było.
>
> **Zgłoszone 2026-10-05 (M7, część 4: cienie księżyca):** bramka `make check` przechodzi
> (formatowanie, buildy i testy w Debug i w Release, clang-tidy), 310 przypadków testowych
> i 103751 asercji, z odznaczonym polem `Shadows` i suwakiem `Moon intensity` cofniętym do
> 0,12 obraz identyczny co do piksela z trzecią częścią poza paskiem HUD (w trybie `Phong`
> różnica najwyżej 1/255), brak błędów OpenGL w buildzie Debug przy mapie cieni 2048
> i 1024 oraz liczba klatek na sekundę z cieniami i bez (sekcja 20.1). Liczby klatek są
> zaszumione i nie wolno ich zestawiać z sekcją 19.1. Żadnego z tych pomiarów nie
> powtarzałem. Wersji kompilatora, karty graficznej i sterownika dla tej części nie
> zapisano. M7 jest rozpoczęty, nie kompletny: w kodzie są cztery części z sześciu (bufor
> HDR, bloom, mgła i winieta, cienie księżyca), cieni latarki i minimapy nie ma
> ([`m7-status.md`](m7-status.md)).
>
> **Zgłoszone 2026-10-06 (M7, część 5: cień latarki):** bramka `make check` przechodzi,
> 329 przypadków testowych i 104306 asercji (po części 4 było 310 i 103751) i start
> programu Debug przez około 7 sekund: OpenGL 4.1.0 NVIDIA, assety wczytane, puste
> standardowe wyjście błędów, panele ukryte (sekcja 21.1). Start **nie obejmował** trybu
> `Gouraud`, zakładki `Flashlight` i jej podglądu, ścieżki z wyłączoną latarką ani przycisku
> `Reload shaders`. Żadnego z tych pomiarów nie powtarzałem, **w chwili tego zgłoszenia nikt nie obejrzał obrazu tej części** i nikt nie zmierzył liczby klatek (agent obejrzał obraz później na zrzutach, nie właściciel: sekcja 21.1; z odczytami liczby klatek: sekcja 23.1). Wersji kompilatora, karty graficznej i
> sterownika dla tej części nie zapisano. M7 jest rozpoczęty, nie kompletny: w kodzie jest
> pięć części z sześciu (bufor HDR, bloom, mgła i winieta, cienie księżyca, cień latarki),
> minimapy nie ma ([`m7-status.md`](m7-status.md)).
>
> **Zgłoszone 2026-10-06 (M7, część 6: minimapa):** bramka `make check` przechodzi w Debug
> i w Release, 414 przypadków testowych i 138711 asercji (po części 5 było 375 i 138506:
> dopisanych w tej części jest 39 przypadków, w tym 19 w `DiscoveryTests.cpp`, 19 w
> `MinimapTests.cpp` i 1 w `MazeLayoutTests.cpp`) i start programu Debug przez około 7
> sekund: OpenGL 4.1.0 NVIDIA, assety wczytane, puste standardowe wyjście błędów (sekcja
> 22.1). Start **nie obejmował** poruszania graczem, klawisza M, zakładki `Minimap`, pola
> `Reveal all`, przycisku `Reload shaders`, zmiany rozmiaru okna ani obu widoków
> diagnostycznych. Żadnego z tych pomiarów nie powtarzałem, **w chwili tego zgłoszenia nikt nie obejrzał minimapy** i nikt nie zmierzył liczby klatek (agent obejrzał ją później na zrzutach, nie właściciel: sekcja 22.1). Wersji kompilatora, karty graficznej i sterownika dla
> tej części nie zapisano. M7 jest kompletny w kodzie na Windowsie (wszystkie sześć części)
> i **nie jest zamknięty** ([`m7-status.md`](m7-status.md)).
>
> **Zgłoszone 2026-10-06 (M8, część 1: environment mapping):** bramka `make check` przechodzi
> dla scalonego drzewa (po minimapie), 445 przypadków testowych (414 przed tą częścią plus 31:
> 11 w `EnvironmentMappingTests.cpp` i 20 w `PuddleTests.cpp`) i 150296 asercji (138711
> plus 11585 asercji tej części, potwierdzone bramką scalonego drzewa), a w
> drzewie z samą tą częścią 406 i 150091. Start programu Debug przez około 8 sekund z pustym
> standardowym wyjściem błędów (sekcja 23.1). Start obejmował **tylko ścieżkę domyślną**
> (Blinn-Phong, widok `Textured`, niebo i efekt włączone) i **nie obejmował** trybów `Unlit`,
> `Gouraud` i `Phong`, widoków diagnostycznych, nieba wyłączonego, efektu wyłączonego,
> przycisku `Reload shaders` (dziś czternaście programów) ani żadnej z dziewięciu kontrolek
> panelu Environment. Żadnego z tych pomiarów nie powtarzałem, **do tego dnia nikt nie obejrzał odbić na kryształach ani w kałużach** (patrz następny akapit) i nikt nie zmierzył liczby klatek. Temat 12 jest w toku, macOS
> otwarty ([`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md)).
>
> **Zgłoszone 2026-10-06 (M8, część 2: selekcja, dźwignie i kartki):** bramka `make check`
> przechodzi w Debug i Release, 466 przypadków testowych (445 przed tą częścią plus 21 w
> `InteractionTests.cpp`) i 152264 asercji (przed tą częścią 150296). Start programu Debug z
> czystym logiem, w którym nowe modele i tekstury są wypisane jako wczytane (sekcja 24.1).
> Agent, który napisał kod, uruchomił grę, sterował nią skryptem i oglądał zrzuty ekranu:
> widział dźwignię z pierścieniem i podpowiedzią, pociągnięcie, pulsujące podświetlenie,
> kartkę z jej kartą, otwartą ścianę na minimapie, restart, zamrożony promień i panel
> Collision. To **nie** jest test właściciela. Po przeróbce modelu dźwigni agent widział też (zrzuty z 2026-10-06, nie właściciel): dźwignię od przodu z 1 m przy włączonej latarce, w górnym położeniu i podświetloną (ciemna płyta, jasna bursztynowa gałka nad środkiem, podpowiedź na dole pośrodku); ten sam widok po E (pręt w dół, gałka poniżej krawędzi płyty z cieniem na ścianie, bez podświetlenia i podpowiedzi); widoki z boku w górnym i dolnym położeniu; wyłączoną latarkę (pociągnięta: płyta prawie czarna, gałka ciemna ochra; w górze z podświetleniem: gałka świeci bursztynem); około 2,4 m pod kątem z podświetleniem i podpowiedzią oraz około 2,9 m poza zasięgiem bez podświetlenia; drugą dźwignię; Gouraud (z podświetleniem) i Unlit; widok Normals (dźwignia jako dane, bez podświetlenia, pierścień i podpowiedź nadal są); kartkę z "E: read note" na dole, wolną od arkusza; otwartą kartę kartki bez podpowiedzi i bez nakładania się na minimapę. Nadal niewidziane: uchwyt w połowie ruchu, cień księżyca od dźwigni, Gouraud i Unlit dla kartki, widok na wprost z 2,5 m, dodatki w panelu Gameplay i przycisk "Pull all levers", inne niż domyślne liczby dźwigni i kartek, czysta rama połowicznie zatopionej ściany. Znane uwagi kosmetyczne: z 1 m na wprost gałka w górnym położeniu zasłania górną trzecią część płyty, a przy wyłączonej latarce i bez podświetlenia płyta jest prawie czarna na ścianie i niesie ją tylko gałka. Temat 15 jest w toku, macOS otwarty.
>
> **Zgłoszone 2026-10-06 (pierwsze obejrzenie obrazu M7 i M8 przez agenta oraz poprawki kałuż, ramki
> minimapy i paska HUD):** po raz pierwszy ktoś obejrzał obraz części 4, 5 i 6 M7 i części 1 M8. To był
> **agent**, na zrzutach ekranu (wersja Release z commitu `9a33f18`, 1280 x 720, RTX 4070 Ti SUPER, sterownik
> zgłaszający OpenGL 4.1.0 NVIDIA 610.74; ustawienia zmieniał tymczasowy hak testowy, który pisał te same pola
> co panele). Wyniki są w sekcjach 20.1, 21.1, 22.1 i 23.1 jako "widziane na zrzucie ekranu przez agenta
> (2026-10-06), nie przez właściciela". **Listy właściciela (od 17.2 do 24.2), macOS, odhaczenie tematów i tag
> zostają otwarte**, a właściciel nie wykonał żadnej z list ręcznych. Na ekranie znaleziono błędy kałuż (cztery
> z trzynastu obcięte do 54 do 69 procent, czarne dziury w wiązce latarki, ukryte linie panelu Environment), po
> czym właściciel zdecydował (2026-10-06): kałuże idą za gruntem, woda ma być lepiej widoczna, pasek HUD stoi
> przy górnej krawędzi, dopóki panele są schowane. Poprawki (17 plików w `src/`, `assets/shaders/` i `tests/`)
> są w commicie `ee093c2`. Bramka scalonego drzewa z poprawkami: `make check` przechodzi,
> **467 przypadków testowych i 158006 asercji** (przed poprawkami 466 i 152264), start Debug przez 8 sekund bez
> linii błędu (zgłoszone przez bramkę, nie powtarzałem). Rzędy liczby klatek z jednej sesji (około 1250 ze wszystkim
> włączonym, około 1920 przy wyłączonych dwóch cieniach, mapie i environment mapping, około 578 w buildzie Debug)
> są **odczytami, nie pomiarami**, a spadek liczby klatek z [`m7-status.md`](m7-status.md), sekcja 5, zostaje niewyjaśniony.
> Na macOS nic z tego nie było budowane ani uruchamiane.
>
> **Zgłoszone 2026-10-06 (M9, część 1: kamera menu):** bramka `make check` przechodzi,
> **497 przypadków testowych i 219050 asercji** (przed tą częścią 467 i 158006; nowych
> przypadków 30: 23 w `MenuCameraTests.cpp` i 7 w `StartOptionsTests.cpp`, policzone z plików).
> Agent, który napisał kod, uruchomił grę z `--menu-camera`, nagrał ją i obejrzał klatki (sekcja
> 25.1): kamera w korytarzach, bez HUD, minimapy i paneli, bez przeciętych ścian, ruch płynny. To
> **nie** jest test właściciela. Klawisza F2 i kontrolek panelu Camera nikt nie uruchomił na
> ekranie. Właściciel zdecydował (2026-10-06), że tłem menu będzie zmontowana pętla wideo, a
> przełączniki `--menu-shot` i `--menu-time` zostają. Lista właściciela: sekcja 25.2. Na macOS nic
> z tego nie było budowane.
>
> **Zgłoszone 2026-10-06 (M9, część 2: menu w RmlUi, ekrany gry i Escape):** bramka `make check`
> przechodzi, **519 przypadków testowych i 219195 asercji** na commicie `8c99911` (przed tą częścią 497
> i 219050; nowych przypadków 22: 21 w `GameStateTests.cpp` i 1 w `StartOptionsTests.cpp`, policzone z
> plików). Agent, który napisał kod, uruchomił grę, sterował nią skryptem i oglądał zrzuty ekranu
> (sekcja 26.1): menu główne nad przelatującą kamerą, `Play`, Escape z pauzą i stojącym czasem, F, M i R
> bez działania w pauzie, panele nad pauzą, `Resume`, `Restart`, `Back to menu`, `Quit` (kod wyjścia 0),
> `--play`, F2 i `--menu-camera`. Ekran wyniku widział **tylko** przez tymczasową, niezatwierdzoną
> linię, która po trzech sekundach wymuszała wygraną: przejścia całej gry do wygranej nikt nie zagrał.
> To **nie** jest test właściciela. Nie widziane przez nikogo: kursor (przechwycony albo wolny), obrót
> myszą po `Play`, zmiana rozmiaru okna, minimalizacja, skalowanie ekranu inne niż 100 procent, Tab i
> Enter w menu. Lista właściciela: sekcja 26.2. Od tej części **Escape cofa o jeden ekran (z rundy do pauzy, a pauza zatrzymuje rundę) i nigdy nie zamyka
> programu**, gra **startuje w menu głównym**, a punkty `[ ]` starszych sekcji o
> Escape i o kursorze zostały przepisane na zachowanie dzisiejszego programu (sekcje 11, 12 i 14; opis: sekcja 26). Na macOS nic z tego nie było budowane.
>
> **Nadal niesprawdzone:** wszystko, co wymaga człowieka przy myszy i klawiaturze (chodzenie
> i ślizganie po ścianach, klawisze N, F i R, obrót myszą, przyciski, listy i suwaki paneli, w
> tym lista `Lighting`, cały panel Lights i cały panel Gameplay, zbieranie kryształów, pusta
> bateria, przejście przez otwartą bramę, karta wygranej, HUD przy ukrytych panelach,
> rozwijanie paneli Camera i Gameplay, pole `Skybox` i suwak `Sky brightness`, chodzenie po
> nierównym podłożu, panele Terrain i Grass z ich suwakami i polami wyboru, panel
> Framebuffers z suwakiem ekspozycji, listą krzywych i podglądami, jego dwie zakładki
> z kontrolkami bloomu, mgły i winiety, cały panel Shadows z przełączaniem rozdzielczości
> mapy cieni w działającej grze i jego zakładka `Flashlight`, trzy nowe suwaki ręki w panelu
> Lights, cienie latarki na ekranie i minimapa (obraz, klawisz M, trzecia zakładka panelu Framebuffers: **agent widział je na zrzutach 2026-10-06, właściciel nie**; tak samo cienie księżyca, tryby oświetlenia i widoki diagnostyczne w części M7 i M8), zmiana rozmiaru okna,
> docking, przycisk "Reload shaders"), praca w Visual Studio (Open Folder, F5, Build
> Solution), RenderDoc i clangd w edytorze. Zdania o tych rzeczach są nadal przewidywaniem i
> są tak oznaczone. Listy kontrolne w sekcjach 11 (pierwszy build, stan M1), 12 (M2 + M3),
> 13 (oświetlenie i mapy normalnych, M4), 14 (rozgrywka, M5), 15 (skybox, pierwsza część
> M6), 16 (teren i trawa, druga część M6), 17 (bufor HDR i gamma, pierwsza część M7),
> 18 (bloom, druga część M7), 19 (mgła i winieta, trzecia część M7), 20 (cienie księżyca,
> czwarta część M7), 21 (cień latarki, piąta część M7) i 22 (minimapa, szósta część M7) 23 (M8, część 1: environment mapping) i 24 (M8, część 2: selekcja, dźwignie i kartki) oraz 25 (M9, część 1: kamera menu) i 26 (M9, część 2: menu w RmlUi, ekrany gry i Escape)
> rozróżniają punkty
> zmierzone
> (`[x]`, z wynikiem) od otwartych (`[ ]`).
> Sekcje 11, 12 i 13 są zapisem stanu z 2026-10-05: liczby i teksty paneli w ich punktach
> `[x]` opisują program z dnia pomiaru (z kostką z M1, a w sekcji 13 także z kostkami
> znaczników świateł, światłami w ślepych zaułkach i pięcioma programami shaderów: M5 to
> wszystko usunął albo zastąpił). Punkty otwarte `[ ]` tych sekcji mają teksty dzisiejszego
> programu. Sekcja 14 opisuje program sprzed nieba: liczby testów i liczba programów w jej
> punktach `[x]` to stan po M5. Sekcje od 11 do 15 powstały, gdy podłogą labiryntu były
> płaskie płytki (model `floor_tile.obj` z teksturą `floor_stone.png`): druga część M6
> usunęła je i zastąpiła terenem z mapy wysokości, więc słowo "podłoga" w punktach `[x]`
> tych sekcji oznacza tamte płytki, a plików o tych nazwach już nie ma. To, co program
> pokazuje dziś, opisują sekcje 2, 14, 15 i 16, koniec klatki (bufor HDR, bloom, mgłę
> i winietę) sekcje 17, 18 i 19, cienie księżyca sekcja 20, cień latarki i latarkę w ręce
> sekcja 21, minimapę sekcja 22, environment mapping (M8, część 1) sekcja 23, a selekcję, dźwignie i kartki (M8, część 2) sekcja 24, kamerę menu i przełączniki wiersza poleceń (M9, część 1) sekcja 25.

## 1. Wymagania

| Narzędzie | Po co | Uwagi |
|---|---|---|
| Visual Studio 2022 albo same "Build Tools for Visual Studio 2022", z pakietem roboczym "Desktop development with C++" (Programowanie aplikacji klasycznych w C++) | kompilator MSVC, Windows SDK, MSBuild, dołączone CMake i Ninja | do pracy z terminala wystarczają same Build Tools (tak było na moim PC). IDE jest potrzebne tylko do sekcji 4. Alternatywa: CLion |
| git dostępny w `PATH` | CMake klonuje nim GLFW, GLM, ImGui, doctest, stb, FreeType i RmlUi podczas konfiguracji | sprawdzenie: `git --version` w nowym oknie terminala. Instalator: <https://git-scm.com/> |
| CMake w wersji co najmniej 3.24 | konfiguracja i build | jest częścią pakietu roboczego C++ (u mnie 3.31.6-msvc6). Osobny instalator: <https://cmake.org/download/> |

Środowisko, na którym wykonałem pomiary z tego dokumentu (2026-10-05 i 2026-10-05):

| Element | Wersja |
|---|---|
| System | Windows 11 Pro 10.0.26200 |
| Narzędzia | Visual Studio Build Tools 2022 17.14.37516, pakiet roboczy C++ (bez IDE) |
| Kompilator | MSVC `cl` 19.44.35228 (zestaw narzędzi 14.44.35207) |
| Windows SDK | 10.0.26100.0 |
| MSBuild | 17.14.51 |
| CMake | 3.31.6-msvc6 (dołączony do Build Tools) |
| Karta graficzna | NVIDIA GeForce RTX 4070 Ti SUPER (oraz zintegrowana AMD Radeon) |

Uwagi:

- Instalacja Visual Studio albo Build Tools nie dodaje `cmake` do `PATH` zwykłego terminala
  (potwierdzone: w zwykłym terminalu polecenie `cmake` nie istnieje). Są dwie drogi: wejść w
  środowisko deweloperskie (sekcja 2) albo zainstalować CMake osobno z opcją dodania do
  `PATH`.
- Git jest potrzebny w tym samym terminalu, w którym uruchamiamy CMake. Jeśli
  `git --version` nie działa, konfiguracja zakończy się błędem przy pobieraniu GLFW.
- Bibliotek nie instalujemy ręcznie. GLFW, GLM, ImGui, doctest, stb (dla stb_image), FreeType i RmlUi pobiera
  CMake, GLAD jest w repozytorium.
- Sterownik karty graficznej musi obsługiwać OpenGL 4.1 lub nowszy. Aktualne sterowniki
  kart NVIDIA, AMD i Intel obsługują 4.6. Przy bardzo starym sterowniku okno się nie utworzy.

## 2. Budowanie z terminala

Te same presety co na Macu. Polecenia wykonujemy w katalogu głównym repozytorium, w
terminalu ze środowiskiem deweloperskim Visual Studio.

### Środowisko deweloperskie

Do środowiska wchodzi się skrótem "Developer PowerShell for VS 2022" z menu Start albo, w
dowolnym oknie PowerShell, skryptem, który ten skrót uruchamia. Tak robiłem przy pomiarach:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
```

- Ścieżka dotyczy samych Build Tools. Przy zainstalowanym IDE zamiast `BuildTools` jest
  nazwa edycji, na przykład `Community`.
- Skrypt dopisuje do `PATH` kompilator `cl.exe`, MSBuild oraz dołączone CMake i Ninja. Działa
  tylko w tym oknie terminala.
- Zawsze podawałem `-Arch amd64 -HostArch amd64`, czyli kompilator 64 bitowy. Jak zachowuje
  się powłoka bez tych opcji, nie mierzyłem. Ma to znaczenie dla generatora Ninja, który
  używa tego `cl.exe`, który akurat jest w `PATH` (sekcja 4). Generator Visual Studio sam
  wybiera kompilator.

### Debug

```bat
cmake --preset debug
cmake --build --preset debug
build\debug\Debug\night_maze.exe
```

### Release

```bat
cmake --preset release
cmake --build --preset release
build\release\Release\night_maze.exe
```

Zwróć uwagę na dodatkowy katalog `Debug` lub `Release` w ścieżce programu. Na Macu program
leży w `build/debug/night_maze`. Wyjaśnienie w następnej sekcji.

### Przełączniki wiersza poleceń (od M9, części 1)

Program przyjmuje pięć przełączników (piąty, `--play`, doszedł w M9, części 2). Są czytane przed otwarciem okna, a błędny kończy program dwiema liniami `[error]` (komunikat i lista przełączników) i niezerowym kodem wyjścia.

| Przełącznik | Znaczenie |
|---|---|
| `--seed <liczba>` | ziarno pierwszego labiryntu, liczba całkowita od 0 do 4294967295 (domyślnie 1) |
| `--play` | start od razu w rundzie, z pominięciem menu głównego (dla testów, skryptów i nagrywania; bez wartości) |
| `--menu-camera` | start od razu w trybie kamery menu (gra pokazuje samą siebie; to samo robi klawisz F2, który od części 2 działa tylko w rundzie); pomija też menu główne |
| `--menu-shot <walk\|glide>` | ujęcie kamery menu: spacer po korytarzach albo wysoki przelot. Samo nie włącza trybu |
| `--menu-time <sekundy>` | start ujęcia tyle sekund w głąb pętli, liczba z kropką dziesiętną (przecinek jest odrzucany), może być ujemna. Samo nie włącza trybu |

Przykład: `build\debug\Debug\night_maze.exe --menu-camera --seed 1 --menu-shot glide`. Opis i przepis na nagranie klipu: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), sekcje 2.9 i 5.9.

### Co powinno się pojawić (zmierzone)

Słowo "zmierzone" w tytule dotyczy stanu do M4. To, co lista mówi o M5 (kryształy,
brama, pasek HUD i jego napisy, panel Gameplay, liczby modeli i tekstur
w liniach `[info]`), wynika z kodu, a nie z pomiaru: dla M5 zmierzone są build, testy
jednostkowe i obraz na zrzutach, których listy nie zapisano (sekcja 14.1). Tak samo jest
z tym, co lista mówi o M6 (niebo, nierówne podłoże, trawa) i o M7 (dwanaście
paneli, osiem programów po pierwszej części, dziesięć po drugiej i jedenaście po czwartej,
nowe wartości startowe świateł i koloru tła, poświata kryształów, cienie księżyca): wynika
z kodu i ze zgłoszonych pomiarów (sekcje 15.1, 16.1, 17.1, 18.1 i 20.1).

- `cmake --preset debug` kończy się bez błędów. Wypisuje jedno ostrzeżenie o nieużytej
  zmiennej `CMAKE_BUILD_TYPE`. Jest ono oczekiwane (sekcja 3).
- `cmake --build --preset debug` i `cmake --build --preset release` kończą się kodem 0, bez
  ostrzeżeń i bez błędów. W wyjściu buildu jest linia `Copying assets next to the
  executable` (sekcja 7).
- Program otwiera okno z nocnym widokiem z wnętrza labiryntu: łagodnie nierówne podłoże
  z teksturą ubitej ziemi z mchem i kamykami (teren z mapy wysokości, od drugiej części
  M6 w miejscu płaskich płytek podłogi), ściany i słupki z teksturą kamienia, wzdłuż ścian
  kępki trawy kołysane wiatrem, wszystko oświetlone (tryb startowy to Blinn-Phong). Poza
  labiryntem podłoże przechodzi we wzgórza. Świecą trzy rodzaje świateł:
  słabe, chłodne światło księżyca (kierunkowe), ciepły stożek latarki gracza na środku
  obrazu (reflektor) i turkusowe światła punktowe nad kryształami. Kryształ to mały model,
  który unosi się na środku komórki, kołysze się, obraca i sam świeci (13 w labiryncie
  startowym). Miejsca, do których żadne światło nie
  dociera, są ciemne, ale nie czarne
  (światło otoczenia). Od czwartej części M7 ściany, słupki, brama, kryształy i wzgórza
  rzucają cień w świetle księżyca (sekcja 20), więc na podłożu i na ścianach widać granice
  światła i cienia. Od piątej części M7 (2026-10-06) cień rzuca też latarka, trzymana w
  ręce, a nie w oku (sekcja 21). Światła kryształów cieni nie rzucają: świecą przez ściany.
  Nad ścianami jest od pierwszej części M6 nocne niebo z gwiazdami i księżycem (skybox,
  sekcja 15). Prawie czarne, granatowe tło widać tylko po odznaczeniu pola `Skybox`: kolor
  czyszczenia to od M7
  `{0.022F, 0.033F, 0.088F}` (liczby sRGB, przeliczane na liniowe przed `glClearColor`, od
  M4 do M6 `{0.01F, 0.015F, 0.04F}`, przed M4 `{0.02F, 0.03F, 0.08F}`). Kolorowej
  kostki z M1, która do M4 wisiała nad komórką w przeciwległym rogu labiryntu, już nie
  ma: M5 ją usunął. Przy komórce wyjścia stoi drewniana brama. U góry okna, na środku,
  jest pasek HUD: napis `Crystals`, liczby `0 / 10` i `(of 13)`, czas rundy `0:00` i pod
  nimi pasek baterii z napisem `100%`. Widok startowy z M4 jest sprawdzony na zrzucie
  ekranu (sekcja 13.1). Obraz po M5 był oglądany na zrzutach, których listy nie zapisano
  (sekcja 14.1). W terminalu
  pierwsze dwie linie to:

  ```text
  [info] GL_VERSION:  4.1.0 NVIDIA 610.74
  [info] GL_RENDERER: NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2
  ```

  Napisy zależą od karty i sterownika. Sterownik NVIDII oddał kontekst dokładnie w wersji
  4.1, o którą prosi program. Komputer ma też zintegrowaną kartę AMD Radeon: system sam
  wybrał kartę NVIDIA. Po nich pamięć podręczna assetów wypisuje po jednej linii `[info]`
  na każdy wczytany plik (`Loaded texture: ...` dla ośmiu tekstur, czyli czterech obrazów
  koloru i czterech map normalnych: kamienia ścian, kryształu, drewna bramy i podłoża, oraz
  `Loaded model: ...` dla pięciu modeli:
  ściany, słupka, dwóch kryształów i bramy, co wynika z
  kodu `assets::AssetCache` i z plików `.mtl`. Do M5 modeli było sześć, szóstym była płytka
  podłogi). Od M6 są też linie spoza pamięci podręcznej: sześć linii
  `Loaded sky face: ...` i jedna `Loaded heightmap: ...` z plikiem `heightmap.png`.
  Zmierzone jest to, że na starcie
  nie ma żadnej linii `[error]`, w tym żadnej z nazwą błędu OpenGL (`GL_INVALID_...`), także
  po dodaniu oświetlenia (2026-10-05, stan M4). Dla M5 tego wyniku nie zapisano (punkt
  otwarty w sekcji 14.2). Dokładnej liczby linii `[info]` przy tych pomiarach
  nie zapisałem.
- Widocznych jest trzynaście paneli w ciemnym, granatowym motywie: Renderer nad Lights w lewej
  kolumnie, Maze nad Assets w prawej, Collision i Shaders na dole między kolumnami, a
  Camera i Gameplay u góry, między kolumnami, obok siebie, zwinięte do samych pasków tytułu
  (panel rozwija kliknięcie strzałki w jego pasku). Tuż pod nimi jest drugi rząd pasków
  tytułu: Terrain pod Camera i Grass pod Gameplay, też zwinięte (od drugiej części M6).
  Pod nimi jest trzeci rząd: jeden szeroki pasek Framebuffers, też zwinięty (od pierwszej
  części M7), a pod nim czwarty: pasek Shadows tej samej szerokości, też zwinięty (od
  czwartej części M7), a pod nim piąty: pasek Environment tej samej szerokości, też zwinięty (od
  pierwszej części M8). Zwiniętych paneli jest więc siedem. Pasek HUD stoi pod tymi pięcioma
  rzędami
  pasków tytułu i nie jest panelem: nie znika razem z panelami i nie reaguje na mysz.
  Każdy panel ma w kodzie miejsce i rozmiar startowy, ułożone
  dla okna 1280 x 720 (`src/debug/PanelLayout.hpp`). Działają one tylko wtedy, gdy w
  katalogu roboczym nie ma pliku `imgui.ini` z wpisem danego panelu (sekcja 7). To nie jest
  cecha Windowsa.
  Od 2026-10-06 ten opis nie jest aktualny: trzynaście paneli zastąpiło jedno okno debug, które startuje ukryte (`~`) i stoi przy
  prawej krawędzi pod paskiem HUD, bez zwiniętych pasków i bez `PanelLayout` (sekcja 27). Opis powyżej zostaje jako zapis
  pomiaru z dnia, w którym go zapisano.
- Tekst okna debug (od 2026-10-06 14 pikseli, HUD ma 16) jest w czcionce Atkinson Hyperlegible z pliku
  `assets\fonts\AtkinsonHyperlegible-Regular.ttf` w kopii katalogu `assets` obok programu.
  Gdy tego pliku brakuje, w konsoli jest jedna linia
  `[error] Panel font cannot be loaded, using the built-in font: ...`, a panele używają
  czcionki wbudowanej w ImGui (zmierzone).

Opis samego pliku presetów (ukryty preset `base`, `inherits`, `binaryDir`) jest w
[`build-macos.md`](build-macos.md), sekcja 3. Plik jest wspólny dla obu systemów.

### Testy jednostkowe

Zwykły build buduje też program testowy `night_maze_tests.exe` (kolizje, labirynt, gracz,
loadery, od M4 także tekst shaderów z `#include`, matematyka świateł, ustawienia
oświetlenia, macierz normalnych i styczne wierzchołków, od M5 kule kolizji, wyjście,
kryształy i reguły rundy, od M6 pliki nieba, teren i miejsca kępek trawy, a od M7 funkcje
przeliczające sRGB, teksty stanu framebuffera oraz matematyka bloomu, mgły, winiety i mapy
cieni: kod bez okna). Testy uruchamia `ctest`, program z pakietu CMake, dostępny w tym
samym środowisku deweloperskim:

```bat
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```

- `-C Debug` jest **wymagane** z generatorem Visual Studio: jeden katalog buildu mieści tu
  kilka konfiguracji (sekcja 3) i `ctest` musi wiedzieć, którą uruchomić. Zmierzone bez tego
  argumentu: `Test not available without configuration.  (Missing "-C <config>"?)`, wynik
  `***Not Run`, kod wyjścia 8.
- `--output-on-failure` wypisuje raport programu testowego, gdy test nie przejdzie.
- `ctest` niczego nie buduje. Po zmianie kodu najpierw `cmake --build --preset debug`.

Zmierzone 2026-10-05 (po czystym buildzie obu presetów, bez ostrzeżeń, przed dodaniem testów
oświetlenia: wyjścia `ctest` z 2026-10-05 nie zapisałem, inny może być w nim tylko
czas):

```text
    Start 1: night_maze_tests
1/1 Test #1: night_maze_tests .................   Passed    0.40 sec

100% tests passed, 0 tests failed out of 1
```

Dla `ctest` cały program jest jednym testem. Szczegóły pokazuje sam program:

```bat
build\debug\Debug\night_maze_tests.exe
```

```text
[doctest] doctest version is "2.5.3"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:    256 |    256 passed | 0 failed | 0 skipped
[doctest] assertions: 101232 | 101232 passed | 0 failed |
[doctest] Status: SUCCESS!
```

Te same liczby daje `build\release\Release\night_maze_tests.exe` (oba pomiary 2026-10-05,
po drugiej części M6: trzy ostatnie linie są przepisane z wyjścia programu, dwóch
pierwszych wtedy nie zapisałem i zostały z wcześniejszego raportu).
Przypadki w plikach:
`ColliderTests.cpp` 19, `CrystalTests.cpp` 14, `ExitTests.cpp` 11, `GrassTests.cpp` 9,
`ImageLoaderTests.cpp`
10, `LightingTests.cpp` 10, `LightTests.cpp` 20, `MazeGeneratorTests.cpp` 11,
`MazeLayoutTests.cpp` 12, `MazeTests.cpp` 8, `MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp`
19, `PlayerTests.cpp` 13, `RoundTests.cpp` 25, `ShaderSourceTests.cpp` 22,
`SkyboxTests.cpp` 5, `TangentTests.cpp` 9, `TerrainTests.cpp` 27, `TransformTests.cpp` 4,
razem 256 (liczby na plik policzone z makr `TEST_CASE` w kodzie testów). Po
kroku łączącym M2 + M3 program miał osiem plików z testami, po M4 trzynaście (163
przypadki i 62220 asercji, sekcja 13), po M5 szesnaście (215 i 85098, sekcja 14), po
pierwszej części M6 siedemnaście (221 i 85175, sekcja 15), po drugiej dziewiętnaście (256
i 101232, sekcja 16: lista wyżej). Po pierwszej części M7 miał dwadzieścia jeden:
doszły `ColorSpaceTests.cpp` (9 przypadków) i `FramebufferTests.cpp` (3),
a `LightingTests.cpp` ma 11 (269 przypadków i 102103 asercje, sekcja 17.1). Po
drugiej części M7 miał dwadzieścia dwa: doszedł `BloomTests.cpp` (7 przypadków, razem
276 przypadków i 102139 asercji, sekcja 18.1). Po trzeciej części M7 miał
dwadzieścia cztery: doszły `FogTests.cpp` (11 przypadków) i `VignetteTests.cpp` (7, razem
294 przypadki i 102412 asercji, sekcja 19.1). Dziś, po czwartej części M7, ma dwadzieścia
pięć: doszedł `ShadowTests.cpp` (16 przypadków). Zgłoszone liczby dla tego stanu to
310 przypadków i 103751 asercji (sekcja 20.1). Dziś, po piątej części M7, plików jest nadal
dwadzieścia pięć: `ShadowTests.cpp` ma 31 przypadków, `LightingTests.cpp` 15, a zgłoszone
liczby to **329 przypadków i 104306 asercji** (sekcja 21.1). Blok wyjścia programu wyżej jest zapisem
z drugiej części M6: nowego wyjścia nie przepisywałem, bo sam go nie uruchamiałem.
Program testowy nie otwiera okna. Opis biblioteki, makr i opcji programu:
[`../libraries/doctest.md`](../libraries/doctest.md).

Plik [`Makefile`](../../Makefile) ze skrótami (`make run`, `make check`) działa też na
Windowsie, pod dwoma warunkami. Po pierwsze potrzebny jest program `make` (u mnie GNU Make
4.4.1 zainstalowany przez scoop). Po drugie `make` trzeba uruchamiać w środowisku
deweloperskim z sekcji 2: w zwykłym terminalu nie ma `cmake` ani `ctest`, więc każdy cel
kończy się błędem `CreateProcess(NULL, cmake --preset debug, ...) failed`. Powłoka typu Unix
nie jest potrzebna: plik działa z PowerShella i z Git Basha uruchomionego z tego środowiska
(`& "C:\Program Files\Git\bin\bash.exe"`, samo `bash` to u mnie WSL).

Stan z 2026-10-05: `make -n` (przebieg na sucho) przechodzi dla wszystkich dwunastu celów w
obu powłokach, a `make test` został naprawdę uruchomiony i przeszedł w obu, na kopii
ostatniego commita. Cele `run`, `run-release`, `format`, `test-release` oraz pełne `tidy`
i `check` nie były jeszcze uruchamiane na Windowsie. Cel `tidy` konfiguruje tu dodatkowy
katalog `build\ninja-debug` generatorem Ninja, bo generator Visual Studio nie zapisuje
`compile_commands.json` (sekcja 3). `make help` z PowerShella wypisuje tekst razem ze
znakami cudzysłowu: to tylko kosmetyka. Opis pliku:
[`project-structure.md`](project-structure.md), sekcja 3.12.

## 3. Generator Visual Studio jest wielokonfiguracyjny

To najważniejsza różnica względem Maca i częste pytanie na przeglądzie kodu.

**Generator** to część CMake, która tworzy pliki dla konkretnego systemu budowania. Nasze
presety nie wskazują generatora, więc CMake wybiera domyślny dla platformy:

| | macOS | Windows z Visual Studio 2022 albo Build Tools 2022 |
|---|---|---|
| Domyślny generator | Unix Makefiles | Visual Studio 17 2022 |
| Rodzaj | jednokonfiguracyjny (single-config) | wielokonfiguracyjny (multi-config) |
| Kiedy wybieramy Debug lub Release | przy **konfiguracji**, zmienną `CMAKE_BUILD_TYPE` | przy **budowaniu**, opcją `--config` |
| Co zawiera katalog buildu | pliki dla jednej konfiguracji | rozwiązanie `.sln` ze wszystkimi konfiguracjami naraz |
| Ścieżka programu | `build/debug/night_maze` | `build\debug\Debug\night_maze.exe` |

Kolumna Windows jest zmierzona w dwóch miejscach: po `cmake --preset debug` z terminala wpis
`CMAKE_GENERATOR` w `build\debug\CMakeCache.txt` ma wartość `Visual Studio 17 2022`, a
program powstaje w `build\debug\Debug\night_maze.exe`.

Generator jednokonfiguracyjny ustala typ buildu raz, podczas `cmake --preset debug`.
Generator wielokonfiguracyjny tworzy projekt, który zna wszystkie konfiguracje (Debug,
Release, RelWithDebInfo, MinSizeRel), a wybór następuje dopiero przy budowaniu. Żeby wyniki
się nie nadpisywały, każda konfiguracja dostaje własny podkatalog, stąd `Debug\` w ścieżce.

Konsekwencje dla naszego `CMakePresets.json`:

```json
"cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
```

Generator Visual Studio **ignoruje `CMAKE_BUILD_TYPE`**. Ta linia działa na Macu, na
Windowsie nie robi nic. Widać to w wyjściu konfiguracji: CMake wypisuje ostrzeżenie
`Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE`. To ostrzeżenie
jest oczekiwane i niczego nie trzeba z nim robić.

```json
"buildPresets": [
    { "name": "debug", "displayName": "Debug", "configurePreset": "debug", "configuration": "Debug" },
```

Pole **`configuration` w presecie budowania** wybiera konfigurację na Windowsie.
`cmake --build --preset debug` jest tam równoważne `cmake --build build\debug --config Debug`.
Na Macu z kolei to pole jest ignorowane.

Dlatego presety ustawiają **obie** rzeczy: `CMAKE_BUILD_TYPE` dla generatorów
jednokonfiguracyjnych i `configuration` dla wielokonfiguracyjnych. Ten sam plik działa
poprawnie na obu systemach, a każdy system korzysta z "swojej" połowy.

Skutek uboczny wart zrozumienia: na Windowsie katalogi `build\debug` i `build\release` są
technicznie takim samym projektem Visual Studio. O tym, co powstanie, decyduje preset
budowania, a nie preset konfiguracji. Trzymamy się jednak pary `debug` z `debug` i `release`
z `release`, żeby polecenia były identyczne jak na Macu.

Jeszcze jedna różnica: `CMAKE_EXPORT_COMPILE_COMMANDS` działa tylko z generatorami Makefile
i Ninja. Generator Visual Studio nie tworzy `compile_commands.json`. Visual Studio go nie
potrzebuje, ale clangd i clang-tidy na Windowsie wymagają generatora Ninja (podsekcja
"Cursor i VS Code z clangd na Windowsie" w sekcji 4).

## 4. Otwieranie folderu w Visual Studio

**Ta sekcja nie została sprawdzona.** Na moim PC są same Build Tools, bez IDE, więc kroków z
Visual Studio (Open Folder, wybór elementu startowego, F5, Build Solution) nie mogłem
wykonać. Opis wynika z dokumentacji narzędzi.

Visual Studio 2022 ma wbudowaną obsługę CMake i presetów. Nie tworzymy ręcznie pliku `.sln`.

1. File, Open, Folder i wskazujemy katalog repozytorium (albo "Open a local folder" na
   ekranie startowym).
2. Visual Studio wykrywa `CMakeLists.txt` i `CMakePresets.json` i uruchamia konfigurację.
   Postęp i błędy widać w oknie Output (lista "CMake").
3. Na pasku narzędzi pojawiają się listy rozwijane: preset konfiguracji (`Debug` lub
   `Release`, czyli nasze `displayName`) i preset budowania.
4. Jako element startowy (Select Startup Item) wybieramy `night_maze.exe`.
5. F5 uruchamia z debuggerem, Ctrl+F5 bez niego.

Do sprawdzenia przy pierwszym uruchomieniu w IDE:

- **Którego generatora użyje Visual Studio.** Nasz preset nie podaje pola `generator`.
  Z terminala CMake wybiera generator Visual Studio (zmierzone, sekcja 3). Otwierając folder
  w IDE, Visual Studio może zastosować własny wybór generatora, w tym Ninja, którą ma w
  zestawie. Z Ninja build jest jednokonfiguracyjny: działa `CMAKE_BUILD_TYPE`, a program
  leży bezpośrednio w katalogu buildu, bez podkatalogu `Debug`. Presety są przygotowane na
  oba przypadki, ale ścieżka programu będzie inna. Użyty generator widać w pierwszych
  liniach okna Output oraz w `build\debug\CMakeCache.txt` (wpis `CMAKE_GENERATOR`).
- **Nie mieszać generatorów w jednym katalogu.** Jeśli `build\debug` utworzył terminal
  jednym generatorem, a IDE spróbuje użyć innego, CMake zgłosi błąd o niezgodności
  generatora. Rozwiązanie: usunąć `build\debug` i trzymać się jednego sposobu pracy.

Katalogi `.vs/` i `out/` (domyślne katalogi robocze Visual Studio) są w `.gitignore`.

### Cursor i VS Code z clangd na Windowsie

Repozytorium zawiera konfigurację edytora wspólną dla obu systemów: [`.clangd`](../../.clangd),
[`.vscode/settings.json`](../../.vscode/settings.json) i
[`.vscode/extensions.json`](../../.vscode/extensions.json) (opis kluczy w
[`project-structure.md`](project-structure.md), sekcje 3.9 do 3.11, przebieg na Macu w
[`build-macos.md`](build-macos.md), sekcja 6).

Na Windowsie jest jedna istotna różnica. Plik `.clangd` wskazuje na
`build/debug/compile_commands.json`, a generator Visual Studio, którego CMake używa
domyślnie z terminala, **tego pliku nie tworzy** (sekcja 3). Skutek: po zwykłym
`cmake --preset debug` clangd nadal nie zna flag kompilacji, a edytor pokazuje czerwone
błędy "file not found" przy każdym `#include`, mimo że projekt buduje się poprawnie.

Żeby clangd działał, katalog `build\debug` musi być skonfigurowany generatorem Ninja:

```powershell
cmake --preset debug -G Ninja
cmake --build --preset debug
```

Uwagi do tych poleceń:

- Trzeba je wykonać w środowisku deweloperskim (sekcja 2), żeby kompilator MSVC (`cl.exe`) i
  Ninja dołączona do narzędzi Visual Studio były w `PATH`. Ninja używa tego `cl.exe`, który
  znajdzie w `PATH`, więc architekturę wybiera opcja `-Arch` skryptu (u mnie `amd64`).
- Jeden katalog buildu to jeden generator. Jeśli `build\debug` powstał wcześniej generatorem
  Visual Studio, trzeba go najpierw usunąć.
- Z Ninja build jest jednokonfiguracyjny, więc program leży w `build\debug\night_maze.exe`,
  bez podkatalogu `Debug`.
- Do czasu wykonania konfiguracji edytor pokazuje błędy "file not found", tak samo jak na
  Macu.

**Co zmierzyłem.** Generator Ninja sprawdziłem w osobnym katalogu, żeby nie usuwać katalogu
`build\debug` utworzonego generatorem Visual Studio:

```powershell
cmake --preset debug -G Ninja -B build\ninja-debug
cmake --build build\ninja-debug
```

Wynik: 52 kroki budowania, zero ostrzeżeń, plik `build\ninja-debug\compile_commands.json`
powstał, program leży w `build\ninja-debug\night_maze.exe` (bez podkatalogu `Debug`), a
katalog `assets` został skopiowany obok niego. Wariant z usunięciem `build\debug` różni się
tylko katalogiem, ale dla clangd potrzebny jest właśnie on, bo `.clangd` wskazuje na
`build/debug/compile_commands.json`.

**Czego nie zmierzyłem.** Czy clangd w edytorze poprawnie czyta polecenia kompilatora MSVC z
tego pliku (czyli czy błędy "file not found" znikają) i czy rozszerzenie CodeLLDB z listy
rekomendacji nadaje się do debugowania programu zbudowanego przez MSVC (jeśli nie, zostaje
debugger Visual Studio). Oba punkty są na liście kontrolnej w sekcji 11.

### CLion na Windowsie

Niesprawdzone. CLion też czyta `CMakePresets.json`. Dwie rzeczy do ustawienia:

- W Settings, Build, Execution, Deployment, Toolchains wybrać toolchain Visual Studio, a nie
  dołączony MinGW. Z MinGW kompilatorem jest GCC, więc w `CMakeLists.txt` zadziała gałąź
  `else()` z flagami `-Wall -Wextra -Wpedantic` zamiast `/W4`. To też powinno działać, ale
  jest inną konfiguracją niż ta opisana tutaj.
- Włączyć profile CMake z presetów zamiast domyślnego `cmake-build-debug`.

## 5. Ostrzeżenia: `/W4 /permissive-`

Fragment [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# Strict warnings for our own targets only (third-party code is built with its defaults).
function(night_maze_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
```

- `if(MSVC)` jest prawdą, gdy kompilatorem jest Microsoft Visual C++. Flagi MSVC mają inną
  składnię (zaczynają się od `/`), więc potrzebne jest rozgałęzienie.
- **`/W4`**: poziom ostrzeżeń 4, najwyższy rozsądny w codziennej pracy. Odpowiednik
  `-Wall -Wextra`. Istnieje też `/Wall`, ale włącza ostrzeżenia w nagłówkach systemowych i
  jest w praktyce nieużywalny.
- **`/permissive-`**: tryb ścisłej zgodności ze standardem C++. MSVC historycznie akceptował
  konstrukcje niezgodne ze standardem, a ta flaga je wyłącza. Odpowiednik `-Wpedantic` w tym
  sensie, że kod, który przejdzie na MSVC, ma większą szansę skompilować się w clang, i
  odwrotnie. To ważne w projekcie na dwa systemy.
- `PRIVATE`: flagi dotyczą tylko wskazanego targetu i nie przenoszą się na jego użytkowników.
- Funkcję wywołujemy tylko dla naszych czterech targetów: `engine`, `game_logic`,
  `night_maze` i `night_maze_tests`. GLFW, ImGui, GLAD i stb_image kompilują się ze swoimi
  domyślnymi ustawieniami. GLM i doctest nie mają własnych plików do skompilowania (same nagłówki).

Nagłówki bibliotek są oznaczone jako systemowe (`SYSTEM` w `target_include_directories`,
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` dla GLFW i GLM). Na Macu daje to `-isystem`. Na
Windowsie CMake i MSVC realizują to samo opcją `/external:I` i wyłączeniem ostrzeżeń dla
takich katalogów. Zmierzone w wygenerowanym projekcie: katalogi `external/glad/include`,
`_deps/glfw-src/include` i `_deps/glm-src` trafiają do kompilatora przez `/external:I`, a
ustawienie `ExternalWarningLevel` ma wartość `TurnOffAllWarnings`. Z tych nagłówków nie
pojawia się pod `/W4` żadne ostrzeżenie. To samo jest zmierzone dla doctest w projekcie
`night_maze_tests`: katalogi `_deps/doctest-src` i `_deps/doctest-src/doctest` trafiają do
kompilatora przez `/external:I`, choć w `Dependencies.cmake` nie ma dla nich naszego kroku
(nagłówek oznacza jako systemowy `CMakeLists.txt` samego doctest,
[`../libraries/doctest.md`](../libraries/doctest.md), sekcja 2). Dla GLM ma to największe znaczenie, bo cały kod tej
biblioteki kompiluje się wewnątrz naszych plików ([`../libraries/glm.md`](../libraries/glm.md),
sekcja 4, pułapka 14).

Standard C++20 ustawia `set(CMAKE_CXX_STANDARD 20)` razem z `CMAKE_CXX_EXTENSIONS OFF`. Na
MSVC przekłada się to na flagę `/std:c++20`.

Definicja `GL_SILENCE_DEPRECATION` na Windowsie nie ma żadnego efektu. Dotyczy tylko
nagłówków Apple. Zostaje, bo jedna lista definicji dla obu systemów jest prostsza.

Cel: **zero ostrzeżeń pod `/W4`** w naszym kodzie. MSVC zgłasza czasem inne ostrzeżenia niż
clang (na przykład o zawężających konwersjach typów), więc pierwszy build mógł ujawnić
miejsca, których Mac nie pokazał. Nie ujawnił: build Debug i build Release całego `src/`
przeszły w MSVC 19.44 bez żadnego ostrzeżenia i bez zmian w kodzie.

## 6. Okno konsoli

Program jest zdefiniowany jako:

```cmake
add_executable(night_maze
    src/main.cpp
    ...
)
```

Bez słowa `WIN32` w `add_executable` powstaje aplikacja konsolowa: punktem wejścia jest
zwykłe `int main()` (tak jak w [`src/main.cpp`](../../src/main.cpp)), a przy uruchomieniu
obok okna gry otwiera się okno konsoli.

To jest zamierzone. Do konsoli trafiają komunikaty z `core::Log`:

```text
[info] GL_VERSION:  ...
[info] GL_RENDERER: ...
```

oraz błędy z `GL_CHECK` i z callbacku błędów GLFW. Bez konsoli nie byłoby ich gdzie zobaczyć.

- Uruchomienie z terminala: komunikaty pojawiają się w tym samym terminalu (zmierzone,
  dokładne linie w sekcji 2).
- Uruchomienie dwuklikiem lub z Visual Studio: otwiera się osobne okno konsoli. Zamknięcie
  go krzyżykiem zabija program. Tego sposobu uruchamiania nie sprawdzałem.
- Jeśli program kończy się błędem przy starcie, konsola otwarta dwuklikiem zniknie od razu.
  Wtedy uruchamiamy z terminala, żeby przeczytać linię `[error] Fatal: ...`.

## 7. Katalog roboczy, `imgui.ini` i katalog `assets`

Dear ImGui zapisuje układ paneli w pliku `imgui.ini` w **katalogu roboczym (working
directory)** procesu, a nie obok pliku `.exe`. Od 2026-10-06 zapisuje tam już tylko przypięty panel okna debug i stan
dokowania (sekcja 27): samo okno debug ma miejsce i rozmiar liczone w każdej klatce.

| Sposób uruchomienia | Katalog roboczy | Gdzie powstanie `imgui.ini` |
|---|---|---|
| `build\debug\Debug\night_maze.exe` z katalogu repozytorium | katalog repozytorium | w katalogu głównym repozytorium |
| dwuklik na `night_maze.exe` | katalog z plikiem `.exe` | `build\debug\Debug\` |
| Visual Studio (F5) | ustawiany przez IDE, zwykle katalog pliku wykonywalnego | do sprawdzenia |

Ta tabela jest na Windowsie w większości przewidywaniem. Zmierzony jest jeden przypadek:
program uruchomiony z katalogiem roboczym `build\debug\Debug` i zabity po siedmiu sekundach
zostawił `imgui.ini` w tym katalogu. ImGui zapisuje plik także w trakcie działania, kilka
sekund po zmianie układu, a nie tylko przy zamykaniu. Przy wcześniejszych pomiarach program
był zabijany wcześniej i plik nie powstawał. Wiersze o dwukliku i o Visual Studio pozostają
przewidywaniem.

> Od 2026-10-06 nie istnieją `PanelLayout`, `placePanelOnFirstUse` ani trzynaście paneli. Przypięty panel dostaje miejsce
> z kodu (320 x 460 pikseli przy prawej krawędzi, pod paskiem HUD) tylko wtedy, gdy `imgui.ini` nie ma jego wpisu
> (`ImGuiCond_FirstUseEver`). Akapit poniżej opisuje układ do 2026-10-05.

Bez tego pliku trzynaście paneli otwiera się w układzie zapisanym w kodzie (stałe
`..._PLACEMENT` w `src/debug/PanelLayout.hpp`, funkcja `placePanelOnFirstUse`, warunek
`ImGuiCond_FirstUseEver`). Ten sam warunek obejmuje trzy rzeczy: pozycję, rozmiar i to, czy
panel startuje zwinięty do paska tytułu (`ImGui::SetNextWindowCollapsed`). Zwinięte startuje
siedem paneli: Camera i Gameplay (pole `collapsed = true` w stałych `CAMERA_PLACEMENT` i
`GAMEPLAY_PLACEMENT`), a od drugiej części M6 także Terrain i Grass w drugim rzędzie pod
nimi (`TERRAIN_PLACEMENT` i `GRASS_PLACEMENT`, z polem `foldedRowsBefore = 1`), od
pierwszej części M7 Framebuffers w trzecim rzędzie (`FRAMEBUFFERS_PLACEMENT`,
`foldedRowsBefore = 2`), a od czwartej części M7 Shadows w czwartym (`SHADOWS_PLACEMENT`,
`foldedRowsBefore = 3`), a od pierwszej części M8 Environment w piątym
(`ENVIRONMENT_PLACEMENT`, `foldedRowsBefore = 4`). Gdy plik istnieje
i ma wpis panelu, wygrywa wpis: także stan zwinięcia jest potem brany z pliku. Plik
`imgui.ini` zapisany przez program sprzed M4 ma wpisy sześciu paneli w starym układzie
(Camera pod Rendererem) i nie ma wpisu panelu Lights. Z takim plikiem sześć paneli zostaje
na starych miejscach, a tylko nowy panel Lights dostaje miejsce z kodu, czyli lewą kolumnę
pod Rendererem, gdzie w starym układzie stoi Camera: panele nachodzą na siebie. Plik
zapisany przez program z M4 ma wpisy siedmiu paneli i nie ma wpisu panelu Gameplay: ten
jeden panel dostaje miejsce z kodu, u góry, na prawo od paska panelu Camera, gdzie w
układzie z M4 nic nie stoi, a dolny rząd (Collision i Shaders) zostaje o 8 pikseli niższy
niż w dzisiejszym układzie. Oba opisy to wnioski
z kodu, nie obserwacje. Żeby obejrzeć układ domyślny, trzeba plik usunąć przed
uruchomieniem (sekcja 14.2). Paska HUD plik nie dotyczy: jego okna mają flagę
`ImGuiWindowFlags_NoSavedSettings`.

Plik jest w `.gitignore`, więc nigdzie nie przeszkadza w repozytorium. Skutkiem różnych
katalogów jest tylko to, że układ paneli ustawiony przy uruchomieniu z terminala nie jest
widoczny przy uruchomieniu z IDE i odwrotnie.

Dla plików z `assets/` (shadery, modele i tekstury) katalog roboczy **nie ma
znaczenia**: program szuka ich względem pliku `.exe`, przez `core::assetPath`
([`../modules/core/paths.md`](../modules/core/paths.md)). Na Windowsie położenie programu
podaje `GetModuleFileNameW`. Zmierzone w stanie M1 (gdy program wczytywał same shadery i
rysował kostkę): program startuje bez linii `[error]` uruchomiony z katalogu repozytorium,
z katalogu roboczego `C:\` oraz z kopii katalogu `build\debug\Debug` umieszczonej w katalogu
z polskimi literami w nazwie (`...\Temp\nm-Żółw\`). Dla modeli i tekstur tych trzech prób
nie powtórzyłem (punkt otwarty w sekcji 12). PRD wymaga budowania ścieżek wyłącznie przez `std::filesystem`.

### Katalog `assets` na Windowsie: kopia, nie dowiązanie

Program oczekuje katalogu `assets` obok `night_maze.exe`, czyli w `build\debug\Debug\assets\`.
Na macOS build tworzy w tym miejscu dowiązanie symboliczne do katalogu w repozytorium. Na
Windowsie utworzenie dowiązania wymaga trybu dewelopera albo uprawnień administratora, więc
build **kopiuje** katalog: robi to target `copy_assets` poleceniem `cmake -E copy_directory`
([`project-structure.md`](project-structure.md), sekcja 3.1, blok 7).

Program czyta więc kopię, a nie pliki z repozytorium. Kopię odświeżają dwa polecenia:

| Polecenie | Co robi | Przy działającym programie |
|---|---|---|
| `cmake --build --preset debug --target copy_assets` | tylko kopiuje katalog `assets`. Nie buduje programu, bo `copy_assets` od niego nie zależy | działa (kod wyjścia 0, kopia odświeżona) |
| `cmake --build --preset debug` | buduje wszystko, a `copy_assets` należy do targetu domyślnego (`ALL`), więc kopia jest robiona od nowa przy każdym takim budowaniu, także gdy żaden plik C++ się nie zmienił | z generatorem Visual Studio **kończy się błędem** `LNK1168` (niżej) |

Reguła pracy z shaderami na Windowsie, gdy program działa:

1. zmień plik w `assets\shaders\` (także plik dołączany dyrektywą `#include`, czyli
   `assets\shaders\common\lighting.glsl`: kopiowany jest cały katalog, z podkatalogami),
2. w drugim terminalu wykonaj `cmake --build --preset debug --target copy_assets`,
3. naciśnij przycisk "Reload shaders" w oknie debug (Diagnostics / Frame and shaders / Shaders).

Gdy program nie działa, wystarczy zwykłe `cmake --build --preset debug` i ponowne
uruchomienie. Na macOS krok 2 nie jest potrzebny. Pominięcie go na Windowsie nie daje błędu:
panel dalej pokazuje przy każdym programie `: OK`, a obraz się nie zmienia, bo program
wczytał poprawnie starą kopię pliku.

Okno debug (Diagnostics / Frame and shaders / Shaders) ma dziś jedną linię na program, a programów jest jedenaście (`textured`,
`color`, `lit`, `gouraud`, od M6 `skybox` i `grass`, od pierwszej części M7 `composite`
i `preview`, od drugiej `bright` i `blur`, od czwartej `shadow_depth`, czyli linia
`shadow_depth.vert + shadow_depth.frag`). W M4 było ich pięć:
piąty, `basic`, rysował kostkę z M1 i został usunięty w M5 razem z plikami `basic.vert`
i `basic.frag`. Po udanym wczytaniu
linia ma postać:

```text
textured.vert + textured.frag: OK
```

Po nieudanym przeładowaniu linia jest czerwona, a pod nią, też na czerwono, stoi komunikat
błędu (ten sam tekst trafia do konsoli jako `[error]`):

```text
lit.vert + lit.frag: FAILED, the previous program stays in use
```

Gdy program nie wczytał się ani razu (błąd już przy starcie), końcówka linii brzmi
`FAILED, there is no program to draw with`. Podpowiedź (tooltip) nad linią programu pokazuje
w dwóch wierszach pełne ścieżki obu czytanych plików, czyli kopii w
`build\debug\Debug\assets\shaders\`. Wcześniejsza wersja panelu (do 2026-10-05) miała dla
każdego programu blok kilku linii z osobnymi etykietami plików i stanu: tych napisów już
nie ma. Kod panelu: `src/debug/panels/ShadersPanel.cpp`, opis w
[`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6, a
nazwy plików w komunikatach błędów w
[`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md).

**Dlaczego nie pełny build przy działającym programie.** Wcześniejsza wersja tego dokumentu
przewidywała, że `cmake --build --preset debug` da się wykonać przy działającym programie,
bo bez zmian w C++ nic nie jest linkowane. Pomiar pokazał co innego. Z generatorem Visual
Studio taki build kończy się błędem `LINK : fatal error LNK1168` (dalej w tej linii jest
komunikat o pliku `night_maze.exe`, którego nie można otworzyć do zapisu). Windows blokuje plik `.exe`
działającego programu, a MSBuild próbuje go wtedy zlinkować od nowa, także gdy żaden plik
C++ się nie zmienił. Ten sam build przy zatrzymanym programie niczego nie linkuje (pomiar
niżej). Dlaczego MSBuild zachowuje się różnie w tych dwóch sytuacjach, nie badałem.

Z tego powodu z `CMakeLists.txt` zniknęła linia `add_dependencies(copy_assets night_maze)`.
Target `copy_assets` nie zależy teraz od programu (polecenie `copy_directory` samo tworzy
katalog docelowy), więc budowany osobno nie dotyka `night_maze.exe`.

Z generatorem Ninja pełny build przy działającym programie przeszedł: wykonał jeden krok,
`[1/1] Copying assets next to the executable`. Nie było wtedy nic do zlinkowania. Jak Ninja
zachowa się, gdy przy działającym programie zmieni się plik C++, nie mierzyłem.

**Zmierzone 2026-10-05** (generator Visual Studio, o ile nie napisano inaczej):

| Próba | Wynik |
|---|---|
| pierwszy build Debug i Release | powstają `build\debug\Debug\assets\shaders\basic.vert` i `basic.frag`, tak samo w `build\release\Release\` (stan M1, gdy shaderów było dwa. Od M5 tych dwóch plików nie ma w repozytorium). W wyjściu jest linia `Copying assets next to the executable` |
| drugi build bez żadnych zmian, program zatrzymany | linia pojawia się ponownie, program nie jest linkowany |
| zmiana shadera, `cmake --build --preset debug`, uruchomienie | kopia odświeżona, program pokazuje nowe kolory |
| zmiana shadera, `cmake --build --preset debug --target night_maze` | kopia **nie** została odświeżona |
| `cmake --build --preset debug` przy działającym programie | błąd `LNK1168` |
| `cmake --build --preset debug --target copy_assets` przy działającym programie | kod wyjścia 0, kopia odświeżona |
| `--target copy_assets` po usunięciu katalogu `assets` obok programu | katalog odtworzony |
| pełny build przy działającym programie, generator Ninja | przeszedł, jeden krok: kopiowanie |

Samego przycisku "Reload shaders" nikt przy tych próbach nie naciskał: krok 3 reguły jest na
listach kontrolnych w sekcjach 11 i 13.2 jako otwarty. Tego, że kopia obejmuje podkatalog
`shaders\common\`, osobno nie sprawdzałem. Wynika to pośrednio z pomiaru z 2026-10-05:
programy `lit` i `gouraud` dołączają `common/lighting.glsl`, a gra startuje bez linii
`[error]`.

Uwagi:

- Budowanie **samego** targetu `night_maze` (`cmake --build --preset debug --target night_maze`)
  kopii nie odświeża, bo `night_maze` nie zależy od `copy_assets` (zmierzone). Uruchomienie
  klawiszem F5 w Visual Studio może budować tylko projekt startowy. Jeśli tak jest, przed F5
  trzeba zbudować całe rozwiązanie (Build Solution) albo sam target `copy_assets`. To punkt
  do sprawdzenia z listy w sekcji 11.
- Edytowanie plików wprost w `build\debug\Debug\assets\` działa, ale zmiany przepadną przy
  następnym kopiowaniu, bo kopia zostanie nadpisana plikami z repozytorium.
- `copy_directory` nadpisuje istniejące pliki, ale nie usuwa z kopii plików, których nie ma
  już w repozytorium. Po usunięciu albo zmianie nazwy shadera warto skasować katalog
  `build\debug\Debug\assets\` i wykonać `--target copy_assets` ponownie. Przykład: w
  katalogu buildu sprzed M5 w kopii zostają `basic.vert` i `basic.frag`. Niczemu to nie
  szkodzi, bo program ich już nie czyta (wniosek z opisanego zachowania `copy_directory`,
  nie pomiar).
- Kopię można też zrobić ręcznie, bez CMake jako systemu budowania:
  `cmake -E copy_directory assets build\debug\Debug\assets`.
- Skrót `make debug` nie zastępuje kroku 2: wykonuje pełny build, a nie sam `copy_assets`
  (i nie był na Windowsie uruchamiany, sekcja 2).

## 8. RenderDoc

RenderDoc to darmowy debugger grafiki: przechwytuje jedną klatkę i pozwala obejrzeć każde
wywołanie rysujące, stan potoku, zawartość tekstur i buforów oraz wejścia i wyjścia shaderów.
Strona projektu: <https://renderdoc.org/>.

- **Działa tylko na Windowsie (i Linuksie). Nie obsługuje macOS.** Na Macu zostają `GL_CHECK`
  i panele ImGui. To jeden z powodów, dla których projekt ma działać na obu systemach.
- Wymaga kontekstu OpenGL w profilu Core w wersji co najmniej 3.2. Nasz kontekst 4.1 Core
  spełnia ten warunek.
- Jest narzędziem zewnętrznym. Nie wymaga żadnych zmian w kodzie ani w CMake.

Typowe użycie: w RenderDoc, w zakładce Launch Application, wskazujemy
`build\debug\Debug\night_maze.exe`, ustawiamy Working Directory na katalog repozytorium,
uruchamiamy program i przechwytujemy klatkę klawiszem F12 lub PrintScreen.

Dziś w przechwyconej klatce powinny być: czyszczenie ekranu, jedno wywołanie
`glDrawElements` na cały teren (jedna siatka, 18432 trójkąty w labiryncie startowym), po
jednym na każdą ścianę i słupek labiryntu (dla labiryntu startowego
121 + 121, w trybie startowym programem `lit`), tym samym programem jedno na bramę,
dopóki nie schowała się pod ziemią, i po jednym na każdy niezebrany kryształ (13 na
starcie rundy), potem jedno wywołanie z prymitywem `GL_POINTS` na całą trawę (program
`grass`, 1843 punkty przy ustawieniach startowych: trójkąty źdźbeł powstają dopiero
w shaderze geometrii), niebo programem `skybox`, a na końcu rysowanie ImGui (panele
i HUD). Do M5 w miejscu terenu było 100 wywołań, po jednym na płytkę podłogi. Przy
zaznaczonym polu
`Draw collision shapes` dochodzą linie programem `color` (rysowane przed niebem). Kostki z M1 i znaczników
świateł, które były w klatce do M4, już nie ma. Można w niej obejrzeć bufory wierzchołków i indeksów, macierze w
uniformach, bufor uniformów ze światłami (blok `LightBlock`, 928 bajtów, punkt wiązania 1),
związaną teksturę i sampler, bufor głębi oraz wejścia i wyjścia shaderów. Narzędzie stanie
się naprawdę użyteczne przy cieniach i efektach pozaekranowych.

Przechwycenia klatki z programu `night_maze` jeszcze nie sprawdzałem (punkt na liście
kontrolnej w sekcji 11).

## 9. Końce linii: `.gitattributes`

Windows tradycyjnie kończy linie parą znaków CRLF, macOS i Linux samym LF. Bez ustaleń
praca na dwóch systemach produkuje commity, w których "zmieniła się" każda linia pliku.

Cały plik [`.gitattributes`](../../.gitattributes):

```gitattributes
* text=auto
```

- `*`: reguła dotyczy wszystkich plików.
- `text=auto`: Git sam rozpoznaje, czy plik jest tekstowy. Pliki tekstowe są zapisywane w
  repozytorium zawsze z końcami LF. Pliki binarne (obrazy, modele) nie są ruszane.
- Przy pobieraniu plików do katalogu roboczego Git może na Windowsie zamienić LF na CRLF,
  zależnie od lokalnego ustawienia `core.autocrlf`. Przy commicie zamienia je z powrotem.

Efekt: w repozytorium zawsze jest LF, niezależnie od tego, na którym komputerze powstał
commit. Reguła jest w repozytorium, a nie w ustawieniach Gita na jednym komputerze, więc
działa tak samo wszędzie.

Zmierzone na Windowsie: po konfiguracji i wszystkich buildach z tego dokumentu (Debug,
Release, Ninja) `git status` nie pokazuje żadnego zmienionego pliku, a katalog `build/` jest
ignorowany.

Kompilatory na obu systemach akceptują oba rodzaje końców linii. Problem dotyczył wyłącznie
czytelności historii Gita.

## 10. Rozwiązywanie problemów

Kolumna "Stan" mówi, czy objaw widziałem na Windowsie (zmierzone), czy wiersz wynika tylko z
dokumentacji narzędzi (przewidywane).

| Objaw | Prawdopodobna przyczyna | Rozwiązanie | Stan |
|---|---|---|---|
| `'cmake' is not recognized` albo podobny komunikat powłoki | CMake z Visual Studio nie jest w `PATH` zwykłego terminala | wejdź w środowisko deweloperskie (sekcja 2) albo zainstaluj CMake osobno | zmierzone |
| Ostrzeżenie `Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE` przy konfiguracji | generator Visual Studio ignoruje `CMAKE_BUILD_TYPE` | nic, to oczekiwane (sekcja 3) | zmierzone |
| `LINK : fatal error LNK1168` przy `cmake --build --preset debug` | program `night_maze.exe` działa, a Windows blokuje jego plik | zamknij program i zbuduj ponownie. Do odświeżenia samych shaderów użyj `cmake --build --preset debug --target copy_assets` (sekcja 7) | zmierzone |
| Panele leżą jeden na drugim albo w dziwnych miejscach, na przykład panel Lights zasłania panel Camera | w katalogu roboczym jest `imgui.ini` zapisany przez starszą wersję programu albo przy innym rozmiarze okna: jego wpisy wygrywają z układem startowym z kodu | usuń `imgui.ini` z katalogu, z którego startuje program (sekcja 7), albo rozsuń panele myszą za paski tytułu | przewidywane (w stanie M1, bez pozycji startowych w kodzie, trzy panele leżały jeden na drugim: zmierzone) |
| Nie widać panelu Camera, Gameplay, Terrain, Grass, Framebuffers, Shadows albo Environment, są tylko wąskie paski z tymi napisami u góry okna, w pięciu rzędach | te siedem paneli startuje zwinięte do paska tytułu: w oknie 1280 x 720 nie ma miejsca na trzynaście otwartych paneli | kliknij strzałkę w pasku tytułu panelu | przewidywane |
| Latarka nie daje się włączyć klawiszem F ani polem `Flashlight on (key F)` | bateria jest pusta: na pasku HUD jest `0%` i napis `Battery empty. Find a crystal.`. To reguła gry, nie błąd | zbierz kryształ (daje 25% baterii), potem naciśnij F. Do testów: suwak `Battery` w panelu Gameplay albo klawisz R (nowa runda) | przewidywane (z kodu `game::updateRound`, sekcja 14.2) |
| Podłoże albo ściany są białe, w konsoli linia `[error]` o pliku obrazu | obok `night_maze.exe` brakuje pliku z `assets\textures\` albo nie da się go zdekodować. Część modelu (albo teren, gdy chodzi o `ground.png`) dostaje wtedy białą teksturę zastępczą | `cmake --build --preset debug --target copy_assets`, potem ponowne uruchomienie (pamięć podręczna nie ponawia nieudanego wczytania) | zmierzone dla modelu (zrzut ekranu z celowo usuniętą teksturą ówczesnej płytki podłogi, sekcja 12), dla terenu przewidywane z kodu `game::TerrainRenderer` |
| Podłoże jest zupełnie płaskie, a w konsoli jest linia `[error]` o pliku `heightmap.png` | mapy wysokości nie udało się wczytać. Gra używa wtedy płaskiej mapy (wszystkie wysokości 0) i działa dalej | `cmake --build --preset debug --target copy_assets`, potem ponowne uruchomienie (mapa jest czytana raz, przy starcie). Płaskie podłoże bez linii `[error]` to suwak `Height scale` w panelu Terrain ustawiony na 0 | przewidywane (z kodu `loadHeightmap` w `src/game/NightMazeApp.cpp`) |
| Okno jest czarne albo pokazuje zamrożony obraz, w konsoli `[error]` zaczynający się od `Framebuffer of` albo `Framebuffer cannot be created` | bufora sceny nie udało się utworzyć (sterownik uznał go za niekompletny albo dostał rozmiar poniżej 1). `onRender` pomija wtedy scenę i ostatni przebieg, a ImGui rysuje dalej | zapisać cały komunikat: podaje rozmiar, formaty i powód. Opis stanów: [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) | przewidywane, nie zaobserwowane |
| Scena jest czarna, panele działają, w konsoli `[error]` z nazwą `composite.vert` albo `composite.frag` | program `composite` nie zbudował się przy starcie, więc obrazu sceny nie ma czym przenieść do okna | popraw plik shadera, odśwież kopię (sekcja 7), "Reload shaders" | przewidywane |
| Podglądy tekstur w panelu Assets są wyraźnie ciemniejsze niż pliki | sterownik nie ma rozszerzenia `GL_EXT_texture_sRGB_decode`, więc ImGui czyta tekstury sRGB zdekodowane do wartości liniowych. Sama scena jest poprawna | brak, to znane ograniczenie ([`../modules/debug-ui.md`](../modules/debug-ui.md)) | przewidywane, na tym komputerze rozszerzenie jest |
| Nie ma trawy, w konsoli `[error]` z nazwą `grass.vert`, `grass.geom` albo `grass.frag` | program `grass` nie zbudował się przy starcie, więc trawy nie ma czym rysować. Reszta sceny jest rysowana | popraw plik shadera, odśwież kopię (sekcja 7), "Reload shaders". Trawy nie ma też przy odznaczonym polu `Enabled` i przy suwaku `Density` równym 0 w panelu Grass | przewidywane (z kodu `GrassRenderer::draw`). Zgłoszone dla celowo zepsutego pliku (bez informacji, czy przed startem, czy przed przeładowaniem): linia sterownika zaczyna się od `grass.geom(84)`, a gra działa dalej (sekcja 16.1) |
| Brakuje ścian albo słupków, w konsoli `[error]` z nazwą pliku `.obj` albo `.mtl` | brakuje pliku modelu albo jego pliku `.mtl`. Model, którego nie udało się wczytać, nie jest rysowany, reszta labiryntu tak | jak wyżej | przewidywane |
| Zmiana w pliku shadera nie jest widoczna po ponownym uruchomieniu | program czyta kopię obok `.exe`, a po zmianie pliku nie było kopiowania albo zbudowano tylko target `night_maze` (możliwe przy F5 w Visual Studio) | `cmake --build --preset debug --target copy_assets` albo pełny build przy zamkniętym programie, sekcja 7 | zmierzone dla `--target night_maze`, F5 przewidywane |
| `[error] Shader compilation failed: ...\assets\shaders/color.frag` (albo inny plik shadera) i linia sterownika. Przy błędzie już na starcie znika to, co rysuje ten program (dla `color` linie kształtów kolizji, dla `textured` scena w trybie `Unlit` i w widokach debug), przy błędzie po "Reload shaders" obraz zostaje, bo działa poprzedni program | błąd składni w pliku shadera. Mieszane ukośniki w ścieżce są poprawne (sekcja 11, klasa `gfx::Shader`). Linia sterownika zaczyna się od nazwy pliku i numeru linii, na przykład `color.frag(4)` | popraw plik, odśwież kopię (sekcja 7) | zmierzone w stanie M1 na pliku `basic.frag`, którego od M5 nie ma, gdy program rysował samą kostkę: w oknie zostawało wtedy samo tło i panele, a linia sterownika zaczynała się od `0(15)`. Nazwa pliku w miejscu numeru: zmierzone 2026-10-05 jako `basic.frag(4)` (sekcja 13.1). Dla dzisiejszych plików przewidywane |
| Komunikat błędu shadera wskazuje `common/lighting.glsl(N)` albo `common/shadows.glsl(N)`, choć zmieniany był inny plik, albo czerwone są naraz linie `lit`, `gouraud` i `grass` w panelu Shaders | błąd jest w pliku dołączanym przez `#include`. Oba pliki dołączają dziś trzy programy (`lit.frag` i `grass.frag` oba, `gouraud.vert` pierwszy, `gouraud.frag` drugi), więc żaden z trzech nie daje się zbudować | popraw `assets\shaders\common\lighting.glsl`, odśwież kopię (sekcja 7), "Reload shaders" | komunikat z nazwą pliku zmierzony na zrzucie ekranu (sekcja 13.1), reszta przewidywana |
| `[error] Shader include failed: ...` | plik z dyrektywy `#include` nie istnieje w kopii obok programu, dołącza sam siebie albo linia `#include` jest błędnie zapisana. Komunikat podaje plik i linię | popraw dyrektywę albo odśwież kopię (sekcja 7) | przewidywane (tekst z kodu `src/gfx/Shader.cpp`) |
| `[error] Uniform block LightBlock is ... bytes in the shader, but 928 bytes in the C++ code` | sterownik ułożył blok uniformów inaczej niż struktura `scene::LightBlockData`, albo blok w `common/lighting.glsl` zmieniono bez zmiany struktury | porównaj blok w shaderze ze strukturą w `src/scene/LightBlock.hpp` ([`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md)) | przewidywane (na karcie NVIDIA linia się nie pojawia: zmierzone) |
| Nie widać podłoża, ścian, słupków, kryształów ani bramy, są tylko niebo, trawa, panele i pasek HUD | oświetlony program (`lit`, a w trybie `Gouraud` program `gouraud`) nie wczytał się przy starcie, więc nie ma czym rysować sceny. W konsoli jest `[error]`, a linia programu w panelu Shaders kończy się napisem `FAILED, there is no program to draw with` | popraw plik shadera, odśwież kopię (sekcja 7), "Reload shaders". Do tego czasu tryb `Unlit` w panelu Renderer rysuje scenę programem `textured` | przewidywane (z kodu `NightMazeApp::drawLitMaze`) |
| Konfiguracja pada przy pobieraniu GLFW, GLM lub ImGui | brak `git` w `PATH` albo brak sieci | zainstaluj git, otwórz nowy terminal, sprawdź `git --version` | przewidywane |
| Błąd o niezgodności generatora | katalog buildu utworzony innym generatorem (terminal a IDE, Visual Studio a Ninja) | usuń `build\debug` i skonfiguruj ponownie jednym narzędziem, albo użyj osobnego katalogu (`-B`, sekcja 4) | przewidywane |
| Nie ma pliku `build\debug\Debug\night_maze.exe` | użyto generatora jednokonfiguracyjnego (Ninja) | szukaj bezpośrednio w katalogu buildu, na przykład `build\debug\night_maze.exe`, patrz sekcja 4 | zmierzone (Ninja nie tworzy podkatalogu `Debug`) |
| `Fatal: Failed to create a window with an OpenGL 4.1 Core context` | sterownik bez OpenGL 4.1, sesja pulpitu zdalnego albo maszyna wirtualna | zaktualizuj sterownik karty. Przeczytaj linię `GLFW error` powyżej | przewidywane |
| `GL_RENDERER` pokazuje kartę zintegrowaną na komputerze z drugą kartą | system wybrał kartę energooszczędną | w ustawieniach grafiki Windows lub panelu sterownika przypisz `night_maze.exe` do wydajnej karty | przewidywane (na moim PC z kartami AMD Radeon i NVIDIA system sam wybrał NVIDIA) |
| FPS dużo wyższe niż odświeżanie monitora | sterownik wymusza wyłączony vsync | sprawdź ustawienie synchronizacji pionowej w panelu sterownika | przewidywane |
| Ostrzeżenia `/W4` z plików w `_deps` lub `external` | nagłówki systemowe nie zostały wyciszone | sprawdź, czy katalog trafia do kompilatora przez `/external:I` (sekcja 5), zanotuj wersje CMake i MSVC | przewidywane (u mnie nie wystąpiło) |
| Układ paneli nie zapamiętuje się (od 2026-10-06 dotyczy tylko przypiętego panelu okna debug) | różne katalogi robocze | sekcja 7 | przewidywane |
| Gra działa, ale nie widać okna debug ani paska stanu | okno debug startuje ukryte (od 2026-10-06) | naciśnij `~` (klawisz na lewo od `1`) | z kodu, nie z pomiaru na tym komputerze |
| `[error] Shader file cannot be opened: ...\assets\shaders/textured.vert` (i takie same linie dla pozostałych programów), w oknie samo tło | obok `night_maze.exe` nie ma katalogu `assets` (program skopiowany ręcznie albo zbudowano tylko target `night_maze`, bez `copy_assets`) | `cmake --build --preset debug --target copy_assets`, sekcja 7 | przewidywane |
| Cursor lub VS Code pokazuje "file not found" przy każdym `#include`, choć build przechodzi | generator Visual Studio nie tworzy `compile_commands.json`, którego szuka `.clangd` | skonfiguruj `build\debug` generatorem Ninja, sekcja 4 | przewidywane |

Od 2026-10-06 wiersze tej tabeli o panelach (nakładanie się, wąskie paski zwiniętych paneli, panel Assets i panel Shaders) opisują
program sprzed jednego okna debug. Dzisiejsze miejsca: Diagnostics / Assets i Diagnostics / Frame and shaders / Shaders, a okno jest stałe
i nie nakłada się na siebie (tabela w sekcji 27.3).

## 11. Lista kontrolna pierwszego buildu na Windowsie

Do przejścia na PC przed uznaniem M1 za zamknięty na obu systemach i przed tagiem wersji.

**Ta lista powstała dla stanu M1**, w którym program rysował samą kostkę na środku okna, a
kamera latała swobodnie. Punkty `[x]` są zapisem pomiarów z tamtego stanu i zostają bez
zmian w treści: mówią o kostce na środku, dwóch liniach `[info]` i trzech panelach, bo tak
wtedy było. Po M2 + M3 program startuje w labiryncie (sekcja 2), a kamera idzie za graczem.
Kostka wisiała potem nad komórką w przeciwległym rogu labiryntu do M4. M5 usunął ją
razem z programem `basic`, plikami `basic.vert` i `basic.frag` i danymi wierzchołków w
`NightMazeApp`. Punkty otwarte `[ ]`, które mówiły o kostce, o plikach `basic.*`
albo o locie, są przepisane tak, żeby dało się je wykonać w obecnym programie: zamiast
`basic` używają programu `color` albo `textured`. Sprawdzenia
samego labiryntu, gracza i nowych paneli są w sekcji 12, oświetlenia w sekcji 13, a
rozgrywki w sekcji 14.

> Od 2026-10-06: kroki `[ ]` tej sekcji, które mówią o panelach, wskazują miejsce w oknie debug (tabela w sekcji 27.3), a okno startuje
> ukryte (`~`). Kroki o zwijaniu, dokowaniu i układzie paneli mają dopisek "bez odpowiednika w oknie debug".

Po dodaniu oświetlenia (2026-10-05) zmieniły się trzy rzeczy, o których mówią punkty tej
listy. Panel Shaders ma jedną linię na program zamiast kilkuliniowego bloku. Komunikat błędu shadera zaczyna się od nazwy pliku zamiast od numeru `0`.
Panel Camera startuje zwinięty do paska tytułu. Punkty `[x]` zostają z tekstami z dnia
pomiaru i mają dopisek, punkty `[ ]` mają już teksty dzisiejszego programu.

Punkty `[x]` są zmierzone 2026-10-05 w środowisku z sekcji 1, a wynik jest zapisany przy
punkcie. Punkty `[ ]` są otwarte: nikt ich jeszcze nie wykonał. Punkt, z którego zmierzona
jest tylko część, jest rozbity na dwa. Otwarte zostały trzy grupy: to, co wymaga człowieka
przy myszy i klawiaturze, to, co wymaga środowiska Visual Studio (IDE), oraz RenderDoc,
clangd w edytorze i `make`.

**Środowisko**

- [x] `cmake --version` pokazuje co najmniej 3.24: 3.31.6-msvc6
- [x] `git --version` działa w tym samym terminalu
- [x] wersja narzędzi i kompilatora: Visual Studio Build Tools 2022 17.14.37516 (bez IDE),
      MSVC `cl` 19.44.35228, zestaw narzędzi 14.44.35207, Windows SDK 10.0.26100.0, MSBuild
      17.14.51, Windows 11 Pro 10.0.26200
- [x] `cmake` nie jest w `PATH` zwykłego terminala, potrzebne jest środowisko deweloperskie
      (`Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64`)

**Konfiguracja**

- [x] `cmake --preset debug` kończy się bez błędów. Jedno oczekiwane ostrzeżenie:
      `Manually-specified variables were not used by the project: CMAKE_BUILD_TYPE`
- [x] w `build\debug\_deps` są katalogi `glfw`, `glm` i `imgui`. Wersję potwierdza log tylko
      dla GLM (następny punkt), wersje GLFW `3.4` i ImGui `v1.92.9b-docking` wynikają z tagów
      w `cmake/Dependencies.cmake`
- [x] w logu konfiguracji jest linia `GLM: Version 1.0.3`
- [ ] w rozwiązaniu nie ma projektu biblioteki `glm` (`GLM_BUILD_LIBRARY` jest wyłączone)
- [x] generator z `build\debug\CMakeCache.txt` (`CMAKE_GENERATOR`) dla terminala:
      `Visual Studio 17 2022`
- [ ] to samo dla "Open Folder" w Visual Studio (wymaga IDE)

**Build Debug**

- [x] `cmake --build --preset debug` kończy się bez błędów: kod wyjścia 0
- [x] **zero ostrzeżeń pod `/W4`** w plikach z `src/`: zero ostrzeżeń, zero błędów
- [x] brak ostrzeżeń pochodzących z nagłówków GLFW, GLAD i ImGui w naszych plikach: żadnego.
      Katalogi `external/glad/include` i `_deps/glfw-src/include` trafiają do kompilatora
      przez `/external:I`, `ExternalWarningLevel` to `TurnOffAllWarnings`
- [x] `src/scene/Transform.cpp` i `src/scene/Camera.cpp` to pierwsze pliki, które dołączają
      GLM (`<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`): brak ostrzeżeń z nagłówków
      GLM pod `/W4`. Katalog `_deps\glm-src` trafia do kompilatora przez `/external:I`
- [x] te same dwa pliki kompilują się pod `/W4 /permissive-` bez ostrzeżeń we własnym
      kodzie (stałe `constexpr glm::vec3`: `AXIS_X`, `AXIS_Y`, `AXIS_Z` w `Transform.cpp`,
      `static constexpr` `Camera::WORLD_UP` w `Camera.hpp`, `std::sin`, `std::cos` i
      `std::floor` na typie `float`, `std::clamp`): zero ostrzeżeń
- [x] `src/core/Paths.cpp` kompiluje się pod `/W4 /permissive-` bez ostrzeżeń (gałąź `_WIN32`
      z `<windows.h>` i `GetModuleFileNameW`, którą MSVC zobaczył pierwszy raz): zero
      ostrzeżeń, także o konwersji typów i o ponownej definicji `NOMINMAX` albo
      `WIN32_LEAN_AND_MEAN`
- [x] program jest w `build\debug\Debug\night_maze.exe`

**Uruchomienie**

- [x] okno otwiera się, tło jest ciemnogranatowe (sprawdzone na zrzucie ekranu)
- [ ] okno ma rozmiar 1280 x 720 i tytuł "Night Maze" (nie zapisałem przy pomiarze)
- [x] na środku okna widać kostkę z trzema ścianami w jednolitych kolorach: czerwoną z
      przodu, niebieską z lewej, turkusową u góry. Żadna ściana nie "prześwituje" przez
      inną (test głębi działa). Sprawdzone na zrzucie ekranu
- [ ] zmiana rozmiaru okna myszą (szersze, węższe, wyższe niż szersze): ściany i słupki
      zachowują proporcje, obraz się nie rozciąga
- [ ] minimalizacja okna i przywrócenie: program nie kończy pracy, w konsoli nie ma linii
      `[error]` ani komunikatu o asercji, obraz wraca. Zapisać, jaki rozmiar framebuffera
      pokazuje okno debug (Render / Scene) zaraz po przywróceniu (na Windowsie zminimalizowane okno ma
      framebuffer 0 x 0, a `NightMazeApp::onRender` pomija wtedy rysowanie: sprawdza
      szerokość i wysokość)
- [x] przy uruchomieniu z terminala są w nim dokładnie dwie linie `[info]`
- [ ] przy uruchomieniu dwuklikiem otwiera się osobne okno konsoli z liniami `[info]`
      (dwie o sterowniku i linie `Loaded ...` pamięci podręcznej assetów)
- [x] dokładny napis `GL_VERSION` i `GL_RENDERER`: `4.1.0 NVIDIA 610.74` oraz
      `NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2`. Komputer ma też zintegrowaną kartę AMD
      Radeon, system sam wybrał NVIDIA
- [x] w konsoli nie ma linii `[error]`
- [x] panele "Renderer", "Shaders" i "Camera" są widoczne. Przy pierwszym uruchomieniu (bez
      `imgui.ini`) leżą jeden na drugim (stan M1. Dziś paneli jest dwanaście, mają miejsca
      startowe i według kodu się nie zasłaniają, sekcje 14.2, 16.2, 17.2 i 20.2)
- [ ] FPS i czas klatki w oknie debug (Diagnostics / Frame and shaders / Frame) się aktualizują
- [ ] linie "Framebuffer" i "Window" pokazują te same wartości (na Windowsie powinny być równe)

**Sterowanie i interfejs**

- [ ] klawisz `~` (na lewo od `1`, w kodzie `GLFW_KEY_GRAVE_ACCENT`) ukrywa i pokazuje
  panele. Pasek HUD u góry okna zostaje (sekcja 14.2) i od 2026-10-06 przesuwa się wtedy do górnej krawędzi okna, a po powrocie paneli wraca pod rzędy pasków
- [ ] Escape nie zamyka programu na żadnym ekranie: w menu głównym nic nie robi, a program zamyka przycisk `Quit` (kod wyjścia 0)
- [ ] podczas wpisywania wartości w polu `Clear color` (Ctrl i kliknięcie) Esc anuluje tylko
  edycję i nie zamyka programu, a `~` nie chowa paneli
- [ ] w menu głównym kursor myszy jest widoczny, a po `Play` jest przechwycony bez klikania w scenę
  (gra startuje w menu głównym: sekcja 26)
- [ ] krzyżyk okna zamyka program bez błędów w konsoli
- [ ] docking: okno debug (Render / Scene) daje się przeciągnąć i zadokować do krawędzi okna, środek [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      pozostaje przezroczysty
- [ ] edytor "Clear color" zmienia kolor tła na żywo
- [ ] po ponownym uruchomieniu układ paneli jest zapamiętany (zapisać, gdzie powstał [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `imgui.ini`). Przy pomiarach program był zatrzymywany przez zabicie procesu, więc plik
      nie powstał

**Zmiana rozmiaru**

- [ ] zmiana rozmiaru okna: obraz wypełnia całe okno, wartości w panelu się zmieniają
- [ ] maksymalizacja i przywrócenie okna działają
- [ ] minimalizacja i przywrócenie nie powodują błędów ani zawieszenia
- [ ] przeciąganie okna między monitorami o różnym skalowaniu (jeśli są dostępne).
      Oczekiwane: obraz sceny poprawny, a panele zostają w skali monitora, na którym
      program wystartował (skala jest czytana raz, [`../modules/debug-ui.md`](../modules/debug-ui.md),
      sekcja 5.12.4)

**Build Release**

- [x] `cmake --preset release` i `cmake --build --preset release` bez błędów i ostrzeżeń:
      zero ostrzeżeń
- [x] `build\release\Release\night_maze.exe` uruchamia się i zachowuje tak samo: te same dwie
      linie `[info]`, kostka widoczna (bez części ręcznej)

**Ścieżki do assetów (`core::executableDir`, `core::assetPath`)**

Opis: [`../modules/core/paths.md`](../modules/core/paths.md) i sekcja 7 tego dokumentu.
Funkcje są wołane przy każdym starcie programu (wczytywanie shaderów).

- [ ] ćwiczenie 1 z `paths.md` (tymczasowe `core::logInfo` w `main`): `executableDir()`
      wypisuje katalog pliku `.exe` (z generatorem Visual Studio `build\debug\Debug`) i nie
      zmienia się przy uruchomieniu z innego katalogu roboczego. Wycofać zmianę. Samego
      ćwiczenia nie robiłem. Pośrednio potwierdzają to punkty niżej: start z `C:\` oraz
      ścieżka `...\build\debug\Debug\assets\shaders/basic.frag` w komunikacie błędu shadera
      (pomiar ze stanu M1, pliku `basic.frag` od M5 nie ma)
- [x] po buildzie katalog `build\debug\Debug\assets\shaders\` istnieje i zawiera `basic.vert`
      oraz `basic.frag` (to samo dla `build\release\Release\`). W wyjściu buildu jest linia
      `Copying assets next to the executable`
- [x] drugi build bez żadnych zmian (program zatrzymany): linia `Copying assets next to the
      executable` pojawia się ponownie, a program nie jest linkowany
- [x] zmiana koloru w `assets\shaders\basic.frag`, potem `cmake --build --preset debug` i
      uruchomienie: program pokazuje nowe kolory (odwrócone kolory sprawdzone na zrzucie
      ekranu). Zmiana wycofana
- [x] to samo, ale z `cmake --build --preset debug --target night_maze`: kopia nie została
      odświeżona, program nie widzi zmiany (zgodnie z oczekiwaniem)
- [x] `cmake --build --preset debug` przy działającym programie (generator Visual Studio):
      **nie przechodzi**, `LINK : fatal error LNK1168`, także bez zmian w C++. Wcześniejsze
      oczekiwanie (build bez linkowania) było błędne
- [x] `cmake --build --preset debug --target copy_assets` przy działającym programie: kod
      wyjścia 0, kopia odświeżona
- [x] `--target copy_assets` po usunięciu katalogu `assets` obok programu: katalog odtworzony
- [ ] Visual Studio: zmiana w shaderze, potem samo F5. Zapisać, czy kopia została
      odświeżona (czyli czy F5 buduje też `copy_assets`), i jeśli nie, czy pomaga Build
      Solution (wymaga IDE)
- [x] program startuje bez linii `[error]` i pokazuje kostkę uruchomiony z katalogu
      repozytorium i z innego katalogu roboczego (`C:\`)
- [ ] to samo przy uruchomieniu dwuklikiem i z Visual Studio (F5)
- [x] program startuje i pokazuje kostkę z katalogu, którego ścieżka zawiera polskie litery:
      kopia `build\debug\Debug` w `...\Temp\nm-Żółw\`, bez linii `[error]`

**Klasa `gfx::Shader`**

Opis: [`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md). Na Windowsie klasa
jest skompilowana i sprawdzona przy starcie programu (poprawne pliki i błąd składni).

- [x] `src/gfx/Shader.cpp` i `src/core/Paths.cpp` kompilują się w MSVC z `/W4 /permissive-`
      bez ostrzeżeń (w szczególności `core::pathText` w `Paths.cpp`: `std::string` budowany z
      iteratorów `std::u8string`): zero ostrzeżeń
- [x] celowy błąd składni (usunięty średnik w `basic.frag`, potem `cmake --build --preset
      debug` i uruchomienie): w konsoli jest **jeden** wpis `[error]`, okno pokazuje samo tło
      i panele, program się nie zamyka. Plik przywrócony. Dokładny tekst:

  ```text
  [error] Shader compilation failed: <repo>\build\debug\Debug\assets\shaders/basic.frag
  0(15) : error C0000: syntax error, unexpected '}', expecting ',' or ';' at token "}"
  ```

  Druga linia to format sterownika NVIDII: numer napisu źródłowego 0 i numer linii 15 w
  nawiasie. Ścieżka ma mieszane ukośniki: wsteczne z katalogu programu i zwykły przed
  `basic.frag`, bo nazwa względna `shaders/basic.frag` jest w kodzie zapisana z `/`.
  Windows przyjmuje oba. To zapis ze stanu M1 (2026-10-05). Później zmieniły się dwie
  rzeczy: od M4 w miejscu numeru `0` stoi nazwa pliku (zmierzone 2026-10-05 dla innego
  błędu jako `basic.frag(4)`, sekcja 13.1), a od M5 nie ma ani pliku `basic.frag`, ani
  kostki. Tę samą próbę robi się dziś na `color.frag` (znikają linie kształtów kolizji)
  albo na `textured.frag` (znika scena w trybie `Unlit`)
- [ ] ścieżka z polską literą w komunikacie błędu nie zamyka
      programu (w konsoli litera może być wyświetlona błędnie, to dopuszczalne)

**Panel Shaders i przeładowanie shaderów**

Opis: [`../modules/gfx/shader-hot-reload.md`](../modules/gfx/shader-hot-reload.md), sekcja 6. Na
Windowsie panel jest skompilowany i wyświetla się poprawnie. Przycisku "Reload shaders"
nikt tam jeszcze nie nacisnął. Teksty linii panelu w punktach otwartych są dzisiejsze
(sekcja 7). Te same kroki dla oświetlonych programów i pliku `common/lighting.glsl` są w
sekcji 13.2.

- [x] `src/debug/panels/ShadersPanel.cpp`, `src/debug/DebugContext.hpp` i `src/main.cpp`
      kompilują się w MSVC z `/W4 /permissive-` bez ostrzeżeń (w szczególności stała
      `ERROR_TEXT_COLOR`, dziś `inline constexpr ImVec4` w `src/debug/Theme.hpp`, i
      inicjalizatory desygnowane `DebugContext` z ówczesnym polem `.shader` w kolejności
      deklaracji; to pole zniknęło w M5 razem z programem `basic`):
      zero ostrzeżeń
- [x] panel "Shaders" jest widoczny i pokazuje oba pliki programu `basic` oraz poprawne
      wczytanie (stan M1, 2026-10-05, z jednym programem i ówczesnymi napisami. Dziś
      programu `basic` nie ma, a panel pokazuje jedenaście linii postaci
      `textured.vert + textured.frag: OK`, sekcje 7 i 14.2)
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders) daje się zadokować [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
- [ ] podpowiedź nad linią `textured.vert + textured.frag: OK` pokazuje w dwóch wierszach
      pełne ścieżki obu plików, czyli kopii w `build\debug\Debug\assets\shaders\`. Oczekiwane
      są mieszane ukośniki, tak jak w komunikacie błędu wyżej:
      `...\assets\shaders/textured.frag`
      (podpowiedź i komunikat powstają z tej samej ścieżki). Wcześniejsze oczekiwanie "same
      ukośniki wsteczne" było najpewniej błędne. Samej podpowiedzi nie oglądałem
- [ ] przeładowanie udane: zaznaczyć `Draw collision shapes` w oknie debug (Diagnostics / Collision and picking) (żółte
      linie rysuje program `color`). Przy działającym programie zmienić w
      `assets\shaders\color.frag` linię `fragColor = vec4(uColor, 1.0);` na
      `fragColor = vec4(uColor.bgr, 1.0);`, w drugim terminalu
      `cmake --build --preset debug --target copy_assets`, potem "Reload shaders". Zapisać,
      czy linie zmieniają kolor (oczekiwane: tak, żółte linie ścian robią się niebieskawe,
      bo czerwony i niebieski kanał zamieniają się miejscami). Przycisk przeładowuje
      wszystkie programy naraz (dziś jedenaście). Samo polecenie przy działającym programie
      jest już
      zmierzone (kod wyjścia 0, kopia odświeżona), otwarte zostaje naciśnięcie przycisku.
      Ten sam krok dla programu `textured` jest w sekcji 12.2
- [ ] to samo bez kopiowania: "Reload shaders" zaraz po zapisaniu pliku. Oczekiwane:
      linia `color.vert + color.frag: OK` i obraz bez zmian (program czyta kopię)
- [ ] przeładowanie nieudane: usunięty średnik w `color.frag`,
      `cmake --build --preset debug --target copy_assets`, "Reload shaders". Oczekiwane:
      czerwona linia `color.vert + color.frag: FAILED, the previous program stays in use`,
      pod nią czerwony tekst ze ścieżką pliku i dziennikiem sterownika, ten sam tekst w
      konsoli jako `[error]`, dziesięć pozostałych linii z `: OK`, linie kształtów kolizji
      bez
      zmian. Linia sterownika powinna zaczynać się od nazwy pliku i numeru linii,
      `color.frag(N)`
- [ ] naprawa: przywrócony plik (`git checkout assets/shaders/color.frag`),
      `cmake --build --preset debug --target copy_assets`,
      "Reload shaders". Oczekiwane: linia wraca do `color.vert + color.frag: OK`, pierwotne
      kolory
- [ ] po kilku przeładowaniach w konsoli nie ma żadnej linii `[error] GL_...` (backend ImGui
      i usunięty stary program, `shader-hot-reload.md`, sekcja 6.3)
- [ ] ścieżka z polską literą (kopia katalogu programu jak w punkcie o `Żółw` wyżej):
      podpowiedź nad linią programu pokazuje ścieżkę bez zamknięcia programu, a polskie
      litery są wyświetlone poprawnie (sama linia pokazuje dziś tylko nazwy plików). Wcześniejsze oczekiwanie "znak zastępczy" dotyczyło domyślnej czcionki
      ImGui i jest nieaktualne: panele mają czcionkę Atkinson Hyperlegible z polskimi
      literami. Zmierzone jest tylko to, że czcionka je rysuje: tymczasowy napis `Zażółć
      gęślą jaźń` z kompletem małych i wielkich liter oraz tymczasowa podpowiedź z napisem
      `Żółw` wyglądają poprawnie na zrzucie ekranu. Sam start z takiego katalogu też jest
      zmierzony. Otwarte zostaje obejrzenie w panelu **prawdziwej** ścieżki z polską literą,
      czyli całej drogi przez `core::pathText`

**Kamera: sterowanie i panel Camera**

Opis: [`../modules/scene/camera-controls.md`](../modules/scene/camera-controls.md),
sekcje 5 i 6, [`../modules/game/player.md`](../modules/game/player.md) (ruch gracza),
[`../modules/debug-ui.md`](../modules/debug-ui.md), sekcja 5. Na Windowsie
kod jest skompilowany, a panel pokazuje wartości startowe. Sterowanie nie było tam jeszcze
sprawdzane. Windows jest jedyną z dwóch platform, na której GLFW włącza surowy ruch myszy
(`GLFW_RAW_MOUSE_MOTION`), więc obrót myszą działa tu inną ścieżką niż na Macu.

Od M4 panel Camera startuje zwinięty do paska tytułu (o ile `imgui.ini` nie ma jego wpisu).
Przed punktami, które każą coś w nim odczytać albo przesunąć, trzeba go rozwinąć
kliknięciem strzałki w pasku tytułu (sekcja 13.2).

- [x] `src/game/NightMazeApp.cpp`, `src/debug/panels/CameraPanel.cpp`,
      `src/debug/DebugUI.cpp` i `src/main.cpp` kompilują się w MSVC z `/W4 /permissive-` bez
      ostrzeżeń (w szczególności `static_cast<float>` z `double` przy `mouseDeltaX`, `fixedDt`
      i `alpha`, `glm::mix` z trzecim argumentem `float`, domyślny inicjalizator pola z
      pozycją sprzed ostatniego kroku, `io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse`
      i pola `DebugContext` w kolejności deklaracji): zero ostrzeżeń. Po M2 + M3 to pole
      nazywa się `m_previousPlayerPosition`, a build nadal nie daje ostrzeżeń (sekcja 12)
- [x] stan M1: panel "Camera" był widoczny i pokazywał pozycję kamery 0, 0, 3, `Yaw` 0,
      `Pitch` 0, `FOV` 60. Wartości startowe po M2 + M3 są w sekcji 12
- [ ] okno debug (Player) daje się zadokować [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
- [ ] kliknięcie lewym przyciskiem w scenę chowa kursor, a kursor nie wyjeżdża poza okno
      (także na drugi monitor)
- [ ] pierwsza klatka po kliknięciu: obraz nie szarpie (kamera nie odskakuje)
- [ ] ruch myszy w prawo obraca kamerę w prawo (ściany uciekają w lewo), ruch do góry podnosi
      wzrok. `Pitch` zatrzymuje się na 89 i na -89
- [ ] obrót jest płynny i ten sam ruch ręki daje ten sam obrót niezależnie od szybkości ruchu
      (surowy ruch myszy, bez przyspieszenia systemowego). Zapisać, czy czułość domyślna
      0,1 jest wygodna, czy wymaga innej wartości niż na Macu
- [ ] długi obrót w jedną stronę (kilka pełnych obrotów): kamera kręci się bez zatrzymania,
      `Yaw` zawija się przez 360
- [ ] W, S, A, D przesuwają gracza zgodnie z opisem (chodzenie w poziomie, lewy Shift to
      sprint, w trybie noclip spacja i lewy Shift to góra i dół), ruch po skosie (W i D) nie
      jest szybszy, klawisze przeciwne (W i S) się znoszą. Szczegółowe kroki: sekcja 12
- [ ] przy widocznym kursorze klawisze ruchu nie przesuwają gracza
- [ ] pierwszy Esc przy przechwyconym kursorze otwiera pauzę i oddaje kursor (pojawia się w miejscu, w którym
      zniknął), nie zamykając programu, drugi Esc wznawia grę i przechwytuje kursor ponownie
- [ ] po pokazaniu paneli tyldą (kursor oddany), **bez ruszania myszą**, kliknięcie w scenę znowu przechwytuje kursor i żaden [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      panel nie reaguje na to kliknięcie. Zapisać wynik także dla sytuacji, w której przed
      tyldą mysz była długo przesuwana w stronę zadokowanego panelu (to samo sprawdzić na
      Macu: po zwolnieniu kursora ImGui może do pierwszego ruchu myszy pamiętać ostatnią
      pozycję ukrytego kursora)
- [ ] suwaki okna debug (Player / View) (na przykład `FOV`) działają, a ich przeciąganie nie obraca kamery
      i nie chowa kursora, także gdy kursor wyjedzie przy przeciąganiu nad scenę
- [ ] kliknięcie w panel (pasek tytułu, suwak) nie chowa kursora
- [ ] przy przechwyconym kursorze panele nie reagują na mysz: kręcenie myszą tak, żeby ukryty [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      kursor "przeszedł" nad zadokowanym panelem, nie zatrzymuje obrotu, nie podświetla
      widżetów i nie zmienia żadnej wartości, także przy klikaniu i przy trzymaniu przycisku
- [ ] Alt+Tab przy przechwyconym kursorze: kursor jest widoczny w innym programie. Po
      powrocie do okna zapisać, czy kursor jest znowu schowany i czy kamera nie odskoczyła
- [ ] ruch jest płynny na monitorze o odświeżaniu innym niż 60 Hz (na przykład 144 Hz): chód
      bokiem (D) wzdłuż ściany przy `Walk speed` 20 nie szarpie. Zapisać FPS z okna debug (Diagnostics / Frame and shaders / Frame)
- [ ] to samo przy wyłączonym vsync w panelu sterownika (kilkaset FPS i więcej, większość
      klatek bez kroku symulacji): ruch nadal płynny, prędkość chodu i czułość myszy takie
      same
- [ ] `Near plane` przesuwany w górę wycina najbliższe ściany, `Far plane` przesuwany w dół
      obcina koniec korytarza. Zapisać wartości, przy których to widać z pozycji startowej
- [ ] minimalizacja okna przy przechwyconym kursorze i przywrócenie: bez linii `[error]` i
      bez asercji
- [ ] w konsoli przez cały test ręczny nie ma linii `[error]`

**Klasy `gfx::Buffer` i `gfx::VertexArray`**

Opis: [`../modules/gfx/buffers-vao.md`](../modules/gfx/buffers-vao.md) (klasy) i
[`../modules/gfx/indexed-drawing.md`](../modules/gfx/indexed-drawing.md) (rysowanie z
indeksami). Punkty tej grupy są zapisem ze stanu M1, w którym obu klas używała wprost
kostka w `NightMazeApp`. M5 usunął kostkę razem z jej danymi (tablicami wierzchołków i
indeksów, stałymi układu) i funkcją rysującą: dziś obu klas używa tylko `gfx::Mesh`,
czyli modele labiryntu, kryształów i bramy oraz linie kształtów kolizji.

- [x] `src/gfx/Buffer.cpp` i `src/gfx/VertexArray.cpp` kompilują się w MSVC z
      `/W4 /permissive-` bez ostrzeżeń (w szczególności `reinterpret_cast<const void*>` z
      `std::size_t` w `setFloatAttribute` i `static_cast<GLsizeiptr>` w konstruktorze
      `Buffer`): zero ostrzeżeń
- [x] `src/game/NightMazeApp.cpp` kompiluje się bez ostrzeżeń (stałe `VERTEX_STRIDE` i
      `COLOR_OFFSET` liczone z `sizeof(float)`, tablice `VERTICES` i `INDICES`, rozmiar
      `INDICES.size() * sizeof(GLuint)`, dzielenie `static_cast<float>` przy proporcjach,
      `nullptr` jako ostatni argument `glDrawElements`): zero ostrzeżeń
- [x] `src/gfx/Shader.cpp` i `Shader.hpp` kompilują się bez ostrzeżeń z nagłówkami GLM
      (`<glm/glm.hpp>` w nagłówku, `<glm/gtc/type_ptr.hpp>` i `glm::value_ptr` w `setMat4`):
      zero ostrzeżeń
- [x] kostka jest widoczna i w konsoli nie ma linii `[error]`, także żadnej
      `GL_INVALID_OPERATION after glDrawElements` ani po `glUniformMatrix4fv` (build Debug,
      w którym `GL_CHECK` jest aktywne. Stan M1)

**Testy jednostkowe i kod bez okna (M2 + M3)**

Opis: [`../libraries/doctest.md`](../libraries/doctest.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md),
[`../modules/game/maze-generator.md`](../modules/game/maze-generator.md). Ten kod powstał na
Windowsie, więc wszystkie punkty poza ostatnimi dwoma są zmierzone przy jego pisaniu.

- [x] `cmake --preset debug` pobiera doctest: w `build\debug\_deps` są katalogi
      `doctest-src`, `doctest-build` i `doctest-subbuild`, plik
      `doctest-src\scripts\version.txt` zawiera `2.5.3`
- [x] czysty build (`--clean-first`) presetów `debug` i `release`: kod wyjścia 0, zero
      ostrzeżeń pod `/W4 /permissive-`, także w `src/scene/Collider.*`, `src/game/Maze*` i w
      pięciu plikach `tests/`
- [x] nagłówek doctest nie daje ostrzeżeń: katalogi `_deps/doctest-src` i
      `_deps/doctest-src/doctest` trafiają do kompilatora przez `/external:I`,
      `ExternalWarningLevel` to `TurnOffAllWarnings`
- [x] powstają `build\debug\Debug\game_logic.lib` i `build\debug\Debug\night_maze_tests.exe`
      (to samo w `build\release\Release\`)
- [x] `ctest --test-dir build/debug -C Debug --output-on-failure`: `100% tests passed, 0 tests
      failed out of 1`, kod wyjścia 0. To samo dla `build/release` i `-C Release`
- [x] `ctest` bez `-C`: test nie jest uruchamiany (`***Not Run`, kod wyjścia 8)
- [x] program testowy uruchomiony wprost, Debug i Release: wszystkie przypadki testowe tego
      etapu (kolizje i labirynt) przechodzą, `Status: SUCCESS!`. Liczby dla całego programu
      testowego po M2 + M3 są w sekcji 12, po M4 w sekcji 13.1, a dzisiejsze w sekcji 14.1
- [x] labirynt wzorcowy (4 na 4, ziarno 1) jest ten sam w Debug i w Release i zgadza się z
      niezależnym skryptem w Pythonie
- [x] program `night_maze.exe` nadal się buduje w obu konfiguracjach (nie uruchamiałem go po
      tej zmianie: nowy kod nie jest jeszcze wołany przez grę)
- [x] generator Ninja (`cmake --build build\ninja-debug`): build przechodzi, testy przechodzą
      (`ctest --test-dir build/ninja-debug`, tu bez `-C`)
- [x] clang-format 19.1.5: `--dry-run --Werror` na plikach z `src/` i `tests/` nie zgłasza
      różnic
- [x] clang-tidy 19.1.5 z bazy poleceń `build\ninja-debug`: żadnej diagnostyki w nowych
      plikach i w testach. Jedna w starszym pliku: `modernize-return-braced-init-list` w
      `src/core/Paths.cpp` (gałąź Windows, linia `return std::filesystem::path(buffer);`).
      Poprawiona w kroku łączącym M2 + M3: linia brzmi teraz `return {buffer};`
- [x] diagnostyki kompilatora clang dla nowych plików i testów, jako zastępstwo za build na
      Macu: `clang-tidy --checks=-*,clang-diagnostic-*,readability-identifier-naming
      --extra-arg=/clang:-Wpedantic -p build\ninja-debug` na czterech nowych plikach `.cpp` z
      `src/` i pięciu z `tests/`: żadnej diagnostyki. Sprawdzenie, że polecenie w ogóle coś
      widzi: po tymczasowym dopisaniu nieużywanej zmiennej i porównania `int` z `unsigned`
      zgłasza `clang-diagnostic-unused-variable` i `clang-diagnostic-sign-compare` (zmiana
      wycofana). Ograniczenie: to clang 19 w trybie zgodności z MSVC i z biblioteką
      standardową MSVC, a nie Apple clang z libc++. Samej flagi `-Wpedantic` osobną próbą nie
      sprawdzałem
- [x] loader OBJ i siatka (temat 4, 2026-10-05): `src/assets/ObjLoader.*`,
      `src/gfx/Vertex.hpp`, `src/gfx/Mesh.*` i `tests/ObjLoaderTests.cpp` budują się bez
      ostrzeżeń pod `/W4 /permissive-` w konfiguracji Debug, generatorem Ninja i generatorem
      Visual Studio (osobne katalogi buildu). `night_maze_tests.exe
      --source-file=*ObjLoaderTests*`: wtedy 18 przypadków testowych i 804 asercje, po
      mapach normalnych (M4) 20 przypadków i 1576 asercji, `Status: SUCCESS!`. Konfiguracji Release dla tych plików wtedy nie budowałem (po M2 + M3 jest
      zbudowana i przetestowana, sekcja 12). `gfx::Mesh` był wtedy tylko skompilowany, bez
      użytkownika. Dziś tworzą go `assets::AssetCache` i `game::ColliderLines`, testu
      jednostkowego nadal nie ma ([`../modules/gfx/mesh.md`](../modules/gfx/mesh.md))
- [x] clang-tidy 19.1.5 z bazy poleceń buildu Ninja na `src/gfx/Mesh.cpp`,
      `src/assets/ObjLoader.cpp` i `tests/ObjLoaderTests.cpp`: żadnej diagnostyki z regułami
      projektu (`.clang-tidy`) i żadnej z samymi diagnostykami kompilatora clang
      (`--checks=-*,clang-diagnostic-*,readability-identifier-naming` z `-Wall -Wextra
      -Wpedantic`). Ograniczenie to samo co wyżej: clang 19 z biblioteką standardową MSVC,
      nie Apple clang z libc++. clang-format `--dry-run --Werror` na sześciu nowych plikach
      nie zgłasza różnic
- [x] stb_image (temat 5, 2026-10-05): `cmake --preset debug` pobiera repozytorium stb w
      commicie `2c980bb59875b0d32144a71867fbdebb2f77cd20` do `_deps\stb-src` (12 MB, pełny
      klon, bo bez `GIT_SHALLOW`), pierwsza linia `stb_image.h` to `stb_image - v2.30`.
      Target `stb_image` buduje się z jednego pliku `external\stb\stb_image.c` bez
      ostrzeżeń, generatorem Ninja i generatorem Visual Studio. W poleceniu kompilacji
      tego pliku nie ma `/W4`, a katalog `_deps\stb-src` trafia do kompilacji
      `ImageLoader.cpp` przez `/external:I` z wyłączonymi ostrzeżeniami. `-DSTBI_NO_STDIO`
      jest w poleceniach obu plików
      ([`../libraries/stb_image.md`](../libraries/stb_image.md), sekcja 2)
- [x] loader obrazów, tekstura i settery uniformów (temat 5, 2026-10-05):
      `src/assets/ImageLoader.*`, `src/gfx/Texture2D.*`, `src/gfx/Shader.*` (`setInt`,
      `setVec3`) i `tests/ImageLoaderTests.cpp` budują się bez ostrzeżeń pod
      `/W4 /permissive-` w konfiguracji Debug, generatorem Ninja i generatorem Visual
      Studio (osobne katalogi buildu). `night_maze_tests.exe
      --source-file=*ImageLoaderTests*`: wtedy 7 przypadków testowych i 35 asercji, po
      mapach normalnych (M4) 9 przypadków i 57 asercji, `Status: SUCCESS!`. Cały program testowy razem z testami loadera OBJ też przechodził (liczby po
      M2 + M3: sekcja 12). Konfiguracji Release dla tych plików wtedy nie budowałem. Test nazwy pliku ze
      znakami spoza ASCII (polskie litery i znak japoński) przechodzi przy stronie kodowej
      systemu 1250 ([`../modules/assets/images.md`](../modules/assets/images.md),
      sekcja 5.7)
- [x] clang-tidy 19.1.5 z regułami projektu (`.clang-tidy`, baza poleceń buildu Ninja) na
      `src/assets/ImageLoader.cpp`, `src/gfx/Texture2D.cpp`, `src/gfx/Shader.cpp` i
      `tests/ImageLoaderTests.cpp`: żadnej diagnostyki. clang-format `--dry-run --Werror` na
      siedmiu plikach tego kroku nie zgłasza różnic. Samych diagnostyk kompilatora clang
      (`-Wall -Wextra -Wpedantic`) osobną próbą nie sprawdzałem
- [x] `gfx::Texture2D` sprawdzona programem z ukrytym oknem, poza repozytorium (karta
      NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74, `GL_VERSION` równe `4.1.0 NVIDIA
      610.74`): tworzenie, mipmapy do poziomu 1 x 1, orientacja (pierwszy piksel danych w
      lewym dolnym rogu), wyrównanie wierszy, trzy filtry, anizotropia od 1 do 16,
      przenoszenie, złe argumenty, `glGetError` czysty. Rozszerzenie
      `GL_EXT_texture_filter_anisotropic` jest na liście sterownika (404 rozszerzenia).
      Pełne tabele: [`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcja 5.9
- [x] zmierzona osobliwość tego komputera: poziom anizotropii ustawiony na obiekcie
      tekstury (`glTexParameterf`) nie daje błędu, odczytuje się jako 1 i nie zmienia
      obrazu, a ustawiony na obiekcie samplera (`glSamplerParameterf`) działa. Dlatego
      `Texture2D` trzyma filtr, zawijanie i anizotropię w obiekcie samplera. Przyczyny nie
      ustaliłem ([`../modules/gfx/textures.md`](../modules/gfx/textures.md), sekcje 2.8 i
      5.9)
- [x] tekstury w oknie gry: po kroku łączącym M2 + M3 `Texture2D` ma użytkownika
      (`assets::AssetCache`), a labirynt jest oteksturowany. Filtry i anizotropia na ścianach
      są sprawdzone na zrzutach ekranu, ręczne klikanie widżetów panelu Assets jest otwarte
      (sekcja 12). Pokaz tematu 5 z PRD (podgląd tekstur) niesie panel "Assets". Przełącznik
      map normalnych z PRD doszedł razem z mapami w M4 (sekcje 13.3 i 13.4)
- [x] diagnostyka w `Paths.cpp` poprawiona (`return {buffer};`), clang-tidy na zmienionych
      plikach nie zgłasza niczego (sekcja 12). Na Macu ta gałąź nie jest kompilowana
- [ ] te same testy na macOS: dopiero to porównanie mierzy, że oba systemy generują ten sam
      labirynt (lista w [`build-macos.md`](build-macos.md), sekcja 2, "Testy jednostkowe")

**Git i narzędzia**

- [x] po skonfigurowaniu i zbudowaniu (Debug, Release, Ninja) `git status` nie pokazuje
      zmienionych plików (końce linii)
- [x] `build/` nie pojawia się w `git status`
- [ ] `.vs/` i `imgui.ini` nie pojawiają się w `git status` (żaden z nich przy pomiarach nie
      powstał: nie było IDE, a program był zatrzymywany przez zabicie procesu)
- [ ] Visual Studio: "Open Folder", konfiguracja z presetów, F5 uruchamia `night_maze.exe`
      (wymaga IDE)
- [ ] RenderDoc: przechwycenie jednej klatki działa (opcjonalnie)
- [x] generator Ninja: `cmake --preset debug -G Ninja -B build\ninja-debug` i
      `cmake --build build\ninja-debug` przechodzą (52 kroki, zero ostrzeżeń), powstaje
      `build\ninja-debug\compile_commands.json`, program leży w
      `build\ninja-debug\night_maze.exe`, katalog `assets` jest skopiowany obok niego
- [ ] Cursor lub VS Code z clangd: po `cmake --preset debug -G Ninja` w czystym `build\debug`
      błędy "file not found" w edytorze znikają (zapisać wynik, opcjonalnie)
- [ ] rozszerzenie CodeLLDB debuguje program zbudowany przez MSVC (zapisać wynik,
      opcjonalnie)
- [ ] `Makefile` w Git Bash albo innej powłoce z `make` (na moim PC nie ma `make`, plik nie
      był uruchamiany)

## 12. Lista kontrolna M2 + M3: labirynt, gracz, tekstury, panele

Krok, który łączy kolizje, labirynt, loadery, siatkę i tekstury w działającą grę. Opis kodu:
[`../modules/game/player.md`](../modules/game/player.md),
[`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md),
[`../modules/assets/asset-cache.md`](../modules/assets/asset-cache.md),
[`../modules/scene/collision.md`](../modules/scene/collision.md) (linie pudełek i panel
Collision), [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) (panel
Maze), [`../modules/gfx/textures.md`](../modules/gfx/textures.md) (shadery `textured`).

Kamień milowy **nie jest zamknięty**: część ręczna poniżej jest otwarta, na macOS kod nie
był budowany ([`build-macos.md`](build-macos.md)) i nie ma tagu.

**Ta sekcja jest zapisem stanu z 2026-10-05**, sprzed oświetlenia. Program miał wtedy o jeden
panel mniej (bez Lights), o dwa programy shaderów mniej (bez `lit` i `gouraud`), panel
Shaders z kilkoma liniami na program, równo jasny labirynt i mniej testów. Punkty `[x]`
są pomiarami z tamtego dnia i opisują tamten stan. Stan po M4 (163 przypadki i 62220
asercji, siedem paneli, pięć programów, mapy normalnych) opisuje sekcja 13, stan po M5
(215 i 85098, osiem paneli, cztery programy, runda z kryształami) sekcja 14, stan po
pierwszej części M6 (221 i 85175, pięć programów, niebo) sekcja 15, stan po drugiej (256
i 101232, dziesięć paneli, sześć programów, teren w miejscu płytek podłogi i trawa)
sekcja 16, stan po pierwszej części M7 (zgłoszone 269 i 102103, jedenaście paneli, osiem
programów, scena rysowana do bufora HDR, gamma i nowe wartości świateł) sekcja 17,
stan po drugiej (zgłoszone 276 i 102139, dziesięć programów, bloom) sekcja 18,
stan po trzeciej (zgłoszone 294 i 102412, nadal dziesięć programów, mgła i winieta)
sekcja 19, po czwartej (zgłoszone 310 i 103751, dwanaście paneli, jedenaście programów,
cienie księżyca) sekcja 20, po piątej (zgłoszone 329 i 104306, nadal dwanaście paneli
i jedenaście programów, cień latarki) sekcja 21, a dzisiejszy (zgłoszone 414 i 138711,
trzynaście programów, dwanaście paneli, minimapa) sekcja 22, a po M8, części 1 (zgłoszone 445 i 150296 w scalonym drzewie, trzynaście paneli, czternaście programów, environment mapping) sekcja 23. Punkty otwarte `[ ]` w sekcji
12.2 są przepisane tak, żeby dało się je wykonać w dzisiejszym programie. Punkty `[x]`
mówią o podłodze z płytek (`floor_tile.obj`, `floor_stone.png`), którą druga część M6
usunęła.

### 12.1. Zmierzone (2026-10-05)

Środowisko: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74.

- [x] build Debug i Release generatorem Visual Studio oraz build generatorem Ninja: kod
      wyjścia 0, zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: wszystkie ówczesne przypadki przechodzą,
      `Status: SUCCESS!` (2026-10-05. Dzisiejsze liczby przypadków i asercji są w sekcji
      20.1). `ctest` dla obu konfiguracji: `100% tests passed, 0 tests failed out of 1`
- [x] clang-tidy na plikach zmienionych w tym kroku: żadnej diagnostyki
- [x] gra startuje bez linii `[error]`, w tym bez żadnej linii z nazwą błędu OpenGL
      (`GL_INVALID_...`), w buildzie Debug, w którym `GL_CHECK` jest aktywne
- [x] zrzut ekranu ze startu: widok z wnętrza labiryntu, tekstury kamienia na podłodze,
      ścianach i słupkach stoją prosto i nie są odbite lustrzanie (wtedy równo jasne, bez
      oświetlenia. Dzisiejszy widok startowy: sekcja 2)
- [x] zrzut ekranu z góry w trybie noclip: układ ścian zgadza się z planem w panelu Maze, a
      żółte linie pudełek kolizji leżą na ścianach i słupkach
- [x] zrzuty ekranu obu widoków debug shadera `textured`: normalne jako kolor i współrzędne
      UV jako kolor
- [x] zrzuty ekranu porównania filtrów na ścianie widzianej pod płaskim kątem: najbliższy
      sąsiad, dwuliniowy, trójliniowy i trójliniowy z anizotropią 16x
- [x] podglądy tekstur w panelu Assets stoją prosto (nie do góry nogami)
- [x] brak pliku tekstury: powierzchnia rysuje się z białą teksturą zastępczą (w kolorze `Kd`
      materiału), w konsoli jest jedna linia `[error]`

Stany z czterech ostatnich punktów i widok z góry zostały osiągnięte tymczasowymi wstawkami
w kodzie, które są już usunięte, a nie kliknięciami w panelach. Dlatego te same widżety są
jeszcze raz na liście otwartej.

Motyw paneli, czcionka i układ startowy (zmiana po M2 + M3, zmierzone 2026-10-05 na tym samym
PC, ekran 1920 x 1080 przy skali 100%, opis w [`../modules/debug-ui.md`](../modules/debug-ui.md),
sekcje 5.4 (układ okna) i 5.12 (motyw; opis dotyczy stanu sprzed 2026-10-06):

- [x] build Debug i Release generatorem Visual Studio oraz build generatorem Ninja z plikami
      `src/debug/Theme.*` i `src/debug/PanelLayout.*`: kod wyjścia 0, zero ostrzeżeń pod
      `/W4 /permissive-`. Wszystkie ówczesne przypadki testowe przechodzą w Debug
      (2026-10-05), `ctest` przechodzi w Debug i w Release
- [x] clang-format i clang-tidy na plikach `src/debug/`: żadnej diagnostyki. Osobny
      przebieg clang-tidy z samymi diagnostykami kompilatora clang i flagami
      `-Wall -Wextra -Wpedantic` też nic nie zgłasza (kontrola: celowo dopisana nieużywana
      zmienna była w tym przebiegu zgłaszana)
- [x] katalog `assets\fonts` (czcionka, `OFL.txt`, `README.md`) trafia do kopii obok
      `night_maze.exe` bez zmian w CMake: `copy_assets` kopiuje cały katalog `assets`
- [x] okno 1280 x 720 po usunięciu `imgui.ini`: ówczesne panele (bez Lights) się nie zasłaniają i żaden nie
      wychodzi poza okno. Renderer i Camera stoją przy lewej krawędzi, Maze i Assets przy
      prawej, Collision i Shaders przy dolnej, między kolumnami. Panele Renderer, Camera,
      Maze, Collision i Shaders pokazują całą zawartość bez przewijania, panel Assets się
      przewija. Środek górnej części okna jest wolny i widać w nim scenę (układ z
      2026-10-05. Dziś paneli jest dwanaście, pod Rendererem stoi Lights, a Camera,
      Gameplay, Terrain, Grass, Framebuffers i Shadows są zwinięte u góry w czterech
      rzędach: sekcje 14.2, 16.2, 17.2 i 20.2)
- [x] większe okno w pierwszej klatce (1560 x 860 i 1700 x 940, ustawione tymczasową zmianą
      rozmiaru startowego): prawa kolumna stoi przy prawej krawędzi, dolny rząd przy dolnej
- [x] polskie litery w czcionce paneli: tymczasowy napis z kompletem liter i tymczasowa
      podpowiedź wyglądają poprawnie
- [x] tekst błędu shadera w panelu Shaders (zepsuty `color.frag` w kopii `assets`) i wpis
      `Failed to load` w panelu Assets (zmieniona nazwa tekstury) są czytelne, także na tle
      białych ścian
- [x] brak pliku czcionki i plik, który nie jest czcionką: jedna linia `[error]`, panele w
      czcionce wbudowanej, program działa
- [x] skala 150% symulowana mnożnikiem w kodzie: tekst i odstępy rosną, tekst jest ostry. W
      oknie 1280 x 720 panele się wtedy nie zasłaniają, ale ich zawartość się nie mieści
      (paski przewijania, ucięte etykiety)

Tu także stany były ustawiane tymczasowymi wstawkami, już usuniętymi. Stany "pod kursorem" i
"wciśnięty" były rysowane przez podstawienie koloru, a nie przez najechanie myszą.

Obserwacja, nie pomiar wydajności: na starcie panel Renderer pokazywał około 1500 FPS w
buildzie Debug, z synchronizacją pionową taką, jaką ustawił sterownik. Nie wyciągam z tej
liczby żadnych wniosków: nie wiem, czy vsync był aktywny, a pomiar był jeden.

### 12.2. Otwarte: test ręczny na około dziesięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką: chodzenia i ślizgania prawdziwymi klawiszami,
klawisza N, obrotu myszą w labiryncie, przycisków `Regenerate` i `Random seed`, przycisku
`Reload shaders` (dziś z jedenastoma programami) oraz klikania list i suwaka w oknie debug (Diagnostics / Assets).
Przy każdym kroku jest to, co powinno być widać. Oczekiwania wynikają z kodu i z testów
jednostkowych, nie z obserwacji.

Lista powstała przed oświetleniem. W dzisiejszym programie scena jest nocna, więc dwie
rady ułatwiają jej przejście: lista `Lighting` w oknie debug (Render / Scene) ustawiona na `Unlit` daje
równo jasny labirynt z tamtego dnia (wygodny do oglądania tekstur, filtrów i kolizji), a
okno debug (Player) trzeba najpierw rozwinąć strzałką w pasku tytułu. Kroki samego oświetlenia są
w sekcji 13.2. Od M5 w labiryncie trwa też runda: u góry okna jest pasek HUD, w komórkach
wiszą kryształy, a wejście w kryształ go zbiera. Chodzeniu, kolizjom i panelom z tej listy
to nie przeszkadza, a stan rundy przywraca klawisz R. Kroki samej rozgrywki są w sekcji
14.2.

Przygotowanie:

- [ ] zamknąć program, usunąć `imgui.ini` z katalogu, z którego program będzie uruchamiany
      (przy starcie z katalogu repozytorium: z katalogu głównego repozytorium), zbudować
      (`cmake --build --preset debug`) i uruchomić `build\debug\Debug\night_maze.exe`

Start i układ paneli:

- [x] okno 1280 x 720: ówczesne panele (bez Lights) nie zasłaniają się nawzajem. Renderer i Camera stoją w
      kolumnie przy lewej krawędzi, Maze i Assets przy prawej, Collision i Shaders na dole
      między kolumnami. Środek górnej części okna jest wolny. Zawartość ówczesnego okna debug (Diagnostics / Frame and shaders / Shaders)
      mieściła się w nim bez przewijania, dopóki żaden program nie ma błędu (zmierzone na
      zrzucie ekranu 2026-10-05, sekcja 12.1. To układ i okno debug (Diagnostics / Frame and shaders / Shaders) z tamtego dnia:
      dzisiejszy układ dwunastu paneli jest punktem otwartym w sekcjach 14.2, 16.2, 17.2
      i 20.2)
- [ ] po prawdziwym usunięciu `imgui.ini` ręką i starcie z katalogu repozytorium (układ
      dwunastu paneli z sekcji 14.2, 16.2, 17.2 i 20.2): obejrzeć na żywo, czy tekst jest
      wygodny do czytania z odległości (projektor) i czy najechanie myszą na suwak, przycisk
      i pole wyboru zmienia ich tło na ciepły brąz, a panel z fokusem ma morski pasek tytułu
- [ ] okno debug (Player / Position) (rozwinąć strzałką w pasku tytułu): `Mode: walking`, `Player feet` 1, 0, 1, `Eye: 1.00, 1.70, 1.00`, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Yaw` 180 (labirynt startowy: 10 na 10, ziarno 1), `Pitch` 0, `Walk speed` 3.0,
      `Sprint speed` 5.5, `Fly speed` 6.0. Kąt 180 dla ziarna 1 podał autor kodu z
      uruchomienia, żaden test go nie przypina: testy sprawdzają tylko, że kamera patrzy w
      stronę bez ściany
- [ ] okno debug (World / Maze): `Width` 10, `Height` 10, `Seed` 1, linie `In play: 10 x 10 cells, seed 1`,
      `Walls: 121, pillars: 121` i (od M5) `Crystals: 13, exit in cell (6, 5)`, pod nimi plan
      z bursztynową kropką w lewym górnym rogu i
      kreską skierowaną w dół planu (południe). Plan z kropką w tym miejscu jest widoczny na
      zrzucie ekranu z M2 + M3, wartości suwaków i dwóch pierwszych linii tekstu też. Od M5
      na planie są także kryształy, brama i strefa wyjścia (sekcja 14.2)
- [ ] okno debug (Diagnostics / Collision and picking): `Boxes: 121 walls, 121 pillars, 1 gate`,
      `All boxes: 243, pickup spheres: 13`,
      `Wall box: 0.30 m thick (the visible wall: 0.20 m)`, pudełko gracza `min: 0.70, 0.12,
      0.70` i `max: 1.30, 1.92, 1.30` (stopy stoją na terenie, który w środku komórki
      startowej ma wysokość 0,124 m: liczba z przeliczenia w sekcji 16.1, nie z ekranu. Do
      M5 było tu `0.00` i `1.80`). Do M4 w tym miejscu stały linie `Wall boxes: 121`,
      `Pillar boxes: 121` i `All boxes: 242`: bramy i kul nie było
- [ ] okno debug (Diagnostics / Assets): pięć modeli (`wall_straight.obj`, `wall_pillar.obj`,
      `crystal_a.obj`, `crystal_b.obj`, `gate.obj`; kolejność według wczytania może być
      inna), każdy z jedną częścią, nazwą pliku tekstury i linią `normal map:` z nazwą mapy
      normalnych, osiem tekstur z podglądem (`wall_stone.png`,
      `crystal.png`, `gate_wood.png`, `ground.png` i mapa normalnych każdej z nich, o tej
      samej nazwie z końcówką `_normal`), brak sekcji `Failed to load`. Do M5 modeli było
      sześć, z płytką podłogi `floor_tile.obj`, a w miejscu `ground.png` była tekstura
      `floor_stone.png`: teren nie jest modelem z pliku, więc na liście `Models` go nie
      ma, a jego dwie tekstury są na liście `Textures`. Pliku `heightmap.png` na liście
      nie ma, bo nie przechodzi przez pamięć podręczną (zgłoszone, sekcja 16.1)

Chodzenie i kolizje (kliknąć w scenę, kursor znika):

- [ ] W idzie tam, gdzie patrzy kamera, ale zawsze poziomo: z wzrokiem wbitym w ziemię
      prędkość jest ta sama, a y w linii `Eye` jest stale o 1.70 większe od y w polu
      `Player feet` (od drugiej części M6 obie liczby zmieniają się w marszu, bo stopy
      idą po nierównym terenie: do M5 `Eye` miało stale y równe 1.70). Lewy Shift
      przyspiesza
- [ ] dojście do ściany na wprost: gracz staje, obraz nie drży, ściana nie jest przycięta
      przez bliską płaszczyznę
- [ ] ślizganie: ustawić się ukosem do ściany i trzymać W. Gracz sunie wzdłuż ściany,
      zamiast stanąć
- [ ] ślizganie wzdłuż ściany obok słupków: iść przytulonym do długiej prostej ściany przez
      kilka komórek. Gracz **nie może** zahaczać o słupki stojące co 2 m (pudełko ściany ma
      grubość słupka, 0,3 m)
- [ ] róg: wejść ukosem w narożnik wewnętrzny (gracz staje w rogu) i obejść narożnik
      zewnętrzny (gracz zsuwa się po nim bez zacięcia)
- [ ] nie da się wyjść poza labirynt: obejść kawałek ściany zewnętrznej, pchając w nią
- [ ] kropka na planie w oknie debug (World / Maze) porusza się razem z graczem, a jej kreska obraca się
      razem z kamerą
- [ ] Esc otwiera pauzę i oddaje kursor, klawisze ruchu przestają działać, gracz staje w miejscu, a runda stoi

Noclip:

- [ ] klawisz N (działa także przy widocznym kursorze): `Mode: noclip (free flight)`, w
      okna debug (Player / Position) pole `Noclip (key N)` jest zaznaczone
- [ ] w trybie noclip (kursor przechwycony) spacja wznosi, lewy Shift opuszcza, W leci tam,
      gdzie patrzy kamera, także w górę i w dół, przez ściany
- [ ] wzlecieć spacją ponad ściany i spojrzeć w dół: układ ścian zgadza się z planem w panelu
      Maze (północ, czyli -Z, jest na górze planu). Z góry widać też turkusowe kryształy w
      komórkach, w których plan ma kropki, i bramę przy komórce z zielonym prostokątem.
      Kolorowej kostki nad rogiem przeciwległym do startu już nie ma (usunięta w M5)
- [ ] drugi raz N w powietrzu: gracz od razu stoi na podłodze (y stóp równe 0), bez
      widocznego zjazdu w dół. Jeśli wylądował w ścianie, może z niej wyjść
- [ ] to samo polem wyboru `Noclip (key N)` w oknie debug (Player / Position) zamiast klawisza

Pudełka kolizji:

- [ ] `Draw collision shapes` (do M4 pole nazywało się `Draw collision boxes`): żółte linie
      obrysowują każdą ścianę i każdy słupek, linie nie
      migoczą (pudełka są rysowane o 1 cm większe). Pudełka ścian są wyraźnie grubsze od
      korpusu ściany i równe ze słupkami. Linie za ścianą są zasłonięte. Pomarańczowe
      pudełko bramy, turkusowe kule kryształów i pudełko strefy wyjścia w kolorze magenty
      opisuje sekcja 14.2
- [ ] zielone pudełko gracza: kamera stoi w jego środku, więc widać je po spojrzeniu pod
      nogi albo nad głowę, a w całości z boku nie widać go nigdy. Od M5 w środku pudełka
      jest też zielona kula z trzech okręgów (zasięg gracza). Zapisać, jak to wygląda

Regeneracja:

- [ ] zmienić `Width` i `Height` (suwaki od 2 do 40) oraz `Seed`: nic się nie dzieje, linia
      `In play` pokazuje stary labirynt. Dopiero `Regenerate` buduje nowy: zmienia się plan,
      linia `In play`, liczby ścian i linia `Crystals: ...`, gracz stoi znowu w
      `Player feet` 1, 0, 1 z `Pitch` 0 i patrzy w otwarty korytarz, a runda zaczyna się od
      nowa (pasek HUD: zero zebranych kryształów, czas `0:00`, bateria `100%`)
- [ ] `Regenerate` nie zmienia trybu noclip, prędkości, trybu widoku ani pola `Draw collision
      shapes`: ustawić je przed kliknięciem i sprawdzić po nim. Zmienia jedno ustawienie:
      nowa runda zawsze włącza latarkę
- [ ] ten sam rozmiar i to samo ziarno dwa razy dają ten sam plan
- [ ] `Random seed`: w polu `Seed` pojawia się nowa liczba i od razu powstaje nowy labirynt.
      Wpisanie tej liczby później i `Regenerate` odtwarza go
- [ ] labirynt 40 na 40: zapisać FPS z okna debug (Diagnostics / Frame and shaders / Frame) (każdy obiekt to osobne wywołanie
      rysujące)

Okno debug (Render / Textures and normals) (stanąć tak, żeby widzieć długi korytarz i podłoże pod płaskim kątem. W
nocnej scenie daleki koniec korytarza jest ciemny, więc do porównania filtrów najpierw
ustawić `Lighting` w oknie debug (Render / Scene) na `Unlit`):

- [ ] `View mode`, `Normals as colour`: podłoże w odcieniach jasnej zieleni (normalne bliskie +Y, odchylone przez pochyłość terenu i przez mapę normalnych), powierzchnie zwrócone
      na +X czerwonawe, na +Z niebieskawe, a zwrócone w przeciwne strony ciemne w tym
      kanale, więc dwie strony tej samej ściany mają różne kolory. `UVs as colour`: czerwono-zielone przejścia, które zaczynają się od nowa tam,
      gdzie tekstura się powtarza. `Textured` przywraca obraz. Linie kształtów kolizji nie
      zmieniają wyglądu, a kryształy i brama są w obu widokach pokolorowane według tej
      samej reguły co ściany. Oba widoki rysuje zawsze program `textured`, bez świateł (sekcja
      13.2). Opisane kolory normalnych to kolory podstawowe powierzchni: przy zaznaczonym
      polu `Normal mapping` i trybie `Lighting` innym niż `Gouraud` widać na nich jeszcze
      rysunek fug z map normalnych (sekcja 13.4). Żeby zobaczyć same normalne modelu,
      odznaczyć `Normal mapping`
- [ ] `Filter`, `Nearest`: z bliska widać kwadratowe teksele, w oddali obraz ziarni się i
      migocze przy ruchu. `Bilinear`: z bliska gładko, w oddali nadal migocze. `Trilinear`
      (ustawienie startowe): w oddali spokojnie, ale rozmyte
- [ ] `Anisotropy`: suwak od 1x do maksimum sterownika (na tym PC 16x). Przy `Trilinear`
      przesunięcie w prawo wyostrza podłoże i ściany widziane pod płaskim kątem w oddali
- [ ] podglądy tekstur w panelu nie reagują na filtr ani na anizotropię (rysuje je ImGui
      własnym samplerem) i stoją prosto
- [ ] najechanie myszą na nazwę pliku pokazuje pełną ścieżkę

Shadery i brakujący plik:

- [ ] `Reload shaders` po zmianie w `textured.frag`: najpierw ustawić `Lighting` w panelu
      Renderer na `Unlit`, bo w pozostałych trybach labirynt rysują programy `lit` albo
      `gouraud` i zmiana w `textured.frag` nie byłaby widoczna. Przy działającym programie
      zmienić w `assets\shaders\textured.frag` linię
      `fragColor = vec4(texel * uTint * (vec3(1.0) + uEmissive), 1.0);` na
      `fragColor = vec4(texel * uTint * (vec3(1.0) + uEmissive) * vec3(1.0, 0.5, 0.5), 1.0);`,
      w drugim terminalu
      `cmake --build --preset debug --target copy_assets`, potem kliknąć `Reload shaders`.
      Oczekiwane: labirynt robi się czerwonawy, wszystkie jedenaście linii panelu kończy
      się napisem `: OK`
- [ ] błąd w jednym programie (nadal w trybie `Unlit`): usunąć średnik w `textured.frag`,
      `copy_assets`, `Reload shaders`. Oczekiwane: czerwona linia
      `textured.vert + textured.frag: FAILED, the previous program stays in use`, pod nią
      czerwony tekst ze ścieżką pliku i linią sterownika zaczynającą się od
      `textured.frag(N)`, labirynt rysuje się poprzednią wersją, dziesięć pozostałych
      linii ma `: OK`. Przywrócić plik (`git checkout assets/shaders/textured.frag`),
      `copy_assets`, `Reload shaders`
- [ ] celowo brakująca tekstura: zamknąć program, zmienić nazwę
      `build\debug\Debug\assets\textures\wall_stone.png` (kopii, nie pliku w repozytorium),
      uruchomić. Oczekiwane: w trybie `Unlit` ściany i słupki są białe (`Kd` materiału to
      biel), w trybach z oświetleniem nie mają rysunku kamienia i mają kolor padającego na
      nie światła (biała tekstura zastępcza razy światło). Mapa normalnych kamienia wczytała
      się niezależnie, więc w trybach `Phong` i `Blinn-Phong` na białych ścianach nadal widać
      relief fug. Podłoże, kryształy i brama bez zmian,
      w konsoli jedna linia `[error]`, w oknie debug (Diagnostics / Assets) przy części `wall_stone` napis
      `no texture (white)` i sekcja `Failed to load` z nazwą pliku na czerwono. Przywrócić
      nazwę pliku. (Do M5 ten punkt używał tekstury `floor_stone.png` płytki podłogi,
      a zrzut ekranu z sekcji 12.1 pokazuje tamtą wersję.)
- [ ] start z innego katalogu roboczego (`C:\`) i z katalogu ze znakami spoza ASCII w
      ścieżce: modele i tekstury wczytują się tak samo (w stanie M1 sprawdzone tylko dla
      shaderów)

Okno:

- [ ] zmiana rozmiaru okna myszą: obraz wypełnia okno, ściany i słupki zachowują proporcje.
      Zapisać, co dzieje się z panelami przy prawej krawędzi, gdy okno robi się węższe
- [ ] okno zmaksymalizowane **po starcie**: układ startowy jest liczony raz, w pierwszej
      klatce, z rozmiaru okna w tej chwili (1280 x 720), więc po maksymalizacji panele
      zostają w lewej górnej części okna, w tych samych miejscach co w małym oknie.
      Zapisać, jak to wygląda. Panele liczone od rogów większego okna widać dopiero wtedy,
      gdy okno jest duże już w pierwszej klatce (zmierzone tymczasową zmianą rozmiaru
      startowego, sekcja 12.1)
- [ ] ekran ze skalą 150% (Ustawienia, Ekran, Skala): tekst paneli jest 1,5 raza większy i
      ostry. W oknie 1280 x 720 zawartość paneli się nie mieści, po powiększeniu okna do
      1920 x 1080 i usunięciu `imgui.ini` układ wygląda jak przy 100%. Niesprawdzone na
      prawdziwym ekranie: ten PC ma skalę 100%
- [ ] minimalizacja i przywrócenie w trakcie chodzenia: bez linii `[error]` i bez asercji
- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo

## 13. Lista kontrolna M4: oświetlenie i mapy normalnych

Sekcje 13.1 i 13.2 to pierwsza część M4 (oświetlenie), sekcje 13.3 i 13.4 to druga część
(mapy normalnych). Pierwsza część kamienia milowego M4: trzy rodzaje świateł (księżyc, latarka gracza, światła
punktowe w ślepych zaułkach), cztery tryby cieniowania labiryntu, blok uniformów ze
światłami, dyrektywa `#include` w shaderach, panel Lights i układ siedmiu paneli. Opis kodu:
[`../modules/scene/lights.md`](../modules/scene/lights.md) (rodzaje świateł, model odbicia,
plik `common/lighting.glsl`, panel Lights),
[`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md)
(cieniowanie na wierzchołek i na fragment, Phong i Blinn-Phong, przełącznik trybu),
[`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md) (blok `LightBlock`,
układ `std140`), [`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md)
(`#include`, nazwy plików w błędach),
[`../modules/game/flashlight.md`](../modules/game/flashlight.md) (latarka, klawisz F,
światła punktowe gry). Decyzje:
[`../decisions/no-gamma-until-m7.md`](../decisions/no-gamma-until-m7.md) i
[`../decisions/dead-end-lights.md`](../decisions/dead-end-lights.md).

Kamień milowy **nie jest zamknięty**: kod obu części jest kompletny, ale części ręczne
poniżej (13.2 i 13.4) są otwarte, na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)) i
nie ma tagu. Czego w tej części nie było: cieni (światła świeciły przez ściany. Cienie
księżyca doszły w czwartej części M7, sekcja 20, cienie latarki w piątej części, sekcja 21) oraz korekcji
gamma i tekstur sRGB (doszły w pierwszej części M7, sekcja 17).
Baterii latarki w M4 też nie było:
doszła w M5 (sekcja 14).

**Sekcje 13.1 i 13.3 są zapisem stanu po M4.** M5 zmienił cztery rzeczy, o których mówią
ich punkty `[x]`. Światła punktowe nie wiszą już w ślepych zaułkach, tylko nad kryształami
(w labiryncie startowym było 11 świateł, dziś jest 13 kryształów), a notatka
[`../decisions/dead-end-lights.md`](../decisions/dead-end-lights.md) opisuje rozwiązanie
zastąpione. Kostek oznaczających światła i kostki z M1 nie ma. Programów shaderów jest
cztery, bez `basic`. Paneli jest osiem, a dolny rząd jest o 8 pikseli wyższy. M6 zmienił
kolejne: doszły niebo, trawa i dwa programy (po M6 było ich sześć), paneli było dziesięć,
a płytki podłogi, o których mówią punkty `[x]`, zastąpił teren (sekcje 15 i 16). Pierwsza
część M7 dodała bufor HDR, gammę, dwa programy i jedenasty panel oraz zmieniła wartości
startowe świateł, więc liczby świateł w punktach `[x]` tej sekcji są dawne (sekcja 17).
Druga część dodała dwa programy (sekcja 18), a czwarta cienie księżyca, jedenasty program
i dwunasty panel oraz podniosła `Moon intensity` do 0,2 (sekcja 20). Punkty `[x]`
zostają z tekstami z dnia pomiaru. Punkty otwarte w sekcjach 13.2 i 13.4 są przepisane
tak, żeby dało się je wykonać w dzisiejszym programie.

### 13.1. Zmierzone (2026-10-05)

Środowisko: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER, sterownik 610.74.

- [x] build Debug i Release: zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: wszystkie przypadki przechodzą. Liczby
      podaję dla stanu po obu częściach M4: 163 przypadki testowe i 62220
      asercji (sekcja 13.3. Dzisiejsze liczby, po M5: sekcja 14.1). Przypadki w plikach
      (policzone wtedy także jako makra `TEST_CASE`):
      `ColliderTests.cpp` 12, `ImageLoaderTests.cpp` 9, `LightingTests.cpp` 17,
      `LightTests.cpp` 20, `MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12,
      `MazeTests.cpp` 6, `MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 20, `PlayerTests.cpp`
      13, `ShaderSourceTests.cpp` 22, `TangentTests.cpp` 9, `TransformTests.cpp` 4. Z
      oświetleniem doszły cztery pliki (`ShaderSourceTests.cpp`, `LightTests.cpp`,
      `LightingTests.cpp`, `TransformTests.cpp`), z mapami normalnych piąty
      (`TangentTests.cpp`)
- [x] clang-format i clang-tidy: żadnej uwagi
- [x] clang-tidy wymagał jednej zmiany konfiguracji: linii
      `ExtraArgs: ['-D_CRT_USE_BUILTIN_OFFSETOF']` w `.clang-tidy` (ten sam przełącznik jest
      w `.clangd`, w `CompileFlags: Add`). Powód: `src/scene/LightBlock.hpp` sprawdza układ
      bloku świateł liniami `static_assert(offsetof(...) == ...)`. Na Windowsie clang-tidy
      czyta nagłówki biblioteki C Microsoftu, w których makro `offsetof` jest zapisane
      rzutowaniem wskaźnika. MSVC przyjmuje taki zapis wewnątrz `static_assert`, clang nie.
      Makro `_CRT_USE_BUILTIN_OFFSETOF` przełącza w tych nagłówkach `offsetof` na wersję
      wbudowaną w kompilator. Nie da się go ustawić w `CMakeLists.txt`: MSVC odmawia
      definiowania tej nazwy (ostrzeżenie C4117, nazwa zastrzeżona). Opis:
      [`project-structure.md`](project-structure.md), sekcje 3.6 i 3.9
- [x] gra startuje bez linii `[error]`, w tym bez żadnej z nazwą błędu OpenGL (`GL_...`), w
      buildzie Debug, w którym `GL_CHECK` jest aktywne. Z tego wynika, że pięć ówczesnych
      programów shaderów się wczytało (z `basic`, a także `lit` i `gouraud`, które dołączają
      `common/lighting.glsl`) i że nie pojawiła się linia `[error] Uniform block LightBlock
      is ... bytes in the shader, but 928 bytes in the C++ code`: sterownik NVIDII układa
      blok w tylu bajtach, ile ma struktura `scene::LightBlockData`
- [x] zrzut ekranu ze startu: nocna scena w trybie Blinn-Phong
- [x] zrzuty ekranu czterech trybów oświetlenia (`Unlit`, `Gouraud`, `Phong`,
      `Blinn-Phong`), każdy z trzech punktów widzenia
- [x] zrzut ekranu z wyłączoną latarką
- [x] zrzut ekranu ślepego zaułka z jego światłem punktowym (stan M4. Od M5 światło
      punktowe wisi nad kryształem, a w ślepym zaułku jest tylko wtedy, gdy stoi w nim
      kryształ)
- [x] zrzut ekranu ścian oświetlonych przez księżyc i ścian, do których jego światło nie
      dociera. Które to strony, wynika z kodu: przy kątach startowych (`Moon yaw` 25,
      `Moon pitch` -50) światło biegnie w kierunku około (0,27, -0,77, -0,58), więc
      oświetla podłogę i te strony ścian, które patrzą w stronę -X i +Z, a strony patrzące
      w stronę +X i -Z dostają od księżyca zero i świecą tylko światłem otoczenia
- [x] błąd wewnątrz dołączanego pliku jest pokazany z nazwą tego pliku, a obraz rysuje
      dalej poprzedni program (zrzut ekranu). Surowa linia sterownika NVIDII ma w miejscu
      nazwy numer napisu źródłowego:

  ```text
  1(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
  ```

  a w panelu Shaders i w konsoli stoi:

  ```text
  common/lighting.glsl(63) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
  ```

  Numer 63 to linia w pliku `common/lighting.glsl`, a nie w tekście po wklejeniu: pilnują
  tego dyrektywy `#line`, które program dopisuje wokół dołączonego pliku. Z kodu
  (`gfx::nameSourceFiles`) wynika też, że komunikat shadera z więcej niż jednym plikiem
  kończy się linią legendy. W chwili tego pomiaru `lit.frag` dołączał jeden plik i legenda
  miała postać `Source files: 0 = lit.frag, 1 = common/lighting.glsl`. Dziś `lit.frag`
  dołącza też `common/normal_map.glsl`, więc legenda ma trzy pozycje: tego komunikatu po
  zmianie nikt nie wywołał (punkt otwarty w sekcji 13.4)
- [x] shader bez `#include` też dostaje nazwę pliku w komunikacie: `basic.frag(4)` w
      miejscu `0(4)` (pomiar na pliku, który M5 usunął. Dziś shaderem bez `#include` jest
      na przykład `color.frag`)

Żaden z tych stanów nie był ustawiany ręką: klawiszem F, listą `Lighting`, widżetami panelu
Lights ani przyciskiem `Reload shaders`. Dlatego te same kroki są jeszcze raz na liście
otwartej. Format błędów sterownika Apple (`ERROR: 1:15:`) jest obsłużony w kodzie i
sprawdzony tylko testem jednostkowym, nie na prawdziwym sterowniku.

### 13.2. Otwarte: test ręczny na około dziesięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze zrzutów z sekcji 13.1,
nie z klikania. Nazwy widżetów są zapisane tak jak w `src/debug/panels/LightsPanel.cpp` i
`RendererPanel.cpp`. Dokładną wartość suwaka wpisuje się po kliknięciu go z wciśniętym Ctrl.

Przygotowanie:

- [ ] zamknąć program, usunąć `imgui.ini` z katalogu, z którego program będzie uruchamiany
      (przy starcie z katalogu repozytorium: z katalogu głównego repozytorium), zbudować
      (`cmake --build --preset debug`) i uruchomić `build\debug\Debug\night_maze.exe`

Układ paneli (okno 1280 x 720, bez `imgui.ini`). W M4 paneli było siedem, a dolny rząd
miał wysokość 272. Dziś jest ich dwanaście:

- [ ] układ dwunastu paneli i paska HUD: punkty z wymiarami są w sekcjach 14.2, 16.2, 17.2 [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      i 20.2. Tu wystarczy
      sprawdzić, że Lights stoi pod Rendererem w lewej kolumnie, że żaden panel nie
      zasłania innego i że środek okna, w który świeci latarka, jest wolny
- [ ] okno debug (Render / Scene): pod edytorem `Clear color` jest lista `Lighting` z wybraną pozycją
      `Blinn-Phong`
- [ ] okno debug (Light / Lights): u góry edytor koloru `Ambient`, pod nim cztery grupy z paskami: [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Moon (directional)` (zwinięta), `Flashlight (spot)`, `Point lights (crystals)` i
      `Highlight (specular)` (rozwinięte). Przy zwiniętej grupie księżyca panel pokazuje
      całą zawartość bez przewijania (tak było w M4: zapisać, czy jest tak nadal).
      Wartości startowe: `Flashlight on (key F)`
      zaznaczone, `Beam intensity` 1.30 (do M6 1.60), `Cone` z polami `inner 13.0 deg` i
      `outer 21.0 deg`, `Beam range` 16.0 m, linia `Lit: 13 of 13 crystals (at most 16)`,
      `Point intensity` 0.90 (do M6 2.00), `Point radius` 3.0 m, `Strength` 0.25, `Shininess` 32. Do M4
      trzecia grupa nazywała się `Point lights (dead ends)` i miała linię
      `In this maze: 11 (at most 16)`
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders): przycisk `Reload shaders` i jedenaście linii. Cztery pierwsze to
      `textured.vert + textured.frag: OK`,
      `color.vert + color.frag: OK`, `lit.vert + lit.frag: OK`,
      `gouraud.vert + gouraud.frag: OK` (po M5 były tylko te cztery, pozostałe siedem
      doszło w M6 i w M7: sekcje 15.2, 16.2, 17.2, 18.2 i 20.2). Najechanie myszą na linię
      pokazuje w dwóch wierszach pełne ścieżki obu plików
- [ ] rozwinięcie okna debug (Player): kliknąć strzałkę w jego pasku tytułu. Panel otwiera się w [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      dół do rozmiaru 280 x 416 i kończy się 8 pikseli nad dolnym rzędem. Zasłania lewą
      część sceny, lewą część paska HUD i żadnego innego panelu. Jest trochę niższy od
      swojej zawartości, więc
      ma pasek przewijania. Drugie kliknięcie strzałki zwija go z powrotem. Po ponownym
      uruchomieniu (już z `imgui.ini`) panel jest w tym stanie, w jakim został

Latarka:

- [ ] klawisz F wyłącza latarkę: ciepła plama na środku obrazu znika, zostają księżyc,
      światła punktowe i światło otoczenia, a pole `Flashlight on (key F)` w oknie debug (Light / Lights)
      samo się odznacza. Drugi raz F włącza ją i zaznacza pole. Klawisz działa także przy
      widocznym kursorze (tak jak N), ale nie wtedy, gdy klawiaturę ma panel (na przykład
      trwa wpisywanie wartości w polu). Od M5 latarka zużywa baterię i przy pustej baterii
      nie daje się włączyć: te kroki są w sekcji 14.2. Do tej listy wystarczy bateria,
      która nie jest pusta (pasek HUD u góry okna)
- [ ] to samo kliknięciem pola `Flashlight on (key F)` zamiast klawisza
- [ ] stożek zostaje na środku obrazu w ruchu: iść do przodu (W), bokiem (A i D) i biec
      (lewy Shift) wzdłuż ściany. Plama latarki nie spóźnia się za obrazem i nie drży.
      Wynika to z kodu (latarka jest stawiana w tym samym punkcie, z którego liczony jest
      widok klatki), nikt tego nie oglądał w ruchu

Cztery tryby (lista `Lighting` w oknie debug (Render / Scene)):

- [ ] `Unlit`: labirynt równo jasny, jak przed M4, sama tekstura. Klawisz F niczego nie
      zmienia w obrazie. Kryształy i brama są rysowane tym samym programem `textured`:
      brama równo jasna jak ściany, kryształy jaśniejsze od nich i turkusowe (uniform
      `uEmissive` rozjaśnia ich teksturę także w tym programie)
- [ ] `Gouraud`: światło liczone w wierzchołkach i rozciągane po trójkątach. Powierzchnie
      mają łagodne przejścia jasności od narożnika do narożnika, bez okrągłych plam.
      Kryształy świecą własnym kolorem także w tym trybie
- [ ] `Phong`: światło liczone dla każdego fragmentu. Latarka daje okrągłą plamę z miękkim
      brzegiem, światła punktowe okrągłe kałuże światła na podłodze i ścianach
- [ ] `Blinn-Phong` (tryb startowy): to samo co `Phong`, inny jest tylko wzór połysku
      (porównanie niżej)

Gouraud a Phong:

- [ ] na ścianie: stanąć około 2 m przed ścianą, twarzą do niej, i przełączać `Phong` i
      `Gouraud`. W `Phong` na ścianie jest okrągła plama latarki. W `Gouraud` plama znika
      albo rozmazuje się wzdłuż krawędzi trójkątów: duża ściana boczna segmentu (2 m na
      2,6 m) ma wierzchołki tylko w czterech narożnikach, a światło, które pada między
      nie, nie trafia w żaden wierzchołek. Przesuwać wzrok powoli w stronę narożnika
      ściany: w `Gouraud` jasność pojawia się dopiero wtedy, gdy stożek obejmie
      wierzchołek, i rozchodzi się od niego po trójkątach
- [ ] u podstawy słupka: skierować latarkę na podłoże przy słupku i przełączać tryby. W
      `Phong` plama jest okrągła na podłożu i na słupku. W `Gouraud` krawędź plamy na
      podłożu jest kanciasta, złożona z trójkątów siatki terenu, ale drobnych: wierzchołki
      terenu leżą co 0,5 m, więc różnica między trybami jest na podłożu dużo mniejsza niż
      na ścianie. Cokół słupka, który ma
      wierzchołki blisko siebie, wygląda podobnie w obu trybach. To oczekiwanie z
      geometrii, nie obserwacja: zapisać, co widać naprawdę. (Do M5 podłogą były płytki
      2 m na 2 m o czterech wierzchołkach i w `Gouraud` rozjaśniały się dużymi
      trójkątnymi klinami od narożnika: tego obrazu już nie ma, został na ścianach.)

Phong a Blinn-Phong (w oknie debug (Light / Lights) ustawić `Strength` 1.0 i `Shininess` 16):

- [ ] twarzą do ściany, latarka na wprost: przełączać `Phong` i `Blinn-Phong`. W
      `Blinn-Phong` jasna plama połysku na środku jest szersza i jaśniejsza niż w `Phong`
      przy tym samym wykładniku
- [ ] pod płaskim kątem do światła punktowego albo do księżyca: stanąć tak, żeby patrzeć
      wzdłuż ściany albo podłoża, ze światłem daleko z przodu. W `Blinn-Phong` połysk
      rozciąga się w podłużną smugę, w `Phong` jest mniejszy albo urywa się (wzór Phonga
      daje zero, gdy między promieniem odbitym a kierunkiem do oka jest więcej niż 90
      stopni). Po próbie przywrócić `Strength` 0.25 i `Shininess` 32

Księżyc (w oknie debug (Light / Lights) rozwinąć grupę `Moon (directional)`, panel zaczyna się wtedy
przewijać):

- [ ] wartości startowe: `Moon yaw` 25 deg, `Moon pitch` -50 deg, `Moon intensity` 0.20
      (od pierwszej do trzeciej części M7 0.12, do M6 0.30).
      Wyłączyć latarkę (F), żeby widzieć samo światło księżyca. Strony ścian patrzące w
      stronę -X i +Z są jaśniejsze, strony patrzące w stronę +X i -Z ciemne (tylko światło
      otoczenia). Na planie w oknie debug (World / Maze) północ to -Z, czyli góra planu, a +X to prawa
      strona. Od czwartej części M7 ściany rzucają w tym świetle cień, więc także strona
      zwrócona do księżyca i podłoże są ciemne tam, gdzie zasłania je inna ściana. Żeby
      zobaczyć samą zależność od kierunku, bez cieni, odznaczyć pole `Shadows` w oknie debug
      (Light / Shadows) (sekcja 20.2)
- [ ] `Moon yaw` (suwak od 0 do 360): kąt mówi, w którą stronę światło biegnie. Przy 205
      (o 180 więcej niż na starcie) jasne i ciemne strony ścian zamieniają się miejscami.
      Przy 90 światło biegnie w stronę +X: ze stron ścian jasne są tylko te, które patrzą w
      stronę -X
- [ ] `Moon pitch` (suwak od -90 do -5): przy -90 światło pada prosto w dół, podłoże jest
      najjaśniejsze, a żadna pionowa strona ściany nie dostaje światła księżyca. Przy -5
      światło ledwie muska podłoże, a ściany zwrócone do księżyca są najjaśniejsze: tak
      jest przy odznaczonym polu `Shadows` w oknie debug (Light / Shadows). Przy zaznaczonym cienie są
      przy -90 schowane pod ścianami, a przy -5 bardzo długie: zakrywają większość podłoża
      w korytarzach i te ściany zwrócone do księżyca, przed którymi stoi inna ściana.
      Jasne zostają ściany, których od strony księżyca nic nie zasłania
- [ ] `Moon intensity` 0 wyłącza księżyc, a razem z nim znikają jego cienie. Przy
      wartości startowej i zaznaczonym polu `Shadows` podłoże i ściany, które stoją za inną
      ścianą, światła księżyca nie dostają (do trzeciej części M7 cieni nie było i księżyc
      oświetlał także je). Po odznaczeniu pola `Shadows` jest jak dawniej

Światła punktowe:

- [ ] policzyć źródła świateł: klawisz N, wznieść się spacją nad ściany i spojrzeć w dół.
      Świecących turkusowych kryształów jest 13, tyle, ile pokazuje linia
      `Lit: 13 of 13 crystals (at most 16)` i ile kropek ma plan w oknie debug (World / Maze). Każdy
      unosi się nad środkiem swojej komórki, a jego światło wisi 0,15 m nad jego czubkiem
      (około 1,55 m nad podłożem w środku komórki). Komórka startowa (lewy górny róg planu) i komórka
      wyjścia (zielony prostokąt na planie) kryształu nie mają. W M4 źródłami były
      turkusowe kostki w ślepych zaułkach, 11 w tym labiryncie: tych kostek już nie ma
- [ ] `Point radius` (suwak od 0.5 do 12.0 m): większy promień powiększa kałuże światła.
      Światła punktowe nie rzucają cieni (cień rzuca tylko księżyc), więc przy dużym
      promieniu światło widać także w sąsiednich korytarzach, za ścianą. `Point intensity` 0
      gasi światła wokół kryształów, ale same kryształy świecą dalej: ich blask jest liczony
      z `Point colour`, nie z natężenia
- [ ] `Point colour` zmienia naraz kolor świateł i kolor, którym świecą kryształy

Stożek i zasięg latarki:

- [ ] `Cone`: dwa pola przeciągane myszą, `inner` i `outer`, w stopniach od osi stożka (od
      1 do 60). Większe `outer` poszerza plamę. `inner` bliskie `outer` daje ostry brzeg,
      `inner` dużo mniejsze od `outer` szeroki, miękki brzeg. Pola `inner` nie da się
      przeciągnąć powyżej `outer`
- [ ] `Beam range` (suwak od 2.0 do 60.0 m): mała wartość sprawia, że latarka oświetla
      tylko najbliższe ściany, duża rozjaśnia koniec długiego korytarza

Regeneracja:

- [ ] w oknie debug (World / Maze) ustawić inne ziarno (albo kliknąć `Random seed`) i `Regenerate`:
      kryształy i ich światła są w komórkach nowego labiryntu, a liczby w linii
      `Lit: ...` i w linii `Crystals: ...` okna debug (World / Maze) odpowiadają nowemu planowi.
      Ustawienia z okna debug (Light / Lights) i tryb
      `Lighting` zostają bez zmian, poza jednym: nowa runda włącza latarkę
- [ ] `Width` 4, `Height` 4, `Seed` 1, `Regenerate`: linia [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Lit: 2 of 2 crystals (at most 16)`, w oknie debug (World / Maze) `Crystals: 2, exit in cell (3, 1)`,
      na pasku HUD `0 / 2` i `(of 2)`. Kryształy wiszą w lewym dolnym rogu planu (komórka
      (0, 3), ślepy zaułek) i w drugiej kolumnie drugiego rzędu od góry (komórka (1, 1)),
      a wyjście jest w prawej kolumnie w drugim rzędzie od góry, z bramą od północy
      (pilnują tego testy `golden maze: 4 x 4 cells from seed 1 has exactly these two
      crystals` i `golden maze: 4 x 4 cells from seed 1 has its exit in the dead end
      (3, 1)`)
- [ ] `Width` 40, `Height` 40, `Regenerate`: kryształów jest dokładnie 16
      (`Lit: 16 of 16 crystals (at most 16)`: jeden na osiem komórek dałby 200, a shader ma
      miejsce na 16 świateł), a na pasku HUD stoi `0 / 12` i `(of 16)`. Zapisać, jak
      kryształy są rozłożone na planie, oraz FPS z panelu
      Renderer w trybach `Gouraud` i `Blinn-Phong`

Błąd w dołączanym pliku:

- [ ] przy działającym programie zrobić celowy błąd w
      `assets\shaders\common\lighting.glsl` (na przykład usunąć średnik na końcu linii
      `return max(dot(normal, toLight), 0.0);`), w drugim terminalu
      `cmake --build --preset debug --target copy_assets`, potem kliknąć `Reload shaders`.
      Oczekiwane: linie `lit.vert + lit.frag: FAILED, the previous program stays in use`,
      `gouraud.vert + gouraud.frag: FAILED, the previous program stays in use` i linia
      programu `grass` (jego `grass.frag` też dołącza ten plik) są
      czerwone, pod każdą jest czerwony komunikat, który nazywa plik `common/lighting.glsl`
      i numer linii w tym pliku (sterownik może wskazać linię następną po usuniętym
      średniku), osiem pozostałych linii ma `: OK`, obraz się nie
      zmienia. Te same
      komunikaty są w konsoli jako `[error]`
- [ ] naprawa: `git checkout assets/shaders`, znowu
      `cmake --build --preset debug --target copy_assets` i `Reload shaders`. Oczekiwane:
      wszystkie jedenaście linii kończy się napisem `: OK`, a `git status` nie pokazuje
      zmienionych plików w `assets/shaders`

Widoki debug w trybie z oświetleniem:

- [ ] przy `Lighting` równym `Blinn-Phong` wybrać w oknie debug (Render / Textures and normals) `View mode`
      `Normals as colour`, potem `UVs as colour`. Labirynt jest wtedy rysowany programem
      `textured`, bez świateł, tak samo jak w trybie `Unlit`, a razem z nim kryształy i
      brama, pokolorowane według tej samej reguły co ściany i bez własnego blasku
      (`uEmissive` działa tylko w widoku `Textured`). Widok normalnych pokazuje normalne
      używane przez wybrany tryb: z map
      normalnych w trybach `Unlit`, `Phong` i `Blinn-Phong`, z samej siatki w trybie
      `Gouraud` (sekcja 13.4). `Textured` przywraca oświetlony obraz

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo, w szczególności żadna z `GL_INVALID_...` po kilku przeładowaniach shaderów

### 13.3. Zmierzone: mapy normalnych (2026-10-05)

Druga część M4. Środowisko to samo: MSVC 19.44, karta NVIDIA GeForce RTX 4070 Ti SUPER,
sterownik 610.74. Opis kodu: [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md),
decyzja: [`../decisions/tangents-on-load.md`](../decisions/tangents-on-load.md).

- [x] build Debug i Release: zero ostrzeżeń pod `/W4 /permissive-`
- [x] `night_maze_tests.exe` w Debug i w Release: 163 przypadki testowe, 62220 asercji,
      wszystkie przechodzą. Doszło 14 przypadków: nowy plik `TangentTests.cpp` (9 przypadków,
      177 asercji), po dwa w `ObjLoaderTests.cpp` (po tej części 20 i 1576) i
      `ImageLoaderTests.cpp` (po tej części 9 i 57), jeden w `LightingTests.cpp` (po tej
      części 17 i 133). Liczby z plików osobno:
      opcja `--source-file`, Debug. To liczby z M4: dla stanu po M5 mam liczby przypadków
      w plikach (sekcja 14.1), a liczb asercji w plikach nie
- [x] clang-format i clang-tidy: żadnej uwagi
- [x] gra startuje bez linii `[error]`, w tym bez żadnej z nazwą błędu OpenGL (`GL_...`), w
      buildzie Debug. Z tego wynika, że programy `lit` i `textured` skompilowały się z
      drugim plikiem dołączanym `common/normal_map.glsl`, że obie mapy normalnych się
      wczytały i że żaden model nie dostał ostrzeżenia o lustrzanych trójkątach
- [x] kierunek reliefu na zrzutach ekranu: fugi czytają się jako rowki, a nie jako wałki, na
      ścianach wzdłuż osi X, na ścianach wzdłuż osi Z (obróconych o 90 stopni), na słupku i
      na podłodze
- [x] reakcja na kierunek światła: po przejściu światła z lewej strony na prawą jasne i
      ciemne skosy fug zamieniają się miejscami
- [x] średnia jasność zrzutu (skala od 0 do 255) z mapami normalnych i bez nich jest prawie
      równa: ściana wzdłuż X 43,29 i 44,00, ściana wzdłuż Z 35,85 i 36,16, słupek 35,26 i
      35,55, podłoga 22,69 i 22,86. Mapa przesuwa światło między skosami, nie przyciemnia
      sceny
- [x] tryby `Gouraud` i `Unlit`: zrzuty ekranu identyczne co do piksela z mapami włączonymi
      i wyłączonymi
- [x] skrypty Blendera są powtarzalne: dwa kolejne uruchomienia skryptu tekstur i skryptów
      modeli dały identyczne skróty wszystkich dziesięciu ówczesnych plików wyjściowych
      (4 PNG, 3 OBJ, 3 MTL. Od M5 skrypty piszą 8 PNG, 6 OBJ i 6 MTL: dla nich tej próby
      nie zapisano). Obrazy koloru i pliki `.obj` są bajt w bajt takie same jak przed tą częścią,
      każdy plik `.mtl` dostał jedną linię `map_Bump`
- [x] liczba wierzchołków i indeksów modeli bez zmian: ściana i słupek 60 i 90, podłoga 4 i
      6 (styczne nie dodają wierzchołków)

Żaden z tych stanów nie był ustawiany kliknięciem w panelu: pola `Normal mapping` nikt
jeszcze nie kliknął ręką. Znane ograniczenia obrazu (podwójnie ciemne fugi, słaba siatka w
ziarnie przy bardzo płaskim kącie światła, migotanie w oddali ocenione tylko na
nieruchomych klatkach) opisuje [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md),
sekcja 2.12.

### 13.4. Otwarte: mapy normalnych, test ręczny na około pięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu, z testów
jednostkowych i ze zrzutów z sekcji 13.3. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/AssetsPanel.cpp`. Program uruchomiony jak w sekcji 13.2, bez `imgui.ini`.

- [ ] układ: okno debug (Render / Textures and normals) stoi w prawej kolumnie pod oknem debug (World / Maze). Pod listą `View mode`
      jest pole `Normal mapping` (zaznaczone) i notatka, pod nimi `Filter` i
      `Anisotropy`. Sprawdzić, czy panel nie zasłania innego i czy do list `Models` i
      `Textures` trzeba przewijać (panel jest niski, przewijanie jest spodziewane)
- [ ] okno debug (Diagnostics / Assets), lista `Models`: pod każdą częścią modelu jest linia
      `normal map: wall_stone_normal.png` (ściana i słupek), a od M5 także
      `normal map: crystal_normal.png` (oba kryształy) i
      `normal map: gate_wood_normal.png` (brama). Linii
      `normal map: floor_stone_normal.png` płytki podłogi już nie ma: M6 usunął model
- [ ] okno debug (Diagnostics / Assets), lista `Textures`: osiem tekstur z podglądem, każda 512 x 512:
      `wall_stone.png`, `crystal.png`, `gate_wood.png`, `ground.png` (od M6, w miejscu
      `floor_stone.png`) i cztery mapy
      normalnych o tych samych nazwach z końcówką `_normal`
      (kolejność według wczytania może być inna). Podglądy map normalnych są jasnoniebieskie
      i stoją prosto, a na mapie kamienia widać kolorowe kreski fug
- [ ] `Lighting` równe `Blinn-Phong`, podejść do ściany z włączoną latarką: fugi są rowkami.
      Odznaczyć `Normal mapping`: ściana staje się płaska, plama latarki przesuwa się po
      rysunku kamieni. Zaznaczyć: relief wraca od razu, bez przeładowania
- [ ] to samo przy `Lighting` równym `Phong`
- [ ] stanąć blisko ściany i patrzeć wzdłuż niej, tak żeby latarka świeciła pod płaskim
      kątem: relief jest najmocniejszy. Zrobić krok tak, żeby światło padało z drugiej
      strony: jasne i ciemne skosy zamieniają się miejscami. Zanotować, czy w ziarnie widać
      regularną siatkę i czy przeszkadza
- [ ] przejść korytarzem i obserwować dalekie ściany i podłoże: czy relief migocze w ruchu.
      Powtórzyć z `Filter` równym `Nearest` i `Trilinear` oraz z `Anisotropy` 1 i 16
      (ustawienia działają także na mapy normalnych)
- [ ] `View mode` równe `Normals as colour` przy `Lighting` równym `Blinn-Phong`: na każdej
      ścianie widać kolor podstawowy z rysunkiem fug w innych odcieniach. Odznaczyć
      `Normal mapping`: każda ściana ma jeden jednolity kolor. Zaznaczyć z powrotem
- [ ] `View mode` równe `Normals as colour` przy `Lighting` równym `Gouraud`: ściany są
      jednolite także przy zaznaczonym `Normal mapping` (widok pokazuje normalne, których
      używa wybrany tryb). Przy `Unlit` rysunek fug wraca
- [ ] `View mode` równe `Textured`, `Lighting` równe `Gouraud`: przełączanie
      `Normal mapping` niczego nie zmienia. To samo przy `Unlit`
- [ ] brakująca mapa normalnych: zamknąć program, zmienić nazwę kopii
      `build\debug\Debug\assets\textures\wall_stone_normal.png` (kopii, nie pliku w
      repozytorium), uruchomić. Oczekiwane: ściany i słupki mają teksturę koloru, ale pod
      latarką są płaskie, bez rowków fug (płaska mapa zastępcza), podłoże i brama bez
      zmian, w konsoli jedna linia `[error]`, w
      okna debug (Diagnostics / Assets) przy części `wall_stone` napis `normal map: none (flat)` i sekcja
      `Failed to load` z nazwą pliku. Przywrócić nazwę pliku. (Do M5 ten punkt używał
      mapy `floor_stone_normal.png` płytki podłogi.)
- [ ] `Reload shaders` przy włączonych mapach: wszystkie linie (dziś jedenaście) kończą
      się napisem `: OK`, relief nie znika (numery jednostek i przełącznik są wysyłane w
      każdej klatce)
- [ ] błąd w drugim pliku dołączanym: dopisać literę w `common/normal_map.glsl` w kopii
      `assets` obok programu, `Reload shaders`. Oczekiwane: programy `lit` i `textured` mają
      `FAILED` z nazwą `common/normal_map.glsl` i numerem linii w tym pliku, pozostałe
      dziewięć (w tym `color` i `gouraud`) ma `: OK`, obraz się nie zmienia. Cofnąć zmianę i
      przeładować
- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tymi wywołanymi
      celowo

## 14. Lista kontrolna M5: rozgrywka

Kamień milowy M5 zamienia spacer po oświetlonym labiryncie w rundę: w komórkach wiszą
kryształy, każdy zebrany doładowuje baterię latarki, po zebraniu wystarczającej liczby
otwiera się brama, a przejście przez nią do komórki wyjścia kończy rundę wygraną. Doszły:
kule jako drugi kształt kolizji, wyjście w komórce najdalszej od startu, kryształy i brama
jako modele, bateria latarki z migotaniem, pasek HUD z kartą wygranej, panel Gameplay,
klawisz R i układ ośmiu paneli. Zniknęły: kostka z M1 z programem `basic`, kostki
oznaczające światła i światła w ślepych zaułkach. Opis kodu:
[`../modules/game/gameplay.md`](../modules/game/gameplay.md) (reguły rundy, wyjście,
kryształy, HUD i panel Gameplay), [`../modules/scene/collision.md`](../modules/scene/collision.md)
(kule i ich rysowanie liniami), [`../modules/game/flashlight.md`](../modules/game/flashlight.md)
(bateria, migotanie, światła kryształów),
[`../modules/game/maze-rendering.md`](../modules/game/maze-rendering.md) (wspólne rysowanie
modeli). Decyzje:
[`../decisions/battery-darkness-no-loss.md`](../decisions/battery-darkness-no-loss.md),
[`../decisions/crystal-count-and-gate-threshold.md`](../decisions/crystal-count-and-gate-threshold.md),
[`../decisions/exit-farthest-cell.md`](../decisions/exit-farthest-cell.md) i
[`../decisions/enemy-after-m5.md`](../decisions/enemy-after-m5.md).

Kamień milowy **nie jest zamknięty**: kod jest kompletny na Windowsie, ale część ręczna
poniżej (14.2) jest otwarta w całości, na macOS kod nie był budowany
([`build-macos.md`](build-macos.md)) i nie ma tagu. Czego w M5 nie ma: stanu przegranej
(pusta bateria oznacza tylko ciemność, runda trwa dalej), przeciwnika (jest w planie na
później, kodu nie ma), cieni (cienie księżyca doszły w czwartej części M7, sekcja 20,
cienie latarki w piątej, sekcja 21) i korekcji gamma (doszła w pierwszej części M7, sekcja 17).

### 14.1. Zmierzone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla M5 nie
zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).

- [x] build Debug i Release: zero ostrzeżeń
- [x] `night_maze_tests.exe` w Debug i w Release: 215 przypadków testowych i 85098 asercji,
      wszystkie przechodzą. Przypadki w plikach (liczba makr `TEST_CASE` w pliku):
      `ColliderTests.cpp` 19, `CrystalTests.cpp` 14, `ExitTests.cpp` 11,
      `ImageLoaderTests.cpp` 9, `LightingTests.cpp` 10, `LightTests.cpp` 20,
      `MazeGeneratorTests.cpp` 11, `MazeLayoutTests.cpp` 12, `MazeTests.cpp` 8,
      `MazeWorldTests.cpp` 8, `ObjLoaderTests.cpp` 20, `PlayerTests.cpp` 13,
      `RoundTests.cpp` 25, `ShaderSourceTests.cpp` 22, `TangentTests.cpp` 9,
      `TransformTests.cpp` 4. Względem M4 (163 przypadki, 62220 asercji) doszły trzy pliki
      (`ExitTests.cpp`, `CrystalTests.cpp`, `RoundTests.cpp`) i siedem przypadków kul w
      `ColliderTests.cpp`, `MazeTests.cpp` ma o dwa przypadki więcej (test ślepego zaułka
      przeniesiony razem z funkcją `isDeadEnd` i test porównania `MazeCell`), a
      `LightingTests.cpp` o siedem mniej (ten jeden przeniesiony, a sześć testów świateł w
      ślepych zaułkach usuniętych razem z tym kodem). Liczb asercji w poszczególnych plikach dla M5 nie zapisano
- [x] obraz sprawdzony na zrzutach ekranu. Zrzuty powstały przez tymczasowe wstawki w
      kodzie, które po pomiarze zostały usunięte. Listy zrzutów (jakie stany, z jakich
      miejsc) nie zapisano

Czego dla M5 nie zapisano i czego dlatego tu nie twierdzę: wyniku clang-format i
clang-tidy, tego, czy gra startuje bez linii `[error]`, oraz liczby i treści zrzutów
ekranu. Te punkty są na liście otwartej niżej.

Żaden stan rundy nie był ustawiany ręką: klawiszem R, klawiszem F przy pustej baterii,
wejściem w kryształ, przejściem przez bramę ani widżetami panelu Gameplay. Dlatego całą
rozgrywkę trzeba przejść jeszcze raz według listy otwartej.

### 14.2. Otwarte: test ręczny na około dwadzieścia minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu i z testów jednostkowych (`tests/RoundTests.cpp`,
`CrystalTests.cpp`, `ExitTests.cpp`), nie z klikania. Nazwy widżetów i napisy są zapisane
tak jak w `src/debug/panels/GameplayPanel.cpp`, `src/debug/Hud.cpp`, `LightsPanel.cpp`,
`CollisionPanel.cpp` i `MazePanel.cpp`. Liczby dotyczą labiryntu startowego (10 na 10,
ziarno 1): 13 kryształów, brama otwiera się po dziesiątym, wyjście jest w komórce (6, 5).

Trzy rzeczy ułatwiają przejście listy. Dokładną wartość suwaka wpisuje się po kliknięciu go
z wciśniętym Ctrl, ale dopóki trwa wpisywanie, klawiaturę ma panel: klawisze R, F i N nie
działają, dopóki nie zatwierdzę wartości klawiszem Enter. Okno debug (Gameplay) startuje zwinięty (od 2026-10-06 bez odpowiednika: okno debug nie zwija się do paska tytułu):
rozwija go strzałka w pasku tytułu. Kryształy na planie w oknie debug (World / Maze) to turkusowe kropki,
więc plan pokazuje, dokąd iść.

Przygotowanie:

- [ ] zamknąć program, usunąć `imgui.ini` z katalogu, z którego program będzie uruchamiany
      (przy starcie z katalogu repozytorium: z katalogu głównego repozytorium), zbudować
      (`cmake --build --preset debug`) i uruchomić `build\debug\Debug\night_maze.exe`
- [ ] na starcie w konsoli nie ma żadnej linii `[error]`, w tym żadnej z nazwą błędu OpenGL
      (`GL_...`). Są linie `[info] Loaded model: ...` dla pięciu modeli (w M5 było ich
      sześć, z płytką podłogi) i
      `[info] Loaded texture: ...` dla ośmiu tekstur. Dla M5 tego wyniku nie zapisano

Układ paneli i pasek HUD (okno 1280 x 720, bez `imgui.ini`). W M5 paneli było osiem, dziś
jest ich dwanaście (jedenasty, Framebuffers, opisuje sekcja 17.2, dwunasty, Shadows,
sekcja 20.2), a wymiary niżej są dzisiejsze, ze stałych w `src/debug/PanelLayout.hpp`:

- [ ] lewa kolumna: Renderer (336 x 284) nad Lights (336 x 412). Prawa kolumna: Maze [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      (300 x 480) nad Assets (300 x 216). Dolny rząd między kolumnami: Collision
      (312 x 280) i Shaders (292 x 280). U góry, między kolumnami, dwa paski tytułu obok
      siebie: Camera, a na prawo od niego Gameplay, oba zwinięte. Pod nimi drugi rząd:
      Terrain pod Camera i Grass pod Gameplay, też zwinięte (sekcja 16.2). Pod nimi
      trzeci i czwarty rząd: szerokie paski Framebuffers (sekcja 17.2) i Shadows (sekcja
      20.2), oba zwinięte. Żaden panel nie
      zasłania innego, a środek okna, w który świeci latarka, jest wolny. (W M5 Renderer
      miał 336 x 230, a Lights 336 x 466: Renderer urósł razem z kontrolkami nieba.)
- [ ] [stan na dziś: od okna debug (World / Reflections) rzędów jest pięć, a od 2026-10-06 przy schowanych panelach HUD stoi przy górnej krawędzi; liczby w tym punkcie to stan z czwartej części M7] pasek HUD stoi na środku górnej krawędzi okna, pod czterema rzędami pasków tytułu [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      (Camera i Gameplay, pod nimi Terrain i Grass, pod nimi Framebuffers, a pod nim
      Shadows: od czwartej części M7 `FOLDED_ROW_COUNT` wynosi 4, od pierwszej do
      trzeciej było 3). Odległość od góry to cztery razy
      wysokość paska tytułu z odstępem 8 pikseli i jeszcze 16 pikseli (`foldedRowsHeight`
      i `HUD_TOP_OFFSET`): wysokość paska zależy od czcionki. W M5, z jednym rzędem,
      było 46 pikseli od góry, a zgłoszone przesunięcie o jeden rząd to około 30 pikseli
      (sekcja 16.2), więc przy czterech rzędach wychodzi około 136 pikseli: policzone z
      tych dwóch liczb, nie zmierzone. Pierwsza linia: turkusowy napis `Crystals`, liczby
      `0 / 10`, przygaszony napis `(of 13)` i przygaszony czas `0:00`, który rośnie co
      sekundę. Druga
      linia: bursztynowy pasek baterii i napis `100%`, który powoli maleje. Trzeciej linii
      (podpowiedzi) nie ma. Zapisać, czy pasek HUD nie nachodzi na paski tytułu nad nim
- [ ] rozwinięcie okna debug (Gameplay): kliknąć strzałkę w jego pasku tytułu. Panel otwiera się [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      w dół do rozmiaru 324 x 416 i kończy się 8 pikseli nad dolnym rzędem. Zasłania prawą
      część paska HUD (pasek zostaje pod panelami, flaga
      `ImGuiWindowFlags_NoBringToFrontOnFocus`) i zwinięty pasek tytułu okna debug (World / Terrain and grass / Grass) pod
      sobą, ale żadnego innego panelu. Zawartość od góry:
      `Round: playing, ... s`, `Crystals: 0 collected, 10 needed, 13 in the maze`,
      `Gate: closed`, przycisk `Restart round (key R)`, suwak `Battery` (na starcie 1.00,
      maleje), zaznaczone pole `Battery drains`, suwaki `Crystals needed` (`0.70 of all`),
      `Battery lifetime` (`180 s`), `Recharge` (`0.25`), `Flicker below` (`0.20`) i
      `Pickup radius` (`0.60 m`). Zapisać, czy zawartość mieści się bez przewijania
- [ ] okno debug (World / Maze): pod linią `Walls: 121, pillars: 121` jest linia [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Crystals: 13, exit in cell (6, 5)`. Na planie: 13 turkusowych kropek, zielony
      prostokąt w siódmej kolumnie i szóstym rzędzie (komórki liczy się od zera, od lewego
      górnego rogu) i gruba brązowa kreska na prawej (wschodniej) krawędzi tej komórki. W
      komórce startowej i w komórce wyjścia kropki nie ma
- [ ] okno debug (Diagnostics / Collision and picking): pole `Draw collision shapes` (odznaczone), pod nim legenda
      `Yellow: walls, pillars. Green: player. Orange: gate. Cyan: crystal pickup. Magenta:
      exit zone.`, pole `Noclip (key N)`, linie `Boxes: 121 walls, 121 pillars, 1 gate` i
      `All boxes: 243, pickup spheres: 13`
- [ ] okno debug (Light / Lights): trzecia grupa nazywa się `Point lights (crystals)`, jej pierwsza linia
      to `Lit: 13 of 13 crystals (at most 16)`
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders): jedenaście linii zakończonych `: OK` (`textured`, `color`, `lit`,
      `gouraud`, od M6 `skybox` i `grass`, ta ostatnia z trzema plikami, a od M7 pięć
      kolejnych: sekcje 17.2, 18.2 i 20.2). W M5 linii było cztery
- [ ] okno debug (Diagnostics / Assets): pięć modeli i osiem tekstur (lista w sekcji 12.2)

Kryształy i brama w obrazie:

- [ ] podejść do najbliższego kryształu. Kryształ unosi się nad środkiem komórki (podstawa
      około 0,9 m nad podłożem w środku komórki, wysokość 0,5 m), kołysze się w górę i w dół o 8 cm raz na
      3 sekundy i obraca wokół osi pionowej (pełny obrót w 9 sekund). Świeci na turkusowo
      także tam, gdzie nie pada na niego żadne światło, a wokół niego na podłożu, trawie i
      ścianach leży turkusowa plama jego światła punktowego. Blask kryształu i plama
      pulsują razem, raz na 2,4 sekundy
- [ ] obejrzeć kilka kryształów: są dwa kształty (jeden wysoki odłamek, grupa trzech
      odłamków na podstawie). Dwa kryształy widziane naraz nie kołyszą się równo, ale
      pulsują równo
- [ ] dojść do bramy: drewniana, szeroka na 2 m i wysoka na 2,75 m (niższa od ścian),
      stoi między dwoma słupkami na jedynym otwartym boku komórki wyjścia. Gracz zatrzymuje
      się na niej tak jak na ścianie
- [ ] przełączyć listę `Lighting` w oknie debug (Render / Scene): kryształy i brama są rysowane tym
      samym programem co ściany, więc w każdym trybie są cieniowane tak jak one

HUD przy ukrytych panelach:

- [ ] klawisz na lewo od `1` (akcent słaby, na klawiaturze amerykańskiej znaki `` ` `` i [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `~`, w kodzie `GLFW_KEY_GRAVE_ACCENT` w `src/main.cpp`): dwanaście paneli znika, pasek HUD
      zostaje (od 2026-10-06 przesuwa się wtedy do górnej krawędzi okna), czas rośnie dalej. Drugie naciśnięcie przywraca panele (i HUD wraca pod rzędy pasków). Klawisz działa przy
      wolnym i przy przechwyconym kursorze
- [ ] przy ukrytych panelach zebrać kryształ (krok niżej): liczba na pasku HUD rośnie
- [ ] pasek HUD nie przyjmuje myszy: kliknięcie w niego przy wolnym kursorze przechwytuje
      kursor tak samo jak kliknięcie w scenę (oczekiwanie z flagi
      `ImGuiWindowFlags_NoInputs`: zapisać wynik)

Zbieranie kryształów:

- [ ] wejść w kryształ. Znika, zanim gracz dojdzie do środka komórki: wystarcza odległość [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      około 0,86 m w poziomie od środka komórki (kula zasięgu gracza o promieniu 0,3 m
      nachodzi na kulę kryształu o promieniu 0,6 m, a ich środki dzieli 0,25 m wysokości).
      Jednocześnie: gaśnie turkusowa plama tego kryształu, pasek HUD pokazuje `1 / 10`, na
      planie w oknie debug (World / Maze) kropka zamienia się w ciemny pierścień, okno debug (Light / Lights) pokazuje
      `Lit: 12 of 13 crystals (at most 16)`, okno debug (Diagnostics / Collision and picking) `pickup spheres: 12`, a panel
      Gameplay `Crystals: 1 collected, 10 needed, 13 in the maze`
- [ ] doładowanie: w oknie debug (Gameplay) odznaczyć `Battery drains`, ustawić `Battery` na
      0.50 i zebrać kryształ. Oczekiwane: `Battery` 0.75, na pasku HUD `75%`. Zaznaczyć
      `Battery drains` z powrotem. Przy baterii powyżej 75% kryształ dopełnia ją tylko do
      `100%`, więcej się nie zmieści
- [ ] `Recharge` (suwak od 0.00 do 1.00): przy 0.00 zebrany kryształ nie zmienia baterii,
      przy 1.00 ładuje ją do pełna. Przywrócić 0.25
- [ ] `Pickup radius` z zaznaczonym `Draw collision shapes`: wokół każdego niezebranego
      kryształu jest turkusowa kula narysowana trzema okręgami (jeden poziomy, dwa
      pionowe). Kula stoi w miejscu, a kryształ kołysze się w jej środku. Po spojrzeniu pod
      nogi widać zieloną kulę zasięgu gracza w środku zielonego pudełka. Suwak (od 0.10 do
      2.00 m) zmienia rozmiar turkusowych kul od razu. Przy 2.00 kryształ zbiera się ze
      środka sąsiedniej komórki, przez ścianę. Przy 0.10 trzeba stanąć prawie na środku
      komórki kryształu (bliżej niż około 0,3 m). Przywrócić 0.60

Bateria i latarka:

- [ ] zużycie: przy włączonej latarce liczba na pasku HUD maleje, przy startowym
      `Battery lifetime` 180 s o 10 punktów procentowych na 18 sekund. Wyłączyć latarkę
      klawiszem F: liczba stoi w miejscu. Włączyć z powrotem
- [ ] `Battery lifetime` (suwak od 5 do 600 s): ustawić 5 s. Pełna bateria wyczerpuje się w
      5 sekund świecenia. Przywrócić 180 s
- [ ] `Battery drains` odznaczone: bateria nie maleje także przy włączonej latarce
- [ ] migotanie na ekranie: odznaczyć `Battery drains`, ustawić `Battery` na 0.10, latarka
      włączona, stanąć twarzą do ściany. Oczekiwane: pasek baterii na HUD jest czerwony
      (poniżej progu 0.20), a plama latarki co chwilę przygasa w nieregularnych odstępach
      i wraca, najgłębiej o mniej więcej 40% jasności. Ustawić 0.02: przygasa głębiej,
      prawie o 80%. Ustawić 0.25: światło jest równe, pasek bursztynowy. Migotania nie
      mylić z pulsowaniem świateł kryształów, które jest powolne i równe
- [ ] `Flicker below` (suwak od 0.00 do 0.50): przy 0.00 latarka nie migocze nawet przy
      `Battery` 0.02, a pasek nie robi się czerwony. Przy 0.50 migocze już przy `Battery`
      0.40. Przywrócić 0.20
- [ ] pusta bateria: ustawić `Battery` na 0.00 (`Battery drains` może zostać odznaczone).
      Oczekiwane: plama latarki znika, zostają księżyc, światła kryształów i światło
      otoczenia. Pasek HUD pokazuje pusty pasek, `0%` i czerwoną podpowiedź
      `Battery empty. Find a crystal.`. W oknie debug (Light / Lights) pole `Flashlight on (key F)` samo
      się odznacza, a po najechaniu na nie myszą pojawia się podpowiedź
      `The battery is empty: collect a crystal first.`. Runda trwa dalej: czas rośnie,
      gracz chodzi, okno debug (Gameplay) pokazuje `Round: playing`. Stanu przegranej nie ma
- [ ] klawisz F przy pustej baterii: latarka się nie zapala, w żadnej klatce. Kliknięcie
      pola `Flashlight on (key F)` też jej nie zapala: pole odznacza się z powrotem po
      najbliższym kroku symulacji (może mignąć)
- [ ] zebrać kryształ przy pustej baterii: `Battery` 0.25, na pasku HUD `25%`, pasek
      bursztynowy (0.25 nie jest poniżej progu 0.20), podpowiedź znika. Latarka nadal jest
      wyłączona: kryształ jej sam nie zapala. Nacisnąć F: latarka świeci

Brama:

- [ ] `Crystals needed` (suwak od 0.05 do 1.00) zmienia liczbę potrzebnych kryształów od
      razu: przy `1.00 of all` pasek HUD pokazuje `/ 13`, przy 0.50 `/ 7`, przy 0.05 `/ 1`.
      Przywrócić 0.70 (`/ 10`). Ta sama liczba jest w oknie debug (Gameplay) (`... needed`)
- [ ] otwarta brama zostaje otwarta: zebrać trzy kryształy, obniżyć `Crystals needed` do
      0.20 (potrzebne 3). Brama otwiera się od razu. Podnieść suwak do 1.00: pasek HUD
      pokazuje `3 / 13`, a brama zostaje otwarta (`Gate: open`). Nacisnąć R, przywrócić
      0.70
- [ ] opadanie bramy na oczach: stanąć przed zamkniętą bramą (na planie: gruba brązowa
      kreska), zebrać wcześniej jeden kryształ i przesunąć `Crystals needed` na 0.05.
      Oczekiwane: brama zjeżdża w ziemię i znika pod nią w 1,5 sekundy, okno debug (Gameplay)
      pokazuje w tym czasie `Gate: opening, N%` z rosnącą liczbą, potem `Gate: open`.
      Przejść przez próg da się od pierwszej chwili, zanim brama zjedzie: pudełko bramy
      przestaje być przeszkodą w chwili otwarcia. Nacisnąć R, przywrócić 0.70
- [ ] brama po dziesiątym z 13 kryształów (suwak na 0.70): po zebraniu dziewiątego pasek
      HUD pokazuje `9 / 10` i nie ma podpowiedzi. Po dziesiątym: `10 / 10` i turkusowa
      podpowiedź `The gate is open. Find the exit.`. Okno debug (Gameplay): `Gate: opening, N%`,
      po 1,5 sekundy `Gate: open`. Okno debug (Diagnostics / Collision and picking): `Boxes: 121 walls, 121 pillars, 0 gate`
      i `All boxes: 242, pickup spheres: 3`. Na planie kreska bramy robi się ciemna. Z
      zaznaczonym `Draw collision shapes` pomarańczowe pudełko bramy znika od razu
- [ ] brama otwarta przy pustej baterii: na pasku HUD są obie podpowiedzi naraz, jedna pod
      drugą

Wyjście i wygrana:

- [ ] z zaznaczonym `Draw collision shapes` obejrzeć komórkę wyjścia: w jej środku stoi
      pudełko w kolorze magenty, 1 na 1 m na podłodze i wysokie jak ściany. To strefa
      wyjścia
- [ ] przejść przez otwartą bramę do komórki wyjścia. Gdy gracz wejdzie w strefę (wystarcza
      krok za linię bramy), na środku okna pojawia się karta: duży turkusowy napis
      `You escaped`, pod kreską `Time: m:ss`, `Crystals: N of 13` i bursztynowy napis
      `R: play again`. Na pasku HUD czas staje, a podpowiedzi znikają. Okno debug (Gameplay)
      pokazuje `Round: won, ... s` z liczbą, która już nie rośnie
- [ ] po wygranej scena żyje dalej: niezebrane kryształy nadal się kołyszą, obracają,
      pulsują i świecą, gracz może chodzić. Reguły rundy stoją: wejście w niezebrany
      kryształ go nie zbiera, bateria nie maleje
- [ ] karta a panele: karta pojawia się nad panelami. Kliknięcie w panel wysuwa ten panel
      przed kartę. Zapisać, czy karta nie zasłania czegoś, czego potrzeba po wygranej
- [ ] noclip nie wygrywa przez zamkniętą bramę: nacisnąć R (brama zamknięta), potem N i
      wlecieć na wysokości stania, z poziomym wzrokiem, przez zamkniętą bramę w środek
      strefy wyjścia. Oczekiwane: karty nie ma, `Round: playing`. Lot nad ścianami niczego
      nie sprawdza, bo kula zasięgu gracza wisi 0,9 m nad stopami i strefy wtedy nie
      dotyka. Wyłączyć noclip (N)

Restart rundy:

- [ ] klawisz R w środku rundy. Przed naciśnięciem: zebrać kilka kryształów, wyłączyć
      latarkę, ustawić `Battery` na 0.30, odejść od startu. Po naciśnięciu: kryształy
      wracają (pasek HUD `0 / 10`, 13 kropek na planie, `Lit: 13 of 13 crystals`), bateria
      `100%`, latarka włączona (pole `Flashlight on (key F)` zaznaczone), brama zamknięta
      (`Gate: closed`, `1 gate`), gracz na starcie (`Player feet` 1, 0, 1, `Yaw` 180,
      `Pitch` 0 w oknie debug (Player / View)), czas `0:00`. Kryształy są w tych samych komórkach co
      przedtem
- [ ] czego R nie zmienia: labiryntu, trybu noclip, suwaków i pola okna debug (Gameplay),
      ustawień okna debug (Light / Lights) (poza włączeniem latarki), trybu `Lighting`, pola
      `Draw collision shapes`. Ustawić kilka z nich przed naciśnięciem i sprawdzić po nim
- [ ] R z karty wygranej: karta znika, zaczyna się nowa runda w tym samym labiryncie
- [ ] R działa także przy wolnym kursorze (tak jak N i F), a nie działa, gdy trwa
      wpisywanie wartości w polu panelu
- [ ] przycisk `Restart round (key R)` w oknie debug (Gameplay) robi to samo co klawisz
- [ ] `Regenerate` w oknie debug (World / Maze) też zaczyna nową rundę, w nowym labiryncie

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]`
- [ ] clang-format (`--dry-run --Werror`) i clang-tidy na plikach z `src/` i `tests/`: dla
      M5 wyniku nie zapisano. Uruchomić i zapisać wynik, w szczególności dla nowych plików
      `src/game/Exit.*`, `Crystals.*`, `Round.*`, `GameplayRenderer.*`, `ModelDraw.*`,
      `src/debug/Hud.*`, `src/debug/panels/GameplayPanel.*` i trzech nowych plików testów

## 15. Lista kontrolna M6, część 1: skybox

Pierwsza część kamienia milowego M6 daje labiryntowi niebo: teksturę sześcienną (cube map)
z sześciu obrazów 1024 x 1024, rysowaną jako ostatnie wywołanie rysujące sceny. Doszły:
klasa `gfx::Cubemap`, klasa `game::Skybox`, piąty program shaderów (`skybox.vert` i
`skybox.frag`), typ `assets::RowOrder` w loaderze obrazów (ściany nieba nie są odwracane),
skrypt `tools/blender/make_skybox.py` i katalog `assets/skybox/`, pole wyboru `Skybox`
i suwak `Sky brightness` w panelu Renderer oraz plik testów `tests/SkyboxTests.cpp`. Opis
kodu: [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md) (teoria, przebieg
rysujący, shadery, skrypt, testy), [`../modules/gfx/cubemap.md`](../modules/gfx/cubemap.md)
(klasa `Cubemap`), [`../modules/assets/images.md`](../modules/assets/images.md) (`RowOrder`),
[`../modules/debug-ui.md`](../modules/debug-ui.md) (panel Renderer i układ paneli),
[`blender.md`](blender.md) (skrypt nieba). Decyzje:
[`../decisions/skybox-in-game-layer.md`](../decisions/skybox-in-game-layer.md) i
[`../decisions/painted-moon-fixed-direction.md`](../decisions/painted-moon-fixed-direction.md).

Ta sekcja dotyczy **tylko skyboxa**. Pozostałe dwie części M6, teren z mapy wysokości
i trawę z shadera geometrii, opisuje sekcja 16. Punkty `[x]` niżej są stanem po pierwszej
części: 221 przypadków testowych, pięć programów, podłoga z płytek. Kamień milowy M6
**nie jest zamknięty**: część ręczna poniżej (15.2) jest otwarta w całości, na macOS kod
nie był budowany ([`build-macos.md`](build-macos.md)) i nie ma tagu.

### 15.1. Zmierzone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).

- [x] build Debug i Release: zero ostrzeżeń
- [x] `night_maze_tests.exe` w Debug i w Release: 221 przypadków testowych i 85175 asercji,
      wszystkie przechodzą. Względem M5 (215 przypadków, 85098 asercji) doszedł plik
      `SkyboxTests.cpp` (5 przypadków, 71 asercji) i jeden przypadek w
      `ImageLoaderTests.cpp` (dziś 10 przypadków, o 6 asercji więcej): `215 + 5 + 1 = 221`
      i `85098 + 71 + 6 = 85175`. Liczby na plik policzyłem z makr w kodzie testów, sumy
      są zgłoszonym wynikiem programu
- [x] orientacja nieba na zrzucie ekranu: przy kamerze ustawionej na yaw 205 i pitch 50
      tarcza księżyca jest w środku obrazu, horyzont jest poziomy, a na krawędziach
      sześcianu nie widać szwów
- [x] pliki nieba (odczytane z nagłówków PNG i z rozmiarów plików w repozytorium): sześć
      plików 1024 x 1024, 8 bitów na kanał, RGB bez alfy, razem 5 278 627 bajtów (od
      860 524 do 904 707 każdy). Piksel w kolumnie 330 i wierszu 901 od góry pliku
      `py.png`, czyli tam, gdzie reguła tekstury sześciennej umieszcza kierunek do
      księżyca, ma kolor `(200, 207, 224)`

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wyniku clang-format
i clang-tidy, tego, czy gra startuje bez linii `[error]`, czasu klatki z niebem i bez niego,
czasu wczytania sześciu plików przy starcie ani tego, czy dwa uruchomienia skryptu
`make_skybox.py` dają te same bajty. Te punkty są na liście otwartej niżej.

Żadnej kontrolki nikt nie dotknął myszą: pola `Skybox`, suwaka `Sky brightness` ani
przycisku `Reload shaders` przy pięciu programach.

### 15.2. Otwarte: test ręczny na około dziesięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i z jednego zrzutu ekranu,
nie z klikania. Nazwy widżetów są zapisane tak jak w `src/debug/panels/RendererPanel.cpp`.

Przygotowanie:

- [ ] **usunąć plik `imgui.ini`** z katalogu, z którego uruchamiam program (sekcja 7). Stary
      plik pamięta wysokość okna debug (Render / Scene) sprzed tej zmiany (230), a nowe kontrolki są
      wtedy pod jego dolną krawędzią i wygląda to tak, jakby ich nie było
- [ ] uruchomić grę z terminala. Oczekiwane w konsoli: sześć linii
      `[info] Loaded sky face: ...` z nazwami `px.png`, `nx.png`, `py.png`, `ny.png`,
      `pz.png`, `nz.png`, w tej kolejności, i żadnej linii `[error]`

Panele:

- [ ] okno debug (Render / Scene): pod listą `Lighting` jest pole wyboru `Skybox` (zaznaczone) i suwak
      `Sky brightness` (wartość 2.200, do M6 1.000). Panel nie ma paska przewijania w oknie 1280 x 720
- [ ] okno debug (Light / Lights) pod nim zaczyna się niżej niż dotąd i ma pasek przewijania (tak jest [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      zamierzone: komentarz w `PanelLayout.hpp`). Wszystkie cztery grupy da się przewinąć
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders): jedenaście linii programów, piąta to `skybox.vert + skybox.frag: OK`
      (szósta, `grass`, doszła w drugiej części M6, pięć kolejnych w M7)
- [ ] najechać kursorem na pole `Skybox`: podpowiedź zaczyna się od
      `The night sky (a cube map).` i mówi, że namalowany księżyc nie podąża za suwakami
      `Moon` okna debug (Light / Lights)

Przełącznik:

- [ ] spojrzeć w górę nad ściany: widać gwiazdy, a przy obrocie jaśniejszy pas Drogi
      Mlecznej
- [ ] odznaczyć `Skybox`: nad ścianami jest płaski, bardzo ciemny granat. Zmienić
      `Clear color` na jaskrawy: tło zmienia kolor. Przywrócić kolor i zaznaczyć `Skybox`:
      niebo wraca, a `Clear color` nie ma już żadnego widocznego skutku
- [ ] przełączyć listę `Lighting` przez wszystkie cztery tryby: niebo jest w każdym takie
      samo

Jasność:

- [ ] `Sky brightness` na 0: niebo jest czarne, gwiazd nie ma. Ściany i kryształy bez zmian
- [ ] `Sky brightness` na 6 (koniec suwaka, do M6 kończył się na 3): tło wyraźnie
      granatowe, gwiazd widać więcej
- [ ] patrząc na księżyc, przesuwać suwak od 1 w górę: tuż powyżej 1 najjaśniejsze miejsca
      tarczy zaczynają się przepalać, a około 1,5 znikają także szare plamy i tarcza jest
      płaską białą plamą (framebuffer obcina wartości do 1, bufora HDR nie ma do M7).
      Zapisać, przy jakich wartościach to widać
- [ ] wrócić do 1 (kliknięcie suwaka z wciśniętym Ctrl i wpisanie `1`)

Szwy i horyzont. Najwygodniej w trybie noclip (klawisz N) nad ścianami, gdzie widać cały
horyzont:

- [ ] obrócić się powoli o pełne koło, patrząc poziomo: jaśniejszy pas przy horyzoncie jest
      poziomy i ciągły, bez pionowych linii co 90 stopni (krawędzie między ścianami
      bocznymi są przy yaw 45, 135, 225 i 315)
- [ ] spojrzeć w cztery górne narożniki sześcianu: yaw 45, 135, 225, 315 przy pitch około
      35. W narożniku spotykają się trzy ściany. Nie widać linii, załamania jasności ani
      gwiazdy przeciętej na pół
- [ ] spojrzeć na krawędzie ściany górnej: pitch 45 przy yaw 0, 90, 180 i 270. Droga
      Mleczna i poświata księżyca przechodzą przez krawędź bez przerwy
- [ ] spojrzeć prosto w górę (pitch 89) i obrócić się: niebo obraca się wokół środka
      ekranu, bez skoków
- [ ] iść albo lecieć w jedną stronę, patrząc w niebo: gwiazdy nie przesuwają się wcale.
      Ściany pod nimi przesuwają się normalnie

Księżyc a światło księżyca:

- [ ] w oknie debug (Player / View) ustawić `Yaw` na 205 i `Pitch` na 50: tarcza księżyca jest w środku
      ekranu, z jasną poświatą wokół
- [ ] wyłączyć latarkę (klawisz F) i obejrzeć ściany: jaśniejsze są te, które są zwrócone
      w stronę księżyca (lica zwrócone w stronę +Z i, słabiej, -X), ciemniejsze te
      odwrócone od niego
- [ ] w oknie debug (Light / Lights) rozwinąć grupę `Moon (directional)` i przesunąć `Moon yaw`: światło [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      na ścianach wędruje, **tarcza na niebie zostaje w miejscu**. To znane ograniczenie,
      nie błąd. Przywrócić 25

Widoki diagnostyczne:

- [ ] okno debug (Render / Textures and normals), `View mode`: `Normals as colour`. Niebo jest gładkim gradientem bez
      gwiazd: różowoczerwone w stronę +X (yaw 90), jasnozielone prosto w górę,
      niebieskofioletowe w stronę +Z (yaw 180), oliwkowe w stronę -Z (yaw 0)
- [ ] `View mode`: `UVs as colour`. Ściany zmieniają kolory, niebo wygląda tak samo jak
      w poprzednim widoku
- [ ] w którymś z tych widoków odznaczyć `Skybox`: tło jest kolorem `Clear color`. Zaznaczyć
      z powrotem i wrócić do `Textured`

Kolejność rysowania:

- [ ] okno debug (Diagnostics / Collision and picking), `Draw collision shapes`: linie pudełek widziane na tle nieba są
      w całości widoczne, niebo ich nie zamalowuje
- [ ] kryształ widziany na tle nieba (z góry, w trybie noclip) ma ostre krawędzie, bez
      obwódki w kolorze `Clear color`
- [ ] zmienić rozmiar okna myszą: niebo wypełnia całe tło przy każdych proporcjach

Przeładowanie shaderów:

- [ ] w `assets/shaders/skybox.frag` zamienić `sky * uBrightness` na `sky.bgr * uBrightness`,
      skopiować assety (`cmake --build --preset debug --target copy_assets`) i kliknąć
      `Reload shaders`: niebo zmienia odcień z granatowego na brunatny, wszystkie linie
      (dziś jedenaście) nadal kończą się `OK`
- [ ] wpisać w tym samym pliku błąd składni, skopiować assety, `Reload shaders`: linia
      programu `skybox` jest czerwona (`FAILED, the previous program stays in use`) z nazwą
      pliku i numerem linii, **niebo nadal się rysuje** poprzednią wersją, pozostałe
      dziesięć linii kończy się `OK`
- [ ] cofnąć obie zmiany (`git checkout assets`), skopiować assety, `Reload shaders`:
      jedenaście razy `OK`, niebo jak na początku

Brak pliku:

- [ ] w kopii katalogu `assets` obok programu (sekcja 7) zmienić nazwę `skybox/px.png` i
      uruchomić grę. Oczekiwane: jedna linia `[error] Image file cannot be opened: ...` z
      nazwą pliku, gra działa, tłem jest `Clear color`, pole `Skybox` jest zaznaczone i nic
      nie zmienia. Przywrócić plik (`cmake --build --preset debug --target copy_assets`)

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza tą jedną
      wywołaną celowo
- [ ] clang-format (`--dry-run --Werror`) i clang-tidy na plikach z `src/` i `tests/`:
      wyniku dla tej części nie zapisano. Uruchomić i zapisać, w szczególności dla
      `src/gfx/Cubemap.*`, `src/game/Skybox.*` i `tests/SkyboxTests.cpp`
- [ ] (wymaga Blendera) uruchomić dwa razy
      `tools/blender/make_skybox.py` ([`blender.md`](blender.md), sekcja 7.7) i po każdym
      razie `git status`: oczekiwane brak zmian w `assets/skybox/`. Jeśli pliki się
      różnią, uruchomić testy i zapisać wynik

## 16. Lista kontrolna M6, część 2: teren i trawa

Druga część kamienia milowego M6 zamienia płaską podłogę z płytek na teren i sadzi na nim
trawę. Podłoże jest jedną siatką zbudowaną z mapy wysokości (`assets/textures/heightmap.png`):
łagodnie nierówne pod labiryntem i przechodzące we wzgórza poza nim. Ściany, słupki
i brama są opuszczone do najniższego punktu podłoża pod swoim obrysem, więc nie ma pod nimi
szczelin, kryształy i strefa wyjścia stoją na podłożu, a stopy gracza biorą wysokość
z terenu. Wzdłuż ścian i rzadko na wzgórzach rosną kępki trawy: w buforze jest jeden punkt
na kępkę, a źdźbła buduje z niego shader geometrii. Doszły: struktury i funkcje
`game::Terrain`, `game::Heightmap` i `game::placeOnTerrain`, klasa `game::TerrainRenderer`,
funkcja `game::placeGrass` i klasa `game::GrassRenderer`, opcjonalny etap geometrii
w `gfx::Shader`, szósty program shaderów (`grass.vert`, `grass.geom`, `grass.frag`),
prymityw `GL_POINTS` w `gfx::Mesh`, skrypt `tools/blender/make_heightmap.py`, tekstury
`ground.png` i `ground_normal.png`, panele Terrain i Grass oraz pliki testów
`tests/TerrainTests.cpp` i `tests/GrassTests.cpp`. Zniknęły: model `floor_tile.obj` z plikiem
`.mtl`, skrypt `build_floor_tile.py` i tekstury `floor_stone.png` i `floor_stone_normal.png`.
Opis kodu: [`../modules/renderer/terrain.md`](../modules/renderer/terrain.md) (temat 13:
mapa wysokości, wzór na wysokość, siatka, `heightAt`, stanie na terenie, wireframe, panel
Terrain), [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md)
(temat 9: shader geometrii, trzy shadery trawy, miejsca kępek, panel Grass),
[`../modules/gfx/shader-class.md`](../modules/gfx/shader-class.md) (etap geometrii w klasie
`Shader`), [`../modules/debug-ui.md`](../modules/debug-ui.md) (dwa nowe panele, drugi rząd
pasków tytułu, pasek HUD), [`blender.md`](blender.md) (skrypt mapy wysokości i tekstury
podłoża). Decyzje:
[`../decisions/gentle-terrain-under-maze.md`](../decisions/gentle-terrain-under-maze.md),
[`../decisions/walls-sunk-to-lowest-corner.md`](../decisions/walls-sunk-to-lowest-corner.md),
[`../decisions/heightmap-tiled-in-world-metres.md`](../decisions/heightmap-tiled-in-world-metres.md),
[`../decisions/floor-tiles-retired.md`](../decisions/floor-tiles-retired.md),
[`../decisions/height-scale-rebuilds-terrain.md`](../decisions/height-scale-rebuilds-terrain.md)
i [`../decisions/grass-lit-with-up-normal.md`](../decisions/grass-lit-with-up-normal.md).

Z tą częścią kod M6 jest kompletny na Windowsie (skybox, teren, trawa). Kamień milowy M6
**nie jest zamknięty**: część ręczna poniżej (16.2) i część ręczna skyboxa (15.2) są
otwarte w całości, na macOS kod nie był budowany ([`build-macos.md`](build-macos.md))
i nie ma tagu. **Nie są zbudowane:** iskry wokół kryształów, które PRD wymienia w temacie
9 obok trawy. Bloom, mgła, cienie księżyca i cienie latarki, których po tej części też nie
było, doszły w drugiej, trzeciej, czwartej i piątej części M7 (sekcje 18, 19, 20 i 21).

### 16.1. Zmierzone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).
Punkty są w trzech grupach, bo nie wszystkie mają tę samą wagę.

Sprawdzone dziś na gotowych plikach, bez klikania w grze:

- [x] `night_maze_tests.exe` w Debug i w Release: 256 przypadków testowych i 101232
      asercje, wszystkie przechodzą. Uruchomione na istniejących programach testowych
      (żaden plik w `src/`, `tests/`, `assets/` ani `tools/` nie jest od nich nowszy).
      Względem pierwszej części M6 (221 przypadków) doszły `TerrainTests.cpp` (27
      przypadków) i `GrassTests.cpp` (9), a z `ObjLoaderTests.cpp` zniknął przypadek
      płytki podłogi (dziś 19): `221 + 27 + 9 - 1 = 256`. Liczby na plik policzyłem z makr
      w kodzie testów
- [x] siatka terenu labiryntu startowego (10 x 10 komórek): 97 x 97 punktów co 0,5 m,
      18432 trójkąty. Z kodu: `(10 + 2 * 7) * 4 + 1 = 97` punktów w rzędzie (7 komórek
      marginesu z każdej strony, 4 kroki siatki na komórkę) i `96 * 96 * 2 = 18432`
- [x] wysokości przeliczone niezależnie, skryptem w Pythonie, z pikseli pliku
      `heightmap.png` (256 x 256, 8 bitów na kanał) i ze wzorów z `src/game/Terrain.cpp`,
      przy skali wysokości 1: podłoże w obrębie labiryntu ma od 0,085 m do 0,461 m,
      najniższy punkt całej siatki ma 0,0 m, a najwyższe wzgórze 3,37 m (punkt siatki
      `x = 33`, `z = -5,5`, czyli 13 m na wschód od labiryntu)
- [x] wysokość stóp gracza z tego samego przeliczenia: 0,124 m na starcie (środek komórki
      startowej, `x = 1`, `z = 1`), 0,278 m w punkcie `(9, 1)` i 0,352 m w punkcie
      `(17, 1)`. Zgadza się z liczbami zgłoszonymi z gry (gracz przestawiany tam
      teleportem). Wszystkie trzy punkty są punktami siatki, więc nie sprawdzają
      interpolacji wewnątrz trójkąta: tę sprawdzają tylko testy jednostkowe
- [x] liczba kępek trawy przy gęstości startowej 2,5: labirynt startowy ma 121 ścian,
      każda dostaje `round(2,5 * 2 m) = 5` kępek na stronę, czyli 10, razem 1210. Na
      wzgórzach jest 806 prób (`round(48 m * 48 m * 0,35)`), z których 633 leżą dalej niż
      0,6 m od labiryntu i zostają. Razem **1843 kępki**, czyli 5529 źdźbeł (3 na kępkę).
      Liczbę 633 odtworzyłem własną implementacją generatora `mt19937` w Pythonie,
      a sumę 1843 zgłoszono z panelu Grass

Zgłoszone z dnia, w którym powstał kod, i tu nie powtarzane:

- [x] czysty build Debug i Release: zero ostrzeżeń
- [x] clang-format bez uwag
- [x] liczba klatek na sekundę w Release przy ustawieniach startowych: około 2000 przed
      tą częścią i po niej. Rozrzut między uruchomieniami (od 1438 do 2040) jest większy
      niż jakakolwiek różnica, więc z tego pomiaru nie da się odczytać kosztu terenu ani
      trawy. Synchronizacja pionowa (`swap interval 1`) nie ogranicza na tym komputerze
      liczby klatek
- [x] zepsuty plik `grass.geom`: komunikat sterownika zaczyna się od `grass.geom(84)`,
      czyli nazwy pliku shadera geometrii i numeru linii, a gra działa dalej
- [x] plik `heightmap.png` nie jest na liście panelu Assets: wczytuje go
      `NightMazeApp` funkcją `assets::loadImage`, poza `assets::AssetCache`
- [x] obraz sprawdzony na zrzutach ekranu zrobionych tymczasowymi wstawkami w kodzie,
      które są już usunięte. Listy zrzutów nie zapisano

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wyniku clang-tidy,
tego, czy gra startuje bez linii `[error]`, czasu budowy terenu przy ruchu suwaka
`Height scale` ani tego, czy dwa uruchomienia skryptu `make_heightmap.py` dają te same
bajty.

Żadnej kontrolki nikt nie dotknął myszą i nikt nie przeszedł po terenie ręką: suwaki i pola
paneli Terrain i Grass, przycisk `Reload shaders` przy sześciu programach i chodzenie
klawiszami są otwarte. Płynność zmiany wysokości w marszu (bez schodków) sprawdzają tylko
testy jednostkowe. Na macOS nic z tej części nie było budowane ani uruchamiane.

### 16.2. Otwarte: test ręczny na około piętnaście minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze zgłoszonych zrzutów
ekranu, nie z klikania. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/TerrainPanel.cpp` i `src/debug/panels/GrassPanel.cpp`.

Przygotowanie:

- [ ] **usunąć plik `imgui.ini`** z katalogu, z którego uruchamiam program (sekcja 7). Stary
      plik nie ma wpisów paneli Terrain i Grass, więc te dwa staną w miejscach z kodu, ale
      pozostałe zostaną tam, gdzie zapisał je plik, i układ może się nie zgadzać
- [ ] uruchomić grę z terminala. Oczekiwane w konsoli: jedna linia
      `[info] Loaded heightmap: ...` z plikiem `heightmap.png`, linie
      `[info] Loaded texture: ...` dla `ground.png` i `ground_normal.png` i żadnej linii
      `[error]`

Panele i pasek HUD:

- [ ] u góry okna, między kolumnami, są w dwóch pierwszych rzędach pasków tytułu Camera [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      i Gameplay, a tuż pod nimi Terrain (pod Camera, tej samej szerokości) i Grass (pod
      Gameplay). Wszystkie cztery są zwinięte. Paski nie nachodzą na siebie. Pod nimi są
      dziś jeszcze dwa rzędy: Framebuffers (sekcja 17.2) i Shadows (sekcja 20.2)
- [ ] [stan z czwartej części M7; dziś rzędów jest pięć] pasek HUD stoi dziś pod czwartym rzędem pasków, z wyraźnym odstępem, i nie nachodzi [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      na żaden z nich (po tej części stał pod drugim, a zgłoszone przesunięcie względem
      wersji przed nią to około 30 pikseli w dół)
- [ ] rozwinąć okno debug (World / Terrain and grass / Terrain) (strzałka w pasku). Zawartość od góry: suwak `Height scale` [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      (`1.00`), pole wyboru `Wireframe` (odznaczone), kreska, linie
      `Grid: 97 x 97 points, 0.50 m apart`, `Triangles: 18432` i
      `Height: 0.00 m to 3.37 m`. Zapisać, czy zawartość mieści się bez przewijania
- [ ] rozwinąć okno debug (World / Terrain and grass / Grass). Zawartość od góry: pole wyboru `Enabled` (zaznaczone), suwaki [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Density` (`2.5 per m`), `Blade height` (`0.30 m`) i `Wind strength` (`1.00`),
      kreska i linia `Tufts: 1843 (5529 blades)`. Zapisać, czy zawartość mieści się bez
      przewijania
- [ ] rozwinięty okno debug (Player) zasłania pasek Terrain, a rozwinięty Gameplay pasek Grass [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      (tak jest zamierzone: komentarz w `PanelLayout.hpp`). Rozwinięte panele Terrain
      i Grass zasłaniają tylko pas sceny pod sobą
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders): jedenaście linii programów, szósta to
      `grass.vert + grass.geom + grass.frag: OK` (po tej części była ostatnia). Podpowiedź
      po najechaniu kursorem pokazuje trzy pełne ścieżki, po jednej w linii
- [ ] okno debug (Diagnostics / Assets): na liście `Textures` są `ground.png` i `ground_normal.png`, nie ma
      `heightmap.png`, a na liście `Models` nie ma płytki podłogi (pięć modeli)
- [ ] okno debug (Player / Position): `Player feet` ma na starcie y równe 0.124, a `Eye` y równe 1.82

Chodzenie po podłożu (kliknąć w scenę, kursor znika):

- [ ] przejść kilka korytarzy klawiszami W, A, S, D: podłoże łagodnie się wznosi i opada,
      a obraz płynie, bez schodków i bez drgania w pionie. Lewy Shift przyspiesza i nic
      się nie zmienia poza prędkością
- [ ] obserwować w oknie debug (Player / Position) pole `Player feet` w marszu: y zmienia się płynnie
      w granicach od około 0.08 do 0.46 w obrębie labiryntu, a `Eye` jest zawsze o 1.70
      wyżej
- [ ] iść pod górę i z góry tym samym korytarzem: prędkość po ziemi jest ta sama w obie
      strony (klawisze przesuwają gracza tylko poziomo)
- [ ] dojść do ściany i iść wzdłuż niej skosem: gracz zatrzymuje się i ślizga tak samo jak
      na płaskiej podłodze, nie wchodzi w ścianę w żadnym miejscu, także tam, gdzie podłoże
      jest najwyżej
- [ ] stanąć w miejscu i nie dotykać klawiszy: obraz stoi, wysokość się nie zmienia

Szczeliny. Obejść powoli kilka ścian i słupków, patrząc na ich podstawy, także od strony,
z której podłoże opada:

- [ ] pod żadną ścianą, słupkiem ani pod bramą nie widać szczeliny, przez którą
      prześwituje niebo albo podłoże po drugiej stronie. Podstawy są miejscami zagłębione
      w ziemi (tak ma być: obiekt stoi na najniższym punkcie podłoża pod swoim obrysem)
- [ ] słupek i ściany, które się z nim stykają, mogą mieć podstawy na trochę różnych
      wysokościach (każdy obiekt jest opuszczany osobno). Zapisać, czy widać to z poziomu
      oczu i czy przeszkadza
- [ ] górne krawędzie sąsiednich ścian nie są na jednej wysokości: różnią się o kilka do
      kilkunastu centymetrów. Zapisać, jak to wygląda z góry w trybie noclip

Skala wysokości (okno debug (World / Terrain and grass / Terrain), suwak `Height scale`, zakres od 0.00 do 2.50):

- [ ] przesuwać suwak powoli: podłoże zmienia się na bieżąco, a ściany, słupki, brama,
      kryształy, trawa i gracz podążają za nim w tej samej klatce. Nic nie zostaje w
      powietrzu ani pod ziemią. Runda trwa dalej: licznik kryształów, czas i bateria się
      nie zerują
- [ ] `Height scale` na 0: świat jest płaski, linia w panelu to `Height: 0.00 m to 0.00 m`,
      `Player feet` ma y równe 0, wzgórz nie ma. Trawa stoi na płaskim
- [ ] `Height scale` na 2.50 (prawy koniec suwaka): linia `Height: 0.00 m to 8.43 m`,
      podłoże w labiryncie jest wyraźnie pofałdowane (na starcie stopy na około 0.31),
      wzgórza są wysokie. Przejść kilka korytarzy: gracz nadal zatrzymuje się na każdej
      ścianie i nigdzie nie da się przejść pod ścianą ani przez nią. Pod żadną ścianą nie
      ma szczeliny
- [ ] kliknąć suwak z wciśniętym Ctrl i wpisać `5`: wartość wraca do 2.50 (suwak ma flagę
      `ImGuiSliderFlags_AlwaysClamp`). Wpisać `-1`: wartość to 0.00
- [ ] wrócić do 1.00. Liczba trójkątów i punktów siatki nie zmieniała się przez cały czas
      (skala zmienia wysokości, nie siatkę)
- [ ] zapisać, czy przy szybkim przeciąganiu suwaka gra zwalnia (każda zmiana buduje teren,
      macierze, pudełka kolizji i trawę od nowa)

Wireframe:

- [ ] zaznaczyć `Wireframe`: podłoże zmienia się w siatkę linii w kolorach podłoża, przez
      którą widać to, co jest pod terenem: dolną część nieba. Widać kwadraty o boku 0,5 m przecięte
      przekątną, zawsze w tę samą stronę: z północnego zachodu na południowy wschód
- [ ] **tylko teren** jest liniami: ściany, słupki, brama, kryształy, trawa, niebo, panele
      i pasek HUD są wypełnione jak zwykle
- [ ] przełączyć listę `Lighting` w oknie debug (Render / Scene) przez cztery tryby przy włączonym
      `Wireframe`: siatka zostaje w każdym trybie, zmienia się tylko jej jasność
- [ ] w trybie noclip wznieść się nad labirynt: widać, że siatka jest równa w całym
      terenie (97 na 97 punktów), a w obrębie labiryntu na jedną komórkę przypadają
      cztery kwadraty wzdłuż każdego boku
- [ ] odznaczyć `Wireframe`: podłoże jest znów wypełnione

Wzgórza i noclip:

- [ ] klawisz N, wznieść się spacją nad ściany i rozejrzeć się: wokół labiryntu
      podłoże rośnie we wzgórza, płynnie, bez załamania na granicy
      labiryntu. Teren kończy się 14 m za zewnętrznymi ścianami, równą krawędzią, za
      którą jest niebo
- [ ] najwyższe wzgórze jest na wschód od labiryntu (około 13 m za wschodnią ścianą)
- [ ] na wzgórzach rosną rzadkie kępki trawy, a w pasie tuż przy zewnętrznych ścianach
      trawa jest tylko ta przyścienna
- [ ] wylecieć nad wzgórze, zatrzymać się kilka metrów nad nim i wyłączyć noclip (N):
      stopy spadają na podłoże od razu, w jednej klatce, bez płynnego zjazdu, i gracz
      stoi na wzgórzu. Da się po nim chodzić (poza labiryntem nie ma ścian), a po
      wyjściu poza krawędź terenu wysokość zostaje taka jak na krawędzi
- [ ] klawisz R: gracz wraca na start, na podłoże

Trawa (okno debug (World / Terrain and grass / Grass)):

- [ ] przy ścianach, po obu stronach każdej, rosną kępki po trzy źdźbła, ciemniejsze u
      nasady i jaśniejsze na czubku. Stoją w pasie przy ścianie, nie w linii od linijki,
      i nie wchodzą w ściany ani w słupki. W poprzek otwartej strony komórki wyjścia
      (przy bramie) trawy nie ma
- [ ] źdźbła kołyszą się, a kołysanie przechodzi po trawie falą. Nasady stoją w miejscu,
      ruszają się czubki
- [ ] odznaczyć `Enabled`: trawa znika cała, linia `Tufts: 1843 (5529 blades)` zostaje.
      Zaznaczyć z powrotem
- [ ] `Density` na 0.0: trawy nie ma, linia to `Tufts: 0 (0 blades)`
- [ ] `Density` na 8.0 (prawy koniec): trawa jest gęsta, przy ścianach 16 kępek na stronę
      (3872 przy samych ścianach, do tego wzgórza). Zapisać liczbę z linii `Tufts` i to,
      czy liczba klatek w oknie debug (Render / Scene) wyraźnie spada
- [ ] wrócić do 2.5: linia znów pokazuje `Tufts: 1843 (5529 blades)`, a kępki stoją w tych
      samych miejscach co na początku (miejsca zależą tylko od ziarna labiryntu i od
      gęstości)
- [ ] `Blade height` od 0.05 do 0.80: źdźbła rosną i maleją na bieżąco, liczba kępek się
      nie zmienia. Wrócić do 0.30
- [ ] `Wind strength` na 0.00: trawa stoi nieruchomo. Na 3.00: czubki wychylają się
      wyraźnie, nasady nadal stoją. Wrócić do 1.00
- [ ] przejść przez kępkę: trawa nie zatrzymuje gracza i nie reaguje na niego

Trawa a światło:

- [ ] skierować latarkę na trawę przy ścianie: kępki w stożku są jasne, poza nim ciemne,
      tak jak podłoże pod nimi. Wyłączyć latarkę (F): trawa ciemnieje razem z podłożem
- [ ] podejść do kryształu: trawa w zasięgu jego światła ma turkusowy odcień i pulsuje
      razem z plamą światła na podłożu
- [ ] obejść kępkę dookoła z włączoną latarką: jej jasność nie skacze przy zmianie strony
      (trawa jest cieniowana normalną podłoża, nie normalną źdźbła) i źdźbła widać z obu
      stron
- [ ] lista `Lighting` w oknie debug (Render / Scene), cztery tryby po kolei. `Blinn-Phong` i `Phong`:
      trawa oświetlona jak wyżej, bez połysku. `Gouraud`: ściany i podłoże są cieniowane
      na wierzchołek, a trawa **nadal na fragment**, więc krawędź stożka latarki na trawie
      jest gładka. `Unlit`: trawa ma pełną jasność, jak reszta sceny
- [ ] okno debug (Render / Textures and normals), `View mode`: `Normals as colour`. Trawa jest jednolicie jasnozielona
      (normalna prosto w górę), podłoże w odcieniach jasnej zieleni. `UVs as colour`:
      źdźbła są ciemne u nasady i zielenieją ku czubkowi, z czerwienią rosnącą w poprzek
      źdźbła, a podłoże ma czerwono-zielone przejścia powtarzane co 4 m. Wrócić do
      `Textured`

Przeładowanie shaderów:

- [ ] w `assets/shaders/grass.geom` zmienić `const float LEAN = 0.4;` na `1.2`, skopiować
      assety (`cmake --build --preset debug --target copy_assets`) i kliknąć
      `Reload shaders`: źdźbła kładą się mocno na boki, wszystkie linie (dziś jedenaście)
      nadal kończą się `OK`
- [ ] wpisać w tym samym pliku błąd składni (na przykład usunąć średnik po
      `EmitVertex()`), skopiować assety, `Reload shaders`: linia programu `grass` jest
      czerwona (`FAILED, the previous program stays in use`), komunikat zaczyna się od
      nazwy `grass.geom` z numerem linii w nawiasie, **trawa nadal się rysuje**
      poprzednią wersją i nadal się kołysze, pozostałe dziesięć linii kończy się `OK`
- [ ] zepsuć w ten sam sposób `grass.frag`: komunikat nazywa `grass.frag`. Zepsuć
      `common/lighting.glsl`: czerwone są trzy linie naraz (`lit`, `gouraud` i `grass`),
      a komunikat nazywa plik dołączany
- [ ] cofnąć zmiany w plikach shaderów, skopiować assety, `Reload shaders`: jedenaście
      razy `OK`, trawa jak na początku i nadal oświetlona (połączenie z blokiem świateł jest
      odtwarzane po przeładowaniu)

Nowy labirynt (okno debug (World / Maze)):

- [ ] `Seed` 2, `Regenerate`: nowy układ ścian, podłoże pod labiryntem jest takie samo
      jak przedtem (mapa wysokości nie zależy od ziarna), trawa rośnie przy nowych
      ścianach, a liczba kępek w oknie debug (World / Terrain and grass / Grass) jest bliska poprzedniej (ścian jest znów
      121, zmienia się tylko liczba kępek na wzgórzach)
- [ ] `Seed` 1, `Regenerate`: linia znów pokazuje `Tufts: 1843 (5529 blades)`. Zapamiętać
      miejsce jednej kępki przy starcie, zmienić ziarno, wrócić do 1: kępka stoi tam, gdzie
      stała
- [ ] `Width` 20, `Height` 15, `Regenerate`: okno debug (World / Terrain and grass / Terrain) pokazuje
      `Grid: 137 x 117 points, 0.50 m apart` i `Triangles: 31552`, wzgórza zaczynają się
      za nowymi ścianami zewnętrznymi i mają ten sam rozmiar co przedtem (mapa wysokości
      powtarza się co 48 m, nie rozciąga). Pod żadną ścianą nie ma szczeliny
- [ ] `Width` 40, `Height` 40, `Regenerate`: `Grid: 217 x 217 points, 0.50 m apart`,
      `Triangles: 93312`. Zapisać liczbę kępek, liczbę klatek w Debug i w Release i to,
      czy w tak dużym labiryncie widać powtarzanie się pagórków
- [ ] `Width` 2, `Height` 2, `Regenerate`: `Grid: 65 x 65 points, 0.50 m apart`, gra
      działa, wzgórza stoją blisko
- [ ] ustawić `Height scale` na 2.50, potem `Regenerate`: nowy labirynt powstaje od razu
      na wysokim terenie. Wrócić do 10 na 10, ziarna 1 i skali 1.00

Kryształy, brama i wyjście na podłożu:

- [ ] obejrzeć kilka kryształów w różnych miejscach labiryntu: każdy unosi się na tej
      samej wysokości nad podłożem swojej komórki (podstawa około 0,9 m nad ziemią), żaden
      nie tkwi w ziemi ani nie wisi wyraźnie wyżej niż inne
- [ ] zebrać kryształ, wchodząc w niego: zbieranie działa tak samo jak na płaskiej
      podłodze, także w najwyżej i w najniżej położonej komórce
- [ ] okno debug (Diagnostics / Collision and picking), `Draw collision shapes`: żółte pudełka ścian i słupków zaczynają
      się na wysokości podstaw modeli (każde na innej), zielone pudełko gracza stoi na
      podłożu, turkusowe kule kryształów otaczają kryształy, a magentowe pudełko strefy
      wyjścia stoi na podłożu komórki wyjścia. Żadne pudełko ściany nie kończy się nad
      stopami gracza, który przy niej stoi
- [ ] brama: stoi na podłożu bez szczeliny pod spodem. Otworzyć ją (zebrać kryształy albo
      w oknie debug (Gameplay) przesunąć `Crystals needed` na 0.05 po zebraniu jednego):
      zjeżdża w ziemię w 1,5 sekundy i **znika pod nią w całości**, nic z niej nie wystaje,
      także po stronie, z której podłoże opada
- [ ] przejść przez otwartą bramę do komórki wyjścia: karta `You escaped` pojawia się tak
      jak przedtem
- [ ] klawisz R w trakcie rundy i po wygranej: kryształy wracają na swoje miejsca nad
      podłożem, gracz staje na starcie na podłożu, trawa i teren się nie zmieniają

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza wywołanymi
      celowo
- [ ] liczba klatek na sekundę w Release przy ustawieniach startowych, z okna debug (Render / Scene):
      zapisać wartość przy włączonej i wyłączonej trawie (`Enabled`) oraz z `Wireframe`.
      Zgłoszone około 2000 nie rozróżnia tych stanów. Jeśli liczba stoi na częstotliwości
      odświeżania monitora, zapisać to: na tym komputerze synchronizacja pionowa nie
      ograniczała liczby klatek
- [ ] clang-tidy na plikach z `src/` i `tests/`: wyniku dla tej części nie zapisano.
      Uruchomić i zapisać, w szczególności dla `src/game/Terrain.*`, `src/game/Grass.*`,
      `src/game/TerrainRenderer.*`, `src/game/GrassRenderer.*` i obu nowych plików testów
- [ ] (wymaga Blendera) uruchomić dwa razy `tools/blender/make_heightmap.py`
      ([`blender.md`](blender.md)) i po każdym razie `git status`: oczekiwane brak zmian
      w `assets/textures/heightmap.png`. Jeśli plik się różni, uruchomić testy i zapisać
      wynik (jeden z nich sprawdza, że podłoże pod labiryntem startowym ma rozpiętość od
      0,3 m do 0,5 m)
- [ ] (wymaga Blendera) zmienić w tym skrypcie `HEIGHTMAP_SEED` na inną liczbę,
      wygenerować plik, skopiować assety i uruchomić grę: inne pagórki, nadal bez
      szczelin pod ścianami. Uruchomić testy i zapisać, czy test rozpiętości podłoża
      przechodzi dla nowego obrazu. Przywrócić ziarno 53 i wygenerować plik jeszcze raz:
      `git status` nie powinien pokazywać zmiany w obrazie

## 17. Lista kontrolna M7, część 1: bufor HDR i gamma

Pierwsza część kamienia milowego M7 zmienia koniec klatki. Scena nie jest już rysowana
prosto do okna, tylko do własnego framebuffera z teksturą zmiennoprzecinkową (`GL_RGBA16F`)
i teksturą głębi, a do okna przenosi ją osobny przebieg: ekspozycja, mapowanie tonów
i kodowanie sRGB. Razem z tym weszła korekcja gamma w całym potoku: tekstury koloru i niebo
są teksturami sRGB, mapy normalnych zostają liniowe, a kolory wpisane liczbami są
przeliczane raz. Doszły: klasa `gfx::Framebuffer`, typ `gfx::ColorSpace` z funkcjami
`srgbToLinear` i `linearToSrgb`, funkcja `gfx::hasExtension`, klasa `game::PostProcess`,
siódmy i ósmy program shaderów (`post/composite.vert` z `post/composite.frag` i ten sam
shader wierzchołków z `post/preview.frag`), pliki `common/color.glsl` i `common/depth.glsl`,
klasa `debug::RawTextureSampler`, jedenasty panel, Framebuffers, oraz pliki testów
`tests/ColorSpaceTests.cpp` i `tests/FramebufferTests.cpp`. Zmieniły się: konstruktory
`gfx::Texture2D` i `gfx::Cubemap` i funkcja `assets::AssetCache::texture` (nowy,
obowiązkowy argument `gfx::ColorSpace`), `NightMazeApp::onRender` oraz wartości startowe
świateł, świecenia kryształów, jasności nieba i koloru tła.
Opis kodu: [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md) (gamma: sRGB,
trzy etapy potoku, które tekstury są sRGB, kolory wpisane liczbami, porównanie ze starym
potokiem), [`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) (obiekt
framebuffera, załączniki, kompletność, zmiana rozmiaru),
[`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) (temat 10:
trójkąt na cały ekran, ekspozycja, krzywe mapowania tonów, kolejność klatki, podglądy
załączników, panel Framebuffers), [`../modules/debug-ui.md`](../modules/debug-ui.md)
(jedenasty panel, trzeci rząd pasków tytułu, `RawTextureSampler`). Decyzje:
[`../decisions/gamma-linear-pipeline.md`](../decisions/gamma-linear-pipeline.md),
[`../decisions/srgb-encode-in-shader.md`](../decisions/srgb-encode-in-shader.md),
[`../decisions/aces-default-tone-mapping.md`](../decisions/aces-default-tone-mapping.md),
[`../decisions/depth-attachment-as-texture.md`](../decisions/depth-attachment-as-texture.md)
i [`../decisions/post-process-in-game-layer.md`](../decisions/post-process-in-game-layer.md).
Notatka [`../decisions/no-gamma-until-m7.md`](../decisions/no-gamma-until-m7.md) jest
zastąpiona.

Kamień milowy M7 jest **rozpoczęty i nie jest kompletny**. Temat 10 wykładu jest w toku:
po tej części były bufor HDR, przebieg składający i podglądy załączników, a bloom doszedł
w części drugiej (sekcja 18), a mgła i winieta w trzeciej (sekcja 19). Cienie księżyca
(temat 11) doszły w części czwartej (sekcja 20). **Nie są zbudowane:** cienie latarki
i minimapa. Część ręczna poniżej (17.2) jest otwarta
w całości, na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)) i nie ma
tagu.

### 17.1. Zgłoszone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).
**Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej sekcji**: wszystkie są
zgłoszone z dnia, w którym powstał kod. Liczby przypadków na plik policzyłem z makr
`TEST_CASE` w kodzie testów.

Bramka i testy:

- [x] `make check` przechodzi: formatowanie (clang-format), testy w Debug i w Release,
      clang-tidy
- [x] build Debug i Release: zero ostrzeżeń
- [x] `night_maze_tests.exe` w Debug i w Release: 269 przypadków testowych i 102103
      asercje, wszystkie przechodzą. Względem drugiej części M6 (256 przypadków) doszły
      `ColorSpaceTests.cpp` (9 przypadków), `FramebufferTests.cpp` (3) i jeden przypadek
      w `LightingTests.cpp` (dziś 11): `256 + 9 + 3 + 1 = 269`

Porównanie obrazu z poprzednim commitem (zrobione przed ponownym dobraniem świateł
i jasności nieba, więc dotyczy samej zmiany potoku):

- [x] tryb `Unlit`, mapowanie tonów `None (clamp)`, ekspozycja 1: obraz **nie jest**
      identyczny co do piksela. Ściany i podłoże różnią się najwyżej o 22 poziomy z 255,
      średnio o 1,1, i tylko na spoinach cegieł. Powód: filtr tekstury miesza teraz
      wartości liniowe, a nie bajty z pliku
      ([`../modules/gfx/color-space.md`](../modules/gfx/color-space.md), sekcje 2.10 i 5.9)
- [x] widoki `Normals as colour` i `UVs as colour`: różnica najwyżej 1 poziom
- [x] podglądy tekstur w panelu Assets: identyczne co do piksela (karta ma rozszerzenie
      `GL_EXT_texture_sRGB_decode`, więc `RawTextureSampler` działa)
- [x] dziś niebo i kryształy różnią się od poprzedniego commita **z założenia**: jasność
      nieba ma wartość startową 2,2 zamiast 1,0, a `CRYSTAL_GLOW_STRENGTH` 2,5 zamiast 1,0

Zachowanie programu w buildzie Debug (z `GL_CHECK`):

- [x] brak błędów OpenGL przy otwartym panelu Framebuffers, czyli z rysowanymi podglądami
- [x] brak błędów po zmianie rozmiaru okna na 1400 x 800: bufor sceny jest tworzony od nowa
      w nowym rozmiarze
- [x] brak błędów po minimalizacji okna (framebuffer 0 x 0) i po przywróceniu: klatka
      z rozmiarem 0 jest pomijana w całości

Koszt, Release, synchronizacja pionowa wyłączona, panele ukryte:

- [x] 1280 x 720: około 2700 klatek na sekundę przed zmianą i około 2500 po niej
- [x] 2560 x 1440: około 2020 przed zmianą i około 1960 po niej

Nowe wartości startowe (stare w nawiasie). Kolory są liczbami sRGB, jak w panelach:

| Ustawienie | Wartość |
|---|---|
| `Ambient` (panel Lights) | `(0.105, 0.135, 0.225)` (było `(0.035, 0.045, 0.075)`) |
| `Moon intensity` | 0,12 (było 0,3) |
| `Beam intensity` | 1,3 (było 1,6) |
| `Point intensity` | 0,9 (było 2,0) |
| `CRYSTAL_GLOW_STRENGTH` (stała, bez kontrolki) | 2,5 (było 1,0) |
| `Sky brightness` (panel Renderer) | 2,2, suwak do 6 (było 1,0, suwak do 3) |
| `Clear color` (panel Renderer) | `(0.022, 0.033, 0.088)` (było `(0.01, 0.015, 0.04)`) |
| `Exposure` (panel Framebuffers, nowe) | 1,0 |
| `Tone mapping` (nowe) | `ACES (fitted)` |
| `Depth range` (nowe) | 15 m |
| `FOLDED_ROW_COUNT` (stała układu paneli) | 3 (było 2): pasek HUD stoi o jeden pasek tytułu niżej |

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wersji kompilatora,
karty i sterownika, listy zrzutów ekranu, rozrzutu liczby klatek między uruchomieniami ani
tego, jak wygląda panel Assets na karcie bez rozszerzenia `GL_EXT_texture_sRGB_decode`
(tam podglądy tekstur sRGB powinny być ciemniejsze niż pliki).

Znane ograniczenia tej części, zapisane także w dokumentach modułów:

- framebuffer ma jeden załącznik koloru
- kolor `Kd` materiałów nie jest przeliczany z sRGB (wszystkie modele mają białe `Kd`)
- ścieżka framebuffera bez tekstury koloru (`glDrawBuffer(GL_NONE)` i
  `glReadBuffer(GL_NONE)`, przygotowana dla map cieni) **nigdy nie była wykonana**
  (stan z dnia tej części: od czwartej części M7 używa jej mapa cieni księżyca, sekcja
  20)
- bufor sceny ma pełną rozdzielczość okna

Żadnej nowej kontrolki nikt nie kliknął myszą, nikt nie zmieniał rozmiaru okna przez
przeciąganie krawędzi i nikt nie użył przycisku `Reload shaders` przy ośmiu programach. Na
macOS nic z tej części nie było budowane ani uruchamiane.

### 17.2. Otwarte: test ręczny na około piętnaście minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze zgłoszonych pomiarów,
nie z klikania. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/FramebuffersPanel.cpp`, `RendererPanel.cpp` i `AssetsPanel.cpp`.

**Uwaga po drugiej części M7.** Ta lista powstała dla programu z pierwszej części i nie
została wykonana, zanim doszedł bloom. Wykonując ją w dzisiejszym programie, trzeba
pamiętać o sześciu różnicach:

- okno debug (Post process / Previews) wygląda inaczej: kontrolki stoją w dwóch kolumnach, `Depth range`
  jest w tabeli nad kreską, pod linią `Scene framebuffer` jest linia `Bloom targets`,
  a obrazów jest cztery. Ten układ opisuje sekcja 18.2. Od trzeciej części M7 tabela
  kontrolek jest dodatkowo w zakładce `Tone and bloom`, obok zakładki `Fog and vignette`
  (sekcja 19.2), a kroki o wyglądzie sceny najlepiej wykonać z odznaczonymi polami `Fog`
  i `Vignette`, bo opisują obraz bez mgły i bez winiety
- podpisy obrazów brzmią dziś `HDR colour` i `Depth`, a nie `Colour (HDR, cut off at 1)`
  i `Depth (as distance)`. Dawne dopiski są w podpowiedziach po najechaniu kursorem
- kroki o wyglądzie kryształu, ekspozycji i krzywych najlepiej wykonać z **odznaczonym**
  polem `Bloom`, bo opisują obraz bez poświaty. Kryształy są przy tym jaśniejsze i bledsze
  niż w pierwszej części: `CRYSTAL_GLOW_STRENGTH` ma dziś 4,0, a nie 2,5 z tabeli wyżej
- lista w oknie debug (Diagnostics / Frame and shaders / Shaders) ma jedenaście linii, nie osiem. Linie `composite.vert +
  composite.frag` i `composite.vert + preview.frag` są siódmą i ósmą, nie dwiema
  ostatnimi, a zepsuty `composite.vert` daje cztery czerwone linie, nie dwie
- podglądy kosztują przy rozwiniętym panelu cztery małe przebiegi, nie dwa
- od czwartej części M7 w scenie są cienie księżyca, a `Moon intensity` ma 0,2, nie 0,12
  z tabeli wyżej. Kroki, które porównują obraz z wersją sprzed M7, najlepiej wykonać z
  odznaczonym polem `Shadows` w oknie debug (Light / Shadows) (sekcja 20.2)

Przygotowanie:

- [ ] **usunąć plik `imgui.ini`** z katalogu, z którego uruchamiam program (sekcja 7). Stary
      plik nie ma wpisu okna debug (Post process), więc ten stanie w miejscu z kodu, ale
      pozostałe zostaną tam, gdzie zapisał je plik, i układ może się nie zgadzać
- [ ] uruchomić grę z terminala. Oczekiwane w konsoli: żadnej linii `[error]`,
      w szczególności żadnej zaczynającej się od `Framebuffer of` ani
      `Framebuffer cannot be created`, i żadnej linii
      `Texture is asked for as sRGB and as linear`

Wygląd sceny startowej w porównaniu z poprzednią wersją (panele ukryte klawiszem akcentu):

- [ ] scena jest nadal nocą: ciemne ściany, wyraźna ciepła plama latarki, zimne światło
      księżyca na stronach ścian zwróconych do niego. Zapisać wrażenie: jaśniej, ciemniej
      czy podobnie jak przed M7, i czy w najciemniejszych kątach widać jeszcze fugi
- [ ] brzeg plamy latarki przechodzi w ciemność łagodniej niż przed M7, a jej środek nie
      jest płaską białą plamą: fugi i faktura kamienia są w nim widoczne
- [ ] kryształ jest najjaśniejszą rzeczą w swoim kącie i wyraźnie świeci, ale jego
      ścianki dają się odróżnić. Pulsuje razem ze światłem, które rzuca na ściany
- [ ] niebo: gwiazdy i tarcza księżyca są widoczne i jaśniejsze od reszty nieba. Zapisać,
      czy niebo nie jest za jasne w porównaniu ze ścianami
- [ ] trawa ma gradient od ciemnej nasady do jaśniejszego czubka, jak przedtem
- [ ] linie brył kolizji (okno debug (Diagnostics / Collision and picking), pole `Draw collision shapes`): kolory linii są takie
      jak przedtem, czyste i jasne, nie wyprane

Pasek HUD i panele bez zmian:

- [ ] pasek HUD i karta wygranej mają te same kolory co przed M7 (są rysowane po [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      ostatnim przebiegu, prosto do okna). Pasek stoi dziś o dwa paski tytułu niżej niż
      przed M7: jeden rząd doszedł w tej części, drugi w czwartej
- [ ] otworzyć panele klawiszem akcentu: kolory motywu, tła paneli i tekstu są takie jak
      przed M7. Nic nie jest rozjaśnione ani wyblakłe
- [ ] u góry okna, między kolumnami, są cztery rzędy pasków tytułu: Camera i Gameplay, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      pod nimi Terrain i Grass, pod nimi jeden szeroki pasek Framebuffers, szeroki jak dwa
      paski nad nim razem, a pod nim pasek Shadows tej samej szerokości (czwarty rząd
      doszedł w czwartej części M7). Wszystkie sześć są zwinięte i nie nachodzą na siebie
      ani na pasek HUD
- [ ] okno debug (Render / Scene): `Sky brightness` pokazuje 2,2, a suwak dochodzi do 6. `Clear color`
      pokazuje ciemny granat

Okno debug (Post process):

- [ ] rozwinąć panel (strzałka w pasku). Zawartość od góry: suwak `Exposure` (`1.00`), [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      lista `Tone mapping` (`ACES (fitted)`), kreska, linia
      `Scene framebuffer: 1280 x 720 px, GL_RGBA16F + GL_DEPTH_COMPONENT24` (rozmiar taki
      jak w linii `Framebuffer` okna debug (Render / Scene)), suwak `Depth range` (`15 m`) i dwa
      obrazy obok siebie z podpisami `Colour (HDR, cut off at 1)` i `Depth (as distance)`.
      Zapisać, czy zawartość mieści się bez przewijania
- [ ] w pierwszej klatce po rozwinięciu w miejscu obrazów może mignąć napis [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `(no picture yet)`: podglądy są rysowane dopiero w następnej klatce
- [ ] oba obrazy są **we właściwą stronę** (niebo u góry, podłoże na dole) i mają kształt
      okna
- [ ] rozwinięty panel zasłania scenę między kolumnami i żadnego innego otwartego panelu [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]

Ekspozycja:

- [ ] przesunąć `Exposure` w lewo do `0.10`: scena ciemnieje prawie do czerni, widać
      tylko kryształy, środek plamy latarki i księżyc
- [ ] przesunąć w prawo do `8.00`: scena jest bardzo jasna, plama latarki przepalona,
      ale przejścia do bieli są miękkie (krzywa ACES)
- [ ] suwak jest logarytmiczny: droga od 0,5 do 1 jest tak samo długa jak od 1 do 2
- [ ] podgląd `Colour (HDR, cut off at 1)` **nie zmienia się** przy ruchu suwaka: pokazuje
      zawartość bufora, a nie gotową klatkę
- [ ] wpisać wartość spoza zakresu (Ctrl i kliknięcie w suwak, potem na przykład `20`):
      zostaje przycięta do `8.00`
- [ ] ustawić z powrotem `1.00`

Mapowanie tonów, każda z trzech pozycji listy (ekspozycja 1, patrzeć na kryształ z bliska
i na plamę latarki na ścianie):

- [ ] `None (clamp)`: kryształ jest płaską plamą jednego jasnego koloru, bez widocznych
      ścianek. Ciemne partie sceny są jaśniejsze niż przy ACES
- [ ] `Reinhard`: nic nie jest przepalone, ale cała scena jest ciemniejsza i bardziej
      płaska, a kryształ nie dochodzi do pełnej jasności
- [ ] `ACES (fitted)`: większy kontrast, ciemne tony ciemniejsze, kryształ jasny
      z widocznymi ściankami
- [ ] przy każdej zmianie listy panele i pasek HUD nie zmieniają kolorów
- [ ] zostawić `ACES (fitted)`

Podglądy załączników:

- [ ] podgląd koloru pokazuje scenę bez ekspozycji i bez krzywej: jasne miejsca są obcięte
      do bieli (kryształy jako płaskie plamy), reszta wygląda jak przy `None (clamp)`
- [ ] podgląd głębi: to, co blisko kamery, jest ciemne, dalsze ściany jaśniejsze, niebo
      białe. Podejść do ściany: ściana ciemnieje
- [ ] `Depth range` na `2 m`: prawie wszystko jest białe, ciemne zostaje tylko to, co tuż
      przed kamerą. Na `100 m`: prawie wszystko jest ciemne, niebo zostaje białe
- [ ] obrócić kamerę: oba podglądy podążają za sceną bez opóźnienia widocznego gołym okiem
- [ ] zwinąć okno debug (Post process) i zapisać liczbę klatek z okna debug (Render / Scene), potem rozwinąć [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      i zapisać jeszcze raz: podglądy kosztują dwa małe przebiegi tylko przy rozwiniętym
      panelu
- [ ] ukryć wszystkie panele klawiszem akcentu przy rozwiniętym okna debug (Post process) [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      i pokazać je znowu: podglądy wracają, bez błędów w konsoli

Oba widoki diagnostyczne (okno debug (Render / Textures and normals), lista `View mode`):

- [ ] `Normals as colour`: płaska ściana zwrócona w +X jest czerwonawa, podłoże zielonkawe,
      jak przed M7. Kolory **nie zmieniają się** przy ruchu suwaka `Exposure` ani przy
      zmianie `Tone mapping`
- [ ] `UVs as colour`: gradient od czerni przez czerwień i zieleń, powtarzany tam, gdzie
      powtarza się tekstura. Też niezależny od `Exposure` i `Tone mapping`
- [ ] w obu widokach niebo i trawa pokazują swoje dane (kierunek, normalną albo
      współrzędne źdźbła), jak przed M7
- [ ] wrócić do `Textured`: `Exposure` i `Tone mapping` znów działają, a ich wartości
      w panelu są takie, jakie zostawiłem

Okno debug (Diagnostics / Assets):

- [ ] na liście `Textures` przy każdej pozycji stoi rozmiar i przestrzeń kolorów: `sRGB` przy
      `wall_stone.png`, `gate_wood.png`, `crystal.png` i `ground.png`, `linear` przy
      czterech plikach `..._normal.png`
- [ ] podglądy tekstur koloru wyglądają jak pliki otwarte w przeglądarce obrazów (nie są
      ciemniejsze), a podglądy map normalnych są jasnoniebieskie
- [ ] zmiana filtra i anizotropii działa jak przedtem

Zmiana rozmiaru okna:

- [ ] złapać krawędź okna myszą i przeciągać powoli w obie strony przez kilka sekund.
      Program nie ma funkcji odświeżania wołanej przez system w trakcie przeciągania,
      a system potrafi wtedy wstrzymać `glfwPollEvents`, więc obraz może być w tym czasie
      zamrożony albo rozciągnięty: zapisać, co widać. **Po puszczeniu krawędzi** obraz od
      razu wypełnia całe okno w nowym rozmiarze, bez czarnych pasów i bez rozciągnięcia,
      a w konsoli nie ma linii `[error]` (bufor sceny jest tworzony od nowa w pierwszej
      klatce z nowym rozmiarem)
- [ ] linia `Scene framebuffer` w oknie debug (Post process) pokazuje po puszczeniu krawędzi
      ten sam rozmiar co linia `Framebuffer` w oknie debug (Render / Scene)
- [ ] zrobić okno bardzo wąskie i bardzo niskie: gra działa, podglądy zachowują kształt
      okna
- [ ] zmaksymalizować okno i przywrócić je

Minimalizacja i przywrócenie:

- [ ] zminimalizować okno na kilka sekund i przywrócić je: obraz wraca od razu, bez
      czarnej klatki i bez linii `[error]`
- [ ] to samo przy rozwiniętym okna debug (Post process) [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
- [ ] to samo w buildzie Debug: żadnej linii `GL_` w konsoli

Przeładowanie shaderów (okno debug (Diagnostics / Frame and shaders / Shaders)):

- [ ] lista ma osiem linii. Dwie ostatnie to `composite.vert + composite.frag: OK`
      i `composite.vert + preview.frag: OK`
- [ ] nacisnąć `Reload shaders`: wszystkie osiem linii zostają `OK`, obraz się nie
      zmienia, podglądy działają dalej
- [ ] w pliku `assets/shaders/post/composite.frag` zamienić ostatnią linię `main` na
      `fragColor = vec4(color, 1.0);` i nacisnąć `Reload shaders`: scena wyraźnie
      ciemnieje (brak kodowania sRGB), a panele zostają bez zmian. Przywrócić linię
      i przeładować
- [ ] zepsuć `assets/shaders/common/color.glsl` (na przykład usunąć średnik)
      i przeładować: błąd z nazwą pliku `color.glsl` pojawia się przy każdym programie,
      który ten plik dołącza (`textured`, `skybox`, `grass`, `composite`, `preview`, a od
      drugiej części M7 także `bright`), a gra
      działa dalej na poprzednich wersjach programów. Naprawić i przeładować
- [ ] zepsuć `assets/shaders/post/composite.vert` i przeładować: dwie ostatnie linie są
      czerwone, obraz zostaje (stare programy działają dalej). Naprawić i przeładować

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza wywołanymi
      celowo
- [ ] liczba klatek na sekundę w Release przy ustawieniach startowych, z okna debug (Render / Scene),
      w oknie 1280 x 720 i po zmaksymalizowaniu: zapisać obie wartości. Zgłoszone około
      2500 i około 1960 (w 2560 x 1440) zmierzono przy wyłączonej synchronizacji pionowej
      i ukrytych panelach, więc liczba z panelu może być inna. Jeśli stoi na częstotliwości
      odświeżania monitora, zapisać to
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 18. Lista kontrolna M7, część 2: bloom

Druga część kamienia milowego M7 dodaje pierwszy efekt liczony z gotowej sceny: poświatę
wokół jasnych miejsc (bloom). Po scenie, a przed przebiegiem składającym, gra rysuje trzy
kroki w trzech celach `GL_RGBA16F` o połowie szerokości i połowie wysokości bufora sceny:
przebieg jasności (zostaje światło jaśniejsze od progu), rozmycie Gaussa (rozdzielne,
poziomo i pionowo, startowo sześć powtórzeń) i dodanie wyniku do sceny przed ekspozycją
i mapowaniem tonów. Doszły: pliki `src/game/Bloom.hpp` i `Bloom.cpp` (ustawienia
`BloomSettings`, funkcje `bloomTargetExtent` i `bloomBlurWeights`, w bibliotece
`game_logic`), dziewiąty i dziesiąty program shaderów (`post/composite.vert`
z `post/bright.frag` i z `post/blur.frag`) oraz plik testów `tests/BloomTests.cpp`.
Zmieniły się: klasa `game::PostProcess` (funkcja `drawBloom`, trzy cele bloomu, dwa
kolejne podglądy), `post/composite.frag` (uniformy `uBloom`, `uBloomEnabled`,
`uBloomIntensity`), `common/color.glsl` (funkcja `luminance`), klasa `gfx::Shader` (setter
tablicy `setFloatArray`), `NightMazeApp` (dwa programy i wywołanie `drawBloom`
w `onRender`), `ShaderUniforms.hpp` (osiem nazw), panel Framebuffers, `DebugContext` (dwa
pola, razem trzydzieści), `DebugUI::draw` (`SHADER_COUNT` równe 10) oraz jedna stała gry:
`CRYSTAL_GLOW_STRENGTH` wynosi 4,0 zamiast 2,5.
Opis kodu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md)
(sekcje 2.11 do 2.15: dlaczego bloom potrzebuje HDR, luminancja, wzór przebiegu jasności,
wagi Gaussa, rozmycie rozdzielne, ping-pong, połowa rozdzielczości, dodanie przed
mapowaniem tonów. Sekcje 4.6 do 4.8, 5.10 i 5.11: shadery i kod linia po linii),
[`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md) (sekcja 5.8: `setFloatArray`),
[`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) (sekcja 5.14: cele bloomu
jako użytkownicy klasy), [`../modules/debug-ui.md`](../modules/debug-ui.md) (panel
Framebuffers w nowym układzie). Decyzje:
[`../decisions/bloom-half-resolution-three-targets.md`](../decisions/bloom-half-resolution-three-targets.md),
[`../decisions/bright-pass-keeps-hue.md`](../decisions/bright-pass-keeps-hue.md),
[`../decisions/blur-weights-computed-on-cpu.md`](../decisions/blur-weights-computed-on-cpu.md)
i [`../decisions/crystal-glow-raised-for-bloom.md`](../decisions/crystal-glow-raised-for-bloom.md).

Kamień milowy M7 jest nadal **rozpoczęty i nie jest kompletny**. Temat 10 wykładu jest
w toku: są bufor HDR, przebieg składający, podglądy załączników i bloom. Mgła i winieta
doszły w części trzeciej (sekcja 19), a cienie księżyca (temat 11) w czwartej (sekcja
20). **Nie są zbudowane:** cienie latarki i minimapa.
Część ręczna poniżej (18.2) jest otwarta w całości, na macOS kod nie był budowany
([`build-macos.md`](build-macos.md)) i nie ma tagu.

### 18.1. Zgłoszone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).
**Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej sekcji**: wszystkie są
zgłoszone z dnia, w którym powstał kod. Sam policzyłem tylko liczbę makr `TEST_CASE`
w plikach testów (276) i asercje w `BloomTests.cpp` (36), a liczby w tabelach wartości
startowych przepisałem z kodu.

Bramka i testy:

- [x] `make check` przechodzi: formatowanie (clang-format), buildy Debug i Release, testy
      w obu, clang-tidy
- [x] build Debug i Release: zero ostrzeżeń
- [x] `night_maze_tests.exe` w Debug i w Release: 276 przypadków testowych i 102139
      asercji, wszystkie przechodzą. Względem pierwszej części M7 (269 przypadków
      i 102103 asercje) doszedł plik `BloomTests.cpp`: 7 przypadków i 36 asercji,
      `269 + 7 = 276` i `102103 + 36 = 102139`

Porównanie obrazu z pierwszą częścią M7:

- [x] z odznaczonym polem `Bloom` obraz jest identyczny co do piksela z obrazem pierwszej
      części, w sześciu widokach. Pomiar zrobiono **przed** podniesieniem
      `CRYSTAL_GLOW_STRENGTH`, więc dotyczy samego dodania bloomu do potoku
- [x] dziś kryształy różnią się od pierwszej części **z założenia**, także przy
      wyłączonym bloomie: `CRYSTAL_GLOW_STRENGTH` ma 4,0 zamiast 2,5 i kryształy są bledsze

Zrzuty ekranu (robione bez myszy):

- [x] poświata kryształu w najciemniejszej chwili pulsu i w najjaśniejszej, w trybach
      `Unlit`, `Gouraud` i `Blinn-Phong`, ze ściankami kryształu nadal widocznymi
- [x] poświata wokół tarczy księżyca
- [x] gwiazdy zostają punktami, bez poświaty
- [x] próg 0,3 i próg 2,0
- [x] panel Framebuffers z czterema podglądami
- [x] okno zmienione na 1000 x 600: cele bloomu mają 500 x 300
- [x] widok normalnych: w miejscu dwóch obrazów bloomu stoi `(not drawn)`

Koszt, Release, panele ukryte. Pomiar jest niespokojny, dlatego zakresy:

- [x] 1280 x 720: od 1900 do 2450 klatek na sekundę z wyłączonym bloomem, od 1500 do 2150
      z włączonym
- [x] 2560 x 1440: od 1370 do 1480 z wyłączonym bloomem, od 880 do 925 z włączonym

W czasie klatki to w większym oknie około 0,7 ms bez bloomu i około 1,1 ms z nim, czyli
bloom kosztuje tam około 0,4 ms. W mniejszym oknie zakresy na siebie zachodzą. Tych liczb
nie da się zestawić z sekcją 17.1 (około 2500 i około 1960): to inna sesja pomiarowa,
a sama wartość bez bloomu w 2560 x 1440 jest tu o jedną czwartą niższa niż tam, czego
żadna zmiana w kodzie nie tłumaczy.

Wartości startowe (nowe albo zmienione w tej części):

| Ustawienie | Wartość |
|---|---|
| `Bloom` (panel Framebuffers, nowe) | zaznaczone |
| `Threshold` (nowe) | 0,80, suwak od 0 do 4 |
| `Intensity` (nowe) | 1,00, suwak od 0 do 2 |
| `Blur iterations` (nowe) | 6, suwak od 1 do 10 |
| `CRYSTAL_GLOW_STRENGTH` (stała, bez kontrolki) | 4,0 (było 2,5) |
| `BLOOM_DOWNSCALE` (stała) | 2: cele bloomu mają połowę szerokości i wysokości sceny |
| `BLOOM_BLUR_RADIUS`, `BLOOM_BLUR_SIGMA` (stałe) | 6 i 3,0: 13 odczytów tekstury na przebieg rozmycia |
| `SHADER_COUNT` (stała panelu Shaders) | 10 (było 8) |

Otwarte obserwacje, zgłoszone razem z kodem:

- plama latarki na ścianie **nie daje poświaty**, nawet z odległości metra. Ściana
  w świetle latarki zostaje pod progiem 0,8. Czy tak ma zostać, nikt jeszcze nie
  zdecydował
- poświata jest mierzona w tekselach celu o połowie rozdzielczości, więc w 2560 x 1440
  jest względem ekranu o połowę cieńsza niż w 1280 x 720. W większym oknie sprawdzono
  tylko wycinek obrazu

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wersji kompilatora,
karty i sterownika, pamięci karty zajętej przez trzy cele (liczby w dokumencie modułu są
policzone z rozmiaru), kosztu przy innej liczbie iteracji niż 6 ani tego, czy siedem
kontrolek i cztery obrazy mieszczą się w panelu bez paska przewijania przy skali innej
niż 100%.

Żadnej kontrolki bloomu nikt nie kliknął myszą i nikt nie użył przycisku `Reload shaders`
przy dziesięciu programach. Na macOS nic z tej części nie było budowane ani uruchamiane.

### 18.2. Otwarte: test ręczny na około piętnaście minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze zgłoszonych zrzutów,
nie z klikania. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/FramebuffersPanel.cpp`.

Przygotowanie:

- [ ] uruchomić grę z terminala. Oczekiwane w konsoli: żadnej linii `[error]`,
      w szczególności żadnej zaczynającej się od `Framebuffer of` ani
      `Framebuffer cannot be created` (cele bloomu powstają w pierwszej klatce)
- [ ] otworzyć panele klawiszem akcentu i rozwinąć okno debug (Post process) [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]

Okno debug (Post process) w nowym układzie:

- [ ] kontrolki stoją w dwóch kolumnach, wierszami: `Exposure` i `Tone mapping`, `Bloom`
      i `Blur iterations`, `Threshold` i `Intensity`, `Depth range` i puste miejsce.
      Wartości startowe: `1.00`, `ACES (fitted)`, zaznaczone, `6`, `0.80`, `1.00`, `15 m`
- [ ] pod kreską dwie linie: `Scene framebuffer: 1280 x 720 px, GL_RGBA16F +
      GL_DEPTH_COMPONENT24` i `Bloom targets (3): 640 x 360 px, GL_RGBA16F`
- [ ] pod nimi cztery obrazy obok siebie z podpisami `HDR colour`, `Depth`, `Bright pass`
      i `Bloom`. Wszystkie we właściwą stronę (niebo u góry) i w kształcie okna
- [ ] zapisać, czy cała zawartość mieści się w panelu bez paska przewijania
- [ ] najechać kursorem na każdą z siedmiu kontrolek i na każdy z czterech obrazów:
      przy każdym pojawia się podpowiedź. Przy obrazach: `The colour attachment of the
      scene, cut off at 1.`, `The depth attachment of the scene, as a distance.`, `What
      the scene has above the bloom threshold.`, `The bright pass after the blur, before
      the intensity.`

Włącznik:

- [ ] stanąć kilka metrów od kryształu, tak żeby było go widać razem z otoczeniem.
      Odznaczyć `Bloom`: poświata wokół kryształu znika, sam kryształ zostaje. Reszta
      sceny się nie zmienia
- [ ] przy odznaczonym polu druga linia brzmi `Bloom targets: not drawn (bloom off or
      a debug view)`, a w miejscu obrazów `Bright pass` i `Bloom` stoi `(not drawn)`.
      Obrazy `HDR colour` i `Depth` działają dalej
- [ ] zaznaczyć z powrotem: poświata i oba obrazy wracają od razu

Próg:

- [ ] przesunąć `Threshold` w lewo do `0.00`: świeci cały obraz, scena robi się mleczna
      i jaśniejsza, a obraz `Bright pass` wygląda jak obraz `HDR colour` (przy progu 0 przebieg
      jasności przepuszcza cały kolor)
- [ ] ustawić około `0.30`: w obrazie `Bright pass` widać niebo i ściany w świetle
      latarki, a w oknie mają one poświatę
- [ ] wrócić na `0.80`: w obrazie `Bright pass` zostają kryształy i tarcza księżyca, na
      czarnym tle, z **ostrymi** krawędziami
- [ ] przesuwać powoli od `0.80` do `2.00`: plamy w obrazie `Bright pass` ciemnieją
      i kurczą się **płynnie**, nic nie znika skokiem. Przy `2.00` zostaje sam środek
      kryształu
- [ ] przesunąć do `4.00`: obraz `Bright pass` jest czarny albo prawie czarny. Zapisać,
      czy cokolwiek w scenie jeszcze przechodzi przez próg
- [ ] wpisać wartość spoza zakresu (Ctrl i kliknięcie, potem `9`): zostaje przycięta do
      `4.00`. Ustawić z powrotem `0.80`

Intensywność:

- [ ] `Intensity` na `0.00`: poświata znika z okna, ale obrazy `Bright pass` i `Bloom`
      w panelu **się nie zmieniają** (pokazują cele przed mnożeniem)
- [ ] `Intensity` na `2.00`: poświata jest wyraźnie mocniejsza, środek kryształu bieleje
- [ ] ustawić z powrotem `1.00`

Iteracje:

- [ ] `Blur iterations` na `1`: poświata jest wąską obwódką tuż przy krysztale, obraz
      `Bloom` jest tylko lekko rozmyty
- [ ] `Blur iterations` na `10`: poświata jest szeroka i miękka. Jest **okrągła**, nie
      kwadratowa i nie w kształcie krzyża
- [ ] przy każdej wartości zapisać liczbę klatek z okna debug (Render / Scene) (z odznaczoną
      synchronizacją pionową, jeśli się da): każda iteracja to dwa przebiegi więcej
- [ ] obraz `Bright pass` **nie zmienia się** przy ruchu tego suwaka: rozmycie go nie
      dotyka
- [ ] ustawić z powrotem `6`

Podglądy:

- [ ] obraz `Bloom` to te same plamy co w `Bright pass`, rozlane w miękkie koła
- [ ] obraz `HDR colour` **nie ma** poświaty: bloom nie jest zapisywany do bufora sceny
- [ ] obrócić kamerę: wszystkie cztery obrazy podążają za sceną bez opóźnienia widocznego
      gołym okiem
- [ ] zwinąć panel i zapisać liczbę klatek, rozwinąć i zapisać jeszcze raz: przy [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      rozwiniętym panelu dochodzą cztery małe przebiegi (w pierwszej części M7 dwa)

Kryształ z bliska i z daleka:

- [ ] podejść do kryształu na krok: ścianki są widoczne (kryształ nie jest płaską białą
      plamą), a poświata wychodzi poza jego obrys
- [ ] patrzeć na niego przez kilka pełnych pulsów (jeden trwa 2,4 sekundy): poświata
      słabnie i rośnie **płynnie** i w żadnej chwili nie znika całkiem. To jest powód,
      dla którego siła świecenia wzrosła do 4,0
- [ ] odejść na koniec długiego korytarza: kryształ jest mały, ale poświata nadal jest
      widoczna i nie migocze przy ruchu kamery. Zapisać, czy przy powolnym obrocie widać
      drganie jasności
- [ ] stanąć tak, żeby kryształ był zasłonięty ścianą do połowy: poświata widocznej
      połowy wychodzi także na ścianę, która go zasłania (bloom jest liczony z obrazu,
      nie z geometrii). Zapisać, czy to nie razi
- [ ] zebrać kryształ: jego poświata znika razem z nim
- [ ] przełączyć listę `Lighting` w oknie debug (Render / Scene) na `Unlit`, `Gouraud`, `Phong`
      i `Blinn-Phong`: kryształ ma poświatę w każdym trybie

Księżyc i gwiazdy:

- [ ] spojrzeć na księżyc: tarcza ma miękką poświatę
- [ ] gwiazdy zostają ostrymi punktami, bez poświaty. Zapisać, czy najjaśniejsze mają
      choć ślad
- [ ] przesunąć `Sky brightness` w oknie debug (Render / Scene) do 6: poświata księżyca rośnie.
      Zapisać, czy gwiazdy zaczynają świecić. Wrócić na 2,2
- [ ] odznaczyć pole `Skybox`: poświata księżyca znika razem z niebem

Latarka (otwarta obserwacja z sekcji 18.1):

- [ ] podejść do ściany na metr ze światłem latarki na wprost: zapisać, czy plama latarki
      ma poświatę. Zgłoszone: nie ma
- [ ] przy tej samej ścianie obniżać `Threshold`, aż plama zacznie świecić: zapisać
      wartość. To jest liczba potrzebna do decyzji, czy próg albo latarkę zmienić

Ekspozycja i mapowanie tonów razem z bloomem:

- [ ] `Exposure` na `0.25`: poświata ciemnieje razem ze sceną, a obrazy `Bright pass`
      i `Bloom` w panelu się nie zmieniają (próg działa przed ekspozycją). Wrócić na `1.00`
- [ ] `Tone mapping` na `None (clamp)`: środek kryształu z poświatą jest płaską białą
      plamą. Na `Reinhard`: zapisać, jak wygląda poświata. Zostawić `ACES (fitted)`

Oba widoki diagnostyczne (okno debug (Render / Textures and normals), lista `View mode`):

- [ ] `Normals as colour`: obraz jest taki jak przed tą częścią, bez żadnej poświaty.
      Pole `Bloom` w oknie debug (Post process) jest **nadal zaznaczone**, druga linia brzmi
      `Bloom targets: not drawn (bloom off or a debug view)`, a w miejscu dwóch obrazów
      bloomu stoi `(not drawn)`
- [ ] `UVs as colour`: to samo. Jasne żółte rogi płytek tekstury nie mają poświaty
- [ ] w obu widokach ruch suwaków `Threshold`, `Intensity` i `Blur iterations` nie
      zmienia obrazu
- [ ] wrócić do `Textured`: poświata wraca, a ustawienia bloomu są takie, jakie zostawiłem

Zmiana rozmiaru okna:

- [ ] przeciągnąć krawędź okna i puścić: linia `Bloom targets (3)` pokazuje połowę liczb
      z linii `Scene framebuffer` (dla nieparzystych zaokrągloną w dół), a poświata jest
      na swoim miejscu, nieprzesunięta względem kryształu
- [ ] zrobić okno bardzo wąskie i bardzo niskie: gra działa, w konsoli nie ma linii
      `[error]`
- [ ] zmaksymalizować okno: poświata jest względem ekranu cieńsza niż w oknie
      1280 x 720. Zapisać, czy to przeszkadza, i przy jakiej wartości `Blur iterations`
      wygląda jak w małym oknie
- [ ] zminimalizować okno na kilka sekund i przywrócić: obraz z poświatą wraca od razu,
      bez linii `[error]`, także w buildzie Debug bez linii `GL_`

Przeładowanie shaderów (okno debug (Diagnostics / Frame and shaders / Shaders)):

- [ ] lista ma jedenaście linii. Linie od siódmej do dziesiątej to
      `composite.vert + composite.frag: OK`,
      `composite.vert + preview.frag: OK`, `composite.vert + bright.frag: OK`
      i `composite.vert + blur.frag: OK` (po tej części były ostatnie: jedenasta,
      `shadow_depth.vert + shadow_depth.frag: OK`, doszła w czwartej części)
- [ ] nacisnąć `Reload shaders`: wszystkie jedenaście linii zostaje `OK`, a poświata
      wygląda tak samo jak przed kliknięciem (wagi rozmycia są wysyłane w każdej klatce)
- [ ] w pliku `assets/shaders/post/bright.frag` zamienić linię z `share` na
      `float share = brightness > uThreshold ? 1.0 : 0.0;`, skopiować assety
      (`cmake --build --preset debug --target copy_assets`) i przeładować: poświata jest
      mocniejsza, a brzeg plam w obrazie `Bright pass` jest ostry jak wycięty nożem.
      Przywrócić linię i przeładować
- [ ] zepsuć `assets/shaders/post/blur.frag` (na przykład usunąć średnik) i przeładować:
      linia `blur` jest czerwona z nazwą pliku w komunikacie, a gra działa dalej ze starym
      programem. Naprawić i przeładować
- [ ] zepsuć `assets/shaders/post/composite.vert` i przeładować: **cztery** linie, od
      siódmej do dziesiątej, są czerwone (w pierwszej części M7 były to dwie), linia
      `shadow_depth` zostaje `OK`, obraz zostaje. Naprawić
      i przeładować

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza wywołanymi
      celowo
- [ ] liczba klatek na sekundę w Release przy ustawieniach startowych, z okna debug (Render / Scene),
      z bloomem i bez, w oknie 1280 x 720 i po zmaksymalizowaniu: zapisać cztery wartości.
      Zgłoszone zakresy są w sekcji 18.1, zmierzone przy ukrytych panelach. Jeśli liczba
      stoi na częstotliwości odświeżania monitora, zapisać to
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 19. Lista kontrolna M7, część 3: mgła i winieta

Trzecia część kamienia milowego M7 dodaje dwa efekty, które nie potrzebują ani nowego
przebiegu, ani nowego bufora: oba są liniami istniejącego przebiegu składającego
(`post/composite.frag`). Mgła miesza kolor sceny z kolorem mgły tym mocniej, im dalej od
oka i im niżej leży powierzchnia. Odległość i wysokość bierze z pozycji w świecie, którą
shader odtwarza dla każdego piksela z tekstury głębi sceny i z macierzy odwrotnej do
`projection * view`. Winieta przyciemnia rogi gotowego obrazu. Kolejność w `main` ma
teraz siedem kroków: scena, mgła, bloom, ekspozycja, mapowanie tonów, winieta, kodowanie
sRGB. Doszły: pliki `src/game/Fog.hpp` i `Fog.cpp` (ustawienia `FogSettings`, funkcje
`fogHeightFactor`, `fogAmount`, `fogAmountAt` i `worldPositionFromDepth`),
`src/game/Vignette.hpp` i `Vignette.cpp` (ustawienia `VignetteSettings`, funkcja
`vignetteFactor`, stałe `SCREEN_CENTER` i `VIGNETTE_CORNER_DISTANCE`), wszystkie
w bibliotece `game_logic`, oraz pliki testów `tests/FogTests.cpp`
i `tests/VignetteTests.cpp`. Zmieniły się: `post/composite.frag` (jedenaście nowych
uniformów i cztery funkcje), klasa `game::PostProcess` (funkcja `composite` dostaje
strukturę `SceneView` z macierzą odwrotną i pozycją oka, a tekstura głębi sceny trafia na
jednostkę teksturującą 2), `PostProcessSettings` (pola `fog` i `vignette`),
`ShaderUniforms.hpp` (jedenaście nazw), `NightMazeApp` (budowa `SceneView` w `onRender`,
a kopia ustawień dla widoków diagnostycznych wyłącza już pięć rzeczy), panel Framebuffers
(dwie zakładki), `DebugContext.hpp` (sam komentarz, pól było nadal trzydzieści: dziś,
po czwartej części było ich trzydzieści cztery, dziś, po piątej, trzydzieści osiem) i
`CMakeLists.txt`. **Nie doszedł**
żaden program shaderów (było ich nadal dziesięć, dziś jest jedenaście), żaden
framebuffer ani żaden panel (było ich nadal jedenaście, dziś jest dwanaście).
Opis kodu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md)
(sekcje 2.17 do 2.22: prawo wykładnicze mgły, odległość od oka zamiast głębi, pozycja
w świecie z głębi, współczynnik wysokości i to, co psuje branie go w samym pikselu, niebo,
kolor mgły i miejsce w kolejności, winieta. Sekcje 4.2 i 4.9, 5.6, 5.7, 5.12 i 5.13:
shader i kod linia po linii), [`../modules/debug-ui.md`](../modules/debug-ui.md) (panel
Framebuffers z dwiema zakładkami). Decyzje:
[`../decisions/fog-distance-from-reconstructed-position.md`](../decisions/fog-distance-from-reconstructed-position.md),
[`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md),
[`../decisions/fog-no-special-case-for-sky.md`](../decisions/fog-no-special-case-for-sky.md),
[`../decisions/bloom-from-unfogged-scene.md`](../decisions/bloom-from-unfogged-scene.md)
i [`../decisions/vignette-not-aspect-corrected.md`](../decisions/vignette-not-aspect-corrected.md).

Kamień milowy M7 jest nadal **rozpoczęty i nie jest kompletny**. Temat 10 wykładu jest
w toku: są bufor HDR, przebieg składający, podglądy załączników, bloom, mgła i winieta.
Cienie księżyca (temat 11) doszły w części czwartej (sekcja 20). **Nie są zbudowane:**
cienie latarki i minimapa. Część ręczna poniżej (19.2) jest
otwarta w całości, tak samo jak części ręczne 17.2, 18.2 i 20.2, na macOS kod nie był budowany
([`build-macos.md`](build-macos.md)) i nie ma tagu.

### 19.1. Zgłoszone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).
**Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej sekcji**: wszystkie są
zgłoszone z dnia, w którym powstał kod. Sam policzyłem tylko makra `TEST_CASE` w plikach
testów (294 w dwudziestu czterech plikach, w tym 11 w `FogTests.cpp` i 7
w `VignetteTests.cpp`), a liczby w tabeli wartości startowych i zakresy suwaków
przepisałem z kodu. Liczby asercji są zgłoszone: asercji wykonanych nie da się policzyć
z samego kodu, bo część stoi w pętlach.

Bramka i testy:

- [x] `make check` przechodzi: formatowanie (clang-format), buildy Debug i Release, testy
      w obu, clang-tidy
- [x] build Debug i Release: zero ostrzeżeń
- [x] `night_maze_tests.exe` w Debug i w Release: 294 przypadki testowe i 102412 asercji,
      wszystkie przechodzą. Względem drugiej części M7 (276 przypadków i 102139 asercji)
      doszły dwa pliki: `FogTests.cpp` (11 przypadków i 241 asercji)
      i `VignetteTests.cpp` (7 przypadków i 32 asercje), razem 18 przypadków i 273
      asercje, `276 + 18 = 294` i `102139 + 273 = 102412`

Porównanie obrazu z drugą częścią M7:

- [x] z odznaczonymi polami `Fog` i `Vignette` obraz jest identyczny co do piksela
      z obrazem drugiej części. Przy wyłączonej mgle shader nie czyta tekstury głębi
      i nie jest ona wiązana
- [x] oba widoki diagnostyczne (`Normals as colour` i `UVs as colour`) są identyczne
      z drugą częścią **przy włączonych wartościach startowych**, czyli z zaznaczonymi
      polami `Fog` i `Vignette`: kopia ustawień dla tych widoków wyłącza oba efekty

Koszt, Release, panele ukryte. **Jeden spokojny przebieg**, nie zakresy z kilku:

- [x] 1280 x 720: około 1880 klatek na sekundę z oboma efektami i około 1900 bez nich
- [x] 2560 x 1440: około 1145 z oboma efektami i około 1158 bez nich

To jest różnica około 1 %, czyli przy jednym przebiegu w granicach szumu pomiaru: z tych
liczb wynika tylko tyle, że kosztu nie widać. Nie da się ich zestawić z zakresami
z sekcji 18.1 (inna sesja pomiarowa).

Wartości startowe (wszystkie nowe). Kontrolki stoją w panelu Framebuffers, w zakładce
`Fog and vignette`. W nawiasie jest napis, jaki pokazuje suwak:

| Ustawienie | Wartość |
|---|---|
| `Fog` | zaznaczone |
| `Density` | 0,1 na metr (`0.100 /m`), suwak od 0 do 0,5 |
| `Base height` | 0,5 m (`0.50 m`), suwak od -2 do 6 |
| `Height falloff` | 0,4 na metr (`0.40 /m`), suwak od 0 do 3 |
| `Fog colour` | `(0.14, 0.18, 0.26)` jako sRGB, pole wyboru koloru bez zakresu |
| `Vignette` | zaznaczone |
| `Strength` | 0,3 (`0.30`), suwak od 0 do 1 |
| `Radius` | 0,4 (`0.40`), suwak od 0 do 0,65 |
| `VIGNETTE_CORNER_DISTANCE` (stała, bez kontrolki) | 0,70710678: odległość od środka ekranu do rogu we współrzędnych tekstury |
| `DEPTH_TEXTURE_UNIT` (stała) | 2: jednostka teksturująca tekstury głębi w przebiegu składającym |

Bez zmian zostały: `SHADER_COUNT` (10), wysokość panelu `FRAMEBUFFERS_HEIGHT` (344)
i siedem kontrolek z poprzednich części, które stoją teraz w zakładce `Tone and bloom`.

Co te wartości znaczą w liczbach (policzone ze wzoru, nie zmierzone na ekranie): na
podłożu mgła zabiera 18 % koloru po 2 m, 63 % po 10 m i 95 % po 30 m, a połowę po 6,9 m.
Współczynnik wysokości wynosi 1 do wysokości 0,5 m, 0,67 na 1,5 m i 0,37 na 3 m, czyli na
wierzchu ściany. Winieta mnoży środek ekranu przez 1,0, środek każdej krawędzi przez
0,925, a róg przez 0,70.

Znane ograniczenia tej części, zapisane także w kodzie i w dokumencie modułu:

- wysokość jest brana **w samym pikselu**, a nie sumowana wzdłuż promienia. Z 30 m nad
  podłożem, patrząc prosto w dół, podłoże dostaje około 95 % mgły, a dokładna całka dałaby
  około 22 %: z wysoka w trybie noclip labirynt jest prawie zakryty
  ([`../decisions/fog-height-at-the-pixel.md`](../decisions/fog-height-at-the-pixel.md))
- niebo nie ma osobnego przypadku: piksel nieba ma głębię 1, czyli punkt na dalekiej
  płaszczyźnie obcinania (100 m). Ta płaszczyzna jest płaska i obraca się z kamerą, więc
  ten sam niski pas nieba jest 100 m od oka na środku ekranu i od 143 do 155 m przy
  bokach. Mgła na ścianach i na podłożu od obrotu kamery nie zależy, ale cienki pas
  zamglenia na samym niebie może się przy obrocie lekko zmieniać. Tego nikt nie oglądał:
  jest punktem listy 19.2
- winieta nie jest poprawiana o proporcje okna: jasny środek jest elipsą w kształcie okna

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wersji kompilatora,
karty i sterownika, listy zrzutów ekranu ani liczby widoków, w których porównywano obraz
z drugą częścią, tego, czy przy pomiarze liczby klatek bloom był włączony, rozrzutu
liczby klatek między uruchomieniami, kosztu samej mgły i samej winiety osobno, zachowania
buildu Debug (linie `GL_`) przy włączonej mgle po zmianie rozmiaru okna i po minimalizacji
ani tego, czy zakładki, osiem kontrolek i cztery obrazy mieszczą się w panelu bez paska
przewijania.

Żadnej kontrolki mgły ani winiety nikt nie kliknął myszą, nikt nie przełączył zakładki
i nikt nie użył przycisku `Reload shaders` po tej zmianie. Na macOS nic z tej części nie
było budowane ani uruchamiane.

### 19.2. Otwarte: test ręczny na około dwadzieścia minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze wzorów, nie z klikania
ani ze zrzutów ekranu. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/FramebuffersPanel.cpp`, `RendererPanel.cpp`, `AssetsPanel.cpp`
i `CameraPanel.cpp`. Ustawień mgły i winiety program nigdzie nie zapisuje, więc ponowne
uruchomienie gry zawsze przywraca wartości startowe.

Listy 17.2 i 18.2 też są otwarte. Ich kroki dotyczące kontrolek okna debug (Post process)
wykonuje się teraz w zakładce `Tone and bloom`.

Przygotowanie:

- [ ] uruchomić grę z terminala. Oczekiwane w konsoli: żadnej linii `[error]`,
      w szczególności żadnej o shaderze `composite.frag`
- [ ] otworzyć panele klawiszem akcentu i rozwinąć okno debug (Post process) [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]

Okno debug (Post process) w nowym układzie:

- [ ] na górze panelu jest pasek z dwiema zakładkami: `Tone and bloom`
      i `Fog and vignette`. Po starcie wybrana jest pierwsza
- [ ] zakładka `Tone and bloom` ma te same siedem kontrolek co w sekcji 18.2, w dwóch [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      kolumnach i czterech wierszach: `Exposure` i `Tone mapping`, `Bloom`
      i `Blur iterations`, `Threshold` i `Intensity`, `Depth range` i puste miejsce.
      Wartości startowe: `1.00`, `ACES (fitted)`, zaznaczone, `6`, `0.80`, `1.00`, `15 m`
- [ ] kliknąć zakładkę `Fog and vignette`: osiem kontrolek w dwóch kolumnach i czterech [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      wierszach: `Fog` i `Density`, `Base height` i `Height falloff`, `Fog colour`
      i `Vignette`, `Strength` i `Radius`
- [ ] wartości startowe w tej zakładce: `Fog` zaznaczone, `0.100 /m`, `0.50 m`,
      `0.40 /m`, `Vignette` zaznaczone, `0.30`, `0.40`. `Fog colour` to trzy pola
      z liczbami i kwadrat z kolorem: oczekiwane `36`, `46` i `66` (R, G, B w skali od 0 do
      255, bo kod nie zmienia domyślnego trybu `ColorEdit3`) i ciemny, szaroniebieski
      kwadrat. Zapisać, jeśli pola pokazują co innego
- [ ] pod zakładkami nic się nie zmieniło: kreska, linie `Scene framebuffer: 1280 x 720
      px, GL_RGBA16F + GL_DEPTH_COMPONENT24` i `Bloom targets (3): 640 x 360 px,
      GL_RGBA16F`, a pod nimi cztery obrazy `HDR colour`, `Depth`, `Bright pass` i `Bloom`
- [ ] przełączyć zakładki kilka razy: linie i obrazy pod nimi nie skaczą w pionie (obie
      zakładki mają po cztery wiersze), wartości kontrolek zostają, a obraz gry się nie
      zmienia
- [ ] zapisać, czy w **każdej** z dwóch zakładek dawnego panelu cała zawartość mieściła się w panelu bez
      paska przewijania [od 2026-10-06 bez odpowiednika w oknie debug: dziś karty kategorii Post process przewija okno]. Wysokość panelu (344) nie została zmieniona, a doszedł wiersz
      zakładek
- [ ] najechać kursorem na każdą z piętnastu kontrolek (siedem w pierwszej zakładce, osiem
      w drugiej) i na każdy z czterech obrazów: przy każdym pojawia się podpowiedź.
      Podpowiedź przy `Tone mapping` kończy się teraz słowami `without exposure, tone
      mapping, bloom, fog and vignette.` Początki ośmiu nowych: `Far and low surfaces fade
      into the fog colour.`, `Fog: amount = 1 - exp(-density * height factor *
      distance).`, `Fog: up to this world height the fog has its full density.`, `Fog: how
      fast it thins out above the base height:`, `The colour surfaces fade into, as an
      sRGB value.`, `The corners of the finished picture are darkened, after tone
      mapping.`, `Vignette: the share of the light the corners lose.`, `Vignette: the
      distance from the middle of the screen at which the darkening starts.`

Włącznik mgły:

- [ ] stanąć na początku długiego korytarza. Odznaczyć `Fog`: koniec korytarza, dalekie
      podłoże i niebo tuż nad horyzontem odzyskują własne kolory. Najmniej zmienia się
      to, co stoi blisko: dwa metry przed kamerą mgła zabiera na podłożu 18 %, a na
      ścianie na wysokości oczu mniej
- [ ] zaznaczyć z powrotem: mgła wraca od razu
- [ ] przy odznaczonym polu `Fog` ruch suwaków `Density`, `Base height`
      i `Height falloff` oraz zmiana `Fog colour` nie zmieniają obrazu
- [ ] cztery obrazy w panelu są takie same przy zaznaczonym i przy odznaczonym polu `Fog`
      (pokazują bufory, a mgła powstaje dopiero w przebiegu składającym)

Gęstość (`Density`):

- [ ] przesunąć w lewo do `0.000 /m`: obraz jest taki sam jak przy odznaczonym polu `Fog`,
      także na niebie
- [ ] przesuwać powoli w prawo: mgła gęstnieje **płynnie**, bez skoków i bez pasów
- [ ] przesunąć do `0.500 /m`: połowa koloru podłoża znika już po 1,4 m, labiryntu prawie
      nie widać, ściana pięć metrów dalej ma u podstawy prawie sam kolor mgły. Niebo
      wysoko nad głową zostaje czyste
- [ ] wpisać wartość spoza zakresu (Ctrl i kliknięcie, potem `2`): zostaje przycięta do
      `0.500 /m`. Ustawić z powrotem `0.100 /m`

Wysokość podstawy (`Base height`):

- [ ] przesunąć do `-2.00 m`: mgła przy podłożu rzednie (podłoże leży wtedy ponad dwa
      metry nad podstawą i dostaje mniej niż połowę gęstości), korytarz widać dalej
- [ ] przesunąć do `6.00 m`: całe ściany (mają 3 m) stoją w pełnej gęstości, więc wierzch
      dalekiej ściany jest tak samo zamglony jak jej podstawa, a pas zamglenia nad
      horyzontem jest wyższy
- [ ] ustawić z powrotem `0.50 m`

Zanik z wysokością (`Height falloff`):

- [ ] przesunąć do `0.00 /m`: mgła jest taka sama na każdej wysokości, **razem z niebem**.
      Całe niebo ma kolor mgły (punkt nieba jest 100 m od oka), gwiazdy znikają, a wierzch
      dalekiej ściany jest zamglony jak jej podstawa. Tarcza księżyca też wtapia się
      w mgłę, ale w jej miejscu **zostaje miękka poświata**: bloom jest liczony ze sceny
      bez mgły i dodawany po mgle
- [ ] przy `0.00 /m` odznaczyć `Bloom` w zakładce `Tone and bloom`: poświata księżyca
      znika i niebo jest jednolite. Zaznaczyć z powrotem
- [ ] przesunąć do `3.00 /m`: mgła jest warstwą grubości około metra. Dalekie podłoże
      nadal znika, ale ściany powyżej mniej więcej półtora metra są prawie czyste, a pas
      zamglenia nad horyzontem jest cieńszy
- [ ] ustawić z powrotem `0.40 /m`

Długi korytarz (ustawienia startowe):

- [ ] koniec korytarza wtapia się w kolor mgły: podłoże 10 m dalej ma około dwóch trzecich
      koloru mgły, 30 m dalej prawie sam kolor mgły
- [ ] na dalekich ścianach mgła słabnie ku górze: wierzch ściany jest wyraźniejszy niż jej
      podstawa (na 3 m gęstość to około jedna trzecia pełnej)
- [ ] iść powoli korytarzem: mgła cofa się płynnie, bez pasów i bez migotania. Zapisać,
      czy na ciemnych ścianach widać schodki koloru
- [ ] poświecić latarką w głąb korytarza: plama latarki na dalekiej ścianie jest przez
      mgłę bledsza, a sama mgła w snopie światła **nie świeci** (mgła zastępuje kolor
      powierzchni, nie jest oświetlana). Zapisać, czy to razi

Obrót w miejscu (mgła nie może pływać):

- [ ] odznaczyć `Vignette` i zgasić latarkę klawiszem F, bo oba efekty zmieniają jasność
      przy brzegach ekranu i udawałyby zmianę mgły
- [ ] stanąć kilka metrów przed ścianą i obracać kamerę w lewo i w prawo, tak żeby ten sam
      kawałek ściany przechodził ze środka ekranu do jego brzegu: mgła na tym kawałku
      **się nie zmienia**. Nie jaśnieje ani nie gęstnieje przy brzegu i nie pulsuje
- [ ] to samo w korytarzu, patrząc wzdłuż niego i obracając kamerę aż koniec korytarza
      dojdzie do brzegu ekranu, oraz przy ruchu kamery w górę i w dół: mgła na ścianach
      i na podłożu stoi w miejscu. To jest powód, dla którego shader liczy odległość od
      oka, a nie głębię wzdłuż osi widoku
      ([`../decisions/fog-distance-from-reconstructed-position.md`](../decisions/fog-distance-from-reconstructed-position.md))
- [ ] nadal bez winiety i bez latarki spojrzeć na niski pas nieba tuż nad horyzontem,
      stojąc na podłożu tam, gdzie horyzont widać (wzdłuż długiego korytarza albo przy
      wyjściu), i obracać kamerę **powoli**. Zapisać, czy zamglenie samego nieba jest przy
      bokach ekranu słabsze niż na środku i czy przy obrocie widać, jak się przesuwa. Ze
      wzoru wynika, że dla oka na wysokości stojącego gracza 3 stopnie nad horyzontem
      mgły jest około 53 % na środku i od 31 do 36 % przy bokach. Z wyższego punktu
      (w trybie noclip) wszystkie te wartości są mniejsze, ale środek nadal powinien być
      bardziej zamglony niż boki. Nikt tego nie oglądał
- [ ] zaznaczyć `Vignette` z powrotem i włączyć latarkę

Widok z góry w trybie noclip (znane ograniczenie):

- [ ] nacisnąć N: okno debug (Player / Position) pokazuje `Mode: noclip (free flight)`. Kliknąć w scenę
      (klawisze ruchu działają tylko przy przechwyconej myszy) i lecieć w górę klawiszem
      Spacja, aż środkowa liczba pola `Player feet` w oknie debug (Player / Position) pokaże około 30
      (można ją też wpisać: Ctrl i kliknięcie w środkowe pole)
- [ ] spojrzeć prosto w dół: labirynt jest **prawie zakryty mgłą**. Ze wzoru podłoże
      dostaje około 95 % mgły, wierzchy ścian około 65 %, więc z labiryntu powinien
      zostać blady rysunek wierzchów ścian. To jest znane ograniczenie (wysokość brana
      w pikselu, sekcja 19.1), nie błąd do naprawienia w tej części. Zapisać, jak to
      wygląda
- [ ] opadać klawiszem lewy Shift: labirynt wyłania się stopniowo, bez skoku
- [ ] nacisnąć N jeszcze raz: gracz wraca na podłoże, panel pokazuje `Mode: walking`

Niebo i księżyc:

- [ ] spojrzeć na księżyc i w gwiazdy: są wyraźne, tak jak przy odznaczonym polu `Fog`
      (porównać, klikając pole)
- [ ] spojrzeć na horyzont: niebo tuż nad nim jest zamglone, a przejście w czyste niebo
      wyżej jest płynne, bez widocznej krawędzi. Ze wzoru: prawie sama mgła na horyzoncie,
      około połowy 3 stopnie nad nim, poniżej 1 % od około 10 stopni
- [ ] obejrzeć krawędź ściany na tle nieba: zapisać, czy wzdłuż niej widać jasną albo
      ciemną obwódkę
- [ ] odznaczyć pole `Skybox` w oknie debug (Render / Scene): niebo zastępuje kolor tła, a pas zamglenia
      przy horyzoncie **zostaje** (wyczyszczony bufor głębi też ma wartość 1). Zaznaczyć
      z powrotem
- [ ] przesunąć `Far plane` w oknie debug (Player / View) ze `100.0 m` na około `20.0 m`: zapisać, jak
      zmienia się zamglenie nieba (punkt nieba leży na dalekiej płaszczyźnie, więc mgła
      na niebie od niej zależy). Wrócić na `100.0 m`

Kryształ przez mgłę:

- [ ] stanąć na końcu długiego korytarza, z kryształem na drugim końcu: bryła kryształu
      blednie w mgle, a jego poświata **zostaje** (bloom jest liczony ze sceny bez mgły
      i dodawany po mgle,
      [`../decisions/bloom-from-unfogged-scene.md`](../decisions/bloom-from-unfogged-scene.md))
- [ ] w zakładce `Tone and bloom` odznaczyć `Bloom`: poświata znika i z kryształu zostaje
      blada plama w mgle. Zaznaczyć z powrotem
- [ ] podejść do kryształu na krok: wygląda jak przed tą częścią, mgła go prawie nie
      dotyka

Kolor mgły (`Fog colour`):

- [ ] kliknąć kwadrat z kolorem i wybrać czystą czerwień (`255`, `0`, `0`): koniec
      korytarza i horyzont wtapiają się w czerwień, bliskie rzeczy zostają bez zmian
- [ ] porównać czerwień w oddali z kwadratem w panelu: na ekranie jest **ciemniejsza**
      (kolor jest mieszany przed ekspozycją i mapowaniem tonów, a krzywa ACES dociska
      ciemne i średnie tony)
- [ ] odznaczyć `Vignette` i `Bloom` (poświata jest dodawana po mgle i rozjaśniałaby ją),
      ustawić `Tone mapping` na `None (clamp)` przy `Exposure` `1.00` i przesunąć
      `Density` do `0.500 /m`: najgłębsza mgła na środku ekranu ma teraz **ten sam** kolor
      co kwadrat (przeliczenie z sRGB i kodowanie na sRGB się znoszą). Zapisać, czy tak
      jest. Przywrócić `ACES (fitted)`, `0.100 /m`, winietę i bloom
- [ ] przywrócić kolor startowy: wpisać w trzy pola `36`, `46` i `66` (Ctrl i kliknięcie
      w pole). To daje sRGB 0,141, 0,180 i 0,259, czyli kolor startowy z dokładnością do
      jednego poziomu z 255. Dokładnie `(0.14, 0.18, 0.26)` wraca po ponownym
      uruchomieniu gry

Ekspozycja i mapowanie tonów razem z mgłą:

- [ ] `Exposure` na `0.25`: mgła ciemnieje razem ze sceną. `Exposure` na `4.00`: jaśnieje
      razem z nią. Wrócić na `1.00`
- [ ] przełączyć `Tone mapping` na `None (clamp)` i na `Reinhard`: mgła jest widoczna
      w każdym trybie. Zapisać, jak zmienia się jej jasność. Zostawić `ACES (fitted)`
- [ ] przesunąć `Depth range`: zmienia się tylko obraz `Depth` w panelu, mgła w oknie
      **nie** (mgła nie korzysta z zakresu podglądu głębi)

Włącznik winiety:

- [ ] spojrzeć na coś równo oświetlonego, co wypełnia cały ekran (na przykład ściana
      z bliska przy liście `Lighting` ustawionej na `Unlit`). Odznaczyć `Vignette`: rogi
      lekko jaśnieją, środek ekranu się nie zmienia. Efekt jest z założenia subtelny: róg
      traci 30 %, środek krawędzi 7,5 %
- [ ] zaznaczyć z powrotem: rogi ciemnieją od razu. Wszystkie cztery tak samo
- [ ] panele i pasek HUD **nie są** przyciemniane w rogach: są rysowane po przebiegu
      składającym

Winieta przesadzona:

- [ ] `Strength` na `1.00`: same rogi są czarne, środek ekranu się nie zmienia, a środek
      każdej krawędzi traci około jednej czwartej światła
- [ ] `Strength` na `0.00`: obraz jest taki sam jak przy odznaczonym polu `Vignette`
- [ ] przy `Strength` `1.00` przesunąć `Radius` do `0.00`: ciemnienie zaczyna się od
      samego środka, jasny zostaje tylko środek ekranu, a przy środkach krawędzi zostaje
      około jednej piątej światła
- [ ] `Radius` na `0.65`: ciemne są tylko same rogi, wąskimi plamami, a środki krawędzi
      są nietknięte
- [ ] ustawić `Radius` na `0.40` i przy `Strength` `1.00` zrobić okno szerokie i niskie:
      jasny środek jest **elipsą w kształcie okna**, nie kołem. Potem okno wąskie
      i wysokie: elipsa jest wysoka. W obu wszystkie cztery rogi są jednakowo ciemne
      ([`../decisions/vignette-not-aspect-corrected.md`](../decisions/vignette-not-aspect-corrected.md))
- [ ] wpisać w `Radius` wartość spoza zakresu (Ctrl i kliknięcie, potem `0.9`): zostaje
      przycięta do `0.65`. Ustawić z powrotem `Strength` `0.30` i `Radius` `0.40`

Oba widoki diagnostyczne (okno debug (Render / Textures and normals), lista `View mode`):

- [ ] `Normals as colour`: obraz jest taki jak przed tą częścią, **bez mgły i bez
      winiety**: dalekie ściany mają tak samo czyste kolory jak bliskie, a rogi ekranu nie
      są ciemniejsze. Pola `Fog` i `Vignette` w oknie debug (Post process) są **nadal
      zaznaczone**
- [ ] `UVs as colour`: to samo
- [ ] w obu widokach ruch suwaków z zakładki `Fog and vignette` i zmiana `Fog colour` nie [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      zmieniają obrazu
- [ ] wrócić do `Textured`: mgła i winieta wracają z takimi ustawieniami, jakie zostawiłem

Podglądy:

- [ ] obraz `HDR colour` **nie ma** ani mgły, ani winiety: daleki koniec korytarza jest
      w nim wyraźniejszy niż w oknie, a rogi nie są ciemniejsze (podgląd pokazuje bufor
      sceny, a oba efekty powstają dopiero w przebiegu składającym)
- [ ] obrazy `Depth`, `Bright pass` i `Bloom` wyglądają tak jak w drugiej części

Zmiana rozmiaru okna:

- [ ] przeciągnąć krawędź okna i puścić: mgła leży na tych samych ścianach co przedtem,
      nie jest przesunięta ani przeskalowana względem sceny
- [ ] zmaksymalizować okno: mgła i winieta wyglądają tak samo jak w oknie 1280 x 720
      (żaden z efektów nie zależy od liczby pikseli)
- [ ] zrobić okno bardzo wąskie i bardzo niskie: gra działa, w konsoli nie ma linii
      `[error]`
- [ ] zminimalizować okno na kilka sekund i przywrócić: obraz z mgłą i winietą wraca od
      razu, bez linii `[error]`, także w buildzie Debug bez linii `GL_`

Przeładowanie shaderów (okno debug (Diagnostics / Frame and shaders / Shaders)):

- [ ] lista ma jedenaście linii (po tej części było ich nadal dziesięć, jedenasta doszła
      w czwartej), wszystkie `OK`. Mgła i winieta są w linii
      `composite.vert + composite.frag: OK`
- [ ] ustawić coś innego niż wartości startowe (na przykład `Density` `0.200 /m`
      i czerwony kolor mgły) i nacisnąć `Reload shaders`: wszystkie jedenaście linii
      zostaje `OK`, a obraz wygląda tak samo jak przed kliknięciem (uniformy są wysyłane w
      każdej klatce). Przywrócić ustawienia
- [ ] w pliku `assets/shaders/post/composite.frag` w funkcji `fogHeightFactor` zamienić
      linię `return exp(-uFogHeightFalloff * heightAboveBase);` na `return 1.0;`,
      skopiować assety (`cmake --build --preset debug --target copy_assets`)
      i przeładować: obraz jest taki sam jak przy `Height falloff` równym `0.00 /m`, czyli
      mgła zakrywa także niebo, a suwaki `Base height` i `Height falloff` przestają
      działać. Linia programu zostaje `OK` i nie powinno być linii `[error]`: oba uniformy
      przestają być aktywne, a ustawienie uniformu, którego program nie ma, OpenGL pomija
      bez błędu. Przywrócić linię, skopiować assety i przeładować
- [ ] w tym samym pliku przenieść cały blok `if (uBloomEnabled == 1) { ... }` nad blok
      `if (uFogEnabled == 1) { ... }`, skopiować assety i przeładować: poświata dalekiego
      kryształu blednie teraz w mgle razem z jego bryłą. Przywrócić kolejność, skopiować
      assety i przeładować
- [ ] zepsuć `assets/shaders/post/composite.frag` (na przykład usunąć średnik w funkcji
      `fogAmount`), skopiować assety i przeładować: linia `composite.vert +
      composite.frag` jest czerwona z nazwą pliku w komunikacie, a gra działa dalej ze
      starym programem, z mgłą i winietą. Naprawić, skopiować assety i przeładować

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza wywołanymi
      celowo
- [ ] liczba klatek na sekundę w Release przy ustawieniach startowych, z okna debug (Render / Scene),
      z zaznaczonymi polami `Fog` i `Vignette` i z oboma odznaczonymi, w oknie 1280 x 720
      i po zmaksymalizowaniu: zapisać cztery wartości. Zgłoszone liczby są w sekcji 19.1,
      zmierzone przy ukrytych panelach. Jeśli liczba stoi na częstotliwości odświeżania
      monitora, zapisać to
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 20. Lista kontrolna M7, część 4: cienie księżyca

Czwarta część kamienia milowego M7 dodaje pierwszy przebieg, który rysuje scenę z innego
miejsca niż kamera: mapę cieni księżyca. Na początku każdej klatki wszystko, co rzuca cień
(teren, ściany, słupki, brama, kryształy), jest rysowane z kierunku księżyca do tekstury
głębi 2048 x 2048, rzutem ortograficznym dopasowanym do całego terenu. Programy `lit`,
`gouraud` i `grass` porównują potem z tą teksturą głębię każdego fragmentu i odejmują od
światła **sam udział księżyca**. Doszły: pliki `src/scene/LightSpace.hpp` i `LightSpace.cpp`
(struktura `LightSpace`, funkcje `directionalLightSpace` i `shadowMapCoordinates`) oraz
`src/gfx/ComparisonSampler.hpp` i `ComparisonSampler.cpp` w bibliotece `engine`,
`src/game/Shadows.hpp` i `Shadows.cpp` (ustawienia `ShadowSettings` i matematyka bez OpenGL)
w bibliotece `game_logic`, `src/game/ShadowMap.hpp` i `ShadowMap.cpp` oraz panel
`src/debug/panels/ShadowsPanel.hpp` i `ShadowsPanel.cpp` w programie, shadery
`shadow_depth.vert`, `shadow_depth.frag` i `common/shadows.glsl` oraz plik testów
`tests/ShadowTests.cpp`. Zmieniły się: `NightMazeApp` (funkcje `drawMoonShadowMap` i
`drawShadowCasters`, przebieg cieni jako pierwszy w `onRender`), `Lighting.hpp` i
`Lighting.cpp` (funkcja `moonDirection`, `moonIntensity` 0,2 zamiast 0,12),
`ShaderUniforms.hpp` (struktura `ShadowUniformNames`, stałe `MOON_SHADOW_UNIFORMS` i
`MOON_SHADOW_TEXTURE_UNIT`), `PostProcess.hpp` (`AttachmentPreview::RawDepth`),
`common/lighting.glsl` (pola `moonDiffuse` i `moonSpecular`, funkcja `moonFacing`),
`lit.frag`, `gouraud.vert`, `gouraud.frag`, `grass.frag`, `post/preview.frag` (tryb 2),
`DebugContext.hpp` (cztery nowe pola, razem trzydzieści cztery), `DebugUI.cpp`
(`SHADER_COUNT` 11), `PanelLayout.hpp` (`SHADOWS_PLACEMENT`, `FOLDED_ROW_COUNT` 4 zamiast
3), `main.cpp` i `CMakeLists.txt`. **Doszły** jeden program shaderów (jest ich jedenaście) i
jeden panel (jest ich dwanaście, sześć startuje zwiniętych). Blok uniformów `LightBlock` się
nie zmienił. Opis kodu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md)
(cały temat: dwa przebiegi, pudełko światła, rozdzielczość, porównanie, filtr sprzętowy,
PCF, acne, bias, peter panning, Gouraud, trawa, podgląd, panel),
[`../modules/gfx/comparison-sampler.md`](../modules/gfx/comparison-sampler.md),
[`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md) (ścieżka bez koloru,
użyta pierwszy raz), [`../modules/debug-ui.md`](../modules/debug-ui.md) (panel Shadows).
Decyzje:
[`../decisions/shadow-box-fitted-to-terrain.md`](../decisions/shadow-box-fitted-to-terrain.md),
[`../decisions/shadow-matrix-as-plain-uniforms.md`](../decisions/shadow-matrix-as-plain-uniforms.md),
[`../decisions/shadow-bias-in-metres-in-shader.md`](../decisions/shadow-bias-in-metres-in-shader.md),
[`../decisions/gouraud-shadow-test-per-fragment.md`](../decisions/gouraud-shadow-test-per-fragment.md),
[`../decisions/grass-casts-no-shadow.md`](../decisions/grass-casts-no-shadow.md) i
[`../decisions/shadow-takes-only-moon-light.md`](../decisions/shadow-takes-only-moon-light.md).

Kamień milowy M7 jest nadal **rozpoczęty i nie jest kompletny**: w kodzie są cztery części z
sześciu. Temat 11 wykładu (shadow mapping) jest **w toku**: są cienie księżyca z PCF i
biasem. **Nie są zbudowane:** cienie latarki (część piąta) i minimapa (część szósta, temat
10). Część ręczna poniżej (20.2) jest otwarta w całości, tak samo jak części ręczne 17.2,
18.2 i 19.2, na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)) i nie ma
tagu. Stan całego M7 w jednym miejscu: [`m7-status.md`](m7-status.md).

Zdania o cieniach latarki i minimapie w tej sekcji opisują stan po części czwartej. Cień
latarki doszedł 2026-10-06: sekcja 21.

### 20.1. Zgłoszone (2026-10-05)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano, więc ich tu nie podaję (środowisko wcześniejszych pomiarów jest w sekcji 1).
**Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej sekcji**: wszystkie są
zgłoszone z dnia, w którym powstał kod. Sam policzyłem makra `TEST_CASE` w plikach testów
(310 w dwudziestu pięciu plikach, w tym 16 w `ShadowTests.cpp`) i asercje nowego pliku: 129
nie zależy od labiryntu, a jeden przypadek sprawdza po 5 warunków dla każdego z 242 pudełek
kolizji labiryntu startowego, razem 1339. Liczby w tabeli wartości startowych i zakresy
suwaków przepisałem z kodu. Rozmiary pudełka światła i tekseli policzyłem ze wzorów z kodu:
nie są odczytane z ekranu.

Bramka i testy:

- [x] `make check` przechodzi: formatowanie (clang-format), buildy Debug i Release, testy w
      obu, clang-tidy
- [x] `night_maze_tests.exe`: 310 przypadków testowych i 103751 asercji, wszystkie
      przechodzą. Względem trzeciej części M7 (294 przypadki i 102412 asercji) doszedł jeden
      plik, `ShadowTests.cpp` (16 przypadków i 1339 asercji): `294 + 16 = 310` i `102412 +
      1339 = 103751`

Porównanie obrazu z trzecią częścią M7:

- [x] z odznaczonym polem `Shadows` i suwakiem `Moon intensity` cofniętym do 0,12 obraz jest
      identyczny co do piksela z obrazem trzeciej części **poza paskiem HUD**, który stoi o
      jeden rząd pasków tytułowych niżej. W trybie `Phong` różnica wynosi najwyżej 1/255 w
      kanale
- [x] build Debug nie zapisał żadnej linii z błędem OpenGL (`GL_...`) przy mapie 2048 x 2048
      i przy 1024 x 1024

Zachowanie biasu, obserwacja w grze:

- [x] acne: z samymi suwakami `Constant bias` i `Slope bias` na 0 obraz wygląda na **ogólnie
      ciemniejszy**. Wyraźne prążki widać dopiero po odznaczeniu także `PCF` i `Hardware 2 x
      2 filter`
- [x] peter panning: przy biasie 0,5 m widać światło przeciekające na ścianach

Koszt, Release, panele ukryte. Pomiar **zaszumiony**: klatka trwa poniżej 1 ms.

| Okno | `Shadows` odznaczone | mapa 2048 | mapa 1024 |
|---|---|---|---|
| 1280 x 720 | około 1250 klatek na sekundę | około 1000 | około 1200 |
| 2560 x 1440 | około 630 | około 560 | około 690 |

Tych liczb **nie wolno zestawiać** z sekcją 19.1 (około 1880 i 1145 klatek na sekundę z mgłą
i winietą): to inna sesja pomiarowa. Zestawienie pokazałoby spadek o jedną trzecią albo
więcej przy **wyłączonych** cieniach, a z kodu taki koszt nie wynika: przy wyłączonych
cieniach dochodzi co klatkę policzenie pudełka światła, siedem uniformów na program i jedna
gałąź w shaderach. Skąd różnica, **nie jest wyjaśnione**. Sam pomiar też pokazuje swój szum:
690 klatek z mapą 1024 to więcej niż 630 bez cieni. Potrzebny jest czysty pomiar w jednej
sesji (ostatni punkt listy 20.2).

Wartości startowe (wszystkie nowe poza księżycem). Kontrolki stoją w nowym panelu Shadows, w
zakładce `Moon`. W nawiasie jest napis, jaki pokazuje kontrolka:

| Ustawienie | Wartość |
|---|---|
| `Shadows` | zaznaczone |
| `Resolution` | `2048 x 2048`, druga pozycja listy to `1024 x 1024` |
| `Constant bias` | 0,02 m (`0.020 m`), suwak od 0 do 0,5 |
| `Slope bias` | 0,12 m (`0.120 m`), suwak od 0 do 1 |
| `Hardware 2 x 2 filter` | zaznaczone |
| `PCF` | zaznaczone |
| `Kernel` | `3 x 3`, dalsze pozycje `5 x 5` i `7 x 7` |
| `Strength` | 1,0 (`1.00`), suwak od 0 do 1 |
| `Moon intensity` (panel Lights) | 0,2 (było 0,12) |
| `MOON_SHADOW_TEXTURE_UNIT` (stała) | 3: jednostka teksturująca mapy cieni w przebiegu sceny |
| `LIGHT_BOX_MARGIN` (stała) | 0,5 m wolnego miejsca wokół pudełka światła |
| `FOLDED_ROW_COUNT` (stała) | 4 (było 3) |
| `SHADER_COUNT` (stała) | 11 (było 10) |
| `SHADOWS_HEIGHT` (stała) | 324: wysokość rozwiniętego panelu Shadows |

Co te wartości znaczą w liczbach dla labiryntu startowego (policzone ze wzorów, nie
zmierzone na ekranie): pudełko światła ma 64,8 na 54,1 m i 47,0 m głębi, teksel mapy 2048 ma
3,2 cm, a mapy 1024 6,3 cm (pierwszą liczbę przypina test). Bias poziomego gruntu to 4,8 cm,
ściany muskanej przez światło do 14 cm. Poziomy grunt w świetle księżyca jest około 4,6 raza
jaśniejszy niż w cieniu.

Znane ograniczenia tej części, zapisane także w dokumencie modułu (sekcja 2.19):

- cień rzuca tylko księżyc. Latarka i światła kryształów nadal świecą przez ściany
- jedna mapa bez kaskad, pudełko stałe względem terenu: około połowy mapy pokrywa wzgórza
  poza labiryntem
- bias nie zależy od rozmiaru teksela ani od jądra PCF. Z rachunku wynika, że jądra `5 x 5`
  i `7 x 7` oraz mapa 1024 mogą lekko przyciemniać oświetlony grunt. Tego nikt nie oglądał:
  jest punktem listy 20.2
- trawa przyjmuje cień, ale go nie rzuca
- namalowana tarcza księżyca nie idzie za suwakami `Moon yaw` i `Moon pitch`, a cienie idą

Czego dla tej części nie zapisano i czego dlatego tu nie twierdzę: wersji kompilatora, karty
i sterownika, listy zrzutów ekranu, tego, w jakich widokach porównywano obraz z trzecią
częścią, rozrzutu liczby klatek między uruchomieniami, kosztu samego przebiegu głębi i
samego odczytu osobno, zużycia pamięci karty ani tego, czy sześć pasków tytułowych i HUD
mieszczą się bez nakładania w oknie innym niż 1280 x 720.

Żadnej kontrolki panelu Shadows nikt nie kliknął myszą, nikt nie przełączył rozdzielczości w
działającej grze i nikt nie użył przycisku `Reload shaders` po tej zmianie. Na macOS nic z
tej części nie było budowane ani uruchamiane.

**Widziane na zrzutach ekranu przez agenta (2026-10-06).** Środowisko i metoda tego oglądania (jedna sesja agenta, 2026-10-06): wersja Release z commitu `9a33f18`, okno 1280 x 720, karta RTX 4070 Ti SUPER, sterownik zgłaszający OpenGL 4.1.0 NVIDIA 610.74. Ustawienia były zmieniane **tymczasowym hakiem testowym**, który pisał te same pola, które edytują panele, więc punkty typu "odznacz X w panelu" są potwierdzone co do **efektu**, nie co do widżetu. Prawdziwe, syntetyczne wejście posłużyło do obrotu myszą, klawiszy W, Shift, M, F, tyldy i kliknięć w panele.

Poniższe punkty to "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela". **Nie zamykają żadnego punktu listy właściciela** (sekcja 20.2).

- widziane na zrzucie ekranu przez agenta: w konsoli żadnej linii `[error]`
- widziane na zrzucie ekranu przez agenta: zakładka `Moon` panelu Shadows: `2048 x 2048`, `64.8 x 54.1 m, 47.0 m deep`, `3.2 cm`
- widziane na zrzucie ekranu przez agenta: podgląd mapy cieni jest szary z ciemnymi liniami labiryntu
- widziane na zrzucie ekranu przez agenta: wyłączone `Shadows` rozjaśnia ziemię, która była w cieniu
- widziane na zrzucie ekranu przez agenta: przy wartościach startowych krawędź cienia jest miękka, a przy obu filtrach wyłączonych ma schodki
- widziane na zrzucie ekranu przez agenta: Blinn-Phong, Phong i Gouraud pokazują cienie w tych samych miejscach, a `Unlit` i oba widoki diagnostyczne bez cieni
- widziane na zrzucie ekranu przez agenta: patrząc z góry, wszystkie cienie padają w tę samą stronę
- widziane na zrzucie ekranu przez agenta: bez shadow acne (prążków na oświetlonym gruncie)

**Nie widziane na ekranie:** przeciąganie `Moon yaw` i `Moon pitch` (sweepy), suwaki przeciągane i wpisywane w panelu, `Reload shaders`, zmiana rozmiaru okna.

### 20.2. Otwarte: test ręczny na około dwadzieścia pięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Punkty, które agent widział na zrzutach (lista "Widziane na zrzutach ekranu przez agenta" w sekcji 20.1), zostają `[ ]`: oglądanie przez agenta nie zastępuje testu właściciela.

Tych kroków nikt jeszcze nie wykonał ręką. Przy każdym jest to, co zrobić, i to, co powinno
być widać. Oczekiwania wynikają z kodu, z testów jednostkowych i ze wzorów, nie z klikania
ani ze zrzutów ekranu. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/ShadowsPanel.cpp`, `LightsPanel.cpp`, `RendererPanel.cpp`,
`AssetsPanel.cpp` i `TerrainPanel.cpp`. Ustawień cieni program nigdzie nie zapisuje, więc
ponowne uruchomienie gry zawsze przywraca wartości startowe.

Listy 17.2, 18.2 i 19.2 też są otwarte.

Przygotowanie:

- [ ] usunąć stary plik `imgui.ini` z katalogu programu i uruchomić grę z terminala.
      Oczekiwane w konsoli: żadnej linii `[error]`, w szczególności żadnej o shaderach
      `shadow_depth.vert`, `shadow_depth.frag`, `lit.frag`, `gouraud.frag` i `grass.frag`
      ani o niekompletnym framebufferze
- [ ] otworzyć panele klawiszem akcentu. U góry między kolumnami są cztery rzędy pasków [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      tytułowych: Camera i Gameplay, Terrain i Grass, Framebuffers, a pod nim nowy pasek
      Shadows tej samej szerokości. HUD stoi pod czwartym rzędem (stan z tej części; dziś pod piątym, przy widocznych panelach) i nie nachodzi na żaden pasek
- [ ] rozwinąć okno debug (Light / Shadows): jedna zakładka `Moon`, po lewej osiem kontrolek, kreska i trzy [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      linie faktów, po prawej podpis `Depth seen from the moon` i kwadratowy obraz
- [ ] linie faktów przy wartościach startowych: `Map: 2048 x 2048, GL_DEPTH_COMPONENT24`,
      `Covers 64.8 x 54.1 m, 47.0 m deep` i `One texel: 3.2 cm`. Te liczby są policzone, nie
      odczytane: zapisać, jeśli panel pokazuje co innego

Przełącznik:

- [ ] stanąć w korytarzu, w którym widać granicę światła i cienia na ziemi (latarka zgaszona
      klawiszem F). Odznaczyć `Shadows`: zacienione miejsca jaśnieją, granica znika. Linia
      faktów zmienia się na `Map: not drawn`, a w miejscu obrazu stoi `(not drawn)`.
      Zaznaczyć z powrotem: cienie wracają w tych samych miejscach
- [ ] idąc i obracając kamerę, patrzeć na krawędź cienia na ziemi: stoi w miejscu i nie
      migocze

Rozdzielczość:

- [ ] lista `Resolution`, wybrać `1024 x 1024`: krawędzie cieni są grubsze, linie faktów
      pokazują `Map: 1024 x 1024, ...` i `One texel: 6.3 cm`, linia `Covers` się nie
      zmienia. W konsoli żadnej linii `[error]`
- [ ] przełączyć kilka razy między dwiema pozycjami: za każdym razem obraz i fakty zmieniają
      się od razu, cienie nie znikają na dłużej niż jedną klatkę
- [ ] wrócić do `2048 x 2048`

Bias:

- [ ] odznaczyć `PCF` i `Hardware 2 x 2 filter`, ustawić `Constant bias` i `Slope bias` na
      0: na oświetlonych powierzchniach widać prążki (acne). Zapisać, w którą stronę biegną
      na ziemi względem kierunku księżyca
- [ ] nadal przy biasie 0 zaznaczyć oba filtry: prążki zamieniają się w równe przyciemnienie
      oświetlonych powierzchni
- [ ] przesuwać sam `Constant bias` w górę od 0: przyciemnienie znika. Zapisać, przy jakiej
      wartości grunt przestaje się zmieniać
- [ ] ustawić `Constant bias` na 0,02 i przesuwać `Slope bias` od 0 do 0,12: zmiana powinna
      być widoczna głównie na ścianach, które księżyc oświetla pod ostrym kątem, i na
      zboczach
- [ ] `Constant bias` na koniec zakresu (`0.500 m`): przy podstawie ścian po stronie cienia
      pojawia się światło, cień odkleja się od ściany (peter panning). Zapisać, przy których
      ścianach to widać: z rachunku wynika, że przy ścianach o normalnej wzdłuż osi Z (droga
      promienia przez ścianę 0,34 m), a przy ścianach o normalnej wzdłuż X jeszcze nie (0,74
      m)
- [ ] `Slope bias` na koniec zakresu (`1.000 m`) przy `Constant bias` 0,02: zapisać, co się
      zmienia
- [ ] wpisać w suwak z klawiatury (Ctrl i kliknięcie) wartość spoza zakresu, na przykład 5:
      zostaje obcięta do końca zakresu
- [ ] przywrócić `0.020 m` i `0.120 m`

Filtr sprzętowy i PCF:

- [ ] przy wartościach startowych podejść blisko do krawędzi cienia na ziemi: przejście jest
      miękkie, szerokości kilku centymetrów
- [ ] odznaczyć `PCF`: krawędź jest wyraźnie węższa
- [ ] odznaczyć także `Hardware 2 x 2 filter`: krawędź ma schodki wielkości teksela mapy
- [ ] zaznaczyć `Hardware 2 x 2 filter` przy odznaczonym `PCF`: schodki są wygładzone
- [ ] zaznaczyć `PCF` i wybrać w liście `Kernel` kolejno `3 x 3`, `5 x 5` i `7 x 7`:
      przejście jest coraz szersze
- [ ] **bias a jądro (otwarta obserwacja):** stanąć na otwartym, oświetlonym gruncie. Przy
      jądrze `7 x 7` odznaczać i zaznaczać `Shadows`: zapisać, czy oświetlony grunt
      ciemnieje po zaznaczeniu. Powtórzyć przy `Resolution` `1024 x 1024` z jądrem `3 x 3` i
      z jądrem `7 x 7`. Z rachunku w dokumencie modułu (sekcja 2.11) wynika, że może lekko
      ciemnieć. Jeśli ciemnieje, zapisać, przy jakim `Constant bias` przestaje
- [ ] wrócić do `2048 x 2048`, `3 x 3`, oba filtry zaznaczone

Siła:

- [ ] suwak `Strength` na 0: cieni nie widać, obraz jak przy odznaczonym `Shadows`. Na 0,5:
      cienie o połowę słabsze. Na 1: jak na starcie

Księżyc (okno debug (Light / Lights), grupa `Moon (directional)`), przy rozwiniętym okna debug (Light / Shadows):

- [ ] przesuwać `Moon yaw` przez cały zakres: cienie obracają się płynnie wokół ścian, obraz
      mapy w oknie debug (Light / Shadows) też się zmienia, linia `Covers` pokazuje inne rozmiary. Tarcza
      księżyca na niebie zostaje na miejscu (znane ograniczenie)
- [ ] `Moon pitch` na -90: cienie chowają się pod ściany, obraz mapy to widok prosto z góry,
      linia `Covers` pokazuje około `49.0 x 49.0 m, 7.5 m deep`. W konsoli żadnej linii
      `[error]`, cienie nie znikają
- [ ] przesuwać `Moon pitch` powoli między -90 a -85: w okolicy -87,4 obraz mapy w panelu
      obraca się skokiem (zmiana wektora "w górę" światła). Cienie w scenie nie powinny przy
      tym skoczyć
- [ ] `Moon pitch` na -5: cienie są bardzo długie, linia `Covers` pokazuje około `64.8 x
      13.1 m, 65.1 m deep`. Zapisać, jak wyglądają krawędzie cieni i czy widać acne
- [ ] `Moon intensity` na 0,12 i z powrotem na 0,2: przy 0,2 różnica między światłem a
      cieniem jest wyraźniejsza
- [ ] przywrócić `Moon yaw` 25 i `Moon pitch` -50

Podgląd:

- [ ] obraz w oknie debug (Light / Shadows): jasne tło, ciemniejsze plamy wzgórz i ciemne linie ścian
      labiryntu. Jest szary, nie czerwony
- [ ] zwinąć okno debug (Light / Shadows) i rozwinąć: przez pierwszą klatkę w miejscu obrazu może stać `(no [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      picture yet)`, potem obraz wraca
- [ ] najechać na obraz: podpowiedź zaczyna się od `The shadow map: black is near the moon`
- [ ] patrząc na obraz, przesunąć w oknie debug (Light / Lights) suwak `Moon yaw`: obraz zmienia się na
      żywo, bo mapa jest rysowana co klatkę

Tryby cieniowania (okno debug (Render / Scene), lista `Lighting`):

- [ ] `Blinn-Phong` (startowy) i `Phong`: cienie w tych samych miejscach
- [ ] `Gouraud`: cienie w tych samych miejscach i z **tak samo ostrymi** krawędziami.
      Światło jest kanciaste jak zawsze w tym trybie
- [ ] `Unlit`: cieni nie ma, scena jest równo jasna jak dotąd
- [ ] okno debug (Render / Textures and normals), lista `View mode`: `Normals as colour` i `UVs as colour` nie pokazują
      cieni. Wrócić do `Textured` i `Blinn-Phong`

Tylko księżyc:

- [ ] stanąć w cieniu ściany i zapalić latarkę (F): plama latarki jest w cieniu tak samo
      jasna jak poza nim
- [ ] znaleźć kryształ stojący w cieniu ściany: świeci, ma poświatę i oświetla ziemię wokół
      siebie
- [ ] kryształ stojący w świetle księżyca rzuca na ziemię mały cień, a ściany rzucają cień
      na kryształy
- [ ] trawa w cieniu ściany jest ciemna jak ziemia pod nią. Kępka na granicy cienia jest
      częściowo jasna. Trawa na otwartym gruncie nie ma pod sobą cienia

Teren i widok z góry:

- [ ] okno debug (World / Terrain and grass / Terrain), pole `Wireframe`: teren jest rysowany liniami, a cienie na nim i cień
      wzgórz zostają pełne
- [ ] suwak `Height scale` w górę: wzgórza rosną, ich cienie się wydłużają, linia `Covers`
      rośnie. Wrócić do 1
- [ ] klawisz N (noclip), wznieść się nad labirynt i spojrzeć w dół. Odznaczyć `Fog` w
      okna debug (Post process), bo z góry mgła zakrywa labirynt (znane ograniczenie z sekcji
      19.1). Cienie wszystkich ścian padają w tę samą stronę, nie urywają się na brzegu
      labiryntu ani na brzegu terenu, a na wzgórzach nie ma obcych smug
- [ ] okno debug (World / Maze), przycisk `Regenerate` z innym rozmiarem labiryntu: cienie pasują do nowych
      ścian od pierwszej klatki, linia `Covers` zmienia się razem z terenem

Przeładowanie shaderów (okno debug (Diagnostics / Frame and shaders / Shaders)):

- [ ] `Reload shaders` przy wartościach startowych: jedenaście linii, wszystkie `OK`,
      ostatnia to `shadow_depth.vert + shadow_depth.frag`. Cienie zostają
- [ ] odznaczyć `Shadows` i kliknąć `Reload shaders`: scena nadal się rysuje (uniformy cieni
      są ustawiane także przy wyłączonych cieniach). W konsoli builda Debug żadnej linii
      `GL_INVALID_OPERATION`
- [ ] zepsuć `assets/shaders/common/shadows.glsl` (na przykład usunąć średnik), skopiować
      assety i przeładować: linie programów `lit`, `gouraud` i `grass` są czerwone z nazwą
      pliku w komunikacie, a gra działa dalej ze starymi programami. Naprawić, skopiować
      assety i przeładować

Na koniec:

- [ ] przez cały test w konsoli nie pojawia się żadna linia `[error]` poza wywołanymi celowo
- [ ] **czysty pomiar kosztu.** W jednej sesji, w buildzie Release, z ukrytymi panelami, w
      oknie 1280 x 720 i po zmaksymalizowaniu zapisać liczbę klatek na sekundę dla: commita
      trzeciej części M7 (`e8e1822`), tej części z odznaczonym `Shadows`, z mapą 2048 i z
      mapą 1024. Każdą wartość zapisać dwa razy, żeby widzieć rozrzut. Pytanie, na które
      pomiar ma odpowiedzieć: ile kosztuje ta część przy **wyłączonych** cieniach względem
      trzeciej części (sekcja 20.1 tego nie rozstrzyga). Jeśli liczba stoi na częstotliwości
      odświeżania monitora, zapisać to
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 21. Lista kontrolna M7, część 5: cień latarki

Piąta część kamienia milowego M7 daje latarce własną mapę cieni i przenosi jej światło z oka
do ręki. Na początku klatki, zaraz po przebiegu cieni księżyca, wszystko, co rzuca cień,
jest rysowane drugi raz: z ręki gracza, wzdłuż osi stożka latarki, **rzutem
perspektywicznym** o kącie rozwarcia 46 stopni (dwa razy 21 stopni zewnętrznego stożka plus
2 stopnie zapasu), z płaszczyzną bliską 0,05 m i daleką równą zasięgowi latarki (16 m), do
drugiej tekstury głębi, domyślnie 1024 x 1024, związanej z jednostką teksturującą 4. Programy
`lit`, `gouraud` i `grass` odejmują potem od światła **sam udział latarki**, osobno od
udziału księżyca. Latarka stoi w ręce: 0,20 m w prawo i 0,25 m w dół od oka, a promień celuje
w punkt na osi widzenia 4 m przed okiem. Doszły: w `src/scene/LightSpace.*` funkcja
`spotLightSpace` i wyliczenie `LightProjection` (pola `kind`, `position`, `nearPlane`,
`farPlane` struktury `LightSpace`), w `src/game/Lighting.*` struktura `FlashlightPose`,
funkcja `flashlightPose`, trzy pola ustawień (`flashlightHandRight`, `flashlightHandDown`,
`flashlightConvergeDistance`) i stałe `MAX_FLASHLIGHT_HAND_RIGHT` i
`MIN_FLASHLIGHT_CONVERGE_DISTANCE`, w `src/game/Shadows.*` funkcje `flashlightShadowDefaults`,
`biasForShader` i `shadowTexelSizeAt`, w `src/game/ShadowMap.*` nowy argument
`drawPreview(previewShader, lightSpace)` i pole `lightPosition` w `ShadowUniformNames`,
w `ShaderUniforms.hpp` stałe `FLASHLIGHT_SHADOW_UNIFORMS` i `FLASHLIGHT_SHADOW_TEXTURE_UNIT`,
w `NightMazeApp` funkcje `drawFlashlightShadowMap` i `setShadowUniformsOf` oraz pozycja
latarki liczona **przed** przebiegami cieni, w `common/shadows.glsl` funkcja
`flashlightShadow` i uniformy `uFlashlightShadow*`, w `common/lighting.glsl` pola
`flashlightDiffuse` i `flashlightSpecular` oraz funkcja `flashlightFacing`, w `gouraud.vert` i
`gouraud.frag` trzy nowe zmienne przekazywane, w `post/preview.frag` użycie trybu głębi
perspektywicznej także dla mapy latarki, w `DebugContext.hpp` cztery nowe pola (razem
trzydzieści osiem), w `ShadowsPanel.*` struktura `ShadowMapView` i druga zakładka
`Flashlight`, w `LightsPanel.cpp` trzy suwaki. **Nie doszedł** żaden program shaderów (jest
ich nadal jedenaście) ani panel (jest ich nadal dwanaście). Blok uniformów `LightBlock` się
nie zmienił. Opis kodu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md)
(sekcja 2.20 i jej podsekcje: perspektywiczna przestrzeń światła, nieliniowa głębia, bias w
metrach w przestrzeni świata, punkt za światłem, podgląd z linearyzacją, teksel na metr),
[`../modules/game/flashlight.md`](../modules/game/flashlight.md) (ręka, `flashlightPose`),
[`../modules/debug-ui.md`](../modules/debug-ui.md) (zakładka `Flashlight`, trzy suwaki).
Decyzje:
[`../decisions/flashlight-in-hand.md`](../decisions/flashlight-in-hand.md),
[`../decisions/flashlight-shadow-bias-in-world-space.md`](../decisions/flashlight-shadow-bias-in-world-space.md)
i
[`../decisions/flashlight-hand-straight-down.md`](../decisions/flashlight-hand-straight-down.md).

(Stan po części piątej, 2026-10-06. Od szóstej części minimapa jest w kodzie, sekcja 22.)
Kamień milowy M7 jest nadal **rozpoczęty i nie jest kompletny**: w kodzie jest pięć części z
sześciu. Temat 11 wykładu (shadow mapping) ma w kodzie obie mapy, ortograficzną i
perspektywiczną, ale **nie jest zaliczony**: części ręczne 20.2 i 21.2 są otwarte, a na
macOS kod nie był budowany ([`build-macos.md`](build-macos.md)). **Nie jest zbudowana**
minimapa (część szósta, temat 10). Nie ma tagu. Stan całego M7 w jednym miejscu:
[`m7-status.md`](m7-status.md).

### 21.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano w tym dokumencie. Z uruchomienia programu zgłoszono tylko linię `GL_VERSION`
(OpenGL 4.1.0, NVIDIA). **Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej
sekcji.** Liczby w tabelach poniżej przepisałem z kodu albo policzyłem z jego stałych: nie są odczytane z ekranu, a **w chwili tego zgłoszenia nikt nie obejrzał obrazu tej części** (agent obejrzał go później: lista "Widziane na zrzutach ekranu przez agenta" przed sekcją 21.2).

Bramka i start:

- [x] `make check` przechodzi (zgłoszone)
- [x] `night_maze_tests.exe`: 329 przypadków testowych i 104306 asercji, wszystkie
      przechodzą (zgłoszone). Względem części czwartej (310 i 103751) doszło 19 przypadków i
      555 asercji. Przypadki policzyłem z plików testów: 15 nowych w `ShadowTests.cpp` (z 16
      na 31) i 4 nowe w `LightingTests.cpp` (z 11 na 15, a jeden dotychczasowy przypadek
      zmienił nazwę na `the flashlight sits in the hand and is aimed at a point in front of
      the eye`): `310 + 15 + 4 = 329`. Podziału 555 asercji między pliki nie liczyłem
- [x] start programu Debug przez około 7 sekund (zgłoszone): `GL_VERSION` 4.1.0 NVIDIA,
      assety wczytane, standardowe wyjście błędów puste, panele ukryte

Czego ten start **nie** obejmował (zgłoszone wprost):

- rysowania w trybie `Gouraud`
- zakładki `Flashlight` panelu Shadows i jej podglądu
- ścieżki z wyłączoną latarką (klawisz F)
- przycisku `Reload shaders`

Nikt nie kliknął żadnej z nowych kontrolek (trzy suwaki w panelu Lights, zakładka
`Flashlight`) i nikt nie obejrzał ani jednego cienia latarki. Liczby klatek dla tej części
nie zostały zmierzone.

Wartości startowe nowych ustawień. Suwaki `Hand right`, `Hand down` i `Converge at` stoją w
panelu Lights, w grupie `Flashlight (spot)`, a zakładka `Flashlight` w panelu Shadows. W
nawiasie jest napis, jaki pokazuje kontrolka:

| Ustawienie | Wartość |
|---|---|
| `Hand right` | 0,20 m (`0.20 m`), suwak od 0 do 0,25 (`MAX_FLASHLIGHT_HAND_RIGHT`) |
| `Hand down` | 0,25 m (`0.25 m`), suwak od 0 do 0,5 |
| `Converge at` | 4,0 m (`4.0 m`), suwak od 0,5 (`MIN_FLASHLIGHT_CONVERGE_DISTANCE`) do 20 |
| `Beam range` (istniejący) | 16 m, jest też płaszczyzną daleką mapy cieni latarki |
| `Shadows` (zakładka `Flashlight`) | zaznaczone |
| `Resolution` (zakładka `Flashlight`) | `1024 x 1024` (księżyc: `2048 x 2048`) |
| `Constant bias`, `Slope bias` (zakładka `Flashlight`) | 0,010 m i 0,130 m (księżyc: 0,020 i 0,120) |
| `Hardware 2 x 2 filter`, `PCF`, `Kernel`, `Strength` | jak u księżyca: zaznaczone, zaznaczone, `3 x 3`, 1,00 |
| `FLASHLIGHT_SHADOW_TEXTURE_UNIT` (stała) | 4 |
| `SPOT_NEAR_PLANE` (stała) | 0,05 m |
| `SPOT_CONE_MARGIN_DEGREES` (stała) | 2 stopnie |

Co te wartości znaczą w liczbach (policzone ze wzorów i komentarzy kodu, **nie zmierzone na
ekranie**; część jest przypięta testami `ShadowTests.cpp` i `LightingTests.cpp`):

- ręka stoi 1,45 m nad gruntem (oko 1,7 m minus 0,25 m) i 0,2 m od środka ciała, czyli 0,1 m
  wewnątrz pudła kolizji o szerokości 0,6 m. Największy suwak (0,25 m) zostawia 0,05 m,
  czyli dokładnie płaszczyznę bliską mapy
- plama latarki na ścianie 4 m przed graczem leży w środku ekranu. Na ścianie 1 m przed nim
  około 0,15 m w prawo i 0,19 m niżej od osi widzenia, a na ścianie 8 m przed nim około
  0,2 m w lewo i 0,25 m wyżej (poziomy wzrok, rachunek z podobieństwa trójkątów)
- mapa pokrywa 13,6 x 13,6 m na płaszczyźnie dalekiej (16 m). Teksel ma 0,83 mm na każdy
  metr odległości: 3,3 mm na ścianie 4 m dalej i 13,3 mm na końcu zasięgu, wobec 32 mm u
  księżyca. Panel powinien pokazać `Covers 13.6 x 13.6 m at 16.0 m` i `One texel: 0.08 cm
  per metre away`
- bias startowy na gruncie 10 m przed graczem to 12,1 cm przy potrzebnych 11,6 cm
  (rachunek w komentarzu do `FLASHLIGHT_SHADOW_SLOPE_BIAS`, sprawdza go test `the default
  bias of the flashlight covers the ground up to 10 m ahead`). Dalej bias jest za mały, ale
  prawie tam nie dociera światło
- mapa 1024 x 1024 z głębią 24-bitową to około 3 do 4 MB

Znane ograniczenia tej części (szczegóły w dokumencie modułu, sekcja 2.20):

- cień latarki zabiera tylko udział latarki: światło otoczenia, księżyc, kryształy i
  świecenie własne zostają. Światła kryształów nadal świecą przez ściany
- trawa przyjmuje cień latarki, ale go nie rzuca (jak u księżyca)
- test cienia latarki wykonuje się dla każdego fragmentu, także poza stożkiem: kod nie ma
  wcześniejszego wyjścia dla fragmentu, do którego latarka nie dociera (poza przypadkiem,
  gdy cienie latarki są wyłączone albo mapa nie była rysowana)
- mapa idzie za ręką, więc przy ruchu siatka tekseli przesuwa się po świecie (czego pudełko
  księżyca unika): krawędzie cieni mogą migotać. Nikt tego nie oglądał
- wektor "w górę" światła zmienia się skokiem, gdy wiązka jest odchylona od pionu o mniej niż
  2,56 stopnia (próg `VERTICAL_DIRECTION_LIMIT`), więc mapa w jednej klatce obraca się. Przy
  ustawieniach startowych to **nie następuje** nawet przy kamerze nachylonej o 89 stopni (wiązka
  ma wtedy 2,85 stopnia od pionu w górę i 3,24 w dół, bo ręka jest 0,2 m w prawo). Następuje
  przy `Converge at` powyżej około 4,6 m (patrzenie w górę) albo 5,2 m (w dół), albo przy
  mniejszym `Hand right`. Policzone, nie oglądane
- panel Shadows pokazuje obraz zakładki `Flashlight` według faktu (czy mapa była rysowana w
  ostatniej klatce), a zakładki `Moon` według ustawienia `Shadows`

**Widziane na zrzutach ekranu przez agenta (2026-10-06).** Środowisko i metoda jak w sekcji 20.1 (ta sama sesja, ten sam hak testowy).

Poniższe punkty to "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela". **Nie zamykają żadnego punktu listy właściciela** (sekcja 21.2).

- widziane na zrzucie ekranu przez agenta: panel Shadows ma dwie zakładki, a panel Lights trzy nowe suwaki ręki
- widziane na zrzucie ekranu przez agenta: zakładka `Flashlight`: `1024 x 1024`, `13.6 x 13.6 m at 16.0 m`, `0.08 cm per metre away`
- widziane na zrzucie ekranu przez agenta: plama latarki jest wyśrodkowana w 4 m, a w 1 m i 1,6 m leży na prawo i poniżej środka obrazu (ręka jest przesunięta)
- widziane na zrzucie ekranu przez agenta: cień kryształu leży obok niego (w górę i w lewo), wypełniony turkusem własnego światła kryształu; przy słupkach cienkie pasy
- widziane na zrzucie ekranu przez agenta: oświetlony grunt jest identyczny przy włączonych i wyłączonych cieniach do 13 m, z jądrami `3 x 3` i `7 x 7`; przy bias 0 widać przyciemnienie i prążki
- widziane na zrzucie ekranu przez agenta: podstawy ścian są czyste: bez jasnej szczeliny i bez ciemnego pasa
- widziane na zrzucie ekranu przez agenta: patrząc prosto w dół, plama jest okrągła i nie ma skoku obrazu
- widziane na zrzucie ekranu przez agenta: noclip (klawisz N) w ścianę bez awarii
- widziane na zrzucie ekranu przez agenta: podgląd mapy latarki jest szary z białym niebem, a klawisz F przełącza światło

**Nie widziane na ekranie:** brama rzucająca cień latarki; cienie latarki w trybie `Gouraud`. **Znane ograniczenie trybu `Gouraud` (z kodu, potwierdzone na zrzucie):** światło jest liczone w wierzchołkach, a ściana ma jeden czworokąt na 2 m, więc sama plama latarki na ścianach znika, zanim cień ma co pokazać. To granica trybu, nie błąd cieni.

### 21.2. Otwarte: test ręczny na około trzydzieści minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Punkty, które agent widział na zrzutach (lista "Widziane na zrzutach ekranu przez agenta" w sekcji 21.1), zostają `[ ]`: oglądanie przez agenta nie zastępuje testu właściciela.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu, z testów i ze wzorów,
nie z klikania ani ze zrzutów ekranu. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/ShadowsPanel.cpp` i `LightsPanel.cpp`. Ustawień cieni i ręki program nigdzie
nie zapisuje, więc ponowne uruchomienie gry przywraca wartości startowe. Listy 17.2 do 20.2
też są otwarte.

Przygotowanie:

- [ ] usunąć stary plik `imgui.ini`, uruchomić grę z terminala. Oczekiwane w konsoli: żadnej
      linii `[error]`, w szczególności o shaderach `lit.frag`, `gouraud.vert`, `gouraud.frag`,
      `grass.frag`, `common/shadows.glsl` i `common/lighting.glsl` ani o niekompletnym
      framebufferze. W buildzie Debug żadnej linii `GL_...`
- [ ] okno debug (Light / Shadows) ma teraz dwie zakładki, `Moon` i `Flashlight`. Okno debug (Light / Lights) w grupie
      `Flashlight (spot)` ma po suwaku `Beam range` trzy nowe suwaki: `Hand right`, `Hand
      down` i `Converge at`
- [ ] zakładka `Flashlight`, wartości startowe: `Map: 1024 x 1024, GL_DEPTH_COMPONENT24`, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Covers 13.6 x 13.6 m at 16.0 m`, `One texel: 0.08 cm per metre away`. Te liczby są
      policzone, nie odczytane: zapisać, jeśli panel pokazuje co innego

Obraz bez zmian i położenie plamy:

- [ ] **obraz księżyca bez zmian.** Zgasić latarkę (F): scena ma wyglądać jak po części 4 [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      (cienie księżyca w tych samych miejscach, ten sam kontrast). Zakładka `Moon` działa
      jak dotąd
- [ ] zapalić latarkę, stanąć twarzą do ściany 4 m dalej: plama jest w środku ekranu
- [ ] podejść do ściany na 1 m: plama jest w prawo i poniżej środka ekranu (rachunek: około
      0,15 m w prawo i 0,19 m niżej). Odejść na 8 m: plama jest lekko w lewo i powyżej
      środka
- [ ] `Converge at` na 10: plama jest w środku ekranu na ścianie 10 m dalej, a z bliska
      jeszcze bardziej w prawo i w dół. `Converge at` na 0,5: plama z bliska jest blisko
      środka, a daleko mocno w lewo i w górę. Wrócić do 4
- [ ] `Hand right` i `Hand down` na 0: latarka jest w oku, plama zawsze w środku, a cienie
      latarki chowają się za rzucającymi (każdy dokładnie za swoim przedmiotem). Przywrócić
      0,20 i 0,25
- [ ] obracać mysz podczas chodzenia: plama nie spóźnia się za obrazem (pozycja ręki jest
      liczona z tego samego oka co macierz widoku)

Cienie latarki:

- [ ] stanąć w korytarzu, patrzeć wzdłuż niego: róg ściany lub słupek rzuca cień na ścianę
      za nim, a cień jest **widoczny** obok rzucającego (to jest cel przeniesienia latarki do
      ręki). Zapisać, po której stronie rzucającego pada cień
- [ ] w zakładce `Flashlight` odznaczyć `Shadows`: latarka znów świeci przez wszystko, cienie
      latarki znikają, cienie księżyca zostają. Linia faktów pokazuje `Map: not drawn`, a w
      miejscu obrazu stoi `(not drawn)`. Zaznaczyć z powrotem
- [ ] `Strength` zakładki `Flashlight` na 0, 0,5 i 1: cienie latarki niewidoczne, o połowę [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      słabsze, pełne. Cienie księżyca bez zmian
- [ ] **dwa cienie naraz.** Stanąć tam, gdzie cień księżyca i cień latarki nakładają się:
      fragment w obu traci oba udziały, ale zachowuje światło otoczenia i kryształy. Nic nie
      robi się czarne
- [ ] kryształ stojący w cieniu latarki nadal świeci własnym światłem i oświetla ziemię
- [ ] trawa przy ścianie w cieniu latarki jest ciemniejsza od trawy w plamie

Bias i wycieki:

- [ ] **grunt daleko.** Patrzeć na poziomy grunt 8 do 16 m przed sobą (korytarz lub otwarty
      teren). Zapisać, czy oświetlony grunt ciemnieje albo ma prążki za około 10 m. Powtórzyć
      dla `Kernel` `3 x 3`, `5 x 5` i `7 x 7` oraz dla obu biasów równych 0 (acne). Z rachunku
      bias startowy wystarcza do 10 m przy jądrze `3 x 3`: dalej i przy większym jądrze może
      brakować
- [ ] **podstawy ścian.** Iść wzdłuż korytarza, patrzeć na styk ściany z ziemią w plamie:
      zapisać, czy między ścianą a ziemią jest jasna szczelina (za duży bias, światło
      przecieka u podstawy) albo ciemny pas (za mały)
- [ ] `Constant bias` na koniec zakresu: cień odkleja się od rzucającego. Wrócić do 0,010 i
      0,130
- [ ] **za płaszczyzną daleką.** `Beam range` na 6 m: za 6 m światło zanika, a cienie nie
      pojawiają się za granicą zasięgu (kod zwraca "oświetlone" dla głębi powyżej 1). Zapisać,
      czy na granicy widać pas albo skok jasności. Wrócić do 16 m
- [ ] `Beam range` na 60 m: mapa pokrywa dużo więcej, tekseli jest mniej na metr. Zapisać
      wygląd krawędzi cieni na 20 m. Wrócić do 16 m

Kamera i ruch:

- [ ] **prawie prosto w górę i w dół, ustawienia startowe.** Patrzeć kolejno na niebo i na
      ziemię pod stopami (pochylenie do 89 stopni). Oczekiwane (z rachunku): cień nie znika i
      nie skacze, bo wiązka nie zbliża się do pionu bliżej niż 2,85 stopnia
- [ ] **skok wektora "w górę", wywołany.** Ustawić `Converge at` na 10 m (albo `Hand right` na 0),
      patrzeć w górę i w dół z pochyleniem 89 stopni. Oczekiwane: w jednej klatce przy
      przejściu przez próg mapa obraca się. Zapisać, czy skok jest widoczny w scenie
- [ ] **migotanie krawędzi.** Iść, obracać się i patrzeć na krawędź cienia latarki na ścianie:
      zapisać, czy krawędź pływa albo migocze (mapa idzie za ręką). Powtórzyć z `Kernel`
      `7 x 7`
- [ ] latarka przy niskiej baterii (okno debug (Gameplay), `Battery` poniżej progu): światło migocze, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      a przebieg cieni dostaje ustawienia klatki (nie ustawienia z panelu), więc cień ma iść
      za światłem. Przy pustej baterii latarka gaśnie, a zakładka `Flashlight` pokazuje
      `(not drawn)`
- [ ] klawisz F: zakładka `Flashlight` przełącza się między obrazem a `(not drawn)`, a po [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      ponownym włączeniu wraca bez błędów w konsoli

Pozycja przy ścianie:

- [ ] stanąć bokiem do ściany tak blisko, jak pozwalają kolizje, `Hand right` 0,20, potem 0,25
      (koniec zakresu): ściana obok nie znika i nie jest odcięta (płaszczyzna bliska 0,05
      m). Obrócić się dookoła w miejscu: ręka zawsze zostaje wewnątrz ciała
- [ ] **noclip (klawisz N)**, lot w ścianę tak, żeby światło znalazło się wewnątrz niej: nie ma [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      awarii ani NaN w obrazie. Zapisać, co widać (zakładka `Flashlight` i scena)
- [ ] patrząc w dół (`Hand down` 0,25): latarka jest pod okiem, a nie za nim

Zakładka `Flashlight` okna debug (Light / Shadows):

- [ ] obraz po prawej (`Distance seen from the flashlight`): szary, nie czerwony i nie
      cały biały. Bliskie powierzchnie ciemne, dalekie jasne, puste miejsca (niebo) białe
- [ ] po naciśnięciu F (latarka zgaszona) w miejscu obrazu stoi `(not drawn)`, a linia faktów
      pokazuje `Map: not drawn`
- [ ] lista `Resolution`, `2048 x 2048`: fakty pokazują `One texel: 0.04 cm per metre away`,
      a po powrocie do `1024 x 1024` `0.08 cm per metre away`. W konsoli żadnej linii
      `[error]`
- [ ] podpowiedź obrazu zaczyna się od `The shadow map of the flashlight: black is at the
      hand`
- [ ] przełączanie zakładek `Flashlight` i `Moon`: obraz mapy wraca po jednej klatce. Zwinięty [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      okno debug (Light / Shadows): żadna z map nie jest rysowana do podglądu
- [ ] układ: okno debug (Light / Shadows) po zmianie ma dwie zakładki i nadal mieści osiem kontrolek, trzy
      linie faktów i obraz bez przewijania. Grupa `Flashlight (spot)` w oknie debug (Light / Lights) jest
      dłuższa o trzy suwaki: sprawdzić, czy panel mieści się w oknie 1280 x 720

Tryby i przeładowanie:

- [ ] okno debug (Render / Scene), lista `Lighting`: `Blinn-Phong` i `Phong` z cieniami latarki w tych samych
      miejscach. **`Gouraud`**: cienie latarki w tych samych miejscach i z ostrymi krawędziami
      (udział latarki jest liczony na wierzchołek, test cienia na fragment). Ten tryb nie był
      rysowany podczas zgłoszonego startu. `Unlit`: żadnych cieni
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders), `Reload shaders`: jedenaście linii `OK`, cienie latarki zostają. Odznaczyć
      oba `Shadows` (księżyc i latarka) i przeładować: scena nadal się rysuje, w buildzie
      Debug żadnej linii `GL_INVALID_OPERATION` (dwa samplery cieni na jednostkach 3 i 4 oraz
      sampler tekstury na 0 i 1). Przeładowanie nie było w zgłoszonym starcie
- [ ] zakładka `Moon`: jej obraz i fakty bez zmian względem listy 20.2 (`Map: 2048 x 2048`, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      `Covers 64.8 x 54.1 m, 47.0 m deep`)

Na koniec:

- [ ] **koszt drugiego przebiegu głębi.** W jednej sesji, w buildzie Release, z ukrytymi
      panelami, w oknie 1280 x 720 i po zmaksymalizowaniu zapisać liczbę klatek na sekundę
      dla commita części 4 (poprzedniego commita tego repozytorium) i dla tej części, każdą
      wartość dwa razy: z latarką zgaszoną, z latarką zapaloną i `Shadows` zakładki
      `Flashlight` odznaczonym, z mapą 1024 i z mapą 2048. Przebieg latarki rysuje te same
      obiekty drugi raz, a test cienia dochodzi do każdego fragmentu (sekcja 21.1)
- [ ] w konsoli przez cały test żadnej linii `[error]` poza wywołanymi celowo
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 22. Lista kontrolna M7, część 6: minimapa

Szósta, ostatnia część kamienia milowego M7 daje grze minimapę: kwadratową mapę labiryntu
w prawym dolnym rogu okna (od 2026-10-06 domyślnie w lewym dolnym: sekcja 27), z północą u góry, która pokazuje **tylko korytarze, które gracz
odkrył**. Dwa przebiegi po przebiegu składającym (`NightMazeApp::drawMinimap`): pierwszy
rysuje schemat z danych labiryntu (podłogi odkrytych komórek, ściany, brama, kryształy,
strzałkę gracza) do własnego framebuffera `GL_RGBA8` bez głębi, o boku równym kwadratowi na
ekranie (domyślnie 0,28 wysokości framebuffera okna, czyli 202 piksele przy 720), drugi
kopiuje ten obraz piksel w piksel w róg okna z przezroczystością 0,85. Komórka jest odkryta
przez linię wzroku wzdłuż korytarzy: komórka gracza i komórki w linii prostej w czterech
kierunkach aż do ściany. Doszły: `src/game/Discovery.*` (stan i reguła), `src/game/Minimap.*`
(ustawienia, kwadrat w oknie, rzut ortograficzny, lista trójkątów), `src/game/MinimapRenderer.*`
(framebuffer, bufor `GL_DYNAMIC_DRAW`, dwa przebiegi), `Round::discovery` w `Round.*`,
`cellAt` w `MazeLayout.*`, czwarty parametr (podpowiedź użycia) i metoda `setData` w
`gfx::Buffer`, trzy stałe w `ShaderUniforms.hpp`, `drawMinimap` i klawisz M w `NightMazeApp`,
cztery nowe pola `DebugContext` (razem czterdzieści dwa), dwa programy shaderów (razem
trzynaście: `post/minimap.vert` z `post/minimap.frag` oraz `post/minimap_overlay.frag` z
istniejącym `post/composite.vert`) i trzecia zakładka `Minimap` panelu Framebuffers. **Nie
doszedł** żaden panel (jest ich nadal dwanaście). Opis kodu:
[`../modules/renderer/minimap.md`](../modules/renderer/minimap.md). Decyzje:
[`../decisions/minimap-discovered-corridors.md`](../decisions/minimap-discovered-corridors.md)
(reguła odkrywania i schemat z danych to decyzje właściciela, reszta to wybory wykonawcze),
[`../decisions/minimap-srgb-constants-after-composite.md`](../decisions/minimap-srgb-constants-after-composite.md)
i
[`../decisions/minimap-vertices-rebuilt-every-frame.md`](../decisions/minimap-vertices-rebuilt-every-frame.md).

Kamień milowy M7 jest od tej części **kompletny w kodzie na Windowsie** (wszystkie sześć
części) i **nie jest zamknięty**: listy ręczne od 17.2 do 22.2 są otwarte, na macOS kod nie
był budowany ([`build-macos.md`](build-macos.md)), tematy 10 i 11 nie są odhaczone w
[`../syllabus.md`](../syllabus.md) i nie ma tagu. Stan całego M7 w jednym miejscu:
[`m7-status.md`](m7-status.md).

### 22.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru
nie zapisano w tym dokumencie. Z uruchomienia programu zgłoszono linię `GL_VERSION`
(OpenGL 4.1.0, NVIDIA) i linie wczytania zasobów. **Żadnego z poniższych punktów nie
powtarzałem przy pisaniu tej sekcji.** Liczby w tabelach poniżej przepisałem z kodu albo
policzyłem z jego stałych i testów: nie są odczytane z ekranu, a **do 2026-10-06 nikt nie obejrzał minimapy** (agent obejrzał ją później: lista niżej, przed sekcją 22.2).

Bramka i start:

- [x] `make check` przechodzi w Debug i w Release (zgłoszone)
- [x] `night_maze_tests.exe`: 414 przypadków testowych i 138711 asercji, wszystkie
      przechodzą (zgłoszone). Względem części piątej (375 i 138506) doszło 39 przypadków i
      205 asercji. Przypadki policzyłem z plików testów: 19 nowych w `DiscoveryTests.cpp`,
      19 nowych w `MinimapTests.cpp` i 1 nowy w `MazeLayoutTests.cpp` (`cellAt finds the
      cell a point of the world lies in`): `375 + 19 + 19 + 1 = 414`. Podziału 205 asercji
      między pliki nie liczyłem: pętle w testach sprawiają, że liczba `CHECK` w pliku nie
      jest liczbą asercji w czasie działania
- [x] start programu Debug przez około 7 sekund (zgłoszone): `GL_VERSION` 4.1.0 NVIDIA,
      assety wczytane, standardowe wyjście błędów puste. Minimapa jest domyślnie włączona,
      więc z pustego wyjścia błędów wynika (**moje wnioskowanie ze zgłoszenia**, nie osobna
      obserwacja), że oba nowe programy się skompilowały, a framebuffer mapy był kompletny.
      Wyjście błędów nic nie mówi o wyglądzie mapy: trójkąt odrzucony przy odrzucaniu
      tylnych ścian nie zgłasza błędu OpenGL

Czego ten start **nie** obejmował (zgłoszone wprost):

- poruszania graczem (odkrywania komórek w ruchu)
- klawisza M
- zakładki `Minimap` panelu Framebuffers
- pola `Reveal all`
- przycisku `Reload shaders`
- zmiany rozmiaru okna
- obu widoków diagnostycznych

Nikt nie kliknął żadnej z nowych kontrolek i nikt nie obejrzał mapy. Liczby klatek dla tej
części nie zostały zmierzone.

Wartości startowe ustawień (`MinimapSettings`, pilnuje ich test `the minimap settings start
with the agreed values`). Kontrolki stoją w zakładce `Minimap` panelu Framebuffers, napisy
są takie jak w `FramebuffersPanel.cpp`:

| Ustawienie | Wartość |
|---|---|
| `Minimap` (klawisz M) | zaznaczone |
| `Reveal all` | odznaczone |
| `Size` | 0,28 (suwak od 0,10 do 0,60) |
| `Margin` | 0,020 (suwak od 0,000 do 0,100) |
| `Corner` | `Bottom right` (lista: `Top left`, `Top right`, `Bottom left`, `Bottom right`) |
| `Opacity` | 0,85 (suwak od 0,10 do 1,00) |

Co te wartości znaczą w liczbach (policzone z kodu, jego komentarzy i testów, **nie
zmierzone na ekranie**; część jest przypięta testami `MinimapTests.cpp`):

| Okno (framebuffer) | Bok kwadratu | Margines | Lewy dolny róg kwadratu w prawym dolnym rogu okna, `x` i `y` |
|---|---|---|---|
| 1280 x 720 | 202 (`lround(201,6)`) | 14 (`lround(14,4)`) | 1064 i 14 |
| 2560 x 1440 | 403 (`lround(403,2)`) | 29 (`lround(28,8)`) | 2128 i 29 |

- labirynt 10 x 10 (domyślny): świat widoczny ma 21,2 m (półbok 10,6 m), przy boku 202
  pikseli to około 0,105 m na piksel. Komórka ma około 19 pikseli, ściana (0,3 m) około 2,9
  piksela, środek komórki `(0, 0)` leży około 15,2 piksela od lewej i od górnej krawędzi mapy
- labirynt 40 x 40 (największy z panelu Maze): półbok 42,4 m, około 0,42 m na piksel, komórka
  około 4,8 piksela. Działają wtedy wszystkie trzy minima w pikselach: ściana 0,63 m, romb
  kryształu o promieniu 0,84 m i strzałka długa na 2,1 m. Czy taka mapa jest czytelna,
  nikt nie sprawdzał
- lista trójkątów przy pokazanym w całości labiryncie 10 x 10: około 1400 wierzchołków po 20
  bajtów (komentarz w kodzie), moje przybliżenie z liczby podłóg i ścian to 1407, czyli około
  28 KB kopiowane na kartę w każdej klatce. Liczby nie sprawdzono w działającym programie
- strzałka: kierunek `(sin yaw, -cos yaw)`: yaw 0 patrzy na północ (u góry mapy), 90 na
  wschód (w prawo). Pilnuje tego test `the arrow of the player is drawn last and points
  where the camera looks`

Znane ograniczenia tej części (szczegóły w dokumencie modułu, sekcja 2.12):

- obrazu nie oglądał właściciel (agent tak, patrz niżej): kolory, grubości, rozmiar strzałki i czytelność są policzone
- w domyślnym układzie paneli panel Assets stoi w prawej kolumnie do dołu okna i **przykrywa
  prawy dolny róg**, w którym stoi mapa. Do oglądania mapy trzeba schować panele
- panel Framebuffers nie dostał większej wysokości (`FRAMEBUFFERS_HEIGHT` 344 bez zmian),
  a zakładka `Minimap` jest wyższa niż dwie pozostałe: panel może się przewijać
- panel Shaders ma trzynaście linii przy wysokości panelu 280: z rachunku (13 x 21 +
  22 + 27, około 322) przewija się
- mieszanie nakładki odbywa się na wartościach sRGB, a nie liniowych (jak HUD i panele)
- ściana między komórką odkrytą a nieodkrytą jest rysowana, więc wchodzi grubością o pół
  grubości w komórkę nieodkrytą
- reguła nie ma zasięgu: korytarz jest odkrywany aż do ściany, także długi
- odkrywanie zostaje po zmianie skali wysokości terenu (to nie jest nowa runda), a jest
  zerowane przy klawiszu R i przy nowym labiryncie
- lista wierzchołków jest odbudowywana i kopiowana co klatkę: koszt niezmierzony

**Widziane na zrzutach ekranu przez agenta (2026-10-06).** Środowisko i metoda jak w sekcji 20.1 (ta sama sesja, ten sam hak testowy).

Poniższe punkty to "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela". **Nie zamykają żadnego punktu listy właściciela** (sekcja 22.2).

- widziane na zrzucie ekranu przez agenta: mapa stoi w prawym dolnym rogu (wtedy domyślny róg, od 2026-10-06 jest lewy dolny), jest kwadratowa (202 piksele), ciemna z jasnymi ścianami
- widziane na zrzucie ekranu przez agenta: północ jest u góry, strzałka gracza się obraca (po obrocie do `Yaw` 270 wskazuje w lewo)
- widziane na zrzucie ekranu przez agenta: na starcie widać komórkę startu i korytarze od niej; korytarz jest odkryty do pierwszej ściany i nie dalej
- widziane na zrzucie ekranu przez agenta: turkusowe romby kryształów w odkrytych komórkach; start niebieski, wyjście zielone, brama pomarańczowa (z `Reveal all`)
- widziane na zrzucie ekranu przez agenta: zakładka `Minimap` pokazuje `202 x 202 px, GL_RGBA8`, obraz jest ostry
- widziane na zrzucie ekranu przez agenta: klawisz M chowa mapę, a zakładka pokazuje wtedy `not drawn (minimap off)`; `Reveal all` pokazuje cały labirynt
- widziane na zrzucie ekranu przez agenta: górne rogi okna są wolne od HUD
- widziane na zrzucie ekranu przez agenta: oba widoki diagnostyczne i `Unlit` nie zmieniają mapy
- widziane na zrzucie ekranu przez agenta: w labiryncie 40 x 40 strzałka jest większa od komórki i zasłania komórkę startu
- widziane na zrzucie ekranu przez agenta: panel Framebuffers przewija się na zakładce `Minimap`, a cztery podglądy pod nią są obcięte (otwarty drobny punkt kosmetyczny)
- widziane na zrzucie ekranu przez agenta: ramka wokół mapy (po poprawce z 2026-10-06; widział ją autor poprawek w swoim worktree przed scaleniem, nie ten sam agent): cienka i wyciszona przy starcie rundy

**Nie widziane na ekranie:** reguły odkrywania na odnogach i w narożnikach, otwarcie bramy i zebranie kryształu widziane na mapie (to **nie było chodzone**), kwadraty dźwigni i kartek, zmiana rozmiaru okna, `Reload shaders`.

**Ramka minimapy (poprawka wykonawcza z 2026-10-06, nie decyzja właściciela).** Przy kilku odkrytych komórkach mapa była ciemnym kwadratem na ciemnej scenie i nic nie pokazywało, gdzie się kończy. Przebieg nakładki (`post/minimap_overlay.frag`) rysuje teraz linię o szerokości `max(1, round(0,0015 * wysokość framebuffera okna))` pikseli (1 piksel przy 720, 2 przy 1440) w kolorze `MINIMAP_BORDER_COLOR` `(0,30; 0,36; 0,48)` (sRGB), na najbardziej zewnętrznych pikselach kwadratu, z tą samą przezroczystością 0,85 co mapa (zgłoszone z kodu; opis w [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md), sekcja 2.9). Zmiana nie ma testu jednostkowego.

### 22.2. Otwarte: test ręczny na około trzydzieści minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Punkty, które agent widział na zrzutach (lista "Widziane na zrzutach ekranu przez agenta" w sekcji 22.1), zostają `[ ]`: oglądanie przez agenta nie zastępuje testu właściciela. Doszedł jeden punkt po poprawce z 2026-10-06:

- [ ] **ramka minimapy.** Przy starcie rundy mapa ma cienką, wyciszoną niebiesko-szarą linię po całym obwodzie (1 piksel przy 720 pikselach wysokości framebuffera okna, 2 przy 1440), która pokazuje, gdzie mapa się kończy, nie zasłania pierwszych komórek i nie wygląda jak ściana labiryntu

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu, z testów i ze wzorów,
nie z klikania ani ze zrzutów ekranu. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/FramebuffersPanel.cpp`. Ustawień minimapy program nigdzie nie zapisuje, więc
ponowne uruchomienie gry przywraca wartości startowe. Listy 17.2 do 21.2 też są otwarte.

Przygotowanie:

- [ ] usunąć stary plik `imgui.ini`, uruchomić grę z terminala. Oczekiwane w konsoli: żadnej
      linii `[error]`, w szczególności o `minimap.vert`, `minimap.frag`,
      `minimap_overlay.frag` ani o niekompletnym framebufferze. W buildzie Debug żadnej linii
      `GL_...`
- [ ] **schować panele** (klawisz akcentu, `~`) przed oglądaniem mapy: w domyślnym układzie
      okno debug (Diagnostics / Assets) stoi w prawej kolumnie do dołu okna i przykrywa prawy dolny róg. Panele
      wracają tym samym klawiszem
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders): trzynaście linii `OK`, dwie ostatnie to `minimap.vert + minimap.frag` i
      `composite.vert + minimap_overlay.frag`. Zapisać, czy panel się przewija (z rachunku
      tak)

Wygląd i orientacja mapy:

- [ ] mapa stoi w lewym dolnym rogu (od 2026-10-06 domyślny róg, wcześniej prawy dolny), jest kwadratem, ma ciemne tło i jasne ściany. Zapisać
      wygląd kolorów i czytelność: **tego nikt jeszcze nie widział**
- [ ] **północ u góry.** Ustawić się twarzą na północ (strzałka na mapie wskazuje w górę), potem
      obracać mysz: strzałka obraca się razem z kamerą, mapa stoi w miejscu. Twarzą na wschód
      strzałka wskazuje w prawo (yaw 90 stopni)
- [ ] strzałka porusza się płynnie podczas chodzenia (pozycja jest zmieszana między krokami
      stałymi jak kamera), a nie skokami
- [ ] na starcie mapa pokazuje komórkę startu (jaśniejszy niebieski kolor podłogi) i
      korytarze, które z niej wychodzą, aż do ściany. Reszta jest ciemna

Odkrywanie:

- [ ] **korytarz odkryty do ściany.** Stanąć w korytarzu: na mapie pojawia się cała prosta
      do pierwszej ściany w obu kierunkach. Nie więcej
- [ ] **odnoga ukryta do wejścia w linię.** W ścianie korytarza jest otwór prowadzący w bok:
      na mapie widać go jako przerwę w ścianie, ale komórki za nią są ciemne. Dopiero po
      stanięciu w komórce na wysokości otworu (w linii z odnogą) odnoga się odkrywa. To jest
      reguła z decyzji właściciela: zapisać, czy wygląda poprawnie
- [ ] **róg.** Po skręcie w korytarz za rogiem komórki za rogiem są odkryte dopiero po
      stanięciu w komórce narożnej
- [ ] **start, wyjście i brama.** Komórka startu jest jaśniejszym niebieskim, komórka wyjścia
      ciemnozielonym (pojawia się dopiero po odkryciu). Brama przy wyjściu ma kolor
      pomarańczowy, gdy blokuje. Po zebraniu progu kryształów i otwarciu bramy zmienia kolor
      na przygaszony niebieskozielony
- [ ] **kryształy.** Turkusowe romby leżą w odkrytych komórkach. Po zebraniu kryształu jego
      romb znika z mapy. Kryształy w nieodkrytych komórkach nie są widoczne
- [ ] **wygrana.** Po przejściu przez bramę odkrywanie trwa dalej (spacer za kartą wygranej
      nadal odkrywa komórki)
- [ ] **noclip (klawisz N).** W locie nad labiryntem odkrywane są komórki pod graczem, a
      strzałka jest na mapie w odpowiednim miejscu. Poza labiryntem strzałka może być ucięta
      przez krawędź obrazu mapy
- [ ] **nowy labirynt i restart.** `Regenerate` w oknie debug (World / Maze) i klawisz R zerują odkrycie:
      mapa znów pokazuje tylko komórkę startu i korytarze z niej. Nowy labirynt ma inny
      kształt mapy
- [ ] **zmiana skali wysokości terenu** (okno debug (World / Terrain and grass / Terrain)): odkrycie **zostaje** (to nie jest
      nowa runda)

Położenie i rozmiar:

- [ ] **położenie względem HUD.** HUD stoi na górze pośrodku. Mapa w lewym dolnym rogu (od 2026-10-06 domyślny róg) nie
      zasłania go przy 720p i przy 1440p. Zapisać, czy przy `Top right` mapa i HUD się
      spotykają
- [ ] rozdzielczość 1280 x 720: mapa ma mieć 202 piksele boku i margines 14 pikseli (z rachunku).
      W zakładce `Minimap` linia `Framebuffer:` pokazuje `202 x 202 px, GL_RGBA8`
- [ ] rozdzielczość 2560 x 1440 (większe okno albo maksymalizacja): `403 x 403 px`. Mapa
      zajmuje ten sam ułamek wysokości okna co przy 720p
- [ ] kopia piksel w piksel: obraz mapy jest ostry, bez rozmycia ani ząbków na krawędziach
      ścian (zapisać, czy tak jest)
- [ ] labirynt 40 x 40 (okno debug (World / Maze), `Width` i `Height` na 40, `Regenerate`): zapisać
      czytelność. Z rachunku komórka ma około 5 pikseli, a ściana, romb kryształu i strzałka
      mają minima w pikselach. Czy da się z mapy czytać? Czy strzałka jest widoczna?

Zakładka `Minimap` okna debug (Gameplay / Minimap):

- [ ] dawny panel Framebuffers miał trzy zakładki (`Tone and bloom`, `Fog and vignette`, `Minimap`). Dziś te kontrolki stoją w kategorii Post process (karty Tone mapping, Bloom, Fog, Vignette, Previews, bez zakładek) i w kategorii Gameplay (karta Minimap)
- [ ] zakładka `Minimap`: **siedem wierszy** lewej kolumny (pole `Minimap`, pole `Reveal all`, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      suwaki `Size` i `Margin`, lista `Corner`, suwak `Opacity` i linia `Framebuffer:`) i
      obraz po prawej. **Czy panel się przewija** (wysokość 344 nie została zmieniona) i czy
      obraz mieści się bez przewijania. Zapisać rozmiar obrazu
- [ ] obraz w zakładce jest tym samym obrazem co w rogu okna, ale **bez przezroczystości**
      (pełne krycie, tło ciemnoniebieskie) i taki, jaki leży w teksturze (sRGB pokazane tak,
      jak zapisano). Zapisać, czy nie jest odwrócony: powinien być prosto
- [ ] klawisz M wyłącza mapę i pole `Minimap` się odznacza. Z wyłączoną mapą linia mówi
      `Framebuffer: not drawn (minimap off)`, a obraz zastępuje napis `(not drawn)`
- [ ] **`Reveal all`.** Zaznaczone: cały labirynt jest na mapie, także nieodkryte komórki,
      wyjście i brama. Odznaczone: mapa wraca dokładnie do stanu odkrycia sprzed zaznaczenia
      (odkrycie nie zniknęło)
- [ ] `Size` na 0,10 i na 0,60: mapa zmienia rozmiar, linia `Framebuffer:` pokazuje nowy
      bok (72 i 432 piksele przy 720p), mapa zostaje ostra
- [ ] `Margin` na 0 i na 0,100: odstęp od krawędzi okna zmienia się, mapa nigdy nie wychodzi
      poza okno
- [ ] `Corner`: wszystkie cztery rogi. Mapa stoi w wybranym rogu, a przy górnych rogach [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      zapisać, czy HUD ją nachodzi
- [ ] `Opacity` na 1,00 i na 0,10: przy 1,00 mapa zakrywa scenę całkowicie, przy 0,10 scena
      jest prawie w całości widoczna. Zapisać wygląd przy 0,85 (wartość startowa)

Okno, shadery, tryby:

- [ ] **zmiana rozmiaru okna.** Przeciągnąć krawędź: mapa zostaje w lewym dolnym rogu (od 2026-10-06 domyślny róg), jej
      rozmiar idzie za wysokością, linia `Framebuffer:` się zmienia, w buildzie Debug żadnej
      linii `GL_...` i żadnego `is not complete`. **Zminimalizować** okno (framebuffer
      0 x 0) i przywrócić: bez awarii, mapa wraca
- [ ] **`Reload shaders`.** Trzynaście linii `OK`, mapa nadal się rysuje. Po przeładowaniu
      uniform `uMap` wraca do 0 i jest ustawiany w każdej klatce (komentarz w kodzie): zapisać,
      czy mapa nie znika
- [ ] **zepsuty shader.** Wprowadzić celowy błąd w `minimap.frag`, `Reload shaders`: linia
      `minimap.vert + minimap.frag` pokazuje błąd, a mapa nie rysuje się (kod
      `MinimapRenderer::drawMap` zwraca `false` dla nieprawidłowego programu), ale program
      nie ulega awarii. Przywrócić plik i przeładować
- [ ] **oba widoki diagnostyczne** (lista `View mode` w oknie debug (Render / Scene)): mapa jest widoczna
      w obu, bez zmiany kolorów (jest po przebiegu składającym)
- [ ] **mgła i bloom nie ruszają mapy.** Zwiększyć `Density` mgły i `Intensity` bloomu: kolory
      mapy się nie zmieniają
- [ ] **tryby `Gouraud`, `Phong` i `Unlit`** (lista `Lighting`): mapa wygląda tak samo

Wydajność:

- [ ] **FPS z mapą i bez niej, w jednej sesji.** Build Release, panele ukryte, okno 1280 x 720
      i po zmaksymalizowaniu: zapisać liczbę klatek na sekundę z mapą włączoną i wyłączoną
      (klawisz M), każdą wartość dwa razy, w tej samej sesji. Mapa dodaje budowę listy na
      procesorze, `glBufferData` i dwa przebiegi w każdej klatce. Liczb klatek między sesjami
      nie wolno porównywać (zob. [`m7-status.md`](m7-status.md), sekcja 5)
- [ ] w konsoli przez cały test żadnej linii `[error]` poza wywołanymi celowo
- [ ] zapisać wersję kompilatora, kartę graficzną i wersję sterownika: dla tej części nie
      zostały zapisane

## 23. Lista kontrolna M8, część 1: environment mapping

Pierwsza część kamienia milowego M8 daje kryształom i kałużom niebo. Kryształ odbija i załamuje
teksturę sześcienną nocnego nieba (ta sama, z której rysuje się skybox), a suwak `Refract /
reflect` wybiera między jednym a drugim. Kałuże leżą w części korytarzy (domyślnie 15
procent wolnych komórek, w labiryncie startowym 13 z 85) i odbijają to samo niebo, tym mocniej,
im bardziej płasko na nie patrzę (przybliżenie Schlicka). Od poprawek z 2026-10-06 kałuża jest cienką warstwą wody, która idzie za gruntem (każdy wierzchołek 8 mm nad terenem), ma miękki brzeg i przepuszcza grunt w środku. Obie rzeczy rysuje **nowy, czternasty
program shaderów, `reflect`** (po jedenastu starych i dwóch programach minimapy), w osobnym przebiegu po trawie i przed niebem. Doszedł też
**trzynasty panel, Environment**. Doszły: `src/game/EnvironmentMapping.*` (ustawienia i cztery
funkcje), `src/game/Puddles.*` (rozmieszczenie z ziarna, siatka kałuży idąca za gruntem),
`src/game/PuddleRenderer.*`, `assets/shaders/reflect.vert` i `reflect.frag`,
`src/debug/panels/EnvironmentPanel.*`, `tests/EnvironmentMappingTests.cpp` i
`tests/PuddleTests.cpp`. Zmieniły się: `NightMazeApp.*` (przebieg `drawReflections`, funkcje
`crystalsReflect`, `drawGateAndCrystals` i `layPuddles`), `GameplayRenderer.*` (połówki
`drawGate` i `drawCrystals`), `MazeWorld.*` (stała `START_CELL` w nagłówku), `ShaderUniforms.hpp`
(dziewięć nazw uniformów i `ENVIRONMENT_TEXTURE_UNIT` równa 5), `Skybox.hpp` (`cubemap()`),
`DebugContext.hpp` (trzy pola, razem 45 po scaleniu z minimapą), `DebugUI.cpp` (czternaście programów w panelu Shaders),
`PanelLayout.hpp` (`FOLDED_ROW_COUNT` z 4 na 5) i `main.cpp`. Opis kodu:
[`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md). Decyzje:
[`../decisions/reflect-own-program-and-pass.md`](../decisions/reflect-own-program-and-pass.md),
[`../decisions/puddles-follow-the-ground.md`](../decisions/puddles-follow-the-ground.md) (zastępuje [`../decisions/puddle-on-lowest-ground.md`](../decisions/puddle-on-lowest-ground.md), zachowaną jako historię),
[`../decisions/visible-effect-over-physical-values.md`](../decisions/visible-effect-over-physical-values.md).

Temat 12 wykładu (environment mapping) jest **w toku**, nie zaliczony: obraz obejrzał tylko agent (zrzuty z 2026-10-06, sekcja 23.1, nie właściciel), test ręczny właściciela (23.2) jest otwarty, a na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)). M8 jako całość nie jest zamknięty.

### 23.1. Zgłoszone (2026-10-06)

Środowisko: Windows, scalone drzewo (po minimapie).
Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru nie zapisano w tym
dokumencie. **Żadnego z poniższych punktów nie powtarzałem przy pisaniu tej sekcji.** Liczby w
tabelach poniżej przepisałem z kodu albo policzyłem z jego stałych: nie są odczytane z ekranu, a **w chwili tego zgłoszenia nikt nie obejrzał obrazu tej części** (agent obejrzał go później: lista "Widziane na zrzutach ekranu przez agenta" przed sekcją 23.2).

Bramka i start:

- [x] `make check` przechodzi (zgłoszone)
- [x] `night_maze_tests.exe` w scalonym drzewie (po szóstej części M7, minimapie): 445
      przypadków testowych i 150296 asercji (bramka scalonego drzewa z 2026-10-06: 138711 plus 11585 asercji tej części).
      Przypadki policzyłem z plików testów: 414 przed tą częścią plus 11 w
      `EnvironmentMappingTests.cpp` i 20 w `PuddleTests.cpp`, razem 31 (`414 + 31 = 445`).
      Podziału 11585 nowych asercji między pliki nie liczyłem. Przed scaleniem, w drzewie z samą
      tą częścią, zgłoszono 406 przypadków i 150091 asercji (przed tą częścią 375 i 138506)
- [x] start programu Debug przez około 8 sekund, standardowe wyjście błędów puste (zgłoszone)

Czego ten start **nie** obejmował (zgłoszone wprost; część z tego agent obejrzał potem, lista niżej). Wykonała się **tylko ścieżka domyślna**:
oświetlenie Blinna-Phonga, widok `Textured`, niebo włączone, efekt włączony. Nie wykonało się:

- rysowanie w trybach `Unlit`, `Gouraud` i `Phong`
- dwa widoki diagnostyczne (`Normals as colour`, `UVs as colour`)
- niebo wyłączone
- efekt wyłączony (pole `Environment mapping`) i kałuże wyłączone (pole `Puddles`)
- przycisk `Reload shaders` (dziś przy czternastu programach)

W tym starcie nikt nie kliknął żadnej z nowych kontrolek i nikt nie obejrzał ani jednego odbicia.

Wartości startowe (kontrolki panelu **Environment**; w nawiasie napis, jaki pokazuje kontrolka):

| Ustawienie | Wartość |
|---|---|
| `Environment mapping` | zaznaczone |
| `Sky share` | 0,50, suwak od 0 do 1 |
| `Refract / reflect` | 0,50, suwak od 0 do 1 |
| `Refraction ratio` | 0,67 (`AIR_TO_GLASS_RATIO` = 1 / 1,5), suwak od 0,40 do 1,50 |
| `Glow` | 1,00, suwak od 0 do 1 |
| `Puddles` | zaznaczone |
| `Share of cells` | 0,15, suwak od 0 do 0,50 (`MAX_PUDDLE_SHARE`) |
| `Reflectivity` | 0,50 (od poprawek z 2026-10-06, wcześniej 0,35), suwak od 0 do 1 |
| `Fresnel` | zaznaczone |
| `Puddles: N` | 13 w labiryncie startowym (policzone: `lround(85 * 0,15)`, test `the default maze has 13 puddles at the default share`) |
| `ENVIRONMENT_TEXTURE_UNIT` (stała) | 5 |
| `PUDDLE_LIFT`, `PUDDLE_RINGS`, `PUDDLE_CORNERS` (stałe) | 0,008 m, 6 i 32 (od poprawek z 2026-10-06; wcześniej `PUDDLE_DEPTH` 0,02 m i 16 narożników) |
| `PUDDLE_RIM_FADE`, `PUDDLE_OPACITY` (stałe) | 0,45 i 0,7 |
| kolor wody `PUDDLE_COLOR` (sRGB) | `(0,32; 0,40; 0,50)`, wcześniej `(0,07; 0,09; 0,11)` |
| `PUDDLE_MIN_RADIUS`, `PUDDLE_MAX_RADIUS`, `PUDDLE_MAX_OFFSET` (stałe) | 0,25 m, 0,45 m i 0,30 m |
| panel Environment | zwinięty w piątym rzędzie pasków pod panelem Shadows, po rozwinięciu 296 w wysokość |

Co te wartości znaczą w liczbach (policzone ze wzorów i komentarzy kodu, **nie zmierzone na
ekranie**; część jest przypięta testami):

- kałuża sięga najwyżej 0,75 m od środka komórki (0,3 przesunięcia plus 0,45 promienia), a stopa
  ściany zaczyna się 0,8 m od środka: żadna nie dotyka ściany (test `no puddle reaches a wall`)
- udział nieba w kałuży przy Fresnelu i `Reflectivity` 0,5, oczy 1,7 m nad gruntem: 50 procent z góry, 50 procent w 2 m, 54 procent w 4 m, 66 procent w 8 m, 79 procent w 16 m (przy dawnych 0,35: 35, 35, 40, 55 i 72). Krycie wody w środku kałuży (`0,7 + 0,3 F`): 0,85 z góry i w 2 m, 0,86 w 4 m, 0,90 w 8 m, 0,94 w 16 m. Dla 0,02: 2,5
  procent w 2 m, 10 procent w 4 m, 33 procent w 8 m. Test przypina cztery z tych nierówności
- kąt krytyczny szkła do powietrza: 41,81 stopnia (test), wody do powietrza 48,75 stopnia (nie
  testowany)
- księżyc w kałuży: kamera `Yaw` 205 i `Pitch` -50 (znak sprawdzić w panelu Camera), kałuża około
  1,4 m przed graczem w poziomie. Przybliżona odbijalność w tym miejscu 0,5 (policzone)
- jasność nieba na kryształach: zenit razy 2,2 to około 0,003, tarcza księżyca około 1,3 do 1,6.
  Świecenie kryształu przed teksturą około 2,45 przy pulsie 1 i 1,72 przy 0,7 (z notatki o bloomie)
- wysokość wody: każdy wierzchołek kałuży 8 mm nad gruntem pod nim. Najmniejszy odstęp od gruntu między wierzchołkami, z komentarza testu (nie mierzony ponownie): 7,56 mm przy skali 1,0 i 6,89 mm przy 2,5 dla 13 kałuż, 7,35 mm i 6,37 mm dla największej kałuży w dziewięciu miejscach każdej komórki. (Dawna reguła, "najniższy grunt z 17 punktów plus 2 cm", już nie obowiązuje.)

Znane ograniczenia tej części (szczegóły w dokumencie modułu, sekcja 2.15):

- tylko niebo: ściany, kryształy i gracz nie są odbijane
- kryształ ma jedno załamanie (wejście), bez tylnej powierzchni
- przy `Glow` 1 niebo na kryształach jest słabą domieszką (policzone), a `Sky share` 0,5 zmniejsza
  o połowę część oświetloną kryształu
- przy włączonym efekcie Gouraud nie zmienia światła kryształów i kałuż (światło na fragment)
- kałuże nie rzucają cienia; są trudne do zobaczenia poza wiązką latarki i dalej niż około 4 m (agent po poprawkach); warstwa jest płaska między wierzchołkami (do 7,5 cm), a nie gładka
- kępka trawy może stać w kałuży przy jej brzegu (pas trawy 0,57 do 0,79 m od środka komórki,
  kałuża do 0,75 m)
- wartości są dobrane do widoczności, nie do fizyki (`Reflectivity` 0,5, prawdziwa woda 0,02)

**Widziane na zrzutach ekranu przez agenta (2026-10-06).** Środowisko i metoda tego oglądania (jedna sesja agenta, 2026-10-06): wersja Release z commitu `9a33f18`, okno 1280 x 720, karta RTX 4070 Ti SUPER, sterownik zgłaszający OpenGL 4.1.0 NVIDIA 610.74. Ustawienia były zmieniane **tymczasowym hakiem testowym**, który pisał te same pola, które edytują panele, więc punkty typu "odznacz X w panelu" są potwierdzone co do **efektu**, nie co do widżetu. Prawdziwe, syntetyczne wejście posłużyło do obrotu myszą, klawiszy W, Shift, M, F, tyldy i kliknięć w panele.

Poniższe punkty to "widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela". **Nie zamykają żadnego punktu listy właściciela** (sekcja 23.2), a jej punkty zostają `[ ]`.

- widziane na zrzucie ekranu przez agenta: panel Environment stoi w piątym rzędzie pasków, HUD pod nim; wartości startowe jak w tabeli (w wersji z commitu `9a33f18`, czyli `Reflectivity` 0,35) i linia `Puddles: 13`
- widziane na zrzucie ekranu przez agenta: efekt wyłączony wygląda jak przed tą częścią
- widziane na zrzucie ekranu przez agenta: `Glow` 0 przy `Sky share` 1 pokazuje gwiazdy na krysztale
- widziane na zrzucie ekranu przez agenta: `Refract / reflect` 0, 0,5 i 1 dają różne obrazy; `Refraction ratio` 1,0 i 1,5 dają czysty obraz
- widziane na zrzucie ekranu przez agenta: przy pełnym świeceniu widać słaby odcień nieba na krysztale
- widziane na zrzucie ekranu przez agenta: przy wyłączonym niebie kryształ jest prawie czarny
- widziane na zrzucie ekranu przez agenta: tarcza księżyca i gwiazdy w kałuży przy `Yaw` 205 i `Pitch` -50
- widziane na zrzucie ekranu przez agenta: Phong, Gouraud, Unlit i oba widoki diagnostyczne bez czarnych kryształów
- widziane na zrzucie ekranu przez agenta: brzeg kałuży nie migocze między dwiema klatkami
- widziane na zrzucie ekranu przez agenta: obraz jako całość jest zdrowy: nic czarnego, prześwietlonego ani migoczącego, dwie klatki w odstępie 100 ms różnią się tylko zegarem HUD, falowaniem trawy i pulsem kryształów; mgła patrząc z góry ukrywa labirynt tak, jak mówi sekcja 19

**Znalezione na ekranie jako błędne** w wersji z commitu `9a33f18` (przyczyna poprawek niżej):

- **cztery z trzynastu kałuż pokazywały tylko 54 do 69 procent tarczy**, zakończone prostą cięciwą; trzy kolejne były lekko obcięte, sześć całych
- kałuże czytały się jako **prawie czarne dziury** w wiązce latarki i **nie były widoczne z wysokości chodzenia** w 4 m i 8 m
- panel Environment **ukrywał** `Fresnel` i `Puddles: 13`
- **nie oceniono** różnicy między `Fresnel` włączonym i wyłączonym

**Poprawki z 2026-10-06 (po pierwszym obejrzeniu).** Decyzje właściciela, w całości: (1) kałuże idą za gruntem, (2) woda ma być lepiej widoczna: jaśniejszy odcień, mocniej odbijająca, miękki brzeg, więcej narożników, (3) pasek HUD stoi przy górnej krawędzi, dopóki panele debug są schowane. Dwie dalsze zmiany to poprawki wykonawcze, nie decyzje: panel Environment w dwóch kolumnach (żeby nic nie było schowane) i cienka ramka wokół minimapy (sekcja 22). Zmiany są w drzewie roboczym, w 17 plikach w `src/`, `assets/shaders/` i `tests/`, i w chwili pisania **nie są zacommitowane**. Opis z kodu (nie powtarzałem go na ekranie):

- kałuża to jeden wierzchołek w środku i 6 pierścieni po 32 wierzchołki (193 wierzchołki, 352 trójkąty), zbudowana dla każdej kałuży osobno w przestrzeni świata, każdy wierzchołek 8 mm (`PUDDLE_LIFT`) nad `Terrain::heightAt`; wszystkie kałuże w jednej siatce z macierzą modelu równą jedynce; normalne `(0, 1, 0)`
- zewnętrzne 45 procent promienia zanika (`PUDDLE_RIM_FADE`), liczone na fragment ze współrzędnej tekstury
- przebieg kałuż używa mieszania alfa i nie zapisuje głębi, a po rysowaniu przywraca stan mieszania, maskę głębi i uniform brzegu
- środek kałuży przepuszcza grunt: krycie `mix(0,7; 1; F)`, czyli przy ustawieniach startowych 0,85 prosto w dół. Wychodzi to **poza listę właściciela** i było wyborem autora poprawek (żeby nie wyglądała jak czarna dziura)
- kolor wody z `(0,07; 0,09; 0,11)` na `(0,32; 0,40; 0,50)`, `Reflectivity` startowe z 0,35 na 0,5
- usunięte: `puddleWaterLevel`, `PUDDLE_DEPTH`, `puddleModelMatrix`. Test na prawdziwej mapie wysokości mierzy, jak blisko wody podchodzi grunt między wierzchołkami: najmniej 7,56 mm przy skali 1,0 i 6,89 mm przy 2,5 dla 13 kałuż, 7,35 mm i 6,37 mm dla największej kałuży w dziewięciu miejscach każdej komórki (grunt zbliża się najwyżej o 0,44 i 1,11 mm oraz 0,65 i 1,63 mm), i wymaga ponad 6 mm z 8. Siatka terenu ma 0,5 m, a odstęp wierzchołków kałuży najwyżej 7,5 cm; nie ma `glPolygonOffset`
- `drawHud` dostaje `panelsVisible` (przy schowanych panelach wysokość rzędów pasków to 0); ramka minimapy ma 1 piksel (2 przy 1440p) w kolorze `(0,30; 0,36; 0,48)`; panel Environment ma `Crystals` i `Puddles` w dwóch kolumnach

Dowody tych poprawek, osobno:

- **Zgłoszone przez bramkę** (koordynator, scalone drzewo): `make check` przechodzi, **467 przypadków testowych i 158006 asercji** (przed poprawkami 466 i 152264). Nowy jest jeden przypadek w `tests/PuddleTests.cpp` (21 zamiast 20, policzone z pliku), a liczby asercji zmieniły się także w przerobionych testach kałuż, więc różnicy 5742 asercji nie przypisuję jednemu przypadkowi. Start programu Debug przez 8 sekund: 31 linii logu, pusty strumień błędów, żadnej linii błędu
- **Widziane na zrzucie ekranu przez agenta, nie przez właściciela** (autor poprawek, build jego worktree **przed scaleniem** z kodem dźwigni i kartek): wszystkie 13 kałuż z góry całe, okrągłe i miękkie przy skali 1,0, a przy 2,5 całe i nic nie wystaje; z wysokości chodzenia przy włączonej latarce w 2 m szaroniebieska mokra plama z kamykami widocznymi przez wodę, w 4 m mała niebieskawa plama z błyskiem gwiazdy, w 8 m kałuża nie do odróżnienia; przy wyłączonej latarce w cieniu księżyca tylko błysk gwiazdy w 2 m i 4 m; w świetle księżyca tarcza księżyca i gwiazdy w kałuży o miękkim brzegu; Blinn-Phong, Phong, Gouraud i Unlit pokazują miękką plamę; w widoku `Normals as colour` kałuża jest niewidoczna (pozioma normalna, ten sam kolor co płaski grunt), w `UVs as colour` to pełna 32-kątna tarcza, bo program `textured` zapisuje alfę 1; udział 0, kałuże wyłączone i niebo wyłączone bez awarii; HUD przy górnej krawędzi przy schowanych panelach i pod pięcioma rzędami pasków przy widocznych; ramka minimapy cienka i wyciszona przy starcie rundy; panel Environment pokazuje wszystkie swoje linie. Koordynator obejrzał zestawienie trzynastu kałuż z góry przed i po i to potwierdza
- **Nadal otwarte lub niewidziane:** kałuże bez latarki w zacienionych korytarzach i dalej niż około 4 m z latarką pozostają **trudne do zobaczenia**; widoki chodzenia w świetle księżyca w 2, 4 i 8 m; `Share of cells` 0,5; nowe tooltipy; ścieżka mieszania w buildzie Debug w ruchu; liczba klatek po zmianie; różnica Fresnela

**Odczyty liczby klatek** (jedna sesja, Release, 1280 x 720, panele ukryte, widok korytarza; synchronizacja pionowa nie była aktywna na tej maszynie, choć `Window.cpp` o nią prosi; liczby nieco zaniżone przez hak testowy). To **odczyty, nie pomiary**: wszystko włączone około 1250; cienie latarki wyłączone około 1370 do 1410; minimapa wyłączona około 1250 do 1270; environment mapping wyłączony około 1240 do 1270; cienie księżyca wyłączone około 1490 (jeden odczyt); oba cienie, minimapa i environment wyłączone około 1920 (jeden odczyt); build Debug około 578. Odpowiadają tylko z grubsza na otwarte punkty o koszcie liczby klatek (sekcje 20.2, 21.2, 22.2 i 23.2): wyłączenie environment mapping i minimapy mieści się w rozrzucie odczytu. Nie wyjaśniają one spadku liczby klatek zapisanego w [`m7-status.md`](m7-status.md), sekcja 5, i nie porównywano ich z wcześniejszymi commitami.

**Nie sprawdzono w ogóle:** suwaki przeciągane i wpisywane w panelach, tooltipy, `Reload shaders`, zmiana rozmiaru okna, maksymalizacja, 1440p, minimalizacja, przeciąganie `Yaw` i `Pitch` księżyca, `Beam range` 6 i 60, `Converge at`, migotanie przy niskiej baterii, skala wysokości 2,5 na kałużach przed poprawką.

### 23.2. Otwarte: test ręczny na około czterdzieści minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu, z testów i ze wzorów,
nie z klikania ani ze zrzutów ekranu. Nazwy widżetów są zapisane tak jak w
`src/debug/panels/EnvironmentPanel.cpp`. Ustawień efektu program nigdzie nie zapisuje, więc
ponowne uruchomienie gry przywraca wartości startowe. Listy 17.2 do 22.2 też są otwarte. **Punkty, które agent widział na zrzutach (sekcja 23.1), zostają `[ ]`:** oglądanie przez agenta nie zastępuje testu właściciela.

Przygotowanie:

- [ ] usunąć stary plik `imgui.ini`, uruchomić grę z terminala. Oczekiwane w konsoli: żadnej
      linii `[error]`, w szczególności o `reflect.vert`, `reflect.frag` ani o niekompletnym
      framebufferze. W buildzie Debug żadnej linii `GL_...`
- [ ] okno debug (Diagnostics / Frame and shaders / Shaders) ma czternaście linii, ostatnia dla programu `reflect`, z `OK`
- [ ] panel **Environment** jest trzynasty, zwinięty w piątym rzędzie pasków tytułowych pod panelem [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      Shadows, a pasek HUD stoi (przy widocznych panelach) o jeden rząd niżej niż przed tą częścią. Po rozwinięciu panel ma dziewięć kontrolek w dwóch grupach (`Crystals`, `Puddles`), od poprawek z 2026-10-06 **w dwóch kolumnach**, i linię `Puddles: 13` pod `Fresnel`: wszystkie bez przewijania (w wersji z `9a33f18` dwie ostatnie linie były schowane). Żaden panel nie jest zasłonięty (zapisać, jeśli któryś jest)
- [ ] wartości startowe z tabeli powyżej

Obraz bez zmian i podstawowy wygląd (tryb Blinn-Phong, `Textured`, niebo włączone):

- [ ] **efekt wyłączony = obraz sprzed M8.** Odznaczyć `Environment mapping`: kryształy wyglądają
      jak przed tą częścią (świecą, pulsują), kałuż nie ma. Zaznaczyć z powrotem
- [ ] **kryształ z efektem.** Stanąć kilka metrów od kryształu. Zapisać, czy widać niebo na jego
      ścianach i czy kryształ nadal świeci i ma poświatę bloomu
- [ ] **niebo na kryształach wyraźnie.** `Glow` na 0 i `Sky share` na 1: kryształ pokazuje samo
      niebo (gwiazdy, w odpowiednim kierunku księżyc). Zapisać, co widać. Wrócić do 1,00 i 0,50
- [ ] `Refract / reflect` na 0, 0,5 i 1: trzy różne obrazy. Zapisać różnicę
- [ ] **część oświetlona kryształu.** Porównać kryształ przy `Sky share` 0 i 0,5 (przy `Glow` 1):
      zapisać, czy przy 0,5 jest widocznie ciemniejszy
- [ ] **poświata bloomu.** Przy `Glow` 1 poświata wokół kryształu jest taka jak przed tą częścią.
      Przy `Glow` 0 halo znika. Wrócić do 1,00
- [ ] **brak paralaksy.** Odejść od kryształu: gwiazdy w odbiciu nie zmieniają kierunku wraz z
      odległością. Obracać się wokół kryształu: odbite niebo obraca się razem ze światem (a nie z
      kamerą)

Załamanie i całkowite odbicie:

- [ ] `Refraction ratio` na 1,00: promień nie jest zginany. Na 0,41: najsilniejsze zgięcie. Zapisać
      różnice
- [ ] `Refraction ratio` na 1,50 przy `Refract / reflect` 0: płaskie promienie wracają jako
      odbicie, nie jako czarne ani losowe plamy. Zapisać, gdzie na krysztale to widać. Wrócić do
      0,67

Kałuże:

- [ ] `Puddles: 13` w labiryncie startowym. Odnaleźć kałuże, zapisać, ile udało się znaleźć w
      jednym korytarzu
- [ ] **kałuża nie dotyka ściany ani słupka**, nie leży w komórce startu, wyjścia ani pod
      kryształem
- [ ] **księżyc w kałuży.** Okno debug (Player / View), `Yaw` 205 i `Pitch` -50 (sprawdzić znak i zakres): kałuża około 1,4 m przed graczem powinna pokazywać tarczę księżyca. Zapisać, czy tak jest (agent widział tarczę i gwiazdy w kałuży przy tych ustawieniach, w wersji z `9a33f18`)
- [ ] **Fresnel.** Odznaczyć `Fresnel`: dalekie kałuże słabną względem bliskich. Zaznaczyć: dalekie
      mocniej odbijają
- [ ] **prawdziwa woda.** `Reflectivity` na 0,02 przy zaznaczonym `Fresnel`: kałuża w następnej
      komórce prawie niewidoczna. Zapisać, w jakiej odległości zaczyna coś odbijać. Wrócić do 0,50
- [ ] **kałuża w cieniu.** Stanąć tam, gdzie cień księżyca albo latarki pada na kałużę: odbłysk
      znika, odbite niebo nie ciemnieje
- [ ] **odległe kałuże we mgle.** Zapisać, czy dalekie kałuże, w których Fresnel odbija najwięcej, [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      giną razem z odbiciem w mgle. (Okno debug (Post process), zakładka `Fog and vignette`)
- [ ] **kępka trawy w kałuży.** Przejrzeć kałuże przy ścianach: zapisać, czy źdźbła wystają z wody
- [ ] odznaczyć `Puddles`: kałuże znikają, kryształy nadal pokazują niebo. Zaznaczyć. Linia
      `Puddles: 13` nie zmienia się przy odznaczeniu pola

Udział i ponowne rozmieszczenie:

- [ ] `Share of cells` na 0: zero kałuż, linia `Puddles: 0`. Na 0,50: `Puddles: 43` (policzone).
      Powolne przeciągnięcie z 0,15 w górę dodaje kałuże, **żadna z istniejących nie zmienia
      miejsca ani rozmiaru**. Wrócić do 0,15
- [ ] nowy labirynt (okno debug (World / Maze), `Regenerate`): kałuże są w innych komórkach, `Puddles: N` zgadza
      się z `lround(wolne komórki * 0,15)`. Ta sama wartość ziarna daje te same kałuże

Teren i woda:

- [ ] **skala wysokości.** Okno debug (World / Terrain and grass / Terrain), `Height scale` od 0 do 2,5: kałuże idą w górę i w dół z gruntem, nie przeskakują do innych komórek. **Żadna kałuża nie jest obcięta prostą cięciwą ani nie wisi nad niższym gruntem**, także przy 2,5, i nic nie wystaje spod wody. Zapisać, czy brzeg wygląda jak brzeg wody (agent widział 13 kałuż z góry całych przy 1,0 i 2,5, nie z wysokości chodzenia przy 2,5)
- [ ] **brzeg i migotanie.** Na styku kałuży i gruntu: zapisać, czy brzeg migocze i czy z bliska widać fasetki warstwy (warstwa 8 mm nad gruntem, płaska między wierzchołkami)
- [ ] **miękki brzeg i przezroczysty środek.** Patrząc z góry (noclip, klawisz N, w dół): brzeg kałuży zanika bez widocznej linii, a przez środek prześwituje grunt. W widoku `UVs as colour` ta sama kałuża jest pełną 32-kątną tarczą (program `textured` zapisuje alfę 1)
- [ ] **wygląd z wysokości chodzenia.** W 2, 4 i 8 m, z latarką i bez, w świetle księżyca i w cieniu: zapisać, w jakiej odległości kałuża przestaje być widoczna. Wersja po poprawkach według agenta: w 2 m plama z kamykami, w 4 m mała niebieskawa plama z błyskiem, w 8 m nie do odróżnienia; bez latarki w cieniu tylko błysk gwiazdy. Zapisać, czy właściciel uważa to za wystarczające
- [ ] `Share of cells` 0,5: ścieżka mieszania i wygląd 43 kałuż w ruchu, w buildzie Debug bez `GL_...` w konsoli
- [ ] nowe tooltipy: `Puddles` ("A puddle is a thin film of water that follows the ground..."), `Reflectivity` ("...The rest is the water and the ground that shows through it.") i `Fresnel` (o kryciu gruntu)

Tryby oświetlenia i widoki (tu nikt jeszcze nic nie uruchomił):

- [ ] **Phong.** Lista `Lighting`: `Phong`. Kryształy i kałuże: odbłysk Phonga zamiast Blinna-Phonga,
      bez błędów w konsoli
- [ ] **Gouraud.** `Gouraud`: ściany mają światło na wierzchołek, a kryształy i kałuże wyglądają jak
      w Phongu (światło na fragment). Zapisać, że przełączenie nie zmienia wyglądu kryształów
- [ ] **Unlit.** `Unlit`: kryształy i kałuże pokazane z pełną jasnością, niebo nadal domieszane.
      Zapisać wygląd
- [ ] **widoki diagnostyczne.** Okno debug (Render / Textures and normals), `View mode`: `Normals as colour` i `UVs as colour`:
      kryształy są rysowane jak ściany (kolor z danych), kałuże też (kolor normalnej `(0,5, 1, 0,5)`
      albo współrzędnej tekstury), a obraz nie ma niczego z nieba na kryształach. Wrócić do `Textured`
- [ ] **niebo wyłączone.** Okno debug (Render / Scene), odznaczyć `Skybox`: kryształ i kałuża pokazują kolor
      czyszczenia w każdym kierunku (nie czerń). Zaznaczyć z powrotem
- [ ] **efekt wyłączony w widoku diagnostycznym.** Odznaczyć `Environment mapping` w widoku
      `Normals as colour`: kałuże znikają

Shader na żywo:

- [ ] **`Reload shaders`.** Przycisk w oknie debug (Diagnostics / Frame and shaders / Shaders): czternaście linii z `OK`. Zmienić w `reflect.frag`
      wzór koloru, `copy_assets`, `Reload shaders`: obraz kryształów się zmienia bez restartu.
      Wprowadzić celowy błąd składni: linia `reflect` pokazuje błąd, kryształy wracają do programu
      ścian (a kałuże nie są rysowane w widoku `Textured`). Wycofać zmianę

Okno i granice:

- [ ] zmiana rozmiaru okna do bardzo małego i minimalizacja: bez `GL_...` w konsoli i bez zmiany [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
      obrazu po powrocie. Pasek HUD pod pięcioma rzędami pasków tytułowych (przy widocznych panelach)
- [ ] **HUD przy schowanych panelach** (klawisz tyldy): pasek stoi przy górnej krawędzi okna, a nie nad środkiem obrazu, i wraca pod pięć rzędów pasków po przywróceniu paneli (decyzja właściciela z 2026-10-06; agent widział to na zrzutach). Zapisać, czy przeskok przy klawiszu przeszkadza [od 2026-10-06 bez odpowiednika w oknie debug, patrz sekcja 27.3]
- [ ] **noclip (klawisz N)**: lot nad kałużami i kryształami z góry. Fresnel przy patrzeniu w dół:
      kałuża pokazuje głównie jasną szaroniebieską wodę z gruntem przez nią (`F` około 0,5 przy `Reflectivity` 0,5, krycie około 0,85), a nie lustro
- [ ] **brak ostrzeżeń kompilatora** przy buildzie Debug i Release po tej części

Sprawdziwszy wszystko: zaznaczyć wyniki tutaj, wpisać zapisane obserwacje, i dopiero wtedy
zmienić stan tematu 12 w [`../syllabus.md`](../syllabus.md) z "w toku".

## 24. Lista kontrolna M8, część 2: selekcja, dźwignie i kartki

Druga część kamienia milowego M8 podpina promień z "M8, podstawy bez okna" do działającej gry
(temat 15 wykładu, selekcja obiektów). W każdej klatce program rzuca jeden promień z oka gracza:
przez środek obrazu, gdy mysz jest przechwycona, albo przez kursor, gdy jest wolna. Dźwignia na
ścianie, którą promień trafia w zasięgu 2,5 m, jest podświetlona pulsującym świeceniem, a klawisz
E albo lewy przycisk myszy ją pociąga: jedna wewnętrzna ściana opada w ziemię w 1,5 s i
przestaje blokować gracza od chwili pociągnięcia. Kartka na ścianie otwiera kartę HUD z krótką
podpowiedzią. Doszły: `src/game/Interaction.*` (wynik wskazywania w klatce, reakcja na klawisz,
macierze modeli, podświetlenie), `src/game/InteractableRenderer.*` (rysowanie dźwigni i kartek),
`tests/InteractionTests.cpp` (21 przypadków), trzy modele (`lever.obj`, `lever_handle.obj`,
`note.obj`), tekstury `lever_iron`, `lever_brass` i `note_paper` (każda z mapą normalnych) oraz
skrypty `tools/blender/build_lever.py` i `build_note.py`. Zmieniły się: `Input.*` (odczyt
pozycji kursora), `MazeWorld.*`, `Round.*`, `Minimap.*`, `MazeRenderer.*`, `ColliderLines.*`,
`NightMazeApp.*`, `DebugContext.hpp` (47 pól w tej części, doszły `pick` i `pickDebug`; od M9, części 1 jest ich 49), `DebugUI.cpp`,
`Hud.*`, `Theme.hpp`, panele Collision, Gameplay i Maze, `main.cpp`, `CMakeLists.txt`,
`make_all.py` i `make_textures.py`. Liczba programów shaderów (14) i paneli (13) się nie zmieniła.
Opis kodu: [`../modules/scene/picking.md`](../modules/scene/picking.md) i
[`../modules/game/interactables.md`](../modules/game/interactables.md).

Temat 15 wykładu jest **w toku**, nie zaliczony: test ręczny właściciela (sekcja 24.2) jest
otwarty, a na macOS kod nie był budowany ([`build-macos.md`](build-macos.md)). M8 jako całość nie
jest zamknięty.

### 24.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru nie
zapisano w tym dokumencie. Poniższe punkty są **zgłoszone przez autora kodu**, nie powtarzałem
ich przy pisaniu tej sekcji.

Bramka i start:

- [x] `make check` przechodzi w Debug i Release (zgłoszone)
- [x] `night_maze_tests.exe`: **466 przypadków testowych i 152264 asercji** w obu konfiguracjach.
      Przed tą częścią 445 i 150296. Nowych przypadków jest 21 i wszystkie są w
      `tests/InteractionTests.cpp` (policzone z pliku: 21 makr `TEST_CASE`), a
      `tests/InteractablesTests.cpp` zmienił tylko komentarz
- [x] start programu Debug z czystym logiem (bez linii `[error]`), w którym trzy nowe modele
      (`lever.obj`, `lever_handle.obj`, `note.obj`) i nowe tekstury są wypisane jako wczytane

**Widziane na zrzutach ekranu przez agenta.** Poniższe punkty są zapisane jako "widziane na
zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela": agent, który napisał kod,
uruchomił grę, sterował nią skryptem i oglądał zrzuty ekranu. To **nie jest** test ręczny
właściciela i nie zamyka żadnego punktu z sekcji 24.2.

- widziane na zrzucie ekranu przez agenta: płytka dźwigni na ścianie, wokół środka obrazu
  pierścień celownika i podpowiedź "E: pull lever"
- widziane na zrzucie ekranu przez agenta: po E rączka opuszczona, a podpowiedź zniknęła
- widziane na zrzucie ekranu przez agenta: podświetlenie pulsuje (mocniejsze przy wyłączonej
  latarce)
- widziane na zrzucie ekranu przez agenta: kartka z liniami atramentu, jej karta z podpowiedzią i
  napisem "E: close", a po odejściu od kartki karta zniknęła
- widziane na zrzucie ekranu przez agenta: ściana się otworzyła, minimapa przestała rysować tę
  ścianę i pokazała korytarz za nią, a gracz przeszedł przez otwór
- widziane na zrzucie ekranu przez agenta: po R minimapa wróciła do stanu początkowego, a ściana
  znowu blokowała
- widziane na zrzucie ekranu przez agenta: zamrożony promień jako zielona linia i trafione
  pudełko na zielono
- widziane na zrzucie ekranu przez agenta: panel Collision z początkiem, kierunkiem i linią
  "Hit: lever 0 at 0.72 m"
- widziane na zrzucie ekranu przez agenta: kliknięcie wolnym kursorem w dźwignię ją pociągnęło

Po przeróbce modelu dźwigni agent widział też (zrzuty z 2026-10-06, nie właściciel): dźwignię od przodu z 1 m przy włączonej latarce, w górnym położeniu i podświetloną (ciemna płyta, jasna bursztynowa gałka nad środkiem, podpowiedź na dole pośrodku); ten sam widok po E (pręt w dół, gałka poniżej krawędzi płyty z cieniem na ścianie, bez podświetlenia i podpowiedzi); widoki z boku w górnym i dolnym położeniu; wyłączoną latarkę (pociągnięta: płyta prawie czarna, gałka ciemna ochra; w górze z podświetleniem: gałka świeci bursztynem); około 2,4 m pod kątem z podświetleniem i podpowiedzią oraz około 2,9 m poza zasięgiem bez podświetlenia; drugą dźwignię; Gouraud (z podświetleniem) i Unlit; widok Normals (dźwignia jako dane, bez podświetlenia, pierścień i podpowiedź nadal są); kartkę z "E: read note" na dole, wolną od arkusza; otwartą kartę kartki bez podpowiedzi i bez nakładania się na minimapę.

**Nie widziane na ekranie** (nikt tego nie oglądał): uchwyt w połowie ruchu, cień księżyca od dźwigni, Gouraud i Unlit dla kartki, widok na wprost z 2,5 m, dodatki w panelu Gameplay i przycisk "Pull all levers", inne niż domyślne liczby dźwigni i kartek, czysta rama połowicznie zatopionej ściany.

**Znane problemy wyglądu w chwili pisania (otwarte):**

- model dźwigni czytał się z przodu jak szara płyta i został przerobiony: płyta 0,16 x 0,30 m i 0,02 m grubości z podniesioną ramą z czterech listew po 0,02 m i obudową osi wystającą na 0,07 m (52 trójkąty, materiał `lever_iron`, ciemne żelazo), uchwyt z prętem 0,03 x 0,03 m i 0,14 m długości oraz gałką 0,07 x 0,07 m kończącą się 0,20 m od osi (20 trójkątów, materiał `lever_brass`, mosiądz); zrzuty po przeróbce są na liście wyżej. Znane drobne uwagi kosmetyczne: patrząc na wprost z 1 m, gałka w górnym położeniu zasłania górną trzecią część płyty, a przy wyłączonej latarce i bez podświetlenia płyta jest prawie czarna na tle ściany i niesie ją tylko gałka.
- podpowiedź "E: pull lever" zasłaniała wskazany obiekt i została przesunięta: podpowiedź stoi na dole okna, pośrodku (`PROMPT_PLACE` = (0,5; 0,9) okna), a nie pod celownikiem, więc nie zasłania wskazanego obiektu.

Znane ograniczenia (z kodu, nie ze zrzutów): kępki trawy przy otwartej ścianie zostają, a słupek,
który kończył tylko otwartą ścianę, stoi sam.

### 24.2. Otwarte: test ręczny na około trzydzieści pięć minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu i z testów. Pierwsze
grupy powtarzają to, co agent widział na zrzutach: to ma zobaczyć właściciel.

Przygotowanie:

- [ ] usunąć stary `imgui.ini`, uruchomić grę z terminala: żadnej linii `[error]`, w buildzie
      Debug żadnej linii `GL_...`, trzy nowe modele i nowe tekstury wypisane jako wczytane
- [ ] okno debug (World / Maze) ma dwa suwaki `Levers` i `Notes` (od 0 do 16) i linię "Levers: N, notes: M" z
      liczbami, które labirynt naprawdę dostał (domyślnie żądane 2 i 3)
- [ ] okno debug (Gameplay) ma linie "Levers: N pulled of M" i "Note card: open/closed" oraz przycisk
      `Pull all levers`

Dźwignia i kartka (to, co widział agent):

- [ ] dźwignia na ścianie jest widoczna, a przy mierzeniu w nią środkiem obrazu z mniej niż 2,5 m
      celownik dostaje pierścień i pojawia się "E: pull lever". Z dalej niż 2,5 m nie
- [ ] wygląd dźwigni po przeróbce (ciemna płyta z ramą, mosiężna gałka): zapisać, czy rączka i płytka
      dają się odróżnić od ściany i od siebie
- [ ] miejsce podpowiedzi (na dole okna, pośrodku): zapisać, czy podpowiedź nie
      zasłania wskazanego obiektu
- [ ] podświetlenie pulsuje, przy wyłączonej latarce (klawisz F) jest mocniejsze
- [ ] E albo lewy przycisk myszy pociąga dźwignię: rączka opada w około 0,3 s, podpowiedź znika,
      podświetlenie znika
- [ ] ściana dźwigni opada w około 1,5 s, minimapa przestaje ją rysować i pokazuje korytarz za
      nią, a gracz przechodzi przez otwór
- [ ] pociągnięta dźwignia nie reaguje drugi raz (brak podpowiedzi, brak podświetlenia)
- [ ] kartka: pierścień, "E: read note", E otwiera kartę z napisem "A note on the wall",
      tekstem i "E: close"
- [ ] karta kartki zamyka się: klawiszem E, kliknięciem, po odejściu dalej niż 3,0 m od kartki,
      po wygranej, a także przy restarcie (R) i regeneracji labiryntu
- [ ] klawisz R przywraca wszystkie ściany: minimapa wraca do stanu początkowego, ściana znowu
      blokuje, dźwignie są znowu do pociągnięcia

Zamrożony promień i okno debug (Diagnostics / Collision and picking) (to, co widział agent):

- [ ] pole `Draw pick boxes and ray` rysuje czerwone pudełka dźwigni, białe pudełka kartek i
      promień, a trafione pudełko na zielono
- [ ] pole `Freeze the drawn ray` zatrzymuje rysowany promień (po kroku w bok widać go z boku jako
      zieloną linię), a restart rundy go czyści
- [ ] okno debug (Diagnostics / Collision and picking) pokazuje "Ray through", początek, kierunek i linię w rodzaju
      "Hit: lever 0 at 0.72 m", a liczba pudełek ścian maleje po pociągnięciu dźwigni
- [ ] kliknięcie wolnym kursorem w dźwignię ją pociąga. Kliknięcie wolnym kursorem gdzie indziej
      w scenie przechwytuje kursor. Przy otwartej karcie kliknięcie ją zamyka i nie przechwytuje
- [ ] klawisz E działa też przy wolnym kursorze (na to, na co wskazuje kursor)
- [ ] kliknięcie w panel debugowania nie dociera do gry (nie pociąga ani nie przechwytuje)
- [ ] kursor poza oknem nie wskazuje niczego, zmiana rozmiaru okna i zminimalizowanie okna nie
      psują wskazywania ani nie wypisują błędu

To, czego agent nie widział:

- [ ] tryby `Gouraud` i `Unlit`: dźwignia, kartka i podświetlenie rysują się, ściany opadają
- [ ] widoki `Normals as colour` i `UVs as colour`: dźwignia i kartka rysują się, podświetlenia
      w nich nie ma (pokazują dane)
- [ ] cienie księżyca rzucane przez dźwignię i kartkę na ścianę; opadająca ściana rzuca cień
      zgodny z tym, co jeszcze wystaje z ziemi
- [ ] przycisk `Pull all levers` otwiera wszystkie ściany naraz (linia "Levers" je liczy), a
      restart je zamyka
- [ ] druga dźwignia: pociągnięcie jednej nie rusza ściany drugiej
- [ ] pola `Levers` i `Notes` z innymi liczbami (0, 1, 16) i `Regenerate`: labirynt dostaje tyle,
      ile się da (może mniej), a po 0 nie ma nic do wskazania
- [ ] klatka z ramą ściany zatopioną do połowy: zapisać, czy nie ma migotania ani szczeliny

Granice i szczególne przypadki:

- [ ] wejście w ścianę w czasie jej 1,5 s opadania: gracz przechodzi przez ścianę, która jest
      jeszcze widoczna (ściana przestaje blokować w chwili pociągnięcia, jak brama)
- [ ] w otwartym przejściu stoi sam słupek i zostają kępki trawy (znane ograniczenie): zapisać,
      czy to przeszkadza
- [ ] podpowiedzi kartek o kryształach po zebraniu kryształów: tekst liczy tylko kryształy, które
      zostały, a po zebraniu wszystkich mówi "No crystal is left to find."
- [ ] minimapa: małe kwadraty dźwigni (pociągnięta ciemniejsza) i kartek w odkrytych komórkach
- [ ] plan w oknie debug (World / Maze): kwadraty dźwigni i kartek, otwarta ściana narysowana przygaszona
- [ ] labirynt 40 x 40: pociągnięcie dźwigni, minimapa i plan działają, brak spadku płynności
- [ ] brak ostrzeżeń kompilatora przy buildzie Debug i Release po tej części

Sprawdziwszy wszystko: zaznaczyć wyniki tutaj, wpisać zapisane obserwacje, i dopiero wtedy
zmienić stan tematu 15 w [`../syllabus.md`](../syllabus.md) z "w toku".

## 25. Lista kontrolna M9, część 1: kamera menu

Pierwsza część kamienia milowego M9 dodaje tryb, w którym gra pokazuje samą siebie: HUD, minimapa i
panele są schowane, a kamera sama jedzie przez labirynt. Włącza go klawisz F2, pole `Menu camera (F2)`
w panelu Camera albo przełącznik `--menu-camera`. Są dwa ujęcia (spacer po korytarzach z latarką i
wysoki przelot nad labiryntem), a przełączniki wiersza poleceń to `--menu-camera`, `--seed <n>`,
`--menu-shot <walk|glide>` i `--menu-time <sekundy>`. Doszły: `src/game/MenuCamera.*`,
`src/game/StartOptions.*`, `tests/MenuCameraTests.cpp` (23 przypadki) i `tests/StartOptionsTests.cpp`
(7 przypadków). Zmieniły się: `CMakeLists.txt`, `src/main.cpp`, `NightMazeApp.*`,
`DebugContext.hpp` (49 pól, doszły `menuCamera` i `menuCameraLoopSeconds`), `DebugUI.*` i
`panels/CameraPanel.*`. Liczba programów shaderów (14) i paneli (13) się nie zmieniła. Opis kodu:
[`../modules/game/menu-camera.md`](../modules/game/menu-camera.md).

Decyzje właściciela (2026-10-06) są osobno: menu powstanie w RmlUi, **tłem menu będzie zmontowana,
wyrenderowana wcześniej pętla wideo**, a przełączniki `--menu-shot` i `--menu-time` zostają, żeby
można było nagrywać klipy ponownie ([`../decisions/menu-background-prerendered-loop.md`](../decisions/menu-background-prerendered-loop.md)).
Wszystko inne w tym trybie (prędkość 0,7 m/s, wysokość 1,5 m, klawisz F2, zamrożenie rundy, jedna
zamknięta pętla przez wszystkie kryształy) to wybory wykonawcze.

Tej części **nie zamyka** żaden test ręczny: lista w sekcji 25.2 jest otwarta, a na macOS kod nie był
budowany ([`build-macos.md`](build-macos.md)). M9 jako całość dopiero się zaczął (w chwili tej części nie
było jeszcze menu, RmlUi ani stanów gry: stan z 2026-10-06, M9, część 2, jest w sekcji 26. Punkty 25.2 o starcie bez
przełączników i o kursorze po wyłączeniu trybu opisują program sprzed części 2: dziś gra startuje w menu głównym, F2
włącza kamerę menu tylko w rundzie, a po wyłączeniu trybu kursor jest znów przechwytywany).

### 25.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora, karty graficznej ani sterownika dla tego pomiaru nie
zapisano w tym dokumencie. Poniższe punkty są **zgłoszone przez bramkę i autora kodu**, nie powtarzałem
ich przy pisaniu tej sekcji.

Bramka:

- [x] `make check` przechodzi (zgłoszone)
- [x] `night_maze_tests.exe`: **497 przypadków testowych i 219050 asercji** (przed tą częścią 467 i
      158006). Nowych przypadków jest 30 i wszystkie są w nowych plikach (policzone z plików: 23
      makr `TEST_CASE` w `tests/MenuCameraTests.cpp` i 7 w `tests/StartOptionsTests.cpp`, 467 + 30 =
      497). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona

**Widziane na zrzutach ekranu przez agenta.** Poniższe punkty są zapisane jako "widziane na zrzucie
ekranu przez agenta (2026-10-06), nie przez właściciela": agent, który napisał kod, uruchomił grę z
przełącznikiem `--menu-camera`, nagrał ją `ffmpeg` i obejrzał wyciągnięte klatki. To **nie jest** test
ręczny właściciela i nie zamyka żadnego punktu z sekcji 25.2.

- widziane na zrzucie ekranu przez agenta: kamera zostaje we wnętrzu korytarzy, bez HUD, minimapy
  i paneli
- widziane na zrzucie ekranu przez agenta: żadna ściana nie jest przecięta, ruch między kolejnymi
  klatkami jest płynny
- widziane na zrzucie ekranu przez agenta: mniej więcej połowa spaceru to dobre widoki w głąb
  korytarza, a reszta to ściany z bliska w ciasnych zakrętach i w miejscach, gdzie kamera zawraca
  przy celu
- widziane na zrzucie ekranu przez agenta: kamera mija kryształy w przelotowym korytarzu w
  odległości około 20 cm z boku i tuż nad nimi

**Nie widziane na ekranie** (nikt tego nie oglądał): klawisz F2 (każde uruchomienie agenta było z
przełącznikiem), kontrolki grupy `Menu camera` w panelu Camera, wysoki przelot w całej pętli, powrót
paneli po wyłączeniu trybu, kryształy kołyszące się w trybie, klawisz tyldy w trakcie trybu i
wszystkie punkty z sekcji 25.2.

**Materiał porównawczy poza repozytorium** (zgłoszone przez autora, nie powtarzałem, nie ma tych
plików w projekcie i nie ma na nie odnośników): dwa nagrania pokazane właścicielowi w makiecie
projektowej, ciągłe ujęcie 30 s (9,5 MB) i zmontowana pętla 24 s z czterech ujęć z przenikaniami
(10,6 MB w 720p30; ta sama pętla w jakości docelowej CRF 14 miała 19,7 MB). Po ich obejrzeniu
właściciel zdecydował, że tłem menu będzie zmontowana pętla.

Znane ograniczenia (z kodu, nie ze zrzutów): zmiana prędkości suwakiem w trakcie przeskakuje kamerę
(pozycja to `(sekundy + przesunięcie) * prędkość`), zmiana ujęcia też, brama i ściany po dźwigniach
zatrzymują się w połowie opadania na czas trybu, a w pierwszej zmontowanej pętli brakuje kałuż i bramy.

### 25.2. Otwarte: test ręczny na około trzydzieści minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu i z testów.

Przygotowanie:

- [ ] usunąć stary `imgui.ini`, uruchomić grę z terminala bez przełączników: gra startuje jak
      zawsze (HUD, minimapa i panele widoczne), żadnej linii `[error]`
- [ ] okno debug (Player / Menu camera) ma na dole grupę `Menu camera`: pole `Menu camera (F2)`, listę `Shot`, suwaki
      `Speed` (domyślnie 0,70 m/s) i `Eye height` (domyślnie 1,50 m), pole `Time offset` (0,0 s) i
      linię `One loop: N s`

F2 w trakcie rundy:

- [ ] F2: HUD, minimapa i panele znikają, kamera sama jedzie po korytarzach, a w logu jest linia
      `Menu camera on, one loop takes N s`
- [ ] runda stoi: licznik czasu i bateria (sprawdzić w oknie debug (Gameplay) po naciśnięciu tyldy) nie
      zmieniają się, a kryształy dalej się kołyszą i ich światła pulsują
- [ ] F2 jeszcze raz: HUD, minimapa i panele wracają dokładnie tak, jak były (także gdy przed F2
      panele były schowane tyldą: wtedy po F2 zostają schowane)
- [ ] po wyłączeniu trybu gracz patrzy tam, gdzie patrzył przed F2, a kursor jest wolny (trzeba
      kliknąć w scenę)
- [ ] w trakcie trybu klawisze R, N, F, M, E i mysz nic nie robią, a kliknięcie w scenę nie
      przechwytuje kursora
- [ ] klawisz tyldy w trakcie trybu pokazuje i chowa panele, żeby można było zmienić ustawienia
- [ ] F2 nie działa w trakcie edycji pola tekstowego w panelu (klawiatura zablokowana)

Przełączniki:

- [ ] `night_maze.exe --menu-camera`: gra startuje od razu w trybie
- [ ] `--seed 7` (bez trybu): inny labirynt niż domyślny, `--seed 4294967295` przyjęte,
      `--seed 4294967296`, `--seed -3` i `--seed abc` odrzucone
- [ ] `--menu-shot glide --menu-camera`: wysoki przelot, latarka wyłączona; `--menu-shot walk`:
      spacer, latarka włączona; `--menu-shot` samo, bez `--menu-camera`, nie włącza trybu
- [ ] `--menu-time 30 --menu-camera`: kamera startuje 30 s w głąb pętli (inne miejsce niż bez
      przełącznika); `--menu-time 12,5` (przecinek) jest odrzucone, `--menu-time 12.5` przyjęte
- [ ] zły przełącznik (`--fullscreen`, `--seed` bez wartości, `--menu-shot orbit`): dwie linie
      `[error]` (komunikat i lista przełączników), kod wyjścia niezerowy, okno się nie otwiera
- [ ] te same przełączniki dają dwa razy ten sam obraz (porównać dwa zrzuty po tej samej liczbie
      sekund)

Nagranie klipu (do tego służy tryb):

- [ ] nagrać kilka sekund okna gry (`ffmpeg` albo nagrywanie ekranu) z `--menu-camera --seed 1
      --menu-shot glide`, potem z `--seed 6 --menu-shot walk`: obraz bez HUD, minimapy i paneli,
      płynny, bez przeskoku po pełnej pętli. Zapisać, jak długo trwa pętla (`One loop`)
- [ ] ten sam klip nagrany drugi raz z tymi samymi przełącznikami jest taki sam co do klatki
- [ ] czy zmontowanie pętli z kilku ujęć (przenikania) jest możliwe z tych nagrań: zapisać uwagi

Ujęcia i ustawienia:

- [ ] oba ujęcia w panelu (lista `Shot`): przelot patrzy w dół na środek labiryntu, spacer na
      wysokości oczu; przełączenie przeskakuje kamerę (znane ograniczenie)
- [ ] suwak `Speed`: 0 zatrzymuje kamerę, 4 jedzie bardzo szybko; zmiana w trakcie przeskakuje
      (znane ograniczenie, zapisać jak bardzo)
- [ ] suwak `Eye height` od 0,3 do 2,8 m: spacer zmienia wysokość, przelot się nie zmienia
- [ ] pole `Time offset` (także ujemne i duże): ujęcie startuje w innym miejscu, bez przeskoku po
      zawinięciu
- [ ] pętla po pełnym obiegu: w oknie debug (World / Maze) ustawić mały labirynt (na przykład 3 na 3) i
      `Regenerate`, odczekać `One loop` sekund i sprawdzić, że w miejscu zamknięcia nie ma
      skoku obrazu ani obrotu
- [ ] ciasne zakręty i miejsca zawracania: zapisać, czy kamera nie przechodzi przez słupek ani ścianę
      i czy obrót widoku nie jest nagły
- [ ] kamera przy krysztale: mija go w odległości kilkunastu centymetrów, bez wejścia w kryształ
- [ ] spacer w pełnym labiryncie 10 na 10: zapisać wartość `One loop` (autor zgłasza około 500 s)

Granice i szczególne przypadki:

- [ ] labirynt 1 na 1 (okno debug (World / Maze), `Regenerate`): spacer to małe koło w środku komórki, bez błędu
      i bez `NaN`, przelot działa
- [ ] `Regenerate` w trakcie trybu: ścieżka dopasowuje się do nowego labiryntu, kamera nie
      przechodzi przez ściany
- [ ] zmiana skali wysokości terenu w trakcie trybu: kamera idzie za gruntem
- [ ] zmiana rozmiaru okna i zminimalizowanie okna w trakcie trybu nie psują obrazu ani nie
      wypisują błędu
- [ ] restart rundy z okna debug (Gameplay) w trakcie trybu: runda zaczyna się od nowa, tryb działa dalej
- [ ] brama albo ściana, która opadała w chwili F2: stoi zatrzymana w trakcie trybu i dokańcza po
      wyłączeniu (znane zachowanie, zapisać)
- [ ] brak ostrzeżeń kompilatora przy buildzie Debug i Release po tej części

Sprawdziwszy wszystko: zaznaczyć wyniki tutaj i wpisać zapisane obserwacje. Zamknięcie tej listy nie
zamyka M9.

## 26. Lista kontrolna M9, część 2: menu w RmlUi, ekrany gry i Escape

Druga część kamienia milowego M9 dodaje menu gry. Gra ma teraz **ekrany**: menu główne, rundę, pauzę
i ekran wyniku, i **startuje w menu głównym** (przełączniki `--play` i `--menu-camera` je pomijają).
Menu rysuje RmlUi (dokumenty `assets/ui/*.rml` i arkusz `menu.rcss`) na wierzchu klatki gry i minimapy,
a przed panelami Dear ImGui. Escape cofa o jeden ekran i **nigdy nie zamyka programu**: wyjście to przycisk
`Quit` w menu głównym. HUD jest tylko w rundzie, minimapa w rundzie i w pauzie. Doszły: `src/ui/*`
(`UiLayer`, `AssetFileInterface`), `src/core/Files.*` (`readBinaryFile` i `TEXT_FONT_FILE`, przeniesione z
`debug/Theme.cpp`), `src/game/GameState.*`, `assets/ui/*`, `tests/GameStateTests.cpp` (21 przypadków)
i jeden nowy przypadek w `tests/StartOptionsTests.cpp` (razem 8). Zmieniły się: `CMakeLists.txt` (nowa
biblioteka `ui`), `cmake/Dependencies.cmake` (RmlUi 6.3 i FreeType 2.14.3: dwie nowe zależności z
`FetchContent`, razem siedem, [`../libraries/rmlui.md`](../libraries/rmlui.md)), `THIRD-PARTY-NOTICES.txt`
i `launcher/scripts/build-notices.mjs`, `src/core/Application.*` (wirtualna `onEscapePressed`),
`NightMazeApp.*`, `StartOptions.*` (`--play`), `src/main.cpp` i w `src/debug/`: `Theme.cpp`,
`DebugContext.hpp` (50 pól, doszło `hudVisible`) i `DebugUI.cpp`. Liczba programów shaderów gry (14) i
paneli (13) się nie zmieniła: RmlUi kompiluje własne shadery we własnym rendererze. Opis kodu:
[`../modules/ui/README.md`](../modules/ui/README.md), [`../modules/game/game-states.md`](../modules/game/game-states.md),
[`../libraries/rmlui.md`](../libraries/rmlui.md).

Decyzje właściciela (2026-10-06) są osobno: menu w RmlUi ([`../decisions/menu-in-rmlui.md`](../decisions/menu-in-rmlui.md)),
Escape cofa o jeden ekran ([`../decisions/escape-pauses-and-goes-back.md`](../decisions/escape-pauses-and-goes-back.md)),
zakres menu w M9 ([`../decisions/menu-scope-for-m9.md`](../decisions/menu-scope-for-m9.md)) i odtwarzanie tła
przez dekodery systemu z nieruchomym obrazem na wypadek porażki
([`../decisions/video-through-os-decoders-with-still-fallback.md`](../decisions/video-through-os-decoders-with-still-fallback.md)).
Wszystko inne (kolejność klatki, warstwa `ui`, nazwy akcji w dokumentach) to wybory wykonawcze.

Tej części **nie zamyka** żaden test ręczny: lista w sekcji 26.2 jest otwarta, a na macOS kod nie był
budowany ([`build-macos.md`](build-macos.md)). Nie istnieje jeszcze: ekran ustawień, poziomy trudności z
liczbami i tło menu w postaci pętli wideo (menu główne pokazuje żywy wysoki przelot kamery menu).

### 26.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora ani sterownika dla bramki nie zapisano w tym dokumencie.

Bramka (zgłoszona przez bramkę na commicie `8c99911`, nie powtarzałem jej przy pisaniu tej sekcji):

- [x] `make check` przechodzi (zgłoszone)
- [x] `night_maze_tests.exe`: **519 przypadków testowych i 219195 asercji** (przed tą częścią 497 i
      219050). Nowych przypadków jest 22: 21 w `tests/GameStateTests.cpp` i 1 w
      `tests/StartOptionsTests.cpp` (policzone z plików: 497 + 22 = 519, a suma makr `TEST_CASE` w
      `tests/*.cpp` to 519). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona. Żaden z
      tych przypadków nie dotyka klas `ui::UiLayer` i `ui::AssetFileInterface`: wymagają okna i OpenGL

**Widziane na zrzutach ekranu przez agenta.** Poniższe punkty są zapisane jako "widziane na zrzucie
ekranu przez agenta (2026-10-06), nie przez właściciela": agent, który napisał kod, uruchomił grę (Windows,
Release, 1280 x 720, RTX 4070 Ti SUPER), sterował nią skryptem i oglądał zrzuty ekranu. To **nie jest** test
ręczny właściciela i nie zamyka żadnego punktu z sekcji 26.2.

- widziane na zrzucie ekranu przez agenta: menu główne po starcie, nad przelatującą kamerą, bez HUD i
  minimapy; przycisk `Play` pod kursorem robi się pomarańczowy
- widziane na zrzucie ekranu przez agenta: `Play` zaczyna rundę, z HUD i minimapą
- widziane na zrzucie ekranu przez agenta: Escape pokazuje pauzę, HUD znika, minimapa jest przyciemniona
  pod menu, a czas rundy stoi
- widziane na zrzucie ekranu przez agenta: klawisze F, M i R w pauzie nic nie robią
- widziane na zrzucie ekranu przez agenta: tylda nad pauzą pokazuje panele na wierzchu, suwak w panelu
  Maze da się przesunąć i żaden przycisk menu nie zadziałał
- widziane na zrzucie ekranu przez agenta: `Resume` przez Escape, `Restart` (czas wraca do 0:01),
  `Back to menu`, ponowne `Play` (labirynt zbudowany od nowa ze zmienioną liczbą kartek) i `Quit` (proces
  kończy się kodem 0)
- widziane na zrzucie ekranu przez agenta: tylda w trakcie rundy pokazuje panele
- widziane na zrzucie ekranu przez agenta: `--play` startuje od razu w rundzie; F2 włącza i wyłącza
  kamerę menu; `--menu-camera --menu-shot glide` startuje bez dokumentu, Escape pokazuje pauzę, drugi
  Escape ją chowa
- widziane na zrzucie ekranu przez agenta, ale **tylko przez tymczasową, niezatwierdzoną linię** w
  jednorazowym buildzie, która po trzech sekundach rundy ustawiała `RoundState::Won`: ekran wyniku
  ("You escaped", Time 0:03, Crystals 0 / 13), bez HUD i minimapy, panele nad nim, `Restart`, `Back to
  menu` i Escape do menu głównego. **Przejścia całej gry do wygranej nikt nie zagrał**, więc ekranu wyniku
  z prawdziwego zakończenia rundy nikt nie widział

**Nie widziane na ekranie** (nikt tego nie oglądał): sam kursor (przechwycony albo wolny: nie da się go
sfotografować), obrót myszą po `Play`, ponowne przechwycenie kursora kliknięciem w scenę po tyldzie, zmiana
rozmiaru okna, minimalizacja, skalowanie ekranu inne niż 100 procent, Tab i Enter w menu, pole tekstowe w
menu (żaden dokument go nie ma), build Debug zatwierdzonego kodu na ekranie i wszystko na macOS.

Znane ograniczenia (z kodu, nie ze zrzutów): pauza nie zatrzymuje wiatru w trawie (idzie od `glfwGetTime`),
poziom trudności nic nie zmienia, `drawsScene` jest przetestowana, ale renderer jej nie woła, menu główne
używa wysokiego przelotu kamery menu zamiast tła w postaci wideo, bez dokumentów menu gra startuje w
rundzie, a karta wygranej z `Hud.cpp` jest martwym kodem.

### 26.2. Otwarte: test ręczny na około trzydzieści minut

> Od 2026-10-06 nazwy paneli w tych krokach wskazują miejsce w oknie debug (Kategoria / zakładka / karta), a okno startuje ukryte: przed kontrolkami naciśnij `~`. Mapa i zasady czytania starszych kroków: sekcja 27.3. Zwijanie, rzędy pasków tytułu, dokowanie i stare zakładki paneli opisują program sprzed tej zmiany. Żaden punkt nie został odhaczony ani odznaczony.

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu i z testów.

Przygotowanie:

- [ ] usunąć stary `imgui.ini`, uruchomić grę z terminala bez przełączników: menu główne nad
      przelatującą kamerą, napisy czytelne, w logu brak linii `[error]` i brak linii `RmlUi:` z błędem
- [ ] `THIRD-PARTY-NOTICES.txt` zawiera sekcje RmlUi, kontenerów RmlUi, FreeType i kopii zlib z FreeType

Kursor i mysz (to, czego nie da się zobaczyć na zrzucie):

- [ ] w menu głównym kursor jest widoczny i mysz nie obraca kamery
- [ ] `Play`: kursor znika, a mysz obraca kamerę **od razu**, bez klikania w scenę
- [ ] Escape w rundzie: kursor się pojawia, a mysz nie obraca kamery; Escape jeszcze raz (albo `Resume`):
      kursor znów znika i mysz obraca kamerę
- [ ] tylda w rundzie: panele i kursor; kliknięcie w scenę przechwytuje kursor ponownie i żaden panel nie
      reaguje na to kliknięcie
- [ ] klawisze W, A, S, D, Shift i spacja działają po `Play` bez dodatkowego kliknięcia

Menu:

- [ ] Tab i Enter w menu: zapisać, czy Tab przechodzi po przyciskach i czy Enter klika (kod niczego tu
      nie ustawia, wynik nie jest znany)
- [ ] w menu głównym, w pauzie i na ekranie wyniku klawisze R, N, F, M i E nic nie robią
- [ ] `Quit` zamyka okno z kodem wyjścia 0, a Escape w menu głównym nie robi nic
- [ ] `Back to menu` z pauzy i z wyniku, potem `Play`: nowa gra
- [ ] pauza: czas rundy na HUD po wznowieniu nie przeskoczył, kryształy w pauzie stoją, trawa nadal
      faluje (znane zachowanie)
- [ ] panele debug nad pauzą i nad menu głównym: kliknięcie panelu nie klika przycisku pod spodem

Rozmiar okna i skala:

- [ ] zmiana rozmiaru okna w menu: panel zostaje na środku, przyciski reagują tam, gdzie je widać
- [ ] minimalizacja i przywrócenie okna w menu i w rundzie: obraz wraca, w logu brak błędu
- [ ] skalowanie ekranu 125, 150 i 200 procent: menu proporcjonalnie większe, przyciski reagują tam, gdzie
      je widać (przycisk ma 220 dp, czyli 330 pikseli przy 150 procentach: policzone, nie zmierzone)
- [ ] okno przeniesione między ekranami o różnej skali: menu dopasowuje rozmiar

Runda i wynik:

- [ ] przejście całej gry do wygranej: ekran wyniku z prawdziwym czasem i liczbą kryształów, `Restart`,
      `Back to menu` i Escape z wyniku do menu głównego
- [ ] `Restart` z pauzy: czas rundy wraca do zera, labirynt ten sam; `Play` po powrocie do menu: labirynt
      zbudowany od nowa z tego samego ziarna

Przełączniki i błędy:

- [ ] `--play`: start od razu w rundzie; `--menu-camera`: start w kamerze menu, bez menu głównego; `--play now`:
      dwie linie `[error]` (komunikat i lista pięciu przełączników), kod wyjścia niezerowy
- [ ] F2 w menu głównym i w pauzie nic nie robi, w rundzie włącza kamerę menu
- [ ] brak plików menu: tymczasowo zmienić nazwę katalogu `assets/ui` i uruchomić: gra startuje w rundzie,
      w logu błąd "The menus cannot be shown" (agent tego nie sprawdzał)
- [ ] build Debug zatwierdzonego kodu: przejść wszystkie ekrany i sprawdzić, że log nie ma linii z
      `GL_` (w `UiLayer::draw` stoi `GL_CHECK`)
- [ ] brak ostrzeżeń kompilatora przy buildzie Debug i Release po tej części

Sprawdziwszy wszystko: zaznaczyć wyniki tutaj i wpisać zapisane obserwacje. Zamknięcie tej listy nie
zamyka M9.

## 27. Lista kontrolna: okno debug w miejsce trzynastu paneli

2026-10-06 trzynaście osobnych paneli Dear ImGui (`src/debug/panels/*`, układanych przez `PanelLayout`) zastąpiło **jedno
okno debug**: klasa `DebugWindow` w `src/debug/DebugWindow.*`, widżety w `src/debug/Widgets.*` i `src/debug/Icons.*`,
siedem plików kategorii w `src/debug/categories/`, test wyszukiwania w `tests/SearchTests.cpp`. Okno ma pionowy pasek ikon z
siedmioma kategoriami (Render, Light, Post process, World, Player, Gameplay, Diagnostics), nagłówek z polem wyszukiwania i
przyciskiem przypięcia, zakładki w trzech kategoriach i karty z wierszami ustawień. W prawym górnym rogu stoi pasek stanu.
Zostało wszystkich 114 kontrolek i każdy odczyt, który był w panelach. Zmieniło się też: okno debug **startuje ukryte** (pokazuje
je `~`), pasek HUD **zawsze** stoi przy górnej krawędzi okna (nie ma już `FOLDED_ROW_COUNT` ani argumentu `panelsVisible` w
`drawHud`), minimapa startuje w **lewym dolnym** rogu (było: prawy dolny), tekst okna ma 14 pikseli (HUD zostaje przy 16), a
w `imgui.ini` zostaje już tylko przypięty panel. Opis kodu: [`../modules/debug-ui.md`](../modules/debug-ui.md), decyzje:
[`../decisions/debug-window-redesign.md`](../decisions/debug-window-redesign.md), [`../decisions/hud-always-at-the-top-edge.md`](../decisions/hud-always-at-the-top-edge.md) i [`../decisions/minimap-default-corner-bottom-left.md`](../decisions/minimap-default-corner-bottom-left.md). To, że okno startuje ukryte, jest decyzją
koordynatora pracy, a nie właściciela: czeka na jego potwierdzenie.

Tej zmiany **nie zamyka** żaden test ręczny: lista w sekcji 27.2 jest otwarta, a na macOS kod nie był budowany
([`build-macos.md`](build-macos.md)).

### 27.1. Zgłoszone (2026-10-06)

Środowisko: Windows. Wersji kompilatora ani sterownika dla bramki nie zapisano w tym dokumencie.

Bramka (zgłoszona przez bramkę na gałęzi z oknem debug, nie powtarzałem jej przy pisaniu tej sekcji):

- [x] `night_maze_tests.exe` w buildzie Debug: **526 przypadków testowych i 219214 asercji** (zgłoszone). Przed tą zmianą było 519
      i 219195. Nowych przypadków jest 7, wszystkie w `tests/SearchTests.cpp` (`matchesSearch` i `hasSearchWords`), a asercji
      19 więcej. Liczbę przypadków sprawdziłem na kodzie: suma makr `TEST_CASE` w `tests/*.cpp` na commicie `f6c6cd5` to 526
      (519 + 7). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona. Żaden z tych przypadków nie dotyka okna,
      kategorii ani widżetów: te wymagają ImGui i okna

**Widziane na zrzutach ekranu przez agenta.** Poniższe punkty są zapisane jako "widziane na zrzucie ekranu przez agenta
(2026-10-06), nie przez właściciela": agent, który napisał kod, uruchomił własny build Debug (Windows 11, skalowanie ekranu 100
procent, NVIDIA GeForce RTX 4070 Ti SUPER, OpenGL 4.1), sterował grą skryptem z prawdziwą myszą i klawiaturą i oglądał zrzuty
ekranu. To **nie jest** test ręczny właściciela i nie zamyka żadnego punktu z sekcji 27.2.

- widziane na zrzucie ekranu przez agenta: okno 1280 x 720 z `--play`: okno debug jest ukryte na starcie, pasek HUD stoi przy
  górnej krawędzi, minimapa w lewym dolnym rogu. Po `~` pojawia się pasek stanu w prawym górnym rogu i okno przy prawej krawędzi,
  pod paskiem HUD, a pasek HUD się nie przesuwa
- widziane na zrzucie ekranu przez agenta: wszystkie siedem kategorii i każda zakładka rysują się: Render, Light (zakładki
  Lights i Shadows, z obydwoma obrazami map cieni), Post process (z czterema podglądami), World (Maze z planem, Terrain and
  grass, Reflections), Player, Gameplay (z obrazem framebuffera minimapy), Diagnostics (Frame and shaders, Collision and
  picking, Assets z listą modeli i podglądami tekstur)
- widziane na zrzucie ekranu przez agenta: wyszukiwanie. `bias`: sześć wierszy w dwóch kartach cieni (wiersze `Resolution`
  trafiają się przez tekst pomocy). `shader`: przełącznik trawy przez tekst pomocy, cała karta `Frame` przez nazwę zakładki
  `Frame and shaders` i karta `Shaders` z listą. `fog density`: dwa wiersze. `zzz`: napis "Nothing matches. Try a shorter word.".
  Po Escape okno wraca do swojej kategorii
- widziane na zrzucie ekranu przez agenta: po jednej obsłużonej kontrolce z każdej kategorii, z widocznym skutkiem w obrazie
  albo w odczycie. Render: `Skybox` wyłączony (gwiazdy znikają), `Sky brightness` na 6. Light: latarka wyłączona i włączona,
  `Moon intensity` na 2. Post process: `Fog` wyłączona, `Exposure` na 8. World: `Wireframe` (podłoże jako linie), `Height scale`
  na 2,5. Player: `FOV` na 120, `Noclip` (linia `Mode` mówi noclip). Gameplay: `Minimap` wyłączona i włączona, `Battery` na 0
  (HUD pokazuje 0 procent i podpowiedź "Battery empty. Find a crystal.", latarka jest ciemna, HUD i okno się nie zasłaniają).
  Diagnostics: `Draw collision shapes` (żółte linie). Diagnostics nie ma suwaka
- widziane na zrzucie ekranu przez agenta: Ctrl+klik na `Yaw` zamienił pasek w pole tekstowe, wpisane "45" zostało przyjęte,
  Tab przeszedł do `Pitch`, wpisane tam "999" zostało przycięte do 89.0
- widziane na zrzucie ekranu przez agenta: przypięcie. Kategoria Light jako mały panel przy prawej krawędzi. Kliknięcie w scenę
  przechwyciło kursor (kropka celownika, ukryty kursor), a panel został na ekranie. `~` schował panel i pokazał go ponownie.
  Strzałka "następna" przeszła do Post process. Przycisk rozwinięcia przywrócił okno
- widziane na zrzucie ekranu przez agenta: menu główne (start bez `--play`): okno ukryte, po `~` leży nad prawą częścią karty
  menu, a pasek stanu mówi "Main menu". Pauza (Escape w rundzie): to samo, pasek mówi "Paused"
- widziane na zrzucie ekranu przez agenta: okno 1100 x 700: okno ma 550 pikseli szerokości, a karty stoją w jednej kolumnie
  (oglądane: Light, Post process, World, Gameplay, Diagnostics)

**Nie widziane na ekranie** (nikt tego nie oglądał): macOS, skalowanie ekranu inne niż 100 procent, przypięty panel zadokowany
albo ze zmienionym rozmiarem, okno wyboru koloru z wiersza koloru, otwarta lista (`Combo`), nieudane przeładowanie shadera w
nowej liście, karta `Failed to load`, ekran wyniku rundy z oknem debug, wyszarzony wiersz `Anisotropy` (ten sterownik ma
anizotropię), build Release na ekranie, pad. Suwak przeciągnięto w lewo tylko raz (`Battery`).

Znane ograniczenia (z kodu, nie ze zrzutów): brak okienek przy ikonie zębatki (każda kontrolka jest w wierszu), obydwa podglądy
map cieni są żądane naraz (zakładka Shadows pokazuje księżyc i latarkę obok siebie, a stary panel miał zakładkę na światło),
wartość suwaka wpisuje się Ctrl+klikiem, nie ma pogrubionej czcionki ani czcionki z ikonami (ikony rysuje `ImDrawList`), a
`Sky brightness` przycina wpisaną wartość do zakresu od 0 do 6 (stary suwak tego nie robił). Kontrastu koloru `TEXT_FAINT_COLOR`
na tle karty nikt nie zmierzył.

### 27.2. Otwarte: test ręczny na około czterdzieści minut

Tych kroków nikt jeszcze nie wykonał ręką. Oczekiwania wynikają z kodu i z testów.

Start i pasek stanu:

- [ ] usunąć stary `imgui.ini`, uruchomić grę z terminala bez przełączników: menu główne, okno debug **ukryte**, w logu brak
      linii `[error]`
- [ ] `~`: pokazuje okno przy prawej krawędzi i pasek stanu w prawym górnym rogu (`NIGHT MAZE`, ekran, FPS, czas klatki w ms,
      `seed N`); drugie `~` chowa oba. Pasek stanu zmienia ekran: "Main menu", "Playing", "Paused", "Round end"
- [ ] `--play`: pasek HUD stoi przy górnej krawędzi i **nie przeskakuje** po `~`; okno debug zaczyna się pod nim i nie zasłania
      go przy 1280 x 720, 1920 x 1080 i przy małej baterii (dwie linie podpowiedzi)
- [ ] minimapa stoi w **lewym dolnym** rogu (domyślnie), a okno debug po prawej jej nie zasłania; `Corner` w Gameplay / Minimap
      przełącza wszystkie cztery rogi

Układ i skala:

- [ ] okno 1280 x 720: karty w dwóch kolumnach, środek obrazu wolny; zmiana rozmiaru okna gry: okno idzie za nim
- [ ] okno 1100 x 700: okno ma 550 pikseli, karty w jednej kolumnie, etykiety nie są ucięte; bardzo małe okno: okno nie ma
      mniej niż 160 pikseli wysokości i wystaje poniżej dolnej krawędzi (zapisać, jak to wygląda)
- [ ] maksymalizacja i przywrócenie okna: okno debug zostaje przy prawej krawędzi, a obraz wraca
- [ ] skalowanie ekranu 125, 150 i 200 procent: tekst, ikony, suwaki i przełączniki rosną proporcjonalnie i są ostre, pasek stanu
      mieści się, napis `NIGHT MAZE` pod ikonami znika, gdy brakuje miejsca
- [ ] czytelność tekstu 14 pikseli w oknie (szczególnie `TEXT_FAINT_COLOR` na kartach: ścieżka karty w wynikach wyszukiwania) i
      16 pikseli w pasku HUD: zapisać, czy któryś jest za mały

Kategorie i zakładki:

- [ ] siedem ikon na pasku, podpowiedź przy każdej; nagłówek pokazuje nazwę i liczbę kontrolek (Render 8, Light 34, Post
      process 15, World 22, Player 16, Gameplay 15, Diagnostics 4)
- [ ] zakładki w Light, World i Diagnostics przełączają się, a po powrocie do kategorii jest wybrana ta, na której ją zostawiono
- [ ] po jednej zmienionej kontrolce w każdej kategorii (lista z sekcji 27.1) daje skutek w obrazie albo w odczycie, także
      w buildzie Release

Widżety:

- [ ] przełącznik zmienia stan po kliknięciu; suwak ma dwanaście kresek, wartość po lewej, przeciąganie w obie strony działa
      (w lewo sprawdzono tylko na `Battery`)
- [ ] Ctrl+klik na suwaku: pole tekstowe, wpisana wartość spoza zakresu jest przycinana; sprawdzić `Sky brightness` (wpisać 10:
      ma zostać 6)
- [ ] lista (`Combo`): otwarta lista czyta się, wybór zmienia wartość (na przykład `Lighting`, `Filter`)
- [ ] wiersz koloru (`Clear colour`): okno wyboru koloru otwiera się, a zapis `#rrggbb` obok pola odpowiada wybranemu kolorowi
- [ ] podpowiedź kartą po chwili spoczynku myszy, na etykiecie i na kontrolce; długa etykieta jest ucięta, a podpowiedź ją
      pokazuje w całości
- [ ] wyszarzony wiersz `Anisotropy` (tylko na sterowniku bez anizotropii: zapisać, czy taki sprzęt jest pod ręką)

Wyszukiwanie:

- [ ] `bias`, `shader`, `fog density`: wyniki jak w sekcji 27.1; nagłówek pokazuje "Search" i "N matches", a karty w wynikach
      mają miejsce (na przykład `Light / Shadows`) przy prawej krawędzi
- [ ] wartość zmieniona z wyników wyszukiwania działa (wiersze są żywe)
- [ ] Escape w polu opróżnia je i **nie otwiera pauzy**; klawisze gry nie działają, gdy pole jest edytowane; kliknięcie ikony
      kategorii kończy wyszukiwanie
- [ ] `zzz`: "Nothing matches. Try a shorter word."

Przypięcie:

- [ ] przycisk pinezki zamienia okno na mały panel z jedną kategorią; strzałki przechodzą po kategoriach (z ostatniej na
      pierwszą), przycisk rozwinięcia i krzyżyk w pasku tytułu przywracają okno
- [ ] panel da się przesunąć, zmienić jego rozmiar i zadokować do krawędzi okna gry; po ponownym uruchomieniu leży tam, gdzie
      go zostawiono (`imgui.ini`); samo okno debug niczego do `imgui.ini` nie zapisuje (sprawdzić plik)
- [ ] przy przechwyconym kursorze panel da się czytać, ale nie reaguje na mysz; `~` chowa go i pokazuje

Odczyty i ekrany:

- [ ] Diagnostics: `Frame` (FPS, czas klatki, framebuffer, okno, OpenGL, GPU), `Shaders` (czternaście programów z `OK`),
      Collision and picking, Assets: wartości zgadzają się z tym, co było w starych panelach
- [ ] nieudane przeładowanie shadera: zepsuć plik w kopii `assets`, `Reload shaders`: program z `FAILED`, plik i tekst błędu
      w liście, pełne ścieżki w podpowiedzi
- [ ] zmienić nazwę tekstury w kopii `assets`: karta `Failed to load` w Diagnostics / Assets pokazuje wpis
- [ ] okno debug nad pauzą, menu głównym i ekranem wyniku prawdziwie wygranej rundy: kliknięcie okna nie klika przycisku pod
      spodem; F2 chowa okno i przywraca je po wyłączeniu kamery menu
- [ ] build Release: okno, pasek stanu i HUD wyglądają jak w Debug; brak ostrzeżeń kompilatora po tej zmianie

Sprawdziwszy wszystko: zaznaczyć wyniki tutaj i wpisać zapisane obserwacje.

### 27.3. Mapa: stare panele i nowe miejsca

Nazwy "panel X" w starszych sekcjach tego dokumentu oznaczają miejsca z tej tabeli (źródło: `Categories.hpp`, pliki w
`src/debug/categories/` i nota autora kodu). Punkty `[x]` i opisy pomiarów zostają z nazwami z dnia pomiaru. Punkty `[ ]` mają
nazwy zamienione na nowe miejsce, a krok, który opisuje coś, czego okno nie ma (zwijanie panelu do paska tytułu, rozwijanie
strzałką, rzędy pasków tytułu, układ kolumn paneli, dokowanie stałego panelu, zakładki starych paneli), ma dopisek "bez
odpowiednika w oknie debug": tylko przypięty panel da się przesuwać, zmieniać i dokować.

| Stary panel | Kontrolki | Nowe miejsce (kategoria / zakładka / karta) |
|---|---:|---|
| Renderer | 4 | Render: karta Scene (`Lighting`, `Skybox`, `Sky brightness`, `Clear colour`). FPS, czas klatki, framebuffer, okno, OpenGL i GPU: Diagnostics / Frame and shaders / Frame |
| Shaders | 1 | Diagnostics / Frame and shaders / Shaders: `Reload shaders` i lista czternastu programów |
| Camera | 15 | Player: Position (`Player feet`, odczyty `Mode` i `Eye`), View (`Yaw`, `Pitch`, `FOV`, `Near plane`, `Far plane`), Movement (`Mouse sensitivity` i trzy prędkości), Menu camera (`Menu camera (F2)`, `Shot`, `Speed`, `Eye height`, `Time offset`) |
| Gameplay | 9 | Gameplay: Round (`Restart round`, `Pull all levers`), Battery, Rules (`Crystals needed`, `Battery lifetime`, `Recharge`, `Flicker below`, `Pickup radius`) |
| Terrain | 2 | World / Terrain and grass / Terrain: `Height scale`, `Wireframe` |
| Grass | 4 | World / Terrain and grass / Grass: `Grass`, `Density`, `Blade height`, `Wind strength` |
| Framebuffers | 21 | Post process: Tone mapping, Bloom, Fog, Vignette, Previews (`Depth range`, cztery obrazy). Minimapa (sześć kontrolek, trzecia zakładka starego panelu): Gameplay / Minimap |
| Shadows | 16 | Light / Shadows: karty Moon shadows i Flashlight shadows, po osiem kontrolek, i dwie karty z obrazami map |
| Environment | 9 | World / Reflections: karty Crystals (pięć kontrolek) i Puddles (cztery) |
| Maze | 7 | World / Maze: karta Next maze (`Width`, `Height`, `Seed`, `Levers`, `Notes`, `Regenerate`, `Random seed`), In play i Plan |
| Collision | 4 | Diagnostics / Collision and picking / Debug drawing (trzy pola) i Player / Position (`Noclip`) |
| Assets | 4 | Render: karta Textures and normals (`View mode`, `Normal mapping`, `Filter`, `Anisotropy`). Listy modeli, tekstur i `Failed to load`: Diagnostics / Assets |
| Lights | 18 | Light / Lights: karty Ambient, Moon, Flashlight, Crystal lights, Highlight |

Razem 13 paneli i 114 kontrolek w siedmiu kategoriach (Render 8, Light 34, Post process 15, World 22, Player 16, Gameplay 15,
Diagnostics 4). Liczby pochodzą z pliku `Categories.hpp` i z noty autora kodu. Mniejsze zmiany: format wartości `Mouse
sensitivity` to `%.2f` (było `%.2f deg/unit`), `Crystals needed` to `%.2f` (było `%.2f of all`), oba pola `Cone` pokazują
`%.1f deg`, a grupy panelu Lights (dawniej `CollapsingHeader`) są kartami, grupa księżyca nie startuje zwinięta.

## 28. Powiązane dokumenty

- Wersja dla macOS (zweryfikowana) i opis presetów: [`build-macos.md`](build-macos.md)
- Mapa repozytorium i plików konfiguracyjnych: [`project-structure.md`](project-structure.md)
- Stan kamienia milowego M7 w jednym miejscu: [`m7-status.md`](m7-status.md)
- Biblioteki: [`../libraries/glfw.md`](../libraries/glfw.md),
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md),
  [`../libraries/doctest.md`](../libraries/doctest.md) (testy jednostkowe)
- Kamera menu i przełączniki wiersza poleceń: [`../modules/game/menu-camera.md`](../modules/game/menu-camera.md), [`../decisions/menu-background-prerendered-loop.md`](../decisions/menu-background-prerendered-loop.md)
- Menu w RmlUi, ekrany gry i Escape: [`../modules/ui/README.md`](../modules/ui/README.md), [`../modules/game/game-states.md`](../modules/game/game-states.md), [`../libraries/rmlui.md`](../libraries/rmlui.md), [`../decisions/escape-pauses-and-goes-back.md`](../decisions/escape-pauses-and-goes-back.md)
- Launcher (poza ocenianym kodem C++): [`launcher.md`](launcher.md)
- Selekcja, dźwignie i kartki: [`../modules/scene/picking.md`](../modules/scene/picking.md), [`../modules/game/interactables.md`](../modules/game/interactables.md)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md),
  [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md) (mapy cieni księżyca i latarki),
  [`../modules/game/flashlight.md`](../modules/game/flashlight.md) (latarka w ręce),
  [`../modules/renderer/minimap.md`](../modules/renderer/minimap.md) (minimapa),
  [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md) (odbicia i załamania nieba)
- Dokumentacja CMake (generatory, presety): <https://cmake.org/cmake/help/latest/>
