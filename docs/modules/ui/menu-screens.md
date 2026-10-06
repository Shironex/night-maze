# Moduł ui: cztery ekrany menu (menu główne, pauza, koniec rundy, ustawienia), ich RML i RCSS

Kamień milowy: M9, część 3 (2026-10-06). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z warstwy `ui::UiLayer` ([`README.md`](README.md)), z biblioteki RmlUi ([`../../libraries/rmlui.md`](../../libraries/rmlui.md)), z ekranów i zdarzeń gry ([`../game/game-states.md`](../game/game-states.md)), z poziomów trudności ([`../game/difficulty.md`](../game/difficulty.md)) i z ustawień ([`../game/settings.md`](../game/settings.md)).
Kod: dokumenty [`assets/ui/main_menu.rml`](../../../assets/ui/main_menu.rml), [`pause.rml`](../../../assets/ui/pause.rml), [`round_end.rml`](../../../assets/ui/round_end.rml), [`settings.rml`](../../../assets/ui/settings.rml) i arkusz [`menu.rcss`](../../../assets/ui/menu.rcss); warstwa w [`src/ui/UiLayer.hpp`](../../../src/ui/UiLayer.hpp) i [`UiLayer.cpp`](../../../src/ui/UiLayer.cpp); gra w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`handleMenuActions`, `handleMenuCommand`, `handleControlChanges`, `readSeedField`, `rollSeed`, `showScreen`, `fillMainMenuDocument`, `fillPauseDocument`, `fillRoundEndDocument`, `fillSettingsDocument`), [`src/game/GameState.cpp`](../../../src/game/GameState.cpp) (`eventForAction`), [`src/game/StartOptions.cpp`](../../../src/game/StartOptions.cpp) (`parseSeed`) i [`src/main.cpp`](../../../src/main.cpp) (blokada klawiatury). Testów samych dokumentów i klasy `UiLayer` **nie ma** (potrzebują okna i OpenGL); testowane są reguły gry, które za nimi stoją ([`../game/game-states.md`](../game/game-states.md), [`../game/settings.md`](../game/settings.md)).

**Stan na dziś:** gra ma **cztery dokumenty menu** wspólnie stylowane jednym arkuszem. Wszystkie długości w arkuszu są w `vh` (setna część wysokości okna), więc menu zajmuje ten sam kawałek obrazu w oknie każdego rozmiaru. Przyciski nazywają, co robią (`data-action`), kontrolki nazywają, jakie ustawienie zmieniają (`data-setting`), a gra zamienia nazwy na zdarzenia, polecenia i wartości. Klawiatura (Tab, strzałki, Enter, Spacja) działa w każdym dokumencie bez kodu gry. Ekran wchodzi z animacją (rozjaśnienie i wjazd).

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (tak jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę i autora kodu (2026-10-06), nie powtórzone przy pisaniu tego dokumentu:** bramka na gałęzi menu (przed scaleniem z oknem debug) zgłosiła **557 przypadków testowych i 220100 asercji**, a bramka na gałęzi okna debug **526 i 219214** (każda osobno, każda przed scaleniem). **Dla scalonego drzewa żadna bramka nie zgłosiła liczb**: pełna bramka nie została na nim uruchomiona (przebieg przerwał system z braku pamięci, a właściciel zdecydował, że tego dnia go pomija). Żaden z testów nie dotyka dokumentów RML ani `UiLayer`. **Uzupełnienie z 2026-10-06 (po M9, części 4):** pełna bramka została uruchomiona na scalonym drzewie: `make check` przeszedł na `0f8d3b9` (**564 przypadki testowe i 220119 asercji**, zgłoszone przez bramkę, nie powtarzałem). Zdanie powyżej o braku bramki dla scalonego drzewa opisuje stan sprzed tego przebiegu i zostaje jako historia.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela** (Windows, Release, RTX 4070 Ti SUPER, gałąź menu przed scaleniem, 100 procent skalowania): każdy ekran w oknie 1280 na 720 i w pełnym ekranie 1920 na 1080; najechanie myszą na pozycję menu głównego, pauzy i przycisk ekranu wyniku; pierścienie fokusu; nawigacja strzałkami po menu głównym i po wierszach ustawień; Tab po menu głównym; wpisane złe ziarno z podpowiedzią i dobre ziarno 12345 uruchomione Enterem (i pokazane na ekranie pauzy); każdy poziom uruchomiony z sumą na HUD odpowiednio 10 z 13, 19 z 26 i 32 z 40; ustawienia z menu i z pauzy, suwaki ruszane klawiaturą, pole widzenia zmieniające się za panelem; pełny ekran włączony i wyłączony; krok rozmiaru okna; restart gry z zachowanymi ustawieniami; nowe losowe ziarno po `Back to menu`; automatyczna pauza; pierwsza klatka wejścia ekranu (panel jeszcze niewidoczny); `Regenerate` z okna debug na ekranie wyniku opuszczający ekran; prawdziwy koniec rundy na `Easy`, ziarno 7, osiągnięty **lotem z wyłączonymi kolizjami (klawisz `N`) od kryształu do kryształu, nie chodzeniem** (czas 0:23). Większość klatek zapisał tymczasowy, niezatwierdzony skrót `F9`, który zrzucał bufor ramki (zrzut okna i zrzut ekranu zwracały nieaktualny obraz okna z pełnym ekranem OpenGL). **Nie widziane:** przycisk `Restart` okna debug na ekranie wyniku (ta sama ścieżka kodu co `Regenerate` plus `handleGameEvent(Restart)`), zmiana świateł przy ruchu po labiryncie z więcej niż 16 kryształami, suwak przeciągany myszą, skalowanie ekranu inne niż 100 procent, drugi monitor, build Debug na ekranie (agent), wszystko na macOS, chodzenie po korytarzach i obrót myszą ręką człowieka. **Zdarzenie niewyjaśnione:** w jednym wczesnym przebiegu okno gry zniknęło zaraz po sekwencji Dół, Dół, Dół, Enter w menu głównym (kod wyjścia nie został zapisany); ta sama sekwencja odtworzona dwa razy później otworzyła ustawienia z kodem wyjścia 0. **Przypuszczenie autora** (nie sprawdzone): Enter trafił w `Quit`, który stoi tuż pod `Settings` i o nic nie pyta.
3. **Otwarte i relacja właściciela:** lista ręczna w [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 28.2, i [`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcja "M9, część 3 (ekrany menu, poziomy trudności, ustawienia) na macOS" (w całości otwarta). Właściciel zagrał 2026-10-06 na `Hard` w buildzie Debug i zgłosił, że menu, okno debug i gra działają ("it was great"). To **relacja właściciela**, nie zamknięta lista kontrolna.

Liczby w sekcji 2.10 (przykłady) są **policzone ręcznie z kodu i z arkusza**, nie zmierzone w programie.

## 1. Po co to jest

### 1.1 Do czego służą ekrany

Gra ma cztery ekrany z przyciskami: **menu główne** (start gry, wybór poziomu i ziarna, ustawienia, wyjście), **pauzę** (wznowienie, restart, ustawienia, powrót do menu), **koniec rundy** (wynik i trzy sposoby na ciąg dalszy) i **ustawienia** (czułość myszy, pole widzenia, pełny ekran, rozmiar okna). Dokument [`README.md`](README.md) opisuje warstwę, która je pokazuje, a [`../game/game-states.md`](../game/game-states.md) reguły przejść. Ten dokument opisuje **same ekrany**: z czego są zbudowane i jak ich przyciski i kontrolki docierają do gry.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu: menu w RmlUi ([`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md)), cztery ekrany i angielskie teksty ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md)), Escape cofa o jeden ekran ([`../../decisions/escape-pauses-and-goes-back.md`](../../decisions/escape-pauses-and-goes-back.md)). Właściciel wybrał RmlUi po obejrzeniu makiety projektowej z dwoma wariantami menu. **Nie ma decyzji właściciela o wyglądzie ekranów ani o ich zawartości ponad listą powyżej.**

**Wszystko inne jest wyborem wykonawczym autora kodu:** układ i kolory, długości w `vh`, nazwy akcji, to, że ziarno wpisuje się w pole tekstowe, że `New seed` losuje nowe, że ustawienia mają dwa suwaki, przełącznik i krok rozmiaru, kolejność strzałek po menu głównym, wejście ekranu z rozjaśnieniem i wjazdem. Makiety nie ma w repozytorium i **nie widziałem jej**, więc nie umiem porównać z nią wyglądu (sekcja 2.9).

## 2. Teoria

### 2.1 RML i RCSS, jednostka `vh`

