//#######################################################################################################
//################## Plugin 162 : MPL3115A2 I2C Altitude Barometric Pressure Temperature Sensor  ########
//#######################################################################################################
// This plugin works with ESPEasy V2

#define PLUGIN_162
#define PLUGIN_ID_162 162
#define PLUGIN_NAME_162       "Temperature & Pressure & Altitude - MPL3115A2"
#define PLUGIN_VALUENAME1_162 "Temperature"
#define PLUGIN_VALUENAME2_162 "Pressure"
#define PLUGIN_VALUENAME3_162 "Altitude"

boolean Plugin_162_init = false;
uint8_t i2cAddress162;
uint8_t osr162;
uint8_t mode162;
uint8_t configRegister162;

boolean Plugin_162(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte portValue = 0;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_162;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_BARO;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_162);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_162));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_162));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_162));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

        byte choice0 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        String options0[2];
        options0[0] = F("Pressure");
        options0[1] = F("Altitude");
        int optionValues0[2] = { 0, 1 };
        addFormSelector(string, F("Mode"), F("plugin_162_mode"), 2 , options0, optionValues0, choice0);

        byte choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

        String options1[8];
        options1[0] = F("1");
        options1[1] = F("2");
        options1[2] = F("4");
        options1[3] = F("8");
        options1[4] = F("16");
        options1[5] = F("32");
        options1[6] = F("64");
        options1[7] = F("128");
        int optionValues1[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        addFormSelector(string, F("Oversample Ratio"), F("plugin_162_osr"), 8 , options1, optionValues1, choice1);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_162_mode"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_162_osr"));

        success = true;
        Plugin_162_init = false;
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_162_init = true;
        mode162 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        osr162 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        
        if (checkDeviceID162()) addLog(LOG_LEVEL_INFO, F("MPL3115A2 : Found"));
        else addLog(LOG_LEVEL_INFO, F("MPL3115A2 : Not Found"));
        
        configRegister162 = (mode162 * 128) + (osr162 * 8);
        setup162(configRegister162);
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        float temperature = -99.99;
        float pressure = 0.0;
        float altitude = -9999.99;
        
        mode162 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        osr162 = (uint8_t)Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        configRegister162 = (mode162 * 128) + (osr162 * 8);
        
        if (startConversion162(configRegister162))
        {
          if (mode162 == 0) pressure = readPressure162(configRegister162);
          else altitude = readAltitude162(configRegister162);
          temperature = readTemperature162(configRegister162);
        }
        UserVar[event->BaseVarIndex] = temperature;
        UserVar[event->BaseVarIndex + 1] = pressure;
        UserVar[event->BaseVarIndex + 2] = altitude;        
        String log = F("MPL3115A2 : Temperature : ");
        log += UserVar[event->BaseVarIndex];
        addLog(LOG_LEVEL_INFO, log);
        if (mode162 == 0)
        {
          log = F("MPL3115A2 : Pressure : ");
          log += UserVar[event->BaseVarIndex + 1];          
        }
        else
        {
          log = F("MPL3115A2 : Altitude : ");
          log += UserVar[event->BaseVarIndex + 2];          
        }
        addLog(LOG_LEVEL_INFO, log);  
 
        success = true;
        break;
      }

  }
  return success;
}
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                    MPL3115A2 system constants                          ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
//// I2C ADDRESS/BITS
#define MPL3115A2_ADDRESS                       (0x60)    // 1100000

//// REGISTERS

#define MPL3115A2_REGISTER_STATUS               (0x00)
#define MPL3115A2_REGISTER_STATUS_TDR 0x02
#define MPL3115A2_REGISTER_STATUS_PDR 0x04
#define MPL3115A2_REGISTER_STATUS_PTDR 0x08

#define MPL3115A2_REGISTER_PRESSURE_MSB         (0x01)
#define MPL3115A2_REGISTER_PRESSURE_CSB         (0x02)
#define MPL3115A2_REGISTER_PRESSURE_LSB         (0x03)

#define MPL3115A2_REGISTER_TEMP_MSB             (0x04)
#define MPL3115A2_REGISTER_TEMP_LSB             (0x05)

#define MPL3115A2_REGISTER_DR_STATUS            (0x06)

#define MPL3115A2_OUT_P_DELTA_MSB               (0x07)
#define MPL3115A2_OUT_P_DELTA_CSB               (0x08)
#define MPL3115A2_OUT_P_DELTA_LSB               (0x09)

#define MPL3115A2_OUT_T_DELTA_MSB               (0x0A)
#define MPL3115A2_OUT_T_DELTA_LSB               (0x0B)

#define MPL3115A2_WHOAMI                        (0x0C)
#define MPL3115A2_WHOAMI_DEVICE_ID              (0xC4)
#define MPL3115A2_PT_DATA_CFG 0x13
#define MPL3115A2_PT_DATA_CFG_TDEFE 0x01
#define MPL3115A2_PT_DATA_CFG_PDEFE 0x02
#define MPL3115A2_PT_DATA_CFG_DREM 0x04

