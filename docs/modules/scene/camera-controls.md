# Moduł scene: sterowanie kamerą i panel Camera

Kamień milowy: M1, zmienione w M2 + M3 (kamera podąża za graczem) w M4 (oko i kierunek patrzenia ustawiają też latarkę, panel Camera startuje zwinięty) i w M5 (klawisz R przywraca kamerze pozę startową, panel Camera stoi obok panelu Gameplay). Temat wykładu: 3 (Przekształcenia przestrzeni).
Kod: sterowanie w [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp) i [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp), panel w [`src/debug/panels/CameraPanel.hpp`](../../../src/debug/panels/CameraPanel.hpp) i [`src/debug/panels/CameraPanel.cpp`](../../../src/debug/panels/CameraPanel.cpp), podpięcie w [`src/main.cpp`](../../../src/main.cpp).

Część modułu `scene`. Wstęp do modułu i jego miejsce w warstwach są w [`README.md`](README.md). Pozostałe części: [`transforms.md`](transforms.md) (przestrzenie współrzędnych i macierz modelu) i [`camera.md`](camera.md) (macierz widoku, rzutowanie, struktura `Camera`). Ten dokument zakłada znajomość [`camera.md`](camera.md) (kąty yaw i pitch, `forward()`, `right()`, `rotate()`, `viewMatrix(eye)`) i korzysta z trzech dokumentów modułu `core`: [`../core/input.md`](../core/input.md) (mysz, przechwycenie kursora, blokady), [`../core/main-loop.md`](../core/main-loop.md) (stały krok i `alpha`) oraz z [`../debug-ui.md`](../debug-ui.md) (jak panel jest podpięty). Ruch gracza, za którym kamera podąża, opisuje [`../game/player.md`](../game/player.md).

## 1. Po co to jest

Kamera z [`camera.md`](camera.md) to same liczby: pozycja, dwa kąty i parametry rzutowania. Sama z siebie stoi w miejscu. Ten dokument opisuje, kto i kiedy te liczby zmienia.

W kamieniu milowym M1 kamerą sterowało się wprost: mysz ją obracała, a klawisze przesuwały ją w locie. Od M2 + M3 podział jest inny:

| Liczby kamery | Kto je zmienia | Gdzie to jest opisane |
|---|---|---|
| kąty yaw i pitch | mysz, raz na klatkę, w `NightMazeApp::onRender` | tutaj, sekcje 2.1, 2.3 i 5.3 |
| pozycja | nikt wprost: po każdym kroku symulacji kamera staje w oczach gracza (`game::Player`) | [`../game/player.md`](../game/player.md) |
| punkt, z którego rysowana jest klatka | interpolacja stóp gracza z `alpha` plus wysokość oczu | tutaj, sekcje 2.4 i 5.5, oraz [`../game/player.md`](../game/player.md), sekcja 5 |
| FOV, bliska i daleka płaszczyzna | tylko panel Camera | tutaj, sekcja 6 |

Dawny lot kamery nie zniknął: stał się trybem noclip gracza (klawisz N). Sterowanie należy do `game::NightMazeApp` i do `game::Player`, a nie do `scene::Camera`. Drugim sposobem zmiany tych samych liczb jest panel Camera z `debug/`: pokaz tematu 3 na obronie.

Stan na dziś: program startuje z graczem w środku komórki (0, 0) labiryntu, oko w `(1; 1,824; 1)` (1,7 m nad gruntem, który ma na starcie 0,124 m przy domyślnej skali wysokości terenu), pitch 0, yaw w stronę otwartego boku komórki startowej (dla ziarna 1: 180, południe). Tę pozę ustawia `beginRound`, a od M5 wraca do niej także klawisz R, który zaczyna rundę od nowa na tym samym labiryncie (sekcja 5.2). Kliknięcie w scenę przechwytuje kursor, mysz obraca kamerę, klawisze poruszają gracza, Escape oddaje kursor. Od M4 scena jest nocna i oświetlona, a oko i kierunek patrzenia kamery mają drugiego odbiorcę: w tym samym punkcie i w tym samym kierunku świeci latarka gracza (sekcja 5.5). Panel Camera przy pierwszym uruchomieniu jest zwinięty do paska tytułu i od M5 stoi obok zwiniętego panelu Gameplay (sekcja 6). Co z tego jest sprawdzone, mówi sekcja 5.6: build i testy na Windowsie przechodzą, obraz był oglądany na zrzutach ekranu, a obrotu myszą, klawiszy i panelu nikt jeszcze nie sprawdził ręcznie.

## 2. Teoria

Z [`camera.md`](camera.md), sekcja 2.2, wiadomo, jak z dwóch kątów powstaje kierunek patrzenia. Ta sekcja mówi, skąd biorą się same kąty i pozycja: z myszy, z gracza i z upływu czasu.

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

### 2.2 Klawiatura porusza gracza, nie kamerę

Klawisze W, A, S, D, spacja i lewy Shift nie zmieniają już pozycji kamery. Trafiają do gracza jako struktura `game::PlayerInput`, a gracz przesuwa się o jeden stały krok w funkcji `Player::update`. Pozycja kamery jest skutkiem: po kroku kamera dostaje pozycję oczu gracza. Całość, razem z normalizacją kierunku i z kolizjami, opisuje [`../game/player.md`](../game/player.md), sekcje 2 i 5.

Kąty kamery wracają jednak do ruchu jako dane wejściowe. Gracz idzie "do przodu" względem tego, gdzie patrzy kamera:

| Tryb gracza | Których kątów używa ruch | Skutek |
|---|---|---|
| chodzenie | tylko yaw (pitch jest zastąpiony zerem) | W prowadzi poziomo w stronę, w którą patrzę, także gdy patrzę w ziemię albo w niebo |
| noclip (klawisz N) | yaw i pitch | W prowadzi dokładnie tam, gdzie patrzę: ze wzrokiem podniesionym o 30 stopni połowa prędkości idzie w górę (`sin(30 stopni) = 0,5`) |

Tryb noclip to lot kamery z M1 bez zmian w zachowaniu: W i S wzdłuż `forward()`, A i D wzdłuż `right()`, spacja i lewy Shift wzdłuż `WORLD_UP`.

### 2.3 Obrót raz na klatkę, ruch stałym krokiem

**Dlaczego obrót raz na klatkę, a ruch stałym krokiem.** To dwa różne rodzaje danych wejściowych:

| | Obrót myszą | Ruch klawiszami |
|---|---|---|
| Rodzaj danych | przesunięcie myszy: wartość opisująca **jedną klatkę** | stan klawisza: "jest wciśnięty", taki sam przez całą klatkę |
| Od czego zależy wynik | tylko od drogi, którą przebyła mysz | od **czasu** trzymania klawisza |
| Gdzie w pętli | `onRender`, dokładnie raz na klatkę | `onUpdate`, stałym krokiem `FIXED_DT` |

Przesunięcie myszy to gotowa wielkość: ręka przesunęła mysz o tyle i kamera ma się obrócić o tyle razy czułość, niezależnie od tego, ile trwała klatka. Nie mnożę jej przez czas. Trzeba ją tylko zastosować **dokładnie raz**. `onUpdate` wykonuje się od zera do wielu razy na klatkę ([`../core/main-loop.md`](../core/main-loop.md), sekcja 2.2): w klatce bez kroku ruch myszy by przepadł, a w klatce z trzema krokami zostałby dodany trzy razy, więc czułość zależałaby od FPS ([`../core/input.md`](../core/input.md), sekcja 2.8). Ruch klawiszami zależy od czasu, a czas symulacji płynie właśnie stałymi krokami: 120 kroków po `prędkość / 120` metra daje dokładnie `prędkość` metrów na sekundę przy każdym FPS. Stałego kroku wymagają też kolizje ([`collision.md`](collision.md), sekcja 2.8).

Ta sama reguła dotyczy klawisza N: przełączenie trybu to zdarzenie jednej klatki (`wasKeyPressed`), więc stoi w `onRender`, obok obrotu myszą. Tuż pod nim, z tego samego powodu, czytany jest klawisz F, który włącza i wyłącza latarkę ([`../game/flashlight.md`](../game/flashlight.md)), a tuż nad nim klawisz R, który zaczyna rundę od nowa (sekcja 5.2).

### 2.4 Interpolacja z `alpha`

**Interpolacja z `alpha`.** Skoro pozycja gracza zmienia się tylko w krokach symulacji (co 8,33 ms), a klatki są rysowane we własnym rytmie (na przykład co 6 ms), to klatka wypada zwykle **między** dwoma krokami. Rysowanie zawsze z ostatniej policzonej pozycji daje szarpanie: jedne klatki pokazują postęp o jeden krok, a inne o zero albo o dwa. Rozwiązanie: pamiętam pozycję stóp sprzed ostatniego kroku i rysuję z punktu leżącego między nią a pozycją bieżącą, w proporcji `alpha` ([`../core/main-loop.md`](../core/main-loop.md), sekcja 2.4). Oko jest o stałą wysokość nad tym punktem:

