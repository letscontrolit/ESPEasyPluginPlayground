//-------------------------------------------------------------------------------//
//                                                                               //
//                        HOMEASSISTANT OUTPUT DEVICE                            //
//                                                                               //
//-------------------------------------------------------------------------------//
/* by michael baeck
    plugin generates a Rule-based switch/fan (more soon) that will be automatically added 
    to Homeassistant by using its Discovery function.

    * REQUIRES P126 (Homeassistant Discovery) to be included!

    [!] MAJOR ISSUE:  by espeasy default MQTT_MAX_PACKET_SIZE in pubsubclient.h is set to 384
    [!]               most payloads will be 600-800, please increase for testing this plugin.
    [!]               section in lib\pubsubclient\src\PubSubClient.h:
                      // MQTT_MAX_PACKET_SIZE : Maximum packet size
                      #ifndef MQTT_MAX_PACKET_SIZE
                      #define MQTT_MAX_PACKET_SIZE 384
                      #endif

    
    naming conventions
      entity_id               switch.SYSNAME_TASK  (adjustable)

      last-will-topic         <- controller settings

      sensor-discovery-topic  PREFIX / switch / MAC / MAC_TASKID_VALUEID /config
      sensor-state-topic      <- controller settings

    used vars:
      Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0-3] - value tracking
      Settings.TaskDevicePluginConfig[event->TaskIndex][0]        - component
      Settings.TaskDevicePluginConfig[event->TaskIndex][1]        - action type
      [todo] Settings.TaskDevicePluginConfig[event->TaskIndex][2] - extra device option // add choice for 1 to 4 switches in one


    bugs/todo
      [todo] option to invert gpio,x,0/1
      [todo] switch types
        option0 = gpio-based  = implemented in P126

        option1 = event-based = sub_topic.replace("#","") + "cmd"
                  "stat_t":"homeassistant/switch/807D3A6EA0C5/807D3A6EA0C5_5_0/state",
                  "stat_on": 1,
                  "stat_off": 0,
                  payload on  = event,haswitch[taskid]on
                  payload off = event,haswitch[taskid]off

                  "on haswitch[taskid]on do"
                  "  //place your code here"
                  "  //place your code here"
                  "  publish homeassistant/switch/807D3A6EA0C5/807D3A6EA0C5_5_0/state,1"
                  "endon"

        option2 = cmd-based   = sub_topic.replace("#","") + "cmd"

    build results [20181112]
      none yet

*/  

//#ifdef USES_P128_DISABLED
//#ifdef PLUGIN_BUILD_DEVELOPMENT
//#ifdef PLUGIN_BUILD_TESTING
//#define PLUGIN_119_DEBUG  true


// force using discovery plugin when using this one
#ifndef USES_P126
 #define USES_P126
#endif

#define PLUGIN_128
#define PLUGIN_ID_128           128     
#define PLUGIN_NAME_128         "Generic - Homeassistant Device [DEVELOPMENT]"  
#define PLUGIN_VALUENAME1_128   "output1"
#define PLUGIN_VALUENAME2_128   "output2"
#define PLUGIN_VALUENAME3_128   "output3"
#define PLUGIN_VALUENAME4_128   "output4"
//-------------------------------------------------------------------------------//
// global vars

  #define _P128_VERSION         1
  #define _P128_CMDSTR          5
  //#define _P128_COMPCOUNT       7 // 9
  #define _P128_SWITCH          0
  #define _P128_LIGHT           1
  #define _P128_CLIMATE         2
  #define _P128_FAN             3
  #define _P128_LOCK            4
  #define _P128_CAMERA          5
  #define _P128_COVER           6
  // #define _P128_GARAGE          6
  // #define _P128_WINDOW          7
  // #define _P128_DAMPER          8

  #define _P128_RULE            0
  #define _P128_CMD             1
  #define _P128_GPIO            2
  //#define _P128_DEBUG


  String _P128_component[_P128_COMPCOUNT];
  String _P128_log;

  

//-------------------------------------------------------------------------------//

