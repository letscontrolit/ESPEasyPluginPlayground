#ifdef USES_P123
//#######################################################################################################
//######################## Plugin 123 SI7013 I2C Temperature Humidity Sensor  ###########################
//#######################################################################################################
// 08-06-2019 Florin Muntean

#define PLUGIN_123
#define PLUGIN_ID_123        123
#define PLUGIN_NAME_123       "Environment - SI7013"
#define PLUGIN_VALUENAME1_123 "Temperature"
#define PLUGIN_VALUENAME2_123 "Humidity"
#define PLUGIN_VALUENAME3_123 "ADC"

boolean Plugin_123_init = false;

// ======================================
// SI7013 sensor
// ======================================
#define SI7021_I2C_ADDRESS      0x40 // I2C address for the sensor
#define SI7013_I2C_ADDRESS      0x41 // I2C address for the sensor
#define SI7013_MEASURE_TEMP_HUM 0xE0 // Measure Temp only after a RH conversion done

#define SI7013_MEASURE_HUM      0xF5 // No hold
#define SI7013_MEASURE_TEMP     0xF3 // No hold

#define SI7013_MEASURE_TEMP_HM  0xE3 // Default hold Master
#define SI7013_MEASURE_HUM_HM   0xE5 // Default hold Master
#define SI7013_WRITE_REG1        0xE6
#define SI7013_READ_REG1         0xE7
#define SI7013_SOFT_RESET       0xFE

#define SI7013_READ_ADC         0xEE
#define SI7013_READ_REG2        0x10
#define SI7013_WRITE_REG2       0x50


// SI7013 Sensor resolution
// default at power up is SI7013_RESOLUTION_14T_12RH
#define SI7013_RESOLUTION_14T_12RH 0x00 // 12 bits RH / 14 bits Temp
#define SI7013_RESOLUTION_13T_10RH 0x80 // 10 bits RH / 13 bits Temp
#define SI7013_RESOLUTION_12T_08RH 0x01 //  8 bits RH / 12 bits Temp
#define SI7013_RESOLUTION_11T_11RH 0x81 // 11 bits RH / 11 bits Temp
#define SI7013_RESOLUTION_MASK 0B01111110


uint16_t si7013_humidity;    // latest humidity value read
int16_t  si7013_temperature; // latest temperature value read (*100)
int32_t si7013_adc;        //moving average
//uint8_t MOVING_AVERAGE_power 4 //2^4

int Plugin_123_i2c_addresses[2] = { SI7021_I2C_ADDRESS, SI7013_I2C_ADDRESS };

uint8_t Plugin_123_i2c_addr(struct EventStruct *event) {
   return (uint8_t)PCONFIG(1);
}

uint8_t Plugin_123_device_index(const uint8_t i2caddr) {
  switch(i2caddr) {
    case SI7021_I2C_ADDRESS:   return 0u;
    case SI7013_I2C_ADDRESS:  return 1u;
    
  }
  return 1u; // Some default
}


