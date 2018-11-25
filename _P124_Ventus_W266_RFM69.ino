#ifdef PLUGIN_BUILD_TESTING

// #######################################################################################################
// ##################################### Plugin 124: Ventus W266 RFM69 ###################################
// #######################################################################################################

// Purpose: Receiving weather data from a Ventus W266 outdoor unit using a RFM69 receiver modul

// This plugin is based on the P186_Ventus_W266 plugin and is modified to use a RFM69HCW transceiver
// instead of the internal RFM31 of the Ventus indoor unit.

// With this plugin it is possible to receive the data sent from the outdoor unit of a Ventus W266
// (aka Renkforce W205GU) and forward it to a server.
// All you need is a RFM69 directly wired to an ESP8266.
// No additional components are needed for the interface since both moduls run on 3.3V.

// There are different versions of the RFM69 available.
// In my setup, i used a RFM69HCW conncted to an ESP12F.
// The wiring is:

// ESP12F    RFM69HCW
// ------------------
// GPIO-4    RESET
// GPIO-5    DIO0
// GPIO-15   NSS
// GPIO-14   SCK
// GPIO-13   MOSI
// GPIO-12   MISO

// The following information can be received:
// Humidity, temperature, wind direction, wind average, wind gust, rainfall, UV and lightning.
// That is more than te maximum of 4 values per device for espeasy. The plugins functionality is therefore
// divided per sensorgroup.

// The plugin can (and should) be used more then one time, however only one plugin instance can be
// the main plugin. The plugin function can be selected by a dropdown and only the MAIN plugin has
// the ability to set the sensor address (remote unit ID).

// To find out the current ID of your Ventus remote unit, simply setup a task with instance type "MAIN Temp/Hygro" and observe the LOG.
// Everytime the RFM69 receives a valid packet from a Ventus W266 it shows up in the LOG (approx. every 30 seconds)
// The first number after the "RX:" is the ID of the unit received.
// This ID-number then has to be entered in the "Unit ID" field in the task setup of the "MAIN Temp/Hygro" instance.
// Data is sent to the specified server as soon as it is received.

// RFM69 RX-buffer content:
// ************************
// IDhh 1A tlth ?b tlth wb alahglgh rlrh?? uv ld?? lllhcrc
// 9827 1A B100 00 B100 06 00000000 1E0000 00 3F8A 2A0017
//  0 1  2  3 4  5  6 7  8  9 0 1 2  3 4 5  6  7 8  9 0 1
//
// ID ..... ID of the remote unit. Changes randomly each time the batteries are removed. Is needed to identify "your" unit.
// hh ..... humidity -> Humidity bcd encoded
// tlth ... temperature (low/high byte) > Temperature is stored as a 16bit integer holding the temperature in degree celcius*10
// b ...... battery low > This uint8 is 00 when battery is ok, 01 when battery is low
// wb ..... bearing (cw 0-15) > The wind bearing in 16 clockwise steps (0 = north, 4 = east, 8 = south and C = west)
// alah ... wind average (low/high byte) > A uint16 holding the wind avarage in m/s
// glgh ... wind gust (low/high byte) > A uint16 holding the wind gust in m/s
// rlrh ... rainfall (low/high byte) > Accumulated rainfall in 1/4mm
// uv ..... uv-index > The UV value * 10
// ld ..... lightningstorm-distance (3F max) > The distance to the stormfront in km
// lllh ... strike count (low/high byte) > A uint16 holding the accumulated number of detected lightning strikes
// crc .... CRC checksum > Poly 0x31, init 0xff, revin&revout, xorout 0x00. Like Maxim 1-wire but with a 0xff init value

#include <SPI.h>

#define PLUGIN_124_DEBUG            true                        // Shows received frames and crc in log@INFO

#define PLUGIN_124
#define PLUGIN_ID_124               124
#define PLUGIN_NAME_124             "Ventus W266 RFM69 [TESTING]"
#define PLUGIN_VALUENAME1_124       ""
#define PLUGIN_VALUENAME2_124       ""
#define PLUGIN_VALUENAME3_124       ""

#define Plugin_124_RESET_Pin        4
#define Plugin_124_DIO0_Pin         5
#define Plugin_124_SPI_CS_Pin       15

#define PAYLOAD_SIZE                22

enum instanceTypes { INSTANCE_TH,
                     INSTANCE_WIND,
                     INSTANCE_RAIN,
                     INSTANCE_UV,
                     INSTANCE_LIGHTNING,
                     INSTANCE_BATTERY,
                     NUMBER_OF_INSTANCES
                   };

boolean firstrun = true;
boolean dataValid = false;
boolean dataPending = false;

uint16_t timer300s = 0;
uint16_t timer3600s = 0;

uint8_t humidity = 0;
float temperature = 0;
float windDIR = 0;
float windAVG = 0;
float windGUST = 0;
float rainTotal = 0;
float uv = 0;
int16_t strikesDistance = -1;
uint16_t strikesTotal = 0;
int8_t batteryLow = -1;

// statisitcs variables
float rainLevelOneHourAgoe = 0;
float rainLevelPastHour = 0;
uint16_t strikes5minutesAgoe = 0;
uint16_t strikesPast5minutes = 0;

uint8_t timeSlot = 0;

// ******************************************************

// RFM69 registers

#define RFM69_REG_00_FIFO                             0x00
#define RFM69_REG_01_OPMODE                           0x01
#define RFM69_REG_02_MODUL                            0x02
#define RFM69_REG_03_BITRATE_MSB                      0x03
#define RFM69_REG_04_BITRATE_LSB                      0x04
#define RFM69_REG_05_FDEV_MSB                         0x05
#define RFM69_REG_06_FDEV_LSB                         0x06
#define RFM69_REG_07_FRF_MSB                          0x07
#define RFM69_REG_08_FRF_MID                          0x08
#define RFM69_REG_09_FRF_LSB                          0x09
#define RFM69_REG_0A_OSC1                             0x0A
#define RFM69_REG_0B_AFC_CTRL                         0x0B
#define RFM69_REG_0D_LISTEN1                          0x0D
#define RFM69_REG_0E_LISTEN2                          0x0E
#define RFM69_REG_0F_LISTEN3                          0x0F
#define RFM69_REG_10_VERSION                          0x10
#define RFM69_REG_11_PA_LEVEL                         0x11
#define RFM69_REG_12_PA_RAMP                          0x12
#define RFM69_REG_13_OCP                              0x13
#define RFM69_REG_18_LNA                              0x18
#define RFM69_REG_19_RX_BW                            0x19
#define RFM69_REG_1A_AFC_BW                           0x1A
#define RFM69_REG_1B_OOK_PEAK                         0x1B
#define RFM69_REG_1C_OOK_AVG                          0x1C
#define RFM69_REG_1D_OOF_FIX                          0x1D
#define RFM69_REG_1E_AFC_FEI                          0x1E
#define RFM69_REG_1F_AFC_MSB                          0x1F
#define RFM69_REG_20_AFC_LSB                          0x20
#define RFM69_REG_21_FEI_MSB                          0x21
#define RFM69_REG_22_FEI_LSB                          0x22
#define RFM69_REG_23_RSSI_CONFIG                      0x23
#define RFM69_REG_24_RSSI_VALUE                       0x24
#define RFM69_REG_25_DIO_MAPPING1                     0x25
#define RFM69_REG_26_DIO_MAPPING2                     0x26
#define RFM69_REG_27_IRQ_FLAGS1                       0x27
#define RFM69_REG_28_IRQ_FLAGS2                       0x28
#define RFM69_REG_29_RSSI_THRESHOLD                   0x29
#define RFM69_REG_2A_RX_TIMEOUT1                      0x2A
#define RFM69_REG_2B_RX_TIMEOUT2                      0x2B
#define RFM69_REG_2C_PREAMBLE_MSB                     0x2C
#define RFM69_REG_2D_PREAMBLE_LSB                     0x2D
#define RFM69_REG_2E_SYNC_CONFIG                      0x2E
#define RFM69_REG_2F_SYNCVALUE1                       0x2F
#define RFM69_REG_30_SYNCVALUE2                       0x30
#define RFM69_REG_31_SYNCVALUE3                       0x31
#define RFM69_REG_32_SYNCVALUE4                       0x32
#define RFM69_REG_33_SYNCVALUE5                       0x33
#define RFM69_REG_34_SYNCVALUE6                       0x34
#define RFM69_REG_35_SYNCVALUE7                       0x35
#define RFM69_REG_36_SYNCVALUE8                       0x36
#define RFM69_REG_37_PACKET_CONFIG1                   0x37
#define RFM69_REG_38_PAYLOAD_LENGTH                   0x38
#define RFM69_REG_39_NODE_ADDRESS                     0x39
#define RFM69_REG_3A_BROADCAST_ADDRESS                0x3A
#define RFM69_REG_3B_AUTOMODES                        0x3B
#define RFM69_REG_3C_FIFO_THRESHOLD                   0x3C
#define RFM69_REG_3D_PACKET_CONFIG2                   0x3D
#define RFM69_REG_3E_AES_KEY_BYTE1                    0x3E
#define RFM69_REG_3F_AES_KEY_BYTE2                    0x3F
#define RFM69_REG_40_AES_KEY_BYTE3                    0x40
#define RFM69_REG_41_AES_KEY_BYTE4                    0x41
#define RFM69_REG_42_AES_KEY_BYTE5                    0x42
#define RFM69_REG_43_AES_KEY_BYTE6                    0x43
#define RFM69_REG_44_AES_KEY_BYTE7                    0x44
#define RFM69_REG_45_AES_KEY_BYTE8                    0x45
#define RFM69_REG_46_AES_KEY_BYTE9                    0x46
#define RFM69_REG_47_AES_KEY_BYTE10                   0x47
#define RFM69_REG_48_AES_KEY_BYTE11                   0x48
#define RFM69_REG_49_AES_KEY_BYTE12                   0x49
#define RFM69_REG_4A_AES_KEY_BYTE13                   0x4A
#define RFM69_REG_4B_AES_KEY_BYTE14                   0x4B
#define RFM69_REG_4C_AES_KEY_BYTE15                   0x4C
#define RFM69_REG_4D_AES_KEY_BYTE16                   0x4D
#define RFM69_REG_4E_TEMP1                            0x4E
#define RFM69_REG_4F_TEMP2                            0x4F
#define RFM69_REG_58_TEST_LNA                         0x58
#define RFM69_REG_5A_TEST_PA1                         0x5A
#define RFM69_REG_5C_TEST_PA2                         0x5C
#define RFM69_REG_6F_TEST_DAGC                        0x6F
#define RFM69_REG_71_TEST_AFC                         0x71

