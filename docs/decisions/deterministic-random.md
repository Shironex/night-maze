# Losowość: własna funkcja `randomBelow` zamiast rozkładów z biblioteki standardowej

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/MazeGenerator.hpp`](../../src/game/MazeGenerator.hpp), [`MazeGenerator.cpp`](../../src/game/MazeGenerator.cpp), od M5 także [`src/game/Crystals.cpp`](../../src/game/Crystals.cpp) (`placeCrystals`). Dokument modułu: [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), sekcje 2.6 i 5.4.

## 1. Kontekst

Labirynt powstaje z ziarna (PRD, sekcja 2: "seed w ImGui, przycisk Regeneruj"). Projekt powstaje na dwóch komputerach, na macOS (clang i jego biblioteka standardowa) i na Windowsie (MSVC i jego biblioteka standardowa), i na obu ma się zachowywać tak samo.

Wymaganie: **to samo ziarno i ten sam rozmiar dają ten sam labirynt na obu systemach.** Bez tego:

- błędu znalezionego w labiryncie na jednym komputerze nie da się odtworzyć na drugim,
- nie da się przygotować pokazu na obronę na jednym systemie i pokazać go na drugim,
- test z przypiętym labiryntem przechodziłby tylko na systemie, na którym powstał.

## 2. Decyzja

Liczby losowe pochodzą z `std::mt19937` zasianego jedną liczbą 32-bitową. Zamiany liczby z generatora na zakres dokonuje własna funkcja `game::randomBelow` (reszta z dzielenia z odrzucaniem). Rozkładów i algorytmów losowych z biblioteki standardowej (`std::uniform_int_distribution`, `std::shuffle`) w kodzie gry nie używam.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **`std::mt19937` i własna `randomBelow` (wybrana)** | wynik generatora jest ustalony przez standard C++ co do bitu. Funkcja to kilka linii arytmetyki na liczbach całkowitych bez znaku, takiej samej w każdym kompilatorze. Bez błędu reszty z dzielenia | kilka linii własnego kodu do utrzymania i przetestowania |
| `std::mt19937` i `std::uniform_int_distribution` | zapis idiomatyczny, znany z każdego poradnika | standard nie opisuje algorytmu rozkładu, więc każda biblioteka liczy inaczej. Ten sam generator i ziarno mogą dać inne liczby na macOS i na Windowsie |
| `std::shuffle` na liście sąsiadów | krótki kod: potasuj i weź pierwszego | ten sam problem co wyżej, bo algorytm tasowania też nie jest określony. Do tego zużywa więcej losowań |
| `std::mt19937` i samo `generator() % n` | jedna linia, wynik taki sam wszędzie | błąd reszty z dzielenia. Dla `n` do 4 jest pomijalny, ale to zły wzorzec do kopiowania tam, gdzie `n` będzie duże |
| `rand()` i `srand()` | najprostsze | algorytm zależy od biblioteki C, zakres bywa mały (na Windowsie do 32767), stan jest globalny |
| własny generator (na przykład xorshift albo LCG) i własna funkcja zakresu | pełna kontrola, brak zależności od `<random>` | więcej własnego kodu i ryzyko słabego generatora. `mt19937` jest już w bibliotece i standard gwarantuje jego wynik |
| biblioteka zewnętrzna z generatorem | sprawdzone algorytmy | nowa zależność dla jednej funkcji |

## 4. Uzasadnienie i skutki

**Co gwarantuje standard, a czego nie.** Dla silników (generatorów) standard podaje algorytm, parametry, sposób zasiania jedną liczbą i wartość kontrolną: dziesięciotysięczny wynik `mt19937` utworzonego bez argumentów to 4123659995. Dla rozkładów podaje tylko, jaki rozkład prawdopodobieństwa mają dać. Sposób jego uzyskania zostawia implementacji. Dlatego granica przebiega dokładnie tu: generator biorę z biblioteki, a zamianę na zakres piszę sam.

**Co jest zmierzone.**

- Test `std::mt19937 gives the sequence the C++ standard promises` potwierdza wartość kontrolną na Windowsie.
- Po wstawieniu `std::uniform_int_distribution` do generatora na Windowsie labirynt dla ziarna 1 jest **inny** niż z `randomBelow`: dwa testy z przypiętym labiryntem przestają przechodzić, a testy własności (labirynt doskonały) przechodzą nadal ([`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), ćwiczenie 9). Rozkład z biblioteki MSVC nie jest więc zwykłą resztą z dzielenia.
- Niezależny skrypt w Pythonie z własną implementacją Mersenne Twister daje te same liczby i te same labirynty co program (labirynt 4 na 4 z ziarna 1 i 3 na 3 z ziarna 7, dwanaście pierwszych wyników `randomBelow`).

