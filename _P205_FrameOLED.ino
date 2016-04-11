//#######################################################################################################
//#################################### Plugin 205: OLED SSD1306 display #################################
//## This is a modification to Plugin_023 with graphics library provided from squix78 github ############
//#### https://github.com/squix78/esp8266-oled-ssd1306
//
// New init overload added to above library in order to enable init of the oled library within the INIT
// call to this plugin.
//
// The OLED can display up to 12 strings in four frames - ie 12 frames with 1 line, 6 with 2 lines or 3 with 4 lines.
// The font size is adjsted according to the number of lines required per frame.
//
// The usual parseTemplate routine is used so that strings like %systime% or [DHT#Temperature] are parsed properly
//
//#################################### Version 0.3 12-May-2016 ##########################################
//#######################################################################################################

#define PLUGIN_205
#define PLUGIN_ID_205         205
#define PLUGIN_NAME_205       "Display - OLED SSD1306 Framed"
#define PLUGIN_VALUENAME1_205 "OnOff"
#define PLUGIN_VALUENAME2_205 "Display"
#define Nlines 12				// The number of different lines which can be displayed - each line is 32 chars max

// The line below defines the dummy function PLUGIN_COMMAND which is only for Namirda use

#ifndef PLUGIN_COMMAND
#define PLUGIN_COMMAND 999
#endif     

#include "SSD1306.h"
#include "images.h"

// Instantiate display here - does not work to do this within the INIT call

SSD1306 display(0,0,0); // The parameters here are dummy only. The real i2c address is provided by local init override

boolean Plugin_205(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte displayTimer = 0;
  static byte frameCounter=0;   // need to keep track of framecounter from call to call
  static bool firstcall=true;   // This is used to clear the init graphic on the first call to read
  
  int linesPerFrame;     // the number of lines in each frame
  int NFrames;			// the number of frames
  
  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_205;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_205);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_205));  // OnOff
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_205));  // Display
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[2];
        options[0] = F("3C");
        options[1] = F("3D");
        int optionValues[2];
        optionValues[0] = 0x3C;
        optionValues[1] = 0x3D;
        string += F("<TR><TD>I2C Address:<TD><select name='plugin_205_adr'>");
        for (byte x = 0; x < 2; x++)
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

        byte choice2 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String options2[2];
        options2[0] = F("Normal");
        options2[1] = F("Rotated");
        int optionValues2[2];
        optionValues2[0] = 1;
        optionValues2[1] = 2;
        string += F("<TR><TD>Rotation:<TD><select name='plugin_205_rotate'>");
        for (byte x = 0; x < 2; x++)
        {
          string += F("<option value='");
          string += optionValues2[x];
          string += "'";
          if (choice2 == optionValues2[x])
            string += F(" selected");
          string += ">";
          string += options2[x];
          string += F("</option>");
        }
        string += F("</select>");

        byte choice3 = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        String options3[4];
        options3[0] = F("1");
        options3[1] = F("2");
		options3[2] = F("3");
        options3[3] = F("4");
        int optionValues3[4];
        optionValues3[0] = 1;
        optionValues3[1] = 2;
		optionValues3[2] = 3;
		optionValues3[3] = 4;
        string += F("<TR><TD>Lines per Frame:<TD><select name='plugin_205_nlines'>");
        for (byte x = 0; x < 4; x++)
        {
          string += F("<option value='");
          string += optionValues3[x];
          string += "'";
          if (choice3 == optionValues3[x])
            string += F(" selected");
          string += ">";
          string += options3[x];
          string += F("</option>");
        }
        string += F("</select>");

        char deviceTemplate[Nlines][32];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        for (byte varNr = 0; varNr < Nlines; varNr++)
        {
          string += F("<TR><TD>Line ");
          string += varNr + 1;
          string += F(":<TD><input type='text' size='32' maxlength='32' name='Plugin_205_template");
          string += varNr + 1;
          string += F("' value='");
          string += deviceTemplate[varNr];
          string += F("'>");
        }

        string += F("<TR><TD>Display button:<TD>");
        addPinSelect(false, string, "taskdevicepin3", Settings.TaskDevicePin3[event->TaskIndex]);

        char tmpString[128];

        sprintf_P(tmpString, PSTR("<TR><TD>Display Timeout:<TD><input type='text' name='plugin_205_timer' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
        string += tmpString;

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

		String plugin1 = WebServer.arg("plugin_205_adr");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_205_rotate");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        String plugin3 = WebServer.arg("plugin_205_nlines");
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();
		String plugin4 = WebServer.arg("plugin_205_timer");
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = plugin4.toInt();

        char deviceTemplate[Nlines][32];

		String argName;

        for (byte varNr = 0; varNr < Nlines; varNr++)
        {
          argName = F("Plugin_205_template");
          argName += varNr + 1;
          strncpy(deviceTemplate[varNr], WebServer.arg(argName).c_str(), sizeof(deviceTemplate[varNr]));
        }

        Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      { 

        int OLED_address = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

//      Init the display and turn it on

        display.init(OLED_address);		// call to local override of init function
        display.displayOn();

//      Display the device name and the logo

        display_espname();
        display_logo();
        display.display();

//      flip screen if required

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][1] == 2)display.flipScreenVertically();

//      Handle display timer

        displayTimer = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1){
          pinMode(Settings.TaskDevicePin3[event->TaskIndex], INPUT_PULLUP);
        }
        
