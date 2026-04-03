import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { CrawlerService } from './crawler.service';
import { CrawlService } from './crawl.service';
import { CrawlController } from './crawl.controller';
import { Manga, MangaSchema } from '../db/schemas/manga.schema';
import { Episode, EpisodeSchema } from '../db/schemas/episode.schema';
import { LogsModule } from '../logs/logs.module';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: Manga.name, schema: MangaSchema },
      { name: Episode.name, schema: EpisodeSchema },
    ]),
    LogsModule,
  ],
  controllers: [CrawlController],
  providers: [CrawlerService, CrawlService],
  exports: [CrawlerService, CrawlService],
})
export class CrawlerModule {}
