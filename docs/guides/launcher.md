# Launcher

Launcher to mały program na pulpit, który instaluje najnowszą wersję Night Maze, pilnuje aktualizacji i uruchamia grę. Znajomy pobiera go raz i od tej pory zawsze gra w najnowszą wersję. Leży w katalogu [`launcher/`](../../launcher/) tego repozytorium, ale **nie jest częścią ocenianego kodu C++ kursu**: to osobny program w Rust (reguły instalacji) i TypeScript z React (okno), zbudowany w Tauri 2, który nie dzieli buildu CMake z grą. Dlatego nie ma dokumentu modułu w dziesięciu sekcjach i nie ma wiersza w [`../syllabus.md`](../syllabus.md).

**Szczegółowy opis jest po angielsku w [`launcher/README.md`](../../launcher/README.md)** i to tam trzeba zaglądać po: uruchamianie w trybie deweloperskim, test z lokalnym serwerem, format `manifest.json` i `news.json`, układ plików na komputerze znajomego, zasadę wycofania do poprzedniej wersji i listę rzeczy jeszcze niezrobionych. Ten dokument tego nie powtarza.

Decyzje właściciela o launcherze (technika, miejsce kodu, skąd wydania, samoaktualizacja, kolejność prac) są w [`../decisions/launcher-tauri-in-this-repo.md`](../decisions/launcher-tauri-in-this-repo.md).

## 1. Co launcher robi

Według [`launcher/README.md`](../../launcher/README.md) (sekcja "What it does"):

- czyta `manifest.json` z najnowszego wydania repozytorium gry,
- pobiera archiwum zip dla tego systemu, sprawdza jego rozmiar i sumę SHA-256 z manifestu, rozpakowuje obok już zainstalowanych wersji i robi z niej bieżącą,
- uruchamia grę z katalogiem `data/` jako katalogiem roboczym i zapisuje jej wyjście konsoli do `data/logs/last-run.log`,
- sam wraca o jedną wersję, gdy nowa nie uruchomi się (reguła wycofania z README),
- uruchamia zainstalowaną wersję, gdy nie ma sieci.

Cały dostęp do sieci jest w Rust. Okno nie wysyła żadnych zapytań.

## 2. Co zostało sprawdzone, a co nie

Trzy rodzaje dowodów trzymam osobno, tak jak w [`build-windows.md`](build-windows.md).

**Zgłoszone z repozytorium (nie powtarzałem):** testy jednostkowe i przepływów względem serwera na pętli zwrotnej z atrapą gry (`cargo test`), testy widoku i skryptów (`pnpm test`, `node --test`). Komunikaty commitów opisują je jako uruchomione; ja ich przy pisaniu tego dokumentu nie uruchamiałem.

**Widziane na zrzucie ekranu przez agenta (2026-10-06), nie przez właściciela:** na Windowsie 11, przeciw lokalnemu serwerowi testowemu, agent obejrzał na zrzutach ekranu **pięć przepływów** okna. Które to były przepływy, zgłoszono mi tylko jako liczbę, a w repozytorium takiej listy nie ma, więc ich nie wymieniam. README launchera wylicza stany, które okno umie pokazać (`ready`, `update`, `downloading`, `installing`, `running`, `offline`, `first-run`, `first-run-offline`, `rolled-back`, `update-failed`, `launcher-too-old`), ale nie mówi, że każdy z nich obejrzano. To **nie jest** test właściciela.

**Nie sprawdzone:**

- **macOS w całości.** Launcher nie był na nim budowany ani uruchamiany. Półprzezroczysty pasek tytułu (`src-tauri/tauri.macos.conf.json`), podpis ad hoc pakietu i bit wykonywalny rozpakowanej gry są napisane tylko z dokumentacji.
- **Workflow wydania** (`.github/workflows/release.yml`): **nigdy nie działał**. Został sparsowany jako YAML, a skrypty uruchomiono lokalnie na Windowsie. Zadanie macOS, warunki `if:` i krok publikacji nie były wykonane.
- **Instalator launchera** (`pnpm tauri build`) i jego instalacja na czystym komputerze.
- **SmartScreen i programy antywirusowe.** Launcher nie ma podpisu kodu, więc Windows ostrzeże przy pierwszym starcie instalatora. Jak zachowa się SmartScreen i czy antywirus sprzeciwi się programowi, który pobiera i uruchamia inny program, nikt nie sprawdził na czystym komputerze.
- **Prawdziwy adres wydań.** Nic nie jest opublikowane: nie ma tagu ani wydania, a repozytorium jest prywatne. Przeciw prawdziwemu adresowi launcher pokazuje "offline".
- **Samoaktualizacja** launchera i podpis manifestu: nie istnieją (kolejność prac w README).

## 3. Uruchomienie lokalne

Krótko, szczegóły w README launchera: potrzebne są Rust (stable), Node 22 i pnpm 10, a na Windowsie środowisko WebView2 (jest w Windows 11).

```sh
cd launcher
pnpm install
pnpm tauri dev
```

Tak uruchomiony czyta prawdziwy adres wydań i instaluje do prawdziwego katalogu użytkownika. Do prób służą zmienne `NIGHT_MAZE_LAUNCHER_FEED` i `NIGHT_MAZE_LAUNCHER_ROOT` oraz skrypty `launcher/scripts/package-game.mjs` i `serve.mjs` (przykład w README).

## 4. Powiązane

- Lista bibliotek w pliku `THIRD-PARTY-NOTICES.txt` w korzeniu repozytorium jest generowana skryptem `launcher/scripts/build-notices.mjs` i po M9, części 2, obejmuje też RmlUi i FreeType ([`../libraries/rmlui.md`](../libraries/rmlui.md)). Generuje się ją ponownie, gdy zmieni się wersja w `cmake/Dependencies.cmake`.
- Plan wydań czeka na koniec M9 ([`../decisions/launcher-tauri-in-this-repo.md`](../decisions/launcher-tauri-in-this-repo.md)).
