//---------------------------------------------------------------------------//
//                                                                           //
//                          HOMEASSISTANT DISCOVERY                          //
//                                                                           //
//---------------------------------------------------------------------------//
/* by michael baeck
  plugin searches for active mqtt-enabled taskvalues and pushes json payloads 
  for homeassistant to take for discovery.
  https://www.letscontrolit.com/forum/viewtopic.php?f=18&t=6061

  * REQUIRES P128 (Homeassistant Output Device) to be included!
  * P126 + P128 are dependend on each other (at least to compile, may be changed later)
    
  [!] MAJOR ISSUE:  by espeasy default MQTT_MAX_PACKET_SIZE in pubsubclient.h is set to 384
  [!]               most payloads will be 500-600, please increase for testing this plugin.
  [!]               section in lib\pubsubclient\src\PubSubClient.h:
                    // MQTT_MAX_PACKET_SIZE : Maximum packet size
                    #ifndef MQTT_MAX_PACKET_SIZE
                    #define MQTT_MAX_PACKET_SIZE 384
                    #endif

  1 value = 1 entity. 
  whole unit (ESP) = 1 device & 1 entity.

  classification of sensor (motion, light, temp, etc.) can be done in webform.

  devices can also be deleted but homeassistant refuses to remove them from its store.
  (it's an issue with HA, that should be fixed soon)

  this is my first arduino code and first commit ever to anything, but it's pretty useful already.
  However, it's all done learning by doing and code might be ugly...
  testing and feedback highly appreciated!


  only tested on a couple of 4M ESP12 yet, ESP01 soon, ESP32 doesn't compile. 
  storage might be short on ESP32 as struct reserves space for all tasks

  * stay tuned for more plugins like homeassistant-switch, etc

  supported components
    binary_sensor
      [X] homeassistant native  [1/0]
        [X] battery           [low/norm]
        [X] cold              [cold/norm]
        [X] connectivity      [conn/disco]
        [X] door              [open/close]
        [X] garage_door       [open/close]
        [X] gas               [gas/clear]
        [X] heat              [hot/norm]
        [X] light             [light/dark]
        [X] lock              [unlocked/locked]
        [X] moisture          [wet/dry]
        [X] motion            [motion/clear]
        [X] moving            [moving/stopped]
        [X] occupancy         [occ/clear]
        [X] opening           [open/close]
        [X] plug              [plug/unplug]
        [X] power             [power/none]
        [X] presesnce         [home/away]
        [X] problem           [problem/ok]
        [X] safety            [unsafe/safe]
        [X] smoke             [smoke/clear]
        [X] sound             [sound/clear]
        [X] vibration         [vibration/clear]
        [X] window            [open/close]
      [x] custom
        [x] water
    sensor
      [X] homeassistant native
        [x] battery
        [x] humimdity
        [x] illuminance
          [x] unit_of_measurement lx or lm
        [x] temperature
          [x] unit_of_measurement °C or °F
        [x] pressure
          [x] unit_of_measurement hPa or mbar
      [x] custom
        [x] plant
        [x] radio
        [x] scale
      [ ] camera
      [ ] cover
            damper
            garage
            window

      [ ] fan
      [ ] climate
      [ ] light
      [ ] lock
    output
      [dev] switch
        [test] simple gpio switch (included)
        [dev] complex switch (via P128)
  
  naming conventions
    entity_id               COMPONENT.SYSNAME_TASK_VALUE    (adjustable)
                            COMPONENT.char[26]_char[41]_char[41]

    last-will-topic         -> controller settings

    sensor-discovery-topic  PREFIX / COMPONENT / MAC / MAC_TASKID_VALUEID /config
    sensor-state-topic      -> controller settings

    system-discovery-topic  PREFIX / COMPONENT / MAC / MAC_system /config
    system-state-topic      PREFIX / COMPONENT / MAC / MAC_system /state

    observation-this-unit   PREFIX / + / MAC / + /config       
    observation-all-units   PREFIX / + / + / + /config

    example payload:
    {
      "name":"esp22",
      "ic":"mdi:chip",
      "stat_t":"dev/sensor/807D3A6EA0C5/807D3A6EA0C5_system/state",
      "val_tpl":"{{ value_json.state }}",
      "uniq_id":"807D3A6EA0C5_system",
      "avty_t":"esp22/state",
      "pl_avail":"online",
      "pl_not_avail":"offline",
      "json_attr":["plugin","unit","version","uptime","cpu","hostname","ip","mac","ssid","bssid","rssi","last_disconnect","last_boot_cause" ],
      "device":{
        "sw_version":"EspEasy - Mega (Nov 12 2018)",
        "manufacturer":"letscontrolit.com",
        "model":"PLATFORMIO_ESP12E",
        "connections":[["mac","80:7D:3A:6E:A0:C5"]],
        "name":"esp22",
        "identifiers":["7250117","807D3A6EA0C5"]
      }
    }

  bugs/todo
    [!] no command return (commands working though)
    [todo] simple/advanced mode
    [test] unit choice
    [test] assign settings by Pxxx

  build results [20181112]
    [!]  Environment esp-wrover-kit_test_1M8_partition   [ERROR]
    [!]  Environment esp32dev                            [ERROR]
    [!]  Environment esp32test_1M8_partition             [ERROR]
    [x]  Environment normal_ESP8266_1024                 [SUCCESS]
    [x]  Environment normal_core_241_ESP8266_1024        [SUCCESS]
    [x]  Environment normal_ESP8285_1024                 [SUCCESS]
    [x]  Environment normal_WROOM02_2048                 [SUCCESS]
    [x]  Environment normal_ESP8266_4096                 [SUCCESS]
    [x]  Environment normal_core_241_ESP8266_4096        [SUCCESS]
    [x]  Environment normal_IR_ESP8266_4096              [SUCCESS]
    [x]  Environment test_ESP8266_1024                   [SUCCESS]
    [x]  Environment test_ESP8285_1024                   [SUCCESS]
    [x]  Environment test_WROOM02_2048                   [SUCCESS]
    [x]  Environment test_ESP8266_4096                   [SUCCESS]
    [x]  Environment test_ESP8266_4096_VCC               [SUCCESS]
    [x]  Environment dev_ESP8266_1024                    [SUCCESS]
    [x]  Environment dev_ESP8285_1024                    [SUCCESS]
    [x]  Environment dev_WROOM02_2048                    [SUCCESS]
    [x]  Environment dev_ESP8266_4096                    [SUCCESS]
    [x]  Environment dev_ESP8266PUYA_1024                [SUCCESS]
    [x]  Environment dev_ESP8266PUYA_1024_VCC            [SUCCESS]
    [x]  Environment hard_SONOFF_POW                     [SUCCESS]
    [x]  Environment hard_SONOFF_POW_R2_4M               [SUCCESS]
    [x]  Environment hard_Shelly_1                       [SUCCESS]
    [x]  Environment hard_Ventus_W266                    [SUCCESS]
*/  


//#ifdef USES_P126
//#ifdef PLUGIN_BUILD_DEVELOPMENT
//#ifdef PLUGIN_BUILD_TESTING
//#define PLUGIN_119_DEBUG  true

// force using discovery plugin when using this one
#ifndef USES_P128
 #define USES_P128
#endif

#define PLUGIN_126
#define PLUGIN_ID_126     126     
#define PLUGIN_NAME_126   "Generic - Homeassistant Discovery [DEVELOPMENT]"  

//---------------------------------------------------------------------------//
// struct + global vars
  #define _P126_VERSION             53

  #define _P126_HASWITCH            128
  #define _P128_F(taskid,mode) (_P128_make_config(discovery,taskid,mode))            
  #define _P128_WEB             1
  #define _P128_COMPONENT       2
  #define _P128_ENTITYID        3
  #define _P128_PAYLOAD         4
  #define _P128_TOPIC           5
  #define _P128_COMPCOUNT       7 // 9

  #define _P126_INTERVAL            5000
  #define _P126_CLASSCOUNT          9
  #define _P126_BINCLASSCOUNT       26

  #define _P126_GENERIC         0         //  %   raw   K
  #define _P126_BATTERY         1   
  #define _P126_HUMIDITY        2
  #define _P126_LIGHT           3
  #define _P126_TEMP            4
  #define _P126_PRESSURE        5
  #define _P126_PLANT           6
  #define _P126_RADIO           7
  #define _P126_LOAD            8

  #define _P126_UNITMAP             1
  #define _P126_LIGHTUNITMAP        2
  #define _P126_TEMPUNITMAP         3
  #define _P126_LOADUNITMAP         4
  #define _P126_PRESSUNITMAP        5
  #define _P126_COLORUNITMAP        6
  #define _P126_DISTANCEUNITMAP     7
  #define _P126_PARTICLEUNITMAP     8
  #define _P126_ENERGYUNITMAP       9
  #define _P126_PERCENTUNITMAP      10

  #define _P126_UNITCOUNT           9
  #define _P126_LIGHTUNITCOUNT      4
  #define _P126_TEMPUNITCOUNT       2
  #define _P126_LOADUNITCOUNT       4
  #define _P126_PRESSUNITCOUNT      2
  #define _P126_COLORUNITCOUNT      8
  #define _P126_DISTANCEUNITCOUNT   4
  #define _P126_PARTICLEUNITCOUNT   3
  #define _P126_ENERGYUNITCOUNT     4

  #define _P126_MAXCLASS            _P126_BINCLASSCOUNT
  #define _P126_MAXUNIT             _P126_UNITCOUNT

  //#define _P126_DEVICE  // saves another char to flash shown in HA frontend for device "D1mini bedroom", etc
  //#define _P126_DEBUG

  struct DiscoveryStruct {
    // temporary
      int taskid;
      int ctrlid;
      String cmdtopic;
      String publish;
      String lwttopic;
      String lwtup;
      String lwtdown;
      int qsize;
      int pubinterval;
    // save to flash
      struct savestruct {
        byte init;
        char prefix[21];
        char custom[41];
        #ifdef _P126_DEVICE
        char device[21];
        #endif
        bool usename;
        bool usecustom;
        bool usetask;
        bool usevalue;
        bool moved;
        struct taskstruct {
          bool enable;
          struct valuestruct {
            bool enable;
            bool binary;
            byte type;
            byte unit;
            byte unitmap;
          } value[4];
        } task[TASKS_MAX];
      } save;
  };

  bool _P126_cleaning = false;
  unsigned long _P126_time;
  byte _P126_increment;
  String _P126_log;

  String _P126_class[_P126_CLASSCOUNT];
  String _P126_binclass[_P126_BINCLASSCOUNT];
  String _P126_unit[_P126_UNITCOUNT];
  String _P126_lightunit[_P126_LIGHTUNITCOUNT];
  String _P126_tempunit[_P126_TEMPUNITCOUNT];
  String _P126_loadunit[_P126_LOADUNITCOUNT];
  String _P126_pressunit[_P126_PRESSUNITCOUNT];
  String _P126_colorunit[_P126_COLORUNITCOUNT];
  String _P126_distanceunit[_P126_DISTANCEUNITCOUNT];
  String _P126_particleunit[_P126_PARTICLEUNITCOUNT];
  String _P126_energyunit[_P126_ENERGYUNITCOUNT];

