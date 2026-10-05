# Moduł scene: sterowanie kamerą i panel Camera

Kamień milowy: M1. Temat wykładu: 3 (Przekształcenia przestrzeni).
Kod: sterowanie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), panel w [`src/debug/panels/CameraPanel.hpp`](../../../src/debug/panels/CameraPanel.hpp) i [`src/debug/panels/CameraPanel.cpp`](../../../src/debug/panels/CameraPanel.cpp), podpięcie w [`src/main.cpp`](../../../src/main.cpp).

Część modułu `scene`. Wstęp do modułu i jego miejsce w warstwach są w [`README.md`](README.md). Pozostałe części: [`transforms.md`](transforms.md) (przestrzenie współrzędnych i macierz modelu) i [`camera.md`](camera.md) (macierz widoku, rzutowanie, struktura `Camera`). Ten dokument zakłada znajomość [`camera.md`](camera.md) (kąty yaw i pitch, `forward()`, `right()`, `rotate()`, `viewMatrix(eye)`) i korzysta z trzech dokumentów modułu `core`: [`../core/input.md`](../core/input.md) (mysz, przechwycenie kursora, blokady), [`../core/main-loop.md`](../core/main-loop.md) (stały krok i `alpha`) oraz z [`../debug-ui.md`](../debug-ui.md) (jak panel jest podpięty).

## 1. Po co to jest

Kamera z [`camera.md`](camera.md) to same liczby: pozycja, dwa kąty i parametry rzutowania. Sama z siebie stoi w miejscu. Ten dokument opisuje, kto i kiedy te liczby zmienia. Mysz obraca kamerę, klawiatura ją przesuwa, a stały krok symulacji i interpolacja z `alpha` sprawiają, że ruch jest płynny i nie zależy od FPS. Sterowanie należy do `game::NightMazeApp`, a nie do `scene::Camera`. Drugim sposobem zmiany tych samych liczb jest panel Camera z `debug/`: pokaz tematu 3 na obronie.

Stan na dziś: kamera startuje w pozycji domyślnej `(0, 0, 3)` i można nią latać wokół kostki: kliknięcie w scenę przechwytuje kursor, mysz obraca kamerę, klawisze W, A, S, D, spacja i lewy Shift ją przesuwają, a Escape oddaje kursor (teoria w sekcji 2, kod w sekcjach od 5.2 do 5.5). Pola kamery, czułość myszy i prędkość ruchu edytuje panel Camera (sekcja 6).

## 2. Teoria

Z [`camera.md`](camera.md), sekcja 2.2, wiadomo, jak z dwóch kątów powstaje kierunek patrzenia. Ta sekcja mówi, skąd biorą się same kąty i pozycja: z myszy, z klawiatury i z upływu czasu.

### 2.1 Mysz: przesunięcie na kąty

**Mysz: przesunięcie na kąty.** Kamery nie interesuje, gdzie kursor jest, tylko o ile się przesunął od poprzedniej klatki ([`../core/input.md`](../core/input.md), sekcja 2.4). Przesunięcie w poziomie zmienia yaw, przesunięcie w pionie zmienia pitch:

```text
zmiana yaw   =  mouseDeltaX * czułość
zmiana pitch = -mouseDeltaY * czułość
```

- **Czułość** (sensitivity) to współczynnik przeliczający ruch myszy na kąt. Jej jednostka to **stopnie na jednostkę współrzędnych ekranu**. Przy wartości domyślnej 0,1 przesunięcie kursora o 900 jednostek obraca kamerę o 90 stopni. Jednostką myszy są współrzędne ekranu (te same co rozmiar okna), a nie piksele framebuffera, więc ten sam ruch ręki daje ten sam obrót na zwykłym ekranie i na ekranie Retina ([`../core/input.md`](../core/input.md), sekcja 2.5).
- **Znak przy x** się nie zmienia: ruch myszy w prawo daje dodatnie `mouseDeltaX`, a dodatni yaw obraca kamerę w prawo. Po to yaw ma w tym projekcie kierunek kompasu ([`camera.md`](camera.md), sekcja 2.2).
- **Minus przy y** bierze się z dwóch przeciwnych konwencji: współrzędna y ekranu rośnie **w dół**, a pitch rośnie przy patrzeniu **w górę**. Ruch myszy do góry daje ujemne `mouseDeltaY`, więc trzeba odwrócić znak, żeby podnosił wzrok. Kto woli odwrócone sterowanie jak w symulatorze lotu (invert y), usuwa ten minus.
- Yaw jest potem zawijany, a pitch przycinany: robi to `Camera::rotate` ([`camera.md`](camera.md), sekcja 5.4).

### 2.2 Klawiatura: ruch wzdłuż osi kamery

**Klawiatura: ruch wzdłuż osi kamery.** Kamera ma trzy kierunki, wzdłuż których może się przesuwać:

| Klawisze | Kierunek | Skąd się bierze |
|---|---|---|
| W i S | do przodu i do tyłu | `forward()`: kierunek patrzenia ([`camera.md`](camera.md), sekcja 2.2) |
| D i A | w prawo i w lewo | `right()`: zawsze poziomy |
| spacja i lewy Shift | w górę i w dół | `WORLD_UP`: pion świata, niezależnie od tego, gdzie patrzę |

Każdy wciśnięty klawisz dodaje swój wektor do sumy (klawisz przeciwny go odejmuje), więc dwa klawisze przeciwne znoszą się do zera, a dwa prostopadłe dają ruch po skosie. Nowa pozycja to:

```text
pozycja = pozycja + kierunek * prędkość * czas
```

Prędkość jest w metrach na sekundę, czas w sekundach, więc iloczyn to metry: droga przebyta w tym odcinku czasu.

**To jest lot, nie chodzenie.** W i S przesuwają wzdłuż `forward()`, który ma składową pionową, gdy patrzę w górę albo w dół. Trzymając W ze wzrokiem podniesionym o 30 stopni, kamera wznosi się: połowa prędkości idzie w górę (`sin(30 stopni) = 0,5`). Dla kamery latającej (free fly) tak ma być: leci tam, gdzie patrzy. Dla postaci chodzącej po podłodze byłby to błąd. Chodzenie wymaga rzutowania kierunku na poziom, czyli wyzerowania składowej y i ponownej normalizacji (ćwiczenie 1). W projekcie zostaje lot, dopóki nie ma kolizji i podłogi (M2).

**Normalizacja kierunku.** Wzór na pozycję zakłada, że `kierunek` ma długość 1. Suma dwóch prostopadłych wektorów jednostkowych (W i D naraz) ma długość `sqrt(2)`, czyli około 1,41: bez poprawki ruch po skosie byłby o 41 procent szybszy niż na wprost. Sumę trzeba więc **znormalizować**, czyli podzielić przez jej długość. Jest jeden wyjątek: gdy żaden klawisz nie jest wciśnięty (albo wciśnięte są dwa przeciwne), suma jest wektorem zerowym, a jego normalizacja to dzielenie zera przez zero. Wynikiem jest `NaN` w każdej składowej, `NaN` dodany do pozycji zostaje w niej na zawsze i obraz znika. Wektor zerowy zostawiam więc bez zmian.

### 2.3 Obrót raz na klatkę, ruch stałym krokiem

**Dlaczego obrót raz na klatkę, a ruch stałym krokiem.** To dwa różne rodzaje danych wejściowych:

| | Obrót myszą | Ruch klawiszami |
|---|---|---|
| Rodzaj danych | przesunięcie myszy: wartość opisująca **jedną klatkę** | stan klawisza: "jest wciśnięty", taki sam przez całą klatkę |
| Od czego zależy wynik | tylko od drogi, którą przebyła mysz | od **czasu** trzymania klawisza |
| Gdzie w pętli | `onRender`, dokładnie raz na klatkę | `onUpdate`, stałym krokiem `FIXED_DT` |