//      Set the initial value of OnOff to On

        UserVar[event->BaseVarIndex]=1;      //  Save the fact that the initial state of the display is ON
        UserVar[event->BaseVarIndex+1]=99;   // Dummy Initial Value for Display
        
        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          if (!digitalRead(Settings.TaskDevicePin3[event->TaskIndex]))
          {
            display.displayOn();
            displayTimer = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
          }
        }
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if ( displayTimer > 0)
        {
          displayTimer--;
          if (displayTimer == 0)
            display.displayOff();
        }
        success=true;
        break;
      }

    case PLUGIN_READ:
      {
        
        if (firstcall) {
          display.clear(); // get rid of the init screen if this is the first call
          firstcall=false;
        }

        //      Define Scroll area layout

        linesPerFrame=Settings.TaskDevicePluginConfig[event->TaskIndex][2];  
        NFrames=Nlines/linesPerFrame;
                
        char deviceTemplate[Nlines][32];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

//      Now create the string for the outgoing and incoming frames

        String tmpString[4];
        String newString[4];
        String oldString[4];

//      Construct the outgoing string

        for (byte i = 0; i <linesPerFrame; i++) { 
          tmpString[i] = deviceTemplate[(linesPerFrame*frameCounter)+i];
          oldString[i] = parseTemplate(tmpString[i], 20);
          oldString[i].trim();
        }

       // now loop round looking for the next frame with some content
       
        int tlen=0;
        int ntries=0;
        while(tlen == 0){

//        Stop after framecount loops if no data found

          ntries+=1;
          if (ntries > NFrames) break;

//        Increment the frame counter

          frameCounter=frameCounter+1;
          if ( frameCounter > NFrames-1) frameCounter=0;

//        Contruct incoming strings

          for (byte i = 0; i <linesPerFrame; i++) {  
            tmpString[i] = deviceTemplate[(linesPerFrame*frameCounter)+i];          
            newString[i] = parseTemplate(tmpString[i], 20);
            newString[i].trim();
          }

//      skip this frame if all lines in frame are blank - we exit the while loop if tlen is not zero

          tlen=0;
          for (byte i = 0; i <linesPerFrame; i++) {
            tlen+=newString[i].length();
          }
        }

//      Update display

        display_time();

		int nbars=(WiFi.RSSI()+100)/8;
		display_wifibars(105,0,15,10,5,nbars);

        display_espname();
        display_indicator(frameCounter,NFrames);
        display.display();
        
        display_scroll(oldString,newString,linesPerFrame);

        success = false;    // If we do not set false then a value is output each read
        break;
      }

    case PLUGIN_WRITE:
      {
        String tmpString  = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex)
          tmpString = tmpString.substring(0, argIndex);
        if (tmpString.equalsIgnoreCase(F("OLED")))
        {
          success = true;
          argIndex = string.lastIndexOf(',');
          tmpString = string.substring(argIndex + 1);
          display.drawString(event->Par1 - 1, event->Par2 - 1, tmpString);

        }
        if (tmpString.equalsIgnoreCase(F("OLEDCMD")))
        {
          success = true;
          argIndex = string.lastIndexOf(',');
          tmpString = string.substring(argIndex + 1);
          if (tmpString.equalsIgnoreCase(F("Off")))
            display.displayOff();
          else if (tmpString.equalsIgnoreCase(F("On")))
            display.displayOn();
          else if (tmpString.equalsIgnoreCase(F("Clear")))
            display.clear();
        }
        break;
      }

