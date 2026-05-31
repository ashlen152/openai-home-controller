import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';

export type PumpSettingDocument = PumpSetting & Document;

@Schema({ timestamps: true })
export class PumpSetting {
  @Prop({ required: true, unique: true })
  pumpId: string;

  @Prop({ required: true })
  enabled: boolean;

  @Prop({ required: true })
  dailyVolume: number;

  @Prop({ required: true, min: 0, max: 23 })
  dayStartHour: number;

  @Prop({ required: true, min: 0, max: 23 })
  dayEndHour: number;

  @Prop({ required: true, min: 0, max: 100 })
  dayPercent: number;

  @Prop({ required: true, min: 1, max: 288 })
  scheduleSlots: number;

  @Prop({ required: true })
  stepsPerML: number;

  @Prop({ required: true, min: 0, max: 2 })
  activeProfile: number;

  @Prop({ required: true, default: 0 })
  pausedUntil: number;

  @Prop({ required: true })
  lastSync: number;

  @Prop({
    type: [{ timestamp: Number, steps: Number, measuredML: Number, stepsPerML: Number }],
    default: [],
  })
  calibrationHistory: Array<{
    timestamp: number;
    steps: number;
    measuredML: number;
    stepsPerML: number;
  }>;
}

export const PumpSettingSchema = SchemaFactory.createForClass(PumpSetting);
