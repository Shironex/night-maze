# Moduł game: ekrany gry jako zwykłe dane, przejścia, Escape, ustawienia i przełącznik `--play`

Kamień milowy: M9, część 2 (2026-10-06), uzupełniony w części 3 (2026-10-06: ekrany ustawień, nowy labirynt na ekranie wyniku, wstrzymanie po utracie fokusu, poziom trudności). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z rundy i jej stanów ([`gameplay.md`](gameplay.md): `Round`, `RoundState`, `updateRound`, `animationSeconds`), ze stałego kroku ([`../core/main-loop.md`](../core/main-loop.md)), z kamery menu ([`menu-camera.md`](menu-camera.md)) i z przełączników wiersza poleceń (tamże, sekcja 5.5). Warstwę, która pokazuje menu, opisuje [`../ui/README.md`](../ui/README.md), same ekrany [`../ui/menu-screens.md`](../ui/menu-screens.md), ustawienia [`settings.md`](settings.md), a poziomy trudności [`difficulty.md`](difficulty.md).
Kod: [`src/game/GameState.hpp`](../../../src/game/GameState.hpp) i [`GameState.cpp`](../../../src/game/GameState.cpp) (typy i czyste funkcje), użycie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`onEscapePressed`, `handleGameEvent`, `handleMenuActions`, `showScreen`, `startNewGame`, `fillRoundEndDocument`, `onUpdate`, `onRender`), [`src/game/StartOptions.hpp`](../../../src/game/StartOptions.hpp) i [`StartOptions.cpp`](../../../src/game/StartOptions.cpp) (`--play`), [`src/core/Application.cpp`](../../../src/core/Application.cpp) (`onEscapePressed`), [`src/main.cpp`](../../../src/main.cpp), [`src/debug/DebugContext.hpp`](../../../src/debug/DebugContext.hpp) i [`DebugUI.cpp`](../../../src/debug/DebugUI.cpp) (`hudVisible`). Testy: [`tests/GameStateTests.cpp`](../../../tests/GameStateTests.cpp) (od części 3 26 przypadków, w części 2 było 21) i [`tests/StartOptionsTests.cpp`](../../../tests/StartOptionsTests.cpp) (od części 3 10 przypadków, w części 2 było 8).

**Stan na dziś (po części 3):** gra ma **siedem wartości `GameMode`**: `MainMenu`, `Playing`, `Paused`, `RoundEnd`, `Quitting` i dwa ekrany ustawień, `SettingsFromMenu` i `SettingsFromPause`. Zdarzeń jest **jedenaście** (siedem z części 2 plus `OpenSettings`, `CloseSettings`, `NewMaze` i `FocusLost`). Startuje w menu głównym. Reguły przejść i pytania "co ten ekran pozwala" są **zwykłymi danymi i czystymi funkcjami** w bibliotece `game_logic`, bez okna, OpenGL i RmlUi, więc mają testy. Aplikacja trzyma jeden `GameMode`, wysyła do niego zdarzenia i zadaje pytania, zamiast sama rozstrzygać, co ekran pozwala. Ekran ustawień jest osobną wartością dla każdego miejsca, z którego go otwarto, więc **sam ekran pamięta, dokąd prowadzi "wstecz" i co pokazuje za sobą**. Poziomy trudności z liczbami są ([`difficulty.md`](difficulty.md)), a decyzje o zakresie menu zostały zapisane w [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md).

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę (2026-10-06), nie powtórzone przy pisaniu.** Stan po części 3: bramka na gałęzi menu (przed scaleniem z oknem debug) zgłosiła **557 przypadków testowych i 220100 asercji**, a bramka na gałęzi okna debug **526 i 219214** (każda osobno, każda przed scaleniem). **Dla scalonego drzewa żadna bramka nie zgłosiła liczb**: pełna bramka nie została na nim uruchomiona (przebieg przerwał system z braku pamięci, a właściciel zdecydował, że tego dnia go pomija). Stan z części 2: `make check` przechodzi, **519 przypadków testowych i 219195 asercji** na commicie `8c99911` (Stan z 2026-10-06 po wymianie paneli na okno debugowania, liczby zgłoszone przez bramkę na gałęzi debug, nie powtórzone przeze mnie: **526 przypadków testowych i 219214 asercji**, siedem przypadków i 19 asercji więcej w `tests/SearchTests.cpp`; `DebugContext` ma 51 pól.) (przed częścią 497 i 219050). Przypadki policzyłem z plików: 21 w `GameStateTests.cpp`, 1 nowy w `StartOptionsTests.cpp`, razem 22, a 497 + 22 = 519 (suma makr `TEST_CASE` w `tests/*.cpp` to 519). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela** (Windows, Release, 1280 x 720): menu główne po starcie nad przelatującą kamerą, bez HUD i minimapy; `Play` zaczyna rundę z HUD i minimapą; Escape pokazuje pauzę, HUD znika, minimapa jest przyciemniona pod menu, a czas rundy stoi; klawisze F, M i R w pauzie nic nie robią; `Resume` przez Escape; `Restart` (czas wraca do 0:01); `Back to menu`; ponowne `Play` (labirynt zbudowany od nowa ze zmienioną liczbą kartek); `Quit` (proces kończy się kodem 0); `--play` startuje od razu w rundzie; F2 włącza i wyłącza; `--menu-camera --menu-shot glide` bez dokumentu, Escape pokazuje pauzę, drugi Escape ją chowa. **Ekran wyniku** widziany tylko przez tymczasową, niezatwierdzoną linię w jednorazowym buildzie, która po trzech sekundach rundy ustawiała `RoundState::Won`: "You escaped", Time 0:03, Crystals 0 / 13, bez HUD i minimapy, panele nad nim, `Restart`, `Back to menu` i Escape do menu głównego. **Przejścia całej gry do wygranej nikt nie zagrał.**

