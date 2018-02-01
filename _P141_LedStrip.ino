/*
//#######################################################################################################
//#################################### Plugin 141: LedStrip ############################################
//#######################################################################################################
	Origal Work by by Jochen Krapf (jk@nerd2nerd.org)

	List of commands:
	(1) RGB,<red 0-255>,<green 0-255>,<blue 0-255>
	(2) HSV,<hue 0-360>,<saturation 0-100>,<value/brightness 0-100>
	(3) HSL,<hue 0-360>,<saturation 0-100>,<lightness 0-100>
	(4) HUE,<hue 0-360>
	(5) SAT,<saturation 0-100>
	(6) VAL,<value/brightness 0-100>
	(7) DIMM,<value/brightness 0-100>
	(8) OFF
	(9) CYCLE,<time 1-999>	time for full color hue circle; 0 to return to normal mode

	Usage:
	(1): Set RGB Color to LED (eg. RGB,255,255,255)

	// #include <*.h>	- no external lib required

	--- HUACANXING H801 ---------------------------------
	RGBWW GPIO	: 15, 13, 12, 14, 4, NOT Inversed (REBOOT)
	Led   GPIO	: 5, Inversed

*/

static float	plugin141_hsv_prev[3]	= {0,0,0};
static float	plugin141_hsv_dest[3]	= {0,0,0};
static float	plugin141_hsv_act[3]	= {0,0,0};
static long		plugin141_ms_fade_begin	= 0;
static long		plugin141_ms_fade_end	= 0;

static float	plugin141_cycle			= 0;
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
#define PLUGIN_141_VALUENAME_3	"State"

#define PLUGIN_141_MS_FADE_TIME	1500

#define PLUGIN_141_PWM_OFFSET 	0		
// ESP-PWM has flickering problems with values <6 and >1017. If problem is fixed in ESP libs the define can be set to 0 (or code removed)
// see https://github.com/esp8266/Arduino/issues/836		https://github.com/SmingHub/Sming/issues/70		https://github.com/espruino/Espruino/issues/914


