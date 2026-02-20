# Feature: Web Dashboard & Test Mode Portal

## Metadata
- **Priority:** High
- **Complexity:** High
- **Estimated Sessions:** 4-6
- **Dependencies:** None (can be built in parallel with other features)

## Description
A modern web application hosted on the ESP32 that provides real-time status monitoring, full test mode controls (mirroring the Python test_mode_gui.py), and a dedicated motor limits finder (mirroring motor_limits_gui.py). The ESP32 runs in WiFi Access Point mode, creating its own network (e.g., "LevelingPrism"). Users connect their phone or laptop and open a browser. The UI is stored on LittleFS and communicates via WebSocket for real-time data.

## User Story
As a user, I want to connect to the ESP32's WiFi hotspot and open a web dashboard on my phone or laptop so that I can monitor the platform status, test all hardware components, and find motor limits — all without needing a USB serial connection.

## Requirements

### Tab 1: Dashboard (Status Overview)
- [ ] Real-time bubble level visualization (canvas-based, like Python GUI)
  - Color coding: green (<0.5°), orange (0.5-2°), red (>2°)
  - Pitch/roll numeric readouts in degrees
- [ ] Motor 1 and Motor 2 position bars (vertical, color-coded)
  - Blue (normal), Red (at limit)
  - Shows current position within MIN/MAX range
  - Displays position value and [MIN]/[MAX] labels
- [ ] Current system state with color-coded badge
- [ ] IMU data section: accel X/Y/Z, gyro X/Y/Z, temperature
- [ ] Calibration status (yes/no)
- [ ] Leveling status: pitch/roll error, correction output (when in LEVELING)
- [ ] Quick action buttons: Start Leveling, Reset to IDLE, Query Positions
- [ ] Continuous logging toggle (live P/R/M1/M2 stream)
- [ ] Connection status indicator (WebSocket connected/disconnected)

### Tab 2: Test Mode (mirrors test_mode_gui.py)

#### Motors Section
- [ ] Motor 1 frame: step input, Move +/- buttons
- [ ] Motor 2 frame: step input, Move +/- buttons (M2 direction reversed in UI)
- [ ] Quick step buttons: ±10, ±50, ±100, ±200, ±500, ±1000
- [ ] Both-motors simultaneous move (+ and - buttons)
- [ ] Continuous rotation toggles (m1c, m2c)
- [ ] Stop All button (mstop)
- [ ] Speed control (RPM input, 1-15)
- [ ] Motor position reset: mreset, mreset1, mreset2
- [ ] Motor position set: mset <value>

#### IMU Section
- [ ] I2C bus scan button
- [ ] Initialize IMU button
- [ ] Single reading button
- [ ] Raw values button
- [ ] Stream toggle (10 Hz IMU data)
- [ ] Run Calibration button

#### LED Section
- [ ] ON/OFF buttons
- [ ] Pattern buttons: Slow Blink, Fast Blink, Double Pulse, Error, Cycle
- [ ] Color buttons: Red, Green, Blue, Yellow, Cyan, Purple, White
- [ ] LED raw test button (ledtest)

#### Button Section
- [ ] Button test toggle (shows SHORT_PRESS / LONG_PRESS events)

#### System Section
- [ ] Show Pin Info, Show Status, Show Help buttons
- [ ] Toggle Logging, Reset to IDLE buttons
- [ ] Stability timeout adjustment (st <sec> input)
- [ ] PI gain adjustment (Kp/Ki inputs with current values)
- [ ] Level tolerance adjustment (input with current value)

### Tab 3: Motor Limits (mirrors motor_limits_gui.py)
- [ ] Step amount selector: radio buttons (100, 500, 1000, 5000, 10000) + custom input
- [ ] Both Motors frame: large +/- buttons for synchronized movement
- [ ] Motor 1 frame:
  - Large position display (current step count, bold)
  - +/- movement buttons
  - Set Zero button (reset position to 0)
  - Set IN (min limit) button — captures current position
  - Set OUT (max limit) button — captures current position
  - IN/OUT value display
