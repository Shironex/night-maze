# miniaudio 0.11.25

Dokument biblioteki dla pierwszego kawałka dźwięku (2026-10-07). Opisuje konfigurację z [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake) (ostatni blok pliku), plik [`external/miniaudio/miniaudio.c`](../../external/miniaudio/miniaudio.c) i tę część API, której używa biblioteka `audio` w [`src/audio/AudioEngine.cpp`](../../src/audio/AudioEngine.cpp). Dokument o samej bibliotece `audio` (klasa `audio::AudioEngine`, głosy, wątki) to [`../modules/audio/README.md`](../modules/audio/README.md). Dokument o tym, **kiedy** gra odtwarza który dźwięk, to [`../modules/game/sound-cues.md`](../modules/game/sound-cues.md). Wybór biblioteki jest decyzją właściciela: [`../decisions/audio-on-miniaudio.md`](../decisions/audio-on-miniaudio.md).

**Czego ten dokument nie obiecuje.** Wersja i licencja są odczytane z pobranego źródła (`MA_VERSION_*` w `miniaudio.h` i plik `LICENSE`). To, jak miniaudio działa w środku (wątek urządzenia, mikser), jest opisane według komentarzy w kodzie projektu i według dokumentacji, którą ten kod cytuje, a nie według własnych eksperymentów. **Nikt nie słyszał dźwięków wydawanych przez ten kod:** sprawdzono je logami, testami reguł i pomiarami plików. **macOS nie był budowany**, więc to, co poniżej o macOS, jest odczytem z komentarza w `Dependencies.cmake`, nie sprawdzeniem.

## 1. Czym jest miniaudio

miniaudio to biblioteka do odtwarzania (i nagrywania) dźwięku, dostarczana jako **jeden plik nagłówkowy**, który zawiera też swoją implementację. To ta sama konstrukcja co [`stb_image.md`](stb_image.md): nagłówek dołączony zwykle tylko deklaruje funkcje, a zdefiniowanie makra `MINIAUDIO_IMPLEMENTATION` przed dołączeniem sprawia, że ten sam nagłówek wypisuje także ich definicje. Makro musi być zdefiniowane w **dokładnie jednym** pliku programu.

Biblioteka ma dwie warstwy, z których projekt używa wyższej:

- **niska warstwa**: urządzenie (`ma_device`), które otwiera kartę dźwiękową przez interfejs systemu i co kilka milisekund prosi o kolejną porcję próbek,
- **wysoka warstwa, silnik** (`ma_engine`): trzyma urządzenie, miksuje wszystkie odtwarzane dźwięki (`ma_sound`) do jednej porcji i ma jedną głośność główną (`ma_engine_set_volume`).

### Za co miniaudio NIE odpowiada w tym projekcie

| Rzecz | Kto to robi |
|---|---|
| otwieranie plików | `core::readBinaryFile` (miniaudio dostaje bajty, nie ścieżkę), jak przy obrazach |
| wiedza, który dźwięk znaczy co | `game::SoundCue` i reguły w `game/SoundCues.*` |
| głośność gracza | ustawienie `master_volume` i funkcja `game::masterVolumeGain` |
| wątek dźwięku | miniaudio uruchamia go sam przy otwarciu urządzenia (opis w [`../modules/audio/README.md`](../modules/audio/README.md)) |

## 2. Dlaczego miniaudio jest w projekcie

Decyzja właściciela z 2026-10-06: dźwięk w grze będzie oparty na miniaudio ([`../decisions/audio-on-miniaudio.md`](../decisions/audio-on-miniaudio.md)). Uzasadnienia właściciela nie ma w notatce i nie dopisuję własnego.

## 3. Przypięta wersja

| Biblioteka | Tag | Licencja |
|---|---|---|
| miniaudio | `0.11.25` | do wyboru: domena publiczna (Unlicense) albo MIT No Attribution |

Wersję odczytałem z pobranego źródła (`build/release/_deps/miniaudio-src/miniaudio.h`): `MA_VERSION_MAJOR` 0, `MA_VERSION_MINOR` 11, `MA_VERSION_REVISION` 25, a repozytorium pobrane przez FetchContent opisuje się jako `0.11.25`. Daty wydania nie sprawdzałem, więc jej nie podaję. Biblioteka jest pobierana w czasie konfiguracji, jak pozostałe, i nie leży w repozytorium.

