#include <WiFi.h>
#include <ETH.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <Preferences.h>

// Nastavení WiFi Access Pointu
const char* ssid = "ESP32tester";
const char* password = "ESP32tester";

// Ethernet statická IP konfigurace
IPAddress local_IP;
IPAddress gateway(192, 168, 1, 1);    // Brána 
IPAddress subnet(255, 255, 255, 0);   // Maska podsítě

Preferences preferences;

// Vytvoření instance webového serveru na portu 80
WebServer server(80);

// Funkce pro inicializaci Ethernetu
void ethEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet started");
      ETH.setHostname("esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Ethernet IP: ");
      Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
  }
}

// Funkce pro zobrazení hlavní stránky
void handleRoot() {
  String html = "<html><head><title>ESP32 Tester</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; 
  html += "<style>body { margin: 0; display: flex; justify-content: center; align-items: center; flex-direction: column; height: 100vh; color: white; background-color: #282a2b; font-family: Arial; }";
  html += "#content { text-align: center; } button { padding: 10px 20px; background-color: #2CBBFA; color: white; border: none; border-radius: 5px; cursor: pointer; } button:hover { background-color: #1c7aa3; }";
  html += "footer { position: absolute; bottom: 10px; text-align: center; width: 100%; font-size: 12px; color: white; } footer a { color: #2CBBFA; text-decoration: none; } footer a:hover { text-decoration: underline; }";
  html += "</style></head>";
  html += "<body><div id='content'>";
  html += "<h1>ESP32 Tester</h1>";

  // Ethernet status
  if (ETH.linkUp()) {
    html += "<p><b>ETH IP:</b> " + ETH.localIP().toString() + "</p>";
  } else {
    html += "<p><b>ETH IP:</b> Not connected</p>";
  }

  // WiFi status
  html += "<p><b>AP IP:</b> " + WiFi.softAPIP().toString() + "<br>";
  html += "<b>AP SSID:</b> " + String(ssid) + "<br>"; 
  html += "<b>AP PSW:</b> " + String(password) + "</p>"; 

  html += "<button onclick=\"window.location='/sett'\">Settings</button>";
  html += "</div>";

  // Footer
  html += "<footer>2024 &copy; <a href='https://www.ok1kky.cz' target='_blank'>OK1KKY</a></footer>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSettings() {
  String html = "<html><head><title>Settings</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; 
  html += "<style>body { margin: 0; display: flex; justify-content: center; align-items: center; flex-direction: column; height: 100vh; color: white; background-color: #282a2b; font-family: Arial; }";
  html += "#content { text-align: center; } form { margin: 0; } input, button { margin: 10px; padding: 10px; } input { border: 1px solid #ccc; border-radius: 5px; } button { background-color: #2cbbfa; color: white; border: none; border-radius: 5px; cursor: pointer; } button:hover { background-color: #1c7aa3; } .button-container { display: flex; justify-content: center; gap: 10px; }";
  html += "footer { position: absolute; bottom: 10px; text-align: center; width: 100%; font-size: 12px; color: white; } footer a { color: #2CBBFA; text-decoration: none; } footer a:hover { text-decoration: underline; }";
  html += "</style></head>";
  html += "<body><div id='content'>";
  html += "<h1>Settings</h1>";
  html += "<form action='/sett' method='POST'>";
  html += "<label for='ip'>Set IP Address:</label><br>";
  html += "<input type='text' id='ip' name='ip' value='" + local_IP.toString() + "'><br>";
  html += "<div class='button-container'>";
  html += "<button type='submit'>Save</button>";
  html += "<button type='button' onclick=\"window.location='/'\">Back</button>";
  html += "</div></form>";
  html += "</div>";

  // Footer
  html += "<footer>2024 &copy; <a href='https://www.ok1kky.cz' target='_blank'>OK1KKY</a></footer>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Funkce pro zpracování POST požadavku ze stránky nastavení
void handleSettingsPost() {
  if (server.hasArg("ip")) {
    String newIP = server.arg("ip");
    local_IP.fromString(newIP);

    // Uložit IP do NVS
    preferences.begin("eth-config", false);
    preferences.putString("ip", newIP);
    preferences.end();

    // Restartovat Ethernet s novou IP
    ETH.config(local_IP, gateway, subnet);
    Serial.println("New IP Address Set: " + newIP);
  }
  server.sendHeader("Location", "/sett");
  server.send(303);
}

void setup() {
  // Zahájení komunikace přes Serial monitor
  Serial.begin(115200);
  Serial.println("Starting...");

  // Načtení uložené IP adresy
  preferences.begin("eth-config", true);
  String savedIP = preferences.getString("ip", "192.168.1.2");
  preferences.end();
  local_IP.fromString(savedIP);

  // Inicializace Ethernetu
  WiFi.onEvent(ethEvent);
  ETH.begin();
  ETH.config(local_IP, gateway, subnet);

  // Spuštění WiFi Access Pointu
  WiFi.softAP(ssid, password);
  Serial.printf("WiFi Access Point '%s' created.\n", ssid);

  // Nastavení webového serveru
  server.on("/", handleRoot);
  server.on("/sett", HTTP_GET, handleSettings);
  server.on("/sett", HTTP_POST, handleSettingsPost);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Zpracování klientů webového serveru
  server.handleClient();
}
