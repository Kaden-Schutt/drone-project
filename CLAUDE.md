# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FSE 100 Mini Project: Firefighting Quadcopter — an Arduino-based drone simulated in TinkerCAD. The main flight code lives in `Drone-Code-v1.ino` (currently empty, to be written). The `tests/` directory contains 5 wiring validation sketches that must be run in TinkerCAD's simulator.

**AI Policy:** Permitted with citation per FSE 100 syllabus. Include citation when generating submission content:
```
Note: AI assistance (Claude) was used to help structure this response.
Specifically: [describe how AI was used]
```

## Hardware Pin Map

Two L293D motor controllers (UFRONT, UREAR) drive 4 DC motors. One servo valve and two analog sensors.

| Motor | Position | Enable (PWM) | Input A | Input B | Controller |
|-------|----------|-------------|---------|---------|------------|
| MLB | Left Back | D5 | D13 | D12 | UREAR |
| MRB | Right Back | D6 | D7 | D2 | UREAR |
| MLF | Left Front | D9 | A1 | A0 | UFRONT |
| MRF | Right Front | D10 | D11 | D8 | UFRONT |

**MLB polarity is physically reversed** — Input A/B are swapped in code so "forward" matches the other motors.

| Sensor/Actuator | Pin | Notes |
|-----------------|-----|-------|
| TMP36 | A4 | ~25°C room temp, ADC ≈ 153 |
| Photoresistor | A5 | 10k pull-down divider |
| Servo valve | D3 | 0°–180° proportional flow control |

## Running Tests

Tests run inside TinkerCAD (no local compilation toolchain). To test:
1. Open the [TinkerCAD project](https://www.tinkercad.com/things/6yOd0E1L9jI-color-coded-concat?sharecode=-n0Dz9ZZDIp9Zq27qAVLtp7cF929VPaRdQeI4db4ZJc)
2. Paste test file contents into the Arduino editor
3. Start simulation, open Serial Monitor at 9600 baud
4. Run tests **in order** 1 → 5

| Test | File | Validates |
|------|------|-----------|
| 1 | `tests/test1_sensors.ino` | TMP36 and photoresistor ADC ranges |
| 2 | `tests/test2_servo.ino` | Servo signal wire on D3 |
| 3 | `tests/test3_isolation.ino` | Each motor spins alone (no crossed wires) |
| 4 | `tests/test4_direction.ino` | Both direction pins work per motor |
| 5 | `tests/test5_flight_profile.ino` | Takeoff ramp, hover, pitch, roll, yaw |

## Code Conventions

- All `.ino` files target Arduino Uno (ATmega328P, 5V logic, 6 PWM pins)
- Motor speed control via `analogWrite()` on enable pins (0–255 PWM)
- Motor direction via `digitalWrite()` on input A/B pins
- Serial output at 9600 baud for all diagnostics
- Pin constants follow the naming pattern: `EN_<motor>` for enable, `<motor>_A` / `<motor>_B` for direction
- Motor names: MLB (motor left back), MRB (motor right back), MLF (motor left front), MRF (motor right front)
