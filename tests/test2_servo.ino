// TEST 2: Servo Valve on D3
// Sweeps 0° -> 90° -> 180° -> 0° and checks attachment
// Visually confirm servo arm moves in TinkerCAD

#include <Servo.h>

Servo valve;

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("========================================");
  Serial.println("TEST 2: SERVO VALVE WIRING (D3)");
  Serial.println("========================================");

  valve.attach(3);
  delay(200);

  if (valve.attached()) {
    Serial.println("Servo attached on D3  -->  PASS");
  } else {
    Serial.println("Servo attach FAILED on D3  -->  FAIL");
    Serial.println("   ^ Check blue wire from D3 to servo SIG pin");
    return;
  }

  Serial.println("\nSweeping -- watch the servo arm in TinkerCAD:");

  Serial.print("  0 degrees... ");
  valve.write(0);
  delay(1000);
  Serial.println("done");

  Serial.print("  90 degrees... ");
  valve.write(90);
  delay(1000);
  Serial.println("done");

  Serial.print("  180 degrees... ");
  valve.write(180);
  delay(1000);
  Serial.println("done");

  Serial.print("  Back to 0... ");
  valve.write(0);
  delay(1000);
  Serial.println("done");

  Serial.println("\n----------------------------------------");
  Serial.println("SERVO RESULT: PASS (confirm arm moved visually)");
  Serial.println("========================================");
}

void loop() {}
