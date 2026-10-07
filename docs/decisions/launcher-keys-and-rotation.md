# Klucze launchera: który co podpisuje, kto je trzyma i co znaczy utrata każdego

Data: 2026-10-07. Stan: obowiązuje, z kodem. Sesję kluczy wykonał i opisał właściciel projektu (sekcja 2); projekt rotacji z kluczem B jest w kodzie, ale nie ma go na liście decyzji właściciela, więc traktuję go jako wybór wykonawczy do potwierdzenia. Kontekst, tabela, skutki i warunki powrotu to moja analiza. Rozstrzyga punkt otwarty z [`launcher-tauri-in-this-repo.md`](launcher-tauri-in-this-repo.md) ("samoaktualizacja i podpis manifestu przed pierwszym wydaniem").
Kod: [`launcher/crates/core/src/signature.rs`](../../launcher/crates/core/src/signature.rs) (`RELEASE_KEYS`, `trusted_keys`, `verify`), [`launcher/src-tauri/src/updater.rs`](../../launcher/src-tauri/src/updater.rs) (`ROTATION_KEY`, `fetch`), [`launcher/src-tauri/tauri.conf.json`](../../launcher/src-tauri/tauri.conf.json) (`plugins.updater.pubkey`, `requireSignedVersion`), [`launcher/dev-keys/`](../../launcher/dev-keys/), skrypty `sign-file.mjs`, `build-feed.mjs`, `build-latest.mjs` w [`launcher/scripts/`](../../launcher/scripts/), przykład [`verify_manifest.rs`](../../launcher/crates/core/examples/verify_manifest.rs). Opis kroków: [`launcher/README.md`](../../launcher/README.md), sekcja "Keys" i "Releasing".

## 1. Kontekst

Launcher pobiera i uruchamia program, więc ma dwie rzeczy do udowodnienia. Po pierwsze, że manifest (i `news.json`) wydania gry pochodzi od właściciela: suma SHA-256 w manifeście dowodzi tylko, że zip jest tym, który manifest nazywa, a manifest leży w tym samym miejscu co zip, więc kto potrafi zmienić wydanie, zmieni oba (komentarz na początku `signature.rs`). Po drugie, że instalator nowego launchera pochodzi od właściciela. Obie rzeczy sprawdza podpis minisign w formacie Tauri CLI, ale innym kluczem i innym kodem.

Klucz publiczny jest wkompilowany w zainstalowany launcher i da się go zmienić tylko aktualizacją, którą ta kopia przyjmie. Kto traci wszystkie klucze danej pary, traci możliwość aktualizowania zainstalowanych kopii. Z tego wynika cały projekt poniżej.

## 2. Decyzja

Sesja kluczy, wykonana przez właściciela projektu 2026-10-07 (to opis jego pracy, nie wybór z listy opcji):

1. Cztery klucze narzędziem Tauri CLI: **aktualizacji A i B** oraz **manifestu A i B**.
2. Przechowywane **poza repozytorium**, z kopiami w **menedżerze haseł i na drugim dysku**. **Każda kopia została sprawdzona przez podpisanie z niej pliku testowego.**
3. **Dwa klucze B zostały potem usunięte z komputera.**
4. Klucz **deweloperski z pustym hasłem** jest w repozytorium w `launcher/dev-keys/`.

Decyzja właściciela tego samego dnia (z listy opcji, której nie mam): **klucze wygenerowane i wyrzucone w trakcie przebiegu testu są danymi testowymi i są dozwolone. Prawdziwe klucze i klucz deweloperski, który jest w repozytorium, tworzy wyłącznie właściciel.**

Nic ponad to nie zostało rozstrzygnięte w tych decyzjach. Reszta tej notatki to analiza albo fakty sprawdzone w kodzie.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz opisuje to, co jest w kodzie; powodów właściciela dla rozdzielenia kluczy ani dla zimnej rezerwy nie mam.

Co każdy klucz robi (sprawdzone w kodzie i README):

