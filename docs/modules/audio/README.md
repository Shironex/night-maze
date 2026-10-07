# Moduł audio: odtwarzanie krótkich dźwięków przez miniaudio

Kamień milowy: pierwszy kawałek dźwięku, wersja 0.10.0 (2026-10-07). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z logu i ścieżek ([`../core/paths.md`](../core/paths.md)), z czytania plików (`core::readBinaryFile`, [`../core/paths.md`](../core/paths.md)), z biblioteki miniaudio ([`../../libraries/miniaudio.md`](../../libraries/miniaudio.md)) i z reguł, kiedy który dźwięk gra ([`../game/sound-cues.md`](../game/sound-cues.md)).
Kod: [`src/audio/AudioEngine.hpp`](../../../src/audio/AudioEngine.hpp) i [`AudioEngine.cpp`](../../../src/audio/AudioEngine.cpp) (cała biblioteka: dwa pliki), blok `audio` w [`CMakeLists.txt`](../../../CMakeLists.txt), blok miniaudio w [`cmake/Dependencies.cmake`](../../../cmake/Dependencies.cmake), [`external/miniaudio/miniaudio.c`](../../../external/miniaudio/miniaudio.c), pliki [`assets/audio/*.wav`](../../../assets/audio/) z [`README.md`](../../../assets/audio/README.md) i skrypt [`tools/make_sounds.py`](../../../tools/make_sounds.py). Użycie: [`src/game/NightMazeApp.cpp`](../../../src/game/NightMazeApp.cpp) (`m_audio`, `playCue`, `applyAudioSettings`) i karta Audio w oknie debug. Testów jednostkowych biblioteka **nie ma** (sekcja 5.5).

**Stan na dziś:** biblioteka statyczna `audio` otwiera domyślną kartę dźwiękową, wczytuje siedem krótkich plików WAV raz, w całości do pamięci, i odtwarza je na żądanie. Każdy dźwięk ma **trzy głosy** używane po kolei, więc dźwięk zagrany drugi raz, gdy pierwszy jeszcze wybrzmiewa, nie jest ucinany. **Nie ma funkcji zatrzymania:** dźwięk, który się zaczął, gra do końca, także gdy gracz wejdzie w menu. **Nic z naszego kodu nie działa na wątku dźwięku.** Bez karty dźwiękowej gra działa dalej i jest niema.

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (tak jak w [`../video/README.md`](../video/README.md)):

1. **Zgłoszone przez autora kodu (2026-10-07), nie powtórzone przy pisaniu tego dokumentu:** logi (kod pisze linie `Audio: opened ...` i `Audio: N of M sounds loaded`), testy reguł dźwięków (`SoundCueTests.cpp`, [`../game/sound-cues.md`](../game/sound-cues.md)) i pomiary plików dźwiękowych skryptem `--report`. Nie uruchamiałem testów, bramki ani gry przy pisaniu tego dokumentu i nie podaję liczb z bramki.
2. **Nikt nie słyszał tych dźwięków.** Ani właściciel, ani agent. Czy brzmią dobrze, czy puls słabej baterii jest dość cichy, żeby powtarzać się przez minutę, i czy plik `gate_open.wav` nie jest za głośny, rozstrzyga słuchacz, w grze, w słuchawkach i na głośnikach.
3. **macOS nie był budowany ani uruchamiany**, Linux też nie. Gałąź kodu CMake dla obu systemów jest odczytana z komentarza ([`../../libraries/miniaudio.md`](../../libraries/miniaudio.md), sekcja 4.4).

## 1. Po co to jest

### 1.1 Co robi biblioteka i dlaczego stoi tam, gdzie stoi

Gra była niema. Decyzja właściciela ([`../../decisions/audio-on-miniaudio.md`](../../decisions/audio-on-miniaudio.md)) wybiera bibliotekę miniaudio; ta biblioteka projektu to jej cienka obudowa, która umie trzy rzeczy: **wczytać listę plików**, **zagrać dźwięk o danym numerze** i **ustawić głośność główną**.

