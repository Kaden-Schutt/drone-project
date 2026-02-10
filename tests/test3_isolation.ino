// TEST 3: Motor Isolation
// Spins each motor ALONE for 3 seconds
// All other motors OFF -- if more than one spins, wiring is crossed
//
// Pin map (from master doc, MLB polarity corrected):
//   MLB: EN=D5,  A=D13, B=D12  (UREAR 1-2, polarity reversed)
//   MRB: EN=D6,  A=D7,  B=D2   (UREAR 3-4)
//   MLF: EN=D9,  A=A1,  B=A0   (UFRONT 1-2)
//   MRF: EN=D10, A=D11, B=D8   (UFRONT 3-4)

const int EN_MLB = 5;   const int MLB_A = 13;  const int MLB_B = 12;
const int EN_MRB = 6;   const int MRB_A = 7;   const int MRB_B = 2;
const int EN_MLF = 9;   const int MLF_A = A1;  const int MLF_B = A0;
const int EN_MRF = 10;  const int MRF_A = 11;  const int MRF_B = 8;

const int allPins[] = {5,6,9,10,12,13,7,2,A1,A0,11,8};

void allOff() {
  for (int i = 0; i < 12; i++) {
    digitalWrite(allPins[i], LOW);
  }
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 12; i++) {
    pinMode(allPins[i], OUTPUT);
    digitalWrite(allPins[i], LOW);
  }
  delay(500);

  Serial.println("==========================================");
  Serial.println("TEST 3: MOTOR ISOLATION");
  Serial.println("Only ONE motor should spin at a time.");
  Serial.println("If wrong motor spins -> wiring is crossed.");
  Serial.println("==========================================\n");

  // --- MLB (Left Back) ---
  Serial.println(">> MLB (Left Back) spinning...");
  Serial.println("   Expected: ONLY the LEFT-BACK motor spins");
  allOff();
  digitalWrite(MLB_A, HIGH);
  digitalWrite(MLB_B, LOW);
  analogWrite(EN_MLB, 200);
  delay(3000);
  allOff();
  Serial.println("   MLB stopped.\n");
  delay(1000);

  // --- MRB (Right Back) ---
  Serial.println(">> MRB (Right Back) spinning...");
  Serial.println("   Expected: ONLY the RIGHT-BACK motor spins");
  allOff();
  digitalWrite(MRB_A, HIGH);
  digitalWrite(MRB_B, LOW);
  analogWrite(EN_MRB, 200);
  delay(3000);
  allOff();
  Serial.println("   MRB stopped.\n");
  delay(1000);

  // --- MLF (Left Front) ---
  Serial.println(">> MLF (Left Front) spinning...");
  Serial.println("   Expected: ONLY the LEFT-FRONT motor spins");
  allOff();
  digitalWrite(MLF_A, HIGH);
  digitalWrite(MLF_B, LOW);
  analogWrite(EN_MLF, 200);
  delay(3000);
  allOff();
  Serial.println("   MLF stopped.\n");
  delay(1000);

  // --- MRF (Right Front) ---
  Serial.println(">> MRF (Right Front) spinning...");
  Serial.println("   Expected: ONLY the RIGHT-FRONT motor spins");
  allOff();
  digitalWrite(MRF_A, HIGH);
  digitalWrite(MRF_B, LOW);
  analogWrite(EN_MRF, 200);
  delay(3000);
  allOff();
  Serial.println("   MRF stopped.\n");
  delay(1000);

  Serial.println("==========================================");
  Serial.println("ISOLATION RESULT:");
  Serial.println("  PASS if each motor spun alone");
  Serial.println("  FAIL if wrong motor spun or none spun");
  Serial.println("==========================================");
}

void loop() {}
