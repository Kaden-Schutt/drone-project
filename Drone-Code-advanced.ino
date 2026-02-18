// ================================================================
//  FIREFIGHTING DRONE — Advanced Edition
//  FSE 100 Mini Project
//
//  Same pinout as Drone-Code-v1.ino, with added features:
//    - Flight state machine (prevents invalid operations)
//    - Autonomous firefighting sequence
//    - Smooth speed ramping (gradual accel/decel)
//    - Battery simulation with low-power warnings
//    - Motor self-test command
//    - Proportional water flow (variable servo angle)
//    - Emergency landing
//    - Flight uptime tracking
//
//  Timer0 (D5/D6)  — untouched (millis/delay/Serial safe)
//  Timer1 (D9/D10) — back motor enables
//  Timer2 (D3/D11) — front motor enables
// ================================================================

// ============ PIN ASSIGNMENTS (same as original) ============
const int leftBackEnable   = 9;
const int rightBackEnable  = 10;
const int leftFrontEnable  = 3;
const int rightFrontEnable = 11;

const int leftBackDirA   = 13;  // swapped (MLB polarity fix)
const int leftBackDirB   = 12;
const int rightBackDirA  = 7;
const int rightBackDirB  = 2;
const int leftFrontDirA  = A1;
const int leftFrontDirB  = A0;
const int rightFrontDirA = 4;
const int rightFrontDirB = 8;

const int tempSensorPin  = A4;
const int lightSensorPin = A5;
const int servoValvePin  = 5;

// ============ STATE MACHINE ============
// The drone operates as a finite state machine. Each state defines
// what operations are valid, preventing conflicts like trying to
// move while already in a firefighting sequence.
enum FlightState {
  STATE_GROUNDED,       // motors off, safe on ground
  STATE_HOVERING,       // airborne, holding position
  STATE_MOVING,         // translating in a direction
  STATE_FIREFIGHTING,   // autonomous extinguish sequence
  STATE_LANDING,        // controlled descent in progress
  STATE_EMERGENCY       // immediate shutdown
};

FlightState currentState = STATE_GROUNDED;
const char* stateNames[] = {
  "GROUNDED", "HOVERING", "MOVING", "FIREFIGHTING", "LANDING", "EMERGENCY"
};

// ============ SPEED & RAMPING ============
// Instead of jumping to target speed instantly, motors ramp up
// gradually. This simulates realistic motor response and reduces
// mechanical stress.
const int SPEED_HOVER      = 170;
const int SPEED_FRONT_SLOW = 150;
const int SPEED_BACK_FAST  = 200;
const int SPEED_FRONT_FAST = 200;
const int SPEED_BACK_SLOW  = 150;
const int SPEED_TURN_FAST  = 200;
const int SPEED_TURN_SLOW  = 150;
const int SPEED_ASCEND     = 220;  // all motors above hover → climb
const int SPEED_DESCEND    = 120;  // all motors below hover → sink

// Current actual speed for each motor [LB, RB, LF, RF]
int currentSpeed[4] = {0, 0, 0, 0};
// Target speed each motor is ramping toward
int targetSpeed[4]  = {0, 0, 0, 0};
// Enable pin array matching the speed arrays
const int enablePins[4] = {9, 10, 3, 11};

const int RAMP_STEP       = 5;    // PWM units per ramp tick
const unsigned long RAMP_INTERVAL = 20; // ms between ramp steps
unsigned long lastRampTime = 0;

// ============ WATER SYSTEM ============
const int TOTAL_WATER     = 2000;
const int WATER_FLOW_RATE = 50;  // mL per second at full open
const int LOW_WATER_WARN  = 400; // warn below this level

int  currentWater     = TOTAL_WATER;
bool valveOpen        = false;
int  valveAngle       = 0;       // 0–90° for proportional flow
unsigned long lastWaterUpdate = 0;

// ============ FIRE DETECTION ============
const int TEMP_THRESHOLD  = 25;
const int LIGHT_THRESHOLD = 500;
unsigned long lastFireCheck    = 0;
unsigned long fireCheckInterval = 2000;
bool autoFirefight = true;  // autonomous mode enabled by default

// Firefighting sequence tracking
unsigned long firefightStart = 0;
const unsigned long FIREFIGHT_SPRAY_TIME = 5000; // spray for 5s

