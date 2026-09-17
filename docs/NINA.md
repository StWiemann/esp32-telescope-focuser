# NINA Integration Guide

## Requirements

- N.I.N.A. 3.x (tested with 3.2+)
- ESP32 Focuser on the same WiFi network
- Position zeroed (ZERO button pressed after fully retracting focuser)

---

## Setup Steps

### 1. Power on the ESP32 Focuser

Connect the ESP32 to 5 V power. Watch the OLED:
```
ESP32 FOCUSER
Pos: ----
STATUS: UNZEROED
WiFi: OK   Alp: OK
```
`WiFi: OK` and `Alp: OK` must both appear before proceeding.

### 2. Zero the Focuser

Before using with NINA, establish the position reference:

1. Use the **−** button to drive the focuser completely inward (to mechanical stop).
2. Hold the **ZERO** button for 2 seconds.
3. OLED shows `STATUS: READY` and `Pos: 0`.

> Do this once per power cycle or any time the position is uncertain.

### 3. Open N.I.N.A.

Navigate to: **Equipment → Focuser → [dropdown]**

### 4. Select Alpaca Focuser

1. In the focuser dropdown, choose **ASCOM Alpaca**.
2. Click the gear icon (⚙) next to the dropdown.
3. In the ASCOM Alpaca Chooser, click **Discover**.
4. Wait a few seconds; the focuser should appear as `ESP32 P200 Focuser`.
5. Select it and click **OK**.

If auto-discovery fails:
- Click **Add** and manually enter `http://esp32-focuser.local` (or the IP address).
- Port: `80`, Device type: `Focuser`, Device number: `0`.

### 5. Connect

Click the **Connect** button in NINA's focuser panel.
NINA reads `Position`, `MaxStep`, `IsMoving`, `Absolute=True`.

### 6. Test Manual Movement

Use NINA's **+/−** buttons or the arrow controls to move the focuser.
Verify the OLED position updates.

### 7. Autofocus

NINA's built-in autofocus (Equipment → Focuser → AF Wizard or during imaging):
1. Configure the autofocus step size (recommended: 200–500 steps as a starting
   point; calibrate with your scope/camera combination).
2. Run **Auto Focus** – NINA will call `Move` to absolute positions around the
   current position and evaluate HFR curves.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Focuser not discovered | Different subnet, mDNS blocked | Use IP instead of hostname |
| Position jumps at connect | NINA reads position 0 (unzeroed) | Zero first, then connect |
| "IsMoving" stuck true | Motor busy, halt then reconnect | Press STOP in web UI |
| NINA shows no response | Alpaca port blocked (firewall) | Check Windows firewall allows port 80 inbound |

---

## Alpaca Endpoints Used by NINA

NINA queries/sets these Alpaca Focuser V3 properties:

| Property | Used |
|----------|------|
| `connected` (GET/PUT) | ✓ |
| `position` (GET) | ✓ |
| `ismoving` (GET) | ✓ |
| `move` (PUT) | ✓ absolute |
| `halt` (PUT) | ✓ |
| `maxstep` (GET) | ✓ |
| `absolute` (GET) | returns `true` |
| `tempcomp` | returns `false` |
| `temperature` | returns `NaN` |

---

## ASCOM Conform Universal

To verify full conformance: run ConformU 3.x against the focuser.
The firmware targets zero errors and zero issues on ConformU when the
underlying ESP32AlpacaDevices2 library is used.
