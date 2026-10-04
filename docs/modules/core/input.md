# Moduł core: klawiatura

Kamień milowy: M0. Temat wykładu: 1 (Pierwszy program OpenGL).
Kod: [`src/core/Input.hpp`](../../../src/core/Input.hpp), [`src/core/Input.cpp`](../../../src/core/Input.cpp), użycie w [`src/core/Application.cpp`](../../../src/core/Application.cpp) i [`src/main.cpp`](../../../src/main.cpp).

Część modułu `core`. Wstęp do całego modułu i diagram warstw są w [`README.md`](README.md). Pozostałe części: [`window-context.md`](window-context.md) (okno i kontekst), [`main-loop.md`](main-loop.md) (pętla i czas), [`gl-check.md`](gl-check.md) (błędy OpenGL). Funkcje wejścia samej biblioteki GLFW opisuje [`../../libraries/glfw.md`](../../libraries/glfw.md).

## 1. Po co to jest

Gra potrzebuje odpowiedzi na dwa różne pytania o klawisz: "czy jest teraz trzymany" (ruch) i "czy został właśnie wciśnięty" (przełącznik, zamknięcie programu). GLFW odpowiada wprost tylko na pierwsze. Klasa `core::Input` raz na klatkę robi migawkę (snapshot) stanu całej klawiatury, pamięta migawkę z poprzedniej klatki i z ich porównania wyprowadza drugie pytanie. Ma też jedną flagę, `setKeyboardBlocked`, którą ktoś z zewnątrz może na chwilę odebrać grze klawiaturę. Dziś robi to `main.cpp`, gdy klawiatury używa panel ImGui: dzięki temu Escape wciśnięty podczas edycji pola w panelu nie zamyka programu. Mysz nie jest jeszcze obsługiwana (pojawi się w M1 razem z kamerą).

## 2. Teoria

### 2.1 Odpytywanie zamiast zdarzeń

Biblioteka okienkowa daje dwa sposoby czytania klawiatury:

- **zdarzenia** (events, callbacki): biblioteka woła moją funkcję przy każdym naciśnięciu i puszczeniu,
- **odpytywanie** (polling): w dowolnej chwili pytam "jaki jest teraz stan klawisza X".

`Input` używa odpytywania. Jest prostsze (żadnych funkcji zwrotnych, żadnego stanu globalnego), pasuje do pętli gry, która i tak wykonuje się co klatkę, i nie koliduje z callbackami, które instaluje backend ImGui. Ceną jest to, że widzę tylko stan w chwili odczytu, a nie historię zdarzeń (sekcja 7, pułapka 3).

### 2.2 Stan ciągły i zbocze

- **Stan ciągły** (level): klawisz jest wciśnięty albo nie. Wartość trwa tak długo, jak trzymam klawisz. Do ruchu.
- **Zbocze** (edge): chwila przejścia z "puszczony" na "wciśnięty". Trwa dokładnie jedną klatkę. Do akcji jednorazowych.

Zbocza nie da się odczytać z jednego pomiaru. Potrzebne są dwa: bieżący i poprzedni. Stąd dwie tablice w klasie.

### 2.3 Kto ma klawiaturę

W programie są dwaj odbiorcy tego samego klawisza: gra i interfejs debugowy. Gdy wpisuję liczbę w pole panelu, naciśnięcia mają trafić tylko do panelu. Ktoś musi więc rozstrzygać, kto w danej chwili "ma" klawiaturę. ImGui wie, czy jej używa (pole `WantCaptureKeyboard`, zob. [`../../libraries/imgui.md`](../../libraries/imgui.md), sekcja 3.8), a `Input` jest miejscem, przez które przechodzą wszystkie pytania gry o klawisze. Wystarczy więc jedna flaga w `Input` i jedno miejsce, które ją ustawia. Jak to jest zrobione bez wprowadzania ImGui do `core/`, opisuje sekcja 5.6.

## 3. Jak to działa w OpenGL

Klawiatura nie jest częścią OpenGL: `Input` nie woła żadnej funkcji `gl*`. Rozmawia wyłącznie z GLFW, a konkretnie z jedną funkcją:

| Wywołanie | Kto woła | Co robi |
|---|---|---|
| `glfwPollEvents()` | `Window::pollEvents()` w `Application::run` | Odbiera zdarzenia systemu i aktualizuje stan klawiszy zapamiętany wewnątrz GLFW |
| `glfwGetKey(m_window, key)` | `Input::update()` | Zwraca `GLFW_PRESS` albo `GLFW_RELEASE`: stan zapamiętany podczas ostatniego `glfwPollEvents`. Nie pyta systemu |

