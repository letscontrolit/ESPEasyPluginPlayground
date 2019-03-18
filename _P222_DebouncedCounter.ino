#ifdef USES_P222
//#######################################################################################################
//#################################### Plugin 222: Pulse  ###############################################
//#######################################################################################################

#define PLUGIN_222
#define PLUGIN_ID_222         222
#define PLUGIN_NAME_222       "Generic - Debounced Counter"
#define PLUGIN_VALUENAME1_222 "Count"
#define PLUGIN_VALUENAME2_222 "Total"
#define PLUGIN_VALUENAME3_222 "Time"

void Plugin_222_pulse_interrupt1() ICACHE_RAM_ATTR;
void Plugin_222_pulse_interrupt2() ICACHE_RAM_ATTR;
void Plugin_222_pulse_interrupt3() ICACHE_RAM_ATTR;
void Plugin_222_pulse_interrupt4() ICACHE_RAM_ATTR;
//this takes 20 bytes of IRAM per handler
// void Plugin_222_pulse_interrupt5() ICACHE_RAM_ATTR;
// void Plugin_222_pulse_interrupt6() ICACHE_RAM_ATTR;
// void Plugin_222_pulse_interrupt7() ICACHE_RAM_ATTR;
// void Plugin_222_pulse_interrupt8() ICACHE_RAM_ATTR;

unsigned long Plugin_222_pulseCounter[TASKS_MAX];
unsigned long Plugin_222_pulseTotalCounter[TASKS_MAX];
unsigned long Plugin_222_pulseTime[TASKS_MAX];
unsigned long Plugin_222_pulseTimePrevious[TASKS_MAX];
byte Plugin_222_pulsePin[TASKS_MAX];
byte Plugin_222_pulseMode[TASKS_MAX];
byte Plugin_222_ct0[TASKS_MAX];
byte Plugin_222_ct1[TASKS_MAX];
byte Plugin_222_prescaler[TASKS_MAX];
byte Plugin_222_prescaler_cnt[TASKS_MAX];
byte Plugin_222_signal_state[TASKS_MAX];
byte Plugin_222_signal_falling[TASKS_MAX];
byte Plugin_222_signal_rising[TASKS_MAX];

