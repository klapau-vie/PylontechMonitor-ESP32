# 🔋 Pylontech Battery Monitor (ESP32)

*[🇬🇧 English Version below](#english-version)*

Ein schlanker, stabiler und responsiver ESP32-Datenlogger zum Auslesen von Pylontech-Batterien über den Konsolen-Port. Dieses Projekt wurde speziell für eine **saubere Integration in ioBroker** via MQTT optimiert. 

Dieses Projekt ist ein Fork / eine Weiterentwicklung des ursprünglichen Projekts von **[hidaba](https://github.com/hidaba/PylontechMonitoring)**. 

![Web UI Screenshot](dashboar.png)

## ✨ Features & Neuerungen
* **ioBroker & HA ready:** Sendet rohe, saubere MQTT-Werte. Überträgt Auto-Discovery-Daten, sodass der Objektbaum vollautomatisch und fehlerfrei aufgebaut wird.
* **Modernes Web-Dashboard:** Integriertes Dark-Mode Dashboard (mit AJAX) zur flüssigen Live-Anzeige von Leistung (W), SoC (%), Temperatur und System-Uptime.
* **Bis zu 16 Batterien:** Erkennt dynamisch in Reihe geschaltete Batterien. Leere Slots ("Absent") werden intelligent ignoriert.
* **Smart Parsing:** Ein robuster Token-Parser liest die Konsolen-Ausgaben zuverlässig aus – unabhängig von Firmware-Versionen oder Formatierungs-Verschiebungen.

## 🛠️ Hardware & Verkabelung
**Getestete Hardware:** Erfolgreich getestet mit **Pylontech US2000C**.
Das Auslesen erfolgt über einen ESP32 und ein RS232-zu-TTL Modul (z. B. MAX3232). 

👉 **Für den genauen Schaltplan und die RJ45-Pinbelegung schau bitte in das [Original-Repository von hidaba](https://github.com/hidaba/PylontechMonitoring).** Dort ist die Verkabelung perfekt und bebildert erklärt.

## 🚀 Installation
1. Lade das Repository als ZIP herunter.
2. Öffne die `PyloMonESP32.ino` in der Arduino IDE.
3. Installiere die benötigten Bibliotheken: `PubSubClient`, `ArduinoJson`.
4. Öffne die **`PylontechMonitoring.h`** und trage deine WLAN- und MQTT-Daten ein. Passe ggf. `MAX_PYLON_BATTERIES` an dein Setup an.
5. Flashe den Code auf den ESP32.

## ⚠️ Haftungsausschluss (Disclaimer)
**Die Nutzung dieses Sketches erfolgt absolut auf eigene Gefahr!** Ich übernehme keinerlei Haftung oder Verantwortung für Schäden an deiner Batterie, deinem Wechselrichter, deinem Haus oder Personen, die durch die Nutzung dieses Codes oder fehlerhafte Verkabelung entstehen. Batterien bergen ein hohes Gefahrenpotenzial. Wenn du dir unsicher bist, ziehe eine Fachkraft zurate.

## 💡 Credits & Danksagung
* **[hidaba](https://github.com/hidaba/PylontechMonitoring):** Für die hervorragende Ausgangsbasis und den initialen Code.
* **Google Gemini (AI):** Für die Hilfe bei der Code-Optimierung (robuster Parser, Memory-Management, ioBroker-Integration und UI-Design).

---
---

<a name="english-version"></a>
# 🇬🇧 English Version

A lightweight, stable, and responsive ESP32 data logger for reading Pylontech batteries via the console port. This project was heavily optimized for a **clean integration into ioBroker** via MQTT.

This project is a fork / further development of the original project by **[hidaba](https://github.com/hidaba/PylontechMonitoring)**.

## ✨ Features & Updates
* **ioBroker & HA ready:** Sends clean MQTT data. Transmits Auto-Discovery payloads so your smart home automatically builds the perfect object tree without errors.
* **Modern Web Dashboard:** Integrated dark-mode dashboard (using AJAX) for smooth, live monitoring of Power (W), SoC (%), Temperature, and System Uptime.
* **Up to 16 Batteries:** Dynamically detects connected batteries in a stack. Empty slots ("Absent") are intelligently ignored.
* **Smart Parsing:** A robust token-based parser reliably reads console outputs, regardless of Pylontech firmware versions or formatting shifts.

## 🛠️ Hardware & Wiring
**Tested Hardware:** Successfully tested with **Pylontech US2000C**.
Reading is done via an ESP32 and an RS232-to-TTL module (e.g., MAX3232).

👉 **For the exact wiring diagram and RJ45 pinout, please refer to the [original repository by hidaba](https://github.com/hidaba/PylontechMonitoring).** The wiring is explained perfectly with images there.

## 🚀 Installation
1. Download this repository as a ZIP file.
2. Open `PyloMonESP32.ino` in the Arduino IDE.
3. Install the required libraries: `PubSubClient`, `ArduinoJson`.
4. Open **`PylontechMonitoring.h`** and insert your WiFi and MQTT credentials. Adjust `MAX_PYLON_BATTERIES` if necessary.
5. Flash the code to your ESP32.

## ⚠️ Disclaimer
**Use this sketch strictly at your own risk!**
I take no liability or responsibility for any damage to your battery, inverter, property, or persons caused by the use of this code or incorrect wiring. Batteries are highly dangerous. If you are unsure about what you are doing, consult a professional.

## 💡 Credits & Acknowledgements
* **[hidaba](https://github.com/hidaba/PylontechMonitoring):** For the excellent groundwork and the initial code.
* **Google Gemini (AI):** For the help with code optimization (robust parsing, memory management, ioBroker integration, and UI design).
