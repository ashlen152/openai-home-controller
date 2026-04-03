# SmartPump Server — AGENTS.md

> **Part of the SmartPump Monorepo**. The firmware lives at `apps/firmware/` (ESP32/PlatformIO). See root `AGENTS.md` for cross-app workflow.

## Project Overview

NestJS server providing REST APIs for:

- **Pump Control** — Settings management, dose event logging, pump commands (for ESP32 firmware)
- **Manga Tracking** — Reader, tracker, crawler integration
- **RAG Search** — OpenAI/Ollama-powered semantic search
- **Workflows** — Automation pipelines

## Tech Stack

- **Framework**: NestJS 10 with TypeScript
- **Database**: MongoDB (Mongoose ODM)
- **Queue**: BullMQ + Redis
- **Auth**: Custom auth module
- **Search**: OpenAI API + Ollama (local)
- **Browser**: OpenCode browser tools (NO puppeteer)

## Directory Structure

```
apps/server/
├── src/
│   ├── pump/              # Pump API (firmware integration)
│   │   ├── controllers/   # HTTP endpoints
│   │   ├── services/      # Business logic
│   │   ├── dto/           # Request/response validation
│   │   └── schemas/       # MongoDB schemas
│   ├── mangas/            # Manga reader & tracker
│   ├── crawler/           # Web scraping (AsuraScans, etc.)
│   ├── rag/               # RAG search (OpenAI/Ollama)
│   ├── auth/              # Authentication
│   ├── scheduler/         # Scheduled jobs (chapter crawling)
│   ├── workflows/         # Workflow automation
│   ├── logs/              # Logging endpoint
│   ├── health/            # Health check
│   ├── config/            # App configuration
│   ├── db/                # Database schemas
│   └── adapters/          # External service adapters
├── test/                  # Jest tests
├── scripts/               # CLI scripts (TikTok crawler, etc.)
├── public/                # Static assets (dashboard UI)
├── Dockerfile*            # Container configs
└── docker-compose*.yml    # Docker infrastructure
```

## Commands

```bash
# From repo root
npm run server:dev          # Watch mode
npm run server:build        # Production build
npm run server:test         # Jest tests
npm run server:test:e2e     # E2E tests
npm run server:mdns         # With mDNS (http://openai.local:3000)

# From apps/server/
npm run start:dev
npm run build
npm run test
```

## Pump API Endpoints (Firmware Integration)

| Endpoint                         | Method | Purpose                     |
| -------------------------------- | ------ | --------------------------- |
| `/api/health`                    | GET    | Health check                |
| `/api/pump-settings`             | GET    | List all pumps              |
| `/api/pump-settings/:pumpId`     | GET    | Get pump settings           |
| `/api/pump-settings`             | POST   | Create/update pump settings |
| `/api/pump-settings/:pumpId`     | DELETE | Delete pump                 |
| `/api/dose-events`               | POST   | Log dose event              |
| `/api/dose-events/:pumpId`       | GET    | Get dose history            |
| `/api/dose-events/:pumpId/today` | GET    | Get today's doses           |
| `/api/pump-commands`             | POST   | Queue command for firmware  |
| `/api/pump-commands/:pumpId`     | GET    | Get pending commands        |

## IMPORTANT

- **NEVER use puppeteer** — uses OpenCode browser tools instead
- MongoDB data stored in `apps/server/data/mongo/` (excluded from git)
- Server advertises as `openai.local:3000` with mDNS
