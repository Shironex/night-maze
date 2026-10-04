# Moduł core: klawiatura i mysz

Kamień milowy: M0 (klawiatura), M1 (mysz). Temat wykładu: 1 (Pierwszy program OpenGL).
Kod: [`src/core/Input.hpp`](../../../src/core/Input.hpp), [`src/core/Input.cpp`](../../../src/core/Input.cpp), użycie w [`src/core/Application.cpp`](../../../src/core/Application.cpp) i [`src/main.cpp`](../../../src/main.cpp).

Część modułu `core`. Wstęp do całego modułu i diagram warstw są w [`README.md`](README.md). Pozostałe części: [`window-context.md`](window-context.md) (okno i kontekst), [`main-loop.md`](main-loop.md) (pętla i czas), [`gl-check.md`](gl-check.md) (błędy OpenGL). Funkcje wejścia samej biblioteki GLFW opisuje [`../../libraries/glfw.md`](../../libraries/glfw.md).

## 1. Po co to jest

Gra potrzebuje odpowiedzi na dwa różne pytania o klawisz: "czy jest teraz trzymany" (ruch) i "czy został właśnie wciśnięty" (przełącznik, zamknięcie programu). GLFW odpowiada wprost tylko na pierwsze. Klasa `core::Input` raz na klatkę robi migawkę (snapshot) stanu całej klawiatury, pamięta migawkę z poprzedniej klatki i z ich porównania wyprowadza drugie pytanie. Ma też jedną flagę, `setKeyboardBlocked`, którą ktoś z zewnątrz może na chwilę odebrać grze klawiaturę. Dziś robi to `main.cpp`, gdy klawiatury używa panel ImGui: dzięki temu Escape wciśnięty podczas edycji pola w panelu nie zamyka programu.

Z myszą jest tak samo, tylko pytań jest więcej. Przyciski myszy mają te same dwa pytania co klawisze (`isMouseButtonDown`, `wasMouseButtonPressed`). Dochodzi trzecie: "o ile kursor przesunął się od poprzedniej klatki" (`mouseDeltaX`, `mouseDeltaY`), bo właśnie z przesunięcia, a nie z pozycji, liczy się obrót kamery. `Input` potrafi też przechwycić kursor (`setCursorCaptured`): schować go i zdjąć z niego ograniczenie krawędziami ekranu. Mysz ma własną flagę blokady, `setMouseBlocked`, ustawianą przez `main.cpp`, gdy kursor jest nad panelem ImGui.

Stan na dziś: całe API myszy jest gotowe, a blokada myszy i Escape zwalniający kursor już działają, ale żaden kod nie pyta jeszcze o przyciski ani o przesunięcie i nikt nie woła `setCursorCaptured(true)`. Pierwszym użytkownikiem będzie kamera w M1.

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

`Input` udostępnia tylko przesunięcie, bo tylko ono będzie potrzebne. GLFW przy odpytywaniu podaje pozycję (`glfwGetCursorPos`), więc przesunięcie liczę sam: pozycja z tej klatki minus pozycja z poprzedniej. Tak jak przy zboczu, potrzebne są dwa pomiary. Z tego wynika problem "pierwszej myszy" (first mouse): przy pierwszym odczycie nie ma poprzedniej pozycji, z którą można porównać. Gdybym odjął pozycję od zera, dostałbym jedno ogromne przesunięcie i kamera szarpnęłaby w losowym kierunku. Ten sam problem wraca przy każdej zmianie trybu kursora (sekcja 2.6).

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
| [`src/main.cpp`](../../../src/main.cpp) | przełącznik paneli (`GLFW_KEY_GRAVE_ACCENT`) i ustawianie blokady klawiatury i myszy |

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
- `wasKeyPressed`: **zbocze** (edge), "w tej klatce jest wciśnięty, a w poprzedniej nie był". Do akcji jednorazowych (klawisz `~` przełącza panele, Escape zwalnia kursor albo zamyka program).

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

Dlatego obsługa przełącznika paneli stoi w `DebugNightMazeApp::onRender` w `main.cpp` (`if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) { m_debugUI.toggleVisible(); }`), a Escape w `Application::run` przed pętlą kroków: oba miejsca wykonują się dokładnie raz na klatkę. W `onUpdate` wolno używać `isKeyDown`, bo stan ciągły jest taki sam w każdym kroku danej klatki. Gdy w późniejszych kamieniach milowych jednorazowa akcja będzie musiała wpłynąć na symulację (na przykład skok), trzeba ją odczytać raz na klatkę i przekazać do symulacji jako zapamiętane żądanie.

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
m_debugUI.draw(time(), window(), clearColor());