//---------------------------------------------------------------------------//

boolean Plugin_126(byte function, struct EventStruct *event, String& string) {

  boolean success = false;
  String tmpnote;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number = PLUGIN_ID_126;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_NONE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;      
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = false;
        Device[deviceCount].DecimalsOnly = false;
        break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_126);
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      //---------------------------------------------------------------------------//
      // load user setting
        struct DiscoveryStruct discovery;
          discovery.ctrlid = 0;
          discovery.pubinterval = 0;
          discovery.qsize = 0;
          discovery.save.init = 0;

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

        String errnote = F("<br>");
        if (discovery.save.init != _P126_VERSION) {
        //---------------------------------------------------------------------------//
        // clean settings, if major changes in plugin to avoid corrupt config
          if (discovery.save.init == 0) {
            // set default strings, if new config (and keep if older config)
            errnote = F("No settings found.");
            strncpy(discovery.save.prefix, "homeassistant", sizeof(discovery.save.prefix));
            strncpy(discovery.save.custom, "customstring", sizeof(discovery.save.custom));
          } else {
            errnote = F("<font color=\"red\">Loaded settings with ID ");
            errnote += String(discovery.save.init);
            errnote += F(" not matching v");
            errnote += String(_P126_VERSION);
            errnote += F(".</font>");
          }
          errnote += F(" Loading defaults.<br><br>");        

          discovery.save.usecustom = false;
          discovery.save.usename = true;
          discovery.save.usetask = true;
          discovery.save.usevalue = true;
          for (byte i = 0; i < TASKS_MAX; i++) {
            discovery.save.task[i].enable = false;
            for (byte j = 0; j < VARS_PER_TASK; j++) {
              discovery.save.task[i].value[j].enable = false;
              discovery.save.task[i].value[j].binary = false;
              discovery.save.task[i].value[j].type = 0;
              discovery.save.task[i].value[j].unit = 0;
              discovery.save.task[i].value[j].unitmap = 0;
            }
          } 
        }        


      //---------------------------------------------------------------------------//
      // introduction
        html_BR();
        tmpnote += errnote;
        tmpnote += F("This plugin pushes configs that are compliant to home-assistant's discovery feature.");
        tmpnote += F("<br>It creates one additional sensor for the ESP node itself which contains system info like wifi-signal, ip-address, etc.");
        tmpnote += F(" Interval setting on bottom defines update frequency of that sensor. Set to 0 to disable.");
        addFormNote(tmpnote);
      

      //---------------------------------------------------------------------------//
      // global settings
        addFormSubHeader(F("GLOBAL SETTINGS"));
        #ifdef _P126_DEVICE
        addFormTextBox(String(F("hardware model")), String(F("Plugin_126_device")), discovery.save.device, 20);
        #endif
        addFormTextBox(String(F("discovery prefix")), String(F("Plugin_126_prefix")), discovery.save.prefix, 20);
        addFormNote(F("Change discovery topic here, if done in home-assistant. Defaults to \"homeassistant\"."));
        
        html_BR();

        addFormCheckBox(String(F("include sysname")), String(F("Plugin_126_usename")), discovery.save.usename);
        addFormCheckBox(String(F("include custom")), String(F("Plugin_126_usecustom")), discovery.save.usecustom);
        addFormCheckBox(String(F("include taskname")), String(F("Plugin_126_usetask")), discovery.save.usetask);
        //.addFormCheckBox(String(F("use value name")), String(F("Plugin_126_usevalue")), discovery.save.usevalue);
        //.if (discovery.save.usecustom) {
        addFormTextBox(String(F("custom string ")), String(F("Plugin_126_custom")), discovery.save.custom, 40);
        addFormCheckBox(String(F("move custom to end")), String(F("Plugin_126_moved")), discovery.save.moved);
        //.}

        html_BR();

        tmpnote = F("Adjust how the entity_ids in homeassistant are named.");
        tmpnote += F(" Click submit to preview your settings before publishing.");
        addFormNote(tmpnote);

        //---------------------------------------------------------------------------//
        // show example entity_id based on settings
          bool b1 = discovery.save.usename;
          String s1 = Settings.Name;
          bool b2 = discovery.save.usecustom;
          String s2 = discovery.save.custom;
          bool b3 = discovery.save.usetask;
          String s3 = F("%tskname%");
          bool b4 = true; //.discovery.save.usevalue;
          String s4 = F("%valname%");
          if (discovery.save.moved) {
            b4 = discovery.save.usecustom;
            s4 = discovery.save.custom;
            b2 = discovery.save.usetask;
            s2 = F("%tskname%");
            b3 = discovery.save.usevalue;
            s3 = F("%valname%");
          }
      
          String entity_id = F("example: ");
            entity_id += F("<b>sensor.");
          if (b1) {
            entity_id += String(s1);
            if (b2 || b3 || b4) entity_id += F("_");
          }
          if (b2) {
            entity_id += String(s2); 
            if (b3 || b4) entity_id += F("_");
          }
          if (b3) {
            entity_id += String(s3);
            if (b4) entity_id += F("_");
          }
          if (b4) {
          entity_id += String(s4);
          }
          entity_id += F("</b>");
          entity_id.toLowerCase();
          addHtml(entity_id);


      //---------------------------------------------------------------------------//
      // device settings
        int msgcount = _P126_find_sensors(&discovery);
        
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save)); // need to save unit-map here

        if (Settings.TaskDeviceTimer[event->TaskIndex] > 0) msgcount++;
    

      //---------------------------------------------------------------------------//
      // command buttons
        addFormSubHeader(F("CONTROL"));

        tmpnote =  F("<br>Configs will NOT be pushed automatically. ");
        tmpnote += F("Once plugin is saved, you can use the buttons below.<br>");
        tmpnote += F("You can trigger actions via these commands as well: ");
        tmpnote += F("\"<b>discovery,update</b>\", \"<b>discovery,delete</b>\" and \"<b>discovery,cleanup</b>\" ");
        addFormNote(tmpnote);

        //.html_BR();

        if (Settings.TaskDeviceEnabled[event->TaskIndex] && !_P126_cleaning) {
          html_BR();
          
          addButton(F("/tools?cmd=discovery%2Cupdate"), F("Update Configs"));
          addButton(F("/tools?cmd=discovery%2Cdelete"), F("Delete Configs"));
          addButton(F("/tools?cmd=discovery%2Ccleanup"), F("Full cleanup"));
          html_BR();

          int duration = TASKS_MAX * 2 * _P126_INTERVAL /1000/60;
          int payloads = TASKS_MAX * 2 * 4;
          tmpnote = F("<br><font color=\"red\">Warning!</font> Cleanup will send ");
          tmpnote += String(payloads);
          tmpnote += F(" empty payloads in about ");
          tmpnote += String(duration);
          tmpnote += F(" minutes.");
          addFormNote(tmpnote);
        }

        if (_P126_cleaning) {
          html_BR();
          int duration = (TASKS_MAX * 2 - (_P126_increment + 1)) * _P126_INTERVAL / 1000;
          String message = F("<font color=\"red\">CLEANUP IN PROGESS!</font> Remaining time: about ");
          message += String(duration);
          message += F(" seconds");
          addHtml(message);
        }


      //---------------------------------------------------------------------------//
      // notes
        addFormSubHeader(F("NOTES"));

        _P126_get_ctrl(&discovery);

        tmpnote = F("<br>- On update, <font color=\"#07D\">");
        tmpnote += String(msgcount);
        tmpnote += F("</font> messages will be published.");

        if (discovery.qsize < 2 * msgcount) {
          tmpnote += F("Your message queue is set to: <font color=\"#07D\">");
          tmpnote += String(discovery.qsize);
          tmpnote += F("</font>");
          tmpnote += F(". Consider increasing it, if not all configs reach homeassistant.");
        }

        if (discovery.lwtup.length() > 10) {
          tmpnote += F("Your LWT Connect Message is <font color=\"#07D\">");
          tmpnote += String(discovery.lwtup.length());
          tmpnote += F("</font> charackters long");
          tmpnote += F(". Consider to shorten it, to reduce payload size.");
        }

        if (discovery.lwtdown.length() > 10) {
          tmpnote += F("Your LWT Disconnect Message is <font color=\"#07D\">");
          tmpnote += String(discovery.lwtdown.length());
          tmpnote += F("</font> charackters long");
          tmpnote += F(". Consider to shorten it, to reduce payload size.");
        }

        if (!Settings.MQTTRetainFlag) {
          tmpnote += F("<br>- Retain option is not set. It's highly advised to set it in advanced settings.");
        }

        // if (discovery.pubinterval < 500) {
        //   tmpnote += F("<br>- Message interval is set to <font color=\"#07D\">");
        //   tmpnote += String(discovery.pubinterval);
        //   tmpnote += F("</font>ms. Consider increasing it, if not all configs reach homeassistant.");
        // }
        
        tmpnote += F("<br>- ESPEasy limits mqtt message size to <font color=\"#07D\">");
        tmpnote += String(MQTT_MAX_PACKET_SIZE);
        tmpnote += F("</font>, check logs, to see if this affects you.");

        tmpnote += F("<br>- Tasks must have a MQTT controller enabled to show up.");

        tmpnote += F("<br>- Don't forget to submit this form, before using commands.");

        tmpnote += F("<br>- New sensors will be added immediately to homeassistant.");
        tmpnote += F(" Changes and deletions require homeassistant to be restarted.");

        tmpnote += F("<br><br>v");
        tmpnote += _P126_VERSION;
        tmpnote += F("; struct size: ");
        tmpnote += String(sizeof(discovery));
        tmpnote += F("Byte memory / ");
        tmpnote += String(sizeof(discovery.save));
        tmpnote += F("Byte flash");

        addFormNote(tmpnote);
      //---------------------------------------------------------------------------//

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      //---------------------------------------------------------------------------//
      // init settings struct
        struct DiscoveryStruct discovery;

        #ifdef _P126_DEBUG
          Serial.println(F("--------------- WEBSAVE INIT ---------------------------"));
          Serial.println(F("Filling empty struct"));
        #endif
        discovery.save.init = _P126_VERSION;
        strncpy(discovery.save.prefix, "homeassistant", sizeof(discovery.save.prefix));
        discovery.save.usecustom = false;
        discovery.save.usename = true;
        discovery.save.usetask = true;
        discovery.save.usevalue = true;
        discovery.save.moved = false;
        for (byte i = 0; i < TASKS_MAX; i++) {
          discovery.save.task[i].enable = false;
          for (byte j = 0; j < VARS_PER_TASK; j++) {
            discovery.save.task[i].value[j].enable = false;
            discovery.save.task[i].value[j].binary = false;
            discovery.save.task[i].value[j].type = 0;
            discovery.save.task[i].value[j].unit = 0;
              discovery.save.task[i].value[j].unitmap = 0;
          }
        } 


      //---------------------------------------------------------------------------//
      // copy settings from webserver to settings struct
        #ifdef _P126_DEVICE
        strncpy(discovery.save.device, WebServer.arg(F("Plugin_126_device")).c_str(), sizeof(discovery.save.device));
        #endif
        strncpy(discovery.save.prefix, WebServer.arg(F("Plugin_126_prefix")).c_str(), sizeof(discovery.save.prefix));
          #ifdef _P126_DEBUG
            Serial.print(F("prefix from form: "));
            Serial.println(WebServer.arg(F("Plugin_126_prefix")).c_str());
            Serial.print(F("prefix saved to struct: "));
            Serial.println(String(discovery.save.prefix));
          #endif

        strncpy(discovery.save.custom, WebServer.arg(F("Plugin_126_custom")).c_str(), sizeof(discovery.save.custom));
          #ifdef _P126_DEBUG
            Serial.print(F("custom from form: "));
            Serial.println(WebServer.arg(F("Plugin_126_custom")).c_str());
            Serial.print(F("custom saved to struct: "));
            Serial.println(String(discovery.save.custom));
          #endif

        if (WebServer.arg(F("Plugin_126_custom")).length() == 0) {
          discovery.save.usecustom = false;  
        } else {
        discovery.save.usecustom = isFormItemChecked(F("Plugin_126_usecustom"));
        }

        discovery.save.usename = isFormItemChecked(F("Plugin_126_usename"));
        discovery.save.usetask = isFormItemChecked(F("Plugin_126_usetask"));
        discovery.save.usevalue = true; //isFormItemChecked(F("Plugin_126_usevalue"));
        discovery.save.moved = isFormItemChecked(F("Plugin_126_moved"));
          #ifdef _P126_DEBUG
            Serial.print(F("usename saved to struct: "));
            Serial.println(toString(discovery.save.usename));
            Serial.print(F("usecustom saved to struct: "));
            Serial.println(toString(discovery.save.usecustom));
            Serial.print(F("usetask saved to struct: "));
            Serial.println(toString(discovery.save.usetask));
            Serial.print(F("usevalue saved to struct: "));
            Serial.println(toString(discovery.save.usevalue));
          #endif

        _P126_find_sensors(&discovery, true);


      //---------------------------------------------------------------------------//
      // save user settings to flash
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));
          #ifdef _P126_DEBUG
            Serial.println(F("saved discovery to flash "));
            Serial.println(F("-----------------------------------------------------"));
          #endif
      //---------------------------------------------------------------------------//

      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      //---------------------------------------------------------------------------//
      // load settings
        struct DiscoveryStruct discovery;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));
      
      //---------------------------------------------------------------------------//
      // valid check
        if (discovery.save.init != _P126_VERSION) {
          addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
          success = false;
        } else {
      //---------------------------------------------------------------------------//    
      // publish system state
          _P126_get_ctrl(&discovery);
          success = _P126_system_state(&discovery, false);
        }
      //---------------------------------------------------------------------------//

      break;
    }

    case PLUGIN_WRITE:
    {
      String tmpString  = string;
      int argIndex = tmpString.indexOf(',');

      if (argIndex)
        tmpString = tmpString.substring(0, argIndex);

      if (tmpString.equalsIgnoreCase(F("discovery"))) {
        success = true; 

        _P126_log = F("P[126] command issued : ");
        _P126_log += string;
        addLog(LOG_LEVEL_INFO,_P126_log);

        if (_P126_cleaning) {
          addLog(LOG_LEVEL_ERROR, F("[P126] cleanup in progress, ignoring command"));
        } else {
          //---------------------------------------------------------------------------//
          // load settings
            struct DiscoveryStruct discovery;
            _P126_get_id(&discovery);   // search for taskid (not given with event)
            LoadCustomTaskSettings(discovery.taskid, (byte*)&discovery.save, sizeof(discovery.save));

            if (discovery.save.init != _P126_VERSION) {
              addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
              break;
            } 

            _P126_get_ctrl(&discovery);
          //---------------------------------------------------------------------------//

          argIndex = string.lastIndexOf(',');
          tmpString = string.substring(argIndex + 1);

          if (tmpString.equalsIgnoreCase(F("update"))) {

            _P126_system_config(&discovery, false);
            _P126_sensor_config(&discovery, false);
            _P126_system_state(&discovery, false);
          } 
          else if (tmpString.equalsIgnoreCase(F("delete"))) {
            _P126_delete_configs(&discovery);
          }
          else if (tmpString.equalsIgnoreCase(F("debug"))) {
            _P126_debug(&discovery);
          }
          else if (tmpString.equalsIgnoreCase(F("cleanup"))) {
            int duration = TASKS_MAX * 2 * _P126_INTERVAL / 1000 / 60;
            _P126_log = F("[P126] starting cleanup; expected duration is: ");
            _P126_log += String(duration);
            _P126_log += F(" minutes");
            addLog(LOG_LEVEL_INFO, _P126_log);

            _P126_cleaning = true;   // enable cleanup-mode
            _P126_time = millis();
            _P126_increment = 0;

            _P126_delete_system(&discovery);   // run once

            _P126_cleanup(&discovery);  // run first time
          }
          else {success = false;}
        }
      }
      break;
    }

  	case PLUGIN_EXIT:
  	{
      //_P126_get_ctrl(ctrlid, ctrldat);
      //success = _P126_delete_configs(event->TaskIndex, ctrlid, ctrldat);
  	  //perform cleanup tasks here. For example, free memory

  	  break;

  	}

    case PLUGIN_ONCE_A_SECOND:
    {
      if (_P126_cleaning && timeOutReached(_P126_time + _P126_INTERVAL)) {
        _P126_time = millis();

        struct DiscoveryStruct discovery;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

        if (discovery.save.init != _P126_VERSION) {
          addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
          break;
        } 

        _P126_get_ctrl(&discovery);

        _P126_cleanup(&discovery);  // run 2nd to nth time

        if (_P126_increment >= TASKS_MAX * 2 - 1) {
          _P126_cleaning = false;
          _P126_increment = 0;

          addLog(LOG_LEVEL_INFO, F("[P126] cleanup completed"));
        } else {
          tmpnote = F("[P126] cleanup ");
          tmpnote += String(_P126_increment);
          tmpnote += F(" of ");
          tmpnote += String(TASKS_MAX * 2 - 1);
          tmpnote += F(" completed");
          addLog(LOG_LEVEL_DEBUG, tmpnote);

          _P126_increment++;
        }
      }
      success = true;
    }  
  } //switch

  return success;
}