Nowa biblioteka statyczna `audio` stoi **obok `ui` i `video`**, nad `engine`, pod `game`. Powody (wybór wykonawczy, z komentarza w `CMakeLists.txt`):

- **Nie wie nic o grze.** Dostaje listę ścieżek i potem numer dźwięku. Co dźwięk znaczy i kiedy gra, rozstrzyga `game/SoundCues.*` w `game_logic`.
- **Nie jest częścią `engine`**, z tego samego powodu co `ui` i `video`: `game_logic` i program testowy linkują `engine`, a nie powinny linkować miniaudio, które otwiera kartę dźwiękową. Dlatego `audio` jest dolinkowana tylko do programu `night_maze`, i **żaden test nie potrzebuje karty dźwiękowej**.
- **Nagłówek nie dołącza miniaudio.** Wszystkie obiekty biblioteki są w strukturze `AudioEngine::Backend`, znanej tylko plikowi `.cpp` (wzór "wskaźnik do implementacji", pimpl). Kod, który używa klasy, nie potrzebuje ani nagłówka miniaudio, ani jego ścieżki.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu:

1. Dźwięk oparty na miniaudio ([`../../decisions/audio-on-miniaudio.md`](../../decisions/audio-on-miniaudio.md), 2026-10-06).
2. Dźwięki mają być zrobione (po tym, jak pierwsze były tymczasowe), a nie pobrane ani nagrane: skrypt w repozytorium ([`../../decisions/sounds-generated-by-script.md`](../../decisions/sounds-generated-by-script.md), 2026-10-07).

**Wszystko inne w tym dokumencie jest wyborem wykonawczym autora kodu:** podział na bibliotekę `audio` i reguły w `game_logic`, wczytanie całego dźwięku do pamięci, trzy głosy na dźwięk, brak funkcji zatrzymania, brak własnych wywołań zwrotnych, format plików (WAV, 16 bitów, 44100 próbek na sekundę), siedem dźwięków i ich głośności względem siebie, to, że klasa nie rzuca wyjątków z powodu braku karty ani pliku.

## 2. Teoria

### 2.1 Próbki, ramki, częstotliwość

Dźwięk w pliku WAV to ciąg **próbek** (samples): liczb mówiących, jak wychylona jest membrana głośnika w kolejnych chwilach. **Częstotliwość próbkowania** to liczba chwil na sekundę. Wszystkie siedem plików gry ma 44100 na sekundę (odczytane z nagłówków plików). **Ramka** (frame) to jedna chwila dla wszystkich kanałów naraz: dla pliku stereo ramka to dwie próbki. Plik `crystal_pickup.wav` ma 55125 ramek, czyli 55125 / 44100 = 1,25 s.

### 2.2 Dlaczego dekodować raz i trzymać w pamięci

Pliki mają od 0,10 s do 1,85 s, razem 698852 bajty (odczytane z plików: 0,7 MB). Taki dźwięk można zdekodować w całości przy starcie. Wtedy w chwili odtwarzania nie ma żadnej pracy poza wskazaniem miejsca w gotowych próbkach: nic nie czyta dysku i nie dekoduje w środku klatki. Gdyby dźwięki były długie (muzyka), trzeba by je strumieniować; **tego kod nie robi**.

Dekodowanie odbywa się od razu **do formatu, w którym silnik miksuje** (32-bitowe liczby zmiennoprzecinkowe, liczba kanałów i częstotliwość urządzenia), więc w trakcie gry nie trzeba niczego konwertować.

### 2.3 Głos: dlaczego trzy kopie jednego dźwięku

Gdy ten sam dźwięk zostanie zagrany drugi raz, zanim pierwszy się skończył (kryształy zbierane jeden po drugim), są dwa wyjścia: uciąć pierwszy i zacząć od nowa albo zagrać oba naraz. Ucięcie dźwięku w środku jest słyszalne jako trzask (membrana głośnika skacze), więc kod robi drugie.

