# Moduł game: ustawienia gracza, plik `night-maze-settings.txt`, pełny ekran i pauza po utracie fokusu

Kamień milowy: M9, część 3 (2026-10-06). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z okna ([`../core/window-context.md`](../core/window-context.md): `core::Window`, GLFW), z plików ([`../core/paths.md`](../core/paths.md)), z ekranów ([`game-states.md`](game-states.md)), z poziomów trudności ([`difficulty.md`](difficulty.md)) i z warstwy menu ([`../ui/menu-screens.md`](../ui/menu-screens.md): suwaki i przełącznik ekranu ustawień).
Kod: [`src/game/Settings.hpp`](../../../src/game/Settings.hpp) i [`Settings.cpp`](../../../src/game/Settings.cpp) (dane, odczyt i zapis tekstu pliku, rozmiary okna), [`src/core/Files.hpp`](../../../src/core/Files.hpp) i [`Files.cpp`](../../../src/core/Files.cpp) (`readTextFile`, `writeTextFile`), [`src/core/Window.hpp`](../../../src/core/Window.hpp) i [`Window.cpp`](../../../src/core/Window.cpp) (`setWindowedSize`, `setFullscreen`, `isFullscreen`, `desktopSize`, `isFocused`), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`loadSettings`, `saveSettings`, `applyViewSettings`, `applyWindowSettings`, `handleControlChanges`, `handleMenuCommand`, `fillSettingsDocument`, `onRender`) i dokument [`assets/ui/settings.rml`](../../../assets/ui/settings.rml). Testy: [`tests/SettingsTests.cpp`](../../../tests/SettingsTests.cpp) (17 przypadków).

