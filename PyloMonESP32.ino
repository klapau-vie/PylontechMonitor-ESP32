/********************************************************************************
 * Pylontech Battery Monitoring - ESP32 Port (Fixed)
 ********************************************************************************/

#include "PylontechMonitoring.h"
#include "batteryStack.h"
#include <WiFi.h>           
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>      
#include <circular_log.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Standard für ESP32 DevKits
#endif

// --- GLOBALS ---
#ifdef ENABLE_MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
#endif

// Puffer für Batterie-Antworten
char g_szRecvBuff[7000];

// Webserver und Logging
WebServer server(80); 
circular_log<7000> g_log;

// Baudrate der Batterie-Verbindung
int g_baudRate = 0;

// Definition der Seriellen Schnittstelle zur Batterie (Serial2)
HardwareSerial &BatterySerial = Serial2;

// Global instance of the battery stack data
batteryStack g_stack;

// Vorwärtsdeklarationen (wichtig, damit der Compiler die Funktionen kennt)
unsigned long os_getCurrentTimeSec();
void wakeUpConsole();
bool sendCommandAndReadSerialResponse(const char* pszCommand);
bool parsePwrResponse(const char* pStr);
void prepareJsonOutput(char* pBuff, int buffSize);
void handleRoot();
void handleLog();
void handleReq();
void handleJsonOut();
void mqttLoop();

// Logging Helper
void Log(const char* msg)
{
  g_log.Log(msg);
  Serial.println(msg); // Debug Output auf USB
}

// Power Metering Globals (optional)
unsigned long powerIN = 0;       
unsigned long powerOUT = 0;
unsigned long powerINWh = 0;    
unsigned long powerOUTWh = 0;

// --- SETUP ---
void setup() {
  // 1. Debug Serial (USB)
  Serial.begin(PYLON_BAUDRATE);
  Log("Starting ESP32 Pylontech Monitor...");

  // 2. Batterie Serial (Serial2 an Pins 16/17)
  BatterySerial.begin(PYLON_BAUDRATE, SERIAL_8N1, PY_RX_PIN, PY_TX_PIN);
  g_baudRate = PYLON_BAUDRATE;

  memset(g_szRecvBuff, 0, sizeof(g_szRecvBuff));
  
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, HIGH); 

  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);

#ifdef STATIC_IP
  WiFi.config(ip, gateway, subnet, dns); 
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  // Warten auf WiFi
  for(int ix=0; ix<10; ix++) {
    Log("Wait for WIFI Connection...");
    if(WiFi.status() == WL_CONNECTED) break;
    delay(1000);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
      Log("WiFi Connected!");
      Log(WiFi.localIP().toString().c_str());
  } else {
      Log("WiFi Connection Failed!");
  }

  ArduinoOTA.setHostname(WIFI_HOSTNAME);
  ArduinoOTA.begin();

  // Webserver Routen
  server.on("/", handleRoot);
  server.on("/log", handleLog);
  server.on("/req", handleReq);
  server.on("/jsonOut", handleJsonOut);
  server.on("/reboot", [](){
    #ifdef AUTHENTICATION
      if (!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
      }
    #endif
    server.send(200, "text/html", "Rebooting...");
    delay(250);
    ESP.restart();
  });
  
  server.begin(); 
  
#ifdef ENABLE_MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(2048); // Größerer Buffer für ESP32
#endif

  Log("Boot event finished");
}

// --- WEB HANDLERS ---

void handleLog()
{
#ifdef AUTHENTICATION
  if (!server.authenticate(www_username, www_password)) return server.requestAuthentication();
#endif
  server.send(200, "text/html", g_log.c_str());
}

void switchBaud(int newRate)
{
  if(g_baudRate == newRate) return;
  
  BatterySerial.flush();
  delay(20);
  BatterySerial.end();
  delay(20);

  char szMsg[50];
  snprintf(szMsg, sizeof(szMsg)-1, "Switching Battery Serial to: %d", newRate);
  Log(szMsg);

  BatterySerial.begin(newRate, SERIAL_8N1, PY_RX_PIN, PY_TX_PIN);
  g_baudRate = newRate;
  delay(20);
}

