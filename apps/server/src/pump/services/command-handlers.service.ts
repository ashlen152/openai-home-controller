import { Injectable, NotFoundException, BadRequestException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { PumpCommand, PumpCommandDocument } from '../schemas/pump-command.schema';
import { PumpSettingsService } from './pump-settings.service';
import {
  CalibrateCommandDto,
  TestDoseCommandDto,
  SaveCalibrationCommandDto,
} from '../dto/command-payloads.dto';

@Injectable()
export class CommandHandlersService {
  constructor(
    @InjectModel(PumpCommand.name) private pumpCommandModel: Model<PumpCommandDocument>,
    private pumpSettingsService: PumpSettingsService,
  ) {}

  async handleCalibrate(
    dto: CalibrateCommandDto,
  ): Promise<{ success: boolean; commandId: string; message: string }> {
    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const volume = dto.volume || 5.0;

    const command = new this.pumpCommandModel({
      pumpId: dto.pumpId,
      commandId,
      command: 'CALIBRATE',
      status: 'pending',
      payload: { volume },
    });

    await command.save();

    return {
      success: true,
      commandId,
      message: `CALIBRATE command queued for pump ${dto.pumpId} (${volume}ml)`,
    };
  }

  async handleTestDose(
    dto: TestDoseCommandDto,
  ): Promise<{ success: boolean; commandId: string; message: string }> {
    const settings = await this.pumpSettingsService.getSettings(dto.pumpId);
    let steps = 0;
    let speed = dto.speed || 2000;

    if (settings && settings.stepsPerML > 0) {
      steps = Math.round(settings.stepsPerML * dto.volume);
      speed = Math.max(1500, Math.min(5000, Math.round(settings.stepsPerML * 2)));
    } else {
      throw new BadRequestException(
        `Pump ${dto.pumpId} has no calibration. Please calibrate first.`,
      );
    }

    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const command = new this.pumpCommandModel({
      pumpId: dto.pumpId,
      commandId,
      command: 'TEST_DOSE',
      status: 'pending',
      payload: { steps, speed, volume: dto.volume },
    });

    await command.save();

    return {
      success: true,
      commandId,
      message: `TEST_DOSE command queued for pump ${dto.pumpId} (${dto.volume}ml)`,
    };
  }

  async handleSaveCalibration(
    dto: SaveCalibrationCommandDto,
  ): Promise<{ success: boolean; commandId: string; message: string }> {
    const calibrationCmd = await this.pumpCommandModel
      .findOne({
        pumpId: dto.pumpId,
        command: 'CALIBRATE',
        status: 'completed',
      })
      .sort({ completedAt: -1 })
      .exec();

    let actualSteps = 0;
    if (calibrationCmd) {
      actualSteps =
        (calibrationCmd as any).stepsCompleted ?? (calibrationCmd as any).payload?.steps ?? 0;
    } else {
      const settings = await this.pumpSettingsService.getSettings(dto.pumpId);
      if (settings && settings.stepsPerML > 0) {
        actualSteps = Math.round(settings.stepsPerML * 5);
      } else {
        actualSteps = 38170;
      }
    }

    const stepsPerML = Math.round(actualSteps / dto.measuredML);

    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const command = new this.pumpCommandModel({
      pumpId: dto.pumpId,
      commandId,
      command: 'SAVE_CALIBRATION',
      status: 'pending',
      payload: { stepsPerML, measuredML: dto.measuredML, actualSteps },
    });

    await command.save();

    return {
      success: true,
      commandId,
      message: `SAVE_CALIBRATION command sent (${stepsPerML} steps/ml)`,
    };
  }

  async completeCommand(
    pumpId: string,
    commandId: string,
    status: string,
    stepsCompleted?: number,
    error?: string,
  ): Promise<{ success: boolean; message: string }> {
    const command = await this.pumpCommandModel.findOneAndUpdate(
      { commandId, pumpId },
      {
        status,
        stepsCompleted,
        completedAt: new Date(),
        error,
      },
      { new: true },
    );

    if (!command) {
      throw new NotFoundException(`Command ${commandId} not found`);
    }

    if (command.command === 'CALIBRATE' && status === 'completed') {
      await this.handleCalibrationComplete(pumpId, stepsCompleted);
    }

    return {
      success: true,
      message: `Command ${commandId} marked as ${status}`,
    };
  }

  private async handleCalibrationComplete(pumpId: string, stepsCompleted?: number): Promise<void> {
    const saveCalCmd = await this.pumpCommandModel
      .findOne({ pumpId, command: 'SAVE_CALIBRATION', status: 'pending' })
      .sort({ createdAt: -1 })
      .exec();

    if (saveCalCmd && saveCalCmd.payload?.stepsPerML) {
      const currentSettings = await this.pumpSettingsService.getSettings(pumpId);
      if (currentSettings) {
        await this.pumpSettingsService.upsertSettings({
          pumpId: currentSettings.pumpId,
          enabled: currentSettings.enabled,
          dailyVolume: currentSettings.dailyVolume,
          dayStartHour: currentSettings.dayStartHour,
          dayEndHour: currentSettings.dayEndHour,
          dayPercent: currentSettings.dayPercent,
          stepsPerML: saveCalCmd.payload.stepsPerML,
          activeProfile: currentSettings.activeProfile,
          pausedUntil: currentSettings.pausedUntil,
        });
      }
    }
  }
}
