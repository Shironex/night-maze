# Moduł video: tło menu jako wideo odtwarzane przez dekoder systemu

Kamień milowy: M9, część 4 (2026-10-06). Temat wykładu: brak własnego (to dodatek do gry, poza listą 15 tematów). Dokument korzysta z tekstur ([`../gfx/textures.md`](../gfx/textures.md)), z trójkąta na cały ekran i shaderów przebiegów ([`../renderer/post-process.md`](../renderer/post-process.md)), z przestrzeni kolorów ([`../gfx/color-space.md`](../gfx/color-space.md)), z ekranów gry ([`../game/game-states.md`](../game/game-states.md): `usesMenuCamera`, `drawsScene`), z kamery menu, którą nagrano pętlę ([`../game/menu-camera.md`](../game/menu-camera.md)), z menu na wierzchu ([`../ui/menu-screens.md`](../ui/menu-screens.md)) i ze ścieżek do plików ([`../core/paths.md`](../core/paths.md)).
Kod: [`src/video/VideoDecoder.hpp`](../../../src/video/VideoDecoder.hpp) (jeden interfejs platformy), [`VideoDecoderWindows.cpp`](../../../src/video/VideoDecoderWindows.cpp) (Media Foundation), [`VideoDecoderApple.mm`](../../../src/video/VideoDecoderApple.mm) (AVFoundation, **nigdy nie skompilowany**), [`VideoPlayer.hpp`](../../../src/video/VideoPlayer.hpp) i [`VideoPlayer.cpp`](../../../src/video/VideoPlayer.cpp) (wątek dekodujący, kolejka, zegar, tekstura), [`VideoClock.hpp`](../../../src/video/VideoClock.hpp) i [`VideoClock.cpp`](../../../src/video/VideoClock.cpp) (czysta arytmetyka zegara); w bibliotece `engine`: [`src/gfx/CoverFit.hpp`](../../../src/gfx/CoverFit.hpp) i [`FrameTexture.hpp`](../../../src/gfx/FrameTexture.hpp) (z plikami `.cpp`); w `game_logic`: [`src/game/MenuBackground.hpp`](../../../src/game/MenuBackground.hpp) (reguła wyboru tła); w programie: [`src/game/MenuBackgroundRenderer.hpp`](../../../src/game/MenuBackgroundRenderer.hpp) i gałąź w `NightMazeApp::onRender`; shader [`assets/shaders/post/menu_background.frag`](../../../assets/shaders/post/menu_background.frag); pliki [`assets/video/menu_loop.mp4`](../../../assets/video/menu_loop.mp4) i [`menu_still.png`](../../../assets/video/menu_still.png); narzędzie [`tools/record_menu_loop.py`](../../../tools/record_menu_loop.py). Testy: [`tests/CoverFitTests.cpp`](../../../tests/CoverFitTests.cpp) (5 przypadków), [`tests/VideoClockTests.cpp`](../../../tests/VideoClockTests.cpp) (7), [`tests/MenuBackgroundTests.cpp`](../../../tests/MenuBackgroundTests.cpp) (6) i jeden nowy przypadek w [`tests/StartOptionsTests.cpp`](../../../tests/StartOptionsTests.cpp).

**Stan na dziś:** menu główne i ustawienia otwarte z niego pokazują **nagraną pętlę wideo** (30 s, 1280 x 720, 30 klatek na sekundę) zamiast żywego przelotu kamery nad labiryntem. Wideo odtwarza dekoder systemu: **Media Foundation na Windowsie** (kod uruchomiony i zmierzony) i **AVFoundation na macOS** (kod napisany z dokumentacji, **nigdy nie skompilowany i nigdy nie uruchomiony**). Gdy wideo nie da się odtworzyć, menu pokazuje **nieruchomy obraz** (klatka 0 pętli jako zwykły PNG), a gdy i tego nie da się wczytać, **żywą scenę** jak przed tą częścią. Dopóki tło to wideo albo obraz, **scena nie jest rysowana w ogóle**. Przełącznik `--menu-background <video|still|scene>` wymusza wybór. Decyzje właściciela są w dwóch notatkach ([`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md) i [`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md)), wszystko inne w tym dokumencie to wybory wykonawcze (sekcja 1.2).

**Uczciwie o tym, co sprawdzono.** Trzy rodzaje dowodów trzymam osobno (tak jak w [`../../guides/build-windows.md`](../../guides/build-windows.md)):