// ImGui now knows whether it is using the keyboard (a text field is being edited
// or a widget is active) and the mouse (the cursor is over a panel or a widget is
// being dragged). Block each device for the game from the next frame on, so typing
// does not trigger Escape, the panel toggle or player movement, and working with
// a panel does not click or look around in the scene.
input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
input().setMouseBlocked(m_debugUI.wantsMouse());
```

Druga z tych linii to blokada myszy, opisana w sekcji 5.10.

**Dlaczego `core` dostaje neutralną flagę, a nie pyta ImGui samo.** `core/` to biblioteka `engine`, która linkuje tylko `glad` i `glfw` i zgodnie z regułą warstw nie zna ani `debug/`, ani ImGui ([`README.md`](README.md), sekcja 3). Gdyby `Input::isKeyDown` wołało `ImGui::GetIO()`, `engine` musiałby linkować ImGui, a każdy program zbudowany na `engine` (na przykład zadanie laboratoryjne bez paneli) ciągnąłby tę bibliotekę za sobą. Flaga `m_keyboardBlocked` mówi tylko "ktoś inny ma teraz klawiaturę". `Input` nie wie kto i dlaczego: decyduje ten, kto woła setter. Dziś jest to `main.cpp` i powodem jest ImGui, ale tym samym setterem mogłoby się posłużyć na przykład menu pauzy. `debug::DebugUI` z kolei nie wie, co wołający zrobi z odpowiedzią `wantsKeyboard()`. Oba końce skleja `main.cpp`, jedyny plik znający obie warstwy.

**Dlaczego `WantCaptureKeyboard` jest prawdziwe nie tylko w polach tekstowych.** ImGui ustawia tę flagę, gdy **jakikolwiek widżet jest aktywny** (trwa edycja pola, przeciągany jest suwak albo wartość, trzymany jest przycisk, przesuwane jest okno panelu) albo gdy otwarte jest okno modalne. Powód: aktywny widżet może sam używać klawiszy. Escape anuluje edycję albo przeciąganie, Tab przechodzi do następnego pola, Ctrl, Shift i Alt zmieniają zachowanie przeciągania. ImGui nie próbuje zgadywać, które klawisze "należą" do widżetu, tylko zgłasza, że na czas aktywności bierze całą klawiaturę. Samo najechanie kursorem na panel flagi nie ustawia (to obszar drugiej flagi, `WantCaptureMouse`). Skutek praktyczny: podczas przeciągania wartości `Clear color` Escape nie zamyka programu, a `~` nie chowa paneli.

**Dlaczego blokada jest spóźniona i dlaczego to nie szkodzi.** Opóźnienie ma dwa źródła:

1. `setKeyboardBlocked` stoi na końcu `onRender`. Pytania o klawisze w tej klatce (Escape w `Application::run`, `~` na początku `onRender`) już padły, więc nowa wartość flagi działa **od następnej klatki**.
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
| `~` (na lewo od `1`) | `GLFW_KEY_GRAVE_ACCENT` | `wasKeyPressed` | `DebugNightMazeApp::onRender` w [`main.cpp`](../../../src/main.cpp) | chowa i pokazuje panele debug |

Oba pytania przechodzą przez `Input`, więc oba podlegają blokadzie. `isKeyDown` nie jest jeszcze nigdzie używane: przyda się w M1 do ruchu kamery.

Obsługa Escape w `Application::run`:

```cpp
// Escape first gives a captured cursor back, and closes the window only when
// the cursor is not captured.
if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
    if (m_input.isCursorCaptured()) {
        m_input.setCursorCaptured(false);
    } else {
        m_window.requestClose();
    }
}
```

Jedno naciśnięcie Escape robi dokładnie jedną z dwóch rzeczy. Przy przechwyconym kursorze użytkownik nie widzi kursora i nie może kliknąć w panel ani w krzyżyk okna, więc pierwszy Escape ma mu oddać mysz, a dopiero drugi zamyka program. Ponieważ dziś nikt nie przechwytuje kursora, `isCursorCaptured()` zawsze zwraca `false` i Escape po prostu zamyka program, tak jak w M0.

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

`Input` celowo nie ma funkcji zwracającej pozycję kursora ani obsługi kółka myszy: nic w projekcie ich nie potrzebuje.

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

Kto woła te funkcje dziś: tylko `Application::run`, które po Escape woła `setCursorCaptured(false)` (sekcja 5.7). `setCursorCaptured(true)` nie jest jeszcze wołane nigdzie, więc kursor jest zawsze w trybie `GLFW_CURSOR_NORMAL`. W M1 kamera będzie przechwytywać kursor po kliknięciu lewym przyciskiem w scenę.

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
    return ImGui::GetIO().WantCaptureMouse;
}
```