void waitForSerial()
{
  for(int ix=0; ix<150; ix++) {
    if(BatterySerial.available()) break;
    delay(10);
  }
}

int readFromSerial()
{
  memset(g_szRecvBuff, 0, sizeof(g_szRecvBuff));
  int recvBuffLen = 0;
  bool foundTerminator = false;

  waitForSerial();
  while(BatterySerial.available())
  {
    char szResponse[256] = "";
    const int readNow = BatterySerial.readBytesUntil('>', szResponse, sizeof(szResponse)-1);
    
    if(readNow > 0) 
    {
      if(readNow + recvBuffLen + 1 >= (int)(sizeof(g_szRecvBuff))) {
        Log("WARNING: Buffer overflow!");
        break;
      }

      strcat(g_szRecvBuff, szResponse);
      recvBuffLen += readNow;

      if(strstr(g_szRecvBuff, "$$\r\n\rpylon")) {
        strcat(g_szRecvBuff, ">");
        foundTerminator = true;
        break;
      }

      if(strstr(g_szRecvBuff, "Press [Enter] to be continued,other key to exit")) {
        BatterySerial.write("\r");
      }

      waitForSerial();
    }
    if (!BatterySerial.available()) delay(10); 
  }
  return recvBuffLen;
}

bool sendCommandAndReadSerialResponse(const char* pszCommand)
{
  switchBaud(PYLON_BAUDRATE);
  // Buffer leeren
  while(BatterySerial.available()) BatterySerial.read();

  if(pszCommand[0] != '\0') {
    BatterySerial.write(pszCommand);
  }
  BatterySerial.write("\n");
  
  const int recvBuffLen = readFromSerial();
  if(recvBuffLen > 0) return true;

  // Wakeup Versuch
  Log("No response, attempting wake up...");
  wakeUpConsole();
  
  if(pszCommand[0] != '\0') {
    BatterySerial.write(pszCommand);
  }
  BatterySerial.write("\n");

  return readFromSerial() > 0;
}

void handleReq()
{
#ifdef AUTHENTICATION
  if (!server.authenticate(www_username, www_password)) return server.requestAuthentication();
#endif

  if(server.hasArg("code") == false) {
    sendCommandAndReadSerialResponse("");
  } else {
    String cmd = server.arg("code");
    sendCommandAndReadSerialResponse(cmd.c_str());
    
    // WICHTIG: Übersetzt den Text in Zahlen, wenn "PWR Daten" geklickt wird!
    if(cmd == "pwr") {
      parsePwrResponse(g_szRecvBuff);
    }
  }
  handleRoot();
}

void handleJsonOut()
{
#ifdef AUTHENTICATION
  if (!server.authenticate(www_username, www_password)) return server.requestAuthentication();
#endif

  // Frische Daten von der Batterie holen
  if(sendCommandAndReadSerialResponse("pwr") == true) {
    parsePwrResponse(g_szRecvBuff);
  }

  // WICHTIG: Einen eigenen Puffer für das JSON nutzen
  char jsonBuff[512];
  prepareJsonOutput(jsonBuff, sizeof(jsonBuff));
  
  // ANTI-CACHING: Verbietet dem Browser, diese JSON-Daten zu speichern!
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.send(200, "application/json", jsonBuff);
}

