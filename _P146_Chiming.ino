//#######################################################################################################
//#################################### Plugin 146: Chiming Mechanism ####################################
//#######################################################################################################
// written by Jochen Krapf (jk@nerd2nerd.org)

// List of commands:
// (1) CHIME,<tokens>             Play token direct
// (2) CHIMESAVE,<name>,<tokens>  Save tokens with given name in FFS
// (3) CHIMEPLAY,<name>           Play saved tokens given name out of FFS

// List of tokens:
// (a) '1'...'9', 'A'...'F'       Bell number - 1=1st bell, 2=2nd bell, 4=3rd bell, 8=4th bell, numbers can be added to strike simultaniouly, F=all bells
// (b) '!'                        Double strike prev. token
// (c) '-' or ' '                 Normal Pause
// (d) '='                        Long Pause (3 times normal)
// (e) '.'                        Short Pause (1/3 of normal)
// (f) '|'                        Shortest Pause
// (g) '#'                        Comment - rest of the tokens will be ignored
// Note: If no pause is specified, a normal pause will be inserted "111" -> "1-1-1"

// Usage as Hourly Chime Clock:
// save twelve comma separated tokens with name "hours", enable checkbox "Hourly Chiming Clock Strike" in plugin web interface and enable NTP (advanced settings)
//
// examples:
// Historical coded with 1 bell:              "1,11,111,1111,11111,111111,1111111,11111111,111111111,1111111111,11111111111,111111111111"
// Binary coded with 2 bells (2nd bell=1):    "1112,1121,1122,1211,1212,1221,1222,2111,2112,2121,2122,2211"
// Binary coded with 1 bell (short pause=1):  "1_1_1_11,1_1_11_1,1_1_111,1_11_1_1,1_11_11,1_111_1,1_1111,11_1_1_1,11_1_11,11_11_1,11_111,111_1_1"
// Binary coded with 1 bell (double strike=1):"1111!,111!1,111!1!,11!11,11!11!,11!1!1,11!1!1!,1!111,1!111!,1!11!1,1!11!1!,1!1!11"
//
// CHIMESAVE,hours,1111!,111!1,111!1!,11!11,11!11!,11!1!1,11!1!1!,1!111,1!111!,1!11!1,1!11!1!,1!1!11

// Usage as Alarm Clock:
// save tokens with name "<HH><MM>" and enable NTP (advanced settings)
//
// examples:
// CHIMESAVE,0815,1111!           Dayly Alarm at 8:15am
// CHIMESAVE,2015,11121           Dayly Alarm at 8:15pm
// CHIMESAVE,2015                 Delete Alarm at 8:15pm


#ifdef PLUGIN_BUILD_TESTING

//#include <*.h>   - no external lib required

#define PLUGIN_146
#define PLUGIN_ID_146         146
#define PLUGIN_NAME_146       "Chiming Mechanism"

#define PLUGIN_146_FIFO_SIZE 64   // must be power of 2
#define PLUGIN_146_FIFO_MASK (PLUGIN_146_FIFO_SIZE-1)

class CPlugin_146_Data
{
public:
  long millisStateEnd;
  long millisChimeTime;
  long millisPauseTime;

  int pin[4];
  byte lowActive;
  byte chimeClock;

  char FIFO[PLUGIN_146_FIFO_SIZE];
  byte FIFO_IndexR;
  byte FIFO_IndexW;

  void Plugin_146_Data()
  {
    millisStateEnd = 0;
    millisChimeTime = 60;
    millisPauseTime = 400;

    for (byte i=0; i<4; i++)
      pin[i] = -1;
    lowActive = false;
    chimeClock = true;

    FIFO_IndexR = 0;
    FIFO_IndexW = 0;
  }
};

static CPlugin_146_Data* Plugin_146_Data = NULL;


