#include <Arduino.h>
#include <SoftwareSerial.h>

//#######################################################################################################
//#################################### Plugin 117: Nextion <info@sensorio.cz>  ##########################
//#################################### Based on the work of  majklovec         ##########################
//#######################################################################################################

#define PLUGIN_117
#define PLUGIN_ID_117 117
#define PLUGIN_NAME_117 "Display: Nextion"
#define PLUGIN_VALUENAME1_117 "code"
unsigned long Plugin_117_code = 0;
int8_t Plugin_117_RXpin = -1;
int8_t Plugin_117_TXpin = -1;

SoftwareSerial *nextion;

boolean Plugin_117(byte function, struct EventStruct *event, String &string) {
  boolean success = false;
  static byte displayTimer = 0;

  switch (function) {

    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number = PLUGIN_ID_117;

      Device[deviceCount].Type = DEVICE_TYPE_DUAL;
      Device[deviceCount].VType = SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = true;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = false;
      Device[deviceCount].ValueCount = 4;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = true;
      Device[deviceCount].GlobalSyncOption = true;

      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_117);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES: {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0],PSTR(PLUGIN_VALUENAME1_117));
      break;
    }
    
    case PLUGIN_TEN_PER_SECOND: {
      char __buffer[80];

      uint16_t i;
      uint8_t c;
      while (nextion->available() > 0) {
        delay(10);
        c = nextion->read();
        
        if (0x65 == c) {
          if (nextion->available() >= 6) {
            __buffer[0] = c;
            for (i = 1; i < 7; i++) {
              __buffer[i] = nextion->read();
            }

            __buffer[i] = 0x00;

            if (0xFF == __buffer[4] && 0xFF == __buffer[5] && 0xFF == __buffer[6]) {
              Plugin_117_code = ((__buffer[1] >> 8) & __buffer[2] >> 8) & __buffer[3];
              UserVar[event->BaseVarIndex] = __buffer[1] * 256 + __buffer[2];
              UserVar[event->BaseVarIndex + 1] = __buffer[3];
              String log = F("Nextion : code: ");
              log += __buffer[1];
              log += ",";
              log += __buffer[2];
              log += ",";
              log += __buffer[3];

              addLog(LOG_LEVEL_INFO, log);
              sendData(event);
            }
          }
        } else {

          if (c == 's') {
            i = 1;
            __buffer[0] = c;
            c=0;
            while (nextion->available() > 0) {
              __buffer[i++] = nextion->read();
              Serial.write((char)__buffer[i]);
            }
            Serial.println();
            __buffer[i] = 0x00;

            UserVar[event->BaseVarIndex] = __buffer[2];
            UserVar[event->BaseVarIndex+1] = __buffer[3];
            UserVar[event->BaseVarIndex+2] = __buffer[4];
            UserVar[event->BaseVarIndex+3] = __buffer[5];

            String log = F("Nextion : send command: ");
            log += __buffer;
            log += UserVar[event->BaseVarIndex];
            addLog(LOG_LEVEL_INFO, log);
            
            ExecuteCommand(VALUE_SOURCE_SYSTEM, __buffer);
          }
        }
      }

      if (Settings.TaskDevicePin3[event->TaskIndex] != -1) {
        if (!digitalRead(Settings.TaskDevicePin3[event->TaskIndex])) {
          Plugin_117_displayOn();
          displayTimer = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        }
      }

      success = true;
      break;
    }

    case PLUGIN_ONCE_A_SECOND: {
      if ( displayTimer > 0) {
        displayTimer--;
        if (displayTimer == 0)
          Plugin_117_displayOff();
        }
        break;
      }


    case PLUGIN_WEBFORM_SAVE: {

      String plugin1 = WebServer.arg("plugin_117_timer");
      Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
      String plugin2 = WebServer.arg("plugin_117_rxwait");
      Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
      String plugin3 = WebServer.arg("plugin_117_events");
      Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();
      String plugin4 = WebServer.arg("plugin_117_sensortype");
      Settings.TaskDevicePluginConfig[event->TaskIndex][3] = plugin4.toInt();

      char deviceTemplate[8][64];
      for (byte varNr = 0; varNr < 8; varNr++) {
        char argc[25];
        String arg = F("Plugin_117_template");
        arg += varNr + 1;
        arg.toCharArray(argc, 25);
        String tmpString = WebServer.arg(argc);
        strncpy(deviceTemplate[varNr], tmpString.c_str(), sizeof(deviceTemplate[varNr]));
      }

//      Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

      SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
    
      success = true;
      break;
    }


    case PLUGIN_WEBFORM_LOAD: {

      char tmpString[128];

      char deviceTemplate[8][64];
      LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
      for (byte varNr = 0; varNr < 8; varNr++)
        {
          string += F("<TR><TD>Line ");
          string += varNr + 1;
          string += F(":<TD><input type='text'li size='50' maxlength='50' name='Plugin_117_template");
          string += varNr + 1;
          string += F("' value='");
          string += deviceTemplate[varNr];
          string += F("'>");
        }

      string += F("<TR><TD>Display button:<TD>");
      addPinSelect(false, string, "taskdevicepin3", Settings.TaskDevicePin3[event->TaskIndex]);

      sprintf_P(tmpString, PSTR("<TR><TD>Display Timeout:<TD><input type='text' name='plugin_117_timer' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
      string += tmpString;

      success = true;
      break;
    }

    case PLUGIN_INIT: {

      LoadTaskSettings(event->TaskIndex);
//      if ((ExtraTaskSettings.TaskDevicePluginConfigLong[0] != 0) && (ExtraTaskSettings.TaskDevicePluginConfigLong[1] != 0)) {
//      if (Settings.TaskDevicePin1[event->TaskIndex] != -1) {
//        pinMode(Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);
//       digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW);
//        delay(500);
//        digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH);
//        pinMode(Settings.TaskDevicePin1[event->TaskIndex], INPUT_PULLUP);
//      }

      displayTimer = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
      if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
          pinMode(Settings.TaskDevicePin3[event->TaskIndex], INPUT_PULLUP);

      if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
        {
          Plugin_117_RXpin = Settings.TaskDevicePin1[event->TaskIndex];
//          pinMode(Plugin_117_RXpin, INPUT_PULLUP);
//          attachInterrupt(Plugin_117_RXpin, RF_ISR, CHANGE);
          success = true;
        }

      if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
        {
          Plugin_117_TXpin = Settings.TaskDevicePin2[event->TaskIndex];
//          pinMode(Plugin_117_TXpin, OUTPUT);
        }
      
      if (!nextion) {
//        nextion = new SoftwareSerial(13,14);
        nextion = new SoftwareSerial(Plugin_117_RXpin , Plugin_117_TXpin);
      }

      Serial.print("pin: ");
      Serial.println(Settings.TaskDevicePin1[event->TaskIndex]);

      nextion->begin(9600);

      success = true;
      break;
    }

    case PLUGIN_READ: {
        char deviceTemplate[8][64];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        for (byte x = 0; x < 8; x++) {
          String tmpString = deviceTemplate[x];
          if (tmpString.length()) {
            String newString = parseTemplate(tmpString, 0);
            Serial.println(newString);

            sendCommand(newString.c_str());
            }
        }
        success = false;
        break;
      }


    case PLUGIN_WRITE: {
      String tmpString = string;
      int argIndex = tmpString.indexOf(',');
      if (argIndex)
        tmpString = tmpString.substring(0, argIndex);
      if (tmpString.equalsIgnoreCase(F("NEXTION"))) {
        argIndex = string.indexOf(',');
        tmpString = string.substring(argIndex + 1);

        sendCommand(tmpString.c_str());

        Serial.println(tmpString);
        success = true;
      }
      break;
    }
  }
  return success;
}




void Plugin_117_displayOn() {
}

void Plugin_117_displayOff() {
  
}

void sendCommand(const char *cmd) {
  nextion->print(cmd);
  nextion->write(0xff);
  nextion->write(0xff);
  nextion->write(0xff);
}
