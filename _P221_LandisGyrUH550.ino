#define USES_P221
#ifdef USES_P221
//#######################################################################################################
//############################# Plugin 218: read Landis & Gyr UH550 #####################################
//#######################################################################################################

//IR RX/TX sensor based on https://wiki.volkszaehler.org/hardware/controllers/ir-schreib-lesekopf

//Device pin 1 = RX
//Device pin 2 = TX

#include <ESPeasySoftwareSerial.h>
#define PLUGIN_221
#define PLUGIN_ID_221 221
#define PLUGIN_NAME_221 "Communication - Landis & Gyr UH550 [TESTING]"
#define PLUGIN_VALUENAME1_221 "Heat"
#define PLUGIN_221_VARIABLECOUNT 1

#define USERVARSMAX 8

// Search filters in received data.
// NOTE: This data remains in Flash!
const char initFilter[] PROGMEM = {"/LUGCUH50"};
const char qh_Filter[] PROGMEM = {"6.8("};                // 6.8 - Quantity of Thermal Energy (GJ)
const char vol_Filter[] PROGMEM = {"6.26("};              // 6.26 - Volume (m3)
const char qhPrev_Filter[] PROGMEM = {"6.8*01("};         // 6.8*01 - Quantity of Thermal Energy previous year (GJ)
const char volPrev_Filter[] PROGMEM = {"6.26*01("};       // 6.26*01 - Volume previous year (m3)
const char poMax_Filter[] PROGMEM = {"6.6("};             // 6.6 - Power Max (kW)
const char inOutFlowMaxTemp_Filter[] PROGMEM = {"9.4("};  // 9.4 - max inflow & max reflow temperature (C°)
const char opHours_Filter[] PROGMEM = {"6.31("};          // 6.31 - Operating Hours (hour)
const char* const filters[] PROGMEM = { qh_Filter, vol_Filter, inOutFlowMaxTemp_Filter, qhPrev_Filter, volPrev_Filter, poMax_Filter, opHours_Filter }; 
const byte numberFilters = sizeof(filters) / sizeof(filters[0]);

boolean Plugin_221_init = false;
byte PIN_SER_RX = 0;
byte PIN_SER_TX = 0;

struct ValueUnitPair
{
  char value[10];
  char unit[10];
};

struct DataSet
{
  char address[10];
  byte vupCount;
  struct ValueUnitPair vup[2];
};

struct DataSet datasets[USERVARSMAX];
byte indexDS = 0;

bool newDataAvailable[TASKS_MAX];

