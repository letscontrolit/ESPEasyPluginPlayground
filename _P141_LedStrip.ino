/*
https://github.com/ddtlabs/ESPEasy-Plugin-Lights/blob/master/_P123_LIGHTS.ino


//#######################################################################################################
//#################################### Plugin 141: LedStrip ############################################
//#######################################################################################################

	Copyright Francois Dechery 2017 - https://github.com/soif


	List of commands:
	* RGB,<red 0-255>,<green 0-255>,<blue 0-255>
	* HSV,<hue 0-255>,<saturation 0-255>,<value/brightness 0-255>
	* HUE,<hue 0-360>
	* SAT,<saturation 0-100>
	* VAL,<value/brightness 0-100>
	* DIM,<value/brightness 0-100>
	* ON
	* OFF
	* CYCLE,<time 1-999>	time for full color hue circle; 0 to return to normal mode

	Usage:
	(1): Set RGB Color to LED (eg. /control?cmd=RGB,255,255,255)


	--- HUACANXING H801 ---------------------------------
	RGBWW GPIO	: 15, 13, 12, 14, 4, NOT Inversed (REBOOT)
	Led   GPIO	: 5, Inversed


*/

//#define FASTLED_FORCE_SOFTWARE_SPI
//#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
//FASTLED_USING_NAMESPACE ;

#include <IRremoteESP8266.h>



static int		plugin141_pins[4]		= {-1,-1,-1,-1};
static int		plugin141_pin_inverse	= false;

#define PLUGIN_141
#define PLUGIN_ID_141			141
#define PLUGIN_NAME_141			"Output - LedStrip"

#define PLUGIN_141_MAX_PINS		16
#define PLUGIN_141_CONF_0		"pin_r"
#define PLUGIN_141_CONF_1		"pin_g"
#define PLUGIN_141_CONF_2		"pin_b"
#define PLUGIN_141_CONF_3		"pin_w1"

#define PLUGIN_141_VALUENAME_0	"Hue"
#define PLUGIN_141_VALUENAME_1	"Sat"
#define PLUGIN_141_VALUENAME_2	"Val"
#define PLUGIN_141_VALUENAME_3	"Mode"

#define PLUGIN_141_MS_FADE_TIME	1500

#define PLUGIN_141_PWM_OFFSET 	0
// ESP-PWM has flickering problems with values <6 and >1017. If problem is fixed in ESP libs the define can be set to 0 (or code removed)
// see https://github.com/esp8266/Arduino/issues/836		https://github.com/SmingHub/Sming/issues/70		https://github.com/espruino/Espruino/issues/914


#define PLUGIN_141_STRIP_TYPE	0
#define PLUGIN_141_LOGPREFIX	"LedStrip1: "



// #### Defined ##########################################################################
    #define LIGHT_IR_PIN        4    // IR LED

        #define IR_BUTTON_0  0xFF906F // Brightness +
        #define IR_BUTTON_1  0xFFB847 // Brightness -
        #define IR_BUTTON_2  0xFFF807 // OFF
        #define IR_BUTTON_3  0xFFB04F // ON

        #define IR_BUTTON_4  0xFF9867 // RED
        #define IR_BUTTON_5  0xFFD827 // GREEN
        #define IR_BUTTON_6  0xFF8877 // BLUE
        #define IR_BUTTON_7  0xFFA857 // WHITE

        #define IR_BUTTON_8  0xFFE817 // "Red" 1
        #define IR_BUTTON_9  0xFF48B7 // "Green" 1
        #define IR_BUTTON_10 0xFF6897 // "Blue" 1
        #define IR_BUTTON_11 0xFFB24D // FLASH Mode

        #define IR_BUTTON_12 0xFF02FD // "Red" 2
        #define IR_BUTTON_13 0xFF32CD // "Green" 2
        #define IR_BUTTON_14 0xFF20DF // "Blue" 2
        #define IR_BUTTON_15 0xFF00FF // STROBE Mode

        #define IR_BUTTON_16 0xFF50AF // "Red" 3
        #define IR_BUTTON_17 0xFF7887 // "Green" 3
        #define IR_BUTTON_18 0xFF708F // "Blue" 3
        #define IR_BUTTON_19 0xFF58A7 // FADE Mode

        #define IR_BUTTON_20 0xFF38C7 // "Red" 4
        #define IR_BUTTON_21 0xFF28D7 // "Green" 4
        #define IR_BUTTON_22 0xFFF00F // "Blue" 4
        #define IR_BUTTON_23 0xFF30CF // SMOOTH Mode


#define ANIM_MODE_COUNT (2 + 5)


#define ANIM_SPEED_STEP 20
#define ANIM1_SPEED 350			// flash ON Variable
#define ANIM1_PAUSE 200			// flash OFF fixed
#define ANIM2_SPEED 550			// strobe OFF variable
#define ANIM2_PAUSE 150			// storbe ON fixed
#define ANIM3_SPEED 100			// fade speed
#define ANIM4_SPEED 700			// smooth speed
#define ANIM5_SPEED 200			// party speed

