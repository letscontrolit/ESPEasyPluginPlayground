//=======================================================================================//
//                                                                                       //
//                              HOMEASSISTANT DISCOVERY                                  //
//                                                                                       //
//=======================================================================================//
/* by michael baeck
    plugin searches for active mqtt-enabled taskvalues and pushes json payloads 
    for homeassistant to take for discovery.

    [!] MAJOR ISSUE:  by espeasy default MQTT_MAX_PACKET_SIZE in pubsubclient.h is set to 384
    [!]               most payloads will be 500-600
    [!]               section in lib\pubsubclient\src\PubSubClient.h:
                      // MQTT_MAX_PACKET_SIZE : Maximum packet size
                      #ifndef MQTT_MAX_PACKET_SIZE
                      #define MQTT_MAX_PACKET_SIZE 384 // need to fix this here, because this define cannot be overruled within the Arduino sketch...
                      #endif

    1 value = 1 entity. 
    whole unit (ESP) = 1 device & 1 entity.

    classification of sensor (motion, light, temp, etc.) can be done in webform.

    devices can also be deleted again but homeassistant refuses to remove them from its store.
    (but it behaves the same when I publish the messages manually...)

    this is my first arduino code and first commit ever to anything, but it's pretty useful already.
    testing and feedback highly appreciated!


    only tested on a couple of 4M ESP12 yet, ESP01 and ESP32 soon. 
    storage might be short on ESP32 as struct reserves space for all tasks

    [*] stay tuned for more plugins like homeassistant-switch, etc

    supported components
      custom                  //[todo]
        binary_sensor
          [x] water
          [ ] door
        sensor
          [x] plant
          [x] radio
          [x] scale
      homeassistant native
        [X] binary_sensor     [1/0]
              battery           [low/norm]
              cold              [cold/norm]
              connectivity      [conn/disco]
              door              [open/close]
              garage_door       [open/close]
              gas               [gas/clear]
              heat              [hot/norm]
              light             [light/dark]
              lock              [unlocked/locked]
              moisture          [wet/dry]
              motion            [motion/clear]
              moving            [moving/stopped]
              occupancy         [occ/clear]
              opening           [open/close]
              plug              [plug/unplug]
              power             [power/none]
              presesnce         [home/away]
              problem           [problem/ok]
              safety            [unsafe/safe]
              smoke             [smoke/clear]
              sound             [sound/clear]
              vibration         [vibration/clear]
              window            [open/close]
        [X] sensor
              battery
              humimdity
              illuminance
                [ ] unit_of_measurement lx or lm //[todo]
              temperature
                [ ] unit_of_measurement °C or °F //[todo]
              ppressure
                [ ] unit_of_measurement hPa or mbar //[todo]
        [ ] camera
        [ ] cover
              damper
              garage
              window
        [ ] fan
        [ ] climate
        [ ] light
        [ ] lock
        [ ] switch
    
    naming conventions
      entity_id               COMPONENT.SYSNAME_TASK_VALUE  (adjustable)

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
      [?] assign unit by Pxxx

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

//=======================================================================================//
  // store these values
  // prefix
  // task[12]
    // value[4]
      // bool enable
      // byte option

//#ifdef USES_P126
//#ifdef PLUGIN_BUILD_DEVELOPMENT
//#ifdef PLUGIN_BUILD_TESTING
//#define PLUGIN_119_DEBUG  true

#define PLUGIN_126
#define PLUGIN_ID_126     126     
#define PLUGIN_NAME_126   "Generic - Homeassistant Discovery [DEVELOPMENT]"  

//#define _P126_DEBUG
#define PLUGIN_126_VERSION 47
#define _P126_INTERVAL 5000
bool _P126_cleanup = false;
unsigned long _P126_time;
byte _P126_increment;
String _P126_log;

struct DiscoveryStruct {
  int taskid;
  int ctrlid;
  String publish;
  String lwttopic;
  String lwtup;
  String lwtdown;
  int qsize;
  int pubinterval;
  struct savestruct {
    byte init;
    char prefix[21];
    char custom[41];
    bool usename;
    bool usecustom;
    bool usetask;
    bool usevalue;
    bool moved;
    struct taskstruct {
      bool enable;
      struct valuestruct {
        bool enable;
        byte option;
        byte unit;
      } value[4];
    } task[TASKS_MAX];
  } save;
};

//=======================================================================================//
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

      //=======================================================================================//
      // load user setting
        struct DiscoveryStruct discovery;
          discovery.ctrlid = 0;
          discovery.pubinterval = 0;
          discovery.qsize = 0;
          discovery.save.init = 0;

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

        #ifdef _P126_DEBUG
          Serial.print(F("loaded struct for TaskIndex : "));
          Serial.println(String(event->TaskIndex));
          Serial.println(F("--------------- LOAD ---------------------------"));
          Serial.print(F("discovery.save.init : "));
          Serial.println(String(discovery.save.init));  
          Serial.print(F("discovery.save.prefix "));
          Serial.println(String(discovery.save.prefix));
          Serial.print(F("discovery.save.custom "));
          Serial.println(String(discovery.save.custom));
          for (byte i = 0; i < TASKS_MAX; i++) {
            for (byte j = 0; j < 4; j++) {
              Serial.print(F("TASK : "));
              Serial.print(String(i));
              Serial.print(F(" - VALUE : "));
              Serial.print(String(j));
              Serial.print(F(" - ENABLE : "));
              Serial.print(String(discovery.save.task[i].value[j].enable));
              Serial.print(F(" - OPTION : "));
              Serial.print(String(discovery.save.task[i].value[j].option));
              Serial.print(F(" - UNIT : "));
              Serial.println(String(discovery.save.task[i].value[j].unit));
            }
          }
        #endif

        String errnote = F("<br>");
        if (discovery.save.init != PLUGIN_126_VERSION) {
          errnote = F("<font color=\"red\">Loaded settings with ID ");
          errnote += String(discovery.save.init);
          errnote += F(" not matching v");
          errnote += String(PLUGIN_126_VERSION);
          errnote += F(". Loading default settings.</font><br><br>");        

          strncpy(discovery.save.prefix, "homeassistant", sizeof(discovery.save.prefix));
          strncpy(discovery.save.custom, "customstring", sizeof(discovery.save.custom));
          discovery.save.usecustom = false;
          discovery.save.usename = true;
          discovery.save.usetask = true;
          discovery.save.usevalue = true;
          for (byte i = 0; i < TASKS_MAX; i++) {
            discovery.save.task[i].enable = false;
            for (byte j = 0; j < 4; j++) {
              discovery.save.task[i].value[j].enable = false;
              discovery.save.task[i].value[j].option = 10;
              discovery.save.task[i].value[j].unit = 0;
            }
          } 
        }        
      //=======================================================================================//
    

      //=======================================================================================//
      // introduction
        html_BR();
        tmpnote += errnote;
        tmpnote += F("This plugin will push sensor configs that are compliant to home-assistant's discovery feature.");
        tmpnote += F("<br>There will be one additional sensor for the ESP node itself which contains system info like wifi-signal, ip-address, etc.");
        tmpnote += F(" Interval setting on bottom defines update frequency of that sensor. Set to 0 to disable.");
        addFormNote(tmpnote);
      //=======================================================================================//
      

      //=======================================================================================//
      // global settings
        addFormSubHeader(F("GLOBAL SETTINGS"));
        
        addFormTextBox(String(F("discovery prefix ")), String(F("Plugin_126_prefix")), discovery.save.prefix, 20);
        addFormNote(F("Change discovery topic here, if done in home-assistant. Defaults to \"homeassistant\"."));
        
        html_BR();

        addFormCheckBox(String(F("include sysname")), String(F("Plugin_126_usename")), discovery.save.usename);
        addFormCheckBox(String(F("include custom")), String(F("Plugin_126_usecustom")), discovery.save.usecustom);
        addFormCheckBox(String(F("include taskname")), String(F("Plugin_126_usetask")), discovery.save.usetask);
        //addFormCheckBox(String(F("use value name")), String(F("Plugin_126_usevalue")), discovery.save.usevalue);
        //if (discovery.save.usecustom) {
        addFormTextBox(String(F("custom string ")), String(F("Plugin_126_custom")), discovery.save.custom, 40);
        addFormCheckBox(String(F("move custom to end")), String(F("Plugin_126_moved")), discovery.save.moved);
        //}

        html_BR();

        tmpnote = F("Adjust how the entity_ids in homeassistant are named.");
        tmpnote += F(" Click submit to preview your settings before publishing.");
        addFormNote(tmpnote);

        //=======================================================================================//
        // show example entity_id based on settings
          bool b1 = discovery.save.usename;
          String s1 = Settings.Name;
          bool b2 = discovery.save.usecustom;
          String s2 = discovery.save.custom;
          bool b3 = discovery.save.usetask;
          String s3 = F("%tskname%");
          bool b4 = true; //discovery.save.usevalue;
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

          #ifdef _P126_DEBUG
            Serial.print(F("customstr(String()) "));
            Serial.println(String(discovery.save.custom));
            String tmpcustom = String(discovery.save.custom);
            Serial.print(F("customstr(String tmpcustom) "));
            Serial.println(tmpcustom);
            Serial.print(F("customstr(char) "));
            Serial.println(discovery.save.custom);
          #endif

          // if (discovery.save.custom == "") {
          //   Serial.print(F("custom string empty turning false "));
          //   b2 = false;
          //   Serial.print(F("b2 usecustom "));
          //   Serial.println(toString(b2));
          // }
        //=======================================================================================//

      //=======================================================================================//


      //=======================================================================================//
      // device settings
        int msgcount = find_sensors(&discovery);
        if (Settings.TaskDeviceTimer[event->TaskIndex] > 0) msgcount++;
      //=======================================================================================//
    

      //=======================================================================================//
      // command buttons
        addFormSubHeader(F("CONTROL"));

        tmpnote = F("<br>Configs will NOT be pushed automatically. Once plugin is saved, you can use the buttons below.<br>");
        tmpnote += F("You can trigger actions via these commands as well: \"<b>discovery,update</b>\", \"<b>discovery,delete</b>\" and \"<b>discovery,cleanup</b>\" ");
        addFormNote(tmpnote);

        //html_BR();

        if (Settings.TaskDeviceEnabled[event->TaskIndex] && !_P126_cleanup) {
          html_BR();
          
          addButton(F("/tools?cmd=discovery%2Cupdate"), "Update Configs");
          addButton(F("/tools?cmd=discovery%2Cdelete"), "Delete Configs");
          addButton(F("/tools?cmd=discovery%2Ccleanup"), F("Full cleanup"));
          html_BR();

          int duration = TASKS_MAX * 2 * _P126_INTERVAL /1000/60;
          int payloads = TASKS_MAX * 2 * 4;
          tmpnote = F("<br><font color=\"red\">Warning! Cleanup will send ");
          tmpnote += String(payloads);
          tmpnote += F(" empty payloads in about ");
          tmpnote += String(duration);
          tmpnote += F(" minutes. This might slow down or even crash the unit.</font>");
          addFormNote(tmpnote);
        }

        if (_P126_cleanup) {
          html_BR();
          int duration = (TASKS_MAX * 2 - (_P126_increment + 1)) * _P126_INTERVAL / 1000;
          String message = F("<font color=\"red\">CLEANUP IN PROGESS!</font> Remaining time: about ");
          message += String(duration);
          message += F(" seconds");
          addHtml(message);
        }
      //=======================================================================================//


      //=======================================================================================//
      // notes
        addFormSubHeader(F("NOTES"));

        get_ctrl(&discovery);

        tmpnote = F("<br>- On update, <font color=\"#07D\">");
        tmpnote += String(msgcount);
        tmpnote += F("</font> messages will be published.");

        if (discovery.qsize < 2 * msgcount) {
          tmpnote += F("Your message queue is set to: <font color=\"#07D\">");
          tmpnote += String(discovery.qsize);
          tmpnote += F("</font>");
          tmpnote += F(". Consider increasing it, if not all configs reach homeassistant.");
        }

        if (!Settings.MQTTRetainFlag) {
          tmpnote += F("<br>- Retain option is not set. It's highly advised to set it in advanced settings.");
        }

        if (discovery.pubinterval < 500) {
          tmpnote += F("<br>- Message interval is set to <font color=\"#07D\">");
          tmpnote += String(discovery.pubinterval);
          tmpnote += F("</font>ms. Consider increasing it, if not all configs reach homeassistant.");
        }
        
        tmpnote += F("<br>- ESPEasy limits mqtt message size to <font color=\"#07D\">");
        tmpnote += String(MQTT_MAX_PACKET_SIZE);
        tmpnote += F("</font>, check logs, to see if this affects you.");

        tmpnote += F("<br>- Tasks must have a MQTT controller enabled to show up.");

        tmpnote += F("<br>- Don't forget to submit this form, before using commands.");

        tmpnote += F("<br>- New sensors will be added immediately to homeassistant.");
        tmpnote += F(" Changes and deletions require homeassistant to be rebooted.");

        tmpnote += F("<br><br>v");
        tmpnote += PLUGIN_126_VERSION;
        tmpnote += F("; struct size: ");
        tmpnote += String(sizeof(discovery));
        tmpnote += F("Byte memory / ");
        tmpnote += String(sizeof(discovery.save));
        tmpnote += F("Byte flash");

        addFormNote(tmpnote);
      //=======================================================================================//

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      //=======================================================================================//
      // init settings struct
        struct DiscoveryStruct discovery;

        #ifdef _P126_DEBUG
          Serial.println(F("--------------- WEBSAVE INIT ---------------------------"));
          Serial.println(F("Filling empty struct"));
        #endif
        discovery.save.init = PLUGIN_126_VERSION;
        strncpy(discovery.save.prefix, "homeassistant", sizeof(discovery.save.prefix));
        discovery.save.usecustom = false;
        discovery.save.usename = true;
        discovery.save.usetask = true;
        discovery.save.usevalue = true;
        discovery.save.moved = false;
        for (byte i = 0; i < TASKS_MAX; i++) {
          discovery.save.task[i].enable = false;
          for (byte j = 0; j < 4; j++) {
            discovery.save.task[i].value[j].enable = false;
            discovery.save.task[i].value[j].option = 10;
            discovery.save.task[i].value[j].unit = 0;
          }
        } 
      //=======================================================================================//


      //=======================================================================================//
      // copy settings from webserver to settings struct
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

        find_sensors(&discovery, true);
      //=======================================================================================//


      //=======================================================================================//
      // save user settings to flash
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));
          #ifdef _P126_DEBUG
            Serial.println(F("saved discovery to flash "));
            Serial.println(F("-----------------------------------------------------"));
          #endif
      //=======================================================================================//


      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      struct DiscoveryStruct discovery;
      LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));
      
      if (discovery.save.init != PLUGIN_126_VERSION) {
        addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
        success = false;
      } else {
        get_ctrl(&discovery);
        success = system_state(&discovery, false);
      }

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

        if (_P126_cleanup) {
          addLog(LOG_LEVEL_ERROR, F("[P126] cleanup in progress, ignoring command"));
        } else {
          //=======================================================================================//
          // load settings
            struct DiscoveryStruct discovery;
            get_id(&discovery);   // search for taskid (not given with event)
            LoadCustomTaskSettings(discovery.taskid, (byte*)&discovery.save, sizeof(discovery.save));

            if (discovery.save.init != PLUGIN_126_VERSION) {
              addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
              break;
            } 

            get_ctrl(&discovery);
          //=======================================================================================//

          argIndex = string.lastIndexOf(',');
          tmpString = string.substring(argIndex + 1);

          if (tmpString.equalsIgnoreCase(F("update"))) {

            system_config(&discovery, false);
            sensor_config(&discovery, false);
            system_state(&discovery, false);
          } 
          else if (tmpString.equalsIgnoreCase(F("delete"))) {
            delete_configs(&discovery);
          }
          else if (tmpString.equalsIgnoreCase(F("cleanup"))) {
            int duration = TASKS_MAX * 2 * _P126_INTERVAL / 1000 / 60;
            _P126_log = F("[P126] starting cleanup; expected duration is: ");
            _P126_log += String(duration);
            _P126_log += F(" minutes");
            addLog(LOG_LEVEL_INFO, _P126_log);

            _P126_cleanup = true;   // enable cleanup-mode
            _P126_time = millis();
            _P126_increment = 0;

            delete_system(&discovery);   // run once

            cleanup(&discovery);  // run first time
          }
        }
      }
      break;
    }

  	case PLUGIN_EXIT:
  	{
      //get_ctrl(ctrlid, ctrldat);
      //success = delete_configs(event->TaskIndex, ctrlid, ctrldat);
  	  //perform cleanup tasks here. For example, free memory

  	  break;

  	}

    case PLUGIN_ONCE_A_SECOND:
    {
      if (_P126_cleanup && timeOutReached(_P126_time + _P126_INTERVAL)) {
        _P126_time = millis();

        struct DiscoveryStruct discovery;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

        if (discovery.save.init != PLUGIN_126_VERSION) {
          addLog(LOG_LEVEL_ERROR, F("[P126] no valid settings found!"));
          break;
        } 

        get_ctrl(&discovery);

        cleanup(&discovery);  // run 2nd to nth time

        if (_P126_increment >= TASKS_MAX * 2 - 1) {
          _P126_cleanup = false;
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

//=======================================================================================//
int find_sensors(struct DiscoveryStruct *discovery) {
  int msgcount = find_sensors(discovery, false);
  return msgcount;
}

#define _P126_CLASSCOUNT 35
int find_sensors(struct DiscoveryStruct *discovery, bool save) {
  _P126_log = F("P[126] search active tasks: state of save is : ");
  _P126_log += String(save);
  addLog(LOG_LEVEL_DEBUG,_P126_log);
  // Serial.println(_P126_log);

  int msgcount = 0;

  String sensorclass[_P126_CLASSCOUNT];
    sensorclass[0] = F("------SENSOR-------");        //  %   raw   K
    sensorclass[1] = F("battery");                    //  %   
    sensorclass[2] = F("humidity");                   //  %
    sensorclass[3] = F("illuminance");                //  lx  lm
    sensorclass[4] = F("temperature");                //  °C  °F
    sensorclass[5] = F("pressure");                   //  hPa mbar
    sensorclass[6] = F("plant");                      //  %
    sensorclass[7] = F("radio/RF");                      //  none
    sensorclass[8] = F("scale/load/weight");          //  kg

    sensorclass[10] = F("---BINARY_SENSOR---");
    sensorclass[11] = F("battery"); 
    sensorclass[12] = F("cold");
    sensorclass[13] = F("connectivity");
    sensorclass[14] = F("door");
    sensorclass[15] = F("garage_door");
    sensorclass[16] = F("gas");
    sensorclass[17] = F("heat");
    sensorclass[18] = F("light");
    sensorclass[19] = F("lock");
    sensorclass[20] = F("moisture");
    sensorclass[21] = F("motion");
    sensorclass[22] = F("moving");
    sensorclass[23] = F("occupancy");
    sensorclass[24] = F("opening");
    sensorclass[25] = F("plug");
    sensorclass[26] = F("power");
    sensorclass[27] = F("presence");
    sensorclass[28] = F("problem");
    sensorclass[29] = F("safety");
    sensorclass[30] = F("smoke");
    sensorclass[31] = F("sound");
    sensorclass[32] = F("vibration");
    sensorclass[33] = F("water");
    sensorclass[34] = F("window");

  String sensorunit[9];
    sensorunit[0] = F("raw");
    sensorunit[1] = F("%");
    sensorunit[2] = F("K");     // Kelvin
    sensorunit[3] = F("dB");
    sensorunit[4] = F("V");     // Volt
    sensorunit[5] = F("A");     // Ampere
    sensorunit[6] = F("h");     // hours
    sensorunit[7] = F("m");     // minutes
    sensorunit[8] = F("s");     // seconds

  String lightunit[2];
    lightunit[0] = F("lx");
    lightunit[1] = F("lm");
  String tempunit[2];
    tempunit[0] = F("°C");
    tempunit[1] = F("°F");
  String loadunit[4];
    loadunit[0] = F("g");
    loadunit[1] = F("kg");
    loadunit[2] = F("lb");
    loadunit[3] = F("raw");
  String pressunit[2];
    pressunit[0] = F("hPa");
    pressunit[1] = F("mbar");

  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;
  byte lastActiveTaskIndex = 0;

  if (save) {
    lastActiveTaskIndex = lastTaskIndex;
  } else {
    for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {            // find last task
      if (Settings.TaskDeviceNumber[TaskIndex]) lastActiveTaskIndex = TaskIndex;
    }
  }
  
  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastActiveTaskIndex; TaskIndex++) {      // for each task
    //discovery->save.task[TaskIndex].enable = false;

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
            _P126_log = F("P[126] found active task with id : ");
            _P126_log += String(TaskIndex);
            addLog(LOG_LEVEL_DEBUG,_P126_log);
            // Serial.println(_P126_log);
            
            if (!save) {
              String header = F("Task ");
              header += String(TaskIndex + 1);
              header += F(" - ");
              header += getPluginNameFromDeviceIndex(DeviceIndex);
              addFormSubHeader(header);
            }

            for (byte x = 0; x < 4; x++) {                           // for each value
              if (x < Device[DeviceIndex].ValueCount) {
                if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() > 0) {
                  _P126_log = F("P[126] found value with name : ");
                  _P126_log += String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                  addLog(LOG_LEVEL_DEBUG,_P126_log);
                  // Serial.println(_P126_log);
                  
                  msgcount++;

                  String enableid = F("P126_");
                  enableid += String(TaskIndex);
                  enableid += F("_");
                  enableid += String(x);
                  enableid += F("_enable");

                  String optionid = F("P126_");
                  optionid += String(TaskIndex);
                  optionid += F("_");
                  optionid += String(x);
                  optionid += F("_option");

                  String unitid = F("P126_");
                  optionid += String(TaskIndex);
                  optionid += F("_");
                  optionid += String(x);
                  optionid += F("_unit");

                  if (!save) {  // if loading webform
                    // add value header                  
                      String valuename = F("<font color=\"#07D\"><b>");
                      valuename += String(ExtraTaskSettings.TaskDeviceName);
                      valuename += F(" ");
                      valuename += String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                      valuename += F("</b></font>");
                      
                      html_TR_TD();
                      addHtml(valuename);
                    // add full entity_id
                      bool b1 = discovery->save.usename;
                      String s1 = Settings.Name;
                      bool b2 = discovery->save.usecustom;
                      String s2 = discovery->save.custom;
                      bool b3 = discovery->save.usetask;
                      String s3 = ExtraTaskSettings.TaskDeviceName;
                      bool b4 = discovery->save.usevalue;
                      String s4 = ExtraTaskSettings.TaskDeviceValueNames[x];
                      if (String(discovery->save.custom).length() == 0) b2 = false;
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
                      if (discovery->save.task[TaskIndex].value[x].option < 10) {
                        entity_id += F("sensor.");
                      } else {
                        entity_id += F("binary_sensor.");
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

                      if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() == 0) {
                        addFormNote(F("No valuename set in device options!"));
                      }

                    // add user input
                      addFormSelector(F("type"), optionid, _P126_CLASSCOUNT, sensorclass, NULL, discovery->save.task[TaskIndex].value[x].option);

                      switch (discovery->save.task[TaskIndex].value[x].option) {
                        case 0:
                          addFormSelector(F("unit"), unitid, 9, sensorunit, NULL, discovery->save.task[TaskIndex].value[x].unit);
                          break;
                        case 3:
                          addFormSelector(F("unit"), unitid, 2, lightunit, NULL, discovery->save.task[TaskIndex].value[x].unit);
                          break;
                        case 4:
                          addFormSelector(F("unit"), unitid, 2, tempunit, NULL, discovery->save.task[TaskIndex].value[x].unit);
                          break;
                        case 5:
                          addFormSelector(F("unit"), unitid, 2, pressunit, NULL, discovery->save.task[TaskIndex].value[x].unit);
                          break;
                        case 8:
                          addFormSelector(F("unit"), unitid, 4, loadunit, NULL, discovery->save.task[TaskIndex].value[x].unit);
                          break;
                      }

                      addFormCheckBox(F("enable"), enableid, discovery->save.task[TaskIndex].value[x].enable);

                      html_BR();

                  } else {      // if saving webform
                    int defaultoption;
                    if (DeviceIndex == 1) {
                      defaultoption = 10;
                    } else {
                      defaultoption = 0;
                    }
                    _P126_log = F("P[126] saving sensorclass : ");
                    _P126_log += sensorclass[getFormItemInt(optionid, defaultoption)];
                    addLog(LOG_LEVEL_DEBUG,_P126_log);

                    if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() == 0) {
                      discovery->save.task[TaskIndex].value[x].enable = false;
                    } else {
                      discovery->save.task[TaskIndex].value[x].enable = isFormItemChecked(enableid);
                      if (discovery->save.task[TaskIndex].value[x].enable) discovery->save.task[TaskIndex].enable = true;
                    }

                    discovery->save.task[TaskIndex].value[x].option = getFormItemInt(optionid, defaultoption);

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

bool system_config(struct DiscoveryStruct *discovery, bool brief) {

  /*  home-assistant parameters
    'aux_cmd_t':           'aux_command_topic',
    'aux_stat_tpl':        'aux_state_template',
    'aux_stat_t':          'aux_state_topic',
    'avty_t':              'availability_topic',
    'away_mode_cmd_t':     'away_mode_command_topic',
    'away_mode_stat_tpl':  'away_mode_state_template',
    'away_mode_stat_t':    'away_mode_state_topic',
    'bri_cmd_t':           'brightness_command_topic',
    'bri_scl':             'brightness_scale',
    'bri_stat_t':          'brightness_state_topic',
    'bri_val_tpl':         'brightness_value_template',
    'clr_temp_cmd_t':      'color_temp_command_topic',
    'clr_temp_stat_t':     'color_temp_state_topic',
    'clr_temp_val_tpl':    'color_temp_value_template',
    'cmd_t':               'command_topic',
    'curr_temp_t':         'current_temperature_topic',
    'dev_cla':             'device_class',
    'fx_cmd_t':            'effect_command_topic',
    'fx_list':             'effect_list',
    'fx_stat_t':           'effect_state_topic',
    'fx_val_tpl':          'effect_value_template',
    'exp_aft':             'expire_after',
    'fan_mode_cmd_t':      'fan_mode_command_topic',
    'fan_mode_stat_tpl':   'fan_mode_state_template',
    'fan_mode_stat_t':     'fan_mode_state_topic',
    'frc_upd':             'force_update',
    'hold_cmd_t':          'hold_command_topic',
    'hold_stat_tpl':       'hold_state_template',
    'hold_stat_t':         'hold_state_topic',
    'ic':                  'icon',
    'init':                'initial',
    'json_attr':           'json_attributes',
    'max_temp':            'max_temp',
    'min_temp':            'min_temp',
    'mode_cmd_t':          'mode_command_topic',
    'mode_stat_tpl':       'mode_state_template',
    'mode_stat_t':         'mode_state_topic',
    'name':                'name',
    'on_cmd_type':         'on_command_type',
    'opt':                 'optimistic',
    'osc_cmd_t':           'oscillation_command_topic',
    'osc_stat_t':          'oscillation_state_topic',
    'osc_val_tpl':         'oscillation_value_template',
    'pl_arm_away':         'payload_arm_away',
    'pl_arm_home':         'payload_arm_home',
    'pl_avail':            'payload_available',
    'pl_cls':              'payload_close',
    'pl_disarm':           'payload_disarm',
    'pl_hi_spd':           'payload_high_speed',
    'pl_lock':             'payload_lock',
    'pl_lo_spd':           'payload_low_speed',
    'pl_med_spd':          'payload_medium_speed',
    'pl_not_avail':        'payload_not_available',
    'pl_off':              'payload_off',
    'pl_on':               'payload_on',
    'pl_open':             'payload_open',
    'pl_osc_off':          'payload_oscillation_off',
    'pl_osc_on':           'payload_oscillation_on',
    'pl_stop':             'payload_stop',
    'pl_unlk':             'payload_unlock',
    'pow_cmd_t':           'power_command_topic',
    'ret':                 'retain',
    'rgb_cmd_tpl':         'rgb_command_template',
    'rgb_cmd_t':           'rgb_command_topic',
    'rgb_stat_t':          'rgb_state_topic',
    'rgb_val_tpl':         'rgb_value_template',
    'send_if_off':         'send_if_off',
    'set_pos_tpl':         'set_position_template',
    'set_pos_t':           'set_position_topic',
    'spd_cmd_t':           'speed_command_topic',
    'spd_stat_t':          'speed_state_topic',
    'spd_val_tpl':         'speed_value_template',
    'spds':                'speeds',
    'stat_clsd':           'state_closed',
    'stat_off':            'state_off',
    'stat_on':             'state_on',
    'stat_open':           'state_open',
    'stat_t':              'state_topic',
    'stat_val_tpl':        'state_value_template',
    'swing_mode_cmd_t':    'swing_mode_command_topic',
    'swing_mode_stat_tpl': 'swing_mode_state_template',
    'swing_mode_stat_t':   'swing_mode_state_topic',
    'temp_cmd_t':          'temperature_command_topic',
    'temp_stat_tpl':       'temperature_state_template',
    'temp_stat_t':         'temperature_state_topic',
    'tilt_clsd_val':       'tilt_closed_value',
    'tilt_cmd_t':          'tilt_command_topic',
    'tilt_inv_stat':       'tilt_invert_state',
    'tilt_max':            'tilt_max',
    'tilt_min':            'tilt_min',
    'tilt_opnd_val':       'tilt_opened_value',
    'tilt_status_opt':     'tilt_status_optimistic',
    'tilt_status_t':       'tilt_status_topic',
    't':                   'topic',
    'uniq_id':             'unique_id',
    'unit_of_meas':        'unit_of_measurement',
    'val_tpl':             'value_template',
    'whit_val_cmd_t':      'white_value_command_topic',
    'whit_val_stat_t':     'white_value_state_topic',
    'whit_val_tpl':        'white_value_template',
    'xy_cmd_t':            'xy_command_topic',
    'xy_stat_t':           'xy_state_topic',
    'xy_val_tpl':          'xy_value_template',
  */

  //  create topics
    //  prefix/component/device/entity/[config|state]
    //  %prefix% / %component% / %mac% / %mac%_system / [config|state]
    String discovery_topic = make_topic(discovery->save.prefix, F("sensor"), make_unique());

    String state_topic = discovery_topic;
    state_topic.replace(F("/config"),F("/state"));

  //  create discovery payload
    String payload;
    if (Settings.TaskDeviceTimer[discovery->taskid] == 0) {
      payload = "";
    } else {
      payload = F("{");
      payload += add_line(F("name"), String(Settings.Name));
      payload += add_line(F("ic"), F("mdi:chip"));
      payload += add_line(F("stat_t"), state_topic);
      payload += add_line(F("val_tpl"), F("{{ value_json.state }}"));
      payload += add_line(F("uniq_id"), make_unique());
      payload += add_line(F("avty_t"), discovery->lwttopic);
      payload += add_line(F("pl_avail"), discovery->lwtup);
      payload += add_line(F("pl_not_avail"), discovery->lwtdown);
      if (brief) {
        payload += F("\"json_attr\":[\"plugin\",\"unit\",\"uptime\",\"ip\",\"rssi\"],");
      } else {
        payload += F("\"json_attr\":[\"plugin\",\"unit\",\"version\",\"uptime\",\"cpu\",\"hostname\",\"ip\",\"mac\",\"ssid\",\"bssid\",\"rssi\",\"last_disconnect\",\"last_boot_cause\"],");
      }
      payload += add_device(brief);
      payload += F("}");
        _P126_log = F("P[126] payload created : ");
        _P126_log += payload;
        addLog(LOG_LEVEL_DEBUG,_P126_log);
    }

  //  check payload size and publish                     
    if (check_length(discovery_topic, payload)) {
      return (publish(discovery->ctrlid, discovery_topic, payload));
    } else if (!brief) {
      addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying with reduced content."));
      return (system_config(discovery, true)); // start over with brief-flag
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

bool sensor_config(struct DiscoveryStruct *discovery, bool brief) {
  bool success = false;

  
  String sensorclass[_P126_CLASSCOUNT];
    sensorclass[0] = F("------SENSOR-------");        //  %   raw   K
    sensorclass[1] = F("battery");                    //  %   
    sensorclass[2] = F("humidity");                   //  %
    sensorclass[3] = F("illuminance");                //  lx  lm
    sensorclass[4] = F("temperature");                //  °C  °F
    sensorclass[5] = F("pressure");                   //  hPa mbar
    sensorclass[6] = F("plant");                      //  %
    sensorclass[7] = F("radio/RF");                      //  none
    sensorclass[8] = F("scale/load/weight");          //  kg

    sensorclass[10] = F("---BINARY_SENSOR---");
    sensorclass[11] = F("battery"); 
    sensorclass[12] = F("cold");
    sensorclass[13] = F("connectivity");
    sensorclass[14] = F("door");
    sensorclass[15] = F("garage_door");
    sensorclass[16] = F("gas");
    sensorclass[17] = F("heat");
    sensorclass[18] = F("light");
    sensorclass[19] = F("lock");
    sensorclass[20] = F("moisture");
    sensorclass[21] = F("motion");
    sensorclass[22] = F("moving");
    sensorclass[23] = F("occupancy");
    sensorclass[24] = F("opening");
    sensorclass[25] = F("plug");
    sensorclass[26] = F("power");
    sensorclass[27] = F("presence");
    sensorclass[28] = F("problem");
    sensorclass[29] = F("safety");
    sensorclass[30] = F("smoke");
    sensorclass[31] = F("sound");
    sensorclass[32] = F("vibration");
    sensorclass[33] = F("water");
    sensorclass[34] = F("window");
  String sensorunit[9];
    sensorunit[0] = F("raw");
    sensorunit[1] = F("%");
    sensorunit[2] = F("K");     // Kelvin
    sensorunit[3] = F("dB");
    sensorunit[4] = F("V");     // Volt
    sensorunit[5] = F("A");     // Ampere
    sensorunit[6] = F("h");     // hours
    sensorunit[7] = F("m");     // minutes
    sensorunit[8] = F("s");     // seconds
  String lightunit[2];
    lightunit[0] = F("lx");
    lightunit[1] = F("lm");
  String tempunit[2];
    tempunit[0] = F("°C");
    tempunit[1] = F("°F");
  String loadunit[4];
    loadunit[0] = F("g");
    loadunit[1] = F("kg");
    loadunit[2] = F("lb");
    loadunit[3] = F("raw");
  String pressunit[2];
    pressunit[0] = F("hPa");
    pressunit[1] = F("mbar");



  byte lastTaskIndex = TASKS_MAX - 1;
  
  for (byte TaskIndex = 0; TaskIndex <= lastTaskIndex; TaskIndex++) {   // for each task

    if (discovery->save.task[TaskIndex].enable) {
      LoadTaskSettings(TaskIndex);

      String tasktopic = String(discovery->publish);
      tasktopic.replace(F("%tskname%"), ExtraTaskSettings.TaskDeviceName);

      byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);

      for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++) {   // for each value

        if (discovery->save.task[TaskIndex].value[x].enable) {
          success = false;
          //=======================================================================================//
          //  get state topic from controller settings
            String state_topic = tasktopic;
            state_topic.replace(F("%valname%"), ExtraTaskSettings.TaskDeviceValueNames[x]);
          //=======================================================================================//

          //=======================================================================================//
          //  define entity_id
            bool b1 = discovery->save.usename;
            String s1 = Settings.Name;
            bool b2 = discovery->save.usecustom;
            String s2 = discovery->save.custom;
            bool b3 = discovery->save.usetask;
            String s3 = ExtraTaskSettings.TaskDeviceName;
            bool b4 = discovery->save.usevalue;
            String s4 = ExtraTaskSettings.TaskDeviceValueNames[x];
            // change order if set
              if (discovery->save.moved) {
                b4 = discovery->save.usecustom;
                s4 = discovery->save.custom;
                b2 = discovery->save.usetask;
                s2 = ExtraTaskSettings.TaskDeviceName;
                b3 = discovery->save.usevalue;
                s3 = ExtraTaskSettings.TaskDeviceValueNames[x];
              }
            //

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
          //=======================================================================================//

          //=======================================================================================//
          //  define discovery topic
            String component;
            if (discovery->save.task[TaskIndex].value[x].option < 10) {
              component = F("sensor");
            } else {
              component = F("binary_sensor");
            }

            String uniquestr = make_unique(TaskIndex, x);                 // eg 00AA11BB22CC_1_1

            String discovery_topic = make_topic(discovery->save.prefix, component, uniquestr);   // eg homeasistant/sensor/00AA11BB22CC/00AA11BB22CC_1_1/config
          //=======================================================================================//

          //=======================================================================================//
          //  create payload
            String payload;

            if (discovery->save.task[TaskIndex].value[x].enable) {              // if enabled, create json

              payload = F("{");
              payload += add_line(F("name"), entity_id);

              //=======================================================================================//
              // assign class/icon/unit
                // every sensor should contain   (class || icon) + unit
                // every binary_sensor should contain   (class || icon)
                // no unit will result in String in HA
                switch (discovery->save.task[TaskIndex].value[x].option) {
                  case 0:   //sensor
                    payload += add_line(F("unit_of_meas"), sensorunit[discovery->save.task[TaskIndex].value[x].unit]);
                    break;
                  case 1:   //battery (same logic as humidity)
                  case 2:   //humidity
                    payload += add_line(F("unit_of_meas"), F("%"));
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                  case 3:   //illuminance
                    payload += add_line(F("unit_of_meas"), lightunit[discovery->save.task[TaskIndex].value[x].unit]);
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                  case 4:   //temp
                    payload += add_line(F("unit_of_meas"), tempunit[discovery->save.task[TaskIndex].value[x].unit]);
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                  case 5:   //pressure
                    payload += add_line(F("unit_of_meas"), pressunit[discovery->save.task[TaskIndex].value[x].unit]);
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                  case 6:   //plant
                    payload += add_line(F("ic"), F("mdi:leaf"));
                    payload += add_line(F("unit_of_meas"), F("%"));
                    break;
                  case 7:   //RF
                    payload += add_line(F("ic"), F("mdi:radio-tower"));
                    break;
                  case 8:   //scale
                    payload += add_line(F("ic"), F("mdi:scale"));
                    payload += add_line(F("unit_of_meas"), loadunit[discovery->save.task[TaskIndex].value[x].unit]);
                  case 9:
                  case 10:
                    break;
                  case 33:    //water
                    payload += add_line(F("ic"), F("mdi:cup-water"));
                    break;
                  default:
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                }
              //=======================================================================================//

              payload += add_line(F("stat_t"), state_topic);
              payload += add_line(F("uniq_id"), uniquestr);
              payload += add_line(F("avty_t"), discovery->lwttopic);
              payload += add_line(F("pl_avail"), discovery->lwtup);
              payload += add_line(F("pl_not_avail"), discovery->lwtdown);
              if (!brief) payload += add_line(F("frc_upd"), F("true"));
              payload += add_device(brief);
              payload += F("}");
              
            } else {                                                            // if disabled, publish empty payload to delete
              payload = F("");
            }
          //=======================================================================================//

          //=======================================================================================//    
          //  check and publish payload
            if (check_length(discovery_topic, payload)) {
              success = publish(discovery->ctrlid, discovery_topic, payload);
            } else if (!brief) {
              addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying again with reduced content."));
              success = sensor_config(discovery, true);
            } else {
              _P126_log = F("P[126] Cannot publish config, because payload exceeds limits. You can publish the message manually from other client.: state of payload is : ");
              _P126_log += payload;
              addLog(LOG_LEVEL_ERROR,_P126_log);
              
              success = false;
            }
          //=======================================================================================//
        } 

      } 

    } 

  }
  return success;
} 

