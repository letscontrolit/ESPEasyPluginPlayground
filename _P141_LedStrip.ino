/* #######################################################################################
############################### Plugin 141: LedStrip #####################################
##########################################################################################

Features :
	- Controls basic RGB Led Strips
	- [TODO] Controls basic RGBW(W) Led Strips
	- [TODO] Controls various "Pixels" Led Strips
	- RGB or HSV commands
	- Various Animations mode
	- [TODO] Supports Infra-Red Remotes
	- Smooth Diming (thanks to FastLED lib)
	- Rainbow color map (thanks to FastLED lib)

Installation :
	- Add the FastLED library in the 'src/lib/' folder from https://github.com/FastLED/FastLED/tree/3.1.8
	- Move this file to the 'src/' folder
	- Add "#include <FastLED.h>" at the start of src/lib/ESPEasy.ino file else compiler fails
	
List of commands :
	- ON
	- OFF
	- RGB,<red 0-255>,<green 0-255>,<blue 0-255>
	- HSV,<hue 0-255>,<saturation 0-255>,<value/brightness 0-255>
	- HUE,<hue 0-360>
	- SAT,<saturation 0-100>
	- VAL,<value/brightness 0-1023>
	- DIM,<value/brightness 0-1023>
	- H_RGB,<RGB HEX COLOR > ie #FF0000 for red
	- H_HSV,<HSV HEX COLOR > ie #00FFFF for red
	- SPEED,<0-65535> Fast to slow
	- MODE,<mode 0-6>,<Speed 1-255>	time for full color hue circle;
		Available  Modes:
			* 0 : OFF
			* 1 : ON
			* 2 : Flash
			* 3 : Strobe
			* 4 : Fade
			* 5 : Smooth
			* 6 : Party 

Command Examples :
	-  /control?cmd=ON					Turn Leds On
	-  /control?cmd=OFF					Turn Leds Off
	-  /control?cmd=RGB,255,0,0			Set Leds to Red
	-  /control?cmd=HSV,0,255,255		Set Leds to Red
	-  /control?cmd=Mode,2				Animate Leds in "Flash" mode
	-  /control?cmd=Mode,5,400			Animate Leds in "Smooth" mode, with a speed of 400

Recommended Hardware :
	Works out of the box on theses cheap hackable chinese devices :
	- Huacanxing H801
		Product: https://www.google.fr/search?q=Huacanxing+H801&tbm=isch 
		Flash  : http://tinkerman.cat/closer-look-h801-led-wifi-controller/	
	- Magic Home	
		Product: https://www.google.fr/search?q=magic+home+led+controller&tbm=isch 
		Flash  : https://github.com/xoseperez/espurna/wiki/Hardware#magic-home-led-controller

------------------------------------------------------------------------------------------
	Copyright Francois Dechery 2017 - https://github.com/soif/
------------------------------------------------------------------------------------------
*/



// #### Includes #########################################################################
// ESP-PWM has flickering problems with values <6 and >1017. If problem is fixed in ESP libs the define can be set to 0 (or code removed)
// see https://github.com/esp8266/Arduino/issues/836		https://github.com/SmingHub/Sming/issues/70		https://github.com/espruino/Espruino/issues/914

//#define FASTLED_FORCE_SOFTWARE_SPI
//#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
//FASTLED_USING_NAMESPACE ;

//#include <IRremoteESP8266.h>


// #### Defines ##########################################################################
#define PLUGIN_141
#define PLUGIN_ID_141			141
#define PLUGIN_NAME_141			"Output - LedStrip"

#define PLUGIN_141_CONF_0		"strip_type"
#define PLUGIN_141_CONF_1		"strip_pin1"
#define PLUGIN_141_CONF_2		"strip_pin2"
#define PLUGIN_141_CONF_3		"strip_pin3"
#define PLUGIN_141_CONF_4		"strip_pin4"
#define PLUGIN_141_CONF_5		"strip_pin5"
#define PLUGIN_141_CONF_6		"strip_pin_ir"

#define PLUGIN_141_VALUENAME_0	"Hue"
#define PLUGIN_141_VALUENAME_1	"Sat"
#define PLUGIN_141_VALUENAME_2	"Val"
#define PLUGIN_141_VALUENAME_3	"Mode"

#define PLUGIN_141_LOGPREFIX	"LedStrip: "

#define PLUGIN_141_GPIO_LAST	16		// last GPIO available