```text
stopy = poprzednia * (1 - alpha) + bieżąca * alpha        czyli glm::mix(poprzednia, bieżąca, alpha)
oko   = stopy + (0, 1,7, 0)
```

```text
kroki symulacji (co 8,33 ms):   k0          k1          k2          k3
                                 |-----------|-----------|-----------|-----> czas
klatki (co 6 ms):                      K1      K2    K3      K4    K5

K3 wypada po kroku k2: w akumulatorze została reszta 1,33 ms, alpha = 1,33 / 8,33 = 0,16
poprzednia = pozycja po kroku k1,  bieżąca = pozycja po kroku k2
stopy      = punkt w 16 procentach drogi od poprzedniej do bieżącej
```

Przykład na liczbach. Gracz idzie w kierunku +X z prędkością 3 m/s, więc jeden krok to `3 / 120 = 0,025` m. Klatki trwają po 6 ms (około 167 FPS). Przed pierwszą klatką z tabeli gracz jest w x = 1,000, a przed ostatnim krokiem był w x = 0,975:

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

Sterowanie i panel nie mają shaderów. Pola kamery, które zmieniają, trafiają do shaderów jako macierz widoku i macierz rzutowania. Obie są liczone raz na klatkę i podawane każdemu programowi, którym ta klatka rysuje, pod tymi samymi nazwami `uView` i `uProjection` ([`../gfx/uniforms.md`](../gfx/uniforms.md), sekcja 5). Programów jest od pierwszej części M6 pięć (`textured`, `color`, `lit`, `gouraud`, `skybox`), a w jednej klatce pracują najwyżej trzy z nich: jeden z trójki `textured`, `lit`, `gouraud` dla labiryntu, bramy i kryształów, `color` dla linii kształtów kolizji i `skybox` dla nieba ([`camera.md`](camera.md), sekcja 5.7). To zdanie opisuje stan z pierwszej części M6. Od drugiej części M6 macierze kamery dostaje też program `grass`, więc programów z macierzami kamery jest sześć, a w klatce pracują najwyżej cztery. Od czwartej części M7 program `shadow_depth` ma te same dwie nazwy uniformów, ale dostaje pod nimi widok i rzutowanie księżyca: sterowanie kamerą na mapę cieni nie wpływa. Mnożenie w shaderze wierzchołków omawia [`transforms.md`](transforms.md), sekcja 4.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co zawiera |
|---|---|
| [`src/game/NightMazeApp.hpp`](../../../src/game/NightMazeApp.hpp), [`.cpp`](../../../src/game/NightMazeApp.cpp) | obrót myszą oraz klawisze R, N i F w `onRender`, poza startowa kamery w `beginRound`, oko z interpolacji w `onRender` (od M4 także dla latarki), pola `m_camera` i `m_mouseSensitivity`, akcesory `camera()`, `mouseSensitivity()`, `player()` (sekcje od 5.2 do 5.5) |
| [`src/game/Player.hpp`](../../../src/game/Player.hpp), [`.cpp`](../../../src/game/Player.cpp) | ruch: to, co w M1 było lotem kamery w `onUpdate` ([`../game/player.md`](../game/player.md)) |
| [`src/debug/panels/CameraPanel.hpp`](../../../src/debug/panels/CameraPanel.hpp), [`.cpp`](../../../src/debug/panels/CameraPanel.cpp) | funkcja `debug::drawCameraPanel`: panel "Camera" (sekcja 6). Należy do programu `night_maze`, nie do biblioteki `engine` |
| [`src/main.cpp`](../../../src/main.cpp) | klasa `DebugNightMazeApp`: przekazuje kamerę, gracza i czułość do `debug::DebugContext`, wyłącza panelom mysz przy przechwyconym kursorze i blokuje grze mysz, gdy używa jej ImGui ([`../debug-ui.md`](../debug-ui.md), sekcja 5) |
| [`src/scene/Camera.hpp`](../../../src/scene/Camera.hpp), [`.cpp`](../../../src/scene/Camera.cpp) | to, czym sterowanie się posługuje: `rotate`, `forward`, `right`, `WORLD_UP`, `viewMatrix(eye)` ([`camera.md`](camera.md), sekcje od 5.2 do 5.5) |

### 5.2 Sterowanie w skrócie, stałe, pola i akcesory

Sterowanie stoi w `game::NightMazeApp`, a nie w `scene::Camera`: kamera zostaje czystą matematyką bez wejścia i czasu ([`README.md`](README.md), sekcja 3), a o tym, które klawisze i jaka mysz ją poruszają, decyduje gra. Teoria jest w sekcji 2.

**Sterowanie w skrócie:**

| Co robię | Skutek |
|---|---|
| klikam lewym przyciskiem w scenę | kursor zostaje przechwycony (znika), zaczyna działać obrót i ruch |
| ruszam myszą (kursor przechwycony) | kamera się obraca: w prawo i w lewo (yaw), w górę i w dół (pitch) |
| W, S, A, D (chodzenie) | gracz idzie poziomo: do przodu, do tyłu, w lewo, w prawo względem yaw |
| lewy Shift (chodzenie) | sprint |
| N | przełącza chodzenie i noclip. Działa także przy wolnym kursorze |
| F | włącza i wyłącza latarkę, która świeci z oka kamery w kierunku patrzenia. Działa także przy wolnym kursorze. Od M5 latarka ma baterię: przy pustej klawisz nadal przestawia przełącznik, ale najbliższy krok symulacji gasi ją z powrotem ([`../game/flashlight.md`](../game/flashlight.md)) |
| R | zaczyna rundę od nowa na tym samym labiryncie (`beginRound`): gracz wraca na start, a kamera dostaje yaw startowy i poziomy pitch. Działa także przy wolnym kursorze ([`../game/gameplay.md`](../game/gameplay.md)) |
| W, S, A, D (noclip) | lot wzdłuż kierunku patrzenia i na boki, przez ściany |
| spacja, lewy Shift (noclip) | lot pionowo w górę i w dół |
| Escape | oddaje kursor. Drugi Escape zamyka program ([`../core/input.md`](../core/input.md), sekcja 5.7) |

Klawisze R, N i F są czytane w `onRender` niezależnie od tego, czy kursor jest przechwycony. Nie działają tylko wtedy, gdy klawiaturę ma ImGui (na przykład podczas wpisywania liczby w pole panelu), bo gra pyta o nie przez `input()`, a ten podlega blokadzie klawiatury ([`../core/input.md`](../core/input.md)).

**Poza startowa.** Jedyne miejsce poza myszą i panelem, które zmienia kąty kamery, to koniec funkcji `NightMazeApp::beginRound`:

```cpp
    m_camera.position = m_player.eyePosition();
    m_camera.yawDegrees = m_mazeWorld.startYawDegrees;
    m_camera.pitchDegrees = LEVEL_PITCH_DEGREES;
```

`beginRound` wykonuje się w konstruktorze, po każdej wymianie labiryntu (`regenerateMaze`) i po restarcie rundy (klawisz R albo przycisk `Restart round (key R)` w panelu Gameplay). Wcześniej ta sama funkcja stawia gracza na `m_mazeWorld.startPosition` i wpisuje tę samą wartość do `m_previousPlayerPosition`, żeby następna klatka nie była rysowana z punktu między starym a nowym miejscem (pułapka 6). Kąty są wpisywane wprost w pola, z pominięciem `rotate`. To bezpieczne, bo obie wartości mieszczą się w zakresach: `startYawDegrees` to 0, 90, 180 albo 270 (`yawTowards`), a `LEVEL_PITCH_DEGREES` to 0. Trybu noclip `beginRound` nie zmienia.

**Stała i pola** (`NightMazeApp.hpp`):

```cpp
    // Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
    // is measured in the units of the window size, not in framebuffer pixels, so the same
    // hand movement turns the camera equally on a Retina display.
    static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;
```

```cpp
    // Where the scene is seen from (the view and projection matrices). The angles are
    // turned by the mouse. The position is not controlled directly: after every fixed
    // step it is set to the eyes of the player.
    scene::Camera m_camera;
```

```cpp
    // How the camera is turned. It belongs to the controls, not to the camera.
    float m_mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
```

| Element | Znaczenie |
|---|---|
| `DEFAULT_MOUSE_SENSITIVITY = 0.1F` | czułość startowa: 0,1 stopnia na jednostkę współrzędnych ekranu. Przesunięcie myszy o całą szerokość okna 1280 to obrót o 128 stopni |
| `m_camera` | kamera gry. Jej kąty zmienia mysz i panel. Jej pozycję nadpisuje `onUpdate` po każdym kroku (`m_camera.position = m_player.eyePosition();`), więc ręczna zmiana pozycji kamery nie miałaby sensu: stąd w panelu edytuje się stopy gracza |
| `m_mouseSensitivity` | bieżąca czułość. To pole, a nie stała, bo zmienia je panel Camera. Stała `DEFAULT_...` jest `static constexpr` w klasie, żeby dało się jej użyć jako wartości początkowej pola |

