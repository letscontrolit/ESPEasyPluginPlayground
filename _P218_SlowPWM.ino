#ifdef USES_P218
#define PLUGIN_218
#define PLUGIN_ID_218        218
#define PLUGIN_NAME_218      "Regulator - Slow/Soft PWM [TESTING]"

#define PLUGIN_VALUENAME1_218 "Output"


#define PLUGINT_DEFAULT_PERIOD_218 1024
#define PLUGINT_MAX_PERIOD_218 30000
#define PLUGINT_MIN_PERIOD_218 4

// A plugin has implement the Bresenham PWM fo slow devices (heaters and so on)

/*
  Main reason to have slow PWN.... Sometimes we need to works with AC.
  You need to get a part of period/search zero/... to make able power regulation on AC.... 
  or (for slow device/heaters) control of series of periods (triac)
  
  So I use 1 sec as base frequency for PWM in this plugin. You can switch it to 1/10 sec as you wish.
  Of course you should understand the consequences.

  I use it with heater/regulator. It is incomparably better than _P021_Level to make temperature stable.
  The plugin take float from another plugin [0-1] (power factor) and use to control power [0-100]%
  Values less 0 use as 0 (off). Values greater 1 assume as 1 (100% power).

  I use it as follows:
  DS18b20 (P004_Dallas) temperature -> 
  "Rule" calculetes power and put it to P033_Dummy (e.g. TaskValueSet,3,1,(22-%eventvalue%)/16 )
  This power factor is used by this plugin.

  So I have P (proporcional) power regulator.
  This can work offline.

  You can implement any robust power controller by using different calculation or/and external calculations of power factor.
*/

static_assert(VARS_PER_TASK >= 3, "VARS_PER_TASK should be greater or equal 3");
static_assert(PLUGIN_CONFIGVAR_MAX >= 4, "PLUGIN_CONFIGVAR_MAX should be greater or equal 4");

// compiler fails on c++ style code. will try C style

// I want to use type inference but compiler fails.
// check that the types have not changed
static_assert(std::is_same<std::remove_all_extents<decltype(Settings.TaskDevicePluginConfigFloat)>::type, float>::value, "Needs to update types in plugin");
static_assert(std::is_same<std::remove_all_extents<decltype(Settings.TaskDevicePluginConfig)>::type, int16_t>::value, "Needs to update types in plugin");
static_assert(sizeof(float) >= 2 * sizeof(int16_t), "plugin use memory allocatied for float as two int16");

#define GET_CONFIG_INT_218(event, id)           (Settings.TaskDevicePluginConfig[event->TaskIndex][id])
#define SET_CONFIG_INT_218(event, id, value)    (Settings.TaskDevicePluginConfig[event->TaskIndex][id] = (value))

#define GET_USER_FLOAT_POINTER_218(event, id)   (&(UserVar[event->BaseVarIndex + id]))

// to avoid the “strict aliasing” warning that appears when make this by using “define”
// compiler produce bad code if all "#define" replace by "template inline"
template<typename T>
inline int16_t * GET_HIGH_HALF_FLOAT_AS_INT_POINTER_218(T event, int16_t id)
{
  return reinterpret_cast<int16_t*>(GET_USER_FLOAT_POINTER_218(event, id));
}

//#define GET_HIGH_HALF_FLOAT_AS_INT_POINTER_218(event, id) ((int16_t *)(reinterpret_cast<int16_t*>(GET_USER_FLOAT_POINTER_218(event, id))))
#define GET_LOW_HALF_FLOAT_AS_INT_POINTER_218(event, id)  (&(GET_HIGH_HALF_FLOAT_AS_INT_POINTER_218(event, id)[1]))

// config int
#define PLUGIN_CONTROLLED_ID_218       0
#define PLUGIN_CONTROLLED_VAR_ID_218   1
#define PLUGIN_PERIOD_218              2
//#define PLUGIN_FLAGS_218               3

// flag id (checkbox)
#define FAST_PWM_ID_218                0
#define INVERT_OUTPTUT_218             1

// user var
#define PLUGIN_SHOW_STATE_218          0
#define PLUGIN_PACKED_INT12_218        1 // state + error pwm
#define PLUGIN_PACKED_INT34_218        2 // step  + previous value pwm

