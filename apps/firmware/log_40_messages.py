import serial
import time
import os
from dotenv import load_dotenv

load_dotenv()

PORT = os.getenv('UPLOAD_PORT')

if not PORT:
    raise ValueError("UPLOAD_PORT not set in environment")


ser = serial.Serial(PORT, 115200, timeout=1)
time.sleep(2)
for i in range(40):
    if ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(line)
    time.sleep(0.5)
ser.close()

