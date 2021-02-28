//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

// CAN Layer functions
// -----------------------------------------------------------------------------------------------
#include "DueCANLayer.h"
extern byte canInit(byte cPort, long lBaudRate, int nTxMailboxes);
extern byte canTx(byte cPort, long lMsgID, bool bExtendedFormat, byte* cData, byte cDataLen);
extern byte canRx(byte cPort, long* lMsgID, bool* bExtendedFormat, byte* cData, byte* cDataLen);

// Timer functions
// -----------------------------------------------------------------------------------------------
#include "TimerControl.h"
extern void TimerInit(void);
extern void TimerControl(void);
extern void TimerStart(struct Timer* pTimer, int nCount);
extern void TimerReset(struct Timer* pTimer);
extern struct Timer pTimer[];

// CAN Bus Data Mapping
// -----------------------------------------------------------------------------------------------
struct Mapping
{
  byte cReceivingPort;             // 0/1
  long lReceivingMsgID;
  long lTransmittedMsgID;
};

struct Mapping CAN_DataMapping[] = {
  // cReceivingPort, lReceivingMsgID, lTransmittedMsgID
     0,              0x0CF11E05,           0x601, // Kelly Message 1
     0,              0x0CF11F05,           0x602, // Kelly Message 2
     255,            0x000,           0x000  // End of Table
};

int nMappingEntries = 0;   // Will be determined in setup()

// Internal functions
// -----------------------------------------------------------------------------------------------
void LEDControl(void);

// Module variables
// -----------------------------------------------------------------------------------------------
int TimerActivity_CAN0 = 0;
int TimerActivity_CAN1 = 0;

int LED1 = 14;
int LED2 = 15;

int nTxMailboxes = 3; // Equal portion between transmission and reception

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

unsigned long KellyCANMessage1= 0x0CF11E05;
unsigned long KellyCANMessage2= 0x0CF11F05;
int voltage;
int rpm;
int current;
int throttle;
int speed;

void setup()
{
  // Set the serial interface baud rate
  Serial.begin(115200);
 lcd.begin(16, 2);              // start the library
// lcd.setCursor(0,0);
// lcd.print("Kelly Value"); // print a simple message
 
  // Initialzie the timer control; also resets all timers
  TimerInit();

  // Determine simulation entries
  nMappingEntries = 0;
  while(1)
  {
    if(CAN_DataMapping[nMappingEntries].cReceivingPort == 0 
    || CAN_DataMapping[nMappingEntries].cReceivingPort == 1)
      nMappingEntries++;
    else
      break;
  }

  // Initialize the outputs for the LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  // Initialize both CAN controllers
  if(canInit(0, CAN_BPS_250K, nTxMailboxes) == CAN_OK)
    Serial.print("CAN0: Initialized Successfully @ 250k.\n\r");
  else
    Serial.print("CAN0: Initialization Failed.\n\r");
  
  if(canInit(1, CAN_BPS_500K, nTxMailboxes) == CAN_OK)
    Serial.print("CAN1: Initialized Successfully @ 500k.\n\r");
  else
    Serial.print("CAN1: Initialization Failed.\n\r");
  
}// end setup

