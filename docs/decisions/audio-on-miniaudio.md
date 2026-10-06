# Dźwięk przez bibliotekę miniaudio

Data: 2026-10-06. Stan: obowiązuje jako decyzja o wyborze biblioteki, **kodu dźwięku nie ma**. Decyzja właściciela projektu jest w całości w sekcji 2; kontekst, tabela, skutki i warunki powrotu to moja analiza.
Kod: brak. W `cmake/Dependencies.cmake` nie ma biblioteki dźwiękowej, a w `src/` nie ma kodu odtwarzającego dźwięk (sprawdzone wyszukiwaniem: nazwa `miniaudio` nie występuje w kodzie ani w assetach (poza tą notatką i jej odnośnikami w dokumentach)). Dokument modułu: brak.

## 1. Kontekst

Gra jest dziś bez dźwięku. Rozmowa o kierunku dalszych prac (2026-10-06, po M9, części 4) objęła dźwięk, a właściciel wybrał bibliotekę.

Projekt ma preferencję dla małych własnych pomocników zamiast dużych bibliotek ([`video-through-os-decoders-with-still-fallback.md`](video-through-os-decoders-with-still-fallback.md) wybrała dekodery systemu zamiast FFmpeg z tego samego powodu), a zależności są dziś zapisane w jednym miejscu, `cmake/Dependencies.cmake` (siedem pozycji, [`../libraries/rmlui.md`](../libraries/rmlui.md) opisuje dwie ostatnie).

## 2. Decyzja

Decyzja właściciela projektu (2026-10-06), w całości:

1. Dźwięk w grze będzie oparty na bibliotece **miniaudio**.

Nic ponad to nie zostało rozstrzygnięte w tej decyzji. Reszta tej notatki to analiza, nie decyzja.

## 3. Rozważane możliwości

To jest analiza, nie decyzja. Pierwszy wiersz jest decyzją właściciela, pozostałe to możliwości, których nie oceniałem wobec żadnych liczb z repozytorium.

| Możliwość | Zalety | Wady |
|---|---|---|
| **miniaudio (wybrana)** | wybór właściciela; z mojej ogólnej wiedzy (nie sprawdzonej w tym repozytorium) to biblioteka dźwiękowa dostarczana jako jeden plik, co pasuje do podejścia małych zależności | wersja i licencja nie są sprawdzone (sekcja 4); dochodzi ósma zależność |
| Dekodery i API dźwięku systemu operacyjnego (jak przy wideo) | zero nowych bibliotek | osobny kod zależny od platformy dla każdego systemu (tak jest przy wideo, [`../modules/video/README.md`](../modules/video/README.md)) |
| Brak dźwięku | zero kodu | gra zostaje niema, a właściciel chce z niej zrobić grę do portfolio ([`build-for-the-portfolio-not-the-defence.md`](build-for-the-portfolio-not-the-defence.md)) |

## 4. Uzasadnienie i skutki

**Dlaczego tak.** Uzasadnienia właściciela nie mam. Nie dopisuję własnego.

**Skutki znane i otwarte** (analiza):

- **Wersję i licencję trzeba sprawdzić na przypiętym wydaniu, gdy praca się zacznie.** Do tego czasu nie zapisuję żadnej licencji ani numeru wersji: nie sprawdziłem ich, a nie chcę, żeby notatka niosła liczbę, której nikt nie widział. Powinno to iść razem z dopisaniem biblioteki do `THIRD-PARTY-NOTICES.txt` (plik jest generowany skryptem `launcher/scripts/build-notices.mjs` z pobranych zależności, [`../guides/project-structure.md`](../guides/project-structure.md)).
- **Pobieranie.** Zależności są pobierane przez `FetchContent` z przypiętymi wersjami. Nowa pozycja ma iść tą samą drogą.
- **Warstwa.** Gdzie kod dźwięku stanie (biblioteka obok `video`, część `game`), nie jest rozstrzygnięte. Wzór z biblioteki `video` (ogólne API w nagłówku, kod zależny od systemu schowany) jest możliwy, ale to moje przypuszczenie.
- **Pliki dźwiękowe.** Nie ma żadnych assetów dźwiękowych i nie ustalono, skąd będą pochodzić ani w jakim formacie.
- **macOS i Windows.** Nic z dźwiękiem nie było budowane na żadnym z nich.

**Czego ta notatka nie przesądza.** Wersji ani licencji biblioteki, miejsca kodu w warstwach, listy dźwięków, formatu plików ani tego, kiedy praca się zacznie.

## 5. Kiedy wrócić do tej decyzji

- Gdy praca nad dźwiękiem się zacznie: sprawdzić wersję i licencję na przypiętym wydaniu, zapisać je tutaj i dopisać do `THIRD-PARTY-NOTICES.txt`.
- Jeśli licencja albo sposób budowania okaże się nie do przyjęcia: wrócić do tabeli.
