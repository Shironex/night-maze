# Przebieg jasności: udział jasności ponad progiem zamiast twardego progu

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/bright.frag`](../../assets/shaders/post/bright.frag), [`assets/shaders/common/color.glsl`](../../assets/shaders/common/color.glsl) (`luminance`, `REC709_LUMINANCE_WEIGHTS`), [`src/game/Bloom.hpp`](../../src/game/Bloom.hpp) (`BloomSettings::threshold`). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.12 i 4.6.

## 1. Kontekst

Pierwszy krok bloomu ma zostawić z obrazu sceny tylko to, co jest jaśniejsze od progu, a resztę zamienić w czerń. "Jaśniejsze od progu" trzeba zamienić na wzór, a możliwych wzorów jest kilka i dają różny obraz. W tej grze rzeczą, która świeci najmocniej, jest kryształ: ma nasyconą turkusową barwę i **pulsuje**, czyli jego jasność zmienia się płynnie w czasie między 70 a 100 procentami. Wzór musi więc znosić dobrze dwie rzeczy: jasność, która przechodzi przez okolice progu, i kolor, którego kanały bardzo się różnią.

## 2. Decyzja

Przebieg jasności liczy jasność piksela jako luminancję z wagami Rec. 709 i mnoży **cały kolor** przez udział jasności, który leży ponad progiem: `kolor * max(L - T, 0) / L`. Mianownik jest zabezpieczony stałą `MIN_LUMINANCE = 0.0001`.

## 3. Rozważane możliwości

Liczby w tabeli to wynik dla piksela (0,3, 2,0, 1,5) o jasności 1,6025 i progu 0,8, policzony ze wzorów.

| Możliwość | Wynik | Zalety | Wady |
|---|---|---|---|
| twardy próg: cały kolor, gdy `L > T`, inaczej czerń | (0,3, 2,0, 1,5) | najprostszy wzór, znany z większości opisów bloomu. Zachowuje barwę | skok na progu: piksel o jasności 0,79 nie daje nic, o jasności 0,81 oddaje wszystko. Poświata pulsującego kryształu zapalałaby się i gasła, a krawędzie jasnych plam migotałyby przy ruchu kamery |
| odjęcie progu od każdego kanału: `max(kolor - T, 0)` | (0, 1,2, 0,7) | bez skoku, bez dzielenia | zmienia barwę: kanały poniżej progu znikają w całości. Proporcja zielonego do niebieskiego zmienia się z 1,33 na 1,71, a czerwony przepada. Poświata turkusowego kryształu byłaby bardziej zielona niż on sam |
| **udział jasności ponad progiem (wybrana)** | (0,150, 1,002, 0,751) | bez skoku: jasność wyniku to dokładnie `L - T`, czyli zero na progu. Bez zmiany barwy: trzy kanały są mnożone przez tę samą liczbę | jedno dzielenie na piksel i konieczność zabezpieczenia przed `0 / 0`. Piksel tuż nad progiem prawie nic nie daje, więc próg trzeba ustawić wyraźnie pod tym, co ma świecić |
| miękkie kolano: gładkie przejście w pasie wokół progu, jak w dużych silnikach | jeszcze łagodniejszy początek poświaty, osobny parametr szerokości przejścia | drugi suwak i wzór, którego nie da się wytłumaczyć w dwóch zdaniach. Nie budowałem tego |
| jasność jako największy z trzech kanałów zamiast luminancji | tańsze, nasycone kolory łatwiej przekraczają próg | czysty błękit o wartości 1 byłby "tak samo jasny" jak biel, co nie zgadza się z tym, co widzi oko |

## 4. Uzasadnienie i skutki

**Dlaczego ten wzór.** Oba problemy z kontekstu są w nim rozwiązane jedną linią shadera. Komentarz przy `CRYSTAL_GLOW_STRENGTH` nazywa cel wprost: poświata ma oddychać, a nie mrugać. Dzielenie przez `L` jest tym, co pozwala skalować cały kolor zamiast odejmować od kanałów, i dlatego barwa zostaje.

**Dlaczego luminancja Rec. 709.** sRGB ma te same barwy podstawowe co Rec. 709, więc to są wagi właściwe dla kolorów, którymi gra liczy. Bufor sceny jest liniowy, a wagi są zdefiniowane dla wartości liniowych, więc niczego nie trzeba przeliczać. Funkcja stoi w `common/color.glsl`, obok pozostałych funkcji o kolorze, a nie w `bright.frag`. Kod nie mówi dlaczego. Mój odczyt: tak samo jak `linearToSrgb` dotyczy ona wartości liniowych, a plik wspólny jest miejscem, z którego weźmie ją następny shader, który będzie potrzebował jasności.

**Skutki, które przyjmuję.**

- Próg nie jest granicą "świeci albo nie", tylko punktem, od którego poświata zaczyna rosnąć od zera. Piksel o jasności 1,0 przy progu 0,8 oddaje tylko 20 procent siebie.
- Z tego wynika otwarta obserwacja: plama latarki na ścianie nie daje poświaty nawet z metra. Ściana zostaje pod progiem albo tuż nad nim. Przy twardym progu i tej samej liczbie mogłaby świecić.
- Próg jest w liniowych wartościach bufora przed ekspozycją, więc suwak `Exposure` nie zmienia tego, co świeci.
- Wzór istnieje tylko w GLSL i nie ma testu jednostkowego. Liczby w dokumencie modułu są przeliczone ze wzoru, nie odczytane z gry.

**Czego nie zmierzyłem.** Dwóch odrzuconych wzorów nikt w tej grze nie uruchomił: ich wady to wnioski z rachunku na jednym pikselu, a nie obserwacje z ekranu. Dokument modułu ma ćwiczenia, które pozwalają je obejrzeć (sekcja 8).

## 5. Kiedy wrócić do tej decyzji

- Gdy latarka albo inne światło ma dawać poświatę na powierzchniach: wtedy niższy próg albo miękkie kolano.
- Gdy dojdą kolorowe światła o bardzo różnych barwach (M8) i okaże się, że niebieskie rzeczy prawie nie świecą: waga błękitu w luminancji to 0,07.
- Gdy powstanie odpowiednik `luminance` w C++ (na przykład dla automatycznej ekspozycji): wtedy wzór przebiegu jasności też warto przenieść tam, gdzie da się go testować.
