//############################# Plugin 165: Serial MCU controlled switch v1.5 ###########################
//
//  Designed for TUYA/YEWELINK Wifi Touch Light switch with ESP8266 + PIC16F1829 MCU,
//  the similar Sonoff Dual MCU controlled Wifi relay and LCTECH WIFI RELAY is also supported.
//  Based on P020 Ser2Net and P001 Switch.
//
//  Dummy device (Switch) can be used to control relays, one device per relay and/or rules Publish command 
//  for example.
//  Support thread: https://www.letscontrolit.com/forum/viewtopic.php?f=6&t=3245
//
//#######################################################################################################

#define PLUGIN_165
#define PLUGIN_ID_165         165
#define PLUGIN_NAME_165       "Serial MCU controlled switch"
#define PLUGIN_VALUENAME1_165 "Relay0"
#define PLUGIN_VALUENAME2_165 "Relay1"
#define PLUGIN_VALUENAME3_165 "Relay2"

#define BUFFER_SIZE   128 // at least 3x33 byte serial buffer needed for Tuya

#define SER_SWITCH_YEWE 1
#define SER_SWITCH_SONOFFDUAL 2
#define SER_SWITCH_LCTECH 3

static byte Plugin_165_switchstate[3];
static byte Plugin_165_ostate[3];
byte Plugin_165_commandstate = 0; // 0:no,1:inprogress,2:finished
byte Plugin_165_numrelay = 1;
byte Plugin_165_ownindex;
byte Plugin_165_globalpar0;
byte Plugin_165_globalpar1;
boolean Plugin_165_init = false;

