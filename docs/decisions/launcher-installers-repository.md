# Instalatory launchera w repozytorium `Shironex/night-maze-launcher`, a ten adres jest wkompilowany w każdy zainstalowany launcher

Data: 2026-10-07, uzupełniona wieczorem tego dnia (sekcja 5a). Stan wieczorem 2026-10-07: obowiązuje, z kodem; repozytorium ma już dwa wydania, 0.1.0 i 0.1.1, a adres aktualizacji odpowiada. Nazwa repozytorium jest decyzją właściciela projektu (sekcja 2). To, że adres wynikający z tej nazwy jest wkompilowany w zainstalowane launchery, jest faktem sprawdzonym w kodzie, a nie uzasadnieniem właściciela. Kontekst, tabela i skutki to moja analiza. Rozstrzyga punkt otwarty z [`launcher-tauri-in-this-repo.md`](launcher-tauri-in-this-repo.md) ("nazwa drugiego repozytorium").
Kod: [`launcher/src-tauri/tauri.conf.json`](../../launcher/src-tauri/tauri.conf.json) (`plugins.updater.endpoints`, `identifier`), [`launcher/src-tauri/src/updater.rs`](../../launcher/src-tauri/src/updater.rs) (test `updates_come_from_the_launcher_repository_over_https`), [`launcher/scripts/build-latest.mjs`](../../launcher/scripts/build-latest.mjs) (parametr `--repo`). Opis kroków: [`launcher/README.md`](../../launcher/README.md), sekcje "Releasing" i "Keys".

## 1. Kontekst

Decyzja z 2026-10-06 mówiła, że instalatory samego launchera trafią do drugiego, małego, publicznego repozytorium, ale nie podawała jego nazwy. Powód istnienia drugiego repozytorium jest w README launchera: wydanie "latest" repozytorium gry musi zawsze być najnowszym wydaniem gry, bo adres `releases/latest/download/manifest.json` przestałby się rozwiązywać, gdyby najnowszym wydaniem było wydanie launchera.

Launcher aktualizuje się przez `tauri-plugin-updater`, który czyta `latest.json` z adresu zapisanego w konfiguracji. Adres jest częścią zbudowanego launchera, więc nazwa repozytorium przestaje być sprawą samego GitHuba.

## 2. Decyzja

Decyzja właściciela projektu (2026-10-07), wybór z listy opcji, której nie mam w materiałach (opcji nie opisuję):

**Drugie publiczne repozytorium na instalatory launchera to `Shironex/night-maze-launcher`**, założone przez właściciela tego dnia.

Powodów wyboru nazwy właściciel nie podał, więc ich nie zapisuję. Sprawdzone `gh repo view Shironex/night-maze-launcher` (2026-10-07): repozytorium istnieje, jest publiczne, nie jest puste, domyślna gałąź to `main`.

Sprawdzone w kodzie (to nie jest część decyzji): adres `https://github.com/Shironex/night-maze-launcher/releases/latest/download/latest.json` stoi w `plugins.updater.endpoints` w `tauri.conf.json`, a test w `updater.rs` porównuje tę listę z dokładnie tym jednym adresem i pilnuje, żeby w konfiguracji nie było `dangerousInsecureTransportProtocol`. Nazwa stoi też w poleceniach z README (`build-latest.mjs --repo`, `gh release create --repo`).

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz jest decyzją właściciela. Innych nazw, które rozważał właściciel, nie znam.

| Możliwość | Uwaga |
|---|---|
| **`Shironex/night-maze-launcher`, publiczne (wybrane)** | adres aktualizacji launchera jest tym, co stoi w konfiguracji. Repozytorium gry może zostać prywatne do czasu upublicznienia, a instalator launchera można pobrać bez konta |
| Instalatory w repozytorium gry | odrzucone już 2026-10-06 (kolumna "Gdzie instalatory launchera" w [`launcher-tauri-in-this-repo.md`](launcher-tauri-in-this-repo.md)): wydanie launchera zająłoby "latest" gry |
| Inna nazwa drugiego repozytorium | nie znam takich opcji z materiałów. Kod nie ma jednego miejsca z nazwą: jest w konfiguracji, w testach i w poleceniach README |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia nazwy od właściciela nie mam. Nie dopisuję własnego.