Przesunięcie myszy to gotowa wielkość: ręka przesunęła mysz o tyle i kamera ma się obrócić o tyle razy czułość, niezależnie od tego, ile trwała klatka. Nie mnożę jej przez czas. Trzeba ją tylko zastosować **dokładnie raz**. `onUpdate` wykonuje się od zera do wielu razy na klatkę ([`../core/main-loop.md`](../core/main-loop.md), sekcja 2.2): w klatce bez kroku ruch myszy by przepadł, a w klatce z trzema krokami zostałby dodany trzy razy, więc czułość zależałaby od FPS ([`../core/input.md`](../core/input.md), sekcja 2.8). Ruch klawiszami zależy od czasu, a czas symulacji płynie właśnie stałymi krokami: 120 kroków po `prędkość / 120` metra daje dokładnie `prędkość` metrów na sekundę przy każdym FPS. Od M2 w tym samym miejscu dojdą kolizje, które tego stałego kroku wymagają.

### 2.4 Interpolacja z `alpha`

**Interpolacja z `alpha`.** Skoro pozycja zmienia się tylko w krokach symulacji (co 8,33 ms), a klatki są rysowane we własnym rytmie (na przykład co 6 ms), to klatka wypada zwykle **między** dwoma krokami. Rysowanie zawsze z ostatniej policzonej pozycji daje szarpanie: jedne klatki pokazują postęp o jeden krok, a inne o zero albo o dwa. Rozwiązanie: pamiętam pozycję sprzed ostatniego kroku i rysuję z punktu leżącego między nią a pozycją bieżącą, w proporcji `alpha` ([`../core/main-loop.md`](../core/main-loop.md), sekcja 2.4):

```text
oko = poprzednia * (1 - alpha) + bieżąca * alpha        czyli glm::mix(poprzednia, bieżąca, alpha)
```

```text
kroki symulacji (co 8,33 ms):   k0          k1          k2          k3
                                 |-----------|-----------|-----------|-----> czas
klatki (co 6 ms):                      K1      K2    K3      K4    K5

K3 wypada po kroku k2: w akumulatorze została reszta 1,33 ms, alpha = 1,33 / 8,33 = 0,16
poprzednia = pozycja po kroku k1,  bieżąca = pozycja po kroku k2
oko        = punkt w 16 procentach drogi od poprzedniej do bieżącej
```

Przykład na liczbach. Kamera leci w prawo z prędkością 3 m/s, więc jeden krok to `3 / 120 = 0,025` m. Klatki trwają po 6 ms (około 167 FPS). Przed pierwszą klatką z tabeli kamera jest w x = 1,000, a przed ostatnim krokiem była w x = 0,975:

| Klatka | Akumulator po `beginFrame` | Kroki | Reszta | `alpha` | Poprzednia x | Bieżąca x | Oko x (z interpolacją) | Przyrost oka | Przyrost bez interpolacji |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 6,00 ms | 0 | 6,00 ms | 0,72 | 0,975 | 1,000 | 0,993 | | |
| 2 | 12,00 ms | 1 | 3,67 ms | 0,44 | 1,000 | 1,025 | 1,011 | 0,018 | 0,025 |
| 3 | 9,67 ms | 1 | 1,33 ms | 0,16 | 1,025 | 1,050 | 1,029 | 0,018 | 0,025 |
| 4 | 7,33 ms | 0 | 7,33 ms | 0,88 | 1,025 | 1,050 | 1,047 | 0,018 | 0,000 |
| 5 | 13,33 ms | 1 | 5,00 ms | 0,60 | 1,050 | 1,075 | 1,065 | 0,018 | 0,025 |
| 6 | 11,00 ms | 1 | 2,67 ms | 0,32 | 1,075 | 1,100 | 1,083 | 0,018 | 0,025 |

Z interpolacją oko przesuwa się w każdej klatce o te same 0,018 m, czyli dokładnie `3 m/s * 0,006 s`. Bez niej (ostatnia kolumna: rysowanie z pozycji bieżącej) obraz skacze o 0,025, a w klatce 4 stoi w miejscu, bo nie zmieścił się w niej żaden krok. Klatka 4 pokazuje też, że interpolacja działa poprawnie w klatce **bez kroku**: poprzednia i bieżąca pozycja są te same co w klatce 3, urosło tylko `alpha` (z 0,16 do 0,88), więc oko przesunęło się dalej wzdłuż tego samego odcinka.

Cena interpolacji: obraz jest spóźniony względem symulacji o najwyżej jeden krok (8,33 ms), bo rysuję punkt między przedostatnim a ostatnim stanem, a nie stan najnowszy. Przy 120 krokach na sekundę tego opóźnienia nie da się zauważyć.

Interpoluję tylko **pozycję**. Kąty zmieniają się raz na klatkę, w tej samej klatce, w której są rysowane, więc nie mają dwóch stanów do mieszania.

## 3. Jak to działa w OpenGL

Sterowanie kamerą (sekcje 2 i 5) nie dokłada do listy wywołań OpenGL niczego: obrót i ruch zmieniają tylko liczby, z których powstaje macierz widoku. Wywołania, które z tych liczb korzystają, i ich kolejność w klatce opisuje [`camera.md`](camera.md), sekcja 3. Odczyt myszy i klawiatury to GLFW, a nie OpenGL: opisuje go [`../core/input.md`](../core/input.md).

## 4. Shadery

Sterowanie i panel nie mają shaderów. Pola kamery, które zmieniają, trafiają do shadera jako macierz widoku i macierz rzutowania ([`camera.md`](camera.md), sekcja 5.7), a mnożenie w `basic.vert` omawia [`transforms.md`](transforms.md), sekcja 4.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | sterowanie kamerą: obrót myszą w `onRender`, ruch klawiszami w `onUpdate`, pola `m_previousCameraPosition`, `m_mouseSensitivity`, `m_moveSpeed`, akcesory `camera()`, `mouseSensitivity()`, `moveSpeed()` (sekcje od 5.2 do 5.5) |
| [`src/debug/panels/CameraPanel.hpp`](../../../src/debug/panels/CameraPanel.hpp), [`.cpp`](../../../src/debug/panels/CameraPanel.cpp) | funkcja `debug::drawCameraPanel`: panel "Camera" (sekcja 6). Należy do programu `night_maze`, nie do biblioteki `engine` |
| [`src/main.cpp`](../../../src/main.cpp) | klasa `DebugNightMazeApp`: przekazuje kamerę, czułość i prędkość do `debug::DebugContext`, wyłącza panelom mysz przy przechwyconym kursorze i blokuje grze mysz, gdy używa jej ImGui ([`../debug-ui.md`](../debug-ui.md), sekcja 5.6) |
| [`src/scene/Camera.hpp`](../../../src/scene/Camera.hpp), [`.cpp`](../../../src/scene/Camera.cpp) | to, czym sterowanie się posługuje: `rotate`, `forward`, `right`, `WORLD_UP`, `viewMatrix(eye)` ([`camera.md`](camera.md), sekcje od 5.2 do 5.5) |

### 5.2 Sterowanie w skrócie, stałe, pola i akcesory

Sterowanie stoi w `game::NightMazeApp`, a nie w `scene::Camera`: kamera zostaje czystą matematyką bez wejścia i czasu ([`README.md`](README.md), sekcja 3), a o tym, które klawisze i jaka mysz ją poruszają, decyduje gra. Teoria jest w sekcji 2.

**Sterowanie w skrócie:**

| Co robię | Skutek |
|---|---|
| klikam lewym przyciskiem w scenę | kursor zostaje przechwycony (znika), zaczyna działać obrót i ruch |
| ruszam myszą (kursor przechwycony) | kamera się obraca: w prawo i w lewo (yaw), w górę i w dół (pitch) |
| W, S | lot do przodu i do tyłu wzdłuż kierunku patrzenia |
| A, D | lot w lewo i w prawo |
| spacja, lewy Shift | lot pionowo w górę i w dół |
| Escape | oddaje kursor. Drugi Escape zamyka program ([`../core/input.md`](../core/input.md), sekcja 5.7) |

**Stałe i pola** (`NightMazeApp.hpp`):