- [ ] Motor 2 frame: identical to Motor 1 (with M2 direction reversal)
- [ ] Config Values display:
  - M1: IN=value, OUT=value
  - M2: IN=value, OUT=value
  - Generated `#define MOTOR_MIN_POSITION` and `MOTOR_MAX_POSITION` values
- [ ] Copy to Clipboard button for config.h values
- [ ] Unlock limits button (munlock) — removes position safety limits during limit-finding
- [ ] Lock limits button (mlock) — restores default limits

### Tab 4: Serial Terminal
- [ ] Scrolling output log showing all serial-like messages
- [ ] Command input field with Send button
- [ ] Commands sent as raw serial passthrough to firmware
- [ ] Clear log button
- [ ] Auto-scroll toggle

### Infrastructure
- [ ] ESP32 WiFi Access Point mode with configurable SSID/password
- [ ] AsyncWebServer serving static files from LittleFS
- [ ] WebSocket for real-time bidirectional data (status streaming + commands)
- [ ] LittleFS filesystem with HTML/CSS/JS files
- [ ] Responsive design (works on phone and desktop)

## Technical Approach

### Architecture Overview
```
Phone/Laptop Browser
    ↕ WiFi (AP mode)
ESP32
    ├── AsyncWebServer (port 80)
    │   ├── GET / → index.html (from LittleFS)
    │   ├── GET /style.css, /app.js (from LittleFS)
    │   └── WebSocket /ws
    │       ├── Server → Client: JSON status updates (10 Hz)
    │       └── Client → Server: JSON commands
    └── Existing firmware (state machine, IMU, motors, serial CLI)
```

### Phase 1: WiFi + Web Server Foundation
1. Add ESPAsyncWebServer and AsyncTCP libraries to platformio.ini
2. Add LittleFS to platformio.ini build config
3. Create `lib/WebServer/WebServer.h` and `.cpp` — wrapper class that:
   - Starts WiFi in AP mode (SSID: "LevelingPrism", password: "level1234")
   - Initializes LittleFS
   - Creates AsyncWebServer on port 80
   - Serves static files from LittleFS root
   - Sets up WebSocket endpoint at `/ws`
4. Create minimal `data/index.html` with "Connected!" test
5. Initialize in `setup()`, call `ws.cleanupClients()` in `loop()`

### Phase 2: Real-Time Status Streaming
1. Create a `getSystemStatusJSON()` function that builds a JSON object with:
   - `state`: current SystemState as string
   - `pitch`, `roll`: current filtered angles
   - `accel`: {x, y, z} in g
   - `gyro`: {x, y, z} in deg/s
   - `temperature`: IMU temp in °C
   - `motor1`: {position, atLimit, min, max}
   - `motor2`: {position, atLimit, min, max}
   - `calibrated`: bool
   - `config`: {kpPitch, kiPitch, kpRoll, kiRoll, tolerance, stabilityTimeoutMs}
   - `leveling`: {pitchError, rollError, m1Correction, m2Correction} (when in LEVELING state)
   - `uptime`: millis()
2. In `loop()`, send JSON over WebSocket at 10 Hz to all connected clients
3. Use ArduinoJson library for serialization

### Phase 3: Command Handling
WebSocket incoming messages are JSON commands. Full command set:

#### Motor Commands
- `{"cmd": "m1", "value": N}` → `motors.moveMotor1(N)`
- `{"cmd": "m2", "value": N}` → `motors.moveMotor2(N)` (web UI handles M2 direction flip)
- `{"cmd": "both", "m1": N, "m2": M}` → `motors.moveBoth(N, M)`
- `{"cmd": "m1c"}` / `{"cmd": "m2c"}` → toggle continuous rotation
- `{"cmd": "mstop"}` → stop all motors
- `{"cmd": "mspeed", "value": N}` → set speed RPM
- `{"cmd": "mreset"}` → reset both positions
- `{"cmd": "mreset1"}` / `{"cmd": "mreset2"}` → reset individual
- `{"cmd": "mset", "value": N}` → set both position counters
- `{"cmd": "mpos"}` → query positions (response in status JSON)
- `{"cmd": "munlock"}` → remove position limits
- `{"cmd": "mlock"}` → restore position limits

