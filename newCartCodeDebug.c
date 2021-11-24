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
bool halt=true;
bool poke=true;

//load Cell related
float totalWeight;
float averageReading;


//RFID variables
byte block_id=2;
byte block_name=4;
byte block_weight=5;
byte block_price=6;
byte block_status=8;

String fetchedPid="";
String fetchedName="";
String fetchedWeight="";
String fetchedPrice="";
String fetchedStatus="";

byte readbuffer[18];


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//RFID construcor
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; // Security key for accessing data.

 

void setup() {
  
    Serial.begin(57600); delay(10);
    SPI.begin();   
    Serial.println("--STARTING UP--");//debug
    initialize_loadcell();
    Serial.println("place the Rfid near sensor");   //debug
}


void loop() {
  
  initialize_scanner();

  if(DISABLE_LOAD_CELL)
  {
    halt=false;
  }
  
  if(!DISABLE_LOAD_CELL)
    verify_weight(0);

  if(!halt){
    if(rfid_checks()){
      beep(50,100);
      extract_data_from_tags();
     }  
    halt=true; 
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
    Serial.print("Product ID: ");//debug
    Serial.println(fetchedPid);//debug
    readBlock(block_status,readbuffer,fetchedStatus);
    Serial.print("Product Status: ");//debug
    Serial.println(fetchedStatus);//debug
    readBlock(block_name,readbuffer,fetchedName);
    Serial.print("Product Name: ");//debug
    Serial.println(fetchedName);//debug
    readBlock(block_weight,readbuffer,fetchedWeight);
    Serial.print("Product Weight: ");//debug
    Serial.println(fetchedWeight);//debug
    readBlock(block_price,readbuffer,fetchedPrice);
    Serial.print("Product Price: ");//debug
    Serial.println(fetchedPrice);//debug
    
      
    if(fetchedStatus=="scanned"){//condition for removing item from the cart (again scan the rfid) 
        byte productStatus[16]="unscanned";
        writeBlock(block_status,productStatus);
        beep(10,1000);    
        Serial.println("Removing Item from Cart");//debug
        delay(2000);
        //send product Id to esp for deleting item (with ack to delete ).............................................................................
        return;
    }

    else if(fetchedStatus=="unscanned")
    {    
        byte productStatus[16]="scanned";//product shopped
        writeBlock(block_status,productStatus);
        
        if(!DISABLE_LOAD_CELL){
          get_load_cell_reading();
          Serial.print("average load cell reading : "); Serial.println(averageReading);//debug
          
          if(verify_weight(1))
            LoadCell.tareNoDelay();   
        }
    }  
}


/*
*write on block 8 and 
*mark the product as bought
*(used for checks at the end of shopping)
*/
int writeBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;
  //checking for trailer block
  if (blockNumber > 2 && (blockNumber+1)%4 == 0){
      Serial.print(blockNumber);
      Serial.println(" is a trailer block:");
      return 2;
      }

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
     Serial.print("PCD_Authenticate() failed: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return 3;//return "3" as error message
  }

  //writing the block     
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  if (status != MFRC522::STATUS_OK) {
     Serial.print("MIFARE_Write() failed: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return 4;//return "4" as error message
  }
  
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
    for(byte i=0;i<10;i++)
    { 
      delay(50);     
      while(!LoadCell.update());
      if(LoadCell.getData()>2){
        Serial.print(".");//debug
        poke=true;
        beep(20,100);    
        return false;
      }
    }

    //send acknowledgement to esp(OK).................................................................................................
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
    Serial.println("");Serial.println("");//debug
    return true;
  }

  beep(200,1000);
  delay(1000);
  Serial.println("WEIGHT DO NOT MATCH.PLS try SCANNING AGAIN");//debug
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
      Serial.println("Timeout, check MCU>HX711 wiring and pin designations");//debug
      while (1);
    }
    else {
      LoadCell.setCalFactor(calibrationValue);
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
  Serial.println(F("**Card Detected:**"));//debug
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
  fetchedStatus="";
  halt=true;

}

/*
 * activates the buzzer 
 * with the passed frequency
 * and time
 */
void beep(int hz,int d){

  tone(buzzerPin,hz,d);
  
}