// RegOpMode
#define RF_OPMODE_SEQUENCER_OFF                       0x80
#define RF_OPMODE_SEQUENCER_ON                        0x00  // Default

#define RF_OPMODE_LISTEN_ON                           0x40
#define RF_OPMODE_LISTEN_OFF                          0x00  // Default

#define RF_OPMODE_LISTENABORT                         0x20

#define RF_OPMODE_SLEEP                               0x00
#define RF_OPMODE_STANDBY                             0x04  // Default
#define RF_OPMODE_SYNTHESIZER                         0x08
#define RF_OPMODE_TRANSMITTER                         0x0C
#define RF_OPMODE_RECEIVER                            0x10

// RegDataModul
#define RFMODULMODE_PACKET                            0x00  // Default
#define RFMODULMODE_CONTINUOUS                        0x40
#define RFMODULMODE_CONTINUOUSNOBSYNC                 0x60

#define RFMODUL_MODULATIONTYPE_FSK                    0x00  // Default
#define RFMODUL_MODULATIONTYPE_OOK                    0x08

#define RFMODUL_MODULATIONSHAPING_00                  0x00  // Default
#define RFMODUL_MODULATIONSHAPING_01                  0x01
#define RFMODUL_MODULATIONSHAPING_10                  0x02
#define RFMODUL_MODULATIONSHAPING_11                  0x03

// RegOsc1
#define RF_OSC1_RCCAL_START                           0x80
#define RF_OSC1_RCCAL_DONE                            0x40

// RegAfcCtrl
#define RF_AFCLOWBETA_ON                              0x20
#define RF_AFCLOWBETA_OFF                             0x00    // Default

// RegLowBat
#define RF_LOWBAT_MONITOR                             0x10
#define RF_LOWBAT_ON                                  0x08
#define RF_LOWBAT_OFF                                 0x00  // Default

#define RF_LOWBAT_TRIM_1695                           0x00
#define RF_LOWBAT_TRIM_1764                           0x01
#define RF_LOWBAT_TRIM_1835                           0x02  // Default
#define RF_LOWBAT_TRIM_1905                           0x03
#define RF_LOWBAT_TRIM_1976                           0x04
#define RF_LOWBAT_TRIM_2045                           0x05
#define RF_LOWBAT_TRIM_2116                           0x06
#define RF_LOWBAT_TRIM_2185                           0x07

// RegListen1
#define RF_LISTEN1_RESOL_64                           0x50
#define RF_LISTEN1_RESOL_4100                         0xA0  // Default
#define RF_LISTEN1_RESOL_262000                       0xF0

#define RF_LISTEN1_CRITERIA_RSSI                      0x00  // Default
#define RF_LISTEN1_CRITERIA_RSSIANDSYNC               0x08

#define RF_LISTEN1_END_00                             0x00
#define RF_LISTEN1_END_01                             0x02  // Default
#define RF_LISTEN1_END_10                             0x04

// RegListen2
#define RF_LISTEN2_COEFIDLE_VALUE                     0xF5 // Default

// RegListen3
#define RF_LISTEN3_COEFRX_VALUE                       0x20 // Default

// RegPaLevel
#define RF_PALEVEL_PA0_ON                             0x80  // Default
#define RF_PALEVEL_PA0_OFF                            0x00
#define RF_PALEVEL_PA1_ON                             0x40
#define RF_PALEVEL_PA1_OFF                            0x00  // Default
#define RF_PALEVEL_PA2_ON                             0x20
#define RF_PALEVEL_PA2_OFF                            0x00  // Default

// RegPaRamp
#define RF_PARAMP_3400                                0x00
#define RF_PARAMP_2000                                0x01
#define RF_PARAMP_1000                                0x02
#define RF_PARAMP_500                                 0x03
#define RF_PARAMP_250                                 0x04
#define RF_PARAMP_125                                 0x05
#define RF_PARAMP_100                                 0x06
#define RF_PARAMP_62                                  0x07
#define RF_PARAMP_50                                  0x08
#define RF_PARAMP_40                                  0x09  // Default
#define RF_PARAMP_31                                  0x0A
#define RF_PARAMP_25                                  0x0B
#define RF_PARAMP_20                                  0x0C
#define RF_PARAMP_15                                  0x0D
#define RF_PARAMP_12                                  0x0E
#define RF_PARAMP_10                                  0x0F

// RegOcp
#define RF_OCP_OFF                                    0x0F
#define RF_OCP_ON                                     0x1A  // Default

#define RF_OCP_TRIM_45                                0x00
#define RF_OCP_TRIM_50                                0x01
#define RF_OCP_TRIM_55                                0x02
#define RF_OCP_TRIM_60                                0x03
#define RF_OCP_TRIM_65                                0x04
#define RF_OCP_TRIM_70                                0x05
#define RF_OCP_TRIM_75                                0x06
#define RF_OCP_TRIM_80                                0x07
#define RF_OCP_TRIM_85                                0x08
#define RF_OCP_TRIM_90                                0x09
#define RF_OCP_TRIM_95                                0x0A
#define RF_OCP_TRIM_100                               0x0B  // Default
#define RF_OCP_TRIM_105                               0x0C
#define RF_OCP_TRIM_110                               0x0D
#define RF_OCP_TRIM_115                               0x0E
#define RF_OCP_TRIM_120                               0x0F

// RegAgcRef
#define RF_AGCREF_AUTO_ON                             0x40  // Default
#define RF_AGCREF_AUTO_OFF                            0x00

#define RF_AGCREF_LEVEL_MINUS80                       0x00  // Default
#define RF_AGCREF_LEVEL_MINUS81                       0x01
#define RF_AGCREF_LEVEL_MINUS82                       0x02
#define RF_AGCREF_LEVEL_MINUS83                       0x03
#define RF_AGCREF_LEVEL_MINUS84                       0x04
#define RF_AGCREF_LEVEL_MINUS85                       0x05
#define RF_AGCREF_LEVEL_MINUS86                       0x06
#define RF_AGCREF_LEVEL_MINUS87                       0x07
#define RF_AGCREF_LEVEL_MINUS88                       0x08
#define RF_AGCREF_LEVEL_MINUS89                       0x09
#define RF_AGCREF_LEVEL_MINUS90                       0x0A
#define RF_AGCREF_LEVEL_MINUS91                       0x0B
#define RF_AGCREF_LEVEL_MINUS92                       0x0C
#define RF_AGCREF_LEVEL_MINUS93                       0x0D
#define RF_AGCREF_LEVEL_MINUS94                       0x0E
#define RF_AGCREF_LEVEL_MINUS95                       0x0F
#define RF_AGCREF_LEVEL_MINUS96                       0x10
#define RF_AGCREF_LEVEL_MINUS97                       0x11
#define RF_AGCREF_LEVEL_MINUS98                       0x12
#define RF_AGCREF_LEVEL_MINUS99                       0x13
#define RF_AGCREF_LEVEL_MINUS100                      0x14
#define RF_AGCREF_LEVEL_MINUS101                      0x15
#define RF_AGCREF_LEVEL_MINUS102                      0x16
#define RF_AGCREF_LEVEL_MINUS103                      0x17
#define RF_AGCREF_LEVEL_MINUS104                      0x18
#define RF_AGCREF_LEVEL_MINUS105                      0x19
#define RF_AGCREF_LEVEL_MINUS106                      0x1A
#define RF_AGCREF_LEVEL_MINUS107                      0x1B
#define RF_AGCREF_LEVEL_MINUS108                      0x1C
#define RF_AGCREF_LEVEL_MINUS109                      0x1D
#define RF_AGCREF_LEVEL_MINUS110                      0x1E
#define RF_AGCREF_LEVEL_MINUS111                      0x1F
#define RF_AGCREF_LEVEL_MINUS112                      0x20
#define RF_AGCREF_LEVEL_MINUS113                      0x21
#define RF_AGCREF_LEVEL_MINUS114                      0x22
#define RF_AGCREF_LEVEL_MINUS115                      0x23
#define RF_AGCREF_LEVEL_MINUS116                      0x24
#define RF_AGCREF_LEVEL_MINUS117                      0x25
#define RF_AGCREF_LEVEL_MINUS118                      0x26
#define RF_AGCREF_LEVEL_MINUS119                      0x27
#define RF_AGCREF_LEVEL_MINUS120                      0x28
#define RF_AGCREF_LEVEL_MINUS121                      0x29
#define RF_AGCREF_LEVEL_MINUS122                      0x2A
#define RF_AGCREF_LEVEL_MINUS123                      0x2B
#define RF_AGCREF_LEVEL_MINUS124                      0x2C
#define RF_AGCREF_LEVEL_MINUS125                      0x2D
#define RF_AGCREF_LEVEL_MINUS126                      0x2E
#define RF_AGCREF_LEVEL_MINUS127                      0x2F
#define RF_AGCREF_LEVEL_MINUS128                      0x30
#define RF_AGCREF_LEVEL_MINUS129                      0x31
#define RF_AGCREF_LEVEL_MINUS130                      0x32
#define RF_AGCREF_LEVEL_MINUS131                      0x33
#define RF_AGCREF_LEVEL_MINUS132                      0x34
#define RF_AGCREF_LEVEL_MINUS133                      0x35
#define RF_AGCREF_LEVEL_MINUS134                      0x36
#define RF_AGCREF_LEVEL_MINUS135                      0x37
#define RF_AGCREF_LEVEL_MINUS136                      0x38
#define RF_AGCREF_LEVEL_MINUS137                      0x39
#define RF_AGCREF_LEVEL_MINUS138                      0x3A
#define RF_AGCREF_LEVEL_MINUS139                      0x3B
#define RF_AGCREF_LEVEL_MINUS140                      0x3C
#define RF_AGCREF_LEVEL_MINUS141                      0x3D
#define RF_AGCREF_LEVEL_MINUS142                      0x3E
#define RF_AGCREF_LEVEL_MINUS143                      0x3F

