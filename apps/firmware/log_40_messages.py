import serial
import time
ser = serial.Serial('/dev/cu.usbserial-1130', 115200, timeout=1)
time.sleep(2)
for i in range(40):
    if ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(line)
    time.sleep(0.5)
ser.close()

