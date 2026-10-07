# Launcher

Launcher to mały program na pulpit, który instaluje najnowszą wersję Night Maze, pilnuje aktualizacji i uruchamia grę. Znajomy pobiera go raz i od tej pory zawsze gra w najnowszą wersję. Leży w katalogu [`launcher/`](../../launcher/) tego repozytorium, ale **nie jest częścią ocenianego kodu C++ kursu**: to osobny program w Rust (reguły instalacji) i TypeScript z React (okno), zbudowany w Tauri 2, który nie dzieli buildu CMake z grą. Dlatego nie ma dokumentu modułu w dziesięciu sekcjach i nie ma wiersza w [`../syllabus.md`](../syllabus.md).

**Szczegółowy opis jest po angielsku w [`launcher/README.md`](../../launcher/README.md)** i to tam trzeba zaglądać po dokładne polecenia: uruchamianie w trybie deweloperskim, test z lokalnym serwerem, sprawdzenia, format `manifest.json` i `news.json`, układ plików na komputerze znajomego, zasadę wycofania do poprzedniej wersji, wydanie launchera, wydanie gry, klucze i listę rzeczy jeszcze niezrobionych. Ten dokument nie kopiuje poleceń.

Decyzje właściciela o launcherze są w [`../decisions/launcher-tauri-in-this-repo.md`](../decisions/launcher-tauri-in-this-repo.md) (technika, miejsce kodu, skąd wydania, uzupełnienie z 2026-10-07) i w czterech notatkach z tego dnia: [`../decisions/launcher-installers-repository.md`](../decisions/launcher-installers-repository.md), [`../decisions/launcher-keys-and-rotation.md`](../decisions/launcher-keys-and-rotation.md), [`../decisions/licence-mit-code-assets-excluded.md`](../decisions/licence-mit-code-assets-excluded.md) i [`../decisions/launcher-first-release-windows-and-release-flow.md`](../decisions/launcher-first-release-windows-and-release-flow.md).

Stan na wieczór 2026-10-07: kod jest i **pierwsze wydania są opublikowane**. Właściciel zbudował lokalnie na Windowsie i opublikował launcher 0.1.0 i 0.1.1 (publiczne repozytorium `Shironex/night-maze-launcher`), repozytorium gry `Shironex/night-maze` jest od tego wieczoru publiczne, a wydanie gry 0.10.0 (tag `v0.10.0`) podpisał kluczem manifestu A. Prawdziwa samoaktualizacja launchera została uruchomiona **raz, na jednym komputerze z Windows 11**. Dalej nie uruchomiono rotacji kluczy, aktualizacji na czystym drugim komputerze, SmartScreena na komputerze, który nigdy nie widział launchera, workflow wydania ani niczego na macOS (sekcja 2). Rano tego samego dnia stan był inny (nic nie było opublikowane); notatki o decyzjach mają dopisane daty.

## 1. Co launcher robi

Według [`launcher/README.md`](../../launcher/README.md) (sekcja "What it does"):

- czyta `manifest.json` z najnowszego wydania repozytorium gry razem z `manifest.json.sig` obok niego. Manifest (i `news.json`) bez poprawnego podpisu jednego z dwóch kluczy wydania jest odrzucany ([`launcher/crates/core/src/signature.rs`](../../launcher/crates/core/src/signature.rs), `RELEASE_KEYS`),
- pobiera archiwum zip dla tego systemu, sprawdza jego rozmiar i sumę SHA-256 z manifestu, rozpakowuje obok już zainstalowanych wersji i robi z niej bieżącą,
- uruchamia grę z katalogiem `data/` jako katalogiem roboczym i zapisuje jej wyjście konsoli do `data/logs/last-run.log`,
- sam wraca o jedną wersję, gdy nowa nie uruchomi się (reguła wycofania z README),
- uruchamia zainstalowaną wersję, gdy nie ma sieci,
- **aktualizuje samego siebie**: czyta `latest.json` z najnowszego wydania repozytorium `Shironex/night-maze-launcher` (przez `tauri-plugin-updater`, [`launcher/src-tauri/src/updater.rs`](../../launcher/src-tauri/src/updater.rs)) przy starcie i z ustawień, a gdy jest nowszy launcher, okno pokazuje przycisk "Update launcher". Instalator jest pobierany, jego podpis sprawdzany kluczem aktualizacji i dopiero wtedy zastępuje launcher. Build deweloperski nigdy tego nie robi (`cfg!(debug_assertions)` w `updater.rs`).

