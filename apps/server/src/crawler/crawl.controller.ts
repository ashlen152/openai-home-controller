import { Controller, Post, Get, Param, Body, HttpCode, HttpStatus } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse } from '@nestjs/swagger';
import { CrawlService } from './crawl.service';

@ApiTags('crawl')
@Controller('crawl')
export class CrawlController {
  constructor(private readonly crawlService: CrawlService) {}

  @Post('bookmarks')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ summary: 'Crawl bookmarks from AsuraScans using Puppeteer' })
  @ApiResponse({ status: 200, description: 'Crawl results' })
  async crawlBookmarks(@Body() body: { mangaId?: string } = {}) {
    return this.crawlService.syncBookmarks(false);
  }

  @Post('manga/:id')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ summary: 'Crawl manga info and chapters' })
  @ApiResponse({ status: 200, description: 'Manga crawl results' })
  async crawlManga(@Param('id') mangaId: string) {
    return { success: true, mangaId, message: 'Use /crawl/bookmarks/sync to sync all bookmarks' };
  }

  @Post('chapter')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ summary: 'Crawl chapter images' })
  @ApiResponse({ status: 200, description: 'Chapter images' })
  async crawlChapter(@Body() body: { url: string; episodeId: string }) {
    return this.crawlService.crawlChapterImages(body.episodeId, body.url);
  }

  @Get('status')
  @ApiOperation({ summary: 'Check crawl service status' })
  async getStatus() {
    return this.crawlService.getStatus();
  }

  @Post('bookmarks/sync')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ summary: 'Sync bookmarks from AsuraScans - fetches and compares with DB' })
  @ApiResponse({ status: 200, description: 'Sync results with new manga and chapters' })
  async syncBookmarks(@Body() body: { dryRun?: boolean } = {}) {
    return this.crawlService.syncBookmarks(body.dryRun);
  }

  @Get('chapters/check')
  @ApiOperation({ summary: 'Check which manga have new chapters available' })
  @ApiResponse({ status: 200, description: 'List of manga with new chapters' })
  async checkNewChapters(@Body() body: { dryRun?: boolean } = {}) {
    return this.crawlService.checkNewChapters(body.dryRun);
  }

  @Post('chapters/crawl')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ summary: 'Crawl new chapters for all manga or specific manga' })
  @ApiResponse({ status: 200, description: 'Crawl results with new episodes' })
  async crawlNewChapters(
    @Body() body: { mangaId?: string; dryRun?: boolean; maxChapters?: number } = {},
  ) {
    return this.crawlService.crawlNewChapters(body.mangaId, body.dryRun, body.maxChapters);
  }
}
