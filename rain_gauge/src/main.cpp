#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "common.h"
#include <CayenneLPP.h>

#define DEFAULT_MM_PER_COUNT 0.254
#define ROW 0
#define ROW_OFFSET 100

//CY_FLASH_SIZEOF_ROW is 256 , CY_SFLASH_USERBASE is 0x0ffff400
#define addr CY_SFLASH_USERBASE + CY_FLASH_SIZEOF_ROW* ROW + ROW_OFFSET

// send every 5 minutes
uint8_t send_interval = 1;
uint32_t appTxDutyCycle = send_interval * 60000;
uint8_t battery_send_interval = 5;
uint8_t battery_send_counter = 0;

// use port 2
uint8_t appPort = 2;

// union to store directly on EEPROM
union {
    float number;
    uint8_t bytes[4];
} mm_per_tip;

// the tip counter and mm, reset after each cycle
uint8_t tips = 0;

static unsigned long last_switch;

// prepare the outgoing package
static void prepareTxFrame(uint8_t port)
{
    // contains the LoRa frame
    CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);
    // send battery status only every "battery_send_interval" minutes
    if (battery_send_counter <= 0) {
	lpp.addVoltage(1, getBatteryVoltage() / 1000.0);
	battery_send_counter = battery_send_interval;
    } else {
	Serial.println("Skip adding batter volt");
	battery_send_counter -= send_interval;
    }

    if (tips > 0) {
	Serial.printf("Tips = %i\n", tips);
	lpp.addAnalogInput(1, mm_per_tip.number * tips);
	tips = 0;
    } else {
	Serial.println("Skip adding rain fall");
    }

    appDataSize = lpp.getSize();
    Serial.printf("appDataSize = %i\n", appDataSize);
    lpp.getBuffer(), memcpy(appData, lpp.getBuffer(), appDataSize);
}

// custom commands to provision device
bool checkUserAt(char* cmd, char* content)
{
    // this command is used to set the seesaw size
    if (strcmp(cmd, "mmPerTip") == 0) {
	if (atof(content) != mm_per_tip.number) {
	    mm_per_tip.number = atof(content);
	    Serial.print("Set mm per tip to ");
	    Serial.print(mm_per_tip.number, 4);
	    Serial.println("mm");
	    Serial.println();
	    FLASH_update(addr, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
	} else {
	    Serial.println("Same mm per tip as before");
	}

	return true;
    }
    return false;
}

// run on each interrupt aka tip of the seesaw
void interrupt_handler()
{
    // debounce for a quarter second = max. 4 counts per second
    if (labs(millis() - last_switch) > 250) {
	tips++;
	last_switch = millis();
    }
}

void setup()
{
    Serial.begin(115200);

    // reach mm per tip from EEPROM
    FLASH_read_at(addr, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
    if (mm_per_tip.number == 0.0) {
	mm_per_tip.number = DEFAULT_MM_PER_COUNT;
    }
    Serial.printf("Current mm per tip is ");
    Serial.print(mm_per_tip.number, 4);
    Serial.println("mm");

    // enable user input via Serial
    enableAt();

    deviceState = DEVICE_STATE_INIT;

    // enable interrupt pin
    PINMODE_INPUT_PULLDOWN(GPIO3);
    attachInterrupt(GPIO3, interrupt_handler, FALLING);

    LoRaWAN.ifskipjoin();
}

void loop()
{
    switch (deviceState) {
    case DEVICE_STATE_INIT: {
	getDevParam();
	printDevParam();
	LoRaWAN.init(loraWanClass, loraWanRegion);
	deviceState = DEVICE_STATE_JOIN;
	break;
    }
    case DEVICE_STATE_JOIN: {
	LoRaWAN.join();
	break;
    }
    case DEVICE_STATE_SEND: {
	prepareTxFrame(appPort);

	// don't send empty packages
	if (appDataSize > 0) {
	    LoRaWAN.send();
	} else {
	    Serial.println("Package is empty, don't send anything");
	}
	deviceState = DEVICE_STATE_CYCLE;
	break;
    }
    case DEVICE_STATE_CYCLE: {
	txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
	LoRaWAN.cycle(txDutyCycleTime);
	deviceState = DEVICE_STATE_SLEEP;
	break;
    }
    case DEVICE_STATE_SLEEP: {
	LoRaWAN.sleep();
	break;
    }
    default: {
	deviceState = DEVICE_STATE_INIT;
	break;
    }
    }
}
