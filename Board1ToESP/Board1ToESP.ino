#include <SoftwareSerial.h>
SoftwareSerial espSerial(2, 3); // ESP RX, TX

#include <AltSoftSerial.h>
AltSoftSerial stmSerial; // STM32 RX, TX

const char* ssid = "Toqa";
const char* pass = "toqa2004";
const char* host = "io.adafruit.com";
const char* key = "PUT_YOUR_ADAFRUIT_KEY_HERE";
const char* user = "toqahossamx";

bool isConnected = false;
bool connectedd = false;

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  espSerial.begin(9600);
  stmSerial.begin(9600);

  Serial.println("Initializing...");
  // Clear buffers
  while(espSerial.available()) espSerial.read();
  
  sendCommand("AT+RST", 2000);
  sendCommand("AT+CWMODE=1", 1000);
  
  String wifiCmd = "AT+CWJAP=\"" + String(ssid) + "\",\"" + String(pass) + "\"";
  sendCommand(wifiCmd, 8000);
  
  connectToServer();
}

void loop() {
  if (!isConnected) {
    digitalWrite(13, 0);
    connectToServer();
  } else {
    digitalWrite(13, 1);
  }

  if (stmSerial.available() > 0) {
    String incomingData = stmSerial.readStringUntil('\n');
    incomingData.trim();
    
    if (incomingData.length() > 0) {
      Serial.println("Received: " + incomingData);
      int commaIndex = incomingData.indexOf(',');
      if (commaIndex != -1) {
        String type = incomingData.substring(0, commaIndex); 
        String val = incomingData.substring(commaIndex + 1); 
        
        if (type == "R") {
          connectedd = sendAdafruitPost("rfid", val);
          if (!connectedd){
            while(1){
            connectToServer();
            if(isConnected){
              sendAdafruitPost("rfid", val);
              break;
              }
              delay(2000);
            }  
          }
        } else {
          connectedd = sendAdafruitPost("sensor", incomingData); // Send only the value
            if (!connectedd){
            while(1){
            connectToServer();
            if(isConnected){
              sendAdafruitPost("sensor", incomingData);
              break;
              }
              delay(2000);
            }  
          }
        }
      }
    }
  }
}

void connectToServer() {
  Serial.println("Attempting to connect...");
  // Flush buffer to ensure we aren't reading old "ERROR" messages
  while(espSerial.available()) espSerial.read();
  
  espSerial.println("AT+CIPSTART=\"TCP\",\"" + String(host) + "\",80");
  
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (espSerial.find("OK") || espSerial.find("ALREADY")) {
      isConnected = true;
      Serial.println("Connected!");
      return;
    }
  }
  isConnected = false;
  Serial.println("Failed to connect!");
}




bool sendAdafruitPost(String feedName, String val) {
  while(espSerial.available()) espSerial.read();

  String postData = "value=" + val;
  String httpRequest = "POST /api/v2/" + String(user) + "/feeds/" + feedName + "/data HTTP/1.1\r\n" +
                       "Host: " + String(host) + "\r\n" +
                       "X-AIO-Key: " + String(key) + "\r\n" +
                       "Content-Type: application/x-www-form-urlencoded\r\n" +
                       "Content-Length: " + String(postData.length()) + "\r\n" +
                       "Connection: keep-alive\r\n\r\n" +
                       postData;

  espSerial.print("AT+CIPSEND=");
  espSerial.println(httpRequest.length());

  unsigned long start = millis();
  bool promptReceived = false;
  while (millis() - start < 2000) {
    if (espSerial.find(">")) {
      promptReceived = true;
      break;
    }
  }

  if (promptReceived) {
    espSerial.print(httpRequest);
    if (espSerial.find("SEND OK")) {
      isConnected = true;
      return true;
    } else {
      isConnected = false;
      return false;
    }
  } else {
    isConnected = false;
    return false;
  }
}

void sendCommand(String cmd, int waitTime) {
  espSerial.println(cmd);
  unsigned long start = millis();
  while (millis() - start < waitTime) {
    if (espSerial.available()) {
      Serial.write(espSerial.read());
    }
  }
}
