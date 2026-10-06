# Mapa cieni księżyca: pudełko światła dopasowane do terenu, nie do kamery

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/scene/LightSpace.cpp`](../../src/scene/LightSpace.cpp) (`directionalLightSpace`, `LIGHT_BOX_MARGIN`), [`src/game/Shadows.cpp`](../../src/game/Shadows.cpp) (`shadowCasterBounds`, `shadowTexelSize`), [`src/game/NightMazeApp.cpp`](../../src/game/NightMazeApp.cpp) (`drawMoonShadowMap`), [`tests/ShadowTests.cpp`](../../tests/ShadowTests.cpp). Dokument modułu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcje 2.3 i 2.4.

## 1. Kontekst

Księżyc jest światłem kierunkowym, więc jego mapa cieni jest rysowana rzutem ortograficznym: bryłą widzenia jest pudełko. Trzeba zdecydować, **co to pudełko obejmuje**. Od tego zależą dwie rzeczy naraz: jak drobne są teksele mapy na ziemi (czyli jak ostre są krawędzie cieni) i czy cienie stoją w miejscu, gdy gracz chodzi.

Scena jest mała i zamknięta. Teren labiryntu startowego ma 48 m na 48 m, a wszystko, co rzuca cień, stoi na nim i nie jest wyższe niż słupek (3,15 m). Ściana ma 0,2 m grubości.

## 2. Decyzja

Pudełko światła obejmuje **cały teren**, od najniższego gruntu do wysokości słupka nad najwyższym gruntem, z marginesem 0,5 m z każdej strony. Jest liczone od nowa **co klatkę**, wyłącznie z terenu i z dwóch kątów księżyca. Kamera nie bierze w tym udziału.

## 3. Rozważane możliwości

Teksele policzone dla labiryntu startowego i księżyca startowego (yaw 25, pitch -50).

| Możliwość | Teksel przy mapie 2048 | Cienie przy ruchu gracza | Zalety | Wady |
|---|---|---|---|---|
| **pudełko wokół terenu (wybrane)** | 3,2 cm (pudełko 64,8 x 54,1 m) | stoją nieruchomo | jedna funkcja bez stanu. Mapa pokrywa co klatkę ten sam grunt. Działa dla każdego labiryntu, skali wysokości i kąta księżyca bez dodatkowego kodu | około połowy mapy pokrywa wzgórza poza labiryntem. Teksel rośnie razem z terenem |
| pudełko wokół samego labiryntu | około 1,3 cm (pudełko szerokości około 27,6 m) | stoją nieruchomo | drobniejsze cienie w labiryncie | wzgórza przestają rzucać i przyjmować cień, a na granicy pudełka cień urywa się w pół zbocza |
| pudełko wokół bryły widzenia kamery | zależy od zasięgu widoku, przy krótkim dużo mniejszy | **migoczą**, jeśli pudełka nie przyciągnąć do siatki tekseli | rozdzielczość tam, gdzie patrzy gracz | stabilizacja (stały rozmiar, przyciąganie do tekseli), rzucający spoza widoku trzeba doliczać osobno, więcej kodu do obrony |
| kaskady (kilka map o różnym zasięgu) | najlepszy z bliska | stabilne przy poprawnej stabilizacji | standard w dużych światach | kilka przebiegów głębi, wybór kaskady w shaderze, szwy między kaskadami. Scena 48 m tego nie potrzebuje |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Przy terenie 48 m mapa 2048 daje teksel 3,2 cm i ścianę grubości ponad 6 tekseli: to wystarcza, co przypina test `the shadow map of the moon is fine enough for the walls of the default maze`. Druga rzecz jest ważniejsza na pokazie: cień, który nie zależy od kamery, **nie migocze**. W nocnej scenie z ostrym kontrastem migotanie krawędzi byłoby pierwszą rzeczą, którą widać.

**Dlaczego co klatkę.** Policzenie pudełka to osiem mnożeń macierzy przez wektor i jedno `lookAt`. Dzięki temu nie ma żadnego miejsca, w którym trzeba pamiętać o odświeżeniu: nowy labirynt, zmiana skali wysokości terenu i suwaki księżyca w zakładce Light / Lights działają same.

**Skutki, które przyjmuję.**

- Rozdzielczość jest rozłożona równo: tyle samo tekseli dostaje metr pod nogami gracza i metr dalekiego wzgórza.
- Labirynt dużo większy niż startowy dostanie grubsze teksele. Lista `Resolution` ma tylko 1024 i 2048.
- Góra pudełka stoi wyżej, niż stoi cokolwiek (słupek na szczycie najwyższego wzgórza nie istnieje). Kosztuje to trochę zakresu głębi, którego przy 24 bitach i tak jest w nadmiarze.
- Pudełko ma nierówne boki, a mapa jest kwadratem tekseli, więc teksele nie są kwadratowe.

**Czego nie zmierzyłem.** Rozmiary pudełka i tekseli są policzone ze wzorów (szerokość i teksel potwierdza test). Wersji z pudełkiem wokół kamery nikt nie napisał, więc "migocze" opisuje znaną własność tej metody, a nie obserwację z tej gry.

## 5. Kiedy wrócić do tej decyzji

- Gdy teren urośnie na tyle, że teksel przy mapie 2048 przekroczy grubość ściany podzieloną przez dwa albo trzy.
- Gdy gra dostanie drobne rzucające (mniejsze niż kilka centymetrów), których cień ma być czytelny z bliska.
- Gdy pomiar pokaże, że przebieg głębi dla całego terenu jest za drogi i warto rysować tylko to, co widać.
