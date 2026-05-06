// =========================
// INCLUDES DE BIBLIOTECAS
// =========================
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include "DHTesp.h"
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>


// =========================
// CONFIGURACAO DO WIFI
// =========================

//#define WIFI_SSID "FIAP-IOT"
//#define WIFI_PASSWORD "F!@p25.IOT"
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// =========================
// Variaveis
// ========================
LiquidCrystal_I2C lcd(0x27, 16, 2);
const char* WEATHER_URL_TEMP_SP =
  "https://api.open-meteo.com/v1/forecast?latitude=-23.55&longitude=-46.63&current=temperature_2m";

const char* WEATHER_URL_HUMI_SP =
  "https://api.open-meteo.com/v1/forecast?latitude=-23.55&longitude=-46.63&current=relative_humidity_2m";

const int buttomSP = 26;
const int buttom2 = 27;

const int DHT_PIN = 18;

int escolha = 0;
DHTesp dhtSensor;

WebServer server(80);
 
const int LED1 = 26;
const int LED2 = 27;
 
bool led1State = false;
bool led2State = false;

// =========================
// funções de configuração e auxiliares
// ========================

String boolToJson(bool value) {
  return value ? "true" : "false";
}

void sendJsonStatus() {
  String response = "{";
  response += "\"led1\":" + boolToJson(led1State) + ",";
  response += "\"led2\":" + boolToJson(led2State);
  response += "}";

  server.send(200, "application/json", response);
}

void sendJsonTempHumi(String temp, String humi) {
  String response = "{";
  response += "\"Source\":DHT22," ;
  response += "\"Temperatura\":" + temp + ",";
  response += "\"Umidade\":" + humi;
  response += "\"Status\":OK" ;
  response += "}";

  server.send(200, "application/json", response);
}

void sendJsonLedResponse(int ledNumber, bool state) {
  String response = "{";
  response += "\"ok\":true,";
  response += "\"led\":" + String(ledNumber) + ",";
  response += "\"state\":" + boolToJson(state);
  response += "}";

  server.send(200, "application/json", response);
}

void sendJsonError(String message, int code = 400) {
  String response = "{";
  response += "\"ok\":false,";
  response += "\"error\":\"" + message + "\"";
  response += "}";

  server.send(code, "application/json", response);
}
float makeGetRequestTemp(const char* url, String cidade) {
  HTTPClient http;

  Serial.println("\n--- FAZENDO GET ---");
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.println("Erro na requisicao");
    http.end();
    return 0;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("Erro no JSON");
    return 0;
  }

  float temperatura = doc["current"]["temperature_2m"];

 return temperatura;
}
float makeGetRequestHumi(const char* url, String cidade) {
  HTTPClient http;

  Serial.println("\n--- FAZENDO GET ---");
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.println("Erro na requisicao");
    http.end();
    return 0;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("Erro no JSON");
    return 0;
  }
  float humidity = doc["current"]["relative_humidity_2m"];

 return humidity;
}

void setup() {
  Serial.begin(115200);
  pinMode(buttomSP, INPUT_PULLUP);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }

  lcd.init();                   
  lcd.backlight();
  
  Serial.println(" Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  server.on("/api/status", HTTP_GET, []() {
    sendJsonStatus();
  });

  server.on(UriBraces("/api/led/{}/{}"), HTTP_POST, []() {
    String led = server.pathArg(0);
    String action = server.pathArg(1);

    int ledNumber = led.toInt();

    if (ledNumber != 1 && ledNumber != 2) {
      sendJsonError("LED invalido", 404);
      return;
    }

    if (action != "on" && action != "off") {
      sendJsonError("Acao invalida", 400);
      return;
    }

    bool newState = (action == "on");

    if (ledNumber == 1) {
      led1State = newState;
      digitalWrite(LED1, led1State);
      sendJsonLedResponse(1, led1State);
    } else {
      led2State = newState;
      digitalWrite(LED2, led2State);
      sendJsonLedResponse(2, led2State);
    }
  });

  server.on("/api/sensor/temphumi", HTTP_GET, []() {
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    String temp = String(data.temperature);
    String humi = String(data.humidity);
    sendJsonTempHumi(temp, humi);
  });

  server.onNotFound([]() {
    sendJsonError("Rota nao encontrada", 404);
  });

  server.begin();
  Serial.println("HTTP server started");

  if (digitalRead(buttomSP) == HIGH) {
    escolha = 1;

    Serial.print(escolha);
  switch (escolha) {
      case 0:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("IP:");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        break;
      case 1:
        lcd.clear();
        float temp = makeGetRequestTemp(WEATHER_URL_TEMP_SP, "Sao Paulo");
        float humi = makeGetRequestHumi(WEATHER_URL_HUMI_SP, "Sao Paulo");
        lcd.setCursor(0, 0);
        lcd.print("Temp em SP: ");
        lcd.print(temp);
        lcd.setCursor(0, 1);
        lcd.print("Humi em SP: ");
        lcd.print(humi);
        break;
    };
  };
}

// =========================
// função principal loop no final do código
// ========================
void loop() {
  
  server.handleClient();
}