// TEST 1: Sensor Range Check — rerunnable
// Validates TMP36 on A4 and photoresistor on A5
// Type 'r' in serial monitor to run/rerun after adjusting light slider
//
// NOTE: Photoresistor defaults to 0V (total darkness) at sim start.
//       Click the photoresistor in TinkerCAD and raise the light slider
//       before running.

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("========================================");
  Serial.println("TEST 1: SENSOR WIRING VALIDATION");
  Serial.println("Type 'r' to run/rerun (adjust light first)");
  Serial.println("========================================");
}

void runTest() {
  int passes = 0;
  int fails = 0;

  // TMP36 on A4
  int tmp36 = analogRead(A4);
  float voltage = tmp36 * (5.0 / 1023.0);
  float tempC = (voltage - 0.5) * 100.0;

  Serial.print("\nA4 (TMP36): ADC=");
  Serial.print(tmp36);
  Serial.print(" | ");
  Serial.print(tempC, 1);
  Serial.print("C  -->  ");

  if (tmp36 > 50 && tmp36 < 300) {
    Serial.println("PASS");
    passes++;
  } else {
    Serial.println("FAIL");
    fails++;
  }

  // Photoresistor on A5
  int photo = analogRead(A5);
  Serial.print("A5 (Photo): ADC=");
  Serial.print(photo);
  Serial.print("  -->  ");

  if (photo > 0 && photo < 1023) {
    Serial.println("PASS");
    passes++;
  } else if (photo == 0) {
    Serial.println("FAIL (0 = total darkness or disconnected)");
    Serial.println("   ^ Slide light level up in TinkerCAD, then type 'r'");
    fails++;
  } else {
    Serial.println("FAIL (1023 = pull-down resistor missing)");
    fails++;
  }

  Serial.print("\nRESULT: ");
  Serial.print(passes);
  Serial.print("/2 PASS  ");
  Serial.println(fails > 0 ? "-- type 'r' to rerun" : "");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      runTest();
    }
  }
}