2a. **Widziane na zrzucie ekranu przez agenta w części 3 (2026-10-06), nie przez właściciela** (Windows, Release, gałąź menu przed scaleniem): ustawienia z menu głównego i z pauzy; automatyczna pauza po oddaniu pierwszego planu pasku zadań; `Regenerate` z okna debug na ekranie wyniku opuszczające ekran; **prawdziwy koniec rundy** na `Easy`, ziarno 7, osiągnięty lotem z wyłączonymi kolizjami (klawisz `N`) od kryształu do kryształu (nie chodzeniem): ekran wyniku z czasem 0:23. Nie widziane: przycisk `Restart` okna debug na ekranie wyniku, wszystko na macOS.
3. **Otwarte:** lista właściciela ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcje 26.2 i 28.2) i macOS ([`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcje "M9, część 2 (menu w RmlUi) na macOS" i "M9, część 3 (ekrany menu, poziomy trudności, ustawienia) na macOS"). Właściciel zagrał 2026-10-06 na `Hard` w buildzie Debug i zgłosił, że menu, okno debug i gra działają ("it was great"): to relacja właściciela, nie zamknięta lista.

Liczby w sekcji 2.6 (przykład) są **ilustracją policzoną ręcznie z kodu**, nie pomiarem z gry.

## 1. Po co to jest

### 1.1 Do czego służą ekrany

Do M9 gra miała tylko `RoundState` (`Playing` i `Won`) i nic poza nim: nie było menu, pauzy ani ekranu wyniku, a Escape zamykał program. Z menu pojawia się pytanie "na jakim ekranie jesteśmy" i każda część gry musi na nie odpowiadać zgodnie: czy działa runda, czy rysować HUD, czy kursor jest wolny. Gdyby każda część miała własne `if`, rozjechałyby się. Dlatego odpowiedzi są w **jednym miejscu** (`GameState`), jako funkcje jednej wartości `GameMode`.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu:

1. Escape zatrzymuje grę i cofa o jeden ekran, wyjście z programu to przycisk menu ([`../../decisions/escape-pauses-and-goes-back.md`](../../decisions/escape-pauses-and-goes-back.md)).
2. Zakres menu w M9: menu główne, pauza, koniec rundy i ustawienia, trzy poziomy trudności, teksty po angielsku ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md)).
3. Menu w RmlUi ([`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md)).

**Wszystko inne jest wyborem wykonawczym**: że stany są `enum class`, a przejścia jedną funkcją `nextMode` z wierszami dla każdego ekranu, że zdarzenie bez znaczenia zostawia ekran bez zmian, że HUD jest tylko w rundzie, że minimapa zostaje w pauzie, że pauza zamraża zegar animacji, że menu główne pożycza wysoki przelot kamery menu, że bez dokumentów menu gra startuje w rundzie.

## 2. Teoria

### 2.1 Automat skończony

Zachowanie gry zależy od ekranu, na którym jest, i od zdarzenia, które właśnie nastąpiło. To **automat skończony** (finite state machine): zbiór stanów, zbiór zdarzeń i funkcja, która dla pary (stan, zdarzenie) mówi, jaki stan jest następny. Tu stanów jest siedem (`GameMode`), zdarzeń jedenaście (`GameEvent`), a funkcja to `nextMode`. Para, której tabela nie wymienia, **zostawia stan bez zmian**: wołający może wysłać dowolne zdarzenie w dowolnym momencie, bez sprawdzania wcześniej, czy ma sens.

Tabela przejść (wszystkie wiersze; kod w sekcji 5.3, tabela w komentarzu w `GameState.hpp`):

| Ekran | Zdarzenie | Następny ekran | Skutek uboczny w `NightMazeApp::handleGameEvent` |
|---|---|---|---|
| `MainMenu` | `Play` | `Playing` | najpierw `readSeedField` (ziarno z pola; nieczytelne ziarno zostawia menu bez zdarzenia), potem `startNewGame(m_newGame)`: labirynt zbudowany od nowa, nowa runda |
| `MainMenu` | `OpenSettings` | `SettingsFromMenu` | brak |
| `MainMenu` | `Quit` | `Quitting` | `window().requestClose()` |
| `Playing` | `Escape` albo `FocusLost` | `Paused` | brak |
| `Playing` | `RoundWon` | `RoundEnd` | brak (czas i kryształy wpisuje do dokumentu `showScreen`) |
| `Paused` | `Escape` albo `Resume` | `Playing` | brak, runda trwa dalej |
| `Paused` | `Restart` | `Playing` | `beginRound()`: ten sam labirynt, to samo ziarno i poziom, runda od początku |
| `Paused` | `OpenSettings` | `SettingsFromPause` | brak |
| `Paused` | `BackToMenu` | `MainMenu` | `rollSeed()` (runda jest porzucona) |
| `RoundEnd` | `Restart` ("Play again") | `Playing` | `beginRound()` |
| `RoundEnd` | `NewMaze` | `Playing` | `m_newGame.seed = randomSeed()`, potem `startNewGame` (ten sam poziom, inne ziarno) |
| `RoundEnd` | `BackToMenu` albo `Escape` | `MainMenu` | `rollSeed()` |
| `SettingsFromMenu` | `CloseSettings` albo `Escape` | `MainMenu` | `saveSettings()`; ziarno z pola zostaje |
| `SettingsFromPause` | `CloseSettings` albo `Escape` | `Paused` | `saveSettings()` |
| każdy inny | cokolwiek innego | bez zmian | brak |

`FocusLost` ma znaczenie tylko na ekranie `Playing`. Na każdym innym ekranie zostawia ekran bez zmian, więc `handleGameEvent` wraca od razu.

`Quitting` jest stanem końcowym: nic z niego nie wychodzi. Przejść do niego można tylko z menu głównego (`Quit`).

### 2.2 Pytania zamiast `if`

Poza `nextMode` moduł ma funkcje, które odpowiadają na pytania o ekran. Reszta gry pyta je, a nie porównuje `GameMode` ze stałymi:

| Pytanie | `MainMenu` | `Playing` | `Paused` | `RoundEnd` | `SettingsFromMenu` | `SettingsFromPause` | Znaczenie |
|---|---|---|---|---|---|---|---|
| `updatesRound` | nie | tak | nie | nie | nie | nie | działają reguły rundy: gracz, bateria, kryształy, czas |
| `animatesScene` | tak | tak | **nie** | tak | tak | **nie** | idzie `Round::animationSeconds`: kołysanie kryształów i puls świateł |
| `isMenuOpen` | tak | nie | tak | tak | tak | tak | pokazany jest dokument, kursor jest wolny |
| `showsHud` | nie | tak | nie | nie | nie | nie | rysowany jest HUD |
| `showsMinimap` | nie | tak | tak | nie | nie | tak | rysowana jest minimapa |
| `usesMenuCamera` | tak | nie | nie | nie | tak | nie | obraz z kamery menu, a nie z oczu gracza |
| `drawsScene(tryb, tłoNaCałeOkno)` | fałsz tylko z tłem | tak | tak | tak | fałsz tylko z tłem | tak | czy scena jest rysowana |

