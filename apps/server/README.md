# OpenAI Workflow

NestJS server with MongoDB for manga tracking and pump controller.

## IMPORTANT - DO NOT USE PUPPETEER

This project uses **OpenCode browser tools** for web crawling (manga sites like AsuraScans).

**NEVER install or use puppeteer** - it will cause build errors and conflicts with OpenCode's built-in browser automation.

If you need to crawl websites:

1. Use OpenCode's browser tools (`dev-browser` skill)
2. Then store data via API endpoints

## Quick Start

```bash
# Install dependencies
npm install

# Start MongoDB (required)
docker run -d -p 27017:27017 mongo

# Run server with mDNS (access at http://openai.local:3000)
npm run start:mdns
```

## Access

- Dashboard: http://openai.local:3000/
- API: http://openai.local:3000/api/*

## Features

- Pump Controller API (SmartPump)
- Manga Reader & Tracker
- RAG-powered search

## API Endpoints

### Pump Settings

- `GET /api/pump-settings` - List all pumps
- `GET /api/pump-settings/:pumpId` - Get pump
- `POST /api/pump-settings` - Create/update pump
- `DELETE /api/pump-settings/:pumpId` - Delete pump

### Dose Events

- `POST /api/dose-events` - Log dose
- `GET /api/dose-events/:pumpId` - Get history
- `GET /api/dose-events/:pumpId/today` - Today's doses

### Health

- `GET /api/health` - Server health
