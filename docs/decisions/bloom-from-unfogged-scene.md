# Bloom liczony ze sceny bez mgły i dodawany po mgle

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (kolejność bloków w `main`: mgła, potem bloom), [`src/game/PostProcess.cpp`](../../src/game/PostProcess.cpp) (`drawBloom` czyta kolor sceny, `composite` miesza mgłę), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`onRender`: `drawBloom` przed `composite`). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcje 2.15 i 2.21.

## 1. Kontekst

Od drugiej części M7 poświata jest liczona z tekstury koloru sceny w osobnych przebiegach (`drawBloom`) i dodawana do sceny w przebiegu składającym. Trzecia część dodała mgłę w tym samym przebiegu składającym. Mgła nie jest przebiegiem z własnym celem: nigdy nie jest zapisywana do tekstury sceny, powstaje dopiero w drodze do okna.

Z tego wynikają dwa pytania naraz. Z czego liczyć poświatę: ze sceny czystej czy zamglonej? I w jakiej kolejności łączyć trzy rzeczy w jednym pikselu: scenę, mgłę i poświatę?

Rzeczą, która świeci najmocniej, jest kryształ, czyli cel gry. Gracz ma go widzieć z daleka.

## 2. Decyzja

Poświata jest liczona ze sceny **bez mgły**, a w przebiegu składającym mgła jest mieszana **przed** dodaniem poświaty: `color = mix(scena, mgła, amount)`, potem `color += bloom * intensity`. Poświata nie jest więc ani osłabiana przez mgłę, ani zastępowana jej kolorem.

## 3. Rozważane możliwości

Przykład: kryształ 30 m od oka, 95 procent mgły w pikselach jego bryły.

| Możliwość | Bryła kryształu | Poświata | Zalety | Wady |
|---|---|---|---|---|
| **bloom ze sceny bez mgły, dodany po mgle (wybrana)** | w 95 procentach kolor mgły | pełna, jak z bliska | kryształ świeci przez mgłę i jest widoczny z końca korytarza. Żadnego nowego przebiegu ani celu. `drawBloom` bez zmian | niefizyczne: przy bardzo gęstej mgle zostaje poświata bez widocznego źródła |
| bloom ze sceny bez mgły, dodany **przed** mgłą | jak wyżej | mieszana z mgłą według głębi piksela, na który padła | poświata słabnie z odległością | głębia pod poświatą jest głębią tego, co za nią leży, a nie źródła: poświata rozlana na bliską ścianę prawie nie słabnie, a ta sama poświata na tle dalekiej ściany znika. Obwódka nierówna i zależna od tła |
| mgła jako osobny przebieg do własnego celu, bloom liczony z obrazu zamglonego | jak wyżej | słabnie razem ze źródłem: zamglony kryształ spada pod próg i przestaje świecić | spójne: co ginie we mgle, nie świeci | jeden pełnoekranowy cel `GL_RGBA16F` i jeden przebieg więcej. Dalekie kryształy tracą poświatę zupełnie, czyli znika to, po co bloom powstał |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Poświata w tej grze nie jest ozdobą, tylko sygnałem: mówi, gdzie jest kryształ. Mgła, która ją gasi, odbierałaby graczowi informację dokładnie wtedy, gdy jej potrzebuje, czyli z daleka. Komentarz w shaderze nazywa zamiar wprost: blask kryształu ma świecić przez mgłę, a nie być zastępowany jej kolorem. Do tego wybrana wersja nie kosztuje nic: przebiegi bloomu z drugiej części zostały nietknięte, a w przebiegu składającym zdecydowała sama kolejność dwóch bloków.

**Co mówi na to fizyka.** Prawdziwa mgła osłabiłaby światło kryształu po drodze tak samo jak światło ściany. Za to sama rozświetliłaby się wokół niego: świecąca kula we mgle ma szeroką, miękką aureolę. Dodanie poświaty po mgle wygląda podobnie do tej aureoli, choć powstaje z innego powodu. Nie twierdzę, że to model fizyczny.

**Skutki, które przyjmuję.**

- Przy dużej gęstości (suwak `Density` w prawo) bryła dalekiego kryształu znika w całości, a poświata zostaje: plama światła bez źródła. Przy wartościach startowych jest na to mało miejsca: labirynt startowy ma 20 m boku, a na 20 metrach mgła zabiera 86 procent, więc bryłę kryształu w korytarzu jeszcze widać.
- To samo dotyczy tarczy księżyca: jej poświata nie słabnie we mgle. Przy `Height falloff` równym 0 niebo znika w mgle, a poświata księżyca nadal jest dodawana.
- Poświata kryształu zasłoniętego ścianą nie przechodzi przez ścianę: tego kryształu nie ma w obrazie sceny, więc nie ma z czego jej policzyć. Mgła tego nie zmienia.
- Obraz `Bright pass` i obraz `Bloom` w panelu Framebuffers wyglądają tak samo z mgłą i bez niej.

**Czego nie zmierzyłem.** Odrzuconych wersji nikt nie zbudował, a wybranej nikt nie oglądał z myszą w ręku: opis wyglądu wynika z kolejności linii w shaderze. Test ręczny ma punkt "kryształ świecący przez mgłę" ([`../guides/build-windows.md`](../guides/build-windows.md), sekcja 19).

## 5. Kiedy wrócić do tej decyzji

- Gdy poświaty bez źródła zaczną razić przy gęstszej mgle albo większym labiryncie: wtedy pomnożenie poświaty przez przepuszczalność mgły w miejscu źródła albo mgła jako osobny przebieg.
- Gdy dojdą inne jasne rzeczy, które **mają** ginąć we mgle (dalekie światła, ogień).
- Gdy mgła dostanie własne rozpraszanie światła (snop latarki): wtedy aureola wokół źródeł powinna wynikać z mgły, nie z bloomu.
