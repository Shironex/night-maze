# Launcher w Tauri 2 w katalogu `launcher/` tego repozytorium, tylko dla Night Maze

Data: 2026-10-06, uzupełniona 2026-10-07 (sekcja 2a: repozytorium instalatorów, klucze, podpis, samoaktualizacja, licencja, pierwsze wydanie tylko na Windowsa) i wieczorem 2026-10-07 (sekcja 2b: pierwsze wydania). Stan wieczorem 2026-10-07: obowiązuje jako decyzja, **kod launchera jest** (commity od `c5b3cbd` do `0c87e4d`, od 2026-10-07 także podpisy, samoaktualizacja i skrypty wydania, lista w sekcji 2a), **pierwsze wydania są opublikowane** (launcher 0.1.0 i 0.1.1, gra 0.10.0), prawdziwa samoaktualizacja launchera została uruchomiona **raz, na jednym komputerze z Windows 11**, a workflow wydania **nigdy nie działał**, rotacja kluczy, aktualizacja na czystym drugim komputerze i SmartScreen na komputerze, który launchera nie widział, **nigdy nie były uruchomione**, a macOS jest **nieprzetestowany**. Rano tego samego dnia stan był inny: nic nie było opublikowane i żadna aktualizacja nie była uruchomiona. Decyzje właściciela projektu są w sekcjach 2 i 2a; opis stanu, tabela możliwości i skutki to moja analiza. Launcher leży poza ocenianym kodem C++ kursu ([`../guides/launcher.md`](../guides/launcher.md)).
Kod: [`launcher/`](../../launcher/) (biblioteka reguł `crates/core` w Rust, powłoka `src-tauri`, okno w React i TypeScript w `src`, skrypty pakujące w `scripts/`), [`.github/workflows/release.yml`](../../.github/workflows/release.yml). Opis szczegółowy po angielsku: [`launcher/README.md`](../../launcher/README.md).

## 1. Kontekst

Znajomi mają grać w nowsze wersje gry bez ręcznego pobierania i rozpakowywania. Launcher ma jednorazowo zainstalować najnowszą wersję, pilnować aktualizacji i uruchamiać grę. Jest to osobny program w innym języku niż reszta projektu, więc trzeba było rozstrzygnąć, gdzie leży kod, skąd bierze wydania i jak sam się aktualizuje.

## 2. Decyzja

Decyzje właściciela projektu (2026-10-06), w całości:

1. Launcher jest zbudowany w **Tauri 2** i leży w katalogu **`launcher/` tego samego repozytorium**.
2. Obsługuje **tylko Night Maze**, nie jest launcherem ogólnym.
3. Instaluje wersje z **wydań własnego repozytorium gry**, gdy to repozytorium stanie się publiczne.
4. Instalatory samego launchera trafią do **drugiego, małego, publicznego repozytorium**.
5. **Samoaktualizacja launchera i podpisanie manifestu** zostaną ustawione razem z właścicielem **przed pierwszym wydaniem**. Żadnych kluczy dziś nie ma.
6. Przycisk **"Launch vX"** zostaje.
7. **Proces wydania czeka na koniec M9.**

Reszta tej notatki to analiza.

## 2a. Uzupełnienie z 2026-10-07: sześć rozstrzygnięć i co z punktów otwartych jest zamknięte

Decyzje właściciela projektu (2026-10-07), w całości. Sekcja 2 wyżej zostaje bez zmian. Punkty 1 do 5 właściciel wybrał z listy opcji (samych opcji nie mam w materiałach, więc ich nie opisuję); punkt 6 jest opisem pracy, którą właściciel wykonał sam, a nie wyborem z listy.

