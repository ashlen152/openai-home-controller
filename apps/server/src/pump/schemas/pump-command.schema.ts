import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';

export type PumpCommandDocument = PumpCommand & Document;

@Schema({ timestamps: true })
export class PumpCommand {
  @Prop({ required: true, index: true })
  pumpId: string;

  @Prop({ required: true, unique: true })
  commandId: string;

  @Prop({
    required: true,
    enum: ['CALIBRATE', 'RESET', 'PAUSE', 'RESUME', 'TEST_DOSE', 'SAVE_CALIBRATION', 'SAVE_SETTINGS'],
  })
  command: string;

  @Prop({ type: Object, default: {} })
  payload: Record<string, any>;

  @Prop({ enum: ['pending', 'in_progress', 'completed', 'failed'], default: 'pending' })
  status: string;

  @Prop()
  stepsCompleted: number;

  @Prop()
  completedAt: Date;

  @Prop()
  error: string;
}

export const PumpCommandSchema = SchemaFactory.createForClass(PumpCommand);
