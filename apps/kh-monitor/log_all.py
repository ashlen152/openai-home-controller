
import asyncio
import serial_asyncio
import logging
import os
from dotenv import load_dotenv

load_dotenv()

PORT = os.getenv('UPLOAD_PORT')

if not PORT:
    raise ValueError("UPLOAD_PORT not set in environment")

BAUDRATE = 115200
RECONNECT_DELAY = 2

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)

async def read_serial():
    while True:
        try:
            logging.info(f"Connecting to {PORT}...")
            
            reader, writer = await serial_asyncio.open_serial_connection(
                url=PORT,
                baudrate=BAUDRATE
            )

            logging.info("Connected to serial port")

            while True:
                line = await reader.readline()

                if not line:
                    raise ConnectionError("Serial disconnected")

                try:
                    decoded = line.decode('utf-8').strip()
                except UnicodeDecodeError:
                    continue  # skip broken bytes safely

                if decoded:
                    print(decoded)

        except Exception as e:
            logging.warning(f"Serial error: {e}")
            logging.info(f"Reconnecting in {RECONNECT_DELAY}s...")
            await asyncio.sleep(RECONNECT_DELAY)

async def main():
    await read_serial()

if __name__ == "__main__":
    asyncio.run(main())
