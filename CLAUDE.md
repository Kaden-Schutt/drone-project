# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FSE 100 Mini Project: Firefighting Quadcopter — an Arduino-based drone simulated in TinkerCAD. Four team members contribute code sections with attribution comments.

**Active code file:** `Drone-Code-dashboard.ino` — the current working version with serial HUD, fire/water alerts, and all team features merged.

**Legacy files (reference only):**
- `Drone-Code-v1.ino` — original rewired code before HUD and team merges
- `drone_control.ino` — identical copy of v1 (original filename)

**AI Policy:** Permitted with citation per FSE 100 syllabus. Include citation when generating submission content:
```
Note: AI assistance (Claude) was used to help structure this response.
Specifically: [describe how AI was used]
```

## Hardware Pin Map (Rewired — Timer-Safe)

Two L293D motor controllers (UFRONT, UREAR) drive 4 DC motors. One servo valve and two analog sensors.

**Why rewired:** `analogWrite` on D5/D6 conflicts with Timer0 (breaks `millis()`/`delay()`/Serial). `Servo.h` hijacks Timer1 (kills PWM on D9/D10). Fix: move enables to Timer1/Timer2, servo to manual pulse on D5.

| Timer | Pins | Usage |
|-------|------|-------|
| Timer0 (D5/D6) | D5 servo (manual pulse) | Untouched — `millis()`/`delay()`/Serial safe |
| Timer1 (D9/D10) | D9 EN_MLB, D10 EN_MRB | `analogWrite` OK |
| Timer2 (D3/D11) | D3 EN_MLF, D11 EN_MRF | `analogWrite` OK |

| Motor | Position | Enable (PWM) | Dir A | Dir B | Controller |
|-------|----------|-------------|-------|-------|------------|
| MLB | Left Back | D9 | D13* | D12* | UREAR |
| MRB | Right Back | D10 | D7 | D2 | UREAR |
| MLF | Left Front | D3 | A1 | A0 | UFRONT |
| MRF | Right Front | D11 | D4 | D8 | UFRONT |

*MLB polarity is physically reversed — Dir A/B are swapped in code so "forward" matches the other motors.

| Sensor/Actuator | Pin | Notes |
|-----------------|-----|-------|
| TMP36 | A4 | ~25°C room temp, ADC ~153 |
| Photoresistor | A5 | 10k pull-down divider |
| Servo valve | D5 | Manual pulse (no Servo.h, no timer hijack) |

## Current Code Architecture (Drone-Code-dashboard.ino)

### Key Features
- **6-line serial HUD** — event-driven, only redraws on state change (`needsRefresh` flag)
- **Panic alerts** — `!! FIRE DETECTED !!`, `!! LOW WATER WARNING !!`, `!! TANK EMPTY !!` replace HUD header
- **Alert silencing** — `alertAcknowledged` flag prevents re-triggering; resets when fire condition clears
- **Water system** — simulated 2000mL tank, 50mL/s flow, auto-close on empty with easter egg
- **Manual servo** — `servoCommand()` generates PWM pulses without Servo.h
- **Differential thrust** — forward/backward/turn via speed differences between motor pairs
- **Up/down** — uniform speed increase (ascend) or decrease (descend) across all motors

### Code Sections (Team Attribution)
| Section | Author | Contents |
|---------|--------|----------|
| Pin assignments, manual servo, dashboard HUD | Kaden | Pin constants, `servoCommand()`, `printDashboard()`, `printCommands()` |
| Water system, fire detection, main loop, valve control | Mohamed | Water state, `handleWaterSystem()`, `loop()`, `openValve()`, `closeValve()`, `fireDetected()`, easter egg |
| Setup, motor control | Raghav | `setup()`, `allOff()`, `setAllForward()`, movement functions |
| Up/down functions | Jeremy | `moveUp()`, `moveDown()` |

### State Flags
| Flag | Purpose |
|------|---------|
| `needsRefresh` | Triggers HUD redraw on next loop iteration |
| `fireLocked` | Shows panic banner and "RESPOND TO ALERT" prompt |
| `alertAcknowledged` | Silences fire alert until condition clears and returns |
| `lowWaterWarned` | One-shot low water warning, resets on valve close |
| `valveOpen` | Tracks servo/valve state |

## Running Tests

Tests run inside TinkerCAD (no local compilation toolchain). Tests 2–5 have been updated for the rewired pinout.

1. Open the [TinkerCAD project](https://www.tinkercad.com/things/6yOd0E1L9jI-color-coded-concat?sharecode=-n0Dz9ZZDIp9Zq27qAVLtp7cF929VPaRdQeI4db4ZJc)
2. Paste test file contents into the Arduino editor
3. Start simulation, open Serial Monitor at 9600 baud
4. Run tests **in order** 1 → 5

| Test | File | Validates |
|------|------|-----------|
| 1 | `tests/test1_sensors.ino` | TMP36 and photoresistor ADC ranges |
| 2 | `tests/test2_servo.ino` | Servo manual pulse on D5 (no Servo.h) |
| 3 | `tests/test3_isolation.ino` | Each motor spins alone (rewired pins) |
| 4 | `tests/test4_direction.ino` | Both direction pins work per motor (rewired) |
| 5 | `tests/test5_flight_profile.ino` | Takeoff ramp, hover, pitch, roll, yaw (rewired) |

## Code Conventions

- All `.ino` files target Arduino Uno (ATmega328P, 5V logic, 6 PWM pins)
- Motor speed control via `analogWrite()` on enable pins (0–255 PWM)
- Motor direction via `digitalWrite()` on Dir A/B pins
- Serial output at 9600 baud for all diagnostics
- Pin constants use descriptive names: `leftBackEnable`, `leftBackDirA`, etc.
- Motor names: MLB (motor left back), MRB (motor right back), MLF (motor left front), MRF (motor right front)
- Team attribution via section comments: `// ========== Author - SECTION ============`