**Stan na dziś:** gra ma **ekran ustawień** (z menu głównego i z pauzy) z pięcioma rzeczami do zmiany: czułość myszy, pole widzenia, pełny ekran, rozmiar okna, poziom trudności (ten wybiera się w menu głównym). Ustawienia leżą w **pliku tekstowym** `night-maze-settings.txt` w katalogu roboczym procesu, który gra czyta raz przy starcie i zapisuje, gdy się zmieniły. Czysta część (struktura, odczyt i zapis tekstu pliku, listy rozmiarów) jest w bibliotece `game_logic` i ma testy, a plik i okno są w `NightMazeApp` i `core::Window`. Gra **wstrzymuje rundę, gdy okno traci fokus**. **Kod pełnego ekranu, rozmiaru okna i fokusu nie był budowany ani uruchamiany na macOS.**

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (tak jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę i autora kodu (2026-10-06), nie powtórzone przy pisaniu tego dokumentu:** bramka na gałęzi menu (przed scaleniem z oknem debug) zgłosiła **557 przypadków testowych i 220100 asercji**, z czego 17 przypadków to `SettingsTests.cpp`. Dla scalonego drzewa żadna bramka nie zgłosiła liczb: pełna bramka nie została na nim uruchomiona (przebieg przerwał system z braku pamięci, a właściciel zdecydował, że tego dnia go pomija). Każda z dwóch gałęzi przeszła własną bramkę przed scaleniem. **Uzupełnienie z 2026-10-06 (po M9, części 4):** pełna bramka została uruchomiona na scalonym drzewie: `make check` przeszedł na `0f8d3b9` (**564 przypadki testowe i 220119 asercji**, zgłoszone przez bramkę, nie powtarzałem). Zdanie powyżej o braku bramki dla scalonego drzewa opisuje stan sprzed tego przebiegu i zostaje jako historia.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela** (Windows, Release, RTX 4070 Ti SUPER, gałąź menu przed scaleniem): ekran ustawień z menu i z pauzy; suwaki ruszane klawiaturą, a pole widzenia zmienia się za panelem na żywo; pełny ekran włączony i wyłączony z ekranu ustawień (klient 1920 na 1080, potem powrót do tego samego prostokąta okna); krok rozmiaru okna do 1366 na 768; gra uruchomiona ponownie z tym samym plikiem: czułość, pole widzenia, rozmiar okna i poziom nadal zastosowane; start od razu w pełnym ekranie z pliku; automatyczna pauza po oddaniu pierwszego planu pasku zadań. **Nie widziane:** suwak przeciągany myszą, skalowanie ekranu inne niż 100 procent, gra na drugim monitorze, wszystko na macOS.
3. **Otwarta lista właściciela:** [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 28.2, i [`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcja "M9, część 3 (ekrany menu, poziomy trudności, ustawienia) na macOS" (w całości otwarta). Właściciel zgłosił 2026-10-06, że zagrał na `Hard` w buildzie Debug i że menu, okno debug i gra działają ("it was great"): relacja, nie zamknięta lista.

## 1. Po co to jest

### 1.1 Do czego służą ustawienia

Gracz chce ustawić, jak szybko mysz obraca kamerę, jak szerokie jest pole widzenia i czy gra zajmuje cały ekran, i chce, żeby gra to pamiętała. Bez zapisu każde uruchomienie zaczynałoby od wartości domyślnych. Dlatego ustawienia są jedną strukturą (`GameSettings`) i jednym plikiem.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu ([`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md)): w M9 jest ekran ustawień, a poziom trudności ma być wybierany. **Notatka z zakresu menu nie rozstrzygała**, co jest na ekranie ustawień ani czy ustawienia są zapamiętywane między uruchomieniami ("zakres ustawień nie jest ustalony", "zapisu ustawień w grze nie ma").

**Wszystko poniżej jest wyborem wykonawczym autora kodu** i właściciel może to zmienić: lista pięciu ustawień, ich granice i wartości domyślne, że ustawienia są w pliku tekstowym (a nie w rejestrze ani w pliku binarnym), nazwa pliku i jego miejsce, moment zapisu, reguły tolerancji na błędy, pełny ekran jako "pulpit taki, jaki jest" (bez zmiany rozdzielczości), sześć rozmiarów okna, pauza po utracie fokusu.

## 2. Teoria

### 2.1 Pięć ustawień

| Nazwa w pliku | Wartości | Domyślnie | Do czego służy |
|---|---|---|---|
| `mouse_sensitivity` | od 1,0 do 10,0 | 4,0 | stopnie na jednostkę ruchu myszy = wartość razy 0,025 (`mouseDegreesPerUnit`): 4,0 to 0,1 stopnia, jak przed ustawieniami |
| `field_of_view` | od 45 do 90 (pionowe, w stopniach) | 60 | `scene::Camera::fovDegrees` |
| `fullscreen` | `on` albo `off` | `off` | `core::Window::setFullscreen` |
| `window_size` | `SZEROKOŚĆxWYSOKOŚĆ`, od 640x360 do 7680x4320 | `1280x720` | `core::Window::setWindowedSize` |
| `difficulty` | `easy`, `normal`, `hard` | `normal` | poziom, od którego zaczyna menu główne, i rozmiar pierwszego labiryntu ([`difficulty.md`](difficulty.md)) |

Struktura w kodzie (wartości początkowe pól to wartości domyślne gry bez pliku):

```cpp
/// Everything the settings screen edits and the file stores. The values here are the
/// defaults: the game without a settings file.
struct GameSettings {
    /// From MIN_MOUSE_SENSITIVITY to MAX_MOUSE_SENSITIVITY.
    float mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;

    /// From MIN_FIELD_OF_VIEW_DEGREES to MAX_FIELD_OF_VIEW_DEGREES.
    float fieldOfViewDegrees = DEFAULT_FIELD_OF_VIEW_DEGREES;

    /// True: the game covers the whole screen, at the resolution the desktop has.
    bool fullscreen = false;

    /// The size of the window while the game is not fullscreen.
    WindowSize windowSize = DEFAULT_WINDOW_SIZE;

    /// The difficulty the main menu starts with: the one that was chosen last.
    Difficulty difficulty = Difficulty::Normal;

    bool operator==(const GameSettings& other) const = default;
};
```

(Plik: `src/game/Settings.hpp`.) Operator `==` jest domyślny (`= default`): dzięki niemu `NightMazeApp` porównuje bieżące ustawienia z ostatnio zapisanymi jednym zapisem `m_settings == m_savedSettings`.

**Czułość 1 do 10 jest liczbą dla człowieka**, a gra mnoży ją przez stałą `MOUSE_DEGREES_PER_SENSITIVITY` = 0,025. Dzięki temu domyślne 4 daje dokładnie dawną czułość 0,1 stopnia na jednostkę, a suwak ma wygodny zakres od 0,025 do 0,25 stopnia na jednostkę.

### 2.2 Plik: format, miejsce, moment zapisu

Przykład pliku, tak jak zapisuje go `formatSettings` dla ustawień domyślnych (to nie jest fragment kodu, tylko tekst zapisany w teście `the file is plain text a person can read and edit`):

```text
# Night Maze settings. One "name = value" per line, a line that starts with # is a comment.
mouse_sensitivity = 4.0
field_of_view = 60
fullscreen = off
window_size = 1280x720
difficulty = normal
```

**Miejsce.** Plik nazywa się `night-maze-settings.txt` (`SETTINGS_FILE_NAME`) i leży w **katalogu roboczym procesu**. Launcher uruchamia grę w swoim katalogu `data/`, więc plik zostaje tam, gdy sama gra zostanie zastąpiona nową wersją (komentarz w `Settings.hpp`, według autora). Gdy gra jest uruchomiona z repozytorium przez `make run`, plik powstaje w korzeniu repozytorium i `.gitignore` go ignoruje (wpis `night-maze-settings.txt` w sekcji "Runtime files"). Ścieżka jest **względna** i niczego nie pyta system o katalog użytkownika: to jest wybór wykonawczy, który zakłada, że gra ma prawo pisać w swoim katalogu roboczym (jeśli nie ma, zapis kończy się błędem w logu i gra działa dalej na ustawieniach w pamięci).

**Odczyt.** Raz, w konstruktorze `NightMazeApp`, **przed zbudowaniem pierwszego labiryntu** (pierwszy labirynt ma rozmiar poziomu z pliku):

```cpp
// Reads the settings file from the working directory. Without a file (the first start)
// and with a file that cannot be read the game starts with its defaults: parseSettings
// skips everything it does not understand.
GameSettings loadSettings() {
    std::string text;
    if (!core::readTextFile(SETTINGS_FILE_NAME, text)) {
        core::logInfo(std::string("No settings file (") + SETTINGS_FILE_NAME +
                      "): starting with the defaults");
        return {};
    }
    core::logInfo(std::string("Loaded settings: ") + SETTINGS_FILE_NAME);
    return parseSettings(text);
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja w anonimowej przestrzeni nazw.) Brak pliku daje ustawienia domyślne (linia w logu), a plik, który się wczytał, idzie przez `parseSettings`, który pomija to, czego nie rozumie.

**Zapis.** Funkcja `saveSettings` zapisuje plik **tylko wtedy, gdy ustawienia różnią się od tego, co plik już trzyma**:

```cpp
void NightMazeApp::saveSettings() {
    if (m_settings == m_savedSettings) {
        return;
    }
    if (core::writeTextFile(SETTINGS_FILE_NAME, formatSettings(m_settings))) {
        m_savedSettings = m_settings;
        core::logInfo(std::string("Saved settings: ") + SETTINGS_FILE_NAME);
    } else {
        // The game goes on with the settings it has. They are tried again at the next
        // change.
        core::logError(std::string("The settings file cannot be written: ") + SETTINGS_FILE_NAME);
    }
}
```

(Plik: `src/game/NightMazeApp.cpp`.) Wołana jest w trzech miejscach: **przy wyjściu z ekranu ustawień** (`handleGameEvent`, gdy poprzednim ekranem było `SettingsFromMenu` albo `SettingsFromPause`), **przy starcie nowej gry** (`startNewGame`, żeby zapamiętać poziom) i **w destruktorze** (zamknięcie gry). Bez żadnej zmiany plik w ogóle nie powstaje. Zmiana poziomu przyciskiem w menu głównym zmienia `m_settings.difficulty` od razu, ale trafia do pliku dopiero przy jednym z tych trzech zdarzeń. Błąd zapisu nie przerywa gry: ustawienia zostają w pamięci i zapis jest ponawiany przy następnej zmianie (`m_savedSettings` nie jest aktualizowane po porażce).

**Moment zastosowania.** Ustawienia działają **w chwili zmiany**, nie co klatkę: suwak woła `applyViewSettings` (kamera i czułość), przełącznik pełnego ekranu i krok rozmiaru wołają `applyWindowSettings`. Okno debug (kategoria Player: `Mouse sensitivity` w karcie Movement i `FOV`) nadal może zmieniać te dwie liczby wprost; jego zmiany **nie są zapisywane** i znikają przy następnej zmianie z ekranu ustawień. Suwak `Mouse sensitivity` okna debug jest w stopniach na jednostkę ruchu myszy, nie w skali od 1 do 10 (`Degrees the camera turns for one unit of mouse movement.`).

### 2.3 Tolerancja: brakujący, zepsuty plik i nieznane klucze

Reguły `parseSettings` (komentarz w `Settings.hpp` i testy):

- jedna linia `nazwa = wartość`, dowolne spacje i tabulatory dookoła nazwy i wartości;
- puste linie i linie, które zaczynają się od `#`, są pomijane (`#` ma znaczenie tylko na początku linii: `fullscreen = on # tak` ma wartość `on # tak`, której `applySetting` nie przyjmie);
- **linia, której nie da się odczytać, jest pomijana**: nieznana nazwa (ustawienie nowszej wersji gry), niezrozumiała wartość, brak znaku `=`. Ustawienie zostaje wtedy przy wartości domyślnej, a reszta pliku działa;
- liczba poza granicami jest **przycięta do najbliższej granicy**, a nie odrzucona (`mouse_sensitivity = 0.2` daje 1,0);
- **ostatnia linia powtórzonej nazwy wygrywa**, a linia z błędną wartością nie zmienia już ustawionego (test: `field_of_view = 50`, `= 80`, `= oops` daje 80);
- bajty `EF BB BF` (znacznik kolejności bajtów, byte order mark, dodawany przez Notatnik) na początku tekstu i znaki `\r` na końcach linii są ignorowane;
- pusty tekst daje ustawienia domyślne, a tekst, który nie jest tekstem (bajty `00 01 FF FE`, 100000 liter `x`, 5000 dziewiątek), też: bez wyjątku i bez zawieszenia.

Fragment, który czyta tekst linia po linii:

```cpp
GameSettings parseSettings(std::string_view text) {
    GameSettings settings;
    if (text.starts_with(BYTE_ORDER_MARK)) {
        text.remove_prefix(BYTE_ORDER_MARK.size());
    }

    // Line after line: the text up to the next line end, then the rest.
    while (!text.empty()) {
        const std::size_t lineEnd = text.find('\n');
        const std::string_view line = trimmed(text.substr(0, lineEnd));
        // Without a line end this was the last line: nothing is left.
        text = lineEnd == std::string_view::npos ? std::string_view{} : text.substr(lineEnd + 1);

        if (line.empty() || line.front() == COMMENT_SIGN) {
            continue;
        }
        const std::size_t sign = line.find(ASSIGN_SIGN);
        if (sign == std::string_view::npos) {
            continue;
        }
        // A line that cannot be read changes nothing: applySetting returns false.
        applySetting(settings, trimmed(line.substr(0, sign)), trimmed(line.substr(sign + 1)));
    }
    return settings;
}
```

(Plik: `src/game/Settings.cpp`, funkcja `parseSettings`.) Pętla zjada tekst linia po linii: `find('\n')` daje koniec linii, `trimmed` ucina białe znaki, a `text.substr(lineEnd + 1)` jest resztą. Ostatnia linia bez końca linii też jest czytana (`lineEnd == npos` zostawia pusty tekst po sobie). Wynik `applySetting` jest **celowo ignorowany**: porażka znaczy "ta linia nic nie zmienia".

### 2.4 Własny czytnik liczb

Liczby w pliku czyta funkcja pisana ręcznie, a nie `strtof` ani `from_chars`:

```cpp
// Reads a number written with digits and at most one point: "60", "4.5", ".5". No sign
// and no exponent. False for anything else.
//
// Written by hand on purpose. The functions of the C library (strtof) read "4,5" in
// place of "4.5" when the system is set to a language that writes a comma, so a file
// would mean one thing on one computer and another thing on the next.
bool parseNumber(std::string_view text, float& number) {
    if (text.empty() || text.size() > MAX_NUMBER_LENGTH) {
        return false;
    }
    double value = 0.0;
    // The worth of the next digit after the point: a tenth, a hundredth and so on.
    double place = 1.0;
    bool afterPoint = false;
    bool anyDigit = false;
    for (const char character : text) {
        if (character == '.') {
            if (afterPoint) {
                return false;
            }
            afterPoint = true;
            continue;
        }
        if (character < '0' || character > '9') {
            return false;
        }
        anyDigit = true;
        const double digit = character - '0';
        if (afterPoint) {
            place /= DECIMAL_BASE;
            value += digit * place;
        } else {
            value = value * DECIMAL_BASE + digit;
        }
    }
    if (!anyDigit) {
        return false;
    }
    number = static_cast<float>(value);
    return true;
}
```

(Plik: `src/game/Settings.cpp`, funkcja `parseNumber`, razem z komentarzem.) **Powód z komentarza:** funkcje biblioteki C czytają `4,5` zamiast `4.5`, gdy system jest ustawiony na język z przecinkiem dziesiętnym, więc ten sam plik znaczyłby co innego na dwóch komputerach. Własny czytnik przyjmuje **cyfry i najwyżej jedną kropkę**, bez znaku, bez wykładnika, najwyżej 12 znaków. Przykłady z tego kodu: `"60"` daje 60, `"4.5"` daje 4,5 (4, potem 5 razy 0,1), `".5"` daje 0,5, a `"4,5"`, `"-3"`, `"+3"`, `"1e3"`, `"1.2.3"`, `"."` i napis dłuższy niż 12 znaków są odrzucane. Zapis jest symetryczny: `mouseSensitivityLabel` buduje tekst z liczb całkowitych (`4.0`), więc kropka jest kropką w każdym systemie. Czytnik opcji wiersza poleceń `--menu-time` używa dla odmiany `strtof` ([`menu-camera.md`](menu-camera.md), sekcja 2.9).

Zapis:

```cpp
std::string formatSettings(const GameSettings& settings) {
    std::string text = "# Night Maze settings. One \"name = value\" per line, a line that starts "
                       "with # is a comment.\n";
    text +=
        settingLine(MOUSE_SENSITIVITY_SETTING, mouseSensitivityLabel(settings.mouseSensitivity));
    text += settingLine(FIELD_OF_VIEW_SETTING,
                        std::to_string(std::lround(settings.fieldOfViewDegrees)));
    text +=
        settingLine(FULLSCREEN_SETTING, std::string(settings.fullscreen ? ON_VALUE : OFF_VALUE));
    text += settingLine(WINDOW_SIZE_SETTING, windowSizeValue(settings.windowSize));
    text += settingLine(DIFFICULTY_SETTING, difficultyLevel(settings.difficulty).key);
    return text;
}
```

(Plik: `src/game/Settings.cpp`, funkcja `formatSettings`.) Czułość jest zaokrąglona do jednego miejsca po przecinku (`lround(liczba * 10)`), a pole widzenia do całych stopni. Test `what is written is read back the same` sprawdza, że odczyt tego, co zapisano, daje te same ustawienia (czułość 7,3 wraca jako 7,3), a zapis ustawień odczytanych z zapisu daje ten sam tekst.

### 2.5 Okno: rozmiar, pełny ekran i `glfwSetWindowMonitor`

**Rozmiary.** Ekran ustawień oferuje sześć rozmiarów okna (`WINDOW_SIZES`), od najmniejszego do największego:

```cpp
/// The sizes the settings screen offers, from the smallest to the largest, all 16 : 9
/// except the second. The first one is the size the game always started with.
constexpr std::array<WindowSize, 6> WINDOW_SIZES = {{
    {.width = 1280, .height = 720},
    {.width = 1366, .height = 768},
    {.width = 1600, .height = 900},
    {.width = 1920, .height = 1080},
    {.width = 2560, .height = 1440},
    {.width = 3840, .height = 2160},
}};
constexpr WindowSize DEFAULT_WINDOW_SIZE = WINDOW_SIZES[0];
```

(Plik: `src/game/Settings.hpp`.) Plik może podać **dowolny** rozmiar między 640 na 360 a 7680 na 4320 (`parseWindowSize` przycina każdą liczbę osobno do tych granic i zaokrągla), także taki, którego nie ma na liście. Lista kroków dla ekranu to `windowSizeChoices(pulpit, bieżący)`:

```cpp
std::vector<WindowSize> windowSizeChoices(WindowSize desktop, WindowSize current) {
    std::vector<WindowSize> choices;
    for (const WindowSize& size : WINDOW_SIZES) {
        const bool fits = size.width <= desktop.width && size.height <= desktop.height;
        if (fits || size == DEFAULT_WINDOW_SIZE || size == current) {
            choices.push_back(size);
        }
    }
    if (std::ranges::find(choices, current) == choices.end()) {
        // A size of its own: it goes in front of the first larger one. Wider counts as
        // larger, and of two sizes of the same width the higher one.
        const auto larger = std::ranges::find_if(choices, [current](WindowSize size) {
            return size.width > current.width ||
                   (size.width == current.width && size.height > current.height);
        });
        choices.insert(larger, current);
    }
    return choices;
}

WindowSize steppedWindowSize(const std::vector<WindowSize>& choices, WindowSize current, int step) {
    const auto found = std::ranges::find(choices, current);
    if (found == choices.end()) {
        return current;
    }
    // The place in the list as a whole number, moved by the step and kept in the list.
    const auto last = static_cast<int>(choices.size()) - 1;
    const int place = std::clamp(static_cast<int>(found - choices.begin()) + step, 0, last);
    return choices[static_cast<std::size_t>(place)];
}
```

(Plik: `src/game/Settings.cpp`.) Reguły: oferowane są rozmiary, które mieszczą się na pulpicie, **najmniejszy jest oferowany zawsze** (także na pulpicie, na którym się nie mieści), a bieżący rozmiar jest zawsze na liście (rozmiar z ręcznie napisanego pliku wskakuje przed pierwszy większy; szerszy znaczy większy, a przy tej samej szerokości wyższy). `steppedWindowSize` bierze o `krok` miejsc dalej i **zatrzymuje się na końcach** (`std::clamp`); rozmiar spoza listy i pusta lista dają rozmiar bieżący.

Przykład policzony z kodu: pulpit 1920 na 1080, bieżący 1280 na 720. Mieszczą się 1280x720, 1366x768, 1600x900 i 1920x1080 (2560x1440 i 3840x2160 nie). Krok +1 daje 1366x768 (tak agent zobaczył na ekranie). Przy bieżącym 1500 na 800, którego nie ma na liście, lista to 1280x720, 1366x768, **1500x800**, 1600x900, 1920x1080, a krok +1 daje 1600x900.

**Pełny ekran.** Przełącznik na ekranie ustawień wywołuje `applyWindowSettings`:

```cpp
void NightMazeApp::applyWindowSettings() {
    // The size first: while the window is fullscreen it is only remembered, and
    // switching fullscreen off then goes back to a window of that size.
    window().setWindowedSize(
        {.width = m_settings.windowSize.width, .height = m_settings.windowSize.height});
    window().setFullscreen(m_settings.fullscreen);
}
```

(Plik: `src/game/NightMazeApp.cpp`.) Najpierw rozmiar (przy pełnym ekranie jest tylko zapamiętywany, a wyłączenie pełnego ekranu wraca do okna o tym rozmiarze), potem tryb. Sam przełącznik trybu:

```cpp
void Window::setFullscreen(bool fullscreen) {
    if (fullscreen == m_fullscreen) {
        return;
    }

    if (fullscreen) {
        GLFWmonitor* screen = screenOf(m_handle);
        const GLFWvidmode* mode = screen != nullptr ? glfwGetVideoMode(screen) : nullptr;
        if (mode == nullptr) {
            logError("Fullscreen is not possible: no screen was found");
            return;
        }
        // Where the window is now: the way back.
        glfwGetWindowPos(m_handle, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_handle, &m_windowedSize.width, &m_windowedSize.height);
        // A window with a monitor is fullscreen on it. Asking for the size and the
        // refresh rate the screen already has keeps its video mode as it is.
        glfwSetWindowMonitor(m_handle, screen, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // A window without a monitor is an ordinary window again. The last argument is
        // the refresh rate, which only a fullscreen window has.
        glfwSetWindowMonitor(m_handle, nullptr, m_windowedX, m_windowedY, m_windowedSize.width,
                             m_windowedSize.height, GLFW_DONT_CARE);
    }
    m_fullscreen = fullscreen;
    // Some drivers forget the swap interval when the window changes its kind, so the
    // wait for the screen refresh is asked for again.
    glfwSwapInterval(1);
}
```

(Plik: `src/core/Window.cpp`, funkcja `setFullscreen`.) **Pełny ekran to "pulpit taki, jaki jest"**: okno dostaje monitor (`glfwSetWindowMonitor`) i **bieżący tryb wideo tego ekranu** (jego szerokość, wysokość i częstotliwość), więc rozdzielczość pulpitu się nie zmienia i nic innego na pulpicie się nie rusza. Powrót daje oknu bez monitora (`nullptr`) to miejsce i rozmiar, które miało przed pełnym ekranem (zapamiętane w `m_windowedX`, `m_windowedY` i `m_windowedSize`). Po każdym przełączeniu `glfwSwapInterval(1)` jest wołane znowu, bo niektóre sterowniki zapominają interwał, gdy okno zmienia rodzaj (komentarz w kodzie). Monitor wybiera `screenOf`: ten, z którym okno dzieli największą powierzchnię, a gdy żaden, główny.

**Wiersz rozmiaru okna jest przyciemniony przy pełnym ekranie** (klasa `off`), pokazuje rozdzielczość pulpitu, a jego przyciski nic nie robią (`handleMenuCommand`).

**macOS.** Kod pełnego ekranu, ustawiania rozmiaru i fokusu **nie był budowany ani uruchamiany na macOS**. Do sprawdzenia tam: czy `glfwSetWindowMonitor` z bieżącym trybem wideo daje na Retina pełny ekran bez zmiany trybu, jak działają współrzędne ekranu (okno) a piksele bufora ramki (obraz) przy `desktopSize` i `setWindowedSize`, i czy wyłączenie wraca do poprzedniego miejsca ([`../../guides/build-macos.md`](../../guides/build-macos.md)).

### 2.6 Pauza po utracie fokusu

Okno ma pytanie `isFocused()` (`glfwGetWindowAttrib(GLFW_FOCUSED)`). `onRender` zadaje je **raz na klatkę** i porównuje z klatką wcześniejszą:

```cpp
    // A player who switches to another program does not want the round to go on: the
    // frame in which the window stops being the active one pauses it (game::nextMode,
    // FocusLost). Asked once per frame and compared with the frame before, so it
    // happens once. The menu camera is a recording tool, and the program that records
    // it is the active one then: it is left alone.
    const bool windowFocused = window().isFocused();
    if (m_windowWasFocused && !windowFocused && !m_menuCamera.enabled) {
        if (updatesRound(m_mode)) {
            core::logInfo("The window is no longer the active one: the round is paused");
        }
        handleGameEvent(GameEvent::FocusLost);
    }
    m_windowWasFocused = windowFocused;
```

(Plik: `src/game/NightMazeApp.cpp`, początek `onRender`.) Klatka, w której fokus znika, wysyła `GameEvent::FocusLost`. W `nextMode` zdarzenie ma znaczenie tylko na ekranie `Playing`, gdzie daje `Paused`, tak samo jak Escape; na każdym innym ekranie nic nie zmienia, więc `handleGameEvent` wraca od razu ([`game-states.md`](game-states.md)). Trzy zasady z kodu: **kamera menu** (narzędzie do nagrywania, a program, który nagrywa, jest wtedy aktywnym oknem) pauzy nie wywołuje; okno, które **startuje bez fokusu**, nie pauzuje (konstruktor ustawia `m_windowWasFocused` na stan okna); linia w logu pojawia się tylko, gdy runda rzeczywiście zostaje wstrzymana. Pauza po utracie fokusu **nie wznawia się sama**: gracz wraca przyciskiem `Resume` albo Escape.

### 2.7 Zmiany z ekranu ustawień: suwaki i przyciski

Przycisk ekranu ustawień jest **poleceniem** (`handleMenuCommand`: `toggle-fullscreen`, `window-size-smaller`, `window-size-larger`, `reset-settings`), a suwak **raportuje wartość** (`data-setting`):

```cpp
void NightMazeApp::handleControlChanges() {
    const std::vector<ui::ControlChange> changes = m_ui.takeChanges();
    // Only the settings screen has controls that report: a change that arrives on
    // another screen is a late echo of fillSettingsDocument and is dropped.
    if (m_mode != GameMode::SettingsFromMenu && m_mode != GameMode::SettingsFromPause) {
        return;
    }
    for (const ui::ControlChange& change : changes) {
        GameSettings changed = m_settings;
        // A slider also reports the value the code has just given it. Then nothing is
        // different and nothing is done.
        if (!applySetting(changed, change.name, change.value) || changed == m_settings) {
            continue;
        }
        m_settings = changed;
        applyViewSettings();
        // Only the numbers next to the sliders: the sliders themselves already stand
        // where the player put them.
        m_ui.setText(m_settingsDocument, std::string(MOUSE_SENSITIVITY_SETTING) + TEXT_ID_SUFFIX,
                     mouseSensitivityLabel(m_settings.mouseSensitivity));
        m_ui.setText(m_settingsDocument, std::string(FIELD_OF_VIEW_SETTING) + TEXT_ID_SUFFIX,
                     fieldOfViewLabel(m_settings.fieldOfViewDegrees));
    }
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `handleControlChanges`.) Wartość suwaka jest tekstem, który `applySetting` czyta tak samo jak plik, więc **granice i zaokrąglenia są w jednym miejscu**. Suwak raportuje także wartość, którą kod dopiero co sam mu nadał (`fillSettingsDocument` woła `setValue`): wtedy nic się nie różni i nic się nie dzieje. Zmiana, która przychodzi na innym ekranie niż ustawienia, jest spóźnionym echem i jest odrzucana. Przycisk `Reset defaults` przywraca wszystko poza poziomem trudności, który wybiera się w menu głównym. Element `<span id="saved">` w `settings.rml` istnieje, ale **kod niczego do niego nie pisze** (szukałem w `NightMazeApp.cpp`): jest pusty i nieużywany, a jego przeznaczenia nie znam.

## 3. Jak to działa w OpenGL

Część czysta (`Settings.*`) nie woła OpenGL. Zmiana rozmiaru okna i pełny ekran dotykają okna GLFW i **kontekstu OpenGL**: `glfwSetWindowMonitor` zmienia rodzaj okna, więc po nim wołane jest znowu `glfwSwapInterval(1)` (synchronizacja z odświeżaniem). Rozmiar bufora ramki zmienia się razem z oknem, a gra pyta o niego w każdej klatce (viewport, `UiLayer::draw`), więc nie ma osobnej obsługi zmiany rozmiaru. Pole widzenia zmienia macierz rzutowania z kopii kamery.

## 4. Shadery

Brak: moduł nie zmienia żadnego shadera.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| `src/game/Settings.hpp`, `.cpp` | `GameSettings`, `WindowSize`, `WINDOW_SIZES`, granice i wartości domyślne, `SETTINGS_FILE_NAME`, `applySetting`, `parseSettings`, `formatSettings`, `mouseDegreesPerUnit`, `mouseSensitivityLabel`, `fieldOfViewLabel`, `windowSizeValue`, `windowSizeLabel`, `windowSizeChoices`, `steppedWindowSize`. Część `game_logic` |
| `src/core/Files.hpp`, `.cpp` | `readTextFile`, `writeTextFile` (obok `readBinaryFile`); nie logują, wołający pisze komunikat |
| `src/core/Window.hpp`, `.cpp` | `setWindowedSize`, `setFullscreen`, `isFullscreen`, `desktopSize`, `isFocused` |
| `src/game/NightMazeApp.*` | `loadSettings`, `saveSettings`, `applyViewSettings`, `applyWindowSettings`, `handleControlChanges`, `handleMenuCommand`, `fillSettingsDocument`, składowe `m_settings`, `m_savedSettings`, `m_windowWasFocused` |
| `assets/ui/settings.rml` | ekran ustawień |
| `.gitignore` | wpis `night-maze-settings.txt` |
| `tests/SettingsTests.cpp` | 17 przypadków |

### 5.2 Odczyt i zapis plików

```cpp
/// Reads a whole file as text: its bytes as they are, with no change to the line ends.
/// Returns false when the file cannot be opened, and leaves text as it was then. Like
/// readBinaryFile it logs nothing.
bool readTextFile(const std::filesystem::path& path, std::string& text);

/// Writes text into a file, in place of what the file held before. The bytes are written
/// as they are. Returns false when the file cannot be opened for writing or not all of
/// the text could be written. It logs nothing.
bool writeTextFile(const std::filesystem::path& path, std::string_view text);
```

(Plik: `src/core/Files.hpp`.) Oba zwracają fałsz, gdy plik się nie otwiera (zapis także, gdy nie wszystkie bajty udało się zapisać), i **niczego nie logują**, tak jak `readBinaryFile`. Tekst jest brany bajtami, bez zmiany końców linii (dlatego `parseSettings` zna `\r`).

### 5.3 Testy

`SettingsTests.cpp` (17 przypadków), wszystkie bez okna i bez plików:

| Test | Co sprawdza |
|---|---|
| `without a file the settings are the game as it always was` | wartości domyślne |
| `a settings file is read line by line` | pięć linii daje pięć wartości |
| `what is written is read back the same` | zapis i odczyt, także dla wartości domyślnych (ten sam tekst dwa razy) |
| `the file is plain text a person can read and edit` | dokładny tekst pliku domyślnego |
| `spaces, Windows line ends and a byte order mark do not matter` | tolerancja |
| `lines that cannot be read are skipped and the rest still counts` | nieznana nazwa, zła wartość, brak `=`, pusta nazwa |
| `a broken file never breaks the game` | bajty nie będące tekstem, długie linie, same znaki `=` |
| `when a name appears twice the last line wins` | powtórzona nazwa |
| `numbers outside their limits are brought to the nearest limit` | przycinanie |
| `one setting is set from the text a control of the settings screen sends` | `applySetting` |
| `a value that cannot be read changes nothing and says so` | `applySetting` zwraca fałsz |
| `the mouse turns in proportion to the sensitivity` | `mouseDegreesPerUnit` |
| `a window size is written for the file and for the menu` | `windowSizeValue` i `windowSizeLabel` |
| `the settings screen offers the window sizes that fit on the desktop` | `windowSizeChoices` |
| `the window size in use is always among the choices` | rozmiar spoza listy |
| `the numbers of the settings screen are written like in the file` | etykiety |
| `the window size steps through the choices and stops at both ends` | `steppedWindowSize` |

**Nie ma testu na:** odczyt i zapis prawdziwego pliku (`loadSettings`, `saveSettings`: kod z plikiem i logiem), `Window::setFullscreen` i `isFocused` (kod z oknem), ani na pauzę po utracie fokusu.

## 6. Okno debugowania (dawniej panel ImGui)

Okno debug nie ma kontrolek ustawień gracza. Dwie z liczb (`FOV` i `Mouse sensitivity`) ma w kategorii Player i zmienia je wprost, bez zapisu (sekcja 2.2).

## 7. Pułapki

1. **Plik jest względny.** Leży tam, gdzie gra została uruchomiona. Uruchomiona z innego katalogu, czyta i zapisuje inny plik.
2. **Zapis przy zamknięciu.** Gra zapisuje ustawienia w destruktorze. Zakończenie procesu siłą (zabicie) tego nie zrobi: plik ma wtedy ustawienia z ostatniego wyjścia z ekranu ustawień albo startu gry.
3. **Zmiana poziomu w menu głównym** trafia do pliku dopiero przy `Play`, wyjściu z ekranu ustawień albo zamknięciu.
4. **Komentarz po `#` nie na początku linii** psuje wartość, a linia jest pomijana.
5. **`on` i `off` tylko małymi literami.** `ON` jest odrzucone.
6. **Wiersz rozmiaru okna jest martwy przy pełnym ekranie** (przyciemniony, przyciski nic nie robią).
7. **Zmiana z okna debug nie jest zapisywana** i zostaje nadpisana przy następnej zmianie z ekranu ustawień.
8. **Pauza po utracie fokusu nie wznawia się sama** i nie działa przy kamerze menu.
9. **macOS.** Pełny ekran, rozmiar okna i fokus nie były uruchamiane.
10. **Skalowanie ekranu inne niż 100 procent i drugi monitor** nie były sprawdzone na ekranie.

## 8. Ćwiczenia

1. **Plik na kartce.** Co daje `parseSettings("field_of_view = 200\nfullscreen = ON\n")`? Odpowiedź: pole widzenia 90 (przycięte), pełny ekran `off` (wartość `ON` odrzucona, zostaje domyślna).
2. **Kroki rozmiaru.** Pulpit 1366 na 768, bieżący rozmiar 1280 na 720. Jakie są wybory i co daje krok +1 dwa razy? Odpowiedź: 1280x720 i 1366x768, a drugi krok zostaje na 1366x768.
3. **Czułość.** Ile stopni na jednostkę ruchu myszy daje czułość 7? Odpowiedź: `7 * 0,025 = 0,175`.
4. **Pełny ekran z pliku.** Wpisz `fullscreen = on` do pliku i uruchom. Co robi gra? Odpowiedź: konstruktor woła `applyWindowSettings`, więc okno przechodzi do pełnego ekranu na ekranie, na którym leży (agent widział start prosto w pełny ekran).
5. **Pauza.** Przełącz się Alt+Tab w trakcie rundy. Co się dzieje, gdy wrócisz? Odpowiedź: runda jest wstrzymana (pauza) i czeka na `Resume`.
6. **Zepsuty plik.** Zapisz w pliku sto tysięcy liter `x`. Co robi gra? Odpowiedź: startuje z ustawieniami domyślnymi (test `a broken file never breaks the game`). Plik zostaje, jaki był, dopóki ustawienia nie różnią się od wczytanych, a pierwsza zmiana zapisuje poprawny plik.

## 9. Pytania kontrolne

1. **Dlaczego plik ma format `nazwa = wartość`, a nie JSON?**
   Wybór wykonawczy: człowiek może go przeczytać i poprawić w Notatniku, a parser to kilkadziesiąt linii bez biblioteki.
2. **Dlaczego własny czytnik liczb?**
   `strtof` czyta `4,5` według ustawień regionalnych, więc plik znaczyłby co innego na różnych komputerach.
3. **Co się dzieje z linią, której gra nie rozumie?**
   Jest pomijana, a reszta pliku działa. Dzięki temu plik zapisany przez nowszą wersję gry się wczytuje.
4. **Kiedy gra zapisuje plik?**
   Przy wyjściu z ekranu ustawień, przy starcie nowej gry i w destruktorze, i tylko wtedy, gdy ustawienia różnią się od zapisanych.
5. **Co znaczy "pełny ekran jako pulpit taki, jaki jest"?**
   Okno dostaje bieżący tryb wideo ekranu, na którym leży, więc rozdzielczość pulpitu się nie zmienia.
6. **Dlaczego `setFullscreen` woła `glfwSwapInterval(1)` jeszcze raz?**
   Niektóre sterowniki zapominają interwał, gdy okno zmienia rodzaj.
7. **Kto wysyła `FocusLost` i kiedy?**
   `onRender`, w klatce, w której `isFocused()` zmieniło się z prawdy na fałsz, poza kamerą menu.
8. **Dlaczego rozmiar okna jest przyciemniony przy pełnym ekranie?**
   Bo rozmiar okna nic wtedy nie znaczy: pełny ekran ma rozdzielczość pulpitu.

## 10. Źródła

- Notatki: [`../../decisions/menu-scope-for-m9.md`](../../decisions/menu-scope-for-m9.md).
- Dokumenty: [`difficulty.md`](difficulty.md), [`game-states.md`](game-states.md), [`../ui/menu-screens.md`](../ui/menu-screens.md), [`../core/window-context.md`](../core/window-context.md), [`../core/paths.md`](../core/paths.md), [`../core/input.md`](../core/input.md), [`menu-camera.md`](menu-camera.md).
- GLFW, okna i monitory: <https://www.glfw.org/docs/latest/window_guide.html> (`glfwSetWindowMonitor`, atrybut `GLFW_FOCUSED`).
