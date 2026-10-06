# Moduł core: klawiatura i mysz

Kamień milowy: M0 (klawiatura), M1 (mysz), M8 część 2 (pozycja kursora i klawisz E, 2026-10-06). Temat wykładu: 1 (Pierwszy program OpenGL), a pozycja kursora służy tematowi 15 (selekcja obiektów, [`../scene/picking.md`](../scene/picking.md)).
Kod: [`src/core/Input.hpp`](../../../src/core/Input.hpp), [`src/core/Input.cpp`](../../../src/core/Input.cpp), użycie w [`src/core/Application.cpp`](../../../src/core/Application.cpp), [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (obrót kamery myszą i klawisze gracza) i [`src/main.cpp`](../../../src/main.cpp).

Część modułu `core`. Wstęp do całego modułu i diagram warstw są w [`README.md`](README.md). Pozostałe części: [`window-context.md`](window-context.md) (okno i kontekst), [`main-loop.md`](main-loop.md) (pętla i czas), [`gl-check.md`](gl-check.md) (błędy OpenGL). Funkcje wejścia samej biblioteki GLFW opisuje [`../../libraries/glfw.md`](../../libraries/glfw.md).

## 1. Po co to jest

Gra potrzebuje odpowiedzi na dwa różne pytania o klawisz: "czy jest teraz trzymany" (ruch) i "czy został właśnie wciśnięty" (przełącznik, zamknięcie programu). GLFW odpowiada wprost tylko na pierwsze. Klasa `core::Input` raz na klatkę robi migawkę (snapshot) stanu całej klawiatury, pamięta migawkę z poprzedniej klatki i z ich porównania wyprowadza drugie pytanie. Ma też jedną flagę, `setKeyboardBlocked`, którą ktoś z zewnątrz może na chwilę odebrać grze klawiaturę. Dziś robi to `main.cpp`, gdy klawiatury używa panel ImGui: dzięki temu Escape wciśnięty podczas edycji pola w panelu nie zamyka programu.

Z myszą jest tak samo, tylko pytań jest więcej. Przyciski myszy mają te same dwa pytania co klawisze (`isMouseButtonDown`, `wasMouseButtonPressed`). Dochodzi trzecie: "o ile kursor przesunął się od poprzedniej klatki" (`mouseDeltaX`, `mouseDeltaY`), bo właśnie z przesunięcia, a nie z pozycji, liczy się obrót kamery. `Input` potrafi też przechwycić kursor (`setCursorCaptured`): schować go i zdjąć z niego ograniczenie krawędziami ekranu. Mysz ma własną flagę blokady, `setMouseBlocked`, ustawianą przez `main.cpp`, gdy kursor jest nad panelem ImGui.

Stan na dziś: użytkownikiem myszy i klawiszy jest `game::NightMazeApp`. Kliknięcie lewym przyciskiem w scenę przechwytuje kursor (`wasMouseButtonPressed`, `setCursorCaptured(true)`), przesunięcie myszy obraca kamerę (`mouseDeltaX`, `mouseDeltaY`), klawisze W, A, S, D, spacja i lewy Shift przesuwają gracza (`isKeyDown`, wpisywane do struktury `PlayerInput`), klawisz R zaczyna rundę od nowa, klawisz N przełącza tryb noclip, klawisz F latarkę, a od szóstej części M7 klawisz M włącza i wyłącza minimapę (wszystkie cztery `wasKeyPressed`, czytane raz na klatkę w `onRender`), a Escape oddaje kursor. HUD z M5 (licznik kryształów, bateria, karta wygranej) nie jest użytkownikiem wejścia: niczego nie czyta ani z `Input`, ani z ImGui. Obrót myszą opisuje [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 5, ruch gracza [`../game/player.md`](../game/player.md), sekcja 5, a całe `onUpdate` i `onRender` [`README.md`](README.md), sekcje 6.5 i 6.6. Ten dokument opisuje narzędzia, z których on korzysta. Od M9, części 1 dochodzi klawisz F2 (tryb kamery menu): w tym trybie klawisze i mysz rundy są ignorowane, a przy jego włączeniu kursor jest oddawany.

**Stan z 2026-10-06 (M9, część 2).** Escape nie służy już do oddania kursora przy biegnącej rundzie (otwiera pauzę, która zatrzymuje rundę) i nie zamyka programu: `Application::run` woła metodę wirtualną `onEscapePressed()`, a gra pauzuje (sekcja 5.7, fragment kodu, ma nowe brzmienie). Kursor jest przechwytywany, gdy zaczyna się runda (`NightMazeApp::showScreen`), a nie dopiero po kliknięciu w scenę. Kliknięcie w scenę przechwytuje go jeszcze po pokazaniu paneli klawiszem `~` (`main.cpp` oddaje wtedy kursor). Blokada klawiatury obejmuje od teraz także pole tekstowe menu (`menuUi().wantsKeyboard()`), a menu bez pola tekstowego niczego nie blokuje. Starsze zdania w tym dokumencie o Escape i o kliknięciu jako jedynej drodze do przechwycenia opisują stan sprzed M9, części 2.

## 2. Teoria

### 2.1 Odpytywanie zamiast zdarzeń

Biblioteka okienkowa daje dwa sposoby czytania klawiatury i myszy:

- **zdarzenia** (events, callbacki): biblioteka woła moją funkcję przy każdym naciśnięciu, puszczeniu i ruchu kursora,
- **odpytywanie** (polling): w dowolnej chwili pytam "jaki jest teraz stan klawisza X", "gdzie jest kursor".

`Input` używa odpytywania dla obu urządzeń. Jest prostsze (żadnych funkcji zwrotnych, żadnego stanu globalnego), pasuje do pętli gry, która i tak wykonuje się co klatkę, i nie koliduje z callbackami, które instaluje backend ImGui. Ten ostatni powód jest przy myszy najważniejszy: GLFW przechowuje tylko jeden callback danego rodzaju na okno, a backend ImGui zajmuje callbacki pozycji kursora i przycisków myszy. Własny `glfwSetCursorPosCallback` ustawiony po utworzeniu `DebugUI` podmieniłby callback backendu i panele przestałyby widzieć mysz ([`../debug-ui.md`](../debug-ui.md), sekcja 7, pułapka 6). Ceną odpytywania jest to, że widzę tylko stan w chwili odczytu, a nie historię zdarzeń (sekcja 7, pułapka 3).

### 2.2 Stan ciągły i zbocze

- **Stan ciągły** (level): klawisz albo przycisk jest wciśnięty albo nie. Wartość trwa tak długo, jak go trzymam. Do ruchu.
- **Zbocze** (edge): chwila przejścia z "puszczony" na "wciśnięty". Trwa dokładnie jedną klatkę. Do akcji jednorazowych.

Zbocza nie da się odczytać z jednego pomiaru. Potrzebne są dwa: bieżący i poprzedni. Stąd dwie tablice dla klawiszy i dwie dla przycisków myszy.

### 2.3 Kto ma klawiaturę i mysz

W programie są dwaj odbiorcy tego samego klawisza i tego samego kliknięcia: gra i interfejs debugowy. Gdy wpisuję liczbę w pole panelu, naciśnięcia mają trafić tylko do panelu. Gdy przeciągam suwak w panelu, ruch myszy nie może jednocześnie obracać kamery. Ktoś musi więc rozstrzygać, kto w danej chwili "ma" klawiaturę, a kto mysz. ImGui wie, czy ich używa (pola `WantCaptureKeyboard` i `WantCaptureMouse`, zob. [`../../libraries/imgui.md`](../../libraries/imgui.md), sekcja 3.8), a `Input` jest miejscem, przez które przechodzą wszystkie pytania gry o wejście. Wystarczą więc dwie flagi w `Input` (po jednej na urządzenie) i jedno miejsce, które je ustawia. Jak to jest zrobione bez wprowadzania ImGui do `core/`, opisują sekcje 5.6 i 5.10.

### 2.4 Mysz: pozycja a przesunięcie

O kursor można pytać na dwa sposoby:

- **pozycja** (position): gdzie kursor jest teraz, na przykład (640, 360). Potrzebna do klikania w elementy na ekranie.
- **przesunięcie** (delta): o ile kursor przesunął się od poprzedniego odczytu, na przykład (+12, -3). Potrzebne do obracania kamery: ruch myszy w prawo o 12 jednostek to obrót w prawo o 12 razy czułość.

Do M8, części 2 `Input` udostępniał tylko przesunięcie, bo tylko ono było potrzebne (kamerze). Od M8, części 2 oddaje też **pozycję** (`Input::cursorPosition`, sekcja 5.8.1), bo wskazywanie obiektów kursorem potrzebuje punktu na obrazie. GLFW przy odpytywaniu podaje pozycję (`glfwGetCursorPos`), więc przesunięcie liczę sam: pozycja z tej klatki minus pozycja z poprzedniej. Tak jak przy zboczu, potrzebne są dwa pomiary. Z tego wynika problem "pierwszej myszy" (first mouse): przy pierwszym odczycie nie ma poprzedniej pozycji, z którą można porównać. Gdybym odjął pozycję od zera, dostałbym jedno ogromne przesunięcie i kamera szarpnęłaby w losowym kierunku. Ten sam problem wraca przy każdej zmianie trybu kursora (sekcja 2.6).

### 2.5 Współrzędne ekranu a piksele

GLFW podaje pozycję kursora we **współrzędnych ekranu** (screen coordinates), czyli w tych samych jednostkach, w których podaje rozmiar okna (`glfwGetWindowSize`), a nie w pikselach framebuffera. Początek układu to lewy górny róg obszaru roboczego okna, oś x rośnie w prawo, a oś **y rośnie w dół**. To odwrotnie niż w OpenGL, gdzie y rośnie w górę: ruch myszy "do góry" daje ujemne `mouseDeltaY`.

Na ekranie Retina okno 1280 x 720 ma framebuffer 2560 x 1440 ([`window-context.md`](window-context.md), sekcja 2). Kursor przesunięty przez całą szerokość okna daje przesunięcie 1280, nie 2560. Dla obrotu kamery to nawet wygodne (czułość nie zależy od gęstości ekranu), ale gdy pozycja myszy ma kiedyś wskazywać piksel framebuffera, trzeba ją przeskalować.

### 2.6 Tryby kursora

GLFW ma tryb kursora ustawiany per okno przez `glfwSetInputMode(window, GLFW_CURSOR, tryb)`:

| Tryb | Kursor widoczny | Ruch ograniczony ekranem | Do czego |
|---|---|---|---|
| `GLFW_CURSOR_NORMAL` | tak | tak | zwykła praca z oknem i panelami, tryb domyślny |
| `GLFW_CURSOR_HIDDEN` | nie, gdy jest nad oknem | tak | własny rysowany kursor. Nie nadaje się do kamery: kursor nadal zatrzymuje się na krawędzi ekranu i wychodzi poza okno |
| `GLFW_CURSOR_DISABLED` | nie | nie | sterowanie kamerą myszą (mouse look). GLFW chowa kursor, trzyma go w oknie i podaje **wirtualną** pozycję, która może rosnąć bez końca |

Używam dwóch: `NORMAL` i `DISABLED`. W trybie `DISABLED` `glfwGetCursorPos` zwraca pozycję wirtualną, niezwiązaną z ekranem: mogę kręcić myszą w prawo dowolnie długo, a x będzie rósł. Dokładnie tego potrzebuje kamera FPS. Zmiana trybu zmienia też znaczenie pozycji (prawdziwa albo wirtualna), więc pozycja odczytana tuż po zmianie może odskoczyć od poprzedniej. Przykład: po powrocie do `NORMAL` GLFW stawia kursor tam, gdzie był w chwili przechwycenia, a ostatnia pozycja wirtualna mogła być o tysiące jednostek dalej.

### 2.7 Surowy ruch myszy

System operacyjny przetwarza ruch myszy, zanim przesunie kursor: stosuje przyspieszenie (szybki ruch ręką przesuwa kursor dalej niż wolny ruch na tym samym dystansie) i skalowanie. To pomaga trafić w przycisk, ale przeszkadza w celowaniu kamerą, gdzie ten sam ruch ręki ma zawsze dawać ten sam obrót. **Surowy ruch myszy** (raw mouse motion) to dane prosto z urządzenia, bez tego przetwarzania. W GLFW włącza go `glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE)`, ale:

- nie każda platforma go obsługuje, więc najpierw trzeba zapytać `glfwRawMouseMotionSupported()`. W GLFW 3.4 Windows zwraca prawdę, a macOS fałsz,
- działa tylko w trybie `GLFW_CURSOR_DISABLED`. Przy widocznym kursorze nie ma żadnego skutku.

### 2.8 Dane "na klatkę" a stały krok symulacji

Przesunięcie myszy, tak jak zbocze klawisza, jest wartością opisującą **jedną klatkę**: odstęp między dwoma kolejnymi wywołaniami `Input::update()`. Symulacja idzie natomiast stałym krokiem, a `onUpdate` wykonuje się od zera do wielu razy na klatkę ([`main-loop.md`](main-loop.md), sekcja 2.2). Gdybym dodawał przesunięcie do obrotu kamery w `onUpdate`, to w klatce bez kroku ruch myszy by przepadł, a w klatce z trzema krokami ten sam ruch zostałby zastosowany trzy razy. Obrót zależałby od FPS. Dlatego przesunięcie i `wasMouseButtonPressed` wolno czytać tylko z kodu wykonywanego raz na klatkę (`onRender`). `isMouseButtonDown` jest stanem ciągłym i jest bezpieczne w obu miejscach.

## 3. Jak to działa w OpenGL

Klawiatura i mysz nie są częścią OpenGL: `Input` nie woła żadnej funkcji `gl*`. Rozmawia wyłącznie z GLFW:

| Wywołanie | Kto woła | Co robi |
|---|---|---|
| `glfwPollEvents()` | `Window::pollEvents()` w `Application::run` | Odbiera zdarzenia systemu i aktualizuje stan klawiszy, przycisków i pozycję kursora zapamiętane wewnątrz GLFW |
| `glfwGetKey(m_window, key)` | `Input::update()` | Zwraca `GLFW_PRESS` albo `GLFW_RELEASE`: stan zapamiętany podczas ostatniego `glfwPollEvents`. Nie pyta systemu |
| `glfwGetMouseButton(m_window, button)` | `Input::update()` | To samo dla przycisku myszy. Przyciski mają numery od `GLFW_MOUSE_BUTTON_1` (0) do `GLFW_MOUSE_BUTTON_LAST` (7). `GLFW_MOUSE_BUTTON_LEFT`, `RIGHT` i `MIDDLE` to inne nazwy numerów 0, 1 i 2 |
| `glfwGetCursorPos(m_window, &x, &y)` | `Input::update()` | Wpisuje pozycję kursora we współrzędnych ekranu, względem lewego górnego rogu obszaru roboczego okna. W trybie `GLFW_CURSOR_DISABLED` jest to pozycja wirtualna |
| `glfwSetInputMode(m_window, GLFW_CURSOR, tryb)` | `Input::setCursorCaptured()` | Ustawia tryb kursora: `GLFW_CURSOR_DISABLED` albo `GLFW_CURSOR_NORMAL` |
| `glfwRawMouseMotionSupported()` | `Input::setCursorCaptured()` | Zwraca `GLFW_TRUE`, jeśli platforma ma surowy ruch myszy |
| `glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, wartość)` | `Input::setCursorCaptured()` | Włącza (`GLFW_TRUE`) albo wyłącza (`GLFW_FALSE`) surowy ruch. Wołane tylko wtedy, gdy poprzednia funkcja zwróciła prawdę, bo na platformie bez wsparcia GLFW zgłosiłby błąd |

Kolejność w pętli wynika z tej tabeli: najpierw `m_window.pollEvents()`, potem `m_input.update()`.

## 4. Shadery

Ta część modułu nie ma shaderów i nie ma z nimi żadnego związku.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/core/Input.hpp`](../../../src/core/Input.hpp) | klasa `Input`. Klawiatura: `update`, `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`, stała `KEY_COUNT`. Mysz: `isMouseButtonDown`, `wasMouseButtonPressed`, `mouseDeltaX`, `mouseDeltaY`, `setMouseBlocked`, `setCursorCaptured`, `isCursorCaptured`, stała `MOUSE_BUTTON_COUNT`. Tablice stanu, pozycja kursora i flagi |
| [`src/core/Input.cpp`](../../../src/core/Input.cpp) | implementacja i dwa `static_assert` pilnujące `KEY_COUNT` i `MOUSE_BUTTON_COUNT` |
| [`src/core/Application.cpp`](../../../src/core/Application.cpp) | `m_input.update()` raz na klatkę i obsługa Escape (zwolnienie kursora albo zamknięcie programu) |
| [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) | użytkownik: klawisze N, F, E (od M8, części 2) i (od szóstej części M7) M, pozycja kursora (`cursorPosition`) do promienia wskazywania, kliknięcie, przechwycenie kursora i przesunięcie myszy w `onRender`, klawisze ruchu w `onUpdate` ([`README.md`](README.md), sekcje 6.5 i 6.6, [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 5, [`../game/player.md`](../game/player.md), sekcja 5) |
| [`src/main.cpp`](../../../src/main.cpp) | przełącznik paneli (`GLFW_KEY_GRAVE_ACCENT`, chowa panele, ale nie HUD), ustawianie blokady klawiatury i myszy, wyłączanie myszy w ImGui na czas przechwycenia kursora |

### 5.2 `update`: migawka klawiatury i myszy

```cpp
void Input::update() {
    m_previous = m_current;
    // GLFW key codes start at GLFW_KEY_SPACE (32), lower values are not valid keys.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        m_current[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    m_mousePrevious = m_mouseCurrent;
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        m_mouseCurrent[button] = glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    // Cursor position in screen coordinates, relative to the top left corner of the window.
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(m_window, &cursorX, &cursorY);
    if (m_skipNextMouseDelta) {
        // There is no trustworthy previous position: this is the first update, or the
        // cursor mode has just changed and the reported position may have jumped.
        // Report no movement instead of one huge step (the "first mouse" problem).
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_skipNextMouseDelta = false;
    } else {
        m_mouseDeltaX = cursorX - m_cursorX;
        m_mouseDeltaY = cursorY - m_cursorY;
    }
    m_cursorX = cursorX;
    m_cursorY = cursorY;
}
```

Funkcja ma trzy części: klawisze, przyciski myszy, kursor. Część myszy omawia sekcja 5.8, tutaj klawiatura.

Raz na klatkę kopiuję stan bieżący do poprzedniego i odczytuję nowy stan wszystkich klawiszy. `glfwGetKey` nie pyta systemu, tylko zwraca stan zapamiętany przez GLFW podczas ostatniego `glfwPollEvents`, dlatego w pętli `pollEvents()` stoi przed `m_input.update()`. Pętla zaczyna od `GLFW_KEY_SPACE` (32), bo niższe kody nie są klawiszami i GLFW zgłosiłby dla nich błąd.

Ważne dla sekcji 5.6 i 5.10: `update` **nie sprawdza** flag `m_keyboardBlocked` ani `m_mouseBlocked`. Tablice i pozycja kursora są odświeżane w każdej klatce, także wtedy, gdy klawiatura albo mysz jest zablokowana.

Konstruktor `explicit Input(GLFWwindow* window)` tylko zapamiętuje uchwyt okna. `explicit` zabrania niejawnej konwersji wskaźnika na `Input`. Okno musi żyć dłużej niż `Input`, co gwarantuje kolejność pól w `Application` ([`README.md`](README.md), sekcja 7).

### 5.3 `isKeyDown` i `wasKeyPressed`

Mając dwie migawki, odróżniam dwa pytania:

```cpp
bool Input::isKeyDown(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key];
}

bool Input::wasKeyPressed(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key] && !m_previous[key];
}
```

```cpp
bool Input::isValidKey(int key) const {
    return key >= 0 && key < KEY_COUNT;
}
```

- `isKeyDown`: stan ciągły, "klawisz jest teraz wciśnięty". Do ruchu (trzymam W, idę).
- `wasKeyPressed`: **zbocze** (edge), "w tej klatce jest wciśnięty, a w poprzedniej nie był". Do akcji jednorazowych (klawisz na lewo od `1` przełącza panele, R zaczyna rundę od nowa, Escape zwalnia kursor albo zamyka program).

Operator `&&` wylicza warunki od lewej i przerywa na pierwszym fałszywym (short circuit). Kolejność ma więc znaczenie:

1. `!m_keyboardBlocked`: jeśli klawiatura jest zablokowana, odpowiedź brzmi `false` dla każdego klawisza i dalsze warunki nie są sprawdzane.
2. `isValidKey(key)`: chroni przed wyjściem poza tablicę. To nie jest teoria: GLFW ma stałą `GLFW_KEY_UNKNOWN` równą -1. Dopiero po tym warunku wolno indeksować tablicę.
3. `m_current[key]` i (dla zbocza) `!m_previous[key]`.

Klawisze identyfikuję stałymi GLFW (`GLFW_KEY_ESCAPE`, `GLFW_KEY_GRAVE_ACCENT`). Oznaczają one fizyczne położenie klawisza w układzie US, a nie znak, który klawisz wpisuje.

### 5.4 `KEY_COUNT = 348 + 1`, `MOUSE_BUTTON_COUNT = 7 + 1` i `static_assert`

```cpp
// Input.hpp
// One slot per GLFW key code. 348 is GLFW_KEY_LAST, checked in Input.cpp, so that
// this header does not need to include the GLFW header.
static constexpr int KEY_COUNT = 348 + 1;
// One slot per GLFW mouse button. 7 is GLFW_MOUSE_BUTTON_LAST, checked in Input.cpp.
static constexpr int MOUSE_BUTTON_COUNT = 7 + 1;
```

```cpp
// Input.cpp
Input::Input(GLFWwindow* window) : m_window(window) {
    static_assert(KEY_COUNT == GLFW_KEY_LAST + 1, "KEY_COUNT must cover every GLFW key code");
    static_assert(MOUSE_BUTTON_COUNT == GLFW_MOUSE_BUTTON_LAST + 1,
                  "MOUSE_BUTTON_COUNT must cover every GLFW mouse button");
}
```

Tablica klawiszy jest indeksowana bezpośrednio kodem klawisza GLFW. Największy kod to `GLFW_KEY_LAST` (równy `GLFW_KEY_MENU`, czyli 348), więc poprawne indeksy to od 0 do 348, a to wymaga **349** elementów: stąd `+ 1`. Indeksy od 0 do 31 pozostają nieużywane, to świadomy koszt 32 bajtów w zamian za najprostsze możliwe indeksowanie. `{}` przy polu zeruje tablicę (wszystkie klawisze puszczone).

Tablice przycisków myszy działają identycznie: największy numer to `GLFW_MOUSE_BUTTON_LAST` (7), więc potrzeba **8** elementów. Tu nie ma nieużywanych indeksów, bo numery przycisków zaczynają się od 0.

Dlaczego gołe `348` i `7`, a nie `GLFW_KEY_LAST` i `GLFW_MOUSE_BUTTON_LAST`? Żeby `Input.hpp` nie musiał dołączać nagłówka GLFW (tak samo jak `Window.hpp`, zob. [`window-context.md`](window-context.md), sekcja 5.2). Goła liczba jest jednak "magiczna" i mogłaby się rozjechać z biblioteką po aktualizacji GLFW. Dlatego w `Input.cpp`, gdzie nagłówek GLFW jest już dołączony, stoją dwa `static_assert`: warunki sprawdzane **w czasie kompilacji**, bez żadnego kosztu w działającym programie. Jeśli nowa wersja GLFW doda klawisze albo przyciski, build się nie powiedzie i pokaże podany komunikat. Asercje są wewnątrz konstruktora, bo obie stałe są prywatne i trzeba być w funkcji składowej, żeby mieć do nich dostęp.

### 5.5 Dlaczego `wasKeyPressed` jest "na klatkę rysowaną" i nie wolno go wołać z `onUpdate`

Zbocze istnieje między dwoma kolejnymi wywołaniami `Input::update()`, a to jest wołane dokładnie raz na obrót pętli głównej. `onUpdate` jest natomiast wołane od zera do wielu razy na klatkę ([`main-loop.md`](main-loop.md), sekcja 2.2):

| Liczba kroków w klatce | Co stałoby się z `wasKeyPressed` wołanym w `onUpdate` |
|---|---|
| 0 (szybka klatka) | Naciśnięcie **przepada**: w tej klatce nikt nie zapytał, a w następnej `m_previous` jest już `true` |
| 1 | Działa, ale tylko przypadkiem |
| 2 lub więcej | Każdy krok widzi to samo `true`, więc akcja wykona się kilka razy. Przełącznik włączony i wyłączony w tej samej klatce wygląda jak "klawisz nie działa" |

Dlatego obsługa przełącznika paneli stoi w `DebugNightMazeApp::onRender` w `main.cpp` (`if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) { m_debugUI.toggleVisible(); }`), a Escape w `Application::run` przed pętlą kroków: oba miejsca wykonują się dokładnie raz na klatkę. W `onUpdate` wolno używać `isKeyDown`, bo stan ciągły jest taki sam w każdym kroku danej klatki: tak czytane są klawisze ruchu gracza (sekcja 5.7). Jednorazowa akcja, która ma wpłynąć na symulację, musi zostać odczytana raz na klatkę i przekazana do symulacji jako zapamiętany stan. Tak działa klawisz N: `NightMazeApp::onRender` czyta zbocze przez `wasKeyPressed` i przestawia pole `m_player.noclip`, a kroki symulacji czytają już tylko to pole. Tą samą drogą idzie od M4 klawisz F: zbocze czytane w `onRender` przestawia pole `m_lighting.flashlightOn`, z którego niżej w tej samej funkcji, czyli w tej samej klatce, budowane są światła ([`../game/flashlight.md`](../game/flashlight.md)). Od M5 tak samo działa klawisz R: zbocze czytane w `onRender` woła `beginRound()`, czyli wymienia stan rundy między dwoma krokami symulacji, a kroki widzą już tylko nową rundę ([`README.md`](README.md), sekcja 6.6). Gdyby w późniejszych kamieniach milowych doszedł na przykład skok, pójdzie tą samą drogą.

Ta sama tabela opisuje `wasMouseButtonPressed`, `mouseDeltaX` i `mouseDeltaY` (sekcja 2.8): to też dane jednej klatki.

### 5.6 Blokada klawiatury: gra a panel ImGui

**Problem.** `Input` czyta klawisze prosto z GLFW, a ImGui dostaje te same naciśnięcia przez swoje callbacki. Bez dodatkowego mechanizmu oba systemy reagują na ten sam klawisz: Escape wciśnięty po to, żeby anulować edycję pola w panelu, zamknąłby cały program, a klawisz `~` wpisany w pole schowałby panele.

**Rozwiązanie składa się z trzech małych kawałków, każdy w innej warstwie.**

W `core/` ([`Input.hpp`](../../../src/core/Input.hpp)) jest flaga i jej setter:

```cpp
void setKeyboardBlocked(bool blocked) { m_keyboardBlocked = blocked; }
```

```cpp
bool m_keyboardBlocked = false;
```

Dopóki flaga jest ustawiona, `isKeyDown` i `wasKeyPressed` zwracają `false` dla każdego klawisza (pierwszy warunek w obu funkcjach, sekcja 5.3).

W `debug/` ([`DebugUI.cpp`](../../../src/debug/DebugUI.cpp)) jest pytanie do ImGui:

```cpp
bool DebugUI::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}
```

A w `main.cpp`, na końcu `DebugNightMazeApp::onRender`, jedno łączy się z drugim:

```cpp
// While the cursor is captured the mouse belongs to the camera. The hidden cursor
// still has a position that moves with the mouse, so the panels must ignore it,
// otherwise it would hover and click them unseen.
m_debugUI.setMouseEnabled(!input().isCursorCaptured());
// The context is rebuilt every frame: it only holds references, so it is cheap.
m_debugUI.draw(debug::DebugContext{
    .time = time(),
    .window = window(),
    .clearColor = clearColor(),
    .camera = camera(),
    .mouseSensitivity = mouseSensitivity(),
    .texturedShader = texturedShader(),
    .colorShader = colorShader(),
    .player = player(),
    .mazeSettings = mazeSettings(),
    .mazeWorld = mazeWorld(),
    .assets = assets(),
    .viewMode = viewMode(),
    .drawColliders = drawColliders(),
    .litShader = litShader(),
    .gouraudShader = gouraudShader(),
    .lighting = lighting(),
    .gameplay = gameplaySettings(),
    .round = round(),
    .skyboxShader = skyboxShader(),
    .skybox = skyboxSettings(),
    .grassShader = grassShader(),
    .terrain = terrainSettings(),
    .grass = grassSettings(),
    .grassTuftCount = grassTuftCount(),
    .compositeShader = compositeShader(),
    .previewShader = previewShader(),
    .postProcessSettings = postProcessSettings(),
    .postProcess = postProcess(),
    .brightPassShader = brightPassShader(),
    .blurShader = blurShader(),
    .shadowDepthShader = shadowDepthShader(),
    .moonShadowSettings = moonShadowSettings(),
    .moonShadowMap = moonShadowMap(),
    .moonLightSpace = moonLightSpace(),
});

