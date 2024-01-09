#include "_Plugin_Helper.h"
#ifdef USES_P160

//#######################################################################################################
//#################################### Plugin 160: Input RF #############################################
//#######################################################################################################
/*
    Version: 3.0
    Description: use this script to recieve RF with a cheap MX-05V alike receiver
    Author: S4nder
    Updated by: https://github.com/mgx0 <- updated for ESPEasy ~20231225
    Copyright: (c) 2015-2016 Sander Pleijers (s4nder)
    License: MIT
    License URI: http://en.wikipedia.org/wiki/MIT_License
    Status : "Proof of concept"
    This program was developed independently and it is not supported in any way.
 */


// Library: https://github.com/sui77/rc-switch
#include <RCSwitch.h>
RCSwitch *rfReceiver;

#define PLUGIN_160
#define PLUGIN_ID_160         160
#define PLUGIN_NAME_160       "RF Receiver - MX-05V alike receiver"
#define PLUGIN_ValueNAME1_160 "RF"

#ifndef USES_P016
    int irReceiver = 0; // make sure it has value even if plugin not found
#endif

boolean Plugin_160(byte function, struct EventStruct *event, String& string)
{
    boolean success = false;

    switch (function)
    {
        case PLUGIN_DEVICE_ADD:
        {
                Device[++deviceCount].Number = PLUGIN_ID_160;
                Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
                Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_ULONG;
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
            string = F(PLUGIN_NAME_160);
            break;
        }

        case PLUGIN_GET_DEVICEVALUENAMES:
        {
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_ValueNAME1_160));
            break;
        }

        case PLUGIN_INIT:
        {
            int rfPin = Settings.TaskDevicePin1[event->TaskIndex];
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
                    // *** temp woraround, ESP Easy framework does not currently prepare this...
                    // taken from _P040
                    taskIndex_t index = INVALID_TASK_INDEX;
                    constexpr pluginID_t PLUGIN_ID_P160_RF(PLUGIN_ID_160);
                    for (taskIndex_t y = 0; y < TASKS_MAX; y++){
                        if (Settings.getPluginID_for_task(y) == PLUGIN_ID_P160_RF){
                            index = y;
                        }
                    }

                    const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(index);
                    if (!validDeviceIndex(DeviceIndex)) {
                        break;
                    }

                    event->setTaskIndex(index);
                    if (!validUserVarIndex(event->BaseVarIndex)) {
                        break;
                    }

                    checkDeviceVTypeForTask(event);
                    // *** end of workaround

                    // fill the output data
                    UserVar.setSensorTypeLong(event->TaskIndex, valuerf);

                    // throw some debug info to serial
                    serial_debug_out(rfReceiver->getReceivedValue(), rfReceiver->getReceivedBitlength(), rfReceiver->getReceivedDelay(), rfReceiver->getReceivedRawdata(), rfReceiver->getReceivedProtocol());
                    String log = F("RF Code Recieved: ");
                    log += String(valuerf);
                    addLog(LOG_LEVEL_INFO, log);

                    // emit event
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

void serial_debug_out(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol) {
    if (decimal == 0) {
        Serial.print("Unknown encoding.");
    } else {
        const char* b = dec2binWzerofill(decimal, length);
        Serial.print("Decimal: ");
        Serial.print(decimal);
        Serial.print(" (");
        Serial.print(length );
        Serial.print("Bit)\nBinary: ");
        Serial.print(b);
        Serial.print("\nTri-State: ");
        Serial.print(bin2tristate( b) );
        Serial.print("\nPulseLength: ");
        Serial.print(delay);
        Serial.print(" microseconds");
        Serial.print("\nProtocol: ");
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
        bin[32 + i++] = ((Dec & 1) > 0) ? '1' : '0';
        Dec = Dec >> 1;
    }

    for (unsigned int j = 0; j< bitLength; j++) {
        if (j >= bitLength - i) {
            bin[j] = bin[31 + i - (j - (bitLength - i)) ];
        } else {
            bin[j] = '0';
        }
    }
    bin[bitLength] = '\0';

    return bin;
}

#endif