Miejsce, do którego dźwięk doszedł, jest trzymane w **czytniku** (`ma_audio_buffer_ref`, pole `cursor`). Dwie kopie na jednym czytniku dzieliłyby jedno miejsce i nie mogłyby grać osobno. Dlatego każdy dźwięk ma `VOICE_COUNT = 3` głosów, a każdy głos ma czytnik własny; czytnik **nie kopiuje próbek**, tylko na nie wskazuje. Próbki istnieją raz.

`play` bierze głosy **po kolei** (`nextVoice`, potem `+ 1` modulo 3), więc następny głos to zawsze ten, który zaczął grać najdawniej. Dopiero czwarte zagranie jednego dźwięku w czasie jego długości zaczyna głos od nowa, z możliwym trzaskiem. Komentarz w kodzie zapisuje to jako znane ograniczenie i mówi, że gra tego nie robi (najdłuższy dźwięk trwa niecałe dwie sekundy) oraz że w razie potrzeby trzeba zwiększyć `VOICE_COUNT`.

### 2.4 Wątki: kto gra i co jest bezpieczne

Gdy urządzenie zostaje otwarte, miniaudio uruchamia **wątek dźwięku** (na macOS system woła miniaudio z własnego wątku). Co kilkaset razy na sekundę ten wątek prosi silnik o kolejne milisekundy dźwięku, a silnik miksuje to, co aktualnie gra (komentarz w nagłówku klasy).

Z tego wynikają dwie rzeczy, które komentarz w nagłówku podkreśla:

- **Żaden nasz kod nie działa na tym wątku**: nie zarejestrowano żadnego wywołania zwrotnego tej klasy w miniaudio. `play` i `setMasterVolume` ustawiają tylko liczby, które wątek dźwięku odczyta przy następnym obiegu (miniaudio samo je chroni), więc klasa **nie ma mutexu**.
- Dlatego nic, co robi gra (wolna klatka, budowa labiryntu), nie może zająć wątku dźwięku i **nie przerywa dźwięku**.

Wszystkie funkcje klasy są wołane z jednego wątku, wątku gry.

### 2.5 Głośność: kwadrat zamiast prostej

Głośność gracza to liczba od 0 do 100, a silnik dostaje **współczynnik** od 0 do 1. Ucho słyszy głośność w stosunkach, więc prosta linia sprawiłaby, że wszystko poniżej środka suwaka brzmiałoby prawie tak samo głośno. Dlatego współczynnik to kwadrat: `gain = (volume / 100)^2` (funkcja `game::masterVolumeGain`, [`../game/settings.md`](../game/settings.md)). Środek suwaka (50) daje 0,25, wartość 80 daje 0,64, a oba końce zostają na miejscu (0 i 1). Uwaga: `AudioEngine::masterVolume()` zwraca **ten współczynnik** (0 do 1), nie liczbę z suwaka.

## 3. Jak to działa w OpenGL

Nie dotyczy: biblioteka `audio` nie używa OpenGL.

## 4. Shadery

Nie dotyczy: biblioteka `audio` nie ma shaderów.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Co |
|---|---|
| `src/audio/AudioEngine.hpp` | klasa `audio::AudioEngine`: konstruktor, destruktor i sześć funkcji (`load`, `play`, `setMasterVolume`, `masterVolume`, `isAvailable`, `status`), struktura `Backend` tylko zadeklarowana |
| `src/audio/AudioEngine.cpp` | stała `VOICE_COUNT`, struktury `Voice` i `LoadedSound`, funkcja `loadSound`, `Backend` i funkcje klasy |
| `external/miniaudio/miniaudio.c` | jedyny plik z implementacją miniaudio |
| `assets/audio/*.wav` i `README.md` | siedem dźwięków i tabela: źródło, licencja, autor |
| `tools/make_sounds.py` | skrypt, który te pliki generuje (sekcja 5.6) |

