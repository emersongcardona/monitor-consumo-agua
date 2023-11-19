void ICACHE_RAM_ATTR UpdateParameters();
/////////////////////////////////////////////////
char node_Serie[20] = "IoT_device_1"; //Serial Number
////////////////////////////////////////////////
//##libraries##
#include <FS.h> 
#include <SPI.h>
#include <WiFi.h> 
#include <WiFiClient.h>              
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <PubSubClient.h> 

//Broker credentials 
#define userServerMQTT "ESP32_sensores_flujo" //broker user
#define passwordServerMQTT "12345678" //broker user password
#define serverMQTT "broker.hivemq.com"//broker direction
//#define serverMQTT "91fac7fa88af4444bd283e70ab6f558a.s1.eu.hivemq.cloud"
//#define portMQTT 1883 //broker port
#define portMQTT 1883 //broker port


WiFiClient espClient;

PubSubClient client(espClient);
//client.setInsecure();
WiFiManager wifiManager;

//## Global Variables##
//Json variables
const char* keysCredentialsToCheck[] = {"ip","device_name", "ssid", "password"};    //los valores minimos deseados en credentials file !!!!!! modificar si no se usaran
const char* keysConfigToCheck[] = {"phoneNumber", "max_litros"};    //los valores minimos deseados en config file !!!!!! modificar si no se usaran
char customName[50] = "dispositivo_1";
char nombre[30] = "dispositivo"; 
char codigo[5] = "0";    
char mySSID[25] = "SSID";
char myPassword[25] = "password";
char phoneNumber[9] = "00000000";
char max_litros[5] = "0000";
char myIp[30] = "";

//wifi configuration variables
bool shouldSaveConfig = false;
bool shouldSaveParams = false;
volatile bool wifiConfigurationflag = false;
const byte wifibutton = 18;
unsigned long currentTime;





volatile long startTime = 0;
volatile const int timeThreshold = 500;
volatile bool firstWifi = false;
volatile long startTimeWifiConnect = 0;
volatile const int timeThresholdWifiConnect = 15000;
unsigned long currentMillisWifiConnect;
volatile boolean wifiConnectedFlag = false;
bool flagServerParameters = false;

//MQTT
char configTopic[35];
char getParametersTopic[35];
char ActuadorValvulaTopic[35];
char sensorFloteTopic[35];
char sensorInTopic[35];
char sensorOutTopic[35];
char testTopic[25];




//time
volatile byte myHour;
volatile byte myMin;
boolean timeflag = false;
boolean secondflag = true;
unsigned long currentMillisTime;
volatile long startTimeTime = 0;
volatile const int timeThresholdTime = 1000;
bool flagAlertPeriod = false; 
volatile boolean flagTestTime = true;
bool flagTestTimeEnd = false;
String paramsSend = "";
struct tm * timeinfo;

//errors
volatile bool sensorError = false;


//debug 
String lastAlert = "";
char dataChar[99];
char sensorChar[10];
String data= "";
unsigned long currentMillisAlert;
volatile long startTimeAlert = 0;
volatile const int timeThresholdAlert = 2000;

//Development mode
bool debugFlag = true; //set to false to avoit most of the Serial Print
bool sentDataFlag = false;
String myIpString = "";

//##interruption functions##
void ICACHE_RAM_ATTR UpdateParameters(){
  currentTime = millis();
  if (currentTime - startTime > timeThreshold){
    startTime = currentTime;
    Serial.print("Setup Parameters");
    wifiConfigurationflag = true;
    firstWifi = true;
  }
}

//##Wifi functions##
void saveConfigCallback(){
  debug("Should save config");
  shouldSaveConfig = true;
}

void saveWifiCallback(){
  debug("[CALLBACK] saveCallback fired");
}

void configModeCallback (WiFiManager *myWiFiManager) {
  debug("[CALLBACK] configModeCallback fired");
}

