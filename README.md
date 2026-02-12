# Drone Project — Firefighting Quadcopter

FSE 100 Mini Project: Firefighting Quadcopter

## TinkerCAD

[Open project](https://www.tinkercad.com/things/9Rtwm0qNx6y-fse100-full-spec-drone-code?sharecode=Vy8b_OYDbGn0Ie5N9A64pYtMWNLehlkzH-VMPBi1KqQ)

## Main Code

**`drone_control.ino`** — paste into TinkerCAD Arduino editor.

Serial commands (9600 baud):

| Key | Action |
|-----|--------|
| `f` | Forward |
| `b` | Backward |
| `l` | Turn left |
| `r` | Turn right |
| `h` | Hover |
| `x` | Stop (all motors off) |
| `o` | Open water valve |
| `c` | Close water valve |
| `s` | Print status (temp, light, water, fire) |
| `m` | Print menu |

## Pin Map (Rewired — Timer-Safe)

**Why rewired:** `analogWrite` on D5/D6 conflicts with Timer0 (breaks Serial/delay). `Servo.h` hijacks Timer1 (kills PWM on D9/D10). Fix: move enables to Timer1/Timer2, servo to manual pulse on D5.

| Pin | Function | Timer |
|-----|----------|-------|
| D9  | EN_MLB (Left Back speed) | Timer1 ✅ |
| D10 | EN_MRB (Right Back speed) | Timer1 ✅ |
| D3  | EN_MLF (Left Front speed) | Timer2 ✅ |
| D11 | EN_MRF (Right Front speed) | Timer2 ✅ |
| D5  | Servo valve (manual pulse) | No timer needed |

| Motor | Position | Enable | Dir A | Dir B | Controller |
|-------|----------|--------|-------|-------|------------|
| MLB   | Left Back | D9 | D13* | D12* | UREAR 1-2 |
| MRB   | Right Back | D10 | D7 | D2 | UREAR 3-4 |
| MLF   | Left Front | D3 | A1 | A0 | UFRONT 1-2 |
| MRF   | Right Front | D11 | D4 | D8 | UFRONT 3-4 |

*MLB polarity reversed — Dir A/B swapped in code.

| Sensor | Pin |
|--------|-----|
| TMP36 | A4 |
| Photoresistor | A5 (10k pull-down) |
| Servo valve | D5 |

## Wiring Validation Tests (`tests/`)

Run in order 1→5 in TinkerCAD. Paste each file into the Arduino editor.

| Test | File | What it checks |
|------|------|----------------|
| 1 | `test1_sensors.ino` | TMP36 + photoresistor range (type `r` to run) |
| 2 | `test2_servo.ino` | Servo sweep on D3 (visual) |
| 3 | `test3_isolation.ino` | Each motor spins alone |
| 4 | `test4_direction.ino` | Each motor reverses direction |
| 5 | `test5_flight_profile.ino` | Takeoff, hover, pitch, roll, yaw |

**Note:** Tests still use old pin assignments. Run main code with new pins.

## Team
- Kaden Schutt
- Mohamed Tigana
- Raghav Pahuja
- Jeremy Branom
