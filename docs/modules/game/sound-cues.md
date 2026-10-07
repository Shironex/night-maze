# Moduł game: dźwięki gry, czyli reguły "co się stało, to ten dźwięk"

Kamień milowy: pierwszy kawałek dźwięku, wersja 0.10.0 (2026-10-07). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z rundy ([`gameplay.md`](gameplay.md): `Round`, bateria, brama), z latarki ([`flashlight.md`](flashlight.md): migotanie przy słabej baterii), z dźwigni ([`interactables.md`](interactables.md)), z ustawień ([`settings.md`](settings.md): głośność) i z biblioteki, która dźwięki odtwarza ([`../audio/README.md`](../audio/README.md)).
Kod: [`src/game/SoundCues.hpp`](../../../src/game/SoundCues.hpp) i [`SoundCues.cpp`](../../../src/game/SoundCues.cpp) (w bibliotece `game_logic`: dane i czyste funkcje, bez karty dźwiękowej), użycie w [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) i [`.hpp`](../../../src/game/NightMazeApp.hpp) (konstruktor, `playCue`, `applyAudioSettings`, `handleControlChanges`, `beginRound`, `onUpdate`, `onRender`, `handleInteraction`). Testy: [`tests/SoundCueTests.cpp`](../../../tests/SoundCueTests.cpp) (15 przypadków).

**Stan na dziś:** gra ma **siedem dźwięków** (`SoundCue`): włączenie i wyłączenie latarki, martwa latarka, puls słabej baterii, zebranie kryształu, pociągnięcie dźwigni i otwarcie bramy. Reguły, który dźwięk należy do jakiego zdarzenia, są zwykłymi funkcjami bez dźwięku w środku, więc mają testy. `NightMazeApp` pyta je w kilku miejscach i oddaje odpowiedź do `audio::AudioEngine`.

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno:

1. **Zgłoszone przez autora kodu (2026-10-07), nie powtórzone przy pisaniu tego dokumentu:** testy `SoundCueTests.cpp` (15 przypadków, napisane i uruchomione przez autora). Nie uruchamiałem testów przy pisaniu i nie podaję liczby asercji ani wyniku bramki.
2. **Nikt nie słyszał tych dźwięków**, więc nie wiadomo, czy zdarzenia i dźwięki trafiają się w grze tak, jak testy zakładają. Hooki w `NightMazeApp` nie mają testów jednostkowych (wymagają okna).
3. **macOS nie był budowany.**

## 1. Po co to jest

### 1.1 Podział: reguły osobno, odtwarzanie osobno

Biblioteka `audio` zna tylko pliki i numery. Gra zna zdarzenia: "kryształ został zebrany", "bateria się skończyła". Ktoś musi je połączyć, i ta część jest trudna do zepsucia po cichu, bo dźwięk nie wyrzuca błędu: pomylone zdarzenie po prostu gra nie tam, gdzie trzeba. Dlatego:

- **`game/SoundCues.*`** (w `game_logic`) odpowiada na pytanie "który dźwięk" i "czy teraz uderzenie pulsu". To zwykłe dane i czyste funkcje, jak reszta `game_logic`, z testami bez okna i bez karty dźwiękowej.
- **`NightMazeApp`** pyta o te reguły w kilku miejscach, w których coś się dzieje, i woła `playCue`.
- **`audio::AudioEngine`** wie o plikach i numerach, nic o grze.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzja właściciela dotyczy tylko biblioteki ([`../../decisions/audio-on-miniaudio.md`](../../decisions/audio-on-miniaudio.md)) i sposobu zrobienia dźwięków ([`../../decisions/sounds-generated-by-script.md`](../../decisions/sounds-generated-by-script.md)). **Wszystko poniżej jest wyborem wykonawczym autora kodu:** lista siedmiu dźwięków, które zdarzenie ma który dźwięk, to, że dwa kryształy w jednym kroku to jeden dźwięk, tempo pulsu, to, że dźwięk grany pod menu nie jest ucinany, i to, że suwak głośności gra próbkę.

