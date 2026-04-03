import { Injectable, Logger } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { IMangaAdapter } from '../adapters/interfaces/manga-adapter.interface';
import { Manga, MangaDocument } from '../db/schemas/manga.schema';
import { Episode } from '../db/schemas/episode.schema';

@Injectable()
export class CrawlerService {
  private readonly logger = new Logger(CrawlerService.name);

  constructor(
    @InjectModel(Manga.name) private mangaModel: Model<MangaDocument>,
    @InjectModel(Episode.name) private episodeModel: Model<Episode>,
  ) {}

  async addMangaFromAdapter(adapter: IMangaAdapter, mangaData: { id: string; title: string; coverImage?: string }): Promise<MangaDocument> {
    let manga = await this.mangaModel.findOne({ externalId: mangaData.id, source: adapter.name });
    
    if (!manga) {
      manga = await this.mangaModel.create({
        externalId: mangaData.id,
        title: mangaData.title,
        coverImage: mangaData.coverImage || '',
        source: adapter.name,
        isBookmarked: true,
        lastCheckedAt: new Date(),
      });
      this.logger.log(`Added manga: ${mangaData.title}`);
    }
    
    return manga;
  }

  async addEpisodeFromAdapter(manga: MangaDocument, episodeData: { 
    id: string; 
    title: string; 
    number: number; 
    url: string; 
    publishedAt?: Date 
  }): Promise<Episode | null> {
    const existing = await this.episodeModel.findOne({ 
      externalId: episodeData.id, 
      mangaId: manga._id 
    });
    
    if (existing) return null;
    
    const episode = await this.episodeModel.create({
      mangaId: manga._id,
      externalId: episodeData.id,
      title: episodeData.title,
      number: episodeData.number,
      url: episodeData.url,
      imageUrls: [],
      publishedAt: episodeData.publishedAt || new Date(),
      isNew: true,
      crawledAt: new Date(),
    });
    
    this.logger.log(`Added episode: ${manga.title} - ${episodeData.title}`);
    return episode;
  }

  async updateEpisodeImages(externalId: string, imageUrls: string[]): Promise<void> {
    await this.episodeModel.updateOne(
      { externalId },
      { imageUrls, crawledAt: new Date(), isNew: false }
    );
  }

  async crawlBookmarkList(adapter: IMangaAdapter): Promise<{ newMangas: number; newChapters: number }> {
    this.logger.log('Starting bookmark crawl...');
    this.logger.log('Note: Use OpenCode browser tools to browse AsuraScans, then call API endpoints to store data');
    
    const bookmarks = await adapter.getBookmarks();
    let newMangas = 0;
    let newChapters = 0;

    for (const bookmark of bookmarks) {
      const manga = await this.addMangaFromAdapter(adapter, bookmark);

      const mangaInfo = await adapter.getMangaInfo(bookmark.id);

      for (const chapter of mangaInfo.chapters.slice(0, 5)) {
        const episode = await this.addEpisodeFromAdapter(manga, {
          id: chapter.id,
          title: chapter.title,
          number: chapter.number,
          url: chapter.url,
          publishedAt: chapter.publishedAt,
        });
        if (episode) newChapters++;
      }

      await this.mangaModel.updateOne(
        { _id: manga._id },
        { lastCheckedAt: new Date(), lastChapterAt: mangaInfo.chapters[0]?.publishedAt },
      );
    }

    this.logger.log(`Crawl complete: ${newMangas} new mangas, ${newChapters} new chapters`);
    return { newMangas, newChapters };
  }

  async crawlChapter(chapterUrl: string): Promise<string[]> {
    this.logger.log(`Crawling chapter: ${chapterUrl}`);
    this.logger.log('Note: Use OpenCode browser tools to get images, then call PATCH /episodes/:id/images');
    return [];
  }

  async crawlAllNewEpisodes(adapter: IMangaAdapter): Promise<number> {
    const newEpisodes = await this.episodeModel.find({ isNew: true, imageUrls: { $size: 0 } });
    this.logger.log(`Found ${newEpisodes.length} episodes without images`);
    return newEpisodes.length;
  }
}