// ImGui now knows whether it is using the keyboard (a text field is being edited
// or a widget is active) and the mouse (the cursor is over a panel or a widget is
// being dragged). Block each device for the game from the next frame on, so typing
// does not trigger Escape, the panel toggle or player movement, and working with
// a panel does not click or look around in the scene.
input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
input().setMouseBlocked(m_debugUI.wantsMouse());
```

Lista pól kontekstu jest tu w stanie z czwartej części M7 (cienie księżyca, 2026-10-05): trzydzieści cztery pola, z czego cztery ostatnie należą do programu głębi i panelu Shadows ([`README.md`](README.md), sekcja 6). Od piątej części M7 (2026-10-06) pól jest trzydzieści osiem: doszły cztery z cieniem latarki. Dla wejścia liczą się tylko trzy wywołania spoza tej listy. Ostatnie z nich to blokada myszy, opisana w sekcji 5.10. Pierwsza (`setMouseEnabled`) działa w przeciwną stronę, odcina ImGui od myszy przy przechwyconym kursorze, i jest opisana w sekcji 5.11.

**Dlaczego `core` dostaje neutralną flagę, a nie pyta ImGui samo.** `core/` to biblioteka `engine`, która linkuje tylko `glad`, `glfw` i nagłówki GLM i zgodnie z regułą warstw nie zna ani `debug/`, ani ImGui ([`README.md`](README.md), sekcja 3). Gdyby `Input::isKeyDown` wołało `ImGui::GetIO()`, `engine` musiałby linkować ImGui, a każdy program zbudowany na `engine` (na przykład zadanie laboratoryjne bez paneli) ciągnąłby tę bibliotekę za sobą. Flaga `m_keyboardBlocked` mówi tylko "ktoś inny ma teraz klawiaturę". `Input` nie wie kto i dlaczego: decyduje ten, kto woła setter. Dziś jest to `main.cpp` i powodem jest ImGui, ale tym samym setterem mogłoby się posłużyć na przykład menu pauzy. `debug::DebugUI` z kolei nie wie, co wołający zrobi z odpowiedzią `wantsKeyboard()`. Oba końce skleja `main.cpp`, jedyny plik znający obie warstwy.

**Dlaczego `WantCaptureKeyboard` jest prawdziwe nie tylko w polach tekstowych.** ImGui ustawia tę flagę, gdy **jakikolwiek widżet jest aktywny** (trwa edycja pola, przeciągany jest suwak albo wartość, trzymany jest przycisk, przesuwane jest okno panelu) albo gdy otwarte jest okno modalne. Powód: aktywny widżet może sam używać klawiszy. Escape anuluje edycję albo przeciąganie, Tab przechodzi do następnego pola, Ctrl, Shift i Alt zmieniają zachowanie przeciągania. ImGui nie próbuje zgadywać, które klawisze "należą" do widżetu, tylko zgłasza, że na czas aktywności bierze całą klawiaturę. Samo najechanie kursorem na panel flagi nie ustawia (to obszar drugiej flagi, `WantCaptureMouse`). Skutek praktyczny: podczas przeciągania wartości `Clear color` Escape nie zamyka programu, a `~` nie chowa paneli.

**Dlaczego blokada jest spóźniona i dlaczego to nie szkodzi.** Opóźnienie ma dwa źródła:

1. `setKeyboardBlocked` stoi na końcu `onRender`. Pytania o klawisze w tej klatce (Escape w `Application::run`, klawisze ruchu w `onUpdate`, R, N i F na początku `onRender`, przełącznik paneli tuż przed `draw`) już padły, więc nowa wartość flagi działa **od następnej klatki**.
2. ImGui wylicza `WantCaptureKeyboard` w `ImGui::NewFrame()`, czyli na początku `DebugUI::draw`, na podstawie tego, który widżet był aktywny po poprzedniej klatce. Widżet kliknięty w bieżącej klatce zostanie więc uwzględniony dopiero w następnym `NewFrame`.

| Klatka | Co się dzieje |
|---|---|
| N | Klikam pole w panelu. Widżet staje się aktywny w trakcie `draw`. Flaga ImGui została policzona wcześniej (w `NewFrame` tej klatki) i jest jeszcze `false`, więc `setKeyboardBlocked(false)` |
| N+1 | `NewFrame` widzi aktywny widżet, `WantCaptureKeyboard` jest `true`. Po `draw` wykonuje się `setKeyboardBlocked(true)`. Pytania o klawisze w tej klatce padły wcześniej, jeszcze bez blokady |
| N+2 i dalej | `isKeyDown` i `wasKeyPressed` zwracają `false` dla wszystkich klawiszy |

Od kliknięcia do pełnej blokady mijają więc najwyżej dwie klatki, czyli około 33 ms przy 60 FPS. Żeby coś poszło źle, musiałbym w tym czasie zdążyć wcisnąć Escape po kliknięciu myszą, a tak szybko człowiek tego nie zrobi. Przy odblokowaniu działa to samo opóźnienie, tylko w drugą stronę, i tam jest wręcz pomocne (następny akapit).

Dlaczego czytam flagę **po** `draw`, a nie przed: flaga jest aktualizowana w `NewFrame`, który stoi w środku `draw`. Przed `draw` dostałbym wartość o jeszcze jedną klatkę starszą.

**Dlaczego tablice odświeżają się mimo blokady.** `update()` wypełnia `m_current` i `m_previous` w każdej klatce, niezależnie od flagi. Flaga wpływa tylko na **odpowiedzi**, nie na pomiar. Gdyby `update` przy blokadzie nie robił nic, tablice zamarzłyby w stanie sprzed blokady i po odblokowaniu pokazałyby fałszywe zbocze. Przykład z Escape, którym anuluję edycję pola:

| Klatka | Zdarzenie | Z odświeżaniem (tak jest w kodzie) | Gdyby tablice były zamrożone |
|---|---|---|---|
| N | wciskam Escape, ImGui anuluje edycję, blokada trwa | `m_current[Esc] = true`, odpowiedź `false` (blokada) | tablice bez zmian: `m_current[Esc] = false` |
| N+1 | ImGui zgłasza, że nie używa już klawiatury, na końcu klatki blokada zostaje zdjęta | `m_previous[Esc] = true`, `m_current[Esc] = true`, odpowiedź `false` (blokada) | tablice bez zmian |
| N+2 | pierwsza klatka bez blokady, Escape wciąż fizycznie wciśnięty | `m_previous[Esc] = true`, więc **nie ma zbocza**, program działa dalej | `m_previous[Esc] = false`, `m_current[Esc] = true`: zbocze, **program się zamyka** |

Dzięki odświeżaniu klawisz wciśnięty w czasie blokady jest po jej zdjęciu widziany jako "trzymany od dawna", a nie "właśnie wciśnięty".

### 5.7 Gdzie klawisze są czytane

| Klawisz | Stała | Pytanie | Gdzie | Co robi |
|---|---|---|---|---|
| Escape | `GLFW_KEY_ESCAPE` | `wasKeyPressed` | `Application::run` w [`Application.cpp`](../../../src/core/Application.cpp) | zwalnia przechwycony kursor, a gdy kursor nie jest przechwycony, zamyka program (`m_window.requestClose()`) |
| klawisz na lewo od `1` (na klawiaturze US `` ` `` i `~`) | `GLFW_KEY_GRAVE_ACCENT` | `wasKeyPressed` | `DebugNightMazeApp::onRender` w [`main.cpp`](../../../src/main.cpp) | chowa i pokazuje panele debug. HUD gry zostaje na ekranie: `DebugUI::draw` rysuje go niezależnie od tego przełącznika |
| R | `GLFW_KEY_R` (stała `RESTART_KEY`) | `wasKeyPressed` | `NightMazeApp::onRender` w [`NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), zaraz po prośbie o nowy labirynt | zaczyna rundę od nowa na tym samym labiryncie (`beginRound()`). Ten sam skutek ma flaga `GameplaySettings::restart`, którą ustawia przycisk `Restart round (key R)` panelu Gameplay: oba warunki stoją w jednym `if`. Działa także przy wolnym kursorze i po wygranej ([`../game/gameplay.md`](../game/gameplay.md)) |
| N | `GLFW_KEY_N` (stała `NOCLIP_KEY`) | `wasKeyPressed` | `NightMazeApp::onRender`, pod klawiszem R | przełącza `m_player.noclip`: chodzenie z kolizjami albo lot bez kolizji. Działa także przy wolnym kursorze |
| F | `GLFW_KEY_F` (stała `FLASHLIGHT_KEY`) | `wasKeyPressed` | `NightMazeApp::onRender`, tuż pod klawiszem N | przełącza `m_lighting.flashlightOn`: latarka gracza świeci albo nie. Działa także przy wolnym kursorze ([`../game/flashlight.md`](../game/flashlight.md)). Przy pustej baterii klawisz nadal ustawia pole, ale najbliższy stały krok (`updateRound`) je gasi, a klatka i tak jest rysowana bez latarki (`lightingForFrame`) |
| F2 | `GLFW_KEY_F2` (stała `MENU_CAMERA_KEY`, od M9, części 1) | `wasKeyPressed` | `NightMazeApp::updateMenuCameraSwitch`, wołana na początku `onRender` | włącza i wyłącza tryb kamery menu: gra pokazuje samą siebie, a HUD, minimapa i panele są schowane. Nie działa, gdy ImGui edytuje pole tekstowe (klawiatura zablokowana). W trakcie trybu klawisze R, N, F, M, E i mysz rundy są ignorowane ([`../game/menu-camera.md`](../game/menu-camera.md)) |
| E | `GLFW_KEY_E` (stała `INTERACT_KEY`, od M8, części 2) | `wasKeyPressed` | `NightMazeApp::handleInteraction`, wołana z `onRender` | używa tego, na co wskazuje promień: pociąga dźwignię, czyta kartkę albo zamyka kartę. Działa przy przechwyconym i przy wolnym kursorze. Lewy przycisk myszy robi to samo (reguły niżej) |
| W, S, A, D | `GLFW_KEY_W`, `GLFW_KEY_S`, `GLFW_KEY_A`, `GLFW_KEY_D` | `isKeyDown` | `NightMazeApp::onUpdate`, tamże | pola `forward`, `backward`, `left`, `right` struktury `PlayerInput`, tylko przy przechwyconym kursorze |
| spacja | `GLFW_KEY_SPACE` | `isKeyDown` | `NightMazeApp::onUpdate` | pole `up`: w górę, używane tylko w trybie noclip |
| lewy Shift | `GLFW_KEY_LEFT_SHIFT` | `isKeyDown`, dwa razy | `NightMazeApp::onUpdate` | pola `down` (noclip: w dół) i `sprint` (chodzenie: bieg). Gracz czyta to pole, które należy do jego trybu |

Wszystkie pytania przechodzą przez `Input`, więc wszystkie podlegają blokadzie. Pięć pierwszych (E to szósty) to zbocza i stoi w kodzie wykonywanym raz na klatkę. Klawisze ruchu to stan ciągły, czytany w każdym kroku symulacji.

Klawisze ruchu nie trafiają do gracza wprost. `onUpdate` wpisuje je do struktury `game::PlayerInput` (siedem pól `bool`) i dopiero ją przekazuje do `Player::update`:

```cpp
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = input().isKeyDown(GLFW_KEY_W);
        wanted.backward = input().isKeyDown(GLFW_KEY_S);
        wanted.left = input().isKeyDown(GLFW_KEY_A);
        wanted.right = input().isKeyDown(GLFW_KEY_D);
        wanted.up = input().isKeyDown(GLFW_KEY_SPACE);
        // Left Shift has one meaning per mode: sprint when walking, down when flying.
        // The player uses the field that belongs to its mode and ignores the other.
        wanted.down = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
        wanted.sprint = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
    }