void handleRoot() {
#ifdef AUTHENTICATION
  if (!server.authenticate(www_username, www_password)) return server.requestAuthentication();
#endif

  // Uptime berechnen
  unsigned long val = os_getCurrentTimeSec();
  unsigned long days = val / (3600*24);
  val -= days * (3600*24);
  unsigned long hours = val / 3600;
  val -= hours * 3600;
  unsigned long minutes = val / 60;

// -- NEU --
  // Großer Buffer auf Heap
  char* szTmp = (char*)malloc(10000); 
  if(!szTmp) { server.send(500, "text/plain", "OOM"); return; }

// 1. HTML Header & CSS (Kompaktes Dark Theme)
  snprintf(szTmp, 10000, 
    "<!DOCTYPE html><html lang='de'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Pylontech Monitor</title>"
    "<style>"
    "body{font-family:'Segoe UI',Roboto,Helvetica,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:10px;}" 
    "h1{color:#f8fafc;text-align:center;font-size:18px;margin-bottom:10px;letter-spacing:1px;}" 
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:10px;margin-bottom:12px;}" 
    ".card{background:#1e293b;padding:10px;border-radius:8px;box-shadow:0 4px 6px -1px rgba(0,0,0,0.3);text-align:center;border-top:3px solid #3b82f6;}" 
    ".card h3{margin:0 0 4px;color:#94a3b8;font-size:10px;letter-spacing:1px;text-transform:uppercase;}" 
    ".card p{margin:0;font-size:18px;font-weight:bold;color:#f8fafc;}" 
    ".btn{display:inline-block;background:#2563eb;color:#fff;padding:8px 12px;text-decoration:none;border-radius:6px;border:none;cursor:pointer;font-size:13px;font-weight:600;transition:all 0.2s;}"
    ".btn:hover{background:#1d4ed8;}"
    ".btn-danger{background:#dc2626;}"
    ".btn-danger:hover{background:#b91c1c;}"
    ".btn-batt{background:#059669;}" 
    ".btn-batt:hover{background:#047857;}"
    ".actions{display:flex;gap:8px;justify-content:center;margin-bottom:10px;flex-wrap:wrap;align-items:center;}" 
    ".console{background:#020617;color:#34d399;font-family:'Consolas',monospace;font-size:12px;width:100%%;height:70vh;padding:12px;border-radius:8px;border:1px solid #334155;box-sizing:border-box;resize:vertical;line-height:1.3;}" 
    "input[type='text']{padding:7px;border:1px solid #334155;background:#1e293b;color:#fff;border-radius:6px;width:160px;font-size:13px;outline:none;}"
    "input[type='text']:focus{border-color:#3b82f6;}"
    "</style></head><body>"
    "<h1>🔋 Pylontech Monitor</h1>"
  );

// 2. Info-Karten (Stats Grid) mit Live-Update Script
  char stats[1500];
  snprintf(stats, 1500, 
    "<div class='grid'>"
    "<div class='card' style='border-top-color:#f59e0b;'><h3>⚡ Leistung</h3><p id='ui-pwr'>%ld W</p></div>"
    "<div class='card' style='border-top-color:#10b981;'><h3>🔋 SoC</h3><p id='ui-soc'>%d %%</p></div>"
    "<div class='card' style='border-top-color:#ef4444;'><h3>🌡️ Temperatur</h3><p id='ui-temp'>%.1f &deg;C</p></div>"
    "<div class='card' style='border-top-color:#8b5cf6;'><h3>⏱️ Uptime</h3><p>%02dd %02dh %02dm</p></div>"
    "<div class='card' style='border-top-color:#0ea5e9;'><h3>📡 WiFi (%s)</h3><p>%ld dBm</p></div>"
    "</div>"
    // --- NEU: Anti-Caching Script ---
    "<script>"
    "setInterval(function(){"
    "  fetch('/jsonOut?t=' + Date.now())" // <-- WICHTIG: Zeitstempel trickst den Cache aus!
    "  .then(r=>r.json())"
    "  .then(d=>{"
    "    document.getElementById('ui-pwr').innerText = d.powerDC + ' W';"
    "    document.getElementById('ui-soc').innerText = d.soc + ' %';"
    "    document.getElementById('ui-temp').innerHTML = (d.temp / 1000.0).toFixed(1) + ' &deg;C';"
    "  })"
    "  .catch(e=>console.log('Update Error:', e));"
    "}, 10000);"
    "</script>",
    g_stack.getPowerDC(), 
    g_stack.soc, 
    (float)g_stack.temp / 1000.0,
    (int)days, (int)hours, (int)minutes, 
    WiFi.SSID().c_str(), WiFi.RSSI()
  );
  
  strncat(szTmp, stats, 10000 - strlen(szTmp) - 1);

