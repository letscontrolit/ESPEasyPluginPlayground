#ifdef USES_P198
//#######################################################################################################
//################################# Plugin 198: Venta Humidifiers #######################################
//#######################################################################################################

#define PLUGIN_198
#define PLUGIN_ID_198         198
#define PLUGIN_NAME_198       "Venta Humidifier"
#define PLUGIN_VALUENAME1_198 "Power"
#define PLUGIN_VALUENAME2_198 "Level"
#define PLUGIN_VALUENAME3_198 "Error"
#define PLUGIN_VALUENAME4_198 "CaseOpen"


typedef struct {
	int pwr_led;
	int led1;
	int led2;
	int led3;
	int error_led;
	int pwr_btn;
	int up_btn;
	int open_sensor;
	int err_led_ctl;
	int press_duration;
} venta_config;

typedef struct venta_status {
	bool power;
	int level;
	bool error;
	bool open;
} venta_status;

bool pressVentaButton(int pin, int duration, struct EventStruct *event = NULL) {
	digitalWrite(pin, true);
	delay(duration);
	digitalWrite(pin, false);
	setPinState(PLUGIN_ID_198, pin, PIN_MODE_OUTPUT, true);
	String log = String(F("Venta: GPIO ")) + String(pin) + String(F(" Pulsed for ")) + String(duration) + String(F(" mS"));
	addLog(LOG_LEVEL_INFO, log);
	if (event) {
		SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_198, pin, log, 0));
	}
	return true;
}

bool ventaErrLED(int pin, int state, struct EventStruct *event = NULL) {
	digitalWrite(pin, state);
	setPinState(PLUGIN_ID_198, pin, PIN_MODE_OUTPUT, state);
	String log = String(F("Venta: GPIO ")) + String(pin) + String(F(" set to ")) + String(state);
	addLog(LOG_LEVEL_INFO, log);
	if (event) {
		SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_198, pin, log, 0));
	}
	return true;
}




