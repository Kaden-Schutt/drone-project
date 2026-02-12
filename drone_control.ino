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

// ============ PIN ASSIGNMENTS (REWIRED) ============
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

// ============ MANUAL SERVO (replaces Servo.h) ============
void servoCommand(int angle) {
  angle = constrain(angle, 0, 180);
  for (int i = 0; i < 25; i++) {  // 25 × 20ms = 500ms to reach position
    int pulse = map(angle, 0, 180, 544, 2400);
    digitalWrite(servoValvePin, HIGH);
    delayMicroseconds(pulse);
    digitalWrite(servoValvePin, LOW);
    delay(20);
  }
  // stops pulsing — servo holds mechanically, no jitter
}

// ============ WATER SYSTEM ============
const int totalWater = 2000;
const int waterFlowRate = 50;

int currentWater = totalWater;
bool valveOpen = false;
unsigned long lastWaterUpdate = 0;
unsigned long valveOpenTime = 0;

// ============ FIRE DETECTION ============
const int tempThreshold = 25;
const int lightThreshold = 500;

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
  printMenu();
}

void loop() {
  handleWaterSystem();

  if (millis() - lastFireCheck >= fireCheckInterval) {
    if (fireDetected()) {
      Serial.println("FIRE DETECTED");
    }
    lastFireCheck = millis();
  }

  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read();
    }

    switch (c) {
      case 'f': moveForward();  break;
      case 'b': moveBackward(); break;
      case 'l': turnLeft();     break;
      case 'r': turnRight();    break;
      case 'h': hover();        break;
      case 'x': allOff();       break;
      case 'o': openValve();    break;
      case 'c': closeValve();   break;
      case 's': printStatus();  break;
      case 'm': printMenu();    break;
      default:  Serial.println("unk"); break;
    }
  }

  delay(10);
}

void printMenu() {
  Serial.println("m:menu f:forward b:back l:left r:right h:hover x:stop o:open c:close s:status");
}

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
  Serial.println("forward");
}

void moveBackward() {
  allOff();
  setAllForward();
  analogWrite(leftFrontEnable, SPEED_FRONT_FAST);
  analogWrite(rightFrontEnable, SPEED_FRONT_FAST);
  analogWrite(leftBackEnable, SPEED_BACK_SLOW);
  analogWrite(rightBackEnable, SPEED_BACK_SLOW);
  Serial.println("backward");
}

void turnRight() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable, SPEED_TURN_FAST);
  analogWrite(leftFrontEnable, SPEED_TURN_FAST);
  analogWrite(rightBackEnable, SPEED_TURN_SLOW);
  analogWrite(rightFrontEnable, SPEED_TURN_SLOW);
  Serial.println("right");
}

void turnLeft() {
  allOff();
  setAllForward();
  analogWrite(leftBackEnable, SPEED_TURN_SLOW);
  analogWrite(leftFrontEnable, SPEED_TURN_SLOW);
  analogWrite(rightBackEnable, SPEED_TURN_FAST);
  analogWrite(rightFrontEnable, SPEED_TURN_FAST);
  Serial.println("left");
}

void hover() {
  setAllForward();
  analogWrite(leftBackEnable, SPEED_HOVER);
  analogWrite(rightBackEnable, SPEED_HOVER);
  analogWrite(leftFrontEnable, SPEED_HOVER);
  analogWrite(rightFrontEnable, SPEED_HOVER);
  Serial.println("hover");
}

void handleWaterSystem() {
  if (valveOpen && currentWater > 0) {
    unsigned long n = millis();
    if (n - lastWaterUpdate >= 1000) {
      currentWater -= waterFlowRate;
      lastWaterUpdate = n;
      if (currentWater <= 0) {
        currentWater = 0;
        closeValve();
        Serial.println("empty auto-close");
      } else {
        Serial.print("water:");
        Serial.println(currentWater);
      }
    }
  }
}

void printStatus() {
  Serial.print("water:");
  Serial.print(currentWater);
  Serial.print("/");
  Serial.println(totalWater);

  int raw = analogRead(tempSensorPin);
  int t = map(raw, 20, 358, -40, 125);
  Serial.print("temp:");
  Serial.println(t);

  int l = analogRead(lightSensorPin);
  Serial.print("light:");
  Serial.println(l);

  Serial.print("valve:");
  Serial.println(valveOpen ? "open" : "closed");

  Serial.print("fire:");
  Serial.println(fireDetected() ? "yes" : "no");
}

// ============ Mohamed's functions ============
void openValve() {
  servoCommand(90);
  valveOpen = true;
  lastWaterUpdate = millis();
  Serial.println("valve open");
}

void closeValve() {
  servoCommand(0);
  valveOpen = false;
  Serial.println("valve closed");
}

bool fireDetected() {
  int rawTemp = analogRead(tempSensorPin);
  int tempC = map(rawTemp, 20, 358, -40, 125);
  int lightReading = analogRead(lightSensorPin);
  return (tempC >= tempThreshold && lightReading >= lightThreshold);
}
