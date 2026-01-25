#include <WiFi.h> // for checking if WiFi is connected to update RTC
#include "time.h" 
#include "esp_sntp.h"

#define DEBUG true // Enable debug output to serial port

// INPUT PINS

#define PIN_TEMP_ONEWIRE 4
#define PIN_INCLINOMETER 6

// OUTPUT PINS 

#define PIN_RELAY_PUMP 26
#define PUN_RELAY_ACUTATOR 24


// Temp sensor OneWire addresses (placeholder)
#define TEMP_ADDR_INPUT 11
#define TEMP_ADDR_COLLECTOR 12
#define TEMP_ADDR_TANK 13
#define TEMP_ADDR_AIR 14


// RTC SETUP
bool RTC_SYNCED = false;
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
#define gmtOffset_sec (-6 * 3600)
#define daylightOffset_sec 3600


void setup() {
  if( DEBUG ) {
    Serial.begin(115200);
  }

	// Setup wifi, hopefully accessable from matter
  // update RTC from ntp if possible, otherwise assume time is already correct in case of no internet

  // init matter
}

void loop() {
  // If matter has started a WiFi connection AND the RTC has not already been updated, then try to update the RTC
  if( RTC_SYNCED && (WiFi.status() != WL_CONNECTED) ) {
    rtc_sync();
  }


  // Temp sensor data
  int temp_input;
  int temp_collector;
  int temp_tank;
  int temp_air;

	// read temp sensors
  // random data until sensors connected
  temp_input = random(15, 19);
  temp_collector = random(24, 29);
  temp_tank = random(35, 39);
  temp_air = random(15, 25);


  // send sensor data over matter
  if( DEBUG ) {
    Serial.printf("Temp Input:    %d C\n", temp_input);
    Serial.printf("Temp Collect:  %d C\n", temp_collector);
    Serial.printf("Temp Tank:     %d C\n", temp_tank);
    Serial.printf("Temp Air:      %d C\n\n", temp_air);
  }

  // enable/disable pump if needed (check time since last toggle)


  // read angle from inclinometer

  // read or calculate solar noon for today if not already found.

  // calculate offset from solar noon and desired angle

  // set acutator to correct position based on desired angle and current angle

}



void rtc_sync() {
  sntp_set_time_sync_notification_cb(rtc_sync_callback); // set callback func to update RTC_SYNCED and print to debug.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

}

void rtc_sync_callback(struct timeval *t) {
  if( DEBUG ) {
    Serial.println("Got time adjustment from NTP!");
  }
  RTC_SYNCED = true;
}
