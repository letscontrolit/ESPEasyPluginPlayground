#ifdef USES_P201

#ifdef ESP32 //only available for ESP32 due to the cpu power required and the limited ADC capabilities for ESP8266

#include "_Plugin_Helper.h"
#include "src/DataStructs/PinMode.h"
#include <driver/adc.h>
#include <WiFiUdp.h>

#define PLUGIN_201
#define PLUGIN_ID_201        201
#define PLUGIN_NAME_201       "VBAN Audio Transmitter [DEVELOPMENT]"
#define PLUGIN_VALUENAME1_201 "Level"

boolean Plugin_201_init = false;

typedef struct tagVBAN_HEADER     
{         
    char        vban[4];          /* contains 'V' 'B', 'A', 'N' */         
    uint8_t     format_SR;     /* SR index (see SRList above) */         
    uint8_t     format_nbs;    /* nb sample per frame (1 to 256) */         
    uint8_t     format_nbc;    /* nb channel (1 to 256) */         
    uint8_t     format_bit;    /* mask = 0x07 (nb Byte integer from 1 to 4) */         
    char        streamName[16];/* stream name */         
    uint32_t    nuFrame;       /* growing frame number. */     
}  __attribute__((packed)) T_VBAN_HEADER;     
 
#define VBAN_HEADER_SIZE (4 + 4 + 16 + 4) 

/*
#define VBAN_SR_MAXNUMBER 21  
static long VBAN_SRList[VBAN_SR_MAXNUMBER]= {6000, 12000, 24000, 48000, 96000, 192000, 384000, 8000, 16000, 32000, 64000, 128000, 256000, 512000, 11025, 22050, 44100, 88200, 176400, 352800, 705600}; 
*/

#define VBAN_PROTOCOL_AUDIO  0x00 
#define VBAN_PROTOCOL_SERIAL  0x20 
#define VBAN_PROTOCOL_TXT   0x40 
#define VBAN_PROTOCOL_SERVICE  0x60 
#define VBAN_PROTOCOL_UNDEFINED_1 0x80 
#define VBAN_PROTOCOL_UNDEFINED_2 0xA0 
#define VBAN_PROTOCOL_UNDEFINED_3 0xC0 
#define VBAN_PROTOCOL_USER   0xE0 

#define VBAN_DATATYPE_BYTE8   0x00 
#define VBAN_DATATYPE_INT16   0x01 
#define VBAN_DATATYPE_INT24   0x02 
#define VBAN_DATATYPE_INT32   0x03 
#define VBAN_DATATYPE_FLOAT32  0x04 
#define VBAN_DATATYPE_FLOAT64  0x05 
#define VBAN_DATATYPE_12BITS   0x06 
#define VBAN_DATATYPE_10BITS   0x07 

#define VBAN_CODEC_PCM   0x00 
#define VBAN_CODEC_VBCA   0x10 //VB-AUDIO AOIP CODEC 
#define VBAN_CODEC_VBCV   0x20 //VB-AUDIO VOIP CODEC  
#define VBAN_CODEC_UNDEFINED_1  0x30 
#define VBAN_CODEC_UNDEFINED_2  0x40 
#define VBAN_CODEC_UNDEFINED_3  0x50 
#define VBAN_CODEC_UNDEFINED_4  0x60 
#define VBAN_CODEC_UNDEFINED_5  0x70 
#define VBAN_CODEC_UNDEFINED_6  0x80 
#define VBAN_CODEC_UNDEFINED_7  0x90 
#define VBAN_CODEC_UNDEFINED_8  0xA0 
#define VBAN_CODEC_UNDEFINED_9  0xB0 
#define VBAN_CODEC_UNDEFINED_10  0xC0 
#define VBAN_CODEC_UNDEFINED_11  0xD0 
#define VBAN_CODEC_UNDEFINED_12  0xE0 
#define VBAN_CODEC_USER   0xF0

#define VBAN_PORT 6980 

/* Attenuation
When VDD_A is 3.3V:
0dB attenuaton (ADC_ATTEN_0db) gives full-scale voltage 1.1V
2.5dB attenuation (ADC_ATTEN_2_5db) gives full-scale voltage 1.5V
6dB attenuation (ADC_ATTEN_6db) gives full-scale voltage 2.2V
11dB attenuation (ADC_ATTEN_11db) gives full-scale voltage 3.9V (see note below)
*/
#define ADC_ATTENUATION ADC_ATTEN_6db
//#define ADC_CHANNEL ADC1_CHANNEL_7
adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_0;


