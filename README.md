# Drone Project — Wiring Validation Test Suite

FSE 100 Mini Project: Firefighting Quadcopter

## Pin Map (Quick Reference)

| Motor | Position    | Enable (PWM) | Input A | Input B | Controller |
|-------|-------------|-------------|---------|---------|------------|
| MLB   | Left Back   | D5          | D13 (2A)* | D12 (1A)* | UREAR |
| MRB   | Right Back  | D6          | D7 (3A) | D2 (4A) | UREAR |
| MLF   | Left Front  | D9          | A1 (1A) | A0 (2A) | UFRONT |
| MRF   | Right Front | D10         | D11 (3A)| D8 (4A) | UFRONT |

*MLB polarity is physically reversed — Input A/B are swapped in code so "forward" matches the other motors.

| Sensor/Actuator | Pin | Notes |
|-----------------|-----|-------|
| TMP36           | A4  | ~25°C at room temp, ADC ≈ 153 |
| Photoresistor   | A5  | 10k pull-down divider. Click component to adjust light. |
| Servo valve     | D3  | 0°–180° proportional flow control |

## How to Run Tests

1. Open the [TinkerCAD project](https://www.tinkercad.com/things/6yOd0E1L9jI-color-coded-concat?sharecode=-n0Dz9ZZDIp9Zq27qAVLtp7cF929VPaRdQeI4db4ZJc)
2. Delete the existing code in the Arduino editor
3. Paste the contents of a test file
4. Start simulation and open Serial Monitor (9600 baud)
5. Run tests **in order 1 → 5**

## Tests

### Test 1: Sensor Wiring (`test1_sensors.ino`)
- Validates TMP36 (A4) and photoresistor (A5) are reading valid ranges
- **Important:** Click the photoresistor and raise the light slider before running — it defaults to 0V (total darkness)
- Type `r` in serial monitor to run/rerun
- **Auto PASS/FAIL** in serial output

### Test 2: Servo Valve (`test2_servo.ino`)
- Confirms D3 signal wire reaches the servo
- Sweeps 0° → 90° → 180° → 0°
- **Watch the servo arm move** in TinkerCAD to confirm

### Test 3: Motor Isolation (`test3_isolation.ino`)
- Spins each motor alone for 3 seconds, all others off
- **PASS** = only the named motor spins each round
- **FAIL** = wrong motor spins (wires crossed) or no motor spins (disconnected)

### Test 4: Direction Reversal (`test4_direction.ino`)
- Each motor spins one direction for 2s, then reverses for 2s
- **PASS** = motor changes direction
- **FAIL** = same direction both times (an input pin is disconnected or shorted)

### Test 5: Flight Profile (`test5_flight_profile.ino`)
- 5 phases, ~30 seconds total:
  1. **Takeoff ramp** — all 4 motors 0→200 over 5s (should gradually speed up)
  2. **Hover** — all 4 at 200 for 5s (should be equal speed)
  3. **Pitch forward** — front=240 rear=120 for 5s (front pair faster)
  4. **Roll right** — left=240 right=120 for 5s (left pair faster)
  5. **Yaw CW** — diagonal pairs opposite direction for 5s
- Serial monitor prints what to watch for before each phase

## What Each Test Catches

| Test | Catches |
|------|---------|
| 1    | Disconnected sensor wires, missing pull-down resistor, shorted pins |
| 2    | Servo signal wire disconnected or on wrong pin |
| 3    | Crossed motor wires, wrong enable-to-motor mapping |
| 4    | Direction input pin disconnected or both pins shorted together |
| 5    | Enable not carrying PWM (hardwired to VCC), wrong differential mapping |

## Team
- Kaden Schutt
- Mohamed Tigana
- Raghav Pahuja
- Jeremy Branom
