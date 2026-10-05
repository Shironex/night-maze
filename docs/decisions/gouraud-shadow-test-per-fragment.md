# Cienie w trybie Gouraud: światło na wierzchołek, odczyt mapy cieni na fragment

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/gouraud.vert`](../../assets/shaders/gouraud.vert) (wyjścia `vMoonDiffuseLight`, `vMoonSpecularLight`, `vWorldPosition`, `vMoonFacing`), [`assets/shaders/gouraud.frag`](../../assets/shaders/gouraud.frag) (`moonShadow` i odejmowanie), [`assets/shaders/common/lighting.glsl`](../../assets/shaders/common/lighting.glsl) (pola `moonDiffuse`, `moonSpecular`). Dokumenty modułów: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.15, i [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md).

## 1. Kontekst

Tryb Gouraud istnieje w grze po to, żeby pokazać temat 7 wykładu: światło policzone w wierzchołkach i zmieszane w poprzek trójkąta, w porównaniu ze światłem liczonym dla każdego fragmentu. Cały sens trybu to zdanie "tu wszystko liczy się na wierzchołek".

Cień jest jednak innym rodzajem informacji niż światło. Światło zmienia się na ścianie łagodnie, więc mieszanie z rogów daje wynik podobny do prawdziwego. Cień ma **krawędź**, która przecina ścianę w dowolnym miejscu, a segment ściany ma cztery wierzchołki na jedną ścianę boczną.

## 2. Decyzja

W programie `gouraud` oświetlenie zostaje na wierzchołek, ale **odczyt mapy cieni jest wykonywany na fragment**. Shader wierzchołków przekazuje osobno udział księżyca w świetle, pozycję w świecie i cosinus do biasu, a shader fragmentów odejmuje udział księżyca pomnożony przez wynik odczytu.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **światło na wierzchołek, cień na fragment (wybrane)** | cienie mają te same krawędzie we wszystkich trybach. Różnica między Gouraud a Phong zostaje tym, czym była: miejscem liczenia światła | zdanie "wszystko na wierzchołek" ma wyjątek. Cztery dodatkowe zmienne przekazywane do shadera fragmentów |
| cień na wierzchołek | tryb czysty co do zasady | cztery odpowiedzi na ścianę: ściana cała jasna, cała ciemna albo w gradiencie od rogu do rogu, który nie przypomina cienia. Wynik zależy od gęstości siatki, nie od sceny |
| brak cieni w trybie Gouraud | brak wyjątku, brak dodatkowych zmiennych | przełączenie trybu zmienia dwie rzeczy naraz (światło i cienie), więc porównanie trybów przestaje pokazywać to, co ma pokazywać |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Pokaz tematu 7 polega na przełączaniu listy `Lighting` i patrzeniu, co się zmienia. Powinna zmieniać się jedna rzecz: kanciasty odbłysk w Gouraud przeciw gładkiemu w Phong. Gdyby razem z trybem znikały albo psuły się cienie, pokaz mieszałby dwa tematy.

**Jak to jest uczciwe.** Wyjątek jest zapisany wprost w komentarzach obu shaderów i w dokumencie modułu. Udział księżyca (`vMoonDiffuseLight`, `vMoonSpecularLight`) jest nadal liczony na wierzchołek i mieszany przez rasteryzer, więc **ilość** odejmowanego światła ma wszystkie cechy cieniowania Gouraud. Na fragment liczone jest tylko pytanie "czy tu dociera księżyc".

**Skutki, które przyjmuję.**

- Program `gouraud` wykonuje w shaderze fragmentów tyle samo odczytów mapy co program `lit`, więc tryb Gouraud nie jest już "prawie darmowy na fragment".
- `vMoonFacing` jest mieszany liniowo między wierzchołkami. Na płaskiej ścianie jest stały, na terenie zmienia się łagodnie, więc bias wychodzi prawie taki sam jak w trybie Phong.
- Wspólna funkcja `computeLighting` musi oddawać udział księżyca osobno. To rozwiązanie obsługuje oba programy jedną funkcją.

**Czego nie zmierzyłem.** Zgłoszone jest, że obraz bez cieni jest identyczny z obrazem sprzed zmiany. Porównania cieni w trybach Gouraud i Phong obok siebie nikt nie zgłosił: jest na liście testów ręcznych.

## 5. Kiedy wrócić do tej decyzji

- Gdyby prowadzący wymagał trybu Gouraud bez żadnej pracy na fragment: wtedy trzecia możliwość (brak cieni w tym trybie) jest zmianą jednej linii.
- Gdy dojdzie cień latarki: ten sam wzór trzeba będzie powtórzyć dla udziału reflektora.
