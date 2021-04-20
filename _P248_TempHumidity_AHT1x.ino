//#######################################################################################################
//################ Plugin 248: AHT10            Temperature and Humidity Sensor (I2C) ###################
//#######################################################################################################

#ifdef USES_P248

#include "_Plugin_Helper.h"

// https://wiki.liutyi.info/display/ARDUINO/AHT10
// https://github.com/enjoyneering/AHT10/blob/master/src/AHT10.h
// https://github.com/adafruit/Adafruit_AHTX0
// https://github.com/arendst/Tasmota/blob/0650744ac27108931a070918f08173d71ddfd68d/tasmota/xsns_63_aht1x.ino

//#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_248
#define PLUGIN_ID_248         248
#define PLUGIN_NAME_248       "Environment - AHT10 [TESTING]"
#define PLUGIN_VALUENAME1_248 "Temperature"
#define PLUGIN_VALUENAME2_248 "Humidity"
#define PLUGIN_VALUENAME3_248 "AbsoluteHumidity"
#define PLUGIN_VALUENAME4_248 "DewPoint" // max 4 variables allowed


#define AHT10_ADDRESS_0X38         0x38  //chip I2C address no.1 for AHT10/AHT15/AHT20, address pin connected to GND
#define AHT10_ADDRESS_0X39         0x39  //chip I2C address no.2 for AHT10 only, address pin connected to Vcc

#define AHT10_INIT_CMD             0xE1  //initialization command for AHT10/AHT15
#define AHT20_INIT_CMD             0xBE  //initialization command for AHT20
#define AHT10_START_MEASURMENT_CMD 0xAC  //start measurment command
#define AHT10_NORMAL_CMD           0xA8  //normal cycle mode command, no info in datasheet!!!
#define AHT10_SOFT_RESET_CMD       0xBA  //soft reset command

#define AHT10_INIT_NORMAL_MODE     0x00  //enable normal mode
#define AHT10_INIT_CYCLE_MODE      0x20  //enable cycle mode
#define AHT10_INIT_CMD_MODE        0x40  //enable command mode
#define AHT10_INIT_CAL_ENABLE      0x08  //load factory calibration coeff


#define AHT10_DATA_MEASURMENT_CMD  0x33  //no info in datasheet!!! my guess it is DAC resolution, saw someone send 0x00 instead
#define AHT10_DATA_NOP             0x00  //no info in datasheet!!!


#define AHT10_MEASURMENT_DELAY     80    //at least 75 milliseconds
#define AHT10_POWER_ON_DELAY       40    //at least 20..40 milliseconds
#define AHT10_CMD_DELAY            350   //at least 300 milliseconds, no info in datasheet!!!
#define AHT10_SOFT_RESET_DELAY     20    //less than 20 milliseconds

#define AHT10_FORCE_READ_DATA      true  //force to read data
#define AHT10_USE_READ_DATA        false //force to use data from previous read
#define AHT10_ERROR                0xFF  //returns 255, if communication error is occurred

 
#define AHTX0_I2CADDR_DEFAULT 0x38   ///< AHT default i2c address
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated


#ifdef USE_AHT2x
  #define AHTX_CMD     0xB1 // Cmd for AHT2x
#else
  #define AHTX_CMD     0xE1 // Cmd for AHT1x
#endif

uint8_t AHTSetCalCmd[3]    = { AHTX_CMD, 0x08, 0x00 }; // load factory calibration coeff
uint8_t AHTSetCycleCmd[3]  = { AHTX_CMD, 0x28, 0x00 }; // enable cycle mode
uint8_t AHTMeasureCmd[3]   = { 0xAC, 0x33, 0x00 };     // start measurment command
uint8_t AHTResetCmd        =   0xBA;                   // soft reset command

uint8_t          AHT10_rawDataBuffer[6] = {AHT10_ERROR, 0, 0, 0, 0, 0};


bool AHT1XStartMeasurement(uint8_t address) {
  Wire.beginTransmission(address);
  Wire.write(AHTMeasureCmd, 3);
  if (Wire.endTransmission() != 0)
    return false;
  return true;
}

bool AHT1XReadMesurement(uint8_t address) {
  
  Wire.requestFrom(address, (uint8_t) 6);

  for(uint8_t i = 0; Wire.available() > 0; i++) {
     AHT10_rawDataBuffer[i] = Wire.read();
  }
  if (AHT10_rawDataBuffer[0] & 0x80)
    return false; //device is busy

  //humidity    = (((data[1] << 12)| (data[2] << 4) | data[3] >> 4) * AHT_HUMIDITY_CONST / KILOBYTE_CONST);
  //temperature = ((AHT_TEMPERATURE_CONST * (((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]) / KILOBYTE_CONST) - AHT_TEMPERATURE_OFFSET);

  return true;
  //return (!isnan(aht1x_sensors[aht1x_idx].temperature) && !isnan(aht1x_sensors[aht1x_idx].humidity) && (aht1x_sensors[aht1x_idx].humidity != 0));
}


/**************************************************************************/
/*
    readTemperature()
    Read temperature, °C 
    NOTE:
    - temperature range      -40°C..+80°C
    - temperature resolution 0.01°C
    - temperature accuracy   ±0.3°C
*/
/**************************************************************************/
float AHT10_getTemperature()
{
  if (AHT10_rawDataBuffer[0] == AHT10_ERROR) return AHT10_ERROR; //error handler, collision on I2C bus



  uint32_t temperature = ((uint32_t)(AHT10_rawDataBuffer[3] & 0x0F) << 16) | ((uint16_t)AHT10_rawDataBuffer[4] << 8) | AHT10_rawDataBuffer[5]; //20-bit raw temperature data

  return (float)temperature * 0.000191 - 50;
}