// RegAgcThresh1
#define RF_AGCTHRESH1_SNRMARGIN_000                   0x00
#define RF_AGCTHRESH1_SNRMARGIN_001                   0x20
#define RF_AGCTHRESH1_SNRMARGIN_010                   0x40
#define RF_AGCTHRESH1_SNRMARGIN_011                   0x60
#define RF_AGCTHRESH1_SNRMARGIN_100                   0x80
#define RF_AGCTHRESH1_SNRMARGIN_101                   0xA0  // Default
#define RF_AGCTHRESH1_SNRMARGIN_110                   0xC0
#define RF_AGCTHRESH1_SNRMARGIN_111                   0xE0

#define RF_AGCTHRESH1_STEP1_0                         0x00
#define RF_AGCTHRESH1_STEP1_1                         0x01
#define RF_AGCTHRESH1_STEP1_2                         0x02
#define RF_AGCTHRESH1_STEP1_3                         0x03
#define RF_AGCTHRESH1_STEP1_4                         0x04
#define RF_AGCTHRESH1_STEP1_5                         0x05
#define RF_AGCTHRESH1_STEP1_6                         0x06
#define RF_AGCTHRESH1_STEP1_7                         0x07
#define RF_AGCTHRESH1_STEP1_8                         0x08
#define RF_AGCTHRESH1_STEP1_9                         0x09
#define RF_AGCTHRESH1_STEP1_10                        0x0A
#define RF_AGCTHRESH1_STEP1_11                        0x0B
#define RF_AGCTHRESH1_STEP1_12                        0x0C
#define RF_AGCTHRESH1_STEP1_13                        0x0D
#define RF_AGCTHRESH1_STEP1_14                        0x0E
#define RF_AGCTHRESH1_STEP1_15                        0x0F
#define RF_AGCTHRESH1_STEP1_16                        0x10  // Default
#define RF_AGCTHRESH1_STEP1_17                        0x11
#define RF_AGCTHRESH1_STEP1_18                        0x12
#define RF_AGCTHRESH1_STEP1_19                        0x13
#define RF_AGCTHRESH1_STEP1_20                        0x14
#define RF_AGCTHRESH1_STEP1_21                        0x15
#define RF_AGCTHRESH1_STEP1_22                        0x16
#define RF_AGCTHRESH1_STEP1_23                        0x17
#define RF_AGCTHRESH1_STEP1_24                        0x18
#define RF_AGCTHRESH1_STEP1_25                        0x19
#define RF_AGCTHRESH1_STEP1_26                        0x1A
#define RF_AGCTHRESH1_STEP1_27                        0x1B
#define RF_AGCTHRESH1_STEP1_28                        0x1C
#define RF_AGCTHRESH1_STEP1_29                        0x1D
#define RF_AGCTHRESH1_STEP1_30                        0x1E
#define RF_AGCTHRESH1_STEP1_31                        0x1F

// RegAgcThresh2
#define RF_AGCTHRESH2_STEP2_0                         0x00
#define RF_AGCTHRESH2_STEP2_1                         0x10
#define RF_AGCTHRESH2_STEP2_2                         0x20
#define RF_AGCTHRESH2_STEP2_3                         0x30  // XXX wrong -- Default
#define RF_AGCTHRESH2_STEP2_4                         0x40
#define RF_AGCTHRESH2_STEP2_5                         0x50
#define RF_AGCTHRESH2_STEP2_6                         0x60
#define RF_AGCTHRESH2_STEP2_7                         0x70    // default
#define RF_AGCTHRESH2_STEP2_8                         0x80
#define RF_AGCTHRESH2_STEP2_9                         0x90
#define RF_AGCTHRESH2_STEP2_10                        0xA0
#define RF_AGCTHRESH2_STEP2_11                        0xB0
#define RF_AGCTHRESH2_STEP2_12                        0xC0
#define RF_AGCTHRESH2_STEP2_13                        0xD0
#define RF_AGCTHRESH2_STEP2_14                        0xE0
#define RF_AGCTHRESH2_STEP2_15                        0xF0

#define RF_AGCTHRESH2_STEP3_0                         0x00
#define RF_AGCTHRESH2_STEP3_1                         0x01
#define RF_AGCTHRESH2_STEP3_2                         0x02
#define RF_AGCTHRESH2_STEP3_3                         0x03
#define RF_AGCTHRESH2_STEP3_4                         0x04
#define RF_AGCTHRESH2_STEP3_5                         0x05
#define RF_AGCTHRESH2_STEP3_6                         0x06
#define RF_AGCTHRESH2_STEP3_7                         0x07
#define RF_AGCTHRESH2_STEP3_8                         0x08
#define RF_AGCTHRESH2_STEP3_9                         0x09
#define RF_AGCTHRESH2_STEP3_10                        0x0A
#define RF_AGCTHRESH2_STEP3_11                        0x0B  // Default
#define RF_AGCTHRESH2_STEP3_12                        0x0C
#define RF_AGCTHRESH2_STEP3_13                        0x0D
#define RF_AGCTHRESH2_STEP3_14                        0x0E
#define RF_AGCTHRESH2_STEP3_15                        0x0F

// RegAgcThresh3
#define RF_AGCTHRESH3_STEP4_0                         0x00
#define RF_AGCTHRESH3_STEP4_1                         0x10
#define RF_AGCTHRESH3_STEP4_2                         0x20
#define RF_AGCTHRESH3_STEP4_3                         0x30
#define RF_AGCTHRESH3_STEP4_4                         0x40
#define RF_AGCTHRESH3_STEP4_5                         0x50
#define RF_AGCTHRESH3_STEP4_6                         0x60
#define RF_AGCTHRESH3_STEP4_7                         0x70
#define RF_AGCTHRESH3_STEP4_8                         0x80
#define RF_AGCTHRESH3_STEP4_9                         0x90  // Default
#define RF_AGCTHRESH3_STEP4_10                        0xA0
#define RF_AGCTHRESH3_STEP4_11                        0xB0
#define RF_AGCTHRESH3_STEP4_12                        0xC0
#define RF_AGCTHRESH3_STEP4_13                        0xD0
#define RF_AGCTHRESH3_STEP4_14                        0xE0
#define RF_AGCTHRESH3_STEP4_15                        0xF0

#define RF_AGCTHRESH3_STEP5_0                         0x00
#define RF_AGCTHRESH3_STEP5_1                         0x01
#define RF_AGCTHRESH3_STEP5_2                         0x02
#define RF_AGCTHRESH3_STEP5_3                         0x03
#define RF_AGCTHRESH3_STEP5_4                         0x04
#define RF_AGCTHRESH3_STEP5_5                         0x05
#define RF_AGCTHRESH3_STEP5_6                         0x06
#define RF_AGCTHRESH3_STEP5_7                         0x07
#define RF_AGCTHRES33_STEP5_8                         0x08
#define RF_AGCTHRESH3_STEP5_9                         0x09
#define RF_AGCTHRESH3_STEP5_10                        0x0A
#define RF_AGCTHRESH3_STEP5_11                        0x0B  // Default
#define RF_AGCTHRESH3_STEP5_12                        0x0C
#define RF_AGCTHRESH3_STEP5_13                        0x0D
#define RF_AGCTHRESH3_STEP5_14                        0x0E
#define RF_AGCTHRESH3_STEP5_15                        0x0F

// RegLna
#define RF_LNA_ZIN_50                                 0x00
#define RF_LNA_ZIN_200                                0x80  // Default

#define RF_LNA_LOWPOWER_OFF                           0x00  // Default
#define RF_LNA_LOWPOWER_ON                            0x40

#define RF_LNA_CURRENTGAIN                            0x38

