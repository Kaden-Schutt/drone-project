// TEST 5: Flight Profile Demo
//
// Phase 1: Ramp 0->200 over 5s (takeoff)
// Phase 2: Hover at 200 all motors for 5s
// Phase 3: Pitch -- front fast, rear slow for 5s
// Phase 4: Roll -- left fast, right slow for 5s
// Phase 5: Yaw -- diagonal pairs opposite direction for 5s
//
// Total runtime: ~30 seconds

const int EN_MLB = 5;   const int MLB_A = 13;  const int MLB_B = 12;
const int EN_MRB = 6;   const int MRB_A = 7;   const int MRB_B = 2;
const int EN_MLF = 9;   const int MLF_A = A1;  const int MLF_B = A0;
const int EN_MRF = 10;  const int MRF_A = 11;  const int MRF_B = 8;

const int allPins[] = {5,6,9,10,12,13,7,2,A1,A0,11,8};

void allOff() {
  for (int i = 0; i < 12; i++) digitalWrite(allPins[i], LOW);
}

void allForward() {
  digitalWrite(MLB_A, HIGH); digitalWrite(MLB_B, LOW);
  digitalWrite(MRB_A, HIGH); digitalWrite(MRB_B, LOW);
  digitalWrite(MLF_A, HIGH); digitalWrite(MLF_B, LOW);
  digitalWrite(MRF_A, HIGH); digitalWrite(MRF_B, LOW);
}

void setSpeeds(int mlb, int mrb, int mlf, int mrf) {
  analogWrite(EN_MLB, mlb);
  analogWrite(EN_MRB, mrb);
  analogWrite(EN_MLF, mlf);
  analogWrite(EN_MRF, mrf);
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 12; i++) {
    pinMode(allPins[i], OUTPUT);
    digitalWrite(allPins[i], LOW);
  }
  delay(500);

  Serial.println("==========================================");
  Serial.println("TEST 5: FLIGHT PROFILE DEMO");
  Serial.println("==========================================\n");

  // --- PHASE 1: Takeoff ramp (5s) ---
  Serial.println(">> PHASE 1: Takeoff ramp (0 -> 200 over 5s)");
  Serial.println("   All 4 motors should gradually speed up together.");
  allForward();
  for (int pwm = 0; pwm <= 200; pwm++) {
    setSpeeds(pwm, pwm, pwm, pwm);
    delay(25);  // 200 steps x 25ms = 5s
  }
  Serial.println("   PASS if all 4 ramped up smoothly.\n");

  // --- PHASE 2: Hover (5s) ---
  Serial.println(">> PHASE 2: Hover (all motors @ 200 for 5s)");
  Serial.println("   All 4 motors should spin at equal speed.");
  setSpeeds(200, 200, 200, 200);
  delay(5000);
  Serial.println("   PASS if all 4 matched speed.\n");

  // --- PHASE 3: Pitch forward (5s) ---
  Serial.println(">> PHASE 3: Pitch forward (5s)");
  Serial.println("   Front=240, Rear=120 -- front pair should spin faster.");
  setSpeeds(120, 120, 240, 240);  // MLB,MRB slow | MLF,MRF fast
  delay(5000);
  Serial.println("   PASS if front > rear speed.\n");

  // --- PHASE 4: Roll right (5s) ---
  Serial.println(">> PHASE 4: Roll right (5s)");
  Serial.println("   Left=240, Right=120 -- left pair should spin faster.");
  setSpeeds(240, 120, 240, 120);  // MLB,MLF fast | MRB,MRF slow
  delay(5000);
  Serial.println("   PASS if left > right speed.\n");

  // --- PHASE 5: Yaw clockwise (5s) ---
  Serial.println(">> PHASE 5: Yaw CW (5s)");
  Serial.println("   MLB+MRF forward, MRB+MLF reverse -- torque imbalance spins frame.");
  allOff();
  digitalWrite(MLB_A, HIGH); digitalWrite(MLB_B, LOW);   // forward
  digitalWrite(MRF_A, HIGH); digitalWrite(MRF_B, LOW);   // forward
  digitalWrite(MRB_A, LOW);  digitalWrite(MRB_B, HIGH);  // reverse
  digitalWrite(MLF_A, LOW);  digitalWrite(MLF_B, HIGH);  // reverse
  setSpeeds(200, 200, 200, 200);
  delay(5000);
  Serial.println("   PASS if diagonal pairs spin opposite directions.\n");

  // --- SHUTDOWN ---
  allOff();
  Serial.println("==========================================");
  Serial.println("FLIGHT PROFILE COMPLETE");
  Serial.println("  Phase 1: Ramp    -- smooth speedup?");
  Serial.println("  Phase 2: Hover   -- all equal?");
  Serial.println("  Phase 3: Pitch   -- front > rear?");
  Serial.println("  Phase 4: Roll    -- left > right?");
  Serial.println("  Phase 5: Yaw     -- diagonals opposite?");
  Serial.println("==========================================");
}

void loop() {}