## 4. Jak jest pobierana i co zostało wyłączone

Plik: [`cmake/Dependencies.cmake`](../../cmake/Dependencies.cmake). Pozostałe zależności opisują [`glfw.md`](glfw.md), [`imgui.md`](imgui.md), [`stb_image.md`](stb_image.md), [`rmlui.md`](rmlui.md) i inne dokumenty tego katalogu.

### 4.1 Pobranie bez uruchamiania jej CMake: `SOURCE_SUBDIR`

```cmake
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(miniaudio)
```

(Plik: `cmake/Dependencies.cmake`.) W odróżnieniu od stb repozytorium miniaudio **ma** własny `CMakeLists.txt` i ten plik nie może tu zadziałać. Komentarz w pliku podaje dwa powody:

- definiuje on target o nazwie `miniaudio`, czyli o nazwie małej biblioteki, którą projekt definiuje niżej, a dwa targety nie mogą mieć jednej nazwy,
- dodaje reguły instalacji (opcja `MINIAUDIO_INSTALL` jest domyślnie włączona) i szuka na komputerze bibliotek libvorbis i libopus, żeby zbudować wokół nich dodatkowe dekodery, więc build zależałby od tego, co akurat jest zainstalowane.

Wyłączenie opcji nie usunęłoby konfliktu nazw. Dlatego `SOURCE_SUBDIR` wskazuje katalog, którego w repozytorium nie ma (`cmake` to tylko nazwa, która tam nie istnieje): FetchContent pobiera wtedy źródło i **niczego z niego nie dodaje do buildu**. `FetchContent_MakeAvailable` ustawia zmienną `miniaudio_SOURCE_DIR`, z której korzysta następny blok.

### 4.2 Target i plik z implementacją

```cmake
add_library(miniaudio STATIC ${CMAKE_CURRENT_LIST_DIR}/../external/miniaudio/miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC ${miniaudio_SOURCE_DIR})
```

Implementacja kompiluje się raz, w [`external/miniaudio/miniaudio.c`](../../external/miniaudio/miniaudio.c), do małej biblioteki statycznej. Powody są te same co przy stb: target ma osobny, bo funkcja `night_maze_enable_warnings` **nigdy** nie jest dla niego wołana (cudzy kod kompiluje się z domyślnymi ostrzeżeniami kompilatora), a `SYSTEM` sprawia, że nagłówek nie daje ostrzeżeń w pliku, który go dołącza, i że clang-tidy go nie sprawdza (jego filtr nagłówków łapie każdą ścieżkę z `src/`, a pobrane źródło leży w katalogu `miniaudio-src`).

Sam plik `miniaudio.c` ma dwie linie kodu:

```c
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
```

(Plik: `external/miniaudio/miniaudio.c`, linie 11 i 12.) Makra `MA_NO_*` celowo **nie są** zapisane w tym pliku, tylko w CMake (następny punkt), żeby plik implementacji i plik, który dołącza nagłówek, widziały zawsze ten sam zestaw.

### 4.3 Makra `MA_NO_*`: co wycięto

```cmake
target_compile_definitions(miniaudio PUBLIC
    MA_NO_ENCODING
    MA_NO_FLAC
    MA_NO_MP3
    MA_NO_GENERATION
    MA_NO_RESOURCE_MANAGER
)
```

| Makro | Co wyłącza | Dlaczego można (według komentarza w pliku) |
|---|---|---|
| `MA_NO_ENCODING` | zapis plików dźwiękowych | gra tylko odtwarza |
| `MA_NO_FLAC`, `MA_NO_MP3` | wbudowane dekodery FLAC i MP3 | każdy dźwięk gry to plik WAV, dekoder WAV zostaje |
| `MA_NO_GENERATION` | generatory fal sinusoidalnych i szumu | dźwięki pochodzą z plików |
| `MA_NO_RESOURCE_MANAGER` | menedżer zasobów: otwieranie plików po nazwie, dekodowanie na własnym wątku, strumieniowanie długich | `audio::AudioEngine` sam czyta każdy plik i każe miniaudio zdekodować bajty naraz; zostaje silnik wysokiego poziomu, mikser i urządzenie |