boolean Plugin_198(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
	static struct venta_status status;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_198;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
//        Device[deviceCount].Custom = true;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = true;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = false;
        Device[deviceCount].DecimalsOnly = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_198);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_198));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_198));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_198));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_198));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        venta_config cfg;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

        addFormPinSelect(F("Power LED"), F("plugin_198_pinpwrled"), cfg.pwr_led);
        addFormPinSelect(F("LED 1"), F("plugin_198_pinled1"), cfg.led1);
        addFormPinSelect(F("LED 2"), F("plugin_198_pinled2"), cfg.led2);
        addFormPinSelect(F("LED 3"), F("plugin_198_pinled3"), cfg.led3);
        addFormPinSelect(F("Error LED"), F("plugin_198_pinerrled"), cfg.error_led);
        addFormPinSelect(F("Power Button"), F("plugin_198_pinpwrbtn"), cfg.pwr_btn);
        addFormPinSelect(F("Up/Down Button"), F("plugin_198_pinudbtn"), cfg.up_btn);
        addFormPinSelect(F("Open Sensor"), F("plugin_198_pinopenbtn"), cfg.open_sensor);
        addFormPinSelect(F("Error LED Ctl"), F("plugin_198_pinerrledctl"), cfg.err_led_ctl);

				addFormNumericBox(F("Button press duration (ms)"), F("plugin_198_pressduration"), cfg.press_duration);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        venta_config cfg;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

        cfg.pwr_led = getFormItemInt(F("plugin_198_pinpwrled"));
        cfg.led1 = getFormItemInt(F("plugin_198_pinled1"));
        cfg.led2 = getFormItemInt(F("plugin_198_pinled2"));
        cfg.led3 = getFormItemInt(F("plugin_198_pinled3"));
        cfg.error_led = getFormItemInt(F("plugin_198_pinerrled"));
        cfg.pwr_btn = getFormItemInt(F("plugin_198_pinpwrbtn"));
        cfg.up_btn = getFormItemInt(F("plugin_198_pinudbtn"));
        cfg.open_sensor = getFormItemInt(F("plugin_198_pinopenbtn"));
        cfg.err_led_ctl = getFormItemInt(F("plugin_198_pinerrledctl"));
				cfg.press_duration = getFormItemInt(F("plugin_198_pressduration"));

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SHOW_VALUES:
        // TODO
        break;

    case PLUGIN_INIT:
      {
        venta_config cfg;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

				int inputtype = (Settings.TaskDevicePin1PullUp[event->TaskIndex]) ? INPUT_PULLUP : INPUT;
        pinMode(cfg.pwr_led, inputtype); // PWR LED
        pinMode(cfg.led1, inputtype); // LED 1
        pinMode(cfg.led2, inputtype); // LED 2
        pinMode(cfg.led3, inputtype); // LED 3
        pinMode(cfg.error_led, inputtype); // ERR LED
        pinMode(cfg.pwr_btn, OUTPUT); // Power Button
        pinMode(cfg.up_btn, OUTPUT); // Up/Down Button
        pinMode(cfg.open_sensor, inputtype); // Case Open Sensor
        pinMode(cfg.err_led_ctl, OUTPUT);

        attachInterrupt(digitalPinToInterrupt(cfg.pwr_led), Plugin_198_ISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(cfg.led1), Plugin_198_ISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(cfg.led2), Plugin_198_ISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(cfg.led3), Plugin_198_ISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(cfg.error_led), Plugin_198_ISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(cfg.open_sensor), Plugin_198_ISR, CHANGE);

        setPinState(PLUGIN_ID_198, cfg.pwr_btn, PIN_MODE_OUTPUT, 0); // Power Button
        setPinState(PLUGIN_ID_198, cfg.up_btn, PIN_MODE_OUTPUT, 0); // Up/Down Button
        setPinState(PLUGIN_ID_198, cfg.err_led_ctl, PIN_MODE_OUTPUT, 0); // Error LED control

        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
				struct venta_status st;
        success = readVentaState(event, &st) && assignVentaState(event, &st);

				if (ventaStatusChanged(status, st)) {
					status = st;
					ventaLog(event->BaseVarIndex);
	        sendData(event);
				}

        break;
      }

    case PLUGIN_READ:
      {
				struct venta_status st;
        success = readVentaState(event, &st) && assignVentaState(event, &st);
        ventaLog(event->BaseVarIndex);
        break;
      }

    case PLUGIN_WRITE:
      {
				// TODO: Proper log handling
        String log = "";
        String command = parseString(string, 1);

				venta_config cfg;
			  LoadCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

				struct venta_status st;
				success = readVentaState(event, &st);

        if (command == F("ventapower")) {
					success = true;
					if (event->Par1 != st.power) {
						// Change power status by emulating a button press
						pressVentaButton(cfg.pwr_btn, cfg.press_duration);
					}
				}
				if (command == F("ventapowerbtn")) {
					// A buttonpress of the the power button, irrespective of current state
					success = true;
					// Change power status by emulating a button press
					pressVentaButton(cfg.pwr_btn, cfg.press_duration);
				}
				if (command == F("ventalevelup")) {
					// A buttonpress of the the UP button, irrespective of current state
					success = true;
					// Change level by emulating a button press
					pressVentaButton(cfg.up_btn, cfg.press_duration);
				}

				if (command == F("ventaerrorled")) {
					success = true;
					if (event->Par1 != st.error) {
						ventaErrLED(cfg.err_led_ctl, event->Par1);
					}
				}
				if (command == F("ventaerrorflash")) {
					success = true;
					if (!st.error) {
						ventaErrLED(cfg.err_led_ctl, true);
						setPluginTaskTimer(event->Par1, PLUGIN_ID_198, event->TaskIndex, cfg.err_led_ctl, false);
					}
				}

				if (command == F("ventalevel") && event->Par1 == 0) {
					// Simply turn off
					success = true;
					if (st.power) {
						pressVentaButton(cfg.pwr_btn, cfg.press_duration);
						delay(100);
						readVentaState(event, &st) && assignVentaState(event, &st);
					}
				}
				if (command == F("ventalevel") && event->Par1 > 0) {
					success = true;
					int newlvl = event->Par1;
					// This is the most complicated command, as it needs to combine a lot of logic / input from different states:
					// if OFF -> turn on (emulate power btn press); wait
					// if error -> press up button; Wait
					// if still error -> unsuccessful
					// if current level != Par1 => emulate UP btn press 1-3 times.
					if ((!st.power && newlvl > 0) || (st.power && newlvl == 0)) {
						pressVentaButton(cfg.pwr_btn, cfg.press_duration);
						delay(100);
						readVentaState(event, &st) && assignVentaState(event, &st);
					}
					if (st.error) {
						pressVentaButton(cfg.up_btn, cfg.press_duration);
						delay(100);
						readVentaState(event, &st) && assignVentaState(event, &st);
					}
					if (!st.power || st.error) {
						log = String(F("Venta Level set unsuccessful, Target level ")) + String(newlvl) +
						      String(F(", power ")) + String(st.power) + String(F(", level ")) + String(st.level) +
									String(F(", error ")) + String(st.error);
					  addLog(LOG_LEVEL_INFO, log);
					}
					readVentaState(event, &st) && assignVentaState(event, &st);
					if (st.level != newlvl) {
						int diff = newlvl - st.level;
						if (diff < 0) diff += 3;
						for (int i = 0; i < diff; i++) {
							pressVentaButton(cfg.up_btn, cfg.press_duration);
							delay(100);
						}
					}
				}

				if (ventaStatusChanged(status, st)) {
					status = st;
					ventaLog(event->BaseVarIndex);
	        sendData(event);
				}


        break;
      }




    case PLUGIN_TIMER_IN:
      {
		    ventaErrLED(event->Par1, event->Par2);
        break;
      }
  }
  return success;
}


