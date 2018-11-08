#ifdef USES_P218
//#######################################################################################################
//#################################### Plugin 218: Blind Control ################################################
//#######################################################################################################

/*

this plugin controles the level of a blind or a windowopener in numbers 0-100.
for example using solidstate relays to control the direction of movement (connected to the motor).
a attacht potentiometer like a "spindeltrimmer" with more than one turn is connected to the esp adc and the slowest gear of the transmission.
a slider/fader can be added to control the level manualy. what ever was changed last, silder or external control, wil be the new target value,
the motor will reach to until the potentiometer reach that value.

plugin written by Lars Reinhardt electronic_iq@hotmail.com

this plugin is based on the _P033_Dummy.ino

*/

#ifdef PLUGIN_BUILD_DEV

    int16_t SliderWas;                                                        // Teile dieser variablen dienen als Schnitstelle
    int16_t TargetValue;
    int16_t CurrentValue;
    int8_t dir;
    int8_t run;

#define PLUGIN_218
#define PLUGIN_ID_218         218
#define PLUGIN_NAME_218       "Regulator - Blind/Window Control [DEVELOPMENT]"
#define PLUGIN_VALUENAME1_218 "CurrentValue"
#define PLUGIN_VALUENAME3_218 "SliderIs"
#define PLUGIN_VALUENAME2_218 "TargetValue"

boolean Plugin_218(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_218;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;                          // bestimmt die anschlussart, hier 3 GPIO. andere I2C, etc...
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;                         // bestimmt die anzahl und art der variablen für die schnittstelle
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].DecimalsOnly = true;
        Device[deviceCount].ValueCount = 3;                                     // bestimmt die anzahl der variablen für die schnittstelle?
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_218);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_218));             // namen der variablen wie im #define angegeben werden übergeben
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_218));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_218));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:                                                                         // teile der konfiguration werden hier bestimmt
      {

        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];                           // die auswahl box ob fenster oder rollo setup gewünscht ist
        String options[2];
        options[0] = F("Blind");
        options[1] = F("Window");
        int optionValues[2];
        optionValues[0] = SENSOR_TYPE_TRIPLE;                                                         // hier war ich zu dumm. konnte den namen nicht frei wählen. musste passende option wie im plugin dummy wählen.
        optionValues[1] = SENSOR_TYPE_DUAL;                                                           // wie macht man das richtig?

        addHtml(F("<br />   1st GPIO = down/run   2nd GPIO = up/dir   3rd GPIO = Blindsensor Pin   Slider Pin is GPIO 12 (D6)"));       // text um die zuordnung der GPIOs zu erklären, schöner wäre die bezeichnung direkt vor der auswahlbox zu haben. auch hier war ich zu dumm!

        addFormSelector(F("Control Type"), F("plugin_218_sensortype"), 2, options, optionValues, choice );

        addFormNumericBox(F("Slider 1"), F("plugin_002_adc1"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][0], 0, 1023);      // Box um die "callibrationswerte" in der gui abzufragen
        addFormNumericBox(F("Slider 2"), F("plugin_002_adc2"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][1], 0, 1023);
        addFormNumericBox(F("Blind 1"), F("plugin_002_adc3"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][2], 0, 1023);
        addFormNumericBox(F("Blind 2"), F("plugin_002_adc4"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][3], 0, 1023);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:                                                                                       // hier werden die oben eingetragenen werte gespeichert
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_218_sensortype"));

        Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = getFormItemInt(F("plugin_002_adc1"));
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = getFormItemInt(F("plugin_002_adc2"));
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][2] = getFormItemInt(F("plugin_002_adc3"));
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][3] = getFormItemInt(F("plugin_002_adc4"));

        success = true;
        break;
      }

    case PLUGIN_READ:                                                                           // das kann bestimmt noch ganz anders aussehen. hier habe ich nichts geändert, alles original vom dummy plugin kopiert
      {
        event->sensorType = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        for (byte x=0; x<3;x++)
        {
          String log = F("Dummy: value ");
          log += x+1;
          log += F(": ");
          log += UserVar[event->BaseVarIndex+x];
          addLog(LOG_LEVEL_INFO,log);
        }
        success = true;
        break;
      }

          case PLUGIN_TEN_PER_SECOND:                                     // hier beginnt der cod für die regelung. er wird 10 mal pro sekunde ausgeführt. das genügt für die steuerung, sie ist sehr langsam
      {

        int8_t slider = 12;                                                       // leider gehen nur 3 pins über die gui auszuwählen, darum muss dieser pin hier gewählt werden
        int8_t up = Settings.TaskDevicePin2[event->TaskIndex];                    // pin15 in meinem serup
        int8_t down = Settings.TaskDevicePin1[event->TaskIndex];                  // pin13
        int8_t blind = Settings.TaskDevicePin3[event->TaskIndex];                 // pin14
        int slider1 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][0];   // "calibrationswerte" für die normierung
        int slider2 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][1];   //
        int blind1 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][2];    //
        int blind2 = Settings.TaskDevicePluginConfigLong[event->TaskIndex][3];    //

