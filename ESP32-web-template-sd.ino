#include <WiFi.h>
#include <WiFiMulti.h>

#include <WebSocketsServer.h>
#include <Husarnet.h>
#include <ArduinoJson.h>

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include <SPI.h>

#define HTTP_PORT 8000
#define WEBSOCKET_PORT 8001

const int BUTTON_PIN = 18;
const int LED_PIN = 17;

const int SDCS_PIN = 22;

typedef struct {
  char ssid[30];
  char pass[30];
} WifiNetwork;

struct WifiConfig {
  WifiNetwork net[10];
};

struct HusarnetConfig {
  char hostname[30];
  char joincode[30];
};

WifiConfig wifi_conf;
HusarnetConfig husarnet_conf;

int wifi_net_no = 0;

WiFiMulti wifiMulti;

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
HusarnetServer server(HTTP_PORT);

//Use https://arduinojson.org/v5/assistant/ to compute buffer size
StaticJsonBuffer<200> jsonBufferTx;
StaticJsonBuffer<100> jsonBufferRx;
StaticJsonBuffer<500> jsonBufferSettings;

JsonObject& rootTx = jsonBufferTx.createObject();

char* html;
bool wsconnected = false;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      {
        wsconnected = false;
        Serial.printf("ws [%u] Disconnected\r\n", num);
      }
      break;
    case WStype_CONNECTED:
      {
        wsconnected = true;
        Serial.printf("ws [%u] Connection from Husarnet\r\n", num);
      }
      break;

    case WStype_TEXT:
      {
        Serial.printf("ws [%u] Text:\r\n", num);
        for (int i = 0; i < length; i++) {
          Serial.printf("%c", (char)(*(payload + i)));
        }
        Serial.println();

        JsonObject& rootRx = jsonBufferRx.parseObject(payload);
        jsonBufferRx.clear();

        uint8_t ledState = rootRx["led"];

        Serial.printf("LED state = %d\r\n", ledState);
        if (ledState == 1) {
          digitalWrite(LED_PIN, HIGH);
        }
        if (ledState == 0) {
          digitalWrite(LED_PIN, LOW);
        }
      }
      break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      Serial.printf("ws [%u] other event\r\n", num);
      break;
  }

}

void writeFile(fs::FS &fs, const char * path, const char * message);

void taskWifi( void * parameter );
void taskHTTP( void * parameter );
void taskWebSocket( void * parameter );
void taskStatus( void * parameter );

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // init SD card
  SD.begin(SDCS_PIN);
  if (!SD.begin(SDCS_PIN)) {
    Serial.println("Attach SD card and restart");
    return;
  }

  uint8_t cardType = SD.cardType();
  switch (cardType) {
    case CARD_NONE:
    case CARD_UNKNOWN:
      Serial.println("Attach SD card and restart");
      return;
    case CARD_SD:
      Serial.println("SD card detected");
      break;
    case CARD_SDHC:
      Serial.println("SDHC card detected");
      break;
  }

  //read html file from SD card
  File file = SD.open("/index.htm");
  if (!file) {
    Serial.println("index.htm file doesn't exist. Save it on SD card and restart.");
    return;
  }
  Serial.printf("index.htm file size: %d\r\n", file.size());
  html = new char [file.size() + 1];
  file.read((uint8_t*)html, file.size());
  html[file.size()] = 0;  //make a CString
  file.close();

  //read network credentials from SD card
  file = SD.open("/settings.js");
  if (!file) {
    Serial.println("settings.js file doesn't exist. Save it on SD card and restart.");
    return;
  }
  Serial.printf("settings.js file size: %d\r\n", file.size());
  char* setting_json = new char [file.size() + 1];
  file.read((uint8_t*)setting_json, file.size());
  setting_json[file.size()] = 0;  //make a CString
  file.close();

  JsonObject& rootSettings = jsonBufferSettings.parseObject(setting_json);
  String husarnet_hostname = rootSettings["husarnet"]["hostname"];
  String husarnet_joincode = rootSettings["husarnet"]["joincode"];
  strcpy (husarnet_conf.hostname, husarnet_hostname.c_str());
  strcpy (husarnet_conf.joincode, husarnet_joincode.c_str());
  Serial.println();
  Serial.println("husarnet");
  Serial.printf("hostname: %s\r\n", husarnet_conf.hostname);
  Serial.printf("joincode: %s\r\n", husarnet_conf.joincode);

  JsonArray& wifinetworks = rootSettings["wifi"];
  wifi_net_no = wifinetworks.size();
  Serial.println();
  Serial.printf("number of Wi-Fi networks: %d\r\n", wifi_net_no);

  if (wifi_net_no > 10) {
    Serial.println("too many WiFi networks on SD card, 10 max");
    wifi_net_no = 10;
  }

  for (int i = 0; i < wifi_net_no; i++) {
    String ssid = wifinetworks[i]["ssid"];
    String pass = wifinetworks[i]["pass"];
    wifiMulti.addAP(ssid.c_str(), pass.c_str());

    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssid.c_str(), pass.c_str());
  }

  xTaskCreate(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */

  xTaskCreate(
    taskHTTP,          /* Task function. */
    "taskHTTP",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    2,                /* Priority of the task. */
    NULL);            /* Task handle. */

  xTaskCreate(
    taskWebSocket,          /* Task function. */
    "taskWebSocket",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */

  xTaskCreate(
    taskStatus,          /* Task function. */
    "taskStatus",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */
}

void taskWifi( void * parameter ) {
  while (1) {
    uint8_t stat = wifiMulti.run();
    if (stat == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      Husarnet.join(husarnet_conf.joincode, husarnet_conf.hostname);
      Husarnet.start();

      server.begin();

      while (WiFi.status() == WL_CONNECTED) {
        delay(500);
      }
    } else {
      Serial.printf("WiFi error: %d\r\n", (int)stat);
      delay(500);
    }
  }
}

void taskHTTP( void * parameter )
{
  String header;

  while (1) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    HusarnetClient client = server.available();

    if (client) {
      Serial.println("New Client.");
      String currentLine = "";
      Serial.printf("connected: %d\r\n", client.connected());
      while (client.connected()) {

        if (client.available()) {
          char c = client.read();
          Serial.write(c);
          header += c;
          if (c == '\n') {
            if (currentLine.length() == 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              client.println(html);
              break;
            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
        }
      }

      header = "";

      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    } else {
      delay(200);
    }
  }
}

void taskWebSocket( void * parameter )
{
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  while (1) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    webSocket.loop();
    delay(1);
  }
}

void taskStatus( void * parameter )
{
  String output;
  int cnt = 0;
  uint8_t button_status = 0;
  while (1) {
    if (wsconnected == true) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        button_status = 1;
      } else {
        button_status = 0;
      }
      output = "";

      rootTx["counter"] = cnt++;
      rootTx["button"] = button_status;
      rootTx.printTo(output);

      Serial.print(F("Sending: "));
      Serial.print(output);
      Serial.println();

      webSocket.sendTXT(0, output);
    }
    delay(100);
  }
}

void loop()
{
  delay(5000);
}
