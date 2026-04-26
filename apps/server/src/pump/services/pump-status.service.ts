import { Injectable } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { PumpStatus, PumpStatusDocument } from '../schemas/pump-status.schema';
import { ReportPumpStatusDto } from '../dto/report-pump-status.dto';

@Injectable()
export class PumpStatusService {
  constructor(@InjectModel(PumpStatus.name) private pumpStatusModel: Model<PumpStatusDocument>) {}

  async reportStatus(dto: ReportPumpStatusDto): Promise<PumpStatus> {
    const existing = await this.pumpStatusModel.findOne({ pumpId: dto.pumpId }).exec();

    if (existing) {
      Object.assign(existing, dto);
      return existing.save();
    }

    const created = new this.pumpStatusModel(dto);
    return created.save();
  }

  async getStatus(pumpId: string): Promise<PumpStatus | null> {
    return this.pumpStatusModel.findOne({ pumpId }).exec();
  }

  async getAllStatus(): Promise<PumpStatus[]> {
    return this.pumpStatusModel.find().exec();
  }

  async markOffline(pumpId: string): Promise<void> {
    await this.pumpStatusModel.updateOne({ pumpId }, { online: false }).exec();
  }
}
