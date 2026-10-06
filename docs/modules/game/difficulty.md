# Moduł game: trzy poziomy trudności jako jedna tabela liczb

Kamień milowy: M9, część 3 (2026-10-06). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z reguł rundy ([`gameplay.md`](gameplay.md): `requiredCrystalCount`, `batteryLifetimeSeconds`, `batteryPerCrystal`), z kryształów ([`gameplay.md`](gameplay.md), sekcja 2.7, i [`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md)), z ekranów ([`game-states.md`](game-states.md)), z kamery menu ([`menu-camera.md`](menu-camera.md)) i z cieni księżyca ([`../renderer/shadows.md`](../renderer/shadows.md)).
Kod: [`src/game/Difficulty.hpp`](../../../src/game/Difficulty.hpp) i [`Difficulty.cpp`](../../../src/game/Difficulty.cpp) (typ, tabela, wyszukiwanie po nazwie), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`startNewGame`, `fillMainMenuDocument`, konstruktor, `mazeSettingsFor`) i w [`src/game/Settings.cpp`](../../../src/game/Settings.cpp) (nazwa poziomu w pliku ustawień). Testy: [`tests/DifficultyTests.cpp`](../../../tests/DifficultyTests.cpp) (8 przypadków).

**Stan na dziś:** gra ma **trzy poziomy**, `Easy`, `Normal` i `Hard`, zapisane w **jednej tabeli** (`LEVELS` w `Difficulty.cpp`). Wiersz ma siedem pól: dwie nazwy, rozmiar labiryntu (dwie liczby), liczbę kryształów, ułamek kryształów, który otwiera bramę, i czas życia baterii. Przycisk `Play` kopiuje wiersz wybranego poziomu do prośby o labirynt i do reguł rundy. **Liczby poziomów `Normal` i `Hard` są propozycją autora kodu, którą właściciel ma dopracować po zagraniu**, a nie jego decyzją (sekcja 1.2).

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (tak jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę i autora kodu (2026-10-06), nie powtórzone przy pisaniu tego dokumentu:** bramka na gałęzi menu (przed scaleniem z oknem debug) zgłosiła **557 przypadków testowych i 220100 asercji**; osiem przypadków z tego to `DifficultyTests.cpp`. Dla **scalonego drzewa** (okno debug i ekrany menu razem) **żadna bramka nie zgłosiła liczb**: pełna bramka na scalonym drzewie nie została uruchomiona (przebieg przerwał system z braku pamięci, a właściciel zdecydował, że tego dnia go pomija). **Pomiary z sekcji 2.3 i 2.4 to pomiary autora kodu** z jednorazowego testu, którego nie zatwierdzono do repozytorium: seedy od 1 do 50 na poziom, prawdziwa mapa wysokości, gałąź menu przed scaleniem, maszyna autora (RTX 4070 Ti SUPER, Release, okno 720p, panele debug widoczne, vsync nie ograniczał). Nie mierzyłem niczego sam i nic nie zmierzono na scalonym drzewie.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela:** każdy poziom uruchomiony z menu, z sumą na HUD odpowiednio 10 z 13, 19 z 26 i 32 z 40 kryształów; rzeczywisty koniec rundy na `Easy`, ziarno 7, osiągnięty lotem z wyłączonymi kolizjami (klawisz `N`) od kryształu do kryształu (czas 0:23), **nie przejściem korytarzami**. Nie widziane: cień księżyca z bliska na `Hard` (dwa zrzuty z komórki startu leżały w całości w cieniu, a z przelotu nic nie wyglądało gorzej niż na `Easy`).
3. **Otwarta lista właściciela:** [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 28.2 (Windows), i [`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcja "M9, część 3 (ekrany menu, poziomy trudności, ustawienia) na macOS" (w całości otwarta). Właściciel zgłosił 2026-10-06, że zagrał na `Hard` w buildzie Debug i że menu, okno debug i gra działają ("it was great"). To **relacja właściciela**, nie zamknięta lista i nie zatwierdzenie liczb poziomów.

Liczby w sekcji 2.5 są **policzone ręcznie z kodu**, nie zmierzone w programie.

## 1. Po co to jest

### 1.1 Do czego służy tabela

Właściciel uznał, że runda jest za łatwa i za krótka ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md)). Gra ma więc kilka rozmiarów rundy, a każdy z nich to zestaw liczb, które i tak już istniały w kodzie jako osobne pola: rozmiar labiryntu (`MazeSettings::width` i `height`), liczba kryształów (`MazeSettings::crystalCount`), ułamek otwierający bramę (`GameplaySettings::requiredFraction`) i czas baterii (`GameplaySettings::batteryLifetimeSeconds`). Tabela zbiera je w jednym miejscu, żeby **poziom był jedną wartością** (`Difficulty`), a nie czterema ustawieniami, które trzeba pamiętać razem.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md), 2026-10-06):

