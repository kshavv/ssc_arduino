
#include <HX711_ADC.h>
#include <SPI.h>
#include <MFRC522.h>  


//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 6; //mcu > HX711 sck pin
#define RST_PIN 9  //reset pin(RFID)
#define SS_PIN 10 //slave select pin(RFID)

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//RFID construcor
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; // Security key for accessing data.


//control variable
bool halt=0;
float averageReading=0.0;

//RFID variables
byte block_id=2;
byte block_name=4;
byte block_weight=5;
byte block_price=6;

String fetchedPid="";
String fetchedName="";
String fetchedWeight="";
String fetchedPrice="";

byte readbuffer[18];



void setup() {
  
  Serial.begin(57600); delay(10);
  SPI.begin(); 

  Serial.println("--INTITIALIZING LOAD CELL--");
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

  Serial.println("--INTITIALIZING SCANNER--");
  initialize_scanner();
  
  if(!rfid_checks())
    return; //return to the start of the loop
    
  readBlock(block_id,readbuffer,fetchedPid);
  Serial.println(fetchedPid);
  readBlock(block_name,readbuffer,fetchedName);
  Serial.println(fetchedName);
  readBlock(block_weight,readbuffer,fetchedWeight);
  Serial.println(fetchedWeight);
  readBlock(block_price,readbuffer,fetchedPrice);
  Serial.println(fetchedPrice);
  
  get_load_cell_reading();

  Serial.print("average load cell reading :"); Serial.println(averageReading);
 
      
  
  fetchedPid="";
  fetchedName="";
  fetchedWeight="";
  fetchedPrice="";
  
  //send weight and uid to database for further operations
}

/*
 * reads load cell reading and
 * updates averageReading of the   
 * item placed
 */
void get_load_cell_reading(){  
  LoadCell.tareNoDelay();
  Serial.println("put some weight");
  while(1){
    while(!LoadCell.update());
    if(LoadCell.getData()>1)
      break;
  }
  Serial.println("ready");

  float loadCellReadings[10]; 
  byte i=0;

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
   
  for(i=0;i<10;i++)
    averageReading+=loadCellReadings[i];
 
  averageReading/=10; 
}


/**
 * checks if cards is getting read properly
 */
bool rfid_checks(){
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return false;
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return false;
  Serial.println(F("**Card Detected:**"));
  Serial.println("place your RFID card");
  return true;
}


/**
 * reads data from blocks
 */
int readBlock(int blockNumber, byte arrayAddress[],String& store) 
{
  byte largestModulo4Number=blockNumber/4*4;
  byte trailerBlock=largestModulo4Number+3;

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
  
  //reading a block       
  byte buffersize = 18;//size of the buffer on which we are going to write
  MFRC522::StatusCode status;
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }

    for (uint8_t i = 0; i < 16; i++)
      store+=char(arrayAddress[i]);
}



/*
 * function for intializing scanner
 */
void initialize_scanner(){
  mfrc522.PCD_Init();     //INITIATING RFID SCANNER
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; //setting the key for data access(default key is "FF")
  
}