#define RF_LNA_GAINSELECT_AUTO                        0x00  // Default
#define RF_LNA_GAINSELECT_MAX                         0x01
#define RF_LNA_GAINSELECT_MAXMINUS6                   0x02
#define RF_LNA_GAINSELECT_MAXMINUS12                  0x03
#define RF_LNA_GAINSELECT_MAXMINUS24                  0x04
#define RF_LNA_GAINSELECT_MAXMINUS36                  0x05
#define RF_LNA_GAINSELECT_MAXMINUS48                  0x06

// RegRxBw
#define RF_RXBW_DCCFREQ_000                           0x00
#define RF_RXBW_DCCFREQ_001                           0x20
#define RF_RXBW_DCCFREQ_010                           0x40  // Default
#define RF_RXBW_DCCFREQ_011                           0x60
#define RF_RXBW_DCCFREQ_100                           0x80
#define RF_RXBW_DCCFREQ_101                           0xA0
#define RF_RXBW_DCCFREQ_110                           0xC0
#define RF_RXBW_DCCFREQ_111                           0xE0

#define RF_RXBW_MANT_16                               0x00
#define RF_RXBW_MANT_20                               0x08
#define RF_RXBW_MANT_24                               0x10  // Default

#define RF_RXBW_EXP_0                                 0x00
#define RF_RXBW_EXP_1                                 0x01
#define RF_RXBW_EXP_2                                 0x02
#define RF_RXBW_EXP_3                                 0x03
#define RF_RXBW_EXP_4                                 0x04
#define RF_RXBW_EXP_5                                 0x05  // Default
#define RF_RXBW_EXP_6                                 0x06
#define RF_RXBW_EXP_7                                 0x07

// RegAfcBw
#define RF_AFCBW_DCCFREQAFC_000                       0x00
#define RF_AFCBW_DCCFREQAFC_001                       0x20
#define RF_AFCBW_DCCFREQAFC_010                       0x40
#define RF_AFCBW_DCCFREQAFC_011                       0x60
#define RF_AFCBW_DCCFREQAFC_100                       0x80  // Default
#define RF_AFCBW_DCCFREQAFC_101                       0xA0
#define RF_AFCBW_DCCFREQAFC_110                       0xC0
#define RF_AFCBW_DCCFREQAFC_111                       0xE0

#define RF_AFCBW_MANTAFC_16                           0x00
#define RF_AFCBW_MANTAFC_20                           0x08  // Default
#define RF_AFCBW_MANTAFC_24                           0x10

#define RF_AFCBW_EXPAFC_0                             0x00
#define RF_AFCBW_EXPAFC_1                             0x01
#define RF_AFCBW_EXPAFC_2                             0x02
#define RF_AFCBW_EXPAFC_3                             0x03  // Default
#define RF_AFCBW_EXPAFC_4                             0x04
#define RF_AFCBW_EXPAFC_5                             0x05
#define RF_AFCBW_EXPAFC_6                             0x06
#define RF_AFCBW_EXPAFC_7                             0x07

// RegOokPeak
#define RF_OOKPEAK_THRESHTYPE_FIXED                   0x00
#define RF_OOKPEAK_THRESHTYPE_PEAK                    0x40  // Default
#define RF_OOKPEAK_THRESHTYPE_AVERAGE                 0x80

#define RF_OOKPEAK_PEAKTHRESHSTEP_000                 0x00  // Default
#define RF_OOKPEAK_PEAKTHRESHSTEP_001                 0x08
#define RF_OOKPEAK_PEAKTHRESHSTEP_010                 0x10
#define RF_OOKPEAK_PEAKTHRESHSTEP_011                 0x18
#define RF_OOKPEAK_PEAKTHRESHSTEP_100                 0x20
#define RF_OOKPEAK_PEAKTHRESHSTEP_101                 0x28
#define RF_OOKPEAK_PEAKTHRESHSTEP_110                 0x30
#define RF_OOKPEAK_PEAKTHRESHSTEP_111                 0x38

#define RF_OOKPEAK_PEAKTHRESHDEC_000                  0x00  // Default
#define RF_OOKPEAK_PEAKTHRESHDEC_001                  0x01
#define RF_OOKPEAK_PEAKTHRESHDEC_010                  0x02
#define RF_OOKPEAK_PEAKTHRESHDEC_011                  0x03
#define RF_OOKPEAK_PEAKTHRESHDEC_100                  0x04
#define RF_OOKPEAK_PEAKTHRESHDEC_101                  0x05
#define RF_OOKPEAK_PEAKTHRESHDEC_110                  0x06
#define RF_OOKPEAK_PEAKTHRESHDEC_111                  0x07

// RegOokAvg
#define RF_OOKAVG_AVERAGETHRESHFILT_00                0x00
#define RF_OOKAVG_AVERAGETHRESHFILT_01                0x40
#define RF_OOKAVG_AVERAGETHRESHFILT_10                0x80  // Default
#define RF_OOKAVG_AVERAGETHRESHFILT_11                0xC0

// RegOokFix
#define RF_OOKFIX_FIXEDTHRESH_VALUE                   0x06  // Default

// RegAfcFei
#define RF_AFCFEI_FEI_DONE                            0x40
#define RF_AFCFEI_FEI_START                           0x20
#define RF_AFCFEI_AFC_DONE                            0x10
#define RF_AFCFEI_AFCAUTOCLEAR_ON                     0x08
#define RF_AFCFEI_AFCAUTOCLEAR_OFF                    0x00  // Default

#define RF_AFCFEI_AFCAUTO_ON                          0x04
#define RF_AFCFEI_AFCAUTO_OFF                         0x00  // Default

#define RF_AFCFEI_AFC_CLEAR                           0x02
#define RF_AFCFEI_AFC_START                           0x01

// RegRssiConfig
#define RF_RSSI_FASTRX_ON                             0x08
#define RF_RSSI_FASTRX_OFF                            0x00  // Default
#define RF_RSSI_DONE                                  0x02
#define RF_RSSI_START                                 0x01

// RegDioMapping1
#define RF_DIOMAPPING1_DIO0_00                        0x00  // Default
#define RF_DIOMAPPING1_DIO0_01                        0x40
#define RF_DIOMAPPING1_DIO0_10                        0x80
#define RF_DIOMAPPING1_DIO0_11                        0xC0

#define RF_DIOMAPPING1_DIO1_00                        0x00  // Default
#define RF_DIOMAPPING1_DIO1_01                        0x10
#define RF_DIOMAPPING1_DIO1_10                        0x20
#define RF_DIOMAPPING1_DIO1_11                        0x30

#define RF_DIOMAPPING1_DIO2_00                        0x00  // Default
#define RF_DIOMAPPING1_DIO2_01                        0x04
#define RF_DIOMAPPING1_DIO2_10                        0x08
#define RF_DIOMAPPING1_DIO2_11                        0x0C

#define RF_DIOMAPPING1_DIO3_00                        0x00  // Default
#define RF_DIOMAPPING1_DIO3_01                        0x01
#define RF_DIOMAPPING1_DIO3_10                        0x02
#define RF_DIOMAPPING1_DIO3_11                        0x03


// RegDioMapping2
#define RF_DIOMAPPING2_DIO4_00                        0x00  // Default
#define RF_DIOMAPPING2_DIO4_01                        0x40
#define RF_DIOMAPPING2_DIO4_10                        0x80
#define RF_DIOMAPPING2_DIO4_11                        0xC0

#define RF_DIOMAPPING2_DIO5_00                        0x00  // Default
#define RF_DIOMAPPING2_DIO5_01                        0x10
#define RF_DIOMAPPING2_DIO5_10                        0x20
#define RF_DIOMAPPING2_DIO5_11                        0x30

#define RF_DIOMAPPING2_CLKOUT_32                      0x00
#define RF_DIOMAPPING2_CLKOUT_16                      0x01
#define RF_DIOMAPPING2_CLKOUT_8                       0x02
#define RF_DIOMAPPING2_CLKOUT_4                       0x03
#define RF_DIOMAPPING2_CLKOUT_2                       0x04
#define RF_DIOMAPPING2_CLKOUT_1                       0x05
#define RF_DIOMAPPING2_CLKOUT_RC                      0x06
#define RF_DIOMAPPING2_CLKOUT_OFF                     0x07  // Default

// RegIrqFlags1
#define RF_IRQFLAGS1_MODEREADY                        0x80
#define RF_IRQFLAGS1_RXREADY                          0x40
#define RF_IRQFLAGS1_TXREADY                          0x20
#define RF_IRQFLAGS1_PLLLOCK                          0x10
#define RF_IRQFLAGS1_RSSI                             0x08
#define RF_IRQFLAGS1_TIMEOUT                          0x04
#define RF_IRQFLAGS1_AUTOMODE                         0x02
#define RF_IRQFLAGS1_SYNCADDRESSMATCH                 0x01

// RegIrqFlags2
#define RF_IRQFLAGS2_FIFOFULL                         0x80
#define RF_IRQFLAGS2_FIFONOTEMPTY                     0x40
#define RF_IRQFLAGS2_FIFOLEVEL                        0x20
#define RF_IRQFLAGS2_FIFOOVERRUN                      0x10
#define RF_IRQFLAGS2_PACKETSENT                       0x08
#define RF_IRQFLAGS2_PAYLOADREADY                     0x04
#define RF_IRQFLAGS2_CRCOK                            0x02
#define RF_IRQFLAGS2_LOWBAT                           0x01

