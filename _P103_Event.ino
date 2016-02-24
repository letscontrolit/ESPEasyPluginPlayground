//#######################################################################################################
//#################################### Plugin 103: Event Action #########################################
//#######################################################################################################
#define PLUGIN_103
#define PLUGIN_ID_103        103
#define PLUGIN_NAME_103       "Event Action"
#define PLUGIN_VALUENAME1_103 ""

#define PLUGIN_103_LISTEN_BOOT       1
#define PLUGIN_103_LISTEN_TASKVALUE  2
#define PLUGIN_103_LISTEN_TIMER      3
#define PLUGIN_103_LISTEN_USEREVENT  4
#define PLUGIN_103_LISTEN_CLOCKEVENT 5

#define TIMER_MAX                    8
#define PLUGIN_TIMER_EVENT          90
#define PLUGIN_USER_EVENT           91

unsigned long UserTimer[TIMER_MAX];

struct Plugin_103_Struct
{
  char commandOn[80];
  char commandOff[80];
};

boolean Plugin_103(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte switchstate[TASKS_MAX];

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_103;
        Device[deviceCount].Custom = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_103);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_103));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        struct Plugin_103_Struct custom;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&custom, sizeof(custom));

        char tmpString[200];

        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[5];
        options[0] = F("Boot");
        options[1] = F("SensorValue");
        options[2] = F("Timer");
        options[3] = F("UserEvent");
        options[4] = F("Clock");
        int optionValues[5];
        optionValues[0] = PLUGIN_103_LISTEN_BOOT;
        optionValues[1] = PLUGIN_103_LISTEN_TASKVALUE;
        optionValues[2] = PLUGIN_103_LISTEN_TIMER;
        optionValues[3] = PLUGIN_103_LISTEN_USEREVENT;
        optionValues[4] = PLUGIN_103_LISTEN_CLOCKEVENT;
        string += F("<TR><TD>Event Source:<TD><select name='plugin_103_source' LANGUAGE=javascript onchange=\"return dept_onchange(frmselect)\">");

        for (byte x = 0; x < 5; x++)
        {
          string += F("<option value='");
          string += optionValues[x];
          string += "'";
          if (choice == optionValues[x])
            string += F(" selected");
          string += ">";
          string += options[x];
          string += F("</option>");
        }
        string += F("</select>");

        switch (choice)
        {
          case PLUGIN_103_LISTEN_BOOT:
            {
              break;
            }
          case PLUGIN_103_LISTEN_TASKVALUE:
            {
              string += F("<TR><TD>Check Task:<TD>");
              addTaskSelect(string, "plugin_103_task", Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

              LoadTaskSettings(Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
              string += F("<TR><TD>Check Value:<TD>");
              addTaskValueSelect(string, "plugin_103_value", Settings.TaskDevicePluginConfig[event->TaskIndex][2], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

              string += F("<TR><TD>Set Value:<TD><input type='text' name='plugin_103_value1' value='");
              string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
              string += F("'>");
              string += F("<TR><TD>Hysteresis:<TD><input type='text' name='plugin_103_value2' value='");
              string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1];
              string += F("'>");
              LoadTaskSettings(event->TaskIndex);
              break;
            }
          case PLUGIN_103_LISTEN_TIMER:
            {
              string += F("<TR><TD>Timer nr:<TD><input type='text' name='plugin_103_value1' value='");
              string += (int)Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
              string += F("'>");
              break;
            }
          case PLUGIN_103_LISTEN_USEREVENT:
            {
              string += F("<TR><TD>Userevent nr:<TD><input type='text' name='plugin_103_value1' value='");
              string += (int)Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
              string += F("'>");
              break;
            }
          case PLUGIN_103_LISTEN_CLOCKEVENT:
            {
              string += F("<TR><TD>Day,Time:<TD><input type='text' name='plugin_103_value1' value='");
              string += timeLong2String(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0]);
              string += F("'>");
              break;
            }
        }

        byte actionchoice = ExtraTaskSettings.TaskDevicePluginConfigLong[0];
        String options2[5];
        options2[0] = F("");
        options2[1] = F("GPIO Set");
        options2[2] = F("Timer Set");
        options2[3] = F("Send UserEvent");
        options2[4] = F("Custom");
        int optionValues2[5];
        optionValues2[0] = 0;
        optionValues2[1] = 1;
        optionValues2[2] = 2;
        optionValues2[3] = 3;
        optionValues2[4] = 99;
        string += F("<TR><TD>On Action:<TD><select name='plugin_103_onaction' LANGUAGE=javascript onchange=\"return dept_onchange(frmselect)\">");
        for (byte x = 0; x < 5; x++)
        {
          string += F("<option value='");
          string += optionValues2[x];
          string += "'";
          if (actionchoice == optionValues2[x])
            string += F(" selected");
          string += ">";
          string += options2[x];
          string += F("</option>");
        }
        string += F("</select>");

        switch (actionchoice)
        {
          case 1:
            {
              sprintf_P(tmpString, PSTR("<TR><TD>GPIO Pin:<TD><input type='text' name='plugin_103_onactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
              string += tmpString;
              sprintf_P(tmpString, PSTR("<TR><TD>State:<TD><input type='text' name='plugin_103_onactionpar2' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
              string += tmpString;
              break;
            }
          case 2:
            {
              sprintf_P(tmpString, PSTR("<TR><TD>Timer Nr:<TD><input type='text' name='plugin_103_onactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
              string += tmpString;
              sprintf_P(tmpString, PSTR("<TR><TD>Timer Value:<TD><input type='text' name='plugin_103_onactionpar2' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
              string += tmpString;
              break;
            }
          case 3:
            {
              sprintf_P(tmpString, PSTR("<TR><TD>Userevent Nr:<TD><input type='text' name='plugin_103_onactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
              string += tmpString;
              break;
            }
          case 99:
            {
              sprintf_P(tmpString, PSTR("<TR><TD>Command:<TD><input type='text' maxlength='79' name='plugin_103_onactioncmd' value='%s'>"), custom.commandOn);
              string += tmpString;
              break;
            }
        }

        if (choice == PLUGIN_103_LISTEN_TASKVALUE)
        {
          byte actionchoice2 = ExtraTaskSettings.TaskDevicePluginConfigLong[4];
          string += F("<TR><TD>Off Action:<TD><select name='plugin_103_offaction' LANGUAGE=javascript onchange=\"return dept_onchange(frmselect)\">");
          for (byte x = 0; x < 5; x++)
          {
            string += F("<option value='");
            string += optionValues2[x];
            string += "'";
            if (actionchoice2 == optionValues2[x])
              string += F(" selected");
            string += ">";
            string += options2[x];
            string += F("</option>");
          }
          string += F("</select>");

          switch (actionchoice2)
          {
            case 1:
              {
                sprintf_P(tmpString, PSTR("<TR><TD>GPIO Pin:<TD><input type='text' name='plugin_103_offactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[5]);
                string += tmpString;
                sprintf_P(tmpString, PSTR("<TR><TD>State:<TD><input type='text' name='plugin_103_offactionpar2' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[6]);
                string += tmpString;
                break;
              }
            case 2:
              {
                sprintf_P(tmpString, PSTR("<TR><TD>Timer Nr:<TD><input type='text' name='plugin_103_offactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[5]);
                string += tmpString;
                sprintf_P(tmpString, PSTR("<TR><TD>Timer Value:<TD><input type='text' name='plugin_103_offactionpar2' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[6]);
                string += tmpString;
                break;
              }
            case 3:
              {
                sprintf_P(tmpString, PSTR("<TR><TD>Userevent Nr:<TD><input type='text' name='plugin_103_offactionpar1' value='%d'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[5]);
                string += tmpString;
                break;
              }
            case 99:
              {
                sprintf_P(tmpString, PSTR("<TR><TD>Command:<TD><input type='text' maxlength='79' name='plugin_103_offactioncmd' value='%s'>"), custom.commandOff);
                string += tmpString;
                break;
              }
          }
        } //endif choice

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_103_source");
        String plugin2 = WebServer.arg("plugin_103_task");
        String plugin3 = WebServer.arg("plugin_103_value");
        String plugin4 = WebServer.arg("plugin_103_value1");
        String plugin5 = WebServer.arg("plugin_103_value2");
        String plugin6 = WebServer.arg("plugin_103_onaction");
        String plugin7 = WebServer.arg("plugin_103_onactionpar1");
        String plugin8 = WebServer.arg("plugin_103_onactionpar2");
        String plugin9 = WebServer.arg("plugin_103_onactionpar3");
        String plugin10 = WebServer.arg("plugin_103_offaction");
        String plugin11 = WebServer.arg("plugin_103_offactionpar1");
        String plugin12 = WebServer.arg("plugin_103_offactionpar2");
        String plugin13 = WebServer.arg("plugin_103_offactionpar3");
        String plugin14 = WebServer.arg("plugin_103_onactioncmd");
        String plugin15 = WebServer.arg("plugin_103_offactioncmd");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != PLUGIN_103_LISTEN_CLOCKEVENT)
          Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = plugin4.toFloat();
        else
          Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = string2TimeLong(plugin4);

        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1] = plugin5.toFloat();
        ExtraTaskSettings.TaskDevicePluginConfigLong[0] = plugin6.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[1] = plugin7.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[2] = plugin8.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[3] = plugin9.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[4] = plugin10.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[5] = plugin11.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[6] = plugin12.toInt();
        ExtraTaskSettings.TaskDevicePluginConfigLong[7] = plugin13.toInt();
        SaveTaskSettings(event->TaskIndex);

        struct Plugin_103_Struct custom;
        strncpy(custom.commandOn, plugin14.c_str(), sizeof(custom.commandOn));
        strncpy(custom.commandOff, plugin15.c_str(), sizeof(custom.commandOff));
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&custom, sizeof(custom));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        // check if this task is set to "boot event" as source
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == PLUGIN_103_LISTEN_BOOT)
        {
          Serial.println(F("Boot Event"));
          Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[0], ExtraTaskSettings.TaskDevicePluginConfigLong[1], ExtraTaskSettings.TaskDevicePluginConfigLong[2], 1);
        }
        success = true;
        break;
      }

    case PLUGIN_TIMER_EVENT:
      {
        LoadTaskSettings(event->TaskIndex);
        Serial.print(F("Timer event "));
        Serial.println(event->Par1);
        Serial.print(F(" listen to "));
        Serial.println(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        // Par1 has timer number, stored in pluginconfigfloat[0]
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == PLUGIN_103_LISTEN_TIMER)
          if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] == event->Par1)
          {
            Serial.println(F("Timer event Hit!"));
            Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[0], ExtraTaskSettings.TaskDevicePluginConfigLong[1], ExtraTaskSettings.TaskDevicePluginConfigLong[2], 1);
          }
        break;
      }

    case PLUGIN_USER_EVENT:
      {
        LoadTaskSettings(event->TaskIndex);
        Serial.print(F("User event "));
        Serial.print(event->Par1);
        Serial.print(F(" listen to "));
        Serial.println(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        // Par1 has userevent number, stored in pluginconfigfloat[0]
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == PLUGIN_103_LISTEN_USEREVENT)
          if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] == event->Par1)
          {
            Serial.println(F("Userevent Hit!"));
            Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[0], ExtraTaskSettings.TaskDevicePluginConfigLong[1], ExtraTaskSettings.TaskDevicePluginConfigLong[2], 1);
          }
        break;
      }

#if FEATURE_TIME
    case PLUGIN_CLOCK_IN:
      {
        unsigned long clockEvent = (unsigned long)minute() % 10 | (unsigned long)(minute() / 10) << 4 | (unsigned long)(hour() % 10) << 8 | (unsigned long)(hour() / 10) << 12 | (unsigned long)weekday() << 16;
        unsigned long clockSet = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
        unsigned long Mask;
        for (byte y = 0; y < 8; y++)
        {
          if (((clockSet >> (y * 4)) & 0xf) == 0xf)  // if nibble y has the wildcard value 0xf
          {
            Mask = 0xffffffff  ^ (0xFUL << (y * 4)); // Mask to wipe nibble position y.
            clockEvent &= Mask;                      // clear nibble
            clockEvent |= (0xFUL << (y * 4));        // fill with wildcard value 0xf
          }
        }
        if (clockEvent == clockSet)
          if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == PLUGIN_103_LISTEN_CLOCKEVENT)
          {
            LoadTaskSettings(event->TaskIndex);
            Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[0], ExtraTaskSettings.TaskDevicePluginConfigLong[1], ExtraTaskSettings.TaskDevicePluginConfigLong[2], 1);
          }
        break;
      }
#endif

    case PLUGIN_ONCE_A_SECOND:
      {
        for (byte x = 0; x < TIMER_MAX; x++)
        {
          if (UserTimer[x] != 0L) // timer active?
          {
            if (UserTimer[x] < millis()) // timer finished?
            {
              UserTimer[x] = 0L; // turn off this timer
              struct EventStruct TempEvent;

              for (byte y = 0; y < TASKS_MAX; y++)
              {
                if (Settings.TaskDeviceNumber[y] != 0)
                {
                  byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[y]);
                  TempEvent.TaskIndex = y;
                  TempEvent.BaseVarIndex = y * VARS_PER_TASK;
                  TempEvent.idx = Settings.TaskDeviceID[y];
                  TempEvent.sensorType = Device[DeviceIndex].VType;
                  TempEvent.OriginTaskIndex = event->TaskIndex;
                  TempEvent.Par1 = x + 1; // Set timernr that fired
                  for (byte z = 0; z < PLUGIN_MAX; z++)
                  {
                    if (Plugin_id[z] == Settings.TaskDeviceNumber[y])
                    {
                      Serial.println(F("timer event out"));
                      Plugin_ptr[z](PLUGIN_TIMER_EVENT, &TempEvent, dummyString);
                    }
                  }
                }
              }

            }
          }
        }
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        // if listening to other task values
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == PLUGIN_103_LISTEN_TASKVALUE)
        {
          // we're checking a var from another task, so calculate that basevar
          byte TaskIndex = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          byte BaseVarIndex = TaskIndex * VARS_PER_TASK + Settings.TaskDevicePluginConfig[event->TaskIndex][2];
          float value = UserVar[BaseVarIndex];
          byte state = switchstate[event->TaskIndex];
          // compare with threshold value
          float valueLowThreshold = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] - (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1] / 2);
          float valueHighThreshold = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] + (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1] / 2);
          if (value <= valueLowThreshold)
            state = 1;
          if (value >= valueHighThreshold)
            state = 0;
          if (state != switchstate[event->TaskIndex])
          {
            switchstate[event->TaskIndex] = state;
            Serial.println(F("trigger"));
            LoadTaskSettings(event->TaskIndex);
            if (state == 0)
              Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[0], ExtraTaskSettings.TaskDevicePluginConfigLong[1], ExtraTaskSettings.TaskDevicePluginConfigLong[2], 1);
            else
              Plugin_103_Action(event->TaskIndex, ExtraTaskSettings.TaskDevicePluginConfigLong[4], ExtraTaskSettings.TaskDevicePluginConfigLong[5], ExtraTaskSettings.TaskDevicePluginConfigLong[6], 0);
          }
          success = true;
        }
        break;
      }

  }
  return success;
}


