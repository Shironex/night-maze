# Budowanie na Windowsie

> **Uwaga: build na Windowsie NIE został jeszcze przetestowany.**
> Kod M0 powstał i został zweryfikowany wyłącznie na macOS (patrz
> [`build-macos.md`](build-macos.md)). Ten dokument opisuje, jak build **powinien** przebiegać
> na podstawie konfiguracji CMake i dokumentacji narzędzi. Każde stwierdzenie o zachowaniu na
> Windowsie jest przewidywaniem do sprawdzenia. Na końcu jest lista kontrolna pierwszego
> buildu. Po jej przejściu ten dokument trzeba poprawić i usunąć to ostrzeżenie.

## 1. Wymagania

| Narzędzie | Po co | Uwagi |
|---|---|---|
| Visual Studio 2022 z pakietem roboczym "Desktop development with C++" (Programowanie aplikacji klasycznych w C++) | kompilator MSVC, Windows SDK, wbudowany CMake | wybór z PRD. Alternatywa: CLion |
| git dostępny w `PATH` | CMake klonuje nim GLFW i ImGui podczas konfiguracji | sprawdzenie: `git --version` w nowym oknie terminala. Instalator: <https://git-scm.com/> |
| CMake w wersji co najmniej 3.24 | konfiguracja i build | jest częścią pakietu roboczego C++ w Visual Studio. Osobny instalator: <https://cmake.org/download/> |

Uwagi:

- Samo zainstalowanie Visual Studio nie dodaje `cmake` do `PATH` zwykłego terminala. Są dwie
  drogi: używać "Developer PowerShell for VS 2022" (skrót w menu Start, ustawia środowisko
  kompilatora i narzędzi) albo zainstalować CMake osobno z opcją dodania do `PATH`.
- Git jest potrzebny w tym samym terminalu, w którym uruchamiamy CMake. Jeśli
  `git --version` nie działa, konfiguracja zakończy się błędem przy pobieraniu GLFW.
- Bibliotek nie instalujemy ręcznie. GLFW i ImGui pobiera CMake, GLAD jest w repozytorium.
- Sterownik karty graficznej musi obsługiwać OpenGL 4.1 lub nowszy. Aktualne sterowniki
  kart NVIDIA, AMD i Intel obsługują 4.6. Przy bardzo starym sterowniku okno się nie utworzy.

## 2. Budowanie z terminala

Te same presety co na Macu. Polecenia wykonujemy w katalogu głównym repozytorium, w
"Developer PowerShell for VS 2022".

### Debug

```bat
cmake --preset debug
cmake --build --preset debug
build\debug\Debug\night_maze.exe
```

### Release

```bat
cmake --preset release
cmake --build --preset release
build\release\Release\night_maze.exe
```

Zwróć uwagę na dodatkowy katalog `Debug` lub `Release` w ścieżce programu. Na Macu program
leży w `build/debug/night_maze`. Wyjaśnienie w następnej sekcji.

Opis samego pliku presetów (ukryty preset `base`, `inherits`, `binaryDir`) jest w
[`build-macos.md`](build-macos.md), sekcja 3. Plik jest wspólny dla obu systemów.

Plik [`Makefile`](../../Makefile) ze skrótami (`make run`, `make check`) na Windowsie nie był
jeszcze uruchamiany. Wymaga programu `make` i powłoki typu Unix (na przykład Git Bash), a
bez nich wystarczą polecenia `cmake` podane wyżej. Opis:
[`project-structure.md`](project-structure.md), sekcja 3.12.

## 3. Generator Visual Studio jest wielokonfiguracyjny

To najważniejsza różnica względem Maca i częste pytanie na przeglądzie kodu.

**Generator** to część CMake, która tworzy pliki dla konkretnego systemu budowania. Nasze
presety nie wskazują generatora, więc CMake wybiera domyślny dla platformy:

| | macOS | Windows z Visual Studio 2022 |
|---|---|---|
| Domyślny generator | Unix Makefiles | Visual Studio 17 2022 |
| Rodzaj | jednokonfiguracyjny (single-config) | wielokonfiguracyjny (multi-config) |
| Kiedy wybieramy Debug lub Release | przy **konfiguracji**, zmienną `CMAKE_BUILD_TYPE` | przy **budowaniu**, opcją `--config` |
| Co zawiera katalog buildu | pliki dla jednej konfiguracji | rozwiązanie `.sln` ze wszystkimi konfiguracjami naraz |
| Ścieżka programu | `build/debug/night_maze` | `build\debug\Debug\night_maze.exe` |