**Każdy ekran ustawień jest traktowany jak ekran, z którego go otwarto** (test `the settings screen keeps the picture of the screen it was opened from`): ustawienia z pauzy stoją na zatrzymanym obrazie z minimapą pod spodem, a ustawienia z menu głównego na przelocie kamery menu. Stan `Quitting` jest we wszystkich funkcjach traktowany jak "nic": `updatesRound` fałsz, `isMenuOpen` fałsz, `animatesScene` prawda, `showsHud` fałsz.

Uwagi (z komentarzy w kodzie):

- **`animatesScene`:** pauza (i ustawienia otwarte z pauzy) zatrzymuje wszystko, co rusza się samo, więc stoi też `animationSeconds`. **Wiatr w trawie nie stoi:** shader trawy bierze czas z `glfwGetTime`, nie z `animationSeconds`, więc źdźbła nadal falują w menu pauzy.
- **`showsHud`:** HUD jest tylko w rundzie, bo Dear ImGui rysuje po RmlUi, więc HUD w pauzie leżałby na przyciskach. **`showsMinimap`:** minimapa jest rysowana przed menu, więc w pauzie zostaje pod przyciemnioną warstwą jako część zatrzymanego obrazu.
- **`usesMenuCamera`:** menu główne nie ma rundy do pokazania, więc pożycza kamerę menu (sekcja 2.5).
- **`drawsScene`:** jest **przetestowana, ale renderer jeszcze jej nie woła**: tła na całe okno (wideo) dziś nie ma ([`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md)). Każdy ekran pokazuje scenę, także pod menu.

### 2.3 Zdarzenia: przyciski, klawisz i sama runda

Jedenaście zdarzeń ma cztery źródła:

- **Przyciski menu:** `Play`, `Resume`, `Restart`, `BackToMenu`, `Quit`, `OpenSettings`, `CloseSettings` i `NewMaze`. Każdy przycisk niesie w dokumencie nazwę (`data-action`), a funkcja `eventForAction` zamienia nazwę na zdarzenie: `"play"`, `"resume"`, `"restart"`, `"menu"`, `"quit"`, `"settings"`, `"back"` i `"new-maze"`. Przyciski, które nie zmieniają ekranu (poziom, nowe ziarno, ustawienia okna), są **poleceniami ekranu**, a nie zdarzeniami ([`../ui/menu-screens.md`](../ui/menu-screens.md), sekcja 2.3). Nieznana nazwa daje fałsz i zostawia zdarzenie, więc gra zapisuje ostrzeżenie w logu.
- **Klawisz Escape:** `Escape` nie jest przyciskiem i nie ma nazwy.
- **Sama runda:** `RoundWon`, gdy krok reguł ustawi `RoundState::Won`.
- **Okno:** `FocusLost`, w klatce, w której okno przestaje być aktywne (`onRender`, [`settings.md`](settings.md), sekcja 2.6). Poza kamerą menu (narzędziem do nagrywania).

### 2.4 Escape

Pętla główna wywołuje wirtualną `Application::onEscapePressed()` po odczytaniu zdarzeń okna, **przed krokami stałymi**, więc to, co zmienia, obowiązuje w całej klatce. Domyślna wersja zamyka okno (program, który jej nie nadpisuje, działa jak dawniej), a `NightMazeApp` wysyła `GameEvent::Escape`. Funkcja nie jest wywoływana, gdy klawiatura jest zablokowana (edycja pola tekstowego w panelu). Escape **cofa o jeden ekran** i **nigdy nie zamyka programu**: w menu głównym nie robi nic, a wyjście to przycisk `Quit`.

### 2.5 Przełącznik `--play` i kamera menu

Gra startuje w menu głównym, chyba że:

- podano **`--play`**: start od razu w rundzie. Przełącznik `--seed N` wpisuje N do pola ziarna w menu głównym (`StartOptions::seedGiven`), a bez niego pole dostaje losowe ziarno (`rollSeed`); ziarno pierwszego labiryntu za menu to nadal `options.seed` (domyślnie 1), a jego rozmiar to poziom zapisany w pliku ustawień. Przełącznik bez wartości (następne słowo jest czytane jako kolejny przełącznik, test sprawdza, że `--play now` jest błędem). Lista przełączników ma pięć pozycji, a `StartOptions` cztery pola: `seed`, `seedGiven`, `menuCamera` i `play`. Służy testom, skryptom i nagrywaniu;
- podano **`--menu-camera`**: kamera menu jest narzędziem do nagrywania gry, więc też pomija menu główne, żeby żadne menu nie leżało na nagrywanym obrazie;
- **nie wczytał się któryś z czterech dokumentów menu**: gra startuje w rundzie (sekcja 5.5).

Kod w konstruktorze:

```cpp
      m_mode(options.play || options.menuCamera.enabled ? GameMode::Playing : GameMode::MainMenu),
```

(Plik: `src/game/NightMazeApp.cpp`, lista inicjalizacyjna konstruktora.)

**Jak kamera menu ma się do ekranów.** Są dwie różne rzeczy o podobnej nazwie:

- **Flaga `m_menuCamera.enabled`** zostaje tym, czym była: **narzędziem do nagrywania** (F2, kategoria Player, `--menu-camera`). Teraz działa **tylko w rundzie**: F2 jest odczytywany tylko gdy `updatesRound(m_mode)`. Włączona flaga chowa HUD, minimapę i panele (robi to `main.cpp`, który czyta flagę).
- **Obraz z kamery menu w menu głównym** to coś innego: `usesMenuCamera(MainMenu)` mówi, że obraz bierze kamera menu, **bez ustawiania flagi**. Dzięki temu panele debug nie są chowane, a logika F2 w `main.cpp` pozostaje nietknięta. W menu głównym ustawienia kamery są kopią ustawień z flagi, ale z ujęciem wymuszonym na **wysoki przelot** (`MenuShot::HighGlide`), gdy flaga jest wyłączona:

```cpp
    const bool menuCamera = m_menuCamera.enabled || usesMenuCamera(m_mode);
    // The settings the menu camera uses in this frame. The main menu always shows the
    // high glide over the maze, whatever shot the recording tool is set to.
    MenuCameraSettings menuCameraSettings = m_menuCamera;
    if (!m_menuCamera.enabled) {
        menuCameraSettings.shot = MenuShot::HighGlide;
    }
```

(Plik: `src/game/NightMazeApp.cpp`, `onRender`; w pliku między pierwszą a drugą linią jest komentarz o dwóch pytaniach klatki, pominięty tutaj.)

Wysoki przelot jest stanem przejściowym: docelowo tłem menu głównego ma być pętla wideo ([`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md)), której odtwarzania jeszcze nie ma.

