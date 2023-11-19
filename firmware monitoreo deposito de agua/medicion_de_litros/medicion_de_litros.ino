//informacion del sistema
float maxCapacityliters = 45;
float factor_conversion=7.12; //para convertir de frecuencia a caudal
volatile float currentCapacityliters = 0;

//sensor de flote
const byte pinFloatSensor = 26;    //Max capacity float sensor  
volatile bool flagFloatSensor = false; // indica si el botón está presionado
unsigned long floatSensorCurrentTime = 0;
unsigned long floatSensorLastTime = 0; // última vez que se cambió el estado del botón
unsigned long floatSensorDelay = 3000; // última vez que se cambió el estado del botón

//output water
volatile int NumPulsos; //variable para la cantidad de pulsos recibidos
const byte outPinSensor = 14;    //Sensor conectado en el pin 2
float volumen=0;
float volumenDiario=0;
long dt=0; //variación de tiempo por cada bucle
long t0=0; //millis() del bucle anterior

//input water
const byte  inPinSensor = 25;    //input water sensor
volatile int NumPulsosIn; //variable para la cantidad de pulsos recibidos
float volumenIn=0;
long dtIn=0; //variación de tiempo por cada bucle
long t0In=0; //millis() del bucle anterior
float volumenDiarioIn=0;

//---Función que se ejecuta en interrupción---------------
void ICACHE_RAM_ATTR ContarPulsos ()  
{ 
  NumPulsos++;  //incrementamos la variable de pulsos
} 

void ICACHE_RAM_ATTR ContarPulsosIn ()  
{ 
  NumPulsosIn++;  //incrementamos la variable de pulsos
} 

void ICACHE_RAM_ATTR maxCapacidad(){ 
  if (!flagFloatSensor) { // si el botón no está presionado
      flagFloatSensor = true; // indicar que el botón está presionado
      Serial.println("deposito lleno");
      volumen=0; // resetear el volumen para conocer el volumen actual
      currentCapacityliters = maxCapacityliters;  //incrementamos la variable de pulsos
  }
} 


//---Función para obtener frecuencia de los pulsos--------
int ObtenerFrecuecia() 
{
  int frecuencia;
  NumPulsos = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //Deshabilitamos  las interrupciones
  frecuencia=NumPulsos; //Hz(pulsos por segundo)
  return frecuencia;
}

void obtenerVolumen(){
  float frecuencia=ObtenerFrecuecia(); //obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m=frecuencia/factor_conversion; //calculamos el caudal en L/m
  dt=millis()-t0; //calculamos la variación de tiempo
  t0=millis();
  volumen=volumen+(caudal_L_m/60)*(dt/1000); // volumen(L)=caudal(L/s)*tiempo(s)
  volumenDiario = volumenDiario+(caudal_L_m/60)*(dt/1000); //volumen diario consumido
  currentCapacityliters = maxCapacityliters - volumen;
   //-----Enviamos por el puerto serie---------------
  if(caudal_L_m != 0 ){ 
    Serial.print ("Caudal saliente: "); 
    Serial.print (caudal_L_m,3); 
    Serial.print ("L/min\tVolumen: "); 
    Serial.print (volumen,3); 
    Serial.print (" L   ");
    Serial.print ("litros restantes: "); 
    Serial.print (currentCapacityliters,3); 
      Serial.print ("  litros/dia consumidos: "); 
    Serial.println (volumenDiario,3); 
  }
}


int ObtenerFrecueciaIn() 
{
  int frecuenciaIn;
  NumPulsosIn = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //Deshabilitamos  las interrupciones
  frecuenciaIn=NumPulsosIn; //Hz(pulsos por segundo)
  return frecuenciaIn;
}

void obtenerVolumenIn(){
  float frecuencia = ObtenerFrecueciaIn(); //obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m=frecuencia/factor_conversion; //calculamos el caudal en L/m
  dtIn=millis()-t0In; //calculamos la variación de tiempo
  t0In=millis();
  volumenIn=volumenIn+(caudal_L_m/60)*(dtIn/1000); // volumen(L)=caudal(L/s)*tiempo(s)
  volumenDiarioIn = volumenDiarioIn+(caudal_L_m/60)*(dtIn/1000); //volumen diario ingresado
  currentCapacityliters = maxCapacityliters + volumenIn;
   //-----Enviamos por el puerto serie---------------
  if(caudal_L_m !=0){
    Serial.print ("Caudal entrante: "); 
    Serial.print (caudal_L_m,3); 
    Serial.print ("L/min\tVolumen: "); 
    Serial.print (volumenIn,3); 
    Serial.print (" L   ");
    Serial.print ("litros restantes: "); 
    Serial.print (currentCapacityliters,3); 
    Serial.print ("  litros/dia consumidos: "); 
    Serial.println (volumenDiario,3); 
  }
}
void setup(){ 
  
  Serial.begin(115200); 
  pinMode(outPinSensor, INPUT); 
  pinMode(inPinSensor, INPUT); 
  pinMode(pinFloatSensor, INPUT); 
  attachInterrupt(digitalPinToInterrupt(outPinSensor),ContarPulsos,RISING);//(Interrupción 0(Pin2),función,Flanco de subida)
  attachInterrupt(digitalPinToInterrupt(inPinSensor),ContarPulsosIn,RISING);
  attachInterrupt(digitalPinToInterrupt(pinFloatSensor), maxCapacidad, FALLING);
  t0=millis();
  t0In=millis();
} 

void loop () {   
  obtenerVolumen();
  obtenerVolumenIn();

    if(flagFloatSensor){ 
      floatSensorCurrentTime = millis();
      if((floatSensorCurrentTime - floatSensorLastTime) > floatSensorDelay){ // si el botón está presionado y ha pasado suficiente tiempo desde el último cambio de estado
        flagFloatSensor = false; // indicar que el botón ya no está presionado
        floatSensorLastTime = floatSensorCurrentTime; // actualizar el tiempo del último cambio de estado
      }
    }
}
