# Bateria: pusta oznacza ciemność, a nie przegraną

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Round.hpp`](../../src/game/Round.hpp), [`Round.cpp`](../../src/game/Round.cpp) (`RoundState`, `updateRound`, `flashlightFlicker`, `lightingForFrame`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`beginRound`), [`src/debug/Hud.cpp`](../../src/debug/Hud.cpp) (podpowiedź na ekranie), testy w [`tests/RoundTests.cpp`](../../tests/RoundTests.cpp). Dokument modułu: [`../modules/game/gameplay.md`](../modules/game/gameplay.md), sekcje 2 i 5.

## 1. Kontekst

Latarka ma baterię. PRD (sekcja 2) opisuje ją tak: "bateria spada w czasie, migotanie przy niskim stanie", a pętlę rozgrywki kończy zdaniem "dotrzyj do wyjścia, zanim bateria się skończy". W wizji stoi też, że kryształy są "miejscami ładowania baterii, więc ciemność jest głównym przeciwnikiem".

PRD nie mówi, co dokładnie dzieje się, gdy bateria spadnie do zera. Zdanie o wyjściu sugeruje limit czasu, ale ekranu przegranej PRD nie opisuje (jest tylko opcjonalny ekran końca rundy z czasem i liczbą kryształów). W M5 trzeba było to rozstrzygnąć, bo od tego zależy lista stanów rundy.

Ograniczenia: runda ma dać się przejść na obronie w kilka minut i nie może się skończyć w połowie pokazu, a każdy stan gry to kod, testy i ekran do wyjaśnienia.

## 2. Decyzja

Pusta bateria tylko gasi latarkę. Runda trwa dalej, stanu przegranej nie ma. Latarka wraca, gdy gracz zbierze kryształ. To odejście od zdania z PRD "zanim bateria się skończy": bateria nie jest limitem czasu rundy, tylko zasobem światła.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Ciemność bez przegranej (wybrana)** | dwa stany rundy zamiast trzech. Kara jest widoczna od razu i wynika wprost z tematu gry (światło). Gracz zawsze może się uratować, bo kryształy ładują baterię i same świecą. Pokaz na obronie nie urwie się sam | bateria nie wymusza pośpiechu. Rundy nie da się przegrać, więc czas na ekranie jest jedyną miarą wyniku. Inaczej niż w zdaniu z PRD |
| Stan przegranej i restart, gdy bateria spadnie do zera | zgodne z dosłownym odczytaniem PRD. Wyraźna stawka | trzeci stan rundy, ekran przegranej i jego testy. Gracz, który zgubił się w dużym labiryncie, przegrywa bez własnego błędu w ostatniej chwili. Pokaz paneli przy włączonej latarce musiałby pilnować zegara |
| Odliczanie po zgaśnięciu: przegrana dopiero po kilkunastu sekundach ciemności | daje szansę na dojście do kryształu | dwa zegary zamiast jednego, czwarta rzecz do pokazania na ekranie i dodatkowa liczba do strojenia. Nadal stan przegranej ze wszystkimi kosztami |
| Minimalna poświata: latarka nigdy nie gaśnie do końca | gracz nigdy nie zostaje w ciemności | pusta bateria przestaje cokolwiek znaczyć, a migotanie przy niskim stanie niczego nie zapowiada. Temat stożka światła traci najlepszy kontrast: scena z latarką i bez niej |

## 4. Uzasadnienie i skutki

**Dlaczego ciemność wystarcza jako kara.** Bez latarki zostają światło otoczenia, księżyc i światła punktowe nad kryształami, które jeszcze wiszą. Korytarz bez kryształu jest prawie czarny, ale kryształ widać z daleka, więc gracz ma dokąd iść. To jest dokładnie pętla z PRD ("zauważ poświatę kryształu, zbierz go"), tylko bez ekranu porażki.

**Dlaczego nie stan przegranej.** Dochodziłby trzeci stan, jego ekran i przejścia, a zysk dla tematów wykładu jest zerowy. Dla obrony to wręcz ryzyko: pełna bateria starcza na 180 s świecenia, a pokaz paneli z włączoną latarką może potrwać dłużej.

**Co jest w kodzie.**

- `RoundState` ma dwie wartości: `Playing` i `Won`. Komentarz przy typie mówi wprost, że "lost" nie ma.
- Bateria spada tylko wtedy, gdy latarka świeci, i tylko w stanie `Playing`. Pełna starcza na `batteryLifetimeSeconds` (180 s).
- Gdy bateria jest pusta, `updateRound` ustawia przełącznik latarki na fałsz w każdym kroku. Klawisz F i pole wyboru w zakładce Light / Lights nie utrzymają jej włączonej. `lightingForFrame` dodatkowo wyłącza latarkę w ustawieniach klatki.
- Zebrany kryształ oddaje `batteryPerCrystal` (0,25) baterii. Sprawdzenie pustej baterii stoi po zbieraniu, więc kryształ zebrany w tym samym kroku, w którym bateria się skończyła, ratuje światło (jest na to test).
- Poniżej `lowBatteryThreshold` (0,2) latarka migocze, coraz głębiej: to zapowiedź ciemności.
- HUD pokazuje wtedy `Battery empty. Find a crystal.`, a pasek baterii robi się czerwony już poniżej progu.
- Nowa runda (klawisz R, przycisk w kategorii Gameplay, nowy labirynt) zaczyna z pełną baterią i włączoną latarką.

**Co przez to tracę.**

- Po zebraniu kryształ znika razem ze swoim światłem. Gracz, który zebrał wszystkie i wyczerpał baterię, zostaje z samym księżycem i światłem otoczenia do końca rundy. Brama jest wtedy już otwarta, ale wyjścia trzeba szukać po ciemku.
- Włączenie latarki po zebraniu kryształu jest ręczne (klawisz F): kod sam jej nie zapala.
- Nie ma presji czasu. Jedyną nagrodą za szybkość jest czas rundy na karcie `You escaped`.

**Czego nie sprawdziłem.** Reguły są przypięte testami w `tests/RoundTests.cpp`. Tego, jak ciemna jest scena bez latarki i czy da się w niej znaleźć drogę, nikt jeszcze nie ocenił w ręcznej grze.

## 5. Kiedy wrócić do tej decyzji

- Gdy ręczna gra pokaże, że ciemność nie jest żadną karą (księżyc oświetla za dużo) albo że jest karą nie do zniesienia (labirynt nie do przejścia). Najpierw stroiłbym światło otoczenia i księżyc, dopiero potem reguły.
- Gdy dojdzie przeciwnik ([`enemy-after-m5.md`](enemy-after-m5.md)): złapanie gracza to naturalny powód, żeby runda mogła się skończyć porażką. Wtedy stan przegranej wróci, ale z innego powodu niż bateria.
- Gdy dojdą cienie i mgła (M7): scena bez latarki będzie wyglądać inaczej i ocenę trzeba powtórzyć.
