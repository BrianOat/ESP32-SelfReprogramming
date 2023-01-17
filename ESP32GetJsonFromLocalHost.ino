#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <thread>

const char *SSID = ""; //Replace with the name of your WIFI
const char *PASSWORD = ""; //Replace with Wifi password
const char *HOST = "";  // Replace with the IP address of your local host
const int PORT = 8000; 
const char *URL = "/Updates.json"; //name of json file
int LED_BUILTIN = 2;
void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
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
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  
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

  // Find the OTA data partition
  const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
  if (ota_partition == NULL) {
    Serial.println("Failed to find OTA partition");
    return;
  }

  // Erase the OTA partition
  esp_ota_handle_t ota_handle;
  Serial.println("Erasing OTA partition");
  if (esp_ota_begin(ota_partition,0,&ota_handle) != ESP_OK) {
    Serial.println("Failed to begin OTA update");
    return;
  }
  
  // Write the .bin file to the OTA partition
  size_t bytesWritten = 0;
  const char *buf = http.getString().c_str();
  size_t len = http.getSize();
  Serial.println("Writing .bin file to OTA partition");
  while (bytesWritten < len) {
    size_t toWrite = min(len - bytesWritten, (size_t)1024);
    if (esp_ota_write(ota_handle, buf + bytesWritten, toWrite) != ESP_OK) {
  Serial.println("Failed to write to OTA partition");
  return;
}
    bytesWritten += toWrite;
  }

  // Finish the OTA update and set the new partition as the boot partition
  if (esp_ota_end(ota_handle) != ESP_OK){
  Serial.println("Failed to end OTA update");
  return;
}
  if (esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
    Serial.println("Failed to set boot partition");
    return;
  }

  Serial.println("OTA update complete! Restarting...");
  ESP.restart();
}
