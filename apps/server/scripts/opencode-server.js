#!/usr/bin/env node

/**
 * OpenCode Browser Wrapper - HTTP API for OpenCode browser automation
 * 
 * Run: node scripts/opencode-server.js
 * Then call: POST http://localhost:3849/crawl
 */

const http = require('http');
const { execSync, spawn } = require('child_process');

const PORT = process.env.PORT || 3849;
const HOST = process.env.HOST || '0.0.0.0';

const server = http.createServer(async (req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Content-Type', 'application/json');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  if (req.method === 'POST' && req.url === '/crawl') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', async () => {
      try {
        const { prompt, model } = JSON.parse(body);
        
        if (!prompt) {
          res.writeHead(400);
          res.end(JSON.stringify({ error: 'Missing prompt' }));
          return;
        }

        console.log('🤖 Executing OpenCode with prompt:', prompt.substring(0, 50) + '...');
        
        const opencodePaths = [
          process.env.OPENCODE_PATH,
          '/opt/homebrew/bin/opencode',
          '/usr/local/bin/opencode',
          '/usr/bin/opencode',
        ].filter(Boolean);
        
        const { existsSync } = require('fs');
        let opencodeCmd = null;
        for (const p of opencodePaths) {
          if (existsSync(p)) { opencodeCmd = p; break; }
        }
        if (!opencodeCmd) throw new Error('OpenCode not found');
        const result = execSync(
          `${opencodeCmd} run "${prompt.replace(/"/g, '\\"')}" -m ${model || 'opencode/mimo-v2-pro-free'}`,
          { timeout: 180000, encoding: 'utf-8' }
        );

        console.log('✅ OpenCode completed');
        res.writeHead(200);
        res.end(JSON.stringify({ success: true, result }));
      } catch (error) {
        console.error('❌ OpenCode error:', error.message);
        res.writeHead(500);
        res.end(JSON.stringify({ success: false, error: error.message }));
      }
    });
    return;
  }

  if (req.method === 'GET' && req.url === '/health') {
    res.writeHead(200);
    res.end(JSON.stringify({ status: 'ok' }));
    return;
  }

  res.writeHead(404);
  res.end(JSON.stringify({ error: 'Not found' }));
});

server.listen(PORT, HOST, () => {
  console.log(`🚀 OpenCode wrapper listening on http://${HOST}:${PORT}`);
  console.log('   POST /crawl - Execute OpenCode prompt');
  console.log('   GET  /health - Health check');
});

process.on('SIGINT', () => {
  console.log('\n👋 Shutting down...');
  server.close();
  process.exit(0);
});