1. **Znajomi dostają grę przez launcher**, a nie jako zip wysyłany ręcznie.
2. **Drugie publiczne repozytorium na instalatory launchera to `Shironex/night-maze-launcher`**, założone przez właściciela tego dnia.
3. **Licencja, gdy repozytorium gry stanie się publiczne: MIT dla kodu, assety nią nie są objęte.**
4. **Pierwsze wydanie dla znajomych jest tylko na Windowsa.** macOS dojdzie po tym, jak właściciel zbuduje i uruchomi grę oraz launcher na Macu.
5. **Klucze wygenerowane i wyrzucone w trakcie przebiegu testu są danymi testowymi i są dozwolone.** Prawdziwe klucze i klucz deweloperski, który jest w repozytorium, tworzy wyłącznie właściciel.
6. **Sesja kluczy została wykonana przez właściciela tego dnia:** cztery klucze (aktualizacji A i B, manifestu A i B) narzędziem Tauri CLI, przechowywane poza repozytorium, kopie w menedżerze haseł i na drugim dysku, każda kopia sprawdzona przez podpisanie z niej pliku testowego; dwa klucze B zostały potem usunięte z komputera; klucz deweloperski z pustym hasłem jest w repozytorium w `launcher/dev-keys/`.

Nic ponad to nie zostało rozstrzygnięte w tych decyzjach. Reszta tej sekcji to analiza albo sprawdzone fakty z kodu.

### Co z punktów otwartych z sekcji 2 i 5 jest zamknięte (analiza, sprawdzone w kodzie)

| Punkt otwarty z 2026-10-06 | Stan po 2026-10-07 | Gdzie to jest |
|---|---|---|
| Nazwa drugiego repozytorium | `Shironex/night-maze-launcher`. `gh repo view` (2026-10-07) pokazuje: publiczne, niepuste, gałąź `main`. Repozytorium gry, `Shironex/night-maze`, jest nadal prywatne | [`launcher-installers-repository.md`](launcher-installers-repository.md) |
| Klucze | cztery klucze istnieją (sesja z punktu 6). W kodzie są cztery klucze publiczne: A aktualizacji w `tauri.conf.json`, B aktualizacji jako stała `ROTATION_KEY` w `updater.rs`, A i B manifestu w `RELEASE_KEYS` w `signature.rs`. Kluczy prywatnych w repozytorium nie ma poza deweloperskim | [`launcher-keys-and-rotation.md`](launcher-keys-and-rotation.md) |
| Podpis manifestu | zrobiony. Launcher czyta `manifest.json.sig` i `news.json.sig`, sprawdza je przed parsowaniem (`fetch_signed_once` w `launcher.rs` woła `signature::verify`) i odrzuca plik bez poprawnego podpisu. Podpisują skrypty `sign-file.mjs` i `build-feed.mjs` | `launcher/crates/core/src/signature.rs`, `launcher/crates/core/src/launcher.rs`, `launcher/scripts/` |
| Samoaktualizacja | zrobiona. `tauri-plugin-updater` czyta `latest.json` z repozytorium instalatorów, klucz A jest w konfiguracji, klucz B jako stała rotacyjna, `requireSignedVersion` jest włączone, w oknie jest przycisk "Update launcher" i wiersz w ustawieniach | `launcher/src-tauri/src/updater.rs`, `launcher/src-tauri/tauri.conf.json` |
| Proces wydania | zapisany w README launchera (sekcja "Releasing"). Workflow działa tylko ręcznie i zostawia szkic, który właściciel podpisuje u siebie | [`launcher-first-release-windows-and-release-flow.md`](launcher-first-release-windows-and-release-flow.md) |
| Licencja | plik `LICENSE` w korzeniu (MIT dla kodu, wyłączone assety). Nazwa właściciela praw w pliku czeka na jego potwierdzenie | [`licence-mit-code-assets-excluded.md`](licence-mit-code-assets-excluded.md) |
| Numer pierwszej wersji launchera | `0.1.0` w `launcher/Cargo.toml` (jedyne źródło wersji launchera) | `launcher/Cargo.toml` |

Pozostałe zmiany w launcherze z tego dnia: noty licencyjne launchera (`launcher/THIRD-PARTY-NOTICES.txt`, `scripts/build-launcher-notices.mjs`, instalowane obok launchera), przykład `verify_manifest` sprawdzający manifest tym samym kodem co launcher i tylko kluczami wydania, skrypt `build-latest.mjs` pisący `latest.json`.