// ============ BATTERY SIMULATION ============
// Simulates battery drain based on motor load. Motors at full
// speed drain faster than at hover speed.
float batteryPercent     = 100.0;
const float DRAIN_IDLE   = 0.001;  // %/loop while grounded
const float DRAIN_HOVER  = 0.005;  // %/loop while hovering
const float DRAIN_MOVE   = 0.008;  // %/loop while moving
const float BATTERY_LOW  = 20.0;
const float BATTERY_CRIT = 5.0;
unsigned long lastBattWarn = 0;

// ============ FLIGHT TIMER ============
unsigned long flightStartTime = 0;  // when drone left ground
bool inFlight = false;

// ============ MANUAL SERVO ============
void servoCommand(int angle) {
  angle = constrain(angle, 0, 180);
  for (int i = 0; i < 25; i++) {
    int pulse = map(angle, 0, 180, 544, 2400);
    digitalWrite(servoValvePin, HIGH);
    delayMicroseconds(pulse);
    digitalWrite(servoValvePin, LOW);
    delay(20);
  }
}

// ============ MOTOR PRIMITIVES ============
void allOff() {
  for (int i = 0; i < 4; i++) {
    currentSpeed[i] = 0;
    targetSpeed[i]  = 0;
    analogWrite(enablePins[i], 0);
  }
  digitalWrite(leftBackDirA, LOW);   digitalWrite(leftBackDirB, LOW);
  digitalWrite(rightBackDirA, LOW);  digitalWrite(rightBackDirB, LOW);
  digitalWrite(leftFrontDirA, LOW);  digitalWrite(leftFrontDirB, LOW);
  digitalWrite(rightFrontDirA, LOW); digitalWrite(rightFrontDirB, LOW);
}

void setAllForward() {
  digitalWrite(leftBackDirA, HIGH);   digitalWrite(leftBackDirB, LOW);
  digitalWrite(rightBackDirA, HIGH);  digitalWrite(rightBackDirB, LOW);
  digitalWrite(leftFrontDirA, HIGH);  digitalWrite(leftFrontDirB, LOW);
  digitalWrite(rightFrontDirA, HIGH); digitalWrite(rightFrontDirB, LOW);
}

// Set target speeds — motors will ramp toward these values
void setTargetSpeeds(int lb, int rb, int lf, int rf) {
  setAllForward();
  targetSpeed[0] = lb;
  targetSpeed[1] = rb;
  targetSpeed[2] = lf;
  targetSpeed[3] = rf;
}

// ============ SPEED RAMPING ENGINE ============
// Called every loop iteration. Moves current speeds toward target
// speeds by RAMP_STEP per RAMP_INTERVAL, creating smooth accel.
void updateSpeedRamp() {
  if (millis() - lastRampTime < RAMP_INTERVAL) return;
  lastRampTime = millis();

  for (int i = 0; i < 4; i++) {
    if (currentSpeed[i] < targetSpeed[i]) {
      currentSpeed[i] = min(currentSpeed[i] + RAMP_STEP, targetSpeed[i]);
    } else if (currentSpeed[i] > targetSpeed[i]) {
      currentSpeed[i] = max(currentSpeed[i] - RAMP_STEP, targetSpeed[i]);
    }
    analogWrite(enablePins[i], currentSpeed[i]);
  }
}

// ============ FLIGHT COMMANDS ============
void beginFlight() {
  if (!inFlight) {
    inFlight = true;
    flightStartTime = millis();
  }
}

void moveForward() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_BACK_FAST, SPEED_BACK_FAST,
                  SPEED_FRONT_SLOW, SPEED_FRONT_SLOW);
  Serial.println("forward (ramping)");
}

void moveBackward() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_BACK_SLOW, SPEED_BACK_SLOW,
                  SPEED_FRONT_FAST, SPEED_FRONT_FAST);
  Serial.println("backward (ramping)");
}

void turnRight() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_TURN_FAST, SPEED_TURN_SLOW,
                  SPEED_TURN_FAST, SPEED_TURN_SLOW);
  Serial.println("right (ramping)");
}

