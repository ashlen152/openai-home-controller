import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';

export type MangaDocument = Manga & Document;

@Schema({ timestamps: true })
export class Manga {
  @Prop({ required: true, unique: true })
  externalId: string;

  @Prop({ required: true })
  title: string;

  @Prop()
  coverImage: string;

  @Prop()
  url: string;

  @Prop()
  description: string;

  @Prop({ required: true })
  source: string;

  @Prop({ default: false })
  isBookmarked: boolean;

  @Prop()
  lastCheckedAt: Date;

  @Prop()
  lastChapterAt: Date;

  @Prop({ default: 0 })
  latestChapter: number;

  @Prop({ default: 0 })
  currentChapter: number;

  @Prop({ default: false })
  hasNewChapters: boolean;
}

export const MangaSchema = SchemaFactory.createForClass(Manga);
MangaSchema.index({ source: 1, isBookmarked: 1 });