//---------------------------------------------------------------------------//
// plugin functions
void _P126_make_classes() {

  //---------------------------------------------------------------------------//
  // declare classes and units
    _P126_class[_P126_GENERIC] = F("generic sensor");        //  %   raw   K
    _P126_class[_P126_BATTERY] = F("battery");                    //  %   
    _P126_class[_P126_HUMIDITY] = F("humidity");                   //  %
    _P126_class[_P126_LIGHT] = F("illuminance");                //  lx  lm
    _P126_class[_P126_TEMP] = F("temperature");                //  °C  °F
    _P126_class[_P126_PRESSURE] = F("pressure");                   //  hPa mbar
    _P126_class[_P126_PLANT] = F("plant");                      //  %
    _P126_class[_P126_RADIO] = F("radio/RF");                      //  none
    _P126_class[_P126_LOAD] = F("scale/load/weight");          //  kg

    _P126_binclass[0] = F("generic binary sensor");
    _P126_binclass[1] = F("OUTPUT switch");
    _P126_binclass[2] = F("battery"); 
    _P126_binclass[3] = F("cold");
    _P126_binclass[4] = F("connectivity");
    _P126_binclass[5] = F("door");
    _P126_binclass[6] = F("garage_door");
    _P126_binclass[7] = F("gas");
    _P126_binclass[8] = F("heat");
    _P126_binclass[9] = F("light");
    _P126_binclass[10] = F("lock");
    _P126_binclass[11] = F("moisture");
    _P126_binclass[12] = F("motion");
    _P126_binclass[13] = F("moving");
    _P126_binclass[14] = F("occupancy");
    _P126_binclass[15] = F("opening");
    _P126_binclass[16] = F("plug");
    _P126_binclass[17] = F("power");
    _P126_binclass[18] = F("presence");
    _P126_binclass[19] = F("problem");
    _P126_binclass[20] = F("safety");
    _P126_binclass[21] = F("smoke");
    _P126_binclass[22] = F("sound");
    _P126_binclass[23] = F("vibration");
    _P126_binclass[24] = F("water");
    _P126_binclass[25] = F("window");

    _P126_unit[0] = F("NONE ");
    _P126_unit[1] = F("%");
    _P126_unit[2] = F("dB");
    _P126_unit[3] = F("count"); 
    _P126_unit[4] = F("total");
    _P126_unit[5] = F("h");     // hours
    _P126_unit[6] = F("m");     // minutes
    _P126_unit[7] = F("s");     // seconds
    _P126_unit[8] = F("raw");

    _P126_lightunit[0] = F("lx");
    _P126_lightunit[1] = F("lm");
    _P126_lightunit[2] = F("infrared");
    _P126_lightunit[3] = F("broadband");

    _P126_tempunit[0] = F("°C");
    _P126_tempunit[1] = F("°F");

    _P126_energyunit[0] = F("V");
    _P126_energyunit[1] = F("A");
    _P126_energyunit[2] = F("W");
    _P126_energyunit[3] = F("Pulses");
    _P126_energyunit[4] = F("raw");

    _P126_loadunit[0] = F("g");
    _P126_loadunit[1] = F("kg");
    _P126_loadunit[2] = F("lb");
    _P126_loadunit[3] = F("raw");

    _P126_distanceunit[0] = F("mm");
    _P126_distanceunit[1] = F("cm");
    _P126_distanceunit[2] = F("m");
    _P126_distanceunit[3] = F("ft");

    _P126_particleunit[0] = F("pm");
    _P126_particleunit[1] = F("ppm");
    _P126_particleunit[2] = F("dust");
    // _P126_particleunit[1] = F("pm2.5");
    // _P126_particleunit[2] = F("pm10");

    _P126_pressunit[0] = F("hPa");
    _P126_pressunit[1] = F("mbar");

    _P126_colorunit[0] = F("r");
    _P126_colorunit[1] = F("g");
    _P126_colorunit[2] = F("b");
    _P126_colorunit[3] = F("W");
    _P126_colorunit[4] = F("Y");
    _P126_colorunit[5] = F("K");
    _P126_colorunit[6] = F("lx");
    _P126_colorunit[7] = F("%");
  //---------------------------------------------------------------------------//

}