/*-----Internal variables and buffers ----- */
uint8_t *buffer;  //a circular buffer for vban package with audio data  



uint32_t frameCounter=0; //the incrementing frame counter for vban packages
volatile int32_t transmitNow = -1; // -1=no sending; otherwise we record here the starting point a multiple of VBAN_PACKET_SIZE
uint16_t sample_idx=VBAN_HEADER_SIZE; //index for audio aquisition; set to beginning of the first data portion


uint16_t audio_power=0;


TaskHandle_t adcHandlerTask; //task to send UDP packages 
hw_timer_t * timer = NULL; //timer used to aquire audio data


WiFiUDP p201UDP; //the UDP object
IPAddress UDP_IP;// = IPAddress(192,168,100,62);
uint16_t UDP_PORT = VBAN_PORT;
int16_t AUDIO_OFFSET=0;

#define SAMPLES 256 //number of sampels per package
#define CHANNELS 1 //number of channels
//#define STREAM_NAME "Stream1" //name of the stream
#define SAMPLE_SIZE_BYTES 2

#define AUDIO_SIZE (SAMPLES * CHANNELS * SAMPLE_SIZE_BYTES)
#define VBAN_PACKET_SIZE  (VBAN_HEADER_SIZE + AUDIO_SIZE)
#define BLOCKS 2 
#define P201_BUFFER_SIZE (BLOCKS * VBAN_PACKET_SIZE)

void P201_fillHeader(struct tagVBAN_HEADER * header){
  strcpy_P(header->vban, PSTR("VBAN")); 
  header->format_SR = VBAN_PROTOCOL_AUDIO | 8; // 9; //32KHz or use  8; //16Khz or use 7; //8Khz
  header->format_nbs = SAMPLES-1;  
  header->format_nbc = CHANNELS-1; //mono   
  header->format_bit = VBAN_CODEC_PCM | VBAN_DATATYPE_INT16;    /* mask = 0x07 (nb Byte integer from 1 to 4) */         
  strcpy_P(header->streamName , Settings.Name );/* stream name max 16 chars*/         
  header->nuFrame = 0;       /* growing frame number. */     
}



#define ismulticast(addr1) (((uint32_t)(addr1) & PP_HTONL(0xf0000000UL)) == PP_HTONL(0xe0000000UL))


int P201_sendUDP(IPAddress toIP, uint16_t port, uint8_t * package, size_t length){
  if (WiFiConnected()) 
  {
    
    if (ismulticast(toIP))
    {
      if (p201UDP.beginMulticastPacket()) //(toIP, port, WiFi.localIP(),2))
      {
        p201UDP.write(package,length);
        p201UDP.endPacket();
      } 

    }
    else
      if (p201UDP.beginPacket(toIP, port))
      {
        p201UDP.write(package,length);
        p201UDP.endPacket();
      }
    return 0;
    
  }
  return -1;
}



