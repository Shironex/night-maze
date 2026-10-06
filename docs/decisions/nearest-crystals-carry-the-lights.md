# Więcej kryształów niż świateł: światła niosą kryształy najbliższe oka, z wygaszaniem na brzegu zbioru

Data: 2026-10-06. Stan: obowiązuje, jako wybór wykonawczy autora kodu. Nie jest decyzją właściciela: właściciel zdecydował, że poziomy trudności zmieniają liczbę kryształów ([`menu-scope-for-m9.md`](menu-scope-for-m9.md)), a to, jak światła nadążają za większą liczbą kryształów, wybrał autor kodu. Zmienia jedno z ograniczeń z [`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md): limit 16 kryształów, wynikający z liczby świateł w shaderze.
Kod: [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) i [`Lighting.cpp`](../../src/game/Lighting.cpp) (`PointLightSpot`, `nearestPointLights`, `POINT_LIGHT_FADE_DISTANCE`, `buildLightSet`), [`src/game/Crystals.hpp`](../../src/game/Crystals.hpp) (`MAX_CRYSTAL_COUNT`, `placeCrystals`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onRender`), testy w [`tests/LightingTests.cpp`](../../tests/LightingTests.cpp), [`tests/CrystalTests.cpp`](../../tests/CrystalTests.cpp) i [`tests/RoundTests.cpp`](../../tests/RoundTests.cpp). Dokumenty modułów: [`../modules/scene/lights.md`](../modules/scene/lights.md), [`../modules/game/flashlight.md`](../modules/game/flashlight.md), [`../modules/game/gameplay.md`](../modules/game/gameplay.md), [`../modules/game/difficulty.md`](../modules/game/difficulty.md).

## 1. Kontekst

