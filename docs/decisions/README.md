# Notatki o decyzjach

Krótkie notatki "dlaczego tak, a nie inaczej". PRD (sekcja 7) przewiduje ten katalog jako miejsce na uzasadnienia wyborów, o które prowadzący może zapytać na obronie: dlaczego własny kod zamiast gotowej biblioteki, dlaczego prostsze rozwiązanie zamiast pełnego.

## Czym notatka różni się od dokumentu modułu

| | Dokument modułu (`modules/`) | Notatka o decyzji (`decisions/`) |
|---|---|---|
| odpowiada na pytanie | jak to działa i jak to jest napisane | dlaczego wybrałem to, a nie coś innego |
| długość | pełne dziesięć sekcji | jedna strona |
| zmienia się | z każdą zmianą kodu | tylko gdy zmienia się sama decyzja |
| zawiera kod | tak, linia po linii | nie, najwyżej nazwy plików i funkcji |

Notatka nie powtarza teorii ani kodu: odsyła do dokumentu modułu. Zapisuje to, czego w kodzie nie widać: jakie były inne możliwości i co przeważyło.

## Układ notatki

Każda notatka ma te same pięć części:

1. **Kontekst**: jaki problem trzeba było rozwiązać i jakie były ograniczenia.
2. **Decyzja**: co wybrałem, w jednym albo dwóch zdaniach.
3. **Rozważane możliwości**: tabela z zaletami i wadami każdej.
4. **Uzasadnienie i skutki**: dlaczego ta, co dzięki niej dostaję i co przez nią tracę.
5. **Kiedy wrócić do tej decyzji**: co musiałoby się zmienić, żeby wybór przestał być dobry.

Na górze notatki stoi data i stan: `obowiązuje` albo `zastąpiona` (z odnośnikiem do nowej notatki). Notatek się nie usuwa: zastąpiona decyzja zostaje jako historia.

## Lista notatek

Jedenaście notatek. Pierwszych pięć powstało do M4, cztery następne zapisują decyzje z M5, a dwie ostatnie decyzje z pierwszej części M6, skyboxa (wszystkie 2026-10-05).

| Notatka | Stan | Decyzja | Kod, którego dotyczy | Dokument modułu |
|---|---|---|---|---|
| [`collision-aabb-sliding.md`](collision-aabb-sliding.md) | obowiązuje | kolizje to własne pudełka AABB i ruch oś po osi ze ślizganiem, bez silnika fizyki. Kule służą tylko do testu nakładania | [`src/scene/Collider.*`](../../src/scene/) | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| [`deterministic-random.md`](deterministic-random.md) | obowiązuje | losowość z `std::mt19937` i własnej funkcji `randomBelow`, bez rozkładów i algorytmów losowych z biblioteki standardowej | [`src/game/MazeGenerator.*`](../../src/game/), [`src/game/Crystals.cpp`](../../src/game/Crystals.cpp) | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| [`no-gamma-until-m7.md`](no-gamma-until-m7.md) | obowiązuje | do M7 nie ma korekcji gamma ani tekstur sRGB: dojdą razem z framebufferem HDR | [`assets/shaders/lit.frag`](../../assets/shaders/lit.frag), [`gouraud.frag`](../../assets/shaders/gouraud.frag), [`src/gfx/Texture2D.*`](../../src/gfx/) | [`../modules/scene/lights.md`](../modules/scene/lights.md), [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| [`tangents-on-load.md`](tangents-on-load.md) | obowiązuje | styczne do map normalnych są liczone przy wczytaniu modelu i tam ortogonalizowane (Gram-Schmidt na procesorze), wierzchołek przechowuje samą styczną bez znaku skrętności, a bitangentę liczy shader | [`src/assets/Tangents.*`](../../src/assets/), [`src/gfx/Vertex.hpp`](../../src/gfx/Vertex.hpp), [`assets/shaders/common/normal_map.glsl`](../../assets/shaders/common/normal_map.glsl) | [`../modules/gfx/normal-mapping.md`](../modules/gfx/normal-mapping.md) |
| [`dead-end-lights.md`](dead-end-lights.md) | zastąpiona 2026-10-05 przez [`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md) | (historia, M4) światła punktowe wisiały w ślepych zaułkach labiryntu, bez losowania, najwyżej 16, dopóki nie powstały kryształy | kod usunięty w M5. Zostało `isDeadEnd` w [`src/game/Maze.*`](../../src/game/) | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |
| [`crystal-count-and-gate-threshold.md`](crystal-count-and-gate-threshold.md) | obowiązuje | jeden kryształ na 8 komórek labiryntu, od 1 do 16 (tyle świateł mieści shader). Brama otwiera się przy 70 procentach, zaokrąglone w górę, próg do zmiany suwakiem `Crystals needed` | [`src/game/Crystals.*`](../../src/game/), [`src/game/Round.*`](../../src/game/) | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| [`exit-farthest-cell.md`](exit-farthest-cell.md) | obowiązuje | wyjściem jest komórka najdalsza od startu w liczbie przejść (BFS), a nie przeciwległy narożnik. W labiryncie doskonałym to zawsze ślepy zaułek, więc zamyka ją jedna brama | [`src/game/Exit.*`](../../src/game/), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| [`battery-darkness-no-loss.md`](battery-darkness-no-loss.md) | obowiązuje | pusta bateria tylko gasi latarkę: runda trwa dalej, stanu przegranej nie ma | [`src/game/Round.*`](../../src/game/) | [`../modules/game/gameplay.md`](../modules/game/gameplay.md) |
| [`enemy-after-m5.md`](enemy-after-m5.md) | obowiązuje | przeciwnik, który goni gracza, jest w planie, ale po M5. Nie ma dla niego kodu ani projektu | brak | brak |
| [`skybox-in-game-layer.md`](skybox-in-game-layer.md) | obowiązuje | niebo rysuje klasa `game::Skybox` w `src/game/`, obok pozostałych klas rysujących, z programem jako polem `NightMazeApp`. Warstwy `src/renderer/` i klasy `SkyboxPass` z PRD nie ma, dopóki klatka jest jednym przebiegiem | [`src/game/Skybox.*`](../../src/game/), [`src/gfx/Cubemap.*`](../../src/gfx/), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md), [`../modules/renderer/README.md`](../modules/renderer/README.md) |
| [`painted-moon-fixed-direction.md`](painted-moon-fixed-direction.md) | obowiązuje | tarcza księżyca jest częścią obrazu nieba i stoi w kierunku przeciwnym do domyślnego światła księżyca. Dwa kąty są zapisane drugi raz w skrypcie, zgodności pilnuje test, a suwaki panelu Lights tarczy nie ruszają | [`tools/blender/make_skybox.py`](../../tools/blender/make_skybox.py), [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp), [`tests/SkyboxTests.cpp`](../../tests/SkyboxTests.cpp) | [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md) |

## Jak dodać notatkę

1. Nowy plik `docs/decisions/<temat>.md`, nazwa małymi literami z łącznikami.
2. Pięć części z listy wyżej. Liczby i wyniki pomiarów tylko takie, które da się wskazać w kodzie, w testach albo w dokumencie modułu.
3. Wiersz w tabeli "Lista notatek" w tym pliku i odnośnik z dokumentu modułu (sekcja 10, "Źródła").
