#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#define RELAY_PIN 5

const char* ssid = "MyWiFiAP";
const char* password = "MyWiFiPassword";
const char* mqttServer = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
const char* mqttClientID = "gbrain";
const String mqttTopic = "gbrain";
unsigned long timeVal;
int check = 0;

ESP8266WebServer server(80);


WiFiClient espClient;
PubSubClient client(espClient);

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Wi-Fi Setting</h1>";
  html += "<form action=\"/save\" method=\"POST\">";
  html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
  html += "Password: <input type=\"password\" name=\"password\"><br>";
  html += "Topic: <input type=\"text\" name=\"topic\"><br>";
  html += "<input type=\"submit\" value=\"Save\">";
  html += "</form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String topic = server.arg("topic");

  // 와이파이 정보를 저장하고 설정을 적용하는 로직을 추가하면 됩니다.
  // 예: WiFi.begin(), while(WiFi.status() != WL_CONNECTED) 등

  String html = "<html><body>";
  html += "<h1>Wi-Fi Setting Info</h1>";
  html += "<p>SSID: " + ssid + "</p>";
  html += "<p>Password: " + password + "</p>";
  html += "<p>Topic: " + topic + "</p>";
  html += "</body></html>";
  String result = mqttTopic + "/" + topic;
  
  char mqttTopics[1000];
  strcpy(mqttTopics, result.c_str());

  server.send(200, "text/html", html);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT Broker...");
    if (client.connect(mqttClientID)) {
      Serial.println("Connected to MQTT Broker");

    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }



  // MQTT 메시지 수신 콜백 함수 등록
  client.setCallback(callback);
  // 주제에 대해 구독 시작
  client.subscribe(mqttTopics);
}

void setup() {
  Serial.begin(115200);

  // SoftAP 모드로 설정
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress ipAddress = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ipAddress);

  // 웹 서버 설정
  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();
  Serial.println("HTTP server started");

  
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
  server.handleClient();
  client.loop();
}


// MQTT 메시지 수신 콜백 함수
void callback(char* topic, byte* payload, unsigned int length) {

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();
  if ((char)payload[0] == '1' && check == 0){
    timeVal = millis();
    check = 1;
    digitalWrite(RELAY_PIN, HIGH);
  }
  else {
    check = 0;
    digitalWrite(RELAY_PIN, LOW);
  }

  if (millis() - timeVal >= 1500 && check = 1) {
    check = 0;
    digitalWrite(RELAY_PIN, LOW);
  }
}