#define PLUGIN_141_PIN_1		1		// first setting holding pins
#define PLUGIN_141_PINS_COUNT	5		// max number of pins

#define PLUGIN_141_FIRST_TYPE_PIX		11	// first strip type which is a "pixels" one
#define PLUGIN_141_FIRST_TYPE_PIX_SPI	31	// first strip type which is a "SPI pixels" one

// IR Buttons -------------------------------------
#ifndef PLUGIN_141_CUSTOM_BUT
	#define PLUGIN_141_IR_BUT_0  0xFF906F // Brightness +
	#define PLUGIN_141_IR_BUT_1  0xFFB847 // Brightness -
	#define PLUGIN_141_IR_BUT_2  0xFFF807 // OFF
	#define PLUGIN_141_IR_BUT_3  0xFFB04F // ON

	#define PLUGIN_141_IR_BUT_4  0xFF9867 // RED
	#define PLUGIN_141_IR_BUT_5  0xFFD827 // GREEN
	#define PLUGIN_141_IR_BUT_6  0xFF8877 // BLUE
	#define PLUGIN_141_IR_BUT_7  0xFFA857 // WHITE

	#define PLUGIN_141_IR_BUT_8  0xFFE817 // "Red" 1
	#define PLUGIN_141_IR_BUT_9  0xFF48B7 // "Green" 1
	#define PLUGIN_141_IR_BUT_10 0xFF6897 // "Blue" 1
	#define PLUGIN_141_IR_BUT_11 0xFFB24D // FLASH Mode

	#define PLUGIN_141_IR_BUT_12 0xFF02FD // "Red" 2
	#define PLUGIN_141_IR_BUT_13 0xFF32CD // "Green" 2
	#define PLUGIN_141_IR_BUT_14 0xFF20DF // "Blue" 2
	#define PLUGIN_141_IR_BUT_15 0xFF00FF // STROBE Mode

	#define PLUGIN_141_IR_BUT_16 0xFF50AF // "Red" 3
	#define PLUGIN_141_IR_BUT_17 0xFF7887 // "Green" 3
	#define PLUGIN_141_IR_BUT_18 0xFF708F // "Blue" 3
	#define PLUGIN_141_IR_BUT_19 0xFF58A7 // FADE Mode

	#define PLUGIN_141_IR_BUT_20 0xFF38C7 // "Red" 4
	#define PLUGIN_141_IR_BUT_21 0xFF28D7 // "Green" 4
	#define PLUGIN_141_IR_BUT_22 0xFFF00F // "Blue" 4
	#define PLUGIN_141_IR_BUT_23 0xFF30CF // SMOOTH Mode
#endif
#define PLUGIN_141_IR_BUTTONS_COUNT 24

// Animations -------------------------------------
#define PLUGIN_141_FIRST_ANIM_MODE	 	2		// first mode which is an animation
#define PLUGIN_141_MODES_TOTAL 			(2 + 5)	// total number of animation mode

#define PLUGIN_141_ANIM_FLASH_SPEED		350		// flash ON Variable
#define PLUGIN_141_ANIM_FLASH_PAUSE		200		// flash OFF fixed

#define PLUGIN_141_ANIM_STROBE_SPEED	550		// strobe OFF variable
#define PLUGIN_141_ANIM_STROBE_PAUSE	150		// storbe ON fixed

#define PLUGIN_141_ANIM_FADE_SPEED		100		// fade speed
#define PLUGIN_141_ANIM_SMOOTH_SPEED	700		// smooth speed
#define PLUGIN_141_ANIM_PARTY_SPEED		200		// party speed