void saveParamCallback(){
  debug("[CALLBACK] saveParamCallback fired");
  shouldSaveParams = true;
  wifiManager.stopConfigPortal();
}

void bindServerCallback(){
}

void handleRoute(){
  debug("[HTTP] handle route");
  wifiManager.server->send(200, "text/plain", "hello from user code");
}

//first connection to Wifi
void connectWifi(){
  debug("intentando conexion");
  Serial.print("red:");
  Serial.println(mySSID);  
  Serial.print("password:");
  Serial.println(myPassword);  
  WiFi.begin(mySSID, myPassword);
    delay(3000);
   if(WiFi.status() == WL_CONNECTED){ 
     debug("Conectado");

   }else{

   }
}

//Reconnect to WiFi
void wifiReconnect(){
currentMillisWifiConnect = millis();
  if (currentMillisWifiConnect - startTimeWifiConnect > timeThresholdWifiConnect){
    startTimeWifiConnect = currentMillisWifiConnect;
    if(WiFi.status() == WL_CONNECTED){
      debug("Wifi conectado");
      myIpString = WiFi.localIP().toString();
      myIpString.toCharArray(myIp, myIpString.length()+1);
      saveCredentialsgFile();
      wifiConnectedFlag = true;
    }else{
      debug("Wifi No conectado");
      wifiConnectedFlag = false;
      debug("intento de reconexion");
      connectWifi();
    }
  }
}

//Enable the WiFi web server
void setup_wifi(){
  debug("configurando portal");
  wifiConfigurationflag = false; 
  //loadConfigFile();
  //sendWarning("A3");
  //custom parameters web here
  //wifiManager.startConfigPortal(node_Serie, "iotDevice");
  WiFiManagerParameter custom_html("<center>Register Device</center>");


  WiFiManagerParameter custom_ssid("myssid", "RED", "", 25,"");
  WiFiManagerParameter custom_password("mypassword", "Contraseña", "", 25);
  WiFiManagerParameter custom_phoneNumber("phoneNumber", "numero de telefono", "", 12);
  WiFiManagerParameter custom_max_litros("max_litros", "max litros", "", 4);


  //callbacks web portal
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setSaveConfigCallback(saveWifiCallback);
  wifiManager.setSaveParamsCallback(saveParamCallback);

  //web portal configurations 
 // wifiManager.setSTAStaticIPConfig(IPAddress(192,168,4,2)); // set static ip,gw,sn 192.168.4.1
  wifiManager.setConfigPortalTimeout(180);
  
  wifiManager.addParameter(&custom_ssid);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_phoneNumber);
  wifiManager.addParameter(&custom_max_litros);
 
  wifiManager.startConfigPortal(node_Serie, "iotDevice");

  strcpy(mySSID, custom_ssid.getValue());
  strcpy(myPassword, custom_password.getValue());
  strcpy(phoneNumber, custom_phoneNumber.getValue());
  strcpy(max_litros, custom_max_litros.getValue());
  
  debug("The values catched are: ");
  debug("\tSSID : " + String(mySSID));
  debug("\tPassword : " + String(myPassword));
  debug("\tphone_number : " + String(phoneNumber));
  debug("\tmax_litros : " + String(max_litros));
  saveConfigFile();
  saveCredentialsgFile();
  
 connectWifi();

  // if(WiFi.status() == WL_CONNECTED){
  //   debug("Wifi connected by wifimanager");  
  //   File configFile = SPIFFS.open("/config.json", "r");
  //   size_t size = configFile.size();
  //   std::unique_ptr<char[]> buf(new char[size]);
  //   configFile.readBytes(buf.get(), size);
  //   String params = buf.get();
  //   paramsSend = params;
  //   //paramsData(paramsSend);
  //   wifiConnectedFlag = true;
  //   }else{
  //     debug("Wifi No conectado");
  //     wifiConnectedFlag = false;
  //   }

  // WiFi.mode(WIFI_STA);
  // if(firstWifi){
  //   loadConfigFile();
  //   firstWifi = false;
  // }
}