boolean Plugin_222(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_TEN_PER_SECOND:
      {
        if ( (Plugin_222_pulseMode[event->TaskIndex] == FALLING
          || Plugin_222_pulseMode[event->TaskIndex] == CHANGE )
          && Plugin_222_signal_falling[event->TaskIndex])
        {
            Plugin_222_signal_falling[event->TaskIndex]=0;
            Plugin_222_pulseCounter[event->TaskIndex]++;
            Plugin_222_pulseTotalCounter[event->TaskIndex]++;
            Plugin_222_pulseTime[event->TaskIndex] = timePassedSince(Plugin_222_pulseTimePrevious[event->TaskIndex]);;
            Plugin_222_pulseTimePrevious[event->TaskIndex]=millis();
        }
        if ( (Plugin_222_pulseMode[event->TaskIndex] ==  RISING
          || Plugin_222_pulseMode[event->TaskIndex] == CHANGE )
           && Plugin_222_signal_rising[event->TaskIndex])
        {
            Plugin_222_signal_rising[event->TaskIndex]=0;
            Plugin_222_pulseCounter[event->TaskIndex]++;
            Plugin_222_pulseTotalCounter[event->TaskIndex]++;
            Plugin_222_pulseTime[event->TaskIndex] = timePassedSince(Plugin_222_pulseTimePrevious[event->TaskIndex]);;
            Plugin_222_pulseTimePrevious[event->TaskIndex]=millis();
        }
        break;
      }
      case PLUGIN_FIFTY_PER_SECOND:
      {
        if ( Plugin_222_prescaler_cnt[event->TaskIndex]++ % Plugin_222_prescaler[event->TaskIndex] == 0 ) // prescale value reacheed?
        {
          // Credit: https://www.mikrocontroller.net/topic/tasten-entprellen-bulletproof
          byte i = Plugin_222_signal_state[event->TaskIndex] ^ ~digitalRead(Plugin_222_pulsePin[event->TaskIndex]);	// key changed ?
          Plugin_222_ct0[event->TaskIndex] = ~( Plugin_222_ct0[event->TaskIndex] & i );		// reset or count ct0
          Plugin_222_ct1[event->TaskIndex] = Plugin_222_ct0[event->TaskIndex] ^ (Plugin_222_ct1[event->TaskIndex] & i);		// reset or count ct1
          i &= Plugin_222_ct0[event->TaskIndex] & Plugin_222_ct1[event->TaskIndex];		// count until roll over
          Plugin_222_signal_state[event->TaskIndex] ^= i;		// then toggle debounced state
          Plugin_222_signal_falling[event->TaskIndex] |= Plugin_222_signal_state[event->TaskIndex] & i;	// 0->1: key pressing detect
          Plugin_222_signal_rising[event->TaskIndex] |= ~Plugin_222_signal_state[event->TaskIndex] & i;        // 1->0: key release detect
        }
        break;
      }
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_222;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_222);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_222));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_222));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_222));
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = formatGpioName_input(F("Pulse"));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        String debounceTime[6];
        debounceTime[0] = F("80ms");
        debounceTime[1] = F("160ms");
        debounceTime[2] = F("240ms");
        debounceTime[3] = F("320ms");
        debounceTime[4] = F("400ms");
        debounceTime[5] = F("800ms");

        int debouncePrescale[] = { 1, 2, 3, 4, 5, 10 };
        addFormSelector(F("Debounce Time"), F("p222"), 6, debounceTime, debouncePrescale, PCONFIG(0) );

        byte choice = PCONFIG(1);
        byte choice2 = PCONFIG(2);
        String options[3] = { F("Delta"),  F("Total"), F("Delta/Total") };
        addFormSelector(F("Counter Type"), F("p222_countertype"), 3, options, NULL, choice );

        if (choice !=0)
          addHtml(F("<span style=\"color:red\">Total count is not persistent!</span>"));

        String modeRaise[3];
        modeRaise[0] = F("CHANGE");
        modeRaise[1] = F("RISING");
        modeRaise[2] = F("FALLING");

        int modeValues[3];
        modeValues[0] = CHANGE;
        modeValues[1] = RISING;
        modeValues[2] = FALLING;

        addFormSelector(F("Mode Type"), F("p222_raisetype"), 3, modeRaise, modeValues, choice2);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p222"));
        PCONFIG(1) = getFormItemInt(F("p222_countertype"));
        PCONFIG(2) = getFormItemInt(F("p222_raisetype"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SHOW_VALUES:
      {

        string += F("<div class=\"div_l\">");
        string += ExtraTaskSettings.TaskDeviceValueNames[0];
        string += F(":</div><div class=\"div_r\">");
        string += Plugin_222_pulseCounter[event->TaskIndex];
        string += F("</div><div class=\"div_br\"></div><div class=\"div_l\">");
        string += ExtraTaskSettings.TaskDeviceValueNames[1];
        string += F(":</div><div class=\"div_r\">");
        string += Plugin_222_pulseTotalCounter[event->TaskIndex];
        string += F("</div><div class=\"div_br\"></div><div class=\"div_l\">");
        string += ExtraTaskSettings.TaskDeviceValueNames[2];
        string += F(":</div><div class=\"div_r\">");
        string += Plugin_222_pulseTime[event->TaskIndex];
        string += F("</div>");
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        String log = F("INIT : Pulse ");
        log += CONFIG_PIN1;
        log += " as ";
        log +=  PCONFIG(2);
        log += " with debouce ";
        log += PCONFIG(0);
        //log += "*80ms";
        addLog(LOG_LEVEL_INFO,log);
        pinMode(CONFIG_PIN1, INPUT_PULLUP);
        Plugin_222_ct0[event->TaskIndex]=0; // reset debounce counter
        Plugin_222_ct1[event->TaskIndex]=0; // dito
        Plugin_222_signal_rising[event->TaskIndex]=0; // reset state
        Plugin_222_signal_falling[event->TaskIndex]=0; // reset state
        Plugin_222_prescaler[event->TaskIndex]=PCONFIG(0); // prescaler preset
        Plugin_222_prescaler_cnt[event->TaskIndex]=0; // prescaler counter
        Plugin_222_pulsePin[event->TaskIndex]=CONFIG_PIN1; // remember pin
        Plugin_222_pulseMode[event->TaskIndex]=PCONFIG(2); // remember edge
        Plugin_222_pulseTimePrevious[event->TaskIndex]=millis(); // remmember now
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        UserVar[event->BaseVarIndex] = Plugin_222_pulseCounter[event->TaskIndex];
        UserVar[event->BaseVarIndex+1] = Plugin_222_pulseTotalCounter[event->TaskIndex];

        switch (PCONFIG(1)) //Counter type
        {
          case 0:
          {
            event->sensorType = SENSOR_TYPE_SINGLE;
            UserVar[event->BaseVarIndex] = Plugin_222_pulseCounter[event->TaskIndex];
            break;
          }
          case 1:
          {
            event->sensorType = SENSOR_TYPE_SINGLE;
            UserVar[event->BaseVarIndex+1] = Plugin_222_pulseTotalCounter[event->TaskIndex];
            break;
          }
          case 2:
          {
            event->sensorType = SENSOR_TYPE_DUAL;
            UserVar[event->BaseVarIndex] = Plugin_222_pulseCounter[event->TaskIndex];
            UserVar[event->BaseVarIndex+1] = Plugin_222_pulseTotalCounter[event->TaskIndex];
            break;
          }
        }
        Plugin_222_pulseCounter[event->TaskIndex] = 0;
        success = true;
        break;
      }
  }
  return success;
}

#endif // USES_P222