boolean Plugin_123(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_123;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM_BARO; //using this type in order to send the 3rd value which is the ADC 
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_123);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_123));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_123));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_123));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        #define SI7013_RESOLUTION_OPTION 4

        byte choice = PCONFIG(0);
        String options[SI7013_RESOLUTION_OPTION];
        int optionValues[SI7013_RESOLUTION_OPTION];
        optionValues[0] = SI7013_RESOLUTION_14T_12RH;
        options[0] = F("Temp 14 bits / RH 12 bits");
        optionValues[1] = SI7013_RESOLUTION_13T_10RH;
        options[1] = F("Temp 13 bits / RH 10 bits");
        optionValues[2] = SI7013_RESOLUTION_12T_08RH;
        options[2] = F("Temp 12 bits / RH  8 bits");
        optionValues[3] = SI7013_RESOLUTION_11T_11RH;
        options[3] = F("Temp 11 bits / RH 11 bits");
        addFormSelector(F("Resolution"), F("p014_res"), SI7013_RESOLUTION_OPTION, options, optionValues, choice);
        //addUnit(F("bits"));

        addFormSelectorI2C(F("p123_i2c"), 2, Plugin_123_i2c_addresses, Plugin_123_i2c_addr(event));
 
        addFormNumericBox("Filter Power", F("p123_filter"), PCONFIG(2), 0, 4);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p123_res"));
        PCONFIG(1) = getFormItemInt(F("p123_i2c"));
        PCONFIG(2) = getFormItemInt(F("p123_filter"));
        Plugin_123_init = false; // Force device setup next time

        success = true;
        break;
      }

    case PLUGIN_READ:
      {
       // Get sensor resolution configuration
        uint8_t res = PCONFIG(0);
        uint8_t filter_power = PCONFIG(2);
        const uint8_t i2caddr =  Plugin_123_i2c_addr(event);

        

        if (!Plugin_123_init) {
          Plugin_123_init = Plugin_123_si7013_begin(i2caddr,res);
          Plugin_123_si7013_readADC(i2caddr,filter_power);
          si7013_adc = si7013_adc << filter_power; //this is the first measurement 
        }

        // Read values only if init has been done okay
        if (Plugin_123_init && Plugin_123_si7013_readValues(i2caddr,res,filter_power) == 0) {
          UserVar[event->BaseVarIndex] = si7013_temperature/100.0;
          UserVar[event->BaseVarIndex + 1] = si7013_humidity / 10.0;
          UserVar[event->BaseVarIndex + 2] = si7013_adc >> filter_power;
          success = true;
          
          /*
          String log = F("SI7013 : Temperature: ");
          log += UserVar[event->BaseVarIndex];
          addLog(LOG_LEVEL_INFO,log);
          log = F("SI7013 : Humidity: ");
          log += UserVar[event->BaseVarIndex + 1];
          addLog(LOG_LEVEL_INFO,log);
          */
        } else {
          addLog(LOG_LEVEL_INFO,F("SI7013 : Read Error!"));
        }

        break;
      }

  }
  return success;
}

/* ======================================================================
Function: Plugin_123_si7013_begin
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : true if okay
Comments: -
====================================================================== */
boolean Plugin_123_si7013_begin(uint8_t i2caddr, uint8_t resolution)
{
  uint8_t ret;

  // Set the resolution we want
  ret = Plugin_123_si7013_setResolution(i2caddr, resolution);
  if ( ret == 0 ) {
    ret = true;
  } else {
    String log = F("SI7013 : Res=0x");
    log += String(resolution,HEX);
    log += F(" => Error 0x");
    log += String(ret,HEX);
    addLog(LOG_LEVEL_INFO,log);
    ret = false;
  }

  return ret;
}

/* ======================================================================
Function: Plugin_123_si7013_checkCRC
Purpose : check the CRC of received data
Input   : value read from sensor
Output  : CRC read from sensor
Comments: 0 if okay
====================================================================== */
uint8_t Plugin_123_si7013_checkCRC(uint16_t data, uint8_t check)
{
  uint32_t remainder, divisor;

  //Pad with 8 bits because we have to add in the check value
  remainder = (uint32_t)data << 8;

  // From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  // POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
  // 0x988000 is the 0x0131 polynomial shifted to farthest left of three bytes
  divisor = (uint32_t) 0x988000;

  // Add the check value
  remainder |= check;

  // Operate on only 16 positions of max 24.
  // The remaining 8 are our remainder and should be zero when we're done.
  for (uint8_t i = 0 ; i < 16 ; i++) {
    //Check if there is a one in the left position
    if( remainder & (uint32_t)1<<(23 - i) )
      remainder ^= divisor;

    //Rotate the divisor max 16 times so that we have 8 bits left of a remainder
    divisor >>= 1;
  }
  return ((uint8_t) remainder);
}