boolean Plugin_128(byte function, struct EventStruct *event, String& string) {

  boolean success = false;
  String tmpnote;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number = PLUGIN_ID_128;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD; // defines number of values to publish
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 4;      
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].DecimalsOnly = true;
        break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_128);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_128));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_128));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_128));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_128));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      //-------------------------------------------------------------------------------//
      // load discovery setting
        struct DiscoveryStruct discovery;
        String discoveryurl;

        // search for plugin
          if (_P126_get_id(&discovery)) { 
            
            // plugin found, load settings from flash
              LoadCustomTaskSettings(discovery.taskid, (byte*)&discovery.save, sizeof(discovery.save));
              _P126_get_ctrl(&discovery);

              discoveryurl = F("/devices?index=");
              discoveryurl += String(discovery.taskid+1);
              
              // check valid
                if (discovery.save.init != _P126_VERSION) { 
                  
                  // if invalid
                    addFormNote(F("No valid settings found. Please visit Discovery page."));
                    
                    addButton(discoveryurl, "Discovery Settings");
                    
                    break;
                }
          } else {
            // plugin not found
              html_BR();

              addFormNote(F("Discovery settings not found. Please set up Discovery plugin first."));
              break;
          }
        

      //-------------------------------------------------------------------------------//
      // introduction
        html_BR();

        tmpnote  = F("<br>This plugin uses the rules engine to create Home Assistant compatible devices.");
        tmpnote += F("<br>This way you can make any plugin available to HA that can be controlled via EspEasy commands.");
        addFormNote(tmpnote);
      

      //-------------------------------------------------------------------------------//
      // settings
        addFormSubHeader(F("SETTINGS"));
        
        _P128_make_arrays();

        addFormSelector(F("Component"), F("_P128_component"), _P128_COMPCOUNT, _P128_component, NULL, Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
   
        String devtype[2] = {F("Rule based"), F("Command based")};
        addFormSelector(F("Control type"), F("_P128_control"), 1, devtype, NULL, Settings.TaskDevicePluginConfig[event->TaskIndex][1]);


        String payload = _P128_make_config(&discovery, event->TaskIndex, _P128_WEB);

        html_BR();

      //-------------------------------------------------------------------------------//
      // notes
        addFormSubHeader(F("NOTES"));

        tmpnote = F("Once this task is enabled and submitted, you can push the config via discovery plugin.");
        addFormNote(tmpnote);

        html_BR();
        
        if (Settings.TaskDeviceEnabled[event->TaskIndex]) {
          addButton(discoveryurl, "Discovery Plugin");
          html_BR();
        }

        
        tmpnote  = F("Config will have a size of ");
        tmpnote += String(payload.length());
        tmpnote += F(" which ");
        if (payload.length() > MQTT_MAX_PACKET_SIZE) {
          tmpnote += F("exceeds the maximum MQTT packet size of ");
          tmpnote += String(MQTT_MAX_PACKET_SIZE);
          tmpnote += F(". You can publish the payload from another device:<br><br>");
          addFormNote(tmpnote);
          addHtml(payload);
        } else {
          tmpnote += F("fits in the maximum MQTT packet size of ");
          tmpnote += String(MQTT_MAX_PACKET_SIZE);
          addFormNote(tmpnote);
        }

      //-------------------------------------------------------------------------------//


      SaveCustomTaskSettings(discovery.taskid, (byte*)&discovery.save, sizeof(discovery.save));

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      //-------------------------------------------------------------------------------//
      // copy settings from webserver
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("_P128_component"), 0);
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("_P128_control"), 0);

      //-------------------------------------------------------------------------------//

      success = true;
      break;
    }

    case PLUGIN_READ:
    {
     
      break;
    }

    case PLUGIN_WRITE:
    {

      break;
    }

  	case PLUGIN_EXIT:
  	{
  	  //perform cleanup tasks here. For example, free memory
  	  break;
  	}

    case PLUGIN_ONCE_A_SECOND:
    {
      
      break;
    }

    case PLUGIN_TEN_PER_SECOND:
    {
      bool pushvalue = false;

      for (byte x=0;x<3;x++) {
          // #ifdef _P128_DEBUG
          //   Serial.print(F("stored uservar = "));
          //   Serial.println(String(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][x]));
          //   Serial.print(F("current uservar = "));
          //   Serial.println(String(UserVar[event->BaseVarIndex + x]));
          // #endif
        if (UserVar[event->BaseVarIndex + x] != Settings.TaskDevicePluginConfigFloat[event->TaskIndex][x]) {
          Settings.TaskDevicePluginConfigFloat[event->TaskIndex][x] = UserVar[event->BaseVarIndex + x];
          pushvalue = true;
          #ifdef _P128_DEBUG
            Serial.print(F("published and saved NEW uservar = "));
            Serial.println(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][x]);
          #endif
        }
      }
      if (pushvalue) sendData(event);

      success = true;
    }
  } //switch

  return success;
}

