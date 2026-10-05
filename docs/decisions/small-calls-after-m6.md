# Cztery drobne rozstrzygnięcia po M6: klawisz paneli, Esc, latarka po zebraniu kryształu, rozmiar nieba

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/main.cpp`](../../src/main.cpp) (klawisz paneli), [`src/game/Round.cpp`](../../src/game/Round.cpp) (`updateRound`: bateria i latarka), [`assets/skybox/`](../../assets/skybox/) (sześć ścian nieba), [`tools/blender/make_skybox.py`](../../tools/blender/make_skybox.py). Dokumenty modułów: [`../modules/debug-ui.md`](../modules/debug-ui.md), [`../modules/game/gameplay.md`](../modules/game/gameplay.md), [`../modules/renderer/skybox.md`](../modules/renderer/skybox.md).

## 1. Kontekst

Po zakończeniu kodu M5 i M6 zostały cztery otwarte pytania. Żadne nie blokowało pracy, ale każde dotyczy czegoś, o co można zostać zapytanym na obronie ("dlaczego tak?"). Odpowiedziałem na wszystkie tego samego dnia i zapisuję je razem, bo każde z osobna jest za małe na własną notatkę.

1. PRD mówi, że panele debugowania otwiera klawisz F1. W kodzie jest to klawisz akcentu (`GLFW_KEY_GRAVE_ACCENT`, ten pod Esc).
2. Esc zwalnia mysz, ale runda biegnie dalej: zegar liczy czas, a bateria latarki się rozładowuje.
3. Zebranie kryształu doładowuje baterię, ale nie zapala latarki, która zgasła po rozładowaniu.
4. Sześć obrazów nieba zajmuje razem 5,28 MB w repozytorium (1024 px na ścianę, a dithering przeciw pasmom psuje kompresję PNG).

## 2. Decyzja

1. **Klawisz paneli zostaje klawiszem akcentu.** Zmieniłem go z F1 świadomie, pracując na MacBooku: tam klawisze funkcyjne domyślnie sterują jasnością i głośnością, a F1 wymaga trzymania Fn. PRD w tym miejscu jest nieaktualny, kod i dokumenty są źródłem prawdy.
2. **Esc na razie nie zatrzymuje rundy.** Pauza dojdzie razem z menu (plan M9), jako jeden z jego stanów.
3. **Latarka po zebraniu kryształu zostaje zgaszona.** Gracz zapala ją sam klawiszem F.
4. **Obrazy nieba zostają w rozmiarze 1024 px.** Nie zmniejszam ich.

## 3. Rozważane możliwości

| Pytanie | Wybrane | Druga możliwość | Dlaczego nie druga |
|---|---|---|---|
| Klawisz paneli | klawisz akcentu | F1 jak w PRD | na Macu F1 bez Fn zmienia jasność ekranu, więc w trakcie pokazu łatwo o pomyłkę. Klawisz akcentu działa tak samo na obu systemach i leży pod lewą ręką obok W A S D |
| Esc | tylko zwalnia mysz | Esc od razu zatrzymuje zegar i baterię | pauza to stan gry: potrzebuje ekranu, powrotu i decyzji, co się wtedy rysuje. To zakres menu. Zrobiona teraz osobno, zostałaby przerobiona przy menu |
| Latarka po krysztale | zostaje zgaszona | zapala się sama | włącznik należy do gracza. Latarka, która zapala się sama, zaczęłaby od razu zużywać świeżo zebraną energię, także wtedy, gdy gracz chce ją oszczędzać i idzie w świetle kryształów |
| Rozmiar nieba | 1024 px, 5,28 MB | 512 px, około cztery razy mniej | grafika gry ma być dalej rozwijana, a nie upraszczana. Przy 512 px gwiazdy stają się widocznie rozmyte w oknie 1440p |

## 4. Uzasadnienie i skutki

- **Klawisz paneli.** Wszędzie, gdzie dokumenty mówią o otwieraniu paneli, mowa o klawiszu akcentu. Na obronie zdanie z PRD "otwieram panel F1" czytam jako "otwieram panel klawiszem akcentu". Skutek uboczny: na klawiaturach, gdzie ten klawisz wymaga kombinacji (część układów europejskich), trzeba by dodać drugi klawisz. Na układzie polskim programisty i na amerykańskim problemu nie ma.
- **Esc.** Do czasu menu czas rundy pokazany na karcie wygranej zawiera chwile, w których mysz była zwolniona (na przykład podczas przestawiania suwaków w panelach). To znane ograniczenie: reguły rundy opisuje [`../modules/game/gameplay.md`](../modules/game/gameplay.md), a `updateRound` nie wie, czy mysz jest przechwycona. Kto mierzy czas przejścia, nie powinien w trakcie rundy zwalniać myszy.
- **Latarka.** Reguła w `updateRound` zostaje jedna: pusta bateria wymusza zgaszenie, nic nie wymusza zapalenia. Wyjątek z komentarza w kodzie też zostaje: kryształ zebrany w tym samym kroku, w którym bateria doszła do zera, ratuje światło, bo zebranie jest liczone przed sprawdzeniem pustej baterii.
- **Niebo.** Repozytorium rośnie o 5,28 MB i będzie rosło dalej wraz z kolejnymi zasobami. Przyjmuję to. Gdyby rozmiar repozytorium zaczął przeszkadzać (czas klonowania, limit hostingu), pierwszym krokiem jest Git LFS dla katalogu `assets/`, a nie zmniejszanie obrazów.

## 5. Kiedy wrócić do tej decyzji

- Klawisz paneli: gdy ktoś uruchomi grę na klawiaturze, na której klawisz akcentu nie jest pojedynczym klawiszem.
- Esc: przy menu (M9). Wtedy ta część notatki zostaje zastąpiona opisem pauzy.
- Latarka: gdyby testy z graczami pokazały, że ciemność po zebraniu kryształu jest odbierana jako błąd, a nie jako wybór.
- Niebo: gdy `assets/` przekroczy rozmiar wygodny dla zwykłego Gita.
