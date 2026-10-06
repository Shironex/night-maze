# Moduł game: ekrany gry jako zwykłe dane, przejścia, Escape i przełącznik `--play`

Kamień milowy: M9, część 2 (2026-10-06). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z rundy i jej stanów ([`gameplay.md`](gameplay.md): `Round`, `RoundState`, `updateRound`, `animationSeconds`), ze stałego kroku ([`../core/main-loop.md`](../core/main-loop.md)), z kamery menu ([`menu-camera.md`](menu-camera.md)) i z przełączników wiersza poleceń (tamże, sekcja 5.5). Warstwę, która pokazuje menu, opisuje [`../ui/README.md`](../ui/README.md).
Kod: [`src/game/GameState.hpp`](../../../src/game/GameState.hpp) i [`GameState.cpp`](../../../src/game/GameState.cpp) (typy i czyste funkcje), użycie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`onEscapePressed`, `handleGameEvent`, `handleMenuActions`, `showScreen`, `startNewGame`, `fillRoundEndDocument`, `onUpdate`, `onRender`), [`src/game/StartOptions.hpp`](../../../src/game/StartOptions.hpp) i [`StartOptions.cpp`](../../../src/game/StartOptions.cpp) (`--play`), [`src/core/Application.cpp`](../../../src/core/Application.cpp) (`onEscapePressed`), [`src/main.cpp`](../../../src/main.cpp), [`src/debug/DebugContext.hpp`](../../../src/debug/DebugContext.hpp) i [`DebugUI.cpp`](../../../src/debug/DebugUI.cpp) (`hudVisible`). Testy: [`tests/GameStateTests.cpp`](../../../tests/GameStateTests.cpp) (21 przypadków) i [`tests/StartOptionsTests.cpp`](../../../tests/StartOptionsTests.cpp) (8 przypadków, z czego jeden nowy: `--play`).