1. Gra ma **trzy poziomy trudności**.
2. Poziomy zmieniają **rozmiar labiryntu i liczbę kryształów**.
3. **Liczby poziomów nie są ustalone**: zostaną dobrane przy pracy nad balansem.

**Wszystko inne jest propozycją autora kodu, którą właściciel dopracuje po zagraniu:**

- **czas baterii i próg bramy w tabeli.** Decyzja właściciela mówi o rozmiarze i kryształach. Kod zmienia też baterię (180, 150 i 120 sekund) i ułamek otwierający bramę (70, 70 i 80 procent). To rozszerzenie decyzji, które autor kodu uznał za potrzebne, żeby większy labirynt był trudniejszy w sposób, który da się policzyć (sekcja 2.4). Notatka z zakresu menu zostawiała baterię jako trzecią liczbę, "o którą kod już pyta", i nie ruszała jej;
- **same liczby**: 10 na 10, 16 na 16 i 22 na 22; 13, 26 i 40 kryształów; 180, 150 i 120 sekund;
- **liczba kryształów nie wynika z reguły "jeden na osiem komórek"** (decyzja właściciela z 2026-10-05, [`../../decisions/crystal-count-and-gate-threshold.md`](../../decisions/crystal-count-and-gate-threshold.md)). Ta reguła dałaby 13, 32 i 61 kryształów (wzór `(komórki + 4) / 8` z `crystalCountFor`: `(100 + 4) / 8 = 13`, `(256 + 4) / 8 = 32`, `(484 + 4) / 8 = 61`). Zgadza się z nią tylko `Easy`, i pilnuje tego test `the easy level is the game as it was before the levels existed`. `Normal` (26) i `Hard` (40) mają **mniej** kryształów, niż dałaby reguła; w kodzie nie ma zapisu, dlaczego akurat te liczby, więc nie zgaduję powodu. Poziomy podają liczbę wprost (`placeCrystals` przyjmuje `wantedCount`). Wzór `crystalCountFor` działa tylko wtedy, gdy `MazeSettings::crystalCount` ma wartość domyślną `CRYSTAL_COUNT_FROM_SIZE` (tak buduje świat `buildMazeWorld` wywołany bez liczby, na przykład w testach). W działającej grze pole zawsze niesie liczbę poziomu, bo wpisują ją `mazeSettingsFor` i `startNewGame`;
- struktura: że jest jedna tabela `constexpr`, że wiersz jest strukturą `DifficultyLevel`, że nazwa w pliku i w dokumentach to `key` małymi literami, że pierwszy labirynt za menu główne ma rozmiar poziomu zapisanego w pliku ustawień.

## 2. Teoria

### 2.1 Co robi każda liczba

| Pole | Co robi w grze | Gdzie jest używane |
|---|---|---|
| `name` | napis w menu: `Easy`, `Normal`, `Hard` | przyciski, ekran pauzy i wyniku (`m_playedDifficultyName`) |
| `key` | nazwa w pliku ustawień i w identyfikatorach przycisków: `easy`, `normal`, `hard` | `difficultyFromKey`, `formatSettings`, `data-action="difficulty-easy"` |
| `mazeWidth`, `mazeHeight` | rozmiar labiryntu w komórkach (komórka ma 2 m na 2 m) | `MazeSettings::width` i `height` |
| `crystalCount` | ile kryształów jest w labiryncie | `MazeSettings::crystalCount`, dalej `placeCrystals(..., wantedCount)` |
| `requiredFraction` | jaka część kryształów otwiera bramę | `GameplaySettings::requiredFraction`, dalej `requiredCrystalCount` |
| `batteryLifetimeSeconds` | ile sekund świeci latarka z pełną baterią | `GameplaySettings::batteryLifetimeSeconds`, dalej `drainBattery` |

Poziom działa **przez trzy kopie** w `startNewGame`: do prośby o labirynt (rozmiar i kryształy), do reguł rundy (ułamek i bateria) i do napisu nazwy poziomu. Poniżej tabela w kodzie, w całości (z pliku `Difficulty.cpp`):

