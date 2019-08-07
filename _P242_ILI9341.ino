#ifdef USES_P242
//#######################################################################################################
//#################################### Plugin 242: ILI9341 TFT 2.4inches display #################################
//#######################################################################################################

#define PLUGIN_242
#define PLUGIN_ID_242         242
#define PLUGIN_NAME_242       "Display - TFT 2.4 inches ILI9341/XPT2046"
#define PLUGIN_VALUENAME1_242 "TFT"
#define PLUGIN_242_MAX_DISPLAY 1

/* 
README

	This plugin allow to control a TFT screen (ILI9341) through HTTP API
	Tested with WEMOS D1 Mini Pro and Wemos TDFT 2.4
	Tested with ESPEasy 2.4.2  -tag mega-201902225)

	Plugin lib_deps = Adafruit GFX, Adafruit ILI9341

*/

/*

API Documentation

	This plugin is controlled by HTTP API
	
	http://<espeasy_ip>/control?cmd=tft,0,0,HelloWorld

	TFT,<row>,<col>,<size=1>,<foreColor=white>,<backColor=black>,<text>	-	Write text messages to TFT screen, ROW for row, COL for starting column, and Text for text.
	
	TFTCMD,<value=on|off|clear>,<clearColor=white>


Example:
	Write Text :
		http://<espeasy_ip>/control?cmd=tft,0,0,HelloWorld
	
	Write Text another place:
		http://<espeasy_ip>/control?cmd=tft,100,40,HelloWorld

	Write bigger Text :
		http://<espeasy_ip>/control?cmd=tft,0,0,3,HelloWorld

	Write RED Text :
		http://<espeasy_ip>/control?cmd=tft,0,0,3,HelloWorld

	Write RED Text (size is 1):
		http://<espeasy_ip>/control?cmd=tft,0,0,1,RED,HelloWorld

	Write RED Text on YELLOW background (size is 1):
		http://<espeasy_ip>/control?cmd=tft,0,0,1,RED,YELLOW,HelloWorld
		
	Switch display ON
		http://<espeasy_ip>/control?cmd=tftcmd,on

	Switch display OFF
		http://<espeasy_ip>/control?cmd=tftcmd,off

	Clear whole display
		http://<espeasy_ip>/control?cmd=tftcmd,clear

	Clear GREEN whole display
		http://<espeasy_ip>/control?cmd=tftcmd,clear,green
*/



//plugin dependency
#include <Adafruit_ILI9341.h>

//declare functions for using default value parameters
void Plugin_242_printText(const char *string, int X, int Y, unsigned int textSize = 1, unsigned short color = ILI9341_WHITE, unsigned short bkcolor = ILI9341_BLACK);
void Plugin_242_clear_display( unsigned short bkcolor = ILI9341_WHITE);

//The setting structure
struct Plugin_242_TFT_SettingStruct
{
  Plugin_242_TFT_SettingStruct()
  : address_tft_cs(D0), address_tft_dc(D8), address_tft_rst(-1), address_ts_cs(D3), rotation(0){}
  byte address_tft_cs;
  byte address_tft_dc;
  byte address_tft_rst;
  byte address_ts_cs;
  byte rotation;
} TFT_Settings;

//The display pointer
Adafruit_ILI9341 *tft = NULL;