// RegRssiThresh
#define RF_RSSITHRESH_VALUE                           0xE4  // Default

// RegRxTimeout1
#define RF_RXTIMEOUT1_RXSTART_VALUE                   0x00  // Default

// RegRxTimeout2
#define RF_RXTIMEOUT2_RSSITHRESH_VALUE                0x00  // Default

// RegPreamble
#define RF_PREAMBLESIZE_MSB_VALUE                     0x00  // Default
#define RF_PREAMBLESIZE_LSB_VALUE                     0x03  // Default

// RegSyncConfig
#define RF_SYNC_ON                                    0x80  // Default
#define RF_SYNC_OFF                                   0x00

#define RF_SYNC_FIFOFILL_AUTO                         0x00  // Default -- when sync interrupt occurs
#define RF_SYNC_FIFOFILL_MANUAL                       0x40

#define RF_SYNC_SIZE_1                                0x00
#define RF_SYNC_SIZE_2                                0x08
#define RF_SYNC_SIZE_3                                0x10
#define RF_SYNC_SIZE_4                                0x18  // Default
#define RF_SYNC_SIZE_5                                0x20
#define RF_SYNC_SIZE_6                                0x28
#define RF_SYNC_SIZE_7                                0x30
#define RF_SYNC_SIZE_8                                0x38

#define RF_SYNC_TOL_0                                 0x00  // Default
#define RF_SYNC_TOL_1                                 0x01
#define RF_SYNC_TOL_2                                 0x02
#define RF_SYNC_TOL_3                                 0x03
#define RF_SYNC_TOL_4                                 0x04
#define RF_SYNC_TOL_5                                 0x05
#define RF_SYNC_TOL_6                                 0x06
#define RF_SYNC_TOL_7                                 0x07

// RegSyncValue1-8
#define RF_SYNC_BYTE1_VALUE                           0x00  // Default
#define RF_SYNC_BYTE2_VALUE                           0x00  // Default
#define RF_SYNC_BYTE3_VALUE                           0x00  // Default
#define RF_SYNC_BYTE4_VALUE                           0x00  // Default
#define RF_SYNC_BYTE5_VALUE                           0x00  // Default
#define RF_SYNC_BYTE6_VALUE                           0x00  // Default
#define RF_SYNC_BYTE7_VALUE                           0x00  // Default
#define RF_SYNC_BYTE8_VALUE                           0x00  // Default

// RegPacketConfig1
#define RF_PACKET1_FORMAT_FIXED                       0x00  // Default
#define RF_PACKET1_FORMAT_VARIABLE                    0x80

#define RF_PACKET1_DCFREE_OFF                         0x00  // Default
#define RF_PACKET1_DCFREE_MANCHESTER                  0x20
#define RF_PACKET1_DCFREE_WHITENING                   0x40

#define RF_PACKET1_CRC_ON                             0x10  // Default
#define RF_PACKET1_CRC_OFF                            0x00

#define RF_PACKET1_CRCAUTOCLEAR_ON                    0x00  // Default
#define RF_PACKET1_CRCAUTOCLEAR_OFF                   0x08

#define RF_PACKET1_ADRSFILTERING_OFF                  0x00  // Default
#define RF_PACKET1_ADRSFILTERING_NODE                 0x02
#define RF_PACKET1_ADRSFILTERING_NODEBROADCAST        0x04

// RegPayloadLength
#define RF_PAYLOADLENGTH_VALUE                        0x40  // Default

// RegBroadcastAdrs
#define RF_BROADCASTADDRESS_VALUE                     0x00

// RegAutoModes
#define RF_AUTOMODES_ENTER_OFF                        0x00  // Default
#define RF_AUTOMODES_ENTER_FIFONOTEMPTY               0x20
#define RF_AUTOMODES_ENTER_FIFOLEVEL                  0x40
#define RF_AUTOMODES_ENTER_CRCOK                      0x60
#define RF_AUTOMODES_ENTER_PAYLOADREADY               0x80
#define RF_AUTOMODES_ENTER_SYNCADRSMATCH              0xA0
#define RF_AUTOMODES_ENTER_PACKETSENT                 0xC0
#define RF_AUTOMODES_ENTER_FIFOEMPTY                  0xE0

#define RF_AUTOMODES_EXIT_OFF                         0x00  // Default
#define RF_AUTOMODES_EXIT_FIFOEMPTY                   0x04
#define RF_AUTOMODES_EXIT_FIFOLEVEL                   0x08
#define RF_AUTOMODES_EXIT_CRCOK                       0x0C
#define RF_AUTOMODES_EXIT_PAYLOADREADY                0x10
#define RF_AUTOMODES_EXIT_SYNCADRSMATCH               0x14
#define RF_AUTOMODES_EXIT_PACKETSENT                  0x18
#define RF_AUTOMODES_EXIT_RXTIMEOUT                   0x1C

#define RF_AUTOMODES_INTERMEDIATE_SLEEP               0x00  // Default
#define RF_AUTOMODES_INTERMEDIATE_STANDBY             0x01
#define RF_AUTOMODES_INTERMEDIATE_RECEIVER            0x02
#define RF_AUTOMODES_INTERMEDIATE_TRANSMITTER         0x03

// RegFifoThresh
#define RF_FIFOTHRESH_TXSTART_FIFOTHRESH              0x00
#define RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY            0x80  // Default

#define RF_FIFOTHRESH_VALUE                           0x0F  // Default

// RegPacketConfig2
#define RF_PACKET2_RXRESTARTDELAY_1BIT                0x00  // Default
#define RF_PACKET2_RXRESTARTDELAY_2BITS               0x10
#define RF_PACKET2_RXRESTARTDELAY_4BITS               0x20
#define RF_PACKET2_RXRESTARTDELAY_8BITS               0x30
#define RF_PACKET2_RXRESTARTDELAY_16BITS              0x40
#define RF_PACKET2_RXRESTARTDELAY_32BITS              0x50
#define RF_PACKET2_RXRESTARTDELAY_64BITS              0x60
#define RF_PACKET2_RXRESTARTDELAY_128BITS             0x70
#define RF_PACKET2_RXRESTARTDELAY_256BITS             0x80
#define RF_PACKET2_RXRESTARTDELAY_512BITS             0x90
#define RF_PACKET2_RXRESTARTDELAY_1024BITS            0xA0
#define RF_PACKET2_RXRESTARTDELAY_2048BITS            0xB0
#define RF_PACKET2_RXRESTARTDELAY_NONE                0xC0
#define RF_PACKET2_RXRESTART                          0x04

#define RF_PACKET2_AUTORXRESTART_ON                   0x02  // Default
#define RF_PACKET2_AUTORXRESTART_OFF                  0x00

#define RF_PACKET2_AES_ON                             0x01
#define RF_PACKET2_AES_OFF                            0x00  // Default

// RegAesKey1-16
#define RF_AESKEY1_VALUE                              0x00  // Default
#define RF_AESKEY2_VALUE                              0x00  // Default
#define RF_AESKEY3_VALUE                              0x00  // Default
#define RF_AESKEY4_VALUE                              0x00  // Default
#define RF_AESKEY5_VALUE                              0x00  // Default
#define RF_AESKEY6_VALUE                              0x00  // Default
#define RF_AESKEY7_VALUE                              0x00  // Default
#define RF_AESKEY8_VALUE                              0x00  // Default
#define RF_AESKEY9_VALUE                              0x00  // Default
#define RF_AESKEY10_VALUE                             0x00  // Default
#define RF_AESKEY11_VALUE                             0x00  // Default
#define RF_AESKEY12_VALUE                             0x00  // Default
#define RF_AESKEY13_VALUE                             0x00  // Default
#define RF_AESKEY14_VALUE                             0x00  // Default
#define RF_AESKEY15_VALUE                             0x00  // Default
#define RF_AESKEY16_VALUE                             0x00  // Default

// RegTemp1
#define RF_TEMP1_MEAS_START                           0x08
#define RF_TEMP1_MEAS_RUNNING                         0x04
#define RF_TEMP1_ADCLOWPOWER_ON                       0x01  // Default
#define RF_TEMP1_ADCLOWPOWER_OFF                      0x00

// RegTestPa1
#define RF_PA20DBM1_NORMAL_AND_RX                     0x55  // Default
#define RF_PA20DBM1_20_DBM_MODE                       0x5D

// RegTestPa2
#define RF_PA20DBM2_NORMAL_AND_RX                     0x70  // Default
#define RF_PA20DBM2_20_DBM_MODE                       0x7C

// RegTestDagc
#define RF_DAGC_NORMAL                                0x00  // Reset value
#define RF_DAGC_IMPROVED_LOWBETA1                     0x20
#define RF_DAGC_IMPROVED_LOWBETA0                     0x30  // Recommended default