```cpp
// One row per level, in the order of the enum: the number of a level is its row.
constexpr std::array<DifficultyLevel, ALL_DIFFICULTIES.size()> LEVELS = {{
    {.name = "Easy",
     .key = "easy",
     .mazeWidth = 10,
     .mazeHeight = 10,
     .crystalCount = 13,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 180.0F},
    {.name = "Normal",
     .key = "normal",
     .mazeWidth = 16,
     .mazeHeight = 16,
     .crystalCount = 26,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 150.0F},
    {.name = "Hard",
     .key = "hard",
     .mazeWidth = 22,
     .mazeHeight = 22,
     .crystalCount = 40,
     .requiredFraction = 0.8F,
     .batteryLifetimeSeconds = 120.0F},
}};
```

(Plik: `src/game/Difficulty.cpp`, linie od komentarza do końca tabeli.) Kolejność wierszy jest kolejnością `enum class Difficulty`: numer poziomu jest numerem wiersza. `difficultyLevel(Difficulty)` bierze wiersz funkcją `at` (rzuca wyjątek dla numeru spoza tablicy, więc pomyłka nie czyta cudzej pamięci), a `difficultyFromKey` szuka wiersza po `key`:

```cpp
const DifficultyLevel& difficultyLevel(Difficulty difficulty) {
    return LEVELS.at(static_cast<std::size_t>(difficulty));
}

bool difficultyFromKey(std::string_view key, Difficulty& difficulty) {
    for (const Difficulty candidate : ALL_DIFFICULTIES) {
        if (key == difficultyLevel(candidate).key) {
            difficulty = candidate;
            return true;
        }
    }
    return false;
}
```

(Plik: `src/game/Difficulty.cpp`.) Nazwa spoza listy daje fałsz i **zostawia argument bez zmian**: test sprawdza, że `"Easy"` (wielką literą), `""` i `"nightmare"` nic nie zmieniają. To przydaje się w pliku ustawień, gdzie błędny wiersz ma być pominięty ([`settings.md`](settings.md)).

### 2.2 Jak poziom trafia do gry

