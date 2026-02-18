# Drone Project — Firefighting Quadcopter

FSE 100 Mini Project: Arduino-based firefighting quadcopter simulated in TinkerCAD. The drone detects fires via temperature and light sensors, sprays water through a servo-controlled valve, and is operated through a serial command interface with a live ASCII HUD.

## TinkerCAD

[Open project](https://www.tinkercad.com/things/gTlSbLrZ1fu-finalized-fse100-mini-project-pt-4?sharecode=BiLSVvK5VS0GNerAj0-p8AShSNrBMOhkJbpxzXZT1K0)

## Project Phases

### Phase 1 — Wiring Validation
Built the TinkerCAD circuit with two L293D motor controllers, 4 DC motors, a servo, TMP36 temperature sensor, and photoresistor. Wrote 5 test sketches (`tests/`) to validate every connection before writing flight code.

### Phase 2 — Base Flight Code
Wrote `drone_control.ino` with serial command control (forward, backward, left, right, hover, stop), water valve open/close, and basic fire detection using temperature and light thresholds. Used `Servo.h` and original pin assignments.

### Phase 3 — Timer Conflict Resolution
Discovered `analogWrite` on D5/D6 conflicts with Timer0 (breaks `millis()`/`delay()`/Serial), and `Servo.h` hijacks Timer1 (kills PWM on D9/D10). Rewired all motor enables to Timer1/Timer2 pins and replaced `Servo.h` with manual pulse generation on D5. Updated tests 2–5 for new pinout.

### Phase 4 — Team Code Merge
Integrated contributions from all four team members with attribution comments. Added water system simulation (2000mL tank, 50mL/s flow rate, auto-close on empty), fire detection loop, and valve control logic.

### Phase 5 — Dashboard HUD
Added a 6-line ASCII HUD to the serial monitor, designed to fit TinkerCAD's default view. Event-driven — only redraws when state changes (no serial spam). Added up/down flight commands, panic-style alerts for fire detection, low water, and empty tank. Implemented alert silencing so operators aren't repeatedly interrupted.

### Phase 6 — Cleanup
Removed experimental alternate code versions (minimal, readable, advanced). Consolidated to a single active code file (`Drone-Code-dashboard.ino`).

## Files

| File | Description |
|------|-------------|
| `Drone-Code-dashboard.ino` | **Active** — current code with HUD, alerts, and all team features |
| `Drone-Code-v1.ino` | Legacy — original rewired code before HUD |
| `drone_control.ino` | Legacy — original filename, identical to v1 |
| `tests/test1_sensors.ino` | TMP36 + photoresistor ADC validation |
| `tests/test2_servo.ino` | Servo manual pulse on D5 |
| `tests/test3_isolation.ino` | Motor isolation (no crossed wires) |
| `tests/test4_direction.ino` | Motor direction pin validation |
| `tests/test5_flight_profile.ino` | Flight profile (takeoff, hover, pitch, roll, yaw) |

## Serial Commands (9600 baud)

| Key | Action |
|-----|--------|
| `f` | Forward |
| `b` | Backward |
| `l` | Turn left |
| `r` | Turn right |
| `u` | Ascend |
| `d` | Descend |
| `h` | Hover |
| `x` | Stop (all motors off) |
| `o` | Open water valve |
| `c` | Close water valve |
| `s` | Refresh HUD |
| `m` | Show command list |

## Dashboard HUD

```
=== DRONE HUD ===               !! FIRE DETECTED !!
MODE:HOVER  FIRE:no              MODE:HOVER  FIRE:YES
WATER[########--]1600/2000       WATER[########--]1600/2000
VALVE:closed                     VALVE:closed
> hovering                       > Press 'o' to spray or 'c' to dismiss
press m for commands             >> RESPOND TO ALERT <<
```

The HUD switches to panic mode for fire detection, low water, and empty tank. Any command silences the alert; it only re-triggers if the fire condition clears and returns.

## Pin Map (Rewired — Timer-Safe)

| Timer | Pins | Usage |
|-------|------|-------|
| Timer0 (D5/D6) | D5 servo (manual pulse) | Untouched — `millis()`/`delay()`/Serial safe |
| Timer1 (D9/D10) | D9 EN_MLB, D10 EN_MRB | `analogWrite` OK |
| Timer2 (D3/D11) | D3 EN_MLF, D11 EN_MRF | `analogWrite` OK |

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
| Servo valve | D5 (manual pulse) |

## Wiring Validation Tests

Run in order 1 → 5 in TinkerCAD. Paste each file into the Arduino editor, start simulation, open Serial Monitor at 9600 baud.

| Test | File | What it checks |
|------|------|----------------|
| 1 | `test1_sensors.ino` | TMP36 + photoresistor range (type `r` to run) |
| 2 | `test2_servo.ino` | Servo manual pulse on D5 |
| 3 | `test3_isolation.ino` | Each motor spins alone |
| 4 | `test4_direction.ino` | Each motor reverses direction |
| 5 | `test5_flight_profile.ino` | Takeoff, hover, pitch, roll, yaw |

## Team
- Kaden Schutt — pin assignments, timer fix, manual servo, dashboard HUD
- Mohamed Tigana — water system, fire detection, main loop, valve control
- Raghav Pahuja — setup, motor control functions
- Jeremy Branom — ascend/descend functions
