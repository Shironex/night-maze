# Wartości odbić dobrane tak, żeby efekt było widać, a nie tak, jak jest w fizyce

Data: 2026-10-06. Stan: obowiązuje (wybór wykonawczy), z uzupełnieniem z 2026-10-06 na końcu: po obejrzeniu zrzutów wartość startowa `puddleReflectivity` wynosi 0,5, a kolor wody jest jaśniejszy.
Kod: [`src/game/EnvironmentMapping.hpp`](../../src/game/EnvironmentMapping.hpp) (`EnvironmentSettings`: `puddleReflectivity`, `crystalGlowShare`, `crystalStrength`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawReflections`, `PUDDLE_SPECULAR_STRENGTH`, `PUDDLE_SHININESS`), [`assets/shaders/reflect.frag`](../../assets/shaders/reflect.frag), [`tests/EnvironmentMappingTests.cpp`](../../tests/EnvironmentMappingTests.cpp). Dokument modułu: [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcje 2.7 i 2.9.

## 1. Kontekst

**Decyzja właściciela projektu (2026-10-06), w całości:** environment mapping jest pokazany na kryształach i w kałużach. Kryształy odbijają i załamują teksturę sześcienną nocnego nieba, a suwak daje wybór między jednym a drugim. Płaskie kałuże w niektórych komórkach korytarzy odbijają to samo niebo.

Efekt ma być **zobaczony**: na obronie i na zrzucie ekranu. Fizyka mu w tym przeszkadza z dwóch powodów:

- **Niebo jest ciemne.** Zenit razy jasność nieba 2,2 to około 0,003 (policzone z [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), sekcja 5.6). Jasna jest tylko tarcza księżyca (około 1,3 do 1,6) i najjaśniejsze gwiazdy.
- **Prawdziwa woda odbija mało.** Dla współczynnika załamania wody 1,33 przybliżenie Schlicka daje `F0 = ((1,33 - 1) / (1,33 + 1))^2 = 0,020`. Ten rachunek **nie jest w kodzie**: kod ma tylko liczbę 0,02 w komentarzu i w teście. Tabela dla oczu 1,7 m nad gruntem (model z testu `why the default reflectivity of a puddle is far above the one of real water`): kałuża 2 m dalej odbija 2,5 procent ciemnego nieba, 4 m dalej 10 procent, 8 m dalej 33 procent.

## 2. Decyzja

Wartości startowe są wybrane **dla widoczności**:

- `puddleReflectivity` = **0,35** zamiast 0,02 (kałuża 2 m dalej odbija 35 procent, 8 m dalej 55 procent, 16 m dalej 72 procent, policzone ze wzoru Schlicka). **Od uzupełnienia z 2026-10-06 wartość startowa to 0,5** (niżej).
- Odbłysk kałuży jest ostry i mocny (`PUDDLE_SPECULAR_STRENGTH` 1,0, `PUDDLE_SHININESS` 128), a nie słaby i szeroki jak kamień ścian.
- Kryształ ma **stały** udział nieba 0,5 bez efektu Fresnela, a jego własne świecenie zostaje **całe** (`crystalGlowShare` równe dokładnie 1,0, test tego pilnuje), żeby bloom nadal go znajdował.
- Suwaki pozwalają zejść do wartości fizycznych: `Reflectivity` od 0 (w tym 0,02) i `Glow` od 0.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **wartości widoczne, suwaki do zejścia niżej (wybrane)** | efekt jest widoczny od pierwszej klatki. Fizyczne wartości są o jeden ruch suwaka dalej, więc można je pokazać jako kontrast | kałuża wygląda bardziej jak lustro niż jak woda. `Sky share` 0,5 zmniejsza o połowę część oświetloną kryształu (nikt nie sprawdził, czy to przeszkadza) |
| wartości fizyczne (`F0` 0,02, Fresnel także dla kryształów) | zgodność z rzeczywistością i z tabelą w dokumencie | na ciemnym niebie kałuża w następnej komórce odbija 2,5 procent: prawie niewidoczna, a pokaz tematu 12 traci cel |
| podnieść jasność nieba tylko w odbiciu | zachowuje fizyczne `F0` | niebo w kałuży byłoby jaśniejsze niż niebo nad nią: księżyc w kałuży nie byłby tą samą tarczą. Kod mnoży odbicie przez tę samą `uSkyBrightness` co niebo |
| świecenie kryształu mieszane z niebem | prostszy wzór | `mix` obcinałby świecenie, a bloom straciłby źródło (notatka [`crystal-glow-raised-for-bloom.md`](crystal-glow-raised-for-bloom.md)) |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Jest to gra ze wskazanym pokazem, nie symulator wody. Odbicie ma być czytelne, a fizykę prowadzący zobaczy w tabeli i na suwaku. Najważniejsze liczby są w teście, więc decyzja jest sprawdzalna: test `why the default reflectivity of a puddle is far above the one of real water` pilnuje, że dla `F0` 0,02 odbicie jest małe w 2 i 4 m, a dla wartości startowej duże.

**Skutki, które przyjmuję.**

- **Przy świeceniu 1,0 niebo na kryształach jest słabą domieszką** (komentarz przy `crystalGlowShare` mówi to samo). Aby je zobaczyć wyraźnie, trzeba zmniejszyć `Glow`, a to zabiera także halo bloomu. Policzone, nikt nie oglądał.
- Wartości startowe nie są "prawdziwe": w dokumencie piszę to wprost, w tabeli sekcji 2.9.
- Wyglądu nikt nie sprawdził: może się okazać, że 0,35 to za dużo albo za mało.

## 5. Kiedy wrócić do tej decyzji

- Gdy po obejrzeniu kałuże wyglądają jak lustro w podłodze: obniżyć `puddleReflectivity` (na przykład do 0,2).
- Gdy kryształy po `Sky share` 0,5 są za ciemne: obniżyć udział albo dodać światło własne.
- Gdy prowadzący zapyta o fizykę: pokazać `Reflectivity` na 0,02 i tabelę.

## Uzupełnienie 2026-10-06: po obejrzeniu zrzutów woda jest jaśniejsza i mocniej odbija

**Co zobaczył agent** (pierwsze zrzuty ekranu, wersja Release z commitu `9a33f18`; "widziane na zrzucie ekranu przez agenta, nie przez właściciela"): kałuże czytały się jako **prawie czarne dziury** w wiązce latarki i nie były widoczne z wysokości chodzenia w 4 m i 8 m. Ten sam agent widział w kałuży tarczę księżyca i gwiazdy przy `Yaw` 205 i `Pitch` -50, więc odbicie samo działało. **Nie oceniono** różnicy między `Fresnel` włączonym i wyłączonym.

**Decyzja właściciela projektu (2026-10-06), w całości:** woda ma być lepiej widoczna: jaśniejszy odcień, mocniej odbijająca, miękki brzeg, więcej narożników. (Dwie pozostałe decyzje tego dnia: [`puddles-follow-the-ground.md`](puddles-follow-the-ground.md) i [`hud-at-top-edge-when-panels-hidden.md`](hud-at-top-edge-when-panels-hidden.md).)

**Jak to jest w kodzie.**

| Co | Przed | Po | Skąd |
|---|---|---|---|
| kolor wody (`PUDDLE_COLOR`, sRGB) | `(0,07; 0,09; 0,11)` | `(0,32; 0,40; 0,50)` | `PuddleRenderer.cpp` |
| `puddleReflectivity` (`F0`) | 0,35 | 0,5 | `EnvironmentMapping.hpp` |
| brzeg | twardy | miękki: zewnętrzne 45 procent promienia znika (`PUDDLE_RIM_FADE`) | `reflect.frag`, `Puddles.hpp` |
| narożniki brzegu | 16 | 32, w 6 pierścieniach | `Puddles.hpp` |

Przy `F0` 0,5 kałuża 2 m dalej odbija 50 procent, 8 m dalej 66, 16 m dalej 79 (policzone ze wzoru Schlicka, tabela w sekcji 2.9 dokumentu modułu). Komentarz przy stałej koloru mówi wprost, że pierwsza wartość dawała czarną dziurę w wiązce latarki.

**Wybór wykonawczy, którego nie ma na liście właściciela: środek kałuży przepuszcza grunt.** Autor poprawek dodał przezroczystość: krycie wody w środku to `mix(0,7; 1; F) * (1 - zanik brzegu)` (`PUDDLE_OPACITY` 0,7), więc przy ustawieniach startowych grunt prześwituje w około 15 procentach prosto w dół (krycie 0,85) i coraz mniej dalej. Powód z komentarzy kodu: cienka warstwa wody pokazuje ziemię i kamyki pod sobą, a to rozbija wygląd czarnej dziury. To jest **jego wybór, poza decyzją właściciela**, i można go cofnąć jedną stałą (`PUDDLE_OPACITY` 1 daje krycie 1).

**Co agent widział po zmianie** (autor poprawek, build jego worktree przed scaleniem z kodem dźwigni; nie właściciel): z wysokości chodzenia przy włączonej latarce w 2 m szaroniebieska mokra plama z kamykami widocznymi przez wodę, w 4 m mała niebieskawa plama z błyskiem gwiazdy, w 8 m kałuża nie do odróżnienia; przy wyłączonej latarce w cieniu księżyca tylko błysk gwiazdy w 2 m i 4 m; w świetle księżyca tarcza księżyca i gwiazdy w kałuży o miękkim brzegu. **Nie widziane:** widoki chodzenia w świetle księżyca w 2, 4 i 8 m, wygląd przy `Share of cells` 0,5, różnica `Fresnel` włączony i wyłączony. Poza wiązką latarki i dalej niż około 4 m kałuże pozostają trudne do zobaczenia.

**Kiedy wrócić do tego uzupełnienia.** Gdy właściciel obejrzy grę i kałuża będzie za mało albo za bardzo widoczna: `PUDDLE_COLOR`, `PUDDLE_OPACITY` i `puddleReflectivity` to trzy stałe do zmiany. Gdy wyjdzie na jaw, że przezroczystość środka przeszkadza: `PUDDLE_OPACITY` na 1.