#define SET_CONTROLLED_TASK_218(event, value)         (SET_CONFIG_INT_218(event, PLUGIN_CONTROLLED_ID_218, value))
#define GET_CONTROLLED_TASK_218(event)                (GET_CONFIG_INT_218(event, PLUGIN_CONTROLLED_ID_218))

#define SET_CONTROLLED_TASK_VAR_ID_218(event, value)  (SET_CONFIG_INT_218(event, PLUGIN_CONTROLLED_VAR_ID_218, value))
#define GET_CONTROLLED_TASK_VAR_ID_218(event)         (GET_CONFIG_INT_218(event, PLUGIN_CONTROLLED_VAR_ID_218))

#define GET_PERIOD_218(event)                         (GET_CONFIG_INT_218(event, PLUGIN_PERIOD_218))
#define SET_PERIOD_218(event, value)                  (SET_CONFIG_INT_218(event, PLUGIN_PERIOD_218, value))

#define GET_PWM_CUR_SATE_POINTER_218(event)           (GET_HIGH_HALF_FLOAT_AS_INT_POINTER_218(event, PLUGIN_PACKED_INT12_218))
#define GET_PWM_ERROR_POINTER_218(event)              (GET_LOW_HALF_FLOAT_AS_INT_POINTER_218(event, PLUGIN_PACKED_INT12_218))
#define GET_PWM_STEP_POINTER_218(event)               (GET_HIGH_HALF_FLOAT_AS_INT_POINTER_218(event, PLUGIN_PACKED_INT34_218))
#define GET_PWM_PREVIOUS_VALUE_POINTER_218(event)     (GET_LOW_HALF_FLOAT_AS_INT_POINTER_218(event, PLUGIN_PACKED_INT34_218))
#define GET_PWM_CUR_SATE_SHOW_POINTER_218(event)      (GET_USER_FLOAT_POINTER_218(event, PLUGIN_SHOW_STATE_218))

//#define GET_PLUGIN_FLAG_POINTER_218(event)            (&(GET_CONFIG_INT_218(event, PLUGIN_FLAGS_218)))
//#define GET_PLUGIN_FLAG_218(event, mask)              (((*GET_PLUGIN_FLAG_POINTER_218(event)) & (1 << mask)) != 0)
//#define SET_PLUGIN_FLAG_218(event, mask, data)        (data == 0 ? ((*GET_PLUGIN_FLAG_POINTER_218(event)) &= ~(1 << mask)) : ((*GET_PLUGIN_FLAG_POINTER_218(event)) |= (1 << mask)))
#define GET_PLUGIN_FLAG_218(event, mask)              (Settings.TaskDevicePluginConfigLong[event->TaskIndex][mask] != 0)
#define SET_PLUGIN_FLAG_218(event, mask, data)        (Settings.TaskDevicePluginConfigLong[event->TaskIndex][mask] = data)

