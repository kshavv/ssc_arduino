
#include <HX711_ADC.h>


//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 6; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

unsigned long t = 0;

//control variable
bool halt=0;





void setup() {
  
  Serial.begin(57600); delay(10);
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue=260.24;

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
  }
  
}

void loop() {
   
  //put rfid code here to stop load cell until item is scanned
  
  if(halt==0)
  { 
    get_load_cell_reading();
  }
      
  //send weight and uid to database for further operations

}


void get_load_cell_reading(){
  
  LoadCell.tareNoDelay();
  Serial.println("put some weight");
  while(1){
    while(!LoadCell.update());
    if(LoadCell.getData()>0.5)
      break;
  }
  Serial.println("ready");

  float loadCellReadings[10]; 
  int i=0;

  for(i=0;i<30;i++)
  {        
    delay(100);
    while(!LoadCell.update());
    if(i<20)
      continue;
    float reading = LoadCell.getData();
    Serial.println(reading);
    loadCellReadings[i-20]=reading;
  }
  
  float averageReading=0.0;
  for(i=0;i<10;i++)
    averageReading+=loadCellReadings[i];
 
  averageReading/=10;
  Serial.println("average Reading:");
  Serial.println(averageReading);
  halt=1;
  
}