Blok światła w shaderze ma 16 miejsc na światła punktowe (`scene::MAX_POINT_LIGHTS`, blok ma 928 bajtów), a do tej pory każdy kryształ niósł światło, więc liczba kryształów była przycięta do 16 ([`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md)). Trzy poziomy trudności ([`menu-scope-for-m9.md`](menu-scope-for-m9.md)) mają według propozycji autora 13, 26 i 40 kryształów ([`../modules/game/difficulty.md`](../modules/game/difficulty.md)), czyli więcej niż 16. Trzeba było zdecydować, co się dzieje ze światłami, gdy kryształów jest więcej niż miejsc.

Ograniczenia: jeden blok uniformów wysyłany raz na klatkę, pętla po światłach w czterech programach shaderów (wszystkie czytają ten blok), brak zmian w shaderach i w bloku, ta sama klatka dla tego samego ziarna i położenia oka.

## 2. Decyzja

Labirynt może mieć do 64 kryształów (`MAX_CRYSTAL_COUNT`). W każdej klatce światła niosą **16 kryształów najbliższych oka** tej klatki (w menu głównym oka kamery menu), a światła przy brzegu zbioru są **wygaszane do zera**, żeby żadne nie zapalało się ani nie gasło skokiem. Brzeg to odległość pierwszego światła, które nie weszło do zbioru: światło na brzegu ma siłę 0, a od 4 m (`POINT_LIGHT_FADE_DISTANCE`) wewnątrz brzegu siłę 1. Intensywność światła to intensywność z ustawień razy siła. Jarzenie samego kryształu (materiał, bloom) jest rysowane dla każdego kryształu, bo to nie jest światło.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **16 najbliższych oka, z wygaszaniem na brzegu (wybrana)** | blok i shadery bez zmian, koszt klatki stały, brak skoków, bo dwa światła, które zamieniają się miejscami, mają tę samą odległość, czyli oba są na brzegu i oba ciemne | wybór co klatkę (sortowanie kilkudziesięciu odległości na procesorze), dalekie kryształy świecą tylko własnym jarzeniem, a na brzegu zbioru światło słabnie |
| Ten sam wybór bez wygaszania | najprostszy | światło zapala się i gaśnie pełną jasnością w klatce, w której dwa kryształy zamieniają się miejscami: to dokładnie zastrzeżenie z notatki o liczbie kryształów |
| Większy blok światła (na przykład 64 miejsca) | każdy kryształ ma swoje światło, bez wyboru | blok rośnie z 928 do 3232 bajtów (`160 + 64 * 48`, rozmiar według układu `std140` z [`../modules/gfx/uniform-buffers.md`](../modules/gfx/uniform-buffers.md)), a pętla po światłach w czterech programach liczy każde światło dla każdego fragmentu: koszt rośnie z liczbą świateł, choć większość leży poza zasięgiem 3 m. **Nie mierzyłem tego**; to wniosek z budowy shaderów |
| Oświetlenie klastrowane (światła przypisane do komórek widoku) | skaluje się na wiele świateł | to osobny przebieg i osobne struktury danych, a przypisanie na GPU wymaga zwykle shaderów obliczeniowych, których OpenGL 4.1 (wersja dostępna na macOS) nie ma. Zbyt duży koszt dla gry, której światła mają zasięg 3 m |
| Zostawić limit 16 kryształów i różnicować poziomy samym rozmiarem | zero kodu | sprzeczne z decyzją o poziomach zmieniających liczbę kryształów; w dużym labiryncie jeden kryształ na kilkadziesiąt komórek |

## 4. Uzasadnienie i skutki

**Dlaczego najbliższe oka wystarczą.** Światło kryształu sięga około 3 m (`LightingSettings::pointRadius`), więc kryształ daleko od oka oświetla ziemię, która jest mała w obrazie albo ukryta za ścianami. Jego własne jarzenie (materiał emisyjny z bloomem) jest rysowane dla każdego kryształu, więc dalekie kryształy nadal widać.

**Dlaczego wygaszanie działa.** Zbiór świateł zmienia się w klatce, w której dwa światła zamieniają się miejscami w uporządkowaniu po odległości. W tej klatce są one tak samo daleko od oka, więc obie leżą na brzegu zbioru, gdzie siła wynosi 0. Test `two lights that change places at the edge of the set are both dark` sprawdza to dla dwóch świateł w odległościach 5,00 m i 5,01 m (siła poniżej 0,01 po obu stronach zamiany). Test `with too many lights the nearest ones are chosen and fade towards the edge` sprawdza liczby: dziesięć świateł w odległościach od 1 do 10 m, miejsce na sześć, brzeg w 7 m, siły 1, 1, 1, 0,75, 0,5, 0,25.

**Skutki dla liczb.** `crystalCountFor` jest przycięte do `MAX_CRYSTAL_COUNT` (64), nie do 16: labirynt 16 na 16 dostaje przy domyślnych ustawieniach 32 kryształy, a 40 na 40 dostaje 64. `placeCrystals` przyjmuje liczbę wprost, a większa liczba **tylko dopisuje** kryształy na końcu listy: pierwsze zostają w tych samych komórkach. Limit 64 nie wynika z shadera: każdy kryształ to model rysowany w trzech przebiegach (dwie mapy cieni i scena) i romb na minimapie.

**Co przez to tracę.**

- Nikt nie widział na ekranie, jak światła gasną i zapalają się przy ruchu po labiryncie z więcej niż 16 kryształami. Zachowanie jest policzone i przetestowane na danych, nie obejrzane.
- Menu główne na `Normal` i `Hard` ma w tle przelot, a światła wybiera się spośród 16 najbliższych oka przelotu: spora część tła może nie mieć światła kryształu. Nikt tego nie oglądał na ekranie.
- Linia `Lit` w oknie debug pokazuje `min(pozostałe, 16)` z liczbą pozostałych kryształów: liczy światła w zbiorze, także te wygaszone do zera.
- Przy dwóch światłach równo oddalonych od oka kolejność zależy od stabilnego sortowania (`stable_sort`), więc ten sam wejściowy porządek daje ten sam wynik.

## 5. Kiedy wrócić do tej decyzji

- Gdy ręczna gra pokaże zauważalne gaśnięcie świateł przy ruchu: najpierw większa odległość wygaszania (`POINT_LIGHT_FADE_DISTANCE`), potem większy blok światła.
- Gdy pomiar na scalonym drzewie pokaże, że wybór co klatkę kosztuje: dziś jest to sortowanie najwyżej 64 odległości.
- Gdy tło menu głównego na `Normal` i `Hard` okaże się za ciemne: osobny wybór świateł dla przelotu.
- Gdy długość tablicy świateł w shaderze zmieni się: stała `MAX_POINT_LIGHTS` w trzech miejscach ([`../modules/scene/lights.md`](../modules/scene/lights.md)).
