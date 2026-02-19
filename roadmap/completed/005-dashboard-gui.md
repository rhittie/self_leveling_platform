# Feature: Dashboard GUI with Bubble Level

## Metadata
- **Priority:** Medium
- **Complexity:** Medium
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Added a Dashboard tab to the test mode GUI with a visual bubble level display and motor position bars. Parses IMU stream data and motor positions from serial output.

## Requirements
- [x] Bubble level visualization showing pitch/roll
- [x] Motor position bar graphs
- [x] Real-time serial data parsing ([IMU], P:R:, [MPOS] formats)
- [x] Dashboard as first/default tab

## Files Modified
- `tools/test_mode_gui.py` — Dashboard tab with bubble level and motor bars

## Status
- **Completed:** 2026-02-06