// #### Variables ########################################################################
unsigned long v_p141_but_codes[]={		// IR remote buttons codes
	PLUGIN_141_IR_BUT_0  , //	Brightness +
	PLUGIN_141_IR_BUT_1  , //	Brightness -
	PLUGIN_141_IR_BUT_2  , //	OFF
	PLUGIN_141_IR_BUT_3  , //	ON

	PLUGIN_141_IR_BUT_4  , //	Red
	PLUGIN_141_IR_BUT_5  , //	Green
	PLUGIN_141_IR_BUT_6  , //	Blue
	PLUGIN_141_IR_BUT_7  , //	White

	PLUGIN_141_IR_BUT_8  , //	R1
	PLUGIN_141_IR_BUT_9  , //	G1
	PLUGIN_141_IR_BUT_10 , //	B1
	PLUGIN_141_IR_BUT_11 , //	Flash

	PLUGIN_141_IR_BUT_12 , //	R2
	PLUGIN_141_IR_BUT_13 , //	G2
	PLUGIN_141_IR_BUT_14 , //	B2

	PLUGIN_141_IR_BUT_15 , //	Strobe
	PLUGIN_141_IR_BUT_16 , //	R3
	PLUGIN_141_IR_BUT_17 , //	G3
	PLUGIN_141_IR_BUT_18 , //	B3

	PLUGIN_141_IR_BUT_19 , //	Fade
	PLUGIN_141_IR_BUT_20 , //	R4
	PLUGIN_141_IR_BUT_21 , //	G4
	PLUGIN_141_IR_BUT_22 , //	B4
	PLUGIN_141_IR_BUT_23	//	Smooth
};

unsigned long v_p141_but_colors[]={	// IR remote buttons colors
	0,			//	Brightness +
	0,			//	Brightness -
	0,			//	OFF
	0,			//	ON
	0xFF0000,	//	Red
	0x00FF00,	//	Green
	0x0000FF,	//	Blue
	0xFFFFFF,	//	White
	0xD13A01,	//	R1
	0x00E644,	//	G1
	0x0040A7,	//	B1
	0,			//	Flash
	0xE96F2A,	//	R2
	0x00BEBF,	//	G2
	0x56406F,	//	B2
	0,			//	Strobe
	0xEE9819,	//	R3
	0x00799A,	//	G3
	0x944E80,	//	B3
	0,			//	Fade
	0xFFFF00,	//	R4
	0x0060A1,	//	G4
	0xEF45AD,	//	B4
	0			//	Smooth
};

static int		v_p141_pins[5]				= {-1, -1, -1, -1, -1};
static int		v_p141_pin_ir				= -1;
static int		v_p141_pin_inverse			= false;
CHSV 			v_p141_cur_color			= CHSV(0,255,255);
CHSV 			v_p141_cur_anim_color		= CHSV(0,255,255);
byte 			v_p141_cur_strip_type 		= 0 ;
byte 			v_p141_cur_anim_mode  		= 0 ;
byte 			v_p141_cur_anim_step  		= 0;
boolean 		v_p141_cur_anim_dir	  		= true;
unsigned long	v_p141_cur_anim_speed 		= 1000;
unsigned long	v_p141_anim_last_update 	= millis();
unsigned long	v_p141_last_ir_button		= 0;


// #######################################################################################

