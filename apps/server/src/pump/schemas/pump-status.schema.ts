import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';

export type PumpStatusDocument = PumpStatus & Document;

@Schema({ timestamps: true })
export class PumpStatus {
  @Prop({ required: true, index: true })
  pumpId: string;

  // Connection state
  @Prop({ required: true, default: false })
  online: boolean;

  @Prop({ required: true })
  lastHeartbeat: number;

  // WiFi info
  @Prop({ default: -1 })
  wifiRssi: number;

  @Prop()
  ipAddress: string;

  // Runtime state
  @Prop({ default: false })
  isDosing: boolean;

  @Prop({ default: 0 })
  totalDosedToday: number;

  // ESP32-reported settings snapshot (what the firmware THINKS it has)
  @Prop()
  reportedEnabled: boolean;

  @Prop()
  reportedDailyVolume: number;

  @Prop()
  reportedDayStartHour: number;

  @Prop()
  reportedDayEndHour: number;

  @Prop()
  reportedDayPercent: number;

  @Prop()
  reportedStepsPerML: number;

  @Prop()
  reportedActiveProfile: number;

  @Prop()
  reportedPausedUntil: number;

  // When the firmware last synced settings from server
  @Prop()
  lastSettingsSync: number;

  // Uptime
  @Prop()
  uptimeSeconds: number;

  // Free heap memory
  @Prop()
  freeHeap: number;
}

export const PumpStatusSchema = SchemaFactory.createForClass(PumpStatus);
PumpStatusSchema.index({ pumpId: 1, lastHeartbeat: -1 });