boolean Plugin_165(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_165;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }
    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_165);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_165));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_165));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_165));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[3];
        options[0] = F("Yewelink/TUYA");
        options[1] = F("Sonoff Dual");
        options[2] = F("LC TECH");
        int optionValues[3] = { SER_SWITCH_YEWE, SER_SWITCH_SONOFFDUAL, SER_SWITCH_LCTECH };
        addFormSelector(string, F("Switch Type"), F("plugin_165_type"), 3, options, optionValues, choice);

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_YEWE)
        {
          choice = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          String buttonOptions[3];
          buttonOptions[0] = F("1");
          buttonOptions[1] = F("2");
          buttonOptions[2] = F("3");
          addFormSelector(string, F("Number of relays"), F("plugin_165_button"), 3, buttonOptions, NULL, choice);
        }

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_SONOFFDUAL)
        {
          choice = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          String modeoptions[2];
          modeoptions[0] = F("Off");
          modeoptions[1] = F("On");
          int modeoptionValues[2] = { 0, 1 };
          addFormSelector(string, F("Exclude/Blinds mode"), F("plugin_165_mode"), 2, modeoptions, modeoptionValues, choice);
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_165_type"));
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_YEWE)
        {
          Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_165_button"));
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_SONOFFDUAL)
        {
          Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_165_mode"));
        }

        Plugin_165_globalpar0 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_165_globalpar1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        Plugin_165_ownindex = event->TaskIndex;
        Serial.setDebugOutput(false);
        Serial.setRxBufferSize(BUFFER_SIZE); // Arduino core for ESP8266 WiFi chip 2.4.0
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_YEWE)
        {
          Plugin_165_numrelay = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          Serial.begin(9600, SERIAL_8N1);
          delay(1);
          getmcustate(); // request status on startup
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_SONOFFDUAL)
        {
          Plugin_165_numrelay = 3; // 3rd button is the "wifi" button
          Serial.begin(19230, SERIAL_8N1);
          // on start we do not know the state of the relays...
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_LCTECH)
        {
          Plugin_165_numrelay = 1;
          Serial.begin(9600, SERIAL_8N1);
          // on start we do not know the state of the relays...
        }

        Plugin_165_globalpar0 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_165_globalpar1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

        success = true;
        Plugin_165_init = true;
        break;
      }


    case PLUGIN_SERIAL_IN:
      {
        int bytes_read = 0;
        byte serial_buf[BUFFER_SIZE];
        String log;

        if (Plugin_165_init)
        {
          while (Serial.available() > 0) {
            yield();
            if (bytes_read < BUFFER_SIZE) {
              serial_buf[bytes_read] = Serial.read();

              if (bytes_read == 0) { // packet start

                Plugin_165_commandstate = 0;
                switch (Settings.TaskDevicePluginConfig[event->TaskIndex][0])
                {
                  case SER_SWITCH_YEWE: //decode first byte of package
                    {
                      if (serial_buf[bytes_read] == 0x55) {
                        Plugin_165_commandstate = 1;
                      }
                      break;
                    }
                  case SER_SWITCH_SONOFFDUAL: //decode first byte of package
                    {
                      if (serial_buf[bytes_read] == 0xA0) {
                        Plugin_165_commandstate = 1;
                      }
                      break;
                    }
                }
              } else {

                if (Plugin_165_commandstate == 1) {

                  if (bytes_read == 1) { // check if packet is valid
                    switch (Settings.TaskDevicePluginConfig[event->TaskIndex][0])
                    {
                      case SER_SWITCH_YEWE:
                        {
                          if (serial_buf[bytes_read] != 0xAA) {
                            Plugin_165_commandstate = 0;
                            bytes_read = 0;
                          }
                          break;
                        }
                      case SER_SWITCH_SONOFFDUAL:
                        {
                          if ((serial_buf[bytes_read] != 0x04) && (serial_buf[bytes_read] != 0x00)) {
                            Plugin_165_commandstate = 0;
                            bytes_read = 0;
                          }
                          break;
                        }
                    }
                  }

                  if ( (bytes_read == 2) && (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_SONOFFDUAL)) { // decode Sonoff Dual status changes
                    Plugin_165_ostate[0] = Plugin_165_switchstate[0]; Plugin_165_ostate[1] = Plugin_165_switchstate[1]; Plugin_165_ostate[2] = Plugin_165_switchstate[2];
                    Plugin_165_switchstate[0] = 0; Plugin_165_switchstate[1] = 0; Plugin_165_switchstate[2] = 0;
                    if ((serial_buf[bytes_read] & 1) == 1) {
                      Plugin_165_switchstate[0] = 1;
                    }
                    if ((serial_buf[bytes_read] & 2) == 2) {
                      Plugin_165_switchstate[1] = 1;
                    }
                    if ((serial_buf[bytes_read] & 4) == 4) {
                      Plugin_165_switchstate[2] = 1;
                    }
                    Plugin_165_commandstate = 2; bytes_read = 0;

                    if (Settings.TaskDevicePluginConfig[event->TaskIndex][1] == 1)
                    { // exclusive on mode
                      if ((Plugin_165_ostate[0] == 1) && (Plugin_165_switchstate[1] == 1)) {
                        sendmcucommand(0, 0, Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
                        Plugin_165_switchstate[0] = 0;
                      }
                      if ((Plugin_165_ostate[1] == 1) && (Plugin_165_switchstate[0] == 1)) {
                        sendmcucommand(1, 0, Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
                        Plugin_165_switchstate[1] = 0;
                      }
                    }

                    log = F("SW   : State ");
                    if (Plugin_165_ostate[0] != Plugin_165_switchstate[0]) {
                      UserVar[event->BaseVarIndex] = Plugin_165_switchstate[0];
                      log += F(" r0:");
                      log += Plugin_165_switchstate[0];
                    }
                    if (Plugin_165_ostate[1] != Plugin_165_switchstate[1]) {
                      UserVar[event->BaseVarIndex + 1] = Plugin_165_switchstate[1];
                      log += F(" r1:");
                      log += Plugin_165_switchstate[1];
                    }
                    if (Plugin_165_ostate[2] != Plugin_165_switchstate[2]) {
                      UserVar[event->BaseVarIndex + 2] = Plugin_165_switchstate[2];
                      log += F(" b2:");
                      log += Plugin_165_switchstate[1];
                    }
                    addLog(LOG_LEVEL_INFO, log);
                    if ( (Plugin_165_ostate[0] != Plugin_165_switchstate[0]) || (Plugin_165_ostate[1] != Plugin_165_switchstate[1]) || (Plugin_165_ostate[2] != Plugin_165_switchstate[2]) ) {
                      event->sensorType = SENSOR_TYPE_SWITCH;
                      sendData(event);
                    }
                  }
                  if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_YEWE) { // decode Tuya/Yewelink status report package
                    if ((bytes_read == 3) && (serial_buf[bytes_read] != 7))
                    {
                      Plugin_165_commandstate = 0;  // command code 7 means status reporting, we do not need other packets
                      bytes_read = 0;
                    }
                    if (bytes_read == 10) {
                      byte btnnum = (serial_buf[6] - 1);
                      Plugin_165_ostate[btnnum] = Plugin_165_switchstate[btnnum];
                      Plugin_165_switchstate[btnnum] = serial_buf[10];
                      Plugin_165_commandstate = 2; bytes_read = 0;

                      if (Plugin_165_ostate[btnnum] != Plugin_165_switchstate[btnnum]) {
                        log = F("SW   : State");
                        switch (btnnum) {
                          case 0: {
                              if (Plugin_165_numrelay > 0) {
                                UserVar[event->BaseVarIndex + btnnum] = Plugin_165_switchstate[btnnum];
                                log += F(" r0:");
                                log += Plugin_165_switchstate[btnnum];
                              }

                            }
                          case 1: {
                              if (Plugin_165_numrelay > 1) {
                                UserVar[event->BaseVarIndex + btnnum] = Plugin_165_switchstate[btnnum];
                                log += F(" r1:");
                                log += Plugin_165_switchstate[btnnum];
                              }

                            }
                          case 2: {
                              if (Plugin_165_numrelay > 2) {
                                UserVar[event->BaseVarIndex + btnnum] = Plugin_165_switchstate[btnnum];
                                log += F(" r2:");
                                log += Plugin_165_switchstate[btnnum];
                              }

                            }
                        }
                        event->sensorType = SENSOR_TYPE_SWITCH;
                        addLog(LOG_LEVEL_INFO, log);
                        sendData(event);
                      }
                    } //10th byte end (Tuya package)


                  } // yewe decode end
                } // Plugin_165_commandstate 1 end
              } // end of status decoding

              if (Plugin_165_commandstate == 1) {
                bytes_read++;
              }
            } else
              Serial.read(); // if buffer full, dump incoming
          }
        } // plugin initialized end
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (Plugin_165_init)
        {
          if ((Settings.TaskDevicePluginConfig[event->TaskIndex][0] == SER_SWITCH_YEWE) && (Plugin_165_commandstate != 1))
          { // check Tuya state if anybody ask for it
            String log = F("SW   : ReadState");
            addLog(LOG_LEVEL_INFO, log);
            getmcustate();
          }
          success = true;
        }
        break;
      }

    case PLUGIN_WRITE:
      {
        String log = "";
        String command = parseString(string, 1);
        byte rnum = 0;
        byte rcmd = 0;
        byte par3 = 0;

        if (Plugin_165_init)
        {
          if ( command == F("relay") ) // deal with relay change command
          {
            success = true;

            if ((event->Par1 >= 0) && (event->Par1 < Plugin_165_numrelay)) {
              rnum = event->Par1;
            }
            if ((event->Par2 == 0) || (event->Par2 == 1)) {
              rcmd = event->Par2;
            }

            LoadTaskSettings(Plugin_165_ownindex); // get our own task values please
            event->TaskIndex = Plugin_165_ownindex;
            byte varIndex = Plugin_165_ownindex * VARS_PER_TASK;
            event->BaseVarIndex = varIndex;

            if ( Plugin_165_globalpar0 < SER_SWITCH_LCTECH) {
              par3 = Plugin_165_globalpar1;
            }
            sendmcucommand(rnum, rcmd, Plugin_165_globalpar0, par3);
            if ( Plugin_165_globalpar0 > SER_SWITCH_YEWE) { // report state only if not Yewe
              if (UserVar[(varIndex + rnum)] != Plugin_165_switchstate[rnum]) { // report only if state is really changed
                UserVar[(varIndex + rnum)] = Plugin_165_switchstate[rnum];
                if (( par3 == 1) && (rcmd == 1) )
                { // exclusive on mode for Dual
                  if (rnum == 0) {
                    UserVar[(varIndex + 1)] = 0;
                  }
                  if (rnum == 1) {
                    UserVar[varIndex] = 0;
                  }
                }
                event->sensorType = SENSOR_TYPE_SWITCH;
                sendData(event);
              }
            }
            String log = F("SW   : SetSwitch r");
            log += event->Par1;
            log += F(":");
            log += rcmd;
            addLog(LOG_LEVEL_INFO, log);

          }
        }
        break;
      }

  }
  return success;
}

