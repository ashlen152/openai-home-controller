export interface LLMResponse {
  content: string;
  usage: { promptTokens: number; completionTokens: number; totalTokens: number };
}

export interface CompletionOptions {
  temperature?: number;
  maxTokens?: number;
  model?: string;
}

export const LLM_PROVIDER = 'LLM_PROVIDER';

export interface ILLMProvider {
  readonly name: string;
  complete(prompt: string, options?: CompletionOptions): Promise<LLMResponse>;
  embed(text: string): Promise<number[]>;
  embedBatch(texts: string[]): Promise<number[][]>;
}