//A plugin has to implement the following function
boolean Plugin_201(byte function, struct EventStruct *event, String& string)
{
  //function: reason the plugin was called
  //event: ??add description here??
  //string: ??add description here??

  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
        //This case defines the device characteristics, edit appropriately
        Device[++deviceCount].Number = PLUGIN_ID_201;
        Device[deviceCount].Type = DEVICE_TYPE_ANALOG;  //ANALOG-IN , AR (optional), GAIN (optiona)
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE; //type of value the plugin will return, used only for Domoticz
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;             //number of output variables. The value should match the number of keys PLUGIN_VALUENAME1_xxx
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].DecimalsOnly = true;
        break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      //return the device name
      string = F(PLUGIN_NAME_201);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      //called when the user opens the module configuration page
      //it allows to add a new row for each output variable of the plugin
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_201));
      break;
    }

    case PLUGIN_SET_DEFAULTS:
    {
      PCONFIG(0)=VBAN_PORT;
      PCONFIG(3) = 2048; //the zero position
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      //this case defines what should be displayed on the web form, when this plugin is selected
      //The user's selection will be stored in
      //PCONFIG(x) (custom configuration)

      addHtml(F("<TR><TD>Analog Pin:<TD>"));
      String adc_choices[8] = { F("GPIO-36"), F("GPIO-37"), F("GPIO-38"), F("GPIO-39"), F("GPIO-32"), F("GPIO-33"), F("GPIO-34"), F("GPIO-35") };
      addFormSelector(F("Audio Pin"), F("P201_adc"), 8, adc_choices, NULL, CONFIG_PIN1);

      // Make sure not to append data to the string variable in this PLUGIN_WEBFORM_LOAD call.
      // This has changed, so now use the appropriate functions to write directly to the Streaming
      // web_server. This takes much less memory and is faster.
      // There will be an error in the web interface if something is added to the "string" variable.

      //Use any of the following (defined at web_server.ino):
      addFormPinSelect(F("GAIN Pin"), F("taskdevicepin2"), Settings.TaskDevicePin2[event->TaskIndex]);
      addFormPinSelect(F("A/R  Pin"), F("taskdevicepin3"), Settings.TaskDevicePin3[event->TaskIndex]);
      //addFormNote(F("PIN1 = Audio IN ; PIN2 = AGC ; PIN3 = AR"));
      //To add some html, which cannot be done in the existing functions, add it in the following way:
      //addHtml(F("<TR><TD>Analog Pin:<TD>"));


      //For strings, always use the F() macro, which stores the string in flash, not in memory.

      String gain_choices[5] = { F("Auto"), F("40 Db"), F("50 Db"), F("60 Db")};
      addFormSelector(F("GAIN"), F("P201_gain"), 4, gain_choices, NULL, PCONFIG(1));

      String ar_choices[5] = { F("Auto"), F("1:500"), F("1:2000"), F("1:4000")};
      addFormSelector(F("ATTACK/RELEASE Ratio"), F("P201_AR"), 4, ar_choices, NULL, PCONFIG(2));

      //number selection (min-value - max-value)
      addFormNumericBox(F("VBAN Port"), F("P201_VBAN_PORT"), PCONFIG(0), 1, 65535);
       
      IPAddress addr(PCONFIG_LONG(0));
      byte ip[4];
      ip[0] = addr[0]; ip[1]=addr[1]; ip[2]=addr[2]; ip[3]=addr[3];
      addFormIPBox(F("Target IP"), F("P201_IP"),ip );
      //after the form has been loaded, set success and break

      //addFormCheckBox(F("Multicast"),F("P201_Multicast"), PCONFIG(4));

      addFormNumericBox(F("Zero Offset"), F("P201_offset"), PCONFIG(3), 0, 65535);

      
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      //this case defines the code to be executed when the form is submitted
      //the plugin settings should be saved to PCONFIG(x)
      //ping configuration should be read from CONFIG_PIN1 and stored
      CONFIG_PIN1 = getFormItemInt(F("P201_adc"));
      
      PCONFIG(0) = getFormItemInt(F("P201_VBAN_PORT"));
      PCONFIG(1) = getFormItemInt(F("P201_gain"));
      PCONFIG(2) = getFormItemInt(F("P201_AR"));
      PCONFIG(3) = getFormItemInt(F("P201_offset"));
      //PCONFIG(4) = isFormItemChecked(F("P201_Multicast"));
      
      IPAddress ip;
      ip.fromString(web_server.arg(F("P201_IP")).c_str());
      PCONFIG_LONG(0) = ip;

      //after the form has been saved successfuly, set success and break
      success = true;
      break;

    }
    case PLUGIN_INIT:
    {
      //this case defines code to be executed when the plugin is initialised
      UDP_IP = IPAddress(PCONFIG_LONG(0));
      UDP_PORT = PCONFIG(0);
      AUDIO_OFFSET = PCONFIG(3);
      
      
      
      if (ismulticast(UDP_IP))
      {
        addLog(LOG_LEVEL_INFO, F("IP address is Multicast"));
        p201UDP.beginMulticast(UDP_IP,UDP_PORT);
      }
      

      buffer = (uint8_t*)calloc(P201_BUFFER_SIZE,sizeof(uint8_t));
      if (buffer == NULL) 
      {
        addLog(LOG_LEVEL_ERROR,F("VBAN: Buffer Allocation error"));
      }
      else
      {
        P201_fillHeader((T_VBAN_HEADER *)buffer);
        P201_fillHeader((T_VBAN_HEADER *)(buffer+VBAN_PACKET_SIZE));
      }

      
      String log = F("VBAN: sending to: ");
      log+=UDP_IP.toString();
      log+=F(":");
      log+=UDP_PORT;
      log+=F(" stream=");
      log+=Settings.Name ;
      addLog(LOG_LEVEL_INFO,log);
      

      /*
      Trilevel Amplifier Gain Control.
       GAIN = VDD, gain set to 40dB. 
       GAIN = GND, gain set to 50dB. 
       GAIN = Unconnected, uncompressed gain set to 60dB. 
      */
      if (CONFIG_PIN2>0)
       switch(PCONFIG(1)){
          case 1: //40db
            pinMode(CONFIG_PIN2,OUTPUT);
            digitalWrite(CONFIG_PIN2,HIGH);
            break;
          case 2: //50db
            pinMode(CONFIG_PIN2,OUTPUT);
            digitalWrite(CONFIG_PIN2,LOW);
            break;
          case 3: //60db
          default:
          pinMode(CONFIG_PIN2, INPUT); //high Z
        }

      /*
      Trilevel Attack and Release Ratio Select. Controls the ratio of attack time to release time for the AGC circuit.
       A/R = GND: Attack/Release Ratio is 1:500 
       A/R = VDD: Attack/Release Ratio is 1:2000 
       A/R = Unconnected: Attack/Release Ratio is 1:4000
      */
      if (CONFIG_PIN3>0)
        switch(PCONFIG(2)){
          case 1: //AR=1:500
            pinMode(CONFIG_PIN3,OUTPUT);
            digitalWrite(CONFIG_PIN3,LOW);
            break;
          case 2: //AR=1:2000
            pinMode(CONFIG_PIN3,OUTPUT);
            digitalWrite(CONFIG_PIN3,HIGH);
            break;
          case 3: //60db
          default:
          pinMode(CONFIG_PIN3, INPUT); //high Z
        }
      
      //p201UDP.begin(VBAN_PORT);


      //WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

      
      ADC_CHANNEL = (adc1_channel_t)digitalPinToAnalogChannel(CONFIG_PIN1);
      
      {
      adc1_config_width(ADC_WIDTH_12Bit); // configure the analogue to digital converter
      adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTENUATION); //ADC_ATTEN_11db); // connects the ADC 1 with channel 7 (GPIO 35)
      adc1_get_raw(ADC_CHANNEL); //we need this to initialize the ADC correctly
      }

      xTaskCreate(adcHandler, "Handler Task", 5000, NULL, 1, &adcHandlerTask);

      //timer = timerBegin(0, 80, true); // 80 Prescaler => 1 microseconds for 8Khz
      timer = timerBegin(0, 40, true); // 40 Prescaler => 0.5 microseconds  for 16Khz
      //timer = timerBegin(0, 20, true); // 20 Prescaler => 0.25 microseconds  for 32Khz
      timerAttachInterrupt(timer, &onTimer, true); // binds the handling function to our timer 
      timerAlarmWrite(timer, 125, true); //x*8Khz sample rate
      timerAlarmEnable(timer);


      //after the plugin has been initialised successfuly, set success and break
      log = F("VBAN: Initialized");
      addLog(LOG_LEVEL_INFO,log);
      success = true;
      break;

    }

    case PLUGIN_READ:
    {
      //code to be executed to read data
      //It is executed according to the delay configured on the device configuration page, only once

      //after the plugin has read data successfuly, set success and break
      //success = true;
      break;

    }

    case PLUGIN_WRITE:
    {
      //this case defines code to be executed when the plugin executes an action (command).
      //Commands can be accessed via rules or via http.
      //As an example, http://192.168.1.12//control?cmd=dothis
      //implies that there exists the comamnd "dothis"

      //if (plugin_not_initialised)
       // break;

      //implement any commands here

       break;
    }

	case PLUGIN_EXIT:
	{
	  //perform cleanup tasks here. For example, free memory
    free(buffer);
    buffer= NULL;
	  break;

	}
    
    case PLUGIN_ONCE_A_SECOND:
    {
      //code to be executed once a second. Tasks which do not require fast response can be added here
      UserVar[event->BaseVarIndex] = audio_power;
      success = true;
      
      break;
    }
  
    /*
    case PLUGIN_TEN_PER_SECOND:
    {
      //code to be executed 10 times per second. Tasks which require fast response can be added here
      //be careful on what is added here. Heavy processing will result in slowing the module down!
      
      success = true;
      break;
    
    
    }

  
    case PLUGIN_FIFTY_PER_SECOND:
    {

      success = true;
      break;
    }
    */
  }   // switch
  return success;

}     //main plugin function