## 2. Teoria

### 2.1 Zdarzenie wykrywa się przez porównanie "przed" i "po"

`game::updateRound` zmienia rundę i nie zgłasza, co zrobił. Zamiast go zmieniać, kod **kopiuje kilka liczb przed krokiem** (`RoundSoundSnapshot`) i po kroku porównuje je z rundą. Co się różni, to się stało:

| Różnica | Dźwięk |
|---|---|
| więcej zebranych kryształów niż przed krokiem | `CrystalPickup` |
| brama była zamknięta, jest otwarta | `GateOpen` |
| bateria miała ładunek, jest pusta, a latarka z włączonej stała się wyłączona | `FlashlightDead` |

Porównanie "więcej niż" (a nie "inne niż") ma powód: licznik, który zmalał, należy do innej rundy i nie jest zebraniem. Kolejność w liście wyników jest kolejnością z tabeli.

**Dwa kryształy w jednym kroku to jeden dźwięk**: dwie kopie jednego dźwięku w tej samej chwili to tylko jeden głośniejszy dźwięk. **Kryształ, który otwiera bramę, daje oba dźwięki, najpierw zebranie** (jest na to test).

**Martwa latarka w kroku i klawisz F.** Naciśnięcie F przy pustej baterii daje `FlashlightDead` od razu (reguła klawisza, niżej). Potem najbliższy krok wyłącza przełącznik z powrotem. Gdyby krok oceniał tylko przełącznik, zagrałby drugi raz. Dlatego reguła pyta o ładunek **przed** krokiem: dźwięk jest tylko wtedy, gdy bateria miała ładunek i skończyła się w tym kroku z włączoną latarką. Kryształ zebrany w tym samym kroku ładuje baterię, zanim to jest sprawdzane, więc uratowana latarka nie wydaje dźwięku umierania.

### 2.2 Klawisz latarki

`flashlightKeyCue(battery, wasOn)`: przy pustej baterii (`battery <= 0`) zawsze `FlashlightDead`, bez względu na przełącznik; w przeciwnym razie przełącznik się odwraca i dźwięk jest tym nowego stanu (`FlashlightOn` albo `FlashlightOff`). Dźwięk jest wybierany **przed** odwróceniem przełącznika.

### 2.3 Puls słabej baterii

Gdy bateria jest słaba, a latarka świeci, gra daje powtarzające się uderzenie, jak bicie serca. Warunki:

- `batteryIsLow(battery, settings)`: `battery > 0` i `battery < settings.lowBatteryThreshold`. To ten sam zakres, w którym migocze światło (`flashlightFlicker`). Próg domyślny to 0,2 (`GameplaySettings::lowBatteryThreshold`); przy progu 0 puls nigdy nie gra.
- `lowBatteryPulseSounds`: runda jest w stanie `Playing`, latarka włączona i bateria słaba. Przy wyłączonej latarce nic się nie rozładowuje, więc nie ma przed czym ostrzegać, a pusta bateria jest cicha jak jej światło.

**Odstęp między uderzeniami** rośnie z szybkością w miarę rozładowania: `LOW_BATTERY_PULSE_SLOW_SECONDS` = 2,0 s tuż poniżej progu, `LOW_BATTERY_PULSE_FAST_SECONDS` = 0,8 s tuż przed zerem, a pośrodku liniowo względem ładunku:

`weakness = 1 - battery / threshold`,  `interval = 2,0 + (0,8 - 2,0) * weakness`.

**Przykład policzony ręcznie** (próg 0,2): bateria 0,1 daje `weakness = 1 - 0,1 / 0,2 = 0,5` i odstęp `2,0 - 1,2 * 0,5 = 1,4 s`. Bateria 0,05 daje `weakness = 0,75` i odstęp `2,0 - 0,9 = 1,1 s`. Bateria powyżej progu daje odstęp wolny, 2,0 s (poza zakresem funkcja zwraca właśnie to, więc dzielenie przez próg 0 jest bezpieczne).