Dokument RML to drzewo elementów jak w HTML, arkusz RCSS to podzbiór CSS z dodatkami RmlUi (podstawy: [`../../libraries/rmlui.md`](../../libraries/rmlui.md), sekcja 5). Trzy rzeczy, które różnią ten arkusz od przykładu z `rmlui.md` z części 2:

- **Wszystkie długości są w `vh`**: jeden `vh` to setna część **wysokości** okna (komentarz na początku arkusza). Kontekst RmlUi ma rozmiar bufora ramki okna, więc `vh` jest liczone od niego. Menu zajmuje ten sam **kawałek obrazu** w oknie 1280 na 720 i w pełnym ekranie 1920 na 1080. Wyjątkiem jest jedno przesunięcie poświaty tytułu w `dp` (`font-effect: glow(0dp 6dp 0dp 0dp #56d6ca48)`).
- Ma to **skutek dla skalowania ekranu systemu**: menu nie rośnie, gdy system skaluje ekran do 150 procent, bo rozmiar bufora ramki już uwzględnia skalę, a `vh` jest jego częścią. Starsze sformułowania o `220dp` i "330 pikselach przy 150 procentach" ([`README.md`](README.md), sekcja 2.4, i `rmlui.md`, sekcja 5) dotyczą arkusza z części 2, którego już nie ma.
- **RmlUi nie ma wbudowanego arkusza stylów**, więc bez reguły `display: block` każdy element jest w linii i nic nie przyjmuje fokusu klawiatury. Arkusz zaczyna się od `body`, `div, h1, h2, p` i od reguły, która daje `button` i `input` `tab-index: auto` i `nav: auto`.

Kolory są kolorami HUD i okna debug: biel księżyca `#e2eaf6`, przygaszony niebieskoszary `#8c9ab6`, **bursztyn** `#ffb854` (najechanie, jak latarka), **kryształ** `#56d6ca` (fokus klawiatury, jak kryształy), granatowe powierzchnie. Kolor `#rrggbbaa` ma w ostatnich dwóch cyfrach przezroczystość.

### 2.2 Cztery ekrany

| Ekran | Plik | Klasa `body` | Co zawiera | Kto wypełnia tekst |
|---|---|---|---|---|
| menu główne | `main_menu.rml` | `left` | tytuł, hasło, `Play`, trzy przyciski poziomu, pole ziarna z `New seed`, linia podpowiedzi, `Settings`, `Quit`, blok informacji (rozmiar, kryształy do znalezienia, bateria), podpowiedź klawiszy | `fillMainMenuDocument`, `rollSeed`, `readSeedField` |
| pauza | `pause.rml` | `left paused` | `Paused`, `Resume`, `Restart maze`, `Settings`, `Back to menu`, notatka "Normal, seed 7. Press Esc to resume." | `fillPauseDocument` |
| koniec rundy | `round_end.rml` | `dim` | karta "You escaped", wiersze Time, Crystals, Difficulty, Seed, trzy przyciski | `fillRoundEndDocument` |
| ustawienia | `settings.rml` | `dim` | panel: czułość myszy, pole widzenia, pełny ekran, rozmiar okna, `Back`, `Reset defaults` | `fillSettingsDocument` |

Dwie klasy `body` w arkuszu decydują o tym, co leży **między sceną a menu**: `body.left` ciemni obraz po lewej i zostawia go czystym po prawej (i trochę u dołu, pod blokiem informacji), `body.left.paused` przyciemnia całość mocniej, żeby zatrzymany obraz czytał się jako "nie biegnie", a `body.dim` przyciemnia cały obraz jednym kolorem pod własnym panelem ekranu:

```css
body.left {
    decorator: linear-gradient(90deg, #050812f0 0%, #050812b8 30%, #05081226 62%, #05081200 78%),
               linear-gradient(0deg, #05081299 0%, #05081200 30%);
}
/* The pause menu lies over a round that stands still: all of it is dimmed a little
   more, so the stopped picture reads as "not running". */
body.left.paused {
    decorator: linear-gradient(90deg, #050812f5 0%, #050812cc 30%, #05081280 62%, #05081266 100%);
}
/* The result and the settings have a panel of their own: the whole picture is dimmed. */
body.dim {
    background-color: #090d1a99;
}
```

(Plik: `assets/ui/menu.rcss`.) `decorator: linear-gradient(...)` maluje tło gradientem (dwa gradienty po przecinku leżą jeden na drugim).

**Menu główne**, cały dokument:

```xml
<rml>
<head>
    <title>Main menu</title>
    <link type="text/rcss" href="menu.rcss"/>
</head>
<body class="left">
<!-- A button names what it does with data-action: the game turns the name into an event
     (game::eventForAction) or into a command of this screen (NightMazeApp). The elements
     with an id are written by the game: the chosen difficulty, the seed and the numbers
     of the info block. -->
<div class="column">
    <div class="rise">
        <div class="gem"></div>
        <h1 class="wordmark">NIGHT MAZE</h1>
        <p class="tagline">The moon sees every corridor. You see one.</p>
    </div>
    <div class="menu">
        <div class="rise d1">
            <button class="item" id="play" data-action="play" autofocus><div class="bar"></div>Play</button>
        </div>
        <div class="rise d2">
            <div class="choices">
                <span class="label">Difficulty</span>
                <button class="chip" id="difficulty-easy" data-action="difficulty-easy">Easy</button>
                <button class="chip" id="difficulty-normal" data-action="difficulty-normal">Normal</button>
                <button class="chip" id="difficulty-hard" data-action="difficulty-hard">Hard</button>
            </div>
            <!-- Enter in the seed field starts the game, like the button Play. -->
            <div class="choices">
                <span class="label">Seed</span>
                <input type="text" id="seed" maxlength="10" data-submit="play"/>
                <button class="chip" id="new-seed" data-action="new-seed">New seed</button>
            </div>
            <!-- Empty until the seed field holds something that is not a seed. -->
            <p class="hint" id="seed-hint"></p>
        </div>
        <div class="rise d3">
            <button class="item" id="open-settings" data-action="settings"><div class="bar"></div>Settings</button>
        </div>
        <div class="rise d4">
            <button class="item" id="quit" data-action="quit"><div class="bar"></div>Quit</button>
        </div>
    </div>
</div>
<div class="info rise d5">
    <div class="fact">
        <p class="fact-name">Maze</p>
        <p class="fact-value" id="info-maze">16 x 16</p>
    </div>
    <div class="fact">
        <p class="fact-name">Crystals to find</p>
        <p class="fact-value" id="info-crystals">19 of 26</p>
    </div>
    <div class="fact">
        <p class="fact-name">Battery</p>
        <p class="fact-value" id="info-battery">2:30</p>
    </div>
</div>
<p class="keys corner">Arrows or Tab: move | Enter: choose</p>
</body>
</rml>
```

(Plik: `assets/ui/main_menu.rml`, w całości.) Elementy z `id` pisze gra: przyciski poziomu (klasa `chosen`), ziarno, linię podpowiedzi i trzy liczby bloku informacji (`info-maze`, `info-crystals`, `info-battery`). Wartości w pliku (`16 x 16`, `19 of 26`, `2:30`) to tylko tekst zastępczy, który gra podmienia przy pokazaniu ekranu. Atrybut `autofocus` na `Play` mówi, który element dostaje fokus klawiatury po pokazaniu dokumentu. `<div class="rise d1">` to jedna "porcja", która wjeżdża z opóźnieniem (sekcja 2.7).

**Pauza**, cały dokument:

```xml
<rml>
<head>
    <title>Pause</title>
    <link type="text/rcss" href="menu.rcss"/>
</head>
<body class="left paused">
<div class="column">
    <div class="rise">
        <p class="overline">Paused</p>
    </div>
    <div class="menu">
        <div class="rise d1">
            <button class="item" data-action="resume" autofocus><div class="bar"></div>Resume</button>
        </div>
        <div class="rise d2">
            <button class="item" data-action="restart"><div class="bar"></div>Restart maze</button>
        </div>
        <div class="rise d3">
            <button class="item" data-action="settings"><div class="bar"></div>Settings</button>
        </div>
        <div class="rise d4">
            <button class="item" data-action="menu"><div class="bar"></div>Back to menu</button>
        </div>
    </div>
    <!-- The seed and the difficulty of the round are written by the game when the
         screen comes up (NightMazeApp::fillPauseDocument). -->
    <div class="rise d5">
        <p class="note"><span id="difficulty">Normal</span>, seed <span id="seed">1</span>. Press <span class="amber">Esc</span> to resume.</p>
    </div>
</div>
</body>
</rml>
```

(Plik: `assets/ui/pause.rml`, w całości.) Pauza nie ma `id` na przyciskach: nie piszą do nich ani gra, ani arkusz. Piszą do dwóch `span` w notatce (`difficulty` i `seed`), a nazwa poziomu to `m_playedDifficultyName` (nazwa poziomu albo `Custom` po przebudowie labiryntu z okna debug).

