//#######################################################################################################
//################ Plugin 096: Viessmann Vitotronic  									################### 
/*  Usage: This plugin does add two webpages /read and /write which are used to interface with the Vito
 *  use /read and /write to access data points.
 *    
 *  Parameters for /read: 
 *  &DP=0x2000 data point to read, in hex
 *  &Type=TempL|TempS|CountL|CountS|Mode|Hours|CoP|Raw|all
 *  &ReadLen=2 if you use Raw or all. Otherwise the number of bytes to read is derived from the data type. 
 *  &MQTTTopic=vito/test untested; may require mqtt controller set up
 *  
 *  Example: http://192.168.1.18/read?DP=0x00f8&Type=all&ReadLen=2
 *  
 *  Parameters for /write: 
 *  &DP=0x2000 data point to read, in hex
 *  &Type=TempL|TempS|CountL|CountS|Mode|Hours|CoP 
 *  &Value=12.3  
 *   
 *  Example: http://192.168.1.18/write?DP=0x2000&Type=TempL&Value=22.1
 *  
 *  

*/

//#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_096
#define PLUGIN_ID_096         96
#define PLUGIN_NAME_096       "Vito [TESTING]"
 
#include <OptolinkP300.hpp>  // requires VitoWifi https://github.com/bertmelis/VitoWiFi // just extract to ESPEasy/lib/VitoWifi or create a (win:hard~) link and put it here.
#include <Datapoint.hpp>     // requires VitoWifi https://github.com/bertmelis/VitoWiFi // just extract to ESPEasy/lib/VitoWifi or create a (win:hard~) link and put it here.

//==============================================
// vitotronic LIBRARY - vito.h
// =============================================
# ifndef VITO_H
# define VITO_H
 
 
#endif

//==============================================
// vito implementaion - MT681.cpp
// =============================================
 
#ifndef CONFIG
#define CONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][n])
#endif
OptolinkP300* myOptolinkPlugin_182 = NULL;

String logString;
bool readWriteDP (word address, byte Len, uint8_t* rawdata, bool Write = false ){
  logString=""; 
  unsigned long now = millis(); 
  if ( Len==0 ) { addLog(LOG_LEVEL_DEBUG,F("Vito: readLen 0 is invalid")); return false;}
  while (myOptolinkPlugin_182->isBusy() && ((millis()-now) < 3000)) 
      {
      delay(10);  // wait for optolink to become available
      myOptolinkPlugin_182->loop();
      }
     if (myOptolinkPlugin_182->isBusy() ) { 
        addLog(LOG_LEVEL_DEBUG,F("Vito: optolink busy "));
        return false;
        }
      logString+=(F("Vito: Address: 0x")); 
        if (address<0x1000) logString+=String("0");
        if (address<0x100) logString+=String("0");
        if (address<0x10) logString+=String("0");
        
      logString+=String( (uint16_t)address, HEX);
      logString+=String(F(" Len: "));
      logString+=String( Len);
       
     
        
    if (!Write) {
        logString+=("... read");
        myOptolinkPlugin_182->readFromDP( address, Len);  // issue read command
        }
    else 
        {
          logString+=("... write");
         myOptolinkPlugin_182->writeToDP( address, Len, rawdata);
        }
         
    while (((millis()-now) < 3000)){
        myOptolinkPlugin_182->loop();
        if (myOptolinkPlugin_182->available() > 0) {
          myOptolinkPlugin_182->read(rawdata);
           
          logString+=(F(" data: "));
          addLog(LOG_LEVEL_DEBUG, logString);
          for (uint8_t i = 0; i < Len; ++i) {
            if ( rawdata[i] < 0x10) logString+=("0");
            logString+=String(rawdata[i], HEX);
           }
           return true; 
        }
        if (myOptolinkPlugin_182->available() < 0) {
          logString+="Optolink error: ";logString+=(myOptolinkPlugin_182->readError());
           addLog(LOG_LEVEL_ERROR,logString);
          return false; 
        }
  }
   logString+=F("Vito: Optolink unknown error. ");
   addLog(LOG_LEVEL_ERROR,logString);
  return false;
}
 


String retval=""; 
String retval_all="";
DPValue globalDPvalue(23.6f);