#### IMU Commands
- `{"cmd": "calibrate"}` → `imu.calibrate()`
- `{"cmd": "scan"}` → I2C bus scan
- `{"cmd": "imuinit"}` → initialize IMU
- `{"cmd": "read"}` → single IMU reading
- `{"cmd": "stream"}` → toggle streaming
- `{"cmd": "raw"}` → show raw sensor values

#### LED Commands
- `{"cmd": "led", "value": "on|off|slow|fast|pulse|error|cycle|red|green|blue|yellow|cyan|purple|white"}`
- `{"cmd": "ledtest"}` → raw GPIO LED test

#### Config Commands
- `{"cmd": "setPI", "kpP": F, "kiP": F, "kpR": F, "kiR": F}` → update PI gains
- `{"cmd": "setTolerance", "value": F}` → update level tolerance
- `{"cmd": "setStabilityTimeout", "value": F}` → set timeout in seconds

#### State Commands
- `{"cmd": "level"}` → start leveling
- `{"cmd": "reset"}` → return to IDLE
- `{"cmd": "testmode"}` → enter test mode
- `{"cmd": "log", "enabled": bool}` → toggle logging

#### Raw Serial Passthrough
- `{"cmd": "serial", "text": "..."}` → execute as if typed in serial monitor

All commands return: `{"ack": "cmdName", "ok": true/false, "msg": "..."}`

### Phase 4: Web UI (LittleFS Files)

**`data/index.html`** — Single page app with 4 tabs:
1. **Dashboard** — Live status (bubble level, motor bars, state badge, IMU data)
2. **Test Mode** — Full hardware controls (motors, IMU, LED, button, system settings)
3. **Motor Limits** — Dedicated limit-finding tool (step selector, per-motor controls, limit capture, config generator)
4. **Serial Terminal** — Raw command input/output

**`data/style.css`** — Modern dark theme CSS:
- CSS Grid/Flexbox layout
- Color-coded state badges
- Responsive breakpoints for phone/desktop
- Animated bubble level indicator (canvas)
- Motor position bar charts
- Large touch-friendly buttons for motor controls
- Color-matched LED buttons

**`data/app.js`** — Client-side JavaScript:
- WebSocket connection with auto-reconnect
- Parse incoming JSON status, update all DOM elements across all tabs
- Send commands as JSON on button clicks
- Tab switching logic
- Bubble level canvas rendering
- Motor limits state management (track IN/OUT for each motor, generate config values)
- Serial terminal with scrollback buffer
- Motor 2 direction reversal handled client-side (+ button sends negative steps)

### Phase 5: Integration & Polish
1. Ensure WebSocket + serial CLI coexist (both can send commands)
2. Add WiFi status to serial output on boot (IP address, SSID)
3. Add connection count indicator
4. Handle WebSocket disconnection gracefully (auto-reconnect in JS)
5. Test on phone browser (Chrome mobile, Safari)
6. Add mDNS so user can navigate to `http://prism.local`

### Libraries Required
- **ESPAsyncWebServer** — Async HTTP + WebSocket server
- **AsyncTCP** — Required by ESPAsyncWebServer on ESP32
- **ArduinoJson** — JSON serialization/deserialization
- **LittleFS** — Built into ESP32 Arduino core (no extra lib needed)

### Memory Considerations
- ESP32 has ~320KB RAM, ~4MB flash
- LittleFS partition: ~1.5MB available (plenty for HTML/CSS/JS)
- WebSocket buffers: ~2-4KB per client (support max 4 clients)
- ArduinoJson: use stack-allocated `StaticJsonDocument<512>` for status messages
- AsyncWebServer: runs on core 0 (WiFi core), firmware on core 1

## Subtasks

