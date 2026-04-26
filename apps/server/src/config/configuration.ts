import { Injectable } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';

export default () => ({
  port: parseInt(process.env.PORT || '3000', 10),
  mongodbUri: process.env.MONGODB_URI || 'mongodb://localhost:27017/openai-workflow',
  llmProvider: process.env.LLM_PROVIDER || 'opencode-cli',
  ollamaBaseUrl: process.env.OLLAMA_BASE_URL || 'http://localhost:11434',
  ollamaModel: process.env.OLLAMA_MODEL || 'dolphin-mistral',
  ollamaEmbedModel: process.env.OLLAMA_EMBED_MODEL || 'nomic-embed-text',
  openaiApiKey: process.env.OPENAI_API_KEY,
  opencodeApiKey: process.env.OPENCODE_API_KEY,
  asuraEmail: process.env.ASURA_EMAIL,
  asuraPassword: process.env.ASURA_PASSWORD,
  playwrightStoragePath: process.env.PLAYWRIGHT_STORAGE_PATH || './playwright/.auth/user.json',
  playwrightHeadful: process.env.PLAYWRIGHT_HEADFUL === 'true',
  useMock: process.env.USE_MOCK === 'true',
});
