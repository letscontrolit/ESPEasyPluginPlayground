#ifdef USES_P246

// #######################################################################################################
// ################################## Plugin 246: fast Analog ############################################
// #######################################################################################################

#define PLUGIN_246
#define PLUGIN_ID_246         246
#define PLUGIN_NAME_246       "Analog input - internal (fast)"
#define PLUGIN_VALUENAME1_246 "Analog"

#include "_Plugin_Helper.h"

#ifdef ESP32
  # define P246_MAX_ADC_VALUE    4095
#endif // ifdef ESP32
#ifdef ESP8266
  # define P246_MAX_ADC_VALUE    1023
#endif // ifdef ESP8266

#define P246_10XPERSECOND        0 //Values for selectbox - 0 = selected
#define P246_50XPERSECOND        1
#define P246_SAMPLERATE          PCONFIG(0)
#define P246_THRESHOLD           PCONFIG_LONG(0)
#define P246_THREStriggerABOVE   0 //Values for selectbox - 0 = selected
#define P246_THREStriggerBELOW   1
#define P246_THREStriggerSAME    2
#define P246_TRIGGER             PCONFIG(1)
#define P246_sendDataDelay       PCONFIG_LONG(1)
#define P246_DEBUG               PCONFIG(2)

unsigned long time_now = 0;
bool dataSended = 0;

struct P246_data_struct : public PluginTaskData_base {
  P246_data_struct() {}

  ~P246_data_struct() {}
};

