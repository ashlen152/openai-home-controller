import { Injectable, Logger } from '@nestjs/common';
import { ILLMProvider, LLMResponse, CompletionOptions } from './interfaces/llm-provider.interface';
import { ConfigService } from '@nestjs/config';

@Injectable()
export class OllamaProvider implements ILLMProvider {
  readonly name = 'ollama';
  private readonly logger = new Logger(OllamaProvider.name);
  private baseUrl: string;

  constructor(private configService: ConfigService) {
    this.baseUrl = this.configService.get('ollamaBaseUrl') || 'http://localhost:11434';
  }

  async complete(prompt: string, options?: CompletionOptions): Promise<LLMResponse> {
    const model = options?.model || this.configService.get('ollamaModel') || 'llama3.2-vision';

    try {
      const response = await fetch(`${this.baseUrl}/api/generate`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          model,
          prompt,
          stream: false,
          options: {
            temperature: options?.temperature || 0.7,
            num_predict: options?.maxTokens || 512,
          },
        }),
      });

      if (!response.ok) {
        throw new Error(`Ollama API error: ${response.status}`);
      }

      const data = await response.json();
      return {
        content: data.response || '',
        usage: {
          promptTokens: data.prompt_eval_count || 0,
          completionTokens: data.eval_count || 0,
          totalTokens: (data.prompt_eval_count || 0) + (data.eval_count || 0),
        },
      };
    } catch (error) {
      this.logger.error('Ollama complete failed:', error);
      return {
        content: `Error: ${error.message}`,
        usage: { promptTokens: 0, completionTokens: 0, totalTokens: 0 },
      };
    }
  }

  async embed(text: string): Promise<number[]> {
    const model = this.configService.get('ollamaEmbedModel') || 'nomic-embed-text';

    try {
      const response = await fetch(`${this.baseUrl}/api/embeddings`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ model, prompt: text }),
      });

      if (!response.ok) {
        throw new Error(`Ollama embedding error: ${response.status}`);
      }

      const data = await response.json();
      return data.embedding || [];
    } catch (error) {
      this.logger.error('Ollama embed failed:', error);
      return [];
    }
  }

  async embedBatch(texts: string[]): Promise<number[][]> {
    return Promise.all(texts.map((text) => this.embed(text)));
  }
}
