import { Controller, Get, Param } from '@nestjs/common';
import { PumpStatusService } from '../services/pump-status.service';

// Read-only pump status endpoints. POST /api/pump-status merged into POST /api/pump-settings/report/:pumpId
@Controller('pump-status')
export class PumpStatusController {
  constructor(private readonly pumpStatusService: PumpStatusService) {}

  @Get(':pumpId')
  async getStatus(@Param('pumpId') pumpId: string) {
    return this.pumpStatusService.getStatus(pumpId);
  }

  @Get()
  async getAllStatus() {
    return this.pumpStatusService.getAllStatus();
  }
}
