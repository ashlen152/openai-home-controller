import { Controller, Post, Body, Param } from '@nestjs/common';
import { CommandHandlersService } from '../services/command-handlers.service';
import { CalibrateCommandDto, SaveCalibrationCommandDto } from '../dto/command-payloads.dto';

@Controller('pump-commands/calibrate')
export class CalibrateController {
  constructor(private readonly commandHandlers: CommandHandlersService) {}

  @Post('start')
  async startCalibration(@Body() dto: CalibrateCommandDto) {
    return this.commandHandlers.handleCalibrate(dto);
  }

  @Post('save')
  async saveCalibration(@Body() dto: SaveCalibrationCommandDto) {
    return this.commandHandlers.handleSaveCalibration(dto);
  }
}
