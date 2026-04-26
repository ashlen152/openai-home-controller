import { Injectable, Logger } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { IMangaAdapter, MangaInfo, MangaChapter, ChapterImages } from '../interfaces/manga-adapter.interface';

@Injectable()
export class AsuraScansAdapter implements IMangaAdapter {
  readonly name = 'asurascans';
  readonly baseUrl = 'https://asurascans.com';
  private readonly logger = new Logger(AsuraScansAdapter.name);

  constructor(private configService: ConfigService) {}

  async login(credentials: { email: string; password: string }): Promise<void> {
    this.logger.warn('Manual login required via OpenCode browser tools');
  }

  async isAuthenticated(): Promise<boolean> {
    return true;
  }

  async getMangaInfo(mangaId: string): Promise<MangaInfo> {
    this.logger.warn(`Use OpenCode browser to view manga ${mangaId}, then call POST /mangas`);
    return { id: mangaId, title: 'Use OpenCode browser', coverImage: '', description: '', chapters: [] };
  }

  async getBookmarks(): Promise<MangaInfo[]> {
    this.logger.warn('Use OpenCode browser to view bookmarks, then call POST /mangas');
    return [];
  }

  async getChapterImages(chapterUrl: string): Promise<ChapterImages> {
    this.logger.warn(`Use OpenCode browser to view chapter ${chapterUrl}, then call PATCH /episodes/:id/images`);
    return { chapterId: chapterUrl.split('/').pop() || '', images: [] };
  }

  async getLatestChapter(mangaId: string): Promise<MangaChapter | null> {
    return null;
  }
}
