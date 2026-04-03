import { Controller, Get, Post, Body, Param } from '@nestjs/common';
import { PumpCommandsService } from '../services/pump-commands.service';
import { CreatePumpCommandDto } from '../dto/create-pump-command.dto';
import { CompletePumpCommandDto } from '../dto/complete-pump-command.dto';
import { CalculateStepsDto } from '../dto/calculate-steps.dto';

@Controller('pump-commands')
export class PumpCommandsController {
  constructor(private readonly pumpCommandsService: PumpCommandsService) {}

  @Post()
  async createCommand(@Body() dto: CreatePumpCommandDto) {
    return this.pumpCommandsService.createCommand(dto);
  }

  @Get(':pumpId')
  async getPendingCommands(@Param('pumpId') pumpId: string) {
    return this.pumpCommandsService.getPendingCommands(pumpId);
  }

  @Post(':pumpId/complete')
  async completeCommand(@Param('pumpId') pumpId: string, @Body() dto: CompletePumpCommandDto) {
    return this.pumpCommandsService.completeCommand(pumpId, dto);
  }

  @Post('calculate-steps')
  async calculateSteps(@Body() dto: CalculateStepsDto) {
    return this.pumpCommandsService.calculateSteps(dto);
  }

  @Get(':pumpId/calibration-history')
  async getCalibrationHistory(@Param('pumpId') pumpId: string) {
    return this.pumpCommandsService.getCalibrationHistory(pumpId);
  }

  // Test dosing endpoint
  @Post(':pumpId/test-dose/:volume')
  async testDose(
    @Param('pumpId') pumpId: string,
    @Param('volume') volume: string,
    @Body() body: { speed?: number },
  ) {
    const vol = parseFloat(volume);
    if (isNaN(vol) || vol <= 0) {
      throw new Error('Invalid volume');
    }
    return this.pumpCommandsService.createCommand({
      pumpId,
      command: 'TEST_DOSE',
      steps: 0,
      speed: body?.speed || 0,
      metadata: { testVolume: vol },
    });
  }

  // Calibration endpoint - start 5ml calibration
  @Post(':pumpId/calibrate/start')
  async startCalibration(@Param('pumpId') pumpId: string) {
    return this.pumpCommandsService.createCommand({
      pumpId,
      command: 'CALIBRATE',
      steps: 0,
      speed: 0,
      metadata: { calibrationVolume: 5.0 },
    });
  }

  // Calibration endpoint - save with measured ml
  @Post(':pumpId/calibrate/save')
  async saveCalibration(@Param('pumpId') pumpId: string, @Body() body: { measuredML: number }) {
    const measuredML =
      typeof body.measuredML === 'string' ? parseFloat(body.measuredML) : body.measuredML;
    if (isNaN(measuredML) || measuredML <= 0) {
      throw new Error('Invalid measured ml');
    }
    return this.pumpCommandsService.saveCalibrationWithCommand(pumpId, measuredML);
  }
}
