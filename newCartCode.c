#include <HX711_ADC.h>
#include <SPI.h>
#include <MFRC522.h>


//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 6; //mcu > HX711 sck pin
const int buzzerPin=7;
#define RST_PIN 9  //reset pin(RFID)
#define SS_PIN 10 //slave select pin(RFID)



//CONTROL VARIABLES
const bool DISABLE_LOAD_CELL=false;
bool halt=false;
bool poke=false;

//load Cell related
float currentWeight;
float totalWeight;
float averageReading;


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


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//RFID construcor
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; // Security key for accessing data.

 

void setup() {
  
    Serial.begin(57600); delay(10);
    SPI.begin();   
    Serial.println("--STARTING UP--");
    initialize_loadcell();
    Serial.println("place the Rfid near sensor");
}


void loop() {
  
  initialize_scanner();

  if(!DISABLE_LOAD_CELL)
    verify_weight(0);

  if(!halt)
  {
    if(rfid_checks()){
      beep(300,200);
      extract_data_from_tags();

      if(!DISABLE_LOAD_CELL){
        get_load_cell_reading();
        Serial.print("average load cell reading : "); Serial.println(averageReading);
        if(verify_weight(1))
          LoadCell.tareNoDelay();
      }  
      halt=true; 
    }
    
  }

}


/*
 * calls the read block
 * function to store data in their
 * designated variable also logs 
 * the values for debugging
 */
void extract_data_from_tags(){
  
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
 
}



/**
 * reads data from blocks
 * from the rfid tags and store 
 * them in their designated variable
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
 * reads load cell reading and
 * updates averageReading of the   
 * item placed on the loadcell after scanning
 */
bool get_load_cell_reading(){
  byte i=0;
  float reading;
  
  LoadCell.tareNoDelay();
  Serial.println("put the item on the cart");
  
  while(1){//stop the process till the load cell detects some weight
    while(!LoadCell.update());
    if(LoadCell.getData()>1){
      Serial.println("*WEIGHT DETECTED*");
      break;
    }
  }
    
  float loadCellReadings[10]; 
  for(i=0;i<30;i++)
  {        
    delay(70);
    while(!LoadCell.update());
    if(i<20)
      continue;
    reading = LoadCell.getData();
    loadCellReadings[i-20]=reading;
  }
  
  for(i=0;i<10;i++)
    averageReading+=loadCellReadings[i];
  averageReading/=10;
  return true;
}

/*
 * function for verifying weight
 * param=0(verifies unscanned weight)
 * param=1(verifies scanned weight)
 */
bool verify_weight(byte param){
  if(param==0)
  {
    byte i;
    for(i=0;i<10;i++)
    {  
      delay(50);     
      while(!LoadCell.update());
      if(LoadCell.getData()>2){
        if(!halt){ //'if' statement ensures that these will get printed only once
          Serial.println("**unscanned weight is present!!!**");
          delay(300);
          Serial.print("Please Remove the weight");
        }
        else
          Serial.print(".");
        halt=true;
        poke=true;
        return false;
      }
    }
    if(poke){
      Serial.println("");
      Serial.println("Weight verified.Ready to scan");
    }
      
    poke=false;
    halt=false;
    return true;    
  }
    
  float x=fetchedWeight.toFloat();
  if(abs(averageReading-x)<3)
  {
    Serial.println("ITEM_PLACED_SUCCESSFULLY");
    totalWeight+=averageReading;
    //send acknowledgement and fetched product ID to esp*************************************************************************************************
    LoadCell.tareNoDelay();
    delay(200);
    Serial.println("");Serial.println("");

    return true;
  }
  beep(1000,1000);
  Serial.println("WEIGHT DO NOT MATCH.PLS try SCANNING AGAIN");
  return false;
   
 }


/*
 * code for initializing load cell and
 * setting up the callibration factor
 */
void initialize_loadcell(){
    
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
    LoadCell.tareNoDelay();
    totalWeight=0;
  
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
  return true;
}


/*
 * function for intializing scanner
 * setting up the keys 
 * and resetting all rfid related variables
 */
void initialize_scanner(){
  mfrc522.PCD_Init();     //INITIATING RFID SCANNER
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; //setting the key for data access(default key is "FF") 
  //setting all variables empty before starting a new scan
  fetchedPid="";
  fetchedName="";
  fetchedWeight="";
  fetchedPrice="";

}

/*
 * activates the buzzer 
 * with the passed frequency
 * and time
 */
void beep(int hz,int d){

  tone(buzzerPin,hz,d);
  
}