W `main.cpp`, zaraz po blokadzie klawiatury:

```cpp
input().setMouseBlocked(m_debugUI.wantsMouse());
```

**Kiedy `WantCaptureMouse` jest prawdziwe.** Gdy kursor jest nad oknem ImGui (panelem) albo gdy trwa przeciąganie rozpoczęte na widżecie, nawet jeśli kursor wyjechał już poza panel. Przezroczysty środek obszaru dokowania (`PassthruCentralNode`) nie liczy się jako okno pod kursorem, więc nad sceną flaga jest fałszywa i gra widzi mysz. To inny warunek niż przy klawiaturze: tam liczy się aktywny widżet, tu wystarczy samo najechanie na panel.

**Te same trzy własności co przy klawiaturze:**

1. `core/` dostaje neutralną flagę i nie wie, kto ją ustawił ani dlaczego.
2. Blokada działa z opóźnieniem jednej klatki: `setMouseBlocked` stoi na końcu `onRender`, a ImGui liczy `WantCaptureMouse` w `NewFrame` na początku `draw`.
3. `update` odświeża stan przycisków i pozycję kursora także przy blokadzie. Dzięki temu po zdjęciu blokady przycisk trzymany od dawna nie tworzy fałszywego zbocza, a przesunięcie to zawsze ruch z ostatniej klatki, nie cała droga, którą kursor przebył nad panelem.

Dziś blokada myszy nie daje widocznego efektu, bo żaden kod gry nie pyta jeszcze o mysz. Jest przygotowana po to, żeby w M1 przeciąganie suwaka w panelu nie obracało kamery, a kliknięcie w panel nie przechwytywało kursora.

## 6. Panel ImGui

`Input` nie ma własnego elementu w panelu. Panel Renderer jest za to narzędziem do sprawdzenia blokady klawiatury:

| Co zrobić | Co obserwować |
|---|---|
| Nacisnąć `~`, gdy żaden widżet nie jest aktywny | Panele znikają i wracają: `wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)` działa |
| Kliknąć z wciśniętym Ctrl w jedną ze składowych `Clear color` (pole przechodzi w tryb wpisywania) i nacisnąć Escape | ImGui anuluje edycję, program **nie** zamyka się |
| W tym samym trybie wpisywania nacisnąć `~` | Panele nie znikają |
| Przytrzymać mysz na składowej `Clear color` (przeciąganie wartości) i nacisnąć Escape | Program nie zamyka się: aktywny jest widżet, choć to nie pole tekstowe |
| Nacisnąć Escape, gdy żaden widżet nie jest aktywny | Program zamyka się |

Mysz nie ma jeszcze niczego do zaobserwowania w działającym programie: kursor nie jest przechwytywany i gra nie reaguje na mysz. Sposób na podejrzenie wartości opisuje ćwiczenie 4.

## 7. Pułapki

