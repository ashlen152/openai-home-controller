import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { RagService } from './rag.service';
import { RagController } from './rag.controller';
import { OpenCodeCLIProvider } from './llm/opencode-cli.provider';
import { OllamaProvider } from './llm/ollama.provider';
import { Episode, EpisodeSchema } from '../db/schemas/episode.schema';
import { Manga, MangaSchema } from '../db/schemas/manga.schema';
import { LLM_PROVIDER } from './llm/interfaces/llm-provider.interface';

@Module({
  imports: [
    ConfigModule,
    MongooseModule.forFeature([
      { name: Episode.name, schema: EpisodeSchema },
      { name: Manga.name, schema: MangaSchema },
    ]),
  ],
  controllers: [RagController],
  providers: [
    RagService,
    OpenCodeCLIProvider,
    OllamaProvider,
    {
      provide: LLM_PROVIDER,
      useFactory: (configService: ConfigService) => {
        const provider = configService.get('llmProvider') || 'opencode-cli';
        if (provider === 'ollama') {
          return new OllamaProvider(configService);
        }
        return new OpenCodeCLIProvider(configService);
      },
      inject: [ConfigService],
    },
  ],
  exports: [RagService, LLM_PROVIDER, OpenCodeCLIProvider, OllamaProvider],
})
export class RagModule {}
