#include "Network.h"
#include "time.h"
#include <bits/stdc++.h>
#include <string>
#include "addons/TokenHelper.h"
#define WIFI_SSID "TP-Link_77CE"
#define WIFI_PASSWORD "29457915"

#define API_KEY "AIzaSyAsIlkYQFu9rExzhdpG5HHfnhEI66Unj-4"
#define FIREBASE_PROJECT_ID "myx-baggage"
#define USER_EMAIL "reader1@myxbaggage.com"
#define USER_PASSWORD "Xu@n3309"

using namespace std;

static Network *instance = NULL;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;

Network::Network(){
  instance = this;
}

void WiFiEventConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("WIFI CONNECTED! WAITING FOR LOCAL IP ADDR");
}

void WiFiEventGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("LOCAL IP ADDRESS: ");
    Serial.println(WiFi.localIP());
    instance->firebaseInit();
}

void WiFiEventDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("WIFI DISCONNECTED!");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void FirestoreTokenStatusCallback(TokenInfo info){
  Serial.printf("Token Info: type = %s, status = %s\n", getTokenType(info).c_str(), getTokenStatus(info).c_str());
}

void convertValue(char* a, int size, int& value)
{
  int i;
  string s = "";
  for (i = 0; i < size; i++)
  {
    s = s + a[i];
  }
  value = atoi(s.c_str());
}

void Network::initWiFi(){
    WiFi.disconnect();
    WiFi.onEvent(WiFiEventConnected, SYSTEM_EVENT_STA_CONNECTED);
    WiFi.onEvent(WiFiEventGotIP, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent(WiFiEventDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void Network::firebaseInit(){
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = FirestoreTokenStatusCallback;

  Firebase.begin(&config, &auth);
}

void Network::firestoreDataUpdate(String tagID){
  struct tm timeinfo;

  if(WiFi.status() == WL_CONNECTED && Firebase.ready() && getLocalTime(&timeinfo)){
    String documentPath = "tagCheckIn/" + tagID;

    FirebaseJson content;

    int hour, min;

    char timeHour[3];
    strftime(timeHour,3, "%H", &timeinfo);
    char timeMin[3];
    strftime(timeMin,3, "%M", &timeinfo);

    convertValue(timeHour, sizeof(timeHour), hour);
    convertValue(timeMin, sizeof(timeMin), min);

    String strHour;
    String strMin;
    
    if (hour<10)
    {
      strHour += "0";
      strHour += String(hour);
    }
    else
    {
      strHour += String(hour);
    }

    if (min<10)
    {
      strMin += "0";
      strMin += String(min);
    }
    else
    {
      strMin += String(min);
    }

    cout << endl;

    String detectTime;
    detectTime += strHour;
    detectTime += ":";
    detectTime += strMin;
    String airportID = "AOR";

    content.set("fields/detectTime/stringValue", detectTime);
    content.set("fields/airportID/stringValue", airportID);
    content.set("fields/usage/integerValue", 0);

    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "detectTime,airportID,usage")){
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }

    if(Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())){
      Serial.printf("ok\n\%s\n\n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }
  }
}