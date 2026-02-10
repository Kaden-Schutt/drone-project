// TEST 4: Direction Reversal
// Each motor: spins direction A for 2s, stops, then direction B for 2s
// Validates both direction input pins are wired correctly
// If motor spins same direction both times -> an input pin is disconnected

const int EN_MLB = 5;   const int MLB_A = 13;  const int MLB_B = 12;
const int EN_MRB = 6;   const int MRB_A = 7;   const int MRB_B = 2;
const int EN_MLF = 9;   const int MLF_A = A1;  const int MLF_B = A0;
const int EN_MRF = 10;  const int MRF_A = 11;  const int MRF_B = 8;

const int allPins[] = {5,6,9,10,12,13,7,2,A1,A0,11,8};

void allOff() {
  for (int i = 0; i < 12; i++) digitalWrite(allPins[i], LOW);
}

void testMotor(const char* name, int en, int inA, int inB) {
  Serial.print(">> ");
  Serial.print(name);
  Serial.println(": Direction A (1A=HIGH, 2A=LOW)...");
  allOff();
  digitalWrite(inA, HIGH);
  digitalWrite(inB, LOW);
  analogWrite(en, 180);
  delay(2000);
  allOff();
  delay(500);

  Serial.print(">> ");
  Serial.print(name);
  Serial.println(": Direction B (1A=LOW, 2A=HIGH)...");
  allOff();
  digitalWrite(inA, LOW);
  digitalWrite(inB, HIGH);
  analogWrite(en, 180);
  delay(2000);
  allOff();
  delay(500);

  Serial.print("   ");
  Serial.print(name);
  Serial.println(": PASS if direction reversed. FAIL if same both times.\n");
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 12; i++) {
    pinMode(allPins[i], OUTPUT);
    digitalWrite(allPins[i], LOW);
  }
  delay(500);

  Serial.println("==========================================");
  Serial.println("TEST 4: DIRECTION REVERSAL");
  Serial.println("Each motor should change direction.");
  Serial.println("==========================================\n");

  testMotor("MLB (Left Back)",   EN_MLB, MLB_A, MLB_B);
  testMotor("MRB (Right Back)",  EN_MRB, MRB_A, MRB_B);
  testMotor("MLF (Left Front)",  EN_MLF, MLF_A, MLF_B);
  testMotor("MRF (Right Front)", EN_MRF, MRF_A, MRF_B);

  Serial.println("==========================================");
  Serial.println("DIRECTION RESULT:");
  Serial.println("  PASS = direction changed for all 4 motors");
  Serial.println("  FAIL = any motor same direction both ways");
  Serial.println("==========================================");
}

void loop() {}
