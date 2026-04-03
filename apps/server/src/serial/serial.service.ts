import { Injectable, OnModuleInit, OnModuleDestroy } from '@nestjs/common';
import { EventEmitter2 } from '@nestjs/event-emitter';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';

@Injectable()
export class SerialService implements OnModuleInit, OnModuleDestroy {
  private port: SerialPort | null = null;
  private parser: ReadlineParser | null = null;
  private isConnected = false;
  private readonly SERIAL_PATH = '/dev/cu.usbserial-1130';
  private readonly BAUD_RATE = 115200;
  private readonly AUTO_DISCONNECT_TIMEOUT = 60000; // 60 seconds of inactivity
  private inactivityTimer: NodeJS.Timeout | null = null;
  private manualConnection = false;

  constructor(private eventEmitter: EventEmitter2) {}

  async onModuleInit() {
    // Don't auto-connect on startup - wait for explicit request
  }

  async onModuleDestroy() {
    this.disconnect();
  }

  isSerialConnected(): boolean {
    return this.isConnected;
  }

  async connect(manual = false): Promise<boolean> {
    if (this.isConnected) {
      this.manualConnection = this.manualConnection || manual;
      this.resetInactivityTimer();
      return true;
    }

    try {
      this.port = new SerialPort({
        path: this.SERIAL_PATH,
        baudRate: this.BAUD_RATE,
      });

      this.parser = this.port.pipe(new ReadlineParser({ delimiter: '\n' }));

      this.parser.on('data', (line: string) => {
        const trimmed = line.trim();
        if (trimmed) {
          this.eventEmitter.emit('esp32.log', {
            timestamp: new Date(),
            message: trimmed,
          });
          this.resetInactivityTimer();
        }
      });

      this.port.on('open', () => {
        console.log('[SerialService] ESP32 connected');
        this.isConnected = true;
        this.manualConnection = manual;
        this.resetInactivityTimer();
      });

      this.port.on('close', () => {
        console.log('[SerialService] ESP32 disconnected');
        this.isConnected = false;
        this.clearInactivityTimer();
      });

      this.port.on('error', (err) => {
        console.error('[SerialService] Error:', err.message);
        this.isConnected = false;
      });

      return true;
    } catch (error) {
      console.error('[SerialService] Failed to connect:', error);
      this.isConnected = false;
      return false;
    }
  }

  private resetInactivityTimer(): void {
    this.clearInactivityTimer();
    if (!this.manualConnection) {
      this.inactivityTimer = setTimeout(() => {
        console.log('[SerialService] Auto-disconnecting due to inactivity');
        this.disconnect();
      }, this.AUTO_DISCONNECT_TIMEOUT);
    }
  }

  private clearInactivityTimer(): void {
    if (this.inactivityTimer) {
      clearTimeout(this.inactivityTimer);
      this.inactivityTimer = null;
    }
  }

  disconnect(): void {
    this.clearInactivityTimer();
    if (this.port && this.port.isOpen) {
      this.port.close();
      this.port = null;
      this.parser = null;
      this.isConnected = false;
      this.manualConnection = false;
    }
  }
}