// RegBitRate (bits/sec) example bit rates
#define RF_BITRATEMSB_1200                            0x68
#define RF_BITRATELSB_1200                            0x2B
#define RF_BITRATEMSB_2400                            0x34
#define RF_BITRATELSB_2400                            0x15
#define RF_BITRATEMSB_4800                            0x1A  // Default
#define RF_BITRATELSB_4800                            0x0B  // Default
#define RF_BITRATEMSB_9600                            0x0D
#define RF_BITRATELSB_9600                            0x05
#define RF_BITRATEMSB_19200                           0x06
#define RF_BITRATELSB_19200                           0x83
#define RF_BITRATEMSB_38400                           0x03
#define RF_BITRATELSB_38400                           0x41
#define RF_BITRATEMSB_38323                           0x03
#define RF_BITRATELSB_38323                           0x43
#define RF_BITRATEMSB_34482                           0x03
#define RF_BITRATELSB_34482                           0xA0
#define RF_BITRATEMSB_76800                           0x01
#define RF_BITRATELSB_76800                           0xA1
#define RF_BITRATEMSB_153600                          0x00
#define RF_BITRATELSB_153600                          0xD0
#define RF_BITRATEMSB_57600                           0x02
#define RF_BITRATELSB_57600                           0x2C
#define RF_BITRATEMSB_115200                          0x01
#define RF_BITRATELSB_115200                          0x16
#define RF_BITRATEMSB_12500                           0x0A
#define RF_BITRATELSB_12500                           0x00
#define RF_BITRATEMSB_25000                           0x05
#define RF_BITRATELSB_25000                           0x00
#define RF_BITRATEMSB_50000                           0x02
#define RF_BITRATELSB_50000                           0x80
#define RF_BITRATEMSB_100000                          0x01
#define RF_BITRATELSB_100000                          0x40
#define RF_BITRATEMSB_150000                          0x00
#define RF_BITRATELSB_150000                          0xD5
#define RF_BITRATEMSB_200000                          0x00
#define RF_BITRATELSB_200000                          0xA0
#define RF_BITRATEMSB_250000                          0x00
#define RF_BITRATELSB_250000                          0x80
#define RF_BITRATEMSB_300000                          0x00
#define RF_BITRATELSB_300000                          0x6B
#define RF_BITRATEMSB_32768                           0x03
#define RF_BITRATELSB_32768                           0xD1

// RegFdev - frequency deviation (Hz)
#define RF_FDEVMSB_2000                               0x00
#define RF_FDEVLSB_2000                               0x21
#define RF_FDEVMSB_5000                               0x00  // Default
#define RF_FDEVLSB_5000                               0x52  // Default
#define RF_FDEVMSB_7500                               0x00
#define RF_FDEVLSB_7500                               0x7B
#define RF_FDEVMSB_10000                              0x00
#define RF_FDEVLSB_10000                              0xA4
#define RF_FDEVMSB_15000                              0x00
#define RF_FDEVLSB_15000                              0xF6
#define RF_FDEVMSB_20000                              0x01
#define RF_FDEVLSB_20000                              0x48
#define RF_FDEVMSB_25000                              0x01
#define RF_FDEVLSB_25000                              0x9A
#define RF_FDEVMSB_30000                              0x01
#define RF_FDEVLSB_30000                              0xEC
#define RF_FDEVMSB_35000                              0x02
#define RF_FDEVLSB_35000                              0x3D
#define RF_FDEVMSB_40000                              0x02
#define RF_FDEVLSB_40000                              0x8F
#define RF_FDEVMSB_45000                              0x02
#define RF_FDEVLSB_45000                              0xE1
#define RF_FDEVMSB_50000                              0x03
#define RF_FDEVLSB_50000                              0x33
#define RF_FDEVMSB_55000                              0x03
#define RF_FDEVLSB_55000                              0x85
#define RF_FDEVMSB_60000                              0x03
#define RF_FDEVLSB_60000                              0xD7
#define RF_FDEVMSB_65000                              0x04
#define RF_FDEVLSB_65000                              0x29
#define RF_FDEVMSB_70000                              0x04
#define RF_FDEVLSB_70000                              0x7B
#define RF_FDEVMSB_75000                              0x04
#define RF_FDEVLSB_75000                              0xCD
#define RF_FDEVMSB_80000                              0x05
#define RF_FDEVLSB_80000                              0x1F
#define RF_FDEVMSB_85000                              0x05
#define RF_FDEVLSB_85000                              0x71
#define RF_FDEVMSB_90000                              0x05
#define RF_FDEVLSB_90000                              0xC3
#define RF_FDEVMSB_95000                              0x06
#define RF_FDEVLSB_95000                              0x14
#define RF_FDEVMSB_100000                             0x06
#define RF_FDEVLSB_100000                             0x66
#define RF_FDEVMSB_110000                             0x07
#define RF_FDEVLSB_110000                             0x0A
#define RF_FDEVMSB_120000                             0x07
#define RF_FDEVLSB_120000                             0xAE
#define RF_FDEVMSB_130000                             0x08
#define RF_FDEVLSB_130000                             0x52
#define RF_FDEVMSB_140000                             0x08
#define RF_FDEVLSB_140000                             0xF6
#define RF_FDEVMSB_150000                             0x09
#define RF_FDEVLSB_150000                             0x9A
#define RF_FDEVMSB_160000                             0x0A
#define RF_FDEVLSB_160000                             0x3D
#define RF_FDEVMSB_170000                             0x0A
#define RF_FDEVLSB_170000                             0xE1
#define RF_FDEVMSB_180000                             0x0B
#define RF_FDEVLSB_180000                             0x85
#define RF_FDEVMSB_190000                             0x0C
#define RF_FDEVLSB_190000                             0x29
#define RF_FDEVMSB_200000                             0x0C
#define RF_FDEVLSB_200000                             0xCD
#define RF_FDEVMSB_210000                             0x0D
#define RF_FDEVLSB_210000                             0x71
#define RF_FDEVMSB_220000                             0x0E
#define RF_FDEVLSB_220000                             0x14
#define RF_FDEVMSB_230000                             0x0E
#define RF_FDEVLSB_230000                             0xB8
#define RF_FDEVMSB_240000                             0x0F
#define RF_FDEVLSB_240000                             0x5C
#define RF_FDEVMSB_250000                             0x10
#define RF_FDEVLSB_250000                             0x00
#define RF_FDEVMSB_260000                             0x10
#define RF_FDEVLSB_260000                             0xA4
#define RF_FDEVMSB_270000                             0x11
#define RF_FDEVLSB_270000                             0x48
#define RF_FDEVMSB_280000                             0x11
#define RF_FDEVLSB_280000                             0xEC
#define RF_FDEVMSB_290000                             0x12
#define RF_FDEVLSB_290000                             0x8F
#define RF_FDEVMSB_300000                             0x13
#define RF_FDEVLSB_300000                             0x33

// **********************************

void RFM69_writeReg(uint8_t address, uint8_t data)
{
  digitalWrite(Plugin_124_SPI_CS_Pin, LOW);
  SPI.transfer(address | 0x80);
  SPI.transfer(data);
  digitalWrite(Plugin_124_SPI_CS_Pin, HIGH);
}

uint8_t RFM69_readReg(uint8_t address)
{
  uint8_t dataRX;

  digitalWrite(Plugin_124_SPI_CS_Pin, LOW);
  SPI.transfer(address & ~0x80);
  dataRX = SPI.transfer(0x00);
  digitalWrite(Plugin_124_SPI_CS_Pin, HIGH);

  return dataRX;
}

void RFM69_set_frequency(uint64_t freq)
{
  uint32_t fc = (((uint64_t)freq * 524288) / 3200);

  RFM69_writeReg(RFM69_REG_07_FRF_MSB, (fc >> 16) & 0xFF);
  RFM69_writeReg(RFM69_REG_08_FRF_MID, (fc >> 8) & 0xFF);
  RFM69_writeReg(RFM69_REG_09_FRF_LSB, fc & 0xFF);
}

void RFM69_setHighPowerRegs(uint8_t highPower)
{
  if (highPower)
  {
    RFM69_writeReg(RFM69_REG_5A_TEST_PA1, highPower ? 0x5D : 0x55);   // +20dBm mode
    RFM69_writeReg(RFM69_REG_5C_TEST_PA2, highPower ? 0x7C : 0x70);   // +20dBm mode
  } else {
    RFM69_writeReg(RFM69_REG_5A_TEST_PA1, highPower ? 0x55 : 0x55);   // normal mode
    RFM69_writeReg(RFM69_REG_5C_TEST_PA2, highPower ? 0x70 : 0x70);   // normal mode
  }
}

void RFM69_setMode(uint8_t newMode)
{
  uint8_t opModeReg = RFM69_readReg(RFM69_REG_01_OPMODE) & 0x1C;

  if (newMode == opModeReg)
    return;

  RFM69_writeReg(RFM69_REG_01_OPMODE, (opModeReg & 0xE3) | newMode);

  switch (newMode)
  {
    case RF_OPMODE_TRANSMITTER: RFM69_setHighPowerRegs(1);
      break;
    case RF_OPMODE_RECEIVER:    RFM69_setHighPowerRegs(0);
      break;
    case RF_OPMODE_SYNTHESIZER: break;
    case RF_OPMODE_STANDBY:     break;
    case RF_OPMODE_SLEEP:       break;
    default:                    return;
  }
}