// 3. Buttons & Kommando-Zeile (Eingabefeld integriert)
  strncat(szTmp, 
    "<div class='actions'>"
    // Das Formular steht jetzt direkt inline VOR den anderen Buttons
    "<form action='/req' method='get' style='display:inline-flex;gap:6px;margin:0;'>"
    "<input type='text' name='code' placeholder='Kommando...'/> "
    "<button type='submit' class='btn'>Senden</button>"
    "</form>"
    // Die normalen Buttons folgen danach
    "<a href='/req?code=pwr' class='btn'>PWR Daten</a>"
    "<a href='/req?code=help' class='btn'>Hilfe</a>"
    "<a href='/req?code=log' class='btn'>Event Log</a>"
    "<a href='/log' class='btn'>System Log</a>"
    "<a href='/reboot' onclick=\"return confirm('System wirklich neu starten?');\" class='btn btn-danger'>Reboot</a>"
    "</div>", 
    10000 - strlen(szTmp) - 1
  );

  // Batterie-Buttons dynamisch generieren
  strncat(szTmp, "<div class='actions'>", 10000 - strlen(szTmp) - 1);
  for(int i = 1; i <= MAX_PYLON_BATTERIES; i++) {
    char btnHtml[100];
    snprintf(btnHtml, sizeof(btnHtml), "<a href='/req?code=bat%%20%d' class='btn btn-batt'>Batt%02d</a>", i, i);
    strncat(szTmp, btnHtml, 10000 - strlen(szTmp) - 1);
  }
  strncat(szTmp, "</div>", 10000 - strlen(szTmp) - 1);

// 4. Konsolen-Ausgabe (Schwarzer Hintergrund, grüne Schrift)
  strncat(szTmp, "<textarea class='console' readonly>", 10000 - strlen(szTmp) - 1);
  strncat(szTmp, g_szRecvBuff, 10000 - strlen(szTmp) - 1);
  strncat(szTmp, "</textarea></body></html>", 10000 - strlen(szTmp) - 1);

  server.send(200, "text/html", szTmp);
  free(szTmp);
}

unsigned long os_getCurrentTimeSec()
{
  static unsigned int wrapCnt = 0;
  static unsigned long lastVal = 0;
  unsigned long currentVal = millis();

  if(currentVal < lastVal) wrapCnt++;
  lastVal = currentVal;
  
  unsigned long seconds = currentVal/1000;
  return (wrapCnt*4294967) + seconds;
}

void wakeUpConsole()
{
  switchBaud(1200);
  BatterySerial.write("~20014682C0048520FCC3\r");
  delay(1000);

  byte newLineBuff[] = {0x0E, 0x0A};
  switchBaud(PYLON_BAUDRATE);
  
  for(int ix=0; ix<10; ix++)
  {
    BatterySerial.write(newLineBuff, sizeof(newLineBuff));
    delay(1000);
    if(BatterySerial.available()) {
      while(BatterySerial.available()) BatterySerial.read();
      break;
    }
  }
}

// Helper parsing functions
long extractInt(const char* pStr, int pos) {
  return atol(pStr+pos);
}

void getColumn(const char* pLine, int colIndex, char* strOut, int strOutSize) {
  const char* p = pLine;
  int currentCol = 0;
  strOut[0] = '\0';
  
  while (*p) {
    while (isspace(*p)) p++; // Leerzeichen überspringen
    if (*p == '\0') break;
    
    const char* start = p;
    while (*p && !isspace(*p)) p++; // Bis zum Wort-Ende lesen
    
    if (currentCol == colIndex) {
      int len = p - start;
      if (len >= strOutSize) len = strOutSize - 1;
      strncpy(strOut, start, len);
      strOut[len] = '\0';
      return;
    }
    currentCol++;
  }
}