**Koniec rundy**:

```xml
<rml>
<head>
    <title>Round end</title>
    <link type="text/rcss" href="menu.rcss"/>
</head>
<body class="dim">
<div class="center">
    <div class="card">
        <h2 class="card-title">You escaped</h2>
        <div class="sep"></div>
        <!-- The four values are written by the game when the screen comes up
             (NightMazeApp::fillRoundEndDocument). The seed and the difficulty together
             name the maze: a friend who enters both plays the same one. -->
        <div class="row"><span class="row-name">Time</span><span class="row-value" id="time">0:00</span></div>
        <div class="row"><span class="row-name">Crystals</span><span class="row-value" id="crystals">0 of 0</span></div>
        <div class="row"><span class="row-name">Difficulty</span><span class="row-value" id="difficulty">Normal</span></div>
        <div class="row"><span class="row-name">Seed</span><span class="row-value" id="seed">1</span></div>
        <div class="buttons">
            <button class="btn primary" data-action="restart" autofocus>Play again</button>
            <button class="btn" data-action="new-maze">New maze</button>
            <button class="btn" data-action="menu">Back to menu</button>
        </div>
        <p class="keys">Play again: the same maze | New maze: another seed | Esc: back to menu</p>
    </div>
</div>
</body>
</rml>
```

(Plik: `assets/ui/round_end.rml`, w całości.) Komentarz w pliku podaje powód pokazania ziarna i poziomu: **razem nazywają labirynt**, więc znajomy, który je wpisze, zagra ten sam. `Play again` ma `autofocus`, więc Enter na tym ekranie zaczyna ten sam labirynt od nowa.

**Ustawienia**:

```xml
<rml>
<head>
    <title>Settings</title>
    <link type="text/rcss" href="menu.rcss"/>
</head>
<body class="dim">
<!-- A control names its setting with data-setting: when it changes, the game gets the
     name and the new value (game::applySetting), uses the value at once and writes the
     settings file. The limits of the two sliders are the ones in game/Settings.hpp. The
     elements with an id are written by the game. -->
<div class="panel">
    <h2 class="panel-title">Settings</h2>
    <div class="sep"></div>
    <div class="field">
        <span class="field-name">Mouse sensitivity</span>
        <input type="range" id="mouse_sensitivity" data-setting="mouse_sensitivity" min="1" max="10" step="0.1" value="4" autofocus/>
        <span class="field-value" id="mouse_sensitivity-text">4.0</span>
    </div>
    <div class="field">
        <span class="field-name">Field of view</span>
        <input type="range" id="field_of_view" data-setting="field_of_view" min="45" max="90" step="1" value="60"/>
        <span class="field-value" id="field_of_view-text">60 deg</span>
    </div>
    <div class="field">
        <span class="field-name">Fullscreen</span>
        <button class="toggle" id="fullscreen" data-action="toggle-fullscreen"><div class="knob"></div></button>
        <span class="field-value" id="fullscreen-text">Off</span>
    </div>
    <div class="field" id="window-size-row">
        <span class="field-name">Window size</span>
        <button class="step" data-action="window-size-smaller">-</button>
        <span class="field-value wide" id="window-size">1280 x 720</span>
        <button class="step" data-action="window-size-larger">+</button>
    </div>
    <div class="buttons">
        <button class="btn primary" data-action="back">Back</button>
        <button class="btn" data-action="reset-settings">Reset defaults</button>
        <span class="keys" id="saved"></span>
    </div>
    <p class="keys">Left and right move a slider | Esc: back</p>
</div>
</body>
</rml>
```

(Plik: `assets/ui/settings.rml`, w całości.) Suwaki są elementami `input type="range"` z atrybutami `min`, `max`, `step` i `data-setting`; granice 1 do 10 i 45 do 90 są **powtórzeniem** granic z `game/Settings.hpp` (komentarz w dokumencie to mówi). Przełącznik pełnego ekranu to przycisk (`data-action`), nie pole wyboru: jego stan pokazuje klasa `on`, którą ustawia gra. Element `<span id="saved">` jest **pusty i nieużywany**: kod gry niczego do niego nie pisze.

### 2.3 Od przycisku do gry: `data-action`

Przycisk ma atrybut `data-action="play"`. Kliknięcie **nie wywołuje funkcji gry**: warstwa dopisuje nazwę do listy, a gra raz na klatkę zabiera listę i rozstrzyga, co nazwa znaczy. To ta sama kolejka co w części 2 ([`README.md`](README.md), sekcja 2.3), z jedną zmianą w sposobie użycia: nazwy dzielą się teraz na **zdarzenia** i **polecenia ekranu**.

```cpp
void NightMazeApp::handleMenuActions() {
    for (const std::string& action : m_ui.takeActions()) {
        GameEvent event = GameEvent::Escape;
        if (eventForAction(action, event)) {
            // Play in the main menu starts the seed of the seed field. A field that
            // cannot be read keeps the menu open.
            if (action == PLAY_ACTION && m_mode == GameMode::MainMenu && !readSeedField()) {
                continue;
            }
            handleGameEvent(event);
        } else if (!handleMenuCommand(action)) {
            core::logWarn("A menu button has an unknown action: " + action);
        }
    }
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `handleMenuActions`.) Kolejność: nazwa najpierw jest sprawdzana jako **zdarzenie** (`eventForAction` z `GameState.cpp`); jeśli nią nie jest, jako **polecenie ekranu** (`handleMenuCommand`); jeśli też nie, w logu pojawia się ostrzeżenie. `Play` w menu głównym ma jeszcze **bramkę**: ziarno z pola musi dać się odczytać (sekcja 2.6), inaczej klik jest pochłonięty, a menu zostaje.

**Zdarzenia** (`eventForAction`; osiem nazw, co każda uruchamia w automacie, jest w [`../game/game-states.md`](../game/game-states.md)):

```cpp
bool eventForAction(std::string_view action, GameEvent& event) {
    if (action == PLAY_ACTION) {
        event = GameEvent::Play;
    } else if (action == RESUME_ACTION) {
        event = GameEvent::Resume;
    } else if (action == RESTART_ACTION) {
        event = GameEvent::Restart;
    } else if (action == BACK_TO_MENU_ACTION) {
        event = GameEvent::BackToMenu;
    } else if (action == QUIT_ACTION) {
        event = GameEvent::Quit;
    } else if (action == SETTINGS_ACTION) {
        event = GameEvent::OpenSettings;
    } else if (action == BACK_ACTION) {
        event = GameEvent::CloseSettings;
    } else if (action == NEW_MAZE_ACTION) {
        event = GameEvent::NewMaze;
    } else {
        return false;
    }
    return true;
}
```

(Plik: `src/game/GameState.cpp`.) `play`, `resume`, `restart`, `menu`, `quit`, `settings`, `back`, `new-maze`. **Polecenia ekranu** (`handleMenuCommand`, osiem nazw): `difficulty-easy`, `difficulty-normal`, `difficulty-hard`, `new-seed`, `toggle-fullscreen`, `window-size-smaller`, `window-size-larger`, `reset-settings`.

```cpp
bool NightMazeApp::handleMenuCommand(const std::string& action) {
    // The three difficulty buttons of the main menu: "difficulty-" and the key of
    // a level. The choice is a setting too, so the next start of the game shows it.
    const std::string difficultyPrefix = DIFFICULTY_ID_PREFIX;
    if (action.starts_with(difficultyPrefix)) {
        if (!difficultyFromKey(action.substr(difficultyPrefix.size()), m_newGame.difficulty)) {
            return false;
        }
        m_settings.difficulty = m_newGame.difficulty;
        fillMainMenuDocument();
        return true;
    }
    if (action == NEW_SEED_ACTION) {
        rollSeed();
        return true;
    }

    // The buttons of the settings screen. Each one changes m_settings, uses the new
    // value at once and shows it. The file is written when the screen is left.
    if (action == TOGGLE_FULLSCREEN_ACTION) {
        m_settings.fullscreen = !m_settings.fullscreen;
        applyWindowSettings();
        fillSettingsDocument();
        return true;
    }
    if (action == WINDOW_SIZE_SMALLER_ACTION || action == WINDOW_SIZE_LARGER_ACTION) {
        // While the game is fullscreen the size of the window means nothing: the row
        // is shown dimmed and its buttons do nothing.
        if (!m_settings.fullscreen) {
            const core::Size desktop = window().desktopSize();
            const std::vector<WindowSize> choices = windowSizeChoices(
                {.width = desktop.width, .height = desktop.height}, m_settings.windowSize);
            const int step = action == WINDOW_SIZE_LARGER_ACTION ? 1 : -1;
            m_settings.windowSize = steppedWindowSize(choices, m_settings.windowSize, step);
            applyWindowSettings();
            fillSettingsDocument();
        }
        return true;
    }
    if (action == RESET_SETTINGS_ACTION) {
        // Everything this screen shows goes back to its default. The difficulty is
        // chosen in the main menu and stays.
        const Difficulty difficulty = m_settings.difficulty;
        m_settings = GameSettings{};
        m_settings.difficulty = difficulty;
        applyViewSettings();
        applyWindowSettings();
        fillSettingsDocument();
        return true;
    }
    return false;
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `handleMenuCommand`.) Co z tego wynika: przyciski poziomu **nie wchodzą do automatu ekranów** (poziom to dane, nie ekran): zapisują wybór w `m_newGame` i w `m_settings` i odświeżają dokument menu (klasa `chosen`, blok informacji). `New seed` losuje nowe ziarno. Trzy przyciski ustawień i `Reset defaults` zmieniają `m_settings`, stosują zmianę od razu (`applyWindowSettings`, `applyViewSettings`) i odświeżają ekran; **plik jest zapisywany przy wyjściu z ekranu**, nie przy każdym kliknięciu ([`../game/settings.md`](../game/settings.md)). Polecenie, którego nikt nie zna, zwraca fałsz, a `handleMenuActions` zapisuje ostrzeżenie.

