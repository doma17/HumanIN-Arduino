#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define RELAY_PIN 5
#define LED_PIN 4  // 내장 LED 핀 번호 정의


/// !!! -> Need to be changed for each device
/// /// -> No action

const char* ssid = "G-BRAIN_GH7";         /// !!! KYU IL LEE: WiFi from the receiver
const char* password = "gbrain0814";      /// /// KYU IL LEE: Password for access to WiFi from the receiver
const char* mqttServer = "61.101.55.94";  /// /// KYU IL LEE: Gbrain Company IP from Internet Provider (Can be changed)
const int mqttPort = 1883;                /// /// KYU IL LEE: MQTT port (default / no change)

// receiver Serial Code를 넣어주세요! eg) gbrain_GH8
const char* mqttClientID = "GH7_receiver";  /// !!! KYU IL LEE: MQTT Client ID for the receiver (GH8: MQTT Client ID for the app on the mobile)
const char* mqttUsername = "gbrain";        /// /// KYU IL LEE: User ID set up in the MQTT server (no change)
const char* mqttPassword = "gbrain";        /// /// KYU IL LEE: User Password for User ID (no change)

const String topic = "GH7";             /// !!! KYU IL LEE: usage? maybe "topic" name
const String baseMqttTopic = "gbrain";  /// /// KYU IL LEE: Keep as it is (usage?)
const char* MQTTTopic = "gbrain/GH7";   /// !!! KYU IL LEE: Added for MQTT subscription

/// /// (Paragraph 1)
/// /// After uploading the code to the receiver Arduino board,
/// /// the user needs to connect to the WiFi network (from the receiver) whose ssid (eg. G-BRAIN_GH8) is shown in the above
/// /// with the password from user's mobile device.
/// ///
/// /// (Paragraph 2)
/// /// After connected to the WiFi from the receiver,
/// /// the user needs to connect to 192.168.4.1 (browser)
/// /// and put in user's WiFi ssid (eg. Gbrain) and password,
/// /// and the topic name (eg. GH8).
/// /// Click the save button and the receiver board will start to operate.
/// ///
/// /// Paragraph 2 is required to be repeated for every turn-on of the receiver board.
/// ///
/// /// (Paragraph 3)
/// /// The user (mobile device) needs to disconnect from the WiFi from the receiver
/// /// and connect to the user's own WiFi.
/// /// Then, open the HumanIn App. and choose the working bluetooth device (sender, eg. GH8).
/// /// The sender will start to work to publish the signal to the MQTT broker,
/// /// and the receiver will receive (subscribe) the published signal to control TENS.

unsigned long timeVal;
int check = 0;

// 연결 시간 초과 변수
unsigned long timeOutVal = 15000;

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

void handleWiFiConnect() {
  DynamicJsonDocument jsonBuffer(1024);
  deserializeJson(jsonBuffer, server.arg("plain"));

  String ssid = jsonBuffer["ssid"].as<String>();
  String password = jsonBuffer["password"].as<String>();
  String topic = jsonBuffer["topic"].as<String>();
  String result = baseMqttTopic + "/" + topic;

  // char mqttTopic[1000];
  // strcpy(mqttTopic, result.c_str());
  const char* mqttTopic = result.c_str();
  Serial.print("[handleWiFiConnect] SSID = ");
  Serial.println(ssid);
  Serial.print("[handleWiFiConnect] Password = ");
  Serial.println(password);
  Serial.print("[handleWiFiConnect] MqttTopic = ");
  Serial.println(mqttTopic);

  WiFi.begin(ssid, password);

  unsigned long wifiConnectTimeVal = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiConnectTimeVal > timeOutVal) {
      Serial.println("[handleWiFiConnect] Not Connect WIFI. Please check ssid or password");
      server.send(400, "text/plain", "Not Connect WIFI. Please check ssid or password");
      return;
    }

    blinkLED(1000);

    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  client.setServer(mqttServer, mqttPort);

  unsigned long mqttConnectTimeVal = millis();

  while (!client.connected()) {
    Serial.println("[handleWiFiConnect] Connecting to MQTT Broker...");

    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT Broker");
      Serial.println(mqttClientID);
      Serial.println(mqttUsername);
      Serial.println(client.connected());
    } else {
      if (millis() - mqttConnectTimeVal > timeOutVal) {
        Serial.println("Not Connect MQTT Server. Please check receiver code");
        server.send(500, "text/plain", "Not Connect MQTT Server. Please check receiver code");
        return;
      }

      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");

      blinkLED(5000);
    }
  }


  // MQTT 메시지 수신 콜백 함수 등록
  client.setCallback(callback);
  // 주제에 대해 구독 시작
  // client.subscribe(mqttTopic);

  /// /// KYU IL LEE: Changed for variable type matching
  // client.subscribe(MQTTTopic);
  client.subscribe(mqttTopic);

  server.send(200, "text/plain", "Connect Success!!");
}

// 입력한 Time 만큼 Delay되면서 LED 점등
void blinkLED(int time) {
  int delayTime = time / 2;

  digitalWrite(LED_PIN, HIGH);
  delay(delayTime);
  digitalWrite(LED_PIN, LOW);
  delay(delayTime);
}

void handleRestart() {
  Serial.println("restart...");
  server.send(200, "text/plain", "restart!!");

  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  // SoftAP 모드로 설정
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress ipAddress = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ipAddress);

  // 서버 설정
  server.on("/api/ssid", HTTP_POST, handleWiFiConnect);
  server.on("/api/restart", HTTP_GET, handleRestart);

  server.begin();
  Serial.println("API server started");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // LED 켜기
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // LED 켜기
  digitalWrite(LED_PIN, HIGH);

  server.handleClient();
  client.loop();
}


// MQTT 메시지 수신 콜백 함수
void callback(char* topic, byte* payload, unsigned int length) {

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  if ((char)payload[0] == '1' && check == 0) {
    timeVal = millis();
    check = 1;
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    check = 0;
    digitalWrite(RELAY_PIN, LOW);
  }

  if (millis() - timeVal >= 1500 && check == 1) {
    check = 0;
    digitalWrite(RELAY_PIN, LOW);
  }
}