### 2.6 Przykład policzony ręcznie: start, `Play`, Escape, `Resume`, wygrana

Liczby czasu i kryształów są **ilustracją** (13 kryształów to wartość labiryntu 10 na 10, ziarno 1, z [`gameplay.md`](gameplay.md); czas 95,7 s i 11 zebranych to wymyślony przykład). Stan po każdym kroku, policzony z tabel z sekcji 2.1 i 2.2 i z kodu:

| Krok | Zdarzenie | `GameMode` | Dokument | Kursor | HUD | Minimapa | Runda biegnie | Zegar animacji |
|---|---|---|---|---|---|---|---|---|
| 0 | start programu | `MainMenu` | menu główne | wolny | nie | nie | nie | idzie |
| 1 | klik `Play` | `Playing` | brak | przechwycony | tak | tak | tak | idzie |
| 2 | 40 s gry | `Playing` | brak | przechwycony | tak | tak | tak (`elapsedSeconds` około 40) | idzie |
| 3 | Escape | `Paused` | pauza | wolny | nie | tak (pod menu) | nie (stoi na około 40) | **stoi** |
| 4 | Escape (albo `Resume`) | `Playing` | brak | przechwycony | tak | tak | tak (dalej od około 40) | idzie |
| 5 | 55,7 s później gracz wchodzi w otwartą bramę | `RoundEnd` | wynik | wolny | nie | nie | nie | idzie |
| 6 | klik `Back to menu` | `MainMenu` | menu główne | wolny | nie | nie | nie | idzie |

Krok po kroku:

- **Krok 1.** `nextMode(MainMenu, Play)` daje `Playing`, a że `startsNewGame(MainMenu, Play)` jest prawdą (od części 3 `startsRound` jest tylko dla `Restart`), a przedtem bramka w `handleMenuActions` przeczytała ziarno z pola (`readSeedField`), wołane jest `startNewGame(m_newGame)`, czyli labirynt od nowa z ziarna, nowa runda. `showScreen` chowa dokument i przechwytuje kursor.
- **Krok 3.** Escape: `nextMode(Playing, Escape)` daje `Paused`, `startsRound` jest fałszem, więc **żadnej nowej rundy**. `showScreen` pokazuje dokument pauzy i oddaje kursor. Od następnego kroku stałego `onUpdate` kończy się wcześnie, bo `updatesRound(Paused)` jest fałszem, a `animatesScene(Paused)` też, więc nie rośnie nawet `animationSeconds`.
- **Krok 4.** `nextMode(Paused, Escape)` (albo `Resume`) daje `Playing`. Nic nie zaczyna rundy od nowa, więc `elapsedSeconds` rośnie dalej od około 40. Kursor jest znów przechwycony.
- **Krok 5.** Krok stały ustawia `RoundState::Won` i na końcu `onUpdate` wysyła `RoundWon`. `nextMode(Playing, RoundWon)` daje `RoundEnd`. `showScreen` wypełnia dokument wyniku: **czas 95,7 s** daje tekst `1:35` (`timeText` obcina ułamek: 95 / 60 = 1 minuta, 95 % 60 = 35 s, a 35 jest co najmniej 10, więc bez zera przed liczbą), kryształy `11 / 13`. Kolejne kroki stałe w tej samej klatce znajdują rundę zatrzymaną (`updatesRound(RoundEnd)` fałsz).
- **Krok 6.** `nextMode(RoundEnd, BackToMenu)` daje `MainMenu`. Rundy nikt nie zaczyna: przy `Play` powstanie nowa.

Gdyby w kroku 6 był `Restart`, wynik byłby inny: `Playing` i `beginRound()` na tym samym labiryncie. Gdyby w kroku 3 gracz kliknął `Restart` w pauzie, też byłoby `Playing` i `beginRound()`, a `elapsedSeconds` wróciłoby do zera.

## 3. Jak to działa w OpenGL

Moduł nie woła OpenGL i nie ma nowych obiektów OpenGL: to dane i czyste funkcje, jak reszta `game_logic`. Ekran wpływa na klatkę przez pytania z sekcji 2.2: czy rysować minimapę (`showsMinimap`), czy HUD (`hudVisible`, przez `DebugContext`), z której kamery brać obraz (`usesMenuCamera`) i czy pokazywać dokument (`showScreen`). Menu rysuje na końcu `onRender` warstwa `ui` ([`../ui/README.md`](../ui/README.md), sekcja 2.5).

## 4. Shadery

Brak: moduł nie zmienia żadnego shadera. Menu rysuje RmlUi własnymi shaderami, których ta gra nie przeładowuje ([`../ui/README.md`](../ui/README.md), sekcja 4).

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| `src/game/GameState.hpp`, `.cpp` | `GameMode`, `GameEvent`, `NewGame`, `nextMode`, `startsRound`, `startsNewGame`, `eventForAction`, `updatesRound`, `animatesScene`, `isMenuOpen`, `showsHud`, `showsMinimap`, `usesMenuCamera`, `drawsScene`. Część `game_logic`: czysta logika, bez okna i bez dołączania `ui` |
| `src/game/NightMazeApp.hpp`, `.cpp` | składowe `m_mode`, `m_newGame`, `m_settings`, `m_windowWasFocused`, `onEscapePressed`, `handleGameEvent`, `handleMenuActions`, `handleMenuCommand`, `handleControlChanges`, `readSeedField`, `rollSeed`, `showScreen`, `startNewGame`, cztery funkcje `fill*Document`, akcesory `gameMode()` i `hudVisible()` |
| `src/core/Application.*` | wirtualna `onEscapePressed()` |
| `src/game/StartOptions.*` | pole `play` i przełącznik `--play`, lista przełączników ma pięć pozycji; od części 3 pole `seedGiven` i publiczne `parseSeed` (czyta je też pole ziarna w menu) |
| `src/debug/DebugContext.hpp`, `DebugUI.cpp` | pole `hudVisible` (50 pól) i warunek rysowania HUD |
| `src/game/Difficulty.*`, `src/game/Settings.*` | typ `Difficulty` (przeniesiony z `GameState.hpp`) i tabela poziomów ([`difficulty.md`](difficulty.md)); ustawienia i plik ([`settings.md`](settings.md)) |
| `tests/GameStateTests.cpp` | 26 przypadków testowych (21 w części 2) |

### 5.2 Typy

`GameMode` ma od części 3 **siedem** wartości, `GameEvent` **jedenaście**, a `NewGame` niesie poziom i ziarno. Wszystkie trzy w jednym fragmencie nagłówka (z komentarzami):