void ventaLog(int baseIndex) {
  String log = F("VENTA: State ");
  log += UserVar[baseIndex];
  log += F(", LVL ");
  log += UserVar[baseIndex + 1];
  log += F(", Err ");
  log += UserVar[baseIndex + 2];
  log += F(", Open ");
  log += UserVar[baseIndex + 3];
  addLog(LOG_LEVEL_INFO, log);
}

bool ventaStatusChanged(struct venta_status status, struct venta_status st) {
	return !(
		  (st.power == status.power) &&
			(st.level == status.level) &&
			(st.error == status.error) &&
			(st.open == status.open));
}

bool readVentaState(struct EventStruct *event, struct venta_status *status) {
  venta_config cfg;
  LoadCustomTaskSettings(event->TaskIndex, (byte*)&cfg, sizeof(cfg));

	status->power = digitalRead(cfg.pwr_led) == LOW;
  status->error = digitalRead(cfg.error_led) == LOW;
  status->open = digitalRead(cfg.open_sensor) == LOW;

	int level = 0;
  if (digitalRead(cfg.led1) == LOW) {
    level = 1;
  } else if (digitalRead(cfg.led2) == LOW) {
    level = 2;
  } else if (digitalRead(cfg.led3) == LOW) {
    level = 3;
  } else if (!status->power) {
    level = 0;
  } else if (status->error) {
    level = -1;
  } else {
    level = -1;
  }
	status->level = level;

  return true;
}

bool assignVentaState(struct EventStruct *event, struct venta_status *status) {
	UserVar[event->BaseVarIndex + 0] = int(status->power);
	UserVar[event->BaseVarIndex + 1] = status->level;
	UserVar[event->BaseVarIndex + 2] = status->error;
	UserVar[event->BaseVarIndex + 3] = status->open;
  return true;
}

void Plugin_198_ISR() {
  // TODO: How can I directly handle a changed LED status here without having the plugin polling 10/second?
  // success = readVentaState(event);
}


#endif
