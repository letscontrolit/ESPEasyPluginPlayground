//#######################################################################################################
//#################################### Plugin 110: P1Gateway ############################################
//#######################################################################################################

/*
	This plug-in is meant for the P1-wifi gateway board (see romix.macuser.nl). It interfaces a 
	Wemos D1 mini with the P1 Smart Meter port. The board inverst the comm signals provided by the P1 port
	The plug-in:
		locally stores key values provided by the Smart meter to be read using a webbrower
		(optionally) locally stores raw data on a micro SD-card if the SD shield is present
		can send current meter values to a slave unit containing an Oled display
		sends data to Domoticz (as the 020 Ser2Net plug-in does)

    This plug-in requires adding  these lines to WebServer.ino 
    
     in fucntion:
     void WebServerInit()
        after  WebServer.on("/rules", handle_rules)

     add:
         WebServer.on("/downloadSD", handle_SDdownload);
         WebServer.on("/emptySD", handle_SDwipe);
         WebServer.on("/P1", handle_P1monitor);


      in function:
         void addHeader(boolean showMenu, String& str)
      after
          if (Settings.UseRules)
          str += F("<a class=\"button-menu\" href=\"rules\">Rules</a>");
      add:
          str += F("<a class=\"button-menu\" href=\"tools\">Tools</a><BR>");
          for (int i=0; i<= deviceCount; i++){
             if (Device[i].Number == 110) {
              str += F("<a class=\"button-menu\" href=\"P1\">P1</a>"); 
              if (ExtraTaskSettings.TaskDevicePluginConfigLong[5]) {
                  str += F("<a class=\"button-menu\" href=\"downloadSD\">SDdump</a>"); 
                  str += F("<a class=\"button-menu\" href=\"emptySD\">SDwipe</a>");
              }
            }
          }  
          str += F("<BR>"); 

 

*/
//#define STATUS_LED 14

#define PLUGIN_110
#define PLUGIN_ID_110         110
#define PLUGIN_NAME_110       "P1 Gateway"
#define PLUGIN_VALUENAME1_110 "P1Gateway"

#define BUFFER_SIZE 1024
#define NETBUF_SIZE 128

unsigned long P1lastSent = millis();
boolean Plugin_110_init = false;


//size_t bytes_read = 0;
//int state = 0;
//#define DISABLED 0
//#define WAITING 1
//#define READING 2
//#define CHECKSUM 3
//#define DONE 4
String checkS = ""; 
//int checkI = 0;
//int line=0;

// recipients
bool hasOLED = false; 
bool hasSD = false;
bool hasDomoticz = false;
bool parsed = false;
bool displayed = false;
bool stored = false;
bool domoticzd = false;
int tempState = 0;

// RL SD card support
#include <SPI.h>
#include <SD.h>
int canary = 0;
boolean pause = false; // use to hold off receiving data while busy with SD card
const int chipSelect = D8;


Sd2Card card;
SdVolume volume;
SdFile root;
File dataFile;

//OLED vars
int pulse =0;
String command;


// Local datastore
char ch;
int incomingByte = 0;
//int pos181;
//int pos182;
//int pos170;
//int pos270;
//int pos281;
//int pos282;
//int pos2421; // gas 0-1:24.2.1
//int tempPos;
//int tempPos2;
//String inputString = "";
//String T170;
//String T270;
//String T181;
//String T182;
//String T281;
//String T282;
//String G2421;
//int counter = 0;

// end local datastore

WiFiServer *P1GatewayServer;
WiFiClient P1GatewayClient;

String trim_zero(String data){
  int i = 0;
  while (data.charAt(i) =='0' && data.charAt(1+i) !='.') {
    data.setCharAt(i,' ');
    i++;
  }
  data.trim();
  return data;
}
  
int month()
{
  return tm.Month;
}

int day()
{
  return tm.Day;
}

int year()
{
  return tm.Year;
}




