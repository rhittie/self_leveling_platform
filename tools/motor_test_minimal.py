"""Minimal motor test - sends m2 commands via pyserial to isolate GUI issue."""
import serial
import time

port = "COM4"
print(f"Opening {port}...")
ser = serial.Serial(port, 115200, timeout=1)

# Wait for ESP32 boot
time.sleep(3)
# Drain boot messages
while ser.in_waiting:
    print(f"  boot: {ser.readline().decode(errors='replace').strip()}")

# Enter test mode
print("\n>> test")
ser.write(b"test\n")
time.sleep(2)
while ser.in_waiting:
    print(f"  {ser.readline().decode(errors='replace').strip()}")

# Unlock limits
print("\n>> munlock")
ser.write(b"munlock\n")
time.sleep(0.5)
while ser.in_waiting:
    print(f"  {ser.readline().decode(errors='replace').strip()}")

# Test motor 2 three times
for i in range(3):
    input(f"\nPress ENTER to send 'm2 500' (attempt {i+1}/3)...")
    print(">> m2 500")
    ser.write(b"m2 500\n")
    time.sleep(2)
    while ser.in_waiting:
        print(f"  {ser.readline().decode(errors='replace').strip()}")

print("\nDone. Closing port.")
ser.close()
