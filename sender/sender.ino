#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <driver/adc.h>
using namespace std;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const String KEYWORD_PLOT = "plot"; 
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
double limit = 155;
double doubleValue;
int sensorValue;   
int check = 0;
unsigned long timeVal;
String message = "";

void configSensor() {
  pinMode(BUILTIN_LED, OUTPUT);
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
  adc1_config_width(ADC_WIDTH_BIT_12);
}

void writeStrValue(String a) {
  pCharacteristic->setValue(a.c_str());
  pCharacteristic->notify();
}

void readDataStream() {
  doubleValue = adc1_get_raw(ADC1_CHANNEL_4);
  doubleValue = doubleValue*3.3/4095*100;
  Serial.println(doubleValue);
  if (doubleValue >= limit && check == 0){
    check = 1;
    writeStrValue(String(doubleValue) + ",1");
    timeVal = millis();
  }
  else if (check == 1 && millis() - timeVal >= 1500){
    check = 0;
    writeStrValue(String(doubleValue) + ",0");
  }
  else{
    writeStrValue(String(doubleValue) + ",2");
  }
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // 광고 객체 가져오기
    pAdvertising->start(); // 광고 시작
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.println("Received data:");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
      }
      Serial.println();
      limit = stod(value.c_str());
    }
  }
};

void setup() {
  Serial.begin(115200);
  configSensor();

  BLEDevice::init("ESP32_BLE_Device");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());

  BLE2902 *p2902 = new BLE2902();
  pCharacteristic->addDescriptor(p2902);

  pService->start();
  
  // Configure advertising data
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);         
  pAdvertising->addServiceUUID(pCharacteristic->getUUID());
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

void loop() {
  readDataStream();

  if (deviceConnected != oldDeviceConnected) {
    if (deviceConnected) {
      Serial.println("Device connected");
    } else {
      Serial.println("Device disconnected");
    }
    oldDeviceConnected = deviceConnected;
  }

  delay(20);
}
