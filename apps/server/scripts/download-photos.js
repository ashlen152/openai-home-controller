#!/usr/bin/env node

const CDP = require('chrome-remote-interface');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const c = {
  reset: '\x1b[0m', bright: '\x1b[1m', green: '\x1b[32m',
  yellow: '\x1b[33m', blue: '\x1b[34m', cyan: '\x1b[36m', red: '\x1b[31m',
};

const log = {
  info: (msg) => console.log(`${c.blue}[INFO]${c.reset} ${msg}`),
  success: (msg) => console.log(`${c.green}[SUCCESS]${c.reset} ${msg}`),
  warn: (msg) => console.log(`${c.yellow}[WARN]${c.reset} ${msg}`),
  error: (msg) => console.log(`${c.red}[ERROR]${c.reset} ${msg}`),
  progress: (cur, total, msg) => console.log(`${c.cyan}[${cur}/${total}]${c.reset} ${msg}`),
};

async function downloadImage(url, outputPath) {
  return new Promise((resolve, reject) => {
    const protocol = url.startsWith('https') ? https : http;
    const file = fs.createWriteStream(outputPath);
    
    protocol.get(url, {
      headers: {
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36',
        'Referer': 'https://www.tiktok.com/',
      }
    }, (response) => {
      if (response.statusCode >= 300 && response.statusCode < 400 && response.headers.location) {
        file.close();
        downloadImage(response.headers.location, outputPath).then(resolve).catch(reject);
        return;
      }
      if (response.statusCode !== 200) {
        file.close();
        fs.unlink(outputPath, () => {});
        reject(new Error(`HTTP ${response.statusCode}`));
        return;
      }
      response.pipe(file);
      file.on('finish', () => { file.close(); resolve(); });
    }).on('error', (err) => {
      file.close();
      fs.unlink(outputPath, () => {});
      reject(err);
    });
  });
}

async function main() {
  const args = process.argv.slice(2);
  if (args.length < 2) {
    console.log('Usage: node download-photos-v3.js <urls.json> <port>');
    return;
  }

  const urlsFile = args[0];
  const port = parseInt(args[1]) || 9222;
  const data = JSON.parse(fs.readFileSync(urlsFile, 'utf8'));
  const photos = data.photos || [];
  
  const outputDir = path.join(path.dirname(urlsFile), 'photos');
  if (!fs.existsSync(outputDir)) fs.mkdirSync(outputDir, { recursive: true });

  log.info(`Connecting to Chrome on port ${port}...`);
  
  const client = await CDP({ port });
  const { Page, Runtime } = client;
  await Promise.all([Page.enable(), Runtime.enable()]);

  let downloadedCount = 0, skippedCount = 0, errorCount = 0;

  log.info(`Downloading photos from ${photos.length} posts...`);

  for (let i = 0; i < photos.length; i++) {
    const photoUrl = photos[i];
    const photoId = photoUrl.match(/\/photo\/(\d+)/)?.[1] || i;

    log.progress(i + 1, photos.length, `Processing photo ${photoId}...`);

    try {
      // First navigate to about:blank to reset state
      await Page.navigate({ url: 'about:blank' });
      await Page.loadEventFired();
      
      // Then navigate to the photo URL
      await Page.navigate({ url: photoUrl });
      await Page.loadEventFired();
      
      // Wait for images to load
      await new Promise(r => setTimeout(r, 2500));

      const result = await Runtime.evaluate({
        expression: `
          (() => {
            const imgs = document.querySelectorAll('img[src*="photomode"]');
            const urls = [];
            imgs.forEach(img => {
              if (img.naturalWidth > 500) urls.push(img.src);
            });
            return [...new Set(urls)];
          })()
        `,
        returnByValue: true
      });

      const imageUrls = result.result?.value || [];

      if (imageUrls.length === 0) {
        log.warn(`No images on photo ${photoId}`);
      } else {
        log.info(`Found ${imageUrls.length} images`);
        for (let j = 0; j < imageUrls.length; j++) {
          const imageUrl = imageUrls[j];
          const fileName = `photo_${photoId}_${j + 1}.jpg`;
          const outputPath = path.join(outputDir, fileName);

          if (fs.existsSync(outputPath)) {
            skippedCount++;
            continue;
          }

          try {
            await downloadImage(imageUrl, outputPath);
            const stats = fs.statSync(outputPath);
            if (stats.size < 1000) {
              fs.unlinkSync(outputPath);
              log.warn(`Too small: ${fileName}`);
            } else {
              downloadedCount++;
              log.success(`Saved ${fileName} (${(stats.size/1024).toFixed(0)}KB)`);
            }
          } catch (err) {
            log.warn(`DL failed: ${err.message}`);
            errorCount++;
          }
        }
      }
      
    } catch (err) {
      log.error(`Error: ${err.message}`);
      errorCount++;
    }

    if ((i + 1) % 20 === 0) {
      log.info(`Progress: ${downloadedCount} downloaded, ${skippedCount} skipped, ${errorCount} errors`);
    }
  }

  log.success(`\nComplete! Downloaded: ${downloadedCount}, Skipped: ${skippedCount}, Errors: ${errorCount}`);
  await client.close();
}

if (require.main === module) {
  main().catch(err => { log.error(`Fatal: ${err.message}`); process.exit(1); });
}
