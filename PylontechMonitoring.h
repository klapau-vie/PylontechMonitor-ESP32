#ifndef PYLONTECH_H
#define PYLONTECH_H

// +++ START CONFIGURATION +++

// WICHTIG: WiFi Zugangsdaten
#define WIFI_SSID "---***---"
#define WIFI_PASS "---***---"
#define WIFI_HOSTNAME "PylontechMonitor"

// Statische IP (bei Bedarf einkommentieren)
//#define STATIC_IP
IPAddress ip(192, 168, 11, 10);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 11, 1);
IPAddress dns(192, 168, 11, 1);

// Authentifizierung (bei Bedarf einkommentieren)
#define AUTHENTICATION 1
const char* www_username = "admin";
const char* www_password = "---***---";

// MQTT aktivieren
#define ENABLE_MQTT

// MQTT Broker Einstellungen
#define MQTT_SERVER        "---***---"
#define MQTT_PORT          1883
#define MQTT_USER          "---***---"
#define MQTT_PASSWORD      "---***---"

#define MQTT_TOPIC_ROOT    "pylontech/"
#define MQTT_PUSH_FREQ_SEC 360

// Max Batterien
#define MAX_PYLON_BATTERIES 16

#define PYLON_BAUDRATE 115200 

// +++ ESP32 PIN DEFINITIONEN +++
// Wir nutzen Serial2 für die Batterie, damit USB für Debugging frei bleibt.
#define PY_RX_PIN 16  // Verbinde dies mit TX des MAX3232
#define PY_TX_PIN 17  // Verbinde dies mit RX des MAX3232

// +++ END CONFIGURATION +++

#endif // PYLONTECH_H