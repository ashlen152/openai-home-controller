import { Injectable, NotFoundException, BadRequestException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { PumpCommand, PumpCommandDocument } from '../schemas/pump-command.schema';
import { PumpSettingsService } from './pump-settings.service';
import { CreatePumpCommandDto } from '../dto/create-pump-command.dto';
import { CompletePumpCommandDto } from '../dto/complete-pump-command.dto';
import { CalculateStepsDto } from '../dto/calculate-steps.dto';
import { SaveCalibrationCommandDto } from '../dto/save-calibration.dto';

@Injectable()
export class PumpCommandsService {
  constructor(
    @InjectModel(PumpCommand.name) private pumpCommandModel: Model<PumpCommandDocument>,
    private pumpSettingsService: PumpSettingsService,
  ) {}

  async createCommand(
    dto: CreatePumpCommandDto,
  ): Promise<{ success: boolean; commandId: string; message: string }> {
    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

    // For TEST_DOSE or new payload-based commands, use payload if provided
    let payload: any = dto.payload || undefined;
    // Local compatibility: expose legacy variables for this flow
    let steps: number = dto.steps ?? 200000;
    let speed: number = dto.speed ?? 0;
    if (!payload) {
      // Backward compatibility: build payload from legacy fields if present
      payload = {
        steps: steps,
        speed: speed,
      };
    }

    if (dto.command === 'TEST_DOSE' && dto.pumpId) {
      try {
        const settings = await this.pumpSettingsService.getSettings(dto.pumpId);
        if (settings && settings.stepsPerML > 0) {
          const volume = dto.metadata?.testVolume || 1.0;
          steps = Math.round(settings.stepsPerML * volume);

          speed = Math.max(1500, Math.min(5000, Math.round(settings.stepsPerML * 2)));
          payload = { steps, speed };
        }
      } catch (error) {
        console.log('Failed to fetch pump settings for TEST_DOSE:', error.message);
      }
    }

    const command = new this.pumpCommandModel({
      pumpId: dto.pumpId,
      commandId,
      command: dto.command,
      status: 'pending',
      payload: payload,
    });

    await command.save();

    return {
      success: true,
      commandId,
      message: `${dto.command} command queued for pump ${dto.pumpId}`,
    };
  }

  async getPendingCommands(
    pumpId: string,
  ): Promise<{ pumpId: string; pendingCommands: PumpCommand[] }> {
    const commands = await this.pumpCommandModel
      .find({ pumpId, status: 'pending' })
      .sort({ createdAt: 1 })
      .exec();

    return {
      pumpId,
      pendingCommands: commands,
    };
  }

  async completeCommand(
    pumpId: string,
    dto: CompletePumpCommandDto,
  ): Promise<{ success: boolean; message: string }> {
    const command = await this.pumpCommandModel.findOneAndUpdate(
      { commandId: dto.commandId, pumpId },
      {
        status: dto.status,
        stepsCompleted: dto.stepsCompleted,
        completedAt: new Date(),
        error: dto.error,
      },
      { new: true },
    );

    if (!command) {
      throw new NotFoundException(`Command ${dto.commandId} not found`);
    }

    if (command.command === 'CALIBRATE' && dto.status === 'completed') {
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

    return {
      success: true,
      message: `Command ${dto.commandId} marked as ${dto.status}`,
    };
  }

  async calculateSteps(dto: CalculateStepsDto): Promise<{
    success: boolean;
    stepsPerML: number;
    previousStepsPerML: number;
    message: string;
    calculation: string;
  }> {
    const settings = await this.pumpSettingsService.getSettings(dto.pumpId);
    const previousStepsPerML = settings?.stepsPerML || 0;

    const stepsPerML = this.calculateStepsPerML(dto.steps, dto.measuredML);

    if (dto.applyToPump) {
      await this.pumpSettingsService.upsertSettings({
        pumpId: dto.pumpId,
        enabled: settings?.enabled ?? true,
        dailyVolume: settings?.dailyVolume ?? 30,
        dayStartHour: settings?.dayStartHour ?? 6,
        dayEndHour: settings?.dayEndHour ?? 22,
        dayPercent: settings?.dayPercent ?? 70,
        stepsPerML,
        activeProfile: settings?.activeProfile ?? 1,
        pausedUntil: settings?.pausedUntil ?? 0,
      });

      await this.pumpSettingsService.addCalibrationHistory(dto.pumpId, {
        timestamp: Date.now(),
        steps: dto.steps,
        measuredML: dto.measuredML,
        stepsPerML,
      });
    }

    return {
      success: true,
      stepsPerML,
      previousStepsPerML,
      message: `Calculated: ${dto.steps} steps / ${dto.measuredML} mL = ${stepsPerML.toFixed(2)} steps/mL`,
      calculation: `${dto.steps} / ${dto.measuredML} = ${stepsPerML.toFixed(2)}`,
    };
  }

  async getCalibrationHistory(pumpId: string): Promise<{
    pumpId: string;
    calibrations: Array<{
      timestamp: number;
      steps: number;
      measuredML: number;
      stepsPerML: number;
    }>;
  }> {
    const settings = await this.pumpSettingsService.getSettings(pumpId);
    return {
      pumpId,
      calibrations: (settings as any)?.calibrationHistory || [],
    };
  }

  private calculateStepsPerML(steps: number, measuredML: number): number {
    if (measuredML <= 0) {
      throw new BadRequestException('Measured volume must be greater than 0');
    }

    const stepsPerML = steps / measuredML;

    if (stepsPerML < 100 || stepsPerML > 50000) {
      throw new BadRequestException(
        `Calculated steps/mL (${stepsPerML.toFixed(2)}) is outside expected range (100-50000). Please verify your measurement.`,
      );
    }

    return Math.round(stepsPerML * 100) / 100;
  }

  async queueSaveSettings(
    pumpId: string,
  ): Promise<{ success: boolean; commandId: string; message: string }> {
    const settings = await this.pumpSettingsService.getSettings(pumpId);
    if (!settings) {
      throw new NotFoundException(`Pump settings not found for pumpId: ${pumpId}`);
    }

    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

    const command = new this.pumpCommandModel({
      pumpId,
      commandId,
      command: 'SAVE_SETTINGS',
      status: 'pending',
      payload: {
        enabled: settings.enabled,
        dailyVolume: settings.dailyVolume,
        dayStartHour: settings.dayStartHour,
        dayEndHour: settings.dayEndHour,
        dayPercent: settings.dayPercent,
        stepsPerML: settings.stepsPerML,
        activeProfile: settings.activeProfile,
        pausedUntil: settings.pausedUntil,
      },
    });

    await command.save();

    return {
      success: true,
      commandId,
      message: `SAVE_SETTINGS command queued for pump ${pumpId}`,
    };
  }

  async saveCalibration(
    pumpId: string,
    measuredML: number,
  ): Promise<{ success: boolean; message: string; stepsPerML: number }> {
    const calibrationCmd = await this.pumpCommandModel
      .findOne({
        pumpId,
        command: 'CALIBRATE',
        status: 'completed',
      })
      .sort({ completedAt: -1 })
      .exec();

    if (!calibrationCmd) {
      throw new NotFoundException('No completed calibration found. Start calibration first.');
    }

    const actualSteps =
      (calibrationCmd as any).stepsCompleted ?? (calibrationCmd as any).payload?.steps ?? 0;
    const stepsPerML = this.calculateStepsPerML(actualSteps, measuredML);

    await this.pumpSettingsService.upsertSettings({
      pumpId,
      enabled: true,
      dailyVolume: 30,
      dayStartHour: 8,
      dayEndHour: 20,
      dayPercent: 70,
      stepsPerML,
      activeProfile: 1,
    });

    const cmdMetadata = (calibrationCmd as any).toObject?.()?.metadata || {};
    await this.pumpCommandModel.findByIdAndUpdate(calibrationCmd._id, {
      status: 'saved',
      metadata: { ...cmdMetadata, measuredML, stepsPerML },
    });

    return {
      success: true,
      message: `Calibration saved: ${stepsPerML.toFixed(2)} steps/ml`,
      stepsPerML,
    };
  }

  async saveCalibrationWithCommand(
    pumpId: string,
    measuredML: number,
  ): Promise<{ success: boolean; message: string; stepsPerML: number }> {
    const calibrationCmd = await this.pumpCommandModel
      .findOne({
        pumpId,
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
      const settings = await this.pumpSettingsService.getSettings(pumpId);
      if (settings && settings.stepsPerML > 0) {
        actualSteps = Math.round(settings.stepsPerML * 5);
      } else {
        actualSteps = 38170;
      }
    }

    const stepsPerML = Math.round(actualSteps / measuredML);

    const commandId = `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const command = new this.pumpCommandModel({
      pumpId,
      commandId,
      command: 'SAVE_CALIBRATION',
      status: 'pending',
      payload: { stepsPerML },
    });
    await command.save();

    return {
      success: true,
      message: `SAVE_CALIBRATION command sent (${stepsPerML} steps/ml)`,
      stepsPerML,
    };
  }
}