void turnLeft() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_TURN_SLOW, SPEED_TURN_FAST,
                  SPEED_TURN_SLOW, SPEED_TURN_FAST);
  Serial.println("left (ramping)");
}

void hover() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_HOVERING;
  setTargetSpeeds(SPEED_HOVER, SPEED_HOVER, SPEED_HOVER, SPEED_HOVER);
  Serial.println("hover (ramping)");
}

void ascend() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  beginFlight();
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_ASCEND, SPEED_ASCEND, SPEED_ASCEND, SPEED_ASCEND);
  Serial.println("ascend (ramping)");
}

void descend() {
  if (currentState == STATE_FIREFIGHTING || currentState == STATE_LANDING) {
    Serial.println("ERR: busy"); return;
  }
  if (currentState == STATE_GROUNDED) {
    Serial.println("already grounded"); return;
  }
  currentState = STATE_MOVING;
  setTargetSpeeds(SPEED_DESCEND, SPEED_DESCEND, SPEED_DESCEND, SPEED_DESCEND);
  Serial.println("descend (ramping)");
}

// ============ LANDING SEQUENCE ============
// Gradually reduces thrust to zero over ~2 seconds, then grounds.
void initiateLanding() {
  if (currentState == STATE_GROUNDED) {
    Serial.println("already grounded"); return;
  }
  currentState = STATE_LANDING;
  Serial.println("LANDING...");
  // Ramp targets to zero — the ramp engine will gradually slow
  setTargetSpeeds(0, 0, 0, 0);
}

// Check if landing is complete (all motors reached zero)
void checkLandingComplete() {
  if (currentState != STATE_LANDING) return;
  for (int i = 0; i < 4; i++) {
    if (currentSpeed[i] > 0) return; // still spinning down
  }
  // All motors stopped
  allOff();
  currentState = STATE_GROUNDED;
  inFlight = false;
  Serial.println("LANDED");
}

// ============ EMERGENCY STOP ============
void emergencyStop() {
  currentState = STATE_EMERGENCY;
  allOff();
  if (valveOpen) { servoCommand(0); valveOpen = false; valveAngle = 0; }
  inFlight = false;
  Serial.println("!!! EMERGENCY STOP !!!");
  currentState = STATE_GROUNDED;
}

// ============ WATER / VALVE SYSTEM ============
void openValve() {
  if (currentWater <= 0) {
    Serial.println("ERR: tank empty"); return;
  }
  servoCommand(90);
  valveAngle = 90;
  valveOpen = true;
  lastWaterUpdate = millis();
  Serial.println("valve open (full flow)");
}

// Proportional valve: open to a specific angle (0–90)
void openValvePartial(int angle) {
  if (currentWater <= 0) {
    Serial.println("ERR: tank empty"); return;
  }
  angle = constrain(angle, 0, 90);
  servoCommand(angle);
  valveAngle = angle;
  valveOpen = (angle > 0);
  lastWaterUpdate = millis();
  Serial.print("valve at ");
  Serial.print(angle);
  Serial.println(" deg");
}

void closeValve() {
  servoCommand(0);
  valveAngle = 0;
  valveOpen = false;
  Serial.println("valve closed");
}

void handleWaterSystem() {
  if (!valveOpen || currentWater <= 0) return;

  unsigned long now = millis();
  if (now - lastWaterUpdate < 1000) return;

  // Flow is proportional to valve angle (90° = full rate)
  int flowThisTick = (int)((long)WATER_FLOW_RATE * valveAngle / 90);
  currentWater -= max(flowThisTick, 1);
  lastWaterUpdate = now;

  if (currentWater <= 0) {
    currentWater = 0;
    closeValve();
    Serial.println("TANK EMPTY — auto-close");
  } else {
    Serial.print("water:");
    Serial.print(currentWater);
    Serial.print("mL");
    if (currentWater <= LOW_WATER_WARN) Serial.print(" [LOW]");
    Serial.println();
  }
}

// ============ FIRE DETECTION & AUTO-RESPONSE ============
bool fireDetected() {
  int rawTemp = analogRead(tempSensorPin);
  int tempC   = map(rawTemp, 20, 358, -40, 125);
  int light   = analogRead(lightSensorPin);
  return (tempC >= TEMP_THRESHOLD && light >= LIGHT_THRESHOLD);
}