void parse_P1(String inputString){
           //  if (inputString.indexOf("/") != -1 && inputString.indexOf("!") != -1) {//  && !parsed) {
                       addLog(LOG_LEVEL_DEBUG_MORE,"Parsing data");           

                        pos181 = inputString.indexOf("1-0:1.8.1", 0);
                        tempPos = inputString.indexOf("*kWh)", pos181);
                        if (tempPos != -1)
                            T181 = trim_zero(inputString.substring(pos181 + 10, tempPos + 4));

                        pos182 = inputString.indexOf("1-0:1.8.2", 0);
                        tempPos = inputString.indexOf("*kWh)", pos182);
                        if (tempPos != -1)
                            T182 = trim_zero(inputString.substring(pos182 + 10, tempPos + 4));

                         pos281 = inputString.indexOf("1-0:2.8.1", 0);
                        tempPos = inputString.indexOf("*kWh)", pos281);
                        if (tempPos != -1)
                            T281 = trim_zero(inputString.substring(pos281 + 10, tempPos + 4));

                         pos282 = inputString.indexOf("1-0:2.8.2", 0);
                        tempPos = inputString.indexOf("*kWh)", pos282);
                        if (tempPos != -1) T282 = trim_zero(inputString.substring(pos282 + 10, tempPos + 4));
                        
                        pos170 = inputString.indexOf("1-0:1.7.0", 0);
                        tempPos = inputString.indexOf("*kW)", pos170);
                        if (tempPos != -1) T170 = trim_zero(inputString.substring(pos170 + 10, tempPos+3));

                        pos270 = inputString.indexOf("1-0:2.7.0", 0);
                        tempPos = inputString.indexOf("*kW)", pos270);
                        if (tempPos != -1) T270 = trim_zero(inputString.substring(pos270 + 10, tempPos+3));

                        pos2421 = inputString.indexOf("0-1:24.2.1", 0);
                        tempPos = inputString.indexOf("S)(", pos2421);
                        tempPos2 = inputString.indexOf("*m3)", tempPos);

                       // 0-1:24.2.1(160516110000S)(06303.228*m3)
                        if (tempPos2 != -1) G2421 = trim_zero(inputString.substring(tempPos + 3, tempPos2+3));

                       
                        parsed = true;
                        addLog(LOG_LEVEL_DEBUG_MORE,"Parsing data: done");           

          //   }
}

void handle_OLED(){
                       //  addLog(LOG_LEVEL_DEBUG,"Handling OLed");           

   switch (pulse) {
              case 5:   {                                             // Current
                          command = "/control?cmd=oled,1,1,Actueel%20%20%20%20%20%20%20%20%20";
                          command.concat("%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20");
                                  command.concat("%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20");
                          command.concat("Totaal%20T1/T2%20%20%20%20");
                          command.concat("%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20");
                                  command.concat("%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20");
                          command.concat("Totaal%20gas%20%20%20%20%20%20");
                          command.concat("%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20");
                          
                          break;
                        }

              case 10: 
                       {                                             // Current
                          command = "/control?cmd=oled,2,2,";
                          command.concat(trim_zero(T170));
                          break;
                        }
              case 20: 
                       {                                             // Current
                          command = "/control?cmd=oled,2,2,";
                          command.concat(trim_zero(T170));
                          break;
                        }
              case 15:  {                                             // Totaal T1
                          command = "/control?cmd=oled,5,2,";
                          command.concat(trim_zero(T181));
                          break;
                        }

              case 23:  {                                             // Totaal T2
                          command = "/control?cmd=oled,6,2,";
                          command.concat(trim_zero(T182));
                          break;
                        }

              case 28:  {                                             // Gas T1
                          command = "/control?cmd=oled,8,2,";
                          command.concat(trim_zero(G2421));
                          break;
                        }
              case 1:  {                                             // Clear display
                          command = "/cmd=oledcmd,clear";
                          break;
                        }
              
            default: break;
            } //switch 
            pulse++;
            if (pulse == 180) pulse = 0;
            P1_2_oLED(command);
          //  addLog(LOG_LEVEL_DEBUG,"Handling OLed: done"); 
            }

