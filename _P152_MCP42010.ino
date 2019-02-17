//#######################################################################################################
//#################################### Plugin 152: MCP42010 #############################################
//###################################### GPIO 12 / 14 / 15 ##############################################
//#######################################################################################################
// written by antibill

// List of commands:
// (1) VAL1,<chambre 0-255>
// (2) VAL2,<second 0-255>

// Usage:
// (1): Set value to potentiometer (http://xx.xx.xx.xx/control?cmd=chambre,255)


#include <MCP42010.h>

static float Plugin_152_PotDest[2] = {0,0};

#define PLUGIN_152
#define PLUGIN_ID_152         152
#define PLUGIN_NAME_152       "MCP42010"
#define PLUGIN_VALUENAME1_152 "P1"
#define PLUGIN_VALUENAME2_152 "P2"

int Plugin_152_pin[3] = {-1,-1,-1};



boolean Plugin_152(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_152;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;           // SPI pins for ESP8266 are CS=15, CLK=14, MOSI=13
        Device[deviceCount].Ports = 0;
        Device[deviceCount].VType = SENSOR_TYPE_DUAL;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_152);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_152));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_152));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
      //  char tmpString[128];

        addHtml(F("<TR><TD>GPIO:<TD>"));

        addHtml(F("<TR><TD>1st GPIO (CS):<TD>"));
        addPinSelect(false, "taskdevicepin1", Plugin_152_pin[0] = Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addHtml(F("<TR><TD>2nd GPIO (CLK):<TD>"));
        addPinSelect(false, "taskdevicepin2", Plugin_152_pin[1] = Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        addHtml(F("<TR><TD>3rd GPIO (MOSI):<TD>"));
        addPinSelect(false, "taskdevicepin3", Plugin_152_pin[2] = Settings.TaskDevicePluginConfig[event->TaskIndex][2]);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {


        String plugin2 = WebServer.arg("taskdevicepin1");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin2.toInt();
        String plugin3 = WebServer.arg("taskdevicepin2");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin3.toInt();
        String plugin4 = WebServer.arg("taskdevicepin3");
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin4.toInt();

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        int pCS = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int pCLK = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        int pMOSI = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        Plugin_152_pin[0] = pCS;
        Plugin_152_pin[1] = pCLK;
        Plugin_152_pin[2] = pMOSI;

        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
          MCP42010 digipot(Plugin_152_pin[0], Plugin_152_pin[1], Plugin_152_pin[2]);


        if (command == F("chambre"))
        {
          int val1;
          val1 = event->Par1;   //Pot1
          Plugin_152_PotDest[0] = val1;
          digipot.setPot(1,val1);
          success = true;
        }

        if (command == F("second"))
        {
          int val2;
          val2 = event->Par1;   //Pot2
          Plugin_152_PotDest[1] = val2;
          digipot.setPot(2,val2);
          success = true;
        }




        break;
      }

    case PLUGIN_READ:
      {

        UserVar[event->BaseVarIndex + 0]= Plugin_152_PotDest[0] ;
        UserVar[event->BaseVarIndex + 1] = Plugin_152_PotDest[1] ;
        success = true;
        break;
      }


  }
  return success;
}