```

Dwie różne bramki decydują o tym, czy klawisz zadziała:

| Klawisze | Warunek w grze | Blokada klawiatury przez ImGui |
|---|---|---|
| W, S, A, D, spacja, lewy Shift | tylko przy przechwyconym kursorze (`if (input().isCursorCaptured())`). Bez przechwycenia `PlayerInput` zostaje pusty, ale krok gracza i tak się wykonuje | działa (`isKeyDown` zwraca `false`), choć przy przechwyconym kursorze ImGui nie ma myszy, więc nie ma jak uaktywnić widżetu |
| R | żadnego: działa także przy wolnym kursorze i w każdym stanie rundy | działa: podczas edycji pola albo przeciągania suwaka R nie zaczyna rundy od nowa |
| N | żadnego: działa także przy wolnym kursorze | działa: podczas edycji pola w panelu (na przykład ziarna w panelu Maze) N nie przełącza trybu |
| F | żadnego: działa także przy wolnym kursorze | działa tak samo: podczas edycji pola F nie przełącza latarki |

Dlaczego N nie wymaga przechwyconego kursora: tryb noclip ma też pole wyboru w panelu Collision, którego używa się przy wolnym kursorze, więc oba przełączniki mają działać w tym samym stanie programu. Z klawiszem F jest tak samo: jego odpowiednikiem jest pole wyboru `Flashlight on (key F)` w panelu Lights. Klawisz R ma odpowiednik w przycisku `Restart round (key R)` panelu Gameplay, a po wygranej gracz zwykle ma już wolny kursor albo dopiero co skończył biec, więc R ma działać w obu stanach.

**HUD nie bierze wejścia.** Pasek na górze ekranu i karta `You escaped` to okna ImGui, ale z flagą `ImGuiWindowFlags_NoInputs` (i `NoNav`, `NoFocusOnAppearing`): mysz przechodzi przez nie do sceny, nie da się ich kliknąć ani uaktywnić, więc same nie ustawiają ani `WantCaptureMouse`, ani `WantCaptureKeyboard`. Napis `R: play again` na karcie jest tylko podpowiedzią. Naciśnięcie R czyta gra, drogą z tabeli wyżej. Kliknięcie w kartę przy wolnym kursorze powinno więc trafić w scenę i przechwycić kursor: tak wynika z flag, ręcznie nikt tego jeszcze nie sprawdził ([`../../libraries/imgui.md`](../../libraries/imgui.md)).

Klawisza R, klawisza N, klawisza F i ruchu prawdziwymi klawiszami nikt jeszcze ręcznie nie sprawdził (lista kontrolna w [`../../guides/build-windows.md`](../../guides/build-windows.md)). Logikę gracza sprawdzają testy jednostkowe, które ustawiają pola `PlayerInput` bez klawiatury ([`../game/player.md`](../game/player.md), sekcja 5).

Obsługa Escape w `Application::run`:

```cpp
// What Escape means is up to the program: a game opens its pause menu.
if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
    onEscapePressed();
}
```

**Stan z 2026-10-06 (M9, część 2).** Powyżej jest dzisiejszy kod z `src/core/Application.cpp`. Domyślne ciało `Application::onEscapePressed` zamyka okno, ale `game::NightMazeApp` je nadpisuje: Escape pauzuje grę, wznawia ją albo wraca z ekranu wyniku do menu głównego i nigdy nie zamyka programu (wyjście to przycisk `Quit` w menu głównym, [`../game/game-states.md`](../game/game-states.md)). Kursor nie jest już zwalniany przez Escape: przechwytuje go `NightMazeApp::showScreen`, gdy ekran staje się grą, i oddaje, gdy staje się menu. Zdania niżej o dwóch naciśnięciach Escape (pierwsze zwalnia kursor, drugie zamyka program) i wiersze tabel o Escape opisują stan sprzed tej części. Prawdziwe pozostaje to, że przy zablokowanej klawiaturze (aktywny widżet ImGui albo pole tekstowe menu) `wasKeyPressed` zwraca `false` i `onEscapePressed` nie jest wołane.

Jedno naciśnięcie Escape robi dokładnie jedną z dwóch rzeczy. Przy przechwyconym kursorze użytkownik nie widzi kursora i nie może kliknąć w panel ani w krzyżyk okna, więc pierwszy Escape ma mu oddać mysz, a dopiero drugi zamyka program. Kursor przechwytuje kamera po kliknięciu w scenę (sekcja 5.9). Dopóki nikt w scenę nie kliknął, `isCursorCaptured()` zwraca `false` i Escape po prostu zamyka program, tak jak w M0.

### 5.8 Mysz: przyciski i przesunięcie

**Przyciski** działają jak klawisze. W `update` (sekcja 5.2) kopiuję `m_mouseCurrent` do `m_mousePrevious` i odczytuję wszystkie przyciski od `GLFW_MOUSE_BUTTON_1` do `GLFW_MOUSE_BUTTON_LAST` przez `glfwGetMouseButton`. Pytania mają ten sam kształt co przy klawiszach:

```cpp
bool Input::isMouseButtonDown(int button) const {
    return !m_mouseBlocked && isValidMouseButton(button) && m_mouseCurrent[button];
}

