#ifdef USES_P129
//#######################################################################################################
//################################ Plugin-214: RC522 SPI RFID reader ####################################
//#######################################################################################################

#define PLUGIN_129
#define PLUGIN_ID_129         129
#define PLUGIN_NAME_129       "RFID - RC522 SPI"
#define PLUGIN_VALUENAME1_129 "Tag"
#define PLUGIN_129_CS         16

#include <SPI.h>
#include <MFRC522.h>

MFRC522 mfrc522;

uint8_t Plugin_129_packetbuffer[64];
uint8_t Plugin_129_command;

boolean Plugin_129(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_129;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_LONG;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_129);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_129));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
      	addFormPinSelect(string, F("Reset Pin"), F("taskdevicepin3"), Settings.TaskDevicePin3[event->TaskIndex]);
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        
        for(byte x=0; x < 3; x++)
        {
          String log = F("MFRC522: Init");
          addLog(LOG_LEVEL_INFO, log);
          if(Plugin_129_Init(Settings.TaskDevicePin1[event->TaskIndex],Settings.TaskDevicePin3[event->TaskIndex]))
            break;
          delay(1000);
        }
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        static unsigned long tempcounter = 0;
        static byte counter;
        static byte errorCount=0;

        counter++;
        if (counter == 3)
        {
          counter = 0;
          uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
          uint8_t uidLength;
          byte error = Plugin_129_readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

          if (error == 1)
          {
            errorCount++;
            String log = F("MFRC522: Read error: ");
            log += errorCount;
            addLog(LOG_LEVEL_ERROR, log);
          }
          else
            errorCount=0;

          if (errorCount > 2) // if three consecutive I2C errors, reset PN532
          {
            Plugin_129_Init(Settings.TaskDevicePin1[event->TaskIndex],Settings.TaskDevicePin3[event->TaskIndex]);
          }

          if (error == 0) {
            unsigned long key = uid[0];
            for (uint8_t i = 1; i < 4; i++) {
              key <<= 8;
              key += uid[i];
            }
            UserVar[event->BaseVarIndex] = (key & 0xFFFF);
            UserVar[event->BaseVarIndex + 1] = ((key >> 16) & 0xFFFF);
            String log = F("MFRC522: Tag: ");
            log += key;
            tempcounter++;
            log += " ";
            log += tempcounter;
            addLog(LOG_LEVEL_INFO, log);
            sendData(event);
          }
        }
        break;
      }
  }
  return success;
}


/*********************************************************************************************\
 * MFRC522 init
\*********************************************************************************************/
boolean Plugin_129_Init(int8_t csPin, int8_t resetPin)
{
  if (resetPin != -1)
  {
    String log = F("MFRC522: Reset on pin: ");
    log += resetPin;
    addLog(LOG_LEVEL_INFO, log);
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    delay(100);
    digitalWrite(resetPin, HIGH);
    pinMode(resetPin, INPUT_PULLUP);
    delay(10);
  }

    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, LOW);
    

  mfrc522.PCD_Init(csPin,resetPin);   // Init MFRC522 module

  //If you set Antenna Gain to Max it will increase reading distance
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  
  bool result = mfrc522.PCD_PerformSelfTest(); // perform the test
  
  if (result) {
    //String log = F("RC522: Found");
    // Get the MFRC522 software version
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    
    // When 0x00 or 0xFF is returned, communication probably failed
    if ((v == 0x00) || (v == 0xFF)) {
      String log=F("MFRC522: Communication failure, is the MFRC522 properly connected?");
      addLog(LOG_LEVEL_ERROR,log);
      return false;
    }
    else
    {
      String log=F("MFRC522: Software Version: ");
      if (v == 0x91)
        log+=F(" = v1.0");
      else if (v == 0x92)
        log+=F(" = v2.0");
      else
        log+=F(" (unknown),probably a chinese clone?");
            
      addLog(LOG_LEVEL_INFO, log);
    }
  }
  else
    return false;

  return true;
}




/*********************************************************************************************\
 * RC522 read tag ID
\*********************************************************************************************/
byte Plugin_129_readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength)
{
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 2;
  }
  String log = F("MFRC522: New Card Detected");
  addLog(LOG_LEVEL_INFO,log);
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 1;
  }
  
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  log =F("MFRC522: Scanned PICC's UID");
  addLog(LOG_LEVEL_INFO,log);
  for ( uint8_t i = 0; i < 4; i++) {  //
      uid[i] = mfrc522.uid.uidByte[i];
    //Serial.print(readCard[i], HEX);
  }
  *uidLength = 4;
  mfrc522.PICC_HaltA(); // Stop reading
  return 0;

}


#endif // USES_P129