boolean Plugin_141 (byte function, struct EventStruct *event, String& string) 
{

	boolean success = false;

	switch (function) {

		case PLUGIN_DEVICE_ADD: 
		{
			Device[++deviceCount].Number		= PLUGIN_ID_141;
			Device[deviceCount].Type			= DEVICE_TYPE_DUMMY;						// Nothing else really fit the bill ...
			Device[deviceCount].Ports			= 0;
			Device[deviceCount].VType			= SENSOR_TYPE_TRIPLE;
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
			// char tmpString[128]; // Unused ???

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
			String log = F("LedStrip: Pins ");
			for (byte i=0; i<4; i++)	{
				int pin = Settings.TaskDevicePluginConfig[event->TaskIndex][i];
				plugin141_pins[i] = pin;
				if (pin >= 0){
					pinMode(pin, OUTPUT);
				}
				log += pin;
				log += F(" ");
			}

			plugin141_pin_inverse = Settings.TaskDevicePin1Inversed[event->TaskIndex];
			if(plugin141_pin_inverse){
				log += F("INVERT");
			}
			else{
				log += F("NORM");
			}
			addLog(LOG_LEVEL_INFO, log);

			Plugin141_Output(plugin141_hsv_dest);

			success = true;
			break;
		}

		case PLUGIN_WRITE:
		{
			bool has_new_value = false;

			String command = parseString(string, 1);

			if (command == F("rgb"))	{
				float rgb[3];
				rgb[0] = event->Par1 / 255.0;		//R
				rgb[1] = event->Par2 / 255.0;		//G
				rgb[2] = event->Par3 / 255.0;		//B
				Plugin141_hsvClamp(rgb);
				Plugin141_rgb2hsv(rgb, plugin141_hsv_dest);
				has_new_value = true;
			}

			if (command == F("hsv"))	{
				plugin141_hsv_dest[0] = event->Par1 / 360.0;	 //Hue
				plugin141_hsv_dest[1] = event->Par2 / 100.0;	 //Saturation
				plugin141_hsv_dest[2] = event->Par3 / 100.0;	 //Value/Brightness
				has_new_value = true;
			}

			if (command == F("hsl"))	{
				float hsl[3];
				hsl[0] = event->Par1 / 360.0;		//Hue
				hsl[1] = event->Par2 / 100.0;		//Saturation
				hsl[2] = event->Par3 / 100.0;		//Lightness
				Plugin141_hsl2hsv(hsl, plugin141_hsv_dest);
				has_new_value = true;
			}

			if (command == F("hue"))	{
				plugin141_hsv_dest[0] = event->Par1 / 360.0;	 //Hue
				has_new_value = true;
			}

			if (command == F("sat"))	{
				plugin141_hsv_dest[1] = event->Par1 / 100.0;	 //Saturation
				has_new_value = true;
			}

			if (command == F("val") || command == F("dimm"))	{
				plugin141_hsv_dest[2] = event->Par1 / 100.0;	 //Value/Brightness
				has_new_value = true;
			}

			if (command == F("off"))	{
				plugin141_hsv_dest[2] = 0.0;	 //Value/Brightness
				has_new_value = true;
			}

			if (command == F("cycle"))	{
				plugin141_cycle = event->Par1;		//seconds for a full color hue circle
				if (plugin141_cycle > 0){
					plugin141_cycle = (1.0 / 50.0) / plugin141_cycle;		//50Hz based increment value
				}
				success = true;
			}

			if (has_new_value)	{
				Plugin141_hsvClamp(plugin141_hsv_dest);
				Plugin141_hsvClamp(plugin141_hsv_prev);

				if (plugin141_ms_fade_begin!=0) {	//still fading?
					for (byte i=0; i<3; i++) {
						plugin141_hsv_prev[i] = plugin141_hsv_act[i];
					}
				}	

				//get the shortest way around the color circle
				if ((plugin141_hsv_dest[0]-plugin141_hsv_prev[0]) > 0.5) {
					plugin141_hsv_prev[0] += 1.0;
				}
				if ((plugin141_hsv_dest[0]-plugin141_hsv_prev[0]) < -0.5) {
					plugin141_hsv_prev[0] -= 1.0;
				}

				plugin141_ms_fade_begin = millis();
				plugin141_ms_fade_end = plugin141_ms_fade_begin + PLUGIN_141_MS_FADE_TIME;

				String log = F("LedStrip: hsv ");
				for (byte i=0; i<3; i++) {
					log += toString(plugin141_hsv_dest[i], 3);
					log += F(" ");
				}
				addLog(LOG_LEVEL_INFO, log);

				plugin141_cycle = 0;		//ends cycle loop
				success = true;
			}
			break;
		}

		case PLUGIN_READ:	
		{
			UserVar[event->BaseVarIndex + 0] = plugin141_hsv_dest[0] * 360.0;
			UserVar[event->BaseVarIndex + 1] = plugin141_hsv_dest[1] * 100.0;
			UserVar[event->BaseVarIndex + 2] = plugin141_hsv_dest[2] * 100.0;
			success = true;
			break;
		}

		//case PLUGIN_TEN_PER_SECOND:
		case PLUGIN_FIFTY_PER_SECOND:	
		{
			// cyclic colors
			if (plugin141_cycle > 0) {
				plugin141_hsv_dest[0] += plugin141_cycle;
				Plugin141_hsvCopy(plugin141_hsv_dest, plugin141_hsv_prev);
				Plugin141_hsvCopy(plugin141_hsv_dest, plugin141_hsv_act);
				Plugin141_Output(plugin141_hsv_dest);
			}
			//fading required?
			else if (plugin141_ms_fade_end != 0) {		
				long millisAct = millis();

				//destination reached?
				if (millisAct >= plugin141_ms_fade_end) {	
					plugin141_ms_fade_begin = 0;
					plugin141_ms_fade_end = 0;
					Plugin141_hsvCopy(plugin141_hsv_dest, plugin141_hsv_prev);
					Plugin141_hsvCopy(plugin141_hsv_dest, plugin141_hsv_act);
				}
				//just fading
				else {	
					float fade = float(millisAct-plugin141_ms_fade_begin) / float(plugin141_ms_fade_end-plugin141_ms_fade_begin);
					fade = Plugin141_ValueClamp(fade);
					fade = Plugin141_ValueSmoothFadingOut(fade);

					for (byte i=0; i<3; i++){
						plugin141_hsv_act[i] = Plugin141_Mix(plugin141_hsv_prev[i], plugin141_hsv_dest[i], fade);
					}
				}

				Plugin141_Output(plugin141_hsv_act);
			}
			success = true;
			break;
		}
	}

	return success;
}

void Plugin141_Output(float* hsvIn) {
	float hsvw[4];
	float rgbw[4];

	String log = F("LedStrip: RGBW ");

	Plugin141_hsvCopy(hsvIn, hsvw);
	Plugin141_hsvClamp(hsvw);

	if (plugin141_pins[3] >= 0) {	//has white channel?
		hsvw[3] = (1.0 - hsvw[1]) * hsvw[2];	 // w = (1-s)*v
		hsvw[2] *= hsvw[1];		// v = s*v
	}
	else{
		hsvw[3] = 0.0;	 // w = 0
	}

	//convert to RGB color space
	Plugin141_hsv2rgb(hsvw, rgbw);
	rgbw[3] = hsvw[3];

	//reduce power for Plugin141_Mix colors
	float cv = sqrt(rgbw[0]*rgbw[0] + rgbw[1]*rgbw[1] + rgbw[2]*rgbw[2]);
	if (cv > 0.0)	{
		cv = hsvw[2] / cv;
		cv = Plugin141_Mix(1.0, cv, 0.42);
		for (byte i=0; i<3; i++){
			rgbw[i] *= cv;
		}
	}

	int actRGBW[4];
	static int lastRGBW[4] = {-1,-1,-1,-1};

	//converting and corrections for each RGBW value
	for (byte i=0; i<4; i++) {
		int pin = plugin141_pins[i];
		//log += pin;
		//log += F("=");
		
		if (pin >= 0) {	//pin assigned for RGBW value
			rgbw[i] *= rgbw[i];		//simple gamma correction

			actRGBW[i] = rgbw[i] * (PWMRANGE-2*PLUGIN_141_PWM_OFFSET) + PLUGIN_141_PWM_OFFSET + 0.5;
			//if (actRGBW[i] > PWMRANGE)
			//	actRGBW[i] = PWMRANGE;

			log += String(actRGBW[i], DEC);
			log += F(" ");
		}
		else	{
			log += F("- ");
		}
	}

	if (PLUGIN_141_PWM_OFFSET != 0 && actRGBW[0] == PLUGIN_141_PWM_OFFSET && actRGBW[1] == PLUGIN_141_PWM_OFFSET && actRGBW[2] == PLUGIN_141_PWM_OFFSET){
		actRGBW[0] = actRGBW[1] = actRGBW[2] = 0;
	}

	//output to PWM
	for (byte i=0; i<4; i++)	{
		int pin = plugin141_pins[i];
		if (pin >= 0)	{	//pin assigned for RGBW value
			if (lastRGBW[i] != actRGBW[i])	{	 //has changed since last output?
				lastRGBW[i] = actRGBW[i];

				if (plugin141_pin_inverse) {	//low active or common annode LED?
					actRGBW[i] = PWMRANGE - actRGBW[i];
				}

				analogWrite(pin, actRGBW[i]);
				setPinState(PLUGIN_ID_141, pin, PIN_MODE_PWM, actRGBW[i]);

				log += F("~");
			}
			else{
				log += F(".");				
			}
		}
	}

	addLog(LOG_LEVEL_DEBUG, log);
}

