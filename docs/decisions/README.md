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

| Notatka | Decyzja | Kod, którego dotyczy | Dokument modułu |
|---|---|---|---|
| [`collision-aabb-sliding.md`](collision-aabb-sliding.md) | kolizje to własne pudełka AABB i ruch oś po osi ze ślizganiem, bez silnika fizyki | [`src/scene/Collider.*`](../../src/scene/) | [`../modules/scene/collision.md`](../modules/scene/collision.md) |
| [`deterministic-random.md`](deterministic-random.md) | losowość z `std::mt19937` i własnej funkcji `randomBelow`, bez rozkładów z biblioteki standardowej | [`src/game/MazeGenerator.*`](../../src/game/) | [`../modules/game/maze-generator.md`](../modules/game/maze-generator.md) |
| [`no-gamma-until-m7.md`](no-gamma-until-m7.md) | w M4 nie ma korekcji gamma ani tekstur sRGB: dojdą w M7 razem z framebufferem HDR | [`assets/shaders/lit.frag`](../../assets/shaders/lit.frag), [`gouraud.frag`](../../assets/shaders/gouraud.frag), [`src/gfx/Texture2D.*`](../../src/gfx/) | [`../modules/scene/lights.md`](../modules/scene/lights.md), [`../modules/renderer/lighting-gouraud-phong.md`](../modules/renderer/lighting-gouraud-phong.md) |
| [`dead-end-lights.md`](dead-end-lights.md) | światła punktowe wiszą w ślepych zaułkach labiryntu (bez losowania, najwyżej 16), dopóki w M5 nie powstaną kryształy | [`src/game/Lighting.*`](../../src/game/), [`src/game/MazeWorld.cpp`](../../src/game/MazeWorld.cpp) | [`../modules/game/flashlight.md`](../modules/game/flashlight.md) |

## Jak dodać notatkę

1. Nowy plik `docs/decisions/<temat>.md`, nazwa małymi literami z łącznikami.
2. Pięć części z listy wyżej. Liczby i wyniki pomiarów tylko takie, które da się wskazać w kodzie, w testach albo w dokumencie modułu.
3. Wiersz w tabeli "Lista notatek" w tym pliku i odnośnik z dokumentu modułu (sekcja 10, "Źródła").
