#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "LittleFS.h"

/*** pin information ***/
const int lamp1 = 12;  // output
const int lamp2 = 13;  // output
const int key1 = 0;  // input
const int key2 = 2;  // input

/*** period tasks and their respective periods (in milliseconds) ***/
const int lamp_status_request_period = 1000;
const int schedule_request_period = 30000;
const int schedule_action_period = 15000;
const int switch_check_period = 100;
const int disconnection_period = 10000;
int last_disconnected = -100000;  // last time the device was unable to connect to the server

/*** server request addresses ***/
const char* lamp_status_request_address = "http://my-smartlamp.fandogh.cloud/control/getupdate/";
const char* schedule_request_address = "http://my-smartlamp.fandogh.cloud/control/getschedule/";
const char* post_lamp_status_address = "http://my-smartlamp.fandogh.cloud/control/report/";

/*** WiFi config ***/
const char* ssid = "arian";
const char* password = "002002002";

const char* schedule_path = "/schedule.txt";  // path to the schedule file saved on NodeMCU hard drive

WiFiClient wifiClient;

void setup () {

  Serial.begin(115200);  // start the serial monitor
  delay(10); // give it a moment

  initFS();  // initialize the LittleFS file system

  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  digitalWrite(lamp1, HIGH);  // start on
  digitalWrite(lamp2, HIGH);  // start on

  pinMode(key1, INPUT);  // default value: HIGH
  pinMode(key2, INPUT);  // default value: HIGH

  // connect to WiFi network
  Serial.println("Welcome to SmartLamp-1.0!");
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);

  // show ... until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());  // print the IP address of the device

  // clear the previously acquired schedule file, as its timing is no longer valid
  writeFile(LittleFS, schedule_path, "");  // ignore the 'write failed' message, it does erase the schedule file
}

/*** execute four periodic tasks ***/
void loop() {
  
  read_lamps_status_from_server();
    
  read_schedule_from_server();

  monitor_physical_keys();

  put_schedule_in_action();

}

/*** returns true if enough time has elapsed since 'last', and updates 'last' in that case ***/
bool millis_elapsed(int milliseconds, int &last) {
  int now = millis();
  if (now - last > milliseconds) {
    last = now;
    return true;
  }
  return false;
}

/*** same function as above, without updating 'last' ***/
bool millis_elapsed_no_update(int milliseconds, int last) {
  int now = millis();
  if (now - last > milliseconds) {
    return true;
  }
  return false;
}

/*** read (request) the lamps' status data from server ***/
void read_lamps_status_from_server() {
  
  if (not check_connection())
    return;
  
  static int lamp_status_last = millis();
  if (millis_elapsed(lamp_status_request_period, lamp_status_last)) {
    HTTPClient http;  // object of the class HTTPClient
    http.begin(wifiClient, lamp_status_request_address);
    int httpCode = http.GET(); // send the request
    if (httpCode > 0) {  // check the returning code
      Serial.print("Lamp Status Data: ");
      String payload = http.getString();  // get the text from the destination
      Serial.println(payload);  // print the text
      digitalWrite(lamp1, payload[0] - '0');  // turn lamp1 On/Off
      digitalWrite(lamp2, payload[1] - '0');  // turn lamp2 On/Off
    } else {
      Serial.println("HTTP connection failed while requesting lamps' status!");
      last_disconnected = millis();
    }
    http.end();  // close connection
  }
  
}

/*** read (request) the scheduling data from server ***/
void read_schedule_from_server() {
  
  if (not check_connection())
    return;
  
  static int schedule_request_last = millis();
  if (millis_elapsed(schedule_request_period, schedule_request_last)) {
    HTTPClient http;  // object of the class HTTPClient
    http.begin(wifiClient, schedule_request_address);
    int httpCode = http.GET(); // send the request
    if (httpCode > 0) {  // check the returning code
      Serial.println("Scheduling data received:");
      String payload = http.getString();  // get the text from the destination
      payload = String() + millis() + " " + payload;
      Serial.println(payload);  // print the text
      writeFile(LittleFS, schedule_path, payload.c_str());  // save in a file
    } else {
      Serial.println("HTTP connection failed while requesting schedule!");
      last_disconnected = millis();
    }
    http.end();  // close connection
  }
  
}

