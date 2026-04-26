import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';

export enum DoseEventStatus {
  STARTED = 'started',
  COMPLETED = 'completed',
  FAILED = 'failed',
}

export type DoseEventDocument = DoseEvent & Document;

@Schema({ timestamps: true })
export class DoseEvent {
  @Prop({ required: true })
  pumpId: string;

  @Prop({ required: true })
  eventId: string;

  @Prop({ required: true })
  timestamp: number;

  @Prop({ required: false, type: Number, nullable: true })
  dosingTimestamp?: number | null;

  @Prop({ required: true })
  volume: number;

  @Prop({ required: true, enum: DoseEventStatus })
  status: string;

  @Prop({ required: false, type: Boolean, nullable: true })
  success?: boolean | null;

  @Prop({ required: false, type: Object })
  metadata?: Record<string, any>;
}

export const DoseEventSchema = SchemaFactory.createForClass(DoseEvent);
DoseEventSchema.index({ pumpId: 1, timestamp: -1 });
DoseEventSchema.index({ pumpId: 1, eventId: 1 });
