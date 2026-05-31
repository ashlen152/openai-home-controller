import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { PumpSetting, PumpSettingDocument } from '../schemas/pump-setting.schema';
import { CreatePumpSettingsDto } from '../dto/create-pump-settings.dto';
import { UpdatePumpSettingsDto } from '../dto/update-pump-settings.dto';

interface CalibrationRecord {
  timestamp: number;
  steps: number;
  measuredML: number;
  stepsPerML: number;
}

@Injectable()
export class PumpSettingsService {
  constructor(
    @InjectModel(PumpSetting.name) private pumpSettingModel: Model<PumpSettingDocument>,
  ) {}

  async getSettings(pumpId: string): Promise<PumpSetting | null> {
    return this.pumpSettingModel.findOne({ pumpId }).exec();
  }

  async getAllPumps(): Promise<PumpSetting[]> {
    return this.pumpSettingModel.find().exec();
  }

  async upsertSettings(dto: UpdatePumpSettingsDto): Promise<PumpSetting> {
    const existing = await this.pumpSettingModel.findOne({ pumpId: dto.pumpId }).exec();
    const now = Date.now();

    if (existing) {
      // Merge only provided fields, preserve existing values for optional fields
      const updateData: Partial<PumpSetting> = {};
      if (dto.enabled !== undefined) updateData.enabled = dto.enabled;
      if (dto.dailyVolume !== undefined) updateData.dailyVolume = dto.dailyVolume;
      if (dto.dayStartHour !== undefined) updateData.dayStartHour = dto.dayStartHour;
      if (dto.dayEndHour !== undefined) updateData.dayEndHour = dto.dayEndHour;
      if (dto.dayPercent !== undefined) updateData.dayPercent = dto.dayPercent;
      if (dto.scheduleSlots !== undefined) updateData.scheduleSlots = dto.scheduleSlots;
      if (dto.stepsPerML !== undefined) updateData.stepsPerML = dto.stepsPerML;
      if (dto.activeProfile !== undefined) updateData.activeProfile = dto.activeProfile;
      if (dto.pausedUntil !== undefined) updateData.pausedUntil = dto.pausedUntil;

      Object.assign(existing, updateData);
      existing.lastSync = now;
      return existing.save();
    } else {
      // Create new - all required fields must be present
      const createDto = dto as CreatePumpSettingsDto;
      const created = new this.pumpSettingModel({
        pumpId: createDto.pumpId,
        enabled: createDto.enabled ?? true,
        dailyVolume: createDto.dailyVolume ?? 30,
        dayStartHour: createDto.dayStartHour ?? 8,
        dayEndHour: createDto.dayEndHour ?? 20,
        dayPercent: createDto.dayPercent ?? 70,
        scheduleSlots: createDto.scheduleSlots ?? 24,
        stepsPerML: createDto.stepsPerML ?? 17730,
        activeProfile: createDto.activeProfile ?? 1,
        pausedUntil: createDto.pausedUntil ?? 0,
        lastSync: now,
      });
      return created.save();
    }
  }

  async deleteSettings(pumpId: string): Promise<boolean> {
    const result = await this.pumpSettingModel.deleteOne({ pumpId }).exec();
    return result.deletedCount === 1;
  }

  async addCalibrationHistory(pumpId: string, record: CalibrationRecord): Promise<void> {
    await this.pumpSettingModel.updateOne(
      { pumpId },
      {
        $push: { calibrationHistory: { $each: [record], $position: 0 } },
      },
    );
  }
}