boolean Plugin_146(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_146;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = true;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].TimerOptional = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_146);
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        //default values
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] <= 0)   //Plugin_146_millisChimeTime
          Settings.TaskDevicePluginConfig[event->TaskIndex][0] = 60;
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][1] <= 0)   //Plugin_146_millisPauseTime
          Settings.TaskDevicePluginConfig[event->TaskIndex][1] = 400;

        addFormPinSelect(string, F("4th GPIO"), F("TDP4"), (int)(Settings.TaskDevicePin[3][event->TaskIndex]));


        addFormSubHeader(string, F("Timing"));

        addFormNumericBox(string, F("Chiming/Strike Time (ct)"), F("chimetime"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addUnit(string, F("ms"));

        addFormNumericBox(string, F("Normal Pause Time (t)"), F("pausetime"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        addUnit(string, F("ms"));

        addFormNote(string, F("'1=1'&rArr;3t, '1-1' or '11'&rArr;1t, '1.1'&rArr;&#8531;t, '1|1'&rArr;&frac12;ct"));


        addFormSubHeader(string, F("Chiming Clock"));

        addFormCheckBox(string, F("Hourly Chiming Clock Strike"), F("chimeclock"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        //string += F("<TR><TD><TD>");
        string += F(" ");
        addButton(string, F("'control?cmd=chimeplay,hours'"), F("Test 1&hellip;12"));

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][2] && !Settings.UseNTP)
          addFormNote(string, F("Enable and configure NTP!"));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePin[3][event->TaskIndex] = (int8_t)getFormItemInt(F("TDP4"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("chimetime"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("pausetime"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = isFormItemChecked(F("chimeclock"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!Plugin_146_Data)
          Plugin_146_Data = new CPlugin_146_Data();

        Plugin_146_Data->lowActive = Settings.TaskDevicePin1Inversed[event->TaskIndex];
        Plugin_146_Data->millisChimeTime = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_146_Data->millisPauseTime = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        Plugin_146_Data->chimeClock = Settings.TaskDevicePluginConfig[event->TaskIndex][2];

        String log = F("Chime: GPIO: ");
        for (byte i=0; i<4; i++)
        {
          int pin = Settings.TaskDevicePin[i][event->TaskIndex];
          Plugin_146_Data->pin[i] = pin;
          if (pin >= 0)
          {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, Plugin_146_Data->lowActive);
          }
          log += pin;
          log += F(" ");
        }
        if (Plugin_146_Data->lowActive)
          log += F("!");
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        if (!Plugin_146_Data)
          break;

        String command = parseString(string, 1);

        if (command == F("chime"))
        {
          int paramPos = getParamStartPos(string, 2);
          String param = string.substring(paramPos);
          Plugin_146_AddStringFIFO(param);
          success = true;
        }
        if (command == F("chimeplay"))
        {
          String name = parseString(string, 2);
          String param;
          Plugin_146_ReadChime(name, param);
          Plugin_146_AddStringFIFO(param);
          success = true;
        }
        if (command == F("chimesave"))
        {
          String name = parseString(string, 2);
          int paramPos = getParamStartPos(string, 3);
          String param = string.substring(paramPos);
          Plugin_146_WriteChime(name, param);
          Plugin_146_AddStringFIFO(F("1"));
          success = true;
        }

        break;
      }

      case PLUGIN_CLOCK_IN:
        {
          if (!Plugin_146_Data)
            break;

          String tokens = "";
          byte hours = hour();
          byte minutes = minute();

          if (Plugin_146_Data->chimeClock)
          {
            char tmpString[8];

            sprintf_P(tmpString, PSTR("%02d%02d"), hours, minutes);
            if (Plugin_146_ReadChime(tmpString, tokens))
              Plugin_146_AddStringFIFO(tokens);

            if (minutes == 0)
            {
              if (Plugin_146_ReadChime("hours", tokens) == 0)
                tokens = F("1111!,111!1,111!1!,11!11,11!11!,11!1!1,11!1!1!,1!111,1!111!,1!11!1,1!11!1!,1!1!11");   //1..12

              // hours 0..23 -> 1..12
              hours = hours % 12;
              if (hours == 0)
                hours = 12;

              byte index = hours;
              //byte index = (minutes % 10);   //test only

              tokens = parseString(tokens, index);
              Plugin_146_AddStringFIFO(tokens);
            }
          }

          success = true;
          break;
        }


    case PLUGIN_FIFTY_PER_SECOND:
    //case PLUGIN_TEN_PER_SECOND:
      {
        if (!Plugin_146_Data)
          break;

        long millisAct = millis();

        if (Plugin_146_Data->millisStateEnd > 0)   // just striking?
        {
          if (Plugin_146_Data->millisStateEnd <= millisAct)   // end reached?
          {
            for (byte i=0; i<4; i++)
            {
              if (Plugin_146_Data->pin[i] >= 0)
                digitalWrite(Plugin_146_Data->pin[i], Plugin_146_Data->lowActive);
            }
            Plugin_146_Data->millisStateEnd = 0;
          }
        }

        if (Plugin_146_Data->millisStateEnd == 0)   // just finished?
        {
          if (! Plugin_146_IsEmptyFIFO())
          {
            char c = Plugin_146_ReadFIFO();

            String log = F("Chime: Process '");
            log += c;
            log += "'";
            addLog(LOG_LEVEL_DEBUG, log);

            switch (c)
            {
              case 'a':
              case 'b':
              case 'c':
              case 'd':
              case 'e':
              case 'f':
              case 'A':
              case 'B':
              case 'C':
              case 'D':
              case 'E':
              case 'F':
                c -= 'A' - '0' - 10;
                //vvv

              case '0':   //strikes 1=1st bell, 2=2nd bell, 4=3rd bell, 8=4rd bell
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
              {
                byte mask = 1;
                for (byte i=0; i<4; i++)
                {
                  if (Plugin_146_Data->pin[i] >= 0)
                    if (c & mask)
                      digitalWrite(Plugin_146_Data->pin[i], !Plugin_146_Data->lowActive);
                  mask <<= 1;
                }
                Plugin_146_Data->millisStateEnd = millisAct + Plugin_146_Data->millisChimeTime;
                break;
              }
              case '=':   //long pause
              case ' ':
              case ',':
                Plugin_146_Data->millisStateEnd = millisAct + Plugin_146_Data->millisPauseTime*3;
                break;
              case '-':   //single pause
                Plugin_146_Data->millisStateEnd = millisAct + Plugin_146_Data->millisPauseTime;
                break;
              case '.':   //short pause
                Plugin_146_Data->millisStateEnd = millisAct + Plugin_146_Data->millisPauseTime/3;
                break;
              case '|':   //shortest pause
                Plugin_146_Data->millisStateEnd = millisAct + Plugin_146_Data->millisChimeTime/2;
                break;
              case '#':   //comment -> eat till FIFO is empty
                while (Plugin_146_ReadFIFO());
                break;
              default:   //unknown char -> do nothing
                break;
            }
          }

        }
        success = true;
        break;
      }

  }
  return success;
}

// FIFO functions

void Plugin_146_WriteFIFO(char c)
{
  if (Plugin_146_Data->FIFO_IndexR == ((Plugin_146_Data->FIFO_IndexW+1) & PLUGIN_146_FIFO_MASK))   // FIFO full?
    return;

  Plugin_146_Data->FIFO[Plugin_146_Data->FIFO_IndexW] = c;
  Plugin_146_Data->FIFO_IndexW++;
  Plugin_146_Data->FIFO_IndexW &= PLUGIN_146_FIFO_MASK;
}

char Plugin_146_ReadFIFO()
{
  if (Plugin_146_IsEmptyFIFO())
    return '\0';

  char c = Plugin_146_Data->FIFO[Plugin_146_Data->FIFO_IndexR];
  Plugin_146_Data->FIFO_IndexR++;
  Plugin_146_Data->FIFO_IndexR &= PLUGIN_146_FIFO_MASK;

  return c;
}

char Plugin_146_PeekFIFO()
{
  if (Plugin_146_IsEmptyFIFO())
    return '\0';

  return Plugin_146_Data->FIFO[Plugin_146_Data->FIFO_IndexR];
}

boolean Plugin_146_IsEmptyFIFO()
{
  return (Plugin_146_Data->FIFO_IndexR == Plugin_146_Data->FIFO_IndexW);
}

void Plugin_146_AddStringFIFO(const String& param)
{
  if (param.length() == 0)
    return;

  byte i = 0;
  char c = param[i];
  char c_last = '\0';

  while (c != 0)
  {
    if (isDigit(c) && isDigit(c_last))   // "11" is shortcut for "1-1" -> add pause
      Plugin_146_WriteFIFO('-');
    if (c == '!')   //double strike -> add shortest pause and repeat last strike
    {
      Plugin_146_WriteFIFO('|');
      c = c_last;
    }
    Plugin_146_WriteFIFO(c);
    c_last = c;

    c = param[++i];
  }

  Plugin_146_WriteFIFO('=');
}

//File I/O functions

void Plugin_146_WriteChime(const String& name, const String& tokens)
{
  String fileName = F("chime_");
  fileName += name;
  fileName += F(".txt");

  String log = F("Chime: write ");
  log += fileName;
  log += F(" ");

  fs::File f = SPIFFS.open(fileName, "w");
  if (f)
  {
    f.print(tokens);
    f.close();
    //flashCount();
    log += tokens;
  }

  addLog(LOG_LEVEL_INFO, log);
}

byte Plugin_146_ReadChime(const String& name, String& tokens)
{
  String fileName = F("chime_");
  fileName += name;
  fileName += F(".txt");

  String log = F("Chime: read ");
  log += fileName;
  log += F(" ");

  tokens = "";
  fs::File f = SPIFFS.open(fileName, "r+");
  if (f)
  {
    char c;
    while (f.available())
    {
      c = f.read();
      tokens += c;
    }
    f.close();

    log += tokens;
  }

  addLog(LOG_LEVEL_INFO, log);

  return tokens.length();
}

#endif