Biblioteka `audio` w `CMakeLists.txt` ma **dwa pliki** (`AudioEngine.cpp` i `.hpp`), linkuje `engine` jako `PUBLIC` (tylko po to, żeby dostać korzeń `src/` do ścieżek dołączania i funkcje `core::`) i `miniaudio` jako `PRIVATE`, i jest dolinkowana do `night_maze` (`target_link_libraries(night_maze PRIVATE engine game_logic ui video audio imgui)`).

### 5.2 Interfejs

```cpp
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    void load(std::span<const std::filesystem::path> files);
    void play(std::size_t index);
    void setMasterVolume(float volume);
    float masterVolume() const { return m_masterVolume; }
    bool isAvailable() const { return m_backend != nullptr; }
    const std::string& status() const { return m_status; }
    // ...
};
```

(Plik: `src/audio/AudioEngine.hpp`, bez komentarzy.) Konstruktor **nigdy nie rzuca**, gdy nie ma karty: pyta się `isAvailable()` i `status()`. Żadna funkcja klasy nie rzuca wyjątku z powodu braku karty, brakującego pliku ani pliku, którego nie da się zdekodować; zamiast tego jest jedna linia w logu i dźwięk, który nic nie robi. Kopiowanie jest zablokowane (`= delete`).

### 5.3 Konstruktor: otwarcie karty

```cpp
auto backend = std::make_unique<Backend>();
const ma_result result = ma_engine_init(nullptr, &backend->engine);
if (result != MA_SUCCESS) {
    m_status = std::string("off: ") + ma_result_description(result);
    core::logWarn("Audio: the sound device cannot be opened (" +
                  std::string(ma_result_description(result)) + "): the game is silent");
    return;
}
backend->engineReady = true;
```

(Plik: `src/audio/AudioEngine.cpp`, konstruktor, skrócony.) `Backend` żyje na stercie od początku, bo miniaudio trzyma wskaźniki do silnika. `ma_engine_init(nullptr, ...)` bez konfiguracji otwiera domyślne urządzenie odtwarzania w jego ulubionym formacie i je uruchamia. Gdy się nie uda, `m_backend` zostaje pusty, a każda inna funkcja najpierw o to pyta. Gdy się uda, kod składa linię stanu (nazwa urządzenia, częstotliwość, liczba kanałów) i pisze `Audio: opened <urządzenie>` do logu.

### 5.4 `load`, `loadSound` i `play`

`load` czyści listę dźwięków i dla każdego pliku woła `loadSound`. Plik, którego nie da się wczytać, **zachowuje swój numer** (na liście jest pusty wskaźnik), więc numery pozostałych dźwięków się nie przesuwają. Na końcu jedna linia: `Audio: N of M sounds loaded`.

`loadSound` robi po kolei:

1. Czyta bajty pliku przez `core::readBinaryFile` (jedyny sposób otwierania plików w programie, działa też ze ścieżką ze znakami spoza strony kodowej Windows, jak przy obrazach i stb).
2. `ma_decode_memory` dekoduje całość do 32-bitowych liczb zmiennoprzecinkowych w formacie silnika. Blok od dekodera jest kopiowany do `std::vector<float>`, a `ma_free` oddaje go od razu.
3. Dla każdego z trzech głosów: czytnik `ma_audio_buffer_ref_init` wskazujący na wspólne próbki, ręczne wpisanie `sampleRate` czytnika (komentarz: ta wersja zostawia 0, a silnik o to pyta), i `ma_sound_init_from_data_source` z flagami `MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH` (dźwięk jest tak samo głośny w obu uszach i ma stałą wysokość).