Makra są `PUBLIC`, i to ma tu większe znaczenie niż przy stb: według komentarza w pliku **zmieniają one, jakie pola mają struktury miniaudio**, więc plik z implementacją i plik, który dołącza nagłówek, muszą widzieć ten sam zestaw, inaczej oba mogłyby się różnić co do rozmiaru struktury. Konsekwencja do zapamiętania: skoro nie ma dekodera FLAC ani MP3, **zamiana pliku dźwiękowego na nagranie wymaga formatu WAV** ([`../../assets/audio/README.md`](../../assets/audio/README.md)).

### 4.4 Co linkuje każdy system

Według komentarza w pliku (który cytuje sekcję 2 "Building" nagłówka miniaudio):

| System | Linkowanie |
|---|---|
| Windows | nic: biblioteki dźwiękowe systemu są ładowane w czasie działania programu |
| macOS | nic: miniaudio ładuje ramy Core Audio w czasie działania. Komentarz zaznacza jeden haczyk: tak zbudowany program może nie przejść notaryzacji Apple. Obejściem jest makro `MA_NO_RUNTIME_LINKING` razem z ramami CoreFoundation, CoreAudio i AudioToolbox. Nie jest użyte, bo gra nie jest notaryzowana |
| Linux | `dl`, `pthread` i `m` (przez `CMAKE_DL_LIBS` i `Threads::Threads`) |

Blok `if(UNIX AND NOT APPLE)` w pliku obejmuje gałąź Linuksa. **Linux nie był budowany**, a macOS też nie.

### 4.5 Podłączenie do `audio`

```cmake
target_link_libraries(audio PUBLIC engine)
target_link_libraries(audio PRIVATE miniaudio)
```

(Plik: `CMakeLists.txt`.) `miniaudio` jest `PRIVATE`: nagłówek `audio/AudioEngine.hpp` chowa wszystkie obiekty biblioteki za wskaźnikiem na strukturę znaną tylko plikowi `.cpp`, więc kod używający klasy nie potrzebuje ani nagłówka miniaudio, ani jego ścieżki.

## 5. Z czego projekt korzysta

Wszystko w jednym pliku, `src/audio/AudioEngine.cpp`:

| Funkcja lub typ | Do czego |
|---|---|
| `ma_engine_init(nullptr, &engine)` | otwiera domyślne urządzenie odtwarzania w formacie, jaki ono woli, i je uruchamia (bez konfiguracji) |
| `ma_engine_get_channels`, `ma_engine_get_sample_rate` | format, w którym silnik miksuje |
| `ma_device_get_name` przez `ma_engine_get_device` | nazwa urządzenia do linii stanu |
| `ma_decoder_config_init` i `ma_decode_memory` | dekodują cały plik z bajtów w pamięci do `f32`, kanałów i częstotliwości silnika |
| `ma_free` | oddaje blok z dekodera |
| `ma_audio_buffer_ref_init` | "czytnik": miejsce w próbkach, które nie są kopiowane |
| `ma_sound_init_from_data_source` z flagami `MA_SOUND_FLAG_NO_SPATIALIZATION` i `MA_SOUND_FLAG_NO_PITCH` | część silnika, która gra to, co da czytnik, bez położenia w świecie 3D i bez zmiennej wysokości |
| `ma_sound_seek_to_pcm_frame`, `ma_sound_start` | odtworzenie od początku |
| `ma_engine_set_volume` | głośność główna |
| `ma_result_description` | tekst błędu do logu |
| `ma_sound_uninit`, `ma_audio_buffer_ref_uninit`, `ma_engine_uninit` | sprzątanie |

**Nie używa:** menedżera zasobów (wyłączonego), wczytywania z pliku po nazwie, strumieniowania, dźwięku przestrzennego (3D), efektów i węzłów własnych, nagrywania, własnych wywołań zwrotnych urządzenia (**nic z naszego kodu nie działa na wątku dźwięku**).

## 6. Licencja

