#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_DRV2605.h>
#include "time.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// TODO AJOUTER L'HEURE POUR VIBRER ET AUSSI LORS DE L'APPUIE SUR UN BOUTON ARRETER DE FAIRE VIBRER
#define LED_PIN 1
#define LED_RED_PIN 2
#define LED_GREEN_PIN 3
#define LED_BLUE_PIN 4
uint8_t mac_pilullier[] = {0x30, 0x30, 0xF9, 0x33, 0xFB, 0x40};

Adafruit_DRV2605 drv;
String dataValue = "OFF";

Preferences preferences;
boolean reponseEnvoyee = false;

typedef struct struct_message
{
  char c[32];
  char j[128];
} struct_message;

typedef struct struct_message_reponse
{
  char r[32];
} struct_message_reponse;

typedef struct horaires_table
{
  char *jourDefine;
  int heureDefine;
  boolean estDejaPris;
} struct_horaires;

struct_message incomingMessage;
struct_message_reponse messageReponse;
struct_horaires horaires_tab;

struct tm tm;

esp_now_peer_info_t peerInfo;
String horaires = "";
boolean stopReceived = false;
boolean messageReceived = false;
u_int8_t calculateWeekDay(const char *currentDay)
{
  if (currentDay == "Lundi")
  {
    return 1;
  }
  else if (currentDay == "Mardi")
  {
    return 2;
  }
  else if (currentDay == "Mercredi")
  {
    return 3;
  }
  else if (currentDay == "Jeudi")
  {
    return 4;
  }
  else if (currentDay == "Vendredi")
  {
    return 5;
  }
  else if (currentDay == "Samedi")
  {
    return 6;
  }
  else
  {
    return 0;
  }
}

u_int8_t defineSummerHour(int month, int day, int hour)
{
  u_int8_t isSummerHour;
  if (month == 2 && day == 29 && hour == 2)
  {
    isSummerHour = 1;
  }
  else if (month == 9 && day == 27 && hour == 2)
  {
    isSummerHour = 0;
  }
  return isSummerHour;
}

void setTimeFromPillulier(JsonDocument json)
{
  Serial.printf("Paramètrage de l'ESP-32 à l'heure du pillulier'...");
  const char *currentDay = json["currentDay"];
  for (size_t i = 0; i < json["date"].size(); i++)
  {
    //Serial.printf(json["date"][i]);
    //Serial.println();
  }

  u_int16_t year = json["date"][0];
  u_int8_t month = json["date"][1];
  u_int8_t hour = json["date"][3];
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = json["date"][2];
  tm.tm_wday = calculateWeekDay(currentDay);
  tm.tm_hour = hour + 1;
  tm.tm_min = json["date"][4];
  tm.tm_sec = json["date"][5];
  tm.tm_isdst = defineSummerHour(tm.tm_mon, tm.tm_mday, tm.tm_hour);
  time_t t = mktime(&tm);
  Serial.printf("Paramètrage de l'heure: %s", asctime(&tm));
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);
}

void deserializeDatasFromPilullier()
{
  String datas = preferences.getString("HOR_P_G");
  horaires = datas;
  Serial.print("|--------------------------|");
  Serial.print(datas);
  JsonDocument json;
  deserializeJson(json, datas);
  setTimeFromPillulier(json);
  // JsonObject schedule = json["schedule"];

  // Serial.print("---------------------------");
  // Serial.printf("SCHEDULE %s", schedule);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));

  if (strcmp(incomingMessage.c, "json") == 0)
  {
    preferences.putString("HOR_P_G", incomingMessage.j);
    Serial.printf("Nouveaux Horaires: %s", incomingMessage.j);
    horaires = incomingMessage.j;
    deserializeDatasFromPilullier();
    messageReceived = true;
  }
  else if (strcmp(incomingMessage.c, "STOP") == 0)
  {
    reponseEnvoyee = false;
    stopReceived = true;
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println("Data Send");
}

void getDays()
{
  JsonDocument json;
  deserializeJson(json, horaires);

  switch (tm.tm_wday)
  {
  case 0:
    if (json["sunday"])
    {
      for (size_t i = 0; i < json["sunday"].size(); i++)
      {
        printf("Horaires du dimanche: %s", json["sunday"][i]);
      }
    }
  case 1:
    if (json["monday"])
    {
      for (size_t i = 0; i < json["monday"].size(); i++)
      {
        printf("Horaires du lundi: %s", json["monday"][i]);
      }
    }
    break;
  case 2:
    for (size_t i = 0; i < json["tuesday"].size(); i++)
    {
      Serial.print("ON EST MARDI PENSEZ À PRENDRE VOS MÉDICAMENTS À: ");
      Serial.println();
      Serial.printf(json["tuesday"][i]);
      Serial.println();
      String horaireChar = json["tuesday"][i];
      String heureStr = horaireChar.substring(0,2);
      String minuteStr = horaireChar.substring(3,5);
      uint8_t heure = heureStr.toInt();
      uint8_t minute = minuteStr.toInt();
      Serial.println(heure);
      Serial.println(minute);

      if((tm.tm_hour == heure +1) && tm.tm_min == minute) {
        Serial.print("PRENEZ VOS MEDICAMENTS");
      }
    }
    
    break;
  case 3:
    if (json["Mercredi"])
    {
      for (size_t i = 0; i < json["Mercredi"].size(); i++)
      {
        printf("Horaires du Mercredi: %s", json["Mercredi"][i]);
      }
    }
  case 4:
    if (json["Jeudi"])
    {
      for (size_t i = 0; i < json["Jeudi"].size(); i++)
      {
        printf("Horaires du jeudi: %s", json["Jeudi"][i]);
      }
    }
  case 5:
    if (json["Vendredi"])
    {
      for (size_t i = 0; i < json["Vendredi"].size(); i++)
      {
        printf("Horaires du vendredi: %s", json["Vendredi"][i]);
      }
    }
  case 6:
    if (json["Samedi"])
    {
      for (size_t i = 0; i < json["Samedi"].size(); i++)
      {
        printf("Horaires du samedi: %s", json["Samedi"][i]);
      }
    }
  default:
    break;
  }
}

void setup()
{
  Wire.begin();
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
  // Enregistrer le pair

  preferences.begin("pilullier_IOT", false);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, mac_pilullier, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Ajouter le pair
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Échec de l'ajout du pair");
    return;
  }
  // esp_now_send
  drv.begin();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);
}

void loop()
{
  digitalWrite(LED_PIN, LOW);
  Serial.print(reponseEnvoyee);
  if (messageReceived == true && reponseEnvoyee == false)
  {
    strncpy(messageReponse.r, "OK", 32);
    Serial.printf("Reponse: %s", messageReponse.r);
    esp_err_t result = esp_now_send(mac_pilullier, (u_int8_t *)&messageReponse, sizeof(messageReponse));
    getDays();
    reponseEnvoyee = true;
    messageReceived = false;
    stopReceived = false;
  }

  if (reponseEnvoyee == true && stopReceived == false)
  {
    drv.setWaveform(0, 1);
    drv.setWaveform(1, 0);
    drv.go();
  }
  delay(1000);
}