```cpp
// Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
// is measured in the units of the window size, not in framebuffer pixels, so the same
// hand movement turns the camera equally on a Retina display.
static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;
// Camera speed in metres per second (one world unit is one metre).
static constexpr float DEFAULT_MOVE_SPEED = 3.0F;
```

```cpp
// Camera position before the last fixed step. onRender draws from a point between
// this one and m_camera.position. It starts equal to the camera position, so the
// frames before the first step are drawn from where the camera stands. Declared after
// m_camera, because members are initialized top to bottom.
glm::vec3 m_previousCameraPosition = m_camera.position;

// How the camera is controlled. These belong to the controls, not to the camera.
float m_mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
float m_moveSpeed = DEFAULT_MOVE_SPEED;
```

| Element | Znaczenie |
|---|---|
| `DEFAULT_MOUSE_SENSITIVITY = 0.1F` | czułość startowa: 0,1 stopnia na jednostkę współrzędnych ekranu. Przesunięcie myszy o całą szerokość okna 1280 to obrót o 128 stopni |
| `DEFAULT_MOVE_SPEED = 3.0F` | prędkość startowa: 3 metry na sekundę, czyli szybki marsz. Kostka o boku 1 m jest 3 m od kamery, więc dolot zajmuje sekundę |
| `m_previousCameraPosition` | pozycja kamery sprzed ostatniego kroku symulacji. Para z `m_camera.position` potrzebna do interpolacji |
| `= m_camera.position` | wartość początkowa: **ta sama** co pozycja kamery. Pola są inicjalizowane w kolejności deklaracji, więc to pole musi stać pod `m_camera`. Dzięki temu pierwsze klatki, narysowane przed pierwszym krokiem, mieszają dwie identyczne pozycje i `alpha` nie ma na nie wpływu |
| `m_mouseSensitivity`, `m_moveSpeed` | bieżące ustawienia sterowania. To pola, a nie stałe, bo zmienia je panel Camera. Stałe `DEFAULT_...` są `static constexpr` w klasie, żeby dało się ich użyć jako wartości początkowych pól |

Stałe i pola są prywatne. Panel dostaje je przez trzy chronione akcesory, obok istniejących `clearColor()` i `shader()`:

```cpp
/// The camera, exposed so the debug UI can show and edit its position, angles and
/// projection live.
scene::Camera& camera() { return m_camera; }

/// Mouse look sensitivity in degrees per screen coordinate unit of mouse movement,
/// exposed so the debug UI can edit it live.
float& mouseSensitivity() { return m_mouseSensitivity; }

/// Camera movement speed in metres per second, exposed so the debug UI can edit it live.
float& moveSpeed() { return m_moveSpeed; }
```

Każdy zwraca referencję do jednego pola, więc panel edytuje oryginał, a nie kopię. Są chronione (`protected`): widzi je tylko klasa pochodna, czyli `DebugNightMazeApp` w `main.cpp`, która przekazuje je do `debug::DebugContext` ([`../debug-ui.md`](../debug-ui.md), sekcja 5.5). Gra nadal nie dołącza niczego z `debug/`.

### 5.3 Obrót myszą: początek `onRender`

```cpp
void NightMazeApp::onRender(double alpha) {
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

| Linia | Znaczenie |
|---|---|
| `void NightMazeApp::onRender(double alpha)` | parametr ma już nazwę, bo jest używany (niżej, przy liczeniu oka) |
| `if (!input().isCursorCaptured())` | dwa stany: kursor wolny (czekam na kliknięcie) albo przechwycony (obracam kamerę). Nigdy oba naraz |
| `input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)` | zbocze: prawda tylko w klatce, w której przycisk został wciśnięty. Kliknięcie w panel debug tu nie dociera, bo `main.cpp` blokuje grze mysz, gdy używa jej ImGui ([`../core/input.md`](../core/input.md), sekcja 5.10) |
| `input().setCursorCaptured(true);` | tryb `GLFW_CURSOR_DISABLED`: kursor znika i jego ruch nie jest ograniczony krawędzią ekranu ([`../core/input.md`](../core/input.md), sekcja 5.9) |
| `else` | w klatce kliknięcia kamera się nie obraca: przesunięcie z tej klatki to jeszcze ruch widocznego kursora po ekranie |
| `static_cast<float>(input().mouseDeltaX()) * m_mouseSensitivity` | przesunięcie w poziomie razy czułość daje zmianę yaw w stopniach. `Input` zwraca `double` (tak podaje GLFW), kamera liczy na `float`, stąd jawne rzutowanie |
| `-static_cast<float>(input().mouseDeltaY()) * m_mouseSensitivity` | to samo dla pitch, z minusem: y ekranu rośnie w dół, pitch rośnie w górę (sekcja 2.1) |
| `m_camera.rotate(yawDelta, pitchDelta);` | dodaje zmiany, zawija yaw i przycina pitch ([`camera.md`](camera.md), sekcja 5.4) |

Trzy rzeczy, których w tym kodzie nie widać, a które działają:

1. **Brak szarpnięcia po kliknięciu.** `setCursorCaptured` ustawia w `Input` flagę, przez którą następny odczyt myszy zgłasza zerowe przesunięcie (problem "pierwszej myszy", [`../core/input.md`](../core/input.md), sekcja 5.8). Pierwsza klatka z przechwyconym kursorem woła więc `rotate(0, 0)`.
2. **Escape nie jest tu obsługiwany.** Zwalnia kursor `core::Application::run`, zanim dojdzie do `onRender` ([`../core/main-loop.md`](../core/main-loop.md), sekcja 5.2). W klatce z Escape `isCursorCaptured()` jest już fałszem i kamera się nie obraca.
3. **Brak mnożenia przez czas.** Przesunięcie myszy jest gotową drogą, nie prędkością (sekcja 2.3).

Kod dołącza `<GLFW/glfw3.h>` tylko dla stałych `GLFW_KEY_...` i `GLFW_MOUSE_BUTTON_LEFT`. Żadnej funkcji GLFW nie woła: wszystkie pytania o wejście idą przez `input()`, czyli podlegają blokadzie klawiatury i myszy.

### 5.4 Ruch klawiszami: `onUpdate`

```cpp
void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the camera was before this step. It is done in every step, also
    // when the camera does not move, so that onRender never blends with an old position.
    m_previousCameraPosition = m_camera.position;

    // The camera is controlled only while the cursor is captured: one click in the scene
    // switches on both mouse look and movement, Escape switches both off.
    if (!input().isCursorCaptured()) {
        return;
    }

    // Free flight (there are no collisions yet): W and S move along the view direction,
    // so looking up while holding W also climbs. A and D move sideways, Space and Left
    // Shift move straight up and down. Opposite keys cancel each other.
    const glm::vec3 forward = m_camera.forward();
    const glm::vec3 right = m_camera.right();
    glm::vec3 direction{0.0F};
    if (input().isKeyDown(GLFW_KEY_W)) {
        direction += forward;
    }
    if (input().isKeyDown(GLFW_KEY_S)) {
        direction -= forward;
    }
    if (input().isKeyDown(GLFW_KEY_D)) {
        direction += right;
    }
    if (input().isKeyDown(GLFW_KEY_A)) {
        direction -= right;
    }
    if (input().isKeyDown(GLFW_KEY_SPACE)) {
        direction += scene::Camera::WORLD_UP;
    }
    if (input().isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        direction -= scene::Camera::WORLD_UP;
    }

    // Two keys at once give a vector longer than 1 (about 1.41 for W and D), which would
    // make diagonal movement faster. Normalizing brings the length back to 1. With no key
    // held the vector is zero and must be left alone: normalizing it divides by zero.
    if (glm::length(direction) > 0.0F) {
        direction = glm::normalize(direction);
    }

    // Distance of one step: metres per second times seconds.
    m_camera.position += direction * (m_moveSpeed * static_cast<float>(fixedDt));
}
```

| Linia | Znaczenie |
|---|---|
| `m_previousCameraPosition = m_camera.position;` | **pierwsza linia każdego kroku**, także wtedy, gdy kamera się nie rusza. Po kroku para (poprzednia, bieżąca) opisuje dokładnie ten jeden krok |
| `if (!input().isCursorCaptured()) { return; }` | ruch działa tylko przy przechwyconym kursorze (uzasadnienie niżej). Linia stoi **po** zapamiętaniu poprzedniej pozycji |
| `const glm::vec3 forward = m_camera.forward();` (i `right`) | oba kierunki liczę raz na krok i trzymam w zmiennych lokalnych, bo każdy może być użyty dwa razy |
| `glm::vec3 direction{0.0F};` | suma kierunków zaczyna od wektora zerowego. Jawne `{0.0F}`, bo `glm::vec3 direction;` zostawiłoby przypadkowe wartości |
| `if (input().isKeyDown(GLFW_KEY_W)) { direction += forward; }` (i pięć kolejnych) | `isKeyDown` to stan ciągły, bezpieczny w `onUpdate` ([`../core/input.md`](../core/input.md), sekcja 5.5). Sześć osobnych `if`, bez `else`: klawisze przeciwne znoszą się w sumie, a nie wykluczają |
| `scene::Camera::WORLD_UP` | spacja i Shift przesuwają wzdłuż pionu świata, nie wzdłuż "góry kamery": lot w górę jest pionowy także wtedy, gdy patrzę w dół |
| `if (glm::length(direction) > 0.0F)` | normalizuję tylko wektor niezerowy. `glm::length` to pierwiastek z sumy kwadratów składowych. Dla wektora zerowego `glm::normalize` dałoby `NaN` (sekcja 2.2) |
| `direction = glm::normalize(direction);` | długość wraca do 1: ruch po skosie ma tę samą prędkość co ruch na wprost |
| `m_moveSpeed * static_cast<float>(fixedDt)` | droga jednego kroku w metrach: prędkość razy czas kroku. `fixedDt` to zawsze `Time::FIXED_DT`, nigdy czas zmierzony. Przy 3 m/s wychodzi 0,025 m |
| `m_camera.position += direction * (...)` | przesunięcie o tę drogę w wybranym kierunku. Nawias sprawia, że najpierw mnożone są dwie liczby, a wektor jest mnożony raz |

Przypadek, w którym suma jest prawie zerowa: W razem ze spacją przy patrzeniu pionowo w dół (pitch -89). `forward` to prawie `(0, -1, 0)`, `WORLD_UP` to `(0, 1, 0)`, suma ma długość około 0,017. To nie jest zero, więc zostaje znormalizowana i kamera leci poziomo z pełną prędkością. Zero wychodzi dokładnie tylko wtedy, gdy klawisze się znoszą (W z S, A z D, spacja z Shiftem) albo żaden nie jest wciśnięty: odejmowany jest wtedy ten sam wektor, który został dodany.

**Decyzja: ruch tylko przy przechwyconym kursorze.** Kliknięcie w scenę włącza całe sterowanie kamerą, a Escape całe wyłącza. Powody:

- jedna reguła dla obu urządzeń: albo steruję kamerą (kursor schowany), albo pracuję z panelami (kursor widoczny). Nie ma stanu pośredniego, w którym klawisze działają, a mysz nie,
- przy widocznym kursorze używam paneli, a ImGui blokuje klawiaturę gry tylko na czas, gdy widżet jest aktywny ([`../core/input.md`](../core/input.md), sekcja 5.6). Bez tej reguły przypadkowe W albo spacja naciśnięte między dwoma kliknięciami w panel przesuwałyby kamerę, którą właśnie ustawiam suwakami,
- tak działają gry: wyjście do menu zatrzymuje sterowanie postacią.

Cena: żeby polecieć, trzeba najpierw kliknąć w scenę. Przypomina o tym linia pomocy na górze panelu Camera.

### 5.5 Interpolacja: oko w `onRender`

```cpp
// The simulation moves the camera in fixed steps, and this frame is drawn at some
// moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
// between the position before the last step and the position after it keeps the
// movement smooth at any frame rate. m_camera.position itself is not changed.
const glm::vec3 eye =
    glm::mix(m_previousCameraPosition, m_camera.position, static_cast<float>(alpha));
