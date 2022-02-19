#include "_Plugin_Helper.h"
#ifdef USES_P200

// #######################################################################################################
// ####################### Plugin 200 VE.Direct ##########################################################
// ########################## by timokovanen #############################################################
// #######################################################################################################
// Based TD-er P087 Serial Proxy
//
// Output (string):
//  - JSON (checksum validation)
//  - CSV (checksum validation)
//  - RAW BASE64 (checksum included, no validation)

#include "src/PluginStructs/P200_data_struct.h"

#include <Regexp.h>

#define PLUGIN_200
#define PLUGIN_ID_200           200
#define PLUGIN_NAME_200         "Communication - VE.Direct [TESTING]"
#define PLUGIN_VALUENAME1_200   "vedirect"

#define P200_BAUDRATE           19200

boolean Plugin_200(uint8_t function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number           = PLUGIN_ID_200;
      Device[deviceCount].Type               = DEVICE_TYPE_SERIAL;
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_STRING;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = false;
      Device[deviceCount].ValueCount         = 1;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = false;
      Device[deviceCount].GlobalSyncOption   = false;
      Device[deviceCount].ExitTaskBeforeSave = false;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_200);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES: {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_200));
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES: {
      serialHelper_getGpioNames(event, false, true); // TX optional
      break;
    }

    case PLUGIN_WEBFORM_SHOW_VALUES:
    {
      P200_data_struct *P200_data =
        static_cast<P200_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr != P200_data) && P200_data->isInitialized()) {
        uint32_t success, error, length_last;
        P200_data->getSentencesReceived(success, error, length_last);
        uint8_t varNr = VARS_PER_TASK;
        pluginWebformShowValue(event->TaskIndex, varNr++, F("Success"),     String(success));
        pluginWebformShowValue(event->TaskIndex, varNr++, F("Error"),       String(error));
        pluginWebformShowValue(event->TaskIndex, varNr++, F("Length Last"), String(length_last), true);

        success = true;
      }
      break;
    }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
    {
      string += serialHelper_getSerialTypeLabel(event);
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_LOAD: {
      addFormSubHeader(F("Output"));

      const __FlashStringHelper * options[3] = { F("JSON"),  F("CSV"),  F("RAW (BASE64 encoded, no checksum validation)") };
      int optionValues[3]   = { P200_OUTPUT_JSON, P200_OUTPUT_CSV, P200_OUTPUT_RAW };
      addFormSelector(F("Format"), F("p200_output"), 3, options, optionValues, PCONFIG(0));

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      PCONFIG(0) = getFormItemInt(F("p200_output"));
      success = true;
      break;
    }

    case PLUGIN_INIT: {
      const int16_t serial_rx = CONFIG_PIN1;
      const int16_t serial_tx = CONFIG_PIN2;
      const ESPEasySerialPort port = static_cast<ESPEasySerialPort>(CONFIG_PORT);
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P200_data_struct());
      P200_data_struct *P200_data =
        static_cast<P200_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P200_data) {
        return success;
      }

      P200_data->output_type = PCONFIG(0);

      if (P200_data->init(port, serial_rx, serial_tx, P200_BAUDRATE)) {
        success = true;
        serialHelper_log_GpioDescription(port, serial_rx, serial_tx);
      } else {
        clearPluginTaskData(event->TaskIndex);
      }
      break;
    }

    case PLUGIN_FIFTY_PER_SECOND: {
      if (Settings.TaskDeviceEnabled[event->TaskIndex]) {
        P200_data_struct *P200_data =
          static_cast<P200_data_struct *>(getPluginTaskData(event->TaskIndex));

        if ((nullptr != P200_data) && P200_data->loop()) {
          Scheduler.schedule_task_device_timer(event->TaskIndex, millis() + 10);
          delay(0); // Processing a full sentence may take a while, run some
                    // background tasks.
        }
        success = true;
      }
      break;
    }

    case PLUGIN_READ: {
      P200_data_struct *P200_data =
        static_cast<P200_data_struct *>(getPluginTaskData(event->TaskIndex));
      if ((nullptr != P200_data) && P200_data->getSentence(event->String2)) {
          success = true;
      }
      break;
    }

  }
  return success;
}

#endif // USES_P200
