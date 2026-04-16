#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <BH1750FVI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "Web_air.h"   // giao diện web chính
#include "wifi_config.h" // giao diện config Wi-Fi 

// ====== Cảm biến SHT31 ======
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// ====== Cảm biến BH1750 ======
uint8_t ADDRESSPIN = 13;
BH1750FVI::eDeviceAddress_t DEVICEADDRESS = BH1750FVI::k_DevAddress_L;
BH1750FVI::eDeviceMode_t DEVICEMODE = BH1750FVI::k_DevModeContHighRes;
BH1750FVI LightSensor(ADDRESSPIN, DEVICEADDRESS, DEVICEMODE);

// ====== Cảm biến analog ======
#define RAIN_PIN       33   
#define WIND_DIR_PIN   35   
#define WIND_SPEED_PIN 32   

// ====== WiFi & Server ======
#define EEPROM_SIZE 96
WebServer server(80);

bool uartActive = false;
unsigned long lastUartTime = 0;
bool connected = false;   

// Wi-Fi mặc định
const char* hardcoded_ssid = "American Study // HD"; 
const char* hardcoded_pass = "66668888";

// AP config khi không kết nối được
const char* my_default_ssid = "ESP01_AQM";
const char* my_default_pass = "12345678";

// IP tĩnh
IPAddress local_IP(192, 168, 1, 179);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// ====== Hàm đọc cảm biến ======
float readTemperature() { return sht31.readTemperature(); }
float readHumidity()    { return sht31.readHumidity(); }
uint16_t readBH1750()   { return LightSensor.GetLightIntensity(); }

String readRainSensor() {
  int rainVal = digitalRead(RAIN_PIN);
  return (rainVal > 0) ? "Dry" : "Rain";
}

String readWindDirection() {
  int Direction = analogRead(WIND_DIR_PIN);
  if (Direction < 200) return "N";
  else if (Direction < 500) return "NE";
  else if (Direction < 1000) return "E";
  else if (Direction < 1500) return "SE";
  else if (Direction < 2000) return "S";
  else if (Direction < 2500) return "SW";
  else if (Direction < 3000) return "W";
  else if (Direction <= 3500) return "NW";
  return "Unknown";
}

float readWindSpeed() {
  int Speed = analogRead(WIND_SPEED_PIN);
  return Speed * 0.00733;
}

String readWeather(uint16_t lux, String rain){
  if (rain == "Rain") return "Rainy";
  else if (lux < 50) return "Dark";
  else if (lux < 500) return "Cloudy";
  return "Sunny";
}

// ====== Web Handler ======
void handleRoot() { server.send_P(200, "text/html", MAIN_page); }

void handleData() {
  float temperature = readTemperature();
  float humidity = readHumidity();
  uint16_t lux = readBH1750();
  String rain = readRainSensor();
  String weather = readWeather(lux, rain);
  String direc = readWindDirection();
  float windspeed = readWindSpeed();

  String json = "{";
  json += "\"temperature\":" + String(temperature, 2) + ",";
  json += "\"humidity\":" + String(humidity, 2) + ",";
  json += "\"Light\":" + String(lux) + ",";
  json += "\"weather\":\"" + weather + "\",";
  json += "\"direc\":\"" + direc + "\",";
  json += "\"speed\":" + String(windspeed, 2);
  json += "}";
  server.send(200, "application/json", json);
}

// ====== EEPROM Wi-Fi ======
String storedSSID = "";
String storedPASS = "";

void saveWiFiConfig(String ssid, String pass) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) EEPROM.write(i, (i < ssid.length()) ? ssid[i] : 0);
  for (int i = 0; i < 64; i++) EEPROM.write(32 + i, (i < pass.length()) ? pass[i] : 0);
  EEPROM.commit();
}

void loadWiFiConfig() {
  EEPROM.begin(EEPROM_SIZE);
  char ssid[33], pass[65];
  for (int i = 0; i < 32; i++) ssid[i] = EEPROM.read(i);
  for (int i = 0; i < 64; i++) pass[i] = EEPROM.read(32 + i);
  ssid[32] = 0;
  pass[64] = 0;
  storedSSID = String(ssid);
  storedPASS = String(pass);
}

bool connectWiFi(String ssid, String pass) {
  WiFi.disconnect(true);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Lỗi cấu hình IP tĩnh");
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("Đang kết nối tới Wi-Fi: %s\n", ssid.c_str());

  unsigned long start = millis();
  while (millis() - start < 20000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("✅ Kết nối thành công! IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n❌ Kết nối thất bại.");
  return false;
}

void startAPConfig() {
  IPAddress apIP(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  WiFi.softAP(my_default_ssid, my_default_pass);

  Serial.printf("AP Config → SSID: %s, Pass: %s\n", my_default_ssid, my_default_pass);
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", []() {
    server.send(200, "text/html", configPage);
  });
  server.on("/scanwifi", []() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i) json += ",";
      json += "\"" + WiFi.SSID(i) + "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  server.on("/savewifi", []() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
      saveWiFiConfig(server.arg("ssid"), server.arg("pass"));
      server.send(200, "text/plain", "Lưu thành công. ESP sẽ khởi động lại.");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "Thiếu tham số");
    }
  });
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  Wire.begin();

  loadWiFiConfig();

  if (storedSSID.length() > 0) {
    connected = connectWiFi(storedSSID, storedPASS);
  }
  if (!connected) {
    connected = connectWiFi(hardcoded_ssid, hardcoded_pass);
  }
  if (!connected) {
    startAPConfig();
  }

  if (!sht31.begin(0x44)) {
    Serial.println("⚠️ Không tìm thấy SHT31!");
  }
  LightSensor.begin();

  pinMode(RAIN_PIN, INPUT);
  pinMode(WIND_DIR_PIN, INPUT);
  pinMode(WIND_SPEED_PIN, INPUT);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("✅ WebServer started!");
}

// ====== Loop ======
void loop() {
  server.handleClient();
}
