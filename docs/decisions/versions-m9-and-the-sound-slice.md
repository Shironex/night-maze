# Numery wersji: 0.9.0 to kamień M9, 0.10.0 to kawałek z dźwiękiem

Data: 2026-10-07, uzupełniona wieczorem tego dnia (sekcja 4a). Stan wieczorem 2026-10-07: obowiązuje, z kodem; tag `v0.10.0` istnieje i wydanie 0.10.0 jest opublikowane (rano nie było żadnego tagu). Decyzja właściciela projektu jest w sekcji 2; kontekst, tabela, skutki i warunki powrotu to moja analiza.
Kod: [`CMakeLists.txt`](../../CMakeLists.txt) (linia `project(NightMaze VERSION 0.10.0 ...)`), [`.github/workflows/release.yml`](../../.github/workflows/release.yml) (porównanie tagu z wersją), [`CHANGELOG.md`](../../CHANGELOG.md). Dokument modułu: [`../guides/launcher.md`](../guides/launcher.md) (wydania).

## 1. Kontekst

Do 2026-10-07 `CMakeLists.txt` miał wersję `0.1.0` z pierwszego commita z targetami. Repozytorium nie ma żadnego tagu (`git tag` jest puste). Workflow wydania ([`.github/workflows/release.yml`](../../.github/workflows/release.yml)) startuje ręcznie z istniejącego tagu i sprawdza, że tag jest równy `v` plus wersja z `CMakeLists.txt`; inaczej zatrzymuje wydanie w pierwszej minucie.

## 2. Decyzja

Decyzja właściciela projektu (2026-10-07), wybór spośród opcji, które mu przedstawiono (samych opcji nie mam w materiałach, więc ich nie opisuję):

1. **0.9.0 to kamień milowy M9.**
2. **0.10.0 to kawałek z dźwiękiem.**

Sprawdzone w kodzie (nie jest to część decyzji): jedynym źródłem wersji jest linia `project(NightMaze VERSION ...)` w `CMakeLists.txt`, z której workflow wydania czyta wersję i z którą porównuje tag.

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Opcji, z których wybierał właściciel, nie znam; poniżej tylko to, co wynika z kodu.

| Możliwość | Uwaga |
|---|---|
| **0.9.0 = M9, 0.10.0 = kawałek z dźwiękiem (wybrane)** | numer wersji w `CMakeLists.txt` to `0.10.0` |
| Zostać przy `0.1.0` | wersja z pierwszego commita z targetami; nie mówi nic o stanie gry |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela nie mam. Nie dopisuję własnego.

**Skutki znane** (analiza, sprawdzone w repozytorium):

- **Wersja `0.9.0` nigdy nie stała w `CMakeLists.txt`.** Historia pliku (`git log -S"NightMaze VERSION"`) pokazuje przejście od razu z `0.1.0` do `0.10.0` w commicie `74256ef` ("build(cmake): set the version to 0.10.0", 2026-10-07). Etykieta 0.9.0 dla M9 istnieje więc jako numeracja właściciela, bez commita i bez tagu.
- **Żaden tag nie istnieje**, więc nie ma jeszcze tagu `v0.10.0`. Workflow wydania wymaga, żeby tag istniał i równał się `v0.10.0`.
- **Wpis w dzienniku zmian** dla 0.10.0 dodał commit `186d2e8` ("docs(release): add the changelog entry for 0.10.0").
- **Zmiana wersji to zmiana jednej linii** w `CMakeLists.txt`. Tag musi potem być z nią zgodny.
- **Dwa dokumenty cytują starą wersję `0.1.0`:** [`../guides/project-structure.md`](../guides/project-structure.md) i [`../libraries/glad.md`](../libraries/glad.md); w tym samym commicie co ta notatka są poprawione.

**Czego ta notatka nie przesądza.** Numeru następnego kawałka, daty pierwszego wydania ani tego, czy kamień M9 dostanie tag `v0.9.0` po fakcie.

## 4a. Uzupełnienie z wieczoru 2026-10-07: tag `v0.10.0` istnieje

Opis stanu (zgłoszone przez zlecającego; sprawdziłem `git tag` i `gh release view`). Zdanie z sekcji 4, że żaden tag nie istnieje, opisuje stan z rana. Wieczorem właściciel wypchnął `main` i opisany (annotated) tag `v0.10.0`, który wskazuje commit `7b5e0b1` (`CMakeLists.txt` ma wtedy `0.10.0`, więc tag zgadza się z wersją, jak wymaga workflow). Wydanie `v0.10.0` w `Shironex/night-maze` jest opublikowane, nie jest szkicem i ma pięć plików (zip, manifest, news, dwa podpisy). Zbudowano je lokalnie, nie przez workflow. Tagu `v0.9.0` nadal nie ma i nic nie wskazuje, że będzie: ostatni punkt "Czego ta notatka nie przesądza" pozostaje otwarty.

## 5. Kiedy wrócić do tej decyzji

- Przy następnym kawałku: wybrać kolejny numer i zmienić linię w `CMakeLists.txt`.
- Przy pierwszym tagu: sprawdzić, że tag równa się `v` plus wersja z `CMakeLists.txt`. (Zrobione wieczorem 2026-10-07: `v0.10.0` na `7b5e0b1`, sekcja 4a.)
