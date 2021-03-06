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


String values;


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//RFID construcor
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; // Security key for accessing data.

 

void setup() {
  
    Serial.begin(57600); delay(10);
    SPI.begin();   
    initialize_loadcell();
}


void loop() {
  
  initialize_scanner();  
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
    readBlock(block_status,readbuffer,fetchedStatus);
    readBlock(block_name,readbuffer,fetchedName);
    readBlock(block_weight,readbuffer,fetchedWeight);
    readBlock(block_price,readbuffer,fetchedPrice);
         
    if(fetchedStatus=="scanned"){//condition for removing item from the cart (again scan the rfid) 
        byte productStatus[16]="unscanned";
        writeBlock(block_status,productStatus);
        beep(10,1000);    
        //send product Id to esp for deleting item (with ack to delete )......................................
        send_to_esp("REMOVE",fetchedPid);
        return;
    }

    else if(fetchedStatus=="unscanned")
    {    
        byte productStatus[16]="scanned";//product shopped
        writeBlock(block_status,productStatus);
        
        if(!DISABLE_LOAD_CELL){
          get_load_cell_reading();
          
          if(verify_weight(1))
            LoadCell.tareNoDelay();   
        }
    }  
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
  
  while(1){//stop the process till the load cell detects some weight
    
    while(!LoadCell.update());   
    if(LoadCell.getData()>3){
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
      delay(20);
      if(LoadCell.getData()>5){
        beep(20,100);
        return false;
      }
    }

    //send acknowledgement to esp(OK).........
    
    halt=false;
    return true;   
  }
    
  float x=fetchedWeight.toFloat();
  if(abs(averageReading-x)<4)
  {
    totalWeight+=averageReading;
    //send acknowledgement and fetched product ID to esp***********
    send_to_esp("ADD",fetchedPid);
      
    LoadCell.tareNoDelay();   
    return true;
  }

  beep(200,1000);
  delay(1000);
  return false;
 
 }



/**
 * send data to esp
 */

void send_to_esp(String ack,String pid)
{
  delay(2000);
  values=(ack+","+pid);
  Serial.flush();
  delay(500);
  Serial.print(values);  
}

 

/*
 * code for initializing load cell and
 * setting up the callibration factor
 */
void initialize_loadcell(){
    
    LoadCell.begin();
    float calibrationValue=214.00;
    unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
    boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag()) {
      while (1);
    }
    else {
      LoadCell.setCalFactor(calibrationValue);
    }
    LoadCell.tareNoDelay();
    totalWeight=0;
  
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
      return 2;
      }

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
     return 3;//return "3" as error message
  }

  //writing the block     
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  if (status != MFRC522::STATUS_OK) {
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
         return 3;//return "3" as error message
  }
  
  //reading a block       
  byte buffersize = 18;//size of the buffer on which we are going to write
  
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
  if (status != MFRC522::STATUS_OK) {
          return 4;//return "4" as error message
  }

    for (uint8_t i = 0; i < 16; i++)
      store+=char(arrayAddress[i]);
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