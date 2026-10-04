# Moduł scene: co jest w scenie i skąd na nią patrzę

Kamień milowy: M1. Temat wykładu: 3 (Przekształcenia przestrzeni).
Kod: [`src/scene/`](../../../src/scene/).

Moduł `gfx` umie narysować to, co dostanie: bufor wierzchołków, program shaderów. Nie wie, **gdzie** w świecie coś stoi ani **skąd** jest oglądane. Na te dwa pytania odpowiada moduł `scene`: opisuje położenie obiektów i kamerę, a z tego opisu liczy macierze, które shader wierzchołków mnoży przez każdy wierzchołek. Docelowo (PRD, sekcja 6) warstwa `scene/` ma zawierać encje, transformy, kamerę, światła, kolizje i selekcję. Na dziś ma dwie struktury: `scene::Transform` i `scene::Camera`.

Obie są gotowe i sprawdzone, ale gra ich jeszcze nie używa: `game::NightMazeApp` nadal rysuje trójkąt bez macierzy. Pierwszym użytkownikiem będzie kostka rysowana z macierzami model, view i projection, w następnym kroku M1. Ten plik jest wstępem do modułu: wspólna zasada obu struktur, miejsce modułu w warstwach i indeks dokumentów.

## 1. Dokumenty modułu

| Dokument | Co opisuje | Struktury i pliki |
|---|---|---|
| [`transforms-camera.md`](transforms-camera.md) | przestrzenie współrzędnych (lokalna, świata, widoku, przycięcia, NDC, okna), współrzędne jednorodne, macierze przesunięcia, obrotu i skali, znaczenie kolejności, kąty Eulera i blokada przegubu, macierz widoku i `lookAt`, kamera FPS (yaw, pitch, wektor kierunku), rzutowanie perspektywiczne, nieliniowa głębia, konwencja układu projektu | `Transform`, `Camera` |

Dokument tematyczny ma te same dziesięć sekcji co dokumenty modułów `core` i `gfx`: Po co to jest, Teoria, Jak to działa w OpenGL, Shadery, Kod w projekcie, Panel ImGui, Pułapki, Ćwiczenia, Pytania kontrolne, Źródła.

Proponowana kolejność czytania: [`../../libraries/glm.md`](../../libraries/glm.md) (typy i funkcje biblioteki), ten plik, potem [`transforms-camera.md`](transforms-camera.md). Wcześniej warto znać [`../gfx/shaders.md`](../gfx/shaders.md), sekcja 2.2, bo tam jest opisane, co shader wierzchołków musi zapisać do `gl_Position`.

## 2. Wspólna zasada: dane i matematyka, bez OpenGL

Klasy `gfx` opakowują obiekty żyjące na karcie graficznej, więc mają konstruktory, destruktory i zakaz kopiowania ([`../gfx/README.md`](../gfx/README.md), sekcja 2). Struktury `scene` są ich przeciwieństwem:

| Cecha | Klasy `gfx` | Struktury `scene` |
|---|---|---|
| co trzymają | identyfikator obiektu OpenGL | kilka liczb: wektory i kąty |
| wywołania `gl*` | w każdej funkcji | żadnego |
| kopiowanie | zabronione (`= delete`), tylko przenoszenie | zwykłe: kopia to drugi, niezależny zestaw liczb |
| pola | prywatne, z prefiksem `m_` | publiczne, bez prefiksu |
| wymaga kontekstu OpenGL | tak, przez całe życie | nie: można je tworzyć przed oknem i używać w programie bez okna |
| zależności | `core/`, GLAD | tylko GLM i biblioteka standardowa |

Z tego wynikają trzy rzeczy:

1. **Macierze liczy procesor.** `Transform::matrix()`, `Camera::viewMatrix()` i `Camera::projectionMatrix()` to zwykłe funkcje C++ zwracające `glm::mat4`. OpenGL dowiaduje się o macierzy dopiero wtedy, gdy kod rysujący wyśle ją do shadera.
2. **Kod da się sprawdzić bez okna.** Wystarczy program konsolowy, który woła funkcje i wypisuje wyniki. Tak została sprawdzona matematyka obu struktur ([`transforms-camera.md`](transforms-camera.md), sekcja 5.8).
3. **Struktury nie znają wejścia ani czasu.** `Camera` nie czyta klawiatury ani myszy i nie ma prędkości ruchu. Ma pola i funkcję `rotate`, a o tym, kiedy i o ile je zmienić, decyduje właściciel kamery. Dzięki temu ta sama kamera nadaje się do gry, do zadania laboratoryjnego i do sterowania z panelu.

Kąty są wszędzie trzymane w stopniach, z jednostką w nazwie pola (`rotationDegrees`, `yawDegrees`, `fovDegrees`), a zamiana na radiany odbywa się w miejscu użycia.

## 3. Miejsce w warstwach

```mermaid
flowchart TD
    Main["main.cpp<br/>DebugNightMazeApp, main"] --> Debug["debug/<br/>DebugUI, panele"]
    Main --> Game["game/<br/>NightMazeApp"]
    Debug --> Core["core/<br/>Application, Window, Input, Time, Log, Paths, GL_CHECK"]
    Game --> Core
    Game --> Gfx
    Debug --> Gfx
    Gfx["gfx/<br/>Shader, Buffer, VertexArray"] --> Core
    Scene["scene/<br/>Transform, Camera"] --> Glm["GLM"]
    Gfx --> Glad["GLAD"]
    Core --> Glad
    Core --> Glfw["GLFW"]
    Debug --> ImGui["Dear ImGui"]
```