boolean Plugin_141 (byte function, struct EventStruct *event, String& string)
{

	boolean success = false;

	switch (function) {

		case PLUGIN_DEVICE_ADD:
		{
			Device[++deviceCount].Number		= PLUGIN_ID_141;
			Device[deviceCount].Type			= DEVICE_TYPE_DUMMY;						// Nothing else really fit the bill ...
			Device[deviceCount].Ports			= 0;
			Device[deviceCount].VType			= SENSOR_TYPE_QUAD;
			Device[deviceCount].PullUpOption	= false;
			Device[deviceCount].InverseLogicOption = true;
			Device[deviceCount].FormulaOption	= true;
			Device[deviceCount].ValueCount		= 4;
			Device[deviceCount].SendDataOption	= true;
			Device[deviceCount].TimerOption		= true;
			Device[deviceCount].TimerOptional	= true;
			Device[deviceCount].GlobalSyncOption= true;
			break;
		}

		case PLUGIN_GET_DEVICENAME:
		{
			string = F(PLUGIN_NAME_141);
			break;
		}

		case PLUGIN_GET_DEVICEVALUENAMES:
		{
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_141_VALUENAME_0));
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_141_VALUENAME_1));
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_141_VALUENAME_2));
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_141_VALUENAME_3));
			break;
		}

		case PLUGIN_WEBFORM_LOAD:
		{

			// type selector .............................
			String options[17];
			int optionValues[17];

			byte i=0;

			options[i] 			= F("-- Basic ----------------------");
			optionValues[i++]	= 0;

			options[i] 			= F("RGB");
			optionValues[i++]	= 1;

			options[i] 			= F("RGB + IR");
			optionValues[i++]	= 2;

			options[i]			= F("RGBW");
			optionValues[i++]	= 3;

			options[i]			= F("RGBW + IR");
			optionValues[i++]	= 4;

			options[i]			= F("RGBWW");
			optionValues[i++]	= 5;

			options[i]			= F("RGBWW + IR");
			optionValues[i++]	= 6;

			options[i] 			= F("-- Pixels : 1 pin  ------------");
			optionValues[i++]	= 0;

			options[i]			= F("NEOPIXEL");
			optionValues[i++]	= 11;

			options[i]			= F("TM1809 / TM1812");
			optionValues[i++]	= 12;

			options[i]			= F("WS2811");
			optionValues[i++]	= 13;

			options[i]			= F("WS2812 / WS2812B / WS2852");
			optionValues[i++]	= 14;

			options[i] 			= F("-- Pixels : 2 pins (SPI) -------");
			optionValues[i++]	= 0;

			options[i]			= F("SPI: APA102/DOTSTAR");
			optionValues[i++]	= 31;

			options[i]			= F("SPI: LPD8806");
			optionValues[i++]	= 32;

			options[i]			= F("SPI: P9813");
			optionValues[i++]	= 33;

			options[i]			= F("SPI: WS2801");
			optionValues[i++]	= 34;


			byte type = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
			addFormSelector(F("LedStrip Type"), PLUGIN_141_CONF_0, i , options, optionValues, NULL ,type, true );


			// show options depending on type selected ..........
			//led Strip
			if( type> 0 && type < PLUGIN_141_FIRST_TYPE_PIX ){
				// RGB
				addRowLabel("GPIO Red");
				addPinSelect(false, PLUGIN_141_CONF_1, Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
			
				addRowLabel("GPIO Green");
				addPinSelect(false, PLUGIN_141_CONF_2, Settings.TaskDevicePluginConfig[event->TaskIndex][2]);

				addRowLabel("GPIO Blue");
				addPinSelect(false, PLUGIN_141_CONF_3, Settings.TaskDevicePluginConfig[event->TaskIndex][3]);

				// RGBW
				if( type >=3 ){
					addRowLabel("GPIO White 1");
					addPinSelect(false, PLUGIN_141_CONF_4, Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
				}
				
				// RGBWW
				if( type >=5 ){
					addRowLabel("GPIO White 2");
					addPinSelect(false, PLUGIN_141_CONF_5, Settings.TaskDevicePluginConfig[event->TaskIndex][5]);
				}
				
				// has IR				
				if( type ==2 || type == 4 || type == 6 ){
					addRowLabel("GPIO InfraRed");
					addPinSelect(false, PLUGIN_141_CONF_6, Settings.TaskDevicePluginConfig[event->TaskIndex][6]);
				}

				addFormSeparator(2);
				addHtml(F("<TR><TD>Huacanxing H801 pins:</TD><TD>15, 13, 12, 14, 4 - normal<TD>"));			
				addHtml(F("<TR><TD>Magic Home v1.0 pins:</TD><TD>14,  5, 12, 13, IR=? - Invers<TD>"));			
				addHtml(F("<TR><TD>Magic Home v2.0 pins:</TD><TD> 5, 12, 13, 15, IR=4 - Invers<TD>"));			
			}
			else if( type >= PLUGIN_141_FIRST_TYPE_PIX ){
				addHtml(F("<TR><TD><b style='color:red'>NOT yet implemented</b><TD>"));			
				
				if (type < PLUGIN_141_FIRST_TYPE_PIX_SPI){
					addRowLabel("GPIO Data");
					addPinSelect(false, PLUGIN_141_CONF_1, Settings.TaskDevicePluginConfig[event->TaskIndex][1]);			
				}
				else {
					addHtml(F("<TR><TD>Use hardware SPI GPIOs<TD>"));
				}
			}

			success = true;
			break;
		}

		case PLUGIN_WEBFORM_SAVE:
		{
        	Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F(PLUGIN_141_CONF_0));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F(PLUGIN_141_CONF_1));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F(PLUGIN_141_CONF_2));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F(PLUGIN_141_CONF_3));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][4] = getFormItemInt(F(PLUGIN_141_CONF_4));
	        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = getFormItemInt(F(PLUGIN_141_CONF_5));
	        Settings.TaskDevicePluginConfig[event->TaskIndex][6] = getFormItemInt(F(PLUGIN_141_CONF_6));

			// reset invalid pins
			for (byte i=PLUGIN_141_PIN_1; i < (PLUGIN_141_PIN_1 + PLUGIN_141_PINS_COUNT) ; i++){
				if (Settings.TaskDevicePluginConfig[event->TaskIndex][i] > PLUGIN_141_GPIO_LAST){
					Settings.TaskDevicePluginConfig[event->TaskIndex][i] = -1;
				}
			}

			success = true;
			break;
		}

		case PLUGIN_INIT:
		{
			analogWriteFreq(400);

			// start IR Receivers ..........
			//IRrecv v_p141_ir_recv(v_p141_pin_ir); 		//IRrecv _ir_recv(v_p141_pin_ir, IR_LED_PIN); dont work. Why ?
			//decode_results v_p141_ir_results;


			// assign pins .................
			String log = F(PLUGIN_141_LOGPREFIX);
			log += F("Pins ");

			for (byte i=PLUGIN_141_PIN_1; i < (PLUGIN_141_PIN_1 + PLUGIN_141_PINS_COUNT) ; i++)	{
				int pin = Settings.TaskDevicePluginConfig[event->TaskIndex][i];
				v_p141_pins[i - PLUGIN_141_PIN_1] = pin;
				if (pin >= 0){
					pinMode(pin, OUTPUT);
				}
				log += pin;
				log += F(" ");
			}

			v_p141_pin_ir		= Settings.TaskDevicePluginConfig[event->TaskIndex][6];
			v_p141_pin_inverse	= Settings.TaskDevicePin1Inversed[event->TaskIndex];
			
			//if(v_p141_pin_inverse){log += F("INVERT");}else{log += F("NORM");}
			addLog(LOG_LEVEL_INFO, log);

			success = true;
			break;
		}

		case PLUGIN_WRITE:
		{
			String command = parseString(string, 1);

			if (command == F("off"))	{
				Fp141_CommandOff();
				success = true;
			}

			if (command == F("on"))	{
				Fp141_CommandOn();
				success = true;
			}

			if (command == F("rgb"))	{
				Fp141_SetCurrentColor( Fp141_RgbToHSV(CRGB(event->Par1, event->Par2, event->Par3)));
				Fp141_OutputCurrentColor();
				success = true;
			}

			if (command == F("hsv"))	{
				Fp141_SetCurrentColor(CHSV (event->Par1, event->Par2, event->Par3));
				Fp141_OutputCurrentColor();
				success = true;
			}

			if (command == F("hue"))	{
				v_p141_cur_color.h	= event->Par1 ;	 //Hue
				Fp141_SetCurrentColor(v_p141_cur_color);
				Fp141_OutputCurrentColor();
				success = true;
			}

			if (command == F("sat"))	{
				v_p141_cur_color.s 	= event->Par1 ;	 //Saturation
				Fp141_SetCurrentColor(v_p141_cur_color);
				Fp141_OutputCurrentColor();
				success = true;
			}

			if (command == F("val") || command == F("dim"))	{
				v_p141_cur_color.v 	= event->Par1 ;	 //Value/Brightness
				Fp141_SetCurrentColor(v_p141_cur_color);
				Fp141_OutputCurrentColor();
				success = true;
			}

			if (command == F("speed"))	{
				v_p141_cur_anim_speed = event->Par1 ;
				success = true;
			}

			if (command == F("h_rgb"))	{
				String color = parseString(string, 2);
				v_p141_cur_color = Fp141_RgbToHSV(Fp141_CharToRgb( color.c_str() ));
				Fp141_SetCurrentColor(v_p141_cur_color);
				Fp141_OutputCurrentColor();
				//String log = F(PLUGIN_141_LOGPREFIX); log += F("H_RGB="); log += color; addLog(LOG_LEVEL_DEBUG, log);
				success = true;
			}

			if (command == F("h_hsv"))	{
				String color		= parseString(string, 2);
				v_p141_cur_color	= Fp141_CharToHsv( color.c_str() );
				Fp141_SetCurrentColor(v_p141_cur_color);
				Fp141_OutputCurrentColor();
				//String log = F(PLUGIN_141_LOGPREFIX); log += F("H_HSV="); log += color; addLog(LOG_LEVEL_DEBUG, log);
			  success = true;
			}


			if (command == F("mode"))	{
				byte mode = event->Par1;

				if(mode == 0){
					Fp141_CommandOff();
					success = true;
				}
				else if(mode == 1){
					Fp141_CommandOn();
					success = true;
				}
				else if(mode >= PLUGIN_141_FIRST_ANIM_MODE && mode < PLUGIN_141_MODES_TOTAL ){
					Fp141_ProcessAnimation(mode, true, false, event->Par2, event->Par3, event->Par4, event->Par5);
						success = true;
				}
			}

			if(success) {
				Fp141_SendStatus(event->Source);
			}

			//store current value
			UserVar[event->BaseVarIndex + 0] = (int) v_p141_cur_color.h;
			UserVar[event->BaseVarIndex + 1] = (int) v_p141_cur_color.s;
			UserVar[event->BaseVarIndex + 2] = (int) v_p141_cur_color.v;
			UserVar[event->BaseVarIndex + 3] = (int) v_p141_cur_anim_mode;

			break;
		}

		case PLUGIN_READ:
		{
			//String log = F(PLUGIN_141_LOGPREFIX); log += F("PLUGIN_READ !!!!"); addLog(LOG_LEVEL_INFO, log);

			success = true;
			break;
		}

		case PLUGIN_FIFTY_PER_SECOND:
		{
			Fp141_Loop();
			success = true;
			break;
		}
	}

	return success;
}

