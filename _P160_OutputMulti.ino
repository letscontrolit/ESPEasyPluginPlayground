/* Multiple output plugin for ESPEasy  */
/* One dummy "blocker" values supported per outputs  */

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_160
#define PLUGIN_ID_160     160
#define PLUGIN_NAME_160   "Output - Multiple GPIO"
#define PLUGIN_VALUENAME1_160 "O1"
#define PLUGIN_VALUENAME2_160 ""
#define PLUGIN_VALUENAME3_160 ""
#define PLUGIN_VALUENAME4_160 ""

#define P160_Nlines 4
#define P160_MaxInstances 3
#define P160_Nchars 32

char P160_deviceTemplate[P160_MaxInstances][P160_Nlines][P160_Nchars];

boolean Plugin_160(byte function, struct EventStruct *event, String& string)
{

  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_160;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].TimerOptional = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_160);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_160));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_160));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_160));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_160));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

        //addFormPinSelect(F("Relay 1"), F("taskdevicepin1"), Settings.TaskDevicePin1[event->TaskIndex]);
        //addFormPinSelect(F("Relay 2"), F("taskdevicepin2"), Settings.TaskDevicePin2[event->TaskIndex]);
        //addFormPinSelect(F("Relay 3"), F("taskdevicepin3"), Settings.TaskDevicePin3[event->TaskIndex]);
        LoadTaskSettings(event->TaskIndex);
        addFormPinSelect(F("4th GPIO"), F("taskdevicepin4"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addFormCheckBox(F("Active state is LOW"), F("Plugin_160_inverted"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

        addFormNote(F("You can specify 4 output pin above."));

        byte baseaddr = 0;
        if (event->TaskIndex > 0) {
          for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
          {
            if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_160) {
              baseaddr = baseaddr + 1;
            }
          }
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = baseaddr;

        //String logs = String(F("MultiOut : Task:")) + String(event->TaskIndex) + F(" instance ") + baseaddr;
        //addLog(LOG_LEVEL_INFO, logs);

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&P160_deviceTemplate[baseaddr], sizeof(P160_deviceTemplate[baseaddr]));
        for (byte varNr = 0; varNr < P160_Nlines; varNr++)
        {
          addFormTextBox(String(F("Blocker ")) + (varNr + 1), String(F("Plugin_160_template")) + (varNr + 1), P160_deviceTemplate[baseaddr][varNr], P160_Nchars);
        }
        addFormNote(F("You can specify 4 blocker expression - for example dummy values - one for every GPIO output. 0 or no expression means no restirection, 1 means it can not be activated."));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("taskdevicepin4"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = isFormItemChecked(F("Plugin_160_inverted"));

        //String logs = String(F("MultiOut : Task:")) + String(event->TaskIndex) + F(" instance ") + Settings.TaskDevicePluginConfig[event->TaskIndex][3];
        //addLog(LOG_LEVEL_INFO, logs);

        String argName;
        byte raddr = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
        for (byte varNr = 0; varNr < P160_Nlines; varNr++)
        {
          argName = F("Plugin_160_template");
          argName += varNr + 1;
          strncpy(P160_deviceTemplate[raddr][varNr], WebServer.arg(argName).c_str(), sizeof(P160_deviceTemplate[raddr][varNr]));
        }

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&P160_deviceTemplate[raddr], sizeof(P160_deviceTemplate[raddr]));

        success = true;
        break;

      }
    case PLUGIN_INIT:
      {
        byte p160_relaycount = 0;
        char echr[41] = {0};
        LoadTaskSettings(event->TaskIndex);
        if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
        {
          p160_relaycount = 1;
          pinMode(Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);
          digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
          UserVar[event->BaseVarIndex] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        }
        if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
        {
          if (p160_relaycount > 0) {
            p160_relaycount = 2;
            pinMode(Settings.TaskDevicePin2[event->TaskIndex], OUTPUT);
            digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            UserVar[event->BaseVarIndex+1] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          }
        }
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          if (p160_relaycount > 1) {
            p160_relaycount = 3;
            pinMode(Settings.TaskDevicePin3[event->TaskIndex], OUTPUT);
            digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            UserVar[event->BaseVarIndex+2] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          }
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != -1)
        {
          if (p160_relaycount > 2) {
            p160_relaycount = 4;
            pinMode(Settings.TaskDevicePluginConfig[event->TaskIndex][0], OUTPUT);
            digitalWrite(Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            UserVar[event->BaseVarIndex+3] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          }
        }
        if (p160_relaycount < 4) {
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], echr);
        }
        if (p160_relaycount < 3) {
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], echr);
        }
        if (p160_relaycount < 2) {
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], echr);
        }
        if (p160_relaycount < 1) {
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], echr);
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = p160_relaycount;

        byte baseaddr = 0;
        if (event->TaskIndex > 0) {
          for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
          {
            if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_160) {
              baseaddr = baseaddr + 1;
            }
          }
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = baseaddr;

        String logs = String(F("MultiOut : Task:")) + String(event->TaskIndex) + F(" instance ") + String(Settings.TaskDevicePluginConfig[event->TaskIndex][3]) + F(" gpios ")+ String(Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        addLog(LOG_LEVEL_INFO, logs);

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&P160_deviceTemplate[baseaddr], sizeof(P160_deviceTemplate[baseaddr]));
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][3] < P160_MaxInstances) {
          success = true;
        } else {
          success = false;
          logs = String(F("MO : Task init failed"));
          addLog(LOG_LEVEL_INFO, logs);
        }
        break;
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
        if (command == F("output"))
        {
          String taskName = parseString(string, 2);
          int8_t taskIndex = getTaskIndexByName(taskName);
          if (taskIndex != -1)
          {
            String logs = F("");
            success = true;
            LoadTaskSettings(taskIndex);
            byte BaseVarIndex1 = taskIndex * VARS_PER_TASK;
            String valueNum = parseString(string, 3);
            byte relnum = (byte)valueNum.toInt();
            byte pinnum = -1;
              switch (relnum) {
                case 0:
                  pinnum = Settings.TaskDevicePin1[taskIndex];
                  break;
                case 1:
                  pinnum = Settings.TaskDevicePin2[taskIndex];
                  break;
                case 2:
                  pinnum = Settings.TaskDevicePin3[taskIndex];
                  break;
                case 3:
                  pinnum = Settings.TaskDevicePluginConfig[taskIndex][0];
                  break;
              }
              byte pinvalue = -1;
              String vvalue = parseString(string, 4);
              if (vvalue.length() < 1) {
                pinvalue = !UserVar[BaseVarIndex1 + relnum];
              } else {
                pinvalue = (byte)vvalue.toInt();
              }
              
              byte inhibaddr = Settings.TaskDevicePluginConfig[taskIndex][3];              
              String tmpString = String(P160_deviceTemplate[inhibaddr][relnum]);              
              byte inhibitvalue = parseTemplate(tmpString, 20).toInt();
              if (inhibitvalue >= 1) {
                logs = F("Blocker active ");
                logs += String(inhibitvalue)+ F(" set to LOW");
                addLog(LOG_LEVEL_INFO, logs);                 
                pinvalue = Settings.TaskDevicePluginConfig[taskIndex][1];
              }
              
              if (pinvalue != -1) {
                if (pinnum != -1) {
                  pinMode(pinnum, OUTPUT);
                  digitalWrite(pinnum, pinvalue);
                }
                UserVar[BaseVarIndex1 + relnum] = pinvalue;
              }
            logs = String(F("MultiOut : ")) + taskName + F(" GPIO ") + pinnum + F(" value: ") + pinvalue;
            addLog(LOG_LEVEL_INFO, logs);
            logs = F("\nOk");            
            SendStatus(event->Source, logs);
          }
        }
        break;
      }


  }   // switch
  return success;

}     //

#endif