```cpp
void AudioEngine::play(std::size_t index) {
    if (m_backend == nullptr || index >= m_backend->sounds.size() ||
        m_backend->sounds[index] == nullptr) {
        return;
    }
    LoadedSound& loaded = *m_backend->sounds[index];
    ma_sound& sound = loaded.voices.at(loaded.nextVoice).sound;
    loaded.nextVoice = (loaded.nextVoice + 1) % VOICE_COUNT;
    ma_sound_seek_to_pcm_frame(&sound, 0);
    ma_sound_start(&sound);
}
```

(Plik: `src/audio/AudioEngine.cpp`, funkcja `play`, bez komentarzy.) Co dzieje się w `play`:

- Strażnik: brak karty, numer spoza listy i dźwięk, którego plik się nie wczytał, kończą funkcję bez efektu.
- Bierze głos w kolejce i przesuwa kolejkę.
- `ma_sound_seek_to_pcm_frame(&sound, 0)` cofa głos do pierwszej ramki, a `ma_sound_start` go uruchamia. Dla głosu, który się skończył, przewinięcie cofa go, a start gra. Dla głosu, który jeszcze gra, start nic nie robi, a przewinięcie każe mu zacząć od początku (to jest to ucięcie z sekcji 2.3). Oba wywołania tylko zostawiają notatkę dla wątku dźwięku.

**Przykład policzony ręcznie.** Dźwięk `crystal_pickup` trwa 1,25 s. Gracz zbiera kryształy w chwilach 0,0 s, 0,5 s, 1,0 s i 1,2 s. Pierwsze zagranie bierze głos 0, drugie głos 1, trzecie głos 2 (wtedy gra jeszcze głos 0, bo skończy się w 1,25 s), a czwarte, w 1,2 s, wraca do głosu 0, który jeszcze gra: zostanie **cofnięty**. To dokładnie czwarte zagranie w czasie długości dźwięku (`VOICE_COUNT + 1`), o którym mówi komentarz. W grze takie zbieranie jest mało prawdopodobne, ale kod go nie wyklucza.

### 5.5 Destruktor, kolejność i testy

`~Backend` najpierw czyści dźwięki (są częścią silnika), a dopiero potem wywołuje `ma_engine_uninit`. W `~LoadedSound` dźwięk głosu jest zwalniany przed jego czytnikiem, bo czyta z niego. Próbki idą na końcu, same, bo składowe niszczą się po ciele destruktora.

**Testów jednostkowych biblioteki nie ma:** wymagałaby karty dźwiękowej. Reguły dźwięków są testowane osobno ([`../game/sound-cues.md`](../game/sound-cues.md), 15 przypadków), a samo odtwarzanie sprawdzono logami i pomiarami plików.

### 5.6 Pliki dźwiękowe i skrypt

Siedem plików WAV (odczytane z nagłówków): 16 bitów, 44100 próbek na sekundę, mono poza `crystal_pickup.wav` i `gate_open.wav`, które są stereo.

| Plik | Kanały | Czas (s) | Bajty |
|---|---:|---:|---:|
| `flashlight_on.wav` | 1 | 0,11 | 9746 |
| `flashlight_off.wav` | 1 | 0,10 | 8864 |
| `flashlight_dead.wav` | 1 | 0,16 | 14156 |
| `low_battery_pulse.wav` | 1 | 0,45 | 39734 |
| `crystal_pickup.wav` | 2 | 1,25 | 220544 |
| `lever_pull.wav` | 1 | 0,90 | 79424 |
| `gate_open.wav` | 2 | 1,85 | 326384 |

Dźwięki generuje [`tools/make_sounds.py`](../../../tools/make_sounds.py): tylko biblioteka standardowa Pythona, z fal sinusoidalnych i szumu, bez nagrywania i bez pobierania. Skrypt ma ustalone ziarno szumu dla każdego dźwięku, więc ponowne uruchomienie zapisuje te same bajty **na tym samym komputerze**; nagłówek skryptu zaznacza, że inny system może zaokrąglić sinus w ostatniej cyfrze i przesunąć pojedyncze próbki o jeden krok. Przełączniki: `--out-dir`, `--report` (zmierz pliki zamiast je robić) i `--pictures DIR` (narysuj fale i spektrogramy). Tabela źródło, licencja, autor jest w [`assets/audio/README.md`](../../../assets/audio/README.md). Nie uruchamiaj skryptu bez `--report`, jeśli nie chcesz nadpisać plików w `assets/audio`.

