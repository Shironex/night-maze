# Przeciwnik: chcę go mieć, ale po M5

Data: 2026-10-05. Stan: obowiązuje; **dopisek z 2026-10-06:** część pytań z sekcji 5 jest rozstrzygnięta decyzją właściciela w [`enemy-light-shy-shadow.md`](enemy-light-shy-shadow.md) (przeciwnik rusza się tylko, gdy jest nieoświetlony i niewidziany; złapanie cofa na początek tego samego labiryntu bez ekranu przegranej; poziom Calm bez niego). Tabela tamtej notatki mówi, co zostaje otwarte.
Kod: brak. W repozytorium nie ma ani jednej linii przeciwnika. Dokument modułu: brak. Zakres M5 opisuje [`../modules/game/gameplay.md`](../modules/game/gameplay.md).

## 1. Kontekst

PRD wymienia przeciwnika dwa razy: jako mechanikę o priorytecie COULD (najniższym na liście) i jako "opcjonalny przeciwnik" w ostatnim kamieniu milowym, M9. W wizji stoi, że "ciemność jest głównym przeciwnikiem". Do zaliczenia przeciwnik nie jest potrzebny: żaden z 15 tematów wykładu go nie wymaga.

Przy zamykaniu zakresu M5 (kryształy, bateria, brama, wyjście) padło pytanie, czy dołożyć do niego coś, co gracza goni. Projekt ma dla mnie dwa cele: zaliczenie i kod, który pokażę w portfolio. Drugi cel sięga dalej niż plan wykładu.

## 2. Decyzja

Chcę, żeby w grze był przeciwnik, który goni gracza. Nie wchodzi do M5 i nie ma dla niego kodu ani projektu. M5 zostaje przy swoim zakresie z PRD: zbieranie, bateria, brama.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Przeciwnik później, M5 bez niego (wybrana)** | M5 ma zakres, który da się skończyć, sprawdzić i zamknąć. Decyzja jest zapisana, więc nie zginie. Zgodne z miejscem przeciwnika w PRD (opcjonalny, na końcu) | gra do tego czasu nie ma żadnego zagrożenia poza ciemnością i nie da się jej przegrać |
| Przeciwnik w M5 | runda od razu miałaby stawkę | M5 już zmienia rozgrywkę, światła, shadery i panele. Dochodziłby ruch po labiryncie, reguła złapania, stan przegranej i ich testy. Kamień milowy, który nie jest jeszcze sprawdzony ręcznie ani na macOS, urósłby zamiast się zamknąć |
| Bez przeciwnika w ogóle | najmniej kodu, PRD na to pozwala | gra zostaje pokazem tematów wykładu. Dla portfolio chcę czegoś, co jest grą także bez panelu debug |

## 4. Uzasadnienie i skutki

**Dlaczego chcę.** Plan wykładu kończy się na technikach renderowania. Projekt ma pójść dalej, a przeciwnik to najprostszy sposób, żeby labirynt, światło i kolizje zaczęły pracować razem na rozgrywkę.

**Dlaczego nie teraz.** M5 jest gotowe w kodzie na Windowsie, ale nie jest zamknięte: nie było budowane na macOS i nikt nie przeszedł rundy ręcznie. Dokładanie nowej mechaniki do niesprawdzonej to prosty sposób, żeby nie zamknąć żadnej. Przed przeciwnikiem są też tematy wykładu, od których zależy zaliczenie (M6 do M8).

**Skutki dla M5.**

- Rundy nie da się przegrać: `RoundState` ma tylko `Playing` i `Won` ([`battery-darkness-no-loss.md`](battery-darkness-no-loss.md)).
- W kodzie nie ma żadnego przygotowania pod przeciwnika: żadnych pustych klas, flag ani pól na zapas. Gdy przyjdzie jego czas, zacznie się od projektu.
- Dokumenty nie opisują przeciwnika. Ta notatka jest jedynym miejscem, w którym jest zapisany.

## 5. Kiedy wrócić do tej decyzji

Gdy tematy wykładu będą odhaczone, a M5 zamknięte. Trzeba będzie wtedy rozstrzygnąć, w tej kolejności:

- czy w ogóle, przy czasie, który zostanie do obrony,
- do którego kamienia milowego trafia i co z niego wypada w zamian,
- jak przeciwnik porusza się po labiryncie i skąd wie, gdzie jest gracz,
- co się dzieje, gdy dogoni gracza, i czy wraca przez to stan przegranej,
- jak ma się do światła, które jest tematem gry,
- jak to przetestować bez okna, tak jak resztę reguł rundy,
- czy jego zachowanie ma być powtarzalne dla ziarna, tak jak labirynt i kryształy.

Każda z tych odpowiedzi dostanie własną notatkę albo sekcję w dokumencie modułu. Do tego czasu ta notatka niczego nie przesądza poza jednym: przeciwnik jest w planie.
