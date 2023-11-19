void ICACHE_RAM_ATTR outWaterSensorPWM();
void ICACHE_RAM_ATTR inWaterSensorPWM();
void ICACHE_RAM_ATTR maxCapacidad();

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

//output water
const byte pinOutWaterSensor = 14;    //Output water sensor 
volatile int outNumPulsos; //variable para la cantidad de pulsos recibidos

//input water
const byte pinInWaterSensor = 25;    //input water sensor
volatile int inNumPulsos; //variable para la cantidad de pulsos recibidos


//---Función que se ejecuta en interrupción---------------
void ICACHE_RAM_ATTR outWaterSensorPWM(){ 
  outNumPulsos++;  //incrementamos la variable de pulsos
} 

void ICACHE_RAM_ATTR inWaterSensorPWM(){ 
  inNumPulsos++;  //incrementamos la variable de pulsos
} 

void ICACHE_RAM_ATTR maxCapacidad(){ 
  if (!flagFloatSensor) { // si el botón no está presionado
      flagFloatSensor = true; // indicar que el botón está presionado
      Serial.println("deposito lleno");
      currentCapacityliters = maxCapacityliters;  //incrementamos la variable de pulsos
  }
} 

//---Función para obtener frecuencia de los pulsos--------
int outWaterSensorFrecuency() {
  int  outfrecuencia;
  outNumPulsos = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  //attachInterrupt(digitalPinToInterrupt(pinOutWaterSensor), outWaterSensorPWM, RISING);
  delay(1000);   //muestra de 1 segundo
  //detachInterrupt(digitalPinToInterrupt(pinOutWaterSensor));
  noInterrupts(); //Desabilitamos las interrupciones
   outfrecuencia=outNumPulsos; //Hz(pulsos por segundo)
  return  outfrecuencia;
}

void outWaterSensor(){
  float frecuencia=outWaterSensorFrecuency(); //obtenemos la Frecuencia de los pulsos en Hz
  float caudal_L_m=frecuencia/factor_conversion; //calculamos el caudal en L/m
  float caudal_L_h=caudal_L_m*60; //calculamos el caudal en L/h
  currentCapacityliters = currentCapacityliters - (caudal_L_m/60);
  if(caudal_L_m != 0 ){
      Serial.println(outNumPulsos++);
      Serial.print ("FrecuenciaPulsos salida: "); 
      Serial.print (frecuencia,0); 
      Serial.print ("Hz\tCaudal: "); 
      Serial.print ((caudal_L_m/60),3); 
      Serial.print (" L/s\t"); 
      Serial.print (caudal_L_m,3); 
      Serial.print (" L/m\t"); 
      Serial.print (caudal_L_h,3); 
      Serial.print ("L/h"); 
      Serial.print ("\tlitros actuales: "); 
      Serial.println (currentCapacityliters); 
  }

}

int inWaterSensorFrecuency() {
  int infrecuencia;
  inNumPulsos = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //Desabilitamos las interrupciones
  infrecuencia=inNumPulsos; //Hz(pulsos por segundo)
  return infrecuencia;
}

void inWaterSensor(){
  float frecuencia=inWaterSensorFrecuency(); //obtenemos la Frecuencia de los pulsos en Hz
  float caudal_L_m=frecuencia/factor_conversion; //calculamos el caudal en L/m
  float caudal_L_h=caudal_L_m*60; //calculamos el caudal en L/h
  currentCapacityliters = currentCapacityliters + (caudal_L_m/60);

  // if(currentCapacityliters > maxCapacityliters ){   // averiguar porquer esto rompe la memoria
  //   Serial.println("posible desborde de agua \n revise su cierre de flote");
  // } 
  if(caudal_L_m != 0 ){
      Serial.println(inNumPulsos++);
      Serial.print ("FrecuenciaPulsos entrada : "); 
      Serial.print (frecuencia,0); 
      Serial.print ("Hz\tCaudal: "); 
      Serial.print ((caudal_L_m/60),3); 
      Serial.print (" L/s\t"); 
      Serial.print (caudal_L_m,3); 
      Serial.print (" L/m\t"); 
      Serial.print (caudal_L_h,3); 
      Serial.print ("L/h"); 
      Serial.print ("\tlitros actuales: "); 
      Serial.println (currentCapacityliters); 
  }
}

void setup(){ 
  Serial.begin(115200); 
  pinMode(pinOutWaterSensor, INPUT);
  pinMode(pinInWaterSensor, INPUT); 
  pinMode(pinFloatSensor, INPUT); 

  //attachInterrupt(0,outWaterSensorPWM,RISING); //(Interrupcion 0(Pin2),funcion,Flanco de subida)
  attachInterrupt(digitalPinToInterrupt(pinOutWaterSensor), outWaterSensorPWM, RISING);
  attachInterrupt(digitalPinToInterrupt(pinInWaterSensor), inWaterSensorPWM, RISING);
  attachInterrupt(digitalPinToInterrupt(pinFloatSensor), maxCapacidad, FALLING);
} 

void loop (){
  outWaterSensor();
  inWaterSensor();

  if(flagFloatSensor){ 
    floatSensorCurrentTime = millis();
    if((floatSensorCurrentTime - floatSensorLastTime) > floatSensorDelay){ // si el botón está presionado y ha pasado suficiente tiempo desde el último cambio de estado
      flagFloatSensor = false; // indicar que el botón ya no está presionado
      floatSensorLastTime = floatSensorCurrentTime; // actualizar el tiempo del último cambio de estado
    }
  }
}