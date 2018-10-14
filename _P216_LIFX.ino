#ifdef USES_P216
/*
//#######################################################################################################
//######################################## Plugin 216: LIFX ########################################
//#######################################################################################################

ESPEasy Plugin to control LIFX color bulb.
LIFX bulds can be put on / off, color and intensity can as well be adjusted.
written by Didier Rossi (didros)

Main command (command line is not case sensitive):
  sendtolifx,<taskname>,<cmd>,<param>,<param>,<param>, ...
        <taskname>  : name of task as defined in the "Devices" tab in the admin webapp
        <cmd>       : power or color, see below.

List of sub commands <cmd>:
  (1) power,<value>[,<delay>] : put light on or off.
      <value>   : 0 -> off, 65535 -> on. Intensity can NOT be adjusted. Use the "color" command for that.
      <delay>   : transition delay in ms from the current state (e.g. off) to the new state (e.g. on). The light will fade nicely.
        
  (2) color,(<colorname>|<hue>,<saturation>,<brightness>,<kelvin>)[,<delay>] : change the color. Can be run light on or off.
      <delay>   : transition delay in ms from the current state (e.g. color red) to the new state (e.g. colore green). The light
                  will fade nicely from inital color to the new, visible as long as the light is on, obviously.
      <colorname>:predefined following colors, with full itensity,
                    "RED", "CYAN", "BLUE", "ORANGE", "YELLOW", "GREEN", "PURPLE", "WHITE", "WARM_WHITE", "COLD_WHTE", "GOLD"
      <hue>,<saturation>,<brightness>,<kelvin> :
                  Hue: range 0 to 65535
                  Saturation: range 0 to 65535
                  Brightness: range 0 to 65535
                  Kelvin: range 2500° (warm) to 9000° (cool)
                  See also here https://lan.developer.lifx.com/docs/light-messages


Examples with a task with name "lifx1":
  Switch the light on (no delay):
    http://192.168.1.191/control?cmd=SendToLIFX,lifx1,power,65535

  Switch the light off (no delay):
    http://192.168.1.191/control?cmd=SendToLIFX,lifx1,power,0

  Set the light to a the predefined color ORANGE, with 5 sec fading from initial to new color. 
    http://192.168.1.191/control?cmd=SendToLIFX,lifx1,color,orange,5000

  Defined the color "manually" (red) providing hue, saturation, brightness and kelvin, with 5 sec fading.
    http://192.168.1.191/control?cmd=SendToLIFX,lifx1,color,62978,65535,65535,3500,5000


*/

#define PLUGIN_216
#define PLUGIN_ID_216         216
#define PLUGIN_NAME_216       "Output - LIFX"
#define PLUGIN_VALUENAME1_216 "LIFX"

#define LIFX_UDP_PORT     56700

typedef struct __attribute__((packed)) {
  // Frame
  uint16_t msgsize;
  uint16_t protocol;
  uint32_t source;    // 32 bits/uint32, unique ID set by client. If zero, broadcast reply
  
  // Frame address
  uint8_t target_mac[8];  // 64 bits/uint64, either single MAC address or all zeroes for broadcast.
  uint8_t reserved1[6];
  uint8_t ctrl;           // 6 first bits are reserved, next bit ack_required, next bit res_required (response message)
  uint8_t seq_num;        // Message sequence number (will be provided in response if response is requested)

  // Protcol Header
  uint64_t reserved2;
  uint16_t packet_type;   // Message type determines the payload being used
  uint16_t reserved3;
  
} lx_header_t;

typedef struct __attribute__((packed)) {
  uint8_t   rfu;
  uint16_t  hue;
  uint16_t  saturation;
  uint16_t  brightness;
  uint16_t  kelvin;
  uint32_t  duration;
} setcolor_102_t;

typedef struct __attribute__((packed)) {
  uint16_t  level;
  uint32_t  duration;
} setpower_117_t;