// ---------------------------------------------------------------------------------------
void Fp141_confirmFlash(){
}

// ---------------------------------------------------------------------------------------
void Fp141_Loop(){
	Fp141_ProcessAnimation(v_p141_cur_anim_mode, false, false, 0, 0, 0, 0);
}

// ---------------------------------------------------------------------------------------
void Fp141_ProcessAnimation(byte mode, boolean init, boolean is_button, int speed, int p3, int p4, int p5 ){
	String log = F(PLUGIN_141_LOGPREFIX);

	if(init){
		Fp141_SetCurrentMode(mode);

		if(v_p141_cur_anim_mode == mode && is_button){
			Fp141_confirmFlash();
			//log += F("Stop Animation "); log += mode; addLog(LOG_LEVEL_INFO, log);
			Fp141_CommandOff();
			return;
		}
/*
		else if(mode == 0){
			//log += F("Stop Animation "); log += mode; addLog(LOG_LEVEL_INFO, log);
			Fp141_CommandOff();
			return;
		}
*/
		else{
			//log = F(PLUGIN_141_LOGPREFIX); log += F("Start Animation "); log += mode; addLog(LOG_LEVEL_INFO, log);
		}
	}

	if     (mode==2)	{Fp141_AnimFlash	(init, speed);}
	else if(mode==3)	{Fp141_AnimStrobe	(init, speed);}
	else if(mode==4)	{Fp141_AnimFade		(init, speed);}
	else if(mode==5)	{Fp141_AnimSmooth	(init, speed);}
	else if(mode==6)	{Fp141_AnimParty	(init, speed);}
	else{
		//invalid mode
	}
}



