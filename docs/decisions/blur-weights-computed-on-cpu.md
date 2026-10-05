# Wagi rozmycia Gaussa: liczone w C++ i wysyłane jako tablica uniformów

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Bloom.hpp`](../../src/game/Bloom.hpp), [`src/game/Bloom.cpp`](../../src/game/Bloom.cpp) (`bloomBlurWeights`, `BLOOM_BLUR_RADIUS`, `BLOOM_BLUR_SIGMA`), [`assets/shaders/post/blur.frag`](../../assets/shaders/post/blur.frag) (`BLUR_RADIUS`, `uWeights`), [`src/gfx/Shader.hpp`](../../src/gfx/Shader.hpp) (`setFloatArray`), [`tests/BloomTests.cpp`](../../tests/BloomTests.cpp). Dokumenty modułów: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.13 i 5.10, [`../modules/gfx/uniforms.md`](../modules/gfx/uniforms.md).

## 1. Kontekst

Rozmycie Gaussa potrzebuje zestawu wag: siedmiu liczb dla promienia 6, policzonych z funkcji `exp(-d * d / (2 * sigma * sigma))` i podzielonych przez ich sumę. Te liczby muszą znaleźć się w shaderze fragmentów. Do tej części klasa `gfx::Shader` umiała ustawiać pojedyncze uniformy (`int`, `float`, `vec3`, macierze), a tablic poza blokami uniformów w grze nie było.

Zasada projektu mówi, że wszystko, co da się policzyć bez OpenGL, ma leżeć w bibliotece, którą linkuje program testowy, bo shaderów testy nie uruchamiają.

## 2. Decyzja

Wagi liczy funkcja `game::bloomBlurWeights` w C++ (biblioteka `game_logic`), a `PostProcess::drawBloom` wysyła je raz na klatkę do tablicy `uniform float uWeights[BLUR_RADIUS + 1]` nową metodą `Shader::setFloatArray`, czyli jednym wywołaniem `glUniform1fv`. Promień jest zapisany w dwóch miejscach, jako `BLOOM_BLUR_RADIUS` w C++ i `BLUR_RADIUS` w shaderze, z komentarzem po obu stronach, że muszą się zgadzać.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| wagi wpisane w shader jako stała tablica liczb | zero kodu po stronie C++, żadnej nowej metody w `Shader`. Tak robi większość przykładów | siedem liczb "znikąd": nie widać, z jakiej sigmy pochodzą, i nikt nie sprawdza, czy sumują się do 1. Zmiana sigmy to ręczne przeliczenie i przepisanie. Test jednostkowy nie ma czego dotknąć |
| wagi liczone w shaderze, `exp` dla każdego odczytu | jedno miejsce z promieniem i sigmą, sigma mogłaby być uniformem z suwaka | trzynaście wywołań `exp` i normalizacja **na każdy piksel każdego przebiegu**, czyli miliony razy na klatkę, żeby dostać zawsze te same siedem liczb. Nadal bez testu |
| **wagi liczone w C++, wysyłane tablicą uniformów (wybrana)** | wzór jest w C++ i ma trzy testy: suma równa 1, wagi maleją, stosunki zgodne z funkcją Gaussa (plus dwie liczby policzone ręcznie). Sigma jest nazwaną stałą z komentarzem. Shader tylko mnoży i dodaje | promień w dwóch miejscach, których zgodności nic nie pilnuje. Nowa metoda `setFloatArray` w klasie `Shader`. Siedem `exp` na klatkę (pomijalne) |
| jak wyżej, ale promień wstrzykiwany do shadera z C++ (dopisywana linia `#define` przy wczytywaniu) | jedno miejsce z promieniem | loader shaderów musiałby umieć wstawiać definicje, czego dziś nie robi ([`../modules/gfx/shader-includes.md`](../modules/gfx/shader-includes.md)). Dużo mechanizmu dla jednej liczby |
| wagi w bloku uniformów (UBO), jak światła | mechanizm już jest w projekcie | blok dla siedmiu liczb jednego programu to więcej kodu niż jedno `glUniform1fv`, a reguły wyrównania `std140` dla tablicy `float` (każdy element zajmuje 16 bajtów) to dodatkowa pułapka do tłumaczenia |

## 4. Uzasadnienie i skutki

**Dlaczego C++.** Normalizacja wag jest jedyną częścią bloomu, w której błąd rachunkowy rośnie wykładniczo: przebiegów jest domyślnie dwanaście z rzędu, więc suma wag 1,1 zamiast 1 daje obraz ponad trzy razy za jasny. To dokładnie ta część, którą warto mieć pod testem, a test może dotknąć tylko kodu w C++.

**Dlaczego tablica uniformów, a nie siedem osobnych uniformów.** Shader czyta wagi w pętli po indeksie, a `glUniform1fv` wysyła całą tablicę jednym wywołaniem.

**Dlaczego wagi są wysyłane co klatkę, chociaż się nie zmieniają.** Przeładowanie shaderów tworzy nowy obiekt programu z wyzerowanymi uniformami. Wysyłanie co klatkę znaczy, że po `Reload shaders` nic nie trzeba pamiętać ani odtwarzać.

**Skutki, które przyjmuję.**

- Promień w dwóch miejscach. Rozjazd nie daje błędu kompilacji ani błędu OpenGL: przy większym promieniu w C++ shader używa początku jądra o sumie mniejszej niż 1 i poświata ciemnieje, przy większym w shaderze ostatnie wagi są zerem. Dokument modułu ma to w pułapkach.
- Sigma nie jest suwakiem. Szerokością poświaty steruje liczba iteracji.
- Jądro jest ucięte na dwóch sigmach: za promieniem zostaje 2,9 procent dzwonu, a normalizacja rozdziela je na pozostałe wagi. Rzeczywiste odchylenie standardowe jądra to 2,73 zamiast 3.

**Czego nie zmierzyłem.** Kosztu wariantu z `exp` w shaderze nikt nie mierzył: "miliony razy na klatkę" to rachunek (230 400 pikseli razy 13 odczytów razy 12 przebiegów przy oknie 1280 x 720), nie pomiar czasu.

## 5. Kiedy wrócić do tej decyzji

- Gdy promień ma się zmieniać w działającej grze: wtedy trzeba albo wstrzykiwania stałej do shadera, albo tablicy o największym rozmiarze i uniformu z liczbą użytych elementów.
- Gdy loader shaderów dostanie definicje wstawiane z C++ z innego powodu: wtedy promień od razu wraca do jednego miejsca.
- Gdy koszt bloomu trzeba będzie obniżyć: trik z odczytami między tekselami (7 odczytów zamiast 13) zmienia to, co jest wysyłane, na wagi i przesunięcia.
