//***********************************************************************************************//
//    HOMEASSISTANT DISCOVERY
//***********************************************************************************************//
/* by michael baeck
    plugin searches for active mqtt-enabled taskvalues and pushes json payloads 
    for homeassistant to take for discovery.
    1 value = 1 sensor. whole unit (ESP) = 1 device.

    classification of sensor (motion, light, temp, etc.) can be done in webform.

    devices can also be deleted again but homeassistant refuses to remove them from its store.
    (but it behaves the same when I publish the messages manually...)

    this is my first arduino work and first commit ever to anything, but it's pretty useful already.
    testing and feedback highly appreciated!


    [*] stay tuned for more plugins like (homeassistant-switch, etc)

    supported components
      [X] binary_sensor [1/0]
          0 - battery [low/norm]
          1 - cold [cold/norm]
          2 - connectivity [conn/disco]
          3 - door [open/close]
          4 - garage_door [open/close]
          5 - gas [gas/clear]
          6 - heat [hot/norm]
          7 - light [light/dark]
          8 - lock [unlocked/locked]
          9 - moisture [wet/dry]
          10 - motion [motion/clear]
          11 - moving [moving/stopped]
          12 - occupancy [occ/clear]
          13 - opening [open/close]
          14 - plug [plug/unplug]
          15 - power [power/none]
          16 - presesnce [home/away]
          17 - problem [problem/ok]
          18 - safety [unsafe/safe]
          19 - smoke [smoke/clear]
          20 - sound [sound/clear]
          21 - vibration [vibration/clear]
          22 - window [open/close]
      [X] sensor
          0 - battery
          1 - humimdity
          2 - illuminance
            - unit_of_measurement lx or lm //[todo]
          3 - temperature
            - unit_of_measurement °C or °F //[todo]
          4 - pressure
            - unit_of_measurement hPa or mbar //[todo]
      [ ] camera
      [ ] cover
          - damper
          - garage
          - window
      [ ] fan
      [ ] climate
      [ ] light
      [ ] lock
      [ ] switch
*/

//#ifdef PLUGIN_BUILD_DEVELOPMENT
//#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_126
#define PLUGIN_ID_126     126     
#define PLUGIN_NAME_126   "Generic - Homeassistant Discovery [DEVELOPMENT]"  
//#define PLUGIN_119_DEBUG  false    

//***********************************************************************************************//
  // store these values
  // prefix
  // task[12]
    // value[4]
      // bool enable
      // byte option

String p126log;
struct DiscoveryStruct {
  int taskid;
  int ctrlid;
  String publish;
  String lwttopic;
  String lwtup;
  String lwtdown;
  struct savestruct {
    char prefix[41];
    struct taskstruct {
      byte taskid;
      struct valuestruct {
        bool enable = false;
        byte option;
      } value[4];
    } task[TASKS_MAX];
  } save;

};