```cpp
/// The screen the game is on. Exactly one at a time.
enum class GameMode {
    MainMenu = 0, ///< the main menu, the first thing after the start
    Playing,      ///< a round is being played
    Paused,       ///< a round is stopped, the pause menu is shown over it
    RoundEnd,     ///< the round is won, its result is shown
    Quitting,     ///< the player asked to leave: the program closes its window
    /// The settings screen, opened from the main menu. It is a screen of its own for
    /// each place it can be opened from, so the screen itself remembers where "back"
    /// leads and what is shown behind it.
    SettingsFromMenu,
    SettingsFromPause, ///< the settings screen, opened from the pause menu
};

/// Something that can change the screen: a button of a menu, the Escape key, or the
/// round itself.
enum class GameEvent {
    Play = 0,      ///< button "Play" of the main menu: start a new game
    Resume,        ///< button "Resume" of the pause menu
    Restart,       ///< button "Restart": the same maze again, from the start
    BackToMenu,    ///< button "Back to menu"
    Quit,          ///< button "Quit" of the main menu
    Escape,        ///< the Escape key
    RoundWon,      ///< the player walked through the open gate (RoundState::Won)
    OpenSettings,  ///< button "Settings" of the main menu and of the pause menu
    CloseSettings, ///< button "Back" of the settings screen
    NewMaze,       ///< button "New maze" of the result screen: a new game, another maze
    FocusLost,     ///< the window of the game stopped being the active window
};

/// What the button "Play" asks for: the game that is started next.
struct NewGame {
    /// The level: the size of the maze, its crystals, the gate and the battery
    /// (game::difficultyLevel).
    Difficulty difficulty = Difficulty::Normal;

    /// The seed of the maze (MazeSettings::seed).
    std::uint32_t seed = DEFAULT_MAZE_SEED;
};
```

(Plik: `src/game/GameState.hpp`.) `GameMode` ma `MainMenu = 0`, `Playing`, `Paused`, `RoundEnd`, `Quitting`, `SettingsFromMenu` i `SettingsFromPause`; `GameEvent` ma `Play = 0`, `Resume`, `Restart`, `BackToMenu`, `Quit`, `Escape`, `RoundWon`, `OpenSettings`, `CloseSettings`, `NewMaze` i `FocusLost`. **`Difficulty` nie leży już w tym pliku**: od części 3 jest w `Difficulty.hpp` razem z tabelą liczb ([`difficulty.md`](difficulty.md)). **Poziom trudności jest czytany**: `startNewGame` kopiuje wiersz tabeli poziomu do prośby o labirynt i do reguł rundy. **`m_newGame.seed` już nie podąża za `regenerateMaze`** (w części 2 podążało): ziarno `Play` pochodzi z pola ziarna w menu głównym (`readSeedField`), a `New maze` losuje nowe. Ziarno pierwszego labiryntu za menu to `options.seed` z wiersza poleceń (domyślnie 1), a pole ziarna dostaje osobną, losową liczbę (albo ziarno z `--seed`).



### 5.3 `nextMode`

```cpp
GameMode nextMode(GameMode mode, GameEvent event) {
    // One block per screen: the events that mean something there. Everything else
    // falls through to the last line and changes nothing.
    switch (mode) {
    case GameMode::MainMenu:
        if (event == GameEvent::Play) {
            return GameMode::Playing;
        }
        if (event == GameEvent::Quit) {
            return GameMode::Quitting;
        }
        if (event == GameEvent::OpenSettings) {
            return GameMode::SettingsFromMenu;
        }
        break;
    case GameMode::Playing:
        // A player who switches to another program does not want the round to go on
        // without them: losing the focus pauses like the Escape key.
        if (event == GameEvent::Escape || event == GameEvent::FocusLost) {
            return GameMode::Paused;
        }
        if (event == GameEvent::RoundWon) {
            return GameMode::RoundEnd;
        }
        break;
    case GameMode::Paused:
        if (event == GameEvent::Escape || event == GameEvent::Resume ||
            event == GameEvent::Restart) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu) {
            return GameMode::MainMenu;
        }
        if (event == GameEvent::OpenSettings) {
            return GameMode::SettingsFromPause;
        }
        break;
    case GameMode::RoundEnd:
        if (event == GameEvent::Restart || event == GameEvent::NewMaze) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::SettingsFromMenu:
        if (event == GameEvent::CloseSettings || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::SettingsFromPause:
        if (event == GameEvent::CloseSettings || event == GameEvent::Escape) {
            return GameMode::Paused;
        }
        break;
    case GameMode::Quitting:
        // The program is closing: nothing brings it back.
        break;
    }
    return mode;
}
```

(Plik: `src/game/GameState.cpp`, funkcja `nextMode`, w całości, z komentarzami.) Jeden blok na ekran, w nim zdarzenia, które tam coś znaczą. Wszystko inne wypada przez `break` do ostatniej linii i **nic nie zmienia**. Dlatego test "jeden ekran, wszystkie zdarzenia" może przejść po wszystkich parach. Dwie rzeczy z części 3: **`FocusLost` jest w bloku `Playing` obok `Escape`** (komentarz w kodzie: gracz, który przełącza się do innego programu, nie chce, żeby runda biegła bez niego), a ekrany ustawień mają po jednym bloku z `CloseSettings` i `Escape`, każdy z innym celem.

Dwa pytania o zdarzenie, zadawane **przed** zmianą ekranu, bo odpowiedź zależy od ekranu, na którym je wysłano:

```cpp
bool startsRound(GameMode mode, GameEvent event) {
    if (event == GameEvent::Restart) {
        return mode == GameMode::Paused || mode == GameMode::RoundEnd;
    }
    return false;
}

bool startsNewGame(GameMode mode, GameEvent event) {
    if (event == GameEvent::Play) {
        return mode == GameMode::MainMenu;
    }
    if (event == GameEvent::NewMaze) {
        return mode == GameMode::RoundEnd;
    }
    return false;
}
```

(Plik: `src/game/GameState.cpp`.) `startsRound` odpowiada, czy zdarzenie **zaczyna rundę od początku na tym samym labiryncie**: tylko `Restart` w pauzie i na ekranie wyniku (w części 2 także `Play`). `startsNewGame` odpowiada, czy zdarzenie **buduje nowy labirynt i zaczyna w nim rundę**: `Play` w menu głównym i `NewMaze` na ekranie wyniku.

### 5.4 `handleGameEvent`: co robi aplikacja po zdarzeniu

