import { Injectable, Logger } from '@nestjs/common';
import { ILLMProvider, LLMResponse, CompletionOptions } from './interfaces/llm-provider.interface';
import { exec } from 'child_process';
import { promisify } from 'util';
import { ConfigService } from '@nestjs/config';

const execAsync = promisify(exec);

@Injectable()
export class OpenCodeCLIProvider implements ILLMProvider {
  readonly name = 'opencode-cli';
  private readonly logger = new Logger(OpenCodeCLIProvider.name);

  constructor(private configService: ConfigService) {}

  async complete(prompt: string, options?: CompletionOptions): Promise<LLMResponse> {
    const model = options?.model || this.configService.get('opencodeModel') || 'github-copilot/claude-haiku-4.5';

    try {
      const escapedPrompt = prompt.replace(/"/g, '\\"');
      const { stdout, stderr } = await execAsync(
        `opencode run "${escapedPrompt}" -m ${model} --format default`,
        { timeout: 60000 }
      );

      const output = stdout + stderr;
      const content = this.extractContent(output);

      return {
        content,
        usage: {
          promptTokens: Math.ceil(prompt.length / 4),
          completionTokens: Math.ceil(content.length / 4),
          totalTokens: Math.ceil((prompt.length + content.length) / 4),
        },
      };
    } catch (error: any) {
      this.logger.error('OpenCode CLI call failed:', error.message);
      return {
        content: `Error: ${error.message}`,
        usage: { promptTokens: 0, completionTokens: 0, totalTokens: 0 },
      };
    }
  }

  async embed(text: string): Promise<number[]> {
    this.logger.warn('OpenCode CLI does not support embeddings, use Ollama instead');
    return new Array(768).fill(0);
  }

  async embedBatch(texts: string[]): Promise<number[][]> {
    return Promise.all(texts.map((text) => this.embed(text)));
  }

  private extractContent(output: string): string {
    const lines = output.split('\n');
    const contentLines: string[] = [];
    let inContent = false;

    for (const line of lines) {
      if (line.startsWith('>')) {
        inContent = true;
        continue;
      }
      if (inContent && line.trim()) {
        contentLines.push(line);
      }
    }

    return contentLines.join('\n').trim() || output;
  }
}
