/*##########################################################################################
  ############################### Plugin 160: Multiple output plugin #######################
  ##########################################################################################

  Features :
	- Records max 4 GPIO output states in UserVars (max 3 instance means 3x4 output pin)
        - One "blocker" line belongs to every GPIO,
          when blocker evaluates to >= 1 then GPIO can not be set to active state
          otherwise GPIO can be setted
        - changing values has to be done with the "output" command!
        - can be specified one master relay task (proposed type:P029 Output), this gpio will be high
          AFTER any output pin (or sibling output pin) switched to high, and master relay still high
          until any output pin (or sibling output pin) switched to high, master relay will be low
          directly BEFORE the last of the output pins (+sibling output pins if used) go to low
          Sibling output tasks has to be P160 MultiOut task number or 0 if not used

  List of commands :
	- output,[devicename],[pin_number],[status]   Set specific GPIO status of the named task (0/1)
	- output,[devicename],[pin_number]            Toggle GPIO status (pin_number:0-3)

  Command Examples :
	-  /control?cmd=output,seldevice,0,0          Set task named 'seldevice' first GPIO to 0
	-  /control?cmd=output,seldevice,1            Toggle task named 'seldevice' second GPIO

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

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
        addFormNote(F("You can specify 4 output pin above."));
        addFormCheckBox(F("Active state is LOW"), F("Plugin_160_inverted"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        addFormNumericBox(F("Use Master relay at Task number"), F("Plugin_160_mr"), Settings.TaskDevicePluginConfig[event->TaskIndex][4], 0, TASKS_MAX + 1);
        addFormNumericBox(F("Use Sibling relay group#1 at Task number"), F("Plugin_160_sr1"), Settings.TaskDevicePluginConfig[event->TaskIndex][5], 0, TASKS_MAX + 1);
        addFormNumericBox(F("Use Sibling relay group#2 at Task number"), F("Plugin_160_sr2"), Settings.TaskDevicePluginConfig[event->TaskIndex][6], 0, TASKS_MAX + 1);
        addFormNote(F("You can specify 1 master and 2 group x 4 sibling relay (sibling relays controls the same master). Write 0's if not needed."));
        addFormNumericBox(F("Master relay cooldown/warmup time"), F("Plugin_160_mrdelay"), Settings.TaskDevicePluginConfig[event->TaskIndex][7], 0, 2000);
        addUnit(F("msec"));

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
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = getFormItemInt(F("Plugin_160_mr"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = getFormItemInt(F("Plugin_160_sr1"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][6] = getFormItemInt(F("Plugin_160_sr2"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][7] = getFormItemInt(F("Plugin_160_mrdelay"));

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
            UserVar[event->BaseVarIndex + 1] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          }
        }
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          if (p160_relaycount > 1) {
            p160_relaycount = 3;
            pinMode(Settings.TaskDevicePin3[event->TaskIndex], OUTPUT);
            digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            UserVar[event->BaseVarIndex + 2] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          }
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != -1)
        {
          if (p160_relaycount > 2) {
            p160_relaycount = 4;
            pinMode(Settings.TaskDevicePluginConfig[event->TaskIndex][0], OUTPUT);
            digitalWrite(Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            UserVar[event->BaseVarIndex + 3] = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
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

        String logs = String(F("MultiOut : Task:")) + String(event->TaskIndex) + F(" instance ") + String(Settings.TaskDevicePluginConfig[event->TaskIndex][3]) + F(" gpios ") + String(Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
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
              logs += String(inhibitvalue) + F(" set to LOW");
              addLog(LOG_LEVEL_INFO, logs);
              pinvalue = Settings.TaskDevicePluginConfig[taskIndex][1];
            }
            byte TaskIndexM = Settings.TaskDevicePluginConfig[taskIndex][4];
            byte BaseVarIndexM = 0;
            if ( (pinvalue == Settings.TaskDevicePluginConfig[taskIndex][1]) && (TaskIndexM > 0) ) {
              TaskIndexM = TaskIndexM - 1; // tasks are 0 based in memory, 1 based on webgui

              BaseVarIndexM = TaskIndexM * VARS_PER_TASK;
              if (UserVar[BaseVarIndexM] != pinvalue) {
                bool AllOff = true;

                for (byte TVI = 0; TVI < Settings.TaskDevicePluginConfig[taskIndex][2]; TVI++)           // check own values
                {
                  if (relnum != TVI) {
                    if (UserVar[BaseVarIndex1 + TVI] != Settings.TaskDevicePluginConfig[taskIndex][1]) {
                      AllOff = false;
                      break;
                    }
                  }
                }
                byte TaskIndexS = 0;
                byte BaseVarIndexS = 0;
                if (AllOff) {
                  TaskIndexS = Settings.TaskDevicePluginConfig[taskIndex][5];
                  if (TaskIndexS > 0) {
                    TaskIndexS = TaskIndexS - 1;

                    BaseVarIndexS = TaskIndexS * VARS_PER_TASK;
                    for (byte TVI = 0; TVI < Settings.TaskDevicePluginConfig[TaskIndexS][2]; TVI++)         // check first sibling values
                    {
                      if (UserVar[BaseVarIndexS + TVI] != Settings.TaskDevicePluginConfig[taskIndex][1]) {
                        AllOff = false;
                        break;
                      }
                    }
                  }
                }
                if (AllOff) {
                  TaskIndexS = Settings.TaskDevicePluginConfig[taskIndex][6];
                  if (TaskIndexS > 0) {
                    TaskIndexS = TaskIndexS - 1;
                    BaseVarIndexS = TaskIndexS * VARS_PER_TASK;
                    for (byte TVI = 0; TVI < Settings.TaskDevicePluginConfig[TaskIndexS][2]; TVI++)          // check second sibling values
                    {
                      if (UserVar[BaseVarIndexS + TVI] != Settings.TaskDevicePluginConfig[taskIndex][1]) {
                        AllOff = false;
                        break;
                      }
                    }
                  }
                }
                if (AllOff) {
                  logs = F("Master relay set to LOW ");
                  logs += String(Settings.TaskDevicePin1[TaskIndexM]);
                  addLog(LOG_LEVEL_INFO, logs);
                  pinMode(Settings.TaskDevicePin1[TaskIndexM], OUTPUT);
                  digitalWrite(Settings.TaskDevicePin1[TaskIndexM], pinvalue);
                  UserVar[BaseVarIndexM] = pinvalue;
                  delay(Settings.TaskDevicePluginConfig[taskIndex][7]); // cooldown delay
                }
              }
            }

            if (pinvalue != -1) {
              if (pinnum != -1) {
                pinMode(pinnum, OUTPUT);
                digitalWrite(pinnum, pinvalue);
              }
              UserVar[BaseVarIndex1 + relnum] = pinvalue;
              String events = getTaskDeviceName(taskIndex);
              events += F("#");
              events += ExtraTaskSettings.TaskDeviceValueNames[relnum];
              events += F("=");
              events += String(pinvalue);
              rulesProcessing(events);
            }
            logs = String(F("MultiOut : ")) + taskName + F(" GPIO ") + pinnum + F(" value: ") + pinvalue;
            addLog(LOG_LEVEL_INFO, logs);

            if ( ((!pinvalue) == Settings.TaskDevicePluginConfig[taskIndex][1]) && (TaskIndexM > 0) ) {
              TaskIndexM = TaskIndexM - 1; // tasks are 0 based in memory, 1 based on webgui
              logs = F("Master relay set to HIGH ");
              logs += String(Settings.TaskDevicePin1[TaskIndexM]);
              addLog(LOG_LEVEL_INFO, logs);

              BaseVarIndexM = TaskIndexM * VARS_PER_TASK;
              if (UserVar[BaseVarIndexM] != pinvalue) { // check if pin is already in high?
                delay(Settings.TaskDevicePluginConfig[taskIndex][7]); // warmup delay
              }

              pinMode(Settings.TaskDevicePin1[TaskIndexM], OUTPUT);
              digitalWrite(Settings.TaskDevicePin1[TaskIndexM], pinvalue);
              UserVar[BaseVarIndexM] = pinvalue;
            }

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