// ---------------------------------------------------------------------------------------
void Fp141_AnimFlash(boolean init, byte speed){
	if(init){
		v_p141_cur_anim_speed	= speed ? speed : PLUGIN_141_ANIM_FLASH_SPEED;
		v_p141_cur_anim_step		= 1;
	}
	unsigned long now= millis();
	if(v_p141_cur_anim_step ==1 && now > (v_p141_anim_last_update + v_p141_cur_anim_speed) ){
		v_p141_cur_anim_step	= 0;
		v_p141_anim_last_update	= now;
		Fp141_OutputCurrentColor();
	}
	else if(v_p141_cur_anim_step ==0 && now > (v_p141_anim_last_update + PLUGIN_141_ANIM_FLASH_PAUSE) ){
		v_p141_cur_anim_step	= 1;
		v_p141_anim_last_update	= now;
		Fp141_OutputHSV(CHSV {0,0,0});
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_AnimStrobe(boolean init, byte speed){
	if(init){
		v_p141_cur_anim_speed	= speed ? speed : PLUGIN_141_ANIM_STROBE_SPEED;
		v_p141_cur_anim_step	= 1;
	}
	unsigned long now= millis();
	if(v_p141_cur_anim_step==1 && now > (v_p141_anim_last_update + PLUGIN_141_ANIM_STROBE_PAUSE) ){
		v_p141_cur_anim_step	= 0;
		v_p141_anim_last_update	= now;
		Fp141_OutputCurrentColor();
	}
	else if(v_p141_cur_anim_step==0 && now > (v_p141_anim_last_update + v_p141_cur_anim_speed) ){
		v_p141_cur_anim_step	= 1;
		v_p141_anim_last_update	= now;
		Fp141_OutputHSV(CHSV {0,0,0});
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_AnimFade(boolean init, byte speed){
	if(init){
		v_p141_cur_anim_speed	= speed ? speed : PLUGIN_141_ANIM_FADE_SPEED;
		v_p141_cur_anim_step	= v_p141_cur_color.v;
	}
	unsigned long now= millis();
	if( now > (v_p141_anim_last_update + v_p141_cur_anim_speed) ){
		Fp141_OutputHSV( CHSV(v_p141_cur_color.h, v_p141_cur_color.s, dim8_lin(v_p141_cur_anim_step)) );
		if(v_p141_cur_anim_dir){
			if(v_p141_cur_anim_step == 255){
				v_p141_cur_anim_dir=false;
			}
			else{
				v_p141_cur_anim_step++;
			}
		}
		else{
			if(v_p141_cur_anim_step == 1){
				v_p141_cur_anim_dir=true;
			}
			else{
				v_p141_cur_anim_step--;
			}
		}
		v_p141_anim_last_update = now;
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_AnimSmooth(boolean init, byte speed){
	if(init){
		v_p141_cur_anim_speed	= speed ? speed : PLUGIN_141_ANIM_SMOOTH_SPEED;
		v_p141_cur_anim_step	= 0;
		v_p141_cur_anim_color	= v_p141_cur_color;
		v_p141_cur_anim_color.s	= 255;
		v_p141_cur_anim_color.v	= 255;
	}
	unsigned long now= millis();
	if( now > (v_p141_anim_last_update + v_p141_cur_anim_speed) ){
		v_p141_cur_anim_color.h = v_p141_cur_anim_step;
		Fp141_OutputHSV(v_p141_cur_anim_color);
		v_p141_cur_anim_step++;
		if(v_p141_cur_anim_step > 255){
			v_p141_cur_anim_step=0;
		}
		v_p141_anim_last_update = now;
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_AnimParty(boolean init, byte speed){
	if(init){
		v_p141_cur_anim_speed	= speed ? speed : PLUGIN_141_ANIM_PARTY_SPEED;
		v_p141_cur_anim_step	= 0;
		v_p141_cur_anim_color	= v_p141_cur_color;
		v_p141_cur_anim_color.s	= 255;
		v_p141_cur_anim_color.v	= 255;
	}
	unsigned long now= millis();
	if(v_p141_cur_anim_step == 1 && now > (v_p141_anim_last_update + v_p141_cur_anim_speed) ){ 
		v_p141_cur_anim_color.h	= random(0,255); 
		Fp141_OutputHSV(v_p141_cur_anim_color);
		v_p141_cur_anim_step	= 0;
		v_p141_anim_last_update	= now;
	}
	else if(v_p141_cur_anim_step==0 && now > (v_p141_anim_last_update + v_p141_cur_anim_speed + 15) ){
		Fp141_OutputHSV(CHSV {0,0,0});
		v_p141_cur_anim_step	= 1;
		v_p141_anim_last_update	= now;
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_CommandOn(){
	v_p141_cur_color.v	= 255;
	Fp141_SetCurrentMode(1);
	Fp141_SetCurrentColor(v_p141_cur_color);
	Fp141_OutputCurrentColor();
}

// ---------------------------------------------------------------------------------------
void Fp141_CommandOff(){
	v_p141_cur_color.v 	= 0;
	Fp141_SetCurrentMode(0);
	Fp141_SetCurrentColor(v_p141_cur_color);
	Fp141_OutputCurrentColor();
}

// ---------------------------------------------------------------------------------------
void Fp141_OutputRGB( const CRGB& rgb){
	if(v_p141_cur_strip_type < PLUGIN_141_FIRST_TYPE_PIX ){
		analogWrite(v_p141_pins[0], v_p141_pin_inverse ? (PWMRANGE - rgb.r*4)  : rgb.r*4);
		analogWrite(v_p141_pins[1], v_p141_pin_inverse ? (PWMRANGE - rgb.g*4)  : rgb.g*4);
		analogWrite(v_p141_pins[2], v_p141_pin_inverse ? (PWMRANGE - rgb.b*4)  : rgb.b*4);
	}
	else{
		// pixels mode
	}

	//log only when NOT in anim mode
	if(v_p141_cur_anim_mode < PLUGIN_141_FIRST_ANIM_MODE ){
		String log = F(PLUGIN_141_LOGPREFIX);
		log += F("RGB = ");
		log += rgb.r;	log += F(",");
		log += rgb.g;	log += F(",");
		log += rgb.b;
		addLog(LOG_LEVEL_DEBUG, log);
	}
}

// ---------------------------------------------------------------------------------------
void Fp141_OutputHSV(CHSV hsv){
	//log only when NOT in anim mode
	if(v_p141_cur_anim_mode < 2){
		String log = F(PLUGIN_141_LOGPREFIX);
		log += F("HSV = ");
		log += hsv.h;	log += F(",");
		log += hsv.s;	log += F(",");
		log += hsv.v;
		addLog(LOG_LEVEL_DEBUG, log);
	}
	Fp141_OutputRGB( CHSV(hsv) );
}

// ---------------------------------------------------------------------------------------
void Fp141_OutputCurrentColor(){
	Fp141_OutputHSV( v_p141_cur_color );
}

// ---------------------------------------------------------------------------------------
CHSV Fp141_RgbToHSV(CRGB rgb){
     return rgb2hsv_approximate(rgb);
}

// ---------------------------------------------------------------------------------------
void Fp141_SetCurrentColor(CHSV hsv){
	v_p141_cur_color		= hsv;
	v_p141_cur_anim_color	= hsv;
}

// ---------------------------------------------------------------------------------------
void Fp141_SetCurrentMode(byte mode){
	v_p141_cur_anim_mode 				= mode;
}

// ---------------------------------------------------------------------------------------
CRGB Fp141_CharToRgb(const char * rgb) {
    char * p = (char *) rgb;
    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {
           ++p;
 	}
    return Fp141_LongToRgb( strtoul(p, NULL, 16) );
}

// ---------------------------------------------------------------------------------------
CHSV Fp141_CharToHsv(const char * rgb) {
    char * p = (char *) rgb;
    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {
           ++p;
 	}
    return Fp141_LongToHsv( strtoul(p, NULL, 16) );
}

// ---------------------------------------------------------------------------------------
CRGB Fp141_LongToRgb(unsigned long rgb){
	CRGB out;
	out.r = rgb >> 16;
	out.g = rgb >> 8 & 0xFF;
	out.b = rgb & 0xFF;
	return out;
}

// ---------------------------------------------------------------------------------------
CHSV Fp141_LongToHsv(unsigned long hsv){
	CHSV out;
	out.h = hsv >> 16;
	out.s = hsv >> 8 & 0xFF;
	out.v = hsv & 0xFF;
	return out;
}

// ---------------------------------------------------------------------------------
// ------------------------------ JsonResponse -------------------------------------
// ---------------------------------------------------------------------------------
void Fp141_SendStatus(byte eventSource) {

	CRGB rgb;
	rgb = v_p141_cur_color;

  String log = F(PLUGIN_141_LOGPREFIX);
  log += F("JSON reply send.");
  addLog(LOG_LEVEL_INFO, log);

  String json;
  printToWebJSON = true;
  json += F("{\n");
  json += F("\"plugin\": \"141");
  json += F("\",\n\"mode\": \"");
  json += v_p141_cur_anim_mode;
  json += F("\",\n\"speed\": \"");
  json += v_p141_cur_anim_speed;
  json += F("\",\n\"r\": \"");
  json += rgb.r;
  json += F("\",\n\"g\": \"");
  json += rgb.g;
  json += F("\",\n\"b\": \"");
  json += rgb.b;
  json += F("\",\n\"h\": \"");
  json += v_p141_cur_color.h;
  json += F("\",\n\"s\": \"");
  json += v_p141_cur_color.s;
	json += F("\",\n\"v\": \"");
  json += v_p141_cur_color.v;
  json += F("\"\n}\n");
  SendStatus(eventSource, json); // send http response to controller (JSON format)
}

/*
// ---------------------------------------------------------------------------------------
unsigned long Fp141_rgbToLong(CRGB in){
	return (((long)in.r & 0xFF) << 16) + (((long)in.g & 0xFF) << 8) + ((long)in.b & 0xFF);
}

// ---------------------------------------------------------------------------------------
unsigned long Fp141_hsvToLong(CHSV in){
	return (((long)in.h & 0xFF) << 16) + (((long)in.s & 0xFF) << 8) + ((long)in.v & 0xFF);
}
*/