int _P126_find_sensors(struct DiscoveryStruct *discovery) {
  int msgcount = _P126_find_sensors(discovery, false);
  return msgcount;
}

int _P126_find_sensors(struct DiscoveryStruct *discovery, bool save) {
  _P126_log = F("P[126] search active tasks: state of save is : ");
  _P126_log += String(save);
  addLog(LOG_LEVEL_DEBUG,_P126_log);

    _P126_debug2(F("find sensor, save mode : "), save);

  int msgcount = 0;
  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;
  byte lastActiveTaskIndex = 0;

  _P126_make_classes();

  if (save) {
    lastActiveTaskIndex = lastTaskIndex;
  } else {
    for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {            // find last task
      if (Settings.TaskDeviceNumber[TaskIndex]) lastActiveTaskIndex = TaskIndex;
    }
  }
  
  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastActiveTaskIndex; TaskIndex++) {      // for each task
    //discovery->save.task[TaskIndex].enable = false;
    _P126_debug2(F("find sensor "));
    _P126_debug2(F("taskindex "), TaskIndex);
    if (Settings.TaskDeviceNumber[TaskIndex]) {    
      byte pluginid = Settings.TaskDeviceNumber[TaskIndex];                                         // if task exists
    _P126_debug2(F("pluginid "),pluginid);
      byte DeviceIndex = getDeviceIndex(pluginid);
      LoadTaskSettings(TaskIndex);

      if (Settings.TaskDeviceEnabled[TaskIndex]) {                                            // if task enabled
        bool ctrlenabled = false;
        _P126_debug2(F("task is enabled "));
        for (byte x = 0; x < CONTROLLER_MAX; x++) {                                           // if any controller enabled //[TODO] only if specific controller matches
          if (Settings.TaskDeviceSendData[x][TaskIndex]) ctrlenabled = true;
        }

        if (ctrlenabled) {
          if (Device[DeviceIndex].ValueCount != 0) {                                          // only tasks with values
            _P126_log = F("P[126] found active task with id : ");
            _P126_log += String(TaskIndex);
            addLog(LOG_LEVEL_DEBUG,_P126_log);
            // Serial.println(_P126_log);
            _P126_debug2(F("ctrl enabled and values available "));
            if (!save) {
              String header = F("Task ");
              header += String(TaskIndex + 1);
              header += F(" - ");
              header += getPluginNameFromDeviceIndex(DeviceIndex);
              addFormSubHeader(header);
              _P126_debug2(F("print header "),header);
            }

                      byte unitmap; //[todo] where to declare? compiler complaining

            for (byte x = 0; x < VARS_PER_TASK; x++) {                                        // for each value
              if (x < Device[DeviceIndex].ValueCount && !(x > 0 && pluginid == _P126_HASWITCH)) {
                if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() > 0) {
                  _P126_log = F("P[126] found value with name : ");
                  _P126_log += String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                  addLog(LOG_LEVEL_DEBUG,_P126_log);
                  // Serial.println(_P126_log);
                  _P126_debug2(F("starting with value "),x);
                  msgcount++;

                  String enableid = F("P126_");
                  enableid += String(TaskIndex);
                  enableid += F("_");
                  enableid += String(x);
                  enableid += F("_enable");
                  _P126_debug2(F("enableid "),enableid);

                  String classid = F("P126_");
                  classid += String(TaskIndex);
                  classid += F("_");
                  classid += String(x);
                  classid += F("_type");
                  _P126_debug2(F("classid "),classid);

                  String unitid = F("P126_");
                  unitid += String(TaskIndex);
                  unitid += F("_");
                  unitid += String(x);
                  unitid += F("_unit");
                  _P126_debug2(F("unitid: "),unitid);

                  String outputid = F("P126_");
                  outputid += String(TaskIndex);
                  outputid += F("_");
                  outputid += String(x);
                  outputid += F("_output");
                  _P126_debug2(F("outputid: "),outputid);

                  if (!save) {  // if loading webform
                    //---------------------------------------------------------------------------//
                    // print value header                  
                      String valuename = F("<font color=\"#07D\"><b>");
                      valuename += String(ExtraTaskSettings.TaskDeviceName);
                      if (pluginid != _P126_HASWITCH) {
                        valuename += F(" ");
                        valuename += String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                        valuename += F("</b></font>");
                      }
                      
                      html_TR_TD();
                      addHtml(valuename);
                      _P126_debug2(F("valuename: "),valuename);
                    //---------------------------------------------------------------------------//
                    // print entity_id
                      bool b1 = discovery->save.usename;
                      String s1 = Settings.Name;
                      bool b2 = discovery->save.usecustom;
                      String s2 = discovery->save.custom;
                      bool b3 = discovery->save.usetask;
                      String s3 = ExtraTaskSettings.TaskDeviceName;
                      bool b4 = discovery->save.usevalue;
                      String s4 = ExtraTaskSettings.TaskDeviceValueNames[x];

                      if (s1.length() == 0) b1 = false;
                      if (s2.length() == 0) b2 = false;
                      if (s3.length() == 0) b3 = false;
                      if (s4.length() == 0) b4 = false;

                      // change order if set
                        if (discovery->save.moved) {
                          b4 = discovery->save.usecustom;
                          s4 = discovery->save.custom;
                          b2 = discovery->save.usetask;
                          s2 = ExtraTaskSettings.TaskDeviceName;
                          b3 = discovery->save.usevalue;
                          s3 = ExtraTaskSettings.TaskDeviceValueNames[x];
                        }

                      String entity_id = F("<span class='note'>");
                      if (pluginid == _P126_HASWITCH) {
                        entity_id += _P128_F(TaskIndex, _P128_COMPONENT);
                        entity_id += F(".");
                        b3 = true;
                        b4 = false;
                      } else {
                        if (discovery->save.task[TaskIndex].value[x].binary) {
                          if (discovery->save.task[TaskIndex].value[x].type == 1) {
                            entity_id += F("switch.");
                          } else {
                            entity_id += F("binary_sensor.");
                          }
                        } else {
                          entity_id += F("sensor.");
                        }
                      }

                        if (b1) {
                          entity_id += String(s1);
                          if (b2 || b3 || b4) entity_id += F("_");
                        }

                        if (b2) {
                          entity_id += String(s2); 
                          if (b3 || b4) entity_id += F("_");
                        }

                        if (b3) {
                          entity_id += String(s3);
                          if (b4) entity_id += F("_");
                        }

                        if (b4) {
                        entity_id += String(s4);
                        }
                      

                      entity_id += F("</span>");
                      entity_id.toLowerCase();
                      
                      html_TD();
                      addHtml(entity_id);
                      _P126_debug2(F("entityid "),entity_id);

                      if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() == 0) {
                        addFormNote(F("No valuename set in device options!"));
                        _P126_debug2(F("no valuename set "));
                      }

                    //---------------------------------------------------------------------------//
                    // define settings by plugin
                      int pluginid = Settings.TaskDeviceNumber[TaskIndex];

                      int classcount = 0;
                      String *classdropdown[_P126_MAXCLASS];
                      int classpreselect = 0;

                      int unitcount = 0;
                      String *unitdropdown[_P126_MAXUNIT];
                      String singleunit;
                      _P126_debug2(F("search settings for plugin "),pluginid);

                      // by default make float-sensor choice 
                      for (byte y = 0; y < _P126_CLASSCOUNT; y++) {
                        classdropdown[y] = &_P126_class[y];
                      }


                      switch (pluginid) {
                        case _P126_HASWITCH:
                        // if task is HA-Device plugin make vars to skip next steps 
                          classcount = -2;
                          unitcount = -2;
                          _P126_debug2(F("plugin is HAswitch, break "));
                          break;
                        case 1: //switch
                        case 9:
                        case 19:
                        // if task is binary, overwrite choice with binary options
                        // for binary no unit required...
                          _P126_debug2(F("binary sensor found "));
                          classcount = _P126_BINCLASSCOUNT;
                          for (byte y = 0; y < _P126_BINCLASSCOUNT; y++) {
                            classdropdown[y] = &_P126_binclass[y];
                          }
                          break;

                        case 4:   //temp
                        case 24:   //temp
                        case 39:   //temp
                        case 69:   //temp
                        case 5:   //temp/humidity
                        case 14:  //temp/humidity
                        case 31:  //temp/humidity
                        case 34:  //temp/humidity
                        case 51:  //temp/humidity
                        case 68:  //temp/humidity
                        case 72:  //temp/humidity
                        case 28:  //temp/humidity/pressure
                          _P126_debug2(F("temp/hum/press found "));
                          switch (x) {
                            case 0:
                              classpreselect = _P126_TEMP;
                              unitcount = _P126_TEMPUNITCOUNT;
                              unitmap = _P126_TEMPUNITMAP;
                              for (byte y = 0; y < _P126_TEMPUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_tempunit[y];
                              }
                              break;
                            case 1:
                              classpreselect = _P126_HUMIDITY;
                              unitcount = 1;
                              singleunit = F("%");
                              unitdropdown[0] = &singleunit;
                              unitmap = _P126_PERCENTUNITMAP;
                              break;
                            case 2:
                              classpreselect = _P126_PRESSURE;
                              unitcount = _P126_PRESSUNITCOUNT;
                              unitmap = _P126_PRESSUNITMAP;
                              for (byte y = 0; y < _P126_PRESSUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_pressunit[y];
                              }
                              break;
                          }
                          break;

                        case 6:  //temp/pressure
                        case 30:  //temp/pressure
                        case 32:  //temp/pressure
                          switch (x) {
                            case 0:
                              classpreselect = _P126_TEMP;
                              unitcount = _P126_TEMPUNITCOUNT;
                              unitmap = _P126_TEMPUNITMAP;
                              for (byte y = 0; y < _P126_TEMPUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_tempunit[y];
                              }
                              break;
                            case 1:
                              classpreselect = _P126_PRESSURE;
                              unitcount = _P126_PRESSUNITCOUNT;
                              unitmap = _P126_PRESSUNITMAP;
                              for (byte y = 0; y < _P126_PRESSUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_pressunit[y];
                              }
                              break;
                          }
                          break;

                        case 47:  //temp/moisture/light
                          switch (x) {
                            case 0:
                              classpreselect = _P126_TEMP;
                              unitcount = _P126_TEMPUNITCOUNT;
                              unitmap = _P126_TEMPUNITMAP;
                              for (byte y = 0; y < _P126_TEMPUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_tempunit[y];
                              }
                              break;
                            case 1:
                              unitcount = 1;
                              singleunit = F("%");
                              unitdropdown[0] = &singleunit;
                              unitmap = _P126_PERCENTUNITMAP;
                              break;
                            case 2:
                              classpreselect = _P126_LIGHT;
                              unitcount = _P126_LIGHTUNITCOUNT;
                              unitmap = _P126_LIGHTUNITMAP;
                              for (byte y = 0; y < _P126_LIGHTUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_lightunit[y];
                              }
                              break;
                          }
                          break;
                        case 10: //light
                        case 15:
                          classpreselect = _P126_LIGHT;
                          unitcount = _P126_LIGHTUNITCOUNT;
                          unitmap = _P126_LIGHTUNITMAP;
                          for (byte y = 0; y < _P126_LIGHTUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_lightunit[y];
                          }
                          break;

                        case 13: //distance
                        // has no class, to be skipped
                          classcount = -1;
                          unitcount = _P126_DISTANCEUNITCOUNT;
                          unitmap = _P126_DISTANCEUNITMAP;
                          for (byte y = 0; y < _P126_DISTANCEUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_distanceunit[y];
                          }
                          break;

                        case 49:  //dust/temp
                          switch (x) {
                            case 0:
                            // has no class, to be skipped
                              classcount = -1;
                              unitcount = _P126_PARTICLEUNITCOUNT;
                              unitmap = _P126_PARTICLEUNITMAP;
                              for (byte y = 0; y < _P126_PARTICLEUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_particleunit[y];
                              }
                              break;
                            case 1:
                              classpreselect = _P126_TEMP;
                              unitcount = _P126_TEMPUNITCOUNT;
                              unitmap = _P126_TEMPUNITMAP;
                              for (byte y = 0; y < _P126_TEMPUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_tempunit[y];
                              }
                              break;
                          }
                          break;

                        case 18:
                        case 56: //dust/dust
                        case 53: //dust/dust/dust
                        // has no class, to be skipped
                          classcount = -1;
                          unitcount = _P126_PARTICLEUNITCOUNT;
                          unitmap = _P126_PARTICLEUNITMAP;
                          for (byte y = 0; y < _P126_PARTICLEUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_particleunit[y];
                          }
                          break;

                        case 66: //color
                        case 50:
                          classpreselect = _P126_LIGHT;
                          unitcount = _P126_COLORUNITCOUNT;
                          unitmap = _P126_COLORUNITMAP;
                          for (byte y = 0; y < _P126_COLORUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_colorunit[y];
                          }
                          break;

                        case 67: //hx711 customclass
                          classpreselect = _P126_LOAD;
                          unitcount = _P126_LOADUNITCOUNT;
                          unitmap = _P126_LOADUNITMAP;
                          for (byte y = 0; y < _P126_LOADUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_loadunit[y];
                          }
                          break;

                        case 78: //energy variable
                        case 76:
                        case 77:
                        case 27:
                        // has no class, to be skipped
                          classcount = -1;
                          unitcount = _P126_ENERGYUNITCOUNT;
                          unitmap = _P126_ENERGYUNITMAP;
                          for (byte y = 0; y < _P126_ENERGYUNITCOUNT; y++) {
                            unitdropdown[y] = &_P126_energyunit[y];
                          }
                          break;

                        default: //anything else
                        // no class found, let user choose
                          classcount = _P126_CLASSCOUNT;
                          unitcount = -1;
                          break;
                      }

                      // no class found, let user choose
                      if (classcount > 0) { 
                        addFormSelector(F("type"), classid, classcount, *classdropdown, NULL, discovery->save.task[TaskIndex].value[x].type);
                      // class found and preselected
                      } else if (classcount == 0) { 
                        addFormSelector(F("type"), classid, _P126_CLASSCOUNT, *classdropdown, NULL, classpreselect);
                      }
                      // for classified and classless, give unit choice
                      if (unitcount > 0) addFormSelector(F("unit"), unitid, unitcount, *unitdropdown, NULL, discovery->save.task[TaskIndex].value[x].unit);

                    //---------------------------------------------------------------------------//
                    // if no plugin settings defined (anything else), take user input
                      if (unitcount == -1) {
                        switch (discovery->save.task[TaskIndex].value[x].type) {
                          case 0: //generic
                            unitcount = _P126_UNITCOUNT;
                            unitmap = _P126_UNITMAP;
                            for (byte y = 0; y < _P126_UNITCOUNT; y++) {
                              unitdropdown[y] = &_P126_unit[y];
                            }
                            break;
                          case 3:
                            unitcount = _P126_LIGHTUNITCOUNT;
                            unitmap = _P126_LIGHTUNITMAP;
                            for (byte y = 0; y < _P126_LIGHTUNITCOUNT; y++) {
                              unitdropdown[y] = &_P126_lightunit[y];
                            }
                            break;
                          case 4:
                            unitcount = _P126_TEMPUNITCOUNT;
                            unitmap = _P126_TEMPUNITMAP;
                              for (byte y = 0; y < _P126_TEMPUNITCOUNT; y++) {
                                unitdropdown[y] = &_P126_tempunit[y];
                              }
                            break;
                          case 5:
                            unitcount = _P126_PRESSUNITCOUNT;
                            unitmap = _P126_PRESSUNITMAP;
                            for (byte y = 0; y < _P126_PRESSUNITCOUNT; y++) {
                              unitdropdown[y] = &_P126_pressunit[y];
                            }
                            break;
                          case 1: //battery
                          case 2: //humidity
                          case 6: //plant
                              unitcount = 1;
                              singleunit = F("%");
                              unitdropdown[0] = &singleunit;
                              unitmap = _P126_PERCENTUNITMAP;
                              break;
                          case 8: //load
                            unitcount = _P126_LOADUNITCOUNT;
                            unitmap = _P126_LOADUNITMAP;
                            for (byte y = 0; y < _P126_LOADUNITCOUNT; y++) {
                              unitdropdown[y] = &_P126_loadunit[y];
                            }
                            break;
                        }

                        addFormSelector(F("unit"), unitid, unitcount, *unitdropdown, NULL, discovery->save.task[TaskIndex].value[x].unit);
                      }

                      addFormCheckBox(F("enable"), enableid, discovery->save.task[TaskIndex].value[x].enable);

                      discovery->save.task[TaskIndex].value[x].unitmap = unitmap;

                      html_BR();
                    //---------------------------------------------------------------------------//

                  } else {    // if saving webform
                    int pluginid = Settings.TaskDeviceNumber[TaskIndex];

                    if (pluginid == (1 || 9 || 19)) discovery->save.task[TaskIndex].value[x].binary = true;

                    if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() == 0) {    // disable if empty value name
                      discovery->save.task[TaskIndex].value[x].enable = false;
                    } else {
                      discovery->save.task[TaskIndex].value[x].enable = isFormItemChecked(enableid);    // else use user input
                      if (discovery->save.task[TaskIndex].value[x].enable) discovery->save.task[TaskIndex].enable = true;   // if value enabled, enable task as well
                    }

                    discovery->save.task[TaskIndex].value[x].type = getFormItemInt(classid, 0);

                    discovery->save.task[TaskIndex].value[x].unit = getFormItemInt(unitid, 0);
                  }
                }
              }
            } // values
          } // tasks with value
        } // controller enabled
      } // enabled tasks
    } // existing tasks
  } //all tasks
  return msgcount;
}