```cpp
void NightMazeApp::startNewGame(const NewGame& newGame) {
    // The numbers of the level: the size of the maze and its crystals go into the
    // request for the maze, the gate and the battery into the rules of the round. They
    // overwrite what the debug UI may have set in the same fields, so every new game of
    // a level is the same game. The numbers of levers and notes are not part of a
    // level and stay as they are.
    const DifficultyLevel& level = difficultyLevel(newGame.difficulty);
    m_mazeSettings.width = level.mazeWidth;
    m_mazeSettings.height = level.mazeHeight;
    m_mazeSettings.crystalCount = level.crystalCount;
    m_gameplay.requiredFraction = level.requiredFraction;
    m_gameplay.batteryLifetimeSeconds = level.batteryLifetimeSeconds;

    // The maze is always built again, also for the seed that is in play: the level may
    // be another one, and the numbers of levers and notes may have been changed.
    m_mazeSettings.seed = newGame.seed;
    m_mazeSettings.regenerate = false;
    regenerateMaze();
    m_playedDifficultyName = level.name;

    // The difficulty that was just played is the one the main menu starts with next
    // time, so it is written to the settings file now.
    saveSettings();
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `startNewGame`, w całości.) Trzy rzeczy do zapamiętania:

- liczby poziomu **nadpisują** to, co okno debug mogło ustawić w tych samych polach. Dzięki temu każda nowa gra danego poziomu jest tą samą grą. Liczba dźwigni i kartek **nie jest częścią poziomu** i zostaje taka, jaka była;
- labirynt jest budowany od nowa także dla ziarna, które już gra (poziom mógł się zmienić);
- na końcu poziom, który właśnie zagrano, trafia do pliku ustawień, więc menu główne następnym razem zaczyna od niego.

**Pierwszy labirynt**, ten za menu głównym i ten, w którym zaczyna się runda `--play`, też bierze wiersz tabeli: poziom zapisany w pliku ustawień (domyślnie `Normal`). Robi to funkcja `mazeSettingsFor` w konstruktorze `NightMazeApp`, a reguły pierwszej rundy kopiuje ten sam wiersz. Ziarno pierwszego labiryntu to `options.seed` (domyślnie 1), a nie liczba z pola ziarna w menu: pole pokazuje osobną, losową liczbę dla pierwszej **gry** ([`../ui/menu-screens.md`](../ui/menu-screens.md)). Ustawienia domyślne `GameplaySettings` (0,7 i 180 s) i `DEFAULT_MAZE_WIDTH` oraz `DEFAULT_MAZE_HEIGHT` (10) **nie zmieniły się**: to one są poziomem `Easy`.

### 2.3 Propozycja liczb i pomiary autora

**To jest PROPOZYCJA autora kodu, którą właściciel dopracuje po zagraniu.** Poniższe liczby pochodzą z notatki przekazanej przez autora i z komentarza w `Difficulty.hpp`; nie mierzyłem ich i nie da się ich powtórzyć z repozytorium (test pomiarowy był jednorazowy i nie został zatwierdzony). Każdy poziom: 50 labiryntów, ziarna od 1 do 50, prawdziwa mapa wysokości. **Najkrótsza runda** to najkrótszy spacer, który zbiera potrzebne kryształy i kończy się w wyjściu, **dla gracza, który zna labirynt**: dwa razy liczba krawędzi poddrzewa łączącego start, wyjście i wybrane kryształy, minus droga do wyjścia; kryształy wybierane zachłannie, według najmniejszej liczby nowych komórek. Zachłanny wybór to **heurystyka, nie dowód minimum**: prawdziwy najkrótszy spacer może być krótszy. Prędkość chodu to 3 m/s (`Player::WALK_SPEED`).

Tabela w komentarzu `Difficulty.hpp` (liczby autora):

| Poziom | Labirynt | Kryształy | Brama otwiera się przy | Bateria | Droga do wyjścia (średnia) | Najkrótsza runda (średnia) |
|---|---|---|---|---|---|---|
| `Easy` | 10 na 10 | 13 | 70 procent (10) | 180 s | 135 m | 211 m, 1:10 |
| `Normal` (domyślny) | 16 na 16 | 26 | 70 procent (19) | 150 s | 312 m | 538 m, 2:59 |
| `Hard` | 22 na 22 | 40 | 80 procent (32) | 120 s | 527 m | 1081 m, 6:00 |

Pomiary autora, drogi w metrach (minimum, średnia, maksimum z 50 ziaren):

| Poziom | Droga od startu do wyjścia, m | Wszystkie korytarze, m | Najkrótsza runda, m | Najkrótsza runda przy chodzie | Sekund na potrzebny kryształ | Ślepe zaułki (średnia) |
|---|---|---|---|---|---|---|
| `Easy` | 100 / 135 / 172 | 198 | 174 / 211 / 260 | 1:10 | 7,0 | 12 |
| `Normal` | 234 / 312 / 392 | 510 | 470 / 538 / 654 | 2:59 | 9,4 | 27 |
| `Hard` | 362 / 527 / 690 | 966 | 888 / 1081 / 1286 | 6:00 | 11,3 | 50 |

Inne liczby autora, na poziom (ziarno 1 dla szerokości texela i odległości; okno 720p, Release, panele debug pokazane, maszyna autora, **gałąź menu przed scaleniem, ze starymi panelami**):

| Poziom | Texel cienia księżyca przy 2048 | Najdalszy róg lądu od kamery przelotu (daleka płaszczyzna 100 m) | Budowa świata | Budowa ścieżki kamery menu | Czas klatki, przelot w menu | Czas klatki w rundzie |
|---|---|---|---|---|---|---|
| `Easy` | 3,16 cm | 47 m | 0,2 ms | 10 ms | 0,83 ms | 0,86 ms |
| `Normal` | 3,94 cm | 64 m | 0,4 ms | 27 ms | 1,14 ms | 1,29 ms |
| `Hard` | 4,72 cm | 80 m | 0,7 ms | 50 ms | 1,54 ms | 1,82 ms |

Nic z tych liczb nie zmierzono na scalonym drzewie (okno debug zastąpiło wtedy panele), więc czasy klatek mogą dziś być inne. Co z nich wynika (rozumowanie autora kodu, które sprawdziłem tylko arytmetycznie): liczba sekund na potrzebny kryształ (najkrótsza runda przy chodzie podzielona przez liczbę potrzebnych kryształów) rośnie z 7,0 przez 9,4 do 11,3, a bateria maleje, więc **na `Hard` światło trzeba planować**. Komentarz w `Difficulty.hpp` liczy to tak: pełna bateria plus każdy potrzebny kryształ daje 18 minut światła dla rundy, której najkrótszy spacer trwa 6 (to **górne ograniczenie**, patrz sekcja 2.5).

### 2.4 Co zależy od rozmiaru labiryntu i dlaczego nic poza tabelą nie musiało się zmienić

Autor sprawdził, co w grze zależy od rozmiaru labiryntu, i zapisał dwa testy, które pilnują granic (oba w `DifficultyTests.cpp`):

- **Kamera przelotu widzi cały ląd każdego poziomu w obrębie dalekiej płaszczyzny** (100 m): najdalszy róg lądu jest od kamery przelotu oddalony o 47, 64 i 80 m (pomiar autora, ziarno 1). Test `the glide of the menu camera sees the whole land of every level` sprawdza to dla 32 punktów okręgu.
- **Texel mapy cieni księżyca zostaje poniżej 5 cm** i rośnie z poziomem (3,16, 3,94 i 4,72 cm przy mapie 2048). Mapa księżyca obejmuje cały ląd, więc większy labirynt to większe texele. Test `the shadow map of the moon stays finer than a wall is thick on every level` pilnuje progu 0,05 m (komentarz w teście: ściana ma 0,2 m, a texel o czwartą część tego jeszcze rysuje jej cień) i tego, że texel rośnie ze stopniem trudności:

```cpp
TEST_CASE("the shadow map of the moon stays finer than a wall is thick on every level") {
    // The map of the moon covers the whole land, so a larger maze means larger texels.
    // The walls are 0.2 m thick: a texel of a quarter of that still draws their shadows.
    constexpr float LARGEST_TEXEL = 0.05F;
    const glm::vec3 moon = game::moonDirection(game::LightingSettings{});
    float previous = 0.0F;
    for (const Difficulty difficulty : game::ALL_DIFFICULTIES) {
        CAPTURE(game::difficultyLevel(difficulty).name);
        const game::MazeWorld world = worldOf(difficulty, 1U);
        const scene::LightSpace lightSpace =
            scene::directionalLightSpace(game::shadowCasterBounds(world.terrain), moon);
        const float texel = game::shadowTexelSize(lightSpace, game::SHADOW_MAP_SIZE_HIGH);
        CHECK(texel < LARGEST_TEXEL);
        CHECK(texel > previous);
        previous = texel;
    }
}
```

(Plik: `tests/DifficultyTests.cpp`.) Reszta według autora **nie ma limitu, którego poziom dosięga**: minimapa skaluje się sama (`minimapHalfExtent`), a teren, trawa i kałuże nie mają górnej granicy, do której poziom dochodzi. Liczba kryształów (do 40) mieści się w `MAX_CRYSTAL_COUNT` (64), a światła punktowe wybiera się spośród nich co klatkę ([`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md)). **Granica rozmiaru według autora:** labirynt większy niż około 24 na 24 przekroczyłby próg 5 cm dla texela i zbliżyłby się do dalekiej płaszczyzny w przelocie. To ocena autora na podstawie trzech punktów pomiaru (3,16, 3,94 i 4,72 cm), nie osobny pomiar.