Prędkości ruchu nie są już polami aplikacji. W M1 była jedna prędkość kamery. Teraz są trzy (chód, sprint, lot) i należą do gracza: `Player::walkSpeed`, `sprintSpeed`, `flySpeed` ([`../game/player.md`](../game/player.md), sekcja 5). Poprzednia pozycja potrzebna do interpolacji też jest teraz pozycją gracza (`m_previousPlayerPosition`).

Panel dostaje te dane przez trzy chronione akcesory:

```cpp
    /// The camera, exposed so the debug UI can show and edit its angles and projection
    /// live. Its position follows the eyes of the player, see player().
    scene::Camera& camera() { return m_camera; }

    /// Mouse look sensitivity in degrees per screen coordinate unit of mouse movement,
    /// exposed so the debug UI can edit it live.
    float& mouseSensitivity() { return m_mouseSensitivity; }

    /// The player, exposed so the debug UI can show and edit its position, its speeds
    /// and the noclip mode live.
    Player& player() { return m_player; }
```

Każdy zwraca referencję do jednego pola, więc panel edytuje oryginał, a nie kopię. Są chronione (`protected`): widzi je tylko klasa pochodna, czyli `DebugNightMazeApp` w `main.cpp`, która przekazuje je do `debug::DebugContext` ([`../debug-ui.md`](../debug-ui.md), sekcja 5). Gra nadal nie dołącza niczego z `debug/`.

### 5.3 Obrót myszą w `onRender`

`onRender` zaczyna się od czterech krótkich bloków, które nie dotyczą myszy: zbudowania nowego labiryntu, jeśli panel o to poprosił ([`../game/maze-rendering.md`](../game/maze-rendering.md), sekcja 5), restartu rundy (klawisz R albo prośba z panelu Gameplay, sekcja 5.2), klawisza N ([`../game/player.md`](../game/player.md), sekcja 5) i klawisza F, który przełącza latarkę ([`../game/flashlight.md`](../game/flashlight.md)). Zaraz po nich stoi obrót:

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

Ten blok nie zmienił się od M1.

| Linia | Znaczenie |
|---|---|
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

**Kolejność w klatce.** `Application::run` woła najpierw kroki symulacji, a dopiero potem `onRender`. Kąty zmienione myszą w tej klatce są więc od razu użyte do narysowania jej (macierz widoku powstaje niżej w tym samym `onRender`), ale gracz użyje ich jako kierunku ruchu dopiero w krokach następnej klatki. Opóźnienie to jedna klatka i nie da się go zauważyć.

### 5.4 Ruch klawiszami: gdzie się podział

W M1 funkcja `onUpdate` sama czytała klawisze i przesuwała `m_camera.position`. Tego kodu już nie ma. `onUpdate` robi teraz pięć rzeczy: zapamiętuje poprzednią pozycję gracza, zamienia klawisze na `PlayerInput`, woła `m_player.update(...)` z kątami kamery i listą przeszkód rundy (`m_obstacles`: pudełka labiryntu i, dopóki jest zamknięta, pudełko bramy), ustawia `m_camera.position` na oczy gracza, a od M5 na końcu woła `updateRound`, czyli reguły rundy. Ruch linia po linii jest w [`../game/player.md`](../game/player.md), sekcje 5.5 i 5.6, a reguły rundy w [`../game/gameplay.md`](../game/gameplay.md).

Co z M1 zostało bez zmian w zachowaniu:

- **Ruch tylko przy przechwyconym kursorze.** Kliknięcie w scenę włącza całe sterowanie, Escape całe wyłącza. Powody są te same: jedna reguła dla myszy i klawiszy, brak przypadkowego ruchu podczas pracy z panelami, zachowanie znane z gier. Różnica techniczna: krok gracza wykonuje się teraz zawsze, tylko z pustym wejściem, zamiast wczesnego `return`.
- **Normalizacja kierunku** i pominięcie jej dla wektora zerowego.
- **Stały krok**: droga to prędkość razy `fixedDt`, nigdy razy czas klatki.

Co się zmieniło: przy chodzeniu ruch jest poziomy i ograniczony ścianami, a klawisze góra i dół działają tylko w trybie noclip.

### 5.5 Interpolacja: oko w `onRender`

```cpp
    // The simulation moves the player in fixed steps, and this frame is drawn at some
    // moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
    // between the position before the last step and the position after it keeps the
    // movement smooth at any frame rate, in all three directions: the height of the
    // feet follows the ground from step to step and is blended like x and z.
    // m_player.position itself is not changed. The eyes are a fixed height above the
    // feet, so blending the feet and then going up gives the same point as blending the
    // eyes.
    const glm::vec3 feet =
        glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha));
    const glm::vec3 eye = feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F};

    // The two matrices that are the same for everything drawn in this frame.
    const glm::mat4 view = m_camera.viewMatrix(eye);
    const glm::mat4 projection = m_camera.projectionMatrix(aspectRatio);
```

`glm::mix(a, b, t)` zwraca `a * (1 - t) + b * t` ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.8). `alpha` przychodzi jako `double`, a wektory są typu `float`, stąd rzutowanie. Wynik trafia do `m_camera.viewMatrix(eye)` ([`camera.md`](camera.md), sekcja 5.7).

Trzy pozycje, które łatwo pomylić:

| Pozycja | Kto ją ustawia | Do czego służy |
|---|---|---|
| `m_player.position` | `Player::update`, co krok | stan symulacji: stopy gracza |
| `m_camera.position` | `onUpdate`, co krok: `m_player.eyePosition()` | oczy po ostatnim kroku. Pokazuje ją panel Camera jako `Eye`. Do rysowania **nie** jest używana |
| `eye` (zmienna lokalna) | `onRender`, co klatkę | punkt, z którego naprawdę rysowana jest klatka: między dwoma krokami |

Macierze widoku i rzutowania są liczone raz na klatkę i przekazywane dwóm funkcjom rysującym (`drawMaze` i, gdy linie kolizji są włączone, `drawColliderLines`), więc labirynt, brama, kryształy i linie kształtów kolizji są zawsze widziane z tego samego punktu.

**Drugi odbiorca oka: latarka.** Zaraz po dwóch macierzach, w tym samym `onRender`, stoi:

```cpp
    const LightingSettings frameLighting = lightingForFrame(m_lighting, m_round, m_gameplay);
    const std::vector<glm::vec3> crystalLights = crystalLightPositions(m_round);
    const scene::LightSet lights =
        buildLightSet(frameLighting, flashlight, crystalLights);
    m_lightRig.upload(lights, eye);
```

| Wartość z kamery | Dokąd trafia | Po co |
|---|---|---|
| `eye` (to samo oko, z którego powstała macierz widoku) | `flashlightPose`: punkt odniesienia ręki i punktu, w który celuje wiązka (od piątej części M7) | latarka stoi 0,2 m na prawo i 0,25 m poniżej tego punktu, a nie w nim |
| `m_camera.forward()` (kierunek z bieżących yaw i pitch) | `flashlightPose`: kierunek, w którym leży punkt zbieżności | wiązka celuje z ręki w punkt 4 m przed okiem na osi widoku, więc plama jest w środku ekranu na ścianie w tej odległości |
| `m_camera.right()` | `flashlightPose`: kierunek "na prawo" | wektor jest poziomy, więc ręka jest zawsze 0,2 m od środka ciała, jakkolwiek gracz jest obrócony |
| `eye` | `m_lightRig.upload`: pozycja kamery w bloku świateł | shader liczy z niej kierunek do oka, potrzebny do połysku |

Dwie pierwsze linie są z M5 i z kamery niczego nie biorą: `lightingForFrame` robi kopię ustawień, w której słaba bateria przygasza latarkę, a `crystalLightPositions` podaje miejsca świateł punktowych nad niezebranymi kryształami ([`../game/gameplay.md`](../game/gameplay.md)).

Kolejność ma znaczenie i jest zapisana w komentarzu nad tym kodem: światła powstają **po** obrocie myszą z tej klatki i z **interpolowanego** oka, a nie z `m_camera.position`. Dzięki temu środek stożka latarki wypada w środku ekranu. Z `m_camera.position` (oczy po ostatnim kroku) latarka zostawałaby w ruchu o ułamek kroku za obrazem. To wynika z kodu: tego, czy plama światła trzyma się środka ekranu podczas chodzenia, nikt jeszcze nie sprawdził ręcznie. Budowę zestawu świateł i samą latarkę opisuje [`../game/flashlight.md`](../game/flashlight.md), a wysyłkę do karty [`../gfx/uniform-buffers.md`](../gfx/uniform-buffers.md).

