//#######################################################################################################
//############ Plugin 178 SCD30 I2C CO2, Humidity and Temperature Sensor ################################
//#######################################################################################################
// development version
// by: V0JT4
// this plugin is based on the Frogmore42 library
// written based code from https://github.com/Frogmore42/Sonoff-Tasmota/tree/development/lib/FrogmoreScd30
// Commands:
//   SCDGETABC - shows automatic calibration period in days, 0 = disable
//   SCDGETALT - shows altitude compensation configuration in meters above sea level
//   SCDGETTMP - hows temperature offset in C

#define PLUGIN_178
#define PLUGIN_ID_178         178
#define PLUGIN_NAME_178       "Gases - CO2 SCD30"
#define PLUGIN_VALUENAME1_178 "CO2"
#define PLUGIN_VALUENAME2_178 "Humidity"
#define PLUGIN_VALUENAME3_178 "Temperature"
#define PLUGIN_VALUENAME4_178 "CO2raw"

#include "FrogmoreScd30.h"
boolean Plugin_178_init = false;
FrogmoreScd30 scd30;

boolean plugin_178_begin()
{
  if (!Plugin_178_init)
  {
    //Wire.begin();   called in ESPEasy framework
    scd30.begin();
    uint16_t calibration = 0;
    scd30.getCalibrationType(&calibration);
    if (calibration)
    {
      scd30.setManualCalibration();
    }
    scd30.beginMeasuring();
    Plugin_178_init = true;
  }
  return(true);
}

boolean Plugin_178(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_178;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_178);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_178));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_178));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_178));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_178));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormNumericBox(F("Altitude"), F("plugin_178_SCD30_alt"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addUnit(F("m"));
        addFormTextBox(F("Temp offset"), F("plugin_178_SCD30_tmp"), String(PCONFIG_FLOAT(0), 2), 5);
        addUnit(F("C"));
        addHtml(F("<span style=\"color:red\">Tools->Advanced->I2C ClockStretchLimit should be set to 20000</span>"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        uint16_t alt = getFormItemInt(F("plugin_178_SCD30_alt"));
        if (alt > 2000) alt = 2000;
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = alt;
        PCONFIG_FLOAT(0) = getFormItemFloat(F("plugin_178_SCD30_tmp"));
        success = true;
        break;
      }
    case PLUGIN_INIT:
      {
        plugin_178_begin();
        scd30.setAltitudeCompensation(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        scd30.setTemperatureOffset(PCONFIG_FLOAT(0));
        break;
      }
    case PLUGIN_READ:
      {
        plugin_178_begin();
        uint16_t scd30_CO2 = 0;
        uint16_t scd30_CO2EAvg = 0;
        float scd30_Humid = 0.0;
        float scd30_Temp = 0.0;
        switch (scd30.readMeasurement(&scd30_CO2, &scd30_CO2EAvg, &scd30_Temp, &scd30_Humid))
          {
            case ERROR_SCD30_NO_ERROR:
              UserVar[event->BaseVarIndex] = scd30_CO2EAvg;
              UserVar[event->BaseVarIndex+1] = scd30_Humid;
              UserVar[event->BaseVarIndex+2] = scd30_Temp;
              UserVar[event->BaseVarIndex+3] = scd30_CO2;
              if (scd30_CO2EAvg > 5000)
              {
                addLog(LOG_LEVEL_INFO,F("SCD30: Sensor saturated! > 5000 ppm"));
              }
              break;
            case ERROR_SCD30_NO_DATA:
            case ERROR_SCD30_CRC_ERROR:
            case ERROR_SCD30_CO2_ZERO:
              break;
            default:
            {
              scd30.softReset();
            }
            break;
          }
        success = true;
        break;
      }
    case PLUGIN_WRITE:
      {
        uint16_t value = 0;
        String log = F("");
        float temp;
        if (string.equalsIgnoreCase(F("SCDGETABC")))
        {
          scd30.getCalibrationType(&value);
          log += F("ABC: ");
          log += value;
        } else if (string.equalsIgnoreCase(F("SCDGETALT")))
        {
          scd30.getAltitudeCompensation(&value);
          log += F("Altitude: ");
          log += value;
        } else if (string.equalsIgnoreCase(F("SCDGETTMP")))
        {
          scd30.getTemperatureOffset(&temp);
          log += F("Temp offset: ");
          log += String(temp,2);
        } else 
        {
          break;
        }
        SendStatus(event->Source, log);
        success = true;
        break;
      }
  }
  return success;
}
