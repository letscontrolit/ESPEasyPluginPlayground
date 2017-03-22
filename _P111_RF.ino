//#######################################################################################################
//#################################### Plugin 111: Input RF #############################################
//#######################################################################################################
/*
Version: 1.1
Description: use this script to recieve RF with a cheap MX-05V alike reciever
Author: S4nder
Copyright: (c) 2015-2016 Sander Pleijers (s4nder)
License: MIT
License URI: http://en.wikipedia.org/wiki/MIT_License
Status : "Proof of concept"

This program was developed independently and it is not supported in any way.
*/

#include <RCSwitch.h>
RCSwitch *rfReceiver;

#define PLUGIN_111
#define PLUGIN_ID_111         111
#define PLUGIN_NAME_111       "RF Recieve - MX-05V alike reciever"
#define PLUGIN_ValueNAME1_111 "RF"

boolean Plugin_111(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_111;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_LONG;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_111);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_ValueNAME1_111));
        break;
      }

    case PLUGIN_INIT:
      {
        int rfPin = Settings.TaskDevicePin1[event->TaskIndex];
        //Serial.println(String(rfPin));
        if (irReceiver != 0) {
          String log = F("BUG: Cannot use IR reciever and RF reciever at the same time!");
          Serial.print(log);
          addLog(LOG_LEVEL_INFO, log);
          delete rfReceiver;
          rfReceiver = 0;
        } else {
          if (rfPin != -1)
          {
            Serial.println("INIT: RF433 RX created!");
            rfReceiver = new RCSwitch();
            rfReceiver->enableReceive(rfPin);
          }
          if (rfReceiver != 0 && rfPin == -1)
          {
            Serial.println("INIT: RF433 RX removed!");
            rfReceiver->resetAvailable();
            delete rfReceiver;
            rfReceiver = 0;
          }
        }
        success = true;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if (irReceiver != 0) break;
        if (rfReceiver->available())
        {
          Serial.print("RF recieved");
          int valuerf = rfReceiver->getReceivedValue();

          if (valuerf == 0) {
            Serial.print("Unknown encoding");

            String log = F("RF Code Recieved: ");
            log += String(valuerf);
            log += " =Unknown encoding";
            addLog(LOG_LEVEL_INFO, log);
          } else {
            Serial.print("Received ");
            Serial.print( rfReceiver->getReceivedValue() );
            Serial.print(" / ");
            Serial.print( rfReceiver->getReceivedBitlength() );
            Serial.print("bit ");
            Serial.print("Protocol: ");
            Serial.println( rfReceiver->getReceivedProtocol() );

            UserVar[event->BaseVarIndex] = (valuerf & 0xFFFF);
            UserVar[event->BaseVarIndex + 1] = ((valuerf >> 16) & 0xFFFF);
            
            String log = F("RF Code Recieved: ");
            log += String(valuerf);
            addLog(LOG_LEVEL_INFO, log);
            
            /*
            Usage:
            1=RFSEND
            2=commando
            3=repeat (if not set will use default settings)
            4=bits (if not set will use default settings)
            
                                                1      2              3  4
            http://<ESP IP address>/control?cmd=RFSEND,blablacommando,10,24
            Output this in logging:
            */
            
            String url = String(Settings.Name) + "/control?cmd=RFSEND," + String(rfReceiver->getReceivedValue()) + ",1," + String(rfReceiver->getReceivedBitlength()) ;
            String printString = F("For sending this command,");
            addLog(LOG_LEVEL_INFO, printString);
            printString = F("use URL: <a href=\"http://");
            printString += url;
            printString += F("\">http://");
            printString += url;
            printString += F("</a>");
            addLog(LOG_LEVEL_INFO, printString);
            
            sendData(event);
          }
          rfReceiver->resetAvailable();
        }
        success = true;
        break;
      }
  }
  return success;
}


