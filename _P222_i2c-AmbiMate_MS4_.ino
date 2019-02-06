#ifdef USES_P222
//#######################################################################################################
//#################### Plugin 222 TE Connectivity AmbiMate MS4 multi sensor (T/H/L/M/CO2)  ##############
//#######################################################################################################
//
// AmbiMate multi-sensor
// like this one: https://www.te.com/global-en/product-2316851-1.html
//


#define PLUGIN_222
#define PLUGIN_ID_222        222
#define PLUGIN_NAME_222       "Environment - TE AmbiMate MS4 [TESTING]"
#define PLUGIN_VALUENAME1_222 "Temperature"
#define PLUGIN_VALUENAME2_222 "Humidity"
#define PLUGIN_VALUENAME3_222 "Light"
#define PLUGIN_VALUENAME4_222 "Motion"
//#define PLUGIN_VALUENAME5_222 "Power"
//#define PLUGIN_VALUENAME6_222 "CO2"
//#define PLUGIN_VALUENAME7_222 "VOC"
//#define PLUGIN_VALUENAME8_222 "Audio"

//Default I2C Address of the sensor
#define AMIBMATESENSOR_DEFAULT_ADDR 0x2A

//Multi Sensor Register Addresses
#define AMBIMATESENSOR_GET_STATUS          0x00 // (r/w) 1 byte
#define AMBIMATESENSOR_GET_TEMPERATURE 	   0x01 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_HUMIDITY        0x03 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_LIGHT 		       0x05 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_AUDIO  	       0x07 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_POWER 		       0x09 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_CO2    	       0x0B // (r) 	2 bytes
#define AMBIMATESENSOR_GET VOC 			       0x0D // (r) 	2 bytes
#define AMBIMATESENSOR_GET_VERSION 		     0x80 // (r) 	1 bytes
#define AMBIMATESENSOR_GET_SUBVERSION      0x81 // (r)  1 bytes
#define AMBIMATESENSOR_GET_OPT_SENSORS     0x82 // (r)	1 bytes
#define AMBIMATESENSOR_SET_SCAN_START_BYTE 0xC0 // (w)  1 bytes
#define AMBIMATESENSOR_SET_AUDIO_EVENT_LVL 0xC1 // (r/w) 1 bytes
#define AMBIMATESENSOR_SET_RESET_BYTE      0xF0 // (w) 1 byte (0xA5 initiates processor reset, all others ignored)

//Options to include/exclude VOC sensor
#define AMBIMATESENSOR_READ_EXCLUDE_GAS    0x3F //
#define AMBIMATESENSOR_READ_INCLUDE_GAS    0x7F //
#define AMBIMATESENSOR_READ_SENSORS        0x00 //



uint8_t _i2caddrP222;

boolean Plugin_222(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_222;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4; //8;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_222);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_222));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_222));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_222));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_222));
        //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_222));
        //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[5], PSTR(PLUGIN_VALUENAME6_222));
        //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[6], PSTR(PLUGIN_VALUENAME7_222));
        //strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[7], PSTR(PLUGIN_VALUENAME8_222));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

        addFormSeparator(2);

        addFormCheckBox(F("Use Celsius"), F("p222_Celsius"), PCONFIG(0));

        addFormSeparator(2);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

        PCONFIG(0) = isFormItemChecked(F("p222_Celsius"));

        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        _i2caddrP222 = AMIBMATESENSOR_DEFAULT_ADDR;

        bool good;

        uint8_t opt_sensors;

        unsigned char buf[20];

        // Read Optional Sensors byte
        opt_sensors = I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_OPT_SENSORS, &good);

        delayBackground(1000);

        if (opt_sensors & 0x01) //Gas Sensor installed
        {
          // Read All Sensors next byte
          good = I2C_write8_reg(_i2caddrP222, AMBIMATESENSOR_SET_SCAN_START_BYTE, AMBIMATESENSOR_READ_INCLUDE_GAS);
        }
        else
        {
          good = I2C_write8_reg(_i2caddrP222, AMBIMATESENSOR_SET_SCAN_START_BYTE, AMBIMATESENSOR_READ_EXCLUDE_GAS);
        }

        // 100ms delay
        delayBackground(100);

        // Read Sensors next byte
        good = I2C_write8(_i2caddrP222, AMBIMATESENSOR_READ_SENSORS);

        Wire.requestFrom(0x2A, 15);   // request bytes from slave device

        // Acquire the Raw Data
        unsigned int i = 0;
        while (Wire.available()) { // slave may send less than requested
          buf[i] = Wire.read(); // receive a byte as character
          i++;
        }

        // convert the raw data to engineering units
        unsigned int status = buf[0];
        float temperatureC = (buf[1] * 256.0 + buf[2]) / 10.0;
        float temperatureF = ((temperatureC * 9.0) / 5.0) + 32.0;
        float Humidity = (buf[3] * 256.0 + buf[4]) / 10.0;
        unsigned int light = (buf[5] * 256.0 + buf[6]);
        unsigned int audio = (buf[7] * 256.0 + buf[8]);
        float batVolts = ((buf[9] * 256.0 + buf[10]) / 1024.0) * (3.3 / 0.330);
        unsigned int co2_ppm = (buf[11] * 256.0 + buf[12]);
        unsigned int voc_ppm = (buf[13] * 256.0 + buf[14]);

        if(PCONFIG(0))
        {
          UserVar[event->BaseVarIndex] = (float)temperatureC;
        }
        else{
          UserVar[event->BaseVarIndex] = (float)temperatureF;
        }
        UserVar[event->BaseVarIndex + 1] = (float)Humidity;
        UserVar[event->BaseVarIndex + 2] = (float)light;

        if (status & 0x01){
          UserVar[event->BaseVarIndex + 3] = 1;}
        else{
          UserVar[event->BaseVarIndex + 3] = 0;
          }

        //UserVar[event->BaseVarIndex + 4] = (float)batVolts;

        //UserVar[event->BaseVarIndex + 5] = (float)co2_ppm;
        //UserVar[event->BaseVarIndex + 6] = (float)voc_ppm;
        //UserVar[event->BaseVarIndex + 7] = (float)audio;


        String log = F("AmbiMate: Address: 0x");
        log += String(_i2caddrP222,HEX);
        // if (PCONFIG(2)) {
        //   log += F(" Version: 0x");
        //   log += String(sensorVersion,HEX);
        // }
        addLog(LOG_LEVEL_INFO, log);
        log = F("AmbiMate: Temperature: ");
        log += temperatureC;
        addLog(LOG_LEVEL_INFO, log);
        log = F("AmbiMate: Humidity: ");
        log += Humidity;
        addLog(LOG_LEVEL_INFO, log);
        log = F("AmbiMate: Light: ");
        log += light;
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;

        }
      }

  return success;
}


//**************************************************************************/
// Read temperature
//**************************************************************************/
// float Plugin_222_readTemperature()
// {
//   return I2C_readS16_reg(_i2caddrP222, AMBIMATESENSOR_GET_TEMPERATURE);
// }

//**************************************************************************/
// Read light
//**************************************************************************/
// float Plugin_222_readLight() {
//   return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_LIGHT);
// }

//**************************************************************************/
// Read moisture
//**************************************************************************/
// unsigned int Plugin_222_readHumidity() {
//   return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_HUMIDITY);
// }

// Read Sensor Version
// uint8_t Plugin_222_getVersion() {
//   return I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_VERSION);
// }


#endif // USES_P222