1. **Zgłoszone przez bramkę i autora kodu (2026-10-06), nie powtórzone przy pisaniu tego dokumentu:** `make check` przeszedł na `0f8d3b9` (przed tą częścią): **564 przypadki testowe i 220119 asercji**, a na gałęzi wideo **583 przypadki testowe i 220420 asercji** (19 przypadków więcej: 5 + 7 + 6 + 1). **Wszystkie pomiary w sekcji 2.11 to pomiary autora kodu na jego komputerze** (Windows, Release, okno 1280 x 720, karta NVIDIA GeForce, vsync nie ograniczał klatek). Liczb nie mierzyłem sam i nie ma ich w repozytorium: narzędzie pomiarowe (`decodetool`) i łatka z tymczasowymi pomiarami (`measure-temporary.patch`) leżą poza repozytorium i nie zostały zatwierdzone.
2. **Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela:** menu z wideo w czterech rozmiarach okna, ustawienia, `Play`, pauza i `Back to menu`, wszystkie ścieżki zapasowe, okno debug nad menu, klatki z miejsca zamknięcia pętli (lista: [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 29). **Wideo w ruchu nikt nie oglądał okiem**: każde sprawdzenie było zrzutem albo liczbą.
3. **Otwarta lista właściciela:** [`../../guides/build-windows.md`](../../guides/build-windows.md), sekcja 29.2 (Windows), i [`../../guides/build-macos.md`](../../guides/build-macos.md), podsekcja "M9, część 4 (wideo w tle menu) na macOS" (w całości otwarta, piętnaście punktów).

**Nie sprawdzono nigdzie:** pełnego ekranu, rozmiarów okna powyżej 1600 x 900, prawdziwej edycji Windows N (ścieżkę zasymulowano), skalowania ekranu innego niż 100 procent, drugiego monitora, awarii dekodera w środku odtwarzania (kod jest, ścieżka nie była uruchomiona), całego kodu macOS.

## 1. Po co to jest

### 1.1 Co robi biblioteka i dlaczego stoi tam, gdzie stoi

Właściciel zdecydował, że tłem menu ma być wcześniej wyrenderowana pętla wideo, odtwarzana przez dekodery systemu operacyjnego, z nieruchomym obrazem na wypadek porażki. Ta część to kod, który to robi: otwiera plik `.mp4`, dekoduje klatki, pokazuje właściwą klatkę we właściwej chwili i rysuje ją na całe okno pod menu.

Nowa biblioteka statyczna `video` stoi **obok `ui`, nad `gfx`, pod `game`**. Powody (wybór wykonawczy):

- **Potrzebuje tekstury OpenGL**, więc stoi nad `gfx` (tekstura z klatką to `gfx::FrameTexture`).
- **Nic nie wie o grze**: dostaje ścieżkę do pliku i oddaje teksturę. Dlatego stoi pod `game`. Co z niej zrobić, wie `game::MenuBackgroundRenderer`.
- **Nie jest częścią `engine`**, z tego samego powodu co `ui`: `game_logic` i program testowy linkują `engine`, a nie powinny linkować ani Media Foundation, ani pliku w Objective-C++.
- **Części czyste leżą tam, gdzie sięgają testy**: `gfx::coverFit` w `engine` (obok `gfx/ColorSpace`), reguła wyboru tła w `game_logic`, a `video/VideoClock.cpp` jest kompilowany **bezpośrednio do programu testowego** (tak jak `src/debug/Search.cpp`), bo linkowanie całej biblioteki wciągnęłoby Media Foundation do testów.
- **Kod zależny od systemu stoi za jednym interfejsem**, `video/VideoDecoder.hpp`. Żaden inny plik nie dołącza nagłówka systemowego o wideo ani nie wie, jaki system działa. To drugi i trzeci plik projektu z kodem zależnym od platformy (pierwszy to `src/core/Paths.cpp`, [`../core/paths.md`](../core/paths.md)).

Dlaczego nie `gfx::Texture2D` dla klatek: ta klasa buduje mipmapy przy każdym wysłaniu, powtarza obraz na brzegach (`GL_REPEAT`) i jest pomyślana jako "wyślij raz". Klatka wideo jest wysyłana do 30 razy na sekundę, nie ma mipmap i nie może się powtarzać. Stąd osobna klasa `gfx::FrameTexture`.

### 1.2 Decyzje właściciela, a wybory wykonawcze

Decyzje właściciela projektu (2026-10-06, szczegóły w notatkach):

1. Tłem menu jest zmontowana, wyrenderowana wcześniej pętla wideo, a nie żywa scena ([`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md)).
2. Pętla jest odtwarzana przez **dekodery wideo systemu operacyjnego**, nie przez FFmpeg ani dekoder dołączony do projektu. Gdy odtwarzanie się nie uda, tłem jest **nieruchomy obraz** ([`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md)).

**Wszystko inne w tym dokumencie jest wyborem wykonawczym autora kodu**, nie decyzją właściciela: wybór Media Foundation i AVFoundation (notatka mówi tylko "dekodery systemu"), format MP4 z H.264, osobny wątek dekodujący, kolejka czterech klatek, zegar wideo, "pokrycie" zamiast rozciągnięcia, ścieżka koloru (BT.709, zakres ograniczony), kolejność wyboru tła (wideo, obraz, żywa scena), przełącznik `--menu-background`, to, że wideo **wznawia** się po powrocie do menu (nie startuje od początku), brak przyciemnienia w shaderze i parametry nagrania.

## 2. Teoria

### 2.1 Dwa zegary

Plik wideo ma własny czas: każda klatka ma **znacznik czasu** (stamp), chwilę, w której ma być pokazana, liczoną od początku pliku (dla 30 klatek na sekundę: 0, 1/30, 2/30 i tak dalej). Gra ma drugi zegar: czas swoich klatek, które przychodzą w dowolnym tempie (60, 144 albo 20 na sekundę). Wideo musi iść **według własnych znaczników**, nie według tempa gry:

- gra szybsza niż wideo (60 na 30) pokazuje każdą klatkę wideo przez kilka klatek gry,
- gra wolniejsza (20 na 30) **pomija** część klatek wideo, ale zachowuje prędkość: po dwóch sekundach wideo jest na klatce 60, niezależnie od tego, ile klatek gry narysowano.

Gdyby wideo szło "jedna klatka wideo na jedną klatkę gry", na monitorze 144 Hz pętla trwałaby 6 sekund zamiast 30. Stąd osobny zegar wideo (sekcja 2.3).

### 2.2 Oś czasu, która nie cofa się

Znaczniki pliku zaczynają się od nowa przy każdym przejściu pętli: ostatnia klatka ma 29,9667 s, a pierwsza następna znowu 0. Gdyby porównywać "czy klatka jest już należna" ze znacznikami wprost, na styku pętli porównanie musiałoby się wywrócić. Dlatego odtwarzacz liczy **czas na osi**, która idzie dalej, przejście po przejściu: `czas = początek przejścia + (znacznik - znacznik pierwszej klatki)`. Początek następnego przejścia to początek poprzedniego plus czas trwania jednego przejścia (`passSeconds`: od pierwszego do ostatniego znacznika plus czas pokazywania ostatniej klatki). Wtedy "czy klatka jest należna" to zawsze jedno porównanie dwóch liczb, także na styku.

Policzone ręcznie dla pliku z pętli (900 klatek, 30 na sekundę, pierwszy znacznik 0):

- ostatni znacznik: `899 / 30 = 29,9667 s`,
- czas jednego przejścia: `(29,9667 - 0) + 1/30 = 30,0 s`,
- pierwsza klatka drugiego przejścia ma czas `30,0 + (0 - 0) = 30,0 s`, czyli `30,0 - 29,9667 = 0,0333 s = 1/30 s` po ostatniej klatce pierwszego przejścia, tak samo jak każda inna para sąsiednich klatek.

Pierwszy znacznik nie musi być zerem: klip próbny z klatkami B zaczynał od 0,0667 s. Odjęcie `firstStampSeconds` sprawia, że gra się tak samo (test `a file whose first stamp is not zero plays the same`).

```cpp
double timelineSeconds(double passStartSeconds, double frameStampSeconds,
                       double firstStampSeconds) {
    return passStartSeconds + (frameStampSeconds - firstStampSeconds);
}

double passSeconds(double firstStampSeconds, double lastStampSeconds, double frameSeconds) {
    return (lastStampSeconds - firstStampSeconds) + frameSeconds;
}

std::size_t dueFrameCount(std::span<const double> waitingSeconds, double clockSeconds) {
    std::size_t count = 0;
    // The list is in the order of showing, so the first frame that is not due yet ends
    // the search.
    while (count < waitingSeconds.size() && waitingSeconds[count] <= clockSeconds) {
        ++count;
    }
    return count;
}

double clockWithEmptyQueue(double clockSeconds, double deltaSeconds, double shownSeconds,
                           double frameSeconds) {
    return std::min(clockSeconds + deltaSeconds, shownSeconds + frameSeconds);
}
```

(Plik: `src/video/VideoClock.cpp`, linie 9 do 31.)

Pierwsze dwie funkcje to powyższe wzory, trzecia (`dueFrameCount`) i czwarta (`clockWithEmptyQueue`) są wyjaśnione w sekcji 2.3. Pełny komentarz o tym, po co każda jest, jest w [`src/video/VideoClock.hpp`](../../../src/video/VideoClock.hpp).

### 2.3 Kolejka czterech klatek i wybór klatki należnej

Wątek dekodujący trzyma w kolejce **najwyżej cztery** zdekodowane klatki naprzód (`VideoPlayer::QUEUE_CAPACITY`). Cztery klatki to `4 / 30 = 133 ms` wideo. W pamięci to cztery obrazy: `1280 * 720 * 4 = 3 686 400` bajtów każdy, razem `14 745 600` bajtów (14,7 MB, w komentarzu w kodzie "15 MB").

Wątek rysujący woła raz na klatkę gry `update(deltaSeconds)`. W środku:

1. zegar wideo rusza o czas klatki gry: `m_clockSeconds = clockBefore + deltaSeconds`,
2. czasy klatek z kolejki trafiają do małej tablicy, a `dueFrameCount` mówi, **ile z nich jest już należnych** (`czas <= zegar`),
3. wszystkie należne wychodzą z kolejki, ale **pokazana jest tylko ostatnia**: klatki przed nią to klatki, na które gra była za wolna, a ich bufory wracają do puli,
4. jeśli kolejka jest pusta, zegar nie może uciec obrazowi (`clockWithEmptyQueue`): `min(zegar + delta, czas pokazanej klatki + czas jednej klatki)`.

Przykład policzony ręcznie. Kolejka ma klatki o czasach `1,000; 1,033; 1,067; 1,100` s, a zegar po ruchu o `delta` wynosi `1,040`. `dueFrameCount` przechodzi listę od początku: `1,000 <= 1,040` tak, `1,033 <= 1,040` tak, `1,067 <= 1,040` nie, koniec. Wynik 2. Z kolejki wychodzą dwie klatki, na ekranie ląduje ta o czasie 1,033, a klatka 1,000 została pominięta (gra była wolniejsza niż wideo). Kolejka ma teraz dwie klatki i wątek dekodujący dopełnia ją do czterech.

Po co punkt 4: gdyby dekoder spóźnił się o sekundę (albo gra stała w miejscu), zegar poszedłby o sekundę do przodu, a klatki, które potem dojdą, byłyby wszystkie spóźnione naraz i wideo **przeskoczyłoby** do przodu. Z `clockWithEmptyQueue` zegar czeka w chwili, w której należy się następna klatka, i wideo idzie dalej od miejsca, w którym stanęło. Test `with an empty queue the clock waits for the next frame` sprawdza oba końce.

### 2.4 Dlaczego wątek i dlaczego przewinięcie jest na wątku dekodującym

Zwykła klatka dekoduje się w około 0,9 ms (pomiar autora, sekcja 2.11). **Pierwsza klatka po przewinięciu** pliku na początek trwa od 16,5 do 36,7 ms, bo dekoder zaczyna od nowa od klatki kluczowej. Sam `rewind` trwa 1,4 do 2,3 ms: wolny jest dekoder, nie przesunięcie.

Klip próbny pokazał, że przy dekodowaniu na wątku rysującym restart pętli zacinał grę o **28 do 35 ms przy każdym przejściu**, czyli o jedną albo dwie klatki gry. Rozwiązanie: wątek dekodujący robi całą pracę z plikiem, także `rewind`, a wątek rysujący **nigdy nie czeka na dekoder**. Kolejka z 133 ms zapasu to około 3,6 raza więcej niż najwolniejszy zmierzony krok (`133 / 36,7`). Gdy w kolejce są klatki, wolne dekodowanie pierwszej klatki nowego przejścia dzieje się w tle, a rysowanie bierze klatki z zapasu.

Zmierzone w działającej grze (sekcja 2.11): kolejka **nigdy nie spadła poniżej czterech klatek**, zanim klatka została wzięta, i żadna klatka nie została pominięta na styku pętli.

### 2.5 Bezpieczeństwo wątków prostymi słowami

Dwa wątki działają razem i każdy ma jedno zadanie, a reguły są takie:

- **Dekoder należy do jednego wątku.** Ten, który go utworzył, woła każdą jego metodę i go niszczy (zasada w komentarzu `VideoDecoder`). Dzięki temu w dekoderze nie ma ani jednego zamka. Media Foundation i COM są uruchamiane właśnie na tym wątku.
- **OpenGL należy do wątku rysującego.** Teksturę tworzy konstruktor odtwarzacza (wątek rysujący), wysyła ją `update` (też on). Kontekst OpenGL jest związany z jednym wątkiem.
- **Wspólne są tylko: kolejka, pula zapasowych buforów i kilka flag** (`m_failed`, `m_error`, `m_stop`), chronione **jednym mutexem** z jedną zmienną warunkową. Mutex to zamek: kto go trzyma, tego, co jest pod nim, nie rusza nikt inny. Zmienna warunkowa pozwala wątkowi **zasnąć do czasu zmiany**: `wait(lock, warunek)` oddaje zamek na czas snu, sprawdza warunek przy każdym obudzeniu i wraca z zamkiem. Zmianę ogłasza `notify_all`.
- **Praca wolna dzieje się poza zamkiem**: dekodowanie klatki (wątek dekodujący) i wysłanie pikseli do karty (wątek rysujący). Zamek trzyma się tylko na przestawienie kolejki, więc żaden wątek nie czeka na wolną pracę drugiego.
- **Bufory pikseli są recyklingowane** (`m_spare`): w trakcie odtwarzania nic nie jest alokowane.
- **Wyjątek na wątku kończy cały program**, więc ciało wątku jest owinięte w `try { ... } catch (...)`, który zamienia wyjątek w "wideo się nie udało". Ta lambda w konstruktorze niesie jedyny `NOLINTNEXTLINE(bugprone-exception-escape)` w całej tej części, z powodem w komentarzu: clang-tidy widzi, że zablokowanie mutexu też może rzucić.
- **Destruktor** ustawia `m_stop`, budzi wątek i czeka na jego koniec (`join`). Dzięki temu zamknięcie programu w trakcie wideo nie zawiesza się: wątek zauważa `m_stop` przed następną klatką, najwyżej po kilku milisekundach.

Konstruktor odtwarzacza **czeka na pierwszą klatkę** (albo na wiadomość, że jej nie będzie). Zmierzone przez autora: otwarcie dekodera 101 do 109 ms, pierwsza klatka 26 do 37 ms. Dzięki temu wybór tła i jego linia w logu zapadają już przy starcie.

Gdy `update` nie jest wołane (trwa runda), zegar stoi, a wątek dekodujący śpi na pełnej kolejce: wideo nic nie kosztuje i **po powrocie do menu idzie dalej od miejsca, w którym stanęło**. To wybór (wznowienie, nie restart): po około 9 s rundy i pauzie menu wróciło mniej więcej do tego miejsca pętli, w którym zostało (koniec ujęcia z przelotem), a nie 9 s dalej i nie na początek (widziane na zrzutach przez agenta).

### 2.6 Pokrycie okna: `coverFit`

Obraz ma proporcje 16 : 9, a okno może mieć dowolne. Rozciągnięcie zniekształciłoby obraz, a pasy po bokach (letterbox) zostawiłyby puste miejsce. Wybór (wykonawczy): **pokrycie** (cover), jak tapeta na pulpicie. Obraz jest skalowany tak, żeby pokrył całe okno, a to, co wystaje po dwóch przeciwległych stronach, jest odcinane, **po tyle samo z każdej**. Środek obrazu zostaje w środku okna.

`gfx::coverFit` zwraca prostokąt we współrzędnych tekstury (0 do 1), czyli **jaka część obrazu jest widoczna**. Liczą się tylko kształty, nie rozmiary: 1280 x 720 w oknie 1920 x 1080 to cały obraz.

```cpp
UvRect coverFit(int pictureWidth, int pictureHeight, int targetWidth, int targetHeight) {
    UvRect shown;
    if (pictureWidth < 1 || pictureHeight < 1 || targetWidth < 1 || targetHeight < 1) {
        return shown;
    }

    // Width divided by height: 1.78 for 16 : 9. The casts make it a division of floats.
    const float pictureAspect =
        static_cast<float>(pictureWidth) / static_cast<float>(pictureHeight);
    const float targetAspect = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);

    if (targetAspect > pictureAspect) {
        // The target is wider: the full width is shown, and of the height only the
        // share that gives the shown part the shape of the target.
        const float shownShare = pictureAspect / targetAspect;
        shown.top = (1.0F - shownShare) * HALF;
        shown.bottom = shown.top + shownShare;
    } else {
        // The target is narrower (or the same): the full height, a share of the width.
        const float shownShare = targetAspect / pictureAspect;
        shown.left = (1.0F - shownShare) * HALF;
        shown.right = shown.left + shownShare;
    }
    return shown;
}
```

(Plik: `src/gfx/CoverFit.cpp`, linie 14 do 38.)

Dwa przykłady policzone ręcznie (obraz 1280 x 720, `pictureAspect = 1280 / 720 = 1,7778`):

**Okno 1100 x 700.** `targetAspect = 1100 / 700 = 1,5714`. To mniej niż `1,7778`, więc cel jest węższy: cała wysokość, część szerokości. `shownShare = 1,5714 / 1,7778 = 0,88393`. `left = (1 - 0,88393) * 0,5 = 0,05804`, `right = 0,05804 + 0,88393 = 0,94196`. W pikselach obrazu: widoczne `0,88393 * 1280 = 1131` kolumn z 1280, a po `74` kolumny odcięte z lewej i z prawej. `top = 0`, `bottom = 1`.

**Okno 800 x 900.** `targetAspect = 800 / 900 = 0,8889`, znowu węższy. `shownShare = 0,8889 / 1,7778 = 0,5`. `left = (1 - 0,5) * 0,5 = 0,25`, `right = 0,75`. Widoczne `640` kolumn z 1280, po `320` odciętych z każdej strony. To okno pokazuje tylko środkową połowę obrazu na całą wysokość.

Dla porównania okno 1600 x 900 ma kształt 16 : 9: `shownShare = 1`, cały obraz. Okno 2560 x 1080 (szersze) ma `shownShare = 1,7778 / 2,3704 = 0,75`, więc `top = 0,125`, `bottom = 0,875` (to wartości z testu `a wider target keeps the full width and loses rows at the top and the bottom`).

Agent oglądał menu na zrzutach w czterech rozmiarach: 1280 x 720, 1600 x 900, 1100 x 700 i 800 x 900 (obraz pokrywa okno, nie jest rozciągnięty, tekst menu czytelny). Pełny ekran i rozmiary powyżej 1600 x 900 nie były oglądane.

### 2.7 Ścieżka koloru: BT.709, zakres ograniczony

Wideo H.264 nie przechowuje RGB. Przechowuje **jasność** (Y') i dwie **różnice kolorów** (Cb, Cr), przy czym różnice kolorów mają **połowę rozdzielczości** (format `yuv420p`). Liczby nie zajmują pełnego zakresu 0 do 255: w **zakresie ograniczonym** (limited, "tv") jasność idzie od 16 do 235. Żeby dostać RGB, trzeba znać **macierz**, czyli wzór przeliczenia liczb Y', Cb i Cr na R', G' i B'. Dla obrazu w wysokiej rozdzielczości używa się macierzy BT.709 (jej współczynników tu nie przytaczam: nie ma ich w kodzie projektu, a program ich nie liczy, robi to dekoder systemu). Gdyby dekoder użył innej macierzy (BT.601, dla obrazu o niskiej rozdzielczości), kolory byłyby przekłamane: zieleń za ciemna.

W tym projekcie droga jest taka:

1. **Nagranie** (`tools/record_menu_loop.py`) koduje wideo z `scale=out_color_matrix=bt709:out_range=tv`, `format=yuv420p` i na końcu `setparams` z BT.709 i zakresem `tv`, żeby **plik był oznaczony** tym, czym jest (sekcja 2.12).
2. **Dekoder** (Media Foundation) przelicza YUV na RGB (czy robi to według znaczników pliku, zmierzono tylko pośrednio: wynik zgadza się z ffmpeg użytym z BT.709, sekcja niżej): prosimy go o format wyjściowy `MFVideoFormat_RGB32` i włączamy `MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING`, który pozwala czytnikowi na takie przeliczenie. Wynik to bajty B, G, R i jeden nieużywany, już **zakodowane do wyświetlenia** (wartości sRGB).
3. **Tekstura** `GL_RGBA8` (nie `GL_SRGB8_ALPHA8`): bajty trafiają do shadera bez dekodowania sRGB.
4. **Shader** zapisuje je bez zmian, a `GL_FRAMEBUFFER_SRGB` jest wyłączone, tak jak w przebiegu składającym i nakładce minimapy. **Nie wolno** tu użyć `linearToSrgb` z `composite.frag`: te bajty są już zakodowane, drugie kodowanie rozjaśniłoby obraz i zbladłby.
5. **Obraz nieruchomy** jest ładowany od górnego wiersza (`RowOrder::TopFirst`) i przechodzi przez tę samą klasę tekstury i ten sam shader.

Pomiary autora kodu (sekcja 2.11): dekoder kontra ffmpeg (`scale=in_color_matrix=bt709:in_range=tv`) na klatkach 0, 300 i 899 pętli: Media Foundation jest jaśniejszy o 0,83 do 1,45 wartości kodowej średnio (średni błąd bezwzględny 0,87 do 1,46), 55 do 90 procent pikseli różni się najwyżej o 1, PSNR 44,2 do 47,3 dB. Największe pojedyncze różnice (do 59) leżą na ostrych krawędziach kolorów, gdzie oba programy inaczej powiększają kolor o połowie rozdzielczości. Że macierz jest właściwa, pokazały kolorowe pasy testowe: przez te same ustawienia kodowania średni błąd bezwzględny wobec ffmpeg z BT.709 to 0,60 do 0,79, a wobec ffmpeg z BT.601 2,6 do 4,7 (zieleń średnio o 3,9 za ciemna). Droga shadera: zrzut tła w postaci obrazu nieruchomego w 1280 x 720, obszar x 1010 do 1270, y 10 do 490 (gdzie dokumenty menu są w całości przezroczyste), jest **bajt w bajt** plikiem PNG (100 procent zgodności, w dwóch przebiegach). **Nie porównano** zrzutu odtwarzanego wideo z klatką, którą ten zrzut pokazuje.

Przyciemnienia w shaderze nie dodano: uniform `uBrightness` istnieje i ma wartość 1,0 (`BACKGROUND_BRIGHTNESS`). Dokumenty menu same przyciemniają obraz pod tekstem (`body.left` i `body.dim` w `assets/ui/menu.rcss`), a na zrzutach tekst był czytelny na wszystkich czterech ujęciach pętli, także w jasnym stożku latarki.

### 2.8 Media Foundation krok po kroku (Windows)

Media Foundation to część Windowsa, która czyta i dekoduje pliki medialne. Jej obiekty to obiekty COM: funkcja tworzy obiekt i wpisuje wskaźnik, każde wywołanie odpowiada kodem `HRESULT` (`SUCCEEDED` albo `FAILED`), a obiekt żyje do ostatniego `Release()`. `ComPtr` to inteligentny wskaźnik Windowsa, który woła `Release()` w destruktorze, jak `std::unique_ptr` woła `delete`.

Kolejność w `MediaFoundationDecoder::open` i wokół niego:

1. **Sonda** `mediaFoundationInstalled()` (poniżej): dla `mfplat.dll` i `mfreadwrite.dll` woła `LoadLibraryExW(nazwa, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)` (szuka tylko w katalogu Windowsa, nie w katalogu gry) i od razu `FreeLibrary`. Brak którejś biblioteki to `false`.
2. `CoInitializeEx(nullptr, COINIT_MULTITHREADED)`: COM trzeba uruchomić na tym wątku przed Media Foundation. Wątek dekodujący nie ma okna, więc tryb wielowątkowy.
3. `MFStartup(MF_VERSION)`.
4. **Atrybuty** czytnika: `MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = TRUE`, żeby czytnik mógł przeliczyć YUV na RGB.
5. `MFCreateSourceReaderFromURL(file.c_str(), ...)`: **czytnik źródła** (source reader). `path::c_str()` jest na Windowsie szerokim napisem, więc ścieżka z polskimi literami działa. To wywołanie zawodzi dla pliku, który nie jest wideo (zaobserwowany kod `0xC00D36C4` dla pliku 300 KB innych bajtów o nazwie `menu_loop.mp4`).
6. **Wybór strumienia**: wszystkie wyłączone, pierwszy strumień wideo włączony. Ścieżka dźwiękowa, gdyby była, nie jest nawet dekodowana.
7. **Typ wyjściowy**: `MFMediaType_Video` i `MFVideoFormat_RGB32` przez `SetCurrentMediaType`. To wywołanie zawodzi, gdy Windows nie ma dekodera dla tego wideo (komunikat "Windows has no decoder that turns this video into pictures").
8. `readOutputFormat`: rozmiar klatki (`MF_MT_FRAME_SIZE`), tempo jako ułamek (`MF_MT_FRAME_RATE`, `30 / 1` albo `30000 / 1001` dla telewizyjnych 29,97), z niego `frameSeconds` jako odwrotność, i **krok wiersza** (`MF_MT_DEFAULT_STRIDE`), który jest zapisany jako liczba bez znaku, a znaczy liczbę ze znakiem (ujemny dla obrazu przechowywanego od dolnego wiersza).
9. `readFrame`: blokujące `ReadSample`. Flagi: błąd to `Failed`, koniec strumienia to `EndOfFile`, zmiana typu wyjściowego sprawdza, czy rozmiar został ten sam (odtwarzacz ma jedną teksturę jednego rozmiaru), a próbka pusta bez końca to przerwa w strumieniu, więc pętla idzie po następną. Znacznik z `ReadSample` jest w jednostkach 100 ns (dziesięć milionów na sekundę) i jest dzielony przez `STAMP_UNITS_PER_SECOND`.
10. `copyFrame`: `ConvertToContiguousBuffer` (próbka może trzymać bajty w kilku buforach), potem lepsza droga przez `IMF2DBuffer::Lock2D`, który mówi, gdzie jest górny wiersz i jaki jest krok, albo zwykła przez `Lock` z krokiem z formatu. `copyRows` kopiuje wiersze od górnego, także gdy krok jest ujemny.
11. `rewind` (sekcja 5.4): `SetCurrentPosition(GUID_NULL, ...)` z `PROPVARIANT` typu `VT_I8` o wartości 0 (czas w jednostkach 100 ns).
12. **Destruktor** zwalnia w odwrotnej kolejności niż start: czytnik, `MFShutdown`, `CoUninitialize`.

```cpp
bool mediaFoundationInstalled() {
    for (const wchar_t* name : MEDIA_FOUNDATION_LIBRARIES) {
        // Only the directory of Windows itself is searched, not the directory of the
        // game or the working directory.
        const HMODULE library = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (library == nullptr) {
            return false;
        }
        // The question is answered. The library is loaded again at the first call.
        FreeLibrary(library);
    }
    return true;
}
```

(Plik: `src/video/VideoDecoderWindows.cpp`, linie 66 do 78.)

**Windows N.** Edycje N systemu Windows nie mają Media Foundation, dopóki nie zainstaluje się Media Feature Pack. Program, który zależy od brakującej biblioteki DLL, **w ogóle się nie uruchamia**. Dlatego obie biblioteki są **ładowane z opóźnieniem** (`/DELAYLOAD`, biblioteka `delayimp`): Windows ładuje je przy pierwszym wywołaniu, a sonda pyta o nie wcześniej. `dumpbin /imports` pliku Release (pomiar autora) wymienia obie w sekcji importów z opóźnieniem i żadnej wśród bibliotek ładowanych przy starcie. **Nie sprawdzono na prawdziwej edycji N**: komunikat "Media Foundation is not installed (a Windows N edition needs the Media Feature Pack)" wywołano, kierując sondę na nazwę nieistniejącej biblioteki w lokalnej wersji, której nie zatwierdzono.

```cmake
    target_link_options(video INTERFACE "/DELAYLOAD:mfplat.dll" "/DELAYLOAD:mfreadwrite.dll")
    target_link_libraries(video INTERFACE delayimp)
```

(Plik: `CMakeLists.txt`, linie 241 do 242.)

### 2.9 Dekoder macOS: napisany, nigdy nieuruchomiony

**To jest kod niesprawdzony.** [`src/video/VideoDecoderApple.mm`](../../../src/video/VideoDecoderApple.mm) napisano na komputerze z Windowsem, na podstawie dokumentacji AVFoundation. **Nigdy go nie skompilowano i nigdy nie uruchomiono.** Plik sam to mówi w pierwszym komentarzu. Co robi (według kodu, bez gwarancji, że działa):

- plik `.mm` to Objective-C++ (C++ z możliwością wołania klas Apple'a, metody w nawiasach kwadratowych), kompilowany z ARC (`-fobjc-arc`): kompilator sam zwalnia obiekty Objective-C,
- trzy obiekty: **asset** (`AVURLAsset`, plik jako całość), **czytnik** (`AVAssetReader`, czyta asset raz od początku do końca) i **wyjście** (`AVAssetReaderTrackOutput`, wydaje zdekodowane klatki jednej ścieżki). Format wyjściowy `kCVPixelFormatType_32BGRA`, ten sam układ bajtów co z Windowsa,
- `readFrame` woła `copyNextSampleBuffer`, a brak próbki znaczy koniec pliku, gdy czytnik ma stan `Completed`, a błąd w innym razie,
- **`rewind` tworzy nowy `AVAssetReader`** dla tego samego assetu, bo czytnik czyta raz i cofnąć się nie umie (na wątku dekodującym, tak samo jak na Windowsie),
- trzy właściwości odczytywane synchronicznie (`tracksWithMediaType:`, `formatDescriptions`, `nominalFrameRate`) są oznaczone przez Apple'a jako przestarzałe od macOS 15. Kod ma wokół nich `#pragma clang diagnostic`, bo działa na wątku, którego jedyne zadanie to czekanie na dekoder.

Punkty, które są **zgadywaniem** i musi je sprawdzić Mac (piętnaście punktów jest w [`../../guides/build-macos.md`](../../guides/build-macos.md)): rzutowanie `__bridge` na `CMVideoFormatDescriptionRef`, `return` wewnątrz `@autoreleasepool` w pętli, działanie `enable_language(OBJCXX)` w CMake, to, czy `clang-format` na Macu sformatuje plik tak samo jak ten z Visual Studio, to, czy obraz jest odwrócony albo ma zamienione kanały, i to, czy AVFoundation nie stosuje własnej obróbki koloru do wyjścia 32BGRA. Gdyby `openVideoDecoder` zawiodło, menu pokazuje obraz nieruchomy, więc gra jest używalna.

### 2.10 Wybór tła: wideo, obraz, żywa scena

Reguła jest zwykłą funkcją bez okna, OpenGL i dekodera (`game::chooseMenuBackground`), żeby dało się ją przetestować. Wejście: czego chciano (`wanted`: domyślnie wideo, inaczej według `--menu-background`), czy wideo gra, czy obraz wczytano i tekst błędu wideo.

| Czego chciano | Wideo gra | Obraz wczytany | Pokazane |
|---|---|---|---|
| żywa scena | dowolnie | dowolnie | żywa scena |
| wideo | tak | dowolnie | wideo |
| wideo | nie | tak | obraz |
| obraz | dowolnie | tak | obraz |
| wideo albo obraz | nie (albo nie pytano) | nie | żywa scena |

Żywa scena nie potrzebuje żadnego pliku, więc **menu zawsze ma tło**. Obraz jest wczytywany **tylko wtedy, gdy jest potrzebny**. Wybór kończy **jedna linia w logu**: `info`, gdy jest to, o co poproszono, `warn` w przeciwnym razie. Linie zaobserwowane przez autora kodu:

- `[info] Menu background: video (the video plays)`
- `[info] Menu background: still (asked for on the command line)`
- `[info] Menu background: scene (asked for on the command line)`
- `[warn] Menu background: still (the video cannot be played: the file is missing [...menu_loop.mp4])`
- `[warn] Menu background: still (the video cannot be played: Media Foundation cannot read the file as a video (HRESULT 0xC00D36C4) [...])` (plik 300 KB innych bajtów o nazwie `menu_loop.mp4`)
- `[warn] Menu background: still (the video cannot be played: Media Foundation is not installed (a Windows N edition needs the Media Feature Pack) [...])` (zasymulowane, sekcja 2.8)
- `[warn] Menu background: scene (the video cannot be played: the file is missing [...], and the still image cannot be loaded either)` (po linii `[error]` z ładowania obrazu)

Dekoder, który poddaje się **w trakcie** odtwarzania, jest obsłużony tak samo w `MenuBackgroundRenderer::update` (wideo jest wtedy zastąpione obrazem, a gdy go nie ma, sceną, i pada nowa linia logu). **Ta ścieżka nie była uruchomiona.**

```cpp
MenuBackgroundChoice chooseMenuBackground(MenuBackground wanted, bool videoPlays, bool stillLoaded,
                                          std::string_view videoError) {
    if (wanted == MenuBackground::LiveScene) {
        return {.background = MenuBackground::LiveScene, .reason = ASKED_FOR_REASON};
    }
    if (wanted == MenuBackground::Video && videoPlays) {
        return {.background = MenuBackground::Video, .reason = VIDEO_PLAYS_REASON};
    }

    // From here on the video is not shown: it was not asked for, or it failed.
    const std::string whyNoVideo =
        wanted == MenuBackground::Still ? ASKED_FOR_REASON : videoFailedReason(videoError);
    if (stillLoaded) {
        return {.background = MenuBackground::Still, .reason = whyNoVideo};
    }
    return {.background = MenuBackground::LiveScene,
            .reason = whyNoVideo + ", and " + NO_STILL_REASON};
}
```

(Plik: `src/game/MenuBackground.cpp`, linie 28 do 45.)

### 2.11 Pomiary autora kodu

**Wszystko w tej sekcji to pomiary autora kodu na jego komputerze: Windows, build Release, okno 1280 x 720, karta NVIDIA GeForce.** Nie ma ich w repozytorium i ich nie powtarzałem. Narzędzia pomiarowe leżą poza repozytorium.

**Dekoder** (małe narzędzie poza repozytorium zbudowane z `VideoDecoderWindows.cpp`):

| Klip | Otwarcie | Pierwsza klatka przejścia (po `rewind`) | Inne klatki, średnio | Najwolniejsza inna klatka | Samo wywołanie `rewind` |
|---|---|---|---|---|---|
| klip próbny (High, klatki B, 720 klatek) | 109 ms | 27,4 do 36,7 ms | 0,84 do 0,89 ms | 15,6 do 17,8 ms | 1,9 do 2,3 ms |
| pętla (High, bez klatek B, 900 klatek) | 101 ms | 16,5 do 26,0 ms | 0,88 do 0,95 ms | 13,8 do 15,7 ms | 1,4 do 1,6 ms |

Wolnym krokiem jest **pierwsze dekodowanie po przewinięciu**, nie samo przewinięcie. W odtwarzaczu oba dzieją się na wątku dekodującym. **Wysłanie klatki do karty** na wątku rysującym: 1,06 ms średnio, 3,4 ms maksimum (3796 wysłań).

**Miejsce zamknięcia pętli w działającej grze** (tymczasowe pomiary w kodzie, niezatwierdzone), menu główne 1280 x 720, pięć przejść w dwóch przebiegach:

| Przejście zaczyna się | Odstęp klatek wideo przed / po pierwszej klatce przejścia | Najdłuższa klatka gry w 250 ms od niego | Długość kolejki |
|---|---|---|---|
| 30,03 s (przebieg 1) | 32,4 / 30,3 ms | 15,9 ms | 4 |
| 60,02 s (przebieg 1) | 32,5 / 34,1 ms | 8,3 ms | 4 |
| 90,02 s (przebieg 1) | 33,4 / 33,0 ms | 4,3 ms | 4 |
| 120,02 s (przebieg 1) | 33,3 / 33,5 ms | 5,1 ms | 4 |
| 30,00 s (przebieg 2) | 33,3 / 33,2 ms | 4,3 ms | 4 |

Kolejka **nigdy nie spadła poniżej czterech klatek** przed wzięciem klatki i żadna klatka nie została pominięta na styku pętli. **Te 15,9 ms przy pierwszym przejściu to nie restart pętli:** w przebiegu 1 były **serie długich klatek** między 12 s a 35 s (do 101 ms, 133 klatki powyżej 20 ms, 26 pominiętych klatek wideo), przy czym sam `glFinish` był wolny, co wskazuje na coś innego używającego karty graficznej. **Nie były związane z miejscem zamknięcia pętli i nie powtórzyły się:** przebieg 2 (40 s) nie miał klatki powyżej 12,4 ms i miał jedną pominiętą klatkę wideo.

**Szew pętli w samym pliku:** średnia bezwzględna różnica jasności między ostatnią a pierwszą klatką to 1,63, a między zwykłymi sąsiadami w pobliżu szwu 1,6 do 2,4.

**Czas klatki menu**, 1280 x 720, Release, około 14 s na wariant. Vsync nie działał na tym komputerze w żadnym przebiegu (okno debug pokazywało ponad 1000 FPS), więc to prawdziwe czasy klatek. Pomiar dodaje `glFinish` na klatkę:

| Tło | Czas klatki, średnio | mediana | `onRender` plus `glFinish`, średnio |
|---|---|---|---|
| żywa scena (`--menu-background scene`) | 2,25 ms | 1,97 ms | 2,05 ms |
| wideo | 1,19 ms | 1,09 ms | 1,02 ms |
| obraz nieruchomy (`--menu-background still`) | 1,00 ms | 0,72 ms | 0,80 ms |

**Plik:** 17 646 243 bajtów (rozmiar pliku w repozytorium, odczytany przy pisaniu tego dokumentu), 900 klatek, 30,0 s, czyli `17 646 243 * 8 / 30 = 4,71 Mbit/s`. Notatka autora kodu podaje 17 650 837 bajtów (różnica 4 594 bajty, niewyjaśniona; liczby w tym dokumencie pochodzą z pliku w repozytorium). Dla porównania ta sama pętla przy CRF 18 miała 13,18 MB, 3,52 Mbit/s (pomiar autora). Obraz nieruchomy: 611 660 bajtów, 1280 x 720, RGB.

### 2.12 Nagranie pętli (skrót)

Pełny opis skryptu jest w [`../game/menu-camera.md`](../game/menu-camera.md), sekcja 5.9. W skrócie: `tools/record_menu_loop.py` uruchamia grę w trybie kamery menu dla każdego ujęcia z listy, nagrywa okno bezstratnie, skleja ujęcia przenikaniami w jedną pętlę bez szwu, koduje jeden plik H.264 z oznaczeniem BT.709 i zakresu `tv` i zapisuje klatkę 0 jako PNG. Pętla w repozytorium ma cztery ujęcia po 8,5 s, ziarno 1, poziom `Easy`. **Nie ma w niej odbicia w kałuży** (kamery nie patrzą w dół).

## 3. Jak to działa w OpenGL

**Tekstura klatki** (`gfx::FrameTexture`, [`src/gfx/FrameTexture.cpp`](../../../src/gfx/FrameTexture.cpp)):

- `glTexImage2D` z `GL_RGBA8` i wskaźnikiem pustym tworzy **pamięć bez zawartości**. Każde wysłanie wypełnia ją przez `glTexSubImage2D` (nie tworzy nowej pamięci). `GL_RGBA8`, a nie `GL_SRGB8_ALPHA8`: obrazy są już zakodowane do wyświetlenia, a shader zapisuje je bez zmian.
- `GL_LINEAR` w obu kierunkach i **bez mipmap**. Tekstura bez mipmap nie może mieć filtru zmniejszania, który się do nich odwołuje (domyślny to robi), bo czytałaby się jako czarna.
- `GL_CLAMP_TO_EDGE`: na brzegu powtarza się ostatni teksel, więc mieszanie nigdy nie sięga na przeciwległą krawędź.
- `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` na czas wysłania, bo wiersze są ułożone ciasno, także przy trzech bajtach na piksel. Poprzednia wartość wraca (ustawienie należy do całego kontekstu).
- Kolejność bajtów: klatka z dekodera to `GL_BGRA` (`PixelOrder::Bgra`), obraz z pliku to `GL_RGB` albo `GL_RGBA`. **Pierwszy wiersz wchodzi do `v = 0`**, więc obraz zapisany od górnego wiersza jest czytany z `v = 0` u góry.
- `bind(unit)` aktywuje jednostkę, wiąże teksturę i **czyści obiekt samplera tej jednostki** (`glBindSampler(unit, 0)`). Obiekt samplera wygrywa z parametrami tekstury: `Texture2D` zostawiłby tam sampler z `GL_REPEAT` i szukaniem mipmap.

**Rysowanie tła** (`MenuBackgroundRenderer::draw`): wyłącza test głębi (bufor głębi okna nigdy nie jest czyszczony), mieszanie i `GL_FRAMEBUFFER_SRGB`, ustawia pięć uniformów, wiąże pusty VAO (profil Core wymaga go przy każdym rysowaniu) i rysuje **jeden trójkąt na cały ekran** (`glDrawArrays(GL_TRIANGLES, 0, 3)` z `post/composite.vert`, trzy wierzchołki z `gl_VertexID`). Trójkąt pokrywa każdy piksel i zastępuje to, co tam jest.

**Klatka z tłem** (`NightMazeApp::onRender`): zaraz po sprawdzeniu bufora o rozmiarze 0, gdy `usesMenuCamera(m_mode)`, wideo rusza o `time().deltaSeconds()`, a potem pada pytanie `drawsScene(m_mode, coversWindow(...))`. Gdy fałsz, klatka to: okno jako cel, trójkąt tła, `m_ui.draw`, koniec. **Pominięte:** oba przebiegi cieni, przebieg sceny HDR, podglądy, bloom, przebieg składający, minimapa, wysłanie świateł i zegar kamery menu. `drawsScene` była od części 2 przetestowana, ale nieużywana: **od tej części jest wpięta**.

```cpp
    if (usesMenuCamera(m_mode)) {
        m_menuBackground.update(time().deltaSeconds());
    }
    if (!drawsScene(m_mode, coversWindow(m_menuBackground.background()))) {
        // Nothing is picked under a menu, as in the frames that draw the scene.
        m_pick = pickNothing(m_round);
        // The picture goes straight into the window, like the composite pass does.
        gfx::Framebuffer::bindDefault(framebuffer.width, framebuffer.height);
        m_menuBackground.draw(framebuffer);
        m_ui.draw(framebuffer);
        return;
    }
```

(Plik: `src/game/NightMazeApp.cpp`, linie 1038 do 1049.)

## 4. Shadery

[`assets/shaders/post/menu_background.frag`](../../../assets/shaders/post/menu_background.frag) działa z tym samym `post/composite.vert` co przebieg składający. Uniformy: `uPicture` (jednostka tekstury), `uShownLeft`, `uShownRight`, `uShownTop`, `uShownBottom` (prostokąt z `coverFit`) i `uBrightness`.

```glsl
void main() {
    // From the window to the picture. The window counts v from the bottom up and the
    // picture has its top row at v = 0, so the top of the window (vUv.y = 1) reads the
    // top edge of the shown part and the bottom of the window its bottom edge.
    vec2 uv = vec2(mix(uShownLeft, uShownRight, vUv.x), mix(uShownBottom, uShownTop, vUv.y));

    // The bytes of the picture are sRGB values already, like the window expects them:
    // a video and a picture file are stored encoded for a screen. The texture is
    // a GL_RGBA8 one, so they arrive here unchanged, and GL_FRAMEBUFFER_SRGB is off, so
    // they are written unchanged. Encoding them with linearToSrgb, as composite.frag
    // does with the scene, would encode them a second time and make the picture pale.
    //
    // The scrim multiplies those encoded values. That is not the physically right way to
    // dim light, but it is a look and not a lighting step, and the menu documents on top
    // are blended in the same encoded values.
    vec3 color = texture(uPicture, uv).rgb * uBrightness;

    fragColor = vec4(color, 1.0);
}
```

(Plik: `assets/shaders/post/menu_background.frag`, linie 32 do 50.)

Co dzieje się w `main`:

- `vUv` to współrzędna okna, `(0, 0)` w lewym dolnym rogu. Obraz ma górny wiersz przy `v = 0`. Dlatego górna krawędź okna (`vUv.y = 1`) czyta **górną** krawędź widocznej części (`uShownTop`), a dolna **dolną** (`uShownBottom`): `mix(uShownBottom, uShownTop, vUv.y)`. `mix(a, b, t)` to `a * (1 - t) + b * t`.
- `texture(uPicture, uv).rgb * uBrightness`: bajty są już zakodowane (sRGB), więc przychodzą bez zmian i wychodzą bez zmian. `uBrightness` wynosi 1,0.
- Alfa 1,0: tło nie miesza się z tym, co było.

Dla okna 800 x 900 z przykładu w sekcji 2.6 piksel w środku okna (`vUv = (0,5; 0,5)`) czyta `u = mix(0,25; 0,75; 0,5) = 0,5` i `v = mix(1; 0; 0,5) = 0,5`: środek obrazu. Piksel przy lewej krawędzi okna (`vUv.x = 0`) czyta `u = 0,25`, czyli kolumnę 320 obrazu.

## 5. Kod w projekcie

### 5.1 Pliki

| Plik | Biblioteka | Co |
|---|---|---|
| `src/gfx/CoverFit.hpp`, `.cpp` | `engine` | `gfx::UvRect`, `gfx::coverFit`: czysta matematyka |
| `src/gfx/FrameTexture.hpp`, `.cpp` | `engine` | `gfx::FrameTexture`, `gfx::PixelOrder` |
| `src/video/VideoClock.hpp`, `.cpp` | `video` (i bezpośrednio w testach) | `timelineSeconds`, `passSeconds`, `dueFrameCount`, `clockWithEmptyQueue` |
| `src/video/VideoDecoder.hpp` | `video` | jeden interfejs platformy |
| `src/video/VideoDecoderWindows.cpp` | `video` | Media Foundation. Całe ciało w `#ifdef _WIN32`, plik jest na liście kompilowanych na każdym systemie |
| `src/video/VideoDecoderApple.mm` | `video`, tylko Apple | AVFoundation, Objective-C++ z ARC. **Niesprawdzony** |
| `src/video/VideoPlayer.hpp`, `.cpp` | `video` | `video::VideoPlayer`: wątek, kolejka, zegar, tekstura |
| `src/game/MenuBackground.hpp`, `.cpp` | `game_logic` | `MenuBackground`, `chooseMenuBackground`, `coversWindow`, `menuBackgroundName`, `parseMenuBackground` |
| `src/game/MenuBackgroundRenderer.hpp`, `.cpp` | `night_maze` | otwiera wideo i obraz, loguje wybór, rysuje |
| `assets/shaders/post/menu_background.frag` | | shader z sekcji 4 |
| `assets/video/menu_loop.mp4`, `menu_still.png` | | pętla i jej klatka 0 |
| `tools/record_menu_loop.py` | | nagranie |

Zmienione: `CMakeLists.txt` (biblioteka `video`, nowe pliki, `src/video/VideoClock.cpp` w programie testowym), `Makefile` (`OBJCXX_SOURCES` tylko w `format` i `format-check`, **nie** w `tidy`), `.clang-format` (druga sekcja `Language: ObjC` z tymi samymi opcjami), `.gitattributes` (`*.mp4 binary`), `src/game/StartOptions.*` (pole `menuBackground`), `src/game/NightMazeApp.*` (składowa `m_menuBackground`, wczesna gałąź w `onRender`).

### 5.2 Interfejs platformy

```cpp
/// One pixel of a decoded frame: blue, green, red and one unused byte, in this order.
/// It is the order both operating systems deliver, and OpenGL takes it as GL_BGRA.
constexpr std::size_t VIDEO_BYTES_PER_PIXEL = 4;

/// What a decoder knows about its file once it is open.
struct VideoFormat {
    /// The size of a frame in pixels.
    int width = 0;
    int height = 0;

    /// How long one frame is shown, in seconds: 1 / 30 for 30 frames per second.
    double frameSeconds = 1.0 / 30.0;
};

/// How a call of VideoDecoder::readFrame ended.
enum class VideoRead {
    Frame,     ///< a frame was decoded: pixels and stampSeconds are filled
    EndOfFile, ///< the last frame was read before: nothing was filled, rewind goes on
    Failed,    ///< the decoder gave up: nothing was filled, and the file cannot be played
};

/// An open video file. It hands out the frames in the order they are shown.
///
/// A decoder belongs to ONE thread: the thread that called openVideoDecoder makes every
/// later call and destroys the object. Media Foundation is started for a thread, and
/// keeping to this rule means no part of a decoder ever needs a lock.
class VideoDecoder {
public:
    VideoDecoder() = default;
    virtual ~VideoDecoder() = default;

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    /// The size of the frames and the time one of them is shown.
    virtual VideoFormat format() const = 0;

    /// Decodes the next frame. On VideoRead::Frame, pixels holds width * height *
    /// VIDEO_BYTES_PER_PIXEL bytes, row by row without gaps, TOP row first, and
    /// stampSeconds the time the file gives the frame, counted from the start of the
    /// file. pixels is resized as needed: handing in a vector of the right size again
    /// and again decodes without allocating memory.
    ///
    /// The call blocks until the frame is decoded (a few milliseconds).
    virtual VideoRead readFrame(std::vector<unsigned char>& pixels, double& stampSeconds) = 0;

    /// Goes back to the first frame of the file: the next readFrame decodes it. False
    /// when that failed. The call itself is short, but the readFrame after it is the
    /// slow one (measured on Windows: 16 to 37 milliseconds, against less than one for
    /// any other frame, because the decoder starts again at a key frame). That is why
    /// a player keeps both away from the thread that draws.
    virtual bool rewind() = 0;
};

/// Opens a video file with the decoder of the operating system. The file is an MP4 with
/// H.264 video, the one format both systems decode without anything installed.
///
/// Returns the decoder, or nullptr with one sentence in error: the decoder of the
/// system is missing (a Windows N edition without the Media Feature Pack), the file is
/// not a video it can decode, or a step of setting it up failed. It does not throw and
/// it does not log.
std::unique_ptr<VideoDecoder> openVideoDecoder(const std::filesystem::path& file,
                                               std::string& error);
```

(Plik: `src/video/VideoDecoder.hpp`, linie 19 do 81.)

Trzy rzeczy do zapamiętania: `readFrame` **blokuje** do chwili, gdy klatka jest zdekodowana, i oddaje **górny wiersz pierwszy**; wektor `pixels` może wracać o tym samym rozmiarze i wtedy dekodowanie nic nie alokuje; a `openVideoDecoder` **nie rzuca i nie loguje**, tylko zwraca `nullptr` ze zdaniem w `error`.

### 5.3 Odtwarzacz

Odtwarzacz jest jedną klasą, a jego `update` to dokładnie to, co opisuje sekcja 2.3:

```cpp
void VideoPlayer::update(double deltaSeconds) {
    if (m_texture == nullptr) {
        return;
    }

    // The frame that goes into the texture in this call, if one is due.
    Frame newest;
    bool hasNewest = false;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (m_failed) {
            return;
        }
        const double clockBefore = m_clockSeconds;
        m_clockSeconds = clockBefore + deltaSeconds;

        // The times of the waiting frames, for the question how many of them are due.
        std::array<double, QUEUE_CAPACITY> times{};
        std::size_t count = 0;
        for (const Frame& frame : m_waiting) {
            times[count] = frame.seconds;
            ++count;
        }
        const std::size_t due =
            dueFrameCount(std::span<const double>(times.data(), count), m_clockSeconds);

        // All of them leave the queue. Only the last one is shown: a frame before it
        // is a frame the game was too slow for, and its buffer goes straight back.
        for (std::size_t i = 0; i < due; ++i) {
            if (hasNewest) {
                m_spare.push_back(std::move(newest.pixels));
            }
            newest = std::move(m_waiting.front());
            m_waiting.pop_front();
            hasNewest = true;
        }
        if (hasNewest) {
            m_shownSeconds = newest.seconds;
        }
        // Nothing left to show after this: the clock must not run ahead of the decoder.
        if (m_waiting.empty()) {
            m_clockSeconds = clockWithEmptyQueue(clockBefore, deltaSeconds, m_shownSeconds,
                                                 m_format.frameSeconds);
        }
    }
    if (!hasNewest) {
        return;
    }
    // The decoding thread has room again. It is woken before the upload, so it decodes
    // while this thread copies the pixels to the graphics card (outside of the lock:
    // neither thread ever waits for the slow work of the other).
    m_changed.notify_all();
    m_texture->upload(newest.pixels.data(), gfx::PixelOrder::Bgra);

    const std::lock_guard<std::mutex> lock(m_mutex);
    m_spare.push_back(std::move(newest.pixels));
}
```

(Plik: `src/video/VideoPlayer.cpp`, linie 83 do 139.)

Zwróć uwagę na dwa miejsca. `m_changed.notify_all()` jest wołane **przed** wysłaniem pikseli, żeby wątek dekodujący dekodował, kiedy ten wątek kopiuje do karty, a samo wysłanie jest **poza zamkiem**. A w gałęzi `if (m_waiting.empty())` zegar jest korygowany przez `clockWithEmptyQueue`.

Pętla wątku dekodującego, część o końcu pliku:

```cpp
        if (read == VideoRead::EndOfFile) {
            // The loop: back to the first frame. A file without a single frame would
            // go round here for ever, so it counts as a failure.
            if (!passHasFrames) {
                giveUp("the file holds no frames");
                return;
            }
            if (!decoder->rewind()) {
                giveUp("the decoder cannot go back to the first frame");
                return;
            }
            passStartSeconds +=
                passSeconds(firstStampSeconds, lastStampSeconds, m_format.frameSeconds);
            passHasFrames = false;
            continue;
        }
```

(Plik: `src/video/VideoPlayer.cpp`, linie 216 do 231.)

Plik bez ani jednej klatki kręciłby się tu bez końca, więc liczy się jako porażka (`passHasFrames`). Początek następnego przejścia na osi czasu to suma czasów dotychczasowych przejść.

### 5.4 Windows: przewijanie

```cpp
    bool rewind() override {
        // The position is a time in units of 100 nanoseconds, wrapped in a PROPVARIANT
        // (a struct that can hold a value of many types): VT_I8 is a 64 bit integer.
        // GUID_NULL says that the number is such a time.
        PROPVARIANT start;
        PropVariantInit(&start);
        start.vt = VT_I8;
        start.hVal.QuadPart = 0;
        const HRESULT moved = m_reader->SetCurrentPosition(GUID_NULL, start);
        PropVariantClear(&start);
        return SUCCEEDED(moved);
    }
```

(Plik: `src/video/VideoDecoderWindows.cpp`, linie 209 do 220.)

### 5.5 Przełącznik

`--menu-background <video|still|scene>` jest szóstym przełącznikiem (`START_OPTIONS_USAGE` wymienia sześć). `StartOptions` ma od tej części **pięć pól**: `seed`, `seedGiven`, `menuCamera`, `play` i `menuBackground`. Domyślnie `Video`. Nazwa spoza trzech jest błędem (jak każdy niezrozumiały przełącznik: dwie linie `[error]` i koniec programu). Przełącznik **nic poza tłem nie zmienia**: gra nadal startuje w menu głównym (test `the background of the main menu can be named, and only by its three names`).

```cpp
        } else if (name == MENU_BACKGROUND_SWITCH) {
            understood = parseMenuBackground(value, options.menuBackground);
```

(Plik: `src/game/StartOptions.cpp`, linie 110 do 111.)

### 5.6 Testy

Wszystkie bez okna i bez dekodera.

- **`CoverFitTests.cpp` (5):** ten sam kształt daje cały obraz przy dowolnym rozmiarze (`scale` 1, 2, 3); szerszy cel zachowuje szerokość i traci wiersze (2560 x 1080: `top` 0,125, `bottom` 0,875); węższy zachowuje wysokość i traci kolumny (960 x 720: `left` 0,125, `right` 0,875); pięć celów (1100 x 700, 1366 x 768, 700 x 1100, 3840 x 1080, 1 x 1): widoczna część ma kształt celu, nic spoza obrazu nie jest pokazane, a odcięcie jest po tyle samo z obu stron; rozmiar poniżej 1 daje cały obraz.
- **`VideoClockTests.cpp` (7):** oś czasu idzie dalej tam, gdzie znaczniki zaczynają się od nowa (dwanaście klatek z pliku o czterech klatkach); pierwszy znacznik niezerowy gra tak samo; jedno przejście trwa tyle, ile jego klatki (720 klatek to 24 s); należne klatki (dokładnie w swojej chwili klatka jest należna); gra dwa razy szybsza pokazuje każdą klatkę wideo dwa razy (`shown[i] == (i + 1) / 2`), także na styku pętli; gra wolniejsza pomija klatki i zachowuje prędkość (po 40 klatkach gry, czyli 2 s, wideo jest na klatce 60, nigdy nie cofa się i nigdy nie skacze o więcej niż dwie klatki); z pustą kolejką zegar czeka na następną klatkę.
- **`MenuBackgroundTests.cpp` (6):** wideo, gdy chciano i gra; wideo, które nie gra, daje obraz i podaje powód (tekst błędu jest w powodzie); bez wideo i bez obrazu jest żywa scena i obie porażki są nazwane; obraz i żywą scenę można wymusić, cokolwiek robi wideo; wideo i obraz pokrywają okno, a żywa scena jest sceną; każde tło ma nazwę, a nazwa jest czytana z powrotem.
- **`StartOptionsTests.cpp`:** jeden nowy przypadek (nazwa tła, tylko trzy nazwy) i dwie nowe kontrole, razem 11 przypadków w pliku.

**Nie ma testu na:** dekoder (Media Foundation i AVFoundation), `VideoPlayer` (wątek, kolejka, OpenGL), `MenuBackgroundRenderer`, kolejność klatki w `onRender` ani na shader. Całą tę część sprawdzono zrzutami i pomiarami.

## 6. Okno debugowania (dawniej panel ImGui)

Ta część **nie dodaje żadnej kontrolki** do okna debug. Co widać: okno debug rysuje się nad menu z wideo tak samo jak nad każdym innym tłem. Agent obejrzał w świeżym starcie **każdą kategorię okna debug nad menu z wideo** (build Debug, `GL_CHECK` aktywne, bez linii `[error]` ani `[warn]` w logu); okno nad menu sprawdzono też w buildzie Release. Bufor sceny nie jest tworzony, dopóki scena nie jest rysowana, więc kategoria Post process pokazuje "0 x 0 px" i "no picture yet". Linia "not drawn (bloom off or a debug view)" tej kategorii **nie jest prawdziwym powodem w menu** z wideo: to mały nieaktualny tekst okna debug (znany, nienaprawiony).

## 7. Pułapki

1. **Kod macOS jest niesprawdzony.** Plik `.mm` nigdy nie był kompilowany. Pierwszy build na Macu może go wywrócić i trzeba wtedy zacząć od listy z [`../../guides/build-macos.md`](../../guides/build-macos.md).
2. **Edycja Windows N nie była sprawdzana.** Ścieżka została tylko zasymulowana.
3. **Wideo oglądano na zrzutach, nie okiem.** Płynność w ruchu nie jest potwierdzona przez człowieka.
4. **Format pliku jest sztywny.** Dekoder zakłada MP4 z H.264, który oba systemy odtwarzają bez dodatków. Pętla jest nagrana bez klatek B (`-bf 0`). Klip próbny z klatkami B grał na Windowsie, ale na macOS kolejność klatek z wyjścia czytnika trzeba by sprawdzić (punkt 9 listy Maca).
5. **Rozmiar klatki musi się zgadzać.** Odtwarzacz ma jedną teksturę jednego rozmiaru. Dekoder, który w środku zmieni rozmiar, daje `Failed`, a wideo przechodzi w obraz (ścieżka nieuruchomiona).
6. **Ścieżka pliku pętli.** Plik szuka się jako `assets/video/menu_loop.mp4` względem pliku wykonywalnego ([`../core/paths.md`](../core/paths.md)). Na macOS katalog `assets` obok pliku wykonywalnego jest dowiązaniem do repozytorium, więc zmiana nazwy pliku do próby zapasowej dotyka repozytorium (trzeba nazwę przywrócić).
7. **Wątek w tle trzyma plik.** Zamknięcie programu czeka na wątek dekodujący, najwyżej kilka milisekund.
8. **Wideo stoi, gdy nie jest wołane `update`.** Podczas rundy zegar wideo stoi (wybór: wznowienie). Po powrocie menu zaczyna od miejsca, w którym stanęło.
9. **Obraz nieruchomy jest klatką 0.** Gdy pętla zostanie nagrana na nowo, obraz też trzeba wygenerować od nowa: robi to ten sam skrypt.
10. **Pętla się zestarzeje.** Każda zmiana grafiki czyni ją nieaktualną. Przepis: [`../game/menu-camera.md`](../game/menu-camera.md), sekcja 5.9.
11. **Nagrywanie jest tylko na Windowsie** (`gdigrab`, `user32`). Na Macu nagrania nie przewidziano.
12. **Seria długich klatek w pomiarze nie dotyczy pętli.** Seria z przebiegu 1 nie była związana z miejscem zamknięcia pętli i nie powtórzyła się, ale **nie znaleziono jej przyczyny** (autor podejrzewa inny program używający karty graficznej).

## 8. Ćwiczenia

1. **Oś czasu na kartce.** Plik ma 600 klatek przy 30 na sekundę i pierwszy znacznik 0,1 s. Policz czas pierwszej klatki trzeciego przejścia. Odpowiedź: ostatni znacznik `0,1 + 599/30 = 20,0667 s`, `passSeconds = (20,0667 - 0,1) + 0,0333 = 20,0 s`, początek trzeciego przejścia `2 * 20,0 = 40,0 s`, a czas pierwszej klatki `40,0 + (0,1 - 0,1) = 40,0 s`.
2. **Okno panoramiczne.** Policz `coverFit(1280, 720, 3440, 1440)`. Odpowiedź: `targetAspect = 2,3889`, `shownShare = 1,7778 / 2,3889 = 0,7442`, `top = (1 - 0,7442) * 0,5 = 0,1279`, `bottom = 0,8721`.
3. **Ile klatek pominięto.** Kolejka: `2,000; 2,033; 2,067; 2,100`, zegar `2,075`. Ile klatek wyjdzie i która będzie pokazana? Odpowiedź: trzy (`2,000`, `2,033`, `2,067` są `<= 2,075`), pokazana `2,067`, pominięte dwie.
4. **Pusta kolejka.** Pokazana klatka ma czas `5,0`, zegar `5,0`, `delta` 0,2. Do czego dojdzie zegar? Odpowiedź: `min(5,2; 5,0 + 0,0333) = 5,0333`.
5. **Tło ze sceny.** Uruchom z `--menu-background scene` i porównaj z domyślnym. Co mówi linia logu? Odpowiedź: `[info] Menu background: scene (asked for on the command line)` zamiast `video (the video plays)`.
6. **Brak pliku.** Zmień nazwę `assets/video/menu_loop.mp4` w katalogu builda i uruchom grę. Co pokazuje menu i co mówi log? Odpowiedź: obraz nieruchomy i linię `[warn] Menu background: still (the video cannot be played: the file is missing [...])`.
7. **Obie porażki.** Zmień nazwę obu plików w `assets/video/`. Odpowiedź: żywa scena, jedna linia `[warn]` z końcówką ", and the still image cannot be loaded either" i wcześniej linia `[error]` z ładowania obrazu.

## 9. Pytania kontrolne

1. **Dlaczego biblioteka `video` stoi obok `ui`, a nie w `engine`?**
   `game_logic` i testy linkują `engine`, a nie powinny linkować Media Foundation ani pliku Objective-C++. Potrzebuje tekstury, więc jest nad `gfx`, a o grze nic nie wie, więc pod `game`.
2. **Co jest za jednym interfejsem `VideoDecoder`?**
   Cały kod zależny od systemu: Media Foundation w `VideoDecoderWindows.cpp` i AVFoundation w `VideoDecoderApple.mm`. Żaden inny plik nie dołącza nagłówka systemowego o wideo.
3. **Dlaczego wideo ma własny zegar, a nie "klatka na klatkę gry"?**
   Gra ma dowolne tempo (60, 144, 20), a wideo ma swoje znaczniki. Z własnym zegarem pętla trwa zawsze 30 s.
4. **Do czego służy oś czasu, która nie cofa się?**
   Żeby pytanie "czy klatka jest należna" było jednym porównaniem dwóch liczb także na styku pętli, gdzie znaczniki pliku zaczynają się od nowa.
5. **Dlaczego przewinięcie jest na wątku dekodującym i dlaczego kolejka ma cztery klatki?**
   Pierwsza klatka po przewinięciu trwa 16 do 37 ms, a zwykła około 0,9 ms. Na wątku rysującym dawało to zacięcie 28 do 35 ms przy każdym przejściu. Cztery klatki to 133 ms zapasu, około 3,6 raza więcej niż najwolniejszy krok.
6. **Co chroni mutex, a co dzieje się poza nim?**
   Kolejkę, pulę buforów i flagi. Poza nim jest dekodowanie klatki i wysłanie pikseli do karty.
7. **Dlaczego zegar czeka przy pustej kolejce?**
   Żeby po spóźnieniu dekodera klatki, które dojdą, nie były wszystkie spóźnione naraz i wideo nie przeskoczyło do przodu.
8. **Co robi `coverFit` dla okna 800 x 900?**
   Pokazuje środkową połowę obrazu (`left` 0,25, `right` 0,75) na całą wysokość, bez rozciągania.
9. **Dlaczego shader nie koduje koloru do sRGB?**
   Bajty z dekodera są już zakodowane do wyświetlenia. Drugie kodowanie rozjaśniłoby obraz.
10. **Jaka macierz kolorów jest użyta i skąd wiadomo, że właściwa?**
    BT.709, zakres ograniczony. Pasy testowe dały średni błąd 0,60 do 0,79 wobec ffmpeg z BT.709 i 2,6 do 4,7 wobec BT.601 (pomiar autora).
11. **Dlaczego w kodzie jest sonda `mediaFoundationInstalled`, skoro biblioteki są ładowane z opóźnieniem?**
    Brakująca biblioteka zakończyłaby program przy pierwszym wywołaniu. Sonda pyta wcześniej i pozwala przejść do obrazu nieruchomego.
12. **Co z kodem macOS?**
    Jest napisany z dokumentacji i nigdy nie został skompilowany ani uruchomiony. Wymaga listy kontrolnej z `build-macos.md`.
13. **Jaka jest kolejność wyboru tła?**
    Wideo, jeśli gra, potem obraz, na końcu żywa scena. Zawsze jedna linia logu, `warn`, gdy to nie to, o co poproszono.
14. **Co pomija klatka z tłem wideo?**
    Oba przebiegi cieni, scenę HDR, podglądy, bloom, przebieg składający, minimapę, wysłanie świateł i zegar kamery menu.

## 10. Źródła

- Notatki: [`../../decisions/menu-background-prerendered-loop.md`](../../decisions/menu-background-prerendered-loop.md), [`../../decisions/video-through-os-decoders-with-still-fallback.md`](../../decisions/video-through-os-decoders-with-still-fallback.md), [`../../decisions/menu-in-rmlui.md`](../../decisions/menu-in-rmlui.md).
- Dokumenty: [`../game/menu-camera.md`](../game/menu-camera.md) (nagranie), [`../game/game-states.md`](../game/game-states.md), [`../ui/menu-screens.md`](../ui/menu-screens.md), [`../renderer/post-process.md`](../renderer/post-process.md), [`../gfx/color-space.md`](../gfx/color-space.md), [`../core/paths.md`](../core/paths.md).
- Media Foundation: dokumentacja Microsoftu, "Source Reader" (`IMFSourceReader`, `ReadSample`, `SetCurrentPosition`, `MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING`).
- AVFoundation: dokumentacja Apple'a, `AVAssetReader` i `AVAssetReaderTrackOutput`.
- Macierz BT.709 i zakres ograniczony: ITU-R BT.709.
- `std::mutex`, `std::condition_variable` i `std::thread`: dowolny podręcznik współbieżności w C++.
- Pokrycie (cover) i letterbox: opisy `object-fit: cover` w CSS.