Po stronie warstwy kliknięcie obsługuje jeden nasłuchiwacz na kontekście, a w razie potrzeby także drugi, na zdarzenie `change`:

```cpp
// The listener of every click in the context. RmlUi calls ProcessEvent with the element
// that was clicked. That element is often not the button itself but the text inside
// of it, so the search goes up through the parents until one of them carries the
// action attribute.
class ActionListener final : public Rml::EventListener {
public:
    explicit ActionListener(std::vector<std::string>& actions) : m_actions(actions) {}

    void ProcessEvent(Rml::Event& event) override {
        for (Rml::Element* element = event.GetTargetElement(); element != nullptr;
             element = element->GetParentNode()) {
            if (element->HasAttribute(ACTION_ATTRIBUTE)) {
                m_actions.push_back(element->GetAttribute<Rml::String>(ACTION_ATTRIBUTE, ""));
                return;
            }
        }
    }

private:
    // The list of the layer the names are written to.
    std::vector<std::string>& m_actions;
};

// The listener of every "change" event in the context: a slider that was moved, a text
// field that was typed into. Only a control with the setting attribute is reported.
class ChangeListener final : public Rml::EventListener {
public:
    ChangeListener(std::vector<ControlChange>& changes, std::vector<std::string>& actions)
        : m_changes(changes), m_actions(actions) {}

    void ProcessEvent(Rml::Event& event) override {
        Rml::Element* element = event.GetTargetElement();
        if (element == nullptr) {
            return;
        }
        // Enter in a text field: the action the field names, as if its button was
        // clicked. The text itself did not change.
        if (event.GetParameter<bool>(LINEBREAK_PARAMETER, false)) {
            if (element->HasAttribute(SUBMIT_ATTRIBUTE)) {
                m_actions.push_back(element->GetAttribute<Rml::String>(SUBMIT_ATTRIBUTE, ""));
            }
            return;
        }
        if (element->HasAttribute(SETTING_ATTRIBUTE)) {
            m_changes.push_back({.name = element->GetAttribute<Rml::String>(SETTING_ATTRIBUTE, ""),
                                 .value = event.GetParameter<Rml::String>(VALUE_PARAMETER, "")});
        }
    }

private:
    // The two lists of the layer that are written to.
    std::vector<ControlChange>& m_changes;
    std::vector<std::string>& m_actions;
};
```

(Plik: `src/ui/UiLayer.cpp`, dwie klasy w anonimowej przestrzeni nazw.) `ActionListener` idzie od klikniętego elementu **w górę po rodzicach** (kliknięty bywa tekst wewnątrz przycisku) i bierze pierwszy `data-action`. `ChangeListener` obsługuje zmiany kontrolek (sekcja 2.4) i klawisz Enter w polu tekstowym (sekcja 2.6).

### 2.4 Zmienione kontrolki: `data-setting` i `takeChanges`

Suwak nie jest przyciskiem: nie klika się go, tylko przesuwa. RmlUi wysyła wtedy zdarzenie `change` z parametrem `value` (tekst), a `ChangeListener` zapisuje **parę** (nazwa z `data-setting`, nowa wartość) na drugiej liście, którą gra zabiera `UiLayer::takeChanges()` na początku `onRender`, zaraz po `handleMenuActions`. Gra zamienia parę na zmianę ustawienia tą samą funkcją, która czyta plik (`applySetting`), więc **granice i zaokrąglenia są w jednym miejscu** (kod: [`../game/settings.md`](../game/settings.md), sekcja 2.7, funkcja `handleControlChanges`). Przeciągany suwak raportuje **wiele** wartości (każde przesunięcie), a gra bierze je kolejno.

Dwa ważne szczegóły z kodu:

- **Echo.** Gra sama ustawia wartość suwaka (`UiLayer::setValue` w `fillSettingsDocument`), a suwak raportuje ją jak zmianę gracza. `handleControlChanges` porównuje nowe ustawienia ze starymi i gdy się nie różnią, nic nie robi. Zmiana, która przychodzi na innym ekranie niż ustawienia, jest odrzucana jako spóźnione echo.
- **Liczba obok suwaka** (`field_of_view-text`) jest osobnym elementem, który gra odświeża po każdej zmianie.

Nowe funkcje warstwy w tej części: `setValue`, `value` (kontrolki formularza), `setClass`, `takeChanges` (struktura `ControlChange {name, value}`) i drugi nasłuchiwacz. `UiLayer::wantsKeyboard` jest prawdą **tylko dla pola tekstowego** (`input` bez `type` albo z `type="text"`, i `textarea`), a nie dla suwaka:

```cpp
bool UiLayer::wantsKeyboard() const {
    if (!isValid() || m_shown == NO_DOCUMENT) {
        return false;
    }
    return isTextField(m_context->GetFocusElement());
}
```

(Plik: `src/ui/UiLayer.cpp`.)

### 2.5 Fokus i nawigacja klawiaturą

Wszystko poniżej to **kod RmlUi**, który włączają reguły arkusza; kod gry nie robi nic. Źródłem są komentarz w arkuszu i notatka autora (`ElementDocument::ProcessDefaultAction`); nie czytałem kodu RmlUi.

- **`tab-index: auto`** (arkusz: `button, input { tab-index: auto; nav: auto; cursor: pointer; }`) pozwala Tab i Shift+Tab zatrzymywać się na elemencie, a Enterowi i Spacji **kliknąć** element z fokusem: kliknięcie z klawiatury raportuje `data-action` jak kliknięcie myszą.
- **`nav: auto`** pozwala strzałkom przenieść fokus do **najbliższego sąsiada** w danym kierunku.
- **`autofocus`** wskazuje element, który dostaje fokus po pokazaniu dokumentu (`Show(ModalFlag::None, FocusFlag::Auto)` w `UiLayer::show`): `Play` w menu głównym, `Resume` w pauzie, `Play again` na ekranie wyniku, suwak czułości w ustawieniach.
- **Pierścień fokusu** pokazuje `:focus-visible`: kryształowa ramka dla pozycji menu i przycisków, pierścień wokół uchwytu suwaka.

`nav: auto` szuka najbliższego elementu **prosto pod** bieżącym. Wąskie przyciski, które nie nakładają się poziomo, są pomijane, a w menu głównym małe kontrolki dwóch wierszy nie stoją pod słowem `Play`. Dlatego menu główne ma **jawną mapę strzałek**:

```css
/* The way of the arrow keys through the main menu, named element by element. "auto"
   would look for the nearest element straight below, and the small controls of the two
   rows do not stand below the word "Play": the arrows would skip them. */
#play { nav-down: #difficulty-normal; }
#difficulty-easy, #difficulty-normal, #difficulty-hard { nav-up: #play; nav-down: #seed; }
#seed { nav-up: #difficulty-normal; nav-down: #open-settings; }
#new-seed { nav-up: #difficulty-hard; nav-down: #open-settings; }
#open-settings { nav-up: #seed; nav-down: #quit; }
#quit { nav-up: #open-settings; }
```