//***********************************************************************************************//
boolean Plugin_126(byte function, struct EventStruct *event, String& string) {

  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number = PLUGIN_ID_126;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
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

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      //no values
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {

      struct DiscoveryStruct discovery;
      LoadCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

      String tmpnote;
      //===================================================================================================================================
      tmpnote =  F("This plugin will push sensor configs that are compliant to home-assistant's discovery feature.");
      tmpnote += F("Tasks must have a MQTT controller enabled. It creates one additional sensor for the ESP node itself");
      tmpnote += F("which contains system info like wifi-signal, ip-address, etc.");
      tmpnote += F("Interval setting on bottom defines update frequency of that sensor.");
      addFormNote(tmpnote);
      

      //===================================================================================================================================
      addFormSubHeader(F("Settings"));
      
      if (String(discovery.save.prefix) == "") strncpy(discovery.save.prefix, "homeassistant",14);

      addFormTextBox(String(F("discovery prefix ")), String(F("Plugin_126_prefix")), discovery.save.prefix, 40);
      addFormNote(F("Change discovery topic here, if changed in home-assistant. Defaults to \"homeassistant\"."));

      find_sensors(&discovery);


      //===================================================================================================================================
      addFormSubHeader(F("CONTROL"));

      tmpnote = F("Configs will NOT be pushed automatically. Once plugin is saved, you can use the buttons below.");
      tmpnote += F("You can trigger actions via these commands as well: \"discovery,update\" and \"discovery,delete\"");
      addFormNote(tmpnote);

      if (Settings.TaskDeviceEnabled[event->TaskIndex]) {
        html_BR();
        tmpnote = F("http://");
        tmpnote += WiFi.localIP().toString();
        tmpnote += F("/tools?cmd=discovery%2Cupdate");
        addButton(tmpnote, "Update Configs");
        tmpnote = F("http://");
        tmpnote += WiFi.localIP().toString();
        tmpnote += F("/tools?cmd=discovery%2Cdelete");
        addButton(tmpnote, "Delete Configs");
        // tmpnote = F("http://");
        // tmpnote += WiFi.localIP().toString();
        // tmpnote += F("/tools?cmd=discovery%2Ccleanup");
        // addButton(tmpnote, "Full cleanup");
        html_BR();
      }


      //===================================================================================================================================
      addFormSubHeader(F("NOTES"));
      
      tmpnote = F("known issues: deletion of sensors in home-assistant is unreliable; espeasy limits mqtt message size to ");
      tmpnote += String(MQTT_MAX_PACKET_SIZE);
      tmpnote += F(", check logs, if this affects you");
      addFormNote(tmpnote);

      tmpnote = F("v0.2.2; struct size: ");
      tmpnote += String(sizeof(discovery.save));
      tmpnote += F("Byte; event->TaskIndex: ");
      tmpnote += String(event->TaskIndex);
      get_id(&discovery);
      tmpnote += F("; stored TaskIndex: ");
      tmpnote += discovery.taskid;
      addFormNote(tmpnote);
      //===================================================================================================================================

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      struct DiscoveryStruct discovery;

      String argName;
      argName = F("Plugin_126_prefix");
      strncpy(discovery.save.prefix, WebServer.arg(argName).c_str(), sizeof(discovery.save.prefix));

      find_sensors(&discovery, true);
      p126log = F("P[126] search for sensor completed: state of event->TaskIndex is : ");
      p126log += String(event->TaskIndex);
      addLog(LOG_LEVEL_DEBUG,p126log);
        //Serial.println(p126log);
      
      SaveCustomTaskSettings(event->TaskIndex, (byte*)&discovery.save, sizeof(discovery.save));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      //this case defines code to be executed when the plugin is initialised
      
      //system_config();
      //after the plugin has been initialised successfuly, set success and break
      success = true;
      break;

    }

    case PLUGIN_READ:
    {
      //code to be executed to read data
      //It is executed according to the delay configured on the device configuration page, only once

      //after the plugin has read data successfuly, set success and break

      struct DiscoveryStruct discovery;
      get_ctrl(&discovery);
      success = system_state(&discovery, false);

      break;

    }

    case PLUGIN_WRITE:
    {
      //this case defines code to be executed when the plugin executes an action (command).
      //Commands can be accessed via rules or via http.
      //As an example, http://192.168.1.12//control?cmd=dothis
      //implies that there exists the comamnd "dothis"

      

      
      
      //if (plugin_not_initialised)
        //break;

      //parse string to extract the command
      String tmpString  = string;
      int argIndex = tmpString.indexOf(',');

      if (argIndex)
        tmpString = tmpString.substring(0, argIndex);

      if (tmpString.equalsIgnoreCase(F("discovery"))) {
        p126log = F("P[126] command issued: state of string is : ");
        p126log += string;
        // Serial.println(p126log);
        addLog(LOG_LEVEL_INFO,p126log);

        struct DiscoveryStruct discovery;
        get_id(&discovery);
        get_ctrl(&discovery);
        // Serial.println(F("taskid:"));
        // Serial.println(discovery.taskid);
        // Serial.println(F("event taskid:"));
        // Serial.println(event->TaskIndex);
        LoadCustomTaskSettings(discovery.taskid, (byte*)&discovery.save, sizeof(discovery.save));

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
          cleanup(&discovery);  //function produces lots of MQTT messages; need to find a way to split/slow down
        }
        success = true; 
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
      //code to be executed once a second. Tasks which do not require fast response can be added here

      success = true;

    }

  }  
  return success;
}  

