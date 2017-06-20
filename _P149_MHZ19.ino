/*
 *
 * This plug in is written by Dmitry (rel22 ___ inbox.ru)
 * Plugin is based upon SenseAir plugin by Daniel Tedenljung info__AT__tedenljungconsulting.com
 *
 * This plugin reads the CO2 and Temperateure values from MH-Z19 NDIR Sensor
 * DevicePin1 - is RX for ESP
 * DevicePin2 - is TX for ESP
 */


#define PLUGIN_149
#define PLUGIN_ID_149         149
#define PLUGIN_NAME_149       "CO2 Sensor - MH-Z19"
#define PLUGIN_VALUENAME1_149 "PPM"
#define PLUGIN_VALUENAME2_149 "Temperature"
#define PLUGIN_READ_TIMEOUT   3000

#include <SoftwareSerial.h>
SoftwareSerial * Plugin_149_S8;
unsigned long Plugin_149_start;
boolean Plugin_149_init = false;
const int Plugin_149_warmUpTime = 180000; // 3 minutes in ms

// 9-bytes CMD PPM read command
byte mhzCmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
byte mhzResp[9]; // 9 bytes bytes response

boolean Plugin_149(byte function, struct EventStruct * event, String& string)
{
    bool success = false;

    switch (function)
    {
        case PLUGIN_DEVICE_ADD:
        {
            Device[++deviceCount].Number           = PLUGIN_ID_149;
            Device[deviceCount].Type               = DEVICE_TYPE_DUAL;
            Device[deviceCount].VType              = SENSOR_TYPE_DUAL;
            Device[deviceCount].Ports              = 0;
            Device[deviceCount].PullUpOption       = false;
            Device[deviceCount].InverseLogicOption = false;
            Device[deviceCount].FormulaOption      = true;
            Device[deviceCount].ValueCount         = 2;
            Device[deviceCount].SendDataOption     = true;
            Device[deviceCount].TimerOption        = true;
            Device[deviceCount].GlobalSyncOption   = true;
            break;
        }

        case PLUGIN_GET_DEVICENAME:
        {
            string = F(PLUGIN_NAME_149);
            break;
        }

        case PLUGIN_GET_DEVICEVALUENAMES:
        {
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_149));
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_149));
            break;
        }

        case PLUGIN_WEBFORM_LOAD:
        {
            addFormNote(string, F("1st GPIO connects to sensor TX pin. 2nd GPIO connects to sensor RX pin."));
            success = true;
            break;
        }

        case PLUGIN_INIT:
        {
            if (Settings.TaskDevicePin1[event->TaskIndex] != -1 && Settings.TaskDevicePin2[event->TaskIndex] != -1)
            {
                Plugin_149_S8 = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex]);
                Plugin_149_S8->begin(9600);
                Plugin_149_start = millis();
                Plugin_149_init  = false; // force warmup period
                String log = F("MHZ19:  Init OK ");
                addLog(LOG_LEVEL_INFO, log);
            }

            success = true;
            break;
        }

        case PLUGIN_READ:
        {
            if (Plugin_149_init)
            {
                // send read PPM command
                int nbBytesSent = Plugin_149_S8->write(mhzCmd, 9);
                if (nbBytesSent != 9)
                {
                    String log = F("MHZ19: Error, nb bytes sent != 9 : ");
                    log += nbBytesSent;
                    addLog(LOG_LEVEL_INFO, log);
                }

                // get response
                memset(mhzResp, 0, 9);

                long start  = millis();
                int counter = 0;
                while (((millis() - start) < PLUGIN_READ_TIMEOUT) && (counter < 9))
                {
                    if (Plugin_149_S8->available() > 0)
                    {
                        mhzResp[counter++] = Plugin_149_S8->read();
                    }
                    else
                    {
                        delay(10);
                    }
                }

                if (counter < 9)
                {
                    String log = F("MHZ19: Error, timeout while trying to read");
                    addLog(LOG_LEVEL_INFO, log);
                }

                unsigned int ppm = 0;
                int temperature = 0;
                int i;
                byte crc = 0;
                for (i = 1; i < 8; i++) crc += mhzResp[i];

                crc = 255 - crc;
                crc++;

                if (!(mhzResp[0] == 0xFF && mhzResp[1] == 0x86 && mhzResp[8] == crc) )
                {
                    String log = F("MHZ19: Read error : CRC = ");
                    log += String(crc);
                    log += " / ";
                    log += String(mhzResp[8]);
                    log += " bytes read  => ";
                    for (i = 0; i < 9; i++)
                    {
                        log += mhzResp[i];
                        log += "/";
                    }

                    addLog(LOG_LEVEL_ERROR, log);

                    success = false;
                    break;
                }
                else
                {
                    // calculate CO2 PPM
                    unsigned int mhzRespHigh = (unsigned int) mhzResp[2];
                    unsigned int mhzRespLow  = (unsigned int) mhzResp[3];
                    ppm = (256 * mhzRespHigh) + mhzRespLow;
                    temperature = (unsigned int) mhzResp[4] - 40;
                }

                UserVar[event->BaseVarIndex] = (float) ppm;
                UserVar[event->BaseVarIndex + 1] = temperature;
                String log = F("MHZ19: PPM value: ");
                log += ppm;
                addLog(LOG_LEVEL_INFO, log);
                log  = F("MH-Z19: Temperature: ");
                log += temperature;
                addLog(LOG_LEVEL_INFO, log);
                success = true;
                break;
            }
            else if (millis() - Plugin_149_start >= Plugin_149_warmUpTime)
            {
                Plugin_149_init = true;
                String log = F("MH-Z19  : Warmup Complete");
                addLog(LOG_LEVEL_DEBUG, log);
            }
            else
            {
                // wait for warmup
                String log = F("MH-Z19  : warming up (seconds) : ");
                log += ((millis() - Plugin_149_start) / 1000);
                log += F(" of ");
                log += Plugin_149_warmUpTime/1000;
                addLog(LOG_LEVEL_DEBUG, log);
            }
            break;
        }
    }

    return success;
} // Plugin_149
