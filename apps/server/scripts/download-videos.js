#!/usr/bin/env node

const CDP = require('chrome-remote-interface');
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

async function main() {
  const args = process.argv.slice(2);
  if (args.length < 2) {
    console.log(`Usage: node download-videos.js <urls.json> <port>`);
    console.log(`Example: node download-videos.js tiktok_download/cuxi037/urls.json 9222`);
    return;
  }

  const urlsFile = args[0];
  const port = parseInt(args[1]) || 9222;
  const data = JSON.parse(fs.readFileSync(urlsFile, 'utf8'));
  const videos = data.videos || [];
  
  const outputDir = path.join(path.dirname(urlsFile), 'videos');
  if (!fs.existsSync(outputDir)) fs.mkdirSync(outputDir, { recursive: true });

  log.info(`Connecting to Chrome on port ${port}...`);
  
  const client = await CDP({ port });
  const { Page, Runtime } = client;
  await Promise.all([Page.enable(), Runtime.enable()]);

  let downloadedCount = 0, skippedCount = 0, errorCount = 0;

  log.info(`Recording videos from ${videos.length} posts...`);

  for (let i = 0; i < videos.length; i++) {
    const videoUrl = videos[i];
    const videoId = videoUrl.match(/\/video\/(\d+)/)?.[1] || i;

    log.progress(i + 1, videos.length, `Processing video ${videoId}...`);

    const outputPath = path.join(outputDir, `video_${videoId}.webm`);

    if (fs.existsSync(outputPath)) {
      log.warn(`Already exists, skipping: video_${videoId}.webm`);
      skippedCount++;
      continue;
    }

    try {
      // Navigate to about:blank first
      await Page.navigate({ url: 'about:blank' });
      await Page.loadEventFired();
      
      // Navigate to video URL
      await Page.navigate({ url: videoUrl });
      await Page.loadEventFired();
      
      // Wait for video to load
      await new Promise(r => setTimeout(r, 3000));

      // Get video duration and element
      const videoInfo = await Runtime.evaluate({
        expression: `
          (() => {
            const video = document.querySelector('video');
            if (!video) return null;
            return {
              duration: video.duration,
              src: video.src,
              readyState: video.readyState
            };
          })()
        `,
        returnByValue: true
      });

      if (!videoInfo.result?.value) {
        log.warn(`No video element found for ${videoId}`);
        errorCount++;
        continue;
      }

      const { duration, src } = videoInfo.result.value;
      
      if (!duration || duration === 0) {
        log.warn(`Video not ready for ${videoId}, duration: ${duration}`);
        errorCount++;
        continue;
      }

      log.info(`Video duration: ${duration.toFixed(1)}s`);

      // Use MediaRecorder to capture the video
      // This is done via evaluate_script in CDP
      const recordScript = `
        (async () => {
          const video = document.querySelector('video');
          if (!video) throw new Error('No video element');

          // Wait for video to be ready
          if (video.readyState < 2) {
            await new Promise(r => video.addEventListener('loadeddata', r, { once: true }));
          }

          const stream = video.captureStream(60);
          const chunks = [];
          
          const recorder = new MediaRecorder(stream, {
            mimeType: 'video/webm;codecs=vp9',
            videoBitsPerSecond: 15000000
          });
          
          recorder.ondataavailable = e => {
            if (e.data.size > 0) chunks.push(e.data);
          };
          
          return new Promise((resolve, reject) => {
            recorder.onstop = () => {
              const blob = new Blob(chunks, { type: 'video/webm' });
              const reader = new FileReader();
              reader.onload = () => resolve(Array.from(new Uint8Array(reader.result)));
              reader.onerror = reject;
              reader.readAsArrayBuffer(blob);
            };
            
            recorder.onerror = reject;
            recorder.start();
            
            // Record for duration + 3 seconds buffer
            setTimeout(() => recorder.stop(), (video.duration + 3) * 1000);
          });
        })()
      `;

      const result = await Runtime.evaluate({
        expression: recordScript,
        returnByValue: true,
        awaitPromise: true,
        timeout: 120000 // 2 minute timeout
      });

      if (!result.result?.value) {
        log.warn(`Recording failed for ${videoId}`);
        errorCount++;
        continue;
      }

      // Write video data
      const videoData = new Uint8Array(result.result.value);
      fs.writeFileSync(outputPath, Buffer.from(videoData));

      const stats = fs.statSync(outputPath);
      if (stats.size < 1000) {
        fs.unlinkSync(outputPath);
        log.warn(`Recording too small: ${stats.size} bytes`);
        errorCount++;
      } else {
        downloadedCount++;
        const sizeMB = (stats.size / (1024 * 1024)).toFixed(2);
        log.success(`Saved video_${videoId}.webm (${sizeMB} MB)`);
      }

    } catch (err) {
      log.error(`Error: ${err.message}`);
      errorCount++;
    }

    if ((i + 1) % 10 === 0) {
      log.info(`Progress: ${downloadedCount} downloaded, ${skippedCount} skipped, ${errorCount} errors`);
    }
  }

  log.success(`\nComplete! Downloaded: ${downloadedCount}, Skipped: ${skippedCount}, Errors: ${errorCount}`);
  log.info(`Output: ${outputDir}`);
  await client.close();
}

if (require.main === module) {
  main().catch(err => { log.error(`Fatal: ${err.message}`); process.exit(1); });
}
