
// OUTPUT PINS 
#define PIN_PUMP 31
#define PIN_ARM_ENABLE 26
#define PIN_ARM_EXTEND 25
#define PIN_ARM_RETRACT 24


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
}


void loop() {
  Serial.println("1/6 Pump ON");
  digitalWrite(PIN_PUMP, HIGH);
  wait_for_input();

  Serial.println("2/6 Pump OFF, Arm enabled, but not moving");
  digitalWrite(PIN_PUMP, LOW);
  digitalWrite(PIN_ARM_ENABLE, HIGH);
  wait_for_input();

  Serial.println("3/6 Arm extend");
  digitalWrite(PIN_ARM_EXTEND, HIGH);
  wait_for_input();

  Serial.println("4/6 Arm retract");
  digitalWrite(PIN_ARM_EXTEND, LOW);
  digitalWrite(PIN_ARM_RETRACT, HIGH);
  wait_for_input();

  Serial.println("5/6 Arm disabled, retract still high (should be stopped)");
  digitalWrite(PIN_ARM_ENABLE, LOW);
  wait_for_input();

  Serial.println("6/6 Arm stopped, should have no outputs");
  digitalWrite(PIN_ARM_RETRACT, LOW);
  wait_for_input();
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