### 2.4 Zegar pulsu liczy w krokach symulacji

`LowBatteryPulse` ma jedno pole, `secondsToNextBeat`, a `advanceLowBatteryPulse` zmniejsza je o jeden **stały krok symulacji** (`Time::FIXED_DT` = 1/120 s), a nie o klatkę gry. Dzięki temu uderzenia przypadają w tych samych chwilach przy każdej liczbie klatek na sekundę. Reguły:

1. **Bateria nie jest słaba** (naładowana kryształem albo pusta): zegar wraca do 0, więc przy następnym osłabieniu **pierwsze uderzenie jest od razu**.
2. **Bateria jest słaba, ale puls milczy** (latarka wyłączona, runda skończona): zegar stoi. Wyłączenie i włączenie światła nie daje dodatkowego uderzenia.
3. **W pozostałych przypadkach** zegar biegnie. Gdy dojdzie do zera lub poniżej, jest uderzenie (funkcja zwraca `true`), a zegar jest **ustawiany** na odstęp dla bieżącego ładunku.

Ustawianie, a nie dodawanie odstępu, ma powód z komentarza: po bardzo długim kroku dodawanie zostawiłoby ujemny czas i kolejne kroki uderzałyby jedno po drugim, aż wszystko się wyrówna. Jedno wywołanie daje **najwyżej jedno uderzenie**, jest na to test (`one very long step is one beat, and no burst follows it`). Nowa runda zeruje zegar przypisaniem nowego `LowBatteryPulse`.

## 3. Jak to działa w OpenGL

Nie dotyczy: reguły dźwięków nie używają OpenGL.

## 4. Shadery

Nie dotyczy.

## 5. Kod w projekcie

### 5.1 Plik z regułami

| Element | Co |
|---|---|
| `enum class SoundCue` | siedem wartości od 0: `FlashlightOn`, `FlashlightOff`, `FlashlightDead`, `LowBatteryPulse`, `CrystalPickup`, `LeverPull`, `GateOpen` |
| `SOUND_CUE_COUNT` | 7 |
| `soundCueIndex`, `soundCueFile`, `soundCueName` | numer, plik (względem `assets/`, na przykład `audio/crystal_pickup.wav`) i nazwa do okna debug (`crystal pickup`) |
| `flashlightKeyCue` | dźwięk jednego naciśnięcia klawisza latarki |
| `RoundSoundSnapshot`, `soundSnapshot`, `roundStepCues` | wykrywanie zdarzeń kroku |
| `batteryIsLow`, `lowBatteryPulseSounds`, `lowBatteryPulseInterval`, `advanceLowBatteryPulse`, `LowBatteryPulse` | puls |

**Tabela plików.** W `SoundCues.cpp` jest jedna tablica `SOUND_CUES` o długości `SOUND_CUE_COUNT`, w kolejności wyliczenia: numer wpisu jest numerem dźwięku. To jedyne miejsce, w którym są zapisane nazwy plików. Dźwięk dopisany do wyliczenia i do licznika bez wiersza w tablicy zostawia wpis z dwoma pustymi wskaźnikami, który znajduje test `every cue has a file and a name of its own`.

### 5.2 Wczytanie w konstruktorze

```cpp
std::array<std::filesystem::path, SOUND_CUE_COUNT> soundFiles;
for (std::size_t i = 0; i < SOUND_CUE_COUNT; ++i) {
    soundFiles.at(i) = core::assetPath(soundCueFile(static_cast<SoundCue>(i)));
}
m_audio.load(soundFiles);
```

(Plik: `src/game/NightMazeApp.cpp`, konstruktor `NightMazeApp`, bez komentarzy.) Numer dźwięku w bibliotece `audio` jest numerem wartości wyliczenia, bo lista plików jest zbudowana w tej samej kolejności. Brakujący plik jest w logu, a jego dźwięk cichy.