Przypadki brzegowe interpolacji (start, klatka bez kroku, klatka z kilkoma krokami, wyłączenie noclip w powietrzu, nowy labirynt, edycja pozycji z panelu, okno zminimalizowane) są zebrane w tabeli w [`../game/player.md`](../game/player.md), sekcja 5.7.

### 5.6 Jak sprawdzone zostało sterowanie

Prawdziwych naciśnięć klawiszy i ruchu myszy nie da się wstrzyknąć do GLFW z kodu: `glfwGetKey` i `glfwGetCursorPos` oddają to, co przyszło z systemu. Sprawdzenie ma więc część automatyczną (z trzech kamieni milowych), stan zgłoszony po M4 i M5 oraz część ręczną.

**Część automatyczna z M1 (pomiar historyczny).** Poniższa tabela opisuje pomiar wykonany na kodzie kamienia milowego M1, gdy w początku układu stała kostka (usunięta w M5), a kamera startowała w `(0, 0, 3)` i była sterowana wprost. Nie został powtórzony na obecnym kodzie, a pozycje i rozmiary w pikselach dotyczą tamtej sceny. Zostawiam go, bo sprawdza kod, który się od tamtej pory nie zmienił: `Camera::rotate`, `Camera::viewMatrix(eye)`, `Camera::projectionMatrix`, mieszanie dwóch pozycji z `alpha` i przełącznik `DebugUI::setMouseEnabled`. Wierszy o ruchu kamery klawiszami nie ma, bo tamten kod został zastąpiony graczem i jego testami.

Tymczasowy program (nie trafił do repozytorium) z klasą pochodną od `game::NightMazeApp` i z własnym `debug::DebugUI`, z ukrytym oknem (`GLFW_VISIBLE` równe `GLFW_FALSE`). Wołał prawdziwe `onUpdate` i `onRender`, zmieniał kamerę przez akcesor `camera()` i odczytywał narysowaną klatkę przez `glReadPixels`. Framebuffer 2560 x 1440, współrzędne y liczone od dołu:

| Sprawdzenie (kod z M1) | Wynik |
|---|---|
| pierwsza klatka przed jakimkolwiek krokiem, `alpha` 0 i 0,9 | ten sam obraz: kostka w kolumnach od 977 do 1638, wierszach od 391 do 1004 |
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
| po `setMouseEnabled(false)`, ten sam kursor | `wantsMouse()` zwraca fałsz już w tym samym `draw`. Wciśnięty i przeciągany przycisk nie aktywuje widżetu i nie zmienia żadnej wartości ([`../debug-ui.md`](../debug-ui.md), sekcja 5) |

Zdarzenia myszy w dwóch ostatnich wierszach były podawane wprost do ImGui (`ImGuiIO::AddMousePosEvent`, `AddMouseButtonEvent`), z pominięciem GLFW.

**Część automatyczna z M2 + M3.** Ruch gracza (kierunki względem yaw, niezależność chodzenia od pitch, lot wzdłuż pitch, normalizacja, kolizje) sprawdza 13 przypadków testowych w `tests/PlayerTests.cpp` ([`../game/player.md`](../game/player.md), sekcja 5.9). Na Windowsie (2026-10-05, MSVC 19.44) przechodziły w Debug i Release. Program uruchomiony tam startował bez linii `[error]` i bez linii `GL_`, a na zrzutach ekranu sprawdzone były: widok startowy ze środka labiryntu i widok z góry w trybie noclip, na którym ściany zgadzają się z planem w panelu Maze. Ten drugi stan został osiągnięty tymczasowym kodem (potem usuniętym), a nie myszą i klawiszami.

**Stan po M4.** Na Windowsie (2026-10-05, MSVC 19.44, RTX 4070 Ti SUPER, sterownik NVIDIA 610.74) kod z oświetleniem i mapami normalnych budował się w Debug i Release bez ostrzeżeń, 163 przypadki testowe i 62220 asercji przechodziły w obu (w tym te same 13 przypadków gracza), a gra startowała bez linii `[error]` i bez linii `GL_`. Widok startowy i scena z latarką włączoną i wyłączoną były obejrzane na zrzutach ekranu.

**Stan po M5.** Zgłoszone dla Windowsa 2026-10-05: kod z rundą, kryształami i bramą buduje się w Debug i Release bez ostrzeżeń, a 215 przypadków testowych i 85098 asercji przechodzi w obu konfiguracjach (w tym te same 13 przypadków gracza). Obraz był sprawdzany na zrzutach ekranu robionych przez tymczasowy kod, który potem usunięto. M5 nie jest zamknięty: na macOS nic z niego nie było budowane ani uruchamiane, a ręcznie nikt jeszcze niczego nie sprawdził.

**Stan po drugiej części M6.** Gracz chodzi po terenie z mapy wysokości, więc `Player feet` i `Eye` pokazują `y` gruntu i `y` gruntu plus 1,7, a nie 0 i 1,7. Paneli było wtedy dziesięć: doszły Terrain i Grass, zwinięte w drugim rzędzie pasków pod Camera i Gameplay. Cały program testowy miał 256 przypadków i 101232 asercje (uruchomione 2026-10-05 w Debug i Release). M6 też nie jest zamknięty: macOS i testy ręczne są otwarte.

**Stan po pierwszej części M7.** Sterowanie kamerą się nie zmieniło. Paneli było od tej części jedenaście: doszedł Framebuffers, zwinięty w trzecim rzędzie pasków, więc HUD stanął o jeden pasek niżej. Scena, którą ogląda kamera, jest rysowana do framebuffera HDR i przenoszona do okna ostatnim przebiegiem klatki ([`../renderer/post-process.md`](../renderer/post-process.md)): dla myszy i klawiatury nic z tego nie wynika. Zgłoszone dla Windowsa: 269 przypadków i 102103 asercje w Debug i Release, a po drugiej części M7 276 i 102139, po trzeciej 294 i 102412, po czwartej 310 i 103751. Na macOS nic z M7 nie było budowane.

**Stan po czwartej części M7 (cienie księżyca, 2026-10-05).** Sterowanie kamerą znów się nie zmieniło. Paneli jest dwanaście: doszedł Shadows, zwinięty w czwartym rzędzie pasków, więc HUD stoi o jeszcze jeden pasek niżej (sekcja 6). Kamera gracza nie bierze udziału w liczeniu mapy cieni: pudełko światła księżyca zależy od terenu i kierunku księżyca, nie od oka ([`lights.md`](lights.md), sekcja 5.7), więc obrót i ruch kamery nie przesuwają cieni. Nowego panelu i wyglądu cieni w ruchu nikt nie sprawdzał ręcznie.

**Część ręczna.** Tego, co wymaga człowieka przy myszy i klawiaturze, nikt jeszcze nie sprawdził na obecnym kodzie: przechwycenia kursora po kliknięciu, kierunku i płynności obrotu wewnątrz labiryntu, chodzenia, klawisza N, klawisza F (także przy pustej baterii), klawisza R (czy kamera wraca do yaw startowego i poziomego pitch), tego, czy stożek latarki trzyma się środka ekranu w ruchu, rozwinięcia panelu Camera kliknięciem, zwolnienia kursora klawiszem Escape, zachowania paneli przy przechwyconym kursorze. To jest scenariusz z sekcji 6.3 i lista kontrolna w [`../../guides/build-windows.md`](../../guides/build-windows.md). Na macOS obecny kod nie był budowany ani uruchamiany ([`../../guides/build-macos.md`](../../guides/build-macos.md)).

## 6. Panel ImGui

Panel **Camera** jest pokazem tematu 3 (PRD, sekcja 3: "Pozycja/rotacja kamery, FOV"). Kod: [`src/debug/panels/CameraPanel.cpp`](../../../src/debug/panels/CameraPanel.cpp). Jak panel jest podpięty do `DebugUI` i skąd dostaje dane, opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5.