//***********************************************************************************************//
void find_sensors(struct DiscoveryStruct *discovery) {
  find_sensors(discovery, false);
}
void find_sensors(struct DiscoveryStruct *discovery, bool save) {
  p126log = F("P[126] search active tasks: state of save is : ");
  p126log += String(save);
  addLog(LOG_LEVEL_DEBUG,p126log);
        // Serial.println(p126log);

  String sensorclass[34];
    sensorclass[0] = F("------SENSOR-------");
    sensorclass[1] = F("battery");
    sensorclass[2] = F("humidity");
    sensorclass[3] = F("illuminance");
    sensorclass[4] = F("temperature");
    sensorclass[5] = F("pressure");
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
    sensorclass[33] = F("window");

  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;
  byte lastActiveTaskIndex = 0;

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {            // find last task
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
            p126log = F("P[126] found active task with id : ");
            p126log += String(TaskIndex);
            addLog(LOG_LEVEL_DEBUG,p126log);
            // Serial.println(p126log);
            
            if (!save) {
              String header = F("Task ");
              header += String(TaskIndex + 1);
              header += F(" \"");
              header += String(ExtraTaskSettings.TaskDeviceName);
              header += F("\" values:");
              addFormSubHeader(header);
            }

            for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++) {                           // for each value
              p126log = F("P[126] found value with name : ");
              p126log += String(ExtraTaskSettings.TaskDeviceValueNames[x]);
              addLog(LOG_LEVEL_DEBUG,p126log);
              // Serial.println(p126log);
              
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

              //bool enable = true;
              if (!save) {
                String enablestr = String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                enablestr += F(": push config");
                String optionstr = String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                optionstr += F(": sensor type");

                addFormCheckBox(enablestr, enableid, discovery->save.task[TaskIndex].value[x].enable);
                addFormSelector(optionstr, optionid, 34, sensorclass, NULL, discovery->save.task[TaskIndex].value[x].option);
              } else {
                int defaultoption;
                if (DeviceIndex == 1) {
                  defaultoption = 10;
                } else {
                  defaultoption = 0;
                }
                p126log = F("P[126] saving Settings: state of getFormItemInt(optionid, defaultoption) is : ");
                p126log += String(getFormItemInt(optionid, defaultoption));
                addLog(LOG_LEVEL_DEBUG,p126log);
                // Serial.println(p126log);
                
                discovery->save.task[TaskIndex].value[x].enable = isFormItemChecked(enableid);
                discovery->save.task[TaskIndex].value[x].option = getFormItemInt(optionid, defaultoption);
              }
            }
          }
        }
      }
    }
  }
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

  String state_topic = String(Settings.Name); 
  state_topic += F("/system/state");
  p126log = F("P[126] system status topic defined: state of state_topic is : ");
  p126log += state_topic;
  addLog(LOG_LEVEL_DEBUG,p126log);
    // Serial.println(p126log);
  

  String discovery_topic = String(discovery->save.prefix);
  discovery_topic += F("/sensor/");   
  discovery_topic += String(Settings.Name); 
  discovery_topic += F("/config");
  p126log = F("P[126] system status config topic defined: state of discovery_topic is : ");
  p126log += discovery_topic;
  addLog(LOG_LEVEL_DEBUG,p126log);
    // Serial.println(p126log);
  

  String uniquestr = WiFi.macAddress();
  uniquestr.replace(F(":"),F(""));
  uniquestr += F("_state");
  p126log = F("P[126] unique id created : ");
  p126log += discovery_topic;
  addLog(LOG_LEVEL_DEBUG,p126log);

  //entity data
  String payload = F("{");
  payload += add_line(F("name"), String(Settings.Name));
  payload += add_line(F("ic"), F("mdi:chip"));
  payload += add_line(F("stat_t"), state_topic);
  payload += add_line(F("val_tpl"), F("{{ value_json.state }}"));
  payload += add_line(F("uniq_id"), uniquestr);
  payload += add_line(F("avty_t"), discovery->lwttopic);
  payload += add_line(F("pl_avail"), discovery->lwtup);
  payload += add_line(F("pl_not_avail"), discovery->lwtdown);
  if (brief) {
    payload += F("\"json_attr\":[\"unit\",\"uptime\",\"ip\",\"rssi\"],");
  } else {
    payload += F("\"json_attr\":[\"unit\",\"version\",\"uptime\",\"cpu\",\"hostname\",\"ip\",\"mac\",\"ssid\",\"bssid\",\"rssi\",\"last_disconnect\",\"last_boot_cause\"],");
  }
  payload += add_device(brief);
  payload += F("}");
                       
  if (check_length(discovery_topic, payload)) {
    return (publish(discovery->ctrlid, discovery_topic, payload));
  } else if (!brief) {
    addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying again with reduced content."));
    return (system_config(discovery, true));
  } else {
    p126log = F("P[126] Cannot publish config, because payload exceeds limits. You can publish the message manually from other client.: state of payload is : ");
    p126log += payload;
    addLog(LOG_LEVEL_ERROR,p126log);
      // Serial.println(p126log);
    
    return false;
  }
}  