void loop()
{

//   lcd.setCursor(0,1);            // move cursor to second line "1" and 0 spaces over
// lcd.print(millis()/1000);      // display seconds elapsed since power-up
// lcd.setCursor(0,1);            // move to the begining of the second line
 
  // Declarations
  byte cPort, cTxPort;
  long lMsgID;
  bool bExtendedFormat;
  byte cData[8];
  byte cDataLen;
  
  // Start timer for LED indicators
  TimerStart(&pTimerLEDs, TIMER_RATE_LED_BLINK);

  while(1)  // Endless loop
  {
    // Control LED status according to CAN traffic
    LEDControl();

    // Check for received CAN messages
    for(cPort = 0; cPort <= 1; cPort++)
    {
      if(canRx(cPort, &lMsgID, &bExtendedFormat, &cData[0], &cDataLen) == CAN_OK)
      {
        // Scan through the mapping list
        for(int nIndex = 0; nIndex < nMappingEntries; nIndex++)
        {
          if(cPort == CAN_DataMapping[nIndex].cReceivingPort
          && lMsgID == CAN_DataMapping[nIndex].lReceivingMsgID)
          {
            cTxPort = 0;
            if(cPort == 0) cTxPort = 1;
              
            if(canTx(cTxPort, CAN_DataMapping[nIndex].lTransmittedMsgID, bExtendedFormat, &cData[0], cDataLen) == CAN_ERROR)
              Serial.println("Transmision Error.");
            
          }// end if
          
        }// end for
  
      }// end if

    }// end for

    // Check for received message
    long lMsgID;
    bool bExtendedFormat;
    byte cRxData[8];
    byte cDataLen;
    if(canRx(0, &lMsgID, &bExtendedFormat, &cRxData[0], &cDataLen) == CAN_OK)
    {
      Serial.print("CAN0: Rx - MsgID:");
      Serial.print(lMsgID, HEX);
 //     Serial.print(" Ext:");
 //     Serial.print(bExtendedFormat);
  //    Serial.print(" Len:");
  //    Serial.print(cDataLen);
  //    Serial.print(" Data:");
 
    

     
   if (lMsgID==KellyCANMessage1)
   {
         lcd.setCursor(0,0); 
         rpm = (cRxData[1] << 8) + cRxData[0]; // rpm values range from 0-6000RPM
         lcd.print(rpm);
         lcd.print(" RPM ");
         int rps = rpm / 60 ;
         speed = rps * 6,33 ;
         //speed km/h = km/h= 3,14(pi)x0,56(wheel diameter in m)x13,33(rev per second(rpm/60))x3,6(m/s to km/h)
         //3,14x0,56x13,3(800/60)x3,6=84,19 km/h
         lcd.setCursor(10,0);   
          lcd.print(speed);
         lcd.print(" KMH ");
         current = ((cRxData[3] << 8) + cRxData[2]) / 10.0; // current values range from 0-400A
         voltage = ((cRxData[5] << 8) + cRxData[4]) / 10.0; // voltage values range from 0-180V
        lcd.setCursor(0,1); 
         lcd.print(voltage);
         lcd.print(" V  ");
          lcd.setCursor(10,1); 
         lcd.print(current);
         lcd.print(" A ");
        //      status.errors = parse_errors(msg.buf[6], msg.buf[7]);         
   //      lcd.setCursor(3,1); 
   //      lcd.print(cRxData[5], HEX);
    
       
  //    for(byte cIndex = 0; cIndex < cDataLen; cIndex++)
 //     {
 //       Serial.print(cRxData[cIndex], HEX);
 //       Serial.print(" ");
 //       lcd.print(cRxData[cIndex], HEX);      
 //     }// end for


  //    Serial.print("\n\r");
   }  

   if (lMsgID==KellyCANMessage2)
   {

        // throttle will only go from 0.8-4.2V
        // throttle values map from 0-255 to 0-5V
        throttle = (cRxData[0] * 5.0) / 255.0;
        // temperature offset of 40C
     //   controller_temp = msg.buf[1] - 40;
        // temperature offset of 30C
   //     motor_temp = msg.buf[2] - 30;

    //    uint8_t controller_status = msg.buf[4];
        // two least significant bits
//        status.command_status = controller_status & 0x03;
 //       status.feedback_status = (controller_status & 0x0C) >> 2;

//        uint8_t switch_status = msg.buf[5];
 
//         lcd.print(throttle);
//         lcd.print(" V ");

   }  

   
    }// end if

//    if(canRx(1, &lMsgID, &bExtendedFormat, &cRxData[0], &cDataLen) == CAN_OK)
//    {
//      Serial.print("CAN1: Rx - MsgID:");
//      Serial.print(lMsgID, HEX);
 //     Serial.print(" Ext:");
//      Serial.print(bExtendedFormat);
 //     Serial.print(" Len:");
 //     Serial.print(cDataLen);
 //     Serial.print(" Data:");

 //     for(byte cIndex = 0; cIndex < cDataLen; cIndex++)
 //     {
//        Serial.print(cRxData[cIndex], HEX);
//        Serial.print(" ");
//      }// end for

 //     Serial.print("\n\r");
      
 //   }// end if
 
  }// end while

}// end loop

// ------------------------------------------------------------------------
// LED Data Traffic
// ------------------------------------------------------------------------
// Note: CAN0 --> LED1
//       CAN1 --> LED2
//
void LEDControl(void)
{
    if(pTimerLEDs.bExpired == true)
    {
      // Restart the timer
      TimerStart(&pTimerLEDs, TIMER_RATE_LED_BLINK);

      // Check for activity on CAN0
      if(TimerActivity_CAN0 > 0)
      {
        TimerActivity_CAN0--;
        digitalWrite(LED1, HIGH);
      }// end if
      else
        digitalWrite(LED1, LOW);

      // Check for activity on CAN1
      if(TimerActivity_CAN1 > 0)
      {
        TimerActivity_CAN1--;
        digitalWrite(LED2, HIGH);
      }// end if
      else
        digitalWrite(LED2, LOW);

    }// end if

}// end LEDControl