// //##MQTT functions##



boolean MQTT_reconnect() {    
  client.setServer(serverMQTT, portMQTT);                                           //Reconnect to MQTT server
  if(client.connect(node_Serie, userServerMQTT, passwordServerMQTT)) {
    debug("connected to MQTT server");
    client.subscribe(configTopic);
    client.subscribe(ActuadorValvulaTopic);
    client.subscribe(sensorFloteTopic);
    client.subscribe(sensorInTopic);
    client.subscribe(sensorOutTopic);
    client.subscribe(getParametersTopic);
    client.subscribe(testTopic);
    //paramsData(paramsSend);
  }else{
    debug("Fallo, error=");
    Serial.print(client.state());
    debug("again in 2 seconds");
    delay(1000);
  }
  return client.connected();
}

//Listen and react to the topics 
void callback(char* topic, byte* payload, unsigned int length) {
  String recv_payload = String(( char *) payload);
  debug("topic: ", topic);
  Serial.print("message: ");
  Serial.println(recv_payload);

  if(String(topic) == configTopic){
    String recv_payload = String(( char *) payload);
    char configValue[50] = "config:value";
    recv_payload.toCharArray(configValue,length +1);
    //flagServerParameters = true;
    debug("ingreso", configValue);

    if(strcmp(configValue, "get") == 0){
      // paramsData(paramsSend);
       debug("enviando parametros");
      }
    
    if(strcmp(configValue, "reset") == 0){
       sentDataFlag = false;
       autoreboot();
      }

    

      char* param = strtok(configValue, ":");
      // separa y maneja el payload escuchado en params en formato test:valor
      // if(strcmp(param, "test") == 0){
      //    Serial.println(param);
      //    param = strtok(NULL, ":");
      //    Serial.println(param);
      //    strcpy(temp_low, param);
      //    flagServerParameters = true;
      // }

      
    }
        
  if(flagServerParameters){
    saveConfigFile();
    loadConfigFile();
    flagServerParameters = false;
    currentMillisAlert= millis();
    if(currentMillisAlert - startTimeAlert > timeThresholdAlert){
      startTimeAlert = currentMillisAlert;         
    }
  }
} 

// //Estructurate and send data 
// void sendData(char* alertId, String sensorId) {
//   if(sensorId == "T1"){
//     stringtemp = String(tempMessage, precision);
//     data= stringtemp;
//     data.toCharArray(dataChar, data.length()+1);
//    sentDataFlag = client.publish(t1Topic, dataChar);
//    }

//   if(sensorId == "H1"){
//     data= (String)humeMessage;
//     data.toCharArray(dataChar, data.length()+1);
//    sentDataFlag = client.publish(h1Topic, dataChar);
//   }
//    autoreboot();
// }
 

// //Send an array that contains all parameters into this RAVEN
void sendMessage(String params){
  String datoString = params;
  byte lenghtDato = datoString.indexOf('}')+2;
  char dato[lenghtDato];
  datoString.toCharArray(dato, lenghtDato);
  Serial.println(dato);
  client.publish(getParametersTopic, dato);   //getParametersTopic
  if(WiFi.status() == WL_CONNECTED){
  }
}