### 2.5 Przykład policzony ręcznie: poziom `Normal`

Z wiersza `Normal` (16 na 16, 26 kryształów, 0,7, 150 s) wynika:

1. **Komórki:** 16 razy 16 to 256 komórek. Reguła "jeden na osiem komórek" dałaby `(256 + 4) / 8 = 32` kryształy (dzielenie całkowite z zaokrągleniem do najbliższej), ale poziom prosi o 26, więc `placeCrystals(maze, seed, start, exit, 26)` stawia ich 26 (ślepe zaułki najpierw, potem inne komórki; zawsze nie w komórce startu i wyjścia).
2. **Próg bramy:** `requiredCrystalCount(26, 0.7F)` liczy `ceil(0,7 * 26 - 0,001)`. `0,7F * 26` to około 18,2, minus 0,001 to około 18,199, a sufit z tego to **19**. Menu główne pokazuje więc "19 of 26" (`fillMainMenuDocument` wywołuje tę samą funkcję). Poprawka 0,001 (`ROUNDING_GUARD`) chroni przed iloczynem w `float`, który wypada odrobinę ponad liczbą całkowitą.
3. **Bateria:** pełna bateria to 150 s ze świecącą latarką. Zebrany kryształ oddaje `batteryPerCrystal` = 0,25 pełnej baterii, czyli `0,25 * 150 = 37,5 s`. Menu główne pokazuje czas baterii jako `timeText(150)`, czyli `2:30`.
4. **Górne ograniczenie światła:** pełna bateria plus 19 potrzebnych kryształów daje najwyżej `150 + 19 * 37,5 = 862,5 s`, czyli około 14 minut 22 sekund. **To jest tylko górne ograniczenie**, z dwóch powodów. Po pierwsze, reguła zbierania przycina ładunek do 1:

```cpp
        if (scene::overlaps(reach, pickup)) {
            crystal.collected = true;
            ++round.collectedCount;
            round.battery = std::min(round.battery + settings.batteryPerCrystal, 1.0F);
        }
```

