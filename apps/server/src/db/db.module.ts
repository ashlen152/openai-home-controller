import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { Manga, MangaSchema, Episode, EpisodeSchema } from './schemas';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: Manga.name, schema: MangaSchema },
      { name: Episode.name, schema: EpisodeSchema },
    ]),
  ],
  exports: [MongooseModule],
})
export class DbModule {}