void getmcustate() { 
  Serial.write(0x55); // Tuya header 55AA
  Serial.write(0xAA);
  Serial.write(0x00); // version 00
  Serial.write(0x08); // Tuya command 08 - request status
  Serial.write(0x00);
  Serial.write(0x00);
  Serial.write(0x07);
  Serial.flush();
}

void sendmcucommand(byte btnnum, byte state, byte swtype, byte btnum_mode) // btnnum=0,1,2, state=0/1
{
  byte sstate;

  switch (swtype)
  {
    case SER_SWITCH_YEWE:
      {
        Serial.write(0x55); // Tuya header 55AA
        Serial.write(0xAA);
        Serial.write(0x00); // version 00
        Serial.write(0x06); // Tuya command 06 - send order
        Serial.write(0x00);
        Serial.write(0x05); // following data length 0x05
        Serial.write( (btnnum + 1) ); // relay number 1,2,3
        Serial.write(0x01); // ?
        Serial.write(0x00); // ?
        Serial.write(0x01); // ?
        Serial.write( state ); // status
        Serial.write((13 + btnnum + state)); // checksum:sum of all bytes in packet mod 256
        Serial.flush();
        break;
      }
    case SER_SWITCH_SONOFFDUAL:
      {
        Plugin_165_switchstate[btnnum] = state;
        if (( btnum_mode == 1) && (state == 1) )
        { // exclusive on mode
          if (btnnum == 0) {
            Plugin_165_switchstate[1] = 0;
          }
          if (btnnum == 1) {
            Plugin_165_switchstate[0] = 0;
          }
        }
        sstate = Plugin_165_switchstate[0] + (Plugin_165_switchstate[1] << 1) + (Plugin_165_switchstate[2] << 2);
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write( sstate );
        Serial.write(0xA1);
        Serial.flush();
        delay(1);
        break;
      }
    case SER_SWITCH_LCTECH:
      {
        if (btnnum == 0) {       // only one relay possible as i know
          Plugin_165_switchstate[btnnum] = state;
          for (byte x = 0; x < 2; x++) // try twice to be sure
          {
            Serial.write(0xA0);
            Serial.write(0x01);
            if (state == 0) { // off
              Serial.write(0x00);
              Serial.write(0xA1);
            } else {         // on
              Serial.write(0x01);
              Serial.write(0xA2);
            }
            Serial.flush();
            delay(1);
          }
          break;
        }
      }
  }
}