(Plik: `src/game/Round.cpp`, `collectCrystals`.) Kryształ zebrany przy ładunku powyżej 0,75 oddaje tylko tyle, ile brakuje do pełnej baterii, więc część jego światła przepada. Po drugie, latarka drenuje baterię tylko, gdy świeci, a gracz może ją wyłączyć. Ta sama arytmetyka dla `Easy`: `ceil(0,7 * 13 - 0,001) = ceil(9,099) = 10` kryształów, `0,25 * 180 = 45 s` na kryształ; dla `Hard`: `ceil(0,8 * 40 - 0,001) = ceil(31,999) = 32` kryształy, `0,25 * 120 = 30 s` na kryształ, `120 + 32 * 30 = 1080 s`, czyli 18 minut, jak w komentarzu autora (znów górne ograniczenie). Sumy z menu (10 z 13, 19 z 26, 32 z 40) zgadzają się z tym, co agent zobaczył na HUD (sekcja o dowodach).
5. **Porównanie z najkrótszą rundą autora:** 2:59 chodu (179 s) na `Normal` przy 862,5 s górnego ograniczenia światła to margines, w którym mieści się szukanie. Gracz, który **nie zna labiryntu**, potrzebuje wielokrotności najkrótszej rundy (komentarz w `Difficulty.hpp`); ile dokładnie, nikt nie zmierzył.

## 3. Jak to działa w OpenGL

Moduł nie woła OpenGL i nie ma nowych obiektów OpenGL: to dane i dwie czyste funkcje, jak reszta `game_logic`. Poziom wpływa na obraz tylko pośrednio, przez rozmiar labiryntu (liczba trójkątów ścian, wielkość mapy cieni księżyca i terenu, liczba kryształów w trzech przebiegach rysowania) i liczbę kryształów, z których światła punktowe wybiera się co klatkę.

## 4. Shadery

Brak: moduł nie zmienia żadnego shadera. Blok światła (`scene::LightSet`, 928 bajtów, 16 świateł punktowych) jest bez zmian.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| `src/game/Difficulty.hpp`, `.cpp` | `enum class Difficulty` (przeniesione z `GameState.hpp`), `ALL_DIFFICULTIES`, `struct DifficultyLevel`, `difficultyLevel`, `difficultyFromKey`. Część `game_logic` |
| `src/game/NightMazeApp.*` | `startNewGame` (kopiuje wiersz), `mazeSettingsFor` (wiersz dla pierwszego labiryntu), `fillMainMenuDocument` (blok informacji o poziomie), konstruktor (reguły pierwszej rundy) |
| `src/game/Settings.*` | pole `GameSettings::difficulty` i nazwa `difficulty` w pliku |
| `tests/DifficultyTests.cpp` | 8 przypadków |

### 5.2 Typy

```cpp
/// How hard a new game is. The numbers behind each level are in difficultyLevel.
enum class Difficulty {
    Easy = 0,
    Normal,
    Hard,
};

/// Every level, in the order the main menu shows them.
constexpr std::array<Difficulty, 3> ALL_DIFFICULTIES = {Difficulty::Easy, Difficulty::Normal,
                                                        Difficulty::Hard};
```

(Plik: `src/game/Difficulty.hpp`.) `Difficulty` ma trzy wartości, a `ALL_DIFFICULTIES` daje je w kolejności, w jakiej pokazuje je menu główne (i w jakiej leżą w tabeli). Struktura wiersza:

```cpp
/// The numbers of one level: everything that differs between an easy and a hard game.
/// A new game copies them into the request for the maze (MazeSettings) and into the
/// rules of the round (GameplaySettings).
struct DifficultyLevel {
    /// The name the menus show: "Easy".
    const char* name;

    /// The name in the settings file and in the documents of the menu: "easy". Lower
    /// case and without spaces.
    const char* key;

    /// The size of the maze in cells: columns and rows.
    int mazeWidth;
    int mazeHeight;

    /// How many crystals float in the maze (MazeSettings::crystalCount).
    int crystalCount;

    /// The part of the crystals that opens the gate (GameplaySettings::requiredFraction).
    float requiredFraction;

    /// How long a full battery lasts with the flashlight on, in seconds
    /// (GameplaySettings::batteryLifetimeSeconds). A collected crystal gives a quarter of
    /// it back, so a shorter battery also makes every crystal worth less light.
    float batteryLifetimeSeconds;
};
```

