/*

  This plug in is written by Jochen Krapf (jk@nerd2nerd.org)
  Plugin is based upon SDS011 dust sensor PM2.5 and PM10 lib (https://github.com/ricki-z/SDS011.git) by R. Zschiegner (rz@madavi.de)

  This plugin reads the particle concentration from SDS011 Sensor
  DevicePin1 - is RX on ESP
  DevicePin2 - is TX on ESP
*/

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_145
#define PLUGIN_ID_145         145
#define PLUGIN_NAME_145       "Dust Sensor - SDS011/SDS018"
#define PLUGIN_VALUENAME1_145 "PM2.5"   // Dust <2.5µm in µg/m³
#define PLUGIN_VALUENAME2_145 "PM10"    // Dust <10µm in µg/m³
#define PLUGIN_READ_TIMEOUT   3000

boolean Plugin_145_init = false;

#include <SDS011.h>   //https://github.com/ricki-z/SDS011.git

SDS011 *Plugin_145_SDS;


boolean Plugin_145(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_145;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_DUAL;
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
        string = F(PLUGIN_NAME_145);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_145));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_145));
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_145_SDS = new SDS011();
        Plugin_145_SDS->begin(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex]);
        addLog(LOG_LEVEL_INFO, F("SDS  : Init OK "));

        //delay first read, because hardware needs to initialize on cold boot
        //otherwise we get a weird value or read error
        timerSensor[event->TaskIndex] = millis() + 15000;

        Plugin_145_init = true;
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);

        if (command == F("sdssleep"))
        {
          Plugin_145_SDS->sleep();;
          addLog(LOG_LEVEL_INFO, F("SDS  : sleep"));
          success = true;
        }
        if (command == F("sdswakeup"))
        {
          Plugin_145_SDS->wakeup();;
          addLog(LOG_LEVEL_INFO, F("SDS  : wake up"));
          success = true;
        }
        break;
      }

    case PLUGIN_READ:
      {

        if (Plugin_145_init)
        {
          float pm25, pm10;
          Plugin_145_SDS->read(&pm25,&pm10);;

          UserVar[event->BaseVarIndex + 0] = pm25;
          UserVar[event->BaseVarIndex + 1] = pm10;
          success = true;
        }
        break;
      }
  }

  return success;
}

#endif