bool Input::wasMouseButtonPressed(int button) const {
    return !m_mouseBlocked && isValidMouseButton(button) && m_mouseCurrent[button] &&
           !m_mousePrevious[button];
}
```

```cpp
bool Input::isValidMouseButton(int button) const {
    return button >= 0 && button < MOUSE_BUTTON_COUNT;
}
```

Kolejność warunków jest ta sama: najpierw blokada (`!m_mouseBlocked`), potem zakres (dla numeru spoza zakresu odpowiedź to `false` i tablica nie jest indeksowana), na końcu stan. Przyciski identyfikuję stałymi GLFW, na przykład `GLFW_MOUSE_BUTTON_LEFT`.

**Kursor.** Trzecia część `update`:

| Linia | Co robi |
|---|---|
| `double cursorX = 0.0; double cursorY = 0.0;` | Zmienne lokalne na nową pozycję. `glfwGetCursorPos` zwraca wynik przez wskaźniki, więc muszą istnieć wcześniej |
| `glfwGetCursorPos(m_window, &cursorX, &cursorY);` | Pozycja we współrzędnych ekranu, y w dół (sekcja 2.5) |
| `if (m_skipNextMouseDelta) { ... }` | Nie ma wiarygodnej poprzedniej pozycji, więc przesunięcie w tej klatce to zero. Flaga gaśnie i następna klatka liczy już normalnie |
| `m_mouseDeltaX = cursorX - m_cursorX;` (i to samo dla y) | Przesunięcie: nowa pozycja minus pozycja zapamiętana przez poprzedni `update` |
| `m_cursorX = cursorX; m_cursorY = cursorY;` | Zapamiętuję nową pozycję jako "poprzednią" dla następnej klatki. Wykonuje się w obu gałęziach |

Pole `m_skipNextMouseDelta` rozwiązuje problem "pierwszej myszy" (sekcja 2.4) w dwóch sytuacjach jednym mechanizmem:

```cpp
// True when the next update has no valid previous cursor position to compare with:
// before the first update and after the cursor mode has changed.
bool m_skipNextMouseDelta = true;
```

Wartość początkowa `true` obsługuje pierwszy `update` po starcie programu (`m_cursorX` i `m_cursorY` są wtedy zerami, a kursor może być gdziekolwiek). Drugą sytuacją jest zmiana trybu kursora (sekcja 5.9).

Odczyt przesunięcia:

```cpp
double Input::mouseDeltaX() const {
    return m_mouseBlocked ? 0.0 : m_mouseDeltaX;
}