Kolejność w pętli wynika z tej tabeli: najpierw `m_window.pollEvents()`, potem `m_input.update()`.

## 4. Shadery

Ta część modułu nie ma shaderów i nie ma z nimi żadnego związku.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/core/Input.hpp`](../../../src/core/Input.hpp) | klasa `Input`: `update`, `isKeyDown`, `wasKeyPressed`, `setKeyboardBlocked`, stała `KEY_COUNT`, dwie tablice stanu, flaga `m_keyboardBlocked` |
| [`src/core/Input.cpp`](../../../src/core/Input.cpp) | implementacja i `static_assert` pilnujący `KEY_COUNT` |
| [`src/core/Application.cpp`](../../../src/core/Application.cpp) | `m_input.update()` raz na klatkę i obsługa Escape |
| [`src/main.cpp`](../../../src/main.cpp) | przełącznik paneli (`GLFW_KEY_GRAVE_ACCENT`) i ustawianie blokady klawiatury |

### 5.2 `update`: migawka klawiatury

```cpp
void Input::update() {
    m_previous = m_current;
    // GLFW key codes start at GLFW_KEY_SPACE (32), lower values are not valid keys.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        m_current[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }
}
```

Raz na klatkę kopiuję stan bieżący do poprzedniego i odczytuję nowy stan wszystkich klawiszy. `glfwGetKey` nie pyta systemu, tylko zwraca stan zapamiętany przez GLFW podczas ostatniego `glfwPollEvents`, dlatego w pętli `pollEvents()` stoi przed `m_input.update()`. Pętla zaczyna od `GLFW_KEY_SPACE` (32), bo niższe kody nie są klawiszami i GLFW zgłosiłby dla nich błąd.

Ważne dla sekcji 5.6: `update` **nie sprawdza** flagi `m_keyboardBlocked`. Tablice są odświeżane w każdej klatce, także wtedy, gdy klawiatura jest zablokowana.

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

bool Input::isValidKey(int key) const {
    return key >= 0 && key < KEY_COUNT;
}
```

- `isKeyDown`: stan ciągły, "klawisz jest teraz wciśnięty". Do ruchu (trzymam W, idę).
- `wasKeyPressed`: **zbocze** (edge), "w tej klatce jest wciśnięty, a w poprzedniej nie był". Do akcji jednorazowych (klawisz `~` przełącza panele, Escape zamyka).

Operator `&&` wylicza warunki od lewej i przerywa na pierwszym fałszywym (short circuit). Kolejność ma więc znaczenie:

1. `!m_keyboardBlocked`: jeśli klawiatura jest zablokowana, odpowiedź brzmi `false` dla każdego klawisza i dalsze warunki nie są sprawdzane.
2. `isValidKey(key)`: chroni przed wyjściem poza tablicę. To nie jest teoria: GLFW ma stałą `GLFW_KEY_UNKNOWN` równą -1. Dopiero po tym warunku wolno indeksować tablicę.
3. `m_current[key]` i (dla zbocza) `!m_previous[key]`.

Klawisze identyfikuję stałymi GLFW (`GLFW_KEY_ESCAPE`, `GLFW_KEY_GRAVE_ACCENT`). Oznaczają one fizyczne położenie klawisza w układzie US, a nie znak, który klawisz wpisuje.

### 5.4 `KEY_COUNT = 348 + 1` i `static_assert`

```cpp
// Input.hpp
static constexpr int KEY_COUNT = 348 + 1;
std::array<bool, KEY_COUNT> m_current{};  // key states in this frame
std::array<bool, KEY_COUNT> m_previous{}; // key states in the previous frame
```

```cpp
// Input.cpp
Input::Input(GLFWwindow* window) : m_window(window) {
    static_assert(KEY_COUNT == GLFW_KEY_LAST + 1, "KEY_COUNT must cover every GLFW key code");
}
```

Tablica jest indeksowana bezpośrednio kodem klawisza GLFW. Największy kod to `GLFW_KEY_LAST` (równy `GLFW_KEY_MENU`, czyli 348), więc poprawne indeksy to od 0 do 348, a to wymaga **349** elementów: stąd `+ 1`. Indeksy od 0 do 31 pozostają nieużywane, to świadomy koszt 32 bajtów w zamian za najprostsze możliwe indeksowanie. `{}` przy polu zeruje tablicę (wszystkie klawisze puszczone).