bool _P126_system_config(struct DiscoveryStruct *discovery, bool brief) {

  //  create topics
    //  prefix/component/device/entity/[config|state]
    //  %prefix% / %component% / %mac% / %mac%_system / [config|state]
    String discovery_topic = _P126_make_topic(discovery->save.prefix, F("sensor"), _P126_make_unique());

    String state_topic = discovery_topic;
    state_topic.replace(F("/config"),F("/state"));

  //  create discovery payload
    String payload;
    if (Settings.TaskDeviceTimer[discovery->taskid] == 0) {
      payload = "";
    } else {
      payload = F("{");
      payload += _P126_add_line(F("name"), String(Settings.Name));
      payload += _P126_add_line(F("ic"), F("mdi:chip"));
      payload += _P126_add_line(F("stat_t"), state_topic);
      payload += _P126_add_line(F("val_tpl"), F("{{ value_json.state }}"));
      payload += _P126_add_line(F("uniq_id"), _P126_make_unique());
      payload += _P126_add_line(F("avty_t"), discovery->lwttopic);
      payload += _P126_add_line(F("pl_avail"), discovery->lwtup);
      payload += _P126_add_line(F("pl_not_avail"), discovery->lwtdown);
      if (brief) {
        payload += F("\"json_attr\":[\"plugin\",\"unit\",\"uptime\",\"ip\",\"rssi\"],");
      } else {
        payload += F("\"json_attr\":[\"plugin\",\"unit\",\"version\",\"uptime\",\"cpu\",\"hostname\",\"ip\",\"mac\",\"ssid\",\"bssid\",\"rssi\",\"last_disconnect\",\"last_boot_cause\"],");
      }
      payload += _P126_add_device(brief);
      #ifdef _P126_DEVICE
      payload.replace(ARDUINO_BOARD,discovery->save.device);
      #endif
        _P126_log = F("P[126] payload created : ");
        _P126_log += payload;
        addLog(LOG_LEVEL_DEBUG,_P126_log);
    }

  //  check payload size and publish                     
    if (_P126_check_length(discovery_topic, payload)) {
      return (_P126_publish(discovery->ctrlid, discovery_topic, payload));
    } else if (!brief) {
      addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying with reduced content."));
      return (_P126_system_config(discovery, true)); // start over with brief-flag
    } else {
      _P126_log = F("P[126] payload exceeds limits. You can publish the message manually from other client. topic : ");
      _P126_log += discovery_topic;
      addLog(LOG_LEVEL_ERROR,_P126_log);
      _P126_log = F("P[126] payload : ");
      _P126_log += payload;
      addLog(LOG_LEVEL_ERROR,_P126_log);
        // Serial.println(_P126_log);
      return false;
    }
}  

