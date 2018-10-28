

//#######################################################################################################
//#################################### Plugin 111: Input RF #############################################
//#######################################################################################################
/*
   Version: 2.0
   Description: use this script to recieve RF with a cheap MX-05V alike receiver
   Author: S4nder
   Copyright: (c) 2015-2016 Sander Pleijers (s4nder)
   License: MIT
   License URI: http://en.wikipedia.org/wiki/MIT_License
   Status : "Proof of concept"

   This program was developed independently and it is not supported in any way.
 */

#ifdef PLUGIN_BUILD_TESTING

#include <RCSwitch.h>
RCSwitch *rfReceiver;

#define PLUGIN_111
#define PLUGIN_ID_111         111
#define PLUGIN_NAME_111       "RF Receiver - MX-05V alike receiver"
#define PLUGIN_ValueNAME1_111 "RF"

#ifndef USES_P016
 int irReceiver = 0; // make sure it has value even if plugin not found
#endif                

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
                //Serial.println("INIT: RF433 RX rfpin: ");
                //Serial.print(String(rfPin));
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
                                output(rfReceiver->getReceivedValue(), rfReceiver->getReceivedBitlength(), rfReceiver->getReceivedDelay(), rfReceiver->getReceivedRawdata(), rfReceiver->getReceivedProtocol());

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
                                 */

                                String url = String(Settings.Name) + "/control?cmd=RFSEND," + String(rfReceiver->getReceivedValue()) + ",1," + String(rfReceiver->getReceivedBitlength());
                                String printString = F("To send this command, ");
                                //addLog(LOG_LEVEL_INFO, printString);
                                printString += F("use this: <a href=\"http://");
                                printString += url;
                                printString += F("\">URL</a>");
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

/* extended logging, for in terminal monitor */
static const char* bin2tristate(const char* bin);
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength);

void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol) {

        if (decimal == 0) {
                Serial.print("Unknown encoding.");
        } else {
                const char* b = dec2binWzerofill(decimal, length);
                Serial.print("Decimal: ");
                Serial.print(decimal);
                Serial.print(" (");
                Serial.print( length );
                Serial.print("Bit) Binary: ");
                Serial.print( b );
                Serial.print(" Tri-State: ");
                Serial.print( bin2tristate( b) );
                Serial.print(" PulseLength: ");
                Serial.print(delay);
                Serial.print(" microseconds");
                Serial.print(" Protocol: ");
                Serial.println(protocol);
        }

        Serial.print("Raw data: ");
        for (unsigned int i=0; i<= length*2; i++) {
                Serial.print(raw[i]);
                Serial.print(",");
        }
        Serial.println();
        Serial.println();
}

static const char* bin2tristate(const char* bin) {
        static char returnValue[50];
        int pos = 0;
        int pos2 = 0;
        while (bin[pos]!='\0' && bin[pos+1]!='\0') {
                if (bin[pos]=='0' && bin[pos+1]=='0') {
                        returnValue[pos2] = '0';
                } else if (bin[pos]=='1' && bin[pos+1]=='1') {
                        returnValue[pos2] = '1';
                } else if (bin[pos]=='0' && bin[pos+1]=='1') {
                        returnValue[pos2] = 'F';
                } else {
                        return "not applicable";
                }
                pos = pos+2;
                pos2++;
        }
        returnValue[pos2] = '\0';
        return returnValue;
}

static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
        static char bin[64];
        unsigned int i=0;

        while (Dec > 0) {
                bin[32+i++] = ((Dec & 1) > 0) ? '1' : '0';
                Dec = Dec >> 1;
        }

        for (unsigned int j = 0; j< bitLength; j++) {
                if (j >= bitLength - i) {
                        bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
                } else {
                        bin[j] = '0';
                }
        }
        bin[bitLength] = '\0';

        return bin;
}

#endif