Podpis manifestu i podpis aktualizacji launchera to dwie osobne rzeczy z osobnymi kluczami. Co który klucz robi: [`../decisions/launcher-keys-and-rotation.md`](../decisions/launcher-keys-and-rotation.md).

Cały dostęp do sieci jest w Rust. Okno nie wysyła żadnych zapytań.

Wersja launchera ma jedno źródło: `[workspace.package]` w `launcher/Cargo.toml`. W `tauri.conf.json` nie ma pola `version`, a test w `updater.rs` (`the_version_has_one_source`) tego pilnuje. Wersja gry ma inne jedyne źródło, linię `project(NightMaze VERSION ...)` w `CMakeLists.txt` ([`../decisions/versions-m9-and-the-sound-slice.md`](../decisions/versions-m9-and-the-sound-slice.md)).

## 2. Co zostało sprawdzone, a co nie

Trzy rodzaje dowodów trzymam osobno, tak jak w [`build-windows.md`](build-windows.md). Wszystko, co niżej jest "zgłoszone", dostałem od zlecającego tę dokumentację; tego nie powtarzałem ani nie uruchamiałem.

**Zgłoszone z repozytorium i od zlecającego (2026-10-07, Windows, nie powtarzałem):**

- zestawy testów Rust (`cargo test --workspace`) i okna (`pnpm test`); lista poleceń jest w README launchera, sekcja "Checks",
- **build wydaniowy launchera z kluczem deweloperskim** (nigdy do publikacji). README launchera w sekcji "Checks" zapisuje już ten jeden build próbny. W katalogu `launcher/target/release/bundle/nsis` leży instalator `0.1.0` z tego dnia (zobaczyłem plik, nie uruchamiałem go),
- **pełny suchy przebieg wydania gry** w osobnym worktree z kluczem deweloperskim: statyczne środowisko uruchomieniowe i brak zależności od bibliotek DLL Visual C++, zip zawiera plik wykonywalny, 86 plików assetów i plik z notami (liczbę 86 plików w `assets/` sprawdziłem poleceniem `find`, reszta jest zgłoszona), rozmiar, suma i nazwa pliku w manifeście zgadzają się z zipem, przykład `verify_manifest` odrzuca podpis kluczem deweloperskim, a gra startuje z rozpakowanego katalogu z dźwiękiem i filmem w menu.

**Uruchomione wieczorem 2026-10-07 przez właściciela (zgłoszone przeze mnie od zlecającego, nie powtarzałem; to, co dało się sprawdzić poleceniami tylko do odczytu, jest oznaczone):**

- **launcher 0.1.0 i 0.1.1 zbudowane lokalnie na Windowsie przez właściciela, podpisane kluczem aktualizacji A i opublikowane jako zwykłe wydania** w publicznym repozytorium `Shironex/night-maze-launcher`. Sprawdziłem `gh release list` i `gh release view`: oba wydania istnieją, nie są szkicem ani wydaniem wstępnym, 0.1.1 jest oznaczone jako najnowsze, każde ma instalator i `latest.json`. Że podpisy niosą wersję i że `requireSignedVersion` je przyjęło, jest zgłoszone,
- **prawdziwa samoaktualizacja, raz.** Właściciel zainstalował 0.1.0 i zaktualizował go do 0.1.1 przyciskiem w ustawieniach; launcher uruchomił się ponownie jako 0.1.1 i napisał, że jest aktualny. To jest jedyny dowód całej drogi (kod aktualizacji, sprawdzenie podpisu z `requireSignedVersion`, pasywny instalator, restart), z jednego komputera z Windows 11,
- **wydanie gry 0.10.0.** Zbudowane lokalnie w osobnym worktree ze statycznym środowiskiem uruchomieniowym, spakowane, a `manifest.json` i `news.json` podpisał właściciel kluczem manifestu A. Przykład `verify_manifest` napisał: "verified: manifest of version 0.10.0 is signed by the first (manifest-a) release key". To **pierwszy raz, gdy podpis prawdziwego klucza wydania sprawdził kod launchera**. Wydanie `v0.10.0` ma pięć plików (zip, manifest, news, dwa podpisy): sprawdziłem `gh release view`. Zainstalowany launcher 0.1.1 pokazał potem notatki 0.10.0 i przycisk instalacji, zainstalował grę i ją uruchomił (zgłoszone),
- **repozytorium gry jest publiczne** od tego wieczoru (sprawdziłem `gh repo view`: `PUBLIC`), `main` jest wypchnięty, a opisany tag `v0.10.0` wskazuje commit `7b5e0b1` (sprawdziłem `git`). Decyzje właściciela, cytat: "Release now" i, o dokumentach kursu w `docs/`, "Yes, publish everything".