| Klucz | Połowa publiczna | Co podpisuje | Kto go ma |
|---|---|---|---|
| aktualizacji A | `plugins.updater.pubkey` w `tauri.conf.json` | każdy instalator launchera (`pnpm tauri build`) | właściciel, używany przy wydaniu launchera |
| aktualizacji B | stała `ROTATION_KEY` w `updater.rs` | nic, rezerwa | właściciel; usunięty z komputera, kopie poza nim |
| manifestu A | `RELEASE_KEYS[0]` w `signature.rs` | `manifest.json` i `news.json` każdego wydania gry | właściciel, używany przy wydaniu gry |
| manifestu B | `RELEASE_KEYS[1]` w `signature.rs` | nic, rezerwa | właściciel; usunięty z komputera, kopie poza nim |
| deweloperski | `dev-keys/dev.key.pub`, wczytywany stałą `DEV_KEY` | lokalne podajniki testowe | każdy: hasło jest puste, klucz prywatny jest w repozytorium |

Klucz deweloperski jest w launcherze **tylko w buildzie deweloperskim**: `DEV_KEY` ma `#[cfg(debug_assertions)]`, więc w buildzie wydaniowym nie ma wartości do porównania ani gałęzi, którą dałoby się wziąć przez pomyłkę. Przykład `verify_manifest` sprawdza manifest tylko kluczami wydania, więc manifest podpisany deweloperskim kluczem nie przejdzie tej bramki przed publikacją.

Inne projekty (analiza, żadnego z nich nie wybrano):

| Możliwość | Uwaga |
|---|---|
| **Dwie pary z zimną rezerwą, jak w kodzie** | utrata jednego klucza pary nie zatrzymuje aktualizacji. Koszt: cztery klucze do utrzymania i dwa pola do pilnowania przy rotacji |
| Jeden klucz na oba zastosowania | mniej do trzymania, ale jedna utrata zatrzymuje oba mechanizmy naraz. Powodu, dla którego klucze są rozdzielone, właściciel nie podał |
| Jeden klucz na zastosowanie bez rezerwy | `updater.rs` mówi wprost, czym to grozi: bez rezerwy utracony klucz zostawia każdy zainstalowany launcher bez możliwości aktualizacji, na stałe |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela dla układu czterech kluczy nie mam. Powód zimnej rezerwy podaje komentarz w kodzie (`updater.rs`, przy `ROTATION_KEY`), i to jest uzasadnienie autora kodu, a nie właściciela.

**Co znaczy utrata każdego klucza** (analiza, z README i z kodu; żadnego z tych przejść nie uruchomiono):

| Utrata | Skutek | Co zrobić |
|---|---|---|
| manifestu A | nic się nie psuje, bo zainstalowane launchery ufają też B | podpisać następne wydanie gry kluczem manifestu B, potem wydać nowy launcher z `RELEASE_KEYS` zawierającym B i nowy klucz, i przestać używać A. Ujawniony A zostaje akceptowany przez launchery, które się jeszcze nie zaktualizowały |
| manifestu B | niczego nie psuje dziś, ale rezerwy już nie ma (wniosek z kodu: lista kluczy jest stałą) | wydać launcher z nowym drugim kluczem, podpisany kluczem aktualizacji |
| aktualizacji A | zainstalowane launchery nie przejdą sprawdzenia nowego instalatora kluczem z konfiguracji | zbudować następne wydanie launchera kluczem B jako `TAURI_SIGNING_PRIVATE_KEY` (CLI ostrzeże, że nie pasuje do skonfigurowanego, i to jest tym razem oczekiwane). `fetch` w `updater.rs` próbuje wtedy raz klucza B, ale tylko po błędzie podpisu. To wydanie musi nieść nowy `pubkey` i nowy `ROTATION_KEY`, inaczej następna utrata będzie ostateczna |
| aktualizacji B | niczego nie psuje dziś, rezerwy już nie ma | wydać launcher z nowym `ROTATION_KEY`, podpisany kluczem A |
| obu kluczy jednej pary | nic nie da się zrotować | zrobić nowe klucze, wydać nowy instalator, każdy znajomy instaluje go ręcznie |

