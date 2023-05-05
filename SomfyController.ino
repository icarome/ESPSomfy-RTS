#include <WiFi.h>
#include <LittleFS.h>
#include "Tcp.h"
#include "ConfigSettings.h"
#include "Network.h"
#include "Web.h"
#include "Sockets.h"
#include "Utils.h"
#include "Somfy.h"
#include "MQTT.h"
#include <AsyncTCP.h>
#include <vector>

ConfigSettings settings;
Web webServer;
SocketEmitter sockEmit;
Network net;
rebootDelay_t rebootDelay;
SomfyShadeController somfy;
MQTTClass mqtt;
char newcommand[1024];
bool isToSend = false;


bool sendCode(String command){
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, command);
  if(error){
    doc.clear();
    return false;
  }else{
    JsonObject obj = doc.as<JsonObject>();
    String command = obj["command"];
    uint8_t shadeId = obj["id"];
    uint8_t repeat = 2;
    SomfyShade* shade = somfy.getShadeById(shadeId);
    if (shade) {
      if(command == "up"){
        shade->sendCommand(somfy_commands::Up, repeat);
        return true;
      }
      if(command == "down"){
        shade->sendCommand(somfy_commands::Down, repeat);
        return true;
      }
      if(command == "stop"){
        shade->sendCommand(somfy_commands::My, repeat);
        return true;
      }
      if(command == "set"){
         uint8_t target = obj["position"];
         shade->moveToTarget(target);
         return true;
      }
    }
  }
  return true;
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
  strncpy(newcommand, (char*)data, len);
  newcommand[len] = '\0';
  isToSend = true;
  String totest = newcommand;
  Serial.println(totest);
  if(totest.endsWith("##")){
     client->close();
     client->stop();
  }
}
static void handleDisconnect(void* arg, AsyncClient* client) {
  client->close();
  client->stop();
  std::vector<AsyncClient*>::iterator position = std::find(clients.begin(), clients.end(), client);
  if (position != clients.end())
    clients.erase(position);
  free(client);
}
// static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
//   //Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
// }
static void handleNewClient(void* arg, AsyncClient* client) {
  clients.push_back(client);
  client->onData(&handleData, NULL);
  //client->onError(&handleError, NULL);
  client->onDisconnect(&handleDisconnect, NULL);
  //client->onTimeout(&handleTimeOut, NULL);
//  for(int i = 0; i<CHANNELS_NUMBER; i++){
//    String fb = feedback(i+1);
//    if (client->space() > 80 && client->canSend()) {
//        client->add(fb.c_str(), strlen(fb.c_str()));
//      }
//  }
  char new_line[1];
  new_line[0] = '\n';
  client->send();
  client->write(new_line, 1);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Startup/Boot....");
  settings.begin();
  Serial.println("Mounting File System...");
  if(LittleFS.begin()) Serial.println("File system mounted successfully");
  else Serial.println("Error mounting file system");
  if(WiFi.status() == WL_CONNECTED) WiFi.disconnect(true);
  delay(10);
  Serial.println();
  webServer.startup();
  webServer.begin();
  AsyncServer* server2 = new AsyncServer(23);
  server2->onClient(&handleNewClient, server2);
  server2->begin();
  delay(1000);
  net.setup();  
  somfy.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(rebootDelay.reboot && millis() > rebootDelay.rebootTime) ESP.restart();
  net.loop();
  somfy.loop();
  if(net.connected()) {
    webServer.loop();
    sockEmit.loop();
  }
  if(rebootDelay.reboot && millis() > rebootDelay.rebootTime) {
    net.end();
    ESP.restart();
  }
  if(isToSend){
    sendCode(newcommand);
    isToSend = false;
  }
}
