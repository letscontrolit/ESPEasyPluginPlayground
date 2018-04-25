#ifdef USES_P133
//#######################################################################################################
//########################### Plugin 133 VL53L0X I2C Ranging LIDAR      #################################
//#######################################################################################################
//###################################### stefan@clumsy.ch      ##########################################
//#######################################################################################################


// needs VL53L0X library from pololu https://github.com/pololu/vl53l0x-arduino



#define PLUGIN_133
#define PLUGIN_ID_133        133
#define PLUGIN_NAME_133       "Distance - VL53L0X [TESTING]"
#define PLUGIN_VALUENAME1_133 "Distance"


#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

////////////////////////////
// VL53L0X Command Codes //
////////////////////////////

boolean Plugin_133_init[2] = {false, false};

boolean Plugin_133(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_133;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_133);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_133));
        break;
      }
    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int optionValues[2] = { 0x29, 0x30 };
        addFormSelectorI2C(F("plugin_133_vl53l0x_i2c"), 2, optionValues, choice);
        addFormNote(F("SDO Low=0x29, High=0x30"));

        unsigned int choiceMode2 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String optionsMode2[3];
        optionsMode2[0] = F("VL53L0X_NORMAL_TIMING");
        optionsMode2[1] = F("VL53L0X_FAST_TIMING");
        optionsMode2[2] = F("VL53L0X_ACCURATE_TIMING");
        int optionValuesMode2[3];
        optionValuesMode2[0] = 80;
        optionValuesMode2[1] = 20;
        optionValuesMode2[2] = 320;
        addFormSelector(F("Timing"), F("plugin_133_vl53l0x_timing"), 3, optionsMode2, optionValuesMode2, choiceMode2);

        boolean choiceMode3 = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        String optionsMode3[2];
        optionsMode3[0] = F("VL53L0X_NORMAL_RANGE");
        optionsMode3[1] = F("VL53L0X_LONG_RANGE");
        int optionValuesMode3[2];
        optionValuesMode3[0] = 0;
        optionValuesMode3[1] = 1;
        addFormSelector(F("Range"), F("plugin_133_vl53l0x_range"), 2, optionsMode3, optionValuesMode3, choiceMode3);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_133_vl53l0x_i2c"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_133_vl53l0x_timing"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_133_vl53l0x_range"));

        Plugin_133_init[Settings.TaskDevicePluginConfig[event->TaskIndex][0]] = false;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {

        int idx = Settings.TaskDevicePluginConfig[event->TaskIndex][0] ;
//        Plugin_133_init[idx] &= Plugin_133_check(Settings.TaskDevicePluginConfig[event->TaskIndex][0]); // Check id device is present

//        IT = Settings.TaskDevicePluginConfig[event->TaskIndex][1]; // set Integration Time

        if (!Plugin_133_init[idx])
        {
          Plugin_133_init[idx] = Plugin_133_begin(Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePluginConfig[event->TaskIndex][1], Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        }


        String log = F("VL53L0X  : idx: 0x");
        log += String(idx, HEX);
        addLog(LOG_LEVEL_DEBUG, log);
        log = F("VL53L0X  : plugin(idx): 0x");
        log += String(Plugin_133_init[idx], HEX);
        addLog(LOG_LEVEL_DEBUG, log);

        if (Plugin_133_init[idx])
        {
          success = true;
          long dist = sensor.readRangeSingleMillimeters();
          if (sensor.timeoutOccurred()) {
            addLog(LOG_LEVEL_DEBUG, "VL53L0X: TIMEOUT");
            success = false;
          } else if ( dist >= 8190 ) {
            addLog(LOG_LEVEL_DEBUG, "VL53L0X: NO MEASUREMENT");
            success = false;
          } else {
            UserVar[event->BaseVarIndex] = dist;
          }
          
          addLog(LOG_LEVEL_DEBUG, log);
          log = F("VL53L0X: Address: 0x");
          log += String(Settings.TaskDevicePluginConfig[event->TaskIndex][0], HEX);
          log += F(" / Timing: ");
          log += String(Settings.TaskDevicePluginConfig[event->TaskIndex][1], DEC);
          log += F(" / Long Range: ");
          log += String(Settings.TaskDevicePluginConfig[event->TaskIndex][2], BIN);
          log += F(" / Distance: ");
          log += UserVar[event->BaseVarIndex];
          addLog(LOG_LEVEL_INFO, log);

        }
        break;
      }

  }
  return success;
}

//**************************************************************************/
// Check VL53L0X presence
//**************************************************************************/
/*bool Plugin_133_check(int a) {
  vl53l0x_i2caddr = a ? a : 0x29;
  bool wire_status = false;
  uint16_t deviceID = Plugin_133_getVL53L0XID(a);

  String log = F("VL53L0X  : ID: 0x");
  log += String(deviceID, HEX);
  addLog(LOG_LEVEL_DEBUG, log);

  if (deviceID != 0x29) {
    return false;
  } else {
    return true;
  }
}
*/

//**************************************************************************/
// Initialize VL53L0X
//**************************************************************************/
bool Plugin_133_begin(int a, int timing, boolean range) {
/*  
    if (! Plugin_133_check(a))
    return false;
*/

  sensor.init();
  sensor.setTimeout(500);

  if ( range ) {
    // lower the return signal rate limit (default is 0.25 MCPS)
    sensor.setSignalRateLimit(0.1);
    // increase laser pulse periods (defaults are 14 and 10 PCLKs)
    sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
    sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  }

  sensor.setMeasurementTimingBudget(timing * 1000);

  delay(timing + 50);

  return true;
}


#endif