(Plik: `assets/ui/menu.rcss`.) Co z tego wynika dla klawisza **w dół**: z `Play` do `Normal`, z dowolnego przycisku poziomu do pola ziarna, z pola ziarna i z `New seed` do `Settings`, z `Settings` do `Quit`. Klawisz **w górę** (z mapy): z `Quit` do `Settings`, z `Settings` do pola ziarna, z pola ziarna do `Normal`, z `New seed` do `Hard`, z przycisków poziomu do `Play`. Strzałki w lewo i w prawo działają jak `nav: auto`, czyli idą do sąsiada w wierszu. Wyjątki ma pole tekstowe i suwak: **lewo i prawo przesuwają w nich kursor tekstowy albo uchwyt**, więc mają `nav-left: none; nav-right: none`, a opuszcza się je tylko strzałkami w górę i w dół. Pozostałe trzy ekrany używają samego `nav: auto`.

**Escape** nie idzie przez RmlUi: pętla główna woła `onEscapePressed` i gra zamienia go na `GameEvent::Escape`. Jedyny wyjątek to pole tekstowe:

```cpp
void UiLayer::onKey(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    UiLayer* layer = layerOf(window);
    if (!layer->takesKeyboard()) {
        return;
    }
    // Escape in a text field ends the typing: the focus leaves the field and goes to
    // the document. The game does not see this press (its keyboard is blocked while
    // a field has the focus), so the first Escape leaves the field and only the next
    // one goes a screen back.
    Rml::Element* focused = layer->m_context->GetFocusElement();
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && isTextField(focused)) {
        focused->Blur();
        return;
    }
    RmlGLFW::ProcessKeyCallback(layer->m_context, key, action, mods);
}
```

(Plik: `src/ui/UiLayer.cpp`, funkcja `onKey`.) Escape w polu tekstowym **tylko odbiera fokus** (`Blur`) i nie dociera do gry, więc pierwszy Escape wychodzi z pola, a dopiero następny cofa o ekran.

**Blokada klawiatury gry.** Gdy fokus ma pole tekstowe, pisanie należy do pola, a nie do gry (cyfry nie mogą poruszać gracza, `~` nie może pokazywać panelu). Robi to jedna linia w `main.cpp`:

```cpp
        // ImGui now knows whether it is using the keyboard (a text field is being edited
        // or a widget is active) and the mouse (the cursor is over a panel or a widget is
        // being dragged). Block each device for the game from the next frame on, so typing
        // does not trigger Escape, the panel toggle or player movement, and working with
        // a panel does not click or look around in the scene.
        //
        // A menu blocks nothing here. The game itself reads no key and no mouse of the
        // round while a menu is open (game::updatesRound), and that takes effect in
        // the same frame. The one exception is a text field of a menu: while it has
        // the focus, typing belongs to it, like for a text field of a panel.
        input().setKeyboardBlocked(m_debugUI.wantsKeyboard() || menuUi().wantsKeyboard());
        input().setMouseBlocked(m_debugUI.wantsMouse());
```

(Plik: `src/main.cpp`, koniec `onRender` klasy `DebugNightMazeApp`.) `wantsKeyboard()` menu jest składnikiem alternatywy: blokada działa **od następnej klatki** (informacja pochodzi z końca tej).

### 2.6 Pole ziarna i wpisywanie tekstu

W menu głównym jest pole `<input type="text" id="seed" maxlength="10" data-submit="play"/>`. Cztery rzeczy:

- **`maxlength="10"`**: atrybut ogranicza długość tekstu do 10 znaków (zachowanie RmlUi, nie sprawdzone na ekranie). Największe ziarno, 4294967295, ma 10 cyfr.
- **`data-submit="play"`**: Enter w polu to akcja `play`. W RmlUi Enter w polu tekstowym wysyła to samo zdarzenie `change` z parametrem `linebreak` (komentarz w kodzie), a `ChangeListener` (sekcja 2.3) zamienia je na akcję zapisaną w `data-submit`, jakby kliknięto `Play`. Sam tekst się wtedy nie zmienił.
- **Czytanie ziarna** (`parseSeed`, wspólne z `--seed` wiersza poleceń):

```cpp
// Digits only: no sign, no spaces, and not more than a std::uint32_t holds.
bool parseSeed(std::string_view text, std::uint32_t& seed) {
    if (text.empty()) {
        return false;
    }
    // Digit after digit: the number so far times ten, plus the new digit. It is kept in
    // 64 bits, where one more digit than a seed can have still fits, so a number that
    // is too large is noticed and does not wrap around.
    constexpr std::uint64_t BASE = 10;
    constexpr std::uint64_t LARGEST_SEED = std::numeric_limits<std::uint32_t>::max();
    std::uint64_t value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return false;
        }
        value = value * BASE + static_cast<std::uint64_t>(character - '0');
        if (value > LARGEST_SEED) {
            return false;
        }
    }
    seed = static_cast<std::uint32_t>(value);
    return true;
}
```

(Plik: `src/game/StartOptions.cpp`.)

Ziarno to **same cyfry** od 0 do 4294967295: bez znaku, bez spacji, bez cyfr dziesiętnych po przecinku, bez wartości większej od największej `std::uint32_t` (liczba jest liczona w 64 bitach, więc przekroczenie jest wykrywane przed zawinięciem). Wiodące zera są dozwolone (`007` to 7), pusty tekst nie jest ziarnem (to osobny przypadek niżej).

- **Reakcja gry** (`readSeedField` i `rollSeed`):

```cpp
bool NightMazeApp::readSeedField() {
    const std::string text = m_ui.value(m_mainMenuDocument, SEED_ID);
    if (text.empty()) {
        // Nothing typed: any maze will do.
        rollSeed();
        return true;
    }
    if (!parseSeed(text, m_newGame.seed)) {
        m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, SEED_HINT_TEXT);
        return false;
    }
    m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, "");
    return true;
}

void NightMazeApp::rollSeed() {
    m_newGame.seed = randomSeed();
    m_ui.setValue(m_mainMenuDocument, SEED_ID, std::to_string(m_newGame.seed));
    m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, "");
}
```

(Plik: `src/game/NightMazeApp.cpp`.) **Puste pole to losowe ziarno** (gra losuje je, wpisuje do pola i startuje). **Pole, którego nie da się odczytać, nie startuje gry** i wypisuje w linii `seed-hint` (czerwona, `#ff6b5a`) tekst `Digits only, up to 4294967295`; poprawne ziarno czyści podpowiedź. `rollSeed` losuje liczbę od 1 do 999999 (`std::random_device`) i wpisuje ją do pola. Losuje się **przy starcie gry** i **za każdym razem, gdy menu główne jest wchodzone z gry** (z pauzy albo z wyniku). **Powrót z ustawień zostawia ziarno** (i to, co wpisano w polu). `--seed N` wstawia N do pola dla pierwszej gry (`StartOptions::seedGiven`).

### 2.7 Wejście ekranu: rozjaśnienie i wjazd

Ekran nie pojawia się nagle. `UiLayer::show` robi trzy kroki w tej kolejności:

```cpp
void UiLayer::show(DocumentId document) {
    // A number that names no document shows nothing.
    if (documentOf(document) == nullptr) {
        document = NO_DOCUMENT;
    }
    if (document == m_shown) {
        return;
    }

    if (Rml::ElementDocument* previous = documentOf(m_shown)) {
        // Without the class the document is back in the state its next entrance
        // starts from.
        previous->SetClass(OPEN_CLASS, false);
        previous->Hide();
        // The cursor is no longer over anything of that document: without this
        // a button would still be drawn hovered when the document comes back.
        m_context->ProcessMouseLeave();
    }
    m_shown = document;
    if (Rml::ElementDocument* next = documentOf(m_shown)) {
        // Not modal (the debug UI stays usable), and the keyboard focus goes to the
        // element with the attribute autofocus, so Enter and the arrow keys work at
        // once.
        next->Show(Rml::ModalFlag::None, Rml::FocusFlag::Auto);
        // The entrance: a transition runs between two computed styles. So the style
        // WITHOUT the class is computed once, by this update, and then the class is
        // added. The next update finds the difference and starts the transitions the
        // style sheet names for it.
        m_context->Update();
        next->SetClass(OPEN_CLASS, true);
    }
}
```

(Plik: `src/ui/UiLayer.cpp`, funkcja `show`.) Komentarz w kodzie tłumaczy trik: **przejście (`transition`) biegnie między dwoma obliczonymi stylami**. Dlatego (1) `Show` pokazuje dokument bez klasy `open`, (2) jedno `Update` oblicza styl **bez** klasy (początek animacji), (3) dopiero potem klasa `open` trafia na `body`, a następne `Update` widzi różnicę i uruchamia przejścia z arkusza. Klasa jest zdejmowana przy ukrywaniu dokumentu, więc kolejne wejście zaczyna od początku. W arkuszu to wygląda tak:

```css
body {
    display: block;
    width: 100%;
    height: 100%;
    font-family: "Atkinson Hyperlegible";
    font-size: 2.8vh;
    color: #e2eaf6;
    /* The entrance of a screen. The game adds the class "open" to the body one moment
       after the document became visible (ui::UiLayer::show), and the transition fades
       the screen in between the two states. */
    opacity: 0;
    transition: opacity 0.25s cubic-out;
}
body.open {
    opacity: 1;
}
```