//Generate the topics according to the serial number of this RAVEN 
void generateTopics(){
  String topic = ""; 

  topic = (String)node_Serie + "/config";
  topic.toCharArray(configTopic, topic.length() + 1);
  debug("topic", configTopic);

   topic = (String)node_Serie + "actuador/valvula";
  topic.toCharArray(configTopic, topic.length() + 1);
  debug("topic", ActuadorValvulaTopic);

  topic = (String)node_Serie + "/sensor/flote";
  topic.toCharArray(sensorFloteTopic, topic.length() + 1);
  debug("topic", sensorFloteTopic);

  topic = (String)node_Serie + "/sensor/in";
  topic.toCharArray(sensorInTopic, topic.length() + 1);
  debug("topic", sensorInTopic);

    topic = (String)node_Serie + "/sensor/out";
  topic.toCharArray(sensorOutTopic, topic.length() + 1);
  debug("topic", sensorOutTopic);
  
  topic = (String)node_Serie + "/action/params";
  topic.toCharArray(getParametersTopic, topic.length() + 1);
  debug("topic", getParametersTopic);

  topic = (String)node_Serie + "/test";
  topic.toCharArray(testTopic, topic.length() + 1);
  debug("topic", testTopic);
}

//##Json inside file functions##
void loadConfigFile(){                                //Read and load values into the internal memory 
  SPIFFS.format();                          //format everytime a new parameter is added
  debug("getting config File...");
  if (SPIFFS.begin()) {
    debug("config Spiffs begun");
    if (!SPIFFS.exists("/config.json")) {
        File configFile = SPIFFS.open("/config.json", "w");
        debug("SPIFFS file /config.json created");
    }
    
    //if file exists, reading and loading
    debug("reading config file");
    File configFile = SPIFFS.open("/config.json", "r");

    if(configFile){
        debug("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        String content = configFile.readString();
        Serial.println("Content of credentials file:");
        Serial.println(content); 
        String params = buf.get();
        paramsSend = params;
        debug("sending parameters");
        Serial.println(params); 
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( !deserializeError ) { 

          bool allKeysExist = true;
          for (const auto& key : keysConfigToCheck) {
              if (!json.containsKey(key)) {
                  allKeysExist = false;
                  break;
              }
          }
                      
          if (allKeysExist) {
              strcpy(phoneNumber, json["phoneNumber"]);
              strcpy(max_litros, json["max_litros"]);
              debug("config variables loaded");
          }else{
              debug("!no tiene la estructura dada \n formateando el archivo!");
              SPIFFS.remove("/config.json");
              delay(1000);
              debug("bye");
              ESP.restart();
          }
            
        }else{
          debug("failed to load config json deserializeError error");
        }
      configFile.close();
      }else{
          debug("no existe configFile");
      }
  }else{
    debug("failed to mount config File");
  }
}


//##Json inside file functions##
void loadCredentialsFile(){                                //Read and load values into the internal memory 
  debug("getting credentials File...");
  if (SPIFFS.begin()) {

    if (!SPIFFS.exists("/credetials.json")) {
        File credentialsFile = SPIFFS.open("/credetials.json", "w");
        debug("SPIFFS file /credetials.json created");
    }
    
    //if file exists, reading and loading
    debug("reading credetials file");
    File credentialsFile = SPIFFS.open("/credetials.json", "r");

      if (credentialsFile) {
        credentialsFile.close();
        credentialsFile = SPIFFS.open("/credetials.json", "r");
        debug("opened credentials file");
        // String content = credentialsFile.readString();
        // Serial.println("Content of credentials file:");
        // Serial.println(content); 
        size_t size = credentialsFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        credentialsFile.readBytes(buf.get(), size);
        String params = buf.get();
        paramsSend = params;
        Serial.println(params); 
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( !deserializeError ) {
            
            bool allKeysExist = true;
            for (const auto& key : keysCredentialsToCheck) {
                if (!json.containsKey(key)) {
                    allKeysExist = false;
                    break;
                }
            }
                        
            if (allKeysExist) {
                strcpy(customName, json["device_name"]);
                strcpy(mySSID, json["ssid"]);
                strcpy(myPassword, json["password"]);
                debug("credentials variables loaded");
            }else{
                debug("!no tiene la estructura dada \n formateando el archivo!");
                SPIFFS.remove("/credetials.json");
                delay(1000);
                debug("bye");
                ESP.restart();
            }
              
        }else{
          debug("failed to load credentials json, error deserializeError");
        }
        credentialsFile.close();
      }else{
          debug("no existe credentialsFile ");
      }
  }else{
    debug("failed to mount credentials file");
  }
}

//Save parameters into the internal memory in Json format 
bool saveConfigFile(){
    debug("saving config");
    DynamicJsonDocument json(1024);
    json["phoneNumber"] = phoneNumber;
    json["max_litros"] = max_litros;
  
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      debug("failed to open config file for writing");
      return false;
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.flush();
    configFile.close();
    debug("config saved");
    return true;
}

void saveCredentialsgFile(){
    debug("saving credentials");
    DynamicJsonDocument json(1024);
    json["ip"] = myIp;
    json["device_name"] = customName;
    json["ssid"] = mySSID;
    json["password"] = myPassword;
    
    File credentialsFile = SPIFFS.open("/credetials.json", "w");
    if (!credentialsFile) {
      debug("failed to open config file for writing");
      return;
    }

    serializeJson(json, Serial);
    serializeJson(json, credentialsFile);
    credentialsFile.flush();
    credentialsFile.close();
    debug("credentials saved");

}

//##System functions##
void autoreboot(){
  if(sentDataFlag){
    debug("sent data");
  }else{
    if(WiFi.status() == WL_CONNECTED){
    debug("unstable System, safe reboot");
    debug("good bye world");
    WiFi.disconnect();
    delay(500);
    client.disconnect();
    delay(500);
    ESP.restart();
    }
  }
}   

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(node_Serie);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setParamsPage(true);
  wifiManager.setDarkMode(true);
  delay(500);
  client.setServer(serverMQTT, portMQTT);
  client.setCallback(callback);
  client.setBufferSize(1024);
  client.setKeepAlive(150);  
  generateTopics();
  loadConfigFile();
  loadCredentialsFile();
  debug("wait to connect");
  if( (String(mySSID) == "SSID" && String(myPassword) == "password" )){
      setup_wifi();      
    }else{
      connectWifi();    
    }
  delay(3000);
  myIpString = WiFi.localIP().toString();
  myIpString.toCharArray(myIp, myIpString.length()+1);
  pinMode(wifibutton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(wifibutton), UpdateParameters, FALLING);
  myIpString = WiFi.localIP().toString();
  myIpString.toCharArray(myIp, myIpString.length()+1);
  delay(2500); 
  //saveConfigFile();
   
}


