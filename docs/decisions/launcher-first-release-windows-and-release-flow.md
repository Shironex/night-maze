# Pierwsze wydanie dla znajomych jest tylko na Windowsa, a wydania powstają lokalnie i przez ręczny workflow ze szkicem

Data: 2026-10-07, uzupełniona wieczorem tego dnia (sekcja 4a). Stan wieczorem 2026-10-07: obowiązuje, z kodem i skryptami; **pierwsze wydania powstały lokalnie** (launcher 0.1.0 i 0.1.1, gra 0.10.0, sekcja 4a), a workflow **nadal nigdy nie działał** i jest tylko ręczny. Rano tego dnia stan był inny: żadne wydanie nie powstało. Z tej notatki tylko jedno zdanie jest decyzją właściciela: pierwsze wydanie jest tylko na Windowsa (sekcja 2). **Cały przepływ wydania w sekcji 4 to przyjęte założenia, do potwierdzenia przez właściciela**, bo właściciela o nie nie pytano. Kontekst, tabela i skutki to moja analiza. Rozstrzyga punkt otwarty z [`launcher-tauri-in-this-repo.md`](launcher-tauri-in-this-repo.md) ("proces wydania").
Kod: [`.github/workflows/release.yml`](../../.github/workflows/release.yml), skrypty w [`launcher/scripts/`](../../launcher/scripts/) (`package-game.mjs`, `build-feed.mjs`, `sign-file.mjs`, `build-latest.mjs`), przykład [`verify_manifest.rs`](../../launcher/crates/core/examples/verify_manifest.rs). Dokładne polecenia: [`launcher/README.md`](../../launcher/README.md), sekcje "Releasing" i "Before the first friend gets a link".

## 1. Kontekst

Znajomi mają dostawać grę przez launcher, a nie jako zip wysyłany ręcznie (decyzja właściciela z 2026-10-07, [`launcher-tauri-in-this-repo.md`](launcher-tauri-in-this-repo.md), sekcja 2a). Do tego potrzebne są dwa rodzaje wydań z dwóch repozytoriów ([`launcher-installers-repository.md`](launcher-installers-repository.md)): launchera i gry. Wersja gry ma jedno źródło, `CMakeLists.txt` ([`versions-m9-and-the-sound-slice.md`](versions-m9-and-the-sound-slice.md)); dziś to `0.10.0`, a tagu `v0.10.0` nie ma.

Do 2026-10-07 workflow wydania startował po wypchnięciu tagu `v*`. Commit `8330497` ("ci(release): run by hand only and publish a draft for the owner to sign") zmienił go na uruchamiany tylko ręcznie.

## 2. Decyzja

Decyzja właściciela projektu (2026-10-07), wybór z listy opcji, której nie mam w materiałach (opcji nie opisuję):

**Pierwsze wydanie dla znajomych jest tylko na Windowsa. macOS dojdzie po tym, jak właściciel zbuduje i uruchomi grę oraz launcher na Macu.**

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Dla pierwszego zdania (tylko Windows) nie znam innych opcji z listy właściciela; wiersze dotyczą przepływu z sekcji 4.

| Możliwość | Uwaga |
|---|---|
| **Windows najpierw, macOS po sprawdzeniu na Macu (wybrane)** | zadanie macOS w workflow istnieje, ale jest wyłączone domyślnie (`if: ${{ inputs.macos }}`); `build-latest.mjs` pisze `latest.json` tylko z wpisem Windows; wszystko dla macOS jest napisane tylko z dokumentacji |
| Oba systemy od razu | wymagałoby uruchomienia gry i launchera na macOS przed pierwszym wydaniem; takiego uruchomienia nie było |
| Wydanie lokalne, bez workflow (przyjęte założenie) | żadnych minut CI i żadnych sekretów; klucz nie opuszcza komputera. Pierwsze wydania są budowane lokalnie i wysyłane przez `gh release create` |
| Workflow budujący i zostawiający szkic (przyjęte założenie, jako druga droga) | build na czystym runnerze; komentarz w `release.yml` zaznacza, że dopóki repozytorium jest prywatne, każdy przebieg to płatne minuty |
| Workflow, który sam podpisuje | wymagałby klucza manifestu na GitHubie jako sekretu. Odrzucone przez założenie "żadnych sekretów GitHuba dla pierwszych wydań" |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela dla pierwszego zdania nie mam. Poniższy przepływ zaproponował zlecający prace, a nie właściciel, i jest w całości **przyjętym założeniem, do potwierdzenia**.