boolean Plugin_216(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_216;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Custom = true;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_216);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_216));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        // IP Address
        char strIP[20];
        IPAddress tmpIP(((byte *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex])[0],
                        ((byte *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex])[1],
                        ((byte *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex])[2],
                        ((byte *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex])[3]);
        formatIP(tmpIP, strIP);
        addFormTextBox(F("LIFX IP address"), F("lifxip"), strIP, 15);

        char str[18];
        // Shown MAC address with nice format
        sprintf_P(str, PSTR("%02x:%02x:%02x:%02x:%02x:%02x"),   ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[0], 
                                                                ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[1], 
                                                                ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[2],
                                                                ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[3],
                                                                ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[4],
                                                                ((byte*)&Settings.TaskDevicePluginConfig[event->TaskIndex])[5]);
        addFormTextBox(F("LIFX MAC address"), F("lifxMAC"), str, 17);

      	success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        // IP Address
        byte tmpIP[4];
        str2ip(WebServer.arg(F("lifxip")), tmpIP);
        memcpy((byte *)Settings.TaskDevicePluginConfigLong[event->TaskIndex], tmpIP,4);

        str2mac(WebServer.arg(F("lifxMAC")), (byte*)Settings.TaskDevicePluginConfig[event->TaskIndex]);

        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        String log = "";
        String command = parseString(string, 1);
        Serial.print(F("In _LIFX, command= ")); Serial.println(command);
        if (command == F("sendtolifx")) {
          success = true; // Return true only if is a plugin specific command

          uint8_t data[50];
          uint16_t msglength=0;
          uint32_t duration=0;
          
          String taskName = parseString(string, 2);
          int8_t taskIndex = getTaskIndexByName(taskName);
          if (taskIndex != -1)
          {

            String cmd = parseString(string,3);
            String value1 = parseString(string,4);
            String value2 = parseString(string,5);
            String value3 = parseString(string,6);
            String value4 = parseString(string,7);
            String value5 = parseString(string,8);

            if (cmd == "power") { // On / off
              Serial.print(F("SendToLIFX, Set Power, value= ")); Serial.println(value1);
              //log = String(F("SendToLIFX, Set Power, value= ")) + value1;
              //addLog(LOG_LEVEL_INFO, log);

              int state = atoi(value1.c_str());
              if (value2.length() > 0) {
                duration = atoi(value2.c_str());
              }
              msglength = getPktSetBulbPower(taskIndex, data, state, duration);
            }
            else if (cmd == "color") { 
              // Define color, intensity, transition time (duration)
              // Parameters syntax :
              // - Alt 1: predefined color (full brightness), duration (in ms)
              //          e.g. http://192.168.1.191/control?cmd=SendToLIFX,lifx1,color,orange,5000
              // - Alt 2: hue, saturation, brightness, kelvin, duration
              //          e.g. (red) http://192.168.1.191/control?cmd=SendToLIFX,lifx1,color,62978,65535,65535,3500,5000
              // See also here https://lan.developer.lifx.com/docs/light-messages
              Serial.println("SendToLIFX, Set color " + value1 + " " + value2 + " " + value3 + " " + value4);

              if (value2.length() > 0) {
                duration = atoi(value2.c_str());
              }
              
              if (strcasecmp_P(value1.c_str(),PSTR("RED")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,62978,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("CYAN")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,29814,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("ORANGE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,5525,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("YELLOW")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,7615,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("GREEN")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,16173,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("BLUE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,43634,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("PURPLE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,50486,65535,65535,3500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("WHITE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,58275, 0, 65535, 5500, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("COLD_WHITE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,58275, 0, 65535, 9000, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("WARM_WHITE")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,58275, 0, 65535, 3200, duration);
              }
              else if (strcasecmp_P(value1.c_str(),PSTR("GOLD")) == 0) {
                msglength = getPktSetBulbColor(taskIndex, data,58275, 0, 65535, 2500, duration);
              }
              else {
                duration = 0;
                if (value5.length() > 0) {
                  duration = atoi(value5.c_str());
                }
                msglength = getPktSetBulbColor(taskIndex, data, atoi(value1.c_str()), atoi(value2.c_str()), atoi(value3.c_str()), atoi(value4.c_str()), duration);
              }
            }
            else {
              success = false;
            }

            if (success) {
              IPAddress UDP_IP( ((byte *)&Settings.TaskDevicePluginConfigLong[taskIndex])[0],
                                ((byte *)&Settings.TaskDevicePluginConfigLong[taskIndex])[1],
                                ((byte *)&Settings.TaskDevicePluginConfigLong[taskIndex])[2],
                                ((byte *)&Settings.TaskDevicePluginConfigLong[taskIndex])[3]);
        
              portUDP.beginPacket(UDP_IP, LIFX_UDP_PORT);
              portUDP.write(data, (uint8_t)msglength);
              portUDP.endPacket();
            }
          }
        }
        break;
      }

  }
  return success;
}

uint16_t getPktSetBulbPower(int8_t taskIndex, uint8_t * data, int state, uint32_t duration) {
  lx_header_t * tmp;
  setpower_117_t * tmp2;

  tmp = (lx_header_t *)data;

  memset(data, 0, 42);
  
  tmp->msgsize = 42; // 36 (header) + 6 (setpower payload)

  tmp->protocol = 0x1400;  // PROTOCOL_COMMAND
  tmp->packet_type = 117; // Light message : SetPower - 117 (0x75)
  memcpy(&tmp->target_mac, Settings.TaskDevicePluginConfig[taskIndex], 6);
  memcpy(&tmp->source, "didi", 4);
  
  // The payload : https://lan.developer.lifx.com/docs/light-messages#section-setpower-117
  tmp2 = (setpower_117_t *)&data[36];
  tmp2->level = state;
  tmp2->duration = duration;

  print_msg_serial(data, tmp->msgsize);
  
  return tmp->msgsize;
}


uint16_t getPktSetBulbColor(int8_t taskIndex, uint8_t * data, uint16_t h, uint16_t s, uint16_t b, uint16_t k, uint32_t duration) {
  lx_header_t * tmp;
  setcolor_102_t * tmp2;

  tmp = (lx_header_t *)data;

  memset(data, 0, 49);

  tmp->msgsize = 49; // 36 (header) + 13 (setcolor payload)
  
  tmp->protocol = 0x1400;  // PROTOCOL_COMMAND
  tmp->packet_type = 102; // Light message : SetColor - 102 (0x66)
  memcpy(&tmp->target_mac, Settings.TaskDevicePluginConfig[taskIndex], 6);
  memcpy(&tmp->source, "didi", 4);
  
  // The payload HSBK : https://lan.developer.lifx.com/docs/light-messages#section-hsbk
  tmp2 = (setcolor_102_t *)&data[36];
  tmp2->duration = duration;
  tmp2->hue = h;
  tmp2->saturation = s;
  tmp2->brightness = b;
  tmp2->kelvin = k;

  print_msg_serial(data, tmp->msgsize);
  
  return tmp->msgsize;
}

void print_msg_serial(uint8_t * data, uint16_t len) {
   char str[10];
   for (int i=0 ; i<len; i++) {
      sprintf_P(str, PSTR("%02x "), data[i]);
      if ( (i+1) % 8 == 0 )
        Serial.println(str);
      else
        Serial.print(str);
   }
   Serial.println("");
}

/********************************************************************************************
  Convert a String representing a MAC address, with ":" between bytes, to byte array
 ********************************************************************************************/
void str2mac(String string, byte* MAC)
{
  String tmpString = string;
  tmpString += ":";
  String locateString = "";
  byte count = 0;
  int index = tmpString.indexOf(':');
  while (count < 6)
  {
    locateString = tmpString.substring(0, index);
    tmpString = tmpString.substring(index + 1);
    MAC[count] = (byte)strtol(locateString.c_str(), NULL, 16);
    index = tmpString.indexOf(':');
    count++;
  }
}

#endif