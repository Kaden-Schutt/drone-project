// ================================================================
//  FIREFIGHTING DRONE — Readable Edition
//  FSE 100 Mini Project
//
//  Purpose:  Serial-controlled quadcopter simulation with fire
//            detection and water-valve extinguishing system.
//
//  Hardware: Arduino Uno (ATmega328P), 2× L293D motor drivers,
//            4× DC motors, 1× servo, TMP36, photoresistor.
//
//  Pinout has been rewired from the default to avoid Timer0
//  conflicts. Timer0 drives millis()/delay()/Serial, so motor
//  PWM was moved to Timer1 and Timer2 pins instead.
//
//    Timer0 (D5/D6)  → NOT used for PWM (keeps timing safe)
//    Timer1 (D9/D10) → back motor enables
//    Timer2 (D3/D11) → front motor enables
//    D5              → servo (manual pulse, no Servo.h)
// ================================================================


// ================================================================
//  SECTION 1: PIN DEFINITIONS
//  All pin constants are grouped by function for easy reference.
//  "Enable" pins carry PWM for speed control.
//  "Dir" pins set motor rotation direction via the L293D H-bridge.
// ================================================================

// --- Back motors (controlled by rear L293D, using Timer1) ---
const int leftBackEnable   = 9;   // PWM speed for left-back motor
const int rightBackEnable  = 10;  // PWM speed for right-back motor

// --- Front motors (controlled by front L293D, using Timer2) ---
const int leftFrontEnable  = 3;   // PWM speed for left-front motor
const int rightFrontEnable = 11;  // PWM speed for right-front motor

// --- Direction pins for left-back motor ---
// NOTE: DirA and DirB are physically swapped in code because
// this motor is mounted in reverse polarity on the frame.
const int leftBackDirA  = 13;     // swapped (polarity fix)
const int leftBackDirB  = 12;     // swapped (polarity fix)

// --- Direction pins for right-back motor ---
const int rightBackDirA = 7;
const int rightBackDirB = 2;

// --- Direction pins for left-front motor ---
const int leftFrontDirA = A1;     // analog pins used as digital
const int leftFrontDirB = A0;

// --- Direction pins for right-front motor ---
const int rightFrontDirA = 4;     // was D11, moved to free PWM pin
const int rightFrontDirB = 8;

// --- Sensors ---
const int tempSensorPin  = A4;    // TMP36 temperature sensor
const int lightSensorPin = A5;    // photoresistor with 10k divider

// --- Servo ---
const int servoValvePin  = 5;     // manual pulse (avoids Servo.h
                                   // which would hijack a timer)


// ================================================================
//  SECTION 2: FLIGHT SPEED CONSTANTS
//  Quadcopter movement works by differential thrust:
//    - Forward:  back motors spin FASTER than front → nose dips
//    - Backward: front motors spin FASTER than back → tail dips
//    - Turn:     one side spins FASTER than the other → yaw
//    - Hover:    all four motors at equal, moderate speed
// ================================================================

const int SPEED_HOVER      = 170; // balanced lift, no movement
const int SPEED_FRONT_SLOW = 150; // front motors during forward
const int SPEED_BACK_FAST  = 200; // back motors during forward
const int SPEED_FRONT_FAST = 200; // front motors during backward
const int SPEED_BACK_SLOW  = 150; // back motors during backward
const int SPEED_TURN_FAST  = 200; // faster side during a turn
const int SPEED_TURN_SLOW  = 150; // slower side during a turn
const int SPEED_ASCEND     = 220; // all motors above hover → climb
const int SPEED_DESCEND    = 120; // all motors below hover → sink


// ================================================================
//  SECTION 3: WATER SYSTEM CONFIGURATION
//  The drone carries a simulated water tank. When the valve is
//  opened (servo → 90°), water drains at a fixed rate. When the
//  tank is empty, the valve auto-closes to prevent dry running.
// ================================================================

const int TOTAL_WATER_ML    = 2000; // full tank capacity in mL
const int WATER_FLOW_RATE   = 50;   // drain rate: mL per second
const int LOW_WATER_LEVEL   = 400;  // warn operator below this

int       currentWater      = TOTAL_WATER_ML;  // current level
bool      lowWaterWarned    = false;            // one-shot warning flag
bool      valveOpen         = false;            // valve state
unsigned long lastWaterUpdate = 0;  // timestamp of last drain tick


// ================================================================
//  SECTION 4: FIRE DETECTION CONFIGURATION
//  Fire is detected when BOTH conditions are met simultaneously:
//    1. Temperature exceeds threshold (heat source nearby)
//    2. Light level exceeds threshold (flame brightness)
//  This dual-sensor approach reduces false positives.
// ================================================================

