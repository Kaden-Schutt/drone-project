// Firefighting Drone — Serial Control
// FSE 100 Mini Project
//
// PIN FIX: Enables moved off Timer0 (D5/D6) onto Timer1/Timer2
//          Servo moved to D5 with manual pulse (no Servo.h, no timer hijack)
//          MRF direction moved from D11 to D4 (frees D11 for PWM)
//
// Timer0 (D5/D6)  — untouched, millis()/delay()/Serial safe
// Timer1 (D9/D10) — EN_MLB, EN_MRB (analogWrite OK)
// Timer2 (D3/D11) — EN_MLF, EN_MRF (analogWrite OK)

// ========= Kaden - PIN ASSIGNMENTS (REWIRED) ============
const int leftBackEnable = 9;       // was D5 → now Timer1
const int rightBackEnable = 10;     // was D6 → now Timer1
const int leftFrontEnable = 3;      // was D9 → now Timer2
const int rightFrontEnable = 11;    // was D10 → now Timer2

const int leftBackDirA = 13;        // swapped (MLB polarity fix)
const int leftBackDirB = 12;        // swapped
const int rightBackDirA = 7;
const int rightBackDirB = 2;
const int leftFrontDirA = A1;
const int leftFrontDirB = A0;
const int rightFrontDirA = 4;       // was D11 → freed for PWM
const int rightFrontDirB = 8;

const int tempSensorPin = A4;
const int lightSensorPin = A5;

const int servoValvePin = 5;        // was D3 → manual pulse, no timer

// ========= Kaden - MANUAL SERVO (replaces Servo.h from Mohamed) ==========
void servoCommand(int angle) {
  angle = constrain(angle, 0, 180);
  for (int i = 0; i < 25; i++) {  // 25 × 20ms = 500ms to reach position
    int pulse = map(angle, 0, 180, 544, 2400);
    digitalWrite(servoValvePin, HIGH);
    delayMicroseconds(pulse);
    digitalWrite(servoValvePin, LOW);
    delay(20);
  }
  //  servo holds mechanically, no PWM blocking
}

// ========== Mohamed - WATER SYSTEM ============
const int totalWater = 2000;
const int waterFlowRate = 50;

int currentWater = totalWater;
bool valveOpen = false;
unsigned long lastWaterUpdate = 0;
unsigned long valveOpenTime = 0;

// ========== Team Effort FIRE DETECTION ============
const int tempThreshold = 80; //celcius
const int lightThreshold = 500;
const int waterLowThreshold = totalWater * 0.05; // 5% = 100ml

unsigned long lastFireCheck = 0;
unsigned long fireCheckInterval = 2000;

// ============ SPEED CONSTANTS ============
const int SPEED_HOVER = 170;
const int SPEED_FRONT_SLOW = 150;
const int SPEED_BACK_FAST = 200;
const int SPEED_FRONT_FAST = 200;
const int SPEED_BACK_SLOW = 150;
const int SPEED_TURN_FAST = 200;
const int SPEED_TURN_SLOW = 150;
const int  SPEED_UP = 205;
const int  SPEED_DOWN = 135;
bool fireLocked = false;
bool lowWaterWarned = false;
bool alertAcknowledged = false;  // once operator responds, stay silent
                                 // until fire condition clears & returns

// ============ DASHBOARD STATE ============
const char* droneMode = "IDLE";
const char* statusMsg = "System ready";
bool needsRefresh = true;

// ============ Kaden - DASHBOARD HUD ============
// 6-line event-driven HUD — only redraws when state changes.
//
// === DRONE HUD ===            !! FIRE DETECTED !!
// MODE:HOVER  FIRE:no          MODE:HOVER  FIRE:YES
// WATER[########--]1600/2000   WATER[########--]1600/2000
// VALVE:closed                 VALVE:closed
// > System ready               > Press 'o' to spray or 'c' to dismiss
// press m for commands          >> RESPOND TO ALERT <<

void printDashboard() {
  // Line 1: header or alert banner
  if (fireLocked) {
    if (currentWater <= waterLowThreshold && currentWater > 0) {
      Serial.println("!! LOW WATER WARNING !!");
    } else if (currentWater <= 0) {
      Serial.println("!! TANK EMPTY !!");
    } else {
      Serial.println("!! FIRE DETECTED !!");
    }
  } else {
    Serial.println("=== DRONE HUD ===");
  }

  // Line 2: flight mode + fire status
  bool fire = fireDetected();
  Serial.print("MODE:");
  Serial.print(droneMode);
  Serial.print("  FIRE:");
  Serial.println(fire ? "YES" : "no");

  // Line 3: water level with ASCII bar graph
  int filled = map(currentWater, 0, totalWater, 0, 10);
  filled = constrain(filled, 0, 10);
  Serial.print("WATER[");
  for (int i = 0; i < 10; i++) {
    Serial.print(i < filled ? "#" : "-");
  }
  Serial.print("]");
  Serial.print(currentWater);
  Serial.print("/");
  Serial.println(totalWater);

  // Line 4: valve state + water warnings
  Serial.print("VALVE:");
  Serial.print(valveOpen ? "OPEN" : "closed");
  if (currentWater <= 0)                         Serial.print(" EMPTY");
  else if (currentWater <= waterLowThreshold)     Serial.print(" LOW!");
  Serial.println();

  // Line 5: contextual status message
  Serial.print("> ");
  Serial.println(statusMsg);

  // Line 6: command hint or alert prompt
  if (fireLocked) {
    Serial.println(">> RESPOND TO ALERT <<");
  } else {
    Serial.println("press m for commands");
  }
}

