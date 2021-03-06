/**
 *  **DATA FORMAT TO BE STORED ON EACH CARD**
 *  product_id
 *  product_name
 *  product_weight
 *  product_status
 */ 

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9  //reset pin
#define SS_PIN 10  //slave select pin

MFRC522 mfrc522(SS_PIN, RST_PIN);// instatiate a MFRC522 reader object.
MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information

void setup(){
  Serial.begin(57600);        
  SPI.begin();               
  mfrc522.PCD_Init();   // Init RFID reader 
  Serial.println("PUT YOUR RFID CARD NEAR THE SCANNER....");
 
  // Preparing the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  //storing  the key value to "0xFF" for new key otherwise, we would need to know the key to be able to access it
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF; 
}


int block_id=2;//dont choose trailer blocks((block_no+1)%4==0)
int block_name=4;
int block_weight=5;
int block_price=6;
int block_status=8;

//*********************SET PRODUCT DETAILS HERE******************************/
//THESE DETAILS WILL GET STORED ON THE RFID CARD AT BLOCK LOCATION  2,4,5 repectively.                       
byte product_id[16]="sm2115";
byte product_name[16]="marker";
byte product_weight[16]="17";
byte product_price[16]="20";
byte product_status[16]="1";

byte readbackblock[18];//This array is used for reading out a block.

void loop()
{
  if(!rfid_checks())
    return; //return to the start of loop

 writeBlock(block_id, product_id);
 writeBlock(block_name, product_name); 
 writeBlock(block_weight, product_weight);  
 writeBlock(block_price, product_price);
 writeBlock(block_status, product_status);
 Serial.println("data updated");
 
 readBlock(block_id, readbackblock);
 Serial.print("read block: ");
 for (int j=0 ; j<16 ; j++)
   Serial.write (readbackblock[j]);
 
 Serial.println("");
   
 readBlock(block_name, readbackblock);
 Serial.print("read block: ");
 for (int j=0 ; j<16 ; j++)
   Serial.write (readbackblock[j]);

 Serial.println("");

 readBlock(block_weight, readbackblock);
 Serial.print("read block: ");
 for (int j=0 ; j<16 ; j++)
   Serial.write (readbackblock[j]);

 Serial.println("");
   
 readBlock(block_price, readbackblock);
 Serial.print("read block: ");
 for (int j=0 ; j<16 ; j++)
   Serial.write (readbackblock[j]);

 Serial.println("");
 
 readBlock(block_status, readbackblock);
 Serial.print("read block: ");
 for (int j=0 ; j<16 ; j++)
   Serial.write (readbackblock[j]);
         
}


int writeBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;
  //checking for trailer block
  if (blockNumber > 2 && (blockNumber+1)%4 == 0){Serial.print(blockNumber);Serial.println(" is a trailer block:");return 2;}

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


int readBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }

  //reading a block       
  byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }
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