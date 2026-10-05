# Bloom: trzy cele `GL_RGBA16F` o połowie rozdzielczości sceny

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`src/game/Bloom.hpp`](../../src/game/Bloom.hpp) (`BLOOM_DOWNSCALE`, `bloomTargetExtent`), [`src/game/PostProcess.hpp`](../../src/game/PostProcess.hpp) (`m_brightPass`, `m_blurHorizontal`, `m_bloom`), [`src/game/PostProcess.cpp`](../../src/game/PostProcess.cpp) (`drawBloom`, `fitTarget`), [`tests/BloomTests.cpp`](../../tests/BloomTests.cpp). Dokument modułu: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), sekcja 2.14.

## 1. Kontekst

Bloom to przebieg jasności i kilka powtórzeń rozmycia Gaussa, każde jako przebieg poziomy i pionowy. Przebieg nie może czytać tekstury, do której rysuje, więc rozmycie potrzebuje co najmniej dwóch celów, które zamieniają się rolami (ping-pong). Trzeba było rozstrzygnąć trzy rzeczy naraz: w jakiej rozdzielczości pracują te przebiegi, ile jest celów i w jakim są formacie.

Ograniczenia: PRD wymaga stabilnych 60 klatek na sekundę w 1440p na MacBooku i sam wspomina o post-processie w połowie rozdzielczości, a panel Framebuffers ma pokazywać załączniki, czyli także to, co zostało po progu.

## 2. Decyzja

Wszystkie przebiegi bloomu rysują do celów o **połowie szerokości i połowie wysokości** bufora sceny (`BLOOM_DOWNSCALE = 2`, dzielenie całkowite, nie mniej niż 1 piksel). Celów jest **trzy**, wszystkie `GL_RGBA16F` bez głębi: `m_brightPass` trzyma wynik przebiegu jasności i nigdy nie jest celem rozmycia, a `m_blurHorizontal` i `m_bloom` grają w ping-ponga.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| pełna rozdzielczość, dwa cele | najprostszy kod: te same rozmiary co scena. Ostry przebieg jasności, także dla pojedynczych pikseli | cztery razy więcej fragmentów w każdym z trzynastu przebiegów. Ta sama poświata wymaga cztery razy więcej iteracji (szerokość rośnie jak pierwiastek z ich liczby). Dwa cele 1280 x 720 w `GL_RGBA16F` to 14 MiB zamiast 5 |
| połowa rozdzielczości, dwa cele | minimum pamięci: przebieg jasności pisze od razu do jednego z celów ping-ponga | wynik przebiegu jasności jest zamazywany już w pierwszej iteracji. Podgląd `Bright pass` w panelu pokazywałby obraz rozmyty, a nie to, co zostało po progu, albo trzeba by rysować go w środku funkcji, przed pętlą |
| **połowa rozdzielczości, trzy cele (wybrana)** | ćwierć kosztu każdego przebiegu, to samo jądro sięga na ekranie dwa razy dalej, a panel pokazuje oba kroki osobno. Kod pętli jest prosty: poziomy zawsze pisze do jednego celu, pionowy zawsze do drugiego | jedna tekstura więcej (1,76 MiB przy 1280 x 720, 7,03 MiB przy 2560 x 1440). Poświata jest mierzona w pikselach celu, więc jej szerokość względem ekranu zależy od rozdzielczości. Próg działa na średniej z czterech pikseli sceny, więc jednopikselowe punkty słabną czterokrotnie przed porównaniem |
| łańcuch coraz mniejszych celów (połowa, ćwierć, jedna ósma) sumowanych na końcu | szeroka poświata bardzo tanio i szerokość mniej zależna od rozdzielczości. Tak robią to duże silniki | wyraźnie więcej kodu i więcej celów do pokazania i wytłumaczenia. Nie budowałem tego i nie mierzyłem |
| cele w `GL_RGBA8` | połowa pamięci | wynik przebiegu jasności byłby obcinany do 1, więc poświata światła kilka razy jaśniejszego od bieli byłaby taka sama jak poświata bieli. To zabiera sens całemu buforowi HDR |

## 4. Uzasadnienie i skutki

**Dlaczego połowa.** Rozmyty obraz nie ma ostrych szczegółów, po których byłoby widać mniejszy rozmiar, więc niższa rozdzielczość jest tu prawie darmowa. Filtr liniowy pomaga dwa razy: przy zmniejszaniu (przebieg jasności dostaje średnią czterech pikseli sceny) i przy powiększaniu (przebieg składający rozciąga wynik gładko na cały ekran).

**Dlaczego trzy.** Panel Framebuffers jest pokazem tematu 10 i ma pozwalać rozłożyć efekt na kroki. Trzeci cel kosztuje kilka megabajtów i jedno pole w klasie, a daje obraz, na którym widać sam próg. Komentarz w `drawBloom` mówi to jednym zdaniem: przebieg jasności nie jest nigdy zamazywany, więc jego podgląd pokazuje go takim, jaki był.

**Dlaczego `GL_RGBA16F`.** Z tego samego powodu co scena: poświata ma być proporcjonalna do tego, o ile coś jest jaśniejsze od progu.

**Skutki, które przyjmuję.**

- Poświata ma stałą szerokość w pikselach celu. Przy sześciu iteracjach to około 13 pikseli ekranu: 1,9 procent wysokości okna 720 i 0,9 procent przy 1440. Na większym ekranie wygląda cieniej. Kod tego nie wyrównuje.
- Przy nieparzystym rozmiarze sceny (1281 daje 640) średnia czterech pikseli ma nierówne wagi i skrajna kolumna sceny jest gorzej reprezentowana. Nie widziałem tego na obrazie i nikt tego nie zgłosił.
- Trzy cele idą za rozmiarem bufora sceny w tej samej klatce (`fitTarget` w `drawBloom`), więc zmiana rozmiaru okna odtwarza trzy tekstury.

**Czego nie zmierzyłem.** Liczby pamięci są policzone z rozmiaru (8 bajtów na piksel), nie odczytane z karty. Zgłoszony koszt bloomu (Windows, Release, pomiar niespokojny) to około 0,4 ms na klatkę w 2560 x 1440. Wariantu w pełnej rozdzielczości nikt nie budował, więc "cztery razy drożej" jest rachunkiem liczby fragmentów, nie pomiarem. Na MacBooku nic z tego nie było uruchamiane.

## 5. Kiedy wrócić do tej decyzji

- Gdy na ekranie Retina albo w 1440p poświata okaże się za cienka: pierwszym krokiem jest liczba iteracji zależna od wysokości celu, drugim łańcuch mniejszych celów.
- Gdy bloom na MacBooku zabierze zauważalną część budżetu klatki: wtedy ćwierć rozdzielczości albo mniej iteracji.
- Gdy gwiazdy albo iskry wokół kryształów (PRD wymienia je w temacie 9, nie są zbudowane) mają świecić: próg po uśrednieniu je gasi, więc trzeba by progować w pełnej rozdzielczości albo podnieść ich jasność.
- Gdy trzeci cel przestanie być potrzebny panelowi: wtedy wystarczą dwa.
