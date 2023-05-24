/*****************************************************************
File:          BM22S402x-1.cpp
Author:        BESTMODULES
Description:   The sensor with UART and obtain the corresponding value
History:
V1.0.1   -- initial version; 2023-2-16; Arduino IDE :v1.8.19
******************************************************************/
#include "BM22S402x-1.h"

/*-------------------------------------  Public  -------------------------------------*/
/**********************************************************
Description: Constructor
Parameters:  statusPin：Status pin connection with Arduino
             *theSerial : Wire object if your board has more than one UART interface
Return: None
Others: None
**********************************************************/
BM22S402x_1::BM22S402x_1(HardwareSerial *theSerial)
{
  _softSerial = NULL;
  _hardSerial = theSerial;
}

/**********************************************************
Description: Constructor
Parameters: statusPin: Status pin connection with Arduino
            rxPin: Receive pin of the UART
            txPin: Send pin of UART
Return: None
Others: None
**********************************************************/
BM22S402x_1::BM22S402x_1(uint8_t rxPin, uint8_t txPin)
{
  _hardSerial = NULL;
  _rxPin = rxPin;
  _txPin = txPin;
  _softSerial = new SoftwareSerial(_rxPin, _txPin);
}

/**********************************************************
Description: Module initial(baud rate: 38400 bps)
Parameters: None
Return: None
Others: None
**********************************************************/
void BM22S402x_1::begin()
{
  if (_softSerial != NULL)
  {
    _softSerial->begin(BM22S402x_1_BAUD);
  }
  else
  {
    _hardSerial->begin(BM22S402x_1_BAUD);
  }
}

/**********************************************************
Description: Get Devce ID Number
Parameters: devID[]:Device ID(8 byte)
Return:   0: Read ok
          1: Check error
          2: Timeout error
          3: CMD error
Others: None
**********************************************************/
uint8_t BM22S402x_1::getDevID(uint8_t devID[])
{
  uint8_t sendBuf[4] = {0xfb, 0x03, 0x00, 0x03};
  uint8_t recBuf[14], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 14);
  if (errFlag == READ_OK)
  {
    for (uint8_t i = 0; i < 10; i++)
    {
      devID[i] = recBuf[i + 3];
    }
  }
  delay(10); // Communication interval delay
  return errFlag;
}

/**********************************************************
Description: Read parameter to module register
Parameters:  cmd: Command code for reading registers
Return: 8-bit or 16-bit parameter
Others: None
**********************************************************/
uint16_t BM22S402x_1::readCommand(uint8_t cmd)
{
  uint16_t tmp = 0;
  uint8_t sendBuf[4] = {0xfb, cmd, 0x00, 0x00};
  uint8_t recBuf[6], recBufLen, errFlag;
  sendBuf[3] = sendBuf[1] + sendBuf[2];
  writeBytes(sendBuf, 4);
  recBufLen = getDataPacketLen(cmd);
  errFlag = readBytes(recBuf, recBufLen);
  if (errFlag == READ_OK && cmd == recBuf[1])
  {
    if (cmd == 0x08)
    {
      tmp = ((uint16_t)recBuf[4] << 8) | recBuf[3];
    }
    else
    {
      tmp = recBuf[3];
    }
  }
  delay(10); // Communication interval delay
  return tmp;
}

