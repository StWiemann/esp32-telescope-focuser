# Calibration Guide

## Equipment Needed

- Dial gauge / digital indicator, 0.01 mm resolution
- Magnetic stand or bracket to mount the gauge against the focuser tube

---

## 1. Determine Actual Steps per Revolution

The 28BYJ-48 gear ratio is nominally 64:1 but varies by batch.
Default firmware value: **4076 half-steps/revolution**.

### Procedure

1. Mount the gauge tip against the focuser drawtube at 90° to the tube axis.
   (Alternatively, mark a line on the motor pulley and count rotations.)
2. Zero the focuser (ZERO button with focuser fully retracted).
3. Using the web UI (`/api/move`), command exactly one motor-output-shaft
   revolution worth of steps:
   - At default 16T motor / 60T focuser ratio, `stepsPerRev / ratio` steps
     would rotate the focuser shaft one full turn.
   - Start with a round number: command **4000 steps** to the web UI.
4. Record the actual drawtube displacement on the dial gauge (mm).
5. Repeat for 4100 and 4200 steps to bracket the correct value.
6. Linear interpolation: `stepsForFullRev = steps × (fullRoundDisplacement / measuredDisplacement)`
7. Update `stepsPerRev` in the web UI configuration.

---

## 2. Determine Travel per Step (μm/step)

After step 1:

```
stepsPerMm = stepsPerRev * beltRatio / drawtubeMmPerRev
micronsPerStep = 1000 / stepsPerMm
```

Example (P200 Crayford, ~2 mm per focuser shaft turn):
```
stepsPerRev   = 4076   (half-steps, motor output shaft)
beltRatio     = 60 / 16 = 3.75
drawtubeTrav  = 2.0 mm per focuser shaft revolution
stepsPerMm    = 4076 × 3.75 / 2.0 ≈ 7643 steps/mm
micronsPerStep≈ 0.13 μm/step
```

The web configuration shows `Steps/mm` in the belt ratio display after entering
the pulley sizes.

---

## 3. Determine Maximum Usable Travel

1. With the focuser fully retracted, zero the position (ZERO → position = 0).
2. Move outward in increments of 10 000 steps via the web UI.
3. Stop 2–3 mm before the mechanical hard stop.
4. Read the position; set `maxPosition` to this value.
5. Save configuration.

This prevents the firmware from commanding moves past the mechanical stop.

---

## 4. Measure Backlash

Backlash is the mechanical play in the gearbox + belt + Crayford combination.

### Procedure

1. Zero the focuser at the retracted position.
2. Move outward by 5000 steps.
3. Mount the dial gauge and zero it.
4. Command the focuser 500 steps inward.
5. Note the gauge reading (A mm).
6. Command the focuser 500 steps outward (back to the same logical position).
7. Note the gauge reading (B mm). If B ≠ 0.00, the difference is backlash.
8. Backlash in steps: `backlashSteps = round(backlashDistance_mm × stepsPerMm)`

### Setting in Firmware

- Open `http://esp32-focuser.local/` → Configuration.
- Set `Backlash steps` to the measured value.
- Set `Backlash approach direction`:
  - `+1` (outward): always approach the final position from below.  
    Use when your Crayford sags (gravity loads inward).
  - `-1` (inward): always approach from above.
- Save.

### Verification

1. Move to position 10 000.
2. Move to 8 000 (inward).
3. Move back to 10 000 (outward).
4. Compare dial gauge reading before and after: should be ≤ 0.05 mm.
