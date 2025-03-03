#include <esp_now.h>
#include <WiFi.h>
/// AJOUTER L'HEURE POUR VIBRER ET AUSSI LORS DE L'APPUIE SUR UN BOUTON ARRETER DE FAIRE VIBRER
#define LED_PIN 1
#define LED_RED_PIN 2
#define LED_GREEN_PIN 3
#define LED_BLUE_PIN 4
#include "time.h"
String dataValue = "OFF";
typedef struct struct_message
{
  char a[32];
} struct_message;

struct_message incomingMessage;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message: ");
  Serial.println(incomingMessage.a);

  if (strcmp(incomingMessage.a, "LED_ON") == 0)
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else if (strcmp(incomingMessage.a, "LED_OFF") == 0)
  {
    digitalWrite(LED_PIN, LOW);
  }
  else if (strcmp(incomingMessage.a, "LED_FRANCE") == 0)
  {
    int red = random(0, 256);
    int green = random(0, 256);
    int blue = random(0, 256);

    analogWrite(LED_RED_PIN, red);
    analogWrite(LED_GREEN_PIN, green);
    analogWrite(LED_BLUE_PIN, blue);
  }
}

void setTimeFromPillulier(int yr, int month, int mday, int hr, int minute, int sec, int isDst) {
  Serial.printf("Paramètrage de l'ESP-32 à l'heure du pillulier'...");
  struct tm tm;
  tm.tm_year = yr - 1900;
  tm.tm_mon = month -1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;
  time_t t = mktime(&tm);
  Serial.printf("Paramètrage de l'heure: %s", asctime(&tm));
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);
}

void loop()
{
  digitalWrite(LED_PIN, LOW);
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