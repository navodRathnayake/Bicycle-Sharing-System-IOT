#include<SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <TinyGPS++.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <Servo.h>



#define DHTTYPE DHT11
Servo servo;


const char* ssid = "SLT-4G_1694C2";
const char* password = "23F72BAB";

SoftwareSerial obj (5,4);

String humidityStr;
String temperatureStr;
String statusStr;
bool lockStatus = true;

String gLatitude = "0.0000";
String gLongatidue = "0.0000";

int status1 = 12;
int status2 = 13;
int status3 = 15;


String serverName = "http://192.168.1.161:8000";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

DHT dht (5,DHTTYPE);

int gBicycleStatusID;
int gUserID;
int gPathID;
int gRecentActivityID;
int gHumidity;
float gTemperature;

StaticJsonDocument<200> bicycleLiveLocation;
StaticJsonDocument<300> weatherDataDoc;
StaticJsonDocument<200> GPSDataDoc;



void setup() {
  Serial.begin(115200);
  obj.begin(9200);
  dht.begin();

  servo.attach(14);
  servo.write(0);

  pinMode(status1,OUTPUT);
  pinMode(status2,OUTPUT);
  pinMode(status3,OUTPUT);


  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {

 
  getSerialCommunicationData();

  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;

      Serial.print("Humidity Data from String : ");
      Serial.print(humidityStr);
      Serial.print("%");

      checkBicycleAvailability("/api/bicycles/1", client);

      if(gBicycleStatusID != 1){
        int serviceResponse = getService("/api/service/1", client);
        if(serviceResponse == 200){

          Serial.println("---------------------------------------------------");

          Serial.print("User ID : ");
          Serial.println(gUserID);

          Serial.print("Path ID : ");
          Serial.println(gUserID);

          Serial.print("Recent Activity ID : ");
          Serial.println(gRecentActivityID);

          Serial.println("---------------------------------------------------");

          getSerialCommunicationData();
          postWeatherData(client);
          postGPSData(client);
          updateBicycleLocation(client);

          Serial.print("Now : ");
          Serial.print(gBicycleStatusID);

          while(1){
            Serial.println("While Loop Here");
            checkBicycleAvailability("/api/bicycles/1", client);
            if(gBicycleStatusID == 1){

              digitalWrite(status1, HIGH);
              digitalWrite(status2, LOW);
              digitalWrite(status3,LOW);

              Serial.println("Available State - End Service");
              lockTheBicycle();
              postWeatherData(client);
              postGPSData(client);
              updateBicycleLocation(client);
              return;
            }
            if(gBicycleStatusID == 2){

              digitalWrite(status1, LOW);
              digitalWrite(status2, HIGH);
              digitalWrite(status3,LOW); 

              unlockTheBicycle();

              Serial.println("Riding State");
              postGPSData(client);
              updateBicycleLocation(client);
              while(1){
                checkBicycleAvailability("/api/bicycles/1", client);
                getSerialCommunicationData();

                postGPSData(client);
                updateBicycleLocation(client);
                lockStatus ? unlockTheBicycle() : lockTheBicycle();
                Serial.println("");
                Serial.println("--------------------------");
                Serial.println("Bicycle Status : On Service");
                Serial.println("--------------------------");
                Serial.println("");

                
                if(gBicycleStatusID == 1 || gBicycleStatusID == 3){
                  lockTheBicycle();
                  postGPSData(client);
                  return;
                }
                
              }
            }
            if(gBicycleStatusID == 3){

              digitalWrite(status1, LOW);
              digitalWrite(status2, LOW);
              digitalWrite(status3,HIGH);

              lockTheBicycle();
              updateBicycleLocation(client);
              while(1){
                checkBicycleAvailability("/api/bicycles/1", client);
                Serial.println("Parking State");
                updateBicycleLocation(client);

                  Serial.println("");
                  Serial.println("--------------------------");
                  Serial.println("Bicycle Status : Parking");
                  Serial.println("--------------------------");
                  Serial.println("");

                
                if(gBicycleStatusID == 1 || gBicycleStatusID == 2){
                  if(gBicycleStatusID == 1){
                    lockTheBicycle();
                    return;
                  }
                  unlockTheBicycle();
                  return;
                }
               
              }
            }
            
          }
        }
        else{
          return;
        }
      }else{
        updateBicycleLocation(client);
        Serial.println("");
        Serial.println("--------------------------");
        Serial.println("Bicycle Status : Available");
        Serial.println("--------------------------");
        Serial.println("");

        digitalWrite(status1, HIGH);
        digitalWrite(status2, LOW);
        digitalWrite(status3,LOW);
        
      }
      

      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void checkBicycleAvailability(String serverURL, WiFiClient client){
  HTTPClient http;
  String serverPath = serverName + serverURL;
  http.begin(client, serverPath.c_str());

  int httpResponseCode = http.GET();

  if (httpResponseCode>0) {
        // Serial.print("HTTP Response code: ");
        // Serial.println(httpResponseCode);
        String bicyclePayload = http.getString();
        // Serial.println(payload);

        JSONVar bicycleResponse = JSON.parse(bicyclePayload);

        if (JSON.typeof(bicycleResponse) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      // Serial.print("JSON object = ");
      // Serial.println(bicycleResponse);

      Serial.print("Bicycle Status ID :");
      Serial.println(bicycleResponse["Bicycle"]["statusId"]["id"]);

      gBicycleStatusID = bicycleResponse["Bicycle"]["statusId"]["id"];
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    return;
  }
  http.end();
}


int getService(String serverURL, WiFiClient client){
  HTTPClient http;
  String serverPath = serverName + serverURL;
  http.begin(client, serverPath.c_str());

  int httpResponseCode = http.GET();

  if (httpResponseCode>0) {
        String servicePlayload = http.getString();

        JSONVar serviceResponse = JSON.parse(servicePlayload);

        if (JSON.typeof(serviceResponse) == "undefined") {
        Serial.println("Parsing input failed!");
        return 500;
      }

       gUserID = serviceResponse["serviceDetails"][0]["user_id"];
       gPathID = serviceResponse["serviceDetails"][0]["path_id"];
       gRecentActivityID = serviceResponse["serviceDetails"][0]["recent_activity_id"];

      Serial.println("------------------------");
      Serial.println("The Service API Has Read");
      Serial.println("------------------------");


      return 200;
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    return 500;
  }
  http.end();
}

int postAPIData(String serverURL, WiFiClient client, String json){
      HTTPClient http;
      String serverPath = serverName + serverURL;
      http.begin(client, serverPath);
  
      http.addHeader("Content-Type", "application/vnd.api+json");
      http.addHeader("Accept", "application/vnd.api+json");   

      int httpResponseCode = http.POST(json);
      
      
     
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpResponseCode);

      if(httpResponseCode == 200){
        Serial.println("API data has Posted to "+ serverURL);

        http.end();
        return 200;
      }
      else{
        http.end();
        return 500;
      }
      
      
}

int patchAPIData(String serverURL, WiFiClient client, String json){
      HTTPClient http;
      String serverPath = serverName + serverURL;
      http.begin(client, serverPath);
  
      http.addHeader("Content-Type", "application/vnd.api+json");
      http.addHeader("Accept", "application/vnd.api+json");   

      int httpResponseCode = http.PATCH(json);
      
      
     
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpResponseCode);

      if(httpResponseCode == 200){
        Serial.println("API data has Posted to "+ serverURL);

        http.end();
        return 200;
      }
      else{
        http.end();
        return 500;
      }
      
      
}

void gpsTrackerLatLong(){
  // Get the latitude and longatide
}

void postWeatherData(WiFiClient client){

  weatherDataDoc["recentActivityId"] = gRecentActivityID;
  weatherDataDoc["humidity"] = gHumidity;
  weatherDataDoc["temperature"] = temperatureStr;
  weatherDataDoc["visibility"] = 1;
  weatherDataDoc["weatherStatus"] = "N/A";
  weatherDataDoc["windSpeed"] = "120.567";

  String myJson;
  serializeJson(weatherDataDoc, myJson);
  int postWeatherResponse = postAPIData("/api/weather", client, myJson);
  // Serial.print(myJson);
  postWeatherResponse == 200 ? printProcessStatus("Weather Data has updated") : printProcessStatus("Weather data cannot updated");
}

void postGPSData(WiFiClient client){

  getCurrentLocation();

  GPSDataDoc["pathId"] = String(gPathID);
  GPSDataDoc["bicycleId"] = "1";
  GPSDataDoc["gpsPointsLang"] = String(gLatitude);
  GPSDataDoc["gpsPointsLong"] = String(gLongatidue);

  String myJson;
  serializeJson(GPSDataDoc, myJson);
  // Serial.print(myJson);
  int postGPSResponse = postAPIData("/api/gps", client, myJson);

  postGPSResponse == 200 ? printProcessStatus("GPS Data has updated") : printProcessStatus("GPS data cannot updated");
}

void lockTheBicycle(){
  servo.write(0);
  Serial.println("Locked the bicycle");
}

void unlockTheBicycle(){
  servo.write(180);
  Serial.println("Unlocked the bicycle");
}

void getSerialCommunicationData() {
  String response = obj.readStringUntil('\r');

  if (response.length() == 0) {
    // No data received, return or handle this situation
    return;
  }

  int firstComma = response.indexOf(',');
  int secondComma = response.indexOf(',', firstComma + 1);

  if (firstComma != -1 && secondComma != -1) {
    humidityStr = response.substring(0, firstComma);
    temperatureStr = response.substring(firstComma + 1, secondComma);
    statusStr = response.substring(secondComma + 1);
    // statusStr = response.charAt(response.length() - 1);

    // Convert the strings to other data types
    int humidity = humidityStr.toInt();
    float temperature = temperatureStr.toFloat();

    gHumidity = humidityStr.toInt();
    gTemperature = temperatureStr.toFloat();
    lockStatus = (statusStr.toInt() == 1);
      

    Serial.println(statusStr.length());

    if(statusStr.length() > 1){
      statusStr = response.substring(secondComma + 1);
      Serial.print("Line is Gt than 1 and last string is : ");
      statusStr = response.charAt(response.length() - 1);
      Serial.println(statusStr);
      lockStatus = (statusStr.toInt() == 1);
    }

    

    // Print the values
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Status: ");
    Serial.println(lockStatus);

  } else {
    // Handle the case when the data format is invalid
    Serial.println("Invalid data format: " + response);
    // You can add more error handling here, like resetting the serial input buffer.
  }
}

int getCurrentLocation(){

  
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    
    client->setInsecure();
    
    HTTPClient https;

     String geolocationData = "{\"homeMobileCountryCode\":310, "
                      "\"homeMobileNetworkCode\":410, "
                      "\"radioType\":\"gsm\", "
                      "\"carrier\":\"Vodafone\", "
                      "\"considerIp\":true}";
    
    if (https.begin(*client, "https://www.googleapis.com/geolocation/v1/geolocate?key=YOUR_GEOLOCATION_API_KEY")) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.POST(geolocationData);
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String geoLocationPayload = https.getString();
          Serial.println(geoLocationPayload);

          JSONVar geoLocationResponse = JSON.parse(geoLocationPayload);

          if (JSON.typeof(geoLocationResponse) == "undefined") {
          Serial.println("Parsing input failed!");
          return 500;
        }

        // gLatitude = geoLocationResponse["location"]["lat"];
        gLongatidue = String(geoLocationResponse["location"]["lng"]);

        Serial.println(type_name(geoLocationResponse["location"]["lat"]));
        gLatitude = JSON.stringify(geoLocationResponse["location"]["lat"]);
        gLongatidue = JSON.stringify(geoLocationResponse["location"]["lng"]);

        

        Serial.print("Latitude :");
        Serial.println(gLatitude);

        Serial.print("Longatidue :");
        Serial.println(gLongatidue);
          
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        return 500;
      }

      https.end();
      return 200;

    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
      return 500;
    }
}

void updateBicycleLocation(WiFiClient client){

  getCurrentLocation();

  bicycleLiveLocation["liveLang"] = String(gLatitude);
  bicycleLiveLocation["liveLong"] = String(gLongatidue);

  String myJson;
  serializeJson(bicycleLiveLocation, myJson);
  int updateBycycleResponse = patchAPIData("/api/bicycle/update/1",client, myJson);
  // Serial.print(myJson);
  updateBycycleResponse == 200 ? printProcessStatus("Bicycle Location Has Updated!") : printProcessStatus("Bicycle Location Cannot Update!");
}

void printProcessStatus(String status){
  Serial.println("--------------------------");
  Serial.println("status");
  Serial.println("--------------------------");
}

template <class T>
String type_name(const T&)
{   
    String s = __PRETTY_FUNCTION__;

    int start = s.indexOf("[with T = ") + 10;
    int stop = s.lastIndexOf(']');

    return s.substring(start, stop);
}