### 5.3 `playCue`

```cpp
void NightMazeApp::playCue(SoundCue cue) {
    m_audio.play(soundCueIndex(cue));
    m_lastCueName = soundCueName(cue);
    ++m_cuesPlayed;
}
```

(Plik: `src/game/NightMazeApp.cpp`, bez komentarza.) Pamięta też nazwę ostatniego dźwięku i licznik dla karty Audio w oknie debug. **Licznik rośnie także przy braku karty dźwiękowej.**

### 5.4 Gdzie dźwięki są wywoływane

| Miejsce | Funkcja | Dźwięk |
|---|---|---|
| krok symulacji | `onUpdate` | `roundStepCues(soundBefore, ...)`: `CrystalPickup`, `GateOpen`, `FlashlightDead`, a potem `LowBatteryPulse`, gdy `advanceLowBatteryPulse` zwróci `true` |
| klawisz F | `onRender` (raz na klatkę, tylko gdy `roundInput`) | `flashlightKeyCue(m_round.battery, m_lighting.flashlightOn)`, wołane przed odwróceniem przełącznika |
| przycisk "pull all levers" okna debug | `onRender` | jedno `LeverPull` dla wszystkich dźwigni naraz, tylko gdy `roundInput` i `pullAllLevers(...) > 0` |
| kliknięcie albo klawisz użycia na dźwigni | `handleInteraction` | `LeverPull`, gdy `interact(...)` zwróci `true` (pierwsze pociągnięcie) |
| nowa runda | `beginRound` | brak dźwięku: `m_lowBatteryPulse = {}` zeruje zegar pulsu |
| suwak głośności | `handleControlChanges` | `FlashlightOn` jako próbka głośności, najwyżej raz na 0,2 s (`VOLUME_SAMPLE_SECONDS`) |

Krok symulacji:

```cpp
const RoundSoundSnapshot soundBefore = soundSnapshot(m_round, m_lighting.flashlightOn);
updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn,
            static_cast<float>(fixedDt));
for (const SoundCue cue : roundStepCues(soundBefore, m_round, m_lighting.flashlightOn)) {
    playCue(cue);
}
if (advanceLowBatteryPulse(m_lowBatteryPulse, m_round, m_lighting.flashlightOn, m_gameplay,
                           static_cast<float>(fixedDt))) {
    playCue(SoundCue::LowBatteryPulse);
}
```

(Plik: `src/game/NightMazeApp.cpp`, funkcja `onUpdate`, bez komentarzy.) Migawka jest brana **tuż przed każdym krokiem**, więc nowa runda niczego nie wywołuje (także ta, która zaczyna się z otwartą bramą, w labiryncie bez kryształów albo bez bramy): porównuje się zawsze z chwilą bezpośrednio przed krokiem. Dlatego `beginRound` nie musi czyścić migawki, a czyści tylko zegar pulsu, który jest stanem przechodzącym z kroku na krok.

