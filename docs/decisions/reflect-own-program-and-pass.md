# Kryształy i kałuże rysuje osobny program `reflect` w osobnym przebiegu, a nie gałąź w `lit.frag`

Data: 2026-10-06. Stan: obowiązuje (wybór wykonawczy).
Kod: [`assets/shaders/reflect.vert`](../../assets/shaders/reflect.vert), [`assets/shaders/reflect.frag`](../../assets/shaders/reflect.frag), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawReflections`, `crystalsReflect`, `drawGateAndCrystals`), [`src/game/GameplayRenderer.cpp`](../../src/game/GameplayRenderer.cpp) (`drawGate`, `drawCrystals`). Dokument modułu: [`../modules/renderer/env-mapping.md`](../modules/renderer/env-mapping.md), sekcje 2.13, 3.3 i 5.7.

## 1. Kontekst

**Decyzja właściciela projektu (2026-10-06), w całości:** environment mapping jest pokazany na kryształach i w kałużach. Kryształy odbijają i załamują teksturę sześcienną nocnego nieba, a suwak daje wybór między jednym a drugim. Płaskie kałuże w niektórych komórkach korytarzy odbijają to samo niebo.

Ta notatka dotyczy czegoś, czego właściciel nie rozstrzygał: **którym programem to narysować**. Kryształy były rysowane razem ze ścianami tym samym programem i w tym samym przebiegu (`lit`, `gouraud` albo `textured`, zależnie od trybu oświetlenia i widoku). Odczyt tekstury sześciennej wymaga nowych uniformów (sampler sześcienny, jasność nieba, udział nieba, współczynnik załamania) i nowego wzoru na kolor.

## 2. Decyzja

Powstaje **czternasty program shaderów, `reflect`** (po jedenastu starych i dwóch programach minimapy), i osobny przebieg `drawReflections` po trawie i przed niebem. Rysuje kryształy i kałuże. Gdy efekt jest wyłączony, widok diagnostyczny albo gdy program się nie skompilował, kryształy wracają do programu ścian, a `drawGateAndCrystals` pilnuje, żeby każdy kryształ był narysowany **dokładnie raz** w klatce.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **osobny program i przebieg (wybrane)** | `lit.frag`, `gouraud.frag` i `textured.frag` zostają bez zmian i bez nowych uniformów. Efekt wyłączony oznacza stary kod ścieżką ścian. Nowy program ma własny panel w liście Shaders i własne przeładowanie | jeden dodatkowy program i przebieg. Blok oświetlenia z `lit.frag` jest w `reflect.frag` powtórzony (te same funkcje z plików dołączanych, ale ta sama kolejność rachunków w `main`). Tryb Gouraud i widoki diagnostyczne nie obejmują kryształów (sekcja 4) |
| gałąź w `lit.frag`, `gouraud.frag` i `textured.frag` | jedna ścieżka rysowania, jeden przebieg, żadnych różnic między trybami | trzy shadery dostają sampler sześcienny, pięć uniformów i gałąź, którą wszystkie ściany wykonują bez potrzeby. Program `gouraud` (światło na wierzchołek) musiałby i tak czytać niebo na fragment. Kałuży nie da się zrobić jako ściany, więc rysowanie i tak by się rozdzieliło |
| odbicie jako efekt po scenie (w przebiegu składającym) | żaden shader sceny się nie zmienia | przebieg składający zna tylko kolor i głębię piksela, nie jego normalną ani to, czy to kryształ. Kierunku odbicia z tego nie odtworzy |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Efekt dotyczy dwóch rodzajów obiektów (kryształów i kałuż), a nie wszystkich powierzchni. Osobny program trzyma nowy kod w jednym miejscu i daje prosty przełącznik: wyłączony efekt to dokładnie poprzedni obraz.

**Skutki, które przyjmuję i które wynikają z kodu** (**żaden z tych przypadków poza pierwszym nie był jeszcze uruchomiony**):

- **Blinn-Phong i Phong** (`Textured`): kryształ i kałuża w programie `reflect`, światło na fragment, odbłysk Blinna-Phonga albo Phonga, mapa normalnych kryształu, jeśli jest włączona. Uruchomiona była tylko ścieżka Blinna-Phonga.
- **Gouraud:** kryształy i kałuże mają światło liczone **na fragment** z odbłyskiem Phonga i bez mapy normalnych, a ściany dalej na wierzchołek. Przełącznik trybu z pokazu tematu 7 nie zmienia więc światła kryształów przy włączonym efekcie. Aby pokazać Gouraud na kryształach, wyłączam efekt.
- **Unlit:** program `reflect` dostaje `uLit = 0` i pokazuje powierzchnię z pełną jasnością, **a niebo nadal jest domieszane**.
- **`Normals as colour` i `UVs as colour`:** kryształ jest rysowany programem `textured` razem ze ścianami (dane, nie niebo), a kałuże tym samym programem po przebiegu ścian, gdy efekt i pole `Puddles` są włączone.
- **Efekt wyłączony:** kryształy jak przed M8, kałuż nie ma w żadnym widoku.
- **Program `reflect` nie skompilował się:** kryształy wracają do programu ścian, kałuże nie są rysowane w widoku `Textured`, ale **są** rysowane w widokach diagnostycznych (gałąź tych widoków jest przed sprawdzeniem ważności programu). To zachowanie wynika z kolejności warunków, nie z osobnej decyzji.
- Blok świateł jest teraz podłączony do czterech programów (`lit`, `gouraud`, `grass`, `reflect`), a uniformy cieni ustawia `setShadowUniformsOf` także dla `reflect`.
- `GameplayRenderer::draw` rozpadło się na `drawGate` i `drawCrystals` (stara funkcja woła obie, bo przebieg cieni rysuje kryształy programem głębi).

**Czego nie zmierzyłem.** Kosztu dodatkowego programu i przebiegu w czasie klatki nikt nie zmierzył.

## 5. Kiedy wrócić do tej decyzji

- Gdy kryształy mają się różnić między trybami oświetlenia (prowadzący chce zobaczyć Gouraud na kryształach przy włączonym efekcie): potrzebny byłby wariant `reflect` ze światłem na wierzchołek albo gałąź w `gouraud.*`.
- Gdy powierzchni pokazujących niebo przybędzie (na przykład ściany): wtedy gałąź w `lit.frag` zacznie być tańsza niż drugi program.
- Gdy powtórzony blok oświetlenia w `reflect.frag` zacznie się rozjeżdżać z `lit.frag`: wspólna funkcja w pliku dołączanym zamiast dwóch kopii `main`.