boolean Plugin_242(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_242;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_NONE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_242);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_242));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        {
          addFormPinSelect(F("TFT CS"), F("p242_tft_cs"), PCONFIG(0));
          addFormPinSelect(F("TFT DC"), F("p242_tft_dc"), PCONFIG(1));
          addFormPinSelect(F("TFT RST"), F("p242_tft_rst"), PCONFIG(2));
          addFormPinSelect(F("TS CS"), F("ts_cs"), PCONFIG(3));
        }
        {
          byte choice2 = PCONFIG(4);
          String options2[4] = { F("Normal"), F("+90°"), F("+180°"), F("+270°") };
          int optionValues2[4] = { 0, 1, 2, 3 };
          addFormSelector(F("Rotation"), F("p242_rotate"), 4, options2, optionValues2, choice2);
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p242_tft_cs"));
        PCONFIG(1) = getFormItemInt(F("p242_tft_dc"));
        PCONFIG(2) = getFormItemInt(F("p242_tft_rst"));
        PCONFIG(3) = getFormItemInt(F("p242_ts_cs"));
        PCONFIG(4) = getFormItemInt(F("p242_rotate"));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {

        TFT_Settings.address_tft_cs = PCONFIG(0);
        TFT_Settings.address_tft_dc = PCONFIG(1);
        TFT_Settings.address_tft_rst = PCONFIG(2);
        TFT_Settings.address_ts_cs = PCONFIG(3);
        TFT_Settings.rotation = PCONFIG(4);

        tft = new Adafruit_ILI9341(TFT_Settings.address_tft_cs, TFT_Settings.address_tft_dc, TFT_Settings.address_tft_rst);
        tft->begin();

        Plugin_242_clear_display();
        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        success = false;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        success = false;
        break;
      }

    case PLUGIN_READ:
      {
        success = false;
        break;
      }

    case PLUGIN_WRITE:
      {
        String arguments = String(string);

        int dotPos = arguments.indexOf('.');
        if(dotPos > -1 && arguments.substring(dotPos,dotPos+3).equalsIgnoreCase(F("tft")))
        {
          LoadTaskSettings(event->TaskIndex);
          String name = arguments.substring(0,dotPos);
          name.replace("[","");
          name.replace("]","");
          if(name.equalsIgnoreCase(getTaskDeviceName(event->TaskIndex)) == true)
          {
            arguments = arguments.substring(dotPos+1);
          }
          else
          {
             return false;
          }
        }


        int argIndex = arguments.indexOf(',');
        if (argIndex)
          arguments = arguments.substring(0, argIndex);
        if (arguments.equalsIgnoreCase(F("TFTCMD")))
        {
          success = true;
          arguments = string.substring(argIndex + 1);
          String sParams[2];
          int argCount = Plugin_242_StringSplit(arguments, ',', sParams, 2);
          if(argCount>0)
          {
            if (sParams[0].equalsIgnoreCase(F("Off")))
            {
              Plugin_242_displayOff();
            }
            else if (sParams[0].equalsIgnoreCase(F("On")))
            {
              Plugin_242_displayOn();
            }
            else if (sParams[0].equalsIgnoreCase(F("Clear")))
            {
              if(argCount ==2)
              {
                Plugin_242_clear_display(Plugin_242_ParseColor(sParams[1]));
              }
              else
              {
                Plugin_242_clear_display();
              }
            }
          }
        }
        else if (arguments.equalsIgnoreCase(F("TFT")))
        {
          success = true;
          arguments = string.substring(argIndex + 1);
          String sParams[6];
          int argCount = Plugin_242_StringSplit(arguments, ',', sParams, 6);

          switch (argCount)
          {
          case 3: //single text
            Plugin_242_printText(sParams[2].c_str(), atoi(sParams[0].c_str()) - 1, atoi(sParams[1].c_str()) - 1);  
            break;

          case 4: //text + size
            Plugin_242_printText(sParams[3].c_str(), atoi(sParams[0].c_str()) - 1, atoi(sParams[1].c_str()) - 1, atoi(sParams[2].c_str()));  
            break;

          case 5: //text + size + color
            Plugin_242_printText(sParams[4].c_str(), atoi(sParams[0].c_str()) - 1, atoi(sParams[1].c_str()) - 1, atoi(sParams[2].c_str()), Plugin_242_ParseColor(sParams[3]));  
            break;
          
          case 6: //text + size + color
            Plugin_242_printText(sParams[5].c_str(), atoi(sParams[0].c_str()) - 1, atoi(sParams[1].c_str()) - 1, atoi(sParams[2].c_str()), Plugin_242_ParseColor(sParams[3]), Plugin_242_ParseColor(sParams[4]));  
            break;

          default:
            if (loglevelActiveFor(LOG_LEVEL_INFO)) {
              addLog(LOG_LEVEL_INFO, "Fail to parse command correctly; please check API documentation");
              String log  = "Parsed command = \"";
              log += string;
              log += "\"";
              addLog(LOG_LEVEL_INFO, log);
            }
            success = false;
            break;
          }
        }
        break;
      }
  }
  return success;
}