**Przyjęte założenia** (każde sprawdzone w kodzie jako to, co kod robi, ale nie jako decyzja właściciela):

- **Manifest jest podpisywany na komputerze właściciela, a jego klucz nigdy nie trafia na GitHuba.** Workflow nie zna klucza: krok pisania plików json używa `--no-sign`, a `build-feed.mjs` dla adresu github.com odmawia pracy bez `--sign-key` albo `--no-sign`.
- **Żadnych sekretów GitHuba dla pierwszych wydań.** Workflow używa tylko `github.token` do utworzenia szkicu.
- **Pierwsze wydania są budowane lokalnie.** Droga lokalna z README: budowa ze statycznym środowiskiem uruchomieniowym, testy, `package-game.mjs`, `build-feed.mjs` z kluczem manifestu A, sprawdzenie przykładem `verify_manifest`, `gh release create`.
- **Workflow działa tylko ręcznie i kończy na szkicu.** Wejścia: `tag` (musi istnieć i równać się `v` plus wersja z `CMakeLists.txt`, inaczej zatrzymuje się w pierwszej minucie) i `macos` (domyślnie `false`). Wynik: **szkic** wydania z zipem, `manifest.json` i `news.json` bez podpisów. Szkic nigdy nie jest "latest", więc żaden launcher go nie widzi. Właściciel pobiera pliki json, podpisuje je u siebie (`sign-file.mjs`), sprawdza (`verify_manifest`), wysyła dwa pliki `.sig` i dopiero wtedy publikuje szkic poleceniem `gh release edit ... --draft=false --latest`.
- **Niepodpisane (bez podpisu kodu) binaria Windows są akceptowane dla znajomych.** README: SmartScreen ostrzeże przy instalatorze, znajomy ma kliknąć "More info", potem "Run anyway". Jak naprawdę zachowa się SmartScreen na czystym komputerze, nikt nie sprawdził.
- **Instalator launchera nie jest budowany w workflow w ogóle.** Wychodzi z drugiego repozytorium, na razie z komputera właściciela (`pnpm tauri build --bundles nsis`, `build-latest.mjs`, `gh release create`). Workflow wydania launchera mógłby później zamieszkać w drugim repozytorium.
- **`requireSignedVersion` jest włączone** ([`launcher-keys-and-rotation.md`](launcher-keys-and-rotation.md)).

**Co zostało uruchomione, a co nie** (zgłoszone przez zlecającego 2026-10-07, nie powtarzałem): suchy przebieg wydania gry w osobnym worktree z kluczem deweloperskim (zip z plikiem wykonywalnym, 86 plikami assetów i notami, zgodność rozmiaru, sumy i nazwy w manifeście, odrzucenie podpisu deweloperskiego przez `verify_manifest`, start gry z rozpakowanego katalogu). **Nigdy nie uruchomione:** workflow na runnerze (zadanie macOS, warunki `if:`, krok publikacji), prawdziwa aktualizacja launchera, rotacja kluczy, podpis prawdziwym kluczem sprawdzony przez launcher, cokolwiek na macOS, SmartScreen na czystym komputerze. Szczegóły: [`../guides/launcher.md`](../guides/launcher.md), sekcja 2.

**Skutki znane:**

- **Pierwsze wydanie gry jest jednocześnie pierwszą próbą prawdziwego klucza.** Do tej pory podpis sprawdzały tylko klucze testowe i deweloperski.
- **Launcher nie zainstaluje niczego z zaplanowanego wydania, dopóki repozytorium gry jest prywatne.** README, lista przed pierwszym linkiem, krok 6.
- **Kolejność przed pierwszym linkiem dla znajomego** (README): wydać launcher 0.1.0, zainstalować go z publicznego repozytorium, wydać 0.1.1, kliknąć "Update launcher" w zainstalowanym 0.1.0, i dopiero wtedy wydać pierwszą grę. To jedyny test całej drogi aktualizacji.
- **macOS wymaga osobnej pracy:** wpis macOS w `latest.json`, zadanie w workflow, podpis ad hoc pakietu, bit wykonywalny.

