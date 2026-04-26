import { Injectable, Logger } from '@nestjs/common';
import { Cron } from '@nestjs/schedule';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { Manga, MangaDocument } from '../db/schemas/manga.schema';
import { Episode } from '../db/schemas/episode.schema';

@Injectable()
export class ChapterSchedulerService {
  private readonly logger = new Logger(ChapterSchedulerService.name);
  private isRunning = false;

  constructor(
    @InjectModel(Manga.name) private mangaModel: Model<MangaDocument>,
    @InjectModel(Episode.name) private episodeModel: Model<Episode>,
  ) {}

  @Cron('0 0 * * *')
  async handleDailyCheck() {
    if (this.isRunning) {
      this.logger.warn('Previous check still running, skipping...');
      return;
    }

    this.isRunning = true;
    this.logger.log('Starting daily chapter check...');
    this.logger.log('Note: Use OpenCode browser tools to crawl manga data, then use API to store');

    try {
      const bookmarkedManga = await this.mangaModel.find({ isBookmarked: true });
      this.logger.log(`Checking ${bookmarkedManga.length} bookmarked manga for updates`);

      for (const manga of bookmarkedManga) {
        await this.mangaModel.updateOne({ _id: manga._id }, { lastCheckedAt: new Date() });
      }

      this.logger.log('Daily check complete. Use OpenCode to crawl new chapters.');
    } catch (error: any) {
      this.logger.error(`Daily check failed: ${error.message}`);
    } finally {
      this.isRunning = false;
    }
  }

  async triggerDailyCheck() {
    return this.handleDailyCheck();
  }

  async checkAndCrawlNewChapters() {
    this.logger.log('Use OpenCode browser tools to crawl manga chapters');
    this.logger.log('Then use API endpoints to store data:');
    this.logger.log('  - POST /api/mangas - Add new manga');
    this.logger.log('  - POST /api/mangas/:id/episodes - Add episodes');

    const bookmarkedManga = await this.mangaModel.find({ isBookmarked: true });
    return { mangaChecked: bookmarkedManga.length, newChapters: 0, episodesCrawled: 0 };
  }
}
