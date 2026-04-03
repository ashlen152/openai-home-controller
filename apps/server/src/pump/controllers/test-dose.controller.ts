import { Controller, Post, Body, Param } from '@nestjs/common';
import { CommandHandlersService } from '../services/command-handlers.service';
import { TestDoseCommandDto } from '../dto/command-payloads.dto';

@Controller('pump-commands/test-dose')
export class TestDoseController {
  constructor(private readonly commandHandlers: CommandHandlersService) {}

  @Post()
  async testDose(@Body() dto: TestDoseCommandDto) {
    return this.commandHandlers.handleTestDose(dto);
  }

  @Post(':pumpId/:volume')
  async testDoseByVolume(
    @Param('pumpId') pumpId: string,
    @Param('volume') volume: string,
    @Body() body: { speed?: number },
  ) {
    const vol = parseFloat(volume);
    if (isNaN(vol) || vol <= 0) {
      throw new Error('Invalid volume');
    }
    return this.commandHandlers.handleTestDose({
      pumpId,
      volume: vol,
      speed: body?.speed,
    });
  }
}