```cpp
void NightMazeApp::handleGameEvent(GameEvent event) {
    const GameMode before = m_mode;
    // Asked before the screen changes: the answers depend on the screen the event
    // was sent on.
    const bool newRound = startsRound(before, event);
    const bool newGame = startsNewGame(before, event);
    m_mode = nextMode(before, event);

    // Without the documents only the pause can be entered: it shows nothing, but Escape
    // leaves it again. The main menu and the result screen have no key that leaves
    // them, so a round is started in their place.
    bool roundInsteadOfMenu = false;
    if (!m_menusLoaded && (m_mode == GameMode::MainMenu || m_mode == GameMode::RoundEnd)) {
        m_mode = GameMode::Playing;
        roundInsteadOfMenu = true;
    }

    if (m_mode == before && !roundInsteadOfMenu) {
        return;
    }
    if (newGame) {
        // "New maze" on the result screen: the same difficulty, another seed. "Play"
        // in the main menu starts the seed its field shows (handleMenuActions has read
        // it).
        if (event == GameEvent::NewMaze) {
            m_newGame.seed = randomSeed();
        }
        startNewGame(m_newGame);
    } else if (newRound || roundInsteadOfMenu) {
        beginRound();
    }

    // The settings screen was left: what was changed there goes into the file.
    const bool wasSettings =
        before == GameMode::SettingsFromMenu || before == GameMode::SettingsFromPause;
    if (wasSettings) {
        saveSettings();
    }
    // Back in the main menu after a game: the next game gets a fresh seed. Coming back
    // from the settings the seed stays, with whatever was typed into its field.
    if (m_mode == GameMode::MainMenu && !wasSettings) {
        rollSeed();
    }

    if (m_mode == GameMode::Quitting) {
        // The main loop ends after this frame (core::Application::run).
        window().requestClose();
    }
    showScreen();
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `handleGameEvent`, w całości, z komentarzami.) Kolejność: (1) zapamiętać ekran i zapytać `startsRound` i `startsNewGame`; (2) policzyć nowy ekran; (3) obsłużyć brak dokumentów; (4) jeśli ekran się nie zmienił, wyjść (tak kończą się zdarzenia bez znaczenia, także `FocusLost` poza rundą); (5) **nowa gra** (`Play`, `NewMaze`) buduje labirynt przez `startNewGame`, a `Restart` woła `beginRound`; `NewMaze` losuje przedtem nowe ziarno (`randomSeed`) i zachowuje poziom; (6) wyjście z ekranu ustawień zapisuje plik (`saveSettings`); (7) powrót do menu głównego **po grze** losuje nowe ziarno w polu (`rollSeed`), a powrót **z ustawień** zostawia ziarno i to, co w polu wpisano; (8) `Quitting` zamyka okno; (9) `showScreen` dopasowuje okno do ekranu. **Nowa gra ma pierwszeństwo przed `beginRound`**, bo `startNewGame` woła `regenerateMaze`, a ono samo zaczyna rundę.

### 5.5 Bez dokumentów menu

Jeśli któryś z czterech dokumentów się nie wczytał, konstruktor zapisuje błąd i ustawia `Playing`. Z ekranów menu da się jeszcze wejść tylko do pauzy (nic nie pokazuje, ale Escape z niej wychodzi). Menu główne i ekran wyniku nie mają klawisza wyjścia, więc zamiast nich zaczyna się nowa runda (zmienna `roundInsteadOfMenu`). Czyli bez menu gra jest ciągłą rundą z pauzą pod Escape. To obsługa błędu, nie decyzja właściciela.

### 5.6 `showScreen`: okno dopasowane do ekranu

```cpp
void NightMazeApp::showScreen() {
    // The document of the screen, filled with what it shows at this moment.
    ui::DocumentId document = ui::NO_DOCUMENT;
    if (m_mode == GameMode::MainMenu) {
        document = m_mainMenuDocument;
        fillMainMenuDocument();
    } else if (m_mode == GameMode::Paused) {
        document = m_pauseDocument;
        fillPauseDocument();
    } else if (m_mode == GameMode::RoundEnd) {
        document = m_roundEndDocument;
        fillRoundEndDocument();
    } else if (m_mode == GameMode::SettingsFromMenu || m_mode == GameMode::SettingsFromPause) {
        document = m_settingsDocument;
        fillSettingsDocument();
    }
    m_ui.show(document);

    // The cursor follows the screen: captured for mouse look while a round is played,
    // free for the buttons of a menu. The menu camera does not turn with the mouse, so
    // it leaves the cursor free too.
    input().setCursorCaptured(updatesRound(m_mode) && !m_menuCamera.enabled);
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `showScreen`, w całości, z komentarzami.) Funkcja wybiera dokument ekranu, **wypełnia go bieżącymi liczbami** (`fillMainMenuDocument`, `fillPauseDocument`, `fillRoundEndDocument`, `fillSettingsDocument`: [`../ui/menu-screens.md`](../ui/menu-screens.md)) i pokazuje. **Kursor idzie za ekranem:** przechwycony w rundzie (do obrotu myszą), wolny na każdym ekranie z menu. Kamera menu nie obraca się myszą, więc też zostawia kursor wolny. Pokazanie panelu klawiszem tyldy oddaje kursor (kod w `main.cpp`), a kliknięcie w scenę przechwytuje go ponownie (`handleInteraction`, bez zmian). **Klik w scenę nie jest już drogą do obrotu myszą:** kursor jest przechwytywany przy starcie rundy.

### 5.7 Krok stały i wygrana, klatka i klawisze rundy

W `onUpdate`:

```cpp
    if (!updatesRound(m_mode) || m_menuCamera.enabled) {
        if (animatesScene(m_mode)) {
            m_round.animationSeconds += static_cast<float>(fixedDt);
        }
        return;
    }
```

(Plik: `src/game/NightMazeApp.cpp`, `onUpdate`.) Poza rundą krok kończy się wcześnie. Zegar animacji idzie wszędzie poza pauzą. Na końcu kroku, w którym gracz przeszedł przez otwartą bramę, stoi:

```cpp
    if (m_round.state == RoundState::Won) {
        handleGameEvent(GameEvent::RoundWon);
    }
```

(Plik: `src/game/NightMazeApp.cpp`, koniec `onUpdate`.) Wywołanie jest w kroku stałym, nie w klatce, więc kroki, które nastąpią później w tej samej klatce, już zastają zatrzymaną rundę.

W `onRender` klawisze i mysz rundy (R, N, F, M, E, kliknięcie, obrót myszą) mają jeden warunek:

```cpp
    const bool roundInput = updatesRound(m_mode) && !m_menuCamera.enabled;
```