//-------------------------------------------------------------------------------//
// plugin functions
void _P128_make_arrays() {

  //-------------------------------------------------------------------------------//
  // declare arrays
    _P128_component[_P128_SWITCH]   = String(F("switch"));
    _P128_component[_P128_LIGHT]    = String(F("light"));
    _P128_component[_P128_CLIMATE]  = String(F("climate"));
    _P128_component[_P128_FAN]      = String(F("fan"));
    _P128_component[_P128_LOCK]     = String(F("lock"));
    _P128_component[_P128_CAMERA]   = String(F("camera"));
    _P128_component[_P128_COVER]    = String(F("cover"));
    // _P128_component[_P128_GARAGE]   = F("cover_garage");
    // _P128_component[_P128_WINDOW]   = F("cover_window");
    // _P128_component[_P128_DAMPER]   = F("cover_damper");
}

String _P128_make_config(struct DiscoveryStruct *discovery, byte taskid, byte mode) {

  String tmpnote;

  #ifdef _P128_DEBUG
  Serial.print(F("make_config called, mode is "));
  Serial.println(String(mode));
  #endif

    #ifdef _P128_DEBUG
    Serial.print(F("make_config: passed taskid is "));
    Serial.println(String(taskid));
    #endif
  
  _P128_make_arrays();

  //-------------------------------------------------------------------------------//
  // entity_id + component
    String component = _P128_component[Settings.TaskDevicePluginConfig[taskid][0]];
    if (mode == _P128_COMPONENT) return component;

    #ifdef _P128_DEBUG
    Serial.print(F("make_config: component is "));
    Serial.println(component);
    #endif
  
    String entity_id;
    if (mode == _P128_WEB) {
      discovery->save.usevalue = false;    // unlike discovery plugin, don't use value name here
      discovery->save.usetask = true;      // ensure to use taskname (instead)
    }
    entity_id = _P126_make_entity(discovery, taskid, byte(0));   // also loads ExtraTaskSettings
    if (mode == _P128_ENTITYID) return entity_id;

    #ifdef _P128_DEBUG
    Serial.print(F("make_config: entity_id is "));
    Serial.println(entity_id);
    #endif
  
  if (mode == _P128_WEB) {
  // preview entity_id
    tmpnote = component;
    tmpnote += F(".");
    tmpnote += entity_id;
    //[-]tmpnote += F("&nbsp<span class='note'>(follows scheme in discovery plugin")

    html_TR_TD();
    addHtml(F("entity_id:"));
    
    html_TD();
    addHtml(tmpnote);
  }

  //-------------------------------------------------------------------------------//
  // discovery topic
    String payload;
    String tmpl;
    String cmdstr[_P128_CMDSTR];
    byte webtask = taskid + 1;

    payload = F("{");

    // unique id
      String unique_id = _P126_make_unique(taskid, byte(0));

    // topic 
      String discovery_topic;
      discovery_topic = _P126_make_topic(discovery->save.prefix, component, unique_id);
      if (mode == _P128_TOPIC) return discovery_topic;

  //-------------------------------------------------------------------------------//
  // common, generic data
    payload += _P126_add_line(F("name"), entity_id);
    payload += _P126_add_line(F("uniq_id"), unique_id);
    payload += _P126_add_line(F("avty_t"), discovery->lwttopic);
    payload += _P126_add_line(F("pl_avail"), discovery->lwtup);
    payload += _P126_add_line(F("pl_not_avail"), discovery->lwtdown);

  // command topic
    String cmd_topic;
    cmd_topic = discovery->cmdtopic;
    cmd_topic += F("cmd");
    payload += _P126_add_line(F("cmd_t"), cmd_topic);

  // first state topic
    String state_topic_base;
    state_topic_base = String(discovery->publish);
    state_topic_base.replace(F("%tskname%"), ExtraTaskSettings.TaskDeviceName);

    String state_topic[4];
    for (byte x = 0; x < 4; x++) {
      state_topic[x] = state_topic_base;
      state_topic[x].replace(F("%valname%"), ExtraTaskSettings.TaskDeviceValueNames[x]);
    }
    
    payload += _P126_add_line(F("stat_t"), state_topic[0]);


  //-------------------------------------------------------------------------------//
  // add component data
    switch (Settings.TaskDevicePluginConfig[taskid][1]) {
    // rule-based control
      case _P128_RULE:

        // check if rules enabled
          if (!Settings.UseRules && mode == _P128_WEB) {
            addFormNote(F("Please enable Rules first in advanced settings."));
            break;
          } else {

            switch (Settings.TaskDevicePluginConfig[taskid][0]) {
              //-------------------------------------------------------------------------------//
              // RULE BASED FAN
                case _P128_FAN:

                  if (mode == _P128_WEB) {
                    addFormNote(F("The fan plugin allows to control on/off and three speeds."));
                    
                    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], "state");
                    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], "speed");

                    ExtraTaskSettings.TaskDeviceValueDecimals[0] = 0;
                    ExtraTaskSettings.TaskDeviceValueDecimals[1] = 0;

                    addFormSubHeader(F("RULES"));

                    tmpnote  = F("<br>Copy these templates to your rules and replace the examples");
                    tmpnote += F(" with the actual commands to trigger described actions.");
                    addFormNote(tmpnote);

                    html_BR();


                    //-------------------------------------------------------------------------------//
                    // turn on rule 
                      tmpnote = F("//commands to be executed when...turning on<br>");
                      //on haX_1 do
                        tmpnote += F("on ha"); 
                        tmpnote += String(webtask); 
                        tmpnote += F("_1 do<br>");
                      //  TaskValueSet,X,1,1
                        tmpnote += F("&nbsp&nbspTaskValueSet,");    
                        tmpnote += String(webtask); 
                        tmpnote += F(",1,1<br>");
                      //  if [task#value]>0
                        tmpnote += F("&nbsp&nbspif [");
                        tmpnote += String(ExtraTaskSettings.TaskDeviceName);
                        tmpnote += F("#");
                        tmpnote += String(ExtraTaskSettings.TaskDeviceValueNames[1]);
                        tmpnote += F("]>0<br>");
                      //    event,haXspd[task#value]
                        tmpnote += F("&nbsp&nbsp&nbsp&nbspevent,ha"); 
                        tmpnote += String(webtask); 
                        tmpnote += F("spd[");
                        tmpnote += String(ExtraTaskSettings.TaskDeviceName);
                        tmpnote += F("#");
                        tmpnote += String(ExtraTaskSettings.TaskDeviceValueNames[1]);
                        tmpnote += F("]<br>");
                      //  else
                        tmpnote += F("&nbsp&nbspelse<br>"); 
                      //    event,haXspd1
                        tmpnote += F("&nbsp&nbsp&nbsp&nbspevent,ha"); 
                        tmpnote += String(webtask); 
                        tmpnote += F("spd1<br>");
                      //  endif
                      //endon
                        tmpnote += F("&nbsp&nbspendif<br>endon<br><br>");
                    //-------------------------------------------------------------------------------//
                      
                    addHtml(tmpnote); // print text/advises
                  }

                  //-------------------------------------------------------------------------------//
                  // on-command config
                    cmdstr[0] = F("event,ha");
                    cmdstr[0] += String(webtask);
                    cmdstr[0] += F("_1");
                    payload += _P126_add_line(F("pl_on"), cmdstr[0]);

                    //[-] payload += _P126_add_line(F("stat_on"), F("1"));

                  if (mode == _P128_WEB) {
                    //-------------------------------------------------------------------------------//
                    // turn off rule  
                      tmpnote  = F("//...turning off<br>");
                      tmpnote += F("on ha"); 
                      tmpnote += String(webtask); 
                      tmpnote += F("_0 do<br>");
                      tmpnote += F("&nbsp&nbspTaskValueSet,"); 
                      tmpnote += String(webtask); 
                      tmpnote += F(",1,0<br>");
                      tmpnote += F("<font color=\"#07D\">&nbsp&nbspgpio,12,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbspgpio,13,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbspgpio,14,1 //<--REPLACE<br></font>");
                      tmpnote += F("endon<br><br>");
                    //-------------------------------------------------------------------------------//

                    addHtml(tmpnote); // print text/advises
                  }

                  //-------------------------------------------------------------------------------//
                  // off-command config
                    cmdstr[1] = F("event,ha");
                    cmdstr[1] += String(webtask);
                    cmdstr[1] += F("_0");
                    payload += _P126_add_line(F("pl_off"), cmdstr[1]);

                    tmpl = F("{{'event,ha");
                    tmpl += String(webtask);
                    tmpl += F("_'+value|string}}");
                    payload += _P126_add_line(F("stat_val_tpl"), tmpl);
                  
                    //[-] payload += _P126_add_line(F("stat_off"), F("0"));

                  if (mode == _P128_WEB) {
                    //-------------------------------------------------------------------------------//
                    // low speed rule
                      tmpnote = F("//...low speed<br>");
                      tmpnote += F("on ha");
                      tmpnote += String(webtask);
                      tmpnote += F("spd1 do<br>");
                      tmpnote += F("&nbsp&nbspTaskValueSet,");
                      tmpnote += String(webtask);
                      tmpnote += F(",2,1<br>");
                      tmpnote += F("&nbsp&nbspif [");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceName);
                      tmpnote += F("#");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceValueNames[0]);
                      tmpnote += F("]=1<br>");

                      tmpnote += F("<font color=\"#07D\">&nbsp&nbsp&nbsp&nbspgpio,12,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,13,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,14,0 //<--REPLACE<br></font>");

                      tmpnote += F("&nbsp&nbspendif<br>endon<br><br>");
                    //-------------------------------------------------------------------------------//

                    addHtml(tmpnote); // print text/advises
                  }

                  //-------------------------------------------------------------------------------//
                  // low speed config
                    cmdstr[2] = F("event,ha");
                    cmdstr[2] += String(webtask);
                    cmdstr[2] += F("spd1");
                    payload += _P126_add_line(F("pl_lo_spd"), cmdstr[2]);         //  event,Hass12spd1

                  if (mode == _P128_WEB) {
                    //-------------------------------------------------------------------------------//
                    // medium speed rule
                      tmpnote  = F("//...medium speed<br>");
                      tmpnote += F("on ha");
                      tmpnote += String(webtask);
                      tmpnote += F("spd2 do<br>");
                      tmpnote += F("&nbsp&nbspTaskValueSet,");
                      tmpnote += String(webtask);
                      tmpnote += F(",2,2<br>");
                      tmpnote += F("&nbsp&nbspif [");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceName);
                      tmpnote += F("#");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceValueNames[0]);
                      tmpnote += F("]=1<br>");

                      tmpnote += F("<font color=\"#07D\">&nbsp&nbsp&nbsp&nbspgpio,13,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,14,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,12,0 //<--REPLACE<br></font>");

                      tmpnote += F("&nbsp&nbspendif<br>endon<br><br>");
                    //-------------------------------------------------------------------------------//

                    addHtml(tmpnote); // print text/advises
                  }

                  //-------------------------------------------------------------------------------//
                  // medium speed config
                    cmdstr[3] = F("event,ha");
                    cmdstr[3] += String(webtask);
                    cmdstr[3] += F("spd2");
                    payload += _P126_add_line(F("pl_med_spd"), cmdstr[3]);        //  event,Hass12spd2

                  if (mode == _P128_WEB) {
                    //-------------------------------------------------------------------------------//
                    // high speed rule
                      tmpnote  = F("//...high speed<br>");
                      tmpnote += F("on ha");
                      tmpnote += String(webtask);
                      tmpnote += F("spd3 do<br>");
                      tmpnote += F("&nbsp&nbspTaskValueSet,");
                      tmpnote += String(webtask);
                      tmpnote += F(",2,3<br>");
                      tmpnote += F("&nbsp&nbspif [");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceName);
                      tmpnote += F("#");
                      tmpnote += String(ExtraTaskSettings.TaskDeviceValueNames[0]);
                      tmpnote += F("]=1<br>");

                      tmpnote += F("<font color=\"#07D\">&nbsp&nbsp&nbsp&nbspgpio,12,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,14,1 //<--REPLACE<br>");
                      tmpnote += F("&nbsp&nbsp&nbsp&nbspgpio,13,0 //<--REPLACE<br></font>");

                      tmpnote += F("&nbsp&nbspendif<br>endon<br><br>");
                    //-------------------------------------------------------------------------------//

                    addHtml(tmpnote); // print text/advises
                  }

                  //-------------------------------------------------------------------------------//
                  // high speed config
                    cmdstr[4] = F("event,ha");
                    cmdstr[4] += String(webtask);
                    cmdstr[4] += F("spd3");
                    payload += _P126_add_line(F("pl_hi_spd"), cmdstr[4]);         //  event,Hass12spd3

                  //-------------------------------------------------------------------------------//
                  // common speed config
                    payload += _P126_add_line(F("spd_cmd_t"), cmd_topic);         //  esp22/cmd

                    payload += _P126_add_line(F("spd_stat_t"), state_topic[1]);   //  esp22/hafan/output2

                    tmpl  = F("{{'event,ha");
                    tmpl += String(webtask);
                    tmpl += F("spd'+value|string}}");
                    payload += _P126_add_line(F("spd_val_tpl"), tmpl);            //  {{'event,Hass12spd'+value|string}}

                    payload += F("\"spds\":[\"low\",\"medium\",\"high\"],");  // ["low","medium","high"]
                  //-------------------------------------------------------------------------------//

                  break;

              //-------------------------------------------------------------------------------//
              // RULE BASED SWITCH
                case _P128_SWITCH:

                    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], "state");
                   
                    ExtraTaskSettings.TaskDeviceValueDecimals[0] = 0;

                  //-------------------------------------------------------------------------------//
                  // on-command config
                    cmdstr[0] = F("event,ha");
                    cmdstr[0] += String(webtask);
                    cmdstr[0] += F("on");
                    payload += _P126_add_line(F("pl_on"), cmdstr[0]);

                    payload += _P126_add_line(F("state_on"), F("1"));

                  //-------------------------------------------------------------------------------//
                  // notes + turn on rule
                    if (mode == _P128_WEB) {

                      addFormNote(F("The switch plugin creates a simple on/off device."));
                      

                      addFormSubHeader(F("RULES"));

                      tmpnote  = F("<br>Copy these templates to your rules and replace the examples");
                      tmpnote += F(" with the actual commands to trigger described actions.");
                      addFormNote(tmpnote);

                      html_BR();

                      //-------------------------------------------------------------------------------//
                      // turn on rule
                        tmpnote = F("//commands to be executed when...turning on<br>");
                        tmpnote += F("on ha"); 
                        tmpnote += String(webtask); 
                        tmpnote += F("on do<br>");
                        tmpnote += F("&nbsp&nbspTaskValueSet,"); 
                        tmpnote += String(webtask); 
                        tmpnote += F(",1,1<br>");
                        tmpnote += F("<font color=\"#07D\">&nbsp&nbsprc,on=1010110101 //<--REPLACE</font><br>");
                        tmpnote += F("endon<br><br>");
                          
                        addHtml(tmpnote); // print text/advises
                      //-------------------------------------------------------------------------------//
                    }

                  //-------------------------------------------------------------------------------//
                  // off-command config
                    cmdstr[1] = F("event,ha");
                    cmdstr[1] += String(webtask);
                    cmdstr[1] += F("off");
                    payload += _P126_add_line(F("pl_off"), cmdstr[1]);

                    payload += _P126_add_line(F("state_off"), F("0"));
                       
                  //-------------------------------------------------------------------------------//
                  // turn off rule
                    if (mode == _P128_WEB) {
                        tmpnote  = F("//...turning off<br>");
                        tmpnote += F("on ha"); 
                        tmpnote += String(webtask); 
                        tmpnote += F("off do<br>");
                        tmpnote += F("&nbsp&nbspTaskValueSet,"); 
                        tmpnote += String(webtask); 
                        tmpnote += F(",1,0<br>");
                        tmpnote += F("<font color=\"#07D\">&nbsp&nbsprc,off=1010110101 //<--REPLACE</font><br>");
                        tmpnote += F("endon<br><br>");
                          
                        addHtml(tmpnote); // print text/advises
                    }
                  //-------------------------------------------------------------------------------//

                  break;
                
                case _P128_LIGHT:
                case _P128_CLIMATE:
                case _P128_COVER:
                case _P128_CAMERA:
                  break;
                  //-------------------------------------------------------------------------------//
            }
            //[-]if (!unattanded) addHtml(tmpnote); // print text/advises
          }
          break; //case RULE

        //-------------------------------------------------------------------------------//
    // command-based control (not in use)
      case _P128_CMD:
        //[.]   switch (Settings.TaskDevicePluginConfig[taskid][0]) {
        //[.]     case _P128_SWITCH:
        //[.]       cmdstr[0] = String(commands[0]);
        //[.]       cmdstr[1] = String(commands[1]);
        //[.]       addFormTextBox(String(F("command on")), String(F("Plugin_126_cmdon")), cmdstr[0], 40);
        //[.]       addFormTextBox(String(F("command off")), String(F("Plugin_126_cmdoff")), cmdstr[1], 40);
        //[.]       break;

        //[.]     case _P128_LIGHT:
        //[.]     case _P128_CLIMATE:
        //[.]     case _P128_FAN:
        //[.]     case _P128_COVER:
        //[.]     case _P128_CAMERA:
        //[.]       break;
        //[.]   }

        break; //case CMD
    }

    payload += _P126_add_device(false);  // closes json payload
    #ifdef _P126_DEVICE
      payload.replace(ARDUINO_BOARD, discovery->save.device);
    #endif

  //-------------------------------------------------------------------------------//
  return payload;
}

