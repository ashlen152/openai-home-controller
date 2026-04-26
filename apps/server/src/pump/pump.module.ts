import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { PumpSettingsController } from './controllers/pump-settings.controller';
import { PumpSettingsService } from './services/pump-settings.service';
import { DoseEventsController } from './controllers/dose-events.controller';
import { DoseEventsService } from './services/dose-events.service';
import { PumpCommandsController } from './controllers/pump-commands.controller';
import { PumpCommandsService } from './services/pump-commands.service';
import { PumpStatusController } from './controllers/pump-status.controller';
import { PumpStatusService } from './services/pump-status.service';
import { CalibrateController } from './controllers/calibrate.controller';
import { TestDoseController } from './controllers/test-dose.controller';
import { CommandHandlersService } from './services/command-handlers.service';
import { PumpSettingSchema } from './schemas/pump-setting.schema';
import { DoseEventSchema } from './schemas/dose-event.schema';
import { PumpCommandSchema } from './schemas/pump-command.schema';
import { PumpStatusSchema } from './schemas/pump-status.schema';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: 'PumpSetting', schema: PumpSettingSchema },
      { name: 'DoseEvent', schema: DoseEventSchema },
      { name: 'PumpCommand', schema: PumpCommandSchema },
      { name: 'PumpStatus', schema: PumpStatusSchema },
    ]),
  ],
  controllers: [
    PumpSettingsController,
    DoseEventsController,
    PumpCommandsController,
    PumpStatusController,
    CalibrateController,
    TestDoseController,
  ],
  providers: [
    PumpSettingsService,
    DoseEventsService,
    PumpCommandsService,
    PumpStatusService,
    CommandHandlersService,
  ],
  exports: [
    PumpSettingsService,
    DoseEventsService,
    PumpCommandsService,
    PumpStatusService,
    CommandHandlersService,
  ],
})
export class PumpModule {}