const int TEMP_THRESHOLD_C     = 25;    // degrees Celsius
const int LIGHT_THRESHOLD      = 500;   // raw ADC value (0–1023)
const unsigned long FIRE_CHECK_INTERVAL = 2000; // check every 2s

unsigned long lastFireCheck = 0;


// ================================================================
//  SECTION 5: SERVO CONTROL
//  We avoid Servo.h because it takes over Timer1, which we need
//  for motor PWM. Instead, we generate the servo signal manually
//  using timed digital pulses:
//    0° → 544μs pulse
//    180° → 2400μs pulse
//  The pulse repeats every 20ms (50Hz), and we send 25 pulses
//  (500ms total) to give the servo time to reach its position.
// ================================================================

void servoCommand(int angle) {
  // Clamp angle to the valid servo range
  angle = constrain(angle, 0, 180);

  // Send 25 pulses to allow the servo to physically move
  for (int i = 0; i < 25; i++) {
    // Convert angle to pulse width in microseconds
    int pulseWidth = map(angle, 0, 180, 544, 2400);

    // Generate one pulse
    digitalWrite(servoValvePin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(servoValvePin, LOW);

    // Wait for the remainder of the 20ms period
    delay(20);
  }
  // After pulsing stops, servo holds position mechanically
}


// ================================================================
//  SECTION 6: MOTOR CONTROL HELPERS
//  These functions provide building blocks for flight commands.
//  Motor direction is set via the L293D truth table:
//    DirA=HIGH, DirB=LOW  → forward spin
//    DirA=LOW,  DirB=HIGH → reverse spin
//    Both LOW             → motor stopped (coast)
// ================================================================

/**
 * Stops all four motors immediately.
 * Sets all enable pins to 0 (no PWM) and all direction pins LOW.
 */
void allOff() {
  // Disable PWM on all enable pins
  analogWrite(leftBackEnable,   0);
  analogWrite(rightBackEnable,  0);
  analogWrite(leftFrontEnable,  0);
  analogWrite(rightFrontEnable, 0);

  // Clear all direction pins (motor coast/stop)
  digitalWrite(leftBackDirA,   LOW);
  digitalWrite(leftBackDirB,   LOW);
  digitalWrite(rightBackDirA,  LOW);
  digitalWrite(rightBackDirB,  LOW);
  digitalWrite(leftFrontDirA,  LOW);
  digitalWrite(leftFrontDirB,  LOW);
  digitalWrite(rightFrontDirA, LOW);
  digitalWrite(rightFrontDirB, LOW);
}

/**
 * Sets all four motors to spin in the "forward" direction.
 * DirA = HIGH, DirB = LOW for each motor.
 * Note: left-back DirA/B are already swapped in the pin
 * definitions to account for its reversed physical mounting.
 */
void setAllForward() {
  digitalWrite(leftBackDirA,   HIGH);
  digitalWrite(leftBackDirB,   LOW);
  digitalWrite(rightBackDirA,  HIGH);
  digitalWrite(rightBackDirB,  LOW);
  digitalWrite(leftFrontDirA,  HIGH);
  digitalWrite(leftFrontDirB,  LOW);
  digitalWrite(rightFrontDirA, HIGH);
  digitalWrite(rightFrontDirB, LOW);
}


// ================================================================
//  SECTION 7: FLIGHT COMMANDS
//  Each command follows the pattern:
//    1. allOff()        — stop everything cleanly
//    2. setAllForward() — set rotation direction
//    3. analogWrite()   — apply differential speeds
//  The speed difference between motor pairs creates movement.
// ================================================================

/**
 * Fly forward: back motors faster than front.
 * This creates a nose-down pitch, pushing the drone forward.
 */
void moveForward() {
  allOff();
  setAllForward();
  analogWrite(leftFrontEnable,  SPEED_FRONT_SLOW);  // 150
  analogWrite(rightFrontEnable, SPEED_FRONT_SLOW);   // 150
  analogWrite(leftBackEnable,   SPEED_BACK_FAST);    // 200
  analogWrite(rightBackEnable,  SPEED_BACK_FAST);    // 200
  Serial.println("forward");
}

/**
 * Fly backward: front motors faster than back.
 * This creates a tail-down pitch, pushing the drone backward.
 */
void moveBackward() {
  allOff();
  setAllForward();
  analogWrite(leftFrontEnable,  SPEED_FRONT_FAST);   // 200
  analogWrite(rightFrontEnable, SPEED_FRONT_FAST);    // 200
  analogWrite(leftBackEnable,   SPEED_BACK_SLOW);     // 150
  analogWrite(rightBackEnable,  SPEED_BACK_SLOW);     // 150
  Serial.println("backward");
}

/**
 * Turn right (yaw): left-side motors faster than right-side.
 * The torque difference causes clockwise rotation when viewed
 * from above.
 */
void turnRight() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable,   SPEED_TURN_FAST);    // 200
  analogWrite(leftFrontEnable,  SPEED_TURN_FAST);    // 200
  analogWrite(rightBackEnable,  SPEED_TURN_SLOW);    // 150
  analogWrite(rightFrontEnable, SPEED_TURN_SLOW);    // 150
  Serial.println("right");
}

