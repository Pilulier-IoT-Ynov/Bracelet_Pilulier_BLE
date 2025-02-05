#include <Arduino.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2904.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

#define LED_PIN 2
#define BUTTON_PIN 1
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "12345678-1234-1234-1234-123456789abc"

#define SDA 5
#define SCL 6

Adafruit_DRV2605 drv;
BLECharacteristic *pCharacteristic;

String dataValue = "OFF";

class MyServerCallbacks : public BLEServerCallbacks
{
public:
  void onConnect(BLEServer *pServer)
  {
    Serial.println("Client connected");
    Serial.flush(); // Ensure the message is completely sent
  }

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Client disconnected, restarting ESP32...");
    Serial.flush(); // Ensure the message is completely sent
    delay(1000);    // Wait a moment before restarting
    ESP.restart();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value == "ON")
    {
      digitalWrite(LED_PIN, HIGH);
      dataValue = "ON";
    
    }
    else if (value == "OFF")
    {
      digitalWrite(LED_PIN, LOW);
      dataValue = "OFF";
    }
  }
};

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  BLEDevice::init("ESP32_JEREM");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  BLE2904 *p2904Descriptor = new BLE2904();
  p2904Descriptor->setFormat(BLE2904::FORMAT_UTF8);
  pCharacteristic->addDescriptor(p2904Descriptor);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Waiting for a client connection to notify...");
  Wire.begin(SDA, SCL);
  if(!drv.begin()) {
    Serial.println("Impossible de trouver le DRV2605L");
    while(1);
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);
}

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    pCharacteristic->setValue("OFF");
    pCharacteristic->notify();
  }
  if(dataValue == "ON") {
      drv.setWaveform(0, 1);
      drv.setWaveform(1,0);
      drv.go();
  }

  delay(500);
}