/* ======================================================================
Function: si7013_readRegister
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_123_si7013_readRegister(uint8_t i2caddr, const uint8_t reg, uint8_t * value)
{
  
  // Request user register
  Wire.beginTransmission(i2caddr);
  Wire.write(reg);
  Wire.endTransmission();

  // request 1 byte result
  Wire.requestFrom(i2caddr, 1u);
  if (Wire.available()>=1) {
      *value = Wire.read();
      return 0;
  }

  return 1;
}

/* ======================================================================
Function: Plugin_123_si7013_startConv
Purpose : return temperature or humidity measured
Input   : data type SI7013_READ_HUM or SI7013_READ_TEMP
          current config resolution
Output  : 0 if okay
Comments: internal values of temp and rh are set
====================================================================== */
int8_t Plugin_123_si7013_startConv(uint8_t i2caddr, uint8_t datatype, uint8_t resolution)
{
  long data;
  uint16_t raw ;
  uint8_t checksum,tmp;



  //Request a reading
  Wire.beginTransmission(i2caddr);
  Wire.write(datatype);
  Wire.endTransmission();

  // Tried clock streching and looping until no NACK from SI7021 to know
  // when conversion's done. None have worked so far !!!
  // I fade up, I'm waiting maximum conversion time + 1ms, this works !!
  // I increased these value to add HTU21D compatibility
  // Max for SI7021 is 3/5/7/12 ms
  // max for HTU21D is 7/13/25/50 ms

  // Martinus modification 2016-01-07:
  // My test sample was still not working with 11 bit
  // So to be more safe, we add 5 ms to each and use 8,10,13,21 ms
  // But for ESP Easy, I think it does not matter at all...

  // Martinus is correct there was a bug Mesasure HUM need
  // hum+temp delay because it also measure temp

  if (resolution == SI7013_RESOLUTION_11T_11RH)
    tmp = 7;
  else if (resolution == SI7013_RESOLUTION_12T_08RH)
    tmp = 13;
  else if (resolution == SI7013_RESOLUTION_13T_10RH)
    tmp = 25;
  else
    tmp = 50;

  // Humidity fire also temp measurment so delay
  // need to be increased by 2 if no Hold Master
  if (datatype == SI7013_MEASURE_HUM)
    tmp *=2;

  delay(tmp);

  /*
  // Wait for data to become available, device will NACK during conversion
  tmp = 0;
  do
  {
    // Request device
    Wire.beginTransmission(SI7021_I2C_ADDRESS);
    //Wire.write(SI7021_READ_REG);
    error = Wire.endTransmission(true);
    delay(1);
  }
  // always use time out in loop to avoid potential lockup (here 12ms max)
  // https://www.silabs.com/Support%20Documents/TechnicalDocs/Si7021-A20.pdf page 5
  while(error!=0 && tmp++<=12 );
  */
  if ( Wire.requestFrom(i2caddr, 3u) < 3 ) {
    return -1;
  }

  // Comes back in three bytes, data(MSB) / data(LSB) / Checksum
  raw  = ((uint16_t) Wire.read()) << 8;
  raw |= Wire.read();
  checksum = Wire.read();

  // Check CRC of data received
  if(Plugin_123_si7013_checkCRC(raw, checksum) != 0) {
    addLog(LOG_LEVEL_INFO,F("SI7013 : checksum error!"));
    return -1;
  }

  // Humidity
  if (datatype == SI7013_MEASURE_HUM || datatype == SI7013_MEASURE_HUM_HM) {
    // Convert value to Himidity percent
    // pm-cz: it is possible to enable decimal places for humidity as well by multiplying the value in formula by 100
    data = ((1250 * (long)raw) >> 16) - 60;

    // Datasheet says doing this check
    if (data>1000) data = 1000;
    if (data<0)   data = 0;

    //pm-cz: Let us make sure we have enough precision due to ADC bits
    if (resolution == SI7013_RESOLUTION_12T_08RH) {
      data = (data + 5) / 10;
      data *= 10;
    }
    // save value
    si7013_humidity = (uint16_t) data;

  // Temperature
  } else  if (datatype == SI7013_MEASURE_TEMP ||datatype == SI7013_MEASURE_TEMP_HM || datatype == SI7013_MEASURE_TEMP_HUM) {
    // Convert value to Temperature (*100)
    // for 23.45C value will be 2345
    data =  ((17572 * (long)raw) >> 16) - 4685;

    /*
    // pm-cz: We should probably check for precision here as well
    if (resolution != SI7021_RESOLUTION_14T_12RH) {
      if (data > 0) {
        data = (data + 5) / 10;
      } else {
        data = (data - 5) / 10;
      }
      data *= 10;
    }
    */

    // save value
    si7013_temperature = (int16_t) data;
  }

  return 0;
}


