#include "_Plugin_Helper.h"
#ifdef USES_P248

// #######################################################################################################
// ######################## Plugin 248 AHT I2C Temperature and Humidity Sensor  ##########################
// #######################################################################################################
// data sheet AHT10: https://wiki.liutyi.info/display/ARDUINO/AHT10
// device AHT10: http://www.aosong.com/en/products-40.html
// device and manual AHT20: http://www.aosong.com/en/products-32.html
// device and manual AHT21: http://www.aosong.com/en/products-60.html

#include "src/PluginStructs/P248_data_struct.h"

#define PLUGIN_248
#define PLUGIN_ID_248         248
#define PLUGIN_NAME_248       "Environment - AHT10/20/21 [TESTING]"
#define PLUGIN_VALUENAME1_248 "Temperature"
#define PLUGIN_VALUENAME2_248 "Humidity"


boolean Plugin_248(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_248;
      Device[deviceCount].Type               = DEVICE_TYPE_I2C;
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_TEMP_HUM;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = 2;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
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
      break;
    }

    case PLUGIN_INIT:
    {
      initPluginTaskData(event->TaskIndex,
        new (std::nothrow) P248_data_struct(PCONFIG(0), (AHTx_device_type)PCONFIG(1)));
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P248_data) {
        return success;
      }
      success = true;

      break;
    }

    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      int Plugin_28_i2c_addresses[2] = { 0x38, 0x39 };
      addFormSelectorI2C(F("i2c_addr"), 2, Plugin_28_i2c_addresses, PCONFIG(0));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      addFormNote(F("SDO Low=0x38, High=0x39"));

      const String options[] = { F("AHT10"), F("AHT20"), F("AHT21") };
      int indices[] = { AHT10_DEVICE, AHT20_DEVICE, AHT21_DEVICE };
      addFormSelector(F("Sensor model"), F("p248_ahttype"), 3, options, indices, PCONFIG(1));

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      PCONFIG(0) = getFormItemInt(F("i2c_addr"));
      PCONFIG(1) = getFormItemInt(F("p248_ahttype"));
      success    = true;
      break;
    }
    case PLUGIN_ONCE_A_SECOND:
    {
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P248_data) {
        if (P248_data->updateMeasurements(event->TaskIndex)) {
          // Update was succesfull, schedule a read.
          Scheduler.schedule_task_device_timer(event->TaskIndex, millis() + 10);
        }
      }
      break;
    }

    case PLUGIN_READ:
    {
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P248_data) {
        if (P248_data->state != AHTx_New_values) {
          success = false;
          break;
        }
        P248_data->state = AHTx_Values_read;

        UserVar[event->BaseVarIndex]     = P248_data->getTemperature();
        UserVar[event->BaseVarIndex + 1] = P248_data->getHumidity();

        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          String log;
          log.reserve(40); // Prevent re-allocation
          log  = P248_data->getDeviceName();
          log += F(" : Address: 0x");
          log += String(PCONFIG(0), HEX);
          addLog(LOG_LEVEL_INFO, log);
          log  = P248_data->getDeviceName();
          log += F(" : Temperature: ");
          log += formatUserVarNoCheck(event->TaskIndex, 0);
          addLog(LOG_LEVEL_INFO, log);

          log  = P248_data->getDeviceName();
          log += F(" : Humidity: ");
          log += formatUserVarNoCheck(event->TaskIndex, 1);
          addLog(LOG_LEVEL_INFO, log);
        }
        success = true;
      }
      break;
    }
  }
  return success;
}

#endif // USES_P248