(Plik: `src/game/Difficulty.hpp`.) Komentarz przy `batteryLifetimeSeconds` mówi, że zebrany kryształ oddaje ćwierć baterii, **więc krótsza bateria oznacza też mniej światła z każdego kryształu** (37,5 s na `Normal`, 30 s na `Hard`).

### 5.3 Testy

`DifficultyTests.cpp` (8 przypadków), wszystkie bez okna:

| Test | Co sprawdza |
|---|---|
| `the three levels have these numbers` | każda liczba z tabeli, wprost. Zmiana liczby w tabeli zmienia grę i wymaga zmiany tego testu, komentarza w `Difficulty.hpp` i dokumentów |
| `the easy level is the game as it was before the levels existed` | `Easy` ma domyślny rozmiar (`DEFAULT_MAZE_WIDTH`, `DEFAULT_MAZE_HEIGHT`), liczbę kryształów z `crystalCountFor`, ułamek i baterię z `GameplaySettings` |
| `every level is harder than the one before` | większy obszar, więcej kryształów, ułamek nie mniejszy, więcej potrzebnych kryształów, krótsza bateria |
| `the numbers of every level are ones the game accepts` | rozmiar od 2 do `Maze::MAX_SIZE`, liczba kryształów od 1 do `MAX_CRYSTAL_COUNT`, ułamek od 0 (wyłącznie) do 1, bateria dodatnia |
| `a maze of every level really gets the crystals of its level` | 10 ziaren na poziom: labirynt ma tyle kryształów, ile wiersz mówi, i bramę |
| `the glide of the menu camera sees the whole land of every level` | sekcja 2.4 |
| `the shadow map of the moon stays finer than a wall is thick on every level` | sekcja 2.4 |
| `a level is found by its key, and an unknown key changes nothing` | `difficultyFromKey` |

Przypadek `easy level is the game as it was`:

```cpp
TEST_CASE("the easy level is the game as it was before the levels existed") {
    const game::DifficultyLevel& easy = game::difficultyLevel(Difficulty::Easy);
    const game::GameplaySettings defaults;
    CHECK(easy.mazeWidth == game::DEFAULT_MAZE_WIDTH);
    CHECK(easy.mazeHeight == game::DEFAULT_MAZE_HEIGHT);
    CHECK(easy.crystalCount == game::crystalCountFor(easy.mazeWidth * easy.mazeHeight));
    CHECK(easy.requiredFraction == defaults.requiredFraction);
    CHECK(easy.batteryLifetimeSeconds == defaults.batteryLifetimeSeconds);
}
```

(Plik: `tests/DifficultyTests.cpp`.) **Nie ma testu na:** to, że `startNewGame` kopiuje wiersz do właściwych pól (kod z oknem), ani na wartości liczbowe z pomiarów (jednorazowe), ani na to, że poziom jest grywalny.

## 6. Okno debugowania (dawniej panel ImGui)

**Okno debug nie ma kontrolki poziomu trudności ani liczby kryształów** (`MazeSettings::crystalCount` jest dla niego niewidoczne): rozmiar labiryntu, ziarno i liczbę dźwigni i kartek można ustawić w kategorii World (zakładka Maze). Pole `crystalCount` zostaje w działającej grze przy liczbie ostatnio zagranego poziomu (wpisuje ją `startNewGame`, a w oknie nic jej nie zmienia), więc labirynt o innym rozmiarze, zbudowany przyciskiem `Regenerate`, ma tyle kryształów, ile miał poziom (wzór `crystalCountFor` nie działa, dopóki pole nie wróci do wartości domyślnej). To wniosek z czytania kodu, nie sprawdzony na ekranie. Po przebudowie labiryntu z okna debug pauza i ekran wyniku pokazują poziom jako `Custom` (`m_playedDifficultyName`), bo liczby nie są już liczbami żadnego poziomu.

## 7. Pułapki

