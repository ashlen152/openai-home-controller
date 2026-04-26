import { Injectable, Logger, Inject } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { Episode } from '../db/schemas/episode.schema';
import { Manga } from '../db/schemas/manga.schema';
import { ILLMProvider, LLM_PROVIDER } from './llm/interfaces/llm-provider.interface';

@Injectable()
export class RagService {
  private readonly logger = new Logger(RagService.name);

  constructor(
    @InjectModel(Episode.name) private episodeModel: Model<Episode>,
    @InjectModel(Manga.name) private mangaModel: Model<Manga>,
    @Inject(LLM_PROVIDER) private llmProvider: ILLMProvider,
  ) {}

  async embedEpisode(episodeId: string): Promise<void> {
    const episode = await this.episodeModel.findById(episodeId);
    if (!episode) throw new Error(`Episode ${episodeId} not found`);

    const manga = await this.mangaModel.findById(episode.mangaId);
    const textToEmbed = [
      `Manga: ${manga?.title || 'Unknown'}`,
      `Chapter: ${episode.number}`,
      `Title: ${episode.title}`,
      `URL: ${episode.url}`,
    ].join('\n');

    this.logger.log(`Embedding episode: ${episode.title}`);
    episode.embeddings = await this.llmProvider.embed(textToEmbed);
    await episode.save();
  }

  async embedAllNewEpisodes(): Promise<number> {
    const episodes = await this.episodeModel.find({
      $or: [{ embeddings: { $exists: false } }, { embeddings: null }, { embeddings: { $size: 0 } }],
      imageUrls: { $exists: true, $ne: [] },
    });

    for (const episode of episodes) {
      try {
        await this.embedEpisode(episode._id.toString());
      } catch (error) {
        this.logger.error(`Failed to embed episode ${episode._id}:`, error);
      }
    }
    return episodes.length;
  }

  async query(question: string, mangaId?: string) {
    const questionEmbedding = await this.llmProvider.embed(question);

    const results = await this.episodeModel
      .find({
        embeddings: { $exists: true },
        ...(mangaId && { mangaId }),
      })
      .limit(5);

    const context = results.map((r) => `${r.title}: ${r.url}`).join('\n');

    const prompt = `Based on the following manga chapters, answer the question.\n\nContext:\n${context}\n\nQuestion: ${question}\n\nAnswer:`;
    const answer = await this.llmProvider.complete(prompt);

    return {
      answer: answer.content,
      episodes: results.map((r) => ({ title: r.title, number: r.number, url: r.url })),
    };
  }
}