void adcHandler(void *param) {
  while (true) {
    // Sleep until the ISR gives us something to do, or for 1 second
    ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));  
    if (WiFiConnected()) 
    if (transmitNow>=0){
        //filling the frame counter on the header before sending.
        uint8_t *header = (buffer+transmitNow);
        
        *(header+24) = (uint8_t)counter; //LSB
        *(header+25) = (uint8_t)(counter>>8);
        *(header+26) = (uint8_t)(counter>>16);
        *(header+27) = (uint8_t)(counter>>24); //MSB
        
        P201_sendUDP(UDP_IP, UDP_PORT, (buffer+transmitNow),VBAN_PACKET_SIZE);
        frameCounter++;
        transmitNow = -1;
       // addLog(LOG_LEVEL_INFO,"VBAN: sent");
       
       //processing the buffer further
       uint16_t * audio = (uint16_t *)(buffer+transmitNow);
       int16_t max=0,min=0xffff;
       
       for(int i=0;i<SAMPLES;i++)
       {
         int16_t val = audio[i];
         if (val>min) min=val;
         if (val<max) max=val;
       } 
       audio_power = max-min;
      }
  
  }
}


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; //mutex for the timer
volatile int16_t last_val=-1;
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux); // says that we want to run critical code and don't want to be interrupted
  
  int16_t adcVal = isr_adc1_read(ADC_CHANNEL); //int adcVal = adc1_get_raw(ADC1_CHANNEL_7); // reads the ADC
  //int16_t adcNoise = isr_adc1_read(ADC1_CHANNEL_1); //int adcVal = adc1_get_raw(ADC1_CHANNEL_7); // reads the ADC
  
  if (last_val==-1)
  {
    last_val=adcVal;
    portEXIT_CRITICAL_ISR(&timerMux); // says that we have run our critical code
    return;
  }

  //average the current and last value for noise filtering
  adcVal = (last_val + adcVal) >> 1;
  last_val = adcVal;
  //uint8_t value = isr_map(adcVal, 0 , 4096, 0, 255);  // converts the value to 0..255 (8bit)
  //uint8_t value = isr_map(adcVal-1024, 0 , 4096-1024, 0, 255);  // converts the value to 0..255 (8bit)
  //*(buffer+sample_idx) = value; // stores the value
  adcVal-=AUDIO_OFFSET;
  //adcVal-=2048;
  *(buffer+sample_idx) = (uint8_t) (adcVal);
  sample_idx++;
  *(buffer+sample_idx) = (uint8_t) (adcVal>>8);
  sample_idx++;
  
  if (sample_idx % VBAN_PACKET_SIZE == 0) { // when the buffer is full
    transmitNow = sample_idx - VBAN_PACKET_SIZE; //we can transmit now

    sample_idx = (sample_idx + VBAN_HEADER_SIZE) % P201_BUFFER_SIZE; //move to next data block
    
    // Notify adcTask that the buffer is full.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(adcHandlerTask, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {  portYIELD_FROM_ISR();}

    //memcpy(transmitBuffer, audioBuffer, AUDIO_BUFFER_MAX); // copy buffer into a second buffer
  }

  portEXIT_CRITICAL_ISR(&timerMux); // says that we have run our critical code
}


/*------------------------------------------------------------------------------------------------*/

/*
uint8_t IRAM_ATTR isr_map(long x, long in_min, long in_max, long out_min, long out_max) {
    long divisor = (in_max - in_min);
    if(divisor == 0){
        return -1; //AVR returns -1, SAM returns 0
    }
    return (x - in_min) * (out_max - out_min) / divisor + out_min;
}
*/

//from: https://www.toptal.com/embedded/esp32-audio-sampling
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>
uint16_t IRAM_ATTR isr_adc1_read(int channel) {
    uint16_t adc_value;
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    adc_value = SENS.sar_meas_start1.meas1_data_sar;
    return adc_value;
}

#endif //ESP32
#endif 