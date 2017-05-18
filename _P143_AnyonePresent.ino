//#######################################################################################################
//#################################### Plugin 146: Presence #############################################
//#######################################################################################################
// written by Jochen Krapf (jk@nerd2nerd.org)

/*
EN: The recognition of whether a person is in a room is more difficult than one often thinks. It helps if several different sensors are used (e.g., PIR, radar, light barrier, brightness, noise). However, since the sensors only emit short pulses for many pulses, many data are sent to controllers (e.g., MQTT), and the smart home system must recombine these data through complex rules to a steady "someone there".
This plugin listens for up to 3 sensors and triggers a "someone there" when a sensor responds. A "Nobody there" is not sent until all 3 sensors are silent for the set time (Presence Time). A filter prevents unwanted triggering by ESD pulses.
An analog sensor can also be connected. The absolute value of the analog voltage at AIN is irrelevant. The plugin responds to quick changes of the values.

DE: Die Erkennung ob sich eine Person in einem Raum befindet ist schwieriger als man oftmals denkt. Es hilft, wenn man mehrere unterschiedliche Sensoren einsetzt (z.B. PIR, Radar, Lichtschranke, Helligkeit, Geräusch). Da jedoch die Sensoren nur kurze dafür viele Impulse abgeben werden viele Daten an Controller (z.B. MQTT) gesendet und das Smart-Home-System muss diese Daten über aufwendige Regeln wieder zu einem stetigen „Jemand da“ zusammenfassen.
Dieses Plugin lauscht auf bis zu 3 Sensoren und triggert ein „Jemand da“ wenn ein Sensor anspricht. Ein „Keiner da“ wird erst gesendet, wenn alle 3 Sensoren für die eingestellte Zeit (Presence Time) schweigen. Ein Filter verhindert das ungewollte triggern durch ESD-Impulse.
Es kann auch ein analoger Sensor angeschlossen werden. Der Absolutwert der Analog-Spannung an AIN spielt dabei keine Rolle. Das Plugin reagiert auf schnelle Änderungen der Werte.
*/

#ifdef PLUGIN_BUILD_TESTING


//#include <*.h>   - no external lib required

#define PLUGIN_143
#define PLUGIN_ID_143         143
#define PLUGIN_NAME_143       "Anyone Present"
#define PLUGIN_VALUENAME1_143 "Presence"

static long Plugin_143_millisPresenceEnd = 0;
static long Plugin_143_millisPresenceTime = 10000;

static int Plugin_143_pin[3] = {-1,-1,-1};
static byte Plugin_143_lowActive = false;
static byte Plugin_143_useAin = false;
static byte Plugin_143_counter[3] = {0,0,0};


boolean Plugin_143(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_143;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].PullUpOption = true;
        Device[deviceCount].InverseLogicOption = true;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_143);
        break;
      }

      case PLUGIN_GET_DEVICEVALUENAMES:
        {
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_143));
          break;
        }

    case PLUGIN_WEBFORM_LOAD:
      {
        //default values
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] <= 0)   //Plugin_143_millisPresenceTime
          Settings.TaskDevicePluginConfig[event->TaskIndex][0] = 60;

        addFormCheckBox(string, F("Use Analog Input"), F("useain"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

        addFormNumericBox(string, F("Presence Time"), F("ptime"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addUnit(string, F("sec"));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("ptime"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = isFormItemChecked(F("useain"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_143_lowActive = Settings.TaskDevicePin1Inversed[event->TaskIndex];
        Plugin_143_millisPresenceTime = Settings.TaskDevicePluginConfig[event->TaskIndex][0] * 1000;
        Plugin_143_useAin = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

        String log = F("Any1 : GPIO: ");
        for (byte i=0; i<3; i++)
        {
          int pin = Settings.TaskDevicePin[i][event->TaskIndex];
          Plugin_143_pin[i] = pin;
          if (pin >= 0)
          {
            pinMode(pin, (Settings.TaskDevicePin1PullUp[event->TaskIndex]) ? INPUT_PULLUP : INPUT);
            setPinState(PLUGIN_ID_143, pin, PIN_MODE_INPUT, 0);
          }
          log += pin;
          log += F(" ");
        }
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        UserVar[event->BaseVarIndex + 0] = (Plugin_143_millisPresenceEnd > 0);
        success = true;
        break;
      }

    case PLUGIN_FIFTY_PER_SECOND:
    //case PLUGIN_TEN_PER_SECOND:
      {
        boolean presence = false;
        boolean send = false;
        byte value;

        for (byte i=0; i<3; i++)
          if (Plugin_143_pin[i] >= 0)
          {
              value = digitalRead(Plugin_143_pin[i]);
              if (Plugin_143_lowActive)
                value = !value;

              if (value)
              {
                if (Plugin_143_counter[i] <= 5)
                  Plugin_143_counter[i]++;
                //test Serial.printf(".");
              }
              else
                Plugin_143_counter[i] = 0;

              if (Plugin_143_counter[i] > 5)
                presence = true;
          }

        if (Plugin_143_useAin)
        {
          static float filter0 = 0;
          static float filter1 = 0;

          float ain = analogRead(A0) / 1023.0;

          filter0 = 0.95 * filter0 + 0.05 * ain;
          filter1 = 0.90 * filter1 + 0.10 * ain;
          float diff = (filter0 - filter1) * 20.0;
          if (diff < 0)
            diff = -diff;

          if (diff > 1.0)
            presence = true;
        }

        if (presence)   // anyone here now?
        {
          if (Plugin_143_millisPresenceEnd == 0)   // nobody present?
          {
            send = true;
          }
          Plugin_143_millisPresenceEnd = millis() + Plugin_143_millisPresenceTime;
        }
        else   // nobody here now?
        {
          if (Plugin_143_millisPresenceEnd != 0 && Plugin_143_millisPresenceEnd < millis())   // timeout?
          {
            send = true;
            Plugin_143_millisPresenceEnd = 0;
          }
        }

        if (send)
        {
          UserVar[event->BaseVarIndex] = presence;
          event->sensorType = SENSOR_TYPE_SWITCH;

          String log = F("Any1 : Presence=");
          log += presence;
          addLog(LOG_LEVEL_INFO, log);

          sendData(event);
        }

        success = true;
        break;
      }

  }
  return success;
}

#endif