void loop() {
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    MQTT_reconnect();
  }
  client.loop();
 // wifiReconnect();
  
  if(wifiConfigurationflag){
    debug("se activo wifi setup");
    setup_wifi();
  }

  if (client.connected() && WiFi.status() == WL_CONNECTED) {
      loadConfigFile();
      loadCredentialsFile();
      delay(5000);
  }


  if(WiFi.status() == WL_CONNECTED){    // if connect to wifi blue light onboard turns on 
    digitalWrite(2, HIGH); 
  }else{
    connectWifi();
    digitalWrite(2, LOW);
  }
}


//##debug functions##
void debug(String message, int number){
  if(debugFlag){
    Serial.print(message );
    Serial.print(": ");
    Serial.println(number);
  }
}

void debug(String message, float number){
  if(debugFlag){
    Serial.print(message);
    Serial.print(": ");
    Serial.println(number);
  }
}

void debug(String message, byte number){
  if(debugFlag){
    Serial.print(message );
    Serial.print(": ");
    Serial.println(number);
  }
}

void debug(String message, char* arreglo){
  if(debugFlag){
    Serial.print(message);
    Serial.print(": ");
    Serial.println(arreglo);
  }
}

void debug(String message){
  if(debugFlag){
    Serial.println(message);
  }
}

void debug(float message){
  if(debugFlag){
    Serial.println(message);
  }
}

void debug(byte message){
  if(debugFlag){
    Serial.println(message);
  }
}

void debug(int message){
  if(debugFlag){
    Serial.println(message);
  }
}