**Pod menu nie ma dźwięków rundy.** `onUpdate` wraca wcześniej, gdy runda nie jest rozgrywana (menu, pauza, kamera menu), więc ani zdarzenia kroku, ani puls się nie wywołują, a zegar pulsu stoi. Klawisze rundy (F, dźwignie, „pull all levers") są czytane tylko przy `roundInput`.

### 5.5 Dźwięk, który zaczął grać, gra do końca, także pod menu

Nic nie zatrzymuje dźwięku celowo: każdy jest jednorazowym odgłosem krótszym niż dwie sekundy, więc najdłuższą rzeczą słyszalną pod menu jest ogon bramy (plik trwa 1,85 s), a zatrzymywanie dźwięków przy pauzie wymagałoby sposobu ich wznawiania. Komentarz nad `playCue` mówi to wprost. Jedyny dźwięk, który samo menu wydaje, to **próbka suwaka głośności** na ekranie ustawień: ma być słyszana.

Próbka to dźwięk `FlashlightOn` zagrany przy zmianie głośności, nie częściej niż co 0,2 s (suwak zgłasza nową wartość prawie w każdej klatce przeciąganej myszy, a klik w każdej klatce brzmiałby jak bzyczenie). Czas oczekiwania zmniejsza się o prawdziwy czas klatki. Próbka też zwiększa `Cues played`.

### 5.6 Głośność

`applyAudioSettings()` woła `m_audio.setMasterVolume(masterVolumeGain(m_settings.masterVolume))`. Jest wołane z konstruktora (po wczytaniu pliku ustawień), po każdej zmianie ustawień z ekranu i po `Reset defaults`. Szczegóły ustawienia: [`settings.md`](settings.md).

### 5.7 Testy

`SoundCueTests.cpp`, 15 przypadków, wszystkie bez okna i bez karty dźwiękowej:

| Test | Co sprawdza |
|---|---|
| `every cue has a file and a name of its own` | tablica ma wiersz dla każdego dźwięku, pliki zaczynają się od `audio/` i kończą na `.wav`, nazwy i pliki są różne, ostatnia wartość to ostatni wiersz |
| `the flashlight key clicks on, clicks off, and clicks dead on an empty battery` | `flashlightKeyCue` |
| `a step that changes nothing has no cue` | pusta lista |
| `a collected crystal is one cue, also when the step collected two` | jeden dźwięk na krok |
| `the gate that opens is a cue, the gate that stays open is not` | brama |
| `the crystal that opens the gate gives both cues, the pickup first` | kolejność |
| `a new round fires nothing, whatever the round before looked like` | świeża runda |
| `the battery that runs out with the light on clicks dead, once` | martwa latarka raz |
| `a battery that is empty with the light off made no sound of dying` | cisza przy wyłączonej latarce |
| `the low battery pulse sounds only while a played round has a low, lit battery` | `lowBatteryPulseSounds` |
| `the pulse gets faster as the battery runs down` | odstęp |
| `the pulse beats at once when the battery gets low and then at its interval` | pierwsze uderzenie od razu |
| `the pulse is silent with the light off and with a full or an empty battery` | cisza |
| `one very long step is one beat, and no burst follows it` | jedno uderzenie na krok |
| `switching the light off holds the pulse, a charged battery and a new round reset it` | zegar |

**Nie ma testu na:** hooki w `NightMazeApp` (miejsca z sekcji 5.4: to kod z oknem), kolejność wywołań w klatce ani to, co słychać.

## 6. Okno debugowania (dawniej panel ImGui)

Karta Audio w Diagnostics (zakładka Frame and shaders) pokazuje `Last cue` i `Cues played`. Opis: [`../audio/README.md`](../audio/README.md), sekcja 6. Karta nie ma kontrolek. Przycisk "pull all levers" jest w kategorii Gameplay i gra jeden dźwięk dźwigni.

## 7. Pułapki

1. **Nikt nie słyszał tych dźwięków.** Testy sprawdzają, że reguły wskazują właściwy dźwięk, nie to, że gra go w tej chwili, w której gracz się tego spodziewa.
2. **Hooki nie mają testów.** Pominięte miejsce wywołania to cisza bez błędu.
3. **Migawka musi być z tej samej rundy.** Porównanie z migawką poprzedniej rundy dałoby fałszywe zdarzenie. Dlatego migawka jest brana przed każdym krokiem.
4. **`FlashlightOn` ma dwa znaczenia:** klik latarki i próbka suwaka głośności.
5. **`Cues played` liczy też wywołania bez karty dźwiękowej.**
6. **Dźwięk zaczęty pod menu albo tuż przed wejściem do menu gra do końca.**
7. **Dopisanie dźwięku wymaga czterech miejsc:** wartość wyliczenia, `SOUND_CUE_COUNT`, wiersz tablicy i plik w `assets/audio/` (razem z wierszem w `assets/audio/README.md`).

## 8. Ćwiczenia

1. **Odstęp pulsu.** Próg 0,2, bateria 0,15. Jaki odstęp? Odpowiedź: `weakness = 1 - 0,15 / 0,2 = 0,25`, `2,0 - 1,2 * 0,25 = 1,7 s`.
2. **Ile kroków do uderzenia?** Przy odstępie 1,4 s i kroku 1/120 s? Odpowiedź: 1,4 * 120 = 168 kroków (zegar jest ustawiany na 1,4 s po uderzeniu i maleje o 1/120 s w kroku).
3. **Co zagra?** Latarka świeci, bateria miała 0,001, krok ją opróżnia. Odpowiedź: `FlashlightDead` (przed krokiem miała ładunek, po nim jest pusta, przełącznik się wyłączył).
4. **Co zagra przy F na pustej baterii?** Odpowiedź: `FlashlightDead` od razu, a krok, który wyłączy przełącznik, nie zagra go drugi raz.
5. **Dopisz dźwięk.** Co trzeba dopisać dla ósmego dźwięku? Odpowiedź: sekcja 7, punkt 7.

## 9. Pytania kontrolne

1. **Dlaczego reguły dźwięków są w `game_logic`, a nie w `NightMazeApp`?**
   Żeby mieć testy bez okna i bez karty dźwiękowej. `NightMazeApp` tylko pyta o regułę i oddaje odpowiedź bibliotece `audio`.
2. **Jak gra wie, że kryształ został zebrany, skoro `updateRound` tego nie zgłasza?**
   Kopiuje kilka liczb przed krokiem i porównuje je z rundą po kroku (`roundStepCues`).
3. **Dlaczego dwa kryształy w jednym kroku to jeden dźwięk?**
   Dwie kopie tego samego dźwięku w tej samej chwili to tylko jeden głośniejszy dźwięk.
4. **Dlaczego `FlashlightDead` ma warunek "ładunek przed krokiem"?**
   Żeby krok, który wyłącza przełącznik po naciśnięciu F na pustej baterii, nie zagrał dźwięku drugi raz.
5. **Od czego zależy odstęp pulsu?**
   Od ładunku: 2,0 s tuż poniżej progu, 0,8 s tuż przed zerem, liniowo pomiędzy.
6. **Dlaczego zegar pulsu liczy w krokach symulacji?**
   Żeby uderzenia przypadały w tych samych chwilach przy każdej liczbie klatek na sekundę.
7. **Dlaczego po długim kroku nie ma serii uderzeń?**
   Po uderzeniu zegar jest ustawiany na odstęp, a nie powiększany o niego, więc nie zostaje ujemny zapas.
8. **Co się dzieje z dźwiękiem, gdy gracz wejdzie do menu?**
   Gra do końca. Nowych dźwięków rundy nie ma, bo `onUpdate` wraca wcześniej.
9. **Po co próbka na suwaku głośności i dlaczego co 0,2 s?**
   Żeby gracz od razu usłyszał nową głośność. Suwak zgłasza wartość prawie w każdej klatce, a klik w każdej klatce brzmiałby jak bzyczenie.

## 10. Źródła

- Notatki: [`../../decisions/audio-on-miniaudio.md`](../../decisions/audio-on-miniaudio.md), [`../../decisions/sounds-generated-by-script.md`](../../decisions/sounds-generated-by-script.md).
- Dokumenty: [`../audio/README.md`](../audio/README.md), [`../../libraries/miniaudio.md`](../../libraries/miniaudio.md), [`gameplay.md`](gameplay.md), [`flashlight.md`](flashlight.md), [`interactables.md`](interactables.md), [`settings.md`](settings.md), [`../debug-ui.md`](../debug-ui.md).
- Stały krok symulacji: [`../core/`](../core/) (pętla główna, `Time::FIXED_DT`).
