# Cień księżyca odbiera tylko światło księżyca

Data: 2026-10-05. Stan: obowiązuje.
Kod: [`assets/shaders/common/lighting.glsl`](../../assets/shaders/common/lighting.glsl) (struktura `Lighting`: `moonDiffuse`, `moonSpecular`, funkcja `computeLighting`), [`assets/shaders/lit.frag`](../../assets/shaders/lit.frag), [`gouraud.frag`](../../assets/shaders/gouraud.frag), [`grass.frag`](../../assets/shaders/grass.frag) (odejmowanie), [`assets/shaders/common/shadows.glsl`](../../assets/shaders/common/shadows.glsl) (`moonShadow`, `uMoonShadowStrength`), [`src/game/Lighting.hpp`](../../src/game/Lighting.hpp) (`moonIntensity`). Dokument modułu: [`../modules/renderer/shadows.md`](../modules/renderer/shadows.md), sekcja 2.14.

## 1. Kontekst

Scena ma cztery źródła jasności: światło otoczenia, księżyc, latarkę gracza i do 16 świateł kryształów, a do tego świecenie własne kryształów. Funkcja `computeLighting` sumowała je do dwóch liczb, części rozproszonej i odbłysku. Mapa cieni księżyca odpowiada na jedno pytanie: czy do punktu dociera światło **księżyca**.

Najprostszy sposób użycia tej odpowiedzi, znany z wielu poradników, to pomnożenie końcowego koloru (albo wszystkiego poza światłem otoczenia) przez `1 - cień`.

## 2. Decyzja

`computeLighting` zwraca dodatkowo **udział księżyca** w każdej z dwóch sum. Wołający odejmuje od sumy ten udział pomnożony przez udział cienia (od 0 do 1). Nic innego nie jest przyciemniane: ani światło otoczenia, ani latarka, ani kryształy, ani świecenie własne.

## 3. Rozważane możliwości

| Możliwość | Zalety | Wady |
|---|---|---|
| **odjęcie udziału księżyca (wybrane)** | fizycznie sensowne: każde światło ma własną widoczność. Latarka i kryształy świecą w cieniu księżyca. `computeLighting` nie wie nic o cieniach, więc ta sama funkcja służy programom `lit` (na fragment) i `gouraud` (na wierzchołek) | dwa dodatkowe pola w strukturze i dwie dodatkowe zmienne w programie Gouraud. Każde kolejne światło z cieniem potrzebuje własnej pary pól |
| pomnożenie całego koloru przez `1 - cień` | jedna linia | w cieniu gaśnie latarka, światło kryształów i świecenie własne. Kryształ stojący w cieniu ściany przestaje świecić |
| pomnożenie wszystkiego poza światłem otoczenia | prawie jedna linia | latarka i kryształy nadal gasną w cieniu księżyca |
| parametr "cień" przekazany do `computeLighting` | bez odejmowania, bez dodatkowych pól | nie działa w trybie Gouraud, gdzie funkcja jest wołana w wierzchołku, a cień jest znany dopiero w fragmencie |

## 4. Uzasadnienie i skutki

**Dlaczego ta.** Gra jest o chodzeniu z latarką po ciemnym labiryncie i o kryształach, które świecą. Gdyby cień księżyca gasił latarkę, w najciemniejszych miejscach, czyli tam, gdzie latarka jest potrzebna, nie byłoby jej widać. To byłby błąd widoczny w pierwszej minucie.

**Intensywność księżyca.** W cieniu zostaje samo światło otoczenia, więc o tym, czy cień w ogóle widać, decyduje stosunek "otoczenie plus księżyc" do "samo otoczenie". Dlatego razem z cieniami domyślna intensywność księżyca wzrosła z 0,12 do 0,2: poziomy grunt w świetle księżyca jest wtedy około pięć razy jaśniejszy niż w cieniu (policzone na wartościach liniowych: od 4,6 do 4,7 raza w każdym kanale).

**Skutki, które przyjmuję.**

- Latarka i kryształy nie mają własnych map cieni, więc nadal świecą przez ściany. Cień latarki jest planowany, cieni kryształów nie ma w planie.
- Odejmowanie dwóch liczb, które powinny być równe, może dać wynik o ostatni bit poniżej zera. Stąd `max(..., 0.0)` w trzech shaderach.
- Suwak `Strength` skaluje odejmowaną część. To pokrętło wyglądu, a nie model fizyczny.
- Scena jest ogólnie jaśniejsza niż przed tą częścią w miejscach oświetlonych przez księżyc. Liczby w dokumentach, które wynikały z intensywności 0,12, trzeba czytać z tą poprawką.

**Co jest zgłoszone (Windows, 2026-10-05).** Z wyłączonymi cieniami i księżycem cofniętym do 0,12 obraz jest identyczny co do piksela z obrazem sprzed zmiany (w trybie Phong różnica najwyżej 1/255), więc wydzielenie udziału księżyca nie zmieniło sumy światła.

## 5. Kiedy wrócić do tej decyzji

- Gdy latarka dostanie mapę cieni: struktura `Lighting` potrzebuje wtedy drugiej pary pól, a przy trzecim świetle warto rozważyć tablicę udziałów.
- Gdyby wygląd wymagał cieni przyciemniających także światło otoczenia (na przykład prosty model zacienienia otoczenia): to osobny efekt, nie zmiana tej reguły.