Strzałka znaczy "zna i dołącza nagłówki". Diagram pokazuje stan faktyczny, dlatego do `scene/` nie prowadzi jeszcze żadna strzałka: nikt go nie dołącza.

Pełny łańcuch warstw z PRD to `core <- gfx <- renderer <- scene <- game`. Warstwy `renderer/` w M1 nie ma, więc dziś łańcuch to `core <- gfx <- scene <- game`. Zasady dla `scene`:

1. `scene/` **może** zależeć od `core/`, `gfx/` i GLM. Dziś korzysta tylko z GLM: `Transform` i `Camera` nie potrzebują ani okna, ani obiektów OpenGL.
2. `scene/` nie zna `game/`, `debug/`, ImGui ani wejścia. Nie dołącza GLFW: o klawiszach i myszy wie tylko ten, kto steruje kamerą.
3. `core/` i `gfx/` nie znają `scene/`. Zależność idzie w jedną stronę.
4. Użytkownikami będą `game/` (posiada kamerę i transformy, wysyła macierze do shadera) i `debug/` (panel edytujący pola kamery). To dozwolone kierunki.

W [`CMakeLists.txt`](../../../CMakeLists.txt) pliki `src/scene/*` należą do tej samej biblioteki statycznej `engine` co `src/core/*` i `src/gfx/*`. Nic w nich nie jest specyficzne dla Night Maze, więc warstwa nadaje się do zadań laboratoryjnych. GLM jest linkowane do `engine` jako `PUBLIC`, bo nagłówki `scene/` pokazują typy `glm::vec3` i `glm::mat4` w swoim API: każdy, kto je dołączy, musi znaleźć `<glm/glm.hpp>` ([`../../libraries/glm.md`](../../libraries/glm.md), sekcja 2).

## 4. Indeks: plik kodu, dokument

| Plik kodu | Co zawiera | Dokument |
|---|---|---|
| [`src/scene/Transform.hpp`](../../../src/scene/Transform.hpp), [`.cpp`](../../../src/scene/Transform.cpp) | struktura `Transform`: pola `position`, `rotationDegrees`, `scale` i funkcja `matrix()`, która zwraca macierz modelu `T * Ry * Rx * Rz * S` | [`transforms-camera.md`](transforms-camera.md), sekcje 5.2 i 5.3 |
| [`src/scene/Camera.hpp`](../../../src/scene/Camera.hpp), [`.cpp`](../../../src/scene/Camera.cpp) | struktura `Camera`: pola `position`, `yawDegrees`, `pitchDegrees`, `fovDegrees`, `nearPlane`, `farPlane`, stałe `WORLD_UP` i `MAX_PITCH_DEGREES`, funkcje `forward`, `right`, `rotate`, `viewMatrix`, `projectionMatrix` | [`transforms-camera.md`](transforms-camera.md), sekcje od 5.4 do 5.7 |

## 5. Konwencja układu współrzędnych

Jedna konwencja dla całego projektu, opisana dokładnie w [`transforms-camera.md`](transforms-camera.md), sekcja 2.11:

| Ustalenie | Wartość |
|---|---|
| skrętność | układ prawoskrętny (right-handed) |
| góra | +Y (`Camera::WORLD_UP`) |
| przód | -Z (kamera z yaw 0 i pitch 0 patrzy wzdłuż -Z) |
| jednostka | 1 jednostka to 1 metr |
| kąty | w stopniach w polach, w radianach dopiero przy wywołaniu funkcji matematycznych |

To konwencja OpenGL i wartości domyślne GLM. PRD (sekcja 9) ustala te same osie dla modeli eksportowanych z Blendera.

## 6. Pytania kontrolne

Pytania z odpowiedziami do matematyki i kodu są w [`transforms-camera.md`](transforms-camera.md), sekcja 9. Trzy pytania dotyczące treści tego pliku:

1. **Czym struktury `scene` różnią się od klas `gfx`?**
   Klasy `gfx` posiadają obiekt OpenGL: tworzą go w konstruktorze, usuwają w destruktorze i nie dają się kopiować. Struktury `scene` to same liczby z publicznymi polami: nie wołają OpenGL, nie potrzebują kontekstu i kopiują się jak zwykłe dane.

2. **Od czego może zależeć `scene/`, a od czego zależy dziś?**
   Może od `core/`, `gfx/` i GLM. Dziś dołącza tylko GLM i bibliotekę standardową. Nie może znać `game/`, `debug/`, ImGui ani wejścia.

3. **Dlaczego `Camera` nie obsługuje klawiatury i myszy?**
   Bo to kwestia sterowania, a nie kamery. `scene/` jest częścią biblioteki `engine` i ma nadawać się do innych programów. Wejście czyta właściciel kamery i przekłada je na zmiany pól oraz wywołania `rotate`.

## 7. Źródła

- LearnOpenGL, rozdziały "Transformations", "Coordinate Systems" i "Camera": <https://learnopengl.com/Getting-started/Transformations>, <https://learnopengl.com/Getting-started/Coordinate-Systems>, <https://learnopengl.com/Getting-started/Camera>.
- PRD ([`../../PRD.pdf`](../../PRD.pdf)): sekcja 6 (podział na warstwy, zawartość `scene/`), sekcja 9 (konwencja osi).
- Dokument biblioteki: [`../../libraries/glm.md`](../../libraries/glm.md).
- Szczegółowe źródła są w sekcji 10 dokumentu [`transforms-camera.md`](transforms-camera.md).