bool sensor_config(struct DiscoveryStruct *discovery, bool brief) {
  bool success = false;

  String sensorclass[34];
    sensorclass[0] = F("------SENSOR-------");
    sensorclass[1] = F("battery");
    sensorclass[2] = F("humidity");
    sensorclass[3] = F("illuminance");
    sensorclass[4] = F("temperature");
    sensorclass[5] = F("pressure");
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
    sensorclass[33] = F("window");

  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;
  byte lastActiveTaskIndex = 0;

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {            // find last task
    if (Settings.TaskDeviceNumber[TaskIndex]) lastActiveTaskIndex = TaskIndex;
  }
  
  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastActiveTaskIndex; TaskIndex++) {      // for each task
    if (Settings.TaskDeviceNumber[TaskIndex]) {                                              // if task exists
      byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);
      LoadTaskSettings(TaskIndex);

      if (Settings.TaskDeviceEnabled[TaskIndex]) {                                             // if task enabled
        bool ctrlenabled = false;
        for (byte x = 0; x < CONTROLLER_MAX; x++) {                                             // if any controller enabled //[TODO] only if specific controller matches
          if (Settings.TaskDeviceSendData[x][TaskIndex]) ctrlenabled = true;
        }

        if (ctrlenabled) {
          if (Device[DeviceIndex].ValueCount != 0) {                                            // only tasks with values
            String tasktopic = String(discovery->publish);
            tasktopic.replace(F("%tskname%"), ExtraTaskSettings.TaskDeviceName);

            for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++) {                         // for each value
              if (discovery->save.task[TaskIndex].value[x].enable) {  
                success = false;
                String state_topic = tasktopic;
                state_topic.replace(F("%valname%"), ExtraTaskSettings.TaskDeviceValueNames[x]);     // esp00/light/lux

                String sensorname = String(Settings.Name);                                          // sensor.esp00_light_lux
                sensorname += F("_");
                sensorname += String(ExtraTaskSettings.TaskDeviceName);
                sensorname += F("_");
                sensorname += String(ExtraTaskSettings.TaskDeviceValueNames[x]);

                String component;
                if (discovery->save.task[TaskIndex].value[x].option < 10) {
                  component = F("/sensor/");
                } else {
                  component = F("/binary_sensor/");
                }

                String uniquestr = WiFi.macAddress();                 // 00AA11BB22CC_1_1
                uniquestr.replace(F(":"),F(""));
                uniquestr += F("_");
                uniquestr += String(TaskIndex);
                uniquestr += F("_");
                uniquestr += String(x);

                String discovery_topic = String(discovery->save.prefix);                                    // homeasistant/sensor/00AA11BB22CC_1_1/config
                discovery_topic += component;
                discovery_topic += uniquestr;
                discovery_topic += F("/config");

                //entity data
                //add_line(F("ic"), F("mdi:chip"));
                //add_line(F("cmd_t"), _____________);
                //add_line(F("stat_clsd"), 0);
                //add_line(F("stat_off"), 0);
                //add_line(F("stat_on"), 1);
                //add_line(F("stat_open"), 1);
                //add_line(F("pl_off"), 0);       //binary, switch
                //add_line(F("pl_on"), 1);        //binary, switch
                //add_line(F("exp_aft", _______));                                            //[TODO]
                //add_line(F("unit_of_meas", _______));                                            //[TODO]
                //payload += F("\"json_attr\":[\"type\",\"interval\"],");
                //next_line(F("TaskInterval"), String(taskInterval));
                //next_line(F("Type"), getPluginNameFromDeviceIndex(DeviceIndex));
                String payload = F("{");
                payload += add_line(F("name"), sensorname);
                switch (discovery->save.task[TaskIndex].value[x].option) {
                  case 0:
                  case 6:
                  case 7:
                  case 8:
                  case 9:
                  case 10:
                    payload += add_line(F("val_tpl"), F("{{ value|float }}"));
                    break;
                  default:
                    payload += add_line(F("dev_cla"), sensorclass[discovery->save.task[TaskIndex].value[x].option]);
                    break;
                }
                payload += add_line(F("stat_t"), state_topic);
                payload += add_line(F("uniq_id"), uniquestr);
                payload += add_line(F("avty_t"), discovery->lwttopic);
                payload += add_line(F("pl_avail"), discovery->lwtup);
                payload += add_line(F("pl_not_avail"), discovery->lwtdown);
                payload += add_line(F("frc_upd"), F("true"));
                payload += add_device(brief);
                payload += F("}");
                    
                if (check_length(discovery_topic, payload)) {
                  success = publish(discovery->ctrlid, discovery_topic, payload);
                } else if (!brief) {
                  addLog(LOG_LEVEL_ERROR, F("P[126] Payload exceeds limits. Trying again with reduced content."));
                  success = system_config(discovery, true);
                } else {
                  p126log = F("P[126] Cannot publish config, because payload exceeds limits. You can publish the message manually from other client.: state of payload is : ");
                  p126log += payload;
                  addLog(LOG_LEVEL_ERROR,p126log);
                  
                  return false;
                }
              }
            } 
          }  
        }
      }
    } 
  } 
  return success;
} 