Głośności dźwięków są różne celowo (to miks gry): szczyty od -3 dBFS (kryształ i brama) do -16 dBFS (puls), żaden nie jest głośniejszy niż -3 dBFS (z `assets/audio/README.md`). **Liczby z `--report` nie dowodzą, że dźwięk brzmi dobrze.**

## 6. Okno debugowania (dawniej panel ImGui)

Kategoria Diagnostics, zakładka Frame and shaders, ma kartę **Audio** (tylko do odczytu, zero kontrolek, więc liczba kontrolek okna zostaje 114): `Device` (linia `status()`: nazwa urządzenia, częstotliwość, kanały i liczba wczytanych dźwięków albo powód, dla którego dźwięku nie ma), `Last cue` (nazwa ostatniego dźwięku), `Cues played` (ile ich było od startu) i `Master volume` (liczba z ustawień i współczynnik silnika, z dopiskiem, że zmienia się ją na ekranie ustawień). Opis okna: [`../debug-ui.md`](../debug-ui.md), sekcja 6.7.

`Cues played` liczy każde wywołanie `playCue`, także przy braku karty dźwiękowej: licznik pokazuje, co **gra chciała** zagrać, a nie co było słychać.

## 7. Pułapki

1. **Nikt nie słyszał dźwięków.** Sprawdzenie jest otwarte.
2. **macOS i Linux nie były budowane.**
3. **Brak funkcji zatrzymania.** Dźwięk gra do końca, także pod menu (najdłuższy ogon to brama, 1,85 s). To świadomy wybór ([`../game/sound-cues.md`](../game/sound-cues.md), sekcja 5.5).
4. **Czwarte zagranie w czasie długości dźwięku ucina głos** (sekcja 5.4). Kod tego nie robi, ale nie wyklucza.
5. **Struktury miniaudio nie mogą się przemieszczać.** Dlatego `LoadedSound` leży za `std::unique_ptr` i ma skasowane kopiowanie.
6. **Plik, którego nie da się wczytać, jest ciszą, nie błędem.** Jest jedna linia `Audio: <plik>: <powód>` w logu i tyle. Nie ma testu, który by to wykrył.
7. **`masterVolume()` zwraca współczynnik, nie suwak** (sekcja 2.5).
8. **Wszystkie funkcje z jednego wątku.** Klasa nie ma mutexu.
9. **Plik zastępczy musi być WAV**, bo inne dekodery są wycięte z miniaudio ([`../../libraries/miniaudio.md`](../../libraries/miniaudio.md), sekcja 4.3).

## 8. Ćwiczenia

1. **Długość dźwięku z nagłówka.** Plik ma 81585 ramek przy 44100 na sekundę. Ile trwa? Odpowiedź: 81585 / 44100 = 1,85 s (`gate_open.wav`).
2. **Który głos?** Dźwięk zagrano pięć razy pod rząd. Który głos zagra piąty raz? Odpowiedź: kolejka to 0, 1, 2, 0, 1, więc piąty raz zagra głos 1. Jeśli ten głos jeszcze gra, zostanie cofnięty do początku (ucięty).
3. **Współczynnik głośności.** Suwak stoi na 30. Jaki współczynnik dostaje silnik? Odpowiedź: (30 / 100)^2 = 0,09.
4. **Brak pliku.** Zmień nazwę `assets/audio/lever_pull.wav` w katalogu builda i uruchom grę. Co mówi log, co karta Audio i co się dzieje po pociągnięciu dźwigni? Odpowiedź: w logu `Audio: ... lever_pull.wav: the file cannot be opened` i `Audio: 6 of 7 sounds loaded`; karta `Device` kończy się na `6 of 7 sounds loaded`; dźwignia działa, `Cues played` rośnie, ale nic nie słychać.
5. **Brak karty.** Co pokaże karta `Device`, gdy urządzenie nie otworzy się? Odpowiedź: tekst zaczynający się od `off: `, a po nim opis błędu miniaudio.

