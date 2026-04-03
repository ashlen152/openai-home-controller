import { Injectable, Logger } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { spawn } from 'child_process';
import * as path from 'path';
import { Manga, MangaDocument } from '../db/schemas/manga.schema';
import { Episode } from '../db/schemas/episode.schema';
import { LogsService } from '../logs/logs.service';

@Injectable()
export class CrawlService {
  private readonly logger = new Logger(CrawlService.name);
  private lastCrawl: Date | null = null;
  private isRunning = false;

  constructor(
    @InjectModel(Manga.name) private mangaModel: Model<MangaDocument>,
    @InjectModel(Episode.name) private episodeModel: Model<Episode>,
    private readonly logsService: LogsService,
  ) {}

  private async runScript(scriptName: string, args: string[] = []): Promise<void> {
    if (this.isRunning) {
      this.logsService.warn('Another crawl is already running', 'Crawler');
      return;
    }

    this.isRunning = true;
    const scriptDir = path.join(process.cwd(), 'scripts');
    const scriptPath = path.join(scriptDir, scriptName);

    return new Promise((resolve) => {
      const child: any = spawn('node', [scriptPath, ...args], {
        cwd: scriptDir,
        shell: true,
      });

      child.stdout.on('data', (data: any) => {
        const lines = data
          .toString()
          .split('\n')
          .filter((l: string) => l.trim());
        for (const line of lines) {
          if (line.includes('🤖') || line.includes('Calling OpenCode')) {
            this.logsService.info(line, 'OpenCode');
          } else if (line.includes('🌐') || line.includes('Fetching')) {
            this.logsService.info(line, 'Crawler');
          } else if (line.includes('📚') || line.includes('Found')) {
            this.logsService.info(line, 'Crawler');
          } else if (line.includes('📖')) {
            this.logsService.info(line, 'Crawler');
          } else if (line.includes('🆕') || line.includes('NEW')) {
            this.logsService.info(line, 'Crawler');
          } else if (line.includes('✅') || line.includes('complete')) {
            this.logsService.info(line, 'Crawler');
          } else if (line.includes('❌') || line.includes('Error')) {
            this.logsService.error(line, 'Crawler');
          } else if (line.includes('⚠️')) {
            this.logsService.warn(line, 'Crawler');
          } else if (line.includes('=') || line.includes('─') || line.includes('SUMMARY')) {
          } else if (line.trim()) {
            this.logsService.info(line, 'Crawler');
          }
        }
      });

      child.stderr.on('data', (data: Buffer) => {
        const msg = data.toString().trim();
        if (msg) {
          this.logsService.error(msg, 'Crawler');
        }
      });

      child.on('close', (code: number | null) => {
        this.isRunning = false;
        if (code === 0) {
          this.logsService.info('Script completed successfully', 'Crawler');
        } else {
          this.logsService.error(`Script exited with code ${code}`, 'Crawler');
        }
        resolve();
      });

      child.on('error', (err: Error) => {
        this.isRunning = false;
        this.logsService.error(`Failed to run script: ${err.message}`, 'Crawler');
        resolve();
      });
    });
  }

  async getStatus() {
    return {
      status: 'ready',
      lastCrawl: this.lastCrawl,
      note: 'Use OpenCode browser tools to crawl manga, then use API to store data',
    };
  }

  async syncBookmarks(dryRun = false) {
    const msg = 'Starting bookmark sync - using OpenCode to crawl AsuraScans';
    this.logger.log(msg);
    this.logsService.info(msg, 'Crawler');

    if (this.isRunning) {
      return {
        success: false,
        message: 'Another crawl is already running',
        newManga: 0,
        newChapters: 0,
      };
    }

    this.logsService.info('Running bookmark-sync script...', 'Crawler');

    try {
      await this.runScript('bookmark-sync.js', dryRun ? ['--dry'] : []);
      return {
        success: true,
        message: 'Bookmark sync completed',
        newManga: 0,
        newChapters: 0,
      };
    } catch (error: any) {
      this.logsService.error(`Bookmark sync failed: ${error.message}`, 'Crawler');
      return {
        success: false,
        message: error.message,
        newManga: 0,
        newChapters: 0,
      };
    }
  }

  async checkNewChapters(dryRun = false) {
    const msg = 'Checking for new chapters...';
    this.logger.log(msg);
    this.logsService.info(msg, 'Crawler');

    if (this.isRunning) {
      return {
        totalManga: 0,
        mangaWithNewChapters: 0,
        newChaptersFound: 0,
        manga: [],
        message: 'Another crawl is already running',
      };
    }

    this.logsService.info('Running bookmark-sync to check for new chapters...', 'Crawler');

    try {
      await this.runScript('bookmark-sync.js', dryRun ? ['--dry'] : []);

      const allManga = await this.mangaModel.find({});
      const results = {
        totalManga: allManga.length,
        mangaWithNewChapters: 0,
        newChaptersFound: 0,
        manga: [] as any[],
      };

      for (const manga of allManga) {
        const latestChapter = manga.latestChapter || 0;
        const currentChapter = manga.currentChapter || 0;

        if (latestChapter > currentChapter) {
          const newChapters = latestChapter - currentChapter;
          results.mangaWithNewChapters++;
          results.newChaptersFound += newChapters;

          results.manga.push({
            id: manga._id,
            title: manga.title,
            currentChapter,
            latestChapter,
            newChapters,
            url: manga.url,
          });
        }
      }

      this.logsService.info(
        `Found ${results.newChaptersFound} new chapters across ${results.mangaWithNewChapters} manga`,
        'Crawler',
      );
      return results;
    } catch (error: any) {
      this.logsService.error(`Check failed: ${error.message}`, 'Crawler');
      return {
        totalManga: 0,
        mangaWithNewChapters: 0,
        newChaptersFound: 0,
        manga: [],
        message: error.message,
      };
    }
  }

  async crawlNewChapters(mangaId?: string, dryRun = false, maxChaptersPerManga = 5) {
    const msg = 'Use OpenCode browser tools to crawl chapter images';
    this.logger.log(msg);
    this.logsService.info(msg, 'Crawler');
    this.logsService.info('Then use: PATCH /api/episodes/:id/images to store images', 'Crawler');

    let mangaToProcess: any[];

    if (mangaId) {
      const manga = await this.mangaModel.findById(mangaId);
      if (!manga) {
        return { success: false, message: 'Manga not found' };
      }
      mangaToProcess = [manga];
    } else {
      mangaToProcess = await this.mangaModel.find({ latestChapter: { $gt: 0 } });
    }

    return {
      success: true,
      total: mangaToProcess.length,
      processed: 0,
      newEpisodes: 0,
      errors: 0,
      message: 'Use OpenCode to crawl chapter images',
    };
  }

  async crawlBookmarks(mangaId: string): Promise<{ success: boolean; count: number }> {
    return { success: true, count: 0 };
  }

  async crawlMangaInfo(mangaId: string): Promise<{ success: boolean; info: any }> {
    return { success: true, info: {} };
  }

  async crawlChapterImages(
    episodeId: string,
    url: string,
  ): Promise<{ success: boolean; images: string[] }> {
    const msg = `Use OpenCode to crawl: ${url}`;
    this.logger.log(msg);
    this.logsService.info(msg, 'Crawler');
    this.logsService.info('Then use PATCH /api/episodes/:id/images to store images', 'Crawler');
    return { success: false, images: [] };
  }
}