bool parsePwrResponse(const char* pStr)
{
  if(strstr(pStr, "Command completed successfully") == NULL) return false;

  int chargeCnt = 0, dischargeCnt = 0, idleCnt = 0, alarmCnt = 0;
  int socAvg = 0, socLow = 0, tempHigh = 0, tempLow = 0;

  memset(&g_stack, 0, sizeof(g_stack));

  for(int ix=0; ix<MAX_PYLON_BATTERIES; ix++)
  {
    char szToFind[32];
    snprintf(szToFind, sizeof(szToFind), "\r\r\n%d     ", ix+1);
    const char* pLineStart = strstr(pStr, szToFind);
    if(pLineStart == NULL) continue; 

    pLineStart += 3; 

    // -- NEU: Das spaltenbasierte Auslesen --
    // Base State ist immer das 9. Wort (Index 8)
    getColumn(pLineStart, 8, g_stack.batts[ix].baseState, sizeof(g_stack.batts[ix].baseState));
    
    if(strcmp(g_stack.batts[ix].baseState, "Absent") == 0) {
      g_stack.batts[ix].isPresent = false;
      continue; // Keine Batterie da -> abbrechen!
    } else {
      g_stack.batts[ix].isPresent = true;
      
      char tempBuf[16];
      // Zahlenwerte auslesen (Spalten 1 bis 7, sowie SoC an Spalte 12)
      getColumn(pLineStart, 1, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].voltage = atol(tempBuf);
      getColumn(pLineStart, 2, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].current = atol(tempBuf);
      getColumn(pLineStart, 3, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].tempr = atol(tempBuf);
      getColumn(pLineStart, 4, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].cellTempLow = atol(tempBuf);
      getColumn(pLineStart, 5, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].cellTempHigh = atol(tempBuf);
      getColumn(pLineStart, 6, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].cellVoltLow = atol(tempBuf);
      getColumn(pLineStart, 7, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].cellVoltHigh = atol(tempBuf);
      
      getColumn(pLineStart, 12, tempBuf, sizeof(tempBuf)); g_stack.batts[ix].soc = atol(tempBuf); // atol ignoriert automatisch das % Zeichen
    }

      g_stack.batteryCount++;
      g_stack.currentDC += g_stack.batts[ix].current;
      g_stack.avgVoltage += g_stack.batts[ix].voltage;
      socAvg += g_stack.batts[ix].soc;

      if(!g_stack.batts[ix].isNormal()) alarmCnt++;
      else if(g_stack.batts[ix].isCharging()) chargeCnt++;
      else if(g_stack.batts[ix].isDischarging()) dischargeCnt++;
      else if(g_stack.batts[ix].isIdle()) idleCnt++;
      else alarmCnt++;

      if(g_stack.batteryCount == 1) {
        socLow = g_stack.batts[ix].soc;
        tempLow  = g_stack.batts[ix].cellTempLow;
        tempHigh = g_stack.batts[ix].cellTempHigh;
      } else {
        if(socLow > g_stack.batts[ix].soc) socLow = g_stack.batts[ix].soc;
        if(tempHigh < g_stack.batts[ix].cellTempHigh) tempHigh = g_stack.batts[ix].cellTempHigh;
        if(tempLow > g_stack.batts[ix].cellTempLow) tempLow = g_stack.batts[ix].cellTempLow;
      }
    }

  if(g_stack.batteryCount > 0) {
      g_stack.avgVoltage /= g_stack.batteryCount;
      g_stack.soc = socLow;
      
      if(tempHigh > 15000) g_stack.temp = tempHigh;
      else g_stack.temp = tempLow;

      if(alarmCnt > 0) strcpy(g_stack.baseState, "Alarm!");
      else if(chargeCnt == g_stack.batteryCount) {
        strcpy(g_stack.baseState, "Charge");
        g_stack.soc = (int)(socAvg / g_stack.batteryCount);
      }
      else if(dischargeCnt == g_stack.batteryCount) strcpy(g_stack.baseState, "Dischg");
      else if(idleCnt == g_stack.batteryCount) strcpy(g_stack.baseState, "Idle");
      else strcpy(g_stack.baseState, "Balance");
  }
  return true;
}