(Plik: `src/game/NightMazeApp.cpp`, `onRender`.) To pytanie o stan gry, a nie blokada w `core::Input`, więc **nie ma jednoklatkowego opóźnienia**. Menu niczego w `core::Input` nie blokuje ([`../ui/README.md`](../ui/README.md), sekcja 2.6). F2 działa tylko, gdy `updatesRound`. Klawisz tyldy do paneli działa na każdym ekranie.

### 5.8 Testy

`GameStateTests.cpp` (26 przypadków; 21 w części 2, pięć nowych w części 3), wszystkie bez okna: menu główne zaczyna grę przyciskiem `Play` i wychodzi `Quit`; w menu głównym Escape i przyciski innych ekranów nic nie robią; Escape pauzuje i znów wznawia; pauza wznawia, restartuje albo wraca do menu (i `Quit`, `Play` w pauzie nic nie robią); wygrana idzie do wyniku tylko w rundzie; w rundzie przyciski menu nic nie robią; wynik restartuje albo wraca do menu, także Escape; z `Quitting` nic nie wychodzi; tylko menu główne dochodzi do `Quitting`; nazwy przycisków dają zdarzenia; nieznana nazwa nie daje zdarzenia; runda biegnie tylko w `Playing`; menu jest otwarte na każdym ekranie poza grą; żaden ekran nie uruchamia rundy pod otwartym menu; HUD należy do rundy, a minimapa także do pauzy; scena jest pomijana tylko za menu głównym z tłem na całe okno; nowa gra prosi o normalny poziom i domyślne ziarno. **Nowe lub zmienione w części 3:**

| Test | Co sprawdza |
|---|---|
| `the settings open from the main menu and from the pause menu, and go back there` | `OpenSettings` z menu głównego prowadzi do `SettingsFromMenu` i `CloseSettings` wraca do `MainMenu`; z pauzy do `SettingsFromPause` i z powrotem do `Paused` |
| `on the settings screen only Back and Escape do something` | na ekranach ustawień znaczą tylko `CloseSettings` i `Escape` |
| `losing the focus pauses a running round and changes no other screen` | `FocusLost` daje `Paused` tylko na `Playing` |
| `a round starts from the beginning on the same maze with Restart` (dawniej z `Play`) | `startsRound` tylko dla `Restart` w pauzie i na wyniku |
| `a new game starts with Play in the main menu and with New maze after a round` | `startsNewGame` |
| `every event that starts a round or a game also leads into the game` | każde takie zdarzenie prowadzi do `Playing` |
| `the scene stops moving only in the pause menu and in its settings` | `animatesScene` |
| `only the main menu and its settings are shown through the menu camera` | `usesMenuCamera` |
| `the settings screen keeps the picture of the screen it was opened from` | `animatesScene`, `showsMinimap`, `usesMenuCamera`, `drawsScene` dla ekranu ustawień jak dla ekranu, z którego go otwarto |

`StartOptionsTests.cpp` ma od części 3 10 przypadków (8 w części 2): doszły `a seed on the command line is remembered as given, also the default one` (`seedGiven`) i `a seed is read from digits only, up to the largest 32 bit number` (`parseSeed`, wspólne z polem ziarna). Dołożony w części 2 przypadek `--play` zostaje: ustawia pole `play`, może stać w dowolnym miejscu i nie przyjmuje wartości.

**Nie ma testu na:** `NightMazeApp::handleGameEvent`, `showScreen` i kursor (kod z OpenGL i okna), kolejność klatki, wstrzymanie po utracie fokusu (kod z oknem), ani na to, że `drawsScene` jest użyte (nie jest).

### 5.9 Jak to sprawdzono

Patrz "Uczciwie o tym, co sprawdzono" na początku: bramka (zgłoszona), zrzuty agenta (nie właściciela), lista właściciela otwarta.

## 6. Okno debugowania (dawniej panel ImGui)

Brak panelu o ekranach. Pole `hudVisible` w `DebugContext` mówi `DebugUI::draw`, czy rysować HUD:

```cpp
    if (context.hudVisible) {
        drawHud(context.mazeWorld, context.round, context.gameplay, context.pick, m_visible);
    }
```

(Plik: `src/debug/DebugUI.cpp`.) `main.cpp` podaje `hudVisible()` z `NightMazeApp`: prawda tylko w rundzie i bez kamery menu. Zastąpiło to warunek `!context.menuCamera.enabled`. W pliku `Hud.cpp` nic się nie zmieniło: jego karta "You escaped" i tekst "press R" **nie mogą się już pojawić**, bo HUD jest ukryty na ekranie wyniku (ekran wyniku jest teraz dokumentem RmlUi). `R` nadal restartuje rundę w trakcie gry.

## 7. Pułapki

1. **`Quit` jest tylko w menu głównym.** Z pauzy trzeba wrócić do menu głównego przyciskiem `Back to menu`, a stamtąd wyjść. Escape nigdy nie zamyka programu.
2. **Pauza nie zatrzymuje wiatru w trawie.** Ten ruch idzie od `glfwGetTime`, nie od `animationSeconds`.
3. **Zmiana ekranu w kroku stałym.** `RoundWon` jest wysyłane z `onUpdate`, więc `showScreen` (kursor, dokument) wykonuje się w środku kroku stałego. Działa, bo nic w tym kroku już nie zależy od ekranu.
4. **`Play` i `New maze` budują labirynt od nowa.** Także dla ziarna, które już gra: poziom mógł się zmienić, a liczby dźwigni i kartek w `m_mazeSettings` mogły się zmienić w oknie debug (komentarz w kodzie `startNewGame`).
5. **`Back to menu` porzuca rundę.** Nie ma zapisu ani wznawiania: następne `Play` zaczyna nową grę.
6. **Poziom trudności zmienia grę** (od części 3). Liczby `Normal` i `Hard` są propozycją autora kodu, którą właściciel dopracuje po zagraniu ([`difficulty.md`](difficulty.md)). Po przebudowie labiryntu z okna debug pauza i wynik pokazują poziom `Custom`.
7. **`--menu-camera` pomija menu główne**, a F2 nie działa w menu. Nagrywanie z menu głównego wymaga innego startu.
8. **Karta wygranej z `Hud.cpp` jest martwym kodem.** Kod nie został usunięty ani zmieniony w tej części.
9. **Bez dokumentów menu** wygląda to jak gra bez menu (sekcja 5.5), a jedyny ślad to wpis w logu.
10. **Pauza po utracie fokusu nie wznawia się sama** i nie działa przy kamerze menu ani wtedy, gdy okno startuje bez fokusu.
11. **Ziarno wraca do losowego po każdym wejściu do menu głównego z gry**, a nie po powrocie z ustawień.
12. **`RoundWon` i `FocusLost` nie mają nazw.** Pierwsze wysyła krok stały, drugie klatka: nie są przyciskami.