### Przyjęte założenia, do potwierdzenia przez właściciela (analiza: właściciel o nich nie decydował)

Zaproponował je zlecający prace, a nie właściciel. Są w kodzie i w README, ale to nie są decyzje właściciela:

- manifest jest podpisywany na komputerze właściciela, a jego klucz nigdy nie trafia na GitHuba,
- nie ma sekretów GitHuba dla pierwszych wydań (workflow używa tylko `github.token`),
- pierwsze wydania są budowane lokalnie i wysyłane poleceniem `gh release create`,
- niepodpisane (bez podpisu kodu) binaria Windows są akceptowane dla znajomych,
- `requireSignedVersion` jest włączone,
- workflow wydania launchera mógłby później mieć swoje miejsce w drugim repozytorium (dziś instalator launchera nie jest budowany w workflow w ogóle: komentarz na końcu `release.yml`).

### Co zostało sprawdzone i czego nie (2026-10-07)

Szczegóły i rozróżnienie rodzajów dowodów: [`../guides/launcher.md`](../guides/launcher.md), sekcja 2. Zgłoszone przez zlecającego, nie powtarzałem: testy Rust i okna, build wydaniowy launchera z kluczem deweloperskim, suchy przebieg wydania gry w osobnym worktree z kluczem deweloperskim. **Nigdy nie uruchomione:** prawdziwa aktualizacja launchera z wersji do wersji, rotacja kluczy, podpis prawdziwym kluczem wydania sprawdzony przez launcher (pierwsze prawdziwe wydanie jest tym sprawdzeniem), workflow wydania na runnerze, cokolwiek na macOS, zachowanie SmartScreen na czystym komputerze.

## 2b. Uzupełnienie z wieczoru 2026-10-07: pierwsze wydania

Decyzje właściciela projektu (2026-10-07, wieczór), cytowane: o wydaniu gry **"Release now"**; o dokumentach kursu w `docs/` **"Yes, publish everything"**. Nic ponad to nie jest tu decyzją. Reszta tej sekcji to opis stanu i analiza.

Co zrobił właściciel (zgłoszone przez zlecającego, nie powtarzałem; to, co sprawdziłem poleceniami tylko do odczytu, jest oznaczone):

- zbudował lokalnie na Windowsie launcher 0.1.0 i 0.1.1, podpisał kluczem aktualizacji A i opublikował jako zwykłe wydania w `Shironex/night-maze-launcher`. Sprawdziłem `gh release list`: oba wydania są, 0.1.1 jako najnowsze, każde z instalatorem i `latest.json`. Sprawdziłem też `latest.json` spod prawdziwego adresu z konfiguracji: wersja 0.1.1, a komentarz podpisu niesie `version:0.1.1`, zgodnie z `requireSignedVersion` (to, że wtyczka przyjęła podpisy, jest zgłoszone),
- zainstalował 0.1.0 i zaktualizował go do 0.1.1 przyciskiem w ustawieniach; launcher uruchomił się jako 0.1.1 i napisał, że jest aktualny. Prawdziwa droga samoaktualizacji jest więc dowiedziona **jeden raz, na jednym komputerze z Windows 11**,
- upublicznił repozytorium gry (sprawdziłem `gh repo view`: `PUBLIC`), wypchnął `main` i opisany tag `v0.10.0` na commit `7b5e0b1` (sprawdziłem `git`),
- zbudował grę 0.10.0 lokalnie w osobnym worktree ze statycznym środowiskiem uruchomieniowym, spakował ją i podpisał `manifest.json` i `news.json` kluczem manifestu A. Przykład `verify_manifest` napisał: "verified: manifest of version 0.10.0 is signed by the first (manifest-a) release key". To pierwszy raz, gdy podpis prawdziwego klucza wydania sprawdził kod launchera. Wydanie `v0.10.0` ma pięć plików: zip, manifest, news i dwa podpisy (sprawdziłem `gh release view`; adres manifestu `releases/latest/download/manifest.json` zwraca wersję 0.10.0). Zainstalowany launcher 0.1.1 pokazał notatki 0.10.0 i przycisk instalacji, zainstalował grę i ją uruchomił (zgłoszone).