void prepareJsonOutput(char* pBuff, int buffSize)
{
  memset(pBuff, 0, buffSize);
  snprintf(pBuff, buffSize-1, 
    "{\"soc\": %d, \"temp\": %d, \"currentDC\": %ld, \"avgVoltage\": %ld, \"baseState\": \"%s\", \"batteryCount\": %d, \"powerDC\": %ld, \"estPowerAC\": %ld, \"isNormal\": %s}",
    g_stack.soc, g_stack.temp, g_stack.currentDC, g_stack.avgVoltage, g_stack.baseState, 
    g_stack.batteryCount, g_stack.getPowerDC(), g_stack.getEstPowerAc(),
    g_stack.isNormal() ? "true" : "false");
}

// --- MAIN LOOP ---
void loop() {
#ifdef ENABLE_MQTT
  mqttLoop();
#endif
  ArduinoOTA.handle();
  server.handleClient();

  int bytesAv = BatterySerial.available();
  if(bytesAv > 0)
  {
    if(bytesAv > 63) bytesAv = 63;
    char buff[70] = "RCV:";
    if(BatterySerial.readBytes(buff+4, bytesAv) > 0) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); 
      Log(buff);
    }
  }
}

// --- MQTT IMPLEMENTATION ---
#ifdef ENABLE_MQTT
#define ABS_DIFF(a, b) (a > b ? a-b : b-a)

void mqtt_publish_f(const char* topic, float newValue, float oldValue, float minDiff, bool force) {
  char szTmp[16] = "";
  snprintf(szTmp, 15, "%.2f", newValue);
  if(force || ABS_DIFF(newValue, oldValue) > minDiff) {
    mqttClient.publish(topic, szTmp, false);
  }
}

void mqtt_publish_i(const char* topic, int newValue, int oldValue, int minDiff, bool force) {
  char szTmp[16] = "";
  snprintf(szTmp, 15, "%d", newValue);
  if(force || ABS_DIFF(newValue, oldValue) > minDiff) {
    mqttClient.publish(topic, szTmp, false);
  }
}

void mqtt_publish_s(const char* topic, const char* newValue, const char* oldValue, bool force) {
  if(force || strcmp(newValue, oldValue) != 0) {
    mqttClient.publish(topic, newValue, false);
  }
}

// --- AUTO-DISCOVERY FÜR IOBROKER / HOME ASSISTANT ---
void publishSensorDiscovery(const char* sensorId, const char* sensorName, const char* unit, const char* deviceClass, const char* stateTopic) {
  StaticJsonDocument<512> doc;
  doc["name"] = sensorName;
  doc["state_topic"] = stateTopic;
  doc["unique_id"] = String(WIFI_HOSTNAME) + "_" + sensorId;

  if (strlen(unit) > 0) doc["unit_of_measurement"] = unit;
  if (strlen(deviceClass) > 0) doc["device_class"] = deviceClass;

  JsonObject dev = doc.createNestedObject("device");
  dev["identifiers"] = String(WIFI_HOSTNAME);
  dev["name"] = String(WIFI_HOSTNAME);
  dev["manufacturer"] = "Pylontech";

  // Wichtig: Das Topic muss zwingend mit "homeassistant/sensor/" beginnen, damit Auto-Systeme es erkennen!
  String configTopic = String("homeassistant/sensor/") + String(WIFI_HOSTNAME) + "_" + sensorId + "/config";

  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(configTopic.c_str(), (const uint8_t*)buffer, n, true);
}

void publishAutoDiscovery() {
  // 1. Globale Werte anlegen
  publishSensorDiscovery("soc", "Pylontech SoC", "%", "battery", MQTT_TOPIC_ROOT "soc");
  publishSensorDiscovery("temp", "Pylontech Temperatur", "°C", "temperature", MQTT_TOPIC_ROOT "temp");
  publishSensorDiscovery("currentDC", "Pylontech Strom", "A", "current", MQTT_TOPIC_ROOT "currentDC");
  publishSensorDiscovery("powerDC", "Pylontech Leistung", "W", "power", MQTT_TOPIC_ROOT "getPowerDC");

  // 2. Dynamisch für alle physisch ERKANNTEN Batterien anlegen (Spannung & Status)
  for(int i = 0; i < g_stack.batteryCount; i++) {
    String idVolt = String("bat") + i + "_voltage";
    String nameVolt = String("Batterie ") + i + " Spannung";
    String topicVolt = String(MQTT_TOPIC_ROOT) + i + "/voltage";
    publishSensorDiscovery(idVolt.c_str(), nameVolt.c_str(), "V", "voltage", topicVolt.c_str());

    String idState = String("bat") + i + "_state";
    String nameState = String("Batterie ") + i + " Status";
    String topicState = String(MQTT_TOPIC_ROOT) + i + "/state";
    publishSensorDiscovery(idState.c_str(), nameState.c_str(), "", "", topicState.c_str());
  }
}