boolean Plugin_221(byte function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_221;
      Device[deviceCount].Type = DEVICE_TYPE_DUAL;
      Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = true;
      Device[deviceCount].ValueCount = 1;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = true;
      Device[deviceCount].GlobalSyncOption = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_221);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_221));
      break;
    }

    case PLUGIN_INIT:
    {
      for (byte i = 0; i < TASKS_MAX; i++)
      {
        newDataAvailable[i] = false;
      }
      Plugin_221_init = true;

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      byte valueSet = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

      String optionsValueSet[8] = {  F("Quantity of Thermal Energy"),
                                      F("Volume"),
                                      F("Thermal Energy previous Year"),
                                      F("Volume previous Year"),
                                      F("maxPower"),
                                      F("maxInflowTemp"),
                                      F("maxReflowTemp"),
                                      F("operatingHours") };
      addFormSelector(F("Valueset"), F("plugin_221_query"), 8, optionsValueSet, NULL, valueSet );

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_221"));
      Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_221_query"));

      Plugin_221_init = false; // Force device setup next time
      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      // read data from smartmeter only in first task and plugin initialized
      if (Plugin_221_init && (Settings.TaskDevicePluginConfig[event->TaskIndex][1] == 0))
      { 
        // String log = F("LUGUH: start read cycle: task <nr> : ");
        // log += event->TaskIndex;
        // log += " : "; 
        // log += getTimeString(':', true);
        // addLog(LOG_LEVEL_INFO, log);

        for (byte i = 0; i < TASKS_MAX; i++)
        {
          newDataAvailable[i] = false;
        }

        PIN_SER_RX = Settings.TaskDevicePin1[event->TaskIndex];
        PIN_SER_TX = Settings.TaskDevicePin2[event->TaskIndex];

        ESPeasySoftwareSerial softSerial(PIN_SER_RX, PIN_SER_TX, false); // Initialize serial
        char *index = 0;

        pinMode(PIN_SER_RX, INPUT);
        pinMode(PIN_SER_TX, OUTPUT);

        //read Landis&Gyr
        // 40 NUL chars + "/?!" + CR LF in 7 bit and EVEN parity
        byte initstr[] = { 0xAF, 0x3F, 0x21, 0x8D, 0x0A };

        byte r = 0;
        byte to = 0;
        byte bufpos;
        char message[80];
        int parityerrors;
        boolean lineComplete = false; // whether line is complete
        boolean initComplete = false; // whether initialization is complete and identification is correct

        softSerial.flush();
        softSerial.begin(300);

        for (byte i = 0; i<40; i++)
        {
          softSerial.write((byte)0x00);  
        }
        softSerial.write(initstr, sizeof(initstr));

        initDataSet();

        to = 0;
        r = 0;
        bufpos = 0;
        parityerrors = 0;

        while (r != 0x21) // telegram ends with "!"
        {
          if (softSerial.available())
          {
            // receive byte
            r = softSerial.read();
            if (parityCheck(r))
            {
              parityerrors += 1;
            }
            r = r & 127; // Mask MSB to remove parity
            if (r != 0x02)
            {
              message[bufpos++] = char(r);
            }
            if (message[bufpos - 1] == '\n') // newline found
            {
              message[bufpos - 1] = 0;
              if (bufpos > 2) 
              {
                if (message[bufpos - 2] == '\r') // CR found
                {
                  message[bufpos - 2] = 0;
                }
                lineComplete = true;
              }
              else // nur leere Zeile
              {
                message[0] = 0;
                bufpos = 0;
              }
            }
            to = 0;
          }
          else
          {
            to++;
            delay(25);
          }

          if (to > 100)
          {
            message[bufpos] = 0;
            String log = F("LUGUH: ERR(TIMEOUT):");
            addLog(LOG_LEVEL_INFO, log);
            break;
          }

          if (lineComplete && !initComplete)
          {
            index = strstr_P(message, initFilter); // search /LUGCUH50
            if (index != 0)                        // found /LUGCUH50
            {
              // change baud rate after initialization
              String log = F("LUGUH: ");
              log += F("meter identifier received: /LUGCUH50");
              softSerial.begin(2400);
              initComplete = true;
              lineComplete = false;
              bufpos = 0;
              addLog(LOG_LEVEL_INFO, log);
            }
          }

          if (lineComplete && initComplete)
          {
            if (parityerrors == 0)
            {
              // split line of message in OBIS-key(Value*Unit) parts
              message[bufpos] = 0;

              // line split
              char* token;
              // char keyValueUnit[80];
              // char keyValueUnitCpy[80];
              char filter[15];
              byte keyValueUnitArrIndex = 0;
              char keyValueUnitArr[6][70];

              token = strtok(message, ")");
              while (token != NULL)
              {
                strcpy(keyValueUnitArr[keyValueUnitArrIndex++], token);
                token = strtok(NULL, ")");
              }
              
              for (byte i = 0; i < keyValueUnitArrIndex; i++)
              {
                for (byte k = 0; k < numberFilters; k++)
                {
                  
                  strcpy_P(filter, (char*)pgm_read_ptr(&(filters[k])));
                  // strcpy(keyValueUnitCpy, keyValueUnit);
                  if (strstr(keyValueUnitArr[i], filter) != 0)
                  {
                    char *token;
                    char valueUnit[80];
                    // char valueUnitCpy[80];

                    token = strtok(keyValueUnitArr[i], "(");
                    strcpy(datasets[indexDS].address, token);
                    datasets[indexDS].vupCount = 0;

                    token = strtok(NULL, "(");
                    strcpy(valueUnit, token);
                    // strcpy(valueUnitCpy, token);
                    
                    char *end_str;
                    token = strtok_r(valueUnit, "&", &end_str);
                    while (token != NULL)
                    {
                      char *end_token;
                      char *token2 = strtok_r(token, "*", &end_token);
                      while (token2 != NULL)
                      {
                        byte vupIndex = datasets[indexDS].vupCount;
                        strcpy(datasets[indexDS].vup[vupIndex].value, token);
                        datasets[indexDS].vupCount++;
                        token2 = strtok_r(NULL, "*", &end_token);
                        if (token2 != NULL)
                        {
                          strcpy(datasets[indexDS].vup[vupIndex].unit, token2);
                        }
                        token2 = strtok_r(NULL, "*", &end_token);
                      }
                      token = strtok_r(NULL, "&", &end_str);
                    }
                    indexDS++;
                  }
                }
              }
              bufpos = 0;
              message[bufpos] = 0;
            }  // if (parityerrors == 0)
            else
            {
              message[bufpos] = 0;
              String log = F("LUGUH: ERR(PARITY):");
              log += message;
              addLog(LOG_LEVEL_ERROR, log);
              break;
            }
            lineComplete = false;
            bufpos = 0;
          }  // if (lineComplete && initComplete)
        } // while r != 0x21
        
        for (byte i = 0; i<indexDS; i++)
        {
          String log = F("LUGUH: <DataSet> OBIS: ");
          log += datasets[i].address;
          for (byte j = 0; j < datasets[i].vupCount; j++)
          {
            log += F(" value: ");
            log += datasets[i].vup[j].value;

            if (datasets[i].vup[j].unit != NULL)
            {
              log += F(" unit: ");
              log += datasets[i].vup[j].unit;
            }
          }
          addLog(LOG_LEVEL_INFO, log);
        }

        // inform other task about new data ..
        for (byte i = 0; i < TASKS_MAX; i++)
        {
          newDataAvailable[i] = true;
        }

        lineComplete = false;
        initComplete = false;
      } // if (Plugin_221_init)
      success = true;
      break;
    } // case PLUGIN_READ

    case PLUGIN_ONCE_A_SECOND:
    {
      //code to be executed once a second. Tasks which do not require fast response can be added here
      if (newDataAvailable[event->TaskIndex] && Plugin_221_init)
      {
        String log = F("LUGUH: update uservars for task: ");
        log += event->TaskIndex;
        switch(Settings.TaskDevicePluginConfig[event->TaskIndex][1])
        {         
          case 0:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.8", 1, indexDS);
            break;
          }
          case 1:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.26", 1, indexDS);
            break;
          }
          case 2:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.8*01", 1, indexDS);
            break;
          }
          case 3:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.26*01", 1, indexDS);
            break;
          }
          case 4:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.6", 1, indexDS);
            break;
          }
          case 5:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("9.4", 1, indexDS);
            break;
          }
          case 6:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("9.4", 2, indexDS);
            break;
          }
          case 7:
          {
            UserVar[event->BaseVarIndex] = readFromDataSet("6.31", 1, indexDS);
            break;
          }
        }
        newDataAvailable[event->TaskIndex] = false;
        log += F(" : ");
        log += UserVar[event->BaseVarIndex];
        addLog(LOG_LEVEL_INFO, log);
      }
      success = true;
      break;
    }
  }
  return success;
}

bool parityCheck(unsigned input)
{
  bool inputparity = input & 128;
  int x = input & 127;

  int parity = 0;
  while (x != 0)
  {
    parity ^= x;
    x >>= 1;
  }

  if ((parity & 0x1) != inputparity)
    return (1);
  else
    return (0);
}

void initDataSet()
{
  indexDS = 0;
  for(byte i = 0; i < USERVARSMAX; i++)
  {
    datasets[i].address[0] = '\0';
    datasets[i].vupCount = 0;
    for (byte j = 0; j < 2; j++)
    {
      datasets[i].vup[j].value[0] = '\0';
      datasets[i].vup[j].unit[0] = '\0';
    }
  }
}

float readFromDataSet(const char OBIS[], byte valueToRead, byte dataSetSize)
{
  for (byte i = 0; i<dataSetSize; i++)
  {
    if (strcmp(datasets[i].address, OBIS) == 0)
    {
      if (datasets[i].vupCount < valueToRead)
      {
        valueToRead = datasets[i].vupCount;
      }
      return atof(datasets[i].vup[valueToRead-1].value);
    } 
  }
  return 0.;
}


#endif // USES_P221