### Session 1: WiFi + WebSocket Foundation
1. [ ] Add library dependencies to platformio.ini: ESPAsyncWebServer, AsyncTCP, ArduinoJson
2. [ ] Add LittleFS build flags and partition table to platformio.ini
3. [ ] Add WiFi config defines to config.h (SSID, password, channel)
4. [ ] Create `lib/WebServer/WebServer.h` — class declaration
5. [ ] Create `lib/WebServer/WebServer.cpp` — WiFi AP init, AsyncWebServer, LittleFS, WebSocket setup
6. [ ] Create `data/index.html` — minimal test page ("ESP32 Connected!")
7. [ ] Initialize WebServer in main.cpp `setup()`, add `cleanupClients()` to `loop()`
8. [ ] Build, upload firmware + upload LittleFS filesystem
9. [ ] Test: connect phone to WiFi AP, open 192.168.4.1, see test page

### Session 2: Status Streaming + Command Handling
10. [ ] Create `getSystemStatusJSON()` function in main.cpp (or WebServer class)
11. [ ] Add 10 Hz WebSocket broadcast in `loop()` — send JSON status to all clients
12. [ ] Implement WebSocket `onEvent` handler for incoming JSON commands
13. [ ] Map all motor commands (m1, m2, both, mstop, mspeed, mreset, mreset1/2, mset, mpos, munlock, mlock, m1c, m2c)
14. [ ] Map all IMU commands (calibrate, scan, imuinit, read, stream, raw)
15. [ ] Map all LED commands (on/off/patterns/colors/ledtest)
16. [ ] Map config commands (setPI, setTolerance, setStabilityTimeout)
17. [ ] Map state commands (level, reset, testmode, log)
18. [ ] Add serial passthrough command
19. [ ] Test: use browser console or wscat to verify JSON messages flow both ways

### Session 3: Dashboard Tab UI
20. [ ] Create `data/style.css` — dark theme, responsive layout, tab system, state badges
21. [ ] Create `data/app.js` — WebSocket client with auto-reconnect, tab switching, DOM updates
22. [ ] Build Dashboard: state badge, bubble level (canvas), pitch/roll readouts
23. [ ] Build Dashboard: motor position bars (vertical, color-coded, with limits)
24. [ ] Build Dashboard: IMU data section (accel/gyro/temp, calibration status)
25. [ ] Build Dashboard: quick action buttons (Level, Reset, Query Positions)
26. [ ] Build Dashboard: live log stream section (toggleable)
27. [ ] Upload LittleFS, test on phone and desktop browser

### Session 4: Test Mode Tab UI
28. [ ] Build Motors section: step input, +/- buttons, quick step presets (10/50/100/200/500/1000)
29. [ ] Build Motors section: both-motors control, continuous toggles, stop all, speed control
30. [ ] Build Motors section: position reset buttons (mreset, mreset1, mreset2, mset)
31. [ ] Build IMU section: scan, init, read, raw, stream, calibrate buttons
32. [ ] Build LED section: on/off, pattern buttons, color buttons, ledtest
33. [ ] Build Button section: button test toggle with event display
34. [ ] Build System section: PI gain inputs, tolerance input, stability timeout input
35. [ ] Build System section: state buttons (Level, Reset, Test Mode), logging toggle
36. [ ] Upload LittleFS, test all controls end-to-end

### Session 5: Motor Limits Tab UI
37. [ ] Build step amount selector: radio buttons (100/500/1000/5000/10000) + custom input
38. [ ] Build Both Motors frame: large +/- buttons for synchronized movement
39. [ ] Build Motor 1 frame: large position display, +/- buttons, Set Zero button
40. [ ] Build Motor 1 frame: Set IN (min) and Set OUT (max) capture buttons with value display
41. [ ] Build Motor 2 frame: identical to M1 with direction reversal
42. [ ] Build Config Values display: M1/M2 IN/OUT values + generated #define statements
43. [ ] Build Copy to Clipboard button for config values
44. [ ] Build unlock/lock limits buttons (munlock/mlock)
45. [ ] Upload LittleFS, test full limit-finding workflow