- **miniaudio: do wyboru** domena publiczna (Unlicense, "ALTERNATIVE 1") albo MIT No Attribution ("ALTERNATIVE 2", "Copyright 2025 David Reid"). Tak mówi pierwsze zdanie pliku `LICENSE`: "This software is available as a choice of the following licenses. Choose whichever you prefer." Żadna z dwóch nie wymaga noty w rozpowszechnianej kopii (komentarz w `launcher/scripts/build-notices.mjs` mówi to samo).
- **Gdzie to jest:** `THIRD-PARTY-NOTICES.txt` w korzeniu repozytorium zawiera sekcję `miniaudio 0.11.25` z całym plikiem `LICENSE` (obie alternatywy). Plik jest **generowany** skryptem `node launcher/scripts/build-notices.mjs --deps build/release/_deps` i trzeba go wygenerować od nowa, gdy zmieni się wersja w `Dependencies.cmake`. Skrypt ma teraz jedenaście pozycji (policzone w pliku `build-notices.mjs`).
- Pliki dźwiękowe w `assets/audio/` nie są cudzym materiałem: generuje je skrypt w repozytorium ([`../decisions/sounds-generated-by-script.md`](../decisions/sounds-generated-by-script.md)).

## 7. Pułapki

1. **Tego, jak brzmi dźwięk, nikt nie sprawdził uchem.** Sprawdzono logi, testy reguł i pomiary plików. Pierwsze uruchomienie z głośnikami i słuchawkami jest na liście właściciela.
2. **macOS i Linux nie były budowane.** Gałąź macOS (Core Audio ładowane w czasie działania) i Linuksa (`dl`, `pthread`, `m`) jest odczytana z komentarza w CMake.
3. **Makra `MA_NO_*` muszą być wspólne.** Dopisanie ich tylko w `miniaudio.c` rozjechałoby rozmiary struktur. Są w CMake jako `PUBLIC`.
4. **Ta wersja zostawia częstotliwość czytnika równą 0.** Kod ręcznie wpisuje `voice.reader.sampleRate` (komentarz w `loadSound`), bo silnik o nią pyta. Po zmianie wersji trzeba sprawdzić, czy to nadal potrzebne.
5. **Struktury miniaudio nie mogą się przemieszczać w pamięci.** Biblioteka trzyma na nie wskaźniki, więc każdy załadowany dźwięk leży za `std::unique_ptr`.
6. **Sam `CMakeLists.txt` miniaudio nie może zostać uruchomiony** (sekcja 4.1).
7. **Zmiana wersji:** zmienić tag w `Dependencies.cmake`, skonfigurować od nowa i wygenerować od nowa `THIRD-PARTY-NOTICES.txt`. W `external/miniaudio/` nic nie trzeba zmieniać.

## 8. Pytania kontrolne

1. **Dlaczego `miniaudio.c` jest osobnym targetem?**
   Żeby cudzy kod kompilował się bez ścisłych ostrzeżeń projektu i żeby implementacja powstała w dokładnie jednym pliku.
2. **Do czego służy `SOURCE_SUBDIR cmake`?**
   Wskazuje nieistniejący katalog, więc FetchContent pobiera źródło, ale nie uruchamia `CMakeLists.txt` miniaudio: ten definiuje target o tej samej nazwie, dodaje instalację i szuka bibliotek z systemu.
3. **Dlaczego makra `MA_NO_*` są `PUBLIC`?**
   Zmieniają pola struktur, więc plik z implementacją i plik dołączający nagłówek muszą widzieć ten sam zestaw.
4. **Jakie licencje ma miniaudio i która jest wybrana?**
   Domena publiczna (Unlicense) albo MIT No Attribution, do wyboru. Projekt nie musi wybierać, bo żadna nie wymaga noty; tekst i tak jest w `THIRD-PARTY-NOTICES.txt`.
5. **Czemu pliki dźwiękowe muszą być WAV?**
   Dekodery FLAC i MP3 są wycięte, a dekoder WAV zostaje.
6. **Czemu `audio` linkuje miniaudio jako `PRIVATE`?**
   Nagłówek `AudioEngine.hpp` chowa jej obiekty za wskaźnikiem, więc kod, który klasy używa, nie potrzebuje nagłówka biblioteki.

## 9. Źródła

- Repozytorium i dokumentacja miniaudio: <https://github.com/mackron/miniaudio> i <https://miniaud.io> (tag `0.11.25`; sekcja "Building" w nagłówku `miniaudio.h`).
- W projekcie: [`../decisions/audio-on-miniaudio.md`](../decisions/audio-on-miniaudio.md), [`../modules/audio/README.md`](../modules/audio/README.md), [`../modules/game/sound-cues.md`](../modules/game/sound-cues.md), [`stb_image.md`](stb_image.md) (ten sam wzór jednego nagłówka), [`../../external/miniaudio/README.md`](../../external/miniaudio/README.md).
