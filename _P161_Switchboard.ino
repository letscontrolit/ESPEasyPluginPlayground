/*##########################################################################################
  ############################### Plugin 161: Switchboard plugin #######################
  ##########################################################################################

  Features :
	- Displays input/output devices states/values at a dedicated URL
    http://ipaddress/board  

  List of internal commands :
   - board?cmd=output,[devicename],[pin_number]  Send the output command to a device, than return status in html
   - board?cmd=toggle,[taskindex],[valuenumber]  Toggle state of taskindex numbered task specified value,
                                                  if devicepin defined, than toggle it also (for Output helper)
                                                  than return html

  Known bug:
   - if empty slots exists in between devices in device pages, the /json output causes refresh errors

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

/* Switchboard plugin for ESPEasy  */

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_161
#define PLUGIN_ID_161     161
#define PLUGIN_NAME_161   "Web - Switchboard"
#define PLUGIN_VALUENAME1_161 "Board"

boolean Plugin_161_init = false;
byte Plugin_161_taskindex;

boolean Plugin_161(byte function, struct EventStruct *event, String& string)
{

  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_161;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_161);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_161));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

          addFormNumericBox(F("Buttons per row"), F("plugin_161_btns"), Settings.TaskDevicePluginConfig[event->TaskIndex][0], 1, 8);
          addFormCheckBox(F("Display switch inputs"), F("Plugin_161_swi"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
          addFormCheckBox(F("Outputs as buttons"), F("Plugin_161_outs"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
          addFormCheckBox(F("Multi-Outputs as buttons"), F("Plugin_161_outm"), Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
          addFormCheckBox(F("Dummies as buttons"), F("Plugin_161_dum"), Settings.TaskDevicePluginConfig[event->TaskIndex][5]);
          addFormNumericBox(F("Last Task number to add"), F("Plugin_161_lt"), Settings.TaskDevicePluginConfig[event->TaskIndex][6], 1, TASKS_MAX+1);

        addFormNote(F("When activating this plugin the above selected devices will appear at http://IPADDRESS/board ."));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_161_btns"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = isFormItemChecked(F("Plugin_161_swi"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = isFormItemChecked(F("Plugin_161_outs"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = isFormItemChecked(F("Plugin_161_outm"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = isFormItemChecked(F("Plugin_161_dum"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][6] = getFormItemInt(F("Plugin_161_lt"));

        success = true;
        break;

      }
    case PLUGIN_INIT:
      {
        Plugin_161_taskindex = event->TaskIndex;

        WebServer.on("/board", p161_handle_board);

        Plugin_161_init = true;
        success = true;
        break;

      }


  }   // switch
  return success;

}     //function

//plugin specific procedures and functions

void p161_handle_board()
{
  if (Plugin_161_init)
  {
    String wcmnd = WebServer.arg("cmd");
    TXBuffer.startStream();
    // espeasy header start
    sendHeadandTail(F("TmplStd"), _HEAD);

    if (wcmnd.length() != 0) // execute command
    {
      String command = parseString(wcmnd, 1);
      String tasknum = parseString(wcmnd, 2);
      String valuenum = parseString(wcmnd, 3);
      if (command.equalsIgnoreCase(F("toggle"))) {
       byte TaskIndex1 = tasknum.toInt();
       byte ValueName1 = valuenum.toInt();
       LoadTaskSettings(TaskIndex1);              // using other devices variables as our own is dangerous.. but we are brave!
       byte BaseVarIndex1 = TaskIndex1 * VARS_PER_TASK;
       if (Settings.TaskDeviceEnabled[TaskIndex1]) {
          byte pinnum = -1;
          byte pinvalue = -1;
          switch(ValueName1)
          {
            case 0:
              pinnum = Settings.TaskDevicePin1[TaskIndex1];
              pinvalue = (byte)UserVar[BaseVarIndex1];
              pinvalue = !pinvalue;
              UserVar[BaseVarIndex1] = pinvalue;
             break;
            case 1:
              pinnum = Settings.TaskDevicePin2[TaskIndex1];
              pinvalue = (byte)UserVar[BaseVarIndex1+ValueName1];
              pinvalue = !pinvalue;
              UserVar[BaseVarIndex1+ValueName1] = pinvalue;
             break;
            case 2:
              pinnum = Settings.TaskDevicePin3[TaskIndex1];
              pinvalue = (byte)UserVar[BaseVarIndex1+ValueName1];
              pinvalue = !pinvalue;
              UserVar[BaseVarIndex1+ValueName1] = pinvalue;
             break;
            case 3:
              pinvalue = (byte)UserVar[BaseVarIndex1+ValueName1];
              pinvalue = !pinvalue;
              UserVar[BaseVarIndex1+ValueName1] = pinvalue;
             break;
          }
          if (pinnum >= 0 && pinnum <= PIN_D_MAX && pinvalue>=0 && pinvalue <= 1)
          {
            pinMode(pinnum, OUTPUT);
            digitalWrite(pinnum, pinvalue);
            setPinState(PLUGIN_ID_161, pinnum, PIN_MODE_OUTPUT, pinvalue);
            String logs = String(F("SWB  : GPIO ")) + String(pinnum) + String(F(" Set to ")) + String(pinvalue);
            addLog(LOG_LEVEL_INFO, logs);
          }
        }
      } // end of toggle command
      if (command.equalsIgnoreCase(F("output"))) {
        struct EventStruct TempEvent;
        parseCommandString(&TempEvent, wcmnd);
        TempEvent.Source = VALUE_SOURCE_HTTP;
        if (PluginCall(PLUGIN_WRITE, &TempEvent, wcmnd));
        String logs = F("SWB :");
        logs += String(wcmnd);
        addLog(LOG_LEVEL_INFO,logs);
      }
    }

    byte firstTaskIndex = 0;
    byte lastTaskIndex = TASKS_MAX - 1;
    if (Settings.TaskDevicePluginConfig[Plugin_161_taskindex][6]-1 < lastTaskIndex) {
      lastTaskIndex = Settings.TaskDevicePluginConfig[Plugin_161_taskindex][6]-1;
    }
    byte lastActiveTaskIndex = 0;
    for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {
      if (Settings.TaskDeviceNumber[TaskIndex] && Settings.TaskDeviceEnabled[TaskIndex])
        lastActiveTaskIndex = TaskIndex;
    }

    TXBuffer += F("<script>"); // DO NOT LEAVE EMPTY SLOTS AT DEVICE PAGE INSIDE OTHERS, ONLY AT THE END! CAUSES INTERESTING ERRORS!
    TXBuffer += F("function callcmd(cmdline) { commandurl = 'control?cmd='+cmdline; var xmlHttp = new XMLHttpRequest(); xmlHttp.open('GET', commandurl, true); xmlHttp.send(null);}");
    TXBuffer += F(" (function(){ var max_tasknumber = ");
    TXBuffer += String(lastActiveTaskIndex); // loop from 0 to lastactivetaskindex
    TXBuffer += F("; var max_taskvalues = 4; var timeForNext = 1500; var c; var k; var err = ''; var i = setInterval(function(){ var url = '/json';");
    TXBuffer += F("  fetch(url).then( function(response) {  if (response.status !== 200) { console.log('Looks like there was a problem. Status Code: ' +  response.status); return; } response.json().then(function(data) {");
    TXBuffer += F("timeForNext = data.TTL;for (c = 0; c <= max_tasknumber; c++) {for (k = 0; k < max_taskvalues; k++) {var cell = document.getElementById('value_' + c + '_' + k);");
    TXBuffer += F("if (cell){var taskvalue = data.Sensors[c].TaskValues[k].Value; cell.innerHTML= taskvalue;} var cell2 = document.getElementById('valuename_' + c + '_' + k);");
    TXBuffer += F("if (cell2){var taskvalue = data.Sensors[c].TaskValues[k].Value; if (taskvalue < 1) {cell2.classList.remove('on'); cell2.classList.add('off');} else {cell2.classList.remove('off'); cell2.classList.add('on');");
    TXBuffer += F("}}}}});}) ;}, timeForNext);})();");
    TXBuffer += F("</script>");
    TXBuffer += F("<style>.btn{margin-left:2px;margin-top:2px;float:left;display:block;border:1px solid black;border-radius:0.3rem;color:black;min-height:50px;font-size:1.2rem;width:");
    TXBuffer += String(100/(Settings.TaskDevicePluginConfig[Plugin_161_taskindex][0])-1); // TXBuffer += F("24");
    TXBuffer += F("%;-webkit-transition-duration:0.4s;transition-duration:0.4s;cursor:pointer;text-align:center;vertical-align:middle;text-decoration:none;}");
    TXBuffer += F(".on {background-color: rgba(0, 255, 0, 0.6) !important;} .off{background-color: rgba(255, 255, 0, 0.6) !important;}.btn:hover{color:blue;}</style>");

    TXBuffer  += F("<div style='width:100%;overflow:hidden;'>&nbsp;</div><div style='min-height: 100%;height: 100%;width:100%;'>");
    for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++)
    {
      if (Settings.TaskDeviceNumber[TaskIndex])
      {
        byte BaseVarIndex = TaskIndex * VARS_PER_TASK;
        byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);
        LoadTaskSettings(TaskIndex);
        if (Settings.TaskDeviceEnabled[TaskIndex]) {                   // tnum String(TaskIndex + 1)
          String devname = getPluginNameFromDeviceIndex(DeviceIndex);    // tname String(ExtraTaskSettings.TaskDeviceName)

          if ( ((devname.substring(0, 10).equalsIgnoreCase(F("output - d"))) && Settings.TaskDevicePluginConfig[Plugin_161_taskindex][3] ) ||
          ((devname.substring(0, 15).equalsIgnoreCase(F("generic - dummy"))) && Settings.TaskDevicePluginConfig[Plugin_161_taskindex][5]) ) {
            if (Device[DeviceIndex].ValueCount != 0) {
              for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++)
              {
               if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() > 0) {
                TXBuffer  += F("<a href='board?cmd=toggle,");
                TXBuffer  += String(TaskIndex) + F(",")+String(x);
                TXBuffer  += F("' class='btn ");
                if (UserVar[BaseVarIndex + x] < 1) {
                  TXBuffer  += F("off");
                } else {
                  TXBuffer  += F("on");
                }
                TXBuffer  += F("' id='valuename_");
                TXBuffer  += String(TaskIndex);
                TXBuffer  += F("_");
                TXBuffer  += String(x);
                TXBuffer  += F("'>");
                TXBuffer  += ExtraTaskSettings.TaskDeviceValueNames[x];
                TXBuffer  += F("</a>");
              }}


            }
          } // output end

          if ( (devname.substring(0, 14).equalsIgnoreCase(F("output - multi"))) && Settings.TaskDevicePluginConfig[Plugin_161_taskindex][4]) {
            if (Device[DeviceIndex].ValueCount != 0) {
              for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++)
              {
               if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() > 0) {
                TXBuffer  += F("<a href='board?cmd=output,");
                TXBuffer  += String(ExtraTaskSettings.TaskDeviceName) + F(",")+String(x);
                TXBuffer  += F("' class='btn ");
                if (UserVar[BaseVarIndex + x] < 1) {
                  TXBuffer  += F("off");
                } else {
                  TXBuffer  += F("on");
                }
                TXBuffer  += F("' id='valuename_");
                TXBuffer  += String(TaskIndex);
                TXBuffer  += F("_");
                TXBuffer  += String(x);
                TXBuffer  += F("'>");
                TXBuffer  += ExtraTaskSettings.TaskDeviceValueNames[x];
                TXBuffer  += F("</a>");
              }}


            }
          } // multi output end


        } // device enabled end
      }
    }
    TXBuffer  += F("<div style='width:100%;overflow:hidden;'>&nbsp;</div>");
    for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++)
    {
      if (Settings.TaskDeviceNumber[TaskIndex])
      {
        byte BaseVarIndex = TaskIndex * VARS_PER_TASK;
        byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);
        LoadTaskSettings(TaskIndex);
        if (Settings.TaskDeviceEnabled[TaskIndex]) {                   // tnum String(TaskIndex + 1)
          String devname = getPluginNameFromDeviceIndex(DeviceIndex);    // tname String(ExtraTaskSettings.TaskDeviceName)

          if ((devname.substring(0, 12).equalsIgnoreCase(F("switch input"))) && Settings.TaskDevicePluginConfig[Plugin_161_taskindex][1]) {
            if (Device[DeviceIndex].ValueCount != 0) {
              for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++)
              {
               if (String(ExtraTaskSettings.TaskDeviceValueNames[x]).length() >0)
               {
                TXBuffer  += F("<div class='btn ");
                if (UserVar[BaseVarIndex + x] < 1) {
                  TXBuffer  += F("off");
                } else {
                  TXBuffer  += F("on");
                }
                TXBuffer  += F("' id='valuename_");
                TXBuffer  += String(TaskIndex);
                TXBuffer  += F("_");
                TXBuffer  += String(x);
                TXBuffer  += F("'>");
                TXBuffer  += ExtraTaskSettings.TaskDeviceValueNames[x];

                TXBuffer  += F("<br><span id='value_");
                TXBuffer  += String(TaskIndex);
                TXBuffer  += F("_");
                TXBuffer  += String(x);
                TXBuffer  += F("'>");
                TXBuffer  += toString(UserVar[BaseVarIndex + x], ExtraTaskSettings.TaskDeviceValueDecimals[x]);
                TXBuffer  += F("</span></div>");
               }
              }

            }
          } // switch input end

        } // device enabled end
      }
    }
    TXBuffer  += F("</div>");
    sendHeadandTail(F("TmplStd"), _TAIL);
    TXBuffer.endStream();
  } // end of handler
}

#endif
