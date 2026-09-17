# Mechanics Guide

## Belt Drive System

### Pulleys

| Component | Recommended | Notes |
|-----------|------------|-------|
| Belt type | GT2 / 2GT | 2 mm pitch; widely available |
| Motor pulley | 16T, 5 mm bore | Matches 28BYJ-48 output shaft |
| Focuser pulley | 60T, press-fit or set-screw | Size to fit Crayford knob shaft |
| Belt width | 6 mm | Standard width for GT2 |

### Gear Ratio

```
ratio = focuserPulleyTeeth / motorPulleyTeeth
      = 60 / 16
      = 3.750
```

A higher ratio → more torque, finer control, slower speed.
A lower ratio → less torque, coarser steps, faster speed.

The firmware computes the ratio from the pulley tooth counts and displays it
in the web UI and configuration page.

---

## Motor Mount

The 28BYJ-48 motor body should be:
- Rigidly attached to the telescope or focuser body (no flex).
- Positioned so the belt runs straight (no lateral load on motor shaft).
- Accessible for belt tension adjustment.

Belt tension: the belt should not sag but also not be so tight that it binds
the motor shaft bearing.

---

## Step Size and Resolution

With the default configuration:
```
stepsPerRev   = 4076 half-steps (motor output shaft)
ratio         = 3.75
Crayford      ≈ 2 mm per focuser shaft revolution (P200 estimate)

stepsPerMm    = 4076 × 3.75 / 2.0 ≈ 7642 steps/mm
resolution    ≈ 0.13 μm per step
```

> **Actual values depend on your specific Crayford drawtube pitch.
> Calibrate with a dial gauge (see docs/CALIBRATION.md).**

---

## Crayford Friction

The 28BYJ-48 with ULN2003 has limited torque (~0.034 N·m at the motor shaft).
Through the 3.75 belt ratio the effective torque at the focuser knob is:
```
~0.13 N·m
```

This is sufficient for most Crayford focusers under typical loads.
If the motor stalls:
- Reduce camera weight on the focuser.
- Reduce the Crayford friction (tension screw).
- Reduce motor speed (`motorMaxSpeed` in config).
- Consider a 20T→60T ratio (ratio 3.0) for more speed, or 12T→60T (ratio 5.0) for more torque.

---

## Sky-Watcher P200 Specifics

The P200 (200/1000) Crayford focuser has a 2" drawtube with:
- Approx. 60 mm total travel
- Standard 2" barrel
- Dual-speed optional (not interfered with by the motor; the motor replaces
  or supplements the coarse knob)

Mount the 60T pulley on the coarse focuser knob shaft. The fine adjust knob
can remain functional for manual override.