#define BUTTONS_COUNT 24

// #### Variables ########################################################################
unsigned long r_but_codes[]={		// IR remote buttons codes
	IR_BUTTON_0  , //	Brightness +
	IR_BUTTON_1  , //	Brightness -
	IR_BUTTON_2  , //	OFF
	IR_BUTTON_3  , //	ON
	IR_BUTTON_4  , //	Red
	IR_BUTTON_5  , //	Green
	IR_BUTTON_6  , //	Blue
	IR_BUTTON_7  , //	White
	IR_BUTTON_8  , //	R1
	IR_BUTTON_9  , //	G1
	IR_BUTTON_10 , //	B1
	IR_BUTTON_11 , //	Flash
	IR_BUTTON_12 , //	R2
	IR_BUTTON_13 , //	G2
	IR_BUTTON_14 , //	B2
	IR_BUTTON_15 , //	Strobe
	IR_BUTTON_16 , //	R3
	IR_BUTTON_17 , //	G3
	IR_BUTTON_18 , //	B3
	IR_BUTTON_19 , //	Fade
	IR_BUTTON_20 , //	R4
	IR_BUTTON_21 , //	G4
	IR_BUTTON_22 , //	B4
	IR_BUTTON_23	//	Smooth
};

unsigned long r_but_colors[]={	// IR remote buttons colors
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

// variables declarations ###############################################################
CHSV 			_cur_color				= CHSV(0,255,255);
CHSV 			_cur_anim_color			= CHSV(0,0,0);
byte 			_cur_status  			= 0 ;
byte 			_cur_anim_mode  		= 0 ;
byte 			_cur_anim_step  		= 0;
boolean 		_cur_anim_dir	  		= true;
unsigned long	_cur_anim_speed 		= 1000;
unsigned long	_anim_last_update 		= millis();
unsigned long	_last_ir_button			= 0;
unsigned long	_last_status_led_time	= 0;

IRrecv _ir_recv(LIGHT_IR_PIN); 		//IRrecv _ir_recv(IR_PIN, IR_LED_PIN); dont work. Why ?
decode_results _ir_results;



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

			string += F("<TR><TD>GPIO:<TD>");

			string += F("<TR><TD>1st GPIO (R):<TD>");
			addPinSelect(false, string, PLUGIN_141_CONF_0, Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
			
			string += F("<TR><TD>2nd GPIO (G):<TD>");
			addPinSelect(false, string, PLUGIN_141_CONF_1, Settings.TaskDevicePluginConfig[event->TaskIndex][1]);

			string += F("<TR><TD>3rd GPIO (B):<TD>");
			addPinSelect(false, string, PLUGIN_141_CONF_2, Settings.TaskDevicePluginConfig[event->TaskIndex][2]);

			string += F("<TR><TD>4th GPIO (W) optional:<TD>");
			addPinSelect(false, string, PLUGIN_141_CONF_3, Settings.TaskDevicePluginConfig[event->TaskIndex][3]);

			success = true;
			break;
		}

		case PLUGIN_WEBFORM_SAVE:
		{
        	Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F(PLUGIN_141_CONF_0));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F(PLUGIN_141_CONF_1));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F(PLUGIN_141_CONF_2));
        	Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F(PLUGIN_141_CONF_3));

			// reset invalid pins
			for (byte i=0; i<4; i++){
				if (Settings.TaskDevicePluginConfig[event->TaskIndex][i] >= PLUGIN_141_MAX_PINS){
					Settings.TaskDevicePluginConfig[event->TaskIndex][i] = -1;
				}
			}

			success = true;
			break;
		}

		case PLUGIN_INIT:
		{
			analogWriteFreq(400);

			// assign pins
			
			String log = F(PLUGIN_141_LOGPREFIX);
			log += F("Pins ");
			
			for (byte i=0; i<3; i++)	{
				int pin = Settings.TaskDevicePluginConfig[event->TaskIndex][i];
				plugin141_pins[i] = pin;
				if (pin >= 0){
					pinMode(pin, OUTPUT);
				}
				log += pin;
				log += F(" ");
			}
			/*
			plugin141_pin_inverse = Settings.TaskDevicePin1Inversed[event->TaskIndex];
			if(plugin141_pin_inverse){
				log += F("INVERT");
			}
			else{
				log += F("NORM");
			}
			*/
			addLog(LOG_LEVEL_INFO, log);

			success = true;
			break;
		}

		case PLUGIN_WRITE:
		{
			String command = parseString(string, 1);

			if (command == F("rgb"))	{
				CRGB rgb = CRGB( (int) event->Par1 , (int) event->Par2 , (int) event->Par3 );
				CHSV hsv = _rgbToHsv(rgb);
				_setCurrentColor(hsv);
				_setLedsHSV(hsv);
			}

			if (command == F("hsv"))	{
				CHSV hsv = CHSV ( (int) event->Par1 , (int) event->Par2 , (int) event->Par3   );
				_setCurrentColor(hsv);
				_setLedsHSV(hsv);
			}

			if (command == F("hue"))	{
				_cur_color.h = event->Par1 ;	 //Hue
				_setCurrentColor(_cur_color);
				_setLedsHSV(_cur_color);
			}

			if (command == F("sat"))	{
				_cur_color.s = event->Par1 ;	 //Saturation
				_setCurrentColor(_cur_color);
				_setLedsHSV(_cur_color);
			}

			if (command == F("val") || command == F("dim"))	{
				_cur_color.v = event->Par1 ;	 //Value/Brightness
				_setCurrentColor(_cur_color);
				_setLedsHSV(_cur_color);
			}

			if (command == F("off"))	{
				_CommandOff();
			}

			if (command == F("on"))	{
				_CommandOn();
			}


			if (command == F("mode"))	{
				int mode = (int) event->Par1;
				
				if(mode == 0){
					_CommandOff();
				}
				else if(mode == 1){
					_CommandOff();
				}
				else if(mode > 1 && mode <= ANIM_MODE_COUNT ){
					int speed = (int) event->Par2;
					_setCurrentMode(mode);
				}
			}

			success = true;

			break;
		}

		case PLUGIN_READ:
		{
			String log = F(PLUGIN_141_LOGPREFIX);
			log += F("PLUGIN_READ !!!!");
			addLog(LOG_LEVEL_INFO, log);

			UserVar[event->BaseVarIndex + 0] = (int) _cur_color.h;
			UserVar[event->BaseVarIndex + 1] = (int) _cur_color.s;
			UserVar[event->BaseVarIndex + 2] = (int) _cur_color.v;
			UserVar[event->BaseVarIndex + 3] = (int) _cur_anim_mode;
			success = true;
			break;
		}

		//case PLUGIN_TEN_PER_SECOND:
		case PLUGIN_FIFTY_PER_SECOND:
		{
			success = true;
			break;
		}
	}

	return success;
}

