//#######################################################################################################
//#################################### Plugin 126: Ping IP (non-blocking) ###############################
//#######################################################################################################
// Plugin originally created by Neutrino
// Updated by Alexander Nagy ( https://bitekmindenhol.blog.hu/ )
//
// Used Library: https://github.com/bluemurder/esp8266-ping by Alessio Leoncini
//

#define PLUGIN_126
#define PLUGIN_ID_126         126
#define PLUGIN_NAME_126       "Network - Ping IP Device"
#define PLUGIN_VALUENAME1_126 "Ping"

#include <Pinger.h>         // ESP8266-ping

#define P126_MaxInstances 3 // maximal allowed number of P126 devices
Pinger p126_pinger[P126_MaxInstances];

boolean Plugin_126_init[P126_MaxInstances];

boolean Plugin_126(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_126;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_126);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_126));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormNumericBox(F("IP 1st octet"), F("plugin_126_1"), Settings.TaskDevicePluginConfig[event->TaskIndex][1], 1, 254);
        addFormNumericBox(F("IP 2nd octet"), F("plugin_126_2"), Settings.TaskDevicePluginConfig[event->TaskIndex][2], 0, 255);
        addFormNumericBox(F("IP 3rd octet"), F("plugin_126_3"), Settings.TaskDevicePluginConfig[event->TaskIndex][3], 0, 255);
        addFormNumericBox(F("IP 4th octet"), F("plugin_126_4"), Settings.TaskDevicePluginConfig[event->TaskIndex][4], 1, 254);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        LoadTaskSettings(event->TaskIndex);
        byte baseaddr = 0;
        for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
        {
          if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_126) {
            baseaddr = baseaddr + 1;
          }
        }
        success = false;
        if (baseaddr < P126_MaxInstances) {
          Settings.TaskDevicePluginConfig[event->TaskIndex][0] = baseaddr;
          Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_126_1"));
          Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_126_2"));
          Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F("plugin_126_3"));
          Settings.TaskDevicePluginConfig[event->TaskIndex][4] = getFormItemInt(F("plugin_126_4"));
          success = true;
        } else {
          Settings.TaskDeviceEnabled[event->TaskIndex] = false;
          String log = F("Maximum number of Ping devices reached! ");
          log += String(baseaddr);
          addLog(LOG_LEVEL_INFO, log);
        }
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        byte baseaddr = 0;
        for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
        {
          if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_126) {
            baseaddr = baseaddr + 1;
          }
        }
        success = false;
        if (baseaddr < P126_MaxInstances) {
          Settings.TaskDevicePluginConfig[event->TaskIndex][0] = baseaddr;
          success = true;
          Plugin_126_init[baseaddr] = true;
        } else {
          Settings.TaskDeviceEnabled[event->TaskIndex] = false;
          String log = F("Maximum number of Ping devices reached! ");
          log += String(baseaddr);
          addLog(LOG_LEVEL_INFO, log);
        }
        break;
      }

    case PLUGIN_READ:
      {
        success = false;
        bool ret = false;
        LoadTaskSettings(event->TaskIndex);
        byte cpy = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        if (cpy < P126_MaxInstances) {
          if (Plugin_126_init[cpy])
          {
            if ((Settings.TaskDevicePluginConfig[event->TaskIndex][1] > 0) and (Settings.TaskDevicePluginConfig[event->TaskIndex][4] > 0)) {
              IPAddress ip( Settings.TaskDevicePluginConfig[event->TaskIndex][1], Settings.TaskDevicePluginConfig[event->TaskIndex][2], Settings.TaskDevicePluginConfig[event->TaskIndex][3], Settings.TaskDevicePluginConfig[event->TaskIndex][4] ); // The remote ip to ping
              String log = F("Ping ");
              ret = p126_pinger[cpy].Ping(ip, 3, 100); // 3 probe, 100millisec timeout
              p126_pinger[cpy].OnEnd([](const PingerResponse & response)
              {
                for (byte TaskIndex = 0; TaskIndex < TASKS_MAX; TaskIndex++)
                {
                  if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_126) {
                    if (
                      (response.DestIPAddress[0] == Settings.TaskDevicePluginConfig[TaskIndex][1]) and
                      (response.DestIPAddress[1] == Settings.TaskDevicePluginConfig[TaskIndex][2]) and
                      (response.DestIPAddress[2] == Settings.TaskDevicePluginConfig[TaskIndex][3]) and
                      (response.DestIPAddress[3] == Settings.TaskDevicePluginConfig[TaskIndex][4])) {
                      UserVar[(TaskIndex * VARS_PER_TASK)] = (response.TotalReceivedResponses > 0);
                      break;
                    }

                  }
                }

                String logs = F("Ping result for:");
                logs += response.DestIPAddress.toString().c_str();
                logs += F(" ");
                logs += String(response.TotalReceivedResponses);
                addLog(LOG_LEVEL_DEBUG, logs);
                return true;
              });
              success = true;
            }
          }
        }
        break;
      }

  }
  return success;
}
