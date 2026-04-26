#!/usr/bin/env node

require('dotenv').config();

const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const TIKTOK_ACCOUNT = process.env.TIKTOK_ACCOUNT;

if (!TIKTOK_ACCOUNT) throw new Error('TIKTOK_ACCOUNT env is required');

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

class TikTokDownloader {
  constructor(accountUrl, outputDir) {
    this.accountUrl = accountUrl;
    this.outputDir = outputDir || './tiktok_download';
    this.videos = [];
    this.images = [];
  }

  async downloadFile(url, filePath) {
    return new Promise((resolve, reject) => {
      const protocol = url.startsWith('https') ? https : http;
      const options = {
        headers: {
          'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36',
          Referer: 'https://www.tiktok.com/',
        },
      };

      const file = fs.createWriteStream(filePath);

      protocol
        .get(url, options, (response) => {
          if (
            response.statusCode >= 300 &&
            response.statusCode < 400 &&
            response.headers.location
          ) {
            file.close();
            this.downloadFile(response.headers.location, filePath).then(resolve).catch(reject);
            return;
          }

          if (response.statusCode !== 200) {
            file.close();
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
          fs.unlink(filePath, () => {});
          reject(err);
        });
    });
  }

  extractVideoIds() {
    const videoIds = [];
    for (const url of this.videos) {
      const match = url.match(/\/video\/(\d+)/);
      if (match && !match[1].startsWith('7')) {
        videoIds.push(match[1]);
      }
    }
    return [...new Set(videoIds)];
  }

  extractImageIds() {
    const imageIds = [];
    for (const url of this.images) {
      const match = url.match(/\/photo\/(\d+)/);
      if (match) {
        imageIds.push({ id: match[1], url });
      }
    }
    return [...new Set(imageIds.map((i) => JSON.stringify(i)))].map((s) => JSON.parse(s));
  }

  async downloadImages(urls, prefix = 'image') {
    if (!fs.existsSync(this.outputDir)) {
      fs.mkdirSync(this.outputDir, { recursive: true });
    }

    let count = 0;
    for (let i = 0; i < urls.length; i++) {
      const url = urls[i];
      const fileName = `${prefix}_${i + 1}_${Date.now()}.jpg`;
      const outputPath = path.join(this.outputDir, fileName);

      try {
        log.info(`Downloading image ${i + 1}/${urls.length}`);
        await this.downloadFile(url, outputPath);
        const stats = fs.statSync(outputPath);
        const sizeKB = (stats.size / 1024).toFixed(1);

        if (stats.size < 1000) {
          fs.unlinkSync(outputPath);
          log.warn(`Skipped (too small): ${fileName}`);
        } else {
          log.success(`Saved: ${fileName} (${sizeKB} KB)`);
          count++;
        }
      } catch (err) {
        log.error(`Failed: ${err.message}`);
      }
    }

    log.success(`Downloaded ${count} images`);
    return count;
  }

  addVideoUrl(url) {
    if (!this.videos.includes(url) && url.includes('/video/')) {
      this.videos.push(url);
    }
  }

  addImageUrl(url) {
    if (!this.images.includes(url) && url.includes('/photo/')) {
      this.images.push(url);
    }
  }

  saveVideoList(filepath) {
    const data = {
      accountUrl: this.accountUrl,
      scrapedAt: new Date().toISOString(),
      videos: this.videos,
      images: this.images,
    };
    fs.writeFileSync(filepath, JSON.stringify(data, null, 2));
    log.success(`Saved to: ${filepath}`);
  }

  loadVideoList(filepath) {
    const data = JSON.parse(fs.readFileSync(filepath, 'utf8'));
    this.videos = data.videos || [];
    this.images = data.images || [];
    return data;
  }
}

function showHelp() {
  console.log(`
${c.bright}TikTok Account Downloader${c.reset}
${c.gray}(via OpenCode Chrome DevTools)${c.reset}

${c.cyan}Usage:${c.reset}
  node scripts/tiktok-downloader.js <command> [options]

${c.cyan}Commands:${c.reset}
  scrape <url>          Show Chrome DevTools commands for scraping
  download <file>      Download from saved URL list
  help                 Show this help

${c.cyan}Examples:${c.reset}
  node scripts/tiktok-downloader.js scrape https://www.tiktok.com/@${TIKTOK_ACCOUNT}

`);
}

function showScrapeHelp(url) {
  console.log(`
${c.bright}╔══════════════════════════════════════════════════════════════╗${c.reset}
${c.bright}║           TikTok Account Downloader                        ║${c.reset}
${c.bright}╚══════════════════════════════════════════════════════════════╝${c.reset}
`);
  log.info(`Account: ${url}\n`);

  console.log(`${c.yellow}STEP 1: Navigate to account${c.reset}`);
  console.log(`chrome-devtools_navigate_page(url="${url}")`);
  console.log('');

  console.log(`${c.yellow}STEP 2: Scroll to load all videos/images${c.reset}`);
  console.log(`chrome-devtools_evaluate_script(() => { window.scrollBy(0, 1000); })`);
  console.log('');

  console.log(`${c.yellow}STEP 3: Extract ALL video and image URLs${c.reset}`);
  console.log(`chrome-devtools_evaluate_script(() => {
  const links = document.querySelectorAll('a[href*="/video/"], a[href*="/photo/"]');
  const urls = [...new Set([...links].map(a => a.href))];
  return urls.filter(u => u.includes('/video/') || u.includes('/photo/'));
})`);
  console.log('');

  console.log(`${c.yellow}FOR VIDEOS - Record using MediaRecorder:${c.reset}`);
  console.log(`chrome-devtools_evaluate_script(async () => {
  const video = document.querySelector('video');
  if (!video) return { error: 'no video found' };
  const stream = video.captureStream(60);
  const recorder = new MediaRecorder(stream, {
    mimeType: 'video/webm;codecs=vp9',
    videoBitsPerSecond: 15000000
  });
  const chunks = [];
  recorder.ondataavailable = e => { if (e.data.size) chunks.push(e.data); };
  recorder.onstop = () => {
    const blob = new Blob(chunks, { type: 'video/webm' });
    const reader = new FileReader();
    reader.onloadend = () => { window._videoBlob = reader.result; };
    reader.readAsDataURL(blob);
  };
  recorder.start();
  setTimeout(() => recorder.stop(), (video.duration + 3) * 1000);
  return { started: true, duration: video.duration };
})`);
  console.log('');

  console.log(`${c.yellow}Save recorded video:${c.reset}`);
  console.log(`chrome-devtools_evaluate_script(() => {
  if (window._videoBlob) {
    const a = document.createElement('a');
    a.href = window._videoBlob;
    a.download = 'tiktok_video.webm';
    a.click();
  }
})`);
  console.log('');

  console.log(`${c.yellow}FOR IMAGES - Extract image URLs from photo posts:${c.reset}`);
  console.log(`chrome-devtools_evaluate_script(() => {
  // Get images from photo posts (photomode images)
  const images = [];
  const imgs = document.querySelectorAll('img[src*="photomode"]');
  imgs.forEach(img => {
    if (img.naturalWidth > 500) {
      images.push(img.src.split('?')[0]);
    }
  });
  return [...new Set(images)];
})`);
  console.log('');

  console.log(`${c.yellow}Download images with curl:${c.reset}`);
  console.log(`curl -L -o image.jpg "IMAGE_URL" -H "Referer: https://www.tiktok.com/"`);
  console.log('');

  console.log(`${c.yellow}Convert video to MP4:${c.reset}`);
  console.log(`ffmpeg -i input.webm -c:v libx264 -c:a aac -movflags +faststart output.mp4`);
  console.log('');
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0 || args[0] === 'help') {
    showHelp();
    return;
  }

  const command = args[0];

  if (command === 'scrape') {
    if (args.length < 2) {
      log.error('Usage: node tiktok-downloader.js scrape <url>');
      return;
    }
    showScrapeHelp(args[1]);
  } else if (command === 'download') {
    if (args.length < 2) {
      log.error('Usage: node tiktok-downloader.js download <file.json>');
      return;
    }

    const downloader = new TikTokDownloader();
    const data = downloader.loadVideoList(args[1]);

    log.info(`Videos found: ${data.videos.length}`);
    log.info(`Images found: ${data.images.length}`);

    if (data.images.length > 0) {
      log.info('\nDownloading images...');
      await downloader.downloadImages(data.images);
    }

    log.success('Done!');
  } else {
    log.error(`Unknown command: ${command}`);
    showHelp();
  }
}

module.exports = { TikTokDownloader };

if (require.main === module) {
  main().catch(console.error);
}
