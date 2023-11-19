/////////////////////////////////////////////////
char node_Serie[20] = "luis-device-1"; //Serial Number
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
#define portMQTT 1883 //broker port
#define BUFFER_SIZE 256


WiFiClient espClient;

PubSubClient client(espClient);
//client.setInsecure();
WiFiManager wifiManager;

//## Global Variables##
//Json variables
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
bool sendMessageFlag = false; 

// Buffer circular
char messageBuffer[BUFFER_SIZE];
int writeIndex = 0;
int readIndex = 0;

//informacion del sistema
float maxCapacityliters = 45;
float factor_conversion=7.5; //para convertir de frecuencia a caudal
volatile float currentCapacityliters = 0;

//sensor de flote
const byte pinFloatSensor = 26;    //Max capacity float sensor  
volatile bool flagFloatSensor = false; // indica si el botón está presionado
unsigned long floatSensorCurrentTime = 0;
unsigned long floatSensorLastTime = 0; // última vez que se cambió el estado del botón
unsigned long floatSensorDelay = 3000; // última vez que se cambió el estado del botón




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

void ICACHE_RAM_ATTR maxCapacidad(){ 
  if (!flagFloatSensor) { // si el botón no está presionado
      flagFloatSensor = true; // indicar que el botón está presionado
      Serial.println("deposito lleno");
      //volumen=0; // resetear el volumen para conocer el volumen actual
      currentCapacityliters = maxCapacityliters;  //incrementamos la variable de pulsos
      addToBuffer("hola desde interrupcion");
      sendMessageFlag = true;
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
  //loadFs();
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
  saveFs();
  
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
  //   loadFs();
  //   firstWifi = false;
  // }
}

// //##MQTT functions##



boolean MQTT_reconnect() {    
  client.setServer(serverMQTT, portMQTT);                                           //Reconnect to MQTT server
  if(client.connect(node_Serie, userServerMQTT, passwordServerMQTT)) {
    //debug("connected to MQTT server");
    client.subscribe(configTopic);
    client.subscribe(ActuadorValvulaTopic);
    client.subscribe(sensorFloteTopic);
    client.subscribe(sensorInTopic);
    client.subscribe(sensorOutTopic);
    client.subscribe(getParametersTopic);
    client.subscribe(testTopic,2);
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
  Serial.print("message entrante de mqtt: ");
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
    saveFs();
    loadFs();
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

//añadir informacion al buffer circuilar 
void addToBuffer(const char* message) {
  int messageLength = strlen(message);
  int spaceAvailable = BUFFER_SIZE - 1 - (writeIndex - readIndex);
  if (spaceAvailable >= messageLength) {
    memcpy(&messageBuffer[writeIndex % BUFFER_SIZE], message, messageLength);
    writeIndex += messageLength;
  }
}

void sendMessages(char topic[]) {
  printf("El contenido de topic es: %s\n", topic);
  while (readIndex < writeIndex) {
    int messageLength = strcspn(&messageBuffer[readIndex % BUFFER_SIZE], "\n") + 1;
    char message[messageLength];
    memcpy(message, &messageBuffer[readIndex % BUFFER_SIZE], messageLength);
    message[messageLength - 1] = '\0';
    readIndex += messageLength;

    if(client.publish("IoT-device-1/test", "{ mensaje: mocos de mono}")){
      debug("envio mocos");
    }
    if (client.connected()) {
      debug("conectado.. enviando mensaje");
      printf("El contenido de topic es:%s\n", topic);
      printf("El contenido del mensaje es:%s\n", message);
      
      if(client.publish(topic, message)){
        debug("mensaje enviado");
      }else {
        debug("no se envio el mensaje valiste verga");
      }
    }
  }
}

// //Send an array that contains all parameters into this RAVEN
// void sendMessage(String params){
//   String datoString = params;
//   byte lenghtDato = datoString.indexOf('}')+2;
//   char dato[lenghtDato];
//   datoString.toCharArray(dato, lenghtDato);
//   Serial.println(dato);
//   client.publish(getParametersTopic, dato);   //getParametersTopic
//   if(WiFi.status() == WL_CONNECTED){
//   }
// }



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
void loadFs(){                                //Read and load values into the internal memory 
  //SPIFFS.format();                          //format everytime a new parameter is added
  debug("mounting FS...");
  if (SPIFFS.begin()) {
    debug("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //if file exists, reading and loading
      debug("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        debug("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        String params = buf.get();
        
        paramsSend = params;

        debug("sending parameters");
        //paramsData(paramsSend);
        
        Serial.println(params); 
  #ifdef ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
  #else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
  #endif
        debug("\nparsed json");
        debug("dentro del json");
        strcpy(customName, json["custom_name"]);
        strcpy(mySSID, json["ssid"]);
        strcpy(myPassword, json["password"]);
        strcpy(phoneNumber, json["phoneNumber"]);
        strcpy(max_litros, json["max_litros"]);
        //cast varibles
        //tem_low = atoi(temp_low);
           
        debug("parameters loaded");
        }else{
          debug("failed to load json config");
        }
      }
    }
  }else{
    debug("failed to mount FS");
  }
}

//Save parameters into the internal memory in Json format 
void saveFs (){
    debug("saving config");
#ifdef ARDUINOJSON_VERSION_MAJOR >= 6.0
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    debug("saving data");
    json["ip"] = myIp;
    json["custom_name"] = customName;
    json["serie"] = node_Serie;
    json["ssid"] = mySSID;
    json["password"] = myPassword;
    json["phoneNumber"] = phoneNumber;
    json["max_litros"] = max_litros;
  
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      debug("failed to open config file for writing");
    }

#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    debug("");
    configFile.close();
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
  loadFs();
  debug("wait to connect");
  if( (String(mySSID) == "SSID" && String(myPassword) == "password" )){
      setup_wifi();      
    }else{
      connectWifi();    
    }
  delay(2000);
  myIpString = WiFi.localIP().toString();
  myIpString.toCharArray(myIp, myIpString.length()+1);
  pinMode(wifibutton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(wifibutton), UpdateParameters, FALLING);
  myIpString = WiFi.localIP().toString();
  myIpString.toCharArray(myIp, myIpString.length()+1);
  debug("IP saved", myIp);
  delay(2500); 
  saveFs();

  pinMode(pinFloatSensor, INPUT); 
  attachInterrupt(digitalPinToInterrupt(pinFloatSensor), maxCapacidad, FALLING);
   
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


  if(WiFi.status() == WL_CONNECTED){    // if connect to wifi blue light onboard turns on 
    digitalWrite(2, HIGH); 
  }else{
    connectWifi();
    digitalWrite(2, LOW);
  }

  if(flagFloatSensor){ 
      floatSensorCurrentTime = millis();
      if((floatSensorCurrentTime - floatSensorLastTime) > floatSensorDelay){ // si el botón está presionado y ha pasado suficiente tiempo desde el último cambio de estado
        flagFloatSensor = false; // indicar que el botón ya no está presionado
        floatSensorLastTime = floatSensorCurrentTime; // actualizar el tiempo del último cambio de estado
      }
    }

   if(sendMessageFlag){
      sendMessages(testTopic);
      sendMessageFlag = false; 
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