/**
 * Turn left (yaw): right-side motors faster than left-side.
 * The torque difference causes counter-clockwise rotation.
 */
void turnLeft() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable,   SPEED_TURN_SLOW);    // 150
  analogWrite(leftFrontEnable,  SPEED_TURN_SLOW);    // 150
  analogWrite(rightBackEnable,  SPEED_TURN_FAST);    // 200
  analogWrite(rightFrontEnable, SPEED_TURN_FAST);    // 200
  Serial.println("left");
}

/**
 * Hover in place: all motors at equal, moderate speed.
 * Does NOT call allOff() first — simply overwrites speeds
 * for a smoother transition from movement to hover.
 */
void hover() {
  setAllForward();
  analogWrite(leftBackEnable,   SPEED_HOVER);         // 170
  analogWrite(rightBackEnable,  SPEED_HOVER);          // 170
  analogWrite(leftFrontEnable,  SPEED_HOVER);          // 170
  analogWrite(rightFrontEnable, SPEED_HOVER);          // 170
  Serial.println("hover");
}

/**
 * Ascend: all motors above hover speed.
 * Equal thrust on all four, but higher than hover → net upward force.
 */
void ascend() {
  setAllForward();
  analogWrite(leftBackEnable,   SPEED_ASCEND);        // 220
  analogWrite(rightBackEnable,  SPEED_ASCEND);         // 220
  analogWrite(leftFrontEnable,  SPEED_ASCEND);         // 220
  analogWrite(rightFrontEnable, SPEED_ASCEND);         // 220
  Serial.println("ascend");
}

/**
 * Descend: all motors below hover speed.
 * Equal thrust on all four, but lower than hover → controlled sink.
 */
void descend() {
  setAllForward();
  analogWrite(leftBackEnable,   SPEED_DESCEND);       // 120
  analogWrite(rightBackEnable,  SPEED_DESCEND);        // 120
  analogWrite(leftFrontEnable,  SPEED_DESCEND);        // 120
  analogWrite(rightFrontEnable, SPEED_DESCEND);        // 120
  Serial.println("descend");
}


// ================================================================
//  SECTION 8: WATER SYSTEM MANAGEMENT
//  Called every loop iteration. Simulates water flow by
//  subtracting from the tank level once per second while the
//  valve is open. Auto-closes when the tank runs dry.
// ================================================================

void handleWaterSystem() {
  // Only drain if valve is open and water remains
  if (!valveOpen || currentWater <= 0) {
    return;
  }

  unsigned long now = millis();

  // Check if one second has elapsed since last drain
  if (now - lastWaterUpdate < 1000) {
    return;
  }

  // Subtract one second's worth of water flow
  currentWater -= WATER_FLOW_RATE;
  lastWaterUpdate = now;

  // Check if tank is now empty
  if (currentWater <= 0) {
    currentWater = 0;
    closeValve();
    Serial.println("empty auto-close");
  } else {
    // Report remaining water level
    Serial.print("water:");
    Serial.print(currentWater);

    // One-time low water warning when level drops below threshold
    if (currentWater <= LOW_WATER_LEVEL && !lowWaterWarned) {
      Serial.print(" WARNING: LOW WATER");
      lowWaterWarned = true;
    }
    Serial.println();
  }
}


// ================================================================
//  SECTION 9: VALVE CONTROL
//  The servo physically opens and closes a water valve.
//  0° = closed, 90° = open.
// ================================================================

void openValve() {
  servoCommand(90);             // rotate servo to open position
  valveOpen = true;
  lastWaterUpdate = millis();   // reset drain timer
  Serial.println("valve open");
}

void closeValve() {
  servoCommand(0);              // rotate servo to closed position
  valveOpen = false;
  Serial.println("valve closed");
}


