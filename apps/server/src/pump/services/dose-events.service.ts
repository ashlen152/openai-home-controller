import { Injectable, BadRequestException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { DoseEvent, DoseEventDocument } from '../schemas/dose-event.schema';
import { CreateDoseEventDto } from '../dto/create-dose-event.dto';
import { DoseEventStatus } from '../schemas/dose-event.schema';

@Injectable()
export class DoseEventsService {
  constructor(@InjectModel(DoseEvent.name) private doseEventModel: Model<DoseEventDocument>) {}

  async logDoseEvent(dto: CreateDoseEventDto): Promise<DoseEvent> {
    // Validate success based on status
    if (dto.status === DoseEventStatus.STARTED) {
      if (dto.success !== null) {
        throw new BadRequestException('For started events, success must be null');
      }
    } else if (dto.status === DoseEventStatus.COMPLETED || dto.status === DoseEventStatus.FAILED) {
      if (typeof dto.success !== 'boolean') {
        throw new BadRequestException('For completed/failed events, success must be boolean');
      }
    }

    const created = new this.doseEventModel(dto);
    return created.save();
  }

  async getDoseHistory(pumpId: string): Promise<DoseEvent[]> {
    return this.doseEventModel.find({ pumpId }).sort({ timestamp: -1 }).exec();
  }

  async getTodaysDoses(pumpId: string): Promise<DoseEvent[]> {
    const startOfDay = new Date();
    startOfDay.setHours(0, 0, 0, 0);
    const endOfDay = new Date();
    endOfDay.setHours(23, 59, 59, 999);

    return this.doseEventModel
      .find({
        pumpId,
        timestamp: {
          $gte: startOfDay.getTime() / 1000,
          $lte: endOfDay.getTime() / 1000,
        },
      })
      .sort({ timestamp: -1 })
      .exec();
  }
}