(Plik: `assets/ui/menu.rcss`, `body` i `body.open`.) `body` ma przezroczystość 0, a `body.open` 1; przejście trwa 0,25 s z krzywą `cubic-out`. **Porcje menu wjeżdżają jedna po drugiej z lewej** (klasa `rise` i `d1` do `d6`):

```css
.rise {
    opacity: 0;
    transform: translateX(-3vh);
    transition: opacity transform 0.5s 0.05s cubic-out;
}
.rise.d1 { transition: opacity transform 0.5s 0.11s cubic-out; }
.rise.d2 { transition: opacity transform 0.5s 0.17s cubic-out; }
.rise.d3 { transition: opacity transform 0.5s 0.23s cubic-out; }
.rise.d4 { transition: opacity transform 0.5s 0.29s cubic-out; }
.rise.d5 { transition: opacity transform 0.5s 0.35s cubic-out; }
.rise.d6 { transition: opacity transform 0.5s 0.41s cubic-out; }
body.open .rise {
    opacity: 1;
    transform: translateX(0vh);
}
```

(Plik: `assets/ui/menu.rcss`.) Każda porcja startuje z przezroczystością 0 i przesunięciem o 3 `vh` w lewo (`translateX(-3vh)`); trzeci parametr `transition` to **opóźnienie**: 0,05 s dla `rise`, a potem `0,11`, `0,17`, `0,23`, `0,29` i `0,35` s dla `d1` do `d5` (`d6` ma 0,41 s i żaden dokument go nie używa). Przejście trwa 0,5 s. Karta wyniku i panel ustawień dostają osobny wjazd: unoszą się z `translateY(2.5vh)` do 0 w 0,45 s:

```css
.card, .panel {
    padding: 4.3vh 5vh;
    border: 0.15vh #5c709c73;
    border-radius: 2.1vh;
    decorator: linear-gradient(160deg, #18223ef0 0%, #090d1af5 100%);
    box-shadow: #00000080 0 2.5vh 5vh 1.5vh;
    /* The entrance: the box rises a little while the screen fades in. */
    transform: translateY(2.5vh);
    transition: transform 0.45s cubic-out;
}
body.open .card, body.open .panel {
    transform: translateY(0vh);
}
```

(Plik: `assets/ui/menu.rcss`, `.card` i `.panel`.) Agent widział na zrzucie **pierwszą klatkę wejścia** (panel jeszcze niewidoczny).

### 2.8 Czego RCSS nie umiał

To są trzy ograniczenia, na które trafił autor, plus kilka rzeczy, o których mówi sam arkusz:

1. **Brak rozmycia klatki gry.** `backdrop-filter` rozmyłby tylko to, co RmlUi samo narysowało pod spodem, bo klatka gry leży poza warstwami renderera (wniosek z czytania kodu, nie sprawdzony na ekranie: [`../../libraries/rmlui.md`](../../libraries/rmlui.md), pułapka 2). Tło pod menu to więc **ciemnienie gradientem** (sekcja 2.2), a nie rozmycie.
2. **Brak animacji cienia (`box-shadow`).** Przejście nie potrafi animować `box-shadow`: RmlUi zapisuje ostrzeżenie w logu (według autora). Dlatego poświata fokusu pojawia się od razu, a animowane są kolor, przesunięcie, przezroczystość i szerokość paska. Cień z samym rozmyciem i bez rozciągnięcia (`spread`) jest **prawie niewidoczny**: poświaty mają rozmycie i rozciągnięcie, a pierścienie fokusu tylko rozciągnięcie (pierwszy wiersz `box-shadow` w regule `button.item:focus-visible`: `#56d6ca 0 0 0 0.25vh`, ostry pierścień bez rozmycia, a po przecinku miękka poświata).
3. **Jedna grubość czcionki.** Gra ładuje jeden plik (`fonts/AtkinsonHyperlegible-Regular.ttf`), więc nie ma pogrubienia. Hierarchię robią **rozmiar, kolor i odstępy** (`letter-spacing`, `text-transform`).

Z komentarza w arkuszu: nie ma `::before` i **nie ma obrysu** (`outline`), więc pasek przed pozycją menu jest prawdziwym elementem (`<div class="bar">`), a ramka fokusu jest obramowaniem albo cieniem. Suwak składa RmlUi z elementów `slidertrack`, `sliderprogress`, `sliderbar` i dwóch strzałek (w tym arkuszu o zerowym rozmiarze); **margines `sliderprogress` jest liczony względem toru**, co autor odkrył metodą prób.

### 2.9 Odstępstwa od makiety

**Nie umiem ich wyliczyć.** Makieta z dwoma wariantami menu nie leży w repozytorium i jej nie widziałem, a notatka przekazana przez autora nie zawiera listy różnic. Znane z kodu są trzy ograniczenia z sekcji 2.8: wszystko, co makieta robiła rozmyciem tła, animacją cienia albo grubszą czcionką, w grze jest zrobione inaczej (gradientem, natychmiastowym cieniem, kolorem i rozmiarem). Pełne porównanie z makietą wymaga, żeby ktoś, kto ją ma, położył ją obok zrzutów ekranu.

### 2.10 Przykład policzony ręcznie

**(a) Co znaczy `vh`.** W oknie 1280 na 720 jeden `vh` to 7,2 piksela, w 1920 na 1080 to 10,8 piksela. Z arkusza: tekst bazowy `2,8vh` to 20,2 piksela przy 720 i 30,2 przy 1080; pozycja menu `4,6vh` to 33,1 i 49,7 piksela; tytuł `8,9vh` to 64,1 i 96,1 piksela; kolumna menu zaczyna się `12,4vh` od lewej krawędzi, czyli 89,3 i 133,9 piksela. **Proporcje obrazu są te same.** Skutek uboczny (z arytmetyki, nie sprawdzony na ekranie): `vh` nie zna szerokości, więc okno węższe niż około `92,4vh` (kolumna ma `left: 12.4vh` i `width: 80vh`) przycięłoby kolumnę menu głównego.

**(b) Zły numer ziarna.** Gracz w menu głównym wpisuje w pole `12x4` i naciska Enter.

1. Znaki `1`, `2`, `x`, `4` idą do pola przez `onChar` (pole ma fokus, więc `wantsKeyboard()` jest prawdą i gra ich nie widzi).
2. Enter: RmlUi wysyła `change` z `linebreak`; `ChangeListener` widzi atrybut `data-submit` i dopisuje `"play"` do listy akcji.
3. Początek następnej klatki: `handleMenuActions` bierze `"play"`. `eventForAction` daje `GameEvent::Play`, ale `action == "play"` i tryb to `MainMenu`, więc najpierw `readSeedField`.
4. `parseSeed("12x4")`: po cyfrach `1` i `2` znak `x` nie jest cyfrą, więc zwraca fałsz. `readSeedField` wpisuje `Digits only, up to 4294967295` w element `seed-hint` i zwraca fałsz; pętla robi `continue`. **Gra się nie zaczyna, menu zostaje**, a pole zachowuje to, co wpisano.
5. Gracz poprawia pole na `124` i znów Enter: `parseSeed` zwraca prawdę i ustawia `m_newGame.seed = 124`, podpowiedź jest czyszczona, `handleGameEvent(Play)` startuje grę z `startNewGame`.

**(c) Zmiana pola widzenia z klawiatury.** Ustawienia, fokus na suwaku `field_of_view` (nie ma `autofocus`: ma go suwak czułości, więc gracz najpierw naciska strzałkę w dół). Strzałka w prawo przesuwa suwak o `step="1"`, na przykład z 60 na 61. RmlUi wysyła `change` z wartością `"61"`, `ChangeListener` zapisuje `{field_of_view, "61"}`, `handleControlChanges` czyta tekst `applySetting`, dostaje 61 (w granicach 45 do 90), ustawia `m_settings`, woła `applyViewSettings` (kamera ma od razu `fovDegrees = 61`) i zmienia tekst `61 deg` obok suwaka. Agent widział, że pole widzenia zmienia się za panelem na żywo. Plik zapisuje się dopiero po wyjściu z ekranu przez `Back` albo Escape.

## 3. Jak to działa w OpenGL

Dokumenty rysuje renderer GL3 z backendów RmlUi, w jednym przejściu na końcu klatki ([`README.md`](README.md), sekcja 3). Ta część nie dodała nowego kodu OpenGL: gradienty, cienie, zaokrąglenia i przezroczystość są rysowane przez shadery RmlUi. Rozjaśnienie ekranu to zmiana przezroczystości `body`, czyli mieszanie z klatką gry pod spodem (renderer nakłada wynik z alfą wstępnie przemnożoną).