// Start autonomous firefighting: hover + spray for FIREFIGHT_SPRAY_TIME
void beginFirefightSequence() {
  if (currentState == STATE_FIREFIGHTING) return; // already fighting
  if (currentWater <= 0) {
    Serial.println("FIRE but tank empty!"); return;
  }

  Serial.println(">>> AUTO-FIREFIGHT: hovering + spraying");
  currentState = STATE_FIREFIGHTING;
  firefightStart = millis();

  // Hover in place
  beginFlight();
  setAllForward();
  setTargetSpeeds(SPEED_HOVER, SPEED_HOVER, SPEED_HOVER, SPEED_HOVER);

  // Open valve full
  openValve();
}

// Update firefighting sequence (called in loop)
void updateFirefightSequence() {
  if (currentState != STATE_FIREFIGHTING) return;

  // Check if spray time elapsed or tank empty
  bool timeUp  = (millis() - firefightStart >= FIREFIGHT_SPRAY_TIME);
  bool tankOut = (currentWater <= 0);
  bool fireGone = !fireDetected();

  if (timeUp || tankOut || fireGone) {
    closeValve();
    currentState = STATE_HOVERING;
    Serial.print(">>> AUTO-FIREFIGHT DONE (");
    if (fireGone)     Serial.print("fire out");
    else if (tankOut) Serial.print("tank empty");
    else              Serial.print("timeout");
    Serial.println(") → hover");
  }
}

void handleFireDetection() {
  if (millis() - lastFireCheck < fireCheckInterval) return;
  lastFireCheck = millis();

  if (fireDetected()) {
    Serial.println("FIRE DETECTED!");
    if (autoFirefight && currentState != STATE_FIREFIGHTING
        && currentState != STATE_LANDING && currentState != STATE_EMERGENCY) {
      beginFirefightSequence();
    }
  }
}

// ============ BATTERY SIMULATION ============
void updateBattery() {
  // Drain rate depends on current state
  float drain = DRAIN_IDLE;
  if (currentState == STATE_HOVERING || currentState == STATE_FIREFIGHTING) drain = DRAIN_HOVER;
  else if (currentState == STATE_MOVING) drain = DRAIN_MOVE;

  batteryPercent -= drain;
  if (batteryPercent < 0) batteryPercent = 0;

  // Periodic warnings
  if (batteryPercent <= BATTERY_CRIT && millis() - lastBattWarn > 5000) {
    Serial.println("!!! BATTERY CRITICAL — LAND NOW !!!");
    lastBattWarn = millis();
    if (currentState != STATE_GROUNDED && currentState != STATE_LANDING) {
      initiateLanding(); // force auto-land
    }
  } else if (batteryPercent <= BATTERY_LOW && millis() - lastBattWarn > 10000) {
    Serial.println("WARNING: battery low");
    lastBattWarn = millis();
  }
}

// ============ MOTOR SELF-TEST ============
// Spins each motor individually for 1 second at low speed, useful
// for verifying wiring without a full flight.
void motorTest() {
  const char* names[] = {"LB", "RB", "LF", "RF"};
  const int dirA[] = {leftBackDirA, rightBackDirA, leftFrontDirA, rightFrontDirA};
  const int dirB[] = {leftBackDirB, rightBackDirB, leftFrontDirB, rightFrontDirB};

  Serial.println("=== MOTOR TEST ===");
  for (int m = 0; m < 4; m++) {
    Serial.print("Testing "); Serial.print(names[m]); Serial.println("...");
    digitalWrite(dirA[m], HIGH);
    digitalWrite(dirB[m], LOW);
    analogWrite(enablePins[m], 120);
    delay(1000);
    analogWrite(enablePins[m], 0);
    digitalWrite(dirA[m], LOW);
    delay(500);
  }
  Serial.println("=== TEST COMPLETE ===");
}

