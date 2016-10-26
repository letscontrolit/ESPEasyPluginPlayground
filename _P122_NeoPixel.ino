//#######################################################################################################
//#################################### Plugin 122: NeoPixel clock #######################################
//#######################################################################################################

// Command: NeoPixel <led nr>,<red>,<green>,<blue>

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel *Plugin_122_pixels;

#define PLUGIN_122
#define PLUGIN_ID_122         122
#define PLUGIN_NAME_122       "NeoPixel Basic"
#define PLUGIN_VALUENAME1_122 ""
boolean Plugin_122(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_122;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Custom = true;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_122);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_122));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char tmpString[128];
        sprintf_P(tmpString, PSTR("<TR><TD>Led Count:<TD><input type='text' name='plugin_122_leds' size='3' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        string += tmpString;

        string += F("<TR><TD>GPIO:<TD>");
        addPinSelect(false, string, "taskdevicepin1", Settings.TaskDevicePin1[event->TaskIndex]);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg(F("plugin_122_leds"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!Plugin_122_pixels)
        {
          Plugin_122_pixels = new Adafruit_NeoPixel(Settings.TaskDevicePluginConfig[event->TaskIndex][0], Settings.TaskDevicePin1[event->TaskIndex], NEO_GRB + NEO_KHZ800);
          Plugin_122_pixels->begin(); // This initializes the NeoPixel library.
        }
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        if (Plugin_122_pixels)
        {
          String tmpString  = string;
          int argIndex = tmpString.indexOf(',');
          if (argIndex)
            tmpString = tmpString.substring(0, argIndex);

          if (tmpString.equalsIgnoreCase(F("NeoPixel")))
          {
            char Line[80];
            char TmpStr1[80];
            TmpStr1[0] = 0;
            string.toCharArray(Line, 80);
            int Par4 = 0;
            if (GetArgv(Line, TmpStr1, 5)) Par4 = str2int(TmpStr1);
            Plugin_122_pixels->setPixelColor(event->Par1 - 1, Plugin_122_pixels->Color(event->Par2, event->Par3, Par4));
            Plugin_122_pixels->show(); // This sends the updated pixel color to the hardware.
            success = true;
          }
        }
        break;
      }

  }
  return success;
}