**Stan na dziś:** gra ma **cztery ekrany** i stan zamykania: `MainMenu`, `Playing`, `Paused`, `RoundEnd` i `Quitting`. Startuje w menu głównym. Reguły przejść i pytania "co ten ekran pozwala" są **zwykłymi danymi i czystymi funkcjami** w bibliotece `game_logic`, bez okna, OpenGL i RmlUi, więc mają testy. Aplikacja trzyma jeden `GameMode`, wysyła do niego zdarzenia i zadaje pytania, zamiast sama rozstrzygać, co ekran pozwala. **Ekranu ustawień i poziomów trudności z liczbami nie ma** ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md)).

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę (2026-10-06), nie powtórzone przy pisaniu:** `make check` przechodzi, **519 przypadków testowych i 219195 asercji** na commicie `8c99911` (przed częścią 497 i 219050). Przypadki policzyłem z plików: 21 w `GameStateTests.cpp`, 1 nowy w `StartOptionsTests.cpp`, razem 22, a 497 + 22 = 519 (suma makr `TEST_CASE` w `tests/*.cpp` to 519). Liczby asercji nie da się policzyć z plików, jest tylko zgłoszona.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela** (Windows, Release, 1280 x 720): menu główne po starcie nad przelatującą kamerą, bez HUD i minimapy; `Play` zaczyna rundę z HUD i minimapą; Escape pokazuje pauzę, HUD znika, minimapa jest przyciemniona pod menu, a czas rundy stoi; klawisze F, M i R w pauzie nic nie robią; `Resume` przez Escape; `Restart` (czas wraca do 0:01); `Back to menu`; ponowne `Play` (labirynt zbudowany od nowa ze zmienioną liczbą kartek); `Quit` (proces kończy się kodem 0); `--play` startuje od razu w rundzie; F2 włącza i wyłącza; `--menu-camera --menu-shot glide` bez dokumentu, Escape pokazuje pauzę, drugi Escape ją chowa. **Ekran wyniku** widziany tylko przez tymczasową, niezatwierdzoną linię w jednorazowym buildzie, która po trzech sekundach rundy ustawiała `RoundState::Won`: "You escaped", Time 0:03, Crystals 0 / 13, bez HUD i minimapy, panele nad nim, `Restart`, `Back to menu` i Escape do menu głównego. **Przejścia całej gry do wygranej nikt nie zagrał.**
3. **Otwarte:** lista właściciela ([`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 26.2) i macOS ([`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcja "M9, część 2 (menu w RmlUi) na macOS").

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

Zachowanie gry zależy od ekranu, na którym jest, i od zdarzenia, które właśnie nastąpiło. To **automat skończony** (finite state machine): zbiór stanów, zbiór zdarzeń i funkcja, która dla pary (stan, zdarzenie) mówi, jaki stan jest następny. Tu stanów jest pięć (`GameMode`), zdarzeń siedem (`GameEvent`), a funkcja to `nextMode`. Para, której tabela nie wymienia, **zostawia stan bez zmian**: wołający może wysłać dowolne zdarzenie w dowolnym momencie, bez sprawdzania wcześniej, czy ma sens.

Tabela przejść (wszystkie wiersze; kod w sekcji 5.3, tabela w komentarzu w `GameState.hpp`):

| Ekran | Zdarzenie | Następny ekran | Skutek uboczny w `NightMazeApp` |
|---|---|---|---|
| `MainMenu` | `Play` | `Playing` | `startNewGame(m_newGame)`: labirynt zbudowany od nowa z ziarna, nowa runda |
| `MainMenu` | `Quit` | `Quitting` | `window().requestClose()` |
| `Playing` | `Escape` | `Paused` | brak |
| `Playing` | `RoundWon` | `RoundEnd` | czas i kryształy wpisane do dokumentu wyniku |
| `Paused` | `Escape` albo `Resume` | `Playing` | brak, runda trwa dalej |
| `Paused` | `Restart` | `Playing` | `beginRound()`: ten sam labirynt, runda od początku |
| `Paused` | `BackToMenu` | `MainMenu` | brak (runda jest porzucona, `Play` zbuduje nową) |
| `RoundEnd` | `Restart` | `Playing` | `beginRound()` |
| `RoundEnd` | `BackToMenu` albo `Escape` | `MainMenu` | brak |
| każdy inny | cokolwiek innego | bez zmian | brak |

`Quitting` jest stanem końcowym: nic z niego nie wychodzi. Przejść do niego można tylko z menu głównego.

### 2.2 Pytania zamiast `if`

Poza `nextMode` moduł ma funkcje, które odpowiadają na pytania o ekran. Reszta gry pyta je, a nie porównuje `GameMode` ze stałymi:

| Pytanie | `MainMenu` | `Playing` | `Paused` | `RoundEnd` | Znaczenie |
|---|---|---|---|---|---|
| `updatesRound` | nie | tak | nie | nie | działają reguły rundy: gracz, bateria, kryształy, czas |
| `animatesScene` | tak | tak | **nie** | tak | idzie `Round::animationSeconds`: kołysanie kryształów i puls świateł |
| `isMenuOpen` | tak | nie | tak | tak | pokazany jest dokument, kursor jest wolny |
| `showsHud` | nie | tak | nie | nie | rysowany jest HUD |
| `showsMinimap` | nie | tak | tak | nie | rysowana jest minimapa |
| `usesMenuCamera` | tak | nie | nie | nie | obraz z kamery menu, a nie z oczu gracza |
| `drawsScene(tryb, tłoNaCałeOkno)` | fałsz tylko z tłem | tak | tak | tak | czy scena jest rysowana |

Stan `Quitting` jest we wszystkich funkcjach traktowany jak "nic": `updatesRound` fałsz, `isMenuOpen` fałsz, `animatesScene` prawda, `showsHud` fałsz.

Uwagi (z komentarzy w kodzie):

- **`animatesScene`:** pauza zatrzymuje wszystko, co rusza się samo, więc stoi też `animationSeconds`. **Wiatr w trawie nie stoi:** shader trawy bierze czas z `glfwGetTime`, nie z `animationSeconds`, więc źdźbła nadal falują w menu pauzy.
- **`showsHud`:** HUD jest tylko w rundzie, bo Dear ImGui rysuje po RmlUi, więc HUD w pauzie leżałby na przyciskach. **`showsMinimap`:** minimapa jest rysowana przed menu, więc w pauzie zostaje pod przyciemnioną warstwą jako część zatrzymanego obrazu.
- **`usesMenuCamera`:** menu główne nie ma rundy do pokazania, więc pożycza kamerę menu (sekcja 2.5).
- **`drawsScene`:** jest **przetestowana, ale renderer jeszcze jej nie woła**: tła na całe okno (wideo) dziś nie ma ([`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md)). Każdy ekran pokazuje scenę, także pod menu.

### 2.3 Zdarzenia: przyciski, klawisz i sama runda

Siedem zdarzeń ma trzy źródła:

- **Przyciski menu:** `Play`, `Resume`, `Restart`, `BackToMenu`, `Quit`. Każdy przycisk niesie w dokumencie nazwę (`data-action`), a funkcja `eventForAction` zamienia nazwę na zdarzenie: `"play"`, `"resume"`, `"restart"`, `"menu"` i `"quit"`. Nieznana nazwa daje fałsz i zostawia zdarzenie, więc gra zapisuje ostrzeżenie w logu.
- **Klawisz Escape:** `Escape` nie jest przyciskiem i nie ma nazwy.
- **Sama runda:** `RoundWon`, gdy krok reguł ustawi `RoundState::Won`.

### 2.4 Escape

Pętla główna wywołuje wirtualną `Application::onEscapePressed()` po odczytaniu zdarzeń okna, **przed krokami stałymi**, więc to, co zmienia, obowiązuje w całej klatce. Domyślna wersja zamyka okno (program, który jej nie nadpisuje, działa jak dawniej), a `NightMazeApp` wysyła `GameEvent::Escape`. Funkcja nie jest wywoływana, gdy klawiatura jest zablokowana (edycja pola tekstowego w panelu). Escape **cofa o jeden ekran** i **nigdy nie zamyka programu**: w menu głównym nie robi nic, a wyjście to przycisk `Quit`.

### 2.5 Przełącznik `--play` i kamera menu

Gra startuje w menu głównym, chyba że:

- podano **`--play`**: start od razu w rundzie. Przełącznik bez wartości (następne słowo jest czytane jako kolejny przełącznik, test sprawdza, że `--play now` jest błędem). Służy testom, skryptom i nagrywaniu;
- podano **`--menu-camera`**: kamera menu jest narzędziem do nagrywania gry, więc też pomija menu główne, żeby żadne menu nie leżało na nagrywanym obrazie;
- **nie wczytał się żaden z trzech dokumentów menu**: gra startuje w rundzie (sekcja 5.5).

Kod w konstruktorze:

```cpp
      m_mode(options.play || options.menuCamera.enabled ? GameMode::Playing : GameMode::MainMenu),
```

(Plik: `src/game/NightMazeApp.cpp`, lista inicjalizacyjna konstruktora.)

**Jak kamera menu ma się do ekranów.** Są dwie różne rzeczy o podobnej nazwie:

- **Flaga `m_menuCamera.enabled`** zostaje tym, czym była: **narzędziem do nagrywania** (F2, panel Camera, `--menu-camera`). Teraz działa **tylko w rundzie**: F2 jest odczytywany tylko gdy `updatesRound(m_mode)`. Włączona flaga chowa HUD, minimapę i panele (robi to `main.cpp`, który czyta flagę).
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

- **Krok 1.** `nextMode(MainMenu, Play)` daje `Playing`, a że `startsRound(MainMenu, Play)` jest prawdą i zdarzenie to `Play`, wołane jest `startNewGame(m_newGame)`, czyli labirynt od nowa z ziarna, nowa runda. `showScreen` chowa dokument i przechwytuje kursor.
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
| `src/game/GameState.hpp`, `.cpp` | `GameMode`, `GameEvent`, `Difficulty`, `NewGame`, `nextMode`, `startsRound`, `eventForAction`, `updatesRound`, `animatesScene`, `isMenuOpen`, `showsHud`, `showsMinimap`, `usesMenuCamera`, `drawsScene`. Część `game_logic`: czysta logika, bez okna i bez dołączania `ui` |
| `src/game/NightMazeApp.hpp`, `.cpp` | składowe `m_mode` i `m_newGame`, `onEscapePressed`, `handleGameEvent`, `handleMenuActions`, `showScreen`, `startNewGame`, `fillRoundEndDocument`, akcesory `gameMode()` i `hudVisible()` |
| `src/core/Application.*` | wirtualna `onEscapePressed()` |
| `src/game/StartOptions.*` | pole `play` i przełącznik `--play`, lista przełączników ma pięć pozycji |
| `src/debug/DebugContext.hpp`, `DebugUI.cpp` | pole `hudVisible` (50 pól) i warunek rysowania HUD |
| `tests/GameStateTests.cpp` | 21 przypadków testowych |

### 5.2 Typy

`GameMode` ma pięć wartości (`MainMenu = 0`, `Playing`, `Paused`, `RoundEnd`, `Quitting`), a `GameEvent` siedem (`Play = 0`, `Resume`, `Restart`, `BackToMenu`, `Quit`, `Escape`, `RoundWon`). `Difficulty` ma trzy (`Easy = 0`, `Normal`, `Hard`), a `NewGame` niesie `difficulty` (domyślnie `Normal`) i `seed` (domyślnie `DEFAULT_MAZE_SEED`):

```cpp
struct NewGame {
    Difficulty difficulty = Difficulty::Normal;

    /// The seed of the maze (MazeSettings::seed).
    std::uint32_t seed = DEFAULT_MAZE_SEED;
};
```

(Plik: `src/game/GameState.hpp`.) **Poziom trudności nie jest jeszcze czytany**: `startNewGame` ma w komentarzu, że trzy poziomy nie mają liczb. `m_newGame.seed` zaczyna od `--seed` i podąża za każdym `regenerateMaze`, więc `Play` buduje znów ten labirynt, który jest w grze, także gdy ziarno zmieniono z panelu Maze.

### 5.3 `nextMode`

```cpp
GameMode nextMode(GameMode mode, GameEvent event) {
    switch (mode) {
    case GameMode::MainMenu:
        if (event == GameEvent::Play) {
            return GameMode::Playing;
        }
        if (event == GameEvent::Quit) {
            return GameMode::Quitting;
        }
        break;
    case GameMode::Playing:
        if (event == GameEvent::Escape) {
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
        break;
    case GameMode::RoundEnd:
        if (event == GameEvent::Restart) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::Quitting:
        break;
    }
    return mode;
}
```

(Plik: `src/game/GameState.cpp`, bez komentarzy.) Jeden blok na ekran, w nim zdarzenia, które tam coś znaczą. Wszystko inne wypada przez `break` do ostatniej linii i **nic nie zmienia**. Dlatego test "jeden ekran, wszystkie zdarzenia" może przejść po wszystkich parach.

`startsRound(mode, event)` odpowiada, czy zdarzenie wysłane na tym ekranie **zaczyna rundę od początku**: `Play` w menu głównym oraz `Restart` w pauzie i na ekranie wyniku. Pytanie zadaje się **przed** zmianą ekranu, bo odpowiedź zależy od ekranu, na którym zdarzenie wysłano.

### 5.4 `handleGameEvent`: co robi aplikacja po zdarzeniu

```cpp
void NightMazeApp::handleGameEvent(GameEvent event) {
    const GameMode before = m_mode;
    const bool newRound = startsRound(before, event);
    m_mode = nextMode(before, event);

    bool roundInsteadOfMenu = false;
    if (!m_menusLoaded && (m_mode == GameMode::MainMenu || m_mode == GameMode::RoundEnd)) {
        m_mode = GameMode::Playing;
        roundInsteadOfMenu = true;
    }

    if (m_mode == before && !roundInsteadOfMenu) {
        return;
    }
    if (event == GameEvent::Play) {
        startNewGame(m_newGame);
    } else if (newRound || roundInsteadOfMenu) {
        beginRound();
    }
    if (m_mode == GameMode::Quitting) {
        window().requestClose();
    }
    showScreen();
}
```

(Plik: `src/game/NightMazeApp.cpp`, bez komentarzy.) Kolejność: (1) zapamiętać ekran i zapytać `startsRound`; (2) policzyć nowy ekran; (3) obsłużyć brak dokumentów; (4) jeśli ekran się nie zmienił, wyjść; (5) `Play` buduje nową grę, a inne zdarzenia zaczynające rundę wołają `beginRound`; (6) `Quitting` zamyka okno; (7) `showScreen` dopasowuje okno do ekranu. **`Play` ma pierwszeństwo przed `beginRound`**, bo `startNewGame` woła `regenerateMaze`, a ono samo zaczyna rundę.

### 5.5 Bez dokumentów menu

Jeśli któryś z trzech dokumentów się nie wczytał, konstruktor zapisuje błąd i ustawia `Playing`. Z ekranów menu da się jeszcze wejść tylko do pauzy (nic nie pokazuje, ale Escape z niej wychodzi). Menu główne i ekran wyniku nie mają klawisza wyjścia, więc zamiast nich zaczyna się nowa runda (zmienna `roundInsteadOfMenu`). Czyli bez menu gra jest ciągłą rundą z pauzą pod Escape. To obsługa błędu, nie decyzja właściciela.

### 5.6 `showScreen`: okno dopasowane do ekranu

```cpp
void NightMazeApp::showScreen() {
    ui::DocumentId document = ui::NO_DOCUMENT;
    if (m_mode == GameMode::MainMenu) {
        document = m_mainMenuDocument;
    } else if (m_mode == GameMode::Paused) {
        document = m_pauseDocument;
    } else if (m_mode == GameMode::RoundEnd) {
        document = m_roundEndDocument;
        fillRoundEndDocument();
    }
    m_ui.show(document);

    input().setCursorCaptured(updatesRound(m_mode) && !m_menuCamera.enabled);
}
```

(Plik: `src/game/NightMazeApp.cpp`, bez komentarzy.) **Kursor idzie za ekranem:** przechwycony w rundzie (do obrotu myszą), wolny na każdym ekranie z menu. Kamera menu nie obraca się myszą, więc też zostawia kursor wolny. Pokazanie panelu klawiszem tyldy oddaje kursor (kod w `main.cpp`), a kliknięcie w scenę przechwytuje go ponownie (`handleInteraction`, bez zmian). **Klik w scenę nie jest już drogą do obrotu myszą:** kursor jest przechwytywany przy starcie rundy.

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

`GameStateTests.cpp` (21 przypadków), wszystkie bez okna: menu główne zaczyna grę przyciskiem `Play` i wychodzi `Quit`; w menu głównym Escape i przyciski innych ekranów nic nie robią; Escape pauzuje i znów wznawia; pauza wznawia, restartuje albo wraca do menu (i `Quit`, `Play` w pauzie nic nie robią); wygrana idzie do wyniku tylko w rundzie; w rundzie przyciski menu nic nie robią; wynik restartuje albo wraca do menu, także Escape; z `Quitting` nic nie wychodzi; tylko menu główne dochodzi do `Quitting`; runda zaczyna się od początku przy `Play` w menu i przy `Restart`; każde zdarzenie, które zaczyna rundę, prowadzi do gry; nazwy przycisków dają zdarzenia; nieznana nazwa nie daje zdarzenia; runda biegnie tylko w `Playing`; scena zatrzymuje się tylko w pauzie; menu jest otwarte na każdym ekranie poza grą; żaden ekran nie uruchamia rundy pod otwartym menu; HUD należy do rundy, a minimapa także do pauzy; tylko menu główne pokazuje obraz z kamery menu; scena jest pomijana tylko za menu głównym z tłem na całe okno; nowa gra prosi o normalny poziom i domyślne ziarno. Dołożony przypadek w `StartOptionsTests.cpp`: `--play` ustawia pole `play` i nic więcej, może stać w dowolnym miejscu i nie przyjmuje wartości.

**Nie ma testu na:** `NightMazeApp::handleGameEvent`, `showScreen` i kursor (kod z OpenGL i okna), kolejność klatki, ani na to, że `drawsScene` jest użyte (nie jest).

### 5.9 Jak to sprawdzono

Patrz "Uczciwie o tym, co sprawdzono" na początku: bramka (zgłoszona), zrzuty agenta (nie właściciela), lista właściciela otwarta.

## 6. Panel ImGui

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
4. **`Play` buduje labirynt od nowa.** Także dla ziarna, które już gra: zmiany rozmiaru i liczb dźwigni i kartek w `m_mazeSettings` mogły się od tego czasu zmienić (komentarz w kodzie).
5. **`Back to menu` porzuca rundę.** Nie ma zapisu ani wznawiania: następne `Play` zaczyna nową grę.
6. **Poziom trudności nic nie zmienia.** Typ i pole są, liczb nie ma.
7. **`--menu-camera` pomija menu główne**, a F2 nie działa w menu. Nagrywanie z menu głównego wymaga innego startu.
8. **Karta wygranej z `Hud.cpp` jest martwym kodem.** Kod nie został usunięty ani zmieniony w tej części.
9. **Bez dokumentów menu** wygląda to jak gra bez menu (sekcja 5.5), a jedyny ślad to wpis w logu.

## 8. Ćwiczenia

1. **Tabela na kartce.** Dla trzech zdarzeń `Escape`, `Restart` i `Quit` wypisz następny ekran na każdym z czterech ekranów. Odpowiedź: `Escape`: z `MainMenu` bez zmian, z `Playing` do `Paused`, z `Paused` do `Playing`, z `RoundEnd` do `MainMenu`. `Restart`: z `MainMenu` i z `Playing` bez zmian, z `Paused` i z `RoundEnd` do `Playing`. `Quit`: z `MainMenu` do `Quitting`, z pozostałych bez zmian.
2. **Które zdarzenia zaczynają rundę?** Odpowiedź: `Play` w menu głównym, `Restart` w pauzie i na ekranie wyniku (trzy pary), i żadne inne.
3. **Czas wyniku.** Co pokaże `timeText` dla 59,9 s, 60 s i 605 s? Odpowiedź: `0:59`, `1:00`, `10:05` (ułamek jest obcinany, pod 10 sekund dopisywane jest zero).
4. **Nowy ekran.** Dodaj ekran `Settings` dostępny z menu głównego i z pauzy. Które funkcje z sekcji 2.2 trzeba uzupełnić, a które testy dopisać? Odpowiedź: `nextMode` (wiersze), `eventForAction` (nazwa), wszystkie pytania z sekcji 2.2 (co ekran pozwala, w szczególności gdzie ma wracać Escape), a testy po jednym przypadku na każdą nową parę oraz na każde pytanie. Poza tym nowy dokument RML.
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

## 10. Źródła

- Notatki: [`../../decisions/escape-pauses-and-goes-back.md`](../../decisions/escape-pauses-and-goes-back.md), [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md), [`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md), [`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md), [`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md).
- Dokumenty: [`../ui/README.md`](../ui/README.md), [`../../libraries/rmlui.md`](../../libraries/rmlui.md), [`menu-camera.md`](menu-camera.md), [`gameplay.md`](gameplay.md), [`../core/main-loop.md`](../core/main-loop.md), [`../debug-ui.md`](../debug-ui.md).
- Automat skończony (finite state machine): dowolny podręcznik programowania gier, rozdział o stanach gry, albo hasło "finite-state machine" w encyklopedii.