**Wcześniej, widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela:** na Windowsie 11, przeciw lokalnemu serwerowi testowemu, agent obejrzał na zrzutach ekranu **pięć przepływów** okna. Które to były przepływy, zgłoszono mi tylko jako liczbę, a w repozytorium takiej listy nie ma, więc ich nie wymieniam. README launchera wylicza stany, które okno umie pokazać (`ready`, `update`, `downloading`, `installing`, `running`, `offline`, `first-run`, `first-run-offline`, `rolled-back`, `update-failed`, `launcher-too-old`, `launcher-update`, `launcher-downloading`), ale nie mówi, że każdy z nich obejrzano. To **nie jest** test właściciela.

**Nigdy nie uruchomione:**

- **aktualizacja launchera na czystym drugim komputerze.** Jedyna prawdziwa aktualizacja (0.1.0 do 0.1.1) poszła na jednym komputerze z Windows 11; nie zapisano, czy był czysty, i nie ma drugiego,
- **rotacja kluczy**, dla żadnej z dwóch par, w tym drugie podejście z kluczem B w `updater.rs` (`fetch`) przeciw prawdziwemu wydaniu,
- **workflow wydania na runnerze GitHuba** (`.github/workflows/release.yml`). Został sparsowany jako YAML, a skrypty uruchomiono lokalnie; pierwsze wydania były lokalne. Jest dziś tylko ręczny. Sprawdziłem `gh run list --workflow release.yml`: brak przebiegów. Zadanie macOS, warunki `if:` i krok publikacji nie były wykonane,
- **cokolwiek na macOS**: budowanie launchera i gry, uruchamianie, aktualizacja launchera, podpis ad hoc pakietu, bit wykonywalny rozpakowanej gry, półprzezroczysty pasek tytułu (`src-tauri/tauri.macos.conf.json`). Jest to napisane tylko z dokumentacji,
- **SmartScreen i programy antywirusowe na komputerze, który nigdy nie widział launchera.** Launcher i jego instalator nie mają podpisu kodu, więc Windows ostrzeże przy pierwszym starcie. Jak zachowa się SmartScreen i czy antywirus sprzeciwi się programowi, który pobiera i uruchamia inny program, nikt nie sprawdził (na komputerze właściciela, gdzie launcher był budowany i instalowany, nie zapisano ostrzeżenia).

**Otwarte sprawy po wydaniach (stan z 2026-10-07):**

- dwa alerty Dependabota w `launcher/`: `source-map-js` (wysoki, w `pnpm-lock.yaml`) i `glib` (średni, w `Cargo.lock`); sprawdziłem `gh api` (alerty otwarte) i `gh run list` (automatyczne aktualizacje obu zakończyły się błędem),
- nagłówek okna launchera mówi "Windows · macOS" (`launcher/src/App.tsx`), a wydawany jest tylko Windows; decyzja właściciela: zostawić na razie,
- właściciel planuje angielskie README obu repozytoriów razem ze swoim zestawem do prezentacji, w tym w repozytorium launchera wyjaśnienie, dlaczego jest osobne, i plan przeniesienia tam później źródeł launchera,
- nie wiem, czy właściciel od wydania posłuchał dźwięków z 0.10.0: wydał bez słuchania, więc nie zapisuję ich jako zatwierdzonych.

## 3. Uruchomienie lokalne

Krótko, szczegóły i polecenia w README launchera (sekcja "Run it in development"): potrzebne są Rust (stable), Node 22 i pnpm 10, a na Windowsie środowisko WebView2 (jest w Windows 11).