//	This option PLUGIN_COMMAND only in use for local namirda version. It is disabled for general release

    case PLUGIN_COMMAND:
      {
        // This option is called when user has made a request to this task
        // We need to extract the second subtopic which is passed here as string
     
        String tmpString  = string;

        if (event->TaskIndex != ExtraTaskSettings.TaskIndex) LoadTaskSettings(event->TaskIndex);    //Load the task settings if required - used to get device names

//      Get the payload

        String Payload=event->String2;
        int intPayload=string2Integer(Payload);

//      Get the ValueNameIndex

        int ValueNameIndex=getValueNameIndex(event->TaskIndex,string);
        if (ValueNameIndex == 255){
          addLog(LOG_LEVEL_INFO,"Internal Error module 204");
          break;
        }
//      Deal with command to switch display on or off. 

        if ( ValueNameIndex == 0 )     // This is a command to switch display on and off
        {
          if ((intPayload != 0) && (intPayload != 1))
          {
            String log = F("ERR  : Illegal Value for [");
            log+=ExtraTaskSettings.TaskDeviceName;
            log+="#";
            log += ExtraTaskSettings.TaskDeviceValueNames[ValueNameIndex];
            log += "] - ";
            log += Payload;
            addLog(LOG_LEVEL_ERROR,log);
            break;
          }
          
          UserVar[event->BaseVarIndex+ValueNameIndex]=intPayload;  // Save the new value for off/on
          
//        And switch the display accordingly  
       
          if (intPayload == 0)display.displayOff();

          if (intPayload != 0)display.displayOn();                       
        } 

//      Now handle a string to the display
//      The required format is N:DisplayString  where N is an integer between 1 and 8

        else if ( ValueNameIndex == 1 )     // This is a string to Display
        {
          UserVar[event->BaseVarIndex+ValueNameIndex]=intPayload;  // Store some nonsense in UserVar - not useful because it can only handle floating point numbers.

 //       Now extract the Line Number and the Colon
 
          int Line=string2Integer(Payload.substring(0,1));
          if (! CheckParam("Line Number",Line,1,8)) {
            success=false;
            return success;
          }

          String colon=Payload.substring(1,2);
          if (colon != ":")
          {
            addLog(LOG_LEVEL_INFO,"ERR  : Syntax for Display Line is N:String");
            break;
          }

          String shortPayload=Payload.substring(2);                       // Remove the first two characters (N:)

          String tPayload=shortPayload;
          String DisplayString = parseTemplate(tPayload, 64);     // Parse Data - must use a temp tPayload because parseTemplate messes with the string!!!

//      Now we get the display lines from the custom settings

        char deviceTemplate[8][64];        
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

//      And display out the four different lines for logging

        for (byte varNr = 0; varNr < 8; varNr++)
        {
          String log=F("Display Line ");
          log+= varNr;
          log+= "  ";
          log+=deviceTemplate[varNr];
          addLog(LOG_LEVEL_DEBUG,log);
        }
               
        strncpy(deviceTemplate[Line-1], shortPayload.c_str(), sizeof(deviceTemplate[Line-1]));      // Here we are saving the unparsed value
        Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));             
        }

//      Cannot use function 'logupdates' here because value is a string!!

        if (event->TaskIndex != ExtraTaskSettings.TaskIndex) LoadTaskSettings(event->TaskIndex);    //Load the task settings if required - used to get device names
       
        String log = F("205  : [");
        log += ExtraTaskSettings.TaskDeviceName;
        log += "#";
        log += ExtraTaskSettings.TaskDeviceValueNames[ValueNameIndex];
        log += "] - set to ";
        log += event->String2;
        addLog(LOG_LEVEL_INFO,log);

        success=true;
    }
    
  }
  return success;
}