void handle_SD(){
tempState = state;
                      state = DISABLED;
                                                  addLog(LOG_LEVEL_DEBUG_MORE,"Handling SD");           

                           File dataFile = SD.open("DATA.TXT", FILE_WRITE);
                           if (dataFile){
                            #if FEATURE_TIME
                                if (Settings.UseNTP)
                                 {
                                  dataFile.println();
                                  String reply = ""; 
                                  reply += F("timestamp: ");
                                  reply += day();
                                  reply += "-";
                                  reply += month();
                                  reply += "-";
                                  reply += year();
                                  reply += " ";
                                  reply += hour();
                                  reply += ":";
                                   if (minute() < 10)
                                    reply += "0";
                                  reply += minute();
                                  dataFile.println(reply);
                                  }
                              #endif
                          
                                dataFile.print(inputString); //(const uint8_t*)serial_buf, bytes_read);
                                dataFile.close();
                                stored = true;
                                } else addLog(LOG_LEVEL_ERROR,"Data file does not exist");

                                
                          addLog(LOG_LEVEL_DEBUG_MORE,"Handling SD: done");           
                           state = tempState;
} //SD card

void handle_Domoticz(){
  tempState = state;
          //P1GatewayClient = P1GatewayServer->available();
         // if (P1GatewayClient) {

         addLog(LOG_LEVEL_DEBUG_MORE,"Handling Domoticz");
         state = DISABLED;
                 P1GatewayClient.print(inputString); //write((const uint8_t*)serial_buf, bytes_read);
                 addLog(LOG_LEVEL_DEBUG_MORE,"Handling Domoticz: done");
                 domoticzd = true;
         // }
         state = tempState;
  }


