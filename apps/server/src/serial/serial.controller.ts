import { Controller, Get, Post, OnModuleInit } from '@nestjs/common';
import { SerialService } from './serial.service';
import { EventEmitter2 } from '@nestjs/event-emitter';

@Controller('serial')
export class SerialController implements OnModuleInit {
  private logs: Array<{ timestamp: Date; message: string }> = [];
  private maxLogs = 500;

  constructor(
    private readonly serialService: SerialService,
    private readonly eventEmitter: EventEmitter2,
  ) {}

  async onModuleInit() {
    this.eventEmitter.on('esp32.log', (log: { timestamp: Date; message: string }) => {
      this.logs.push({ timestamp: log.timestamp, message: log.message });
      if (this.logs.length > this.maxLogs) {
        this.logs = this.logs.slice(-this.maxLogs);
      }
    });
  }

  @Get('status')
  getStatus() {
    return {
      connected: this.serialService.isSerialConnected(),
    };
  }

  @Post('connect')
  async connect() {
    const success = await this.serialService.connect(true);
    return { connected: success };
  }

  @Post('disconnect')
  disconnect() {
    this.serialService.disconnect();
    return { connected: false };
  }

  @Get('logs')
  getLogs() {
    if (!this.serialService.isSerialConnected()) {
      return {
        error: 'Serial not connected',
        connected: false,
        logs: [],
      };
    }
    return {
      connected: true,
      logs: this.logs,
    };
  }

  @Get('clear')
  clearLogs() {
    this.logs = [];
    return { cleared: true };
  }

  addLog(timestamp: Date, message: string) {
    this.logs.push({ timestamp, message });
    if (this.logs.length > this.maxLogs) {
      this.logs = this.logs.slice(-this.maxLogs);
    }
  }
}