// ================================================================
//  SECTION 10: FIRE DETECTION
//  Uses two sensors to detect fire with reduced false positives:
//    - TMP36 measures temperature (mapped from ADC to °C)
//    - Photoresistor measures light intensity
//  Both must exceed their thresholds simultaneously.
// ================================================================

bool fireDetected() {
  // Read and convert temperature
  int rawTemp     = analogRead(tempSensorPin);
  int tempCelsius = map(rawTemp, 20, 358, -40, 125);

  // Read light level (raw ADC 0–1023)
  int lightLevel  = analogRead(lightSensorPin);

  // Fire requires BOTH heat and brightness
  return (tempCelsius >= TEMP_THRESHOLD_C) &&
         (lightLevel  >= LIGHT_THRESHOLD);
}


// ================================================================
//  SECTION 11: STATUS REPORTING
//  Prints a human-readable summary of all drone systems to the
//  Serial Monitor for debugging and monitoring.
// ================================================================

void printStatus() {
  // Water level
  Serial.print("water:");
  Serial.print(currentWater);
  Serial.print("/");
  Serial.println(TOTAL_WATER_ML);

  // Temperature reading
  int rawTemp     = analogRead(tempSensorPin);
  int tempCelsius = map(rawTemp, 20, 358, -40, 125);
  Serial.print("temp:");
  Serial.println(tempCelsius);

  // Light reading
  int lightLevel = analogRead(lightSensorPin);
  Serial.print("light:");
  Serial.println(lightLevel);

  // Valve state
  Serial.print("valve:");
  Serial.println(valveOpen ? "open" : "closed");

  // Fire detection result
  Serial.print("fire:");
  Serial.println(fireDetected() ? "yes" : "no");
}


// ================================================================
//  SECTION 12: MENU
// ================================================================

void printMenu() {
  Serial.println("m:menu f:forward b:back l:left r:right u:up d:down h:hover x:stop o:open c:close s:status");
}


// ================================================================
//  SECTION 13: SETUP
//  Runs once at power-on / reset. Configures all pins, closes
//  the valve, stops all motors, and prints the command menu.
// ================================================================

void setup() {
  // Initialize serial communication at 9600 baud
  Serial.begin(9600);

  // Initialize servo pin and move to closed position
  pinMode(servoValvePin, OUTPUT);
  servoCommand(0);

  // Configure motor enable pins (PWM output)
  pinMode(leftBackEnable,   OUTPUT);
  pinMode(rightBackEnable,  OUTPUT);
  pinMode(leftFrontEnable,  OUTPUT);
  pinMode(rightFrontEnable, OUTPUT);

  // Configure motor direction pins (digital output)
  pinMode(leftBackDirA,   OUTPUT);
  pinMode(leftBackDirB,   OUTPUT);
  pinMode(rightBackDirA,  OUTPUT);
  pinMode(rightBackDirB,  OUTPUT);
  pinMode(leftFrontDirA,  OUTPUT);
  pinMode(leftFrontDirB,  OUTPUT);
  pinMode(rightFrontDirA, OUTPUT);
  pinMode(rightFrontDirB, OUTPUT);

  // Configure sensor pins (analog input)
  pinMode(tempSensorPin,  INPUT);
  pinMode(lightSensorPin, INPUT);

  // Start with everything safely off
  allOff();
  printMenu();
}


// ================================================================
//  SECTION 14: MAIN LOOP
//  Runs repeatedly after setup(). Three responsibilities:
//    1. Manage water drain simulation
//    2. Periodically check for fire
//    3. Process incoming serial commands
// ================================================================

void loop() {
  // --- Water system update ---
  handleWaterSystem();

  // --- Periodic fire detection ---
  if (millis() - lastFireCheck >= FIRE_CHECK_INTERVAL) {
    if (fireDetected()) {
      Serial.println("FIRE DETECTED");
    }
    lastFireCheck = millis();
  }

  // --- Serial command processing ---
  if (Serial.available()) {
    // Read the first character as the command
    char command = Serial.read();

    // Flush any remaining characters (we only use the first)
    while (Serial.available()) {
      Serial.read();
    }

    // Execute the corresponding action
    switch (command) {
      case 'f': moveForward();  break;
      case 'b': moveBackward(); break;
      case 'l': turnLeft();     break;
      case 'r': turnRight();    break;
      case 'h': hover();        break;
      case 'u': ascend();       break;
      case 'd': descend();      break;
      case 'x': allOff();       break;
      case 'o': openValve();    break;
      case 'c': closeValve();   break;
      case 's': printStatus();  break;
      case 'm': printMenu();    break;
      default:
        Serial.println("unk");
        break;
    }
  }

  // Small delay to prevent serial buffer overrun
  delay(10);
}
