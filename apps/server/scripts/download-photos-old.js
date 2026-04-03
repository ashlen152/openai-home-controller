#!/usr/bin/env node

const CDP = require('chrome-remote-interface');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const c = {
  reset: '\x1b[0m',
  bright: '\x1b[1m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  cyan: '\x1b[36m',
  red: '\x1b[31m',
  gray: '\x1b[90m',
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
    const options = {
      headers: {
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36',
        Referer: 'https://www.tiktok.com/',
      },
    };

    const file = fs.createWriteStream(outputPath);

    protocol
      .get(url, options, (response) => {
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
        file.on('finish', () => {
          file.close();
          resolve();
        });
      })
      .on('error', (err) => {
        file.close();
        fs.unlink(outputPath, () => {});
        reject(err);
      });
  });
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.log(`
${c.bright}TikTok Photo Batch Downloader${c.reset}

Usage:
  node scripts/download-photos.js <urls.json> <port>

Example:
  node scripts/download-photos.js tiktok_download/cuxi037/urls.json 9222
`);
    return;
  }

  const urlsFile = args[0];
  const port = parseInt(args[1]) || 9222;

  if (!fs.existsSync(urlsFile)) {
    log.error(`URLs file not found: ${urlsFile}`);
    return;
  }

  const data = JSON.parse(fs.readFileSync(urlsFile, 'utf8'));
  const photos = data.photos || [];

  if (photos.length === 0) {
    log.error('No photos found in URLs file');
    return;
  }

  const outputDir = path.join(path.dirname(urlsFile), 'photos');
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
  }

  log.info(`Connecting to Chrome on port ${port}...`);

  let client;
  try {
    client = await CDP({ port });
  } catch (err) {
    log.error(`Failed to connect to Chrome: ${err.message}`);
    log.info('Make sure Chrome is running with --remote-debugging-port=9222');
    return;
  }

  const { Page, Runtime } = client;
  await Page.enable();
  await Runtime.enable();

  let downloadedCount = 0;
  let skippedCount = 0;

  log.info(`Downloading photos from ${photos.length} posts...`);

  for (let i = 0; i < photos.length; i++) {
    const photoUrl = photos[i];
    const photoId = photoUrl.match(/\/photo\/(\d+)/)?.[1] || i;

    log.progress(i + 1, photos.length, `Processing photo ${photoId}...`);

    try {
      await Page.navigate({ url: photoUrl });
      await Page.loadEventFired();
      await new Promise((r) => setTimeout(r, 1500));

      const result = await Runtime.evaluate({
        expression: `
          (() => {
            const images = [];
            const imgs = document.querySelectorAll('img[src*="photomode"]');
            imgs.forEach(img => {
              if (img.naturalWidth > 500) {
                images.push(img.src);
              }
            });
            return [...new Set(images)];
          })()
        `,
      });

      const imageUrls = result.result.value || [];

      if (imageUrls.length === 0) {
        log.warn(`No images found on photo page`);
        continue;
      }

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
          log.info(`Downloading: ${imageUrl.substring(0, 80)}...`);
          await downloadImage(imageUrl, outputPath);
          const stats = fs.statSync(outputPath);

          if (stats.size < 1000) {
            fs.unlinkSync(outputPath);
            skippedCount++;
            log.warn(`File too small (${stats.size} bytes): ${fileName}`);
          } else {
            downloadedCount++;
            const sizeKB = (stats.size / 1024).toFixed(1);
            log.success(`Saved: ${fileName} (${sizeKB} KB)`);
          }
        } catch (err) {
          log.warn(`Download failed: ${err.message}`);
        }
      }
    } catch (err) {
      log.error(`Error processing photo ${photoId}: ${err.message}`);
    }

    if ((i + 1) % 10 === 0) {
      log.info(`Progress: ${downloadedCount} downloaded, ${skippedCount} skipped`);
    }
  }

  log.success(`\nDownload complete!`);
  log.info(`Downloaded: ${downloadedCount} images`);
  log.info(`Skipped: ${skippedCount} images`);
  log.info(`Output: ${outputDir}`);

  await client.close();
}

if (require.main === module) {
  main().catch((err) => {
    log.error(`Fatal error: ${err.message}`);
    process.exit(1);
  });
}
