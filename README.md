# 🔋 Pylontech Battery Monitor (ESP32)

*[🇬🇧 English Version below](#english-version)*

Ein schlanker, stabiler und responsiver ESP32-Datenlogger zum Auslesen von Pylontech-Batterien über den Konsolen-Port. Dieses Projekt wurde speziell für eine **saubere Integration in ioBroker / Home Assistant** via MQTT optimiert. 

Dieses Projekt ist ein Fork / eine Weiterentwicklung des großartigen ursprünglichen Projekts von **[hidaba](https://github.com/hidaba/PylontechMonitoring)**. 

![Web UI Screenshot]([dashboard.png])

## ✨ Features & Neuerungen
* **ioBroker & HA ready:** Sendet rohe, saubere MQTT-Werte. Überträgt Auto-Discovery-Daten, sodass der Objektbaum vollautomatisch und fehlerfrei aufgebaut wird (kein Matroschka-Effekt mehr!).
* **Modernes Web-Dashboard:** Integriertes Dark-Mode Dashboard (mit AJAX) zur flüssigen Live-Anzeige von Leistung (W), SoC (%), Temperatur und System-Uptime.
* **Support für große Stacks (Bis zu 16 Batterien):** Erkennt dynamisch in Reihe geschaltete Batterien. Ein Parsing-Bug bei zweistelligen Batterie-IDs (>9) wurde behoben. Leere Slots ("Absent") werden intelligent ignoriert.
* **ElegantOTA Integration (NEU):** Firmware-Updates können nun bequem direkt über das Web-Dashboard (OTA Update Button) hochgeladen werden. Ideal für ESP32 in entfernten Subnetzen oder hinter WireGuard-VPNs.
* **MQTT-Stabilität:** Netzwerk-Puffer-Optimierungen verhindern Verbindungsabbrüche beim Senden vieler Daten (Bulk-Updates bei Stacks mit 10+ Batterien).

## 🛠️ Hardware & Verkabelung
**Getestete Hardware:** Erfolgreich getestet mit **Pylontech US2000C** (Setup mit 12 Batterien).
Das Auslesen erfolgt über einen ESP32 und ein RS232-zu-TTL Modul (z. B. MAX3232). 

👉 **Für den genauen Schaltplan und die RJ45-Pinbelegung schau bitte in das [Original-Repository von hidaba](https://github.com/hidaba/PylontechMonitoring).** Dort ist die Verkabelung perfekt und bebildert erklärt.

## 🚀 Installation
1. Lade das Repository als ZIP herunter.
2. Kopiere den Inhalt des beiliegenden **`libraries`**-Ordners in dein lokales Arduino-Bibliotheksverzeichnis (z. B. `Dokumente/Arduino/libraries/`).
3. Öffne die `PyloMonESP32.ino` in der Arduino IDE.
4. Installiere folgende Bibliotheken über den Bibliotheksverwalter: `PubSubClient`, `ArduinoJson`, `ElegantOTA`.
5. Öffne die **`PylontechMonitoring1.h`** und trage deine WLAN- und MQTT-Daten ein. Passe ggf. `MAX_PYLON_BATTERIES` an dein Setup an.
6. Flashe den Code beim ersten Mal per USB auf den ESP32. Zukünftige Updates können bequem als `.bin`-Datei über die Web-UI hochgeladen werden.

## 💡 Hinweis für ioBroker
Damit der MQTT-Objektbaum sauber aufgebaut wird und keine Endlosschleife entsteht:
Gehe in die Einstellungen deiner MQTT-Instanz (Client-Modus), lösche den Eintrag unter **"Prefix for all topics"** (dieses Feld MUSS leer sein!) und deaktiviere "Publish own states on connect". Abonniere `#` (Raute), damit die Auto-Discovery Topics erkannt werden.

## ⚠️ Haftungsausschluss (Disclaimer)
**Die Nutzung dieses Sketches erfolgt absolut auf eigene Gefahr!** Ich übernehme keinerlei Haftung oder Verantwortung für Schäden an deiner Batterie, deinem Wechselrichter, deinem Haus oder Personen, die durch die Nutzung dieses Codes oder fehlerhafte Verkabelung entstehen. Batterien bergen ein hohes Gefahrenpotenzial. Wenn du dir unsicher bist, ziehe eine Fachkraft zurate.

## 🤝 Credits & Danksagung
* **[hidaba](https://github.com/hidaba/PylontechMonitoring):** Für die hervorragende Ausgangsbasis, den initialen Code und die Custom-Libraries.
* **Google Gemini (AI):** Für die intensive Hilfe bei der Code-Optimierung (robuster Parser, Memory-Management, ioBroker-Integration und UI-Design).

---
---

<a name="english-version"></a>
# 🇬🇧 English Version

A lightweight, stable, and responsive ESP32 data logger for reading Pylontech batteries via the console port. This project was heavily optimized for a **clean integration into ioBroker / Home Assistant** via MQTT.

This project is a fork / further development of the great original project by **[hidaba](https://github.com/hidaba/PylontechMonitoring)**.

## ✨ Features & Updates
* **ioBroker & HA ready:** Sends clean MQTT data. Transmits Auto-Discovery payloads so your smart home automatically builds the perfect object tree without errors.
* **Modern Web Dashboard:** Integrated dark-mode dashboard (using AJAX) for smooth, live monitoring of Power (W), SoC (%), Temperature, and System Uptime.
* **Large Stack Support (Up to 16 Batteries):** Dynamically detects connected batteries in a stack. A parsing bug affecting double-digit battery IDs (>9) has been fixed. Empty slots ("Absent") are intelligently ignored.
* **ElegantOTA Integration (NEW):** Firmware updates can now be uploaded directly via the web dashboard. Perfect for ESP32 devices in remote subnets or behind WireGuard VPNs.
* **MQTT Stability:** Network buffer optimizations prevent connection drops when sending large amounts of data (bulk updates for stacks with 10+ batteries).

## 🛠️ Hardware & Wiring
**Tested Hardware:** Successfully tested with **Pylontech US2000C** (12 battery setup).
Reading is done via an ESP32 and an RS232-to-TTL module (e.g., MAX3232).

👉 **For the exact wiring diagram and RJ45 pinout, please refer to the [original repository by hidaba](https://github.com/hidaba/PylontechMonitoring).** The wiring is explained perfectly with images there.

## 🚀 Installation
1. Download this repository as a ZIP file.
2. Copy the contents of the included **`libraries`** folder into your local Arduino libraries directory (e.g., `Documents/Arduino/libraries/`).
3. Open `PyloMonESP32.ino` in the Arduino IDE.
4. Install the following libraries via the Library Manager: `PubSubClient`, `ArduinoJson`, `ElegantOTA`.
5. Open **`PylontechMonitoring1.h`** and insert your WiFi and MQTT credentials. Adjust `MAX_PYLON_BATTERIES` if necessary.
6. Flash the code to your ESP32 via USB for the first time. Future updates can be done by uploading the `.bin` file through the web UI.

## 💡 Note for ioBroker Users
To ensure a clean MQTT object tree and prevent infinite routing loops:
Go to your MQTT instance settings (Client mode), clear the **"Prefix for all topics"** field (it MUST be empty!), and disable "Publish own states on connect". Subscribe to `#` to allow auto-discovery topics.

## ⚠️ Disclaimer
**Use this sketch strictly at your own risk!**
I take no liability or responsibility for any damage to your battery, inverter, property, or persons caused by the use of this code or incorrect wiring. Batteries are highly dangerous. If you are unsure about what you are doing, consult a professional.

## 🤝 Credits & Acknowledgements
* **[hidaba](https://github.com/hidaba/PylontechMonitoring):** For the excellent groundwork, the initial code, and the custom libraries.
* **Google Gemini (AI):** For the extensive help with code optimization (robust parsing, memory management, ioBroker integration, and UI design).
