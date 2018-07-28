# Particles_DX11

## Dotychczas zrobiono
* Rozpoczęto wstępny redesign, poprawiono najbardziej rażące blędy
* Przepisano projekt z DirectX 10 na DirectX 11
* Dodano sortowanie po stronie GPU

## TODO
* Przepisanie zaimplementowanego rozwiązania na Vulkan
* Poprawienie sortowania po stronie GPU (optymalizacja)
* Przepisanie sortowania po stronie CPU by korzystao z tego samego algorytmu co sortowanie po stronie GPU (Bitonic Sort)
* (Opcjonalnie) Dokończenie redesign'u
* Finalne posprzątanie kodu

## Sterowanie
### Emisja Particli
* 1...7 - Rozpoczęcie emisji jednego z efektów
* 0 - Wstrzymanie emisji
* 8 - Wybuch
* 9 - Fajerwerki
### Kamera
* LPM - Obracanie kamery w poziomie
* PPM - Ustawianie kamery w pionie
* Rolka - Przybliżanie/Oddalanie
### Inne
* Esc - Wyjście z aplikacji
* V - Wączenie ograniczenia do 60 FPS
* F1 - Sortowanie po GPU
* F2 - Sortowanie po CPU


# Particles_Vulkan
