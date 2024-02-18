
#include "esp_wpa2.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

#include <DS3231.h>
#include <Wire.h>
#include <Time.h>

// Wifi Connection
const char* ssid = "eduroam";
#define EAP_ID "username"
#define EAP_USERNAME "username"
#define EAP_PASSWORD "password"

// Webserver
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1337;
//const int led_pin = 15;
AsyncWebServer server(http_port);
WebSocketsServer webSocket = WebSocketsServer(ws_port);

// Glasses
#define RTC_Line 21
#define min_night 19
#define night_day_switch 4
#define max_day 18
#define timeout 10
#define sendDataTime 1
int buzzerLux = 1000;
const int threshold = 17;
const int Touch_Pin = 12;
const int Photo_Pin = 32;
const int Buzzer_Pin = 26;

DS3231 myRTC;
bool h12;
bool hPM;
unsigned int hours;
bool toggleRun;
unsigned long startTime, endTime, intervalStartTime, intervalEndTime, hourStartTime, hourEndTime;
int brightCount = 0;
float lux;
char msg_buf[100];
//bedtime set, and target_lux

void setTime(){
  myRTC.setYear(24);
  myRTC.setMonth(2);
  myRTC.setDate(17);
  myRTC.setDoW(7);
  myRTC.setHour(21);
  myRTC.setMinute(36);
  myRTC.setSecond(10);
  myRTC.setClockMode(false);
}

int readTime(){
  bool h12, hPM;
  return myRTC.getHour(h12,hPM);
//   unsigned int minute = myRTC.getMinute();
//   unsigned int sec = myRTC.getSecond();
//   Serial.print("Time is ");
//   Serial.print(hour);
//   Serial.println(" hours");
//   Serial.print(":");
//   Serial.print(minute);
//   Serial.print(":");
//   Serial.println(sec);
}

void start_up(){
//   Serial.println(touchRead(Touch_Pin));
  toggleRun = true;
  startTime = millis();
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "IP_ADDR"){
    return WiFi.localIP().toString();
  } else if (var == "PORT"){
    return String(ws_port);
  }
  return String();
}

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // Handle text messages from client
    case WStype_TEXT:

      // Print out raw message
      Serial.printf("[%u] Received text: %s\n", client_num, payload);

      if ( strcmp((char *)payload, "lux") == 0 ) {
		    sprintf(msg_buf, "lux %d", (int)lux);
       	Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
		    webSocket.sendTXT(client_num, msg_buf);

      } else if ( strstr((char *)payload, "bl") != NULL) {
        payload[0] = '0';
        payload[1] = '0';
        payload[2] = '0';
        sprintf(msg_buf, "%s", payload);
        Serial.printf("payload: %s\n", msg_buf);
        
        buzzerLux = atoi((char *)payload);
        sprintf(msg_buf, "%d", (int)buzzerLux);
        Serial.printf("Buzzer Brightness: %d\n", buzzerLux);

      } else {
        Serial.println("[%u] Message not recognized");
      }
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

void sendLux() {
	sprintf(msg_buf, "lux %d %d", (int)lux, (int)brightCount);
	//Serial.println("Sending lux\n");
	webSocket.broadcastTXT(msg_buf);
}

			

void startGlasses() {
	Serial.begin(115200);
	Wire.begin();
	
	hours = readTime();
	startTime  = millis();
	intervalStartTime = millis();
	hourStartTime = millis();

	pinMode(Photo_Pin, INPUT);
	pinMode(Buzzer_Pin,OUTPUT);
	// setTime();

	touchAttachInterrupt(Touch_Pin, start_up, threshold);
}

void startServer() {
	// Start wifi and webserver
    delay(10);

    // Make sure we can read the file system
    if( !SPIFFS.begin()){
      Serial.println("Error mounting SPIFFS");
      while(1);
    }

    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    
    // WPA2 enterprise magic starts here
    WiFi.disconnect(true);
    WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_ID, EAP_USERNAME, EAP_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Pages
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  		request->send(SPIFFS, "/style.css", "text/css");
  	});
  	server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
  		request->send(SPIFFS, "/script.js", "text/javascript", false, processor);
  	});
//    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
//      request->send_P(200, "text/plain", getTemperature().c_str());
//    });
    server.onNotFound(onPageNotFound);
    server.begin();

	// Websocket  
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
}

void loopGlasses() {
	 //Serial.println(hours);
	 //Serial.println(toggleRun);
	endTime = (millis() - startTime)/1000;
	if(intervalEndTime >= 1) intervalStartTime = millis();
	// Serial.println(intervalEndTime);
	
	// Night
	if((hours > min_night || hours < night_day_switch) && (toggleRun && endTime < timeout)){
		//bright code start?
		int light = analogRead(Photo_Pin);
    //Serial.print("WIJdaowijdowaijfoajiewfo");
		 //Serial.println(light);

		float v_light = ((light)/4095.0) * 3.3;
		float current = v_light / 5000.0;
		float res_photo = (3.3 - v_light)/current;
		lux = (1.25*pow(10,7))*pow(res_photo,-1.4059);
		 //Serial.println(lux);
		
		intervalEndTime = (millis() - intervalStartTime)/1000;
//		Serial.println(intervalEndTime);
		if(intervalEndTime >= 1){
			if(lux > 200) brightCount += lux;
      hourEndTime = (millis() - hourStartTime)/1000;
			if(hourEndTime >= sendDataTime){
        sendLux();
				//send data after sendDataTime seconds
				// Serial.println("Send data to array");
				hourStartTime = millis();
			}
		}

	// Day
	} else if((hours > night_day_switch || hours < max_day) && (toggleRun && endTime < timeout)){
		int light = analogRead(Photo_Pin);
//    Serial.print("WIJdaowijdowaijfoajiewfo");
    
//		 Serial.println(light);

		float v_light = ((light)/4095.0) * 3.3;
		float current = v_light / 5000.0;
		float res_photo = (3.3 - v_light)/current;
		lux = (1.25*pow(10,7))*pow(res_photo,-1.4059);
    
//		 Serial.println(lux);
		
		intervalEndTime = (millis() - intervalStartTime)/1000;
//		Serial.println(intervalEndTime);
		if(intervalEndTime >= 1) {
			if(lux > 200) brightCount += lux;
			hourEndTime = (millis() - hourStartTime)/1000;
			if(hourEndTime >= sendDataTime){
        sendLux();
				//send data after sendDataTime seconds
				// Serial.println("Send data to array");
				hourStartTime = millis();
			}
		}
	} else{
		hours = readTime();
		toggleRun = false;
		delay(100);
		brightCount = 0;
	}
//	Serial.println(brightCount);
	if(brightCount >= buzzerLux){
		//activate buzzer
		digitalWrite(Buzzer_Pin,HIGH);
	} else digitalWrite(Buzzer_Pin,LOW);
}

void setup() {
	startGlasses();
	startServer();
}

void loop() {
  webSocket.loop();
	loopGlasses();
}