//Reset TFT display
void Plugin_242_reset_display()
{
  Plugin_242_clear_display();
}

//Start the display (dwotch on + clear)
void Plugin_242_StartUp_TFT()
{
  Plugin_242_displayOn();
  Plugin_242_clear_display();
}

//switch TFT display ON
void Plugin_242_displayOn()
{
  tft->sendCommand(ILI9341_DISPON);
}

//switch TFT display OFF
void Plugin_242_displayOff()
{
  tft->sendCommand(ILI9341_DISPOFF);
}

//erase the full screen
//param [in] bkcolor : the color for filling screen
void Plugin_242_clear_display( unsigned short bkcolor)
{
  tft->fillScreen(bkcolor);
}

//Print some text
//param [in] string : The text to display
//param [in] X : The left position (X)
//param [in] Y : The top position (Y)
//param [in] textSize : The text size (default 1)
//param [in] color : The fore color (default ILI9341_WHITE)
//param [in] bkcolor : The background color (default ILI9341_BLACK)
void Plugin_242_printText(const char *string, int X, int Y, unsigned int textSize, unsigned short color, unsigned short bkcolor)
{
  tft->setCursor(X, Y);
  tft->setTextColor(color, bkcolor);
  tft->setTextSize(textSize);
  tft->println(string);
}

//Parse color string to ILI9341 color
//param [in] s : The color string (white, red, ...)
//return : color (default ILI9341_WHITE)
unsigned short Plugin_242_ParseColor(String s)
{
  if (s.equalsIgnoreCase("BLACK"))
    return ILI9341_BLACK;
  if (s.equalsIgnoreCase("NAVY"))
    return ILI9341_NAVY;
  if (s.equalsIgnoreCase("DARKGREEN"))
    return ILI9341_DARKGREEN;
  if (s.equalsIgnoreCase("DARKCYAN"))
    return ILI9341_DARKCYAN;
  if (s.equalsIgnoreCase("MAROON"))
    return ILI9341_MAROON;
  if (s.equalsIgnoreCase("PURPLE"))
    return ILI9341_PURPLE;
  if (s.equalsIgnoreCase("OLIVE"))
    return ILI9341_OLIVE;
  if (s.equalsIgnoreCase("LIGHTGREY"))
    return ILI9341_LIGHTGREY;
  if (s.equalsIgnoreCase("DARKGREY"))
    return ILI9341_DARKGREY;
  if (s.equalsIgnoreCase("BLUE"))
    return ILI9341_BLUE;
  if (s.equalsIgnoreCase("GREEN"))
    return ILI9341_GREEN;
  if (s.equalsIgnoreCase("CYAN"))
    return ILI9341_CYAN;
  if (s.equalsIgnoreCase("RED"))
    return ILI9341_RED;
  if (s.equalsIgnoreCase("MAGENTA"))
    return ILI9341_MAGENTA;
  if (s.equalsIgnoreCase("YELLOW"))
    return ILI9341_YELLOW;
  if (s.equalsIgnoreCase("WHITE"))
    return ILI9341_WHITE;
  if (s.equalsIgnoreCase("ORANGE"))
    return ILI9341_ORANGE;
  if (s.equalsIgnoreCase("GREENYELLOW"))
    return ILI9341_GREENYELLOW;
  if (s.equalsIgnoreCase("PINK"))
    return ILI9341_PINK;
  return ILI9341_WHITE;
}

//Split a string by delimiter
//param [in] s : The input string
//param [in] c : The delimiter
//param [out] op : The resulting string array
//param [in] limit : The maximum strings to find
//return : The string count
int Plugin_242_StringSplit(String s, char c, String op[], int limit)
{
  int count = 0;
  char * pch;
  String d = String(c);
  pch = strtok ((char*)(s.c_str()),d.c_str());
  while (pch != NULL && count < limit)
  {
    op[count] = String(pch);
    count++;
    pch = strtok (NULL, ",");
  }  
  return count;
}


#endif // USES_P242