// ============ STATUS ============
void printStatus() {
  Serial.println("--- DRONE STATUS ---");

  Serial.print("state:  "); Serial.println(stateNames[currentState]);

  Serial.print("battery:"); Serial.print((int)batteryPercent); Serial.println("%");

  Serial.print("water:  "); Serial.print(currentWater);
  Serial.print("/"); Serial.print(TOTAL_WATER);
  if (currentWater <= LOW_WATER_WARN) Serial.print(" [LOW]");
  Serial.println("mL");

  Serial.print("valve:  ");
  if (valveOpen) { Serial.print("open @ "); Serial.print(valveAngle); Serial.println("deg"); }
  else Serial.println("closed");

  int rawTemp = analogRead(tempSensorPin);
  int tempC   = map(rawTemp, 20, 358, -40, 125);
  Serial.print("temp:   "); Serial.print(tempC); Serial.println("C");

  int light = analogRead(lightSensorPin);
  Serial.print("light:  "); Serial.println(light);

  Serial.print("fire:   "); Serial.println(fireDetected() ? "YES" : "no");
  Serial.print("auto FF:"); Serial.println(autoFirefight ? "on" : "off");

  if (inFlight) {
    unsigned long flightSec = (millis() - flightStartTime) / 1000;
    Serial.print("uptime: "); Serial.print(flightSec); Serial.println("s");
  }

  // Show current motor speeds
  Serial.print("motors: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(currentSpeed[i]);
    if (i < 3) Serial.print("/");
  }
  Serial.print(" → ");
  for (int i = 0; i < 4; i++) {
    Serial.print(targetSpeed[i]);
    if (i < 3) Serial.print("/");
  }
  Serial.println();
  Serial.println("--------------------");
}

void printMenu() {
  Serial.println("=== COMMANDS ===");
  Serial.println("f:forward  b:back  l:left  r:right");
  Serial.println("u:up  d:down  h:hover  g:land  x:stop  e:E-STOP");
  Serial.println("o:valve open  c:valve close  p:partial (0-90)");
  Serial.println("t:motor test  a:toggle auto-firefight");
  Serial.println("s:status  m:menu");
}

// ============ SETUP ============
void setup() {
  Serial.begin(9600);

  pinMode(servoValvePin, OUTPUT);
  servoCommand(0);

  pinMode(leftBackEnable, OUTPUT);
  pinMode(rightBackEnable, OUTPUT);
  pinMode(leftFrontEnable, OUTPUT);
  pinMode(rightFrontEnable, OUTPUT);

  pinMode(leftBackDirA, OUTPUT);  pinMode(leftBackDirB, OUTPUT);
  pinMode(rightBackDirA, OUTPUT); pinMode(rightBackDirB, OUTPUT);
  pinMode(leftFrontDirA, OUTPUT); pinMode(leftFrontDirB, OUTPUT);
  pinMode(rightFrontDirA, OUTPUT);pinMode(rightFrontDirB, OUTPUT);

  pinMode(tempSensorPin, INPUT);
  pinMode(lightSensorPin, INPUT);

  allOff();
  Serial.println("Firefighting Drone v3 — Advanced");
  printMenu();
}

// ============ MAIN LOOP ============
void loop() {
  // Core systems update every iteration
  updateSpeedRamp();
  handleWaterSystem();
  handleFireDetection();
  updateFirefightSequence();
  checkLandingComplete();
  updateBattery();

  // Serial command processing
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) Serial.read();

    switch (c) {
      case 'f': moveForward();  break;
      case 'b': moveBackward(); break;
      case 'l': turnLeft();     break;
      case 'r': turnRight();    break;
      case 'h': hover();        break;
      case 'u': ascend();       break;
      case 'd': descend();      break;
      case 'g': initiateLanding(); break;
      case 'x': allOff(); currentState = STATE_GROUNDED; inFlight = false;
                Serial.println("stop"); break;
      case 'e': emergencyStop(); break;
      case 'o': openValve();    break;
      case 'c': closeValve();   break;
      case 'p': // proportional valve — wait for angle digits
        Serial.println("enter angle 0-90:");
        delay(2000); // wait for input
        if (Serial.available()) {
          int angle = Serial.parseInt();
          while (Serial.available()) Serial.read();
          openValvePartial(angle);
        }
        break;
      case 't': motorTest();    break;
      case 'a': autoFirefight = !autoFirefight;
                Serial.print("auto-firefight: ");
                Serial.println(autoFirefight ? "ON" : "OFF"); break;
      case 's': printStatus();  break;
      case 'm': printMenu();    break;
      default:  Serial.println("unk"); break;
    }
  }

  delay(10);
}