// Full command list — only shown on 'm' request
void printCommands() {
  Serial.println("=== COMMANDS ===");
  Serial.println("f:fwd b:back l:left r:right");
  Serial.println("u:up d:down h:hover x:stop");
  Serial.println("o:open valve c:close valve");
  Serial.println("s:refresh  m:this menu");
}

// ============ Raghav - SETUP ============
void setup() {
  Serial.begin(9600);

  pinMode(servoValvePin, OUTPUT);
  servoCommand(0);

  pinMode(leftBackEnable, OUTPUT);
  pinMode(rightBackEnable, OUTPUT);
  pinMode(leftFrontEnable, OUTPUT);
  pinMode(rightFrontEnable, OUTPUT);

  pinMode(leftBackDirA, OUTPUT);
  pinMode(leftBackDirB, OUTPUT);
  pinMode(rightBackDirA, OUTPUT);
  pinMode(rightBackDirB, OUTPUT);
  pinMode(leftFrontDirA, OUTPUT);
  pinMode(leftFrontDirB, OUTPUT);
  pinMode(rightFrontDirA, OUTPUT);
  pinMode(rightFrontDirB, OUTPUT);

  pinMode(tempSensorPin, INPUT);
  pinMode(lightSensorPin, INPUT);

  allOff();
  printDashboard();
}

// ============ Mohamed - MAIN LOOP ============
void loop() {
  handleWaterSystem();

  // Fire detection — runs every 2s
  if (millis() - lastFireCheck >= fireCheckInterval) {
    lastFireCheck = millis();

    if (fireDetected()) {
      // Only alert if not already locked AND not previously acknowledged
      if (!fireLocked && !alertAcknowledged) {
        fireLocked = true;
        statusMsg = "Press 'o' to spray or 'c' to dismiss";
        needsRefresh = true;
      }
    } else {
      // Fire condition cleared — reset so next fire triggers a new alert
      alertAcknowledged = false;
    }
  }

  // Command processing
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read();
    }

    // Any input while alert active = operator acknowledged, silence it
    if (fireLocked) {
      fireLocked = false;
      alertAcknowledged = true;
    }

    switch (c) {
      case 'f': moveForward();  needsRefresh = true; break;
      case 'b': moveBackward(); needsRefresh = true; break;
      case 'l': turnLeft();     needsRefresh = true; break;
      case 'r': turnRight();    needsRefresh = true; break;
      case 'h': hover();        needsRefresh = true; break;
      case 'x': allOff(); droneMode = "STOPPED"; statusMsg = "Motors off";
                needsRefresh = true; break;
      case 'o': openValve();    needsRefresh = true; break;
      case 'c': closeValve();   needsRefresh = true; break;
      case 's': needsRefresh = true; break;  // manual refresh
      case 'm': printCommands(); break;       // no HUD refresh
      case 'u': moveUp();       needsRefresh = true; break;
      case 'd': moveDown();     needsRefresh = true; break;
      default:  statusMsg = "unknown command";
                needsRefresh = true; break;
    }
  }

  // Only redraw when something actually changed
  if (needsRefresh) {
    printDashboard();
    needsRefresh = false;
  }

  delay(10);
}

// ============ Raghav - MOTOR CONTROL ============
void allOff() {
  analogWrite(leftBackEnable, 0);
  analogWrite(rightBackEnable, 0);
  analogWrite(leftFrontEnable, 0);
  analogWrite(rightFrontEnable, 0);

  digitalWrite(leftBackDirA, LOW);
  digitalWrite(leftBackDirB, LOW);
  digitalWrite(rightBackDirA, LOW);
  digitalWrite(rightBackDirB, LOW);
  digitalWrite(leftFrontDirA, LOW);
  digitalWrite(leftFrontDirB, LOW);
  digitalWrite(rightFrontDirA, LOW);
  digitalWrite(rightFrontDirB, LOW);
}

void setAllForward() {
  digitalWrite(leftBackDirA, HIGH);
  digitalWrite(leftBackDirB, LOW);
  digitalWrite(rightBackDirA, HIGH);
  digitalWrite(rightBackDirB, LOW);
  digitalWrite(leftFrontDirA, HIGH);
  digitalWrite(leftFrontDirB, LOW);
  digitalWrite(rightFrontDirA, HIGH);
  digitalWrite(rightFrontDirB, LOW);
}

