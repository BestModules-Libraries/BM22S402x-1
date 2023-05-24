/*****************************************************************
File:        LEDIndicatesTriggerState.ino
Description: LED indicates trigger state.
             When the module is triggered,
             the LED of the module lights up for 1s and then goes out.
******************************************************************/
#include <BM22S402x-1.h>
#define RX_PIN 2 // PIR_TX
#define TX_PIN 3 // PIR_RX
#define STATUS 13 // PIR_STATUS

BM22S402x_1 pir(RX_PIN, TX_PIN); // Please uncomment out this line of code if you use SW Serial on BMduino/Arduino
// BM22S402x_1 pir(&Serial); //Please uncomment out this line of code if you use HW Serial on BMduino
// BM22S402x_1 pir(&Serial1); //Please uncomment out this line of code if you use HW Serial1 on BMduino
// BM22S402x_1 pir(&Serial2); //Please uncomment out this line of code if you use HW Serial2 on BMduino
// BM22S402x_1 pir(&Serial3); //Please uncomment out this line of code if you use HW Serial3 on BMduino
// BM22S402x_1 pir(&Serial4); //Please uncomment out this line of code if you use HW Serial4 on BMduino

uint8_t error = 0;

void setup()
{
  pir.begin();
  Serial.begin(9600);
  pinMode(STATUS, OUTPUT);
  error += pir.writeCommand(0x07, L1);   // Sensitivity level: 1 (The most sensitive)
  error += pir.writeCommand(0x09, 10);   // Delay time: 10*0.1 = 1 sec
  error += pir.writeCommand(0x0b, 5);    // Block time:  5*0.2 = 1 sec
  error += pir.writeCommand(0x05, 0x68); // LVD: 2.7V(default), LVD disable, enable PIR, single trigger, CMD mode
  if (error == 0)
  {
    Serial.println("Setting succeeded!");
  }
  else
  {
    Serial.println("Setting failed.");
  }
  while (pir.isStable() == false)
    ;
  Serial.println("Module stabilized.");
}

void loop()
{
  if (pir.isTrigger())
  {
    digitalWrite(STATUS, HIGH); // Module is triggered
  }
  else
  {
    digitalWrite(STATUS, LOW);
  }
}
