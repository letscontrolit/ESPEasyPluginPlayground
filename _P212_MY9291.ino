//#######################################################################################################
//#################################### Plugin 212: MY9291 Basic #########################################
//#######################################################################################################

// The MY9291 led driver is a device found in smart lightbulbs such as the AI Thinker AILight and many 
// other bulbs that are either clones or licenced designs.  This plugin offers RGBW control of these
// bulbs.  The plugin is designed to control 4 channels, the MY9291 can be used in 3 channel mode as well
// 3 Channel should work I think, but is untested

// This plugin requires modification of the EventStruct in order to function. A Par4 integer needs to be
// added currently.  The parseCommandString function will also need to be updated as well.  

// The MY9291 library (from tinkerman) is required for this plugin to function correctly.  
// https://github.com/xoseperez/my9291

// Although the MY9291 led driver can operate in 8bit, 16bit and 32bit modes, the default for the MY9291
// is used.  This offers 8bit control of each channel i.e. a 32bit command is sent on each change

// List of commands:
// (1) MY9291,<red 0-255>,<green 0-255>,<blue 0-255>,<white 0-255>

// Usage:
// (1): Set RGBW Color to specified LED number (eg. MY9291,255,255,255,20)

#include <my9291.h>
my9291 *Plugin_212_rgbw;

#define PLUGIN_212
#define PLUGIN_ID_212         212
#define PLUGIN_NAME_212       "MY9291"
#define PLUGIN_VALUENAME1_212 "RGB"

boolean Plugin_212(byte function, struct EventStruct *event, String& string)
{
	boolean success = false;

	switch (function)
	{

	case PLUGIN_DEVICE_ADD:
	{
		Device[++deviceCount].Number = PLUGIN_ID_212;
		Device[deviceCount].Type = DEVICE_TYPE_DUAL;
		Device[deviceCount].Custom = true;
		Device[deviceCount].TimerOption = false;
		break;
	}

	case PLUGIN_GET_DEVICENAME:
	{
		string = F(PLUGIN_NAME_212);
		break;
	}

	case PLUGIN_GET_DEVICEVALUENAMES:
	{
		strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_212));
		break;
	}

	case PLUGIN_WEBFORM_LOAD:
	{
		char tmpString[128];

		string += F("<TR><TD>DI Pin:<TD>");
		addPinSelect(false, string, "taskdevicepin1", Settings.TaskDevicePin1[event->TaskIndex]);
		string += F("<TR><TD>DCKI Pin:<TD>");
		addPinSelect(false, string, "taskdevicepin2", Settings.TaskDevicePin2[event->TaskIndex]);

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
		if (!Plugin_212_rgbw)
		{
			Plugin_212_rgbw = new my9291(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex], MY9291_COMMAND_DEFAULT);
			Plugin_212_rgbw->setState(true);
		}
		success = true;
		break;
	}

	case PLUGIN_WRITE:
	{
		if (Plugin_212_rgbw)
		{
			String tmpString = string;
			int argIndex = tmpString.indexOf(',');
			if (argIndex)
				tmpString = tmpString.substring(0, argIndex);

			if (tmpString.equalsIgnoreCase(F("MY9291")))
			{
				Plugin_212_rgbw->setColor((my9291_color_t) { event->Par1, event->Par2, event->Par3, event->Par4 });
				Plugin_212_rgbw->setState(true);
				success = true;
			}
		}

		break;
	}
	return success;
	}
}

