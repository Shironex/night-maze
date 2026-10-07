# Licencja: MIT dla kodu, assety nią nie są objęte

Data: 2026-10-07. Stan: obowiązuje jako decyzja, plik `LICENSE` jest utworzony i dodany do historii gita, a nazwę właściciela praw w pliku właściciel potwierdził 2026-10-07 (sekcja 4). Decyzja właściciela projektu jest w sekcji 2; kontekst, tabela, skutki i pytania otwarte to moja analiza.
Kod: [`LICENSE`](../../LICENSE) w korzeniu repozytorium, [`THIRD-PARTY-NOTICES.txt`](../../THIRD-PARTY-NOTICES.txt) (noty bibliotek gry), [`launcher/THIRD-PARTY-NOTICES.txt`](../../launcher/THIRD-PARTY-NOTICES.txt) (noty launchera).

## 1. Kontekst

Repozytorium gry jest prywatne (`gh repo view Shironex/night-maze`, 2026-10-07: `PRIVATE`). Launcher czyta wydania gry bez tokena, więc żeby znajomi mogli dostawać grę, repozytorium musi być publiczne ([`../guides/launcher.md`](../guides/launcher.md)). Z chwilą upublicznienia potrzebny jest plik, który mówi, co wolno robić z kodem, a czego z grafiką, dźwiękiem i tekstem fabuły.

Repozytorium ma dwa rodzaje materiału o różnym pochodzeniu: kod (C++, shadery, skrypty, launcher) i dzieła: modele, tekstury, dźwięki i wideo w `assets/` oraz tekst fabuły w `docs/story/`. Dźwięki generuje skrypt ([`sounds-generated-by-script.md`](sounds-generated-by-script.md)).

## 2. Decyzja

Decyzja właściciela projektu (2026-10-07), wybór z listy opcji, której nie mam w materiałach (opcji nie opisuję):

**Licencja, gdy repozytorium gry stanie się publiczne: MIT dla kodu, assety nią nie są objęte.**

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza albo fakty sprawdzone w pliku.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz jest decyzją właściciela; innych opcji z listy nie znam.

| Możliwość | Uwaga |
|---|---|
| **MIT dla kodu, assety wyłączone (wybrane)** | tekst `LICENSE` ma dwie części: pełną licencję MIT i sekcję "Assets are not covered" |
| MIT dla wszystkiego, razem z assetami | nie znam takiej opcji z materiałów; plik tego nie robi |
| Brak pliku licencji | nie znam takiej opcji z materiałów; w publicznym repozytorium bez pliku nikt nie ma wyraźnego pozwolenia na nic |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela nie mam. Nie dopisuję własnego.

**Co mówi plik** (sprawdzone w `LICENSE`, 2026-10-07):

- Pierwsza część to tekst MIT z linią `Copyright (c) 2026 Kacper Lachowicz (Shironex)`.
- Sekcja "Assets are not covered": licencja MIT dotyczy kodu źródłowego w repozytorium, a **assety nie są nią objęte i zostają z wszystkimi prawami zastrzeżonymi**. Wymienione: modele, tekstury, dźwięki, wideo, grafika interfejsu i tekst fabuły pod `assets/` i `docs/story/`. Kopiowanie lub ponowne użycie wymaga zgody właściciela.
- Wyjątek: plik, który podaje inną licencję. Czcionki i biblioteki stron trzecich zachowują własne licencje, odsyła do `THIRD-PARTY-NOTICES.txt`.

**Nazwa właściciela praw (rozstrzygnięte 2026-10-07).** Właściciel wybrał z trzech możliwości (nazwa konta, imię i nazwisko, oba) trzecią i podał nazwisko: "as for name use Kacper Lachowicz". Linia w pliku brzmi `Copyright (c) 2026 Kacper Lachowicz (Shironex)`. Tego samego dnia właściciel potwierdził podział: shadery i pliki menu (RML, RCSS) w `assets/` są kodem i podlegają MIT, a modele, tekstury, skybox, dźwięki, wideo i tekst fabuły pozostają zastrzeżone.

**Pytania, które plik zostawia otwarte** (analiza, nie decyzje):

- **Katalog `docs/`.** Wyłączony jest tylko `docs/story/`. Polskie dokumenty kursu w reszcie `docs/` nie są wymienione ani jako objęte, ani jako wyłączone. Sformułowanie "source code" ich wprost nie obejmuje. Do rozstrzygnięcia przez właściciela.
- **Paczka dla znajomych nie niesie pliku `LICENSE`.** `launcher/scripts/package-game.mjs` pakuje do zipa plik wykonywalny, `assets/` i `THIRD-PARTY-NOTICES.txt`, bez `LICENSE`. Assety w zipie są więc rozpowszechniane bez żadnej informacji o ich warunkach, poza ogólną zasadą zastrzeżonych praw. Czy to ma zostać, nie wiem.
- **Pliki z własną licencją w `assets/`.** Czcionka Atkinson Hyperlegible ma w repozytorium własny tekst licencji (`assets/fonts/OFL.txt`, czytany przez `build-notices.mjs`). Wyjątek w `LICENSE` ("a file that states another licence") obejmuje takie przypadki, ale nikt nie sprawdził, czy w `assets/` jest więcej plików z cudzą licencją.
- **Czas obowiązywania.** Decyzja mówi "gdy repozytorium stanie się publiczne". Plik jest już na dysku, więc zależy od właściciela, kiedy trafi do historii (zwykły commit) i czy znajomi dostaną grę wcześniej, niż repozytorium będzie publiczne. To drugie nie jest wykluczone przez launcher, który wymaga tylko publicznego repozytorium z wydaniami.

**Czego ta notatka nie przesądza.** Daty upublicznienia, tego, czy `docs/` będzie objęty licencją, ani brzmienia nazwy właściciela praw.

## 5. Kiedy wrócić do tej decyzji

- Przed **upublicznieniem repozytorium gry**: potwierdzić nazwę właściciela praw w `LICENSE`, rozstrzygnąć status `docs/` i dodać plik do historii.
- Gdy do repozytorium trafi **asset z cudzej licencji**, albo gdy właściciel zechce **udostępnić** któryś asset: dopisać go do wyjątków.
- Gdy zmieni się **zestaw bibliotek**: odświeżyć `THIRD-PARTY-NOTICES.txt` (`node launcher/scripts/build-notices.mjs --deps build/release/_deps`), a dla launchera `pnpm notices` w `launcher/`.
