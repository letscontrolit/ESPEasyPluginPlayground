//#######################################################################################################
//################ Plugin 163: DS1631/1731 High Precision Digital Thermometer I2C #######################
//#######################################################################################################

// MyMessage *msgTemp163; // Mysensors

#define PLUGIN_163
#define PLUGIN_ID_163 163
#define PLUGIN_NAME_163 "High Precision Digital Thermometer DS1631/DS1731 I2C"
#define PLUGIN_VALUENAME1_163 "Temperature"

boolean Plugin_163_init = false;
uint8_t i2cAddress163;
uint8_t resolution163;

boolean Plugin_163(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte portValue = 0;
  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_163;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_163);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_163));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice0 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        int optionValues0[8];
        optionValues0[0] = 0x48;
        optionValues0[1] = 0x49;
        optionValues0[2] = 0x4A;
        optionValues0[3] = 0x4B;
        optionValues0[4] = 0x4C;
        optionValues0[5] = 0x4D;
        optionValues0[6] = 0x4E;
        optionValues0[7] = 0x4F;
        addFormSelectorI2C(string, F("plugin_163_adr"),8 , optionValues0, choice0);
        
        byte choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        
        String options1[4];
        options1[0] = F("9");
        options1[1] = F("10");
        options1[2] = F("11");
        options1[3] = F("12");
        int optionValues1[4] = { 0,1,2,3 };
        addFormSelector(string, F("Resolution"), F("plugin_163_res"),4 , options1, optionValues1, choice1);
        addUnit(string, F("bits"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_163_adr"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_163_res"));     
           
        Plugin_163_init = false; // Force device setup next time
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        uint8_t configRegister163;
        
        Plugin_163_init = true;
        i2cAddress163 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        resolution163 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][1];  
        configRegister163 = (resolution163 * 4) + 1;
        writeConfigRegister163(i2cAddress163,configRegister163);
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        float temperature;

        temperature = readTemperature163(i2cAddress163,resolution163);       

        UserVar[event->BaseVarIndex] = temperature;
        String log = F("DS1631  : Temperature: ");
        log += UserVar[event->BaseVarIndex];
        addLog(LOG_LEVEL_INFO,log);
        success = true;
        break;
      }
  }
  return success;
}
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                    DS1631 system constants                             ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
//// I2C address selectable bits
//// A0_DS1631  1 // 0 : GND - 1 : VCC
//// A1_DS1631  1 // 0 : GND - 1 : VCC
//// A2_DS1631  1 // 0 : GND - 1 : VCC
//// ADDRESS_DS1631  (0x48 | A2_DS1631<<2 | A1_DS1631<<1 | A0_DS1631)
//// Status / configuration bits
//// ONESHOT 1   // bit 1SHOT=1 : One shot temperature conversion mode
//// POL     0   // bit POL, not used here
//// NVB     0   // bit NVB, not used here
//// TLF     0   // bit TLF, not used here
//// THF     0   // bit THF, not used here
//// R0_DS1631 0 // RESOLUTION BIT 00 :  9 bits - 01 : 10 bits
//// R1_DS1631 0 // RESOLUTION BIT 10 : 11 bits - 11 : 12 bits
//// Configuration register
//// REGISTER_CONFIG   (THF<<6 | TLF<<5 | NVB<<4 | R1_DS1631<<3 | R0_DS1631<<2 | POL<<1 | ONESHOT)

#define DONE_MASK   0x80   // Temperature conversion status DONE bit mask

//// DS1621 I2C commands
#define READ_TEMPERATURE  0xAA
#define ACCESS_CONFIG     0xAC
#define START_CONVER      0x51
#define STOP_CONVER       0x22
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                       Functions                                        ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Stop the temperature conversion (if configured in continuous         ////
////   conversion mode)                                                     ////
void stopConversion163(uint8_t _i2cAddress) {
  Wire.beginTransmission(_i2cAddress);
  Wire.write(STOP_CONVER);
  Wire.endTransmission();
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Start the temperature conversion                                     ////
void startConversion163(uint8_t _i2cAddress) {
  Wire.beginTransmission(_i2cAddress);
  Wire.write(START_CONVER);
  Wire.endTransmission();
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Write to the configuration register                                  ////
void writeConfigRegister163(uint8_t _i2cAddress, uint8_t _data) {
  stopConversion163(_i2cAddress);
  Wire.beginTransmission(_i2cAddress);
  Wire.write(ACCESS_CONFIG);
  Wire.write(_data);
  Wire.endTransmission();
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read the configuration register                                      ////
uint8_t readConfigRegister163(uint8_t _i2cAddress) {
  uint8_t _data;
  
  Wire.beginTransmission(_i2cAddress);
  Wire.write(ACCESS_CONFIG);
  Wire.endTransmission();
  Wire.requestFrom(_i2cAddress, 1);
  if(Wire.available()) {
    _data = Wire.read();
  }
  return _data;
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Test if temperature conversion ending                                ////
boolean readAvailableData163(uint8_t _i2cAddress) {
  int8_t endConversion  = 0;  
  do {
    Wire.beginTransmission(_i2cAddress);
    Wire.write(ACCESS_CONFIG);
    Wire.endTransmission(false);  // I2C RESTART
    Wire.requestFrom(_i2cAddress, 1);
    if (1 <= Wire.available()) endConversion = Wire.read() & DONE_MASK;
  } while (!endConversion);  
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read temperature and return float value                              ////
float readTemperature163(uint8_t _i2cAddress, uint8_t _resolution){
  uint8_t temperatureMSB = 0;
  uint8_t temperatureLSB = 0;
  float  temperature;
  float  coef[4] = {0.5, 0.25, 0.125, 0.0625};
  
  startConversion163(_i2cAddress);
  readAvailableData163(_i2cAddress);
  Wire.beginTransmission(_i2cAddress);
  Wire.write(READ_TEMPERATURE);
  Wire.endTransmission(false); // I2C RESTART
  Wire.requestFrom(_i2cAddress, 2);
  if (2 <= Wire.available()) {
    temperatureMSB = (uint8_t) Wire.read();
    temperatureLSB = (uint8_t) Wire.read();
  }
  stopConversion163(_i2cAddress);

  temperature = (float) temperatureMSB;
  if (temperatureMSB & 0x80) temperature -= 256;  // negative temperature
  if (_resolution == 0) temperatureLSB /= 128;
  if (_resolution == 1) temperatureLSB /= 64;
  if (_resolution == 2) temperatureLSB /= 32;
  if (_resolution == 3) temperatureLSB /= 16;
    
  temperature += (float)temperatureLSB * coef[_resolution];   
  return (float)temperature; 
}