boolean Plugin_246(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_246;
      Device[deviceCount].Type               = DEVICE_TYPE_ANALOG;
      Device[deviceCount].VType              = SENSOR_TYPE_SINGLE;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = 1;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].TimerOptional      = true;
      Device[deviceCount].GlobalSyncOption   = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_246);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_246));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      #if defined(ESP32)
      addHtml(F("<TR><TD>Analog Pin:<TD>"));
      addADC_PinSelect(false, F("taskdevicepin1"), CONFIG_PIN1);

      #endif // if defined(ESP32)
      addFormSubHeader(F("Configuration"));
      addFormCheckBox(F("Debug"), F("p246_debug"),P246_DEBUG);
      addFormNote(F("Outputs serial log and sends data to Controller 10x/50x per Second (HIGH LOAD!!!)"));
      addFormNumericBox(F("Threshold"), F("p246_thres"), P246_THRESHOLD, 0, P246_MAX_ADC_VALUE);

      String thresh_trigger[3];
      byte   choice1 = P246_TRIGGER;
      thresh_trigger[0] = F("ADC >= Threshold");
      thresh_trigger[1] = F("ADC <= Threshold");
      thresh_trigger[2] = F("ADC == Threshold");
      int thresh_triggers[3]   = { P246_THREStriggerABOVE , P246_THREStriggerBELOW , P246_THREStriggerSAME };
      addFormSelector(F("Trigger"), F("p246_threstrigger"), 3, thresh_trigger, thresh_triggers, choice1);
      
      addFormNumericBox(F("Send to Controller Delay"), F("p246_sendDelay"), P246_sendDataDelay, 1, 9999);
      addUnit(F("sec"));
      addFormNote(F("Delay between sending to Controller (sending Data 10x/50x per sec isn't useful)"));

      String options[2];
      byte   choice2 = P246_SAMPLERATE;
      options[0] = F("10 Times per Second");
      options[1] = F("50 Times per Second");
      int optionValues[2]   = { P246_10XPERSECOND, P246_50XPERSECOND };
      addFormSelector(F("Samplerate"), F("p246_samplerate"), 2, options, optionValues, choice2);
      addFormNote(F("50x produces more load than 10x!!!"));
      {
        // Output the statistics for the current settings.
        int   raw_value = 0;
        if (P246_performRead(event, raw_value)) {
          P246_formatStatistics(F("Current"), raw_value, 0);
        }

      }

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      P246_THRESHOLD = getFormItemInt(F("p246_thres"));
      P246_SAMPLERATE = getFormItemInt(F("p246_samplerate"));
      P246_TRIGGER = getFormItemInt(F("p246_threstrigger"));
      P246_sendDataDelay = getFormItemInt(F("p246_sendDelay"));
      P246_DEBUG = isFormItemChecked(F("p246_debug"));

      success = true;
      break;
    }

    case PLUGIN_EXIT: {
      clearPluginTaskData(event->TaskIndex);
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      initPluginTaskData(event->TaskIndex, new P246_data_struct());
      P246_data_struct *P246_data =
        static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

      P246_sendDataDelay = 1;

      success = (nullptr != P246_data);
      break;
    }
    case PLUGIN_TEN_PER_SECOND: //10xPerSec
    {
      if (P246_SAMPLERATE == P246_10XPERSECOND && (!dataSended || P246_DEBUG)) 
      {
        time_now = millis();
        P246_data_struct *P246_data =
          static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (nullptr != P246_data) {
          int currentValue;

          P246_performRead(event, currentValue);
          //P246_data->addOversamplingValue(currentValue);
          String log = F("10x/sec ADC : ");

          switch (P246_TRIGGER) {
            case P246_THREStriggerABOVE:
              if (currentValue >= P246_THRESHOLD) {
                log += F("ADC >= ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
            case P246_THREStriggerBELOW:
              if (currentValue <= P246_THRESHOLD) {
                log += F("ADC <= ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
            case P246_THREStriggerSAME:
              if (currentValue == P246_THRESHOLD) {
                log += F("ADC == ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
          }         
        }
      } else {
         if (millis() > time_now + P246_sendDataDelay*1000) { //Wait for sendDelay before check ADC and send Data again
           dataSended = 0;
         }
      }
      success = true;
      break;
    }

    case PLUGIN_FIFTY_PER_SECOND:  //50xPerSec
    {
      if (P246_SAMPLERATE == P246_50XPERSECOND && (!dataSended || P246_DEBUG))
      {
        time_now = millis();
        P246_data_struct *P246_data =
          static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (nullptr != P246_data) {
          int currentValue;

          P246_performRead(event, currentValue);
          //P246_data->addOversamplingValue(currentValue);
          String log = F("50x/sec ADC : ");

          switch (P246_TRIGGER) {
            case P246_THREStriggerABOVE:
              if (currentValue >= P246_THRESHOLD) {
                log += F("ADC >= ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
            case P246_THREStriggerBELOW:
              if (currentValue <= P246_THRESHOLD) {
                log += F("ADC <= ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
            case P246_THREStriggerSAME:
              if (currentValue == P246_THRESHOLD) {
                log += F("ADC == ");
                log += String(P246_THRESHOLD);
                log += F(" : Value: ");
                log += String(currentValue);
                addLog(LOG_LEVEL_INFO, log);
                UserVar[event->BaseVarIndex] = currentValue;
                sendData(event);
                dataSended = 1;
              }
              break;
          }         
        }
      } else {
         if (millis() > time_now + P246_sendDataDelay*1000) { //Wait for sendDelay before check ADC and send Data again
           dataSended = 0;
         }
      }
      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      int   raw_value = 0;

      if (P246_performRead(event, raw_value)) {
        //UserVar[event->BaseVarIndex] = res_value;
        UserVar[event->BaseVarIndex] = raw_value;

        P246_data_struct *P246_data =
          static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (nullptr != P246_data) {
          if (loglevelActiveFor(LOG_LEVEL_INFO)) {
            String log = F("ADC  : Analog value: ");
            log += String(raw_value);
            log += F(" = ");
            log += String(UserVar[event->BaseVarIndex], 3);
            addLog(LOG_LEVEL_INFO, log);
          }
          success = true;
        } else {
          addLog(LOG_LEVEL_ERROR, F("ADC  : No value received "));
          success = false;
        }
      }

      break;
    }
  }
  return success;
}


bool P246_performRead(struct EventStruct *event, int& value) {
  #ifdef ESP8266
  value = espeasy_analogRead(A0);
  #endif // if defined(ESP8266)
  #if defined(ESP32)
  value = espeasy_analogRead(CONFIG_PIN1);
  #endif // if defined(ESP32)
  return true;
}

void P246_formatStatistics(const String& label, int raw, float float_value) {
  addRowLabel(label);
  addHtml(String(raw));
  //html_add_estimate_symbol();
  //addHtml(String(float_value, 3));
}

#endif // USES_P246
