# Arduino_NixieClock

Nixie-Uhr auf Basis ESP8266 (ESP-07), Uhrzeit per NTP (sntp).

## Voraussetzungen

- [VS Code](https://code.visualstudio.com/) mit der Extension **PlatformIO IDE**
  (`platformio.platformio-ide`).
- USB-Seriell-Treiber fuer den CP2102-Adapter (unter Linux in der Regel bereits
  vorhanden).
- Beim ersten Build laedt PlatformIO die Platform `espressif8266` und dessen
  Toolchain automatisch herunter (mehrere 100 MB, Internetverbindung noetig).

## Hardware-Version

Ueber `HW_VERSION` in `src/NixieClock.ino` waehlen:

- `HW_POC`   - handverdrahtetes Erstlingsmodell, Pins `{16,14,12,13}` (Standard)
- `HW_BOARD` - designte Platine, Pins `{13,12,14,16}`

## Bauen & Flashen

Der ESP muss zum Flashen im Bootloader-Modus sein (PROG-Taster gedrueckt
halten + RESET, dann PROG loslassen).

- **VS Code:** Projektordner oeffnen → PlatformIO-Extension erkennt
  `platformio.ini` automatisch → ueber die Statusleiste *Build* / *Upload*
  bzw. *Monitor* nutzen.
- **CLI:** `pio run` (bauen), `pio run -t upload` (flashen),
  `pio device monitor -b 115200` (Seriell-Log).

## Erstinbetriebnahme

Nach dem Flashen ist das EEPROM leer. Die Uhr startet daher in den
Access-Point-Modus (`SSID "Nixie"`, ohne Passwort):

1. Mit einem Geraet mit dem WLAN `Nixie` verbinden.
2. Im Browser `http://192.168.4.1` oeffnen.
3. WLAN-Zugangsdaten eintragen und speichern; die Uhr startet neu und
   verbindet sich mit dem konfigurierten Netz und holt die Zeit per NTP.

## Seriell-Log

`Serial.begin(115200)` ist aktiv. Im Log stehen Firmware-/HW-Version,
konfigurierte SSID, WLAN-Status und der verwendete NTP-Server.

## PlatformIO Version

Getestet mit PlatformIO Core `6.2.0` und Platform `espressif8266@4.2.1`.
Aktualisierung des Core: `pip install -U platformio` (bzw. ueber die
PlatformIO-Extension unter *Quick Access → Miscellaneous → PlatformIO Core
CLI*).