**Czego ta notatka nie przesądza.** Daty pierwszego wydania, tego, czy po przetestowaniu workflow wróci wyzwalacz tagu (komentarz w `release.yml` opisuje, co trzeba dopisać), ani numeru pierwszej wersji gry dla znajomych.

## 4a. Uzupełnienie z wieczoru 2026-10-07: jak poszły pierwsze wydania

Opis stanu i analiza; decyzji właściciela tu nie przybyło poza cytatem "Release now" o wydaniu gry (zgłoszone przez zlecającego, nie powtarzałem; oznaczam, co sprawdziłem poleceniami tylko do odczytu).

- **Droga lokalna z założeń z sekcji 4 została przejechana dla trzech wydań.** Launcher 0.1.0 i 0.1.1 zbudował właściciel na Windowsie, podpisał kluczem aktualizacji A i opublikował jako zwykłe wydania w `Shironex/night-maze-launcher`. Grę 0.10.0 zbudował lokalnie w osobnym worktree ze statycznym środowiskiem uruchomieniowym, a jej `manifest.json` i `news.json` podpisał kluczem manifestu A. Sprawdziłem `gh release view`: `v0.10.0` ma pięć plików (zip, manifest, news, dwa podpisy), nie jest szkicem.
- **Workflow nie brał w tym udziału.** `gh run list --workflow release.yml` nie pokazuje przebiegów, więc zadanie macOS, warunki `if:` i krok publikacji nadal nie były wykonane na runnerze. Założenie "wydania lokalne, workflow jako druga droga" zadziałało tak, jak je zapisano.
- **Pierwsze wydanie gry było pierwszą próbą prawdziwego klucza** (skutek z sekcji 4) i ta próba się udała: przykład `verify_manifest` napisał "verified: manifest of version 0.10.0 is signed by the first (manifest-a) release key", a zainstalowany launcher 0.1.1 pokazał notatki, zainstalował grę i ją uruchomił.
- **Kolejność przed pierwszym linkiem została wykonana tak, jak opisano**, z jednym odstępstwem co do czystego komputera: 0.1.0 zainstalowano i zaktualizowano do 0.1.1 przyciskiem w ustawieniach na jednym komputerze z Windows 11, bez czystego drugiego.
- **Repozytorium gry jest od tego wieczoru publiczne** (sprawdziłem `gh repo view`), więc warunek z sekcji 4 ("launcher nie zainstaluje niczego, dopóki repozytorium jest prywatne") nie obowiązuje. Sekcja 4 i sekcja 5 zostają jako historia.
- **Nie wiadomo, czy właściciel posłuchał dźwięków z 0.10.0**: wydał bez słuchania. Nie zapisuję ich jako zatwierdzonych.
- **Nadal nie uruchomiono:** aktualizacji na czystym drugim komputerze, zachowania SmartScreena na komputerze, który nigdy nie widział launchera, rotacji kluczy, niczego na macOS.

## 5. Kiedy wrócić do tej decyzji

- Po **zbudowaniu i uruchomieniu gry oraz launchera na Macu**: dopisać macOS do wydań i usunąć ten warunek.
- Po **pierwszym przebiegu workflow na runnerze**: zapisać, co trzeba było naprawić. Plik sam mówi, że spodziewać się trzeba poprawek.
- Gdy repozytorium przestanie być prywatne: przemyśleć, czy wyzwalacz tagu ma wrócić. (Od wieczoru 2026-10-07 jest publiczne, a pierwsze wydania były lokalne; punkt czeka na pierwszy przebieg workflow na runnerze.)
- Gdy znajomi zaczną **skarżyć się na SmartScreen** albo antywirus: wrócić do założenia o braku podpisu kodu.
- Gdy właściciel zdecyduje inaczej niż któreś z przyjętych założeń (sekrety, budowanie na GitHubie, podpisywanie).
