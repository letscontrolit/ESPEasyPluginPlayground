//#######################################################################################################
//#################################### Plugin 167: ADS1015 I2C 0x48)  ###################################
//#######################################################################################################

#include <Adafruit_ADS1015.h>

#define ARDUINO_ARCH_ESP8266
#define PLUGIN_167
#define PLUGIN_ID_167 25
#define PLUGIN_NAME_167 "Analog input - ADS1015"
#define PLUGIN_VALUENAME1_167 "Analog"

boolean Plugin_167_reading = false;
boolean Plugin_167_init = false;
Adafruit_ADS1015 Plugin_167_ads;

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
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
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

#define ADS1015_MUX_OPTION 8
        byte mux = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        String muxOptions[ADS1015_MUX_OPTION] = {
          F("AIN0 - AIN1 (Differential)"),
          F("AIN0 - AIN3 (Differential)"),
          F("AIN1 - AIN3 (Differential)"),
          F("AIN2 - AIN3 (Differential)"),
          F("AIN0 - GND (Single-Ended)"),
          F("AIN1 - GND (Single-Ended)"),
          F("AIN2 - GND (Single-Ended)"),
          F("AIN3 - GND (Single-Ended)"),
        };
        addFormSelector(string, F("Input Multiplexer"), F("plugin_167_mode"), ADS1015_MUX_OPTION, muxOptions, NULL, mux);

        addFormSubHeader(string, F("Two Point Calibration"));

        addFormCheckBox(string, F("Calibration Enabled"), F("plugin_167_cal"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);

        addFormNumericBox(string, F("Point 1"), F("plugin_167_adc1"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][0], -32768, 32767);
        string += F(" &#8793; ");
        addTextBox(string, F("plugin_167_out1"), String(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0], 3), 10);

        addFormNumericBox(string, F("Point 2"), F("plugin_167_adc2"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][1], -32768, 32767);
        string += F(" &#8793; ");
        addTextBox(string, F("plugin_167_out2"), String(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1], 3), 10);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_167_i2c"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_167_gain"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_167_mode"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = isFormItemChecked(F("plugin_167_cal"));

        Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = getFormItemInt(F("plugin_167_adc1"));
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = getFormItemFloat(F("plugin_167_out1"));

        Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = getFormItemInt(F("plugin_167_adc2"));
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1] = getFormItemFloat(F("plugin_167_out2"));

        Plugin_167_init = false; // Force device setup next time
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_167_ads = Adafruit_ADS1015(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        Plugin_167_ads.begin(Settings.Pin_i2c_sda, Settings.Pin_i2c_scl);
        Plugin_167_ads.setSPS(ADS1015_DR_250SPS);
        Plugin_167_init = true;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {

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



        byte count = 0;
        boolean plugin167_success = false;
        int16_t value = 0;
        String log = F("ADS1015 : Analog value: ");
        byte mux = Settings.TaskDevicePluginConfig[event->TaskIndex][2];

        while ((count < 10) && (plugin167_success == false)) {  // eliminate possible race conditions
          if (Plugin_167_reading == false) {

            Plugin_167_reading = true;

            switch (mux)
            {
              case 0:
                {
                  value = Plugin_167_ads.readADC_Differential_0_1();
                  break;
                }
              case 1:
                {
                  value = Plugin_167_ads.readADC_Differential_0_3();
                  break;
                }
              case 2:
                {
                  value = Plugin_167_ads.readADC_Differential_1_3();
                  break;
                }
              case 3:
                {
                  value = Plugin_167_ads.readADC_Differential_2_3();
                  break;
                }
              case 4:
                {
                  value = Plugin_167_ads.readADC_SingleEnded(0);
                  break;
                }
              case 5:
                {
                  value = Plugin_167_ads.readADC_SingleEnded(1);
                  break;
                }
              case 6:
                {
                  value = Plugin_167_ads.readADC_SingleEnded(2);
                  break;
                }
              case 7:
                {
                  value = Plugin_167_ads.readADC_SingleEnded(3);
                  break;
                }
            }
            Plugin_167_reading = false;
            plugin167_success = true;
          } else {
            delay(8);
          }
          count++;
        }


        UserVar[event->BaseVarIndex] = (float)value;
        log += value;

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][3])   //Calibration?
        {
          int adc1 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][0];
          int adc2 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][1];
          float out1 = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
          float out2 = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1];
          if (adc1 != adc2)
          {
            float normalized = (float)(value - adc1) / (float)(adc2 - adc1);
            UserVar[event->BaseVarIndex] = normalized * (out2 - out1) + out1;

            log += F(" ");
            log += UserVar[event->BaseVarIndex];
          }
        }

        //TEST log += F(" @0x");
        //TEST log += String(config, 16);
        addLog(LOG_LEVEL_DEBUG, log);
        success = true;
        break;
      }
  }
  return success;
}