bool _P126_sensor_config(struct DiscoveryStruct *discovery, bool brief) {
  bool success = false;

  byte lastTaskIndex = TASKS_MAX - 1;

  _P126_make_classes();

  for (byte TaskIndex = 0; TaskIndex <= lastTaskIndex; TaskIndex++) {   // for each task

    if (discovery->save.task[TaskIndex].enable) {   // if task enabled
      LoadTaskSettings(TaskIndex);

      String tasktopic = String(discovery->publish);
      tasktopic.replace(F("%tskname%"), ExtraTaskSettings.TaskDeviceName);

      byte pluginid = Settings.TaskDeviceNumber[TaskIndex];
      byte DeviceIndex = getDeviceIndex(pluginid);

      for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++) {   // for each value

        if (discovery->save.task[TaskIndex].value[x].enable && !(x > 0 && pluginid == _P126_HASWITCH)) {    // if value enabled
          success = false;

          //---------------------------------------------------------------------------//
          //  get state topic from controller settings
            String state_topic = tasktopic;
            state_topic.replace(F("%valname%"), ExtraTaskSettings.TaskDeviceValueNames[x]);


          //---------------------------------------------------------------------------//
          //  define component, entity_id, topic
            String component;
            String entity_id;
            String uniquestr;
            String discovery_topic;

            if (pluginid == _P126_HASWITCH) {
              component = _P128_F(TaskIndex, _P128_COMPONENT);
              //entity_id = _P128_F(TaskIndex, _P128_ENTITYID);
              discovery_topic = _P128_F(TaskIndex, _P128_TOPIC);
            } else {

              entity_id = _P126_make_entity(discovery,TaskIndex, x);

              if (discovery->save.task[TaskIndex].value[x].binary) {
                if (discovery->save.task[TaskIndex].value[x].type == 1) {
                  component = F("switch");
                } else {
                  component = F("binary_sensor");
                }
              } else {
                component = F("sensor");
              }

              uniquestr = _P126_make_unique(TaskIndex, x);   // eg 00AA11BB22CC_1_1

              discovery_topic = _P126_make_topic(discovery->save.prefix, component, uniquestr); 
            }



          //---------------------------------------------------------------------------//
          //  create payload
            String payload;
            String unit_of_meas;
            String cmdtopic;
            //String payloadon;
            //String payloadoff;

            if (pluginid == _P126_HASWITCH) {
              payload = _P128_F(TaskIndex, _P128_PAYLOAD);
            } else if (discovery->save.task[TaskIndex].value[x].enable) {    // if enabled, create json

              payload = F("{");
              payload += _P126_add_line(F("name"), entity_id);

              //---------------------------------------------------------------------------//
              // dealing with binary_sensors/switches
                if (discovery->save.task[TaskIndex].value[x].binary) { 
                  String pl_offon[2];
                  pl_offon[0] = F("0");  // what HA will send & receive
                  pl_offon[1] = F("1");  // what HA will send & receive

                  switch (discovery->save.task[TaskIndex].value[x].type) {
                    case 0: // classless, skip
                      break;
                    case 1: //output switch
                      cmdtopic = discovery->cmdtopic;
                      cmdtopic += F("gpio/");
                      cmdtopic += String(Settings.TaskDevicePin1[TaskIndex]);
                      payload += _P126_add_line(F("cmd_t"), cmdtopic);
                      // dealing with inversed GPIOs
                        if (Settings.TaskDevicePin1Inversed[TaskIndex]) {
                          // if inversed
                            //  HA  on  > ESP = 0
                            //  HA  off > ESP = 1
                            //  ESP on  > HA  = 1   //must be inverted
                            //  ESP off > HA  = 0   //must be inverted
                          pl_offon[0] = F("1");  // what HA will send for off
                          pl_offon[1] = F("0");  // what HA will send for on
                          payload += _P126_add_line(F("val_tpl"), F("{{(value|int-1)*-1}}"));  // what HA will receive
                        }
                      break;

                    case 24: //water
                      payload += _P126_add_line(F("ic"), F("mdi:cup-water"));
                      break;

                    default: //native classes
                      payload += _P126_add_line(F("dev_cla"), _P126_binclass[discovery->save.task[TaskIndex].value[x].type]);
                      break;
                  }

                  payload += _P126_add_line(F("pl_off"), pl_offon[0]);
                  payload += _P126_add_line(F("pl_on"), pl_offon[1]);
              //---------------------------------------------------------------------------//
              // dealing with float sensors
                } else {    
                  //---------------------------------------------------------------------------//
                  // dealing with device_classes and icons
                    switch (discovery->save.task[TaskIndex].value[x].type) { 
                      case 0: //classless, skip
                        break;
                      case 6: //plant
                        payload += _P126_add_line(F("ic"), F("mdi:leaf"));
                        break;
                      case 7: //radio
                        payload += _P126_add_line(F("ic"), F("mdi:radio-tower"));
                        break;
                      case 8: //scale
                        payload += _P126_add_line(F("ic"), F("mdi:scale"));
                        break;
                      default: //native classes
                        payload += _P126_add_line(F("dev_cla"), _P126_class[discovery->save.task[TaskIndex].value[x].type]);
                        break;
                    }
                  //---------------------------------------------------------------------------//
                  // dealing with units
                    // byte *unit = &discovery->save.task[TaskIndex].value[x].unit;
                    bool giveunit = true;
                    switch (discovery->save.task[TaskIndex].value[x].unitmap) {
                      case _P126_TEMPUNITMAP:
                        unit_of_meas = _P126_tempunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_LIGHTUNITMAP:
                        unit_of_meas = _P126_lightunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_PRESSUNITMAP:
                        unit_of_meas = _P126_pressunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_UNITMAP:
                        unit_of_meas = _P126_unit[discovery->save.task[TaskIndex].value[x].unit];
                        if (unit_of_meas == 0) giveunit = false; //if 'NONE' selected, skip
                        break;
                      case _P126_LOADUNITMAP:
                        unit_of_meas = _P126_loadunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_ENERGYUNITMAP:
                        unit_of_meas = _P126_energyunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_PARTICLEUNITMAP:
                        unit_of_meas = _P126_particleunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_DISTANCEUNITMAP:
                        unit_of_meas = _P126_distanceunit[discovery->save.task[TaskIndex].value[x].unit];
                        break;
                      case _P126_PERCENTUNITMAP:
                        unit_of_meas = F("%");
                        break;
                    }
                    if (giveunit) payload += _P126_add_line(F("unit_of_meas"), unit_of_meas);
                  //---------------------------------------------------------------------------//
                }
              //---------------------------------------------------------------------------//

              payload += _P126_add_line(F("stat_t"), state_topic);
              payload += _P126_add_line(F("uniq_id"), uniquestr);
              payload += _P126_add_line(F("avty_t"), discovery->lwttopic);
              payload += _P126_add_line(F("pl_avail"), discovery->lwtup);
              payload += _P126_add_line(F("pl_not_avail"), discovery->lwtdown);
              if (!brief && component == F("sensor")) {
                payload += _P126_add_line(F("frc_upd"), F("true"));
                payload += _P126_add_line(F("exp_aft"), F("3600"));
              }
              payload += _P126_add_device(brief);
              #ifdef _P126_DEVICE
              payload.replace(ARDUINO_BOARD,discovery->save.device);
              #endif
              
            } else {    // if disabled, publish empty payload to delete
              payload = F("");
            }


          //---------------------------------------------------------------------------//    
          //  check and publish payload
            if (_P126_check_length(discovery_topic, payload)) {
              success = _P126_publish(discovery->ctrlid, discovery_topic, payload);
              _P126_log = F("[P126] publish result : ");
              _P126_log += toString(success);
              addLog(LOG_LEVEL_INFO, _P126_log);
            } else if (!brief) {
              addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying again with reduced content."));
              success = _P126_sensor_config(discovery, true);
            } else {
              _P126_log = F("[P126] Cannot publish config, because payload exceeds limits. You can publish the message manually from other client.: state of payload is : ");
              _P126_log += payload;
              addLog(LOG_LEVEL_ERROR,_P126_log);
              
              success = false;
            }
          //---------------------------------------------------------------------------//
        } 
      } 
    } 
  }
  return success;
} 