// The screen is set up as 10 rows at the top for the header, 10 rows at the bottom for the footer and 44 rows in the middle for the scroll region

void display_time(){
  String dtime="%systime%";
  String newString = parseTemplate(dtime, 10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.setColor(BLACK);
  display.fillRect(0, 0, 28, 10);
  display.setColor(WHITE);
  display.drawString(0, 0, newString.substring(0,5));
}

void display_espname(){
  String dtime="%sysname%";
  String newString = parseTemplate(dtime, 10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, newString);
}

void display_logo() {
  // draw an xbm image.
  display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

// Draw the frame position

void display_indicator(int iframe,int frameCount) {

//  Erase Indicator Area

  display.setColor(BLACK);
  display.fillRect(0, 54, 128, 10);
  display.setColor(WHITE);

 // Display chars as required
 
  for (byte i = 0; i < frameCount; i++) {
      const char *image;
      if (iframe == i) {
         image = activeSymbole;
      } else {
         image = inactiveSymbole;
      }

      int x,y;

      y = 56;

	  // I would like a margin of 20 pixels on each side of the indicator.
	  // Therefore the width of the indicator should be 128-40=88 and so space between indicator dots is 88/(framecount-1)
	  // The width of the display is 128 and so the first dot must be at x=20 if it is to be centred at 64

	  int margin = 20;
	  int spacing = (128 - 2 * margin) / (frameCount - 1);

	  x = margin + (spacing*i);
      display.drawXbm(x, y, 8, 8, image);
  }
}

void display_scroll(String outString[], String inString[], int nlines) {

// outString contains the outgoing strings in this frame 
// inString contains the incomng strings in this frame
// nlines is the number of lines in each frame

   int ypos[4]; // ypos contains the heights of the various lines - this depends on the font and the number of lines

   if (nlines == 1){
    display.setFont(ArialMT_Plain_24);
    ypos[0]=20;
   }
   
   if (nlines == 2){
    display.setFont(ArialMT_Plain_16);
    ypos[0]=15;
    ypos[1]=34;
   }

   if (nlines == 3) {
	   display.setFont(Dialog_Plain_12);
	   ypos[0] = 13;
	   ypos[1] = 25;
	   ypos[2] = 37;
   }

   if (nlines == 4){
    display.setFont(ArialMT_Plain_10);
    ypos[0]=12;
    ypos[1]=22;
    ypos[2]=32;
    ypos[3]=42;
   }

   display.setTextAlignment(TEXT_ALIGN_CENTER);

   for (byte i = 0; i <32; i++) {

//  Clear the scroll area

      display.setColor(BLACK);
	  // We allow 12 pixels at the top because otherwise the wifi indicator gets too squashed!!
      display.fillRect(0, 12, 128, 42);   // scrolling window is 44 pixels high - ie 64 less margin of 10 at top and bottom  
      display.setColor(WHITE);

// Now draw the strings

      for (byte j = 0; j<nlines; j++) {
        
        display.drawString(64+(4*i),ypos[j],outString[j]);

        display.drawString(-64+(4*i),ypos[j],inString[j]);
      }
      
      display.display();
      
      delay(2);
      

   }

}
//Draw Signal Strength Bars
void display_wifibars(int x, int y, int size_x, int size_y,int nbars,int nbars_filled) {

//	x,y are the x,y locations
//	sizex,sizey are the sizes (should be a multiple of the number of bars)
//	nbars is the number of bars and nbars_filled is the number of filled bars.

//	We leave a 1 pixel gap between bars

	for (byte ibar = 1; ibar < nbars + 1; ibar++) {

		display.setColor(BLACK);
		display.fillRect(x + (ibar - 1)*size_x / nbars, y, size_x/nbars, size_y);
		display.setColor(WHITE);

		if (ibar <= nbars_filled) {
			display.fillRect(x + (ibar - 1)*size_x / nbars, y + (nbars - ibar)*size_y / nbars, (size_x / nbars)-1, size_y*ibar / nbars);
		}
		else
		{
			display.drawRect(x + (ibar - 1)*size_x / nbars, y + (nbars - ibar)*size_y / nbars, (size_x / nbars)-1, size_y*ibar / nbars);
		}
	}
}

