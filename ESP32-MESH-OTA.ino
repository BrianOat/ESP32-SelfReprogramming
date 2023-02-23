#include <BluetoothSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_ota_ops.h>
#include <LittleFS.h>
#include <ArduinoJSON.h>

// Define OTA_FLASH_BLOCK_SIZE if it is not already defined
#ifndef OTA_FLASH_BLOCK_SIZE
#define OTA_FLASH_BLOCK_SIZE   0x1000  // 4KB
#endif

const char *SSID = ""; //Replace with the name of your WIFI
const char *PASSWORD = ""; //Replace with Wifi password
const char *HOST = "";  // Replace with the IP address of your local host
const int PORT = 8000; 
const char *URL = "/Updates.json"; //name of json file
HTTPClient http;

String firm = "";
void setup(){
 
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 Serial.println();
 Serial.println("Connecting to HOST");
 http.begin(HOST, PORT, URL);
  
}
bool writeFirmware(String firmwareData){
  File firmware = LittleFS.open("/firmware.bin", "w");
  if (!firmware) {
    Serial.println("Failed to open firmware file for writing");
    return false;
  }
  
  // firmwareData is an array of uint8_t containing the binary data of the firmware
  size_t firmSize = sizeof(firmwareData);
  //writes firmware and validates write, but need to covert cstring to unit8_t to write it
  size_t bytesWritten = firmware.print(firmwareData.c_str());
  if (bytesWritten != firmSize) {
    Serial.println("Failed to write firmware to file");
    firmware.close();
    return false;
  }
  return true;
  
}
void updateFirmware(bool fileSaved) {
  if(fileSaved){
    if (!LittleFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return;
    }
      
    File firmware = LittleFS.open("/firmware.bin", "r");
    if (!firmware) {
      Serial.println("Failed to open firmware file");
      return;
    }
  
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
      Serial.println("Failed to get update partition");
      return;
    }
    
    if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle) != ESP_OK) {
      Serial.println("Failed to start firmware update");
      return;
    }
  
    size_t update_size = 0;
    uint8_t data[OTA_FLASH_BLOCK_SIZE];
    while (firmware.available()) {
      size_t len = firmware.read(data, OTA_FLASH_BLOCK_SIZE);
      if (esp_ota_write(update_handle, data, len) != ESP_OK) {
        Serial.println("Firmware update failed");
        return;
      }
      update_size += len;
    }
  
    if (esp_ota_end(update_handle) != ESP_OK) {
      Serial.println("Firmware update failed");
      return;
    }
  
    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
      Serial.println("Failed to set boot partition");
      return;
    }
  
    Serial.println("Firmware update succeeded");
    
    firmware.close();
    LittleFS.end();
  }
  else{
    Serial.println("could not save firmware file");
}
}

void loop(){
  if(http.GET() != HTTP_CODE_OK)
  {
    Serial.println("Failed to Ping server");
    http.end();
    return;
  }
  else{
    String httpString = http.getString();
    //Serial.println(httpString);
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
  if (currentVersion == newestVersion) {
    Serial.println("No updates available");
    http.end();
    return;
  }
  http.begin(HOST, PORT, doc["binFile"] );
  if(http.GET() == HTTP_CODE_OK)
  {
  int numberOfPackets = toInt(http.getString());
  http.begin(HOST, PORT, "/packets" );
  for(int i = 0;i<numberOfPackets;i++){
       if(http.GET() == HTTP_CODE_OK){
          writeFirmware(http.getString());
       }
    }
  }
  //updates the firmware?
    if(firm !=""){
      Serial.println("UpdateFirmware Called");
      updateFirmware(true);
    }
  }
  delay(1000);
}
