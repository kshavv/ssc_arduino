#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>


#define FIREBASE_HOST "smartcart-eb460-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "WaiTbncH5SBxswunbjEoY5KHx9Klk4VsNSqFYS1H"
#define WIFI_SSID "Pupupupupupupkardesabkochochocho"
#define WIFI_PASSWORD "sunudj129"

String productId="";
String ack="";

String values,fetchedData;

void setup() {
  //Initializes the serial connection at 9600 get sensor data from arduino.
  Serial.begin(57600); 
  delay(1000);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);   
  Firebase.pushString("carts/cart_101/status","connected");
   
}
void loop() {


  bool Sr =false;
  while(Serial.available()){

        //get sensor data from serial put in sensor_data
        fetchedData=Serial.readString(); 
        Sr=true;    
        
  }
  delay(1000);
  if(Sr==true){  
    
    values=fetchedData;
    
    //get comma indexes from values variable
    int firstCommaIndex = values.indexOf(',');
    //put data in variables 
    ack = values.substring(0, firstCommaIndex);
    productId= values.substring(firstCommaIndex+1);

    //add PRoductID
     if(ack=="ADD")
     {
        Firebase.pushString("carts/cart_101/items",productId);  
        delay(10);
     }
     
     //remove product Id
     else if(ack=="REMOVE")
     {
        Firebase.pushString("carts/cart_101/deleted_tems",productId);
        delay(10);
     }

     else if(ack=="NOT_OK")
     {
        Firebase.pushString("carts/cart_101/acknowledgement",ack);
        delay(10);
     }

     else if(ack=="OK")
     {
        Firebase.pushString("carts/cart_101/acknowledgement",ack);
        delay(10);
     }

       
    Firebase.pushString("testack",ack);
     delay(10);
 
    Firebase.pushString("testPid",productId);
     delay(10);
  
    
    if (Firebase.failed()) {  
        return;
    }
   
  }
  values="";
}