void RFM69init()
{  
  digitalWrite(Plugin_124_RESET_Pin, HIGH);
  delay(10);
  digitalWrite(Plugin_124_RESET_Pin, LOW);
  delay(10);

  RFM69_writeReg(RFM69_REG_01_OPMODE, RF_OPMODE_STANDBY |
                 RF_OPMODE_SEQUENCER_ON |
                 RF_OPMODE_LISTEN_OFF);

  RFM69_writeReg(RFM69_REG_02_MODUL, RFMODUL_MODULATIONSHAPING_00 |
                 RFMODUL_MODULATIONTYPE_FSK |
                 RFMODULMODE_PACKET);

  RFM69_writeReg(RFM69_REG_03_BITRATE_MSB, RF_BITRATEMSB_4800);
  RFM69_writeReg(RFM69_REG_04_BITRATE_LSB, RF_BITRATELSB_4800);
  RFM69_writeReg(RFM69_REG_05_FDEV_MSB, RF_FDEVMSB_60000);
  RFM69_writeReg(RFM69_REG_06_FDEV_LSB, RF_FDEVLSB_60000);

  RFM69_writeReg(RFM69_REG_07_FRF_MSB, 0xE4);      // 870 MHz default
  RFM69_writeReg(RFM69_REG_08_FRF_MID, 0xC0);
  RFM69_writeReg(RFM69_REG_09_FRF_LSB, 0x00);

  //RFM69_writeReg(RFM69_REG_0A_OSC1, 0x00);
  RFM69_writeReg(RFM69_REG_0B_AFC_CTRL, 0x00);
  //RFM69_writeReg(RFM69_REG_0D_LISTEN1, 0x00);
  //RFM69_writeReg(RFM69_REG_0E_LISTEN2, 0x00);
  //RFM69_writeReg(RFM69_REG_0F_LISTEN3, 0x00);
  //RFM69_writeReg(RFM69_REG_10_VERSION, 0x00);
  RFM69_writeReg(RFM69_REG_11_PA_LEVEL, 0x1F |
                 RF_PALEVEL_PA0_OFF |
                 RF_PALEVEL_PA1_ON |
                 RF_PALEVEL_PA2_ON);
  RFM69_writeReg(RFM69_REG_12_PA_RAMP, RF_PARAMP_40);
  RFM69_writeReg(RFM69_REG_13_OCP,  RF_OCP_OFF |
                 RF_OCP_TRIM_120);
  RFM69_writeReg(RFM69_REG_18_LNA,  RF_LNA_ZIN_50 |
                 RF_LNA_GAINSELECT_AUTO);

  RFM69_writeReg(RFM69_REG_19_RX_BW,  RF_RXBW_DCCFREQ_010 |
                 RF_RXBW_MANT_16 |
                 RF_RXBW_EXP_0);
  RFM69_writeReg(RFM69_REG_1A_AFC_BW, RF_AFCBW_DCCFREQAFC_100 |
                 RF_AFCBW_MANTAFC_20 |
                 RF_AFCBW_EXPAFC_3);

  //RFM69_writeReg(RFM69_REG_1B_OOK_PEAK,
  //RFM69_writeReg(RFM69_REG_1C_OOK_AVG,
  //RFM69_writeReg(RFM69_REG_1D_OOF_FIX,
  RFM69_writeReg(RFM69_REG_1E_AFC_FEI,  RF_AFCFEI_AFCAUTOCLEAR_OFF |
                 RF_AFCFEI_AFCAUTO_OFF);
  //RFM69_writeReg(RFM69_REG_1F_AFC_MSB,
  //RFM69_writeReg(RFM69_REG_20_AFC_LSB,
  //RFM69_writeReg(RFM69_REG_21_FEI_MSB,
  //RFM69_writeReg(RFM69_REG_22_FEI_LSB,
  //RFM69_writeReg(RFM69_REG_23_RSSI_CONFIG,
  //RFM69_writeReg(RFM69_REG_24_RSSI_VALUE,

  RFM69_writeReg(RFM69_REG_25_DIO_MAPPING1, RF_DIOMAPPING1_DIO0_01 |
                 RF_DIOMAPPING1_DIO1_00 |
                 RF_DIOMAPPING1_DIO2_00 |
                 RF_DIOMAPPING1_DIO3_00);
  RFM69_writeReg(RFM69_REG_26_DIO_MAPPING2, RF_DIOMAPPING2_DIO4_00 |
                 RF_DIOMAPPING2_DIO5_00 |
                 RF_DIOMAPPING2_CLKOUT_OFF);

  //RFM69_writeReg(RFM69_REG_27_IRQ_FLAGS1,
  //RFM69_writeReg(RFM69_REG_28_IRQ_FLAGS2,
  RFM69_writeReg(RFM69_REG_29_RSSI_THRESHOLD, RF_RSSITHRESH_VALUE);
  RFM69_writeReg(RFM69_REG_2A_RX_TIMEOUT1,  RF_RXTIMEOUT1_RXSTART_VALUE);
  RFM69_writeReg(RFM69_REG_2B_RX_TIMEOUT2,  RF_RXTIMEOUT2_RSSITHRESH_VALUE);
  RFM69_writeReg(RFM69_REG_2C_PREAMBLE_MSB, 0x00);  // 0x00
  RFM69_writeReg(RFM69_REG_2D_PREAMBLE_LSB, 0x06);  // 0x0F

  RFM69_writeReg(RFM69_REG_2E_SYNC_CONFIG,  RF_SYNC_ON |
                 RF_SYNC_FIFOFILL_AUTO |
                 RF_SYNC_SIZE_2 |
                 RF_SYNC_TOL_0);

  RFM69_writeReg(RFM69_REG_2F_SYNCVALUE1, 0x2D);  // 0x2D
  RFM69_writeReg(RFM69_REG_30_SYNCVALUE2, 0xD4);  // 0xD4
  RFM69_writeReg(RFM69_REG_31_SYNCVALUE3, 0x55);
  RFM69_writeReg(RFM69_REG_32_SYNCVALUE4, 0x55);
  RFM69_writeReg(RFM69_REG_33_SYNCVALUE5, 0x55);
  RFM69_writeReg(RFM69_REG_34_SYNCVALUE6, 0x55);
  RFM69_writeReg(RFM69_REG_35_SYNCVALUE7, 0x55);
  RFM69_writeReg(RFM69_REG_36_SYNCVALUE8, 0x55);

  RFM69_writeReg(RFM69_REG_37_PACKET_CONFIG1, RF_PACKET1_FORMAT_FIXED |
                 RF_PACKET1_DCFREE_OFF |
                 RF_PACKET1_CRC_OFF |
                 RF_PACKET1_CRCAUTOCLEAR_OFF |
                 RF_PACKET1_ADRSFILTERING_OFF);
  RFM69_writeReg(RFM69_REG_38_PAYLOAD_LENGTH, PAYLOAD_SIZE);

  //RFM69_writeReg(RFM69_REG_39_NODE_ADDRESS,
  //RFM69_writeReg(RFM69_REG_3A_BROADCAST_ADDRESS,
  RFM69_writeReg(RFM69_REG_3B_AUTOMODES,  RF_AUTOMODES_ENTER_OFF |
                 RF_AUTOMODES_EXIT_OFF |
                 RF_AUTOMODES_INTERMEDIATE_SLEEP);
  RFM69_writeReg(RFM69_REG_3C_FIFO_THRESHOLD, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY |
                 RF_FIFOTHRESH_VALUE);
  RFM69_writeReg(RFM69_REG_3D_PACKET_CONFIG2, RF_PACKET2_RXRESTARTDELAY_1BIT |
                 //RF_PACKET2_AUTORXRESTART_ON |
                 RF_PACKET2_AES_OFF);
  //RFM69_writeReg(RFM69_REG_3E_AES_KEY_BYTE1, RF_AESKEY1_VALUE);
  //...
  //RFM69_writeReg(RFM69_REG_3F_AES_KEY_BYTE2, RF_AESKEY2_VALUE);
  //RFM69_writeReg(RFM69_REG_58_TEST_LNA,
  RFM69_writeReg(RFM69_REG_5A_TEST_PA1, RF_PA20DBM1_NORMAL_AND_RX);
  RFM69_writeReg(RFM69_REG_5C_TEST_PA2, RF_PA20DBM2_NORMAL_AND_RX);
  RFM69_writeReg(RFM69_REG_6F_TEST_DAGC, RF_DAGC_IMPROVED_LOWBETA0);
  //RFM69_writeReg(RFM69_REG_71_TEST_AFC,

  RFM69_set_frequency(86982);           // 86982 = 869.82MHz Ventus remote unit carrier frequency

  RFM69_setMode(RF_OPMODE_RECEIVER);
  while (!(RFM69_readReg(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_MODEREADY));
}

uint8_t calcCRC(uint8_t *buffer, uint8_t data_size)
{
  uint8_t crc = 0xff;                                   // init = 0xff
  uint8_t data;

  for (uint8_t n = 0; n < data_size; n++)
  {
    data = buffer[n];
    for (uint8_t i = 0; i < 8; i++)
    {
      uint8_t tmp = (crc ^ data) & 0x01;
      crc >>= 1;
      if (tmp) crc ^= 0x8C;
      data >>= 1;
    }
  }

  return crc;
}

boolean Plugin_124(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;
 
  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_124;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].TimerOptional = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_124);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_124));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_124));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_124));        
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        uint8_t instanceType = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String instance[NUMBER_OF_INSTANCES];

        instance[INSTANCE_TH] = F("Main + Temp/Hygro");
        instance[INSTANCE_WIND] = F("Wind");
        instance[INSTANCE_RAIN] = F("Rain");
        instance[INSTANCE_UV] = F("UV");
        instance[INSTANCE_LIGHTNING] = F("Lightning");
        instance[INSTANCE_BATTERY] = F("Battery");

        if (instanceType == INSTANCE_TH)
          addFormNumericBox(F("Unit ID"), F("plugin_124_unitID"), Settings.TaskDevicePluginConfig[event->TaskIndex][1], 0, 255);
          
        addFormSelector(F("Plugin function"), F("plugin_124_instanceType"), NUMBER_OF_INSTANCES, instance, NULL, instanceType);

        addFormSubHeader(F("Information"));

        switch (instanceType)
        {
          case INSTANCE_TH:
            {
              addHtml(F("<BR><B>Be sure to only have one main plugin!</B>"));
              
              addHtml(F("<BR><BR>Value 1: Temperature (1 decimal)"));
              addHtml(F("<BR>Value 2: Humidity (0 decimal)"));
              addHtml(F("<BR>Value 3: not used"));
              break;
            }
          case INSTANCE_WIND:
            {
              addHtml(F("<BR>Value 1: Direction (1 decimal)"));
              addHtml(F("<BR>Value 2: Average m/s (1 decimal)"));
              addHtml(F("<BR>Value 3: Gust m/s (1 decimal)"));
              break;
            }
          case INSTANCE_RAIN:
            {
              addHtml(F("<BR>Value 1: Total rain in mm (1 decimal)"));
              addHtml(F("<BR>Value 2: Rainfall past hour in mm (1 decimal)"));
              addHtml(F("<BR>Value 3: not used"));              
              break;
            }
          case INSTANCE_UV:
            {
              addHtml(F("<BR>Value 1: UV (1 decimal)"));
              addHtml(F("<BR>Value 2: not used"));
              addHtml(F("<BR>Value 3: not used"));
              break;
            }
          case INSTANCE_LIGHTNING:
            {
              addHtml(F("<BR>Value 1: Strike counter (0 decimal)"));
              addHtml(F("<BR>Value 2: Strikes past 5 minutes (0 decimal)"));
              addHtml(F("<BR>Value 3: Distance in km (0 decimal)"));
              break;
            }
          case INSTANCE_BATTERY:
            {
              addHtml(F("<BR>Value 1: Battery low (0 decimal)"));
              addHtml(F("<BR>Value 2: not used"));
              addHtml(F("<BR>Value 3: not used"));
              break;
            }
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_124_instanceType"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_124_unitID"));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        firstrun = true;

        switch (Settings.TaskDevicePluginConfig[event->TaskIndex][0])
        {
          case INSTANCE_TH:
            {
              pinMode(Plugin_124_RESET_Pin, OUTPUT);
              pinMode(Plugin_124_DIO0_Pin, INPUT);
              pinMode(Plugin_124_SPI_CS_Pin, OUTPUT);

              // initialize SPI
              SPI.setHwCs(false);
              SPI.begin();
              addLog(LOG_LEVEL_INFO, F("P124 : SPI Init"));

              // initilize RFM69
              RFM69init();
              
              addLog(LOG_LEVEL_INFO, F("P124 : RFM69 Init"));

              break;
            }
        }
        success = true;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        // Main TH-instance calling?
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == INSTANCE_TH)        
        {
          if (dataPending == true)
          {
            timeSlot++;
          }
          else
          {
            // received data pending?
            if (digitalRead(Plugin_124_DIO0_Pin) == HIGH)
            {
              uint8_t i = 0;
              uint8_t buffer[PAYLOAD_SIZE];
      
              // fetch FIFO
              // receiver is disabled until FIFO is completely empty -> receiver is auto-enabled when done
              digitalWrite(Plugin_124_SPI_CS_Pin, LOW);
              SPI.transfer(RFM69_REG_00_FIFO & ~0x80);
              while ((digitalRead(Plugin_124_DIO0_Pin) == HIGH) && (i < PAYLOAD_SIZE))
                buffer[i++] = SPI.transfer(0x00);
              digitalWrite(Plugin_124_SPI_CS_Pin, HIGH);
      
#ifdef PLUGIN_124_DEBUG
              // CRC correct?
              if (buffer[21] == calcCRC((uint8_t*)buffer, (PAYLOAD_SIZE - 1)))
              {
                String log = F("P124 : RX: ");
      
                for (i = 0; i < PAYLOAD_SIZE; i++)
                {
                  //log += String(buffer[i], HEX);    // hexadecimal output format
                  log += buffer[i];                   // decimal output format
                  log += F(" ");
                }
      
                // Ventus ID matches specified unit ID?
                if (buffer[0] == Settings.TaskDevicePluginConfig[event->TaskIndex][1])
                  log += F(" IDs match");
                else
                  log += F(" IDs do not match!");
      
                addLog(LOG_LEVEL_INFO, log);
              }
#endif  // PLUGIN_124_DEBUG
      
              // CRC correct and IDs match?
              if ((buffer[21] == calcCRC((uint8_t*)buffer, (PAYLOAD_SIZE - 1))) && (buffer[0] == Settings.TaskDevicePluginConfig[event->TaskIndex][1]))
              {
                humidity = (buffer[1] & 0x0F) + (buffer[1] >> 4) * 10;                      // %rel LF. (BCD coded)
                temperature = (float)((int16_t)((buffer[4] << 8) + buffer[3])) / 10.0;      // °C (todo: check offset)
                batteryLow = buffer[5] & 0x01;                                              // 0=ok, 1=low
                windDIR = float(buffer[8] & 0x0F) * 22.5;                                   // 0..360°
                windAVG = float((buffer[10] << 8) + buffer[9]) / 10.0;                      // m/s
                windGUST = float((buffer[12] << 8) + buffer[11]) / 10.0;                    // m/s
                rainTotal = float((buffer[14] << 8) + buffer[13]) / 4.0;                    // mm
                uv = (float)buffer[16] / 10.0;                                              // UV index
                if (buffer[17] == 0x3F)
                  strikesDistance = -1;
                else
                  strikesDistance = buffer[17];                                             // km
                strikesTotal = (buffer[20] << 8) + buffer[19];                              // count
      
                // Data plausibility check
                if ((humidity <= 100) &&
                    (temperature >= -20) && (temperature <= 60) &&
                    (windDIR <= 360) &&
                    (windAVG <= 30) &&
                    (windGUST <= 30) &&
                    (uv <= 15) &&
                    (strikesDistance <= 30)
                   )
                {
                  dataValid = true;
                  dataPending = true;
                  timeSlot = 0;
                }
              }
            }
      
            // sensor data is initialized and valid?
            if (dataValid)
            {
              if (firstrun)
              {
                // initialize reference values for statistics
                firstrun = false;
                strikes5minutesAgoe = strikesTotal;
                rainLevelOneHourAgoe = rainTotal;
              }
            }
          }

          // timer for strikes-statistic
          if (timer300s-- <= 1)
          {
            timer300s = 300;
            if (strikesTotal >= strikes5minutesAgoe)
              strikesPast5minutes = strikesTotal - strikes5minutesAgoe;
            else
              strikesPast5minutes = (strikesTotal + 0xFFFF) - strikes5minutesAgoe;
            strikes5minutesAgoe = strikesTotal;
          }
      
          // timer for rain-statistic
          if (timer3600s-- <= 1)
          {
            timer3600s = 3600;
            if (rainTotal >= rainLevelOneHourAgoe)
              rainLevelPastHour = rainTotal - rainLevelOneHourAgoe;
            else
              rainLevelPastHour = (rainTotal + 0xFFFF) - rainLevelOneHourAgoe;
            rainLevelOneHourAgoe = rainTotal;
          }
        }


        // TRANSFER DATA
        if (dataPending == true)
        {
          // check which instance is calling and send data
          switch (Settings.TaskDevicePluginConfig[event->TaskIndex][0])
          {
            case INSTANCE_TH:
            {
              if (timeSlot == 0)
                {
                  UserVar[event->BaseVarIndex] = temperature;
                  UserVar[event->BaseVarIndex + 1] = humidity;
                  UserVar[event->BaseVarIndex + 2] = 0;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
            }
              
            case INSTANCE_WIND:
              {
                if (timeSlot == 1)
                {
                  UserVar[event->BaseVarIndex] = windDIR;
                  UserVar[event->BaseVarIndex + 1] = windAVG;
                  UserVar[event->BaseVarIndex + 2] = windGUST;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
              }
         
            case INSTANCE_RAIN:
              {
                if (timeSlot == 2)
                {
                  UserVar[event->BaseVarIndex] = rainTotal;
                  UserVar[event->BaseVarIndex + 1] = rainLevelPastHour;
                  UserVar[event->BaseVarIndex + 2] = 0;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
              }
  
            case INSTANCE_UV:
              {
                if (timeSlot == 3)
                {
                  UserVar[event->BaseVarIndex] = uv;
                  UserVar[event->BaseVarIndex + 1] = 0;
                  UserVar[event->BaseVarIndex + 2] = 0;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
              }
              
            case INSTANCE_LIGHTNING:
              {
                if (timeSlot == 4)
                {
                  UserVar[event->BaseVarIndex] = strikesTotal;
                  UserVar[event->BaseVarIndex + 1] = strikesPast5minutes;
                  UserVar[event->BaseVarIndex + 2] = strikesDistance;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
              }
              
            case INSTANCE_BATTERY:
              {
                if (timeSlot == 5)
                {
                  UserVar[event->BaseVarIndex] = batteryLow;
                  UserVar[event->BaseVarIndex + 1] = 0;
                  UserVar[event->BaseVarIndex + 2] = 0;
                  event->sensorType = SENSOR_TYPE_TRIPLE;
                  sendData(event);
                }
                break;
              }
          }
  
          if (timeSlot > 5)
          {
            timeSlot = 0;
            dataPending = false;
          }
        }
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        success = false;
        break;
      }      
  }
  return success;
}

#endif
