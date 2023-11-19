#include <Arduino.h>

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile int NumPulsos1 = 0;
volatile int NumPulsos2 = 0;
int PinSensor1 = 14;
int PinSensor2 = 25;
float factor_conversion = 7.5;

//informacion del sistema
float maxCapacityliters = 45;
volatile float currentCapacityliters = 0;

//sensor de flote
const byte pinFloatSensor = 26;    //Max capacity float sensor  
volatile bool flagFloatSensor = false; // indica si el botón está presionado
unsigned long floatSensorCurrentTime = 0;
unsigned long floatSensorLastTime = 0; // última vez que se cambió el estado del botón
unsigned long floatSensorDelay = 3000; // última vez que se cambió el estado del botón

void IRAM_ATTR ContarPulsos1()
{
  //portENTER_CRITICAL_ISR(&mux);
  NumPulsos1++;
  //portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR ContarPulsos2()
{
  //portENTER_CRITICAL_ISR(&mux);
  NumPulsos2++;
  //portEXIT_CRITICAL_ISR(&mux);
}

void ICACHE_RAM_ATTR maxCapacidad(){ 
  if (!flagFloatSensor) { // si el botón no está presionado
      flagFloatSensor = true; // indicar que el botón está presionado
      Serial.println("deposito lleno");
      currentCapacityliters = maxCapacityliters;  //incrementamos la variable de pulsos
  }
} 


float ObtenerCaudal1(int numPulsos)
{
  float frecuencia = (float)numPulsos / 1.0; //obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m = frecuencia / factor_conversion; //calculamos el caudal en L/m
  float caudal_L_h = caudal_L_m * 60.0; //calculamos el caudal en L/h
  return caudal_L_h;
}

float ObtenerCaudal2(int numPulsos)
{
  float frecuencia = (float)numPulsos / 1.0; //obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m = frecuencia / factor_conversion; //calculamos el caudal en L/m
  float caudal_L_h = caudal_L_m * 60.0; //calculamos el caudal en L/h
  return caudal_L_h;
}

void taskSensor1(void *pvParameters){
  (void)pvParameters;

  pinMode(PinSensor1, INPUT);
  attachInterrupt(PinSensor1, ContarPulsos1, RISING);

  for (;;){
    portENTER_CRITICAL(&mux);
    int numPulsos1 = NumPulsos1;
    NumPulsos1 = 0;
    portEXIT_CRITICAL(&mux);

    float caudal = ObtenerCaudal1(numPulsos1);
    portENTER_CRITICAL_ISR(&mux);
    currentCapacityliters = currentCapacityliters - (caudal/60);
    portEXIT_CRITICAL_ISR(&mux);
    Serial.print("Sensor 1 - Caudal saliente: ");
    Serial.print(caudal);
    Serial.print(" L/h    ");
    Serial.print(caudal/60);
    Serial.print(" L/s    litros restantes: ");
    Serial.println(currentCapacityliters);


    delay(1000);
  }
}

void taskSensor2(void *pvParameters)
{
  (void)pvParameters;

  pinMode(PinSensor2, INPUT);
  attachInterrupt(PinSensor2, ContarPulsos2, RISING);

  for (;;)
  {
    portENTER_CRITICAL(&mux);
    int numPulsos2 = NumPulsos2;
    NumPulsos2 = 0;
    portEXIT_CRITICAL(&mux);

    float caudal = ObtenerCaudal2(numPulsos2);
    portENTER_CRITICAL_ISR(&mux);
    currentCapacityliters = currentCapacityliters - (caudal/60);
    portEXIT_CRITICAL_ISR(&mux);
    Serial.print("Sensor 2 - Caudal entrante: ");
    Serial.print(caudal);
    Serial.print(" L/h    ");
    Serial.print(caudal/60);
    Serial.print(" L/s    litros restantes: ");
    Serial.println(currentCapacityliters);

    delay(1000);
  }
}

void setup(){
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    taskSensor1,
    "Sensor1",
    10000,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    taskSensor2,
    "Sensor2",
    10000,
    NULL,
    1,
    NULL,
    1
  );

  pinMode(pinFloatSensor, INPUT); 
  attachInterrupt(digitalPinToInterrupt(pinFloatSensor), maxCapacidad, FALLING);

}

void loop(){

    if(flagFloatSensor){ 
    floatSensorCurrentTime = millis();
    if((floatSensorCurrentTime - floatSensorLastTime) > floatSensorDelay){ // si el botón está presionado y ha pasado suficiente tiempo desde el último cambio de estado
      flagFloatSensor = false; // indicar que el botón ya no está presionado
      floatSensorLastTime = floatSensorCurrentTime; // actualizar el tiempo del último cambio de estado
    }
  }
}