```

`glm::mix(a, b, t)` zwraca `a * (1 - t) + b * t` ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.8). `alpha` przychodzi jako `double`, a wektory są typu `float`, stąd rzutowanie. Wynik trafia do `m_camera.viewMatrix(eye)` ([`camera.md`](camera.md), sekcja 5.7). Pole `m_camera.position` nie jest zmieniane: rysowanie tylko czyta stan symulacji.

Przypadki brzegowe:

| Sytuacja | Co jest w parze (poprzednia, bieżąca) | Co widać |
|---|---|---|
| pierwsze klatki, przed pierwszym krokiem | dwie identyczne pozycje (inicjalizator pola) | kamera stoi w pozycji startowej, niezależnie od `alpha` |
| klatka bez żadnego kroku (szybki monitor albo wyłączony vsync) | ta sama para co w poprzedniej klatce, `alpha` większe | oko przesuwa się dalej wzdłuż tego samego odcinka: ruch pozostaje płynny (tabela w sekcji 2.4, klatka 4) |
| klatka z kilkoma krokami | para opisuje **ostatni** z nich, bo każdy krok nadpisuje poprzednią pozycję | interpolacja obejmuje ostatni krok, wcześniejsze są już "za" tą klatką |
| kamera stoi (żaden klawisz) | po pierwszym kroku bez ruchu obie pozycje są równe | obraz nieruchomy, `alpha` bez znaczenia |
| kursor zwolniony klawiszem Escape w trakcie lotu | następny krok zapamiętuje pozycję i wraca bez ruchu | kamera dolatuje do pozycji z ostatniego kroku i staje, bez skoku |
| pozycja zmieniona z panelu Camera | panel pisze do `m_camera.position` po narysowaniu sceny. Najbliższy krok kopiuje nową pozycję do poprzedniej | kamera przeskakuje w nowe miejsce i to jest zamierzone. Gdy w następnej klatce wykona się krok (prawie zawsze), przeskok jest natychmiastowy. Gdy trafi się klatka bez kroku, ta jedna klatka jest rysowana z punktu między starą a nową pozycją: zamiast skoku widać jedno pośrednie ujęcie. Trwa to najwyżej jeden krok, czyli 8,33 ms |
| okno zminimalizowane | `onRender` wraca przed liczeniem oka, `onUpdate` działa dalej | po przywróceniu okna kamera jest tam, gdzie doleciała |

### 5.6 Jak sprawdzone zostało sterowanie

Prawdziwych naciśnięć klawiszy i ruchu myszy nie da się wstrzyknąć do GLFW z kodu: `glfwGetKey` i `glfwGetCursorPos` oddają to, co przyszło z systemu. Sprawdzenie ma więc dwie części.

**Część automatyczna.** Tymczasowy program (nie trafił do repozytorium) z klasą pochodną od `game::NightMazeApp` i z własnym `debug::DebugUI`, z ukrytym oknem (`GLFW_VISIBLE` równe `GLFW_FALSE`). Wołał prawdziwe `onUpdate` i `onRender`, zmieniał kamerę przez akcesor `camera()` i odczytywał narysowaną klatkę przez `glReadPixels`. Framebuffer 2560 x 1440, współrzędne y liczone od dołu:

| Sprawdzenie | Wynik |
|---|---|
| pierwsza klatka przed jakimkolwiek krokiem, `alpha` 0 i 0,9 | ten sam obraz: kostka w kolumnach od 977 do 1638, wierszach od 391 do 1004 |
| 10 kroków bez przechwyconego kursora | pozycja bez zmian |
| 10 kroków z przechwyconym kursorem i bez klawiszy (wektor zerowy) | pozycja `(0, 0, 3)`, żadnego `NaN`, yaw i pitch 0, obraz bez zmian |
| kamera w `(0, 0, 2)`, potem w `(0, 0, 6)` | kostka ma 1003 x 980 pikseli, potem 329 x 290 (w pozycji startowej 662 x 614): bliżej znaczy większa |
| kamera przesunięta o 1 m wzdłuż `right()` | kostka w kolumnach od 594 do 1184: przesunęła się w lewo |
| kamera przesunięta o 1 m wzdłuż `WORLD_UP` | kostka w wierszach od 0 do 599: przesunęła się w dół |
| poprzednia pozycja `(0, 0, 3)`, bieżąca `(1, 0, 3)`, `alpha` 0 | obraz identyczny jak z kamery w `(0, 0, 3)` |
| ta sama para, `alpha` 0,5 | obraz identyczny (ten sam prostokąt i ta sama liczba pikseli kostki) jak z kamery stojącej dokładnie w `(0,5, 0, 3)` |
| ta sama para, `alpha` 0,999 | kolumny od 595 do 1184, o jeden piksel od obrazu z kamery w `(1, 0, 3)` |
| `rotate(15, 0)`: obrót w prawo | kostka przesuwa się w lewo (kolumny od 599 do 1303) |
| `rotate(0, 10)`: spojrzenie w górę | kostka przesuwa się w dół (wiersze od 144 do 781) |
| `nearPlane` 2,3, potem 3,0, 3,8 i 3,9 | przy 2,3 obrys kostki bez zmian (ścięty róg odsłania jej wnętrze), przy 3,0 kostka ma 557 x 552 piksele, przy 3,8 zostaje 21 x 31 pikseli, przy 3,9 nie ma jej wcale |
| `farPlane` 3,8, potem 3,0, 2,3 i 2,1 | przy 3,8 obraz bez zmian (ścięty tylny róg jest zasłonięty), przy 3,0 ubywa lewej krawędzi (kolumny od 1026), przy 2,3 zostaje 151 x 225 pikseli, przy 2,1 nie ma jej wcale |
| `fovDegrees` 20, 60 i 120 | wysokość kostki: cały framebuffer (1440), 614 i 205 pikseli |
| okno o rozmiarze 0 x 300 | framebuffer 0 x 600, `onRender` wraca bez asercji i bez błędu OpenGL |
| po każdej klatce `glGetError` | brak błędów |
| ImGui z włączoną myszą, kursor nad panelem | `wantsMouse()` zwraca prawdę. Wciśnięty przycisk aktywuje widżet |
| po `setMouseEnabled(false)`, ten sam kursor | `wantsMouse()` zwraca fałsz już w tym samym `draw`. Wciśnięty i przeciągany przycisk nie aktywuje widżetu i nie zmienia żadnej wartości ([`../debug-ui.md`](../debug-ui.md), sekcja 5.6) |

Zdarzenia myszy w dwóch ostatnich wierszach były podawane wprost do ImGui (`ImGuiIO::AddMousePosEvent`, `AddMouseButtonEvent`), z pominięciem GLFW.

Prawdziwy program uruchomiony na 3 sekundy (Debug) nie wypisuje żadnej linii `[error]`.

**Część ręczna.** Tego, co wymaga człowieka przy myszy i klawiaturze, program testowy nie sprawdził: przechwycenia kursora po kliknięciu, kierunku i płynności obrotu, ruchu klawiszami, zwolnienia kursora klawiszem Escape, zachowania paneli przy przechwyconym kursorze. To jest scenariusz z sekcji 6 i lista kontrolna w [`../../guides/build-windows.md`](../../guides/build-windows.md). Na Windowsie (2026-10-05) kod kompiluje się w MSVC bez ostrzeżeń, program startuje bez linii `[error]`, a panel Camera pokazuje wartości startowe (`Position` 0, 0, 3, `Yaw` 0, `Pitch` 0, `FOV` 60). Samego sterowania (surowy ruch myszy, klawisze, przechwycenie kursora) nikt tam jeszcze nie sprawdził: cała część ręczna listy kontrolnej jest na Windowsie otwarta.

## 6. Panel ImGui

Panel **Camera** jest pokazem tematu 3 (PRD, sekcja 3: "Pozycja/rotacja kamery, FOV"). Kod: [`src/debug/panels/CameraPanel.cpp`](../../../src/debug/panels/CameraPanel.cpp). Jak panel jest podpięty do `DebugUI` i skąd dostaje dane, opisuje [`../debug-ui.md`](../debug-ui.md), sekcje 5.2 i 5.5.

PRD w sekcji 10 wymienia dla panelu Camera: "Pozycja, FOV, czułość myszy, tryb noclip". Trzy pierwsze są. **Trybu noclip nie ma**: noclip znaczy "wyłącz kolizje", a kolizji jeszcze nie ma (M2). Dziś kamera zawsze lata przez wszystko, więc przełącznik nie miałby czego przełączać.

**Panel a przechwycony kursor.** Mysz ma w każdej chwili jednego właściciela. Gdy kursor jest przechwycony, należy do kamery i panele jej nie widzą (`DebugUI::setMouseEnabled(false)`). Gdy kursor jest wolny i używa go ImGui, gra dostaje od `core::Input` odpowiedź "nic się nie dzieje" (blokada myszy), więc przeciąganie suwaka nie obraca kamery, a kliknięcie w panel nie przechwytuje kursora. Mechanizm opisują [`../core/input.md`](../core/input.md), sekcje 5.10 i 5.11, oraz [`../debug-ui.md`](../debug-ui.md), sekcja 5.6. Tutaj jest tylko skutek: krok 10 scenariusza (sekcja 6.3) i pułapka 9.

### 6.1 Kod panelu

```cpp
// How much the position changes for one pixel of dragging, in metres.
constexpr float POSITION_DRAG_SPEED = 0.05F;