Dlaczego gołe `348`, a nie `GLFW_KEY_LAST`? Żeby `Input.hpp` nie musiał dołączać nagłówka GLFW (tak samo jak `Window.hpp`, zob. [`window-context.md`](window-context.md), sekcja 5.2). Goła liczba jest jednak "magiczna" i mogłaby się rozjechać z biblioteką po aktualizacji GLFW. Dlatego w `Input.cpp`, gdzie nagłówek GLFW jest już dołączony, stoi `static_assert`: warunek sprawdzany **w czasie kompilacji**, bez żadnego kosztu w działającym programie. Jeśli nowa wersja GLFW doda klawisze, build się nie powiedzie i pokaże podany komunikat. Asercja jest wewnątrz konstruktora, bo `KEY_COUNT` jest prywatne i trzeba być w funkcji składowej, żeby mieć do niego dostęp.

### 5.5 Dlaczego `wasKeyPressed` jest "na klatkę rysowaną" i nie wolno go wołać z `onUpdate`

Zbocze istnieje między dwoma kolejnymi wywołaniami `Input::update()`, a to jest wołane dokładnie raz na obrót pętli głównej. `onUpdate` jest natomiast wołane od zera do wielu razy na klatkę ([`main-loop.md`](main-loop.md), sekcja 2.2):

| Liczba kroków w klatce | Co stałoby się z `wasKeyPressed` wołanym w `onUpdate` |
|---|---|
| 0 (szybka klatka) | Naciśnięcie **przepada**: w tej klatce nikt nie zapytał, a w następnej `m_previous` jest już `true` |
| 1 | Działa, ale tylko przypadkiem |
| 2 lub więcej | Każdy krok widzi to samo `true`, więc akcja wykona się kilka razy. Przełącznik włączony i wyłączony w tej samej klatce wygląda jak "klawisz nie działa" |

