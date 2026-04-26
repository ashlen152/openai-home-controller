import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { MangasController, EpisodesController } from './mangas.controller';
import { Manga, MangaSchema } from '../db/schemas/manga.schema';
import { Episode, EpisodeSchema } from '../db/schemas/episode.schema';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: Manga.name, schema: MangaSchema },
      { name: Episode.name, schema: EpisodeSchema },
    ]),
  ],
  controllers: [MangasController, EpisodesController],
})
export class MangasModule {}
