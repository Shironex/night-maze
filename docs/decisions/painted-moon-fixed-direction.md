# Księżyc namalowany na niebie, w domyślnym kierunku światła księżyca

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`tools/blender/make_skybox.py`](../../tools/blender/make_skybox.py) (`MOON_LIGHT_YAW_DEGREES`, `MOON_LIGHT_PITCH_DEGREES`, `direction_from_angles`, `moon_layers`), [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) (`moonYawDegrees`, `moonPitchDegrees` i komentarz nad nimi), [`tests/SkyboxTests.cpp`](../../tests/SkyboxTests.cpp) (przypadek `the moon is painted where the default moon light comes from`), [`src/debug/panels/RendererPanel.cpp`](../../src/debug/panels/RendererPanel.cpp) (podpowiedź przy polu `Skybox`), [`assets/skybox/py.png`](../../assets/skybox/py.png) (ściana z tarczą). Dokument modułu: [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcje 2.8, 2.9 i 5.8.

## 1. Kontekst

Od M4 scenę oświetla księżyc: światło kierunkowe o kierunku z dwóch kątów, `moonYawDegrees = 25` i `moonPitchDegrees = -50` w `game::LightingSettings`. Kąty da się zmieniać suwakami `Moon yaw` i `Moon pitch` w panelu Lights. Do M5 samego księżyca nie było widać: nad ścianami był jeden kolor.

W pierwszej części M6 doszło niebo, tekstura sześcienna z sześciu obrazów generowanych skryptem. Nocne niebo bez księżyca wyglądałoby dziwnie w scenie, w której ściany są wyraźnie oświetlone z jednej strony. Trzeba było zdecydować, czy księżyc ma być na niebie i skąd ma wiedzieć, gdzie stać.

Ograniczenia: temat 8 dotyczy tekstury sześciennej, a nie rysowania ciał niebieskich. Obrazy powstają w Pythonie, poza grą. Kierunek światła nie zmienia się w rozgrywce: zmienia go tylko panel debug.

## 2. Decyzja

Tarcza księżyca z poświatą jest **częścią obrazu nieba**. Skrypt maluje ją w kierunku przeciwnym do kierunku, w którym leci domyślne światło księżyca, czyli tam, skąd to światło przychodzi. Dwa kąty domyślne są zapisane drugi raz, w skrypcie (`MOON_LIGHT_YAW_DEGREES`, `MOON_LIGHT_PITCH_DEGREES`). Zgodności obu par pilnuje test na plikach. Tarcza nie podąża za suwakami panelu Lights i mówi o tym podpowiedź przy polu `Skybox`.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Tarcza namalowana w obrazie, w domyślnym kierunku światła (wybrana)** | zero dodatkowego kodu w shaderze i w grze: niebo to nadal jeden odczyt tekstury. Tarcza, plamy i poświata powstają tą samą matematyką co gwiazdy i przechodzą przez krawędzie ścian bez szwów. Scena startowa jest spójna: ściany są jasne od strony księżyca | obraz jest stały: suwaki `Moon yaw` i `Moon pitch` rozjeżdżają światło i tarczę. Dwie liczby w dwóch plikach, w C++ i w Pythonie. Zmiana kierunku wymaga Blendera |
| Niebo bez księżyca | nic do uzgadniania | światło kierunkowe bez widocznego źródła. Nocne niebo traci najważniejszy element |
| Tarcza liczona w `skybox.frag` z bieżącego kierunku światła | podąża za suwakami, jedna liczba w jednym miejscu | shader nieba musiałby dostać kierunek światła (nowy uniform albo blok `LightBlock`), a tarczę, jej miękki brzeg, plamy i dwie warstwy poświaty trzeba by napisać w GLSL. To więcej kodu shadera niż cała reszta `skybox.frag`, na temat, którego wykład nie dotyczy |
| Osobny model księżyca (prostokąt z teksturą zwrócony do kamery) | podąża za suwakami, obraz tarczy zostaje w pliku | nowa siatka, nowy przebieg rysowania, przezroczystość i mieszanie kolorów, których w grze jeszcze nie ma, pytanie o kolejność względem nieba |
| Obracanie całej tekstury sześciennej razem ze światłem | jedna macierz obrotu w shaderze wierzchołków, tarcza zostaje w obrazie | razem z księżycem obraca się horyzont: jaśniejszy pas przestaje być poziomy przy zmianie kąta pitch. Poprawne byłoby tylko dla obrotu wokół osi pionowej |

## 4. Uzasadnienie i skutki

**Dlaczego namalowana.** Rozjazd tarczy i światła może wywołać tylko panel debug, czyli ja podczas pokazu. W rozgrywce kierunek światła jest stały i równy wartości domyślnej, więc dla gracza obie rzeczy zawsze się zgadzają. Cena dynamicznej tarczy (shader, uniform, sprzężenie programu nieba z oświetleniem) byłaby płacona za przypadek, który w grze nie występuje.

**Dlaczego w kierunku przeciwnym do kątów.** Kąty opisują kierunek, w którym światło **leci** (w dół, od księżyca do sceny). Księżyc stoi tam, skąd leci, więc skrypt bierze `-direction_from_angles(25, -50)`, czyli `(-0,272, 0,766, 0,583)`: 50 stopni nad horyzontem. Kamera widzi go w środku ekranu przy yaw 205 i pitch 50.

**Jak sprzężenie jest pilnowane.** Trzema drogami naraz. Komentarze w obu plikach wskazują na siebie nawzajem. Test `the moon is painted where the default moon light comes from` czyta wartości domyślne z `game::LightingSettings`, liczy z nich kierunek do księżyca funkcją gry `scene::directionFromAngles` i sprawdza, że piksel nieba w tym kierunku jest prawie biały, a kierunki daleko od niego ciemne. Podpowiedź przy polu `Skybox` mówi użytkownikowi panelu, czego się spodziewać.

**Co przez to tracę.**

- Suwaki `Moon yaw` i `Moon pitch` zmieniają światło, a tarcza stoi. Przy dużej zmianie kąta yaw widać księżyc z jednej strony i jasne ściany z drugiej. Suwak `Moon pitch` (od -90 do -5 stopni) pozwala ustawić światło prawie poziome, jak od księżyca tuż nad horyzontem, a tarcza stoi nadal 50 stopni nad nim.
- Test nie jest dokładny. Sprawdza jeden piksel, a tarcza ma promień 2,2 stopnia, więc zmiana wartości domyślnej o mniej niż około 2 stopnie na niebie (około 2 stopni kąta pitch albo około 3 stopni kąta yaw) przechodzi niezauważona. Komentarze w `Lighting.hpp` i w skrypcie mówią to wprost: test nie przechodzi dopiero wtedy, gdy obie strony różnią się o więcej niż około 2 stopnie.
- Zmiana domyślnego kierunku księżyca to trzy kroki zamiast jednego: liczby w `Lighting.hpp`, te same liczby w skrypcie, uruchomienie skryptu w Blenderze.
- Tarcza jest zapisana w kanale 8-bitowym, więc nie może być jaśniejsza od bieli. Suwak `Sky brightness` powyżej 1 ją przepala.

## 5. Kiedy wrócić do tej decyzji

- Gdy kierunek światła księżyca zacznie się zmieniać w rozgrywce (pora nocy, księżyc wędrujący po niebie): wtedy stały obraz jest po prostu błędny i tarcza musi być liczona z bieżącego kierunku.
- W M7, gdy księżyc zacznie rzucać cienie: kierunek cieni będzie wtedy widać znacznie wyraźniej niż różnicę jasności ścian, więc rozjazd po ruszeniu suwaka stanie się bardziej rażący. Warto wtedy co najmniej zablokować suwaki albo obracać niebo wokół osi pionowej.
- W M7, przy buforze HDR i bloomie: tarcza powinna być wtedy jaśniejsza od 1, żeby dostać poświatę z bloomu, a tego obraz 8-bitowy nie zapisze. To argument za tarczą liczoną w shaderze albo za osobnym mnożnikiem jasności dla najjaśniejszych tekseli.
- Gdy test przepuści niezgodność, która będzie widoczna na ekranie: wtedy trzeba go zaostrzyć, na przykład porównaniem środka ciężkości jasnych pikseli z kierunkiem światła zamiast odczytu jednego piksela.