boolean Plugin_218(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        //This case defines the device characteristics, edit appropriately

        Device[++deviceCount].Number = PLUGIN_ID_218;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;  // how the device is connected
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH; // SENSOR_TYPE_NONE; //type of value the plugin will return, used only for Domoticz
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;             //number of output variables. The value should match the number of keys PLUGIN_VALUENAME1_xxx
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      //return the device name
      string = F(PLUGIN_NAME_218);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_218));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      addHtml(F("<TR><TD>Check Task:<TD>"));
      addTaskSelect(F("plugin_218_task"), GET_CONTROLLED_TASK_218(event));

      LoadTaskSettings(GET_CONTROLLED_TASK_218(event)); // we need to load the values from another task for selection!
      addHtml(F("<TR><TD>Check Value:<TD>"));
      addTaskValueSelect(F("plugin_218_value"), GET_CONTROLLED_TASK_VAR_ID_218(event), GET_CONTROLLED_TASK_218(event));

      addFormNumericBox(F("Set Period"),
        F("plugin_218_period"), 
        p218_normolize_value(GET_PERIOD_218(event), PLUGINT_MIN_PERIOD_218, PLUGINT_MAX_PERIOD_218),
        PLUGINT_MIN_PERIOD_218,
        PLUGINT_MAX_PERIOD_218
      );

      addFormCheckBox(F("Invert output"), F("plugin_218_invert_output"), GET_PLUGIN_FLAG_218(event, INVERT_OUTPTUT_218));
      addFormCheckBox(F("use 1/10s period"),  F("plugin_218_tenth"), GET_PLUGIN_FLAG_218(event, FAST_PWM_ID_218));
      
      LoadTaskSettings(event->TaskIndex);
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      SET_CONTROLLED_TASK_218(event, getFormItemInt(F("plugin_218_task")));
      SET_CONTROLLED_TASK_VAR_ID_218(event, getFormItemInt(F("plugin_218_value")));
      SET_PERIOD_218(event, p218_normolize_value(getFormItemInt(F("plugin_218_period")), PLUGINT_MIN_PERIOD_218, PLUGINT_MAX_PERIOD_218));

      SET_PLUGIN_FLAG_218(event, INVERT_OUTPTUT_218, isFormItemChecked(F("plugin_218_invert_output")));
      SET_PLUGIN_FLAG_218(event, FAST_PWM_ID_218, isFormItemChecked(F("plugin_218_tenth")));

      p218_init_settings(event);
      
      success = true;
      break;

    }
    case PLUGIN_INIT:
    {
      pinMode(Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);
      success = true;
      break;

    }

	  case PLUGIN_EXIT:
	  {
	    digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], GET_PLUGIN_FLAG_218(event, INVERT_OUTPTUT_218) ? 1 : 0);
      success = true;
	    break;

	  }

    case PLUGIN_TEN_PER_SECOND:
    {
      if (GET_PLUGIN_FLAG_218(event, FAST_PWM_ID_218)) {
        p218_next_step(event);
      }
      success = true;
      break;
    }


    case PLUGIN_ONCE_A_SECOND:
    {
      if (!GET_PLUGIN_FLAG_218(event, FAST_PWM_ID_218)) {
        p218_next_step(event);
      }
      success = true;
      break;
    }
  }   // switch
  return success;

}     //function

template<typename T>
inline T p218_normolize_value(T t, int16_t min, int16_t max)
{
  if (t < min)
    return min;
  if (t > max)
    return max;
  return t;
}

void p218_init_settings(struct EventStruct *event)
{
  *GET_PWM_CUR_SATE_SHOW_POINTER_218(event) = GET_PLUGIN_FLAG_218(event, INVERT_OUTPTUT_218) ? 1.0 : 0.0;
  *GET_PWM_CUR_SATE_POINTER_218(event) = 0;
  *GET_PWM_ERROR_POINTER_218(event) = GET_PERIOD_218(event) / 2;
  *GET_PWM_STEP_POINTER_218(event) = 0;
}

void p218_next_step(struct EventStruct *event)
{
  int16_t  period = GET_PERIOD_218(event);
  int16_t  value;
  {
    int16_t* prev = GET_PWM_PREVIOUS_VALUE_POINTER_218(event);
    int16_t BaseVarIndex = GET_CONTROLLED_TASK_218(event) * VARS_PER_TASK + GET_CONTROLLED_TASK_VAR_ID_218(event);
    float float_value = UserVar[BaseVarIndex];
    if (float_value == float_value) { //  check if value is NaN, use previous value
      value =  p218_normolize_value(float_value * period, 0 , period); // min/max power
      *prev = value; 
    } else {
      value = *prev;
    }
  }
  int16_t* error = GET_PWM_ERROR_POINTER_218(event);
  int16_t* stepNumber = GET_PWM_STEP_POINTER_218(event);
  int16_t* curState = GET_PWM_CUR_SATE_POINTER_218(event);

  int16_t result;
  *error -= value;
  if ( *error < 0 ) {
    *error += period;
    result = 1;
  } else {
    result = 0;
  }
  if ( ++(*stepNumber) >= period) {
    *stepNumber = 0;
    *error = (period) / 2;
  }
  
  if (*curState != result) {
    *curState = result;

    if (GET_PLUGIN_FLAG_218(event, INVERT_OUTPTUT_218)) {
      result = 1 - result;
    }

    digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], result);
    *GET_PWM_CUR_SATE_SHOW_POINTER_218(event) = result;
    sendData(event);
  }
}

#endif
