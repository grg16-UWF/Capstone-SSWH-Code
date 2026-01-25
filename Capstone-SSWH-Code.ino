
#define DEBUG true; // Enable debug output to serial port

// INPUT PINS

#define PIN_TEMP_ONEWIRE 4;
#define PIN_INCLINOMETER 6;

// OUTPUT PINS 

#define PIN_RELAY_PUMP 26;
#define PUN_RELAY_ACUTATOR 24;


// Temp sensor data
int temperatures[4];

// indices of temperatures[] array
#define TEMP_IDX_INPUT 0;
#define TEMP_IDX_COLLECTOR 1;
#define TEMP_IDX_TANK 2;
#define TEMP_IDX_AIR 3;

#define TEMP_ADDR_INPUT 11;
#define TEMP_ADDR_COLLECTOR 12;
#define TEMP_ADDR_TANK 13;
#define TEMP_ADDR_AIR 14;


void setup()
{
	// Setup wifi, hopefully accessable from matter
  // update RTC from ntp if possible, otherwise assume time is already correct in case of no internet

  // init matter
}

void loop()
{
	// read temp sensors

  // send sensor data over matter

  // enable/disable pump if needed (check time since last toggle)

  // read angle from inclinometer

  // read or calculate solar noon for today if not already found.

  // calculate offset from solar noon and desired angle

  // set acutator to correct position based on desired angle and current angle

}
