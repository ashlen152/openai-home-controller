import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document, Types } from 'mongoose';

export type EpisodeDocument = Episode & Document;

@Schema({ timestamps: true })
export class Episode {
  @Prop({ type: Types.ObjectId, ref: 'Manga', required: true })
  mangaId: Types.ObjectId;

  @Prop({ required: true })
  externalId: string;

  @Prop({ required: true })
  title: string;

  @Prop({ required: true })
  number: number;

  @Prop()
  url: string;

  @Prop({ type: [String] })
  imageUrls: string[];

  @Prop()
  publishedAt: Date;

  @Prop({ type: [Number], select: false })
  embeddings: number[];

  @Prop({ default: 'unread' })
  readStatus: 'unread' | 'reading' | 'read';

  @Prop({ default: false })
  isNew: boolean;

  @Prop()
  crawledAt: Date;
}

export const EpisodeSchema = SchemaFactory.createForClass(Episode);
EpisodeSchema.index({ mangaId: 1, number: -1 });
EpisodeSchema.index({ externalId: 1 }, { unique: true });