/**************************************************************************/
/*
    readHumidity()
    Read relative humidity, %
    NOTE:
    - prolonged exposure for 60 hours at humidity > 80% can lead to a
      temporary drift of the signal +3%. Sensor slowly returns to the
      calibrated state at normal operating conditions.
    - relative humidity range      0%..100%
    - relative humidity resolution 0.024%
    - relative humidity accuracy   ±2%
*/
/**************************************************************************/
float AHT10_getHumidity()
{
  if (AHT10_rawDataBuffer[0] == AHT10_ERROR) return AHT10_ERROR; //error handler, collision on I2C bus

  uint32_t rawData = (((uint32_t)AHT10_rawDataBuffer[1] << 16) | ((uint16_t)AHT10_rawDataBuffer[2] << 8) | (AHT10_rawDataBuffer[3])) >> 4; //20-bit raw humidity data

  float humidity = (float)rawData * 0.000095;

  if (humidity < 0)   return 0;
  if (humidity > 100) return 100;
  
  return humidity;
}





/**************************************************************************/
/*
    readStatusByte()
    Read status byte from sensor over I2C
*/
/**************************************************************************/
unsigned char AHT1XReadStatus(uint8_t address) {
  uint8_t result = 0;
  // Need for AHT20?
  //Wire.beginTransmission(aht1x_address);
  //Wire.write(0x71);
  //if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom(address, (uint8_t) 1);
  result = Wire.read();
  return result;
}



uint8_t AHT10_init(uint8_t address)
{
  Wire.beginTransmission(address);

  Wire.write(AHTResetCmd);
  Wire.endTransmission();
  delay(AHT10_SOFT_RESET_DELAY);

  Wire.beginTransmission(address);
  Wire.write(AHTSetCalCmd, 3);
  if (Wire.endTransmission() != 0)
    return false;
  delay(AHT10_MEASURMENT_DELAY);
  if(AHT1XReadStatus(address) & 0x08) // Sensor calibrated?
    return true;
  return false;
  
  //while (AHT10_getStatus() & AHTX0_STATUS_BUSY) {
  //  delay(10);
  //}
  //if (!(AHT10_getStatus() & AHTX0_STATUS_CALIBRATED)) {
  //  return false;
  //}

  //if (Wire.endTransmission(true) != 0) return AHT10_ERROR;             //safety check, make sure transmission complete

  //return 0x00;

}



//==============================================
// PLUGIN
// =============================================

boolean Plugin_248(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_248;
      Device[deviceCount].Type = DEVICE_TYPE_I2C;
      Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_TEMP_HUM;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = true;
      Device[deviceCount].ValueCount = 2;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = true;
      Device[deviceCount].GlobalSyncOption = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_248);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_248));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_248));
      //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_248));
      //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_248));
    //  strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_248));
      break;
    }

    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      byte choice = PCONFIG(0);

      int optionValues[2] = { 0x38, 0x39 };
      addFormSelectorI2C(F("i2c_addr"), 2, optionValues, choice);
      addFormNote(F("AO Low=0x38, High=0x39"));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      PCONFIG(0) = getFormItemInt(F("i2c_addr"));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      String log;
      uint8_t addr = PCONFIG(0);

      if (!AHT10_init(addr))
        log = F("AHT10: init");

      //if (!AHT10_setNormalMode(addr))         //one measurement+sleep mode
      //  log = F("AHT10: setNormalMode");
      //if (!AHT10_enableFactoryCalCoeff(addr)) //load factory calibration coeff
       // log = F("AHT10: enableFactoryCoeff");

      /*check calibration enable */
      //if (AHT10_getCalibrationBit(false) != 0x01) 
       // log = F("AHT10: getCalibrationBit");

      if (log)
        addLog(LOG_LEVEL_ERROR,log);

      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      uint8_t addr = PCONFIG(0);
      AHT1XStartMeasurement(addr);
      delay(AHT10_MEASURMENT_DELAY);
      bool ret=AHT1XReadMesurement(addr);
      if (!ret)
      {
        String err = F("AHT1x: read error: ");
        err+= ret;
        addLog(LOG_LEVEL_ERROR,err);
      }

      UserVar[event->BaseVarIndex + 0] = AHT10_getTemperature();
      UserVar[event->BaseVarIndex + 1] = AHT10_getHumidity();
      //UserVar[event->BaseVarIndex + 2] = Plugin_248_SHT2x->abshum;
      //UserVar[event->BaseVarIndex + 3] = Plugin_248_SHT2x->doVent;
      
      String log = F("AHT1x: Temperature: ");
      log += UserVar[event->BaseVarIndex + 0];
      addLog(LOG_LEVEL_INFO, log);
      log = F("AHT1x: Humidity: ");
      log += UserVar[event->BaseVarIndex + 1];
      addLog(LOG_LEVEL_INFO, log);

      /*
      log = F("AHT1x: AbsoluteHumidity: ");
      log += UserVar[event->BaseVarIndex + 2];
      addLog(LOG_LEVEL_INFO, log);

      log = F("AHT1x: doVent: ");
      log += UserVar[event->BaseVarIndex + 3];
      addLog(LOG_LEVEL_INFO, log);
      */
      success = true;
      break;
    }
  }
  return success;
}

//#endif // testing

#endif //USES_P248