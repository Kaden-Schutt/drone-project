// TEST 2: Servo Valve on D5 (manual pulse, no Servo.h)
// Sweeps 0° -> 90° -> 180° -> 0°
// Visually confirm servo arm moves in TinkerCAD

const int servoPin = 5;

void servoCommand(int angle) {
  angle = constrain(angle, 0, 180);
  for (int i = 0; i < 25; i++) {
    int pulse = map(angle, 0, 180, 544, 2400);
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(pulse);
    digitalWrite(servoPin, LOW);
    delay(20);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(servoPin, OUTPUT);
  delay(500);
  Serial.println("========================================");
  Serial.println("TEST 2: SERVO VALVE WIRING (D5)");
  Serial.println("  Manual pulse — no Servo.h, no timer hijack");
  Serial.println("========================================");

  Serial.println("\nSweeping -- watch the servo arm in TinkerCAD:");

  Serial.print("  0 degrees... ");
  servoCommand(0);
  Serial.println("done");

  Serial.print("  90 degrees... ");
  servoCommand(90);
  Serial.println("done");

  Serial.print("  180 degrees... ");
  servoCommand(180);
  Serial.println("done");

  Serial.print("  Back to 0... ");
  servoCommand(0);
  Serial.println("done");

  Serial.println("\n----------------------------------------");
  Serial.println("SERVO RESULT: PASS (confirm arm moved visually)");
  Serial.println("========================================");
}

void loop() {}
