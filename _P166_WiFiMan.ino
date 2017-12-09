//############################# Plugin 166: WiFi Power Management Functions v1.0########################
//
//  Can be used with commands and rules
//
//#######################################################################################################

#define PLUGIN_166
#define PLUGIN_ID_166         166
#define PLUGIN_NAME_166       "WiFiMan"
#define PLUGIN_VALUENAME1_166 "ModemSleep"
#define PLUGIN_VALUENAME2_166 "TX"
#define MAX_TX_POWER          20.5

byte Plugin_166_ownindex;
byte Plugin_166_modemsleepstatus;

boolean Plugin_166_init = false;

boolean Plugin_166(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_166;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_DUAL;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }
    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_166);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_166));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_166));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        Plugin_166_ownindex = event->TaskIndex;
        if (WiFi.status() != WL_CONNECTED)
        {
          Plugin_166_modemsleepstatus = 1;
         } else {
          Plugin_166_modemsleepstatus = 0;
         }
        UserVar[event->BaseVarIndex] = Plugin_166_modemsleepstatus;
        UserVar[event->BaseVarIndex+1] = MAX_TX_POWER;

        success = true;
        Plugin_166_init = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (Plugin_166_init)
        {
          success = true;
        }
        break;
      }

    case PLUGIN_WRITE:
      {
        String log = "";
        String command = parseString(string, 1);

        if (Plugin_166_init)
        {
          if ( command == F("modemsleep") )
          {
            success = true;
            byte _rmode = 0;

            if ((event->Par1 >= 0) && (event->Par1 <= 1)) {
              _rmode = event->Par1;
            }
            LoadTaskSettings(Plugin_166_ownindex);
            event->TaskIndex = Plugin_166_ownindex;
            byte varIndex = Plugin_166_ownindex * VARS_PER_TASK;
            event->BaseVarIndex = varIndex;

            setmodemsleep(_rmode);
            UserVar[varIndex] = _rmode;

            String log = F("ModemSleep=");
            log += _rmode;
            addLog(LOG_LEVEL_INFO, log);

          }
          if ( command == F("settx") )
          {
            success = true;
            float _rtx = 20.5;

            if ((event->Par1 >= 0) && (event->Par1 <= 21)) {
              _rtx = event->Par1;
            }
            LoadTaskSettings(Plugin_166_ownindex);
            event->TaskIndex = Plugin_166_ownindex;
            byte varIndex = Plugin_166_ownindex * VARS_PER_TASK;
            event->BaseVarIndex = varIndex;

            settxpower(_rtx);
            UserVar[(varIndex + 1)] = _rtx;

            String log = F("WiFi TX=");
            log += _rtx;
            addLog(LOG_LEVEL_INFO, log);

          }

        }
        break;
      }

  }
  return success;
}

void setmodemsleep(byte state) {
  // 1=Wifi off (sleep), 0= Wifi on to STA (wake)
  if ((state == 0) && (Plugin_166_modemsleepstatus == 1)) {
    WiFi.mode( WIFI_STA ); // set wifi to STA mode, modemsleep is not usable in AP mode
    WiFi.forceSleepWake(); // wake wifi from ModemSleep
    delay( 1 );            // let cpu to deal with it
    WiFi.persistent( false ); // do not use flash memory
    WifiConnect(2);        // use ESPEasy Wifi.ino to reconnect
    wifiSetup = false;     // restore WifiCheck()
    String event = F("System#ModemSleep=0");
    rulesProcessing(event);
    Plugin_166_modemsleepstatus = 0;
  }
  if ((state == 1) && (Plugin_166_modemsleepstatus == 0)) {
    wifiSetup = true; // bypass WifiCheck() with this simple hack
    WiFi.persistent( false ); // do not use flash memory
    wifi_station_disconnect();
    WifiDisconnect(); // disconnect current connections
    delay( 1 );             // wait to finish disconnect
    WiFi.mode( WIFI_OFF ); // set wifi to off mode
    WiFi.forceSleepBegin(); // tell wifi to stay ModemSleep until further notice
    delay( 1 );             // let cpu to acknowledge this
    String event = F("System#ModemSleep=1");
    rulesProcessing(event);
    Plugin_166_modemsleepstatus = 1;
  }

}

void settxpower(float dBm) { // 0-20.5
  WiFi.setOutputPower(dBm);
}