Ponieważ właściciel usunął klucze B z komputera, każde użycie B zaczyna się od odtworzenia klucza z kopii (menedżer haseł albo drugi dysk). Nie sprawdzałem, czy odtworzenie z żadnej z kopii działa, poza tym, że właściciel opisał test podpisania pliku z każdej kopii w dniu sesji.

**Skutki znane:**

- **Zamrożone w każdym zainstalowanym launcherze:** cztery połowy publiczne, adres aktualizacji ([`launcher-installers-repository.md`](launcher-installers-repository.md)), identyfikator aplikacji i `requireSignedVersion`. Ta ostatnia opcja każe wtyczce odrzucić instalator, którego podpis nie niesie wersji ogłoszonej w `latest.json`, więc fałszywy `latest.json` nie sparuje nowego numeru wersji ze starszym, prawdziwym instalatorem. Jest to przyjęte założenie (włączone w konfiguracji), nie decyzja właściciela.
- **Sprawdzenie podpisu nie da się wyłączyć później.** Launcher, któremu żaden klucz pary nie może podpisać, nigdy się nie zaktualizuje i trzeba mu dać nowy instalator ręcznie.
- **Hasło nie jest argumentem polecenia.** `sign-file.mjs` czyta je ze zmiennej środowiskowej i usuwa z procesu podpisującego zmienne `TAURI_SIGNING_PRIVATE_KEY` i `TAURI_SIGNING_PRIVATE_KEY_PATH`, żeby CLI nie wzięło innego klucza. Utracone hasło to utracony klucz (README).
- **Klucze testowe w testach są dozwolone.** Testy w `signature.rs` generują klucze jednorazowe (`KeyPair::generate_unencrypted_keypair`) i podpisują nimi dane próbne, a jeden test używa pliku podpisanego prawdziwym CLI kluczem deweloperskim (`tests/data/signed-by-dev-key.json`), żeby wykryć rozjazd między biblioteką podpisującą a CLI. To stosuje decyzję właściciela o danych testowych.
- **README launchera w sekcji "Keys" mówi ogólnie, że prywatne połowy leżą w katalogu użytkownika na komputerze właściciela.** Nie rozróżnia kluczy B, które właściciel według punktu 3 usunął z komputera. To zdanie README jest do uzgodnienia z decyzją.
- **Nie zajrzałem do katalogu z prawdziwymi kluczami** i nie sprawdzałem, które pliki w nim są. Wszystko o kluczach prywatnych pochodzi z opisu właściciela i z README.

**Czego ta notatka nie przesądza.** Terminu pierwszej rotacji, tego, czy kiedyś pojawi się trzecia para, ani sposobu przechowywania haseł.

## 5. Kiedy wrócić do tej decyzji

- Przy **pierwszym prawdziwym wydaniu gry**: sprawdzić, że launcher przyjmuje podpis prawdziwego klucza manifestu A. Do tej pory podpis sprawdzały tylko klucze testowe i klucz deweloperski.
- Przy **pierwszym wydaniu launchera** i próbie 0.1.0 do 0.1.1: pierwszy raz zadziała (albo nie) cała droga aktualizacji z kluczem A.
- Przed **jakąkolwiek rotacją**: wykonać kolejność z README (nowa para, kopia w dwóch miejscach, nowe połowy publiczne w konfiguracji lub stałych, wydanie podpisane kluczem, któremu zainstalowane kopie jeszcze ufają, sprawdzenie na kopii starej wersji, dopiero potem wycofanie starego klucza). Rotacji, ani drugiego podejścia z kluczem B w `fetch`, nikt dotąd nie uruchomił.
- Gdy **ujawni się** któryś klucz albo hasło zginie.
- Gdy zacznie się wydawać launcher na **macOS**: kluczy aktualizacji dotyczy to samo, ale `build-latest.mjs` nie pisze dziś wpisu macOS.