## 4. Shadery

Brak własnych (liczba programów shaderów gry, czternaście, się nie zmieniła). Cienie, gradienty i zaokrąglenia dokumentów liczą shadery renderera RmlUi.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| `assets/ui/menu.rcss` | jeden arkusz dla czterech dokumentów, wszystkie długości w `vh` (przepisany w tej części) |
| `assets/ui/main_menu.rml`, `pause.rml`, `round_end.rml` | przepisane w tej części |
| `assets/ui/settings.rml` | nowy |
| `src/ui/UiLayer.*` | `show` z wejściem ekranu, `setValue`, `value`, `setClass`, `takeChanges`, drugi nasłuchiwacz, `wantsKeyboard` tylko dla pola tekstowego, Escape w polu |
| `src/game/NightMazeApp.*` | `handleMenuActions`, `handleMenuCommand`, `handleControlChanges`, `readSeedField`, `rollSeed`, `showScreen`, cztery funkcje `fill*Document` |
| `src/game/GameState.*` | `eventForAction`, `startsNewGame` |
| `src/game/StartOptions.*` | `parseSeed` jest publiczne, `seedGiven` |

### 5.2 Co wypełnia gra

```cpp
void NightMazeApp::fillMainMenuDocument() {
    // The chosen one of the three difficulty buttons carries a class.
    for (const Difficulty difficulty : ALL_DIFFICULTIES) {
        m_ui.setClass(m_mainMenuDocument,
                      std::string(DIFFICULTY_ID_PREFIX) + difficultyLevel(difficulty).key,
                      CHOSEN_CLASS, difficulty == m_newGame.difficulty);
    }

    // The info block: what the chosen level means in numbers.
    const DifficultyLevel& level = difficultyLevel(m_newGame.difficulty);
    m_ui.setText(m_mainMenuDocument, INFO_MAZE_ID,
                 std::to_string(level.mazeWidth) + " x " + std::to_string(level.mazeHeight));
    m_ui.setText(m_mainMenuDocument, INFO_CRYSTALS_ID,
                 std::to_string(requiredCrystalCount(level.crystalCount, level.requiredFraction)) +
                     " of " + std::to_string(level.crystalCount));
    m_ui.setText(m_mainMenuDocument, INFO_BATTERY_ID, timeText(level.batteryLifetimeSeconds));
}

void NightMazeApp::fillPauseDocument() {
    m_ui.setText(m_pauseDocument, DIFFICULTY_ID, m_playedDifficultyName);
    m_ui.setText(m_pauseDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
}

void NightMazeApp::fillRoundEndDocument() {
    m_ui.setText(m_roundEndDocument, TIME_ID, timeText(m_round.elapsedSeconds));
    m_ui.setText(m_roundEndDocument, CRYSTALS_ID,
                 std::to_string(m_round.collectedCount) + " of " +
                     std::to_string(m_round.crystals.size()));
    // The difficulty and the seed together name the maze: with both, a friend plays
    // the same one.
    m_ui.setText(m_roundEndDocument, DIFFICULTY_ID, m_playedDifficultyName);
    m_ui.setText(m_roundEndDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
}
```

(Plik: `src/game/NightMazeApp.cpp`, `fillMainMenuDocument`, `fillPauseDocument` i `fillRoundEndDocument`.) Menu główne: klasa `chosen` na przycisku wybranego poziomu i blok informacji z wiersza tabeli (rozmiar `16 x 16`, kryształy `19 of 26` z `requiredCrystalCount`, bateria `2:30` z `timeText`). Pauza i wynik: nazwa poziomu i ziarno z `m_mazeWorld.seed`; wynik jeszcze czas (`timeText`) i liczba zebranych kryształów. `fillSettingsDocument` jest w [`../game/settings.md`](../game/settings.md).

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

(Plik: `src/game/NightMazeApp.cpp`, funkcja `showScreen`.) `showScreen` wybiera dokument ekranu, **wypełnia go bieżącymi liczbami** i pokazuje; `show(NO_DOCUMENT)` w rundzie chowa wszystko. Kursor idzie za ekranem: przechwycony tylko w rundzie.

### 5.3 Wybrane reguły arkusza

Pozycja menu (plain tekst bez ramki, pasek, najechanie, fokus):

```css
button.item {
    display: inline-block;
    position: relative;
    padding: 0.8vh 1.6vh 0.8vh 2.8vh;
    margin-bottom: 0.2vh;
    font-size: 4.6vh;
    line-height: 1.15;
    text-align: left;
    color: #e2eaf6d1;
    border-radius: 0.9vh;
    transition: color transform background-color 0.22s cubic-out;
}
button.item.small {
    font-size: 3.7vh;
}
/* The marker bar in front of an item: it grows out of nothing on hover and on focus. */
button.item .bar {
    position: absolute;
    left: 0;
    top: 50%;
    width: 0vh;
    height: 0.5vh;
    margin-top: -0.25vh;
    background-color: #ffb854;
    transition: width 0.25s cubic-out;
}
button.item:hover {
    color: #ffb854;
    transform: translateX(1.4vh);
}
button.item:hover .bar, button.item:focus-visible .bar {
    width: 2vh;
}
button.item:focus-visible {
    color: #e2eaf6;
    /* A sharp ring and a soft glow around it. */
    box-shadow: #56d6ca 0 0 0 0.25vh, #56d6ca47 0 0 2.2vh 0.7vh;
}
button.item:focus-visible:hover {
    color: #ffb854;
}
button.item:active {
    background-color: #4e3616;
}
```

(Plik: `assets/ui/menu.rcss`.) **Najechanie** robi tekst bursztynowym i przesuwa pozycję o 1,4 `vh` w prawo (`transform: translateX(1.4vh)`), a pasek przed nią rośnie z 0 do 2 `vh` (przejście `width`). **Fokus klawiatury** (`:focus-visible`) daje kryształowy pierścień z miękką poświatą (dwa cienie po przecinku). Kolor `:hover` w stanie `:focus-visible:hover` jest znów bursztynowy, żeby najechanie było widać także na elemencie z fokusem.

Pole tekstowe (`input.text`; reguły `input.text` i `input.range` dotyczą pól `type="text"` i `type="range"`: tak są użyte w arkuszu, a mechanizmu RmlUi, który to rozstrzyga, nie sprawdzałem):

```css
input.text {
    display: block;
    width: 21vh;
    height: 3.5vh;
    margin-right: 1.4vh;
    padding: 0.9vh 1.4vh;
    font-size: 2.7vh;
    line-height: 3.5vh;
    color: #e2eaf6;
    caret-color: #ffb854;
    background-color: #242f4a;
    border: 0.25vh #242f4a;
    border-radius: 0.9vh;
    cursor: text;
    nav-left: none;
    nav-right: none;
    transition: background-color border-color 0.2s cubic-out;
}
input.text:hover {
    background-color: #4e3616;
    border-color: #4e3616;
}
input.text:focus {
    border-color: #56d6ca;
}
/* The marked part of the text. */
input.text selection {
    background-color: #2c6870;
}
```

(Plik: `assets/ui/menu.rcss`.) `nav-left: none; nav-right: none` zostawia strzałki w lewo i w prawo kursorowi tekstowemu, `caret-color` robi bursztynowy kursor, a `selection` stylizuje zaznaczony tekst.

Suwak i przełącznik:

```css
input.range {
    display: block;
    width: 40vh;
    height: 3.6vh;
    nav-left: none;
    nav-right: none;
}
input.range slidertrack {
    height: 1.6vh;
    margin-top: 1vh;
    background-color: #242f4a;
    border-radius: 0.8vh;
}
input.range sliderprogress {
    height: 1.6vh;
    margin-top: 0vh;
    background-color: #ac6e1c;
    border-radius: 0.8vh;
}
input.range sliderbar {
    width: 3.6vh;
    height: 3.6vh;
    background-color: #ffb854;
    border-radius: 1.8vh;
    box-shadow: #ffb85466 0 0 1.2vh 0.4vh;
    transition: background-color 0.2s cubic-out;
}
input.range:hover slidertrack {
    background-color: #4e3616;
}
input.range sliderbar:active {
    background-color: #ffffff;
}
/* The slider with the keyboard focus: a crystal ring around its bar, and a brighter
   track. */
input.range:focus-visible sliderbar {
    box-shadow: #56d6ca 0 0 0 0.45vh, #56d6ca59 0 0 1.6vh 1vh;
}
input.range:focus-visible slidertrack {
    background-color: #303e5e;
}
/* The two arrow buttons at the ends of a slider are not used. */
input.range sliderarrowdec, input.range sliderarrowinc {
    width: 0;
    height: 0;
}

/* A switch: a track with a knob that slides to the right when the switch is on. The
   game sets the class "on". */
button.toggle {
    display: block;
    position: relative;
    width: 7.5vh;
    height: 3.9vh;
    background-color: #242f4a;
    border: 0.25vh #242f4a;
    border-radius: 2.2vh;
    transition: background-color border-color 0.25s cubic-out;
}
button.toggle .knob {
    position: absolute;
    left: 0.5vh;
    top: 0.5vh;
    width: 2.9vh;
    height: 2.9vh;
    background-color: #8c9ab6;
    border-radius: 1.45vh;
    transition: left background-color 0.25s cubic-out;
}
button.toggle:hover {
    background-color: #4e3616;
    border-color: #4e3616;
}
button.toggle.on {
    background-color: #12444c;
    border-color: #12444c;
}
button.toggle.on .knob {
    left: 4.1vh;
    background-color: #ffb854;
    box-shadow: #ffb85466 0 0 1vh 0.3vh;
}
```