String _P126_make_entity(struct DiscoveryStruct *discovery, byte task, byte value) {
  LoadTaskSettings(task);
  byte pluginid = Settings.TaskDeviceNumber[task];

  bool b1 = discovery->save.usename;
  String s1 = Settings.Name;
  bool b2 = discovery->save.usecustom;
  String s2 = discovery->save.custom;
  bool b3 = discovery->save.usetask;
  String s3 = ExtraTaskSettings.TaskDeviceName;
  bool b4 = discovery->save.usevalue;
  String s4 = ExtraTaskSettings.TaskDeviceValueNames[value];
  
  if (pluginid == _P126_HASWITCH) {
    b3 = true;
    b4 = false;
  } 

  if (discovery->save.moved) {    // change order if set
    b4 = discovery->save.usecustom;
    s4 = discovery->save.custom;
    b2 = discovery->save.usetask;
    s2 = ExtraTaskSettings.TaskDeviceName;
    b3 = discovery->save.usevalue;
    s3 = ExtraTaskSettings.TaskDeviceValueNames[value];
  }


  String entity_id = F("");
  if (b1) {
    entity_id += String(s1);
    if (b2 || b3 || b4) entity_id += F("_");
  }

  if (b2) {
    entity_id += String(s2); 
    if (b3 || b4) entity_id += F("_");
  }

  if (b3) {
    entity_id += String(s3);
    if (b4) entity_id += F("_");
  }

  if (b4) {
  entity_id += String(s4);
  }
  entity_id.toLowerCase();

  return entity_id;
}

String _P126_make_topic(char* prefix, String component, byte task, byte value) {
    
    String unique = _P126_make_unique(task, value);
    return _P126_make_topic(prefix, component, unique);
}

String _P126_make_topic(char* prefix, String component, String unique) {
    
    String topic = String(prefix);
    topic += F("/");
    topic += component;
    topic += F("/");   
    topic += WiFi.macAddress();
    topic.replace(F(":"),F(""));
    topic += F("/");
    topic += unique;
    topic += F("/config");
      _P126_log = F("P[126] topic defined : ");
      _P126_log += topic;
      addLog(LOG_LEVEL_DEBUG,_P126_log);

    return topic;
}

String _P126_make_unique() {
    String unique = WiFi.macAddress();
    unique.replace(F(":"),F(""));
    unique += F("_system");

    return unique;
}

String _P126_make_unique(byte task, byte value) {

    String unique = WiFi.macAddress();
    unique.replace(F(":"),F(""));
    unique += F("_");
    unique += String(task);
    unique += F("_");
    unique += String(value);
    
    return unique;
}

bool _P126_system_state(struct DiscoveryStruct *discovery, bool brief) {

  //  define topic
    String state_topic = _P126_make_topic(discovery->save.prefix, F("sensor"), _P126_make_unique());
    state_topic.replace(F("/config"),F("/state"));

  //  define payload
    String versionstr;
    if (String(BUILD_GIT) == "") {
      versionstr += String(CRCValues.compileDate);
    } else {
      versionstr += String(BUILD_GIT);
    }

    String payload = F("{");
    #if defined(ESP8266)
    payload += _P126_add_line(F("hostname"), WiFi.hostname());
    #endif
    payload += _P126_add_line(F("state"),  discovery->lwtup);
    payload += _P126_add_line(F("plugin"), String(_P126_VERSION));
    payload += _P126_add_line(F("version"), versionstr);
    payload += _P126_add_line(F("unit"), String(Settings.Unit));
    payload += _P126_add_line(F("uptime"), String(wdcounter / 2));
    payload += _P126_add_line(F("last_boot_cause"), getLastBootCauseString());
    payload += _P126_add_line(F("cpu"), String(getCPUload()));
    payload += _P126_add_line(F("ip"), WiFi.localIP().toString());
    payload += _P126_add_line(F("mac"), WiFi.macAddress());
    payload += _P126_add_line(F("ssid"), WiFi.SSID());
    payload += _P126_add_line(F("bssid"), WiFi.BSSIDstr());
    payload += _P126_add_line(F("last_disconnect"), getLastDisconnectReason());
    payload += _P126_add_line(F("rssi"), String(WiFi.RSSI()), true);

    //_P126_add_line(F("Subnet Mask"), WiFi.subnetMask().toString());
    //_P126_add_line(F("Gateway IP"), WiFi.gatewayIP().toString());
    //_P126_add_line(F("DNS 1"), WiFi.dnsIP(0).toString());
    //_P126_add_line(F("DNS 2"), WiFi.dnsIP(1).toString());
    //_P126_add_line(F("IP config"), useStaticIP() ? F("Static") : F("DHCP"));
    //_P126_add_line(F("Number reconnects"), String(wifi_reconnects));

  return (_P126_publish(discovery->ctrlid, state_topic, payload));
}

bool _P126_delete_system(struct DiscoveryStruct *discovery) {

  String discovery_topic = _P126_make_topic(discovery->save.prefix, F("sensor"), _P126_make_unique());

  return (_P126_publish(discovery->ctrlid, discovery_topic, ""));
}

bool _P126_cleanup(struct DiscoveryStruct *discovery) {
  bool success;

  byte lasttaskid = TASKS_MAX - 1;
  byte taskid = _P126_increment;
  String component = F("sensor");

  if (_P126_increment > lasttaskid) {
    taskid = _P126_increment - TASKS_MAX;
    component = F("binary_sensor");
  } 

    for (byte x = 0; x < VARS_PER_TASK; x++) {                       // for each value
      success = false;

      String uniquestr = _P126_make_unique(taskid, x);

      String discovery_topic = _P126_make_topic(discovery->save.prefix, component, uniquestr);
      success = _P126_publish(discovery->ctrlid, discovery_topic, "");
    }   

  return success;
}

bool _P126_delete_configs(struct DiscoveryStruct *discovery) {

  bool success1 = _P126_delete_system(discovery);
  bool success2 = false;

  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;
  byte lastActiveTaskIndex = 0;

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {
    if (Settings.TaskDeviceNumber[TaskIndex]) lastActiveTaskIndex = TaskIndex;
  }
  
  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastActiveTaskIndex; TaskIndex++) {      // for each task
    if (Settings.TaskDeviceNumber[TaskIndex]) {                                               // if task exists
      byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);
      LoadTaskSettings(TaskIndex);

      if (Settings.TaskDeviceEnabled[TaskIndex]) {                                              // if task enabled
        bool ctrlenabled = false;
        for (byte x = 0; x < CONTROLLER_MAX; x++) {                                             // if any controller enabled //[TODO] only if specific controller matches
          if (Settings.TaskDeviceSendData[x][TaskIndex]) ctrlenabled = true;
        }

        if (ctrlenabled) {
          if (Device[DeviceIndex].ValueCount != 0) {                                              // only tasks with values

            for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++) {                           // for each value
              success2 = false;

              String uniquestr = _P126_make_unique(TaskIndex, x);

              String sensorname = String(ExtraTaskSettings.TaskDeviceName);
              sensorname += F("_");
              sensorname += String(ExtraTaskSettings.TaskDeviceValueNames[x]);

              String component;
              if (discovery->save.task[TaskIndex].value[x].type < 10) {
                component = F("sensor");
              } else {
                component = F("binary_sensor");
              }

              String discovery_topic = _P126_make_topic(discovery->save.prefix, component, uniquestr);

              success2 = _P126_publish(discovery->ctrlid, discovery_topic, F(""));
            }
          }
        }
      }
    }
  }
  if (success1 && success2) {
    return true;
  } else {
    return false;
  }
    
}

String _P126_add_device(bool brief) {
  String buildstr = "EspEasy";
  buildstr += BUILD_NOTES;
  //buildstr.replace(F(" - "),"");
  if (!brief) { 
    buildstr += F(" (");
    if (String(BUILD_GIT).length() == 0) {
      buildstr += String(CRCValues.compileDate);
    } else {
      buildstr += String(BUILD_GIT);
    }
    buildstr += F(")");
  }

  #if defined(ESP8266)
     String espid = String(ESP.getChipId());
  #endif
  #if defined(ESP32)
    String espid = F("0x");
    uint64_t chipid=ESP.getEfuseMac();   //The chip ID is essentially its MAC address(length: 6 bytes).
    uint32_t ChipId1 = (uint16_t)(chipid>>32);
    String espChipIdS(ChipId1, HEX);
    espChipIdS.toUpperCase();
    espid += espChipIdS;
    ChipId1 = (uint32_t)chipid;
    String espChipIdS1(ChipId1, HEX);
    espChipIdS1.toUpperCase();
    espid += espChipIdS1;
  #endif

  //device data
  String device = F("\"device\":{");
    if (!brief) { 
      device += _P126_add_line(F("sw_version"), buildstr);
      device += _P126_add_line(F("manufacturer"), F("letscontrolit.com"));
      device += _P126_add_line(F("model"), ARDUINO_BOARD);
      //connections
      device += F("\"connections\":[");
        device += F("[\"mac\",\""); 
        device += WiFi.macAddress();
        device += F("\"]");
        device += F("],");  
    }
    device += _P126_add_line(F("name"), String(Settings.Name));                    
    //IDs
    device += F("\"identifiers\":[");
      // if (!brief) {
      //   payload += F("\"espeasy\",");
      //   payload += String(Settings.Unit);
      //   payload += F(",");
      // }
      device += F("\"");
      device += espid;
      device += F("\"");
      device += F(",\"");
      String deviceid = WiFi.macAddress();
      deviceid.replace(F(":"),F(""));
      device += deviceid;
      device += F("\"]");                       
    device += F("}}");  // 1st '}' device, 2nd '}' payload
    return device;
}