Generator jednokonfiguracyjny ustala typ buildu raz, podczas `cmake --preset debug`.
Generator wielokonfiguracyjny tworzy projekt, który zna wszystkie konfiguracje (Debug,
Release, RelWithDebInfo, MinSizeRel), a wybór następuje dopiero przy budowaniu. Żeby wyniki
się nie nadpisywały, każda konfiguracja dostaje własny podkatalog, stąd `Debug\` w ścieżce.

Konsekwencje dla naszego `CMakePresets.json`:

```json
"cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
```

Generator Visual Studio **ignoruje `CMAKE_BUILD_TYPE`**. Ta linia działa na Macu, na
Windowsie nie robi nic.

```json
"buildPresets": [
    { "name": "debug", "displayName": "Debug", "configurePreset": "debug", "configuration": "Debug" },
```

Pole **`configuration` w presecie budowania** wybiera konfigurację na Windowsie.
`cmake --build --preset debug` jest tam równoważne `cmake --build build\debug --config Debug`.
Na Macu z kolei to pole jest ignorowane.

Dlatego presety ustawiają **obie** rzeczy: `CMAKE_BUILD_TYPE` dla generatorów
jednokonfiguracyjnych i `configuration` dla wielokonfiguracyjnych. Ten sam plik działa
poprawnie na obu systemach, a każdy system korzysta z "swojej" połowy.

Skutek uboczny wart zrozumienia: na Windowsie katalogi `build\debug` i `build\release` są
technicznie takim samym projektem Visual Studio. O tym, co powstanie, decyduje preset
budowania, a nie preset konfiguracji. Trzymamy się jednak pary `debug` z `debug` i `release`
z `release`, żeby polecenia były identyczne jak na Macu.

Jeszcze jedna różnica: `CMAKE_EXPORT_COMPILE_COMMANDS` działa tylko z generatorami Makefile
i Ninja. Generator Visual Studio nie tworzy `compile_commands.json`. Visual Studio go nie
potrzebuje, ale clangd i clang-tidy na Windowsie wymagają generatora Ninja (podsekcja
"Cursor i VS Code z clangd na Windowsie" w sekcji 4).

## 4. Otwieranie folderu w Visual Studio

Visual Studio 2022 ma wbudowaną obsługę CMake i presetów. Nie tworzymy ręcznie pliku `.sln`.

1. File, Open, Folder i wskazujemy katalog repozytorium (albo "Open a local folder" na
   ekranie startowym).
2. Visual Studio wykrywa `CMakeLists.txt` i `CMakePresets.json` i uruchamia konfigurację.
   Postęp i błędy widać w oknie Output (lista "CMake").
3. Na pasku narzędzi pojawiają się listy rozwijane: preset konfiguracji (`Debug` lub
   `Release`, czyli nasze `displayName`) i preset budowania.
4. Jako element startowy (Select Startup Item) wybieramy `night_maze.exe`.
5. F5 uruchamia z debuggerem, Ctrl+F5 bez niego.

Do sprawdzenia przy pierwszym uruchomieniu:

- **Którego generatora użyje Visual Studio.** Nasz preset nie podaje pola `generator`.
  Z terminala CMake wybierze generator Visual Studio (sekcja 3). Otwierając folder w IDE,
  Visual Studio może zastosować własny wybór generatora, w tym Ninja, którą ma w zestawie.
  Z Ninja build jest jednokonfiguracyjny: działa `CMAKE_BUILD_TYPE`, a program leży w
  `build\debug\night_maze.exe` (bez podkatalogu `Debug`). Presety są przygotowane na oba
  przypadki, ale ścieżka programu będzie inna. Użyty generator widać w pierwszych liniach
  okna Output oraz w `build\debug\CMakeCache.txt` (wpis `CMAKE_GENERATOR`).
- **Nie mieszać generatorów w jednym katalogu.** Jeśli `build\debug` utworzył terminal
  jednym generatorem, a IDE spróbuje użyć innego, CMake zgłosi błąd o niezgodności
  generatora. Rozwiązanie: usunąć `build\debug` i trzymać się jednego sposobu pracy.

Katalogi `.vs/` i `out/` (domyślne katalogi robocze Visual Studio) są w `.gitignore`.

### Cursor i VS Code z clangd na Windowsie

Repozytorium zawiera konfigurację edytora wspólną dla obu systemów: [`.clangd`](../../.clangd),
[`.vscode/settings.json`](../../.vscode/settings.json) i
[`.vscode/extensions.json`](../../.vscode/extensions.json) (opis kluczy w
[`project-structure.md`](project-structure.md), sekcje 3.9 do 3.11, przebieg na Macu w
[`build-macos.md`](build-macos.md), sekcja 6).

Na Windowsie jest jedna istotna różnica. Plik `.clangd` wskazuje na
`build/debug/compile_commands.json`, a generator Visual Studio, którego CMake używa
domyślnie z terminala, **tego pliku nie tworzy** (sekcja 3). Skutek: po zwykłym
`cmake --preset debug` clangd nadal nie zna flag kompilacji, a edytor pokazuje czerwone
błędy "file not found" przy każdym `#include`, mimo że projekt buduje się poprawnie.

Żeby clangd działał, katalog `build\debug` musi być skonfigurowany generatorem Ninja:

```powershell
cmake --preset debug -G Ninja
cmake --build --preset debug
```

Uwagi do tych poleceń:

- Trzeba je wykonać w "Developer PowerShell for VS 2022", żeby kompilator MSVC (`cl.exe`) i
  Ninja dołączona do Visual Studio były w `PATH`.
- Jeden katalog buildu to jeden generator. Jeśli `build\debug` powstał wcześniej generatorem
  Visual Studio, trzeba go najpierw usunąć.
- Z Ninja build jest jednokonfiguracyjny, więc program leży w `build\debug\night_maze.exe`,
  bez podkatalogu `Debug`.
- Do czasu wykonania konfiguracji edytor pokazuje błędy "file not found", tak samo jak na
  Macu.

**Ten przebieg nie został jeszcze sprawdzony na PC.** Wynika z dokumentacji CMake i clangd.
Do potwierdzenia przy pierwszym buildzie: czy `compile_commands.json` powstaje w
`build\debug`, czy clangd poprawnie czyta polecenia kompilatora MSVC i czy rozszerzenie
CodeLLDB z listy rekomendacji nadaje się do debugowania programu zbudowanego przez MSVC
(jeśli nie, zostaje debugger Visual Studio). Punkty są dopisane do listy kontrolnej w
sekcji 11.

### CLion na Windowsie

CLion też czyta `CMakePresets.json`. Dwie rzeczy do ustawienia:

- W Settings, Build, Execution, Deployment, Toolchains wybrać toolchain Visual Studio, a nie
  dołączony MinGW. Z MinGW kompilatorem jest GCC, więc w `CMakeLists.txt` zadziała gałąź
  `else()` z flagami `-Wall -Wextra -Wpedantic` zamiast `/W4`. To też powinno działać, ale
  jest inną konfiguracją niż ta opisana tutaj.
- Włączyć profile CMake z presetów zamiast domyślnego `cmake-build-debug`.

## 5. Ostrzeżenia: `/W4 /permissive-`

Fragment [`CMakeLists.txt`](../../CMakeLists.txt):

```cmake
# Strict warnings for our own targets only (third-party code is built with its defaults).
function(night_maze_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
```

- `if(MSVC)` jest prawdą, gdy kompilatorem jest Microsoft Visual C++. Flagi MSVC mają inną
  składnię (zaczynają się od `/`), więc potrzebne jest rozgałęzienie.
- **`/W4`**: poziom ostrzeżeń 4, najwyższy rozsądny w codziennej pracy. Odpowiednik
  `-Wall -Wextra`. Istnieje też `/Wall`, ale włącza ostrzeżenia w nagłówkach systemowych i
  jest w praktyce nieużywalny.
- **`/permissive-`**: tryb ścisłej zgodności ze standardem C++. MSVC historycznie akceptował
  konstrukcje niezgodne ze standardem, a ta flaga je wyłącza. Odpowiednik `-Wpedantic` w tym
  sensie, że kod, który przejdzie na MSVC, ma większą szansę skompilować się w clang, i
  odwrotnie. To ważne w projekcie na dwa systemy.
- `PRIVATE`: flagi dotyczą tylko wskazanego targetu i nie przenoszą się na jego użytkowników.
- Funkcję wywołujemy tylko dla `engine` i `night_maze`. GLFW, ImGui i GLAD kompilują się ze
  swoimi domyślnymi ustawieniami.

Nagłówki bibliotek są oznaczone jako systemowe (`SYSTEM` w `target_include_directories`,
`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` dla GLFW). Na Macu daje to `-isystem`. Nowsze wersje
CMake i MSVC realizują to samo opcjami `/external:I` i `/external:W0`. Czy ostrzeżenia z
nagłówków GLFW, GLAD i ImGui faktycznie nie pojawiają się pod `/W4`, trzeba potwierdzić przy
pierwszym buildzie (punkt na liście kontrolnej).

Standard C++20 ustawia `set(CMAKE_CXX_STANDARD 20)` razem z `CMAKE_CXX_EXTENSIONS OFF`. Na
MSVC przekłada się to na flagę `/std:c++20`.

Definicja `GL_SILENCE_DEPRECATION` na Windowsie nie ma żadnego efektu. Dotyczy tylko
nagłówków Apple. Zostaje, bo jedna lista definicji dla obu systemów jest prostsza.

Cel: **zero ostrzeżeń pod `/W4`** w naszym kodzie. MSVC zgłasza czasem inne ostrzeżenia niż
clang (na przykład o zawężających konwersjach typów), więc pierwszy build może ujawnić
miejsca, których Mac nie pokazał.

## 6. Okno konsoli

Program jest zdefiniowany jako:

```cmake
add_executable(night_maze
    src/main.cpp
    ...
)
```

Bez słowa `WIN32` w `add_executable` powstaje aplikacja konsolowa: punktem wejścia jest
zwykłe `int main()` (tak jak w [`src/main.cpp`](../../src/main.cpp)), a przy uruchomieniu
obok okna gry otwiera się okno konsoli.

To jest zamierzone. Do konsoli trafiają komunikaty z `core::Log`:

```text
[info] GL_VERSION:  ...
[info] GL_RENDERER: ...
```

oraz błędy z `GL_CHECK` i z callbacku błędów GLFW. Bez konsoli nie byłoby ich gdzie zobaczyć.

- Uruchomienie z terminala: komunikaty pojawiają się w tym samym terminalu.
- Uruchomienie dwuklikiem lub z Visual Studio: otwiera się osobne okno konsoli. Zamknięcie
  go krzyżykiem zabija program.
- Jeśli program kończy się błędem przy starcie, konsola otwarta dwuklikiem zniknie od razu.
  Wtedy uruchamiamy z terminala, żeby przeczytać linię `[error] Fatal: ...`.

## 7. Katalog roboczy i `imgui.ini`

Dear ImGui zapisuje układ paneli w pliku `imgui.ini` w **katalogu roboczym (working
directory)** procesu, a nie obok pliku `.exe`.

| Sposób uruchomienia | Katalog roboczy | Gdzie powstanie `imgui.ini` |
|---|---|---|
| `build\debug\Debug\night_maze.exe` z katalogu repozytorium | katalog repozytorium | w katalogu głównym repozytorium |
| dwuklik na `night_maze.exe` | katalog z plikiem `.exe` | `build\debug\Debug\` |
| Visual Studio (F5) | ustawiany przez IDE, zwykle katalog pliku wykonywalnego | do sprawdzenia |

Plik jest w `.gitignore`, więc nigdzie nie przeszkadza w repozytorium. Skutkiem różnych
katalogów jest tylko to, że układ paneli ustawiony przy uruchomieniu z terminala nie jest
widoczny przy uruchomieniu z IDE i odwrotnie.

Katalog roboczy stanie się naprawdę ważny od M1, gdy program zacznie wczytywać pliki z
`assets/` (shadery, modele). Wtedy trzeba będzie ustalić jedną regułę wyszukiwania zasobów
dla obu systemów. PRD wymaga budowania ścieżek wyłącznie przez `std::filesystem`.

## 8. RenderDoc

RenderDoc to darmowy debugger grafiki: przechwytuje jedną klatkę i pozwala obejrzeć każde
wywołanie rysujące, stan potoku, zawartość tekstur i buforów oraz wejścia i wyjścia shaderów.
Strona projektu: <https://renderdoc.org/>.

- **Działa tylko na Windowsie (i Linuksie). Nie obsługuje macOS.** Na Macu zostają `GL_CHECK`
  i panele ImGui. To jeden z powodów, dla których projekt ma działać na obu systemach.
- Wymaga kontekstu OpenGL w profilu Core w wersji co najmniej 3.2. Nasz kontekst 4.1 Core
  spełnia ten warunek.
- Jest narzędziem zewnętrznym. Nie wymaga żadnych zmian w kodzie ani w CMake.

Typowe użycie: w RenderDoc, w zakładce Launch Application, wskazujemy
`build\debug\Debug\night_maze.exe`, ustawiamy Working Directory na katalog repozytorium,
uruchamiamy program i przechwytujemy klatkę klawiszem F12 lub PrintScreen.

W M0 w przechwyconej klatce będzie tylko czyszczenie ekranu i rysowanie ImGui. Narzędzie
stanie się naprawdę użyteczne od M1 (pierwsza geometria) i przy cieniach oraz efektach
pozaekranowych.

## 9. Końce linii: `.gitattributes`

Windows tradycyjnie kończy linie parą znaków CRLF, macOS i Linux samym LF. Bez ustaleń
praca na dwóch systemach produkuje commity, w których "zmieniła się" każda linia pliku.

Cały plik [`.gitattributes`](../../.gitattributes):

```gitattributes
* text=auto
```

- `*`: reguła dotyczy wszystkich plików.
- `text=auto`: Git sam rozpoznaje, czy plik jest tekstowy. Pliki tekstowe są zapisywane w
  repozytorium zawsze z końcami LF. Pliki binarne (obrazy, modele) nie są ruszane.
- Przy pobieraniu plików do katalogu roboczego Git może na Windowsie zamienić LF na CRLF,
  zależnie od lokalnego ustawienia `core.autocrlf`. Przy commicie zamienia je z powrotem.

Efekt: w repozytorium zawsze jest LF, niezależnie od tego, na którym komputerze powstał
commit. Reguła jest w repozytorium, a nie w ustawieniach Gita na jednym komputerze, więc
działa tak samo wszędzie.

Kompilatory na obu systemach akceptują oba rodzaje końców linii. Problem dotyczył wyłącznie
czytelności historii Gita.

## 10. Rozwiązywanie problemów (przewidywane)

| Objaw | Prawdopodobna przyczyna | Rozwiązanie |
|---|---|---|
| `'cmake' is not recognized` | CMake z Visual Studio nie jest w `PATH` zwykłego terminala | użyj "Developer PowerShell for VS 2022" albo zainstaluj CMake osobno |
| Konfiguracja pada przy pobieraniu GLFW lub ImGui | brak `git` w `PATH` albo brak sieci | zainstaluj git, otwórz nowy terminal, sprawdź `git --version` |
| Błąd o niezgodności generatora | katalog buildu utworzony innym generatorem (terminal a IDE) | usuń `build\debug` i skonfiguruj ponownie jednym narzędziem |
| Nie ma pliku `build\debug\Debug\night_maze.exe` | użyto generatora jednokonfiguracyjnego (Ninja) | szukaj w `build\debug\night_maze.exe`, patrz sekcja 4 |
| `Fatal: Failed to create a window with an OpenGL 4.1 Core context` | sterownik bez OpenGL 4.1, sesja pulpitu zdalnego albo maszyna wirtualna | zaktualizuj sterownik karty. Przeczytaj linię `GLFW error` powyżej |
| `GL_RENDERER` pokazuje zintegrowaną kartę Intel na laptopie z drugą kartą | system wybrał kartę energooszczędną | w ustawieniach grafiki Windows lub panelu sterownika przypisz `night_maze.exe` do wydajnej karty |
| FPS dużo wyższe niż odświeżanie monitora | sterownik wymusza wyłączony vsync | sprawdź ustawienie synchronizacji pionowej w panelu sterownika |
| Ostrzeżenia `/W4` z plików w `_deps` lub `external` | nagłówki systemowe nie zostały wyciszone | zanotuj wersje CMake i MSVC, to punkt z listy kontrolnej |
| Układ paneli nie zapamiętuje się | różne katalogi robocze | sekcja 7 |
| Cursor lub VS Code pokazuje "file not found" przy każdym `#include`, choć build przechodzi | generator Visual Studio nie tworzy `compile_commands.json`, którego szuka `.clangd` | skonfiguruj `build\debug` generatorem Ninja, sekcja 4 |

## 11. Lista kontrolna pierwszego buildu na Windowsie

Do przejścia na PC przed uznaniem M0 za zamknięty na obu systemach. Wyniki (wersje, napisy,
ewentualne ostrzeżenia) warto zapisać i na ich podstawie poprawić ten dokument.

**Środowisko**

- [ ] `cmake --version` pokazuje co najmniej 3.24 (zapisać wersję)
- [ ] `git --version` działa w tym samym terminalu
- [ ] zapisać wersję Visual Studio i kompilatora MSVC

**Konfiguracja**

- [ ] `cmake --preset debug` kończy się bez błędów
- [ ] GLFW `3.4` i ImGui `v1.92.9b-docking` pobrały się do `build\debug\_deps`
- [ ] zapisać generator z `build\debug\CMakeCache.txt` (`CMAKE_GENERATOR`), osobno dla
      terminala i dla "Open Folder" w Visual Studio

**Build Debug**

- [ ] `cmake --build --preset debug` kończy się bez błędów
- [ ] **zero ostrzeżeń pod `/W4`** w plikach z `src/`
- [ ] brak ostrzeżeń pochodzących z nagłówków GLFW, GLAD i ImGui w naszych plikach
- [ ] program jest w `build\debug\Debug\night_maze.exe` (albo zapisać faktyczną ścieżkę)

**Uruchomienie**

- [ ] okno 1280 x 720 z tytułem "Night Maze" otwiera się, tło jest ciemnogranatowe
- [ ] otwiera się okno konsoli z dwiema liniami `[info]`
- [ ] zapisać dokładny napis `GL_VERSION` (oczekiwane: wersja 4.1 lub wyższa) i `GL_RENDERER`
- [ ] w konsoli nie ma linii `[error]`
- [ ] panel "Renderer" jest widoczny, FPS i czas klatki się aktualizują
- [ ] linie "Framebuffer" i "Window" pokazują te same wartości (na Windowsie powinny być równe)

**Sterowanie i interfejs**

- [ ] klawisz `~` (na lewo od `1`) ukrywa i pokazuje panele
- [ ] Esc zamyka program, kod wyjścia 0
- [ ] podczas wpisywania wartości w polu `Clear color` (Ctrl i kliknięcie) Esc anuluje tylko
  edycję i nie zamyka programu, a `~` nie chowa paneli
- [ ] krzyżyk okna zamyka program bez błędów w konsoli
- [ ] docking: panel "Renderer" daje się przeciągnąć i zadokować do krawędzi okna, środek
      pozostaje przezroczysty
- [ ] edytor "Clear color" zmienia kolor tła na żywo
- [ ] po ponownym uruchomieniu układ paneli jest zapamiętany (zapisać, gdzie powstał
      `imgui.ini`)

**Zmiana rozmiaru**

- [ ] zmiana rozmiaru okna: obraz wypełnia całe okno, wartości w panelu się zmieniają
- [ ] maksymalizacja i przywrócenie okna działają
- [ ] minimalizacja i przywrócenie nie powodują błędów ani zawieszenia
- [ ] przeciąganie okna między monitorami o różnym skalowaniu (jeśli są dostępne)

**Build Release**

- [ ] `cmake --preset release` i `cmake --build --preset release` bez błędów i ostrzeżeń
- [ ] `build\release\Release\night_maze.exe` uruchamia się i zachowuje tak samo

**Git i narzędzia**

- [ ] po sklonowaniu i zbudowaniu `git status` nie pokazuje zmienionych plików (końce linii)
- [ ] `build/`, `.vs/`, `imgui.ini` nie pojawiają się w `git status`
- [ ] Visual Studio: "Open Folder", konfiguracja z presetów, F5 uruchamia `night_maze.exe`
- [ ] RenderDoc: przechwycenie jednej klatki działa (opcjonalnie)
- [ ] Cursor lub VS Code z clangd: po `cmake --preset debug -G Ninja` w czystym `build\debug`
  powstaje `compile_commands.json`, a błędy "file not found" w edytorze znikają
  (zapisać wynik, opcjonalnie)

## 12. Powiązane dokumenty

- Wersja dla macOS (zweryfikowana) i opis presetów: [`build-macos.md`](build-macos.md)
- Mapa repozytorium i plików konfiguracyjnych: [`project-structure.md`](project-structure.md)
- Biblioteki: [`../libraries/glfw.md`](../libraries/glfw.md),
  [`../libraries/glad.md`](../libraries/glad.md), [`../libraries/imgui.md`](../libraries/imgui.md)
- Moduły: [`../modules/core/README.md`](../modules/core/README.md) (wstęp i indeks modułu `core`), [`../modules/debug-ui.md`](../modules/debug-ui.md)
- Dokumentacja CMake (generatory, presety): <https://cmake.org/cmake/help/latest/>