/*** notify the server of change in lamps status ***/
void post_lamps_status_to_server() {
  
  if (not check_connection())
    return;
  
  HTTPClient http;  // object of the class HTTPClient
  http.begin(wifiClient, post_lamp_status_address);
  http.addHeader("Content-Type", "text/plain");
  String lamps_status = String(digitalRead(lamp1)) + String(digitalRead(lamp2));
  int httpResponseCode = http.POST(lamps_status);
  if (httpResponseCode > 0) {
    Serial.print("Writing to server: ");
    Serial.print(lamps_status);
    Serial.print(" , server reply: ");
    String payload = http.getString();
    Serial.println(payload);
  } else {
      Serial.println("HTTP connection failed while posting lamps' status!");
      last_disconnected = millis();
  }
  http.end();  // close connection
  
}

/*** monitor physical key changes ***/
void monitor_physical_keys() {
  
  static int switch_check_last = millis();
  static int last_v1 = digitalRead(key1);
  static int last_v2 = digitalRead(key2);
  if (millis_elapsed(switch_check_period, switch_check_last)) {
    int new_v1 = digitalRead(key1);
    int new_v2 = digitalRead(key2);
    bool notify_server = false;
    if (new_v1 != last_v1) {  // detect the change in key state
      digitalWrite(lamp1, 1 - digitalRead(lamp1));  // change the lamp state
      notify_server = true;
    }
    if (new_v2 != last_v2) {  // detect the change in key state
      digitalWrite(lamp2, 1 - digitalRead(lamp2));  // change the lamp state
      notify_server = true;
    }
    if (notify_server) {
      post_lamps_status_to_server();  // notify the server of this change
    }
    last_v1 = new_v1;
    last_v2 = new_v2;
  }
  
}

/*** read the schedule from hard drive and put it in action ***/
void put_schedule_in_action() {

  static int schedule_action_last = millis();
  if (millis_elapsed(schedule_action_period, schedule_action_last)) {
    String schedule = readFile(LittleFS, schedule_path);
    if (schedule.length() == 0) {
      Serial.println("No schedule saved on device");
    } else {
      // parse the schedule file
      int space_idx = schedule.indexOf(' ');
      int schedule_save_time = schedule.substring(0, space_idx).toInt();
      schedule = schedule.substring(space_idx + 1, schedule.length());
      int now = millis();
      while (schedule.length() > 0) {
        int space_idx = schedule.indexOf(' ');
        String lamp_num_and_status = schedule.substring(0, space_idx);
        schedule = schedule.substring(space_idx + 1, schedule.length());
        space_idx = schedule.indexOf(' ');
        int milliseconds = schedule.substring(0, space_idx).toInt();
        schedule = schedule.substring(space_idx + 1, schedule.length());
        if (schedule_save_time + milliseconds <= now and schedule_save_time + milliseconds + schedule_action_period > now) {
          // then it is time to make the change!
          int lamp_num = lamp_num_and_status[0] == '1' ? lamp1 : lamp2;
          int lamp_status = lamp_num_and_status.substring(1, 2).toInt();
          digitalWrite(lamp_num, lamp_status);
          Serial.print("Schedule put in action: ");
          Serial.println(lamp_num_and_status);
        }
      }
      post_lamps_status_to_server();  // let the server know about the changes in keys
    }
  }

}

/*** function for checking server connection ***/
bool check_connection() {
  if (WiFi.status() != WL_CONNECTED) {  // verfiy WiFi is connected
    Serial.println("WiFi disconnected :/");
    return false;
  }
  if (not millis_elapsed_no_update(disconnection_period, last_disconnected)) {  // do not try to connect if recently got disconnected
    Serial.println("Cannot reach server :/");
    return false;
  }
  
  return true;
}

/*** initialize LittleFS file system ***/
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

/*** function for reading files using LittleFS ***/
String readFile(fs::FS &fs, const char * path){
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

/*** function for writing files using LittleFS ***/
void writeFile(fs::FS &fs, const char * path, const char * message){
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