1. **`wasKeyPressed` w `onUpdate`.** Gubi naciśnięcia albo wykonuje akcję kilka razy (sekcja 5.5).
2. **Pomylenie `isKeyDown` z `wasKeyPressed`.** Przełącznik oparty na `isKeyDown` przełącza się w każdej klatce, dopóki trzymam klawisz. To samo dotyczy pary `isMouseButtonDown` i `wasMouseButtonPressed`.
3. **Bardzo krótkie naciśnięcie.** `Input` odczytuje stan raz na klatkę, więc klawisz albo przycisk wciśnięty i puszczony między dwoma odczytami nie zostanie zauważony. Przy 60 FPS to rzadkie, ale przy długiej klatce możliwe.
4. **Przesunięcie myszy w `onUpdate`.** `mouseDeltaX` i `mouseDeltaY` opisują jedną klatkę. Użyte w `onUpdate` przepadają w klatce bez kroku i liczą się kilka razy w klatce z kilkoma krokami, więc czułość myszy zależałaby od FPS (sekcja 2.8). Czytam je tylko w `onRender`.
5. **"Escape nie działa".** Jeśli Escape albo `~` nie reaguje, najpewniej aktywny jest widżet ImGui (trwa edycja albo przeciąganie). To zamierzone zachowanie blokady, nie błąd. Wystarczy zakończyć edycję (Enter, Escape albo kliknięcie poza polem).
6. **Jedna flaga, jeden właściciel.** `setKeyboardBlocked` i `setMouseBlocked` nadpisują poprzednią wartość. Gdy pojawi się drugi powód blokady, wołający musi sam połączyć oba warunki w jedną wartość, inaczej ostatnie wywołanie w klatce wygra.
7. **Blokada zatrzymuje też ruch.** Przy zablokowanej klawiaturze `isKeyDown` zwraca `false`, więc postać trzymająca W zatrzyma się na czas edycji pola. Tak ma być, ale warto o tym pamiętać od M1.
8. **Pytanie o klawisz albo mysz z pominięciem `Input`.** Bezpośrednie `glfwGetKey`, `glfwGetMouseButton` albo `glfwGetCursorPos` w kodzie gry omija blokadę. Wszystkie pytania gry o wejście mają iść przez `input()`.
9. **Zamiana kolejności `pollEvents` i `update`.** `glfwGetKey`, `glfwGetMouseButton` i `glfwGetCursorPos` oddają stan z ostatniego `glfwPollEvents`, więc `update` przed `pollEvents` widziałby wejście z opóźnieniem jednej klatki.
10. **Skok kursora przy zmianie trybu.** Po `glfwSetInputMode(..., GLFW_CURSOR, ...)` pozycja kursora może odskoczyć. Bez `m_skipNextMouseDelta` pierwsza klatka po przechwyceniu albo zwolnieniu dałaby jedno ogromne przesunięcie. To samo przy pierwszym `update` po starcie.
11. **Blokada myszy spóźnia się o klatkę.** Tak jak przy klawiaturze, nowa wartość `setMouseBlocked` działa od następnej klatki. Kliknięcie, które trafia w panel w pierwszej klatce po najechaniu na niego, gra może jeszcze zobaczyć.
12. **Retina: mysz jest we współrzędnych ekranu.** Przesunięcie jest liczone w jednostkach rozmiaru okna, nie w pikselach framebuffera. Na ekranie 2x ruch przez całe okno 1280 x 720 daje 1280, a nie 2560. Mieszanie tych jednostek z `framebufferSize()` daje wynik dwukrotnie za mały albo za duży.
13. **Oś y myszy rośnie w dół.** Ruch myszy do góry daje ujemne `mouseDeltaY`. Kod kamery musi odwrócić znak, żeby ruch do góry podnosił wzrok.
14. **Własny callback myszy.** `glfwSetCursorPosCallback` albo `glfwSetMouseButtonCallback` ustawione po utworzeniu `DebugUI` podmieniają callbacki backendu ImGui i panele przestają reagować na mysz. Dlatego mysz jest odpytywana.
15. **Przechwycony kursor a panele ImGui.** Backend GLFW biblioteki ImGui w trybie `GLFW_CURSOR_DISABLED` nie zmienia kształtu kursora, ale pozycję kursora nadal przekazuje do ImGui (jest to wtedy pozycja wirtualna). Niewidoczny kursor może więc "najechać" na panel i ustawić `WantCaptureMouse`, a wtedy blokada myszy wyzerowałaby przesunięcie. Dziś tego nie widać, bo nikt nie przechwytuje kursora. Trzeba to rozstrzygnąć w `main.cpp` razem z kodem kamery.

## 8. Ćwiczenia

