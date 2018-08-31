#ifdef PLUGIN_BUILD_TESTING
//#######################################################################################################
//#################################### Plugin 159: Pushbutton    ########################################
//# Multiple pushbutton in one plugin, with ShortPress and LongPress detection,
//# can be detected by rule events as buttonname#Shortpress and butonname#Longpress=button_down_time
//#######################################################################################################

#define PLUGIN_159
#define PLUGIN_ID_159        159
#define PLUGIN_NAME_159       "Switch input - Pushbutton"
#define PLUGIN_VALUENAME1_159 "I1"
#define PLUGIN_VALUENAME2_159 ""
#define PLUGIN_VALUENAME3_159 ""
#define PLUGIN_VALUENAME4_159 ""

#define P159_MaxInstances 3

boolean Plugin_159_init = false;

boolean Plugin_159(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte Plugin_159_pinstate[P159_MaxInstances][4];
  static unsigned long Plugin_159_buttons[P159_MaxInstances][4];

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_159;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = true;
        Device[deviceCount].InverseLogicOption = true;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_159);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_159));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_159));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_159));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_159));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        //addFormPinSelect(F("Relay 1"), F("taskdevicepin1"), Settings.TaskDevicePin1[event->TaskIndex]);
        //addFormPinSelect(F("Relay 2"), F("taskdevicepin2"), Settings.TaskDevicePin2[event->TaskIndex]);
        //addFormPinSelect(F("Relay 3"), F("taskdevicepin3"), Settings.TaskDevicePin3[event->TaskIndex]);
        LoadTaskSettings(event->TaskIndex);
        addFormPinSelect(F("4th GPIO"), F("taskdevicepin4"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);

        int choice = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String buttonOptions[6];
        buttonOptions[0] = F("0.8s");
        buttonOptions[1] = F("1s");
        buttonOptions[2] = F("1.5s");
        buttonOptions[3] = F("2s");
        buttonOptions[4] = F("3s");
        buttonOptions[5] = F("4s");
        int buttonoptionValues[6] = { 800, 1000, 1500, 2000, 3000, 4000 };
        addFormSelector(F("Longpress time"), F("plugin_159_lpt"), 6, buttonOptions, buttonoptionValues, choice);

        addFormNote(F("You can specify 4 normal input or pull-up input pin's above. Default push button active LOW, else choose inverted."));

        byte baseaddr = 0;
        if (event->TaskIndex > 0) {
          for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
          {
            if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_159) {
              baseaddr = baseaddr + 1;
            }
          }
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = baseaddr;

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("taskdevicepin4"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_159_lpt"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        byte baseaddr = 0;
        if (event->TaskIndex > 0) {
          for (byte TaskIndex = 0; TaskIndex < event->TaskIndex; TaskIndex++)
          {
            if (Settings.TaskDeviceNumber[TaskIndex] == PLUGIN_ID_159) {
              baseaddr = baseaddr + 1;
            }
          }
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = baseaddr;        

        if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
        {
          if (Settings.TaskDevicePin1PullUp[event->TaskIndex]) {
            pinMode(Settings.TaskDevicePin1[event->TaskIndex], INPUT_PULLUP);
          } else {
            pinMode(Settings.TaskDevicePin1[event->TaskIndex], INPUT);
          }
          Plugin_159_pinstate[baseaddr][0] = !Settings.TaskDevicePin1Inversed[event->TaskIndex];
        }
        if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
        {
          if (Settings.TaskDevicePin1PullUp[event->TaskIndex]) {
            pinMode(Settings.TaskDevicePin2[event->TaskIndex], INPUT_PULLUP);
          } else {
            pinMode(Settings.TaskDevicePin2[event->TaskIndex], INPUT);
          }
          Plugin_159_pinstate[baseaddr][1] = !Settings.TaskDevicePin1Inversed[event->TaskIndex];
          event->sensorType = SENSOR_TYPE_DUAL;
        }
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          if (Settings.TaskDevicePin1PullUp[event->TaskIndex]) {
            pinMode(Settings.TaskDevicePin3[event->TaskIndex], INPUT_PULLUP);
          } else {
            pinMode(Settings.TaskDevicePin3[event->TaskIndex], INPUT);
          }
          Plugin_159_pinstate[baseaddr][2] = !Settings.TaskDevicePin1Inversed[event->TaskIndex];
          event->sensorType = SENSOR_TYPE_TRIPLE;
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != -1)
        {
          if (Settings.TaskDevicePin1PullUp[event->TaskIndex]) {
            pinMode(Settings.TaskDevicePluginConfig[event->TaskIndex][0], INPUT_PULLUP);
          } else {
            pinMode(Settings.TaskDevicePluginConfig[event->TaskIndex][0], INPUT);
          }
          Plugin_159_pinstate[baseaddr][3] = !Settings.TaskDevicePin1Inversed[event->TaskIndex];
          event->sensorType = SENSOR_TYPE_QUAD;
        }
        UserVar[event->BaseVarIndex] = 0;
        UserVar[event->BaseVarIndex + 1] = 0;
        UserVar[event->BaseVarIndex + 2] = 0;
        UserVar[event->BaseVarIndex + 3] = 0;

        String logs = String(F("PB : Task:")) + String(event->TaskIndex) + F(" instance ") + String(Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        addLog(LOG_LEVEL_INFO, logs);

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][2] < P159_MaxInstances) {
          success = true;
          Plugin_159_init = true;
        } else {
          success = false;
          logs = String(F("PB : Task init failed"));
          addLog(LOG_LEVEL_INFO, logs);
        }
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        unsigned long current_time;
        byte state = 0;
        boolean changed = false;
        String logs = F("");
        if (Plugin_159_init) {
          if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
          {
            state = digitalRead(Settings.TaskDevicePin1[event->TaskIndex]);
            if (Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][0] != state) { //status changed
              current_time = millis();
              if (state == Settings.TaskDevicePin1Inversed[event->TaskIndex]) // button pushed
              {
                Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][0] = current_time; // push started at
              } else { // button released
                LoadTaskSettings(event->TaskIndex);
                current_time = (current_time - Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][0]); // push duration
                if (current_time < Settings.TaskDevicePluginConfig[event->TaskIndex][1]) { // short push
                  UserVar[event->BaseVarIndex] = !UserVar[event->BaseVarIndex];
                  changed = true;
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[0]);
                  events += F("#Shortpress");
                  rulesProcessing(events);
                } else { // long push
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[0]);
                  events += F("#Longpress=");
                  events += String(current_time);
                  rulesProcessing(events);
                }
              }
              Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][0] = state;
            }
          } // btn1


          if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
          {
            state = digitalRead(Settings.TaskDevicePin2[event->TaskIndex]);
            if (Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][1] != state) { //status changed
              current_time = millis();
              if (state == Settings.TaskDevicePin1Inversed[event->TaskIndex]) // button pushed
              {
                Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][1] = current_time; // push started at
              } else { // button released
                LoadTaskSettings(event->TaskIndex);
                current_time = (current_time - Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][1]); // push duration
                if (current_time < Settings.TaskDevicePluginConfig[event->TaskIndex][1]) { // short push
                  UserVar[event->BaseVarIndex+1] = !UserVar[event->BaseVarIndex+1];
                  changed = true;
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[1]);
                  events += F("#Shortpress");
                  rulesProcessing(events);
                } else { // long push
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[1]);
                  events += F("#Longpress=");
                  events += String(current_time);
                  rulesProcessing(events);
                }
              }
              Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][1] = state;
            }
          } // btn2


          if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
          {
            state = digitalRead(Settings.TaskDevicePin3[event->TaskIndex]);
            if (Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][2] != state) { //status changed
              current_time = millis();
              if (state == Settings.TaskDevicePin1Inversed[event->TaskIndex]) // button pushed
              {
                Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][2] = current_time; // push started at
              } else { // button released
                LoadTaskSettings(event->TaskIndex);
                current_time = (current_time - Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][2]); // push duration
                if (current_time < Settings.TaskDevicePluginConfig[event->TaskIndex][1]) { // short push
                  UserVar[event->BaseVarIndex+2] = !UserVar[event->BaseVarIndex+2];
                  changed = true;
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[2]);
                  events += F("#Shortpress");
                  rulesProcessing(events);
                } else { // long push
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[2]);
                  events += F("#Longpress=");
                  events += String(current_time);
                  rulesProcessing(events);
                }
              }
              Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][2] = state;
            }
          } // btn3

          if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] != -1)
          {
            state = digitalRead(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
            if (Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][3] != state) { //status changed
              LoadTaskSettings(event->TaskIndex);
              current_time = millis();
              if (state == Settings.TaskDevicePin1Inversed[event->TaskIndex]) // button pushed
              {
                Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][3] = current_time; // push started at
              } else { // button released
                current_time = (current_time - Plugin_159_buttons[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][3]); // push duration
                if (current_time < Settings.TaskDevicePluginConfig[event->TaskIndex][1]) { // short push
                  UserVar[event->BaseVarIndex+3] = !UserVar[event->BaseVarIndex+3];
                  changed = true;
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[3]);
                  events += F("#Shortpress");
                  rulesProcessing(events);
                } else { // long push
                  String events = String(ExtraTaskSettings.TaskDeviceValueNames[3]);
                  events += F("#Longpress=");
                  events += String(current_time);
                  rulesProcessing(events);
                }
              }
              Plugin_159_pinstate[Settings.TaskDevicePluginConfig[event->TaskIndex][2]][3] = state;
            }
          } // btn4

          if (changed)
          {
            logs += F("PB: State changed");
            logs += state;
            addLog(LOG_LEVEL_INFO, logs);
            sendData(event);
          }

          success = true;
          break;
        }
      }

    case PLUGIN_READ:
      {
        success = true;
        break;

      }
            
  }
  return success;  
}
#endif 