/* ======================================================================
Function: Plugin_123_si7013_readValues
Purpose : read temperature and humidity from SI7021 sensor
Input   : current config resolution
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_123_si7013_readValues(uint8_t i2caddr, uint8_t resolution, uint8_t filter_power)
{
  int8_t error = 0;

  // start humidity conversion
  error |= Plugin_123_si7013_startConv(i2caddr, SI7013_MEASURE_HUM, resolution);

 //the humidity is actually doing a temperature reading Too
 //error|= Plugin_014_si7021_startConv(SI7021_READ_REG)

  // start temperature conversion
  error |= Plugin_123_si7013_startConv(i2caddr, SI7013_MEASURE_TEMP, resolution);


  error|= Plugin_123_si7013_readADC(i2caddr,filter_power);

  return error;
}



int8_t Plugin_123_si7013_readADC(uint8_t i2caddr, uint8_t filter_power)
{
  int16_t raw;
  uint8_t reg;
  uint8_t error;

  //set VOUT
  // Get the current register value
  error = Plugin_123_si7013_readRegister(i2caddr, SI7013_READ_REG2, &reg);
  if ( error == 0) {
       
      // Prepare to write to the register value
    Wire.beginTransmission(i2caddr);
    Wire.write(SI7013_WRITE_REG2);
    
    Wire.write(reg | (1+2+4+64) );//set last three bits to 1 (VIN bufered, Vref=VDD, VOUT=VDD) and No-Hold for bit 6

    Wire.endTransmission();
 
  

    //read adc
    Wire.beginTransmission(i2caddr);
    Wire.write(SI7013_READ_ADC);
    Wire.endTransmission();

    delay(10); //wating for conversion to be done in the specs is mentioned 7ms in normal mode
    if ( Wire.requestFrom(i2caddr, 2u) < 2 ) {
      return -1;
    }

    // Comes back in two bytes, data(MSB) / data(LSB) with no Checksum
    raw  = ((uint16_t) Wire.read()) << 8;
    raw |= Wire.read();


    //Calculate Moving average where 2^filter_power is the moving window of points
    //MA*[i]= MA*[i-1] +X[i] - MA*[i-1]/N

    si7013_adc = si7013_adc + raw - (si7013_adc>>filter_power);


    
    //set vout to gnd to not consume power
    Wire.beginTransmission(i2caddr);
    Wire.write(reg);
    
    return (int8_t) Wire.endTransmission();

  }
  return error;
}

/* ======================================================================
Function: Plugin_123_si7013_setResolution
Purpose : Sets the sensor resolution to one of four levels
Input   : see #define default is SI7013_RESOLUTION_14T_12RH
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_123_si7013_setResolution(uint8_t i2caddr, uint8_t res)
{
  uint8_t reg;
  uint8_t error;

  // Get the current register value
  error = Plugin_123_si7013_readRegister(i2caddr, SI7013_READ_REG1, &reg);
  if ( error == 0) {
    // remove resolution bits
    reg &= SI7013_RESOLUTION_MASK ;

    // Prepare to write to the register value
    Wire.beginTransmission(i2caddr);
    Wire.write(SI7013_WRITE_REG1);

    // Write the new resolution bits but clear unused before
    Wire.write(reg | ( res &= ~SI7013_RESOLUTION_MASK) );
    return (int8_t) Wire.endTransmission();
  }

  return error;
}
#endif // USES_P123