String RawToStr (String type, uint8* raw, uint8 readLen=0, double factor=0){
      //if (factor == 0) factor =1; 
      if (readLen==0 ) readLen = TypeStringToLen(type);
      if (readLen>10) {addLog(LOG_LEVEL_ERROR,"Readlen>10");  readLen=10;}
      if (type == "TempL"){ DPTemp temp_dp("TempL", "DPs", 0x00);               globalDPvalue = temp_dp.decode(raw);   if (factor!=0) retval = factor*globalDPvalue.getFloat(); else retval = globalDPvalue.getFloat();   } 
      else if (type == "TempS") {   DPTempS temps_dp("TempS", "DPs", 0x00);     globalDPvalue = temps_dp.decode(raw);  if (factor!=0) retval = factor*globalDPvalue.getU8();    else retval = globalDPvalue.getU8();  }
      else if (type == "STAT")  {   DPStat stat_dp("Stat", "DPs", 0x00);        globalDPvalue = stat_dp.decode(raw);     retval =  globalDPvalue.getBool() ? "true" : "false";      }
      else if (type == "CountL"){   DPCount count_dp("CountL", "DPs", 0x00);    globalDPvalue = count_dp.decode(raw);  if (factor!=0) retval = factor*globalDPvalue.getU32();   else retval = globalDPvalue.getU32();  }
      else if (type == "CountS"){   DPCountS counts_dp("CountS", "DPs", 0x00);  globalDPvalue = counts_dp.decode(raw); if (factor!=0) retval = factor*globalDPvalue.getU16();   else retval = globalDPvalue.getU16();   }
      else if (type == "Mode")  {   DPMode mode_dp("Mode", "DPs", 0x00);        globalDPvalue = mode_dp.decode(raw);   if (factor!=0) retval = factor*globalDPvalue.getU8();    else retval = globalDPvalue.getU8(); }
      else if (type == "Hours") {   DPHours hours_dp("Hours", "DPs", 0x00);     globalDPvalue = hours_dp.decode(raw);  if (factor!=0) retval = factor*globalDPvalue.getFloat(); else retval = globalDPvalue.getFloat(); }
      else if (type == "CoP")   {   DPCoP cop_dp("CoP", "DPs", 0x00);           globalDPvalue = cop_dp.decode(raw);    if (factor!=0) retval = factor*globalDPvalue.getFloat(); else retval = globalDPvalue.getFloat();}
      else if (type == "Raw")   {
        retval="";
        for (int i = 0; i<readLen; i++) {
          if (raw[i] < 0x10) retval +="0"; 
          retval += String(raw[i],HEX);
        }
      }
      else if (type == "all") {
        retval_all = "TempL:  " + RawToStr("TempL",raw,0,factor) +"\n";
        retval_all += "TempS:  " + RawToStr("TempS",raw,0,factor) +"\n";
        retval_all += "STAT:   " + RawToStr("STAT",raw,0,factor) +"\n";
        retval_all += "CountL: " + RawToStr("CountL",raw,0,factor) +"\n";
        retval_all += "CountS: " + RawToStr("CountS",raw,0,factor) +"\n";
        retval_all += "Mode:   " + RawToStr("Mode",raw,0,factor) +"\n";
        retval_all += "Hours:  " + RawToStr("Hours",raw,0,factor) +"\n";
        retval_all += "CoP:    " + RawToStr("CoP",raw,0,factor) +"\n";
        retval_all += "Raw:    " + RawToStr("Raw",raw, readLen ) +"\n";
        return retval_all;
      }
      
      else  retval = "unknown encoding requested";
      return retval;
}