//#endif // USES_P128





/*  home-assistant parameters

  GENERIC
    'name':                'name',
    'uniq_id':             'unique_id',
    't':                   'topic',
    'ret':                 'retain',

    'avty_t':              'availability_topic',
    'pl_avail':            'payload_available',
    'pl_not_avail':        'payload_not_available',

    'json_attr':           'json_attributes',

    'cmd_t':               'command_topic',
    'pl_off':              'payload_off',
    'pl_on':               'payload_on',

    'stat_t':              'state_topic',
    'stat_off':            'state_off',
    'stat_on':             'state_on',
    'stat_val_tpl':        'state_value_template',

    'val_tpl':             'value_template',
    'ic':                  'icon',

  ALARM
    'pl_arm_away':         'payload_arm_away',
    'pl_arm_home':         'payload_arm_home',
    'pl_disarm':           'payload_disarm',

  BINARY SENSOR
    'dev_cla':             'device_class',
    'exp_aft':             'expire_after',
    'pl_off':              'payload_off',
    'pl_on':               'payload_on',

  SENSOR
    'dev_cla':             'device_class',
    'exp_aft':             'expire_after',
    'frc_upd':             'force_update',
    'unit_of_meas':        'unit_of_measurement',

  SWITCH
    'init':                'initial',
    'opt':                 'optimistic',

  CAMERA

  LOCK
    'pl_unlk':             'payload_unlock',
    'pl_lock':             'payload_lock',

  COVER
    'set_pos_tpl':         'set_position_template',
    'set_pos_t':           'set_position_topic',
    'stat_clsd':           'state_closed',
    'stat_open':           'state_open',
    'pl_cls':              'payload_close',
    'pl_open':             'payload_open',
    'pl_stop':             'payload_stop',
    'tilt_clsd_val':       'tilt_closed_value',
    'tilt_cmd_t':          'tilt_command_topic',
    'tilt_inv_stat':       'tilt_invert_state',
    'tilt_max':            'tilt_max',
    'tilt_min':            'tilt_min',
    'tilt_opnd_val':       'tilt_opened_value',
    'tilt_status_opt':     'tilt_status_optimistic',
    'tilt_status_t':       'tilt_status_topic',

  CLIMATE
    'max_temp':            'max_temp',
    'min_temp':            'min_temp',
    'curr_temp_t':         'current_temperature_topic',
    'mode_cmd_t':          'mode_command_topic',
    'mode_stat_tpl':       'mode_state_template',
    'mode_stat_t':         'mode_state_topic',
    'temp_cmd_t':          'temperature_command_topic',
    'temp_stat_tpl':       'temperature_state_template',
    'temp_stat_t':         'temperature_state_topic',
    'hold_cmd_t':          'hold_command_topic',
    'hold_stat_tpl':       'hold_state_template',
    'hold_stat_t':         'hold_state_topic',
    'away_mode_cmd_t':     'away_mode_command_topic',
    'away_mode_stat_tpl':  'away_mode_state_template',
    'away_mode_stat_t':    'away_mode_state_topic',
    'aux_cmd_t':           'aux_command_topic',
    'aux_stat_tpl':        'aux_state_template',
    'aux_stat_t':          'aux_state_topic',
    'swing_mode_cmd_t':    'swing_mode_command_topic',
    'swing_mode_stat_tpl': 'swing_mode_state_template',
    'swing_mode_stat_t':   'swing_mode_state_topic',
    'send_if_off':         'send_if_off',
    'pow_cmd_t':           'power_command_topic',

  FAN
    'spd_cmd_t':           'speed_command_topic',
    'spd_stat_t':          'speed_state_topic',
    'spd_val_tpl':         'speed_value_template',
    'spds':                'speeds',
    'osc_cmd_t':           'oscillation_command_topic',
    'osc_stat_t':          'oscillation_state_topic',
    'osc_val_tpl':         'oscillation_value_template',
    'pl_osc_off':          'payload_oscillation_off',
    'pl_osc_on':           'payload_oscillation_on',
    'pl_hi_spd':           'payload_high_speed',
    'pl_lo_spd':           'payload_low_speed',
    'pl_med_spd':          'payload_medium_speed',
    'fan_mode_cmd_t':      'fan_mode_command_topic',
    'fan_mode_stat_tpl':   'fan_mode_state_template',
    'fan_mode_stat_t':     'fan_mode_state_topic',

  LIGHT
    'rgb_cmd_tpl':         'rgb_command_template',
    'rgb_cmd_t':           'rgb_command_topic',
    'rgb_stat_t':          'rgb_state_topic',
    'rgb_val_tpl':         'rgb_value_template',
    'whit_val_cmd_t':      'white_value_command_topic',
    'whit_val_stat_t':     'white_value_state_topic',
    'whit_val_tpl':        'white_value_template',
    'xy_cmd_t':            'xy_command_topic',
    'xy_stat_t':           'xy_state_topic',
    'xy_val_tpl':          'xy_value_template',
    'bri_cmd_t':           'brightness_command_topic',
    'bri_scl':             'brightness_scale',
    'bri_stat_t':          'brightness_state_topic',
    'bri_val_tpl':         'brightness_value_template',
    'clr_temp_cmd_t':      'color_temp_command_topic',
    'clr_temp_stat_t':     'color_temp_state_topic',
    'clr_temp_val_tpl':    'color_temp_value_template',
    'fx_cmd_t':            'effect_command_topic',
    'fx_list':             'effect_list',
    'fx_stat_t':           'effect_state_topic',
    'fx_val_tpl':          'effect_value_template',
    'on_cmd_type':         'on_command_type',
*/

/* payload
{
  "name":"michbaeck",
  "uniq_id":"382B7803EC63_1_0",
  "avty_t":"esp07/state",
  "pl_avail":"online",
  "pl_not_avail":"offline",
  "cmd_t":"esp07/cmd",
  "stat_t":"esp07/michbaeck/state",
  "pl_on":"event,ha2_1",
  "pl_off":"event,ha2_0",
  "stat_val_tpl":"{{'event,ha2_'+value|string}}",
  "pl_lo_spd":"event,ha2spd1",
  "pl_med_spd":"event,ha2spd2",
  "pl_hi_spd":"event,ha2spd3",
  "spd_cmd_t":"esp07/cmd",
  "spd_stat_t":"esp07/michbaeck/speed",
  "spd_val_tpl":"{{'event,ha2spd'+value|string}}",
  "spds":["low","medium","high"],
  "device":
  {
    "sw_version":"EspEasy - Mega (Nov 18 2018)",
    "manufacturer":"letscontrolit.com",
    "model":"D1mini Ventilator",
    "connections":[["mac","38:2B:78:03:EC:63"]],
    "name":"esp07",
    "identifiers":["257123","382B7803EC63"]
  }
}

*/