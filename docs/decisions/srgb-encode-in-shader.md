# Kodowanie sRGB w shaderze ostatniego przebiegu, a nie przez `GL_FRAMEBUFFER_SRGB`

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/post/composite.frag`](../../assets/shaders/post/composite.frag) (ostatnia linia `main`), [`assets/shaders/common/color.glsl`](../../assets/shaders/common/color.glsl) (`linearToSrgb`), [`src/game/PostProcess.cpp`](../../src/game/PostProcess.cpp) (`composite`: `glDisable(GL_FRAMEBUFFER_SRGB)`), [`assets/shaders/post/preview.frag`](../../assets/shaders/post/preview.frag). Dokumenty modułów: [`../modules/renderer/post-process.md`](../modules/renderer/post-process.md), [`../modules/gfx/color-space.md`](../modules/gfx/color-space.md).

## 1. Kontekst

Potok liniowy ([`gamma-linear-pipeline.md`](gamma-linear-pipeline.md)) kończy się kodowaniem: wartości liniowe od 0 do 1 muszą stać się liczbami sRGB, zanim trafią do okna. OpenGL daje na to dwie drogi.

Pierwsza to przełącznik `glEnable(GL_FRAMEBUFFER_SRGB)`: gdy cel rysowania ma format sRGB, karta sama koduje każdy zapisywany kolor. Druga to policzenie tej samej funkcji w shaderze fragmentów i zapisanie gotowych liczb do zwykłego celu.

Ograniczenia, które decydowały:

- Po scenie, do tego samego okna, rysuje ImGui: panele debugowania i HUD. Jego motyw ma kolory dobrane jako liczby sRGB, a backend OpenGL zapisuje je bez żadnego przeliczenia.
- Projekt działa na macOS i na Windowsie. To, czy okno dostaje framebuffer zdolny do sRGB, zależy od systemu i sterownika, a o taki framebuffer trzeba prosić wskazówką `GLFW_SRGB_CAPABLE` przed utworzeniem okna.
- Te same wątpliwości zapisała już zastąpiona notatka [`no-gamma-until-m7.md`](no-gamma-until-m7.md) w wierszu o `GL_FRAMEBUFFER_SRGB`.

## 2. Decyzja

Klatkę koduje **shader**: ostatnia linia `post/composite.frag` to `fragColor = vec4(linearToSrgb(color), 1.0);`. Przełącznik `GL_FRAMEBUFFER_SRGB` zostaje **wyłączony**, a `PostProcess::composite` woła `glDisable(GL_FRAMEBUFFER_SRGB)` wprost, choć to stan domyślny: linia zapisuje w kodzie, że przebieg na tym polega. Gra nie prosi o okno sRGB.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **Kodowanie w `composite.frag`, przełącznik wyłączony (wybrana)** | jedno, widoczne miejsce kodowania, które da się pokazać i wytłumaczyć linia po linii. Wynik nie zależy od tego, jaki framebuffer dał system. ImGui rysuje po tym przebiegu do zwykłego okna i zachowuje kolory motywu. Ta sama funkcja GLSL służy podglądom załączników i widokom diagnostycznym | jedna potęga na piksel ekranu w każdej klatce. Funkcja i jej pięć stałych są zapisane także w GLSL, obok wersji C++ |
| `GLFW_SRGB_CAPABLE` i `glEnable(GL_FRAMEBUFFER_SRGB)` przez całą klatkę | kodowanie robi karta, bez kodu w shaderze | ImGui zapisałoby swoje liczby sRGB do celu, który koduje jeszcze raz: panele i HUD wyszłyby rozjaśnione i wyblakłe, a kontrast motywu przestałby się zgadzać. Zachowanie zależy od tego, czy system naprawdę dał okno sRGB |
| To samo, ale przełącznik włączany tylko na czas przebiegu `composite` i wyłączany przed ImGui | kodowanie na karcie, ImGui bez zmian | nadal wymaga okna sRGB, czyli zależy od systemu. Dochodzi stan globalny, który trzeba włączać i wyłączać w dobrej kolejności: pomyłka daje podwójne kodowanie albo jego brak, bez komunikatu. Podglądy załączników i tak potrzebują funkcji w shaderze |
| Pośredni bufor w formacie `GL_SRGB8_ALPHA8` i skopiowanie go do okna | kodowanie na karcie niezależnie od okna | dodatkowy bufor i dodatkowe kopiowanie całego ekranu tylko po to, żeby nie napisać jednej funkcji |

## 4. Uzasadnienie i skutki

**Dlaczego shader.** Na obronie mam umieć pokazać, gdzie dokładnie liczba liniowa staje się liczbą dla ekranu. Z przełącznikiem odpowiedź brzmi "karta robi to sama, jeśli okno jest sRGB", i od razu rodzi pytanie, skąd wiem, że jest. Z funkcją w shaderze odpowiedzią jest jedna linia i dwa wzory, które mają testy po stronie C++.

**ImGui.** To był argument rozstrzygający. Panele i HUD są rysowane po `composite`, prosto do okna (wywołanie w `main.cpp`, po powrocie z `NightMazeApp::onRender`). Przy wyłączonym przełączniku ich kolory trafiają na ekran tak samo jak przed M7 i motyw nie wymagał żadnej zmiany.

**Skutki, które przyjmuję.**

- Kodowanie kosztuje potęgę na piksel. Zgłoszony pomiar całej zmiany M7 (bufor HDR, przebieg `composite`, kodowanie razem) to spadek z około 2700 do 2500 klatek na sekundę w 1280 x 720 i z około 2020 do 1960 w 2560 x 1440 (Windows, Release, bez synchronizacji pionowej, panele ukryte). Samego kodowania nikt nie mierzył osobno.
- Zaokrąglenie do 8 bitów okna następuje po kodowaniu, czyli na liczbach sRGB. To jest właściwa kolejność: ciemne tony dostają więcej stopni.
- ImGui miesza półprzezroczyste panele ze sceną na liczbach sRGB, a nie liniowych. Tak było zawsze i tak działa ImGui w większości programów.
- Podgląd tekstur sRGB w zakładce Diagnostics / Assets potrzebuje osobnego obejścia (`debug::RawTextureSampler`), bo ImGui czyta teksturę i zapisuje ją bez kodowania. Z przełącznikiem włączonym dla ImGui ten podgląd byłby poprawny sam z siebie, ale kosztem wszystkich pozostałych kolorów paneli.
- Linia `glDisable(GL_FRAMEBUFFER_SRGB)` jest zabezpieczeniem: gdyby jakiś sterownik albo późniejszy kod włączył przełącznik na oknie sRGB, obraz byłby zakodowany dwa razy.

**Czego nie zmierzyłem.** Nie sprawdzałem, czy okno na macOS (Retina) albo na Windowsie jest zdolne do sRGB bez proszenia o to. Przy wyłączonym przełączniku nie powinno to mieć znaczenia, ale to wniosek ze specyfikacji, nie z pomiaru. Punkt jest na liście w [`../guides/build-macos.md`](../guides/build-macos.md).

## 5. Kiedy wrócić do tej decyzji

- Gdyby obraz na macOS był wyraźnie jaśniejszy albo ciemniejszy niż na Windowsie przy tych samych ustawieniach.
- Gdyby ImGui zostało zastąpione własnym interfejsem rysowanym w przestrzeni liniowej (menu z M9): wtedy interfejs może wejść do bufora przed kodowaniem i argument o motywie znika.
- Gdyby na końcu klatki doszło mieszanie kolorów w oknie (na przykład nakładana minimapa z przezroczystością): mieszanie przy `GL_FRAMEBUFFER_SRGB` odbywa się liniowo, w shaderze trzeba o to zadbać samemu, rysując takie rzeczy przed kodowaniem. **Dopisek z 2026-10-06 (M7, część szósta):** minimapa to właśnie taka nakładka z przezroczystością i wybrano w niej drugą drogę: mapa jest rysowana **po** przebiegu składającym, jej kolory są stałymi sRGB zapisywanymi bez konwersji, `GL_FRAMEBUFFER_SRGB` zostaje wyłączone, a mieszanie liczy się na wartościach sRGB, czyli nie jest liniowe ([`minimap-srgb-constants-after-composite.md`](minimap-srgb-constants-after-composite.md)). Nikt nie porównywał tego z mieszaniem liniowym.
- Gdyby profilowanie pokazało, że potęga na piksel ma znaczenie na MacBooku w 1440p.