**Skutki znane** (analiza, sprawdzone w repozytorium):

- **Adres jest zamrożony w każdej zainstalowanej kopii.** README launchera (sekcja "Keys") wylicza, co jest wkompilowane i zmienia się tylko przez aktualizację, którą zainstalowana kopia przyjmie: adres aktualizacji, identyfikator aplikacji `com.shironex.nightmaze.launcher`, klucze publiczne i `requireSignedVersion`.
- **Zmiana nazwy po pierwszym wydaniu jest ryzykowna.** Zainstalowane launchery nadal szukałyby starego adresu. Czy GitHub przekierowałby go po zmianie nazwy i czy wtyczka z `https_only` by to przyjęła, nie sprawdziłem. Nie zakładam, że tak.
- **`latest.json` odczyta się tylko z normalnego, opublikowanego wydania.** README: wydanie nie może być szkicem ani `--prerelease`, bo `releases/latest/download/...` wskazuje najnowsze wydanie, które nie jest wstępne. Tag to `v` plus wersja, bo na to czeka adres instalatora w `latest.json`. Repozytorium musi mieć co najmniej jeden commit, żeby tag mógł powstać (to jest spełnione, `isEmpty` wynosi `false`).
- **Dwa repozytoria, dwa niezależne zegary wydań.** Wersja launchera jest w `launcher/Cargo.toml`, wersja gry w `CMakeLists.txt`. Pierwszy launcher ma `0.1.0`.
- **Czego nie ma:** zadania w workflow, które budowałoby instalator launchera. Komentarz na końcu [`.github/workflows/release.yml`](../../.github/workflows/release.yml) mówi, że instalator wychodzi z drugiego repozytorium i że pierwsze wydania są budowane na komputerze właściciela ([`launcher-first-release-windows-and-release-flow.md`](launcher-first-release-windows-and-release-flow.md)).

**Czego ta notatka nie przesądza.** Czy repozytorium gry stanie się publiczne w tym samym dniu co pierwsze wydanie, ani czy kiedyś w drugim repozytorium zamieszka własny workflow wydania launchera.

## 5a. Uzupełnienie z wieczoru 2026-10-07: repozytorium ma wydania

Opis stanu (zgłoszone przez zlecającego; sprawdziłem `gh release list` i `curl` na adres z konfiguracji): właściciel opublikował w `Shironex/night-maze-launcher` zwykłe wydania `v0.1.0` i `v0.1.1`, każde z instalatorem i `latest.json`, oba nie są szkicem ani wydaniem wstępnym. Adres `https://github.com/Shironex/night-maze-launcher/releases/latest/download/latest.json` zwraca wersję 0.1.1. Instalator 0.1.0 zaktualizował się z niego do 0.1.1 na jednym komputerze z Windows 11 (zgłoszone). Repozytorium gry jest od tego wieczoru też publiczne. Nazwy repozytorium nikt nie zmieniał, więc skutki zmiany nazwy po wydaniu (sekcja 4) pozostają niesprawdzone. Sekcja 4 ("dopóki repozytorium gry jest prywatne") i sekcja 5 zostają jako historia.

## 5. Kiedy wrócić do tej decyzji

- Przed **pierwszym wydaniem launchera**: odczytać `latest.json` spod prawdziwego adresu (README, krok 8 wydania launchera). Do tego czasu adres odpowiada błędem, bo wydania nie ma. (Zrobione wieczorem 2026-10-07, sekcja 5a: adres zwraca 0.1.1.)
- Gdy ktoś chce **zmienić nazwę albo przenieść** repozytorium: najpierw sprawdzić, co robią już zainstalowane launchery, bo adres da się zmienić tylko aktualizacją, którą przyjmą.
- Gdy repozytorium launchera dostanie **własny workflow** albo **drugi system** (macOS): `build-latest.mjs` pisze dziś wpis tylko dla Windowsa.