Czego to nie zmienia (analiza): workflow `release.yml` **nadal nigdy nie działał** (pierwsze wydania były lokalne, jak założono; `gh run list --workflow release.yml` nie pokazuje przebiegów) i jest dziś tylko ręczny. **Nadal nie uruchomiono:** rotacji kluczy z kluczem B, aktualizacji na czystym drugim komputerze, zachowania SmartScreena na komputerze, który nigdy nie widział launchera, ani niczego na macOS. Nie wiem, czy właściciel od wydania posłuchał dźwięków z 0.10.0 (wydał bez słuchania), więc nie zapisuję ich jako zatwierdzonych.

Otwarte sprawy po wydaniach: dwa alerty Dependabota w `launcher/` (`source-map-js`, wysoki, `pnpm-lock.yaml`; `glib`, średni, `Cargo.lock`), których automatyczne aktualizacje zakończyły się błędem; nagłówek okna launchera mówi "Windows · macOS", a wydawany jest tylko Windows (właściciel: zostawić na razie); angielskie README obu repozytoriów planuje właściciel razem ze swoim zestawem do prezentacji, w tym w repozytorium launchera wyjaśnienie, dlaczego jest osobne, i plan przeniesienia tam później źródeł launchera.

## 3. Rozważane możliwości

To jest analiza. Pierwszy wiersz każdej grupy jest decyzją właściciela.

| Pytanie | Wybrane | Inna możliwość | Uwaga |
|---|---|---|---|
| Technika | Tauri 2 (reguły w Rust, okno w React) | Electron, program w C++ | Tauri używa systemowego widoku sieciowego, więc instalator jest mały; wymaga Rusta i Node do budowy, których reszta projektu nie wymaga |
| Gdzie kod | `launcher/` w repozytorium gry | osobne repozytorium | jedna historia, wspólne skrypty pakujące i plik `THIRD-PARTY-NOTICES.txt`; nie dzieli buildu CMake z grą |
| Skąd wydania gry | repozytorium gry, gdy będzie publiczne | repozytorium launchera | adres `releases/latest/download/manifest.json` wymaga, by ostatnim wydaniem repozytorium gry było wydanie gry |
| Gdzie instalatory launchera | drugie, małe, publiczne repozytorium | to samo repozytorium | wydania launchera nie mogą stać się "latest" w repozytorium gry, bo adres manifestu przestałby się rozwiązywać (README launchera, punkt o aktualizacji); drugie repozytorium usuwa ten konflikt |
| Samoaktualizacja i podpis | po decyzji i z właścicielem, przed pierwszym wydaniem | od razu | klucz podpisu nie może zostać wyłączony później: gdyby zginął, zainstalowane launchery nigdy się nie zaktualizują (README launchera podaje kolejność kroków) |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Powodów właściciela poza samą decyzją nie podano, więc ich nie zapisuję.

**Uwaga z wieczoru 2026-10-07: poniższy opis stanu i skutki dotyczą stanu z rana, bez zmian jako historia; aktualny stan jest w sekcji 2b.**

**Stan faktyczny z 2026-10-06 (sprawdzony w plikach; historia: samoaktualizacja, podpis manifestu i nazwa drugiego repozytorium są od 2026-10-07 opisane w sekcji 2a, a reszta zdań poniżej nadal się zgadza):**