## 8. Ćwiczenia

1. **Tabela na kartce.** Dla trzech zdarzeń `Escape`, `Restart` i `Quit` wypisz następny ekran na każdym z czterech podstawowych ekranów (dwa ekrany ustawień pomiń: z nich Escape wraca do ekranu, z którego je otwarto, a `Restart` i `Quit` nic nie zmieniają). Odpowiedź: `Escape`: z `MainMenu` bez zmian, z `Playing` do `Paused`, z `Paused` do `Playing`, z `RoundEnd` do `MainMenu`. `Restart`: z `MainMenu` i z `Playing` bez zmian, z `Paused` i z `RoundEnd` do `Playing`. `Quit`: z `MainMenu` do `Quitting`, z pozostałych bez zmian.
2. **Które zdarzenia zaczynają rundę, a które nową grę?** Odpowiedź: rundę od początku na tym samym labiryncie zaczyna `Restart` w pauzie i na ekranie wyniku (`startsRound`, dwie pary). Nową grę z nowym labiryntem zaczynają `Play` w menu głównym i `NewMaze` na ekranie wyniku (`startsNewGame`, dwie pary). Żadne inne.
3. **Czas wyniku.** Co pokaże `timeText` dla 59,9 s, 60 s i 605 s? Odpowiedź: `0:59`, `1:00`, `10:05` (ułamek jest obcinany, pod 10 sekund dopisywane jest zero).
4. **Nowy ekran.** Dodaj ekran `Controls` dostępny z menu głównego i z pauzy (tak jak zrobiono ustawienia w części 3). Które funkcje z sekcji 2.2 trzeba uzupełnić, a które testy dopisać? Odpowiedź: dwie wartości `GameMode` (po jednej na miejsce otwarcia), zdarzenia (albo użyć istniejących `OpenSettings` i `CloseSettings`, jeśli ekran ma te same przejścia), `nextMode` (wiersze), `eventForAction` (nazwy), wszystkie pytania z sekcji 2.2 (co ekran pozwala, w szczególności gdzie ma wracać Escape), a testy po jednym przypadku na każdą nową parę i na każde pytanie. Poza tym nowy dokument RML i wypełnienie go w `showScreen`.
5. **`--play` i `--menu-camera`.** Dlaczego oba pomijają menu? Odpowiedź: `--play` dla testów i skryptów, `--menu-camera` dla nagrywania bez menu na obrazie.
6. **Wiatr w pauzie.** Zmień w `animatesScene` pauzę, żeby też szła: co się zmieni w menu pauzy? Odpowiedź: kryształy i światła dalej się poruszają pod menu, a ruch trawy bez zmian (zależy od `glfwGetTime`).

## 9. Pytania kontrolne

1. **Dlaczego ekrany są zwykłymi danymi i funkcjami w `game_logic`?**
   Żeby można je testować bez okna i żeby wszystkie części gry odpowiadały na pytanie "co ten ekran pozwala" w jednym miejscu.
2. **Co robi `nextMode` dla pary, której nie ma w tabeli?**
   Zwraca ten sam ekran. Wołający może wysłać dowolne zdarzenie w dowolnym momencie.
3. **Dlaczego `startsRound` pyta się przed zmianą ekranu?**
   Bo odpowiedź zależy od ekranu, na którym zdarzenie wysłano.
4. **Dlaczego HUD jest tylko w rundzie, a minimapa także w pauzie?**
   HUD rysuje Dear ImGui po RmlUi, więc w pauzie leżałby na przyciskach. Minimapa jest rysowana przed menu i zostaje pod nim jako część zatrzymanego obrazu.
5. **Co zatrzymuje pauza, a czego nie?**
   Rundę (gracz, bateria, kryształy, czas) i zegar animacji. Nie zatrzymuje wiatru w trawie, bo ten idzie od `glfwGetTime`.
6. **Dlaczego Escape nie zamyka programu?**
   Decyzja właściciela: Escape cofa o jeden ekran, a wyjście z programu to przycisk `Quit` w menu głównym.
7. **Jak kamera menu ma się do ekranów?**
   Flaga `m_menuCamera.enabled` to narzędzie do nagrywania, działa tylko w rundzie. Menu główne pożycza wysoki przelot przez `usesMenuCamera`, bez ustawiania flagi.
8. **Co się dzieje, gdy dokumenty menu się nie wczytają?**
   Błąd w logu, start w rundzie, a menu główne i ekran wyniku są zastępowane nową rundą.
9. **Gdzie jest wysyłane `RoundWon` i dlaczego tam?**
   Na końcu `onUpdate`, bo wtedy kroki stałe późniejsze w tej samej klatce zastają już zatrzymaną rundę.
10. **Dlaczego ustawienia mają dwie wartości `GameMode`, a nie jedną?**
    Żeby sam ekran pamiętał, dokąd prowadzi "wstecz" (do menu głównego albo do pauzy) i co pokazuje za sobą (przelot kamery menu albo zatrzymany obraz z minimapą).
11. **Co zaczyna rundę, a co nową grę?**
    `Restart` zaczyna rundę na tym samym labiryncie (`startsRound`). `Play` w menu głównym i `New maze` na wyniku budują nowy labirynt (`startsNewGame`).
12. **Kiedy i jak gra wstrzymuje się po utracie fokusu?**
    `onRender` w klatce, w której okno przestaje być aktywne, wysyła `FocusLost`; tylko `Playing` odpowiada pauzą, a kamera menu jest wyłączona z tej reguły.

## 10. Źródła

- Notatki: [`../../decisions/escape-pauses-and-goes-back.md`](../../decisions/escape-pauses-and-goes-back.md), [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md), [`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md), [`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md), [`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md).
- Dokumenty: [`../ui/README.md`](../ui/README.md), [`../ui/menu-screens.md`](../ui/menu-screens.md), [`difficulty.md`](difficulty.md), [`settings.md`](settings.md), [`../../libraries/rmlui.md`](../../libraries/rmlui.md), [`menu-camera.md`](menu-camera.md), [`gameplay.md`](gameplay.md), [`../core/main-loop.md`](../core/main-loop.md), [`../debug-ui.md`](../debug-ui.md).
- Automat skończony (finite state machine): dowolny podręcznik programowania gier, rozdział o stanach gry, albo hasło "finite-state machine" w encyklopedii.