/**********************************************************
Description: Query whether the module is stable
Parameters: None
Return:   1: The module is stable
          0: The module is not stable
Others: None
**********************************************************/
bool BM22S402x_1::isStable()
{
  bool result = false;
  uint8_t header[3] = {0xfb, 0x55, 0x07}, recBuf[11] = {0};
  uint8_t headerLen = 3, recBufLen = 11, targetDataBit = 7, delayCnt = 0;
  if (_isAutoMode == true)
  {
    while (result == false)
    {
      result = findPacket(header, headerLen, recBuf, recBufLen);
      delay(50);
      delayCnt++;
      if (delayCnt > 10)
      {
        _isAutoMode = false;
        break;
      }
    }
  }
  if (_isAutoMode == false)
  {
    if ((readCommand(0x0c) & 0x20) == 0x20)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  if (result == true)
  {
    if ((recBuf[targetDataBit] & 0x20) == 0x20)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

/**********************************************************
Description: Query whether the module is triggered by a signal
Parameters: None
Return:   1: Module is triggered by a signal
          0: The module is not triggered by a signal
Others: None
**********************************************************/
bool BM22S402x_1::isTrigger()
{
  bool result = false;
  uint8_t header1[3] = {0xfb, 0x55, 0x07}; // auto mode
  uint8_t header2[3] = {0xfb, 0x0c, 0x01}; // cmd mode
  uint8_t headerLen = 3, recBufLen, delayCnt;
  uint8_t recBuf[11] = {0}, targetDataBit;

  if (_isAutoMode == false)
  {
    delayCnt = 0;
    recBufLen = 5;
    targetDataBit = 3;
    while (result == false)
    {
      result = findPacket(header2, headerLen, recBuf, recBufLen);
      delay(50);
      delayCnt++;
      if (delayCnt > 10)
      {
        break;
      }
    }
  }
  else
  {
    delayCnt = 0;
    recBufLen = 11;
    targetDataBit = 7;
    while (result == false)
    {
      result = findPacket(header1, headerLen, recBuf, recBufLen);
      delay(50);
      delayCnt++;
      if (delayCnt > 10)
      {
        _isAutoMode = false;
        break;
      }
    }
  }

  if (result == true)
  {
    if ((recBuf[targetDataBit] & 0x01) == 0x01)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

/**********************************************************
Description: Query whether the data package automatically output by the module is received
Parameters: None
Return:   1: Received
          0: Not received
Others: None
**********************************************************/
bool BM22S402x_1::isInfoAvailable()
{
  bool result = false;
  uint8_t header[3] = {0xfb, 0x55, 0x07};
  result = findPacket(header, 3, _infoPacket, 11);
  if (result == false)
  {
    for (uint8_t i = 0; i < 11; i++)
    {
      _infoPacket[i] = 0;
    }
  }
  return result;
}

/**********************************************************
Description: Read continuous output information packet
Parameters: dataBuf[0],dataBuf[1]: Low byte of raw PIR value, high byte of raw PIR value(AD)
            dataBuf[2],dataBuf[3]: Low byte of filtered PIR value, high byte of filtered PIR value(AD)
            dataBuf[4]: PIR STATUS Register Value
            dataBuf[5],dataBuf[6]: Low byte of temperature value, high byte of temperature value
Return:   None
Others: See Datasheet for the meaning of each byte
**********************************************************/
void BM22S402x_1::readInfopacket(uint8_t dataBuf[])
{
  for (uint8_t i = 0; i < 7; i++)
  {
    dataBuf[i] = _infoPacket[i + 3];
  }
}

/**********************************************************
Description: Read Filtered PIR value
Parameters: None
Return: Filtered PIR data(2 Bytes)
Others: None
**********************************************************/
uint16_t BM22S402x_1::readPIR()
{
  uint16_t PIRVlaue = 0;
  uint8_t sendBuf[4] = {0xfb, 0x02, 0x00, 0x02};
  uint8_t recBuf[6], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 6);
  if (errFlag == READ_OK && 0x02 == recBuf[1])
  {
    PIRVlaue = ((uint16_t)recBuf[4] << 8) + recBuf[3];
  }
  return PIRVlaue;
}

/**********************************************************
Description: Read Raw PIR AD data
Parameters: none
Return: PIR AD Raw data(2 Bytes)
Others: None
**********************************************************/
uint16_t BM22S402x_1::readRawPIR()
{
  int16_t rawVlaue = 0;
  uint8_t sendBuf[4] = {0xfb, 0x01, 0x00, 0x01};
  uint8_t recBuf[6], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 6);
  if (errFlag == READ_OK && 0x01 == recBuf[1])
  {
    rawVlaue = (int16_t)((uint16_t)recBuf[4] << 8 | recBuf[3]);
  }
  return rawVlaue;
}

/**********************************************************
Description: Read Temperature
Parameters:   0: Return Centigrade
              1: Return fahrenheit
Return: Temperature(unit: Centigrade or Fahrenheit)
Others: None
**********************************************************/
float BM22S402x_1::readTemperature(bool isFahrenheit)
{
  float tempValue = 0;
  uint16_t tmp = 0;
  uint8_t sendBuf[4] = {0xfb, 0x10, 0x00, 0x10};
  uint8_t recBuf[6], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 6);
  if (errFlag == READ_OK && 0x10 == recBuf[1])
  {
    tmp = (uint16_t)recBuf[4] << 8 | recBuf[3];
    tempValue = tmp * 0.1;
    if (isFahrenheit)
    {
      tempValue = 32 + tempValue * 1.8;
    }
  }
  return tempValue;
}

/**********************************************************
Description: Write parameter to module register
Parameters: 8-bit or 16-bit data
Return:   0: Write ok
          1: Check error
          2: Timeout error
          3: CMD error
          4: Setting failed
Others: None
**********************************************************/
uint8_t BM22S402x_1::writeCommand(uint8_t cmd, uint16_t param)
{
  uint8_t paramH = param >> 8, paramL = param, dataLen = 1;
  uint8_t sendBuf[6] = {0xfb, cmd, 0x01, paramL, 0x00, 0x00};
  uint8_t recBuf[6], errFlag;
  if (cmd == 0x09)
  {
    sendBuf[2] = 0x02;
    sendBuf[4] = paramH;
    sendBuf[5] = sendBuf[1] + sendBuf[2] + sendBuf[3] + sendBuf[4];
    dataLen = 2;
  }
  else
  {
    sendBuf[4] = sendBuf[1] + sendBuf[2] + sendBuf[3];
  }
  writeBytes(sendBuf, dataLen + 4);
  errFlag = readBytes(recBuf, dataLen + 4);
  if (errFlag == READ_OK && cmd == recBuf[1])
  {
    if (dataLen == 1)
    {
      if (paramL != recBuf[3])
      {
        errFlag = WRITE_FAILED;
      }
    }
    else
    {
      if ((paramL != recBuf[3]) || (paramH != recBuf[4]))
      {
        errFlag = WRITE_FAILED;
      }
    }
  }
  delay(10); // Communication interval delay
  return errFlag;
}

/**********************************************************
Description: Enable PIR detection
Parameters:   1: Enable PIR
              0: Disable PIR
Return:   0: Write ok
          1: Check error
          2: Timeout error
          3: CMD error
          4: Setting failed
Others: None
**********************************************************/
uint8_t BM22S402x_1::enablePIR(bool isEnable)
{
  uint8_t PIRReg;
  PIRReg = readCommand(0x04);
  if (isEnable)
  {
    PIRReg |= 0x08;
  }
  else
  {
    PIRReg |= 0x00;
  }
  return writeCommand(0x05, PIRReg);
}

/**********************************************************
Description: Reset module
Parameters: None
Return:   0: Reset ok
          1: Check error
          2: Timeout error
          3: CMD error
Others: After receiving this command,
        the sensor will perform a software reset.
**********************************************************/
uint8_t BM22S402x_1::reset()
{
  uint8_t sendBuf[4] = {0xfb, 0x0f, 0x00, 0x0f};
  uint8_t recBuf[4], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 4);
  delay(10); // Communication interval delay
  if (errFlag == READ_OK && recBuf[1] == 0x0f)
  {
    delay(1000); // Wait for the module reset to complete
  }
  return errFlag;
}

/**********************************************************
Description: Restore PIR and Sensitivity register to default values
Parameters: None
Return:   0: Reset ok
          1: Check error
          2: Timeout error
          3: CMD error
          4: Setting failed
Others:
**********************************************************/
uint8_t BM22S402x_1::restoreDefault()
{
  uint8_t errFlag;
  errFlag = writeCommand(0x05, 0x6b);
  if (errFlag == 0)
  {
    errFlag = writeCommand(0x07, L1);
    return errFlag;
  }
  else
  {
    return errFlag;
  }
}

/**********************************************************
Description: Request device enter to sleep mode
Parameters:  none
Return: 0: Sleep ok
        1: Check error
        2: Timeout error
        3: CMD error
Others: Master transmits 1-byte command to sensor and get into sleep.
**********************************************************/
uint8_t BM22S402x_1::sleep()
{
  uint8_t sendBuf[4] = {0xfb, 0x0d, 0x00, 0x0d};
  uint8_t recBuf[4], errFlag;
  writeBytes(sendBuf, 4);
  errFlag = readBytes(recBuf, 4);
  delay(10); // Communication interval delay
  if (errFlag == READ_OK)
  {
    delay(100); // Wait for the module to sleep
  }
  return errFlag;
}

/*-------------------------------------  Private  -------------------------------------*/
/**********************************************************
Description: clear UART FIFO
Parameters:  none
Return:      none
Others:      none
**********************************************************/
void BM22S402x_1::clear_UART_FIFO()
{
  if (_softSerial != NULL)
  {
    while (_softSerial->available() > 0)
    {
      _softSerial->read();
    }
  }
  else
  {
    while (_hardSerial->available() > 0)
    {
      _hardSerial->read();
    }
  }
}

/**********************************************************
Description: Write data through UART
Parameters: wbuf:The array for storing Data to be sent
            wlen:Length of data sent
Return: None
Others: None
**********************************************************/
void BM22S402x_1::writeBytes(uint8_t wbuf[], uint8_t wlen)
{
  clear_UART_FIFO();

  /*Select hardSerial or softSerial according to the setting*/
  if (_softSerial != NULL)
  {
    _softSerial->write(wbuf, wlen);
  }
  if (_hardSerial != NULL)
  {
    _hardSerial->write(wbuf, wlen);
  }
  delay(10); // Wait for the module to reply data
}

/**********************************************************
Description: Read data through UART
Parameters: rbuf: Used to store received data
            rlen: Length of data to be read
Return:   0: Read ok
          1: Check error
          2: Timeout error
          3: CMD error
Others: None
**********************************************************/
uint8_t BM22S402x_1::readBytes(uint8_t rbuf[], uint8_t rlen, uint16_t timeout)
{
  uint8_t i = 0, delayCnt = 0, checkSum = 0;
  uint8_t recBuf[15] = {0};
  /* Select SoftwareSerial Interface */
  if (_softSerial != NULL)
  {
    for (i = 0; i < rlen; i++)
    {
      delayCnt = 0;
      while (_softSerial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1);
        delayCnt++;
      }
      recBuf[i] = _softSerial->read();
    }
  }
  /* Select HardwareSerial Interface */
  else
  {
    for (i = 0; i < rlen; i++)
    {
      delayCnt = 0;
      while (_hardSerial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1);
        delayCnt++;
      }
      recBuf[i] = _hardSerial->read();
    }
  }
  /* Check whether the command is incorrect */
  if (recBuf[0] == 0xFB && recBuf[1] == 0xAB && recBuf[2] == 0x00 && recBuf[3] == 0xAB)
  {
    return CMD_ERROR;
  }

  /* check Sum */
  for (i = 1; i < (rlen - 1); i++)
  {
    checkSum += recBuf[i];
  }
  if (checkSum == recBuf[rlen - 1])
  {
    for (i = 0; i < rlen; i++)
    {
      rbuf[i] = recBuf[i];
    }
    return READ_OK; // Check correct
  }
  else
  {
    return CHECK_ERROR; // Check error
  }
}