byte Par3 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];         // abfragr was gesteuert werden soll.
switch (Par3) {
  case SENSOR_TYPE_DUAL:                                                  // cod für die fenstermotorsteuerung
 dir = Settings.TaskDevicePin2[event->TaskIndex];                         // pin um zuentscheiden ob das fenster auf oder zu gehen soll
 run = Settings.TaskDevicePin1[event->TaskIndex];                         // pin um den antrieb einzuschalten

pinMode(dir, OUTPUT);
pinMode(run, OUTPUT);

  CurrentValue = analogRead(A0);                                        // position des fensters auslesen
  CurrentValue = (int)((CurrentValue-blind1)*100/(blind2-blind1));      // normieren und kommastellen abschneiden. besser runden? #include <math.h> float f; int i = roundf(f);
                                                                        // eigentlich sollte es nicht auf die kommastellen ankommen
  UserVar[event->BaseVarIndex] = (float)CurrentValue;                   // wert der position des fensters an schnittstellen übergeben

   TargetValue = UserVar[event->BaseVarIndex + 1];                      // sollwert der position des fensters abfragen

  if (CurrentValue > TargetValue)                                       // wenn ist und sollwert von einander abweichen...
  {
    if (CurrentValue - TargetValue > 2)                                 // ...gucken ob threshhold überschritten wird. der threshhold sorgt dafür das ein schwanken des wertes ignoriert wird
    {
    digitalWrite(run, HIGH);                                            // ...wenn ja, antrieb starten
    digitalWrite(dir, HIGH);
    }
  }
  else if (CurrentValue < TargetValue)                                  // das gleiche für die andere richtung
   {
    if (TargetValue - CurrentValue > 2)
    {
    digitalWrite(dir, LOW);
    digitalWrite(run, HIGH);
    }
   }
   else                                                                 // sind ist und sollwert gleich genug, soll der motor aus seien, bzw stoppen weil sollwert erreicht
   {
    digitalWrite(dir, LOW);
    digitalWrite(run, LOW);
   }

break;

  case SENSOR_TYPE_TRIPLE:                                                      // cod für die rolladensteuerung

          pinMode(up, OUTPUT);                                                  // pin für rauf fahren
          pinMode(down, OUTPUT);                                                // pin für runter fahren
          pinMode(slider, OUTPUT);                                              // sensoren für slider und mororposition werden gemultiplext
          pinMode(blind, OUTPUT);

          digitalWrite(blind, HIGH);                                            // zuerst wird der sensor für die position des rollos abgefragt
          delay(3);                                                             // kleines delay um dem adc genug zeit einzuräumen, evtl überflüssig
          CurrentValue = analogRead(A0);                                        // position auslesen
          CurrentValue = (int)((CurrentValue-blind1)*100/(blind2-blind1));      // normieren und kommastellen abschneiden. besser runden? #include <math.h> float f; int i = roundf(f);
                                                                                // eigentlich sollte es nicht auf die kommastellen ankommen
          UserVar[event->BaseVarIndex] = (float)CurrentValue;                   // wert der position des fensters an schnittstellen übergeben
          digitalWrite(blind, LOW);                                             // sensor wieder abschalten

          digitalWrite(slider, HIGH);                                           // jetzt wird der sensor für die position des sliders abgefragt
          delay(3);

          CurrentValue = analogRead(A0);                                            // position auslesen
          CurrentValue = (int)((CurrentValue-slider1)*100/(slider2-slider1));       // normieren
          UserVar[event->BaseVarIndex + 2] = (float)CurrentValue;                   // wert der position des sliders an schnittstellen übergeben
          digitalWrite(slider, LOW);                                                // sensor wieder abschalten

          if (CurrentValue > SliderWas)                                         // hat sich der slider seit letztem durchlauf bewegt?...
          {
            if (CurrentValue - SliderWas > 2)                                   // ...gucken ob threshhold überschritten wird. der threshhold sorgt dafür das ein schwanken des wertes ignoriert wird
            {
            TargetValue = CurrentValue;                                         // ...wenn ja, als neuen sollwert für
            UserVar[event->BaseVarIndex + 1] = TargetValue;                     // schnittstellenwert aktualisieren
            SliderWas = CurrentValue;                                           // wert merken um im nächsten durchlauf eine änderung zubemerken
            }
          }
          if (CurrentValue < SliderWas)                                         // das gleiche für die andere richtung
           {
            if (SliderWas - CurrentValue > 2)                                   // ...gucken ob threshhold überschritten wird. der threshhold sorgt dafür das ein schwanken des wertes ignoriert wird
            {
            TargetValue = CurrentValue;
            UserVar[event->BaseVarIndex + 1] = TargetValue;
            SliderWas = CurrentValue;
            }
           }

          TargetValue = UserVar[event->BaseVarIndex + 1];                       // sollwert einlesen, falls er nicht vom slider geändert wurde, kann er extern geändert worden sein (fhem, OpenHAB oder so)


          if (CurrentValue > TargetValue)                                       // wenn ist und sollwert von einander abweichen...
          {
            if (CurrentValue - TargetValue > 2)                                 // ...wenn ja, antrieb starten
            {
            digitalWrite(up, LOW);
            digitalWrite(down, HIGH);
            }
          }
          else if (CurrentValue < TargetValue)                                  // das gleiche für die andere richtung
           {
            if (TargetValue - CurrentValue > 2)
            {
            digitalWrite(down, LOW);
            digitalWrite(up, HIGH);
            }
           }
           else
           {
            digitalWrite(down, LOW);                                            // sind ist und sollwert gleich genug, soll der motor aus seien, bzw stoppen weil sollwert erreicht
            digitalWrite(up, LOW);
           }


        success = true;
        break;
      }
}
  }
  return success;
}
#endif // USES_P218