// ---------------------------------------------------------------------------------------
void _CommandOn(){
	_cur_color.v=255;
	_setCurrentColor(_cur_color);
	_setCurrentMode(1);
	_setLedsHSV(_cur_color);
}

// ---------------------------------------------------------------------------------------
void _CommandOff(){
	_cur_color.v = 0;
	_setCurrentColor(_cur_color);
	_setCurrentMode(0);
	_setLedsHSV(_cur_color);
}



// ---------------------------------------------------------------------------------------
void Plugin141_setLedsRGB( const CRGB& rgb)
{
	if(PLUGIN_141_STRIP_TYPE == 0 ){
		analogWrite(plugin141_pins[0], plugin141_pin_inverse ? (PWMRANGE - rgb.r)  : rgb.r);
		analogWrite(plugin141_pins[1], plugin141_pin_inverse ? (PWMRANGE - rgb.g)  : rgb.g);
		analogWrite(plugin141_pins[2], plugin141_pin_inverse ? (PWMRANGE - rgb.b)  : rgb.b);
	}
	else if(PLUGIN_141_STRIP_TYPE > 0){

	}

	if(_cur_anim_mode < 2){
		String log = F(PLUGIN_141_LOGPREFIX);
		log += F("RGB = ");
		log += rgb.r;	log += F(",");
		log += rgb.g;	log += F(",");
		log += rgb.b;
		addLog(LOG_LEVEL_DEBUG, log);
	}
}

// ---------------------------------------------------------------------------------------
void _setLedsHSV(CHSV hsv){
	if(_cur_anim_mode < 2){
		String log = F(PLUGIN_141_LOGPREFIX);
		log += F("HSV = ");
		log += hsv.h;	log += F(",");
		log += hsv.s;	log += F(",");
		log += hsv.v;
		addLog(LOG_LEVEL_DEBUG, log);
	}
	Plugin141_setLedsRGB( CHSV(hsv) );
}
// ---------------------------------------------------------------------------------------
CHSV _rgbToHsv(CRGB rgb){
     return rgb2hsv_approximate(rgb);
}


// ---------------------------------------------------------------------------------------
void _setCurrentColor(CHSV hsv){
	_cur_color 		= hsv;
	_cur_anim_color = hsv;
	
	// save to web
	//UserVar[event->BaseVarIndex + 0]= _cur_color.h;
	//UserVar[event->BaseVarIndex + 1]= _cur_color.s;
	//UserVar[event->BaseVarIndex + 2]= _cur_color.v;
	
}

// ---------------------------------------------------------------------------------------
void _setCurrentMode(byte mode){
	_cur_anim_mode 		= mode;
	//UserVar[event->BaseVarIndex + 3]= mode;	
}
