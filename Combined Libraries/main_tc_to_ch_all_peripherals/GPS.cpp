#include "mbed.h"
#include "gps.h"
#include "millis.h"

gps::gps(void){
  // Constructor
  currentGeofenceParams.numFences = 0; // Zero the number of geofences currently in use
  moduleQueried.versionNumber = false;
}

//Configure a port to output UBX, NMEA, RTCM3 or a combination thereof
bool gps::setI2COutput(uint8_t comSettings, uint16_t maxWait){
  return (setPortOutput(COM_PORT_I2C, comSettings, maxWait));
}

//Configure a given port to output UBX, NMEA, RTCM3 or a combination thereof
//Port 0=I2c, 1=UART1, 2=UART2, 3=USB, 4=SPI
//Bit:0 = UBX, :1=NMEA, :5=RTCM3
bool gps::setPortOutput(uint8_t portID, uint8_t outStreamSettings, uint16_t maxWait){
  //Get the current config values for this port ID
  if (getPortSettings(portID, maxWait) == false)
    return (false); //Something went wrong. Bail.

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 20;
  packetCfg.startingSpot = 0;

  //payloadCfg is now loaded with current Bytes. Change only the ones we need to
  payloadCfg[14] = outStreamSettings; //OutProtocolMask LSB - Set outStream bits

  return ((sendCommand(&packetCfg, maxWait)) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

//Loads the payloadCfg array with the current protocol bits located the UBX-CFG-PRT register for a given port
bool gps::getPortSettings(uint8_t portID, uint16_t maxWait){
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 1;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = portID;

  return ((sendCommand(&packetCfg, maxWait)) == SFE_UBLOX_STATUS_DATA_RECEIVED); // We are expecting data and an ACK
}

//Save current configuration to flash and BBR (battery backed RAM)
//This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool gps::saveConfiguration(uint16_t maxWait){
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_CFG;
  packetCfg.len = 12;
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++){
    packetCfg.payload[x] = 0;
  }

  packetCfg.payload[4] = 0xFF; //Set any bit in the saveMask field to save current config to Flash and BBR
  packetCfg.payload[5] = 0xFF;

  return (sendCommand(&packetCfg, maxWait) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

//Initialize the Serial port
bool gps::begin(I2C  &wirePort, uint8_t deviceAddress){
  // I2C something(PF_0, PF_1);
  // wirePort = something;
  commType = COMM_TYPE_I2C;
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  //We expect caller to begin their I2C port, with the speed of their choice external to the library
  //But if they forget, we start the hardware here.

  //We're moving away from the practice of starting Wire hardware in a library. This is to avoid cross platform issues.
  //ie, there are some platforms that don't handle multiple starts to the wire hardware. Also, every time you start the wire
  //hardware the clock speed reverts back to 100kHz regardless of previous Wire.setClocks().
  //_i2cPort->begin();

  _gpsI2Caddress = deviceAddress; //Store the I2C address from user

  return (isConnected(0));
}

//Returns true if I2C device ack's
bool gps::isConnected(uint16_t maxWait){
  if (commType == COMM_TYPE_I2C)
  {
    // _i2cPort->beginTransmission((uint8_t)_gpsI2Caddress);
    _i2cPort->start();
    uint8_t status = _i2cPort->write(_gpsI2Caddress);
    _i2cPort->stop();

    if(status != 1){
      printf("isConnected 1\r\n");
      return false; //Sensor did not ack
    }
    // if (_i2cPort->endTransmission() != 0)
    //   return false; //Sensor did not ack
  }

  // Query navigation rate to see whether we get a meaningful response
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RATE;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  return (sendCommand(&packetCfg, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED); // We are polling the RATE so we expect data and an ACK
}

position_time_date_t gps::position_time_date(uint16_t maxWait){

  Ticker flipper;
  flipper.attach_us(&ticker_SysTick_Handler, 1000);

  if ((moduleQueried.latitude&&moduleQueried.longitude&&moduleQueried.altitude&&moduleQueried.gpsiTOW&&
    moduleQueried.gpsYear&&moduleQueried.gpsMonth&&moduleQueried.gpsDay&&moduleQueried.gpsHour&&moduleQueried.gpsMinute&&moduleQueried.gpsSecond) == false){
    getPVT(maxWait);
  }
  moduleQueried.latitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.longitude = false;
  moduleQueried.altitude = false;
  moduleQueried.gpsiTOW = false;
  moduleQueried.gpsYear = false;
  moduleQueried.gpsMonth = false;
  moduleQueried.gpsDay = false;
  moduleQueried.gpsHour = false;
  moduleQueried.gpsMinute = false;
  moduleQueried.gpsSecond = false;
  moduleQueried.all = false;

  position_time_date_t output {
    .lat = (latitude),
    .lon = (longitude),
    .alt = altitude,
    .tow = timeOfWeek,
    .Year = gpsYear,
    .Month = gpsMonth,
    .Day = gpsDay,
    .Hour = gpsHour,
    .Minute = gpsMinute,
    .Second = gpsSecond
  };
  flipper.detach();
  return output;
}
int32_t gps::getLatitude(uint16_t maxWait){
   if (moduleQueried.latitude == false){
     getPVT(maxWait);
   }
   moduleQueried.latitude = false; //Since we are about to give this to user, mark this data as stale
   moduleQueried.all = false;

   return (latitude);
 }
int32_t gps::getLongitude(uint16_t maxWait){
  if (moduleQueried.longitude == false)
    getPVT(maxWait);
  moduleQueried.longitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (longitude);
}
int32_t gps::getAltitude(uint16_t maxWait){
  if (moduleQueried.altitude == false)
    getPVT(maxWait);
  moduleQueried.altitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (altitude);
}
uint32_t gps::getTimeOfWeek(uint16_t maxWait /* = 250*/){
  if (moduleQueried.gpsiTOW == false)
    getPVT(maxWait);
  moduleQueried.gpsiTOW = false; //Since we are about to give this to user, mark this data as stale
  return (timeOfWeek);
}
//Get the current year
uint16_t gps::getYear(uint16_t maxWait){
  if (moduleQueried.gpsYear == false)
    getPVT(maxWait);
  moduleQueried.gpsYear = false; //Since we are about to give this to user, mark this data as stale
  return (gpsYear);
}
//Get the current month
uint8_t gps::getMonth(uint16_t maxWait){
  if (moduleQueried.gpsMonth == false)
    getPVT(maxWait);
  moduleQueried.gpsMonth = false; //Since we are about to give this to user, mark this data as stale
  return (gpsMonth);
}
//Get the current day
uint8_t gps::getDay(uint16_t maxWait){
  if (moduleQueried.gpsDay == false)
    getPVT(maxWait);
  moduleQueried.gpsDay = false; //Since we are about to give this to user, mark this data as stale
  return (gpsDay);
}
//Get the current hour
uint8_t gps::getHour(uint16_t maxWait){
  if (moduleQueried.gpsHour == false)
    getPVT(maxWait);
  moduleQueried.gpsHour = false; //Since we are about to give this to user, mark this data as stale
  return (gpsHour);
}
//Get the current minute
uint8_t gps::getMinute(uint16_t maxWait){
  if (moduleQueried.gpsMinute == false)
    getPVT(maxWait);
  moduleQueried.gpsMinute = false; //Since we are about to give this to user, mark this data as stale
  return (gpsMinute);
}
//Get the current second
uint8_t gps::getSecond(uint16_t maxWait){
  if (moduleQueried.gpsSecond == false)
    getPVT(maxWait);
  moduleQueried.gpsSecond = false; //Since we are about to give this to user, mark this data as stale
  return (gpsSecond);
}

void gps::enableDebugging(bool printLimitedDebug){
 // _debugSerial = &debugPort; //Grab which port the user wants us to use for debugging
 if (printLimitedDebug == false)
 {
   _printDebug = true; //Should we print the commands we send? Good for debugging
 }
 else
 {
   _printLimitedDebug = true; //Should we print limited debug messages? Good for debugging high navigation rates
 }
}

bool gps::getPVT(uint16_t maxWait){
   if (autoPVT && autoPVTImplicitUpdate)
   {
     //The GPS is automatically reporting, we just check whether we got unread data
     if (_printDebug == true)
     {
       printf("getPVT: Autoreporting\r\n");
     }
     checkUbloxInternal(&packetCfg, UBX_CLASS_NAV, UBX_NAV_PVT);
     return moduleQueried.all;
   }
   else if (autoPVT && !autoPVTImplicitUpdate)
   {
     //Someone else has to call checkUblox for us...
     if (_printDebug == true)
     {
       printf("getPVT: Exit immediately\r\n");
     }
     return (false);
   }
   else
   {
     if (_printDebug == true)
     {
       printf("getPVT: Polling\r\n");
     }

     //The GPS is not automatically reporting navigation position so we have to poll explicitly
     packetCfg.cls = UBX_CLASS_NAV;
     packetCfg.id = UBX_NAV_PVT;
     packetCfg.len = 0;
     //packetCfg.startingSpot = 20; //Begin listening at spot 20 so we can record up to 20+MAX_PAYLOAD_SIZE = 84 Bytes Note:now hard-coded in processUBX

     //The data is parsed as part of processing the response
     sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait);


     if (retVal == SFE_UBLOX_STATUS_DATA_RECEIVED){
       return (true);
     }

     if ((retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN) && (packetCfg.cls == UBX_CLASS_NAV))
     {
       if (_printDebug == true)
       {
         printf("getPVT: data was OVERWRITTEN by another NAV message (but that's OK)\r\n");
       }
       return (true);
     }

     if ((retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN) && (packetCfg.cls == UBX_CLASS_HNR))
     {
       if (_printDebug == true)
       {
         printf("getPVT: data was OVERWRITTEN by a HNR message (and that's not OK)\r\n");
       }
       return (false);
     }

     if (_printDebug == true)
     {
       printf("getPVT retVal: \r\n");
       printf(statusString(retVal));
     }
     return (false);
   }
 }

const char *gps::statusString(sfe_ublox_status_e stat){
  switch (stat)
  {
  case SFE_UBLOX_STATUS_SUCCESS:
   return "Success";
   break;
  case SFE_UBLOX_STATUS_FAIL:
   return "General Failure";
   break;
  case SFE_UBLOX_STATUS_CRC_FAIL:
   return "CRC Fail";
   break;
  case SFE_UBLOX_STATUS_TIMEOUT:
   return "Timeout";
   break;
  case SFE_UBLOX_STATUS_COMMAND_NACK:
   return "Command not acknowledged (NACK)";
   break;
  case SFE_UBLOX_STATUS_OUT_OF_RANGE:
   return "Out of range";
   break;
  case SFE_UBLOX_STATUS_INVALID_ARG:
   return "Invalid Arg";
   break;
  case SFE_UBLOX_STATUS_INVALID_OPERATION:
   return "Invalid operation";
   break;
  case SFE_UBLOX_STATUS_MEM_ERR:
   return "Memory Error";
   break;
  case SFE_UBLOX_STATUS_HW_ERR:
   return "Hardware Error";
   break;
  case SFE_UBLOX_STATUS_DATA_SENT:
   return "Data Sent";
   break;
  case SFE_UBLOX_STATUS_DATA_RECEIVED:
   return "Data Received";
   break;
  case SFE_UBLOX_STATUS_I2C_COMM_FAILURE:
   return "I2C Comm Failure";
   break;
  case SFE_UBLOX_STATUS_DATA_OVERWRITTEN:
   return "Data Packet Overwritten";
   break;
  default:
   return "Unknown Status";
   break;
  }
  return "None";
}

//Called regularly to check for available Bytes on the user' specified port
bool gps::checkUbloxInternal(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID){
   if (commType == COMM_TYPE_I2C)
     return (checkUbloxI2C(incomingUBX, requestedClass, requestedID));
   else if (commType == COMM_TYPE_SERIAL)
     return false;
     // return (checkUbloxSerial(incomingUBX, requestedClass, requestedID));
   return false;
 }
//Polls I2C for data, passing any new Bytes to process()
//Returns true if new Bytes are available
bool gps::checkUbloxI2C(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID){
   if (millis() - lastCheck >= i2cPollingWait)
   {
     //Get the number of Bytes available from the module
     uint16_t BytesAvailable = 0;
     char buffer[5]{0};
     buffer[0] = 0xFD;

     // _i2cPort->write(_gpsI2Caddress);
     // _i2cPort->write(0xFD);                     //0xFD (MSB) and 0xFE (LSB) are the registers that contain number of Bytes available
     // if (_i2cPort->endTransmission(false) != 0) //Send a restart command. Do not release bus.
     //   return (false);                          //Sensor did not ACK

     if (_i2cPort->write(_gpsI2Caddress, buffer, 1) != 0){ //Send a restart command. Do not release bus.
       return (false);                          //Sensor did not ACK
     }

     // _i2cPort->read(_gpsI2Caddress, buffer, 2);
     if ( _i2cPort->read(_gpsI2Caddress, buffer, 2) == false)
     {
         uint8_t msb = buffer[0];
         uint8_t lsb = buffer[1];
     //
     // _i2cPort->requestFrom((uint8_t)_gpsI2Caddress, (uint8_t)2);
     // if (_i2cPort->available())
     // {
     //   uint8_t msb = _i2cPort->read();
     //   uint8_t lsb = _i2cPort->read();
       if (lsb == 0xFF)
       {
         //I believe this is a u-blox bug. Device should never present an 0xFF.
         if ((_printDebug == true) || (_printLimitedDebug == true)) // Print this if doing limited debugging
         {
           printf("checkUbloxI2C: u-blox bug, length lsb is 0xFF\r\n");
         }
         // if (checksumFailurePin >= 0)
         // {
         //   digitalWrite((uint8_t)checksumFailurePin, LOW);
         //   ThisThread::sleep_for(10);
         //   digitalWrite((uint8_t)checksumFailurePin, HIGH);
         // }
         lastCheck = millis(); //Put off checking to avoid I2C bus traffic
         return (false);
       }
       BytesAvailable = (uint16_t)msb << 8 | lsb;
     }

     if (BytesAvailable == 0)
     {
       if (_printDebug == true)
       {
         printf("checkUbloxI2C: OK, zero Bytes available\r\n");
       }
       lastCheck = millis(); //Put off checking to avoid I2C bus traffic
       return (false);
     }

     //Check for undocumented bit error. We found this doing logic scans.
     //This error is rare but if we incorrectly interpret the first bit of the two 'data available' Bytes as 1
     //then we have far too many Bytes to check. May be related to I2C setup time violations: https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/40
     if (BytesAvailable & ((uint16_t)1 << 15))
     {
       //Clear the MSbit
       BytesAvailable &= ~((uint16_t)1 << 15);

       if ((_printDebug == true) || (_printLimitedDebug == true)) // Print this if doing limited debugging
       {
         printf("checkUbloxI2C: Bytes available error:");
         printf("%d\r\n",BytesAvailable);
         // if (checksumFailurePin >= 0)
         // {
         //   digitalWrite((uint8_t)checksumFailurePin, LOW);
         //   ThisThread::sleep_for(10);
         //   digitalWrite((uint8_t)checksumFailurePin, HIGH);
         // }
       }
     }

     if (BytesAvailable > 100)
     {
       if (_printDebug == true)
       {
         printf("checkUbloxI2C: Large packet of ");
         printf("%d",BytesAvailable);
         printf(" Bytes received\r\n");
       }
     }
     else
     {
       if (_printDebug == true)
       {
         printf("checkUbloxI2C: Reading ");
         printf("%d",BytesAvailable);
         printf(" Bytes\r\n");
       }
     }

     while (BytesAvailable)
     {

       // _i2cPort->write(_gpsI2Caddress);
       // _i2cPort->write(0xFF);                     //0xFF is the register to read data from
       // if (_i2cPort->endTransmission(false) != 0) //Send a restart command. Do not release bus.
       //   return (false);                          //Sensor did not ACK
       char buff[1]{0};
       buff[0] = 0xFF;
       if (_i2cPort->write(_gpsI2Caddress, buff, 1) != 0) //Send a restart command. Do not release bus.
         return (false);

       //Limit to 32 Bytes or whatever the buffer limit is for given platform
       uint16_t BytesToRead = BytesAvailable;
       if (BytesToRead > i2cTransactionSize)
         BytesToRead = i2cTransactionSize;

     TRY_AGAIN:

       char rx_buffer[BytesToRead]{0};
       if(_i2cPort->read(_gpsI2Caddress, rx_buffer, BytesToRead) == false){

       // _i2cPort->requestFrom((uint8_t)_gpsI2Caddress, (uint8_t)BytesToRead);
       // if (_i2cPort->available())
       // {
         for (uint16_t x = 0; x < BytesToRead; x++)
         {
           // uint8_t incoming = _i2cPort->read(); //Grab the actual character
           uint8_t incoming = rx_buffer[x];

           //Check to see if the first read is 0x7F. If it is, the module is not ready
           //to respond. Stop, wait, and try again
           if (x == 0)
           {
             if (incoming == 0x7F)
             {
               if ((_printDebug == true) || (_printLimitedDebug == true)) // Print this if doing limited debugging
               {
                 printf("checkUbloxU2C: u-blox error, module not ready with data\r\n");
               }
               ThisThread::sleep_for(5); //In logic analyzation, the module starting responding after 1.48ms
               // if (checksumFailurePin >= 0)
               // {
               //   digitalWrite((uint8_t)checksumFailurePin, LOW);
               //   ThisThread::sleep_for(10);
               //   digitalWrite((uint8_t)checksumFailurePin, HIGH);
               // }
               goto TRY_AGAIN;
             }
           }

           process(incoming, incomingUBX, requestedClass, requestedID); //Process this valid character
         }
       }
       else
         return (false); //Sensor did not respond

       BytesAvailable -= BytesToRead;
     }
   }

   return (true);

} //end checkUbloxI2C()


//Processes NMEA and UBX binary sentences one Byte at a time
//Take a given Byte and file it into the proper array
void gps::process(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID){
   if ((currentSentence == NONE) || (currentSentence == NMEA))
   {
     if (incoming == 0xB5) //UBX binary frames start with 0xB5, aka μ
     {
       //This is the start of a binary sentence. Reset flags.
       //We still don't know the response class
       ubxFrameCounter = 0;
       currentSentence = UBX;
       //Reset the packetBuf.counter even though we will need to reset it again when ubxFrameCounter == 2
       packetBuf.counter = 0;
       ignoreThisPayload = false; //We should not ignore this payload - yet
       //Store data in packetBuf until we know if we have a requested class and ID match
       activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF;
     }
     else if (incoming == '$')
     {
       currentSentence = NMEA;
     }
     else if (incoming == 0xD3) //RTCM frames start with 0xD3
     {
       rtcmFrameCounter = 0;
       currentSentence = RTCM;
     }
     else
     {
       //This character is unknown or we missed the previous start of a sentence
     }
   }

   //Depending on the sentence, pass the character to the individual processor
   if (currentSentence == UBX)
   {
     //Decide what type of response this is
     if ((ubxFrameCounter == 0) && (incoming != 0xB5))      //ISO 'μ'
       currentSentence = NONE;                              //Something went wrong. Reset.
     else if ((ubxFrameCounter == 1) && (incoming != 0x62)) //ASCII 'b'
       currentSentence = NONE;                              //Something went wrong. Reset.
     // Note to future self:
     // There may be some duplication / redundancy in the next few lines as processUBX will also
     // load information into packetBuf, but we'll do it here too for clarity
     else if (ubxFrameCounter == 2) //Class
     {
       // Record the class in packetBuf until we know what to do with it
       packetBuf.cls = incoming; // (Duplication)
       rollingChecksumA = 0;     //Reset our rolling checksums here (not when we receive the 0xB5)
       rollingChecksumB = 0;
       packetBuf.counter = 0;                                   //Reset the packetBuf.counter (again)
       packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // Reset the packet validity (redundant?)
       packetBuf.startingSpot = incomingUBX->startingSpot;      //Copy the startingSpot
     }
     else if (ubxFrameCounter == 3) //ID
     {
       // Record the ID in packetBuf until we know what to do with it
       packetBuf.id = incoming; // (Duplication)
       //We can now identify the type of response
       //If the packet we are receiving is not an ACK then check for a class and ID match
       if (packetBuf.cls != UBX_CLASS_ACK)
       {
         //This is not an ACK so check for a class and ID match
         if ((packetBuf.cls == requestedClass) && (packetBuf.id == requestedID))
         {
           //This is not an ACK and we have a class and ID match
           //So start diverting data into incomingUBX (usually packetCfg)
           activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
           incomingUBX->cls = packetBuf.cls; //Copy the class and ID into incomingUBX (usually packetCfg)
           incomingUBX->id = packetBuf.id;
           incomingUBX->counter = packetBuf.counter; //Copy over the .counter too
         }
         //This is not an ACK and we do not have a complete class and ID match
         //So let's check for an HPPOSLLH message arriving when we were expecting PVT and vice versa
         else if ((packetBuf.cls == requestedClass) &&
           (((packetBuf.id == UBX_NAV_PVT) && (requestedID == UBX_NAV_HPPOSLLH || requestedID == UBX_NAV_DOP)) ||
            ((packetBuf.id == UBX_NAV_HPPOSLLH) && (requestedID == UBX_NAV_PVT || requestedID == UBX_NAV_DOP)) ||
            ((packetBuf.id == UBX_NAV_DOP) && (requestedID == UBX_NAV_PVT || requestedID == UBX_NAV_HPPOSLLH))))
         {
           //This is not the message we were expecting but we start diverting data into incomingUBX (usually packetCfg) and process it anyway
           activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
           incomingUBX->cls = packetBuf.cls; //Copy the class and ID into incomingUBX (usually packetCfg)
           incomingUBX->id = packetBuf.id;
           incomingUBX->counter = packetBuf.counter; //Copy over the .counter too
           if (_printDebug == true)
           {
             printf("process: auto NAV PVT/HPPOSLLH/DOP collision: Requested ID: 0x");
             printf("%X",requestedID);
             printf(" Message ID: 0x");
             printf("%X\r\n",packetBuf.id);
           }
         }
         else if ((packetBuf.cls == requestedClass) &&
           (((packetBuf.id == UBX_HNR_ATT) && (requestedID == UBX_HNR_INS || requestedID == UBX_HNR_PVT)) ||
            ((packetBuf.id == UBX_HNR_INS) && (requestedID == UBX_HNR_ATT || requestedID == UBX_HNR_PVT)) ||
            ((packetBuf.id == UBX_HNR_PVT) && (requestedID == UBX_HNR_ATT || requestedID == UBX_HNR_INS))))
         {
           //This is not the message we were expecting but we start diverting data into incomingUBX (usually packetCfg) and process it anyway
           activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
           incomingUBX->cls = packetBuf.cls; //Copy the class and ID into incomingUBX (usually packetCfg)
           incomingUBX->id = packetBuf.id;
           incomingUBX->counter = packetBuf.counter; //Copy over the .counter too
           if (_printDebug == true)
           {
             printf("process: auto HNR ATT/INS/PVT collision: Requested ID: 0x");
             printf("%X",requestedID);
             printf(" Message ID: 0x");
             printf("%X\r\n",packetBuf.id);
           }
         }
         else
         {
           //This is not an ACK and we do not have a class and ID match
           //so we should keep diverting data into packetBuf and ignore the payload
           ignoreThisPayload = true;
         }
       }
       else
       {
         // This is an ACK so it is to early to do anything with it
         // We need to wait until we have received the length and data Bytes
         // So we should keep diverting data into packetBuf
       }
     }
     else if (ubxFrameCounter == 4) //Length LSB
     {
       //We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
       packetBuf.len = incoming; // (Duplication)
     }
     else if (ubxFrameCounter == 5) //Length MSB
     {
       //We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
       packetBuf.len |= incoming << 8; // (Duplication)
     }
     else if (ubxFrameCounter == 6) //This should be the first Byte of the payload unless .len is zero
     {
       if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
       {
         if (_printDebug == true)
         {
           printf("process: ZERO LENGTH packet received: Class: 0x");
           printf("%X",packetBuf.cls);
           printf(" ID: 0x");
           printf("%X\r\n",packetBuf.id);
         }
         //If length is zero (!) this will be the first Byte of the checksum so record it
         packetBuf.checksumA = incoming;
       }
       else
       {
         //The length is not zero so record this Byte in the payload
         packetBuf.payload[0] = incoming;
       }
     }
     else if (ubxFrameCounter == 7) //This should be the second Byte of the payload unless .len is zero or one
     {
       if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
       {
         //If length is zero (!) this will be the second Byte of the checksum so record it
         packetBuf.checksumB = incoming;
       }
       else if (packetBuf.len == 1) // Check if length is one
       {
         //The length is one so this is the first Byte of the checksum
         packetBuf.checksumA = incoming;
       }
       else // Length is >= 2 so this must be a payload Byte
       {
         packetBuf.payload[1] = incoming;
       }
       // Now that we have received two payload Bytes, we can check for a matching ACK/NACK
       if ((activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF) // If we are not already processing a data packet
           && (packetBuf.cls == UBX_CLASS_ACK)                // and if this is an ACK/NACK
           && (packetBuf.payload[0] == requestedClass)        // and if the class matches
           && (packetBuf.payload[1] == requestedID))          // and if the ID matches
       {
         if (packetBuf.len == 2) // Check if .len is 2
         {
           // Then this is a matching ACK so copy it into packetAck
           activePacketBuffer = SFE_UBLOX_PACKET_PACKETACK;
           packetAck.cls = packetBuf.cls;
           packetAck.id = packetBuf.id;
           packetAck.len = packetBuf.len;
           packetAck.counter = packetBuf.counter;
           packetAck.payload[0] = packetBuf.payload[0];
           packetAck.payload[1] = packetBuf.payload[1];
         }
         else // Length is not 2 (hopefully this is impossible!)
         {
           if (_printDebug == true)
           {
             printf("process: ACK received with .len != 2: Class: 0x");
             printf("%X",packetBuf.payload[0]);
             printf(" ID: 0x");
             printf("%X",packetBuf.payload[1]);
             printf(" len: ");
             printf("%d\r\n",packetBuf.len);
           }
         }
       }
     }

     //Divert incoming into the correct buffer
     if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETACK)
       processUBX(incoming, &packetAck, requestedClass, requestedID);
     else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
       processUBX(incoming, incomingUBX, requestedClass, requestedID);
     else // if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF)
       processUBX(incoming, &packetBuf, requestedClass, requestedID);

     //Finally, increment the frame counter
     ubxFrameCounter++;
   }
   else if (currentSentence == NMEA)
   {
     processNMEA(incoming); //Process each NMEA character
   }
   else if (currentSentence == RTCM)
   {
     processRTCMframe(incoming); //Deal with RTCM Bytes
   }
 }

 //This is the default or generic NMEA processor. We're only going to pipe the data to serial port so we can see it.
 //User could overwrite this function to pipe characters to nmea.process(c) of tinyGPS or MicroNMEA
 //Or user could pipe each character to a buffer, radio, etc.
 void gps::processNMEA(char incoming){
   //If user has assigned an output port then pipe the characters there
   // printf("%d",incoming);
   // if (_nmeaOutputPort != NULL)
   //   _nmeaOutputPort->write(incoming); //Echo this Byte to the serial port
 }

 //We need to be able to identify an RTCM packet and then the length
 //so that we know when the RTCM message is completely received and we then start
 //listening for other sentences (like NMEA or UBX)
 //RTCM packet structure is very odd. I never found RTCM STANDARD 10403.2 but
 //http://d1.amobbs.com/bbs_upload782111/files_39/ourdev_635123CK0HJT.pdf is good
 //https://dspace.cvut.cz/bitstream/handle/10467/65205/F3-BP-2016-Shkalikava-Anastasiya-Prenos%20polohove%20informace%20prostrednictvim%20datove%20site.pdf?sequence=-1
 //Lead me to: https://forum.u-blox.com/index.php/4348/how-to-read-rtcm-messages-from-neo-m8p
 //RTCM 3.2 Bytes look like this:
 //Byte 0: Always 0xD3
 //Byte 1: 6-bits of zero
 //Byte 2: 10-bits of length of this packet including the first two-ish header Bytes, + 6.
 //Byte 3 + 4 bits: Msg type 12 bits
 //Example: D3 00 7C 43 F0 ... / 0x7C = 124+6 = 130 Bytes in this packet, 0x43F = Msg type 1087
 void gps::processRTCMframe(uint8_t incoming){
   if (rtcmFrameCounter == 1)
   {
     rtcmLen = (incoming & 0x03) << 8; //Get the last two bits of this Byte. Bits 8&9 of 10-bit length
   }
   else if (rtcmFrameCounter == 2)
   {
     rtcmLen |= incoming; //Bits 0-7 of packet length
     rtcmLen += 6;        //There are 6 additional Bytes of what we presume is header, msgType, CRC, and stuff
   }
   /*else if (rtcmFrameCounter == 3)
   {
     rtcmMsgType = incoming << 4; //Message Type, MS 4 bits
   }
   else if (rtcmFrameCounter == 4)
   {
     rtcmMsgType |= (incoming >> 4); //Message Type, bits 0-7
   }*/

   rtcmFrameCounter++;

   processRTCM(incoming); //Here is where we expose this Byte to the user

   if (rtcmFrameCounter == rtcmLen)
   {
     //We're done!
     currentSentence = NONE; //Reset and start looking for next sentence type
   }
 }

 //This function is called for each Byte of an RTCM frame
 //Ths user can overwrite this function and process the RTCM frame as they please
 //Bytes can be piped to Serial or other interface. The consumer could be a radio or the internet (Ntrip broadcaster)
 void gps::processRTCM(uint8_t incoming){
   //Radio.sendReliable((String)incoming); //An example of passing this Byte to a radio

   //_debugSerial->write(incoming); //An example of passing this Byte out the serial port

   //Debug printing
   //  printf(" ");
   //  if(incoming < 0x10) printf("0");
   //  if(incoming < 0x10) printf("0");
   //  printf(incoming, HEX);
   //  if(rtcmFrameCounter % 16 == 0) printf(" ");
 }

 //Given a character, file it away into the uxb packet structure
 //Set valid to VALID or NOT_VALID once sentence is completely received and passes or fails CRC
 //The payload portion of the packet can be 100s of Bytes but the max array
 //size is MAX_PAYLOAD_SIZE Bytes. startingSpot can be set so we only record
 //a subset of Bytes within a larger packet.
 void gps::processUBX(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID){
    size_t max_payload_size = (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG) ? MAX_PAYLOAD_SIZE : 2;
    bool overrun = false;

   //Add all incoming Bytes to the rolling checksum
   //Stop at len+4 as this is the checksum Bytes to that should not be added to the rolling checksum
   if (incomingUBX->counter < incomingUBX->len + 4)
     addToChecksum(incoming);

   if (incomingUBX->counter == 0)
   {
     incomingUBX->cls = incoming;
   }
   else if (incomingUBX->counter == 1)
   {
     incomingUBX->id = incoming;
   }
   else if (incomingUBX->counter == 2) //Len LSB
   {
     incomingUBX->len = incoming;
   }
   else if (incomingUBX->counter == 3) //Len MSB
   {
     incomingUBX->len |= incoming << 8;
   }
   else if (incomingUBX->counter == incomingUBX->len + 4) //ChecksumA
   {
     incomingUBX->checksumA = incoming;
   }
   else if (incomingUBX->counter == incomingUBX->len + 5) //ChecksumB
   {
     incomingUBX->checksumB = incoming;

     currentSentence = NONE; //We're done! Reset the sentence to being looking for a new start char

     //Validate this sentence
     if ((incomingUBX->checksumA == rollingChecksumA) && (incomingUBX->checksumB == rollingChecksumB))
     {
       incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_VALID; // Flag the packet as valid

       // Let's check if the class and ID match the requestedClass and requestedID
       // Remember - this could be a data packet or an ACK packet
       if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
       {
         incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
       }

       // If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
       else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
       {
         incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
       }

       // If this is a NACK then let's check if the class and ID match the requestedClass and requestedID
       else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_NACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
       {
         incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_NOTACKNOWLEDGED; // If we have a match, set the classAndIDmatch flag to NOTACKNOWLEDGED
         if (_printDebug == true)
         {
           printf("processUBX: NACK received: Requested Class: 0x");
           printf("%X",incomingUBX->payload[0]);
           printf(" Requested ID: 0x");
           printf("%X\r\n",incomingUBX->payload[1]);
         }
       }

       //This is not an ACK and we do not have a complete class and ID match
       //So let's check for an HPPOSLLH message arriving when we were expecting PVT and vice versa
       else if ((incomingUBX->cls == requestedClass) &&
         (((incomingUBX->id == UBX_NAV_PVT) && (requestedID == UBX_NAV_HPPOSLLH || requestedID == UBX_NAV_DOP)) ||
         ((incomingUBX->id == UBX_NAV_HPPOSLLH) && (requestedID == UBX_NAV_PVT || requestedID == UBX_NAV_DOP)) ||
         ((incomingUBX->id == UBX_NAV_DOP) && (requestedID == UBX_NAV_PVT || requestedID == UBX_NAV_HPPOSLLH))))
       {
         // This isn't the message we are looking for...
         // Let's say so and leave incomingUBX->classAndIDmatch _unchanged_
         if (_printDebug == true)
         {
           printf("processUBX: auto NAV PVT/HPPOSLLH/DOP collision: Requested ID: 0x");
           printf("%X",requestedID);
           printf(" Message ID: 0x");
           printf("%X\r\n",incomingUBX->id);
         }
       }
       // Let's do the same for the HNR messages
       else if ((incomingUBX->cls == requestedClass) &&
         (((incomingUBX->id == UBX_HNR_ATT) && (requestedID == UBX_HNR_INS || requestedID == UBX_HNR_PVT)) ||
          ((incomingUBX->id == UBX_HNR_INS) && (requestedID == UBX_HNR_ATT || requestedID == UBX_HNR_PVT)) ||
          ((incomingUBX->id == UBX_HNR_PVT) && (requestedID == UBX_HNR_ATT || requestedID == UBX_HNR_INS))))
        {
          // This isn't the message we are looking for...
          // Let's say so and leave incomingUBX->classAndIDmatch _unchanged_
          if (_printDebug == true)
          {
            printf("processUBX: auto HNR ATT/INS/PVT collision: Requested ID: 0x");
            printf("%X",requestedID);
            printf(" Message ID: 0x");
            printf("%X\r\n",incomingUBX->id);
          }
        }

       if (_printDebug == true)
       {
         printf("Incoming: Size: ");
         printf("%d",incomingUBX->len);
         printf(" Received: \r\n");
         printPacket(incomingUBX);

         if (incomingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID)
         {
           printf("packetCfg now valid\r\n");
         }
         if (packetAck.valid == SFE_UBLOX_PACKET_VALIDITY_VALID)
         {
           printf("packetAck now valid\r\n");
         }
         if (incomingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID)
         {
           printf("packetCfg classAndIDmatch\r\n");
         }
         if (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID)
         {
           printf("packetAck classAndIDmatch\r\n");
         }
       }

       //We've got a valid packet, now do something with it but only if ignoreThisPayload is false
       if (ignoreThisPayload == false)
       {
         processUBXpacket(incomingUBX);
       }
     }
     else // Checksum failure
     {
       incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;

       // Let's check if the class and ID match the requestedClass and requestedID.
       // This is potentially risky as we are saying that we saw the requested Class and ID
       // but that the packet checksum failed. Potentially it could be the class or ID Bytes
       // that caused the checksum error!
       if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
       {
         incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
       }
       // If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
       else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
       {
         incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
       }

       if ((_printDebug == true) || (_printLimitedDebug == true)) // Print this if doing limited debugging
       {
         //Drive an external pin to allow for easier logic analyzation
         // if (checksumFailurePin >= 0)
         // {
         //   digitalWrite((uint8_t)checksumFailurePin, LOW);
         //   ThisThread::sleep_for(10);
         //   digitalWrite((uint8_t)checksumFailurePin, HIGH);
         // }

         printf("Checksum failed:\r\n");
         printf(" checksumA: ");
         printf("%d\r\n",incomingUBX->checksumA);
         printf(" checksumB: ");
         printf("%d\r\n",incomingUBX->checksumB);

         printf(" rollingChecksumA: ");
         printf("%d\r\n",rollingChecksumA);
         printf(" rollingChecksumB: ");
         printf("%d\r\n",rollingChecksumB);
         printf(" \r\n");

         printf("Failed  : ");
         printf("Size: ");
         printf("%d",incomingUBX->len);
         printf(" Received: ");
         printPacket(incomingUBX);
       }
     }
   }
   else //Load this Byte into the payload array
   {
     //If a UBX_NAV_PVT packet comes in asynchronously, we need to fudge the startingSpot
     uint16_t startingSpot = incomingUBX->startingSpot;
     if (incomingUBX->cls == UBX_CLASS_NAV && incomingUBX->id == UBX_NAV_PVT)
       startingSpot = 0;
     // Check if this is payload data which should be ignored
     if (ignoreThisPayload == false)
     {
       //Begin recording if counter goes past startingSpot
       if ((incomingUBX->counter - 4) >= startingSpot)
       {
         //Check to see if we have room for this Byte
         if (((incomingUBX->counter - 4) - startingSpot) < max_payload_size) //If counter = 208, starting spot = 200, we're good to record.
         {
           incomingUBX->payload[incomingUBX->counter - 4 - startingSpot] = incoming; //Store this Byte into payload array
         }
         else
         {
           overrun = true;
         }
       }
     }
   }

   //Increment the counter
   incomingUBX->counter++;

   if (overrun || (incomingUBX->counter == MAX_PAYLOAD_SIZE))
   {
     //Something has gone very wrong
     currentSentence = NONE; //Reset the sentence to being looking for a new start char
     if ((_printDebug == true) || (_printLimitedDebug == true)) // Print this if doing limited debugging
     {
       if (overrun)
         printf("processUBX: buffer overrun detected");
       else
         printf("processUBX: counter hit MAX_PAYLOAD_SIZE");
     }
   }
 }

 //Given a message and a Byte, add to rolling "8-Bit Fletcher" checksum
 //This is used when receiving messages from module
 void gps::addToChecksum(uint8_t incoming){
   rollingChecksumA += incoming;
   rollingChecksumB += rollingChecksumA;
 }

 //Pretty prints the current ubxPacket
 void gps::printPacket(ubxPacket *packet){
   if (_printDebug == true)
   {
     printf("CLS:");
     if (packet->cls == UBX_CLASS_NAV) //1
       printf("NAV");
     else if (packet->cls == UBX_CLASS_ACK) //5
       printf("ACK");
     else if (packet->cls == UBX_CLASS_CFG) //6
       printf("CFG");
     else if (packet->cls == UBX_CLASS_MON) //0x0A
       printf("MON");
     else
     {
       printf("0x");
       printf("%X",packet->cls);
     }

     printf(" ID:");
     if (packet->cls == UBX_CLASS_NAV && packet->id == UBX_NAV_PVT)
       printf("PVT");
     else if (packet->cls == UBX_CLASS_CFG && packet->id == UBX_CFG_RATE)
       printf("RATE");
     else if (packet->cls == UBX_CLASS_CFG && packet->id == UBX_CFG_CFG)
       printf("SAVE");
     else
     {
       printf("0x");
       printf("%X",packet->id);
     }

     printf(" Len: 0x");
     printf("%X",packet->len);

     // Only print the payload is ignoreThisPayload is false otherwise
     // we could be printing gibberish from beyond the end of packetBuf
     if (ignoreThisPayload == false)
     {
       printf(" Payload:");

       for (int x = 0; x < packet->len; x++)
       {
         printf(" ");
         printf("%X",packet->payload[x]);
       }
     }
     else
     {
       printf(" Payload: IGNORED");
     }
     printf(" ");
   }
 }

 //Once a packet has been received and validated, identify this packet's class/id and update internal flags
 //Note: if the user requests a PVT or a HPPOSLLH message using a custom packet, the data extraction will
 //      not work as expected beacuse extractLong etc are hardwired to packetCfg payloadCfg. Ideally
 //      extractLong etc should be updated so they receive a pointer to the packet buffer.
 void gps::processUBXpacket(ubxPacket *msg){
   switch (msg->cls)
   {
   case UBX_CLASS_NAV:
     if (msg->id == UBX_NAV_PVT && msg->len == 92)
     {
       //Parse various Byte fields into global vars
       constexpr int startingSpot = 0; //fixed value used in processUBX

       timeOfWeek = extractLong(0);
       gpsMillisecond = extractLong(0) % 1000; //Get last three digits of iTOW
       gpsYear = extractInt(4);
       gpsMonth = extractByte(6);
       gpsDay = extractByte(7);
       gpsHour = extractByte(8);
       gpsMinute = extractByte(9);
       gpsSecond = extractByte(10);
       gpsDateValid = extractByte(11) & 0x01;
       gpsTimeValid = (extractByte(11) & 0x02) >> 1;
       gpsNanosecond = extractSignedLong(16); //Includes milliseconds

       fixType = extractByte(20 - startingSpot);
       gnssFixOk = extractByte(21 - startingSpot) & 0x1; //Get the 1st bit
       diffSoln = (extractByte(21 - startingSpot) >> 1) & 0x1; //Get the 2nd bit
       carrierSolution = extractByte(21 - startingSpot) >> 6; //Get 6th&7th bits of this Byte
       headVehValid = (extractByte(21 - startingSpot) >> 5) & 0x1; // Get the 5th bit
       SIV = extractByte(23 - startingSpot);
       longitude = extractSignedLong(24 - startingSpot);
       latitude = extractSignedLong(28 - startingSpot);
       altitude = extractSignedLong(32 - startingSpot);
       altitudeMSL = extractSignedLong(36 - startingSpot);
       horizontalAccEst = extractLong(40 - startingSpot);
       verticalAccEst = extractLong(44 - startingSpot);
       nedNorthVel = extractSignedLong(48 - startingSpot);
       nedEastVel = extractSignedLong(52 - startingSpot);
       nedDownVel = extractSignedLong(56 - startingSpot);
       groundSpeed = extractSignedLong(60 - startingSpot);
       headingOfMotion = extractSignedLong(64 - startingSpot);
       speedAccEst = extractLong(68 - startingSpot);
       headingAccEst = extractLong(72 - startingSpot);
       pDOP = extractInt(76 - startingSpot);
       invalidLlh = extractByte(78 - startingSpot) & 0x1;
       headVeh = extractSignedLong(84 - startingSpot);
       magDec = extractSignedInt(88 - startingSpot);
       magAcc = extractInt(90 - startingSpot);

       //Mark all datums as fresh (not read before)
       moduleQueried.gpsiTOW = true;
       moduleQueried.gpsYear = true;
       moduleQueried.gpsMonth = true;
       moduleQueried.gpsDay = true;
       moduleQueried.gpsHour = true;
       moduleQueried.gpsMinute = true;
       moduleQueried.gpsSecond = true;
       moduleQueried.gpsDateValid = true;
       moduleQueried.gpsTimeValid = true;
       moduleQueried.gpsNanosecond = true;

       moduleQueried.all = true;
       moduleQueried.gnssFixOk = true;
       moduleQueried.diffSoln = true;
       moduleQueried.headVehValid = true;
       moduleQueried.longitude = true;
       moduleQueried.latitude = true;
       moduleQueried.altitude = true;
       moduleQueried.altitudeMSL = true;
       moduleQueried.horizontalAccEst = true;
       moduleQueried.verticalAccEst = true;
       moduleQueried.nedNorthVel = true;
       moduleQueried.nedEastVel = true;
       moduleQueried.nedDownVel = true;
       moduleQueried.SIV = true;
       moduleQueried.fixType = true;
       moduleQueried.carrierSolution = true;
       moduleQueried.groundSpeed = true;
       moduleQueried.headingOfMotion = true;
       moduleQueried.speedAccEst = true;
       moduleQueried.headingAccEst = true;
       moduleQueried.pDOP = true;
       moduleQueried.invalidLlh = true;
       moduleQueried.headVeh = true;
       moduleQueried.magDec = true;
       moduleQueried.magAcc = true;
     }
     else if (msg->id == UBX_NAV_HPPOSLLH && msg->len == 36)
     {
       timeOfWeek = extractLong(4);
       highResLongitude = extractSignedLong(8);
       highResLatitude = extractSignedLong(12);
       elipsoid = extractSignedLong(16);
       meanSeaLevel = extractSignedLong(20);
       highResLongitudeHp = extractSignedChar(24);
       highResLatitudeHp = extractSignedChar(25);
       elipsoidHp = extractSignedChar(26);
       meanSeaLevelHp = extractSignedChar(27);
       horizontalAccuracy = extractLong(28);
       verticalAccuracy = extractLong(32);

       highResModuleQueried.all = true;
       highResModuleQueried.highResLatitude = true;
       highResModuleQueried.highResLatitudeHp = true;
       highResModuleQueried.highResLongitude = true;
       highResModuleQueried.highResLongitudeHp = true;
       highResModuleQueried.elipsoid = true;
       highResModuleQueried.elipsoidHp = true;
       highResModuleQueried.meanSeaLevel = true;
       highResModuleQueried.meanSeaLevelHp = true;
       highResModuleQueried.geoidSeparation = true;
       highResModuleQueried.horizontalAccuracy = true;
       highResModuleQueried.verticalAccuracy = true;
       moduleQueried.gpsiTOW = true; // this can arrive via HPPOS too.

 /*
       if (_printDebug == true)
       {
         printf("Sec: ");
         printf(((float)extractLong(4)) / 1000.0f);
         printf(" ");
         printf("LON: ");
         printf(((float)(int32_t)extractLong(8)) / 10000000.0f);
         printf(" ");
         printf("LAT: ");
         printf(((float)(int32_t)extractLong(12)) / 10000000.0f);
         printf(" ");
         printf("ELI M: ");
         printf(((float)(int32_t)extractLong(16)) / 1000.0f);
         printf(" ");
         printf("MSL M: ");
         printf(((float)(int32_t)extractLong(20)) / 1000.0f);
         printf(" ");
         printf("LON HP: ");
         printf(extractSignedChar(24));
         printf(" ");
         printf("LAT HP: ");
         printf(extractSignedChar(25));
         printf(" ");
         printf("ELI HP: ");
         printf(extractSignedChar(26));
         printf(" ");
         printf("MSL HP: ");
         printf(extractSignedChar(27));
         printf(" ");
         printf("HA 2D M: ");
         printf(((float)(int32_t)extractLong(28)) / 10000.0f);
         printf(" ");
         printf("VERT M: ");
         printf(((float)(int32_t)extractLong(32)) / 10000.0f);
       }
 */
     }
     else if (msg->id == UBX_NAV_DOP && msg->len == 18)
     {
       geometricDOP = extractInt(4);
       positionDOP = extractInt(6);
       timeDOP = extractInt(8);
       verticalDOP = extractInt(10);
       horizontalDOP = extractInt(12);
       northingDOP = extractInt(14);
       eastingDOP = extractInt(16);
       dopModuleQueried.all = true;
       dopModuleQueried.geometricDOP = true;
       dopModuleQueried.positionDOP = true;
       dopModuleQueried.timeDOP = true;
       dopModuleQueried.verticalDOP = true;
       dopModuleQueried.horizontalDOP = true;
       dopModuleQueried.northingDOP = true;
       dopModuleQueried.eastingDOP = true;
     }
     break;
   case UBX_CLASS_HNR:
     if (msg->id == UBX_HNR_ATT && msg->len == 32)
     {
       //Parse various Byte fields into global vars
       hnrAtt.iTOW = extractLong(0);
       hnrAtt.roll = extractSignedLong(8);
       hnrAtt.pitch = extractSignedLong(12);
       hnrAtt.heading = extractSignedLong(16);
       hnrAtt.accRoll = extractLong(20);
       hnrAtt.accPitch = extractLong(24);
       hnrAtt.accHeading = extractLong(28);

       hnrAttQueried = true;
     }
     else if (msg->id == UBX_HNR_INS && msg->len == 36)
     {
       //Parse various Byte fields into global vars
       hnrVehDyn.iTOW = extractLong(8);
       hnrVehDyn.xAngRate = extractSignedLong(12);
       hnrVehDyn.yAngRate = extractSignedLong(16);
       hnrVehDyn.zAngRate = extractSignedLong(20);
       hnrVehDyn.xAccel = extractSignedLong(24);
       hnrVehDyn.yAccel = extractSignedLong(28);
       hnrVehDyn.zAccel = extractSignedLong(32);

       uint32_t bitfield0 = extractLong(0);
       hnrVehDyn.xAngRateValid = (bitfield0 & 0x00000100) > 0;
       hnrVehDyn.yAngRateValid = (bitfield0 & 0x00000200) > 0;
       hnrVehDyn.zAngRateValid = (bitfield0 & 0x00000400) > 0;
       hnrVehDyn.xAccelValid = (bitfield0 & 0x00000800) > 0;
       hnrVehDyn.yAccelValid = (bitfield0 & 0x00001000) > 0;
       hnrVehDyn.zAccelValid = (bitfield0 & 0x00002000) > 0;

       hnrDynQueried = true;
     }
     else if (msg->id == UBX_HNR_PVT && msg->len == 72)
     {
       //Parse various Byte fields into global vars
       hnrPVT.iTOW = extractLong(0);
       hnrPVT.year = extractInt(4);
       hnrPVT.month = extractByte(6);
       hnrPVT.day = extractByte(7);
       hnrPVT.hour = extractByte(8);
       hnrPVT.min = extractByte(9);
       hnrPVT.sec = extractByte(10);
       hnrPVT.nano = extractSignedLong(12);
       hnrPVT.gpsFix = extractByte(16);
       hnrPVT.lon = extractSignedLong(20);
       hnrPVT.lat = extractSignedLong(24);
       hnrPVT.height = extractSignedLong(28);
       hnrPVT.hMSL = extractSignedLong(32);
       hnrPVT.gSpeed = extractSignedLong(36);
       hnrPVT.speed = extractSignedLong(40);
       hnrPVT.headMot = extractSignedLong(44);
       hnrPVT.headVeh = extractSignedLong(48);
       hnrPVT.hAcc = extractLong(52);
       hnrPVT.vAcc = extractLong(56);
       hnrPVT.sAcc = extractLong(60);
       hnrPVT.headAcc = extractLong(64);

       uint8_t valid = extractByte(11);
       hnrPVT.validDate = (valid & 0x01) > 0;
       hnrPVT.validTime = (valid & 0x02) > 0;
       hnrPVT.fullyResolved = (valid & 0x04) > 0;

       uint8_t flags = extractByte(17);
       hnrPVT.gpsFixOK = (flags & 0x01) > 0;
       hnrPVT.diffSoln = (flags & 0x02) > 0;
       hnrPVT.WKNSET = (flags & 0x04) > 0;
       hnrPVT.TOWSET = (flags & 0x08) > 0;
       hnrPVT.headVehValid = (flags & 0x10) > 0;

       hnrPVTQueried = true;
     }
   }
 }

 //Given a spot in the payload array, extract four Bytes and build a long
 uint32_t gps::extractLong(uint8_t spotToStart){
   uint32_t val = 0;
   val |= (uint32_t)payloadCfg[spotToStart + 0] << 8 * 0;
   val |= (uint32_t)payloadCfg[spotToStart + 1] << 8 * 1;
   val |= (uint32_t)payloadCfg[spotToStart + 2] << 8 * 2;
   val |= (uint32_t)payloadCfg[spotToStart + 3] << 8 * 3;
   return (val);
 }

 //Just so there is no ambiguity about whether a uint32_t will cast to a int32_t correctly...
 int32_t gps::extractSignedLong(uint8_t spotToStart){
   union // Use a union to convert from uint32_t to int32_t
   {
       uint32_t unsignedLong;
       int32_t signedLong;
   } unsignedSigned;

   unsignedSigned.unsignedLong = extractLong(spotToStart);
   return (unsignedSigned.signedLong);
 }

 //Given a spot in the payload array, extract two Bytes and build an int
 uint16_t gps::extractInt(uint8_t spotToStart){
   uint16_t val = 0;
   val |= (uint16_t)payloadCfg[spotToStart + 0] << 8 * 0;
   val |= (uint16_t)payloadCfg[spotToStart + 1] << 8 * 1;
   return (val);
 }

 //Just so there is no ambiguity about whether a uint16_t will cast to a int16_t correctly...
 int16_t gps::extractSignedInt(int8_t spotToStart){
   union // Use a union to convert from uint16_t to int16_t
   {
       uint16_t unsignedInt;
       int16_t signedInt;
   } stSignedInt;

   stSignedInt.unsignedInt = extractInt(spotToStart);
   return (stSignedInt.signedInt);
 }

 //Given a spot, extract a Byte from the payload
 uint8_t gps::extractByte(uint8_t spotToStart){
   return (payloadCfg[spotToStart]);
 }

 //Given a spot, extract a signed 8-bit value from the payload
 int8_t gps::extractSignedChar(uint8_t spotToStart){
   return ((int8_t)payloadCfg[spotToStart]);
 }

 //Given a packet and payload, send everything including CRC Bytes via I2C port
 sfe_ublox_status_e gps::sendCommand(ubxPacket *outgoingUBX, uint16_t maxWait){
   sfe_ublox_status_e retVal = SFE_UBLOX_STATUS_SUCCESS;

   calcChecksum(outgoingUBX); //Sets checksum A and B Bytes of the packet

   if (_printDebug == true)
   {
     printf("\nSending: ");
     printPacket(outgoingUBX);
   }

   if (commType == COMM_TYPE_I2C)
   {
     retVal = sendI2cCommand(outgoingUBX, maxWait);
     if (retVal != SFE_UBLOX_STATUS_SUCCESS)
     {
       if (_printDebug == true)
       {
         printf("Send I2C Command failed");
       }
       printf("retVal %X\r\n", retVal);
       return retVal;
     }
   }
   else if (commType == COMM_TYPE_SERIAL)
   {
     // sendSerialCommand(outgoingUBX);
   }

   if (maxWait > 0)
   {
     //Depending on what we just sent, either we need to look for an ACK or not
     if (outgoingUBX->cls == UBX_CLASS_CFG)
     {
       if (_printDebug == true)
       {
         printf("sendCommand: Waiting for ACK response");
       }
       retVal = waitForACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); //Wait for Ack response
     }
     else
     {
       if (_printDebug == true)
       {
         printf("sendCommand: Waiting for No ACK response");
       }
       retVal = waitForNoACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); //Wait for Ack response
     }
   }
   return retVal;
 }

 //Given a message, calc and store the two Byte "8-Bit Fletcher" checksum over the entirety of the message
 //This is called before we send a command message
 void gps::calcChecksum(ubxPacket *msg){
   msg->checksumA = 0;
   msg->checksumB = 0;

   msg->checksumA += msg->cls;
   msg->checksumB += msg->checksumA;

   msg->checksumA += msg->id;
   msg->checksumB += msg->checksumA;

   msg->checksumA += (msg->len & 0xFF);
   msg->checksumB += msg->checksumA;

   msg->checksumA += (msg->len >> 8);
   msg->checksumB += msg->checksumA;

   for (uint16_t i = 0; i < msg->len; i++)
   {
     msg->checksumA += msg->payload[i];
     msg->checksumB += msg->checksumA;
   }
 }

 //Returns false if sensor fails to respond to I2C traffic
 sfe_ublox_status_e gps::sendI2cCommand(ubxPacket *outgoingUBX, uint16_t maxWait){
   //Point at 0xFF data register

   // _i2cPort->beginTransmission((uint8_t)_gpsI2Caddress); //There is no register to write to, we just begin writing data Bytes
   // _i2cPort->write(0xFF);
   // if (_i2cPort->endTransmission(false) != 0)         //Don't release bus
   //   return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK

   char buffer[outgoingUBX->len + 6]{0};
   buffer[0] = 0xFF;


   uint8_t stat[7]{0};
   _i2cPort->start();
   stat[0] = _i2cPort->write(_gpsI2Caddress);
   stat[1] = _i2cPort->write(0xFF);
   _i2cPort->stop();

   if((stat[0]&&stat[1]) != 1){ //Don't release bus
     return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
   }

   buffer[0] = UBX_SYNCH_1;
   buffer[1] = UBX_SYNCH_2;
   buffer[2] = outgoingUBX->cls;
   buffer[3] = outgoingUBX->id;
   buffer[4] = outgoingUBX->len & 0xFF;
   buffer[5] = outgoingUBX->len >> 8;

   _i2cPort->start();
   stat[0] = _i2cPort->write(_gpsI2Caddress);
   //Write header Bytes
   // _i2cPort->beginTransmission((uint8_t)_gpsI2Caddress); //There is no register to write to, we just begin writing data Bytes
   stat[1] = _i2cPort->write(UBX_SYNCH_1);                         //μ - oh ublox, you're funny. I will call you micro-blox from now on.
   stat[2] = _i2cPort->write(UBX_SYNCH_2);                         //b
   stat[3] = _i2cPort->write(outgoingUBX->cls);
   stat[4] = _i2cPort->write(outgoingUBX->id);
   stat[5] = _i2cPort->write(outgoingUBX->len & 0xFF);     //LSB
   stat[6] = _i2cPort->write(outgoingUBX->len >> 8);       //MSB
   // if (_i2cPort->endTransmission(false) != 0)    //Do not release bus
   //   return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
   _i2cPort->stop();

   for(uint8_t i = 0; i < 7; i++){
     if (stat[i] != 1){    //Do not release bus
       return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
     }
   }

   //Write payload. Limit the sends into 32 Byte chunks
   //This code based on ublox: https://forum.u-blox.com/index.php/20528/how-to-use-i2c-to-get-the-nmea-frames
   uint16_t BytesToSend = outgoingUBX->len;

   //"The number of data Bytes must be at least 2 to properly distinguish
   //from the write access to set the address counter in random read accesses."
   uint16_t startSpot = 0;
   while (BytesToSend > 1)
   {
     uint8_t len = BytesToSend;
     if (len > i2cTransactionSize)
       len = i2cTransactionSize;
     _i2cPort->start();
     // _i2cPort->beginTransmission((uint8_t)_gpsI2Caddress);
     //_i2cPort->write(outgoingUBX->payload, len); //Write a portion of the payload to the bus

     for (uint16_t x = 0; x < len; x++){
       buffer[x] = outgoingUBX->payload[startSpot + x];
     }

       // _i2cPort->write(outgoingUBX->payload[startSpot + x]); //Write a portion of the payload to the bus

     // if (_i2cPort->endTransmission(false) != 0)    //Don't release bus
     //   return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK

     if (_i2cPort->write(_gpsI2Caddress, buffer, len) != 0){    //Don't release bus
       return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
     }
     //*outgoingUBX->payload += len; //Move the pointer forward
     startSpot += len; //Move the pointer forward
     BytesToSend -= len;
   }

   //Write checksum
   _i2cPort->start();
   // _i2cPort->beginTransmission((uint8_t)_gpsI2Caddress);
   if (BytesToSend == 1){
      buffer[0] = outgoingUBX->payload[0];
     // _i2cPort->write(outgoingUBX->payload, 1);
      buffer[1] = outgoingUBX->checksumA;
      buffer[2] = outgoingUBX->checksumB;
      if (_i2cPort->write(_gpsI2Caddress, buffer, 3) != 0){
        return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
      }
   }else{
     buffer[0] = outgoingUBX->checksumA;
     buffer[1] = outgoingUBX->checksumB;
     if (_i2cPort->write(_gpsI2Caddress, buffer, 2) != 0){
       return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
     }
   }
   _i2cPort->stop();
   // _i2cPort->write(outgoingUBX->checksumA);
   // _i2cPort->write(outgoingUBX->checksumB);

   //All done transmitting Bytes. Release bus.
   // if (_i2cPort->endTransmission() != 0)
   //   return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); //Sensor did not ACK
   return (SFE_UBLOX_STATUS_SUCCESS);
 }

 //=-=-=-=-=-=-=-= Specific commands =-=-=-=-=-=-=-==-=-=-=-=-=-=-=
 //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

 //When messages from the class CFG are sent to the receiver, the receiver will send an "acknowledge"(UBX - ACK - ACK) or a
 //"not acknowledge"(UBX-ACK-NAK) message back to the sender, depending on whether or not the message was processed correctly.
 //Some messages from other classes also use the same acknowledgement mechanism.

 //When we poll or get a setting, we will receive _both_ a config packet and an ACK
 //If the poll or get request is not valid, we will receive _only_ a NACK

 //If we are trying to get or poll a setting, then packetCfg.len will be 0 or 1 when the packetCfg is _sent_.
 //If we poll the setting for a particular port using UBX-CFG-PRT then .len will be 1 initially
 //For all other gets or polls, .len will be 0 initially
 //(It would be possible for .len to be 2 _if_ we were using UBX-CFG-MSG to poll the settings for a particular message - but we don't use that (currently))

 //If the get or poll _fails_, i.e. is NACK'd, then packetCfg.len could still be 0 or 1 after the NACK is received
 //But if the get or poll is ACK'd, then packetCfg.len will have been updated by the incoming data and will always be at least 2

 //If we are going to set the value for a setting, then packetCfg.len will be at least 3 when the packetCfg is _sent_.
 //(UBX-CFG-MSG appears to have the shortest set length of 3 Bytes)

 //We need to think carefully about how interleaved PVT packets affect things.
 //It is entirely possible that our packetCfg and packetAck were received successfully
 //but while we are still in the "if (checkUblox() == true)" loop a PVT packet is processed
 //or _starts_ to arrive (remember that Serial data can arrive very slowly).

 //Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got an ACK and a valid packetCfg (module is responding with register content)
 //Returns SFE_UBLOX_STATUS_DATA_SENT if we got an ACK and no packetCfg (no valid packetCfg needed, module absorbs new register data)
 //Returns SFE_UBLOX_STATUS_FAIL if something very bad happens (e.g. a double checksum failure)
 //Returns SFE_UBLOX_STATUS_COMMAND_NACK if the packet was not-acknowledged (NACK)
 //Returns SFE_UBLOX_STATUS_CRC_FAIL if we had a checksum failure
 //Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
 //Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an ACK and a valid packetCfg but that the packetCfg has been
 // or is currently being overwritten (remember that Serial data can arrive very slowly)
 sfe_ublox_status_e gps::waitForACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime){
   outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; //This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
   packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
   packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

   unsigned long startTime = millis();
   while (millis() - startTime < maxTime)
   {
     if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) //See if new data is available. Process Bytes as they come in.
     {
       // If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
       // and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
       // then we can be confident that the data in outgoingUBX is valid
       if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: valid data and valid ACK received after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_RECEIVED); //We received valid data and a correct ACK!
       }

       // We can be confident that the data packet (if we are going to get one) will always arrive
       // before the matching ACK. So if we sent a config packet which only produces an ACK
       // then outgoingUBX->classAndIDmatch will be NOT_DEFINED and the packetAck.classAndIDmatch will VALID.
       // We should not check outgoingUBX->valid, outgoingUBX->cls or outgoingUBX->id
       // as these may have been changed by a PVT packet.
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: no data and valid ACK after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_SENT); //We got an ACK but no data...
       }

       // If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
       // but the outgoingUBX->cls or ID no longer match then we can be confident that we had
       // valid data but it has been or is currently being overwritten by another packet (e.g. PVT).
       // If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
       // If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
       // So we cannot use outgoingUBX->valid as part of this check.
       // Note: the addition of packetBuf should make this check redundant!
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: data being OVERWRITTEN after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
       }

       // If packetAck.classAndIDmatch is VALID but both outgoingUBX->valid and outgoingUBX->classAndIDmatch
       // are NOT_VALID then we can be confident we have had a checksum failure on the data packet
       else if ((packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: CRC failed after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_CRC_FAIL); //Checksum fail
       }

       // If our packet was not-acknowledged (NACK) we do not receive a data packet - we only get the NACK.
       // So you would expect outgoingUBX->valid and outgoingUBX->classAndIDmatch to still be NOT_DEFINED
       // But if a full PVT packet arrives afterwards outgoingUBX->valid could be VALID (or just possibly NOT_VALID)
       // but outgoingUBX->cls and outgoingUBX->id would not match...
       // So I think this is telling us we need a special state for packetAck.classAndIDmatch to tell us
       // the packet was definitely NACK'd otherwise we are possibly just guessing...
       // Note: the addition of packetBuf changes the logic of this, but we'll leave the code as is for now.
       else if (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_NOTACKNOWLEDGED)
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: data was NOTACKNOWLEDGED (NACK) after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_COMMAND_NACK); //We received a NACK!
       }

       // If the outgoingUBX->classAndIDmatch is VALID but the packetAck.classAndIDmatch is NOT_VALID
       // then the ack probably had a checksum error. We will take a gamble and return DATA_RECEIVED.
       // If we were playing safe, we should return FAIL instead
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: VALID data and INVALID ACK received after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_RECEIVED); //We received valid data and an invalid ACK!
       }

       // If the outgoingUBX->classAndIDmatch is NOT_VALID and the packetAck.classAndIDmatch is NOT_VALID
       // then we return a FAIL. This must be a double checksum failure?
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: INVALID data and INVALID ACK received after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_FAIL); //We received invalid data and an invalid ACK!
       }

       // If the outgoingUBX->classAndIDmatch is VALID and the packetAck.classAndIDmatch is NOT_DEFINED
       // then the ACK has not yet been received and we should keep waiting for it
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED))
       {
         if (_printDebug == true)
         {
           printf("waitForACKResponse: valid data after ");
           printf("%lu",millis() - startTime);
           printf(" msec. Waiting for ACK.\r\n");
         }
       }

     } //checkUbloxInternal == true

     wait_us(500);
   } //while (millis() - startTime < maxTime)

   // We have timed out...
   // If the outgoingUBX->classAndIDmatch is VALID then we can take a gamble and return DATA_RECEIVED
   // even though we did not get an ACK
   if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
   {
     if (_printDebug == true)
     {
       printf("waitForACKResponse: TIMEOUT with valid data after ");
       printf("%lu",millis() - startTime);
       printf(" msec. \r\n");
     }
     return (SFE_UBLOX_STATUS_DATA_RECEIVED); //We received valid data... But no ACK!
   }

   if (_printDebug == true)
   {
     printf("waitForACKResponse: TIMEOUT after ");
     printf("%lu",millis() - startTime);
     printf(" msec.\r\n");
   }

   return (SFE_UBLOX_STATUS_TIMEOUT);
 }

 //For non-CFG queries no ACK is sent so we use this function
 //Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got a config packet full of response data that has CLS/ID match to our query packet
 //Returns SFE_UBLOX_STATUS_CRC_FAIL if we got a corrupt config packet that has CLS/ID match to our query packet
 //Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
 //Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an a valid packetCfg but that the packetCfg has been
 // or is currently being overwritten (remember that Serial data can arrive very slowly)
 sfe_ublox_status_e gps::waitForNoACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime){
   outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; //This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
   packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
   packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
   packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

   unsigned long startTime = millis();
   while (millis() - startTime < maxTime)
   {
     if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) //See if new data is available. Process Bytes as they come in.
     {

       // If outgoingUBX->classAndIDmatch is VALID
       // and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
       // then we can be confident that the data in outgoingUBX is valid
       if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
       {
         if (_printDebug == true)
         {
           printf("waitForNoACKResponse: valid data with CLS/ID match after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_RECEIVED); //We received valid data!
       }

       // If the outgoingUBX->classAndIDmatch is VALID
       // but the outgoingUBX->cls or ID no longer match then we can be confident that we had
       // valid data but it has been or is currently being overwritten by another packet (e.g. PVT).
       // If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
       // If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
       // So we cannot use outgoingUBX->valid as part of this check.
       // Note: the addition of packetBuf should make this check redundant!
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
       {
         if (_printDebug == true)
         {
           printf("waitForNoACKResponse: data being OVERWRITTEN after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
       }

       // If outgoingUBX->classAndIDmatch is NOT_DEFINED
       // and outgoingUBX->valid is VALID then this must be (e.g.) a PVT packet
       else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID))
       {
         // if (_printDebug == true)
         // {
         //   printf("waitForNoACKResponse: valid but UNWANTED data after ");
         //   printf(millis() - startTime);
         //   printf(" msec. Class: ");
         //   printf(outgoingUBX->cls);
         //   printf(" ID: ");
         //   printf(outgoingUBX->id);
         // }
       }

       // If the outgoingUBX->classAndIDmatch is NOT_VALID then we return CRC failure
       else if (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID)
       {
         if (_printDebug == true)
         {
           printf("waitForNoACKResponse: CLS/ID match but failed CRC after ");
           printf("%lu",millis() - startTime);
           printf(" msec\r\n");
         }
         return (SFE_UBLOX_STATUS_CRC_FAIL); //We received invalid data
       }
     }

     wait_us(500);
   }

   if (_printDebug == true)
   {
     printf("waitForNoACKResponse: TIMEOUT after ");
     printf("%lu",millis() - startTime);
     printf(" msec. No packet received.\r\n");
   }
   return (SFE_UBLOX_STATUS_TIMEOUT);
 }
