//#######################################################################################################
//#################################### Plugin 167: ADS1015 I2C 0x48)  ###################################
//# Supports slow, purple chinese ADS1015
//# Needs soligen2010/Adafruit_ADS1x15 patched library
//# https://github.com/soligen2010/Adafruit_ADS1X15
//# WARNING: This plugin can not coexist with ADS1115 plugin!
//#######################################################################################################

#ifdef PLUGIN_BUILD_TESTING

#include <Adafruit_ADS1015.h>

#define ARDUINO_ARCH_ESP8266
#define PLUGIN_167
#define PLUGIN_ID_167 167
#define PLUGIN_NAME_167 "Analog input - ADS1015"
#define PLUGIN_VALUENAME1_167 "AIN0"
#define PLUGIN_VALUENAME2_167 "AIN1"
#define PLUGIN_VALUENAME3_167 "AIN2"
#define PLUGIN_VALUENAME4_167 "AIN3"

boolean Plugin_167_init = false;
Adafruit_ADS1015 Plugin_167_ads;
static byte Plugin_167_muxes[5];

boolean Plugin_167(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_167;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_167);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_167));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_167));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_167));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_167));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
#define ADS1015_I2C_OPTION 4
        byte addr = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int optionValues[ADS1015_I2C_OPTION] = { 0x48, 0x49, 0x4A, 0x4B };
        addFormSelectorI2C(string, F("plugin_167_i2c"), ADS1015_I2C_OPTION, optionValues, addr);

        addFormSubHeader(string, F("Input"));

#define ADS1015_PGA_OPTION 6
        byte pga = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String pgaOptions[ADS1015_PGA_OPTION] = {
          F("2/3x gain (FS=6.144V)"),
          F("1x gain (FS=4.096V)"),
          F("2x gain (FS=2.048V)"),
          F("4x gain (FS=1.024V)"),
          F("8x gain (FS=0.512V)"),
          F("16x gain (FS=0.256V)")
        };
        addFormSelector(string, F("Gain"), F("plugin_167_gain"), ADS1015_PGA_OPTION, pgaOptions, NULL, pga);

        addFormSubHeader(string, F("Used analog input pins:"));

        addFormCheckBox(string, F("AIN0"), F("plugin_167_ain0"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        addFormCheckBox(string, F("AIN1"), F("plugin_167_ain1"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
        addFormCheckBox(string, F("AIN2"), F("plugin_167_ain2"), Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
        addFormCheckBox(string, F("AIN3"), F("plugin_167_ain3"), Settings.TaskDevicePluginConfig[event->TaskIndex][5]);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_167_i2c"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_167_gain"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = isFormItemChecked(F("plugin_167_ain0"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = isFormItemChecked(F("plugin_167_ain1"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = isFormItemChecked(F("plugin_167_ain2"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = isFormItemChecked(F("plugin_167_ain3"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_167_ads = Adafruit_ADS1015(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        Plugin_167_ads.begin(Settings.Pin_i2c_sda, Settings.Pin_i2c_scl);
        Plugin_167_ads.setSPS(ADS1015_DR_128SPS);
        Plugin_167_init = true;
        byte pga = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        switch (pga)
        {
          case 0:
            {
              Plugin_167_ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
              break;
            }
          case 1:
            {
              Plugin_167_ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
              break;
            }
          case 2:
            {
              Plugin_167_ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
              break;
            }
          case 3:
            {
              Plugin_167_ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
              break;
            }
          case 4:
            {
              Plugin_167_ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
              break;
            }
          case 5:
            {
              Plugin_167_ads.setGain(GAIN_SIXTEEN);
              break;
            }
        }

        byte apin = 0;
        Plugin_167_muxes[4] = 0;
        while (apin < 4) {
          if (Settings.TaskDevicePluginConfig[event->TaskIndex][2 + apin] == true) {
            Plugin_167_muxes[apin] = 1;
            Plugin_167_muxes[4] = Plugin_167_muxes[4] + 1;
          } else {
            Plugin_167_muxes[apin] = 0;
          }
          apin++;
        }

        success = true;
        break;
      }

    case PLUGIN_READ:
      {

        int16_t value = 0;

        String log = F("ADS1015 : ");
        if (Plugin_167_muxes[4] > 0) { // only start if at least one analog pin selected

          log += F("Analog value:");
          if (Plugin_167_muxes[4] == 1) { // only one channel
            byte apin = 0;
            while (apin < 4) {
              if (Plugin_167_muxes[apin] == 1) {
                value = Plugin_167_ads.readADC_SingleEnded(apin);
                UserVar[event->BaseVarIndex + apin] = (float)value;
                log += F(" A");
                log += apin;
                log = F("=");
                log += value;
              }
              apin++;
            }
          } else { // multiple channels selected

            if (Plugin_167_muxes[1] == 1) { // mux1 in list
              value = Plugin_167_ads.readADC_SingleEnded(1);  // fetch AIN1 as reference
              UserVar[event->BaseVarIndex + 1] = (float)value;
              if (Plugin_167_muxes[0] == 1) {
                UserVar[event->BaseVarIndex] = UserVar[event->BaseVarIndex + 1] + Plugin_167_ads.readADC_Differential_0_1();
              }
              if (Plugin_167_muxes[3] == 1) {
                UserVar[event->BaseVarIndex + 3] = UserVar[event->BaseVarIndex + 1] - Plugin_167_ads.readADC_Differential_1_3();
              }
              if (Plugin_167_muxes[2] == 1) {
                value = Plugin_167_ads.readADC_SingleEnded(2);
                UserVar[event->BaseVarIndex + 2] = (float)value;
              }
              value = Plugin_167_ads.readADC_SingleEnded(1);  // fetch AIN1 to memory - last reading sometime arrives in the next read cycle as result for any channel
            } else { // mux1 not in list

              if (Plugin_167_muxes[3] == 1) { // mux3 in list
                value = Plugin_167_ads.readADC_SingleEnded(3);  // fetch AIN3 as reference
                UserVar[event->BaseVarIndex + 3] = (float)value;
                if (Plugin_167_muxes[0] == 1) {
                  UserVar[event->BaseVarIndex] = UserVar[event->BaseVarIndex + 3] + Plugin_167_ads.readADC_Differential_0_3();
                }
                if (Plugin_167_muxes[2] == 1) {
                  UserVar[event->BaseVarIndex + 2] = UserVar[event->BaseVarIndex + 3] + Plugin_167_ads.readADC_Differential_2_3();
                }
                value = Plugin_167_ads.readADC_SingleEnded(3);  // fetch AIN3 to memory
              } else { // end of mux3

                if (Plugin_167_muxes[0] == 1) { // mux0 in list
                  value = Plugin_167_ads.readADC_SingleEnded(0);  // fetch AIN0 as reference
                  UserVar[event->BaseVarIndex] = (float)value;
                  value = Plugin_167_ads.readADC_SingleEnded(2);
                  UserVar[event->BaseVarIndex + 2] = (float)value;
                  value = Plugin_167_ads.readADC_SingleEnded(0);  // fetch AIN0 to memory
                }
              } // end of not mux3
            } // end of not mux1
          }
        }

        addLog(LOG_LEVEL_DEBUG, log);
        success = true;
        break;
      }
  }
  return success;
}

#endif