- **Czego nie ma i co nie było uruchomione**, wprost: nic nie jest opublikowane (brak tagu, brak wydania, repozytorium prywatne); workflow `release.yml` **nigdy nie działał** (został sparsowany jako YAML, a jego skrypty uruchomiono lokalnie na Windowsie; zadanie macOS, warunki `if:` i krok publikacji są nietestowane); **macOS jest w całości nieprzetestowany** (launcher nie był na nim budowany ani uruchamiany); launcher **nie aktualizuje się sam**; manifest **nie jest podpisany** (suma SHA-256 wykrywa uszkodzone pobranie, ale nie chroni przed kimś, kto może zmienić wydanie); **brak podpisu kodu**, więc SmartScreen ostrzeże, a zachowania programów antywirusowych na czystym komputerze nikt nie sprawdzał.
- **Co testowano:** testy jednostkowe reguł i przepływów względem lokalnego serwera na pętli zwrotnej (Rust, `cargo test`), testy widoku (`pnpm test`), oraz okno uruchomione na Windows 11 przeciw lokalnemu serwerowi testowemu ([`launcher/README.md`](../../launcher/README.md), "Status"). Co dokładnie obejrzano na zrzutach ekranu i czego nie sprawdzono: [`../guides/launcher.md`](../guides/launcher.md).
- **Format wydania:** jedno archiwum zip na system, `manifest.json` i `news.json`. Launcher akceptuje tylko https (wyjątek: adres pętli zwrotnej do testów), sprawdza rozmiar i SHA-256 z manifestu, rozpakowuje do katalogu wersji i wraca do poprzedniej wersji, jeśli nowa nie uruchomi się w ciągu 15 sekund (zasada wycofania opisana w README launchera).

**Skutki** (analiza):

- **Do pierwszego wydania launcher pokazuje "offline"** względem prawdziwego adresu, bo wydania nie ma, a repozytorium jest prywatne.
- **Klucz aktualizacji tworzy właściciel**, na swoim komputerze, z hasłem, z dwiema kopiami w dwóch miejscach, tego samego dnia. Klucz prywatny nigdy nie trafia do repozytorium. Ten krok nie został wykonany.
- **Wydania gry zależą od końca M9**: dopóki gra się zmienia, pierwsze wydanie by się zestarzało.
- **Poza kursem.** Launcher nie jest częścią ocenianego kodu C++ i nie ma dokumentu modułu w dziesięciu sekcjach. Jedyny dokument to [`../guides/launcher.md`](../guides/launcher.md).

**Czego ta notatka nie przesądza.** Daty pierwszego wydania, numeru pierwszej wersji, nazwy drugiego repozytorium ani wyglądu okna.

## 5. Kiedy wrócić do tej decyzji

Stan z 2026-10-07. Dawne punkty "ustawić podpisywanie i samoaktualizację" i "założyć drugie repozytorium" są zrobione (sekcja 2a). Wieczorem tego dnia zrobiono też pierwsze dwa z poniższych punktów (pierwsze prawdziwe wydanie z podpisem kluczem manifestu A oraz wydanie launchera 0.1.0 i 0.1.1 z aktualizacją na jednym komputerze) i upubliczniono repozytorium gry (sekcja 2b). Zostają:

- Przy **pierwszym prawdziwym wydaniu**: sprawdzić, że launcher przyjmuje podpis prawdziwego klucza manifestu (`verify_manifest` przed publikacją, potem launcher z prawdziwego instalatora). To jest pierwsza próba tej drogi.
- Po **pierwszym wydaniu launchera**: wykonać kroki z README ("Before the first friend gets a link"), w tym aktualizację z 0.1.0 do 0.1.1 na zainstalowanej kopii. Dopóki to się nie stało, żaden zainstalowany launcher nie zaktualizował się naprawdę.
- Gdy **launcher zostanie zbudowany i uruchomiony na macOS**, a gra tam zbudowana i uruchomiona: zapisać wynik, dopiero wtedy dodać wpis macOS do `latest.json` i zadanie macOS do wydania. Dziś wszystko dla macOS jest napisane tylko z dokumentacji.
- Gdy **repozytorium gry stanie się publiczne**: sprawdzić adres manifestu, potwierdzić licencję i nazwę właściciela praw ([`licence-mit-code-assets-excluded.md`](licence-mit-code-assets-excluded.md)).
- Gdy któryś klucz zostanie **utracony albo ujawniony**, albo gdy trzeba go zmienić: [`launcher-keys-and-rotation.md`](launcher-keys-and-rotation.md).
- Gdy znajomi zaczną skarżyć się na ostrzeżenie SmartScreen: wrócić do przyjętego założenia o braku podpisu kodu.
