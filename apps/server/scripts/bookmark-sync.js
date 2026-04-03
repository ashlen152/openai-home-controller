#!/usr/bin/env node

const { execSync } = require('child_process');
const http = require('http');

const API_URL = process.env.API_URL || 'http://localhost:3000';

const colors = {
  reset: '\x1b[0m', bright: '\x1b[1m', green: '\x1b[32m',
  yellow: '\x1b[33m', blue: '\x1b[34m', cyan: '\x1b[36m', red: '\x1b[31m',
};

const log = (msg, color = 'reset') => console.log(`${colors[color]}${msg}${colors.reset}`);

const apiRequest = (method, path, data = null) => new Promise((resolve, reject) => {
  const url = new URL(path, API_URL);
  const req = http.request({ hostname: url.hostname, port: url.port || 3000, path: url.pathname, method, headers: { 'Content-Type': 'application/json' } }, res => {
    let body = '';
    res.on('data', c => body += c);
    res.on('end', () => { try { resolve(JSON.parse(body)); } catch { resolve(body); } });
  });
  req.on('error', reject);
  if (data) req.write(JSON.stringify(data));
  req.end();
});

const callOpenCode = (prompt, timeout = 180000) => {
  log('🤖 Calling OpenCode CLI...', 'cyan');
  try {
    const result = execSync(`opencode run "${prompt.replace(/"/g, '\\"')}" -m opencode/mimo-v2-pro-free`, { timeout, encoding: 'utf-8' });
    return result;
  } catch (error) {
    log(`❌ OpenCode error: ${error.message}`, 'red');
    throw error;
  }
};

const extractJson = (text) => {
  const patterns = [/```json\n?([\s\S]*?)\n?```/, /```\n?([\s\S]*?)\n?```/, /\{[\s\S]*"bookmarks"[\s\S]*\}/, /\{[\s\S]*"series"[\s\S]*\}/];
  for (const p of patterns) {
    const m = text.match(p);
    if (m) try { return JSON.parse(m[1] || m[0]); } catch { continue; }
  }
  return null;
};

const fetchBookmarksFromAsura = async () => {
  log('\n🌐 Fetching bookmarks from AsuraScans...', 'cyan');
  const prompt = `Browse to https://asurascans.com/bookmarks and extract your bookmarked manga list with current progress and latest chapter.

Return ONLY valid JSON:
{
  "bookmarks": [
    {
      "id": "manga-slug-from-url",
      "title": "Manga Title",
      "url": "https://asurascans.com/manga/manga-slug",
      "coverImage": "https://.../cover.jpg",
      "currentChapter": 1,
      "latestChapter": 250,
      "status": "Reading"
    }
  ]
}
Only return JSON.`;

  try {
    const response = callOpenCode(prompt, 180000);
    const data = extractJson(response);
    if (!data || !data.bookmarks || data.bookmarks.length === 0) {
      log('❌ No bookmarks found', 'red');
      return null;
    }
    return data.bookmarks;
  } catch (error) {
    log(`❌ Failed: ${error.message}`, 'red');
    return null;
  }
};

const fetchLatestChapter = async (mangaId, mangaTitle) => {
  const prompt = `Go to https://asurascans.com/manga/${mangaId}/ and find the latest chapter number and any new chapters since chapter 1.

Return JSON:
{
  "latestChapter": 250,
  "newChapters": [
    { "id": "chapter-248", "number": 248, "title": "Chapter 248", "url": "https://asurascans.com/manga/${mangaId}/chapter-248" }
  ]
}
Only return JSON.`;

  try {
    const response = callOpenCode(prompt, 120000);
    return extractJson(response);
  } catch {
    log(`  ⚠️ Could not fetch latest for ${mangaTitle}`, 'yellow');
    return null;
  }
};

const getMangaFromDb = async (externalId) => {
  try {
    const result = await apiRequest('GET', `/mangas/external/${externalId}`);
    return result.error ? null : result;
  } catch { return null; }
};

const createOrUpdateManga = async (bookmark, dryRun = false) => {
  const existing = await getMangaFromDb(bookmark.id);
  if (existing) {
    if (!dryRun) {
      await apiRequest('PATCH', `/mangas/${existing._id}`, {
        latestChapter: bookmark.latestChapter,
        lastCheckedAt: new Date().toISOString(),
        coverImage: bookmark.coverImage || existing.coverImage,
      });
    }
    return { manga: existing, isNew: false };
  } else {
    if (dryRun) return { manga: { title: bookmark.title, externalId: bookmark.id }, isNew: true };
    const result = await apiRequest('POST', '/mangas', {
      externalId: bookmark.id, title: bookmark.title, coverImage: bookmark.coverImage || '',
      url: bookmark.url, source: 'asurascans', isBookmarked: true,
      latestChapter: bookmark.latestChapter, currentChapter: bookmark.currentChapter,
      lastCheckedAt: new Date().toISOString(),
    });
    return { manga: result, isNew: true };
  }
};