Dlatego obsługa przełącznika paneli stoi w `DebugNightMazeApp::onRender` w `main.cpp` (`if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) { m_debugUI.toggleVisible(); }`), a Escape w `Application::run` przed pętlą kroków: oba miejsca wykonują się dokładnie raz na klatkę. W `onUpdate` wolno używać `isKeyDown`, bo stan ciągły jest taki sam w każdym kroku danej klatki. Gdy w późniejszych kamieniach milowych jednorazowa akcja będzie musiała wpłynąć na symulację (na przykład skok), trzeba ją odczytać raz na klatkę i przekazać do symulacji jako zapamiętane żądanie.

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
// or a widget is active). If so, block the game's keyboard from the next frame
// on, so typing does not trigger Escape, the panel toggle or player movement.
input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
```

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
| Escape | `GLFW_KEY_ESCAPE` | `wasKeyPressed` | `Application::run` w [`Application.cpp`](../../../src/core/Application.cpp) | zamyka program (`m_window.requestClose()`) |
| `~` (na lewo od `1`) | `GLFW_KEY_GRAVE_ACCENT` | `wasKeyPressed` | `DebugNightMazeApp::onRender` w [`main.cpp`](../../../src/main.cpp) | chowa i pokazuje panele debug |

Oba pytania przechodzą przez `Input`, więc oba podlegają blokadzie. `isKeyDown` nie jest jeszcze nigdzie używane: przyda się od M1 (ruch kamery).

## 6. Panel ImGui

`Input` nie ma własnego elementu w panelu. Panel Renderer jest za to narzędziem do sprawdzenia blokady klawiatury:

| Co zrobić | Co obserwować |
|---|---|
| Nacisnąć `~`, gdy żaden widżet nie jest aktywny | Panele znikają i wracają: `wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)` działa |
| Kliknąć z wciśniętym Ctrl w jedną ze składowych `Clear color` (pole przechodzi w tryb wpisywania) i nacisnąć Escape | ImGui anuluje edycję, program **nie** zamyka się |
| W tym samym trybie wpisywania nacisnąć `~` | Panele nie znikają |
| Przytrzymać mysz na składowej `Clear color` (przeciąganie wartości) i nacisnąć Escape | Program nie zamyka się: aktywny jest widżet, choć to nie pole tekstowe |
| Nacisnąć Escape, gdy żaden widżet nie jest aktywny | Program zamyka się |

## 7. Pułapki

1. **`wasKeyPressed` w `onUpdate`.** Gubi naciśnięcia albo wykonuje akcję kilka razy (sekcja 5.5).
2. **Pomylenie `isKeyDown` z `wasKeyPressed`.** Przełącznik oparty na `isKeyDown` przełącza się w każdej klatce, dopóki trzymam klawisz.
3. **Bardzo krótkie naciśnięcie.** `Input` odczytuje stan raz na klatkę, więc klawisz wciśnięty i puszczony między dwoma odczytami nie zostanie zauważony. Przy 60 FPS to rzadkie, ale przy długiej klatce możliwe.
4. **Mysz nadal omija ImGui.** Blokada dotyczy tylko klawiatury. `core::Input` nie obsługuje jeszcze myszy, a gdy w M1 kamera FPS zacznie jej używać, trzeba będzie w ten sam sposób uwzględnić `ImGui::GetIO().WantCaptureMouse`. Bez tego przeciąganie suwaka w panelu obracałoby jednocześnie kamerę.
5. **"Escape nie działa".** Jeśli Escape albo `~` nie reaguje, najpewniej aktywny jest widżet ImGui (trwa edycja albo przeciąganie). To zamierzone zachowanie blokady, nie błąd. Wystarczy zakończyć edycję (Enter, Escape albo kliknięcie poza polem).
6. **Jedna flaga, jeden właściciel.** `setKeyboardBlocked` nadpisuje poprzednią wartość. Gdy pojawi się drugi powód blokady, wołający musi sam połączyć oba warunki w jedną wartość, inaczej ostatnie wywołanie w klatce wygra.
7. **Blokada zatrzymuje też ruch.** Przy zablokowanej klawiaturze `isKeyDown` zwraca `false`, więc postać trzymająca W zatrzyma się na czas edycji pola. Tak ma być, ale warto o tym pamiętać od M1.
8. **Pytanie o klawisz z pominięciem `Input`.** Bezpośrednie `glfwGetKey` w kodzie gry omija blokadę. Wszystkie pytania gry o klawiaturę mają iść przez `input()`.
9. **Zamiana kolejności `pollEvents` i `update`.** `glfwGetKey` oddaje stan z ostatniego `glfwPollEvents`, więc `update` przed `pollEvents` widziałby klawiaturę z opóźnieniem jednej klatki.

## 8. Ćwiczenia

1. **Blokada klawiatury w działaniu.** Uruchom program i wykonaj kolejno wszystkie wiersze tabeli z sekcji 6. Potem w `main.cpp` zakomentuj linię `input().setKeyboardBlocked(m_debugUI.wantsKeyboard());`, zbuduj i powtórz próbę z Escape podczas wpisywania wartości `Clear color`. Opisz różnicę i przywróć linię.
2. **Zamrożone tablice.** Dopisz tymczasowo na początku `Input::update()` warunek `if (m_keyboardBlocked) { return; }`. Zbuduj, wejdź w tryb wpisywania wartości `Clear color` (Ctrl i kliknięcie) i anuluj edycję klawiszem Escape, przytrzymując go przez chwilę. Sprawdź, czy program się zamyka, i wyjaśnij wynik tabelą z sekcji 5.6. Usuń warunek.
3. **`wasKeyPressed` w `onUpdate`.** W `NightMazeApp.cpp` dołącz `<GLFW/glfw3.h>` i `"core/Log.hpp"`, a w `NightMazeApp::onUpdate` dopisz `if (input().wasKeyPressed(GLFW_KEY_SPACE)) { core::logInfo("space"); }`. Naciskaj spację i licz linie w konsoli przypadające na jedno naciśnięcie. Powtórz przy wyłączonym vsync (ćwiczenie 1 w [`window-context.md`](window-context.md)). Wyjaśnij wyniki tabelą z sekcji 5.5 i wycofaj zmiany.

## 9. Pytania kontrolne

1. **Dlaczego `KEY_COUNT = 348 + 1` i co sprawdza `static_assert`?**
   Tablica jest indeksowana kodem klawisza, największy to `GLFW_KEY_LAST` = 348, więc potrzeba 349 elementów. Liczba jest wpisana wprost, żeby nagłówek nie dołączał GLFW, a `static_assert` w `Input.cpp` w czasie kompilacji pilnuje zgodności z GLFW.

2. **Czym różni się `isKeyDown` od `wasKeyPressed` i dlaczego drugiego nie wolno wołać w `onUpdate`?**
   Pierwsze to stan ciągły, drugie to zbocze między dwiema klatkami. `onUpdate` wykonuje się od zera do wielu razy na klatkę, więc zbocze zostałoby zgubione albo obsłużone kilka razy.

3. **Dlaczego `m_input.update()` stoi po `m_window.pollEvents()`?**
   `glfwGetKey` nie pyta systemu, tylko zwraca stan zapamiętany przez GLFW podczas ostatniego `glfwPollEvents`. Najpierw trzeba więc odebrać zdarzenia.

4. **Co się dzieje, gdy wpisuję wartość w panelu ImGui i naciskam Escape? Prześledź drogę przez kod.**
   Aktywny widżet sprawia, że `ImGui::GetIO().WantCaptureKeyboard` jest `true`. `DebugUI::wantsKeyboard()` zwraca tę wartość, a `DebugNightMazeApp::onRender` przekazuje ją do `input().setKeyboardBlocked(...)`. Od następnej klatki `wasKeyPressed(GLFW_KEY_ESCAPE)` w `Application::run` zwraca `false`, więc program się nie zamyka. Escape trafia tylko do ImGui, które anuluje edycję.

5. **Dlaczego `core::Input` ma flagę `m_keyboardBlocked`, zamiast samemu sprawdzać `ImGui::GetIO().WantCaptureKeyboard`?**
   `core/` nie może zależeć od ImGui ani od `debug/` (reguła warstw, biblioteka `engine` linkuje tylko `glad` i `glfw`). Flaga jest neutralna: mówi tylko, że ktoś inny ma klawiaturę. Kto i dlaczego, decyduje `main.cpp`, jedyny plik znający obie warstwy.

6. **Blokada zaczyna działać z opóźnieniem. Skąd się ono bierze i dlaczego nie przeszkadza?**
   `setKeyboardBlocked` jest wołane na końcu `onRender`, po tym, jak pytania o klawisze w tej klatce już padły, więc działa od następnej klatki. Dodatkowo ImGui wylicza `WantCaptureKeyboard` w `NewFrame` na podstawie widżetu aktywnego po poprzedniej klatce. Razem to najwyżej dwie klatki od kliknięcia, czyli ułamek sekundy krótszy niż czas potrzebny na naciśnięcie klawisza po kliknięciu.

7. **Dlaczego po zdjęciu blokady nie pojawia się fałszywe "właśnie wciśnięty"?**
   `Input::update()` odświeża `m_current` i `m_previous` w każdej klatce, także przy blokadzie. Klawisz wciśnięty w czasie blokady ma więc po jej zdjęciu `m_previous == true` i nie tworzy zbocza. Blokada zmienia tylko odpowiedzi `isKeyDown` i `wasKeyPressed`, nie pomiar.

8. **Kiedy `WantCaptureKeyboard` jest prawdziwe?**
   Gdy aktywny jest dowolny widżet ImGui (edycja pola, przeciąganie suwaka lub wartości, trzymany przycisk, przesuwane okno) albo otwarte jest okno modalne. Nie tylko w polach tekstowych, bo aktywny widżet może sam używać klawiszy (Escape, Tab, modyfikatory).

9. **Czy blokada klawiatury rozwiązuje też problem myszy?**
   Nie. Dotyczy tylko klawiatury. Mysz (`WantCaptureMouse`) trzeba będzie obsłużyć osobno w M1, gdy kamera zacznie z niej korzystać.

## 10. Źródła

- Dokumentacja GLFW: <https://www.glfw.org/docs/latest/> (przewodnik o wejściu: `glfwGetKey`, `glfwPollEvents`, kody klawiszy).
- Dear ImGui, repozytorium: <https://github.com/ocornut/imgui> (plik `docs/FAQ.md`, pytanie o to, jak rozpoznać, czy wejście ma trafić do ImGui czy do aplikacji, oraz komentarze przy `WantCaptureKeyboard` w `imgui.h`). Po pierwszej konfiguracji te pliki leżą lokalnie w `build/debug/_deps/imgui-src/`.
- Dokumenty bibliotek w tym repozytorium: [`../../libraries/glfw.md`](../../libraries/glfw.md), [`../../libraries/imgui.md`](../../libraries/imgui.md) (sekcja 3.8).
- Podpięcie nakładki debug: [`../debug-ui.md`](../debug-ui.md).
- LearnOpenGL, rozdział "Hello Window" (<https://learnopengl.com/Getting-started/Hello-Window>): obsługa klawisza Escape przez `glfwGetKey`.
- Janusz Ganczarski, "OpenGL. Podstawy programowania grafiki 3D" (rozdziały o pierwszym programie i obsłudze zdarzeń).
