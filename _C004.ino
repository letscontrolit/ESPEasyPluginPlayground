//#######################################################################################################
//########################### Controller Plugin 104: ThingSpeak #########################################
//#######################################################################################################

#define CPLUGIN_004
#define CPLUGIN_ID_004         4
#define CPLUGIN_NAME_004       "ThingSpeak"

#define NUM_OF_FIELDS      8
#define THINGSPEAK_DELAY   15

float field_data[NUM_OF_FIELDS];
byte field_format[NUM_OF_FIELDS];
unsigned long thingspeak_timer;

boolean CPlugin_004(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  byte x;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_004;
        Protocol[protocolCount].usesMQTT = false;
        Protocol[protocolCount].usesAccount = false;
        Protocol[protocolCount].usesPassword = true;
        Protocol[protocolCount].defaultPort = 80;

        thingspeak_timer = millis() + THINGSPEAK_DELAY*1000 + 30000;
        for(byte i=0; i<NUM_OF_FIELDS; i++)
        {
          field_data[i] = 0;
          field_format[i] = 255;
        }          
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_004);
        break;
      }
      
    case CPLUGIN_PROTOCOL_SEND:
      {
        char log[80];
        boolean success = false;

        byte valueCount = getValueCountFromSensorType(event->sensorType);
        for (x = 0; x < valueCount; x++)
        {
          if(event->idx + x - 1 < NUM_OF_FIELDS)
          {
            field_data[event->idx + x - 1] = UserVar[event->BaseVarIndex + x];
            field_format[event->idx + x - 1] = ExtraTaskSettings.TaskDeviceValueDecimals[x];
          }
        }

        
        if(millis() > thingspeak_timer)
        {
          String postDataStr = F("api_key=");
          postDataStr += SecuritySettings.ControllerPassword; // used for API key
          boolean sendUpdate = false;
          for (x = 0; x < NUM_OF_FIELDS; x++)
          {
            if(field_format[x]!=255)
            {
              postDataStr += F("&field");
              postDataStr += x+1;
              postDataStr += "=";
              int tmp = (int)(field_data[x]*100);
              postDataStr += toString(field_data[x],field_format[x]);
              sendUpdate = true;
            }
          }

          if(sendUpdate)
          {
            thingspeak_timer = millis() + THINGSPEAK_DELAY*1000;

            char host[20];
            sprintf_P(host, PSTR("%u.%u.%u.%u"), Settings.Controller_IP[0], Settings.Controller_IP[1], Settings.Controller_IP[2], Settings.Controller_IP[3]);
            
            sprintf_P(log, PSTR("%s%s using port %u"), "HTTP : connecting to ", host,Settings.ControllerPort);
            addLog(LOG_LEVEL_DEBUG, log);
  
            WiFiClient client;
            if (!client.connect(host, Settings.ControllerPort))
            {
              connectionFailures++;
              strcpy_P(log, PSTR("HTTP : connection failed"));
              addLog(LOG_LEVEL_ERROR, log);
              thingspeak_timer -= Settings.Delay*1000;
              return false;
            }
            statusLED(true);
            if (connectionFailures)
              connectionFailures--;
    
            String hostName = F("api.thingspeak.com"); // PM_CZ: HTTP requests must contain host headers.
            if (Settings.UseDNS)
              hostName = Settings.ControllerHostName;
    
            String postStr = F("POST /update HTTP/1.1\r\n");
            postStr += F("Host: ");
            postStr += hostName;
            postStr += F("\r\n");
            postStr += F("Connection: close\r\n");
    
            postStr += F("Content-Type: application/x-www-form-urlencoded\r\n");
            postStr += F("Content-Length: ");
            postStr += postDataStr.length();
            postStr += F("\r\n\r\n");
            postStr += postDataStr;
    
            // This will send the request to the server
            client.print(postStr);
    
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
            
            for(byte i=0; i<NUM_OF_FIELDS; i++)
              field_format[i] = 255;
          }
        }       
        break;

      }
  }
  return success;
}