/**********************************************************
Description: Find packet with specified header
Parameters: header[]: Packet header
            headerLen: Packet header length
            packet[]: Store found packet
            packetLen: Packet length
            num: Number of executions
Return:   0: Target packet not found
          1: Target packet found
Others: See Datasheet for the meaning of each byte
**********************************************************/
bool BM22S402x_1::findPacket(uint8_t header[], uint8_t headerLen, uint8_t packet[], uint8_t packetLen)
{
  bool isHeader = false, result = false;
  uint8_t cnt = 0, i;
  uint8_t recBuf[11] = {0}, checkSum = 0, failCnt = 0, readCnt = 0;
  /* Select hardSerial or softSerial according to the setting */
  if (_softSerial != NULL)
  {
    cnt = _softSerial->available();
  }
  else if (_hardSerial != NULL)
  {
    cnt = _hardSerial->available();
  }
  if (cnt >= packetLen)
  {
    while (failCnt < 6)
    {
      /* Find header string */
      for (i = 0; i < headerLen;)
      {
        if (_softSerial != NULL)
        {
          recBuf[i] = _softSerial->read();
        }
        else if (_hardSerial != NULL)
        {
          recBuf[i] = _hardSerial->read();
        }
        if (recBuf[i] == header[i])
        {
          isHeader = true; // Fixed code is correct
          i++;             // Next byte
        }
        else if ((recBuf[i] != header[i]) && (i > 0))
        {
          isHeader = false; // Next fixed code error
          failCnt++;
          break;
        }
        else if ((recBuf[i] != header[i]) && (i == 0))
        {
          readCnt++; // header[0] not found,continue
        }
        if (readCnt > (cnt - packetLen))
        {
          return false;
        }
      }

      /* Find the correct fixed code */
      if (isHeader)
      {
        for (i = 1; i < headerLen; i++)
        {
          checkSum += recBuf[i]; // Sum checkSum
        }
        for (i = headerLen; i < packetLen; i++) // Read subsequent data
        {
          if (_softSerial != NULL)
          {
            recBuf[i] = _softSerial->read();
          }
          else if (_hardSerial != NULL)
          {
            recBuf[i] = _hardSerial->read();
          }
          checkSum += recBuf[i]; // Sum checkSum
        }
        checkSum = checkSum - recBuf[packetLen - 1];

        /* Compare whether the check code is correct */
        if (checkSum == recBuf[packetLen - 1])
        {
          for (i = 0; i < packetLen; i++)
          {
            packet[i] = recBuf[i]; // True, assign data to packet[]
          }
          result = true;
          break; // Exit "while (failCnt < 6)" loop
        }
        else
        {
          failCnt++; // Error, failCnt plus 1, return "while (failCnt < 6)" loop
          checkSum = 0;
        }
      }
    }
  }
  return result;
}

/**********************************************************
Description: Get the data packet length of the slave machine replying to this command
Parameters:  cmd: Command code
Return: Data packet length
Others: None
**********************************************************/
uint8_t BM22S402x_1::getDataPacketLen(uint8_t cmd)
{
  uint8_t dataLen = 0;
  switch (cmd)
  {
  case 0x01:
    dataLen = 2;
    break;
  case 0x02:
    dataLen = 2;
    break;
  case 0x03:
    dataLen = 10;
    break;
  case 0x04:
    dataLen = 1;
    break;
  case 0x05:
    dataLen = 1;
    break;
  case 0x06:
    dataLen = 1;
    break;
  case 0x07:
    dataLen = 1;
    break;
  case 0x08:
    dataLen = 2;
    break;
  case 0x09:
    dataLen = 2;
    break;
  case 0x0A:
    dataLen = 1;
    break;
  case 0x0B:
    dataLen = 1;
    break;
  case 0x0C:
    dataLen = 1;
    break;
  case 0x0D:
    dataLen = 1;
    break;
  case 0x0F:
    dataLen = 1;
    break;
  case 0x10:
    dataLen = 2;
    break;
  default:
    break;
  }
  return dataLen + 4;
}