void StrToRaw (String value, String type, uint8* raw ){
  
      for (int i = 0; i<10; i++) raw[i]=0;
      if (type == "TempL")      {   DPTemp temp_dp("TempL", "DPs", 0x00);       DPValue tempDP((float)atof(value.c_str())); temp_dp.encode(raw,tempDP);    } 
      else if (type == "TempS") {   DPTempS temps_dp("TempS", "DPs", 0x00);     DPValue tempDP((uint8_t)atoi(value.c_str())); temps_dp.encode(raw,tempDP);   }
      else if (type == "STAT")  {   DPStat stat_dp("Stat", "DPs", 0x00);        DPValue tempDP((bool )atoi(value.c_str())); stat_dp.encode(raw,tempDP);    }
      else if (type == "CountL"){   DPCount count_dp("CountL", "DPs", 0x00);    DPValue tempDP((uint32_t)atoi(value.c_str())); count_dp.encode(raw,tempDP);   }
      else if (type == "CountS"){   DPCountS counts_dp("CountS", "DPs", 0x00);  DPValue tempDP((uint16_t)atoi(value.c_str())); counts_dp.encode(raw,tempDP);  }
      else if (type == "Mode")  {   DPMode mode_dp("Mode", "DPs", 0x00);        DPValue tempDP((uint8_t)atoi(value.c_str())); mode_dp.encode(raw,tempDP);    }
      else if (type == "Hours") {   DPHours hours_dp("Hours", "DPs", 0x00);     DPValue tempDP((uint16_t)atoi(value.c_str())); hours_dp.encode(raw,tempDP);   }
      else if (type == "CoP")   {   DPCoP cop_dp("CoP", "DPs", 0x00);           DPValue tempDP((uint8_t)atoi(value.c_str())); cop_dp.encode(raw,tempDP);     }
      
}


byte TypeStringToLen (String type){
  byte readLen=4;   // for  CountL, Hours and Raw
      if      ( (type == "TempL") || (type == "CountS"))       readLen = 2;
      else if ( (type == "STAT" ) || (type == "TempS")  || (type == "Mode") || (type == "CoP") ) readLen = 1;
  return readLen;
}

uint8_t out_str[10] = {0};
String type, message = "";

void webPage_read() { // webpage "read" Handler 
    unsigned int address, readLen;
    double factor=0;
    if (WebServer.args() < 2)              { WebServer.send(200, "text/plain", F("You must specify at least 2 arguments, e.g. http://192.168.1.18/read?DP=0x2000&Type=TempL")); return;}
    if (WebServer.arg("DP") =="")          { WebServer.send(200, "text/plain", F("DP argument not found")); return;}
    if (WebServer.arg("Type") =="")        { WebServer.send(200, "text/plain", F("Type argument not found Use &Type=TempL|TempS|CountL|CountS|Mode|Hours|CoP|Raw|all")); return;}
    if (((WebServer.arg("Type") =="Raw") ||(WebServer.arg("Type") =="all"))
     && (WebServer.arg("ReadLen") =="" ))  { WebServer.send(200, "text/plain", F("No ReadLen specified for Type Raw or all")); return;}
    if (((WebServer.arg("Type") !="Raw")
     &&(WebServer.arg("Type") !="all")) 
     && (WebServer.arg("ReadLen") !="" )) { WebServer.send(200, "text/plain",  F("Do not specify a ReadLen unless you use Raw or all")); return;}
    
    if (WebServer.arg("factor") !="") { factor = atof(WebServer.arg("factor").c_str());  }
    
    address = strtol(WebServer.arg("DP").c_str(),0,16); // interpret DP adress as hex
    type=WebServer.arg("Type");  
    
    if ((type == "Raw") || (type == "all") )
        readLen = WebServer.arg("ReadLen").toInt();  
     else
        readLen = TypeStringToLen(WebServer.arg("Type")); 
        
    if (readLen>10)  { WebServer.send(200, "text/plain", F("Do not read more than 10 bytes")); return;}
   
    if (readWriteDP(address,readLen,out_str,false)){ // false = read
        message = RawToStr(type,out_str,readLen,factor);
        
        if  (WebServer.arg("MQTTTopic") !="")  {
          MQTTclient.publish(WebServer.arg("MQTTTopic").c_str(), message.c_str());
          }
    } else  message = "timeout";
    
    logString+=F(" Value ");
    logString += message; 
    addLog(LOG_LEVEL_INFO,logString);
    message += "\n";  
    
    //Serial1.println(message);
    WebServer.send(200, "text/plain", message);//Response to the HTTP request
} 

