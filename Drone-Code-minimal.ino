// Firefighting Drone — Minimal Edition
// Same pinout & behavior as Drone-Code-v1.ino, fewest lines possible
// FSE 100 Mini Project
//
// PIN LAYOUT (rewired for timer safety):
//   Timer1 (D9/D10) — back motor enables
//   Timer2 (D3/D11) — front motor enables
//   Timer0 (D5/D6)  — untouched (millis/delay/Serial safe)
//   D5 — servo via manual pulse (no Servo.h)

// Motor pins indexed [LB, RB, LF, RF]
const int EN[] = {9,10,3,11};   // PWM enable pins
const int DA[] = {13,7,A1,4};   // direction pin A (HIGH = forward)
const int DB[] = {12,2,A0,8};   // direction pin B (LOW  = forward)
const int SRV=5, TMP=A4, LDR=A5;

int water = 2000;  bool vlv = false;
unsigned long lastW = 0, lastF = 0;

// Manual servo: 25 pulses × 20ms = 500ms to settle position
void servo(int a) {
  a = constrain(a, 0, 180);
  for (int i = 0; i < 25; i++) {
    digitalWrite(SRV, HIGH); delayMicroseconds(map(a, 0, 180, 544, 2400));
    digitalWrite(SRV, LOW);  delay(20);
  }
}

// Kill all motors and clear direction pins
void off() { for (int i=0; i<4; i++) { analogWrite(EN[i],0); digitalWrite(DA[i],LOW); digitalWrite(DB[i],LOW); } }

// All direction pins → forward spin
void fwd() { for (int i=0; i<4; i++) { digitalWrite(DA[i],HIGH); digitalWrite(DB[i],LOW); } }

// Reset + set forward + apply speeds to [LB, RB, LF, RF]
void spd(int lb, int rb, int lf, int rf) {
  off(); fwd();
  int s[] = {lb, rb, lf, rf};
  for (int i=0; i<4; i++) analogWrite(EN[i], s[i]);
}

// Fire = hot AND bright (both thresholds exceeded)
bool fire() { return map(analogRead(TMP),20,358,-40,125) >= 25 && analogRead(LDR) >= 500; }

void setup() {
  Serial.begin(9600);
  pinMode(SRV, OUTPUT); servo(0);
  for (int i=0; i<4; i++) { pinMode(EN[i],OUTPUT); pinMode(DA[i],OUTPUT); pinMode(DB[i],OUTPUT); }
  pinMode(TMP, INPUT); pinMode(LDR, INPUT);
  off();
  Serial.println("m:menu f:fwd b:back l:left r:right u:up d:down h:hover x:stop o:open c:close s:status");
}

void loop() {
  // Drain 50mL/sec while valve is open
  if (vlv && water>0 && millis()-lastW >= 1000) {
    water -= 50; lastW = millis();
    if (water <= 0) { water=0; servo(0); vlv=false; Serial.println("empty auto-close"); }
    else { Serial.print("water:"); Serial.print(water); if(water<=400) Serial.print(" LOW"); Serial.println(); }
  }
  // Check sensors for fire every 2s
  if (millis()-lastF >= 2000) { if (fire()) Serial.println("FIRE DETECTED"); lastF = millis(); }

  if (Serial.available()) {
    char c = Serial.read(); while (Serial.available()) Serial.read();
    switch (c) {
      case 'f': spd(200,200,150,150); Serial.println("forward");  break; // back fast → pitch fwd
      case 'b': spd(150,150,200,200); Serial.println("backward"); break; // front fast → pitch back
      case 'r': spd(200,150,200,150); Serial.println("right");    break; // left fast → yaw right
      case 'l': spd(150,200,150,200); Serial.println("left");     break; // right fast → yaw left
      case 'h': fwd(); for(int i=0;i<4;i++) analogWrite(EN[i],170); Serial.println("hover"); break;
      case 'u': fwd(); for(int i=0;i<4;i++) analogWrite(EN[i],220); Serial.println("ascend"); break;  // above hover → climb
      case 'd': fwd(); for(int i=0;i<4;i++) analogWrite(EN[i],120); Serial.println("descend"); break; // below hover → sink
      case 'x': off(); Serial.println("stop"); break;
      case 'o': servo(90); vlv=true;  lastW=millis(); Serial.println("valve open");   break;
      case 'c': servo(0);  vlv=false;                 Serial.println("valve closed"); break;
      case 's': // compact status line
        Serial.print("water:"); Serial.print(water); Serial.print("/2000 temp:");
        Serial.print(map(analogRead(TMP),20,358,-40,125)); Serial.print("C light:");
        Serial.print(analogRead(LDR)); Serial.print(" valve:"); Serial.print(vlv?"open":"closed");
        Serial.print(" fire:"); Serial.println(fire()?"yes":"no"); break;
      case 'm': Serial.println("m:menu f:fwd b:back l:left r:right h:hover x:stop o:open c:close s:status"); break;
      default:  Serial.println("unk"); break;
    }
  }
  delay(10);
}