(Plik: `assets/ui/menu.rcss`.) Suwak składają trzy elementy RmlUi (`slidertrack`, `sliderprogress`, `sliderbar`); dwie strzałki na końcach mają wymiary 0 i **nie są używane**. Przełącznik to `button.toggle` z gałką, która przesuwa się w prawo (`left` z 0,5 do 4,1 `vh`), gdy gra ustawi klasę `on`.

## 6. Okno debugowania (dawniej panel ImGui)

Ekrany nie mają własnej kontrolki w oknie debug. Panele debug leżą **na wierzchu** menu i mają pierwszeństwo myszy pod jednym kursorem ([`README.md`](README.md), sekcja 2.6); `Regenerate` w oknie debug **na ekranie wyniku opuszcza ekran** (stary wynik należał do rundy, której już nie ma), a **debug `Restart` na tym ekranie nie był oglądany na ekranie** (agent widział `Regenerate`). Pole `crystalCount` nie ma w oknie debug kontrolki ([`../game/difficulty.md`](../game/difficulty.md)).

## 7. Pułapki

1. **`Quit` stoi tuż pod `Settings` i o nic nie pyta.** Strzałka w dół z `Settings` i Enter zamykają grę. Niewyjaśnione zniknięcie okna z jednego z wczesnych przebiegów mogło mieć ten powód (przypuszczenie autora).
2. **`nav: auto` pomija wąskie przyciski.** Stąd jawna mapa strzałek w menu głównym. Nowy przycisk dodany do menu głównego trzeba dopisać do mapy.
3. **Pole tekstowe blokuje klawisze gry**, a Escape tylko je opuszcza.
4. **Pole ziarna przyjmuje tylko cyfry** i pokazuje podpowiedź przy próbie startu z czymś innym. Spacja przed liczbą (` 7`) jest błędem.
5. **Zmiana ustawień ze zmianą okna.** `Reset defaults` nie rusza poziomu trudności.
6. **Brak rozmycia** (sekcja 2.8): tło za menu to gradient.
7. **Granice suwaków są w dwóch miejscach** (atrybuty w `settings.rml` i stałe w `Settings.hpp`). Zmiana jednej bez drugiej rozjedzie je (gra i tak przycina wartość przez `applySetting`).
8. **`<span id="saved">`** w ustawieniach jest pusty i nieużywany.
9. **Menu główne na `Normal` i `Hard` ma w tle przelot, a światła kryształów wybiera się spośród 16 najbliższych oka przelotu** ([`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md)): w większym labiryncie spora część tła może nie mieć światła kryształu. **Nikt tego nie oglądał na ekranie.** **Od M9, części 4 domyślnym tłem menu głównego i ustawień z niego jest nagrane wideo** ([`../video/README.md`](../video/README.md)), więc ta pułapka dotyczy tylko `--menu-background scene` i przypadku, gdy nie da się wczytać ani wideo, ani obrazu nieruchomego.
10. **Skalowanie ekranu.** Menu jest w `vh` i nie rośnie ze skalowaniem systemu. Czy przyciski reagują tam, gdzie je widać przy 125, 150 i 200 procentach, nie było sprawdzone.
11. **Wąskie okno** obcina kolumnę menu głównego (arytmetyka z sekcji 2.10, nie sprawdzone).
12. **Nie widziano suwaka przeciąganego myszą ani bufora Debug na ekranie** (agent). Zdarzenie niewyjaśnione: sekcja o dowodach.

## 8. Ćwiczenia

1. **`vh` na kartce.** Ile pikseli ma przycisk `button.item` (`font-size: 4.6vh`) w oknie 1600 na 900? Odpowiedź: `0,046 * 900 = 41,4` piksela.
2. **Nowy przycisk.** Dodaj do pauzy przycisk `Controls` z `data-action="controls"`. Co się stanie po kliknięciu? Odpowiedź: ani `eventForAction`, ani `handleMenuCommand` nie znają nazwy, więc w logu jest ostrzeżenie "A menu button has an unknown action: controls".
3. **Strzałki.** Jakie przyciski odwiedzi klawisz w dół, zaczynając od `Play`, aż do `Quit`? Odpowiedź: `Play`, `Normal`, pole ziarna, `Settings`, `Quit` (z mapy w sekcji 2.5).
4. **Ziarna.** Które z tych tekstów przyjmie pole: `0`, `007`, `4294967295`, `4294967296`, ` 7`, `1e3`? Odpowiedź: `0`, `007` i `4294967295`; pozostałe nie (przekroczenie, spacja, litera).
5. **Echo suwaka.** Dlaczego `handleControlChanges` porównuje nowe ustawienia ze starymi? Odpowiedź: gra sama ustawia wartość suwaka przy pokazaniu ekranu, a suwak raportuje ją jak zmianę gracza; bez porównania gra stosowałaby ustawienia bez powodu.
6. **Wejście bez `Update`.** Co zobaczysz, gdybyś w `show` usunął wywołanie `m_context->Update()`? Odpowiedź (z komentarza w kodzie, nie sprawdzone): styl bez klasy nie byłby obliczony przed jej dodaniem, więc przejście nie miałoby od czego startować i ekran pojawiłby się od razu w stanie `open`.

## 9. Pytania kontrolne

1. **Dlaczego długości w arkuszu są w `vh`?**
   Żeby menu zajmowało ten sam kawałek obrazu w oknie każdego rozmiaru: `vh` to setna część wysokości okna.
2. **Jak przycisk w dokumencie trafia do gry?**
   Przez atrybut `data-action`: warstwa dopisuje nazwę do listy, a gra zabiera listę raz na klatkę i zamienia nazwy na zdarzenia albo polecenia ekranu.
3. **Czym różni się zdarzenie od polecenia ekranu?**
   Zdarzenie przechodzi przez automat ekranów (`nextMode`). Polecenie (poziom, nowe ziarno, ustawienia okna) zmienia dane i odświeża dokument, ale ekranu nie zmienia.
4. **Jak suwak mówi grze o nowej wartości?**
   Atrybutem `data-setting`, zdarzeniem `change` i listą par z `takeChanges`.
5. **Co robi Enter w polu ziarna?**
   Atrybut `data-submit="play"` zamienia go na akcję `play`; gra czyta ziarno i albo startuje, albo pokazuje podpowiedź.
6. **Dlaczego jest jawna mapa strzałek w menu głównym?**
   `nav: auto` szuka najbliższego elementu prosto pod bieżącym i pomija wąskie przyciski.
7. **Po co `Update` przed dodaniem klasy `open`?**
   Przejście biegnie między dwoma obliczonymi stylami, więc styl bez klasy musi zostać obliczony najpierw.
8. **Czego RCSS nie umiał?**
   Rozmyć klatki gry, animować cienia i pokazać drugiej grubości czcionki.
9. **Co robi Escape w polu tekstowym?**
   Odbiera fokus polu i nie dociera do gry.

## 10. Źródła

- Notatki: [`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md), [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md), [`../../decisions/escape-pauses-and-goes-back.md`](../../decisions/escape-pauses-and-goes-back.md), [`../../decisions/nearest-crystals-carry-the-lights.md`](../../decisions/nearest-crystals-carry-the-lights.md).
- Dokumenty: [`README.md`](README.md), [`../../libraries/rmlui.md`](../../libraries/rmlui.md), [`../game/game-states.md`](../game/game-states.md), [`../game/difficulty.md`](../game/difficulty.md), [`../game/settings.md`](../game/settings.md), [`../core/input.md`](../core/input.md), [`../debug-ui.md`](../debug-ui.md).
- RmlUi, RCSS (`transition`, `decorator`, `box-shadow`, `nav`, `tab-index`) i formularze: <https://mikke89.github.io/RmlUiDoc/>.
