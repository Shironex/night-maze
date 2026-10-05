# Kolizje: AABB i ślizganie oś po osi zamiast silnika fizyki

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/scene/Collider.hpp`](../../src/scene/Collider.hpp), [`Collider.cpp`](../../src/scene/Collider.cpp). Dokument modułu: [`../modules/scene/collision.md`](../modules/scene/collision.md).

## 1. Kontekst

Gracz ma chodzić po labiryncie i nie przechodzić przez ściany. PRD (sekcja 2) wymaga "kolizji ze ścianami (ślizganie się wzdłuż ściany)", a temat 14 wykładu to "Wstęp do kolizji": AABB gracza przeciw ścianom i kule dla kryształów.

Ograniczenia, które zawężają wybór:

- każdą linię kodu muszę umieć wyjaśnić na obronie,
- świat gry to wyłącznie ściany równoległe do osi X albo Z, słupki o kwadratowej podstawie i płaska podłoga,
- nic w grze nie odbija się, nie toczy i nie przewraca. Porusza się tylko gracz,
- kod ma dać się sprawdzić testami bez okna i dawać ten sam wynik na macOS i na Windowsie.

## 2. Decyzja

Kolizje są własnym, małym kodem: struktura `scene::Aabb`, test `scene::overlaps` i funkcja `scene::moveAndSlide`, która przesuwa pudełko oś po osi (x, z, y) i na każdej osi mierzy odstęp do najbliższej przeszkody. Bez silnika fizyki i bez biblioteki zewnętrznej.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **AABB i ruch oś po osi z pomiarem odstępu (wybrana)** | około 70 linii kodu (bez komentarzy), które da się wyjaśnić w całości. Pasuje do świata z samych prostopadłościanów. Ślizganie wychodzi samo, bez osobnego kodu. Na jednej osi nie ma tunelowania. Łatwe do testowania | droga po "schodkach" zamiast po prostej, więc wymaga krótkich kroków. Tylko bryły nieobrócone. Brak grawitacji, masy, odbić |
| AABB i test dyskretny: przesuń, sprawdź `overlaps`, cofnij albo wypchnij | jeszcze prostszy do opisania, to jest wersja z większości wprowadzeń | tunelowanie przy długim kroku. Wypychanie wymaga wyboru kierunku. Błędy zaokrągleń `float` dotyczą jej tak samo, więc tolerancja też byłaby potrzebna. Tej wersji nie napisałem, więc nie mam dla niej pomiarów |
| Silnik fizyki jako biblioteka zewnętrzna | gotowe bryły obrócone, siatki, grawitacja, kontrolery postaci | tysiące linii cudzego kodu, których nie wyjaśnię. Nowa duża zależność do zbudowania na dwóch systemach. Prawie żadna z jego możliwości nie jest grze potrzebna. Temat wykładu to wstęp do kolizji, a nie użycie gotowego silnika |
| Kolizje wprost na siatce labiryntu: "czy między komórką A i B jest ściana" | zero geometrii, bardzo szybkie | gracz byłby punktem bez rozmiaru. Brak płynnego ślizgania przy ścianie o grubości 0,2 m. Nie nadaje się do niczego poza labiryntem. Nie realizuje tematu AABB |
| Kula albo kapsuła dla gracza przeciw AABB ścian | gładkie omijanie narożników | test i reakcja są trudniejsze (najbliższy punkt, wektor wypchnięcia). Kula jest zaplanowana dla kryształów, gdzie wystarczy sam test, bez reakcji |

## 4. Uzasadnienie i skutki

**Dlaczego nie silnik fizyki.** Świat gry jest tak prosty, że silnik rozwiązywałby problemy, których tu nie ma, a jego kod byłby czarną skrzynką na obronie. Własne 70 linii realizuje temat wykładu wprost i w całości.

**Dlaczego pomiar odstępu, a nie test dyskretny.** Pierwszy plan zakładał wersję dyskretną i opis jej ograniczenia: tunelowania przy kroku dłuższym niż gracz i ściana razem. Wybrałem pomiar odstępu, bo odpowiada wprost na pytanie "jak daleko wolno", więc nie potrzebuje osobnego kroku cofania ani wyboru kierunku wypchnięcia, a na pojedynczej osi nie może niczego przeskoczyć. To ostatnie jest zmierzone: test `a step much longer than the wall is thick does not jump over it` (krok 50 m, ściana 0,2 m). Wersji dyskretnej nie napisałem, więc porównanie obu opiera się na rozumowaniu, a nie na dwóch działających implementacjach. Tunelowanie zostaje w dokumencie modułu jako teoria i jako kontrast. **Uwaga na obronę:** funkcja `moveAndSlide` nie tuneluje na osi. Jej prawdziwe ograniczenia to droga po schodkach i dopuszczone zagłębienie do 1 mm.

**Co dostaję.** Ślizganie wzdłuż ścian bez osobnego kodu, brak tunelowania na osi, kod bez OpenGL z testami, które przechodzą na Windowsie w Debug i Release, i funkcję, która nie wie nic o labiryncie, więc nadaje się do zadań laboratoryjnych zbudowanych na `engine`.

**Co tracę i jak to ograniczam.**

- Pudełko jedzie po schodkach (całe x, potem całe z, potem całe y). Błąd jest rzędu długości kroku, więc przesunięcie musi pochodzić ze stałego kroku symulacji: 2,5 cm przy 3 m/s i 1/120 s. Ograniczenie jest pokazane testem `documented limit: a step much longer than the boxes can go around an obstacle`.
- Zagłębienie do 1 mm jest dopuszczone celowo (`CONTACT_TOLERANCE`). Bez tolerancji trzy testy nie przechodzą, a pudełko wychodzi przez ścianę zamkniętej komórki ([`../modules/scene/collision.md`](../modules/scene/collision.md), ćwiczenie 7).
- Nie ma brył obróconych. Ściany labiryntu stoją tylko w dwóch ustawieniach i każde ma własne pudełko.
- Nie ma grawitacji ani skoków. Gra ich nie przewiduje.

**Otwarta sprawa.** Pudełka słupków (0,3 m) wystają 5 cm przed lico ścian (0,2 m) i zatrzymują gracza przytulonego do ściany co 2 m. To nie wynika z tej decyzji, tylko z wymiarów, ale trzeba to rozstrzygnąć przy podłączaniu gracza ([`../modules/game/maze-generator.md`](../modules/game/maze-generator.md), pułapka 6).

## 5. Kiedy wrócić do tej decyzji

- Gdy w grze pojawi się obiekt obrócony o kąt inny niż wielokrotność 90 stopni, z którym gracz ma kolidować.
- Gdy pojawią się obiekty poruszające się szybko (pociski): schodkowa droga i lista sprawdzana w całości przestaną wystarczać.
- Gdy lista przeszkód urośnie tak, że sprawdzanie każdej w każdym kroku będzie widoczne w czasie klatki. Wtedy najpierw siatka komórek jako wstępne odsiewanie, a nie zmiana samego testu.
- Gdy gra dostanie grawitację, skoki albo schody: trzeba będzie co najmniej reguły "stoję na czymś".