**Gdzie panel stoi.** Od M5 paneli było osiem, od M6 dziesięć, od pierwszej części M7 jedenaście, od czwartej części M7 jest dwanaście, a dwie kolumny i dolny rząd mieszczą sześć. Pozostałe sześć startuje zwinięte (do trzeciej części M7 było ich pięć). Miejsce panelu Camera w lewej kolumnie, pod panelem Renderer, zajął w M4 panel Lights. Panele Camera i Gameplay przy pierwszym uruchomieniu są **zwinięte** do samych pasków tytułu i stoją obok siebie przy górnej krawędzi okna, między kolumnami. W oknie 1280 x 720 pasek Camera zaczyna się w x 352, y 8 i ma 280 jednostek szerokości, a pasek Gameplay zaczyna się w x 640 i ma 324. Od M6 tuż pod nimi stoi drugi rząd zwiniętych pasków: Terrain pod Camera i Grass pod Gameplay, każdy tak szeroki jak pasek nad nim (`TERRAIN_PLACEMENT` i `GRASS_PLACEMENT` z `foldedRowsBefore = 1`). Od pierwszej części M7 pod nimi jest trzeci rząd: jeden pasek panelu Framebuffers, zaczynający się w x 352 i szeroki na oba paski nad nim razem z odstępem, czyli 612 jednostek (`FRAMEBUFFERS_PLACEMENT` z `foldedRowsBefore = 2`). Od czwartej części M7 pod nim jest czwarty rząd: pasek panelu Shadows, w tym samym miejscu w poziomie i tak samo szeroki jak pasek Framebuffers (`SHADOWS_PLACEMENT` z `foldedRowsBefore = 3` i rozmiarem `FRAMEBUFFERS_WIDTH` na `SHADOWS_HEIGHT`, czyli 612 na 324 jednostki po rozwinięciu). Pod czterema rzędami, na środku, stoi pasek HUD (`foldedRowsHeight(FOLDED_ROW_COUNT, scale)` plus `HUD_TOP_OFFSET` w `src/debug/Hud.cpp`, `FOLDED_ROW_COUNT` równe 4, do trzeciej części M7 3). Kliknięcie strzałki w pasku rozwija panel Camera do prostokąta 280 na 416 jednostek, który sięga w dół do dolnego rzędu paneli i zasłania swoją część sceny oraz zwinięte paski pod sobą (Terrain i swoją część pasków Framebuffers i Shadows), ale żadnego otwartego panelu. Jest trochę niższy niż jego zawartość, więc po rozwinięciu ma pasek przewijania (tak mówi komentarz w `PanelLayout.hpp`). Po rozwinięciu ImGui zapamiętuje ten stan w `imgui.ini`. Rozwijania kliknięciem nikt jeszcze nie sprawdził ręcznie: opis wynika z kodu ([`../debug-ui.md`](../debug-ui.md), sekcja 5.7).

PRD w sekcji 10 wymienia dla panelu Camera: "Pozycja, FOV, czułość myszy, tryb noclip". Wszystkie cztery są w programie, z jedną różnicą w rozmieszczeniu: panel Camera **pokazuje** tryb (linia `Mode:`), a przełącza go klawisz N albo pole wyboru `Noclip (key N)` w panelu Collision ([`collision.md`](collision.md), sekcja 6), bo noclip znaczy "wyłącz kolizje".

**Panel a przechwycony kursor.** Mysz ma w każdej chwili jednego właściciela. Gdy kursor jest przechwycony, należy do kamery i panele jej nie widzą (`DebugUI::setMouseEnabled(false)`). Gdy kursor jest wolny i używa go ImGui, gra dostaje od `core::Input` odpowiedź "nic się nie dzieje" (blokada myszy), więc przeciąganie suwaka nie obraca kamery, a kliknięcie w panel nie przechwytuje kursora. Mechanizm opisują [`../core/input.md`](../core/input.md), sekcje 5.10 i 5.11, oraz [`../debug-ui.md`](../debug-ui.md), sekcja 5. Tutaj jest tylko skutek: krok 9 scenariusza (sekcja 6.3) i pułapka 7.

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

// The near plane must stay above 0. The upper limit is far enough to cut into the walls
// around the player.
constexpr float MIN_NEAR_PLANE = 0.01F;
constexpr float MAX_NEAR_PLANE = 10.0F;

// The lower limit is small enough to cut off the far end of a corridor.
constexpr float MIN_FAR_PLANE = 1.0F;
constexpr float MAX_FAR_PLANE = 200.0F;

// Smallest distance between the two planes. They must never be equal: the projection
// matrix divides by (far - near).
constexpr float MIN_PLANE_DISTANCE = 0.1F;

constexpr float MIN_MOUSE_SENSITIVITY = 0.01F;
constexpr float MAX_MOUSE_SENSITIVITY = 1.0F;