void moveForward() {
  allOff();
  setAllForward();
  analogWrite(leftFrontEnable, SPEED_FRONT_SLOW);
  analogWrite(rightFrontEnable, SPEED_FRONT_SLOW);
  analogWrite(leftBackEnable, SPEED_BACK_FAST);
  analogWrite(rightBackEnable, SPEED_BACK_FAST);
  droneMode = "FORWARD";
  statusMsg = "moving forward";
}

void moveBackward() {
  allOff();
  setAllForward();
  analogWrite(leftFrontEnable, SPEED_FRONT_FAST);
  analogWrite(rightFrontEnable, SPEED_FRONT_FAST);
  analogWrite(leftBackEnable, SPEED_BACK_SLOW);
  analogWrite(rightBackEnable, SPEED_BACK_SLOW);
  droneMode = "BACKWARD";
  statusMsg = "moving backward";
}

void turnRight() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable, SPEED_TURN_FAST);
  analogWrite(leftFrontEnable, SPEED_TURN_FAST);
  analogWrite(rightBackEnable, SPEED_TURN_SLOW);
  analogWrite(rightFrontEnable, SPEED_TURN_SLOW);
  droneMode = "RIGHT";
  statusMsg = "turning right";
}

void turnLeft() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable, SPEED_TURN_SLOW);
  analogWrite(leftFrontEnable, SPEED_TURN_SLOW);
  analogWrite(rightBackEnable, SPEED_TURN_FAST);
  analogWrite(rightFrontEnable, SPEED_TURN_FAST);
  droneMode = "LEFT";
  statusMsg = "turning left";
}

void hover() {
  setAllForward();
  analogWrite(leftBackEnable, SPEED_HOVER);
  analogWrite(rightBackEnable, SPEED_HOVER);
  analogWrite(leftFrontEnable, SPEED_HOVER);
  analogWrite(rightFrontEnable, SPEED_HOVER);
  droneMode = "HOVER";
  statusMsg = "hovering";
}

void handleWaterSystem() {
  if (valveOpen && currentWater > 0) {
    unsigned long n = millis();
    if (n - lastWaterUpdate >= 1000) {
      currentWater -= waterFlowRate;
      lastWaterUpdate = n;

      if (currentWater <= 0) {
        currentWater = 0;
        //Mohamed - Close valve directly — let easter egg stay on screen
        servoCommand(0);
        valveOpen = false;
        fireLocked = false;
        lowWaterWarned = false;
        droneMode = "IDLE";
        Serial.println("empty auto-close, \nGoing back to the base \nOperation completed successfully \nsee you next time captain!!");
        statusMsg = "Tank empty - RTB";
        // Don't set needsRefresh — easter egg stays visible
      } else {
        statusMsg = "spraying...";
        needsRefresh = true;
        if (currentWater <= waterLowThreshold && !lowWaterWarned) {
          statusMsg = "Press 'c' to close or continue spraying";
          fireLocked = true;
          lowWaterWarned = true;
        }

      }
    }
  }


  }


// ===================== Jeremy - UP/DOWN FUNCTIONS =====================
void moveUp() {
  allOff();
  setAllForward();

  analogWrite(leftBackEnable, SPEED_UP);
  analogWrite(rightBackEnable, SPEED_UP);
  analogWrite(leftFrontEnable, SPEED_UP);
  analogWrite(rightFrontEnable, SPEED_UP);

  droneMode = "ASCEND";
  statusMsg = "ascending";
}

void moveDown() {
  allOff();
  setAllForward();

  analogWrite(leftBackEnable, SPEED_DOWN);
  analogWrite(rightBackEnable, SPEED_DOWN);
  analogWrite(leftFrontEnable, SPEED_DOWN);
  analogWrite(rightFrontEnable, SPEED_DOWN);

  droneMode = "DESCEND";
  statusMsg = "descending";
}
// ================================================================


// ============ Mohamed's functions ============
void openValve() {
  if (currentWater <= 0) {
    droneMode = "IDLE";
    statusMsg = "Cannot activate - tank empty";
    fireLocked = true;
    return;
  }
  servoCommand(90);
  valveOpen = true;
  fireLocked = false;
  lastWaterUpdate = millis();
  droneMode = "SPRAYING";
  statusMsg = "!! WATER JET ACTIVATED !!";
}

void closeValve() {
  servoCommand(0);
  valveOpen = false;
  fireLocked = false;
  lowWaterWarned = false;
  droneMode = "IDLE";
  statusMsg = "valve closed";
}

bool fireDetected() {
  int rawTemp = analogRead(tempSensorPin);
  int tempC = map(rawTemp, 20, 358, -40, 125);
  int lightReading = analogRead(lightSensorPin);
  return (tempC >= tempThreshold || lightReading >= lightThreshold);
}