void webPage_write() { // webpage "read" Handler 
    unsigned int address, writeLen;
    if (WebServer.args() < 2)              { WebServer.send(200, "text/plain", F("You must specify at least 2 arguments, e.g. http://192.168.1.18/read?DP=0x2000&Type=TempL")); return;}
    if (WebServer.arg("DP") =="")          { WebServer.send(200, "text/plain", F("DP argument not found")); return;}
    if (WebServer.arg("Type") =="")        { WebServer.send(200, "text/plain", F("Type argument not found Use &Type=TempL|TempS|CountL|CountS|Mode|Hours|CoP|Raw|all")); return;}
    if (WebServer.arg("Value") =="")       { WebServer.send(200, "text/plain", F("Write value not specified")); return;}
    if (WebServer.arg("Type") =="Raw")     { WebServer.send(200, "text/plain", F("Writing Raw is not supported. Use Type=TempL|TempS|CountL|CountS|Mode|Hours|CoP")); return;}
      
    address = strtol(WebServer.arg("DP").c_str(),0,16); // interpret DP adress as hex
    type=WebServer.arg("Type");  
    writeLen = TypeStringToLen(WebServer.arg("Type")); 
    StrToRaw (WebServer.arg("Value"), WebServer.arg("Type"), out_str);
   
    message = RawToStr("all",out_str,writeLen); 
    
    if (readWriteDP(address,writeLen,out_str,true)) // true means write
        message += "write success\n";
    else
         message += "write fail\n";

    logString+=F(" Value ");
    logString += RawToStr(type,out_str,writeLen); ; 
    addLog(LOG_LEVEL_INFO,logString);
 
    message += "\n";  
    WebServer.send(200, "text/plain", message);//Response to the HTTP request
} 
 
 

//==============================================
// PLUGIN
// =============================================

boolean Plugin_096(byte function, struct EventStruct *event, String& string) {
	boolean success = false;
 
	switch (function)
	{
	case PLUGIN_TEN_PER_SECOND:
	{
 		if (myOptolinkPlugin_182 != NULL ) myOptolinkPlugin_182->loop();
		break;
	}

	case PLUGIN_SERIAL_IN:
	{
		break;
	}
	case PLUGIN_DEVICE_ADD:
	{
		Device[++deviceCount].Number = PLUGIN_ID_096;
		Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
		Device[deviceCount].VType = SENSOR_TYPE_NONE;
		Device[deviceCount].Ports = 0;
		Device[deviceCount].PullUpOption = false;
		Device[deviceCount].InverseLogicOption = false;
		Device[deviceCount].FormulaOption = true;
		Device[deviceCount].ValueCount = 0;
		Device[deviceCount].SendDataOption = false;
		Device[deviceCount].TimerOption = false;
		Device[deviceCount].GlobalSyncOption = false;
		break;
	}

	case PLUGIN_GET_DEVICENAME:
	{
		string = F(PLUGIN_NAME_096);
		break;
	}

	case PLUGIN_GET_DEVICEVALUENAMES:
	{
		//strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_182));
		//strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_182));
		//strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_182));
		break;
	}

	case PLUGIN_WEBFORM_LOAD:
	{
    //addFormCheckBox(string, F("swap serial"), F("plugin_182_swapSerial"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
    if (Settings.UseSerial) addHtmlError(F("please disable Enable Serial Port in tools/advanced"));
		success = true;
		break;
	}

	case PLUGIN_WEBFORM_SAVE:
	{
    //Settings.TaskDevicePluginConfig[event->TaskIndex][0] = isFormItemChecked(F("plugin_182_swapSerial"));
		success = true;
		break;
	}

	case PLUGIN_INIT:
	{
  LoadTaskSettings(event->TaskIndex);
	if (!Settings.UseSerial)
		{
    Serial.begin(115200); 
    Serial.println("\n\init..\n\n");
    Serial.flush();
    Serial.swap(); 
    pinMode(TX,INPUT);
    Serial1.begin(115200);  Serial1.println("Serial1 rerouted...");
    if (myOptolinkPlugin_182)
      delete myOptolinkPlugin_182;
    myOptolinkPlugin_182 = new OptolinkP300();
      
    WebServer.on("/read", webPage_read);
    WebServer.on("/write", webPage_write);
    myOptolinkPlugin_182->begin(&Serial);
    //myOptolinkPlugin_182->setLogger(&Serial1);
    Serial.swap(); 
    pinMode(TX,INPUT);
		}
   
		success = true;
		break;
	}
	case PLUGIN_READ:
	{
	  return success;
		break;
	}

	}
	return success;
}