bool system_state(struct DiscoveryStruct *discovery, bool brief) {

  String state_topic = String(Settings.Name);
  state_topic += F("/system/state");

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

  String discovery_topic = String(discovery->save.prefix);
  discovery_topic += F("/sensor/");        
  discovery_topic += String(Settings.Name); 
  discovery_topic += F("/config");

  return (publish(discovery->ctrlid, discovery_topic, ""));
}

bool cleanup(struct DiscoveryStruct *discovery) {
  bool success1 = delete_system(discovery);
  bool success2 = false;
  bool success3 = false;

  byte firstTaskIndex = 0;
  byte lastTaskIndex = TASKS_MAX - 1;

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {
    for (byte x = 0; x = 3; x++) {                       // for each value
      success2 = false;
      success3 = false;

      String uniquestr = WiFi.macAddress();                 // 00AA11BB22CC_1_1
      uniquestr.replace(F(":"),F(""));
      uniquestr += F("_");
      uniquestr += String(TaskIndex);
      uniquestr += F("_");
      uniquestr += String(x);

      String discovery_topic = String(discovery->save.prefix);                                    // homeasistant/sensor/00AA11BB22CC_1_1/config
      discovery_topic += F("/sensor/");
      discovery_topic += uniquestr;
      discovery_topic += F("/config");

      success2 = publish(discovery->ctrlid, discovery_topic, "");

      discovery_topic = String(discovery->save.prefix);                                    // homeasistant/binary_sensor/00AA11BB22CC_1_1/config
      discovery_topic += F("/binary_sensor/");
      discovery_topic += uniquestr;
      discovery_topic += F("/config");

      success3 = publish(discovery->ctrlid, discovery_topic, "");
    }   
  }
  return (success1 && success2 && success3);
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

              String uniquestr = WiFi.macAddress();                 // 00AA11BB22CC_1_1
              uniquestr.replace(F(":"),F(""));
              uniquestr += F("_");
              uniquestr += String(TaskIndex);
              uniquestr += F("_");
              uniquestr += String(x);

              String sensorname = String(ExtraTaskSettings.TaskDeviceName);
              sensorname += F("_");
              sensorname += String(ExtraTaskSettings.TaskDeviceValueNames[x]);

              String component;
              if (discovery->save.task[TaskIndex].value[x].option < 10) {
                component = F("/sensor/");
              } else {
                component = F("/binary_sensor/");
              }

              String discovery_topic = String(discovery->save.prefix);                                    // homeasistant/sensor/00AA11BB22CC_1_1/config
              discovery_topic += component;
              discovery_topic += uniquestr;
              discovery_topic += F("/config");

              success2 = publish(discovery->ctrlid, discovery_topic, "");
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
  String buildstr = "EspEasy ";
  buildstr += BUILD_NOTES;
  buildstr += F(" (");
  if (String(BUILD_GIT) == "") {
    buildstr += String(CRCValues.compileDate);
  } else {
    buildstr += String(BUILD_GIT);
  }
  buildstr += F(")");

  //device data
  String device = F("\"device\":{");
    if (!brief) { 
      device += add_line(F("sw_version"), buildstr);
      device += add_line(F("manufacturer"), F("DIY"));
      device += add_line(F("model"), ARDUINO_BOARD);
      //connections
      device += F("\"connections\":[");
        device += F("[\"mac\",\""); 
        device += WiFi.macAddress();
        device += F("\"]");

        device += F(",[\"ip\",\""); 
        device += WiFi.localIP().toString();
        device += F("\"]");
        
        device += F(",[\"ssid\",\""); 
        device += WiFi.SSID();
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
      device += WiFi.macAddress();
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

  p126log = F("P[126] checking payload size: state of flength is : ");
  p126log += String(flength);
  addLog(LOG_LEVEL_DEBUG,p126log);
    // Serial.println(p126log);
  p126log = F("P[126] state of MQTT_MAX_PACKET_SIZE is : ");
  p126log += String(MQTT_MAX_PACKET_SIZE);
  addLog(LOG_LEVEL_DEBUG,p126log);
    // Serial.println(p126log);
  

  if (MQTT_MAX_PACKET_SIZE >= flength) {
    return true;
  } else {
    return false;
  }
  
}

bool publish(int ctrlid, const String& topic, const String& payload) {
  bool success = false;
  p126log = F("P[126] trying to publish payload: state of topic is : ");
  p126log += topic;
  addLog(LOG_LEVEL_DEBUG,p126log);
    // Serial.println(p126log);
  
  if (MQTTCheck(ctrlid)) {
    p126log = F("P[126] controller check ok: state of payload is : ");
    p126log += payload;
    addLog(LOG_LEVEL_DEBUG,p126log);
      // Serial.println(p126log);
    
    if (ctrlid >= 0) {
      success = MQTTpublish(ctrlid, topic.c_str(), payload.c_str(), Settings.MQTTRetainFlag);
    }
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
}

void get_id(struct DiscoveryStruct *discovery) {
  for (byte x = 0; x < TASKS_MAX; x++) {
    LoadTaskSettings(x);
    int pluginid = Settings.TaskDeviceNumber[x];
      // p126log = F("P[126] searching for pluginid 126: state of task ");
      // p126log += String(x);
      // p126log += F(" is : ");
      // p126log += String(pluginid);
      // addLog(LOG_LEVEL_DEBUG,p126log);
        // Serial.println(p126log);
    if (pluginid == 126) {
      discovery->taskid = x;
      p126log = F("P[126] found correct taskid: state of x is : ");
      p126log += String(x);
      addLog(LOG_LEVEL_DEBUG,p126log);
        // Serial.println(p126log);
      break;        
    } 
      p126log = F("P[126] couldn't fetch taskid: state of x is : ");
      p126log += String(x);
      addLog(LOG_LEVEL_ERROR,p126log);
        // Serial.println(p126log);
  }
}
//#endif