String _P126_add_line(const String& object, const String& value) {
  String line = _P126_add_line(object, value, false);
  return line;
}

String _P126_add_line(const String& object, const String& value, bool last) {

  String line = F("\"");
  line += object;
  line += F("\":");
  if (value.length() == 0 || !isFloat(value)) {
    line += F("\"");
    line += value;
    line += F("\"");
  } else {
    line += value;
  }
  if (last) {
    line += F("}");
  } else {
    line += F(",");
  }
  return line;
}

bool _P126_check_length(const String& topic, const String& payload) {
  int tlength = topic.length();
  int plength = payload.length();
  int flength = 5 + 2 + tlength + plength;

  _P126_log = F("P[126] payload size is : ");
  _P126_log += String(flength);
  _P126_log += F(" - MQTT_MAX_PACKET_SIZE is : ");
  _P126_log += String(MQTT_MAX_PACKET_SIZE);
  addLog(LOG_LEVEL_DEBUG,_P126_log);
    // Serial.println(_P126_log);
  
  if (MQTT_MAX_PACKET_SIZE >= flength) {
    return true;
  } else {
    return false;
  }
  
}

bool _P126_publish(int ctrlid, const String& topic, const String& payload) {
  bool success = false;
  _P126_log = F("P[126] publish : topic : ");
  _P126_log += topic;
  addLog(LOG_LEVEL_DEBUG,_P126_log);
    // Serial.println(_P126_log);
  
  if (MQTTCheck(ctrlid)) {
    _P126_log = F("P[126] publish : controller check ok");
    //_P126_log += payload;
    addLog(LOG_LEVEL_DEBUG,_P126_log);
      // Serial.println(_P126_log);
    
    if (ctrlid >= 0) {
      success = MQTTpublish(ctrlid, topic.c_str(), payload.c_str(), Settings.MQTTRetainFlag);
    }
  } else {
    addLog(LOG_LEVEL_DEBUG, F("[P126] publish : controller check failed"));
  }


  return success;
}

void _P126_get_ctrl(struct DiscoveryStruct *discovery) {
  discovery->ctrlid = firstEnabledMQTTController();
  MakeControllerSettings(ControllerSettings);
  LoadControllerSettings(discovery->ctrlid, ControllerSettings);

  discovery->cmdtopic = ControllerSettings.Subscribe;
  discovery->cmdtopic.replace(F("%sysname%"), Settings.Name);
  discovery->cmdtopic.replace(F("#"), F(""));
  discovery->publish = ControllerSettings.Publish;
  discovery->publish.replace(F("%sysname%"), Settings.Name);
  discovery->lwttopic = ControllerSettings.MQTTLwtTopic;
  discovery->lwttopic.replace(F("%sysname%"), Settings.Name);
  discovery->lwtup = ControllerSettings.LWTMessageConnect;
  discovery->lwtdown = ControllerSettings.LWTMessageDisconnect;
  discovery->qsize = ControllerSettings.MaxQueueDepth;
  discovery->pubinterval = ControllerSettings.MinimalTimeBetweenMessages;
}

bool _P126_get_id(struct DiscoveryStruct *discovery) {
  bool success;
  for (byte x = 0; x < TASKS_MAX; x++) {
    LoadTaskSettings(x);
    int pluginid = Settings.TaskDeviceNumber[x];
    #ifdef _P126_DEBUG
      Serial.print("taskid ");
      Serial.print(String(x));
      Serial.print(" : plugin : ");
      Serial.println(String(pluginid));
    #endif

    if (pluginid == PLUGIN_ID_126) {
      discovery->taskid = x;
      _P126_log = F("P[126] found correct taskid: ");
      _P126_log += String(x);
      addLog(LOG_LEVEL_DEBUG,_P126_log);
      success = true;
      break;
    } else {
      success = false;
    }
  }

  if (!success) {
    _P126_log = F("P[126] couldn't fetch taskid");
    addLog(LOG_LEVEL_ERROR,_P126_log);
  }

  return success;
}
//---------------------------------------------------------------------------//
// debugging functions, to be removed, if moving from playground
void _P126_debug(struct DiscoveryStruct *discovery) {
  String debug;

  _P126_make_classes();

  debug = F("discovery.taskid : ");
  debug += String(discovery->taskid);
  debug += F("\n");

  debug += F("discovery->ctrlid : ");
  debug += String(discovery->ctrlid);
  debug += F("\n");

  debug += F("discovery->publish : ");
  debug += String(discovery->publish);
  debug += F("\n");

  debug += F("discovery->lwttopic : ");
  debug += String(discovery->lwttopic);
  debug += F("\n");

  debug += F("discovery->lwtup : ");
  debug += String(discovery->lwtup);
  debug += F("\n");

  debug += F("discovery->lwtdown : ");
  debug += String(discovery->lwtdown);
  debug += F("\n");

  debug += F("discovery->qsize : ");
  debug += String(discovery->qsize);
  debug += F("\n");

  debug += F("discovery->pubinterval : ");
  debug += String(discovery->pubinterval);
  debug += F("\n");

  debug += F("discovery->save.init : ");
  debug += String(discovery->save.init);
  debug += F("\n");

  debug += F("discovery->save.prefix : ");
  debug += String(discovery->save.prefix);
  debug += F("\n");

  debug += F("discovery->save.custom : ");
  debug += String(discovery->save.custom);
  debug += F("\n");

  debug += F("discovery->save.usename : ");
  debug += String(discovery->save.usename);
  debug += F("\n");

  debug += F("discovery->save.usecustom : ");
  debug += String(discovery->save.usecustom);
  debug += F("\n");

  debug += F("discovery->save.usetask : ");
  debug += String(discovery->save.usetask);
  debug += F("\n");

  debug += F("discovery->save.usevalue : ");
  debug += String(discovery->save.usevalue);
  debug += F("\n");

  debug += F("discovery->save.moved : ");
  debug += String(discovery->save.moved);
  debug += F("\n");

  String topic = F("debug/");
  topic += String(Settings.Name);

  // Serial.println(debug);
  _P126_publish(discovery->ctrlid, topic, debug);

  for (byte i = 0; i < TASKS_MAX; i++) {
    debug = F("discovery->save.task[");
    debug += String(i);
    debug += F("].enable : ");
    debug += toString(discovery->save.task[i].enable);
    debug += F("\n");
    for (byte j = 0; j < VARS_PER_TASK; j++) {
      debug += F("discovery->save.task[");
      debug += String(i);
      debug += F("].value[");
      debug += String(j);
      debug += F("].enable : ");
      debug += toString(discovery->save.task[i].value[j].enable);
      debug += F("\n");
      
      debug += F("discovery->save.task[");
      debug += String(i);
      debug += F("].value[");
      debug += String(j);
      debug += F("].type : ");
      debug += String(discovery->save.task[i].value[j].type);
      debug += F(" : ");
      debug += _P126_class[discovery->save.task[i].value[j].type];
      debug += F("\n");
      
      debug += F("discovery->save.task[");
      debug += String(i);
      debug += F("].value[");
      debug += String(j);
      debug += F("].unit : ");
      debug += String(discovery->save.task[i].value[j].unit);
      debug += F(" : ");

      switch (discovery->save.task[i].value[j].type) {
        case 0:   //sensor
          debug += _P126_unit[discovery->save.task[i].value[j].unit];
          break;
        case 1:   //battery (same logic as humidity)
        case 2:   //humidity
          debug += F("%");
          break;
        case 3:   //illuminance
          debug += _P126_lightunit[discovery->save.task[i].value[j].unit];
          break;
        case 4:   //temp
          debug += _P126_tempunit[discovery->save.task[i].value[j].unit];
          break;
        case 5:   //pressure
          debug += _P126_pressunit[discovery->save.task[i].value[j].unit];
          break;
        case 6:   //plant
          debug += F("mdi:leaf : ");
          debug += F("%");
          break;
        case 7:   //RF
          debug += F("mdi:radio-tower");
          break;
        case 8:   //scale
          debug += F("mdi:scale : ");
          debug += _P126_loadunit[discovery->save.task[i].value[j].unit];
        case 9:
        case 10:
          break;
        case 33:    //water
          debug += F("mdi:cup-water");
          break;
        default:
          break;
      }
      debug += F("\n");

    }

      topic = F("debug/");
      topic += String(Settings.Name);
      topic += F("/task");
      topic += String(i);

      //Serial.println(debug);
      _P126_publish(discovery->ctrlid, topic, debug);
  } 

}
void _P126_debug2(String log){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.println(log);
  #endif
}
void _P126_debug2(String log, String value){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.print(log);
  Serial.print(F(" : "));
  Serial.println(value);
  #endif
}
void _P126_debug2(char* log){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.println(String(log));
  #endif
}
void _P126_debug2(String log, char* value){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.print(log);
  Serial.print(F(" : "));
  Serial.println(String(value));
  #endif
}
void _P126_debug2(bool log){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.println(toString(log));
  #endif
}
void _P126_debug2(String log, bool value){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.print(log);
  Serial.print(F(" : "));
  Serial.println(toString(value));
  #endif
}
void _P126_debug2(int log){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.println(String(log));
  #endif
}
void _P126_debug2(String log, int value){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.print(log);
  Serial.print(F(" : "));
  Serial.println(String(value));
  #endif
}
void _P126_debug2(byte log){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.println(String(log));
  #endif
}
void _P126_debug2(String log, byte value){
  #ifdef _P126_DEBUG
  Serial.print(F("[P126] "));
  Serial.print(log);
  Serial.print(F(" : "));
  Serial.println(String(value));
  #endif
}
//#endif


