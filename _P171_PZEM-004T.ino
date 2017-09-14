//#######################################################################################################
//################### Plugin 171 PZEM-004T AC Current and Voltage measurement sensor ####################
//#######################################################################################################
//
// This plugin is interfacing with PZEM-004T Sesor with softserial communication as the sensor
// has an UART pinout (TX/RX/VCC/GND)
//

//#ifdef PLUGIN_BUILD_TESTING

#include <SoftwareSerial.h>
#include <PZEM004T.h>
PZEM004T *Plugin_171_pzem;
IPAddress pzemIP(192,168,1,1);    // required by the library but not used (dummy value)

#define PLUGIN_171
#define PLUGIN_ID_171        171
#define PLUGIN_171_DEBUG     false   //activate extra log info in the debug
#define PLUGIN_NAME_171       "Voltage & Current (AC) - PZEM-004T [TESTING]"
#define PLUGIN_VALUENAME1_171 "Voltage (V)"
#define PLUGIN_VALUENAME2_171 "Current (A)"
#define PLUGIN_VALUENAME3_171 "Power (W)"
#define PLUGIN_VALUENAME4_171 "Energy (Wh)"

// local parameter for this plugin
#define PZEM_MAX_ATTEMPT      3

boolean Plugin_171(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_171;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
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
        string = F(PLUGIN_NAME_171);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_171));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_171));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_171));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_171));
        break;
      }
      
    case PLUGIN_WEBFORM_LOAD:
      {
        addFormNote(string, F("SoftSerial: 1st=RX-Pin, 2nd=TX-Pin"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (PLUGIN_171_DEBUG) {
          String log = F("PZEM004T: Reading started.");
          addLog(LOG_LEVEL_INFO, log);
        }
    		float pzVoltage = Plugin171_ReadVoltage();
    		float pzCurrent = Plugin171_ReadCurrent();
    		float pzPower   = Plugin171_ReadPower();
    		float pzEnergy  = Plugin171_ReadEnergy();
        //-------------------------------------------------------------------
        // readings can be ZERO if there's no AC input on the module.
        // in this case V A and W are reported correctly as ZERO but
        // the accumulated Energy paramenter will not be saved so to
        // preserve previous value
        //-------------------------------------------------------------------
        UserVar[event->BaseVarIndex]     = pzVoltage;
        UserVar[event->BaseVarIndex + 1] = pzCurrent;
        UserVar[event->BaseVarIndex + 2] = pzPower;
        if (pzEnergy>=0)  UserVar[event->BaseVarIndex + 3] = pzEnergy;
        if (PLUGIN_171_DEBUG) {
          String log = F("PZEM004T: Reading completed.");
          addLog(LOG_LEVEL_INFO, log);
        }
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!Plugin_171_pzem)
        {
          int pzemRXpin = Settings.TaskDevicePin1[event->TaskIndex];
          int pzemTXpin = Settings.TaskDevicePin2[event->TaskIndex];
          Plugin_171_pzem = new PZEM004T(pzemRXpin, pzemTXpin);
          if (PLUGIN_171_DEBUG) {
            String log = F("PZEM004T: Object Initialized");
            log += F(" - RX-Pin="); log += pzemRXpin; 
            log += F(" - TX-Pin="); log += pzemTXpin; 
            addLog(LOG_LEVEL_INFO, log);
          }
		      Plugin_171_pzem->setAddress(pzemIP);  // This initializes the PZEM004T library using a (useless) fake IP address
          if (PLUGIN_171_DEBUG) {
            String log = F("PZEM004T: setup address (dummy)");
            log += F(" - "); log += pzemIP;
            addLog(LOG_LEVEL_INFO, log);
          }
        }
        success = true;
        break;
      }

  }
  return success;
}

//************************************//
//***** reading values functions *****//
//************************************//

// NOTE: readings are attempted only PZEM_AMX_ATTEMPT times

float Plugin171_ReadVoltage() {
  int counter = 0;
	float reading = -1.0;
	do {
		reading = Plugin_171_pzem->voltage(pzemIP);
		wdt_reset();
		counter++;
	} while (counter < PZEM_MAX_ATTEMPT && reading < 0.0);
  if (reading == -1) reading = 0;
	return reading;
}

float Plugin171_ReadCurrent() {
	int counter = 0;
	float reading = -1.0;
	do {
		reading = Plugin_171_pzem->current(pzemIP);
		wdt_reset();
		counter++;
	} while (counter < PZEM_MAX_ATTEMPT && reading < 0.0);
  if (reading == -1) reading = 0;
  return reading;
}

float Plugin171_ReadPower() {
  int counter = 0;
	float reading = -1.0;
	do {
		reading = Plugin_171_pzem->power(pzemIP);
		wdt_reset();
		counter++;
	} while (counter < PZEM_MAX_ATTEMPT && reading < 0.0);
  if (reading == -1) reading = 0;
  return reading;
}

float Plugin171_ReadEnergy() {
	int counter = 0;
	float reading = -1.0;
	do {
		reading = Plugin_171_pzem->energy(pzemIP);
		wdt_reset();
		counter++;
	} while (counter < PZEM_MAX_ATTEMPT && reading < 0.0);
	return reading;
}
//#endif