### Session 6: Serial Terminal + Polish
46. [ ] Build Serial Terminal tab: scrolling output, command input, send button
47. [ ] Build Serial Terminal: clear button, auto-scroll toggle
48. [ ] Route all command acknowledgments and firmware responses to terminal
49. [ ] Test WebSocket + serial CLI coexistence
50. [ ] Add WiFi info to serial boot output (SSID, IP, client count)
51. [ ] Add connection indicator in web UI header
52. [ ] Add mDNS for `http://prism.local`
53. [ ] Test responsive layout on phone (Chrome Mobile, Safari)
54. [ ] Handle edge cases: multiple clients, rapid commands, reconnection
55. [ ] Optimize: minimize HTML/CSS/JS file sizes, tune WebSocket update rate

## Files to Create/Modify

### New Files
- [ ] `lib/WebServer/WebServer.h` — Web server class declaration
- [ ] `lib/WebServer/WebServer.cpp` — WiFi AP, AsyncWebServer, WebSocket, LittleFS, command handling
- [ ] `data/index.html` — Main HTML page (single-page app with 4 tabs)
- [ ] `data/style.css` — Modern dark theme responsive CSS
- [ ] `data/app.js` — Client-side JavaScript (WebSocket, DOM updates, all tab logic)

### Modified Files
- [ ] `platformio.ini` — Add lib_deps (ESPAsyncWebServer, AsyncTCP, ArduinoJson), LittleFS partition config
- [ ] `include/config.h` — Add WiFi AP settings (SSID, password, channel, max clients)
- [ ] `src/main.cpp` — Initialize WebServer, add status JSON builder, add WebSocket broadcast to loop, expose command handlers

## Testing Plan
- [ ] Connect phone/laptop to "LevelingPrism" WiFi, open 192.168.4.1
- [ ] **Dashboard tab**: verify live pitch/roll/motor data at ~10 Hz, bubble level moves, state badge updates
- [ ] **Test Mode tab — Motors**: move each motor via buttons, verify physical movement, test continuous mode
- [ ] **Test Mode tab — IMU**: scan, init, read, stream, calibrate all work
- [ ] **Test Mode tab — LED**: all patterns and colors light up correctly
- [ ] **Test Mode tab — System**: change PI gains, tolerance, stability timeout, verify with `s` command
- [ ] **Motor Limits tab**: move motors, capture IN/OUT limits, verify config values generated correctly
- [ ] **Motor Limits tab**: copy config values to clipboard, verify correct format
- [ ] **Serial Terminal tab**: type commands, see responses, clear works
- [ ] Test on mobile phone: verify all tabs usable on small screen
- [ ] Verify serial CLI still works alongside web interface
- [ ] Test with 2+ clients connected simultaneously

## Status
- **Created:** 2026-02-20
- **Planned:** 2026-02-20
- **Updated:** 2026-02-20 (expanded from 2 tabs to 4 tabs: Dashboard, Test Mode, Motor Limits, Serial Terminal)

## Notes
- ESP32 default AP IP: 192.168.4.1
- ESPAsyncWebServer runs on the WiFi core (core 0) — firmware runs on core 1, so they naturally don't block each other
- LittleFS upload command: `pio run -t uploadfs`
- ArduinoJson v7 recommended (latest, better API)
- WebSocket max message size: keep under 1KB for reliable delivery
- 10 Hz status updates = 100ms interval. Can reduce to 5 Hz if bandwidth is an issue.
- The web dashboard replaces the need for both Python GUIs (test_mode_gui.py and motor_limits_gui.py) when no USB connection is available
- Motor 2 direction reversal is handled client-side in the web UI (same approach as Python GUIs)
- Consider: add mDNS so user can navigate to `http://prism.local` instead of IP address
- Access Point mode means no internet on connected device — warn user in UI or docs
- Motor Limits tab stores IN/OUT values in browser memory (JavaScript) — not persisted on ESP32 until user updates config.h

## Session Log
