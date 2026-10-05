# Trawa cieniowana na fragment normalną podłoża, jednym programem we wszystkich trybach

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/grass.frag`](../../assets/shaders/grass.frag) (`GRASS_NORMAL`, `uLit`, wywołanie `computeLighting`), [`src/game/GrassRenderer.cpp`](../../src/game/GrassRenderer.cpp) (`NO_SPECULAR_STRENGTH`, `PLAIN_SHININESS`, `draw`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawGrass`, `m_lightRig.connect(m_grassShader)`). Dokument modułu: [`../modules/renderer/grass-geometry.md`](../modules/renderer/grass-geometry.md), sekcje 2.7 i 4.3.

## 1. Kontekst

Gra ma cztery tryby cieniowania (temat 7): `Unlit`, `Gouraud`, `Phong` i `Blinn-Phong`. Ściany, teren, kryształy i brama obsługują je trzema programami: `textured` bez światła, `gouraud` ze światłem liczonym w wierzchołkach i `lit` ze światłem liczonym na fragment.

Trawa powstaje inaczej niż wszystko inne: w buforze jest jeden punkt na kępkę, a wierzchołki źdźbeł tworzy shader geometrii. Źdźbło jest płaskim paskiem szerokim na 4 cm, widocznym z obu stron. Trzeba było zdecydować, jak je oświetlić i co zrobić z czterema trybami.

## 2. Decyzja

Trawa ma **jeden program** (`grass`) we wszystkich trybach. Światło jest liczone **na fragment**, funkcją `computeLighting` z tego samego pliku `common/lighting.glsl`, którego używają ściany, z **normalną stałą `(0, 1, 0)`** zamiast normalnej źdźbła i tylko z częścią rozproszoną. W trybie `Unlit` uniform `uLit` wyłącza światło i trawa ma pełną jasność. W trybie `Gouraud` trawa jest cieniowana tak samo jak w `Phong`.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Jeden program, światło na fragment, normalna w górę (wybrana)** | kępka ma jasność ziemi, na której rośnie, z każdej strony tak samo. Te same światła i ten sam plik co ściany, więc latarka i kryształy działają bez nowego kodu. Jeden zestaw trzech plików | w trybie `Gouraud` trawa nie pokazuje cieniowania na wierzchołek. Źdźbła nie mają własnego kształtu w świetle: nie widać, że są płaskie. Światło padające nisko z boku oświetla trawę tak jak ziemię, a nie jak pionowe źdźbło |
| Prawdziwa normalna źdźbła | bliższe fizyce: źdźbło zwrócone do światła jest jaśniejsze | normalna wskazuje w bok, więc źdźbło jest jasne z jednej strony i czarne z drugiej. Trzy źdźbła kępki mają trzy jasności, a przy ruchu kamery i wiatru całość miga. Trzeba by odwracać normalną według `gl_FrontFacing` |
| Trzy programy trawy, jak dla ścian | trawa dosłownie w każdym trybie | dziewięć plików shaderów zamiast trzech. Wersja Gouraud liczyłaby światło w shaderze geometrii dla każdego z 15 wierzchołków kępki |
| Trawa bez światła, sam gradient koloru | najprostszy shader | w scenie nocnej trawa świeciłaby pełną zielenią w ciemnych korytarzach |
| Światło liczone raz na kępkę w shaderze geometrii i przekazane wszystkim wierzchołkom | jedno wywołanie `computeLighting` na kępkę zamiast na fragment | krawędź stożka latarki skakałaby z kępki na kępkę. Plik `lighting.glsl` musiałby być dołączony do etapu geometrii |

## 4. Uzasadnienie i skutki

**Dlaczego normalna w górę.** Trawę ogląda się z wysokości oczu, z kilku metrów. Z tej odległości kępka czyta się jako plama zieleni na ziemi, a nie jako trzy płaskie paski, więc ma wyglądać jak część podłoża. Normalna podłoża daje dokładnie to, i przy okazji usuwa pytanie o stronę źdźbła: wynik nie zależy od tego, którą stronę widać, co pozwala rysować bez odrzucania tylnych ścian i bez `gl_FrontFacing`.

**Dlaczego jeden program.** Tryb `Gouraud` istnieje po to, żeby pokazać różnicę między światłem na wierzchołek i na fragment na dużych trójkątach. Źdźbło nie jest dobrym obiektem do tego pokazu, a kępka nie ma w buforze wierzchołków, w których światło dałoby się policzyć przed shaderem geometrii.

**Dlaczego tylko część rozproszona.** Trawa nie błyszczy. `lighting.glsl` liczy odbłysk zawsze, więc `GrassRenderer` ustawia mu siłę 0 i wykładnik 1, wartości, z którymi rachunek jest bezpieczny, a `grass.frag` czyta samo pole `diffuse`.

**Co przez to tracę.**

- Przełączenie na `Gouraud` nie zmienia wyglądu trawy. Na obronie trzeba to powiedzieć, zanim ktoś zapyta.
- Trawa nie reaguje na mapy normalnych ani na ustawienia odbłysku z panelu Renderer.
- Każdy fragment trawy liczy pełne oświetlenie ze wszystkich świateł.

## 5. Kiedy wrócić do tej decyzji

- Gdy trawa urośnie do rozmiarów, przy których widać pojedyncze źdźbła z bliska (wysoka trawa, kamera przy ziemi).
- W M7, razem z cieniami: trawa bez cienia ściany będzie wtedy jaśniejsza niż ziemia pod nią, i sposób oświetlenia trzeba będzie przejrzeć razem z odczytem mapy cieni.
- Gdy koszt fragmentów trawy okaże się mierzalny: wtedy światło raz na kępkę albo raz na wierzchołek staje się warte swoich wad.
- Gdy dojdą iskry wokół kryształów (druga część tematu 9 w PRD, niezbudowana): drugi program z shaderem geometrii może chcieć wspólnego kodu światła.
