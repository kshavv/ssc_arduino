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
bool rfidDetected=false;
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
  float calibrationValue=217.63;
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
  Serial.println("place the RFID tag near sensor");
  LoadCell.tareNoDelay();
}

void loop() {
  
  initialize_scanner();
  if(!rfid_checks())
    return; //return to the start of the loop
    
  readBlock(block_id,readbuffer,fetchedPid);
  Serial.print("Product ID: ");
  Serial.println(fetchedPid);
  readBlock(block_name,readbuffer,fetchedName);
  Serial.print("Product Name: ");
  Serial.println(fetchedName);
  readBlock(block_weight,readbuffer,fetchedWeight);
  Serial.print("Product Weight: ");
  Serial.println(fetchedWeight);
  readBlock(block_price,readbuffer,fetchedPrice);
  Serial.print("Product Price: ");
  Serial.println(fetchedPrice);

  if(rfidDetected){
    bool check=get_load_cell_reading();
    if(!check)
    {
      Serial.println("place the RFID tag near sensor");  
      return;
    }
  }
  rfidDetected=false;
  Serial.print("average load cell reading :"); Serial.println(averageReading);

  verify_weight();
   
  fetchedPid="";
  fetchedName="";
  fetchedWeight="";
  fetchedPrice="";

  
  
  
  //send weight and uid to database for further operations
  Serial.println("place the RFID tag near sensor");
}


void verify_weight(){
  float x=fetchedWeight.toFloat();
  if(abs(averageReading-x)<3)
  {
    Serial.println("ITEM_PLACED_SUCCESSFULLY");
    return true;
  }
  else{
    Serial.println("WEIGHT DO NOT MATCH.PLS SCAN THE ITEM AGAIN");
    return false;
  }
}


/*
 * reads load cell reading and
 * updates averageReading of the   
 * item placed
 */
bool get_load_cell_reading(){  
  byte i=0;
  float reading;
  
  //check if load is already present
  Serial.print("checking");
  for(i=0;i<10;i++)
  {
    Serial.print(".");
    delay(200);
    while(!LoadCell.update());
    reading=LoadCell.getData();
    if(reading>3){
      Serial.println("");
      Serial.println("**unscanned weight is present!!!**");
      Serial.print("PLEASE REMOVE THE WEIGHT");
      delay(1000);
      while(1){
        delay(400);
        Serial.print(".");
        while(!LoadCell.update());
        reading=LoadCell.getData();
//        if(reading<3)
//          break; 
      }
      Serial.println("");
      return false;
    }
      
  }

  Serial.println("");
  
  LoadCell.tareNoDelay();
  Serial.println("put some weight");
  while(1){
    while(!LoadCell.update());
    if(LoadCell.getData()>1)
      break;
  }
  Serial.println("ready");

  float loadCellReadings[10]; 
  

  for(i=0;i<30;i++)
  {        
    delay(100);
    while(!LoadCell.update());
    if(i<20)
      continue;
    reading = LoadCell.getData();
    loadCellReadings[i-20]=reading;
  }

  LoadCell.tareNoDelay();
   
  for(i=0;i<10;i++)
    averageReading+=loadCellReadings[i];
 
  averageReading/=10; 
  return true;
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
  rfidDetected=true;
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
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
  
  //reading a block       
  byte buffersize = 18;//size of the buffer on which we are going to write
  
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