//********************************************************************************
// Perform configured actions
//********************************************************************************
void Plugin_103_Action(byte TaskIndex, byte cmd, int Par1, int Par2, byte state)
{
  Serial.print(F("Cmd: "));
  Serial.print(cmd);
  Serial.print(" ");
  Serial.print(Par1);
  Serial.print(",");
  Serial.println(Par2);
  switch (cmd)
  {
    case 1: // GPIO set
      pinMode(Par1, OUTPUT);
      digitalWrite(Par1, Par2);
      break;
    case 2: // Timer set
      UserTimer[Par1 - 1] = millis() + 1000 * Par2;
      break;
    case 3: // Send Userevent
      {
        struct EventStruct TempEvent;
        TempEvent.Par1 = Par1; // Set userevent
        for (byte y = 0; y < TASKS_MAX; y++)
        {
          if (Settings.TaskDeviceNumber[y] != 0)
          {
            byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[y]);
            TempEvent.TaskIndex = y;
            TempEvent.BaseVarIndex = y * VARS_PER_TASK;
            TempEvent.idx = Settings.TaskDeviceID[y];
            TempEvent.sensorType = Device[DeviceIndex].VType;
            TempEvent.OriginTaskIndex = TaskIndex;
            for (byte z = 0; z < PLUGIN_MAX; z++)
            {
              if (Plugin_id[z] == Settings.TaskDeviceNumber[y])
              {
                Serial.print(F("user event out from "));
                Serial.print(TaskIndex);
                Serial.print(F(" to "));
                Serial.print(y);
                Serial.print(F(" par1 "));
                Serial.println(Par1);
                Plugin_ptr[z](PLUGIN_USER_EVENT, &TempEvent, dummyString);
              }
            }
          }
        }
        break;
      }
    case 99: // Custom
      {
        String request;

        struct Plugin_103_Struct custom;
        LoadCustomTaskSettings(TaskIndex, (byte*)&custom, sizeof(custom));
        if (state == 1) // on action
          request = custom.commandOn;
        else // off action
          request = custom.commandOff;

        struct EventStruct TempEvent;
        char TmpStr1[80];
        TmpStr1[0] = 0;
        TempEvent.Par1 = 0;
        TempEvent.Par2 = 0;
        TempEvent.Par3 = 0;

        char command[80];
        request.toCharArray(command, 80);
        if (GetArgv(command, TmpStr1, 2)) TempEvent.Par1 = str2int(TmpStr1);
        if (GetArgv(command, TmpStr1, 3)) TempEvent.Par2 = str2int(TmpStr1);
        if (GetArgv(command, TmpStr1, 4)) TempEvent.Par3 = str2int(TmpStr1);

        Serial.print(F("Custom command: "));
        Serial.println(request);

        if (!PluginCall(PLUGIN_WRITE, &TempEvent, request))
          ExecuteCommand(command);

        break;
      }
  }
}

