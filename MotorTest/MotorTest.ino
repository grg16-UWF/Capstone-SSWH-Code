
// OUTPUT PINS 
#define PIN_PUMP 18
#define PIN_ARM_ENABLE 17
#define PIN_ARM_EXTEND 4
#define PIN_ARM_RETRACT 16


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n[STARTUP].............................................");

  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);

  pinMode(PIN_ARM_ENABLE, OUTPUT);
  digitalWrite(PIN_ARM_ENABLE, LOW);

  pinMode(PIN_ARM_EXTEND, OUTPUT);
  digitalWrite(PIN_ARM_EXTEND, LOW);

  pinMode(PIN_ARM_RETRACT, OUTPUT);
  digitalWrite(PIN_ARM_RETRACT, LOW);

  Serial.println("Pin modes set.");
  Serial.println("Any input to serial will proceed to next step.\n");

  Serial.readString();
  delay(250);

}


void loop() {
  delay(1000);

  Serial.println("\nHow many seconds to extend?");
  while( Serial.available() == 0 ) { delay(25); }
  String timeStr = Serial.readString();
  float time = timeStr.toFloat();

  Serial.printf("Ready to extend for %fs", time);
  wait_for_input();
  digitalWrite(PIN_ARM_ENABLE, HIGH);
  digitalWrite(PIN_ARM_EXTEND, HIGH);

  delays(time);

  digitalWrite(PIN_ARM_EXTEND, LOW);
  digitalWrite(PIN_ARM_ENABLE, LOW);

  Serial.println("Ready to retract...");
  wait_for_input();

  digitalWrite(PIN_ARM_ENABLE, HIGH);
  digitalWrite(PIN_ARM_RETRACT, HIGH);

  Serial.println("Waiting to stop retracting...");
  wait_for_input();

  digitalWrite(PIN_ARM_RETRACT, LOW);
  digitalWrite(PIN_ARM_ENABLE, LOW);



  // Serial.printf("1/6 Pump[%d] ON\n", PIN_PUMP);
  // digitalWrite(PIN_PUMP, HIGH);
  // wait_for_input();

  // Serial.printf("2/6 Pump[%d] OFF, Arm enabled[%d], but not moving\n", PIN_PUMP, PIN_ARM_ENABLE);
  // digitalWrite(PIN_PUMP, LOW);
  // digitalWrite(PIN_ARM_ENABLE, HIGH);
  // wait_for_input();

  // Serial.printf("3/6 Arm extend[%d]\n", PIN_ARM_EXTEND);
  // digitalWrite(PIN_ARM_EXTEND, HIGH);
  // wait_for_input();

  // Serial.printf("4/6 Arm retract[%d]\n", PIN_ARM_RETRACT);
  // digitalWrite(PIN_ARM_EXTEND, LOW);
  // digitalWrite(PIN_ARM_RETRACT, HIGH);
  // wait_for_input();

  // Serial.printf("5/6 Arm disabled[%d], retract[%d] still high (should be stopped)", PIN_ARM_ENABLE, PIN_ARM_RETRACT);
  // digitalWrite(PIN_ARM_ENABLE, LOW);
  // wait_for_input();

  // Serial.printf("6/6 Arm retract stopped[%d], should have no outputs", PIN_ARM_RETRACT);
  // digitalWrite(PIN_ARM_RETRACT, LOW);
  // wait_for_input();
}


void wait_for_input() {
  int counter = 0;
  while( Serial.available() <= 0 ) { // check for input every 50ms
    delay(50);
    counter++;

    if(counter >= 10) { // but only print a "." every 500ms
      Serial.print(".");
      counter = 0;
    }

  }

  while( Serial.available() > 0 ) { // faster than Serial.readString(), I only want to purge the buffer
    char __attribute__((unused)) temp = Serial.read();
  }

  Serial.print("\n");
}

void delays(float sec) {
  Serial.print("Delaying");
  int totalTime = sec * 1000.0;

  while( totalTime > 0 ) {
    int sleep = 500;
    if( sleep > totalTime ) {
      sleep = totalTime;
    }
    totalTime -= sleep;
    delay(sleep);
    Serial.print(".");
  }
}










