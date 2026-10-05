# Minimapa pokazuje tylko odkryte korytarze (planowane)

Data: 2026-10-05. Stan: obowiązuje jako decyzja, **kod nie jest jeszcze napisany**.
Kod, którego dotyczy: brak. Minimapy nie ma. Dokumenty: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md) (temat 10, rendering pozaekranowy, w trakcie), [`../syllabus.md`](../syllabus.md). Stan M7: [`../guides/m7-status.md`](../guides/m7-status.md).

## 1. Kontekst

PRD wymienia przy temacie 10 (rendering pozaekranowy) minimapę: widok labiryntu rysowany do osobnego framebuffera. To ostatnia brakująca rzecz tego tematu i szósta, ostatnia część M7.

Gra polega na szukaniu drogi w labiryncie. Minimapa, która od początku pokazuje cały labirynt z wyjściem, odpowiada na to pytanie za gracza.

## 2. Decyzja

Decyzja właściciela projektu z 2026-10-05: minimapa pokazuje **tylko korytarze, które gracz już odkrył**. Do tego przełącznik debugowania, który odsłania cały labirynt.

Uwaga o zakresie: treścią decyzji właściciela jest jedno zdanie z części 2. Kontekst, tabela możliwości i uzasadnienie niżej to moja analiza tej decyzji, nie jej część.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **tylko odkryte korytarze, z przełącznikiem debugowania (wybrane)** | minimapa pomaga wracać i nie błądzić w kółko, ale nie pokazuje drogi do wyjścia. Przełącznik pozwala pokazać na obronie cały obraz z framebuffera | potrzebny jest stan "które komórki są odkryte" i reguła, kiedy komórka się odkrywa |
| cały labirynt od początku | najprostsze: jeden widok z góry bez żadnego stanu | zabiera grze sens: wyjście i droga do niego są widoczne od pierwszej sekundy |
| brak minimapy | zero pracy | temat 10 zostaje bez elementu wymienionego w PRD |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Minimapa ma dwie role i ta decyzja je rozdziela. Dla gracza jest pamięcią przebytej drogi. Dla obrony jest pokazem renderingu do tekstury, i do tego służy przełącznik, który odsłania wszystko.

**Co już wiadomo z wcześniejszych części (ograniczenia, które minimapa odziedziczy).**

- Mgła liczy wysokość w miejscu piksela i z góry zakrywa labirynt prawie w całości ([`fog-height-at-the-pixel.md`](fog-height-at-the-pixel.md)). Widok minimapy nie może przejść przez przebieg składający z włączoną mgłą.
- Framebuffery, przebieg do tekstury i pokazywanie tekstury w ImGui są gotowe ([`../modules/gfx/framebuffers.md`](../modules/gfx/framebuffers.md)).

**Czego dziś nie wiadomo.** Reguła odkrywania (komórka, w której gracz stoi, czy także to, co widzi), sposób rysowania (osobny widok sceny z góry czy schemat z danych labiryntu), rozmiar i miejsce na ekranie nie są ustalone. Zostaną zapisane tutaj, gdy powstanie kod.

## 5. Kiedy wrócić do tej decyzji

- Przy pisaniu szóstej części M7: uzupełnić tę notatkę o regułę odkrywania i o to, jak minimapa jest rysowana.
- Gdyby testy z graczami pokazały, że bez widoku całości gra jest za trudna albo za długa.
