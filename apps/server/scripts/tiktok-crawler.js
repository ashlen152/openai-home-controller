#!/usr/bin/env node

require('dotenv').config();

const CDP = require('chrome-remote-interface');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const TIKTOK_ACCOUNT = process.env.TIKTOK_ACCOUNT;
const DEFAULT_PORT = parseInt(process.env.CHROME_PORT);

if (!TIKTOK_ACCOUNT) throw new Error('TIKTOK_ACCOUNT env is required');
if (!DEFAULT_PORT) throw new Error('CHROME_PORT env is required');

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

class TikTokCrawler {
  constructor(options = {}) {
    this.accountUrl = options.accountUrl || '';
    this.outputDir = options.outputDir || './tiktok_download';
    this.browserPort = options.port || 9222;
    this.client = null;
    this.videos = [];
    this.photos = [];
    this.downloadedPhotos = 0;
    this.downloadedVideos = 0;
  }

  async startBrowser() {
    try {
      this.client = await CDP({ port: this.browserPort });
      const { Page, Runtime, Network } = this.client;

      await Page.enable();
      await Network.enable();

      return true;
    } catch (err) {
      log.error(`Failed to connect to Chrome: ${err.message}`);
      log.info('Make sure Chrome is running with --remote-debugging-port=9222');
      return false;
    }
  }

  async navigate(url) {
    const { Page } = this.client;
    await Page.navigate({ url });
    await Page.loadEventFired();
  }

  async waitForSelector(selector, timeout = 10000) {
    const { Runtime, Page } = this.client;

    const check = async () => {
      const result = await Runtime.evaluate({
        expression: `document.querySelector('${selector}') !== null`,
      });
      return result.result.value;
    };

    const startTime = Date.now();
    while (Date.now() - startTime < timeout) {
      if (await check()) return true;
      await new Promise((r) => setTimeout(r, 500));
    }
    return false;
  }

  async scrollToBottom(scrollPause = 1000) {
    const { Runtime } = this.client;

    let lastHeight = 0;
    let noChangeCount = 0;
    let scrolls = 0;
    const maxScrolls = 200; // Safety limit

    while (noChangeCount < 3 && scrolls < maxScrolls) {
      try {
        const result = await Runtime.evaluate({
          expression: `
            (async () => {
              window.scrollBy(0, 2000);
              await new Promise(r => setTimeout(r, ${scrollPause}));
              return document.body.scrollHeight;
            })()
          `,
          awaitPromise: true,
        });

        if (!result || !result.result || !result.result.value) {
          log.warn('Scroll returned no height, continuing...');
          noChangeCount++;
          continue;
        }

        const newHeight = result.result.value;
        scrolls++;

        if (newHeight === lastHeight) {
          noChangeCount++;
        } else {
          noChangeCount = 0;
          lastHeight = newHeight;
        }

        if (scrolls % 10 === 0) {
          log.info(`Scrolled ${scrolls} times, height: ${newHeight}...`);
        }
      } catch (err) {
        log.warn(`Scroll error: ${err.message}`);
        noChangeCount++;
      }
    }

    return scrolls;
  }

  async extractUrls() {
    const { Runtime } = this.client;

    try {
      const result = await Runtime.evaluate({
        expression: `
          (() => {
            const links = document.querySelectorAll('a[href*="/video/"], a[href*="/photo/"]');
            const urls = [...new Set([...links].map(a => a.href))];
            const videos = urls.filter(u => u.includes('/video/') && !u.includes('/photo/'));
            const photos = urls.filter(u => u.includes('/photo/'));
            return { total: urls.length, videos, photos };
          })()
        `,
        returnByValue: true,
      });

      if (!result || !result.result || !result.result.value) {
        log.warn('No data extracted from page');
        return { total: 0, videos: [], photos: [] };
      }

      const data = result.result.value;
      this.videos = data.videos || [];
      this.photos = data.photos || [];

      return data;
    } catch (err) {
      log.error(`extractUrls error: ${err.message}`);
      return { total: 0, videos: [], photos: [] };
    }
  }

  async scrapeAccount(accountUrl) {
    log.info(`Scraping account: ${accountUrl}`);

    await this.navigate(accountUrl);
    await new Promise((r) => setTimeout(r, 2000));

    log.info('Scrolling to load all content...');
    await this.scrollToBottom(1000);

    const data = await this.extractUrls();

    log.success(`Found ${data.videos.length} videos and ${data.photos.length} photos`);

    return data;
  }

