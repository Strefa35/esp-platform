# LCD Module (PL)

## Przeglad
Modul `lcd_ctrl` zapewnia obsluge wyswietlacza i dotyku na platformie ESP:
- LCD 2.8" (`320x240`) z ILI9341 po SPI
- panel dotykowy NS2009 po I2C
- LVGL 9.x jako framework GUI

Aktualnie modul jest rozwijany pod sprzet klasy ESP32 uzywany w tym projekcie.

## Aktualny stan UI

### Ekran glowny (`ui_main_screen`)
Obecna implementacja zawiera:
- gorny pasek stanu z ikonami po lewej: Ethernet, Wi-Fi, MQTT, Bluetooth
- przycisk ustawien po prawej stronie paska
- duzy cyfrowy zegar i date w lewej czesci
- sekcje oswietlenia (`Ambient (lx)`) z:
  - ikona slonca
  - tekstem aktualnego lux
  - tekstem progu
  - poziomym gradientowym paskiem z markerem wartosci i progu

Wazna uwaga implementacyjna:
- blok karty przekaznikow (podgrzewacz + pompa cyrkulacji) istnieje w kodzie, ale jest obecnie wylaczony kompilacyjnie przez `#if 0` w `ui_main_screen_create()`.

### Wygaszacz (`ui_screensaver`)
Zaimplementowany i aktywny:
- zegar analogowy (gdy `LV_USE_SCALE` jest wlaczone) lub cyfrowy fallback
- etykieta daty
- panel pogody: ikona, temperatura, opis i lokalizacja

### Ekran konfiguracji
- mockup istnieje (`docs/todo/lcd_mockup_config.png`)
- dedykowana implementacja UI tego ekranu nie jest jeszcze gotowa

## Mockupy (320x240)
Pliki referencyjne:
- `docs/todo/lcd_mockup_main.png`
- `docs/todo/lcd_mockup_config.png`
- `docs/todo/lcd_mockup_screensaver.png`

## Warstwy sterownika i integracji

### `lcd_ctrl.c`
Punkty wejscia od strony kontrolera i obsluga wiadomosci przez kolejke.

### `lcd_helper.c`
Warstwa integracji LVGL:
- inicjalizacja LVGL
- podpiete wyswietlanie i dotyk
- okresowy tick LVGL i task handlera
- helpery aktualizacji UI uzywane przez przeplyw LCD controllera

### `lcd_hw.c`
Warstwa kompozycji sprzetu dla init/deinit wyswietlacza i dotyku.

### `ili9341v.c`
Backend wyswietlacza ILI9341:
- konfiguracja panelu i magistrali przez ESP-IDF `esp_lcd`
- integracja callbacka flush dla LVGL
- obsluga buforow ramki z DMA

### `ns2009.c`
Backend dotyku:
- probe i odczyty I2C
- konwersja i kalibracja wspolrzednych
- filtrowanie nacisku/poprawnego dotkniecia

## Kconfig i strojenie
Glowne opcje LCD sa w `modules/lcd_ctrl/Kconfig.inc`, m.in.:
- wlaczenie modulu i poziomy logowania
- kalibracja dotyku, zamiana/odwrocenie osi, prog dotyku
- wybor orientacji wyswietlacza

Rozmiar fontow dla zegara zalezy od wlaczonych fontow LVGL w konfiguracji projektu:
- np. `CONFIG_LV_FONT_MONTSERRAT_14`, `CONFIG_LV_FONT_MONTSERRAT_46`
- jesli font nie jest wlaczony, symbol `lv_font_montserrat_*` nie bedzie dostepny

## Luki / kolejne kroki
- implementacja dedykowanego ekranu konfiguracji
- ponowne wlaczenie i dopracowanie panelu przekaznikow na ekranie glownym
- dalsze wyrownanie dokumentacji i mockupow z finalnym zachowaniem runtime
