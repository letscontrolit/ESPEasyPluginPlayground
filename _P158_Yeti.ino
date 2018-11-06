/*##########################################################################################
  ############################### Plugin 158: Yeti interface plugin ########################
  ##########################################################################################

  Features :
	- Supports Yeti Android application by implementing partial Tasmota commands  (status + power)
        - can be added to Yeti manually at Sonoff-Connect-Advanced and type the module IP address, automatic search may not work

  List of commands :
	- power,[relay_number],[status]             Set specific relay status

  Command Examples :
	-  /control?cmd=power,1,on                  Set first relay to ON
	-  /control?cmd=power,2,off                 Set second relay to OFF

Commands at Tasmota way:

         Get status:
             http://IPADDRESS/cm?cmnd=status%200
         Toggle the first and only GPIO (when only 1 defined):
             http://IPADDRESS/cm?cmnd=Power%20TOGGLE
         Toggle the first GPIO (when more than 1 defined):
             http://IPADDRESS/cm?cmnd=Power1%20TOGGLE
         Toggle the second GPIO (when more than 1 defined):
             http://IPADDRESS/cm?cmnd=Power2%20TOGGLE

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_158
#define PLUGIN_ID_158     158
#define PLUGIN_NAME_158   "Yeti interface"
#define PLUGIN_VALUENAME1_158 "POWER1"
#define PLUGIN_VALUENAME2_158 "POWER2"
#define PLUGIN_VALUENAME3_158 "POWER3"
#define PLUGIN_VALUENAME4_158 "POWER4"

boolean Plugin_158_init = false;
byte p158_relaycount = 0;
byte Plugin_158_taskindex;
byte Plugin_158_varindex;
byte Plugin_158_idx;

boolean Plugin_158(byte function, struct EventStruct *event, String& string)
{

  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_158;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_158);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_158));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_158));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_158));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_158));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

        //addFormPinSelect(F("Relay 1"), F("taskdevicepin1"), Settings.TaskDevicePin1[event->TaskIndex]);
        //addFormPinSelect(F("Relay 2"), F("taskdevicepin2"), Settings.TaskDevicePin2[event->TaskIndex]);
        //addFormPinSelect(F("Relay 3"), F("taskdevicepin3"), Settings.TaskDevicePin3[event->TaskIndex]);
        addFormPinSelect(F("4th GPIO:"), F("taskdevicepin4"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addFormCheckBox(F("Report back only first name to Yeti"), F("Plugin_158_fakename"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        
        addFormNote(F("You can specify 4 output pin above, that can be controlled by Yeti after adding the unit IP address into it."));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("taskdevicepin4"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = isFormItemChecked(F("Plugin_158_fakename"));
        success = true;
        break;

      }
    case PLUGIN_INIT:
      {
        p158_relaycount = 0;
        if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
        {
          p158_relaycount = 1;
          //          UserVar[event->BaseVarIndex] = (digitalRead(Settings.TaskDevicePin1[event->TaskIndex]) == HIGH);
          pinMode(Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);
        }
        if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
        {
          if (p158_relaycount > 0) {
            p158_relaycount = 2;
            //            UserVar[event->BaseVarIndex + 1] = (digitalRead(Settings.TaskDevicePin2[event->TaskIndex]) == HIGH);
            pinMode(Settings.TaskDevicePin2[event->TaskIndex], OUTPUT);
          }
        }
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          if (p158_relaycount > 1) {
            p158_relaycount = 3;
            //            UserVar[event->BaseVarIndex + 2] = (digitalRead(Settings.TaskDevicePin3[event->TaskIndex]) == HIGH);
            pinMode(Settings.TaskDevicePin3[event->TaskIndex], OUTPUT);
          }
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != -1)
        {
          if (p158_relaycount > 2) {
            p158_relaycount = 4;
            //            UserVar[event->BaseVarIndex + 3] = (digitalRead(Settings.TaskDevicePluginConfig[event->TaskIndex][0]) == HIGH);
            pinMode(Settings.TaskDevicePluginConfig[event->TaskIndex][0], OUTPUT);
          }
        }

        Plugin_158_taskindex = event->TaskIndex;
        Plugin_158_varindex = event->BaseVarIndex;
        Plugin_158_idx = event->idx;

        WebServer.on("/cm", p158_handle_cm);

        Plugin_158_init = true;
        success = true;
        break;

      }

    case PLUGIN_READ:
      {
        success = true;
        break;

      }

    case PLUGIN_WRITE:
      {
        if (Plugin_158_init)
        {

          String command = parseString(string, 1);
          if ( command == F("power") )
          {
            success = true;
            p158_set_state(event->Par1, String(event->Par2));
            String log = F("YetiSW   : SetSwitch r");
            log += event->Par1;
            log += F(":");
            log += event->Par2;
            addLog(LOG_LEVEL_INFO, log);
            log = F("\nOk");
            SendStatus(event->Source, log);
          }

        }
        break;
      }


  }   // switch
  return success;

}     //function

//plugin specific procedures and functions
String p158_get_state(byte relnum, boolean retstr) {
  if ((relnum > 0) && (relnum <= p158_relaycount)) {
    if (UserVar[Plugin_158_varindex + relnum - 1] > 0) {
      if (retstr) {
        return F("ON");
      } else {
        return F("1");
      }
    } else {
      if (retstr) {
        return F("OFF");
      } else {
        return F("0");
      }
    }
  } else {
    return F("");
  }
}

void p158_set_state_0(byte relnum, byte newstate) {
 switch (relnum) {
   case 1:
    pinMode(Settings.TaskDevicePin1[Plugin_158_taskindex], OUTPUT);
    digitalWrite(Settings.TaskDevicePin1[Plugin_158_taskindex], newstate);
   break; 
   case 2:
    pinMode(Settings.TaskDevicePin2[Plugin_158_taskindex], OUTPUT);
    digitalWrite(Settings.TaskDevicePin2[Plugin_158_taskindex], newstate);
   break; 
   case 3:
    pinMode(Settings.TaskDevicePin3[Plugin_158_taskindex], OUTPUT);
    digitalWrite(Settings.TaskDevicePin3[Plugin_158_taskindex], newstate);
   break; 
   case 4:
    pinMode(Settings.TaskDevicePluginConfig[Plugin_158_taskindex][0], OUTPUT);
    digitalWrite(Settings.TaskDevicePluginConfig[Plugin_158_taskindex][0], newstate);
   break;  
 }
}


void p158_set_state(byte relnum, String state) {
  LoadTaskSettings(Plugin_158_taskindex);
  if ((relnum > 0) && (relnum <= p158_relaycount)) {
    if ((state.equalsIgnoreCase(F("off"))) || (state.equalsIgnoreCase(F("0")))) {
      UserVar[Plugin_158_varindex + relnum - 1] = 0;
      p158_set_state_0(relnum,0);
    } else if ((state.equalsIgnoreCase(F("on"))) || (state.equalsIgnoreCase(F("1")))) {
      UserVar[Plugin_158_varindex + relnum - 1] = 1;
      p158_set_state_0(relnum,1);      
    } else if ((state.equalsIgnoreCase(F("toggle"))) || (state.equalsIgnoreCase(F("2")))) {
      if (UserVar[Plugin_158_varindex + relnum - 1] > 0) {
        UserVar[Plugin_158_varindex + relnum - 1] = 0;
        p158_set_state_0(relnum,0);     
      } else {
        UserVar[Plugin_158_varindex + relnum - 1] = 1;
        p158_set_state_0(relnum,1);             
      }
    }
  }
}

void p158_handle_cm()
{
  String wcmnd = WebServer.arg("cmnd");
  WebServer.sendHeader("Access-Control-Allow-Origin", "*");
  TXBuffer.startJsonStream();

  if ((wcmnd.length() != 0) && (Plugin_158_init))
  {
    LoadTaskSettings(Plugin_158_taskindex);
    if (wcmnd.substring(0, 6).equalsIgnoreCase(F("status"))) {
      char strUpTime[40];
      int minutes = wdcounter / 2;
      int days = minutes / 1440;
      minutes = minutes % 1440;
      int hrs = minutes / 60;
      minutes = minutes % 60;
      String tstr = F("");
      TXBuffer += F("{");
      if ((wcmnd.length() == 6) || (wcmnd.substring(7) == F("0")) || (wcmnd.substring(7) == F("9"))) { // status=abbrev only, 0=all
        TXBuffer += F("\"Status\":{\n"); // }
        byte modnum = 1;
        switch (p158_relaycount) {
          case 1:
            modnum = 28;
            break;
          case 2:
            modnum = 29;
            break;
          case 3:
            modnum = 30;
            break;
          case 4:
            modnum = 7;
            break;
        }
        stream_next_json_object_value(F("Module"), String(modnum));
        TXBuffer += F("\"FriendlyName\":[");
        if (Settings.TaskDevicePluginConfig[Plugin_158_taskindex][1]) {
          TXBuffer += F("\"");
          TXBuffer += String(ExtraTaskSettings.TaskDeviceValueNames[0]);
          TXBuffer += F("\"");         
        } else        
         for (modnum = 0; modnum < p158_relaycount; modnum++)
         {
          TXBuffer += F("\"");
          TXBuffer += String(ExtraTaskSettings.TaskDeviceValueNames[modnum]);
          TXBuffer += F("\"");
          if (modnum < (p158_relaycount - 1)) {
            TXBuffer += F(",");
          }
         }
        TXBuffer += F("],");
        stream_last_json_object_value(F("Power"), F("0"));
        //        stream_last_json_object_value(F("FriendlyName"), tstr);
      }

      if ((wcmnd.substring(7) == F("1")) || (wcmnd.substring(7) == F("0"))) { // 1=prm, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        sprintf_P(strUpTime, PSTR("%dT%d:%d:0"), days, hrs, minutes);
        TXBuffer += F("\"StatusPRM\":{\n"); // }
        stream_next_json_object_value(F("RestartReason"), getLastBootCauseString());
        stream_last_json_object_value(F("Uptime"), String(strUpTime));
      }
      if ((wcmnd.substring(7) == F("2")) || (wcmnd.substring(7) == F("0"))) { // 2=fwr, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusFWR\":{\n"); // }
        stream_next_json_object_value(F("Version"), F("5.12.0"));
        tstr = String(BUILD);
        tstr += F(BUILD_NOTES);
        stream_next_json_object_value(F("RealVersion"), tstr); // tell the truth
        stream_next_json_object_value(F("BuildDateTime"), String(CRCValues.compileDate) + "T" + String(CRCValues.compileTime));
#if defined(ESP32)
        String strcore = ESP.getSdkVersion();
        String strsdk  = strcore;
#else
        String strcore = ESP.getCoreVersion();
        String strsdk  = system_get_sdk_version();
#endif
        stream_next_json_object_value(F("Core"), strcore);
        stream_next_json_object_value(F("SDK"), strsdk);
        stream_last_json_object_value(F("Firmware"), F("ESPEasy")); // tell the truth
      }
      if ((wcmnd.substring(7) == F("3")) || (wcmnd.substring(7) == F("0"))) { // 3=log, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusLOG\":{\n"); // }
        stream_next_json_object_value(F("SerialLog"), String(Settings.SerialLogLevel));
        stream_next_json_object_value(F("WebLog"), String(Settings.WebLogLevel));
        TXBuffer += F("\"SSId\":[\"");
        TXBuffer += WiFi.SSID();
        TXBuffer += F("\"],");
        stream_last_json_object_value(F("TelePeriod"), F("0"));
      }
      if ((wcmnd.substring(7) == F("4")) || (wcmnd.substring(7) == F("0"))) { // 4=mem, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusMEM\":{\n"); // }
        uint32_t realSize = 0;
#if defined(ESP8266)
        realSize = ESP.getFlashChipRealSize();
#endif
#if defined(ESP32)
        realSize = ESP.getFlashChipSize();
#endif
        uint32_t ideSize = ESP.getFlashChipSize();

#if defined(ESP8266)
        stream_next_json_object_value(F("ProgramSize"), String(ESP.getSketchSize() / 1024));
        stream_next_json_object_value(F("Free"), String(ESP.getFreeSketchSpace() / 1024));
#endif
        stream_next_json_object_value(F("Heap"), String(ESP.getFreeHeap() / 1024));
        stream_next_json_object_value(F("ProgramFlashSize"), String(ideSize / 1024));
        stream_next_json_object_value(F("FlashSize"), String(realSize / 1024));
        stream_last_json_object_value(F("FlashMode"), String(ESP.getFlashChipMode()));
      }

      if ((wcmnd.substring(7) == F("5")) || (wcmnd.substring(7) == F("0"))) { // 5=net, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusNET\":{\n"); // }
#if defined(ESP8266)
        stream_next_json_object_value(F("Hostname"), WiFi.hostname());
#endif
        stream_next_json_object_value(F("IPAddress"), WiFi.localIP().toString());
        stream_next_json_object_value(F("Gateway"), WiFi.gatewayIP().toString());
        stream_next_json_object_value(F("Subnetmask"), WiFi.subnetMask().toString());
        stream_next_json_object_value(F("DNSServer"), WiFi.dnsIP(0).toString());
        stream_next_json_object_value(F("Mac"), WiFi.macAddress());
        stream_next_json_object_value(F("Webserver"), F("2"));
        stream_last_json_object_value(F("WifiConfig"), F("2"));
      }

      if ((wcmnd.substring(7) == F("6")) || (wcmnd.substring(7) == F("0"))) { // 6=mqt, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusMQT\":{\n"); // }
        stream_last_json_object_value(F("MqttType"), F("0")); // not implemented
      }
      if ((wcmnd.substring(7) == F("7")) || (wcmnd.substring(7) == F("0"))) { // 7=time, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusTIM\":{\n"); // }
        stream_next_json_object_value(F("Local"), getDateTimeString('-', ':', 'T'));
        stream_last_json_object_value(F("Timezone"), F("1"));
      }
      if ((wcmnd.substring(7) == F("8")) || (wcmnd.substring(7) == F("10")) || (wcmnd.substring(7) == F("0"))) { // 8=sns, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusSNS\":{\n"); // }
        stream_last_json_object_value(F("Time"), getDateTimeString('-', ':', 'T'));
      }

      if ((wcmnd.substring(7) == F("11")) || (wcmnd.substring(7) == F("0"))) { // 11=sts, 0=all
        if (wcmnd.substring(7) == F("0")) {
          TXBuffer += F(",");
        }
        TXBuffer += F("\"StatusSTS\":{\n"); // }
        stream_next_json_object_value(F("Time"), getDateTimeString('-', ':', 'T'));
        sprintf_P(strUpTime, PSTR("%dT%d:%d:0"), days, hrs, minutes);
        stream_next_json_object_value(F("Uptime"), String(strUpTime));
        if (p158_relaycount == 1) {
          stream_next_json_object_value(F("POWER"), String(p158_get_state(1, true)));
        } else
          for (byte rnum = 0; rnum < p158_relaycount; rnum++)
          {
            tstr = F("POWER");
            tstr += String(rnum + 1);
            stream_next_json_object_value(tstr, String(p158_get_state(rnum + 1, true)));
          }
        TXBuffer += F("\"Wifi\":{\n"); // }
        stream_next_json_object_value(F("AP"), F("1"));
        stream_next_json_object_value(F("SSId"), WiFi.SSID());
        stream_next_json_object_value(F("RSSI"), String( (WiFi.RSSI() * (-1)) ));
        stream_last_json_object_value(F("APMac"), WiFi.BSSIDstr());
        TXBuffer += F("\n}");
      }

      TXBuffer += F("}");
    } // status end
    if (wcmnd.substring(0, 5).equalsIgnoreCase(F("power"))) {
      byte relnum2 = (byte)wcmnd.substring(5, 6).toInt();
      if (wcmnd.length() > 7) {
       p158_set_state(relnum2, wcmnd.substring(7));
      }
      TXBuffer += F("{"); // }
      int tempidx = Plugin_158_idx+relnum2-1;
      if (tempidx<1) {
        tempidx = 1;
      }
      stream_next_json_object_value(F("idx"), String(tempidx));
      stream_next_json_object_value(F("nvalue"), p158_get_state(relnum2, false));
      stream_next_json_object_value(F("svalue"), F(""));
      String pwrstr = F("POWER");
      if (p158_relaycount>1) {
       pwrstr += String(relnum2);
      }
      stream_last_json_object_value(pwrstr, String(p158_get_state(relnum2, true)));

    } // power end
  }
  TXBuffer.endStream();
}

#endif