  async downloadImage(url, outputPath) {
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
          if (
            response.statusCode >= 300 &&
            response.statusCode < 400 &&
            response.headers.location
          ) {
            file.close();
            this.downloadImage(response.headers.location, outputPath).then(resolve).catch(reject);
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

  async extractPhotoImages(photoUrl) {
    await this.navigate(photoUrl);
    await new Promise((r) => setTimeout(r, 1500));

    const { Runtime } = this.client;

    const result = await Runtime.evaluate({
      expression: `
        (() => {
          const images = [];
          const imgs = document.querySelectorAll('img[src*="photomode"]');
          imgs.forEach(img => {
            if (img.naturalWidth > 500) {
              images.push(img.src.split('?')[0]);
            }
          });
          return [...new Set(images)];
        })()
      `,
    });

    return result.result.value || [];
  }

  async downloadPhotos(photoUrls, photosDir) {
    if (!fs.existsSync(photosDir)) {
      fs.mkdirSync(photosDir, { recursive: true });
    }

    log.info(`Downloading photos from ${photoUrls.length} posts...`);

    for (let i = 0; i < photoUrls.length; i++) {
      const photoUrl = photoUrls[i];
      const photoId = photoUrl.match(/\/photo\/(\d+)/)?.[1] || i;

      try {
        log.progress(i + 1, photoUrls.length, `Fetching photo ${photoId}...`);

        const imageUrls = await this.extractPhotoImages(photoUrl);

        for (let j = 0; j < imageUrls.length; j++) {
          const imageUrl = imageUrls[j];
          const fileName = `photo_${photoId}_${j + 1}.jpg`;
          const outputPath = path.join(photosDir, fileName);

          try {
            await this.downloadImage(imageUrl, outputPath);
            const stats = fs.statSync(outputPath);

            if (stats.size < 1000) {
              fs.unlinkSync(outputPath);
            } else {
              this.downloadedPhotos++;
              log.success(`Saved: ${fileName} (${(stats.size / 1024).toFixed(1)} KB)`);
            }
          } catch (err) {
            log.warn(`Failed to download image: ${err.message}`);
          }
        }
      } catch (err) {
        log.error(`Failed to fetch photo ${photoId}: ${err.message}`);
      }
    }

    log.success(`Downloaded ${this.downloadedPhotos} images from photos`);
  }

  async recordVideo(videoUrl, outputPath, duration) {
    const { Runtime, Page } = this.client;

    await this.navigate(videoUrl);
    await new Promise((r) => setTimeout(r, 1500));

    const recordScript = `
      (async () => {
        const video = document.querySelector('video');
        if (!video) return { error: 'no video' };
        
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
        const dur = video.duration || ${duration || 30};
        setTimeout(() => recorder.stop(), (dur + 3) * 1000);
        
        return { started: true, duration: dur };
      })()
    `;

    await Runtime.evaluate({ expression: recordScript, awaitPromise: true });

    const dur = await Runtime.evaluate({
      expression: 'document.querySelector("video")?.duration || 30',
    });
    const waitTime = ((dur.result.value || 30) + 5) * 1000;

    log.info(`Recording video for ${waitTime / 1000} seconds...`);
    await new Promise((r) => setTimeout(r, waitTime));

    const blobResult = await Runtime.evaluate({ expression: 'window._videoBlob' });
    const base64 = blobResult.result.value;

    if (base64) {
      const data = base64.replace(/^data:[^;]+;base64,/, '');
      const buffer = Buffer.from(data, 'base64');
      fs.writeFileSync(outputPath, buffer);
      return true;
    }

    return false;
  }

  saveUrls(filepath) {
    const data = {
      accountUrl: this.accountUrl,
      scrapedAt: new Date().toISOString(),
      videos: this.videos,
      photos: this.photos,
      totalVideos: this.videos.length,
      totalPhotos: this.photos.length,
    };
    fs.writeFileSync(filepath, JSON.stringify(data, null, 2));
    log.success(`Saved URLs to: ${filepath}`);
  }

  loadUrls(filepath) {
    const data = JSON.parse(fs.readFileSync(filepath, 'utf8'));
    this.videos = data.videos || [];
    this.photos = data.photos || [];
    this.accountUrl = data.accountUrl;
    return data;
  }

  async close() {
    if (this.client) {
      await this.client.close();
    }
  }
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0 || args[0] === 'help' || args[0] === '-h') {
    console.log(`
${c.bright}TikTok Account Crawler${c.reset}
${c.gray}(Uses Chrome DevTools Protocol)${c.reset}

${c.cyan}Usage:${c.reset}
  node scripts/tiktok-crawler.js <command> [options]

${c.cyan}Commands:${c.reset}
  scrape <url> [port]     Scrape account and save URLs
  photos <file> [port]    Download all photos from saved URLs
  videos <file> [port]    Record all videos from saved URLs
  help                   Show this help

${c.cyan}Examples:${c.reset}
  # Scrape account and save URLs
  node scripts/tiktok-crawler.js scrape https://www.tiktok.com/@${TIKTOK_ACCOUNT}

  # Download all photos
  node scripts/tiktok-crawler.js photos ${TIKTOK_ACCOUNT}_urls.json

  # Record all videos
  node scripts/tiktok-crawler.js videos ${TIKTOK_ACCOUNT}_urls.json

${c.cyan}Prerequisites:${c.reset}
  Chrome must be running with remote debugging:
  /Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome --remote-debugging-port=9222

`);
    return;
  }

  const command = args[0];

  if (command === 'scrape') {
    if (args.length < 2) {
      log.error('Usage: node tiktok-crawler.js scrape <url> [port]');
      return;
    }

    const accountUrl = args[1];
    const port = parseInt(args[2]) || 9222;
    const accountName = accountUrl.match(/@([^/]+)/)?.[1] || 'account';
    const outputDir = `./tiktok_download/${accountName}`;
    const urlFile = `${outputDir}/urls.json`;

    const crawler = new TikTokCrawler({ accountUrl, outputDir, port });

    if (!(await crawler.startBrowser())) return;

    try {
      const data = await crawler.scrapeAccount(accountUrl);
      crawler.saveUrls(urlFile);
      log.info(`\nURLs saved to: ${urlFile}`);
      log.info(`To download photos: node scripts/tiktok-crawler.js photos ${urlFile}`);
      log.info(`To record videos: node scripts/tiktok-crawler.js videos ${urlFile}`);
    } finally {
      await crawler.close();
    }
  } else if (command === 'photos') {
    if (args.length < 2) {
      log.error('Usage: node tiktok-crawler.js photos <urls.json> [port]');
      return;
    }

    const urlFile = args[1];
    const port = parseInt(args[2]) || 9222;

    const crawler = new TikTokCrawler({ port });

    if (!(await crawler.startBrowser())) return;

    try {
      const data = crawler.loadUrls(urlFile);
      const photosDir = path.join(path.dirname(urlFile), 'photos');
      await crawler.downloadPhotos(data.photos, photosDir);
      log.success(`\nDownloaded ${crawler.downloadedPhotos} photos to: ${photosDir}`);
    } finally {
      await crawler.close();
    }
  } else if (command === 'videos') {
    if (args.length < 2) {
      log.error('Usage: node tiktok-crawler.js videos <urls.json> [port]');
      return;
    }

    const urlFile = args[1];
    const port = parseInt(args[2]) || 9222;

    const crawler = new TikTokCrawler({ port });

    if (!(await crawler.startBrowser())) return;

    try {
      const data = crawler.loadUrls(urlFile);
      const videosDir = path.join(path.dirname(urlFile), 'videos');

      if (!fs.existsSync(videosDir)) {
        fs.mkdirSync(videosDir, { recursive: true });
      }

      log.info(`Recording ${data.videos.length} videos...`);

      for (let i = 0; i < data.videos.length; i++) {
        const videoUrl = data.videos[i];
        const videoId = videoUrl.match(/\/video\/(\d+)/)?.[1] || i;
        const outputPath = path.join(videosDir, `video_${videoId}.webm`);

        log.progress(i + 1, data.videos.length, `Recording video ${videoId}...`);

        try {
          const success = await crawler.recordVideo(videoUrl, outputPath);
          if (success) {
            crawler.downloadedVideos++;
            const stats = fs.statSync(outputPath);
            log.success(
              `Saved: video_${videoId}.webm (${(stats.size / 1024 / 1024).toFixed(1)} MB)`,
            );
          }
        } catch (err) {
          log.error(`Failed: ${err.message}`);
        }
      }

      log.success(`\nRecorded ${crawler.downloadedVideos} videos to: ${videosDir}`);
    } finally {
      await crawler.close();
    }
  } else {
    log.error(`Unknown command: ${command}`);
    console.log('Run with "help" for usage');
  }
}

module.exports = { TikTokCrawler };

if (require.main === module) {
  main().catch((err) => {
    log.error(`Fatal error: ${err.message}`);
    process.exit(1);
  });
}