## 9. Pytania kontrolne

1. **Dlaczego biblioteka `audio` stoi obok `ui` i `video`, a nie w `engine`?**
   `game_logic` i testy linkują `engine`, a nie powinny linkować miniaudio, które otwiera kartę dźwiękową. Dzięki temu żaden test nie potrzebuje karty.
2. **Dlaczego nagłówek `AudioEngine.hpp` nie dołącza miniaudio?**
   Obiekty biblioteki są w strukturze `Backend` znanej tylko plikowi `.cpp`, więc kod używający klasy nie potrzebuje nagłówka miniaudio ani jego ścieżki.
3. **Dlaczego każdy dźwięk jest w całości w pamięci?**
   Są krótkie (od 0,10 s do 1,85 s, razem 0,7 MB), więc odtwarzanie nie wymaga w chwili grania żadnej pracy.
4. **Po co trzy głosy?**
   Żeby dźwięk zagrany ponownie, gdy poprzedni jeszcze wybrzmiewa, grał obok niego, a nie ucinał go (ucięcie słychać jako trzask).
5. **Czemu każdy głos ma własny czytnik?**
   Miejsce, do którego dźwięk doszedł, jest w czytniku. Dwa głosy na jednym czytniku dzieliłyby jedno miejsce.
6. **Co chroni `AudioEngine` przed wyścigiem z wątkiem dźwięku?**
   Nic z naszego kodu tam nie działa, a `play` i `setMasterVolume` ustawiają tylko liczby, które miniaudio samo chroni. Dlatego klasa nie ma mutexu.
7. **Co się dzieje bez karty dźwiękowej?**
   Konstruktor nie rzuca, pisze jedną linię do logu, `isAvailable()` jest fałszem, a każde następne wywołanie nic nie robi.
8. **Jak wygląda współczynnik głośności dla suwaka na 50?**
   0,25: kwadrat z 50 / 100.
9. **Czemu nie ma funkcji zatrzymania?**
   Wybór wykonawczy: każdy dźwięk jest krótkim jednorazowym odgłosem, a zatrzymywanie dźwięków przy pauzie wymagałoby sposobu ich wznawiania.
10. **Skąd się biorą pliki dźwiękowe?**
    Generuje je skrypt `tools/make_sounds.py` z fal i szumu, bez nagrywania i pobierania.

## 10. Źródła

- Notatki: [`../../decisions/audio-on-miniaudio.md`](../../decisions/audio-on-miniaudio.md), [`../../decisions/sounds-generated-by-script.md`](../../decisions/sounds-generated-by-script.md), [`../../decisions/versions-m9-and-the-sound-slice.md`](../../decisions/versions-m9-and-the-sound-slice.md).
- Dokumenty: [`../../libraries/miniaudio.md`](../../libraries/miniaudio.md), [`../game/sound-cues.md`](../game/sound-cues.md), [`../game/settings.md`](../game/settings.md), [`../debug-ui.md`](../debug-ui.md), [`../core/paths.md`](../core/paths.md), [`../video/README.md`](../video/README.md) (ten sam podział na bibliotekę obok `ui`).
- miniaudio: dokumentacja w nagłówku `miniaudio.h` (sekcje o silniku `ma_engine` i o dekodowaniu) i <https://miniaud.io>.
- Dźwięk cyfrowy (próbkowanie, ramki): dowolny podręcznik podstaw cyfrowego przetwarzania dźwięku.