String make_topic(char* prefix, String component, String unique) {
    
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

String make_unique() {
    String unique = WiFi.macAddress();
    unique.replace(F(":"),F(""));
    unique += F("_system");

    return unique;
}

String make_unique(byte task, byte value) {

    String unique = WiFi.macAddress();
    unique.replace(F(":"),F(""));
    unique += F("_");
    unique += String(task);
    unique += F("_");
    unique += String(value);
    
    return unique;
}

bool system_state(struct DiscoveryStruct *discovery, bool brief) {

  //  define topic
    String state_topic = make_topic(discovery->save.prefix, F("sensor"), make_unique());
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
    payload += add_line(F("hostname"), WiFi.hostname());
    #endif
    payload += add_line(F("state"),  discovery->lwtup);
    payload += add_line(F("plugin"), String(PLUGIN_126_VERSION));
    payload += add_line(F("version"), versionstr);
    payload += add_line(F("unit"), String(Settings.Unit));
    payload += add_line(F("uptime"), String(wdcounter / 2));
    payload += add_line(F("last_boot_cause"), getLastBootCauseString());
    payload += add_line(F("cpu"), String(getCPUload()));
    payload += add_line(F("ip"), WiFi.localIP().toString());
    payload += add_line(F("mac"), WiFi.macAddress());
    payload += add_line(F("ssid"), WiFi.SSID());
    payload += add_line(F("bssid"), WiFi.BSSIDstr());
    payload += add_line(F("last_disconnect"), getLastDisconnectReason());
    payload += add_line(F("rssi"), String(WiFi.RSSI()), true);

    //add_line(F("Subnet Mask"), WiFi.subnetMask().toString());
    //add_line(F("Gateway IP"), WiFi.gatewayIP().toString());
    //add_line(F("DNS 1"), WiFi.dnsIP(0).toString());
    //add_line(F("DNS 2"), WiFi.dnsIP(1).toString());
    //add_line(F("IP config"), useStaticIP() ? F("Static") : F("DHCP"));
    //add_line(F("Number reconnects"), String(wifi_reconnects));

  return (publish(discovery->ctrlid, state_topic, payload));
}

bool delete_system(struct DiscoveryStruct *discovery) {

  String discovery_topic = make_topic(discovery->save.prefix, F("sensor"), make_unique());

  return (publish(discovery->ctrlid, discovery_topic, ""));
}

bool cleanup(struct DiscoveryStruct *discovery) {
  bool success;

  byte lasttaskid = TASKS_MAX - 1;
  byte taskid = _P126_increment;
  String component = F("sensor");

  if (_P126_increment > lasttaskid) {
    taskid = _P126_increment - TASKS_MAX;
    component = F("binary_sensor");
  } 

    for (byte x = 0; x <= 3; x++) {                       // for each value
      success = false;

      String uniquestr = make_unique(taskid, x);

      String discovery_topic = make_topic(discovery->save.prefix, component, uniquestr);
      success = publish(discovery->ctrlid, discovery_topic, "");
    }   

  return success;
}

bool delete_configs(struct DiscoveryStruct *discovery) {

  bool success1 = delete_system(discovery);
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

              String uniquestr = make_unique(TaskIndex, x);

              String sensorname = String(ExtraTaskSettings.TaskDeviceName);
              sensorname += F("_");
              sensorname += String(ExtraTaskSettings.TaskDeviceValueNames[x]);

              String component;
              if (discovery->save.task[TaskIndex].value[x].option < 10) {
                component = F("sensor");
              } else {
                component = F("binary_sensor");
              }

              String discovery_topic = make_topic(discovery->save.prefix, component, uniquestr);

              success2 = publish(discovery->ctrlid, discovery_topic, F(""));
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

String add_device(bool brief) {
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
      device += add_line(F("sw_version"), buildstr);
      device += add_line(F("manufacturer"), F("letscontrolit.com"));
      device += add_line(F("model"), ARDUINO_BOARD);
      //connections
      device += F("\"connections\":[");
        device += F("[\"mac\",\""); 
        device += WiFi.macAddress();
        device += F("\"]");
        device += F("],");  
    }
    device += add_line(F("name"), String(Settings.Name));                    
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
    device += F("}");
    return device;
}

String add_line(const String& object, const String& value) {
  String line = add_line(object, value, false);
  return line;
}

String add_line(const String& object, const String& value, bool last) {

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

bool check_length(const String& topic, const String& payload) {
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

bool publish(int ctrlid, const String& topic, const String& payload) {
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

void get_ctrl(struct DiscoveryStruct *discovery) {
  discovery->ctrlid = firstEnabledMQTTController();
  MakeControllerSettings(ControllerSettings);
  LoadControllerSettings(discovery->ctrlid, ControllerSettings);

  discovery->publish = ControllerSettings.Publish;
  discovery->publish.replace(F("%sysname%"), Settings.Name);
  discovery->lwttopic = ControllerSettings.MQTTLwtTopic;
  discovery->lwttopic.replace(F("%sysname%"), Settings.Name);
  discovery->lwtup = ControllerSettings.LWTMessageConnect;
  discovery->lwtdown = ControllerSettings.LWTMessageDisconnect;
  discovery->qsize = ControllerSettings.MaxQueueDepth;
  discovery->pubinterval = ControllerSettings.MinimalTimeBetweenMessages;
}

void get_id(struct DiscoveryStruct *discovery) {
  for (byte x = 0; x < TASKS_MAX; x++) {
    LoadTaskSettings(x);
    int pluginid = Settings.TaskDeviceNumber[x];
    if (pluginid == 126) {
      discovery->taskid = x;
      _P126_log = F("P[126] found correct taskid: state of x is : ");
      _P126_log += String(x);
      addLog(LOG_LEVEL_DEBUG,_P126_log);
      break;        
    } 
      _P126_log = F("P[126] couldn't fetch taskid: state of x is : ");
      _P126_log += String(x);
      addLog(LOG_LEVEL_ERROR,_P126_log);
  }
}
//#endif