// Yaw is kept in the range from 0 to 360 by scene::Camera::rotate.
constexpr float MIN_YAW_DEGREES = 0.0F;
constexpr float MAX_YAW_DEGREES = 360.0F;

// Field of view: 20 is a strong zoom, 120 is very wide. Close to 0 or to 180 the
// projection breaks down.
constexpr float MIN_FOV_DEGREES = 20.0F;
constexpr float MAX_FOV_DEGREES = 120.0F;

// The near plane must stay above 0. The upper limit is far enough to cut into the cube
// from the default camera position.
constexpr float MIN_NEAR_PLANE = 0.01F;
constexpr float MAX_NEAR_PLANE = 10.0F;

// The lower limit is small enough to cut the cube off from behind.
constexpr float MIN_FAR_PLANE = 1.0F;
constexpr float MAX_FAR_PLANE = 200.0F;

// Smallest distance between the two planes. They must never be equal: the projection
// matrix divides by (far - near).
constexpr float MIN_PLANE_DISTANCE = 0.1F;

constexpr float MIN_MOUSE_SENSITIVITY = 0.01F;
constexpr float MAX_MOUSE_SENSITIVITY = 1.0F;

constexpr float MIN_MOVE_SPEED = 0.5F;
constexpr float MAX_MOVE_SPEED = 20.0F;
```

```cpp
void drawCameraPanel(scene::Camera& camera, float& mouseSensitivity, float& moveSpeed) {
    if (ImGui::Begin("Camera")) {
        ImGui::TextWrapped("Click the scene to capture the mouse, Esc releases it. While "
                           "captured: mouse looks around, W A S D move, Space goes up, Left "
                           "Shift goes down.");

        ImGui::Separator();
        // DragFloat3 edits three floats through the pointer: x, y and z of the position.
        // It has no limits, the camera may stand anywhere.
        ImGui::DragFloat3("Position", glm::value_ptr(camera.position), POSITION_DRAG_SPEED);

        // A slider can also be typed into (Ctrl and click), and a typed value may be
        // outside the limits. AlwaysClamp forces it back between them. It matters most
        // for pitch: the field is public and only Camera::rotate clamps it, and a pitch of
        // 90 degrees breaks the view matrix.
        ImGui::SliderFloat("Yaw", &camera.yawDegrees, MIN_YAW_DEGREES, MAX_YAW_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Pitch", &camera.pitchDegrees, -scene::Camera::MAX_PITCH_DEGREES,
                           scene::Camera::MAX_PITCH_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();
        ImGui::SliderFloat("FOV", &camera.fovDegrees, MIN_FOV_DEGREES, MAX_FOV_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
        // Logarithmic: half of the slider covers the small values, where a change of the
        // near plane matters most.
        ImGui::SliderFloat("Near plane", &camera.nearPlane, MIN_NEAR_PLANE, MAX_NEAR_PLANE,
                           "%.2f m", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Far plane", &camera.farPlane, MIN_FAR_PLANE, MAX_FAR_PLANE, "%.1f m",
                           ImGuiSliderFlags_AlwaysClamp);
        // The ranges of the two sliders overlap, so keep the far plane behind the near one.
        camera.farPlane = std::max(camera.farPlane, camera.nearPlane + MIN_PLANE_DISTANCE);

        ImGui::Separator();
        ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, MIN_MOUSE_SENSITIVITY,
                           MAX_MOUSE_SENSITIVITY, "%.2f deg/unit", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Move speed", &moveSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED, "%.1f m/s",
                           ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::End();
}
```

| Element | Znaczenie |
|---|---|
| stałe `MIN_...` i `MAX_...` | granice suwaków, nazwane i opisane w jednym miejscu, w anonimowej przestrzeni nazw pliku. Bez nich w wywołaniach stałyby gołe liczby |
| `drawCameraPanel(scene::Camera& camera, float& mouseSensitivity, float& moveSpeed)` | panel dostaje dokładnie to, co edytuje: trzy referencje bez `const`. Z sygnatury widać, że wszystko jest edytowalne |
| `ImGui::TextWrapped(...)` | linia pomocy tylko do odczytu: jak przechwycić kursor i czym się steruje. Dwa sąsiednie napisy w cudzysłowach kompilator skleja w jeden |
| `ImGui::DragFloat3("Position", glm::value_ptr(camera.position), POSITION_DRAG_SPEED)` | trzy pola przeciągane myszą. `glm::value_ptr` daje wskaźnik na pierwszą składową wektora ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.9), a ImGui czyta i zapisuje przez niego x, y i z. Bez granic: kamera może stać gdziekolwiek |
| `ImGui::SliderFloat("Yaw", &camera.yawDegrees, MIN_YAW_DEGREES, MAX_YAW_DEGREES, "%.1f deg", ...)` | suwak od 0 do 360, czyli zakres, w którym trzyma yaw `Camera::rotate`. `"%.1f deg"` to format wyświetlanej wartości |
| `ImGui::SliderFloat("Pitch", ..., -scene::Camera::MAX_PITCH_DEGREES, scene::Camera::MAX_PITCH_DEGREES, ...)` | suwak od -89 do 89: granice wzięte ze stałej kamery, a nie wpisane drugi raz |
| `ImGuiSliderFlags_AlwaysClamp` | suwak ImGui pozwala wpisać wartość z klawiatury (Ctrl i kliknięcie), a wpisana liczba domyślnie może wyjść poza granice. Ta flaga przycina ją do zakresu. Bez niej dałoby się wpisać pitch 90 i zepsuć macierz widoku ([`camera.md`](camera.md), pułapka 3) |
| `ImGuiSliderFlags_Logarithmic` przy `Near plane` | skala logarytmiczna: połowa długości suwaka przypada na małe wartości, gdzie zmiana bliskiej płaszczyzny ma największe znaczenie. `\|` łączy dwie flagi w jedną maskę |
| `camera.farPlane = std::max(camera.farPlane, camera.nearPlane + MIN_PLANE_DISTANCE);` | zakresy suwaków `Near plane` (do 10) i `Far plane` (od 1) zachodzą na siebie. Ta linia pilnuje, żeby daleka płaszczyzna była zawsze co najmniej 0,1 m za bliską: macierz rzutowania dzieli przez `far - near` ([`camera.md`](camera.md), sekcja 2.3) |
| `ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, ...)`, `"Move speed"` | ustawienia sterowania: pola `NightMazeApp`, nie kamery |

Panel nie ma zmiennych `static` ani globalnych i nie woła żadnej funkcji `gl*`. Zmienia tylko liczby, a skutek widać w następnej klatce, gdy `onRender` policzy z nich macierze. Nie ma też przycisku "zastosuj": w trybie natychmiastowym suwak pisze do pola w chwili przeciągania.

### 6.2 Kontrolki i czego uczą

| Kontrolka | Zakres | Co zmienia | Czego uczy |
|---|---|---|---|
| linia pomocy | odczyt | nic | jak wejść w sterowanie i jak z niego wyjść |
| `Position` | bez granic | `camera.position` | macierz widoku przesuwa świat przeciwnie do kamery: kamera w prawo, kostka na ekranie w lewo. Podczas lotu liczby zmieniają się same |
| `Yaw` | od 0 do 360 stopni | `camera.yawDegrees` | kompas: 0 to -Z, 90 to +X. Przy obrocie myszą w prawo wartość rośnie i po 360 wraca do 0 (zawijanie w `rotate`) |
| `Pitch` | od -89 do 89 stopni | `camera.pitchDegrees` | ograniczenie pitch: suwak i mysz zatrzymują się na 89, żeby kierunek patrzenia nie stał się równoległy do pionu ([`camera.md`](camera.md), sekcja 2.2) |
| `FOV` | od 20 do 120 stopni | `camera.fovDegrees` | kąt widzenia jako zoom: mały kąt powiększa (teleobiektyw), duży pokazuje szeroko i rozciąga obraz przy krawędziach. Kamera się przy tym nie rusza |
| `Near plane` | od 0,01 do 10 m, skala logarytmiczna | `camera.nearPlane` | bliska płaszczyzna obcina wszystko, co bliżej: przy wartości większej niż odległość do kostki widać jej wnętrze, a potem kostka znika |
| `Far plane` | od 1 do 200 m | `camera.farPlane` | daleka płaszczyzna obcina wszystko, co dalej: przy małej wartości kostka znika od tyłu |
| `Mouse sensitivity` | od 0,01 do 1 stopnia na jednostkę | `m_mouseSensitivity` | czułość to tylko mnożnik między ruchem myszy a kątem |
| `Move speed` | od 0,5 do 20 m/s | `m_moveSpeed` | prędkość w metrach na sekundę, niezależna od FPS dzięki stałemu krokowi |

Granice FOV: przy 20 stopniach kostka oglądana z pozycji startowej wypełnia już całą wysokość okna, a przy kącie zbliżonym do 180 `tan(fov / 2)` rośnie do nieskończoności i rzutowanie przestaje mieć sens. Bliska płaszczyzna nie schodzi poniżej 0,01, bo zero daje błędną macierz ([`camera.md`](camera.md), pułapka 4).

### 6.3 Scenariusz pokazu na obronie

Program uruchomiony przez `make run`, panele widoczne, panel Camera zadokowany przy krawędzi.

1. **Stan startowy.** Odczytuję z panelu: `Position` to `(0, 0, 3)`, `Yaw` 0, `Pitch` 0, `FOV` 60. Kamera patrzy wzdłuż -Z na kostkę w początku układu.
2. **Przechwycenie.** Klikam w scenę. Kursor znika. Mówię: tryb `GLFW_CURSOR_DISABLED`, od tej chwili mysz należy do kamery, a panele jej nie widzą.
3. **Obrót.** Ruszam myszą w prawo: obraz ucieka w lewo, `Yaw` rośnie. Ruszam do góry: `Pitch` rośnie i zatrzymuje się na 89. Wyjaśniam minus przy y i ograniczenie pitch.
4. **Lot.** Trzymam W: kostka rośnie, w `Position` maleje z. Dodaję D: lot po skosie z tą samą prędkością (normalizacja). Spacja i Shift: góra i dół wzdłuż pionu świata. Oblatuję kostkę dookoła i pokazuję trzy ściany, których z pozycji startowej nie było widać (tylną zieloną, prawą żółtą, dolną purpurową).
5. **Lot tam, gdzie patrzę.** Patrzę w górę i trzymam W: kamera się wznosi. Mówię, że to kamera latająca i co trzeba by zmienić dla chodzenia.
6. **Zwolnienie.** Naciskam Escape: kursor wraca, program działa dalej. Mówię, że drugi Escape zamknąłby program.
7. **FOV jako zoom.** Przeciągam `FOV` od 60 do 20, potem do 120. `Position` się nie zmienia, a kostka rośnie i maleje.
8. **Bliska płaszczyzna.** Ustawiam kamerę w pozycji startowej (`Position` na `(0, 0, 3)`, `Yaw` i `Pitch` na 0) i przesuwam `Near plane` w górę. Powyżej 2,17 m płaszczyzna zaczyna ciąć najbliższy róg kostki: w wyciętej dziurze widać od środka ściany tylną, prawą i dolną (zieloną, żółtą, purpurową), bo ścian zwróconych tyłem nikt nie odrzuca. Przy 3 m kostka jest już wyraźnie obcięta, powyżej 3,83 m znika cała. Wracam do 0,1.
9. **Daleka płaszczyzna.** Przesuwam `Far plane` w dół. Poniżej 3,83 m płaszczyzna zaczyna ciąć kostkę od tyłu. Najpierw znika tylny róg, którego z tej strony i tak nie widać, przy 3 m ubytek jest już wyraźny, a poniżej 2,17 m kostki nie ma wcale. Wracam do 100.
10. **Panele a kamera.** Przeciągam dowolny suwak: kamera się nie obraca i kursor nie zostaje przechwycony, bo mysz ma ImGui.
11. **Czułość i prędkość.** Ustawiam `Mouse sensitivity` na 0,5, klikam w scenę i pokazuję, że ten sam ruch ręki obraca pięć razy mocniej. To samo z `Move speed`.

Liczby w krokach 8 i 9 wynikają z geometrii: kostka o boku 1 m obrócona o 25 i 35 stopni sięga w głąb od 0,83 m przed swoim środkiem (róg najbliższy kamery) do 0,83 m za nim, a kamera stoi 3 m od środka: kostka zajmuje odległości od 2,17 do 3,83 m. Sprawdzone pomiarem (sekcja 5.6).

## 7. Pułapki

1. **Kierunek, który nie jest jednostkowy.** Ruch liczony jako `kierunek * prędkość * czas` zakłada długość 1. Dwa typowe błędy: użycie `cross(forward, WORLD_UP)` bez normalizacji (przy patrzeniu w górę chodzenie bokiem zwalnia, bo długość to `cos(pitch)`) oraz zsumowanie `forward()` i `right()` przy ruchu po skosie (długość około 1,41, czyli ruch po skosie szybszy o 41 procent). Sumę kierunków trzeba znormalizować, ale tylko wtedy, gdy nie jest wektorem zerowym. Tak robi `NightMazeApp::onUpdate` (sekcja 5.4).
2. **Przesunięcie myszy użyte w `onUpdate`.** `mouseDeltaX` i `mouseDeltaY` opisują jedną klatkę, a `onUpdate` wykonuje się od zera do wielu razy na klatkę. Obrót liczony w `onUpdate` gubi ruch myszy w klatkach bez kroku i liczy go kilka razy w klatkach z kilkoma krokami: czułość zależy od FPS. To samo dotyczy kliknięcia (`wasMouseButtonPressed`). Oba stoją w `onRender`.
3. **Przesunięcie myszy pomnożone przez czas.** Odruch "wszystko razy `dt`" jest tu błędem. Przesunięcie myszy to droga, a nie prędkość: jest już proporcjonalne do czasu klatki, bo w dłuższej klatce ręka zdążyła przesunąć mysz dalej. Pomnożone przez `dt` dałoby obrót zależny od FPS.
4. **Normalizacja wektora zerowego.** `glm::normalize(glm::vec3(0.0F))` dzieli zero przez zero i zwraca `NaN` w każdej składowej. `NaN` dodany do pozycji zostaje w niej na zawsze, macierz widoku wypełnia się `NaN` i obraz znika po pierwszej klatce, w której żaden klawisz nie był wciśnięty. Stąd warunek `glm::length(direction) > 0.0F`.
5. **W unosi kamerę przy patrzeniu w górę.** Ruch wzdłuż `forward()` ma składową pionową. Dla kamery latającej to zamierzone, dla chodzenia błąd: postać odrywałaby się od podłogi przy spojrzeniu w górę i zwalniała w poziomie. Chodzenie wymaga kierunku zrzutowanego na poziom (ćwiczenie 1).
6. **Skok kamery przy pierwszym ruchu myszy.** Pierwsze przesunięcie po przechwyceniu kursora liczone względem starej pozycji kursora dałoby jeden wielki obrót. Chroni przed tym `core::Input` (flaga `m_skipNextMouseDelta`), a kod kamery dodatkowo nie obraca w klatce kliknięcia (gałąź `else`). Kto pisze własne odczytywanie myszy, musi pierwszy odczyt pominąć sam.
7. **Zapomniana pozycja poprzednia.** `m_previousCameraPosition` musi być zapisywane na początku **każdego** kroku, przed ruchem i przed wczesnym `return`. Zapisane tylko wtedy, gdy kamera się rusza, zostawia po zatrzymaniu starą wartość: obraz drga między dwiema pozycjami w rytmie `alpha`. Zapisane po ruchu sprawia, że obie pozycje są zawsze równe i interpolacja nic nie robi.
8. **Pozycja poprzednia różna od bieżącej na starcie.** Gdyby `m_previousCameraPosition` startowało od zera, pierwsze klatki (przed pierwszym krokiem) byłyby rysowane z punktu między `(0, 0, 0)` a `(0, 0, 3)`, czyli z wnętrza kostki. Pole jest inicjalizowane pozycją kamery. Z tego samego powodu pozycji startowej nie zmieniam w ciele konstruktora samym przypisaniem do `m_camera.position`: trzeba by ustawić oba pola.
9. **Obrót myszą, gdy kursor jest nad panelem.** Bez blokady myszy przeciąganie suwaka w panelu obracałoby kamerę, a kliknięcie w panel przechwytywałoby kursor. Gra pyta o mysz wyłącznie przez `input()`, a `main.cpp` blokuje te odpowiedzi, gdy myszy używa ImGui. W drugą stronę działa `DebugUI::setMouseEnabled`: przy przechwyconym kursorze panele nie widzą myszy ([`../debug-ui.md`](../debug-ui.md), sekcja 5.6).
10. **Interpolacja kątów "przy okazji".** Yaw zawija się z 359 do 0. Zwykłe `mix(359, 1, 0,5)` daje 180, czyli obrót w przeciwną stronę. W projekcie kąty nie są interpolowane (zmieniają się raz na klatkę), ale kto przeniesie obrót do `onUpdate`, trafi na ten problem.

## 8. Ćwiczenia

Wszystkie ćwiczenia dotyczą sterowania kamerą (sekcje 2 i 5) i są zmianami w działającym programie: zmiana w `NightMazeApp.cpp` albo w `CameraPanel.cpp` wymaga zbudowania (`make run`). Po każdym ćwiczeniu wycofaj zmianę (`git checkout src assets/shaders`).

1. **Chodzenie zamiast lotu.** W `onUpdate` zastąp `forward` kierunkiem poziomym: weź `m_camera.forward()`, wyzeruj składową `y` i znormalizuj wynik (dlaczego jest to bezpieczne przy ograniczeniu pitch do 89 stopni?). Zakomentuj obsługę spacji i Shifta. Patrz w górę i trzymaj W: czy wysokość w `Position` się zmienia? Czy prędkość w poziomie zależy teraz od pitch?
2. **Sprint.** Dodaj stałą `SPRINT_MULTIPLIER = 2.0F` i mnóż przez nią drogę kroku, gdy wciśnięty jest lewy Ctrl (`GLFW_KEY_LEFT_CONTROL`). Dlaczego mnożnik stoi po normalizacji, a nie przed nią?
3. **Odwrócona oś y.** Usuń minus w linii liczącej `pitchDelta`. Co się zmieniło? Dodaj do `NightMazeApp` pole `bool m_invertMouseY`, doprowadź je do panelu Camera jako `ImGui::Checkbox` (pięć miejsc z [`../debug-ui.md`](../debug-ui.md), sekcja 5.5) i wybieraj znak na jego podstawie.
4. **Przechylenie (roll).** `Camera::viewMatrix` podaje do `lookAt` zawsze `WORLD_UP`, więc kamera się nie przechyla. Dodaj tymczasowo pole `rollDegrees` i obróć wektor góry wokół `forward()` (`glm::rotate(glm::mat4(1.0F), glm::radians(rollDegrees), forward())` razy `vec4(WORLD_UP, 0.0F)`). Dlaczego `right()` przestaje wtedy pasować do obrazu i co trzeba by zmienić w ruchu na boki?
5. **Kamera orbitalna.** Zamiast latać, krąż wokół kostki: w `onRender` policz oko jako `-forward() * promień` (punkt na sferze wokół początku układu) i podaj je do `viewMatrix`. Mysz zmienia wtedy kąt, pod którym oglądasz kostkę, a kostka zostaje na środku. Czym różni się to od kamery FPS, skoro obie używają tych samych dwóch kątów?
6. **Zoom klawiszem.** W `onUpdate` zmniejszaj `m_camera.fovDegrees` z prędkością 60 stopni na sekundę, gdy trzymany jest klawisz Z, i zwiększaj z tą samą prędkością, gdy puszczony, w granicach od 20 do 60. Dlaczego to jest zmiana rzutowania, a nie pozycji? Co musiałoby się stać z czułością myszy, żeby celowanie w przybliżeniu było równie wygodne?
7. **Przesunięcie myszy w złym miejscu.** Przenieś blok obrotu z `onRender` do `onUpdate` (pułapka 2). Porównaj czułość przy włączonym i wyłączonym vsync (ćwiczenie 1 w [`../core/window-context.md`](../core/window-context.md)). Wycofaj zmianę.
8. **Bez interpolacji.** Zamień w `onRender` argument `viewMatrix(eye)` na `viewMatrix(m_camera.position)` i ustaw `Move speed` na 20. Leć bokiem (D) obok kostki i obserwuj jej krawędź, najlepiej na monitorze o odświeżaniu innym niż 60 albo 120 Hz. Potem zamień `static_cast<float>(alpha)` na stałą `0.0F`: dlaczego obraz jest wtedy płynny, ale spóźniony o cały krok?

## 9. Pytania kontrolne

1. **Jak ruch myszy zamienia się w obrót kamery?**
   Raz na klatkę, w `onRender`, gdy kursor jest przechwycony: `yawDelta = mouseDeltaX * czułość`, `pitchDelta = -mouseDeltaY * czułość`, potem `m_camera.rotate(yawDelta, pitchDelta)`. Czułość to stopnie na jednostkę współrzędnych ekranu (domyślnie 0,1). `rotate` zawija yaw i przycina pitch.

2. **Skąd minus przy `mouseDeltaY`?**
   Współrzędna y ekranu rośnie w dół, a pitch rośnie przy patrzeniu w górę. Ruch myszy do góry daje ujemne przesunięcie, więc znak trzeba odwrócić. Przy x znaku nie zmieniam, bo ruch w prawo jest dodatni i dodatni yaw obraca w prawo.

3. **Dlaczego obrót jest w `onRender`, a ruch w `onUpdate`?**
   Przesunięcie myszy to dane jednej klatki: trzeba je zastosować dokładnie raz, a `onUpdate` wykonuje się od zera do wielu razy na klatkę. Ruch zależy od czasu trzymania klawisza, a czas symulacji płynie stałymi krokami: `isKeyDown` jest stanem ciągłym, bezpiecznym w każdym kroku, a droga `prędkość * fixedDt` nie zależy od FPS.

4. **Jak powstaje kierunek ruchu i po co normalizacja?**
   Każdy wciśnięty klawisz dodaje albo odejmuje jeden z trzech wektorów jednostkowych: `forward()`, `right()` albo `WORLD_UP`. Suma dwóch prostopadłych ma długość około 1,41, więc bez normalizacji ruch po skosie byłby o 41 procent szybszy. Normalizuję tylko wektor niezerowy, bo normalizacja zera daje `NaN`.

5. **Co się stanie, gdy patrzę w górę i trzymam W?**
   Kamera się wznosi, bo W przesuwa wzdłuż kierunku patrzenia, który ma składową pionową `sin(pitch)`. To zamierzone: do M2 kamera lata. Chodzenie wymagałoby wyzerowania składowej y kierunku i ponownej normalizacji.

6. **Po co `m_previousCameraPosition` i jak działa interpolacja?**
   Pozycja zmienia się co krok symulacji (8,33 ms), a klatka wypada między krokami. Na początku każdego kroku zapamiętuję pozycję sprzed kroku. `onRender` rysuje z `glm::mix(poprzednia, bieżąca, alpha)`, gdzie `alpha` z zakresu od 0 do 1 mówi, jak daleko klatka jest między krokami. Oko przesuwa się wtedy w każdej klatce o tyle samo, także w klatce bez kroku. Ceną jest opóźnienie obrazu o najwyżej jeden krok.

7. **Co widać, gdy pozycję kamery zmienię z panelu?**
   Kamera przeskakuje. Panel pisze do `m_camera.position`, a najbliższy krok kopiuje tę wartość do pozycji poprzedniej. Jeśli między zmianą a krokiem trafi się klatka bez kroku, ta jedna klatka jest rysowana z punktu pośredniego między starą a nową pozycją.

8. **Dlaczego ruch działa tylko przy przechwyconym kursorze?**
   Żeby była jedna reguła: kliknięcie w scenę włącza całe sterowanie kamerą (mysz i klawisze), Escape całe wyłącza. Przy widocznym kursorze pracuję z panelami i przypadkowe naciśnięcie W nie powinno przesuwać kamery ustawianej suwakami.

9. **Co pokazuje panel Camera i dlaczego suwak `Pitch` ma flagę `ImGuiSliderFlags_AlwaysClamp`?**
   Pozycję, yaw, pitch, FOV, bliską i daleką płaszczyznę, czułość myszy i prędkość ruchu, wszystko edytowalne przez referencje. Pole `pitchDegrees` jest publiczne i zakresu pilnuje tylko `Camera::rotate`, a suwak ImGui pozwala wpisać liczbę z klawiatury. Flaga przycina wpisaną wartość do granic suwaka, czyli do `MAX_PITCH_DEGREES`. Trybu noclip z PRD nie ma, bo nie ma jeszcze kolizji.

10. **Co się dzieje z obrazem, gdy zwiększam `Near plane` powyżej odległości do kostki, i dlaczego FOV działa jak zoom?**
    Bliska płaszczyzna obcina wszystko, co bliżej kamery: najpierw znika najbliższy róg i widać wnętrze kostki, potem cała kostka. FOV zmienia współczynnik `1 / tan(fov / 2)`, przez który macierz rzutowania mnoży x i y: mniejszy kąt to większy współczynnik i większy obraz, bez ruchu kamery.

## 10. Źródła

- Glenn Fiedler, "Fix Your Timestep!", Gaffer on Games: <https://gafferongames.com/post/fix_your_timestep/> (stały krok i interpolacja stanu z `alpha`, sekcja 2.4).
- LearnOpenGL, rozdział "Camera": <https://learnopengl.com/Getting-started/Camera> (część "Walk around" i "Look around": ruch klawiszami, obrót myszą, czułość).
- Dokumenty w tym repozytorium: [`camera.md`](camera.md) (struktura `Camera`), [`transforms.md`](transforms.md) (przestrzenie i macierz modelu), [`../core/main-loop.md`](../core/main-loop.md) (stały krok i `alpha`), [`../core/input.md`](../core/input.md) (mysz, przechwycenie kursora, blokady), [`../debug-ui.md`](../debug-ui.md) (podpięcie panelu Camera, `setMouseEnabled`), [`../../libraries/glm.md`](../../libraries/glm.md) (`mix`, `normalize`, `value_ptr`), [`../../libraries/imgui.md`](../../libraries/imgui.md) (biblioteka Dear ImGui).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)), sekcja 10: lista paneli (panel Camera).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)), sekcja 3: temat 3 i jego pokaz w ImGui.
