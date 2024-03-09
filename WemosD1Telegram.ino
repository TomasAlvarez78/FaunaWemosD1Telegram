#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "config.h"

#define sensorPin A0

const unsigned long timeBetweenMessageRead = 1000;
unsigned long timeBetweenMessageSent = 60000;
const unsigned long timeBetweenSensor = 1000;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

unsigned long lastTimeRead;
unsigned long lastTimeSent;
unsigned long lastTimeSensor;

const int ledPin = LED_BUILTIN;
int ledStatus = 0;

#define moistureMin 370
#define moistureMax 1000

int moistureMiddleValue = 600;

void sendMessage(String message) {
  String chatId = bot.messages[0].chat_id;
    if (millis() - lastTimeSent > timeBetweenMessageSent) {
      Serial.println("Sending message");
      bot.sendMessage(chatId, message, "");
      lastTimeSent = millis();
    }
}

// If the sensor value is 1000 or more than that then the sensor is not in the soil or sensor is disconnected.
// If the sensor value is more than 600 but less than 1000 then the soil is dry.
// If the sensor value is 370 to 600 then the soil is humid.
// If the sensor value is less than 370 then the sensor in the water.

int checkMoisture(){
  int moistureValue = analogRead(sensorPin);
  Serial.println("Moisture value: " + String(moistureValue));

  // Check for possible errors in sensor
  if (moistureValue < moistureMin){
    sendMessage("Sensor in water?\nHello I'm under the water please help me");
    return 0 ;
  }else if (moistureValue > moistureMax){
    sendMessage("Sensor not in soil or disconnected");
    return 0 ;
  }

  // Normal moisture logic
  if (moistureValue > moistureMiddleValue){
      sendMessage("Water your plant...");
  }

  return moistureValue;
}

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);
  for (int i = 0; i < numNewMessages; i++) {
    String chatId = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String fromName = bot.messages[i].from_name;
    if (fromName == "")
      fromName = "Guest";

    if (text == "/ledon") {
      digitalWrite(ledPin, LOW);
      ledStatus = 1;
      bot.sendMessage(chatId, "Led is ON", "");
      return;
    }

    if (text == "/ledoff") {
      ledStatus = 0;
      digitalWrite(ledPin, HIGH);
      bot.sendMessage(chatId, "Led is OFF", "");
      return;
    } 

    if (text.substring(0,12) == "/setMoisture") {
      // Serial.print("String lenght: " + String(text.length()));
      String tempMoisture = text.substring(12,text.length());
      tempMoisture.trim();
      Serial.println(tempMoisture);
      moistureMiddleValue = tempMoisture.toInt(); 
      bot.sendMessage(chatId, "Moisture set to " + tempMoisture, "");
      return;
    }

    if (text.substring(0,10) == "/setTimer") {
      // Serial.print("String lenght: " + String(text.length()));
      String tempTimer = text.substring(10,text.length());
      tempTimer.trim();
      Serial.println(tempTimer);
      timeBetweenMessageSent = tempTimer.toInt();
      bot.sendMessage(chatId, "Timer set to " + tempTimer, "");
      return;
    }

    if (text == "/status") {
      if (ledStatus) {
        bot.sendMessage(chatId, "Led is ON and soil is " + String(checkMoisture()), "");
      } else {
        bot.sendMessage(chatId, "Led is OFF and soil is " + String(checkMoisture()), "");
      }
      return;
    }


    if (text == "/start") {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + fromName + ".\n";
      welcome += "This is Flash Led Bot example.\n\n";
      welcome += "/ledon : to switch the Led ON\n";
      welcome += "/ledoff : to switch the Led OFF\n";
      welcome += "/setMoisture {value} : Set the value for moisture alarm\n";
      welcome += "/setTimer {value} : Set the timer for moisture alarm ( ms )\n";
      welcome += "/status : Returns current status of LED and soil \n";
      bot.sendMessage(chatId, welcome, "Markdown");
      return;
    }


  }
}


void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(ledPin, OUTPUT);
  delay(10);
  digitalWrite(ledPin, HIGH);

  configTime(0, 0, "pool.ntp.org");
  secureClient.setTrustAnchors(&cert);
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  if (millis() - lastTimeSensor > timeBetweenSensor) {
    checkMoisture();
    lastTimeSensor = millis();
  }
  if (millis() - lastTimeRead > timeBetweenMessageRead) {
    Serial.println("Checking for messages...");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Got a new message!");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeRead = millis();
  }
}
