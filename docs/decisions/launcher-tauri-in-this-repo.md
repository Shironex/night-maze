# Launcher w Tauri 2 w katalogu `launcher/` tego repozytorium, tylko dla Night Maze

Data: 2026-10-06. Stan: obowiązuje jako decyzja, **kod launchera jest** (commity od `c5b3cbd` do `0c87e4d`), ale **nic nie zostało opublikowane**, workflow wydania **nigdy nie działał**, a macOS jest **nieprzetestowany**. Decyzje właściciela projektu są w sekcji 2; opis stanu, tabela możliwości i skutki to moja analiza. Launcher leży poza ocenianym kodem C++ kursu ([`../guides/launcher.md`](../guides/launcher.md)).
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

**Stan faktyczny (sprawdzony w plikach):**

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

- Gdy skończy się M9 i zacznie proces wydania: ustawić podpisywanie i samoaktualizację z właścicielem, założyć drugie repozytorium, uruchomić workflow po raz pierwszy.
- Gdy launcher zostanie zbudowany i uruchomiony na macOS: zapisać wynik, bo dziś wszystko dla macOS jest napisane tylko z dokumentacji.
- Gdy repozytorium gry stanie się publiczne: sprawdzić adres manifestu.
