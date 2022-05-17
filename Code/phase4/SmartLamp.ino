#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "LittleFS.h"

WiFiClient wifiClient;

const char* ssid = "Shariati";  // WiFi Name
const char* password = "0050546856";  // WiFi Password
const int lamp1 = 0;
const int lamp2 = 13;
const char* schedule_path = "/schedule.txt";  // path to the schedule file saved in NodeMCU hard drive

const int lamp_status_request_period = 1000000;  // in milliseconds
const int schedule_request_period = 3000000;
const int schedule_action_period = 1000000;
const int switch_check_period = 1000;

void setup () {

  Serial.begin(115200);  // start the serial monitor
  delay(10); // give it a moment

  initFS();

  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  digitalWrite(lamp1, LOW);  // start off
  digitalWrite(lamp2, LOW);  // start off

  // connect to WiFi network:
  Serial.println("Welcome to SmartLamp Version 1.0!");
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);

  // show ... until connected:
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());  // print the IP address of the device

  writeFile(LittleFS, schedule_path, "");  // clear the previously acquired schedule file, as its timing is no longer valid. also, ignore the 'write failed' message. it does erase the schedule file
}

bool millis_elapsed(int milliseconds, int &last){
  int now = millis();
  if (now - last > milliseconds){
    last = now;
    return true;
  }
  return false;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}


// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void loop() {

  // verfiy WiFi is connected:
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected :/");
  } else {

    /*** Read the lamps status data from server ***/
    static int lamp_status_last = millis(); 
    if (millis_elapsed(lamp_status_request_period, lamp_status_last)) {
      HTTPClient http;  // object of the class HTTPClient.
      http.begin(wifiClient, "http://my-smartlamp.fandogh.cloud/control/getupdate/");  // request destination.
      int httpCode = http.GET(); // send the request.
      if (httpCode > 0) {  // check the returning code
//        Serial.println("Lamp Status Data:");
        String payload = http.getString();  // get the text from the destination.
//        Serial.println(payload);  // print the text.
        digitalWrite(lamp1, payload[0] - '0');  // send the payload value to the pin.
        digitalWrite(lamp2, payload[1] - '0');  // send the payload value to the pin.
      } else {
        Serial.println("HTTP Connection Failed!");
      }
      http.end();  // close connection
    }
  
    /*** Read the scheduling data from server ***/
    static int schedule_request_last = millis(); 
    if (millis_elapsed(schedule_request_period, schedule_request_last)) {
      HTTPClient http;  // object of the class HTTPClient.
      http.begin(wifiClient, "http://my-smartlamp.fandogh.cloud/control/getschedule/");  // request destination.
      int httpCode = http.GET(); // send the request.
      if (httpCode > 0) {  // check the returning code
        Serial.println("Schedule Data:");
        String payload = http.getString();  // get the text from the destination
        payload = String() + millis() + " " + payload;
        Serial.println(payload);  // print the text
        writeFile(LittleFS, schedule_path, payload.c_str());  // save in a file
      } else {
        Serial.println("HTTP Connection Failed!");
      }
      http.end();  // close connection
    }
    
  }

  /*** monitor the change in keys ***/
  static int switch_check_last = millis();
  static int switch_value = 0;  // todo: change to the physical input thingy
  if (millis_elapsed(switch_check_period, switch_check_last)) {
    switch_value = 1 - switch_value;
    if (true) {  // todo: if switch value changed
      // check WiFi connection status
      if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        http.begin(wifiClient, "http://my-smartlamp.fandogh.cloud/control/report/");  // post destination
        http.addHeader("Content-Type", "text/plain");
        String lamps_status = String(digitalRead(lamp1)) + String(digitalRead(lamp2));
        int httpResponseCode = http.POST(lamps_status);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        http.end();
      } else {
        Serial.println("WiFi Disconnected");
      }
    }
  }
  
  /*** read the schedule from hard drive and put it in action ***/
  static int schedule_action_last = millis();
  if (millis_elapsed(schedule_action_period, schedule_action_last)) {
    String schedule = readFile(LittleFS, schedule_path);
    if (schedule.length() == 0) {
      Serial.println("No schedule saved on device");
    } else {
      Serial.println(schedule);
      int space_idx = schedule.indexOf(' ');
      int schedule_save_time = schedule.substring(0, space_idx).toInt();
      Serial.println(schedule_save_time);
      schedule = schedule.substring(space_idx + 1, schedule.length());
      int now = millis();
      while (schedule.length() > 0) {
        int space_idx = schedule.indexOf(' ');
        String lamp_num_and_status = schedule.substring(0, space_idx);
        schedule = schedule.substring(space_idx + 1, schedule.length());
        space_idx = schedule.indexOf(' ');
        int milliseconds = schedule.substring(0, space_idx).toInt();
        schedule = schedule.substring(space_idx + 1, schedule.length());
//          Serial.println(lamp_num_and_status);
//          Serial.println(milliseconds);
        if (schedule_save_time + milliseconds <= now and schedule_save_time + milliseconds + schedule_action_period > now) {
          // then it is time to make the change!
          int lamp_num = lamp_num_and_status[0] == '1' ? lamp1 : lamp2;
          int lamp_status = lamp_num_and_status.substring(1, 2).toInt();
          digitalWrite(lamp_num, lamp_status);
          Serial.print("Schedule change:");
          Serial.println(lamp_num_and_status);
        }
      }
    }
  }

}