Uruchomiony w trybie deweloperskim launcher czyta prawdziwy adres wydań i instaluje do prawdziwego katalogu użytkownika. Do prób służą zmienne `NIGHT_MAZE_LAUNCHER_FEED` i `NIGHT_MAZE_LAUNCHER_ROOT` oraz skrypty `launcher/scripts/package-game.mjs` i `serve.mjs` (przykład w README). Lokalny podajnik podpisuje oba pliki json **kluczem deweloperskim** z [`launcher/dev-keys/`](../../launcher/dev-keys/): jego klucz prywatny jest w repozytorium z pustym hasłem celowo, więc niczego nie dowodzi, i dlatego tylko build deweloperski mu ufa (stała `DEV_KEY` istnieje w `signature.rs` wyłącznie pod `#[cfg(debug_assertions)]`).

## 4. Wydania

Dokładne kroki, w kolejności, są w README launchera (sekcja "Releasing"), więc tutaj tylko mapa:

- **dwa repozytoria, dwa wydania**: instalator launchera idzie do `Shironex/night-maze-launcher`, gra i jej manifest do `Shironex/night-maze` ([`../decisions/launcher-installers-repository.md`](../decisions/launcher-installers-repository.md)),
- **launcher** buduje się na komputerze właściciela poleceniem `pnpm tauri build`, które potrzebuje klucza aktualizacji, a `scripts/build-latest.mjs` pisze `latest.json`,
- **gra** ma dwie drogi: lokalną (`package-game.mjs`, `build-feed.mjs` z podpisem, sprawdzenie przykładem `verify_manifest`, `gh release create`) i przez workflow, który działa tylko ręcznie i zostawia **szkic** wydania bez podpisów; właściciel podpisuje pliki json u siebie (`scripts/sign-file.mjs`) i dopiero potem publikuje szkic ([`../decisions/launcher-first-release-windows-and-release-flow.md`](../decisions/launcher-first-release-windows-and-release-flow.md)),
- **pierwsze wydanie dla znajomych jest tylko na Windowsa**,
- przed pierwszym linkiem dla znajomego README podaje listę ośmiu kroków, w tym próbę prawdziwej aktualizacji launchera z 0.1.0 do 0.1.1. Lista zostaje jako procedura na kolejne wydania; dla 0.1.0 i 0.1.1 wykonał ją właściciel wieczorem 2026-10-07 (bez czystego komputera i bez SmartScreena na komputerze, który launchera nie widział). Do pierwszej wersji gry pierwszy krok to było wydanie 0.10.0, tak jak opisano w sekcji 2.

## 5. Powiązane

- Lista bibliotek w pliku `THIRD-PARTY-NOTICES.txt` w korzeniu repozytorium (noty gry) jest generowana skryptem `launcher/scripts/build-notices.mjs` i ma jedenaście sekcji (policzone w skrypcie): GLFW, GLM, Dear ImGui, stb_image, miniaudio, RmlUi, kontenery RmlUi, FreeType, kopia zlib z FreeType, GLAD i czcionka Atkinson Hyperlegible ([`../libraries/rmlui.md`](../libraries/rmlui.md)). Generuje się ją ponownie, gdy zmieni się wersja w `cmake/Dependencies.cmake`.
- Launcher ma własny plik not, [`launcher/THIRD-PARTY-NOTICES.txt`](../../launcher/THIRD-PARTY-NOTICES.txt): skrzynki Rust i pakiety npm w oknie oraz trzy czcionki, generowany skryptem `launcher/scripts/build-launcher-notices.mjs` (`pnpm notices`) i instalowany obok launchera (`bundle.resources` w `tauri.conf.json`).
- Licencja repozytorium: plik [`LICENSE`](../../LICENSE) w korzeniu (MIT dla kodu, assety wyłączone), [`../decisions/licence-mit-code-assets-excluded.md`](../decisions/licence-mit-code-assets-excluded.md).
- Plan wydań czeka na koniec M9 w pierwotnej decyzji; stan po 2026-10-07 opisuje uzupełnienie w [`../decisions/launcher-tauri-in-this-repo.md`](../decisions/launcher-tauri-in-this-repo.md).
