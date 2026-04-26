import { Controller, Get, Post, Body, Query } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse } from '@nestjs/swagger';
import { RagService } from './rag.service';

@ApiTags('rag')
@Controller('rag')
export class RagController {
  constructor(private readonly ragService: RagService) {}

  @Post('embed')
  @ApiOperation({ summary: 'Generate embeddings for all episodes without embeddings' })
  @ApiResponse({ status: 200, description: 'Number of episodes embedded' })
  async embedAll() {
    const count = await this.ragService.embedAllNewEpisodes();
    return { success: true, embedded: count };
  }

  @Get('search')
  @ApiOperation({ summary: 'Search episodes using RAG similarity' })
  @ApiResponse({ status: 200, description: 'Search results with answer' })
  async search(@Query('q') question: string, @Query('mangaId') mangaId?: string) {
    if (!question) {
      return { success: false, message: 'Query parameter "q" is required' };
    }
    return this.ragService.query(question, mangaId);
  }
}