1. **Liczby `Normal` i `Hard` to propozycja.** Nikt jeszcze nie ocenił ich na ekranie po zagraniu rundy na każdym poziomie oprócz relacji właściciela o `Hard`. Zmiana liczby w tabeli wymaga zmiany testu `the three levels have these numbers`, komentarza w `Difficulty.hpp` i tych dokumentów.
2. **Poziom zmienia więcej, niż zdecydował właściciel.** Bateria i próg bramy są dodatkiem autora (sekcja 1.2).
3. **`Normal` i `Hard` łamią regułę "jeden na osiem komórek"** (13, 32, 61 z reguły; 13, 26, 40 z tabeli).
4. **Pomiary pochodzą ze starej gałęzi.** Czasy klatek zmierzono ze starymi panelami debug, przed scaleniem z oknem debug.
5. **"Najkrótsza runda" jest heurystyką** (wybór zachłanny) i dotyczy gracza, który zna labirynt.
6. **Górne ograniczenie światła nie jest czasem świecenia**: ładunek jest przycinany do 1, a gracz może wyłączyć latarkę.
7. **Pierwszy labirynt za menu głównym nie ma ziarna z pola.** Pole ziarna w menu pokazuje losową liczbę dla gry, którą uruchomi `Play`, a labirynt w tle ma ziarno `options.seed` (domyślnie 1).
8. **Poziom `Custom`.** Po przebudowie labiryntu z okna debug napis poziomu to `Custom`.
9. **Menu główne na `Normal` i `Hard`.** Przelot kamery wybiera 16 najbliższych świateł kryształów wokół oka przelotu ([`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md)), więc w większym labiryncie spora część tła może nie mieć światła kryształu. Nikt tego nie oglądał na ekranie.

## 8. Ćwiczenia

1. **Tabela na kartce.** Dla poziomu `Hard` policz próg bramy, sekundy baterii na kryształ i górne ograniczenie światła. Odpowiedź: `ceil(0,8 * 40 - 0,001) = 32`, `0,25 * 120 = 30 s`, `120 + 32 * 30 = 1080 s`, czyli 18 minut.
2. **Reguła osiem komórek.** Ile kryształów dałaby `crystalCountFor` dla `Normal` i `Hard`? Odpowiedź: 32 i 61, a tabela ma 26 i 40.
3. **Hard większy.** Które dwa testy graniczne w `DifficultyTests.cpp` mogłyby zawieść, gdyby `Hard` miał 30 na 30, i dlaczego? Odpowiedź (z pomiarów autora, nie z uruchomienia): prawdopodobnie texel księżyca przekroczy 5 cm, a kamera przelotu zbliży się do dalekiej płaszczyzny, co autor ocenia dla labiryntu większego niż około 24 na 24.
4. **Poziom z pliku.** Wpisz `difficulty = hard` do pliku ustawień i uruchom z `--play`. Jaki labirynt dostaniesz? Odpowiedź: 22 na 22 z 40 kryształami, o ziarnie 1 (domyślne `--seed`), z regułami `Hard`.
5. **Nowy poziom.** Co trzeba zmienić, żeby dodać czwarty poziom? Odpowiedź: wartość w `enum class Difficulty`, wiersz w `LEVELS` (tablica ma rozmiar `ALL_DIFFICULTIES.size()`, więc `ALL_DIFFICULTIES` też), przycisk w `main_menu.rml` (`difficulty-<key>`), testy.

## 9. Pytania kontrolne

1. **Czym jest poziom trudności w kodzie?**
   Jednym wierszem tabeli `LEVELS` z siedmioma polami, kopiowanym przez `startNewGame` do prośby o labirynt i do reguł rundy.
2. **Które liczby poziomu są decyzją właściciela?**
   Że poziomy są trzy i że zmieniają rozmiar labiryntu i liczbę kryształów. Reszta, w tym bateria i próg bramy, jest propozycją autora.
3. **Dlaczego `Normal` ma 26 kryształów, a nie 32?**
   Tabela podaje liczbę wprost. Powodu w kodzie nie zapisano.
4. **Dlaczego "pełna bateria plus kryształy" jest tylko górnym ograniczeniem światła?**
   Ładunek jest przycinany do 1, a gracz może wyłączyć latarkę.
5. **Co sprawdzają dwa testy graniczne?**
   Że kamera przelotu widzi cały ląd w obrębie dalekiej płaszczyzny i że texel mapy cieni księżyca jest poniżej 5 cm.
6. **Skąd pochodzą liczby z tabeli pomiarów?**
   Z jednorazowego testu autora na seedach od 1 do 50, na gałęzi menu przed scaleniem. Nie są w repozytorium.

## 10. Źródła

- Notatki: [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md), [`../../decisions/crystal-count-and-gate-threshold.md`](../../decisions/crystal-count-and-gate-threshold.md), [`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md).
- Dokumenty: [`gameplay.md`](gameplay.md), [`game-states.md`](game-states.md), [`settings.md`](settings.md), [`menu-screens.md`](../ui/menu-screens.md), [`menu-camera.md`](menu-camera.md), [`../renderer/shadows.md`](../renderer/shadows.md), [`../scene/lights.md`](../scene/lights.md).
- Ułamek i sufit: dowolny podręcznik matematyki, funkcje `ceil` i `floor`.
