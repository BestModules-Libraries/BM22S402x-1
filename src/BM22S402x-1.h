/*****************************************************************
File:             BM22S402x-1.h
Author:           BESTMODULES
Description:      Define classes and required variables
History:
V1.0.1   -- initial version; 2023-2-16; Arduino IDE :v1.8.19
******************************************************************/
#ifndef _BM22S402x_1_H_
#define _BM22S402x_1_H_

#include <Arduino.h>
#include <SoftwareSerial.h>

#define BM22S402x_1_BAUD 38400

#define WRITE_OK 0
#define READ_OK 0
#define CHECK_ERROR 1
#define TIMEOUT_ERROR 2
#define CMD_ERROR 3
#define WRITE_FAILED 4

/*PIR Trigger Level: L1~L8,
  L1: Highest sensitivity
  L8: Lowest sensitivity*/
#define L1 0
#define L2 1
#define L3 2
#define L4 3
#define L5 4
#define L6 5
#define L7 6
#define L8 7

class BM22S402x_1
{
public:
  BM22S402x_1(HardwareSerial *theSerial = &Serial);
  BM22S402x_1(uint8_t rxPin, uint8_t txPin);
  void begin();

  uint8_t getDevID(uint8_t devID[]);
  uint16_t readCommand(uint8_t cmd);
  bool isStable();
  bool isTrigger();
  bool isInfoAvailable();
  void readInfopacket(uint8_t dataBuf[]);
  uint16_t readPIR();
  uint16_t readRawPIR();
  float readTemperature(bool isFahrenheit = false);

  uint8_t writeCommand(uint8_t cmd, uint16_t param);
  uint8_t enablePIR(bool isEnable = true);
  uint8_t reset();
  uint8_t restoreDefault();
  uint8_t sleep();

private:
  uint8_t _rxPin, _txPin;
  bool _isAutoMode = true;
  uint8_t _infoPacket[11] = {0};
  void clear_UART_FIFO();
  void writeBytes(uint8_t wbuf[], uint8_t wlen);
  uint8_t readBytes(uint8_t rbuf[], uint8_t rlen, uint16_t timeout = 10);
  bool findPacket(uint8_t header[], uint8_t headerLen, uint8_t packet[], uint8_t packetLen);
  uint8_t getDataPacketLen(uint8_t cmd);
  HardwareSerial *_hardSerial = NULL;
  SoftwareSerial *_softSerial = NULL;
};

#endif
