#define BATTERY_SEND_INTERVAL_MINUTES 360 // 6 hours
#define TEMPERATURE_SEND_INTERVAL_MINUTES 30

//#define RAIN_GAUGE_PIN GPIO5
//#define ONE_WIRE_PIN GPIO6
#define VH400_PIN ADC2
// #define SHT1x_DATA_PIN
// #define SHT1x_CLOCK_PIN

#define DEFAULT_MM_PER_COUNT 0.254 // 0.01"


/* OTAA para should be set via AT commands */
uint8_t devEui[] = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xCA, 0xFE };
uint8_t appEui[] = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xCA, 0xFE };
uint8_t appKey[] = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0x42 };

/* ABP para*/
uint8_t nwkSKey[] = {};
uint8_t appSKey[] = {};
uint32_t devAddr = (uint32_t)0x0;

/*LoraWan channelsmask, default channels 0-7 sub 2*/
uint16_t userChannelsMask[6] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = LORAWAN_CLASS;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
//bool keepNet = LORAWAN_NET_RESERVE;
bool keepNet = false;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = false;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;