void pushBatteryDataToMqtt(const batteryStack& lastSentData, bool forceUpdate)
{
  mqtt_publish_f(MQTT_TOPIC_ROOT "soc", g_stack.soc, lastSentData.soc, 0, forceUpdate);
  mqtt_publish_f(MQTT_TOPIC_ROOT "temp", (float)g_stack.temp/1000.0, (float)lastSentData.temp/1000.0, 0.1, forceUpdate);
  mqtt_publish_i(MQTT_TOPIC_ROOT "currentDC", g_stack.currentDC, lastSentData.currentDC, 1, forceUpdate);
  mqtt_publish_i(MQTT_TOPIC_ROOT "getPowerDC", g_stack.getPowerDC(), lastSentData.getPowerDC(), 1, forceUpdate);
  
  for (int ix = 0; ix < g_stack.batteryCount; ix++) {
    char ixBuff[50];
    String ixBattStr = MQTT_TOPIC_ROOT + String(ix) + "/voltage";
    ixBattStr.toCharArray(ixBuff, 50);
    mqtt_publish_f(ixBuff, g_stack.batts[ix].voltage / 1000.0, lastSentData.batts[ix].voltage / 1000.0, 0, forceUpdate);
    
    // State
    ixBattStr = MQTT_TOPIC_ROOT + String(ix) + "/state";
    ixBattStr.toCharArray(ixBuff, 50);
    const char* newState = g_stack.batts[ix].isIdle() ? "Idle" : g_stack.batts[ix].isCharging() ? "Charging" : g_stack.batts[ix].isDischarging() ? "Discharging" : "Unknown";
    const char* oldState = lastSentData.batts[ix].isIdle() ? "Idle" : lastSentData.batts[ix].isCharging() ? "Charging" : lastSentData.batts[ix].isDischarging() ? "Discharging" : "Unknown";
    mqtt_publish_s(ixBuff, newState, oldState, forceUpdate);
  }
}

void mqttLoop()
{
  static unsigned long g_lastConnectionAttempt = 0;
  const char* topicLastWill = MQTT_TOPIC_ROOT "availability";

  if (!mqttClient.connected() && 
      (g_lastConnectionAttempt == 0 || os_getCurrentTimeSec() - g_lastConnectionAttempt > 60)) {
    if(mqttClient.connect(WIFI_HOSTNAME, MQTT_USER, MQTT_PASSWORD, topicLastWill, 1, true, "offline")) {
      Log("Connected to MQTT server");
      mqttClient.publish(topicLastWill, "online", true);
    } else {
      Log("Failed to connect to MQTT server.");
    }
    g_lastConnectionAttempt = os_getCurrentTimeSec();
  }

  static unsigned long g_lastDataSent = 0;
  if(mqttClient.connected() && 
     os_getCurrentTimeSec() - g_lastDataSent > MQTT_PUSH_FREQ_SEC)
  {
     if(sendCommandAndReadSerialResponse("pwr") == true) {
        static batteryStack lastSentData;
        static unsigned int callCnt = 0;
        parsePwrResponse(g_szRecvBuff);
        static bool discoverySent = false;
        if(!discoverySent && g_stack.batteryCount > 0) {
      publishAutoDiscovery();
      discoverySent = true;
      }
        bool forceUpdate = (callCnt % 20 == 0);
        pushBatteryDataToMqtt(lastSentData, forceUpdate);
        callCnt++;
        g_lastDataSent = os_getCurrentTimeSec();
        memcpy(&lastSentData, &g_stack, sizeof(batteryStack));
     }
  }
  mqttClient.loop();
}

#endif // ENABLE_MQTT
