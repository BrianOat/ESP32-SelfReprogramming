#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Update.h>

const char *SSID = ""; //Replace with the name of your WIFI
const char *PASSWORD = ""; //Replace with Wifi password
const char *HOST = "";  // Replace with the IP address of your local host
const int PORT = 8000; 
const char *URL = "/Updates.json"; //name of json file


void performUpdate(Stream &updateSource, size_t updateSize) {
   if (Update.begin(updateSize)) {      
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
         Serial.println("Written : " + String(written) + " successfully");
      }
      else {
         Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      }
      if (Update.end()) {
         Serial.println("OTA done!");
         if (Update.isFinished()) {
            Serial.println("Update successfully completed. Rebooting.");
         }
         else {
            Serial.println("Update not finished? Something went wrong!");
         }
      }
      else {
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }
   else
   {
      Serial.println("Not enough space to begin OTA");
   }
}

void setup() {
  pinMode(2,OUTPUT);
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
}

void loop() {
  // Check for updates every 10 seconds
  delay(10000);

  // Send a GET request to the server to retrieve the Updates.json file
  HTTPClient http;
  http.begin(HOST, PORT, URL);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Failed to retrieve updates: %d\n", httpCode);
    http.end();
    return;
  }
  String httpStr = http.getString();
  Serial.println("http: " + httpStr);
  // Parse the response as a JSON object
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, httpStr);
  if (error) {
    Serial.println("Failed to parse updates");
    http.end();
    return;
  }
  
  String jsonObj;
  JsonVariant jsonVariant = doc.as<JsonVariant>();
  serializeJson(jsonVariant, jsonObj);
  Serial.println(jsonObj);
  
  // Check if the current version is equal to the newest version
  double currentVersion = doc["currentVersion"];
  double newestVersion = doc["newestVersion"];
  
  Serial.println(currentVersion);
  Serial.println(newestVersion);
  
  if (currentVersion == newestVersion) {
    Serial.println("No updates available");
    http.end();
    return;
  }
  // Download the .bin file from the server
  http.begin(HOST, PORT, doc["binFile"]);
  httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Failed to download .bin file: %d\n", httpCode);
    http.end();
    return;
  }
  Serial.print("Got BIN!");
  Serial.println(httpCode);
  String binFile = http.getString();
  //Serial.println(binFile.c_str());
  File firmwareFile = SPIFFS.open("/firmware.bin");
  firmwareFile.println(binFile);//Change this
  firmwareFile.close();

  File file = SPIFFS.open("/firmware.bin");
  size_t updateSize = file.size();
  Serial.println(updateSize);
  if(updateSize>0){
    performUpdate(file,file.size());
  }
}
  

  
