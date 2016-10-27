#include <SoftwareSerial.h>

//for joystick
#define TOLERANCE   10
#define CENTER_POS  510
#define IS_CENTER(x) ((x >= (CENTER_POS-TOLERANCE))&&(x <= (CENTER_POS+TOLERANCE)))

//for Bluetooth
SoftwareSerial BTSer(2,3);

#define MAX_BT_BUF 30
#define AT_CHECK_CMD  0x0A


//for status
const int statusLedPin = 4;
const int statusBtPin = 7;


//for Rest to Last connection
const int multifunction = 5;

enum{
  VOL_DOWN,
  VOL_UP
};

/* 
  0x03 hid id
  0 bit next       0x01
  1     prev       0x02
  2     stop       0x04
  3     play/pause 0x08
  4     mute       0x10

/*

  HID 0x01
  0x80  Vol Up
  0x81  Vol Down
*/

const unsigned char sendNextPush[] = {0x0C, 0x00, 0xA1, 0x03, 0x01, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};

const unsigned char sendPrevPush[] = {0x0C, 0x00, 0xA1, 0x03, 0x02, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};
                               
const unsigned char sendStopPush[] = {0x0C, 0x00, 0xA1, 0x03, 0x04, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};
                               
const unsigned char sendPlayPush[] = {0x0C, 0x00, 0xA1, 0x03, 0x08, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};
                               
const unsigned char sendMutePush[] = {0x0C, 0x00, 0xA1, 0x03, 0x10, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};
                               
const unsigned char sendMediaRelease[] = {0x0C, 0x00, 0xA1, 0x03, 0x00, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};




const unsigned char sendVolumeDown[] = {0x0C, 0x00, 0xA1, 0x01, 0x00, 
                               0x00, 0x81, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};

const unsigned char sendVolumeUp[] = {0x0C, 0x00, 0xA1, 0x01, 0x00, 
                               0x00, 0x80, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};

const unsigned char sendKeyRelease[] = {0x0C, 0x00, 0xA1, 0x01, 0x00, 
                               0x00, 0x00, 0x00, 0x00, 0x00 ,
                               0x00, 0x00};



//bluetooth uart Setting
char setUart[] = "AT+BTUART,9600,N,1";

//Start Scan
char setScan[] = "AT+BTSCAN";


//for Bluetooth packet
int btRxCount=0;
char btBuf[MAX_BT_BUF];

//for connection
int btConnected=0;


void setup() {
  Serial.begin(9600);
  BTSer.begin(9600); 

  pinMode(statusBtPin, INPUT);
  digitalWrite(statusBtPin, HIGH);  
  
  pinMode(statusLedPin, OUTPUT);

  pinMode(multifunction, OUTPUT);

  digitalWrite(multifunction, HIGH);
  delay(5); 
  digitalWrite(multifunction, LOW);
  //if delay is 10ms, Last Connect Request 
  //if delay is 2500ms, Hard Rest (factory Reset)
  delay(100); 
  digitalWrite(multifunction, HIGH);
   
}


//Send AT Command
void sendAT(char *buf){
  for(int i=0;i<strlen(buf);i++){
    BTSer.write(buf[i]);
  }
  BTSer.write(0x0D);  
}

//Send HID Command
void sendHID(char *buf){  
  for(int i=0;i<12;i++){
    BTSer.write(buf[i]);    
  }  
}

//Send to start pairing mode to bluetooth
void sendBTScan(){
  sendAT(setScan); 
}

//Send Music Play to Bluetooth
void sendPlay(){
  sendHID(sendPlayPush);
  delay(100);
  sendHID(sendMediaRelease);
}


//Send Music Next to Bluetooth
void sendNext(){  
  sendHID(sendNextPush);
  delay(100);
  sendHID(sendMediaRelease);
}

//Send Music Prev to Bluetooth
void sendPrev(){  
  sendHID(sendPrevPush);
  delay(100);
  sendHID(sendMediaRelease);
}

//Send Volume Up & Down to Bluetooth
void sendVolume(u8 up){  
  if(up == VOL_UP){
    sendHID(sendVolumeUp);    
  }else{
    sendHID(sendVolumeDown);    
  }
  delay(100);
  sendHID(sendKeyRelease);
}


void loop() {
  
  if(BTSer.available()){
    char rx = BTSer.read();    
    
    if(btRxCount == 0){
      if(rx == AT_CHECK_CMD){
        btRxCount++;
      }
    }else{
      btBuf[btRxCount++] = rx;
      if(rx == AT_CHECK_CMD){
        //end of packet
        
        btRxCount = 0;
        if(strncmp(&btBuf[1], "CONNECT", 6) == 0){
          //connected          
          btConnected = 1;

          Serial.println("it's Connected");          
        }else if(strncmp(&btBuf[1], "DISCON", 6) == 0){
          //disconnected
          btConnected = 0; 
          sendBTScan();

          Serial.println("it's Disonnected");          
        }
      }      
    }    
    
    Serial.write(rx);
  }

  //for debug
  if(Serial.available()){
    BTSer.write(Serial.read());
  }


  //Bt Status indicator
  digitalWrite(statusLedPin, !digitalRead(statusBtPin));

  
  if(btConnected){
    //read the joystick data
    unsigned int posX = analogRead(0);
    unsigned int posY = analogRead(1);

    //if X isn't center
    if(!IS_CENTER(posX)){      
      if(posX > CENTER_POS){  //Right
        sendNext();   
      }else{                  //left
        sendPrev();       
      }         
    }        
  
    //if Y isn't center
    if(!IS_CENTER(posY)){
      if(posY > CENTER_POS){ //Down
        sendVolume(VOL_DOWN);   
      }else{                 //Up
        sendVolume(VOL_UP);       
      }         
    }
  }
  delay(100);
}
