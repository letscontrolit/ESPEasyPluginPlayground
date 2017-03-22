//#######################################################################################################
//########################### Plugin 112: Output 433 MHZ - RF                      ###########################
//#######################################################################################################
/*
  Version: 1.0
  Description: use this script to send RF with a cheap FS1000A alike sender
  Example of usage:

  Learn codes via _P111_RF.ino plugin!
  Needs: RCSwitch library
  Tested on GPIO:14
  Author: S4nder
  Copyright: (c) 2015-2016 Sander Pleijers (s4nder)
  License: MIT
  License URI: http://en.wikipedia.org/wiki/MIT_License
  Status : "Proof of concept"

  Usage:
  1=RFSEND
  2=commando
  3=repeat (if not set will use default settings)
  4=bits (if not set will use default settings)

                                    1      2              3  4
  http://<ESP IP address>/control?cmd=rfsend,blablacommando,10,24

  This program was developed independently and it is not supported in any way.
*/

#include <RCSwitch.h>
RCSwitch *rcswitchSender;

#define PLUGIN_112
#define PLUGIN_ID_112         112
#define PLUGIN_NAME_112       "RF Transmit - FS1000A alike sender"

unsigned long Plugin_112_Repeat;
unsigned long Plugin_112_Bits;
unsigned long Plugin_112_Pulse;

boolean Plugin_112(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_112;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].SendDataOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_112);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char tmpString[128];
        Plugin_112_Bits = ExtraTaskSettings.TaskDevicePluginConfigLong[0];
        if (Plugin_112_Bits < 100) Plugin_112_Bits = 24;
        Plugin_112_Pulse = ExtraTaskSettings.TaskDevicePluginConfigLong[1];
        if (Plugin_112_Pulse > 1000) Plugin_112_Pulse = 165;
        Plugin_112_Repeat = ExtraTaskSettings.TaskDevicePluginConfigLong[2];
        if (Plugin_112_Repeat > 10) Plugin_112_Pulse = 1;
        sprintf_P(tmpString, PSTR("<TR><TD>Bits (default=24):<TD><input type='text' name='plugin_112_bits' value='%u'>"), Plugin_112_Bits);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Pulselength (default=165):<TD><input type='text' name='plugin_112_pulse' value='%u'>"), Plugin_112_Pulse);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Repeat (default=1):<TD><input type='text' name='plugin_112_repeat' value='%u'>"), Plugin_112_Repeat);
        string += tmpString;
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_112_bits");
        Plugin_112_Bits = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_112_pulse");
        Plugin_112_Pulse = plugin2.toInt();
        String plugin3 = WebServer.arg("plugin_112_repeat");
        Plugin_112_Repeat = plugin3.toInt();
        if (Plugin_112_Bits > 100) Plugin_112_Bits = 24;
        if (Plugin_112_Pulse > 1000) Plugin_112_Pulse = 165;
        if (Plugin_112_Pulse > 10) Plugin_112_Repeat = 1;
        ExtraTaskSettings.TaskDevicePluginConfigLong[0] = Plugin_112_Bits;
        ExtraTaskSettings.TaskDevicePluginConfigLong[1] = Plugin_112_Pulse;
        ExtraTaskSettings.TaskDevicePluginConfigLong[2] = Plugin_112_Repeat;
        SaveTaskSettings(event->TaskIndex);
        success = true;
        break;
      }


    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += String(ExtraTaskSettings.TaskDevicePluginConfigLong[0]);
        string += String(ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
        string += String(ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadTaskSettings(event->TaskIndex);
        Plugin_112_Bits = ExtraTaskSettings.TaskDevicePluginConfigLong[0];
        Plugin_112_Pulse = ExtraTaskSettings.TaskDevicePluginConfigLong[1];
        Plugin_112_Repeat = ExtraTaskSettings.TaskDevicePluginConfigLong[2];
        //if (Plugin_112_Bits > 100) Plugin_112_Bits = 24;
        if (Plugin_112_Pulse > 1000) Plugin_112_Pulse = 165;
        //if (Plugin_112_Repeat > 10) Plugin_112_Repeat = 1;
        int txPin = Settings.TaskDevicePin1[event->TaskIndex];

        if (rcswitchSender == 0 && txPin != -1)
        {
          addLog(LOG_LEVEL_INFO, "INIT: RF433 TX created!");
          //rcswitchSender = new RCSwitch(txPin);

          rcswitchSender = new RCSwitch();

          rcswitchSender->enableTransmit(txPin);
          rcswitchSender->setPulseLength(Plugin_112_Pulse);
        }

        if (rcswitchSender != 0 && txPin == -1)
        {
          addLog(LOG_LEVEL_INFO, "INIT: RF433 TX REMOVED!");
          delete rcswitchSender;
          rcswitchSender = 0;
        }
        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
      }

    case PLUGIN_WRITE:
      {
        String Plugin_112_Result = "";
        unsigned int Plugin_112_DeviceNr = 0;
        unsigned int Plugin_112_Value = 0;

        char command[80]; command[0] = 0;
        char TmpStr1[80]; TmpStr1[0] = 0;
        string.toCharArray(command, 80);

        String tmpString = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex) tmpString = tmpString.substring(0, argIndex);

        if (GetArgv(command, TmpStr1, 2)) Plugin_112_Value = str2int(TmpStr1);
        if (GetArgv(command, TmpStr1, 3)) Plugin_112_Repeat = str2int(TmpStr1);
        if (GetArgv(command, TmpStr1, 4)) Plugin_112_Bits = str2int(TmpStr1);
        if (Plugin_112_Repeat > 10) Plugin_112_Repeat = 1;
        if (Plugin_112_Bits > 100) Plugin_112_Bits = 24;

        if (tmpString.equalsIgnoreCase("RFSEND") && rcswitchSender != 0 && Plugin_112_Bits != 0)
        {
          Serial.println("RFSEND");
          success = true;
          Plugin_112_Result = "Unknown RF command or structure !!!";


          for (int i = 0; i <= Plugin_112_Repeat; i++) {
            rcswitchSender->send(Plugin_112_Value, Plugin_112_Bits);
            //delay(1000);
          }

          addLog(LOG_LEVEL_INFO, "RF Code Sent: " + String(Plugin_112_Value));
          if (printToWeb)
          {
            String url = String(Settings.Name) + "/control?cmd=" + string;
            printWebString += F("RCSwitch Code Sent!");
            printWebString += F("<BR>Value: ");
            printWebString += String(Plugin_112_Value);
            printWebString += F("<BR>Repeats: ");
            printWebString += String(Plugin_112_Repeat);
            printWebString += F("<BR>Bits: ");
            printWebString += String(Plugin_112_Bits);
            printWebString += F("<BR>Pulselength: ");
            printWebString += String(Plugin_112_Pulse);
            printWebString += F("<BR><BR>");
            printWebString += F("<BR>Use URL: <a href=\"http://");
            printWebString += url;
            printWebString += F("\">http://");
            printWebString += url;
            printWebString += F("</a>");
          }

        }
        break;
      }


  }
  return success;
}