#define MPL3115A2_CTRL_REG1                     (0x26)
#define MPL3115A2_CTRL_REG1_SBYB 0x01
#define MPL3115A2_CTRL_REG1_OST 0x02
#define MPL3115A2_CTRL_REG1_RST 0x04
#define MPL3115A2_CTRL_REG1_OS1 0x00
#define MPL3115A2_CTRL_REG1_OS2 0x08
#define MPL3115A2_CTRL_REG1_OS4 0x10
#define MPL3115A2_CTRL_REG1_OS8 0x18
#define MPL3115A2_CTRL_REG1_OS16 0x20
#define MPL3115A2_CTRL_REG1_OS32 0x28
#define MPL3115A2_CTRL_REG1_OS64 0x30
#define MPL3115A2_CTRL_REG1_OS128 0x38
#define MPL3115A2_CTRL_REG1_RAW 0x40
#define MPL3115A2_CTRL_REG1_ALT 0x80
#define MPL3115A2_CTRL_REG1_BAR 0x00
#define MPL3115A2_CTRL_REG2                     (0x27)
#define MPL3115A2_CTRL_REG3                     (0x28)
#define MPL3115A2_CTRL_REG4                     (0x29)
#define MPL3115A2_CTRL_REG5                     (0x2A)

#define MPL3115A2_REGISTER_STARTCONVERSION      (0x12)
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                       Functions                                        ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Setup MPL3115A2                                                      ////
void setup162(uint8_t _data) {
  writeConfigRegister162(MPL3115A2_CTRL_REG1, _data);
  writeConfigRegister162(MPL3115A2_PT_DATA_CFG, MPL3115A2_PT_DATA_CFG_TDEFE | MPL3115A2_PT_DATA_CFG_PDEFE | MPL3115A2_PT_DATA_CFG_DREM);
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Write to the configuration register                                  ////
void writeConfigRegister162(uint8_t _registerAddress, uint8_t _data) {
  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write( _registerAddress);
  Wire.write(_data);
  Wire.endTransmission();
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read the configuration register                                      ////
uint8_t readConfigRegister162(uint8_t _registerAddress) {
  uint8_t _data;

  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write( _registerAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(MPL3115A2_ADDRESS, 1);
  if (Wire.available()) {
    _data = Wire.read();
  }
  Wire.endTransmission();
  return _data;
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   check MPL3115A2 device ID                                            ////
boolean checkDeviceID162(void){
  uint8_t idChip;

  idChip = readConfigRegister162(MPL3115A2_WHOAMI);
  if (idChip == MPL3115A2_WHOAMI_DEVICE_ID) return true;
  return false;  
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Start the temperature conversion                                     ////
boolean startConversion162(uint8_t _data) {

  uint8_t wait = 15;
  uint8_t regData;

  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(MPL3115A2_CTRL_REG1);
  Wire.write(_data | MPL3115A2_CTRL_REG1_OST);
  Wire.endTransmission();

  delay(((1 << osr162) * 4 + 2));

  while (wait > 0) {
    regData = readConfigRegister162(MPL3115A2_CTRL_REG1);
    if (!(regData & MPL3115A2_CTRL_REG1_OST)) break;
    delay(2);
    -- wait;
  }
  if (wait <= 0) return false; // timeout
  return true;
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read pressure and return float value                                 ////
float readPressure162(uint8_t _data) {
  uint8_t pressureMSB = 0;
  uint8_t pressureCSB = 0;  
  uint8_t pressureLSB = 0;
  float  pressure;

  startConversion162(_data);

  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(MPL3115A2_REGISTER_PRESSURE_MSB);
  Wire.endTransmission(false); // I2C RESTART
  Wire.requestFrom(MPL3115A2_ADDRESS, 3);
  if (3 <= Wire.available()) {
    pressureMSB = (uint8_t) Wire.read();
    pressureCSB = (uint8_t) Wire.read();
    pressureLSB = (uint8_t) Wire.read();        
  }

  pressure = (float)pressureMSB * 1024 + (float)pressureCSB * 4 + (float)pressureLSB / 64;
  pressureLSB &= 48;
  pressureLSB /= 16;
  pressure += (float)pressureLSB * 0.25;
  return (float)pressure;
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read altitude and return float value                                 ////
float readAltitude162(uint8_t _data) {
  uint8_t altitudeMSB = 0;
  uint8_t altitudeCSB = 0;  
  uint8_t altitudeLSB = 0;
  float  altitude;

  startConversion162(_data);

  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(MPL3115A2_REGISTER_PRESSURE_MSB);
  Wire.endTransmission(false); // I2C RESTART
  Wire.requestFrom(MPL3115A2_ADDRESS, 3);
  if (3 <= Wire.available()) {
    altitudeMSB = (uint8_t) Wire.read();
    altitudeCSB = (uint8_t) Wire.read();
    altitudeLSB = (uint8_t) Wire.read();        
  }
          
  altitude = (float)altitudeMSB;
  if (altitudeMSB & 0x80) altitude -= 256;  // negative altitude
  altitude *= 256;
  altitude += (float)altitudeCSB;
  altitudeLSB /= 16;
  altitude += (float)altitudeLSB * 0.0625;
  return (float)altitude;
}
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
////   Read temperature and return float value                              ////
float readTemperature162(uint8_t _data) {
  uint8_t temperatureMSB = 0;
  uint8_t temperatureLSB = 0;
  float  temperature;

  startConversion162(_data);

  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(MPL3115A2_REGISTER_TEMP_MSB);
  Wire.endTransmission(false); // I2C RESTART
  Wire.requestFrom(MPL3115A2_ADDRESS, 2);
  if (2 <= Wire.available()) {
    temperatureMSB = (uint8_t) Wire.read();
    temperatureLSB = (uint8_t) Wire.read();
  }

  temperature = (float) temperatureMSB;
  if (temperatureMSB & 0x80) temperature -= 256;  // negative temperature
  temperatureLSB /= 16;
  temperature += (float)temperatureLSB * 0.0625;
  return (float)temperature;
}