// Limits of the three speeds of the player, in metres per second. At the upper limit one
// fixed step (1/120 s) is about 17 cm long, still short next to a wall box (30 cm thick).
// scene::moveAndSlide never jumps over a box whatever the step, but it moves one axis at
// a time, and that staircase path only stays close to the straight one for short steps.
constexpr float MIN_MOVE_SPEED = 0.5F;
constexpr float MAX_MOVE_SPEED = 20.0F;
```

Uwaga do ostatniego komentarza: liczby są poprawne (20 m/s razy 1/120 s to około 0,17 m, pudełko ściany ma 0,3 m), ale uzasadnienie jest słabsze, niż mogłoby być. `moveAndSlide` mierzy odstęp do przeszkody na każdej osi, więc nie przepuściłby gracza przez ścianę także przy kroku dłuższym niż jej grubość ([`collision.md`](collision.md), sekcja 2.6). Krótki krok jest potrzebny z innego powodu: żeby droga "po schodkach" nie różniła się wyraźnie od prostej ([`collision.md`](collision.md), sekcja 2.8).

```cpp
void drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity) {
    // First run only: the top edge of the window, right of the left column, folded to its
    // title bar (the constant is in PanelLayout.hpp). Later ImGui remembers the panel in
    // imgui.ini.
    placePanelOnFirstUse(CAMERA_PLACEMENT);
    if (ImGui::Begin("Camera")) {
        ImGui::TextWrapped("Click the scene to capture the mouse, Esc releases it. While "
                           "captured the mouse looks around. Walking: W A S D walk level, "
                           "Left Shift sprints. Noclip (key N): W A S D fly along the view, "
                           "Space goes up, Left Shift goes down.");

        ImGui::Separator();
        ImGui::Text("Mode: %s", player.noclip ? "noclip (free flight)" : "walking");
        // The camera has no position of its own to edit: after every fixed step the game
        // puts it at the eyes of the player. So the field that can be dragged is the
        // position of the player (the feet), and the eye is only shown. DragFloat3 edits
        // three floats through the pointer: x, y and z. While walking the game keeps y on
        // the ground, so a change of y lasts only in noclip mode.
        ImGui::DragFloat3("Player feet", glm::value_ptr(player.position), POSITION_DRAG_SPEED);
        ImGui::Text("Eye: %.2f, %.2f, %.2f", camera.position.x, camera.position.y,
                    camera.position.z);

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
        ImGui::SliderFloat("Walk speed", &player.walkSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Sprint speed", &player.sprintSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Fly speed", &player.flySpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::End();
}
```

| Element | Znaczenie |
|---|---|
| `placePanelOnFirstUse(CAMERA_PLACEMENT)` | miejsce, rozmiar i stan zwinięcia panelu przy pierwszym uruchomieniu: górna krawędź okna, na prawo od lewej kolumny, zwinięty do paska tytułu. Stała `CAMERA_PLACEMENT` leży w [`PanelLayout.hpp`](../../../src/debug/PanelLayout.hpp) razem z siedmioma pozostałymi. Pole `.collapsed = true` mają dwie z nich: ta i `GAMEPLAY_PLACEMENT`. Rozmiar po rozwinięciu to 280 na 416 jednostek. Zawartość przy dawnej szerokości 336 miała zmierzoną wysokość 452 (pomiar z M4, 2026-10-05), więc w 416 się nie mieści i panel się przewija: dla nowej szerokości wysokości nie mierzyłem. Wywołanie liczy się tylko wtedy, gdy plik `imgui.ini` nie ma jeszcze wpisu dla tego panelu ([`../debug-ui.md`](../debug-ui.md), sekcja 5.7) |
| `if (ImGui::Begin("Camera"))` | dla zwiniętego panelu `Begin` zwraca `false`. Dopóki panel jest zwinięty, żaden z widżetów niżej nie jest budowany, a `ImGui::End()` wykonuje się jak zawsze |
| stałe `MIN_...` i `MAX_...` | granice suwaków, nazwane i opisane w jednym miejscu, w anonimowej przestrzeni nazw pliku. Bez nich w wywołaniach stałyby gołe liczby |
| `drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity)` | panel dostaje dokładnie to, co edytuje: trzy referencje bez `const`. W M1 trzecim parametrem była prędkość ruchu, teraz prędkości są polami gracza |
| `ImGui::SetNextWindowPos(..., ImGuiCond_FirstUseEver)`, `SetNextWindowSize` i `SetNextWindowCollapsed` (wewnątrz `placePanelOnFirstUse`) | dotyczą panelu otwieranego przez najbliższe `Begin`. Warunek `FirstUseEver`: tylko gdy ImGui nie zna jeszcze tego panelu |
| `ImGui::TextWrapped(...)` | linia pomocy tylko do odczytu: jak przechwycić kursor i czym się steruje w obu trybach. Sąsiednie napisy w cudzysłowach kompilator skleja w jeden |
| `ImGui::Text("Mode: %s", player.noclip ? "noclip (free flight)" : "walking")` | bieżący tryb gracza, tylko do odczytu |
| `ImGui::DragFloat3("Player feet", glm::value_ptr(player.position), POSITION_DRAG_SPEED)` | trzy pola przeciągane myszą: stopy gracza. `glm::value_ptr` daje wskaźnik na pierwszą składową wektora ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 3.9), a ImGui czyta i zapisuje przez niego x, y i z. Bez granic. Przy chodzeniu gra co krok ustawia y na 0, więc zmiana y trwa tylko w trybie noclip |
| `ImGui::Text("Eye: %.2f, %.2f, %.2f", camera.position.x, ...)` | pozycja kamery, tylko do odczytu: oczy gracza po ostatnim kroku symulacji, 1,7 m nad stopami |
| `ImGui::SliderFloat("Yaw", &camera.yawDegrees, MIN_YAW_DEGREES, MAX_YAW_DEGREES, "%.1f deg", ...)` | suwak od 0 do 360, czyli zakres, w którym trzyma yaw `Camera::rotate`. `"%.1f deg"` to format wyświetlanej wartości |
| `ImGui::SliderFloat("Pitch", ..., -scene::Camera::MAX_PITCH_DEGREES, scene::Camera::MAX_PITCH_DEGREES, ...)` | suwak od -89 do 89: granice wzięte ze stałej kamery, a nie wpisane drugi raz |
| `ImGuiSliderFlags_AlwaysClamp` | suwak ImGui pozwala wpisać wartość z klawiatury (Ctrl i kliknięcie), a wpisana liczba domyślnie może wyjść poza granice. Ta flaga przycina ją do zakresu. Bez niej dałoby się wpisać pitch 90 i zepsuć macierz widoku ([`camera.md`](camera.md), pułapka 3) |
| `ImGuiSliderFlags_Logarithmic` przy `Near plane` | skala logarytmiczna: połowa długości suwaka przypada na małe wartości, gdzie zmiana bliskiej płaszczyzny ma największe znaczenie. `\|` łączy dwie flagi w jedną maskę |
| `camera.farPlane = std::max(camera.farPlane, camera.nearPlane + MIN_PLANE_DISTANCE);` | zakresy suwaków `Near plane` (do 10) i `Far plane` (od 1) zachodzą na siebie. Ta linia pilnuje, żeby daleka płaszczyzna była zawsze co najmniej 0,1 m za bliską: macierz rzutowania dzieli przez `far - near` ([`camera.md`](camera.md), sekcja 2.3) |
| `ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, ...)` | ustawienie sterowania: pole `NightMazeApp`, nie kamery |
| `"Walk speed"`, `"Sprint speed"`, `"Fly speed"` | trzy suwaki o tych samych granicach (od 0,5 do 20 m/s), każdy pisze do jednego pola gracza |

Panel nie ma zmiennych `static` ani globalnych i nie woła żadnej funkcji `gl*`. Zmienia tylko liczby, a skutek widać w następnej klatce, gdy `onRender` policzy z nich macierze, i w następnym kroku symulacji, gdy gracz użyje nowych prędkości. Nie ma też przycisku "zastosuj": w trybie natychmiastowym suwak pisze do pola w chwili przeciągania.

### 6.2 Kontrolki i czego uczą

| Kontrolka | Zakres | Co zmienia | Czego uczy |
|---|---|---|---|
| linia pomocy | odczyt | nic | jak wejść w sterowanie, jak z niego wyjść i czym różnią się dwa tryby |
| `Mode:` | odczyt | nic | czy gracz chodzi, czy lata |
| `Player feet` | bez granic | `player.position` | kamera podąża za graczem: zmiana stóp przesuwa oko. Podczas chodzenia liczby x i z zmieniają się same, a y idzie za wysokością gruntu |
| `Eye:` | odczyt | nic | oko jest dokładnie 1,7 m nad stopami. Macierz widoku przesuwa świat przeciwnie do oka |
| `Yaw` | od 0 do 360 stopni | `camera.yawDegrees` | kompas: 0 to -Z, 90 to +X. Przy obrocie myszą w prawo wartość rośnie i po 360 wraca do 0 (zawijanie w `rotate`). Kreska na planie w panelu Maze obraca się razem z nią |
| `Pitch` | od -89 do 89 stopni | `camera.pitchDegrees` | ograniczenie pitch: suwak i mysz zatrzymują się na 89, żeby kierunek patrzenia nie stał się równoległy do pionu ([`camera.md`](camera.md), sekcja 2.2) |
| `FOV` | od 20 do 120 stopni | `camera.fovDegrees` | kąt widzenia jako zoom: mały kąt powiększa (teleobiektyw), duży pokazuje szeroko i rozciąga obraz przy krawędziach. Gracz się przy tym nie rusza |
| `Near plane` | od 0,01 do 10 m, skala logarytmiczna | `camera.nearPlane` | bliska płaszczyzna obcina wszystko, co bliżej: przy wartości większej niż odległość do ściany korytarza ściana zostaje przecięta i widać przez nią dalsze komórki |
| `Far plane` | od 1 do 200 m | `camera.farPlane` | daleka płaszczyzna obcina wszystko, co dalej: przy małej wartości znika koniec korytarza, a w jego miejscu widać kolor tła |
| `Mouse sensitivity` | od 0,01 do 1 stopnia na jednostkę | `m_mouseSensitivity` | czułość to tylko mnożnik między ruchem myszy a kątem |
| `Walk speed`, `Sprint speed`, `Fly speed` | od 0,5 do 20 m/s | pola gracza | prędkość w metrach na sekundę, niezależna od FPS dzięki stałemu krokowi |

Granice FOV: przy kącie zbliżonym do 180 `tan(fov / 2)` rośnie do nieskończoności i rzutowanie przestaje mieć sens, a przy kącie bliskim 0 obraz jest wycinkiem tak wąskim, że niczego nie da się rozpoznać. Bliska płaszczyzna nie schodzi poniżej 0,01, bo zero daje błędną macierz ([`camera.md`](camera.md), pułapka 4).

Odległości, przy których płaszczyzny zaczynają ciąć ściany, zależą od tego, gdzie gracz stoi i dokąd patrzy. Dla orientacji: w środku komórki lica ścian bocznych są około 0,9 m od oka (komórka ma 2 m, widoczna ściana 0,2 m grubości). Dokładnych wartości dla labiryntu nie mierzyłem.

### 6.3 Scenariusz pokazu na obronie

Program uruchomiony, panele widoczne. Tego scenariusza nikt jeszcze nie przeszedł ręcznie na obecnym kodzie: opisuje to, co wynika z kodu. Krok zerowy od M4: panel Camera startuje zwinięty, więc najpierw klikam strzałkę w jego pasku tytułu (przy górnej krawędzi, na prawo od panelu Renderer, obok paska panelu Gameplay). Rozwinięty panel zasłania lewą część sceny, więc na czas kroków 2 do 6 można go przeciągnąć albo zadokować tam, gdzie nie przeszkadza.

1. **Stan startowy.** Odczytuję z panelu: `Mode: walking`, `Player feet` to około `(1, 0,12, 1)`, `Eye` to około `(1, 1,82, 1)`, `Pitch` 0, `FOV` 60, a `Yaw` wskazuje otwarty bok komórki startowej (180 dla ziarna 1). Mówię: kamera nie ma własnej pozycji, stoi w oczach gracza.
2. **Przechwycenie.** Klikam w scenę. Kursor znika. Mówię: tryb `GLFW_CURSOR_DISABLED`, od tej chwili mysz należy do kamery, a panele jej nie widzą.
3. **Obrót.** Ruszam myszą w prawo: obraz ucieka w lewo, `Yaw` rośnie, kreska na planie w panelu Maze obraca się zgodnie z ruchem wskazówek zegara. Ruszam do góry: `Pitch` rośnie i zatrzymuje się na 89. Wyjaśniam minus przy y i ograniczenie pitch.
4. **Chód.** Trzymam W: w `Player feet` zmieniają się x albo z, a `Eye` pokazuje te same liczby z y większym o 1,7. Samo y obu linii powoli się zmienia, bo grunt jest nierówny. Patrzę w ziemię i dalej trzymam W: prędkość się nie zmienia (ruch używa tylko yaw).
5. **Lot tam, gdzie patrzę.** Naciskam N (`Mode: noclip (free flight)`), patrzę w górę i trzymam W: wznoszę się. Mówię, że to dawna kamera latająca i że różnica między trybami to jedna linia z pitch.
6. **Zwolnienie.** Naciskam Escape: kursor wraca, program działa dalej. Mówię, że drugi Escape zamknąłby program. Naciskam N, żeby wrócić na grunt.
7. **FOV jako zoom.** Przeciągam `FOV` od 60 do 20, potem do 120. `Player feet` się nie zmienia, a koniec korytarza przybliża się i oddala.
8. **Bliska i daleka płaszczyzna.** Przesuwam `Near plane` w górę, aż najbliższa ściana zostanie przecięta, i wracam do 0,1. Przesuwam `Far plane` w dół, aż zniknie koniec korytarza, i wracam do 100.
9. **Panele a kamera.** Przeciągam dowolny suwak: kamera się nie obraca i kursor nie zostaje przechwycony, bo mysz ma ImGui.
10. **Czułość i prędkość.** Ustawiam `Mouse sensitivity` na 0,5, klikam w scenę i pokazuję, że ten sam ruch ręki obraca pięć razy mocniej. To samo z `Walk speed`.
11. **Restart.** Odchodzę kilka komórek od startu, patrzę w górę i naciskam R. Odczytuję z panelu: `Player feet` to znów około `(1, 0,12, 1)`, `Yaw` 180, `Pitch` 0. Mówię: `beginRound` wpisuje pozę startową wprost w pola gracza i kamery, tak samo jak przy starcie programu.

## 7. Pułapki

1. **Przesunięcie myszy użyte w `onUpdate`.** `mouseDeltaX` i `mouseDeltaY` opisują jedną klatkę, a `onUpdate` wykonuje się od zera do wielu razy na klatkę. Obrót liczony w `onUpdate` gubi ruch myszy w klatkach bez kroku i liczy go kilka razy w klatkach z kilkoma krokami: czułość zależy od FPS. To samo dotyczy kliknięcia (`wasMouseButtonPressed`) oraz klawiszy R, N i F (`wasKeyPressed`). Wszystkich pięć stoi w `onRender`.
2. **Przesunięcie myszy pomnożone przez czas.** Odruch "wszystko razy `dt`" jest tu błędem. Przesunięcie myszy to droga, a nie prędkość: jest już proporcjonalne do czasu klatki, bo w dłuższej klatce ręka zdążyła przesunąć mysz dalej. Pomnożone przez `dt` dałoby obrót zależny od FPS.
3. **Edycja `m_camera.position`.** Pole wygląda jak coś, co można ustawić, ale `onUpdate` nadpisuje je po każdym kroku pozycją oczu gracza. Kto chce przenieść kamerę, przenosi gracza (`m_player.position`) i poprzednią pozycję razem, tak jak `beginRound`.
4. **Rysowanie z `m_camera.position` zamiast z `eye`.** To oczy po ostatnim kroku, bez interpolacji. Obraz byłby poprawny, ale szarpany (ćwiczenie 6).
5. **Skok kamery przy pierwszym ruchu myszy.** Pierwsze przesunięcie po przechwyceniu kursora liczone względem starej pozycji kursora dałoby jeden wielki obrót. Chroni przed tym `core::Input` (flaga `m_skipNextMouseDelta`), a kod kamery dodatkowo nie obraca w klatce kliknięcia (gałąź `else`). Kto pisze własne odczytywanie myszy, musi pierwszy odczyt pominąć sam.
6. **Zapomniana pozycja poprzednia.** `m_previousPlayerPosition` musi być zapisywane na początku **każdego** kroku, przed ruchem. Zapisane tylko wtedy, gdy gracz się rusza, zostawia po zatrzymaniu starą wartość: obraz drga między dwiema pozycjami w rytmie `alpha`. Zapisane po ruchu sprawia, że obie pozycje są zawsze równe i interpolacja nic nie robi. Przy przeniesieniu gracza (nowy labirynt) trzeba ustawić oba pola naraz, inaczej jedna klatka pokaże przelot przez ściany.
7. **Obrót myszą, gdy kursor jest nad panelem.** Bez blokady myszy przeciąganie suwaka w panelu obracałoby kamerę, a kliknięcie w panel przechwytywałoby kursor. Gra pyta o mysz wyłącznie przez `input()`, a `main.cpp` blokuje te odpowiedzi, gdy myszy używa ImGui. W drugą stronę działa `DebugUI::setMouseEnabled`: przy przechwyconym kursorze panele nie widzą myszy ([`../debug-ui.md`](../debug-ui.md), sekcja 5).
8. **Interpolacja kątów "przy okazji".** Yaw zawija się z 359 do 0. Zwykłe `mix(359, 1, 0,5)` daje 180, czyli obrót w przeciwną stronę. W projekcie kąty nie są interpolowane (zmieniają się raz na klatkę), ale kto przeniesie obrót do `onUpdate`, trafi na ten problem.
9. **Kąty zmienione w tej klatce a ruch.** Obrót jest w `onRender`, czyli po krokach symulacji tej klatki. Gracz idzie więc w kierunku z poprzedniej klatki. Różnica to jedna klatka i nie jest błędem, ale wyjaśnia, dlaczego test ruchu podaje yaw jako parametr, a nie czyta go z kamery.
10. **Panel nie stoi tam, gdzie mówi kod.** `CAMERA_PLACEMENT` działa tylko przy pierwszym uruchomieniu. Jeśli w katalogu roboczym leży `imgui.ini` z wpisem `[Window][Camera]`, wygrywa wpis, razem z zapisanym stanem zwinięcia. Plik sprzed M4 trzyma panel Camera rozwinięty w lewej kolumnie, a nowy panel Lights, który wpisu nie ma, staje na nim. Plik sprzed M5 nie ma wpisu panelu Gameplay, więc ten panel staje w miejscu domyślnym niezależnie od tego, gdzie stoją panele zapamiętane. Żeby zobaczyć układ domyślny, trzeba ten plik usunąć.
11. **`Fly speed` nie zmienia chodu, `Sprint speed` nie zmienia lotu.** Każdy suwak pisze do pola używanego w jednym trybie. Ustawienie `Sprint speed` poniżej `Walk speed` jest dozwolone i sprawia, że Shift spowalnia.
12. **"Nie ma panelu Camera".** Jest, tylko zwinięty: sam pasek tytułu przy górnej krawędzi okna, na prawo od panelu Renderer, obok zwiniętego panelu Gameplay. To stan startowy od M4, a nie błąd.
13. **Latarka liczona z `m_camera.position`.** Ta sama pomyłka co w pułapce 4, tylko dla światła: reflektor ustawiony w oczach po ostatnim kroku, a obraz rysowany z oka interpolowanego. W ruchu stożek zostawałby za obrazem o ułamek kroku. `onRender` podaje do `flashlightPose` (a dalej do `buildLightSet`) to samo `eye`, z którego liczy macierz widoku (sekcja 5.5).

## 8. Ćwiczenia

Wszystkie ćwiczenia są zmianami w działającym programie: zmiana w `NightMazeApp.cpp` albo w `CameraPanel.cpp` wymaga zbudowania (`cmake --build --preset debug`). Po każdym ćwiczeniu wycofaj zmianę (`git checkout src assets/shaders`). Ćwiczeń nie wykonywałem na obecnym kodzie: opisy skutków wynikają z czytania kodu.

1. **Odwrócona oś y.** Usuń minus w linii liczącej `pitchDelta`. Co się zmieniło? Dodaj do `NightMazeApp` pole `bool m_invertMouseY`, doprowadź je do panelu Camera jako `ImGui::Checkbox` (kroki opisuje [`../debug-ui.md`](../debug-ui.md), sekcja 5) i wybieraj znak na jego podstawie.
2. **Przechylenie (roll).** `Camera::viewMatrix` podaje do `lookAt` zawsze `WORLD_UP`, więc kamera się nie przechyla. Dodaj tymczasowo pole `rollDegrees` i obróć wektor góry wokół `forward()` (`glm::rotate(glm::mat4(1.0F), glm::radians(rollDegrees), forward())` razy `vec4(WORLD_UP, 0.0F)`). Dlaczego `right()` przestaje wtedy pasować do obrazu i co trzeba by zmienić w ruchu na boki?
3. **Kamera z trzeciej osoby.** W `onRender` odsuń oko od gracza do tyłu: `eye - m_camera.forward() * 3.0F`. Włącz `Draw collision shapes` w panelu Collision, żeby widzieć zielone pudełko gracza. Co się dzieje, gdy stoisz tyłem do ściany, i dlaczego kamera z trzeciej osoby potrzebuje własnych kolizji?
4. **Zoom klawiszem.** W `onUpdate` zmniejszaj `m_camera.fovDegrees` z prędkością 60 stopni na sekundę, gdy trzymany jest klawisz Z, i zwiększaj z tą samą prędkością, gdy puszczony, w granicach od 20 do 60. Dlaczego to jest zmiana rzutowania, a nie pozycji? Co musiałoby się stać z czułością myszy, żeby celowanie w przybliżeniu było równie wygodne?
5. **Przesunięcie myszy w złym miejscu.** Przenieś blok obrotu z `onRender` do `onUpdate` (pułapka 1). Porównaj czułość przy włączonym i wyłączonym vsync (ćwiczenie 1 w [`../core/window-context.md`](../core/window-context.md)). Wycofaj zmianę.
6. **Bez interpolacji.** Zamień w `onRender` argument `viewMatrix(eye)` na `viewMatrix(m_camera.position)` i ustaw `Walk speed` na 20. Idź bokiem (D) wzdłuż ściany i obserwuj spoiny między kamieniami, najlepiej na monitorze o odświeżaniu innym niż 60 albo 120 Hz. Potem przywróć `eye` i zamień `static_cast<float>(alpha)` na stałą `0.0F`: dlaczego obraz jest wtedy płynny, ale spóźniony o cały krok?
7. **Wysokość oczu.** Zmień `Player::EYE_HEIGHT` na `1.0F` i uruchom testy. Który przypadek przestaje przechodzić? Uruchom program: jak zmienia się wrażenie skali labiryntu?

## 9. Pytania kontrolne

1. **Jak ruch myszy zamienia się w obrót kamery?**
   Raz na klatkę, w `onRender`, gdy kursor jest przechwycony: `yawDelta = mouseDeltaX * czułość`, `pitchDelta = -mouseDeltaY * czułość`, potem `m_camera.rotate(yawDelta, pitchDelta)`. Czułość to stopnie na jednostkę współrzędnych ekranu (domyślnie 0,1). `rotate` zawija yaw i przycina pitch.

2. **Skąd minus przy `mouseDeltaY`?**
   Współrzędna y ekranu rośnie w dół, a pitch rośnie przy patrzeniu w górę. Ruch myszy do góry daje ujemne przesunięcie, więc znak trzeba odwrócić. Przy x znaku nie zmieniam, bo ruch w prawo jest dodatni i dodatni yaw obraca w prawo.

3. **Dlaczego obrót jest w `onRender`, a ruch w `onUpdate`?**
   Przesunięcie myszy to dane jednej klatki: trzeba je zastosować dokładnie raz, a `onUpdate` wykonuje się od zera do wielu razy na klatkę. Ruch zależy od czasu trzymania klawisza, a czas symulacji płynie stałymi krokami: `isKeyDown` jest stanem ciągłym, bezpiecznym w każdym kroku, a droga `prędkość * fixedDt` nie zależy od FPS.

4. **Kto ustawia pozycję kamery?**
   `onUpdate`, po każdym kroku: `m_camera.position = m_player.eyePosition()`. Kamera podąża za graczem i nie ma własnego sterowania pozycją. Dlatego panel pozwala przeciągać `Player feet`, a `Eye` tylko pokazuje.

5. **Z jakiego punktu rysowana jest klatka?**
   Nie z `m_camera.position`, tylko z oka policzonego w `onRender`: `glm::mix(poprzednie stopy, bieżące stopy, alpha)` plus `(0, Player::EYE_HEIGHT, 0)`. Ten punkt jest parametrem `viewMatrix(eye)`.

6. **Po co `m_previousPlayerPosition` i jak działa interpolacja?**
   Pozycja zmienia się co krok symulacji (8,33 ms), a klatka wypada między krokami. Na początku każdego kroku zapamiętuję pozycję sprzed kroku. `onRender` miesza ją z bieżącą w proporcji `alpha` z zakresu od 0 do 1. Oko przesuwa się wtedy w każdej klatce o tyle samo, także w klatce bez kroku. Ceną jest opóźnienie obrazu o najwyżej jeden krok.

7. **Co się stanie, gdy patrzę w górę i trzymam W?**
   Zależy od trybu gracza. Przy chodzeniu nic szczególnego: idę poziomo z pełną prędkością, bo ruch używa tylko yaw. W trybie noclip wznoszę się, bo W przesuwa wzdłuż kierunku patrzenia, który ma składową pionową `sin(pitch)`.

8. **Co widać, gdy zmienię `Player feet` z panelu?**
   Gracz i kamera przeskakują. Panel pisze do `m_player.position`, a najbliższy krok kopiuje tę wartość do pozycji poprzedniej. Jeśli między zmianą a krokiem trafi się klatka bez kroku, ta jedna klatka jest rysowana z punktu pośredniego. Przy chodzeniu y wraca do zera w najbliższym kroku.

9. **Dlaczego ruch działa tylko przy przechwyconym kursorze?**
   Żeby była jedna reguła: kliknięcie w scenę włącza całe sterowanie (mysz i klawisze), Escape całe wyłącza. Przy widocznym kursorze pracuję z panelami i przypadkowe naciśnięcie W nie powinno przesuwać gracza. Wyjątkiem są klawisze N, F i R: przełączenie trybu, latarka i restart rundy działają także przy wolnym kursorze.

10. **Co pokazuje panel Camera i dlaczego suwak `Pitch` ma flagę `ImGuiSliderFlags_AlwaysClamp`?**
    Tryb gracza, stopy gracza (edytowalne), oko (odczyt), yaw, pitch, FOV, bliską i daleką płaszczyznę, czułość myszy i trzy prędkości. Pole `pitchDegrees` jest publiczne i zakresu pilnuje tylko `Camera::rotate`, a suwak ImGui pozwala wpisać liczbę z klawiatury. Flaga przycina wpisaną wartość do granic suwaka, czyli do `MAX_PITCH_DEGREES`.

11. **Co się dzieje z obrazem, gdy zwiększam `Near plane`, i dlaczego FOV działa jak zoom?**
    Bliska płaszczyzna obcina wszystko, co bliżej kamery: najbliższe ściany zostają przecięte i widać przez nie dalszą część sceny. FOV zmienia współczynnik `1 / tan(fov / 2)`, przez który macierz rzutowania mnoży x i y: mniejszy kąt to większy współczynnik i większy obraz, bez ruchu kamery.

12. **Gdzie są pozycje i rozmiary panelu przy pierwszym uruchomieniu i co je nadpisuje?**
    W stałej `CAMERA_PLACEMENT` w `src/debug/PanelLayout.hpp` (róg okna, odsunięcie od niego, rozmiar i `.collapsed = true`). Panel przekazuje ją do `placePanelOnFirstUse`, a ta funkcja ustawia pozycję, rozmiar i stan zwinięcia z warunkiem `ImGuiCond_FirstUseEver`. Panel startuje więc jako pasek tytułu przy górnej krawędzi, obok lewej kolumny, obok zwiniętego panelu Gameplay (`GAMEPLAY_PLACEMENT`), i rozwija się do 280 na 416 jednostek. Nadpisuje to wpis w pliku `imgui.ini`, w którym ImGui zapamiętuje układ ustawiony przez użytkownika.

13. **Do czego jeszcze, poza macierzą widoku, służą oko i kierunek patrzenia kamery?**
    Do ustawienia latarki. `onRender` podaje to samo interpolowane `eye` oraz `m_camera.forward()` i `m_camera.right()` do `flashlightPose`, więc reflektor stoi w stałym miejscu względem punktu, z którego rysowana jest klatka (od piątej części M7 w ręce, a nie w oku), i celuje w punkt na osi mojego widoku. `eye` trafia też do `m_lightRig.upload` jako pozycja kamery potrzebna shaderowi do połysku. Latarkę przełącza klawisz F, czytany w `onRender` zaraz po klawiszu N. Od M5 latarka ma baterię i przy pustej gaśnie sama.

14. **Co klawisz R robi z kamerą?**
    Zaczyna rundę od nowa na tym samym labiryncie: `onRender` woła `beginRound`, a ta funkcja stawia gracza na starcie (obie pozycje, bieżącą i poprzednią, naraz), ustawia `m_camera.position` na jego oczy, `yawDegrees` na `startYawDegrees` labiryntu i `pitchDegrees` na 0. Kąty są wpisywane wprost w pola, bez `rotate`, bo obie wartości mieszczą się w zakresach. FOV, płaszczyzn, czułości i trybu noclip restart nie zmienia.

## 10. Źródła

- Glenn Fiedler, "Fix Your Timestep!", Gaffer on Games: <https://gafferongames.com/post/fix_your_timestep/> (stały krok i interpolacja stanu z `alpha`, sekcja 2.4).
- LearnOpenGL, rozdział "Camera": <https://learnopengl.com/Getting-started/Camera> (część "Walk around" i "Look around": ruch klawiszami, obrót myszą, czułość).
- Dokumenty w tym repozytorium: [`camera.md`](camera.md) (struktura `Camera`), [`transforms.md`](transforms.md) (przestrzenie i macierz modelu), [`../game/player.md`](../game/player.md) (gracz: ruch, tryby, kolizje, testy), [`collision.md`](collision.md) (panel Collision z przełącznikiem noclip), [`../core/main-loop.md`](../core/main-loop.md) (stały krok i `alpha`), [`../core/input.md`](../core/input.md) (mysz, przechwycenie kursora, blokady), [`../debug-ui.md`](../debug-ui.md) (podpięcie panelu Camera, `setMouseEnabled`, układ paneli i panel zwinięty), [`../game/flashlight.md`](../game/flashlight.md) (latarka, klawisz F, `buildLightSet`), [`../game/gameplay.md`](../game/gameplay.md) (runda, restart klawiszem R, bateria), [`../../libraries/glm.md`](../../libraries/glm.md) (`mix`, `normalize`, `value_ptr`), [`../../libraries/imgui.md`](../../libraries/imgui.md) (biblioteka Dear ImGui).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)), sekcja 10: lista paneli (panel Camera).
- PRD ([`../../PRD.pdf`](../../PRD.pdf)), sekcja 3: temat 3 i jego pokaz w ImGui.