double Input::mouseDeltaY() const {
    return m_mouseBlocked ? 0.0 : m_mouseDeltaY;
}
```

Operator `? :` zwraca zero przy zablokowanej myszy, a inaczej zapamiętane przesunięcie. Typ to `double`, bo taki typ podaje GLFW (pozycja kursora może mieć część ułamkową). Dodatnie `mouseDeltaX` to ruch w prawo, dodatnie `mouseDeltaY` to ruch **w dół**.

`Input` nie ma obsługi kółka myszy: nic w projekcie jej nie potrzebuje. Pozycję kursora ma od M8, części 2 (następna podsekcja).

#### 5.8.1 Pozycja kursora: `cursorPosition` i `CursorPosition` (M8, część 2)

```cpp
struct CursorPosition {
    bool valid = false;   // false while the mouse is blocked
    double x = 0.0;
    double y = 0.0;
};

CursorPosition Input::cursorPosition() const {
    if (m_mouseBlocked) {
        return {};
    }
    return {.valid = true, .x = m_cursorX, .y = m_cursorY};
}
```

| Element | Znaczenie |
|---|---|
| `m_cursorX`, `m_cursorY` | pozycja zapamiętana przez `update` w tej klatce (te same pola, z których liczy się przesunięcie). Funkcja więc nie woła GLFW: oddaje to, co `update` odczytał raz na klatkę |
| `valid` równe `false` | mysz jest zablokowana dla gry, czyli kursor jest nad panelem ImGui (sekcja 5.10). Ta sama zasada co dla przycisków: kursor nad panelem nie wskazuje niczego w scenie. `x` i `y` nie mają wtedy znaczenia |
| jednostki | współrzędne ekranu, jak w sekcji 2.5: jednostki rozmiaru **okna**, początek w lewym górnym rogu, y w dół |
| przechwycony kursor | pozycja jest wirtualna i bez granic, nie mówi nic o miejscu w oknie (sekcja 2.6). Gra w takim trybie w ogóle jej nie czyta: promień idzie przez środek obrazu |

**Z czym ją łączyć.** Komentarz w nagłówku mówi to wprost: z rozmiarem **okna** (`core::Window::windowSize`), nigdy z rozmiarem framebuffera. Na ekranie Retina oba się różnią (sekcja 2.5), więc pozycja podzielona przez rozmiar framebuffera wskazywałaby złe miejsce. `NightMazeApp::pickForFrame` dzieli ją przez rozmiar okna.

**Czego nie sprawdzono.** Na macOS (Retina) pozycja kursora i rozmiar okna nie były uruchamiane. Na Windowsie, przy skali ekranu 1, okno i framebuffer mają ten sam rozmiar, więc błąd w doborze jednostek nie byłby tam widoczny: to ryzyko na liście macOS.

### 5.12 Klawisz E i reguły kliknięcia w grze (M8, część 2)

`NightMazeApp` czyta nowy klawisz `INTERACT_KEY = GLFW_KEY_E` (`wasKeyPressed`, raz na klatkę w `onRender`, w `handleInteraction`) i lewy przycisk myszy (`wasMouseButtonPressed`). Reguły, z kodu:

| Sytuacja | Co się dzieje |
|---|---|
| E albo kliknięcie, a wynik wskazywania ma działanie (pociągnięcie dźwigni, przeczytanie kartki, zamknięcie karty) | gra to robi. Dotyczy kursora przechwyconego **i** wolnego: E działa na to, w co trafia promień, także przy wolnym kursorze |
| kliknięcie przy **wolnym** kursorze, a nie ma czego użyć | przechwytuje kursor (`setCursorCaptured(true)`), jak przed M8 |
| kliknięcie przy wolnym kursorze **na** dźwigni albo kartce | używa jej i **nie** przechwytuje kursora |
| karta kartki jest otwarta | działanie to zawsze `CloseNote`, także gdy nie ma promienia, więc kliknięcie zamyka kartę i nie przechwytuje kursora |
| kursor nad panelem ImGui (mysz zablokowana) | nie ma promienia, kliknięcie nie dociera do gry (blokada, sekcja 5.10) |
| kursor poza oknem albo okno o rozmiarze 0 | nie ma promienia |

Blok obrotu kamery myszą stoi teraz **przed** zbudowaniem macierzy widoku, żeby obraz i promień tej klatki używały tych samych kątów (macierze widoku i rzutowania są liczone wcześniej niż dotąd). Ta zmiana nie dotyczy `Input`. Szczegóły: [`../scene/picking.md`](../scene/picking.md) i [`../game/interactables.md`](../game/interactables.md).

### 5.9 Przechwycenie kursora: `setCursorCaptured`

```cpp
void Input::setCursorCaptured(bool captured) {
    if (captured == m_cursorCaptured) {
        return;
    }
    m_cursorCaptured = captured;

    // GLFW_CURSOR_DISABLED hides the cursor and gives unlimited virtual movement,
    // GLFW_CURSOR_NORMAL is the ordinary visible cursor.
    glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    // Raw motion skips the system's pointer acceleration, which suits mouse look.
    // Not every platform has it, and it only has an effect while the cursor is disabled.
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }

    // Changing the cursor mode can make the reported position jump, so the next update
    // must not turn that jump into mouse movement.
    m_skipNextMouseDelta = true;
}
```

| Fragment | Dlaczego tak |
|---|---|
| `if (captured == m_cursorCaptured) { return; }` | Wywołanie z bieżącym stanem nic nie robi. Bez tego każde powtórzone `setCursorCaptured(true)` zerowałoby przesunięcie w następnej klatce i kamera by się zacinała |
| `m_cursorCaptured = captured;` | Stan trzymam we własnym polu `bool`. `isCursorCaptured()` zwraca to pole i nie pyta GLFW przy każdym wywołaniu |
| `glfwSetInputMode(..., GLFW_CURSOR, ...)` | Przechwycenie to tryb `GLFW_CURSOR_DISABLED`, zwolnienie to `GLFW_CURSOR_NORMAL` (sekcja 2.6) |
| `if (glfwRawMouseMotionSupported() == GLFW_TRUE)` | Surowy ruch włączam tylko tam, gdzie istnieje (sekcja 2.7). Na macOS z GLFW 3.4 warunek jest fałszywy i ta linia się nie wykonuje |
| `glfwSetInputMode(..., GLFW_RAW_MOUSE_MOTION, ...)` | Razem z przechwyceniem włączam surowy ruch, razem ze zwolnieniem wyłączam |
| `m_skipNextMouseDelta = true;` | Po zmianie trybu pozycja kursora może odskoczyć (sekcja 2.6), więc następny `update` zgłosi zerowe przesunięcie zamiast jednego wielkiego szarpnięcia |

```cpp
bool isCursorCaptured() const { return m_cursorCaptured; }
```

Kto woła te funkcje:

| Wywołanie | Gdzie | Kiedy |
|---|---|---|
| `setCursorCaptured(true)` | `NightMazeApp::onRender` | kursor nie jest przechwycony i `wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)` zwróciło prawdę, czyli po kliknięciu w scenę (kliknięcie w panel blokuje sekcja 5.10) |
| `setCursorCaptured(false)` | `Application::run` | po Escape, gdy kursor jest przechwycony (sekcja 5.7) |
| `isCursorCaptured()` | `Application::run`, `NightMazeApp::onRender` i `onUpdate`, `DebugNightMazeApp::onRender` w `main.cpp` | wybór gałęzi Escape, włączenie obrotu kamery i klawiszy ruchu gracza, wyłączenie myszy w ImGui (sekcja 5.11) |

Kod kamery w `NightMazeApp::onRender`:

```cpp
// Mouse look. It runs here, once per frame, and not in onUpdate: a click and a mouse
// delta describe one frame, and onUpdate runs zero or more times per frame.
if (!input().isCursorCaptured()) {
    // A click on a debug panel does not arrive here: main.cpp blocks the mouse for
    // the game while the debug UI is using it.
    if (input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        input().setCursorCaptured(true);
    }
} else {
    // Mouse movement to the right is positive and positive yaw turns right, so x is
    // used as it is. Screen y grows downwards while pitch grows upwards, hence the
    // minus sign: moving the mouse up (negative y) looks up.
    const float yawDelta = static_cast<float>(input().mouseDeltaX()) * m_mouseSensitivity;
    const float pitchDelta = -static_cast<float>(input().mouseDeltaY()) * m_mouseSensitivity;
    m_camera.rotate(yawDelta, pitchDelta);
}
```

To jedyne miejsce, w którym gra czyta przyciski myszy i jej przesunięcie. Obie gałęzie stoją w `onRender`, bo zbocze przycisku i przesunięcie to dane jednej klatki (sekcja 2.8). Kod linia po linii: [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 5.

### 5.10 Blokada myszy: gra a panel ImGui

Mechanizm jest kopią blokady klawiatury z sekcji 5.6, z tymi samymi trzema kawałkami w trzech warstwach.

W `core/`:

```cpp
void setMouseBlocked(bool blocked) { m_mouseBlocked = blocked; }
```

```cpp
bool m_mouseBlocked = false;
```

Dopóki flaga jest ustawiona, `isMouseButtonDown` i `wasMouseButtonPressed` zwracają `false` dla każdego przycisku, a `mouseDeltaX` i `mouseDeltaY` zwracają 0.

W `debug/`:

```cpp
bool DebugUI::wantsMouse() const {
    const ImGuiIO& io = ImGui::GetIO();
    // With the mouse switched off ImGui still sets WantCaptureMouse while a button is
    // held down and the hidden cursor is at the position of a panel. Nothing in the panel
    // reacts, but the caller would block the mouse for the game, so the answer is no.
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouse) != 0) {
        return false;
    }
    return io.WantCaptureMouse;
}
```

Funkcja zwraca pole ImGui, z jednym wyjątkiem: gdy mysz ImGui jest wyłączona (sekcja 5.11), odpowiedź zawsze brzmi "nie".

W `main.cpp`, zaraz po blokadzie klawiatury:

```cpp
input().setMouseBlocked(m_debugUI.wantsMouse());
```

**Kiedy `WantCaptureMouse` jest prawdziwe.** Gdy kursor jest nad oknem ImGui (panelem) albo gdy trwa przeciąganie rozpoczęte na widżecie, nawet jeśli kursor wyjechał już poza panel. Przezroczysty środek obszaru dokowania (`PassthruCentralNode`) nie liczy się jako okno pod kursorem, więc nad sceną flaga jest fałszywa i gra widzi mysz. To inny warunek niż przy klawiaturze: tam liczy się aktywny widżet, tu wystarczy samo najechanie na panel.

**Te same trzy własności co przy klawiaturze:**

1. `core/` dostaje neutralną flagę i nie wie, kto ją ustawił ani dlaczego.
2. Blokada działa z opóźnieniem jednej klatki: `setMouseBlocked` stoi na końcu `onRender`, a ImGui liczy `WantCaptureMouse` w `NewFrame` na początku `draw`.
3. `update` odświeża stan przycisków i pozycję kursora także przy blokadzie. Dzięki temu po zdjęciu blokady przycisk trzymany od dawna nie tworzy fałszywego zbocza, a przesunięcie to zawsze ruch z ostatniej klatki, nie cała droga, którą kursor przebył nad panelem.

Skutek widać w działającym programie: kliknięcie w panel nie przechwytuje kursora, bo `wasMouseButtonPressed` w kodzie kamery zwraca wtedy `false`. Przeciąganie suwaka nie obraca kamery z dwóch powodów naraz: mysz jest zablokowana, a kursor nie jest przechwycony.

### 5.11 Przechwycony kursor a ImGui

Blokada z sekcji 5.10 chroni grę przed myszą używaną przez panel. Przy przechwyconym kursorze potrzebna jest ochrona w drugą stronę: panele nie mogą reagować na mysz, która steruje kamerą.

**Problem.** Kursor w trybie `GLFW_CURSOR_DISABLED` jest niewidoczny, ale nadal ma pozycję (wirtualną), która zmienia się z każdym ruchem myszy. Backend GLFW biblioteki ImGui przekazuje tę pozycję do ImGui tak samo jak zwykłą: w trybie `DISABLED` pomija tylko zmianę kształtu kursora. Niewidoczny kursor wędrowałby więc po panelach. Skutki bez dodatkowego kodu:

1. najechanie na panel ustawia `WantCaptureMouse`, `main.cpp` blokuje grze mysz, `mouseDeltaX` zwraca 0 i obrót kamery **zamiera** w losowych chwilach,
2. kliknięcie podczas sterowania kamerą trafia w przycisk albo suwak, którego nie widać pod kursorem, którego też nie widać.

**Rozwiązanie.** `debug::DebugUI` ma funkcję `setMouseEnabled(bool)`, która ustawia albo zdejmuje w ImGui flagę `ImGuiConfigFlags_NoMouse` ("ignoruj mysz"). `main.cpp` woła ją co klatkę, tuż przed `draw`:

```cpp
m_debugUI.setMouseEnabled(!input().isCursorCaptured());
```

Podział ról jest ten sam co przy blokadach: `core::Input` wie tylko, czy kursor jest przechwycony, `debug::DebugUI` umie tylko włączyć i wyłączyć mysz w ImGui, a regułę "przechwycony kursor znaczy, że mysz należy do gry" zna jedynie `main.cpp`. `core/` nadal nie zna ImGui.

Kto ma mysz w każdym ze stanów:

| Stan | Kursor | Gra (`core::Input`) | Panele ImGui |
|---|---|---|---|
| kursor wolny, nad sceną | widoczny | widzi kliknięcia i ruch | nie reagują (kursor nie jest nad nimi) |
| kursor wolny, nad panelem albo trwa przeciąganie widżetu | widoczny | mysz zablokowana (sekcja 5.10) | reagują |
| kursor przechwycony | niewidoczny | widzi przesunięcie: kamera się obraca | mysz wyłączona flagą `NoMouse` |

Co dokładnie robi flaga, w której klatce zaczyna działać i dlaczego `wantsMouse()` ma przy niej dodatkowy warunek, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.6. Tam też jest prześledzona cała kolejność zdarzeń w klatce kliknięcia i w klatce z Escape: nie ma w niej klatki, w której mysz miałyby oba systemy albo żaden.

## 6. Panel ImGui

`Input` nie ma własnego elementu w panelu. Panel Renderer jest za to narzędziem do sprawdzenia blokady klawiatury:

| Co zrobić | Co obserwować |
|---|---|
| Nacisnąć `~`, gdy żaden widżet nie jest aktywny | Panele znikają i wracają, HUD gry zostaje na ekranie: `wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)` działa |
| Kliknąć z wciśniętym Ctrl w jedną ze składowych `Clear color` (pole przechodzi w tryb wpisywania) i nacisnąć Escape | ImGui anuluje edycję, program **nie** zamyka się |
| W tym samym trybie wpisywania nacisnąć `~` | Panele nie znikają |
| Przytrzymać mysz na składowej `Clear color` (przeciąganie wartości) i nacisnąć Escape | Program nie zamyka się: aktywny jest widżet, choć to nie pole tekstowe |
| Nacisnąć Escape, gdy żaden widżet nie jest aktywny | Program zamyka się |

Mysz sprawdza się na kamerze, z otwartym panelem Camera ([`../scene/camera-controls.md`](../scene/camera-controls.md), sekcja 6). Od M4 ten panel startuje zwinięty do paska tytułu przy górnej krawędzi okna (od M5 obok tak samo zwiniętego panelu Gameplay), więc przed próbą trzeba go rozwinąć strzałką w pasku:

| Co zrobić | Co obserwować |
|---|---|
| Kliknąć lewym przyciskiem w scenę (nie na dźwigni ani kartce) | Kursor znika: `wasMouseButtonPressed` i `setCursorCaptured(true)`. Kliknięcie na dźwigni albo kartce używa jej i kursora nie przechwytuje (od M8, części 2) |
| Nacisnąć E, patrząc na dźwignię w zasięgu | Linia `Levers: 1 pulled of N` w panelu Gameplay, `wasKeyPressed` w `onRender`. Nikt z właścicieli tego jeszcze nie sprawdził |
| Poruszać myszą przy przechwyconym kursorze | Wartości `Yaw` i `Pitch` w panelu Camera zmieniają się: `mouseDeltaX` i `mouseDeltaY` |
| Kręcić myszą długo w jedną stronę | `Yaw` rośnie bez końca (zawijając się przez 360): pozycja wirtualna nie zatrzymuje się na krawędzi ekranu |
| Przytrzymać W przy przechwyconym kursorze | `Player feet` i `Eye` w panelu się zmieniają, aż gracz dojdzie do ściany: `isKeyDown` |
| Nacisnąć N (kursor przechwycony albo wolny) | Linia `Mode` w panelu Camera zmienia się między `walking` a `noclip (free flight)`: `wasKeyPressed` w `onRender` |
| Nacisnąć R (kursor przechwycony albo wolny) | Gracz wraca na start, licznik kryształów i czas w HUD wracają do zera, bateria do 100%, linia `Round` w panelu Gameplay pokazuje stan po restarcie: `wasKeyPressed` w `onRender` |
| Nacisnąć F (kursor przechwycony albo wolny) | Plama światła latarki w środku ekranu gaśnie albo wraca, a pole `Flashlight on (key F)` w panelu Lights zmienia stan: `wasKeyPressed` w `onRender` |
| Nacisnąć Escape | Kursor wraca w miejsce, w którym zniknął, program działa dalej. Kamera nie szarpie (`m_skipNextMouseDelta`) |
| Nacisnąć Escape drugi raz | Program się zamyka |
| Kliknąć w panel (na przykład w jego pasek tytułu) | Kursor **nie** znika: blokada myszy |
| Przeciągnąć suwak `FOV`, wyjeżdżając kursorem nad scenę | Kamera się nie obraca, kursor nie zostaje przechwycony |
| Przy przechwyconym kursorze poruszać myszą tak, żeby niewidoczny kursor przeszedł nad panelami, i klikać | Obrót się nie zacina, żaden panel nie reaguje: `setMouseEnabled(false)` |

Surowe wartości można podejrzeć kodem z ćwiczenia 4.

## 7. Pułapki

1. **`wasKeyPressed` w `onUpdate`.** Gubi naciśnięcia albo wykonuje akcję kilka razy (sekcja 5.5).
2. **Pomylenie `isKeyDown` z `wasKeyPressed`.** Przełącznik oparty na `isKeyDown` przełącza się w każdej klatce, dopóki trzymam klawisz. To samo dotyczy pary `isMouseButtonDown` i `wasMouseButtonPressed`.
3. **Bardzo krótkie naciśnięcie.** `Input` odczytuje stan raz na klatkę, więc klawisz albo przycisk wciśnięty i puszczony między dwoma odczytami nie zostanie zauważony. Przy 60 FPS to rzadkie, ale przy długiej klatce możliwe.
4. **Przesunięcie myszy w `onUpdate`.** `mouseDeltaX` i `mouseDeltaY` opisują jedną klatkę. Użyte w `onUpdate` przepadają w klatce bez kroku i liczą się kilka razy w klatce z kilkoma krokami, więc czułość myszy zależałaby od FPS (sekcja 2.8). Czytam je tylko w `onRender`.
5. **"Escape nie działa".** Jeśli Escape albo `~` nie reaguje, najpewniej aktywny jest widżet ImGui (trwa edycja albo przeciąganie). To zamierzone zachowanie blokady, nie błąd. Wystarczy zakończyć edycję (Enter, Escape albo kliknięcie poza polem).
6. **Jedna flaga, jeden właściciel.** `setKeyboardBlocked` i `setMouseBlocked` nadpisują poprzednią wartość. Gdy pojawi się drugi powód blokady, wołający musi sam połączyć oba warunki w jedną wartość, inaczej ostatnie wywołanie w klatce wygra.
7. **Blokada zatrzymuje też ruch.** Przy zablokowanej klawiaturze `isKeyDown` zwraca `false`, więc gracz idący na W zatrzymałby się na czas edycji pola. Tak ma być. W praktyce nie da się tego dziś wywołać dla klawiszy ruchu: edycja pola wymaga widocznego kursora, a ruch gracza przechwyconego. Da się to natomiast zobaczyć na klawiszach N, F i R, które działają przy wolnym kursorze: podczas wpisywania liczby w panelu N nie przełącza noclip, a R nie zaczyna rundy od nowa (inaczej wpisanie litery w pole tekstowe kasowałoby postęp).
8. **Pytanie o klawisz albo mysz z pominięciem `Input`.** Bezpośrednie `glfwGetKey`, `glfwGetMouseButton` albo `glfwGetCursorPos` w kodzie gry omija blokadę. Wszystkie pytania gry o wejście mają iść przez `input()`.
9. **Zamiana kolejności `pollEvents` i `update`.** `glfwGetKey`, `glfwGetMouseButton` i `glfwGetCursorPos` oddają stan z ostatniego `glfwPollEvents`, więc `update` przed `pollEvents` widziałby wejście z opóźnieniem jednej klatki.
10. **Skok kursora przy zmianie trybu.** Po `glfwSetInputMode(..., GLFW_CURSOR, ...)` pozycja kursora może odskoczyć. Bez `m_skipNextMouseDelta` pierwsza klatka po przechwyceniu albo zwolnieniu dałaby jedno ogromne przesunięcie. To samo przy pierwszym `update` po starcie.
11. **Blokada myszy spóźnia się o klatkę.** Tak jak przy klawiaturze, nowa wartość `setMouseBlocked` działa od następnej klatki. Kliknięcie, które trafia w panel w tej samej klatce, w której kursor na niego najechał, gra może jeszcze zobaczyć: kamera przechwyciłaby wtedy kursor mimo kliknięcia w panel. Wymaga to najechania i kliknięcia w ciągu jednej klatki (kilkunastu milisekund), a skutek cofa jeden Escape.
12. **Retina: mysz jest we współrzędnych ekranu.** (Od M8, części 2 dotyczy to też `cursorPosition`: dziel ją przez `windowSize()`, nie przez `framebufferSize()`.) Przesunięcie jest liczone w jednostkach rozmiaru okna, nie w pikselach framebuffera. Na ekranie 2x ruch przez całe okno 1280 x 720 daje 1280, a nie 2560. Mieszanie tych jednostek z `framebufferSize()` daje wynik dwukrotnie za mały albo za duży.
13. **Oś y myszy rośnie w dół.** Ruch myszy do góry daje ujemne `mouseDeltaY`. Kod kamery odwraca znak (`-static_cast<float>(input().mouseDeltaY())`), żeby ruch do góry podnosił wzrok.
14. **Własny callback myszy.** `glfwSetCursorPosCallback` albo `glfwSetMouseButtonCallback` ustawione po utworzeniu `DebugUI` podmieniają callbacki backendu ImGui i panele przestają reagować na mysz. Dlatego mysz jest odpytywana.
15. **Przechwycony kursor a panele ImGui.** Backend GLFW biblioteki ImGui w trybie `GLFW_CURSOR_DISABLED` nie zmienia kształtu kursora, ale pozycję kursora nadal przekazuje do ImGui (jest to wtedy pozycja wirtualna). Niewidoczny kursor mógłby więc "najechać" na panel i ustawić `WantCaptureMouse`, blokada myszy wyzerowałaby przesunięcie i kamera przestałaby się obracać, a kliknięcie trafiłoby w niewidoczny widżet. Rozwiązanie: `main.cpp` woła przed `draw` `m_debugUI.setMouseEnabled(!input().isCursorCaptured());`, czyli na czas przechwycenia wyłącza mysz w ImGui flagą `ImGuiConfigFlags_NoMouse` (sekcja 5.11). Kto przenosi ten kod do innego programu i zapomni o tej linii, dostanie kamerę, która zacina się, gdy niewidoczny kursor przechodzi nad panelem.

## 8. Ćwiczenia

1. **Blokada klawiatury w działaniu.** Uruchom program i wykonaj kolejno wszystkie wiersze tabeli z sekcji 6. Potem w `main.cpp` zakomentuj linię `input().setKeyboardBlocked(m_debugUI.wantsKeyboard());`, zbuduj i powtórz próbę z Escape podczas wpisywania wartości `Clear color`. Opisz różnicę i przywróć linię.
2. **Zamrożone tablice.** Dopisz tymczasowo na początku `Input::update()` warunek `if (m_keyboardBlocked) { return; }`. Zbuduj, wejdź w tryb wpisywania wartości `Clear color` (Ctrl i kliknięcie) i anuluj edycję klawiszem Escape, przytrzymując go przez chwilę. Sprawdź, czy program się zamyka, i wyjaśnij wynik tabelą z sekcji 5.6. Usuń warunek.
3. **`wasKeyPressed` w `onUpdate`.** W `NightMazeApp.cpp` dołącz `<GLFW/glfw3.h>` i `"core/Log.hpp"`, a w `NightMazeApp::onUpdate` dopisz `if (input().wasKeyPressed(GLFW_KEY_SPACE)) { core::logInfo("space"); }`. Naciskaj spację i licz linie w konsoli przypadające na jedno naciśnięcie. Powtórz przy wyłączonym vsync (ćwiczenie 1 w [`window-context.md`](window-context.md)). Wyjaśnij wyniki tabelą z sekcji 5.5 i wycofaj zmiany.
4. **Podgląd myszy.** Na początku `NightMazeApp::onRender` dopisz tymczasowo (z nagłówkami `<string>` i `"core/Log.hpp"`, `<GLFW/glfw3.h>` jest już dołączony): `if (input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) { core::logInfo("click"); }` oraz wypisywanie `std::to_string(input().mouseDeltaX())`, gdy wartość jest różna od zera. Sprawdź trzy rzeczy: ruch w prawo daje wartości dodatnie, kliknięcie w scenę wypisuje `click`, a kliknięcie w panel Renderer i ruch kursora nad nim nie wypisują nic. Wycofaj zmiany.
5. **Przechwycenie i Escape.** Kliknij w scenę: kursor znika. Naciśnij Escape: kursor wraca, program działa. Naciśnij Escape drugi raz: program się zamyka. Wskaż w `NightMazeApp::onRender` i w `Application::run` linie, które za to odpowiadają. Potem zamień tymczasowo w `Application::run` kolejność gałęzi tak, żeby Escape zawsze zamykał program, i powiedz, czego użytkownik z przechwyconym kursorem nie może wtedy zrobić. Wycofaj zmianę.
6. **Skok kursora.** Z kodem z ćwiczenia 4 zakomentuj tymczasowo linię `m_skipNextMouseDelta = true;` w `setCursorCaptured`. Przechwyć kursor, pokręć myszą w jedną stronę, naciśnij Escape i odczytaj z konsoli przesunięcie w klatce po zwolnieniu. Co dzieje się z kamerą w pierwszej klatce po ponownym kliknięciu w scenę? Wyjaśnij, skąd wzięło się przesunięcie, i przywróć linię.
7. **Bez `setMouseEnabled`.** Zakomentuj w `main.cpp` linię `m_debugUI.setMouseEnabled(!input().isCursorCaptured());` i zbuduj. Zadokuj panel Camera przy prawej krawędzi, kliknij w scenę i kręć myszą powoli w prawo. Zapisz, w którym momencie obrót się zatrzymuje i co dzieje się po kliknięciu. Wyjaśnij to pułapką 15 i przywróć linię.

## 9. Pytania kontrolne

1. **Dlaczego `KEY_COUNT = 348 + 1` i co sprawdza `static_assert`?**
   Tablica jest indeksowana kodem klawisza, największy to `GLFW_KEY_LAST` = 348, więc potrzeba 349 elementów. Liczba jest wpisana wprost, żeby nagłówek nie dołączał GLFW, a `static_assert` w `Input.cpp` w czasie kompilacji pilnuje zgodności z GLFW. Tak samo działa `MOUSE_BUTTON_COUNT = 7 + 1` dla przycisków myszy.

2. **Czym różni się `isKeyDown` od `wasKeyPressed` i dlaczego drugiego nie wolno wołać w `onUpdate`?**
   Pierwsze to stan ciągły, drugie to zbocze między dwiema klatkami. `onUpdate` wykonuje się od zera do wielu razy na klatkę, więc zbocze zostałoby zgubione albo obsłużone kilka razy.

3. **Dlaczego `m_input.update()` stoi po `m_window.pollEvents()`?**
   `glfwGetKey`, `glfwGetMouseButton` i `glfwGetCursorPos` nie pytają systemu, tylko zwracają stan zapamiętany przez GLFW podczas ostatniego `glfwPollEvents`. Najpierw trzeba więc odebrać zdarzenia.

4. **Co się dzieje, gdy wpisuję wartość w panelu ImGui i naciskam Escape? Prześledź drogę przez kod.**
   Aktywny widżet sprawia, że `ImGui::GetIO().WantCaptureKeyboard` jest `true`. `DebugUI::wantsKeyboard()` zwraca tę wartość, a `DebugNightMazeApp::onRender` przekazuje ją do `input().setKeyboardBlocked(...)`. Od następnej klatki `wasKeyPressed(GLFW_KEY_ESCAPE)` w `Application::run` zwraca `false`, więc program się nie zamyka. Escape trafia tylko do ImGui, które anuluje edycję.

5. **Dlaczego `core::Input` ma flagi `m_keyboardBlocked` i `m_mouseBlocked`, zamiast samemu sprawdzać `ImGui::GetIO()`?**
   `core/` nie może zależeć od ImGui ani od `debug/` (reguła warstw, biblioteka `engine` linkuje tylko `glad`, `glfw` i nagłówki GLM). Flagi są neutralne: mówią tylko, że ktoś inny ma klawiaturę albo mysz. Kto i dlaczego, decyduje `main.cpp`, jedyny plik znający obie warstwy.

6. **Blokada zaczyna działać z opóźnieniem. Skąd się ono bierze i dlaczego nie przeszkadza?**
   `setKeyboardBlocked` jest wołane na końcu `onRender`, po tym, jak pytania o klawisze w tej klatce już padły, więc działa od następnej klatki. Dodatkowo ImGui wylicza `WantCaptureKeyboard` w `NewFrame` na podstawie widżetu aktywnego po poprzedniej klatce. Razem to najwyżej dwie klatki od kliknięcia, czyli ułamek sekundy krótszy niż czas potrzebny na naciśnięcie klawisza po kliknięciu.

7. **Dlaczego po zdjęciu blokady nie pojawia się fałszywe "właśnie wciśnięty"?**
   `Input::update()` odświeża `m_current` i `m_previous` w każdej klatce, także przy blokadzie. Klawisz wciśnięty w czasie blokady ma więc po jej zdjęciu `m_previous == true` i nie tworzy zbocza. Blokada zmienia tylko odpowiedzi `isKeyDown` i `wasKeyPressed`, nie pomiar. Przyciski myszy i pozycja kursora działają tak samo.

8. **Kiedy `WantCaptureKeyboard` jest prawdziwe, a kiedy `WantCaptureMouse`?**
   Pierwsze: gdy aktywny jest dowolny widżet ImGui (edycja pola, przeciąganie suwaka lub wartości, trzymany przycisk, przesuwane okno) albo otwarte jest okno modalne. Drugie: gdy kursor jest nad panelem albo trwa przeciąganie rozpoczęte na widżecie. Przezroczysty środek obszaru dokowania się nie liczy.

9. **Dlaczego mysz jest odpytywana, a nie obsługiwana callbackiem?**
   GLFW trzyma jeden callback danego rodzaju na okno, a callbacki pozycji kursora i przycisków zajmuje backend ImGui. Własny callback ustawiony później by go podmienił. Odpytywanie (`glfwGetCursorPos`, `glfwGetMouseButton`) niczego nie podmienia i pasuje do pętli, która i tak działa co klatkę.

10. **Jak powstaje `mouseDeltaX` i w jakich jednostkach jest?**
    `update` czyta pozycję kursora przez `glfwGetCursorPos` i odejmuje od niej pozycję zapamiętaną w poprzedniej klatce. Jednostką są współrzędne ekranu (te same co rozmiar okna), nie piksele framebuffera. Dodatnie x to ruch w prawo, dodatnie y to ruch w dół.

11. **Czym jest problem "pierwszej myszy" i jak jest rozwiązany?**
    Przesunięcie wymaga dwóch pomiarów. Przy pierwszym odczycie i po zmianie trybu kursora poprzednia pozycja jest nieważna, więc różnica byłaby jednym wielkim skokiem. Pole `m_skipNextMouseDelta` (na starcie `true`, ustawiane też w `setCursorCaptured`) każe następnemu `update` zgłosić zero i tylko zapamiętać pozycję.

12. **Czym różnią się `GLFW_CURSOR_NORMAL`, `GLFW_CURSOR_HIDDEN` i `GLFW_CURSOR_DISABLED`?**
    `NORMAL`: zwykły widoczny kursor. `HIDDEN`: niewidoczny nad oknem, ale dalej ograniczony ekranem. `DISABLED`: niewidoczny, zatrzymany w oknie, z wirtualną pozycją bez ograniczeń, do sterowania kamerą. Używam `NORMAL` i `DISABLED`.

13. **Co to jest surowy ruch myszy i dlaczego przed włączeniem pytam `glfwRawMouseMotionSupported()`?**
    To ruch prosto z urządzenia, bez przyspieszenia i skalowania systemu, więc ten sam ruch ręki daje zawsze ten sam obrót. Nie każda platforma go ma (w GLFW 3.4 macOS nie), a włączenie go tam, gdzie go nie ma, kończy się błędem GLFW. Działa tylko przy kursorze w trybie `DISABLED`.

14. **Dlaczego przesunięcia myszy nie wolno używać w `onUpdate`?**
    To wartość jednej klatki. `onUpdate` wykonuje się od zera do wielu razy na klatkę, więc ruch zostałby zgubiony albo policzony kilka razy i czułość zależałaby od FPS. Czytam je w `onRender`.

15. **Co robi Escape i dlaczego w tej kolejności?**
    Jeśli kursor jest przechwycony, Escape tylko go zwalnia (`setCursorCaptured(false)`). Jeśli nie jest, zamyka program (`requestClose()`). Przy przechwyconym kursorze użytkownik nie ma jak kliknąć w panel ani w okno, więc najpierw odzyskuje mysz. Kursor przechwytuje kamera po kliknięciu w scenę.

16. **Kto i kiedy przechwytuje kursor?**
    `NightMazeApp::onRender`: gdy kursor nie jest przechwycony i `wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)` zwraca prawdę, woła `setCursorCaptured(true)`. Kliknięcie w panel nie dociera, bo przy kursorze nad panelem mysz jest zablokowana. Od następnej klatki ten sam kod czyta `mouseDeltaX` i `mouseDeltaY` i obraca kamerę.

17. **Dlaczego przy przechwyconym kursorze trzeba wyłączyć mysz w ImGui i jak to jest zrobione?**
    Niewidoczny kursor ma wirtualną pozycję, którą backend ImGui nadal przekazuje do biblioteki. Mógłby najechać na panel: ImGui zgłosiłoby `WantCaptureMouse`, blokada wyzerowałaby przesunięcie myszy i kamera by się zacięła, a kliknięcie trafiłoby w niewidoczny widżet. `main.cpp` woła przed `draw` `m_debugUI.setMouseEnabled(!input().isCursorCaptured())`, co ustawia w ImGui flagę `ImGuiConfigFlags_NoMouse`. `core/` nadal nie zna ImGui.

## 10. Źródła

- Dokumentacja GLFW: <https://www.glfw.org/docs/latest/> (przewodnik o wejściu, "Input guide": `glfwGetKey`, `glfwPollEvents`, kody klawiszy, `glfwGetCursorPos`, `glfwGetMouseButton`, tryby kursora, surowy ruch myszy).
- Dear ImGui, repozytorium: <https://github.com/ocornut/imgui> (plik `docs/FAQ.md`, pytanie o to, jak rozpoznać, czy wejście ma trafić do ImGui czy do aplikacji, oraz komentarze przy `WantCaptureKeyboard` i `WantCaptureMouse` w `imgui.h`). Po pierwszej konfiguracji te pliki leżą lokalnie w `build/debug/_deps/imgui-src/`.
- Dokumenty bibliotek w tym repozytorium: [`../../libraries/glfw.md`](../../libraries/glfw.md), [`../../libraries/imgui.md`](../../libraries/imgui.md) (sekcja 3.8).
- Podpięcie nakładki debug: [`../debug-ui.md`](../debug-ui.md) (sekcja 5.6: blokady i `setMouseEnabled`).
- Użytkownik myszy: [`../scene/camera-controls.md`](../scene/camera-controls.md), sekcje 2 i 5. Użytkownik klawiszy ruchu: [`../game/player.md`](../game/player.md), sekcje 2 i 5.
- LearnOpenGL, rozdział "Hello Window" (<https://learnopengl.com/Getting-started/Hello-Window>): obsługa klawisza Escape przez `glfwGetKey`.
- LearnOpenGL, rozdział "Camera" (<https://learnopengl.com/Getting-started/Camera>): sterowanie kamerą myszą, przechwycenie kursora i problem pierwszego odczytu.
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o pierwszym programie i obsłudze zdarzeń).