const addNewEpisodes = async (mangaDbId, mangaExternalId, newChapters, dryRun = false) => {
  let added = 0;
  for (const chapter of newChapters) {
    const chapterId = chapter.id || `ch-${chapter.number}`;
    if (dryRun) {
      log(`    + Would add: Chapter ${chapter.number}`, 'yellow');
      added++;
    } else {
      try {
        await apiRequest('POST', `/mangas/${mangaDbId}/episodes`, {
          externalId: chapterId,
          title: chapter.title || `Chapter ${chapter.number}`,
          number: chapter.number,
          url: chapter.url || `https://asurascans.com/manga/${mangaExternalId}/chapter-${chapter.number}`,
          readStatus: 'unread', isNew: true,
        });
        log(`    + Added: Chapter ${chapter.number}`, 'green');
        added++;
      } catch { /* exists */ }
    }
  }
  return added;
};

const syncBookmarks = async (dryRun = false) => {
  log('\n' + '='.repeat(60), 'bright');
  log(dryRun ? '🔍 DRY RUN MODE' : '🔄 SYNCING BOOKMARKS', 'bright');
  log('='.repeat(60), 'bright');

  const bookmarks = await fetchBookmarksFromAsura();
  if (!bookmarks) { log('\n❌ Could not fetch bookmarks', 'red'); return; }

  log(`\n📚 Found ${bookmarks.length} bookmarked series\n`, 'cyan');

  const stats = { total: bookmarks.length, newManga: 0, newChapters: 0, upToDate: 0 };

  for (const bookmark of bookmarks) {
    log(`\n${'─'.repeat(50)}`, 'blue');
    log(`📖 ${bookmark.title}`, 'bright');
    log(`   ID: ${bookmark.id}`, 'dim');
    log(`   Progress: Ch.${bookmark.currentChapter} → Latest: Ch.${bookmark.latestChapter}`, 'cyan');

    const { manga, isNew } = await createOrUpdateManga(bookmark, dryRun);
    if (isNew) { stats.newManga++; log('   🆕 NEW - Will be added', 'green'); }
    else { log('   📝 Existing - Checking for new chapters...', 'yellow'); }

    if (!dryRun && manga && manga._id) {
      const latestData = await fetchLatestChapter(bookmark.id, bookmark.title);
      if (latestData && latestData.newChapters && latestData.newChapters.length > 0) {
        log(`\n   🆕 ${latestData.newChapters.length} NEW CHAPTERS!`, 'green');
        stats.newChapters += await addNewEpisodes(manga._id, bookmark.id, latestData.newChapters, dryRun);
      } else {
        stats.upToDate++;
        log('   ✅ Already up to date', 'green');
      }
    }
  }

  log('\n' + '='.repeat(60), 'bright');
  log('📊 SUMMARY', 'bright');
  log('='.repeat(60), 'bright');
  log(`   Total bookmarks: ${stats.total}`, 'reset');
  log(`   New manga: ${stats.newManga}`, 'green');
  log(`   New chapters: ${stats.newChapters}`, 'green');
  log(`   Up to date: ${stats.upToDate}`, 'cyan');
  log('='.repeat(60), 'bright');
  if (dryRun) log('\n💡 Run without --dry to apply changes.', 'yellow');
  else log('\n✅ Sync complete!', 'green');
};

const args = process.argv.slice(2);
const dryRun = args.includes('--dry') || args.includes('--preview');

if (args.includes('--help') || args.includes('-h')) {
  console.log(`
Bookmark Sync - Sync AsuraScans bookmarks with database

Usage:
  node scripts/bookmark-sync.js      Sync all bookmarks
  node scripts/bookmark-sync.js --dry Preview changes (no DB writes)

Requirements:
  - OpenCode CLI installed
  - NestJS app running on port 3000
  - Browser logged into AsuraScans
`);
  process.exit(0);
}

syncBookmarks(dryRun).catch(error => { log(`\n❌ Failed: ${error.message}`, 'red'); process.exit(1); });
