//#######################################################################################################
//#################################### Plugin 127: Teleinfo #############################################
//#################################### This plugin transmits data of energy counter to HTTP server  #####
//#################################### Compatible with jeedom plugin Teleinfo                       #####
//#######################################################################################################

#define PLUGIN_127
#define PLUGIN_ID_127         127
#define PLUGIN_NAME_127       "Teleinfo"


#define P127_BUFFER_SIZE 128
boolean Plugin_127_init = false;
int P127_CYCLE = 0;
String P127_SendData = "";
String P127_URL = "";
int P127_PORT = 80;
String P127_HOST = "";

boolean Plugin_127(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte connectionState = 0;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_127;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].Custom = true;
        Device[deviceCount].ValueCount = 0;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_127);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {

        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {




        char deviceTemplate[2][128];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        string += F("<TR><TD>Host:<TD><input type='text' size='64' maxlength='128' name='Plugin_127_host' value='");
        string += deviceTemplate[0];
        string += F("'>");
        char tmpString[128];
        sprintf_P(tmpString, PSTR("<TR><TD>Port:<TD><input type='text' name='Plugin_127_port' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        string += tmpString;
        string += F("<TR><TD>Start url:<TD><input type='text' size='64' maxlength='128' name='Plugin_127_url' value='");
        string += deviceTemplate[1];
        string += F("'>");
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

        String plugin1 = WebServer.arg("Plugin_127_port");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();

        char deviceTemplate[2][128];
        char argc[25];

        String arg = F("Plugin_127_host");
        arg.toCharArray(argc, 25);
        String tmpString = WebServer.arg(argc);
        strncpy(deviceTemplate[0], tmpString.c_str(), sizeof(deviceTemplate[0]));

        arg = F("Plugin_127_url");
        arg.toCharArray(argc, 25);
        tmpString = WebServer.arg(argc);
        strncpy(deviceTemplate[1], tmpString.c_str(), sizeof(deviceTemplate[1]));

        Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        success = true;
        break;
      }


    case PLUGIN_INIT: // ok
      {
        char deviceTemplate[2][128];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        P127_HOST = String(deviceTemplate[0]);
        P127_PORT = int(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        P127_URL = String(deviceTemplate[1]);
        Serial.begin(1200, SERIAL_7E1); //Liaison série avec les paramètres
        Plugin_127_init = true;
        success = true;
        break;
      }


    case PLUGIN_SERIAL_IN:
      {
        uint8_t serial_buf[P127_BUFFER_SIZE];
        int RXWait = 10;
        int timeOut = RXWait;
        size_t bytes_read = 0;
        char log[40];


        while (timeOut > 0)
        {
          while (Serial.available()) {
            if (bytes_read < P127_BUFFER_SIZE) {
              serial_buf[bytes_read] = Serial.read();
              bytes_read++;
            }
            else {
              Serial.read();  // when the buffer is full, just read remaining input, but do not store...

            }
            timeOut = RXWait; // if serial received, reset timeout counter
          }
          delay(1);
          timeOut--;
        }

        if (bytes_read == P127_BUFFER_SIZE)  // if we have a full buffer, drop the last position to stuff with string end marker
        {
          while (Serial.available()) { // read possible remaining data to avoid sending rubbish...
            Serial.read();
          }
          bytes_read--;
          // and log buffer full situation
          // strcpy_P(log, PSTR("Teleinfo: serial buffer full!"));
          // addLog(LOG_LEVEL_ERROR, log);
        }
        serial_buf[bytes_read] = 0; // before logging as a char array, zero terminate the last position to be safe.
        //	char log[P127_BUFFER_SIZE + 40];
        //	sprintf_P(log, PSTR("Ser2N: S>: %s"), (char*)serial_buf);
        //	addLog(LOG_LEVEL_DEBUG, log);

        // We can also use the rules engine for local control!


        String message = (char*)serial_buf;
        int NewLinePos = message.indexOf("\r\n");
        if (NewLinePos > 0) {
          message = message.substring(0, NewLinePos);
        }
        String eventString = "";

        // message.replace("\r", "");
        if (message.length() > 5) {
          int indexValue = message.indexOf(" ");
          String nameValue = message.substring(1, indexValue);
          String value = message.substring(indexValue + 1, message.length() - 3);
          String checksum = message.substring(message.length() - 2);

          //  UserVar[event->BaseVarIndex] = value.toInt();

          if (P127_CYCLE == 0) {
            if (P127_checksum(nameValue, value, checksum)) {

              eventString = F("Teleinfo#");
              eventString += nameValue;
              eventString += F("=");
              eventString += value;
              P127_SendData += nameValue + "=" + value + "&";

            }

          }

          if (nameValue == "ADCO") {
            if (P127_CYCLE < 8) {
              P127_CYCLE++;
            } else {
              if (  P127_checksum(nameValue, value, checksum) ) {
                eventString = F("Teleinfo#SendData");
                P127_CYCLE = 0;
                P127_sendtoHTTP(P127_HOST, P127_PORT, P127_URL + P127_SendData);
                P127_SendData = "";

              }
            }
          }

          if (eventString.length() > 0) {
            rulesProcessing(eventString);
          }

        }

        success = true;
        break;

      }

  }
  return success;
}

boolean P127_checksum(String valuename, String value, String checksum) {
  String data = "";
  int i;
  char sum = 0;
  char sumchar;
  sumchar = checksum.charAt(0);
  data = valuename + " " + value;
  for (i = 0; i < data.length(); i++) {
    sum = sum + data.charAt(i);
  }
  sum = (sum & 0x3F) + 0x20;

  if (sum == sumchar) {
    return true;
  } else {
    return false;
  }
}


boolean P127_sendtoHTTP(String hostname, int port, String url) {


  char log[80];
  boolean success = false;
  char host[128];

  hostname.toCharArray(host, 128);
  sprintf_P(log, PSTR("%s%s using port %u"), "HTTP : connecting to ", host, port);
  addLog(LOG_LEVEL_DEBUG, log);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port))
  {
    connectionFailures++;
    strcpy_P(log, PSTR("HTTP : connection failed"));
    addLog(LOG_LEVEL_ERROR, log);
    return false;
  }
  statusLED(true);
  if (connectionFailures)
    connectionFailures--;

  url.toCharArray(log, 80);
  addLog(LOG_LEVEL_DEBUG_MORE, log);

  String hostName = host;

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timer = millis() + 200;
  while (!client.available() && millis() < timer)
    delay(1);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.toCharArray(log, 80);
    addLog(LOG_LEVEL_DEBUG_MORE, log);
    if (line.substring(0, 15) == "HTTP/1.1 200 OK")
    {
      strcpy_P(log, PSTR("HTTP : Succes!"));
      addLog(LOG_LEVEL_DEBUG, log);
      success = true;
    }
    delay(1);
  }
  strcpy_P(log, PSTR("HTTP : closing connection"));
  addLog(LOG_LEVEL_DEBUG, log);

  client.flush();
  client.stop();
}