void read_data(){
    char messageCRC[4];
    int checkI = 0;
  while (Serial.available() ) {
                    ch = Serial.read();
                    //Serial.write(ch);
                    switch (state){
                      case DISABLED: //ignore incoming data
                        break;
                      case WAITING:
                        if (ch =='/')  {
                             // serial_buf[bytes_read] = ch;
                             if (inputString.length() < 1000) inputString += ch;
                              state = READING;
                              digitalWrite(STATUS_LED,1);

                             // bytes_read++;
                          } // else ignore data
                        break;
                      case READING:
                         // serial_buf[bytes_read] = ch;
                             if (inputString.length() < 1000) inputString += ch;
                        //  bytes_read++;
               
                          if (ch == '!'){
                              state = CHECKSUM;
                              addLog(LOG_LEVEL_DEBUG,"Data reading from P1: done");
                              //Serial.println("read from P1 -->");                                 
                              //Serial.print(inputString);
                              //Serial.println("<-- done from P1");   
                              digitalWrite(STATUS_LED,0);

                              parsed = false;  // reset all actors
                              stored = false;
                              domoticzd = false;
                          }
                          break;
                       case CHECKSUM:
                          checkI ++;
                          if (checkI == 3) {
                               checkI = 0;
                               state = DONE;
                               //inputString = ""; 
                              }
                          break;
                       case DONE:
                         // state = WAITING;
                          break;
                                 

                      }
  } //while
}

            
boolean Plugin_110(byte function, struct EventStruct *event, String& string){
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_110;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Custom = true;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_110);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_110));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char tmpString[128];
        sprintf_P(tmpString, PSTR("<TR><TD>TCP Port:<TD><input type='text' name='plugin_110_port' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[0]);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Baud Rate:<TD><input type='text' name='plugin_110_baud' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Data bits:<TD><input type='text' name='plugin_110_data' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
        string += tmpString;

        byte choice = ExtraTaskSettings.TaskDevicePluginConfigLong[3];
        String options[3];
        options[0] = F("No parity");
        options[1] = F("Even");
        options[2] = F("Odd");
        int optionValues[3];
        optionValues[0] = 0;
        optionValues[1] = 2;
        optionValues[2] = 3;
        string += F("<TR><TD>Parity:<TD><select name='plugin_110_parity'>");
        for (byte x = 0; x < 3; x++)
        {
          string += F("<option value='");
          string += optionValues[x];
          string += "'";
          if (choice == optionValues[x])
            string += F(" selected");
          string += ">";
          string += options[x];
          string += F("</option>");
        }
        string += F("</select>");

        sprintf_P(tmpString, PSTR("<TR><TD>Stop bits:<TD><input type='text' name='plugin_110_stop' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[4]);
        string += tmpString;
        
        string += F("<TR><TD>Store data on SD card:<TD>");
        if (ExtraTaskSettings.TaskDevicePluginConfigLong[5])
          string += F("<input type=checkbox name=plugin_110_sd checked>");
        else
          string += F("<input type=checkbox name=plugin_110_sd>");

        string += F("<TR><TD>Display data on Slave module:<TD>");
        if (ExtraTaskSettings.TaskDevicePluginConfigLong[6])
          string += F("<input type=checkbox name=plugin_110_slave checked>");
        else
          string += F("<input type=checkbox name=plugin_110_slave>");

          sprintf_P(tmpString, PSTR("<TR><TD>Slave IP:<TD><input type='text' name='plugin_110_slaveip1' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[7]);
          string += tmpString;
          sprintf_P(tmpString, PSTR("<input type='text' name='plugin_110_slaveip2' value='%u' width='10'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[8]);
         string += tmpString;
         sprintf_P(tmpString, PSTR("<input type='text' name='plugin_110_slaveip3' value='%u' width='20'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[9]);
         string += tmpString;
         sprintf_P(tmpString, PSTR("<input type='text' name='plugin_110_slaveip4' value='%u' width='30'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[10]);
          string += tmpString;

        string += F("<TR><TD>Send data to Domoticz:<TD>");
        if (ExtraTaskSettings.TaskDevicePluginConfigLong[11])
          string += F("<input type=checkbox name=plugin_110_domoticz checked>");
        else
          string += F("<input type=checkbox name=plugin_110_domoticz>");
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_110_port");
        ExtraTaskSettings.TaskDevicePluginConfigLong[0] = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_110_baud");
        ExtraTaskSettings.TaskDevicePluginConfigLong[1] = plugin2.toInt();
        String plugin3 = WebServer.arg("plugin_110_data");
        ExtraTaskSettings.TaskDevicePluginConfigLong[2] = plugin3.toInt();
        String plugin4 = WebServer.arg("plugin_110_parity");
        ExtraTaskSettings.TaskDevicePluginConfigLong[3] = plugin4.toInt();
        String plugin5 = WebServer.arg("plugin_110_stop");
        ExtraTaskSettings.TaskDevicePluginConfigLong[4] = plugin5.toInt();
        String plugin6 = WebServer.arg("plugin_110_sd");
        ExtraTaskSettings.TaskDevicePluginConfigLong[5] = (plugin6 == "on");
        String plugin7 = WebServer.arg("plugin_110_slave");
        ExtraTaskSettings.TaskDevicePluginConfigLong[6] = (plugin7 == "on");
        //String plugin8 = WebServer.arg("plugin_110_slaveIP");
        String plugin8 = WebServer.arg("plugin_110_slaveip1");
        ExtraTaskSettings.TaskDevicePluginConfigLong[7] = plugin8.toInt();    // slave IP octet 1
        String plugin9 = WebServer.arg("plugin_110_slaveip2");
        ExtraTaskSettings.TaskDevicePluginConfigLong[8] = plugin9.toInt();   // slave IP octet 2
        String plugin10 = WebServer.arg("plugin_110_slaveip3");
        ExtraTaskSettings.TaskDevicePluginConfigLong[9] = plugin10.toInt(); // slave IP octet 3
        String plugin11 = WebServer.arg("plugin_110_slaveip4");
        ExtraTaskSettings.TaskDevicePluginConfigLong[10] = plugin11.toInt(); // slave IP octet 4
        String plugin12 = WebServer.arg("plugin_110_domoticz");
        ExtraTaskSettings.TaskDevicePluginConfigLong[11] = (plugin12 == "on");
        
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        pinMode(STATUS_LED, OUTPUT);
                digitalWrite(STATUS_LED,0);

        

        
        LoadTaskSettings(event->TaskIndex);
        if ((ExtraTaskSettings.TaskDevicePluginConfigLong[0] != 0) && (ExtraTaskSettings.TaskDevicePluginConfigLong[1] != 0))
        {
          byte serialconfig = 0x10;
          serialconfig += ExtraTaskSettings.TaskDevicePluginConfigLong[3];
          serialconfig += (ExtraTaskSettings.TaskDevicePluginConfigLong[2] - 5) << 2;
          if (ExtraTaskSettings.TaskDevicePluginConfigLong[4] == 2)
            serialconfig += 0x20;
          #if ESP_CORE >= 210
            Serial.begin(ExtraTaskSettings.TaskDevicePluginConfigLong[1], (SerialConfig)serialconfig);
          #else
            Serial.begin(ExtraTaskSettings.TaskDevicePluginConfigLong[1], serialconfig);
          #endif

          if (ExtraTaskSettings.TaskDevicePluginConfigLong[11]) { //Domoticz
              hasDomoticz = true;
              addLog(LOG_LEVEL_DEBUG,"Starting server");
              P1GatewayServer = new WiFiServer(ExtraTaskSettings.TaskDevicePluginConfigLong[0]);
              P1GatewayServer->begin();
          } else hasDomoticz = false;
          
          if (ExtraTaskSettings.TaskDevicePluginConfigLong[5]) {  // SD card
              if (SD.begin(chipSelect)){
                 addLog(LOG_LEVEL_DEBUG,"SD Card initialized.");
                 hasSD = true;
              }
          }
          
          if (ExtraTaskSettings.TaskDevicePluginConfigLong[6]) {  // Oled slave
                 addLog(LOG_LEVEL_DEBUG,"OLED initialized.");
                 hasOLED = true;
          }
          Plugin_110_init = true;
        }
        state = WAITING; //let's go 
        addLog(LOG_LEVEL_DEBUG,"Armed and ready to go.");
        digitalWrite(STATUS_LED,1);
        delay(500);
        digitalWrite(STATUS_LED,0);
        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:{
        if (Plugin_110_init && hasDomoticz) {
   //       size_t bytes_read;
          if ( !P1GatewayClient && state == DONE)
          {
           P1GatewayClient = P1GatewayServer->available();
          }
          
          if (P1GatewayClient.connected())
          {
            uint8_t net_buf[NETBUF_SIZE];
            int count = P1GatewayClient.available();
            if (count > 0) {
              if (count > NETBUF_SIZE)
                count = NETBUF_SIZE;
              bytes_read = P1GatewayClient.read(net_buf, count);
              Serial.write(net_buf, bytes_read);
              Serial.flush();

              net_buf[count]=0;
              addLog(LOG_LEVEL_DEBUG,(char*)net_buf);
            }
          }

          
          success = true;
        }
        break;
      }

case PLUGIN_ONCE_A_SECOND: {
        if (Plugin_110_init) {
            canary ++;
            if (canary == 360) { //we're not reset the last 5 minutes, assume we died
               addLog(LOG_LEVEL_ERROR,"Canary died..... Resetting");
              delay(10000);
              ESP.reset();
            }  

          if (hasOLED){       //  oLED display
              handle_OLED();
               yield();
          }
          
       if (state == DONE){
                if (!parsed){
                //  parse_P1();
                  canary = 0;
                  // yield();
                }

                if (hasDomoticz && !domoticzd){
                  handle_Domoticz();
                  canary = 0;
                 // yield();
                }

                    
                if (hasSD && !stored){
                  handle_SD();
                  //yield();
                }
              
              if ((domoticzd && parsed) || (!hasDomoticz && parsed)){
                  canary = 0;
                  addLog(LOG_LEVEL_DEBUG,"New cycle");
                  parsed= false;
                  domoticzd = false;
                  stored = false;
                  displayed=false;
                  state = WAITING; 
                  inputString = "";
                  Serial.flush();
              } 
          }
   
          success = true;
        }
        break;
      }

  case PLUGIN_SERIAL_IN: {
        if (Plugin_110_init && state != DISABLED ) {
          if (Serial.available() > 0) {
            read_data();
          }
           
          success = true;
        } //init
        break;
      } //PLUGIN_SERIAL_IN
  } //switch
} // plug_in


//********************************************************************************
// Ronald experimental SD card support 
//
// Web Interface download page for SD card
//********************************************************************************
void handle_SDdownload(){
  pause = true;
  if (hasSD){
     addLog(LOG_LEVEL_DEBUG,"Reading P1 data from SD card");
     
    File dataFile = SD.open("DATA.TXT");
    if (dataFile) {
      WebServer.sendHeader("Content-Disposition", "attachment; filename=DATA.TXT");
      WebServer.streamFile(dataFile, "application/octet-stream");
      // close the file:
      dataFile.close();
      pause = false;
    } else {
        // if the file didn't open, print an error:
         addLog(LOG_LEVEL_DEBUG,"Can't open data file on SD card)");
     }
  }
   return;
}

void handle_SDwipe(){
  pause = true;
      SD.remove("DATA.TXT");
      if (SD.exists("DATA.TXT")) {
          Serial.println("DATA.TXT exists.");
      }  else {
          Serial.println("DATA.TXT doesn't exist.");
      }
    pause = false;
}

void handle_P1monitor(){
  int i=0;

	if (!isLoggedIn()) return;


 String str = "";
  addHeader(true, str);
  str += F("<script language='JavaScript'>function RefreshMe(){window.location = window.location}setTimeout('RefreshMe()', 3000);</script>");
  str += F("<table><TH>P1 monitor<TH> ");

  str += "<TR><TD>Totaal verbruik tarief 1: <TD>";
  	str += T181;
//         str += F("<BR>");
  str += "<TR><TD>Totaal verbruik tarief 2: <TD>";
  	str += T182;
//         str += F("<BR>");
  str += "<TR><TD>Totaal geleverd tarief 1: <TD>";
  	str += T281;
 //        str += F("<BR>");
   str += "<TR><TD>Totaal geleverd tarief 2: <TD>";
  	str += T281;
//         str += F("<BR>");
  str += "<TR><TD>Actueel verbruik         : <TD>";
  	str += T170;
  //       str += F("<BR>");
   str += "<TR><TD>Huidige teruglevering   : <TD>";
  	str += T270;
    str += "<TR><TD>Totaal gasverbuik   : <TD>";
   str += G2421;
 //        str += F("<BR><BR>");
   //  str += "Input   : ";
   //       str += inputString;

 str += F("</table>");
  addFooter(str);
  WebServer.send(200, "text/html", str);
 // free(TempString);
  
	}

void P1_2_oLED(String &command){
   // Use WiFiClient class to create TCP connections
  char host[20];// = "10.0.1.11" ;
  String hosti;
  if (ExtraTaskSettings.TaskDevicePluginConfigLong[6]){    //only work if OLED is supposed to exist
  hosti.concat( ExtraTaskSettings.TaskDevicePluginConfigLong[7]);
  hosti.concat('.');
   hosti.concat( ExtraTaskSettings.TaskDevicePluginConfigLong[8]);
  hosti.concat('.');
   hosti.concat( ExtraTaskSettings.TaskDevicePluginConfigLong[9]);
  hosti.concat('.');
 hosti.concat( ExtraTaskSettings.TaskDevicePluginConfigLong[10]);
 int len = hosti.length();
hosti.toCharArray(host, len+1);

   //addLog(LOG_LEVEL_DEBUG,host);
   //
  WiFiClient Client;
  const int httpPort = 80;
  if (!Client.connect(host, httpPort)) {
    addLog(LOG_LEVEL_DEBUG,"OLED Connection failed");
    return;
  }

// http://<ESP IP address>/control?cmd=oled,<row>,<col>,<text>  http://<ESP IP address>/control?cmd=oledcmd,on
// http://<ESP IP address>/control?cmd=oledcmd,off // http://<ESP IP address>/control?cmd=oledcmd,clear


  // This will send the request to the server
 Client.print(String("GET ") + command + "&headers=false" + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  delay(500);
    
      while (Client.available()) {
        String line = Client.readStringUntil('\n');
        if (line.substring(0, 15) == "HTTP/1.1 200 OK")
         // addLog(LOG_LEVEL_DEBUG, line);
        delay(1);
      }
      Client.flush();
      Client.stop();
  }
}