**Czego nie zmierzyłem.** Kod nie był jeszcze uruchamiany na macOS. Zgodność między systemami wynika na dziś ze standardu i ze zgodności z niezależną implementacją, a nie z porównania dwóch uruchomień. Zmierzy ją pierwszy przebieg testów na Macu: testy `golden maze: 4 x 4 cells from seed 1 has exactly these walls` i `randomBelow gives the same numbers on every system` istnieją właśnie po to.

**Skutki dla reszty kodu.**

- Każda losowość w grze, od której zależy rozgrywka, ma iść przez `std::mt19937` z jawnym ziarnem i `randomBelow`. W M5 tak powstało rozmieszczenie kryształów: `placeCrystals` ma własny generator, zasiany ziarnem labiryntu powiększonym o stałą, tasuje komórki ręcznie napisanym algorytmem Fishera-Yatesa na `randomBelow` (zamiast `std::shuffle`) i tak samo losuje wariant modelu. Generator labiryntu nie dostał przy tym żadnego losowania, więc labirynty wzorcowe się nie zmieniły. Wyjście nie jest losowane wcale: wynika z gotowej siatki ([`exit-farthest-cell.md`](exit-farthest-cell.md)).
- Na wynik wpływa także **kolejność** i **liczba** losowań. Kolejność kierunków (North, East, South, West) i komórka startowa są częścią "formatu" labiryntu. Dodanie jednego losowania w środku generatora zmienia wszystkie labirynty.
- Generator jest zmienną lokalną funkcji, nie globalną: dwa wywołania nie mogą sobie przeszkodzić.
- W generatorze nie ma liczb zmiennoprzecinkowych. Ich wyniki mogą się różnić między procesorami i ustawieniami kompilatora na ostatniej cyfrze, a tu jedna cyfra zmienia wybór sąsiada.

**Koszt.** Kilka linii w `randomBelow`, trzy przypadki testowe i jedna pułapka do zapamiętania: nie wstawiać rozkładów standardowych do kodu gry.

## 5. Kiedy wrócić do tej decyzji

- Gdy pierwszy przebieg testów na macOS pokaże inny labirynt wzorcowy. Wtedy trzeba znaleźć, która z trzech warstw (generator, zamiana na zakres, kolejność) się różni.
- Gdy gra będzie potrzebowała losowych liczb zmiennoprzecinkowych o powtarzalnym wyniku (na przykład losowe przesunięcie kryształu w komórce). `std::uniform_real_distribution` ma ten sam problem co wersja całkowita, więc potrzebna będzie druga własna funkcja, zbudowana na `randomBelow`.
- Gdy losowość ma być tylko ozdobą bez wpływu na rozgrywkę: tam zgodność między systemami nie jest potrzebna i wolno użyć czegokolwiek, byle z osobnego generatora. Na razie takiego miejsca nie ma. Migotanie latarki z M5 (`flashlightFlicker`) w ogóle nie losuje: to iloczyn dwóch sinusów od czasu, a kołysanie i pulsowanie kryształów to też funkcje czasu.