// HSV->RGB conversion based on GLSL version
// expects hsv channels defined in 0.0 .. 1.0 interval
float Plugin141_Fract(float x) { 
	return x - int(x); 
}

float Plugin141_Mix(float a, float b, float t) { 
	return a + (b - a) * t; 
}

float Plugin141_Step(float e, float x) { 
	return x < e ? 0.0 : 1.0; 
}

float* Plugin141_hsv2rgb(const float* hsv, float* rgb)	{
	rgb[0] = hsv[2] * Plugin141_Mix(1.0, constrain(abs(Plugin141_Fract(hsv[0] + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), hsv[1]);
	rgb[1] = hsv[2] * Plugin141_Mix(1.0, constrain(abs(Plugin141_Fract(hsv[0] + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), hsv[1]);
	rgb[2] = hsv[2] * Plugin141_Mix(1.0, constrain(abs(Plugin141_Fract(hsv[0] + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), hsv[1]);
	return rgb;
}

float* Plugin141_rgb2hsv(const float* rgb, float* hsv)	{
	float s = Plugin141_Step(rgb[2], rgb[1]);
	float px = Plugin141_Mix(rgb[2], rgb[1], s);
	float py = Plugin141_Mix(rgb[1], rgb[2], s);
	float pz = Plugin141_Mix(-1.0, 0.0, s);
	float pw = Plugin141_Mix(0.6666666, -0.3333333, s);
	s = Plugin141_Step(px, rgb[0]);
	float qx = Plugin141_Mix(px, rgb[0], s);
	float qz = Plugin141_Mix(pw, pz, s);
	float qw = Plugin141_Mix(rgb[0], px, s);
	float d = qx - std::min(qw, py);
	hsv[0] = abs(qz + (qw - py) / (6.0 * d + 1e-10));
	hsv[1] = d / (qx + 1e-10);
	hsv[2] = qx;
	return hsv;
}

//see http://codeitdown.com/hsl-hsb-hsv-color/
float* Plugin141_hsl2hsv(const float* hsl, float* hsv){
	float B = ( 2.0*hsl[2] + hsl[1] * (1.0-(2.0*hsl[2]-1.0)) ) / 2.0;		// B = ( 2L+Shsl(1-|2L-1|) ) / 2
	float S = (B!=0) ? 2.0*(B-hsl[2]) / B : 0.0;	 // S = 2(B-L) / B
	hsv[0] = hsl[0];
	hsv[1] = S;
	hsv[2] = B;
	return hsv;
}

float* Plugin141_hsvCopy(const float* hsvSrc, float* hsvDst)	{
	for (byte i=0; i<3; i++){
		hsvDst[i] = hsvSrc[i];
	}
	return hsvDst;
}

float* Plugin141_hsvClamp(float* hsv)	{
	while (hsv[0] > 1.0) {
		hsv[0] -= 1.0;
	}
	while (hsv[0] < 0.0) {
		hsv[0] += 1.0;		
	}

	for (byte i=1; i<3; i++) {
		if (hsv[i] < 0.0) {
			hsv[i] = 0.0;			
		}
		if (hsv[i] > 1.0) {
			hsv[i] = 1.0;
		}
	}
	return hsv;
}

float Plugin141_ValueClamp(float v) {
	if (v < 0.0) {
		v = 0.0;
	}
	if (v > 1.0) {
		v = 1.0;
	}
	return v;
}

float Plugin141_ValueSmoothFadingOut(float v) {
	v = Plugin141_ValueClamp(v);
	v = 1.0-v;	 //smooth fading out
	v *= v;
	v = 1.0-v;
	return v;
}