1. **Blokada klawiatury w działaniu.** Uruchom program i wykonaj kolejno wszystkie wiersze tabeli z sekcji 6. Potem w `main.cpp` zakomentuj linię `input().setKeyboardBlocked(m_debugUI.wantsKeyboard());`, zbuduj i powtórz próbę z Escape podczas wpisywania wartości `Clear color`. Opisz różnicę i przywróć linię.
2. **Zamrożone tablice.** Dopisz tymczasowo na początku `Input::update()` warunek `if (m_keyboardBlocked) { return; }`. Zbuduj, wejdź w tryb wpisywania wartości `Clear color` (Ctrl i kliknięcie) i anuluj edycję klawiszem Escape, przytrzymując go przez chwilę. Sprawdź, czy program się zamyka, i wyjaśnij wynik tabelą z sekcji 5.6. Usuń warunek.
3. **`wasKeyPressed` w `onUpdate`.** W `NightMazeApp.cpp` dołącz `<GLFW/glfw3.h>` i `"core/Log.hpp"`, a w `NightMazeApp::onUpdate` dopisz `if (input().wasKeyPressed(GLFW_KEY_SPACE)) { core::logInfo("space"); }`. Naciskaj spację i licz linie w konsoli przypadające na jedno naciśnięcie. Powtórz przy wyłączonym vsync (ćwiczenie 1 w [`window-context.md`](window-context.md)). Wyjaśnij wyniki tabelą z sekcji 5.5 i wycofaj zmiany.
4. **Podgląd myszy.** W `NightMazeApp::onRender` dopisz tymczasowo (z nagłówkami `<GLFW/glfw3.h>`, `<string>` i `"core/Log.hpp"`): `if (input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) { core::logInfo("click"); }` oraz wypisywanie `std::to_string(input().mouseDeltaX())`, gdy wartość jest różna od zera. Sprawdź trzy rzeczy: ruch w prawo daje wartości dodatnie, kliknięcie w scenę wypisuje `click`, a kliknięcie w panel Renderer i ruch kursora nad nim nie wypisują nic. Wycofaj zmiany.
5. **Przechwycenie i Escape.** W tym samym miejscu dopisz tymczasowo `if (input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) { input().setCursorCaptured(true); }`. Kliknij w scenę: kursor znika. Naciśnij Escape: kursor wraca, program działa. Naciśnij Escape drugi raz: program się zamyka. Wskaż w `Application::run` linie, które za to odpowiadają, i wycofaj zmianę.
6. **Skok kursora.** Z kodem z ćwiczeń 4 i 5 zakomentuj tymczasowo linię `m_skipNextMouseDelta = true;` w `setCursorCaptured`. Przechwyć kursor, pokręć myszą w jedną stronę, naciśnij Escape i odczytaj z konsoli przesunięcie w klatce po zwolnieniu. Wyjaśnij, skąd się wzięło, i przywróć linię.

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
   `core/` nie może zależeć od ImGui ani od `debug/` (reguła warstw, biblioteka `engine` linkuje tylko `glad` i `glfw`). Flagi są neutralne: mówią tylko, że ktoś inny ma klawiaturę albo mysz. Kto i dlaczego, decyduje `main.cpp`, jedyny plik znający obie warstwy.

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
    Jeśli kursor jest przechwycony, Escape tylko go zwalnia (`setCursorCaptured(false)`). Jeśli nie jest, zamyka program (`requestClose()`). Przy przechwyconym kursorze użytkownik nie ma jak kliknąć w panel ani w okno, więc najpierw odzyskuje mysz. Dziś kursor nie jest nigdy przechwytywany, więc Escape zawsze zamyka program.

## 10. Źródła

- Dokumentacja GLFW: <https://www.glfw.org/docs/latest/> (przewodnik o wejściu, "Input guide": `glfwGetKey`, `glfwPollEvents`, kody klawiszy, `glfwGetCursorPos`, `glfwGetMouseButton`, tryby kursora, surowy ruch myszy).
- Dear ImGui, repozytorium: <https://github.com/ocornut/imgui> (plik `docs/FAQ.md`, pytanie o to, jak rozpoznać, czy wejście ma trafić do ImGui czy do aplikacji, oraz komentarze przy `WantCaptureKeyboard` i `WantCaptureMouse` w `imgui.h`). Po pierwszej konfiguracji te pliki leżą lokalnie w `build/debug/_deps/imgui-src/`.
- Dokumenty bibliotek w tym repozytorium: [`../../libraries/glfw.md`](../../libraries/glfw.md), [`../../libraries/imgui.md`](../../libraries/imgui.md) (sekcja 3.8).
- Podpięcie nakładki debug: [`../debug-ui.md`](../debug-ui.md).
- LearnOpenGL, rozdział "Hello Window" (<https://learnopengl.com/Getting-started/Hello-Window>): obsługa klawisza Escape przez `glfwGetKey`.
- LearnOpenGL, rozdział "Camera" (<https://learnopengl.com/Getting-started/Camera>): sterowanie kamerą myszą, przechwycenie kursora i problem pierwszego odczytu.
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o pierwszym programie i obsłudze zdarzeń).
