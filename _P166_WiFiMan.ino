/*##########################################################################################
  ####################### Plugin 166: WiFi Power Management Functions ######################
  ##########################################################################################

  Features :
	- Brings end users the possibility to step into ModemSleep mode with ESP8266.
          It means Wifi go to off, reducing consumption from 80-150mA down ot 15mA,
          and processing continues, so main loop is active, and "rules" is working.
        - KillAp mode purpose is to do not expose AP's with every ESPEasy device, when master AP is go down for a while.

  List of commands :
	- modemsleep,1                     Enter into Modemsleep mode
	- modemsleep,0                     Exit from Modemsleep mode
        - settx,21                         Set WiFi output power: (0-21)
        - killap,1                         Disable AP mode usage, if AP mode started by ESPEasy, plugin shuts it down in every second
        - killap,0                         Enable AP mode usage, if AP mode started by ESPEasy, plugin let it go

  Fired event names, that can be used in rules:
      System#ModemSleep=1                Device enters into Modemsleep mode
      System#ModemSleep=0                Exit from Modemsleep mode
      System#WifiState=1                 Wifi connected
      System#WifiState=0                 Wifi disconnected

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

#define PLUGIN_166
#define PLUGIN_ID_166         166
#define PLUGIN_NAME_166       "WiFiMan"
#define PLUGIN_VALUENAME1_166 "ModemSleep"
#define PLUGIN_VALUENAME2_166 "TX"
#define PLUGIN_VALUENAME3_166 "Connected"
#define PLUGIN_VALUENAME4_166 "KillAP"
#if defined(ESP32)
 #include "esp_wifi_types.h"
 #include "esp_wifi.h"
 #define MAX_TX_POWER          127
#else
 #define MAX_TX_POWER          20.5
#endif

#ifndef WIFI_PS_MAX_MODEM
 #define WIFI_PS_MAX_MODEM WIFI_PS_MODEM
#endif

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
        Device[deviceCount].ValueCount = 4;
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
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_166));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_166));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        int choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String buttonOptions[3];
        buttonOptions[0] = F("Disable");
        buttonOptions[1] = F("Wake by LOW INT");
        buttonOptions[2] = F("Wake by HI INT");
        int buttonoptionValues[3] = { 0, GPIO_PIN_INTR_LOLEVEL, GPIO_PIN_INTR_HILEVEL };
        addFormSelector(F("Light-Sleep"), F("plugin_166_ls"), 3, buttonOptions, buttonoptionValues, choice);

        addFormNote(F("Only enable Light-Sleep if you know what you do! I warned you! Only wake by external gpio supported!"));

        addFormPinSelect(F("Wake GPIO"), F("wakegpio"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_166_ls"));
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] > 0) {
         Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("wakegpio"));
        } else {
         Settings.TaskDevicePluginConfig[event->TaskIndex][1] = -1;
        }

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        Plugin_166_ownindex = event->TaskIndex;
        Plugin_166_modemsleepstatus = 0;
        if (WiFi.status() != WL_CONNECTED)
        {
          UserVar[event->BaseVarIndex + 2] = 0;
        } else {
          UserVar[event->BaseVarIndex + 2] = 1;
        }
        UserVar[event->BaseVarIndex] = Plugin_166_modemsleepstatus;
        UserVar[event->BaseVarIndex + 1] = MAX_TX_POWER;
        UserVar[event->BaseVarIndex + 3] = 0;
        #if defined(ESP32)
         int8_t __rtx = MAX_TX_POWER;
         esp_wifi_get_max_tx_power(&__rtx);
         UserVar[event->BaseVarIndex + 1] = __rtx;
        #endif

        success = true;
        Plugin_166_init = true;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if (Plugin_166_init)
        {
          if (UserVar[event->BaseVarIndex + 3] == 1) { // stop AP mode if started
           if (WifiIsAP(WiFi.getMode())) {
            setAP(false); // setWifiState(WifiDisableAP); // WifiAPMode(false);
           }
          }
          byte _wstate = 1;
          if (WiFi.status() != WL_CONNECTED)
          {
            _wstate = 0;
          }
          if (_wstate != UserVar[event->BaseVarIndex + 2]) { // changed connection status
            UserVar[event->BaseVarIndex + 2] = _wstate;
            String event = F("System#WifiState=");
            event += _wstate;
            rulesProcessing(event);
          }

        }
        success = true;
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

            setmodemsleep(_rmode,Settings.TaskDevicePluginConfig[Plugin_166_ownindex][0],Settings.TaskDevicePluginConfig[Plugin_166_ownindex][1]);
            UserVar[varIndex] = _rmode;

            String log = F("ModemSleep=");
            log += _rmode;
            addLog(LOG_LEVEL_INFO, log);

          }
          if ( command == F("settx") )
          {
            success = true;
            float _rtx = MAX_TX_POWER;
            int8_t __rtx = _rtx;

            if ((event->Par1 >= 0) && (event->Par1 <= MAX_TX_POWER)) {
              _rtx = event->Par1;
            }
            LoadTaskSettings(Plugin_166_ownindex);
            event->TaskIndex = Plugin_166_ownindex;
            byte varIndex = Plugin_166_ownindex * VARS_PER_TASK;
            event->BaseVarIndex = varIndex;

            settxpower(_rtx);
            #if defined(ESP32)
             esp_wifi_get_max_tx_power(&__rtx);
             _rtx = __rtx;
            #endif
            UserVar[(varIndex + 1)] = _rtx;

            String log = F("WiFi TX=");
            log += _rtx;
            addLog(LOG_LEVEL_INFO, log);

          }
          if ( command == F("killap") )
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

            UserVar[(varIndex + 3)] = _rmode;

            String log = F("KillAP=");
            log += _rmode;
            addLog(LOG_LEVEL_INFO, log);

          }


        }
        break;
      }

  }
  return success;
}

void wakeupcallback(void) {
 setmodemsleep(0,0,0);
}

void setmodemsleep(byte state, int lightsleepmode, int wakegpio) {
  // 1=Wifi off (sleep), 0= Wifi on to STA (wake)
  if ((state == 0) && (Plugin_166_modemsleepstatus == 1)) {
    WiFi.mode( WIFI_STA ); // set wifi to STA mode, modemsleep is not usable in AP mode
    #if defined(ESP32)
     esp_wifi_set_ps(WIFI_PS_NONE);
    #else
     WiFi.forceSleepWake(); // wake wifi from ModemSleep
    #endif
    delay( 1 );            // let cpu to deal with it
    WiFi.persistent( false ); // do not use flash memory
//    WifiConnect(2);        // use ESPEasy Wifi.ino to reconnect
    wifiSetup = false;     // restore WifiCheck()
    tryConnectWiFi(); // WiFiConnectRelaxed();
    String event = F("System#ModemSleep=0");
    rulesProcessing(event);
    Plugin_166_modemsleepstatus = 0;
  }
  if ((state == 1) && (Plugin_166_modemsleepstatus == 0)) {
    wifiSetup = true; // bypass WifiCheck() with this simple hack
    WiFi.persistent( false ); // do not use flash memory
    #ifndef ESP32
     wifi_station_disconnect();
    #endif
    WifiDisconnect(); // disconnect current connections
    delay( 1 );             // wait to finish disconnect
    WiFi.mode( WIFI_OFF ); // set wifi to off mode
    Plugin_166_modemsleepstatus = 1;
    String event = F("System#ModemSleep=1");
    rulesProcessing(event);
    #if defined(ESP32)
     esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    #else
     if (lightsleepmode>0) { // do light sleep
      wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
      wifi_fpm_open();
      wifi_enable_gpio_wakeup(GPIO_ID_PIN(wakegpio), GPIO_INT_TYPE(lightsleepmode));
      wifi_fpm_set_wakeup_cb(wakeupcallback);
      wifi_fpm_do_sleep(0xFFFFFFF);
     } else { // do modem sleep
      WiFi.forceSleepBegin(); // tell wifi to stay ModemSleep until further notice
     }
    #endif
    delay( 1 );             // let cpu to acknowledge this
  }

}

void settxpower(float dBm) { // 0-20.5
  #if defined(ESP32)
   esp_wifi_set_max_tx_power((byte)dBm);
  #else
   WiFi.setOutputPower(dBm);
  #endif
}
