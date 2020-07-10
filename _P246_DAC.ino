#ifdef USES_P246
//#######################################################################################################
//#################################### Plugin 246: ESP32 DAC ############################################
//
// DAC for ESP32
//
//#######################################################################################################

#include "_Plugin_Helper.h"

#define PLUGIN_246
#define PLUGIN_ID_246         246
#define PLUGIN_NAME_246       "ESP32 DAC"
#define PLUGIN_VALUENAME1_246 "Output"

#define P246_DAC_VALUE        UserVar[event->BaseVarIndex]

// Start: add to Hardware.ino
#ifdef ESP32

// Get DAC related info for a given GPIO pin
// @param gpio_pin   GPIO pin number
// @param dac        Number of DAC unit
bool getDAC_gpio_info(int gpio_pin, int& dac)
{
  switch (gpio_pin) {
    case 25: dac = 1; break;
    case 26: dac = 2; break;
    default:
      return false;
  }
  return true;
}

#endif
// End: add to Hardware.ino

// Start: add to Misc.ino
#ifdef ESP32

String formatGpioName_DAC(int gpio_pin) {
  int dac;
  if (getDAC_gpio_info(gpio_pin, dac)) {
    return String(F("DAC")) + dac;
  }
  return "";
}

#endif
// End: add to Misc.ino

// Start: add to WebServer_Markup.ino
#ifdef ESP32

void addDAC_PinSelect(const String& id,  int choice)
{
  int NR_ITEMS_PIN_DROPDOWN = 2;
  String *gpio_labels  = new String[NR_ITEMS_PIN_DROPDOWN];
  int    *gpio_numbers = new int[NR_ITEMS_PIN_DROPDOWN];

  int i    = 0;
  int gpio = -1;

  while (i < NR_ITEMS_PIN_DROPDOWN && gpio <= MAX_GPIO) {
    int  pinnr = -1;
    bool input, output, warning;

    if (getGpioInfo(gpio, pinnr, input, output, warning)) {
      int dac;
      if (getDAC_gpio_info(gpio, dac)) {
        gpio_labels[i] = formatGpioName_DAC(gpio);
        if (dac != 0) {
          gpio_labels[i] += F(" / ");
          gpio_labels[i] += createGPIO_label(gpio, pinnr, input, output, warning);
        }
        gpio_numbers[i] = gpio;
        ++i;
      }
    }
    ++gpio;
  }
  bool forI2C = false;
  renderHTMLForPinSelect(gpio_labels, gpio_numbers, forI2C, id, choice, i);
  delete[] gpio_numbers;
  delete[] gpio_labels;
}

#endif
// End: add to WebServer_Markup.ino


boolean Plugin_246(byte function, struct EventStruct * event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number       = PLUGIN_ID_246;
        Device[deviceCount].Type           = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Custom         = true;
        Device[deviceCount].ValueCount     = 1;
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
      addHtml(F("<TR><TD>1st GPIO:<TD>"));
      addDAC_PinSelect(F("taskdevicepin1"), CONFIG_PIN1);

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      success = true;
      break;
    }

    case PLUGIN_WRITE:
    {
      String command = parseString(string, 1);

      if (command == F("dac")) {
        int dac;
        if (getDAC_gpio_info(CONFIG_PIN1, dac) && dac == event->Par1) {
          int value = min(255, max(0, event->Par2));
          P246_DAC_VALUE = value;
          dacWrite(CONFIG_PIN1, value);
          addLog(LOG_LEVEL_DEBUG, formatGpioName_DAC(CONFIG_PIN1) +
              String(F(" : Output ")) + String(value));

          success = true;
        }
      }
      break;
    }
  }

  return success;
}

#endif // USES_P246
